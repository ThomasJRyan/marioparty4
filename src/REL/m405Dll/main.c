#include "REL/executor.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/esprite.h"
#include "game/flag.h"
#include "game/frand.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "ext_math.h"
#include "game/hsfex.h"
#include <string.h>


#ifndef __MWERKS__
#include "game/audio.h"
#endif

#undef ABS
#define ABS(x) ((0 > (x)) ? -(x) : (x))

typedef struct LinkedParticleHandle {
    /* 0x00 */ u16 linkedHandleResourceId;
    /* 0x02 */ s16 linkedHandleParameterId;
    /* 0x04 */ struct LinkedParticleHandle *linkedHandlePreviousEntry;
    /* 0x08 */ struct LinkedParticleHandle *linkedHandleNextEntry;
} LinkedParticleHandle; // Size 0xC

typedef struct ParticleGroupWork {
    /* 0x00 */ HU3DPARMANPARAM *particleGroupParticleParameters;
    /* 0x04 */ s32 *particleGroupParticleIds;
    /* 0x08 */ s32 particleGroupRemainingLifetime;
    /* 0x0C */ s32 particleGroupParticleCount;
    /* 0x10 */ struct ParticleGroupWork *particleGroupPreviousGroup;
    /* 0x14 */ struct ParticleGroupWork *particleGroupNextGroup;
} ParticleGroupWork; // Size 0x18

typedef struct {
    /* 0x00 */ u32 modelCacheResourceId;
    /* 0x04 */ s16 modelCacheModelId;
    /* 0x06 */ char alignmentPadding[2]; // padding?
} ModelCacheEntry; // Size 8

typedef struct {
    /* 0x00 */ Vec particleStartPosition;
    /* 0x0C */ Vec particleEndPosition;
    /* 0x18 */ float waveAmplitude;
    /* 0x1C */ float waveFrequency;
    /* 0x20 */ float wavePhase;
    /* 0x24 */ float waveSpeed;
    /* 0x28 */ s32 selectedPointCount;
    /* 0x2C */ u8 zDirection;
    /* 0x2D */ u8 updateState;
    /* 0x2E */ char alignmentPadding[2];
    /* 0x30 */ float *pointDistances;
    /* 0x34 */ u16 *pointIndices;
} WaterWaveWork; // Size 0x38

typedef struct {
    /* 0x00 */ Vec particlePosition;
    /* 0x0C */ float particleTimer;
} TrailParticleWork; // Size 0x10

typedef struct {
    /* 0x00 */ Vec *basePositions;
    /* 0x04 */ Vec *displacementPositions;
    /* 0x08 */ u8 (*vertexNormals)[0xC]; // Vec?
    /* 0x0C */ u16 (*cellVertexIndices)[4];
    /* 0x10 */ WaterWaveWork *particleWaves;
    /* 0x14 */ Vec *renderPositions;
    /* 0x18 */ Vec *renderNormals;
    /* 0x1C */ GXColor *vertexColors;
    /* 0x20 */ float (*textureCoordinates)[2];
    /* 0x24 */ void *displayListBuffer;
    /* 0x28 */ u32 displayListSize;
} WaterSurfaceMeshWork; // Size unknown

typedef struct {
    /* 0x00 */ void *texturePixels;
    /* 0x04 */ s32 textureBufferSize;
    /* 0x08 */ u16 textureWidth;
    /* 0x0A */ u16 textureHeight;
    /* 0x0C */ GXBool textureClearEnabled;
    /* 0x0D */ char alignmentPadding[3]; // padding?
} FramebufferTexture; // Size 0x10

typedef struct {
    /* 0x00 */ float effectDuration;
    /* 0x04 */ s32 effectType;
} MeterEffectSettings; // Size 8

typedef struct {
    /* 0x000 */ MeterEffectSettings *computerDifficultySettings;
    /* 0x004 */ s32 scoreLabelX;
    /* 0x008 */ s32 scoreLabelY;
    /* 0x00C */ s32 spriteIds[10];
    /* 0x034 */ float countdownValue;
    /* 0x038 */ float stateTimer;
    /* 0x03C */ float effectFrameTimer;
    /* 0x040 */ float effectDelay;
    /* 0x044 */ s32 difficultySetting;
    /* 0x048 */ s32 targetEffectCount;
    /* 0x04C */ Vec actorPosition;
    /* 0x058 */ float actorRotation;
    /* 0x05C */ float targetPositionZ;
    /* 0x060 */ float movementRate;
    /* 0x064 */ float movementAccumulator;
    /* 0x068 */ float remainingMeter;
    /* 0x06C */ float meterTickTimer;
    /* 0x070 */ float meterDecrement;
    /* 0x074 */ float meterIncrement;
    /* 0x078 */ u16 buttonInput;
    /* 0x07A */ char alignmentPadding2[2];
    /* 0x07C */ s32 motionCycleTimer;
    /* 0x080 */ s32 buttonRepeatTimer;
    /* 0x084 */ s32 successfulInputCount;
    /* 0x088 */ s32 inputFrameCounter;
    /* 0x08C */ s32 inputRepeatCounter;
    /* 0x090 */ s32 alternatingInputState;
    /* 0x094 */ s32 playerSide;
    /* 0x098 */ s32 trailPositionIndex;
    /* 0x09C */ s32 boundaryCollisionState;
    /* 0x0A0 */ Vec itemPositions[5];
    /* 0x0DC */ s32 itemEffectTimers[5];
    /* 0x0F0 */ s16 particleManagerIds[5];
    /* 0x0FA */ char alignmentPadding3[2];
    /* 0x0FC */ float boundaryTimer;
    /* 0x100 */ s32 objectState;
    /* 0x104 */ s32 motionIndex;
    /* 0x108 */ s32 playerIndex;
    /* 0x10C */ s32 characterId;
    /* 0x110 */ s32 controllerPort;
} M405PlayerWork; // Size 0x114

// Functions initialize_linked_particle_handles..update_player_rotation_from_input are identical to the ones in m402Dll.
// particle_link_list_sentinel..model_cache_entries seems to be the same as well.

void InitializeLinkedParticleHandles(void);
void RegisterParticleHandle(u16 resourceId, s16 parameterId);
s16 RemoveParticleHandle(s16 resourceId);
void InitializeParticleGroupList(void);
void CreateParticleGroup(s16 particleManagerId, HU3DPARMANPARAM *particleParameters, s32 renderLayer, s32 groupLifetime, s32 particleCount, Vec *particlePositions, u8 blendMode);
void UpdateParticleGroups(void);
void InitializeModelCache(void);
s16 CreateOrLinkCachedModel(s32 modelResourceId);
void SetTimerDigitSprites(s32 *spriteIds, s32 spriteStartIndex, s32 remainingFrames);
void SavePlayerRecord(s32 playerIndex);
void ShowTimerSprites(void);
void AnimateTimerSprites(void);
void CreateTimerSprites(float screenY, s32 labelResourceId);
void InterpolateCameraVector(float progress, Vec *startPosition, Vec *endPosition, Vec *interpolatedPosition);
void SetCameraView(Vec *cameraPosition, Vec *cameraTarget, Vec *cameraUp);
s32 UpdateCameraTransition(float progress, Vec *cameraTarget);
void SetCameraTransitionView(float progress, Vec *cameraTarget);
void ProjectWorldPositionToScreen(Vec *worldPosition, s16 cameraMask, Vec *screenPosition);
void GetModelHookPosition(s16 modelId, char *hookName, Vec *hookPosition);
void GetWorldModelHookPosition(s16 modelId, char *hookName, Vec *hookPosition, Mtx worldTransform);
float InterpolateAngle(float startAngle, float targetAngle, float progress);
void PlayMinigameMusic(s32 sequenceId);
void FadeMinigameMusic(s32 fadeDuration);
void UpdatePlayerRotationFromInput(Vec *playerInput, float *rotationAngle, s32 stickX, s32 stickY);
s32 GetWaterMeshVertexIndex(Vec *worldPosition);
void SelectWaveMeshPoints(WaterWaveWork *waveWork, float minimumX, float maximumX);
void UpdateWaveMeshPointDistances(WaterWaveWork *waveWork);
void CopyFramebufferToTexture(u8 textureIndex, s32 textureBuffer);
void UpdateWaterMeshGeometry(OMOBJ *object);
void CreateWaterMeshResources(OMOBJ *object);
void RenderWaterMesh(HU3DMODEL *model, Mtx modelTransform);
void UpdateWaterMeshNormals(HU3DMODEL *model, Mtx modelTransform);
void CopySceneFramebuffer(HU3DMODEL *model, Mtx modelTransform);
void RenderWaterSurface(HU3DMODEL *model, Mtx modelTransform);
void RenderWaterParticles(HU3DMODEL *model, Mtx modelTransform);
void UpdateWaterWaves(OMOBJ *object);
void CreateWaterEffectObject(OMOBJ *object);
void SetTrailParticlePosition(Vec *position, s32 trailIndex);
void StartWaterWave(Vec *position, float waveAmplitude, s32 waveIndex, s32 waveSide);
float GetWaterDisplacement(Vec *worldPosition);
void DebugPrintPlayerState(void);
void UpdateItemEffect(OMOBJ *object);
void CreateItemObject(OMOBJ *object);
void RecordPlayerTrailPosition(M405PlayerWork *playerWork, Vec *position);
void StartPlayerWaterWave(M405PlayerWork *playerWork);
void UpdatePlayerItemEffects(M405PlayerWork *playerWork, OMOBJ *object);
void UpdatePlayerScoreSprites(M405PlayerWork *playerWork);
void UpdatePlayerMeterSprite(M405PlayerWork *playerWork);
u16 ChooseComputerPlayerInput(M405PlayerWork *playerWork);
void UpdatePlayerMinigameState(OMOBJ *object);
void UpdatePlayerResults(OMOBJ *object);
void CreatePlayerObject(OMOBJ *object);
void InitializeMinigameScene(void);
void UpdateMinigameSequence(OMOBJ *object);
void CreateMinigameScene(OMOBJ *object);
void UpdateMinigameState(OMOBJ *object);

LinkedParticleHandle particleLinkListSentinel;
ParticleGroupWork particleGroupList;
HUPROCESS *objectManager;
Vec cameraTransitionStartPosition;
Vec cameraTransitionEndTarget;
Vec cameraTransitionStartUp;
Vec cameraTransitionEndUp;
u32 minigameFrameCounter;
s32 countdownFrames;
u32 recordedCountdownFrames;
s16 minigameMode;
s16 minigameSubstate;
s32 musicSequenceHandle;
s32 cameraTransitionActive;
float timerPulseAngle;
s32 timerSpriteIds[16];
ModelCacheEntry modelCacheEntries[32];
ANIMDATA *itemAnimationA;
ANIMDATA *itemAnimationB;
ANIMDATA *itemAnimationC;
ANIMDATA *itemAnimationD;
WaterSurfaceMeshWork waterMeshWork;
TrailParticleWork trailParticleWork[80];
u8 waterParticleRenderCount;
OMOBJ **playerObjects;
OMOBJ *minigameSceneObject;
OMOBJ *minigameEffectObject;
OMOBJ *minigameSequenceObject;
u8 reservedSceneData[4];
ANIMDATA *playerResultAnimation;
s16 itemParticleManagerId;
ANIMDATA *playerItemAnimation;
s16 itemParticleId;
float minigameElapsedTime;
s32 winningPlayerIndex;
s32 highestPlayerSide;
s32 minigameScore;
float sceneEffectTimer;
float itemEffectTimer;
s32 minigameStateValue;
s32 minigameStateTimer;
s32 minigameStateCounter;
Vec cameraPositionPair[2];
Vec cameraTargetPair[2];
Vec cameraUpPair[2];
Vec cameraAuxiliaryVectors[2];

s16 playerCharacterModelIds[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

FramebufferTexture framebufferTextureBuffers[] = { { NULL, 0, 640, 480, GX_FALSE }, { NULL, 0, 640, 480, GX_FALSE }, { NULL, 0, 320, 240, GX_TRUE } };

Vec waterWaveOriginPositions[] = { { -450.0f, 0.0f, 0.0f }, { -450.0f, 0.0f, 0.0f }, { -150.0f, 0.0f, 0.0f }, { -150.0f, 0.0f, 0.0f }, { 150.0f, 0.0f, 0.0f },
    { 150.0f, 0.0f, 0.0f }, { 450.0f, 0.0f, 0.0f }, { 450.0f, 0.0f, 0.0f } };

float waterWaveXRanges[]
    = { -600.0f, 300.0f, -600.0f, 300.0f, -300.0f, 300.0f, -300.0f, 300.0f, 0.0f, 300.0f, 0.0f, 300.0f, 300.0f, 300.0f, 300.0f, 300.0f };

u8 unusedMeterColorTable[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0x08, 0x08, 0x08, 0xFF };

GXColor meterLabelColor = { 0xD8, 0xE2, 0xE5, 0x10 };

void InitializeLinkedParticleHandles(void)
{
    particleLinkListSentinel.linkedHandlePreviousEntry = NULL;
    particleLinkListSentinel.linkedHandleNextEntry = NULL;
}

void RegisterParticleHandle(u16 resourceId, s16 parameterId)
{
    LinkedParticleHandle *newListEntry;
    LinkedParticleHandle *listSentinel;
    LinkedParticleHandle *previousHead;

    newListEntry = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*newListEntry), HU_MEMNUM_OVL);
    newListEntry->linkedHandleResourceId = resourceId;
    newListEntry->linkedHandleParameterId = parameterId;
    listSentinel = &particleLinkListSentinel;
    previousHead = listSentinel->linkedHandleNextEntry;
    listSentinel->linkedHandleNextEntry = newListEntry;
    if (NULL != previousHead) {
        previousHead->linkedHandlePreviousEntry = newListEntry;
    }
    newListEntry->linkedHandlePreviousEntry = listSentinel;
    newListEntry->linkedHandleNextEntry = previousHead;
}

