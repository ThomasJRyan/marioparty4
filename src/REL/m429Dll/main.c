#include "REL/executor.h"
#include "game/chrman.h"
#include "game/frand.h"
#include "game/esprite.h"
#include "game/hu3d.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/mapspace.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "ext_math.h"
#include "version.h"


#ifndef __MWERKS__
#include "game/audio.h"
#include "game/hsfex.h"
#endif

void HuSysVWaitSet(s16 vcount);

#undef ABS
#define ABS(x) ((0 > (x)) ? -(x) : (x))

typedef struct ModelMappingNode {
    /* 0x00 */ u16 assetKey;
    /* 0x02 */ s16 mappedValue;
    /* 0x04 */ struct ModelMappingNode *previous;
    /* 0x08 */ struct ModelMappingNode *next;
} ModelMappingNode; // Size 0xC

typedef struct ParticleEffectNode {
    /* 0x00 */ HU3DPARMANPARAM *particleParameters;
    /* 0x04 */ s32 *particleModelIds;
    /* 0x08 */ s32 remainingFrames;
    /* 0x0C */ s32 particleCount;
    /* 0x10 */ struct ParticleEffectNode *previous;
    /* 0x14 */ struct ParticleEffectNode *next;
} ParticleEffectNode; // Size 0x18

typedef struct {
    /* 0x00 */ u32 assetId;
    /* 0x04 */ s16 modelId;
} CachedModelEntry; // Size 8

ModelMappingNode modelMappingListHead;
ParticleEffectNode particleEffectListHead;
HUPROCESS *objectManager;
Vec cameraTransitionStartPosition;
Vec cameraTransitionEndPosition;
Vec cameraTransitionStartTarget;
Vec cameraTransitionEndTarget;
u32 cameraModeFlags;
s32 currentMinigameFrames;
u32 recordedMinigameFrames;
s16 winnerSequenceId;
s16 loserSequenceId;
s32 minigameSequenceHandle;
s32 cameraTransitionComplete;
float timerPulseAngle;
s32 timerSpriteIds[16];
CachedModelEntry cachedModels[32];

void ResetModelMappingList(void)
{
    modelMappingListHead.previous = NULL;
    modelMappingListHead.next = NULL;
}

void RegisterModelMapping(u16 assetKey, s16 modelId)
{
    ModelMappingNode *newMappingEntry;
    ModelMappingNode *listHead;
    ModelMappingNode *previousFirstEntry;

    newMappingEntry = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*newMappingEntry), HU_MEMNUM_OVL);
    newMappingEntry->assetKey = assetKey;
    newMappingEntry->mappedValue = modelId;
    listHead = &modelMappingListHead;
    previousFirstEntry = listHead->next;
    listHead->next = newMappingEntry;
    if (NULL != previousFirstEntry) {
        previousFirstEntry->previous = newMappingEntry;
    }
    newMappingEntry->previous = listHead;
    newMappingEntry->next = previousFirstEntry;
}

s16 TakeModelMapping(s16 assetKey)
{
    ModelMappingNode *mappingEntry;
    ModelMappingNode *previousEntry;
    ModelMappingNode *nextEntry;
    s16 mappedValue;

    mappingEntry = modelMappingListHead.next;
    while (NULL != mappingEntry) {
        if (assetKey == mappingEntry->assetKey) {
            mappedValue = mappingEntry->mappedValue;
            previousEntry = mappingEntry->previous;
            nextEntry = mappingEntry->next;
            previousEntry->next = nextEntry;
            if (NULL != nextEntry) {
                nextEntry->previous = previousEntry;
            }
            HuMemDirectFree(mappingEntry);
            return mappedValue;
        }
        mappingEntry = mappingEntry->next;
    }
    return -1;
}

void ResetParticleEffectList(void)
{
    particleEffectListHead.previous = NULL;
    particleEffectListHead.next = NULL;
}

void CreateParticleEffectGroup(s16 particleManagerId, HU3DPARMANPARAM *particleParameters, s32 renderLayer, s32 initialFrameOffset, s32 particleCount, Vec *particlePositions, u8 blendMode)
{
    ParticleEffectNode *previousFirstEntry;
    ParticleEffectNode *particleEffectEntry;
    ParticleEffectNode *listHead;
    s32 i;

    particleEffectEntry = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*particleEffectEntry), HU_MEMNUM_OVL);
    particleEffectEntry->particleParameters = particleParameters;
    particleEffectEntry->remainingFrames = initialFrameOffset;
    particleEffectEntry->particleCount = particleCount;
    particleEffectEntry->particleModelIds = HuMemDirectMallocNum(HEAP_HEAP, particleCount * sizeof(*particleEffectEntry->particleModelIds), HU_MEMNUM_OVL);
    for (i = 0; i < particleCount; i++, particlePositions++) {
        particleEffectEntry->particleModelIds[i] = Hu3DParManLink(particleManagerId, particleParameters);
        if (particleEffectEntry->particleModelIds[i] >= 0) {
            Hu3DParManAttrReset(particleEffectEntry->particleModelIds[i], 1);
            Hu3DParManPosSet(particleEffectEntry->particleModelIds[i], particlePositions->x, particlePositions->y, particlePositions->z);
            Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(particleEffectEntry->particleModelIds[i]), blendMode);
            Hu3DModelLayerSet(Hu3DParManModelIDGet(particleEffectEntry->particleModelIds[i]), renderLayer);
        }
    }
    listHead = &particleEffectListHead;
    previousFirstEntry = listHead->next;
    listHead->next = particleEffectEntry;
    if (NULL != previousFirstEntry) {
        previousFirstEntry->previous = particleEffectEntry;
    }
    particleEffectEntry->previous = listHead;
    particleEffectEntry->next = previousFirstEntry;
}

void UpdateParticleEffectGroups(void)
{
    ParticleEffectNode *particleEffectEntry;
    ParticleEffectNode *previousEntry;
    ParticleEffectNode *nextEntry;
    s32 i;

    particleEffectEntry = particleEffectListHead.next;
    while (NULL != particleEffectEntry) {
        particleEffectEntry->remainingFrames--;
        if (particleEffectEntry->remainingFrames == 0) {
            for (i = 0; i < particleEffectEntry->particleCount; i++) {
                if (particleEffectEntry->particleModelIds[i] >= 0) {
                    Hu3DParManAttrSet(particleEffectEntry->particleModelIds[i], 1);
                }
            }
        }
        previousEntry = particleEffectEntry->previous;
        nextEntry = particleEffectEntry->next;
        if (-particleEffectEntry->particleParameters->maxTime > particleEffectEntry->remainingFrames) {
            for (i = 0; i < particleEffectEntry->particleCount; i++) {
                if (particleEffectEntry->particleModelIds[i] >= 0) {
                    Hu3DParManKill(particleEffectEntry->particleModelIds[i]);
                }
            }
            previousEntry->next = nextEntry;
            if (NULL != nextEntry) {
                nextEntry->previous = previousEntry;
            }
            HuMemDirectFree(particleEffectEntry->particleModelIds);
            HuMemDirectFree(particleEffectEntry);
        }
        particleEffectEntry = nextEntry;
    }
}

void ClearModelCache(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        cachedModels[i].assetId = 0;
        cachedModels[i].modelId = -1;
    }
}

s16 LoadCachedModel(s32 assetId)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (assetId == cachedModels[i].assetId) {
            return Hu3DModelLink(cachedModels[i].modelId);
        }
    }
    for (i = 0; i < 32; i++) {
        if (cachedModels[i].assetId == 0) {
            cachedModels[i].assetId = assetId;
            cachedModels[i].modelId = Hu3DModelCreateFile(assetId);
            return cachedModels[i].modelId;
        }
    }
    return Hu3DModelCreateFile(assetId);
}

void SetTimerSpriteDigits(s32 *timerSpriteIds, s32 firstSpriteIndex, s32 elapsedFrames)
{
    s32 digitValue;

    espBankSet(timerSpriteIds[firstSpriteIndex + 1], 10);
    espBankSet(timerSpriteIds[firstSpriteIndex + 4], 11);
    if (600 * REFRESH_RATE <= elapsedFrames) {
        elapsedFrames = 600 * REFRESH_RATE - 1;
    }
    digitValue = elapsedFrames / (60 * REFRESH_RATE);
    elapsedFrames -= digitValue * (60 * REFRESH_RATE);
    if (digitValue > 9) {
        digitValue = 9;
    }
    espBankSet(timerSpriteIds[firstSpriteIndex], digitValue);
    digitValue = elapsedFrames / REFRESH_RATE;
    elapsedFrames -= digitValue * REFRESH_RATE;
    espBankSet(timerSpriteIds[firstSpriteIndex + 2], digitValue / 10);
    espBankSet(timerSpriteIds[firstSpriteIndex + 3], digitValue % 10);
    digitValue = 101.5f * elapsedFrames / REFRESH_RATE;
    espBankSet(timerSpriteIds[firstSpriteIndex + 5], digitValue / 10);
    espBankSet(timerSpriteIds[firstSpriteIndex + 6], digitValue % 10);
}

void RecordMinigameTime(s32 playerIndex)
{
    timerPulseAngle = 0.0f;
    recordedMinigameFrames = currentMinigameFrames;
    GWMGRecordSet(playerIndex, recordedMinigameFrames);
    SetTimerSpriteDigits(timerSpriteIds, 7, currentMinigameFrames);
}

void ShowTimerSprites(void)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        espDispOn(timerSpriteIds[i]);
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
            espScaleSet(timerSpriteIds[i], pulseScale, pulseScale);
        }
    }
}

void CreateTimerSprites(float spriteY, s32 timerAssetId)
{
    float spriteY;
    float spriteX;
    s32 i;

    spriteX = 288.0f;
    spriteY = spriteY;
    timerSpriteIds[15] = espEntry(timerAssetId, 0, 0);
    espTPLvlSet(timerSpriteIds[15], 0.5f);
    espColorSet(timerSpriteIds[15], 0, 0, 0);
    espPosSet(timerSpriteIds[15], spriteX, spriteY);
    spriteX = 248.0f;
    spriteY = spriteY - 8.0f;
    timerSpriteIds[14] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 55), 0, 0);
    espPosSet(timerSpriteIds[14], spriteX - 18.0f, spriteY);
    for (i = 7; i <= 13; i++) {
        timerSpriteIds[i] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 48), 0, 0);
        espPosSet(timerSpriteIds[i], spriteX, spriteY);
        spriteX += 16.0f;
    }
    SetTimerSpriteDigits(timerSpriteIds, 7, recordedMinigameFrames);
    spriteX = 248.0f;
    spriteY = spriteY + 8.0f;
    for (i = 0; i <= 6; i++) {
        timerSpriteIds[i] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 49), 0, 0);
        espPosSet(timerSpriteIds[i], spriteX, spriteY);
        spriteX += 16.0f;
    }
    SetTimerSpriteDigits(timerSpriteIds, 0, 0);
    for (i = 0; i < 16; i++) {
        espDrawNoSet(timerSpriteIds[i], 0);
        espDispOff(timerSpriteIds[i]);
        espAttrSet(timerSpriteIds[i], HUSPR_ATTR_NOANIM);
        espPriSet(timerSpriteIds[i], 0x80 + i);
    }
}

void InterpolateVector(float interpolationProgress, Vec *startPosition, Vec *endPosition, Vec *resultPosition)
{
    float interpolationWeight;

    if (1.0f < interpolationProgress) {
        interpolationProgress = 1.0f;
    }
    interpolationWeight = sind(90.0f * interpolationProgress) * sind(90.0f * interpolationProgress);
    resultPosition->x = startPosition->x + interpolationWeight * (endPosition->x - startPosition->x);
    resultPosition->y = startPosition->y + interpolationWeight * (endPosition->y - startPosition->y);
    resultPosition->z = startPosition->z + interpolationWeight * (endPosition->z - startPosition->z);
}

void SetCameraTransform(Vec *cameraPosition, Vec *cameraUp, Vec *cameraTarget)
{
    Hu3DCameraPosSetV(1, cameraPosition, cameraUp, cameraTarget);
    cameraTransitionStartPosition = *cameraPosition;
    cameraTransitionStartTarget = *cameraTarget;
}

s32 UpdateCameraTransition(float interpolationProgress, Vec *cameraUp)
{
    Vec interpolatedCameraPosition;
    Vec interpolatedCameraTarget;

    InterpolateVector(interpolationProgress, &cameraTransitionStartPosition, &cameraTransitionEndPosition, &interpolatedCameraPosition);
    InterpolateVector(interpolationProgress, &cameraTransitionStartTarget, &cameraTransitionEndTarget, &interpolatedCameraTarget);
    Hu3DCameraPosSetV(1, &interpolatedCameraPosition, cameraUp, &interpolatedCameraTarget);
    if (1.0f <= interpolationProgress) {
        return 1;
    }
    else {
        cameraTransitionComplete = 1;
        return 0;
    }
}

void SetInterpolatedCamera(float interpolationProgress, Vec *cameraUp)
{
    Vec interpolatedCameraPosition;
    Vec interpolatedCameraTarget;

    InterpolateVector(interpolationProgress, &cameraTransitionStartPosition, &cameraTransitionEndPosition, &interpolatedCameraPosition);
    InterpolateVector(interpolationProgress, &cameraTransitionStartTarget, &cameraTransitionEndTarget, &interpolatedCameraTarget);
    SetCameraTransform(&interpolatedCameraPosition, cameraUp, &interpolatedCameraTarget);
    cameraTransitionComplete = 1;
}

void ProjectCameraPoint(Vec *worldPosition, s16 cameraMask, Vec *screenPosition)
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
    Mtx objectMatrix;

    Hu3DModelObjMtxGet(modelId, jointName, objectMatrix);
    jointPosition->x = objectMatrix[0][3];
    jointPosition->y = objectMatrix[1][3];
    jointPosition->z = objectMatrix[2][3];
}

void GetModelJointTransform(s16 modelId, char *jointName, Vec *jointPosition, Mtx jointTransform)
{
    Mtx objectMatrix;
    HU3DMODEL *modelData;

    modelData = &Hu3DData[modelId];
    Hu3DModelObjMtxGet(modelId, jointName, objectMatrix);
    MTXConcat(modelData->mtx, objectMatrix, objectMatrix);
    if (NULL != jointTransform) {
        MTXCopy(objectMatrix, jointTransform);
    }
    jointPosition->x = objectMatrix[0][3];
    jointPosition->y = objectMatrix[1][3];
    jointPosition->z = objectMatrix[2][3];
}

float ApproachAngle(float currentAngle, float targetAngle, float interpolationFactor)
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
    interpolatedAngle = fmod(currentAngle + angleDifference * interpolationFactor, 360.0);
    if (0.0f > interpolatedAngle) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

void PlayMinigameSequence(s32 sequenceId)
{
    minigameSequenceHandle = HuAudSeqPlay((s32)sequenceId);
}

void StopMinigameSequence(s32 fadeFrames)
{
    if (minigameSequenceHandle >= 0) {
        if (fadeFrames < 0) {
            fadeFrames = 1000;
        }
        HuAudSeqFadeOut(minigameSequenceHandle, fadeFrames);
    }
    minigameSequenceHandle = -1;
}

