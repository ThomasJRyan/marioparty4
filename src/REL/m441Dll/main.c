#include "REL/executor.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/frand.h"
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
#include "version.h"

#ifndef __MWERKS__
#include <game/hsfex.h>
#include <game/audio.h>
#endif

void HuSysVWaitSet(s16 vcount);

#undef ABS
#define ABS(x) ((0 > (x)) ? -(x) : (x))

typedef struct ModelResourceEntry {
    /* 0x00 */ u16 lookupKey;
    /* 0x02 */ s16 lookupValue;
    /* 0x04 */ struct ModelResourceEntry *previousEntry;
    /* 0x08 */ struct ModelResourceEntry *nextEntry;
} ModelResourceEntry; // Size 0xC

typedef struct ParticleBatch {
    /* 0x00 */ HU3DPARMANPARAM *particleParameters;
    /* 0x04 */ s32 *particleModelIds;
    /* 0x08 */ s32 lifetimeFrames;
    /* 0x0C */ s32 particleCount;
    /* 0x10 */ struct ParticleBatch *previousBatch;
    /* 0x14 */ struct ParticleBatch *nextBatch;
} ParticleBatch; // Size 0x18

typedef struct {
    /* 0x00 */ u32 resourceId;
    /* 0x04 */ s16 modelId;
} ModelResourceCacheEntry; // Size 8

ModelResourceEntry modelResourceListHead;
ParticleBatch particleBatchList;
HUPROCESS *objectManager;
Vec cameraStartPosition;
Vec cameraTargetPosition;
Vec cameraStartUp;
Vec cameraTargetUp;
u32 minigameFrameCounter;
s32 roundTimerFrames;
u32 recordedRoundTime;
s16 sequenceId;
s16 timerSequenceId;
s32 audioSequenceId;
s32 cameraTransitionActive;
float timerPulseAngle;
s32 timerSprites[16];
ModelResourceCacheEntry modelResourceCache[32];

void InitializeLookupList(void)
{
    modelResourceListHead.previousEntry = NULL;
    modelResourceListHead.nextEntry = NULL;
}

void RegisterModelResource(u16 resourceKey, s16 modelId)
{
    ModelResourceEntry *newEntry;
    ModelResourceEntry *listHead;
    ModelResourceEntry *previousFirstEntry;

    newEntry = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*newEntry), HU_MEMNUM_OVL);
    newEntry->lookupKey = resourceKey;
    newEntry->lookupValue = modelId;
    listHead = &modelResourceListHead;
    previousFirstEntry = listHead->nextEntry;
    listHead->nextEntry = newEntry;
    if (NULL != previousFirstEntry) {
        previousFirstEntry->previousEntry = newEntry;
    }
    newEntry->previousEntry = listHead;
    newEntry->nextEntry = previousFirstEntry;
}

s16 TakeModelResource(s16 resourceKey)
{
    ModelResourceEntry *entry;
    ModelResourceEntry *previousEntry;
    ModelResourceEntry *nextEntry;
    s16 removedValue;

    entry = modelResourceListHead.nextEntry;
    while (NULL != entry) {
        if (resourceKey == entry->lookupKey) {
            removedValue = entry->lookupValue;
            previousEntry = entry->previousEntry;
            nextEntry = entry->nextEntry;
            previousEntry->nextEntry = nextEntry;
            if (NULL != nextEntry) {
                nextEntry->previousEntry = previousEntry;
            }
            HuMemDirectFree(entry);
            return removedValue;
        }
        entry = entry->nextEntry;
    }
    return -1;
}

void InitializeParticleBatchList(void)
{
    particleBatchList.particleParameters = NULL;
    particleBatchList.nextBatch = NULL;
}

void CreateParticleBatch(s16 particleManagerId, HU3DPARMANPARAM *particleParameters, s32 modelLayer, s32 lifetimeFrames, s32 particleCount, Vec *particlePositions, u8 blendMode)
{
    ParticleBatch *particleListHead;
    ParticleBatch *newParticleBatch;
    ParticleBatch *previousFirstBatch;
    s32 i;

    newParticleBatch = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*newParticleBatch), HU_MEMNUM_OVL);
    newParticleBatch->particleParameters = particleParameters;
    newParticleBatch->lifetimeFrames = lifetimeFrames;
    newParticleBatch->particleCount = particleCount;
    newParticleBatch->particleModelIds = HuMemDirectMallocNum(HEAP_HEAP, particleCount * sizeof(*newParticleBatch->particleModelIds), HU_MEMNUM_OVL);
    for (i = 0; i < particleCount; i++, particlePositions++) {
        newParticleBatch->particleModelIds[i] = Hu3DParManLink(particleManagerId, particleParameters);
        if (newParticleBatch->particleModelIds[i] >= 0) {
            Hu3DParManAttrReset(newParticleBatch->particleModelIds[i], 1);
            Hu3DParManPosSet(newParticleBatch->particleModelIds[i], particlePositions->x, particlePositions->y, particlePositions->z);
            Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(newParticleBatch->particleModelIds[i]), blendMode);
            Hu3DModelLayerSet(Hu3DParManModelIDGet(newParticleBatch->particleModelIds[i]), modelLayer);
        }
    }
    previousFirstBatch = &particleBatchList;
    particleListHead = previousFirstBatch->nextBatch;
    previousFirstBatch->nextBatch = newParticleBatch;
    if (NULL != particleListHead) {
        particleListHead->previousBatch = newParticleBatch;
    }
    newParticleBatch->previousBatch = previousFirstBatch;
    newParticleBatch->nextBatch = particleListHead;
}

void UpdateParticleBatches(void)
{
    ParticleBatch *particleBatch;
    ParticleBatch *previousBatch;
    ParticleBatch *nextBatch;
    s32 i;

    particleBatch = particleBatchList.nextBatch;
    while (NULL != particleBatch) {
        particleBatch->lifetimeFrames--;
        if (particleBatch->lifetimeFrames == 0) {
            for (i = 0; i < particleBatch->particleCount; i++) {
                if (particleBatch->particleModelIds[i] >= 0) {
                    Hu3DParManAttrSet(particleBatch->particleModelIds[i], 1);
                }
            }
        }
        previousBatch = particleBatch->previousBatch;
        nextBatch = particleBatch->nextBatch;
        if (-particleBatch->particleParameters->maxTime > particleBatch->lifetimeFrames) {
            for (i = 0; i < particleBatch->particleCount; i++) {
                if (particleBatch->particleModelIds[i] >= 0) {
                    Hu3DParManKill(particleBatch->particleModelIds[i]);
                }
            }
            previousBatch->nextBatch = nextBatch;
            if (NULL != nextBatch) {
                nextBatch->previousBatch = previousBatch;
            }
            HuMemDirectFree(particleBatch->particleModelIds);
            HuMemDirectFree(particleBatch);
        }
        particleBatch = nextBatch;
    }
}

void InitializeModelResourceCache(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        modelResourceCache[i].resourceId = 0;
        modelResourceCache[i].modelId = -1;
    }
}

s16 LoadCachedModelResource(s32 resourceId)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (resourceId == modelResourceCache[i].resourceId) {
            return Hu3DModelLink(modelResourceCache[i].modelId);
        }
    }
    for (i = 0; i < 32; i++) {
        if (modelResourceCache[i].resourceId == 0) {
            modelResourceCache[i].resourceId = resourceId;
            modelResourceCache[i].modelId = Hu3DModelCreateFile(resourceId);
            return modelResourceCache[i].modelId;
        }
    }
    return Hu3DModelCreateFile(resourceId);
}

void SetTimerDigitSprites(s32 *spriteIds, s32 firstDigitIndex, s32 timerFrames)
{
    s32 digitValue;

    espBankSet(spriteIds[firstDigitIndex + 1], 10);
    espBankSet(spriteIds[firstDigitIndex + 4], 11);
    if (600 * REFRESH_RATE <= timerFrames) {
        timerFrames = 600 * REFRESH_RATE - 1;
    }
    digitValue = timerFrames / (60 * REFRESH_RATE);
    timerFrames -= digitValue * (60 * REFRESH_RATE);
    if (digitValue > 9) {
        digitValue = 9;
    }
    espBankSet(spriteIds[firstDigitIndex], digitValue);
    digitValue = timerFrames / REFRESH_RATE;
    timerFrames -= digitValue * REFRESH_RATE;
    espBankSet(spriteIds[firstDigitIndex + 2], digitValue / 10);
    espBankSet(spriteIds[firstDigitIndex + 3], digitValue % 10);
    digitValue = 101.5f * timerFrames / REFRESH_RATE;
    espBankSet(spriteIds[firstDigitIndex + 5], digitValue / 10);
    espBankSet(spriteIds[firstDigitIndex + 6], digitValue % 10);
}

void SetPlayerTimerRecord(s32 playerIndex)
{
    timerPulseAngle = 0.0f;
    recordedRoundTime = roundTimerFrames;
    GWMGRecordSet(playerIndex, recordedRoundTime);
    SetTimerDigitSprites(timerSprites, 7, roundTimerFrames);
}

void ShowTimerSprites(void)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        espDispOn(timerSprites[i]);
    }
}

void UpdateTimerPulse(void)
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

void CreateTimerSprites(float verticalPosition, s32 labelResourceId)
{
    float displayY;
    float displayX;
    s32 i;

    displayX = 288.0f;
    displayY = verticalPosition;
    timerSprites[15] = espEntry(labelResourceId, 0, 0);
    espTPLvlSet(timerSprites[15], 0.5f);
    espColorSet(timerSprites[15], 0, 0, 0);
    espPosSet(timerSprites[15], displayX, displayY);
    displayX = 248.0f;
    displayY = verticalPosition - 8.0f;
    timerSprites[14] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 55), 0, 0);
    espPosSet(timerSprites[14], displayX - 18.0f, displayY);
    for (i = 7; i <= 13; i++) {
        timerSprites[i] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 48), 0, 0);
        espPosSet(timerSprites[i], displayX, displayY);
        displayX += 16.0f;
    }
    SetTimerDigitSprites(timerSprites, 7, recordedRoundTime);
    displayX = 248.0f;
    displayY = verticalPosition + 8.0f;
    for (i = 0; i <= 6; i++) {
        timerSprites[i] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 49), 0, 0);
        espPosSet(timerSprites[i], displayX, displayY);
        displayX += 16.0f;
    }
    SetTimerDigitSprites(timerSprites, 0, 0);
    for (i = 0; i < 16; i++) {
        espDrawNoSet(timerSprites[i], 0);
        espDispOff(timerSprites[i]);
        espAttrSet(timerSprites[i], HUSPR_ATTR_NOANIM);
        espPriSet(timerSprites[i], 0x80 + i);
    }
}