s16 RemoveParticleHandle(s16 resourceId)
{
    LinkedParticleHandle *listEntry;
    LinkedParticleHandle *previousEntry;
    LinkedParticleHandle *nextEntry;
    s16 storedParameter;

    listEntry = particleLinkListSentinel.linkedHandleNextEntry;
    while (NULL != listEntry) {
        if (resourceId == listEntry->linkedHandleResourceId) {
            storedParameter = listEntry->linkedHandleParameterId;
            previousEntry = listEntry->linkedHandlePreviousEntry;
            nextEntry = listEntry->linkedHandleNextEntry;
            previousEntry->linkedHandleNextEntry = nextEntry;
            if (NULL != nextEntry) {
                nextEntry->linkedHandlePreviousEntry = previousEntry;
            }
            HuMemDirectFree(listEntry);
            return storedParameter;
        }
        listEntry = listEntry->linkedHandleNextEntry;
    }
    return -1;
}

void InitializeParticleGroupList(void)
{
    particleGroupList.particleGroupPreviousGroup = NULL;
    particleGroupList.particleGroupNextGroup = NULL;
}

void CreateParticleGroup(s16 particleManagerId, HU3DPARMANPARAM *particleParameters, s32 renderLayer, s32 groupLifetime, s32 particleCount, Vec *particlePositions, u8 blendMode)
{
    ParticleGroupWork *previousGroup;
    ParticleGroupWork *particleGroup;
    ParticleGroupWork *groupSentinel;
    s32 i;

    particleGroup = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*particleGroup), HU_MEMNUM_OVL);
    particleGroup->particleGroupParticleParameters = particleParameters;
    particleGroup->particleGroupRemainingLifetime = groupLifetime;
    particleGroup->particleGroupParticleCount = particleCount;
    particleGroup->particleGroupParticleIds = HuMemDirectMallocNum(HEAP_HEAP, particleCount * sizeof(*particleGroup->particleGroupParticleIds), HU_MEMNUM_OVL);
    for (i = 0; i < particleCount; i++, particlePositions++) {
        particleGroup->particleGroupParticleIds[i] = Hu3DParManLink(particleManagerId, particleParameters);
        if (particleGroup->particleGroupParticleIds[i] >= 0) {
            Hu3DParManAttrReset(particleGroup->particleGroupParticleIds[i], 1);
            Hu3DParManPosSet(particleGroup->particleGroupParticleIds[i], particlePositions->x, particlePositions->y, particlePositions->z);
            Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(particleGroup->particleGroupParticleIds[i]), blendMode);
            Hu3DModelLayerSet(Hu3DParManModelIDGet(particleGroup->particleGroupParticleIds[i]), renderLayer);
        }
    }
    groupSentinel = &particleGroupList;
    previousGroup = groupSentinel->particleGroupNextGroup;
    groupSentinel->particleGroupNextGroup = particleGroup;
    if (NULL != previousGroup) {
        previousGroup->particleGroupPreviousGroup = particleGroup;
    }
    particleGroup->particleGroupPreviousGroup = groupSentinel;
    particleGroup->particleGroupNextGroup = previousGroup;
}

void UpdateParticleGroups(void)
{
    ParticleGroupWork *particleGroup;
    ParticleGroupWork *previousGroup;
    ParticleGroupWork *nextGroup;
    s32 i;

    particleGroup = particleGroupList.particleGroupNextGroup;
    while (NULL != particleGroup) {
        particleGroup->particleGroupRemainingLifetime--;
        if (particleGroup->particleGroupRemainingLifetime == 0) {
            for (i = 0; i < particleGroup->particleGroupParticleCount; i++) {
                if (particleGroup->particleGroupParticleIds[i] >= 0) {
                    Hu3DParManAttrSet(particleGroup->particleGroupParticleIds[i], 1);
                }
            }
        }
        previousGroup = particleGroup->particleGroupPreviousGroup;
        nextGroup = particleGroup->particleGroupNextGroup;
        if (-particleGroup->particleGroupParticleParameters->maxTime > particleGroup->particleGroupRemainingLifetime) {
            for (i = 0; i < particleGroup->particleGroupParticleCount; i++) {
                if (particleGroup->particleGroupParticleIds[i] >= 0) {
                    Hu3DParManKill(particleGroup->particleGroupParticleIds[i]);
                }
            }
            previousGroup->particleGroupNextGroup = nextGroup;
            if (NULL != nextGroup) {
                nextGroup->particleGroupPreviousGroup = previousGroup;
            }
            HuMemDirectFree(particleGroup->particleGroupParticleIds);
            HuMemDirectFree(particleGroup);
        }
        particleGroup = nextGroup;
    }
}

void InitializeModelCache(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        modelCacheEntries[i].modelCacheResourceId = 0;
        modelCacheEntries[i].modelCacheModelId = -1;
    }
}

s16 CreateOrLinkCachedModel(s32 modelResourceId)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (modelResourceId == modelCacheEntries[i].modelCacheResourceId) {
            return Hu3DModelLink(modelCacheEntries[i].modelCacheModelId);
        }
    }
    for (i = 0; i < 32; i++) {
        if (modelCacheEntries[i].modelCacheResourceId == 0) {
            modelCacheEntries[i].modelCacheResourceId = modelResourceId;
            modelCacheEntries[i].modelCacheModelId = Hu3DModelCreateFile(modelResourceId);
            return modelCacheEntries[i].modelCacheModelId;
        }
    }
    return Hu3DModelCreateFile(modelResourceId);
}

void SetTimerDigitSprites(s32 *spriteIds, s32 spriteStartIndex, s32 remainingFrames)
{
    s32 digitIndex;

    espBankSet(spriteIds[spriteStartIndex + 1], 10);
    espBankSet(spriteIds[spriteStartIndex + 4], 11);
    if (600 * REFRESH_RATE <= remainingFrames) {
        remainingFrames = 600 * REFRESH_RATE - 1;
    }
    digitIndex = remainingFrames / (60 * REFRESH_RATE);
    remainingFrames -= digitIndex * (60 * REFRESH_RATE);
    if (digitIndex > 9) {
        digitIndex = 9;
    }
    espBankSet(spriteIds[spriteStartIndex], digitIndex);
    digitIndex = remainingFrames / REFRESH_RATE;
    remainingFrames -= digitIndex * REFRESH_RATE;
    espBankSet(spriteIds[spriteStartIndex + 2], digitIndex / 10);
    espBankSet(spriteIds[spriteStartIndex + 3], digitIndex % 10);
    digitIndex = 101.5f * remainingFrames / REFRESH_RATE;
    espBankSet(spriteIds[spriteStartIndex + 5], digitIndex / 10);
    espBankSet(spriteIds[spriteStartIndex + 6], digitIndex % 10);
}

void SavePlayerRecord(s32 playerIndex)
{
    timerPulseAngle = 0.0f;
    recordedCountdownFrames = countdownFrames;
    GWMGRecordSet(playerIndex, recordedCountdownFrames);
    SetTimerDigitSprites(timerSpriteIds, 7, countdownFrames);
}

void ShowTimerSprites(void)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        espDispOn(timerSpriteIds[i]);
    }
}

void AnimateTimerSprites(void)
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

void CreateTimerSprites(float screenY, s32 labelResourceId)
{
    float screenY;
    float screenX;
    s32 i;

    screenX = 288.0f;
    screenY = screenY;
    timerSpriteIds[15] = espEntry(labelResourceId, 0, 0);
    espTPLvlSet(timerSpriteIds[15], 0.5f);
    espColorSet(timerSpriteIds[15], 0, 0, 0);
    espPosSet(timerSpriteIds[15], screenX, screenY);
    screenX = 248.0f;
    screenY = screenY - 8.0f;
    timerSpriteIds[14] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 55), 0, 0);
    espPosSet(timerSpriteIds[14], screenX - 18.0f, screenY);
    for (i = 7; i <= 13; i++) {
        timerSpriteIds[i] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 48), 0, 0);
        espPosSet(timerSpriteIds[i], screenX, screenY);
        screenX += 16.0f;
    }
    SetTimerDigitSprites(timerSpriteIds, 7, recordedCountdownFrames);
    screenX = 248.0f;
    screenY = screenY + 8.0f;
    for (i = 0; i <= 6; i++) {
        timerSpriteIds[i] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 49), 0, 0);
        espPosSet(timerSpriteIds[i], screenX, screenY);
        screenX += 16.0f;
    }
    SetTimerDigitSprites(timerSpriteIds, 0, 0);
    for (i = 0; i < 16; i++) {
        espDrawNoSet(timerSpriteIds[i], 0);
        espDispOff(timerSpriteIds[i]);
        espAttrSet(timerSpriteIds[i], HUSPR_ATTR_NOANIM);
        espPriSet(timerSpriteIds[i], 0x80 + i);
    }
}

void InterpolateCameraVector(float progress, Vec *startPosition, Vec *endPosition, Vec *interpolatedPosition)
{
    float easedProgress;

    if (1.0f < progress) {
        progress = 1.0f;
    }
    easedProgress = sind(90.0f * progress) * sind(90.0f * progress);
    interpolatedPosition->x = startPosition->x + easedProgress * (endPosition->x - startPosition->x);
    interpolatedPosition->y = startPosition->y + easedProgress * (endPosition->y - startPosition->y);
    interpolatedPosition->z = startPosition->z + easedProgress * (endPosition->z - startPosition->z);
}

void SetCameraView(Vec *cameraPosition, Vec *cameraTarget, Vec *cameraUp)
{
    Hu3DCameraPosSetV(1, cameraPosition, cameraTarget, cameraUp);
    cameraTransitionStartPosition = *cameraPosition;
    cameraTransitionStartUp = *cameraUp;
}

s32 UpdateCameraTransition(float progress, Vec *cameraTarget)
{
    Vec cameraPosition;
    Vec cameraUp;

    InterpolateCameraVector(progress, &cameraTransitionStartPosition, &cameraTransitionEndTarget, &cameraPosition);
    InterpolateCameraVector(progress, &cameraTransitionStartUp, &cameraTransitionEndUp, &cameraUp);
    Hu3DCameraPosSetV(1, &cameraPosition, cameraTarget, &cameraUp);
    if (1.0f <= progress) {
        return 1;
    }
    else {
        cameraTransitionActive = 1;
        return 0;
    }
}

void SetCameraTransitionView(float progress, Vec *cameraTarget)
{
    Vec cameraPosition;
    Vec cameraUp;

    InterpolateCameraVector(progress, &cameraTransitionStartPosition, &cameraTransitionEndTarget, &cameraPosition);
    InterpolateCameraVector(progress, &cameraTransitionStartUp, &cameraTransitionEndUp, &cameraUp);
    SetCameraView(&cameraPosition, cameraTarget, &cameraUp);
    cameraTransitionActive = 1;
}

void ProjectWorldPositionToScreen(Vec *worldPosition, s16 cameraMask, Vec *screenPosition)
{
    Mtx localPosition;
    Vec rotatedPosition;
    float rotationSine;
    float rotationCosine;
    float positionX;
    float positionZ;
    HU3DCAMERA *modelData;
    s32 i;

    for (i = 0; i < 16; i++) {
        if (cameraMask & (1 << i)) {
            break;
        }
    }
    modelData = &Hu3DCamera[i];
    C_MTXLookAt(localPosition, &modelData->pos, &modelData->up, &modelData->target);
    MTXMultVec(localPosition, worldPosition, &rotatedPosition);
    rotationSine = rotatedPosition.z * (sind(modelData->fov / 2) / cosd(modelData->fov / 2)) * modelData->aspect;
    rotationCosine = rotatedPosition.z * (sind(modelData->fov / 2) / cosd(modelData->fov / 2));
    positionX = 0.9f * modelData->viewportX;
    positionZ = 0.9f * modelData->viewportW;
    screenPosition->x = positionZ / 2 + rotatedPosition.x * ((positionZ / 2) / -rotationSine) + positionX;
    screenPosition->y = modelData->viewportH / 2 + rotatedPosition.y * ((modelData->viewportH / 2) / rotationCosine) + modelData->viewportY;
    screenPosition->z = 0.0f;
}

void GetModelHookPosition(s16 modelId, char *hookName, Vec *hookPosition)
{
    Mtx modelHookPosition;

    Hu3DModelObjMtxGet(modelId, hookName, modelHookPosition);
    hookPosition->x = modelHookPosition[0][3];
    hookPosition->y = modelHookPosition[1][3];
    hookPosition->z = modelHookPosition[2][3];
}

void GetWorldModelHookPosition(s16 modelId, char *hookName, Vec *hookPosition, Mtx worldTransform)
{
    Mtx modelHookPosition;
    HU3DMODEL *modelData;

    modelData = &Hu3DData[modelId];
    Hu3DModelObjMtxGet(modelId, hookName, modelHookPosition);
    MTXConcat(modelData->mtx, modelHookPosition, modelHookPosition);
    if (NULL != worldTransform) {
        MTXCopy(modelHookPosition, worldTransform);
    }
    hookPosition->x = modelHookPosition[0][3];
    hookPosition->y = modelHookPosition[1][3];
    hookPosition->z = modelHookPosition[2][3];
}

float InterpolateAngle(float startAngle, float targetAngle, float progress)
{
    float smoothedValue;
    float difference;

    smoothedValue = fmod(targetAngle - startAngle, 360.0);
    if (0.0f > smoothedValue) {
        smoothedValue += 360.0f;
    }
    if (180.0f < smoothedValue) {
        smoothedValue -= 360.0f;
    }
    difference = fmod(startAngle + smoothedValue * progress, 360.0);
    if (0.0f > difference) {
        difference += 360.0f;
    }
    return difference;
}

void PlayMinigameMusic(s32 sequenceId)
{
    musicSequenceHandle = HuAudSeqPlay(sequenceId);
}

void FadeMinigameMusic(s32 fadeDuration)
{
    if (musicSequenceHandle >= 0) {
        if (fadeDuration < 0) {
            fadeDuration = 1000;
        }
        HuAudSeqFadeOut(musicSequenceHandle, fadeDuration);
    }
    musicSequenceHandle = -1;
}