void ReadJoystickVector(Vec *position, float *facingAngle, s32 stickX, s32 stickY)
{
    Vec stickDirection;
    float stickMagnitude;
    s32 motionState;

    stickDirection.x = 0.0f;
    stickDirection.y = 0.0f;
    stickDirection.z = 0.0f;
    stickDirection.x = stickX / 4.0f;
    stickDirection.z = -(float)stickY / 4.0f;
    motionState = 0;
    stickMagnitude = VECMagXZ(&stickDirection);
    if (0.5f <= stickMagnitude) {
        if (10.0f <= stickMagnitude) {
            stickDirection.x /= stickMagnitude;
            stickDirection.z /= stickMagnitude;
            stickMagnitude = 10.0f;
            stickDirection.x = 10.0f * stickDirection.x;
            stickDirection.z = 10.0f * stickDirection.z;
        }
        *facingAngle = ApproachAngle(*facingAngle, atan2d(stickDirection.x, stickDirection.z), 0.4f);
        if (8.0f <= stickMagnitude) {
            motionState = 2;
        }
        else {
            motionState = 1;
        }
    }
    VECAdd((Vec *)&position, &stickDirection, (Vec *)&position);
}

typedef struct StageObjectDefinition {
    char *objectName;
    s32 objectFlags;
    s32 relatedObjectIds[4];
    Vec worldPosition;
} StageObjectDefinition;

typedef struct StageDefinition {
    s32 stageId;
    s32 *baseModelIds;
    s32 modelFiles[4];
    float activationRadius;
    s32 objectCount;
    StageObjectDefinition *objects;
} StageDefinition;

typedef struct PlayerAIProfile {
    float motionScale;
    float approachRadius;
    float retreatRadius;
    s32 cooldownFrames;
} PlayerAIProfile;

typedef struct PlayerViewportWork {
    s32 viewportSpriteId;
    float viewportOpacity;
    float viewportCenterX;
    float viewportCenterY;
    float viewportScaleX;
    float viewportScaleY;
} PlayerViewportWork;

