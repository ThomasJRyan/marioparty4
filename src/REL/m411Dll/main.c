#include "REL/executor.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "ext_math.h"

#ifndef __MWERKS__
#include "game/hsfex.h"
#endif

typedef struct {
    /* 0x00 */ s16 modelId;
    /* 0x02 */ s16 motionIds[5];
    /* 0x0C */ s16 spriteGroupId;
    /* 0x0E */ s16 characterMotionIds;
    /* 0x10 */ Vec modelRotation;
    /* 0x1C */ Vec characterAnimationSlots;
} M411ModelMotionSlot; // Size 0x28

typedef struct {
    /* 0x000 */ s16 modelId;
    /* 0x002 */ char motionIds[2];
    /* 0x004 */ Vec modelPosition;
    /* 0x010 */ Vec modelRotation;
    /* 0x01C */ M411ModelMotionSlot characterAnimationSlots[5];
    /* 0x0E4 */ M411ModelMotionSlot characterTransforms[5];
    /* 0x1AC */ M411ModelMotionSlot sceneModelSlots[4];
    /* 0x24C */ char characterNameAnimation[0x28];
    /* 0x274 */ M411ModelMotionSlot sceneAnimationSlots[5];
} M411SceneModelSet; // Size 0x33C

typedef struct {
    /* 0x000 */ s16 modelId;
    /* 0x002 */ s16 motionIds;
    /* 0x004 */ s16 modelPosition;
    /* 0x006 */ s16 playerCharacterCount;
    /* 0x008 */ s16 modelIdSecondary;
    /* 0x00A */ s16 playerIndex;
    /* 0x00C */ s16 spriteGroupId;
    /* 0x00E */ s16 characterMotionIds[9];
    /* 0x020 */ s16 motionFrame;
    /* 0x022 */ s16 modelMotionIds[9];
    /* 0x034 */ s32 playerIndex;
    /* 0x038 */ s32 cameraMask;
    /* 0x03C */ s32 animationFrame;
    /* 0x040 */ s32 animationState;
    /* 0x044 */ s32 animationTimer;
    /* 0x048 */ float positionX;
    /* 0x04C */ float positionY;
    /* 0x050 */ float positionZ;
    /* 0x054 */ float rotationX;
    /* 0x058 */ float rotationY;
    /* 0x05C */ float rotationZ;
    /* 0x060 */ float scaleX;
    /* 0x064 */ float scaleY;
    /* 0x068 */ float scaleZ;
    /* 0x06C */ HSFOBJECT *characterModelParts[10];
    /* 0x094 */ HSFOBJECT *characterShadowParts[10];
    /* 0x0BC */ HSFOBJECT *characterEffectParts[10];
    /* 0x0E4 */ HSFTRANSFORM characterTransforms[10];
    /* 0x24C */ ANIMDATA *characterNameAnimation;
    /* 0x250 */ ANIMDATA *scoreAnimation;
    /* 0x254 */ s8 animationVariant;
    /* 0x255 */ s8 playerTeam;
    /* 0x256 */ s8 playerDifficulty;
    /* 0x257 */ s8 selectionState;
    /* 0x258 */ u16 animationFlags;
    /* 0x25A */ char reservedAnimationData[0xA];
    /* 0x264 */ s32 animationTimer;
    /* 0x268 */ s32 effectTimer;
} M411PlayerSceneState; // Size 0x26C

typedef struct {
    /* 0x00 */ M411PlayerSceneState *modelId;
    /* 0x04 */ s32 modelPosition;
    /* 0x08 */ s32 modelIdSecondary;
    /* 0x0C */ s32 *spriteGroupId;
} M411ModelStateReference; // Size unknown

void UpdateSplitScreenCameras(OMOBJ *cameraObject);
void SetPuzzleScrollSpeed(float scrollSpeed);
void DrawCameraTransitionMask(void);
void CopyCameraTransitionTexture(s16 layer);
s32 CreatePuzzleSceneModels(void);
s32 CreatePuzzleCharacterModels(void);
s32 InitializePuzzleScene(void);
void LoadPuzzleSceneAssets(void);
void DrawParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix, s32 playerIndex);
void UpdateParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void UpdateParticleEffectAlt(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void UpdateParticleEffectSecondary(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void UpdateParticleEffectSecondaryAlt(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void InitializePuzzleBoard(void);
void CreatePuzzleBoard(void);
void RunPuzzleIntro(void);
void RunPuzzleCharacterIntro(void);
void RunPuzzleBoardAnimation(void);
void RunPuzzleSceneProcess(void);
void RunPuzzlePlayerProcess(void);
void RunPuzzleCursorProcess(void);
void RunPuzzleResultAnimation(void);
void RunPuzzleResultEffect(void);
void RunPuzzleCharacterAnimation(void);
void RunPuzzleItemAnimation(void);
void RunPuzzleItemEffect(void);
s8 GetPlayerStickX(s32 playerIndex, s32 stickAxis);
u8 GetPlayerStickY(s32 playerIndex, s32 stickAxis);
u16 GetPlayerInputAngle(s32 playerIndex);
void UpdatePlayerCharacter(M411PlayerSceneState *playerState, s32 playerIndex);
void UpdatePlayerCharacterAlt(M411PlayerSceneState *playerState, s32 playerIndex);
void UpdatePlayerCharacterState(M411PlayerSceneState *playerState, s32 playerIndex, s32 animationState);
void UpdatePlayerEffects(void);
void UpdatePuzzleCollisions(void);
void UpdatePlayerPosition(M411PlayerSceneState *playerState, s32 playerIndex);
void CheckPlayerCollision(s32 playerIndex, s32 otherPlayerIndex);
void UpdatePlayerAnimation(M411PlayerSceneState *playerState, s32 playerIndex, s32 animationState);
void CreatePlayerSprites(void);
void AnimateCameraViewports(void);
void AnimatePlayerSelection(void);
void SetCameraViewports(float viewportWidth, float viewportHeight);
void AnimateCameraOpening(void);
void AnimateCameraClosing(void);
void AnimateWinningPlayer(s32 playerIndex);
void AnimateWinnerCamera(void);
void RotateWinningPlayer(void);
void PlayModeZeroWinnerAnimation(s32 playerIndex);
void PlayModeOneWinnerAnimation(s32 playerIndex);
void PlayModeTwoWinnerAnimation(s32 playerIndex);
void HandlePuzzleSceneExit(void);
void RunPuzzleScene(void);

s16 sceneLightId;
s16 backgroundTextureScrollId;
s16 foregroundTextureScrollId;
s32 sceneState;
s32 winningPlayerIndex;
s32 playerSpriteIds[10];
s32 playerModelIds[10];
s32 puzzleMode;
s32 puzzleVariant;
s32 activeCameraCount;
s32 sceneSoundEffectIds[3];
s32 sceneSoundEffectCount;
float textureScrollSpeed;
M411PlayerSceneState playerSceneStates[4];
M411SceneModelSet sceneModelSet;
OMOBJ *sceneCameraObject;
HUPROCESS *playerProcesses[4];
Vec cameraPositions[8];
Vec cameraTargets[8];
Vec cameraUpVectors[8];
void *cameraTransitionTextureBuffer;

float cameraSettingsByMode[][7] = { { -10.0f, 0.0f, 0.0f, 0.0f, 0.0f, -3375.0f, 7460.0f }, { -9.5f, 0.0f, 0.0f, 0.0f, 0.0f, -510.0f, 1860.0f },
    { -5.0f, 0.0f, 0.0f, 0.0f, 0.0f, -2870.0f, 5060.0f } };

s32 puzzleResourceIds[] = { DATA_MAKE_NUM(DATADIR_M411, 9), DATA_MAKE_NUM(DATADIR_M411, 11), DATA_MAKE_NUM(DATADIR_M411, 10) };

char *puzzleModelNames[] = { "puzzle00", "puzzle00", "puzzle00" };

char *puzzlePieceModelNames[][10] = { { "puzzle_B1-p01", "puzzle_B1-p02", "puzzle_B1-p03", "puzzle_B1-p04", "puzzle_B1-p05", "puzzle_B1-p06",
                                   "puzzle_B1-p07", "puzzle_B1-p08" },
    { "puzzle_B2-p01", "puzzle_B2-p02", "puzzle_B2-p03", "puzzle_B2-p04", "puzzle_B2-p05", "puzzle_B2-p06", "puzzle_B2-p07", "puzzle_B2-p08" },
    { "puzzle_B3-p01", "puzzle_B3-p02", "puzzle_B3-p03", "puzzle_B3-p04", "puzzle_B3-p05", "puzzle_B3-p06", "puzzle_B3-p07", "puzzle_B3-p08" } };

s32 characterModelIndices[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

s32 characterModelResourceIds[8] = { DATA_MAKE_NUM(DATADIR_MARIOMDL1, 0), DATA_MAKE_NUM(DATADIR_LUIGIMDL1, 0), DATA_MAKE_NUM(DATADIR_PEACHMDL1, 0),
    DATA_MAKE_NUM(DATADIR_YOSHIMDL1, 0), DATA_MAKE_NUM(DATADIR_WARIOMDL1, 0), DATA_MAKE_NUM(DATADIR_DONKEYMDL1, 0),
    DATA_MAKE_NUM(DATADIR_DAISYMDL1, 0), DATA_MAKE_NUM(DATADIR_WALUIGIMDL1, 0) };

s32 marioMotionResourceIds[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), DATA_MAKE_NUM(DATADIR_MARIOMOT, 1), DATA_MAKE_NUM(DATADIR_MARIOMOT, 2),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 5), DATA_MAKE_NUM(DATADIR_MARIOMOT, 6), DATA_MAKE_NUM(DATADIR_MARIOMOT, 27), DATA_MAKE_NUM(DATADIR_MARIOMOT, 32),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 33), DATA_MAKE_NUM(DATADIR_MARIOMOT, 23) };

s32 characterMotionResourceIds[8] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0), DATA_MAKE_NUM(DATADIR_PEACHMOT, 0),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0), DATA_MAKE_NUM(DATADIR_WARIOMOT, 0), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0), DATA_MAKE_NUM(DATADIR_DAISYMOT, 0),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0) };

s32 sceneModelMotionResourceIds[] = { DATA_MAKE_NUM(DATADIR_M411, 27), DATA_MAKE_NUM(DATADIR_M411, 28), DATA_MAKE_NUM(DATADIR_M411, 23) };

s32 sceneAnimationResourceIds[] = { DATA_MAKE_NUM(DATADIR_M411, 30), DATA_MAKE_NUM(DATADIR_M411, 31), DATA_MAKE_NUM(DATADIR_M411, 32),
    DATA_MAKE_NUM(DATADIR_M411, 33), DATA_MAKE_NUM(DATADIR_M411, 34) };

void ObjectSetup(void)
{
    Vec lookPosition;
    Vec lookTarget;
    Vec cameraUp;
    float movementAngle;
    float movementAngle2;
    float savedValue;
    HUPROCESS *process;
    s32 stateValue;
    s32 i;

    stateValue = GWSystem.mg_type;
    puzzleMode = stateValue;
    if (stateValue == -1) {
        puzzleMode = frandmod(3);
    }
    puzzleVariant = frandmod(3);
    switch (puzzleVariant) {
        case 0:
            activeCameraCount = 8;
            break;
        case 1:
            activeCameraCount = 8;
            break;
        default:
            activeCameraCount = 8;
            break;
    }
    process = omInitObjMan(50, 0x2000);
    omGameSysInit(process);
    Hu3DCameraCreate(0x1F);
    Hu3DCameraPerspectiveSet(1, 15.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraPerspectiveSet(0x1E, 15.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 320.0f, 240.0f, 0.0f, 1.0f);
    Hu3DCameraViewportSet(2, 0.0f, 0.0f, 318.0f, 238.0f, 0.0f, 1.0f);
    Hu3DCameraViewportSet(4, 322.0f, 0.0f, 318.0f, 238.0f, 0.0f, 1.0f);
    Hu3DCameraViewportSet(8, 0.0f, 242.0f, 318.0f, 238.0f, 0.0f, 1.0f);
    Hu3DCameraViewportSet(0x10, 322.0f, 242.0f, 318.0f, 238.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 0x140, 0xF0);
    Hu3DCameraScissorSet(2, 0, 0, 0x13E, 0xEE);
    Hu3DCameraScissorSet(4, 0x142, 0, 0x13E, 0xEE);
    Hu3DCameraScissorSet(8, 0, 0xF2, 0x13E, 0xEE);
    Hu3DCameraScissorSet(0x10, 0x142, 0xF2, 0x13E, 0xEE);
    if (puzzleMode == 0) {
        sceneCameraObject = omAddObjEx(process, 0x7FDA, 0, 0, -1, UpdateSplitScreenCameras);
    }
    else {
        sceneCameraObject = omAddObjEx(process, 0x7FDA, 0, 0, -1, omOutViewMulti);
    }
    sceneCameraObject->work[0] = 5;
    for (i = 0; i < 1; i++) {
        CRotM[i].x = cameraSettingsByMode[puzzleMode][0];
        CRotM[i].y = cameraSettingsByMode[puzzleMode][1];
        CRotM[i].z = cameraSettingsByMode[puzzleMode][2];
        CenterM[i].x = cameraSettingsByMode[puzzleMode][3];
        CenterM[i].y = cameraSettingsByMode[puzzleMode][4];
        CenterM[i].z = cameraSettingsByMode[puzzleMode][5];
        CZoomM[i] = cameraSettingsByMode[puzzleMode][6];
    }
    for (i = 1; i < sceneCameraObject->work[0]; i++) {
        CRotM[i].x = 0.0f;
        CRotM[i].y = 0.0f;
        CRotM[i].z = 0.0f;
        CenterM[i].x = 0.0f;
        CenterM[i].y = 0.0f;
        CenterM[i].z = 0.0f;
        CZoomM[i] = 1900.0f;
    }
    movementAngle = CRotM[1].x;
    movementAngle2 = CRotM[1].y;
    savedValue = CRotM[1].z;
    cameraPositions[1].x = CenterM[1].x + sind(movementAngle2) * cosd(movementAngle) * CZoomM[1];
    cameraPositions[1].y = CenterM[1].y + -sind(movementAngle) * CZoomM[1];
    cameraPositions[1].z = CenterM[1].z + cosd(movementAngle2) * cosd(movementAngle) * CZoomM[1];
    cameraTargets[1].x = CenterM[1].x;
    cameraTargets[1].y = CenterM[1].y;
    cameraTargets[1].z = CenterM[1].z;
    cameraUpVectors[1].x = sind(movementAngle2) * sind(movementAngle);
    cameraUpVectors[1].y = cosd(movementAngle);
    cameraUpVectors[1].z = cosd(movementAngle2) * sind(movementAngle);
    for (i = 0; i < 1; i++) {
        cameraPositions[i].x = 0.0f;
        cameraPositions[i].y = 1295.0f;
        cameraPositions[i].z = 3971.0f;
        cameraUpVectors[i].x = 0.0f;
        cameraUpVectors[i].y = 1.0f;
        cameraUpVectors[i].z = 0.0f;
        cameraTargets[i].x = 0.0f;
        cameraTargets[i].y = 0.0f;
        cameraTargets[i].z = -3375.0f;
    }
    for (i = 1; i < sceneCameraObject->work[0]; i++) {
        cameraPositions[i].x = 0.0f;
        cameraPositions[i].y = 0.0f;
        cameraPositions[i].z = 1900.0f;
        cameraUpVectors[i].x = 0.0f;
        cameraUpVectors[i].y = 1.0f;
        cameraUpVectors[i].z = 0.0f;
        cameraTargets[i].x = 0.0f;
        cameraTargets[i].y = 0.0f;
        cameraTargets[i].z = 0.0f;
    }
    Hu3DLighInit();
    sceneLightId = Hu3DGLightCreate(10000.0f, 0.0f, 10000.0f, -1.0f, 0.0f, -1.0f, 0xFF, 0xFF, 0xFF);
    Hu3DGLightPosAimSet(sceneLightId, 0.0f, 6796.88f, 9706.95f, 0.0f, 0.0f, 0.0f);
    Hu3DGLightInfinitytSet(sceneLightId);
    lookPosition.x = -1800.0f;
    lookPosition.y = 3600.0f;
    lookPosition.z = 0.0f;
    lookTarget.x = lookTarget.z = 0.0f;
    lookTarget.y = 1.0f;
    cameraUp.x = -100.0f;
    cameraUp.y = 0.0f;
    cameraUp.z = 0.0f;
    Hu3DShadowCreate(30.0f, 20.0f, 10000.0f);
    Hu3DShadowTPLvlSet(0.4f);
    Hu3DShadowPosSet(&lookPosition, &lookTarget, &cameraUp);
    Hu3DBGColorSet(0, 0, 0);
    LoadPuzzleSceneAssets();
    SetCameraViewports(640.0f, 480.0f);
    HuPrcChildCreate(RunPuzzleScene, 0x1000, 0x2000, 0, HuPrcCurrentGet());
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
}

void UpdateSplitScreenCameras(OMOBJ *cameraObject)
{
    s32 i;

    for (i = 0; i < cameraObject->work[0]; i++) {
        Hu3DCameraPosSet(1 << i, cameraPositions[i].x, cameraPositions[i].y, cameraPositions[i].z, cameraUpVectors[i].x, cameraUpVectors[i].y, cameraUpVectors[i].z,
            cameraTargets[i].x, cameraTargets[i].y, cameraTargets[i].z);
    }
}

void SetPuzzleScrollSpeed(float scrollSpeed)
{
    textureScrollSpeed = scrollSpeed;
    Hu3DTexScrollPosMoveSet(backgroundTextureScrollId, -0.01f * scrollSpeed, 0.0f, 0.0f);
    Hu3DTexScrollPosMoveSet(foregroundTextureScrollId, -0.017f * scrollSpeed, 0.0f, 0.0f);
    HuAudFXPitchSet(sceneSoundEffectIds[0], -8191.0f * (1.0f - scrollSpeed));
}

void DrawCameraTransitionMask(void)
{
    Mtx44 projectionMatrix;
    Mtx viewMatrix;
    Vec lookPosition;
    Vec lookTarget;
    Vec cameraUp;
    GXColor clearColor;
    HU3DCAMERA *camera;

    camera = &Hu3DCamera[0];
    C_MTXPerspective(projectionMatrix, camera->fov, camera->aspect, camera->near, camera->far);
    GXSetProjection(projectionMatrix, GX_PERSPECTIVE);
    GXSetViewport(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    GXSetScissor(0, 0, 640, 480);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_RGBA6, 0);
    clearColor.r = clearColor.g = clearColor.b = 0;
    clearColor.a = 0xFF;
    GXSetTevColor(GX_TEVREG0, clearColor);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetCullMode(GX_CULL_NONE);
    GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
    lookPosition.x = lookPosition.y = lookPosition.z = 0.0f;
    lookTarget.x = lookTarget.y = 0.0f;
    lookTarget.z = -100.0f;
    cameraUp.x = cameraUp.z = 0.0f;
    cameraUp.y = 1.0f;
    C_MTXLookAt(viewMatrix, &lookPosition, &cameraUp, &lookTarget);
    GXLoadPosMtxImm(viewMatrix, GX_PNMTX0);
    lookPosition.x = 10000.0f * -sin(camera->fov / 2) * 1.2f;
    lookPosition.y = 10000.0f * -sin(camera->fov / 2) * 0.8333333f;
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-lookPosition.x, -lookPosition.y, -10000.0f);
    GXPosition3f32(lookPosition.x, -lookPosition.y, -10000.0f);
    GXPosition3f32(lookPosition.x, lookPosition.y, -10000.0f);
    GXPosition3f32(-lookPosition.x, lookPosition.y, -10000.0f);
    GXEnd();
}

