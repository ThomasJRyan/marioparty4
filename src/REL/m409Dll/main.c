#include "REL/m409Dll.h"

#include "ext_math.h"
#include "game/audio.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "math.h"
#include <string.h>

// bss
HUPROCESS *sceneProcess;
s16 sceneLightId;
OMOBJ *sceneEffectObject;
OMOBJ *sequenceControllerObject;
Vec unusedSceneVector;
SceneModelWork sceneModelSequences[4];
s32 sceneEffectCount;
TextureScrollState sceneTextureScrollState;
s8 isCameraRotationReversed;
u8 gameSequenceCount;
F32 particleBurstTimer;
F32 finalTransitionProgress;
s16 roundSequenceTimer;
s8 particleSpawnCounter;
F32 particleBurstCounter;
F32 introTransitionProgress;
s8 introCameraEnabled;
s16 sequenceState;
s8 introSkipGate;
s16 outroCameraTimer;
s16 introCameraTimer;
s8 overlayExitRequested;

// data
Vec sceneLightPosition = { 800.0f, 1300.0f, 1000.0f };
Vec sceneLightAimPoint = { 0.0f, 0.0f, 0.0f };
LightColorSettings sceneLightSettings = { { 0xFF, 0xFF, 0xFF, 0xFF }, { 10.0f, 45.0f, 0.0f } };
Vec sceneShadowPosition = { 1300.0f, 2500.0f, 1300.0f };
Vec sceneShadowUpVector = { 0.0f, 1.0f, 0.0f };
Vec sceneShadowTarget = { 0.0f, 0.0f, -1200.0f };
CameraPreset cameraPresets[4]
    = { { 469.8f, { 225.0f, 184.0f, -1358.0f }, { 10.62f, -109.0f, 0.0f } }, { 1800.0f, { 0.0f, 300.0f, -1316.0f }, { -20.38f, 0.0f, 0.0f } },
          { 2209.82f, { 0.0f, 478.0f, -1316.0f }, { 4.62f, 0.0f, 0.0f } }, { 2059.82f, { 0.0f, 528.0f, -1316.0f }, { 10.62f, 0.0f, 0.0f } } };
u32 introSoundEmitterId = 0xFFFFFFFF;
s32 musicSequenceId = -1;

void ObjectSetup(void)
{
    Vec lightAimDirection;
    Vec cameraStartPosition;
    Vec cameraEndPosition;
    Vec cameraForwardDirection;
    HU3DLIGHT *sceneLight;

    Hu3DLightAllKill();
    sceneLightId = Hu3DGLightCreateV(&sceneLightPosition, &sceneLightAimPoint, &sceneLightSettings.lightColor);
    Hu3DGLightInfinitytSet(sceneLightId);
    sceneLight = &Hu3DGlobalLight[sceneLightId];
    sceneLight->type |= 0x8000;
    lightAimDirection.x = lightAimDirection.y = lightAimDirection.z = 0.0f;
    Hu3DGLightPosAimSetV(sceneLightId, &sceneLightPosition, &lightAimDirection);
    Hu3DShadowCreate(45.0f, 20.0f, 10000.0f);
    Hu3DShadowTPLvlSet(0.8f);
    Hu3DShadowPosSet(&sceneShadowPosition, &sceneShadowUpVector, &sceneShadowTarget);
    sceneProcess = omInitObjMan(0x32, 0x2000);
    omGameSysInit(sceneProcess);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 41.5f, 5.0f, 5000.0f, 1.2f);
    omAddObjEx(sceneProcess, 0x7FDA, 0U, 0U, -1, omOutView);
    cameraStartPosition.x = 0.0f;
    cameraStartPosition.y = 374.0f;
    cameraStartPosition.z = 1086.0f;
    cameraEndPosition.x = 0.0f;
    cameraEndPosition.y = 568.0f;
    cameraEndPosition.z = -1316.0f;
    cameraForwardDirection.z = cameraEndPosition.x - cameraStartPosition.x;
    cameraForwardDirection.y = cameraEndPosition.y - cameraStartPosition.y;
    cameraForwardDirection.x = cameraEndPosition.z - cameraStartPosition.z;
    CRot.x = cameraPresets[0].cameraRotation.x;
    CRot.y = cameraPresets[0].cameraRotation.y;
    CRot.z = cameraPresets[0].cameraRotation.z;
    Center.x = cameraPresets[0].cameraCenter.x;
    Center.y = cameraPresets[0].cameraCenter.y;
    Center.z = cameraPresets[0].cameraCenter.z;
    CZoom = cameraPresets[0].cameraZoom;
    omAddObjEx(sceneProcess, 0x3E8, 0, 0, -1, CheckOverlayExitRequest);
    sceneEffectObject = omAddObjEx(sceneProcess, 0xA, 3U, 0U, -1, CreateSceneModels);
    sequenceControllerObject = omAddObjEx(sceneProcess, 0x32, 7U, 0U, -1, CreateGameSequenceController);
    Hu3DBGColorSet(0, 0, 0);
    HuAudSndGrpSet(0x22);
    InitializePlayerObjects(sceneProcess);
}

void CheckOverlayExitRequest(OMOBJ *object)
{
    if ((omSysExitReq != 0) || (overlayExitRequested != 0)) {
        HuAudFXListnerKill();
#if VERSION_NTSC
        KillSceneModel(sceneEffectObject->mdlId[0]);
        KillSceneModel(sceneEffectObject->mdlId[1]);
#endif
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 0x3C);
        MGSeqKillAll();
        object->objFunc = FinishOverlayExit;
    }
}

void FinishOverlayExit(OMOBJ *object)
{
    if ((WipeStatGet() == 0) && (MGSeqDoneCheck() != 0)) {
        CleanupPlayerObjects();
#if VERSION_PAL
        KillSceneModel(sceneEffectObject->mdlId[0]);
        KillSceneModel(sceneEffectObject->mdlId[1]);
#endif
        omOvlReturnEx(1, 1);
    }
}

