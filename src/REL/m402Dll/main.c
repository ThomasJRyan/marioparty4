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

#include "dolphin.h"
#include "ext_math.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/audio.h"
void HuSysVWaitSet(s16 vcount);
#endif

#undef ABS
#define ABS(x) ((0 > (x)) ? -(x) : (x))

typedef struct ResourceValueEntry {
    /* 0x00 */ u16 resourceId;
    /* 0x02 */ s16 resourceValue;
    /* 0x04 */ struct ResourceValueEntry *previousEntry;
    /* 0x08 */ struct ResourceValueEntry *nextEntry;
} ResourceValueEntry; // Size 0xC

typedef struct LinkedParticleBatch {
    /* 0x00 */ HU3DPARMANPARAM *particleParameters;
    /* 0x04 */ s32 *particleIds;
    /* 0x08 */ s32 framesRemaining;
    /* 0x0C */ s32 particleCount;
    /* 0x10 */ struct LinkedParticleBatch *previousEntry;
    /* 0x14 */ struct LinkedParticleBatch *nextEntry;
} LinkedParticleBatch; // Size 0x18

typedef struct {
    /* 0x00 */ u32 resourceFileId;
    /* 0x04 */ s16 cachedModelId;
    /* 0x06 */ char reservedBytes[2]; // padding?
} CachedModelEntry; // Size 8

typedef struct {
    /* 0x00 */ float startingRadius;
    /* 0x04 */ float startingHeight;
    /* 0x08 */ float heightVelocity;
    /* 0x0C */ float currentMotionTime;
    /* 0x10 */ float targetMotionTime;
    /* 0x14 */ float motionHeightOffset;
    /* 0x18 */ float playerAngle;
    /* 0x1C */ float currentRadius;
    /* 0x20 */ float radiusVelocity;
    /* 0x24 */ float targetRadius;
    /* 0x28 */ float playerHeight;
    /* 0x2C */ float animationFrame;
    /* 0x30 */ float stateTimer;
    /* 0x34 */ s32 computerActionChance;
    /* 0x38 */ s32 computerActionTimer;
    /* 0x3C */ s32 computerActionInterval;
    /* 0x40 */ s32 computerActionCounter;
    /* 0x44 */ s32 lastEffectId;
    /* 0x48 */ s32 effectPlayed;
    /* 0x4C */ s32 playerState;
    /* 0x50 */ s32 stateFrame;
    /* 0x54 */ s32 playerIndex;
    /* 0x58 */ s32 characterId;
    /* 0x5C */ s32 controllerPort;
    /* 0x60 */ s32 actionProgress;
} PlayerActionWork; // Size 0x64

typedef struct {
    /* 0x00 */ float durationFrames;
    /* 0x04 */ float motionFrame;
    /* 0x08 */ float heightRatio;
    /* 0x0C */ float motionRatio;
    /* 0x10 */ s32 soundEffectId;
} PlayerActionFrameData; // Size 0x14

typedef struct {
    /* 0x00 */ float positionX;
    /* 0x04 */ float positionY;
    /* 0x08 */ float positionZ;
    /* 0x0C */ float rotationY;
    /* 0x10 */ float verticalVelocity;
    /* 0x14 */ float pauseTimer;
    /* 0x18 */ s32 motionState;
} PlayerEffectWork; // Size 0x1C

typedef struct {
    /* 0x00 */ Vec position;
    /* 0x0C */ float rotationX;
    /* 0x10 */ float rotationY;
    /* 0x14 */ float rotationZ;
    /* 0x18 */ float scaleX;
    /* 0x1C */ float scaleY;
    /* 0x20 */ float scaleZ;
    /* 0x24 */ Vec velocity;
    /* 0x30 */ float transparency;
    /* 0x34 */ float transparencyDecay;
    /* 0x38 */ s32 reuseDelay;
} EffectObjectWork; // Size 0x3C

static void InitializeResourceList(void);
static void RegisterResourceValue(u16 resourceId, s16 resourceValue);
static s16 TakeResourceValue(s16 resourceId);
static void InitializeParticleList(void);
static void CreateLinkedParticleBatch(s16 particleManagerId, HU3DPARMANPARAM *particleParameters, s32 renderLayer, s32 durationFrames, s32 particleCount, Vec *particlePositions, u8 blendMode);
static void UpdateLinkedParticleBatches(void);
static void ResetModelCache(void);
static s16 CreateCachedModel(s32 resourceFileId);
static void SetTimerDigits(s32 *spriteIds, s32 spriteOffset, s32 elapsedFrames);
static void RecordFinalTime(s32 minigameRecordId);
static void ShowTimerSprites(void);
static void AnimateTimerPulse(void);
static void CreateTimerSprites(float verticalPosition, s32 recordTimeFrames);
static void InterpolateCameraVectors(float progress, Vec *startVector, Vec *endVector, Vec *resultVector);
static void SetCameraPositionAndTarget(Vec *cameraPosition, Vec *cameraUp, Vec *cameraTarget);
static s32 AnimateCameraTransition(float progress, Vec *cameraUp);
static void SetCameraTransitionTarget(float progress, Vec *cameraUp);
static void ProjectWorldPositionToScreen(Vec *worldPosition, s16 cameraMask, Vec *screenPosition);
static void GetModelJointPosition(s16 modelId, char *jointName, Vec *jointPosition);
static void GetModelJointTransform(s16 modelId, char *jointName, Vec *jointPosition, Mtx jointTransform);
static float InterpolateAngle(float startAngle, float endAngle, float progress);
static void PlayBackgroundSequence(s32 sequenceId);
static void FadeBackgroundSequence(s32 fadeFrames);
static void MoveCameraFromController(Vec *cameraPosition, float *cameraRotation, s32 controllerX, s32 controllerY);
static void RegisterResourceValue54(void);
static void CreateModelLight(s16 modelId, GXColor *lightColor);
static u16 ChooseComputerPlayerAction(PlayerActionWork *playerWork);
static void ResetPlayerActionState(PlayerActionWork *playerWork, OMOBJ *playerObject);
static void UpdatePlayerStretchEffect(PlayerActionWork *playerWork, OMOBJ *playerObject, u8 actionComplete);
static void BeginPlayerActionAnimation(PlayerActionWork *playerWork, OMOBJ *playerObject, OMOBJ *actionComplete);
static u8 UpdatePlayerActionAnimation(PlayerActionWork *playerWork, OMOBJ *playerObject);
static void UpdatePlayerIdlePose(PlayerActionWork *playerWork, OMOBJ *playerObject);
static void UpdatePlayerAction(OMOBJ *playerObject);
static void BeginPlayerJump(float motionFrame);
static void BeginPlayerLand(float motionFrame);
static void UpdatePlayerModel(OMOBJ *playerObject);
static void CreatePlayerModel(OMOBJ *playerObject);
static void HideShadowModel(OMOBJ *shadowObject);
static void CreateShadowModel(OMOBJ *shadowObject);
static void UpdatePlayerEffect(OMOBJ *effectObject);
static void CreatePlayerEffect(OMOBJ *effectObject);
static void DeleteEffectObject(OMOBJ *effectObject);
static void UpdateEffectObject(OMOBJ *effectObject);
static void CreateEffectModel(OMOBJ *effectObject);
static void SpawnPlayerEffect(s32 effectId, Vec *effectPosition, Vec *effectVelocity);
static void HideStageMarker(OMOBJ *markerObject);
static void CreateStageMarker(OMOBJ *markerObject);
static void HideStageProp(OMOBJ *propObject);
static void CreateStageProp(OMOBJ *propObject);
static void UpdateRotatingLight(void);
static void UpdateMinigameSequence(OMOBJ *sequenceObject);
static void CreateMinigameSequence(OMOBJ *sequenceObject);
static void CleanupMinigame(OMOBJ *sequenceObject);

static ResourceValueEntry resourceValueList;
static LinkedParticleBatch activeParticleBatches;
static HUPROCESS *minigameObjectManager;
static Vec cameraStartPosition;
static Vec cameraCurrentPosition;
static Vec cameraStartTarget;
static Vec cameraCurrentTarget;
static u32 minigameFrameCounter;
static s32 timerFrameCount;
static s32 recordedTimeFrames;
static s16 activeSequenceId;
static s16 countdownSequenceId;
static s32 backgroundSequenceId;
static s32 cameraTransitionComplete;
static float timerPulseAngle;
static s32 timerSpriteIds[16];
static CachedModelEntry modelCacheEntries[32];
static OMOBJ *minigameSequenceObject;
static OMOBJ **stageMarkerObjects;
static OMOBJ **stagePropObjects;
static OMOBJ **playerObjects;
static OMOBJ **shadowObjects;
static OMOBJ **playerEffectObjects;
static OMOBJ **transientEffectObjects;
static u8 unusedGlobalData[4];
static float lightRotationAngle;
static float lightRotationSpeed;
static float sequenceModelTimer;
static float cameraTransitionFrame;
static float playerMotionRadius;
static s16 winningPlayerIndex;
static s16 activeEffectObjectCount;
static s16 cachedEffectModelIds[7];
static s16 globalLightId;
static s32 debugModeEnabled;
static float cameraControlVertical;
static float cameraControlHorizontal;
static float cameraControlDepth;
static u8 unusedGlobalBytes[4];

static s16 characterModelIndices[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

static Vec defaultCameraPosition = { 0.0f, 2500.0f, 2200.0f };
static Vec worldUpVector = { 0.0f, 1.0f, 0.0f };
static Vec defaultCameraTarget = { 0.0f, 388.0f, 0.0f };
static Vec keyLightPosition = { 0.0f, 2000.0f, 1000.0f };
static Vec keyLightDirection = { 0.0f, -2.0f, -1.0f };
static GXColor whiteLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };

static Vec unusedStageDirection = { 0.0f, -1.0f, 0.0f };
static u8 unusedColorBytes[] = { 0xFF, 0xFF, 0xFF, 0xFF };

static Vec shadowLightPosition = { 0.0f, 4000.0f, 1000.0f };
static Vec shadowTargetPosition = { 0.0f, 0.0f, 0.0f };