void InterpolateVector(float progress, Vec *startPosition, Vec *targetPosition, Vec *resultPosition)
{
    float easedProgress;

    if (1.0f < progress) {
        progress = 1.0f;
    }
    easedProgress = sind(90.0f * progress) * sind(90.0f * progress);
    resultPosition->x = startPosition->x + easedProgress * (targetPosition->x - startPosition->x);
    resultPosition->y = startPosition->y + easedProgress * (targetPosition->y - startPosition->y);
    resultPosition->z = startPosition->z + easedProgress * (targetPosition->z - startPosition->z);
}

void SetCameraView(Vec *cameraPosition, Vec *cameraTarget, Vec *cameraUp)
{
    Hu3DCameraPosSetV(1, cameraPosition, cameraTarget, cameraUp);
    cameraStartPosition = *cameraPosition;
    cameraStartUp = *cameraUp;
}

s32 UpdateCameraTransition(float progress, Vec *cameraTarget)
{
    Vec cameraPosition;
    Vec cameraUp;

    InterpolateVector(progress, &cameraStartPosition, &cameraTargetPosition, &cameraPosition);
    InterpolateVector(progress, &cameraStartUp, &cameraTargetUp, &cameraUp);
    Hu3DCameraPosSetV(1, &cameraPosition, cameraTarget, &cameraUp);
    if (1.0f <= progress) {
        return 1;
    }
    else {
        cameraTransitionActive = 1;
        return 0;
    }
}

void FinishCameraTransition(float progress, Vec *cameraTarget)
{
    Vec cameraPosition;
    Vec cameraUp;

    InterpolateVector(progress, &cameraStartPosition, &cameraTargetPosition, &cameraPosition);
    InterpolateVector(progress, &cameraStartUp, &cameraTargetUp, &cameraUp);
    SetCameraView(&cameraPosition, cameraTarget, &cameraUp);
    cameraTransitionActive = 1;
}

void ProjectWorldPositionToScreen(Vec *worldPosition, s16 cameraMask, Vec *screenPosition)
{
    Mtx viewMatrix;
    Vec cameraSpacePosition;
    float horizontalProjectionScale;
    float verticalProjectionScale;
    float viewportLeft;
    float viewportWidth;
    HU3DCAMERA *camera;
    s32 i;

    for (i = 0; i < 16; i++) {
        if (cameraMask & (1 << i)) {
            break;
        }
    }
    camera = &Hu3DCamera[i];
    MTXLookAt(viewMatrix, &camera->pos, &camera->up, &camera->target);
    MTXMultVec(viewMatrix, worldPosition, &cameraSpacePosition);
    horizontalProjectionScale = cameraSpacePosition.z * (sind(camera->fov / 2) / cosd(camera->fov / 2)) * camera->aspect;
    verticalProjectionScale = cameraSpacePosition.z * (sind(camera->fov / 2) / cosd(camera->fov / 2));
    viewportLeft = 0.9f * camera->viewportX;
    viewportWidth = 0.9f * camera->viewportW;
    screenPosition->x = viewportWidth / 2 + cameraSpacePosition.x * ((viewportWidth / 2) / -horizontalProjectionScale) + viewportLeft;
    screenPosition->y = camera->viewportH / 2 + cameraSpacePosition.y * ((camera->viewportH / 2) / verticalProjectionScale) + camera->viewportY;
    screenPosition->z = 0.0f;
}

void GetModelJointPosition(s16 modelId, char *jointName, Vec *jointPosition)
{
    Mtx modelMatrix;

    Hu3DModelObjMtxGet(modelId, jointName, modelMatrix);
    jointPosition->x = modelMatrix[0][3];
    jointPosition->y = modelMatrix[1][3];
    jointPosition->z = modelMatrix[2][3];
}

void GetWorldModelJointPosition(s16 modelId, char *jointName, Vec *jointPosition, Mtx worldMatrix)
{
    Mtx worldModelMatrix;
    HU3DMODEL *modelData;

    modelData = &Hu3DData[modelId];
    Hu3DModelObjMtxGet(modelId, jointName, worldModelMatrix);
    MTXConcat(modelData->mtx, worldModelMatrix, worldModelMatrix);
    if (NULL != worldMatrix) {
        MTXCopy(worldModelMatrix, worldMatrix);
    }
    jointPosition->x = worldModelMatrix[0][3];
    jointPosition->y = worldModelMatrix[1][3];
    jointPosition->z = worldModelMatrix[2][3];
}

float InterpolateAngle(float currentAngle, float targetAngle, float progress)
{
    float angleDifference;
    float interpolatedAngle;

    angleDifference = fmod(targetAngle - currentAngle, 360.0);
    if (0.0f > angleDifference) {
        angleDifference += 360.0f;
    }
    if (180.0f < angleDifference) {
        angleDifference -= 360.0f;
    }
    interpolatedAngle = fmod(currentAngle + angleDifference * progress, 360.0);
    if (0.0f > interpolatedAngle) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

void PlaySequence(s32 sequenceResourceId)
{
    audioSequenceId = HuAudSeqPlay((s32)sequenceResourceId);
}

void FadeOutSequence(s32 fadeFrames)
{
    if (audioSequenceId >= 0) {
        if (fadeFrames < 0) {
            fadeFrames = 1000;
        }
        HuAudSeqFadeOut(audioSequenceId, fadeFrames);
    }
    audioSequenceId = -1;
}

void UpdatePlayerMovement(Vec *position, float *heading, s32 stickX, s32 stickY)
{
    Vec movementInput;
    float inputMagnitude;
    s32 movementMotion;

    movementInput.x = 0.0f;
    movementInput.y = 0.0f;
    movementInput.z = 0.0f;
    movementInput.x = stickX / 4.0f;
    movementInput.z = -(float)stickY / 4.0f;
    movementMotion = 0;
    inputMagnitude = VECMagXZ(&movementInput);
    if (0.5f <= inputMagnitude) {
        if (10.0f <= inputMagnitude) {
            movementInput.x /= inputMagnitude;
            movementInput.z /= inputMagnitude;
            inputMagnitude = 10.0f;
            movementInput.x = 10.0f * movementInput.x;
            movementInput.z = 10.0f * movementInput.z;
        }
        *heading = InterpolateAngle(*heading, atan2d(movementInput.x, movementInput.z), 0.4f);
        if (8.0f <= inputMagnitude) {
            movementMotion = 2;
        }
        else {
            movementMotion = 1;
        }
    }
    VECAdd((Vec *)&position, &movementInput, (Vec *)&position);
}

OMOBJ *mainControlObject;
OMOBJ **playerObjects;
OMOBJ **birdObjects;
OMOBJ **rewardObjects;
s32 activeRewardCount;
ANIMDATA *birdParticleAnimation;
s16 birdParticleManagerId;
ANIMDATA *rewardParticleAnimation;
s16 rewardParticleManagerId;
float minigameStateTimer;
float backgroundRotation;
s16 collectedRewardCount;
s16 activeRewardObjectCount;
s16 activeBirdCounts[3];
s16 birdModelIds[3];
s16 backgroundModelIds[1];
s32 initialBirdSpawnPending;
float cameraZoom;
float cameraHorizontalOffset;
float cameraVerticalOffset;

s16 characterModelIds[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

Vec initialCameraPosition = { 0, 100, 400 };
Vec initialCameraUp = { 0, 1, 0 };
Vec initialCameraTarget = { 0, 100, 0 };
Vec introCameraPosition = { 0, 1200, 2100 };
Vec introCameraUp = { 0, 1, 0 };
Vec introCameraTarget = { 0, 0, 0 };
Vec resultsCameraPosition = { 0, 200, 1000 };
Vec resultsCameraUp = { 0, 1, 0 };
Vec resultsCameraTarget = { 0, 150, 0 };
Vec lightPosition = { 0, 3000, 1000 };
Vec lightDirection = { 0, -3, -1 };
GXColor lightColor = { 255, 255, 255, 255 };
Vec shadowPosition = { 0, 3000, 1000 };
Vec shadowTarget = { 0, 0, 0 };

s32 characterMotionIds[8][16] = {
    {
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x08),
        DATA_MAKE_NUM(DATADIR_M441, 0x10),
        DATA_MAKE_NUM(DATADIR_M441, 0x18),
        DATA_MAKE_NUM(DATADIR_M441, 0x20),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x30),
        DATA_MAKE_NUM(DATADIR_M441, 0x28),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x01),
        DATA_MAKE_NUM(DATADIR_M441, 0x09),
        DATA_MAKE_NUM(DATADIR_M441, 0x11),
        DATA_MAKE_NUM(DATADIR_M441, 0x19),
        DATA_MAKE_NUM(DATADIR_M441, 0x21),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x31),
        DATA_MAKE_NUM(DATADIR_M441, 0x29),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x02),
        DATA_MAKE_NUM(DATADIR_M441, 0x0A),
        DATA_MAKE_NUM(DATADIR_M441, 0x12),
        DATA_MAKE_NUM(DATADIR_M441, 0x1A),
        DATA_MAKE_NUM(DATADIR_M441, 0x22),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x32),
        DATA_MAKE_NUM(DATADIR_M441, 0x2A),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x03),
        DATA_MAKE_NUM(DATADIR_M441, 0x0B),
        DATA_MAKE_NUM(DATADIR_M441, 0x13),
        DATA_MAKE_NUM(DATADIR_M441, 0x1B),
        DATA_MAKE_NUM(DATADIR_M441, 0x23),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x33),
        DATA_MAKE_NUM(DATADIR_M441, 0x2B),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x04),
        DATA_MAKE_NUM(DATADIR_M441, 0x0C),
        DATA_MAKE_NUM(DATADIR_M441, 0x14),
        DATA_MAKE_NUM(DATADIR_M441, 0x1C),
        DATA_MAKE_NUM(DATADIR_M441, 0x24),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x34),
        DATA_MAKE_NUM(DATADIR_M441, 0x2C),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x05),
        DATA_MAKE_NUM(DATADIR_M441, 0x0D),
        DATA_MAKE_NUM(DATADIR_M441, 0x15),
        DATA_MAKE_NUM(DATADIR_M441, 0x1D),
        DATA_MAKE_NUM(DATADIR_M441, 0x25),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x35),
        DATA_MAKE_NUM(DATADIR_M441, 0x2D),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x06),
        DATA_MAKE_NUM(DATADIR_M441, 0x0E),
        DATA_MAKE_NUM(DATADIR_M441, 0x16),
        DATA_MAKE_NUM(DATADIR_M441, 0x1E),
        DATA_MAKE_NUM(DATADIR_M441, 0x26),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x36),
        DATA_MAKE_NUM(DATADIR_M441, 0x2E),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x07),
        DATA_MAKE_NUM(DATADIR_M441, 0x0F),
        DATA_MAKE_NUM(DATADIR_M441, 0x17),
        DATA_MAKE_NUM(DATADIR_M441, 0x1F),
        DATA_MAKE_NUM(DATADIR_M441, 0x27),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_M441, 0x37),
        DATA_MAKE_NUM(DATADIR_M441, 0x2F),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x18),
    },
};

