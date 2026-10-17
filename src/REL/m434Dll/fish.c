#include "game/audio.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"

#include "REL/m434Dll.h"
#include "ext_math.h"
#include <string.h>
#include "version.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

typedef struct FishAttractionEffect {
    s32 remainingFrames;
    s32 fadeFrames;
    s32 ownerFishIndex;
    float currentRadius;
    float targetRadius;
    Vec position;
} FishAttractionEffect;

OMOBJ *fishManagerObject;
FishAttractionEffect fishAttractionEffects[32];
float fishGridInfluence[1024];
u8 fishGridValidCells[1024];
u16 fishGridOccupancy[1024];

s32 fishModelAssets[] = { 0x410005, 0x410006 };

void InitializeFishObject(OMOBJ *object);

void InitializeFishManager(HUPROCESS *objman)
{
    float distanceFromCenter;
    Vec samplePosition;
    s32 rowIndex;
    s32 cellIndex;
    s32 columnIndex;
    OMOBJ *fishManager;
    for (rowIndex = 0; rowIndex < 32; rowIndex++) {
        fishAttractionEffects[rowIndex].fadeFrames = fishAttractionEffects[rowIndex].remainingFrames = 0;
        fishAttractionEffects[rowIndex].currentRadius = fishAttractionEffects[rowIndex].targetRadius = 0;
        fishAttractionEffects[rowIndex].ownerFishIndex = -1;
    }
    samplePosition.y = 0;
    samplePosition.z = -650;
    for (rowIndex = 0; rowIndex < 32; rowIndex++) {
        cellIndex = rowIndex * 32;
        samplePosition.x = -650.0f;
        for (columnIndex = 0; columnIndex < 32; columnIndex++, cellIndex++) {
            fishGridInfluence[cellIndex] = 0;
            fishGridValidCells[cellIndex] = 0;
            fishGridOccupancy[cellIndex] = 0;
            distanceFromCenter = VECMag(&samplePosition);
            if (distanceFromCenter < 522.0f) {
                fishGridValidCells[cellIndex] = 1;
            }
            samplePosition.x += (1300.0f / 32.0f);
        }
        samplePosition.z += (1300.0f / 32.0f);
    }
    fishManager = fishManagerObject = omAddObjEx(objman, 30, 1, 0, -1, InitializeFishObject);
    fishManager->work[0] = 0;
}

void CleanupFishState(void) { }

typedef struct FishWork {
    s16 fishType;
    s16 modelId;
    s16 behaviorState;
    s16 unreferencedFishState6;
    s16 jumpStarted;
    s16 unreferencedFishSpawnDelay;
    s16 attractionCooldown;
    s16 vibrationPlayer;
    s32 isActive;
    s32 isJumping;
    s32 gridCellIndex;
    Vec position;
    Vec movementVector;
    Vec movement;
    Vec jumpProgress;
    float heading;
} FishWork;

void DrawFishManager(HU3DMODEL *model, Mtx mtx);
void UpdateFishObject(OMOBJ *object);

s32 UpdateFishGridMembership(s32 previousCellIndex, Vec *position);