s16 characterModelIds[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

StageObjectDefinition stageAObjectDefinitions[] = {
    { "baseA2-null2", 1, { 13, 1, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null11", 0, { 10, 0, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null12", 0, { 6, 3, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null13", 8, { 20, 2, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null5", 16, { 14, 5, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null6", 0, { 9, 4, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null20", 128, { 2, -1, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null21", 128, { 8, -1, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null22", 0, { 19, 7, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null7", 0, { 5, 15, 10, -1 }, { 0, 0, 0 } },
    { "baseA2-null10", 16, { 1, 9, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null18", 0, { 17, 12, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null17", 0, { 18, 11, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null3", 0, { 0, 14, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null4", 0, { 4, 13, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null8", 0, { 9, 16, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null9", 0, { 22, 15, 17, -1 }, { 0, 0, 0 } },
    { "baseA2-null19", 0, { 11, 23, 16, -1 }, { 0, 0, 0 } },
    { "baseA2-null16", 0, { 12, 19, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null15", 0, { 8, 18, 20, -1 }, { 0, 0, 0 } },
    { "baseA2-null14", 0, { 3, 19, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null39", 0, { 30, 22, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null40", 0, { 16, 21, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null27", 0, { 17, 37, 24, -1 }, { 0, 0, 0 } },
    { "baseA2-null28", 64, { 32, 23, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null23", 64, { 33, 26, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null24", 0, { 38, 25, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null35", 0, { 31, 28, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null34", 32, { 35, 27, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null31", 32, { 36, -1, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null37", 0, { 21, 34, 31, -1 }, { 0, 0, 0 } },
    { "baseA2-null36", 0, { 27, 30, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null29", 0, { 24, 33, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null30", 0, { 25, 32, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null38", 2, { 30, 35, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null33", 0, { 28, 34, 36, -1 }, { 0, 0, 0 } },
    { "baseA2-null32", 0, { 29, 35, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null26", 0, { 23, 38, -1, -1 }, { 0, 0, 0 } },
    { "baseA2-null25", 4, { 26, 37, -1, -1 }, { 0, 0, 0 } },
};

StageObjectDefinition stageBObjectDefinitions[] = {
    { "baseB2-null1", 1, { 12, 1, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null2", 0, { 8, 0, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null6", 0, { 9, 3, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null10", 0, { 15, 2, 4, -1 }, { 0, 0, 0 } },
    { "baseB2-null11", 0, { 10, 3, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null12", 0, { 11, 6, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null13", 8, { 18, 5, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null7", 16, { 14, -1, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null4", 16, { 1, 9, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null5", 0, { 2, 8, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null14", 128, { 4, 11, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null15", 0, { 5, 10, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null3", 0, { 0, 13, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null9", 0, { 21, 12, 14, -1 }, { 0, 0, 0 } },
    { "baseB2-null8", 0, { 7, 13, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null16", 0, { 3, 16, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null17", 0, { 24, 15, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null18", 128, { 18, -1, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null19", 0, { 6, 17, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null20", 0, { 35, 20, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null21", 0, { 27, 19, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null30", 0, { 13, 22, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null29", 0, { 38, 21, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null31", 0, { 32, 24, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null33", 0, { 16, 23, 25, -1 }, { 0, 0, 0 } },
    { "baseB2-null34", 0, { 30, 24, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null42", 64, { 34, -1, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null25", 0, { 20, 28, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null26", 32, { 37, 27, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null36", 0, { 39, 30, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null35", 0, { 25, 33, 29, -1 }, { 0, 0, 0 } },
    { "baseB2-null24", 32, { 36, -1, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null32", 64, { 23, -1, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null38", 0, { 30, 40, 34, -1 }, { 0, 0, 0 } },
    { "baseB2-null40", 0, { 26, 33, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null22", 2, { 19, 36, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null23", 0, { 31, 35, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null27", 0, { 28, 38, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null28", 0, { 22, 37, 39, -1 }, { 0, 0, 0 } },
    { "baseB2-null37", 0, { 29, 38, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null39", 0, { 33, 41, -1, -1 }, { 0, 0, 0 } },
    { "baseB2-null41", 4, { 40, -1, -1, -1 }, { 0, 0, 0 } },
};

Vec cameraTiltUpVector = { 0, 1, -1 };
Vec worldUpVector = { 0, 1, 0 };
s32 playerCameraIds[4] = { 1, 2, 4, 8 };
float fullScreenViewports[4][4] = { 0, 0, 320, 240, 0, 240, 320, 240, 320, 0, 320, 240, 320, 240, 320, 240 };
float playerViewportLayouts[4][4] = { 2, 2, 316, 236, 2, 242, 316, 236, 322, 2, 316, 236, 322, 242, 316, 236 };

float horizontalSplitViewports[4][4] = { 0, 0, 640, 480, 0, 480, 640, 0, 640, 0, 0, 480, 640, 480, 0, 0 };

float verticalSplitViewports[4][4] = { 0, 0, 0, 480, 0, 480, 0, 0, 0, 0, 640, 480, 0, 480, 640, 0 };

float cameraSideOffsets[4][2] = { -1, -1, -1, 1, 1, -1, 1, 1 };

Vec lightPosition = { 0, 10000, 0 };
Vec lightDirection = { 0, -1, 0 };
GXColor lightColor = { 255, 255, 255, 255 };
Vec shadowSourcePosition = { 0, 8000, 1000 };
Vec shadowTargetPosition = { 0, 0, 0 };
s32 characterMotionIds[8][10] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x72),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x5A),
    DATA_MAKE_NUM(DATADIR_M429, 0x08),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x58),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x59),
    DATA_MAKE_NUM(DATADIR_M429, 0x00),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x72),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x5A),
    DATA_MAKE_NUM(DATADIR_M429, 0x09),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x58),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x59),
    DATA_MAKE_NUM(DATADIR_M429, 0x01),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x72),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x5A),
    DATA_MAKE_NUM(DATADIR_M429, 0x0A),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x58),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x59),
    DATA_MAKE_NUM(DATADIR_M429, 0x02),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x72),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x5A),
    DATA_MAKE_NUM(DATADIR_M429, 0x0B),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x58),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x59),
    DATA_MAKE_NUM(DATADIR_M429, 0x03),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x72),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x5A),
    DATA_MAKE_NUM(DATADIR_M429, 0x0C),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x58),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x59),
    DATA_MAKE_NUM(DATADIR_M429, 0x04),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x72),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x5A),
    DATA_MAKE_NUM(DATADIR_M429, 0x0D),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x58),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x59),
    DATA_MAKE_NUM(DATADIR_M429, 0x05),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x72),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x5A),
    DATA_MAKE_NUM(DATADIR_M429, 0x0E),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x58),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x59),
    DATA_MAKE_NUM(DATADIR_M429, 0x06),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x72),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x5A),
    DATA_MAKE_NUM(DATADIR_M429, 0x0F),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x58),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x59),
    DATA_MAKE_NUM(DATADIR_M429, 0x07),
};

s32 characterIconSpriteIds[8] = {
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x00),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x01),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x02),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x03),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x04),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x05),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x06),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x07),
};

s32 playerModelFileIds[8] = {
    DATA_MAKE_NUM(DATADIR_M429, 0x2C),
    DATA_MAKE_NUM(DATADIR_M429, 0x2E),
    DATA_MAKE_NUM(DATADIR_M429, 0x2C),
    DATA_MAKE_NUM(DATADIR_M429, 0x2E),
    DATA_MAKE_NUM(DATADIR_M429, 0x2D),
    DATA_MAKE_NUM(DATADIR_M429, 0x2E),
    DATA_MAKE_NUM(DATADIR_M429, 0x2D),
    DATA_MAKE_NUM(DATADIR_M429, 0x2E),
};

s32 playerMarkerSpriteIds[4] = {
    DATA_MAKE_NUM(DATADIR_M429, 0x3D),
    DATA_MAKE_NUM(DATADIR_M429, 0x3D),
    DATA_MAKE_NUM(DATADIR_M429, 0x3E),
    DATA_MAKE_NUM(DATADIR_M429, 0x3E),
};

s32 playerMotionFileIds[4][3] = {
    DATA_MAKE_NUM(DATADIR_M429, 0x24),
    DATA_MAKE_NUM(DATADIR_M429, 0x30),
    DATA_MAKE_NUM(DATADIR_M429, 0x25),
    DATA_MAKE_NUM(DATADIR_M429, 0x26),
    DATA_MAKE_NUM(DATADIR_M429, 0x30),
    DATA_MAKE_NUM(DATADIR_M429, 0x27),
    DATA_MAKE_NUM(DATADIR_M429, 0x28),
    DATA_MAKE_NUM(DATADIR_M429, 0x30),
    DATA_MAKE_NUM(DATADIR_M429, 0x29),
    DATA_MAKE_NUM(DATADIR_M429, 0x2A),
    DATA_MAKE_NUM(DATADIR_M429, 0x30),
    DATA_MAKE_NUM(DATADIR_M429, 0x2B),
};

s32 rewardSpriteIds[4] = {
    DATA_MAKE_NUM(DATADIR_M429, 0x3A),
    DATA_MAKE_NUM(DATADIR_M429, 0x3A),
    DATA_MAKE_NUM(DATADIR_M429, 0x3B),
    DATA_MAKE_NUM(DATADIR_M429, 0x3B),
};

s32 stageAModelFileIds[4] = {
    DATA_MAKE_NUM(DATADIR_M429, 0x13),
    DATA_MAKE_NUM(DATADIR_M429, 0x16),
    DATA_MAKE_NUM(DATADIR_M429, 0x15),
    DATA_MAKE_NUM(DATADIR_M429, 0x14),
};

s32 stageBModelFileIds[4] = {
    DATA_MAKE_NUM(DATADIR_M429, 0x1C),
    DATA_MAKE_NUM(DATADIR_M429, 0x1F),
    DATA_MAKE_NUM(DATADIR_M429, 0x1E),
    DATA_MAKE_NUM(DATADIR_M429, 0x1D),
};

StageDefinition stageDefinitions[2] = {
    {
        0,
        stageAModelFileIds,
        {
            DATA_MAKE_NUM(DATADIR_M429, 0x10),
            DATA_MAKE_NUM(DATADIR_M429, 0x11),
            DATA_MAKE_NUM(DATADIR_M429, 0x17),
            DATA_MAKE_NUM(DATADIR_M429, 0x18),
        },
        13000,
        39,
        stageAObjectDefinitions,
    },
    {
        4,
        stageBModelFileIds,
        {
            DATA_MAKE_NUM(DATADIR_M429, 0x19),
            DATA_MAKE_NUM(DATADIR_M429, 0x1A),
            DATA_MAKE_NUM(DATADIR_M429, 0x20),
            DATA_MAKE_NUM(DATADIR_M429, 0x21),
        },
        13000,
        42,
        stageBObjectDefinitions,
    },
};

s32 stageResultSpriteIds[8] = {
    DATA_MAKE_NUM(DATADIR_M429, 0x32),
    DATA_MAKE_NUM(DATADIR_M429, 0x33),
    DATA_MAKE_NUM(DATADIR_M429, 0x34),
    DATA_MAKE_NUM(DATADIR_M429, 0x35),
    DATA_MAKE_NUM(DATADIR_M429, 0x36),
    DATA_MAKE_NUM(DATADIR_M429, 0x37),
    DATA_MAKE_NUM(DATADIR_M429, 0x38),
    DATA_MAKE_NUM(DATADIR_M429, 0x39),
};

PlayerAIProfile easyAIProfile = { 0.5f, 100.0f, 130.0f, 15 };

PlayerAIProfile normalAIProfile = { 0.6f, 110.0f, 120.0f, 12 };

PlayerAIProfile hardAIProfile = { 0.7f, 120.0f, 110.0f, 9 };

PlayerAIProfile expertAIProfile = { 0.8f, 130.0f, 100.0f, 6 };

HU3DPARMANPARAM particleParameters = {
    30,
    { 0, 0 },
    1.0f,
    10.0f,
    360.0f,
    { 0.0f, 0.1f, 0.0f },
    1.0f,
    1.0f,
    30.0f,
    1.05f,
    1,
    {
        { 255, 255, 255, 255 },
        { 255, 255, 255, 255 },
        { 255, 255, 255, 255 },
        { 255, 255, 255, 255 },
    },
    {
        { 255, 255, 255, 0 },
        { 255, 255, 255, 0 },
        { 255, 255, 255, 0 },
        { 255, 255, 255, 0 },
    },
};

OMOBJ *gameControllerObject;
OMOBJ *stageObject;
OMOBJ **playerObjects;
OMOBJ **attachedEffectObjects;
OMOBJ **rewardEffectObjects;
OMOBJ **coinEffectObjects;
s32 minigameObjectCount;
StageDefinition *currentStage;
PlayerViewportWork playerViewports[4];
float transitionTimer;
float stageHalfWidth;
float stageHalfDepth;
float cameraDistance;
float cameraAzimuth;
float cameraHeight;
float cameraTransitionDuration;
float effectRotationAngle;
Vec coinEffectControlPoints[2];
Vec cameraPositions[4];
Vec cameraStartPositions[4];
Vec cameraTargets[4];
Vec cameraStartTargets[4];
s32 stageVariant;
s32 teamRewardTurnCounts[2];
s32 winningTeam;
s32 minigameState;
s32 collectedCoinCount;
s32 activeRewardCount;
s16 sharedModelId;
s16 resultSpriteIds[5];
ANIMDATA *particleAnimation;
s16 particleManagerId;
s32 audioEffectHandle;
s32 currentAudioEffectHandle;

typedef struct PlayerWork {
    PlayerAIProfile *aiProfile;
    Vec worldPosition;
    Vec previousPosition;
    Vec joystickVector;
    Vec objectHookPosition;
    Vec attachedEffectPosition;
    Vec attachedEffectRotation;
    float rotationOffset;
    float facingAngle;
    float collisionRadius;
    float movementMagnitude;
    float animationTimer;
    float targetAngle;
    float collisionAngle;
    float effectOscillationAngle;
    float effectFrameCount;
    s32 particleManagerHandle;
    u8 *routeNodes;
    s32 routeLength;
    s32 routeCursor;
    s32 aiApproachTimer;
    s32 aiRetreatTimer;
    s32 selectedObjectIndex;
    s32 cooldownTimer;
    float stateFrameCounter;
    u32 objectVisitFlags[2];
    s32 behaviorState;
    s32 attachedEffectIndex;
    s32 turnSideIndex;
    s32 motionIndex;
    s32 overlayMotionIndex;
    s32 playerMarkerSprite;
    s32 actionPromptSprite;
    s32 playerIconSprites[4];
    s32 rewardSprites[4][2];
    s32 stateFlags;
    s32 currentObjectIndex;
    s32 targetObjectIndex;
    s32 stickX;
    s32 stickY;
    s32 buttonsDown;
    s32 wallCollisionFrames;
    s32 collisionStunFrames;
    s32 collidedWithWall;
    s32 objectCollisionFrames;
    s32 playerSeat;
    s32 playerIndex;
    s32 characterId;
    s32 controllerPort;
    s32 playerGroup;
} PlayerWork;

void InitializePlayerObject(void)
{
    PlayerWork *playerWork;
    s32 playerIndex;
    HuSysVWaitSet(1);
    cameraAzimuth -= HuPadSubStkX[0] / 59.0f;
    cameraDistance -= 4 * (HuPadSubStkY[0] / 59.0f);
    cameraHeight -= 4 * (HuPadTrigL[0] / 150.0f);
    cameraHeight += 4 * (HuPadTrigR[0] / 150.0f);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerWork = playerObjects[playerIndex]->data;
        print8(24, 96 + (16 * playerIndex), 1.5f, "%d %d %d M%2d %04x %2d %2d %d %d", playerWork->wallCollisionFrames, playerWork->objectCollisionFrames, playerWork->collisionStunFrames,
            playerWork->cooldownTimer, playerWork->stateFlags, playerWork->currentObjectIndex, playerWork->targetObjectIndex, playerWork->aiApproachTimer, playerWork->cooldownTimer);
    }
}

StageObjectDefinition *GetStageObjectDefinition(s32 requiredFlags)
{
    StageObjectDefinition *objectDefinition;
    s32 objectIndex;
    objectDefinition = currentStage->objects;
    for (objectIndex = 0; objectIndex < currentStage->objectCount; objectIndex++, objectDefinition++) {
        if (requiredFlags & objectDefinition->objectFlags) {
            return objectDefinition;
        }
    }
    return NULL;
}

void InitializeStageObjects(void)
{
    StageObjectDefinition *boundaryObject;
    s32 stageModelId;

    s32 objectIndex;
    stageModelId = frandmod(2);
    currentStage = &stageDefinitions[stageModelId];
    stageModelId = Hu3DModelCreateFile(currentStage->modelFiles[0]);
    Hu3DModelAttrSet(stageModelId, HU3D_ATTR_DISPOFF);
    Hu3DModelPosSet(stageModelId, 0, 0, 0);
    Hu3DModelRotSet(stageModelId, 0, -90.f * stageVariant, 0);
    for (objectIndex = 0; objectIndex < currentStage->objectCount; objectIndex++) {
        GetModelJointPosition(stageModelId, currentStage->objects[objectIndex].objectName, &currentStage->objects[objectIndex].worldPosition);
    }
    stageHalfWidth = 2000;
    stageHalfDepth = 2000;
    boundaryObject = GetStageObjectDefinition(1);
    if (NULL != boundaryObject) {
        stageHalfWidth = 2.0f * ABS(boundaryObject->worldPosition.x);
        stageHalfDepth = 2.2f * ABS(boundaryObject->worldPosition.z);
    }
}

void UpdatePlayerObjectTracking(PlayerWork *playerWork)
{
    float positionDeltaX;
    float positionDeltaZ;
    float distanceToObject;
    float nearestDistance;
    s32 nearestObjectIndex;
    StageObjectDefinition *objectDefinition;
    s32 objectIndex;
    s32 previousObjectIndex;
    s32 visitedFlagWord;
    s32 visitedFlagBit;
    previousObjectIndex = playerWork->currentObjectIndex;
    nearestObjectIndex = -1;
    nearestDistance = currentStage->activationRadius;
    objectDefinition = currentStage->objects;
    for (objectIndex = 0; objectIndex < currentStage->objectCount; objectIndex++, objectDefinition++) {
        positionDeltaX = playerWork->worldPosition.x - objectDefinition->worldPosition.x;
        positionDeltaZ = playerWork->worldPosition.z - objectDefinition->worldPosition.z;
        distanceToObject = sqrtf((positionDeltaX * positionDeltaX) + (positionDeltaZ * positionDeltaZ));
        if (playerWork->collisionRadius + 200.0f > distanceToObject && nearestDistance > distanceToObject) {
            nearestObjectIndex = objectIndex;
            nearestDistance = distanceToObject;
        }
    }
    if (nearestObjectIndex >= 0) {
        playerWork->currentObjectIndex = nearestObjectIndex;
        if (previousObjectIndex != nearestObjectIndex) {
            if (--playerWork->cooldownTimer < 0) {
                playerWork->cooldownTimer = -1;
            }
        }
        visitedFlagWord = nearestObjectIndex / 32;
        visitedFlagBit = nearestObjectIndex % 32;
        playerWork->objectVisitFlags[visitedFlagWord] |= 1 << visitedFlagBit;
    }
}

void BuildObjectRoute(PlayerWork *playerWork, s32 currentObjectIndex, s32 targetObjectIndex, s32 routeDepth, u8 *candidateRoute, s32 *visitedObjectFlags)
{
    s32 visitedObjectFlags[2];
    s32 neighborIndex;
    StageObjectDefinition *objectDefinition;
    s32 routeIndex;
    u8 *candidateRoute;

    s32 visitedFlagWord;
    s32 visitedFlagBit;
    visitedFlagWord = currentObjectIndex / 32;
    visitedFlagBit = currentObjectIndex % 32;
    visitedObjectFlags[visitedFlagWord] |= 1 << visitedFlagBit;
    candidateRoute = HuMemDirectMallocNum(HEAP_HEAP, routeDepth + 1, HU_MEMNUM_OVL);
    for (neighborIndex = 0; neighborIndex < routeDepth; neighborIndex++) {
        candidateRoute[neighborIndex] = candidateRoute[neighborIndex];
    }
    objectDefinition = &currentStage->objects[currentObjectIndex];
    for (neighborIndex = 0; neighborIndex < 4; neighborIndex++) {
        if (objectDefinition->relatedObjectIds[neighborIndex] < 0) {
            break;
        }
        candidateRoute[routeDepth] = objectDefinition->relatedObjectIds[neighborIndex];
        if (targetObjectIndex == objectDefinition->relatedObjectIds[neighborIndex]) {
            if (playerWork->routeLength > routeDepth + 1) {
                if (NULL != playerWork->routeNodes) {
                    HuMemDirectFree(playerWork->routeNodes);
                }
                playerWork->routeNodes = HuMemDirectMallocNum(HEAP_HEAP, routeDepth + 1, HU_MEMNUM_OVL);
                for (routeIndex = 0; routeIndex < routeDepth + 1; routeIndex++) {
                    playerWork->routeNodes[routeIndex] = candidateRoute[routeIndex];
                }
                playerWork->routeLength = routeDepth + 1;
            }
        }
        else {
            if (objectDefinition->relatedObjectIds[neighborIndex] >= 0) {
                visitedFlagWord = objectDefinition->relatedObjectIds[neighborIndex] / 32;
                visitedFlagBit = objectDefinition->relatedObjectIds[neighborIndex] % 32;
                if ((visitedObjectFlags[visitedFlagWord] & (1 << visitedFlagBit)) == 0u) {
                    for (routeIndex = 0; routeIndex < 2; routeIndex++) {
                        visitedObjectFlags[routeIndex] = visitedObjectFlags[routeIndex];
                    }
                    BuildObjectRoute(playerWork, objectDefinition->relatedObjectIds[neighborIndex], targetObjectIndex, routeDepth + 1, candidateRoute, visitedObjectFlags);
                }
            }
        }
    }
    HuMemDirectFree(candidateRoute);
}

void FindObjectRoute(PlayerWork *playerWork)
{
    s32 visitedObjectFlags[2];
    u8 currentObject;
    s32 flagWordIndex;
    if (NULL != playerWork->routeNodes) {
        HuMemDirectFree(playerWork->routeNodes);
    }
    playerWork->routeNodes = NULL;
    playerWork->routeLength = 65535;
    playerWork->routeCursor = 0;
    if (playerWork->currentObjectIndex == playerWork->targetObjectIndex) {
        playerWork->stateFlags |= 0x8;
        playerWork->targetObjectIndex = -1;
        return;
    }
    currentObject = playerWork->currentObjectIndex;
    for (flagWordIndex = 0; flagWordIndex < 2; flagWordIndex++) {
        visitedObjectFlags[flagWordIndex] = 0;
    }
    BuildObjectRoute(playerWork, playerWork->currentObjectIndex, playerWork->targetObjectIndex, 1, &currentObject, visitedObjectFlags);
}

void BuildRouteNodeList(PlayerWork *playerWork)
{
    Vec routeDirection;
    Vec directionToNextObject;
    float objectDeltaX;
    float objectDeltaZ;
    float distanceToObject;
    StageObjectDefinition *currentObjectDefinition;
    StageObjectDefinition *nextObjectDefinition;

    if (NULL != playerWork->routeNodes && playerWork->routeLength >= 2) {
        currentObjectDefinition = &currentStage->objects[playerWork->routeNodes[0]];
        nextObjectDefinition = &currentStage->objects[playerWork->routeNodes[1]];
        objectDeltaX = currentObjectDefinition->worldPosition.x - playerWork->worldPosition.x;
        objectDeltaZ = currentObjectDefinition->worldPosition.z - playerWork->worldPosition.z;
        distanceToObject = sqrtf((objectDeltaX * objectDeltaX) + (objectDeltaZ * objectDeltaZ));
        if (150.0f > distanceToObject) {
            playerWork->routeCursor = 1;
            return;
        }
        VECSubtract(&nextObjectDefinition->worldPosition, &currentObjectDefinition->worldPosition, &routeDirection);
        VECSubtract(&playerWork->worldPosition, &currentObjectDefinition->worldPosition, &directionToNextObject);
        if (0.0f != VECMag(&routeDirection) && 0.0f != VECMag(&directionToNextObject)) {
            VECNormalize(&routeDirection, &routeDirection);
            VECNormalize(&directionToNextObject, &directionToNextObject);
            if (0.8f < VECDotProduct(&routeDirection, &directionToNextObject)) {
                playerWork->routeCursor = 1;
            }
        }
    }
}

void EmptyObjectCallback(OMOBJ *object) { }

void InitializeStageModel(OMOBJ *object)
{
    object->mdlId[0] = Hu3DModelCreateFile(currentStage->baseModelIds[stageVariant]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    omSetTra(object, 0, 0, 0);
    omSetRot(object, 0, 0, 0);
    object->objFunc = EmptyObjectCallback;
}

typedef struct RewardEffectWork {
    Vec worldPosition;
    float scalePulseAngle;
    float uniformScale;
    s32 sourceObjectIndex;
    s32 scalePulseEnabled;
} RewardEffectWork;

void UpdateRewardEffectScale(OMOBJ *object)
{
    RewardEffectWork *effectWork = object->data;
    if (gameControllerObject->work[0] >= 1007) {
        effectWork->uniformScale *= 0.9f;
        if (0.0f > effectWork->uniformScale) {
            effectWork->uniformScale = 0.0f;
        }
    }
    else {
        if (effectWork->scalePulseEnabled & 0x1) {
            effectWork->scalePulseAngle += 6.0f;
            if (180.0f < effectWork->scalePulseAngle) {
                effectWork->scalePulseAngle = 180.0f;
            }
            effectWork->uniformScale = 1.0 - (0.5 * sind(effectWork->scalePulseAngle));
        }
    }
    omSetSca(object, effectWork->uniformScale, effectWork->uniformScale, effectWork->uniformScale);
    Hu3DModelScaleSet(object->mdlId[1], effectWork->uniformScale, effectWork->uniformScale, effectWork->uniformScale);
    Hu3DModelScaleSet(object->mdlId[0], effectWork->uniformScale, effectWork->uniformScale, effectWork->uniformScale);
}

void InitializeRewardEffect(OMOBJ *object)
{
    RewardEffectWork *effectWork = object->data;
    s32 modelIndex;
    s32 candidateObjectIndex;
    s32 *modelFileIds;
    StageObjectDefinition *candidateObject;
    s32 matchingObjectCount;
    s32 requiredObjectFlags;

    effectWork->worldPosition.x = 0;
    effectWork->worldPosition.y = 0;
    effectWork->worldPosition.z = 0;
    effectWork->scalePulseAngle = 0;
    effectWork->uniformScale = 1;
    effectWork->sourceObjectIndex = -1;
    effectWork->scalePulseEnabled = 0;
    requiredObjectFlags = 16 << ((object->work[0] + stageVariant + 2) & 0x3);
    matchingObjectCount = frandmod(currentStage->objectCount) + 1;
    candidateObjectIndex = frandmod(currentStage->objectCount);
    while (1) {
        candidateObject = &currentStage->objects[candidateObjectIndex];
        if ((requiredObjectFlags & candidateObject->objectFlags) && --matchingObjectCount < 0) {
            effectWork->sourceObjectIndex = candidateObjectIndex;
            effectWork->worldPosition = candidateObject->worldPosition;
            break;
        }
        else {
            candidateObjectIndex = (candidateObjectIndex + 1) % currentStage->objectCount;
        }
    }
    effectWork->worldPosition.y = 100;
    modelFileIds = &playerModelFileIds[object->work[0] * 2];
    object->mdlId[0] = Hu3DModelCreateFile(*modelFileIds++);
    object->mdlId[1] = Hu3DModelCreateFile(*modelFileIds++);
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        Hu3DModelCameraSet(object->mdlId[modelIndex], 15);
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_MOTATTR_LOOP);
        Hu3DModelAttrReset(object->mdlId[modelIndex], HU3D_MOTATTR_PAUSE);
        Hu3DModelLayerSet(object->mdlId[modelIndex], 3);
        Hu3DModelPosSet(object->mdlId[modelIndex], effectWork->worldPosition.x, effectWork->worldPosition.y, effectWork->worldPosition.z);
        Hu3DModelRotSet(object->mdlId[modelIndex], 0, 0, 0);
        Hu3DModelShadowSet(object->mdlId[modelIndex]);
    }
    omSetTra(object, effectWork->worldPosition.x, effectWork->worldPosition.y, effectWork->worldPosition.z);
    omSetRot(object, 0, 0, 0);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    object->objFunc = UpdateRewardEffectScale;
}

s32 ResolvePlayerWallCollision(PlayerWork *playerWork)
{
    AddX = 0;
    AddZ = 0;
    MapWall(playerWork->collisionRadius, playerWork->worldPosition.x, playerWork->worldPosition.y + playerWork->collisionRadius, playerWork->worldPosition.z);

    playerWork->worldPosition.x += AddX;
    playerWork->worldPosition.z += AddZ;
    if (0.0f != ABS(AddX) + ABS(AddZ)) {
        playerWork->collidedWithWall = 1;
        return 1;
    }
    else {
        return 0;
    }
}

static inline void VECSet(Vec *dst, float x, float y, float z)
{
    dst->x = x;
    dst->y = y;
    dst->z = z;
}

void ResolvePlayerCollisions(PlayerWork *playerWork)
{
    Vec playerDelta;
    Vec otherPlayerDirection;
    float positionDeltaX;
    float positionDeltaZ;
    float playerDistance;

    PlayerWork *otherPlayerWork;
    s32 playerIndex;
    s32 opposedPlayerCollisions;
    s32 collisionCount;
    opposedPlayerCollisions = 0;
    collisionCount = 0;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        otherPlayerWork = playerObjects[playerIndex]->data;
        if (playerWork->playerIndex == otherPlayerWork->playerIndex) {
            continue;
        }
        positionDeltaX = playerWork->worldPosition.x - otherPlayerWork->worldPosition.x;
        positionDeltaZ = playerWork->worldPosition.z - otherPlayerWork->worldPosition.z;
        playerDistance = sqrtf((positionDeltaX * positionDeltaX) + (positionDeltaZ * positionDeltaZ));
        if (playerDistance < playerWork->collisionRadius + otherPlayerWork->collisionRadius) {
            if (0.0f == playerDistance) {
                positionDeltaX = 0;
                positionDeltaZ = -1;
            }
            else {
                positionDeltaX /= playerDistance;
                positionDeltaZ /= playerDistance;
            }
            collisionCount++;
            playerWork->worldPosition.x = otherPlayerWork->worldPosition.x + (positionDeltaX * (playerWork->collisionRadius + otherPlayerWork->collisionRadius));
            playerWork->worldPosition.z = otherPlayerWork->worldPosition.z + (positionDeltaZ * (playerWork->collisionRadius + otherPlayerWork->collisionRadius));
            ResolvePlayerWallCollision(playerWork);
            VECSet(&playerDelta, playerWork->stickX, 0, playerWork->stickY);
            VECSet(&otherPlayerDirection, otherPlayerWork->stickX, 0, otherPlayerWork->stickY);
            if (0.0f != VECMag(&playerDelta) && 0.0f != VECMag(&otherPlayerDirection)) {
                VECNormalize(&playerDelta, &playerDelta);
                VECNormalize(&otherPlayerDirection, &otherPlayerDirection);
                if (-0.8f > VECDotProduct(&playerDelta, &otherPlayerDirection)) {
                    opposedPlayerCollisions++;
                }
            }
        }
    }
    if (opposedPlayerCollisions != 0 || collisionCount >= 2) {
        playerWork->wallCollisionFrames++;
        playerWork->objectCollisionFrames++;
    }
    else {
        playerWork->objectCollisionFrames = 0;
    }
}

void ResolvePlayerStageCollision(PlayerWork *playerWork)
{
    float positionDeltaX;
    float positionDeltaZ;
    float propDistance;
    RewardEffectWork *effectWork;
    s32 effectIndex;
    for (effectIndex = 0; effectIndex < 4; effectIndex++) {
        effectWork = rewardEffectObjects[effectIndex]->data;
        positionDeltaX = playerWork->worldPosition.x - effectWork->worldPosition.x;
        positionDeltaZ = playerWork->worldPosition.z - effectWork->worldPosition.z;
        propDistance = sqrtf((positionDeltaX * positionDeltaX) + (positionDeltaZ * positionDeltaZ));
        if (propDistance < playerWork->collisionRadius + 50) {
            if (0.0f == propDistance) {
                positionDeltaX = 0;
                positionDeltaZ = -1;
            }
            else {
                positionDeltaX /= propDistance;
                positionDeltaZ /= propDistance;
            }
            playerWork->worldPosition.x = effectWork->worldPosition.x + (positionDeltaX * (playerWork->collisionRadius + 50));
            playerWork->worldPosition.z = effectWork->worldPosition.z + (positionDeltaZ * (playerWork->collisionRadius + 50));
        }
    }
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

void SetPlayerAnimation(PlayerWork *playerWork, OMOBJ *playerObject, s32 motionIndex)
{
    float overlaySpeed;

    s32 overlayMotionIndex;
    if (motionIndex >= 0 && playerWork->motionIndex != motionIndex) {
        playerWork->motionIndex = motionIndex;
        Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[playerWork->motionIndex], 0, 8, HU3D_MOTATTR_LOOP);
        overlayMotionIndex = -1;
        overlaySpeed = 1;
        if (playerWork->turnSideIndex == 1) {
            overlayMotionIndex = 9;
        }
        if (playerWork->turnSideIndex == 0) {
            switch (playerWork->motionIndex) {
                case 0:
                    overlayMotionIndex = 7;
                    break;

                case 1:
                    overlayMotionIndex = 7;
                    overlaySpeed = 30.0f / 11.0f;
                    break;

                case 2:
                    overlayMotionIndex = 8;
                    break;
            }
        }

        if (overlayMotionIndex > 0 && playerWork->overlayMotionIndex != overlayMotionIndex) {
            playerWork->overlayMotionIndex = overlayMotionIndex;
            Hu3DMotionOverlaySet(playerObject->mdlId[0], playerObject->mtnId[playerWork->overlayMotionIndex]);
            Hu3DMotionOverlaySpeedSet(playerObject->mdlId[0], overlaySpeed);
        }
    }
}

void UpdatePlayerMovement(PlayerWork *playerWork, OMOBJ *playerObject)
{
    s32 motionIndex;
    playerWork->previousPosition = playerWork->worldPosition;
    playerWork->joystickVector.x = 0;
    playerWork->joystickVector.y = 0;
    playerWork->joystickVector.z = 0;
    playerWork->joystickVector.x = playerWork->stickX / 4.0f;
    playerWork->joystickVector.z = (-(float)playerWork->stickY) / 4.0f;
    motionIndex = 0;
    if ((playerWork->stateFlags & 0x1) == 0) {
        playerWork->movementMagnitude = VECMagXZ(&playerWork->joystickVector);
        if (0.5f <= playerWork->movementMagnitude) {
            if (10.0f <= playerWork->movementMagnitude) {
                playerWork->joystickVector.x /= playerWork->movementMagnitude;
                playerWork->joystickVector.z /= playerWork->movementMagnitude;
                playerWork->movementMagnitude = 10;
                playerWork->joystickVector.x = 10.0f * playerWork->joystickVector.x;
                playerWork->joystickVector.z = 10.0f * playerWork->joystickVector.z;
            }
            playerWork->facingAngle = ApproachAngle(playerWork->facingAngle, atan2d(playerWork->joystickVector.x, playerWork->joystickVector.z), 0.4f);
            if (8.0f <= playerWork->movementMagnitude) {
                motionIndex = 2;
            }
            else {
                motionIndex = 1;
            }
        }
        VECAdd(&playerWork->worldPosition, &playerWork->joystickVector, &playerWork->worldPosition);
    }
    playerWork->collidedWithWall = 0;
    ResolvePlayerWallCollision(playerWork);
    ResolvePlayerCollisions(playerWork);
    ResolvePlayerStageCollision(playerWork);
    ResolvePlayerWallCollision(playerWork);
    if (playerWork->collidedWithWall) {
        playerWork->wallCollisionFrames++;
        playerWork->collisionStunFrames++;
    }
    else {
        playerWork->collisionStunFrames = 0;
    }
    UpdatePlayerObjectTracking(playerWork);
    SetPlayerAnimation(playerWork, playerObject, motionIndex);
}

PlayerWork *GetPlayerBySeat(s32 playerSeat)
{
    s32 playerIndex;
    PlayerWork *playerWork;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerWork = playerObjects[playerIndex]->data;
        if (playerSeat == playerWork->playerSeat) {
            return playerWork;
        }
    }
    return NULL;
}

