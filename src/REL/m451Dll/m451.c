#include "game/audio.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "ext_math.h"
#include "math.h"

#include "game/gamework.h"
#include "game/objsub.h"

#include "game/gamework_data.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/hsfex.h"
#endif

typedef struct PuzzlePlayerState {
    s16 modelIds[5];
    s16 spriteGroupId;
    s16 reservedPlayerValues[20];
    s32 playerIndex;
    s32 cameraIndex;
    s32 controllerIndex;
    s32 playerState;
    s32 movementPhase;
    float cursorState[9];
    HSFOBJECT *pieceModels[40];
    HSFOBJECT *overlayModels[40];
    HSFOBJECT *characterModels[40];
    HSFTRANSFORM pieceTransforms[40];
    u32 reservedFlags[2];
    s8 stickX;
    s8 stickY;
    s8 triggerRight;
    s8 triggerLeft;
    u16 buttonDown;
    s16 reservedInputValues[5];
    s32 selectedPieceIndex;
    s32 computerTurnTimer;
} PuzzlePlayerState;

typedef struct CameraViewParams {
    Vec rot;
    Vec pos;
    float zoom;
} PuzzleCameraPreset;

s16 directionalLightId;
s16 reservedModelIds[2];
s16 timerSpriteGroupId;
s16 reservedSpriteGroupId;
s32 puzzleGamePhase;
s32 winningPlayerIndex;
s32 shuffledPieceOrder[40];
s32 pieceRotationQuarters[40];
s32 puzzleMode;
s32 puzzlePresetIndex;
s32 puzzlePieceCount;
s32 reservedPuzzleValue;
PuzzlePlayerState puzzlePlayerState[1];
s32 reservedScratchValues[207];
OMOBJ *multiViewObject;
HUPROCESS *puzzlePlayerProcesses[1];
s32 reservedGlobalValue;

PuzzleCameraPreset puzzleCameraPresets[3]
    = { { { -64, 0, 0 }, { 0, 0, 0 }, 5000 }, { { -9.5, 0, 0 }, { 0, 0, -510 }, 1860 }, { { -5, 0, 0 }, { 0, 0, -2870 }, 5060 } };

s32 puzzleModelDataIds[3] = { DATA_MAKE_NUM(DATADIR_M451, 0x01), DATA_MAKE_NUM(DATADIR_M451, 0x02), DATA_MAKE_NUM(DATADIR_M451, 0x03) };

s32 puzzleAnimationDataIds[3] = { DATA_MAKE_NUM(DATADIR_M451, 0x05), DATA_MAKE_NUM(DATADIR_M451, 0x06), DATA_MAKE_NUM(DATADIR_M451, 0x07) };

char *puzzleAnimationNames[3] = { "puzzle00", "puzzle00", "puzzle00" };

char *puzzlePieceModelNames[3][40] = {
    {
        "puzzle2001",
        "puzzle2002",
        "puzzle2003",
        "puzzle2004",
        "puzzle2005",
        "puzzle2006",
        "puzzle2007",
        "puzzle2008",
        "puzzle2009",
        "puzzle2010",
        "puzzle2011",
        "puzzle2012",
        "puzzle2013",
        "puzzle2014",
        "puzzle2015",
        "puzzle2016",
        "puzzle2017",
        "puzzle2018",
        "puzzle2019",
        "puzzle2020",
    },
    {
        "puzzle3001",
        "puzzle3002",
        "puzzle3003",
        "puzzle3004",
        "puzzle3005",
        "puzzle3006",
        "puzzle3007",
        "puzzle3008",
        "puzzle3009",
        "puzzle3010",
        "puzzle3011",
        "puzzle3012",
        "puzzle3013",
        "puzzle3014",
        "puzzle3015",
        "puzzle3016",
        "puzzle3017",
        "puzzle3018",
        "puzzle3019",
        "puzzle3020",
        "puzzle3021",
        "puzzle3022",
        "puzzle3023",
        "puzzle3024",
        "puzzle3025",
        "puzzle3026",
        "puzzle3027",
        "puzzle3028",
        "puzzle3029",
        "puzzle3030",
    },
    {
        "puzzle4001",
        "puzzle4002",
        "puzzle4003",
        "puzzle4004",
        "puzzle4005",
        "puzzle4006",
        "puzzle4007",
        "puzzle4008",
        "puzzle4009",
        "puzzle4010",
        "puzzle4011",
        "puzzle4012",
        "puzzle4013",
        "puzzle4014",
        "puzzle4015",
        "puzzle4016",
        "puzzle4017",
        "puzzle4018",
        "puzzle4019",
        "puzzle4020",
        "puzzle4021",
        "puzzle4022",
        "puzzle4023",
        "puzzle4024",
        "puzzle4025",
        "puzzle4026",
        "puzzle4027",
        "puzzle4028",
        "puzzle4029",
        "puzzle4030",
        "puzzle4031",
        "puzzle4032",
        "puzzle4033",
        "puzzle4034",
        "puzzle4035",
        "puzzle4036",
        "puzzle4037",
        "puzzle4038",
        "puzzle4039",
        "puzzle4040",
    },
};

s32 characterModelAndMotionDataIds[] = {
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    DATA_MAKE_NUM(DATADIR_MARIOMDL1, 0x00),
    DATA_MAKE_NUM(DATADIR_LUIGIMDL1, 0x00),
    DATA_MAKE_NUM(DATADIR_PEACHMDL1, 0x00),
    DATA_MAKE_NUM(DATADIR_YOSHIMDL1, 0x00),
    DATA_MAKE_NUM(DATADIR_WARIOMDL1, 0x00),
    DATA_MAKE_NUM(DATADIR_DONKEYMDL1, 0x00),
    DATA_MAKE_NUM(DATADIR_DAISYMDL1, 0x00),
    DATA_MAKE_NUM(DATADIR_WALUIGIMDL1, 0x00),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x01),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x05),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x06),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x1B),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x20),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x21),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00),
};

s32 puzzleRecordIds[] = { 6, 7, 8 };

float timerDigitPositions[][2] = { -52, -9, 53, -8, 35, -8, 23, -8, 9, -8, -9, -8, -17, -8, -31, -8, 53, 8, 35, 8, 23, 8, 9, 8, -9, 8, -17, 8, -31, 8 };

s32 puzzleCompletionTimeLimits[] = { REFRESH_RATE * 60, REFRESH_RATE * 120, REFRESH_RATE * 180 };

s32 puzzleRewardIds[] = { 58, 54, 48 };

void PauseSceneVisibilityUpdate(void);
void PuzzleSceneSetup(void);
void PuzzleGameTask(void);