s8 UpdateIntroCameraTransition(void)
{
    Vec cameraForwardDirection;
    F32 modelRotationX;
    F32 transitionProgress;
    F32 easedTransitionProgress;
    s8 transitionComplete;
    CameraPreset *cameraPreset;

    cameraPreset = (CameraPreset *)cameraPresets;
    transitionComplete = 0;
    if (++introCameraTimer > 0x12C) {
        introCameraTimer = 0x12C;
        transitionComplete = 1;
    }
    transitionProgress = introCameraTimer / 300.0f;
    easedTransitionProgress = sind(90.0f * transitionProgress) * sind(90.0f * transitionProgress);
    CZoom = InterpolateCameraValue(cameraPreset[0].cameraZoom, cameraPreset[1].cameraZoom, cameraPreset[2].cameraZoom, easedTransitionProgress);
    Center.x = InterpolateCameraValue(cameraPreset[0].cameraCenter.x, cameraPreset[1].cameraCenter.x, cameraPreset[2].cameraCenter.x, easedTransitionProgress);
    Center.y = InterpolateCameraValue(cameraPreset[0].cameraCenter.y, cameraPreset[1].cameraCenter.y, cameraPreset[2].cameraCenter.y, easedTransitionProgress);
    Center.z = InterpolateCameraValue(cameraPreset[0].cameraCenter.z, cameraPreset[1].cameraCenter.z, cameraPreset[2].cameraCenter.z, easedTransitionProgress);
    CRot.x = InterpolateCameraValue(cameraPreset[0].cameraRotation.x, cameraPreset[1].cameraRotation.x, cameraPreset[2].cameraRotation.x, easedTransitionProgress);
    CRot.y = InterpolateCameraValue(cameraPreset[0].cameraRotation.y, cameraPreset[1].cameraRotation.y, cameraPreset[2].cameraRotation.y, easedTransitionProgress);
    CRot.z = InterpolateCameraValue(cameraPreset[0].cameraRotation.z, cameraPreset[1].cameraRotation.z, cameraPreset[2].cameraRotation.z, easedTransitionProgress);
    if (transitionProgress > 0.65f) {
        Hu3DModelAttrSet(sequenceControllerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    }
    modelRotationX = cameraPreset[0].cameraRotation.x + (-cameraPreset[0].cameraRotation.x * easedTransitionProgress);
    Hu3DModelRotSet(sequenceControllerObject->mdlId[2], 0.2f * modelRotationX, 0.85f * CRot.y, 0.0f);
    Hu3DModelRotSet(sequenceControllerObject->mdlId[3], 0.2f * modelRotationX, 0.85f * CRot.y, 0.0f);
    Hu3DModelRotSet(sequenceControllerObject->mdlId[6], 0.2f * modelRotationX, 0.85f * CRot.y, 0.0f);
    transitionProgress *= 1.2f;
    if (transitionProgress > 1.0f) {
        transitionProgress = 1.0f;
    }
    easedTransitionProgress = cosd(-45.0f + (135.0f * transitionProgress));
    Hu3DTexScrollPosSet(sceneTextureScrollState.backgroundScrollLayer, sceneTextureScrollState.backgroundScrollOffset, 0.0f, 0.0f);
    sceneTextureScrollState.backgroundScrollOffset -= 0.01f * easedTransitionProgress;
    Hu3DTexScrollPosMoveSet(sceneTextureScrollState.cloudScrollLayer1, -0.0001f - (0.005f * easedTransitionProgress), 0.0f, 0.0f);
    Hu3DTexScrollPosMoveSet(sceneTextureScrollState.cloudScrollLayer2, -0.0002f - (0.005f * easedTransitionProgress), 0.0f, 0.0f);
    Hu3DTexScrollPosMoveSet(sceneTextureScrollState.cloudScrollLayer3, -0.0003f - (0.005f * easedTransitionProgress), 0.0f, 0.0f);
    cameraForwardDirection.x = -(sind(CRot.y) * cosd(CRot.x));
    cameraForwardDirection.y = sind(CRot.x);
    cameraForwardDirection.z = -(cosd(CRot.y) * cosd(CRot.x));
    HuAudFXListnerUpdate(&Hu3DCamera->pos, &cameraForwardDirection);
    return transitionComplete;
}

s8 UpdateOutroCameraTransition(void)
{
    F32 transitionProgress;
    F32 easedTransitionProgress;
    s8 transitionComplete;
    CameraPreset *cameraPreset;

    cameraPreset = (CameraPreset *)&cameraPresets[2];
    transitionComplete = 0;
    if (++outroCameraTimer > 0x3C) {
        outroCameraTimer = 0x3C;
        transitionComplete = 1;
    }
    transitionProgress = outroCameraTimer / 60.0f;
    easedTransitionProgress = sind(90.0f * transitionProgress) * sind(90.0f * transitionProgress);
    CZoom = cameraPreset[0].cameraZoom + (easedTransitionProgress * (cameraPreset[1].cameraZoom - cameraPreset[0].cameraZoom));
    Center.x = cameraPreset[0].cameraCenter.x + (easedTransitionProgress * (cameraPreset[1].cameraCenter.x - cameraPreset[0].cameraCenter.x));
    Center.y = cameraPreset[0].cameraCenter.y + (easedTransitionProgress * (cameraPreset[1].cameraCenter.y - cameraPreset[0].cameraCenter.y));
    Center.z = cameraPreset[0].cameraCenter.z + (easedTransitionProgress * (cameraPreset[1].cameraCenter.z - cameraPreset[0].cameraCenter.z));
    CRot.x = cameraPreset[0].cameraRotation.x + (easedTransitionProgress * (cameraPreset[1].cameraRotation.x - cameraPreset[0].cameraRotation.x));
    CRot.y = cameraPreset[0].cameraRotation.y + (easedTransitionProgress * (cameraPreset[1].cameraRotation.y - cameraPreset[0].cameraRotation.y));
    CRot.z = cameraPreset[0].cameraRotation.z + (easedTransitionProgress * (cameraPreset[1].cameraRotation.z - cameraPreset[0].cameraRotation.z));
    return transitionComplete;
}

void CreateSceneModels(OMOBJ *object)
{
    s16 *sceneModelWork;
    ANIMDATA *spriteAnimation;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, 6, HU_MEMNUM_OVL);
    sceneModelWork = (s16 *)object->data;
    sceneModelWork[0] = 1;
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M409, 0x0D), HU_MEMNUM_OVL));
    object->mdlId[0] = CreateSceneModel(spriteAnimation, 0x800, 10.0f, 0x40, 0x40);
    SetSceneModelDrawCallback(object->mdlId[0], DrawSceneModelHook);
    Hu3DModelLayerSet(object->mdlId[0], 5);
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M409, 0x0E), HU_MEMNUM_OVL));
    object->mdlId[1] = CreateSceneModel(spriteAnimation, 0x400, 2.0f, 0, 0);
    SetSceneModelDrawCallback(object->mdlId[1], DrawSceneSpriteHook);
    SetSceneModelFlags(object->mdlId[1], 1);
    Hu3DModelLayerSet(object->mdlId[1], 5);
    HuAudFXListnerSetEX(&Hu3DCamera->pos, &Hu3DCamera->up, 10000.0f, 566.6667f, 0.0f, 300.0f, 300.0f);
    object->objFunc = UpdateSceneEffects;
}

void UpdateSceneEffects(OMOBJ *object)
{
    s16 *unusedSequenceStatePointer;
    s16 unusedPreviousSequenceState;
    s16 unusedNextSequenceState;
    s32 controllerIndex;

    unusedSequenceStatePointer = (s16 *)object->data;
    switch (GetSequenceState(7)) {
        case 1:
            if (introSkipGate == 0) {
                introSkipGate = 1;
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 0x3C);
            }
            if (WipeStatGet() == 0) {
                SetSequenceStatePair(7, 2);

                for (controllerIndex = 0; controllerIndex < 4; controllerIndex++) {
                    omVibrate(controllerIndex, 0xC8, 6, 6);
                }
            }
            break;
        case 2:
            if (UpdateIntroSequence(object) != 0) {
                SetSequenceStatePair(7, 3);
            }
            break;
        case 3:
            switch (GetSequenceState(0x60)) {
                case 0x40:
                    break;
                case 0x20:
                    UpdateFinalCameraSequence(object);
                    break;
            }
            break;
        case 4:
            if (UpdateRoundCameraSequence(object) != 0) {
                SetSequenceStatePair(7, 5);
                return;
            }
            break;
        case 5:
            overlayExitRequested = 1;
            break;
    }
}

s8 UpdateIntroSequence(OMOBJ *object)
{
    Vec effectPosition;
    HU3DMODEL *introSceneModel;
    F32 destinationX;
    F32 sourceX;
    F32 effectY;
    F32 effectZ;
    F32 modelPositionX;
    F32 transitionEasing;
    s8 cameraTransitionComplete;
    TextureScrollState *sequenceWork;

    introSceneModel = &Hu3DData[sequenceControllerObject->mdlId[4]];
    sequenceWork = (TextureScrollState *)object->data;
    if (introCameraEnabled != 0) {
        cameraTransitionComplete = UpdateIntroCameraTransition();
    }
    switch (sequenceState) { /* irregular */
        case -1:
            if ((HuPadBtnDown[0] & 0x100) != 0) {
                sequenceState = 0;
            }
            break;
        case 0:
            Hu3DModelAttrReset(sequenceControllerObject->mdlId[1], HU3D_MOTATTR_PAUSE);
            if (introTransitionProgress == 0.0f) {
                AdvanceGameSequence();
                effectPosition.x = 1520.0f;
                effectPosition.y = 0.0f;
                effectPosition.z = -1200.0f;
                introSoundEmitterId = HuAudFXEmiterPlay(0x554, &effectPosition);
                HuAudFXEmiterPlay(0x559, &effectPosition);
            }
            introTransitionProgress += 0.004f;
            if (introTransitionProgress >= 1.0f) {
                introTransitionProgress = 1.0f;
                sequenceState = 0x63;
            }
            else if (introTransitionProgress > 0.2f) {
                introCameraEnabled = 1;
            }
            transitionEasing = sind(90.0f * introTransitionProgress);
            sourceX = 1520.0f;
            destinationX = -1030.0f;
            modelPositionX = sourceX + (transitionEasing * (destinationX - sourceX));
            Hu3DModelPosSet(sequenceControllerObject->mdlId[4], modelPositionX, 0.0f, -1200.0f);
            Hu3DModelPosSet(sequenceControllerObject->mdlId[1], modelPositionX, 0.0f, -1200.0f);
            if (introSoundEmitterId != ~0) {
                effectPosition.x = modelPositionX;
                effectPosition.y = 0.0f;
                effectPosition.z = -1200.0f;
                HuAudFXEmiterUpDate(introSoundEmitterId, &effectPosition);
            }
            transitionEasing = 1.2f * introTransitionProgress;
            if ((transitionEasing >= 1.0f) && introSoundEmitterId != ~0) {
                HuAudFXEmiterPlay(0x555, &effectPosition);
                effectPosition.x += 200.0f;
                HuAudFXEmiterPlay(0x55E, &effectPosition);
                HuAudFXStop(introSoundEmitterId);
                introSoundEmitterId = -1;
            }
            if (transitionEasing >= 1.0f) {
                transitionEasing = 0.0f;
            }
            else {
                transitionEasing = sind(90.0f + 90.0f * transitionEasing);
            }
            Hu3DMotionSpeedSet(sequenceControllerObject->mdlId[4], 2.0f * transitionEasing);
            if (introTransitionProgress > 0.5f) {
                transitionEasing = 2.0f * (9.0f * (1.0f - introTransitionProgress));
            }
            else {
                transitionEasing = 9.0f;
            }
            CreateSceneEffect(sceneEffectObject->mdlId[0], 0.0f, 0.0f, 0.0f, 0, transitionEasing);
            if ((introTransitionProgress > 0.7f) && (introTransitionProgress < 0.9f)) {
                transitionEasing = (introTransitionProgress - 3.5f);
                if (++particleBurstCounter > (transitionEasing * 10.0f)) {
                    particleBurstCounter = 0.0f;
                    modelPositionX = 190.0f + introSceneModel->pos.x;
                    effectY = 20.0f + introSceneModel->pos.y;
                    effectZ = 95.0f + introSceneModel->pos.z;
                    transitionEasing = 1.0f - (introTransitionProgress - 3.5f);
                    CreateSceneEffectAtPosition(sceneEffectObject->mdlId[1], modelPositionX, effectY, effectZ, (u8)(2.0f * transitionEasing));
                    modelPositionX = 370.0f + introSceneModel->pos.x;
                    CreateSceneEffectAtPosition(sceneEffectObject->mdlId[1], modelPositionX, effectY, effectZ, (u8)transitionEasing);
                    modelPositionX = 490.0f + introSceneModel->pos.x;
                    CreateSceneEffectAtPosition(sceneEffectObject->mdlId[1], modelPositionX, effectY, effectZ, (u8)transitionEasing);
                }
            }
            if (++particleSpawnCounter > 1) {
                particleSpawnCounter = 0;
                CreateSceneEffect(sceneEffectObject->mdlId[0], 190.0f + introSceneModel->pos.x, 15.0f, introSceneModel->pos.z + 95.0f, 1, 1);
                CreateSceneEffect(sceneEffectObject->mdlId[0], 370.0f + introSceneModel->pos.x, 15.0f, introSceneModel->pos.z + 95.0f, 1, 1);
                CreateSceneEffect(sceneEffectObject->mdlId[0], 490.0f + introSceneModel->pos.x, 15.0f, introSceneModel->pos.z + 95.0f, 1, 1);
                CreateSceneEffect(sceneEffectObject->mdlId[0], 190.0f + introSceneModel->pos.x, 15.0f, introSceneModel->pos.z - 95.0f, 1, 1);
                CreateSceneEffect(sceneEffectObject->mdlId[0], 370.0f + introSceneModel->pos.x, 15.0f, introSceneModel->pos.z - 95.0f, 1, 1);
                CreateSceneEffect(sceneEffectObject->mdlId[0], 490.0f + introSceneModel->pos.x, 15.0f, introSceneModel->pos.z - 95.0f, 1, 1);
            }
            break;
        case 99:
            if (cameraTransitionComplete == 0) {
                return 0;
            }
            sequenceWork->reservedState = MGSeqCreate(3, 0);
            MGSeqPosSet(sequenceWork->reservedState, 320.0f, 240.0f);
            sequenceState = 0x270F;
            break;
        default:
            if (MGSeqStatGet(sequenceWork->reservedState) == 0) {
                sequenceState = 0;
                return 1;
            }
            if ((musicSequenceId == -1) && ((MGSeqStatGet(sequenceWork->reservedState) & 0x10) != 0)) {
                musicSequenceId = HuAudSeqPlay(0x48);
            }
            break;
    }
    return 0;
}

