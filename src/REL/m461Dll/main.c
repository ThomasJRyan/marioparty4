#include "REL/executor.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/gamework.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/wipe.h"

#include "game/gamework_data.h"

#include "game/mapspace.h"
#include "game/pad.h"
#include "game/printfunc.h"

#include "game/hu3d.h"

#include "game/esprite.h"
#include "game/sprite.h"

#include "ext_math.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/audio.h"
#endif

#undef ABS
#define ABS(x) ((0 > (x)) ? -(x) : (x))

typedef struct TimedValueNode {
    /* 0x00 */ u16 key;
    /* 0x02 */ s16 value;
    /* 0x04 */ struct TimedValueNode *previous;
    /* 0x08 */ struct TimedValueNode *next;
} TimedValueNode; // Size 0xC

typedef struct ParticleGroupNode {
    /* 0x00 */ HU3DPARMANPARAM *particleParam;
    /* 0x04 */ s32 *particleIds;
    /* 0x08 */ s32 framesRemaining;
    /* 0x0C */ s32 particleCount;
    /* 0x10 */ struct ParticleGroupNode *previous;
    /* 0x14 */ struct ParticleGroupNode *next;
} ParticleGroupNode; // Size 0x18

typedef struct {
    /* 0x00 */ u32 modelFile;
    /* 0x04 */ s16 modelId;
} ModelCacheEntry; // Size 8

TimedValueNode timedValueListSentinel;
ParticleGroupNode particleGroupListSentinel;
HUPROCESS *objectManager;
Vec cameraStartPosition;
Vec cameraEndPosition;
Vec cameraStartTarget;
Vec cameraEndTarget;
u32 spawnCycle;
s32 elapsedFrames;
u32 bestTime;
s16 messageSequenceId;
s16 finishSequenceId;
s32 musicSequenceId;
s32 cameraTransitionActive;
float timerPulseAngle;
s32 timerSprites[16];
ModelCacheEntry modelCache[32];

void TimedValueListInit(void)
{
    timedValueListSentinel.previous = NULL;
    timedValueListSentinel.next = NULL;
}

void TimedValueListInsert(u16 key, s16 value)
{
    TimedValueNode *newEntry;
    TimedValueNode *listHead;
    TimedValueNode *previousFirstEntry;

    newEntry = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*newEntry), HU_MEMNUM_OVL);
    newEntry->key = key;
    newEntry->value = value;
    listHead = &timedValueListSentinel;
    previousFirstEntry = listHead->next;
    listHead->next = newEntry;
    if (NULL != previousFirstEntry) {
        previousFirstEntry->previous = newEntry;
    }
    newEntry->previous = listHead;
    newEntry->next = previousFirstEntry;
}

s16 TimedValueListRemove(s16 key)
{
    TimedValueNode *entry;
    TimedValueNode *previousEntry;
    TimedValueNode *nextEntry;
    s16 value;

    entry = timedValueListSentinel.next;
    while (NULL != entry) {
        if (key == entry->key) {
            value = entry->value;
            previousEntry = entry->previous;
            nextEntry = entry->next;
            previousEntry->next = nextEntry;
            if (NULL != nextEntry) {
                nextEntry->previous = previousEntry;
            }
            HuMemDirectFree(entry);
            return value;
        }
        entry = entry->next;
    }
    return -1;
}

void ParticleGroupListInit(void)
{
    particleGroupListSentinel.previous = NULL;
    particleGroupListSentinel.next = NULL;
}

void ParticleGroupSpawn(s16 particleFile, HU3DPARMANPARAM *particleParam, s32 modelLayer, s32 durationFrames, s32 particleCount, Vec *positions, u8 blendMode)
{
    ParticleGroupNode *previousFirstGroup;
    ParticleGroupNode *newGroup;
    ParticleGroupNode *listHead;
    s32 i;

    newGroup = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*newGroup), HU_MEMNUM_OVL);
    newGroup->particleParam = particleParam;
    newGroup->framesRemaining = durationFrames;
    newGroup->particleCount = particleCount;
    newGroup->particleIds = HuMemDirectMallocNum(HEAP_HEAP, particleCount * sizeof(*newGroup->particleIds), HU_MEMNUM_OVL);
    for (i = 0; i < particleCount; i++, positions++) {
        newGroup->particleIds[i] = Hu3DParManLink(particleFile, particleParam);
        if (newGroup->particleIds[i] >= 0) {
            Hu3DParManAttrReset(newGroup->particleIds[i], 1);
            Hu3DParManPosSet(newGroup->particleIds[i], positions->x, positions->y, positions->z);
            Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(newGroup->particleIds[i]), blendMode);
            Hu3DModelLayerSet(Hu3DParManModelIDGet(newGroup->particleIds[i]), modelLayer);
        }
    }
    listHead = &particleGroupListSentinel;
    previousFirstGroup = listHead->next;
    listHead->next = newGroup;
    if (NULL != previousFirstGroup) {
        previousFirstGroup->previous = newGroup;
    }
    newGroup->previous = listHead;
    newGroup->next = previousFirstGroup;
}

void ParticleGroupUpdate(void)
{
    ParticleGroupNode *group;
    ParticleGroupNode *previousGroup;
    ParticleGroupNode *nextGroup;
    s32 i;

    group = particleGroupListSentinel.next;
    while (NULL != group) {
        group->framesRemaining--;
        if (group->framesRemaining == 0) {
            for (i = 0; i < group->particleCount; i++) {
                if (group->particleIds[i] >= 0) {
                    Hu3DParManAttrSet(group->particleIds[i], 1);
                }
            }
        }
        previousGroup = group->previous;
        nextGroup = group->next;
        if (-group->particleParam->maxTime > group->framesRemaining) {
            for (i = 0; i < group->particleCount; i++) {
                if (group->particleIds[i] >= 0) {
                    Hu3DParManKill(group->particleIds[i]);
                }
            }
            previousGroup->next = nextGroup;
            if (NULL != nextGroup) {
                nextGroup->previous = previousGroup;
            }
            HuMemDirectFree(group->particleIds);
            HuMemDirectFree(group);
        }
        group = nextGroup;
    }
}

void ModelCacheInit(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        modelCache[i].modelFile = 0;
        modelCache[i].modelId = -1;
    }
}

s16 ModelCacheGetOrCreate(s32 modelFile)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (modelFile == modelCache[i].modelFile) {
            return Hu3DModelLink(modelCache[i].modelId);
        }
    }
    for (i = 0; i < 32; i++) {
        if (modelCache[i].modelFile == 0) {
            modelCache[i].modelFile = modelFile;
            modelCache[i].modelId = Hu3DModelCreateFile(modelFile);
            return modelCache[i].modelId;
        }
    }
    return Hu3DModelCreateFile(modelFile);
}

void TimerDigitsUpdate(s32 *spriteIds, s32 firstDigitIndex, s32 frameCount)
{
    s32 timeUnit;

    espBankSet(spriteIds[firstDigitIndex + 1], 10);
    espBankSet(spriteIds[firstDigitIndex + 4], 11);
    if (600 * REFRESH_RATE <= frameCount) {
        frameCount = 600 * REFRESH_RATE - 1;
    }
    timeUnit = frameCount / (60 * REFRESH_RATE);
    frameCount -= timeUnit * (60 * REFRESH_RATE);
    if (timeUnit > 9) {
        timeUnit = 9;
    }
    espBankSet(spriteIds[firstDigitIndex], timeUnit);
    timeUnit = frameCount / REFRESH_RATE;
    frameCount -= timeUnit * REFRESH_RATE;
    espBankSet(spriteIds[firstDigitIndex + 2], timeUnit / 10);
    espBankSet(spriteIds[firstDigitIndex + 3], timeUnit % 10);
    timeUnit = 101.5f * frameCount / REFRESH_RATE;
    espBankSet(spriteIds[firstDigitIndex + 5], timeUnit / 10);
    espBankSet(spriteIds[firstDigitIndex + 6], timeUnit % 10);
}

void RecordTimeUpdate(s32 recordIndex)
{
    timerPulseAngle = 0.0f;
    bestTime = elapsedFrames;
    GWMGRecordSet(recordIndex, bestTime);
    TimerDigitsUpdate(timerSprites, 7, elapsedFrames);
}

void TimerDisplayShow(void)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        espDispOn(timerSprites[i]);
    }
}

void TimerDisplayPulse(void)
{
    float pulseScale;
    s32 i;

    if (0.0f <= timerPulseAngle) {
        timerPulseAngle += 16.0f;
        pulseScale = 1.0 + 0.2f * sind(timerPulseAngle);
        for (i = 7; i <= 14; i++) {
            espScaleSet(timerSprites[i], pulseScale, pulseScale);
        }
    }
}

void TimerDisplayCreate(float yPosition, s32 iconFile)
{
    float digitY;
    float digitX;
    s32 i;

    digitX = 288.0f;
    digitY = yPosition;
    timerSprites[15] = espEntry(iconFile, 0, 0);
    espTPLvlSet(timerSprites[15], 0.5f);
    espColorSet(timerSprites[15], 0, 0, 0);
    espPosSet(timerSprites[15], digitX, digitY);
    digitX = 248.0f;
    digitY = yPosition - 8.0f;
    timerSprites[14] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 55), 0, 0);
    espPosSet(timerSprites[14], digitX - 18.0f, digitY);
    for (i = 7; i <= 13; i++) {
        timerSprites[i] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 48), 0, 0);
        espPosSet(timerSprites[i], digitX, digitY);
        digitX += 16.0f;
    }
    TimerDigitsUpdate(timerSprites, 7, bestTime);
    digitX = 248.0f;
    digitY = yPosition + 8.0f;
    for (i = 0; i <= 6; i++) {
        timerSprites[i] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 49), 0, 0);
        espPosSet(timerSprites[i], digitX, digitY);
        digitX += 16.0f;
    }
    TimerDigitsUpdate(timerSprites, 0, 0);
    for (i = 0; i < 16; i++) {
        espDrawNoSet(timerSprites[i], 0);
        espDispOff(timerSprites[i]);
        espAttrSet(timerSprites[i], HUSPR_ATTR_NOANIM);
        espPriSet(timerSprites[i], 0x80 + i);
    }
}

void VectorEaseInterpolate(float progress, Vec *start, Vec *end, Vec *result)
{
    float interpolationWeight;

    if (1.0f < progress) {
        progress = 1.0f;
    }
    interpolationWeight = sind(90.0f * progress) * sind(90.0f * progress);
    result->x = start->x + interpolationWeight * (end->x - start->x);
    result->y = start->y + interpolationWeight * (end->y - start->y);
    result->z = start->z + interpolationWeight * (end->z - start->z);
}

void CameraPoseSet(Vec *position, Vec *up, Vec *target)
{
    Hu3DCameraPosSetV(1, position, up, target);
    cameraStartPosition = *position;
    cameraStartTarget = *target;
}

