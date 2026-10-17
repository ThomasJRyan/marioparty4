#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/frand.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "dolphin.h"
#include "ext_math.h"

typedef struct {
    /* 0x00 */ s16 mainModelId;
    /* 0x02 */ s16 characterModelId;
    /* 0x04 */ s16 effectModelId;
    /* 0x06 */ s16 motionIds[8];
    /* 0x16 */ s16 motionFrames[4];
    /* 0x1E */ s16 endingMotionState;
    /* 0x20 */ s32 playerIndex;
    /* 0x24 */ s32 animationTimer;
    /* 0x28 */ s32 controllerPort;
    /* 0x2C */ s32 playerState;
    /* 0x30 */ s32 particleEffectFlags;
    /* 0x34 */ s32 movementMode;
    /* 0x38 */ char reservedMotionData[4];
    /* 0x3C */ s32 animationCountdown;
    /* 0x40 */ char reservedPlayerData[4];
    /* 0x44 */ s32 effectDelay;
    /* 0x48 */ s32 footstepFrame;
    /* 0x4C */ Vec position;
    /* 0x58 */ float savedPositionX;
    /* 0x5C */ float savedPositionZ;
    /* 0x60 */ Vec modelRotation;
    /* 0x6C */ float particleOriginX;
    /* 0x70 */ float movementSpeed;
    /* 0x74 */ float particleOriginZ;
    /* 0x78 */ Vec targetPosition;
    /* 0x84 */ float collisionRadius;
    /* 0x88 */ float moveSpeed;
    /* 0x8C */ float cameraRotation;
    /* 0x90 */ float rotationStep;
    /* 0x94 */ float facingAngle;
} SpotlightPlayerWork; // Size 0x98

typedef struct {
    /* 0x00 */ s16 state;
    /* 0x02 */ s16 characterModelId;
    /* 0x04 */ s16 motionIds[3];
    /* 0x0A */ char reservedSpriteData[2];
    /* 0x0C */ s32 objectId;
    /* 0x10 */ s32 motionId;
    /* 0x14 */ Vec position;
    /* 0x20 */ float timer;
    /* 0x24 */ float transitionProgress;
    /* 0x28 */ float movementSpeed;
    /* 0x2C */ float rotation;
    /* 0x30 */ float targetRotation;
    /* 0x34 */ float scale;
    /* 0x38 */ s32 animationState;
    /* 0x3C */ s32 animationTimer;
    /* 0x40 */ float targetX;
    /* 0x44 */ float targetZ;
} MovingObjectWork; // Size 0x48

void InitializeSpotlightScene(void);
float NormalizeAngleDegrees(float angleDegrees);
void HideScoreDigits(void);
void SetScoreDigits(s32 score, s32 isHighScore);
void AnimateMovingObjectSideToSide(void);
void RunMovingObjectProcess(void);
void RunPlayerController(void);
void RunPlayerEndSequence(void);
void UpdateBurstParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void UpdateSparkParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void UpdateStarParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void RunMinigameSequence(void);
s32 IsPositionOccupied(float x, float z, float clearance, s32 objectCount, s32 ignoredObjectIndex);

s16 spotlightLightId;
s16 spotlightModelId;
s32 minigameState;
s32 elapsedFrames;
s32 scoreSpriteGroup;
s32 currentScore;
SpotlightPlayerWork playerWork[1];
MovingObjectWork playerObjectWork[10];
MovingObjectWork effectWork;
OMOBJ *sceneObject;
Vec shadowLightPosition;
Vec shadowLightDirection;
Vec shadowOrigin;

s32 characterModelIds[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

s32 characterMotionIds[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 79), DATA_MAKE_NUM(DATADIR_MARIOMOT, 27), DATA_MAKE_NUM(DATADIR_MARIOMOT, 75),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 22) };

Vec characterModelOffsets[] = { { 0.0f, 0.0f, 35.0f }, { 0.0f, 0.0f, 35.0f }, { 0.0f, 0.0f, 35.0f }, { 0.0f, 0.0f, 35.0f }, { 0.0f, 0.0f, 35.0f },
    { 0.0f, 0.0f, 35.0f }, { 0.0f, 0.0f, 35.0f }, { 0.0f, 0.0f, 35.0f } };

Vec movingObjectSpawnPositions[] = { { -900.0f, 143.0f, -1020.0f }, { -677.0f, 143.0f, -1020.0f }, { 900.0f, 0.0f, -1150.0f }, { -307.0f, 182.0f, -1575.0f } };

s32 scoreProbabilityThresholds[][5] = { { 50, 80, 90, 95, 100 }, { 5, 55, 85, 95, 100 }, { 0, 5, 55, 90, 100 } };

float movingObjectScaleFactors[] = { 0.3f, 0.5f, 0.7f, 0.9f, 1.0f };

void ObjectSetup(void)
{
    HUPROCESS *process;

    process = omInitObjMan(50, 0x2000);
    omGameSysInit(process);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 30.0f, 20.0f, 10000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 640, 480);
    sceneObject = omAddObjEx(process, 0x7FDA, 0, 0, -1, omOutViewMulti);
    sceneObject->work[0] = 1;
    CRotM->x = -26.0f;
    CRotM->y = 0.0f;
    CRotM->z = 0.0f;
    CenterM->x = 0.0f;
    CenterM->y = 0.0f;
    CenterM->z = -295.0f;
    CZoomM[0] = 3030.0f;
    Hu3DLighInit();
    spotlightLightId = Hu3DGLightCreate(10000.0f, 0.0f, 10000.0f, -1.0f, 0.0f, -1.0f, 255, 50, 0);
    Hu3DGLightPosAimSet(spotlightLightId, -2500.0f, 3535.5f, 2500.0f, 0.0f, 0.0f, 0.0f);
    Hu3DGLightInfinitytSet(spotlightLightId);
    shadowLightPosition.x = -2500.0f;
    shadowLightPosition.y = 3535.5f;
    shadowLightPosition.z = 2500.0f;
    shadowLightDirection.x = shadowLightDirection.z = 0.0f;
    shadowLightDirection.y = 1.0f;
    shadowOrigin.x = 0.0f;
    shadowOrigin.y = 0.0f;
    shadowOrigin.z = 0.0f;
    Hu3DShadowCreate(30.0f, 20.0f, 10000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowPosSet(&shadowLightPosition, &shadowLightDirection, &shadowOrigin);
    Hu3DBGColorSet(0, 0, 0);
    InitializeSpotlightScene();
    HuPrcChildCreate(RunMinigameSequence, 0x1000, 0x2000, 0, HuPrcCurrentGet());
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
}