s8 UpdateRoundCameraSequence(OMOBJ *object)
{
    s16 unusedModelId;
    F32 unusedTransitionWeight;
    SceneModelWork *sequenceWork;

    sequenceWork = (SceneModelWork *)object->data;
    switch (sequenceState) {
        case 0:
            sequenceWork->secondaryModelId = MGSeqCreate(3, 1);
            MGSeqPosSet(sequenceWork->secondaryModelId, 320.0f, 240.0f);
            HuAudSeqFadeOut(musicSequenceId, 0x64);
            roundSequenceTimer = 0;
            sequenceState = 1;
            break;
        case 1:
            if (++roundSequenceTimer > 30) {
                sequenceState = 2;
            }
            break;
        case 2:
            if (UpdateOutroCameraTransition()) {
                sequenceState = 3;
            }
            break;
        case 3:
            SetSequenceStatePair(0x80, 0x80);
            if (sequenceWork->secondaryModelId >= 0) {
                GWPlayerCoinWinAdd(sequenceWork->secondaryModelId, 10);
                sequenceWork->secondaryModelId = MGSeqCreate(5, 3, GWPlayerCfg[sequenceWork->secondaryModelId].character, -1, -1, -1);
                HuAudSStreamPlay(1);
            }
            else {
                sequenceWork->secondaryModelId = MGSeqCreate(3, 2);
                HuAudSStreamPlay(4);
            }
            MGSeqPosSet(sequenceWork->secondaryModelId, 320.0f, 240.0f);
            sequenceState = 4;
            roundSequenceTimer = 0;
            break;
        case 4:
            roundSequenceTimer++;
            if (MGSeqStatGet(sequenceWork->secondaryModelId) == 0) {
                sequenceState = 5;
            }
            break;
        case 5:
            roundSequenceTimer++;
#if VERSION_PAL
            if (roundSequenceTimer > 175.0f) {
#else
            if (roundSequenceTimer > 210.0f) {
#endif
                return 1;
            }
            break;
    }
    return 0;
}

void UpdateFinalCameraSequence(OMOBJ *object)
{
    HU3DMODEL *finalSceneModel;
    F32 destinationX;
    F32 sourceX;
    F32 transitionEasing;
    F32 modelPositionX;
    s32 unusedModelId;

    finalSceneModel = &Hu3DData[sequenceControllerObject->mdlId[4]];
    if (GetSequenceState(0x18) != 0x18) {
        finalTransitionProgress += 0.008f;
        if (finalTransitionProgress >= 1.0f) {
            SetSequenceStatePair(0x60, 0x40);
            Hu3DModelAttrSet(sequenceControllerObject->mdlId[4], HU3D_ATTR_DISPOFF);
            finalTransitionProgress = 1.0f;
        }
        transitionEasing = sind(90.0f * finalTransitionProgress);
        sourceX = -1030.0f;
        destinationX = -1880.0f;
        modelPositionX = sourceX + (transitionEasing * (destinationX - sourceX));
        Hu3DModelPosSet(sequenceControllerObject->mdlId[4], modelPositionX, 0.0f, -1200.0f);
        Hu3DMotionSpeedSet(sequenceControllerObject->mdlId[4], 3.0f * transitionEasing);
        transitionEasing = (4.0f * (9.0f * finalTransitionProgress));
        CreateSceneEffect(sceneEffectObject->mdlId[0], 0.0f, 0.0f, 0.0f, 0, transitionEasing);
        CreateSceneEffect(sceneEffectObject->mdlId[0], 190.0f + modelPositionX, 15.0f, 95.0f + finalSceneModel->pos.z, 1, 1);
        CreateSceneEffect(sceneEffectObject->mdlId[0], 370.0f + modelPositionX, 15.0f, 95.0f + finalSceneModel->pos.z, 1, 1);
        CreateSceneEffect(sceneEffectObject->mdlId[0], 490.0f + modelPositionX, 15.0f, 95.0f + finalSceneModel->pos.z, 1, 1);
    }
}

void SetSequenceState(s16 playerIndex)
{
    SceneModelWork *sequenceWork;

    sequenceWork = (SceneModelWork *)sceneEffectObject->data;
    sequenceWork->secondaryModelId = playerIndex;
}

void SetSequenceStatePair(u16 stateMask, u16 newState)
{
    SceneModelWork *sequenceWork;

    sequenceWork = (SceneModelWork *)sceneEffectObject->data;
    sequenceWork->sequenceFlags &= ~stateMask;
    sequenceWork->sequenceFlags |= newState;
}

u16 GetSequenceState(u16 stateMask)
{
    SceneModelWork *sequenceWork;

    sequenceWork = (SceneModelWork *)sceneEffectObject->data;
    return sequenceWork->sequenceFlags & stateMask;
}

static inline void InitializeSceneModelSequence(SceneModelWork *sequenceWork, s16 sequenceSlot, s16 *sequenceDelay, s16 isIntroSequence)
{
    sequenceWork[sequenceSlot].primaryModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M409, 0x02));
    sequenceWork[sequenceSlot].secondaryModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M409, 0x03));
    Hu3DModelAttrSet(sequenceWork[sequenceSlot].primaryModelId, HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(sequenceWork[sequenceSlot].secondaryModelId, HU3D_MOTATTR_PAUSE);
    if (isIntroSequence) {
        Hu3DModelAttrSet(sequenceWork[sequenceSlot].primaryModelId, HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(sequenceWork[sequenceSlot].secondaryModelId, HU3D_ATTR_DISPOFF);
    }
    sequenceWork[sequenceSlot].movingModelId = Hu3DModelCreateFile(0x2001C);
    sequenceWork[sequenceSlot].forwardMotionId = Hu3DJointMotionFile(sequenceWork[sequenceSlot].movingModelId, DATA_MAKE_NUM(DATADIR_M409, 11));
    sequenceWork[sequenceSlot].reverseMotionId = Hu3DJointMotionFile(sequenceWork[sequenceSlot].movingModelId, DATA_MAKE_NUM(DATADIR_M409, 12));
    Hu3DModelAttrSet(sequenceWork[sequenceSlot].movingModelId, HU3D_MOTATTR_LOOP);
    Hu3DMotionSet(sequenceWork[sequenceSlot].movingModelId, sequenceWork[sequenceSlot].forwardMotionId);
    sequenceWork[sequenceSlot].sequenceIndex = sequenceSlot + 3;
    if (isIntroSequence) {
        *sequenceDelay += (((rand8() << 8) | rand8()) % 4) + 2;
    }
    else {
        *sequenceDelay = 2;
    }
    UpdateSceneModelState(&sequenceWork[sequenceSlot], *sequenceDelay, sequenceSlot);
    Hu3DModelPosSet(sequenceWork[sequenceSlot].primaryModelId, sequenceWork[sequenceSlot].transitionStartX, 0.0f, -1200.0f);
    Hu3DModelPosSet(sequenceWork[sequenceSlot].secondaryModelId, sequenceWork[sequenceSlot].transitionStartX, 0.0f, -1200.0f);
}

void CreateGameSequenceController(OMOBJ *object)
{
    s16 initialSequenceDelay; // ! - uninitialized
    s16 sceneModelIndex;
    SceneModelWork *sequenceWork;

    sequenceWork = &sceneModelSequences[0];
    object->stat |= 0x100;
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M409, 0x00));
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M409, 0x08));
    Hu3DModelLayerSet(object->mdlId[0], 2);
    Hu3DModelLayerSet(object->mdlId[5], 2);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M409, 0x05));
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M409, 0x06));
    Hu3DModelScaleSet(object->mdlId[2], 1.1f, 1.0f, 1.0f);
    Hu3DModelScaleSet(object->mdlId[3], 1.1f, 1.0f, 1.0f);
    Hu3DModelRotSet(object->mdlId[2], 0.2f * cameraPresets->cameraRotation.x, 0.85f * cameraPresets->cameraRotation.y, 0.0f);
    Hu3DModelRotSet(object->mdlId[3], 0.2f * cameraPresets->cameraRotation.x, 0.85f * cameraPresets->cameraRotation.y, 0.0f);
    Hu3DModelLayerSet(object->mdlId[2], 2);
    Hu3DModelLayerSet(object->mdlId[3], 2);
    sceneTextureScrollState.cloudScrollLayer2 = Hu3DTexScrollCreate(object->mdlId[2], "cr_kumo2");
    Hu3DTexScrollPosMoveSet(sceneTextureScrollState.cloudScrollLayer2, -0.0002f, 0.0f, 0.0f);
    sceneTextureScrollState.cloudScrollLayer3 = Hu3DTexScrollCreate(object->mdlId[3], "cr_kumo3");
    Hu3DTexScrollPosMoveSet(sceneTextureScrollState.cloudScrollLayer3, -0.0003f, 0.0f, 0.0f);
    sceneModelIndex = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M409, 0x0A));
    Hu3DModelLayerSet(sceneModelIndex, 0);
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M409, 0x07));
    Hu3DModelAttrSet(object->mdlId[6], HU3D_ATTR_ZWRITE_OFF);
    Hu3DModelScaleSet(object->mdlId[6], 1.1f, 1.0f, 1.0f);
    Hu3DModelRotSet(object->mdlId[6], 0.2f * cameraPresets->cameraRotation.x, 0.85f * cameraPresets->cameraRotation.y, 0.0f);
    Hu3DModelLayerSet(object->mdlId[6], 1);
    sceneTextureScrollState.backgroundScrollLayer = Hu3DTexScrollCreate(object->mdlId[6], "bg");
    sceneTextureScrollState.backgroundScrollOffset = 0.0f;
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M409, 0x01));
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M409, 0x04));
    Hu3DModelAttrSet(object->mdlId[4], HU3D_MOTATTR_LOOP);
    Hu3DModelPosSet(object->mdlId[4], 1660.0f, 0.0f, -1200.0f);
    Hu3DModelPosSet(object->mdlId[1], 1660.0f, 0.0f, -1200.0f);
    Hu3DModelLayerSet(object->mdlId[1], 6);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_PAUSE);

    InitializeSceneModelSequence(sequenceWork, 0, &initialSequenceDelay, 0);
    for (sceneModelIndex = 1; sceneModelIndex < 4; sceneModelIndex++) {
        InitializeSceneModelSequence(sequenceWork, sceneModelIndex, &initialSequenceDelay, 1);
    }

    sceneTextureScrollState.sequenceBusy = 0;
    sceneTextureScrollState.textureEventState = 0;
    sceneTextureScrollState.textureEventTimer = 0;
    sceneTextureScrollState.textureEventDuration = 0xB4;
    Hu3DModelShadowMapSet(object->mdlId[0]);
    Hu3DModelShadowSet(object->mdlId[4]);

    for (sceneModelIndex = 0; sceneModelIndex < 4; sceneModelIndex++) {
        Hu3DModelShadowSet(sequenceWork[sceneModelIndex].primaryModelId);
        Hu3DModelShadowSet(sequenceWork[sceneModelIndex].secondaryModelId);
    }
    sceneEffectCount = 0;
    object->objFunc = UpdateGameSequenceController;
}