s32 birdMotionIds[2] = { DATA_MAKE_NUM(DATADIR_M441, 0x40), DATA_MAKE_NUM(DATADIR_M441, 0x41) };

s32 rewardMotionIds[3] = {
    DATA_MAKE_NUM(DATADIR_M441, 0x43),
    DATA_MAKE_NUM(DATADIR_M441, 0x44),
    DATA_MAKE_NUM(DATADIR_M441, 0x45),
};

s32 scoreSpriteResourceIds[4] = {
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x22),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x21),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x2F),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x2F),
};

float playerStartPositions[4][4] = { -350, 0, 350, 135, 350, 0, 350, 225, 350, 0, -350, 315, -350, 0, -350, 45 };

s32 birdModelResourceIds[3] = {
    DATA_MAKE_NUM(DATADIR_M441, 0x3D),
    DATA_MAKE_NUM(DATADIR_M441, 0x3C),
    DATA_MAKE_NUM(DATADIR_M441, 0x3B),
};

s16 birdSpawnLimits[3] = { 10, 5, 2 };
s16 rewardPointValues[3] = { 1, 2, 3 };
Vec birdModelScales[3] = { { 0.5f, 0.5f, 0.5f }, { 0.7f, 0.7f, 0.7f }, { 0.9f, 0.9f, 0.9f } };

float birdSpeedLimits[3][2] = { 0.5f, 4.0f, 0.5f, 4.0f, 2.0f, 6.0f };

s32 backgroundDecorationResourceIds[1] = { DATA_MAKE_NUM(DATADIR_M441, 0x3E) };

float easyAiParameters[3] = { 0.7f, 85.0f, 22.0f };
float mediumAiParameters[3] = { 0.8f, 70.0f, 27.0f };
float hardAiParameters[3] = { 0.9f, 55.0f, 32.0f };
float expertAiParameters[3] = { 1.0f, 40.0f, 37.0f };

HU3DPARMANPARAM birdTrailParticleParameters = { 20, { 0, 0 }, 1, 30, 0, { 0, -0.05f, 0 }, 1.0f, 0.99f, 20.0f, 0.98f, 4,
    { { 128, 128, 128, 255 }, { 255, 0, 255, 255 }, { 0, 255, 255, 255 }, { 255, 255, 255, 255 } },
    { { 128, 128, 128, 0 }, { 255, 0, 255, 0 }, { 0, 255, 255, 0 }, { 255, 255, 255, 0 } } };

HU3DPARMANPARAM collectedBirdParticleParameters = { 30, { 0, 0 }, 1, 0, 360, { 0, -0.05f, 0 }, 10.0f, 0.98f, 20.0f, 1.05f, 4,
    { { 128, 128, 128, 255 }, { 255, 0, 255, 255 }, { 0, 255, 255, 255 }, { 255, 255, 255, 255 } },
    { { 128, 128, 128, 0 }, { 255, 0, 255, 0 }, { 0, 255, 255, 0 }, { 255, 255, 255, 0 } } };

HU3DPARMANPARAM scoreParticleParameters = {
    60,
    { 0, 0 },
    1,
    0,
    360,
    { 0, -0.05f, 0 },
    10.0f,
    0.99f,
    30.0f,
    0.98f,
    1,
    { { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 } },
    { { 255, 255, 255, 0 }, { 255, 255, 255, 0 }, { 255, 255, 255, 0 }, { 255, 255, 255, 0 } },
};

HU3DPARMANPARAM rewardParticleParameters = {
    30,
    { 0, 0 },
    1,
    10,
    360,
    { 0, 0.1f, 0 },
    1.0f,
    1.0f,
    30.0f,
    1.05f,
    1,
    { { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 }, { 255, 255, 255, 255 } },
    { { 255, 255, 255, 0 }, { 255, 255, 255, 0 }, { 255, 255, 255, 0 }, { 255, 255, 255, 0 } },
};

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

void DebugCameraControl(OMOBJ *object)
{
    HuSysVWaitSet(1);
    if (HuPadBtn[0] & PAD_BUTTON_X) {
        roundTimerFrames = -1;
    }
    print8(24, 32, 1.5f, "COUNT:%d MODE:%d MES:%d", minigameFrameCounter, mainControlObject->work[0], sequenceId);
    print8(24, 48, 1.5f, "%f %f %f", cameraVerticalOffset, cameraZoom, cameraHorizontalOffset);
    print8(24, 64, 1.5f, "%x %x", HuMemHeapSizeGet(HEAP_HEAP) - HuMemUsedMallocSizeGet(HEAP_HEAP),
        HuMemHeapSizeGet(HEAP_MODEL) - HuMemUsedMallocSizeGet(HEAP_MODEL));
    cameraHorizontalOffset -= HuPadSubStkX[0] / 59.0f;
    cameraZoom -= 4.0f * (HuPadSubStkY[0] / 59.0f);
    cameraVerticalOffset -= (HuPadTrigL[0] / 150.0f) * 4.0f;
    cameraVerticalOffset += (HuPadTrigR[0] / 150.0f) * 4.0f;
}

typedef struct PlayerWork {
    float *aiParameters;
    Vec worldPosition;
    Vec movementVector;
    Vec resultPosition;
    Vec heldObjectPosition;
    Vec heldObjectRotation;
    Vec heldObjectScale;
    Vec rewardPosition;
    float heading;
    float inputMagnitude;
    float playerRadius;
    Vec stateTimers;
    Vec roamingTarget;
    Vec aiTargetPosition;
    float nearestTargetDistance;
    float aiDecisionTimer;
    s32 coinScore;
    s32 aiMovementState;
    s32 playerState;
    s32 motionIndex;
    s32 celebrationActive;
    s32 stickX;
    s32 stickY;
    s32 buttonDownFlags;
    s32 playerIndex;
    s32 characterId;
    s32 controllerPort;
    s32 placementScore;
    s32 finalPlacement;
    s32 remainingRewardCounts[3];
    s32 scoreSprites[4];
    s16 particleManagerId;
    float stateTimeout;
} PlayerWork;

void LimitPlayerMovementRadius(PlayerWork *playerWork)
{
    float horizontalDeltaX = playerWork->worldPosition.x;
    float horizontalDeltaZ = playerWork->worldPosition.z;
    float horizontalDistance = sqrtf((horizontalDeltaX * horizontalDeltaX) + (horizontalDeltaZ * horizontalDeltaZ));
    if (600.0f - playerWork->playerRadius < horizontalDistance) {
        horizontalDeltaX /= horizontalDistance;
        horizontalDeltaZ /= horizontalDistance;
        playerWork->worldPosition.x = horizontalDeltaX * (600.0f - playerWork->playerRadius);
        playerWork->worldPosition.z = horizontalDeltaZ * (600.0f - playerWork->playerRadius);
    }
}

void ResolvePlayerCollisions(PlayerWork *playerWork, s32 separateVerticalResults)
{
    s32 i;
    for (i = 0; i < 4; i++) {
        PlayerWork *otherPlayerWork = playerObjects[i]->data;
        if (playerWork->playerIndex != otherPlayerWork->playerIndex) {
            float separationX = playerWork->worldPosition.x - otherPlayerWork->worldPosition.x;
            float separationZ = playerWork->worldPosition.z - otherPlayerWork->worldPosition.z;
            float horizontalDistance = sqrtf((separationX * separationX) + (separationZ * separationZ));
            if (horizontalDistance < playerWork->playerRadius + otherPlayerWork->playerRadius) {
                if (separateVerticalResults && playerWork->resultPosition.z == otherPlayerWork->resultPosition.z) {
                    playerWork->resultPosition.z += 55.0f * ((0 > playerWork->worldPosition.z) ? -1 : 1);
                }
                if (0.0f == horizontalDistance) {
                    separationX = 0.0f;
                    separationZ = -1.0f;
                }
                else {
                    separationX /= horizontalDistance;
                    separationZ /= horizontalDistance;
                }
                playerWork->worldPosition.x = otherPlayerWork->worldPosition.x + (separationX * (playerWork->playerRadius + otherPlayerWork->playerRadius));
                playerWork->worldPosition.z = otherPlayerWork->worldPosition.z + (separationZ * (playerWork->playerRadius + otherPlayerWork->playerRadius));
            }
        }
    }
}

void UpdatePlayerMovementAndMotion(PlayerWork *playerWork, OMOBJ *playerObject, s32 useControllerInput)
{
    s32 motionIndex;
    if (!useControllerInput) {
        playerWork->movementVector.x = 0;
        playerWork->movementVector.y = 0;
        playerWork->movementVector.z = 0;
        playerWork->movementVector.x = playerWork->stickX / 4.0f;
        playerWork->movementVector.z = -((float)playerWork->stickY) / 4.0f;
    }
    motionIndex = 1;
    playerWork->inputMagnitude = VECMagXZ(&playerWork->movementVector);
    if (0.5f <= playerWork->inputMagnitude) {
        if (10.0f <= playerWork->inputMagnitude) {
            playerWork->movementVector.x /= playerWork->inputMagnitude;
            playerWork->movementVector.z /= playerWork->inputMagnitude;
            playerWork->inputMagnitude = 10.0f;
            playerWork->movementVector.x = 10.0f * playerWork->movementVector.x;
            playerWork->movementVector.z = 10.0f * playerWork->movementVector.z;
        }
        playerWork->heading = InterpolateAngle(playerWork->heading, atan2d(playerWork->movementVector.x, playerWork->movementVector.z), 0.4f);
        if (8.0f <= playerWork->inputMagnitude) {
            motionIndex = 3;
        }
        else {
            motionIndex = 2;
        }
    }
    VECAdd(&playerWork->worldPosition, &playerWork->movementVector, &playerWork->worldPosition);
    LimitPlayerMovementRadius(playerWork);
    ResolvePlayerCollisions(playerWork, useControllerInput);
    if (useControllerInput) {
        motionIndex += 6;
        if (motionIndex > 6) {
            motionIndex = 7;
        }
    }
    if (motionIndex >= 0 && playerWork->motionIndex != motionIndex) {
        playerWork->motionIndex = motionIndex;
        Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[playerWork->motionIndex], 0, 8, HU3D_MOTATTR_LOOP);
    }
}