void InitializeSpotlightScene(void)
{
    float spawnX;
    float spawnZ;
    void *assetData;
    SpotlightPlayerWork *playerWorkPointer;
    MovingObjectWork *movingObject;
    HUPROCESS *process;
    ANIMDATA *animationData;
    ANIMDATA *scoreAnimations[4];
    s16 modelHandle;
    s16 sharedModelId;
    s16 modelHandle;
    s32 i;
    s32 j;

    elapsedFrames = 0;
    currentScore = 0;
    HuAudSndGrpSet(0x2C);
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M462, 0), HU_MEMNUM_OVL, HEAP_MODEL);
    modelHandle = Hu3DModelCreate(assetData);
    spotlightModelId = modelHandle;
    Hu3DModelLayerSet(modelHandle, 1);
    Hu3DModelShadowMapSet(modelHandle);
    Hu3DModelAttrSet(modelHandle, HU3D_MOTATTR_LOOP);
    i = 0;
    playerWorkPointer = &playerWork[i];
    playerWorkPointer->playerIndex = i;
    playerWorkPointer->animationTimer = i;
    playerWorkPointer->controllerPort = GWPlayerCfg[i].pad_idx;
    playerWorkPointer->characterModelId = CharModelCreate(characterModelIds[GWPlayerCfg[i].character], 2);
    Hu3DModelLayerSet(playerWorkPointer->mainModelId, 1);
    CharEffectLayerSet(2);
    playerWorkPointer->targetPosition.x = characterModelOffsets[GWPlayerCfg[i].character].x;
    playerWorkPointer->targetPosition.y = characterModelOffsets[GWPlayerCfg[i].character].y;
    playerWorkPointer->targetPosition.z = characterModelOffsets[GWPlayerCfg[i].character].z;
    Hu3DModelPosSet(playerWorkPointer->characterModelId, playerWorkPointer->targetPosition.x, playerWorkPointer->targetPosition.y, playerWorkPointer->targetPosition.z);
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M462, 1), HU_MEMNUM_OVL, HEAP_MODEL);
    playerWorkPointer->mainModelId = Hu3DModelCreate(assetData);
    Hu3DModelLayerSet(playerWorkPointer->mainModelId, 1);
    playerWorkPointer->modelRotation.x = playerWorkPointer->modelRotation.z = 0.0f;
    playerWorkPointer->modelRotation.y = 180.0f;
    playerWorkPointer->position.x = 0.0f;
    playerWorkPointer->position.y = 0.0f;
    playerWorkPointer->position.z = 0.0f;
    playerWorkPointer->modelRotation.y = NormalizeAngleDegrees(playerWorkPointer->modelRotation.y + 180.0f);
    playerWorkPointer->facingAngle = playerWorkPointer->modelRotation.y;
    playerWorkPointer->collisionRadius = 200.0f;
    playerWorkPointer->moveSpeed = 6.75f;
    playerWorkPointer->cameraRotation = 1.0f;
    playerWorkPointer->rotationStep = 1.0f;
    Hu3DModelPosSet(playerWorkPointer->mainModelId, playerWorkPointer->position.x, playerWorkPointer->position.y, playerWorkPointer->position.z);
    Hu3DModelRotSet(playerWorkPointer->mainModelId, playerWorkPointer->modelRotation.x, playerWorkPointer->modelRotation.y, playerWorkPointer->modelRotation.z);
    Hu3DModelShadowSet(playerWorkPointer->mainModelId);
    Hu3DModelHookSet(playerWorkPointer->mainModelId, "itemhook_c", playerWorkPointer->characterModelId);
    for (j = 0; j < 4; j++) {
        playerWorkPointer->motionFrames[j] = CharMotionCreate(characterModelIds[GWPlayerCfg[i].character], characterMotionIds[j]);
    }
    CharMotionSet(characterModelIds[GWPlayerCfg[i].character], playerWorkPointer->motionFrames[0]);
    CharMotionTimeSet(characterModelIds[GWPlayerCfg[i].character], 60.0f);
    CharMotionVoiceOnSet(characterModelIds[GWPlayerCfg[i].character], playerWorkPointer->motionFrames[2], 0);
    for (j = 0; j < 8; j++) {
        assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M462, 2 + j), HU_MEMNUM_OVL, HEAP_MODEL);
        playerWorkPointer->motionIds[j] = Hu3DJointMotion(playerWorkPointer->mainModelId, assetData);
    }
    Hu3DMotionSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[0]);
    Hu3DMotionShiftSpeedSet(playerWorkPointer->mainModelId, 1.0f);
    Hu3DMotionSpeedSet(playerWorkPointer->mainModelId, 1.0f);
    Hu3DModelAttrSet(playerWorkPointer->mainModelId, HU3D_MOTATTR_LOOP);
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M462, 10), HU_MEMNUM_OVL, HEAP_MODEL);
    playerWorkPointer->effectModelId = Hu3DModelCreate(assetData);
    Hu3DModelLayerSet(playerWorkPointer->effectModelId, 1);
    Hu3DModelAttrSet(playerWorkPointer->effectModelId, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(playerWorkPointer->effectModelId, HU3D_MOTATTR_PAUSE);
    CharMotionDataClose(characterModelIds[GWPlayerCfg[i].character]);
    process = HuPrcChildCreate(RunPlayerController, 0x2000, 0x3000, 0, HuPrcCurrentGet());
    process->user_data = playerWorkPointer;
    animationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 2), HU_MEMNUM_OVL));
    modelHandle = Hu3DParticleCreate(animationData, 50);
    Hu3DModelPosSet(modelHandle, 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(modelHandle, 1.0f, 1.0f, 1.0f);
    Hu3DModelLayerSet(modelHandle, 3);
    Hu3DParticleColSet(modelHandle, 0xFF, 0xFF, 0xFF);
    Hu3DParticleScaleSet(modelHandle, 0.0f);
    Hu3DParticleHookSet(modelHandle, UpdateBurstParticleEffect);
    animationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 2), HU_MEMNUM_OVL));
    modelHandle = Hu3DParticleCreate(animationData, 50);
    Hu3DModelPosSet(modelHandle, 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(modelHandle, 1.0f, 1.0f, 1.0f);
    Hu3DModelLayerSet(modelHandle, 3);
    Hu3DParticleColSet(modelHandle, 0xFF, 0xFF, 0xFF);
    Hu3DParticleScaleSet(modelHandle, 0.0f);
    Hu3DParticleHookSet(modelHandle, UpdateSparkParticleEffect);
    animationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 2), HU_MEMNUM_OVL));
    modelHandle = Hu3DParticleCreate(animationData, 50);
    Hu3DModelPosSet(modelHandle, 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(modelHandle, 1.0f, 1.0f, 1.0f);
    Hu3DModelLayerSet(modelHandle, 3);
    Hu3DParticleColSet(modelHandle, 0xFF, 0xFF, 0xFF);
    Hu3DParticleScaleSet(modelHandle, 0.0f);
    Hu3DParticleHookSet(modelHandle, UpdateStarParticleEffect);
    HuDataDirClose(DATADIR_EFFECT);
    for (i = 0; i < 10; i++) {
        movingObject = &playerObjectWork[i];
        movingObject->targetRotation = 50.0f;
        do {
            spawnX = 1380.0f * frandf() - 690.0f;
            spawnZ = 1380.0f * frandf() - 690.0f;
        } while (IsPositionOccupied(spawnX, spawnZ, 50.0f, i, -1) != 0);
        movingObject->transitionProgress = spawnX;
        movingObject->position.x = spawnX;
        movingObject->position.y = 0.0f;
        movingObject->movementSpeed = spawnZ;
        movingObject->position.z = spawnZ;
    }
    for (i = 0; i < 10; i++) {
        movingObject = &playerObjectWork[i];
        if (i == 0) {
            assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M462, 12), HU_MEMNUM_OVL, HEAP_MODEL);
            modelHandle = Hu3DModelCreate(assetData);
            sharedModelId = modelHandle;
        }
        else {
            modelHandle = Hu3DModelLink(sharedModelId);
        }
        movingObject->state = modelHandle;
        movingObject->objectId = i;
        movingObject->targetRotation = 50.0f;
        movingObject->motionId = 0;
        Hu3DModelLayerSet(modelHandle, 1);
        Hu3DModelShadowSet(modelHandle);
        Hu3DModelPosSet(modelHandle, movingObject->position.x, movingObject->position.y, movingObject->position.z);
        movingObject->timer = 0.0f;
        movingObject->scale = 6.75f * movingObjectScaleFactors[0];
        for (j = 0; j < 3; j++) {
            assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M462, 13 + j), HU_MEMNUM_OVL, HEAP_MODEL);
            movingObject->motionIds[j] = Hu3DJointMotion(modelHandle, assetData);
        }
        Hu3DMotionSet(modelHandle, movingObject->motionIds[1]);
        Hu3DModelAttrSet(modelHandle, HU3D_MOTATTR_LOOP);
        assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M462, 11), HU_MEMNUM_OVL, HEAP_MODEL);
        modelHandle = Hu3DModelCreate(assetData);
        modelHandle = modelHandle;
        movingObject->characterModelId = modelHandle;
        Hu3DModelAttrSet(modelHandle, HU3D_ATTR_DISPOFF);
        Hu3DModelLayerSet(modelHandle, 3);
        process = HuPrcChildCreate(RunMovingObjectProcess, 0x1000, 0x3000, 0, HuPrcCurrentGet());
        process->user_data = movingObject;
    }
    for (i = 0; i < 2; i++) {
        modelHandle = Hu3DModelLink(sharedModelId);
        assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M462, 16), HU_MEMNUM_OVL, HEAP_MODEL);
        Hu3DMotionSet(modelHandle, Hu3DJointMotion(modelHandle, assetData));
        Hu3DModelAttrSet(modelHandle, HU3D_MOTATTR_LOOP);
        Hu3DModelShadowSet(modelHandle);
        Hu3DModelPosSet(modelHandle, movingObjectSpawnPositions[i].x, movingObjectSpawnPositions[i].y, movingObjectSpawnPositions[i].z);
        if (i != 0) {
            Hu3DMotionTimeSet(modelHandle, Hu3DMotionMaxTimeGet(modelHandle) / 2.0f);
        }
    }
    modelHandle = Hu3DModelLink(sharedModelId);
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M462, 19), HU_MEMNUM_OVL, HEAP_MODEL);
    Hu3DMotionSet(modelHandle, Hu3DJointMotion(modelHandle, assetData));
    Hu3DModelAttrSet(modelHandle, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelHandle);
    Hu3DModelPosSet(modelHandle, movingObjectSpawnPositions[3].x, movingObjectSpawnPositions[3].y, movingObjectSpawnPositions[3].z);
    Hu3DMotionSpeedSet(modelHandle, 0.7f);
    movingObject = &effectWork;
    modelHandle = Hu3DModelLink(sharedModelId);
    movingObject->state = modelHandle;
    for (j = 0; j < 2; j++) {
        assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M462, 17 + j), HU_MEMNUM_OVL, HEAP_MODEL);
        movingObject->motionIds[j] = Hu3DJointMotion(modelHandle, assetData);
    }
    Hu3DMotionSet(modelHandle, movingObject->motionIds[0]);
    Hu3DModelAttrSet(modelHandle, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelHandle);
    movingObject->position.x = movingObjectSpawnPositions[2].x;
    movingObject->position.y = movingObjectSpawnPositions[2].y;
    movingObject->position.z = movingObjectSpawnPositions[2].z;
    Hu3DModelPosSet(modelHandle, movingObject->position.x, movingObject->position.y, movingObject->position.z);
    process = HuPrcChildCreate(AnimateMovingObjectSideToSide, 0x1000, 0x2000, 0, HuPrcCurrentGet());
    process->user_data = movingObject;
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 44), HU_MEMNUM_OVL, HEAP_MODEL);
    scoreAnimations[0] = HuSprAnimRead(assetData);
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 55), HU_MEMNUM_OVL, HEAP_MODEL);
    scoreAnimations[1] = HuSprAnimRead(assetData);
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 48), HU_MEMNUM_OVL, HEAP_MODEL);
    scoreAnimations[2] = HuSprAnimRead(assetData);
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 49), HU_MEMNUM_OVL, HEAP_MODEL);
    scoreAnimations[3] = HuSprAnimRead(assetData);
    scoreSpriteGroup = HuSprGrpCreate(6);
    modelHandle = HuSprCreate(scoreAnimations[0], 0x4000, 0);
    HuSprGrpMemberSet(scoreSpriteGroup, 0, modelHandle);
    HuSprPosSet(scoreSpriteGroup, 0, 0.0f, 0.0f);
    HuSprTPLvlSet(scoreSpriteGroup, 0, 0.5f);
    HuSprColorSet(scoreSpriteGroup, 0, 0, 0, 0);
    modelHandle = HuSprCreate(scoreAnimations[1], 0x4000, 0);
    HuSprGrpMemberSet(scoreSpriteGroup, 1, modelHandle);
    HuSprPosSet(scoreSpriteGroup, 1, -19.0f, -9.0f);
    modelHandle = HuSprCreate(scoreAnimations[2], 0x4000, 0);
    HuSprGrpMemberSet(scoreSpriteGroup, 2, modelHandle);
    HuSprPosSet(scoreSpriteGroup, 2, 20.0f, -8.0f);
    modelHandle = HuSprCreate(scoreAnimations[2], 0x4000, 0);
    HuSprGrpMemberSet(scoreSpriteGroup, 3, modelHandle);
    HuSprPosSet(scoreSpriteGroup, 3, 2.0f, -8.0f);
    modelHandle = HuSprCreate(scoreAnimations[3], 0x4000, 0);
    HuSprGrpMemberSet(scoreSpriteGroup, 4, modelHandle);
    HuSprPosSet(scoreSpriteGroup, 4, 20.0f, 8.0f);
    modelHandle = HuSprCreate(scoreAnimations[3], 0x4000, 0);
    HuSprGrpMemberSet(scoreSpriteGroup, 5, modelHandle);
    HuSprPosSet(scoreSpriteGroup, 5, 2.0f, 8.0f);
    HuSprGrpPosSet(scoreSpriteGroup, 511.0f, 61.0f);
    SetScoreDigits(currentScore, 0);
    SetScoreDigits(GWMGRecordGet(14), 1);
    HideScoreDigits();
    HuPrcChildCreate(RunPlayerEndSequence, 0x1000, 0x1000, 0, HuPrcCurrentGet());
}