static s32 characterMotionResourceIds[8][11] = { { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), DATA_MAKE_NUM(DATADIR_MARIOMOT, 2), DATA_MAKE_NUM(DATADIR_MARIOMOT, 28),
                                        DATA_MAKE_NUM(DATADIR_M402, 16), DATA_MAKE_NUM(DATADIR_MARIOMOT, 62), DATA_MAKE_NUM(DATADIR_MARIOMOT, 3),
                                        DATA_MAKE_NUM(DATADIR_M402, 8), DATA_MAKE_NUM(DATADIR_M402, 0), DATA_MAKE_NUM(DATADIR_M402, 24),
                                        DATA_MAKE_NUM(DATADIR_MARIOMOT, 117), DATA_MAKE_NUM(DATADIR_MARIOMOT, 76) },
    { DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 2), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 28), DATA_MAKE_NUM(DATADIR_M402, 17),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 62), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 3), DATA_MAKE_NUM(DATADIR_M402, 9), DATA_MAKE_NUM(DATADIR_M402, 1),
        DATA_MAKE_NUM(DATADIR_M402, 25), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 117), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 76) },
    { DATA_MAKE_NUM(DATADIR_PEACHMOT, 0), DATA_MAKE_NUM(DATADIR_PEACHMOT, 2), DATA_MAKE_NUM(DATADIR_PEACHMOT, 28), DATA_MAKE_NUM(DATADIR_M402, 18),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 62), DATA_MAKE_NUM(DATADIR_PEACHMOT, 3), DATA_MAKE_NUM(DATADIR_M402, 10), DATA_MAKE_NUM(DATADIR_M402, 2),
        DATA_MAKE_NUM(DATADIR_M402, 26), DATA_MAKE_NUM(DATADIR_PEACHMOT, 117), DATA_MAKE_NUM(DATADIR_PEACHMOT, 76) },
    { DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0), DATA_MAKE_NUM(DATADIR_YOSHIMOT, 2), DATA_MAKE_NUM(DATADIR_YOSHIMOT, 28), DATA_MAKE_NUM(DATADIR_M402, 19),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 62), DATA_MAKE_NUM(DATADIR_YOSHIMOT, 3), DATA_MAKE_NUM(DATADIR_M402, 11), DATA_MAKE_NUM(DATADIR_M402, 3),
        DATA_MAKE_NUM(DATADIR_M402, 27), DATA_MAKE_NUM(DATADIR_YOSHIMOT, 117), DATA_MAKE_NUM(DATADIR_YOSHIMOT, 76) },
    { DATA_MAKE_NUM(DATADIR_WARIOMOT, 0), DATA_MAKE_NUM(DATADIR_WARIOMOT, 2), DATA_MAKE_NUM(DATADIR_WARIOMOT, 28), DATA_MAKE_NUM(DATADIR_M402, 20),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 62), DATA_MAKE_NUM(DATADIR_WARIOMOT, 3), DATA_MAKE_NUM(DATADIR_M402, 12), DATA_MAKE_NUM(DATADIR_M402, 4),
        DATA_MAKE_NUM(DATADIR_M402, 28), DATA_MAKE_NUM(DATADIR_WARIOMOT, 117), DATA_MAKE_NUM(DATADIR_WARIOMOT, 76) },
    { DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 2), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 28), DATA_MAKE_NUM(DATADIR_M402, 21),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 62), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 3), DATA_MAKE_NUM(DATADIR_M402, 13), DATA_MAKE_NUM(DATADIR_M402, 5),
        DATA_MAKE_NUM(DATADIR_M402, 29), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 117), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 76) },
    { DATA_MAKE_NUM(DATADIR_DAISYMOT, 0), DATA_MAKE_NUM(DATADIR_DAISYMOT, 2), DATA_MAKE_NUM(DATADIR_DAISYMOT, 28), DATA_MAKE_NUM(DATADIR_M402, 22),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 62), DATA_MAKE_NUM(DATADIR_DAISYMOT, 3), DATA_MAKE_NUM(DATADIR_M402, 14), DATA_MAKE_NUM(DATADIR_M402, 6),
        DATA_MAKE_NUM(DATADIR_M402, 30), DATA_MAKE_NUM(DATADIR_DAISYMOT, 117), DATA_MAKE_NUM(DATADIR_DAISYMOT, 76) },
    { DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 2), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 28),
        DATA_MAKE_NUM(DATADIR_M402, 23), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 62), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 3), DATA_MAKE_NUM(DATADIR_M402, 15),
        DATA_MAKE_NUM(DATADIR_M402, 7), DATA_MAKE_NUM(DATADIR_M402, 31), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 117),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 76) } };

static s32 stageMarkerModelFiles[]
    = { DATA_MAKE_NUM(DATADIR_M402, 38), DATA_MAKE_NUM(DATADIR_M402, 40), DATA_MAKE_NUM(DATADIR_M402, 37), DATA_MAKE_NUM(DATADIR_M402, 39) };

static s32 stagePropMotionFiles[] = { DATA_MAKE_NUM(DATADIR_M402, 41), DATA_MAKE_NUM(DATADIR_M402, 42), DATA_MAKE_NUM(DATADIR_M402, 43) };

static s32 stagePropModelFile = DATA_MAKE_NUM(DATADIR_M402, 44);

static s32 stagePropSecondaryMotions[] = { DATA_MAKE_NUM(DATADIR_M402, 45), DATA_MAKE_NUM(DATADIR_M402, 46) };

static Vec playerEffectSpawnPositions[] = { { 450.0f, 290.0f, 0.0f }, { -450.0f, 290.0f, 0.0f } };

static s32 effectModelFiles[] = { DATA_MAKE_NUM(DATADIR_M402, 48), DATA_MAKE_NUM(DATADIR_M402, 50), DATA_MAKE_NUM(DATADIR_M402, 47),
    DATA_MAKE_NUM(DATADIR_M402, 49), DATA_MAKE_NUM(DATADIR_M402, 51), DATA_MAKE_NUM(DATADIR_M402, 52), DATA_MAKE_NUM(DATADIR_M402, 53) };

static PlayerActionFrameData playerActionFrameData[] = { { 0.0f, 0.0f, 0.0f, 0.0f, -1 }, { 2.0f, 2.0f, 0.1f, 0.125f, 0x514 }, { 2.0f, 2.0f, 0.2f, 0.25f, -1 },
    { 3.0f, 4.0f, 0.3f, 0.35f, 0x515 }, { 3.0f, 4.0f, 0.4f, 0.45f, -1 }, { 3.0f, 6.0f, 0.45f, 0.5125f, 0x516 }, { 3.0f, 6.0f, 0.5f, 0.575f, -1 },
    { 3.0f, 6.0f, 0.55f, 0.6375f, -1 }, { 3.0f, 6.0f, 0.6f, 0.7f, -1 }, { 4.0f, 8.0f, 0.65f, 0.75f, 0x517 }, { 4.0f, 8.0f, 0.7f, 0.8f, -1 },
    { 4.0f, 8.0f, 0.75f, 0.85f, -1 }, { 4.0f, 8.0f, 0.8f, 0.9f, -1 }, { 4.0f, 10.0f, 0.83f, 0.9166f, 0x518 }, { 4.0f, 10.0f, 0.86f, 0.9333f, -1 },
    { 4.0f, 10.0f, 0.9f, 0.95f, -1 }, { 4.0f, 10.0f, 0.93f, 0.9666f, -1 }, { 4.0f, 10.0f, 0.96f, 0.9833f, -1 }, { 4.0f, 10.0f, 1.0f, 1.0f, -1 },
    { 1.0f, 10.0f, 1.0f, 1.0f, -1 } };

static s32 playerActionSoundEffectIds[] = { 0x00000514, 0x00000515, 0x00000516, 0x00000517, 0x00000518 };

static void InitializeResourceList(void)
{
    resourceValueList.previousEntry = NULL;
    resourceValueList.nextEntry = NULL;
}

static void RegisterResourceValue(u16 resourceId, s16 resourceValue)
{
    ResourceValueEntry *newEntry;
    ResourceValueEntry *listHead;
    ResourceValueEntry *previousFirstEntry;

    newEntry = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*newEntry), HU_MEMNUM_OVL);
    newEntry->resourceId = resourceId;
    newEntry->resourceValue = resourceValue;
    listHead = &resourceValueList;
    previousFirstEntry = listHead->nextEntry;
    listHead->nextEntry = newEntry;
    if (NULL != previousFirstEntry) {
        previousFirstEntry->previousEntry = newEntry;
    }
    newEntry->previousEntry = listHead;
    newEntry->nextEntry = previousFirstEntry;
}

static s16 TakeResourceValue(s16 resourceId)
{
    ResourceValueEntry *entry;
    ResourceValueEntry *previousEntry;
    ResourceValueEntry *nextEntry;
    s16 resourceValue;

    entry = resourceValueList.nextEntry;
    while (NULL != entry) {
        if (resourceId == entry->resourceId) {
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

static void InitializeParticleList(void)
{
    activeParticleBatches.previousEntry = NULL;
    activeParticleBatches.nextEntry = NULL;
}

static void CreateLinkedParticleBatch(s16 particleManagerId, HU3DPARMANPARAM *particleParameters, s32 renderLayer, s32 durationFrames, s32 particleCount, Vec *particlePositions, u8 blendMode)
{
    LinkedParticleBatch *listHead;
    LinkedParticleBatch *particleEntry;
    LinkedParticleBatch *previousFirstEntry;
    s32 i;

    particleEntry = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*particleEntry), HU_MEMNUM_OVL);
    particleEntry->particleParameters = particleParameters;
    particleEntry->framesRemaining = durationFrames;
    particleEntry->particleCount = particleCount;
    particleEntry->particleIds = HuMemDirectMallocNum(HEAP_HEAP, particleCount * sizeof(*particleEntry->particleIds), HU_MEMNUM_OVL);
    for (i = 0; i < particleCount; i++, particlePositions++) {
        particleEntry->particleIds[i] = Hu3DParManLink(particleManagerId, particleParameters);
        if (particleEntry->particleIds[i] >= 0) {
            Hu3DParManAttrReset(particleEntry->particleIds[i], 1);
            Hu3DParManPosSet(particleEntry->particleIds[i], particlePositions->x, particlePositions->y, particlePositions->z);
            Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(particleEntry->particleIds[i]), blendMode);
            Hu3DModelLayerSet(Hu3DParManModelIDGet(particleEntry->particleIds[i]), renderLayer);
        }
    }
    previousFirstEntry = &activeParticleBatches;
    listHead = previousFirstEntry->nextEntry;
    previousFirstEntry->nextEntry = particleEntry;
    if (NULL != listHead) {
        listHead->previousEntry = particleEntry;
    }
    particleEntry->previousEntry = previousFirstEntry;
    particleEntry->nextEntry = listHead;
}

static void UpdateLinkedParticleBatches(void)
{
    LinkedParticleBatch *particleEntry;
    LinkedParticleBatch *previousEntry;
    LinkedParticleBatch *nextEntry;
    s32 i;

    particleEntry = activeParticleBatches.nextEntry;
    while (NULL != particleEntry) {
        particleEntry->framesRemaining--;
        if (particleEntry->framesRemaining == 0) {
            for (i = 0; i < particleEntry->particleCount; i++) {
                if (particleEntry->particleIds[i] >= 0) {
                    Hu3DParManAttrSet(particleEntry->particleIds[i], 1);
                }
            }
        }
        previousEntry = particleEntry->previousEntry;
        nextEntry = particleEntry->nextEntry;
        if (-particleEntry->particleParameters->maxTime > particleEntry->framesRemaining) {
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

static void ResetModelCache(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        modelCacheEntries[i].resourceFileId = 0;
        modelCacheEntries[i].cachedModelId = -1;
    }
}

static s16 CreateCachedModel(s32 resourceFileId)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (resourceFileId == modelCacheEntries[i].resourceFileId) {
            return Hu3DModelLink(modelCacheEntries[i].cachedModelId);
        }
    }
    for (i = 0; i < 32; i++) {
        if (modelCacheEntries[i].resourceFileId == 0) {
            modelCacheEntries[i].resourceFileId = resourceFileId;
            modelCacheEntries[i].cachedModelId = Hu3DModelCreateFile(resourceFileId);
            return modelCacheEntries[i].cachedModelId;
        }
    }
    return Hu3DModelCreateFile(resourceFileId);
}

