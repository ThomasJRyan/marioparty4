#include "game/audio.h"
#include "game/chrman.h"
#include "game/hu3d.h"
#include "game/gamework_data.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "ext_math.h"
#include "version.h"

#include "REL/m434Dll.h"

#ifndef __MWERKS__
#include "game/frand.h"
#include <string.h>
#endif

typedef struct PlayerMotionConfig {
    u16 motionId;
    u16 overlayMotionId;
    float blendTime;
    float overlayTime;
    u32 motionAttributes;
} PlayerMotionConfig;

OMOBJ *playerObjects[4];

char *hookDummyNames[8] = {
    "dmy-itemhook-r",
    "dmy-itemhook-r",
    "dmy-itemhook-r",
    "dmy-itemhook-r",
    "dmy-itemhook-r",
    "dmy-itemhook-r",
    "dmy-itemhook-r",
    "dmy-itemhook-r",
};

char *hookClusterNames[8] = {
    "itemhook-r",
    "itemhook-r",
    "itemhook-r",
    "itemhook-r",
    "itemhook-r",
    "itemhook-r",
    "itemhook-r",
    "itemhook-r",
};

s32 playerBodyModelIds[2] = { 0x410003, 0x410004 };

Vec playerSpawnPositions[2] = { { -550, 0, 550 }, { 550, 0, 550 } };

u32 playerMotionAssets[10] = { 0, 2, 23, 133, 0x41000E, 0x410016, 0x41001E, 0x410026, 0x41002E, 0x410036 };

PlayerMotionConfig playerMotionConfigs[9] = {
    { 0, 4, 0.2f * REFRESH_RATE, 0, HU3D_MOTATTR_LOOP },
    { 1, 6, 0.1f * REFRESH_RATE, 0, HU3D_MOTATTR_LOOP },
    { 0, 8, 0.1f * REFRESH_RATE, 0, HU3D_MOTATTR_NONE },
    { 0, 5, 0.1f * REFRESH_RATE, 0, HU3D_MOTATTR_LOOP },
    { 1, 5, 0.1f * REFRESH_RATE, 0, HU3D_MOTATTR_LOOP },
    { 0, 8, 0.1f * REFRESH_RATE, 0.5f * REFRESH_RATE, HU3D_MOTATTR_NONE },
    { 0, 9, 0.1f * REFRESH_RATE, 0, HU3D_MOTATTR_NONE },
    { 2, 0, 0.2f * REFRESH_RATE, 0, HU3D_MOTATTR_NONE },
    { 3, 0, 0.2f * REFRESH_RATE, 0, HU3D_MOTATTR_NONE },
};

void InitializePlayerObject(OMOBJ *object);

void CreatePlayerObjects(HUPROCESS *objman)
{
    s16 playerIndex;
    OMOBJ *playerObject;
    s16 playerTeam;
    s32 teamPlayerCounts[2] = { 0, 0 };
    omMakeGroupEx(objman, 0, 4);
    omGetGroupMemberListEx(objman, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObject = playerObjects[playerIndex] = omAddObjEx(objman, 40, 2, 10, 0, InitializePlayerObject);
        playerObject->work[0] = playerIndex;
        playerTeam = getPlayerTeam(playerIndex);
        playerObject->work[1] = (playerTeam * 2) + teamPlayerCounts[playerTeam];
        teamPlayerCounts[playerTeam]++;
    }
}

void FreePlayerCharacterModels(void)
{
    CharModelKill(-1);
}

typedef struct PlayerWork {
    s32 teamIndex;
    s32 teamPosition;
    s32 characterModelId;
    s32 hookNameIndex;
    s32 currentMotionId;
    u32 pressedButtons;
    u32 heldButtons;
    float stickMagnitude;
    float stickAngle;
    s32 movementMode;
    Vec targetPosition;
    float interactionRange;
    u16 playerState;
    s16 stateFrame;
    s16 stateSubstateFrame;
    s16 reachedTarget;
    float currentRotation;
    float targetRotation;
    float rotationStep;
    float rotationAcceleration;
    float hookClusterTime;
    s32 caughtFishCount;
    s16 caughtFishIndexes[3];
    s32 canChooseTarget;
    float aiDifficulty;
    s32 maxCaughtFish;
    u16 aiState;
    s32 targetGridCell;
    Vec targetWorldPosition;
    s16 targetWaitFrames;
    float collisionRadius;
    Vec position;
    Vec velocity;
} PlayerWork;