float NormalizeAngleDegrees(float angleDegrees)
{
    while (angleDegrees >= 360.0f || angleDegrees < 0.0f) {
        if (angleDegrees >= 360.0f) {
            angleDegrees -= 360.0f;
        }
        if (angleDegrees < 0.0f) {
            angleDegrees += 360.0f;
        }
    }
    return angleDegrees;
}

float ClampToArenaExtent(float coordinate, float margin)
{
    float arenaExtent;

    if (coordinate > 750.0f - margin) {
        arenaExtent = 750.0f;
    }
    else if (coordinate < -(750.0f - margin)) {
        arenaExtent = 750.0f;
    }
    else {
        arenaExtent = 750.0f;
    }
    return arenaExtent;
}

float ClampActorCoordinate(float coordinate, float margin)
{
    return ClampToArenaExtent(coordinate, margin);
}

void AnimateScoreDigits(void)
{
    float animationPhase;
    float scale;
    s32 i;

    animationPhase = 0.0f;
    while (TRUE) {
        animationPhase += 16.0f;
        scale = 1.0 + 0.2f * sind(animationPhase);
        for (i = 0; i < 3; i++) {
            HuSprScaleSet(scoreSpriteGroup, i + 1, scale, scale);
        }
        HuPrcVSleep();
    }
}

void ShowScoreDigits(void)
{
    s32 i;

    for (i = 0; i < 6; i++) {
        HuSprAttrReset(scoreSpriteGroup, i, HUSPR_ATTR_DISPOFF);
    }
}

void HideScoreDigits(void)
{
    s32 i;

    for (i = 0; i < 6; i++) {
        HuSprAttrSet(scoreSpriteGroup, i, HUSPR_ATTR_DISPOFF);
    }
}

void SetScoreDigits(s32 score, s32 isHighScore)
{
    s32 digitSpriteIndex;

    if (isHighScore != 0) {
        digitSpriteIndex = 2;
    }
    else {
        digitSpriteIndex = 4;
    }
    HuSprBankSet(scoreSpriteGroup, digitSpriteIndex, score % 10);
    HuSprBankSet(scoreSpriteGroup, digitSpriteIndex + 1, score / 10);
    HuSprAttrReset(scoreSpriteGroup, digitSpriteIndex + 1, HUSPR_ATTR_DISPOFF);
}

s32 IsPositionOccupied(float x, float z, float clearance, s32 objectCount, s32 ignoredObjectIndex)
{
    Vec queryPosition;
    Vec actorPosition;
    Vec positionDifference;
    s32 i;
    MovingObjectWork *movingObject;
    SpotlightPlayerWork *playerWorkPointer;

    if (objectCount > 10) {
        objectCount = 10;
    }
    HuSetVecF(&queryPosition, x, 0.0, z);
    for (i = 0; i < objectCount; i++) {
        if (ignoredObjectIndex == i) {
            continue;
        }
        movingObject = &playerObjectWork[i];
        HuSetVecF(&actorPosition, movingObject->position.x, 0.0, movingObject->position.z);
        HuSubVecF(&positionDifference, &queryPosition, &actorPosition);
        if (VECMagXZ(&positionDifference) < movingObject->targetRotation + clearance) {
            break;
        }
    }
    if (i < objectCount) {
        return 1;
    }
    playerWorkPointer = &playerWork[0];
    HuSetVecF(&actorPosition, playerWorkPointer->position.x, 0.0, playerWorkPointer->position.z);
    HuSubVecF(&positionDifference, &queryPosition, &actorPosition);
    if (VECMagXZ(&positionDifference) < playerWorkPointer->collisionRadius + clearance) {
        return 1;
    }
    return 0;
}

s32 ClampMovingObjectPosition(MovingObjectWork *objectWork)
{
    float xBoundary;
    float zBoundary;
    s32 wasClamped;

    wasClamped = 0;
    xBoundary = ClampToArenaExtent(objectWork->position.z, objectWork->targetRotation);
    zBoundary = ClampToArenaExtent(objectWork->position.x, objectWork->targetRotation);
    if (objectWork->position.x > xBoundary - objectWork->targetRotation) {
        wasClamped = 1;
        objectWork->position.x = xBoundary - objectWork->targetRotation;
    }
    if (objectWork->position.x < -(xBoundary - objectWork->targetRotation)) {
        wasClamped = 1;
        objectWork->position.x = -(xBoundary - objectWork->targetRotation);
    }
    if (objectWork->position.z > zBoundary - objectWork->targetRotation) {
        wasClamped = 1;
        objectWork->position.z = zBoundary - objectWork->targetRotation;
    }
    if (objectWork->position.z < -(zBoundary - objectWork->targetRotation)) {
        wasClamped = 1;
        objectWork->position.z = -(zBoundary - objectWork->targetRotation);
    }
    return wasClamped;
}

