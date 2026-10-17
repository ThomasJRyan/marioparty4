#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "ext_math.h"
#include "version.h"

typedef struct {
    /* 0x00 */ OMOBJ *rootObject;
    /* 0x04 */ char reservedObjectData[0xB4];
} ObjectManagerState; // Size 0xB8

typedef struct {
    /* 0x00 */ s16 modelId;
    /* 0x02 */ s16 motionIds[5];
} MascotState; // Size 0xC

typedef struct {
    /* 0x00 */ s16 characterModel;
    /* 0x02 */ s16 shadowModel;
    /* 0x04 */ s16 effectModel;
    /* 0x06 */ s16 modelMotionIds[8];
    /* 0x16 */ s16 additionalMotionId;
    /* 0x18 */ s16 secondaryMotionId;
    /* 0x1A */ s16 tertiaryMotionId;
    /* 0x1C */ s16 characterMotionIds[4];
    /* 0x24 */ s16 resultState;
    /* 0x26 */ char reservedResultData[2];
    /* 0x28 */ s32 playerIndex;
    /* 0x2C */ s32 initializationIndex;
    /* 0x30 */ s32 controllerIndex;
    /* 0x34 */ s32 effectState;
    /* 0x38 */ s32 effectTimer;
    /* 0x3C */ s32 motionAttributes;
    /* 0x40 */ s32 motionFlags;
    /* 0x44 */ s32 motionFrame;
    /* 0x48 */ s32 motionSpeed;
    /* 0x4C */ s32 characterAnimationFlags;
    /* 0x50 */ s32 animationFrame;
    /* 0x54 */ float positionX;
    /* 0x58 */ float positionY;
    /* 0x5C */ float positionZ;
    /* 0x60 */ float rotationX;
    /* 0x64 */ float rotationY;
    /* 0x68 */ float rotationZ;
    /* 0x6C */ float turnAngle;
    /* 0x70 */ float turnSpeed;
    /* 0x74 */ float previousPositionX;
    /* 0x78 */ float previousPositionY;
    /* 0x7C */ float previousPositionZ;
    /* 0x80 */ float startRotationX;
    /* 0x84 */ float startRotationY;
    /* 0x88 */ float startRotationZ;
    /* 0x8C */ float collisionRadius;
    /* 0x90 */ float movementSpeedLimit;
    /* 0x94 */ float movementAcceleration;
    /* 0x98 */ float rotationSpeed;
    /* 0x9C */ float movementSpeed;
    /* 0xA0 */ s8 playerState;
    /* 0xA1 */ s8 teamId;
    /* 0xA2 */ s8 difficulty;
    /* 0xA3 */ s8 verticalInput;
    /* 0xA4 */ u16 buttonState;
    /* 0xA6 */ u16 buttonFlags;
    /* 0xA8 */ s32 playerMotionState;
    /* 0xAC */ s32 playerActionState;
    /* 0xB0 */ s32 playerEventFlags;
    /* 0xB4 */ s32 collisionFlags;
    /* 0xB8 */ s32 effectFlags;
    /* 0xBC */ s32 targetPlayerIndex;
    /* 0xC0 */ s32 transitionState;
    /* 0xC4 */ float transitionTimer;
    /* 0xC8 */ char reservedAnimationData[4];
    /* 0xCC */ float transitionProgress;
    /* 0xD0 */ s32 sequenceId;
    /* 0xD4 */ s32 sequenceState;
    /* 0xD8 */ float fadeProgress;
    /* 0xDC */ float fadeSpeed;
} PlayerState; // Size 0xE0

typedef struct {
    /* 0x00 */ s16 modelId;
    /* 0x02 */ s16 shadowModelId;
    /* 0x04 */ s16 effectModelId;
    /* 0x06 */ s16 motionId;
    /* 0x08 */ s16 objectType;
    /* 0x0A */ s16 objectState;
    /* 0x0C */ float travelAngle;
    /* 0x10 */ float movementSpeed;
    /* 0x14 */ float positionX;
    /* 0x18 */ float positionY;
    /* 0x1C */ float positionZ;
    /* 0x20 */ float rotation;
    /* 0x24 */ float targetX;
    /* 0x28 */ float targetZ;
} MovingObjectState; // Size 0x2C

void PlayerViewCapturePositionSet(s16 playerModelIndex);
void SnapshotTextureDraw(s16 snapshotIndex);
void SceneInitialize(void);
void PlayerParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void ScoreParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void LeftMascotProcess(void);
void RightMascotProcess(void);
void PlayerProcess(void);
void OpponentProcess(void);
void MinigameProcess(void);
void MinigameMainProcess(void);

s16 ambientLightId;
s16 unusedLightData; // unused
s16 playerModelId;
s16 snapshotCaptureEnabled;
s16 leftMascotActive;
s16 victoryEffectModelId;
s32 minigameState;
s32 roundTimer;
s32 humanPlayerIndex;
s32 winningPlayerIndex;
s32 repositionTimer;
s32 playerSoundIds[4];
s32 mascotEventTimers[4];
s32 mascotEventIndex;
PlayerState playerStates[4];
MascotState mascotStates[2];
MovingObjectState movingObjectStates[2];
HUPROCESS *playerProcesses[4];
ObjectManagerState objectManagerState;
Vec shadowPosition;
Vec shadowDirection;
Vec shadowRotation;
void *snapshotColorTextures[8];
void *snapshotDepthTextures[8];
u8 snapshotBufferIndex;
Vec snapshotPositions[8];

s32 characterModelIds[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

s32 playerMotionDataIds[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), DATA_MAKE_NUM(DATADIR_MARIOMOT, 2), DATA_MAKE_NUM(DATADIR_MARIOMOT, 3),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 21), DATA_MAKE_NUM(DATADIR_MARIOMOT, 26), DATA_MAKE_NUM(DATADIR_MARIOMOT, 27), DATA_MAKE_NUM(DATADIR_M419, 0),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 25), DATA_MAKE_NUM(DATADIR_MARIOMOT, 75), DATA_MAKE_NUM(DATADIR_MARIOMOT, 13),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 16) };

s32 opponentMotionDataIds[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 79), DATA_MAKE_NUM(DATADIR_MARIOMOT, 27), DATA_MAKE_NUM(DATADIR_MARIOMOT, 75),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 22) };

Vec characterShadowOffsets[] = { { 0.0f, 0.0f, 35.0f }, { 0.0f, 0.0f, 35.0f }, { 0.0f, 0.0f, 35.0f }, { 0.0f, 0.0f, 35.0f }, { 0.0f, 0.0f, 35.0f },
    { 0.0f, 0.0f, 35.0f }, { 0.0f, 0.0f, 35.0f }, { 0.0f, 0.0f, 35.0f } };

void ObjectSetup(void)
{
    HUPROCESS *objectManager;

    objectManager = omInitObjMan(50, 0x2000);
    omGameSysInit(objectManager);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 20.0f, 20.0f, 10000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 640, 480);
    objectManagerState.rootObject = omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutViewMulti);
    objectManagerState.rootObject->work[0] = 1;
    CRotM->x = -40.0f;
    CRotM->y = 0.0f;
    CRotM->z = 0.0f;
    CenterM->x = 0.0f;
    CenterM->y = 0.0f;
    CenterM->z = 3000.0f;
    CZoomM[0] = 1500.0f;
    Hu3DLighInit();
    ambientLightId = Hu3DGLightCreate(10000.0f, 0.0f, 10000.0f, -1.0f, 0.0f, -1.0f, 0xFF, 0xFF, 0xFF);
    Hu3DGLightPosAimSet(ambientLightId, 0.0f, 4330.13f, 2500.0f, 0.0f, 0.0f, 0.0f);
    Hu3DGLightInfinitytSet(ambientLightId);
    shadowPosition.x = -500.0f;
    shadowPosition.y = 4000.0f;
    shadowPosition.z = 0.0f;
    shadowDirection.x = shadowDirection.z = 0.0f;
    shadowDirection.y = 1.0f;
    shadowRotation.x = 0.0f;
    shadowRotation.y = 0.0f;
    shadowRotation.z = 0.0f;
    Hu3DShadowCreate(30.0f, 20.0f, 10000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowPosSet(&shadowPosition, &shadowDirection, &shadowRotation);
    Hu3DBGColorSet(0, 0, 0);
    SceneInitialize();
    HuPrcChildCreate(MinigameMainProcess, 0x1000, 0x2000, 0, HuPrcCurrentGet());
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
}

void SnapshotBuffersInitialize(void)
{
    s16 i;

    Hu3DLayerHookSet(1, PlayerViewCapturePositionSet);
    for (i = 0; i < 8; i++) {
        snapshotColorTextures[i] = HuMemDirectMallocNum(HEAP_MODEL, 0x4800, HU_MEMNUM_OVL);
        snapshotDepthTextures[i] = HuMemDirectMallocNum(HEAP_MODEL, 0x9000, HU_MEMNUM_OVL);
        snapshotPositions[i].x = snapshotPositions[i].y = 0.0f;
    }
}

void PlayerViewCapturePositionSet(s16 playerModelIndex)
{
    Vec captureWorldPosition;
    Vec captureScreenPosition;
    HU3DMODEL *capturedModel;
    s16 alignedScreenCoordinate;

    capturedModel = &Hu3DData[playerModelId];
    captureWorldPosition = capturedModel->pos;
    captureWorldPosition.y += 50.0f;
    Hu3D3Dto2D(&captureWorldPosition, 1, &captureScreenPosition);
    alignedScreenCoordinate = captureScreenPosition.x - 96.0f;
    alignedScreenCoordinate &= 0xFFF8;
    captureScreenPosition.x = alignedScreenCoordinate;
    if (captureScreenPosition.x < 0.0f) {
        captureScreenPosition.x = 0.0f;
    }
    if (captureScreenPosition.x > 448.0f) {
        captureScreenPosition.x = 448.0f;
    }
    alignedScreenCoordinate = captureScreenPosition.y - 96.0f;
    alignedScreenCoordinate &= 0xFFF8;
    captureScreenPosition.y = alignedScreenCoordinate;
    if (captureScreenPosition.y < 0.0f) {
        captureScreenPosition.y = 0.0f;
    }
    if (captureScreenPosition.y > 288.0f) {
        captureScreenPosition.y = 288.0f;
    }
    snapshotPositions[snapshotBufferIndex] = captureScreenPosition;
    GXSetTexCopySrc(captureScreenPosition.x, captureScreenPosition.y, 192, 192);
    GXSetTexCopyDst(96, 96, GX_TF_RGB5A3, 1);
    GXCopyTex(snapshotColorTextures[snapshotBufferIndex], 0);
    GXSetTexCopySrc(captureScreenPosition.x, captureScreenPosition.y, 192, 192);
    GXSetTexCopyDst(96, 96, GX_TF_Z24X8, 1);
    GXCopyTex(snapshotDepthTextures[snapshotBufferIndex], 0);
}

void SnapshotTexturesDraw(HU3DMODEL *model, Mtx modelMatrix)
{
    Mtx44 orthographicProjection;
#ifdef NON_MATCHING
    GXColor overlayColor = {0};
#else
    GXColor overlayColor;
#endif
    s16 i;

    C_MTXOrtho(orthographicProjection, 0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 8000.0f);
    GXSetProjection(orthographicProjection, GX_ORTHOGRAPHIC);
    MTXIdentity(modelMatrix);
    GXLoadPosMtxImm(modelMatrix, GX_PNMTX0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_RGBA6, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_RGBA6, 0);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevOp(GX_TEVSTAGE1, GX_PASSCLR);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP1, GX_COLOR_NULL);
    GXSetZTexture(GX_ZT_REPLACE, GX_TF_Z24X8, 0);
    GXSetNumTexGens(1);
    GXSetNumTevStages(2);
    if (snapshotCaptureEnabled != 0) {
        for (i = 1; i < 8; i++) {
            overlayColor.a = i * 24;
            GXSetTevColor(GX_TEVREG0, overlayColor);
            SnapshotTextureDraw((snapshotBufferIndex - 8 + i) & 7);
        }
    }
    snapshotBufferIndex = (snapshotBufferIndex + 1) & 7;
    GXSetZTexture(GX_ZT_DISABLE, GX_TF_Z8, 0);
}