void CheckOpponentCollision(PlayerWork *playerWork)
{
    float playerDeltaX;
    float playerDeltaZ;
    float playerDistance;
    PlayerWork *otherPlayerWork;

    if ((playerWork->stateFlags & 0x4) && winningTeam < 0) {
        otherPlayerWork = GetPlayerBySeat(playerWork->playerSeat ^ 1);
        if (otherPlayerWork->stateFlags & 0x4) {
            playerDeltaX = playerWork->worldPosition.x - otherPlayerWork->worldPosition.x;
            playerDeltaZ = playerWork->worldPosition.z - otherPlayerWork->worldPosition.z;
            playerDistance = sqrtf((playerDeltaX * playerDeltaX) + (playerDeltaZ * playerDeltaZ));
            if ((1.5f * (playerWork->collisionRadius + otherPlayerWork->collisionRadius)) >= playerDistance) {
                winningTeam = playerWork->playerSeat & 0x2;
            }
        }
    }
}

void HandleRewardEffectCollision(PlayerWork *playerWork, OMOBJ *playerObject)
{
    float effectDeltaX;
    float effectDeltaZ;
    float effectDistance;
    RewardEffectWork *effectWork;
    s32 effectIndex;
    OMOBJ *effectObject;
    if (playerWork->stateFlags & 0x2) {
        return;
    }
    for (effectIndex = 0; effectIndex < 4; effectIndex++) {
        if ((playerWork->playerSeat & 0x2) != (rewardEffectObjects[effectIndex]->work[0] & 0x2)) {
            continue;
        }
        effectWork = rewardEffectObjects[effectIndex]->data;
        if (effectWork->scalePulseEnabled & 0x1) {
            continue;
        }
        effectDeltaX = effectWork->worldPosition.x - playerWork->worldPosition.x;
        effectDeltaZ = effectWork->worldPosition.z - playerWork->worldPosition.z;
        effectDistance = sqrtf((effectDeltaX * effectDeltaX) + (effectDeltaZ * effectDeltaZ));
        if (50 + playerWork->collisionRadius >= effectDistance) {
            omVibrate(playerWork->playerIndex, 12, 6, 6);
            playerWork->behaviorState = 2002;
            playerWork->animationTimer = 0;
            playerWork->stateFlags |= 0x2;
            playerWork->stateFlags &= ~0x10;
            playerWork->objectHookPosition = effectWork->worldPosition;
            playerWork->turnSideIndex = teamRewardTurnCounts[playerWork->playerSeat >> 1]++;
            playerWork->attachedEffectIndex = (playerWork->playerSeat & 0x2) + playerWork->turnSideIndex;
            Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_LOOP);
            if (!playerWork->turnSideIndex) {
                playerWork->motionIndex = 5;
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[5], 0, 10, HU3D_MOTATTR_NONE);
                HuAudFXPlay(1642);
            }
            else {
                playerWork->motionIndex = 6;
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[6], 0, 10, HU3D_MOTATTR_NONE);
                HuAudFXPlay(1643);
            }
            effectObject = attachedEffectObjects[playerWork->attachedEffectIndex];
            omSetTra(effectObject, playerWork->objectHookPosition.x, playerWork->objectHookPosition.y, playerWork->objectHookPosition.z);
            Hu3DModelAttrReset(effectObject->mdlId[0], HU3D_ATTR_DISPOFF);

            effectWork->scalePulseEnabled |= 0x1;
            Hu3DModelAttrSet(rewardEffectObjects[effectIndex]->mdlId[0], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(rewardEffectObjects[effectIndex]->mdlId[1], HU3D_ATTR_DISPOFF);
            break;
        }
    }
}

void UpdateRewardCollectionAnimation(PlayerWork *playerWork, OMOBJ *playerObject)
{
    float animationProgress;
    float hookDeltaX;
    float hookDeltaZ;
    Vec itemHookPosition;
    Vec animatedPosition;
    OMOBJ *attachedEffect;

    playerWork->animationTimer++;
    animationProgress = playerWork->animationTimer / 50;
    if (1.0f <= animationProgress) {
        animationProgress = 1.0f;
    }
    GetModelJointPosition(playerObject->mdlId[0], CharModelItemHookGet(playerWork->characterId, 8, 0), &itemHookPosition);
    InterpolateVector(animationProgress, &playerWork->objectHookPosition, &itemHookPosition, &animatedPosition);
    animatedPosition.y += sind(180.0f * animationProgress) * 100;
    attachedEffect = attachedEffectObjects[playerWork->attachedEffectIndex];
    omSetTra(attachedEffect, animatedPosition.x, animatedPosition.y, animatedPosition.z);
    (void)animationProgress;
    if (80.0f <= playerWork->animationTimer++) {
        playerWork->behaviorState = 2001;
        playerWork->stateFlags |= 0x4;
        playerWork->motionIndex = 0;
        omSetTra(attachedEffect, 0, 0, 0);
        Hu3DModelHookSet(playerObject->mdlId[0], CharModelItemHookGet(playerWork->characterId, 8, 0), attachedEffect->mdlId[0]);
        if (playerWork->turnSideIndex == 0) {
            playerWork->overlayMotionIndex = 7;
        }
        else {
            playerWork->overlayMotionIndex = 9;
        }
        Hu3DModelAttrSet(playerObject->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_OVL_LOOP);
        Hu3DMotionSet(playerObject->mdlId[0], playerObject->mtnId[0]);
        Hu3DMotionOverlaySet(playerObject->mdlId[0], playerObject->mtnId[playerWork->overlayMotionIndex]);
    }
}

s32 ChooseUnvisitedObject(PlayerWork *playerWork, s32 fallbackObjectIndex)
{
    s32 selectedObject;
    s32 candidateCount;
    s32 visitFlagWord;
    s32 visitFlagBit;
    if (playerWork->stateFlags & 0x10) {
        return fallbackObjectIndex;
    }
    candidateCount = 0;
    selectedObject = fallbackObjectIndex;
    if (playerWork->cooldownTimer >= 0) {
        playerWork->selectedObjectIndex = -1;
        selectedObject = frandmod(currentStage->objectCount);
        do {
            candidateCount++;
            if (currentStage->objectCount <= candidateCount) {
                selectedObject = frandmod(currentStage->objectCount);
            }
            else {
                visitFlagWord = selectedObject / 32;
                visitFlagBit = selectedObject % 32;
                if ((1 << visitFlagBit) & playerWork->objectVisitFlags[visitFlagWord]) {
                    selectedObject = (selectedObject + 1) % currentStage->objectCount;
                }
            }
        } while (selectedObject == fallbackObjectIndex);
    }

    return selectedObject;
}