static void SetTimerDigits(s32 *spriteIds, s32 spriteOffset, s32 elapsedFrames)
{
    s32 digitValue;

    espBankSet(spriteIds[spriteOffset + 1], 10);
    espBankSet(spriteIds[spriteOffset + 4], 11);
    if (600 * REFRESH_RATE <= elapsedFrames) {
        elapsedFrames = 600 * REFRESH_RATE - 1;
    }
    digitValue = elapsedFrames / (60 * REFRESH_RATE);
    elapsedFrames -= digitValue * (60 * REFRESH_RATE);
    if (digitValue > 9) {
        digitValue = 9;
    }
    espBankSet(spriteIds[spriteOffset], digitValue);
    digitValue = elapsedFrames / REFRESH_RATE;
    elapsedFrames -= digitValue * REFRESH_RATE;
    espBankSet(spriteIds[spriteOffset + 2], digitValue / 10);
    espBankSet(spriteIds[spriteOffset + 3], digitValue % 10);
    digitValue = 101.5f * elapsedFrames / REFRESH_RATE;
    espBankSet(spriteIds[spriteOffset + 5], digitValue / 10);
    espBankSet(spriteIds[spriteOffset + 6], digitValue % 10);
}

static void RecordFinalTime(s32 minigameRecordId)
{
    timerPulseAngle = 0.0f;
    recordedTimeFrames = timerFrameCount;
    GWMGRecordSet(minigameRecordId, recordedTimeFrames);
    SetTimerDigits(timerSpriteIds, 7, timerFrameCount);
}

static void ShowTimerSprites(void)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        espDispOn(timerSpriteIds[i]);
    }
}

static void AnimateTimerPulse(void)
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

static void CreateTimerSprites(float verticalPosition, s32 recordTimeAssetId)
{
    float verticalPosition;
    float horizontalPosition;
    s32 i;

    horizontalPosition = 288.0f;
    verticalPosition = verticalPosition;
    timerSpriteIds[15] = espEntry(recordTimeAssetId, 0, 0);
    espTPLvlSet(timerSpriteIds[15], 0.5f);
    espColorSet(timerSpriteIds[15], 0, 0, 0);
    espPosSet(timerSpriteIds[15], horizontalPosition, verticalPosition);
    horizontalPosition = 248.0f;
    verticalPosition = verticalPosition - 8.0f;
    timerSpriteIds[14] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 55), 0, 0);
    espPosSet(timerSpriteIds[14], horizontalPosition - 18.0f, verticalPosition);
    for (i = 7; i <= 13; i++) {
        timerSpriteIds[i] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 48), 0, 0);
        espPosSet(timerSpriteIds[i], horizontalPosition, verticalPosition);
        horizontalPosition += 16.0f;
    }
    SetTimerDigits(timerSpriteIds, 7, recordedTimeFrames);
    horizontalPosition = 248.0f;
    verticalPosition = verticalPosition + 8.0f;
    for (i = 0; i <= 6; i++) {
        timerSpriteIds[i] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 49), 0, 0);
        espPosSet(timerSpriteIds[i], horizontalPosition, verticalPosition);
        horizontalPosition += 16.0f;
    }
    SetTimerDigits(timerSpriteIds, 0, 0);
    for (i = 0; i < 16; i++) {
        espDrawNoSet(timerSpriteIds[i], 0);
        espDispOff(timerSpriteIds[i]);
        espAttrSet(timerSpriteIds[i], HUSPR_ATTR_NOANIM);
        espPriSet(timerSpriteIds[i], 0x80 + i);
    }
}

static void InterpolateCameraVectors(float progress, Vec *startVector, Vec *endVector, Vec *resultVector)
{
    float interpolationWeight;

    if (1.0f < progress) {
        progress = 1.0f;
    }
    interpolationWeight = sind(90.0f * progress) * sind(90.0f * progress);
    resultVector->x = startVector->x + interpolationWeight * (endVector->x - startVector->x);
    resultVector->y = startVector->y + interpolationWeight * (endVector->y - startVector->y);
    resultVector->z = startVector->z + interpolationWeight * (endVector->z - startVector->z);
}

static void SetCameraPositionAndTarget(Vec *cameraPosition, Vec *cameraUp, Vec *cameraTarget)
{
    Hu3DCameraPosSetV(1, cameraPosition, cameraUp, cameraTarget);
    cameraStartPosition = *cameraPosition;
    cameraStartTarget = *cameraTarget;
}

static s32 AnimateCameraTransition(float progress, Vec *cameraUp)
{
    Vec cameraPosition;
    Vec cameraTarget;

    InterpolateCameraVectors(progress, &cameraStartPosition, &cameraCurrentPosition, &cameraPosition);
    InterpolateCameraVectors(progress, &cameraStartTarget, &cameraCurrentTarget, &cameraTarget);
    Hu3DCameraPosSetV(1, &cameraPosition, cameraUp, &cameraTarget);
    if (1.0f <= progress) {
        return 1;
    }
    else {
        cameraTransitionComplete = 1;
        return 0;
    }
}

static void SetCameraTransitionTarget(float progress, Vec *cameraUp)
{
    Vec cameraPosition;
    Vec cameraTarget;

    InterpolateCameraVectors(progress, &cameraStartPosition, &cameraCurrentPosition, &cameraPosition);
    InterpolateCameraVectors(progress, &cameraStartTarget, &cameraCurrentTarget, &cameraTarget);
    SetCameraPositionAndTarget(&cameraPosition, cameraUp, &cameraTarget);
    cameraTransitionComplete = 1;
}

static void ProjectWorldPositionToScreen(Vec *worldPosition, s16 cameraMask, Vec *screenPosition)
{
    Mtx viewMatrix;
    Vec viewSpacePosition;
    float horizontalProjectionScale;
    float verticalProjectionScale;
    float viewportOriginX;
    float viewportWidth;
    HU3DCAMERA *camera;
    s32 cameraIndex;

    for (cameraIndex = 0; cameraIndex < 16; cameraIndex++) {
        if (cameraMask & (1 << cameraIndex)) {
            break;
        }
    }
    camera = &Hu3DCamera[cameraIndex];
    C_MTXLookAt(viewMatrix, &camera->pos, &camera->up, &camera->target);
    MTXMultVec(viewMatrix, worldPosition, &viewSpacePosition);
    horizontalProjectionScale = viewSpacePosition.z * (sind(camera->fov / 2) / cosd(camera->fov / 2)) * camera->aspect;
    verticalProjectionScale = viewSpacePosition.z * (sind(camera->fov / 2) / cosd(camera->fov / 2));
    viewportOriginX = 0.9f * camera->viewportX;
    viewportWidth = 0.9f * camera->viewportW;
    screenPosition->x = viewportWidth / 2 + viewSpacePosition.x * ((viewportWidth / 2) / -horizontalProjectionScale) + viewportOriginX;
    screenPosition->y = camera->viewportH / 2 + viewSpacePosition.y * ((camera->viewportH / 2) / verticalProjectionScale) + camera->viewportY;
    screenPosition->z = 0.0f;
}

static void GetModelJointPosition(s16 modelId, char *jointName, Vec *jointPosition)
{
    Mtx modelObjectMatrix;

    Hu3DModelObjMtxGet(modelId, jointName, modelObjectMatrix);
    jointPosition->x = modelObjectMatrix[0][3];
    jointPosition->y = modelObjectMatrix[1][3];
    jointPosition->z = modelObjectMatrix[2][3];
}

static void GetModelJointTransform(s16 modelId, char *jointName, Vec *jointPosition, Mtx jointTransform)
{
    Mtx modelObjectMatrix;
    HU3DMODEL *modelData;

    modelData = &Hu3DData[modelId];
    Hu3DModelObjMtxGet(modelId, jointName, modelObjectMatrix);
    MTXConcat(modelData->mtx, modelObjectMatrix, modelObjectMatrix);
    if (NULL != jointTransform) {
        MTXCopy(modelObjectMatrix, jointTransform);
    }
    jointPosition->x = modelObjectMatrix[0][3];
    jointPosition->y = modelObjectMatrix[1][3];
    jointPosition->z = modelObjectMatrix[2][3];
}