void UpdatePlayerRotationFromInput(Vec *playerInput, float *rotationAngle, s32 stickX, s32 stickY)
{
    Vec rotatedVector;
    float rotationRadians;
    s32 axisIndex;

    rotatedVector.x = 0.0f;
    rotatedVector.y = 0.0f;
    rotatedVector.z = 0.0f;
    rotatedVector.x = stickX / 4.0f;
    rotatedVector.z = -(float)stickY / 4.0f;
    axisIndex = 0;
    rotationRadians = VECMagXZ(&rotatedVector);
    if (0.5f <= rotationRadians) {
        if (10.0f <= rotationRadians) {
            rotatedVector.x /= rotationRadians;
            rotatedVector.z /= rotationRadians;
            rotationRadians = 10.0f;
            rotatedVector.x = 10.0f * rotatedVector.x;
            rotatedVector.z = 10.0f * rotatedVector.z;
        }
        *rotationAngle = InterpolateAngle(*rotationAngle, atan2d(rotatedVector.x, rotatedVector.z), 0.4f);
        if (8.0f <= rotationRadians) {
            axisIndex = 2;
        }
        else {
            axisIndex = 1;
        }
    }
    VECAdd((Vec *)&playerInput, &rotatedVector, (Vec *)&playerInput);
}

s32 GetWaterMeshVertexIndex(Vec *worldPosition)
{
    float gridCellWidth;
    float gridCellDepth;
    s32 gridX;
    s32 gridZ;

    gridCellWidth = 41.37931f;
    gridCellDepth = 86.206894f;
    gridX = (worldPosition->x + gridCellWidth / 2 + 600.0f) / gridCellWidth;
    gridZ = (worldPosition->z + gridCellDepth / 2 + 1250.0f) / gridCellDepth;
    return gridX + gridZ * 30;
}

void SelectWaveMeshPoints(WaterWaveWork *waveWork, float minimumX, float maximumX)
{
    Vec *meshPosition;
    s32 i;

    waveWork->selectedPointCount = 0;
    meshPosition = waterMeshWork.basePositions;
    for (i = 0; i < 900; i++, meshPosition++) {
        if (minimumX <= meshPosition->x && maximumX >= meshPosition->x) {
            waveWork->pointIndices[waveWork->selectedPointCount] = i;
            waveWork->selectedPointCount++;
        }
    }
}

void UpdateWaveMeshPointDistances(WaterWaveWork *waveWork)
{
    Vec positionDelta;
    Vec *meshPosition;
    float *pointDistance;
    u16 *pointIndex;
    u32 i;

    if (waveWork->updateState == 3) {
        return;
    }
    if (waveWork->updateState == 2) {
        waveWork->updateState = 3;
    }
    pointIndex = waveWork->pointIndices;
    pointDistance = waveWork->pointDistances;
    for (i = 0; i < waveWork->selectedPointCount; i++, pointIndex++, pointDistance++) {
        meshPosition = &waterMeshWork.basePositions[*pointIndex];
        VECSubtract(meshPosition, &waveWork->particleEndPosition, &positionDelta);
        *pointDistance = VECMagXZ(&positionDelta);
        if (waveWork->updateState == 0 && waveWork->particleEndPosition.z > meshPosition->z) {
            *pointDistance *= -1.0f;
        }
        if (waveWork->updateState == 1 && waveWork->particleEndPosition.z < meshPosition->z) {
            *pointDistance *= -1.0f;
        }
    }
}

void CopyFramebufferToTexture(u8 textureIndex, s32 textureBuffer)
{
    GXColor copyClearColor = { 0x1E, 0x66, 0xA2, 0xFF };
    FramebufferTexture *textureBuffer;

    textureBuffer = &framebufferTextureBuffers[textureIndex];
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_TRUE);
    GXSetTexCopySrc(0, 0, 640, 480);
    GXSetTexCopyDst(textureBuffer->textureWidth, textureBuffer->textureHeight, GX_TF_RGB565, textureBuffer->textureClearEnabled);
    GXSetCopyClear(copyClearColor, 0xFFFFFF);
    GXCopyTex(textureBuffer->texturePixels, textureBuffer);
    DCFlushRange(textureBuffer->texturePixels, textureBuffer->textureBufferSize);
}

void UpdateWaterMeshGeometry(OMOBJ *object)
{
    Vec firstEdge;
    Vec secondEdge;
    Vec thirdEdge;
    Vec surfaceNormal;
    Vec *basePosition;
    Vec *displacement;
    Vec *renderPosition;
    Vec *renderPositionStart;
    Vec *normal;
    u16(*cellIndices)[4];
    s32 i;

    basePosition = waterMeshWork.basePositions;
    displacement = waterMeshWork.displacementPositions;
    renderPosition = waterMeshWork.renderPositions;
    for (i = 0; i < 900; i++) {
        VECAdd(basePosition, displacement, renderPosition);
        basePosition++;
        displacement++;
        renderPosition++;
    }
    renderPositionStart = waterMeshWork.renderPositions;
    cellIndices = waterMeshWork.cellVertexIndices;
    normal = waterMeshWork.renderNormals;
    memset(waterMeshWork.renderNormals, 0, 900 * sizeof(*waterMeshWork.renderNormals));
    for (i = 0; i < 841; i++) {
        VECSubtract(&renderPositionStart[(*cellIndices)[1]], &renderPositionStart[(*cellIndices)[0]], &firstEdge);
        VECSubtract(&renderPositionStart[(*cellIndices)[3]], &renderPositionStart[(*cellIndices)[0]], &secondEdge);
        VECSubtract(&renderPositionStart[(*cellIndices)[2]], &renderPositionStart[(*cellIndices)[0]], &thirdEdge);
        VECCrossProduct(&firstEdge, &secondEdge, &surfaceNormal);
        VECNormalize(&surfaceNormal, &surfaceNormal);
        VECAdd(&surfaceNormal, &normal[(*cellIndices)[0]], &normal[(*cellIndices)[0]]);
        VECAdd(&surfaceNormal, &normal[(*cellIndices)[1]], &normal[(*cellIndices)[1]]);
        VECAdd(&surfaceNormal, &normal[(*cellIndices)[3]], &normal[(*cellIndices)[3]]);
        VECCrossProduct(&secondEdge, &thirdEdge, &surfaceNormal);
        VECNormalize(&surfaceNormal, &surfaceNormal);
        VECAdd(&surfaceNormal, &normal[(*cellIndices)[0]], &normal[(*cellIndices)[0]]);
        VECAdd(&surfaceNormal, &normal[(*cellIndices)[3]], &normal[(*cellIndices)[3]]);
        VECAdd(&surfaceNormal, &normal[(*cellIndices)[2]], &normal[(*cellIndices)[2]]);
        cellIndices++;
    }
    for (i = 0; i < 900; i++, normal++) {
        VECNormalize(normal, normal);
    }
    DCStoreRangeNoSync(waterMeshWork.renderPositions, 900 * sizeof(*waterMeshWork.renderPositions));
    DCStoreRangeNoSync(waterMeshWork.renderNormals, 900 * sizeof(*waterMeshWork.renderNormals));
    DCStoreRangeNoSync(waterMeshWork.vertexColors, 900 * sizeof(*waterMeshWork.vertexColors));
    DCStoreRangeNoSync(waterMeshWork.textureCoordinates, 900 * sizeof(*waterMeshWork.textureCoordinates));
}

void CreateWaterMeshResources(OMOBJ *object)
{
    Vec meshPosition;
    Vec *meshPositionCursor;
    FramebufferTexture *textureBuffer;
    WaterWaveWork *particleWave;
    u16(*cellIndicesCursor)[4];
    s32 cellVertexIndex;
    s32 i;
    s32 j;

    itemAnimationA = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M405, 58));
    itemAnimationB = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M405, 59));
    itemAnimationC = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M405, 60));
    itemAnimationD = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M405, 61));
    waterMeshWork.basePositions = HuMemDirectMallocNum(HEAP_HEAP, 900 * sizeof(*waterMeshWork.basePositions), HU_MEMNUM_OVL);
    waterMeshWork.displacementPositions = HuMemDirectMallocNum(HEAP_HEAP, 900 * sizeof(*waterMeshWork.displacementPositions), HU_MEMNUM_OVL);
    waterMeshWork.vertexNormals = HuMemDirectMallocNum(HEAP_HEAP, 900 * sizeof(*waterMeshWork.vertexNormals), HU_MEMNUM_OVL);
    waterMeshWork.renderPositions = HuMemDirectMallocNum(HEAP_HEAP, 900 * sizeof(*waterMeshWork.renderPositions), HU_MEMNUM_OVL);
    waterMeshWork.renderNormals = HuMemDirectMallocNum(HEAP_HEAP, 900 * sizeof(*waterMeshWork.renderNormals), HU_MEMNUM_OVL);
    waterMeshWork.vertexColors = HuMemDirectMallocNum(HEAP_HEAP, 900 * sizeof(*waterMeshWork.vertexColors), HU_MEMNUM_OVL);
    waterMeshWork.textureCoordinates = HuMemDirectMallocNum(HEAP_HEAP, 900 * sizeof(*waterMeshWork.textureCoordinates), HU_MEMNUM_OVL);
    waterMeshWork.cellVertexIndices = HuMemDirectMallocNum(HEAP_HEAP, 841 * sizeof(*waterMeshWork.cellVertexIndices), HU_MEMNUM_OVL);
    waterMeshWork.particleWaves = HuMemDirectMallocNum(HEAP_HEAP, 8 * sizeof(*waterMeshWork.particleWaves), HU_MEMNUM_OVL);
    for (i = 0; i < 8; i++) {
        waterMeshWork.particleWaves[i].pointDistances = HuMemDirectMallocNum(HEAP_HEAP, 900 * sizeof(*waterMeshWork.particleWaves[i].pointDistances), HU_MEMNUM_OVL);
        waterMeshWork.particleWaves[i].pointIndices = HuMemDirectMallocNum(HEAP_HEAP, 900 * sizeof(*waterMeshWork.particleWaves[i].pointIndices), HU_MEMNUM_OVL);
    }
    waterMeshWork.displayListSize = 0x10000;
    waterMeshWork.displayListBuffer = HuMemDirectMallocNum(HEAP_HEAP, waterMeshWork.displayListSize, HU_MEMNUM_OVL);
    DCFlushRange(waterMeshWork.displayListBuffer, waterMeshWork.displayListSize);
    meshPositionCursor = waterMeshWork.basePositions;
    for (i = 0; i < 30; i++) {
        for (j = 0; j < 30; meshPositionCursor++, j++) {
            meshPositionCursor->x = 41.37931f * j - 600.0f;
            meshPositionCursor->y = 0.0f;
            meshPositionCursor->z = 86.206894f * i - 1250.0f;
        }
    }
    memset(waterMeshWork.displacementPositions, 0, 900 * sizeof(*waterMeshWork.displacementPositions));
    cellIndicesCursor = waterMeshWork.cellVertexIndices;
    for (i = 0; i < 29; i++) {
        for (j = 0; j < 29; j++) {
            if (!(i & 1)) {
                cellVertexIndex = j + i * 30;
                (*cellIndicesCursor)[0] = cellVertexIndex;
                (*cellIndicesCursor)[1] = cellVertexIndex + 30;
                (*cellIndicesCursor)[2] = cellVertexIndex + 1;
                (*cellIndicesCursor)[3] = cellVertexIndex + 31;
            }
            else {
                cellVertexIndex = j + i * 30;
                (*cellIndicesCursor)[1] = cellVertexIndex;
                (*cellIndicesCursor)[3] = cellVertexIndex + 30;
                (*cellIndicesCursor)[0] = cellVertexIndex + 1;
                (*cellIndicesCursor)[2] = cellVertexIndex + 31;
            }
            cellIndicesCursor++;
        }
    }
    particleWave = waterMeshWork.particleWaves;
    for (i = 0; i < 8; i++, particleWave++) {
        particleWave->particleStartPosition = waterWaveOriginPositions[i];
        particleWave->particleEndPosition = waterWaveOriginPositions[i];
        particleWave->waveAmplitude = 1.0f;
        particleWave->waveFrequency = 1.0f;
        particleWave->wavePhase = 0.0f;
        particleWave->waveSpeed = 3.6f;
        particleWave->selectedPointCount = 0;
        particleWave->zDirection = 0;
        particleWave->updateState = 2;
        particleWave->particleStartPosition.z = 900.0f * ((i & 1) ? -1.0f : 1.0f);
        particleWave->particleEndPosition.z = 900.0f * ((i & 1) ? -1.0f : 1.0f);
        particleWave->particleStartPosition = waterMeshWork.basePositions[GetWaterMeshVertexIndex(&particleWave->particleStartPosition)];
        particleWave->particleEndPosition = waterMeshWork.basePositions[GetWaterMeshVertexIndex(&particleWave->particleEndPosition)];
        SelectWaveMeshPoints(particleWave, waterWaveXRanges[i * 2], waterWaveXRanges[i * 2] + waterWaveXRanges[i * 2 + 1]);
        UpdateWaveMeshPointDistances(particleWave);
    }
    memcpy(waterMeshWork.renderPositions, waterMeshWork.basePositions, 900 * sizeof(*waterMeshWork.renderPositions));
    for (i = 0; i < 900; i++) {
        waterMeshWork.vertexColors[i].r = 0xF0;
        waterMeshWork.vertexColors[i].g = 0xFA;
        waterMeshWork.vertexColors[i].b = 0xFF;
        waterMeshWork.vertexColors[i].a = 0xC0;
    }
    for (i = 0; i < 30; i++) {
        for (j = 0; j < 30; j++) {
            cellVertexIndex = j + i * 30;
            waterMeshWork.textureCoordinates[cellVertexIndex][0] = 0.00083333335f * waterMeshWork.basePositions[cellVertexIndex].x;
            waterMeshWork.textureCoordinates[cellVertexIndex][1] = 0.0004f * waterMeshWork.basePositions[cellVertexIndex].z;
            waterMeshWork.textureCoordinates[cellVertexIndex][0] *= 3.0f;
            waterMeshWork.textureCoordinates[cellVertexIndex][1] *= 3.0f;
        }
    }
    for (i = 0; i < 80; i++) {
        trailParticleWork[i].particleTimer = 0.0f;
    }
    for (i = 0; i < 3; i++) {
        textureBuffer = &framebufferTextureBuffers[i];
        textureBuffer->textureBufferSize = GXGetTexBufferSize(textureBuffer->textureWidth, textureBuffer->textureHeight, GX_TF_RGB565, GX_FALSE, 0);
        textureBuffer->texturePixels = HuMemDirectMallocNum(HEAP_MODEL, textureBuffer->textureBufferSize, HU_MEMNUM_OVL);
    }
    UpdateWaterMeshGeometry(object);
    GXBeginDisplayList(waterMeshWork.displayListBuffer, waterMeshWork.displayListSize);
    for (i = 0; i < 29; i++) {
        GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 0x3C);
        for (j = 0; j < 30; j++) {
            GXPosition1x16(j + (i + 1) * 30);
            GXPosition1x16(j + i * 30);
        }
        GXEnd();
    }
    waterMeshWork.displayListSize = GXEndDisplayList();
    if (waterMeshWork.displayListSize == 0) {
        OSReport("DL buffer over!!\n");
    }
}