void UpdateComputerPlayerMovement(PlayerWork *playerWork)
{
    float targetDirectionX;
    float targetDirectionZ;
    float targetDistance;
    float randomTargetAngle;
    float jumpChance;

    if (NULL == playerWork->aiParameters) {
        return;
    }
    targetDirectionX = 0;
    targetDirectionZ = 0;
    if (playerWork->aiMovementState == 0) {
        targetDirectionX = playerWork->aiTargetPosition.x - playerWork->worldPosition.x;
        targetDirectionZ = playerWork->aiTargetPosition.z - playerWork->worldPosition.z;
        targetDistance = sqrtf((targetDirectionX * targetDirectionX) + (targetDirectionZ * targetDirectionZ));
        if (10.0f <= targetDistance) {
            targetDirectionX /= targetDistance;
            targetDirectionZ /= targetDistance;
        }
        else {
            targetDirectionX = 0;
            targetDirectionZ = 0;
        }
        if (170.0f < targetDistance && 224.0f > targetDistance) {
            if (200.0f > playerWork->aiTargetPosition.y) {
                playerWork->buttonDownFlags |= PAD_BUTTON_B;
            }
            else {
                playerWork->buttonDownFlags |= PAD_BUTTON_A;
            }
            playerWork->aiMovementState = 1;
            playerWork->aiDecisionTimer = playerWork->aiParameters[1] + ((frandf() * playerWork->aiParameters[1]) / 2);
            jumpChance = 1.0f - (roundTimerFrames / (60.0f * REFRESH_RATE));
            if (0.25f < jumpChance) {
                playerWork->aiDecisionTimer *= playerWork->coinScore / (playerWork->aiParameters[2] * jumpChance);
            }
            if (playerWork->aiDecisionTimer < frandmod(100)) {
                playerWork->aiMovementState = 0;
            }
        }
    }
    else {
        if (playerWork->aiMovementState == 1) {
            playerWork->aiMovementState = 2;
            randomTargetAngle = 360.0f * frandf();
            targetDistance = (600.0f - playerWork->playerRadius) * frandf();
            playerWork->roamingTarget.x = targetDistance * sind(randomTargetAngle);
            playerWork->roamingTarget.y = 0;
            playerWork->roamingTarget.z = targetDistance * cosd(randomTargetAngle);
        }
        targetDirectionX = playerWork->roamingTarget.x - playerWork->worldPosition.x;
        targetDirectionZ = playerWork->roamingTarget.z - playerWork->worldPosition.z;
        targetDistance = sqrtf((targetDirectionX * targetDirectionX) + (targetDirectionZ * targetDirectionZ));
        if (10.0f <= targetDistance) {
            targetDirectionX /= targetDistance;
            targetDirectionZ /= targetDistance;
        }
        else {
            targetDirectionX = 0;
            targetDirectionZ = 0;
            playerWork->aiMovementState = 1;
        }
        playerWork->aiDecisionTimer--;
        if (0.0f > playerWork->aiDecisionTimer) {
            playerWork->aiMovementState = 0;
        }
    }
    playerWork->stickX = (72.0f * targetDirectionX) * playerWork->aiParameters[0];
    playerWork->stickY = (72.0f * -targetDirectionZ) * playerWork->aiParameters[0];
    if (ABS(playerWork->stickX) > 72) {
        playerWork->stickX = ((playerWork->stickX < 0) ? -1 : 1) * 72;
    }
    if (ABS(playerWork->stickY) > 72) {
        playerWork->stickY = ((playerWork->stickY < 0) ? -1 : 1) * 72;
    }
}

void UpdateJumpScale(PlayerWork *playerWork, OMOBJ *playerObject)
{
    playerWork->stateTimers.z++;
    if (10.0f >= playerWork->stateTimers.z) {
        float jumpScale = (0.3f * sind(180.0f * (playerWork->stateTimers.z / 10.0f))) + 1.5f;
        Hu3DModelScaleSet(playerObject->mdlId[2], jumpScale, jumpScale, jumpScale);
    }
}

void UpdatePlayer(OMOBJ *object);

void UpdateInteractivePlayer(OMOBJ *object)
{
    PlayerWork *work = object->data;
    Mtx hookMtx;
    Vec netOfs;
    if (work->playerState == 2004 && mainControlObject->work[0] >= 1005) {
        work->playerState = 2007;
        work->stateTimers.x = 20;
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[1], 0, 20, HU3D_MOTATTR_LOOP);
        object->objFunc = UpdatePlayer;
        return;
    }
    work->nearestTargetDistance = 10000;
    if (GWPlayerCfg[work->playerIndex].iscom) {
        work->stickX = 0;
        work->stickY = 0;
        work->buttonDownFlags = 0;
        UpdateComputerPlayerMovement(work);
    }
    else {
        work->stickX = HuPadStkX[work->controllerPort];
        work->stickY = HuPadStkY[work->controllerPort];
        work->buttonDownFlags = HuPadBtnDown[work->controllerPort];
    }
    switch (work->playerState) {
        case 2004:
            UpdatePlayerMovementAndMotion(work, object, 0);
            if (work->buttonDownFlags & PAD_BUTTON_A) {
                work->playerState = 2005;
                work->stateTimers.x = 10;
                work->stateTimeout = 0;
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[4], 0, 8, HU3D_MOTATTR_NONE);
                Hu3DMotionSet(object->mdlId[1], object->mtnId[11]);
                HuAudFXEmiterPlay(1814, &work->worldPosition);
            }
            if (work->buttonDownFlags & PAD_BUTTON_B) {
                work->playerState = 2005;
                work->stateTimers.x = 10;
                work->stateTimeout = 0;
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[5], 0, 8, HU3D_MOTATTR_NONE);
                Hu3DMotionSet(object->mdlId[1], object->mtnId[11]);
                HuAudFXEmiterPlay(1814, &work->worldPosition);
            }
            break;

        case 2005:
            work->stateTimers.x--;
            if (0.0f > work->stateTimers.x && 20.0f < Hu3DMotionTimeGet(object->mdlId[0])) {
                work->playerState++;
            }
            work->stateTimeout++;
            if (240.0f < work->stateTimeout) {
                work->playerState = 2006;
            }
            break;

        case 2006:
            work->stateTimeout++;
            if (240.0f < work->stateTimeout || Hu3DMotionEndCheck(object->mdlId[0])) {
                work->playerState = 2004;
                work->motionIndex = 1;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[1], 0, 8, HU3D_MOTATTR_LOOP);
            }
            break;

        default:
            OSReport("*** player mode error(%d)!!\n", work->playerState);
            break;
    }
    omSetTra(object, work->worldPosition.x, work->worldPosition.y, work->worldPosition.z);
    omSetRot(object, 0, work->heading, 0);
    netOfs.x = 0;
    netOfs.y = 0;
    netOfs.z = 170;
    Hu3DModelObjMtxGet(object->mdlId[0], CharModelItemHookGet(work->characterId, 2, 0), hookMtx);
    MTXMultVec(hookMtx, &netOfs, &work->heldObjectPosition);
}

static inline void SetVector(Vec *vector, float x, float y, float z)
{
    vector->x = x;
    vector->y = y;
    vector->z = z;
}

void CreateRewardObject(s16 playerIndex, s16 rewardType, Vec *rewardPosition);