void UpdateGameSequenceController(OMOBJ *object)
{
    u16 unusedStateValue1;
    u16 unusedStateValue2;
    u16 unusedStateValue3;
    u16 unusedStateValue4;
    HU3DMODEL *sequenceModel;
    F32 motionEndTime;
    F32 transitionInterpolation;
    F32 transitionStep;
    s16 minimumSequenceIndex;
    s16 inactiveSequenceCount;
    s16 linkedSequenceIndex;
    s16 nextSequenceIndex;
    s16 sequenceIndex;
    s32 unusedSequenceValue;
    s16 unusedSequenceState;
    SceneModelWork *sequenceWork;

    if ((sceneTextureScrollState.sequenceBusy == 0) || (GetSequenceState(0x18) == 0x18)) {
        if (sceneTextureScrollState.textureEventState != 0) {
            sequenceWork = sceneTextureScrollState.activeModelSequence;
            switch (sceneTextureScrollState.textureEventState) {
                case 1:
                    if (++sceneTextureScrollState.textureEventTimer == (sceneTextureScrollState.textureEventDuration - 5)) {
                        Hu3DMotionSet(sequenceWork->movingModelId, sequenceWork->reverseMotionId);
                        Hu3DModelAttrReset(sequenceWork->movingModelId, HU3D_MOTATTR_LOOP);
                        sceneEffectCount++;
                        break;
                    }
                    if (sceneTextureScrollState.textureEventTimer > sceneTextureScrollState.textureEventDuration) {
                        sceneTextureScrollState.textureEventTimer = 0;
                        sceneTextureScrollState.textureEventDuration = 0xF;
                        Hu3DModelAttrReset(sequenceWork->secondaryModelId, HU3D_MOTATTR_PAUSE);
                        sceneTextureScrollState.textureEventState = 2;
                        HuAudFXPlay(0x55B);
                        HuAudFXPlay(0x55D);
                    }
                    break;
                case 2:
                    motionEndTime = Hu3DMotionMaxTimeGet(sequenceWork->secondaryModelId);
                    if (Hu3DMotionTimeGet(sequenceWork->secondaryModelId) >= motionEndTime) {
                        Hu3DMotionSet(sequenceWork->movingModelId, sequenceWork->forwardMotionId);
                        Hu3DModelAttrSet(sequenceWork->movingModelId, HU3D_MOTATTR_LOOP);
                        Hu3DModelAttrSet(sequenceWork->secondaryModelId, HU3D_MOTATTR_PAUSE);
                        SetSequenceStatePair(24, 8);
                        sceneTextureScrollState.textureEventState = 0;
                    }
                    break;
            }
        }
        else {
            sequenceWork = sceneTextureScrollState.activeModelSequence;
            switch (GetSequenceState(0x18)) {
                case 16:
                    Hu3DMotionSet(sequenceWork->movingModelId, sequenceWork->reverseMotionId);
                    Hu3DMotionTimeSet(sequenceWork->movingModelId, Hu3DMotionMaxTimeGet(sequenceWork->movingModelId));
                    Hu3DMotionSpeedSet(sequenceWork->movingModelId, -1.0f);
                    Hu3DModelAttrReset(sequenceWork->movingModelId, HU3D_MOTATTR_LOOP);
                    Hu3DMotionSpeedSet(sequenceWork->secondaryModelId, -1.0f);
                    Hu3DModelAttrReset(sequenceWork->secondaryModelId, HU3D_MOTATTR_PAUSE);
                    SetSequenceStatePair(24, 0x18);
                    HuAudFXPlay(0x55C);
                    HuAudFXPlay(0x55D);
                    break;
                case 24:
                    if (Hu3DMotionTimeGet(sequenceWork->secondaryModelId) <= 0.0f) {
                        Hu3DMotionSet(sequenceWork->movingModelId, sequenceWork->forwardMotionId);
                        Hu3DMotionSpeedSet(sequenceWork->movingModelId, 1.0f);
                        Hu3DModelAttrSet(sequenceWork->movingModelId, HU3D_MOTATTR_LOOP);
                        Hu3DMotionSpeedSet(sequenceWork->secondaryModelId, 1.0f);
                        Hu3DModelAttrSet(sequenceWork->secondaryModelId, HU3D_MOTATTR_PAUSE);
                        SetSequenceStatePair(24, 0);
                    }
                    break;
            }
        }
    }
    else {
        if (GetSequenceState(7) == 2) {
            transitionStep = 0.004f;
        }
        else {
            transitionStep = 0.008f;
        }
        sequenceIndex = 0;
        inactiveSequenceCount = 0;
        sequenceWork = sceneModelSequences;

        for (; sequenceIndex < 4; sequenceIndex++, sequenceWork++) {
            if ((s8)sequenceWork->isActive == 0) {
                inactiveSequenceCount++;
            }
            else {
                if (sequenceWork->sequenceDelay-- <= 0) {
                    sequenceModel = &Hu3DData[sequenceWork->primaryModelId];
                    if ((sequenceWork->sequenceIndex < 2) && (sceneEffectCount < 0xBU)) {
                        Hu3DModelAttrReset(sequenceWork->primaryModelId, HU3D_ATTR_DISPOFF);
                        Hu3DModelAttrReset(sequenceWork->secondaryModelId, HU3D_ATTR_DISPOFF);
                        Hu3DModelAttrReset(sequenceWork->movingModelId, HU3D_ATTR_DISPOFF);
                    }
                    sequenceWork->transitionProgress += transitionStep;
                    transitionInterpolation = sind(90.0f * sequenceWork->transitionProgress);
                    if (sequenceWork->transitionProgress >= 1.0f) {
                        transitionInterpolation = 1.0f;
                        sequenceWork->transitionProgress = transitionInterpolation;
                        sequenceModel->pos.x = sequenceWork->transitionEndX;
                        if (sequenceWork->sequenceIndex == 0) {
                            sceneTextureScrollState.activeModelSequence = sequenceWork;
                        }
                        nextSequenceIndex = sequenceWork->sequenceIndex - 1;
                        if (nextSequenceIndex == -3) {
                            Hu3DModelAttrSet(sequenceWork->primaryModelId, HU3D_ATTR_DISPOFF);
                            Hu3DModelAttrSet(sequenceWork->secondaryModelId, HU3D_ATTR_DISPOFF);
                            Hu3DModelAttrSet(sequenceWork->movingModelId, HU3D_ATTR_DISPOFF);
                            sequenceWork->sequenceIndex = 2;
                            nextSequenceIndex = 1;
                        }
                        UpdateSceneModelState(sequenceWork, 0, nextSequenceIndex);
                    }
                    else {
                        sequenceModel->pos.x = sequenceWork->transitionStartX + (transitionInterpolation * (sequenceWork->transitionEndX - sequenceWork->transitionStartX));
                    }
                    if (transitionInterpolation > 0.1f) {
                        if (++sequenceWork->effectSpawnCount > 1) {
                            sequenceWork->effectSpawnCount = 0;
                            CreateSceneEffect(*sceneEffectObject->mdlId, 130.0f + sequenceModel->pos.x, 15.0f, 95.0f + sequenceModel->pos.z, 1, 1);
                            CreateSceneEffect(*sceneEffectObject->mdlId, 630.0f + sequenceModel->pos.x, 15.0f, 95.0f + sequenceModel->pos.z, 1, 1);
                        }
                    }
                    if ((sequenceWork->transitionProgress > 0.7f) && (sequenceWork->transitionProgress < 0.9f)) {
                        transitionInterpolation = sequenceWork->transitionProgress - 3.5f;
                        if (++particleBurstTimer > (40.0f * transitionInterpolation)) {
                            particleBurstTimer = 0.0f;
                            transitionInterpolation = 1.0f - transitionInterpolation;
                            CreateSceneEffectAtPosition(sceneEffectObject->mdlId[1], 130.0f + sequenceModel->pos.x, 20.0f + sequenceModel->pos.y, 95.0f + sequenceModel->pos.z,
                                (u8)(2.0f * transitionInterpolation));
                            CreateSceneEffectAtPosition(
                                sceneEffectObject->mdlId[1], 630.0f + sequenceModel->pos.x, 20.0f + sequenceModel->pos.y, 95.0f + sequenceModel->pos.z, (u8)transitionInterpolation);
                        }
                        if (sequenceWork->transitionProgress < (0.7f + transitionStep)) {
                            HuAudFXEmiterPlay(0x55E, &sequenceModel->pos);
                        }
                    }
                    Hu3DModelPosSet(sequenceWork->movingModelId, 70.0f + sequenceModel->pos.x, 290.0f, -1200.0f);
                    Hu3DData[sequenceWork->secondaryModelId].pos.x = sequenceModel->pos.x;
                    transitionInterpolation = 0.8f * sind(180.0f * sequenceWork->transitionProgress);
                    Hu3DMotionSpeedSet(sequenceWork->primaryModelId, transitionInterpolation);
                }
            }
        }
        if (inactiveSequenceCount == 4) {
            minimumSequenceIndex = 0x63;
            sceneTextureScrollState.sequenceBusy = 0;

            for (sequenceIndex = 0, sequenceWork = &sceneModelSequences[0]; sequenceIndex < 4; sequenceIndex++, sequenceWork++) {
                if (sequenceWork->sequenceIndex < minimumSequenceIndex) {
                    minimumSequenceIndex = sequenceWork->sequenceIndex;
                    linkedSequenceIndex = sequenceIndex;
                }

                for (nextSequenceIndex = 0, sequenceWork->nextSequence = NULL; nextSequenceIndex < 4; nextSequenceIndex++) {
                    if ((sequenceWork->sequenceIndex + 1) == sceneModelSequences[nextSequenceIndex].sequenceIndex) {
                        sequenceWork->nextSequence = &sceneModelSequences[nextSequenceIndex];
                    }
                }
            }
            sequenceWork = &sceneModelSequences[linkedSequenceIndex];
            sequenceWork->sequenceDelay = 0;

        loop:
            if (sequenceWork->nextSequence) {
                sequenceWork->nextSequence->sequenceDelay = 2 + (((rand8() << 8) | rand8()) % 4) + sequenceWork->sequenceDelay;
                sequenceWork = sequenceWork->nextSequence;
                goto loop;
            }
            sceneTextureScrollState.textureEventState = 1;
        }
    }
}