void SnapshotTextureDraw(s16 snapshotIndex)
{
    GXTexObj colorTexture;
    GXTexObj depthTexture;
    Vec quadTopLeft;
    Vec quadTopRight;
    Vec quadBottomRight;
    Vec quadBottomLeft;

    GXInitTexObj(&colorTexture, snapshotColorTextures[snapshotIndex], 96, 96, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObjLOD(&colorTexture, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXLoadTexObj(&colorTexture, GX_TEXMAP0);
    GXInitTexObj(&depthTexture, snapshotDepthTextures[snapshotIndex], 96, 96, GX_TF_Z24X8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&depthTexture, GX_TEXMAP1);
    quadTopLeft.x = 0.0f;
    quadTopLeft.y = 0.0f;
    quadTopLeft.z = 0.0f;
    quadTopRight.x = 640.0f;
    quadTopRight.y = 0.0f;
    quadTopRight.z = 0.0f;
    quadBottomRight.x = 640.0f;
    quadBottomRight.y = 480.0f;
    quadBottomRight.z = 0.0f;
    quadBottomLeft.x = 0.0f;
    quadBottomLeft.y = 480.0f;
    quadBottomLeft.z = 0.0f;
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXSetZCompLoc(GX_FALSE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(snapshotPositions[snapshotIndex].x, snapshotPositions[snapshotIndex].y, 0.0f);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition3f32(snapshotPositions[snapshotIndex].x + 192.0f, snapshotPositions[snapshotIndex].y, 0.0f);
    GXTexCoord2f32(1.0f, 0.0f);
    GXPosition3f32(snapshotPositions[snapshotIndex].x + 192.0f, snapshotPositions[snapshotIndex].y + 192.0f, 0.0f);
    GXTexCoord2f32(1.0f, 1.0f);
    GXPosition3f32(snapshotPositions[snapshotIndex].x, snapshotPositions[snapshotIndex].y + 192.0f, 0.0f);
    GXTexCoord2f32(0.0f, 1.0f);
    GXEnd();
}

void SceneInitialize(void)
{
    void *modelData;
    MascotState *mascotState;
    PlayerState *playerState;
    HUPROCESS *playerProcess;
    float initialPlayerRadius;
    s16 modelId;
    s32 opponentIndex;
    s32 motionDataId;
    ANIMDATA *particleAnimation;
    s32 i;
    s32 j;

    snapshotCaptureEnabled = 0;
    leftMascotActive = 0;
    roundTimer = 0;
    HuAudSndGrpSet(0x2C);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 8), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelShadowMapSet(modelId);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 9), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelShadowMapSet(modelId);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 10), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelId);
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelShadowSet(modelId);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 11), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelShadowSet(modelId);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 12), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    victoryEffectModelId = modelId;
    Hu3DModelLayerSet(modelId, 1);
    mascotState = &mascotStates[0];
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 13), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    mascotState->modelId = modelId;
    Hu3DModelLayerSet(modelId, 1);
    for (j = 0; j < 5; j++) {
        modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 14 + j), HU_MEMNUM_OVL, HEAP_MODEL);
        mascotState->motionIds[j] = Hu3DJointMotion(modelId, modelData);
    }
    Hu3DMotionSet(modelId, mascotState->motionIds[0]);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelId);
    HuPrcChildCreate(LeftMascotProcess, 0x2000, 0x2000, 0, HuPrcCurrentGet());
    mascotState = &mascotStates[1];
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 19), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    mascotState->modelId = modelId;
    Hu3DModelLayerSet(modelId, 1);
    for (j = 0; j < 5; j++) {
        modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 20 + j), HU_MEMNUM_OVL, HEAP_MODEL);
        mascotState->motionIds[j] = Hu3DJointMotion(modelId, modelData);
    }
    Hu3DMotionSet(modelId, mascotState->motionIds[0]);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelId);
    HuPrcChildCreate(RightMascotProcess, 0x2000, 0x2000, 0, HuPrcCurrentGet());
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 25), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    movingObjectStates[0].modelId = modelId;
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelAttrSet(modelId, 1);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 26), HU_MEMNUM_OVL, HEAP_MODEL);
    movingObjectStates[0].effectModelId = Hu3DJointMotion(modelId, modelData);
    Hu3DMotionSet(modelId, movingObjectStates[0].effectModelId);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelId);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 27), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    movingObjectStates[0].shadowModelId = modelId;
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelAttrSet(modelId, 1);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 28), HU_MEMNUM_OVL, HEAP_MODEL);
    movingObjectStates[0].motionId = Hu3DJointMotion(modelId, modelData);
    Hu3DMotionSet(modelId, movingObjectStates[0].effectModelId);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelId);
    movingObjectStates[0].objectType = 0;
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 29), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    movingObjectStates[1].modelId = modelId;
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelAttrSet(modelId, 1);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    movingObjectStates[1].objectType = 1;
    Hu3DModelShadowSet(modelId);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 30), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    movingObjectStates[1].shadowModelId = modelId;
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelAttrSet(modelId, 1);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    for (i = 0, opponentIndex = 0; i < 4; i++) {
        playerState = &playerStates[i];
        playerState->playerIndex = i;
        playerState->initializationIndex = i;
        playerState->controllerIndex = GWPlayerCfg[i].pad_idx;
        if (GWPlayerCfg[playerState->playerIndex].group == 0) {
            humanPlayerIndex = playerState->playerIndex;
            playerModelId = playerState->characterModel = CharModelCreate(characterModelIds[GWPlayerCfg[i].character], 2);
            Hu3DModelLayerSet(playerState->characterModel, 0);
            CharEffectLayerSet(2);
            SnapshotBuffersInitialize();
            playerState->rotationZ = playerState->turnSpeed = playerState->turnAngle = 0.0f;
            playerState->movementSpeed = playerState->turnAngle;
            playerState->positionX = playerState->positionY = playerState->positionZ = 0.0f;
            playerState->collisionRadius = 50.0f;
            playerState->movementSpeedLimit = 0.0f;
            playerState->movementAcceleration = 1.0f;
            Hu3DModelPosSet(playerState->characterModel, playerState->positionX, playerState->positionY, playerState->positionZ);
            Hu3DModelRotSet(playerState->characterModel, playerState->rotationZ, playerState->turnAngle, playerState->turnSpeed);
            Hu3DModelShadowSet(playerState->characterModel);
            for (j = 0; j < 11; j++) {
                motionDataId = playerMotionDataIds[j];
                if ((motionDataId & 0xFFFF0000) == DATADIR_M419) {
                    motionDataId += GWPlayerCfg[i].character;
                }
                playerState->modelMotionIds[j] = CharMotionCreate(characterModelIds[GWPlayerCfg[i].character], motionDataId);
            }
            CharMotionSet(characterModelIds[GWPlayerCfg[i].character], playerState->modelMotionIds[0]);
            Hu3DModelAttrSet(playerState->characterModel, HU3D_MOTATTR_LOOP);
            CharMotionDataClose(characterModelIds[GWPlayerCfg[i].character]);
            playerProcess = HuPrcChildCreate(PlayerProcess, 0x2000, 0x3000, 0, HuPrcCurrentGet());
            playerProcess->user_data = playerState;
            playerProcesses[i] = playerProcess;
        }
        else {
            playerState->shadowModel = CharModelCreate(characterModelIds[GWPlayerCfg[i].character], 2);
            Hu3DModelLayerSet(playerState->characterModel, 1);
            CharEffectLayerSet(2);
            playerState->startRotationX = characterShadowOffsets[GWPlayerCfg[i].character].x;
            playerState->startRotationY = characterShadowOffsets[GWPlayerCfg[i].character].y;
            playerState->startRotationZ = characterShadowOffsets[GWPlayerCfg[i].character].z;
            Hu3DModelPosSet(playerState->shadowModel, playerState->startRotationX, playerState->startRotationY, playerState->startRotationZ);
            modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 31), HU_MEMNUM_OVL, HEAP_MODEL);
            playerState->characterModel = Hu3DModelCreate(modelData);
            Hu3DModelLayerSet(playerState->characterModel, 1);
            playerState->rotationZ = playerState->turnSpeed = 0.0f;
            playerState->turnAngle = 120.0f * opponentIndex;
            opponentIndex++;
            if (playerState->turnAngle > 360.0f) {
                playerState->turnAngle -= 360.0f;
            }
            Hu3DModelAttrSet(playerState->characterModel, 1);
            initialPlayerRadius = 1000.0f;
            playerState->positionX = initialPlayerRadius * sind(playerState->turnAngle);
            playerState->positionY = 0.0f;
            playerState->positionZ = initialPlayerRadius * cosd(playerState->turnAngle);
            playerState->turnAngle += 180.0f;
            if (playerState->turnAngle > 360.0f) {
                playerState->turnAngle -= 360.0f;
            }
            playerState->movementSpeed = playerState->turnAngle;
            playerState->collisionRadius = 200.0f;
            playerState->movementSpeedLimit = 2.25f;
            playerState->movementAcceleration = 1.0f;
            playerState->rotationSpeed = 0.66f;
            Hu3DModelPosSet(playerState->characterModel, playerState->positionX, playerState->positionY, playerState->positionZ);
            Hu3DModelRotSet(playerState->characterModel, playerState->rotationZ, playerState->turnAngle, playerState->turnSpeed);
            Hu3DModelShadowSet(playerState->characterModel);
            Hu3DModelHookSet(playerState->characterModel, "itemhook_c", playerState->shadowModel);
            for (j = 0; j < 4; j++) {
                playerState->characterMotionIds[j] = CharMotionCreate(characterModelIds[GWPlayerCfg[i].character], opponentMotionDataIds[j]);
            }
            CharMotionSet(characterModelIds[GWPlayerCfg[i].character], playerState->characterMotionIds[0]);
            CharMotionTimeSet(characterModelIds[GWPlayerCfg[i].character], 60.0f);
            CharMotionVoiceOnSet(characterModelIds[GWPlayerCfg[i].character], playerState->characterMotionIds[3], 0);
            for (j = 0; j < 8; j++) {
                modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 32 + j), HU_MEMNUM_OVL, HEAP_MODEL);
                playerState->modelMotionIds[j] = Hu3DJointMotion(playerState->characterModel, modelData);
            }
            Hu3DMotionSet(playerState->characterModel, playerState->modelMotionIds[1]);
            Hu3DMotionShiftSpeedSet(playerState->characterModel, 1.0f);
            Hu3DMotionSpeedSet(playerState->characterModel, 1.0f);
            Hu3DModelAttrSet(playerState->characterModel, HU3D_MOTATTR_LOOP);
            modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M419, 40), HU_MEMNUM_OVL, HEAP_MODEL);
            playerState->effectModel = Hu3DModelCreate(modelData);
            Hu3DModelLayerSet(playerState->effectModel, 1);
            Hu3DModelAttrSet(playerState->effectModel, 1);
            Hu3DModelAttrSet(playerState->effectModel, HU3D_MOTATTR_PAUSE);
            CharMotionDataClose(characterModelIds[GWPlayerCfg[i].character]);
            playerProcess = HuPrcChildCreate(OpponentProcess, 0x2000, 0x3000, 0, HuPrcCurrentGet());
            playerProcess->user_data = playerState;
            playerProcesses[i] = playerProcess;
        }
    }
    particleAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 2), HU_MEMNUM_OVL));
    modelId = Hu3DParticleCreate(particleAnimation, 50);
    Hu3DModelPosSet(modelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(modelId, 1.0f, 1.0f, 1.0f);
    Hu3DModelLayerSet(modelId, 3);
    Hu3DParticleColSet(modelId, 0xFF, 0xFF, 0xFF);
    Hu3DParticleScaleSet(modelId, 0.0f);
    Hu3DParticleHookSet(modelId, PlayerParticleUpdate);
    particleAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 2), HU_MEMNUM_OVL));
    modelId = Hu3DParticleCreate(particleAnimation, 50);
    Hu3DModelPosSet(modelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(modelId, 1.0f, 1.0f, 1.0f);
    Hu3DModelLayerSet(modelId, 3);
    Hu3DParticleColSet(modelId, 0xFF, 0xFF, 0xFF);
    Hu3DParticleScaleSet(modelId, 0.0f);
    Hu3DParticleHookSet(modelId, ScoreParticleUpdate);
    HuDataDirClose(DATADIR_EFFECT);
    mascotEventIndex = 0;
    mascotEventTimers[0] = frandmod(5) * REFRESH_RATE + REFRESH_RATE * 10;
    mascotEventTimers[1] = frandmod(5) * REFRESH_RATE + REFRESH_RATE * 23;
    mascotEventTimers[2] = REFRESH_RATE * 46;
    Hu3DModelLayerSet(Hu3DHookFuncCreate(SnapshotTexturesDraw), 1);
    HuPrcChildCreate(MinigameProcess, 0x1000, 0x1000, 0, HuPrcCurrentGet());
}

float ArenaPositionLimitGet(float position, float radius)
{
    float positionLimit;
    float unusedLimitScratch;

    if (position > 600.0f - radius) {
        positionLimit = 600.0f + 200.0f * ((800.0f - (position + radius)) / 200.0f);
    }
    else if (position < -(600.0f - radius)) {
        positionLimit = 600.0f + 200.0f * ((800.0f - ABS_INV(position - radius)) / 200.0f);
    }
    else {
        positionLimit = 800.0f;
    }
    return positionLimit;
}

float PlayerPositionLimitGet(float position, float radius)
{
    return ArenaPositionLimitGet(position, radius);
}

float AngleNormalize(float angle)
{
    while (angle >= 360.0f || angle < 0.0f) {
        if (angle >= 360.0f) {
            angle -= 360.0f;
        }
        if (angle < 0.0f) {
            angle += 360.0f;
        }
    }
    return angle;
}

void PlayerParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    PlayerState *playerState;
    float particleAngle;
    float particleRadius;
    float emissionAngle;
    s32 particleAlpha;
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
    for (i = 0; i < 4; i++) {
        playerState = &playerStates[i];
        if (playerState->effectTimer & 1) {
            for (j = 0, emissionAngle = 0.0f; j < 16; j++, emissionAngle += 22.5f) {
                for (k = 0; k < particle->maxCnt; k++, particleData++) {
                    if (particleData->accel.x == 0.0f) {
                        break;
                    }
                }
                if (k == particle->maxCnt) {
                    break;
                }
                particleAngle = emissionAngle;
                particleRadius = 125.0f + 70.0f * frandf();
                particleData->pos.x = playerState->previousPositionX + particleRadius * sind(particleAngle);
                particleData->pos.y = playerState->previousPositionY + 50.0f * frandf();
                particleData->pos.z = playerState->previousPositionZ + particleRadius * cosd(particleAngle);
                particleData->vel.x = 5.0 * sind(particleAngle);
                particleData->vel.y = 9.0f + 2.0f * frandf();
                particleData->vel.z = 5.0 * cosd(particleAngle);
                particleData->accel.x = 30.0f;
                particleData->scale = 80.0f + 20.0f * frandf();
                particleData->accel.y = 15.0f;
                particleData->color.a = 0xC8;
                particleData->accel.z = 17.0f;
            }
            playerState->effectTimer &= ~1;
        }
    }
    particleData = particle->data;
    for (k = 0; k < particle->maxCnt; k++, particleData++) {
        if (particleData->accel.x != 0.0f) {
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
            particleAlpha = particleData->color.a;
            particleAlpha -= particleData->accel.z;
            if (particleData->accel.z > 3.0f) {
                particleData->accel.z -= 1.0f;
            }
            if (particleAlpha < 0) {
                particleAlpha = 0;
                particleData->accel.x = 0.0f;
            }
            particleData->color.a = particleAlpha;
            if (particleData->accel.x == 0.0f) {
                particleData->scale = 0.0f;
            }
        }
    }
}

void ScoreParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    PlayerState *playerState;
    float particleOffsetX;
    float particleOffsetZ;
    s32 particleAlpha;
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
    for (i = 0; i < 4; i++) {
        playerState = &playerStates[i];
        if (playerState->effectTimer & 2) {
            for (j = 0; j < 10; j++) {
                for (k = 0; k < particle->maxCnt; k++, particleData++) {
                    if (particleData->accel.x == 0.0f) {
                        break;
                    }
                }
                if (k == particle->maxCnt) {
                    break;
                }
                particleOffsetX = 400.0f * frandf() - 200.0f;
                particleOffsetZ = 400.0f * frandf() - 200.0f;
                particleData->pos.x = playerState->previousPositionX + particleOffsetX;
                particleData->pos.y = playerState->previousPositionY + 50.0f;
                particleData->pos.z = playerState->previousPositionZ + particleOffsetZ;
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
            playerState->effectTimer &= ~2;
        }
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
        if (particleData->accel.x != 0.0f && particleData->colorIdx != 0.0f) {
            VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
            if (particleData->vel.y > 1.0f) {
                particleData->vel.y -= 0.3f;
            }
            particleData->scale += particleData->accel.y;
            if (particleData->accel.y > 2.0f) {
                particleData->accel.y -= 0.4f;
            }
            particleAlpha = particleData->color.a;
            particleAlpha -= particleData->accel.z;
            if (particleData->accel.z > 3.0f) {
                particleData->accel.z -= 1.0f;
            }
            if (particleAlpha < 0) {
                particleAlpha = 0;
                particleData->accel.x = 0.0f;
            }
            particleData->color.a = particleAlpha;
            if (particleData->accel.x == 0.0f) {
                particleData->scale = 0.0f;
            }
        }
    }
}

s32 NearestPlayerGet(MovingObjectState *movingObject)
{
    PlayerState *playerState;
    Vec objectPosition;
    Vec playerPosition;
    Vec separation;
    float nearestDistance;
    float playerDistance;
    s32 nearestPlayerIndex;
    s32 i;

    for (i = 0, nearestDistance = 10000.0f, nearestPlayerIndex = -1; i < 4; i++) {
        playerState = &playerStates[i];
        HuSetVecF(&objectPosition, movingObject->positionX, 0.0, movingObject->positionZ);
        HuSetVecF(&playerPosition, playerState->positionX, 0.0, playerState->positionZ);
        HuSubVecF(&separation, &objectPosition, &playerPosition);
        if ((playerDistance = VECMagXYZ(&separation)) < 50.0f + playerState->collisionRadius && playerDistance < nearestDistance) {
            nearestDistance = playerDistance;
            nearestPlayerIndex = i;
        }
    }
    return nearestPlayerIndex;
}