void InitializeFishObject(OMOBJ *object)
{
    float heading;
    float spawnDistance;
    s16 modelIds[2];
    FishWork *fish;
    s32 fishIndex;
    s32 fishType;
    FishWork *fishArray;
    fishArray = HuMemDirectMallocNum(HEAP_HEAP, 20 * sizeof(FishWork), HU_MEMNUM_OVL);
    object->data = fishArray;
    memset(fishArray, 0, 20 * sizeof(FishWork));
    for (fishIndex = 0; fishIndex < 2; fishIndex++) {
        modelIds[fishIndex] = Hu3DModelCreateFile(fishModelAssets[fishIndex]);
    }
    fish = fishArray;
    for (fishIndex = 0; fishIndex < 20; fishIndex++, fish++) {
        fishType = 0;
        if (fishIndex >= 15) {
            fishType++;
        }
        fish->fishType = fishType + 1;
        fish->modelId = Hu3DModelLink(modelIds[fishType]);
        Hu3DModelCameraSet(fish->modelId, 4);
        Hu3DModelLayerSet(fish->modelId, 4);
        heading = frandmod(360);
        spawnDistance = frandmod(464);
        fish->heading = heading;
        fish->position.x = spawnDistance * sind(heading);
        fish->position.y = -50.0f;
        fish->position.z = spawnDistance * cosd(heading);
        fish->movement.z = frandmod(360);
        fish->jumpProgress.y = 1.0f;
        fish->jumpProgress.z = 0;
        fish->isActive = 1;
        fish->isJumping = 0;
        fish->vibrationPlayer = -1;
        fish->attractionCooldown = 0;
        fish->gridCellIndex = UpdateFishGridMembership(-1, &fish->position);
        fish->unreferencedFishSpawnDelay = 20;
    }
    for (fishIndex = 0; fishIndex < 2; fishIndex++) {
        if (modelIds[fishIndex] > 0) {
            Hu3DModelKill(modelIds[fishIndex]);
        }
    }
    object->work[1] = 140;
    object->mdlId[0] = Hu3DHookFuncCreate(DrawFishManager);
    Hu3DModelCameraSet(object->mdlId[0], 8);
    Hu3DModelLayerSet(object->mdlId[0], 7);
    object->objFunc = UpdateFishObject;
}

void UpdateFishManager(OMOBJ *object);

void DrawFishManager(HU3DMODEL *model, Mtx mtx)
{
    if (!omPauseChk()) {
        UpdateFishManager(fishManagerObject);
    }
}

void UpdateFishObject(OMOBJ *object) { }

void UpdateFishAttractionGrid(void);
s32 GetFishAttractionStrength(FishWork *fish);
void FindFishDestination(FishWork *fish, Vec *destination);