void RenderWaterMesh(HU3DMODEL *model, Mtx modelTransform)
{
    Mtx inverseTranspose;
    Mtx modelTransform;
    Mtx hookTransform;
    Mtx normalTransform;
    Mtx textureTransform;
    float textureScale;
    GXTexObj textureObject;
    FramebufferTexture *textureBuffer;

    GXLoadPosMtxImm(modelTransform, GX_PNMTX0);
    MTXInvXpose(modelTransform, inverseTranspose);
    GXLoadNrmMtxImm(inverseTranspose, GX_PNMTX0);
    textureBuffer = &framebufferTextureBuffers[2];
    GXInitTexObj(&textureObject, textureBuffer->texturePixels, textureBuffer->textureWidth, textureBuffer->textureHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObjLOD(&textureObject, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXLoadTexObj(&textureObject, GX_TEXMAP0);
    HuSprTexLoad(itemAnimationD, 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    HuSprTexLoad(itemAnimationB, 0, 2, GX_CLAMP, GX_CLAMP, GX_LINEAR);
    textureBuffer = &framebufferTextureBuffers[0];
    GXInitTexObj(&textureObject, textureBuffer->texturePixels, textureBuffer->textureWidth, textureBuffer->textureHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&textureObject, GX_TEXMAP3);
    GXSetNumTexGens(4);
    C_MTXLightPerspective(textureTransform, 30.0f, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, inverseTranspose);
    MTXConcat(inverseTranspose, modelTransform, inverseTranspose);
    MTXConcat(textureTransform, Hu3DCameraMtx, modelTransform);
    MTXConcat(modelTransform, inverseTranspose, modelTransform);
    GXLoadTexMtxImm(modelTransform, GX_TEXMTX0, GX_MTX3x4);
    C_MTXLightPerspective(textureTransform, 30.0f, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, inverseTranspose);
    MTXConcat(inverseTranspose, modelTransform, inverseTranspose);
    MTXConcat(textureTransform, Hu3DCameraMtx, modelTransform);
    MTXConcat(modelTransform, inverseTranspose, modelTransform);
    GXLoadTexMtxImm(modelTransform, GX_TEXMTX1, GX_MTX3x4);
    MTXRotRad(inverseTranspose, 'X', MTXDegToRad(-90));
    textureScale = 0.0004f;
    MTXScale(hookTransform, 0.00083333335f, textureScale, 0.0004f);
    MTXConcat(hookTransform, inverseTranspose, normalTransform);
    MTXTrans(modelTransform, 0.5f, 0.5f, 0.5f);
    MTXConcat(modelTransform, normalTransform, normalTransform);
    normalTransform[0][1] = -0.01f;
    GXLoadTexMtxImm(normalTransform, GX_TEXMTX2, GX_MTX2x4);
    MTXRotRad(inverseTranspose, 'X', MTXDegToRad(-90));
    textureScale = 0.0004f;
    MTXScale(hookTransform, textureScale, textureScale, textureScale);
    MTXConcat(hookTransform, inverseTranspose, normalTransform);
    MTXTrans(modelTransform, 0.5f, 0.5f, 0.0f);
    MTXConcat(modelTransform, normalTransform, normalTransform);
    GXLoadTexMtxImm(normalTransform, GX_TEXMTX3, GX_MTX2x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX1, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_POS, GX_TEXMTX2, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD3, GX_TG_MTX2x4, GX_TG_POS, GX_TEXMTX3, GX_FALSE, GX_PTIDENTITY);
    GXSetNumIndStages(2);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD2, GX_TEXMAP2);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
    MTXScale(modelTransform, -0.2f, -0.2f, 0.2f);
    GXSetIndTexMtx(GX_ITM_0, (float(*)[3])modelTransform, -2);
    GXSetTevIndWarp(GX_TEVSTAGE0, GX_INDTEXSTAGE0, GX_TRUE, GX_FALSE, GX_ITM_0);
    GXSetIndTexOrder(GX_INDTEXSTAGE1, GX_TEXCOORD3, GX_TEXMAP3);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE1, GX_ITS_1, GX_ITS_1);
    MTXScale(modelTransform, 0.5f, 0.5f, 0.5f);
    GXSetIndTexMtx(GX_ITM_1, (float(*)[3])modelTransform, 0);
    GXSetTevIndWarp(GX_TEVSTAGE1, GX_INDTEXSTAGE1, GX_TRUE, GX_FALSE, GX_ITM_1);
    GXSetNumTevStages(2);
    meterLabelColor.a = waterParticleRenderCount;
    GXSetTevColor(GX_TEVREG0, meterLabelColor);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_CPREV, GX_CC_TEXC, GX_CC_A0, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_RGBA6, 0);
    GXSetArray(GX_VA_POS, waterMeshWork.renderPositions, 0xC);
    GXSetCullMode(GX_CULL_BACK);
    GXCallDisplayList(waterMeshWork.displayListBuffer, waterMeshWork.displayListSize);
    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevDirect(GX_TEVSTAGE1);
}

void UpdateWaterMeshNormals(HU3DMODEL *model, Mtx modelTransform)
{
    UpdateWaterMeshGeometry(NULL);
}

void CopySceneFramebuffer(HU3DMODEL *model, Mtx modelTransform)
{
    CopyFramebufferToTexture(2, GX_FALSE);
}

void RenderWaterSurface(HU3DMODEL *model, Mtx modelTransform)
{
    Mtx inverseTranspose;
    Mtx modelTransform;
    Mtx hookTransform;
    GXTexObj normalTransform;
    FramebufferTexture *textureBuffer;

    CopyFramebufferToTexture(1, GX_FALSE);
    GXLoadPosMtxImm(modelTransform, GX_PNMTX0);
    C_MTXLightPerspective(inverseTranspose, 30.0f, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, modelTransform);
    MTXConcat(modelTransform, modelTransform, modelTransform);
    MTXConcat(inverseTranspose, Hu3DCameraMtx, hookTransform);
    MTXConcat(hookTransform, modelTransform, modelTransform);
    GXLoadTexMtxImm(modelTransform, GX_TEXMTX0, GX_MTX3x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    textureBuffer = &framebufferTextureBuffers[2];
    GXInitTexObj(&normalTransform, textureBuffer->texturePixels, textureBuffer->textureWidth, textureBuffer->textureHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&normalTransform, GX_TEXMAP0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_RGBA6, 0);
    GXSetZMode(GX_TRUE, GX_GREATER, GX_FALSE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-600.0f, -2.0f, -1250.0f);
    GXPosition3f32(600.0f, -2.0f, -1250.0f);
    GXPosition3f32(600.0f, -2.0f, 1250.0f);
    GXPosition3f32(-600.0f, -2.0f, 1250.0f);
    GXEnd();
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-600.0f, -100.0f, -1250.0f);
    GXPosition3f32(600.0f, -100.0f, -1250.0f);
    GXPosition3f32(600.0f, -100.0f, 1250.0f);
    GXPosition3f32(-600.0f, -100.0f, 1250.0f);
    GXEnd();
    CopyFramebufferToTexture(2, GX_FALSE);
    textureBuffer = &framebufferTextureBuffers[1];
    GXInitTexObj(&normalTransform, textureBuffer->texturePixels, textureBuffer->textureWidth, textureBuffer->textureHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&normalTransform, GX_TEXMAP0);
    GXSetZMode(GX_TRUE, GX_GREATER, GX_FALSE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-600.0f, -2.0f, -1250.0f);
    GXPosition3f32(600.0f, -2.0f, -1250.0f);
    GXPosition3f32(600.0f, -2.0f, 1250.0f);
    GXPosition3f32(-600.0f, -2.0f, 1250.0f);
    GXEnd();
}