void ObjectSetup(void)
{
    Vec shadowPosition;
    Vec shadowUpDirection;
    Vec shadowTarget;
    s32 cameraIndex;
    HUPROCESS *objectManager;
    s32 selectedPuzzleMode;
    selectedPuzzleMode = GWSystem.mg_type;
    puzzleMode = selectedPuzzleMode;
    if (selectedPuzzleMode == -1) {
        puzzleMode = frandmod(3);
    }
    puzzlePresetIndex = puzzleMode;
    switch (puzzlePresetIndex) {
        case 0:
            puzzlePieceCount = 20;
            break;

        case 1:
            puzzlePieceCount = 30;
            break;

        default:
            puzzlePieceCount = 40;
            break;
    }
    objectManager = omInitObjMan(50, 8192);
    omGameSysInit(objectManager);
    Hu3DCameraCreate(3);
    Hu3DCameraPerspectiveSet(1, 15, 20, 25000, 1.2);
    Hu3DCameraPerspectiveSet(2, 15, 20, 25000, 1.2);
    Hu3DCameraViewportSet(1, 0, 0, 320, 240, 0, 1);
    Hu3DCameraViewportSet(2, 0, 0, 640, 480, 0, 1);
    Hu3DCameraScissorSet(1, 0, 0, 320, 240);
    Hu3DCameraScissorSet(2, 0, 0, 640, 480);
    multiViewObject = omAddObjEx(objectManager, 32730, 0, 0, -1, omOutViewMulti);
    multiViewObject->work[0] = 2;
    for (cameraIndex = 0; cameraIndex < 1; cameraIndex++) {
        CRotM[cameraIndex].x = puzzleCameraPresets[puzzleMode].rot.x;
        CRotM[cameraIndex].y = puzzleCameraPresets[puzzleMode].rot.y;
        CRotM[cameraIndex].z = puzzleCameraPresets[puzzleMode].rot.z;
        CenterM[cameraIndex].x = puzzleCameraPresets[puzzleMode].pos.x;
        CenterM[cameraIndex].y = puzzleCameraPresets[puzzleMode].pos.y;
        CenterM[cameraIndex].z = puzzleCameraPresets[puzzleMode].pos.z;
        CZoomM[cameraIndex] = puzzleCameraPresets[puzzleMode].zoom;
    }
    for (cameraIndex = 1; cameraIndex < multiViewObject->work[0]; cameraIndex++) {
        CRotM[cameraIndex].x = 0;
        CRotM[cameraIndex].y = 0;
        CRotM[cameraIndex].z = 0;
        CenterM[cameraIndex].x = 0;
        CenterM[cameraIndex].y = 10;
        CenterM[cameraIndex].z = 0;
        CZoomM[cameraIndex] = 1900;
    }
    Hu3DLighInit();
    directionalLightId = Hu3DGLightCreate(10000, 0, 10000, -1, 0, -1, 255, 255, 255);
    Hu3DGLightPosAimSet(directionalLightId, 0, 6796.88f, 9706.95f, 0, 0, 0);
    Hu3DGLightInfinitytSet(directionalLightId);
    shadowPosition.x = -1800;
    shadowPosition.y = 3600;
    shadowPosition.z = 0;
    shadowUpDirection.x = shadowUpDirection.z = 0;
    shadowUpDirection.y = 1;
    shadowTarget.x = -100;
    shadowTarget.y = 0;
    shadowTarget.z = 0;
    Hu3DShadowCreate(30, 20, 10000);
    Hu3DShadowTPLvlSet(0.4f);
    Hu3DShadowPosSet(&shadowPosition, &shadowUpDirection, &shadowTarget);
    Hu3DBGColorSet(0, 0, 0);
    PuzzleSceneSetup();
    HuPrcSetStat(HuPrcChildCreate(PauseSceneVisibilityUpdate, 4096, 4096, 0, HuPrcCurrentGet()), 12);
    HuPrcChildCreate(PuzzleGameTask, 4096, 8192, 0, HuPrcCurrentGet());
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
}