void UpdateFishManager(OMOBJ *object)
{
    Vec positionDifference;
    float distance;
    float movementSpeed;
    FishWork *fish;
    s32 shouldChangeTarget;
    s32 crossedWaterline;
    s32 fishIndex;
    s32 fishCountBelowStateTwo;
    s32 crossedWaterlineAfterMove;
    s32 fishTargetFound;
    FishWork *fishArray;
    fishArray = object->data;
    UpdateFishAttractionGrid();
    if (object->work[1]) {
        object->work[1]--;
        return;
    }
    fishCountBelowStateTwo = 0;
    fish = fishArray;
    for (fishIndex = 0; fishIndex < 20; fishIndex++, fish++) {
        shouldChangeTarget = 0;
        fishTargetFound = 0;
        switch (fish->fadeFrames) {
            case 0:
                if (fish->currentRadius == 0) {
                    fishTargetFound = GetFishAttractionStrength(fish);
                    if (fishTargetFound) {
                        shouldChangeTarget = 1;
                        fish->currentRadius = 60;
                    }
                }
                else {
                    fish->currentRadius--;
                }
                if (fish->currentRadius == 0 && fish->jumpProgress.y > fish->jumpProgress.z) {
                    shouldChangeTarget = 1;
                }
                else {
                    VECSubtract(&fish->movementVector, &fish->position, &positionDifference);
                    distance = VECMag(&positionDifference);
                    if (distance < 50.0f) {
                        shouldChangeTarget = 1;
                    }
                }
                if (shouldChangeTarget) {
                    if (fish->currentRadius == 0) {
                        distance = frandmod(1000);
                        movementSpeed = frandmod(1000);
                        if (distance > movementSpeed) {
                            movementSpeed = distance;
                        }
                        movementSpeed = 0.53f * movementSpeed;
                        distance = fish->heading + ((0.06f * frandmod(1000)) - 30);
                        fish->heading = distance;
                        fish->movementVector.x = movementSpeed * sind(distance);
                        fish->movementVector.z = movementSpeed * cosd(distance);
                    }
                    else {
                        FindFishDestination(fish, &fish->movementVector);
                    }
                    fish->movementVector.y = fish->position.y;
                    fish->jumpProgress.y = 0;
                    fish->jumpProgress.z = frandmod(200) + 100.0f;
                    fish->movement.y = 200;
                    if (fish->currentRadius) {
                        fish->movement.y *= 2.0f;
                    }
                }
                VECSubtract(&fish->movementVector, &fish->position, &positionDifference);
                fish->jumpProgress.x = atan2d(positionDifference.x, positionDifference.z);
                if (fish->currentRadius != 0) {
                    fish->movement.z = approachAngle(fish->movement.z, fish->jumpProgress.x, 0.15f);
                    fish->heading = atan2d(fish->movementVector.x, fish->movementVector.z);
                }
                else {
                    fish->movement.z = approachAngle(fish->movement.z, fish->jumpProgress.x, 0.07f);
                }
                if (fish->currentRadius != 0) {
                    fish->movement.y += (300.0f - fish->movement.y) * 0.02f;
                }
                else {
                    fish->movement.y += (75.0f - fish->movement.y) * 0.02f;
                }
                fish->movement.x += (fish->movement.y - fish->movement.x) * 0.2f;
                fish->position.x += sind(fish->movement.z) * ((1.0f / REFRESH_RATE) * fish->movement.x);
                fish->position.z += cosd(fish->movement.z) * ((1.0f / REFRESH_RATE) * fish->movement.x);
                fish->jumpProgress.y += (1.0f / REFRESH_RATE) * fish->movement.x;
                fish->gridCellIndex = UpdateFishGridMembership(fish->gridCellIndex, &fish->position);
                Hu3DModelPosSetV(fish->modelId, &fish->position);
                Hu3DModelRotSet(fish->modelId, 0, fish->movement.z, 0);
                break;

            case 1:
                fish->movement.z += 1.0f / REFRESH_RATE * 180.0f;
                Hu3DModelPosSetV(fish->modelId, &fish->position);
                Hu3DModelRotSet(fish->modelId, 0, fish->movement.z, 0);
                if (getRoundState() == 4) {
                    Hu3DModelAttrSet(fish->modelId, HU3D_ATTR_DISPOFF);
                }
                break;

            case 2:
                if (fish->isJumping) {
                    if (fish->jumpStarted == 0) {
                        positionDifference.x = (0.1f * frandmod(1000) + 500.0f) - 50.0f;
                        positionDifference.y = fish->position.y;
                        positionDifference.z = (0.1f * frandmod(1000) + 500.0f) - 50.0f;
                        if (fish->position.x < 0.0f) {
                            positionDifference.x = -positionDifference.x;
                        }
                        VECSubtract(&positionDifference, &fish->position, &positionDifference);
                        fish->jumpProgress.x = fish->movement.z = atan2d(positionDifference.x, positionDifference.z);
                        fish->movementVector.x = (1.0f / REFRESH_RATE) * positionDifference.x;
                        fish->movementVector.y = 400*(1.0f/REFRESH_RATE);
                        fish->movementVector.z = (1.0f / REFRESH_RATE) * positionDifference.z;
                        fish->jumpStarted++;
                        HuAudFXPlay(1734);
                    }
                    crossedWaterline = 0;
                    if (fish->position.y <= -15.000001f) {
                        crossedWaterline = 1;
                    }
                    fish->movementVector.y += -980*(1.0f/REFRESH_RATE)*(1.0f/REFRESH_RATE);
                    VECAdd(&fish->position, &fish->movementVector, &fish->position);
                    if (!crossedWaterline && fish->position.y <= -15.000001f) {
                        ClampPositionToWaterline(&fish->position);
                        if (fish->vibrationPlayer >= 0) {
                            omVibrate(fish->vibrationPlayer, 12, 6, 6);
                            fish->vibrationPlayer = -1;
                        }
                        HuAudFXPlay(1741);
                    }
                    positionDifference = fish->movementVector;
                    positionDifference.y = 0;
                    distance = VECMag(&positionDifference);
                    distance = atan2d(fish->movementVector.y, distance);
                    Hu3DModelPosSetV(fish->modelId, &fish->position);
                    Hu3DModelRotSet(fish->modelId, -distance, fish->movement.z, 0);
                    if (fish->position.y <= -50.0) {
                        fish->isJumping = 0;
                        fish->jumpProgress.z = 0;
                        fish->jumpProgress.y = 1;
                        fish->fadeFrames++;
                    }
                }

                break;

            case 3:
                if (fish->jumpProgress.y > fish->jumpProgress.z) {
                    shouldChangeTarget = 1;
                }
                VECSubtract(&fish->movementVector, &fish->position, &positionDifference);
                distance = VECMag(&positionDifference);
                if (distance < 30.000002f) {
                    shouldChangeTarget = 1;
                }
                if (shouldChangeTarget) {
                    distance = frandmod(360);
                    movementSpeed = (0.08f * frandmod(1000)) + 20;
                    fish->movementVector.x = movementSpeed * sind(distance);
                    fish->movementVector.z = movementSpeed * cosd(distance);
                    fish->movementVector.y = -50;
                    fish->position.y = fish->movementVector.y;
                    fish->movementVector.x += 480.00003f;
                    fish->movementVector.z += 480.00003f;
                    if (fish->position.x < 0.0f) {
                        fish->movementVector.x *= -1.0f;
                    }
                    fish->jumpProgress.y = 0;
                    fish->jumpProgress.z = frandmod(100) + 100.0f;
                    fish->movement.y = 100;
                    if (frandmod(100) < 5) {
                        fish->fadeFrames++;
                        fish->jumpStarted = 0;
                        HuAudFXPlay(1740);
                    }
                }
                VECSubtract(&fish->movementVector, &fish->position, &positionDifference);
                fish->jumpProgress.x = atan2d(positionDifference.x, positionDifference.z);
                fish->movement.z = approachAngle(fish->movement.z, fish->jumpProgress.x, 0.07f);
                fish->movement.y += (50.0f - fish->movement.y) * 0.02f;
                fish->movement.x += (fish->movement.y - fish->movement.x) * 0.2f;
                fish->position.x += sind(fish->movement.z) * ((1.0f / REFRESH_RATE) * fish->movement.x);
                fish->position.z += cosd(fish->movement.z) * ((1.0f / REFRESH_RATE) * fish->movement.x);
                fish->jumpProgress.y += (1.0f / REFRESH_RATE) * fish->movement.x;
                Hu3DModelPosSetV(fish->modelId, &fish->position);
                Hu3DModelRotSet(fish->modelId, 0, fish->movement.z, 0);

                break;

            case 4:
                if (fish->jumpStarted == 0) {
                    positionDifference.x = (0.1f * frandmod(1000) + 480.00003f) - 50.0f;
                    positionDifference.y = fish->position.y;
                    positionDifference.z = (0.1f * frandmod(1000) + 480.00003f) - 50.0f;
                    if (fish->position.x < 0) {
                        positionDifference.x = -positionDifference.x;
                    }
                    VECSubtract(&positionDifference, &fish->position, &positionDifference);
                    fish->jumpProgress.x = fish->movement.z = atan2d(positionDifference.x, positionDifference.z);
                    fish->movementVector.x = (1.0f / REFRESH_RATE) * positionDifference.x;
                    fish->movementVector.y = 400*(1.0f/REFRESH_RATE);
                    fish->movementVector.z = (1.0f / REFRESH_RATE) * positionDifference.z;
                    fish->jumpStarted++;
                }
                crossedWaterline = crossedWaterlineAfterMove = 0;
                if (fish->position.y <= -15.000001f) {
                    crossedWaterline = 1;
                }
                fish->movementVector.y += -980*(1.0f/REFRESH_RATE)*(1.0f/REFRESH_RATE);
                VECAdd(&fish->position, &fish->movementVector, &fish->position);
                if (fish->position.y <= -15.000001f) {
                    crossedWaterlineAfterMove = 1;
                }
                if (crossedWaterline != crossedWaterlineAfterMove) {
                    ClampPositionToWaterline(&fish->position);
                    if (fish->movementVector.y < 0.0f) {
                        HuAudFXPlay(1741);
                    }
                }
                positionDifference = fish->movementVector;
                positionDifference.y = 0;
                distance = VECMag(&positionDifference);
                distance = atan2d(fish->movementVector.y * 0.3f, distance);
                Hu3DModelPosSetV(fish->modelId, &fish->position);
                Hu3DModelRotSet(fish->modelId, -distance, fish->movement.z, 0);
                if (fish->position.y <= -50.0) {
                    fish->jumpProgress.z = 0;
                    fish->jumpProgress.y = 1;
                    fish->fadeFrames--;
                }
                break;
        }
        if (fish->fadeFrames < 2) {
            fishCountBelowStateTwo++;
        }
    }
    if (fishCountBelowStateTwo == 0) {
        SetRoundFinished();
    }
}