void UpdatePlayer(OMOBJ *object)
{
    float angle;
    Mtx hookMtx;
    Vec pos3D;
    Vec pos2D;
    PlayerWork *work = object->data;
    s32 i;

    switch (work->playerState) {
        case 2000:
            if (mainControlObject->work[0] == 1002 && 90.0f < minigameStateTimer) {
                work->playerState++;
                work->stateTimers.x = 10.0f;
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[4], 0, 10, HU3D_MOTATTR_NONE);
                Hu3DMotionSet(object->mdlId[1], object->mtnId[11]);
                HuAudFXEmiterPlay(1814, &work->worldPosition);
            }
            break;

        case 2001:
            work->stateTimers.x--;
            if (0.0f > work->stateTimers.x && Hu3DMotionEndCheck(object->mdlId[0])) {
                work->playerState++;
                work->stateTimers.x = 10.0f;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[5], 0, 10, HU3D_MOTATTR_NONE);
                Hu3DMotionSet(object->mdlId[1], object->mtnId[11]);
                HuAudFXEmiterPlay(1814, &work->worldPosition);
            }
            break;

        case 2002:
            work->stateTimers.x--;
            if (0.0f > work->stateTimers.x && Hu3DMotionEndCheck(object->mdlId[0])) {
                work->playerState++;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[1], 0, 10, HU3D_MOTATTR_LOOP);
            }
            break;

        case 2003:
            if (mainControlObject->work[0] >= 1004) {
                work->playerState++;
                object->objFunc = UpdateInteractivePlayer;
            }
            break;

        case 2007:
            work->stateTimers.x--;
            if (0.0f > work->stateTimers.x && mainControlObject->work[0] == 1006) {
                work->playerState++;
                work->stateTimers.x = 40;
                Hu3DModelObjMtxGet(object->mdlId[0], CharModelItemHookGet(work->characterId, 2, 0), hookMtx);
                Hu3DMtxTransGet(hookMtx, &work->heldObjectPosition);
                Hu3DMtxRotGet(hookMtx, &work->heldObjectRotation);
                Hu3DMtxScaleGet(hookMtx, &work->heldObjectScale);
                Hu3DModelPosSet(object->mdlId[1], work->heldObjectPosition.x, work->heldObjectPosition.y, work->heldObjectPosition.z);
                Hu3DModelRotSet(object->mdlId[1], work->heldObjectRotation.x, work->heldObjectRotation.y, work->heldObjectRotation.z);
                Hu3DModelScaleSet(object->mdlId[1], work->heldObjectScale.x, work->heldObjectScale.y, work->heldObjectScale.z);
                Hu3DModelHookObjReset(object->mdlId[0], CharModelItemHookGet(work->characterId, 2, 0));
            }
            break;

        case 2008:
            work->heldObjectPosition.y += 4;
            work->heldObjectRotation.x += 24.0f;
            work->heldObjectScale.x *= 0.95f;
            work->heldObjectScale.y *= 0.95f;
            work->heldObjectScale.z *= 0.95f;
            Hu3DModelPosSet(object->mdlId[1], work->heldObjectPosition.x, work->heldObjectPosition.y, work->heldObjectPosition.z);
            Hu3DModelRotSet(object->mdlId[1], work->heldObjectRotation.x, work->heldObjectRotation.y, work->heldObjectRotation.z);
            Hu3DModelScaleSet(object->mdlId[1], work->heldObjectScale.x, work->heldObjectScale.y, work->heldObjectScale.z);
            work->stateTimers.x--;
            if (0.0f > work->stateTimers.x) {
                float playerResultX;
                work->playerState++;
                work->stateTimers.x = 0;
                playerResultX = (work->playerIndex * 160.0f) - 240.0f;
                work->resultPosition.x = playerResultX;
                work->resultPosition.y = 0;
                work->resultPosition.z = 0;
                Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
                work->particleManagerId = Hu3DParManLink(rewardParticleManagerId, &rewardParticleParameters);
                Hu3DParManPosSet(work->particleManagerId, work->heldObjectPosition.x, work->heldObjectPosition.y, work->heldObjectPosition.z);
                Hu3DParManAttrReset(work->particleManagerId, 1);
                Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(work->particleManagerId), 1);
                Hu3DModelLayerSet(Hu3DParManModelIDGet(work->particleManagerId), 4);
                HuAudFXPlay(1815);
            }
            break;

        case 2009:
            work->stateTimers.x++;
            if (10.0f == work->stateTimers.x) {
                Hu3DParManAttrSet(work->particleManagerId, 1);
            }
            work->movementVector.x = 0;
            work->movementVector.y = 0;
            work->movementVector.z = 0;
            work->movementVector.x = work->resultPosition.x - work->worldPosition.x;
            work->movementVector.z = work->resultPosition.z - work->worldPosition.z;
            if (10.0f < work->stateTimers.x && 0.0f == work->movementVector.x && 0.0f == work->movementVector.z && 0.0f == work->resultPosition.z) {
                work->playerState++;
                work->celebrationActive = 1;
            }
            else {
                UpdatePlayerMovementAndMotion(work, object, 1);
                if (0.0f != work->resultPosition.z) {
                    work->resultPosition.z -= ((0.0f > work->resultPosition.z) ? -1 : 1) * 5.0f;
                }
            }
            break;

        case 2010:
            work->heading = InterpolateAngle(work->heading, 0.0f, 0.5f);
            angle = fmod(-work->heading, 360.0f);
            if (-180.0f > angle) {
                angle += 360.0f;
            }
            if (work->celebrationActive) {
                if (8.0f > ABS(angle)) {
                    work->celebrationActive = 0;
                    Hu3DModelPosSet(object->mdlId[2], 0, 0, -10);
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[6], 0, 4, HU3D_MOTATTR_LOOP);
                }
            }
            if (mainControlObject->work[0] == 1007) {
                work->playerState++;
                work->stateTimers.x = 10.0f;
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[8], 0, 8, HU3D_MOTATTR_NONE);
                work->stateTimers.y = -90;
                SetVector(&pos3D, work->worldPosition.x, 250, work->worldPosition.z);
                Hu3D3Dto2D(&pos3D, 1, &pos2D);
                espTPLvlSet(work->scoreSprites[0], 0.5f);
                espPosSet(work->scoreSprites[0], pos2D.x, pos2D.y);
                espPosSet(work->scoreSprites[1], pos2D.x, pos2D.y);
                espPosSet(work->scoreSprites[2], pos2D.x - 14, 10 + pos2D.y);
                espPosSet(work->scoreSprites[3], (pos2D.x - 14) + 28, 10 + pos2D.y);
                for (i = 0; i < 4; i++) {
                    espScaleSet(work->scoreSprites[i], 0.1f, 0.1f);
                    espBankSet(work->scoreSprites[i], 0);
                    espDispOn(work->scoreSprites[i]);
                }
                work->particleManagerId = Hu3DParManLink(birdParticleManagerId, &scoreParticleParameters);
                Hu3DParManPosSet(work->particleManagerId, pos3D.x, pos3D.y, pos3D.z);
            }
            break;

        case 2011:
            work->stateTimers.y += 18.0f;
            if (180.0f >= work->stateTimers.y) {
                for (i = 0; i < 4; i++) {
                    espScaleSet(work->scoreSprites[i], sind(work->stateTimers.y) + 1, sind(work->stateTimers.y) + 1);
                }
            }
            work->stateTimers.x--;
            if (0.0f > work->stateTimers.x) {
                float motTime = Hu3DMotionTimeGet(object->mdlId[0]);
                if (18.0f == motTime) {
                    Hu3DParManAttrSet(work->particleManagerId, 1);
                    Hu3DModelPosSet(object->mdlId[2], 0, 0, 0);
                    Hu3DModelHookObjReset(object->mdlId[0], CharModelItemHookGet(work->characterId, 2, 4));
                    Hu3DModelHookSet(object->mdlId[0], CharModelItemHookGet(work->characterId, 2, 0), object->mdlId[2]);
                }
                if (40.0f == motTime) {
                    work->stateTimers.z = 0;
                    Hu3DModelHookObjReset(object->mdlId[0], CharModelItemHookGet(work->characterId, 2, 0));
                    GetModelJointPosition(object->mdlId[0], CharModelItemHookGet(work->characterId, 2, 0), &work->rewardPosition);
                    work->rewardPosition.z += 20.0f;
                    Hu3DModelPosSet(object->mdlId[2], work->rewardPosition.x, work->rewardPosition.y, work->rewardPosition.z);
                }
                if (40.0f <= motTime) {
                    work->stateTimers.z++;
                    if (12.0f >= work->stateTimers.z) {
                        float scale = sind(150.0f * (work->stateTimers.z / 12.0f)) + 1.0;
                        Hu3DModelScaleSet(object->mdlId[2], scale, scale, scale);
                    }
                }
                if (Hu3DMotionEndCheck(object->mdlId[0])) {
                    work->playerState++;
                    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
                    Hu3DMotionSet(object->mdlId[0], object->mtnId[0]);
                    Hu3DMotionSet(object->mdlId[2], object->mtnId[14]);
                    for (i = 0; i < 4; i++) {
                        espScaleSet(work->scoreSprites[i], 1.0f, 1.0f);
                    }
                }
            }
            break;

        case 2012:
            if (Hu3DMotionEndCheck(object->mdlId[2])) {
                work->playerState++;
                work->stateTimers.x = 0;
            }
            break;

        case 2013:
            work->stateTimers.x--;
            if (0.0f > work->stateTimers.x) {
                s32 choType;
                work->stateTimers.x = (20.0f * frandf()) + 20.0f;
                choType = -1;
                for (i = 0; i < 3; i++) {
                    if (choType < 0 && work->remainingRewardCounts[i] > 0) {
                        choType = i;
                    }
                }
                if (choType < 0) {
                    work->playerState++;
                    work->stateTimers.x = 0;
                }
                else {
                    collectedRewardCount++;
                    work->placementScore += rewardPointValues[choType];
                    if (work->placementScore > 99) {
                        work->placementScore = 99;
                    }
                    work->remainingRewardCounts[choType]--;
                    CreateRewardObject(work->playerIndex, choType, &work->rewardPosition);
                    HuAudFXPlay(1816);
                    espBankSet(work->scoreSprites[2], work->placementScore / 10);
                    espBankSet(work->scoreSprites[3], work->placementScore % 10);
                    work->stateTimers.z = 0;
                }
            }
            break;

        case 2014:
            work->stateTimers.x++;
            if (30.0f == work->stateTimers.x) {
                Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
                work->particleManagerId = Hu3DParManLink(rewardParticleManagerId, &rewardParticleParameters);
                Hu3DParManPosSet(work->particleManagerId, work->rewardPosition.x, work->rewardPosition.y + 20, work->rewardPosition.z);
                Hu3DParManAttrReset(work->particleManagerId, 1);
                Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(work->particleManagerId), 1);
                Hu3DModelLayerSet(Hu3DParManModelIDGet(work->particleManagerId), 4);
                HuAudFXPlay(1815);
            }
            if (40.0f == work->stateTimers.x) {
                Hu3DParManAttrSet(work->particleManagerId, 1);
            }
            if (60.0f < work->stateTimers.x && work->finalPlacement >= 0) {
                work->playerState++;
                if (work->finalPlacement == 1) {
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[9], 0, 20, HU3D_MOTATTR_NONE);
                }
                else {
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[10], 0, 20, HU3D_MOTATTR_NONE);
                }
            }
            break;

        default:
            OSReport("*** player mode error(%d)!!\n", work->playerState);
            break;

        case 2015:
            break;
    }
    omSetTra(object, work->worldPosition.x, work->worldPosition.y, work->worldPosition.z);
    omSetRot(object, 0, work->heading, 0);
}

void InitializePlayer(OMOBJ *object)
{
    PlayerWork *work = object->data;
    s32 i;

    work->aiParameters = NULL;
    work->nearestTargetDistance = 10000;
    work->aiDecisionTimer = 0;
    work->coinScore = 0;
    work->aiMovementState = 0;
    work->aiTargetPosition.x = 0;
    work->aiTargetPosition.y = 0;
    work->aiTargetPosition.z = 0;
    work->playerIndex = object->work[0];
    work->characterId = GWPlayerCfg[work->playerIndex].character;
    work->controllerPort = GWPlayerCfg[work->playerIndex].pad_idx;
    work->playerState = 2000;
    work->placementScore = 0;
    work->finalPlacement = -1;
    for (i = 0; i < 3; i++) {
        work->remainingRewardCounts[i] = 0;
    }
    work->motionIndex = 1;
    work->playerRadius = 50;
    work->worldPosition.x = playerStartPositions[work->playerIndex][0];
    work->worldPosition.y = playerStartPositions[work->playerIndex][1];
    work->worldPosition.z = playerStartPositions[work->playerIndex][2];
    work->heading = playerStartPositions[work->playerIndex][3];
    work->rewardPosition.x = 0;
    work->rewardPosition.y = 0;
    work->rewardPosition.z = 0;
    object->mdlId[0] = CharModelCreate(characterModelIds[work->characterId], 2);
    CharModelStepFxSet(characterModelIds[work->characterId], 0);
    for (i = 0; i < 16; i++) {
        object->mtnId[i] = CharMotionCreate(characterModelIds[work->characterId], characterMotionIds[work->characterId][i]);
        CharMotionSet(characterModelIds[work->characterId], object->mtnId[i]);
    }
    CharMotionNoSet(work->characterId, object->mtnId[2], DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02));
    CharMotionNoSet(work->characterId, object->mtnId[7], DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02));
    CharMotionNoSet(work->characterId, object->mtnId[3], DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03));
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[0], 2);
    Hu3DMotionSet(object->mdlId[0], object->mtnId[1]);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M441, 0x3F));
    for (i = 11; i <= 12; i++) {
        object->mtnId[i] = Hu3DJointMotionFile(object->mdlId[1], birdMotionIds[i - 11]);
    }
    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[1], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[1], 2);
    Hu3DModelHookSet(object->mdlId[0], CharModelItemHookGet(work->characterId, 2, 0), object->mdlId[1]);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M441, 0x42));
    for (i = 13; i <= 15; i++) {
        object->mtnId[i] = Hu3DJointMotionFile(object->mdlId[2], rewardMotionIds[i - 13]);
    }
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[2], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[2], 2);
    Hu3DModelHookSet(object->mdlId[0], CharModelItemHookGet(work->characterId, 2, 4), object->mdlId[2]);
    for (i = 0; i < 4; i++) {
        work->scoreSprites[i] = espEntry(scoreSpriteResourceIds[i], 0, 0);
        espDrawNoSet(work->scoreSprites[i], 0x40);
        espDispOff(work->scoreSprites[i]);
        espAttrSet(work->scoreSprites[i], HUSPR_ATTR_NOANIM);
        espBankSet(work->scoreSprites[i], 0);
        espPriSet(work->scoreSprites[i], 128 - i);
        espPosSet(work->scoreSprites[i], 0, 0);
        if (i == 0) {
            espColorSet(work->scoreSprites[i], 0, 0, 0);
        }
    }
    omSetTra(object, work->worldPosition.x, work->worldPosition.y, work->worldPosition.z);
    omSetRot(object, 0, work->heading, 0);
    for (i = 0; i < 3; i++) {
        Hu3DModelShadowSet(object->mdlId[i]);
    }
    for (i = 0; i < 3; i++) {
        Hu3DModelAttrReset(object->mdlId[i], HU3D_ATTR_DISPOFF);
    }
    if (GWPlayerCfg[work->playerIndex].iscom) {
        switch (GWPlayerCfg[work->playerIndex].diff) {
            case 0:
                work->aiParameters = easyAiParameters;
                break;

            case 1:
                work->aiParameters = mediumAiParameters;
                break;

            case 2:
                work->aiParameters = hardAiParameters;
                break;

            case 3:
            default:
                work->aiParameters = expertAiParameters;
                break;
        }
    }
    CharModelLayerSetAll2(6);
    CharMotionDataClose(characterModelIds[work->characterId]);
    object->objFunc = UpdatePlayer;
}