void AdvanceGameSequence(void)
{
    s16 sequenceIndex;

    if ((sceneTextureScrollState.sequenceBusy == 0) && (GetSequenceState(7) != 4)) {
        sceneTextureScrollState.sequenceBusy = 1;

        for (sequenceIndex = 0; sequenceIndex < 4; sequenceIndex++) {
            sceneModelSequences[sequenceIndex].isActive = 1;
        }
        if (++gameSequenceCount == 2) {

            for (sequenceIndex = 0; sequenceIndex < 4; sequenceIndex++) {
                sceneModelSequences[sequenceIndex].sequenceDelay += 3;
            }
            SetSequenceStatePair(0x60, 0x20);
        }
        HuAudFXPlay(0x55F);
    }
}

void UpdateSceneModelState(SceneModelWork *sequenceWork, s16 sequenceDelay, s16 destinationSequenceIndex)
{
    sequenceWork->isActive = 0;
    sequenceWork->sequenceDelay = sequenceDelay;
    sequenceWork->transitionProgress = 0.0f;
    sequenceWork->transitionStartX = ((850.0f * sequenceWork->sequenceIndex) - 370.0f);
    sequenceWork->transitionEndX = ((850.0f * destinationSequenceIndex) - 370.0f);
    sequenceWork->sequenceIndex = destinationSequenceIndex;
    if ((sequenceWork->sequenceIndex >= 2) && (GetSequenceState(7) != 2)) {
        Hu3DModelAttrSet(sequenceWork->primaryModelId, HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(sequenceWork->secondaryModelId, HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(sequenceWork->movingModelId, HU3D_ATTR_DISPOFF);
    }
}

s32 InterpolateCameraParameter(s32 sceneModelId, F32 segmentStartX, F32 segmentStartY, F32 segmentStartZ, F32 segmentDeltaX, F32 segmentDeltaY, F32 segmentDeltaZ)
{
    F32 nearestSegmentParameter;
    F32 closestPointDistance;
    HU3DMODEL *sequenceModel;
    F32 modelX;
    F32 modelY;
    F32 modelZ;
    F32 segmentEndX;
    F32 segmentEndY;
    F32 segmentEndZ;
    s16 linkedSequenceIndex;
    SceneModelWork *sequenceWork;

    sequenceWork = sceneTextureScrollState.activeModelSequence;
    if (!sequenceWork)
        return;

    for (linkedSequenceIndex = 0; linkedSequenceIndex < 2; linkedSequenceIndex++) {
        sequenceModel = &Hu3DData[sequenceWork->movingModelId];
        modelX = sequenceModel->pos.x;
        modelY = 75.0f + sequenceModel->pos.y;
        modelZ = sequenceModel->pos.z;
        segmentEndX = segmentStartX + segmentDeltaX;
        segmentEndY = segmentStartY + segmentDeltaY;
        segmentEndZ = segmentStartZ + segmentDeltaZ;
        closestPointDistance = sqrtf((segmentEndX - modelX) * (segmentEndX - modelX) + (segmentEndY - modelY) * (segmentEndY - modelY)
            + (segmentEndZ - modelZ) * (segmentEndZ - modelZ));
        if (closestPointDistance < 50.0f) {
            return 1;
        }
        nearestSegmentParameter = -((segmentStartZ * segmentDeltaZ) + (((segmentStartY * segmentDeltaY) + (((-modelX * segmentDeltaX) + (segmentStartX * segmentDeltaX)) - (modelY * segmentDeltaY))) - (modelZ * segmentDeltaZ)))
            / ((segmentDeltaZ * segmentDeltaZ) + ((segmentDeltaX * segmentDeltaX) + (segmentDeltaY * segmentDeltaY)));
        if (nearestSegmentParameter >= 1.0f)
            continue;

        segmentEndX = segmentStartX + (segmentDeltaX * nearestSegmentParameter);
        segmentEndY = segmentStartY + (segmentDeltaY * nearestSegmentParameter);
        segmentEndZ = segmentStartZ + (segmentDeltaZ * nearestSegmentParameter);
        closestPointDistance = sqrtf((segmentEndX - modelX) * (segmentEndX - modelX) + (segmentEndY - modelY) * (segmentEndY - modelY)
            + (segmentEndZ - modelZ) * (segmentEndZ - modelZ));
        if (closestPointDistance < 50.0f) {
            return 1;
        }
        sequenceWork = sequenceWork->nextSequence;
        continue;
    }
    return 0;
}

void CopyMatrix(Mtx sourceMatrix, Mtx destinationMatrix)
{
    destinationMatrix[0][0] = sourceMatrix[0][0];
    destinationMatrix[1][0] = sourceMatrix[0][1];
    destinationMatrix[2][0] = sourceMatrix[0][2];
    destinationMatrix[0][1] = sourceMatrix[1][0];
    destinationMatrix[1][1] = sourceMatrix[1][1];
    destinationMatrix[2][1] = sourceMatrix[1][2];
    destinationMatrix[0][2] = sourceMatrix[2][0];
    destinationMatrix[1][2] = sourceMatrix[2][1];
    destinationMatrix[2][2] = sourceMatrix[2][2];
    destinationMatrix[0][3] = 0.0f;
    destinationMatrix[1][3] = 0.0f;
    destinationMatrix[2][3] = 0.0f;
}

F32 InterpolateCameraValue(F32 startValue, F32 controlValue, F32 endValue, F32 interpolationWeight)
{
    F32 inverseInterpolationWeight;

    inverseInterpolationWeight = 1.0f - interpolationWeight;
    return (endValue * (interpolationWeight * interpolationWeight)) + ((startValue * (inverseInterpolationWeight * inverseInterpolationWeight)) + (controlValue * (2.0f * inverseInterpolationWeight * interpolationWeight)));
}

void ResetSceneCamera(void)
{

    if ((HuPadBtnDown[0] & 0x40) != 0) {
        isCameraRotationReversed ^= 1;
    }
    if (isCameraRotationReversed != 0) {
        if ((HuPadBtn[0] & 1) != 0) {
            CRot.y++;
        }
        if ((HuPadBtn[0] & 2) != 0) {
            CRot.y--;
        }
        if ((HuPadBtn[0] & 8) != 0) {
            CRot.x--;
        }
        if ((HuPadBtn[0] & 4) != 0) {
            CRot.x++;
        }
        Center.x += HuPadSubStkX[0];
        if ((HuPadBtn[0] & 0x20) != 0) {
            Center.y += HuPadSubStkY[0];
        }
        else {
            Center.z += HuPadSubStkY[0];
        }
        if ((HuPadBtn[0] & 0x400) != 0) {
            CZoom += 10.0f;
        }
        if ((HuPadBtn[0] & 0x800) != 0) {
            CZoom -= 10.0f;
        }
        if ((HuPadBtnDown[0] & 0x100) != 0) {
            OSReport("\nCZoom = %.2f \n", CZoom);
            OSReport("Center x = %.2f: y = %.2f: z = %.2f \n", Center.x, Center.y, Center.z);
            OSReport("CRot x = %.2f: y = %.2f: z = %.2f \n", CRot.x, CRot.y, CRot.z);
        }
    }
}

void CreateSceneEffect(s16 sceneModelId, F32 positionX, F32 positionY, F32 positionZ, s8 effectType, s16 particleCount)
{
    Vec particlePosition;
    Vec particleScale;
    SceneModelWork *unusedSceneSequence;
    GXColor particleColor;
    HU3DMODEL *sourceSceneModel;
    F32 particleAngle;
    s16 particleIndex;
    SceneParticleWork *particleWork;

    sourceSceneModel = &Hu3DData[sequenceControllerObject->mdlId[4]];
    unusedSceneSequence = (SceneModelWork *)sourceSceneModel->hookData;
    if (effectType == 0) {
        positionX = 150.0f + sourceSceneModel->pos.x;
        positionY = 360.0f + sourceSceneModel->pos.y;
        positionZ = sourceSceneModel->pos.z;
    }

    for (particleIndex = 0; particleIndex < particleCount; particleIndex++) {
        switch (effectType) { /* irregular */
            case 0:
                particlePosition.x = positionX + ((((rand8() << 8) | rand8()) % 41) - 20);
                particlePosition.z = positionZ + ((((rand8() << 8) | rand8()) % 41) - 20);
                particlePosition.y = positionY;
                particleScale.x = 0.01f * ((((rand8() << 8) | rand8()) % 101) + 200);
                particleScale.y = 0.01f * ((((rand8() << 8) | rand8()) % 101) + 200);
                particleScale.z = 0.01f * ((((rand8() << 8) | rand8()) % 101) + 200);
                particleColor.r = particleColor.g = particleColor.b = (((rand8() << 8) | rand8()) % 33) + 32;
                particleColor.a = 0xFF;
                particleWork = CreateSceneParticle(sceneModelId, &particlePosition, &particleScale, 0.0f, &particleColor);
                if (!particleWork)
                    return;

                particleAngle = (((rand8() << 8) | rand8()) % 361);
                particleWork->particleVelocity.x = 2.0 + (3.0 * sind(particleAngle));
                particleWork->particleVelocity.y = (0.1f * ((((rand8() << 8) | rand8()) % 31) + 40));
                particleWork->particleVelocity.z = (3.5 * cosd(particleAngle));
                particleWork->animationFrame = 0;
                particleWork->animationFrameInterval = ((((rand8() << 8) | rand8()) % 3) + 2);
                particleWork->particleColorIntensity = (F32)particleColor.r;
                particleWork->colorIntensityStep = (((((rand8() << 8) | rand8()) % 69) + 0x80) / (F32)(particleWork->animationFrameInterval * 0xE));
                particleWork->particleAlpha = 255.0f;
                particleWork->alphaFadeSpeed = (((((rand8() << 8) | rand8())) % 128 + 0x80) / (F32)(particleWork->animationFrameInterval * 0xE));
                break;
            case 1:
                particlePosition.x = positionX + ((((rand8() << 8) | rand8()) % 41) - 20);
                particlePosition.z = positionZ + ((((rand8() << 8) | rand8()) % 41) - 20);
                particlePosition.y = positionY + ((((rand8() << 8) | rand8()) % 41) - 20);
                particleScale.x = 0.01f * ((((rand8() << 8) | rand8()) % 101) + 300);
                particleScale.y = 0.01f * ((((rand8() << 8) | rand8()) % 101) + 300);
                particleScale.z = 0.01f * ((((rand8() << 8) | rand8()) % 101) + 300);
                particleColor.r = 0xFF;
                particleColor.g = 0xDC;
                particleColor.b = 0x8C;
                particleColor.a = 0x80;
                particleWork = CreateSceneParticle(sceneModelId, &particlePosition, &particleScale, 0.0f, &particleColor);
                if (!particleWork)
                    return;

                particleWork->animationFrame = 0;
                particleWork->animationFrameInterval = ((((rand8() << 8) | rand8()) % 3) + 2);
                particleWork->particleVelocity.x = (0.1f * ((((rand8() << 8) | rand8()) % 11) + 30));
                particleWork->particleVelocity.y = 0.0f;
                particleWork->particleVelocity.z = (0.1f * ((((rand8() << 8) | rand8()) % 41) - 20));
                particleWork->particleAlpha = 32.0f;
                particleWork->alphaFadeSpeed = (32.0f / (particleWork->animationFrameInterval * 0xE));
                break;
        }
        particleWork->particleEffectType = effectType;
    }
}

void CreateSceneEffectAtPosition(s16 sceneModelId, F32 positionX, F32 positionY, F32 positionZ, s16 particleCount)
{
    Vec particlePosition;
    Vec particleScale;
    GXColor particleColor;
    s16 unusedParticleState;
    F32 particleAngle;
    s16 particleIndex;
    SceneParticleWork *particleWork;

    for (particleIndex = 0; particleIndex < particleCount; particleIndex++) {
        particlePosition.x = positionX + ((((rand8() << 8) | rand8()) % 21) - 10);
        particlePosition.y = positionY;
        particlePosition.z = positionZ + ((((rand8() << 8) | rand8()) % 21) - 10);
        particleScale.x = 0.1f * ((((rand8() << 8) | rand8()) % 31) + 10);
        particleScale.y = 1.0f;
        particleScale.z = 1.0f;
        particleColor.r = particleColor.g = particleColor.b = particleColor.a = 0xFF;
        particleWork = CreateSceneParticle(sceneModelId, &particlePosition, &particleScale, 0.0f, &particleColor);
        if (!particleWork)
            break;

        particleWork->particleLifetime = -1;
        particleAngle = ((((rand8() << 8) | rand8()) % 81) - 40);
        particleWork->particleVelocity.x = (13.0 * cosd(particleAngle));
        particleWork->particleVelocity.y = 0.1f * ((((rand8() << 8) | rand8()) % 21) + 20);
        particleWork->particleVelocity.z = (9.0 * sind(particleAngle));
        particleWork->particleAngularVelocity.x = (0.05f * particleWork->particleVelocity.x);
        particleWork->particleAngularVelocity.z = (0.05f * particleWork->particleVelocity.z);
        particleWork->particleColorIntensity = 500.0f;
        particleWork->colorIntensityStep = ((((rand8() << 8) | rand8()) % 21) + 40);
    }
}

void DrawSceneModelHook(HU3DMODEL *model, SceneModelHookWork *hookWork, Mtx transform)
{
    s16 particleIndex;
    GXColor *particleColor;
    SceneParticleWork *particleWork;

    particleIndex = 0;
    particleWork = hookWork->particleWork;
    particleColor = hookWork->particleColors;

    for (; particleIndex < hookWork->particleCapacity; particleIndex++, particleWork++, particleColor++) {
        if (particleWork->isActive != 0) {
            switch (particleWork->particleEffectType) {
                case 0:
                    particleWork->particleVelocity.y -= 0.1f;
                    particleWork->particlePosition.x += particleWork->particleVelocity.x;
                    particleWork->particlePosition.y += particleWork->particleVelocity.y;
                    particleWork->particlePosition.z += particleWork->particleVelocity.z;
                    particleWork->particleColorIntensity += particleWork->colorIntensityStep;
                    if (particleWork->particleColorIntensity > 255.0f) {
                        particleWork->particleColorIntensity = 255.0f;
                    }
                    particleColor->r = particleColor->g = particleColor->b = particleWork->particleColorIntensity;
                    break;
                case 1:
                    particleWork->particlePosition.x += particleWork->particleVelocity.x;
                    particleWork->particlePosition.z += particleWork->particleVelocity.z;
                    break;
            }
            particleWork->particleAlpha -= particleWork->alphaFadeSpeed;
            if (particleWork->particleAlpha < 0.0f) {
                particleWork->particleAlpha = 0.0f;
            }
            particleColor->a = particleWork->particleAlpha;
            particleWork->animationFrame++;
            if (particleWork->animationFrame >= particleWork->animationFrameInterval) {
                particleWork->animationFrame = 0;
                particleWork->particleLifetime++;
            }
            if (particleWork->particleLifetime >= (hookWork->modelCapacity - 2)) {
                particleWork->isActive = 0;
            }
        }
    }
}

void DrawSceneSpriteHook(HU3DMODEL *model, SceneModelHookWork *hookWork, Mtx transform)
{
    GXColor *particleColor;
    F32 angularVelocityMagnitude;
    F32 particleVelocityMagnitude;
    s16 particleIndex;
    s32 alphaValue;
    SceneParticleWork *particleWork;

    particleIndex = 0;
    particleWork = hookWork->particleWork;
    particleColor = hookWork->particleColors;

    for (; particleIndex < hookWork->particleCapacity; particleIndex++, particleWork++, particleColor++) {
        if (particleWork->isActive != 0) {
            particleWork->particleVelocity.x -= particleWork->particleAngularVelocity.x;
            particleWork->particleVelocity.y -= 0.2f;
            particleWork->particleVelocity.z -= particleWork->particleAngularVelocity.z;
            if (particleWork->particleVelocity.x < particleWork->particleAngularVelocity.x) {
                particleWork->particleVelocity.x = particleWork->particleAngularVelocity.x;
            }
            if (particleWork->particleAngularVelocity.z < 0.0f) {
                angularVelocityMagnitude = -particleWork->particleAngularVelocity.z;
            }
            else {
                angularVelocityMagnitude = particleWork->particleAngularVelocity.z;
            }
            if (particleWork->particleVelocity.z < 0.0f) {
                particleVelocityMagnitude = -particleWork->particleVelocity.z;
            }
            else {
                particleVelocityMagnitude = particleWork->particleVelocity.z;
            }
            if (particleVelocityMagnitude < angularVelocityMagnitude) {
                particleWork->particleVelocity.z = particleWork->particleAngularVelocity.z;
            }
            particleWork->particlePosition.x += particleWork->particleVelocity.x;
            particleWork->particlePosition.y += particleWork->particleVelocity.y;
            particleWork->particlePosition.z += particleWork->particleVelocity.z;
            particleWork->particleColorIntensity -= particleWork->colorIntensityStep;
            if (particleWork->particleColorIntensity < 0.0f) {
                particleWork->particleColorIntensity = 0.0f;
            }
            if (particleWork->particleColorIntensity > 255.0f) {
                alphaValue = 0xFF;
            }
            else {
                alphaValue = (u8)particleWork->particleColorIntensity;
            }
            particleColor->a = alphaValue;
            if (0.0f == particleWork->particleColorIntensity) {
                particleWork->isActive = 0;
            }
        }
    }
}

void DrawSceneParticleHook(HU3DMODEL *model, SceneModelHookWork *hookWork, Mtx transform)
{
    Vec particleScale;
    Vec particlePosition;
    GXColor particleColor;
    HU3DMODEL *sourceModel;
    SceneParticleWork *particleWork;

    sourceModel = &Hu3DData[sequenceControllerObject->mdlId[4]];
    particleWork = hookWork->particleWork;
    if (particleWork->isActive == 0) {
        particleScale.x = particleScale.y = particleScale.z = 1.0f;
        particleColor.r = particleColor.g = particleColor.b = particleColor.a = 0xFF;
        CreateSceneParticle(sceneEffectObject->mdlId[2], &particlePosition, &particleScale, 0.0f, &particleColor);
    }
    particleWork->particlePosition.x = (60.0f + sourceModel->pos.x);
    particleWork->particlePosition.y = (260.0f + sourceModel->pos.y);
    particleWork->particlePosition.z = sourceModel->pos.z;
}

SceneParticleWork *CreateSceneParticle(s16 sceneModelId, Vec *particlePosition, Vec *particleScale, F32 particleRotation, GXColor *particleColor)
{
    s16 particleIndex;
    SceneParticleWork *particleWork;
    SceneModelHookWork *hookWork;
    HU3DMODEL *sceneModel;

    sceneModel = &Hu3DData[sceneModelId];
    hookWork = (SceneModelHookWork *)sceneModel->hookData;

    particleIndex = 0;
    particleWork = hookWork->particleWork;
    for (; particleIndex < hookWork->particleCapacity; particleIndex++, particleWork++) {
        if (particleWork->isActive == 0) {
            break;
        }
    }
    if (particleIndex == hookWork->particleCapacity) {
        return NULL;
    }
    hookWork->particleColors[particleIndex].r = particleColor->r;
    hookWork->particleColors[particleIndex].g = particleColor->g;
    hookWork->particleColors[particleIndex].b = particleColor->b;
    hookWork->particleColors[particleIndex].a = particleColor->a;
    particleWork->particlePoolIndex = particleIndex;
    particleWork->particleRotation = particleRotation;
    particleWork->particleScale = *particleScale;
    particleWork->particlePosition = *particlePosition;
    particleWork->particleLifetime = 0;
    particleWork->isActive = 1;
    return particleWork;
}

s16 CreateSceneModel(ANIMDATA *spriteAnimation, s16 particleCapacity, F32 quadHalfSize, s16 tileWidth, s16 tileHeight)
{
    HU3DMODEL *model;
    F32 unusedModelValue;
    s16 hookModelId;
    s16 textureFrameColumns;
    s16 allocationIndex;
    s16 textureFrameRows;
    SceneModelHookWork *hookWork;
    void *displayListAllocation;
    void *displayListSource;
    SceneParticleWork *particleWork;
    Vec *particleVertices;
    GXColor *particleColors;
    HuVec2f *particleTexcoords;

    hookModelId = Hu3DHookFuncCreate(DrawSceneModelCallback);
    model = &Hu3DData[hookModelId];
    hookWork = HuMemDirectMallocNum(HEAP_MODEL, sizeof(SceneModelHookWork), model->mallocNo);
    model->hookData = hookWork;
    spriteAnimation->useNum++;
    hookWork->particleAnimation = spriteAnimation;
    hookWork->particleCapacity = particleCapacity;
    hookWork->hookFlags = 0;
    hookWork->DrawCallback = NULL;
    particleWork = HuMemDirectMallocNum(HEAP_MODEL, particleCapacity * sizeof(SceneParticleWork), model->mallocNo);
    hookWork->particleWork = particleWork;

    for (allocationIndex = 0; allocationIndex < particleCapacity; allocationIndex++, particleWork++) {
        particleWork->particleLifetime = -1;
        particleWork->isActive = 0;
    }
    particleVertices = HuMemDirectMallocNum(HEAP_MODEL, particleCapacity * sizeof(Vec) * 4, model->mallocNo);
    hookWork->particleVertices = particleVertices;

    for (allocationIndex = 0; allocationIndex < (particleCapacity * 4); allocationIndex++, particleVertices++) {
        particleVertices->x = particleVertices->y = particleVertices->z = 0.0f;
    }
    particleColors = HuMemDirectMallocNum(HEAP_MODEL, particleCapacity * sizeof(GXColor), model->mallocNo);
    hookWork->particleColors = particleColors;

    for (allocationIndex = 0; allocationIndex < particleCapacity; allocationIndex++, particleColors++) {
        particleColors->r = particleColors->g = particleColors->b = particleColors->a = 0xFF;
    }
    particleTexcoords = HuMemDirectMallocNum(HEAP_MODEL, particleCapacity * sizeof(HuVec2f) * 4, model->mallocNo);
    hookWork->particleTexcoords = particleTexcoords;

    for (allocationIndex = 0; allocationIndex < particleCapacity; allocationIndex++) {
        particleTexcoords->x = 0.0f;
        particleTexcoords->y = 0.0f;
        particleTexcoords++;
        particleTexcoords->x = 1.0f;
        particleTexcoords->y = 0.0f;
        particleTexcoords++;
        particleTexcoords->x = 1.0f;
        particleTexcoords->y = 1.0f;
        particleTexcoords++;
        particleTexcoords->x = 0.0f;
        particleTexcoords->y = 1.0f;
        particleTexcoords++;
    }
    if ((tileWidth != 0) || (tileHeight != 0)) {
        textureFrameColumns = spriteAnimation->bmp->sizeX / tileWidth;
        textureFrameRows = spriteAnimation->bmp->sizeY / tileHeight;
        hookWork->modelCapacity = (textureFrameColumns * textureFrameRows);
        hookWork->particleFrameWidth = (1.0f / textureFrameColumns);
        hookWork->particleFrameHeight = (1.0f / textureFrameRows);
    }
    else {
        textureFrameColumns = 1;
        hookWork->modelCapacity = 1;
        hookWork->particleFrameWidth = 1.0f;
        hookWork->particleFrameHeight = 1.0f;
    }
    hookWork->particleTexcoordFrames = HuMemDirectMallocNum(HEAP_MODEL, hookWork->modelCapacity * sizeof(HuVec2f), model->mallocNo);
    SetScreenRectangle(hookWork->particleTexcoordFrames, hookWork->modelCapacity, textureFrameColumns, hookWork->particleFrameWidth, hookWork->particleFrameHeight);
    hookWork->quadTopLeft.x = hookWork->quadBottomRight.x = hookWork->quadBottomLeft.y = hookWork->quadBottomRight.y = -quadHalfSize;
    hookWork->quadTopRight.x = hookWork->quadBottomLeft.x = hookWork->quadTopLeft.y = hookWork->quadTopRight.y = quadHalfSize;
    hookWork->quadTopLeft.z = hookWork->quadTopRight.z = hookWork->quadBottomLeft.z = hookWork->quadBottomRight.z = 0.0f;
    displayListAllocation = displayListSource = HuMemDirectMallocNum(HEAP_MODEL, 0x20000, model->mallocNo);
    DCFlushRange(displayListAllocation, 0x20000);
    GXBeginDisplayList(displayListAllocation, 0x20000);
    GXBegin(GX_QUADS, GX_VTXFMT0, (particleCapacity * 4));

    for (allocationIndex = 0; allocationIndex < particleCapacity; allocationIndex++) {
        GXPosition1x16(allocationIndex * 4);
        GXColor1x16(allocationIndex);
        GXTexCoord1x16(allocationIndex * 4);
        GXPosition1x16((allocationIndex * 4) + 1);
        GXColor1x16(allocationIndex);
        GXTexCoord1x16((allocationIndex * 4) + 1);
        GXPosition1x16((allocationIndex * 4) + 2);
        GXColor1x16(allocationIndex);
        GXTexCoord1x16((allocationIndex * 4) + 2);
        GXPosition1x16((allocationIndex * 4) + 3);
        GXColor1x16(allocationIndex);
        GXTexCoord1x16((allocationIndex * 4) + 3);
    }
    hookWork->displayListCapacity = GXEndDisplayList();
    hookWork->displayList = HuMemDirectMallocNum(HEAP_MODEL, hookWork->displayListCapacity, model->mallocNo);
    memcpy(hookWork->displayList, displayListSource, hookWork->displayListCapacity);
    DCFlushRange(hookWork->displayList, hookWork->displayListCapacity);
    HuMemDirectFree(displayListSource);
    return hookModelId;
}

void KillSceneModel(s16 sceneModelId)
{
    HU3DMODEL *sceneModel;
    SceneModelHookWork *hookWork;

    sceneModel = &Hu3DData[sceneModelId];
    hookWork = (SceneModelHookWork *)sceneModel->hookData;
    HuSprAnimKill(hookWork->particleAnimation);
    Hu3DModelKill(sceneModelId);
}

void SetSceneModelDrawCallback(s16 sceneModelId, SceneModelDrawCallback DrawCallback)
{
    HU3DMODEL *sceneModel;
    SceneModelHookWork *hookWork;

    sceneModel = &Hu3DData[sceneModelId];
    hookWork = (SceneModelHookWork *)sceneModel->hookData;
    hookWork->DrawCallback = DrawCallback;
}

void SetSceneModelFlags(s16 sceneModelId, u8 hookFlags)
{
    HU3DMODEL *sceneModel;
    SceneModelHookWork *hookWork;

    sceneModel = &Hu3DData[sceneModelId];
    hookWork = (SceneModelHookWork *)sceneModel->hookData;
    hookWork->hookFlags = hookFlags;
}

void DrawSceneModelCallback(HU3DMODEL *model, F32 (*modelMatrix)[4])
{
    Mtx modelMatrixCopy;
    ROMtx reorderedModelMatrix;
    Mtx particleRotationMatrix;
    Mtx combinedParticleMatrix;
    Vec transformedQuad[4];
    Vec scaledQuad[4];
    Vec rotatedQuad[4];
    Vec *particleVertexOutput;
    s16 textureFormat;
    HuVec2f *particleTexcoord;
    s16 particleIndex;
    SceneModelHookWork *hookWork;
    SceneParticleWork *particleWork;
    SceneModelDrawCallback DrawCallback;

    hookWork = (SceneModelHookWork *)model->hookData;
    GXLoadPosMtxImm(modelMatrix, 0);
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    textureFormat = hookWork->particleAnimation->bmp->dataFmt & 0xF;
    if ((textureFormat == 7) || (textureFormat == 8)) {
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
    }
    else {
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
    }
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    if ((model->attr & 2) != 0) {
        GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    }
    else {
        GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    }
    HuSprTexLoad(hookWork->particleAnimation, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZCompLoc(0);
    switch (hookWork->hookFlags) {
        case 0:
            GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
            break;
        case 1:
            GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
            break;
        case 2:
            GXSetBlendMode(GX_BM_BLEND, GX_BL_ZERO, GX_BL_INVDSTCLR, GX_LO_NOOP);
            break;
    }
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, hookWork->particleVertices, 12);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, hookWork->particleColors, 4);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_TEX0, hookWork->particleTexcoords, 8);
    CopyMatrix(modelMatrix, modelMatrixCopy);
    PSMTXReorder(modelMatrixCopy, reorderedModelMatrix);
    if (((u8)omPauseChk() == 0) && (hookWork->DrawCallback)) {
        DrawCallback = hookWork->DrawCallback;
        DrawCallback(model, hookWork, modelMatrix);
    }
    particleWork = hookWork->particleWork;
    particleVertexOutput = hookWork->particleVertices;
    particleTexcoord = hookWork->particleTexcoords;
    PSMTXROMultVecArray(reorderedModelMatrix, &hookWork->quadTopLeft, &transformedQuad[0], 4);

    for (particleIndex = 0; particleIndex < hookWork->particleCapacity; particleIndex++, particleWork++, particleTexcoord += 4) {
        if (particleWork->isActive == 0) {
            particleVertexOutput->x = particleVertexOutput->y = particleVertexOutput->z = 0.0f;
            particleVertexOutput++;
            particleVertexOutput->x = particleVertexOutput->y = particleVertexOutput->z = 0.0f;
            particleVertexOutput++;
            particleVertexOutput->x = particleVertexOutput->y = particleVertexOutput->z = 0.0f;
            particleVertexOutput++;
            particleVertexOutput->x = particleVertexOutput->y = particleVertexOutput->z = 0.0f;
            particleVertexOutput++;
        }
        else {
            if (particleWork->particleRotation == 0.0f) {
                InterpolateVector(transformedQuad, scaledQuad, &particleWork->particleScale, 4);
                VECAdd(&scaledQuad[0], &particleWork->particlePosition, particleVertexOutput++);
                VECAdd(&scaledQuad[1], &particleWork->particlePosition, particleVertexOutput++);
                VECAdd(&scaledQuad[2], &particleWork->particlePosition, particleVertexOutput++);
                VECAdd(&scaledQuad[3], &particleWork->particlePosition, particleVertexOutput++);
            }
            else {
                InterpolateVector(&hookWork->quadTopLeft, scaledQuad, &particleWork->particleScale, 4);
                MTXRotRad(particleRotationMatrix, 0x5A, 0.017453292f * particleWork->particleRotation);
                MTXConcat(modelMatrixCopy, particleRotationMatrix, combinedParticleMatrix);
                MTXMultVecArray(combinedParticleMatrix, scaledQuad, rotatedQuad, 4);
                VECAdd(&rotatedQuad[0], &particleWork->particlePosition, particleVertexOutput++);
                VECAdd(&rotatedQuad[1], &particleWork->particlePosition, particleVertexOutput++);
                VECAdd(&rotatedQuad[2], &particleWork->particlePosition, particleVertexOutput++);
                VECAdd(&rotatedQuad[3], &particleWork->particlePosition, particleVertexOutput++);
            }
            if (particleWork->particleLifetime != -1) {
                InterpolateScreenRectangle(particleTexcoord, hookWork->particleTexcoordFrames, particleWork->particleLifetime, hookWork->particleFrameWidth, hookWork->particleFrameHeight);
            }
        }
    }
    DCFlushRangeNoSync(hookWork->particleVertices, hookWork->particleCapacity * 0xC * 4);
    DCFlushRangeNoSync(hookWork->particleTexcoords, hookWork->particleCapacity * 8 * 4);
    PPCSync();
    GXCallDisplayList(hookWork->displayList, hookWork->displayListCapacity);
}