void UpdateFishAttractionGrid(void)
{
    Vec gridPosition;
    Vec positionDifference;
    float influenceAtCell;
    float inverseInfluenceRadius;

    FishAttractionEffect *fish;
    s32 fishIndex;
    s32 row;
    s32 minColumn;
    s32 column;
    s32 cellIndex;
    s32 maxColumn;
    s32 maxRow;

    fish = fishAttractionEffects;
    fish = fishAttractionEffects;
    for (fishIndex = 0; fishIndex < 32; fishIndex++, fish++) {
        if (fish->remainingFrames == 0) {
            continue;
        }
        fish->remainingFrames--;
        if (fish->fadeFrames) {
            fish->fadeFrames--;
            fish->currentRadius += (fish->targetRadius - fish->currentRadius) * 0.2f;
        }
        else {
            if (fish->remainingFrames < 100.0f) {
                fish->currentRadius *= 0.98f;
            }
        }
    }
    for (fishIndex = 0; fishIndex < 1024; fishIndex++) {
        fishGridInfluence[fishIndex] = 0;
    }
    fish = fishAttractionEffects;
    for (fishIndex = 0; fishIndex < 32; fishIndex++, fish++) {
        if (fish->remainingFrames == 0) {
            continue;
        }
        minColumn = (16.0f / 650.0f) * (650.0f + (fish->position.x - fish->currentRadius));
        maxColumn = 1.0f + (minColumn + ((16.0f / 650.0f) * (2.0f * fish->currentRadius)));
        row = (16.0f / 650.0f) * (650.0f + (fish->position.z - fish->currentRadius));
        maxRow = 1.0f + (row + ((16.0f / 650.0f) * (2.0f * fish->currentRadius)));
        if (minColumn < 0) {
            minColumn = 0;
        }
        if (maxColumn > 32) {
            maxColumn = 32;
        }
        if (row < 0) {
            row = 0;
        }
        if (maxRow > 32) {
            maxRow = 32;
        }
        inverseInfluenceRadius = 1.0f / fish->currentRadius;
        gridPosition.y = 0;
        gridPosition.z = ((1300.0f / 32.0f) * row) - 650.0f;
        while (row < maxRow) {
            gridPosition.x = ((1300.0f / 32.0f) * minColumn) - 650.0f;
            cellIndex = minColumn + (32 * row);
            for (column = minColumn; column < maxColumn; column++, cellIndex++) {
                VECSubtract(&fish->position, &gridPosition, &positionDifference);
                influenceAtCell = 1.0f - (inverseInfluenceRadius * VECMag(&positionDifference));
                if (influenceAtCell > 0.0f) {
                    fishGridInfluence[cellIndex] += influenceAtCell;
                }
                gridPosition.x += (1300.0f / 32.0f);
            }
            gridPosition.z += (1300.0f / 32.0f);
            row++;
        }
    }
}