void UpdatePlayerObject(OMOBJ *object);
void SetPlayerMotion(OMOBJ *playerObject, u32 motionId);

void InitializePlayerObject(OMOBJ *object)
{
    PlayerWork *playerWork;
    s16 motionIndex;
    s32 characterId;
    s32 playerIndex;

    float aiSpeedScales[4] = { 0.5f, 0.65f, 0.8f, 1.0f };
    s32 aiFishLimits[4] = { 1, 1, 2, 2 };
    s32 hookNameIndex;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerWork), HU_MEMNUM_OVL);
    playerWork = object->data;
    memset(playerWork, 0, sizeof(PlayerWork));
    playerIndex = object->work[0];
    characterId = GWPlayerCfg[playerIndex].character;
    playerWork->characterModelId = characterId;
    hookNameIndex = playerWork->hookNameIndex = 0;
    playerWork->teamIndex = (object->work[1] >> 1) & 0x1;
    playerWork->teamPosition = object->work[1] & 0x1;
    playerWork->aiState = 0;
    playerWork->aiDifficulty = aiSpeedScales[GWPlayerCfg[playerIndex].diff & 0x3];
    playerWork->maxCaughtFish = aiFishLimits[GWPlayerCfg[playerIndex].diff & 0x3];
    object->mdlId[0] = CharModelCreate(characterId, 8);
    CharModelStepFxSet(characterId, 3);
    Hu3DModelCameraSet(object->mdlId[0], 4);
    Hu3DModelLayerSet(object->mdlId[0], 4);
    object->mdlId[1] = Hu3DModelCreateFile(playerBodyModelIds[playerWork->teamIndex]);
    Hu3DModelCameraSet(object->mdlId[1], 4);
    Hu3DModelLayerSet(object->mdlId[1], 4);
    Hu3DModelHookSet(object->mdlId[0], hookDummyNames[playerWork->hookNameIndex], object->mdlId[1]);
    Hu3DModelClusterAttrSet(object->mdlId[1], 0, 0xC0000001);
    for (motionIndex = 0; motionIndex < 10; motionIndex++) {
        if (playerMotionAssets[motionIndex] < 0x10000) {
            object->mtnId[motionIndex] = CharMotionCreate(characterId, playerMotionAssets[motionIndex]);
        }
        else {
            object->mtnId[motionIndex] = Hu3DJointMotionFile(object->mdlId[0], characterId + playerMotionAssets[motionIndex]);
        }
    }
    CharMotionDataClose(characterId);
    object->trans.x = playerWork->position.x = ((2.0f * object->work[1]) * 100.0f) - 300.0f;
    object->trans.y = playerWork->position.y = -70;
    object->trans.z = playerWork->position.z = 0;
    playerWork->velocity.x = playerWork->velocity.y = playerWork->velocity.z = 0;
    playerWork->currentRotation = playerWork->targetRotation = 0;
    playerWork->rotationStep = 0;
    playerWork->rotationAcceleration = 0.02f;
    playerWork->collisionRadius = 55;
    playerWork->targetPosition = playerSpawnPositions[playerWork->teamIndex];
    playerWork->targetPosition.y = object->trans.y;
    playerWork->interactionRange = 400;
    playerWork->caughtFishCount = 0;
    for (motionIndex = 0; motionIndex < 3; motionIndex++) {
        playerWork->caughtFishIndexes[motionIndex] = -1;
    }
    playerWork->hookClusterTime = 0;
    playerWork->currentMotionId = -1;
    playerWork->canChooseTarget = 0;
    SetPlayerMotion(object, 0);
    playerWork->playerState = 0;
    object->objFunc = UpdatePlayerObject;
}