void PauseSceneVisibilityUpdate(void)
{
    PuzzlePlayerState *playerState = &puzzlePlayerState[0];
    while (1) {
        if (omPauseChk()) {
            Hu3DModelAttrSet(data->modelIds[0], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(data->modelIds[1], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(data->modelIds[2], HU3D_ATTR_DISPOFF);
            HuSprAttrSet(data->spriteGroupId, 0, HUSPR_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrReset(data->modelIds[0], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(data->modelIds[1], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(data->modelIds[2], HU3D_ATTR_DISPOFF);
            HuSprAttrReset(data->spriteGroupId, 0, HUSPR_ATTR_DISPOFF);
        }
        HuPrcVSleep();
    }
}

void TimerDigitPulseTask(void)
{
    s32 i;
    float angle = 0;
    while (1) {
        float scale;
        angle += 16.0f;
        scale = (0.2f * sind(angle)) + 1.0;
        for (i = 0; i < 8; i++) {
            HuSprScaleSet(timerSpriteGroupId, i + 1, scale, scale);
        }
        HuPrcVSleep();
    }
}

void TimerDigitsSet(s16 digitSpriteGroup, s16 firstDigitSpriteIndex, u32 elapsedFrames, s32 displayState)
{
    s32 firstSpriteMember;
    u32 elapsedTimeDigits[3];
    if (displayState != 2) {
        elapsedTimeDigits[0] = (elapsedFrames % REFRESH_RATE);
        elapsedFrames = (elapsedFrames - elapsedTimeDigits[0]) / REFRESH_RATE;
        elapsedTimeDigits[0] = 100.0f * (elapsedTimeDigits[0] / REFRESH_RATE_F);
        elapsedTimeDigits[1] = elapsedFrames % 60;
        elapsedFrames = (elapsedFrames - elapsedTimeDigits[1]) / 60;
        elapsedTimeDigits[2] = elapsedFrames;
    }
    else {
        elapsedTimeDigits[0] = 99;
        elapsedTimeDigits[1] = 59;
        elapsedTimeDigits[2] = 9;
    }
    firstSpriteMember = firstDigitSpriteIndex;
    if (displayState != 0) {
        firstSpriteMember += 7;
    }
    HuSprBankSet(digitSpriteGroup, firstSpriteMember, elapsedTimeDigits[0] % 10);
    HuSprBankSet(digitSpriteGroup, firstSpriteMember + 1, elapsedTimeDigits[0] / 10);
    HuSprBankSet(digitSpriteGroup, firstSpriteMember + 3, elapsedTimeDigits[1] % 10);
    HuSprBankSet(digitSpriteGroup, firstSpriteMember + 4, elapsedTimeDigits[1] / 10);
    HuSprBankSet(digitSpriteGroup, firstSpriteMember + 6, elapsedTimeDigits[2] % 10);
}

void PuzzleParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void PuzzlePlayerTask(void);
void PuzzlePlayerSetupTask(void);

void PuzzleSceneSetup(void)
{
    PuzzlePlayerState *playerState;
    s32 playerIndex;
    void *assetData;
    s32 pieceIndex;
    HSFTRANSFORM *pieceTransform;
    s16 createdResourceId;
    HSFTRANSFORM *savedPieceTransform;
    HUPROCESS *playerProcess;
    s32 savedPieceOrder;
    ANIMDATA *timerDigitAnimations[4];
    s32 randomSwapIndex;
    ANIMDATA *characterSpriteAnimation;
    ANIMDATA *particleAnimation;
    HuAudSndGrpSet(36);
    for (playerIndex = 0; playerIndex < 1; playerIndex++) {
        playerState = &puzzlePlayerState[playerIndex];
        playerState->playerIndex = playerIndex;
        playerState->controllerIndex = GWPlayerCfg[playerIndex].pad_idx;
        playerState->cameraIndex = playerIndex + 1;
        playerState->movementPhase = 0;
        playerState->selectedPieceIndex = -1;
        assetData = HuDataSelHeapReadNum(puzzleModelDataIds[puzzlePresetIndex], HU_MEMNUM_OVL, HEAP_MODEL);
        playerState->modelIds[0] = Hu3DModelCreate(assetData);
        assetData = HuDataSelHeapReadNum(puzzleAnimationDataIds[puzzleMode], HU_MEMNUM_OVL, HEAP_MODEL);
        Hu3DAnimCreate(assetData, playerState->modelIds[0], puzzleAnimationNames[puzzlePresetIndex]);
        Hu3DModelCameraSet(playerState->modelIds[0], 1 << playerState->cameraIndex);
        Hu3DModelPosSet(playerState->modelIds[0], 0, 0, 0);
        Hu3DModelRotSet(playerState->modelIds[0], 0, 0, 0);
        Hu3DModelAttrSet(playerState->modelIds[0], HU3D_ATTR_NOCULL);
        assetData = HuDataSelHeapReadNum(puzzleModelDataIds[puzzlePresetIndex], HU_MEMNUM_OVL, HEAP_MODEL);
        playerState->modelIds[1] = Hu3DModelCreate(assetData);
        assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M451, 0x0D), HU_MEMNUM_OVL, HEAP_MODEL);
        Hu3DAnimCreate(assetData, playerState->modelIds[1], puzzleAnimationNames[puzzlePresetIndex]);
        Hu3DModelCameraSet(playerState->modelIds[1], 1 << playerState->cameraIndex);
        Hu3DModelPosSet(playerState->modelIds[1], 0, 0, 0);
        Hu3DModelRotSet(playerState->modelIds[1], 0, 0, 0);
        assetData = HuDataSelHeapReadNum(puzzleModelDataIds[puzzlePresetIndex], HU_MEMNUM_OVL, HEAP_MODEL);
        playerState->modelIds[2] = Hu3DModelCreate(assetData);
        Hu3DModelCameraSet(playerState->modelIds[2], 1 << playerState->cameraIndex);
        Hu3DModelPosSet(playerState->modelIds[2], 0, 0, 0);
        Hu3DModelRotSet(playerState->modelIds[2], 0, 0, 0);
        Hu3DModelTPLvlSet(playerState->modelIds[2], 128);
        Hu3DModelAttrSet(playerState->modelIds[2], HU3D_ATTR_NOCULL);
        for (pieceIndex = 0; pieceIndex < puzzlePieceCount; pieceIndex++) {
            playerState->pieceModels[pieceIndex] = Hu3DModelObjPtrGet(playerState->modelIds[0], puzzlePieceModelNames[puzzlePresetIndex][pieceIndex]);
            if (!playerState->pieceModels[pieceIndex]) {
                OSReport("PeaceNo %d\n", puzzlePresetIndex);
                OSReport("objname %s\n", puzzlePieceModelNames[puzzlePresetIndex][pieceIndex]);
                OSPanic("m451.c", 543, "ObjPtr1");
            }
            savedPieceTransform = &playerState->pieceTransforms[pieceIndex];
            pieceTransform = &playerState->pieceModels[pieceIndex]->mesh.base;
            pieceTransform->pos.z += -10;
            savedPieceTransform->pos.x = pieceTransform->pos.x;
            savedPieceTransform->pos.y = pieceTransform->pos.y;
            savedPieceTransform->pos.z = pieceTransform->pos.z;
            savedPieceTransform->rot.x = pieceTransform->rot.x;
            savedPieceTransform->rot.y = pieceTransform->rot.y;
            savedPieceTransform->rot.z = pieceTransform->rot.z;
            savedPieceTransform->scale.x = pieceTransform->scale.x;
            savedPieceTransform->scale.y = pieceTransform->scale.y;
            savedPieceTransform->scale.z = pieceTransform->scale.z;
            playerState->overlayModels[pieceIndex] = Hu3DModelObjPtrGet(playerState->modelIds[2], puzzlePieceModelNames[puzzlePresetIndex][pieceIndex]);
            if (!playerState->overlayModels[pieceIndex]) {
                OSReport("objname %s\n", puzzlePieceModelNames[puzzlePresetIndex][pieceIndex]);
                OSPanic("m451.c", 555, "ObjPtr2");
            }
            playerState->characterModels[pieceIndex] = Hu3DModelObjPtrGet(playerState->modelIds[1], puzzlePieceModelNames[puzzlePresetIndex][pieceIndex]);
            if (!playerState->characterModels[pieceIndex]) {
                OSReport("objname %s\n", puzzlePieceModelNames[puzzlePresetIndex][pieceIndex]);
                OSPanic("m451.c", 561, "ObjPtr3");
            }
        }
        assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M451, 0x00), HU_MEMNUM_OVL, HEAP_MODEL);
        createdResourceId = Hu3DModelCreate(assetData);
        Hu3DModelCameraSet(createdResourceId, 1 << playerState->cameraIndex);
        Hu3DModelPosSet(createdResourceId, 0, 0, 0);
        Hu3DModelRotSet(createdResourceId, 0, 0, 0);
        assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M451, 0x04), HU_MEMNUM_OVL, HEAP_MODEL);
        playerState->modelIds[4] = Hu3DModelCreate(assetData);
        Hu3DModelCameraSet(playerState->modelIds[4], 1 << playerState->cameraIndex);
        Hu3DModelPosSet(playerState->modelIds[4], 0, 0, 0);
        Hu3DModelRotSet(playerState->modelIds[4], 0, 0, 0);
        assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M451, 0x08) + puzzlePresetIndex, HU_MEMNUM_OVL, HEAP_MODEL);
        Hu3DAnimCreate(assetData, playerState->modelIds[4], "s3puzzle00");
        assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 0x38) + GWPlayerCfg[playerIndex].character, HU_MEMNUM_OVL, HEAP_MODEL);
        characterSpriteAnimation = HuSprAnimRead(assetData);
        createdResourceId = HuSprCreate(characterSpriteAnimation, 16384, 0);
        playerState->spriteGroupId = HuSprGrpCreate(1);
        HuSprGrpMemberSet(playerState->spriteGroupId, 0, createdResourceId);
        HuSprAttrSet(playerState->spriteGroupId, 0, 7);
        particleAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M451, 0x0C), HU_MEMNUM_OVL));
        createdResourceId = Hu3DParticleCreate(particleAnimation, 100);
        Hu3DModelPosSet(createdResourceId, 0, 0, 0);
        Hu3DModelScaleSet(createdResourceId, 1, 1, 1);
        Hu3DModelLayerSet(createdResourceId, 1);
        Hu3DParticleHookSet(createdResourceId, PuzzleParticleUpdate);
        Hu3DModelCameraSet(createdResourceId, 1 << playerState->cameraIndex);
        playerProcess = HuPrcChildCreate(PuzzlePlayerTask, 8192, 12288, 0, HuPrcCurrentGet());
        playerProcess->user_data = playerState;
        puzzlePlayerProcesses[playerIndex] = playerProcess;
    }
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 0x30), HU_MEMNUM_OVL, HEAP_MODEL);
    timerDigitAnimations[0] = HuSprAnimRead(assetData);
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 0x31), HU_MEMNUM_OVL, HEAP_MODEL);
    timerDigitAnimations[1] = HuSprAnimRead(assetData);
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 0x37), HU_MEMNUM_OVL, HEAP_MODEL);
    timerDigitAnimations[2] = HuSprAnimRead(assetData);
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M451, 0x0B), HU_MEMNUM_OVL, HEAP_MODEL);
    timerDigitAnimations[3] = HuSprAnimRead(assetData);
    timerSpriteGroupId = HuSprGrpCreate(16);
    createdResourceId = HuSprCreate(timerDigitAnimations[3], 16385, 0);
    HuSprGrpMemberSet(timerSpriteGroupId, 0, createdResourceId);
    HuSprPosSet(timerSpriteGroupId, 0, 0, 0);
    HuSprTPLvlSet(timerSpriteGroupId, 0, 0.5f);
    HuSprColorSet(timerSpriteGroupId, 0, 0, 0, 0);
    for (playerIndex = 0; playerIndex < 14; playerIndex++) {
        if (playerIndex >= 7) {
            createdResourceId = HuSprCreate(timerDigitAnimations[1], 16384, 0);
        }
        else {
            createdResourceId = HuSprCreate(timerDigitAnimations[0], 16384, 0);
        }
        HuSprGrpMemberSet(timerSpriteGroupId, playerIndex + 2, createdResourceId);
        HuSprPosSet(timerSpriteGroupId, playerIndex + 2, timerDigitPositions[playerIndex + 1][0], timerDigitPositions[playerIndex + 1][1]);
        pieceIndex = 0;
        if (playerIndex == 2 || playerIndex == 9) {
            pieceIndex = 11;
        }
        if (playerIndex == 5 || playerIndex == 12) {
            pieceIndex = 10;
        }
        HuSprBankSet(timerSpriteGroupId, playerIndex + 2, pieceIndex);
    }
    createdResourceId = HuSprCreate(timerDigitAnimations[2], 16384, 0);
    HuSprGrpMemberSet(timerSpriteGroupId, 1, createdResourceId);
    HuSprPosSet(timerSpriteGroupId, 1, timerDigitPositions[0][0], timerDigitPositions[0][1]);
    HuSprGrpPosSet(timerSpriteGroupId, 280, 61);
    TimerDigitsSet(timerSpriteGroupId, 2, GWMGRecordGet(puzzleRecordIds[puzzlePresetIndex]), 0);
    TimerDigitsSet(timerSpriteGroupId, 2, 0, 1);
    for (playerIndex = 0; playerIndex < puzzlePieceCount; playerIndex++) {
        shuffledPieceOrder[playerIndex] = playerIndex;
        pieceRotationQuarters[playerIndex] = frandmod(4);
    }
    for (playerIndex = 0; playerIndex < 50; playerIndex++) {
        pieceIndex = frandmod(puzzlePieceCount);
        randomSwapIndex = frandmod(puzzlePieceCount);
        savedPieceOrder = shuffledPieceOrder[pieceIndex];
        shuffledPieceOrder[pieceIndex] = shuffledPieceOrder[randomSwapIndex];
        shuffledPieceOrder[randomSwapIndex] = savedPieceOrder;
    }
    HuPrcChildCreate(PuzzlePlayerSetupTask, 4096, 4096, 0, HuPrcCurrentGet());
}

void PuzzleParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float particleAngle;
    float particleAngleStep;
    float particleSpeed;
    float ringRadius;

    HU3DPARTICLEDATA *particleData;
    s16 particleIndex;
    PuzzlePlayerState *playerState;
    s16 burstRingIndex;
    s16 particleStep;
    s32 particleAlpha;

    if (particle->count == 0) {
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->accel.x = particleData->scale = 0;
            particleData->color.r = 255;
            particleData->color.g = 255;
            particleData->color.b = 0;
        }
    }
    particleData = particle->data;
    playerState = &puzzlePlayerState[0];
    if (playerState->movementPhase == 1) {
        for (burstRingIndex = 0; burstRingIndex < 4; burstRingIndex++) {
            ringRadius = 1.0f;
            for (particleStep = 0, particleAngleStep = 0; particleStep < 24; particleStep++, particleAngleStep += 15.0f) {
                for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                    if (0.0f == particleData->accel.x) {
                        break;
                    }
                }
                if (particleIndex == particle->maxCnt) {
                    break;
                }
                particleAngle = particleAngleStep;
                particleSpeed = ringRadius + (5.0f * frandf());
                particleData->pos.x = playerState->cursorState[7] + ((burstRingIndex * 10.0f) * sind(particleAngle));
                particleData->pos.y = playerState->cursorState[8] + ((burstRingIndex * 10.0f) * cosd(particleAngle));
                particleData->pos.z = 50;
                particleData->vel.x = particleSpeed * sind(particleAngle);
                particleData->vel.y = particleSpeed * cosd(particleAngle);
                particleData->vel.z = 0;
                particleData->accel.x = 30.0f;
                particleData->accel.y = 360.0f * frandf();
                particleData->accel.z = 225;
                particleData->scale = (5.0f * frandf()) + 25.0f;
                particleData->color.r = 255;
                particleData->color.g = 255;
                particleData->color.b = 0;
                particleData->color.a = 255;
            }
        }
        playerState->movementPhase++;
        DCStoreRangeNoSync(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
    }
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (0.0f == particleData->accel.x) {
            continue;
        }
        VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
        particleData->vel.x *= 0.95f;
        particleData->vel.y *= 0.95f;
        particleData->accel.y += 20.0f;
        if (particleData->accel.y >= 180.0f && particleData->accel.y < 200.0f) {
            particleData->accel.z *= 0.4f;
        }
        if (particleData->accel.y >= 360.0f && particleData->accel.y < 380.0f) {
            particleData->accel.z *= 0.4f;
        }
        if (particleData->accel.y >= 360.0f) {
            particleData->accel.y -= 360.0f;
        }
        particleAlpha = (30.0 * sind(particleData->accel.y)) + particleData->accel.z;
        if (particleAlpha < 0) {
            particleAlpha = 0;
            particleData->accel.x = 0;
        }
        particleData->color.a = particleAlpha;
        if (0.0f == particleData->accel.x) {
            particleData->scale = 0;
        }
    }
    PPCSync();
}

s8 PlayerStickValueGet(s32 playerIndex, s32 verticalAxis)
{
    PuzzlePlayerState *playerState = &puzzlePlayerState[playerIndex];
    s8 stickValue;
    s8 hardwareStickValue;
    s8 simulatedStickValue;
    if (!GWPlayerCfg[playerIndex].iscom) {
        if (verticalAxis) {
            hardwareStickValue = stickValue = HuPadStkY[playerState->controllerIndex];
        }
        else {
            hardwareStickValue = stickValue = HuPadStkX[playerState->controllerIndex];
        }
        (void)hardwareStickValue;
    }
    else {
        if (verticalAxis) {
            simulatedStickValue = stickValue = playerState->stickY;
        }
        else {
            simulatedStickValue = stickValue = playerState->stickX;
        }
    }
    return stickValue;
}

u8 PlayerTriggerValueGet(s32 playerIndex, s32 rightTrigger)
{
    PuzzlePlayerState *playerState = &puzzlePlayerState[playerIndex];
    s8 triggerValue;
    s8 hardwareTriggerValue;
    s8 simulatedTriggerValue;
    if (!GWPlayerCfg[playerIndex].iscom) {
        if (rightTrigger) {
            hardwareTriggerValue = triggerValue = HuPadTrigR[playerState->controllerIndex];
        }
        else {
            hardwareTriggerValue = triggerValue = HuPadTrigL[playerState->controllerIndex];
        }
        (void)hardwareTriggerValue;
    }
    else {
        if (rightTrigger) {
            simulatedTriggerValue = triggerValue = playerState->triggerRight;
        }
        else {
            simulatedTriggerValue = triggerValue = playerState->triggerLeft;
        }
    }
    return triggerValue;
}

static inline s8 PlayerStickValueGetInline(s32 playerIndex, s32 verticalAxis)
{
    PuzzlePlayerState *playerState = &puzzlePlayerState[playerIndex];
    s8 stickValue;
    if (!GWPlayerCfg[playerIndex].iscom) {
        if (verticalAxis) {
            stickValue = HuPadStkY[playerState->controllerIndex];
        }
        else {
            stickValue = HuPadStkX[playerState->controllerIndex];
        }
    }
    else {
        if (verticalAxis) {
            stickValue = playerState->stickY;
        }
        else {
            stickValue = playerState->stickX;
        }
    }
    return stickValue;
}

static inline u8 PlayerTriggerValueGetInline(s32 playerIndex, s32 rightTrigger)
{
    PuzzlePlayerState *playerState = &puzzlePlayerState[playerIndex];
    s8 triggerValue;
    if (!GWPlayerCfg[playerIndex].iscom) {
        if (rightTrigger) {
            triggerValue = HuPadTrigR[playerState->controllerIndex];
        }
        else {
            triggerValue = HuPadTrigL[playerState->controllerIndex];
        }
    }
    else {
        if (rightTrigger) {
            triggerValue = playerState->triggerRight;
        }
        else {
            triggerValue = playerState->triggerLeft;
        }
    }
    return triggerValue;
}

u16 PlayerButtonsDownGet(s32 playerIndex)
{
    PuzzlePlayerState *playerState = &puzzlePlayerState[playerIndex];
    u16 buttonsDown;

    // Hacks to stop spilling in PuzzlePlayerTask
    (void)playerState;
    (void)playerState;
    (void)playerState;
    (void)playerState;

    if (!GWPlayerCfg[playerIndex].iscom) {
        buttonsDown = HuPadBtnDown[playerState->controllerIndex];
    }
    else {
        buttonsDown = playerState->buttonDown;
    }
    return buttonsDown;
}