void UpdateMovingObjectMovement(MovingObjectWork *objectWork)
{
    Vec candidatePosition;
    Vec otherObjectPosition;
    Vec positionDifference;
    float candidateX;
    float candidateZ;
    float movementAngle;
    float avoidanceAngle;
    float combinedCollisionRadius;
    float angleDifference;
    float angleStep;
    float safeX;
    float safeZ;
    float avoidanceDistance;
    float stepSize;
    float remainingDistance;
    MovingObjectWork *movingObject;
    MovingObjectWork *otherMovingObject;
    s32 otherObjectIndex;
    s32 movementSteps;
    s32 i;

    if (IsPositionOccupied(objectWork->position.x, objectWork->position.z, 50.0f, 10, objectWork->objectId) == 0) {
        return;
    }
    movementAngle = NormalizeAngleDegrees(atan2d(objectWork->position.x - objectWork->transitionProgress, objectWork->position.z - objectWork->movementSpeed));
    candidateX = safeX = objectWork->transitionProgress;
    candidateZ = safeZ = objectWork->movementSpeed;
    stepSize = 1.0f;
    for (movementSteps = 1.0f + objectWork->scale / stepSize; movementSteps != 0; movementSteps--) {
        candidateX += stepSize * sind(movementAngle);
        candidateZ += stepSize * cosd(movementAngle);
        HuSetVecF(&candidatePosition, candidateX, objectWork->position.y, candidateZ);
        for (i = 0; i < 10; i++) {
            if (i == objectWork->objectId) {
                continue;
            }
            movingObject = &playerObjectWork[i];
            HuSetVecF(&otherObjectPosition, movingObject->position.x, movingObject->position.y, movingObject->position.z);
            HuSubVecF(&positionDifference, &candidatePosition, &otherObjectPosition);
            if (VECMagXYZ(&positionDifference) < objectWork->targetRotation + movingObject->targetRotation) {
                break;
            }
        }
        if (i < 10) {
            break;
        }
        if (ClampMovingObjectPosition(objectWork)) {
            goto blockEnd;
        }
        safeX = candidateX;
        safeZ = candidateZ;
    }
    movementAngle = NormalizeAngleDegrees(atan2d(objectWork->position.x - objectWork->transitionProgress, objectWork->position.z - objectWork->movementSpeed));
    avoidanceAngle = NormalizeAngleDegrees(atan2d(movingObject->position.x - objectWork->transitionProgress, movingObject->position.z - objectWork->movementSpeed));
    angleDifference = movementAngle - avoidanceAngle;
    if (angleDifference > 0.0f) {
        angleStep = -0.5f;
    }
    else {
        angleStep = 0.5f;
    }
    if (ABS_INV(movementAngle - avoidanceAngle) > 180.0f) {
        angleStep = -angleStep;
    }
    avoidanceAngle = NormalizeAngleDegrees(atan2d(objectWork->transitionProgress - movingObject->position.x, objectWork->movementSpeed - movingObject->position.z));
    avoidanceDistance = 0.8f * objectWork->scale;
    otherObjectIndex = movingObject->objectId;
    combinedCollisionRadius = objectWork->targetRotation + movingObject->targetRotation + 1.0f;
    while (TRUE) {
        candidateX = movingObject->position.x + combinedCollisionRadius * sind(avoidanceAngle);
        candidateZ = movingObject->position.z + combinedCollisionRadius * cosd(avoidanceAngle);
        HuSetVecF(&candidatePosition, candidateX, 0.0, candidateZ);
        for (i = 0; i < 10; i++) {
            if (i == objectWork->objectId || i == otherObjectIndex) {
                continue;
            }
            otherMovingObject = &playerObjectWork[i];
            HuSetVecF(&otherObjectPosition, otherMovingObject->position.x, 0.0, otherMovingObject->position.z);
            HuSubVecF(&positionDifference, &candidatePosition, &otherObjectPosition);
            if (VECMagXZ(&positionDifference) < objectWork->targetRotation + otherMovingObject->targetRotation) {
                break;
            }
        }
        if (i < 10 || ClampMovingObjectPosition(objectWork)) {
            break;
        }
        safeX = candidateX;
        safeZ = candidateZ;
        HuSetVecF(&candidatePosition, objectWork->transitionProgress, 0.0, objectWork->movementSpeed);
        HuSetVecF(&otherObjectPosition, candidateX, 0.0, candidateZ);
        HuSubVecF(&positionDifference, &candidatePosition, &otherObjectPosition);
        if (VECMagXZ(&positionDifference) > avoidanceDistance) {
            break;
        }
        avoidanceAngle += angleStep;
    }
blockEnd:
    objectWork->position.x = safeX;
    objectWork->position.z = safeZ;
}

void UpdateMovingObjectBehavior(MovingObjectWork *objectWork)
{
    Vec targetPosition;
    Vec objectPosition;
    Vec positionDifference;
    s32 i;
    SpotlightPlayerWork *playerWorkPointer;
    MovingObjectWork *nearbyObject;
    float newHeading;
    s32 nearbyObjectCount;
    s32 avoidanceChance;
    s32 nearbyObjectIds[2];

    playerWorkPointer = &playerWork[0];
    objectWork->transitionProgress = objectWork->position.x;
    objectWork->movementSpeed = objectWork->position.z;
    switch (objectWork->animationState) {
        case 0:
            avoidanceChance = 0;
            HuSetVecF(&targetPosition, playerWorkPointer->position.x, 0.0, playerWorkPointer->position.z);
            HuSetVecF(&objectPosition, objectWork->position.x, 0.0, objectWork->position.z);
            HuSubVecF(&positionDifference, &targetPosition, &objectPosition);
            if (VECMagXZ(&positionDifference) < playerWorkPointer->collisionRadius + objectWork->targetRotation + 100.0f) {
                avoidanceChance = 80;
            }
            if (frandmod(100) < avoidanceChance) {
                newHeading = NormalizeAngleDegrees(atan2d(objectWork->position.x - playerWorkPointer->position.x, objectWork->position.z - playerWorkPointer->position.z));
                newHeading += 30.0f * frandf() - 60.0f;
                newHeading = NormalizeAngleDegrees(newHeading);
                objectWork->timer = newHeading;
            }
            else if (ABS_INV(objectWork->position.x) > 550.0f) {
                newHeading = NormalizeAngleDegrees(180.0 + atan2d(objectWork->position.x, 0.0));
                newHeading += 10.0f * frandf() - 20.0f;
                newHeading = NormalizeAngleDegrees(newHeading);
                objectWork->timer = newHeading;
            }
            else if (frandmod(3) != 0) {
                for (i = 0, nearbyObjectCount = 0; i < 10; i++) {
                    if (i == objectWork->objectId) {
                        continue;
                    }
                    nearbyObject = &playerObjectWork[i];
                    HuSetVecF(&targetPosition, nearbyObject->position.x, 0.0, nearbyObject->position.z);
                    HuSetVecF(&objectPosition, objectWork->position.x, 0.0, objectWork->position.z);
                    HuSubVecF(&positionDifference, &targetPosition, &objectPosition);
                    if (VECMagXZ(&positionDifference) < 150.0f) {
                        nearbyObjectIds[nearbyObjectCount++] = objectWork->objectId;
                        if (nearbyObjectCount >= 2) {
                            break;
                        }
                    }
                }
                if (nearbyObjectCount == 1 && ABS_INV(playerWorkPointer->position.x) < 650.0f) {
                    nearbyObject = &playerObjectWork[nearbyObjectIds[0]];
                    newHeading = NormalizeAngleDegrees(atan2d(objectWork->targetX - objectWork->position.x, objectWork->targetZ - objectWork->position.z));
                }
                else {
                    newHeading = NormalizeAngleDegrees(180.0 + atan2d(objectWork->position.x, objectWork->position.z));
                    newHeading += 30.0f * frandf() - 60.0f;
                    newHeading = NormalizeAngleDegrees(newHeading);
                    objectWork->timer = newHeading;
                }
            }
            else {
                newHeading = 360.0f * frandf();
                objectWork->timer = newHeading;
            }
            objectWork->targetX = objectWork->position.x + 100.0 * sind(newHeading);
            objectWork->targetZ = objectWork->position.z + 100.0 * cosd(newHeading);
            objectWork->animationState = 10;
            objectWork->animationTimer = 60;
            break;
        case 10:
            if (objectWork->animationTimer-- == 0) {
                objectWork->animationState = 0;
            }
            if (objectWork->animationTimer < 50) {
                HuSetVecF(&targetPosition, playerWorkPointer->position.x, 0.0, playerWorkPointer->position.z);
                HuSetVecF(&objectPosition, objectWork->position.x, 0.0, objectWork->position.z);
                HuSubVecF(&positionDifference, &targetPosition, &objectPosition);
                if (VECMagXZ(&positionDifference) < playerWorkPointer->collisionRadius + objectWork->targetRotation + 100.0f) {
                    objectWork->animationState = 0;
                }
            }
            HuSetVecF(&targetPosition, objectWork->targetX, 0.0, objectWork->targetZ);
            HuSetVecF(&objectPosition, objectWork->position.x, 0.0, objectWork->position.z);
            HuSubVecF(&positionDifference, &targetPosition, &objectPosition);
            if (VECMagXZ(&positionDifference) > 10.0f) {
                newHeading = NormalizeAngleDegrees(atan2d(objectWork->targetX - objectWork->position.x, objectWork->targetZ - objectWork->position.z));
                objectWork->position.x += objectWork->scale * sind(newHeading);
                objectWork->position.z += objectWork->scale * cosd(newHeading);
                if (ClampMovingObjectPosition(objectWork)) { }
                if (IsPositionOccupied(objectWork->position.x, objectWork->position.z, 50.0f, 10, objectWork->objectId) != 0) {
                    objectWork->position.x = objectWork->transitionProgress;
                    objectWork->position.z = objectWork->movementSpeed;
                    objectWork->animationState = 100;
                    objectWork->animationTimer = 0;
                }
            }
            else {
                objectWork->animationState = 100;
                objectWork->animationTimer = 0;
            }
            break;
        case 100:
            if (objectWork->animationTimer == 0) {
                objectWork->animationState = 0;
            }
            objectWork->animationTimer--;
            break;
    }
}

void AnimateMovingObjectSideToSide(void)
{
    MovingObjectWork *movingObject;
    float homeX;
    float swayAngle;
    s32 i;

    movingObject = HuPrcCurrentGet()->user_data;
    homeX = movingObject->position.x;
    while (TRUE) {
        Hu3DMotionSet(movingObject->state, movingObject->motionIds[0]);
        for (i = 0, swayAngle = 0.0f; i < 60; i++) {
            movingObject->position.x = homeX - 300.0 * sind(swayAngle);
            swayAngle += 1.5f;
            Hu3DModelPosSet(movingObject->state, movingObject->position.x, movingObject->position.y, movingObject->position.z);
            HuPrcVSleep();
        }
        HuPrcSleep(30);
        Hu3DMotionSet(movingObject->state, movingObject->motionIds[1]);
        for (i = 0, swayAngle = 90.0f; i < 60; i++) {
            movingObject->position.x = homeX - 300.0 * sind(swayAngle);
            swayAngle -= 1.5f;
            Hu3DModelPosSet(movingObject->state, movingObject->position.x, movingObject->position.y, movingObject->position.z);
            HuPrcVSleep();
        }
        HuPrcSleep(30);
    }
}