void UpdatePlayerState(OMOBJ *object);
void UpdatePlayerMovement(OMOBJ *object);

void UpdatePlayerObject(OMOBJ *object)
{
    PlayerWork *playerWork = object->data;
    UpdatePlayerMovement(object);
    playerWork->stateSubstateFrame++;
    playerWork->playerState = 0;
    object->objFunc = UpdatePlayerState;
}

void UpdatePlayerExitState(OMOBJ *object);

void UpdatePlayerAi(OMOBJ *object);
void ResolvePlayerCollisions(OMOBJ *object);

float GetAngleDifference(float currentAngle, float targetAngle);

void UpdatePlayerState(OMOBJ *object)
{
    float stickX;
    float stickY;
    float rotationDifference;
    PlayerWork *playerWork = object->data;

    s32 fishIndex;
    s32 fishObjectIndex;
    s32 isPlayerActive = 0;
    Mtx modelMatrix;
    Vec fishWorldPosition;
    char *hookJointNames[3] = { "f0", "f1", "f2" };
    float stickDeadZones[2] = { 0.05, 0.02 };
    playerWork->pressedButtons = 0;
    playerWork->heldButtons = 0;
    playerWork->stickAngle = 0;
    playerWork->stickMagnitude = 0;
    if (getRoundState() == 2) {
        if (GWPlayerCfg[object->work[0]].iscom) {
            UpdatePlayerAi(object);
        }
        else {
            playerWork->heldButtons = HuPadBtn[GWPlayerCfg[object->work[0]].pad_idx];
            playerWork->pressedButtons = HuPadBtnDown[GWPlayerCfg[object->work[0]].pad_idx];
            stickX = HuPadStkX[GWPlayerCfg[object->work[0]].pad_idx];
            stickY = -HuPadStkY[GWPlayerCfg[object->work[0]].pad_idx];

            playerWork->stickAngle = atan2d(stickX, stickY);
            playerWork->stickMagnitude = (1.0f / 72.0f) * VECMagPoint(stickX, stickY, 0);
            if (playerWork->stickMagnitude > 1.0f) {
                playerWork->stickMagnitude = 1.0f;
            }
        }
    }
    playerWork->canChooseTarget = 0;
    switch (playerWork->playerState) {
        case 0:
            isPlayerActive = 1;
            if (playerWork->stickMagnitude > stickDeadZones[playerWork->movementMode]) {
                playerWork->movementMode = 1;
                playerWork->targetRotation = playerWork->stickAngle;
                playerWork->velocity.x += 225.0f * (playerWork->stickMagnitude * sind(playerWork->stickAngle)) * (1.0f / REFRESH_RATE);
                playerWork->velocity.z += 225.0f * (playerWork->stickMagnitude * cosd(playerWork->stickAngle)) * (1.0f / REFRESH_RATE);
                playerWork->rotationStep = 0.2f;
                playerWork->rotationAcceleration = 0;
                if (CharMotionShiftIDGet(playerWork->characterModelId) < 0) {
                    SetPlayerMotion(object, 1);
                    CharMotionSpeedSet(playerWork->characterModelId, playerWork->stickMagnitude);
                }
                if (playerWork->stickMagnitude > 0.75f && (playerWork->stateFrame & 0x7) == 0) {
                    CreateFishAttraction(object->work[0], &playerWork->position, playerWork->stickMagnitude);
                    if ((playerWork->stateFrame & 0xF) == 0) {
                        ClampPositionToWaterline(&playerWork->position);
                    }
                }
            }
            else {
                playerWork->movementMode = 0;
                if (CharMotionShiftIDGet(playerWork->characterModelId) < 0) {
                    SetPlayerMotion(object, 0);
                }
            }
            if (playerWork->pressedButtons & PAD_BUTTON_A) {
                playerWork->playerState = 1;
                playerWork->stateSubstateFrame = 0;
            }
            else if (playerWork->pressedButtons & PAD_BUTTON_B) {
                playerWork->playerState = 3;
                playerWork->stateSubstateFrame = 0;
                playerWork->stateFrame = 0;
            }
            else {
                playerWork->canChooseTarget = 1;
            }
            break;

        case 1:
            playerWork->movementMode = 0;
            if (playerWork->stateSubstateFrame == 0) {
                SetPlayerMotion(object, 2);
                playerWork->stateFrame = 0;
                playerWork->stateSubstateFrame++;
            }
            else if (playerWork->stateSubstateFrame == 1 && playerWork->stateFrame > (0.5f * REFRESH_RATE)) {
                if (playerWork->caughtFishCount < 3) {
                    fishObjectIndex = findFishInCone(&playerWork->position, 20, 150, playerWork->currentRotation, -30, 30, 1, object->work[0]);
                    if (fishObjectIndex >= 0) {
                        playerWork->caughtFishIndexes[playerWork->caughtFishCount++] = fishObjectIndex;
                        omVibrate(object->work[0], 12, 6, 6);
                    }
                }
                playerWork->playerState = 2;
                playerWork->stateFrame = 0;
                playerWork->stateSubstateFrame = 0;
            }
            break;

        case 2:
            if (playerWork->stateSubstateFrame == 0) {
                if (!(playerWork->heldButtons & PAD_BUTTON_A)) {
                    playerWork->movementMode = 0;
                    SetPlayerMotion(object, 5);
                    playerWork->stateFrame = 0;
                    playerWork->stateSubstateFrame++;
                }
                else {
                    rotationDifference = GetAngleDifference(playerWork->currentRotation, playerWork->stickAngle);
                    if (rotationDifference > 10.0f) {
                        rotationDifference = 10.0f;
                    }
                    else if (rotationDifference < -10.0f) {
                        rotationDifference = -10.0f;
                    }
                    if (playerWork->stickMagnitude > stickDeadZones[playerWork->movementMode] && fabs(rotationDifference) > 5.0) {
                        playerWork->movementMode = 1;
                        playerWork->targetRotation = playerWork->currentRotation + rotationDifference;
                        playerWork->rotationStep = 0.2f;
                        playerWork->rotationAcceleration = 0;
                        if (CharMotionShiftIDGet(playerWork->characterModelId) < 0) {
                            SetPlayerMotion(object, 4);
                            CharMotionSpeedSet(playerWork->characterModelId, 0.5f);
                        }
                        if (playerWork->caughtFishCount < 3) {
                            fishObjectIndex = findFishInCone(&playerWork->position, 20, 150, playerWork->currentRotation, -15, 15, 1, object->work[0]);
                            if (fishObjectIndex >= 0) {
                                playerWork->caughtFishIndexes[playerWork->caughtFishCount++] = fishObjectIndex;
                                omVibrate(object->work[0], 12, 6, 6);
                            }
                        }
                    }
                    else {
                        playerWork->movementMode = 0;
                        if (CharMotionShiftIDGet(playerWork->characterModelId) < 0) {
                            SetPlayerMotion(object, 3);
                        }
                    }
                }
            }
            else {
                if (playerWork->stateFrame > (0.5f * REFRESH_RATE)) {
                    playerWork->playerState = 0;
                }
            }
            break;

        case 3:
            playerWork->movementMode = 0;
            if (playerWork->stateSubstateFrame == 0) {
                playerWork->reachedTarget = 0;
                VECSubtract(&playerWork->targetPosition, &playerWork->position, &fishWorldPosition);
                playerWork->targetRotation = atan2d(fishWorldPosition.x, fishWorldPosition.z);
                rotationDifference = VECMag(&fishWorldPosition);
                if (rotationDifference < playerWork->interactionRange) {
                    playerWork->reachedTarget++;
                    SetPlayerMotion(object, 6);
                }
                else {
                    SetPlayerMotion(object, 0);
                }
                playerWork->stateFrame = 0;
                playerWork->stateSubstateFrame++;
            }
            if (playerWork->reachedTarget) {
                if (playerWork->stateFrame > (0.6f*REFRESH_RATE)) {
                    for (fishIndex = 0; fishIndex < playerWork->caughtFishCount; fishIndex++) {
                        RecordCaughtFish(playerWork->caughtFishIndexes[fishIndex]);
                    }
                    playerWork->caughtFishCount = 0;
                    playerWork->reachedTarget = 0;
                }
            }
            if (playerWork->stateFrame > 1.0f * REFRESH_RATE) {
                playerWork->playerState = 0;
            }
            break;
    }
    VECAdd(&playerWork->position, &playerWork->velocity, &playerWork->position);
    playerWork->velocity.x = playerWork->velocity.y = playerWork->velocity.z = 0;
    ResolvePlayerCollisions(object);
    rotationDifference = 0;
    if (playerWork->caughtFishCount) {
        rotationDifference = (playerWork->caughtFishCount * 10) + 10;
    }
    playerWork->hookClusterTime += (rotationDifference - playerWork->hookClusterTime) * 0.3f;
    Hu3DData[object->mdlId[1]].clusterTime[0] = playerWork->hookClusterTime;
    Hu3DModelObjMtxGet(object->mdlId[0], hookDummyNames[playerWork->hookNameIndex], modelMatrix);
    for (fishIndex = 0; fishIndex < playerWork->caughtFishCount; fishIndex++) {
        Hu3DModelObjPosGet(object->mdlId[1], hookJointNames[fishIndex], &fishWorldPosition);
        MTXMultVec(modelMatrix, &fishWorldPosition, &fishWorldPosition);
        SetFishPosition(playerWork->caughtFishIndexes[fishIndex], &fishWorldPosition);
    }
    UpdatePlayerMovement(object);
    if (getRoundState() == 5 && isPlayerActive) {
        playerWork->playerState = 0;
        object->objFunc = UpdatePlayerExitState;
    }
}