static float InterpolateAngle(float startAngle, float endAngle, float progress)
{
    float angleDifference;
    float interpolatedAngle;

    angleDifference = fmod(endAngle - startAngle, 360.0);
    if (0.0f > angleDifference) {
        angleDifference += 360.0f;
    }
    if (180.0f < angleDifference) {
        angleDifference -= 360.0f;
    }
    interpolatedAngle = fmod(startAngle + angleDifference * progress, 360.0);
    if (0.0f > interpolatedAngle) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

static void PlayBackgroundSequence(s32 sequenceId)
{
    backgroundSequenceId = HuAudSeqPlay(sequenceId);
}

static void FadeBackgroundSequence(s32 fadeFrames)
{
    if (backgroundSequenceId >= 0) {
        if (fadeFrames < 0) {
            fadeFrames = 1000;
        }
        HuAudSeqFadeOut(backgroundSequenceId, fadeFrames);
    }
    backgroundSequenceId = -1;
}

static void MoveCameraFromController(Vec *cameraPosition, float *cameraRotation, s32 controllerX, s32 controllerY)
{
    Vec movementVector;
    float movementMagnitude;
    s32 movementState;

    movementVector.x = 0.0f;
    movementVector.y = 0.0f;
    movementVector.z = 0.0f;
    movementVector.x = controllerX / 4.0f;
    movementVector.z = -(float)controllerY / 4.0f;
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
        *cameraRotation = InterpolateAngle(*cameraRotation, atan2d(movementVector.x, movementVector.z), 0.4f);
        if (8.0f <= movementMagnitude) {
            movementState = 2;
        }
        else {
            movementState = 1;
        }
    }
    VECAdd((Vec *)&cameraPosition, &movementVector, (Vec *)&cameraPosition);
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

static void RegisterResourceValue54(void)
{
    HuSysVWaitSet(1);
    if (HuPadBtnDown[0] & 0x400) {
        timerFrameCount = -1;
    }
    print8(24, 32, 1.5f, "%f COUNT:%d MES:%d WIN:%d", lightRotationAngle, minigameFrameCounter, activeSequenceId, winningPlayerIndex);
    print8(24, 48, 1.5f, "%f %f %f", cameraControlDepth, cameraControlVertical, cameraControlHorizontal);
    print8(24, 64, 1.5f, "%x %x", HuMemHeapSizeGet(HEAP_HEAP) - HuMemUsedMallocSizeGet(HEAP_HEAP),
        HuMemHeapSizeGet(HEAP_MODEL) - HuMemUsedMallocSizeGet(HEAP_MODEL));
    cameraControlHorizontal -= HuPadSubStkX[0] / 59.0f;
    cameraControlVertical -= (HuPadSubStkY[0] / 59.0f) * 4.0f;
    cameraControlDepth -= (HuPadTrigL[0] / 150.0f) * 4.0f;
    cameraControlDepth += (HuPadTrigR[0] / 150.0f) * 4.0f;
}

static void CreateModelLight(s16 modelId, GXColor *lightColor)
{
    s16 lightId;

    lightId = Hu3DLLightCreateV(modelId, &keyLightPosition, &keyLightDirection, lightColor);
    Hu3DLLightStaticSet(modelId, lightId, 1);
}

static u16 ChooseComputerPlayerAction(PlayerActionWork *playerWork)
{
    u16 lightId;

    lightId = 0;
    if (frandmod(playerWork->computerActionChance) == 0) {
        lightId = 0x100;
    }
    if (--playerWork->computerActionTimer < 0) {
        playerWork->computerActionChance -= frandmod(3);
        if (playerWork->computerActionChance < 2) {
            playerWork->computerActionChance = 2;
        }
        playerWork->computerActionTimer = (playerWork->computerActionInterval >> 1) + frandmod(playerWork->computerActionInterval);
    }
    if (lightId == 0) {
        if (++playerWork->computerActionCounter > 60) {
            playerWork->computerActionCounter = 0;
            lightId = 0x100;
        }
    }
    else {
        playerWork->computerActionCounter = 0;
    }
    return lightId;
}

static void ResetPlayerActionState(PlayerActionWork *playerWork, OMOBJ *playerObject)
{
    playerWork->playerState = 2016;
    playerWork->actionProgress = 0;
    playerWork->animationFrame = 0.0f;
    playerWork->stateTimer = 0.0f;
    playerWork->currentRadius = 670.0f;
    playerWork->radiusVelocity = 0.0f;
    playerWork->targetRadius = playerWork->currentRadius;
    playerWork->heightVelocity = 0.0f;
    playerWork->currentMotionTime = 0.0f;
    playerWork->targetMotionTime = playerWork->currentMotionTime;
    Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrSet(playerObject->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DMotionSet(playerObject->mdlId[0], playerObject->mtnId[0]);
}

static void UpdatePlayerStretchEffect(PlayerActionWork *playerWork, OMOBJ *playerObject, u8 actionComplete)
{
    Vec modelHookPosition;
    Vec effectPosition;
    float interpolationValue;
    s32 i;

    GetModelJointPosition(playerObject->mdlId[0], "motionC-stretch", &modelHookPosition);
    for (i = 0; i < 8; i++) {
        interpolationValue = playerWork->currentRadius + 50.0f;
        effectPosition.x = 20.0f * frandf() - 10.0f;
        effectPosition.y = playerWork->playerAngle;
        effectPosition.z = 20.0f;
        if (actionComplete != 0) {
            effectPosition.y += 180.0f;
            effectPosition.z = 10.0f;
        }
        interpolationValue = 3.0f * (30.0f * frandf() - 15.0f);
        effectPosition.y += interpolationValue + 10.0f * (0.0f > interpolationValue ? -1 : 1);
        SpawnPlayerEffect(playerWork->playerIndex, &modelHookPosition, &effectPosition);
    }
}

static void BeginPlayerActionAnimation(PlayerActionWork *playerWork, OMOBJ *playerObject, OMOBJ *opponentObject)
{
    playerWork->stateFrame = 0;
    playerWork->radiusVelocity = (770.0f - playerWork->currentRadius) / (REFRESH_RATE * 0.3f);
    playerWork->currentMotionTime = Hu3DMotionTimeGet(opponentObject->mdlId[0]);
    Hu3DMotionTimeSet(stageMarkerObjects[playerWork->playerIndex]->mdlId[0], 0.0f);
    Hu3DMotionSpeedSet(playerObject->mdlId[0], 1.0f);
    Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[7], 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    UpdatePlayerStretchEffect(playerWork, opponentObject, 1);
    CharFXPlay(playerWork->characterId, 0x123);
}

static u8 UpdatePlayerActionAnimation(PlayerActionWork *playerWork, OMOBJ *playerObject)
{
    Vec modelHookPosition;
    float progressAngle;
    float motionTime;

    playerWork->stateFrame++;
    progressAngle = 90.0f * (playerWork->stateFrame / (VERSION_NTSC ? 18.0f : 15.000001f));
    motionTime = playerWork->currentMotionTime * (1.0 - sind(progressAngle));
    Hu3DMotionTimeSet(playerObject->mdlId[0], motionTime);
    GetModelJointPosition(playerObject->mdlId[0], "motionC-stretch", &modelHookPosition);
    playerWork->playerHeight = modelHookPosition.y - playerWork->motionHeightOffset + 388.0f;
    playerWork->currentRadius += playerWork->radiusVelocity;
    if ((REFRESH_RATE * 0.3f) < playerWork->stateFrame) {
        playerWork->radiusVelocity = (670.0f - playerWork->currentRadius) / 20.0f;
        Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
        Hu3DMotionSet(playerObject->mdlId[0], playerObject->mtnId[2]);
        Hu3DMotionTimeSet(playerObject->mdlId[0], 20.0f);
        omVibrate(playerWork->playerIndex, 12, 4, 2);
        HuAudFXPlay(0x50F);
        UpdatePlayerStretchEffect(playerWork, playerObject, 0);
        return 1;
    }
    return 0;
}

static void UpdatePlayerIdlePose(PlayerActionWork *playerWork, OMOBJ *playerObject)
{
    Vec modelHookPosition;

    playerWork->currentRadius += playerWork->radiusVelocity;
    if (670.0f > playerWork->currentRadius) {
        playerWork->currentRadius = 670.0f;
    }
    GetModelJointPosition(playerObject->mdlId[0], "motionC-stretch", &modelHookPosition);
    playerWork->playerHeight = modelHookPosition.y - playerWork->motionHeightOffset + 388.0f;
}

static void UpdatePlayerAction(OMOBJ *playerObject)
{
    Vec playerModelPosition;
    Vec playerEffectPosition;
    Vec opponentModelPosition;
    Vec effectPosition;
    float interpolationValue;
    PlayerActionWork *playerWork;
    OMOBJ *opponentObject;
    PlayerActionFrameData *animationFrameData;
    u16 buttonInput;

    playerWork = playerObject->data;
    if (minigameSequenceObject->work[0] == 1007) {
        switch (playerWork->playerState) {
            case 2017:
                playerWork->playerState = 2014;
                break;
            case 2018:
                playerWork->playerState = 2015;
                break;
            default:
                playerWork->playerState = 2010;
                break;
        }
        Hu3DMotionSpeedSet(playerObject->mdlId[0], 1.0f);
        if (winningPlayerIndex == playerWork->playerIndex) {
            GWPlayerCoinWinSet(playerWork->playerIndex, 10);
        }
        playerObject->objFunc = UpdatePlayerModel;
        return;
    }
    if (minigameSequenceObject->work[0] == 1006) {
        opponentObject = shadowObjects[playerWork->playerIndex];
        if (playerWork->actionProgress < 19) {
            if (GWPlayerCfg[playerWork->playerIndex].iscom) {
                buttonInput = ChooseComputerPlayerAction(playerWork);
            }
            else {
                buttonInput = HuPadBtnDown[playerWork->controllerPort];
            }
            if (playerWork->actionProgress == 0) {
                if (buttonInput & 0x100) {
                    playerWork->actionProgress = 1;
                    playerWork->animationFrame = 0.0f;
                    playerWork->stateTimer = 0.0f;
                    playerWork->heightVelocity = 0.0f;
                    playerWork->currentMotionTime = 0.0f;
                    playerWork->targetMotionTime = playerWork->currentMotionTime;
                    playerWork->radiusVelocity = 0.0f;
                    playerWork->targetRadius = playerWork->currentRadius;
                    HuAudFXPlay(0x50E);
                    Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[5], 0.0f, 2.0f, HU3D_MOTATTR_LOOP);
                }
            }
            else {
                playerWork->stateTimer += 1.0f;
            }
            switch (playerWork->playerState) {
                case 2016:
                    if (buttonInput & 0x100) {
                        animationFrameData = &playerActionFrameData[playerWork->actionProgress];
                        if (animationFrameData->soundEffectId >= 0 && 0.0f == playerWork->animationFrame) {
                            HuAudFXPlay(animationFrameData->soundEffectId);
                        }
                        playerWork->animationFrame += 1.0f;
                        Hu3DMotionTimeSet(stageMarkerObjects[playerWork->playerIndex]->mdlId[0], animationFrameData->motionFrame);
                        interpolationValue = animationFrameData->heightRatio - animationFrameData[-1].heightRatio;
                        interpolationValue *= 1.0f - playerWork->animationFrame / animationFrameData->durationFrames;
                        playerWork->targetRadius = 670.0f - 450.0f * (animationFrameData->heightRatio - interpolationValue);
                        interpolationValue = animationFrameData->motionRatio - animationFrameData[-1].motionRatio;
                        interpolationValue *= 1.0f - playerWork->animationFrame / animationFrameData->durationFrames;
                        playerWork->targetMotionTime = (animationFrameData->motionRatio - interpolationValue) * Hu3DMotionMaxTimeGet(opponentObject->mdlId[0]);
                        interpolationValue = playerWork->actionProgress / 20.0f;
                        playerWork->radiusVelocity = interpolationValue * (playerWork->targetRadius - playerWork->currentRadius);
                        playerWork->heightVelocity = interpolationValue * (playerWork->targetMotionTime - playerWork->currentMotionTime);
                        if (animationFrameData->durationFrames <= playerWork->animationFrame) {
                            playerWork->actionProgress++;
                            if (playerWork->actionProgress == 3) {
                                Hu3DMotionSpeedSet(playerObject->mdlId[0], 0.83f);
                                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[6], 0.0f, 4.0f, HU3D_MOTATTR_LOOP);
                            }
                            if (playerWork->actionProgress == 9) {
                                Hu3DMotionSpeedSet(playerObject->mdlId[0], 1.0f);
                            }
                            playerWork->animationFrame = 0.0f;
                            playerWork->stateTimer = 0.0f;
                            if (playerWork->actionProgress >= 19) {
                                playerWork->currentRadius = playerWork->targetRadius;
                                playerWork->radiusVelocity = 0.0f;
                                playerWork->currentMotionTime = playerWork->targetMotionTime;
                                playerWork->heightVelocity = 0.0f;
                                if (winningPlayerIndex < 0) {
                                    winningPlayerIndex = playerWork->playerIndex;
                                }
                                else if (frandmod(100) > 50) {
                                    winningPlayerIndex = playerWork->playerIndex;
                                }
                            }
                        }
                    }
                    if (1.0f * REFRESH_RATE <= playerWork->stateTimer) {
                        playerWork->playerState = 2017;
                        BeginPlayerActionAnimation(playerWork, playerObject, opponentObject);
                    }
                    else {
                        if (playerWork->currentRadius > playerWork->targetRadius) {
                            playerWork->currentRadius += playerWork->radiusVelocity;
                            if (playerWork->currentRadius <= playerWork->targetRadius) {
                                playerWork->currentRadius = playerWork->targetRadius;
                            }
                        }
                        if (playerWork->currentMotionTime < playerWork->targetMotionTime) {
                            playerWork->currentMotionTime += playerWork->heightVelocity;
                            if (playerWork->currentMotionTime >= playerWork->targetMotionTime) {
                                playerWork->currentMotionTime = playerWork->targetMotionTime;
                            }
                        }
                        Hu3DMotionTimeSet(opponentObject->mdlId[0], playerWork->currentMotionTime);
                    }
                    break;
                case 2017:
                    if (UpdatePlayerActionAnimation(playerWork, opponentObject) != 0) {
                        playerWork->playerState = 2018;
                        playerWork->stateFrame = 2 * REFRESH_RATE;
                    }
                    break;
                case 2018:
                    UpdatePlayerIdlePose(playerWork, opponentObject);
                    playerWork->stateFrame -= 1.0f;
                    if (playerWork->stateFrame == 32) {
                        Hu3DMotionSet(playerObject->mdlId[0], playerObject->mtnId[9]);
                        Hu3DMotionShiftSet(opponentObject->mdlId[0], opponentObject->mtnId[1], 0.0f, 20.0f, HU3D_MOTATTR_PAUSE);
                    }
                    if (0.0f > playerWork->stateFrame) {
                        ResetPlayerActionState(playerWork, playerObject);
                    }
                    break;
                default:
                    OSReport("*** player mode error(%d)!!\n", playerWork->playerState);
                    break;
            }
            if (playerMotionRadius < playerWork->currentRadius) {
                playerMotionRadius = playerWork->currentRadius;
            }
            omSetTra(playerObject, playerWork->currentRadius * sind(180.0f + playerWork->playerAngle), playerWork->playerHeight, playerWork->currentRadius * cosd(180.0f + playerWork->playerAngle));
        }
    }
}