typedef struct BirdRewardWork {
    Vec worldPosition;
    Vec destinationPosition;
    Vec flightMotion;
    Vec flightTimersAndTargetHeight;
    s16 particleManagerId;
    s32 returningToCenter;
    s32 collectedByPlayer;
} BirdRewardWork;

void DeleteBirdObject(OMOBJ *object)
{
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    RegisterModelResource(object->work[0] + 4096, object->mdlId[0]);
    activeBirdCounts[object->work[0]]--;
    Hu3DParManKill(((BirdRewardWork *)object->data)->particleManagerId);
    omDelObjEx(objectManager, object);
}

void CreateBirdObject(s32 returningToCenter);

void UpdateBirdObject(OMOBJ *object)
{
    BirdRewardWork *birdWork;
    PlayerWork *playerWork;
    OMOBJ *selectedPlayerObject;
    float *birdSpeedLimits;

    s32 playerIndex;
    s32 selectedPlayerIndex;
    float directionX;
    float directionZ;
    float verticalDistance;
    float distanceToTarget;
    float motionTime;
    birdWork = object->data;
    if (mainControlObject->work[0] < 1002) {
        return;
    }
    if (birdWork->collectedByPlayer) {
        if (mainControlObject->work[0] >= 1006) {
            DeleteBirdObject(object);
        }
        else {
            birdWork->flightTimersAndTargetHeight.z--;
            if (20.0f == birdWork->flightTimersAndTargetHeight.z) {
                Hu3DParManAttrSet(birdWork->particleManagerId, 1);
            }
            if (0.0f > birdWork->flightTimersAndTargetHeight.z) {
                DeleteBirdObject(object);
                CreateBirdObject(1);
            }
        }
        return;
    }
    else {
        if (mainControlObject->work[0] >= 1006) {
            birdWork->flightMotion.y += 0.1f;
            birdWork->flightMotion.x = InterpolateAngle(birdWork->flightMotion.x, (180.0f > birdWork->flightMotion.x) ? 90.0f : 270.0f, 0.05f);
            birdWork->worldPosition.x += birdWork->flightMotion.y * sind(birdWork->flightMotion.x);
            birdWork->worldPosition.z += birdWork->flightMotion.y * cosd(birdWork->flightMotion.x);
            if (1200.0f < ABS(birdWork->worldPosition.x)) {
                DeleteBirdObject(object);
                return;
            }
        }
        else {
            if (birdWork->returningToCenter) {
                directionX = -birdWork->worldPosition.x;
                directionZ = -birdWork->worldPosition.z;
                birdWork->flightMotion.x = InterpolateAngle(birdWork->flightMotion.x, atan2d(directionX, directionZ), 0.05f);
                birdWork->worldPosition.x += birdWork->flightMotion.y * sind(birdWork->flightMotion.x);
                birdWork->worldPosition.z += birdWork->flightMotion.y * cosd(birdWork->flightMotion.x);
                directionX = -birdWork->worldPosition.x;
                directionZ = -birdWork->worldPosition.z;
                distanceToTarget = sqrtf((directionX * directionX) + (directionZ * directionZ));
                if (600.0f >= distanceToTarget) {
                    birdWork->returningToCenter = 0;
                }
            }
            else {
                birdWork->flightMotion.x += ((frandf() * 2.0f) - 1.0f) * 3.0f;
                birdWork->flightMotion.y += ((frandf() * 2.0f) - 1.0f) * 0.1f;
                birdSpeedLimits = &birdSpeedLimits[object->work[0]][0];
                if (birdSpeedLimits[0] > birdWork->flightMotion.y) {
                    birdWork->flightMotion.y = birdSpeedLimits[0];
                }
                if (birdSpeedLimits[1] < birdWork->flightMotion.y) {
                    birdWork->flightMotion.y = birdSpeedLimits[1];
                }
                birdWork->worldPosition.x += birdWork->flightMotion.y * sind(birdWork->flightMotion.x);
                birdWork->worldPosition.z += birdWork->flightMotion.y * cosd(birdWork->flightMotion.x);
                directionX = -birdWork->worldPosition.x;
                directionZ = -birdWork->worldPosition.z;
                distanceToTarget = sqrtf((directionX * directionX) + (directionZ * directionZ));
                if (600.0f < distanceToTarget) {
                    birdWork->flightMotion.x = InterpolateAngle(birdWork->flightMotion.x, atan2d(directionX, directionZ), 0.05f);
                }
            }
        }
        distanceToTarget = ABS(birdWork->flightTimersAndTargetHeight.y - birdWork->worldPosition.y);
        if (2.0f > distanceToTarget) {
            birdWork->flightTimersAndTargetHeight.x--;
            if (0.0f > birdWork->flightTimersAndTargetHeight.x) {
                birdWork->flightTimersAndTargetHeight.x = 1.0f * REFRESH_RATE;
                if (frandmod(100) > 50) {
                    birdWork->flightTimersAndTargetHeight.y = (frandf() * 220.0f) + 80.0f;
                }
            }
        }
        else {
            birdWork->worldPosition.y += (birdWork->flightTimersAndTargetHeight.y > birdWork->worldPosition.y) ? 1.0f : -1.0f;
        }
        birdWork->flightMotion.z += (frandf() * 4.0f) + 4.0f;
        omSetTra(object, birdWork->worldPosition.x, (sind(birdWork->flightMotion.z) * 10.0) + birdWork->worldPosition.y, birdWork->worldPosition.z);
        omSetRot(object, 0, birdWork->flightMotion.x + 180.0f, 0);
        if (birdWork->particleManagerId >= 0) {
            directionX = sind(birdWork->flightMotion.x + 180.0f) * 30;
            directionZ = cosd(birdWork->flightMotion.x + 180.0f) * 30;
            Hu3DParManPosSet(birdWork->particleManagerId, birdWork->worldPosition.x + directionX, birdWork->worldPosition.y, birdWork->worldPosition.z + directionZ);
            Hu3DParManVecSet(
                birdWork->particleManagerId, birdWork->flightMotion.y * sind(birdWork->flightMotion.x + 180.0f), 0, birdWork->flightMotion.y * cosd(birdWork->flightMotion.x + 180.0f));
        }
        selectedPlayerIndex = -1;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerWork = playerObjects[playerIndex]->data;
            if (600.0f > VECMagXZ(&birdWork->worldPosition)) {
                directionX = playerWork->worldPosition.x - birdWork->worldPosition.x;
                directionZ = playerWork->worldPosition.z - birdWork->worldPosition.z;
                distanceToTarget = sqrtf((directionX * directionX) + (directionZ * directionZ)) - 170;
                if (0.0f < distanceToTarget && playerWork->nearestTargetDistance > distanceToTarget) {
                    playerWork->nearestTargetDistance = distanceToTarget;
                    playerWork->aiTargetPosition = birdWork->worldPosition;
                }
            }
            if (playerWork->playerState == 2005) {
                directionX = playerWork->heldObjectPosition.x - birdWork->worldPosition.x;
                verticalDistance = playerWork->heldObjectPosition.y - birdWork->worldPosition.y;
                directionZ = playerWork->heldObjectPosition.z - birdWork->worldPosition.z;
                distanceToTarget = VECMagPoint(directionX, verticalDistance, directionZ);
                if (60.0f > distanceToTarget) {
                    if (selectedPlayerIndex < 0) {
                        selectedPlayerIndex = playerIndex;
                    }
                    else if (frandmod(100) > 50) {
                        selectedPlayerIndex = playerIndex;
                    }
                }
            }
        }
        if (selectedPlayerIndex >= 0) {
            birdWork->collectedByPlayer = 1;
            birdWork->flightTimersAndTargetHeight.z = 30;
            Hu3DParManKill(birdWork->particleManagerId);
            birdWork->particleManagerId = Hu3DParManLink(birdParticleManagerId, &collectedBirdParticleParameters);
            Hu3DParManPosSet(birdWork->particleManagerId, birdWork->worldPosition.x, birdWork->worldPosition.y, birdWork->worldPosition.z);
            Hu3DParManColorSet(birdWork->particleManagerId, object->work[0]);
            Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
            selectedPlayerObject = playerObjects[selectedPlayerIndex];
            playerWork = selectedPlayerObject->data;
            playerWork->remainingRewardCounts[object->work[0]]++;
            playerWork->coinScore += rewardPointValues[object->work[0]];
            motionTime = Hu3DMotionTimeGet(selectedPlayerObject->mdlId[1]);
            Hu3DMotionSet(selectedPlayerObject->mdlId[1], selectedPlayerObject->mtnId[12]);
            Hu3DMotionTimeSet(selectedPlayerObject->mdlId[1], motionTime);
            Hu3DMotionSet(selectedPlayerObject->mdlId[2], selectedPlayerObject->mtnId[13]);
            omVibrate(playerWork->playerIndex, 12, 6, 6);
            HuAudFXEmiterPlay(1813, &birdWork->worldPosition);
        }
    }
}

void InitializeBirdObject(OMOBJ *object)
{
    BirdRewardWork *work = object->data;
    s16 cachedModelId = TakeModelResource(object->work[0] + 4096);
    Vec *scale;
    if (cachedModelId < 0) {
        object->mdlId[0] = Hu3DModelLink(birdModelIds[object->work[0]]);
    }
    else {
        object->mdlId[0] = cachedModelId;
    }
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[0], 3);
    Hu3DModelShadowSet(object->mdlId[0]);
    scale = &birdModelScales[object->work[0]];
    omSetTra(object, work->worldPosition.x, work->worldPosition.y, work->worldPosition.z);
    omSetRot(object, 0, work->flightMotion.x + 180, 0);
    omSetSca(object, scale->x, scale->y, scale->z);
    work->particleManagerId = Hu3DParManLink(birdParticleManagerId, &birdTrailParticleParameters);
    if (work->particleManagerId >= 0) {
        float x = sind(work->flightMotion.x + 180) * 30.0;
        float y = cosd(work->flightMotion.x + 180) * 30.0;
        Hu3DParManPosSet(work->particleManagerId, work->worldPosition.x + x, work->worldPosition.y, work->worldPosition.z + y);
        Hu3DParManVecSet(work->particleManagerId, work->flightMotion.y * sind(work->flightMotion.x + 180.0f), 0, work->flightMotion.y * cosd(work->flightMotion.x + 180.0f));
        Hu3DParManAttrSet(work->particleManagerId, 0x804);
        Hu3DParManAttrReset(work->particleManagerId, 1);
        Hu3DParManColorSet(work->particleManagerId, object->work[0]);
        Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(work->particleManagerId), 1);
        Hu3DModelLayerSet(Hu3DParManModelIDGet(work->particleManagerId), 4);
    }
    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    object->objFunc = UpdateBirdObject;
}