void CheckObjectRouteBlocked(PlayerWork *playerWork, PlayerWork *opponentWork)
{
    s32 effectIndex;
    s32 relatedObjectIndex;
    s32 objectIsBlocked;
    RewardEffectWork *effectWork;
    s32 relatedObjectId;
    StageObjectDefinition *currentObjectDefinition;

    if (!(playerWork->stateFlags & 0x10)) {
        UpdatePlayerObjectTracking(playerWork);
        if (playerWork->currentObjectIndex >= 0) {
            objectIsBlocked = 0;
            currentObjectDefinition = &currentStage->objects[playerWork->currentObjectIndex];
            for (relatedObjectIndex = 0; relatedObjectIndex < 4; relatedObjectIndex++) {
                relatedObjectId = currentObjectDefinition->relatedObjectIds[relatedObjectIndex];
                if (relatedObjectId < 0) {
                    break;
                }
                if (playerWork->stateFlags & 0x2) {
                    if (relatedObjectId == opponentWork->currentObjectIndex) {
                        objectIsBlocked = 1;
                    }
                }
                else {
                    for (effectIndex = 0; effectIndex < 4; effectIndex++) {
                        if ((playerWork->playerSeat & 0x2) == (rewardEffectObjects[effectIndex]->work[0] & 0x2)) {
                            effectWork = rewardEffectObjects[effectIndex]->data;
                            if (!(effectWork->scalePulseEnabled & 0x1)) {
                                if (relatedObjectId == effectWork->sourceObjectIndex) {
                                    objectIsBlocked = 1;
                                }
                            }
                        }
                    }
                }
                if (objectIsBlocked != 0) {
                    break;
                }
            }
            if (objectIsBlocked != 0) {
                playerWork->targetObjectIndex = -1;
                playerWork->stateFlags |= 0x10;
            }
        }
    }
}

s32 ChooseNearestRewardObject(PlayerWork *playerWork)
{
    s32 effectIndex;
    s32 objectIndex;
    s32 bestObjectIndex;
    RewardEffectWork *effectWork;
    s32 shortestRouteLength;
    bestObjectIndex = -1;
    shortestRouteLength = 65535;
    playerWork->selectedObjectIndex = -1;
    for (objectIndex = 0; objectIndex < 4; objectIndex++) {
        if ((playerWork->playerSeat & 0x2) == (rewardEffectObjects[objectIndex]->work[0] & 0x2)) {
            effectWork = rewardEffectObjects[objectIndex]->data;
            if (effectWork->scalePulseEnabled & 0x1) {
                continue;
            }
            playerWork->targetObjectIndex = effectWork->sourceObjectIndex;
            FindObjectRoute(playerWork);
            if (shortestRouteLength > playerWork->routeLength) {
                bestObjectIndex = effectWork->sourceObjectIndex;
                shortestRouteLength = playerWork->routeLength;
                playerWork->selectedObjectIndex = objectIndex;
            }
        }
    }
    bestObjectIndex = ChooseUnvisitedObject(playerWork, bestObjectIndex);
    return bestObjectIndex;
}

void UpdatePlayerAi(PlayerWork *playerWork)
{
    float targetDeltaX;
    float targetDeltaZ;
    float targetDistance;
    float movementScale;

    Vec targetPosition;
    RewardEffectWork *effectWork;

    PlayerWork *opponentWork;
    s32 newTargetObjectIndex;
    if (NULL == playerWork->aiProfile) {
        return;
    }
    opponentWork = GetPlayerBySeat(playerWork->playerSeat ^ 1);
    playerWork->stateFrameCounter--;
    if (0.0f < playerWork->stateFrameCounter) {
        return;
    }
    if (playerWork->wallCollisionFrames > 30 || playerWork->objectCollisionFrames > 30) {
        playerWork->stateFrameCounter = 30;
        if (playerWork->stateFlags & 0x2) {
            playerWork->stateFrameCounter *= 2.5f;
        }
    }
    playerWork->aiRetreatTimer -= 1.0f;
    if (0.0f > playerWork->aiRetreatTimer) {
        playerWork->aiApproachTimer -= 1.0f;
        if (0.0f > playerWork->aiApproachTimer) {
            playerWork->aiApproachTimer = ((playerWork->aiProfile->approachRadius / 2) * frandf()) + playerWork->aiProfile->approachRadius;
            playerWork->aiRetreatTimer = ((playerWork->aiProfile->retreatRadius / 2) * frandf()) + playerWork->aiProfile->retreatRadius;
        }
    }
    if (playerWork->targetObjectIndex >= 0) {
        if (playerWork->stateFlags & 0x2) {
            if (playerWork->cooldownTimer >= 0) {
                if (playerWork->stateFlags & 0x8) {
                    playerWork->targetObjectIndex = -1;
                }
            }
            else {
                if (playerWork->targetObjectIndex != opponentWork->currentObjectIndex) {
                    playerWork->targetObjectIndex = -1;
                }
            }
        }
        else {
            if (playerWork->selectedObjectIndex >= 0) {
                effectWork = rewardEffectObjects[playerWork->selectedObjectIndex]->data;
                if (effectWork->scalePulseEnabled & 0x1) {
                    playerWork->targetObjectIndex = -1;
                }
            }
            else {
                if (playerWork->stateFlags & 0x8) {
                    playerWork->targetObjectIndex = -1;
                }
            }
        }
        if (playerWork->wallCollisionFrames > 30) {
            playerWork->targetObjectIndex = -1;
        }
        if (playerWork->collisionStunFrames > 30) {
            playerWork->targetObjectIndex = -1;
        }
        if (playerWork->objectCollisionFrames > 30) {
            playerWork->targetObjectIndex = -1;
        }
    }
    newTargetObjectIndex = -1;
    if (playerWork->targetObjectIndex < 0) {
        if (playerWork->stateFlags & 0x2) {
            playerWork->targetObjectIndex = ChooseUnvisitedObject(playerWork, opponentWork->currentObjectIndex);
        }
        else {
            playerWork->targetObjectIndex = ChooseNearestRewardObject(playerWork);
        }
        newTargetObjectIndex = playerWork->targetObjectIndex;
        playerWork->stateFlags &= ~0x8;
        FindObjectRoute(playerWork);
        BuildRouteNodeList(playerWork);
    }

    if ((playerWork->stateFlags & 0x8) && (playerWork->stateFlags & 0x2)) {
        targetPosition = opponentWork->worldPosition;
    }
    else {
        if (NULL == playerWork->routeNodes) {
            if (newTargetObjectIndex < 0) {
                playerWork->stateFlags &= ~0x8;
                playerWork->targetObjectIndex = frandmod(currentStage->objectCount);
                FindObjectRoute(playerWork);
                BuildRouteNodeList(playerWork);
                return;
            }
            else {
                targetPosition = currentStage->objects[newTargetObjectIndex].worldPosition;
            }
        }
        else {
            targetPosition = currentStage->objects[playerWork->routeNodes[playerWork->routeCursor]].worldPosition;
        }
    }
    targetDeltaX = targetPosition.x - playerWork->worldPosition.x;
    targetDeltaZ = targetPosition.z - playerWork->worldPosition.z;
    targetDistance = sqrtf((targetDeltaX * targetDeltaX) + (targetDeltaZ * targetDeltaZ));
    if (10.0f <= targetDistance) {
        targetDeltaX /= targetDistance;
        targetDeltaZ /= targetDistance;
    }
    else {
        targetDeltaX = 0;
        targetDeltaZ = 0;
    }
    if (150.0f > targetDistance) {
        CheckObjectRouteBlocked(playerWork, opponentWork);
        playerWork->routeCursor++;
        if (playerWork->routeLength <= playerWork->routeCursor) {
            playerWork->routeCursor = playerWork->routeLength - 1;
            playerWork->stateFlags |= 8;
        }
    }
    movementScale = playerWork->aiProfile->motionScale;
    playerWork->stickX = (72.0f * targetDeltaX) * movementScale;
    playerWork->stickY = (72.0f * -targetDeltaZ) * movementScale;
    if (ABS(playerWork->stickX) > 72) {
        playerWork->stickX = ((playerWork->stickX < 0) ? -1 : 1) * 72;
    }
    if (ABS(playerWork->stickY) > 72) {
        playerWork->stickY = ((playerWork->stickY < 0) ? -1 : 1) * 72;
    }
}

void SetPlayerCameraViewport(s32 playerIndex, float *viewport)
{
    PlayerViewportWork *playerViewport;
    Hu3DCameraViewportSet(playerCameraIds[playerIndex], viewport[0], viewport[1], viewport[2], viewport[3], 0, 1);
    Hu3DCameraScissorSet(playerCameraIds[playerIndex], viewport[0], viewport[1], viewport[2], viewport[3]);
    playerViewport = &playerViewports[playerIndex];
    playerViewport->viewportCenterX = 0.9f * (viewport[0] + (viewport[2] / 2));
    playerViewport->viewportCenterY = viewport[1] + (viewport[3] / 2);
    playerViewport->viewportScaleX = viewport[2] / 316;
    playerViewport->viewportScaleY = viewport[3] / 236;
}

void UpdatePlayerModelMotion(OMOBJ *object);

void UpdatePlayerDisplay(OMOBJ *object)
{
    PlayerWork *playerWork;
    s32 playerIconIndex;
    playerWork = object->data;

    if (gameControllerObject->work[0] == 1007) {
        playerWork->stateFlags &= ~0x1;
        espDispOff(playerWork->actionPromptSprite);
        for (playerIconIndex = 0; playerIconIndex < 4; playerIconIndex++) {
            espDispOff(playerWork->playerIconSprites[playerIconIndex]);
            espDispOff(playerWork->rewardSprites[playerIconIndex][0]);
            espDispOff(playerWork->rewardSprites[playerIconIndex][1]);
        }
        if (playerWork->behaviorState == 2001) {
            playerWork->behaviorState = 2003;
            SetPlayerAnimation(playerWork, object, 0);
            if (winningTeam == (playerWork->playerSeat & 0x2)) {
                GWPlayerCoinWinSet(playerWork->playerIndex, 10);
            }
            object->objFunc = UpdatePlayerModelMotion;
            return;
        }
    }
    if (GWPlayerCfg[playerWork->playerIndex].iscom) {
        playerWork->stickX = 0;
        playerWork->stickY = 0;
        playerWork->buttonsDown = 0;
        UpdatePlayerAi(playerWork);
    }
    else {
        playerWork->stickX = HuPadStkX[playerWork->controllerPort];
        playerWork->stickY = HuPadStkY[playerWork->controllerPort];
        playerWork->buttonsDown = HuPadBtnDown[playerWork->controllerPort];
    }
    if (gameControllerObject->work[0] < 1007 && (playerWork->buttonsDown & (PAD_BUTTON_X | PAD_BUTTON_Y))) {
        if (playerWork->stateFlags & 0x1) {
            playerWork->stateFlags &= ~0x1;
            espDispOff(playerWork->actionPromptSprite);
            for (playerIconIndex = 0; playerIconIndex < 4; playerIconIndex++) {
                espDispOff(playerWork->playerIconSprites[playerIconIndex]);
                espDispOff(playerWork->rewardSprites[playerIconIndex][0]);
                espDispOff(playerWork->rewardSprites[playerIconIndex][1]);
            }
        }
        else {
            playerWork->stateFlags |= 0x1;
            espDispOn(playerWork->actionPromptSprite);
            for (playerIconIndex = 0; playerIconIndex < 4; playerIconIndex++) {
                espDispOn(playerWork->playerIconSprites[playerIconIndex]);
            }
        }
    }
    switch (playerWork->behaviorState) {
        case 2001:
            UpdatePlayerMovement(playerWork, object);
            HandleRewardEffectCollision(playerWork, object);
            CheckOpponentCollision(playerWork);
            break;

        case 2002:
            UpdateRewardCollectionAnimation(playerWork, object);
            break;

        default:
            OSReport("*** player mode error(%d)!!\n", playerWork->behaviorState);
            break;
    }
    if (playerWork->collisionStunFrames == 0 && playerWork->objectCollisionFrames == 0) {
        playerWork->wallCollisionFrames = 0;
    }
    cameraPositions[playerWork->playerSeat].x = playerWork->worldPosition.x + (cameraDistance * sind(cameraAzimuth));
    cameraPositions[playerWork->playerSeat].y = playerWork->worldPosition.y + cameraHeight;
    cameraPositions[playerWork->playerSeat].z = playerWork->worldPosition.z + (cameraDistance * cosd(cameraAzimuth));
    cameraTargets[playerWork->playerSeat].x = playerWork->worldPosition.x;
    cameraTargets[playerWork->playerSeat].y = playerWork->worldPosition.y;
    cameraTargets[playerWork->playerSeat].z = playerWork->worldPosition.z;
    Hu3DCameraPosSetV(playerCameraIds[playerWork->playerSeat], &cameraPositions[playerWork->playerSeat], &worldUpVector, &cameraTargets[playerWork->playerSeat]);
    playerWork->targetAngle = playerWork->facingAngle;
    omSetTra(object, playerWork->worldPosition.x, playerWork->worldPosition.y, playerWork->worldPosition.z);
    omSetRot(object, 0, playerWork->facingAngle, 0);
}