s32 CheckPlayerMotionFinished(OMOBJ *object);

void UpdatePlayerExitState(OMOBJ *object)
{
    PlayerWork *playerWork = object->data;
    s32 teamSideSigns[2] = { -1, 1 };
    UpdatePlayerMovement(object);
    switch (playerWork->playerState) {
        case 0:
            playerWork->playerState++;
            break;

        case 1:
            Hu3DModelHookReset(object->mdlId[0]);
            Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
            if (teamSideSigns[playerWork->teamIndex] == getRoundWinner()) {
                SetPlayerMotion(object, 7);
            }
            else {
                SetPlayerMotion(object, 8);
            }
            playerWork->targetRotation = 0;
            playerWork->rotationStep = 0;
            playerWork->rotationAcceleration = 0.005f;
            playerWork->playerState++;
            break;

        case 2:
            if (CheckPlayerMotionFinished(object)) {
                playerWork->playerState++;
            }
            break;
    }
}

float ApproachAngle(float currentAngle, float targetAngle, float stepFraction);

void UpdatePlayerMovement(OMOBJ *object)
{
    PlayerWork *playerWork = object->data;
    playerWork->stateFrame++;
    object->trans.x = playerWork->position.x;
    object->trans.y = playerWork->position.y;
    object->trans.z = playerWork->position.z;
    playerWork->rotationStep += playerWork->rotationAcceleration;
    if (playerWork->rotationStep > 0.45f) {
        playerWork->rotationStep = 0.45f;
    }
    playerWork->currentRotation = ApproachAngle(playerWork->currentRotation, playerWork->targetRotation, playerWork->rotationStep);
    object->rot.y = playerWork->currentRotation;
}