s32 CameraPoseTransitionUpdate(float progress, Vec *up)
{
    Vec cameraPosition;
    Vec cameraTarget;

    VectorEaseInterpolate(progress, &cameraStartPosition, &cameraEndPosition, &cameraPosition);
    VectorEaseInterpolate(progress, &cameraStartTarget, &cameraEndTarget, &cameraTarget);
    Hu3DCameraPosSetV(1, &cameraPosition, up, &cameraTarget);
    if (1.0f <= progress) {
        return 1;
    }
    else {
        cameraTransitionActive = 1;
        return 0;
    }
}

void CameraPoseTransitionStart(float progress, Vec *up)
{
    Vec cameraPosition;
    Vec cameraTarget;

    VectorEaseInterpolate(progress, &cameraStartPosition, &cameraEndPosition, &cameraPosition);
    VectorEaseInterpolate(progress, &cameraStartTarget, &cameraEndTarget, &cameraTarget);
    CameraPoseSet(&cameraPosition, up, &cameraTarget);
    cameraTransitionActive = 1;
}

void WorldToScreenPosition(Vec *worldPosition, s16 cameraMask, Vec *screenPosition)
{
    Mtx cameraMatrix;
    Vec cameraSpacePosition;
    float horizontalProjectionScale;
    float verticalProjectionScale;
    float viewportXOffset;
    float viewportWidth;
    HU3DCAMERA *camera;
    s32 i;

    for (i = 0; i < 16; i++) {
        if (cameraMask & (1 << i)) {
            break;
        }
    }
    camera = &Hu3DCamera[i];
    MTXLookAt(cameraMatrix, &camera->pos, &camera->up, &camera->target);
    MTXMultVec(cameraMatrix, worldPosition, &cameraSpacePosition);
    horizontalProjectionScale = cameraSpacePosition.z * (sind(camera->fov / 2) / cosd(camera->fov / 2)) * camera->aspect;
    verticalProjectionScale = cameraSpacePosition.z * (sind(camera->fov / 2) / cosd(camera->fov / 2));
    viewportXOffset = 0.9f * camera->viewportX;
    viewportWidth = 0.9f * camera->viewportW;
    screenPosition->x = viewportWidth / 2 + cameraSpacePosition.x * ((viewportWidth / 2) / -horizontalProjectionScale) + viewportXOffset;
    screenPosition->y = camera->viewportH / 2 + cameraSpacePosition.y * ((camera->viewportH / 2) / verticalProjectionScale) + camera->viewportY;
    screenPosition->z = 0.0f;
}

void ModelJointPositionGet(s16 modelId, char *jointName, Vec *position)
{
    Mtx modelMatrix;

    Hu3DModelObjMtxGet(modelId, jointName, modelMatrix);
    position->x = modelMatrix[0][3];
    position->y = modelMatrix[1][3];
    position->z = modelMatrix[2][3];
}

void ModelJointWorldPositionGet(s16 modelId, char *jointName, Vec *position, Mtx matrixOut)
{
    Mtx worldMatrix;
    HU3DMODEL *model;

    model = &Hu3DData[modelId];
    Hu3DModelObjMtxGet(modelId, jointName, worldMatrix);
    MTXConcat(model->mtx, worldMatrix, worldMatrix);
    if (NULL != matrixOut) {
        MTXCopy(worldMatrix, matrixOut);
    }
    position->x = worldMatrix[0][3];
    position->y = worldMatrix[1][3];
    position->z = worldMatrix[2][3];
}