void MovingObjectPositionClamp(MovingObjectState *movingObject)
{
    float collisionRadius;
    float zPositionLimit;
    float xPositionLimit;

    collisionRadius = 50.0f;
    zPositionLimit = ArenaPositionLimitGet(movingObject->positionZ, 50.0f);
    xPositionLimit = ArenaPositionLimitGet(movingObject->positionX, 50.0f);
    if (movingObject->positionX > zPositionLimit - collisionRadius) {
        movingObject->positionX = zPositionLimit - collisionRadius;
    }
    if (movingObject->positionX < -(zPositionLimit - collisionRadius)) {
        movingObject->positionX = -(zPositionLimit - collisionRadius);
    }
    if (movingObject->positionZ > xPositionLimit - collisionRadius) {
        movingObject->positionZ = xPositionLimit - collisionRadius;
    }
    if (movingObject->positionZ < -(xPositionLimit - collisionRadius)) {
        movingObject->positionZ = -(xPositionLimit - collisionRadius);
    }
    if (movingObject->positionZ > xPositionLimit - collisionRadius || movingObject->positionZ < -(xPositionLimit - collisionRadius)) {
        movingObject->positionX += movingObject->movementSpeed * sind(movingObject->travelAngle) * 0.8f;
    }
    if (movingObject->positionX > zPositionLimit - collisionRadius || movingObject->positionX < -(zPositionLimit - collisionRadius)) {
        movingObject->positionZ += movingObject->movementSpeed * cosd(movingObject->travelAngle) * 0.8f;
    }
    if (movingObject->positionX > zPositionLimit - collisionRadius) {
        movingObject->positionX = zPositionLimit - collisionRadius;
    }
    if (movingObject->positionX < -(zPositionLimit - collisionRadius)) {
        movingObject->positionX = -(zPositionLimit - collisionRadius);
    }
    if (movingObject->positionZ > xPositionLimit - collisionRadius) {
        movingObject->positionZ = xPositionLimit - collisionRadius;
    }
    if (movingObject->positionZ < -(xPositionLimit - collisionRadius)) {
        movingObject->positionZ = -(xPositionLimit - collisionRadius);
    }
}