void RenderWaterParticles(HU3DMODEL *model, Mtx modelTransform)
{
    Mtx inverseTranspose;
    Mtx44 modelTransform;
    float particleScale;
    GXColor particleTransform = { 0x00, 0x00, 0x00, 0xFF };
    GXColor normalTransform = { 0xFF, 0xFF, 0xFF, 0x20 };
    TrailParticleWork *particleParameters;
    s16 particleIndex;
    s16 particleModelId;
    s16 textureIndex;
    s16 particleFrame;
    s16 modelId;
    s16 i;

    C_MTXOrtho(modelTransform, 0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 100.0f);
    GXSetProjection(modelTransform, GX_ORTHOGRAPHIC);
    MTXIdentity(inverseTranspose);
    GXLoadPosMtxImm(inverseTranspose, GX_PNMTX0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanMatColor(GX_COLOR0A0, particleTransform);
    GXSetNumTexGens(0);
    HuSprTexLoad(itemAnimationC, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetScissor(0, 0, framebufferTextureBuffers->textureWidth, framebufferTextureBuffers->textureHeight);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_RGBX8, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition2u16(0, 0);
    GXPosition2u16(framebufferTextureBuffers->textureWidth, 0);
    GXPosition2u16(framebufferTextureBuffers->textureWidth, framebufferTextureBuffers->textureHeight);
    GXPosition2u16(0, framebufferTextureBuffers->textureHeight);
    GXEnd();
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_RGBA4, 0);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_RGBA6, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    particleParameters = trailParticleWork;
    modelId = 0;
    for (i = 0; i < 80; i++, particleParameters++) {
        if (0.1f >= particleParameters->particleTimer) {
            continue;
        }
        modelId = 1;
        particleScale = 200.0f * (1.0f - particleParameters->particleTimer);
        normalTransform.a = 255.0f * particleParameters->particleTimer;
        GXSetChanMatColor(GX_COLOR0A0, normalTransform);
        particleIndex = framebufferTextureBuffers->textureWidth * (1250.0f + particleParameters->particlePosition.x - particleScale) / 2500.0f;
        particleModelId = framebufferTextureBuffers->textureHeight * (1250.0f + particleParameters->particlePosition.z - particleScale) / 2500.0f;
        textureIndex = framebufferTextureBuffers->textureWidth * (1250.0f + particleParameters->particlePosition.x + particleScale) / 2500.0f;
        particleFrame = framebufferTextureBuffers->textureHeight * (1250.0f + particleParameters->particlePosition.z + particleScale) / 2500.0f;
        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        GXPosition2s16(particleIndex, particleModelId);
        GXTexCoord2f32(0.0f, 0.0f);
        GXPosition2s16(textureIndex, particleModelId);
        GXTexCoord2f32(1.0f, 0.0f);
        GXPosition2s16(textureIndex, particleFrame);
        GXTexCoord2f32(1.0f, 1.0f);
        GXPosition2s16(particleIndex, particleFrame);
        GXTexCoord2f32(0.0f, 1.0f);
        GXEnd();
    }
    if (modelId != 0 && waterParticleRenderCount < 24) {
        waterParticleRenderCount++;
    }
    CopyFramebufferToTexture(0, GX_TRUE);
}

void UpdateWaterWaves(OMOBJ *object)
{
    Vec particlePosition;
    float particleSpeed;
    float particleLifetime;
    float *particleManagerId;
    TrailParticleWork *effectState;
    WaterWaveWork *particleWave;
    u16 *particleIndex;
    s32 i;
    u32 j;

    memset(waterMeshWork.displacementPositions, 0, 900 * sizeof(*waterMeshWork.displacementPositions));
    effectState = trailParticleWork;
    for (i = 0; i < 80; i++, effectState++) {
        if (0.1f >= effectState->particleTimer) {
            continue;
        }
        effectState->particleTimer *= 0.95f;
    }
    particleWave = waterMeshWork.particleWaves;
    for (i = 0; i < 8; i++, particleWave++) {
        particleWave->wavePhase += particleWave->waveSpeed - particleWave->waveSpeed * frandf() / 2;
        if (360.0f <= particleWave->wavePhase) {
            particleWave->wavePhase -= 360.0f;
        }
        if (0.0f > particleWave->wavePhase) {
            particleWave->wavePhase += 360.0f;
        }
        particleWave->waveAmplitude *= 0.99f;
        particleWave->waveFrequency += 0.01f * (particleWave->waveAmplitude - particleWave->waveFrequency);
        particleWave->particleEndPosition.z += 0.02f * (particleWave->particleStartPosition.z - particleWave->particleEndPosition.z);
        if (5.0f > particleWave->waveFrequency) {
            particleWave->waveFrequency = 5.0f;
        }
        UpdateWaveMeshPointDistances(particleWave);
        particleSpeed = particleWave->wavePhase;
        particleIndex = particleWave->pointIndices;
        particleManagerId = particleWave->pointDistances;
        for (j = 0; j < particleWave->selectedPointCount; j++, particleIndex++, particleManagerId++) {
            if (0.0f > *particleManagerId) {
                continue;
            }
            particleLifetime = 1.0f - *particleManagerId / 625.0f;
            if (0.2f > particleLifetime) {
                particleLifetime = 0.2f;
            }
            waterMeshWork.displacementPositions[*particleIndex].y += particleWave->waveFrequency * particleLifetime * particleLifetime * sind(particleSpeed + *particleManagerId * particleLifetime * 3.6f);
        }
    }
}

void CreateWaterEffectObject(OMOBJ *object)
{
    waterParticleRenderCount = 0;
    omSetTra(object, 0.0f, 0.0f, 0.0f);
    CreateWaterMeshResources(object);
    object->mdlId[0] = Hu3DHookFuncCreate(RenderWaterParticles);
    Hu3DModelLayerSet(object->mdlId[0], 0);
    object->mdlId[1] = Hu3DHookFuncCreate(CopySceneFramebuffer);
    Hu3DModelLayerSet(object->mdlId[1], 2);
    object->mdlId[2] = Hu3DHookFuncCreate(RenderWaterSurface);
    Hu3DModelLayerSet(object->mdlId[2], 4);
    object->mdlId[3] = Hu3DHookFuncCreate(RenderWaterMesh);
    Hu3DModelLayerSet(object->mdlId[3], 6);
    object->mdlId[4] = Hu3DHookFuncCreate(UpdateWaterMeshNormals);
    Hu3DModelLayerSet(object->mdlId[4], 7);
    object->objFunc = UpdateWaterWaves;
}

void SetTrailParticlePosition(Vec *position, s32 trailIndex)
{
    TrailParticleWork *trailPosition;

    trailPosition = &trailParticleWork[trailIndex];
    trailPosition->particlePosition = *position;
    trailPosition->particleTimer = 1.0f;
}

void StartWaterWave(Vec *position, float waveAmplitude, s32 waveIndex, s32 waveSide)
{
    WaterWaveWork *particleWave;

    particleWave = &waterMeshWork.particleWaves[waveIndex + (waveSide ^ 1)];
    particleWave->zDirection = 0;
    particleWave->updateState = 2;
    particleWave = &waterMeshWork.particleWaves[waveIndex + waveSide];
    particleWave->updateState = waveSide;
    particleWave->waveAmplitude = waveAmplitude;
    particleWave->waveSpeed = 10.8f;
    particleWave->particleStartPosition.x = position->x;
    particleWave->particleStartPosition.y = 0.0f;
    particleWave->particleStartPosition.z = position->z;
    if (1.0f > particleWave->waveFrequency) {
        particleWave->wavePhase = 270.0f;
    }
    if (particleWave->zDirection == 0) {
        particleWave->zDirection = 1;
        particleWave->particleEndPosition = particleWave->particleStartPosition;
    }
}

float GetWaterDisplacement(Vec *worldPosition)
{
    return waterMeshWork.displacementPositions[GetWaterMeshVertexIndex(worldPosition)].y;
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

Vec worldUpVector = { 0.0f, 1.0f, 0.0f };
Vec initialCameraPosition = { 450.0f, 300.0f, 500.0f };
Vec initialCameraTarget = { 450.0f, 150.0f, 1300.0f };
Vec distantCameraTarget = { 0.0f, 800.0f, -3000.0f };
Vec itemCameraTarget = { 0.0f, 0.0f, -500.0f };
Vec farCameraPosition = { -2400.0f, 3500.0f, 0.0f };
Vec zeroPosition = { 0.0f, 0.0f, 0.0f };
Vec overheadCameraPosition = { 0.0f, 5000.0f, 1000.0f };
Vec cameraUpDirection = { 0.0f, -5.0f, -1.0f };
GXColor playerSpriteColor = { 0xFF, 0xFF, 0xFF, 0xFF };
Vec highCameraPosition = { 0.0f, 8000.0f, 1000.0f };
Vec originPosition = { 0.0f, 0.0f, 0.0f };

s32 playerCharacterMotionResources[8][8] = { { DATA_MAKE_NUM(DATADIR_MARIOMOT, 42), DATA_MAKE_NUM(DATADIR_M405, 0), DATA_MAKE_NUM(DATADIR_M405, 8),
                                 DATA_MAKE_NUM(DATADIR_M405, 16), DATA_MAKE_NUM(DATADIR_MARIOMOT, 43), DATA_MAKE_NUM(DATADIR_M405, 24),
                                 DATA_MAKE_NUM(DATADIR_M405, 32), DATA_MAKE_NUM(DATADIR_M405, 40) },
    { DATA_MAKE_NUM(DATADIR_LUIGIMOT, 42), DATA_MAKE_NUM(DATADIR_M405, 1), DATA_MAKE_NUM(DATADIR_M405, 9), DATA_MAKE_NUM(DATADIR_M405, 17),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 43), DATA_MAKE_NUM(DATADIR_M405, 25), DATA_MAKE_NUM(DATADIR_M405, 33), DATA_MAKE_NUM(DATADIR_M405, 41) },
    { DATA_MAKE_NUM(DATADIR_PEACHMOT, 42), DATA_MAKE_NUM(DATADIR_M405, 2), DATA_MAKE_NUM(DATADIR_M405, 10), DATA_MAKE_NUM(DATADIR_M405, 18),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 43), DATA_MAKE_NUM(DATADIR_M405, 26), DATA_MAKE_NUM(DATADIR_M405, 34), DATA_MAKE_NUM(DATADIR_M405, 42) },
    { DATA_MAKE_NUM(DATADIR_YOSHIMOT, 42), DATA_MAKE_NUM(DATADIR_M405, 3), DATA_MAKE_NUM(DATADIR_M405, 11), DATA_MAKE_NUM(DATADIR_M405, 19),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 43), DATA_MAKE_NUM(DATADIR_M405, 27), DATA_MAKE_NUM(DATADIR_M405, 35), DATA_MAKE_NUM(DATADIR_M405, 43) },
    { DATA_MAKE_NUM(DATADIR_WARIOMOT, 42), DATA_MAKE_NUM(DATADIR_M405, 4), DATA_MAKE_NUM(DATADIR_M405, 12), DATA_MAKE_NUM(DATADIR_M405, 20),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 43), DATA_MAKE_NUM(DATADIR_M405, 28), DATA_MAKE_NUM(DATADIR_M405, 36), DATA_MAKE_NUM(DATADIR_M405, 44) },
    { DATA_MAKE_NUM(DATADIR_DONKEYMOT, 42), DATA_MAKE_NUM(DATADIR_M405, 5), DATA_MAKE_NUM(DATADIR_M405, 13), DATA_MAKE_NUM(DATADIR_M405, 21),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 43), DATA_MAKE_NUM(DATADIR_M405, 29), DATA_MAKE_NUM(DATADIR_M405, 37), DATA_MAKE_NUM(DATADIR_M405, 45) },
    { DATA_MAKE_NUM(DATADIR_DAISYMOT, 42), DATA_MAKE_NUM(DATADIR_M405, 6), DATA_MAKE_NUM(DATADIR_M405, 14), DATA_MAKE_NUM(DATADIR_M405, 22),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 43), DATA_MAKE_NUM(DATADIR_M405, 30), DATA_MAKE_NUM(DATADIR_M405, 38), DATA_MAKE_NUM(DATADIR_M405, 46) },
    { DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 42), DATA_MAKE_NUM(DATADIR_M405, 7), DATA_MAKE_NUM(DATADIR_M405, 15), DATA_MAKE_NUM(DATADIR_M405, 23),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 43), DATA_MAKE_NUM(DATADIR_M405, 31), DATA_MAKE_NUM(DATADIR_M405, 39), DATA_MAKE_NUM(DATADIR_M405, 47) } };

s32 itemModelMotionResources[] = { DATA_MAKE_NUM(DATADIR_M405, 55), DATA_MAKE_NUM(DATADIR_M405, 56) };

s32 playerPortraitResources[] = { DATA_MAKE_NUM(DATADIR_MGCONST, 0), DATA_MAKE_NUM(DATADIR_MGCONST, 1), DATA_MAKE_NUM(DATADIR_MGCONST, 2),
    DATA_MAKE_NUM(DATADIR_MGCONST, 3), DATA_MAKE_NUM(DATADIR_MGCONST, 4), DATA_MAKE_NUM(DATADIR_MGCONST, 5), DATA_MAKE_NUM(DATADIR_MGCONST, 6),
    DATA_MAKE_NUM(DATADIR_MGCONST, 7) };

s32 playerScoreLabelOffsets[] = { -224, -128, 128, 224 };