void UpdatePlayerAi(OMOBJ *object)
{
    Vec positionDifference;
    PlayerWork *playerWork = object->data;
    s32 reachedTarget;
    s32 shouldChangeAiState = 0;
    s32 currentGridCell;
    switch (playerWork->aiState) {
        case 0:
            if (playerWork->canChooseTarget) {
                playerWork->targetGridCell = findLeastCrowdedGridCell(&playerWork->position);
                if (playerWork->targetGridCell < 0 || playerWork->caughtFishCount >= playerWork->maxCaughtFish || getRoundTimer() < 10) {
                    shouldChangeAiState = 1;
                }
                if (playerWork->caughtFishCount && shouldChangeAiState) {
                    playerWork->aiState = 3;
                    playerWork->targetWaitFrames = 0;
                }
                else if (playerWork->targetGridCell >= 0) {
                    playerWork->aiState = 1;
                    playerWork->targetWaitFrames = 0;
                    GetPositionForGridCell(playerWork->targetGridCell, &playerWork->targetWorldPosition);
                    playerWork->targetWorldPosition.y = playerWork->position.y;
                }
            }
            break;

        case 1:
            playerWork->stickMagnitude = 0.73f * playerWork->aiDifficulty;
            if (playerWork->caughtFishCount && getRoundTimer() < 10) {
                playerWork->aiState = 0;
            }
            else {
                currentGridCell = getGridCellForPosition(&playerWork->position);
                if (getGridCellFishCount(currentGridCell) > 0) {
                    playerWork->aiState = 2;
                    playerWork->targetWaitFrames = 0;
                }
                else {
                    VECSubtract(&playerWork->targetWorldPosition, &playerWork->position, &positionDifference);
                    playerWork->stickAngle = ApproachAngle(playerWork->currentRotation, atan2d(positionDifference.x, positionDifference.z), playerWork->aiDifficulty * 0.25f);
                    reachedTarget = 0;
                    if (VECMag(&positionDifference) < 50.0f) {
                        reachedTarget = 1;
                    }
                    if (reachedTarget && getGridCellFishCount(playerWork->targetGridCell) > 0) {
                        playerWork->aiState = 2;
                        playerWork->targetWaitFrames = 0;
                    }
                    else {
                        playerWork->targetWaitFrames++;
                        if (reachedTarget || playerWork->targetWaitFrames > (1.0f * REFRESH_RATE)) {
                            if (reachedTarget || frandmod(1000) < 10) {
                                playerWork->targetGridCell = findLeastCrowdedGridCell(&playerWork->position);
                                GetPositionForGridCell(playerWork->targetGridCell, &playerWork->targetWorldPosition);
                                playerWork->targetWorldPosition.y = playerWork->position.y;
                                playerWork->targetWaitFrames = 0;
                                if (playerWork->targetGridCell < 0) {
                                    playerWork->aiState = 0;
                                }
                            }
                        }
                    }
                }
            }
            break;

        case 2:
            playerWork->targetWaitFrames++;
            if (playerWork->targetWaitFrames < (2 * REFRESH_RATE) * playerWork->aiDifficulty) {
                playerWork->stickMagnitude = 0.4f;
                playerWork->stickAngle = playerWork->currentRotation + 10.0f;
                playerWork->pressedButtons = playerWork->heldButtons = PAD_BUTTON_A;
            }
            else {
                playerWork->aiState = 0;
            }
            break;

        case 3:
            playerWork->stickMagnitude = 0.75f + (0.25f * playerWork->aiDifficulty);
            VECSubtract(&playerWork->targetPosition, &playerWork->position, &positionDifference);
            playerWork->stickAngle = ApproachAngle(playerWork->currentRotation, atan2d(positionDifference.x, positionDifference.z), 0.2f);
            if (VECMag(&positionDifference) < playerWork->interactionRange - 30.000002f) {
                playerWork->pressedButtons = playerWork->heldButtons = PAD_BUTTON_B;
                playerWork->aiState = 0;
            }
            break;
    }
}