void CopyCameraTransitionTexture(s16 layer)
{
    if (Hu3DCameraNo == 1) {
        GXSetTexCopySrc(0, 0, 320, 240);
        GXSetTexCopyDst(320, 240, GX_TF_RGB5A3, GX_FALSE);
        GXCopyTex(cameraTransitionTextureBuffer, GX_TRUE);
        DCFlushRange(cameraTransitionTextureBuffer, 0x25800); // TODO: sizeof?
        DrawCameraTransitionMask();
    }
}

s32 CreatePuzzleSceneModels(void)
{
    M411SceneModelSet *sceneModels;
    M411ModelMotionSlot *modelSlot;
    M411ModelMotionSlot *modelSlot2;
    HUPROCESS *process;
    void *resourceData;
    s16 modelId;
    s16 modelId2;
    s16 modelId3;
    s16 modelId4;
    s32 i;
    s32 j;

    sceneModels = &sceneModelSet;
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 15), HU_MEMNUM_OVL, HEAP_MODEL);
    sceneModels->modelId = modelId = Hu3DModelCreate(resourceData);
    Hu3DModelCameraSet(modelId, 1);
    sceneModels->modelPosition.x = sceneModels->modelPosition.y = sceneModels->modelPosition.z = 0.0f;
    sceneModels->modelRotation.x = sceneModels->modelRotation.y = sceneModels->modelRotation.z = 0.0f;
    Hu3DModelPosSet(modelId, sceneModels->modelPosition.x, sceneModels->modelPosition.y, sceneModels->modelPosition.z);
    Hu3DModelRotSet(modelId, sceneModels->modelRotation.x, sceneModels->modelRotation.y, sceneModels->modelRotation.z);
    Hu3DModelShadowMapSet(modelId);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 16), HU_MEMNUM_OVL, HEAP_MODEL);
    sceneModels->characterAnimationSlots[0].modelId = modelId = Hu3DModelCreate(resourceData);
    Hu3DModelCameraSet(modelId, 1);
    Hu3DModelPosSet(modelId, sceneModels->modelPosition.x, sceneModels->modelPosition.y, sceneModels->modelPosition.z);
    Hu3DModelRotSet(modelId, sceneModels->modelRotation.x, sceneModels->modelRotation.y, sceneModels->modelRotation.z);
    modelSlot = &sceneModels->characterTransforms[0];
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 17), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->modelId = modelId = Hu3DModelCreate(resourceData);
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelCameraSet(modelId, 1);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 18), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->spriteGroupId = modelId2 = Hu3DModelCreate(resourceData);
    Hu3DModelAttrSet(modelId2, HU3D_ATTR_DISPOFF);
    Hu3DModelCameraSet(modelId2, 1);
    for (i = 1; i < 5; i++) {
        modelSlot = &sceneModels->characterTransforms[i];
        modelSlot->modelId = Hu3DModelLink(modelId);
        Hu3DModelAttrSet(modelSlot->modelId, HU3D_ATTR_DISPOFF);
        Hu3DModelCameraSet(modelSlot->modelId, 1);
        if (i == 1) {
            resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 19), HU_MEMNUM_OVL, HEAP_MODEL);
            modelSlot->spriteGroupId = modelId3 = Hu3DModelCreate(resourceData);
        }
        else if (i & 1) {
            modelSlot->spriteGroupId = Hu3DModelLink(modelId3);
        }
        else {
            modelSlot->spriteGroupId = Hu3DModelLink(modelId2);
        }
        Hu3DModelAttrSet(modelSlot->spriteGroupId, HU3D_ATTR_DISPOFF);
        Hu3DModelCameraSet(modelSlot->spriteGroupId, 1);
    }
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 17), HU_MEMNUM_OVL, HEAP_MODEL);
    sceneModels->sceneAnimationSlots[0].modelId = modelId = Hu3DModelCreate(resourceData);
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelCameraSet(modelId, 1);
    for (i = 1; i < 5; i++) {
        sceneModels->sceneAnimationSlots[i].modelId = Hu3DModelLink(modelId);
        Hu3DModelAttrSet(sceneModels->sceneAnimationSlots[i].modelId, HU3D_ATTR_DISPOFF);
        Hu3DModelCameraSet(sceneModels->sceneAnimationSlots[i].modelId, 1);
    }
    modelSlot = &sceneModels->characterAnimationSlots[1];
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 20), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->modelId = modelId = Hu3DModelCreate(resourceData);
    Hu3DModelCameraSet(modelId, 1);
    modelSlot->modelRotation.x = -470.0f;
    modelSlot->modelRotation.y = 0.0f;
    modelSlot->modelRotation.z = -930.0f;
    modelSlot->characterAnimationSlots.x = 0.0f;
    modelSlot->characterAnimationSlots.y = 90.0f;
    modelSlot->characterAnimationSlots.z = 0.0f;
    Hu3DModelPosSet(modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
    Hu3DModelRotSet(modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
    for (i = 0; i < 3; i++) {
        resourceData = HuDataSelHeapReadNum(sceneModelMotionResourceIds[i], HU_MEMNUM_OVL, HEAP_MODEL);
        modelSlot->motionIds[i] = Hu3DJointMotion(modelId, resourceData);
    }
    Hu3DMotionSet(modelId, modelSlot->motionIds[0]);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelId);
    modelSlot2 = &sceneModels->characterAnimationSlots[2];
    modelSlot2->modelId = Hu3DModelLink(modelId);
    Hu3DModelCameraSet(modelSlot2->modelId, 1);
    Hu3DModelPosSet(modelSlot2->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
    Hu3DModelRotSet(modelSlot2->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
    for (i = 0; i < 3; i++) {
        resourceData = HuDataSelHeapReadNum(sceneModelMotionResourceIds[i], HU_MEMNUM_OVL, HEAP_MODEL);
        modelSlot2->motionIds[i] = Hu3DJointMotion(modelId, resourceData);
    }
    Hu3DModelAttrSet(modelSlot2->modelId, HU3D_ATTR_CULL_FRONT);
    Hu3DModelAttrSet(modelSlot2->modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelScaleSet(modelSlot2->modelId, 1.0f, -1.0f, 1.0f);
    for (i = 0; i < 2; i++) {
        modelSlot = &sceneModels->sceneModelSlots[i];
        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 29), HU_MEMNUM_OVL, HEAP_MODEL);
        modelSlot->modelId = modelId4 = Hu3DModelCreate(resourceData);
        modelId = modelSlot->modelId;
        modelSlot->characterMotionIds = i;
        Hu3DModelCameraSet(modelId, 1);
        for (j = 0; j < 5; j++) {
            resourceData = HuDataSelHeapReadNum(sceneAnimationResourceIds[j], HU_MEMNUM_OVL, HEAP_MODEL);
            modelSlot->motionIds[j] = Hu3DJointMotion(modelId, resourceData);
        }
        Hu3DMotionSet(modelId, modelSlot->motionIds[0]);
        Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
        modelSlot->modelRotation.x = 200.0f;
        modelSlot->modelRotation.y = 0.0f;
        modelSlot->modelRotation.z = -930.0f;
        Hu3DModelPosSet(modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
        Hu3DModelShadowSet(modelId);
    }
    for (i = 2; i < 4; i++) {
        modelSlot2 = &sceneModels->sceneModelSlots[i];
        modelSlot2->modelId = Hu3DModelLink(modelId4);
        modelSlot2->characterMotionIds = i;
        for (j = 0; j < 5; j++) {
            resourceData = HuDataSelHeapReadNum(sceneAnimationResourceIds[j], HU_MEMNUM_OVL, HEAP_MODEL);
            modelSlot2->motionIds[j] = Hu3DJointMotion(modelSlot2->modelId, resourceData);
        }
        Hu3DModelCameraSet(modelSlot2->modelId, 1);
        Hu3DModelAttrSet(modelSlot2->modelId, HU3D_ATTR_CULL_FRONT);
        Hu3DModelAttrSet(modelSlot2->modelId, HU3D_MOTATTR_LOOP);
        Hu3DModelPosSet(modelSlot2->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
        Hu3DModelScaleSet(modelSlot2->modelId, 1.0f, -1.0f, 1.0f);
    }
    process = HuPrcChildCreate(RunPuzzleSceneProcess, 0x2000, 0x2000, 0, HuPrcCurrentGet());
    process->user_data = sceneModels;
}

s32 CreatePuzzleCharacterModels(void)
{
    M411SceneModelSet *sceneModels;
    M411ModelMotionSlot *modelSlot;
    s16 modelId;
    s16 savedValue;
    s16 modelId2;
    HUPROCESS *process;
    void *resourceData;

    sceneModels = &sceneModelSet;
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 35), HU_MEMNUM_OVL, HEAP_MODEL);
    sceneModels->modelId = modelId = Hu3DModelCreate(resourceData);
    Hu3DModelCameraSet(modelId, 1);
    sceneModels->modelPosition.x = sceneModels->modelPosition.y = sceneModels->modelPosition.z = 0.0f;
    sceneModels->modelRotation.x = sceneModels->modelRotation.y = sceneModels->modelRotation.z = 0.0f;
    Hu3DModelPosSet(modelId, sceneModels->modelPosition.x, sceneModels->modelPosition.y, sceneModels->modelPosition.z);
    Hu3DModelRotSet(modelId, sceneModels->modelRotation.x, sceneModels->modelRotation.y, sceneModels->modelRotation.z);
    Hu3DModelShadowMapSet(modelId);
    modelSlot = &sceneModels->characterAnimationSlots[0];
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 36), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->modelId = modelId = Hu3DModelCreate(resourceData);
    Hu3DModelCameraSet(modelId, 1);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelId);
    modelSlot = &sceneModels->characterAnimationSlots[1];
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 38), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = savedValue = Hu3DModelCreate(resourceData);
    modelSlot->modelId = modelId;
    Hu3DModelCameraSet(modelId, 1);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 39), HU_MEMNUM_OVL, HEAP_MODEL);
    Hu3DMotionSet(modelId, Hu3DJointMotion(modelId, resourceData));
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelId);
    modelSlot = &sceneModels->characterAnimationSlots[2];
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 38), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->modelId = modelId = Hu3DModelCreate(resourceData);
    Hu3DModelCameraSet(modelId, 1);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 40), HU_MEMNUM_OVL, HEAP_MODEL);
    Hu3DMotionSet(modelId, Hu3DJointMotion(modelId, resourceData));
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelPosSet(modelId, 0.0f, 60.0f, -70.0f);
    Hu3DModelShadowSet(modelId);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 37), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId2 = Hu3DModelCreate(resourceData);
    modelSlot = &sceneModels->characterTransforms[0];
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 38), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->modelId = modelId = Hu3DModelCreate(resourceData);
    Hu3DModelCameraSet(modelId, 1);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 41), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->motionIds[0] = Hu3DJointMotion(modelId, resourceData);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 42), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->motionIds[1] = Hu3DJointMotion(modelId, resourceData);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 45), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->motionIds[2] = Hu3DJointMotion(modelId, resourceData);
    Hu3DMotionSet(modelId, modelSlot->motionIds[0]);
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelHookSet(modelId, "itemhook_r", modelId2);
    Hu3DModelShadowSet(modelId);
    modelSlot = &sceneModels->characterTransforms[1];
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 38), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->modelId = modelId = Hu3DModelCreate(resourceData);
    Hu3DModelCameraSet(modelId, 1);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 43), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->motionIds[0] = Hu3DJointMotion(modelId, resourceData);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 44), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->motionIds[1] = Hu3DJointMotion(modelId, resourceData);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 46), HU_MEMNUM_OVL, HEAP_MODEL);
    modelSlot->motionIds[2] = Hu3DJointMotion(modelId, resourceData);
    Hu3DMotionSet(modelId, modelSlot->motionIds[0]);
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelHookSet(modelId, "itemhook_l", modelId2);
    Hu3DModelShadowSet(modelId);
    process = HuPrcChildCreate(RunPuzzleResultAnimation, 0x2000, 0x2000, 0, HuPrcCurrentGet());
    process->user_data = sceneModels;
}