void CreateBirdObject(s32 returningToCenter)
{
    s16 availableBirdType = -1;
    s16 i;
    OMOBJ *object;
    BirdRewardWork *work;
    float radius;
    for (i = 0; i < 3; i++) {
        if (birdSpawnLimits[i] > activeBirdCounts[i]) {
            availableBirdType = i;
        }
    }
    if (availableBirdType < 0) {
        return;
    }
    activeBirdCounts[availableBirdType]++;
    object = omAddObjEx(objectManager, 101, 1, 0, 1, InitializeBirdObject);
    object->work[0] = availableBirdType;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(BirdRewardWork), HU_MEMNUM_OVL);
    work = object->data;
    work->returningToCenter = returningToCenter;
    work->flightMotion.z = 0;
    work->collectedByPlayer = 0;
    work->flightMotion.y = 2;
    work->flightTimersAndTargetHeight.x = (frandf() * (3.0f * REFRESH_RATE)) + (3.0f * REFRESH_RATE);
    if (returningToCenter) {
        work->flightMotion.y = 3;
        work->flightMotion.x = 180.0f - (frandf() * 120.0f);
        if (frandmod(100) > 50) {
            work->flightMotion.x = 360.0f - work->flightMotion.x;
        }
        radius = 1200;
        work->worldPosition.x = radius * sind(work->flightMotion.x + 180.0f);
        work->worldPosition.y = (frandf() * 220.0f) + 80.0f;
        work->worldPosition.z = radius * cosd(work->flightMotion.x + 180.0f);
    }
    else {
        if (initialBirdSpawnPending) {
            initialBirdSpawnPending = 0;
            work->flightMotion.x = 45.0f + (frandf() * 90.0f);
            if (frandmod(100) > 50) {
                work->flightMotion.x += 180.0f;
            }
            work->worldPosition.x = 0;
            work->worldPosition.y = 80;
            work->worldPosition.z = 0;
        }
        else {
        repeat:
            radius = 300;
            radius += radius * frandf();
            work->flightMotion.x = 360.0f * frandf();
            work->worldPosition.x = radius * sind(work->flightMotion.x);
            work->worldPosition.y = (frandf() * 220.0f) + 80.0f;
            work->worldPosition.z = radius * cosd(work->flightMotion.x);
            if (0.0f < work->worldPosition.y) {
                if (150.0f > ABS(work->worldPosition.x)) {
                    goto repeat;
                }
            }
        }
    }
    work->flightTimersAndTargetHeight.y = work->worldPosition.y;
}

void DeleteRewardObject(OMOBJ *object)
{
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    RegisterModelResource(object->work[0] + 4096, object->mdlId[0]);
    Hu3DParManKill(((BirdRewardWork *)object->data)->particleManagerId);
    omDelObjEx(objectManager, object);
    activeRewardObjectCount--;
}

void UpdateRewardFlight(OMOBJ *object)
{
    BirdRewardWork *work = object->data;
    float x = work->destinationPosition.x - work->worldPosition.x;
    float y = work->destinationPosition.y - work->worldPosition.y;
    float len = sqrtf((x * x) + (y * y));
    if (10.0f > len) {
        work->flightMotion.y--;
        if (0.0f > work->flightMotion.y) {
            work->flightMotion.y = 0;
        }
        work->flightMotion.x = InterpolateAngle(work->flightMotion.x, 0.0f, 0.05f);
    }
    else {
        work->flightMotion.y += 0.1f;
        if (3.0f < work->flightMotion.y) {
            work->flightMotion.y = 3.0f;
        }
        work->flightMotion.x = InterpolateAngle(work->flightMotion.x, atan2d(-x, y), 0.05f);
    }
    work->worldPosition.x -= work->flightMotion.y * sind(work->flightMotion.x);
    work->worldPosition.y += work->flightMotion.y * cosd(work->flightMotion.x);
    omSetTra(object, work->worldPosition.x, work->worldPosition.y, work->worldPosition.z);
    omSetRot(object, 90, 0, work->flightMotion.x);
    if (work->particleManagerId >= 0) {
        Hu3DParManPosSet(work->particleManagerId, work->worldPosition.x, work->worldPosition.y, work->worldPosition.z);
        Hu3DParManVecSet(work->particleManagerId, -2 * sind(work->flightMotion.x + 180.0f), 2 * cosd(work->flightMotion.x + 180.0f), 0);
    }
}

void UpdateRewardRise(OMOBJ *object)
{
    BirdRewardWork *work = object->data;
    float x;
    float y;
    work->flightMotion.y -= 0.1f;
    if (2.0f > work->flightMotion.y) {
        work->flightMotion.y = 2.0f;
        if (work->destinationPosition.y > work->worldPosition.y) {
            x = work->destinationPosition.x - work->worldPosition.x;
            y = work->destinationPosition.y - work->worldPosition.y;
            work->flightMotion.x = InterpolateAngle(work->flightMotion.x, atan2d(-x, y), 0.05f);
        }
    }

    work->worldPosition.x -= work->flightMotion.y * sind(work->flightMotion.x);
    work->worldPosition.y += work->flightMotion.y * cosd(work->flightMotion.x);
    omSetTra(object, work->worldPosition.x, work->worldPosition.y, work->worldPosition.z);
    omSetRot(object, 90, 0, work->flightMotion.x);
    if (work->particleManagerId >= 0) {
        x = sind(work->flightMotion.x + 180) * -20.0;
        y = cosd(work->flightMotion.x + 180) * 20.0;
        Hu3DParManPosSet(work->particleManagerId, work->worldPosition.x + x, work->worldPosition.y + y, work->worldPosition.z);
        Hu3DParManVecSet(work->particleManagerId, -work->flightMotion.y * sind(work->flightMotion.x + 180.0f), work->flightMotion.y * cosd(work->flightMotion.x + 180.0f), 0);
    }
    if (450.0f < work->worldPosition.y) {
        DeleteRewardObject(object);
    }
}

void InitializeRewardObject(OMOBJ *object)
{
    BirdRewardWork *work;
    s16 cachedModelId = TakeModelResource(object->work[0] + 4096);
    Vec *scale;
    if (cachedModelId < 0) {
        object->mdlId[0] = Hu3DModelLink(birdModelIds[object->work[0]]);
    }
    else {
        object->mdlId[0] = cachedModelId;
    }
    work = object->data;
    work->flightMotion.y = 6.0f;
    work->flightMotion.x = (frandf() * 60.0f) - 30.0f;
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[0], 3);
    Hu3DModelShadowSet(object->mdlId[0]);
    scale = &birdModelScales[object->work[0]];
    omSetTra(object, work->worldPosition.x, work->worldPosition.y, work->worldPosition.z);
    omSetRot(object, 90, 0, work->flightMotion.x);
    omSetSca(object, scale->x * 0.5f, scale->y * 0.5f, scale->z * 0.5f);
    work->particleManagerId = Hu3DParManLink(birdParticleManagerId, &birdTrailParticleParameters);
    if (work->particleManagerId >= 0) {
        float x = sind(work->flightMotion.x + 180) * -20.0;
        float y = cosd(work->flightMotion.x + 180) * 20.0;
        Hu3DParManPosSet(work->particleManagerId, work->worldPosition.x + x, work->worldPosition.y + y, work->worldPosition.z);
        Hu3DParManVecSet(work->particleManagerId, -work->flightMotion.y * sind(work->flightMotion.x + 180.0f), work->flightMotion.y * cosd(work->flightMotion.x + 180.0f), 0);
        Hu3DParManAttrSet(work->particleManagerId, 0x804);
        Hu3DParManAttrReset(work->particleManagerId, 1);
        Hu3DParManColorSet(work->particleManagerId, object->work[0]);
        Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(work->particleManagerId), 1);
        Hu3DModelLayerSet(Hu3DParManModelIDGet(work->particleManagerId), 4);
    }
    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    object->objFunc = UpdateRewardRise;
}

void CreateRewardObject(s16 playerIndex, s16 rewardType, Vec *rewardPosition)
{
    OMOBJ *object;
    BirdRewardWork *work;
    float x;
    float y;
    float z;
    if (activeRewardObjectCount >= 400) {
        OSReport("over!!\n");
        return;
    }
    activeRewardObjectCount++;
    object = omAddObjEx(objectManager, 102, 1, 0, 2, InitializeRewardObject);
    object->work[0] = rewardType;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(BirdRewardWork), HU_MEMNUM_OVL);
    work = object->data;
    SetVector(&work->worldPosition, rewardPosition->x, rewardPosition->y + 10, rewardPosition->z + 8);
    SetVector(&work->destinationPosition, rewardPosition->x, 250, rewardPosition->z);
}