void UpdatePlayerModelMotion(OMOBJ *object)
{
    float motionDuration;

    Mtx attachedObjectMatrix;
    Vec particlePosition;

    PlayerWork *playerWork = object->data;

    switch (playerWork->behaviorState) {
        case 2000:
            if (gameControllerObject->work[0] >= 1006) {
                playerWork->behaviorState++;
                object->objFunc = UpdatePlayerDisplay;
            }
            break;

        case 2003:
            if (gameControllerObject->work[0] == 1010) {
                Hu3DMotionOverlayReset(object->mdlId[0]);
                if (winningTeam >= 0 && (playerWork->playerSeat == winningTeam || playerWork->playerSeat == (winningTeam ^ 1))) {
                    playerWork->behaviorState = 2004;
                    playerWork->animationTimer = 0;
                    playerWork->stateFlags |= 0x20;
                    GetModelJointPosition(object->mdlId[0], CharModelItemHookGet(playerWork->characterId, 8, 0), &playerWork->objectHookPosition);
                    Hu3DModelObjMtxGet(object->mdlId[0], CharModelItemHookGet(playerWork->characterId, 8, 0), attachedObjectMatrix);
                    Hu3DMtxRotGet(attachedObjectMatrix, &playerWork->attachedEffectRotation);
                    Hu3DModelHookObjReset(object->mdlId[0], CharModelItemHookGet(playerWork->characterId, 8, 0));
                    omSetTra(attachedEffectObjects[playerWork->attachedEffectIndex], playerWork->objectHookPosition.x, playerWork->objectHookPosition.y, playerWork->objectHookPosition.z);
                    if (playerWork->turnSideIndex == 0) {
                        playerWork->rotationOffset = effectRotationAngle;
                        Hu3DMotionSet(object->mdlId[0], object->mtnId[5]);
                    }
                    else {
                        playerWork->rotationOffset = effectRotationAngle + 180;
                        playerWork->attachedEffectRotation.y = effectRotationAngle + 180;
                        Hu3DMotionSet(object->mdlId[0], object->mtnId[6]);
                    }
                    Hu3DMotionTimeSet(object->mdlId[0], Hu3DMotionMaxTimeGet(object->mdlId[0]) - 1);
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
                    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_REV);
                    playerWork->attachedEffectRotation.x = 0;
                    playerWork->attachedEffectRotation.z = 0;
                    omSetRot(attachedEffectObjects[playerWork->attachedEffectIndex], playerWork->attachedEffectRotation.x, playerWork->attachedEffectRotation.y, playerWork->attachedEffectRotation.z);
                    playerWork->rotationOffset = fmod(playerWork->rotationOffset - playerWork->attachedEffectRotation.y, 360);
                    if (0.0f > playerWork->rotationOffset) {
                        playerWork->rotationOffset += 360.0f;
                    }
                    if (180.0f < playerWork->rotationOffset) {
                        playerWork->rotationOffset -= 360.0f;
                    }
                    playerWork->rotationOffset /= 60.0f;
                }
                else {
                    playerWork->behaviorState = 2008;
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[4], 0, 8, HU3D_MOTATTR_NONE);
                }
            }
            if (gameControllerObject->work[0] == 1012) {
                playerWork->behaviorState = 2008;
                Hu3DMotionOverlayReset(object->mdlId[0]);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[4], 0, 8, HU3D_MOTATTR_NONE);
            }
            if (playerWork->behaviorState == 2008 && playerWork->attachedEffectIndex >= 0) {
                Hu3DModelAttrSet(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[0], HU3D_ATTR_DISPOFF);
                GetModelJointPosition(object->mdlId[0], CharModelItemHookGet(playerWork->characterId, 8, 0), &particlePosition);
                playerWork->particleManagerHandle = Hu3DParManLink(particleManagerId, &particleParameters);
                playerWork->effectFrameCount = 0;
                Hu3DParManPosSet(playerWork->particleManagerHandle, particlePosition.x, particlePosition.y, particlePosition.z);
                Hu3DParManAttrReset(playerWork->particleManagerHandle, 1);
                Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(playerWork->particleManagerHandle), 1);
                Hu3DModelLayerSet(Hu3DParManModelIDGet(playerWork->particleManagerHandle), 6);
            }
            break;

        case 2004:
            playerWork->animationTimer++;
            InterpolateVector(playerWork->animationTimer / 60, &playerWork->objectHookPosition, &coinEffectControlPoints[playerWork->turnSideIndex], &playerWork->attachedEffectPosition);
            playerWork->attachedEffectPosition.y += sind(playerWork->animationTimer * 3.0f) * 100;
            omSetTra(attachedEffectObjects[playerWork->attachedEffectIndex], playerWork->attachedEffectPosition.x, playerWork->attachedEffectPosition.y, playerWork->attachedEffectPosition.z);
            playerWork->attachedEffectRotation.y += playerWork->rotationOffset;
            omSetRot(attachedEffectObjects[playerWork->attachedEffectIndex], playerWork->attachedEffectRotation.x, playerWork->attachedEffectRotation.y, playerWork->attachedEffectRotation.z);
            if (60.0f <= playerWork->animationTimer) {
                playerWork->behaviorState++;
                playerWork->animationTimer = 0;
                playerWork->objectHookPosition = playerWork->attachedEffectPosition;
                if (playerWork->turnSideIndex == 1) {
                    coinEffectControlPoints[1].x -= 10 * sind(effectRotationAngle);
                    coinEffectControlPoints[1].z -= 10 * cosd(effectRotationAngle);
                }
                else {
                    particlePosition.x = (30 * sind(effectRotationAngle)) + playerWork->attachedEffectPosition.x;
                    particlePosition.y = 30 + playerWork->attachedEffectPosition.y;
                    particlePosition.z = (30 * cosd(effectRotationAngle)) + playerWork->attachedEffectPosition.z;
                    Hu3DModelTPLvlSet(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[1], 0);
                    Hu3DModelPosSet(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[1], particlePosition.x, particlePosition.y, particlePosition.z);
                    Hu3DModelRotSet(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[1], 0, effectRotationAngle, 0);
                    Hu3DModelAttrReset(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[1], HU3D_ATTR_DISPOFF);
                    Hu3DModelAttrReset(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[1], HU3D_MOTATTR_PAUSE);
                    Hu3DModelAttrSet(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[1], HU3D_MOTATTR_LOOP);
                }
            }
            break;

        case 2005:
            playerWork->animationTimer++;
            InterpolateVector(playerWork->animationTimer / 60, &playerWork->objectHookPosition, &coinEffectControlPoints[playerWork->turnSideIndex], &playerWork->attachedEffectPosition);
            omSetTra(attachedEffectObjects[playerWork->attachedEffectIndex], playerWork->attachedEffectPosition.x, playerWork->attachedEffectPosition.y, playerWork->attachedEffectPosition.z);
            if (playerWork->turnSideIndex == 0) {
                motionDuration = playerWork->animationTimer / 20;
                if (1.0f <= motionDuration) {
                    motionDuration = 1.0f;
                }
                Hu3DModelTPLvlSet(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[1], motionDuration);
            }
            if (60.0f <= playerWork->animationTimer) {
                playerWork->behaviorState++;
                playerWork->animationTimer = 0;
                if (playerWork->turnSideIndex == 1) {
                    Hu3DModelAttrReset(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
                    Hu3DMotionSet(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[0], attachedEffectObjects[playerWork->attachedEffectIndex]->mtnId[0]);
                    HuAudFXPlay(1649);
                }
            }

            break;

        case 2006:
            playerWork->animationTimer++;
            if (20.0f == playerWork->animationTimer) {
                playerWork->targetAngle = playerWork->collisionAngle;
                if (playerWork->turnSideIndex == 0) {
                    Hu3DModelAttrReset(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
                    Hu3DMotionSet(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[0], attachedEffectObjects[playerWork->attachedEffectIndex]->mtnId[0]);
                    HuAudFXPlay(1648);
                }
            }
            if (30.0f < playerWork->animationTimer && Hu3DMotionEndCheck(attachedEffectObjects[playerWork->attachedEffectIndex & 0x2]->mdlId[0])) {
                playerWork->behaviorState++;
                activeRewardCount++;
            }
            break;

        case 2007:
            if ((playerWork->playerSeat & 0x1) < collectedCoinCount) {
                playerWork->behaviorState = 2008;
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_REV);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[3], 0, 8, HU3D_MOTATTR_NONE);
            }
            break;

        case 2008:
            if (playerWork->particleManagerHandle >= 0) {
                playerWork->effectFrameCount++;
                if (10.0f == playerWork->effectFrameCount) {
                    Hu3DParManAttrSet(playerWork->particleManagerHandle, 1);
                }
            }
            break;

        default:
            OSReport("*** player mode error(%d)!!\n", playerWork->behaviorState);
            break;
    }
    if (playerWork->behaviorState >= 2005 && (playerWork->stateFlags & 0x20)) {
        playerWork->effectOscillationAngle += 4.0f;
        particlePosition = playerWork->attachedEffectPosition;
        particlePosition.y += sind(playerWork->effectOscillationAngle) * 5.0;
        omSetTra(attachedEffectObjects[playerWork->attachedEffectIndex], particlePosition.x, particlePosition.y, particlePosition.z);
        particlePosition.x = playerWork->attachedEffectPosition.x + (30.0 * sind(effectRotationAngle));
        particlePosition.y = (playerWork->attachedEffectPosition.y + 30) + (5.0 * sind(playerWork->effectOscillationAngle));
        particlePosition.z = playerWork->attachedEffectPosition.z + (30.0 * cosd(effectRotationAngle));
        Hu3DModelPosSet(attachedEffectObjects[playerWork->attachedEffectIndex]->mdlId[1], particlePosition.x, particlePosition.y, particlePosition.z);
    }
    if (playerWork->behaviorState >= 2003) {
        playerWork->facingAngle = ApproachAngle(playerWork->facingAngle, playerWork->targetAngle, 0.1f);
    }
    omSetTra(object, playerWork->worldPosition.x, playerWork->worldPosition.y, playerWork->worldPosition.z);
    omSetRot(object, 0, playerWork->facingAngle, 0);
}

void UpdatePlayerObject(OMOBJ *object)
{
    PlayerWork *playerWork;
    s32 motionIndex;
    s32 cameraSeat;
    StageObjectDefinition *startingObjectDefinition;

    playerWork = object->data;
    playerWork->characterId = GWPlayerCfg[playerWork->playerIndex].character;
    playerWork->controllerPort = GWPlayerCfg[playerWork->playerIndex].pad_idx;
    playerWork->behaviorState = 2000;
    playerWork->attachedEffectIndex = -1;
    playerWork->turnSideIndex = -1;
    playerWork->stateFlags = 0;
    playerWork->particleManagerHandle = -1;
    playerWork->facingAngle = 0;
    playerWork->effectOscillationAngle = 0;
    playerWork->motionIndex = 0;
    playerWork->overlayMotionIndex = -1;
    playerWork->collisionRadius = 65;
    playerWork->aiProfile = 0;
    playerWork->aiApproachTimer = 0;
    playerWork->aiRetreatTimer = 0;
    playerWork->routeNodes = 0;
    playerWork->routeLength = 65535;
    playerWork->routeCursor = 0;
    playerWork->selectedObjectIndex = -1;
    playerWork->cooldownTimer = -1;
    playerWork->stateFrameCounter = 0;
    playerWork->currentObjectIndex = -1;
    playerWork->targetObjectIndex = -1;
    playerWork->wallCollisionFrames = 0;
    playerWork->collisionStunFrames = 0;
    playerWork->objectCollisionFrames = 0;
    for (motionIndex = 0; motionIndex < 2; motionIndex++) {
        playerWork->objectVisitFlags[motionIndex] = 0;
    }
    object->mdlId[0] = CharModelCreate(characterModelIds[playerWork->characterId], 8);
    CharModelStepFxSet(characterModelIds[playerWork->characterId], 0);
    for (motionIndex = 0; motionIndex < 10; motionIndex++) {
        object->mtnId[motionIndex] = CharMotionCreate(characterModelIds[playerWork->characterId], characterMotionIds[playerWork->characterId][motionIndex]);
        CharMotionSet(characterModelIds[playerWork->characterId], object->mtnId[motionIndex]);
    }
    Hu3DModelCameraSet(object->mdlId[0], 15);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[0], 2);
    Hu3DMotionSet(object->mdlId[0], object->mtnId[0]);
    Hu3DModelShadowSet(object->mdlId[0]);
    playerWork->worldPosition.x = 0;
    playerWork->worldPosition.y = 0;
    playerWork->worldPosition.z = 0;
    cameraSeat = playerWork->playerSeat;
    if (cameraSeat >= 2) {
        cameraSeat ^= 1;
    }
    cameraSeat = (cameraSeat + stageVariant) % 4;
    startingObjectDefinition = GetStageObjectDefinition(1 << cameraSeat);
    if (NULL != startingObjectDefinition) {
        playerWork->worldPosition = startingObjectDefinition->worldPosition;
    }
    omSetTra(object, playerWork->worldPosition.x, playerWork->worldPosition.y, playerWork->worldPosition.z);
    omSetRot(object, 0, playerWork->facingAngle, 0);
    cameraStartPositions[playerWork->playerSeat].x = playerWork->worldPosition.x;
    cameraStartPositions[playerWork->playerSeat].y = playerWork->worldPosition.y + 450;
    cameraStartPositions[playerWork->playerSeat].z = playerWork->worldPosition.z + 850;
    cameraStartTargets[playerWork->playerSeat].x = playerWork->worldPosition.x;
    cameraStartTargets[playerWork->playerSeat].y = playerWork->worldPosition.y;
    cameraStartTargets[playerWork->playerSeat].z = playerWork->worldPosition.z;
    playerWork->playerMarkerSprite = espEntry(playerMarkerSpriteIds[playerWork->playerSeat], 134, 0);
    espDrawNoSet(playerWork->playerMarkerSprite, 0);
    playerViewports[playerWork->playerSeat].viewportSpriteId = playerWork->playerMarkerSprite;
    espDispOff(playerWork->playerMarkerSprite);
    espZRotSet(playerWork->playerMarkerSprite, (playerWork->playerSeat & 0x1) ? 0.0f : 180.0f);
    playerWork->actionPromptSprite = espEntry(DATA_MAKE_NUM(DATADIR_M429, 0x31), 133, 0);
    espDrawNoSet(playerWork->actionPromptSprite, 0);
    espDispOff(playerWork->actionPromptSprite);
    for (motionIndex = 0; motionIndex < 4; motionIndex++) {
        playerWork->playerIconSprites[motionIndex] = espEntry(characterIconSpriteIds[GWPlayerCfg[motionIndex].character], (playerWork->playerIndex == motionIndex) ? 129 : 130, 0);
        espDrawNoSet(playerWork->playerIconSprites[motionIndex], 0);
        espDispOff(playerWork->playerIconSprites[motionIndex]);
        playerWork->rewardSprites[motionIndex][0] = espEntry(rewardSpriteIds[motionIndex], 128, 0);
        espDrawNoSet(playerWork->rewardSprites[motionIndex][0], 0);
        espDispOff(playerWork->rewardSprites[motionIndex][0]);
        playerWork->rewardSprites[motionIndex][1] = espEntry(DATA_MAKE_NUM(DATADIR_M429, 0x3C), 128, 0);
        espDrawNoSet(playerWork->rewardSprites[motionIndex][1], 0);
        espDispOff(playerWork->rewardSprites[motionIndex][1]);
    }
    if (GWPlayerCfg[playerWork->playerIndex].iscom) {
        switch (GWPlayerCfg[playerWork->playerIndex].diff) {
            case 0:
                playerWork->aiProfile = &easyAIProfile;
                break;

            case 1:
                playerWork->aiProfile = &normalAIProfile;
                break;

            case 2:
                playerWork->aiProfile = &hardAIProfile;
                break;

            case 3:
            default:
                playerWork->aiProfile = &expertAIProfile;
                break;
        }
        playerWork->aiApproachTimer = frandf() * playerWork->aiProfile->approachRadius;
        playerWork->cooldownTimer = playerWork->aiProfile->cooldownFrames + ((playerWork->aiProfile->cooldownFrames / 2.0f) * frandf());
    }
    CharMotionVoiceOnSet(playerWork->characterId, object->mtnId[4], 0);
    UpdatePlayerObjectTracking(playerWork);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    CharModelLayerSetAll2(5);
    CharMotionDataClose(characterModelIds[playerWork->characterId]);
    object->objFunc = UpdatePlayerModelMotion;
}

void EmptyPlayerDisplayCallback(OMOBJ *object) { }

void InitializePlayerModels(OMOBJ *object)
{
    s32 modelIndex;
    s32 *modelFileCursor;
    modelFileCursor = &playerMotionFileIds[object->work[0]][0];
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        object->mdlId[modelIndex] = Hu3DModelCreateFile(*modelFileCursor++);
        Hu3DModelCameraSet(object->mdlId[modelIndex], 15);
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(object->mdlId[modelIndex], HU3D_MOTATTR_LOOP);
        Hu3DModelLayerSet(object->mdlId[modelIndex], 3);
        Hu3DModelShadowSet(object->mdlId[modelIndex]);
    }
    for (modelIndex = 0; modelIndex < 1; modelFileCursor++, modelIndex++) {
        object->mtnId[modelIndex] = Hu3DJointMotionFile(object->mdlId[0], modelFileCursor[0]);
    }
    Hu3DMotionSet(object->mdlId[0], object->mtnId[0]);
    omSetTra(object, 0, 0, 0);
    omSetRot(object, 0, 0, 0);
    object->objFunc = EmptyPlayerDisplayCallback;
}