s32 playerItemOffsets[] = { 0, 0, 0, 0, -20, -18, 19, -18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

s32 scoreDigitOffsets[] = { -24, 17, 24, 17, 0, 17 };
s32 characterItemHookIndices[] = { 1, 0, 3, 2, 4 };

u8 playerButtonPatterns[][3] = { { 1, 0, 1 }, { 2, 3, 0 }, { 1, 0, 0 } };

u16 playerButtonMasks[] = { 0x0100, 0x0200, 0x0040, 0x0020, 0x0000 };

MeterEffectSettings computerDifficultyEasySettings = { REFRESH_RATE*59.5f, 2 };
MeterEffectSettings computerDifficultyNormalSettings = { REFRESH_RATE*55.5f, 2 };
MeterEffectSettings computerDifficultyHardSettings = { REFRESH_RATE*53, 2 };
MeterEffectSettings computerDifficultyExpertSettings = { REFRESH_RATE*51, 6 };

HU3DPARMANPARAM waterSplashParticleParameters = { 0x0014, { 0x00, 0x00 }, // padding?
    1.0f, 30.0f, 0.0f, { 0.0f, 0.05f, 0.0f }, 1.0f, 0.999f, 30.0f, 1.03f, 0x0004,
    { { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF } },
    { { 0xFF, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 } } };

HU3DPARMANPARAM waterWaveParticleParameters = { 0x0014, { 0x00, 0x00 }, // padding?
    1.0f, 10.0f, 360.0f, { 0.0f, 0.1f, 0.0f }, 1.0f, 1.0f, 50.0f, 1.05f, 0x0001,
    { { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF } },
    { { 0xFF, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 } } };

void DebugPrintPlayerState(void)
{
    M405PlayerWork *playerWork;
    s32 i;

    fontcolor = 0;
    for (i = 0; i < 4; i++) {
        playerWork = playerObjects[i]->data;
        print8(24, i * 16 + 132, 1.5f, "%d %f %f %f", playerWork->playerIndex, playerWork->countdownValue, playerWork->stateTimer, playerWork->effectFrameTimer);
    }
}

void UpdateItemEffect(OMOBJ *object)
{
    Vec itemPosition;
    s16 modelId;

    switch (object->work[0]) {
        case 0:
            if (minigameSequenceObject->work[0] >= 1004 && 60.0f < minigameElapsedTime) {
                object->work[0]++;
                itemEffectTimer = 0.0f;
                Hu3DMotionSet(object->mdlId[0], object->mtnId[1]);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
                Hu3DModelAttrReset(object->mdlId[1], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
            }
            break;
        case 1:
            itemEffectTimer += 1.0f;
            modelId = object->mdlId[0];
            GetModelHookPosition(modelId, "g001m1_021-itemhook_r", &itemPosition);
            Hu3DParManAttrSet(itemParticleId, 1);
            if (50.0f == itemEffectTimer) {
                HuAudFXPlay(0xD);
            }
            if (50.0f <= itemEffectTimer && 60.0f > itemEffectTimer) {
                Hu3DParManPosSet(itemParticleId, itemPosition.x - 10.0f, itemPosition.y + 35.0f, itemPosition.z - 20.0f);
                Hu3DParManAttrReset(itemParticleId, 1);
            }
            if (Hu3DMotionEndCheck(object->mdlId[0]) != 0) {
                object->work[0]++;
                Hu3DMotionSet(object->mdlId[0], object->mtnId[0]);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
                Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
            }
            break;
        case 2:
            break;
    }
}

void CreateItemObject(OMOBJ *object)
{
    s32 i;

    object->work[0] = 0;
    omSetTra(object, 850.0f, 30.0f, 1350.0f);
    omSetRot(object, 0.0f, -90.0f, 0.0f);
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M405, 53));
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M405, 54));
    for (i = 0; i < 2; i++) {
        object->mtnId[i] = Hu3DJointMotionFile(object->mdlId[0], itemModelMotionResources[i]);
    }
    for (i = 0; i < 2; i++) {
        Hu3DModelAttrSet(object->mdlId[i], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(object->mdlId[i], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
        Hu3DModelLayerSet(object->mdlId[i], 5);
        Hu3DModelShadowSet(object->mdlId[i]);
    }
    Hu3DMotionSet(object->mdlId[0], *object->mtnId);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelHookSet(object->mdlId[0], "g001m1_021-itemhook_r", object->mdlId[1]);
    Hu3DMotionTimeSet(object->mdlId[1], 1.0f);
    for (i = 0; i < 2; i++) {
        Hu3DModelAttrReset(object->mdlId[i], HU3D_ATTR_DISPOFF);
    }
    object->objFunc = UpdateItemEffect;
}

void RecordPlayerTrailPosition(M405PlayerWork *playerWork, Vec *position)
{
    SetTrailParticlePosition(position, playerWork->trailPositionIndex + playerWork->playerIndex * 20);
    playerWork->trailPositionIndex = (playerWork->trailPositionIndex + 1) % 20;
}

void StartPlayerWaterWave(M405PlayerWork *playerWork)
{
    Vec particleOrigin;
    float particleRange;
    WaterWaveWork *particleWave;

    particleRange = 30.0f;
    if (playerWork->motionIndex == 2) {
        particleRange = 20.0f;
    }
    if (playerWork->motionIndex == 4) {
        particleRange = 40.0f;
    }
    particleOrigin = playerWork->actorPosition;
    particleOrigin.z -= 70.0f * (playerWork->playerSide == 1 ? 1.0f : -1.0f);
    if (1250.0f < ABS(particleOrigin.z)) {
        particleOrigin.z = 1250.0f * (0.0f > particleOrigin.z ? -1 : 1);
    }
    StartWaterWave(&particleOrigin, particleRange, (3 - playerWork->playerIndex) * 2, playerWork->playerSide & 1);
}

void UpdatePlayerItemEffects(M405PlayerWork *playerWork, OMOBJ *object)
{
    Vec previousItemPositions[5];
    s32 crossingCount;
    s32 itemHitCount;
    s32 i;

    crossingCount = 0;
    itemHitCount = 0;
    for (i = 0; i < 5; i++) {
        previousItemPositions[i] = playerWork->itemPositions[i];
        if (minigameFrameCounter % 5 == i) {
            GetModelHookPosition(object->mdlId[0], CharModelItemHookGet(playerWork->characterId, 4, characterItemHookIndices[i]), &playerWork->itemPositions[i]);
        }
        if ((0.0f <= previousItemPositions[i].y && 0.0f > playerWork->itemPositions[i].y) || (0.0f >= previousItemPositions[i].y && 0.0f < playerWork->itemPositions[i].y)) {
            itemHitCount++;
            playerWork->itemEffectTimers[i] = 10;
            if (i < 2) {
                crossingCount++;
                RecordPlayerTrailPosition(playerWork, &playerWork->itemPositions[i]);
            }
        }
        Hu3DParManPosSet(playerWork->particleManagerIds[i], playerWork->itemPositions[i].x, playerWork->itemPositions[i].y, playerWork->itemPositions[i].z);
        if (--playerWork->itemEffectTimers[i] <= 0) {
            Hu3DParManAttrSet(playerWork->particleManagerIds[i], 1);
        }
        else {
            Hu3DParManAttrReset(playerWork->particleManagerIds[i], 1);
        }
    }
    if (crossingCount > 0) {
        HuAudFXPlay(0x535);
    }
    if (itemHitCount > 0) {
        StartPlayerWaterWave(playerWork);
        if (playerWork->boundaryCollisionState != 0) {
            playerWork->boundaryCollisionState = 0;
            omVibrate(playerWork->playerIndex, 12, 4, 2);
            HuAudFXPlay(0x536);
        }
    }
}

void UpdatePlayerScoreSprites(M405PlayerWork *playerWork)
{
    u8 *digitLayout;
    s32 i;

    digitLayout = playerButtonPatterns[playerWork->playerSide];
    for (i = 4; i < 10; i++) {
        espDispOff(playerWork->spriteIds[i]);
    }
    espPosSet(playerWork->spriteIds[digitLayout[0] + 6], playerWork->scoreLabelX + scoreDigitOffsets[0], playerWork->scoreLabelY + scoreDigitOffsets[1]);
    espDispOn(playerWork->spriteIds[digitLayout[0] + 6]);
    espPosSet(playerWork->spriteIds[digitLayout[1] + 6], playerWork->scoreLabelX + scoreDigitOffsets[2], playerWork->scoreLabelY + scoreDigitOffsets[3]);
    espDispOn(playerWork->spriteIds[digitLayout[1] + 6]);
    espPosSet(playerWork->spriteIds[digitLayout[2] + 4], playerWork->scoreLabelX + scoreDigitOffsets[4], playerWork->scoreLabelY + scoreDigitOffsets[5]);
    espDispOn(playerWork->spriteIds[digitLayout[2] + 4]);
}

void UpdatePlayerMeterSprite(M405PlayerWork *playerWork)
{
    s32 digitBank;

    digitBank = (113.0f - playerWork->remainingMeter) / 10.0f - 1.0f;
    if (digitBank > 9) {
        espDispOff(playerWork->spriteIds[3]);
    }
    else {
        espDispOn(playerWork->spriteIds[3]);
        espBankSet(playerWork->spriteIds[3], digitBank);
    }
}

u16 ChooseComputerPlayerInput(M405PlayerWork *playerWork)
{
    u16 buttonResult;
    u8 *digitLayout;

    buttonResult = 0;
    playerWork->countdownValue -= 1.0f;
    if (6.0f > playerWork->countdownValue) {
        playerWork->countdownValue = 6.0f;
    }
    if (50.0f > playerWork->stateTimer) {
        playerWork->stateTimer = 50.0f;
    }
    playerWork->effectFrameTimer -= 1.0f;
    if (0.0f > playerWork->effectFrameTimer) {
        playerWork->effectFrameTimer = playerWork->countdownValue / (playerWork->stateTimer / 30.0f);
        if (6.0f > playerWork->effectFrameTimer) {
            playerWork->effectFrameTimer = 6.0f;
        }
        if (REFRESH_RATE*7 < playerWork->countdownValue && 60.0f > playerWork->remainingMeter && playerWork->inputFrameCounter < 10) {
            playerWork->effectDelay = REFRESH_RATE/3.0f;
        }
        if (playerWork->difficultySetting >= 2 && 70.0f < playerWork->remainingMeter) {
            playerWork->effectFrameTimer = 8.0f;
        }
        else if (0.0f != playerWork->effectDelay) {
            playerWork->effectFrameTimer = playerWork->effectDelay;
        }
        else if (playerWork->targetEffectCount > 0) {
            if ((playerWork->targetEffectCount & 1) == 1) {
                playerWork->stateTimer += 40.0f * (playerWork->difficultySetting + 1);
                playerWork->effectFrameTimer = 8.0f;
                playerWork->targetEffectCount--;
            }
            else if (frandmod(10) == 0) {
                playerWork->effectFrameTimer = 8.0f;
                playerWork->targetEffectCount--;
            }
        }
        digitLayout = playerButtonPatterns[playerWork->playerSide];
        buttonResult = playerButtonMasks[digitLayout[0]] | playerButtonMasks[digitLayout[1]];
    }
    if (0.0f != playerWork->effectDelay) {
        if (30.0f < playerWork->remainingMeter) {
            playerWork->effectDelay = 0.0f;
        }
    }
    else if (10.0f > playerWork->remainingMeter) {
        playerWork->effectDelay = REFRESH_RATE/3.0f;
    }
    return buttonResult;
}

void UpdatePlayerMinigameState(OMOBJ *object)
{
    float waveStep;
    float waveDirection;
    M405PlayerWork *playerWork;
    u8 *sequencePattern;
    u16 requiredButtons;
    s32 playerIndex;
    s32 motionIndex;
    s32 i;

    playerWork = object->data;
    if (minigameSequenceObject->work[0] == 1006) {
        playerWork->objectState = 2005;
        playerWork->motionIndex = 0;
        Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[playerWork->motionIndex], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
        Hu3DMotionSpeedSet(object->mdlId[0], 1.0f);
        for (i = 0; i < 5; i++) {
            Hu3DParManAttrSet(playerWork->particleManagerIds[i], 1);
        }
        if (winningPlayerIndex == playerWork->playerIndex) {
            playerIndex = playerWork->playerIndex;
            if (!_CheckFlag(FLAG_ID_MAKE(1, 12))) {
                GWPlayer[playerIndex].coinWin = 10;
            }
        }
        object->objFunc = UpdatePlayerResults;
        return;
    }
    if (GWPlayerCfg[playerWork->playerIndex].iscom) {
        playerWork->buttonInput = ChooseComputerPlayerInput(playerWork);
    }
    else {
        playerWork->buttonInput |= HuPadBtnDown[playerWork->controllerPort];
    }
    motionIndex = playerWork->motionIndex;
    switch (playerWork->objectState) {
        case 2003:
            sequencePattern = playerButtonPatterns[playerWork->playerSide];
            if (sequencePattern[2] == 1) {
                requiredButtons = playerButtonMasks[sequencePattern[0]] | playerButtonMasks[sequencePattern[1]];
            }
            else {
                requiredButtons = playerButtonMasks[sequencePattern[playerWork->alternatingInputState & 1]];
            }
            playerWork->meterTickTimer += 1.0f;
            if ((REFRESH_RATE/3.0f) <= playerWork->meterTickTimer) {
                playerWork->remainingMeter += playerWork->meterIncrement;
                if (0.0f == playerWork->meterDecrement) {
                    if (5.0f < playerWork->meterIncrement) {
                        playerWork->meterIncrement -= 1.0f;
                    }
                }
                else {
                    playerWork->meterIncrement = 7.0f;
                }
                playerWork->meterTickTimer = 0.0f;
                playerWork->meterDecrement = 0.0f;
                if (100.0f <= playerWork->remainingMeter) {
                    playerWork->remainingMeter = 100.0f;
                }
            }
            playerWork->inputFrameCounter++;
            if (1.0f <= playerWork->remainingMeter && requiredButtons == (playerWork->buttonInput & requiredButtons)) {
                playerWork->buttonInput = 0;
                playerWork->inputRepeatCounter = 0;
                playerWork->successfulInputCount++;
                if (playerWork->buttonRepeatTimer == 0) {
                    playerWork->buttonRepeatTimer = 1;
                }
                if (playerWork->successfulInputCount < 10) {
                    playerWork->alternatingInputState ^= 1;
                    waveStep = 30.0f;
                    if (playerWork->inputFrameCounter < 10) {
                        waveStep = 35.0f;
                    }
                    if (playerWork->playerSide == 1) {
                        waveDirection = 1.0f;
                    }
                    else {
                        waveDirection = -1.0f;
                    }
                    playerWork->targetPositionZ += waveStep * waveDirection;
                    playerWork->remainingMeter -= 5.0f;
                    playerWork->meterDecrement += 5.0f;
                    if (0.0f >= playerWork->remainingMeter) {
                        playerWork->remainingMeter = 0.0f;
                        playerWork->targetPositionZ = playerWork->actorPosition.z;
                    }
                }
                playerWork->inputFrameCounter = 0;
            }
            if (playerWork->buttonRepeatTimer > 0) {
                if (++playerWork->buttonRepeatTimer > REFRESH_RATE) {
                    playerWork->buttonRepeatTimer = 0;
                    playerWork->successfulInputCount = 0;
                }
            }
            if (sequencePattern[2] == 1) {
                if (playerWork->buttonInput != 0) {
                    if (++playerWork->inputRepeatCounter >= 2) {
                        playerWork->buttonInput = 0;
                        playerWork->inputRepeatCounter = 0;
                    }
                }
            }
            else {
                playerWork->buttonInput = 0;
            }
            if (1.0f > ABS(playerWork->targetPositionZ - playerWork->actorPosition.z)) {
                playerWork->motionCycleTimer++;
                if ((REFRESH_RATE/2.0f) == playerWork->motionCycleTimer) {
                    motionIndex = 0;
                }
            }
            else {
                playerWork->motionCycleTimer = 0;
                motionIndex = playerWork->playerSide + 2;
                waveStep = 0.1f * (playerWork->targetPositionZ - playerWork->actorPosition.z);
                playerWork->actorPosition.z += waveStep;
                playerWork->stateTimer -= ABS(waveStep);
                playerWork->movementAccumulator = (playerWork->movementAccumulator + ABS(waveStep)) / 2;
                if (playerWork->playerSide >= 2) {
                    if (1150.0f < ABS(playerWork->actorPosition.z)) {
                        playerWork->actorPosition.z = 1150.0f * (0.0f > playerWork->actorPosition.z ? -1 : 1);
                        if (winningPlayerIndex < 0) {
                            winningPlayerIndex = playerWork->playerIndex;
                        }
                        else if (frandmod(100) > 50) {
                            winningPlayerIndex = playerWork->playerIndex;
                        }
                    }
                }
                else {
                    if (1150.0f < ABS(playerWork->actorPosition.z)) {
                        playerWork->objectState = 2004;
                        playerWork->boundaryTimer = 10.0f;
                        playerWork->actorPosition.z = 1150.0f * (0.0f > playerWork->actorPosition.z ? -1 : 1);
                        playerWork->targetPositionZ = playerWork->actorPosition.z;
                        motionIndex = playerWork->playerSide + 5;
                        playerWork->motionIndex = motionIndex;
                        Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
                        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[playerWork->motionIndex], 0.0f, 8.0f, HU3D_MOTATTR_NONE);
                        playerWork->playerSide++;
                        if (playerWork->playerSide > highestPlayerSide) {
                            highestPlayerSide = playerWork->playerSide;
                        }
                        UpdatePlayerScoreSprites(playerWork);
                        omVibrate(playerWork->playerIndex, 12, 6, 6);
                    }
                }
            }
            break;
        case 2004:
            playerWork->boundaryTimer -= 1.0f;
            if (0.0f > playerWork->boundaryTimer && Hu3DMotionEndCheck(object->mdlId[0])) {
                playerWork->objectState = 2003;
                playerWork->inputRepeatCounter = 0;
                playerWork->buttonInput = 0;
                playerWork->buttonRepeatTimer = 0;
                playerWork->successfulInputCount = 0;
                playerWork->inputFrameCounter = 0;
                playerWork->motionCycleTimer = 0;
                motionIndex = playerWork->playerSide + 2;
            }
            break;
        default:
            OSReport("*** player mode error(%d)!!\n", playerWork->objectState);
            break;
    }
    if (motionIndex != playerWork->motionIndex) {
        playerWork->motionIndex = motionIndex;
        Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[playerWork->motionIndex], 0.0f, 30.0f, HU3D_MOTATTR_LOOP);
    }
    if (playerWork->motionIndex >= 2 && playerWork->motionIndex <= 4) {
        playerWork->movementRate = 0.5f * playerWork->movementAccumulator;
        if (1.0f > playerWork->movementRate) {
            playerWork->movementRate = 1.0f;
        }
        if (2.0f < playerWork->movementRate) {
            playerWork->movementRate = 2.0f;
        }
    }
    else {
        playerWork->movementRate = 1.0f;
        playerWork->movementAccumulator = 0.0f;
    }
    Hu3DMotionSpeedSet(object->mdlId[0], playerWork->movementRate);
    UpdatePlayerItemEffects(playerWork, object);
    UpdatePlayerMeterSprite(playerWork);
    omSetTra(object, playerWork->actorPosition.x, playerWork->actorPosition.y, playerWork->actorPosition.z);
    omSetRot(object, 0.0f, playerWork->actorRotation, 0.0f);
}