void SetScreenRectangle(HuVec2f *rectanglePositions, s16 rectangleCount, s16 columns, F32 rectangleWidth, F32 rectangleHeight)
{
    s16 rectangleIndex;
    s16 columnIndex;
    s16 rowIndex;

    for (rectangleIndex = 0; rectangleIndex < rectangleCount; rectangleIndex++, rectanglePositions++) {
        columnIndex = (rectangleIndex % columns);
        rowIndex = (rectangleIndex / columns);
        rectanglePositions->x = (columnIndex * rectangleWidth);
        rectanglePositions->y = (rowIndex * rectangleHeight);
    }
}

void InterpolateScreenRectangle(HuVec2f *outputPositions, HuVec2f *framePositions, s16 frameIndex, F32 frameWidth, F32 frameHeight)
{
    outputPositions[0].x = framePositions[frameIndex].x;
    outputPositions[0].y = framePositions[frameIndex].y;
    outputPositions[1].x = (frameWidth + framePositions[frameIndex].x);
    outputPositions[1].y = framePositions[frameIndex].y;
    outputPositions[2].x = (frameWidth + framePositions[frameIndex].x);
    outputPositions[2].y = (frameHeight + framePositions[frameIndex].y);
    outputPositions[3].x = framePositions[frameIndex].x;
    outputPositions[3].y = (frameHeight + framePositions[frameIndex].y);
}

void InterpolateVector(Vec *sourceVectors, Vec *outputVectors, Vec *scale, s16 vectorCount)
{
    s16 componentIndex;

    for (componentIndex = 0; componentIndex < vectorCount; componentIndex++, sourceVectors++, outputVectors++) {
        outputVectors->x = sourceVectors->x * scale->x;
        outputVectors->y = sourceVectors->y * scale->y;
        outputVectors->z = sourceVectors->z * scale->z;
    }
}