void SpawnMovingObject(MovingObjectWork *objectWork)
{
    float spawnInset;
    float spawnZ;
    float travelX;
    float travelStepX;
    float arenaDepth;
    s32 i;

    do {
        spawnInset = 5.0f;
        if (frandmod(2) != 0) {
            objectWork->position.x = -700.0f + spawnInset;
            objectWork->timer = 90.0f;
            travelX = -700.0f + spawnInset - 500.0f;
            travelStepX = 8.333333f;
        }
        else {
            objectWork->position.x = 700.0f - spawnInset;
            objectWork->timer = 270.0f;
            travelX = 700.0f - spawnInset + 500.0f;
            travelStepX = -8.333333f;
        }
        objectWork->position.y = 0.0f;
        arenaDepth = 550.0f;
        spawnZ = arenaDepth * frandf() * 2.0f - arenaDepth;
        objectWork->position.z = spawnZ;
        objectWork->transitionProgress = objectWork->position.x;
        objectWork->movementSpeed = objectWork->position.z;
    } while (IsPositionOccupied(objectWork->position.x, objectWork->position.z, 50.0f, 10, objectWork->objectId) != 0);
    Hu3DMotionSet(objectWork->state, objectWork->motionIds[2]);
    Hu3DModelRotSet(objectWork->state, 0.0f, objectWork->timer, 0.0f);
    for (i = 0; i < 60; i++) {
        travelX += travelStepX;
        Hu3DModelPosSet(objectWork->state, travelX, 0.0f, spawnZ);
        if (minigameState >= 4) {
            break;
        }
        HuPrcVSleep();
    }
    if (minigameState >= 4) {
        objectWork->position.x = travelX;
        objectWork->position.z = spawnZ;
    }
    Hu3DModelPosSet(objectWork->state, objectWork->position.x, 0.0f, objectWork->position.z);
}

void RunMovingObjectProcess(void)
{
    s32 jumpFrame;
    float initialHeight;
    float jumpAngleStep;
    float jumpAngle;
    float trajectoryZ;
    float direction;
    float trajectoryX;
    MovingObjectWork *movingObject;
    SpotlightPlayerWork *mainPlayer;
    SpotlightPlayerWork *scoringPlayer;
    s32 elapsedSeconds;
    s32 score;
    s32 scoreDigitIndex;
    s32 difficultyTier;
    s32 i;

    movingObject = HuPrcCurrentGet()->user_data;
    movingObject->motionId = 0;
loop1:
    elapsedSeconds = elapsedFrames / REFRESH_RATE;
    difficultyTier = 0;
    if (elapsedSeconds > 10) {
        difficultyTier = 1;
    }
    if (elapsedSeconds > 20) {
        difficultyTier = 2;
    }
    elapsedSeconds = frandmod(100);
    for (i = 0; i < 5; i++) {
        if (scoreProbabilityThresholds[difficultyTier][i] > elapsedSeconds) {
            break;
        }
    }
    if (i > 4) {
        while (TRUE)
            ;
    }
    movingObject->scale = 6.75f * movingObjectScaleFactors[i];
    movingObject->animationState = 0;
    while (TRUE) {
        switch (movingObject->motionId) {
            case 2:
                Hu3DMotionSet(movingObject->state, movingObject->motionIds[1]);
                SpawnMovingObject(movingObject);
                movingObject->motionId = 0;
                if (FALSE) {
                    default:
                        UpdateMovingObjectBehavior(movingObject);
                }
                Hu3DModelPosSet(movingObject->state, movingObject->position.x, movingObject->position.y, movingObject->position.z);
                Hu3DModelRotSet(movingObject->state, 0.0f, movingObject->timer, 0.0f);
                if (minigameState < 4) {
                    HuPrcVSleep();
                    break;
                }
                Hu3DMotionSet(movingObject->state, movingObject->motionIds[0]);
                HuPrcSleep(60);
                mainPlayer = &playerWork[0];
                if (ABS_INV(mainPlayer->position.z - movingObject->position.z) > 250.0f) {
                    if (movingObject->position.x > 0.0f) {
                        direction = 10.0f;
                    }
                    else {
                        direction = -10.0f;
                    }
                }
                else {
                    if (mainPlayer->position.x > movingObject->position.x) {
                        direction = -10.0f;
                    }
                    else {
                        direction = 10.0f;
                    }
                }
                if (direction > 0.0f) {
                    Hu3DModelRotSet(movingObject->state, 0.0f, 90.0f, 0.0f);
                }
                else {
                    Hu3DModelRotSet(movingObject->state, 0.0f, 270.0f, 0.0f);
                }
                Hu3DMotionSet(movingObject->state, movingObject->motionIds[1]);
                while (ABS_INV(movingObject->position.x) < 1500.0f) {
                    movingObject->position.x += direction;
                    Hu3DModelPosSet(movingObject->state, movingObject->position.x, movingObject->position.y, movingObject->position.z);
                    HuPrcVSleep();
                }
                while (TRUE) {
                    HuPrcVSleep();
                }
                break;
            case 3:
                currentScore++;
                if (currentScore > 99) {
                    currentScore = 99;
                }
                score = currentScore;
                scoreDigitIndex = 4;
                HuSprBankSet(scoreSpriteGroup, scoreDigitIndex, score % 10);
                HuSprBankSet(scoreSpriteGroup, scoreDigitIndex + 1, score / 10);
                HuSprAttrReset(scoreSpriteGroup, scoreDigitIndex + 1, HUSPR_ATTR_DISPOFF);
                omVibrate(0, 12, 6, 6);
                HuAudFXPlay(0x45);
                HuAudFXPlay(0x5EE);
                scoringPlayer = &playerWork[0];
                movingObject->rotation = NormalizeAngleDegrees(atan2d(movingObject->position.x - scoringPlayer->position.x, movingObject->position.z - scoringPlayer->position.z));
                Hu3DMotionTimeSet(movingObject->characterModelId, 0.0f);
                Hu3DModelAttrReset(movingObject->characterModelId, HU3D_ATTR_DISPOFF);
                Hu3DModelPosSet(movingObject->characterModelId, scoringPlayer->position.x + scoringPlayer->collisionRadius * sind(movingObject->rotation), movingObject->position.y + 50.0f,
                    scoringPlayer->position.z + scoringPlayer->collisionRadius * cosd(movingObject->rotation));
                i = 0;
                trajectoryX = movingObject->position.x;
                trajectoryZ = movingObject->position.z;
                while (TRUE) {
                    trajectoryX += 30.0 * sind(scoringPlayer->facingAngle);
                    trajectoryZ += 30.0 * cosd(scoringPlayer->facingAngle);
                    if (ABS_INV(trajectoryX) > 1500.0f || trajectoryZ > 1300.0f || trajectoryZ < -2000.0f) {
                        break;
                    }
                    i++;
                }
                initialHeight = movingObject->position.y;
                jumpAngle = 0.0f;
                jumpAngleStep = 180.0f / i;
                jumpFrame = 0;
                for (; i != 0; i--) {
                    movingObject->position.x += 30.0 * sind(movingObject->rotation);
                    movingObject->position.y = 400.0 * sind(jumpAngle);
                    movingObject->position.z += 30.0 * cosd(movingObject->rotation);
                    jumpAngle += jumpAngleStep;
                    Hu3DModelPosSet(movingObject->state, movingObject->position.x, movingObject->position.y, movingObject->position.z);
                    HuPrcVSleep();
                }
                Hu3DModelPosSet(movingObject->state, movingObject->position.x, -1000.0f, movingObject->position.z);
                movingObject->motionId = 2;
                Hu3DModelAttrSet(movingObject->characterModelId, HU3D_ATTR_DISPOFF);
                goto loop1;
        }
    }
}

void UpdateBurstParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    SpotlightPlayerWork *playerWorkPointer;
    float particleAngle;
    float particleSpeed;
    float spawnRadius;
    s32 alpha;
    s16 i;
    s16 j;
    s16 k;

    if (particle->count == 0) {
        particleData = particle->data;
        for (k = 0; k < particle->maxCnt; k++, particleData++) {
            particleData->accel.x = particleData->scale = 0.0f;
        }
    }
    particleData = particle->data;
    for (i = 0; i < 1; i++) {
        playerWorkPointer = &playerWork[i];
        if (!(playerWorkPointer->particleEffectFlags & 1)) {
            continue;
        }
        for (j = 0, particleAngle = 0.0f; j < 16; j++, particleAngle += 22.5f) {
            for (k = 0; k < particle->maxCnt; k++, particleData++) {
                if (particleData->accel.x == 0.0f) {
                    break;
                }
            }
            if (k == particle->maxCnt) {
                break;
            }
            particleSpeed = particleAngle;
            spawnRadius = 125.0f + 70.0f * frandf();
            particleData->pos.x = playerWorkPointer->particleOriginX + spawnRadius * sind(particleSpeed);
            particleData->pos.y = playerWorkPointer->movementSpeed + 50.0f * frandf();
            particleData->pos.z = playerWorkPointer->particleOriginZ + spawnRadius * cosd(particleSpeed);
            particleData->vel.x = 5.0 * sind(particleSpeed);
            particleData->vel.y = 9.0f + 2.0f * frandf();
            particleData->vel.z = 5.0 * cosd(particleSpeed);
            particleData->accel.x = 30.0f;
            particleData->scale = 80.0f + 20.0f * frandf();
            particleData->accel.y = 15.0f;
            particleData->color.a = 0xC8;
            particleData->accel.z = 17.0f;
        }
        playerWorkPointer->particleEffectFlags &= ~1;
    }
    particleData = particle->data;
    for (k = 0; k < particle->maxCnt; k++, particleData++) {
        if (particleData->accel.x == 0.0f) {
            continue;
        }
        VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
        particleData->vel.x *= 0.95f;
        particleData->vel.z *= 0.95f;
        if (particleData->vel.y > 0.1f) {
            particleData->vel.y -= 0.2f;
        }
        if (particleData->vel.y < 0.0f) {
            particleData->vel.y = 0.0f;
        }
        particleData->scale += particleData->accel.y;
        if (particleData->accel.y > 2.0f) {
            particleData->accel.y -= 1.5f;
        }
        alpha = particleData->color.a;
        alpha -= particleData->accel.z;
        if (particleData->accel.z > 3.0f) {
            particleData->accel.z -= 1.0f;
        }
        if (alpha < 0) {
            alpha = 0;
            particleData->accel.x = 0.0f;
        }
        particleData->color.a = alpha;
        if (particleData->accel.x == 0.0f) {
            particleData->scale = 0.0f;
        }
    }
}

void UpdateSparkParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    SpotlightPlayerWork *playerWorkPointer;
    float spawnOffsetX;
    float spawnOffsetZ;
    s32 alpha;
    s16 i;
    s16 j;
    s16 k;

    if (particle->count == 0) {
        particleData = particle->data;
        for (k = 0; k < particle->maxCnt; k++, particleData++) {
            particleData->accel.x = particleData->scale = 0.0f;
            particleData->speedDecay = -1.0f;
        }
    }
    particleData = particle->data;
    for (i = 0; i < 1; i++) {
        playerWorkPointer = &playerWork[i];
        if (!(playerWorkPointer->particleEffectFlags & 2)) {
            continue;
        }
        for (j = 0; j < 10; j++) {
            for (k = 0; k < particle->maxCnt; k++, particleData++) {
                if (particleData->accel.x == 0.0f) {
                    break;
                }
            }
            if (k == particle->maxCnt) {
                break;
            }
            spawnOffsetX = 400.0f * frandf() - 200.0f;
            spawnOffsetZ = 400.0f * frandf() - 200.0f;
            particleData->pos.x = playerWorkPointer->particleOriginX + spawnOffsetX;
            particleData->pos.y = playerWorkPointer->movementSpeed + 50.0f;
            particleData->pos.z = playerWorkPointer->particleOriginZ + spawnOffsetZ;
            particleData->vel.x = 0.0f;
            particleData->vel.y = 7.0f;
            particleData->vel.z = 0.0f;
            particleData->speedDecay = frandmod(10);
            particleData->colorIdx = 0.0f;
            particleData->accel.x = 100.0f;
            particleData->scale = 0.0f;
            particleData->accel.y = 7.0f;
            particleData->color.a = 0xFF;
            particleData->accel.z = 10.0f;
        }
        playerWorkPointer->particleEffectFlags &= ~2;
    }
    particleData = particle->data;
    for (k = 0; k < particle->maxCnt; k++, particleData++) {
        if (particleData->speedDecay > 0.0f) {
            particleData->speedDecay -= 1.0f;
        }
        if (particleData->speedDecay == 0.0f) {
            particleData->colorIdx = 1.0f;
            particleData->scale = 100.0f + 100.0f * frandf();
            particleData->speedDecay = -1.0f;
        }
        if (particleData->accel.x == 0.0f || particleData->colorIdx == 0.0f) {
            continue;
        }
        VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
        if (particleData->vel.y > 1.0f) {
            particleData->vel.y -= 0.3f;
        }
        particleData->scale += particleData->accel.y;
        if (particleData->accel.y > 2.0f) {
            particleData->accel.y -= 0.4f;
        }
        alpha = particleData->color.a;
        alpha -= particleData->accel.z;
        if (particleData->accel.z > 3.0f) {
            particleData->accel.z -= 1.0f;
        }
        if (alpha < 0) {
            alpha = 0;
            particleData->accel.x = 0.0f;
        }
        particleData->color.a = alpha;
        if (particleData->accel.x == 0.0f) {
            particleData->scale = 0.0f;
        }
    }
}

void UpdateStarParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    SpotlightPlayerWork *playerWorkPointer;
    float spawnOffsetX;
    float spawnOffsetZ;
    s32 alpha;
    s16 i;
    s16 j;
    s16 k;

    if (particle->count == 0) {
        particleData = particle->data;
        for (k = 0; k < particle->maxCnt; k++, particleData++) {
            particleData->accel.x = particleData->scale = 0.0f;
            particleData->speedDecay = -1.0f;
        }
    }
    particleData = particle->data;
    for (i = 0; i < 1; i++) {
        playerWorkPointer = &playerWork[i];
        if (!(playerWorkPointer->particleEffectFlags & 4)) {
            continue;
        }
        for (j = 0; j < 10; j++) {
            for (k = 0; k < particle->maxCnt; k++, particleData++) {
                if (particleData->accel.x == 0.0f) {
                    break;
                }
            }
            if (k == particle->maxCnt) {
                break;
            }
            spawnOffsetX = 400.0f * frandf() - 200.0f;
            spawnOffsetZ = 400.0f * frandf() - 200.0f;
            particleData->pos.x = playerWorkPointer->particleOriginX + spawnOffsetX;
            particleData->pos.y = playerWorkPointer->movementSpeed + 50.0f;
            particleData->pos.z = playerWorkPointer->particleOriginZ + spawnOffsetZ;
            particleData->vel.x = 0.0f;
            particleData->vel.y = 7.0f;
            particleData->vel.z = 0.0f;
            particleData->speedDecay = frandmod(10);
            particleData->colorIdx = 0.0f;
            particleData->accel.x = 100.0f;
            particleData->scale = 0.0f;
            particleData->accel.y = 7.0f;
            particleData->color.a = 0xFF;
            particleData->accel.z = 10.0f;
        }
        playerWorkPointer->particleEffectFlags &= ~4;
    }
    particleData = particle->data;
    for (k = 0; k < particle->maxCnt; k++, particleData++) {
        if (particleData->speedDecay > 0.0f) {
            particleData->speedDecay -= 1.0f;
        }
        if (particleData->speedDecay == 0.0f) {
            particleData->colorIdx = 1.0f;
            particleData->scale = 100.0f + 100.0f * frandf();
            particleData->speedDecay = -1.0f;
        }
        if (particleData->accel.x == 0.0f || particleData->colorIdx == 0.0f) {
            continue;
        }
        VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
        if (particleData->vel.y > 1.0f) {
            particleData->vel.y -= 0.3f;
        }
        particleData->scale += particleData->accel.y;
        if (particleData->accel.y > 2.0f) {
            particleData->accel.y -= 0.4f;
        }
        alpha = particleData->color.a;
        alpha -= particleData->accel.z;
        if (particleData->accel.z > 3.0f) {
            particleData->accel.z -= 1.0f;
        }
        if (alpha < 0) {
            alpha = 0;
            particleData->accel.x = 0.0f;
        }
        particleData->color.a = alpha;
        if (particleData->accel.x == 0.0f) {
            particleData->scale = 0.0f;
        }
    }
}

s8 GetPlayerStickAxis(s32 playerIndex, s32 useVerticalAxis)
{
    SpotlightPlayerWork *playerWorkPointer;
    s8 stickAxis;

    playerWorkPointer = &playerWork[playerIndex];
    (useVerticalAxis != 0) ? (stickAxis = HuPadStkY[playerWorkPointer->controllerPort]) : (stickAxis = HuPadStkX[playerWorkPointer->controllerPort]);
    return stickAxis;
}

u8 GetPlayerTriggerInput(s32 playerIndex, s32 useRightTrigger)
{
    SpotlightPlayerWork *playerWorkPointer;
    s8 triggerValue;

    playerWorkPointer = &playerWork[playerIndex];
    (useRightTrigger != 0) ? (triggerValue = HuPadTrigR[playerWorkPointer->controllerPort]) : (triggerValue = HuPadTrigL[playerWorkPointer->controllerPort]);
    return triggerValue;
}

u16 GetPlayerButtonsPressed(s32 playerIndex)
{
    SpotlightPlayerWork *playerWorkPointer;
    u16 pressedButtons;

    playerWorkPointer = &playerWork[playerIndex];
    pressedButtons = HuPadBtnDown[playerWorkPointer->controllerPort];
    return pressedButtons;
}

u16 GetPlayerButtonsHeld(s32 playerIndex)
{
    SpotlightPlayerWork *playerWorkPointer;
    u16 heldButtons;

    playerWorkPointer = &playerWork[playerIndex];
    heldButtons = HuPadBtn[playerWorkPointer->controllerPort];
    return heldButtons;
}

s32 FindCollidingMovingObject(SpotlightPlayerWork *playerWork)
{
    Vec playerPosition;
    Vec objectPosition;
    Vec positionDifference;
    MovingObjectWork *movingObject;
    s32 collidingObjectIndex;
    s32 i;

    collidingObjectIndex = -1;
    HuSetVecF(&playerPosition, playerWork->position.x, playerWork->position.y, playerWork->position.z);
    for (i = 0; i < 10; i++) {
        movingObject = &playerObjectWork[i];
        HuSetVecF(&objectPosition, movingObject->position.x, movingObject->position.y, movingObject->position.z);
        HuSubVecF(&positionDifference, &playerPosition, &objectPosition);
        if (VECMagXYZ(&positionDifference) < playerWork->collisionRadius + movingObject->targetRotation) {
            movingObject->motionId = 3;
            collidingObjectIndex = i;
        }
    }
    return collidingObjectIndex;
}