static void BeginPlayerJump(float motionFrame)
{
    Vec particlePosition;
    s32 i;

    for (i = 0; i < 4; i++) {
        particlePosition.x = 155.5 * sind(motionFrame);
        particlePosition.y = 588.0f;
        particlePosition.z = 155.5 * cosd(motionFrame);
        SpawnPlayerEffect(5, &particlePosition, NULL);
        motionFrame += 90.0f;
    }
}

static void BeginPlayerLand(float motionFrame)
{
    Vec particlePosition;
    s32 i;

    for (i = 0; i < 4; i++) {
        particlePosition.x = 155.5 * sind(motionFrame);
        particlePosition.y = 388.0f;
        particlePosition.z = 155.5 * cosd(motionFrame);
        particlePosition.x += 40.0f * frandf() - 20.0f;
        particlePosition.z += 40.0f * frandf() - 20.0f;
        SpawnPlayerEffect(4, &particlePosition, NULL);
        motionFrame += 90.0f;
    }
}

static void UpdatePlayerModel(OMOBJ *playerObject)
{
    Vec effectPosition;
    float motionTime;
    float rotationAngle;
    PlayerActionWork *playerWork;
    OMOBJ *playerObject;
    s32 particleIndex;

    playerWork = playerObject->data;
    playerObject = shadowObjects[playerWork->playerIndex];
    switch (playerWork->playerState) {
        case 2000:
            if (minigameSequenceObject->work[0] == 1001) {
                playerWork->playerState++;
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[1], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            }
            break;
        case 2001:
            playerWork->currentRadius -= 1.0f;
            if (Hu3DMotionEndCheck(playerObject->mdlId[0])) {
                playerWork->playerState++;
                playerWork->heightVelocity = 0.0f;
                playerWork->startingHeight = 2000.0f;
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[2], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
                Hu3DMotionSet(playerObject->mdlId[0], playerObject->mtnId[0]);
                Hu3DModelAttrSet(playerObject->mdlId[0], HU3D_MOTATTR_PAUSE);
                Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            break;
        case 2002:
            playerWork->heightVelocity += 0.4f;
            playerWork->startingHeight -= playerWork->heightVelocity;
            if (playerWork->effectPlayed != 0 && Hu3DMotionEndCheck(playerObject->mdlId[0])) {
                playerWork->effectPlayed = 0;
                CharFXPlay(playerWork->characterId, 0x128);
            }
            if (786.0f > playerWork->startingHeight) {
                playerWork->playerState++;
                if (playerWork->playerIndex == 0) {
                    HuAudFXPlay(0x50D);
                }
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[3], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            }
            break;
        case 2003:
            playerWork->heightVelocity += 0.4f;
            playerWork->startingHeight -= playerWork->heightVelocity;
            if (536.0f > playerWork->startingHeight) {
                playerWork->playerState++;
                playerWork->stateFrame = 0;
                playerWork->startingHeight = 536.0f;
                Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_PAUSE);
                effectPosition.x = playerWork->startingRadius * sind(180.0f + playerWork->playerAngle);
                effectPosition.y = playerWork->startingHeight;
                effectPosition.z = playerWork->startingRadius * cosd(180.0f + playerWork->playerAngle);
                SpawnPlayerEffect(6, &effectPosition, NULL);
                omVibrate(playerWork->playerIndex, 12, 0xC, 0);
            }
            break;
        case 2004:
            playerWork->stateFrame++;
            if (70.0f < playerWork->stateFrame) {
                playerWork->playerState++;
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[4], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            }
            break;
        case 2005:
            playerWork->stateFrame++;
            if (130.0f == playerWork->stateFrame) {
                if (debugModeEnabled != 0) {
                    debugModeEnabled = 0;
                    HuAudFXPlay(0x510);
                }
                UpdatePlayerStretchEffect(playerWork, playerObject, 0);
            }
            if (!Hu3DMotionEndCheck(playerObject->mdlId[0])) {
                playerWork->currentRadius -= 2.3f;
            }
            if (Hu3DMotionEndCheck(playerObject->mdlId[0])) {
                playerWork->playerState++;
                playerWork->stateFrame = 0;
                playerWork->currentMotionTime = 0.0f;
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[5], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[1], 0.0f, 10.0f, HU3D_MOTATTR_PAUSE);
            }
            break;
        case 2006:
            if (playerWork->stateFrame++ > 10) {
                playerWork->playerState++;
            }
            GetModelJointPosition(playerObject->mdlId[0], "motionC-stretch", &effectPosition);
            playerWork->motionHeightOffset = effectPosition.y;
            break;
        case 2007:
            playerWork->currentRadius -= 5.0f;
            if (670.0f > playerWork->currentRadius) {
                motionTime = (670.0f - playerWork->currentRadius) / 450.0f;
                Hu3DMotionTimeSet(stageMarkerObjects[playerWork->playerIndex]->mdlId[0], 10.0f * motionTime);
                particleIndex = (s32)(10.0f * motionTime) >> 1;
                if (particleIndex < 5 && playerWork->lastEffectId != playerActionSoundEffectIds[particleIndex]) {
                    playerWork->lastEffectId = playerActionSoundEffectIds[particleIndex];
                    HuAudFXPlay(playerWork->lastEffectId);
                }
            }
            if (470.0f > playerWork->currentRadius) {
                playerWork->playerState++;
                playerWork->currentRadius = 470.0f;
                BeginPlayerActionAnimation(playerWork, playerObject, playerObject);
            }
            else {
                playerWork->currentMotionTime += 0.7f;
                Hu3DMotionTimeSet(playerObject->mdlId[0], playerWork->currentMotionTime);
            }
            break;
        case 2008:
            if (UpdatePlayerActionAnimation(playerWork, playerObject) != 0) {
                playerWork->playerState++;
                playerWork->stateFrame = 2 * REFRESH_RATE;
            }
            break;
        case 2009:
            UpdatePlayerIdlePose(playerWork, playerObject);
            playerWork->stateFrame -= 1.0f;
            if (playerWork->stateFrame == 32) {
                Hu3DMotionSet(playerObject->mdlId[0], playerObject->mtnId[9]);
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[1], 0.0f, 20.0f, HU3D_MOTATTR_PAUSE);
            }
            if (0.0f > playerWork->stateFrame) {
                minigameSequenceObject->work[0]++;
                ResetPlayerActionState(playerWork, playerObject);
                playerObject->objFunc = UpdatePlayerAction;
            }
            break;

        case 2010:
            if (playerWork->playerIndex == winningPlayerIndex) {
                playerWork->playerState = 2012;
                playerWork->radiusVelocity = 10.5f;
                Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[8], 0.0f, 20.0f, HU3D_MOTATTR_NONE);
                Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[2], 20.0f, 20.0f, HU3D_MOTATTR_NONE);
                HuAudFXPlay(0x513);
                BeginPlayerJump(playerWork->playerAngle);
            }
            else {
                playerWork->playerState = 2014;
                BeginPlayerActionAnimation(playerWork, playerObject, playerObject);
            }
            break;

        case 2011:
            break;

        case 2012:
            playerWork->currentRadius -= playerWork->radiusVelocity;
            if (0.0f > playerWork->currentRadius) {
                playerWork->currentRadius = 0.0f;
            }
            playerWork->radiusVelocity -= 0.25f;
            if (0.0f > playerWork->radiusVelocity) {
                playerWork->playerState++;
                playerWork->stateFrame = 0;
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[9], 0.0f, 20.0f, HU3D_MOTATTR_NONE);
            }
            if (4.5f <= playerWork->radiusVelocity) {
                BeginPlayerLand(playerWork->playerAngle);
            }
            break;
        case 2013:
            playerWork->stateFrame++;
            rotationAngle = 90.0f * (playerWork->stateFrame / 20.0f);
            motionTime = sind(rotationAngle) * sind(rotationAngle);
            if (180.0f > playerWork->playerAngle) {
                playerWork->targetRadius = playerWork->playerAngle + -playerWork->playerAngle * motionTime;
            }
            else {
                playerWork->targetRadius = playerWork->playerAngle + (360.0f - playerWork->playerAngle) * motionTime;
            }
            omSetRot(playerObject, 0.0f, playerWork->targetRadius, 0.0f);
            if (20.0f <= playerWork->stateFrame) {
                playerWork->playerState = 2011;
            }
            break;
        case 2014:
            if (UpdatePlayerActionAnimation(playerWork, playerObject) != 0) {
                playerWork->playerState++;
                playerWork->stateFrame = 2 * REFRESH_RATE;
            }
            break;
        case 2015:
            UpdatePlayerIdlePose(playerWork, playerObject);
            playerWork->stateFrame -= 1.0f;
            if (playerWork->stateFrame == 32) {
                Hu3DMotionSet(playerObject->mdlId[0], playerObject->mtnId[9]);
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[1], 0.0f, 20.0f, HU3D_MOTATTR_PAUSE);
            }
            if (0.0f > playerWork->stateFrame) {
                playerWork->playerState = 2011;
            }
            break;
        default:
            OSReport("*** player mode error(%d)!!\n", playerWork->playerState);
            break;
    }
    omSetTra(playerObject, playerWork->currentRadius * sind(180.0f + playerWork->playerAngle), playerWork->playerHeight, playerWork->currentRadius * cosd(180.0f + playerWork->playerAngle));
    omSetTra(playerObject, playerWork->startingRadius * sind(180.0f + playerWork->playerAngle), playerWork->startingHeight, playerWork->startingRadius * cosd(180.0f + playerWork->playerAngle));
    if (minigameSequenceObject->work[0] < 1008 && playerWork->playerIndex == winningPlayerIndex) {
        cameraCurrentTarget.x = playerWork->currentRadius * sind(180.0f + playerWork->playerAngle);
        cameraCurrentTarget.y = 508.0f;
        cameraCurrentTarget.z = playerWork->currentRadius * cosd(180.0f + playerWork->playerAngle);
        cameraCurrentPosition.x = cameraCurrentTarget.x;
        cameraCurrentPosition.y = cameraCurrentTarget.y + 270.0f;
        cameraCurrentPosition.z = cameraCurrentTarget.z + 850.0f;
    }
}