void PuzzlePiecePlaceTask(PuzzlePlayerState *playerState, s32 pieceIndex)
{
    float overlayOffsetX;
    float overlayOffsetY;
    float pieceStepX;
    float pieceStepY;
    float pieceStepZ;

    Vec *piecePosition;
    Vec *targetPosition;
    Vec *characterPosition;
    s32 animationFrame;
    Vec *overlayPosition;
    s32 overlayAlpha;

    Vec screenPosition3D;
    Vec screenPosition2D;
    Vec finalScreenPosition3D;
    Vec finalScreenPosition2D;
    piecePosition = &playerState->pieceModels[pieceIndex]->mesh.base.pos;
    overlayPosition = &playerState->overlayModels[pieceIndex]->mesh.base.pos;
    characterPosition = &playerState->characterModels[pieceIndex]->mesh.base.pos;
    targetPosition = &playerState->pieceTransforms[pieceIndex].pos;
    pieceStepX = (targetPosition->x - piecePosition->x) / 15.0f;
    pieceStepZ = (targetPosition->z - piecePosition->z) / 30.0f;
    pieceStepY = (targetPosition->y - piecePosition->y) / 15.0f;
    overlayOffsetX = 20;
    overlayOffsetY = -20;
    HuSprAttrSet(playerState->spriteGroupId, 0, 64);
    omVibrate(playerState->playerIndex, 12, 6, 6);
    overlayAlpha = 0;
    for (animationFrame = 0; animationFrame < 30; animationFrame++) {
        piecePosition->z += pieceStepZ;
        characterPosition->z = overlayPosition->z = piecePosition->z - 5.0f;
        if (animationFrame < 15) {
            piecePosition->x += pieceStepX;
            piecePosition->y += pieceStepY;
            characterPosition->x = piecePosition->x;
            characterPosition->y = piecePosition->y;
            screenPosition3D.x = 30.0f + piecePosition->x;
            screenPosition3D.z = 50.0f;
            screenPosition3D.y = -30.0f + piecePosition->y;
            Hu3D3Dto2D(&screenPosition3D, (1 << playerState->cameraIndex), &screenPosition2D);
            HuSprGrpPosSet(playerState->spriteGroupId, screenPosition2D.x, screenPosition2D.y);
        }
        overlayPosition->x = piecePosition->x + overlayOffsetX;
        overlayPosition->y = piecePosition->y + overlayOffsetY;
        overlayOffsetX -= (4.0f / 7.0f);
        overlayOffsetY -= (-4.0f / 7.0f);
        characterPosition->z += 10.0f;
        if (animationFrame < 14) {
            overlayAlpha += 17;
        }
        else if (animationFrame < 28) {
            overlayAlpha -= 17;
        }
        Hu3DModelTPLvlSet(playerState->modelIds[1], 255 - overlayAlpha);
        HuPrcVSleep();
    }
    piecePosition->x = targetPosition->x;
    piecePosition->y = targetPosition->y;
    piecePosition->z = targetPosition->z;
    finalScreenPosition3D.x = 30.0f + piecePosition->x;
    finalScreenPosition3D.z = 50;
    finalScreenPosition3D.y = -30.0f + piecePosition->y;
    Hu3D3Dto2D(&finalScreenPosition3D, (1 << playerState->cameraIndex), &finalScreenPosition2D);
    HuSprGrpPosSet(playerState->spriteGroupId, finalScreenPosition2D.x, finalScreenPosition2D.y);
    characterPosition->x = 5000;
}

void PuzzlePiecePulseTask(PuzzlePlayerState *playerState, s32 pieceIndex)
{
    HSFTRANSFORM *pieceTransform;
    HSFTRANSFORM *savedTransform;
    HSFTRANSFORM *overlayTransform;
    float pulseAngle;
    pieceTransform = &playerState->pieceModels[pieceIndex]->mesh.base;
    overlayTransform = &playerState->overlayModels[pieceIndex]->mesh.base;
    savedTransform = &playerState->pieceTransforms[pieceIndex];
    for (pulseAngle = 0; pulseAngle < 180; pulseAngle += 10) {
        pieceTransform->scale.x = (savedTransform->scale.x * 0.8f) + (0.2f * (savedTransform->scale.x * (1.0 - sind(pulseAngle))));
        pieceTransform->scale.y = (savedTransform->scale.y * 0.8f) + (0.2f * (savedTransform->scale.y * (1.0 - sind(pulseAngle))));
        pieceTransform->scale.z = (savedTransform->scale.z * 0.8f) + (0.2f * (savedTransform->scale.z * (1.0 - sind(pulseAngle))));
        overlayTransform->scale.x = pieceTransform->scale.x;
        overlayTransform->scale.y = pieceTransform->scale.y;
        overlayTransform->scale.z = 0.8f * pieceTransform->scale.z;
        HuPrcVSleep();
    }
    for (pulseAngle = 180; pulseAngle < 360; pulseAngle += 20) {
        pieceTransform->scale.x = (savedTransform->scale.x * 0.9f) + (0.1f * (savedTransform->scale.x * (1.0 - sind(pulseAngle))));
        pieceTransform->scale.y = (savedTransform->scale.y * 0.9f) + (0.1f * (savedTransform->scale.y * (1.0 - sind(pulseAngle))));
        pieceTransform->scale.z = (savedTransform->scale.z * 0.9f) + (0.1f * (savedTransform->scale.z * (1.0 - sind(pulseAngle))));
        overlayTransform->scale.x = pieceTransform->scale.x;
        overlayTransform->scale.y = pieceTransform->scale.y;
        overlayTransform->scale.z = 0.8f * pieceTransform->scale.z;
        HuPrcVSleep();
    }
    pieceTransform->scale.x = savedTransform->scale.x;
    pieceTransform->scale.y = savedTransform->scale.y;
    pieceTransform->scale.z = savedTransform->scale.z;
    overlayTransform->scale.x = pieceTransform->scale.x;
    overlayTransform->scale.y = pieceTransform->scale.y;
    overlayTransform->scale.z = 0.8f * pieceTransform->scale.z;
}

void PuzzlePieceAppearTask(PuzzlePlayerState *playerState, s32 pieceIndex, s32 pieceOrderIndex)
{
    HSFTRANSFORM *pieceTransform;
    HSFTRANSFORM *overlayTransform;
    s32 frameIndex;

    float scaleProgress;
    float pieceScaleX;
    float pieceScaleY;
    float pieceScaleZ;

    pieceTransform = &playerState->pieceModels[pieceIndex]->mesh.base;
    overlayTransform = &playerState->overlayModels[pieceIndex]->mesh.base;
    pieceScaleX = pieceTransform->scale.x;
    pieceScaleY = pieceTransform->scale.y;
    pieceScaleZ = pieceTransform->scale.z;
    pieceTransform->scale.x = pieceTransform->scale.y = pieceTransform->scale.z = 0;
    overlayTransform->scale.x = overlayTransform->scale.y = overlayTransform->scale.z = 0;
    for (frameIndex = 0, scaleProgress = 0; frameIndex < 30; frameIndex++, scaleProgress += (1.0f / 30.0f)) {
        pieceTransform->scale.x += pieceScaleX / 30.0f;
        pieceTransform->scale.y += pieceScaleY / 30.0f;
        pieceTransform->scale.z += pieceScaleZ / 30.0f;
        overlayTransform->scale.x = pieceTransform->scale.x;
        overlayTransform->scale.y = pieceTransform->scale.y;
        overlayTransform->scale.z = 0.8f * pieceTransform->scale.z;
        if (pieceOrderIndex == 0) {
            HuSprGrpScaleSet(playerState->spriteGroupId, scaleProgress, scaleProgress);
        }
        HuPrcVSleep();
    }
    pieceTransform->scale.x = pieceScaleX;
    pieceTransform->scale.y = pieceScaleY;
    pieceTransform->scale.z = pieceScaleZ;
    overlayTransform->scale.x = pieceTransform->scale.x;
    overlayTransform->scale.y = pieceTransform->scale.y;
    overlayTransform->scale.z = 0.8f * pieceTransform->scale.z;
    if (pieceOrderIndex == 0) {
        HuSprAttrReset(playerState->spriteGroupId, 0, HUSPR_ATTR_NOANIM);
        HuSprGrpScaleSet(playerState->spriteGroupId, 1, 1);
    }
    else {
        HuSprAttrReset(playerState->spriteGroupId, 0, HUSPR_ATTR_REVERSE);
    }
}