s32 CreateBackgroundModel(s32 decorationIndex)
{
    float angle = 12.0f * (decorationIndex % 30);
    float radius = 730.0f + (60.0f * (decorationIndex / 30));
    if (decorationIndex / 30 >= 2 && (100.0f > angle || 260.0f < angle)) {
        return 0;
    }
    else {
        s16 model = Hu3DModelLink(backgroundModelIds[0]);
        Hu3DModelAttrSet(model, HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
        Hu3DModelAttrSet(model, HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
        Hu3DModelLayerSet(model, 1);
        Hu3DMotionTimeSet(model, Hu3DMotionMaxTimeGet(model) * frandf());
        Hu3DModelPosSet(model, ((radius * sind(angle)) + (frandf() * 20.0f)) - 10, -60.0f, radius * cosd(angle));
        Hu3DModelScaleSet(model, 2, 2, 2);
        Hu3DModelAttrReset(model, HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(model, HU3D_ATTR_DISPOFF);
        return 1;
    }
}

void ExitMinigame(OMOBJ *object);

void UpdateMinigame(OMOBJ *object)
{
    Vec cameraForward;
    s32 i;

    if (omSysExitReq) {
        FadeOutSequence(-1);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = ExitMinigame;
    }
    backgroundRotation += (1.0f / REFRESH_RATE);
    if (360.0f <= backgroundRotation) {
        backgroundRotation -= 360.0f;
    }
    Hu3DModelRotSet(object->mdlId[2], 0, backgroundRotation, 0);
    minigameFrameCounter++;
    switch (object->work[0]) {
        case 1000:
            if (!WipeStatGet()) {
                object->work[0]++;
                minigameStateTimer = 0;
            }
            break;

        case 1001:
            minigameStateTimer++;
            if (60.0f < minigameStateTimer) {
                object->work[0]++;
                minigameStateTimer = 0;
                cameraTargetPosition = introCameraPosition;
                cameraTargetUp = introCameraTarget;
                VECSubtract(&cameraTargetUp, &cameraTargetPosition, &cameraForward);
                VECNormalize(&cameraForward, &cameraForward);
                HuAudFXListnerSetEX(&cameraTargetPosition, &cameraForward, 10000.0, 36000.0, 0.0, 300.0, 300.0);
            }
            break;

        case 1002:
            minigameStateTimer++;
            UpdateCameraTransition(minigameStateTimer / (3.0f * REFRESH_RATE), &introCameraUp);
            if ((3.0f * REFRESH_RATE) <= minigameStateTimer) {
                object->work[0]++;
                SetCameraView(&introCameraPosition, &introCameraUp, &introCameraTarget);
            }

            break;

        case 1003:
            if (sequenceId < 0) {
                sequenceId = MGSeqStartCreate();
                Hu3DModelAttrSet(mainControlObject->mdlId[2], HU3D_ATTR_DISPOFF);
            }
            else {
                if (audioSequenceId < 0 && (MGSeqStatGet(sequenceId) & 0x10)) {
                    PlaySequence(69);
                }
                if (!MGSeqStatGet(sequenceId)) {
                    object->work[0]++;
                    roundTimerFrames = 60 * REFRESH_RATE;
                    timerSequenceId = MGSeqTimerCreate(roundTimerFrames / REFRESH_RATE);
                    sequenceId = -1;
                }
            }
            break;

        case 1004:
            MGSeqParamSet(timerSequenceId, 1, (roundTimerFrames + REFRESH_RATE - 1) / REFRESH_RATE);
            roundTimerFrames--;
            if (roundTimerFrames < 0) {
                object->work[0] = 1005;
                roundTimerFrames = 60 * REFRESH_RATE;
                minigameStateTimer = 0;
            }
            break;

        case 1005:
            if (sequenceId < 0) {
                MGSeqParamSet(timerSequenceId, 2, -1);
                timerSequenceId = -1;
                sequenceId = MGSeqFinishCreate();
                FadeOutSequence(100);
                Hu3DModelAttrReset(mainControlObject->mdlId[2], HU3D_ATTR_DISPOFF);
            }
            else {
                for (i = 0; i < 4; i++) {
                    if (omObjGetDataAs(playerObjects[i], PlayerWork)->playerState != 2007) {
                        break;
                    }
                }
                if (i == 4 && !MGSeqStatGet(sequenceId)) {
                    object->work[0]++;
                    sequenceId = -1;
                    collectedRewardCount = 0;
                    minigameStateTimer = 0;
                    cameraTargetPosition = resultsCameraPosition;
                    cameraTargetUp = resultsCameraTarget;
                }
            }
            break;

        case 1006:
            minigameStateTimer++;
            UpdateCameraTransition(minigameStateTimer / (2.0f * REFRESH_RATE), &resultsCameraUp);
            if ((2.0f * REFRESH_RATE) <= minigameStateTimer) {
                minigameStateTimer = 2.0f * REFRESH_RATE;
                for (i = 0; i < 4; i++) {
                    float playerHeading;
                    if (omObjGetDataAs(playerObjects[i], PlayerWork)->playerState != 2010) {
                        break;
                    }
                    playerHeading = omObjGetDataAs(playerObjects[i], PlayerWork)->heading;
                    if (2.0f < playerHeading && 358.0f > playerHeading) {
                        break;
                    }
                }
                if (i == 4) {
                    object->work[0]++;
                    minigameStateTimer = 60;
                }
            }
            break;

        case 1007:
            for (i = 0; i < 4; i++) {
                if (omObjGetDataAs(playerObjects[i], PlayerWork)->playerState != 2014) {
                    break;
                }
            }
            if (i == 4) {
                if (0.0f > minigameStateTimer--) {
                    object->work[0]++;
                    minigameStateTimer = 0;
                    if (collectedRewardCount == 0) {
                        HuAudSStreamPlay(4);
                        for (i = 0; i < 4; i++) {
                            omObjGetDataAs(playerObjects[i], PlayerWork)->finalPlacement = 4;
                        }
                    }
                    else {
                        s32 previousRankedPlayerIndex;
                        HuAudSStreamPlay(1);
                        previousRankedPlayerIndex = -1;
                        for (i = 0; i < 4; i++) {
                            s32 bestPlacementScore = -1;
                            s32 bestPlayerIndex = -1;
                            PlayerWork *rankedPlayerWork;
                            s32 j;
                            for (j = 0; j < 4; j++) {
                                rankedPlayerWork = playerObjects[j]->data;
                                if (rankedPlayerWork->finalPlacement < 0 && bestPlacementScore < rankedPlayerWork->placementScore) {
                                    bestPlacementScore = rankedPlayerWork->placementScore;
                                    bestPlayerIndex = j;
                                }
                            }
                            rankedPlayerWork = playerObjects[bestPlayerIndex]->data;
                            rankedPlayerWork->finalPlacement = i + 1;
                            if (previousRankedPlayerIndex >= 0 && rankedPlayerWork->placementScore == omObjGetDataAs(playerObjects[previousRankedPlayerIndex], PlayerWork)->placementScore) {
                                rankedPlayerWork->finalPlacement = omObjGetDataAs(playerObjects[previousRankedPlayerIndex], PlayerWork)->finalPlacement;
                            }
                            previousRankedPlayerIndex = bestPlayerIndex;
                        }
                    }
                    for (i = 0; i < 4; i++) {
                        GWPlayerCoinWinSet(omObjGetDataAs(playerObjects[i], PlayerWork)->playerIndex, omObjGetDataAs(playerObjects[i], PlayerWork)->finalPlacement - 1);
                    }
                }
            }
            break;

        case 1008:
            minigameStateTimer++;
            if (210.0f < minigameStateTimer) {
                FadeOutSequence(-1);
                sequenceId = -1;
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = ExitMinigame;
            }

            break;

        default:
            OSReport("*** main mode error(%d)!!\n", object->work[0]);
            break;
    }
}

void InitializeMinigame(OMOBJ *object)
{
    s32 singleNo;
    s32 i;
    s32 angleNo;
    backgroundRotation = 0;
    object->work[0] = 1000;
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M441, 0x38));
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[0], 0);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowMapSet(object->mdlId[0]);
    omSetTra(object, 0, -31, 0);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M441, 0x39));
    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[1], 0);
    Hu3DModelPosSet(object->mdlId[1], 0, -31, 0);
    Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M441, 0x3A));
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[2], 0);
    Hu3DModelPosSet(object->mdlId[2], 0, -31, 0);
    Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
    for (singleNo = 0; singleNo < 1; singleNo++) {
        backgroundModelIds[singleNo] = Hu3DModelCreateFile(backgroundDecorationResourceIds[singleNo]);
        Hu3DModelAttrSet(backgroundModelIds[singleNo], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
        Hu3DModelAttrSet(backgroundModelIds[singleNo], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    }
    for (i = 0, angleNo = 0; i < 30;) {
        if (CreateBackgroundModel(angleNo++) != 0) {
            i++;
        }
    }
    HuAudFXPlay(1818);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    object->objFunc = UpdateMinigame;
}

void ObjectSetup(void)
{
    Vec shadowDir;
    s32 i;
    OMOBJ *object;
    s32 glight;
    OSReport("******* M441 ObjectSetup *********\n");
    objectManager = omInitObjMan(600, 8192);
    omGameSysInit(objectManager);
    minigameFrameCounter = 0;
    sequenceId = -1;
    timerSequenceId = -1;
    activeRewardObjectCount = 0;
    audioSequenceId = -1;
    cameraHorizontalOffset = 0;
    cameraZoom = 2100;
    cameraVerticalOffset = 1200;
    Hu3DCameraViewportSet(1, 0, 0, 640, 480, 0, 1);
    Hu3DCameraPerspectiveSet(1, 30, 20, 20000, 1.2);
    SetCameraView(&initialCameraPosition, &initialCameraUp, &initialCameraTarget);

    glight = Hu3DGLightCreateV(&lightPosition, &lightDirection, &lightColor);
    Hu3DGLightStaticSet(glight, 1);
    Hu3DGLightInfinitytSet(glight);
    Hu3DShadowCreate(30, 20, 20000);
    Hu3DShadowTPLvlSet(0.8);
    VECNormalize(&shadowPosition, &shadowDir);
    Hu3DShadowPosSet(&shadowPosition, &shadowDir, &shadowTarget);
    HuSprExecLayerSet(64, 5);
    omMakeGroupEx(objectManager, 0, 4);
    playerObjects = omGetGroupMemberListEx(objectManager, 0);
    for (i = 0; i < 4; i++) {
        object = omAddObjEx(objectManager, 100, 3, 16, 0, InitializePlayer);
        object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerWork), HU_MEMNUM_OVL);
        object->work[0] = i;
    }
    InitializeLookupList();
    initialBirdSpawnPending = 1;
    omMakeGroupEx(objectManager, 1, 20);
    birdObjects = omGetGroupMemberListEx(objectManager, 1);
    for (i = 0; i < 3; i++) {
        activeBirdCounts[i] = 0;
        birdModelIds[i] = Hu3DModelCreateFile(birdModelResourceIds[i]);
        Hu3DModelAttrSet(birdModelIds[i], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(birdModelIds[i], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    }
    for (i = 0; i < 20; i++) {
        CreateBirdObject(0);
    }
    omMakeGroupEx(objectManager, 2, 400);
    rewardObjects = omGetGroupMemberListEx(objectManager, 2);
    birdParticleAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M441, 0x46));
    HuSprAnimLock(birdParticleAnimation);
    birdParticleManagerId = Hu3DParManCreate(birdParticleAnimation, 500, &birdTrailParticleParameters);
    Hu3DParManAttrSet(birdParticleManagerId, 1);
    rewardParticleAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M441, 0x47));
    HuSprAnimLock(rewardParticleAnimation);
    rewardParticleManagerId = Hu3DParManCreate(rewardParticleAnimation, 20, &rewardParticleParameters);
    Hu3DParManAttrSet(rewardParticleManagerId, 1);
    mainControlObject = omAddObjEx(objectManager, 103, 3, 0, -1, InitializeMinigame);
}

void ExitMinigame(OMOBJ *object)
{
    s32 i;
    if (WipeStatGet()) {
        return;
    }
    if (sequenceId >= 0) {
        MGSeqKill(sequenceId);
    }
    if (timerSequenceId >= 0) {
        MGSeqKill(timerSequenceId);
    }
    MGSeqKillAll();
    HuAudAllStop();
    for (i = 0; i < 4; i++) {
        CharModelKill(characterModelIds[GWPlayerCfg[i].character]);
    }
    omOvlReturnEx(1, 1);
}