void PlayerModelFadeProcess(void)
{
    PlayerState *playerState;
    float modelTransparency;
    s32 playerIndex;
    s32 i;

    playerIndex = (s32)HuPrcCurrentGet()->user_data;
    playerState = &playerStates[playerIndex];
    for (i = 0, modelTransparency = 1.0f; i < 30; i++, modelTransparency -= 0.033333335f) {
        Hu3DModelAmbSet(playerState->characterModel, 1.0f, modelTransparency, modelTransparency);
        HuPrcVSleep();
    }
    Hu3DModelAmbSet(playerState->characterModel, 1.0f, 0.0f, 0.0f);
    while (playerState->motionSpeed > 30) {
        HuPrcVSleep();
    }
    for (i = 0, modelTransparency = 0.0f; i < 30; i++, modelTransparency += 0.033333335f) {
        Hu3DModelAmbSet(playerState->characterModel, 1.0f, modelTransparency, modelTransparency);
        HuPrcVSleep();
    }
    Hu3DModelAmbSet(playerState->characterModel, 1.0f, 1.0f, 1.0f);
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void MovingObjectProcess(void)
{
    MovingObjectState *movingObject;
    PlayerState *playerState;
    Vec targetPosition;
    Vec objectPosition;
    Vec separation;
    float collisionAngle;
    float previousPositionX;
    float previousPositionY;
    float previousPositionZ;
    float playerClearanceHeight;
    float verticalDeceleration;
    float playerDistance;
    float nearestDistance;
    float modelScale;
    float rotationStep;
    float rotationAngle;
    float objectHeight;
    float verticalSpeed;
    s32 blinkFrame;
    s32 processInterrupted;
    s16 visibleModelId;
    s32 nearestPlayerIndex;
    s32 animationFrames;
    s32 i;

    movingObject = HuPrcCurrentGet()->user_data;
    HuPrcSleep(25);
    processInterrupted = 0;
    movingObject->objectState = 0;
    animationFrames = 0;
    objectHeight = movingObject->positionY;
    verticalSpeed = 15.0f;
    verticalDeceleration = 0.9f;
    while (objectHeight > 50.0f) {
        verticalSpeed -= verticalDeceleration;
        objectHeight += verticalSpeed;
        animationFrames++;
    }
    if (movingObject->objectType == 0) {
        Hu3DMotionSet(movingObject->modelId, movingObject->effectModelId);
        Hu3DModelAttrSet(movingObject->modelId, HU3D_MOTATTR_LOOP);
        Hu3DModelTPLvlSet(movingObject->modelId, 1.0f);
    }
    else {
        Hu3DModelAttrReset(movingObject->shadowModelId, 1);
    }
    Hu3DModelAttrReset(movingObject->modelId, 1);
    movingObject->travelAngle = atan2d(movingObject->targetX - movingObject->positionX, movingObject->targetZ - movingObject->positionZ);
    HuSetVecF(&targetPosition, movingObject->targetX, 0.0, movingObject->targetZ);
    HuSetVecF(&objectPosition, movingObject->positionX, 0.0, movingObject->positionZ);
    HuSubVecF(&separation, &targetPosition, &objectPosition);
    movingObject->movementSpeed = VECMagXYZ(&separation) / animationFrames;
    objectHeight = movingObject->positionY;
    verticalSpeed = 15.0f;
    verticalDeceleration = 0.9f;
    while (objectHeight > 50.0f) {
        previousPositionX = movingObject->positionX;
        previousPositionY = movingObject->positionY;
        previousPositionZ = movingObject->positionZ;
        movingObject->positionX += movingObject->movementSpeed * sind(movingObject->travelAngle);
        movingObject->positionY = objectHeight;
        movingObject->positionZ += movingObject->movementSpeed * cosd(movingObject->travelAngle);
        if ((nearestPlayerIndex = NearestPlayerGet(movingObject)) != -1) {
            playerState = &playerStates[nearestPlayerIndex];
            if (GWPlayerCfg[playerState->playerIndex].group == 0) {
                playerClearanceHeight = 150.0f;
            }
            else {
                playerClearanceHeight = 400.0f;
            }
            if (movingObject->positionY < playerClearanceHeight) {
                if (movingObject->positionY < playerClearanceHeight && movingObject->positionY > playerClearanceHeight - 50.0f) {
                    verticalSpeed = 15.0f;
                    verticalDeceleration = 0.9f;
                }
                else {
                    collisionAngle = atan2d(movingObject->positionX - playerState->positionX, movingObject->positionZ - playerState->positionZ);
                    movingObject->travelAngle = AngleNormalize(180.0f + movingObject->travelAngle);
                    movingObject->travelAngle = collisionAngle - (movingObject->travelAngle - collisionAngle);
                }
                movingObject->positionX = previousPositionX;
                movingObject->positionY = previousPositionY;
                movingObject->positionZ = previousPositionZ;
            }
        }
        MovingObjectPositionClamp(movingObject);
        Hu3DModelPosSet(movingObject->modelId, movingObject->positionX, movingObject->positionY, movingObject->positionZ);
        if (movingObject->objectType != 0) {
            movingObject->rotation += 10.0f;
            Hu3DModelPosSet(movingObject->shadowModelId, movingObject->positionX, movingObject->positionY, movingObject->positionZ);
            Hu3DModelRotSet(movingObject->shadowModelId, 0.0f, 0.0f, movingObject->rotation);
        }
        Hu3DModelRotSet(movingObject->modelId, 0.0f, 0.0f, movingObject->rotation);
        if (minigameState >= 4) {
            processInterrupted = 1;
            goto movingObjectProcessingComplete;
        }
        HuPrcVSleep();
        verticalSpeed -= verticalDeceleration;
        objectHeight += verticalSpeed;
    }
    Hu3DModelPosSet(movingObject->modelId, movingObject->positionX, 50.0f, movingObject->positionZ);
    if (movingObject->objectType != 0) {
        Hu3DModelPosSet(movingObject->shadowModelId, movingObject->positionX, 50.0f, movingObject->positionZ);
    }
    if (movingObject->objectType == 0) {
        Hu3DModelAttrSet(movingObject->modelId, 1);
        Hu3DModelAttrReset(movingObject->shadowModelId, 1);
        Hu3DModelPosSet(movingObject->shadowModelId, movingObject->positionX, 0.0f, movingObject->positionZ);
        Hu3DModelRotSet(movingObject->shadowModelId, 0.0f, 0.0f, 0.0f);
        Hu3DMotionSet(movingObject->shadowModelId, movingObject->motionId);
        Hu3DModelAttrReset(movingObject->shadowModelId, HU3D_MOTATTR_LOOP);
        Hu3DModelTPLvlSet(movingObject->shadowModelId, 1.0f);
    }
    for (animationFrames = REFRESH_RATE * 5, blinkFrame = 0; animationFrames != 0; animationFrames--) {
        if (movingObject->objectType != 0 && 0.0f != movingObject->rotation) {
            movingObject->rotation += 10.0f;
            if (movingObject->rotation > 360.0f) {
                movingObject->rotation = 0.0f;
            }
            Hu3DModelRotSet(movingObject->modelId, 0.0f, 0.0f, movingObject->rotation);
            Hu3DModelRotSet(movingObject->shadowModelId, 0.0f, 0.0f, movingObject->rotation);
        }
        for (i = 0, nearestDistance = 10000.0f; i < 4; i++) {
            playerState = &playerStates[i];
            HuSetVecF(&targetPosition, movingObject->positionX, 0.0, movingObject->positionZ);
            HuSetVecF(&objectPosition, playerState->positionX, 0.0, playerState->positionZ);
            HuSubVecF(&separation, &targetPosition, &objectPosition);
            if ((playerDistance = VECMagXYZ(&separation)) < 50.0f + playerState->collisionRadius && playerDistance < nearestDistance) {
                nearestDistance = playerDistance;
                nearestPlayerIndex = i;
            }
        }
        if (nearestDistance < 10000.0f) {
            playerState = &playerStates[nearestPlayerIndex];
            movingObject->objectState = 1;
            if (movingObject->objectType != 0) {
                playerState->motionAttributes = 1;
                playerState->motionSpeed = 300;
                if (GWPlayerCfg[playerState->playerIndex].group == 0) {
                    playerState->movementAcceleration = 3.0f;
                    snapshotCaptureEnabled = 1;
                    HuAudFXPlay(0x5E6);
                }
                else {
                    playerState->movementSpeedLimit = 6.75f;
                    playerState->rotationSpeed = 0.99f;
                    HuAudFXPlay(0x5E2);
                }
            }
            else if (playerState->resultState == 0) {
                playerState->effectState = 4;
                PlayerFXPlay(playerState->playerIndex, 0x128);
                HuAudFXPlay(0x5E9);
            }
            break;
        }
        if (animationFrames < REFRESH_RATE) {
            visibleModelId = movingObject->modelId;
            if (movingObject->objectType == 0) {
                visibleModelId = movingObject->shadowModelId;
            }
            if (blinkFrame & 1) {
                Hu3DModelAttrReset(visibleModelId, 1);
            }
            else {
                Hu3DModelAttrSet(visibleModelId, 1);
            }
            blinkFrame++;
        }
        HuPrcVSleep();
        if (minigameState >= 4) {
            processInterrupted = 1;
            break;
        }
    }
movingObjectProcessingComplete:
    if (processInterrupted != 0) {
        for (i = 0, modelScale = 1.0f; i < 30; i++) {
            modelScale -= 0.033333335f;
            Hu3DModelScaleSet(movingObject->modelId, modelScale, modelScale, modelScale);
            HuPrcVSleep();
        }
        Hu3DModelAttrSet(movingObject->modelId, 1);
        Hu3DModelAttrSet(movingObject->shadowModelId, 1);
    }
    else {
        if (movingObject->objectType == 0) {
            playerState = &playerStates[nearestPlayerIndex];
            collisionAngle = atan2d(movingObject->positionX - playerState->positionX, movingObject->positionZ - playerState->positionZ);
            rotationAngle = 1.0f;
            movingObject->rotation = 0.0f;
            for (i = 0, rotationStep = 0.0f; i < 10; i++, rotationStep += 9.0f, rotationAngle -= 0.1f) {
                movingObject->positionX += 2.0 * sind(collisionAngle);
                movingObject->positionY += 10.0f;
                movingObject->positionZ += 2.0 * cosd(collisionAngle);
                Hu3DModelPosSet(movingObject->shadowModelId, movingObject->positionX, movingObject->positionY, movingObject->positionZ);
                movingObject->rotation += 10.0f;
                Hu3DModelRotSet(movingObject->shadowModelId, 0.0f, 0.0f, movingObject->rotation);
                Hu3DModelTPLvlSet(movingObject->shadowModelId, rotationAngle);
                HuPrcVSleep();
            }
        }
        Hu3DModelAttrSet(movingObject->modelId, 1);
        if (movingObject->objectType != 0) {
            Hu3DModelAttrSet(movingObject->shadowModelId, 1);
        }
        movingObject->rotation = 0.0f;
        HuPrcCurrentGet()->user_data = NULL;
        HuPrcKill(HuPrcCurrentGet());
    }
    while (TRUE) {
        HuPrcVSleep();
    }
}

void LeftMascotProcess(void)
{
    MascotState *mascotState;
    MovingObjectState *movingObject;
    HU3DMODEL *mascotModel;
    HUPROCESS *movingObjectProcess;
    s32 motionFrames;

    mascotState = &mascotStates[0];
    while (minigameState != 3) {
        HuPrcVSleep();
    }
    while (TRUE) {
        while (leftMascotActive != 0) {
            HuPrcVSleep();
        }
        if (minigameState >= 4) {
            break;
        }
        movingObject = &movingObjectStates[0];
        if (roundTimer > mascotEventTimers[mascotEventIndex]) {
            mascotEventIndex++;
            movingObject = &movingObjectStates[1];
            Hu3DMotionShiftSet(mascotState->modelId, mascotState->motionIds[4], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
        }
        else {
            Hu3DMotionShiftSet(mascotState->modelId, mascotState->motionIds[3], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
        }
        HuPrcVSleep();
        motionFrames = Hu3DMotionMaxTimeGet(mascotState->modelId);
        while (motionFrames != 0) {
            HuPrcVSleep();
            motionFrames--;
        }
        HuAudFXPlay(0x5E4);
        movingObjectProcess = HuPrcChildCreate(MovingObjectProcess, 0x2010, 0x3000, 0, HuPrcCurrentGet());
        movingObjectProcess->user_data = movingObject;
        mascotModel = &Hu3DData[mascotStates[0].modelId];
        movingObject->positionX = mascotModel->pos.x + 640.0f;
        movingObject->positionY = mascotModel->pos.y + 620.0f;
        movingObject->positionZ = mascotModel->pos.y - 200.0f;
        movingObject->rotation = 0.0f;
        movingObject->targetX = 1000.0f * frandf() - 500.0f;
        movingObject->targetZ = 1000.0f * frandf() - 500.0f;
        while (!Hu3DMotionEndCheck(mascotState->modelId)) {
            HuPrcVSleep();
        }
        Hu3DMotionShiftSet(mascotState->modelId, mascotState->motionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        while (movingObjectProcess->user_data) {
            HuPrcVSleep();
        }
        leftMascotActive = 1;
        if (movingObject->objectState != 0) {
            HuAudFXPlay(0x5E8);
            Hu3DMotionShiftSet(mascotState->modelId, mascotState->motionIds[2], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
            HuPrcSleep(6);
            while (!Hu3DMotionEndCheck(mascotState->modelId)) {
                HuPrcVSleep();
            }
            Hu3DMotionShiftSet(mascotState->modelId, mascotState->motionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        }
    }
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RightMascotProcess(void)
{
    MascotState *mascotState;
    MovingObjectState *movingObject;
    HUPROCESS *movingObjectProcess;
    HU3DMODEL *mascotModel;
    s32 motionFrames;

    mascotState = &mascotStates[1];
    while (minigameState != 3) {
        HuPrcVSleep();
    }
    while (TRUE) {
        while (leftMascotActive == 0) {
            HuPrcVSleep();
        }
        if (minigameState >= 4) {
            break;
        }
        movingObject = movingObjectStates;
        if (roundTimer > mascotEventTimers[mascotEventIndex]) {
            mascotEventIndex++;
            movingObject = &movingObjectStates[1];
            Hu3DMotionShiftSet(mascotState->modelId, mascotState->motionIds[4], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
        }
        else {
            Hu3DMotionShiftSet(mascotState->modelId, mascotState->motionIds[3], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
        }
        HuPrcVSleep();
        motionFrames = Hu3DMotionMaxTimeGet(mascotState->modelId);
        while (motionFrames != 0) {
            HuPrcVSleep();
            motionFrames--;
        }
        HuAudFXPlay(0x5E4);
        movingObjectProcess = HuPrcChildCreate(MovingObjectProcess, 0x2010, 0x3000, 0, HuPrcCurrentGet());
        movingObjectProcess->user_data = movingObject;
        mascotModel = &Hu3DData[mascotStates->modelId];
        movingObject->positionX = -(mascotModel->pos.x + 640.0f);
        movingObject->positionY = mascotModel->pos.y + 620.0f;
        movingObject->positionZ = mascotModel->pos.y - 200.0f;
        movingObject->targetX = 1000.0f * frandf() - 500.0f;
        movingObject->targetZ = 1000.0f * frandf() - 500.0f;
        while (!Hu3DMotionEndCheck(mascotState->modelId)) {
            HuPrcVSleep();
        }
        Hu3DMotionShiftSet(mascotState->modelId, mascotState->motionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        while (movingObjectProcess->user_data) {
            HuPrcVSleep();
        }
        leftMascotActive = 0;
        if (movingObject->objectState != 0) {
            HuAudFXPlay(0x5E8);
            Hu3DMotionShiftSet(mascotState->modelId, mascotState->motionIds[2], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
            HuPrcSleep(6);
            while (!Hu3DMotionEndCheck(mascotState->modelId)) {
                HuPrcVSleep();
            }
            Hu3DMotionShiftSet(mascotState->modelId, mascotState->motionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        }
    }
    while (TRUE) {
        HuPrcVSleep();
    }
}

s8 PlayerStickAxisGet(s32 playerIndex, s32 verticalAxis)
{
    PlayerState *playerState;
    s8 stickAxisValue;

    playerState = &playerStates[playerIndex];
    if (GWPlayerCfg[playerIndex].iscom == 0) {
        (verticalAxis != 0) ? (stickAxisValue = HuPadStkY[playerState->controllerIndex]) : (stickAxisValue = HuPadStkX[playerState->controllerIndex]);
    }
    else {
        (verticalAxis != 0) ? (stickAxisValue = playerState->teamId) : (stickAxisValue = playerState->playerState);
    }
    return stickAxisValue;
}

u8 PlayerTriggerAxisGet(s32 playerIndex, s32 verticalAxis)
{
    PlayerState *playerState;
    s8 triggerAxisValue;

    playerState = &playerStates[playerIndex];
    if (GWPlayerCfg[playerIndex].iscom == 0) {
        (verticalAxis != 0) ? (triggerAxisValue = HuPadTrigR[playerState->controllerIndex]) : (triggerAxisValue = HuPadTrigL[playerState->controllerIndex]);
    }
    else {
        (verticalAxis != 0) ? (triggerAxisValue = playerState->difficulty) : (triggerAxisValue = playerState->verticalInput);
    }
    return triggerAxisValue;
}

u16 PlayerButtonsDownGet(s32 playerIndex)
{
    PlayerState *playerState;
    u16 buttonsPressed;

    playerState = &playerStates[playerIndex];
    if (GWPlayerCfg[playerIndex].iscom == 0) {
        buttonsPressed = HuPadBtnDown[playerState->controllerIndex];
    }
    else {
        buttonsPressed = playerState->buttonFlags;
    }
    return buttonsPressed;
}

u16 PlayerButtonsGet(s32 playerIndex)
{
    PlayerState *playerState;
    u16 buttonsHeld;

    playerState = &playerStates[playerIndex];
    if (GWPlayerCfg[playerIndex].iscom == 0) {
        buttonsHeld = HuPadBtn[playerState->controllerIndex];
    }
    else {
        buttonsHeld = playerState->buttonState;
    }
    return buttonsHeld;
}

s32 NearbyPlayerGet(PlayerState *player, float collisionMargin, s32 excludeHumanPlayer)
{
    Vec separation;
    Vec otherPosition;
    Vec playerPosition;
    PlayerState *otherPlayer;
    s32 nearestPlayerIndex;
    float playerDistance;
    float nearestDistance;
    s32 i;

    nearestPlayerIndex = -1;
    for (i = 0, nearestDistance = 10000.0f; i < 4; i++) {
        if (i != player->playerIndex && (excludeHumanPlayer == 0 || i != humanPlayerIndex)) {
            otherPlayer = &playerStates[i];
            HuSetVecF(&separation, player->positionX, player->positionY, player->positionZ);
            HuSetVecF(&otherPosition, otherPlayer->positionX, otherPlayer->positionY, otherPlayer->positionZ);
            HuSubVecF(&playerPosition, &separation, &otherPosition);
            if ((playerDistance = VECMagXYZ(&playerPosition)) < player->collisionRadius + otherPlayer->collisionRadius + collisionMargin && playerDistance < nearestDistance) {
                nearestDistance = playerDistance;
                nearestPlayerIndex = i;
            }
        }
    }
    return nearestPlayerIndex;
}

s32 PlayerArenaCollisionCheck(PlayerState *player)
{
    float zPositionLimit;
    float xPositionLimit;
    s32 collidingPlayerIndex;

    zPositionLimit = ArenaPositionLimitGet(player->positionZ, player->collisionRadius);
    xPositionLimit = ArenaPositionLimitGet(player->positionX, player->collisionRadius);
    if (player->positionX > zPositionLimit - player->collisionRadius) {
        player->positionX = zPositionLimit - player->collisionRadius;
    }
    if (player->positionX < -(zPositionLimit - player->collisionRadius)) {
        player->positionX = -(zPositionLimit - player->collisionRadius);
    }
    if (player->positionZ > xPositionLimit - player->collisionRadius) {
        player->positionZ = xPositionLimit - player->collisionRadius;
    }
    if (player->positionZ < -(xPositionLimit - player->collisionRadius)) {
        player->positionZ = -(xPositionLimit - player->collisionRadius);
    }
    if ((collidingPlayerIndex = NearbyPlayerGet(player, 0.0f, 0)) != -1) {
        player->positionX = player->rotationX;
        player->positionZ = player->rotationY;
    }
    return collidingPlayerIndex;
}

float AngleApproach(float currentAngle, float targetAngle, float interpolationRate)
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
    interpolatedAngle = fmod(currentAngle + angleDifference * interpolationRate, 360.0);
    if (0.0f > interpolatedAngle) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

float PlayerDistanceGet(float positionX, float positionZ, PlayerState *player)
{
    Vec targetPosition;
    Vec playerPosition;
    Vec separation;

    HuSetVecF(&targetPosition, positionX, 0.0, positionZ);
    HuSetVecF(&playerPosition, player->positionX, 0.0, player->positionZ);
    HuSubVecF(&separation, &targetPosition, &playerPosition);
    return VECMagXZ(&separation);
}

float AngleOppositeGet(float angle, float otherAngle)
{
    float oppositeAngle;

    if (angle > otherAngle) {
        oppositeAngle = AngleNormalize(angle - otherAngle);
        if (oppositeAngle < 180.0f) {
            oppositeAngle = AngleNormalize(otherAngle + oppositeAngle + 180.0f);
        }
        else {
            oppositeAngle = AngleNormalize(otherAngle - oppositeAngle);
        }
    }
    else {
        oppositeAngle = AngleNormalize(otherAngle - angle);
        if (oppositeAngle < 180.0f) {
            oppositeAngle = AngleNormalize(angle + oppositeAngle + 180.0f);
        }
        else {
            oppositeAngle = AngleNormalize(angle - oppositeAngle);
        }
    }
    return oppositeAngle;
}

s32 PlayerNearArenaEdgeCheck(PlayerState *player, float edgeMargin)
{
    s32 nearEdge;
    float xPositionLimit;
    float zPositionLimit;

    xPositionLimit = ArenaPositionLimitGet(player->positionZ, player->collisionRadius);
    zPositionLimit = ArenaPositionLimitGet(player->positionX, player->collisionRadius);
    nearEdge = 0;
    if (player->positionX > xPositionLimit - player->collisionRadius - edgeMargin || player->positionX < -(xPositionLimit - player->collisionRadius - edgeMargin) || player->positionZ > zPositionLimit - player->collisionRadius - edgeMargin
        || player->positionZ < -(zPositionLimit - player->collisionRadius - edgeMargin)) {
        nearEdge = 1;
    }
    return nearEdge;
}

void PlayerMovementUpdate(PlayerState *player)
{
    Vec movementInput;

    HuSetVecF(&movementInput, PlayerStickAxisGet(player->playerIndex, 0) / 4.0f, 0.0, -PlayerStickAxisGet(player->playerIndex, 1) / 4.0f);
    player->movementSpeedLimit = VECMagXYZ(&movementInput);
    if (player->movementSpeedLimit >= 0.5f) {
        if (player->movementSpeedLimit >= 10.0f) {
            movementInput.x /= player->movementSpeedLimit;
            movementInput.z /= player->movementSpeedLimit;
            player->movementSpeedLimit = 10.0f;
            movementInput.x = 10.0f * movementInput.x;
            movementInput.z = 10.0f * movementInput.z;
        }
        player->positionX += movementInput.x * player->movementAcceleration;
        player->positionZ += movementInput.z * player->movementAcceleration;
    }
}

void PlayerTargetSelect(Vec *targetPosition, PlayerState **selectedPlayer)
{
    float nearestDistance;
    PlayerState *selectedPlayerIndex;
    PlayerState *playerState;
    s32 playerIndex;
    s32 i;

    for (i = 0; i < 3; i++) {
        selectedPlayerIndex = selectedPlayer[i];
        (i >= 2) ? (playerIndex = 0) : (playerIndex = i + 1);
        playerState = selectedPlayer[playerIndex];
        nearestDistance = ABS_INV(selectedPlayerIndex->positionX - playerState->positionX) / 2.0f;
        if (selectedPlayerIndex->positionX > playerState->positionX) {
            targetPosition[i].x = selectedPlayerIndex->positionX - nearestDistance;
        }
        else {
            targetPosition[i].x = playerState->positionX - nearestDistance;
        }
        selectedPlayerIndex = selectedPlayer[i];
        (i >= 2) ? (playerIndex = 0) : (playerIndex = i + 1);
        playerState = selectedPlayer[playerIndex];
        nearestDistance = ABS_INV(selectedPlayerIndex->positionZ - playerState->positionZ) / 2.0f;
        if (selectedPlayerIndex->positionZ > playerState->positionZ) {
            targetPosition[i].z = selectedPlayerIndex->positionZ - nearestDistance;
        }
        else {
            targetPosition[i].z = playerState->positionZ - nearestDistance;
        }
    }
}

s32 PlayerTargetFind(float positionX, float positionZ, PlayerState *player, PlayerState **selectedPlayer)
{
    PlayerState *playerIndex;
    PlayerState candidatePlayer;
    float targetAngle;
    s32 targetPlayerIndex;
    s32 i;

    playerIndex = &candidatePlayer;
    playerIndex->playerIndex = player->playerIndex;
    playerIndex->positionX = player->positionX;
    playerIndex->positionY = player->positionY;
    playerIndex->positionZ = player->positionZ;
    playerIndex->collisionRadius = player->collisionRadius;
    playerIndex->movementSpeedLimit = player->movementSpeedLimit;
    playerIndex->movementAcceleration = player->movementAcceleration;
    targetAngle = AngleNormalize(atan2d(positionX - player->positionX, positionZ - player->positionZ));
    targetPlayerIndex = 1;
    while (PlayerDistanceGet(positionX, positionZ, playerIndex) >= 10.0f) {
        playerIndex->positionX += 10.0 * sind(targetAngle);
        playerIndex->positionZ += 10.0 * cosd(targetAngle);
        for (i = 0; i < 3; i++) {
            if (PlayerDistanceGet(selectedPlayer[i]->positionX, selectedPlayer[i]->positionZ, playerIndex) < playerIndex->collisionRadius + selectedPlayer[i]->collisionRadius + 10.0f) {
                break;
            }
        }
        if (i < 3) {
            targetPlayerIndex = 0;
            break;
        }
    }
    return targetPlayerIndex;
}

s32 ActivePlayerCountGet(void)
{
    PlayerState *playerCounts[3];
    PlayerState *teamPlayerCount;
    s32 playerIndex;
    s32 activePlayerCount;
    s32 i;

    for (i = 0, playerIndex = 0; i < 4; i++) {
        if (i == humanPlayerIndex) {
            teamPlayerCount = &playerStates[i];
        }
        else {
            playerCounts[playerIndex++] = &playerStates[i];
        }
    }
    playerIndex = 0;
    for (i = 0; i < 3; i++) {
        if (playerCounts[i]->positionX > teamPlayerCount->positionX) {
            break;
        }
    }
    if (i < 3) {
        playerIndex++;
        for (i = 0; i < 3; i++) {
            if (playerCounts[i]->positionX < teamPlayerCount->positionX) {
                break;
            }
        }
        if (i < 3) {
            playerIndex++;
            for (i = 0; i < 3; i++) {
                if (playerCounts[i]->positionZ > teamPlayerCount->positionZ) {
                    break;
                }
            }
            if (i < 3) {
                playerIndex++;
                for (i = 0; i < 3; i++) {
                    if (playerCounts[i]->positionZ < teamPlayerCount->positionZ) {
                        break;
                    }
                }
                if (i < 3) {
                    playerIndex++;
                }
            }
        }
    }
    activePlayerCount = 0;
    if (playerIndex >= 4) {
        activePlayerCount = 1;
    }
    return activePlayerCount;
}

float PlayerDirectionGet(PlayerState *player)
{
    s32 playerDirections[3];
    s32 candidateIndices[2];
    float candidateAngle;
    float playerDirection;
    float candidateAngles[2];
    PlayerState *candidatePlayers[3];
    s32 candidateCount;
    s32 playerIndex;
    s32 i;

    for (i = 0, candidateCount = 0; i < 4; i++) {
        if (i != player->playerIndex) {
            candidatePlayers[candidateCount++] = &playerStates[i];
        }
    }
    for (i = 0, playerIndex = 0; i < 3; i++) {
        candidateAngle = PlayerDistanceGet(candidatePlayers[i]->positionX, candidatePlayers[i]->positionZ, player);
        if (candidateAngle < player->collisionRadius + candidatePlayers[i]->collisionRadius + 100.0f) {
            playerDirections[i] = 1;
            playerIndex++;
        }
        else {
            playerDirections[i] = -1;
        }
    }
    playerDirection = -1.0f;
    switch (playerIndex) {
        case 1:
            for (i = 0; i < 3; i++) {
                if (playerDirections[i] != 0) {
                    break;
                }
            }
            playerDirection = AngleNormalize(atan2d(player->positionX - candidatePlayers[i]->positionX, player->positionZ - candidatePlayers[i]->positionZ));
            break;
        case 2:
            for (i = 0, candidateCount = 0; i < 3; i++) {
                if (playerDirections[i] == 0) {
                    candidateIndices[candidateCount++] = i;
                }
            }
            for (i = 0; i < 2; i++) {
                candidateAngles[i] = AngleNormalize(atan2d(player->positionX - candidatePlayers[candidateIndices[i]]->positionX, player->positionZ - candidatePlayers[candidateIndices[i]]->positionZ));
            }
            playerDirection = AngleOppositeGet(candidateAngles[0], candidateAngles[1]);
            break;
    }
    if (playerDirection != -1.0f)
        ;
    if (playerDirection != -1.0f) {
        playerDirection = AngleNormalize(playerDirection);
    }
    return playerDirection;
}

void PlayerRoundInitialize(PlayerState *player)
{
    PlayerState *firstOpponent;
    PlayerState *secondOpponent;
    PlayerState *opponents[3];
    Vec cornerTargets[3];
    Vec edgeTargets[3];
    s32 separatedPairFlags[3];
    s32 adjacentEdges[2];
    s32 edgeAvailability[2];
    s32 difficultyScore;
    float movementMagnitude;
    float positionSpread;
    s32 separatedPairCount;
    float savedTargetX;
    float savedTargetZ;
    float previousTargetX;
    float previousTargetZ;
    float targetAngle; // ! - uninitialized
    float targetX;
    float targetZ;
    float maximumPosition;
    float minimumPosition;
    float targetDistance;
    s32 targetIndex;
    s32 canMove;
    s32 opponentIndex;
    s32 opponentScanIndex;
    s32 edgeIndex;

    player->playerState = player->teamId = 0;
    player->verticalInput = player->difficulty = 0;
    player->buttonState = player->buttonFlags = 0;
    switch (GWPlayerCfg[player->playerIndex].diff) {
        case 0:
            difficultyScore = 60;
            break;
        case 1:
            difficultyScore = 80;
            break;
        case 2:
            difficultyScore = 90;
            break;
        default:
            difficultyScore = 95;
            break;
    }
    for (opponentScanIndex = 0, opponentIndex = 0; opponentScanIndex < 4; opponentScanIndex++) {
        if (opponentScanIndex != player->playerIndex) {
            opponents[opponentIndex++] = &playerStates[opponentScanIndex];
        }
    }
    for (opponentScanIndex = 0, minimumPosition = 10000.0f; opponentScanIndex < 3; opponentScanIndex++) {
        if (opponents[opponentScanIndex]->positionX < minimumPosition) {
            minimumPosition = opponents[opponentScanIndex]->positionX;
        }
    }
    for (opponentScanIndex = 0, maximumPosition = -10000.0f; opponentScanIndex < 3; opponentScanIndex++) {
        if (opponents[opponentScanIndex]->positionX > maximumPosition) {
            maximumPosition = opponents[opponentScanIndex]->positionX;
        }
    }
    positionSpread = ABS_INV(maximumPosition - minimumPosition) / 2.0f;
    targetX = maximumPosition - positionSpread;
    for (opponentScanIndex = 0, minimumPosition = 10000.0f; opponentScanIndex < 3; opponentScanIndex++) {
        if (opponents[opponentScanIndex]->positionZ < minimumPosition) {
            minimumPosition = opponents[opponentScanIndex]->positionZ;
        }
    }
    for (opponentScanIndex = 0, maximumPosition = -10000.0f; opponentScanIndex < 3; opponentScanIndex++) {
        if (opponents[opponentScanIndex]->positionZ > maximumPosition) {
            maximumPosition = opponents[opponentScanIndex]->positionZ;
        }
    }
    positionSpread = ABS_INV(maximumPosition - minimumPosition) / 2.0f;
    targetZ = maximumPosition - positionSpread;
    for (opponentScanIndex = 0, minimumPosition = 10000.0f; opponentScanIndex < 3; opponentScanIndex++) {
        targetDistance = PlayerDistanceGet(targetX, targetZ, opponents[opponentScanIndex]);
        if (targetDistance < minimumPosition) {
            minimumPosition = targetDistance;
        }
    }
    if (ActivePlayerCountGet() != 0) {
        player->playerEventFlags = 0;
        if (minimumPosition < 400.0f || (player->collisionFlags & 2)) {
            player->collisionFlags |= 2;
            PlayerTargetSelect(cornerTargets, opponents);
            if (player->targetPlayerIndex == -1) {
                for (opponentScanIndex = 0, separatedPairCount = 0, maximumPosition = 0.0f; opponentScanIndex < 3; opponentScanIndex++) {
                    firstOpponent = opponents[opponentScanIndex];
                    (opponentScanIndex >= 2) ? (opponentIndex = 0) : (opponentIndex = opponentScanIndex + 1);
                    secondOpponent = opponents[opponentIndex];
                    targetDistance = PlayerDistanceGet(firstOpponent->positionX, firstOpponent->positionZ, secondOpponent);
                    if (targetDistance > 700.0f) {
                        separatedPairFlags[opponentScanIndex] = 1;
                        separatedPairCount++;
                    }
                    else {
                        separatedPairFlags[opponentScanIndex] = 0;
                    }
                    if (targetDistance > maximumPosition) {
                        maximumPosition = targetDistance;
                        targetIndex = opponentScanIndex;
                        targetAngle = AngleNormalize(atan2d(cornerTargets[opponentScanIndex].x - player->positionX, cornerTargets[opponentScanIndex].z - player->positionZ));
                    }
                }
                if (separatedPairCount > 0) {
                    opponentIndex = frandmod(separatedPairCount);
                    for (opponentScanIndex = 0; opponentScanIndex < 3; opponentScanIndex++) {
                        if (separatedPairFlags[opponentScanIndex] != 0 && opponentIndex-- == 0) {
                            break;
                        }
                    }
                    targetIndex = opponentScanIndex;
                    targetAngle = AngleNormalize(atan2d(cornerTargets[opponentScanIndex].x - player->positionX, cornerTargets[opponentScanIndex].z - player->positionZ));
                }
                player->targetPlayerIndex = targetIndex;
                player->transitionProgress = targetAngle;
            }
            else {
                targetIndex = player->targetPlayerIndex;
                targetAngle = player->transitionProgress;
            }
            savedTargetX = targetX;
            savedTargetZ = targetZ;
            targetX = cornerTargets[targetIndex].x;
            targetZ = cornerTargets[targetIndex].z;
            if (PlayerDistanceGet(targetX, targetZ, player) <= 30.0f && !(player->collisionFlags & 1)) {
                player->transitionTimer = AngleNormalize(atan2d(targetX, targetZ));
                player->collisionFlags |= 1;
            }
            if (player->collisionFlags & 1) {
                targetX = 800.0 * sind(player->transitionTimer);
                targetZ = 800.0 * cosd(player->transitionTimer);
            }
        }
    }
    else {
        player->playerActionState = 0;
        player->collisionFlags = 0;
        player->targetPlayerIndex = -1;
        if ((minimumPosition >= 600.0f && PlayerTargetFind(targetX, targetZ, player, &opponents[0]) != 0) || (player->playerEventFlags & 0x10)) {
            player->playerEventFlags |= 0x10;
            PlayerTargetSelect(edgeTargets, opponents);
            for (opponentScanIndex = 0, targetIndex = 0, minimumPosition = 10000.0f; opponentScanIndex < 3; opponentScanIndex++) {
                targetDistance = PlayerDistanceGet(edgeTargets[opponentScanIndex].x, edgeTargets[opponentScanIndex].z, player);
                if (targetDistance < minimumPosition) {
                    minimumPosition = targetDistance;
                    targetIndex = opponentScanIndex;
                }
            }
            previousTargetX = targetX;
            previousTargetZ = targetZ;
            targetX = edgeTargets[targetIndex].x;
            targetZ = edgeTargets[targetIndex].z;
            if (PlayerDistanceGet(targetX, targetZ, player) <= 30.0f && !(player->playerEventFlags & 0x20)) {
                player->transitionTimer = AngleNormalize(targetAngle - 90.0f);
                player->playerEventFlags |= 0x20;
            }
            if (player->playerEventFlags & 0x20) {
                targetX = 800.0 * sind(player->transitionTimer);
                targetZ = 800.0 * cosd(player->transitionTimer);
                if (PlayerNearArenaEdgeCheck(player, 10.0f)) {
                    player->playerEventFlags &= ~0x20;
                }
                else {
                    player->playerEventFlags |= 0x20;
                }
            }
        }
        else {
            if (player->positionX >= 0.0f && player->positionZ < 0.0f) {
                adjacentEdges[0] = 0;
                adjacentEdges[1] = 1;
            }
            if (player->positionX < 0.0f && player->positionZ < 0.0f) {
                adjacentEdges[0] = 3;
                adjacentEdges[1] = 0;
            }
            if (player->positionX >= 0.0f && player->positionZ >= 0.0f) {
                adjacentEdges[0] = 1;
                adjacentEdges[1] = 2;
            }
            if (player->positionX < 0.0f && player->positionZ >= 0.0f) {
                adjacentEdges[0] = 2;
                adjacentEdges[1] = 3;
            }
            for (opponentScanIndex = 0; opponentScanIndex < 2; opponentScanIndex++) {
                canMove = 0;
                edgeIndex = adjacentEdges[opponentScanIndex];
                switch (edgeIndex) {
                    case 0:
                        for (opponentIndex = 0; opponentIndex < 3; opponentIndex++) {
                            if (opponents[opponentIndex]->positionZ < -800.0f + opponents[opponentIndex]->collisionRadius + 150.0f) {
                                break;
                            }
                        }
                        if (opponentIndex >= 3) {
                            canMove = 1;
                        }
                        break;
                    case 1:
                        for (opponentIndex = 0; opponentIndex < 3; opponentIndex++) {
                            if (opponents[opponentIndex]->positionX > 800.0f - opponents[opponentIndex]->collisionRadius - 150.0f) {
                                break;
                            }
                        }
                        if (opponentIndex >= 3) {
                            canMove = 1;
                        }
                        break;
                    case 2:
                        for (opponentIndex = 0; opponentIndex < 3; opponentIndex++) {
                            if (opponents[opponentIndex]->positionZ > 800.0f - opponents[opponentIndex]->collisionRadius - 150.0f) {
                                break;
                            }
                        }
                        if (opponentIndex >= 3) {
                            canMove = 1;
                        }
                        break;
                    case 3:
                        for (opponentIndex = 0; opponentIndex < 3; opponentIndex++) {
                            if (opponents[opponentIndex]->positionX < -800.0f + opponents[opponentIndex]->collisionRadius + 150.0f) {
                                break;
                            }
                        }
                        if (opponentIndex >= 3) {
                            canMove = 1;
                        }
                        break;
                }
                edgeAvailability[opponentScanIndex] = canMove;
            }
            canMove = 0;
            if ((player->playerEventFlags & 4) || (player->playerEventFlags & 8)) {
                if ((player->playerEventFlags & 4) && edgeAvailability[0] != 0) {
                    canMove = 1;
                }
                if ((player->playerEventFlags & 8) && edgeAvailability[1] != 0) {
                    canMove = 1;
                }
            }
            if (player->transitionState > 0) {
                canMove = 1;
                player->transitionState--;
            }
            if (canMove == 0) {
                player->playerEventFlags &= ~0xC;
                if (edgeAvailability[0] != 0 && edgeAvailability[1] != 0) {
                    if (frandmod(2) != 0) {
                        player->playerEventFlags |= 4;
                        player->transitionState = 30;
                    }
                    else {
                        player->playerEventFlags |= 8;
                        player->transitionState = 30;
                    }
                }
                else if (edgeAvailability[0] != 0) {
                    player->playerEventFlags |= 4;
                    player->transitionState = 30;
                }
                else if (edgeAvailability[1] != 0) {
                    player->playerEventFlags |= 8;
                    player->transitionState = 30;
                }
                else {
                    player->playerEventFlags |= 0x10;
                }
            }
            if ((player->playerEventFlags & 4) || (player->playerEventFlags & 8)) {
                targetAngle = AngleNormalize(atan2d(player->positionX, player->positionZ));
                if (player->playerEventFlags & 4) {
                    targetAngle += 10.0f;
                }
                else {
                    targetAngle -= 10.0f;
                }
                targetX = 800.0 * sind(targetAngle);
                targetZ = 800.0 * cosd(targetAngle);
            }
            else {
                targetX = player->positionX;
                targetZ = player->positionZ;
            }
        }
    }
    targetDistance = PlayerDistanceGet(targetX, targetZ, player);
    if (targetDistance > 100.0f || player->playerActionState == 1) {
        if (player->playerActionState != 1) {
            player->playerActionState = 1;
        }
        if (targetDistance < 20.0f) {
            player->playerActionState = 0;
            return;
        }
        if (targetDistance < 30.0f) {
            movementMagnitude = 10.0f;
        }
        else {
            movementMagnitude = 50.0f;
        }
        targetAngle = AngleNormalize(atan2d(targetX - player->positionX, targetZ - player->positionZ));
        player->playerState = movementMagnitude * sind(targetAngle);
        player->teamId = movementMagnitude * -cosd(targetAngle);
    }
}

void PlayerProcess(void)
{
    PlayerState *playerState;
    PlayerState *targetPlayer;
    Vec movementInput;
    float initialPlayerHeight;
    float launchAngleStep;
    float turnSpeed;
    float projectedPositionX;
    float projectedPositionZ;
    float collisionDirection;
    float launchAngle;
    s32 motionPaused;
    s32 animationFrames;

    playerState = HuPrcCurrentGet()->user_data;
    playerState->effectState = 0;
    playerState->resultState = 0;
    playerState->playerMotionState = 0;
    playerState->effectTimer = 0;
    playerState->targetPlayerIndex = -1;
    while (minigameState == 0) {
        HuPrcVSleep();
    }
    minigameState = 2;
    while (minigameState == 2) {
        HuPrcVSleep();
    }
    while (TRUE) {
        playerState->rotationX = playerState->positionX;
        playerState->rotationY = playerState->positionZ;
        if (GWPlayerCfg[playerState->playerIndex].iscom == 1) {
            PlayerRoundInitialize(playerState);
        }
        if (playerState->motionAttributes != 0 && playerState->motionSpeed-- == 0) {
            playerState->motionAttributes = 0;
            playerState->movementAcceleration = 1.0f;
            snapshotCaptureEnabled = 0;
        }
        if (playerState->resultState != 0) {
            break;
        }
        switch (playerState->effectState) {
            case 4:
                omVibrate(playerState->playerIndex, 12, 6, 6);
                CharMotionSpeedSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], 1.0f);
                CharMotionSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], playerState->modelMotionIds[6]);
                Hu3DModelAttrReset(playerState->characterModel, HU3D_MOTATTR_LOOP);
                playerState->motionFrame = 60;
                playerState->effectState = 5;
                break;
            case 5:
                if (playerState->motionFrame != 0) {
                    if (playerState->motionFrame == 20) {
                        PlayerFXPlay(playerState->playerIndex, 0x119);
                    }
                    playerState->motionFrame--;
                }
                else {
                    CharMotionShiftSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], playerState->modelMotionIds[7], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
                    playerState->motionFrame = 20;
                    playerState->effectState = 6;
                }
                break;
            case 6:
                if (playerState->motionFrame != 0) {
                    Hu3DModelAttrReset(playerState->characterModel, HU3D_MOTATTR_LOOP);
                    playerState->motionFrame--;
                }
                else if (Hu3DMotionEndCheck(playerState->characterModel) == 1) {
                    CharMotionShiftSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], playerState->modelMotionIds[4], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                    playerState->motionFrame = 10;
                    playerState->effectState = 7;
                }
                break;
            case 7:
                if (playerState->motionFrame != 0) {
                    playerState->motionFrame--;
                }
                else {
                    CharMotionSpeedSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], 2.0f);
                    playerState->effectState = 0;
                }
                break;
            default:
                HuSetVecF(&movementInput, PlayerStickAxisGet(playerState->playerIndex, 0) / 4.0f, 0.0, -PlayerStickAxisGet(playerState->playerIndex, 1) / 4.0f);
                playerState->movementSpeedLimit = VECMagXYZ(&movementInput);
                if (playerState->movementSpeedLimit >= 0.5f) {
                    CharMotionSpeedSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], 1.0f);
                    if (playerState->movementSpeedLimit >= 10.0f) {
                        movementInput.x /= playerState->movementSpeedLimit;
                        movementInput.z /= playerState->movementSpeedLimit;
                        playerState->movementSpeedLimit = 10.0f;
                        movementInput.x = 10.0f * movementInput.x;
                        movementInput.z = 10.0f * movementInput.z;
                    }
                    playerState->turnAngle = AngleApproach(playerState->turnAngle, atan2d(movementInput.x, movementInput.z), 0.4f);
                    if (playerState->movementSpeedLimit >= 8.0f) {
                        if (playerState->effectState != 3) {
                            playerState->effectState = 3;
                            CharMotionShiftSet(
                                characterModelIds[GWPlayerCfg[playerState->playerIndex].character], playerState->modelMotionIds[2], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                        }
                    }
                    else if (playerState->effectState != 2) {
                        playerState->effectState = 2;
                        CharMotionShiftSet(
                            characterModelIds[GWPlayerCfg[playerState->playerIndex].character], playerState->modelMotionIds[1], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                    }
                    playerState->positionX += movementInput.x * playerState->movementAcceleration;
                    playerState->positionZ += movementInput.z * playerState->movementAcceleration;
                }
                else if (playerState->effectState != 0) {
                    playerState->effectState = 0;
                    CharMotionSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], playerState->modelMotionIds[4]);
                    CharMotionSpeedSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], 2.0f);
                    Hu3DModelAttrSet(playerState->characterModel, HU3D_MOTATTR_LOOP);
                }
                break;
        }
        if ((playerState->motionFlags = NearbyPlayerGet(playerState, 0.0f, 0)) != -1) {
            playerState->resultState = 1;
            break;
        }
        if ((playerState->motionFlags = PlayerArenaCollisionCheck(playerState)) != -1) {
            playerState->resultState = 1;
            break;
        }
        Hu3DModelPosSet(playerState->characterModel, playerState->positionX, playerState->positionY, playerState->positionZ);
        Hu3DModelRotSet(playerState->characterModel, playerState->rotationZ, playerState->turnAngle, playerState->turnSpeed);
        HuPrcVSleep();
    }
    snapshotCaptureEnabled = 0;
    if (winningPlayerIndex == humanPlayerIndex) {
        turnSpeed = 10.0f;
        CharMotionSpeedSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], 1.0f);
        Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        animationFrames = (360.0f - AngleNormalize(playerState->turnAngle)) / turnSpeed;
        if (playerState->turnAngle < 180.0f) {
            animationFrames = playerState->turnAngle / turnSpeed;
            turnSpeed = -turnSpeed;
        }
        for (; animationFrames != 0; animationFrames--) {
            playerState->turnAngle += turnSpeed;
            Hu3DModelRotSet(playerState->characterModel, playerState->rotationZ, playerState->turnAngle, playerState->turnSpeed);
            HuPrcVSleep();
        }
        Hu3DModelRotSet(playerState->characterModel, playerState->rotationZ, 0.0f, playerState->turnSpeed);
    }
    else {
        omVibrate(playerState->playerIndex, 12, 4, 2);
        PlayerFXPlay(playerState->playerIndex, 0x123);
        HuAudFXPlay(0x5EC);
        targetPlayer = &playerStates[playerState->motionFlags];
        playerState->movementSpeed = atan2d(playerState->positionX - targetPlayer->positionX, playerState->positionZ - targetPlayer->positionZ);
        animationFrames = 0;
        projectedPositionX = playerState->positionX;
        projectedPositionZ = playerState->positionZ;
        while (TRUE) {
            projectedPositionX += 20.0 * sind(playerState->movementSpeed);
            projectedPositionZ += 20.0 * cosd(playerState->movementSpeed);
            if (ABS_INV(projectedPositionX) > 1200.0f || projectedPositionZ > 1200.0f || projectedPositionZ < -2000.0f) {
                break;
            }
            animationFrames++;
        }
        collisionDirection = AngleNormalize(atan2d(playerState->positionX - targetPlayer->positionX, playerState->positionZ - targetPlayer->positionZ));
        if (ABS_INV(collisionDirection - playerState->turnAngle) > 90.0f && ABS_INV(collisionDirection - playerState->turnAngle) < 270.0f) {
            CharMotionShiftSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], playerState->secondaryMotionId, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
        }
        else {
            CharMotionShiftSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], playerState->tertiaryMotionId, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
        }
        initialPlayerHeight = playerState->positionY;
        launchAngle = 0.0f;
        launchAngleStep = 180.0f / animationFrames;
        motionPaused = 0;
        for (; animationFrames != 0; animationFrames--) {
            playerState->positionX += 20.0 * sind(playerState->movementSpeed);
            playerState->positionY = 700.0 * sind(launchAngle);
            playerState->positionZ += 20.0 * cosd(playerState->movementSpeed);
            launchAngle += launchAngleStep;
            Hu3DModelPosSet(playerState->characterModel, playerState->positionX, playerState->positionY, playerState->positionZ);
            if (Hu3DMotionTimeGet(playerState->characterModel) == 20.0f && motionPaused == 0) {
                motionPaused = 1;
                Hu3DModelAttrSet(playerState->characterModel, HU3D_MOTATTR_PAUSE);
            }
            HuPrcVSleep();
        }
        Hu3DModelPosSet(playerState->characterModel, playerState->positionX, -1000.0f, playerState->positionZ);
        Hu3DModelAttrSet(playerState->characterModel, 1);
    }
    while (TRUE) {
        HuPrcVSleep();
    }
}