void UpdatePlayerResults(OMOBJ *object)
{
    M405PlayerWork *playerWork;
    s32 i;

    playerWork = object->data;
    switch (playerWork->objectState) {
        case 2000:
            if (minigameSequenceObject->work[0] >= 1003) {
                playerWork->objectState++;
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
            }
            break;
        case 2001:
            if (minigameSequenceObject->work[0] >= 1005) {
                playerWork->objectState++;
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
                for (i = 0; i <= 3; i++) {
                    espDispOn(playerWork->spriteIds[i]);
                }
                UpdatePlayerScoreSprites(playerWork);
            }
            else if (30.0f == Hu3DMotionTimeGet(object->mdlId[0])) {
                Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
            }
            break;
        case 2002:
            if (Hu3DMotionEndCheck(object->mdlId[0])) {
                playerWork->objectState++;
                playerWork->actorPosition.z = 900.0f;
                playerWork->targetPositionZ = playerWork->actorPosition.z;
                playerWork->motionIndex = 2;
                Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
                Hu3DMotionSet(object->mdlId[0], object->mtnId[playerWork->motionIndex]);
                Hu3DMotionCalc(object->mdlId[0]);
                object->objFunc = UpdatePlayerMinigameState;
            }
            break;
        case 2005:
            if (frandmod(60) == 0) {
                RecordPlayerTrailPosition(playerWork, &playerWork->actorPosition);
            }
            if (winningPlayerIndex == playerWork->playerIndex) {
                playerWork->actorRotation = InterpolateAngle(playerWork->actorRotation, 0.0f, 0.1f);
            }
            if (minigameSequenceObject->work[0] >= 1008 && playerWork->motionIndex == 0 && winningPlayerIndex == playerWork->playerIndex
                && 1.0f == Hu3DMotionTimeGet(object->mdlId[0])) {
                playerWork->motionIndex = 7;
                Hu3DMotionOverlaySet(object->mdlId[0], object->mtnId[playerWork->motionIndex]);
                Hu3DMotionOverlayTimeSet(object->mdlId[0], 0.0f);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_OVL_PAUSE);
                CharFXPlay(playerWork->characterId, 0x124);
            }
            if (playerWork->motionIndex == 7 && 73.0f <= Hu3DMotionOverlayTimeGet(object->mdlId[0])) {
                Hu3DMotionOverlayTimeSet(object->mdlId[0], 35.0f);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_OVL_PAUSE);
            }
            playerWork->actorPosition.y = -62.0f + 4.0f * GetWaterDisplacement(&playerWork->actorPosition);
            break;
        default:
            OSReport("*** player mode error(%d)!!\n", playerWork->objectState);
            break;
    }
    if (playerWork->objectState < 2005) {
        UpdatePlayerItemEffects(playerWork, object);
    }
    omSetTra(object, playerWork->actorPosition.x, playerWork->actorPosition.y, playerWork->actorPosition.z);
    omSetRot(object, 0.0f, playerWork->actorRotation, 0.0f);
}

void CreatePlayerObject(OMOBJ *object)
{
    float computerAccuracy;
    M405PlayerWork *playerWork;
    s32 i;

    playerWork = object->data;
    playerWork->computerDifficultySettings = NULL;
    playerWork->playerIndex = object->work[0];
    playerWork->characterId = GWPlayerCfg[playerWork->playerIndex].character;
    playerWork->controllerPort = GWPlayerCfg[playerWork->playerIndex].padIdx;
    playerWork->remainingMeter = 100.0f;
    playerWork->meterTickTimer = 0.0f;
    playerWork->meterDecrement = 0.0f;
    playerWork->meterIncrement = 7.0f;
    playerWork->motionCycleTimer = 0;
    playerWork->buttonRepeatTimer = 0;
    playerWork->successfulInputCount = 0;
    playerWork->inputFrameCounter = 0;
    playerWork->buttonInput = 0;
    playerWork->inputRepeatCounter = 0;
    playerWork->alternatingInputState = 0;
    playerWork->playerSide = 0;
    playerWork->trailPositionIndex = 0;
    playerWork->boundaryCollisionState = 1;
    playerWork->motionIndex = 1;
    playerWork->objectState = 2000;
    playerWork->boundaryTimer = 0.0f;
    playerWork->movementRate = 1.0f;
    playerWork->movementAccumulator = 0.0f;
    object->mdlId[0] = CharModelCreate(playerCharacterModelIds[playerWork->characterId], 4);
    CharModelStepFxSet(playerCharacterModelIds[playerWork->characterId], 0);
    for (i = 0; i < 8; i++) {
        object->mtnId[i] = CharMotionCreate(playerCharacterModelIds[playerWork->characterId], playerCharacterMotionResources[playerWork->characterId][i]);
        CharMotionSet(playerCharacterModelIds[playerWork->characterId], object->mtnId[i]);
    }
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[0], 3);
    Hu3DMotionSet(object->mdlId[0], object->mtnId[1]);
    Hu3DMotionTimeSet(object->mdlId[0], 1.0f);
    CharMotionVoiceOnSet(playerWork->characterId, object->mtnId[0], 0);
    playerWork->actorPosition.x = 450.0f - 300.0f * object->work[0];
    playerWork->actorPosition.y = -32.0f;
    playerWork->actorPosition.z = 1328.7f;
    playerWork->actorRotation = 180.0f;
    omSetTra(object, playerWork->actorPosition.x, playerWork->actorPosition.y, playerWork->actorPosition.z);
    omSetRot(object, 0.0f, playerWork->actorRotation, 0.0f);
    for (i = 0; i < 1; i++) {
        Hu3DModelAttrReset(object->mdlId[i], HU3D_ATTR_DISPOFF);
    }
    playerWork->scoreLabelX = 288.0f + playerScoreLabelOffsets[playerWork->playerIndex];
    playerWork->scoreLabelY = 80;
    playerWork->spriteIds[0] = espEntry(DATA_MAKE_NUM(DATADIR_M405, 64), 0, 0);
    espTPLvlSet(playerWork->spriteIds[0], 0.5f);
    espColorSet(playerWork->spriteIds[0], 0, 0, 0);
    playerWork->spriteIds[1] = espEntry(DATA_MAKE_NUM(DATADIR_M405, 63), 0, 0);
    playerWork->spriteIds[2] = espEntry(playerPortraitResources[playerWork->characterId], 0, 0);
    playerWork->spriteIds[3] = espEntry(DATA_MAKE_NUM(DATADIR_M405, 65), 0, 0);
    playerWork->spriteIds[4] = espEntry(DATA_MAKE_NUM(DATADIR_M405, 66), 0, 0);
    playerWork->spriteIds[5] = espEntry(DATA_MAKE_NUM(DATADIR_M405, 67), 0, 0);
    playerWork->spriteIds[6] = espEntry(DATA_MAKE_NUM(DATADIR_M405, 68), 0, 0);
    playerWork->spriteIds[7] = espEntry(DATA_MAKE_NUM(DATADIR_M405, 69), 0, 0);
    playerWork->spriteIds[8] = espEntry(DATA_MAKE_NUM(DATADIR_M405, 70), 0, 0);
    playerWork->spriteIds[9] = espEntry(DATA_MAKE_NUM(DATADIR_M405, 71), 0, 0);
    for (i = 0; i < 10; i++) {
        espDrawNoSet(playerWork->spriteIds[i], 0);
        espDispOff(playerWork->spriteIds[i]);
        espAttrSet(playerWork->spriteIds[i], HUSPR_ATTR_NOANIM);
        espBankSet(playerWork->spriteIds[i], 0);
        espPriSet(playerWork->spriteIds[i], 128 - i);
        espPosSet(playerWork->spriteIds[i], playerWork->scoreLabelX + playerItemOffsets[i * 2], playerWork->scoreLabelY + playerItemOffsets[i * 2 + 1]);
    }
    for (i = 0; i < 5; i++) {
        playerWork->particleManagerIds[i] = Hu3DParManLink(itemParticleManagerId, &waterSplashParticleParameters);
        Hu3DParManAttrSet(playerWork->particleManagerIds[i], 1);
        Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(playerWork->particleManagerIds[i]), 1);
        Hu3DModelLayerSet(Hu3DParManModelIDGet(playerWork->particleManagerIds[i]), 7);
        playerWork->itemEffectTimers[i] = 0;
        GetModelHookPosition(object->mdlId[0], CharModelItemHookGet(playerWork->characterId, 4, characterItemHookIndices[i]), &playerWork->itemPositions[i]);
    }
    playerWork->countdownValue = 0.0f;
    playerWork->targetEffectCount = 0;
    playerWork->stateTimer = 0.0f;
    playerWork->effectFrameTimer = 0.0f;
    playerWork->effectDelay = 0.0f;
    if (GWPlayerCfg[playerWork->playerIndex].iscom) {
        playerWork->difficultySetting = GWPlayerCfg[playerWork->playerIndex].diff;
        switch (GWPlayerCfg[playerWork->playerIndex].diff) {
            case 0:
                playerWork->computerDifficultySettings = &computerDifficultyEasySettings;
                computerAccuracy = 0.1f;
                break;
            case 1:
                playerWork->computerDifficultySettings = &computerDifficultyNormalSettings;
                computerAccuracy = 0.1f;
                break;
            case 2:
                playerWork->computerDifficultySettings = &computerDifficultyHardSettings;
                computerAccuracy = 0.1f;
                break;
            case 3:
            default:
                playerWork->difficultySetting = 3;
                playerWork->computerDifficultySettings = &computerDifficultyExpertSettings;
                computerAccuracy = 0.05f;
                break;
        }
        playerWork->countdownValue = playerWork->computerDifficultySettings->effectDuration + playerWork->computerDifficultySettings->effectDuration * computerAccuracy * frandf();
        playerWork->targetEffectCount = playerWork->computerDifficultySettings->effectType;
        playerWork->stateTimer = 6700.0f;
        playerWork->effectFrameTimer = 0.0f;
        playerWork->effectDelay = 0.0f;
    }
    CharMotionDataClose(playerCharacterModelIds[playerWork->characterId]);
    object->objFunc = UpdatePlayerResults;
}

void InitializeMinigameScene(void)
{
    Vec cameraPosition;
    Vec cameraTarget;
    Vec cameraUp;
    float cameraTransition;
    float cameraRotation;
    float cameraZoom;
    float cameraOffset;
    float cameraAngle;
    float cameraDistance;
    M405PlayerWork *cameraStep;
    s32 i;

    cameraOffset = 576.0f;
    cameraAngle = 0.0f;
    cameraTransition = 480.0f;
    cameraRotation = 0.0f;
    for (i = 0; i < 4; i++) {
        cameraStep = playerObjects[i]->data;
        Hu3D3Dto2D(&cameraStep->itemPositions[4], 1, &cameraPosition);
        if (cameraOffset > cameraPosition.x) {
            cameraOffset = cameraPosition.x;
        }
        if (cameraAngle < cameraPosition.x) {
            cameraAngle = cameraPosition.x;
        }
        if (cameraTransition > cameraPosition.y) {
            cameraTransition = cameraPosition.y;
        }
        if (cameraRotation < cameraPosition.y) {
            cameraRotation = cameraPosition.y;
        }
    }
    cameraTarget = cameraTransitionStartPosition;
    cameraUp = cameraTransitionStartUp;
    switch (highestPlayerSide) {
        case 0:
            if (1.0f < ABS(cameraTarget.z - cameraUp.z) && cameraTarget.z < cameraUp.z) {
                cameraDistance = 0.05f * (288.0f - (cameraOffset + cameraAngle) / 2);
                if (0.0f < cameraDistance) {
                    sceneEffectTimer -= cameraDistance;
                }
                cameraUp.x += 0.02f * -cameraUp.x;
                cameraUp.y += 0.02f * -cameraUp.y;
                cameraUp.z += 0.02f * (sceneEffectTimer - cameraUp.z);
                cameraTarget.x += 0.02f * (-1500.0f - cameraTarget.x);
                cameraTarget.y += 0.02f * (700.0f - cameraTarget.y);
                cameraTarget.z += 0.04f * (cameraUp.z - cameraTarget.z);
            }
            else {
                cameraDistance = 0.05f * (288.0f - (cameraOffset + cameraAngle) / 2);
                if (0.0f < cameraDistance) {
                    cameraTarget.z -= cameraDistance;
                    cameraUp.z -= cameraDistance;
                    cameraTarget.z = cameraUp.z;
                    cameraZoom = ABS(cameraAngle - cameraOffset) / 288.0f;
                    if (1.0f < cameraZoom) {
                        cameraZoom = 1.0f;
                    }
                    cameraDistance = 0.1f * (100.0f - cameraOffset) * cameraZoom;
                    if (cameraDistance < cameraAngle - 520.0f) {
                        cameraDistance = 0.1f * (cameraAngle - 520.0f) * cameraZoom;
                    }
                    if (0.0f < cameraDistance) {
                        cameraTarget.y += 2.0f * cameraDistance;
                        cameraTarget.x -= cameraDistance;
                        cameraUp.x += 0.01f * cameraDistance;
                    }
                }
            }
            break;
        case 1:
            cameraDistance = 0.05f * (340.0f - cameraRotation);
            cameraTarget.z += cameraDistance;
            cameraUp.z += cameraDistance;
            break;
        case 2:
            cameraDistance = 0.05f * (288.0f - (cameraOffset + cameraAngle) / 2);
            cameraTarget.z -= cameraDistance;
            cameraUp.z -= cameraDistance;
            cameraZoom = ABS(cameraAngle - cameraOffset) / 261.81818f;
            if (1.0f < cameraZoom) {
                cameraZoom = 1.0f;
            }
            cameraDistance = 0.1f * (100.0f - cameraOffset) * cameraZoom;
            if (cameraDistance < cameraAngle - 480.0f) {
                cameraDistance = 0.1f * (cameraAngle - 480.0f) * cameraZoom;
            }
            if (0.0f > cameraDistance) {
                if (360.0f > cameraRotation && 1500.0f <= cameraTarget.y) {
                    cameraTarget.y += cameraDistance;
                    cameraTarget.x -= 0.2f * cameraDistance;
                    cameraUp.x += 0.01f * cameraDistance;
                }
            }
            else {
                cameraTarget.y += 2.0f * cameraDistance;
                cameraTarget.x -= 0.5f * cameraDistance;
                cameraUp.x += 0.01f * cameraDistance;
            }
            break;
    }
    SetCameraView(&cameraTarget, &worldUpVector, &cameraUp);
}