void SetPlayerMotion(OMOBJ *playerObject, u32 motionId)
{
    float motionBlendTime;
    PlayerWork *playerWork = playerObject->data;
    if (playerWork->currentMotionId != motionId && motionId < 9) {
        motionBlendTime = playerMotionConfigs[motionId].blendTime;
        if (playerWork->currentMotionId < 0) {
            motionBlendTime = 0;
        }
        playerWork->currentMotionId = motionId;
        CharMotionShiftSet(playerWork->characterModelId, playerObject->mtnId[playerMotionConfigs[motionId].motionId], 0, motionBlendTime, playerMotionConfigs[motionId].motionAttributes);
        CharMotionSpeedSet(playerWork->characterModelId, 1.0f);
        if (playerMotionConfigs[motionId].overlayMotionId) {
            Hu3DMotionOverlaySet(playerObject->mdlId[0], playerObject->mtnId[playerMotionConfigs[motionId].overlayMotionId]);
            Hu3DMotionOverlayTimeSet(playerObject->mdlId[0], playerMotionConfigs[motionId].overlayTime);
            Hu3DData[playerObject->mdlId[0]].motOvlWork.speed = 1.0f;
        }
        else {
            Hu3DMotionOverlayReset(playerObject->mdlId[0]);
        }
    }
}