float ArenaPositionLimitAdjust(s32 playerIndex, float position, float radius)
{
    float positionLimit;
    float adjustedPosition;

    positionLimit = position - AngleNormalize(radius);
    if (positionLimit < 0.0f || ABS_INV(positionLimit) > 180.0f) {
        if (ABS_INV(positionLimit) > 180.0f) {
            adjustedPosition = (360.0f - ABS_INV(positionLimit)) / playerIndex;
            if (positionLimit > 0.0f) {
                adjustedPosition = -adjustedPosition;
            }
        }
        else {
            adjustedPosition = positionLimit / playerIndex;
        }
    }
    else {
        adjustedPosition = positionLimit / playerIndex;
    }
    return adjustedPosition;
}

void PlayerInputProcess(PlayerState *player)
{
    PlayerState simulatedPlayer;
    float opponentAngles[3];
    float playerAngles[2];
    Vec targetPosition;
    Vec referencePosition;
    Vec positionDifference;
    float candidateDistance;
    float nearestDistance;
    float alternateTargetX;
    float alternateTargetZ;
    float humanPlayerDistance;
    float targetAngle;
    s32 nearestPlayerIndex;
    float playerDistance;
    float nearestPlayerDistance;
    float targetX;
    float targetZ;
    float targetAngle;
    s32 nearerComputerCount;
    s32 collidingPlayerIndex;
    PlayerState *humanPlayer;
    PlayerState *simulatedPlayer;
    PlayerState *candidatePlayer;
    s32 difficultyThreshold;
    s32 nextCollidingPlayer;
    s32 nearestOpponentIndex;
    s32 angleCount;
    s32 i;

    player->playerState = player->teamId = 0;
    player->verticalInput = player->difficulty = 0;
    player->buttonState = player->buttonFlags = 0;
    switch (GWPlayerCfg[player->playerIndex].diff) {
        case 0:
            difficultyThreshold = 60;
            break;
        case 1:
            difficultyThreshold = 80;
            break;
        case 2:
            difficultyThreshold = 90;
            break;
        default:
            difficultyThreshold = 95;
            break;
    }
    humanPlayer = &playerStates[humanPlayerIndex];
    simulatedPlayer = &simulatedPlayer;
    for (i = 0, angleCount = 0; i < 4; i++) {
        if (i != humanPlayerIndex && i != player->playerIndex) {
            candidatePlayer = &playerStates[i];
            opponentAngles[angleCount] = AngleNormalize(atan2d(candidatePlayer->positionX - humanPlayer->positionX, candidatePlayer->positionZ - humanPlayer->positionZ));
            angleCount++;
        }
    }
    if (opponentAngles[0] > opponentAngles[1]) {
        opponentAngles[2] = AngleNormalize(opponentAngles[0] - opponentAngles[1]);
        if (opponentAngles[2] < 180.0f) {
            opponentAngles[2] = AngleNormalize(opponentAngles[1] + opponentAngles[2] + 180.0f);
        }
        else {
            opponentAngles[2] = AngleNormalize(opponentAngles[1] - opponentAngles[2]);
        }
    }
    else {
        opponentAngles[2] = AngleNormalize(opponentAngles[1] - opponentAngles[0]);
        if (opponentAngles[2] < 180.0f) {
            opponentAngles[2] = AngleNormalize(opponentAngles[0] + opponentAngles[2] + 180.0f);
        }
        else {
            opponentAngles[2] = AngleNormalize(opponentAngles[0] - opponentAngles[2]);
        }
    }
    candidatePlayer = &playerStates[player->playerIndex];
    HuSetVecF(&targetPosition, candidatePlayer->positionX, candidatePlayer->positionY, candidatePlayer->positionZ);
    HuSetVecF(&referencePosition, humanPlayer->positionX, humanPlayer->positionY, humanPlayer->positionZ);
    HuSubVecF(&positionDifference, &targetPosition, &referencePosition);
    humanPlayerDistance = VECMagXYZ(&positionDifference);
    for (i = 0, nearerComputerCount = 0; i < 4; i++) {
        if (i != humanPlayerIndex && i != player->playerIndex && GWPlayerCfg[i].iscom) {
            candidatePlayer = &playerStates[i];
            HuSetVecF(&targetPosition, candidatePlayer->positionX, candidatePlayer->positionY, candidatePlayer->positionZ);
            HuSetVecF(&referencePosition, humanPlayer->positionX, humanPlayer->positionY, humanPlayer->positionZ);
            HuSubVecF(&positionDifference, &targetPosition, &referencePosition);
            candidateDistance = VECMagXYZ(&positionDifference);
            if (candidateDistance < humanPlayerDistance) {
                nearerComputerCount++;
            }
        }
    }
    if (player->playerActionState != 2) {
        if (ActivePlayerCountGet() != 0) {
            player->playerActionState = 0;
        }
        else {
            if (ABS_INV(humanPlayer->positionX) > 600.0f && ABS_INV(humanPlayer->positionZ) > 600.0f) {
                if (nearerComputerCount <= 0) {
                    player->playerActionState = 0;
                }
                else {
                    player->playerActionState = 2;
                }
            }
            else {
                if (nearerComputerCount <= 0) {
                    player->playerActionState = 0;
                }
                else {
                    player->playerActionState = 1;
                }
            }
        }
    }
    if (player->playerActionState != 3 && player->sequenceId > 180) {
        player->playerActionState = 3;
        player->sequenceState = frandmod(REFRESH_RATE * 2) + REFRESH_RATE * 2;
    }
    switch (player->playerActionState) {
        case 0:
            targetX = humanPlayer->positionX;
            targetZ = humanPlayer->positionZ;
            targetAngle = AngleNormalize(atan2d(targetX - player->positionX, targetZ - player->positionZ));
            break;
        case 1:
            targetX = targetZ = alternateTargetX = alternateTargetZ = 0.0f;
            if (humanPlayer->positionX >= 300.0f && humanPlayer->positionZ < -300.0f) {
                targetX = 550.0f;
                targetZ = -100.0f;
                alternateTargetX = 100.0f;
                alternateTargetZ = 550.0f;
            }
            if (humanPlayer->positionX < -300.0f && humanPlayer->positionZ < -300.0f) {
                targetX = -550.0f;
                targetZ = -100.0f;
                alternateTargetX = -100.0f;
                alternateTargetZ = 550.0f;
            }
            if (humanPlayer->positionX >= 300.0f && humanPlayer->positionZ >= 300.0f) {
                targetX = 550.0f;
                targetZ = -100.0f;
                alternateTargetX = 100.0f;
                alternateTargetZ = -550.0f;
            }
            if (humanPlayer->positionX < -300.0f && humanPlayer->positionZ >= -300.0f) {
                targetX = -550.0f;
                targetZ = -100.0f;
                alternateTargetX = -100.0f;
                alternateTargetZ = -550.0f;
            }
            if (humanPlayer->positionX > 300.0f && humanPlayer->positionZ > -300.0f && humanPlayer->positionZ < 300.0f) {
                targetX = 200.0f;
                targetZ = 550.0f;
                alternateTargetX = 200.0f;
                alternateTargetZ = -550.0f;
            }
            if (humanPlayer->positionX < -300.0f && humanPlayer->positionZ > -300.0f && humanPlayer->positionZ < 300.0f) {
                targetX = -200.0f;
                targetZ = 550.0f;
                alternateTargetX = -200.0f;
                alternateTargetZ = -550.0f;
            }
            if (humanPlayer->positionX < 300.0f && humanPlayer->positionX > -300.0f && humanPlayer->positionZ < -300.0f) {
                targetX = 550.0f;
                targetZ = -200.0f;
                alternateTargetX = -550.0f;
                alternateTargetZ = -200.0f;
            }
            if (humanPlayer->positionX < 300.0f && humanPlayer->positionX > -300.0f && humanPlayer->positionZ > 300.0f) {
                targetX = 550.0f;
                targetZ = 200.0f;
                alternateTargetX = -550.0f;
                alternateTargetZ = 200.0f;
            }
            for (i = 0, nearestOpponentIndex = -1, nearestDistance = 10000.0f; i < 4; i++) {
                if (i != humanPlayerIndex) {
                    candidatePlayer = &playerStates[i];
                    HuSetVecF(&targetPosition, humanPlayer->positionX, 0.0, humanPlayer->positionZ);
                    HuSetVecF(&referencePosition, candidatePlayer->positionX, 0.0, candidatePlayer->positionZ);
                    HuSubVecF(&positionDifference, &targetPosition, &referencePosition);
                    candidateDistance = VECMagXZ(&positionDifference);
                    if (candidateDistance < nearestDistance) {
                        nearestDistance = candidateDistance;
                        nearestOpponentIndex = i;
                    }
                }
            }
            for (i = 0, angleCount = -1, nearestDistance = 10000.0f; i < 4; i++) {
                if (i != humanPlayerIndex && i != nearestOpponentIndex) {
                    candidatePlayer = &playerStates[i];
                    HuSetVecF(&targetPosition, humanPlayer->positionX, 0.0, humanPlayer->positionZ);
                    HuSetVecF(&referencePosition, candidatePlayer->positionX, 0.0, candidatePlayer->positionZ);
                    HuSubVecF(&positionDifference, &targetPosition, &referencePosition);
                    candidateDistance = VECMagXZ(&positionDifference);
                    if (candidateDistance < nearestDistance) {
                        nearestDistance = candidateDistance;
                        angleCount = i;
                    }
                }
            }
            if (player->playerIndex != angleCount) {
                targetX = alternateTargetX;
                targetZ = alternateTargetZ;
            }
            targetAngle = AngleNormalize(atan2d(targetX - player->positionX, targetZ - player->positionZ));
            break;
        case 2:
            for (i = 0, angleCount = 0; i < 4; i++) {
                if (i != humanPlayerIndex && i != player->playerIndex) {
                    candidatePlayer = &playerStates[i];
                    playerAngles[angleCount++] = AngleNormalize(atan2d(candidatePlayer->positionX - player->positionX, candidatePlayer->positionZ - player->positionZ));
                }
            }
            targetAngle = AngleNormalize(ABS_INV(playerAngles[0] - playerAngles[1]) / 2.0f);
            if (targetAngle < 90.0f) {
                targetAngle += 180.0f;
            }
            if (playerAngles[0] < playerAngles[1]) {
                targetAngle += playerAngles[0];
            }
            else {
                targetAngle += playerAngles[1];
            }
            targetAngle = AngleNormalize(targetAngle);
            if (sqrtf(player->positionX * player->positionX + player->positionZ * player->positionZ) < 500.0f) {
                candidateDistance = 600.0f;
                targetX = player->positionX + candidateDistance * sind(targetAngle);
                targetZ = player->positionZ + candidateDistance * cosd(targetAngle);
                targetAngle = AngleNormalize(atan2d(targetX - player->positionX, targetZ - player->positionZ));
            }
            else {
#ifdef NON_MATCHING
                // TODO the value is slightly higher in the emulator
                targetZ = 0.0f;
                targetX = 0.0f;
                targetAngle = 0.0f;
#endif
                player->playerActionState = 1;
            }
            break;
        case 3:
            for (i = 0, nearestPlayerDistance = 10000.0f; i < 4; i++) {
                if (i != humanPlayerIndex && i != player->playerIndex) {
                    candidatePlayer = &playerStates[i];
                    playerDistance = PlayerDistanceGet(candidatePlayer->positionX, candidatePlayer->positionZ, player);
                    if (playerDistance < nearestPlayerDistance) {
                        nearestPlayerDistance = playerDistance;
                        nearestPlayerIndex = i;
                    }
                }
            }
            candidatePlayer = &playerStates[nearestPlayerIndex];
            targetAngle = AngleNormalize(180.0 + atan2d(candidatePlayer->positionX - player->positionX, candidatePlayer->positionZ - player->positionZ));
            targetX = player->positionX + 100.0 * sind(targetAngle);
            targetZ = player->positionZ + 100.0 * cosd(targetAngle);
            if (player->sequenceState-- == 0) {
                player->sequenceId = 0;
                player->playerActionState = 1;
            }
            break;
    }
    simulatedPlayer->movementSpeed = 0.0f;
    if (ABS_INV(player->turnAngle - targetAngle) > 30.0f && ABS_INV(player->turnAngle - targetAngle) < 330.0f) {
        if (ABS_INV(AngleNormalize(player->turnAngle - targetAngle)) > 180.0f) {
            player->playerState = -30;
            simulatedPlayer->movementSpeed = player->rotationSpeed;
        }
        else {
            player->playerState = 30;
            simulatedPlayer->movementSpeed = -player->rotationSpeed;
        }
    }
    if (player->effectFlags == 0) {
        HuSetVecF(&targetPosition, targetX, 0.0, targetZ);
        HuSetVecF(&referencePosition, player->positionX, 0.0, player->positionZ);
        HuSubVecF(&positionDifference, &targetPosition, &referencePosition);
        if (VECMagXZ(&positionDifference) > 10.0f) {
            if (ABS_INV(player->turnAngle - targetAngle) < 90.0f || ABS_INV(player->turnAngle - targetAngle) > 270.0f) {
                player->buttonState |= 0x100;
            }
            else {
                player->buttonState |= 0x200;
                simulatedPlayer->movementSpeed = -simulatedPlayer->movementSpeed;
            }
        }
    }
    simulatedPlayer->playerIndex = player->playerIndex;
    simulatedPlayer->positionX = player->positionX;
    simulatedPlayer->positionY = player->positionY;
    simulatedPlayer->positionZ = player->positionZ;
    simulatedPlayer->movementSpeedLimit = player->movementSpeedLimit;
    simulatedPlayer->movementSpeed = AngleNormalize(player->movementSpeed + simulatedPlayer->movementSpeed);
    simulatedPlayer->collisionRadius = playerStates[humanPlayerIndex].collisionRadius + player->collisionRadius + player->movementSpeedLimit;
    collidingPlayerIndex = NearbyPlayerGet(simulatedPlayer, player->movementSpeedLimit, 0);
    if (collidingPlayerIndex != -1 && collidingPlayerIndex != humanPlayerIndex) {
        if (player->buttonState & 0x100) {
            simulatedPlayer->positionX += simulatedPlayer->movementSpeedLimit * sind(simulatedPlayer->movementSpeed);
            simulatedPlayer->positionZ += simulatedPlayer->movementSpeedLimit * cosd(simulatedPlayer->movementSpeed);
        }
        if (player->buttonState & 0x200) {
            simulatedPlayer->positionX -= simulatedPlayer->movementSpeedLimit * sind(simulatedPlayer->movementSpeed);
            simulatedPlayer->positionZ -= simulatedPlayer->movementSpeedLimit * cosd(simulatedPlayer->movementSpeed);
        }
        candidatePlayer = &playerStates[collidingPlayerIndex];
        HuSetVecF(&targetPosition, player->positionX, 0.0, player->positionZ);
        HuSetVecF(&referencePosition, candidatePlayer->positionX, 0.0, candidatePlayer->positionZ);
        HuSubVecF(&positionDifference, &targetPosition, &referencePosition);
        candidateDistance = VECMagXYZ(&positionDifference);
        HuSetVecF(&targetPosition, simulatedPlayer->positionX, 0.0, simulatedPlayer->positionZ);
        HuSetVecF(&referencePosition, candidatePlayer->positionX, 0.0, candidatePlayer->positionZ);
        HuSubVecF(&positionDifference, &targetPosition, &referencePosition);
        nearestDistance = VECMagXYZ(&positionDifference);
        nextCollidingPlayer = NearbyPlayerGet(simulatedPlayer, 0.0f, 0);
        if (candidateDistance > nearestDistance || (collidingPlayerIndex != nextCollidingPlayer && nextCollidingPlayer != -1)) {
            player->playerState = 30;
            player->buttonState = 0;
            player->playerActionState = 2;
            player->playerMotionState = REFRESH_RATE * 5;
            if (player->effectFlags == 0) {
                player->effectFlags = 60;
            }
        }
        else if (nextCollidingPlayer == -1) {
            player->playerActionState = 2;
            if (ABS_INV(player->turnAngle - targetAngle) < 90.0f || ABS_INV(player->turnAngle - targetAngle) > 270.0f) {
                player->buttonState |= 0x100;
            }
            else {
                player->buttonState |= 0x200;
                simulatedPlayer->movementSpeed = -simulatedPlayer->movementSpeed;
            }
        }
        else {
            player->playerState = 30;
        }
    }
    if (player->effectFlags != 0) {
        player->effectFlags--;
    }
}