s32 InitializePuzzleScene(void)
{
    M411SceneModelSet *sceneModels;
    M411ModelMotionSlot *modelSlot;
    HUPROCESS *process;
    void *resourceData;
    s16 modelId;
    s16 modelId2;
    s32 i;

    sceneModels = &sceneModelSet;
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 47), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(resourceData);
    sceneModels->modelId = modelId;
    Hu3DModelCameraSet(modelId, 1);
    sceneModels->modelPosition.x = sceneModels->modelPosition.y = sceneModels->modelPosition.z = 0.0f;
    sceneModels->modelRotation.x = sceneModels->modelRotation.y = sceneModels->modelRotation.z = 0.0f;
    Hu3DModelPosSet(modelId, sceneModels->modelPosition.x, sceneModels->modelPosition.y, sceneModels->modelPosition.z);
    Hu3DModelRotSet(modelId, sceneModels->modelRotation.x, sceneModels->modelRotation.y, sceneModels->modelRotation.z);
    Hu3DModelShadowMapSet(modelId);
    backgroundTextureScrollId = Hu3DTexScrollCreate(modelId, "jimen01");
    Hu3DTexScrollPosMoveSet(backgroundTextureScrollId, -0.01f, 0.0f, 0.0f);
    foregroundTextureScrollId = Hu3DTexScrollCreate(modelId, "kusa01");
    Hu3DTexScrollPosMoveSet(foregroundTextureScrollId, -0.017f, 0.0f, 0.0f);
    textureScrollSpeed = 1.0f;
    Hu3DTexScrollPosMoveSet(backgroundTextureScrollId, -0.01f, 0.0f, 0.0f);
    Hu3DTexScrollPosMoveSet(foregroundTextureScrollId, -0.017f, 0.0f, 0.0f);
    HuAudFXPitchSet(sceneSoundEffectIds[0], 0);
    modelSlot = &sceneModels->characterAnimationSlots[0];
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 48), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(resourceData);
    modelSlot->modelId = modelId;
    Hu3DModelCameraSet(modelId, 1);
    Hu3DModelPosSet(modelId, sceneModels->modelPosition.x, sceneModels->modelPosition.y, sceneModels->modelPosition.z);
    Hu3DModelRotSet(modelId, sceneModels->modelRotation.x, sceneModels->modelRotation.y, sceneModels->modelRotation.z);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DMotionTimeSet(modelId, Hu3DMotionMaxTimeGet(modelId) / 4);
    modelSlot = &sceneModels->characterAnimationSlots[1];
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 49), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(resourceData);
    modelSlot->modelId = modelId;
    Hu3DModelCameraSet(modelId, 1);
    Hu3DModelPosSet(modelId, sceneModels->modelPosition.x, sceneModels->modelPosition.y, sceneModels->modelPosition.z);
    Hu3DModelRotSet(modelId, sceneModels->modelRotation.x, sceneModels->modelRotation.y, sceneModels->modelRotation.z);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DMotionTimeSet(modelId, Hu3DMotionMaxTimeGet(modelId) / 4);
    for (i = 0; i < 3; i++) {
        modelSlot = &sceneModels->characterAnimationSlots[i + 2];
        if (i == 0) {
            resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 50), HU_MEMNUM_OVL, HEAP_MODEL);
            modelSlot->modelId = modelId = modelId2 = Hu3DModelCreate(resourceData);
        }
        else {
            modelSlot->modelId = Hu3DModelLink(modelId2);
        }
        modelId = modelSlot->modelId;
        Hu3DModelCameraSet(modelId, 1);
        Hu3DModelAttrSet(modelId, HU3D_MOTATTR_SHAPE_LOOP);
    }
    modelSlot = &sceneModels->characterTransforms[0];
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 51), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(resourceData);
    modelSlot->modelId = modelId;
    Hu3DModelCameraSet(modelId, 1);
    for (i = 0; i < 5; i++) {
        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 52 + i), HU_MEMNUM_OVL, HEAP_MODEL);
        modelSlot->motionIds[i] = Hu3DJointMotion(modelId, resourceData);
    }
    modelSlot->modelRotation.x = modelSlot->modelRotation.y = modelSlot->modelRotation.z = 0.0f;
    Hu3DModelPosSet(modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
    Hu3DModelRotSet(modelId, 0.0f, -90.0f, 0.0f);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelId);
    Hu3DMotionSet(modelId, modelSlot->motionIds[0]);
    modelSlot = &sceneModels->sceneModelSlots[0];
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 57), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(resourceData);
    modelSlot->modelId = modelId;
    Hu3DModelCameraSet(modelId, 1);
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    process = HuPrcChildCreate(RunPuzzleItemEffect, 0x2000, 0x2000, 0, HuPrcCurrentGet());
    process->user_data = sceneModels;
}

HU3DPARTICLEHOOK particleUpdateHooks[] = { UpdateParticleEffect, UpdateParticleEffectAlt, UpdateParticleEffectSecondary, UpdateParticleEffectSecondaryAlt };

void LoadPuzzleSceneAssets(void)
{
    u32 stateValue;
    M411PlayerSceneState *playerState;
    void *resourceData;
    HSFTRANSFORM *characterTransform;
    HSFTRANSFORM *characterTransform2;
    ANIMDATA *animation;
    ANIMDATA *animation2;
    HUPROCESS *process;
    s16 modelId;
    s32 stateValue2;
    s32 i;
    s32 j;

    HuAudSndGrpSet(0x24);
    stateValue = GXGetTexBufferSize(320, 240, GX_TF_RGB5A3, GX_FALSE, 0);
    if (!cameraTransitionTextureBuffer) {
        cameraTransitionTextureBuffer = HuMemDirectMallocNum(HEAP_MODEL, stateValue, HU_MEMNUM_OVL);
    }
    Hu3DLayerHookSet(0, CopyCameraTransitionTexture);
    switch (puzzleMode) {
        case 0:
            CreatePuzzleSceneModels();
            break;
        case 1:
            CreatePuzzleCharacterModels();
            break;
        default:
            InitializePuzzleScene();
            break;
    }
    for (i = 0; i < 4; i++) {
        playerState = &playerSceneStates[i];
        playerState->playerIndex = i;
        playerState->animationFrame = GWPlayerCfg[i].pad_idx;
        playerState->cameraMask = i + 1;
        playerState->animationTimer = 0;
        playerState->animationTimer = -1;
        resourceData = HuDataSelHeapReadNum(puzzleResourceIds[puzzleVariant], HU_MEMNUM_OVL, HEAP_MODEL);
        playerState->modelId = Hu3DModelCreate(resourceData);
        playerState->characterNameAnimation = HuSprAnimMake(320, 240, 2);
        playerState->characterNameAnimation->bmp->data = cameraTransitionTextureBuffer;
        Hu3DAnimCreate(playerState->characterNameAnimation, playerState->modelId, puzzleModelNames[puzzleVariant]);
        Hu3DModelCameraSet(playerState->modelId, 1 << playerState->cameraMask);
        Hu3DModelPosSet(playerState->modelId, 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(playerState->modelId, 0.0f, 0.0f, 0.0f);
        Hu3DModelAttrSet(playerState->modelId, HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
        resourceData = HuDataSelHeapReadNum(puzzleResourceIds[puzzleVariant], HU_MEMNUM_OVL, HEAP_MODEL);
        playerState->modelPosition = Hu3DModelCreate(resourceData);
        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 14), HU_MEMNUM_OVL, HEAP_MODEL);
        Hu3DAnimCreate(resourceData, playerState->modelPosition, puzzleModelNames[puzzleVariant]);
        Hu3DModelCameraSet(playerState->modelPosition, 1 << playerState->cameraMask);
        Hu3DModelPosSet(playerState->modelPosition, 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(playerState->modelPosition, 0.0f, 0.0f, 0.0f);
        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 12), HU_MEMNUM_OVL, HEAP_MODEL);
        playerState->motionIds = Hu3DModelCreate(resourceData);
        playerState->scoreAnimation = HuSprAnimMake(320, 240, 2);
        playerState->scoreAnimation->bmp->data = cameraTransitionTextureBuffer;
        Hu3DAnimCreate(playerState->scoreAnimation, playerState->motionIds, puzzleModelNames[puzzleVariant]);
        Hu3DModelCameraSet(playerState->motionIds, 1 << playerState->cameraMask);
        Hu3DModelPosSet(playerState->motionIds, 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(playerState->motionIds, 0.0f, 0.0f, 0.0f);
        resourceData = HuDataSelHeapReadNum(puzzleResourceIds[puzzleVariant], HU_MEMNUM_OVL, HEAP_MODEL);
        playerState->playerCharacterCount = Hu3DModelCreate(resourceData);
        Hu3DModelCameraSet(playerState->playerCharacterCount, 1 << playerState->cameraMask);
        Hu3DModelPosSet(playerState->playerCharacterCount, 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(playerState->playerCharacterCount, 0.0f, 0.0f, 0.0f);
        Hu3DModelTPLvlSet(playerState->playerCharacterCount, 128.0f);
        Hu3DModelAttrSet(playerState->playerCharacterCount, HU3D_ATTR_NOCULL);
        for (j = 0; j < activeCameraCount; j++) {
            playerState->characterModelParts[j] = (HSFOBJECT *)Hu3DModelObjPtrGet(playerState->modelId, puzzlePieceModelNames[puzzleVariant][j]);
            if (!playerState->characterModelParts[j]) {
                OSReport("objname %s\n", puzzlePieceModelNames[puzzleVariant][j]);
                OSPanic("m411.c", 908, "ObjPtr");
            }
            characterTransform = &playerState->characterTransforms[j];
            characterTransform2 = &playerState->characterModelParts[j]->mesh.base;
            characterTransform->pos.x = characterTransform2->pos.x;
            characterTransform->pos.y = characterTransform2->pos.y;
            characterTransform->pos.z = characterTransform2->pos.z;
            characterTransform->rot.x = characterTransform2->rot.x;
            characterTransform->rot.y = characterTransform2->rot.y;
            characterTransform->rot.z = characterTransform2->rot.z;
            characterTransform->scale.x = characterTransform2->scale.x;
            characterTransform->scale.y = characterTransform2->scale.y;
            characterTransform->scale.z = characterTransform2->scale.z;
            playerState->characterShadowParts[j] = (HSFOBJECT *)Hu3DModelObjPtrGet(playerState->playerCharacterCount, puzzlePieceModelNames[puzzleVariant][j]);
            if (!playerState->characterShadowParts[j]) {
                OSReport("objname %s\n", puzzlePieceModelNames[puzzleVariant][j]);
                OSPanic("m411.c", 919, "ObjPtr");
            }
            playerState->characterEffectParts[j] = (HSFOBJECT *)Hu3DModelObjPtrGet(playerState->modelPosition, puzzlePieceModelNames[puzzleVariant][j]);
            if (!playerState->characterEffectParts[j]) {
                OSReport("objname %s\n", puzzlePieceModelNames[puzzleVariant][j]);
                OSPanic("m411.c", 925, "ObjPtr");
            }
        }
        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 0), HU_MEMNUM_OVL, HEAP_MODEL);
        playerState->playerIndex = Hu3DModelCreate(resourceData);
        Hu3DModelCameraSet(playerState->playerIndex, 1 << playerState->cameraMask);
        Hu3DModelPosSet(playerState->playerIndex, 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(playerState->playerIndex, 0.0f, 0.0f, 0.0f);
        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M411, 1 + GWPlayerCfg[i].character), HU_MEMNUM_OVL, HEAP_MODEL);
        modelId = Hu3DModelCreate(resourceData);
        Hu3DModelCameraSet(modelId, 1 << playerState->cameraMask);
        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 56 + GWPlayerCfg[i].character), HU_MEMNUM_OVL, HEAP_MODEL);
        animation = HuSprAnimRead(resourceData);
        modelId = HuSprCreate(animation, 0x4000, 0);
        playerState->spriteGroupId = HuSprGrpCreate(1);
        HuSprGrpMemberSet(playerState->spriteGroupId, 0, modelId);
        HuSprAttrSet(playerState->spriteGroupId, 0, 7);
        playerState->modelIdSecondary = CharModelCreate(characterModelIndices[GWPlayerCfg[i].character], 2);
        Hu3DModelCameraSet(playerState->modelIdSecondary, 1 << playerState->cameraMask);
        CharModelStepFxSet(characterModelIndices[GWPlayerCfg[i].character], 1);
        CharEffectLayerSet(2);
        playerState->positionX = 0.0f;
        switch (characterModelIndices[GWPlayerCfg[i].character]) {
            case 7:
                playerState->positionY = -400.0f;
                break;
            case 5:
                playerState->positionY = -330.0f;
                break;
            case 2:
            case 6:
                playerState->positionY = -375.0f;
                break;
            default:
                playerState->positionY = -350.0f;
                break;
        }
        playerState->positionZ = 500.0f;
        playerState->rotationX = playerState->rotationZ = 0.0f;
        playerState->rotationY = 180.0f;
        Hu3DModelPosSet(playerState->modelIdSecondary, playerState->positionX, playerState->positionY, playerState->positionZ);
        Hu3DModelRotSet(playerState->modelIdSecondary, playerState->rotationX, playerState->rotationY, playerState->rotationZ);
        Hu3DModelAttrSet(playerState->modelIdSecondary, HU3D_ATTR_DISPOFF);
        for (j = 0; j < 9; j++) {
            playerState->characterMotionIds[j] = CharMotionCreate(characterModelIndices[GWPlayerCfg[i].character], marioMotionResourceIds[j]);
        }
        CharMotionSet(characterModelIndices[GWPlayerCfg[i].character], playerState->characterMotionIds[0]);
        Hu3DModelAttrSet(playerState->modelIdSecondary, HU3D_MOTATTR_LOOP);
        if (puzzleMode == 0) {
            resourceData = HuDataSelHeapReadNum(characterModelResourceIds[GWPlayerCfg[i].character], HU_MEMNUM_OVL, HEAP_MODEL);
            playerState->motionFrame = Hu3DModelCreate(resourceData);
            Hu3DModelAttrSet(playerState->motionFrame, HU3D_ATTR_DISPOFF | HU3D_ATTR_CULL_FRONT);
            Hu3DModelCameraSet(playerState->motionFrame, 1);
            for (j = 0; j < 9; j++) {
                resourceData = HuDataSelHeapReadNum((u8)marioMotionResourceIds[j] + characterMotionResourceIds[GWPlayerCfg[i].character], HU_MEMNUM_OVL, HEAP_MODEL);
                playerState->modelMotionIds[j] = Hu3DJointMotion(playerState->motionFrame, resourceData);
            }
            Hu3DMotionSet(playerState->motionFrame, playerState->modelMotionIds[3]);
            Hu3DModelPosSet(playerState->motionFrame, playerState->positionX, 0.0f, playerState->positionZ);
            Hu3DModelRotSet(playerState->motionFrame, playerState->rotationX, playerState->rotationY, playerState->rotationZ);
            Hu3DModelScaleSet(playerState->motionFrame, 1.0f, -1.0f, 1.0f);
        }
        CharMotionDataClose(characterModelIndices[GWPlayerCfg[i].character]);
        animation2 = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M411, 13), HU_MEMNUM_OVL));
        modelId = Hu3DParticleCreate(animation2, 300);
        Hu3DModelPosSet(modelId, 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(modelId, 1.0f, 1.0f, 1.0f);
        Hu3DModelLayerSet(modelId, 1);
        Hu3DParticleColSet(modelId, 0xFF, 0xFF, 0);
        Hu3DParticleScaleSet(modelId, 0.0f);
        Hu3DParticleHookSet(modelId, particleUpdateHooks[playerState->playerIndex]);
        Hu3DModelCameraSet(modelId, 1 << playerState->cameraMask);
        process = HuPrcChildCreate(AnimatePlayerSelection, 0x2000, 0x3000, 0, HuPrcCurrentGet());
        process->user_data = playerState;
        playerProcesses[i] = process;
    }
    for (i = 0; i < activeCameraCount; i++) {
        playerSpriteIds[i] = i;
        playerModelIds[i] = frandmod(4);
    }
    for (i = 0; i < 50; i++) {
        j = frandmod(activeCameraCount);
        stateValue2 = frandmod(activeCameraCount);
        stateValue = playerSpriteIds[j];
        playerSpriteIds[j] = playerSpriteIds[stateValue2];
        playerSpriteIds[stateValue2] = stateValue;
    }
    HuPrcChildCreate(HandlePuzzleSceneExit, 0x1000, 0x1000, 0, HuPrcCurrentGet());
}

void DrawParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix, s32 playerIndex)
{
    float movementAngle;
    float movementAngle2;
    float movementAngle3;
    float movementAngle4;
    HU3DPARTICLEDATA *particleData;
    M411PlayerSceneState *playerState;
    s16 i;
    s16 j;
    s16 k;
    s32 stateValue;

    if (particle->count == 0) {
        particleData = particle->data;
        for (k = 0; k < particle->maxCnt; k++, particleData++) {
            particleData->accel.x = particleData->scale = 0.0f;
        }
    }
    particleData = particle->data;
    playerState = &playerSceneStates[model];
    if (playerState->animationTimer == 1) {
        for (i = 0; i < 4; i++) {
            movementAngle2 = 1.0f;
            for (j = 0, movementAngle = 0.0f; j < 24; j++, movementAngle += 15.0f) {
                for (k = 0; k < particle->maxCnt; k++, particleData++) {
                    if (particleData->accel.x == 0.0f) {
                        break;
                    }
                }
                if (k == particle->maxCnt) {
                    break;
                }
                movementAngle3 = movementAngle;
                movementAngle4 = movementAngle2 + 5.0f * frandf();
                particleData->pos.x = playerState->scaleY + 10.0f * i * sind(movementAngle3);
                particleData->pos.y = playerState->scaleZ + 10.0f * i * cosd(movementAngle3);
                particleData->pos.z = 50.0f;
                particleData->vel.x = movementAngle4 * sind(movementAngle3);
                particleData->vel.y = movementAngle4 * cosd(movementAngle3);
                particleData->vel.z = 0.0f;
                particleData->accel.x = 30.0f;
                particleData->accel.y = 360.0f * frandf();
                particleData->accel.z = 225.0f;
                particleData->scale = 40.0f + 10.0f * frandf();
                particleData->color.r = frandmod(20) + 235;
                particleData->color.g = frandmod(50) + 205;
                particleData->color.b = frandmod(50);
                particleData->color.a = 0xFF;
            }
        }
        playerState->animationTimer++;
        DCStoreRangeNoSync(particle->data, particle->maxCnt * sizeof(*particle->data));
    }
    particleData = particle->data;
    for (k = 0; k < particle->maxCnt; k++, particleData++) {
        if (particleData->accel.x == 0.0f) {
            continue;
        }
        VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
        particleData->vel.x *= 0.95f;
        particleData->vel.y *= 0.95f;
        particleData->accel.y += 20.0f;
        if (particleData->accel.y >= 180.0f && particleData->accel.y < 200.0f) {
            particleData->accel.z *= 0.6f;
        }
        if (particleData->accel.y >= 360.0f && particleData->accel.y < 380.0f) {
            particleData->accel.z *= 0.6f;
        }
        if (particleData->accel.y >= 360.0f) {
            particleData->accel.y -= 360.0f;
        }
        stateValue = particleData->accel.z + 30.0 * sind(particleData->accel.y);
        if (stateValue < 0) {
            stateValue = 0;
            particleData->accel.x = 0.0f;
        }
        particleData->color.a = stateValue;
        if (particleData->accel.x == 0.0f) {
            particleData->scale = 0.0f;
        }
    }
    PPCSync();
}

void UpdateParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    DrawParticleEffect(model, particle, matrix, 0);
}

void UpdateParticleEffectAlt(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    DrawParticleEffect(model, particle, matrix, 1);
}

void UpdateParticleEffectSecondary(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    DrawParticleEffect(model, particle, matrix, 2);
}

void UpdateParticleEffectSecondaryAlt(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    DrawParticleEffect(model, particle, matrix, 3);
}

