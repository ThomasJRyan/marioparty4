#include "REL/executor.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/wipe.h"

#include "game/gamework.h"
#include "game/gamework_data.h"

#include "game/mapspace.h"
#include "game/pad.h"
#include "game/printfunc.h"

#include "game/hu3d.h"

#include "game/esprite.h"
#include "game/sprite.h"

#include "ext_math.h"

#include "string.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/hsfex.h"

void HuSysVWaitSet(s16 vcount);
#endif

#undef ABS
#define ABS(x) ((0 > (x)) ? -(x) : (x))

typedef struct ResourceValueEntry {
    /* 0x00 */ u16 resourceKey;
    /* 0x02 */ s16 resourceValue;
    /* 0x04 */ struct ResourceValueEntry *previousEntry;
    /* 0x08 */ struct ResourceValueEntry *nextEntry;
} ResourceValueEntry; // Size 0xC

typedef struct ParticleEffectEntry {
    /* 0x00 */ HU3DPARMANPARAM *particleParameters;
    /* 0x04 */ s32 *particleIds;
    /* 0x08 */ s32 remainingFrames;
    /* 0x0C */ s32 particleCount;
    /* 0x10 */ struct ParticleEffectEntry *previousEntry;
    /* 0x14 */ struct ParticleEffectEntry *nextEntry;
} ParticleEffectEntry; // Size 0x18

typedef struct {
    /* 0x00 */ u32 resourceId;
    /* 0x04 */ s16 cachedModelId;
} CachedModelEntry; // Size 8

ResourceValueEntry resourceListSentinel;
ParticleEffectEntry particleListSentinel;
HUPROCESS *gameObjectManager;
Vec cameraStartPosition;
Vec cameraStartDirection;
Vec cameraTargetPosition;
Vec cameraTargetDirection;
u32 gameFrameCounter;
s32 elapsedRoundFrames;
u32 bestRoundRecordFrames; // TODO maybe array?
s16 activeMessageSequence;
s16 countdownMessageSequence;
s32 activeMusicSequence;
s32 cameraTransitionActive;
float scorePulseAngle;
s32 timerDigitSprites[16];
CachedModelEntry cachedModels[32];

void InitializeResourceList(void)
{
    resourceListSentinel.previousEntry = NULL;
    resourceListSentinel.nextEntry = NULL;
}

void RegisterResourceValue(u16 resourceParameter0, s16 valueParameter1)
{
    ResourceValueEntry *newEntry;
    ResourceValueEntry *listSentinel;
    ResourceValueEntry *previousHead;

    newEntry = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*newEntry), HU_MEMNUM_OVL);
    newEntry->resourceKey = resourceParameter0;
    newEntry->resourceValue = valueParameter1;
    listSentinel = &resourceListSentinel;
    previousHead = listSentinel->nextEntry;
    listSentinel->nextEntry = newEntry;
    if (NULL != previousHead) {
        previousHead->previousEntry = newEntry;
    }
    newEntry->previousEntry = listSentinel;
    newEntry->nextEntry = previousHead;
}

s16 RemoveResourceValue(s16 valueParameter0)
{
    ResourceValueEntry *entry;
    ResourceValueEntry *previousEntry;
    ResourceValueEntry *nextEntry;
    s16 resourceValue;

    entry = resourceListSentinel.nextEntry;
    while (NULL != entry) {
        if (valueParameter0 == entry->resourceKey) {
            resourceValue = entry->resourceValue;
            previousEntry = entry->previousEntry;
            nextEntry = entry->nextEntry;
            previousEntry->nextEntry = nextEntry;
            if (NULL != nextEntry) {
                nextEntry->previousEntry = previousEntry;
            }
            HuMemDirectFree(entry);
            return resourceValue;
        }
        entry = entry->nextEntry;
    }
    return -1;
}

void InitializeParticleList(void)
{
    particleListSentinel.previousEntry = NULL;
    particleListSentinel.nextEntry = NULL;
}

void CreateParticleEffect(s16 valueParameter0, HU3DPARMANPARAM *particleParameters, s32 indexParameter2, s32 indexParameter3, s32 indexParameter4, Vec *positionParameter5, u8 modeParameter6)
{
    ParticleEffectEntry *listSentinel;
    ParticleEffectEntry *newEntry;
    ParticleEffectEntry *previousHead;
    s32 i;

    newEntry = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*newEntry), HU_MEMNUM_OVL);
    newEntry->particleParameters = particleParameters;
    newEntry->remainingFrames = indexParameter3;
    newEntry->particleCount = indexParameter4;
    newEntry->particleIds = HuMemDirectMallocNum(HEAP_HEAP, indexParameter4 * sizeof(*newEntry->particleIds), HU_MEMNUM_OVL);
    for (i = 0; i < indexParameter4; i++, positionParameter5++) {
        newEntry->particleIds[i] = Hu3DParManLink(valueParameter0, particleParameters);
        if (newEntry->particleIds[i] >= 0) {
            Hu3DParManAttrReset(newEntry->particleIds[i], 1);
            Hu3DParManPosSet(newEntry->particleIds[i], positionParameter5->x, positionParameter5->y, positionParameter5->z);
            Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(newEntry->particleIds[i]), modeParameter6);
            Hu3DModelLayerSet(Hu3DParManModelIDGet(newEntry->particleIds[i]), indexParameter2);
        }
    }
    previousHead = &particleListSentinel;
    listSentinel = previousHead->nextEntry;
    previousHead->nextEntry = newEntry;
    if (NULL != listSentinel) {
        listSentinel->previousEntry = newEntry;
    }
    newEntry->previousEntry = previousHead;
    newEntry->nextEntry = listSentinel;
}

void UpdateParticleEffects(void)
{
    ParticleEffectEntry *particleEntry;
    ParticleEffectEntry *previousEntry;
    ParticleEffectEntry *nextEntry;
    s32 i;

    particleEntry = particleListSentinel.nextEntry;
    while (NULL != particleEntry) {
        particleEntry->remainingFrames--;
        if (particleEntry->remainingFrames == 0) {
            for (i = 0; i < particleEntry->particleCount; i++) {
                if (particleEntry->particleIds[i] >= 0) {
                    Hu3DParManAttrSet(particleEntry->particleIds[i], 1);
                }
            }
        }
        previousEntry = particleEntry->previousEntry;
        nextEntry = particleEntry->nextEntry;
        if (-particleEntry->particleParameters->maxTime > particleEntry->remainingFrames) {
            for (i = 0; i < particleEntry->particleCount; i++) {
                if (particleEntry->particleIds[i] >= 0) {
                    Hu3DParManKill(particleEntry->particleIds[i]);
                }
            }
            previousEntry->nextEntry = nextEntry;
            if (NULL != nextEntry) {
                nextEntry->previousEntry = previousEntry;
            }
            HuMemDirectFree(particleEntry->particleIds);
            HuMemDirectFree(particleEntry);
        }
        particleEntry = nextEntry;
    }
}

void InitializeModelCache(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        cachedModels[i].resourceId = 0;
        cachedModels[i].cachedModelId = -1;
    }
}

s16 GetCachedModel(s32 resourceIdParameter0)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (resourceIdParameter0 == cachedModels[i].resourceId) {
            return Hu3DModelLink(cachedModels[i].cachedModelId);
        }
    }
    for (i = 0; i < 32; i++) {
        if (cachedModels[i].resourceId == 0) {
            cachedModels[i].resourceId = resourceIdParameter0;
            cachedModels[i].cachedModelId = Hu3DModelCreateFile(resourceIdParameter0);
            return cachedModels[i].cachedModelId;
        }
    }
    return Hu3DModelCreateFile(resourceIdParameter0);
}

void SetTimerDigitSprites(s32 *integerPointerParameter0, s32 indexParameter1, s32 indexParameter2)
{
    s32 digitValue;

    espBankSet(integerPointerParameter0[indexParameter1 + 1], 10);
    espBankSet(integerPointerParameter0[indexParameter1 + 4], 11);
    if (600 * REFRESH_RATE <= integerPointerParameter0) {
        integerPointerParameter0 = 600 * REFRESH_RATE - 1;
    }
    digitValue = indexParameter2 / (60 * REFRESH_RATE);
    indexParameter2 -= digitValue * (60 * REFRESH_RATE);
    if (digitValue > 9) {
        digitValue = 9;
    }
    espBankSet(integerPointerParameter0[indexParameter1], digitValue);
    digitValue = indexParameter2 / REFRESH_RATE;
    indexParameter2 -= digitValue * REFRESH_RATE;
    espBankSet(integerPointerParameter0[indexParameter1 + 2], digitValue / 10);
    espBankSet(integerPointerParameter0[indexParameter1 + 3], digitValue % 10);
    digitValue = 101.5f * indexParameter2 / REFRESH_RATE;
    espBankSet(integerPointerParameter0[indexParameter1 + 5], digitValue / 10);
    espBankSet(integerPointerParameter0[indexParameter1 + 6], digitValue % 10);
}

void StoreRoundRecord(s32 indexParameter0)
{
    scorePulseAngle = 0.0f;
    bestRoundRecordFrames = elapsedRoundFrames;
    GWMGRecordSet(indexParameter0, bestRoundRecordFrames);
    SetTimerDigitSprites(timerDigitSprites, 7, elapsedRoundFrames);
}

void ShowTimerSprites(void)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        espDispOn(timerDigitSprites[i]);
    }
}

void UpdateScorePulse(void)
{
    float pulseScale;
    s32 i;

    if (0.0f <= scorePulseAngle) {
        scorePulseAngle += 16.0f;
        pulseScale = 1.0 + 0.2f * sind(scorePulseAngle);
        for (i = 7; i <= 14; i++) {
            espScaleSet(timerDigitSprites[i], pulseScale, pulseScale);
        }
    }
}

void CreateTimerSprites(float valueParameter0, s32 indexParameter1)
{
    float spriteY;
    float spriteX;
    s32 i;

    spriteX = 288.0f;
    spriteY = valueParameter0;
    timerDigitSprites[15] = espEntry(indexParameter1, 0, 0);
    espTPLvlSet(timerDigitSprites[15], 0.5f);
    espColorSet(timerDigitSprites[15], 0, 0, 0);
    espPosSet(timerDigitSprites[15], spriteX, spriteY);
    spriteX = 248.0f;
    spriteY = valueParameter0 - 8.0f;
    timerDigitSprites[14] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 55), 0, 0);
    espPosSet(timerDigitSprites[14], spriteX - 18.0f, spriteY);
    for (i = 7; i <= 13; i++) {
        timerDigitSprites[i] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 48), 0, 0);
        espPosSet(timerDigitSprites[i], spriteX, spriteY);
        spriteX += 16.0f;
    }
    SetTimerDigitSprites(timerDigitSprites, 7, bestRoundRecordFrames);
    spriteX = 248.0f;
    spriteY = valueParameter0 + 8.0f;
    for (i = 0; i <= 6; i++) {
        timerDigitSprites[i] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 49), 0, 0);
        espPosSet(timerDigitSprites[i], spriteX, spriteY);
        spriteX += 16.0f;
    }
    SetTimerDigitSprites(timerDigitSprites, 0, 0);
    for (i = 0; i < 16; i++) {
        espDrawNoSet(timerDigitSprites[i], 0);
        espDispOff(timerDigitSprites[i]);
        espAttrSet(timerDigitSprites[i], HUSPR_ATTR_NOANIM);
        espPriSet(timerDigitSprites[i], 0x80 + i);
    }
}

void CalculateCameraPosition(float valueParameter0, Vec *positionParameter1, Vec *positionParameter2, Vec *positionParameter3)
{
    float easedProgress;

    if (1.0f < valueParameter0) {
        valueParameter0 = 1.0f;
    }
    easedProgress = sind(90.0f * valueParameter0) * sind(90.0f * valueParameter0);
    positionParameter3->x = positionParameter1->x + easedProgress * (positionParameter2->x - positionParameter1->x);
    positionParameter3->y = positionParameter1->y + easedProgress * (positionParameter2->y - positionParameter1->y);
    positionParameter3->z = positionParameter1->z + easedProgress * (positionParameter2->z - positionParameter1->z);
}

void SetCameraTransition(Vec *positionParameter0, Vec *positionParameter1, Vec *positionParameter2)
{
    Hu3DCameraPosSetV(1, positionParameter0, positionParameter1, positionParameter2);
    cameraStartPosition = *positionParameter0;
    cameraTargetPosition = *positionParameter2;
}

s32 UpdateCameraTransition(float valueParameter0, Vec *positionParameter1)
{
    Vec cameraPosition;
    Vec cameraTarget;

    CalculateCameraPosition(valueParameter0, &cameraStartPosition, &cameraStartDirection, &cameraPosition);
    CalculateCameraPosition(valueParameter0, &cameraTargetPosition, &cameraTargetDirection, &cameraTarget);
    Hu3DCameraPosSetV(1, &cameraPosition, positionParameter1, &cameraTarget);
    if (1.0f <= valueParameter0) {
        return 1;
    }
    else {
        cameraTransitionActive = 1;
        return 0;
    }
}

void FinishCameraTransition(float valueParameter0, Vec *positionParameter1)
{
    Vec cameraPosition;
    Vec cameraTarget;

    CalculateCameraPosition(valueParameter0, &cameraStartPosition, &cameraStartDirection, &cameraPosition);
    CalculateCameraPosition(valueParameter0, &cameraTargetPosition, &cameraTargetDirection, &cameraTarget);
    SetCameraTransition(&cameraPosition, positionParameter1, &cameraTarget);
    cameraTransitionActive = 1;
}

void CalculateModelAttachment(Vec *positionParameter0, s16 valueParameter1, Vec *positionParameter2)
{
    Mtx viewMatrix;
    Vec viewPosition;
    float horizontalProjectionScale;
    float verticalProjectionScale;
    float viewportX;
    float viewportWidth;
    HU3DCAMERA *camera;
    s32 i;

    for (i = 0; i < 16; i++) {
        if (positionParameter0 & (1 << i)) {
            break;
        }
    }
    camera = &Hu3DCamera[i];
    MTXLookAt(viewMatrix, &camera->pos, &camera->up, &camera->target);
    MTXMultVec(viewMatrix, positionParameter0, &viewPosition);
    horizontalProjectionScale = viewPosition.z * (sind(camera->fov / 2) / cosd(camera->fov / 2)) * camera->aspect;
    verticalProjectionScale = viewPosition.z * (sind(camera->fov / 2) / cosd(camera->fov / 2));
    viewportX = 0.9f * camera->viewportX;
    viewportWidth = 0.9f * camera->viewportW;
    positionParameter2->x = viewportWidth / 2 + viewPosition.x * ((viewportWidth / 2) / -horizontalProjectionScale) + viewportX;
    positionParameter2->y = camera->viewportH / 2 + viewPosition.y * ((camera->viewportH / 2) / verticalProjectionScale) + camera->viewportY;
    positionParameter2->z = 0.0f;
}

void GetModelAttachmentPosition(s16 valueParameter0, char *nameParameter1, Vec *positionParameter2)
{
    Mtx objectMatrix;

    Hu3DModelObjMtxGet(valueParameter0, nameParameter1, objectMatrix);
    positionParameter2->x = objectMatrix[0][3];
    positionParameter2->y = objectMatrix[1][3];
    positionParameter2->z = objectMatrix[2][3];
}

void GetModelAttachmentMatrix(s16 valueParameter0, char *nameParameter1, Vec *positionParameter2, Mtx matrixParameter3)
{
    Mtx objectMatrix;
    HU3DMODEL *model;

    model = &Hu3DData[valueParameter0];
    Hu3DModelObjMtxGet(valueParameter0, nameParameter1, objectMatrix);
    MTXConcat(model->mtx, objectMatrix, objectMatrix);
    if (NULL != valueParameter0) {
        MTXCopy(objectMatrix, valueParameter0);
    }
    positionParameter2->x = objectMatrix[0][3];
    positionParameter2->y = objectMatrix[1][3];
    positionParameter2->z = objectMatrix[2][3];
}