void UpdateRewardAnimation(OMOBJ *object)
{
    float animationProgress;
    float easingWeight;

    Vec *coinEffectWork;
    PlayerWork *targetPlayerWork;
    s32 targetPlayerSeat;
    coinEffectWork = object->data;
    coinEffectWork[1].x += (frandf() * 12.0f) + 12.0f;
    targetPlayerSeat = winningTeam;
    if (object->work[0] & 0x1) {
        targetPlayerSeat ^= 0x1;
    }
    targetPlayerWork = GetPlayerBySeat(targetPlayerSeat);
    coinEffectWork[1].y++;
    if (90.0f < coinEffectWork[1].y) {
        coinEffectWork[1].y = 90.0f;
    }
    animationProgress = coinEffectWork[1].y / 90.0f;
    easingWeight = sind(90.0f * animationProgress) * sind(90.0f * animationProgress);
    coinEffectWork[0].x = coinEffectControlPoints->x + easingWeight * (targetPlayerWork->worldPosition.x - coinEffectControlPoints->x) * 1.5f;
    coinEffectWork[0].z = coinEffectControlPoints->z + easingWeight * (targetPlayerWork->worldPosition.z - coinEffectControlPoints->z) * 1.5f;
    coinEffectWork[0].y += coinEffectWork[1].z;
    coinEffectWork[1].z -= 0.4f;
    omSetTra(object, coinEffectWork[0].x, coinEffectWork[0].y, coinEffectWork[0].z);
    omSetRot(object, 0, coinEffectWork[1].x, 0);
    if (0.0f > coinEffectWork[1].z && 180.0f > coinEffectWork[0].y) {
        collectedCoinCount++;
        HuAudFXPlay(7);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        CharEffectCoinGlowCreate(playerCameraIds[winningTeam], coinEffectWork);
        omDelObjEx(objectManager, object);
    }
}

void UpdateCoinEffect(OMOBJ *object)
{
    Vec *coinEffectWork;
    coinEffectWork = object->data;
    object->mdlId[0] = Hu3DModelLink(sharedModelId);
    Hu3DModelCameraSet(object->mdlId[0], playerCameraIds[winningTeam]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[0], 4);
    Hu3DModelShadowSet(object->mdlId[0]);
    coinEffectWork[0] = coinEffectControlPoints[0];
    coinEffectWork[0].y += 20;
    coinEffectWork[1].x = 0;
    coinEffectWork[1].y = 0;
    coinEffectWork[1].z = (4 * frandf()) + 10;
    omSetTra(object, coinEffectWork[0].x, coinEffectWork[0].y, coinEffectWork[0].z);
    omSetRot(object, 0, coinEffectWork[1].x, 0);
    omSetSca(object, 0.4f, 0.4f, 0.4f);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    object->objFunc = UpdateRewardAnimation;
}

void CreateCoinEffect(s32 playerIndex)
{
    OMOBJ *object;
    object = omAddObjEx(objectManager, 103, 1, 0, 3, UpdateCoinEffect);
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(Vec) * 2, HU_MEMNUM_OVL);
    object->work[0] = playerIndex;
}

void InitializeCamera(void)
{
    s32 cameraIndex;
    float cameraTilt;
    float fieldOfViewScale;
    Vec cameraOffset;
    fieldOfViewScale = sind(15.0f) / cosd(15.0f);
    cameraTilt = 0.5f;
    cameraOffset.z = (1.0f - cameraTilt) * (currentStage->activationRadius * fieldOfViewScale);
    cameraOffset.x = 1.2f * cameraOffset.z;
    cameraOffset.y = currentStage->activationRadius * cameraTilt;
    Hu3DCameraCreate(15);
    Hu3DCameraPerspectiveSet(15, 30, 10, 20000, 1.2f);
    for (cameraIndex = 0; cameraIndex < 4; cameraIndex++) {
        SetPlayerCameraViewport(cameraIndex, &fullScreenViewports[cameraIndex][0]);
        cameraPositions[cameraIndex].x = cameraOffset.x * cameraSideOffsets[cameraIndex][0];
        cameraPositions[cameraIndex].y = cameraOffset.y;
        cameraPositions[cameraIndex].z = cameraOffset.z * cameraSideOffsets[cameraIndex][1];
        cameraTargets[cameraIndex].x = cameraPositions[cameraIndex].x;
        cameraTargets[cameraIndex].y = 0;
        cameraTargets[cameraIndex].z = cameraPositions[cameraIndex].z;
        Hu3DCameraPosSetV(playerCameraIds[cameraIndex], &cameraPositions[cameraIndex], &cameraTiltUpVector, &cameraTargets[cameraIndex]);
    }
}

void InterpolateCameraView(s32 cameraIndex, Vec *cameraUp, float interpolationProgress)
{
    Vec cameraPosition;
    Vec cameraTarget;
    cameraPosition.x = cameraPositions[cameraIndex].x + (interpolationProgress * (cameraStartPositions[cameraIndex].x - cameraPositions[cameraIndex].x));
    cameraPosition.y = cameraPositions[cameraIndex].y + (interpolationProgress * (cameraStartPositions[cameraIndex].y - cameraPositions[cameraIndex].y));
    cameraPosition.z = cameraPositions[cameraIndex].z + (interpolationProgress * (cameraStartPositions[cameraIndex].z - cameraPositions[cameraIndex].z));
    cameraTarget.x = cameraTargets[cameraIndex].x + (interpolationProgress * (cameraStartTargets[cameraIndex].x - cameraTargets[cameraIndex].x));
    cameraTarget.y = cameraTargets[cameraIndex].y + (interpolationProgress * (cameraStartTargets[cameraIndex].y - cameraTargets[cameraIndex].y));
    cameraTarget.z = cameraTargets[cameraIndex].z + (interpolationProgress * (cameraStartTargets[cameraIndex].z - cameraTargets[cameraIndex].z));
    Hu3DCameraPosSetV(playerCameraIds[cameraIndex], &cameraPosition, cameraUp, &cameraTarget);
}

void UpdateCameraViewports(void)
{
    s32 viewportIndex;
    float *currentViewport;
    float *targetViewport;
    float transitionProgress;
    float transitionDuration;

    float interpolatedViewport[4];
    transitionTimer++;
    transitionDuration = transitionTimer / 120;
    transitionProgress = sind(90 * transitionDuration) * sind(90 * transitionDuration);
    for (viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
        InterpolateCameraView(viewportIndex, &cameraTiltUpVector, transitionProgress);
    }
    if (30.0f < transitionTimer) {
        transitionDuration = (transitionTimer - 30.0f) / 60.0f;
        if (1.0f < transitionDuration) {
            transitionDuration = 1;
        }
        transitionProgress = sind(90 * transitionDuration) * sind(90 * transitionDuration);
        for (viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            currentViewport = &fullScreenViewports[viewportIndex][0];
            targetViewport = &playerViewportLayouts[viewportIndex][0];
            interpolatedViewport[0] = currentViewport[0] + (transitionProgress * (targetViewport[0] - currentViewport[0]));
            interpolatedViewport[1] = currentViewport[1] + (transitionProgress * (targetViewport[1] - currentViewport[1]));
            interpolatedViewport[2] = currentViewport[2] + (transitionProgress * (targetViewport[2] - currentViewport[2]));
            interpolatedViewport[3] = currentViewport[3] + (transitionProgress * (targetViewport[3] - currentViewport[3]));
            SetPlayerCameraViewport(viewportIndex, interpolatedViewport);
        }
    }
}

void InterpolateViewport(float *targetViewports)
{
    float interpolatedViewport[4];
    float *startViewport;
    s32 viewportIndex;
    float transitionProgress;
    float transitionDuration;

    startViewport = &playerViewportLayouts[0][0];
    transitionDuration = transitionTimer / REFRESH_RATE;
    transitionProgress = sind(90 * transitionDuration) * sind(90 * transitionDuration);
    for (viewportIndex = 0; viewportIndex < 4; viewportIndex++, startViewport += 4, targetViewports += 4) {
        interpolatedViewport[0] = startViewport[0] + (transitionProgress * (targetViewports[0] - startViewport[0]));
        interpolatedViewport[1] = startViewport[1] + (transitionProgress * (targetViewports[1] - startViewport[1]));
        interpolatedViewport[2] = startViewport[2] + (transitionProgress * (targetViewports[2] - startViewport[2]));
        interpolatedViewport[3] = startViewport[3] + (transitionProgress * (targetViewports[3] - startViewport[3]));
        SetPlayerCameraViewport(viewportIndex, interpolatedViewport);
    }
}

void UpdateCameraFollow(void)
{
    float playerDeltaX;
    float playerDeltaZ;
    float cameraDistance;

    PlayerWork *winningPlayerWork;
    PlayerWork *otherPlayerWork;
    winningPlayerWork = GetPlayerBySeat(winningTeam);
    otherPlayerWork = GetPlayerBySeat(winningTeam ^ 1);
    playerDeltaX = winningPlayerWork->worldPosition.x - otherPlayerWork->worldPosition.x;
    playerDeltaZ = winningPlayerWork->worldPosition.z - otherPlayerWork->worldPosition.z;
    effectRotationAngle = atan2d(playerDeltaX, playerDeltaZ) + 90;
    if (90.0f < effectRotationAngle) {
        effectRotationAngle -= 180.0f;
    }

    cameraStartTargets[winningTeam].x = cameraTargets[winningTeam].x - (playerDeltaX / 2);
    cameraStartTargets[winningTeam].y = cameraTargets[winningTeam].y + 200;
    cameraStartTargets[winningTeam].z = cameraTargets[winningTeam].z - (playerDeltaZ / 2);
    cameraDistance = 1350;
    cameraStartPositions[winningTeam].x = cameraStartTargets[winningTeam].x + (cameraDistance * sind(effectRotationAngle));
    cameraStartPositions[winningTeam].y = 250 + cameraStartTargets[winningTeam].y;
    cameraStartPositions[winningTeam].z = cameraStartTargets[winningTeam].z + (cameraDistance * cosd(effectRotationAngle));
    coinEffectControlPoints[0] = cameraStartTargets[winningTeam];
    coinEffectControlPoints[1] = cameraStartTargets[winningTeam];
    coinEffectControlPoints[1].x += 50 * sind(effectRotationAngle);
    coinEffectControlPoints[1].y += 15;
    coinEffectControlPoints[1].z += 50 * cosd(effectRotationAngle);
    playerDeltaX = coinEffectControlPoints[0].x - winningPlayerWork->worldPosition.x;
    playerDeltaZ = coinEffectControlPoints[0].z - winningPlayerWork->worldPosition.z;
    winningPlayerWork->targetAngle = atan2d(playerDeltaX, playerDeltaZ);
    playerDeltaX = coinEffectControlPoints[0].x - otherPlayerWork->worldPosition.x;
    playerDeltaZ = coinEffectControlPoints[0].z - otherPlayerWork->worldPosition.z;
    otherPlayerWork->targetAngle = atan2d(playerDeltaX, playerDeltaZ);
    playerDeltaX = cameraStartPositions[winningTeam].x - winningPlayerWork->worldPosition.x;
    playerDeltaZ = cameraStartPositions[winningTeam].z - winningPlayerWork->worldPosition.z;
    winningPlayerWork->collisionAngle = atan2d(playerDeltaX, playerDeltaZ);
    playerDeltaX = cameraStartPositions[winningTeam].x - otherPlayerWork->worldPosition.x;
    playerDeltaZ = cameraStartPositions[winningTeam].z - otherPlayerWork->worldPosition.z;
    otherPlayerWork->collisionAngle = atan2d(playerDeltaX, playerDeltaZ);
    omVibrate(winningPlayerWork->playerIndex, (VERSION_JP ? 3 : 2) * REFRESH_RATE, 6, 6);
    omVibrate(otherPlayerWork->playerIndex, (VERSION_JP ? 3 : 2) * REFRESH_RATE, 6, 6);
}

void ProjectWorldToScreen(float *viewport, float worldX, float worldY, s32 spriteId)
{
    float screenY;
    float screenX;
    screenX = (viewport[2] / 2) * (worldX / stageHalfWidth);
    screenY = (viewport[3] / 2) * (worldY / stageHalfDepth);
    screenX += viewport[0] + (viewport[2] / 2);
    screenY += viewport[1] + (viewport[3] / 2);
    screenX = 576.0f * (screenX / 640.0f);
    screenY = 480.0f * (screenY / 480.0f);
    screenY += (240.0f <= viewport[1]) ? -16.0f : 16.0f;
    espPosSet(spriteId, screenX, screenY);
}

void ExitMinigame(OMOBJ *object);