void RemoveFishGridMembership(s32 cellIndex);

s32 UpdateFishGridMembership(s32 previousCellIndex, Vec *position)
{
    s32 neighborIndex;
    s32 cellRow;
    s32 cellColumn;
    s32 neighborColumn;
    s32 neighborRow;
    s32 neighborOffsets[9][2] = { -1, -1, 0, -1, 1, -1, -1, 0, 0, 0, 1, 0, -1, 1, 0, 1, 1, 1 };
    RemoveFishGridMembership(previousCellIndex);
    previousCellIndex = -1;
    cellColumn = (16.0f / 650.0f) * (650.0f + position->x);
    cellRow = (16.0f / 650.0f) * (650.0f + position->z);
    if (cellColumn >= 0 && cellColumn < 32 && cellRow >= 0 && cellRow < 32) {
        previousCellIndex = cellColumn + (32 * cellRow);
        for (neighborIndex = 0; neighborIndex < 9; neighborIndex++) {
            neighborColumn = cellColumn + neighborOffsets[neighborIndex][0];
            neighborRow = cellRow + neighborOffsets[neighborIndex][1];
            if (neighborColumn >= 0 && neighborColumn < 32 && neighborRow >= 0 && neighborRow < 32) {
                fishGridOccupancy[neighborColumn + (32 * neighborRow)]++;
            }
        }
    }
    return previousCellIndex;
}