void EffectProcess(void)
{
    PlayerState *playerState;
    s32 i;

    playerState = HuPrcCurrentGet()->user_data;
    for (i = 0; i < 20; i++) {
        playerState->startRotationZ -= 4.0f;
        Hu3DModelPosSet(playerState->shadowModel, playerState->startRotationX, playerState->startRotationY, playerState->startRotationZ);
        HuPrcVSleep();
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

s32 PlayerResultCheck(PlayerState *player)
{
    s32 playerState;
    s32 resultState;

    playerState = 1;
    resultState = 1;
    if (player->effectState != 4 && player->effectState != 5) {
        resultState = 0;
    }
    if (resultState == 0 && player->effectState != 6) {
        playerState = 0;
    }
    return playerState;
}

void OpponentProcess(void)
{
    float shadowHeight;
    float modelTransparency;
    float turnStep;
    PlayerState *playerState;
    PlayerState *collidedPlayer;
    s32 movementSoundId;
    s32 movementSoundPlaying;
    s32 rotationFrames;
    s32 collidingPlayerIndex;
    HUPROCESS *fadeProcess;
    s32 firstSoundFrame;
    s32 secondSoundFrame;
    s32 blinkFrame;
    s32 blinkPhase;
    HUPROCESS *effectProcess;

    playerState = HuPrcCurrentGet()->user_data;
    playerState->effectState = 0;
    playerState->playerMotionState = playerState->playerActionState = playerState->playerEventFlags = playerState->collisionFlags = playerState->effectFlags = playerState->animationFrame = 0;
    playerState->transitionState = 0;
    playerState->effectTimer = 0;
    playerState->sequenceId = 0;
    while (minigameState == 0) {
        HuPrcVSleep();
    }
    minigameState = 2;
    while (minigameState == 2) {
        HuPrcVSleep();
    }
    playerState->fadeProgress = playerState->positionX;
    playerState->fadeSpeed = playerState->positionZ;
    movementSoundPlaying = 0;
    while (TRUE) {
        playerState->rotationX = playerState->positionX;
        playerState->rotationY = playerState->positionZ;
        if (GWPlayerCfg[playerState->playerIndex].iscom == 1) {
            PlayerInputProcess(playerState);
        }
        if (playerState->motionAttributes != 0) {
            if (playerState->motionSpeed == 300) {
                fadeProcess = HuPrcChildCreate(PlayerModelFadeProcess, 0x2000, 0x1000, 0, HuPrcCurrentGet());
                fadeProcess->user_data = (void *)playerState->playerIndex;
            }
            if (playerState->motionSpeed-- == 0) {
                playerState->motionAttributes = 0;
                playerState->movementSpeedLimit = 2.25f;
                playerState->rotationSpeed = 0.66f;
            }
        }
        if (playerState->resultState != 0) {
            break;
        }
        switch (playerState->effectState) {
            case 8:
                if (playerState->motionFrame != 0) {
                    playerState->motionFrame--;
                }
                else {
                    Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
                    playerState->effectState = 0;
                }
                break;
            case 4:
                omVibrate(playerState->playerIndex, 12, 4, 2);
                Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[4], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
                playerState->motionFrame = 90;
                playerState->characterAnimationFlags = 30;
                playerState->effectState = 5;
                break;
            case 5:
                if (playerState->characterAnimationFlags-- == 0) {
                    HuAudFXPlay(0x5E7);
                    playerState->previousPositionX = playerState->positionX;
                    playerState->previousPositionY = playerState->positionY;
                    playerState->previousPositionZ = playerState->positionZ;
                    playerState->effectTimer |= 1;
                }
                if (playerState->motionFrame != 0) {
                    playerState->motionFrame--;
                }
                else {
                    Hu3DMotionSet(playerState->characterModel, playerState->modelMotionIds[5]);
                    Hu3DModelAttrReset(playerState->characterModel, HU3D_MOTATTR_LOOP);
                    playerState->effectState = 6;
                }
                break;
            case 6:
                if (Hu3DMotionEndCheck(playerState->characterModel) == 1) {
                    Hu3DMotionSet(playerState->characterModel, playerState->modelMotionIds[0]);
                    playerState->effectState = 0;
                }
                break;
            default:
                if (PlayerStickAxisGet(playerState->playerIndex, 0) > 20) {
                    playerState->movementSpeed -= playerState->rotationSpeed;
                    if (playerState->movementSpeed < 0.0f) {
                        playerState->movementSpeed += 360.0f;
                    }
                    playerState->turnAngle = playerState->movementSpeed;
                }
                if (PlayerStickAxisGet(playerState->playerIndex, 0) < -20) {
                    playerState->movementSpeed += playerState->rotationSpeed;
                    if (playerState->movementSpeed > 360.0f) {
                        playerState->movementSpeed -= 360.0f;
                    }
                    playerState->turnAngle = playerState->movementSpeed;
                }
                if (PlayerButtonsGet(playerState->playerIndex) & 0x100) {
                    playerState->positionX += playerState->movementSpeedLimit * sind(playerState->movementSpeed);
                    playerState->positionZ += playerState->movementSpeedLimit * cosd(playerState->movementSpeed);
                }
                if (PlayerButtonsGet(playerState->playerIndex) & 0x200) {
                    playerState->positionX -= playerState->movementSpeedLimit * sind(playerState->movementSpeed);
                    playerState->positionZ -= playerState->movementSpeedLimit * cosd(playerState->movementSpeed);
                }
                if (PlayerButtonsGet(playerState->playerIndex) & 0x300) {
                    if (playerState->motionAttributes != 1) {
                        if (playerState->effectState != 2) {
                            playerState->effectState = 2;
                            if (PlayerButtonsGet(playerState->playerIndex) & 0x100) {
                                Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[1], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                                Hu3DMotionShiftSpeedSet(playerState->characterModel, 2.0f);
                            }
                            else {
                                Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[1], 0.0f, 8.0f, HU3D_MOTATTR_LOOP | HU3D_MOTATTR_REV);
                                Hu3DMotionShiftSpeedSet(playerState->characterModel, 2.0f);
                            }
                            if (movementSoundPlaying != 0) {
                                movementSoundPlaying = 0;
                                HuAudFXStop(movementSoundId);
                            }
                        }
                    }
                    else {
                        if (playerState->effectState != 3) {
                            playerState->effectState = 3;
                            playerState->animationFrame = 0;
                            if (PlayerButtonsGet(playerState->playerIndex) & 0x100) {
                                Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[2], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                                Hu3DMotionShiftSpeedSet(playerState->characterModel, 2.0f);
                            }
                            else {
                                Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[2], 0.0f, 8.0f, HU3D_MOTATTR_LOOP | HU3D_MOTATTR_REV);
                                Hu3DMotionShiftSpeedSet(playerState->characterModel, 2.0f);
                            }
                            if (movementSoundPlaying != 0) {
                                movementSoundPlaying = 0;
                                HuAudFXStop(movementSoundId);
                            }
                        }
                    }
                    if (playerState->motionAttributes != 1) {
                        firstSoundFrame = 8;
                        secondSoundFrame = 32;
                    }
                    else {
                        firstSoundFrame = 7;
                        secondSoundFrame = 22;
                    }
                    if (playerState->animationFrame == firstSoundFrame) {
                        HuAudFXPlay(0x5E0);
                    }
                    if (playerState->animationFrame == secondSoundFrame) {
                        HuAudFXPlay(0x5E0);
                    }
                    playerState->animationFrame++;
                    if (playerState->motionAttributes != 1) {
                        if (playerState->animationFrame >= 50) {
                            playerState->animationFrame -= 50;
                        }
                    }
                    else {
                        if (playerState->animationFrame >= 30) {
                            playerState->animationFrame -= 30;
                        }
                    }
                }
                else {
                    if (ABS_INV(PlayerStickAxisGet(playerState->playerIndex, 0)) <= 20) {
                        if (playerState->effectState != 0) {
                            playerState->effectState = 0;
                            Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[0], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                            if (movementSoundPlaying != 0) {
                                movementSoundPlaying = 0;
                                HuAudFXStop(movementSoundId);
                            }
                        }
                    }
                    else {
                        if (playerState->effectState != 1) {
                            playerState->effectState = 1;
                            Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[3], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                            Hu3DMotionShiftSpeedSet(playerState->characterModel, 1.5f);
                            movementSoundId = HuAudFXPlay(0x5EB);
                            movementSoundPlaying = 1;
                        }
                    }
                }
                if ((collidingPlayerIndex = NearbyPlayerGet(playerState, 0.0f, 0)) != -1) {
                    collidedPlayer = &playerStates[collidingPlayerIndex];
                    if (GWPlayerCfg[collidedPlayer->playerIndex].group != 0) {
                        HuAudFXPlay(0x5ED);
                        playerState->effectState = 8;
                        playerState->motionFrame = 90;
                        Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[6], 0.0f, 5.0f, 0U);
                        if ((collidedPlayer->effectState == 4 || collidedPlayer->effectState == 5 || collidedPlayer->effectState == 6) == FALSE) {
                            collidedPlayer->effectState = 8;
                            collidedPlayer->motionFrame = 90;
                            Hu3DMotionShiftSet(collidedPlayer->characterModel, collidedPlayer->modelMotionIds[6], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
                        }
                        playerState->positionX = playerState->rotationX;
                        playerState->positionZ = playerState->rotationY;
                    }
                    else {
                        collidedPlayer->resultState = 1;
                        collidedPlayer->motionFlags = playerState->playerIndex;
                    }
                }
                if ((collidingPlayerIndex = PlayerArenaCollisionCheck(playerState)) != -1) {
                    collidedPlayer = &playerStates[collidingPlayerIndex];
                    if (GWPlayerCfg[collidedPlayer->playerIndex].group != 0) {
                        HuAudFXPlay(0x5ED);
                        playerState->effectState = 8;
                        playerState->motionFrame = 90;
                        Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[6], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
                        if ((collidedPlayer->effectState == 4 || collidedPlayer->effectState == 5 || collidedPlayer->effectState == 6) == FALSE) {
                            collidedPlayer->effectState = 8;
                            collidedPlayer->motionFrame = 90;
                            Hu3DMotionShiftSet(collidedPlayer->characterModel, collidedPlayer->modelMotionIds[6], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
                        }
                        playerState->positionX = playerState->rotationX;
                        playerState->positionZ = playerState->rotationY;
                    }
                    else {
                        collidedPlayer->resultState = 1;
                        collidedPlayer->motionFlags = playerState->playerIndex;
                    }
                }
                break;
        }
        if (playerState->fadeProgress == playerState->positionX && playerState->fadeSpeed == playerState->positionZ) {
            playerState->sequenceId++;
        }
        else {
            playerState->fadeProgress = playerState->positionX;
            playerState->fadeSpeed = playerState->positionZ;
            playerState->sequenceId = 0;
        }
        Hu3DModelPosSet(playerState->characterModel, playerState->positionX, playerState->positionY, playerState->positionZ);
        Hu3DModelRotSet(playerState->characterModel, playerState->rotationZ, playerState->turnAngle, playerState->turnSpeed);
        HuPrcVSleep();
    }
    if (movementSoundPlaying != 0) {
        HuAudFXStop(movementSoundId);
    }
    if (winningPlayerIndex == humanPlayerIndex) {
        Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[0], 0.0f, 2.0f, HU3D_MOTATTR_LOOP);
        HuPrcSleep(90);
        if (humanPlayerIndex == 0) {
            if (playerState->playerIndex != 0) {
                HuAudFXPlay(0x5EA);
            }
        }
        else {
            if (playerState->playerIndex == 0) {
                HuAudFXPlay(0x5EA);
            }
        }
        Hu3DModelAttrReset(playerState->effectModel, 1);
        Hu3DModelAttrReset(playerState->effectModel, HU3D_MOTATTR_PAUSE);
        Hu3DModelPosSet(playerState->effectModel, playerState->positionX, playerState->positionY, playerState->positionZ);
        Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[7], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
        if (humanPlayerIndex == 0) {
            if (playerState->playerIndex != 0) {
                HuAudFXPlay(0x5E3);
            }
        }
        else {
            if (playerState->playerIndex == 0) {
                HuAudFXPlay(0x5E3);
            }
        }
        HuPrcSleep(15);
        HuPrcSleep(30);
        playerState->previousPositionX = playerState->positionX;
        playerState->previousPositionY = playerState->positionY;
        playerState->previousPositionZ = playerState->positionZ;
        playerState->effectTimer |= 2;
        while (!Hu3DMotionEndCheck(playerState->characterModel)) {
            HuPrcVSleep();
        }
        playerSoundIds[playerState->playerIndex] = PlayerFXPlay(playerState->playerIndex, 0x11A);
        CharMotionShiftSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], playerState->characterMotionIds[3], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        Hu3DModelHookReset(playerState->characterModel);
        Hu3DModelShadowSet(playerState->shadowModel);
        shadowHeight = 50.0f;
        Hu3DModelPosSet(playerState->shadowModel, playerState->positionX, playerState->positionY + shadowHeight, playerState->positionZ);
        HuPrcSleep(60);
        for (blinkFrame = 0, blinkPhase = 0; blinkFrame < 90; blinkFrame++) {
            if (blinkPhase & 1) {
                Hu3DModelAttrReset(playerState->characterModel, 1);
                Hu3DModelAttrReset(playerState->effectModel, 1);
            }
            else {
                Hu3DModelAttrSet(playerState->characterModel, 1);
                Hu3DModelAttrSet(playerState->effectModel, 1);
            }
            blinkPhase++;
            HuPrcVSleep();
        }
        Hu3DModelAttrSet(playerState->effectModel, 1);
        for (rotationFrames = 0, modelTransparency = 1.0f; rotationFrames < 30; rotationFrames++) {
            modelTransparency -= 0.033333335f;
            Hu3DModelTPLvlSet(playerState->characterModel, modelTransparency);
            if (shadowHeight > 0.0f) {
                shadowHeight -= 5.0f;
                Hu3DModelPosSet(playerState->shadowModel, playerState->positionX, playerState->positionY + shadowHeight, playerState->positionZ);
            }
            HuPrcVSleep();
        }
        Hu3DModelAttrSet(playerState->characterModel, 1);
    }
    else {
        turnStep = playerState->rotationSpeed;
        Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[3], 0.0f, 2.0f, HU3D_MOTATTR_LOOP);
        if (AngleNormalize(playerState->turnAngle) != 0.0f) {
            rotationFrames = (360.0f - AngleNormalize(playerState->turnAngle)) / turnStep;
            if (playerState->turnAngle < 180.0f) {
                rotationFrames = playerState->turnAngle / turnStep;
                turnStep = -turnStep;
            }
            while (rotationFrames != 0) {
                playerState->turnAngle += turnStep;
                Hu3DModelRotSet(playerState->characterModel, playerState->rotationZ, playerState->turnAngle, playerState->turnSpeed);
                HuPrcVSleep();
                rotationFrames--;
            }
            Hu3DModelRotSet(playerState->characterModel, playerState->rotationZ, 0.0f, playerState->turnSpeed);
        }
        Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[0], 0.0f, 2.0f, HU3D_MOTATTR_LOOP);
        while (repositionTimer < 60) {
            HuPrcVSleep();
        }
        effectProcess = HuPrcChildCreate(EffectProcess, 0x2000, 0x1000, 0, HuPrcCurrentGet());
        effectProcess->user_data = playerState;
        CharMotionShiftSet(characterModelIds[GWPlayerCfg[playerState->playerIndex].character], playerState->characterMotionIds[2], 0.0f, 2.0f, HU3D_MOTATTR_NONE);
        playerState->effectState = 10;
    }
    while (TRUE) {
        HuPrcVSleep();
    }
}