float AngleInterpolate(float startAngle, float targetAngle, float interpolationRate)
{
    float angleDifference;
    float interpolatedAngle;

    angleDifference = fmod(targetAngle - startAngle, 360.0);
    if (0.0f > angleDifference) {
        angleDifference += 360.0f;
    }
    if (180.0f < angleDifference) {
        angleDifference -= 360.0f;
    }
    interpolatedAngle = fmod(startAngle + angleDifference * interpolationRate, 360.0);
    if (0.0f > interpolatedAngle) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

void AudioSequencePlay(s32 sequenceId)
{
    musicSequenceId = HuAudSeqPlay((s32)sequenceId);
}

void AudioSequenceFadeOut(s32 fadeFrames)
{
    if (musicSequenceId >= 0) {
        if (fadeFrames < 0) {
            fadeFrames = 1000;
        }
        HuAudSeqFadeOut(musicSequenceId, fadeFrames);
    }
    musicSequenceId = -1;
}

void PlayerStickMovementUpdate(Vec *position, float *rotationY, s32 stickX, s32 stickY)
{
    Vec stickDirection;
    float stickMagnitude;
    s32 movementState;

    stickDirection.x = 0.0f;
    stickDirection.y = 0.0f;
    stickDirection.z = 0.0f;
    stickDirection.x = stickX / 4.0f;
    stickDirection.z = -(float)stickY / 4.0f;
    movementState = 0;
    stickMagnitude = VECMagXZ(&stickDirection);
    if (0.5f <= stickMagnitude) {
        if (10.0f <= stickMagnitude) {
            stickDirection.x /= stickMagnitude;
            stickDirection.z /= stickMagnitude;
            stickMagnitude = 10.0f;
            stickDirection.x = 10.0f * stickDirection.x;
            stickDirection.z = 10.0f * stickDirection.z;
        }
        *rotationY = AngleInterpolate(*rotationY, atan2d(stickDirection.x, stickDirection.z), 0.4f);
        if (8.0f <= stickMagnitude) {
            movementState = 2;
        }
        else {
            movementState = 1;
        }
    }
    VECAdd((Vec *)&position, &stickDirection, (Vec *)&position);
}

typedef void (*PlayerOldHook)(OMOBJ *obj);
typedef Vec *(*PlayerOldPosHook)(OMOBJ *obj);

typedef struct WorkPlayerold {
    OMOBJ *object;
    PlayerOldHook updateCallback;
    PlayerOldPosHook positionCallback;
    PlayerOldHook landingCallback;
    PlayerOldHook collisionCallback;
    s32 *motionIds;
    s32 playerIndex;
    s32 characterIndex;
    s32 stickX;
    s32 stickY;
    s32 motionFlags;
    s32 motionStateFlags;
    s32 reserved30;
    s32 reserved34;
    s32 actionState;
    s32 movementFlags;
    s32 motionState;
    s32 motionId;
    s32 motionFrame;
    s32 collisionFrame;
    Vec *groundPositions;
    Vec groundOffset;
    float groundRotation;
    s32 collisionBoxCount;
    Vec *collisionBoxes;
    Vec position;
    Vec previousPosition;
    Vec velocity;
    Vec inputVelocity;
    float rotationY;
    float horizontalSpeed;
    float jumpTimer;
    float groundHeight;
    Vec groundNormal;
    float movementScale;
    float collisionRadius;
    float collisionHeight;
} WorkPlayerOld;

typedef WorkPlayerOld *(*PlayerOldFind)(int no);

PlayerOldFind playerOldLookup;
float playerOldGroundOffset;
OMOBJ *gameControllerObject;
OMOBJ *playerObject;
OMOBJ *bomheiObjects[20];
int bomheiCount;
int bomheiModelIds[3];
int bomheiMotionIds[4];
s32 spawnedBomheiMask;
s32 bomheiSpawnLimit;
s32 activeBomheiCount;
float stateTimer;
BOOL playerIndex;
BOOL playerCollision;
s32 leftAudioEmitter;
s32 rightAudioEmitter;
float cameraDistance;
float cameraVerticalOffset;
float cameraLookAtHeight;

s16 characterIds[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

s32 playerMotionStates[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

float playerCollisionRadii[10] = { 70, 70, 70, 70, 70, 70, 70, 70, 70, 100 };

float playerCollisionHeights[10] = { 150, 170, 190, 170, 160, 150, 180, 200, 150, 200 };

void PlayerOldMotionStateUpdate(WorkPlayerOld *player)
{
    s32 i;
    for (i = 0; i < 8; i++) {
        if (player->motionId == player->motionIds[i]) {
            player->motionState = playerMotionStates[i];
            return;
        }
    }
    player->motionState = -1;
}

void PlayerCollisionHeightTest(float *minimumDistance, s32 *collisionStateOut, s32 collisionState, Vec *firstCenter, float firstVerticalOffset, Vec *secondCenter, float secondVerticalOffset)
{
    Vec pointA = *firstCenter;
    Vec pointB = *secondCenter;
    float dist;
    pointA.y += firstVerticalOffset;
    pointB.y += secondVerticalOffset;
    dist = VECDistance(&pointA, &pointB);
    if (*minimumDistance > dist) {
        *minimumDistance = dist;
        *collisionStateOut = collisionState;
    }
}

void PlayerPairCollisionTest(WorkPlayerOld *firstPlayer, WorkPlayerOld *secondPlayer, float *minimumDistance, s32 *collisionStateOut)
{
    *minimumDistance = 10 * (firstPlayer->collisionRadius + secondPlayer->collisionRadius);
    PlayerCollisionHeightTest(minimumDistance, collisionStateOut, 0, &firstPlayer->position, firstPlayer->collisionRadius, &secondPlayer->position, secondPlayer->collisionRadius);
    PlayerCollisionHeightTest(minimumDistance, collisionStateOut, 1, &firstPlayer->position, firstPlayer->collisionHeight - firstPlayer->collisionRadius, &secondPlayer->position, secondPlayer->collisionHeight - secondPlayer->collisionRadius);
    PlayerCollisionHeightTest(minimumDistance, collisionStateOut, 2, &firstPlayer->position, firstPlayer->collisionRadius, &secondPlayer->position, secondPlayer->collisionHeight - secondPlayer->collisionRadius);
    PlayerCollisionHeightTest(minimumDistance, collisionStateOut, 3, &firstPlayer->position, firstPlayer->collisionHeight - firstPlayer->collisionRadius, &secondPlayer->position, secondPlayer->collisionRadius);
}

s32 PlayerPairResolve(WorkPlayerOld *player, WorkPlayerOld *otherPlayer)
{
    Vec dir;
    Vec dir2;
    float dist;

    player->movementFlags |= 0x1;
    player->inputVelocity.x = player->stickX / 4.0f;
    player->inputVelocity.z = -(float)player->stickY / 4.0f;
    if (player->inputVelocity.x != 0.0f || player->inputVelocity.z != 0.0f) {
        return;
    }
    VECSubtract(&player->position, &otherPlayer->position, &dir);
    dist = VECMagXZ(&dir);
    if (0.0f == dist) {
        dir.x = 0;
        dir.z = -1;
    }
    else {
        dir.x /= dist;
        dir.z /= dist;
    }
    VECSubtract(&player->position, &player->previousPosition, &dir2);
    dist = VECMagXZ(&dir2);
    if (5 > dist) {
        dist = 5;
    }
    player->inputVelocity.x = dir.x * dist;
    player->inputVelocity.z = dir.z * dist;
}

void PlayerOldJumpSet(WorkPlayerOld *player, float verticalVelocity, float jumpDuration)
{
    player->actionState = 1;
    player->velocity.y = verticalVelocity;
    player->jumpTimer = jumpDuration;
    if (player->movementFlags & 0x2) {
        player->movementFlags &= ~0x2;
    }
}

void PlayerOldManagerSet(PlayerOldFind playerLookup, float groundHeightOffset)
{
    playerOldLookup = playerLookup;
    playerOldGroundOffset = groundHeightOffset;
}

void PlayerOldCreate(WorkPlayerOld *player, OMOBJ *obj, s32 playerIndex, s32 characterIndex, s32 *motionIds, s32 initialMotionId, Vec *initialPosition, float initialRotationY)
{
    player->object = obj;
    player->playerIndex = playerIndex;
    player->characterIndex = characterIndex;
    player->motionIds = motionIds;
    player->updateCallback = NULL;
    player->positionCallback = NULL;
    player->landingCallback = NULL;
    player->collisionCallback = NULL;
    player->groundPositions = NULL;
    player->collisionBoxCount = 0;
    player->collisionBoxes = 0;
    player->actionState = 0;
    player->movementFlags = 0;
    player->motionId = initialMotionId;
    player->motionFrame = 0;
    player->collisionFrame = 0;
    PlayerOldMotionStateUpdate(player);
    player->rotationY = initialRotationY;
    player->position = *initialPosition;
    player->previousPosition = *initialPosition;
    player->velocity.x = 0;
    player->velocity.y = 0;
    player->velocity.z = 0;
    player->movementScale = 1;
    player->groundHeight = 0;
    player->collisionRadius = playerCollisionRadii[player->characterIndex];
    player->collisionHeight = playerCollisionHeights[player->characterIndex];
}

void PlayerOldUpdateCallbackSet(WorkPlayerOld *player, PlayerOldHook updateCallback)
{
    player->updateCallback = updateCallback;
}

void PlayerOldPositionCallbackSet(WorkPlayerOld *player, PlayerOldPosHook positionCallback)
{
    player->positionCallback = positionCallback;
}

void PlayerOldLandingCallbackSet(WorkPlayerOld *player, PlayerOldHook landingCallback)
{
    player->landingCallback = landingCallback;
}

void PlayerOldCollisionCallbackSet(WorkPlayerOld *player, PlayerOldHook collisionCallback)
{
    player->collisionCallback = collisionCallback;
}

void PlayerOldCollisionBoxesSet(WorkPlayerOld *player, s32 collisionBoxCount, Vec *collisionBoxes)
{
    player->collisionBoxCount = collisionBoxCount;
    player->collisionBoxes = collisionBoxes;
}

void PlayerOldMotionFlagsSet(WorkPlayerOld *player)
{
    omSetTra(player->object, player->position.x, player->position.y, player->position.z);
    omSetRot(player->object, 0.0f, player->rotationY, 0.0f);
}

void PlayerOldCollisionUpdate(WorkPlayerOld *player)
{
    BOOL addTime;
    s32 i;
    player->movementFlags &= ~0x100;
    if (player->movementFlags & 0x8) {
        return;
    }
    if (NULL == playerOldLookup) {
        return;
    }
    addTime = FALSE;
    for (i = 0; i < 4; i++) {
        WorkPlayerOld *player2 = playerOldLookup(i);
        BOOL fixPos;
        float minDist;
        s32 state;
        if (player == player2) {
            continue;
        }
        if (player2->movementFlags & 0x8) {
            continue;
        }
        fixPos = FALSE;
        PlayerPairCollisionTest(player, player2, &minDist, &state);
        if (minDist < player->collisionRadius + player2->collisionRadius) {
            if (state < 2) {
                fixPos = TRUE;
                if (player->actionState == 1) {
                    PlayerPairResolve(player, player2);
                }
            }
            else {
                if (state == 2 && player->actionState == 1) {
                    PlayerPairResolve(player, player2);
                    player->movementFlags |= 0x100;
                    player->jumpTimer = 0;
                    if (!(player2->movementFlags & 0x100)) {
                        player->velocity.y = (VERSION_PAL) ? 32.120003f : 26.766666f;
                    }
                }
                if (state == 3) {
                    fixPos = TRUE;
                    if (player->actionState == 1) {
                        if (0.0f < player->velocity.y) {
                            player->velocity.y = 0.0f;
                        }
                        player->jumpTimer = 0;
                    }
                }
            }
        }
        if (fixPos) {
            Vec dir;
            addTime = TRUE;
            VECSubtract(&player->position, &player2->position, &dir);
            if (0.0f == minDist) {
                dir.x = 0;
                dir.z = -1;
            }
            else {
                dir.x /= minDist;
                dir.z /= minDist;
            }
            player->position.x = player2->position.x + (dir.x * (player->collisionRadius + player2->collisionRadius));
            player->position.z = player2->position.z + (dir.z * (player->collisionRadius + player2->collisionRadius));
        }
    }
    if (addTime) {
        player->collisionFrame++;
    }
    else {
        player->collisionFrame = 0;
    }
}

float PlayerOldGroundHeightGet(WorkPlayerOld *player, float previousHeight)
{
    Vec *pos;
    s32 i;
    AddX = 0;
    AddZ = 0;
    if (NULL != player->collisionBoxes) {
        for (pos = player->collisionBoxes, i = 0; i < player->collisionBoxCount; i++, pos += 2) {
            int num = 0;
            if (pos[0].x == pos[1].x || pos[0].x > player->position.x || pos[1].x < player->position.x) {
                num++;
            }
            if (pos[0].y == pos[1].y || pos[0].y > player->position.y || pos[1].y < player->position.y) {
                num++;
            }
            if (pos[0].z == pos[1].z || pos[0].z > player->position.z || pos[1].z < player->position.z) {
                num++;
            }
            if (num == 3) {
                if (pos[0].x != pos[1].x) {
                    if (pos[0].x > player->position.x) {
                        player->position.x = pos[0].x + player->collisionRadius;
                    }
                    if (pos[1].x < player->position.x) {
                        player->position.x = pos[1].x - player->collisionRadius;
                    }
                }
                if (pos[0].y != pos[1].y) {
                    if (pos[0].y > player->position.y) {
                        player->position.y = pos[0].y;
                    }
                    if (pos[1].y < player->position.y) {
                        player->position.y = pos[1].y;
                    }
                }
                if (pos[0].z != pos[1].z) {
                    if (pos[0].z > player->position.z) {
                        player->position.z = pos[0].z + player->collisionRadius;
                    }
                    if (pos[1].z < player->position.z) {
                        player->position.z = pos[1].z - player->collisionRadius;
                    }
                }
            }
        }
    }
    return ABS(AddX) + ABS(AddZ);
}

void PlayerOldIdleUpdate(WorkPlayerOld *player, s32 *motionStateOut, u32 *motionAttributesOut)
{
    *motionAttributesOut = HU3D_MOTATTR_LOOP;
    *motionStateOut = 0;
    if (0.5f <= player->horizontalSpeed) {
        if (7.0f <= player->horizontalSpeed) {
            *motionStateOut = 2;
        }
        else {
            *motionStateOut = 1;
        }
    }

    if (player->motionIds[3] >= 0 && (player->motionFlags & 0x100)) {
        PlayerOldJumpSet(player, (VERSION_PAL) ? 32.120003f : 26.766666f, 1.0f);
        *motionAttributesOut = HU3D_MOTATTR_NONE;
        *motionStateOut = 3;
    }
}

void PlayerOldWalkUpdate(WorkPlayerOld *player, s32 *motionStateOut, u32 *motionAttributesOut)
{
    if (0.0f < player->jumpTimer) {
        if (player->motionStateFlags & 0x100) {
            player->jumpTimer++;
            if (4.0f < player->jumpTimer) {
                player->jumpTimer = 0;
            }
            else {
                player->velocity.y += 219.0f / REFRESH_RATE_F;
            }
        }
        else {
            player->jumpTimer = 0;
        }
    }
    player->velocity.y += -146.0f / REFRESH_RATE_F;
    if (player->motionIds[5] >= 0 && (player->motionFlags & 0x140)) {
        player->actionState = 3;
        player->velocity.y = 0.0f;
        player->movementFlags &= ~0x3;
        player->movementFlags |= 0x4;
        *motionAttributesOut = HU3D_MOTATTR_NONE;
        *motionStateOut = 5;
    }
}

void PlayerOldJumpUpdate(WorkPlayerOld *player, s32 *motionStateOut, u32 *motionAttributesOut)
{
    if ((player->movementFlags & 0x1000) || Hu3DMotionEndCheck(player->object->mdlId[0])) {
        player->movementFlags &= ~0x3;
        if (player->motionState == 6) {
            player->actionState = 4;
            *motionAttributesOut = HU3D_MOTATTR_NONE;
            *motionStateOut = 7;
        }
        else {
            player->actionState = 0;
            *motionAttributesOut = HU3D_MOTATTR_LOOP;
            *motionStateOut = 0;
        }
    }
}

void PlayerOldFallUpdate(WorkPlayerOld *player, s32 *motionStateOut, u32 *motionAttributesOut)
{
    if (Hu3DMotionShiftIDGet(player->object->mdlId[0]) < 0 && Hu3DMotionEndCheck(player->object->mdlId[0])) {
        PlayerOldJumpSet(player, 0.0f, 0.0f);
    }
}

void PlayerOldLandUpdate(WorkPlayerOld *player, s32 *motionStateOut, u32 *motionAttributesOut)
{
    if (Hu3DMotionShiftIDGet(player->object->mdlId[0]) < 0 && Hu3DMotionEndCheck(player->object->mdlId[0])) {
        player->movementFlags &= ~0x4;
        player->actionState = 0;
        *motionAttributesOut = HU3D_MOTATTR_LOOP;
        *motionStateOut = 0;
    }
}

void PlayerOldUpdate(WorkPlayerOld *player)
{
    u32 motAttr = HU3D_MOTATTR_NONE;
    s32 state = -1;
    float heightOffset;
    float distanceToGround;
    float previousHeight;
    Vec *pos;
    BOOL hasGroundContact;
    if (player->movementFlags & 0x1) {
        player->velocity.x = player->inputVelocity.x;
        player->velocity.z = player->inputVelocity.z;
    }
    else {
        player->velocity.x = player->stickX / 4.0f;
        player->velocity.z = -(float)player->stickY / 4.0f;
        if (player->actionState == 1 && 0.0f == player->velocity.x && 0.0f == player->velocity.z) {
            player->velocity.x = 0.98f * (player->position.x - player->previousPosition.x);
            player->velocity.z = 0.98f * (player->position.z - player->previousPosition.z);
        }
        if (player->movementFlags & 0x14) {
            player->velocity.x = 0;
            player->velocity.z = 0;
            player->motionFlags = 0;
        }
    }
    player->previousPosition = player->position;
    player->horizontalSpeed = VECMagXZ(&player->velocity);
    if (0.5f <= player->horizontalSpeed) {
        if (10.0f <= player->horizontalSpeed) {
            player->velocity.x /= player->horizontalSpeed;
            player->velocity.z /= player->horizontalSpeed;
            player->horizontalSpeed = 10;
            player->velocity.x = 10 * player->velocity.x;
            player->velocity.z = 10 * player->velocity.z;
        }
    }
    else {
        player->velocity.x = 0;
        player->velocity.z = 0;
        player->horizontalSpeed = 0;
    }
    player->velocity.x *= player->movementScale;
    player->velocity.z *= player->movementScale;
    player->horizontalSpeed *= player->movementScale;
    if (player->movementFlags & 0x800) {
        player->motionFlags |= 0x100;
        player->motionStateFlags |= 0x100;
        if (player->actionState == 1) {
            if (0.0f == player->jumpTimer) {
                player->movementFlags &= ~0x800;
            }
        }
    }
    switch (player->actionState) {
        case 0:
            PlayerOldIdleUpdate(player, &state, &motAttr);
            break;

        case 1:
            PlayerOldWalkUpdate(player, &state, &motAttr);
            break;

        case 2:
            PlayerOldJumpUpdate(player, &state, &motAttr);
            break;

        case 3:
            PlayerOldFallUpdate(player, &state, &motAttr);
            break;

        case 4:
            PlayerOldLandUpdate(player, &state, &motAttr);
            break;

        default:
            OSReport("*** player action error(%d)!!\n", player->actionState);
            break;
    }
    if (player->movementFlags & 0x2000) {
        player->velocity.y = 0;
    }
    heightOffset = 0;
    if (50.0f < ABS(player->velocity.y)) {
        player->velocity.y = 50.0f * ((0.0f > player->velocity.y) ? -1 : 1);
    }
    if (player->movementFlags & 0x2) {
        Mtx rotMtx;
        MTXRotDeg(rotMtx, 'y', player->groundPositions[2].x - player->groundRotation);
        MTXMultVec(rotMtx, &player->groundOffset, &player->groundOffset);
        VECAdd(&player->groundOffset, &player->groundPositions[0], &player->position);
        player->rotationY += player->groundPositions[2].x - player->groundRotation;
    }
    previousHeight = player->position.y;
    VECAdd(&player->position, &player->velocity, &player->position);
    if ((player->movementFlags & 0x1) == 0 && (0.0f != player->velocity.x || 0.0f != player->velocity.z)) {
        player->rotationY = AngleInterpolate(player->rotationY, atan2d(player->velocity.x, player->velocity.z), 0.4f * player->movementScale);
    }
    heightOffset += PlayerOldGroundHeightGet(player, previousHeight);
    if (nMap == 0) {
        player->groundHeight = playerOldGroundOffset;
    }
    else {
        player->groundHeight = MapPos(player->position.x, player->position.y, player->position.z, player->collisionHeight, &player->groundNormal);
    }
    hasGroundContact = FALSE;
    if (NULL != player->positionCallback) {
        pos = player->positionCallback(player->object);
        if (NULL != pos) {
            if (player->groundHeight < pos[1].y) {
                hasGroundContact = TRUE;
                player->groundPositions = pos;
                player->groundHeight = pos[1].y;
            }
        }
    }
    if (!hasGroundContact && (player->movementFlags & 0x2)) {
        player->movementFlags &= ~0x2;
    }
    if (player->actionState == 1 && player->groundHeight >= player->position.y) {
        player->actionState = 2;
        player->movementFlags &= ~0x3;
        player->position.y = player->groundHeight;
        player->velocity.y = 0;
        motAttr = HU3D_MOTATTR_NONE;
        state = 4;
        if (player->motionState == 5) {
            state = 6;
            if (NULL != player->collisionCallback) {
                player->collisionCallback(player->object);
            }
            if (player->playerIndex < 4) {
                omVibrate(player->playerIndex, 12, 4, 2);
            }
        }
        if (player->movementFlags & 0x200) {
            state = -1;
            if (NULL != player->landingCallback) {
                player->landingCallback(player->object);
            }
        }
        if (hasGroundContact) {
            player->movementFlags |= 0x2;
        }
    }
    distanceToGround = player->position.y - player->groundHeight;
    if (player->actionState == 0 && 10.0f < distanceToGround) {
        if (!(player->movementFlags & 0x2000) && (player->movementFlags & 0x400)) {
            player->movementFlags |= 0x800;
        }
    }
    else if (hasGroundContact) {
        if (!(player->movementFlags & 0x2)) {
            player->position.y = player->groundHeight;
            player->movementFlags |= 0x2;
        }
    }
    PlayerOldCollisionUpdate(player);
    if (NULL != player->updateCallback) {
        player->updateCallback(player->object);
    }
    heightOffset += PlayerOldGroundHeightGet(player, player->position.y);
    if (player->movementFlags & 0x2) {
        player->groundRotation = player->groundPositions[2].x;
        VECSubtract(&player->position, &player->groundPositions[0], &player->groundOffset);
    }
    if (0.0f != heightOffset) {
        player->motionFrame++;
    }
    else {
        player->motionFrame = 0;
    }
    if (state >= 0 && player->motionState != state) {
        player->motionState = state;
        player->motionId = player->motionIds[player->motionState];
        if (player->motionId >= 0 && !(player->movementFlags & 0x20)) {
            Hu3DMotionShiftSet(player->object->mdlId[0], player->object->mtnId[player->motionId], 0, 8, motAttr);
        }
    }
}

void PlayerOldMotionSet(WorkPlayerOld *player, s32 motionIndex)
{
    s32 i;
    player->motionId = motionIndex;
    player->actionState = 0;
    player->movementFlags &= 0x1400;
    player->previousPosition = player->position;
    player->velocity.x = 0;
    player->velocity.y = 0;
    player->velocity.z = 0;
    PlayerOldMotionStateUpdate(player);
    if (player->motionState == 3) {
        Hu3DModelAttrReset(player->object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
        Hu3DMotionSet(player->object->mdlId[0], player->object->mtnId[player->motionId]);
        Hu3DMotionTimeSet(player->object->mdlId[0], Hu3DMotionMaxTimeGet(player->object->mdlId[0]) - 1);
    }
    else if (player->motionState == 5) {
        player->actionState = 3;
        player->movementFlags |= 0x4;
        Hu3DModelAttrReset(player->object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
        Hu3DMotionSet(player->object->mdlId[0], player->object->mtnId[player->motionId]);
        Hu3DMotionTimeSet(player->object->mdlId[0], Hu3DMotionMaxTimeGet(player->object->mdlId[0]) - 1);
    }
    else {
        Hu3DModelAttrReset(player->object->mdlId[0], HU3D_MOTATTR_PAUSE);
        Hu3DMotionShiftSet(player->object->mdlId[0], player->object->mtnId[player->motionId], 0, 8, HU3D_MOTATTR_LOOP);
    }
}

void PlayerOldAttributeSet(WorkPlayerOld *player, s32 attributes)
{
    player->movementFlags |= attributes;
}

void PlayerOldAttributeReset(WorkPlayerOld *player, s32 attributes)
{
    player->movementFlags &= ~attributes;
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

Vec initialCameraPosition = { 0, 600, 600 };
Vec initialCameraTarget = { 0, 1, 0 };
Vec initialCameraUp = { 0, 100, 0 };

Vec finalCameraPosition = { 0, 2300, 2200 };
Vec cameraTransitionTarget = { 0, 1, 0 };
Vec finalCameraTarget = { 0, 0, 0 };

Vec lightPosition = { -1500, 4000, 1500 };
Vec lightDirection = { 1.5, -4, -1.5 };
GXColor lightColor = { 255, 255, 255, 255 };

Vec shadowPosition = { -1500, 4000, 1500 };
Vec shadowTarget = { 0, 0, 0 };
Vec shadowUp = { 0, 1, 0 };

s32 characterMotionFiles[8][5] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x14),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x14),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x14),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x14),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x14),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x14),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x14),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x14),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x17),
};