typedef struct PieceRotationTask {
    PuzzlePlayerState *playerData;
    s32 pieceIndex;
    s32 rotateClockwise;
    s32 *rotationComplete;
} PieceRotationTask;

void PuzzlePieceRotateTask(void)
{
    HSFTRANSFORM *pieceTransform;
    PieceRotationTask *rotationTask;
    s32 animationFrame;
    HSFTRANSFORM *characterTransform;
    s32 frameCount;

    float rotationProgress;
    float targetRotation;
    float initialRotation;
    rotationTask = HuPrcCurrentGet()->user_data;
    pieceTransform = &rotationTask->playerData->pieceModels[rotationTask->pieceIndex]->mesh.base;
    characterTransform = &rotationTask->playerData->characterModels[rotationTask->pieceIndex]->mesh.base;
    HuAudFXPlay(1401);
    if (rotationTask->rotateClockwise) {
        targetRotation = pieceTransform->rot.z + 90.0f;
    }
    else {
        targetRotation = pieceTransform->rot.z - 90.0f;
    }
    if (targetRotation >= 360.0f) {
        targetRotation -= 360.0f;
    }
    if (targetRotation < 0.0f) {
        targetRotation += 360.0f;
    }
    initialRotation = pieceTransform->rot.z;
    rotationProgress = 0;
    for (animationFrame = 0, frameCount = 15; animationFrame < frameCount; animationFrame++) {
        if (rotationTask->rotateClockwise) {
            pieceTransform->rot.z = initialRotation + (100.0 * sind(rotationProgress));
        }
        else {
            pieceTransform->rot.z = initialRotation - (100.0 * sind(rotationProgress));
        }
        characterTransform->rot.z = pieceTransform->rot.z;
        rotationProgress += 6.0f;
        HuPrcVSleep();
    }
    rotationProgress = 90.0f;
    for (animationFrame = 0, frameCount = 3; animationFrame < frameCount; animationFrame++) {
        rotationProgress -= (25.0f / 3.0f);
        if (rotationTask->rotateClockwise) {
            pieceTransform->rot.z = initialRotation + (100.0 * sind(rotationProgress));
        }
        else {
            pieceTransform->rot.z = initialRotation - (100.0 * sind(rotationProgress));
        }
        characterTransform->rot.z = pieceTransform->rot.z;
        HuPrcVSleep();
    }
    pieceTransform->rot.z = characterTransform->rot.z = targetRotation;
    *rotationTask->rotationComplete = 0;
    HuPrcKill(HuPrcCurrentGet());
    while (1) {
        HuPrcVSleep();
    }
}

void PuzzleConfettiParticleTask(void)
{
    Vec *particlePosition = HuPrcCurrentGet()->user_data;
    float verticalSpeed;
    float horizontalSpeed;
    HuPrcSleep(frandmod(REFRESH_RATE / 2.5));
    horizontalSpeed = particlePosition->x / 100.0f;
    verticalSpeed = -7.0f;
    while (1) {
        particlePosition->y -= verticalSpeed;
        if (particlePosition->y < -500.0f) {
            break;
        }
        verticalSpeed += 0.5f;
        if (verticalSpeed > 20.0f) {
            verticalSpeed = 20.0f;
        }
        particlePosition->x += horizontalSpeed;
        particlePosition->z += 10.0f;
        HuPrcVSleep();
    }
    HuPrcKill(HuPrcCurrentGet());
    while (1) {
        HuPrcVSleep();
    }
}

void PuzzlePieceCursorClamp(PuzzlePlayerState *playerState, s32 pieceIndex)
{
    Vec *piecePosition = &playerState->pieceModels[pieceIndex]->mesh.base.pos;
    if (piecePosition->x > 230.0f) {
        piecePosition->x = 230.0f;
    }
    if (piecePosition->x < -230.0f) {
        piecePosition->x = -230.0f;
    }
    if (piecePosition->y > 170.0f) {
        piecePosition->y = 170.0f;
    }
    if (piecePosition->y < -170.0f) {
        piecePosition->y = -170.0f;
    }
}

void ComputerPlayerDelay(s32 playerIndex, s32 delayFrames)
{
    switch (GWPlayerCfg[playerIndex].diff) {
        case 0:
            delayFrames *= 8;
            break;

        case 1:
            delayFrames *= 4;
            break;

        case 2:
            delayFrames *= 2;
            break;
    }
    if (delayFrames <= 1) {
        HuPrcVSleep();
    }
    else {
        HuPrcSleep(frandmod(delayFrames / 2) + delayFrames);
    }
}

void ComputerPlayerInputUpdate(PuzzlePlayerState *playerState, s32 pieceIndex, s32 pieceOrderIndex)
{
    HSFTRANSFORM *pieceTransform;
    s32 selectionChance;
    HSFTRANSFORM *targetTransform;
    s32 randomPieceOffset;
    float cursorAngle;
    Vec targetPosition;
    Vec piecePosition;
    Vec cursorDelta;
    pieceIndex = shuffledPieceOrder[pieceOrderIndex];
    pieceTransform = &playerState->pieceModels[pieceIndex]->mesh.base;
    playerState->stickX = playerState->stickY = 0;
    playerState->triggerLeft = playerState->triggerRight = 0;
    playerState->buttonDown = 0;
    switch (GWPlayerCfg[playerState->playerIndex].diff) {
        case 0:
            selectionChance = (pieceOrderIndex * 2) + 60;
            break;

        case 1:
            selectionChance = pieceOrderIndex + 80;
            break;

        case 2:
            selectionChance = pieceOrderIndex + 90;
            break;

        default:
            selectionChance = pieceOrderIndex + 95;
            break;
    }
    if (selectionChance > 100) {
        selectionChance = 100;
    }
    if (playerState->selectedPieceIndex == -1) {
        if (frandmod(100) < selectionChance || puzzlePieceCount == pieceOrderIndex + 1) {
            playerState->selectedPieceIndex = pieceIndex;
        }
        else {
            randomPieceOffset = frandmod(puzzlePieceCount - pieceOrderIndex);
            playerState->selectedPieceIndex = shuffledPieceOrder[pieceOrderIndex + randomPieceOffset];
        }
        playerState->computerTurnTimer = -1;
        if (GWPlayerCfg[playerState->playerIndex].diff == 2 || GWPlayerCfg[playerState->playerIndex].diff == 3) {
            if (frandmod(100) < selectionChance || puzzlePieceCount == pieceOrderIndex + 1) {
                playerState->computerTurnTimer = 10;
            }
        }
        ComputerPlayerDelay(playerState->playerIndex, 10);
    }
    targetTransform = &playerState->pieceTransforms[playerState->selectedPieceIndex];
    HuSetVecF(&targetPosition, targetTransform->pos.x, targetTransform->pos.y, 0.0);
    HuSetVecF(&piecePosition, pieceTransform->pos.x, pieceTransform->pos.y, 0.0);
    HuSubVecF(&cursorDelta, &targetPosition, &piecePosition);
    if (VECMagXY(&cursorDelta) >= 40.0f) {
        cursorAngle = atan2d(cursorDelta.x, cursorDelta.y);
        playerState->stickX = 50 * sind(cursorAngle);
        playerState->stickY = 50 * cosd(cursorAngle);
        if (playerState->computerTurnTimer >= 0 && playerState->computerTurnTimer-- == 0 && pieceTransform->rot.z != 0.0f) {
            playerState->computerTurnTimer = 10;
            if (0.0f == (pieceTransform->rot.z - 90.0f)) {
                playerState->triggerRight = 50;
            }
            else {
                playerState->triggerLeft = 50;
            }
        }
    }
    else {
        ComputerPlayerDelay(playerState->playerIndex, 1);
        if (frandmod(100) < selectionChance || puzzlePieceCount == pieceOrderIndex + 1) {
            if (0.0f != pieceTransform->rot.z) {
                if (0.0f == (pieceTransform->rot.z - 90.0f)) {
                    playerState->triggerRight = 50;
                }
                else {
                    playerState->triggerLeft = 50;
                }
            }
            else {
                playerState->buttonDown = PAD_BUTTON_A;
                playerState->selectedPieceIndex = -1;
            }
        }
        else {
            switch (frandmod(3)) {
                case 0:
                    playerState->triggerRight = 50;
                    break;

                case 1:
                    playerState->triggerRight = 50;
                    break;

                default:
                    playerState->buttonDown = PAD_BUTTON_A;
                    playerState->selectedPieceIndex = -1;
                    break;
            }
            if (frandmod(100) < selectionChance) {
                playerState->selectedPieceIndex = -1;
            }
        }
    }
}