void RemoveFishGridMembership(s32 cellIndex)
{
    s32 neighborIndex;
    s32 neighborColumn;
    s32 neighborRow;
    s32 cellColumn;
    s32 cellRow;
    s32 neighborOffsets[9][2] = { -1, -1, 0, -1, 1, -1, -1, 0, 0, 0, 1, 0, -1, 1, 0, 1, 1, 1 };
    if (cellIndex < 0 || cellIndex >= 1024) {
        return;
    }
    cellColumn = cellIndex % 32;
    cellRow = (cellIndex - cellColumn) / 32;
    for (neighborIndex = 0; neighborIndex < 9; neighborIndex++) {
        neighborColumn = cellColumn + neighborOffsets[neighborIndex][0];
        neighborRow = cellRow + neighborOffsets[neighborIndex][1];
        if (neighborColumn >= 0 && neighborColumn < 32 && neighborRow >= 0 && neighborRow < 32) {
            fishGridOccupancy[neighborColumn + (32 * neighborRow)]--;
        }
    }
}

s32 GetFishAttractionStrength(FishWork *fish)
{
    float influenceStrength;
    Vec positionDifference;
    FishAttractionEffect *fish;
    s32 fishInfluence;
    s32 fishIndex;
    s32 maxInfluence;
    fish = fishAttractionEffects;
    maxInfluence = 0;
    fish = fishAttractionEffects;
    for (fishIndex = 0; fishIndex < 32; fishIndex++, fish++) {
        if (fish->remainingFrames == 0) {
            continue;
        }
        VECSubtract(&fish->position, &fish->position, &positionDifference);
        influenceStrength = 1.0f - (VECMag(&positionDifference) / fish->currentRadius);
        if (influenceStrength < 0.3f) {
            continue;
        }
        fishInfluence = 1;
        if (fish->fadeFrames) {
            fishInfluence++;
        }
        if (maxInfluence < fishInfluence) {
            maxInfluence = fishInfluence;
        }
    }
    return maxInfluence;
}