static inline void SetVectorComponents(Vec *vector, float x, float y, float z)
{
    vector->x = x;
    vector->y = y;
    vector->z = z;
}

void UpdateMinigameSequence(OMOBJ *object)
{
    Vec particlePosition;
    Vec particleVelocity;
    float particleSpeed;
    M405PlayerWork *particleManagerId;
    s32 particleIndex;

    if (omSysExitReq != 0) {
        FadeMinigameMusic(-1);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = UpdateMinigameState;
    }
    minigameFrameCounter++;
    switch (object->work[0]) {
        case 1000:
            if (WipeStatGet() == 0) {
                object->work[0]++;
                minigameElapsedTime = 0.0f;
                cameraPositionPair[1] = initialCameraPosition;
                cameraUpPair[1] = initialCameraTarget;
                SetVectorComponents(&cameraTargetPair[1], -450.0f, cameraPositionPair[1].y, cameraPositionPair[1].z);
                SetVectorComponents(&cameraAuxiliaryVectors[1], -450.0f, cameraUpPair[1].y, cameraUpPair[1].z);
                cameraPositionPair[0] = cameraTargetPair[1];
                cameraUpPair[0] = cameraAuxiliaryVectors[1];
                SetVectorComponents(&cameraTargetPair[0], -1100.0f, 200.0f + cameraPositionPair[0].y, -200.0f);
                cameraAuxiliaryVectors[0].x = 0.0f;
                cameraAuxiliaryVectors[0].y = 0.0f;
                cameraAuxiliaryVectors[0].z = 1300.0f;
            }
            break;
        case 1001:
            minigameElapsedTime += 1.0f;
            if (90.0f >= minigameElapsedTime) {
                particleIndex = 1;
                particleSpeed = minigameElapsedTime;
            }
            else {
                particleIndex = 0;
                particleSpeed = minigameElapsedTime - 90.0f;
            }
            InterpolateCameraVector(particleSpeed / 90.0f, &cameraPositionPair[particleIndex], &cameraTargetPair[particleIndex], &particlePosition);
            InterpolateCameraVector(particleSpeed / 90.0f, &cameraUpPair[particleIndex], &cameraAuxiliaryVectors[particleIndex], &particleVelocity);
            SetCameraView(&particlePosition, &worldUpVector, &particleVelocity);
            if (180.0f <= minigameElapsedTime) {
                object->work[0]++;
                minigameElapsedTime = 0.0f;
            }
            break;
        case 1002:
            minigameElapsedTime += 1.0f;
            if (10.0f < minigameElapsedTime) {
                object->work[0]++;
                minigameElapsedTime = 0.0f;
            }
            break;
        case 1003:
            minigameElapsedTime += 1.0f;
            if (80.0f < minigameElapsedTime) {
                object->work[0]++;
                minigameElapsedTime = 0.0f;
            }
            break;
        case 1004:
            minigameElapsedTime += 1.0f;
            if (minigameMode < 0) {
                minigameMode = MGSeqStartCreate();
                break;
            }
            if (musicSequenceHandle < 0 && (MGSeqStatGet(minigameMode) & 0x10)) {
                musicSequenceHandle = HuAudSeqPlay(0x47);
            }
            if (MGSeqStatGet(minigameMode) == 0) {
                object->work[0]++;
                countdownFrames = 0;
                minigameMode = -1;
                minigameElapsedTime = 0.0f;
                minigameStateTimer = HuAudFXPlay(0x538);
            }
            break;
        case 1005:
            countdownFrames++;
            if (minigameSubstate >= 0) {
                MGSeqParamSet(minigameSubstate, 1, (((REFRESH_RATE*301)-1) - countdownFrames) / REFRESH_RATE);
            }
            else if (countdownFrames > (REFRESH_RATE*270)) {
                minigameSubstate = MGSeqTimerCreate((((REFRESH_RATE*301)-1) - countdownFrames) / REFRESH_RATE);
                MGSeqPosSet(minigameSubstate, 288.0f, 400.0f);
            }
            SetTimerDigitSprites(timerSpriteIds, 0, countdownFrames);
            if (countdownFrames >= (REFRESH_RATE*300) || winningPlayerIndex >= 0) {
                object->work[0]++;
                minigameElapsedTime = 0.0f;
                if (winningPlayerIndex >= 0) {
                    particleManagerId = playerObjects[winningPlayerIndex]->data;
                    cameraPositionPair[0] = cameraTransitionStartPosition;
                    cameraTargetPair[0] = particleManagerId->actorPosition;
                    cameraTargetPair[0].y += 100.0f;
                    cameraTargetPair[0].z += 500.0f;
                    cameraUpPair[0] = cameraTransitionStartUp;
                    cameraAuxiliaryVectors[0] = particleManagerId->actorPosition;
                    cameraAuxiliaryVectors[0].y += 75.0f;
                    cameraAuxiliaryVectors[0].z += 50.0f;
                }
            }
            if (minigameScore != highestPlayerSide) {
                if (highestPlayerSide == 1) {
                    SetCameraView(&distantCameraTarget, &worldUpVector, &itemCameraTarget);
                }
                if (highestPlayerSide == 2) {
                    SetCameraView(&farCameraPosition, &worldUpVector, &zeroPosition);
                }
            }
            else {
                InitializeMinigameScene();
            }
            minigameScore = highestPlayerSide;
            break;
        case 1006:
            if (minigameMode < 0) {
                if (minigameSubstate >= 0) {
                    MGSeqParamSet(minigameSubstate, 2, -1);
                }
                minigameSubstate = -1;
                minigameMode = MGSeqFinishCreate();
                FadeMinigameMusic(100);
                if (winningPlayerIndex >= 0) {
                    minigameStateCounter = HuAudFXPlay(0x538);
                }
            }
            else {
                minigameElapsedTime += 1.0f;
                if (winningPlayerIndex >= 0) {
                    InterpolateCameraVector(minigameElapsedTime / 60.0f, &cameraPositionPair[0], &cameraTargetPair[0], &particlePosition);
                    InterpolateCameraVector(minigameElapsedTime / 60.0f, &cameraUpPair[0], &cameraAuxiliaryVectors[0], &particleVelocity);
                    SetCameraView(&particlePosition, &worldUpVector, &particleVelocity);
                }
                if (60.0f < minigameElapsedTime && MGSeqStatGet(minigameMode) == 0) {
                    object->work[0] = 1008;
                    if (winningPlayerIndex >= 0 && !GWPlayerCfg[winningPlayerIndex].iscom && countdownFrames < (REFRESH_RATE*60)) {
                        GWGameStat.present[53] = 1;
                    }
                    if (winningPlayerIndex >= 0 && !GWPlayerCfg[winningPlayerIndex].iscom && recordedCountdownFrames > countdownFrames) {
                        object->work[0] = 1007;
                        SavePlayerRecord(0);
                    }
                    minigameMode = -1;
                    minigameElapsedTime = 0.0f;
                }
            }
            break;
        case 1007:
            if (minigameMode < 0) {
                minigameMode = MGSeqRecordCreate(recordedCountdownFrames);
                break;
            }
            if (MGSeqStatGet(minigameMode) == 0) {
                object->work[0]++;
                minigameElapsedTime = 0.0f;
                minigameMode = -1;
            }
            break;
        case 1008:
            if (minigameMode < 0) {
                if (winningPlayerIndex < 0) {
                    minigameMode = MGSeqDrawCreate();
                    HuAudSStreamPlay(4);
                }
                else {
                    minigameMode = MGSeqWinCreate(GWPlayerCfg[winningPlayerIndex].character, -1, -1, -1);
                    HuAudSStreamPlay(1);
                }
            }
            else {
                minigameElapsedTime += 1.0f;
                0 <= winningPlayerIndex;
                if (210.0f < minigameElapsedTime && MGSeqStatGet(minigameMode) == 0) {
                    FadeMinigameMusic(-1);
                    minigameMode = -1;
                    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                    object->objFunc = UpdateMinigameState;
                }
            }
            break;
        default:
            OSReport("*** main mode error(%d)!!\n", object->work[0]);
            break;
    }
    AnimateTimerSprites();
}

void CreateMinigameScene(OMOBJ *object)
{
    s32 i;

    object->work[0] = 1000;
    highestPlayerSide = 0;
    minigameScore = 0;
    sceneEffectTimer = 900.0f;
    omSetTra(object, 0.0f, 0.0f, 0.0f);
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M405, 48));
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[0], 5);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M405, 49));
    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[1], 6);
    Hu3DModelPosSet(object->mdlId[1], 0.0f, 0.0f, 0.0f);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M405, 50));
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[2], 1);
    Hu3DModelPosSet(object->mdlId[2], 0.0f, 0.0f, 0.0f);
    Hu3DModelShadowMapSet(object->mdlId[2]);
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M405, 51));
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[3], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[3], 1);
    Hu3DModelPosSet(object->mdlId[3], 0.0f, -4.0f, 0.0f);
    Hu3DModelTPLvlSet(object->mdlId[3], 0.4f);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M405, 52));
    Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[4], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[4], 6);
    Hu3DModelPosSet(object->mdlId[4], 0.0f, 0.0f, 0.0f);
    CreateTimerSprites(80.0f, DATA_MAKE_NUM(DATADIR_M405, 72));
    ShowTimerSprites();
    for (i = 0; i < 5; i++) {
        Hu3DModelAttrReset(object->mdlId[i], HU3D_ATTR_DISPOFF);
    }
    minigameStateValue = HuAudFXPlay(0x537);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    object->objFunc = UpdateMinigameSequence;
}

void ObjectSetup(void)
{
    Vec cameraUp;
    s32 shadowObject;
    s32 i;
    OMOBJ *camera;

    OSReport("******* M405 ObjectSetup *********\n");
    objectManager = omInitObjMan(100, 0x2000);
    omGameSysInit(objectManager);
    minigameFrameCounter = 0;
    minigameMode = -1;
    minigameSubstate = -1;
    winningPlayerIndex = -1;
    minigameStateValue = -1;
    minigameStateTimer = -1;
    minigameStateCounter = -1;
    musicSequenceHandle = -1;
    timerPulseAngle = -1.0f;
    recordedCountdownFrames = GWMGRecordGet(0);
    if (recordedCountdownFrames == 0) {
        recordedCountdownFrames = REFRESH_RATE*300;
    }
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 30.0f, 20.0f, 20000.0f, 1.2f);
    SetCameraView(&initialCameraPosition, &worldUpVector, &initialCameraTarget);
    shadowObject = Hu3DGLightCreateV(&overheadCameraPosition, &cameraUpDirection, &playerSpriteColor);
    Hu3DGLightStaticSet(shadowObject, 1);
    Hu3DGLightInfinitytSet(shadowObject);
    Hu3DShadowCreate(30.0f, 20.0f, 20000.0f);
    Hu3DShadowTPLvlSet(0.8f);
    VECNormalize(&highCameraPosition, &cameraUp);
    Hu3DShadowPosSet(&highCameraPosition, &cameraUp, &originPosition);
    playerResultAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M405, 57));
    HuSprAnimLock(playerResultAnimation);
    itemParticleManagerId = Hu3DParManCreate(playerResultAnimation, 500, &waterSplashParticleParameters);
    Hu3DParManAttrSet(itemParticleManagerId, 1);
    playerItemAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M405, 62));
    HuSprAnimLock(playerItemAnimation);
    itemParticleId = Hu3DParManCreate(playerItemAnimation, 2, &waterWaveParticleParameters);
    Hu3DParManAttrSet(itemParticleId, 1);
    Hu3DModelLayerSet(Hu3DParManModelIDGet(itemParticleId), 7);
    omMakeGroupEx(objectManager, 0, 4);
    playerObjects = omGetGroupMemberListEx(objectManager, 0);
    for (i = 0; i < 4; i++) {
        camera = omAddObjEx(objectManager, 100, 1, 8, 0, CreatePlayerObject);
        camera->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M405PlayerWork), HU_MEMNUM_OVL);
        camera->work[0] = i;
    }
    minigameSceneObject = omAddObjEx(objectManager, 101, 2, 2, -1, CreateItemObject);
    minigameEffectObject = omAddObjEx(objectManager, 103, 5, 0, -1, CreateWaterEffectObject);
    minigameSequenceObject = omAddObjEx(objectManager, 104, 5, 0, -1, CreateMinigameScene);
}

void UpdateMinigameState(OMOBJ *object)
{
    s32 i;

    if (minigameStateValue >= 0) {
        minigameStateValue = -1;
        HuAudFXStop(minigameStateValue);
    }
    if (minigameStateTimer >= 0) {
        minigameStateTimer = -1;
        HuAudFXStop(minigameStateTimer);
    }
    if (minigameStateCounter >= 0) {
        minigameStateCounter = -1;
        HuAudFXStop(minigameStateCounter);
    }
    if (WipeStatGet() == 0) {
        if (minigameMode >= 0) {
            MGSeqKill(minigameMode);
        }
        if (minigameSubstate >= 0) {
            MGSeqKill(minigameSubstate);
        }
        MGSeqKillAll();
        HuAudAllStop();
        for (i = 0; i < 4; i++) {
            CharModelKill(playerCharacterModelIds[GWPlayerCfg[i].character]);
        }
        omOvlReturnEx(1, 1);
    }
}