void PuzzlePlayerTask(void)
{
    PuzzlePlayerState *playerState;
    HSFTRANSFORM *pieceTransform;
    s32 pieceOrderIndex;
    HSFTRANSFORM *overlayTransform;
    s32 pieceIndex;
    HSFTRANSFORM *targetTransform;
    HUPROCESS *childProcess;
    s32 rotationDirection;
    Vec targetPosition;
    Vec piecePosition;
    Vec cursorDelta;
    PieceRotationTask rotationTask;
    Vec cursorPosition3D;
    Vec cursorPosition2D;
    Vec updatedCursorPosition3D;
    Vec updatedCursorPosition2D;
    s32 rotationInProgress;
    s32 unusedPlayerWorkValueA;
    s32 unusedPlayerWorkValueB;
    float pieceDistance;
    float pieceAngle;

    playerState = HuPrcCurrentGet()->user_data;
    playerState->playerState = 0;
    for (pieceOrderIndex = 0; pieceOrderIndex < puzzlePieceCount; pieceOrderIndex++) {
        pieceTransform = &playerState->overlayModels[pieceOrderIndex]->mesh.base;
        pieceTransform->pos.x = 5000;
        pieceTransform = &playerState->characterModels[pieceOrderIndex]->mesh.base;
        pieceTransform->pos.x = 5000;
    }
    while (puzzleGamePhase == 0) {
        HuPrcVSleep();
    }
    HuPrcSleep(REFRESH_RATE * 3);
    HuAudFXPlay(1391);
    for (pieceOrderIndex = 0; pieceOrderIndex < puzzlePieceCount; pieceOrderIndex++) {
        pieceTransform = &playerState->pieceModels[pieceOrderIndex]->mesh.base;
        pieceAngle = atan2d(pieceTransform->pos.x, pieceTransform->pos.y);
        pieceDistance = VECMagXY(&pieceTransform->pos);
        pieceDistance *= 1.05f;
        pieceTransform->pos.x = pieceDistance * sind(pieceAngle);
        pieceTransform->pos.y = pieceDistance * cosd(pieceAngle);
    }
    omVibrate(playerState->playerIndex, 12, 6, 6);
    HuPrcSleep(30);
    HuAudFXPlay(1387);
    for (pieceOrderIndex = 0; pieceOrderIndex < puzzlePieceCount; pieceOrderIndex++) {
        childProcess = HuPrcChildCreate(PuzzleConfettiParticleTask, 8192, 4096, 0, HuPrcCurrentGet());
        pieceTransform = &playerState->pieceModels[pieceOrderIndex]->mesh.base;
        childProcess->user_data = pieceTransform;
    }
    HuPrcSleep(120);
    puzzleGamePhase = 2;
    for (pieceOrderIndex = 0; pieceOrderIndex < puzzlePieceCount; pieceOrderIndex++) {
        rotationInProgress = 0;
        playerState->movementPhase = 0;
        playerState->selectedPieceIndex = -1;
        pieceIndex = shuffledPieceOrder[pieceOrderIndex];
        pieceTransform = &playerState->pieceModels[pieceIndex]->mesh.base;
        overlayTransform = &playerState->overlayModels[pieceIndex]->mesh.base;
        if (pieceOrderIndex != 0) {
            targetTransform = &playerState->pieceTransforms[shuffledPieceOrder[pieceOrderIndex - 1]];
            pieceTransform->pos.x = targetTransform->pos.x;
            pieceTransform->pos.y = targetTransform->pos.y;
            overlayTransform->pos.x = 20 + pieceTransform->pos.x;
            overlayTransform->pos.y = -20 + pieceTransform->pos.y;
        }
        else {
            pieceTransform->pos.x = pieceTransform->pos.y = 0;
            overlayTransform->pos.x = 20 + pieceTransform->pos.x;
            overlayTransform->pos.y = -20 + pieceTransform->pos.y;
            cursorPosition3D.x = pieceTransform->pos.x + 30;
            cursorPosition3D.z = 50;
            cursorPosition3D.y = -30 + pieceTransform->pos.y;
            Hu3D3Dto2D(&cursorPosition3D, (1 << playerState->cameraIndex), &cursorPosition2D);
            HuSprGrpPosSet(playerState->spriteGroupId, cursorPosition2D.x, cursorPosition2D.y);
            HuSprAttrReset(playerState->spriteGroupId, 0, HUSPR_ATTR_DISPOFF);
        }
        pieceTransform->pos.z = 50;
        overlayTransform->pos.z = pieceTransform->pos.z - 5;
        pieceTransform->rot.z = overlayTransform->rot.z = 90.0f * pieceRotationQuarters[pieceOrderIndex];
        PuzzlePieceAppearTask(playerState, pieceIndex, pieceOrderIndex);
        if (pieceOrderIndex == 0) {
            while (puzzleGamePhase != 3) {
                HuPrcVSleep();
            }
        }
        while (1) {
            if (GWPlayerCfg[playerState->playerIndex].iscom == 1) {
                ComputerPlayerInputUpdate(playerState, pieceIndex, pieceOrderIndex);
            }
            if (!rotationInProgress) {
                rotationDirection = -1;
                if (PlayerTriggerValueGetInline(playerState->playerIndex, 0) > 20) {
                    rotationDirection = 1;
                }
                if (PlayerTriggerValueGetInline(playerState->playerIndex, 1) > 20) {
                    rotationDirection = 0;
                }
                if (rotationDirection != -1) {
                    rotationInProgress = 1;
                    childProcess = HuPrcChildCreate(PuzzlePieceRotateTask, 8192, 8192, 0, HuPrcCurrentGet());
                    childProcess->user_data = &rotationTask;
                    rotationTask.playerData = playerState;
                    rotationTask.pieceIndex = pieceIndex;
                    rotationTask.rotateClockwise = rotationDirection;
                    rotationTask.rotationComplete = &rotationInProgress;
                }
            }
            if ((PlayerButtonsDownGet(playerState->playerIndex) & PAD_BUTTON_A) && !rotationInProgress) {
                if (0.0f == pieceTransform->rot.z) {
                    targetTransform = &playerState->pieceTransforms[pieceIndex];
                    HuSetVecF(&targetPosition, targetTransform->pos.x, targetTransform->pos.y, 0.0f);
                    HuSetVecF(&piecePosition, pieceTransform->pos.x, pieceTransform->pos.y, 0.0f);
                    HuSubVecF(&cursorDelta, &targetPosition, &piecePosition);
                    if (VECMagXY(&cursorDelta) < 40.0f) {
                        HuAudFXPlay(1402);
                        playerState->movementPhase = 1;
                        playerState->cursorState[7] = pieceTransform->pos.x;
                        playerState->cursorState[8] = pieceTransform->pos.y;
                        PuzzlePiecePlaceTask(playerState, pieceIndex);
                        goto end;
                    }
                }
                PuzzlePiecePulseTask(playerState, pieceIndex);
            }
            playerState->cursorState[6] = -1.0f;
            if (ABS_INV(PlayerStickValueGetInline(playerState->playerIndex, 0)) > 20 || ABS_INV(PlayerStickValueGetInline(playerState->playerIndex, 1)) > 20) {
                playerState->cursorState[6] = atan2d(PlayerStickValueGetInline(playerState->playerIndex, 0), PlayerStickValueGetInline(playerState->playerIndex, 1));
            }
            if (-1.0f != playerState->cursorState[6]) {
                pieceTransform->pos.x += 5.0f * sind(playerState->cursorState[6]);
                pieceTransform->pos.y += 5.0f * cosd(playerState->cursorState[6]);
                PuzzlePieceCursorClamp(playerState, pieceIndex);
                overlayTransform->pos.x = 20.0f + pieceTransform->pos.x;
                overlayTransform->pos.y = -20.0f + pieceTransform->pos.y;
                updatedCursorPosition3D.x = 30.0f + pieceTransform->pos.x;
                updatedCursorPosition3D.z = 50;
                updatedCursorPosition3D.y = -30.0f + pieceTransform->pos.y;
                Hu3D3Dto2D(&updatedCursorPosition3D, (1 << playerState->cameraIndex), &updatedCursorPosition2D);
                HuSprGrpPosSet(playerState->spriteGroupId, updatedCursorPosition2D.x, updatedCursorPosition2D.y);
            }
            HuPrcVSleep();
        }

    end:
        (void)playerState;
    }
    playerState->playerState = 4;
    // Hacks to Stop Spilling
    (void)pieceIndex;
    (void)pieceIndex;
    (void)childProcess;
    (void)childProcess;
    (void)childProcess;
    (void)rotationDirection;
    (void)rotationDirection;
    (void)rotationDirection;
    (void)rotationDirection;
    while (1) {
        HuPrcVSleep();
    }
}