s32 bomheiModelFiles[3] = { DATA_MAKE_NUM(DATADIR_M461, 0x02), DATA_MAKE_NUM(DATADIR_M461, 0x03), DATA_MAKE_NUM(DATADIR_M461, 0x08) };

s32 bomheiMotionFiles[4]
    = { DATA_MAKE_NUM(DATADIR_M461, 0x04), DATA_MAKE_NUM(DATADIR_M461, 0x05), DATA_MAKE_NUM(DATADIR_M461, 0x06), DATA_MAKE_NUM(DATADIR_M461, 0x07) };

float bomheiTypeScales[3] = { 0.3f, 0.5f, 0.9f };

float spawnDifficultySchedule[10][2] = {
    0, 60,
    31 * REFRESH_RATE_F, 60,
    61 * REFRESH_RATE_F, 50,
    91 * REFRESH_RATE_F, 50,
    121 * REFRESH_RATE_F, 40,
    151 * REFRESH_RATE_F, 40,
    181 * REFRESH_RATE_F, 30,
    211 * REFRESH_RATE_F, 20,
    241 * REFRESH_RATE_F, 10,
    -1, 10
};

Vec bomheiSpawnPoints[3] = { { 0, 0, 2800 }, { 2800, 0, 0 }, { -2800, 0, 0 } };

s32 bomheiSpawnChances[3] = { 70, 30, 0 };

typedef struct BomheiSpawnPoint {
    Vec position;
    float rotationY;
} BomheiSpawnPoint;

BomheiSpawnPoint bomheiArenaSpawnPoints[8] = { { { 200, 0, 1400 }, 180 }, { { -200, 0, 1400 }, 180 }, { { 200, 0, -1400 }, 0 }, { { -200, 0, -1400 }, 0 },
    { { 1400, 0, 200 }, 270 }, { { 1400, 0, -200 }, 270 }, { { -1400, 0, 200 }, 90 }, { { -1400, 0, -200 }, 90 } };

Vec bomhei2SpawnPoints[4] = { { 600, 0, 600 }, { 600, 0, -600 }, { -600, 0, 600 }, { -600, 0, -600 } };