void FindFishDestination(FishWork *fish, Vec *destination)
{
    Vec candidatePosition;
    Vec bestPosition;
    Vec positionDifference;
    float candidateDistance;
    float bestDistance;
    s32 rowIndex;
    s32 row;
    s32 columnIndex;
    bestPosition.x = bestPosition.y = bestPosition.z = 0;
    bestDistance = 5000;
    candidatePosition.y = 0;
    candidatePosition.z = -650;
    for (row = 0; row < 32; row++) {
        rowIndex = 32 * row;
        candidatePosition.x = -650;
        for (columnIndex = 0; columnIndex < 32; columnIndex++, rowIndex++) {
            if (fishGridInfluence[rowIndex] < 0.15f && fishGridValidCells[rowIndex]) {
                VECSubtract(&fish->position, &candidatePosition, &positionDifference);
                candidateDistance = VECMag(&positionDifference);
                if (candidateDistance > 81.25f) {
                    candidateDistance += 162.5f * fishGridOccupancy[rowIndex];
                    if (candidateDistance < bestDistance) {
                        bestDistance = candidateDistance;
                        bestPosition = candidatePosition;
                    }
                }
            }
            candidatePosition.x += (1300.0f / 32.0f);
        }
        candidatePosition.z += (1300.0f / 32.0f);
    }
    bestPosition.x += ((1300.0f / 32000.0f) * frandmod(1000)) - (1300.0f / 64.0f);
    bestPosition.z += ((1300.0f / 32000.0f) * frandmod(1000)) - (1300.0f / 64.0f);
    *destination = bestPosition;
}

s32 FindFishInCone(Vec *origin, float minimumDistance, float maximumDistance, float facingAngle, float minimumAngle, float maximumAngle, s32 catchFish, s32 playerIndex)
{
    float angleDifference;
    float distance;
    Vec positionDifference;
    FishWork *fish;
    s32 matchingFishIndex;
    s32 fishIndex;
    FishWork *fishArray;
    fishArray = fishManagerObject->data;
    matchingFishIndex = -1;
    fish = fishArray;
    for (fishIndex = 0; fishIndex < 20; fishIndex++, fish++) {
        if (fish->isActive == 0) {
            continue;
        }
        VECSubtract(&fish->position, origin, &positionDifference);
        distance = VECMag(&positionDifference);
        if (distance < minimumDistance || distance > maximumDistance) {
            continue;
        }
        angleDifference = atan2d(positionDifference.x, positionDifference.z);
        angleDifference = getAngleDifference(facingAngle, angleDifference);
        if (angleDifference < minimumAngle || angleDifference > maximumAngle) {
            continue;
        }
        matchingFishIndex++;
        if (catchFish) {
            fish->isJumping = 1;
            fish->isActive = 0;
            fish->behaviorState = 1;
            RemoveFishGridMembership(fish->gridCellIndex);
            fish->gridCellIndex = -1;
            fish->vibrationPlayer = playerIndex;
            HuAudFXPlay(1733);
            matchingFishIndex = fishIndex;
            break;
        }
    }
    if (!catchFish) {
        matchingFishIndex++;
    }
    return matchingFishIndex;
}

void SetFishPosition(s16 fishIndex, Vec *position)
{
    FishWork *fish;
    FishWork *fishArray = fishManagerObject->data;
    if (fishIndex < 0 || fishIndex >= 20) {
        return;
    }
    fish = &fishArray[fishIndex];
    fish->position = *position;
}

void RecordCaughtFish(s16 fishIndex)
{
    FishWork *fish;
    s32 playerIndex;
    FishWork *fishArray = fishManagerObject->data;
    if (fishIndex < 0 || fishIndex >= 20) {
        return;
    }
    fish = &fishArray[fishIndex];
    fish->behaviorState = 2;
    fish->jumpStarted = 0;
    if (fish->position.x >= 0.0f) {
        playerIndex = 1;
    }
    else {
        playerIndex = 0;
    }
    AddTeamScore(playerIndex, fish->fishType);
}