float InterpolateValue(float valueParameter0, float valueParameter1, float valueParameter2)
{
    float angleDelta;
    float interpolatedAngle;

    angleDelta = fmod(valueParameter1 - valueParameter0, 360.0);
    if (0.0f > angleDelta) {
        angleDelta += 360.0f;
    }
    if (180.0f < angleDelta) {
        angleDelta -= 360.0f;
    }
    interpolatedAngle = fmod(valueParameter0 + angleDelta * valueParameter2, 360.0);
    if (0.0f > interpolatedAngle) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

void PlayMusicSequence(s32 indexParameter0)
{
    activeMusicSequence = HuAudSeqPlay(indexParameter0);
}

void StopMusicSequence(s32 indexParameter0)
{
    if (activeMusicSequence >= 0) {
        if (indexParameter0 < 0) {
            indexParameter0 = 1000;
        }
        HuAudSeqFadeOut(activeMusicSequence, indexParameter0);
    }
    activeMusicSequence = -1;
}

void CalculatePlayerCamera(Vec *positionParameter0, float *valuePointerParameter1, s32 indexParameter2, s32 indexParameter3)
{
    Vec movementVector;
    float movementMagnitude;
    s32 movementState;

    movementVector.x = 0.0f;
    movementVector.y = 0.0f;
    movementVector.z = 0.0f;
    movementVector.x = indexParameter2 / 4.0f;
    movementVector.z = -(float)indexParameter3 / 4.0f;
    movementState = 0;
    movementMagnitude = VECMagXZ(&movementVector);
    if (0.5f <= movementMagnitude) {
        if (10.0f <= movementMagnitude) {
            movementVector.x /= movementMagnitude;
            movementVector.z /= movementMagnitude;
            movementMagnitude = 10.0f;
            movementVector.x = 10.0f * movementVector.x;
            movementVector.z = 10.0f * movementVector.z;
        }
        *valuePointerParameter1 = InterpolateValue(*valuePointerParameter1, atan2d(movementVector.x, movementVector.z), 0.4f);
        if (8.0f <= movementMagnitude) {
            movementState = 2;
        }
        else {
            movementState = 1;
        }
    }
    VECAdd((Vec *)&positionParameter0, &movementVector, (Vec *)&positionParameter0);
}

s16 characterModelIds[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
s32 motionStateIds[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
float characterCollisionRadii[0xA] = { 70.0f, 70.0f, 70.0f, 70.0f, 70.0f, 70.0f, 70.0f, 70.0f, 70.0f, 100.0f };
float characterCollisionHeights[0xA] = {
    150.0f,
    170.0f,
    190.0f,
    170.0f,
    160.0f,
    150.0f,
    180.0f,
    200.0f,
    150.0f,
    200.0f,
};

// these are copied from m461Dll/main.c
typedef void (*PlayerOldHook)(OMOBJ *obj);
typedef Vec *(*PlayerOldPosHook)(OMOBJ *obj);

typedef struct PlayerRuntimeState {
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
    s32 unused30;
    s32 unused34;
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

PlayerOldFind playerLookupFunction;
float groundProbeHeight;

void UpdatePlayerMotionState(WorkPlayerOld *player)
{
    s32 i;
    for (i = 0; i < 8; i++) {
        if (player->motionId == player->motionIds[i]) {
            player->motionState = motionStateIds[i];
            return;
        }
    }
    player->motionState = -1;
}

void UpdateMinimumPlayerDistance(float *minDist, s32 *state, s32 stateNew, Vec *posA, float aYOfs, Vec *posB, float bYOfs)
{
    Vec pointA = *posA;
    Vec pointB = *posB;
    float dist;
    pointA.y += aYOfs;
    pointB.y += bYOfs;
    dist = VECDistance(&pointA, &pointB);
    if (*minDist > dist) {
        *minDist = dist;
        *state = stateNew;
    }
}

void ComparePlayerDistances(WorkPlayerOld *player1, WorkPlayerOld *player2, float *minDist, s32 *state)
{
    *minDist = 10 * (player1->collisionRadius + player2->collisionRadius);
    UpdateMinimumPlayerDistance(minDist, state, 0, &player1->position, player1->collisionRadius, &player2->position, player2->collisionRadius);
    UpdateMinimumPlayerDistance(minDist, state, 1, &player1->position, player1->collisionHeight - player1->collisionRadius, &player2->position, player2->collisionHeight - player2->collisionRadius);
    UpdateMinimumPlayerDistance(minDist, state, 2, &player1->position, player1->collisionRadius, &player2->position, player2->collisionHeight - player2->collisionRadius);
    UpdateMinimumPlayerDistance(minDist, state, 3, &player1->position, player1->collisionHeight - player1->collisionRadius, &player2->position, player2->collisionRadius);
}

s32 GetPlayerCollisionState(WorkPlayerOld *player, WorkPlayerOld *player2)
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
    VECSubtract(&player->position, &player2->position, &dir);
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

void SetPlayerCollisionState(WorkPlayerOld *player, float valueParameter1, float valueParameter2)
{
    player->actionState = 1;
    player->velocity.y = valueParameter1;
    player->jumpTimer = valueParameter2;
    if (player->movementFlags & 0x2) {
        player->movementFlags &= ~0x2;
    }
}

void InitializePlayerCollision(PlayerOldFind find, float yDist)
{
    playerLookupFunction = find;
    groundProbeHeight = yDist;
}

void CreatePlayerObject(WorkPlayerOld *player, OMOBJ *obj, s32 indexParameter2, s32 indexParameter3, s32 *integerPointerParameter4, s32 indexParameter5, Vec *pos, float valueParameter7)
{
    player->object = obj;
    player->playerIndex = indexParameter2;
    player->characterIndex = indexParameter3;
    player->motionIds = integerPointerParameter4;
    player->updateCallback = NULL;
    player->positionCallback = NULL;
    player->landingCallback = NULL;
    player->collisionCallback = NULL;
    player->groundPositions = NULL;
    player->collisionBoxCount = 0;
    player->collisionBoxes = 0;
    player->actionState = 0;
    player->movementFlags = 0;
    player->motionId = indexParameter5;
    player->motionFrame = 0;
    player->collisionFrame = 0;
    UpdatePlayerMotionState(player);
    player->rotationY = valueParameter7;
    player->position = *pos;
    player->previousPosition = *pos;
    player->velocity.x = 0;
    player->velocity.y = 0;
    player->velocity.z = 0;
    player->movementScale = 1;
    player->groundHeight = 0;
    player->collisionRadius = characterCollisionRadii[player->characterIndex];
    player->collisionHeight = characterCollisionHeights[player->characterIndex];
}

void SetPlayerUpdateCallback(WorkPlayerOld *player, PlayerOldHook func)
{
    player->updateCallback = func;
}

void SetPlayerPositionCallback(WorkPlayerOld *player, PlayerOldPosHook func)
{
    player->positionCallback = func;
}

void SetPlayerLandingCallback(WorkPlayerOld *player, PlayerOldHook func)
{
    player->landingCallback = func;
}

void SetPlayerCollisionCallback(WorkPlayerOld *player, PlayerOldHook func)
{
    player->collisionCallback = func;
}

void SetPlayerGroundPositions(WorkPlayerOld *player, s32 count, Vec *data)
{
    player->collisionBoxCount = count;
    player->collisionBoxes = data;
}

void ClearPlayerGroundPositions(WorkPlayerOld *player)
{
    omSetTra(player->object, player->position.x, player->position.y, player->position.z);
    omSetRot(player->object, 0.0f, player->rotationY, 0.0f);
}

void UpdatePlayerPhysics(WorkPlayerOld *player)
{
    BOOL addTime;
    s32 i;
    player->movementFlags &= ~0x100;
    if (player->movementFlags & 0x8) {
        return;
    }
    if (NULL == playerLookupFunction) {
        return;
    }
    addTime = FALSE;
    for (i = 0; i < 4; i++) {
        WorkPlayerOld *player2 = playerLookupFunction(i);
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
        ComparePlayerDistances(player, player2, &minDist, &state);
        if (minDist < player->collisionRadius + player2->collisionRadius) {
            if (state < 2) {
                fixPos = TRUE;
                if (player->actionState == 1) {
                    GetPlayerCollisionState(player, player2);
                }
            }
            else {
                if (state == 2 && player->actionState == 1) {
                    GetPlayerCollisionState(player, player2);
                    player->movementFlags |= 0x100;
                    player->jumpTimer = 0;
                    if (!(player2->movementFlags & 0x100)) {
                        player->velocity.y = (20.0f*(80.3f/REFRESH_RATE));
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

float GetPlayerMotionFrame(WorkPlayerOld *player, float valueParameter1)
{
    Vec *pos;
    s32 i;
    AddX = 0.0f;
    AddZ = 0.0f;
    MapWall(player->collisionRadius, player->position.x, valueParameter1 + player->collisionRadius, player->position.z);
    player->position.x += AddX;
    player->position.z += AddZ;
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

void UpdatePlayerIdleMotion(WorkPlayerOld *player, s32 *state, u32 *motAttr)
{
    *motAttr = HU3D_MOTATTR_LOOP;
    *state = 0;
    if (0.5f <= player->horizontalSpeed) {
        if (7.0f <= player->horizontalSpeed) {
            *state = 2;
        }
        else {
            *state = 1;
        }
    }

    if (player->motionIds[3] >= 0 && (player->motionFlags & 0x100)) {
        SetPlayerCollisionState(player, (20.0f*(80.3f/REFRESH_RATE)), 1);
        *motAttr = HU3D_MOTATTR_NONE;
        *state = 3;
    }
}

void UpdatePlayerWalkMotion(WorkPlayerOld *player, s32 *state, u32 *motAttr)
{
    if (0.0f < player->jumpTimer) {
        if (player->motionStateFlags & 0x100) {
            player->jumpTimer++;
            if (4.0f < player->jumpTimer) {
                player->jumpTimer = 0;
            }
            else {
                player->velocity.y += 219.0f / REFRESH_RATE;
            }
        }
        else {
            player->jumpTimer = 0;
        }
    }
    player->velocity.y += -146.0f / REFRESH_RATE;
    if (player->motionIds[5] >= 0 && (player->motionFlags & 0x140)) {
        player->actionState = 3;
        player->velocity.y = 0;
        player->movementFlags &= ~0x3;
        player->movementFlags |= 0x4;
        *motAttr = HU3D_MOTATTR_NONE;
        *state = 5;
    }
}

void UpdatePlayerJumpMotion(WorkPlayerOld *player, s32 *state, u32 *motAttr)
{
    if ((player->movementFlags & 0x1000) || Hu3DMotionEndCheck(player->object->mdlId[0])) {
        player->movementFlags &= ~0x3;
        if (player->motionState == 6) {
            player->actionState = 4;
            *motAttr = HU3D_MOTATTR_NONE;
            *state = 7;
        }
        else {
            player->actionState = 0;
            *motAttr = HU3D_MOTATTR_LOOP;
            *state = 0;
        }
    }
}

void UpdatePlayerFallMotion(WorkPlayerOld *player, s32 *state, u32 *motAttr)
{
    if (Hu3DMotionShiftIDGet(player->object->mdlId[0]) < 0 && Hu3DMotionEndCheck(player->object->mdlId[0])) {
        SetPlayerCollisionState(player, 0.0f, 0.0f);
    }
}

void UpdatePlayerLandMotion(WorkPlayerOld *player, s32 *state, u32 *motAttr)
{
    if (Hu3DMotionShiftIDGet(player->object->mdlId[0]) < 0 && Hu3DMotionEndCheck(player->object->mdlId[0])) {
        player->movementFlags &= ~0x4;
        player->actionState = 0;
        *motAttr = HU3D_MOTATTR_LOOP;
        *state = 0;
    }
}

void UpdatePlayerMovement(WorkPlayerOld *player)
{
    u32 motAttr = HU3D_MOTATTR_NONE;
    s32 state = -1;
    float horizontalCollisionDistance;
    float heightAboveGround;
    float previousY;
    Vec *pos;
    BOOL isOnMovingPlatform;
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
            UpdatePlayerIdleMotion(player, &state, &motAttr);
            break;

        case 1:
            UpdatePlayerWalkMotion(player, &state, &motAttr);
            break;

        case 2:
            UpdatePlayerJumpMotion(player, &state, &motAttr);
            break;

        case 3:
            UpdatePlayerFallMotion(player, &state, &motAttr);
            break;

        case 4:
            UpdatePlayerLandMotion(player, &state, &motAttr);
            break;

        default:
            OSReport("*** player action error(%d)!!\n", player->actionState);
            break;
    }
    if (player->movementFlags & 0x2000) {
        player->velocity.y = 0;
    }
    horizontalCollisionDistance = 0;
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
    previousY = player->position.y;
    VECAdd(&player->position, &player->velocity, &player->position);
    if ((player->movementFlags & 0x1) == 0 && (0.0f != player->velocity.x || 0.0f != player->velocity.z)) {
        player->rotationY = InterpolateValue(player->rotationY, atan2d(player->velocity.x, player->velocity.z), 0.4f * player->movementScale);
    }
    horizontalCollisionDistance += GetPlayerMotionFrame(player, previousY);
    if (nMap == 0) {
        player->groundHeight = groundProbeHeight;
    }
    else {
        player->groundHeight = MapPos(player->position.x, player->position.y, player->position.z, player->collisionHeight, &player->groundNormal);
    }
    isOnMovingPlatform = FALSE;
    if (NULL != player->positionCallback) {
        pos = player->positionCallback(player->object);
        if (NULL != pos) {
            if (player->groundHeight < pos[1].y) {
                isOnMovingPlatform = TRUE;
                player->groundPositions = pos;
                player->groundHeight = pos[1].y;
            }
        }
    }
    if (!isOnMovingPlatform && (player->movementFlags & 0x2)) {
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
        if (isOnMovingPlatform) {
            player->movementFlags |= 0x2;
        }
    }
    heightAboveGround = player->position.y - player->groundHeight;
    if (player->actionState == 0 && 10.0f < heightAboveGround) {
        if ((player->movementFlags & 0x2000) == 0) {
            if ((player->movementFlags & 0x400) != 0) {
                player->movementFlags |= 0x800;
            }
            else {
                SetPlayerCollisionState(player, 0.0f, 0.0f);
                motAttr = HU3D_MOTATTR_NONE;
                state = 3;
            }
        }
    }
    else if (isOnMovingPlatform) {
        if (!(player->movementFlags & 0x2)) {
            player->position.y = player->groundHeight;
            player->movementFlags |= 0x2;
        }
    }
    UpdatePlayerPhysics(player);
    if (NULL != player->updateCallback) {
        player->updateCallback(player->object);
    }
    horizontalCollisionDistance += GetPlayerMotionFrame(player, player->position.y);
    if (player->movementFlags & 0x2) {
        player->groundRotation = player->groundPositions[2].x;
        VECSubtract(&player->position, &player->groundPositions[0], &player->groundOffset);
    }
    if (0.0f != horizontalCollisionDistance) {
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

void SetPlayerMotion(WorkPlayerOld *player, s32 motNo)
{
    s32 i;
    player->motionId = motNo;
    player->actionState = 0;
    player->movementFlags &= 0x1400;
    player->previousPosition = player->position;
    player->velocity.x = 0;
    player->velocity.y = 0;
    player->velocity.z = 0;
    UpdatePlayerMotionState(player);
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

void SetPlayerModelAttributes(WorkPlayerOld *player, s32 attr)
{
    player->movementFlags |= attr;
}

void ClearPlayerModelAttributes(WorkPlayerOld *player, s32 attr)
{
    player->movementFlags &= ~attr;
}

#include "REL/m432data.h"

typedef struct TeamResultState {
    /* 0x000 */ OMOBJ *playerObject;
    /* 0x004 */ OMOBJ *stageObject;
    /* 0x008 */ OMOBJ *linkedPlayerObject;
    /* 0x00C */ OMOBJ *pendingPlayerObject;
    /* 0x010 */ MapObjectDefinition *mapObjectLists[2];
    /* 0x018 */ s32 unreferencedState18;
    /* 0x01C */ Vec cameraPosition;
    /* 0x028 */ Vec cameraLookTarget;
    /* 0x034 */ Vec cameraStartPosition;
    /* 0x040 */ Vec cameraEndPosition;
    /* 0x04C */ Vec cameraStartLookTarget;
    /* 0x058 */ Vec cameraEndLookTarget;
    /* 0x064 */ float cameraOrbitAngle;
    /* 0x068 */ float cameraDistance;
    /* 0x06C */ float cameraHeight;
    /* 0x070 */ float cameraTargetHeight;
    /* 0x074 */ float cameraTargetDepth;
    /* 0x078 */ float cameraTransitionFrame;
    /* 0x07C */ float cameraTransitionDuration;
    /* 0x080 */ float characterRotationA;
    /* 0x084 */ float characterRotationB;
    /* 0x088 */ Vec characterAttachmentPositions[4][3];
    /* 0x118 */ Vec sceneObjectPosition;
    /* 0x124 */ float motionTimeRatio;
    /* 0x128 */ float motionTime;
    /* 0x12C */ float unreferencedMotionState;
    /* 0x130 */ Vec spawnPositions[8];
    /* 0x190 */ Vec currentHookPosition;
    /* 0x19C */ Vec previousHookPosition;
    /* 0x1A8 */ Vec targetHookPosition;
    /* 0x1B4 */ s32 hookSequenceState;
    /* 0x1B8 */ s32 selectedObjectIndex;
    /* 0x1BC */ s32 selectedVariantIndex;
    /* 0x1C0 */ Vec hookModelPosition;
    /* 0x1CC */ s32 impactSoundId;
    /* 0x1D0 */ s32 followupSoundId;
    /* 0x1D4 */ float impactSoundFade;
    /* 0x1D8 */ s32 resultSoundId;
    /* 0x1DC */ float resultSoundFade;
    /* 0x1E0 */ float playerDepthLimit;
    /* 0x1E4 */ float objectFloorHeight;
    /* 0x1E8 */ float resultFramesElapsed;
    /* 0x1EC */ float resultMotionTime;
    /* 0x1F0 */ float playerDepths[2];
} TeamResultState; /* size = 0x1F8 */

typedef struct PlayerGameState {
    WorkPlayerOld playerState;
    s32 *scoreSpriteLayout;
    MapObjectDefinition *activeMapObject;
    PlayerAiBehaviorParams *aiBehaviorParams;
    MapObjectDefinition *pendingMapObject;
    s32 recentResultHistory[10];
    s32 resultHistoryIndex;
    s32 resultSequenceCounter;
    float aiMovementScale;
    float aiActionDelay;
    float targetSearchTimer;
    float scoreSpriteX;
    float scoreSpriteY;
    s32 scoreSpriteIds[9];
    Vec movementTarget;
    float cameraMoveRate;
    float collisionTimer;
    float animationDirection;
    float previousAnimationProgress;
    float animationProgress;
    float playerDepthCap;
    float bouncePhase;
    float bounceStartHeight;
    s32 completionPending;
    s32 inputAcknowledged;
    s32 queuedStickX;
    s32 queuedStickY;
    s32 impactCount;
    s32 mapObjectIndex;
    s32 playerMode;
    s32 playerSlot;
    s32 playerConfigIndex;
    s32 characterIndex;
    s32 controllerPadIndex;
    s32 playerGroup;
} PlayerGameState;

typedef struct FloatingEffectState {
    Vec position;
    float scale;
    float lifetime;
    float xDisplacement;
    float zDisplacement;
} FloatingEffectState; /* size = 0x1C */

void InitializePlayerObject(OMOBJ *object);
void InitializeGameScene(void);
void ExitPostGame(OMOBJ *object);
void UpdateTeamResults(TeamResultState *teamState);
void UpdatePlayerBehavior(PlayerGameState *playerState);

OMOBJ *postGameController;
OMOBJ **effectObjectList;
OMOBJ *floatingEffectObject;
// data PlayerGameState
OMOBJ **playerObjectList;
s32 unreferencedPlayerState;
ANIMDATA *playerSpriteAnimation;
s16 particleManagerId;
s16 floatingEffectFrame;
s16 stageModelId;
s32 winningTeam;
s32 completedPlayerFrames;
s32 resultSoundHandle;
float stateTransitionTimer;
TeamResultState teamResultStates[2];

void InitializePlayerGameState(PlayerGameState *playerStateParameter0)
{
    s32 historyEntry;

    playerStateParameter0->pendingMapObject = 0;
    playerStateParameter0->resultHistoryIndex = 0;
    playerStateParameter0->resultSequenceCounter = 0;
    playerStateParameter0->targetSearchTimer = 0.0f;
    for (historyEntry = 0; historyEntry < 0xA; historyEntry++) {
        playerStateParameter0->recentResultHistory[historyEntry] = -1;
    }
}

PlayerGameState *GetPlayerBySlot(s32 indexParameter0)
{
    s32 objectIndex;
    PlayerGameState *player;

    for (objectIndex = 0; objectIndex < 4; objectIndex++) {
        player = playerObjectList[objectIndex]->data;
        if (indexParameter0 == player->playerSlot) {
            return player;
        }
    }
    return NULL;
}

MapObjectDefinition *FindMapObjectByName(s32 indexParameter0, MapObjectDefinition *mapObjectParameter1)
{
    MapObjectDefinition *mapObject;
    s32 mapObjectListIndex;
    s32 mapObjectIndex;

    for (mapObjectListIndex = 0; mapObjectListIndex < 2; mapObjectListIndex++) {
        mapObject = teamResultStates[indexParameter0].mapObjectLists[mapObjectListIndex];
        for (mapObjectIndex = 0; mapObjectIndex < 0x1F; mapObjectIndex++, mapObject++) {
            if (strcmp(mapObjectParameter1->attachmentName, mapObject->attachmentName) == 0) {
                return mapObject;
            }
        }
    }
    return NULL;
}

inline void SetVectorComponents(Vec *positionParameter0, float valueParameter1, float valueParameter2, float valueParameter3)
{
    positionParameter0->x = valueParameter1;
    positionParameter0->y = valueParameter2;
    positionParameter0->z = valueParameter3;
}

void GetMapObjectWorldPosition(MapObjectDefinition *mapObjectParameter0, Vec *positionParameter1)
{
    Mtx rotationMatrix;

    SetVectorComponents(positionParameter1, 0.0f, 0.0f, mapObjectParameter0->objectVariant);
    MTXRotRad(rotationMatrix, 0x79, 0.017453292f * mapObjectParameter0->rotationDegrees);
    MTXMultVec(rotationMatrix, positionParameter1, positionParameter1);
    VECAdd(&mapObjectParameter0->worldPosition, positionParameter1, positionParameter1);
}

void UpdateTeamCharacterAttachments(s32 indexParameter0, OMOBJ *object)
{
    if (teamResultStates[indexParameter0].resultSoundId >= 0) {
        teamResultStates[indexParameter0].resultSoundFade -= 1.0f / REFRESH_RATE;
        if (0.0f >= teamResultStates[indexParameter0].resultSoundFade) {
            HuAudFXStop(teamResultStates[indexParameter0].resultSoundId);
            teamResultStates[indexParameter0].resultSoundId = -1;
        }
        else {
            HuAudFXPitchSet(teamResultStates[indexParameter0].resultSoundId, (s32)(8191.0f * teamResultStates[indexParameter0].resultSoundFade));
            HuAudFXVolSet(teamResultStates[indexParameter0].resultSoundId, (s32)(127.0f * teamResultStates[indexParameter0].resultSoundFade));
        }
    }
    teamResultStates[indexParameter0].characterAttachmentPositions[0][2].x = teamResultStates[indexParameter0].characterRotationA;
    teamResultStates[indexParameter0].characterAttachmentPositions[1][2].x = teamResultStates[indexParameter0].characterRotationA;
    Hu3DModelRotSet(object->mdlId[20], 0.0f, teamResultStates[indexParameter0].characterRotationA, 0.0f);
    GetModelAttachmentPosition(object->mdlId[20], "m432_13-player2A", &teamResultStates[indexParameter0].characterAttachmentPositions[0][1]);
    GetModelAttachmentPosition(object->mdlId[20], "m432_13-player2B", &teamResultStates[indexParameter0].characterAttachmentPositions[1][1]);
    teamResultStates[indexParameter0].characterAttachmentPositions[2][2].x = teamResultStates[indexParameter0].characterRotationB;
    teamResultStates[indexParameter0].characterAttachmentPositions[3][2].x = teamResultStates[indexParameter0].characterRotationB;
    Hu3DModelRotSet(object->mdlId[21], 0.0f, teamResultStates[indexParameter0].characterRotationB, 0.0f);
    GetModelAttachmentPosition(object->mdlId[21], "m432_13-player2C", &teamResultStates[indexParameter0].characterAttachmentPositions[2][1]);
    GetModelAttachmentPosition(object->mdlId[21], "m432_13-player2C", &teamResultStates[indexParameter0].characterAttachmentPositions[3][1]);
}

void UpdateResultMotionTime(s32 indexParameter0, OMOBJ *objectParameter1)
{
    float motionFrame = teamResultStates[indexParameter0].motionTimeRatio * (Hu3DMotionMaxTimeGet(objectParameter1->mdlId[22]) / 1000.0f);
    teamResultStates[indexParameter0].motionTime += 0.02f * (motionFrame - teamResultStates[indexParameter0].motionTime);
    Hu3DMotionTimeSet(objectParameter1->mdlId[22], teamResultStates[indexParameter0].motionTime);
}

void InitializePlayerMapObjects(PlayerGameState *activeEntity, MapObjectDefinition *currentEntity)
{
    s32 previouslySelectedIndex;
    float playerDistanceX;
    float playerDistanceZ;
    float playerDistanceSquared;
    float candidateDistanceSquared;

    s32 candidateObjectIndex;
    s32 candidateCount;
    PlayerGameState *otherPlayer;
    MapObjectDefinition *candidateObject;
    TeamResultState *playerState;
    s32 candidateOffset;
    s32 currentObjectIndex;
    char **candidateObjectNames;
    u8 *candidateObjectIndices;

    currentObjectIndex = currentEntity->objectVariant;
    otherPlayer = GetPlayerBySlot(activeEntity->playerSlot ^ 1);
    playerState = &teamResultStates[activeEntity->playerSlot >> 1];
    previouslySelectedIndex = playerState->selectedObjectIndex;
    candidateCount = 6;
    candidateObjectIndices = firstTeamObjectIndices;
    candidateObjectNames = firstTeamObjectNames;
    if ((currentEntity->objectFlags & 4) != 0) {
        previouslySelectedIndex = playerState->selectedVariantIndex;
        candidateCount = 0xA;
        candidateObjectIndices = secondTeamObjectIndices;
        candidateObjectNames = secondTeamObjectNames;
    }
    candidateObjectIndex = frandmod(candidateCount - 1);
    for (candidateOffset = 0; candidateOffset < (candidateCount - 1); candidateOffset++) {
        candidateObjectIndex = (candidateObjectIndex + 1) % (candidateCount - 1);
        if ((currentObjectIndex != candidateObjectIndex) && (previouslySelectedIndex != candidateObjectIndex) && (candidateObjectIndex != candidateObjectIndices[currentObjectIndex])) {
            currentEntity->attachmentName = candidateObjectNames[candidateObjectIndex];
            candidateObject = FindMapObjectByName(activeEntity->playerSlot >> 1, currentEntity);
            playerDistanceX = otherPlayer->playerState.position.x - candidateObject->worldPosition.x;
            playerDistanceZ = otherPlayer->playerState.position.z - candidateObject->worldPosition.z;
            if (200.0f < sqrtf((playerDistanceX * playerDistanceX) + (playerDistanceZ * playerDistanceZ))) {
                break;
            }
        }
    }
    (void)currentEntity;
    (void)currentEntity;
    (void)currentEntity;
}

void CreatePlayerMapObjects(PlayerGameState *playerStateParameter0, OMOBJ *object, MapObjectDefinition *mapObjectParameter2, MapObjectDefinition *mapObjectParameter3)
{
    Vec particlePositions[2];
    float objectTargetHeight;

    objectTargetHeight = mapObjectParameter2->modelOrigin.y;
    if (25.0f <= mapObjectParameter3->rotationPhase) {
        objectTargetHeight -= 400.0f;
    }
    else {
        objectTargetHeight -= 8.0f * mapObjectParameter3->rotationPhase;
    }
    if ((mapObjectParameter2->modelOrigin.y == objectTargetHeight) && (0.0f > mapObjectParameter2->worldPosition.y) && (-1.0f < mapObjectParameter2->worldPosition.y)) {
        mapObjectParameter2->worldPosition.y = 0.0f;
        HuAudFXPlay(0x699);
    }
    mapObjectParameter2->worldPosition.y += 0.25f * (objectTargetHeight - mapObjectParameter2->worldPosition.y);
    if ((mapObjectParameter2->objectState < 0) && ((mapObjectParameter2->modelOrigin.y - 200.0f) > mapObjectParameter2->worldPosition.y)) {
        SetVectorComponents(&particlePositions[0], mapObjectParameter2->worldPosition.x - 80.0f, mapObjectParameter2->modelOrigin.y, mapObjectParameter2->worldPosition.z);
        SetVectorComponents(&particlePositions[1], 80.0f + mapObjectParameter2->worldPosition.x, mapObjectParameter2->modelOrigin.y, mapObjectParameter2->worldPosition.z);
        CreateParticleEffect(particleManagerId, &particleParameters, 4, 4, 2, particlePositions, 0);
        HuAudFXPlay(0x69A);
    }
    Hu3DModelPosSet(object->mdlId[mapObjectParameter2->modelSlot], mapObjectParameter2->worldPosition.x, mapObjectParameter2->worldPosition.y, mapObjectParameter2->worldPosition.z);
    if ((mapObjectParameter2->modelOrigin.y - 399.0f) > mapObjectParameter2->worldPosition.y) {
        playerStateParameter0->mapObjectIndex++;
        mapObjectParameter2->objectFlags = 0;
    }
}

void SelectPlayerMapObject(PlayerGameState *playerStateParameter0, MapObjectDefinition *mapObjectParameter1)
{
    float playerDistanceX;
    float playerDistanceZ;
    TeamResultState *playerState;
    s32 selectedObjectIndex;
    s32 objectVariantIndex;
    char *selectedObjectName;

    if ((playerStateParameter0->playerMode == 0x7D3) && !(mapObjectParameter1->worldPosition.y <= playerStateParameter0->playerState.position.y)) {
        playerState = &teamResultStates[playerStateParameter0->playerSlot >> 1];
        playerDistanceX = playerStateParameter0->playerState.position.x - mapObjectParameter1->worldPosition.x;
        playerDistanceZ = playerStateParameter0->playerState.position.z - mapObjectParameter1->worldPosition.z;
        if (sqrtf((playerDistanceX * playerDistanceX) + (playerDistanceZ * playerDistanceZ)) < 100.0f) {
            playerStateParameter0->playerState.movementFlags |= 0x18;
            playerStateParameter0->playerMode = 0x7D7;
            playerStateParameter0->scoreSpriteLayout = NULL;
            objectVariantIndex = mapObjectParameter1->objectVariant;
            selectedObjectIndex = playerState->selectedObjectIndex;
            selectedObjectName = firstTeamObjectNames[5];
            if (mapObjectParameter1->objectFlags & 4) {
                selectedObjectIndex = playerState->selectedVariantIndex;
                selectedObjectName = secondTeamObjectNames[9];
            }
            if (objectVariantIndex == selectedObjectIndex) {
                mapObjectParameter1->attachmentName = selectedObjectName;
            }
            else {
                InitializePlayerMapObjects(playerStateParameter0, mapObjectParameter1);
            }
            playerStateParameter0->activeMapObject = FindMapObjectByName(playerStateParameter0->playerSlot >> 1, mapObjectParameter1);
            playerStateParameter0->recentResultHistory[playerStateParameter0->resultHistoryIndex] = mapObjectParameter1->objectIndex - playerStateParameter0->mapObjectIndex;
            playerStateParameter0->resultHistoryIndex = (playerStateParameter0->resultHistoryIndex + 1) % 10;
        }
    }
}

void UpdateTeamResultState(TeamResultState *teamStateParameter0, s32 indexParameter1, PlayerGameState *playerStateParameter2, PlayerGameState *playerStateParameter3)
{
    MapObjectDefinition *targetObject;
    float leftRotationOffset;
    float rightRotationOffset;
    float motionProgress;
    float motionFrame;
    MapObjectDefinition *mapObject;
    PlayerGameState *player;
    OMOBJ *stageObject;
    s32 mapObjectListIndex;
    s32 mapObjectIndex;

    leftRotationOffset = 0.0f;
    rightRotationOffset = 0.0f;
    motionProgress = 0.0f;
    stageObject = teamStateParameter0->stageObject;
    for (mapObjectListIndex = 0; mapObjectListIndex < 2; mapObjectListIndex++) {
        player = GetPlayerBySlot((mapObjectListIndex + (indexParameter1 * 2)));
        for (mapObject = teamStateParameter0->mapObjectLists[mapObjectListIndex], mapObjectIndex = 0; mapObjectIndex < 0x1F; mapObjectIndex++, mapObject++) {
            if (mapObject->objectFlags & 0x600) {
                if (1.0f < mapObject->rotationVelocity) {
                    mapObject->rotationVelocity = 1.0f;
                }
                mapObject->rotationPhase = mapObject->rotationPhase + mapObject->rotationVelocity;
                mapObject->rotationVelocity = mapObject->rotationVelocity - 0.05f;
                if (0.0f > mapObject->rotationVelocity) {
                    mapObject->rotationVelocity = 0.0f;
                }
                if (mapObject->objectFlags & 0x400) {
                    leftRotationOffset += mapObject->rotationPhase;
                }
                if (mapObject->objectFlags & 0x200) {
                    rightRotationOffset += mapObject->rotationPhase;
                }
            }
            if (mapObject->objectFlags & 0x800) {
                motionProgress += mapObject->rotationPhase;
            }
            if (NULL != mapObject->attachmentName) {
                targetObject = FindMapObjectByName(indexParameter1, mapObject);
                switch (mapObject->objectFlags) {
                    case 3:
                        break;
                    case 1:
                        CreatePlayerMapObjects(player, stageObject, mapObject, targetObject);
                        break;
                    case 2:
                    case 4:
                        SelectPlayerMapObject(player, mapObject);
                        break;
                }
            }
        }
    }
    teamStateParameter0->characterRotationA = 80.0f + -leftRotationOffset;
    teamStateParameter0->characterRotationB = 160.0f + -rightRotationOffset;
    UpdateTeamCharacterAttachments(indexParameter1, stageObject);
    teamStateParameter0->motionTimeRatio = motionProgress;
    if (1000.0f <= teamStateParameter0->motionTimeRatio) {
        teamStateParameter0->motionTimeRatio = 1000.0f;
    }
    motionFrame = teamResultStates[indexParameter1].motionTimeRatio * (Hu3DMotionMaxTimeGet(stageObject->mdlId[22]) / 1000.0f);
    teamResultStates[indexParameter1].motionTime += 0.02f * (motionFrame - teamResultStates[indexParameter1].motionTime);
    Hu3DMotionTimeSet(stageObject->mdlId[22], teamResultStates[indexParameter1].motionTime);
    if ((winningTeam < 0) && (1000.0f <= teamStateParameter0->motionTimeRatio)) {
        winningTeam = playerStateParameter2->playerSlot & 2;
        playerStateParameter2->mapObjectIndex += 4;
        playerStateParameter3->mapObjectIndex += 4;
    }
}

void SetPlayerSpawnPosition(TeamResultState *teamStateParameter0, PlayerGameState *playerStateParameter1, Vec *positionParameter2)
{
    float spawnDistanceX;
    float spawnDistanceZ;
    float spawnDistance;
    float nearestSpawnDistance;
    s32 spawnPositionIndex;

    nearestSpawnDistance = 100000.0f;
    for (spawnPositionIndex = 0; spawnPositionIndex < 8; spawnPositionIndex++) {
        spawnDistanceX = playerStateParameter1->playerState.position.x - teamStateParameter0->spawnPositions[spawnPositionIndex].x;
        spawnDistanceZ = playerStateParameter1->playerState.position.z - teamStateParameter0->spawnPositions[spawnPositionIndex].z;
        spawnDistance = sqrtf((spawnDistanceX * spawnDistanceX) + (spawnDistanceZ * spawnDistanceZ));
        if (nearestSpawnDistance > spawnDistance) {
            nearestSpawnDistance = spawnDistance;
            *positionParameter2 = teamStateParameter0->spawnPositions[spawnPositionIndex];
        }
    }
}

void UpdateTeamResults(TeamResultState *teamStateParameter0)
{
    float horizontalDistanceX;
    float horizontalDistanceZ;
    float movementScale;
    float verticalDistance;

    PlayerGameState *hookedPlayer = teamStateParameter0->linkedPlayerObject->data;

    if (teamStateParameter0->hookSequenceState < 0) {
        SetPlayerSpawnPosition(teamStateParameter0, hookedPlayer, &teamStateParameter0->currentHookPosition);
        hookedPlayer->playerState.movementFlags |= 8;
        hookedPlayer->playerMode = 0x7D9;
        teamStateParameter0->currentHookPosition.y = 3000.0f;
        teamStateParameter0->previousHookPosition = teamStateParameter0->currentHookPosition;
        teamStateParameter0->hookSequenceState = 0;
        Hu3DModelRotSet(teamStateParameter0->stageObject->mdlId[25], 0.0f, 0.0f, 0.0f);
        Hu3DModelAttrReset(teamStateParameter0->stageObject->mdlId[25], HU3D_ATTR_DISPOFF);
    }
    switch (teamStateParameter0->hookSequenceState) {
        case 0:
            UpdatePlayerMovement(&hookedPlayer->playerState);
            verticalDistance = hookedPlayer->playerState.position.y - teamStateParameter0->previousHookPosition.y;
            teamStateParameter0->currentHookPosition.y += 0.03f * verticalDistance;
            if (50.0f > ABS(verticalDistance)) {
                teamStateParameter0->hookSequenceState++;
                teamStateParameter0->targetHookPosition = teamStateParameter0->hookModelPosition;
                teamStateParameter0->targetHookPosition.y += 900.0f;
                teamStateParameter0->targetHookPosition.z -= 150.0f;
                hookedPlayer->playerState.position = teamStateParameter0->previousHookPosition;
                hookedPlayer->playerState.rotationY = 0.0f;
                Hu3DMotionShiftSet(teamStateParameter0->linkedPlayerObject->mdlId[0], teamStateParameter0->linkedPlayerObject->mtnId[6], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
            }
            break;
        case 1:
            if (50.0f > (0.0f > (teamStateParameter0->targetHookPosition.y - teamStateParameter0->currentHookPosition.y) ? -(teamStateParameter0->targetHookPosition.y - teamStateParameter0->currentHookPosition.y) : teamStateParameter0->targetHookPosition.y - teamStateParameter0->currentHookPosition.y)) {
                teamStateParameter0->hookSequenceState++;
            }
            break;
        case 2:
            teamStateParameter0->targetHookPosition.y += 0.1f * (((900.0f + teamStateParameter0->hookModelPosition.y) - 200.0f) - teamStateParameter0->targetHookPosition.y);
            if (((900.0f + teamStateParameter0->hookModelPosition.y) - 190.0f) > teamStateParameter0->targetHookPosition.y) {
                teamStateParameter0->hookSequenceState++;
                hookedPlayer->inputAcknowledged = 1;
                hookedPlayer->playerMode = 0x7D3;
                SetPlayerMotion(&hookedPlayer->playerState, 0);
                hookedPlayer->playerState.movementFlags |= 0x210;
                if (NULL != teamStateParameter0->pendingPlayerObject) {
                    teamStateParameter0->hookSequenceState = 4;
                    teamStateParameter0->linkedPlayerObject = teamStateParameter0->pendingPlayerObject;
                    hookedPlayer = teamStateParameter0->linkedPlayerObject->data;
                    SetPlayerSpawnPosition(teamStateParameter0, hookedPlayer, &teamStateParameter0->targetHookPosition);
                    hookedPlayer->playerState.movementFlags |= 8;
                    hookedPlayer->playerMode = 0x7D9;
                }
            }
            break;
        case 3:
            teamStateParameter0->targetHookPosition.y += 10.0f;
            if ((2000.0f + teamStateParameter0->hookModelPosition.y) < teamStateParameter0->targetHookPosition.y) {
                teamStateParameter0->linkedPlayerObject = NULL;
                Hu3DModelAttrSet(teamStateParameter0->stageObject->mdlId[25], HU3D_ATTR_DISPOFF);
                return;
            }
            break;
        case 4:
            UpdatePlayerMovement(&hookedPlayer->playerState);
            horizontalDistanceX = teamStateParameter0->targetHookPosition.x - teamStateParameter0->currentHookPosition.x;
            horizontalDistanceZ = teamStateParameter0->targetHookPosition.z - teamStateParameter0->currentHookPosition.z;
            teamStateParameter0->currentHookPosition.x += 0.05f * horizontalDistanceX;
            teamStateParameter0->currentHookPosition.y += 0.01f * (teamStateParameter0->targetHookPosition.y - teamStateParameter0->currentHookPosition.y);
            teamStateParameter0->currentHookPosition.z += 0.05f * horizontalDistanceZ;
            if (20.0f > sqrtf((horizontalDistanceX * horizontalDistanceX) + (horizontalDistanceZ * horizontalDistanceZ))) {
                teamStateParameter0->previousHookPosition = teamStateParameter0->currentHookPosition;
                teamStateParameter0->hookSequenceState = 0;
            }
            break;
    }
    if ((teamStateParameter0->hookSequenceState > 0) && (teamStateParameter0->hookSequenceState < 4)) {
        if (teamStateParameter0->hookSequenceState < 3) {
            hookedPlayer->playerState.position = teamStateParameter0->previousHookPosition;
            ClearPlayerGroundPositions(&hookedPlayer->playerState);
        }
        verticalDistance = 0.03f * (teamStateParameter0->targetHookPosition.y - teamStateParameter0->currentHookPosition.y);
        teamStateParameter0->currentHookPosition.y += verticalDistance;
        if (teamStateParameter0->hookModelPosition.y < teamStateParameter0->currentHookPosition.y) {
            movementScale = (teamStateParameter0->targetHookPosition.y - teamStateParameter0->currentHookPosition.y) / 900.0f;
            if (0.1f > movementScale) {
                movementScale = 0.1f;
            }
            if (1.0f < movementScale) {
                movementScale = 1.0f;
            }
            movementScale = 1.0f - movementScale;
            teamStateParameter0->currentHookPosition.x += 0.03f * movementScale * (teamStateParameter0->targetHookPosition.x - teamStateParameter0->currentHookPosition.x);
            teamStateParameter0->currentHookPosition.z += 0.03f * movementScale * (teamStateParameter0->targetHookPosition.z - teamStateParameter0->currentHookPosition.z);
        }
    }
    Hu3DModelPosSet(teamStateParameter0->stageObject->mdlId[25], teamStateParameter0->currentHookPosition.x, teamStateParameter0->currentHookPosition.y, teamStateParameter0->currentHookPosition.z);
    GetModelAttachmentPosition(teamStateParameter0->stageObject->mdlId[25], "g008m2_000-itemhook_sao", &teamStateParameter0->previousHookPosition);
}

void UpdatePlayerResultAnimation(TeamResultState *currentEntity, PlayerGameState *otherEntity, PlayerGameState *comparisonEntity)
{
    float cameraDepthAdjustment;
    float resultProgress;

    if ((currentEntity->cameraTransitionDuration == 0.0f) && ((0.0f < currentEntity->cameraTargetHeight) || (0.0f > currentEntity->cameraTargetDepth))) {
        if (0.0f < currentEntity->cameraTargetHeight) {
            currentEntity->cameraLookTarget.y += 0.05f * (currentEntity->cameraTargetHeight - currentEntity->cameraLookTarget.y);
            if (2.0f
                > (0.0f > (currentEntity->cameraTargetHeight - currentEntity->cameraLookTarget.y) ? -(currentEntity->cameraTargetHeight - currentEntity->cameraLookTarget.y) : currentEntity->cameraTargetHeight - currentEntity->cameraLookTarget.y)) {
                currentEntity->cameraLookTarget.y = currentEntity->cameraTargetHeight;
                currentEntity->cameraTargetHeight = 0.0f;
            }
        }
        if (0.0f > currentEntity->cameraTargetDepth) {
            currentEntity->cameraLookTarget.z += 0.05f * (currentEntity->cameraTargetDepth - currentEntity->cameraLookTarget.z);
            if (2.0f
                > (0.0f > (currentEntity->cameraTargetDepth - currentEntity->cameraLookTarget.z) ? -(currentEntity->cameraTargetDepth - currentEntity->cameraLookTarget.z) : currentEntity->cameraTargetDepth - currentEntity->cameraLookTarget.z)) {
                currentEntity->cameraLookTarget.z = currentEntity->cameraTargetDepth;
                currentEntity->cameraTargetDepth = 0.0f;
            }
        }
    }
    else {
        resultProgress = currentEntity->resultMotionTime / currentEntity->resultFramesElapsed;
        if (265.0f > resultProgress) {
            cameraDepthAdjustment = 0.2f * (265.0f - resultProgress);
            if (1.0f <= ABS(cameraDepthAdjustment)) {
                currentEntity->cameraLookTarget.z -= cameraDepthAdjustment;
            }
        }
        switch (currentEntity->unreferencedState18) {
            case 0:
                if ((-7100.0f > currentEntity->cameraLookTarget.z) && ((-7750.0f < currentEntity->playerDepths[0]) || (-7750.0f < currentEntity->playerDepths[1]))) {
                    currentEntity->cameraLookTarget.z = -7100.0f;
                }
                if ((-7750.0f >= currentEntity->playerDepths[0]) && (-7750.0f >= currentEntity->playerDepths[1])) {
                    currentEntity->unreferencedState18++;
                    currentEntity->cameraTargetDepth = -8700.0f;
                    otherEntity->mapObjectIndex += 3;
                    otherEntity->playerDepthCap = -7750.0f;
                    comparisonEntity->mapObjectIndex += 3;
                    comparisonEntity->playerDepthCap = -7750.0f;
                    GetModelAttachmentPosition(currentEntity->stageObject->mdlId[3], "m432_00-n1", &currentEntity->hookModelPosition);
                    return;
                }
                break;
            case 1:
                if ((-9500.0f > currentEntity->cameraLookTarget.z) && ((-10200.0f < currentEntity->playerDepths[0]) || (-10200.0f < currentEntity->playerDepths[1]))) {
                    currentEntity->cameraLookTarget.z = -9500.0f;
                }
                if ((-10200.0f >= currentEntity->playerDepths[0]) && (-10200.0f >= currentEntity->playerDepths[1])) {
                    currentEntity->unreferencedState18++;
                    currentEntity->cameraTargetDepth = -11100.0f;
                    otherEntity->mapObjectIndex += 3;
                    otherEntity->playerDepthCap = -10200.0f;
                    comparisonEntity->mapObjectIndex += 3;
                    comparisonEntity->playerDepthCap = -10200.0f;
                    return;
                }
                break;
            case 2:
                if ((-11500.0f > currentEntity->cameraLookTarget.z) && ((-12300.0f < currentEntity->playerDepths[0]) || (-12300.0f < currentEntity->playerDepths[1]))) {
                    currentEntity->cameraLookTarget.z = -11500.0f;
                }
                if ((NULL != otherEntity->activeMapObject) && (otherEntity->activeMapObject->objectFlags & 8) && (NULL != comparisonEntity->activeMapObject) && ((comparisonEntity->activeMapObject->objectFlags & 8) != 0)
                    && (otherEntity->playerState.actionState == 0) && (comparisonEntity->playerState.actionState == 0)) {
                    currentEntity->unreferencedState18++;
                    currentEntity->objectFloorHeight = otherEntity->activeMapObject->worldPosition.y;
                    currentEntity->cameraTargetDepth = -13500.0f;
                    currentEntity->cameraTargetHeight = otherEntity->activeMapObject->worldPosition.y;
                    otherEntity->mapObjectIndex += 6;
                    comparisonEntity->mapObjectIndex += 6;
                    return;
                }
                break;
            case 3:
                if ((-13700.0f > currentEntity->cameraLookTarget.z) && ((-14400.0f < currentEntity->playerDepths[0]) || (-14400.0f < currentEntity->playerDepths[1]))) {
                    currentEntity->cameraLookTarget.z = -13700.0f;
                }
                if ((NULL != otherEntity->activeMapObject) && (otherEntity->activeMapObject->objectFlags & 0x10) && (NULL != comparisonEntity->activeMapObject) && (comparisonEntity->activeMapObject->objectFlags & 0x10)
                    && (otherEntity->playerState.actionState == 0) && (comparisonEntity->playerState.actionState == 0)) {
                    currentEntity->unreferencedState18++;
                    currentEntity->objectFloorHeight = otherEntity->activeMapObject->worldPosition.y;
                    currentEntity->cameraTargetDepth = -15700.0f;
                    currentEntity->cameraTransitionFrame = 0.0f;
                    currentEntity->cameraTransitionDuration = 120.0f;
                    currentEntity->cameraStartLookTarget = currentEntity->cameraLookTarget;
                    currentEntity->cameraStartPosition = currentEntity->cameraPosition;
                    currentEntity->cameraEndLookTarget.x = currentEntity->cameraLookTarget.x;
                    currentEntity->cameraEndLookTarget.y = 400.0f + otherEntity->activeMapObject->worldPosition.y;
                    currentEntity->cameraEndLookTarget.z = -15700.0f;
                    currentEntity->cameraEndPosition = currentEntity->cameraEndLookTarget;
                    currentEntity->cameraEndPosition.y = 800.0f + otherEntity->activeMapObject->worldPosition.y;
                    currentEntity->cameraEndPosition.z += 3800.0f;
                    otherEntity->mapObjectIndex += 0xA;
                    otherEntity->cameraMoveRate = 0.0f;
                    GetMapObjectWorldPosition(&teamResultStates[otherEntity->playerSlot >> 1].mapObjectLists[otherEntity->playerSlot & 1][0x1B], &otherEntity->movementTarget);
                    comparisonEntity->mapObjectIndex += 0xA;
                    comparisonEntity->cameraMoveRate = 0.0f;
                    GetMapObjectWorldPosition(&teamResultStates[comparisonEntity->playerSlot >> 1].mapObjectLists[comparisonEntity->playerSlot & 1][0x1B], &comparisonEntity->movementTarget);
                }
                break;
        }
    }
}

s32 GetPlayerTeamRank(PlayerGameState *playerStateParameter0)
{
    if (playerStateParameter0->mapObjectIndex >= 0x1F) {
        return -1;
    }
    if (playerStateParameter0->mapObjectIndex >= 0x1B) {
        return 3;
    }
    if (playerStateParameter0->mapObjectIndex >= 0xB) {
        return 2;
    }
    if (playerStateParameter0->mapObjectIndex >= 5) {
        return 1;
    }
    if (playerStateParameter0->mapObjectIndex >= 1) {
        return 0;
    }
    return -1;
}

MapObjectDefinition *GetPlayerMapObject(PlayerGameState *playerStateParameter0, TeamResultState *teamStateParameter1)
{
    MapObjectDefinition *mapObject = teamStateParameter1->mapObjectLists[playerStateParameter0->playerSlot & 1];
    s32 mapObjectIndex;
    for (mapObjectIndex = 0; mapObjectIndex < 0x1F; mapObjectIndex++, mapObject++) {
        if ((playerStateParameter0->mapObjectIndex <= mapObjectIndex) && (mapObject->objectFlags & 0xF00)) {
            return mapObject;
        }
    }
    return NULL;
}

float GetPlayerDistanceFromTeam(PlayerGameState *playerStateParameter0, TeamResultState *teamStateParameter1, Vec *positionParameter2)
{
    float attachmentOffsetX;
    float attachmentOffsetZ;
    float attachmentDistance;
    float nearestAttachmentDistance;
    s32 attachmentIndex;

    nearestAttachmentDistance = 100000.0f;
    for (attachmentIndex = 0; attachmentIndex < 4; attachmentIndex++) {
        attachmentOffsetX = playerStateParameter0->playerState.position.x - teamStateParameter1->characterAttachmentPositions[attachmentIndex][1].x;
        attachmentOffsetZ = playerStateParameter0->playerState.position.z - teamStateParameter1->characterAttachmentPositions[attachmentIndex][1].z;
        attachmentDistance = sqrtf((attachmentOffsetX * attachmentOffsetX) + (attachmentOffsetZ * attachmentOffsetZ));
        if (nearestAttachmentDistance > attachmentDistance) {
            nearestAttachmentDistance = attachmentDistance;
            *positionParameter2 = teamStateParameter1->characterAttachmentPositions[attachmentIndex][1];
        }
    }
    return nearestAttachmentDistance;
}

void SetPlayerTargetPosition(PlayerGameState *playerStateParameter0, s32 indexParameter1, Vec *positionParameter2)
{
    float movementX;
    float movementZ;
    float movementDistance;
    float maximumMovementDistance;

    maximumMovementDistance = 72.0f * playerStateParameter0->aiBehaviorParams->movementDistanceScale;
    movementX = positionParameter2->x - playerStateParameter0->playerState.position.x;
    movementZ = positionParameter2->z - playerStateParameter0->playerState.position.z;
    movementX *= 4.0f;
    movementZ *= 4.0f;
    movementDistance = sqrtf((movementX * movementX) + (movementZ * movementZ));
    if (maximumMovementDistance < movementDistance) {
        movementX = maximumMovementDistance * (movementX / movementDistance);
        movementZ = maximumMovementDistance * (movementZ / movementDistance);
    }
    playerStateParameter0->playerState.stickX = movementX;
    playerStateParameter0->playerState.stickY = -movementZ;
}

void UpdatePlayerMapObject(PlayerGameState *playerStateParameter0, s32 indexParameter1, MapObjectDefinition *mapObjectParameter2)
{
    Vec targetPosition;

    if (NULL != playerStateParameter0) {
        targetPosition = mapObjectParameter2->worldPosition;
        if (mapObjectParameter2->objectFlags & 0xF00) {
            GetMapObjectWorldPosition(mapObjectParameter2, &targetPosition);
        }
        SetPlayerTargetPosition(playerStateParameter0, 0, &targetPosition);
    }
}

MapObjectDefinition *FindPlayerTargetMapObject(PlayerGameState *playerStateParameter0, PlayerGameState *playerStateParameter1, TeamResultState *teamStateParameter2)
{
    s32 candidateIndex;
    MapObjectDefinition *currentMapObject;
    s32 candidateCount;
    s32 selectedObjectIndex;
    s32 minimumCandidateIndex;
    s32 maximumCandidateIndex;
    s32 historyIndex;
    s32 candidateAttempt;
    s32 historyEntry;
    s32 isRecentCandidate;

    currentMapObject = NULL;
    if (playerStateParameter0->mapObjectIndex < 0x1F) {
        currentMapObject = &teamStateParameter2->mapObjectLists[playerStateParameter0->playerSlot & 1][playerStateParameter0->mapObjectIndex];
    }
    if (NULL == currentMapObject) {
        return NULL;
    }
    if (currentMapObject->objectFlags & 2) {
        candidateCount = 5;
        selectedObjectIndex = teamStateParameter2->selectedObjectIndex;
        minimumCandidateIndex = 0;
        maximumCandidateIndex = 5;
    }
    else {
        candidateCount = 9;
        selectedObjectIndex = teamStateParameter2->selectedVariantIndex;
        minimumCandidateIndex = 0;
        maximumCandidateIndex = 9;
        if (-14100.0f < playerStateParameter0->playerState.position.z) {
            minimumCandidateIndex = 3;
            maximumCandidateIndex = 9;
        }
        if (-13700.0f < playerStateParameter0->playerState.position.z) {
            minimumCandidateIndex = 0;
            maximumCandidateIndex = 6;
        }
        if (-13300.0f < playerStateParameter0->playerState.position.z) {
            minimumCandidateIndex = 0;
            maximumCandidateIndex = 3;
        }
    }
    candidateIndex = frandmod(candidateCount);
    for (candidateAttempt = 0; candidateAttempt < candidateCount; candidateAttempt++) {
        candidateIndex = (candidateIndex + 1) % candidateCount;
        if ((minimumCandidateIndex <= candidateIndex) && (maximumCandidateIndex > candidateIndex)
            && ((playerStateParameter0->resultSequenceCounter != 0) || ((selectedObjectIndex != candidateIndex) && (((currentMapObject->objectFlags & 4) == 0) || (candidateIndex <= 3))))) {
            historyIndex = playerStateParameter0->resultHistoryIndex;
            isRecentCandidate = 0;
            for (historyEntry = 0; historyEntry < playerStateParameter0->aiBehaviorParams->recentResultCount; historyEntry++) {
                historyIndex--;
                if (historyIndex < 0) {
                    historyIndex += 0xA;
                }
                if (candidateIndex == playerStateParameter0->recentResultHistory[historyIndex]) {
                    isRecentCandidate = 1;
                }
            }
            if (isRecentCandidate == 0) {
                break;
            }
        }
    }
    if ((candidateAttempt == candidateCount) || (selectedObjectIndex == candidateIndex)) {
        candidateIndex = frandmod(candidateCount);
        while (candidateIndex == selectedObjectIndex) {
            candidateIndex = (candidateIndex + 1) % candidateCount;
        }
    }
    if (playerStateParameter0->aiBehaviorParams->resultSequenceLimit < playerStateParameter0->resultSequenceCounter) {
        candidateIndex = selectedObjectIndex;
    }
    if (NULL != playerStateParameter0->activeMapObject) {
        if (currentMapObject->objectFlags & 2) {
            if (playerStateParameter0->activeMapObject->objectFlags & 8) {
                candidateIndex = selectedObjectIndex;
            }
        }
        else if (playerStateParameter0->activeMapObject->objectFlags & 0x10) {
            candidateIndex = selectedObjectIndex;
        }
    }
    playerStateParameter0->resultSequenceCounter++;
    (void)minimumCandidateIndex;
    (void)maximumCandidateIndex;
    return &currentMapObject[candidateIndex];
}

void UpdatePlayerMapObjectMotion(PlayerGameState *playerStateParameter0, TeamResultState *teamStateParameter1, MapObjectDefinition *mapObjectParameter2, Vec *positionParameter3)
{
    *positionParameter3 = mapObjectParameter2->worldPosition;
    if (playerStateParameter0->playerState.motionFrame >= 0x14) {
        if (-12200.0f < playerStateParameter0->playerState.position.z) {
            positionParameter3->x = teamStateParameter1->cameraLookTarget.x;
        }
        else if ((-13300.0f < playerStateParameter0->playerState.position.z) && (-13400.0f > mapObjectParameter2->worldPosition.z)) {
            positionParameter3->x = teamStateParameter1->cameraLookTarget.x - 500.0f;
        }
        else if ((-13700.0f < playerStateParameter0->playerState.position.z) && (-13800.0f > mapObjectParameter2->worldPosition.z)) {
            positionParameter3->x = 500.0f + teamStateParameter1->cameraLookTarget.x;
        }
        else if ((-13500.0f > playerStateParameter0->playerState.position.z) && (-13400.0f < mapObjectParameter2->worldPosition.z)) {
            positionParameter3->x = 500.0f + teamStateParameter1->cameraLookTarget.x;
            return;
        }
    }
}

void UpdatePlayerInteraction(PlayerGameState *currentEntity, MapObjectDefinition *candidateEntity)
{
    float actionDelay;
    s32 primaryInputFlag;
    s32 secondaryInputFlag;
    s32 isLateralMove;

    primaryInputFlag = 0;
    secondaryInputFlag = 0;
    isLateralMove = 0;
    switch (candidateEntity->objectFlags & 0xF00) {
        case 0x100:
            primaryInputFlag = 0x200;
            secondaryInputFlag = 0x200;
            break;
        case 0x400:
            primaryInputFlag = 0x800;
            secondaryInputFlag = 0x800;
            isLateralMove = 1;
            break;
        case 0x200:
            primaryInputFlag = 0x400;
            secondaryInputFlag = 0x400;
            isLateralMove = 1;
            break;
        case 0x800:
            if ((currentEntity->aiBehaviorParams->fastMotionThreshold >= currentEntity->previousAnimationProgress) && (currentEntity->aiBehaviorParams->slowMotionThreshold > currentEntity->animationProgress)) {
                currentEntity->playerState.triggerR = 0x96;
            }
            else if ((currentEntity->aiBehaviorParams->slowMotionThreshold <= currentEntity->previousAnimationProgress) && (currentEntity->aiBehaviorParams->fastMotionThreshold >= currentEntity->animationProgress)) {
                currentEntity->playerState.triggerR = 0x96;
            }
            else {
                currentEntity->playerState.triggerL = 0x96;
            }
            if (currentEntity->aiBehaviorParams->actionProbability < frandf()) {
                currentEntity->aiMovementScale *= 0.99f;
            }
            if (currentEntity->aiBehaviorParams->playerState > elapsedRoundFrames) {
                currentEntity->aiMovementScale *= 0.99f;
            }
            else {
                currentEntity->aiMovementScale *= 1.01f;
            }
            if (0.5f > currentEntity->aiMovementScale) {
                currentEntity->aiMovementScale = 0.5f;
            }
            if (1.0f < currentEntity->aiMovementScale) {
                currentEntity->aiMovementScale = 1.0f;
            }
            currentEntity->playerState.triggerL = currentEntity->playerState.triggerL * currentEntity->aiMovementScale;
            currentEntity->playerState.triggerR = currentEntity->playerState.triggerR * currentEntity->aiMovementScale;
            break;
    }
    currentEntity->aiActionDelay -= 1.0f;
    if (0.0f > currentEntity->aiActionDelay) {
        actionDelay = currentEntity->aiBehaviorParams->decisionInterval;
        if ((isLateralMove != 0) && (GWPlayerCfg[currentEntity->playerConfigIndex].diff == 3)) {
            actionDelay = normalAiBehaviorParams.decisionInterval;
        }
        currentEntity->aiActionDelay = actionDelay * (0.9f + (frandf() / 2));
        currentEntity->playerState.cameraLookTarget |= primaryInputFlag;
        currentEntity->playerState.buttonHeld |= secondaryInputFlag;
    }
}

void UpdatePlayerBehavior(PlayerGameState *playerStateParameter0)
{
    Vec targetPosition;
    float targetDistance;

    float distanceX;
    float distanceZ;

    MapObjectDefinition *targetMapObject;
    TeamResultState *playerState;
    MapObjectDefinition *candidateMapObject;
    PlayerGameState *otherPlayer;
    s32 shouldUpdateAi;

#ifdef NON_MATCHING
    targetMapObject = NULL;
#endif

    if (NULL != playerStateParameter0->aiBehaviorParams) {
        otherPlayer = GetPlayerBySlot(playerStateParameter0->playerSlot ^ 1);
        playerState = &teamResultStates[playerStateParameter0->playerSlot >> 1];
        shouldUpdateAi = 1;
        switch (GetPlayerTeamRank(playerStateParameter0)) {
            case 0:
                targetMapObject = GetPlayerMapObject(playerStateParameter0, playerState);
                UpdatePlayerMapObject(playerStateParameter0, 0, targetMapObject);
                break;
            case 1:
                candidateMapObject = GetPlayerMapObject(playerStateParameter0, playerState);
                if (NULL != candidateMapObject) {
                    targetMapObject = candidateMapObject;
                    if (((NULL == playerStateParameter0->activeMapObject) && (NULL != otherPlayer->activeMapObject)) || ((candidateMapObject->worldPosition.z - 400.0f) > playerStateParameter0->playerState.position.z)) {
                        targetMapObject++;
                    }
                    else if ((candidateMapObject->worldPosition.z - 1350.0f) > otherPlayer->playerState.position.z) {
                        targetMapObject++;
                        shouldUpdateAi = 0;
                    }
                    if ((1400.0f < (candidateMapObject->worldPosition.z - playerStateParameter0->playerState.position.z)) && (1400.0f < (candidateMapObject->worldPosition.z - otherPlayer->playerState.position.z))) {
                        shouldUpdateAi = 0;
                        playerStateParameter0->playerState.stickY = 0x48;
                    }
                    else {
                        if (((candidateMapObject->worldPosition.z - 175.0f) > playerStateParameter0->playerState.position.z) && ((candidateMapObject->worldPosition.z - 1350.0f) < playerStateParameter0->playerState.position.z)) {
                            if (((playerState->cameraLookTarget.x - 20.0f) < playerStateParameter0->playerState.position.x) && ((20.0f + playerState->cameraLookTarget.x) > playerStateParameter0->playerState.position.x)
                                && ((candidateMapObject->worldPosition.z - 1000.0f) > playerStateParameter0->playerState.position.z)) {
                                playerStateParameter0->playerState.stickY = 0x48;
                            }
                            if ((candidateMapObject->worldPosition.z - 1100.0f) > playerStateParameter0->playerState.position.z) {
                                playerStateParameter0->playerState.cameraLookTarget |= 0x100;
                                playerStateParameter0->playerState.buttonHeld |= 0x100;
                                UpdatePlayerMapObject(playerStateParameter0, 0, targetMapObject);
                            }
                            else {
                                targetDistance = GetPlayerDistanceFromTeam(playerStateParameter0, playerState, &targetPosition);
                                if ((playerStateParameter0->playerState.collisionRadius < targetDistance) && (400.0f > targetDistance)) {
                                    playerStateParameter0->playerState.cameraLookTarget |= 0x100;
                                    playerStateParameter0->playerState.buttonHeld |= 0x100;
                                    SetPlayerTargetPosition(playerStateParameter0, 0, &targetPosition);
                                }
                            }
                        }
                        else {
                            UpdatePlayerMapObject(playerStateParameter0, 0, targetMapObject);
                        }
                        if (((candidateMapObject->worldPosition.z - 175.0f) < playerStateParameter0->playerState.position.z) && (playerStateParameter0->playerState.groundHeight < playerStateParameter0->playerState.position.y)) {
                            playerStateParameter0->playerState.stickX = 0;
                            playerStateParameter0->playerState.stickY = 0;
                        }
                    }
                }
                break;
            case 2:
                shouldUpdateAi = 0;
                playerStateParameter0->targetSearchTimer -= 1.0f;
                if (NULL == playerStateParameter0->pendingMapObject) {
                    playerStateParameter0->pendingMapObject = FindPlayerTargetMapObject(playerStateParameter0, otherPlayer, playerState);
                }
                if (playerStateParameter0->targetSearchTimer == 1.0f) {
                    playerStateParameter0->pendingMapObject = FindPlayerTargetMapObject(playerStateParameter0, otherPlayer, playerState);
                }
                if (NULL != playerStateParameter0->pendingMapObject) {
                    UpdatePlayerMapObjectMotion(playerStateParameter0, playerState, playerStateParameter0->pendingMapObject, &targetPosition);
                    SetPlayerTargetPosition(playerStateParameter0, 0, &targetPosition);
                    if (playerStateParameter0->playerState.movementFlags & 1) {
                        playerStateParameter0->targetSearchTimer = 20.0f + (20.0f * (playerStateParameter0->playerSlot & 1));
                    }
                    distanceX = playerStateParameter0->playerState.position.x - playerStateParameter0->pendingMapObject->worldPosition.x;
                    distanceZ = playerStateParameter0->playerState.position.z - playerStateParameter0->pendingMapObject->worldPosition.z;
                    targetDistance = sqrtf((distanceX * distanceX) + (distanceZ * distanceZ));
                    if ((200.0f > targetDistance) || (playerStateParameter0->playerState.collisionFrame > 0x14) || (playerStateParameter0->playerState.motionFrame > 0xC8)) {
                        playerStateParameter0->playerState.cameraLookTarget |= 0x100;
                        playerStateParameter0->playerState.buttonHeld |= 0x100;
                    }
                    if (50.0f > targetDistance) {
                        playerStateParameter0->playerState.stickX = 0;
                        playerStateParameter0->playerState.stickY = 0;
                    }
                    if (0.0f < playerStateParameter0->targetSearchTimer) {
                        playerStateParameter0->playerState.stickX = 0;
                        playerStateParameter0->playerState.stickY = 0;
                        playerStateParameter0->playerState.cameraLookTarget &= 0xFFFFFEFF;
                        playerStateParameter0->playerState.buttonHeld &= 0xFFFFFEFF;
                    }
                }
                break;
            case 3:
                targetMapObject = GetPlayerMapObject(playerStateParameter0, playerState);
                break;
        }
        if ((shouldUpdateAi != 0) && (NULL != targetMapObject)) {
            UpdatePlayerInteraction(playerStateParameter0, targetMapObject);
        }
    }
}

float GetPlayerScreenPosition(PlayerGameState *currentEntity)
{
    Vec playerWorldPosition;
    Vec screenPosition;

    playerWorldPosition = currentEntity->playerState.position;
    playerWorldPosition.y = teamResultStates[currentEntity->playerSlot >> 1].cameraLookTarget.y;
    Hu3D3Dto2D(&playerWorldPosition, gameCameraIds[currentEntity->playerSlot >> 1], &screenPosition);
    return screenPosition.y;
}

WorkPlayerOld *GetPlayerRuntimeState(int no)
{
    return &((PlayerGameState *)playerObjectList[no]->data)->playerState;
}

void UpdatePlayerDepthLimit(OMOBJ *object)
{
    float depthLimit;
    PlayerGameState *player;
    MapObjectDefinition *mapObject;
    s32 mapObjectIndex;

    player = object->data;
    depthLimit = teamResultStates[player->playerSlot >> 1].cameraLookTarget.z - 750.0f;
    if (depthLimit > player->playerState.position.z) {
        player->playerState.position.z = depthLimit;
    }
    depthLimit = 900.0f + teamResultStates[player->playerSlot >> 1].cameraLookTarget.z;
    if (depthLimit < player->playerState.position.z) {
        player->playerState.position.z = depthLimit;
    }
    if (player->playerDepthCap < player->playerState.position.z) {
        player->playerState.position.z = player->playerDepthCap;
    }
    mapObject = teamResultStates[player->playerSlot >> 1].mapObjectLists[player->playerSlot & 1];
    if ((player->playerSlot & 1) != 0) {
        teamResultStates[player->playerSlot >> 1].playerDepthLimit = 0.0f;
    }
    for (mapObjectIndex = 0; mapObjectIndex < 0x1F; mapObjectIndex++, mapObject++) {
        if ((player->mapObjectIndex <= mapObjectIndex) && ((mapObject->objectFlags & 1) != 0)) {
            depthLimit = 170.0f + mapObject->worldPosition.z;
            if (depthLimit > player->playerState.position.z) {
                player->playerState.position.z = depthLimit;
            }
            if ((player->playerSlot & 1) && (teamResultStates[player->playerSlot >> 1].playerDepthLimit > (mapObject->worldPosition.z - 170.0f))) {
                teamResultStates[player->playerSlot >> 1].playerDepthLimit = mapObject->worldPosition.z - 170.0f;
            }
        }
    }
    if (((player->playerSlot & 1) == 0) && (teamResultStates[player->playerSlot >> 1].cameraLookTarget.x < player->playerState.position.x)
        && (0.0f > teamResultStates[player->playerSlot >> 1].playerDepthLimit) && (teamResultStates[player->playerSlot >> 1].playerDepthLimit < player->playerState.position.z)) {
        player->playerState.position.z = teamResultStates[player->playerSlot >> 1].playerDepthLimit;
    }
}

Vec *GetPlayerPositionCallback(OMOBJ *object)
{
    float attachmentOffsetX;
    float attachmentOffsetZ;
    float attachmentDistance;

    PlayerGameState *player;
    TeamResultState *playerState;
    s32 attachmentIndex;
    PlayerGameState *playerObject;

    playerObject = object->data;
    player = playerObject;
    playerState = &teamResultStates[playerObject->playerSlot >> 1];
    for (attachmentIndex = 0; attachmentIndex < 4; attachmentIndex++) {
        if ((player->playerState.position.y <= playerState->characterAttachmentPositions[attachmentIndex][1].y)
            && ((player->playerState.position.y + (player->playerState.collisionRadius / 2)) >= playerState->characterAttachmentPositions[attachmentIndex][1].y)) {
            attachmentOffsetX = player->playerState.position.x - playerState->characterAttachmentPositions[attachmentIndex][1].x;
            attachmentOffsetZ = player->playerState.position.z - playerState->characterAttachmentPositions[attachmentIndex][1].z;
            attachmentDistance = sqrtf((attachmentOffsetX * attachmentOffsetX) + (attachmentOffsetZ * attachmentOffsetZ));
            if (attachmentDistance < (player->playerState.collisionRadius + playerState->characterAttachmentPositions[attachmentIndex][2].y)) {
                return &playerState->characterAttachmentPositions[attachmentIndex][0];
            }
        }
    }
    return NULL;
}

void UpdatePlayerDisplay(OMOBJ *object)
{
    PlayerGameState *player = object->data;
    player->inputAcknowledged = 0;
    player->playerState.movementFlags &= 0xFFFFFFEF;
    SetPlayerMotion(&player->playerState, 0);
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

void InitializePostGame(void)
{
    PlayerGameState *player;
    s32 objectIndex;
    u32 playerCount;
    u32 playerIndex;

    if (HuPadBtn[0] & 2) {
        elapsedRoundFrames = 300 * REFRESH_RATE;
    }
    print8(0x18, 0x20, 1.5f, "COUNT:%d MODE:%d MES:%d", gameFrameCounter, postGameController->work[0], activeMessageSequence);
    print8(0x18, 0x30, 1.5f, "%f %f %f %f", teamResultStates[0].cameraOrbitAngle, teamResultStates[0].cameraDistance, teamResultStates[0].cameraHeight, teamResultStates[0].cameraLookTarget.z);
    print8(0x18, 0x40, 1.5f, "%f %f %f %f", teamResultStates[1].cameraOrbitAngle, teamResultStates[1].cameraDistance, teamResultStates[1].cameraHeight, teamResultStates[0].cameraLookTarget.z);
    print8(0x18, 0x50, 1.5f, "%x %x", HuMemHeapSizeGet(HEAP_HEAP) - HuMemUsedMallocSizeGet(HEAP_HEAP),
        HuMemHeapSizeGet(HEAP_MODEL) - HuMemUsedMallocSizeGet(HEAP_MODEL));
    for (objectIndex = 0; objectIndex < 4; objectIndex++) {
        player = playerObjectList[objectIndex]->data;
        print8(0x18, (objectIndex * 0x10) + 0x70, 1.5f, "%d %d %04x %.3f (%.3f %.3f %.3f)", player->playerMode, player->playerState.actionState,
            player->playerState.movementFlags, player->playerState.groundHeight, player->playerState.position.x, player->playerState.position.y, player->playerState.position.z);
    }
}

void UpdatePostGameSequence(OMOBJ *object)
{
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    RegisterResourceValue(0x1000, object->mdlId[0]);
    omDelObjEx(gameObjectManager, object);
    floatingEffectFrame--;
}

void UpdatePostGameSprites(OMOBJ *object)
{
    float outgoingScale;
    float incomingScale;
    float outgoingEase;
    float incomingEase;

    FloatingEffectState *effect;

    effect = object->data;
    effect->lifetime += 1.0f;
    if (30.0f <= effect->lifetime) {
        UpdatePostGameSequence(object);
        return;
    }
    if (10.0f > effect->lifetime) {
        outgoingScale = effect->lifetime / 10.0f;
        incomingScale = 0.0f;
    }
    else {
        outgoingScale = 1.0f;
        incomingScale = (effect->lifetime - 10.0f) / 20.0f;
    }
    outgoingEase = sind((90.0f * outgoingScale)) * sind((90.0f * outgoingScale));
    incomingEase = sind((90.0f * incomingScale)) * sind((90.0f * incomingScale));
    omSetTra(object, effect->position.x + (effect->xDisplacement * outgoingEase), effect->position.y, effect->position.z + (effect->zDisplacement * incomingEase));
}

void UpdatePostGameEffects(OMOBJ *object)
{
    FloatingEffectState *effect;
    ResourceValueEntry *resourceEntry;
    ResourceValueEntry *previousEntry;
    ResourceValueEntry *nextEntry;
    s16 modelId;
    s16 resourceIndex;
    s16 resourceId;

    modelId = RemoveResourceValue(0x1000);
    if (modelId < 0) {
        object->mdlId[0] = Hu3DModelLink(stageModelId);
    }
    else {
        object->mdlId[0] = modelId;
    }
    effect = object->data;
    effect->lifetime = 0.0f;
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[0], 3);
    omSetTra(object, effect->position.x, effect->position.y, effect->position.z);
    omSetRot(object, 0.0f, 0.0f, 0.0f);
    omSetSca(object, effect->scale, effect->scale, effect->scale);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    object->objFunc = UpdatePostGameSprites;
}

void SetMapObjectPosition(Vec *positionParameter0, float valueParameter1, float valueParameter2)
{
    FloatingEffectState *effectData;
    OMOBJ *effect;

    if (floatingEffectFrame < 0xF0) {
        floatingEffectFrame++;
        effect = omAddObjEx(gameObjectManager, 0x67, 1, 0, 3, UpdatePostGameEffects);
        effect->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(FloatingEffectState), 0x10000000);
        effectData = effect->data;
        effectData->position = *positionParameter0;
        effectData->position.x += 30.0f * valueParameter2;
        effectData->scale = 1.0f + valueParameter1;
        effectData->xDisplacement = 70.0f * valueParameter2;
        effectData->zDisplacement = -675.0f;
    }
}

void InitializePlayerResult(PlayerGameState *playerStateParameter0)
{
    Vec playerWorldPosition;
    Vec screenPosition;
    float spriteOffsetX;
    float spriteOffsetY;

    s32 *spriteLayoutEntry;
    s32 isFirstSprite;

    playerWorldPosition = playerStateParameter0->playerState.position;
    playerWorldPosition.y += 220.0f;
    CalculateModelAttachment(&playerWorldPosition, gameCameraIds[playerStateParameter0->playerSlot >> 1], &screenPosition);

    playerStateParameter0->scoreSpriteX = screenPosition.x;
    playerStateParameter0->scoreSpriteY = screenPosition.y;
    if (NULL != playerStateParameter0->scoreSpriteLayout) {
        spriteOffsetX = 0.0f;
        spriteOffsetY = 0.0f;
        for (spriteLayoutEntry = playerStateParameter0->scoreSpriteLayout, isFirstSprite = 1; spriteLayoutEntry[0] >= 0; spriteLayoutEntry += 3) {
            espPosSet(playerStateParameter0->scoreSpriteIds[spriteLayoutEntry[0]], spriteLayoutEntry[1] + (playerStateParameter0->scoreSpriteX + spriteOffsetX), spriteLayoutEntry[2] + (playerStateParameter0->scoreSpriteY + spriteOffsetY));
            espDispOn(playerStateParameter0->scoreSpriteIds[spriteLayoutEntry[0]]);
            if (isFirstSprite != 0) {
                isFirstSprite = 0;
                spriteOffsetX = spriteLayoutEntry[1];
                spriteOffsetY = spriteLayoutEntry[2];
            }
        }
    }
}

s32 GetPlayerResultRank(PlayerGameState *playerStateParameter0)
{
    switch (playerStateParameter0->activeMapObject->objectFlags & 0xF00) {
        case 0x100:
            if (playerStateParameter0->playerState.cameraLookTarget & 0x200) {
                return 0x7D4;
            }
            break;
        case 0x400:
            if (playerStateParameter0->playerState.cameraLookTarget & 0x800) {
                return 0x7D5;
            }
            break;
        case 0x200:
            if (playerStateParameter0->playerState.cameraLookTarget & 0x400) {
                return 0x7D5;
            }
            break;
        case 0x800:
            return 0x7D6;
    }
    return 0x7D3;
}

void UpdatePlayerResult(PlayerGameState *playerStateParameter0)
{
    Vec targetPosition;
    float movementX;
    float movementZ;
    float movementDistance;

    PlayerGameState *player;

    player = playerStateParameter0;
    player->playerState.rotationY = InterpolateValue(player->playerState.rotationY, 180.0f + playerStateParameter0->activeMapObject->rotationDegrees, 0.4f);
    GetMapObjectWorldPosition(playerStateParameter0->activeMapObject, &targetPosition);
    movementX = targetPosition.x - player->playerState.position.x;
    movementZ = targetPosition.z - player->playerState.position.z;
    movementDistance = sqrtf((movementX * movementX) + (movementZ * movementZ));
    if (10.0f <= movementDistance) {
        movementX = 10.0f * (movementX / movementDistance);
        movementZ = 10.0f * (movementZ / movementDistance);
    }
    player->playerState.position.x += movementX;
    player->playerState.position.z += movementZ;
}

void InitializePlayerResultModels(PlayerGameState *playerStateParameter0)
{
    Vec targetPosition;
    float candidateDistance;
    float distanceX;
    float distanceZ;
    float candidateDistanceValue;
    MapObjectDefinition *mapObject;
    s32 mapObjectIndex;
    s32 *scoreSpriteLayout;
    PlayerGameState *otherPlayer;

    scoreSpriteLayout = NULL;
    if (playerStateParameter0->playerState.actionState == 0) {
        for (mapObject = teamResultStates[playerStateParameter0->playerSlot >> 1].mapObjectLists[playerStateParameter0->playerSlot & 1], mapObjectIndex = 0; mapObjectIndex < 0x1F; mapObjectIndex++, mapObject++)
            if ((playerStateParameter0->mapObjectIndex <= mapObjectIndex) && (mapObject->objectFlags & 0xF00) && (((mapObject->objectFlags & 0x700) == 0) || !(20.0f < playerStateParameter0->playerState.position.y))) {
                GetMapObjectWorldPosition(mapObject, &targetPosition);
                distanceX = targetPosition.x - playerStateParameter0->playerState.position.x;
                distanceZ = targetPosition.z - playerStateParameter0->playerState.position.z;
                candidateDistanceValue = sqrtf((distanceX * distanceX) + (distanceZ * distanceZ));
                if (playerStateParameter0->playerState.collisionRadius > candidateDistanceValue) {
                    otherPlayer = GetPlayerBySlot(playerStateParameter0->playerSlot ^ 1);
                    if ((NULL == otherPlayer->activeMapObject) || (mapObject->modelSlot != otherPlayer->activeMapObject->modelSlot)) {
                        playerStateParameter0->activeMapObject = mapObject;
                        switch (mapObject->objectFlags & 0xF00) {
                            case 0x100:
                                scoreSpriteLayout = scoreSpriteLayoutA;
                                break;
                            case 0x200:
                                scoreSpriteLayout = scoreSpriteLayoutB;
                                break;
                            case 0x400:
                                scoreSpriteLayout = scoreSpriteLayoutC;
                                break;
                            case 0x800:
                                scoreSpriteLayout = scoreSpriteLayoutD;
                                break;
                        }
                        break;
                    }
                }
            }
    }

    if (playerStateParameter0->scoreSpriteLayout != scoreSpriteLayout) {
        playerStateParameter0->scoreSpriteLayout = scoreSpriteLayout;
        if (NULL == scoreSpriteLayout) {
            playerStateParameter0->activeMapObject = NULL;
        }
        for (mapObjectIndex = 0; mapObjectIndex < 9; mapObjectIndex++) {
            espDispOff(playerStateParameter0->scoreSpriteIds[mapObjectIndex]);
        }
    }
}

void UpdatePlayerResultModels(PlayerGameState *currentEntity, OMOBJ *firstObject, OMOBJ *secondObject)
{
    Vec targetPosition;
    float motionFrame;

    TeamResultState *playerState;
    PlayerGameState *otherPlayer;
    s32 modelIndex;
    void *playerModelData;
    s32 objectIndex;
    float animationProgress;

    playerState = &teamResultStates[currentEntity->playerSlot >> 1];
    UpdatePlayerResult(currentEntity);
    if (GetPlayerResultRank(currentEntity) == 0x7D4) {
        playerState->impactSoundFade += 1.0f - (2.0f * (currentEntity->collisionTimer / REFRESH_RATE));
        if (0.0f > playerState->impactSoundFade) {
            playerState->impactSoundFade = 0.0f;
        }
        if (1.0f < playerState->impactSoundFade) {
            playerState->impactSoundFade = 1.0f;
        }
        currentEntity->collisionTimer = 0.0f;
        if (25.0f <= (currentEntity->activeMapObject->rotationPhase += 1.0f)) {
            currentEntity->activeMapObject->rotationPhase = 25.0f;
        }
        omVibrate(currentEntity->playerConfigIndex, 0xC, 6, 6);
    }
    else {
        currentEntity->collisionTimer += 1.0f;
        playerState->impactSoundFade -= 1.0f / REFRESH_RATE;
        if (0.0f > playerState->impactSoundFade) {
            playerState->impactSoundFade = 0.0f;
        }
        if (20.0f <= currentEntity->collisionTimer) {
            currentEntity->collisionTimer = 0.0f;
            if (0.0f >= (currentEntity->activeMapObject->rotationPhase -= 1.0f)) {
                currentEntity->activeMapObject->rotationPhase = 0.0f;
            }
        }
    }
    HuAudFXPitchSet(playerState->impactSoundId, (s32)(8191.0f * playerState->impactSoundFade));
    HuAudFXPitchSet(playerState->followupSoundId, (s32)(8191.0f * (1.0f - (currentEntity->activeMapObject->rotationPhase / 25.0f))));
    if (Hu3DMotionShiftIDGet(firstObject->mdlId[0]) < 0) {
        motionFrame = currentEntity->activeMapObject->rotationPhase * (Hu3DMotionMaxTimeGet(firstObject->mdlId[0]) / 25.0f);
        Hu3DMotionTimeSet(firstObject->mdlId[0], motionFrame);
        motionFrame = currentEntity->activeMapObject->rotationPhase * (Hu3DMotionMaxTimeGet((secondObject->mdlId[currentEntity->activeMapObject->modelSlot])) / 25.0f);
        Hu3DMotionTimeSet(secondObject->mdlId[currentEntity->activeMapObject->modelSlot], motionFrame);
    }
    if ((25.0f <= currentEntity->activeMapObject->rotationPhase) || (currentEntity->activeMapObject->rotationPhase == 0.0f)) {
        if (25.0f <= currentEntity->activeMapObject->rotationPhase) {
            currentEntity->mapObjectIndex++;
        }
        currentEntity->playerMode = 0x7D3;
        SetPlayerMotion(&currentEntity->playerState, 0);
        HuAudFXStop(playerState->impactSoundId);
        HuAudFXStop(playerState->followupSoundId);
        playerState->impactSoundId = -1;
        playerState->followupSoundId = -1;
    }
}

void UpdatePlayerCharacter(PlayerGameState *playerStateParameter0, OMOBJ *objectParameter1, OMOBJ *objectParameter2)
{
    UpdatePlayerResult(playerStateParameter0);
    if (GetPlayerResultRank(playerStateParameter0) == 0x7D5) {
        teamResultStates[playerStateParameter0->playerSlot >> 1].resultSoundFade += 1.0f - (2.0f * (playerStateParameter0->collisionTimer / REFRESH_RATE));
        if (0.0f > teamResultStates[playerStateParameter0->playerSlot >> 1].resultSoundFade) {
            teamResultStates[playerStateParameter0->playerSlot >> 1].resultSoundFade = 0.0f;
        }
        if (1.0f < teamResultStates[playerStateParameter0->playerSlot >> 1].resultSoundFade) {
            teamResultStates[playerStateParameter0->playerSlot >> 1].resultSoundFade = 1.0f;
        }
        playerStateParameter0->collisionTimer = 0.0f;
        playerStateParameter0->activeMapObject->rotationVelocity += 3.0f;
        omVibrate(playerStateParameter0->playerConfigIndex, 0xC, 6, 6);
    }
    else {
        playerStateParameter0->collisionTimer += 1.0f;
    }
    if (Hu3DMotionShiftIDGet(objectParameter1->mdlId[0]) < 0) {
        Hu3DMotionTimeSet(objectParameter1->mdlId[0], 120.0 * (fmod(playerStateParameter0->activeMapObject->rotationPhase, 360.0) / 360.0));
        Hu3DMotionTimeSet(
            objectParameter2->mdlId[playerStateParameter0->activeMapObject->modelSlot], Hu3DMotionMaxTimeGet(objectParameter2->mdlId[playerStateParameter0->activeMapObject->modelSlot]) * (fmod(playerStateParameter0->activeMapObject->rotationPhase, 360.0) / 360.0));
    }
    if (20.0f <= playerStateParameter0->collisionTimer) {
        playerStateParameter0->playerMode = 0x7D3;
        SetPlayerMotion(&playerStateParameter0->playerState, 0);
    }
    else if (teamResultStates[playerStateParameter0->playerSlot >> 1].resultSoundId < 0) {
        teamResultStates[playerStateParameter0->playerSlot >> 1].resultSoundFade = 0.5f;
        teamResultStates[playerStateParameter0->playerSlot >> 1].resultSoundId = HuAudFXPlay((playerStateParameter0->playerSlot & 2) == 0 ? 0x68E : 0x68F);
    }
}

void UpdatePlayerCharacterMotion(PlayerGameState *playerStateParameter0, OMOBJ *objectParameter1, OMOBJ *objectParameter2)
{
    float animationDelta;
    float movementDirection;
    float bounceStrength;

    UpdatePlayerResult(playerStateParameter0);
    if ((playerStateParameter0->playerState.triggerL <= 0) || (playerStateParameter0->playerState.triggerR <= 0)) {
        movementDirection = 0.0f;
        if (playerStateParameter0->playerState.triggerL > 0) {
            movementDirection = -(playerStateParameter0->playerState.triggerL / 150.0f);
        }
        if (playerStateParameter0->playerState.triggerR > 0) {
            movementDirection = playerStateParameter0->playerState.triggerR / 150.0f;
        }
        if (movementDirection != 0.0f) {
            if ((0.0f > movementDirection ? -1 : 1) != (0.0f > playerStateParameter0->animationDirection ? -1 : 1)) {
                animationDelta = playerStateParameter0->animationProgress - playerStateParameter0->previousAnimationProgress;
                if (0.0f < animationDelta) {
                    if (0.98f < animationDelta) {
                        animationDelta += 0.6f;
                        if ((playerStateParameter0->playerSlot & 2) == 0) {
                            HuAudFXPlay(0x692);
                        }
                        else {
                            HuAudFXPlay(0x693);
                        }
                    }
                    playerStateParameter0->activeMapObject->rotationPhase += 10.0f * animationDelta;
                    bounceStrength = animationDelta;
                    if (0.5f > bounceStrength) {
                        bounceStrength = 0.0f;
                    }
                    SetMapObjectPosition(&playerStateParameter0->activeMapObject->worldPosition, bounceStrength / 2, (playerStateParameter0->playerSlot & 1) ? -1.0f : 1.0f);
                }
                playerStateParameter0->previousAnimationProgress = playerStateParameter0->animationProgress;
            }
            playerStateParameter0->animationDirection = movementDirection;
            playerStateParameter0->animationProgress = playerStateParameter0->animationProgress + (0.1f * playerStateParameter0->animationDirection);
            if (0.0f > playerStateParameter0->animationProgress) {
                playerStateParameter0->animationProgress = 0.0f;
            }
            if (1.0f < playerStateParameter0->animationProgress) {
                playerStateParameter0->animationProgress = 1.0f;
            }
        }
    }
    if (Hu3DMotionShiftIDGet(objectParameter1->mdlId[0]) < 0) {
        Hu3DMotionTimeSet(objectParameter1->mdlId[0], playerStateParameter0->animationProgress * Hu3DMotionMaxTimeGet(objectParameter1->mdlId[0]));
        Hu3DMotionTimeSet(objectParameter2->mdlId[playerStateParameter0->activeMapObject->modelSlot], playerStateParameter0->animationProgress * Hu3DMotionMaxTimeGet(objectParameter2->mdlId[playerStateParameter0->activeMapObject->modelSlot]));
    }
    if (1000.0f <= teamResultStates[playerStateParameter0->playerSlot >> 1].motionTimeRatio) {
        playerStateParameter0->playerMode = 0x7D3;
        SetPlayerMotion(&playerStateParameter0->playerState, 0);
    }
}

void UpdatePlayerStatus(PlayerGameState *playerStateParameter0, OMOBJ *objectParameter1)
{
    float cameraDepthDifference;
    float cameraDepthLimit;
    s32 horizontalDirection;

    if (playerStateParameter0->playerState.actionState != 0) {
        UpdatePlayerMovement(&playerStateParameter0->playerState);
        return;
    }
    if (playerStateParameter0->activeMapObject->objectFlags & 0x18) {
        if (((playerStateParameter0->activeMapObject->objectFlags & 8) && (playerStateParameter0->impactCount <= 0)) || ((playerStateParameter0->activeMapObject->objectFlags & 0x10) && (playerStateParameter0->impactCount <= 1))) {
            playerStateParameter0->impactCount++;
            omVibrate(playerStateParameter0->playerConfigIndex, 0xC, 4, 2);
            if ((playerStateParameter0->playerSlot & 2) == 0) {
                HuAudFXPlay(0x690);
            }
            else {
                HuAudFXPlay(0x691);
            }
        }
        if ((playerStateParameter0->playerSlot & 1) != 0) {
            horizontalDirection = 1;
        }
        else {
            horizontalDirection = -1;
        }
        playerStateParameter0->queuedStickX = horizontalDirection * 0x18;
        playerStateParameter0->queuedStickY = 0x48;
        playerStateParameter0->playerState.rotationY = 180.0f;
        if ((((playerStateParameter0->activeMapObject->objectFlags & 8) == 0) || !(-11100.0f < teamResultStates[playerStateParameter0->playerSlot >> 1].cameraLookTarget.z))
            && (((playerStateParameter0->activeMapObject->objectFlags & 0x10) == 0) || !(-13500.0f < teamResultStates[playerStateParameter0->playerSlot >> 1].cameraLookTarget.z))) {
            cameraDepthLimit = 800.0f - (400.0f * (playerStateParameter0->playerSlot & 1));
            if (0.0f > (teamResultStates[playerStateParameter0->playerSlot >> 1].cameraTargetDepth - teamResultStates[playerStateParameter0->playerSlot >> 1].cameraLookTarget.z)) {
                cameraDepthDifference = -(teamResultStates[playerStateParameter0->playerSlot >> 1].cameraTargetDepth - teamResultStates[playerStateParameter0->playerSlot >> 1].cameraLookTarget.z);
            }
            else {
                cameraDepthDifference = teamResultStates[playerStateParameter0->playerSlot >> 1].cameraTargetDepth - teamResultStates[playerStateParameter0->playerSlot >> 1].cameraLookTarget.z;
            }
            if (!(cameraDepthLimit < cameraDepthDifference)) {
                InitializePlayerGameState(playerStateParameter0);
                CharFXPlay(playerStateParameter0->characterIndex, 0x122);
                goto block26;
            }
        }
    }
    else {
        playerStateParameter0->queuedStickX = 0;
        playerStateParameter0->queuedStickY = -0x48;
        playerStateParameter0->playerState.rotationY = 0.0f;
    block26:
        playerStateParameter0->pendingMapObject = 0;
        playerStateParameter0->recentResultHistory[playerStateParameter0->resultHistoryIndex] = playerStateParameter0->activeMapObject->objectIndex - playerStateParameter0->mapObjectIndex;
        playerStateParameter0->resultHistoryIndex = (playerStateParameter0->resultHistoryIndex + 1) % 10;
        playerStateParameter0->playerMode = 0x7D8;
        playerStateParameter0->playerState.position = playerStateParameter0->activeMapObject->worldPosition;
        playerStateParameter0->playerState.position.y -= 200.0f;
        SetPlayerMotion(&playerStateParameter0->playerState, 3);
        playerStateParameter0->playerState.movementFlags |= 8;
    }
}

void InitializePlayerScore(PlayerGameState *playerStateParameter0)
{
    float targetOffsetX;
    float targetOffsetZ;
    float targetHeightOffset;

    playerStateParameter0->playerState.stickX = playerStateParameter0->queuedStickX;
    playerStateParameter0->playerState.stickY = playerStateParameter0->queuedStickY;
    playerStateParameter0->playerState.cameraLookTarget = 0x100;
    playerStateParameter0->playerState.buttonHeld = 0x100;
    UpdatePlayerMovement(&playerStateParameter0->playerState);
    targetOffsetX = playerStateParameter0->activeMapObject->worldPosition.x - playerStateParameter0->playerState.position.x;
    targetOffsetZ = playerStateParameter0->activeMapObject->worldPosition.z - playerStateParameter0->playerState.position.z;
    if (sqrtf((targetOffsetX * targetOffsetX) + (targetOffsetZ * targetOffsetZ)) < 100.0f) {
        targetHeightOffset = 0.0f;
        if (playerStateParameter0->queuedStickY > 0) {
            targetHeightOffset = 100.0f;
        }
        if ((playerStateParameter0->activeMapObject->worldPosition.y + targetHeightOffset) > playerStateParameter0->playerState.position.y) {
            playerStateParameter0->playerState.velocity.y = (20.0f*(80.3f/REFRESH_RATE));
            playerStateParameter0->playerState.jumpTimer = 1.0f;
            return;
        }
        playerStateParameter0->playerState.movementFlags &= 0xFFFFFFF7;
        return;
    }
    if ((0.0f > playerStateParameter0->playerState.velocity.y) && ((100.0f + playerStateParameter0->activeMapObject->worldPosition.y) > playerStateParameter0->playerState.position.y)) {
        playerStateParameter0->playerMode = 0x7D3;
        CharFXPlay(playerStateParameter0->characterIndex, 0x10D);
    }
}

void UpdatePlayerScore(PlayerGameState *playerStateParameter0, OMOBJ *objectParameter1)
{
    if ((teamResultStates[playerStateParameter0->playerSlot >> 1].objectFloorHeight - (playerStateParameter0->playerState.collisionRadius / 2)) > playerStateParameter0->playerState.position.y) {
        Hu3DModelShadowDispOff(objectParameter1->mdlId[0]);
        return;
    }
    Hu3DModelShadowDispOn(objectParameter1->mdlId[0]);
}

void UpdatePlayerObject(OMOBJ *object)
{
    float playerHeight;
    float playerDepth;
    float resultScale;
    TeamResultState *playerState;
    PlayerGameState *player;
    s32 isPlayerReady;
    s32 resultMode;

    player = object->data;
    playerState = &teamResultStates[player->playerSlot >> 1];
    if (postGameController->work[0] >= 0x3EC) {
        if (winningTeam != (player->playerSlot & 2)) {
            isPlayerReady = 1;
            if (player->playerState.groundHeight < player->playerState.position.y) {
                isPlayerReady = 0;
            }
            if (player->playerMode == 0x7D7) {
                isPlayerReady = 0;
            }
            if (player->playerMode == 0x7D8) {
                isPlayerReady = 0;
            }
            if (player->playerMode == 0x7D9) {
                isPlayerReady = 0;
            }
            if ((player->playerMode == 0x7D3) && (player->playerState.actionState == 1)) {
                isPlayerReady = 0;
            }
            if (isPlayerReady != 0) {
                player->playerMode = 0x7DA;
                if (winningTeam >= 0) {
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[10], 0.0f, 8.0f, HU3D_ATTR_NONE);
                }
                else {
                    player->completionPending = 1;
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                }
            }
        }
        else {
            player->playerMode = 0x7DB;
            player->movementTarget.x += 100.0f * (player->playerSlot & 1 ? -1.0f : 1.0f);
            player->movementTarget.z += -475.0f;
            Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
            Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[2], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
        }
        if (player->playerMode >= 0x7DA) {
            for (resultMode = 0; resultMode < 9; resultMode++) {
                espDispOff(player->scoreSpriteIds[resultMode]);
            }
            if (winningTeam == (player->playerSlot & 2)) {
                GWPlayerCoinWinSet(player->playerConfigIndex, 10);
            }
            object->objFunc = InitializePlayerObject;
            return;
        }
    }
    if (GWPlayerCfg[player->playerConfigIndex].iscom != 0) {
        player->playerState.stickX = 0;
        player->playerState.stickY = 0;
        player->playerState.cameraLookTarget = 0;
        player->playerState.buttonHeld = 0;
        player->playerState.triggerL = 0;
        player->playerState.triggerR = 0;
        UpdatePlayerBehavior(player);
    }
    else {
        player->playerState.stickX = HuPadStkX[player->controllerPadIndex];
        player->playerState.stickY = HuPadStkY[player->controllerPadIndex];
        player->playerState.cameraLookTarget = HuPadBtnDown[player->controllerPadIndex];
        player->playerState.buttonHeld = HuPadBtn[player->controllerPadIndex];
        player->playerState.triggerL = HuPadTrigL[player->controllerPadIndex];
        player->playerState.triggerR = HuPadTrigR[player->controllerPadIndex];
    }
    if ((player->playerMode == 0x7D3) && ((player->movementTarget.x != 0.0f) || (player->movementTarget.z != 0.0f))) {
        playerDepth = player->movementTarget.x - player->playerState.position.x;
        playerHeight = player->movementTarget.z - player->playerState.position.z;
        if (player->playerState.collisionRadius < ABS(playerHeight)) {
            playerDepth = 0.0f;
        }
        resultScale = sqrtf((playerDepth * playerDepth) + (playerHeight * playerHeight));
        if (10.0f <= resultScale) {
            playerDepth /= resultScale;
            playerHeight /= resultScale;
        }
        else {
            playerDepth = 0.0f;
            playerHeight = 0.0f;
            player->playerState.rotationY = player->cameraMoveRate;
        }
        player->playerState.stickX = 72.0f * playerDepth;
        player->playerState.stickY = 72.0f * -playerHeight;
        player->playerState.cameraLookTarget = 0;
        player->playerState.buttonHeld = 0;
        player->playerState.triggerL = 0;
        player->playerState.triggerR = 0;
    }
    switch (player->playerMode) {
        case 0x7D3:
            if ((player->inputAcknowledged != 0) && ((player->playerState.movementFlags & 0x101) != 0)) {
                player->inputAcknowledged = 0;
                player->playerState.movementFlags &= 0xFFFFFFEF;
                SetPlayerMotion(&player->playerState, 0);
            }
            UpdatePlayerMovement(&player->playerState);
            InitializePlayerResultModels(player);
            if ((NULL != player->scoreSpriteLayout) && (NULL != player->activeMapObject)) {
                player->playerMode = GetPlayerResultRank(player);
                switch (player->playerMode) {
                    case 0x7D4:
                        player->collisionTimer = 0.0f;
                        player->activeMapObject->rotationPhase = 1.0f;
                        playerState->impactSoundFade = 0.5f;
                        if (playerState->impactSoundId < 0) {
                            playerState->impactSoundId = HuAudFXPlay((player->playerSlot & 2) == 0 ? 0x68C : 0x68D);
                        }
                        if (playerState->followupSoundId < 0) {
                            playerState->followupSoundId = HuAudFXPlay((player->playerSlot & 2) == 0 ? 0x695 : 0x696);
                        }
                        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[5], 0.0f, 8.0f, HU3D_MOTATTR_PAUSE);
                        break;
                    case 0x7D5:
                        player->collisionTimer = 0.0f;
                        player->activeMapObject->rotationVelocity += 3.0f;
                        if (playerState->resultSoundId < 0) {
                            playerState->resultSoundFade = 0.5f;
                            playerState->resultSoundId = HuAudFXPlay((player->playerSlot & 2) == 0 ? 0x68E : 0x68F);
                        }
                        Hu3DMotionShiftSet(
                            object->mdlId[0], object->mtnId[7], 120.0 * (fmod(player->activeMapObject->rotationPhase, 360.0) / 360.0), 8.0f, HU3D_MOTATTR_PAUSE);
                        break;
                    case 0x7D6:
                        player->collisionTimer = 0.0f;
                        player->animationDirection = -1.0f;
                        player->animationProgress = 0.0f;
                        player->previousAnimationProgress = 0.0f;
                        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[8], 0.0f, 8.0f, HU3D_MOTATTR_PAUSE);
                        break;
                }
            }
            else if (-800.0f > player->playerState.position.y) {
                player->playerState.movementFlags |= 0x10;
                if (NULL == teamResultStates[player->playerSlot >> 1].linkedPlayerObject) {
                    teamResultStates[player->playerSlot >> 1].linkedPlayerObject = object;
                    teamResultStates[player->playerSlot >> 1].hookSequenceState = -1;
                }
                else {
                    teamResultStates[player->playerSlot >> 1].pendingPlayerObject = object;
                }
            }
            break;
        case 0x7D4:
            UpdatePlayerResultModels(player, object, teamResultStates[player->playerSlot >> 1].stageObject);
            break;
        case 0x7D5:
            UpdatePlayerCharacter(player, object, teamResultStates[player->playerSlot >> 1].stageObject);
            break;
        case 0x7D6:
            UpdatePlayerCharacterMotion(player, object, teamResultStates[player->playerSlot >> 1].stageObject);
            break;
        case 0x7D7:
            UpdatePlayerStatus(player, object);
            break;
        case 0x7D8:
            InitializePlayerScore(player);
            break;
        case 0x7D9:
            break;
        default:
            OSReport("*** player mode error(%d)!!\n", player->playerMode);
            break;
    }
    UpdatePlayerScore(player, object);
    InitializePlayerResult(player);

    teamResultStates[player->playerSlot >> 1].resultFramesElapsed += 1.0f;

    teamResultStates[player->playerSlot >> 1].resultMotionTime += GetPlayerScreenPosition(player);
    teamResultStates[player->playerSlot >> 1].playerDepths[player->playerSlot & 1] = player->playerState.position.z;
    ClearPlayerGroundPositions(&player->playerState);
}

void InitializePlayerObject(OMOBJ *object)
{
    float playerDepth;
    float playerHeight;
    float movementSpeed;
    PlayerGameState *player;
    OMOBJ *stageObject;

    player = object->data;
    stageObject = teamResultStates[player->playerSlot >> 1].stageObject;
    switch (player->playerMode) {
        case 0x7D0:
            if (postGameController->work[0] >= 0x3E9) {
                player->playerMode++;
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
                CharFXPlay(player->characterIndex, 0x125);
            }
            break;
        case 0x7D1:
            player->playerState.stickX = 0;
            player->playerState.stickY = 0x48;
            player->playerState.cameraLookTarget = 0x100;
            player->playerState.buttonHeld = 0x100;
            UpdatePlayerMovement(&player->playerState);
            if (200.0f > player->playerState.position.y) {
                player->playerState.velocity.y = (20.0f*(80.3f/REFRESH_RATE));
                player->playerState.jumpTimer = 1.0f;
            }
            else {
                player->playerMode++;
            }
            break;
        case 0x7D2:
            player->playerState.stickX = 0;
            player->playerState.stickY = 0;
            player->playerState.cameraLookTarget = 0;
            player->playerState.buttonHeld = 0;
            if (player->playerState.actionState == 1) {
                player->playerState.stickY = 0x48;
            }
            UpdatePlayerMovement(&player->playerState);
            if ((player->playerState.actionState == 0) && (postGameController->work[0] >= 0x3EB)) {
                player->playerMode++;
                object->objFunc = UpdatePlayerObject;
            }
            break;
        case 0x7DA:
            if ((player->completionPending != 0) && (winningTeam < 0) && (postGameController->work[0] >= 0x3F1)) {
                player->completionPending = 0;
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[10], 0.0f, 8.0f, HU3D_ATTR_NONE);
            }
            break;
        case 0x7DB:
            playerDepth = player->movementTarget.x - player->playerState.position.x;
            playerHeight = player->movementTarget.z - player->playerState.position.z;
            if (1.0f > ABS(playerDepth)) {
                if (1.0f > ABS(playerHeight)) {
                    player->playerMode++;
                    player->bouncePhase = 0.0f;
                    player->bounceStartHeight = player->playerState.position.y;
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[3], 0.0f, 8.0f, HU3D_ATTR_NONE);
                    break;
                }
            }
            if (1.0f < ABS(playerDepth)) {
                playerHeight = 0.0f;
            }
            movementSpeed = sqrtf((playerDepth * playerDepth) + (playerHeight * playerHeight));
            if (10.0f < movementSpeed) {
                playerDepth = 10.0f * (playerDepth / movementSpeed);
                playerHeight = 10.0f * (playerHeight / movementSpeed);
            }
            player->playerState.position.x += playerDepth;
            player->playerState.position.z += playerHeight;

            player->playerState.rotationY = InterpolateValue(player->playerState.rotationY, atan2d(playerDepth, playerHeight), 0.4f);
            break;
        case 0x7DC:
            player->bouncePhase += 5.0f;
            player->playerState.position.z -= 10.0f;
            player->playerState.position.y = teamResultStates[player->playerSlot >> 1].sceneObjectPosition.y + (120.0 * sind(player->bouncePhase));
            if (180.0f <= player->bouncePhase) {
                player->playerMode++;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[4], 0.0f, 8.0f, HU3D_ATTR_NONE);
            }
            break;
        case 0x7DD:
            if ((Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) && (Hu3DMotionEndCheck(object->mdlId[0]) != 0)) {
                player->playerMode++;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(stageObject->mdlId[9], HU3D_MOTATTR_PAUSE);
                Hu3DModelAttrReset(stageObject->mdlId[15], HU3D_MOTATTR_PAUSE);
                Hu3DModelAttrReset(stageObject->mdlId[23], HU3D_MOTATTR_PAUSE);
                HuAudFXPlay(0x694);
            }
            break;
        case 0x7DE:
            player->playerState.rotationY = InterpolateValue(player->playerState.rotationY, 0.0f, 0.4f);
            if ((Hu3DMotionEndCheck(stageObject->mdlId[9]) != 0) && (Hu3DMotionEndCheck(stageObject->mdlId[15]) != 0)) {
                player->playerMode++;
            }
            break;
        case 0x7DF:
            player->playerState.position.y = teamResultStates[player->playerSlot >> 1].sceneObjectPosition.y;
            if (postGameController->work[0] >= 0x3F1) {
                player->playerMode++;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[9], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                CharFXPlay(player->characterIndex, 0x125);
            }
            break;
        case 0x7E0:
            player->playerState.position.y = teamResultStates[player->playerSlot >> 1].sceneObjectPosition.y;
            break;
        case 0x7D3:
        case 0x7D4:
        case 0x7D5:
        case 0x7D6:
        case 0x7D7:
        case 0x7D8:
        case 0x7D9:
        default:
            OSReport("*** player mode error(%d)!!\n", player->playerMode);
            break;
    }
    UpdatePlayerScore(player, object);
    ClearPlayerGroundPositions(&player->playerState);
}

void UpdatePlayerObject(OMOBJ *object)
{
    Vec playerSpawnPosition;

    PlayerGameState *player;
    s32 modelOrSpriteIndex;
    s32 motionIndex;
    s32 spriteIndex;
    MapObjectDefinition *playerMapObjects;
    s32 playerConfigIndex;
    s32 characterIndex;

    player = object->data;
    playerMapObjects = teamResultStates[player->playerSlot >> 1].mapObjectLists[player->playerSlot & 1];
    player->characterIndex = GWPlayerCfg[player->playerConfigIndex].character;
    player->controllerPadIndex = GWPlayerCfg[player->playerConfigIndex].pad_idx;
    player->playerMode = 0x7D0;
    player->completionPending = 0;
    player->aiBehaviorParams = NULL;
    player->scoreSpriteLayout = NULL;
    player->activeMapObject = NULL;
    player->animationDirection = 0.0f;
    player->animationProgress = 0.0f;
    player->previousAnimationProgress = 0.0f;
    player->mapObjectIndex = 0;
    player->playerDepthCap = 10000.0f;
    player->impactCount = 0;
    player->inputAcknowledged = 0;
    player->aiMovementScale = 0.5f;
    player->aiActionDelay = 0.0f;
    InitializePlayerGameState(player);
    playerSpawnPosition = playerMapObjects[player->mapObjectIndex++].worldPosition;
    playerSpawnPosition.y -= 200.0f;
    CreatePlayerObject(&player->playerState, object, player->playerConfigIndex, player->characterIndex, playerNumberBySlot, 0, &playerSpawnPosition, 180.0f);
    player->playerState.updateCallback = UpdatePlayerDepthLimit;
    player->playerState.positionCallback = GetPlayerPositionCallback;
    SetPlayerGroundPositions(&player->playerState, 1, (player->playerSlot & 2) == 0 ? leftTeamSpawnPositions : rightTeamSpawnPositions);
    player->playerState.landingCallback = UpdatePlayerDisplay;
    player->movementTarget.x = 0.0f;
    player->movementTarget.y = 0.0f;
    player->movementTarget.z = 0.0f;
    object->mdlId[0] = CharModelCreate(characterModelIds[player->characterIndex], 4);
    CharModelStepFxSet(characterModelIds[player->characterIndex], 0);
    for (modelOrSpriteIndex = 0; modelOrSpriteIndex < 0xB; modelOrSpriteIndex++) {
        object->mtnId[modelOrSpriteIndex] = CharMotionCreate(characterModelIds[player->characterIndex], characterMotionFiles[player->characterIndex][modelOrSpriteIndex]);
        CharMotionSet(characterModelIds[player->characterIndex], object->mtnId[modelOrSpriteIndex]);
    }
    Hu3DModelCameraSet(object->mdlId[0], gameCameraIds[player->playerSlot >> 1]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[0], 2);
    Hu3DMotionSet(object->mdlId[0], object->mtnId[0]);
    Hu3DModelShadowSet(object->mdlId[0]);
    ClearPlayerGroundPositions(&player->playerState);
    for (modelOrSpriteIndex = 0; modelOrSpriteIndex < 9; modelOrSpriteIndex++) {
        player->scoreSpriteIds[modelOrSpriteIndex] = espEntry(scoreSpriteFiles[modelOrSpriteIndex], 0, 0);
        espDrawNoSet(player->scoreSpriteIds[modelOrSpriteIndex], 0);
        espDispOff(player->scoreSpriteIds[modelOrSpriteIndex]);
        espAttrSet(player->scoreSpriteIds[modelOrSpriteIndex], HUSPR_ATTR_NOANIM);
        espBankSet(player->scoreSpriteIds[modelOrSpriteIndex], 0);
        espPriSet(player->scoreSpriteIds[modelOrSpriteIndex], 0x80 - modelOrSpriteIndex);
    }
    espTPLvlSet(player->scoreSpriteIds[0], 0.9f);
    espTPLvlSet(player->scoreSpriteIds[1], 0.9f);
    if (GWPlayerCfg[player->playerConfigIndex].iscom != 0) {
        switch (GWPlayerCfg[player->playerConfigIndex].diff) {
            case 0:
                player->aiBehaviorParams = &humanAiBehaviorParams;
                break;
            case 1:
                player->aiBehaviorParams = &easyAiBehaviorParams;
                break;
            case 2:
                player->aiBehaviorParams = &normalAiBehaviorParams;
                break;
            case 3:
            default:
                player->aiBehaviorParams = &hardAiBehaviorParams;
                break;
        }
    }
    CharModelLayerSetAll2(5);
    CharMotionDataClose(characterModelIds[player->characterIndex]);
    object->objFunc = InitializePlayerObject;
}

void EmptyObjectCallback(OMOBJ *object) { }

void InitializeStageObject(OMOBJ *currentEntity)
{
    s32 mapObjectIndex;

    MapObjectDefinition *mapObject;
    s32 modelIndex;
    TeamResultState *stageState;

    stageState = &teamResultStates[currentEntity->work[0]];
    currentEntity->mdlId[0] = GetCachedModel(DATA_MAKE_NUM(DATADIR_M432, 0x02));
    Hu3DModelCameraSet(currentEntity->mdlId[0], gameCameraIds[currentEntity->work[0]]);
    Hu3DModelAttrSet(currentEntity->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(currentEntity->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(currentEntity->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(currentEntity->mdlId[0], 0);

    currentEntity->mdlId[1] = GetCachedModel(DATA_MAKE_NUM(DATADIR_M432, 0x04));
    Hu3DModelCameraSet(currentEntity->mdlId[1], gameCameraIds[currentEntity->work[0]]);
    Hu3DModelAttrSet(currentEntity->mdlId[1], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(currentEntity->mdlId[1], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(currentEntity->mdlId[1], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(currentEntity->mdlId[1], 0);

    currentEntity->mdlId[2] = GetCachedModel(stageModelFiles[currentEntity->work[0]]);
    Hu3DModelCameraSet(currentEntity->mdlId[2], gameCameraIds[currentEntity->work[0]]);
    Hu3DModelAttrSet(currentEntity->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(currentEntity->mdlId[2], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(currentEntity->mdlId[2], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(currentEntity->mdlId[2], 1);

    currentEntity->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M432, 0x01));
    Hu3DModelCameraSet(currentEntity->mdlId[3], gameCameraIds[currentEntity->work[0]]);
    Hu3DModelAttrSet(currentEntity->mdlId[3], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(currentEntity->mdlId[3], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(currentEntity->mdlId[3], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(currentEntity->mdlId[3], 0);

    currentEntity->mdlId[24] = GetCachedModel(DATA_MAKE_NUM(DATADIR_M432, 0x03));
    Hu3DModelCameraSet(currentEntity->mdlId[24], gameCameraIds[currentEntity->work[0]]);
    Hu3DModelLayerSet(currentEntity->mdlId[24], 0);
    Hu3DModelPosSet(currentEntity->mdlId[24], 0.0f, 0.0f, 0.0f);
    Hu3DModelRotSet(currentEntity->mdlId[24], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(currentEntity->mdlId[24], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(currentEntity->mdlId[24], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(currentEntity->mdlId[24], HU3D_MOTATTR_LOOP);

    currentEntity->mdlId[25] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M432, 0x07));
    Hu3DMotionSet(currentEntity->mdlId[25], Hu3DJointMotionFile(currentEntity->mdlId[25], DATA_MAKE_NUM(DATADIR_M432, 0x08)));
    Hu3DModelCameraSet(currentEntity->mdlId[25], gameCameraIds[currentEntity->work[0]]);
    Hu3DModelAttrSet(currentEntity->mdlId[25], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(currentEntity->mdlId[25], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(currentEntity->mdlId[25], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(currentEntity->mdlId[25], 2);
    Hu3DModelShadowSet(currentEntity->mdlId[25]);
    omSetTra(currentEntity, stageState->cameraLookTarget.x, 0.0f, 0.0f);
    omSetRot(currentEntity, 0.0f, 0.0f, 0.0f);
    for (modelIndex = 0; modelIndex < 0x1A; modelIndex++) {
        Hu3DModelPosSet(currentEntity->mdlId[modelIndex], stageState->cameraLookTarget.x, 0.0f, 0.0f);
        Hu3DModelRotSet(currentEntity->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
    }
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        stageState->mapObjectLists[modelIndex] = HuMemDirectMallocNum(HEAP_HEAP, 31 * sizeof(MapObjectDefinition), HU_MEMNUM_OVL);
        memcpy(stageState->mapObjectLists[modelIndex], modelIndex == 0 ? &firstTeamMapObjects : &secondTeamMapObjects, 31 * sizeof(MapObjectDefinition));
        for (mapObject = stageState->mapObjectLists[modelIndex], mapObjectIndex = 0; mapObjectIndex < 0x1F; mapObjectIndex++, mapObject++) {
            GetModelAttachmentPosition(currentEntity->mdlId[3], mapObject->attachmentName, &mapObject->modelOrigin);
            mapObject->worldPosition = mapObject->modelOrigin;
            mapObject->rotationPhase = 0.0f;
            mapObject->rotationVelocity = 0.0f;
            mapObject->objectState = -1;
            mapObject->objectIndex = mapObjectIndex;
            if ((mapObject->modelSlot > 0) && (mapObject->modelDataIds[currentEntity->work[0]] != 0)) {
                currentEntity->mdlId[mapObject->modelSlot] = GetCachedModel(mapObject->modelDataIds[currentEntity->work[0]]);
                Hu3DModelCameraSet(currentEntity->mdlId[mapObject->modelSlot], gameCameraIds[currentEntity->work[0]]);
                Hu3DModelLayerSet(currentEntity->mdlId[mapObject->modelSlot], 0);
                Hu3DModelPosSet(currentEntity->mdlId[mapObject->modelSlot], mapObject->worldPosition.x, mapObject->worldPosition.y, mapObject->worldPosition.z);
                Hu3DModelRotSet(currentEntity->mdlId[mapObject->modelSlot], 0.0f, mapObject->rotationDegrees, 0.0f);
                Hu3DModelAttrSet(currentEntity->mdlId[mapObject->modelSlot], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
                Hu3DModelAttrSet(currentEntity->mdlId[mapObject->modelSlot], HU3D_MOTATTR_PAUSE);
                Hu3DModelAttrReset(currentEntity->mdlId[mapObject->modelSlot], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(currentEntity->mdlId[mapObject->modelSlot], HU3D_ATTR_DISPOFF);
            }
        }
    }
    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
        teamResultStates[currentEntity->work[0]].characterAttachmentPositions[modelIndex][2].y = 35.0f;
    }
    GetModelAttachmentPosition(currentEntity->mdlId[3], "m432_00-d0", &teamResultStates[currentEntity->work[0]].characterAttachmentPositions[0][0]);
    GetModelAttachmentPosition(currentEntity->mdlId[3], "m432_00-d0", &teamResultStates[currentEntity->work[0]].characterAttachmentPositions[1][0]);
    GetModelAttachmentPosition(currentEntity->mdlId[3], "m432_00-d1", &teamResultStates[currentEntity->work[0]].characterAttachmentPositions[2][0]);
    GetModelAttachmentPosition(currentEntity->mdlId[3], "m432_00-d1", &teamResultStates[currentEntity->work[0]].characterAttachmentPositions[3][0]);

    UpdateTeamCharacterAttachments(currentEntity->work[0], currentEntity);

    GetModelAttachmentPosition(currentEntity->mdlId[3], "m432_00-h0", &teamResultStates[currentEntity->work[0]].sceneObjectPosition);
    for (modelIndex = 0; modelIndex < 8; modelIndex++) {
        GetModelAttachmentPosition(currentEntity->mdlId[3], stageObjectNames[modelIndex], &teamResultStates[currentEntity->work[0]].spawnPositions[modelIndex]);
        if ((modelIndex % 4) < 2) {
            teamResultStates[currentEntity->work[0]].spawnPositions[modelIndex].z -= 150.0f;
        }
    }
    GetModelAttachmentPosition(currentEntity->mdlId[3], "m432_00-n0", &teamResultStates[currentEntity->work[0]].hookModelPosition);
    Hu3DModelShadowMapSet(currentEntity->mdlId[0]);
    Hu3DModelShadowMapSet(currentEntity->mdlId[20]);
    Hu3DModelShadowMapSet(currentEntity->mdlId[21]);
    Hu3DModelAttrReset(currentEntity->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(currentEntity->mdlId[1], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(currentEntity->mdlId[2], HU3D_ATTR_DISPOFF);
    currentEntity->objFunc = EmptyObjectCallback;
}

void EmptyStageCallback(OMOBJ *object) { }

void InitializePlayerCharacterObject(OMOBJ *object)
{
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M432, 0x00));
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    omSetTra(object, teamResultStates[object->work[0]].cameraLookTarget.x, 0.0f, 0.0f);
    omSetRot(object, 0.0f, 0.0f, 0.0f);
    object->objFunc = EmptyStageCallback;
}

void UpdateFloatingEffects(OMOBJ *object)
{
    s32 teamIndex;
    PlayerGameState *firstPlayer;
    PlayerGameState *secondPlayer;
    s32 playerIndex;
    s32 objectIndex;
    void *impactSoundIndex;
    void *followupSoundIndex;
    OMOBJ *stageObject;
    void *resultSoundIndex;
    void *soundIndex;

    if (postGameController->work[0] >= 0x3EB) {
        for (teamIndex = 0; teamIndex < 2; teamIndex++) {
            stageObject = teamResultStates[teamIndex].stageObject;
            firstPlayer = GetPlayerBySlot(teamIndex * 2);
            secondPlayer = GetPlayerBySlot(teamIndex * 2 + 1);
            if ((firstPlayer->playerMode == 0x7DF) && (secondPlayer->playerMode == 0x7DF)) {
                teamResultStates[teamIndex].unreferencedMotionState += 0.05f;
                if (12.0f < teamResultStates[teamIndex].unreferencedMotionState) {
                    teamResultStates[teamIndex].unreferencedMotionState = 12.0f;
                }
                teamResultStates[teamIndex].sceneObjectPosition.y += teamResultStates[teamIndex].unreferencedMotionState;
                Hu3DModelPosSet(stageObject->mdlId[22], teamResultStates[teamIndex].sceneObjectPosition.x, teamResultStates[teamIndex].sceneObjectPosition.y, teamResultStates[teamIndex].sceneObjectPosition.z);
                teamResultStates[teamIndex].cameraLookTarget.y += 0.1f * ((500.0f + teamResultStates[teamIndex].sceneObjectPosition.y) - teamResultStates[teamIndex].cameraLookTarget.y);
                teamResultStates[teamIndex].cameraPosition.y += 0.01f * (1000.0f - teamResultStates[teamIndex].cameraPosition.y);
                teamResultStates[teamIndex].cameraPosition.z += 0.002f * ((4000.0f + teamResultStates[teamIndex].sceneObjectPosition.z) - teamResultStates[teamIndex].cameraPosition.z);
                Hu3DCameraPosSetV(gameCameraIds[teamIndex], &teamResultStates[teamIndex].cameraPosition, &worldUpVector, &teamResultStates[teamIndex].cameraLookTarget);
            }
            else {
                UpdateTeamResultState(&teamResultStates[teamIndex], teamIndex, firstPlayer, secondPlayer);
                if (NULL != teamResultStates[teamIndex].linkedPlayerObject) {
                    UpdateTeamResults(&teamResultStates[teamIndex]);
                }
                if ((2.0f <= teamResultStates[teamIndex].resultFramesElapsed) && (teamResultStates[teamIndex].resultMotionTime != 0.0f)) {
                    UpdatePlayerResultAnimation(&teamResultStates[teamIndex], firstPlayer, secondPlayer);
                }
                teamResultStates[teamIndex].pendingPlayerObject = NULL;
                teamResultStates[teamIndex].resultFramesElapsed = 0.0f;
                teamResultStates[teamIndex].resultMotionTime = 0.0f;
                teamResultStates[teamIndex].playerDepths[0] = 0.0f;
                teamResultStates[teamIndex].playerDepths[1] = 0.0f;
            }
        }
#if VERSION_NTSC
        if (postGameController->work[0] >= 0x3EC) {
            for (teamIndex = 0; teamIndex < 2; teamIndex++) {
                if (teamResultStates[teamIndex].impactSoundId >= 0) {
                    HuAudFXStop(teamResultStates[teamIndex].impactSoundId);
                }
                if (teamResultStates[teamIndex].followupSoundId >= 0) {
                    HuAudFXStop(teamResultStates[teamIndex].followupSoundId);
                }
                if (teamResultStates[teamIndex].resultSoundId >= 0) {
                    HuAudFXStop(teamResultStates[teamIndex].resultSoundId);
                }
                teamResultStates[teamIndex].impactSoundId = -1;
                teamResultStates[teamIndex].followupSoundId = -1;
                teamResultStates[teamIndex].resultSoundId = -1;
            }
        }
#endif
    }
}

void UpdateTeamCameras(void)
{
    float cameraEase;
    float transitionProgress;

    TeamResultState *cameraState;
    s32 cameraIndex;

    for (cameraIndex = 0; cameraIndex < 2; cameraIndex++) {
        cameraState = &teamResultStates[cameraIndex];
        if (cameraState->cameraTransitionDuration == 0.0f) {
            cameraState->cameraPosition = cameraState->cameraLookTarget;
            cameraState->cameraPosition.x = cameraState->cameraPosition.x + (cameraState->cameraDistance * sind(cameraState->cameraOrbitAngle));
            cameraState->cameraPosition.y += cameraState->cameraHeight;
            cameraState->cameraPosition.z = cameraState->cameraPosition.z + (cameraState->cameraDistance * cosd(cameraState->cameraOrbitAngle));
        }
        else {
            cameraState->cameraTransitionFrame += 1.0f;
            transitionProgress = cameraState->cameraTransitionFrame / cameraState->cameraTransitionDuration;
            cameraEase = sind((90.0f * transitionProgress)) * sind((90.0f * transitionProgress));

            cameraState->cameraPosition.x = cameraState->cameraStartPosition.x + (cameraEase * (cameraState->cameraEndPosition.x - cameraState->cameraStartPosition.x));
            cameraState->cameraPosition.y = cameraState->cameraStartPosition.y + (cameraEase * (cameraState->cameraEndPosition.y - cameraState->cameraStartPosition.y));
            cameraState->cameraPosition.z = cameraState->cameraStartPosition.z + (cameraEase * (cameraState->cameraEndPosition.z - cameraState->cameraStartPosition.z));
            cameraState->cameraLookTarget.x = cameraState->cameraStartLookTarget.x + (cameraEase * (cameraState->cameraEndLookTarget.x - cameraState->cameraStartLookTarget.x));
            cameraState->cameraLookTarget.y = cameraState->cameraStartLookTarget.y + (cameraEase * (cameraState->cameraEndLookTarget.y - cameraState->cameraStartLookTarget.y));
            cameraState->cameraLookTarget.z = cameraState->cameraStartLookTarget.z + (cameraEase * (cameraState->cameraEndLookTarget.z - cameraState->cameraStartLookTarget.z));
            if (cameraState->cameraTransitionDuration <= cameraState->cameraTransitionFrame) {
                cameraState->cameraDistance = cameraState->cameraPosition.z - cameraState->cameraLookTarget.z;
                cameraState->cameraHeight = cameraState->cameraPosition.y - cameraState->cameraLookTarget.y;
                cameraState->cameraTransitionFrame = 0.0f;
                cameraState->cameraTransitionDuration = 0.0f;
            }
        }
        Hu3DCameraPosSetV(gameCameraIds[cameraIndex], &cameraState->cameraPosition, &worldUpVector, &cameraState->cameraLookTarget);
    }
}

#if VERSION_NTSC
#define PRESENT_RECORD 5400
#else
#define PRESENT_RECORD 5250
#endif

void UpdatePostGameController(OMOBJ *activeEntity)
{
    Vec averagePlayerPosition;
    Vec shadowDirection;
    Vec shadowUp;
    PlayerGameState *playerStates[1];

    float transitionProgress;
    float transitionEase;
    float maximumPlayerDistance;
    float playerDistance;
    float resultTimer;
    float remainingTime;

    TeamResultState *resultState;
    s32 playerIndex;
    s32 completedPlayerCount;

    if (omSysExitReq != 0) {
        StopMusicSequence(-1);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 0x3C);
        activeEntity->objFunc = ExitPostGame;
    }
    gameFrameCounter++;
    switch (activeEntity->work[0]) {
        case 0x3E8:
            if (WipeStatGet() == 0) {
                activeEntity->work[0]++;
                stateTransitionTimer = 0.0f;
            }
            break;
        case 0x3E9:
            stateTransitionTimer += 1.0f;
            transitionProgress = stateTransitionTimer / 60.0f;
            transitionEase = sind((90.0f * transitionProgress)) * sind((90.0f * transitionProgress));
            for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                teamResultStates[playerIndex].cameraLookTarget.z = -300.0f + (-400.0f * transitionEase);
            }
            UpdateTeamCameras();
            if (60.0f <= stateTransitionTimer) {
                activeEntity->work[0]++;
                stateTransitionTimer = 0.0f;
            }
            break;
        case 0x3EA:
            if (activeMessageSequence < 0) {
                activeMessageSequence = MGSeqCreate(3, 0);
            }
            else {
                if ((activeMusicSequence < 0) && ((MGSeqStatGet(activeMessageSequence) & 0x10) != 0)) {
                    PlayMusicSequence(0x46);
                }
                if (MGSeqStatGet(activeMessageSequence) == 0) {
                    activeEntity->work[0]++;
                    elapsedRoundFrames = 0;
                    activeMessageSequence = -1;
                    stateTransitionTimer = 0.0f;
                }
            }
            break;
        case 0x3EB:
            elapsedRoundFrames++;
            if (countdownMessageSequence >= 0) {
                MGSeqParamSet(countdownMessageSequence, 1, (300 * REFRESH_RATE + REFRESH_RATE - 1 - elapsedRoundFrames) / REFRESH_RATE);
            }
            else if (elapsedRoundFrames > 270 * REFRESH_RATE) {
                countdownMessageSequence = MGSeqCreate(1, (300 * REFRESH_RATE + REFRESH_RATE - 1 - elapsedRoundFrames) / REFRESH_RATE, -1, -1);
                MGSeqPosSet(countdownMessageSequence, 288.0f, 400.0f);
            }
            SetTimerDigitSprites(timerDigitSprites, 0, elapsedRoundFrames);

            UpdateTeamCameras();
            if ((elapsedRoundFrames >= 300 * REFRESH_RATE) || (winningTeam >= 0)) {
                activeEntity->work[0] = 0x3EC;
                stateTransitionTimer = 0.0f;
            }
            break;
        case 0x3EC:
            if (activeMessageSequence < 0) {
                if (countdownMessageSequence >= 0) {
                    MGSeqParamSet(countdownMessageSequence, 2, -1);
                }
                countdownMessageSequence = -1;
                completedPlayerFrames = 0;
                activeMessageSequence = MGSeqCreate(3, 1);
                StopMusicSequence(0x64);
            }
            else {
                completedPlayerCount = 0;
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if (((PlayerGameState *)playerObjectList[playerIndex]->data)->playerMode >= 0x7DA) {
                        completedPlayerCount++;
                    }
                }
                if (++completedPlayerFrames > 0x78) {
                    completedPlayerCount = 4;
                }
                stateTransitionTimer += 1.0f;
                if ((completedPlayerCount >= 4) && (60.0f <= stateTransitionTimer) && (MGSeqStatGet(activeMessageSequence) == 0)) {
                    activeEntity->work[0] = 0x3EE;
                    if ((winningTeam >= 0) && (elapsedRoundFrames < PRESENT_RECORD)) {
                        if (GWPlayerCfg[GetPlayerBySlot(winningTeam)->playerConfigIndex].iscom != 0) {
                            if (GWPlayerCfg[GetPlayerBySlot(winningTeam + 1)->playerConfigIndex].iscom == 0) {
                                goto block81;
                            }
                        }
                        else {
                        block81:
                            GWGameStat.present[0x32] = 1;
                        }
                    }
                    if ((winningTeam >= 0) && (bestRoundRecordFrames > elapsedRoundFrames)) {
                        if (GWPlayerCfg[GetPlayerBySlot(winningTeam)->playerConfigIndex].iscom != 0) {
                            if (GWPlayerCfg[GetPlayerBySlot(winningTeam + 1)->playerConfigIndex].iscom == 0) {
                                goto block98;
                            }
                        }
                        else {
                        block98:
                            activeEntity->work[0] = 0x3ED;
                            StoreRoundRecord(3);
                        }
                    }
                    activeMessageSequence = -1;
                    stateTransitionTimer = 0.0f;
                }
            }
            break;
        case 0x3ED:
            if (activeMessageSequence < 0) {
                activeMessageSequence = MGSeqCreate(0xE, bestRoundRecordFrames);
            }
            else if (MGSeqStatGet(activeMessageSequence) == 0) {
                activeEntity->work[0]++;
                stateTransitionTimer = 0.0f;
                activeMessageSequence = -1;
            }
            break;
        case 0x3EE:
            if (winningTeam < 0) {
                activeEntity->work[0] = 0x3F1;
                stateTransitionTimer = 0.0f;
            }
            else if (2000.0f < teamResultStates[winningTeam >> 1].sceneObjectPosition.y) {
                activeEntity->work[0]++;
                WipeColorSet(0xFF, 0xFF, 0xFF);
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
            }
            break;
        case 0x3EF:
            if (WipeStatGet() == 0) {
                activeEntity->work[0]++;
                stateTransitionTimer = 0.0f;
                omDelObjEx(gameObjectManager, floatingEffectObject);
                Hu3DCameraKill(gameCameraIds[(winningTeam >> 1) ^ 1]);
                Hu3DCameraPerspectiveSet(gameCameraIds[winningTeam >> 1], 20.0f, 20.0f, 20000.0f, 1.2f);
                Hu3DCameraViewportSet(gameCameraIds[winningTeam >> 1], 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
                Hu3DCameraScissorSet(gameCameraIds[winningTeam >> 1], 0, 0, 0x280, 0x1E0);
                resultState = &teamResultStates[winningTeam >> 1];
                resultState->sceneObjectPosition.y = 5000.0f;
                Hu3DModelPosSet(resultState->stageObject->mdlId[22], resultState->sceneObjectPosition.x, resultState->sceneObjectPosition.y, resultState->sceneObjectPosition.z);
                Hu3DModelPosSet(resultState->stageObject->mdlId[24], resultState->sceneObjectPosition.x, resultState->sceneObjectPosition.y, resultState->sceneObjectPosition.z);
                Hu3DModelAttrReset(resultState->stageObject->mdlId[24], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(resultState->stageObject->mdlId[24], HU3D_MOTATTR_PAUSE);
                resultState->cameraLookTarget.y = 8000.0f;
                resultState->cameraPosition.y = 8000.0f;
                resultState->cameraPosition.z = 5000.0f + resultState->sceneObjectPosition.z;
                Hu3DCameraPosSetV(gameCameraIds[winningTeam >> 1], &resultState->cameraPosition, &worldUpVector, &resultState->cameraLookTarget);
                resultState->cameraStartLookTarget = resultState->cameraLookTarget;
                resultState->cameraEndLookTarget = resultState->cameraLookTarget;
                resultState->cameraStartPosition = resultState->cameraPosition;
                resultState->cameraEndPosition = resultState->cameraPosition;
                resultState->cameraEndLookTarget.y = 5200.0f;
                resultState->cameraEndPosition.y = 5200.0f;
                resultState->cameraEndPosition.z = 2000.0f + resultState->sceneObjectPosition.z;
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 0x3C);
                resultSoundHandle = HuAudFXPlay(0x69B);
            }
            break;
        case 0x3F0:
            resultState = &teamResultStates[winningTeam >> 1];
            stateTransitionTimer += 1.0f;
            transitionProgress = stateTransitionTimer / (3.0f * REFRESH_RATE);
            transitionEase = sind((90.0f * transitionProgress)) * sind((90.0f * transitionProgress));
            resultState->cameraLookTarget.y = resultState->cameraStartLookTarget.y + (transitionEase * (resultState->cameraEndLookTarget.y - resultState->cameraStartLookTarget.y));
            resultState->cameraPosition.y = resultState->cameraStartPosition.y + (transitionEase * (resultState->cameraEndPosition.y - resultState->cameraStartPosition.y));
            if ((1.5f * REFRESH_RATE) < stateTransitionTimer) {
                transitionProgress = (stateTransitionTimer - 1.5f * REFRESH_RATE) / (1.5f * REFRESH_RATE);
                transitionEase = sind((90.0f * transitionProgress)) * sind((90.0f * transitionProgress));
                resultState->cameraPosition.z = resultState->cameraStartPosition.z + (transitionEase * (resultState->cameraEndPosition.z - resultState->cameraStartPosition.z));
            }
            Hu3DCameraPosSetV(gameCameraIds[winningTeam >> 1], &resultState->cameraPosition, &worldUpVector, &resultState->cameraLookTarget);
            if ((3 * REFRESH_RATE) <= stateTransitionTimer) {
                activeEntity->work[0]++;
                stateTransitionTimer = 0.0f;
            }
            break;
        case 0x3F1:
            if (activeMessageSequence < 0) {
                if (winningTeam < 0) {
                    activeMessageSequence = MGSeqCreate(3, 2);
                    HuAudSStreamPlay(4);
                }
                else {
                    activeMessageSequence = MGSeqCreate(5, 3, GetPlayerBySlot(winningTeam)->characterIndex, GetPlayerBySlot(winningTeam + 1)->characterIndex, -1, -1);
                    HuAudSStreamPlay(1);
                }
            }
            else {
                stateTransitionTimer += 1.0f;
                if (0 <= winningTeam) { }
                if ((210.0f < stateTransitionTimer) && (MGSeqStatGet(activeMessageSequence) == 0)) {
                    StopMusicSequence(-1);
                    activeMessageSequence = -1;
                    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 0x3C);
                    activeEntity->objFunc = ExitPostGame;
                }
            }
            break;
        default:
            OSReport("*** main mode error(%d)!!\n", activeEntity->work[0]);
            break;
    }
    UpdateScorePulse();
    if ((winningTeam >= 0) && (activeEntity->work[0] >= 0x3F0)) {
        resultState = &teamResultStates[winningTeam >> 1];
        resultState->sceneObjectPosition.y = 5000.0 + (10.0 * sind((gameFrameCounter * 4)));
        Hu3DModelPosSet(resultState->stageObject->mdlId[22], resultState->sceneObjectPosition.x, resultState->sceneObjectPosition.y, resultState->sceneObjectPosition.z);
    }
    averagePlayerPosition.x = 0.0f;
    averagePlayerPosition.y = 0.0f;
    averagePlayerPosition.z = 0.0f;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerStates[0] = playerObjectList[playerIndex]->data;
        averagePlayerPosition.x += playerStates[0]->playerState.position.x;
        averagePlayerPosition.z += playerStates[0]->playerState.position.z;
    }
    averagePlayerPosition.x *= 0.25f;
    averagePlayerPosition.z *= 0.25f;
    maximumPlayerDistance = 0.0f;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerDistance = VECDistance(&averagePlayerPosition, &((PlayerGameState *)playerObjectList[playerIndex]->data)->playerState.position);
        if (maximumPlayerDistance < playerDistance) {
            maximumPlayerDistance = playerDistance;
        }
    }

    SetVectorComponents(&shadowUp, averagePlayerPosition.x, 0.0f, averagePlayerPosition.z);

    averagePlayerPosition.y = 6.0f * maximumPlayerDistance;
    averagePlayerPosition.z += 1000.0f;
    VECNormalize(&averagePlayerPosition, &shadowDirection);
    Hu3DShadowPosSet(&averagePlayerPosition, &shadowDirection, &shadowUp);
    UpdateParticleEffects();
}

void InitializePostGameController(OMOBJ *object)
{
    s32 unreferencedLocalValue;
    object->work[0] = 0x3E8;
    CreateTimerSprites(80.0f, DATA_MAKE_NUM(DATADIR_M432, 0x1E));
    ShowTimerSprites();
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    object->objFunc = UpdatePostGameController;
}

void ObjectSetup(void)
{
    Vec shadowDirection;
    float shadowPositionX;
    float shadowPositionY;
    float shadowPositionZ;

    TeamResultState *cameraState;
    s32 setupIndex;
    float *viewport;
    PlayerGameState *player;
    OMOBJ *gameObject;
    s32 playerSearchIndex;
    s32 unreferencedSetupValue;
    s32 playerSortKey;
    s32 bestPlayerIndex;
    s32 bestPlayerSortKey;
    s32 lightIndex;

    OSReport("******* M432 ObjectSetup *********\n");
    gameObjectManager = omInitObjMan(0x258, 0x2000);
    omGameSysInit(gameObjectManager);
    nMap = 0;
    nChar = 0;
    gameFrameCounter = 0;
    activeMessageSequence = -1;
    countdownMessageSequence = -1;
    winningTeam = -1;
    completedPlayerFrames = 0;
    activeMusicSequence = -1;
    resultSoundHandle = -1;
    elapsedRoundFrames = 0;
    bestRoundRecordFrames = GWMGRecordGet(3);
    scorePulseAngle = -1.0f;
    if (bestRoundRecordFrames == 0) {
        bestRoundRecordFrames = 120 * REFRESH_RATE;
    }
    InitializeModelCache();
    for (setupIndex = 0; setupIndex < 2; setupIndex++) {
        teamResultStates[setupIndex].linkedPlayerObject = NULL;
        teamResultStates[setupIndex].unreferencedState18 = 0;
        teamResultStates[setupIndex].cameraLookTarget.x = -800.0f + (1600.0f * setupIndex);
        teamResultStates[setupIndex].cameraLookTarget.y = 0.0f;
        teamResultStates[setupIndex].cameraLookTarget.z = -300.0f;
        teamResultStates[setupIndex].objectFloorHeight = 0.0f;
        teamResultStates[setupIndex].cameraOrbitAngle = 0.0f;
        teamResultStates[setupIndex].cameraDistance = 3900.0f;
        teamResultStates[setupIndex].cameraHeight = 3250.0f;
        teamResultStates[setupIndex].resultFramesElapsed = 0.0f;
        teamResultStates[setupIndex].cameraTargetHeight = 0.0f;
        teamResultStates[setupIndex].cameraTargetDepth = 0.0f;
        teamResultStates[setupIndex].cameraTransitionFrame = 0.0f;
        teamResultStates[setupIndex].cameraTransitionDuration = 0.0f;
        teamResultStates[setupIndex].characterRotationA = 0.0f;
        teamResultStates[setupIndex].characterRotationB = 0.0f;
        teamResultStates[setupIndex].motionTimeRatio = 0.0f;
        teamResultStates[setupIndex].motionTime = 0.0f;
        teamResultStates[setupIndex].unreferencedMotionState = 0.0f;
        teamResultStates[setupIndex].selectedObjectIndex = frandmod(5);
        teamResultStates[setupIndex].selectedVariantIndex = frandmod(9);
        teamResultStates[setupIndex].impactSoundId = -1;
        teamResultStates[setupIndex].followupSoundId = -1;
        teamResultStates[setupIndex].impactSoundFade = 0.0f;
        teamResultStates[setupIndex].resultSoundId = -1;
        teamResultStates[setupIndex].resultSoundFade = 0.0f;
    }
    Hu3DCameraCreate(3);
    Hu3DCameraPerspectiveSet(3, 20.0f, 20.0f, 20000.0f, 0.6f);
    UpdateTeamCameras();
    for (viewport = gameCameraViewports, setupIndex = 0; setupIndex < 2; setupIndex++, viewport += 4) {
        Hu3DCameraViewportSet(gameCameraIds[setupIndex], viewport[0], viewport[1], viewport[2], viewport[3], 0.0f, 1.0f);
        Hu3DCameraScissorSet(gameCameraIds[setupIndex], viewport[0], viewport[1], viewport[2], viewport[3]);
    }
    lightIndex = Hu3DGLightCreateV(&keyLightPosition, &keyLightDirection, &keyLightColor);
    Hu3DGLightStaticSet(lightIndex, 0);
    Hu3DGLightInfinitytSet(lightIndex);
    Hu3DShadowCreate(30.0f, 20.0f, 20000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    VECNormalize(&shadowPosition, &shadowDirection);
    Hu3DShadowPosSet(&shadowPosition, &shadowDirection, &shadowUpVector);
    InitializePlayerCollision(GetPlayerRuntimeState, 0.0f);
    omMakeGroupEx(gameObjectManager, 2, 4);
    playerObjectList = omGetGroupMemberListEx(gameObjectManager, 2);
    for (setupIndex = 0; setupIndex < 4; setupIndex++) {
        gameObject = omAddObjEx(gameObjectManager, 0x68, 1, 0xB, 2, UpdatePlayerObject);
        gameObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerGameState), HU_MEMNUM_OVL);
        gameObject->work[0] = setupIndex;
        player = gameObject->data;
        player->playerSlot = -1;
        player->playerConfigIndex = gameObject->work[0];
        player->playerGroup = GWPlayerCfg[player->playerConfigIndex].group;
    }
    for (setupIndex = 0; setupIndex < 4; setupIndex++) {
        bestPlayerIndex = -1;
        bestPlayerSortKey = 0x28;
        for (playerSearchIndex = 0; playerSearchIndex < 4; playerSearchIndex++) {
            player = playerObjectList[playerSearchIndex]->data;
            if (player->playerSlot < 0) {
                playerSortKey = player->playerConfigIndex + (player->playerGroup * 4);
                if (bestPlayerSortKey > playerSortKey) {
                    bestPlayerSortKey = playerSortKey;
                    bestPlayerIndex = playerSearchIndex;
                }
            }
        }
        ((PlayerGameState *)playerObjectList[bestPlayerIndex]->data)->playerSlot = setupIndex;
    }
    for (setupIndex = 0; setupIndex < 2; setupIndex++) {
        teamResultStates[setupIndex].stageObject = omAddObjEx(gameObjectManager, 0x64, 0x1A, 0, 0, InitializeStageObject);
        teamResultStates[setupIndex].stageObject->work[0] = setupIndex;
        teamResultStates[setupIndex].playerObject = omAddObjEx(gameObjectManager, 0x65, 1, 0, 1, InitializePlayerCharacterObject);
        teamResultStates[setupIndex].playerObject->work[0] = setupIndex;
        MapObject[nMap++] = teamResultStates[setupIndex].playerObject;
    }
    InitializeParticleList();
    playerSpriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M432, 0x28));
    HuSprAnimLock(playerSpriteAnimation);
    particleManagerId = Hu3DParManCreate(playerSpriteAnimation, 0x1F4, &particleParameters);
    Hu3DParManAttrSet(particleManagerId, 1);
    floatingEffectFrame = 0;
    InitializeResourceList();
    omMakeGroupEx(gameObjectManager, 3, 0xF0);
    effectObjectList = omGetGroupMemberListEx(gameObjectManager, 3);
    stageModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M432, 0x09));
    Hu3DModelAttrSet(stageModelId, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(stageModelId, HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    floatingEffectObject = omAddObjEx(gameObjectManager, 0x66, 0, 0, -1, UpdateFloatingEffects);
    postGameController = omAddObjEx(gameObjectManager, 0x69, 0x1A, 0, -1, InitializePostGameController);
    postGameController->work[0] = 0x3E8;
}

void ExitPostGame(OMOBJ *objectParameter0)
{
    s32 characterIndex;

    if (resultSoundHandle >= 0) {
        resultSoundHandle = -1;
        HuAudFXStop(resultSoundHandle);
    }
    if (WipeStatGet() == 0) {
        if (activeMessageSequence >= 0) {
            MGSeqKill(activeMessageSequence);
        }
        if (countdownMessageSequence >= 0) {
            MGSeqKill(countdownMessageSequence);
        }
        MGSeqKillAll();
        HuAudAllStop();
        for (characterIndex = 0; characterIndex < 4; characterIndex++) {
            CharModelKill(characterModelIds[GWPlayerCfg[characterIndex].character]);
        }
        omOvlReturnEx(1, 1);
    }
}