void DebugControlsUpdate(void)
{
    if (HuPadBtn[0] & PAD_BUTTON_X) {
        elapsedFrames = REFRESH_RATE * 600 - 1;
    }
    print8(24, 32, 1.5f, "COUNT:%d MODE:%d MES:%d %d/%d %02x", spawnCycle, gameControllerObject->work[0], messageSequenceId, activeBomheiCount, bomheiSpawnLimit,
        spawnedBomheiMask);
    print8(24, 48, 1.5f, "%f %f %f", cameraLookAtHeight, cameraDistance, cameraVerticalOffset);
    print8(24, 64, 1.5f, "%x %x", HuMemHeapSizeGet(HEAP_HEAP) - HuMemUsedMallocSizeGet(HEAP_HEAP),
        HuMemHeapSizeGet(HEAP_MODEL) - HuMemUsedMallocSizeGet(HEAP_MODEL));
    cameraVerticalOffset -= HuPadSubStkX[0] / 59.0f;
    cameraDistance -= 4.0f * (HuPadSubStkY[0] / 59.0f);
    cameraLookAtHeight -= (HuPadTrigL[0] / 150.0f) * 4.0f;
    cameraLookAtHeight += (HuPadTrigR[0] / 150.0f) * 4.0f;
}

typedef struct WorkPlayer {
    Vec position;
    Vec velocity;
    Vec turnState;
    Vec startPosition;
    Vec targetPosition;
    Vec destination;
    float movementSpeed;
    s32 stickX;
    s32 stickY;
    s32 motion;
    s32 aiState;
    s32 characterId;
    s32 controllerIndex;
} WorkPlayer;

typedef struct WorkBomhei {
    Vec position;
    Vec rotation;
    Vec timers;
    float lifetime;
    s32 playerMaskBit;
    s32 motionId;
    s32 type;
    s32 state;
} WorkBomhei;

void PlayerDistanceVibrationUpdate(Vec *playerPosition)
{
    float dist = VECDistance(playerPosition, &((WorkPlayer *)playerObject->data)->position);
    if (2000 < dist) {
        return;
    }
    if (1000 < dist) {
        omVibrate(playerIndex, 12, 6, 6);
    }
    else if (300 < dist) {
        omVibrate(playerIndex, 12, 4, 2);
    }
    else {
        omVibrate(playerIndex, 12, 12, 0);
    }
}

BOOL ArenaPositionClamp(Vec *position, float boundaryMargin)
{
    BOOL result = FALSE;
    float ofs = 700 - boundaryMargin;
    Mtx rotMtx;

    if (ofs < position->x) {
        position->x = ofs;
    }
    if (ofs < position->z) {
        position->z = ofs;
    }
    if (-ofs > position->x) {
        position->x = -ofs;
    }
    if (-ofs > position->z) {
        position->z = -ofs;
    }
    if (ofs == ABS(position->x) || ofs == ABS(position->z)) {
        result = TRUE;
    }
    MTXRotDeg(rotMtx, 'Y', 45);
    MTXMultVec(rotMtx, position, position);
    ofs = (550 * sqrtf(2)) - boundaryMargin;
    if (ofs < position->x) {
        position->x = ofs;
    }
    if (ofs < position->z) {
        position->z = ofs;
    }
    if (-ofs > position->x) {
        position->x = -ofs;
    }
    if (-ofs > position->z) {
        position->z = -ofs;
    }
    if (ofs == ABS(position->x) || ofs == ABS(position->z)) {
        result = TRUE;
    }
    MTXRotDeg(rotMtx, 'Y', -45);
    MTXMultVec(rotMtx, position, position);
    return result;
}

void CircleCollisionResolve(Vec *firstPosition, Vec *secondPosition, float firstRadius, float secondRadius)
{
    float dx = firstPosition->x - secondPosition->x;
    float dz = firstPosition->z - secondPosition->z;
    float dist = sqrtf((dx * dx) + (dz * dz));
    if (firstRadius + secondRadius > dist) {
        if (0.0f == dist) {
            dx = 0;
            dz = -1;
        }
        else {
            dx /= dist;
            dz /= dist;
        }
        firstPosition->x = secondPosition->x + (dx * (firstRadius + secondRadius));
        firstPosition->z = secondPosition->z + (dz * (firstRadius + secondRadius));
    }
}

void PlayerIntroUpdate(WorkPlayer *player, OMOBJ *obj)
{
    float distMin;
    s32 i;
    player->aiState = 2002;
    Hu3DMotionShiftSet(obj->mdlId[0], obj->mtnId[3], 0, 8, HU3D_MOTATTR_NONE);
    distMin = 100000;
    for (i = 0; i < 3; i++) {
        float dist = VECDistance(&bomheiSpawnPoints[i], &player->position);
        if (distMin > dist) {
            distMin = dist;
            player->destination = bomheiSpawnPoints[i];
        }
    }
    player->startPosition = player->position;
    player->targetPosition = player->position;
    player->movementSpeed = 15;
    leftAudioEmitter = HuAudFXEmiterPlay(2084, &player->position);
    rightAudioEmitter = CharFXPlayPos(player->characterId, 291, &player->position);
}

void PlayerMovementUpdate(WorkPlayer *player, OMOBJ *obj)
{
    s32 i;
    s32 action;
    player->velocity.x = 0;
    player->velocity.y = 0;
    player->velocity.z = 0;
    player->velocity.x = player->stickX / 4.0f;
    player->velocity.z = -(float)player->stickY / 4.0f;
    action = 0;
    player->turnState.z = VECMagXZ(&player->velocity);
    if (0.5f <= player->turnState.z) {
        if (10.0f <= player->turnState.z) {
            player->velocity.x /= player->turnState.z;
            player->velocity.z /= player->turnState.z;
            player->turnState.z = 10;
            player->velocity.x = 10 * player->velocity.x;
            player->velocity.z = 10 * player->velocity.z;
        }
        player->turnState.y = AngleInterpolate(player->turnState.y, atan2d(player->velocity.x, player->velocity.z), 0.4f);
        if (7.0f <= player->turnState.z) {
            action = 2;
        }
        else {
            action = 1;
        }
    }
    VECAdd(&player->position, &player->velocity, &player->position);
    ArenaPositionClamp(&player->position, 70);
    for (i = 0; i < 20; i++) {
        WorkBomhei *bomhei;
        if (NULL == bomheiObjects[i]) {
            continue;
        }
        bomhei = bomheiObjects[i]->data;
        if (bomhei->state == 3004 && 30 > Hu3DMotionTimeGet(bomheiObjects[i]->mdlId[2])) {
            float dx = player->position.x - bomhei->position.x;
            float dz = player->position.z - bomhei->position.z;
            if (200 > sqrtf((dx * dx) + (dz * dz))) {
                playerCollision = 1;
            }
        }
        else {
            CircleCollisionResolve(&player->position, &bomhei->position, 70, 70);
        }
    }
    ArenaPositionClamp(&player->position, 70);
    if (player->motion != action) {
        player->motion = action;
        Hu3DMotionShiftSet(obj->mdlId[0], obj->mtnId[player->motion], 0, 8, HU3D_MOTATTR_LOOP);
    }
}

void PlayerObjectUpdate(OMOBJ *obj)
{
    WorkPlayer *player = obj->data;
    float dx;
    float dz;
    float dist;
    switch (player->aiState) {
        case 2000:
            if (gameControllerObject->work[0] >= 1004) {
                player->aiState++;
            }
            break;

        case 2001:
            player->stickX = HuPadStkX[player->controllerIndex];
            player->stickY = HuPadStkY[player->controllerIndex];
            PlayerMovementUpdate(player, obj);
            if (gameControllerObject->work[0] >= 1005) {
                if (playerCollision) {
                    PlayerIntroUpdate(player, obj);
                }
                else {
                    player->aiState = 2003;
                    Hu3DMotionShiftSet(obj->mdlId[0], obj->mtnId[0], 0, 8, HU3D_MOTATTR_LOOP);
                }
            }
            break;

        case 2002:
            dx = player->destination.x - player->position.x;
            dz = player->destination.z - player->position.z;
            dist = sqrtf((dx * dx) + (dz * dz));
            if (player->movementSpeed < dist) {
                dx = player->movementSpeed * (dx / dist);
                dz = player->movementSpeed * (dz / dist);
            }
            player->turnState.x -= player->movementSpeed;
            if (0.0f > player->turnState.x) {
                player->turnState.x += 360;
            }
            player->turnState.y = AngleInterpolate(player->turnState.y, atan2d(dx, dz) + 180.0, 0.4f);
            player->position.x += dx;
            player->position.z += dz;
            dist = VECDistance(&player->destination, &player->position) / VECDistance(&player->destination, &player->targetPosition);
            player->position.y = 500 * sind(dist * 180);
            player->movementSpeed *= 1.05f;
            if (leftAudioEmitter >= 0) {
                HuAudFXEmiterUpDate(leftAudioEmitter, &player->position);
            }
            if (rightAudioEmitter >= 0) {
                HuAudFXEmiterUpDate(rightAudioEmitter, &player->position);
            }
            PlayerDistanceVibrationUpdate(&player->startPosition);
            if (gameControllerObject->work[0] >= 1007) {
                player->aiState = 2005;
                HuAudSStreamPlay(4);
            }
            break;

        case 2003:
            if (gameControllerObject->work[0] >= 1007) {
                player->aiState = 2004;
                HuAudSStreamPlay(1);
                Hu3DMotionShiftSet(obj->mdlId[0], obj->mtnId[4], 0, 8, HU3D_MOTATTR_NONE);
            }
            break;

        case 2004:
            player->turnState.y = AngleInterpolate(player->turnState.y, 0, 0.4f);
            break;

        case 2005:
            break;

        default:
            OSReport("*** player mode error(%d)!!\n", player->aiState);
            break;
    }
    omSetTra(obj, player->position.x, player->position.y, player->position.z);
    omSetRot(obj, player->turnState.x, player->turnState.y, 0);
}