void PuzzlePlayerSpriteFadeTask(s32 playerSlot)
{
    float spriteScale;
    s32 fadeFrame;
    s32 playerIndex;
    PuzzlePlayerState *activePlayerState = &puzzlePlayerState[playerSlot];
    PuzzlePlayerState *playerState;

    for (fadeFrame = 0, spriteScale = 1; fadeFrame < 15; fadeFrame++) {
        spriteScale -= (1.0f / 15.0f);
        for (playerIndex = 0; playerIndex < 1; playerIndex++) {
            playerState = &puzzlePlayerState[playerIndex];
            HuSprGrpScaleSet(playerState->spriteGroupId, spriteScale, spriteScale);
        }
        HuPrcVSleep();
    }
    for (fadeFrame = 0; fadeFrame < 1; fadeFrame++) {
        HuSprAttrSet(puzzlePlayerState[fadeFrame].spriteGroupId, 0, HUSPR_ATTR_DISPOFF);
    }
}

void PuzzlePlayerSetupTask(void)
{
    while (1) {
        if (omSysExitReq == 1) {
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
            HuAudFadeOut(1000);
            HuPrcSleep(60);
            MGSeqKillAll();
            omOvlReturnEx(1, 1);
            while (1) {
                HuPrcVSleep();
            }
        }
        HuPrcVSleep();
    }
}

void PuzzleGameTask(void)
{
    u32 elapsedFrames;
    s32 playerIndex;
    s32 winnerIndex;
    s16 sequenceId;
    s32 completedPlayerCount;
    s32 musicSequenceId;

    puzzleGamePhase = 0;
    elapsedFrames = 0;
    puzzleGamePhase = 1;
    while (puzzleGamePhase == 1) {
        HuPrcVSleep();
    }
    musicSequenceId = HuAudSeqPlay(69);
    sequenceId = MGSeqStartCreate();
    while (MGSeqStatGet(sequenceId)) {
        HuPrcVSleep();
    }
    puzzleGamePhase = 3;
    while (1) {
        for (playerIndex = 0, completedPlayerCount = 0; playerIndex < 1; playerIndex++) {
            if (puzzlePlayerState[playerIndex].playerState == 4) {
                winnerIndex = playerIndex;
                completedPlayerCount++;
            }
        }
        if (completedPlayerCount != 0) {
            break;
        }
        elapsedFrames++;
        TimerDigitsSet(timerSpriteGroupId, 2, elapsedFrames, 1);
        if (elapsedFrames >= REFRESH_RATE * 600 - 1) {
            winnerIndex = -1;
            break;
        }
        HuPrcVSleep();
    }
    if (winnerIndex == -1) {
        TimerDigitsSet(timerSpriteGroupId, 2, 0, 2);
    }
    HuAudSeqFadeOut(musicSequenceId, 100);
    for (playerIndex = 0; playerIndex < 1; playerIndex++) {
        HuPrcKill(puzzlePlayerProcesses[playerIndex]);
    }
    sequenceId = MGSeqFinishCreate();
    while (MGSeqStatGet(sequenceId)) {
        HuPrcVSleep();
    }
    if (winnerIndex == -1 || elapsedFrames >= GWMGRecordGet(puzzleRecordIds[puzzlePresetIndex])) {
        if (winnerIndex != -1) {
            HuAudSStreamPlay(1);
        }
        else {
            HuAudSStreamPlay(4);
        }
    }
    if (winnerIndex != -1) {
        puzzleGamePhase = 4;
        winningPlayerIndex = winnerIndex;
        PuzzlePlayerSpriteFadeTask(winnerIndex);
    }
    mgRecordExtra = elapsedFrames;
    if (winnerIndex != -1) {
        if (puzzleCompletionTimeLimits[puzzlePresetIndex] >= elapsedFrames) {
            GWGameStat.present[puzzleRewardIds[puzzlePresetIndex]] = 1;
            OSReport("time %d no %d\n", puzzleCompletionTimeLimits[puzzlePresetIndex], puzzleRewardIds[puzzlePresetIndex]);
        }
        if (elapsedFrames < GWMGRecordGet(puzzleRecordIds[puzzlePresetIndex])) {
            GWMGRecordSet(puzzleRecordIds[puzzlePresetIndex], elapsedFrames);
            sequenceId = MGSeqRecordCreate(elapsedFrames);
            HuPrcChildCreate(TimerDigitPulseTask, 4096, 4096, 0, HuPrcCurrentGet());
            TimerDigitsSet(timerSpriteGroupId, 2, elapsedFrames, 0);
            while (MGSeqStatGet(sequenceId)) {
                HuPrcVSleep();
            }
            HuAudSStreamPlay(1);
        }
        HuPrcSleep(REFRESH_RATE * 3);
    }
    else {
        mgRecordExtra = REFRESH_RATE * 600;
        HuPrcSleep(REFRESH_RATE * 3.5);
    }
    HuPrcSleep(REFRESH_RATE / 2);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    HuPrcSleep(60);
    HuAudFadeOut(1);
    MGSeqKillAll();
    omOvlReturnEx(1, 1);
    while (1) {
        HuPrcVSleep();
    }
}