void UpdateCameraTransitionEffect(OMOBJ *object)
{
    float transitionBlend;
    float transitionProgress;
    float shakeOffsetX;
    float shakeOffsetZ;

    float *viewportCoordinates;
    s32 playerIndex;
    PlayerWork *playerWork;
    PlayerViewportWork *viewport;
    s32 iconPlayerIndex;
    RewardEffectWork *rewardEffectWork;
    PlayerWork *iconPlayerWork;

    s32 playersFinishedAnimationCount;
    PlayerWork *winningPlayerWork;
    PlayerWork *otherPlayerWork;
    if (omSysExitReq) {
        StopMinigameSequence(-1);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = ExitMinigame;
    }
    cameraTransitionDuration += 1.0f / REFRESH_RATE;
    if (360.0f <= cameraTransitionDuration) {
        cameraTransitionDuration -= 360.0f;
    }
    Hu3DModelRotSet(object->mdlId[2], 0, cameraTransitionDuration, 0);
    cameraModeFlags++;
    switch (object->work[0]) {

        case 1000:
            if (!WipeStatGet()) {
                object->work[0]++;
                transitionTimer = 0;
                HuAudFXPlay(1644);
            }
            break;

        case 1001:
            transitionTimer++;
            if (60.0f <= transitionTimer) {
                object->work[0]++;
                transitionTimer = 0;
            }
            break;

        case 1002:
            transitionTimer++;
            transitionProgress = transitionTimer / REFRESH_RATE;
            transitionBlend = 1 - (sind(transitionProgress * 90.0f) * sind(transitionProgress * 90.0f));
            for (playerIndex = 0; playerIndex < 5; playerIndex++) {
                espTPLvlSet(resultSpriteIds[playerIndex], transitionBlend);
            }
            if (REFRESH_RATE <= transitionTimer) {
                object->work[0]++;
                transitionTimer = 0;
                for (playerIndex = 0; playerIndex < 5; playerIndex++) {
                    espKill(resultSpriteIds[playerIndex]);
                }
            }
            break;

        case 1003:
            transitionTimer++;
            transitionProgress = transitionTimer / REFRESH_RATE;
            transitionBlend = sind(transitionProgress * 90.0f) * sind(transitionProgress * 90.0f);
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                playerViewports[playerIndex].distanceToObject = transitionBlend;
            }
            if (REFRESH_RATE <= transitionTimer) {
                object->work[0]++;
                transitionTimer = 0;
                audioEffectHandle = HuAudFXPlay(1645);
                currentAudioEffectHandle = HuAudFXPlay(1646);
            }
            break;

        case 1004:
            UpdateCameraViewports();
            if (120.0f <= transitionTimer) {
                object->work[0]++;
                HuAudFXStop(audioEffectHandle);
            }
            break;

        case 1005:
            if (winnerSequenceId < 0) {
                winnerSequenceId = MGSeqStartCreate();
            }
            else {
                if (minigameSequenceHandle < 0 && (MGSeqStatGet(winnerSequenceId) & 0x10)) {
                    PlayMinigameSequence(71);
                }
                if (!MGSeqStatGet(winnerSequenceId)) {
                    object->work[0]++;
                    currentMinigameFrames = 60 * REFRESH_RATE;
                    loserSequenceId = MGSeqTimerCreate(currentMinigameFrames / REFRESH_RATE);
                    winnerSequenceId = -1;
                }
            }
            break;

        case 1006:
            MGSeqParamSet(loserSequenceId, 1, (currentMinigameFrames + REFRESH_RATE - 1) / REFRESH_RATE);
            currentMinigameFrames--;
            if (currentMinigameFrames < 0 || winningTeam >= 0) {
                object->work[0] = 1007;
                currentMinigameFrames = 60 * REFRESH_RATE;
                transitionTimer = 0;
            }
            break;

        case 1007:
            if (winnerSequenceId < 0) {
                MGSeqParamSet(loserSequenceId, 2, -1);
                loserSequenceId = -1;
                winnerSequenceId = MGSeqFinishCreate();
                StopMinigameSequence(100);
                transitionTimer = 0;
                if (winningTeam >= 0) {
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
                    Hu3DModelAttrReset(object->mdlId[3], HU3D_MOTATTR_PAUSE);
                    Hu3DModelAttrReset(object->mdlId[3], HU3D_ATTR_DISPOFF);
                }
            }
            else {
                playersFinishedAnimationCount = 0;
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if (((PlayerWork *)playerObjects[playerIndex]->data)->behaviorState >= 2003) {
                        playersFinishedAnimationCount++;
                    }
                }
                if (playersFinishedAnimationCount == 4) {
                    transitionTimer++;
                }
                if (winningTeam >= 0 && 60.0f == transitionTimer) {
                    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
                    Hu3DModelAttrReset(object->mdlId[4], HU3D_ATTR_DISPOFF);
                    Hu3DModelAttrReset(object->mdlId[4], HU3D_MOTATTR_PAUSE);
                }
                if (60.0f < transitionTimer && !MGSeqStatGet(winnerSequenceId)) {
                    winnerSequenceId = -1;
                    transitionTimer = 0;
                    if (winningTeam < 0) {
                        object->work[0] = 1012;
                        transitionTimer = 0;
                        winnerSequenceId = MGSeqDrawCreate();
                        HuAudSStreamPlay(4);
                    }
                    else {
                        object->work[0] = 1008;
                        UpdateCameraFollow();
                    }
                }
            }
            break;

        case 1008:
            transitionTimer++;
            if (REFRESH_RATE < transitionTimer) {
                object->work[0]++;
                transitionTimer = 0;
                HuAudFXPlay(1650);
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if (winningTeam != playerIndex) {
                        Hu3DCameraKill(playerCameraIds[playerIndex]);
                    }
                }
            }
            else {
                InterpolateViewport(((winningTeam & 0x2) == 0) ? &horizontalSplitViewports[0][0] : &verticalSplitViewports[0][0]);
                transitionProgress = transitionTimer / REFRESH_RATE;
                transitionBlend = sind(transitionProgress * 90.0f) * sind(transitionProgress * 90.0f);
                InterpolateCameraView(winningTeam, &worldUpVector, transitionBlend);
            }
            break;

        case 1009:
            transitionTimer++;
            if (2 * REFRESH_RATE < transitionTimer) {
                object->work[0]++;
                transitionTimer = 0;
                HuAudFXPlay(1651);
            }
            else {
                transitionProgress = transitionTimer / (2 * REFRESH_RATE);
                transitionBlend = sind(transitionProgress * 90.0f) * sind(transitionProgress * 90.0f);
                shakeOffsetX = (6.0f * frandf()) - 3.0f;
                shakeOffsetZ = (6.0f * frandf()) - 3.0f;
                omSetTra(object, shakeOffsetX, -90 * transitionBlend, shakeOffsetZ);
                Hu3DModelPosSet(object->mdlId[3], shakeOffsetX, -90 * transitionBlend, shakeOffsetZ);
                Hu3DModelPosSet(object->mdlId[4], shakeOffsetX, -90 * transitionBlend, shakeOffsetZ);
            }
            break;

        case 1010:
            if (activeRewardCount >= 2) {
                object->work[0]++;
                transitionTimer = 0;
            }
            break;

        case 1011:
            if (collectedCoinCount > 0 && winnerSequenceId < 0) {
                winnerSequenceId = MGSeqWinCreate(GetPlayerBySeat(winningTeam)->characterId, GetPlayerBySeat(winningTeam + 1)->characterId, -1, -1);
                HuAudSStreamPlay(1);
            }
            if (minigameState < 20) {
                if ((cameraModeFlags % 6) == 0) {
                    minigameState++;
                    CreateCoinEffect(minigameState);
                }
            }
            else {
                if (minigameState == collectedCoinCount) {
                    transitionTimer++;
                    if (60.0f < transitionTimer) {
                        object->work[0] = 1012;
                        transitionTimer = 0;
                    }
                }
            }
            break;

        case 1012:
            transitionTimer++;
            if (210.0f < transitionTimer) {
                if (winnerSequenceId < 0 || !MGSeqStatGet(winnerSequenceId)) {
                    StopMinigameSequence(-1);
                    winnerSequenceId = -1;
                    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                    object->objFunc = ExitMinigame;
                }
            }
            break;

        default:
            OSReport("*** main mode error(%d)!!\n", object->work[0]);
            break;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        viewport = &playerViewports[playerIndex];
        if (viewport->viewportSpriteId >= 0) {
            espPosSet(viewport->viewportSpriteId, viewport->viewportCenterX, viewport->viewportCenterY);
            espScaleSet(viewport->viewportSpriteId, viewport->viewportScaleX, viewport->viewportScaleY);
            espTPLvlSet(viewport->viewportSpriteId, viewport->viewportOpacity);
            espDispOn(viewport->viewportSpriteId);
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerWork = playerObjects[playerIndex]->data;
        viewportCoordinates = &playerViewportLayouts[playerWork->playerSeat][0];
        if ((playerWork->stateFlags & 0x1) == 0) {
            continue;
        }
        ProjectWorldToScreen(viewportCoordinates, 0, 0, playerWork->actionPromptSprite);
        for (iconPlayerIndex = 0; iconPlayerIndex < 4; iconPlayerIndex++) {
            iconPlayerWork = playerObjects[iconPlayerIndex]->data;
            ProjectWorldToScreen(viewportCoordinates, iconPlayerWork->worldPosition.x, iconPlayerWork->worldPosition.z, playerWork->playerIconSprites[iconPlayerIndex]);
            rewardEffectWork = rewardEffectObjects[iconPlayerIndex]->data;
            ProjectWorldToScreen(viewportCoordinates, rewardEffectWork->worldPosition.x, rewardEffectWork->worldPosition.z, playerWork->rewardSprites[iconPlayerIndex][0]);
            ProjectWorldToScreen(viewportCoordinates, rewardEffectWork->worldPosition.x, rewardEffectWork->worldPosition.z, playerWork->rewardSprites[iconPlayerIndex][1]);
            if (rewardEffectWork->scalePulseEnabled & 0x1) {
                espDispOn(playerWork->rewardSprites[iconPlayerIndex][1]);
                espDispOff(playerWork->rewardSprites[iconPlayerIndex][0]);
            }
            else {
                espDispOn(playerWork->rewardSprites[iconPlayerIndex][0]);
                espDispOff(playerWork->rewardSprites[iconPlayerIndex][1]);
            }
        }
    }
}

void InitializeMinigameResult(OMOBJ *object)
{
    float overlayCenterX;
    float overlayCenterY;
    s32 playerIndex;
    RewardEffectWork *rewardEffectWork;
    overlayCenterX = 288;
    overlayCenterY = 240;
    resultSpriteIds[0] = espEntry(stageResultSpriteIds[currentStage->stageId + stageVariant], 132, 0);
    espPosSet(resultSpriteIds[0], overlayCenterX, overlayCenterY);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        rewardEffectWork = rewardEffectObjects[playerIndex]->data;
        resultSpriteIds[playerIndex + 1] = espEntry(rewardSpriteIds[playerIndex], 131, 0);
        espPosSet(resultSpriteIds[playerIndex + 1], overlayCenterX + (0.07f * rewardEffectWork->worldPosition.x), overlayCenterY + (0.07f * rewardEffectWork->worldPosition.z));
    }
    for (playerIndex = 0; playerIndex < 5; playerIndex++) {
        espDrawNoSet(resultSpriteIds[playerIndex], 0);
        espTPLvlSet(resultSpriteIds[playerIndex], 1.0f);
        espDispOn(resultSpriteIds[playerIndex]);
    }
    object->mdlId[0] = Hu3DModelCreateFile(currentStage->modelFiles[1]);
    Hu3DModelCameraSet(object->mdlId[0], 15);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[0], 0);
    Hu3DModelShadowMapSet(object->mdlId[0]);
    omSetTra(object, 0, 0, 0);
    omSetRot(object, 0, -90.0f * stageVariant, 0);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M429, 0x22));
    Hu3DModelCameraSet(object->mdlId[1], 15);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[1], 0);
    Hu3DModelShadowMapSet(object->mdlId[1]);
    Hu3DModelPosSet(object->mdlId[1], 0, 0, 0);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M429, 0x23));
    Hu3DModelCameraSet(object->mdlId[2], 15);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[2], 0);
    Hu3DModelPosSet(object->mdlId[2], 0, 0, 0);
    object->mdlId[3] = Hu3DModelCreateFile(currentStage->modelFiles[2]);
    Hu3DModelCameraSet(object->mdlId[3], 15);
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[3], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(object->mdlId[3], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[3], 1);
    Hu3DModelPosSet(object->mdlId[3], 0, 0, 0);
    Hu3DModelRotSet(object->mdlId[3], 0, -90.0f * stageVariant, 0);
    object->mdlId[4] = Hu3DModelCreateFile(currentStage->modelFiles[3]);
    Hu3DModelCameraSet(object->mdlId[4], 15);
    Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[4], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[4], 1);
    Hu3DModelPosSet(object->mdlId[4], 0, 0, 0);
    Hu3DModelRotSet(object->mdlId[4], 0, -90.0f * stageVariant, 0);
    cameraTransitionDuration = 45;
    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        Hu3DModelAttrReset(object->mdlId[playerIndex], HU3D_ATTR_DISPOFF);
    }
    object->work[0] = 1000;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    object->objFunc = UpdateCameraTransitionEffect;
}

void SetupMinigameObjects(void)
{
    s32 playerIndex;
    OMOBJ *playerObject;
    PlayerWork *playerWork;
    s32 candidateIndex;
    s32 lightingId;
    Vec shadowDirection;
    s32 sortOrder;
    s32 lowestSortOrder;
    s32 selectedPlayerIndex;

    OSReport("******* M429 ObjectSetup *********\n");
    nMap = 0;
    nChar = 0;
    objectManager = omInitObjMan(100, 8192);
    omGameSysInit(objectManager);
    cameraModeFlags = 0;
    winnerSequenceId = -1;
    loserSequenceId = -1;
    winningTeam = -1;
    minigameState = 0;
    collectedCoinCount = 0;
    activeRewardCount = 0;
    minigameSequenceHandle = -1;
    audioEffectHandle = -1;
    currentAudioEffectHandle = -1;
    cameraAzimuth = 0;
    cameraDistance = 850;
    cameraHeight = 450;
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        teamRewardTurnCounts[playerIndex] = 0;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerViewports[playerIndex].viewportSpriteId = -1;
        playerViewports[playerIndex].viewportOpacity = 0;
    }
    stageVariant = frandmod(4);
    InitializeStageObjects();
    InitializeCamera();
    lightingId = Hu3DGLightCreateV(&lightPosition, &lightDirection, &lightColor);
    Hu3DGLightStaticSet(lightingId, 1);
    Hu3DGLightInfinitytSet(lightingId);
    Hu3DShadowCreate(30, 10, 20000);
    Hu3DShadowTPLvlSet(0.5f);
    VECNormalize(&shadowSourcePosition, &shadowDirection);
    Hu3DShadowPosSet(&shadowSourcePosition, &shadowDirection, &shadowTargetPosition);
    omMakeGroupEx(objectManager, 0, 4);
    playerObjects = omGetGroupMemberListEx(objectManager, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObject = omAddObjEx(objectManager, 100, 1, 10, 0, UpdatePlayerObject);
        playerObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerWork), HU_MEMNUM_OVL);
        playerObject->work[0] = playerIndex;
        playerWork = playerObject->data;
        playerWork->playerSeat = -1;
        playerWork->playerIndex = playerObject->work[0];
        playerWork->playerGroup = GWPlayerCfg[playerWork->playerIndex].group;
    }
    (void)&selectedPlayerIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        selectedPlayerIndex = -1;
        lowestSortOrder = 40;
        for (candidateIndex = 0; candidateIndex < 4; candidateIndex++) {
            playerWork = playerObjects[candidateIndex]->data;
            if (playerWork->playerSeat < 0) {
                sortOrder = playerWork->playerIndex + (playerWork->playerGroup * 4);
                if (lowestSortOrder > sortOrder) {
                    lowestSortOrder = sortOrder;
                    selectedPlayerIndex = candidateIndex;
                }
            }
        }
        ((PlayerWork *)playerObjects[selectedPlayerIndex]->data)->playerSeat = playerIndex;
    }
    omMakeGroupEx(objectManager, 1, 4);
    attachedEffectObjects = omGetGroupMemberListEx(objectManager, 1);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObject = omAddObjEx(objectManager, 101, 2, 1, 1, InitializePlayerModels);
        playerObject->work[0] = playerIndex;
    }
    omMakeGroupEx(objectManager, 2, 4);
    rewardEffectObjects = omGetGroupMemberListEx(objectManager, 2);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObject = omAddObjEx(objectManager, 102, 2, 0, 2, InitializeRewardEffect);
        playerObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(RewardEffectWork), HU_MEMNUM_OVL);
        playerObject->work[0] = playerIndex;
    }
    sharedModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M429, 0x2F));
    Hu3DModelAttrSet(sharedModelId, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(sharedModelId, HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    omMakeGroupEx(objectManager, 3, 20);
    coinEffectObjects = omGetGroupMemberListEx(objectManager, 3);
    stageObject = omAddObjEx(objectManager, 104, 5, 0, -1, InitializeStageModel);
    MapObject[nMap++] = stageObject;
    gameControllerObject = omAddObjEx(objectManager, 105, 5, 0, -1, InitializeMinigameResult);
    particleAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M429, 0x3F));
    HuSprAnimLock(particleAnimation);
    particleManagerId = Hu3DParManCreate(particleAnimation, 20, &particleParameters);
    Hu3DParManAttrSet(particleManagerId, 1);
}

void ExitMinigame(OMOBJ *object)
{
    s32 playerIndex;
    if (currentAudioEffectHandle >= 0) {
        HuAudFXStop(currentAudioEffectHandle);
        currentAudioEffectHandle = -1;
    }
    if (WipeStatGet()) {
        return;
    }
    if (winnerSequenceId >= 0) {
        MGSeqKill(winnerSequenceId);
    }
    if (loserSequenceId >= 0) {
        MGSeqKill(loserSequenceId);
    }
    MGSeqKillAll();
    HuAudAllStop();
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        CharModelKill(characterModelIds[GWPlayerCfg[playerIndex].character]);
    }
    omOvlReturnEx(1, 1);
}