void PlayersRepositionProcess(void)
{
    PlayerState *playerState;
    float repositionRadius;
    s32 repositionFrame;
    s32 playerIndex;

    for (repositionFrame = 0; repositionFrame < 4; repositionFrame++) {
        Hu3DModelAttrReset(playerStates[repositionFrame].characterModel, 1);
    }
    for (playerIndex = 0, repositionRadius = 1000.0f; playerIndex < 60; playerIndex++, repositionRadius -= 7.5f) {
        for (repositionFrame = 0; repositionFrame < 4; repositionFrame++) {
            if (repositionFrame != humanPlayerIndex) {
                playerState = &playerStates[repositionFrame];
                playerState->positionX = repositionRadius * sind(playerState->turnAngle - 180.0f);
                playerState->positionZ = repositionRadius * cosd(playerState->turnAngle - 180.0f);
                Hu3DModelPosSet(playerState->characterModel, playerState->positionX, playerState->positionY, playerState->positionZ);
            }
        }
        HuPrcVSleep();
    }
    playerState->positionX = 550.0 * sind(playerState->turnAngle - 180.0f);
    playerState->positionZ = 550.0 * cosd(playerState->turnAngle - 180.0f);
    Hu3DModelPosSet(playerState->characterModel, playerState->positionX, playerState->positionY, playerState->positionZ);
    for (repositionFrame = 0; repositionFrame < 4; repositionFrame++) {
        if (repositionFrame != humanPlayerIndex) {
            playerState = &playerStates[repositionFrame];
            Hu3DMotionShiftSet(playerState->characterModel, playerState->modelMotionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        }
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void VictorySequenceProcess(void)
{
    PlayerState *humanPlayer;
    float cameraDepthStep;
    float cameraRotationStart;
    float cameraZoomStart;
    float cameraDepthStart;
    float cameraHeightStart;
    float shakeAngle;
    s32 controllerIndex;
    s32 cameraTransitionFrame;
    s32 cameraAnimationFrame;
    s32 cameraShakeDuration;

    HuAudFXPlay(0x5E5);
    humanPlayer = &playerStates[humanPlayerIndex];
    CharMotionShiftSet(characterModelIds[GWPlayerCfg[humanPlayer->playerIndex].character], humanPlayer->modelMotionIds[4], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    cameraShakeDuration = 180;
    cameraDepthStart = ((CenterM->z - 687.0f) / cameraShakeDuration) * 40.0f / 60.0f;
    cameraDepthStep = ((CenterM->z - 687.0f) / cameraShakeDuration) * 20.0f / 60.0f;
    cameraHeightStart = CenterM->y;
    for (cameraTransitionFrame = 0; cameraTransitionFrame < 3; cameraTransitionFrame++) {
        for (cameraAnimationFrame = 30, shakeAngle = 0.0f; cameraAnimationFrame != 0; cameraAnimationFrame--, shakeAngle += 3.0f) {
            CenterM->y = cameraHeightStart + 50.0 * sind(shakeAngle);
            CenterM->z -= 2.0f * cameraDepthStart;
            HuPrcVSleep();
        }
        for (cameraAnimationFrame = 20, shakeAngle = 90.0f; cameraAnimationFrame != 0; cameraAnimationFrame--, shakeAngle += 4.5f) {
            CenterM->y = cameraHeightStart + 50.0 * sind(shakeAngle);
            CenterM->z -= 2.0f * cameraDepthStep * 1.5f;
            HuPrcVSleep();
        }
        HuAudFXPlay(0x5DF);
        for (controllerIndex = 0; controllerIndex < 4; controllerIndex++) {
            omVibrate(controllerIndex, 12, 12, 0);
        }
        HuPrcSleep(5);
        for (cameraAnimationFrame = 10, shakeAngle = 180.0f; cameraAnimationFrame != 0; cameraAnimationFrame--, shakeAngle += 18.0f) {
            CenterM->y = cameraHeightStart + 10.0 * sind(shakeAngle);
            HuPrcVSleep();
        }
        HuPrcSleep(10);
    }
    HuPrcChildCreate(PlayersRepositionProcess, 0x2000, 0x1000, 0, HuPrcCurrentGet());
    cameraRotationStart = CRotM->x;
    cameraHeightStart = CenterM->y;
    cameraDepthStart = CenterM->z;
    cameraZoomStart = CZoomM[0];
    for (cameraTransitionFrame = 0; cameraTransitionFrame < 60; cameraTransitionFrame++) {
        CRotM->x += (-45.0f - cameraRotationStart) / 60.0f;
        CenterM->y += (100.0f - cameraHeightStart) / 60.0f;
        CenterM->z += (70.0f - cameraDepthStart) / 60.0f;
        CZoomM[0] += (2130.0f - cameraZoomStart) / 60.0f;
        HuPrcVSleep();
    }
    HuAudFXPlay(0x5DF);
    for (controllerIndex = 0; controllerIndex < 4; controllerIndex++) {
        omVibrate(controllerIndex, 12, 12, 0);
    }
    Hu3DModelAttrSet(victoryEffectModelId, 1);
    PlayerFXPlay(humanPlayer->playerIndex, 0x128);
    humanPlayer = &playerStates[humanPlayerIndex];
    while (Hu3DMotionEndCheck(humanPlayer->characterModel)) {
        HuPrcVSleep();
    }
    CharMotionShiftSet(characterModelIds[GWPlayerCfg[humanPlayer->playerIndex].character], humanPlayer->modelMotionIds[5], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(15);
    while (Hu3DMotionEndCheck(humanPlayer->characterModel)) {
        HuPrcVSleep();
    }
    CharMotionShiftSet(characterModelIds[GWPlayerCfg[humanPlayer->playerIndex].character], humanPlayer->modelMotionIds[0], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    HuPrcSleep(60);
    cameraRotationStart = CRotM->x;
    cameraHeightStart = CenterM->y;
    cameraDepthStart = CenterM->z;
    cameraZoomStart = CZoomM[0];
    for (cameraTransitionFrame = 0; cameraTransitionFrame < 60; cameraTransitionFrame++) {
        CRotM->x += (-35.0f - cameraRotationStart) / 60.0f;
        CenterM->y += -cameraHeightStart / 60.0f;
        CenterM->z += (-250.0f - cameraDepthStart) / 60.0f;
        CZoomM[0] += (4820.0f - cameraZoomStart) / 60.0f;
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    humanPlayer = &playerStates[humanPlayerIndex];
    CharMotionShiftSet(characterModelIds[GWPlayerCfg[humanPlayer->playerIndex].character], humanPlayer->modelMotionIds[4], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    HuPrcSleep(15);
    while (Hu3DMotionEndCheck(humanPlayer->characterModel)) {
        HuPrcVSleep();
    }
    CharMotionSpeedSet(characterModelIds[GWPlayerCfg[humanPlayer->playerIndex].character], 2.0f);
}

void PlayerResultProcess(s32 playerIndex)
{
    PlayerState *playerState;
    float cameraStartZoom;
    float cameraStartZ;
    float cameraStartX;
    s32 i;

    if (playerIndex == humanPlayerIndex) {
        HuPrcSleep(120);
        cameraStartZoom = CenterM->x;
        cameraStartZ = CenterM->z;
        cameraStartX = CZoomM[0];
        for (i = 0; i < 60; i++) {
            playerState = &playerStates[humanPlayerIndex];
            CenterM->x += (playerState->positionX - cameraStartZoom) / 60.0f;
            CenterM->z += (playerState->positionZ - 100.0f - cameraStartZ) / 60.0f;
            CZoomM[0] += (1000.0f - cameraStartX) / 60.0f;
            HuPrcVSleep();
        }
        for (i = 0; i < 4; i++) {
            if (i == humanPlayerIndex) {
                continue;
            }
            HuAudFXStop(playerSoundIds[i]);
        }
        HuAudSStreamPlay(1);
        Hu3DMotionShiftSet(playerState->characterModel, playerState->additionalMotionId, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    }
    else {
        for (repositionTimer = 0; repositionTimer < 60; repositionTimer++) {
            HuPrcVSleep();
        }
        HuAudSStreamPlay(1);
    }
}

void MinigameProcess(void)
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

void MinigameMainProcess(void)
{
    s16 countdownSequence;
    s16 minigameSequence;
    s16 remainingTimeFrames;
    s32 backgroundMusicId;
    s32 opponentCount;
    s32 opponentIndices[3];
    s32 i;

    minigameState = 0;
    winningPlayerIndex = -1;
    VictorySequenceProcess();
    HuPrcSleep(30);
    minigameState = 1;
    while (minigameState == 1) {
        HuPrcVSleep();
    }
    backgroundMusicId = HuAudSeqPlay(0x49);
    minigameSequence = MGSeqStartCreate();
    while (MGSeqStatGet(minigameSequence) != 0) {
        HuPrcVSleep();
    }
    remainingTimeFrames = 45 * REFRESH_RATE + REFRESH_RATE - 1;
    countdownSequence = MGSeqTimerCreate(remainingTimeFrames / REFRESH_RATE);
    minigameState = 3;
    while (TRUE) {
        if (playerStates[humanPlayerIndex].resultState == 1) {
            break;
        }
        if (remainingTimeFrames < REFRESH_RATE - 1) {
            winningPlayerIndex = humanPlayerIndex;
            break;
        }
        MGSeqParamSet(countdownSequence, 1, remainingTimeFrames-- / REFRESH_RATE);
        roundTimer++;
        HuPrcVSleep();
    }
    for (i = 0; i < 4; i++) {
        playerStates[i].resultState = 1;
    }
    minigameState = 4;
    HuAudSeqFadeOut(backgroundMusicId, 100);
    MGSeqParamSet(countdownSequence, 2, -1);
    minigameSequence = MGSeqFinishCreate();
    while (MGSeqStatGet(minigameSequence) != 0) {
        HuPrcVSleep();
    }
    PlayerResultProcess(winningPlayerIndex);
    if (winningPlayerIndex == humanPlayerIndex) {
        GWPlayerCoinWinSet(winningPlayerIndex, 10);
        minigameSequence = MGSeqWinCreate(GWPlayerCfg[winningPlayerIndex].character, -1, -1, -1);
    }
    else {
        for (i = 0, opponentCount = 0; i < 4; i++) {
            if (i != humanPlayerIndex) {
                opponentIndices[opponentCount++] = i;
                GWPlayerCoinWinSet(i, 10);
            }
        }
        minigameSequence = MGSeqWinCreate(GWPlayerCfg[opponentIndices[0]].character, GWPlayerCfg[opponentIndices[1]].character, GWPlayerCfg[opponentIndices[2]].character, -1);
    }
    HuPrcSleep(210);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    HuPrcSleep(60);
    HuAudFadeOut(1);
    MGSeqKillAll();
    omOvlReturnEx(1, 1);
    while (TRUE) {
        HuPrcVSleep();
    }
}