s32 ClampPlayerPosition(SpotlightPlayerWork *playerWork)
{
    float xBoundary;
    float zBoundary;
    s32 collisionResult; // ! - uninitialized
#ifdef NON_MATCHING
    collisionResult = 1952805376;
#endif

    xBoundary = ClampToArenaExtent(playerWork->position.z, playerWork->collisionRadius);
    zBoundary = ClampToArenaExtent(playerWork->position.x, playerWork->collisionRadius);
    if (playerWork->position.x > xBoundary - playerWork->collisionRadius) {
        playerWork->position.x = xBoundary - playerWork->collisionRadius;
    }
    if (playerWork->position.x < -(xBoundary - playerWork->collisionRadius)) {
        playerWork->position.x = -(xBoundary - playerWork->collisionRadius);
    }
    if (playerWork->position.z > zBoundary - playerWork->collisionRadius) {
        playerWork->position.z = zBoundary - playerWork->collisionRadius;
    }
    if (playerWork->position.z < -(zBoundary - playerWork->collisionRadius)) {
        playerWork->position.z = -(zBoundary - playerWork->collisionRadius);
    }
    return collisionResult;
}

float ApproachAngleDegrees(float currentAngle, float targetAngle, float stepFraction)
{
    float interpolatedAngle;
    float angleDifference;

    angleDifference = fmod(targetAngle - currentAngle, 360.0);
    if (0.0f > angleDifference) {
        angleDifference += 360.0f;
    }
    if (180.0f < angleDifference) {
        angleDifference -= 360.0f;
    }
    interpolatedAngle = fmod(currentAngle + angleDifference * stepFraction, 360.0);
    if (0.0f > interpolatedAngle) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

void MovePlayerCharacterModel(void)
{
    SpotlightPlayerWork *playerWorkPointer;
    s32 i;

    playerWorkPointer = HuPrcCurrentGet()->user_data;
    for (i = 0; i < 20; i++) {
        playerWorkPointer->targetPosition.z -= 4.0f;
        Hu3DModelPosSet(playerWorkPointer->characterModelId, playerWorkPointer->targetPosition.x, playerWorkPointer->targetPosition.y, playerWorkPointer->targetPosition.z);
        HuPrcVSleep();
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

s32 IsPlayerInScoringState(SpotlightPlayerWork *playerWork)
{
    return (playerWork->playerState == 4 || playerWork->playerState == 5 || playerWork->playerState == 6);
}

void RunPlayerController(void)
{
    float fadeOpacity;
    float fallOffset;
    float turnStep;
    SpotlightPlayerWork *playerWorkPointer;
    s16 walkingSoundId;
    s32 normalFootstepFrame;
    s32 runningFootstepFrame;
    HUPROCESS *process;
    s32 blinkFrame;
    s32 blinkState;
    s32 fadeFrame;
    s32 collisionResult;

    playerWorkPointer = HuPrcCurrentGet()->user_data;
    playerWorkPointer->playerState = 0;
    playerWorkPointer->footstepFrame = 0;
    playerWorkPointer->particleEffectFlags = 0;
    while (minigameState == 0) {
        HuPrcVSleep();
    }
    minigameState = 2;
    while (minigameState == 2) {
        HuPrcVSleep();
    }
    walkingSoundId = -1;
    while (TRUE) {
        playerWorkPointer->savedPositionX = playerWorkPointer->position.x;
        playerWorkPointer->savedPositionZ = playerWorkPointer->position.z;
        if (playerWorkPointer->motionFrames[4] != 0) {
            break;
        }
        switch (playerWorkPointer->playerState) {
            case 8:
                if (playerWorkPointer->animationCountdown != 0) {
                    playerWorkPointer->animationCountdown--;
                }
                else {
                    Hu3DMotionShiftSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
                    playerWorkPointer->playerState = 0;
                }
                break;
            case 4:
                omVibrate(playerWorkPointer->playerIndex, 12, 4, 2);
                Hu3DMotionShiftSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[4], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
                playerWorkPointer->animationCountdown = 90;
                playerWorkPointer->effectDelay = 30;
                playerWorkPointer->playerState = 5;
                break;
            case 5:
                if (playerWorkPointer->effectDelay-- == 0) {
                    playerWorkPointer->particleOriginX = playerWorkPointer->position.x;
                    playerWorkPointer->movementSpeed = playerWorkPointer->position.y;
                    playerWorkPointer->particleOriginZ = playerWorkPointer->position.z;
                    playerWorkPointer->particleEffectFlags |= 1;
                }
                if (playerWorkPointer->animationCountdown != 0) {
                    playerWorkPointer->animationCountdown--;
                }
                else {
                    Hu3DMotionSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[5]);
                    Hu3DModelAttrReset(playerWorkPointer->mainModelId, HU3D_MOTATTR_LOOP);
                    playerWorkPointer->playerState = 6;
                }
                break;
            case 6:
                if (Hu3DMotionEndCheck(playerWorkPointer->mainModelId) == TRUE) {
                    Hu3DMotionSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[0]);
                    playerWorkPointer->playerState = 0;
                }
                break;
            default:
                if (GetPlayerStickAxis(playerWorkPointer->playerIndex, 0) > 20) {
                    playerWorkPointer->facingAngle -= playerWorkPointer->rotationStep;
                    if (playerWorkPointer->facingAngle < 0.0f) {
                        playerWorkPointer->facingAngle += 360.0f;
                    }
                    playerWorkPointer->modelRotation.y = playerWorkPointer->facingAngle;
                }
                if (GetPlayerStickAxis(playerWorkPointer->playerIndex, 0) < -20) {
                    playerWorkPointer->facingAngle += playerWorkPointer->rotationStep;
                    if (playerWorkPointer->facingAngle > 360.0f) {
                        playerWorkPointer->facingAngle -= 360.0f;
                    }
                    playerWorkPointer->modelRotation.y = playerWorkPointer->facingAngle;
                }
                if (GetPlayerButtonsHeld(playerWorkPointer->playerIndex) & PAD_BUTTON_A) {
                    playerWorkPointer->position.x += playerWorkPointer->moveSpeed * sind(playerWorkPointer->facingAngle);
                    playerWorkPointer->position.z += playerWorkPointer->moveSpeed * cosd(playerWorkPointer->facingAngle);
                }
                if (GetPlayerButtonsHeld(playerWorkPointer->playerIndex) & PAD_BUTTON_B) {
                    playerWorkPointer->position.x -= playerWorkPointer->moveSpeed * sind(playerWorkPointer->facingAngle);
                    playerWorkPointer->position.z -= playerWorkPointer->moveSpeed * cosd(playerWorkPointer->facingAngle);
                }
                if (GetPlayerButtonsHeld(playerWorkPointer->playerIndex) & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                    if (playerWorkPointer->movementMode != 1) {
                        if (playerWorkPointer->playerState != 2) {
                            playerWorkPointer->playerState = 2;
                            if (GetPlayerButtonsHeld(playerWorkPointer->playerIndex) & PAD_BUTTON_A) {
                                Hu3DMotionShiftSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[1], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                                Hu3DMotionShiftSpeedSet(playerWorkPointer->mainModelId, 2.0f);
                            }
                            else {
                                Hu3DMotionShiftSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[1], 0.0f, 8.0f, HU3D_MOTATTR_LOOP | HU3D_MOTATTR_REV);
                                Hu3DMotionShiftSpeedSet(playerWorkPointer->mainModelId, 2.0f);
                            }
                            if (walkingSoundId != -1) {
                                HuAudFXStop(walkingSoundId);
                            }
                        }
                    }
                    else {
                        if (playerWorkPointer->playerState != 3) {
                            playerWorkPointer->playerState = 3;
                            playerWorkPointer->footstepFrame = 0;
                            if (GetPlayerButtonsHeld(playerWorkPointer->playerIndex) & PAD_BUTTON_A) {
                                Hu3DMotionShiftSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[2], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                                Hu3DMotionShiftSpeedSet(playerWorkPointer->mainModelId, 2.0f);
                            }
                            else {
                                Hu3DMotionShiftSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[2], 0.0f, 8.0f, HU3D_MOTATTR_LOOP | HU3D_MOTATTR_REV);
                                Hu3DMotionShiftSpeedSet(playerWorkPointer->mainModelId, 2.0f);
                            }
                            if (walkingSoundId != -1) {
                                HuAudFXStop(walkingSoundId);
                            }
                        }
                    }
                    if (playerWorkPointer->movementMode != 1) {
                        normalFootstepFrame = 8;
                        runningFootstepFrame = 32;
                    }
                    else {
                        normalFootstepFrame = 7;
                        runningFootstepFrame = 22;
                    }
                    if (playerWorkPointer->footstepFrame == normalFootstepFrame) {
                        HuAudFXPlay(0x5E0);
                    }
                    if (playerWorkPointer->footstepFrame == runningFootstepFrame) {
                        HuAudFXPlay(0x5E0);
                    }
                    playerWorkPointer->footstepFrame++;
                    if (playerWorkPointer->movementMode != 1) {
                        if (playerWorkPointer->footstepFrame >= 50) {
                            playerWorkPointer->footstepFrame -= 50;
                        }
                    }
                    else {
                        if (playerWorkPointer->footstepFrame >= 30) {
                            playerWorkPointer->footstepFrame -= 30;
                        }
                    }
                }
                else {
                    if (ABS_INV(GetPlayerStickAxis(playerWorkPointer->playerIndex, 0)) <= 20) {
                        if (playerWorkPointer->playerState != 0) {
                            playerWorkPointer->playerState = 0;
                            Hu3DMotionShiftSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[0], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                            if (walkingSoundId != -1) {
                                HuAudFXStop(walkingSoundId);
                            }
                        }
                    }
                    else {
                        if (playerWorkPointer->playerState != 1) {
                            playerWorkPointer->playerState = 1;
                            Hu3DMotionShiftSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[3], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                            Hu3DMotionShiftSpeedSet(playerWorkPointer->mainModelId, 1.5f);
                            walkingSoundId = HuAudFXPlay(0x5EB);
                        }
                    }
                }
                if (FindCollidingMovingObject(playerWorkPointer) != -1) { }
                if ((collisionResult = ClampPlayerPosition(playerWorkPointer)) != -1) { }
                break;
        }
        Hu3DModelPosSet(playerWorkPointer->mainModelId, playerWorkPointer->position.x, playerWorkPointer->position.y, playerWorkPointer->position.z);
        Hu3DModelRotSet(playerWorkPointer->mainModelId, playerWorkPointer->modelRotation.x, playerWorkPointer->modelRotation.y, playerWorkPointer->modelRotation.z);
        HuPrcVSleep();
    }
    if (walkingSoundId != -1) {
        HuAudFXStop(walkingSoundId);
    }
    if (currentScore == 0) {
        HuAudFXPlay(0x5EA);
        Hu3DMotionShiftSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[0], 0.0f, 2.0f, HU3D_MOTATTR_LOOP);
        HuPrcSleep(90);
        Hu3DModelAttrReset(playerWorkPointer->effectModelId, HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(playerWorkPointer->effectModelId, HU3D_MOTATTR_PAUSE);
        Hu3DModelPosSet(playerWorkPointer->effectModelId, playerWorkPointer->position.x, playerWorkPointer->position.y, playerWorkPointer->position.z);
        Hu3DMotionShiftSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[7], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
        HuAudFXPlay(0x5E3);
        HuPrcSleep(15);
        HuPrcSleep(30);
        playerWorkPointer->particleOriginX = playerWorkPointer->position.x;
        playerWorkPointer->movementSpeed = playerWorkPointer->position.y;
        playerWorkPointer->particleOriginZ = playerWorkPointer->position.z;
        playerWorkPointer->particleEffectFlags |= 2;
        while (!Hu3DMotionEndCheck(playerWorkPointer->mainModelId)) {
            HuPrcVSleep();
        }
        CharMotionShiftSet(characterModelIds[GWPlayerCfg[playerWorkPointer->playerIndex].character], playerWorkPointer->motionFrames[3], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        Hu3DModelHookReset(playerWorkPointer->mainModelId);
        Hu3DModelShadowSet(playerWorkPointer->characterModelId);
        fallOffset = 50.0f;
        Hu3DModelPosSet(playerWorkPointer->characterModelId, playerWorkPointer->position.x, playerWorkPointer->position.y + fallOffset, playerWorkPointer->position.z);
        HuPrcSleep(60);
        for (blinkFrame = 0, blinkState = 0; blinkFrame < 90; blinkFrame++) {
            if (blinkState & 1) {
                Hu3DModelAttrReset(playerWorkPointer->mainModelId, HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(playerWorkPointer->effectModelId, HU3D_ATTR_DISPOFF);
            }
            else {
                Hu3DModelAttrSet(playerWorkPointer->mainModelId, HU3D_ATTR_DISPOFF);
                Hu3DModelAttrSet(playerWorkPointer->effectModelId, HU3D_ATTR_DISPOFF);
            }
            blinkState++;
            HuPrcVSleep();
        }
        Hu3DModelAttrSet(playerWorkPointer->effectModelId, HU3D_ATTR_DISPOFF);
        for (fadeFrame = 0, fadeOpacity = 1.0f; fadeFrame < 30; fadeFrame++) {
            fadeOpacity -= 0.033333335f;
            Hu3DModelTPLvlSet(playerWorkPointer->mainModelId, fadeOpacity);
            if (fallOffset > 0.0f) {
                fallOffset -= 5.0f;
                Hu3DModelPosSet(playerWorkPointer->characterModelId, playerWorkPointer->position.x, playerWorkPointer->position.y + fallOffset, playerWorkPointer->position.z);
            }
            HuPrcVSleep();
        }
        Hu3DModelAttrSet(playerWorkPointer->mainModelId, HU3D_ATTR_DISPOFF);
        HuAudSStreamPlay(4);
        HuPrcSleep(REFRESH_RATE*3);
    }
    else {
        turnStep = playerWorkPointer->rotationStep;
        Hu3DMotionShiftSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[3], 0.0f, 2.0f, HU3D_MOTATTR_LOOP);
        if (NormalizeAngleDegrees(playerWorkPointer->modelRotation.y) != 0.0f) {
            fadeFrame = (360.0f - NormalizeAngleDegrees(playerWorkPointer->modelRotation.y)) / turnStep;
            if (playerWorkPointer->modelRotation.y < 180.0f) {
                fadeFrame = playerWorkPointer->modelRotation.y / turnStep;
                turnStep = -turnStep;
            }
            for (; fadeFrame != 0; fadeFrame--) {
                playerWorkPointer->modelRotation.y += turnStep;
                Hu3DModelRotSet(playerWorkPointer->mainModelId, playerWorkPointer->modelRotation.x, playerWorkPointer->modelRotation.y, playerWorkPointer->modelRotation.z);
                HuPrcVSleep();
            }
            Hu3DModelRotSet(playerWorkPointer->mainModelId, playerWorkPointer->modelRotation.x, 0.0f, playerWorkPointer->modelRotation.z);
        }
        Hu3DMotionShiftSet(playerWorkPointer->mainModelId, playerWorkPointer->motionIds[0], 0.0f, 2.0f, HU3D_MOTATTR_LOOP);
        while (minigameState < 5) {
            HuPrcVSleep();
        }
        HuAudSStreamPlay(1);
        process = HuPrcChildCreate(MovePlayerCharacterModel, 0x2000, 0x1000, 0, HuPrcCurrentGet());
        process->user_data = playerWorkPointer;
        PlayerFXPlay(playerWorkPointer->playerIndex, 0x122);
        CharMotionShiftSet(characterModelIds[GWPlayerCfg[playerWorkPointer->playerIndex].character], playerWorkPointer->motionFrames[2], 0.0f, 2.0f, HU3D_MOTATTR_NONE);
        playerWorkPointer->playerState = 10;
        HuPrcSleep(REFRESH_RATE*3);
    }
    minigameState = 6;
    while (TRUE) {
        HuPrcVSleep();
    }
}