static void CreatePlayerModel(OMOBJ *playerObject)
{
    PlayerActionWork *playerWork;
    s16 modelId;
    s16 lightId;
    s32 i;

    playerWork = playerObject->data;
    playerWork->playerIndex = playerObject->work[0];
    playerWork->characterId = GWPlayerCfg[playerWork->playerIndex].character;
    playerWork->controllerPort = GWPlayerCfg[playerWork->playerIndex].pad_idx;
    playerWork->playerState = 2000;
    playerWork->actionProgress = 0;
    playerWork->playerHeight = 388.0f;
    playerWork->animationFrame = 0.0f;
    playerWork->stateTimer = 0.0f;
    playerWork->currentMotionTime = 0.0f;
    playerWork->startingRadius = 895.0f;
    playerWork->startingHeight = 536.0f;
    playerWork->lastEffectId = -1;
    playerWork->effectPlayed = 1;
    playerWork->computerActionChance = 10;
    playerWork->computerActionTimer = 10;
    playerWork->computerActionInterval = 10;
    playerWork->computerActionCounter = 60;
    playerObject->mdlId[0] = CharModelCreate(characterModelIndices[playerWork->characterId], 4);
    CharModelStepFxSet(characterModelIndices[playerWork->characterId], 0);
    for (i = 0; i < 11; i++) {
        playerObject->mtnId[i] = CharMotionCreate(characterModelIndices[playerWork->characterId], characterMotionResourceIds[playerWork->characterId][i]);
        CharMotionSet(characterModelIndices[playerWork->characterId], playerObject->mtnId[i]);
    }
    modelId = playerObject->mdlId[0];
    lightId = Hu3DLLightCreateV(modelId, &keyLightPosition, &keyLightDirection, &whiteLightColor);
    Hu3DLLightStaticSet(modelId, lightId, 1);
    Hu3DModelAttrSet(playerObject->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(playerObject->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(playerObject->mdlId[0], 3);
    Hu3DMotionSet(playerObject->mdlId[0], playerObject->mtnId[0]);
    playerWork->playerAngle = 135.0f + 90.0f * playerWork->playerIndex;
    playerWork->currentRadius = 895.0f;
    omSetTra(playerObject, playerWork->currentRadius * sind(180.0f + playerWork->playerAngle), playerWork->playerHeight, playerWork->currentRadius * cosd(180.0f + playerWork->playerAngle));
    omSetRot(playerObject, 0.0f, playerWork->playerAngle, 0.0f);
    Hu3DModelShadowSet(playerObject->mdlId[0]);
    Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_ATTR_DISPOFF);
    if (GWPlayerCfg[playerWork->playerIndex].iscom) {
        switch (GWPlayerCfg[playerWork->playerIndex].diff) {
            case 0:
                playerWork->computerActionChance = 26;
                playerWork->computerActionInterval = 90;
                break;
            case 1:
                playerWork->computerActionChance = 24;
                playerWork->computerActionInterval = 80;
                break;
            case 2:
                playerWork->computerActionChance = 22;
                playerWork->computerActionInterval = 70;
                break;
            case 3:
            default:
                playerWork->computerActionChance = 18;
                playerWork->computerActionInterval = 40;
                break;
        }
        playerWork->computerActionTimer = (playerWork->computerActionInterval >> 1) + frandmod(playerWork->computerActionInterval);
    }
    CharMotionDataClose(characterModelIndices[playerWork->characterId]);
    playerObject->objFunc = UpdatePlayerModel;
}

static void HideShadowModel(OMOBJ *shadowObject) { }

static void CreateShadowModel(OMOBJ *shadowObject)
{
    float playerAngle;
    float playerHeight;
    float playerRadius;
    s32 playerIndex;
    s32 i;

    playerIndex = shadowObject->work[0];
    shadowObject->mdlId[0] = Hu3DModelCreateFile(stageMarkerModelFiles[playerIndex]);
    for (i = 0; i < 3; i++) {
        shadowObject->mtnId[i] = Hu3DJointMotionFile(shadowObject->mdlId[0], stagePropMotionFiles[i]);
    }
    Hu3DModelAttrSet(shadowObject->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(shadowObject->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(shadowObject->mdlId[0], 4);
    Hu3DMotionSet(shadowObject->mdlId[0], shadowObject->mtnId[0]);
    playerAngle = ((PlayerActionWork *)playerObjects[playerIndex]->data)->playerAngle;
    playerHeight = ((PlayerActionWork *)playerObjects[playerIndex]->data)->startingHeight;
    playerRadius = ((PlayerActionWork *)playerObjects[playerIndex]->data)->startingRadius;
    omSetTra(shadowObject, playerRadius * sind(180.0f + playerAngle), playerHeight, playerRadius * cosd(180.0f + playerAngle));
    omSetRot(shadowObject, 0.0f, playerAngle, 0.0f);
    Hu3DModelShadowSet(shadowObject->mdlId[0]);
    shadowObject->objFunc = HideShadowModel;
}

static void UpdatePlayerEffect(OMOBJ *effectObject)
{
    Vec *basePosition;
    PlayerEffectWork *effectWork;
    float horizontalDistance;

    basePosition = &playerEffectSpawnPositions[effectObject->work[0]];
    effectWork = effectObject->data;
    switch (effectWork->motionState) {
        case 3000:
            effectWork->pauseTimer -= 1.0f;
            if (0.0f > effectWork->pauseTimer && frandmod(20) == 0) {
                effectWork->motionState = 3001;
                effectWork->rotationY = 360.0f * frandf();
                effectWork->verticalVelocity = 10.0f;
                HuAudFXPlay(0x512);
                Hu3DMotionShiftSet(effectObject->mdlId[0], effectObject->mtnId[0], 0.0f, 20.0f, HU3D_MOTATTR_NONE);
            }
            break;
        case 3001:
            effectWork->positionX += sind(effectWork->rotationY);
            effectWork->positionZ += cosd(effectWork->rotationY);
            horizontalDistance = effectWork->positionX - basePosition->x;
            if (50.0f < ABS(horizontalDistance)) {
                effectWork->positionX = basePosition->x + 50.0f * (0.0f > horizontalDistance ? -1 : 1);
            }
            horizontalDistance = effectWork->positionZ - basePosition->z;
            if (50.0f < ABS(horizontalDistance)) {
                effectWork->positionZ = basePosition->z + 50.0f * (0.0f > horizontalDistance ? -1 : 1);
            }
            effectWork->positionY += effectWork->verticalVelocity;
            effectWork->verticalVelocity -= 0.5f;
            if (basePosition->y > effectWork->positionY) {
                effectWork->motionState = 3000;
                effectWork->pauseTimer = 20.0f;
                effectWork->positionY = basePosition->y;
                Hu3DMotionShiftSet(effectObject->mdlId[0], effectObject->mtnId[1], 0.0f, 20.0f, HU3D_MOTATTR_NONE);
            }
            break;
    }
    omSetTra(effectObject, effectWork->positionX, effectWork->positionY, effectWork->positionZ);
    omSetRot(effectObject, 0.0f, effectWork->rotationY, 0.0f);
}

static void CreatePlayerEffect(OMOBJ *effectObject)
{
    Vec *basePosition;
    PlayerEffectWork *effectWork;
    s32 i;

    basePosition = &playerEffectSpawnPositions[effectObject->work[0]];
    effectWork = effectObject->data;
    effectObject->mdlId[0] = Hu3DModelCreateFile(stagePropModelFile);
    for (i = 0; i < 2; i++) {
        effectObject->mtnId[i] = Hu3DJointMotionFile(effectObject->mdlId[0], stagePropSecondaryMotions[i]);
    }
    Hu3DModelAttrSet(effectObject->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(effectObject->mdlId[0], 5);
    Hu3DMotionSet(effectObject->mdlId[0], effectObject->mtnId[1]);
    effectWork->motionState = 3000;
    effectWork->pauseTimer = 0.0f;
    effectWork->positionX = basePosition->x;
    effectWork->positionY = basePosition->y;
    effectWork->positionZ = basePosition->z;
    effectWork->rotationY = 360.0f * frandf();
    omSetTra(effectObject, effectWork->positionX, effectWork->positionY, effectWork->positionZ);
    omSetRot(effectObject, 0.0f, effectWork->rotationY, 0.0f);
    Hu3DModelShadowSet(effectObject->mdlId[0]);
    Hu3DModelAttrReset(effectObject->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(effectObject->mdlId[0], HU3D_ATTR_DISPOFF);
    effectObject->objFunc = UpdatePlayerEffect;
}

static void DeleteEffectObject(OMOBJ *effectObject)
{
    Hu3DModelAttrSet(effectObject->mdlId[0], HU3D_ATTR_DISPOFF);
    RegisterResourceValue(effectObject->work[0], effectObject->mdlId[0]);
    omDelObjEx(minigameObjectManager, effectObject);
    activeEffectObjectCount--;
}

static void UpdateEffectObject(OMOBJ *effectObject)
{
    Mtx rotationMatrix;
    Vec effectPosition;
    float verticalSpeed;
    float motionDuration;
    float fadeDuration;
    float fadeAmount;
    EffectObjectWork *effectWork;

    effectWork = effectObject->data;
    switch (effectObject->work[0]) {
        case 0:
        case 1:
        case 2:
        case 3:
            verticalSpeed = effectWork->velocity.z;
            effectPosition.x = 0.0f;
            effectPosition.y = 0.0f;
            effectPosition.z = verticalSpeed;
            effectWork->velocity.z *= 0.92f;
            MTXIdentity(rotationMatrix);
            mtxRot(rotationMatrix, effectWork->velocity.x, effectWork->velocity.y, 0.0f);
            MTXMultVec(rotationMatrix, &effectPosition, &effectPosition);
            VECAdd(&effectPosition, &effectWork->position, &effectWork->position);
            omSetTra(effectObject, effectWork->position.x, effectWork->position.y, effectWork->position.z);
            Hu3DModelTPLvlSet(effectObject->mdlId[0], effectWork->transparency);
            effectWork->transparency -= 0.02f;
            if (0.1f > effectWork->transparency) {
                DeleteEffectObject(effectObject);
            }
            break;
        case 4:
            effectWork->position.y += effectWork->transparency;
            if (390.0f >= effectWork->position.y) {
                effectWork->position.y = 390.0f;
            }
            omSetTra(effectObject, effectWork->position.x, effectWork->position.y, effectWork->position.z);
            if (390.0f >= effectWork->position.y) {
                break;
            }
            effectWork->rotationX += 8.0f + frandmod(8);
            effectWork->rotationY += 8.0f + frandmod(8);
            effectWork->rotationZ += 8.0f + frandmod(8);
            omSetRot(effectObject, effectWork->rotationX, effectWork->rotationY, effectWork->rotationZ);
            if (0.0f < effectWork->transparency) {
                effectWork->transparency -= 0.3f;
                break;
            }
            if (0.0f == effectWork->transparencyDecay) {
                effectWork->transparencyDecay = 0.003f + 0.006f * frandf();
            }
            effectWork->transparency -= effectWork->transparencyDecay;
            effectWork->position.x += effectWork->velocity.x;
            effectWork->position.z += effectWork->velocity.z;
            if (!(minigameFrameCounter & 3)) {
                effectWork->velocity.x = 6.0f * frandf() - 3.0f;
                effectWork->velocity.z = 6.0f * frandf() - 3.0f;
            }
            break;
        case 5:
            if (Hu3DMotionEndCheck(effectObject->mdlId[0])) {
                DeleteEffectObject(effectObject);
            }
            break;
        case 6:
            effectPosition = defaultCameraPosition;
            motionDuration = Hu3DMotionMaxTimeGet(effectObject->mdlId[0]);
            fadeAmount = 1.0f - Hu3DMotionTimeGet(effectObject->mdlId[0]) / motionDuration;
            effectPosition.x += 100.0f * fadeAmount * frandf() * (frandmod(100) > 50 ? 1.0f : -1.0f);
            effectPosition.z += 100.0f * fadeAmount * frandf() * (frandmod(100) > 50 ? 1.0f : -1.0f);
            Hu3DCameraPosSetV(1, &effectPosition, &worldUpVector, &defaultCameraTarget);
            cameraStartPosition = effectPosition;
            cameraStartTarget = defaultCameraTarget;
            fadeDuration = 0.5f * Hu3DMotionMaxTimeGet(effectObject->mdlId[0]);
            if (fadeDuration < Hu3DMotionTimeGet(effectObject->mdlId[0])) {
                fadeAmount = 1.0f - (Hu3DMotionTimeGet(effectObject->mdlId[0]) - fadeDuration) / fadeDuration;
                Hu3DModelTPLvlSet(effectObject->mdlId[0], fadeAmount);
                fadeAmount = 1.0f - fadeAmount;
                omSetSca(effectObject, effectWork->scaleX + fadeAmount, effectWork->scaleY + fadeAmount, effectWork->scaleZ + fadeAmount);
            }
            if (Hu3DMotionEndCheck(effectObject->mdlId[0])) {
                Hu3DCameraPosSetV(1, &defaultCameraPosition, &worldUpVector, &defaultCameraTarget);
                cameraStartPosition = defaultCameraPosition;
                cameraStartTarget = defaultCameraTarget;
                DeleteEffectObject(effectObject);
            }
            break;
        default:
            OSReport("effect id error!!(%d)\n", effectObject->work[0]);
            break;
    }
}

static void CreateEffectModel(OMOBJ *effectObject)
{
    EffectObjectWork *effectWork;
    s16 cachedModelId;

    effectWork = effectObject->data;
    effectWork->reuseDelay--;
    if (effectWork->reuseDelay <= 0) {
        cachedModelId = TakeResourceValue(effectObject->work[0]);
        if (cachedModelId < 0) {
            effectObject->mdlId[0] = Hu3DModelLink(cachedEffectModelIds[effectObject->work[0]]);
        }
        else {
            effectObject->mdlId[0] = cachedModelId;
        }
        Hu3DMotionTimeSet(effectObject->mdlId[0], 0.0f);
        Hu3DModelAttrSet(effectObject->mdlId[0], HU3D_ATTR_DISPOFF);
        Hu3DModelLayerSet(effectObject->mdlId[0], 2);
        omSetTra(effectObject, effectWork->position.x, effectWork->position.y, effectWork->position.z);
        omSetRot(effectObject, effectWork->rotationX, effectWork->rotationY, effectWork->rotationZ);
        omSetSca(effectObject, effectWork->scaleX, effectWork->scaleY, effectWork->scaleZ);
        Hu3DModelAttrReset(effectObject->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(effectObject->mdlId[0], HU3D_ATTR_DISPOFF);
        effectObject->objFunc = UpdateEffectObject;
    }
}

static void SpawnPlayerEffect(s32 effectId, Vec *effectPosition, Vec *effectVelocity)
{
    OMOBJ *effectObject;
    EffectObjectWork *effectWork;

    if (activeEffectObjectCount < 300) {
        activeEffectObjectCount++;
        effectObject = omAddObjEx(minigameObjectManager, 105, 1, 1, 5, CreateEffectModel);
        effectObject->work[0] = effectId;
        effectObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*effectWork), HU_MEMNUM_OVL);
        effectWork = effectObject->data;
        effectWork->rotationX = 0.0f;
        effectWork->rotationY = 0.0f;
        effectWork->rotationZ = 0.0f;
        effectWork->scaleX = 1.0f;
        effectWork->scaleY = 1.0f;
        effectWork->scaleZ = 1.0f;
        effectWork->velocity.x = 0.0f;
        effectWork->velocity.y = 0.0f;
        effectWork->velocity.z = 0.0f;
        effectWork->position = *effectPosition;
        if (NULL != effectVelocity) {
            effectWork->velocity = *effectVelocity;
        }
        effectWork->reuseDelay = frandmod(10);
        switch (effectId) {
            case 0:
            case 1:
            case 2:
            case 3:
                effectWork->transparency = 1.0f;
                break;
            case 4:
                effectWork->transparency = 12.0f + 2.0f * frandf();
                effectWork->transparencyDecay = 0.0f;
                effectWork->scaleX = 8.0f;
                effectWork->scaleY = 8.0f;
                effectWork->scaleZ = 8.0f;
                break;
            case 5:
                effectWork->scaleX = 4.0f;
                effectWork->scaleY = 4.0f;
                effectWork->scaleZ = 4.0f;
                break;
        }
    }
}

static void HideStageMarker(OMOBJ *markerObject) { }

static void CreateStageMarker(OMOBJ *markerObject)
{
    markerObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M402, 35));
    Hu3DModelAttrSet(markerObject->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(markerObject->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(markerObject->mdlId[0], 0);
    omSetTra(markerObject, 0.0f, 0.0f, 0.0f);
    omSetRot(markerObject, 0.0f, 270.0f + 90.0f * markerObject->work[0], 0.0f);
    Hu3DModelShadowMapSet(markerObject->mdlId[0]);
    Hu3DModelAttrReset(markerObject->mdlId[0], HU3D_ATTR_DISPOFF);
    markerObject->objFunc = HideStageMarker;
}

static void HideStageProp(OMOBJ *propObject) { }

static void CreateStageProp(OMOBJ *propObject)
{
    float playerAngle;

    propObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M402, 36));
    Hu3DModelAttrSet(propObject->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(propObject->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(propObject->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(propObject->mdlId[0], 1);
    playerAngle = 135.0f + 90.0f * propObject->work[0];
    omSetTra(propObject, 155.5 * sind(playerAngle), 0.0f, 155.5 * cosd(playerAngle));
    omSetRot(propObject, 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrReset(propObject->mdlId[0], HU3D_ATTR_DISPOFF);
    propObject->objFunc = HideStageProp;
}

static inline void UpdateRotatingLightInline(Vec *lightDirection, float directionX, float directionY, float directionZ)
{
    lightDirection->x = directionX;
    lightDirection->y = directionY;
    lightDirection->z = directionZ;
}

static void UpdateRotatingLight(void)
{
    Vec lightPosition;
    Vec lightDirection;

    lightRotationAngle += lightRotationSpeed;
    if (360.0f < lightRotationAngle) {
        lightRotationSpeed -= 0.2f;
        if (0.0f > lightRotationSpeed) {
            lightRotationSpeed = 0.0f;
        }
    }
    lightPosition.x = 2000.0 * cosd(lightRotationAngle);
    lightPosition.y = 2000.0 * sind(lightRotationAngle);
    lightPosition.z = 0.0f;
    UpdateRotatingLightInline(&lightDirection, -lightPosition.x, -lightPosition.y, -lightPosition.z);
    Hu3DGLightPosSetV(globalLightId, &lightPosition, &lightDirection);
}

static void UpdateMinigameSequence(OMOBJ *sequenceObject)
{
    Vec cameraTarget;
    float motionProgress;
    OMOBJ *playerObject;

    if (omSysExitReq != 0) {
        FadeBackgroundSequence(-1);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        sequenceObject->objFunc = CleanupMinigame;
    }
    minigameFrameCounter++;
    switch (sequenceObject->work[0]) {
        case 1000:
            if (WipeStatGet() == 0) {
                sequenceObject->work[0]++;
            }
            break;
        case 1001:
        case 1002:
        case 1003:
        case 1004:
            break;
        case 1005:
            if (activeSequenceId < 0) {
                activeSequenceId = MGSeqCreate(3, 0);
            }
            else {
                if (backgroundSequenceId < 0 && (MGSeqStatGet(activeSequenceId) & 0x10)) {
                    backgroundSequenceId = HuAudSeqPlay(0x46);
                }
                if (MGSeqStatGet(activeSequenceId) == 0) {
                    sequenceObject->work[0]++;
                    timerFrameCount = 30 * REFRESH_RATE;
                    countdownSequenceId = MGSeqCreate(1, timerFrameCount / REFRESH_RATE, -1, -1);
                    activeSequenceId = -1;
                }
            }
            break;
        case 1006:
            MGSeqParamSet(countdownSequenceId, 1, (timerFrameCount + REFRESH_RATE - 1) / REFRESH_RATE);
            timerFrameCount--;
            if (timerFrameCount < 0 || winningPlayerIndex >= 0) {
                timerFrameCount = 30 * REFRESH_RATE;
                cameraTransitionFrame = 0.0f;
                sequenceObject->work[0] = 1007;
            }
            if (670.0f <= playerMotionRadius) {
                playerMotionRadius = 670.0f;
            }
            motionProgress = 2.5f * (670.0f - playerMotionRadius);
            cameraTarget = defaultCameraPosition;
            cameraTarget.y = cameraStartPosition.y + (defaultCameraPosition.y - motionProgress - cameraStartPosition.y) / 3.0f;
            cameraTarget.z = cameraStartPosition.z + (defaultCameraPosition.z - motionProgress - cameraStartPosition.z) / 3.0f;
            SetCameraPositionAndTarget(&cameraTarget, &worldUpVector, &defaultCameraTarget);
            playerMotionRadius = 0.0f;
            break;
        case 1007:
            if (activeSequenceId < 0) {
                MGSeqParamSet(countdownSequenceId, 2, -1);
                countdownSequenceId = -1;
                activeSequenceId = MGSeqCreate(3, 1);
                FadeBackgroundSequence(100);
            }
            else {
                cameraTransitionFrame += 1.0f;
                if (1.5f * REFRESH_RATE >= cameraTransitionFrame) {
                    AnimateCameraTransition(cameraTransitionFrame / (1.5f * REFRESH_RATE), &worldUpVector);
                }
                if (MGSeqStatGet(activeSequenceId) == 0) {
                    sequenceObject->work[0]++;
                    activeSequenceId = -1;
                    cameraTransitionFrame = 0.0f;
                }
            }
            break;
        case 1008:
            if (activeSequenceId < 0) {
                if (winningPlayerIndex < 0) {
                    activeSequenceId = MGSeqCreate(3, 2);
                    HuAudSStreamPlay(4);
                }
                else {
                    activeSequenceId = MGSeqCreate(5, 3, GWPlayerCfg[winningPlayerIndex].character, -1, -1, -1);
                    playerObject = playerObjects[winningPlayerIndex];
                    Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[10], 0.0f, 20.0f, HU3D_MOTATTR_NONE);
                    HuAudSStreamPlay(1);
                }
            }
            else {
                cameraTransitionFrame += 1.0f;
                if (winningPlayerIndex >= 0 && 110.0f == cameraTransitionFrame) {
                    Hu3DModelAttrSet(sequenceObject->mdlId[1], HU3D_ATTR_DISPOFF);
                    Hu3DModelAttrReset(sequenceObject->mdlId[2], HU3D_MOTATTR_PAUSE);
                    Hu3DModelAttrReset(sequenceObject->mdlId[2], HU3D_ATTR_DISPOFF);
                    Hu3DMotionTimeSet(sequenceObject->mdlId[2], 0.0f);
                    Hu3DMotionSpeedSet(sequenceObject->mdlId[2], 3.0f);
                }
                if (210.0f < cameraTransitionFrame && MGSeqStatGet(activeSequenceId) == 0) {
                    FadeBackgroundSequence(-1);
                    activeSequenceId = -1;
                    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                    sequenceObject->objFunc = CleanupMinigame;
                }
            }
            break;
        default:
            OSReport("*** main mode error(%d)!!\n", sequenceObject->work[0]);
            break;
    }
    sequenceModelTimer -= 1.0f;
    if (0.0f == sequenceModelTimer) {
        Hu3DModelAttrSet(sequenceObject->mdlId[1], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(sequenceObject->mdlId[2], HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(sequenceObject->mdlId[2], HU3D_ATTR_DISPOFF);
        Hu3DMotionTimeSet(sequenceObject->mdlId[2], 0.0f);
        Hu3DMotionSpeedSet(sequenceObject->mdlId[2], 3.0f);
    }
    if (0.0f > sequenceModelTimer) {
        sequenceModelTimer = -1.0f;
        UpdateRotatingLight();
        if (Hu3DMotionEndCheck(sequenceObject->mdlId[2])) {
            Hu3DModelAttrReset(sequenceObject->mdlId[1], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(sequenceObject->mdlId[2], HU3D_ATTR_DISPOFF);
        }
    }
}

static void CreateMinigameSequence(OMOBJ *sequenceObject)
{
    s16 opponentObject;
    s16 animationFrameData;
    s32 i;

    sequenceObject->work[0] = 1000;
    sequenceObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M402, 32));
    sequenceObject->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M402, 33));
    sequenceObject->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M402, 34));
    for (i = 0; i < 3; i++) {
        Hu3DModelAttrSet(sequenceObject->mdlId[i], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(sequenceObject->mdlId[i], HU3D_MOTATTR_PAUSE);
        Hu3DModelLayerSet(sequenceObject->mdlId[i], 0);
        Hu3DModelPosSet(sequenceObject->mdlId[i], 0.0f, 0.0f, 0.0f);
    }
    Hu3DModelReflectTypeSet(sequenceObject->mdlId[0], 3);
    Hu3DModelReflectTypeSet(sequenceObject->mdlId[1], 3);
    Hu3DModelReflectTypeSet(sequenceObject->mdlId[2], 3);
    opponentObject = sequenceObject->mdlId[0];
    animationFrameData = Hu3DLLightCreateV(opponentObject, &keyLightPosition, &keyLightDirection, &whiteLightColor);
    Hu3DLLightStaticSet(opponentObject, animationFrameData, 1);
    Hu3DModelAttrReset(sequenceObject->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(sequenceObject->mdlId[1], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowMapSet(sequenceObject->mdlId[0]);
    HuAudFXPlay(0x519);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    sequenceObject->objFunc = UpdateMinigameSequence;
}

void ObjectSetup(void)
{
    Vec shadowDirection;
    OMOBJ *createdObject;
    s32 i;

    OSReport("******* M402 ObjectSetup *********\n");
    minigameObjectManager = omInitObjMan(400, 0x2000);
    omGameSysInit(minigameObjectManager);
    minigameFrameCounter = 0;
    activeSequenceId = -1;
    countdownSequenceId = -1;
    winningPlayerIndex = -1;
    backgroundSequenceId = -1;
    debugModeEnabled = 1;
    cameraControlHorizontal = 0.0f;
    cameraControlVertical = 2200.0f;
    cameraControlDepth = 2500.0f;
    cameraCurrentPosition = defaultCameraPosition;
    cameraCurrentTarget = defaultCameraTarget;
    Hu3DReflectNoSet(0);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 30.0f, 20.0f, 20000.0f, 1.2f);
    SetCameraPositionAndTarget(&defaultCameraPosition, &worldUpVector, &defaultCameraTarget);
    lightRotationAngle = 180.0f;
    lightRotationSpeed = 4.0f;
    sequenceModelTimer = 60.0f;
    globalLightId = Hu3DGLightCreateV(&keyLightPosition, &keyLightDirection, &whiteLightColor);
    Hu3DGLightStaticSet(globalLightId, 1);
    UpdateRotatingLight();
    Hu3DShadowCreate(30.0f, 20.0f, 20000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    VECNormalize(&shadowLightPosition, &shadowDirection);
    Hu3DShadowPosSet(&shadowLightPosition, &shadowDirection, &shadowTargetPosition);
    omMakeGroupEx(minigameObjectManager, 0, 4);
    stageMarkerObjects = omGetGroupMemberListEx(minigameObjectManager, 0);
    for (i = 0; i < 4; i++) {
        createdObject = omAddObjEx(minigameObjectManager, 100, 1, 0, 0, CreateStageMarker);
        createdObject->work[0] = i;
    }
    omMakeGroupEx(minigameObjectManager, 1, 4);
    stagePropObjects = omGetGroupMemberListEx(minigameObjectManager, 1);
    for (i = 0; i < 4; i++) {
        createdObject = omAddObjEx(minigameObjectManager, 101, 1, 0, 1, CreateStageProp);
        createdObject->work[0] = i;
    }
    omMakeGroupEx(minigameObjectManager, 2, 4);
    playerObjects = omGetGroupMemberListEx(minigameObjectManager, 2);
    for (i = 0; i < 4; i++) {
        createdObject = omAddObjEx(minigameObjectManager, 102, 1, 11, 2, CreatePlayerModel);
        createdObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerActionWork), HU_MEMNUM_OVL);
        createdObject->work[0] = i;
    }
    omMakeGroupEx(minigameObjectManager, 3, 4);
    shadowObjects = omGetGroupMemberListEx(minigameObjectManager, 3);
    for (i = 0; i < 4; i++) {
        createdObject = omAddObjEx(minigameObjectManager, 103, 1, 3, 3, CreateShadowModel);
        createdObject->work[0] = i;
    }
    omMakeGroupEx(minigameObjectManager, 4, 2);
    playerEffectObjects = omGetGroupMemberListEx(minigameObjectManager, 4);
    for (i = 0; i < 2; i++) {
        createdObject = omAddObjEx(minigameObjectManager, 104, 1, 2, 4, CreatePlayerEffect);
        createdObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerEffectWork), HU_MEMNUM_OVL);
        createdObject->work[0] = i;
    }
    InitializeResourceList();
    activeEffectObjectCount = 0;
    omMakeGroupEx(minigameObjectManager, 5, 300);
    transientEffectObjects = omGetGroupMemberListEx(minigameObjectManager, 5);
    for (i = 0; i < 7; i++) {
        cachedEffectModelIds[i] = Hu3DModelCreateFile(effectModelFiles[i]);
        Hu3DModelAttrSet(cachedEffectModelIds[i], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(cachedEffectModelIds[i], HU3D_MOTATTR_PAUSE);
    }
    minigameSequenceObject = omAddObjEx(minigameObjectManager, 106, 3, 1, -1, CreateMinigameSequence);
}

static void CleanupMinigame(OMOBJ *sequenceObject)
{
    s32 i;

    if (WipeStatGet() == 0) {
        if (activeSequenceId >= 0) {
            MGSeqKill(activeSequenceId);
        }
        if (countdownSequenceId >= 0) {
            MGSeqKill(countdownSequenceId);
        }
        MGSeqKillAll();
        HuAudAllStop();
        for (i = 0; i < 4; i++) {
            CharModelKill(characterModelIndices[GWPlayerCfg[i].character]);
        }
        omOvlReturnEx(1, 1);
    }
}