void InitializePuzzleBoard(void)
{
    float movementAngle;
    float movementAngle2;
    M411ModelMotionSlot *modelSlot;
    s16 modelId;

    modelSlot = HuPrcCurrentGet()->user_data;
    modelId = modelSlot->spriteGroupId;
    Hu3DModelRotSet(modelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrReset(modelId, HU3D_ATTR_DISPOFF);
    movementAngle = -15.0f;
    movementAngle2 = -15.0f;
    Hu3DModelPosSet(modelId, modelSlot->modelRotation.x + 2.0 * sind(modelSlot->characterAnimationSlots.y) + movementAngle, modelSlot->modelRotation.y + movementAngle2,
        modelSlot->modelRotation.z + 2.0 * cosd(modelSlot->characterAnimationSlots.y));
    HuPrcVSleep();
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    HuPrcSleep(4);
    Hu3DModelAttrReset(modelId, HU3D_ATTR_DISPOFF);
    movementAngle = 15.0f;
    movementAngle2 = -15.0f;
    Hu3DModelPosSet(modelId, modelSlot->modelRotation.x + 2.0 * sind(modelSlot->characterAnimationSlots.y) + movementAngle, modelSlot->modelRotation.y + movementAngle2,
        modelSlot->modelRotation.z + 2.0 * cosd(modelSlot->characterAnimationSlots.y));
    HuPrcVSleep();
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    HuPrcSleep(4);
    Hu3DModelAttrReset(modelId, HU3D_ATTR_DISPOFF);
    movementAngle = -15.0f;
    movementAngle2 = 15.0f;
    Hu3DModelPosSet(modelId, modelSlot->modelRotation.x + 2.0 * sind(modelSlot->characterAnimationSlots.y) + movementAngle, modelSlot->modelRotation.y + movementAngle2,
        modelSlot->modelRotation.z + 2.0 * cosd(modelSlot->characterAnimationSlots.y));
    HuPrcVSleep();
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    HuPrcSleep(4);
    Hu3DModelAttrReset(modelId, HU3D_ATTR_DISPOFF);
    movementAngle = 15.0f;
    movementAngle2 = 15.0f;
    Hu3DModelPosSet(modelId, modelSlot->modelRotation.x + 2.0 * sind(modelSlot->characterAnimationSlots.y) + movementAngle, modelSlot->modelRotation.y + movementAngle2,
        modelSlot->modelRotation.z + 2.0 * cosd(modelSlot->characterAnimationSlots.y));
    HuPrcVSleep();
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    HuPrcSleep(4);
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void CreatePuzzleBoard(void)
{
    float movementAngle;
    float movementAngle2;
    float movementAngle3;
    float movementAngle4;
    M411ModelMotionSlot *modelSlot;
    HUPROCESS *process;

    modelSlot = HuPrcCurrentGet()->user_data;
    modelSlot->modelRotation.x = -500.0f + 1000.0f * frandf();
    modelSlot->modelRotation.y = 1200.0f;
    modelSlot->modelRotation.z = 0.0f;
    modelSlot->characterAnimationSlots.x = modelSlot->characterAnimationSlots.y = modelSlot->characterAnimationSlots.z = 0.0f;
    Hu3DModelAttrReset(modelSlot->modelId, HU3D_ATTR_DISPOFF);
    movementAngle = modelSlot->modelRotation.x;
    movementAngle2 = 0.0f;
    movementAngle3 = 1.0f + 2.0f * frandf();
    Hu3DModelScaleSet(modelSlot->modelId, movementAngle3, movementAngle3, movementAngle3);
    Hu3DModelScaleSet(modelSlot->spriteGroupId, movementAngle3, movementAngle3, movementAngle3);
    movementAngle4 = 10.0f + 20.0f * frandf();
    while (TRUE) {
        modelSlot->modelRotation.x = movementAngle + movementAngle4 * sind(movementAngle2);
        movementAngle2 += 10.0f;
        if (movementAngle2 == 180.0f || movementAngle2 == 360.0f) {
            movementAngle4 = 10.0f + 20.0f * frandf();
        }
        if (movementAngle2 > 360.0f) {
            movementAngle2 -= 360.0f;
        }
        modelSlot->modelRotation.y -= 3.0f;
        if (modelSlot->modelRotation.y < 0.0f) {
            break;
        }
        modelSlot->characterAnimationSlots.y += 10.0f;
        if (modelSlot->characterAnimationSlots.y > 360.0f) {
            modelSlot->characterAnimationSlots.y -= 360.0f;
        }
        Hu3DModelPosSet(modelSlot->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
        Hu3DModelRotSet(modelSlot->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        if (modelSlot->characterAnimationSlots.y == 350.0f) {
            process = HuPrcChildCreate(InitializePuzzleBoard, 0x2000, 0x2000, 0, HuPrcCurrentGet());
            process->user_data = modelSlot;
        }
        HuPrcVSleep();
    }
    HuPrcCurrentGet()->user_data = (void *)10000;
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunPuzzleIntro(void)
{
    float movementAngle;
    float movementAngle2;
    float movementAngle3;
    float movementAngle4;
    float movementAngle5;
    float movementAngle6;
    float movementAngle7;
    float movementAngle8;
    float movementAngle9;
    float movementAngle10;
    M411ModelMotionSlot *modelSlot;

    modelSlot = HuPrcCurrentGet()->user_data;
    movementAngle = -600.0f + 1200.0f * frandf();
    movementAngle2 = 1200.0f;
    movementAngle3 = -2900.0f;
    movementAngle4 = movementAngle5 = movementAngle6 = 0.0f;
    Hu3DModelAttrReset(modelSlot->modelId, HU3D_ATTR_DISPOFF);
    movementAngle8 = movementAngle;
    movementAngle7 = 0.0f;
    movementAngle9 = 1.0f + 2.0f * frandf();
    Hu3DModelScaleSet(modelSlot->modelId, movementAngle9, movementAngle9, movementAngle9);
    movementAngle10 = 10.0f + 20.0f * frandf();
    while (TRUE) {
        movementAngle = movementAngle8 + movementAngle10 * sind(movementAngle7);
        movementAngle7 += 10.0f;
        if (movementAngle7 > 360.0f) {
            movementAngle7 -= 360.0f;
        }
        movementAngle2 -= 3.0f;
        if (movementAngle2 < 0.0f) {
            break;
        }
        movementAngle5 += 10.0f;
        if (movementAngle5 > 360.0f) {
            movementAngle5 -= 360.0f;
        }
        Hu3DModelPosSet(modelSlot->modelId, movementAngle, movementAngle2, movementAngle3);
        Hu3DModelRotSet(modelSlot->modelId, movementAngle4, movementAngle5, movementAngle6);
        HuPrcVSleep();
    }
    HuPrcCurrentGet()->user_data = (void *)10000;
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunPuzzleCharacterIntro(void)
{
    float movementAngle;
    float savedValue;
    float movementAngle2;
    float movementAngle3;
    M411ModelMotionSlot *modelSlot;
    M411ModelMotionSlot *modelSlot2;
    s32 i;

    modelSlot = HuPrcCurrentGet()->user_data;
    if (modelSlot->characterMotionIds == 0) {
        movementAngle3 = 0.0f;
        modelSlot2 = &sceneModelSet.sceneModelSlots[2];
    }
    else {
        movementAngle3 = 180.0f;
        modelSlot2 = &sceneModelSet.sceneModelSlots[3];
    }
    movementAngle = modelSlot->modelRotation.x;
    savedValue = modelSlot->modelRotation.y;
    movementAngle2 = modelSlot->modelRotation.z;
    while (TRUE) {
        modelSlot->modelRotation.x = movementAngle + 200.0 * sind(movementAngle3);
        modelSlot->modelRotation.z = movementAngle2 + 200.0 * cosd(movementAngle3);
        movementAngle3 += 1.5f;
        if (!HuPrcCurrentGet()->user_data && ((movementAngle3 > 90.0f && movementAngle3 < 100.0f) || (movementAngle3 > 270.0f && movementAngle3 < 280.0f))) {
            break;
        }
        if (movementAngle3 > 360.0f) {
            movementAngle3 -= 360.0f;
        }
        modelSlot->characterAnimationSlots.y = movementAngle3 + 90.0f;
        Hu3DModelPosSet(modelSlot->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
        Hu3DModelRotSet(modelSlot->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        Hu3DModelPosSet(modelSlot2->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
        Hu3DModelRotSet(modelSlot2->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        HuPrcVSleep();
    }
    while (TRUE) {
        if (modelSlot->characterAnimationSlots.y > 180.0f) {
            modelSlot->characterAnimationSlots.y += 10.0f;
            if (modelSlot->characterAnimationSlots.y >= 360.0f) {
                break;
            }
        }
        if (modelSlot->characterAnimationSlots.y <= 180.0f) {
            modelSlot->characterAnimationSlots.y -= 10.0f;
            if (modelSlot->characterAnimationSlots.y <= 0.0f) {
                break;
            }
        }
        Hu3DModelRotSet(modelSlot->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        Hu3DModelRotSet(modelSlot2->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        HuPrcVSleep();
    }
    Hu3DModelRotSet(modelSlot->modelId, modelSlot->characterAnimationSlots.x, 0.0f, modelSlot->characterAnimationSlots.z);
    Hu3DModelRotSet(modelSlot2->modelId, modelSlot->characterAnimationSlots.x, 0.0f, modelSlot->characterAnimationSlots.z);
    Hu3DMotionShiftSet(modelSlot->modelId, modelSlot->motionIds[1], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    Hu3DMotionShiftSet(modelSlot2->modelId, modelSlot2->motionIds[1], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    for (i = 0; i < 120; i++) {
        if (movementAngle3 > 180.0f) {
            modelSlot->modelRotation.x += 1.6666666f;
            modelSlot->modelRotation.z += 0.8333333f;
        }
        else {
            modelSlot->modelRotation.z += 0.8333333f;
        }
        Hu3DModelPosSet(modelSlot->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
        Hu3DModelPosSet(modelSlot2->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
        HuPrcVSleep();
    }
    Hu3DMotionSet(modelSlot->modelId, modelSlot->motionIds[2]);
    Hu3DModelAttrReset(modelSlot->modelId, HU3D_MOTATTR_LOOP);
    Hu3DMotionSet(modelSlot2->modelId, modelSlot2->motionIds[2]);
    Hu3DModelAttrReset(modelSlot2->modelId, HU3D_MOTATTR_LOOP);
    while (Hu3DMotionEndCheck(modelSlot->modelId) == 0) {
        HuPrcVSleep();
    }
    if (modelSlot->characterMotionIds == 0) {
        Hu3DMotionShiftSet(modelSlot->modelId, modelSlot->motionIds[3], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        Hu3DMotionShiftSet(modelSlot2->modelId, modelSlot2->motionIds[3], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    }
    else {
        Hu3DMotionShiftSet(modelSlot->modelId, modelSlot->motionIds[4], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        Hu3DMotionShiftSet(modelSlot2->modelId, modelSlot2->motionIds[4], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    }
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunPuzzleBoardAnimation(void)
{
    M411ModelMotionSlot *modelSlot;
    M411ModelMotionSlot *modelSlot2;
    s32 i;

    modelSlot = &sceneModelSet.characterAnimationSlots[1];
    modelSlot2 = &sceneModelSet.characterAnimationSlots[2];
    for (i = 0; i < 30; i++) {
        modelSlot->characterAnimationSlots.y -= 2.6666667f;
        Hu3DModelRotSet(modelSlot->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        Hu3DModelRotSet(modelSlot2->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        HuPrcVSleep();
    }
    Hu3DMotionShiftSet(modelSlot->modelId, modelSlot->motionIds[2], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    Hu3DMotionShiftSet(modelSlot2->modelId, modelSlot2->motionIds[2], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    for (i = 0; i < 60; i++) {
        modelSlot->modelRotation.x += 2.5f;
        modelSlot->modelRotation.z += 3.3333333f;
        Hu3DModelPosSet(modelSlot->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
        Hu3DModelPosSet(modelSlot2->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
        HuPrcVSleep();
    }
    for (i = 0; i < 5; i++) {
        modelSlot->characterAnimationSlots.y -= 2.0f;
        Hu3DModelRotSet(modelSlot->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        Hu3DModelRotSet(modelSlot2->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        HuPrcVSleep();
    }
    Hu3DMotionShiftSet(modelSlot->modelId, modelSlot->motionIds[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    Hu3DMotionShiftSet(modelSlot2->modelId, modelSlot2->motionIds[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    while (TRUE) {
        if (sceneState >= 5) {
            break;
        }
        if (Hu3DMotionTimeGet(modelSlot->modelId) == 0.0f) {
            HuAudFXPlay(0x576);
        }
        HuPrcVSleep();
    }
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunPuzzleSceneProcess(void)
{
    M411SceneModelSet *sceneModels;
    HUPROCESS *characterModelProcesses[5];
    HUPROCESS *sceneAnimationProcesses[5];
    HUPROCESS *sceneModelProcesses[2];
    s32 boardSpawnDelay;
    s32 animationSpawnDelay;
    s32 resultAnimationStarted;
    s32 i;

    sceneModels = HuPrcCurrentGet()->user_data;
    for (i = 0; i < 5; i++) {
        characterModelProcesses[i] = NULL;
        sceneAnimationProcesses[i] = NULL;
    }
    for (i = 0; i < 2; i++) {
        sceneModelProcesses[i] = HuPrcChildCreate(RunPuzzleCharacterIntro, 0x2000, 0x2000, 0, HuPrcCurrentGet());
        sceneModelProcesses[i]->user_data = &sceneModels->sceneModelSlots[i];
    }
    boardSpawnDelay = animationSpawnDelay = 0;
    resultAnimationStarted = 0;
    while (TRUE) {
        if (boardSpawnDelay == 0) {
            for (i = 0; i < 5; i++) {
                if (!characterModelProcesses[i]) {
                    characterModelProcesses[i] = HuPrcChildCreate(CreatePuzzleBoard, 0x2000, 0x2000, 0, HuPrcCurrentGet());
                    characterModelProcesses[i]->user_data = &sceneModels->characterTransforms[i];
                    boardSpawnDelay = frandmod(20) + 20;
                    break;
                }
            }
        }
        else {
            boardSpawnDelay--;
        }
        if (animationSpawnDelay == 0) {
            for (i = 0; i < 5; i++) {
                if (!sceneAnimationProcesses[i]) {
                    sceneAnimationProcesses[i] = HuPrcChildCreate(RunPuzzleIntro, 0x2000, 0x2000, 0, HuPrcCurrentGet());
                    sceneAnimationProcesses[i]->user_data = &sceneModels->sceneAnimationSlots[i];
                    animationSpawnDelay = frandmod(20) + 20;
                    break;
                }
            }
        }
        else {
            animationSpawnDelay--;
        }
        if (sceneState == 4 && resultAnimationStarted == 0) {
            resultAnimationStarted = 1;
            HuPrcChildCreate(RunPuzzleBoardAnimation, 0x2000, 0x2000, 0, HuPrcCurrentGet());
            for (i = 0; i < 2; i++) {
                sceneModelProcesses[i]->user_data = NULL;
            }
        }
        HuPrcVSleep();
        for (i = 0; i < 5; i++) {
            if (characterModelProcesses[i] && characterModelProcesses[i]->user_data == (void *)10000) {
                characterModelProcesses[i] = NULL;
            }
            if (sceneAnimationProcesses[i] && sceneAnimationProcesses[i]->user_data == (void *)10000) {
                sceneAnimationProcesses[i] = NULL;
            }
        }
    }
}

void RunPuzzlePlayerProcess(void)
{
    float rotationAngle;
    M411ModelMotionSlot *modelSlot;
    s32 stateValue;
    s32 i;

    stateValue = (s32)HuPrcCurrentGet()->user_data;
    modelSlot = &sceneModelSet.characterTransforms[stateValue - 1];
    modelSlot->modelRotation.y = 0.0f;
    modelSlot->modelRotation.z = 200.0f;
    if (stateValue < 2) {
        modelSlot->modelRotation.x = 500.0f;
        modelSlot->characterAnimationSlots.x = modelSlot->characterAnimationSlots.z = 0.0f;
        modelSlot->characterAnimationSlots.y = -90.0f;
        rotationAngle = (120.0f - modelSlot->modelRotation.x) / 60.0f;
    }
    else {
        modelSlot->modelRotation.x = -500.0f;
        modelSlot->characterAnimationSlots.x = modelSlot->characterAnimationSlots.z = 0.0f;
        modelSlot->characterAnimationSlots.y = 90.0f;
        rotationAngle = (-120.0f - modelSlot->modelRotation.x) / 60.0f;
    }
    Hu3DModelAttrReset(modelSlot->modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelRotSet(modelSlot->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
    for (i = 0; i < 60; i++) {
        modelSlot->modelRotation.x += rotationAngle;
        Hu3DModelPosSet(modelSlot->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
        HuPrcVSleep();
    }
    Hu3DMotionShiftSet(modelSlot->modelId, modelSlot->motionIds[1], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    while (!Hu3DMotionEndCheck(modelSlot->modelId)) {
        HuPrcVSleep();
    }
    Hu3DMotionShiftSet(modelSlot->modelId, modelSlot->motionIds[2], 0.0f, 2.0f, HU3D_MOTATTR_LOOP);
    HuAudFXPlay(0x36);
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunPuzzleCursorProcess(void)
{
    float rotationAngle;
    s32 stateValue;
    s32 stateValue2;
    M411SceneModelSet *sceneModels;
    M411ModelMotionSlot *modelSlot;
    M411ModelMotionSlot *modelSlot2;

    sceneModels = HuPrcCurrentGet()->user_data;
    modelSlot = &sceneModels->characterAnimationSlots[0];
    modelSlot2 = &sceneModels->characterAnimationSlots[1];
    rotationAngle = 2.0f;
    stateValue = 0;
    sceneSoundEffectIds[0] = -1;
    while (TRUE) {
        if (sceneState == 4) {
            break;
        }
        modelSlot->modelRotation.x = 500.0f;
        modelSlot->modelRotation.y = 0.0f;
        modelSlot->modelRotation.z = 100.0f;
        modelSlot->characterAnimationSlots.x = modelSlot->characterAnimationSlots.z = 0.0f;
        modelSlot->characterAnimationSlots.y = -90.0f;
        Hu3DModelRotSet(modelSlot->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        Hu3DModelRotSet(modelSlot2->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        stateValue2 = 0;
        while (TRUE) {
            modelSlot->modelRotation.x -= rotationAngle;
            if (stateValue2 == 0 && modelSlot->modelRotation.x < 400.0f) {
                stateValue2 = 1;
                if (sceneSoundEffectIds[0] != -1) {
                    HuAudFXStop(sceneSoundEffectIds[0]);
                }
                sceneSoundEffectIds[0] = HuAudFXPlayVol(0x56E, 0);
                if (stateValue == 0) {
                    stateValue = 1;
                    HuAudFXVolSet(sceneSoundEffectIds[0], 0x7F);
                }
            }
            if (modelSlot->modelRotation.x < -300.0f) {
                break;
            }
            Hu3DModelPosSet(modelSlot->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
            Hu3DModelPosSet(modelSlot2->modelId, modelSlot->modelRotation.x + 35.0f, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
            HuPrcVSleep();
        }
        if (sceneState == 4) {
            break;
        }
        modelSlot->modelRotation.x = -500.0f;
        modelSlot->characterAnimationSlots.y = 90.0f;
        Hu3DModelRotSet(modelSlot->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        Hu3DModelRotSet(modelSlot2->modelId, modelSlot->characterAnimationSlots.x, modelSlot->characterAnimationSlots.y, modelSlot->characterAnimationSlots.z);
        stateValue2 = 0;
        while (TRUE) {
            modelSlot->modelRotation.x += rotationAngle;
            if (stateValue2 == 0 && modelSlot->modelRotation.x > -400.0f) {
                stateValue2 = 1;
                if (sceneSoundEffectIds[0] != -1) {
                    HuAudFXStop(sceneSoundEffectIds[0]);
                }
                sceneSoundEffectIds[0] = HuAudFXPlayVol(0x56E, 0);
            }
            if (modelSlot->modelRotation.x > 300.0f) {
                break;
            }
            Hu3DModelPosSet(modelSlot->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
            Hu3DModelPosSet(modelSlot2->modelId, modelSlot->modelRotation.x - 35.0f, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
            HuPrcVSleep();
        }
    }
    Hu3DModelAttrSet(modelSlot->modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(modelSlot2->modelId, HU3D_ATTR_DISPOFF);
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunPuzzleResultAnimation(void)
{
    M411SceneModelSet *sceneModels;
    HUPROCESS *process;

    sceneModels = HuPrcCurrentGet()->user_data;
    process = HuPrcChildCreate(RunPuzzleCursorProcess, 0x2000, 0x2000, 0, HuPrcCurrentGet());
    process->user_data = sceneModels;
    while (sceneState != 4) {
        HuPrcVSleep();
    }
    HuPrcSleep(0x122);
    process = HuPrcChildCreate(RunPuzzlePlayerProcess, 0x2000, 0x2000, 0, HuPrcCurrentGet());
    process->user_data = (void *)1;
    process = HuPrcChildCreate(RunPuzzlePlayerProcess, 0x2000, 0x2000, 0, HuPrcCurrentGet());
    process->user_data = (void *)2;
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunPuzzleResultEffect(void)
{
    M411ModelMotionSlot *modelSlot;
    s32 stateValue;

    stateValue = (s32)HuPrcCurrentGet()->user_data;
    modelSlot = &sceneModelSet.characterAnimationSlots[stateValue + 2];
    switch (stateValue) {
        case 0:
            modelSlot->modelRotation.x = 433.0f;
            break;
        case 1:
            modelSlot->modelRotation.x = 0.0f;
            break;
        default:
            modelSlot->modelRotation.x = -433.0f;
            break;
    }
    while (TRUE) {
        Hu3DModelAttrSet(modelSlot->modelId, HU3D_MOTATTR_LOOP);
        modelSlot->modelRotation.x += 2.0f * textureScrollSpeed;
        if (modelSlot->modelRotation.x > 650.0f) {
            modelSlot->modelRotation.x -= 1300.0f;
        }
        Hu3DModelPosSet(modelSlot->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
        HuPrcVSleep();
    }
}

void RunPuzzleCharacterAnimation(void)
{
    float movementAngle;
    float movementAngle2;
    float movementAngle3;
    float movementAngle4;
    float movementAngle5;
    float movementAngle6;
    float movementAngle7; // ! - uninitialized
    float savedValue;
    float movementAngle8;
    M411ModelMotionSlot savedValue2;
    M411ModelMotionSlot *modelSlot;
    M411ModelMotionSlot *modelSlot2;

    modelSlot = &savedValue2;
    modelSlot2 = &sceneModelSet.sceneModelSlots[0];
    modelSlot->modelId = Hu3DModelLink(modelSlot2->modelId);
    Hu3DModelCameraSet(modelSlot->modelId, 1);
    Hu3DModelAttrReset(modelSlot->modelId, HU3D_ATTR_DISPOFF);
    modelSlot->modelRotation.x = -500.0f + 100.0f * frandf();
    modelSlot->modelRotation.y = movementAngle4 = 200.0f + 500.0f * frandf();
    modelSlot->modelRotation.z = 400.0f;
    modelSlot->characterAnimationSlots.x = -80.0f;
    modelSlot->characterAnimationSlots.y = 190.0f;
    modelSlot->characterAnimationSlots.z = 45.0f;
    movementAngle = 5.5f + frandf();
    movementAngle2 = 2.0f;
    movementAngle3 = 0.4f + 0.2f * frandf();
    Hu3DModelScaleSet(modelSlot->modelId, movementAngle3, movementAngle3, movementAngle3);
    movementAngle5 = 0.0f;
    movementAngle6 = 0.0f;
#if NON_MATCHING
    movementAngle7 = 0.0f; // TODO PC is this right?
#endif
    while (TRUE) {
        modelSlot->modelRotation.x += movementAngle;
        movementAngle7 += 10.0f;
        if (movementAngle7 > 360.0f) {
            movementAngle7 -= 360.0f;
        }
        modelSlot->modelRotation.y = movementAngle4 + 20.0 * sind(movementAngle7);
        movementAngle4 -= movementAngle2;
        movementAngle += 0.1f;
        Hu3DModelPosSet(modelSlot->modelId, modelSlot->modelRotation.x, modelSlot->modelRotation.y, modelSlot->modelRotation.z);
        movementAngle5 += 20.0f;
        movementAngle6 += 20.0f;
        if (movementAngle5 > 360.0f) {
            movementAngle5 -= 360.0f;
        }
        if (movementAngle6 > 360.0f) {
            movementAngle6 -= 360.0f;
        }
        movementAngle8 = modelSlot->characterAnimationSlots.y + 40.0 * sind(movementAngle5);
        savedValue = modelSlot->characterAnimationSlots.x + 30.0 * sind(movementAngle5);
        Hu3DModelRotSet(modelSlot->modelId, modelSlot->characterAnimationSlots.x, movementAngle8, modelSlot->characterAnimationSlots.z);
        if (modelSlot->modelRotation.x > 400.0f) {
            break;
        }
        HuPrcVSleep();
    }
    Hu3DModelKill(modelSlot->modelId);
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunPuzzleItemAnimation(void)
{
    while (TRUE) {
        if (sceneState == 4) {
            break;
        }
        HuPrcChildCreate(RunPuzzleCharacterAnimation, 0x2000, 0x2000, 0, HuPrcCurrentGet());
        HuPrcSleep(20);
    }
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunPuzzleItemEffect(void)
{
    float scrollSpeed;
    M411SceneModelSet *sceneModels;
    HUPROCESS *process;
    M411ModelMotionSlot *modelSlot;
    M411PlayerSceneState *winningPlayerState;
    s32 i;

    sceneModels = HuPrcCurrentGet()->user_data;
    HuPrcChildCreate(RunPuzzleItemAnimation, 0x2000, 0x2000, 0, HuPrcCurrentGet());
    for (i = 0; i < 3; i++) {
        process = HuPrcChildCreate(RunPuzzleResultEffect, 0x2000, 0x2000, 0, HuPrcCurrentGet());
        process->user_data = (void *)i;
    }
    while (sceneState != 4) {
        HuPrcVSleep();
    }
    modelSlot = &sceneModelSet.characterTransforms[0];
    Hu3DMotionShiftSet(modelSlot->modelId, modelSlot->motionIds[1], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    for (scrollSpeed = 1.0f; scrollSpeed > 0.0f; scrollSpeed -= 0.033333335f) {
        textureScrollSpeed = scrollSpeed;
        Hu3DTexScrollPosMoveSet(backgroundTextureScrollId, -0.01f * scrollSpeed, 0.0f, 0.0f);
        Hu3DTexScrollPosMoveSet(foregroundTextureScrollId, -0.017f * scrollSpeed, 0.0f, 0.0f);
        HuAudFXPitchSet(sceneSoundEffectIds[0], -8191.0f * (1.0f - scrollSpeed));
        HuPrcVSleep();
    }
    textureScrollSpeed = 0.0f;
    Hu3DTexScrollPosMoveSet(backgroundTextureScrollId, -0.0f, 0.0f, 0.0f);
    Hu3DTexScrollPosMoveSet(foregroundTextureScrollId, -0.0f, 0.0f, 0.0f);
    HuAudFXPitchSet(sceneSoundEffectIds[0], -0x1FFF);
    HuPrcSleep(270);
    winningPlayerState = &playerSceneStates[winningPlayerIndex];
    Hu3DMotionSet(modelSlot->modelId, modelSlot->motionIds[2]);
    for (scrollSpeed = 0.0f; scrollSpeed < 1.0f; scrollSpeed += 0.033333335f) {
        textureScrollSpeed = scrollSpeed;
        Hu3DTexScrollPosMoveSet(backgroundTextureScrollId, -0.01f * scrollSpeed, 0.0f, 0.0f);
        Hu3DTexScrollPosMoveSet(foregroundTextureScrollId, -0.017f * scrollSpeed, 0.0f, 0.0f);
        HuAudFXPitchSet(sceneSoundEffectIds[0], -8191.0f * (1.0f - scrollSpeed));
        HuPrcVSleep();
    }
    textureScrollSpeed = 1.0f;
    Hu3DTexScrollPosMoveSet(backgroundTextureScrollId, -0.01f, 0.0f, 0.0f);
    Hu3DTexScrollPosMoveSet(foregroundTextureScrollId, -0.017f, 0.0f, 0.0f);
    HuAudFXPitchSet(sceneSoundEffectIds[0], 0);
    Hu3DMotionShiftSet(modelSlot->modelId, modelSlot->motionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    while (TRUE) {
        HuPrcVSleep();
    }
}

s8 GetPlayerStickX(s32 playerIndex, s32 stickAxis)
{
    M411PlayerSceneState *playerState;
    s8 stateValue2;

    playerState = &playerSceneStates[playerIndex];
    if (!GWPlayerCfg[playerIndex].iscom) {
        (stickAxis != 0) ? (stateValue2 = HuPadStkY[playerState->animationFrame]) : (stateValue2 = HuPadStkX[playerState->animationFrame]);
    }
    else {
        (stickAxis != 0) ? (stateValue2 = playerState->playerTeam) : (stateValue2 = playerState->animationVariant);
    }
    return stateValue2;
}

u8 GetPlayerStickY(s32 playerIndex, s32 stickAxis)
{
    M411PlayerSceneState *playerState;
    s8 stateValue2;

    playerState = &playerSceneStates[playerIndex];
    if (!GWPlayerCfg[playerIndex].iscom) {
        (stickAxis != 0) ? (stateValue2 = HuPadTrigR[playerState->animationFrame]) : (stateValue2 = HuPadTrigL[playerState->animationFrame]);
    }
    else {
        (stickAxis != 0) ? (stateValue2 = playerState->playerDifficulty) : (stateValue2 = playerState->selectionState);
    }
    return stateValue2;
}

u16 GetPlayerInputAngle(s32 playerIndex)
{
    M411PlayerSceneState *playerState;
    u16 stateValue2;

    playerState = &playerSceneStates[playerIndex];
    if (!GWPlayerCfg[playerIndex].iscom) {
        stateValue2 = HuPadBtnDown[playerState->animationFrame];
    }
    else {
        stateValue2 = playerState->animationFlags;
    }
    return stateValue2;
}

void UpdatePlayerCharacter(M411PlayerSceneState *playerState, s32 playerIndex)
{
    Vec lookPosition;
    Vec lookTarget;
    Vec cameraUp;
    Vec positionOffset;
    float modelScale;
    float modelScale2;
    float modelScale3;
    float modelScale4;
    float modelScale5;
    float modelScale6;
    float modelScale7;
    HSFTRANSFORM *characterTransform;
    HSFTRANSFORM *characterTransform2;
    HSFTRANSFORM *characterTransform3;
    HSFTRANSFORM *characterTransform4;
    s32 stateValue;
    s32 i;

    characterTransform = &playerState->characterModelParts[playerIndex]->mesh.base;
    characterTransform2 = &playerState->characterShadowParts[playerIndex]->mesh.base;
    characterTransform3 = &playerState->characterEffectParts[playerIndex]->mesh.base;
    characterTransform4 = &playerState->characterTransforms[playerIndex];
    modelScale = (characterTransform4->pos.x - characterTransform->pos.x) / 15.0f;
    modelScale3 = (characterTransform4->pos.z - characterTransform->pos.z) / 30.0f;
    modelScale2 = (characterTransform4->pos.y - characterTransform->pos.y) / 15.0f;
    HuSprAttrSet(playerState->spriteGroupId, 0, HUSPR_ATTR_REVERSE);
    omVibrate(playerState->playerIndex, 12, 6, 6);
    stateValue = 0;
    for (i = 0; i < 30; i++) {
        characterTransform->pos.z += modelScale3;
        characterTransform3->pos.z = characterTransform2->pos.z = characterTransform->pos.z - 5.0f;
        if (i < 15) {
            characterTransform->pos.x += modelScale;
            characterTransform->pos.y += modelScale2;
            characterTransform3->pos.x = characterTransform2->pos.x = characterTransform->pos.x;
            characterTransform3->pos.y = characterTransform2->pos.y = characterTransform->pos.y;
            characterTransform2->scale.x *= 0.95f;
            characterTransform2->scale.y *= 0.95f;
            lookPosition.x = characterTransform->pos.x + 60.0f;
            lookPosition.z = 100.0f;
            lookPosition.y = characterTransform->pos.y - 30.0f;
            Hu3D3Dto2D(&lookPosition, 1 << playerState->cameraMask, &lookTarget);
            modelScale4 = 290.0f * (playerState->playerIndex % 2) + lookTarget.x / 2;
            modelScale5 = 245.0f * (playerState->playerIndex / 2) + lookTarget.y / 2;
            HuSprGrpPosSet(playerState->spriteGroupId, modelScale4, modelScale5);
        }
        characterTransform3->pos.z += 10.0f;
        if (i < 14) {
            stateValue += 17;
        }
        else if (i < 28) {
            stateValue -= 17;
        }
        Hu3DModelTPLvlSet(playerState->modelPosition, 255 - stateValue);
        HuPrcVSleep();
    }
    characterTransform->pos.x = characterTransform4->pos.x;
    characterTransform->pos.y = characterTransform4->pos.y;
    characterTransform->pos.z = characterTransform4->pos.z;
    cameraUp.x = characterTransform->pos.x + 60.0f;
    cameraUp.z = 100.0f;
    cameraUp.y = characterTransform->pos.y - 30.0f;
    Hu3D3Dto2D(&cameraUp, 1 << playerState->cameraMask, &positionOffset);
    modelScale6 = 290.0f * (playerState->playerIndex % 2) + positionOffset.x / 2;
    modelScale7 = 245.0f * (playerState->playerIndex / 2) + positionOffset.y / 2;
    HuSprGrpPosSet(playerState->spriteGroupId, modelScale6, modelScale7);
    characterTransform2->pos.x = 5000.0f;
    characterTransform3->pos.x = 5000.0f;
}

void UpdatePlayerCharacterAlt(M411PlayerSceneState *playerState, s32 playerIndex)
{
    float movementAngle;
    HSFTRANSFORM *characterTransform;
    HSFTRANSFORM *characterTransform2;
    HSFTRANSFORM *characterTransform3;

    characterTransform = &playerState->characterModelParts[playerIndex]->mesh.base;
    characterTransform2 = &playerState->characterShadowParts[playerIndex]->mesh.base;
    characterTransform3 = &playerState->characterTransforms[playerIndex];
    for (movementAngle = 0.0f; movementAngle < 180.0f; movementAngle += 10.0f) {
        characterTransform->scale.x = 0.8f * characterTransform3->scale.x + characterTransform3->scale.x * (1.0 - sind(movementAngle)) * 0.2f;
        characterTransform->scale.y = 0.8f * characterTransform3->scale.y + characterTransform3->scale.y * (1.0 - sind(movementAngle)) * 0.2f;
        characterTransform->scale.z = 0.8f * characterTransform3->scale.z + characterTransform3->scale.z * (1.0 - sind(movementAngle)) * 0.2f;
        characterTransform2->scale.x = characterTransform->scale.x;
        characterTransform2->scale.y = characterTransform->scale.y;
        characterTransform2->scale.z = 0.8f * characterTransform->scale.z;
        HuPrcVSleep();
    }
    for (movementAngle = 180.0f; movementAngle < 360.0f; movementAngle += 20.0f) {
        characterTransform->scale.x = 0.9f * characterTransform3->scale.x + characterTransform3->scale.x * (1.0 - sind(movementAngle)) * 0.1f;
        characterTransform->scale.y = 0.9f * characterTransform3->scale.y + characterTransform3->scale.y * (1.0 - sind(movementAngle)) * 0.1f;
        characterTransform->scale.z = 0.9f * characterTransform3->scale.z + characterTransform3->scale.z * (1.0 - sind(movementAngle)) * 0.1f;
        characterTransform2->scale.x = characterTransform->scale.x;
        characterTransform2->scale.y = characterTransform->scale.y;
        characterTransform2->scale.z = 0.8f * characterTransform->scale.z;
        HuPrcVSleep();
    }
    characterTransform->scale.x = characterTransform3->scale.x;
    characterTransform->scale.y = characterTransform3->scale.y;
    characterTransform->scale.z = characterTransform3->scale.z;
    characterTransform2->scale.x = characterTransform->scale.x;
    characterTransform2->scale.y = characterTransform->scale.y;
    characterTransform2->scale.z = 0.8f * characterTransform->scale.z;
}

void UpdatePlayerCharacterState(M411PlayerSceneState *playerState, s32 playerIndex, s32 animationState)
{
    HSFTRANSFORM *characterTransform;
    HSFTRANSFORM *characterTransform2;
    float modelScale;
    float modelScale2;
    float modelScale3;
    float modelScale4;
    s32 i;

    characterTransform2 = &playerState->characterModelParts[playerIndex]->mesh.base;
    characterTransform = &playerState->characterShadowParts[playerIndex]->mesh.base;
    modelScale = characterTransform2->scale.x;
    modelScale2 = characterTransform2->scale.y;
    modelScale3 = characterTransform2->scale.z;
    characterTransform2->scale.x = characterTransform2->scale.y = characterTransform2->scale.z = 0.0f;
    characterTransform->scale.x = characterTransform->scale.y = characterTransform->scale.z = 0.0f;
    for (i = 0, modelScale4 = 0.0f; i < 30; i++, modelScale4 += 0.033333335f) {
        characterTransform2->scale.x += modelScale / 30.0f;
        characterTransform2->scale.y += modelScale2 / 30.0f;
        characterTransform2->scale.z += modelScale3 / 30.0f;
        characterTransform->scale.x = characterTransform2->scale.x;
        characterTransform->scale.y = characterTransform2->scale.y;
        characterTransform->scale.z = 0.8f * characterTransform2->scale.z;
        if (animationState == 0) {
            HuSprGrpScaleSet(playerState->spriteGroupId, modelScale4, modelScale4);
        }
        HuPrcVSleep();
    }
    characterTransform2->scale.x = modelScale;
    characterTransform2->scale.y = modelScale2;
    characterTransform2->scale.z = modelScale3;
    characterTransform->scale.x = characterTransform2->scale.x;
    characterTransform->scale.y = characterTransform2->scale.y;
    characterTransform->scale.z = 0.8f * characterTransform2->scale.z;
    if (animationState == 0) {
        HuSprAttrReset(playerState->spriteGroupId, 0, HUSPR_ATTR_NOANIM);
        HuSprGrpScaleSet(playerState->spriteGroupId, 1.0f, 1.0f);
        return;
    }
    HuSprAttrReset(playerState->spriteGroupId, 0, HUSPR_ATTR_REVERSE);
}

void UpdatePlayerEffects(void)
{
    float movementAngle;
    float movementAngle2;
    float movementAngle3;
    M411ModelStateReference *stateReference;
    HSFTRANSFORM *characterTransform;
    HSFTRANSFORM *characterTransform2;
    s32 loopIndex;
    s32 i;

    stateReference = HuPrcCurrentGet()->user_data;
    characterTransform = &stateReference->modelId->characterModelParts[stateReference->modelPosition]->mesh.base;
    characterTransform2 = &stateReference->modelId->characterShadowParts[stateReference->modelPosition]->mesh.base;
    HuAudFXPlay(0x56C);
    if (stateReference->modelIdSecondary != 0) {
        movementAngle2 = characterTransform->rot.z + 90.0f;
    }
    else {
        movementAngle2 = characterTransform->rot.z - 90.0f;
    }
    if (movementAngle2 >= 360.0f) {
        movementAngle2 -= 360.0f;
    }
    if (movementAngle2 < 0.0f) {
        movementAngle2 += 360.0f;
    }
    movementAngle3 = characterTransform->rot.z;
    movementAngle = 0.0f;
    for (i = 0, loopIndex = 15; i < loopIndex; i++) {
        if (stateReference->modelIdSecondary != 0) {
            characterTransform->rot.z = movementAngle3 + 100.0 * sind(movementAngle);
        }
        else {
            characterTransform->rot.z = movementAngle3 - 100.0 * sind(movementAngle);
        }
        characterTransform2->rot.z = characterTransform->rot.z;
        movementAngle += 6.0f;
        HuPrcVSleep();
    }
    movementAngle = 90.0f;
    for (i = 0, loopIndex = 3; i < loopIndex; i++) {
        movementAngle -= 8.333333f;
        if (stateReference->modelIdSecondary != 0) {
            characterTransform->rot.z = movementAngle3 + 100.0 * sind(movementAngle);
        }
        else {
            characterTransform->rot.z = movementAngle3 - 100.0 * sind(movementAngle);
        }
        characterTransform2->rot.z = characterTransform->rot.z;
        HuPrcVSleep();
    }
    characterTransform2->rot.z = movementAngle2;
    characterTransform->rot.z = movementAngle2;
    *stateReference->spriteGroupId = 0;
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void UpdatePuzzleCollisions(void)
{
    float positionValue;
    float positionValue2;
    HSFTRANSFORM *characterTransform;

    characterTransform = HuPrcCurrentGet()->user_data;
    HuPrcSleep(frandmod(30));
    positionValue = characterTransform->pos.x / 100.0f;
    positionValue2 = -7.0f;
    while (TRUE) {
        characterTransform->pos.y -= positionValue2;
        if (characterTransform->pos.y < -500.0f) {
            break;
        }
        positionValue2 += 0.5f;
        if (positionValue2 > 20.0f) {
            positionValue2 = 20.0f;
        }
        characterTransform->pos.x += positionValue;
        characterTransform->pos.z += 10.0f;
        HuPrcVSleep();
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void UpdatePlayerPosition(M411PlayerSceneState *playerState, s32 playerIndex)
{
    HSFTRANSFORM *characterTransform;

    characterTransform = &playerState->characterModelParts[playerIndex]->mesh.base;
    if (characterTransform->pos.x > 220.0f) {
        characterTransform->pos.x = 220.0f;
    }
    if (characterTransform->pos.x < -220.0f) {
        characterTransform->pos.x = -220.0f;
    }
    if (characterTransform->pos.y > 140.0f) {
        characterTransform->pos.y = 140.0f;
    }
    if (characterTransform->pos.y < -140.0f) {
        characterTransform->pos.y = -140.0f;
    }
}

void CheckPlayerCollision(s32 playerIndex, s32 otherPlayerIndex)
{
    switch (GWPlayerCfg[playerIndex].diff) {
        case 0:
            otherPlayerIndex *= 8;
            break;
        case 1:
            otherPlayerIndex *= 4;
            break;
        case 2:
            otherPlayerIndex *= 3;
            break;
        default:
            otherPlayerIndex *= 2;
            break;
    }
    if (otherPlayerIndex <= 1) {
        HuPrcVSleep();
    }
    else {
        HuPrcSleep(otherPlayerIndex + frandmod(otherPlayerIndex / 2));
    }
}

void UpdatePlayerAnimation(M411PlayerSceneState *playerState, s32 playerIndex, s32 animationState)
{
    Vec lookPosition;
    Vec lookTarget;
    Vec cameraUp;
    float movementAngle;
    HSFTRANSFORM *characterTransform;
    HSFTRANSFORM *characterTransform2;
    s32 stateValue;
    s32 stateValue2;

    playerIndex = playerSpriteIds[animationState];
    characterTransform = &playerState->characterModelParts[playerIndex]->mesh.base;
    playerState->animationVariant = playerState->playerTeam = 0;
    playerState->selectionState = playerState->playerDifficulty = 0;
    playerState->animationFlags = 0;
    switch (GWPlayerCfg[playerState->playerIndex].diff) {
        case 0:
            stateValue = animationState * 2 + 60;
            break;
        case 1:
            stateValue = animationState + 80;
            break;
        case 2:
            stateValue = animationState + 85;
            break;
        default:
            stateValue = animationState + 90;
            break;
    }
    if (stateValue > 100) {
        stateValue = 100;
    }
    if (playerState->animationTimer == -1) {
        if (frandmod(100) < stateValue || activeCameraCount == animationState + 1) {
            playerState->animationTimer = playerIndex;
        }
        else {
            stateValue2 = frandmod(activeCameraCount - animationState);
            playerState->animationTimer = playerSpriteIds[animationState + stateValue2];
        }
        playerState->effectTimer = -1;
        if ((GWPlayerCfg[playerState->playerIndex].diff == 2 || GWPlayerCfg[playerState->playerIndex].diff == 3) && (frandmod(100) < stateValue || activeCameraCount == animationState + 1)) {
            playerState->effectTimer = 10;
        }
        CheckPlayerCollision(playerState->playerIndex, 10);
    }
    characterTransform2 = &playerState->characterTransforms[playerState->animationTimer];
    HuSetVecF(&lookPosition, characterTransform2->pos.x, characterTransform2->pos.y, 0.0);
    HuSetVecF(&lookTarget, characterTransform->pos.x, characterTransform->pos.y, 0.0);
    HuSubVecF(&cameraUp, &lookPosition, &lookTarget);
    if (sqrtf(cameraUp.x * cameraUp.x + cameraUp.y * cameraUp.y) >= 40.0f) {
        movementAngle = atan2d(cameraUp.x, cameraUp.y); // TODO
        playerState->animationVariant = 50.0 * sind(movementAngle);
        playerState->playerTeam = 50.0 * cosd(movementAngle);
        if (playerState->effectTimer >= 0) {
            if (playerState->effectTimer-- == 0 && characterTransform->rot.z != 0.0f) {
                playerState->effectTimer = 10;
                if (characterTransform->rot.z - 90.0f == 0.0f) {
                    playerState->playerDifficulty = 50;
                }
                else {
                    playerState->selectionState = 50;
                }
            }
        }
    }
    else {
        CheckPlayerCollision(playerState->playerIndex, 1);
        if (frandmod(100) < stateValue || activeCameraCount == animationState + 1) {
            if (characterTransform->rot.z != 0.0f) {
                if (characterTransform->rot.z - 90.0f == 0.0f) {
                    playerState->playerDifficulty = 50;
                }
                else {
                    playerState->selectionState = 50;
                }
            }
            else {
                playerState->animationFlags = 0x100;
                playerState->animationTimer = -1;
            }
        }
        else {
            switch (frandmod(3)) {
                case 0:
                    playerState->playerDifficulty = 50;
                    break;
                case 1:
                    playerState->playerDifficulty = 50;
                    break;
                default:
                    playerState->animationFlags = 0x100;
                    playerState->animationTimer = -1;
                    break;
            }
            if (frandmod(100) < stateValue) {
                playerState->animationTimer = -1;
            }
        }
    }
}

void CreatePlayerSprites(void)
{
    s16 loopIndex3;
    s32 loopIndex4;
#if VERSION_PAL
    for (loopIndex3 = 127; loopIndex3 > 0; loopIndex3 -= 5) {
#else
    for (loopIndex3 = 127; loopIndex3 > 0; loopIndex3 -= 4) {
#endif
        for (loopIndex4 = 0; loopIndex4 < sceneSoundEffectCount; loopIndex4++) {
            HuAudFXVolSet(sceneSoundEffectIds[loopIndex4], loopIndex3);
        }
        HuPrcVSleep();
    }
    for (loopIndex4 = 0; loopIndex4 < sceneSoundEffectCount; loopIndex4++) {
        HuAudFXVolSet(sceneSoundEffectIds[loopIndex4], 0);
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void AnimateCameraViewports(void)
{
    s16 loopIndex2;
    s32 modelId;

#if VERSION_PAL
    for (loopIndex2 = 0; loopIndex2 < 127; loopIndex2 += 5) {
#else
    for (loopIndex2 = 0; loopIndex2 < 127; loopIndex2 += 4) {
#endif
        for (modelId = 0; modelId < sceneSoundEffectCount; modelId++) {
            HuAudFXVolSet(sceneSoundEffectIds[modelId], loopIndex2);
        }
        HuPrcVSleep();
    }
    for (modelId = 0; modelId < sceneSoundEffectCount; modelId++) {
        HuAudFXVolSet(sceneSoundEffectIds[modelId], 0x7F);
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

static inline void M411InlineFunc(M411PlayerSceneState *modelId, HSFTRANSFORM *loopIndex2)
{
    Vec lookPosition2;
    Vec lookTarget2;

    float movementAngle3;
    float movementAngle4;

    lookTarget2.x = 60.0f + loopIndex2->pos.x;
    lookTarget2.z = 100.0f;
    lookTarget2.y = loopIndex2->pos.y - 30.0f;
    Hu3D3Dto2D(&lookTarget2, 1 << modelId->cameraMask, &lookPosition2);
    movementAngle4 = 290.0f * (modelId->playerIndex % 2) + lookPosition2.x / 2.0f;
    movementAngle3 = 245.0f * (modelId->playerIndex / 2) + lookPosition2.y / 2.0f;
    HuSprGrpPosSet(modelId->spriteGroupId, movementAngle4, movementAngle3);
}

#define SOME_MACRO(x) ((x) >= 0 ? (x) : (-(x) > 20))

void AnimatePlayerSelection(void)
{
    f32 positionAngle;
    f32 movementRadius;
    HUPROCESS *process;
    M411PlayerSceneState *playerState;
    HSFTRANSFORM *characterTransform;
    s32 loopIndex;
    HSFTRANSFORM *shadowTransform;
    s32 spriteIndex;
    HSFTRANSFORM *cursorTransform;
    s32 moveDirection;
    Vec cursorPosition;
    Vec characterPosition;
    Vec positionDelta;
    M411ModelStateReference modelStateReference;
    s32 effectStarted;

    playerState = HuPrcCurrentGet()->user_data;
    playerState->animationState = 0;
    for (loopIndex = 0; loopIndex < activeCameraCount; loopIndex++) {
        characterTransform = &playerState->characterShadowParts[loopIndex]->mesh.base;
        characterTransform->pos.x = 5000.0f;
        characterTransform = &playerState->characterEffectParts[loopIndex]->mesh.base;
        characterTransform->pos.x = 5000.0f;
    }
    while (sceneState == 0) {
        HuPrcVSleep();
    }
    Hu3DModelAttrReset(playerState->modelIdSecondary, HU3D_ATTR_DISPOFF);
    for (loopIndex = 0; loopIndex < 30; loopIndex++) {
        if (puzzleMode == 0) {
            cameraPositions[playerState->cameraMask].z += 10.0f;
        }
        else {
            CZoomM[playerState->cameraMask] += 10.0f;
        }
        HuPrcVSleep();
    }
    Hu3DModelAttrReset(playerState->modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(playerState->motionIds, HU3D_ATTR_DISPOFF);
    if (playerState->playerIndex == 0) {
        HuAudFXPlay(0x56F);
    }
    for (loopIndex = 0; loopIndex < activeCameraCount; loopIndex++) {
        characterTransform = &playerState->characterModelParts[loopIndex]->mesh.base;
        positionAngle = atan2d(characterTransform->pos.x, characterTransform->pos.y);
        movementRadius = sqrtf(characterTransform->pos.x * characterTransform->pos.x + characterTransform->pos.y * characterTransform->pos.y);
        movementRadius *= 1.05f;
        characterTransform->pos.x = movementRadius * sind(positionAngle);
        characterTransform->pos.y = movementRadius * cosd(positionAngle);
    }
    omVibrate(playerState->playerIndex, 12, 6, 6);
    HuPrcSleep(30);
    CharMotionShiftSet(characterModelIndices[GWPlayerCfg[playerState->playerIndex].character], playerState->characterMotionIds[5], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    HuPrcChildCreate(CreatePlayerSprites, 0x2000, 0x1000, 0, HuPrcCurrentGet());
    if (playerState->playerIndex == 0) {
        HuAudFXPlay(0x56B);
    }
    for (loopIndex = 0; loopIndex < activeCameraCount; loopIndex++) {
        process = HuPrcChildCreate(UpdatePuzzleCollisions, 0x2000, 0x1000, 0, HuPrcCurrentGet());
        characterTransform = &playerState->characterModelParts[loopIndex]->mesh.base;
        process->user_data = characterTransform;
    }
    HuPrcSleep(40);
    HuPrcSleep(40);
    for (loopIndex = 0; loopIndex < 30; loopIndex++) {
        if (puzzleMode == 0) {
            cameraPositions[playerState->cameraMask].z -= 10.0f;
        }
        else {
            CZoomM[playerState->cameraMask] -= 10.0f;
        }
        HuPrcVSleep();
    }
    Hu3DModelAttrSet(playerState->modelIdSecondary, HU3D_ATTR_DISPOFF);
    HuPrcSleep(REFRESH_RATE);
    sceneState = 2;
    for (loopIndex = 0; loopIndex < activeCameraCount; loopIndex++) {
        effectStarted = 0;
        playerState->animationTimer = 0;
        playerState->animationTimer = -1;
        spriteIndex = playerSpriteIds[loopIndex];
        characterTransform = &playerState->characterModelParts[spriteIndex]->mesh.base;
        characterTransform = &playerState->characterShadowParts[spriteIndex]->mesh.base;
        if (loopIndex != 0) {
            cursorTransform = &playerState->characterTransforms[playerSpriteIds[loopIndex - 1]];
            characterTransform->pos.x = cursorTransform->pos.x;
            characterTransform->pos.y = cursorTransform->pos.y;
            characterTransform->pos.x = 20.0f + characterTransform->pos.x;
            characterTransform->pos.y = -20.0f + characterTransform->pos.y;
        }
        else {
            characterTransform->pos.x = characterTransform->pos.y = 0.0f;
            characterTransform->pos.x = 20.0f + characterTransform->pos.x;
            characterTransform->pos.y = -20.0f + characterTransform->pos.y;
            M411InlineFunc(playerState, characterTransform);
            HuSprAttrReset(playerState->spriteGroupId, 0, HUSPR_ATTR_DISPOFF);
        }
        characterTransform->pos.z = 50.0f;
        characterTransform->pos.z = characterTransform->pos.z - 5.0f;
        characterTransform->rot.z = characterTransform->rot.z = 90.0f * playerModelIds[loopIndex];
        UpdatePlayerCharacterState(playerState, spriteIndex, loopIndex);
        if (loopIndex == 0) {
            while (sceneState != 3) {
                HuPrcVSleep();
            }
        }
        while (TRUE) {
            if (GWPlayerCfg[playerState->playerIndex].iscom == 1) {
                UpdatePlayerAnimation(playerState, spriteIndex, loopIndex);
            }
            if (effectStarted == 0) {
                moveDirection = -1;
                if (GetPlayerStickY(playerState->playerIndex, 0) > 20) {
                    moveDirection = 1;
                }
                if (GetPlayerStickY(playerState->playerIndex, 1) > 20) {
                    moveDirection = 0;
                }
                if (moveDirection != -1) {
                    effectStarted = 1;
                    process = HuPrcChildCreate(UpdatePlayerEffects, 0x2000, 0x2000, 0, HuPrcCurrentGet());
                    process->user_data = &modelStateReference;
                    modelStateReference.modelId = playerState;
                    modelStateReference.modelPosition = spriteIndex;
                    modelStateReference.modelIdSecondary = moveDirection;
                    modelStateReference.spriteGroupId = &effectStarted;
                }
            }
            if ((GetPlayerInputAngle(playerState->playerIndex) & 0x100) && effectStarted == 0) {
                if (0.0f == characterTransform->rot.z) {
                    cursorTransform = &playerState->characterTransforms[spriteIndex];
                    HuSetVecF(&cursorPosition, cursorTransform->pos.x, cursorTransform->pos.y, 0.0);
                    HuSetVecF(&characterPosition, characterTransform->pos.x, characterTransform->pos.y, 0.0);
                    HuSubVecF(&positionDelta, &cursorPosition, &characterPosition);
                    if (sqrtf(positionDelta.x * positionDelta.x + positionDelta.y * positionDelta.y) < 40.0f) {
                        HuAudFXPlay(0x56D);
                        playerState->animationTimer = 1;
                        playerState->scaleY = characterTransform->pos.x;
                        playerState->scaleZ = characterTransform->pos.y;
                        UpdatePlayerCharacter(playerState, spriteIndex);
                        break;
                    }
                }
                UpdatePlayerCharacterAlt(playerState, spriteIndex);
            }
            playerState->scaleX = -1.0f;
            if (SOME_MACRO(GetPlayerStickX(playerState->playerIndex, 0)) != 0 || SOME_MACRO(GetPlayerStickX(playerState->playerIndex, 1)) != 0) {
                playerState->scaleX = atan2d(GetPlayerStickX(playerState->playerIndex, 0), GetPlayerStickX(playerState->playerIndex, 1));
            }
            if (-1.0f != playerState->scaleX) {
                characterTransform->pos.x += 5 * sind(playerState->scaleX);
                characterTransform->pos.y += 5 * cosd(playerState->scaleX);
                UpdatePlayerPosition(playerState, spriteIndex);
                characterTransform->pos.x = 20.0f + characterTransform->pos.x;
                characterTransform->pos.y = -20.0f + characterTransform->pos.y;
                M411InlineFunc(playerState, characterTransform);
            }
            HuPrcVSleep();
        }
    }
    playerState->animationState = 4;
    while (TRUE) {
        HuPrcVSleep();
    }
}

void SetCameraViewports(float viewportWidth, float viewportHeight)
{
    Hu3DCameraViewportSet(2, 10.0f, 10.0f, viewportWidth - 11.0f, viewportHeight - 11.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(2, 10, 10, viewportWidth - 11.0f, viewportHeight - 11.0f);
    Hu3DCameraViewportSet(4, viewportWidth + 2.0f, 10.0f, 630.0f - viewportWidth, viewportHeight - 11.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(4, viewportWidth + 2.0f, 10, 630.0f - viewportWidth, viewportHeight - 11.0f);
    Hu3DCameraViewportSet(8, 10.0f, viewportHeight + 2.0f, viewportWidth - 11.0f, 470.0f - viewportHeight, 0.0f, 1.0f);
    Hu3DCameraScissorSet(8, 10, viewportHeight + 2.0f, viewportWidth - 11.0f, 470.0f - viewportHeight);
    Hu3DCameraViewportSet(16, viewportWidth + 2.0f, viewportHeight + 2.0f, 630.0f - viewportWidth, 470.0f - viewportHeight, 0.0f, 1.0f);
    Hu3DCameraScissorSet(16, viewportWidth + 2.0f, viewportHeight + 2.0f, 630.0f - viewportWidth, 470.0f - viewportHeight);
}

void AnimateCameraOpening(void)
{
    float animationStep;
    float animationStep2;
    s32 i;

    Hu3DCameraViewportSet(2, 10.0f, 10.0f, 629.0f, 469.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(2, 10, 10, 629, 469);
    Hu3DCameraViewportSet(4, 642.0f, 10.0f, -10.0f, 469.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(4, 642, 10, -10, 469);
    Hu3DCameraViewportSet(8, 10.0f, 482.0f, 629.0f, -10.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(8, 10, 482, 629, -10);
    Hu3DCameraViewportSet(16, 642.0f, 482.0f, -10.0f, -10.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(16, 642, 482, -10, -10);
    HuPrcSleep(60);
    for (i = 0, animationStep = 645.0f, animationStep2 = 485.0f; i < 30; i++) {
        animationStep -= 10.833333f;
        animationStep2 -= 8.166667f;
        SetCameraViewports(animationStep, animationStep2);
        HuPrcVSleep();
    }
    Hu3DCameraViewportSet(2, 10.0f, 10.0f, 309.0f, 229.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(2, 10, 10, 309, 229);
    Hu3DCameraViewportSet(4, 322.0f, 10.0f, 310.0f, 229.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(4, 322, 10, 310, 229);
    Hu3DCameraViewportSet(8, 10.0f, 242.0f, 309.0f, 230.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(8, 10, 242, 309, 230);
    Hu3DCameraViewportSet(16, 322.0f, 242.0f, 310.0f, 230.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(16, 322, 242, 310, 230);
}

void AnimateCameraClosing(void)
{
    float animationValue;
    float animationValue2;
    float animationValue3;
    float animationValue4;
    float animationValue5;
    float animationValue6;
    s32 stateValue;
    s32 i;

    stateValue = (s32)HuPrcCurrentGet()->user_data;
    switch (stateValue) {
        case 0:
            animationValue = 645.0f;
            animationValue2 = 485.0f;
            break;
        case 1:
            animationValue = -5.0f;
            animationValue2 = 485.0f;
            break;
        case 2:
            animationValue = 645.0f;
            animationValue2 = -5.0f;
            break;
        default:
            animationValue = -5.0f;
            animationValue2 = -5.0f;
            break;
    }
    animationValue3 = 320.0f;
    animationValue4 = 240.0f;
    animationValue5 = (animationValue - animationValue3) / 30.0f;
    animationValue6 = (animationValue2 - animationValue4) / 30.0f;
    for (i = 0; i < 30; i++) {
        animationValue3 += animationValue5;
        animationValue4 += animationValue6;
        SetCameraViewports(animationValue3, animationValue4);
        HuPrcVSleep();
    }
    SetCameraViewports(animationValue, animationValue2);
    HuPrcChildCreate(AnimateCameraViewports, 0x2000, 0x1000, 0, HuPrcCurrentGet());
    HuPrcCurrentGet()->user_data = NULL;
    while (TRUE) {
        HuPrcVSleep();
    }
}

void AnimateWinningPlayer(s32 playerIndex)
{
    float rotationAngle;
    float rotationAngle2;
    s32 playerIndex2;
    M411PlayerSceneState *modelId;
    M411PlayerSceneState *playerState;
    HUPROCESS *process;
    M411ModelMotionSlot *modelSlot;
    s32 i;

    modelId = &playerSceneStates[playerIndex];
    for (i = 0, rotationAngle = 1.0f; i < 15; i++) {
        rotationAngle -= 0.06666667f;
        for (playerIndex2 = 0; playerIndex2 < 4; playerIndex2++) {
            playerState = &playerSceneStates[playerIndex2];
            HuSprGrpScaleSet(playerState->spriteGroupId, rotationAngle, rotationAngle);
        }
        HuPrcVSleep();
    }
    for (i = 0; i < 4; i++) {
        HuSprAttrSet(playerSceneStates[i].spriteGroupId, 0, HUSPR_ATTR_DISPOFF);
    }
    Hu3DModelAttrReset(modelId->motionIds, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(modelId->modelId, HU3D_ATTR_DISPOFF);
    process = HuPrcChildCreate(AnimateCameraClosing, 0x2000, 0x1000, 0, HuPrcCurrentGet());
    process->user_data = (void *)playerIndex;
    while (process->user_data) {
        HuPrcVSleep();
    }
    modelId->positionX = 500.0f;
    modelId->positionY = -280.0f;
    modelId->positionZ = 200.0f;
    modelId->rotationY = -90.0f;
    Hu3DModelAttrReset(modelId->modelIdSecondary, HU3D_ATTR_DISPOFF);
    Hu3DModelRotSet(modelId->modelIdSecondary, modelId->rotationX, modelId->rotationY, modelId->rotationZ);
    CharMotionSet(characterModelIndices[GWPlayerCfg[modelId->playerIndex].character], modelId->characterMotionIds[2]);
    Hu3DModelAttrSet(modelId->modelIdSecondary, HU3D_MOTATTR_LOOP);
    Hu3DModelCameraSet(modelId->modelIdSecondary, 1 << modelId->cameraMask);
    i = 0;
    while (TRUE) {
        modelId->positionX -= 5.0f;
        if (modelId->positionX <= 0.0f) {
            break;
        }
        if (i == 70 && puzzleMode == 2) {
            modelSlot = &sceneModelSet.characterTransforms[0];
            Hu3DMotionSet(modelSlot->modelId, modelSlot->motionIds[4]);
            HuAudFXPlay(0x41);
        }
        Hu3DModelPosSet(modelId->modelIdSecondary, modelId->positionX, modelId->positionY, modelId->positionZ);
        HuPrcVSleep();
        i++;
    }
    Hu3DModelPosSet(modelId->modelIdSecondary, 0.0f, modelId->positionY, modelId->positionZ);
    CharMotionShiftSet(characterModelIndices[GWPlayerCfg[modelId->playerIndex].character], modelId->characterMotionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    HuPrcSleep(10);
    for (i = 0; i < 9; i++) {
        modelId->rotationY -= 10.0f;
        Hu3DModelRotSet(modelId->modelIdSecondary, modelId->rotationX, modelId->rotationY, modelId->rotationZ);
        HuPrcVSleep();
    }
    HuPrcSleep(20);
    CharMotionShiftSet(characterModelIndices[GWPlayerCfg[modelId->playerIndex].character], modelId->characterMotionIds[3], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    if (puzzleMode == 1) {
        rotationAngle2 = -20.0f;
    }
    else {
        rotationAngle2 = -20.0f;
    }
    for (; rotationAngle2 < 0.0f; rotationAngle2 += 1.765f) {
        modelId->positionY -= rotationAngle2;
        Hu3DModelPosSet(modelId->modelIdSecondary, modelId->positionX, modelId->positionY, modelId->positionZ);
        HuPrcVSleep();
    }
    Hu3DModelShadowSet(modelId->modelIdSecondary);
}

void AnimateWinnerCamera(void)
{
    s32 i;

    for (i = 0; i < 67; i++) {
        cameraPositions->y += -10.0f;
        cameraPositions->z += -26.656717f;
        HuPrcVSleep();
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RotateWinningPlayer(void)
{
    M411PlayerSceneState *modelId;

    modelId = &playerSceneStates[winningPlayerIndex];
    while (TRUE) {
        modelId->rotationY += 10.0f;
        if (modelId->rotationY > 360.0f) {
            modelId->rotationY -= 360.0f;
        }
        Hu3DModelRotSet(modelId->modelIdSecondary, modelId->rotationX, modelId->rotationY, modelId->rotationZ);
        Hu3DModelRotSet(modelId->motionFrame, modelId->rotationX, modelId->rotationY, modelId->rotationZ);
        HuPrcVSleep();
    }
}

void PlayModeZeroWinnerAnimation(s32 playerIndex)
{
    float positionValue;
    float positionValue2;
    M411PlayerSceneState *modelId;
    s32 loopIndex;
    s32 i;

    modelId = &playerSceneStates[playerIndex];
    modelId->positionX = 0.0f;
    modelId->positionY = 930.0f;
    modelId->positionZ = 2790.0f;
    Hu3DModelAttrReset(modelId->motionFrame, HU3D_ATTR_DISPOFF);
    Hu3DModelCameraSet(modelId->modelIdSecondary, 1);
    HuPrcChildCreate(AnimateWinnerCamera, 0x2000, 0x1000, 0, HuPrcCurrentGet());
    positionValue = (-730.0f - modelId->positionZ) / 80.0f;
    positionValue2 = 0.0f;
    for (i = 0, loopIndex = 0; i < 80; i++) {
        if (loopIndex == 0) {
            modelId->positionY -= positionValue2;
            if (modelId->positionY < 0.0f) {
                loopIndex = 1;
                CharMotionShiftSet(characterModelIndices[GWPlayerCfg[modelId->playerIndex].character], modelId->characterMotionIds[4], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
                Hu3DMotionShiftSet(modelId->motionFrame, modelId->modelMotionIds[4], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
                modelId->positionY = 0.0f;
                break;
            }
        }
        positionValue2 += 1.2f;
        if (positionValue2 > 14.0f) {
            positionValue2 = 14.0f;
        }
        if (-530.0f < modelId->positionZ) {
            modelId->positionZ += positionValue;
        }
        Hu3DModelPosSet(modelId->modelIdSecondary, modelId->positionX, modelId->positionY, modelId->positionZ);
        Hu3DModelPosSet(modelId->motionFrame, modelId->positionX, -modelId->positionY, modelId->positionZ);
        HuPrcVSleep();
    }
    HuPrcSleep(5);
    while (!Hu3DMotionEndCheck(modelId->modelIdSecondary)) {
        HuPrcVSleep();
    }
    CharMotionShiftSet(characterModelIndices[GWPlayerCfg[modelId->playerIndex].character], modelId->characterMotionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    HuPrcSleep(30);
    HuAudSStreamPlay(1);
    PlayerFXPlay(modelId->playerIndex, 0x124);
    CharMotionShiftSet(characterModelIndices[GWPlayerCfg[modelId->playerIndex].character], modelId->characterMotionIds[7], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    Hu3DMotionShiftSet(modelId->motionFrame, modelId->modelMotionIds[7], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    HuPrcChildCreate(RotateWinningPlayer, 0x2000, 0x1000, 0, HuPrcCurrentGet());
}

void PlayModeOneWinnerAnimation(s32 playerIndex)
{
    float rotationAngle;
    float rotationAngle2;
    M411PlayerSceneState *modelId;
    s32 loopIndex;
    s32 i;

    modelId = &playerSceneStates[playerIndex];
    modelId->positionX = 0.0f;
    modelId->positionY = -40.0f;
    modelId->positionZ = 180.0f;
    Hu3DModelCameraSet(modelId->modelIdSecondary, 1);
    rotationAngle = (200.0f - modelId->positionZ) / 30.0f;
    rotationAngle2 = 0.0f;
    for (i = 0, loopIndex = 0; i < 30; i++) {
        if (loopIndex == 0) {
            modelId->positionY -= rotationAngle2;
            if (modelId->positionY < 0.0f) {
                loopIndex = 1;
                CharMotionShiftSet(characterModelIndices[GWPlayerCfg[modelId->playerIndex].character], modelId->characterMotionIds[2], 0.0f, 20.0f, HU3D_MOTATTR_LOOP);
                modelId->positionY = 0.0f;
            }
        }
        rotationAngle2 += 1.0f;
        if (rotationAngle2 > 10.0f) {
            rotationAngle2 = 10.0f;
        }
        modelId->positionZ += rotationAngle;
        Hu3DModelPosSet(modelId->modelIdSecondary, modelId->positionX, modelId->positionY, modelId->positionZ);
        HuPrcVSleep();
    }
    CharMotionShiftSet(characterModelIndices[GWPlayerCfg[modelId->playerIndex].character], modelId->characterMotionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    for (i = 0; i < 30; i++) {
        modelId->rotationY += 6.0f;
        Hu3DModelRotSet(modelId->modelIdSecondary, modelId->rotationX, modelId->rotationY, modelId->rotationZ);
        HuPrcVSleep();
    }
    HuAudSStreamPlay(1);
    CharMotionShiftSet(characterModelIndices[GWPlayerCfg[modelId->playerIndex].character], modelId->characterMotionIds[8], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
}

void PlayModeTwoWinnerAnimation(s32 playerIndex)
{
    float rotationAngle;
    float rotationAngle2;
    float rotationAngle3;
    M411PlayerSceneState *modelId;
    M411ModelMotionSlot *modelSlot;
    s32 loopIndex;
    s32 i;

    modelId = &playerSceneStates[playerIndex];
    modelSlot = &sceneModelSet.characterTransforms[0];
    modelId->positionX = 0.0f;
    modelId->positionY = 170.0f;
    modelId->positionZ = 900.0f;
    Hu3DModelCameraSet(modelId->modelIdSecondary, 1);
    rotationAngle2 = (150.0f - modelId->positionZ) / 30.0f;
    rotationAngle = -modelId->positionX / 30.0f;
    rotationAngle3 = 0.0f;
    for (i = 0, loopIndex = 0; i < 30; i++) {
        if (loopIndex == 0) {
            modelId->positionY -= rotationAngle3;
            if (modelId->positionY < 0.0f) {
                loopIndex = 1;
                CharMotionShiftSet(characterModelIndices[GWPlayerCfg[modelId->playerIndex].character], modelId->characterMotionIds[2], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
                modelId->positionY = 0.0f;
            }
        }
        rotationAngle3 += 1.0f;
        if (rotationAngle3 > 10.0f) {
            rotationAngle3 = 10.0f;
        }
        modelId->positionX += rotationAngle;
        modelId->positionZ += rotationAngle2;
        Hu3DModelPosSet(modelId->modelIdSecondary, modelId->positionX, modelId->positionY, modelId->positionZ);
        HuPrcVSleep();
    }
    CharMotionShiftSet(characterModelIndices[GWPlayerCfg[modelId->playerIndex].character], modelId->characterMotionIds[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    HuPrcSleep(30);
    rotationAngle = (100.0f - modelId->positionX) / 30.0f;
    rotationAngle2 = -modelId->positionZ / 30.0f;
    rotationAngle3 = 20.0f;
    Hu3DMotionShiftSet(modelSlot->modelId, modelSlot->motionIds[3], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(10);
    CharMotionShiftSet(characterModelIndices[GWPlayerCfg[modelId->playerIndex].character], modelId->characterMotionIds[6], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    for (i = 0; i < 30; i++) {
        modelId->positionY += rotationAngle3;
        rotationAngle3 -= 1.2f;
        modelId->positionX += rotationAngle;
        modelId->positionZ += rotationAngle2;
        Hu3DModelPosSet(modelId->modelIdSecondary, modelId->positionX, modelId->positionY, modelId->positionZ);
        modelId->rotationY += 3.0f;
        Hu3DModelRotSet(modelId->modelIdSecondary, modelId->rotationX, modelId->rotationY, modelId->rotationZ);
        HuPrcVSleep();
    }
    PlayerFXPlay(modelId->playerIndex, 0x124);
    Hu3DModelPosSet(modelId->modelIdSecondary, 0.0f, 0.0f, 0.0f);
    Hu3DModelRotSet(modelId->modelIdSecondary, 0.0f, 0.0f, 0.0f);
    switch (GWPlayerCfg[modelId->playerIndex].character) {
        case 1:
            Hu3DModelPosSet(modelId->modelIdSecondary, 0.0f, -80.0f, 0.0f);
            break;
        case 3:
            Hu3DModelPosSet(modelId->modelIdSecondary, 0.0f, -90.0f, 0.0f);
            break;
        case 2:
            Hu3DModelPosSet(modelId->modelIdSecondary, 0.0f, 20.0f, 0.0f);
            break;
    }
    Hu3DModelHookSet(modelSlot->modelId, "zaseki", modelId->modelIdSecondary);
    HuAudSStreamPlay(1);
}

void HandlePuzzleSceneExit(void)
{
    s32 i;

    while (TRUE) {
        if (omSysExitReq == 1) {
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
            HuAudFadeOut(1000);
            HuPrcSleep(60);
            for (i = 0; i < 4; i++) {
                HuSprAnimKill(playerSceneStates[i].characterNameAnimation);
                HuSprAnimKill(playerSceneStates[i].scoreAnimation);
            }
            MGSeqKillAll();
            omOvlReturnEx(1, 1);
            while (TRUE) {
                HuPrcVSleep();
            }
        }
        HuPrcVSleep();
    }
}

void RunPuzzleScene(void)
{
    s16 messageSequenceId;
    s16 sequenceValue;
    s16 messageSequenceId2;
    s32 audioSequenceId;
    s32 loopIndex;
    s32 playerIndex;
    s32 i;

    sceneState = 0;
    HuPrcSleep(1.0f + wipeData.duration);
    switch (puzzleMode) {
        case 0:
            sceneSoundEffectCount = 1;
            sceneSoundEffectIds[0] = HuAudFXPlay(0x573);
            break;
        case 1:
            sceneSoundEffectCount = 2;
            sceneSoundEffectIds[1] = HuAudFXPlay(0x577);
            break;
        default:
            sceneSoundEffectCount = 1;
            sceneSoundEffectIds[0] = HuAudFXPlay(0x571);
            break;
    }
    AnimateCameraOpening();
    HuPrcSleep(60);
    sceneState = 1;
    while (sceneState == 1) {
        HuPrcVSleep();
    }
    audioSequenceId = HuAudSeqPlay(0x45);
    messageSequenceId = MGSeqCreate(3, 0);
    while (MGSeqStatGet(messageSequenceId) != 0) {
        HuPrcVSleep();
    }
    sequenceValue = REFRESH_RATE * 60 + REFRESH_RATE - 1;
    messageSequenceId2 = MGSeqCreate(1, sequenceValue / REFRESH_RATE, -1, -1);
    sceneState = 3;
    while (TRUE) {
        for (i = 0, loopIndex = 0; i < 4; i++) {
            if (playerSceneStates[i].animationState == 4) {
                loopIndex++;
                playerIndex = i;
            }
        }
        if (loopIndex > 0) {
            if (loopIndex > 1) {
                loopIndex = frandmod(loopIndex);
                for (i = 0; i < 4; i++) {
                    if (playerSceneStates[i].animationState == 4 && loopIndex-- == 0) {
                        playerIndex = i;
                        break;
                    }
                }
            }
            break;
        }
        else if (sequenceValue < REFRESH_RATE - 1) {
            playerIndex = -1;
            break;
        }
        MGSeqParamSet(messageSequenceId2, 1, sequenceValue-- / REFRESH_RATE);
        HuPrcVSleep();
    }
    HuAudSeqFadeOut(audioSequenceId, 100);
    for (i = 0; i < 4; i++) {
        HuPrcKill(playerProcesses[i]);
    }
    MGSeqParamSet(messageSequenceId2, 2, -1);
    messageSequenceId = MGSeqCreate(3, 1);
    while (MGSeqStatGet(messageSequenceId) != 0) {
        HuPrcVSleep();
    }
    if (playerIndex != -1) {
        sceneState = 4;
        winningPlayerIndex = playerIndex;
        AnimateWinningPlayer(playerIndex);
        switch (puzzleMode) {
            case 0:
                PlayModeZeroWinnerAnimation(playerIndex);
                break;
            case 1:
                PlayModeOneWinnerAnimation(playerIndex);
                break;
            default:
                PlayModeTwoWinnerAnimation(playerIndex);
                break;
        }
    }
    if (playerIndex != -1) {
        if (!_CheckFlag(FLAG_ID_MAKE(1, 12))) {
            GWPlayer[playerIndex].coin_win = 10;
        }
        messageSequenceId = MGSeqCreate(5, 3, GWPlayerCfg[playerIndex].character, -1, -1, -1);
    }
    else {
        HuAudSStreamPlay(4);
        messageSequenceId = MGSeqCreate(3, 2);
    }
    HuPrcSleep(210);
    sceneState = 5;
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    HuPrcSleep(60);
    HuAudFadeOut(1);
    for (i = 0; i < 4; i++) {
        HuSprAnimKill(playerSceneStates[i].characterNameAnimation);
        HuSprAnimKill(playerSceneStates[i].scoreAnimation);
    }
    MGSeqKillAll();
    omOvlReturnEx(1, 1);
    while (TRUE) {
        HuPrcVSleep();
    }
}