s32 CheckPlayerMotionFinished(OMOBJ *object)
{
    PlayerWork *playerWork = object->data;
    s32 motionFinished = 0;
    if (CharMotionEndCheck(playerWork->characterModelId) && CharMotionShiftIDGet(playerWork->characterModelId) < 0) {
        motionFinished = 1;
    }
    return motionFinished;
}

s32 SetPlayerMotionWhenFinished(OMOBJ *playerObject, u32 motionId)
{
    s32 result = 0;
    if (CheckPlayerMotionFinished(playerObject)) {
        SetPlayerMotion(playerObject, motionId);
        result = 1;
    }
    return result;
}

s32 CheckPlayerAction(OMOBJ *object)
{
    PlayerWork *unusedPlayerWork = object->data;
    s32 unreferencedReturnValue = 0;
    return unreferencedReturnValue;
}

void ResolvePlayerCollisions(OMOBJ *object)
{
    float playerSeparation;
    Vec positionDifference;
    PlayerWork *firstPlayerWork;
    PlayerWork *secondPlayerWork;
    s32 playerIndex;
    s32 otherPlayerIndex;
    s32 passIndex;
    for (passIndex = 0; passIndex < 2; passIndex++) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            firstPlayerWork = playerObjects[playerIndex]->data;
            for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                if (playerIndex == otherPlayerIndex) {
                    continue;
                }
                secondPlayerWork = playerObjects[otherPlayerIndex]->data;
                VECSubtract(&firstPlayerWork->position, &secondPlayerWork->position, &positionDifference);
                playerSeparation = VECMag(&positionDifference);
                if (playerSeparation < firstPlayerWork->collisionRadius + secondPlayerWork->collisionRadius) {
                    playerSeparation = 0.5f * ((firstPlayerWork->collisionRadius + secondPlayerWork->collisionRadius) - playerSeparation);
                    VECNormalize(&positionDifference, &positionDifference);
                    VECScale(&positionDifference, &positionDifference, playerSeparation);
                    VECAdd(&firstPlayerWork->position, &positionDifference, &firstPlayerWork->position);
                    VECSubtract(&secondPlayerWork->position, &positionDifference, &secondPlayerWork->position);
                }
            }
            positionDifference = firstPlayerWork->position;
            positionDifference.y = 0;
            playerSeparation = VECMag(&positionDifference);
            if (playerSeparation > 510.0f - firstPlayerWork->collisionRadius) {
                VECNormalize(&positionDifference, &positionDifference);
                VECScale(&positionDifference, &positionDifference, -(playerSeparation - (510.0f - firstPlayerWork->collisionRadius)));
                VECAdd(&positionDifference, &firstPlayerWork->position, &firstPlayerWork->position);
            }
        }
    }
}

float ApproachAngle(float currentAngle, float targetAngle, float stepFraction)
{
    float angleDifference;
    angleDifference = fmodf(targetAngle - currentAngle, 360.0);
    if (angleDifference < 0.0f) {
        angleDifference += 360.0f;
    }
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    angleDifference = fmodf((stepFraction * angleDifference) + currentAngle, 360.0);
    if (angleDifference < 0.0f) {
        angleDifference += 360.0f;
    }
    return angleDifference;
}

float GetAngleDifference(float currentAngle, float targetAngle)
{
    float angleDifference;
    angleDifference = fmodf(targetAngle - currentAngle, 360.0);
    if (angleDifference < 0.0f) {
        angleDifference += 360.0f;
    }
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    return angleDifference;
}