void CreateFishAttraction(s32 ownerFishIndex, Vec *position, float attractionStrength)
{
    FishAttractionEffect *effect;
    s32 effectIndex;
    effect = fishAttractionEffects;
    for (effectIndex = 0; effectIndex < 32; effectIndex++, effect++) {
        if (effect->remainingFrames && effect->ownerFishIndex == ownerFishIndex) {
            effect->remainingFrames *= 0.8f;
        }
    }
    effect = fishAttractionEffects;
    for (effectIndex = 0; effectIndex < 32; effectIndex++, effect++) {
        if (!effect->remainingFrames) {
            effect->fadeFrames = 0.5 * REFRESH_RATE;
            effect->remainingFrames = (REFRESH_RATE * (3.0f * attractionStrength)) + 2.5f * REFRESH_RATE;
            effect->ownerFishIndex = ownerFishIndex;
            effect->position = *position;
            effect->currentRadius = 0;
            effect->targetRadius = 150.0f + (250.0f * attractionStrength);
            break;
        }
    }
}

s32 GetGridCellForPosition(Vec *position)
{
    s32 gridColumn;
    s32 gridRow;
    s32 gridCellIndex = -1;
    gridColumn = (16.0f / 650.0f) * (670.3125f + position->x);
    gridRow = (16.0f / 650.0f) * (670.3125f + position->z);
    if (gridColumn >= 0 && gridColumn < 32 && gridRow >= 0 && gridRow < 32) {
        gridCellIndex = gridColumn + (32 * gridRow);
    }
    return gridCellIndex;
}

s32 GetPositionForGridCell(s32 cellIndex, Vec *position)
{
    s32 gridColumn;
    s32 gridRow;
    if (cellIndex < 0 || cellIndex >= 1024) {
        position->x = position->y = position->z = 0;
        return 0;
    }
    gridColumn = cellIndex % 32;
    gridRow = (cellIndex - gridColumn) / 32;
    position->x = ((1300.0f / 32.0f) * gridColumn) - 670.3125f;
    position->y = 0;
    position->z = ((1300.0f / 32.0f) * gridRow) - 670.3125f;
    return 1;
}

s32 GetGridCellFishCount(s32 cellIndex)
{
    s32 neighborOffsets[9][2] = { -1, -1, 0, -1, 1, -1, -1, 0, 0, 0, 1, 0, -1, 1, 0, 1, 1, 1 };
    if (cellIndex < 0 || cellIndex >= 1024) {
        return 0;
    }
    return fishGridOccupancy[cellIndex];
}

s32 FindLeastCrowdedGridCell(Vec *position)
{
    s32 neighborOffsets[9][2] = { -1, -1, 0, -1, 1, -1, -1, 0, 0, 0, 1, 0, -1, 1, 0, 1, 1, 1 };
    Vec cellPosition;
    Vec positionDifference;
    s32 rowIndex;
    s32 cellIndex;
    s32 columnIndex;
    s32 nearbyFishCount;
    s32 leastCrowdedCellIndex = -1;
    float crowdingScore;
    float leastCrowdedScore = 5000;
    cellPosition.y = 0;
    cellPosition.z = -650;
    for (rowIndex = 0; rowIndex < 32; rowIndex++) {
        cellIndex = rowIndex * 32;
        cellPosition.x = -650;
        for (columnIndex = 0; columnIndex < 32; columnIndex++, cellIndex++) {
            nearbyFishCount = fishGridOccupancy[cellIndex];
            if (nearbyFishCount > 0) {
                VECSubtract(position, &cellPosition, &positionDifference);
                crowdingScore = VECMag(&positionDifference) + (0.2f * frandmod(1000));
                crowdingScore -= 50.0f * nearbyFishCount;
                if (crowdingScore < leastCrowdedScore) {
                    leastCrowdedScore = crowdingScore;
                    leastCrowdedCellIndex = cellIndex;
                }
            }
            cellPosition.x += (1300.0f / 32.0f);
        }
        cellPosition.z += (1300.0f / 32.0f);
    }
    return leastCrowdedCellIndex;
}