void NoOpProcess(void) { }

void RunPlayerEndSequence(void)
{
    while (TRUE) {
        if (omSysExitReq == 1) {
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
            HuAudFadeOut(1000);
            HuPrcSleep(60);
            MGSeqKillAll();
            omOvlReturnEx(1, 1);
            while (TRUE) {
                HuPrcVSleep();
            }
        }
        HuPrcVSleep();
    }
}

void RunMinigameSequence(void)
{
    s16 sequenceId;
    s16 sequenceTimer;
    s16 countdownTimer;
    s32 countdownSeconds;
    s32 i;

    minigameState = 0;
    minigameState = 1;
    while (minigameState == 1) {
        HuPrcVSleep();
    }
    countdownSeconds = HuAudSeqPlay(0x46);
    sequenceId = MGSeqStartCreate();
    while (MGSeqStatGet(sequenceId) != 0) {
        HuPrcVSleep();
    }
    ShowScoreDigits();
    sequenceTimer = (REFRESH_RATE*31)-1;
    countdownTimer = MGSeqTimerCreate(sequenceTimer / REFRESH_RATE);
    minigameState = 3;
    while (TRUE) {
        if (sequenceTimer < REFRESH_RATE-1) {
            break;
        }
        MGSeqParamSet(countdownTimer, 1, sequenceTimer-- / REFRESH_RATE);
        elapsedFrames++;
        HuPrcVSleep();
    }
    for (i = 0; i < 1; i++) {
        playerWork[i].endingMotionState = 1;
    }
    minigameState = 4;
    HuAudSeqFadeOut(countdownSeconds, 6);
    MGSeqParamSet(countdownTimer, 2, -1);
    sequenceId = MGSeqFinishCreate();
    while (MGSeqStatGet(sequenceId) != 0) {
        HuPrcVSleep();
    }
    mgRecordExtra = currentScore;
    if (GWMGRecordGet(14) < currentScore) {
        GWMGRecordSet(14, currentScore);
        sequenceId = MGSeqRecordCreate(currentScore);
        HuPrcChildCreate(AnimateScoreDigits, 0x1000, 0x1000, 0, HuPrcCurrentGet());
        SetScoreDigits(currentScore, 1);
        while (MGSeqStatGet(sequenceId) != 0) {
            HuPrcVSleep();
        }
        minigameState = 5;
    }
    else {
        minigameState = 5;
    }
    while (minigameState < 6) {
        HuPrcVSleep();
    }
    HuPrcSleep(REFRESH_RATE/2);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    HuPrcSleep(60);
    HuAudFadeOut(1);
    MGSeqKillAll();
    omOvlReturnEx(1, 1);
    while (TRUE) {
        HuPrcVSleep();
    }
}