void PlayerObjectCreate(OMOBJ *obj)
{
    WorkPlayer *player;
    s32 i;
    obj->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(WorkPlayer), HU_MEMNUM_OVL);
    player = obj->data;
    player->characterId = GWPlayerCfg[playerIndex].character;
    player->controllerIndex = GWPlayerCfg[playerIndex].pad_idx;
    player->aiState = 2000;
    player->motion = 0;
    obj->mdlId[0] = CharModelCreate(characterIds[player->characterId], 2);
    CharModelStepFxSet(characterIds[player->characterId], 0);
    for (i = 0; i < 5; i++) {
        obj->mtnId[i] = CharMotionCreate(characterIds[player->characterId], characterMotionFiles[player->characterId][i]);
        CharMotionSet(characterIds[player->characterId], obj->mtnId[i]);
    }
    Hu3DModelAttrSet(obj->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(obj->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(obj->mdlId[0], 2);
    Hu3DMotionSet(obj->mdlId[0], obj->mtnId[0]);
    Hu3DModelShadowSet(obj->mdlId[0]);
    player->turnState.x = 0;
    player->turnState.y = 0;
    player->position.x = 0;
    player->position.y = 0;
    player->position.z = 0;
    omSetTra(obj, player->position.x, player->position.y, player->position.z);
    omSetRot(obj, player->turnState.x, player->turnState.y, 0);
    Hu3DModelAttrReset(obj->mdlId[0], HU3D_ATTR_DISPOFF);
    CharModelLayerSetAll2(6);
    CharMotionDataClose(characterIds[player->characterId]);
    obj->objFunc = PlayerObjectUpdate;
}

void BomheiMotionSelect(WorkBomhei *bomhei, OMOBJ *obj)
{
    float *data1F0;
    bomhei->state = 3003;
    bomhei->lifetime = 10;
    for (data1F0 = &spawnDifficultySchedule[0][0]; 0.0f <= data1F0[0]; data1F0 += 2) {
        if (data1F0[0] <= elapsedFrames) {
            bomhei->lifetime = data1F0[1];
        }
    }
    Hu3DModelAttrSet(obj->mdlId[1], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(obj->mdlId[1], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(obj->mdlId[1], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(obj->mdlId[0], HU3D_ATTR_DISPOFF);
    HuAudFXEmiterPlay(2087, &bomhei->position);
}

BOOL BomheiArenaCollisionCheck(WorkBomhei *bomhei)
{
    if (bomhei->state == 3002) {
        if (ArenaPositionClamp(&bomhei->position, 70)) {
            return TRUE;
        }
    }
    else {
        float ofs = 700 - 70;

        if (ofs > bomhei->position.x && -ofs < bomhei->position.x) {
            if (ofs > bomhei->position.z && -ofs < bomhei->position.z) {
                if (bomhei->playerMaskBit >= 0) {
                    spawnedBomheiMask = spawnedBomheiMask & ~(u32)(1 << bomhei->playerMaskBit);
                    bomhei->playerMaskBit = -1;
                }
            }
        }
    }
    return FALSE;
}

void BomheiUpdate(WorkBomhei *bomhei, OMOBJ *obj)
{
    float radius = 10 * bomheiTypeScales[bomhei->type];
    Vec playerOfs;
    Vec angle;

    s32 i;
    s32 state;
    BOOL hitTarget;

    angle.x = bomhei->rotation.y;
    if (bomhei->state == 3002) {
        switch (bomhei->type) {
            case 0:
                bomhei->timers.x--;
                if (0.0f > bomhei->timers.x) {
                    radius = 0;
                    if (-bomhei->timers.y > bomhei->timers.x) {
                        bomhei->timers.y = frandmod(REFRESH_RATE * 2) + REFRESH_RATE * 2;
                        bomhei->timers.x = frandmod(REFRESH_RATE * 2) + REFRESH_RATE * 2;
                    }
                }
                else {
                    bomhei->timers.z--;
                    if (0.0f > bomhei->timers.z) {
                        bomhei->timers.z = frandmod(REFRESH_RATE * 2) + REFRESH_RATE * 2;
                        bomhei->rotation.z = frandf() * 360;
                    }
                }
                angle.x = bomhei->rotation.z;
                break;

            case 1:
            case 2:
                VECSubtract(&((WorkPlayer *)playerObject->data)->position, &bomhei->position, &playerOfs);
                angle.x = atan2d(playerOfs.x, playerOfs.z);
                break;
        }
    }
    bomhei->rotation.y = AngleInterpolate(bomhei->rotation.y, angle.x, 0.05f);
    angle.z = radius * sind(bomhei->rotation.y);
    angle.y = radius * cosd(bomhei->rotation.y);
    bomhei->position.x += angle.z;
    bomhei->position.z += angle.y;
    if (0.0f < radius) {
        bomhei->rotation.x = AngleInterpolate(bomhei->rotation.x, atan2d(angle.z, angle.y), 0.2f);
    }
    state = 0;
    if (0.5f <= radius) {
        if (7.0f <= radius) {
            state = 2;
        }
        else {
            state = 1;
        }
    }
    hitTarget = FALSE;
    if (BomheiArenaCollisionCheck(bomhei)) {
        hitTarget = TRUE;
    }
    CircleCollisionResolve(&bomhei->position, &((WorkPlayer *)playerObject->data)->position, 70, 70);
    for (i = 0; i < 20; i++) {
        WorkBomhei *bomhei2;
        if (NULL == bomheiObjects[i]) {
            continue;
        }
        if (obj == bomheiObjects[i]) {
            continue;
        }
        bomhei2 = bomheiObjects[i]->data;
        if (bomhei2->state != 3004) {
            CircleCollisionResolve(&bomhei->position, &bomhei2->position, 70, 70);
        }
    }
    if (BomheiArenaCollisionCheck(bomhei)) {
        hitTarget = TRUE;
    }
    if (hitTarget && bomhei->type == 0) {
        float radius = frandf() * 350;
        float angle2 = frandf() * 360;
        bomhei->rotation.z = atan2d((radius * sind(angle2)) - bomhei->position.x, (radius * cosd(angle2)) - bomhei->position.z);
        bomhei->timers.z = frandmod(REFRESH_RATE * 2) + REFRESH_RATE * 2;
    }
    if (bomhei->motionId != state) {
        bomhei->motionId = state;
        Hu3DMotionShiftSet(obj->mdlId[0], obj->mtnId[bomhei->motionId], 0, 8, HU3D_MOTATTR_LOOP);
    }
}

void BomheiObjectDestroy(OMOBJ *obj);

void BomheiObjectUpdate(OMOBJ *obj)
{
    WorkBomhei *bomhei = obj->data;
    s32 i;
    switch (bomhei->state) {
        case 3000:
            if (gameControllerObject->work[0] >= 1004) {
                bomhei->state = 3002;
            }
            break;

        case 3001:
            BomheiUpdate(bomhei, obj);
            if (bomhei->playerMaskBit < 0) {
                bomhei->state = 3002;
            }
            break;

        case 3002:
            BomheiUpdate(bomhei, obj);
            bomhei->lifetime--;
            if (0.0f > bomhei->lifetime) {
                BomheiMotionSelect(bomhei, obj);
            }
            break;

        case 3003:
            bomhei->lifetime--;
            if (0.0f > bomhei->lifetime) {
                bomhei->state++;
                Hu3DModelAttrSet(obj->mdlId[1], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(obj->mdlId[2], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(obj->mdlId[2], HU3D_ATTR_DISPOFF);
                HuAudFXEmiterPlay(2082, &bomhei->position);
                PlayerDistanceVibrationUpdate(&bomhei->position);
            }
            break;

        case 3004:
            if (Hu3DMotionEndCheck(obj->mdlId[2])) {
                BomheiObjectDestroy(obj);
                return;
            }
            break;

        case 3005:
            break;

        default:
            OSReport("*** bomhei mode error(%d)!!\n", bomhei->state);
            break;
    }
    if (bomhei->state != 3004 && gameControllerObject->work[0] >= 1005) {
        bomhei->state = 3005;
        Hu3DMotionShiftSet(obj->mdlId[0], obj->mtnId[0], 0, 8, HU3D_MOTATTR_LOOP);
    }
    omSetTra(obj, bomhei->position.x, bomhei->position.y, bomhei->position.z);
    omSetRot(obj, 0, bomhei->rotation.x, 0);
    for (i = 0; i < 3; i++) {
        Hu3DModelPosSet(obj->mdlId[i], bomhei->position.x, bomhei->position.y, bomhei->position.z);
        Hu3DModelRotSet(obj->mdlId[i], 0, bomhei->rotation.x, 0);
    }
}

void BomheiObjectCreate(OMOBJ *obj)
{
    WorkBomhei *bomhei = obj->data;
    s32 modelId;
    s32 i;
    omSetTra(obj, bomhei->position.x, bomhei->position.y, bomhei->position.z);
    omSetRot(obj, 0, bomhei->rotation.x, 0);
    modelId = TimedValueListRemove(0x1000);
    if (modelId < 0) {
        obj->mdlId[0] = Hu3DModelLink(bomheiModelIds[0]);
    }
    else {
        obj->mdlId[0] = modelId;
    }
    modelId = TimedValueListRemove(0x2000);
    if (modelId < 0) {
        obj->mdlId[1] = Hu3DModelLink(bomheiModelIds[1]);
    }
    else {
        obj->mdlId[1] = modelId;
    }
    modelId = TimedValueListRemove(0x3000);
    if (modelId < 0) {
        obj->mdlId[2] = Hu3DModelLink(bomheiModelIds[2]);
    }
    else {
        obj->mdlId[2] = modelId;
    }
    for (i = 0; i < 4; i++) {
        obj->mtnId[i] = bomheiMotionIds[i];
    }
    for (i = 0; i < 3; i++) {
        Hu3DModelAttrSet(obj->mdlId[i], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(obj->mdlId[i], HU3D_MOTATTR_LOOP);
        Hu3DModelAttrReset(obj->mdlId[i], HU3D_MOTATTR_PAUSE);
        Hu3DModelLayerSet(obj->mdlId[i], 3);
        Hu3DMotionTimeSet(obj->mdlId[i], 0);
        Hu3DModelPosSet(obj->mdlId[i], bomhei->position.x, bomhei->position.y, bomhei->position.z);
        Hu3DModelRotSet(obj->mdlId[i], 0, bomhei->rotation.x, 0);
    }
    Hu3DModelLayerSet(obj->mdlId[2], 4);
    Hu3DModelShadowSet(obj->mdlId[0]);
    Hu3DModelShadowSet(obj->mdlId[1]);
    for (i = 0; i < 20; i++) {
        if (NULL == bomheiObjects[i]) {
            bomheiObjects[i] = obj;
            break;
        }
    }
    Hu3DMotionSet(obj->mdlId[0], obj->mtnId[0]);
    Hu3DModelAttrSet(obj->mdlId[2], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(obj->mdlId[0], HU3D_ATTR_DISPOFF);
    obj->objFunc = BomheiObjectUpdate;
}

void BomheiSpawn(Vec *position, float rotationY, s32 type, s32 initialState, s32 playerMaskBit)
{
    WorkBomhei *bomhei;
    OMOBJ *obj;
    activeBomheiCount++;
    obj = omAddObjEx(objectManager, 101, 3, 4, -1, BomheiObjectCreate);
    obj->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(WorkBomhei), HU_MEMNUM_OVL);
    bomhei = obj->data;
    bomhei->position = *position;
    bomhei->rotation.x = rotationY;
    bomhei->type = type;
    bomhei->state = initialState;
    bomhei->playerMaskBit = playerMaskBit;
    bomhei->rotation.y = bomhei->rotation.x;
    bomhei->rotation.z = bomhei->rotation.x;
    bomhei->timers.x = frandmod(REFRESH_RATE * 2) + REFRESH_RATE * 2;
    bomhei->timers.y = frandmod(REFRESH_RATE * 2) + REFRESH_RATE * 2;
    bomhei->timers.z = frandmod(REFRESH_RATE * 2) + REFRESH_RATE * 2;
    bomhei->lifetime = frandmod(REFRESH_RATE * 5) + REFRESH_RATE * 3;
    bomhei->motionId = 0;
}

void BomheiObjectDestroy(OMOBJ *obj)
{
    s32 i;
    activeBomheiCount--;
    for (i = 0; i < 20; i++) {
        if (obj == bomheiObjects[i]) {
            bomheiObjects[i] = NULL;
            break;
        }
    }
    for (i = 0; i < 3; i++) {
        Hu3DModelAttrSet(obj->mdlId[i], HU3D_ATTR_DISPOFF);
        TimedValueListInsert((i * 4096) + 4096, obj->mdlId[i]);
    }
    omDelObjEx(objectManager, obj);
}

typedef struct WorkBomhei2 {
    OMOBJ *object;
    OMOBJ *linkedObject;
    Vec position;
    Vec goalPosition;
    Vec previousPosition;
    float rotationY;
    float facingTargetY;
    float stateTimer;
    BOOL needsMotionRefresh;
    s32 motionState;
    s32 behaviorType;
    s32 behaviorState;
    float recoveryTimer;
} WorkBomhei2;

void BomheiSpawnScheduleUpdate(void)
{
    if (elapsedFrames % (REFRESH_RATE * 15) == 0) {
        if (++bomheiSpawnLimit >= 20) {
            bomheiSpawnLimit = 20;
        }
    }
    if (elapsedFrames % (REFRESH_RATE * 30) == 0) {
        if (bomheiSpawnChances[2] < 40) {
            bomheiSpawnChances[2] += 10;
            if (bomheiSpawnChances[2] > 100) {
                bomheiSpawnChances[2] = 100;
            }
            bomheiSpawnChances[1] -= 5;
            if (bomheiSpawnChances[1] < 0) {
                bomheiSpawnChances[1] = 0;
                bomheiSpawnChances[0] -= 5;
            }
            bomheiSpawnChances[0] -= 5;
            if (bomheiSpawnChances[0] < 0) {
                bomheiSpawnChances[0] = 0;
            }
        }
    }
    if (bomheiSpawnLimit > activeBomheiCount) {
        BomheiSpawnPoint *spawnPoint = NULL;
        s32 bitNum = frandmod(8);
        s32 i;
        for (i = 0; i < 8; i++) {
            if (((1 << bitNum) & spawnedBomheiMask) == 0) {
                spawnPoint = &bomheiArenaSpawnPoints[bitNum];
                spawnedBomheiMask |= (1 << bitNum);
                (void)i;
                break;
            }
            bitNum++;
            if (bitNum >= 8) {
                bitNum -= 8;
            }
        }
        if (NULL != spawnPoint) {
            s32 type = 2;
            s32 chance = frandmod(100);
            Vec spawnPosition;
            for (i = 0; i < 3; i++) {
                if (bomheiSpawnChances[i] > chance) {
                    type = i;
                    break;
                }
                chance -= bomheiSpawnChances[i];
            }
            spawnPosition.x = spawnPoint->position.x;
            spawnPosition.y = spawnPoint->position.y;
            spawnPosition.z = spawnPoint->position.z;
            BomheiSpawn(&spawnPosition, spawnPoint->rotationY, type, 3001, bitNum);
        }
    }
}

WorkBomhei2 *Bomhei2Create(OMOBJFUNC updateCallback, s32 priority)
{
    OMOBJ *obj = omAddObjEx(objectManager, priority, 1, 4, -1, updateCallback);
    WorkBomhei2 *bomhei;
    s32 i;
    s32 modelId;
    obj->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(WorkBomhei2), HU_MEMNUM_OVL);
    bomhei = obj->data;
    bomhei->needsMotionRefresh = 1;
    bomhei->motionState = 0;
    bomhei->object = obj;
    bomhei->linkedObject = NULL;
    bomhei->rotationY = 0;
    bomhei->position.x = 0;
    bomhei->position.y = 10;
    bomhei->position.z = 0;
    modelId = TimedValueListRemove(0x1000);
    if (modelId < 0) {
        obj->mdlId[0] = Hu3DModelLink(bomheiModelIds[0]);
    }
    else {
        obj->mdlId[0] = modelId;
    }
    for (i = 0; i < 4; i++) {
        obj->mtnId[i] = bomheiMotionIds[i];
    }
    omSetTra(obj, bomhei->position.x, bomhei->position.y, bomhei->position.z);
    omSetRot(obj, 0, bomhei->rotationY, 0);
    Hu3DModelAttrSet(obj->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(obj->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(obj->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(obj->mdlId[0], 3);
    Hu3DMotionSet(obj->mdlId[0], obj->mtnId[0]);
    Hu3DModelShadowSet(obj->mdlId[0]);
    return bomhei;
}

void Bomhei2ObjectUpdate(OMOBJ *obj)
{
    float dist;
    WorkBomhei2 *bomhei = obj->data;
    WorkBomhei2 *parent = bomhei->linkedObject->data;
    float vel = 3;
    float dx;
    float dz;

    s32 state;
    switch (bomhei->behaviorState) {
        case 0:
            if (bomhei->motionState != 1) {
                bomhei->motionState = 1;
                Hu3DMotionShiftSet(obj->mdlId[0], obj->mtnId[bomhei->motionState], 0, 8, HU3D_MOTATTR_LOOP);
            }
            dx = bomhei->goalPosition.x - bomhei->position.x;
            dz = bomhei->goalPosition.z - bomhei->position.z;
            dist = sqrtf((dx * dx) + (dz * dz));
            if (vel < dist) {
                dx = vel * (dx / dist);
                dz = vel * (dz / dist);
            }
            bomhei->position.x += dx;
            bomhei->position.z += dz;
            bomhei->rotationY = AngleInterpolate(bomhei->rotationY, atan2d(dx, dz), 0.4f);
            bomhei->facingTargetY = bomhei->rotationY;
            if (vel > dist) {
                switch (bomhei->behaviorType) {
                    case 1:
                    case 2:
                        bomhei->behaviorState = 1;
                        bomhei->stateTimer = 0;
                        bomhei->motionState = 0;
                        Hu3DMotionShiftSet(obj->mdlId[0], obj->mtnId[bomhei->motionState], 0, 8, HU3D_MOTATTR_LOOP);
                        break;

                    default:
                        bomhei->behaviorState = 3;
                        break;
                }
            }
            break;

        case 1:
            bomhei->rotationY = AngleInterpolate(bomhei->rotationY, atan2d(parent->position.x - bomhei->position.x, parent->position.z - bomhei->position.z), 0.05f);
            bomhei->facingTargetY = bomhei->rotationY;
            parent->rotationY = AngleInterpolate(parent->rotationY, atan2d(bomhei->position.x - parent->position.x, bomhei->position.z - parent->position.z), 0.05f);
            parent->facingTargetY = parent->rotationY;
            bomhei->stateTimer++;
            if (REFRESH_RATE_F * 3 < bomhei->stateTimer) {
                bomhei->behaviorState = 3;
                if (bomhei->behaviorType == 2) {
                    bomhei->behaviorState = 2;
                }
            }
            break;

        case 2: {
            Vec displacedPosition = parent->position;
            float tempAngle = parent->rotationY;
            parent->previousPosition = parent->position;
            parent->position = bomhei->position;
            parent->rotationY = bomhei->rotationY;
            bomhei->position = displacedPosition;
            bomhei->rotationY = tempAngle;
            bomhei->facingTargetY = tempAngle;
            bomhei->stateTimer = 0;
            bomhei->behaviorState = 3;
        } break;

        case 3:
            bomhei->facingTargetY = AngleInterpolate(bomhei->facingTargetY, atan2d(bomhei->previousPosition.x - bomhei->goalPosition.x, bomhei->previousPosition.z - bomhei->goalPosition.z), 0.2f);
            bomhei->rotationY = bomhei->facingTargetY;
            bomhei->position.x += vel * sind(bomhei->facingTargetY);
            bomhei->position.z += vel * cosd(bomhei->facingTargetY);
            if (bomhei->motionState != 1) {
                bomhei->motionState = 1;
                Hu3DMotionShiftSet(obj->mdlId[0], obj->mtnId[bomhei->motionState], 0, 8, HU3D_MOTATTR_LOOP);
            }
            parent->rotationY = AngleInterpolate(parent->rotationY, atan2d(-parent->position.x, -parent->position.z), 0.05f);
            bomhei->stateTimer++;
            if (bomhei->behaviorType == 2 && 60.0f < bomhei->stateTimer) {
                dx = parent->previousPosition.x - parent->position.x;
                dz = parent->previousPosition.z - parent->position.z;
                dist = sqrtf((dx * dx) + (dz * dz));
                if (vel < dist) {
                    dx = vel * (dx / dist);
                    dz = vel * (dz / dist);
                }
                parent->position.x += dx;
                parent->position.z += dz;
                state = 0;
                if (0.0f < dist) {
                    state = 1;
                }
                if (state != parent->motionState) {
                    parent->motionState = state;
                    Hu3DMotionShiftSet(parent->object->mdlId[0], parent->object->mtnId[parent->motionState], 0, 8, HU3D_MOTATTR_LOOP);
                }
            }
            if (1750 < VECMagXZ(&bomhei->position)) {
                parent->linkedObject = NULL;
                Hu3DModelAttrSet(obj->mdlId[0], HU3D_ATTR_DISPOFF);
                TimedValueListInsert(0x1000, obj->mdlId[0]);
                omDelObjEx(objectManager, obj);
                return;
            }
            break;
    }
    omSetTra(obj, bomhei->position.x, bomhei->position.y, bomhei->position.z);
    omSetRot(obj, 0, bomhei->rotationY, 0);
    if (bomhei->needsMotionRefresh) {
        bomhei->needsMotionRefresh = 0;
        Hu3DMotionTimeSet(obj->mdlId[0], Hu3DMotionMaxTimeGet(obj->mdlId[0]) * frandf());
        Hu3DModelAttrReset(obj->mdlId[0], HU3D_ATTR_DISPOFF);
    }
}

s32 Bomhei2LinkInitialize(WorkBomhei2 *parent, WorkBomhei2 *bomhei)
{
    parent->linkedObject = bomhei->object;
    bomhei->linkedObject = parent->object;
    bomhei->behaviorType = frandmod(3);
    bomhei->behaviorState = 0;
    bomhei->goalPosition.x = 750.0f * ((0 > parent->position.x) ? -1 : 1);
    bomhei->goalPosition.z = 750.0f * ((0 > parent->position.z) ? -1 : 1);
    if (spawnCycle & 0x1) {
        bomhei->position.x = 1400.0f * ((0 > parent->position.x) ? -1 : 1);
        bomhei->position.z = 750.0f * ((0 > parent->position.z) ? -1 : 1);
        bomhei->previousPosition.x = 750.0f * ((0 > parent->position.x) ? -1 : 1);
        bomhei->previousPosition.z = 1400.0f * ((0 > parent->position.z) ? -1 : 1);
    }
    else {
        bomhei->position.x = 750.0f * ((0 > parent->position.x) ? -1 : 1);
        bomhei->position.z = 1400.0f * ((0 > parent->position.z) ? -1 : 1);
        bomhei->previousPosition.x = 1400.0f * ((0 > parent->position.x) ? -1 : 1);
        bomhei->previousPosition.z = 750.0f * ((0 > parent->position.z) ? -1 : 1);
    }
    bomhei->rotationY = atan2d(bomhei->goalPosition.x - bomhei->position.x, bomhei->goalPosition.z - bomhei->position.z);
    bomhei->facingTargetY = bomhei->rotationY;
}

void Bomhei2UnlinkedUpdate(OMOBJ *obj)
{
    WorkBomhei2 *bomhei = obj->data;
    if (NULL == bomhei->linkedObject) {
        bomhei->rotationY = atan2d(-bomhei->position.x, -bomhei->position.z);
        bomhei->recoveryTimer--;
        if (0.0f > bomhei->recoveryTimer) {
            WorkBomhei2 *bomheiNew;
            bomhei->recoveryTimer = frandmod(REFRESH_RATE * 20) + REFRESH_RATE * 5;
            bomheiNew = Bomhei2Create(Bomhei2ObjectUpdate, 102);
            Bomhei2LinkInitialize(bomhei, bomheiNew);
        }
    }
    omSetTra(obj, bomhei->position.x, bomhei->position.y, bomhei->position.z);
    omSetRot(obj, 0, bomhei->rotationY, 0);
    if (bomhei->needsMotionRefresh) {
        bomhei->needsMotionRefresh = 0;
        Hu3DMotionTimeSet(obj->mdlId[0], Hu3DMotionMaxTimeGet(obj->mdlId[0]) * frandf());
        Hu3DModelAttrReset(obj->mdlId[0], HU3D_ATTR_DISPOFF);
    }
}

void MinigameCleanup(OMOBJ *obj);

void MinigameStateUpdate(OMOBJ *obj)
{
    if (omSysExitReq) {
        AudioSequenceFadeOut(-1);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        obj->objFunc = MinigameCleanup;
    }
    spawnCycle++;
    switch (obj->work[0]) {
        case 1000:
            if (!WipeStatGet()) {
                Vec dir;
                obj->work[0]++;
                stateTimer = 0;
                cameraEndPosition = finalCameraPosition;
                cameraEndTarget = finalCameraTarget;
                VECSubtract(&cameraEndTarget, &cameraEndPosition, &dir);
                VECNormalize(&dir, &dir);
                HuAudFXListnerSetEX(&cameraEndPosition, &dir, 14000.0, 36000.0, 1000.0, 300.0, 300.0);
            }

            break;

        case 1001:
            stateTimer++;
            if (30.0f < stateTimer) {
                obj->work[0]++;
                stateTimer = 0;
            }
            break;

        case 1002:
            stateTimer++;
            CameraPoseTransitionUpdate(stateTimer / (REFRESH_RATE_F * 2), &cameraTransitionTarget);
            if ((REFRESH_RATE_F * 2) <= stateTimer) {
                obj->work[0]++;
                stateTimer = 0;
                CameraPoseSet(&finalCameraPosition, &cameraTransitionTarget, &finalCameraTarget);
            }
            break;

        case 1003:
            if (messageSequenceId < 0) {
                messageSequenceId = MGSeqStartCreate();
            }
            else {
                if (musicSequenceId < 0 && (MGSeqStatGet(messageSequenceId) & 0x10)) {
                    AudioSequencePlay(70);
                }
                if (MGSeqStatGet(messageSequenceId) == 0) {
                    obj->work[0]++;
                    messageSequenceId = -1;
                }
            }
            break;

        case 1004:
            elapsedFrames++;
            TimerDigitsUpdate(timerSprites, 0, elapsedFrames);
            if (playerCollision || (REFRESH_RATE * 600 - 1) <= elapsedFrames) {
                obj->work[0] = 1005;
                stateTimer = 0;
            }
            BomheiSpawnScheduleUpdate();
            break;

        case 1005:
            if (messageSequenceId < 0) {
                messageSequenceId = MGSeqFinishCreate();
                AudioSequenceFadeOut(100);
                mgRecordExtra = elapsedFrames;
            }
            else {
                stateTimer++;
                if (60.0f < stateTimer && !MGSeqStatGet(messageSequenceId)) {
                    obj->work[0] = 1007;
                    if (bestTime < elapsedFrames) {
                        obj->work[0] = 1006;
                        RecordTimeUpdate(13);
                    }
                    messageSequenceId = -1;
                    stateTimer = 0;
                }
            }
            break;

        case 1006:
            if (messageSequenceId < 0) {
                messageSequenceId = MGSeqRecordCreate(bestTime);
            }
            else {
                if (!MGSeqStatGet(messageSequenceId)) {
                    obj->work[0]++;
                    stateTimer = 0;
                    messageSequenceId = -1;
                }
            }
            break;

        case 1007:
            stateTimer++;
            if (210.0f < stateTimer) {
                AudioSequenceFadeOut(-1);
                messageSequenceId = -1;
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                obj->objFunc = MinigameCleanup;
            }
            break;

        default:
            OSReport("*** main mode error(%d)!!\n", obj->work[0]);
            break;
    }
    TimerDisplayPulse();
}

void MinigameStateCreate(OMOBJ *obj)
{
    s32 i;
    obj->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M461, 0x00));
    Hu3DModelAttrSet(obj->mdlId[0], HU3D_ATTR_NOCULL | HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(obj->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(obj->mdlId[0], 0);
    Hu3DModelShadowMapSet(obj->mdlId[0]);
    omSetTra(obj, 0, 0, 0);
    omSetRot(obj, 0, 0, 0);
    obj->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M461, 0x01));
    Hu3DModelAttrSet(obj->mdlId[1], HU3D_ATTR_NOCULL | HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(obj->mdlId[1], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(obj->mdlId[1], 1);
    Hu3DModelShadowSet(obj->mdlId[1]);
    Hu3DModelPosSet(obj->mdlId[1], 0, 0, 0);
    Hu3DModelRotSet(obj->mdlId[1], 0, 0, 0);
    TimerDisplayCreate(64, DATA_MAKE_NUM(DATADIR_M461, 0x09));
    TimerDisplayShow();
    for (i = 0; i < 2; i++) {
        Hu3DModelAttrReset(obj->mdlId[i], HU3D_ATTR_DISPOFF);
    }
    HuAudFXPlay(2085);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    obj->objFunc = MinigameStateUpdate;
}

void ObjectSetup(void)
{
    s32 i;
    s32 light;
    float angle;
    float radius;
    float radiusPrev;
    OSReport("******* M461 ObjectSetup *********\n");
    objectManager = omInitObjMan(200, 8192);
    omGameSysInit(objectManager);
    playerCollision = FALSE;
    playerIndex = FALSE;
    spawnCycle = 0;
    messageSequenceId = -1;
    finishSequenceId = -1;
    musicSequenceId = -1;
    leftAudioEmitter = -1;
    rightAudioEmitter = -1;
    cameraVerticalOffset = 0;
    cameraDistance = 2200;
    cameraLookAtHeight = 2300;
    elapsedFrames = 0;
    bestTime = GWMGRecordGet(13);
    timerPulseAngle = -1;
    Hu3DCameraViewportSet(HU3D_CAM0, 0, 0, 640, 480, 0, 1);
    Hu3DCameraPerspectiveSet(HU3D_CAM0, 30, 20, 30000, 1.2f);
    CameraPoseSet(&initialCameraPosition, &initialCameraTarget, &initialCameraUp);
    light = Hu3DGLightCreateV(&lightPosition, &lightDirection, &lightColor);
    Hu3DGLightStaticSet(light, 1);
    Hu3DGLightInfinitytSet(light);
    Hu3DShadowCreate(30, 20, 30000);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowPosSet(&shadowPosition, &shadowUp, &shadowTarget);
    TimedValueListInit();
    for (i = 0; i < 20; i++) {
        s32 modelId;
        bomheiObjects[i] = NULL;
        modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M461, 0x08));
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
        TimedValueListInsert(12288, modelId);
    }
    for (i = 0; i < 3; i++) {
        bomheiModelIds[i] = Hu3DModelCreateFile(bomheiModelFiles[i]);
        Hu3DModelAttrSet(bomheiModelIds[i], HU3D_ATTR_NOCULL | HU3D_ATTR_DISPOFF);
    }
    for (i = 0; i < 4; i++) {
        bomheiMotionIds[i] = Hu3DJointMotionFile(bomheiModelIds[0], bomheiMotionFiles[i]);
    }
    spawnedBomheiMask = 0;
    activeBomheiCount = 0;
    bomheiSpawnLimit = 10;
    angle = 360.0f * frandf();
    radiusPrev = -1;
    for (i = 0; i < bomheiSpawnLimit; i++) {
        Vec spawnPosition;
        do {
            radius = frandmod(5);
        } while (radiusPrev == radius);
        radiusPrev = radius;
        spawnPosition.x = (200 + (100 * radius)) * sind(angle);
        spawnPosition.y = 0;
        spawnPosition.z = (200 + (100 * radius)) * cosd(angle);
        BomheiSpawn(&spawnPosition, 360.0f * frandf(), (i == 0) ? 1 : 0, 3000, -1);
        angle += 36;
        if (360.0f <= angle) {
            angle -= 360.0f;
        }
    }
    for (i = 0; i < 4; i++) {
        WorkBomhei2 *bomhei2 = Bomhei2Create(Bomhei2UnlinkedUpdate, 103);
        bomhei2->position = bomhei2SpawnPoints[i];
        bomhei2->rotationY = atan2d(-bomhei2->position.x, -bomhei2->position.z);
        bomhei2->recoveryTimer = frandmod(REFRESH_RATE * 20) + REFRESH_RATE * 5;
    }
    playerObject = omAddObjEx(objectManager, 100, 1, 5, -1, PlayerObjectCreate);
    gameControllerObject = omAddObjEx(objectManager, 104, 2, 0, -1, MinigameStateCreate);
    gameControllerObject->work[0] = 1000;
    stateTimer = 0;
}

void MinigameCleanup(OMOBJ *obj)
{
    s32 i;
    if (WipeStatGet()) {
        return;
    }
    if (messageSequenceId >= 0) {
        MGSeqKill(messageSequenceId);
    }
    if (finishSequenceId >= 0) {
        MGSeqKill(finishSequenceId);
    }
    MGSeqKillAll();
    HuAudAllStop();
    for (i = 0; i < 4; i++) {
        CharModelKill(characterIds[GWPlayerCfg[i].character]);
    }
    omOvlReturnEx(1, 1);
}
