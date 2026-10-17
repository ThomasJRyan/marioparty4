#include "REL/m440Dll.h"

#include "ext_math.h"
#include "game/audio.h"
#include "game/frand.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include "math.h"

#include "string.h"
#include "version.h"

#undef REFRESH_RATE_F
#if VERSION_NTSC
#define REFRESH_RATE_F 60.0f
#else
#define REFRESH_RATE_F 49.998f
#endif

// bss
OMOBJ *sequenceObject;
OMOBJ *effectObject;
MeshWork *meshWorkArray;
s16 meshCount;
Mtx highlightMatrix;
s16 highlightTextureMapId;
void *capturedFrameTexture;
u32 capturedFrameTextureSize;
s16 effectModelIds[10];
s16 directionalLightId;
u8 modelVisibility;
u8 highlightActive;
u8 wipeComplete;
s16 introSequenceState;
s16 cameraTransitionState;
s16 cameraTransitionFrame;
s8 debugState;
s8 debugLightEnabled;
s8 exitRequested;

// data
f32 playerOrbitAngles[5] = { 90.0f, 80.0f, 100.0f, 70.0f, 110.0f };
s16 rewardModelVisibility[6] = { 1, 1, 1, 1, 1 };
Vec debugLightTarget = { 0.0f, 0.0f, 0.0f };
Vec debugLightPosition = { 800.0f, 1300.0f, 1000.0f };
Vec zeroVector = { 0.0f, 0.0f, 0.0f };
LightParameters directionalLightParameters = { 0xFF, 0xFF, 0xFF, 0xFF, 10.0f, 45.0f, 0.0f };
Vec shadowPosition = { 1300.0f, 2500.0f, 1300.0f };
Vec shadowUpVector = { 0.0f, 1.0f, 0.0f };
Vec shadowTarget = { 0.0f, 0.0f, -500.0f };
CameraPreset cameraPresets[3] = {
    { 1450.0f, { 0.0f, 452.0f, 0.0f }, { 11.0f, -23.0f, 0.0f } },
    { 1700.0f, { 0.0f, 315.0f, 0.0f }, { -2.0f, 0.0f, 0.0f } },
    { 1040.0f, { 200.0f, 21.0f, 0.0f }, { -6.0f, 0.0f, 0.0f } },
};

void ObjectSetup(void)
{
    Vec lightTargetPosition;
    HU3DLIGHT *createdLight;
    HUPROCESS *objectManager;

    Hu3DLightAllKill();
    directionalLightId = Hu3DGLightCreateV(&debugLightPosition, &zeroVector, &directionalLightParameters.color);
    Hu3DGLightInfinitytSet(directionalLightId);
    createdLight = &Hu3DGlobalLight[directionalLightId];
    createdLight->type |= 0x8000;
    lightTargetPosition.x = lightTargetPosition.y = lightTargetPosition.z = 0.0f;
    Hu3DGLightPosAimSetV(directionalLightId, &debugLightPosition, &lightTargetPosition);
    Hu3DShadowCreate(45.0f, 20.0f, 10000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowPosSet(&shadowPosition, &shadowUpVector, &shadowTarget);
    HuAudSndGrpSet(0x42);
    objectManager = omInitObjMan(0x32, 0x2000);
    omGameSysInit(objectManager);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 41.5f, 5.0f, 5000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutView);
    CRot.x = cameraPresets[0].rot.x;
    CRot.y = cameraPresets[0].rot.y;
    CRot.z = cameraPresets[0].rot.z;
    Center.x = cameraPresets[0].center.x;
    Center.y = cameraPresets[0].center.y;
    Center.z = cameraPresets[0].center.z;
    CZoom = cameraPresets[0].zoom;
    omAddObjEx(objectManager, 0x3E8, 0, 0, -1, CheckExitRequest);
    sequenceObject = omAddObjEx(objectManager, 10, 9, 0, -1, InitializeIntroSequence);
    effectObject = omAddObjEx(objectManager, 50, 9, 9, -1, InitializeEffectObject);
    Hu3DBGColorSet(0, 0, 0);
    CreatePlayerObjects(objectManager);
}

void CheckExitRequest(OMOBJ *object)
{
    if ((omSysExitReq != 0) || (exitRequested != 0)) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = &FinishExitSequence;
    }
}

void FinishExitSequence(OMOBJ *object)
{
    if ((WipeStatGet() == 0) && (MGSeqDoneCheck() != 0)) {
        HuMemDirectFree(meshWorkArray);
        DestroyEffectModel(effectModelIds[0]);
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

s32 TransitionCameraToFirstPreset(void)
{
    f32 cameraBlendProgress;
    CameraPreset *cameraPreset;
    s32 transitionComplete;

    cameraPreset = &cameraPresets[0];
    transitionComplete = 0;
    cameraTransitionFrame++;
    cameraBlendProgress = cameraTransitionFrame / (2 * REFRESH_RATE_F);
    if (cameraBlendProgress > 1.0f) {
        cameraTransitionFrame = 0;
        cameraBlendProgress = 1.0f;
        transitionComplete = 1;
    }

    cameraBlendProgress = sind(90.0f * cameraBlendProgress) * sind(90.0f * cameraBlendProgress);
    CZoom = InterpolateValue(cameraPreset[0].zoom, cameraPreset[1].zoom, cameraBlendProgress);
    Center.x = InterpolateValue(cameraPreset[0].center.x, cameraPreset[1].center.x, cameraBlendProgress);
    Center.y = InterpolateValue(cameraPreset[0].center.y, cameraPreset[1].center.y, cameraBlendProgress);
    Center.z = InterpolateValue(cameraPreset[0].center.z, cameraPreset[1].center.z, cameraBlendProgress);
    CRot.x = InterpolateValue(cameraPreset[0].rot.x, cameraPreset[1].rot.x, cameraBlendProgress);
    CRot.y = InterpolateValue(cameraPreset[0].rot.y, cameraPreset[1].rot.y, cameraBlendProgress);
    CRot.z = InterpolateValue(cameraPreset[0].rot.z, cameraPreset[1].rot.z, cameraBlendProgress);
    return transitionComplete;
}

s32 TransitionCameraToSecondPreset(void)
{
    f32 cameraBlendProgress;
    CameraPreset *cameraPreset;
    s32 transitionComplete;

    cameraPreset = &cameraPresets[1];
    transitionComplete = 0;
    cameraTransitionFrame++;
    cameraBlendProgress = cameraTransitionFrame / (2 * REFRESH_RATE_F);
    if (cameraBlendProgress > 1.0f) {
        cameraTransitionFrame = 0;
        cameraBlendProgress = 1.0f;
        transitionComplete = 1;
    }

    cameraBlendProgress = (sind(90.0f * cameraBlendProgress) * sind(90.0f * cameraBlendProgress));
    CZoom = InterpolateValue(cameraPreset[0].zoom, cameraPreset[1].zoom, cameraBlendProgress);
    Center.x = InterpolateValue(cameraPreset[0].center.x, cameraPreset[1].center.x, cameraBlendProgress);
    Center.y = InterpolateValue(cameraPreset[0].center.y, cameraPreset[1].center.y, cameraBlendProgress);
    Center.z = InterpolateValue(cameraPreset[0].center.z, cameraPreset[1].center.z, cameraBlendProgress);
    CRot.x = InterpolateValue(cameraPreset[0].rot.x, cameraPreset[1].rot.x, cameraBlendProgress);
    CRot.y = InterpolateValue(cameraPreset[0].rot.y, cameraPreset[1].rot.y, cameraBlendProgress);
    CRot.z = InterpolateValue(cameraPreset[0].rot.z, cameraPreset[1].rot.z, cameraBlendProgress);
    return transitionComplete;
}

void InitializeIntroSequence(OMOBJ *object)
{
    s16 modelIndex;
    SequenceWork *sequenceWork;
    ANIMDATA *anim;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(SequenceWork), HU_MEMNUM_OVL);
    sequenceWork = (SequenceWork *)object->data;
    sequenceWork->stateFlags = 1;
    sequenceWork->sequenceId = -1;
    sequenceWork->sequenceHandle = -1;
    sequenceWork->timer = 5 * REFRESH_RATE;
    sequenceWork->shakeFrame = 0;
    sequenceWork->audioHandle = -1;

    for (modelIndex = 1; modelIndex < 4; modelIndex++) {
        effectModelIds[modelIndex] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M440, 0x0A));
        Hu3DModelAttrSet(effectModelIds[modelIndex], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(effectModelIds[modelIndex], HU3D_MOTATTR_PAUSE);
        Hu3DModelLayerSet(effectModelIds[modelIndex], 7);
        Hu3DModelScaleSet(effectModelIds[modelIndex], 3.0f, 3.0f, 3.0f);
    }

    anim = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M440, 0x0B), HU_MEMNUM_OVL));
    effectModelIds[0] = CreateSpriteParticleModel(anim, 0x40, 50.0f, 0x40, 0x40);
    SetSpriteUpdateCallback(effectModelIds[0], UpdateParticleModel);
    Hu3DModelLayerSet(effectModelIds[0], 7);
    object->objFunc = &UpdateIntroSequence;
}

void UpdateIntroSequence(OMOBJ *object)
{
    f32 shakeAngle;
    SequenceWork *sequenceWork;

    sequenceWork = object->data;
    switch (GetSequenceState(7)) {
        case 1:
            if (wipeComplete == 0) {
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
                wipeComplete = 1;
            }
            if (WipeStatGet() != 0)
                break;
            SetSequenceState(7, 2);
            break;
        case 2:
            if (UpdateIntroCamera(object) != 0) {
                SetSequenceState(7, 3);
            }
            break;
        case 3:
            if (sequenceWork->sequenceHandle != -1) {
                MGSeqParamSet(sequenceWork->sequenceHandle, 1, ((sequenceWork->timer + REFRESH_RATE - 1) / REFRESH_RATE));
            }
            if ((sequenceWork->audioHandle < 0) && ((MGSeqStatGet(sequenceWork->sequenceId) & 0x10) != 0)) {
                sequenceWork->audioHandle = HuAudSeqPlay(0x49);
            }
            if (debugState == 0) {
                if (sequenceWork->shakeFrame != 0) {
                    sequenceWork->shakeFrame++;
                    shakeAngle = (((rand8() << 8) | rand8()) % 361);
                    Center.x = (cameraPresets[1].center.x + (10.0 * sind(shakeAngle)));
                    Center.y = (cameraPresets[1].center.y + (10.0 * cosd(shakeAngle)));
                    if (sequenceWork->shakeFrame > (2 * REFRESH_RATE / 3)) {
                        sequenceWork->shakeFrame = 0;
                        return;
                    }
                }
                else {
                    Center.x = cameraPresets[1].center.x;
                    Center.y = cameraPresets[1].center.y;
                    Center.z = cameraPresets[1].center.z;
                    return;
                }
            }
            break;
        case 4:
            if (UpdateCameraForPlayerSequence(object) != 0) {
                SetSequenceState(7, 5);
                return;
            }
            break;
        case 5:
            exitRequested = 1;
            break;
    }
}

u8 UpdateIntroCamera(OMOBJ *object)
{
    f32 cameraBlendProgress;
    CameraPreset *cameraPresets;
    u8 transitionComplete;

    switch (cameraTransitionState) {
        case 0:
            SetEffectFlags(7, 1);
            SetEffectFlags(0x18, 8);
            HuAudFXPlay(0x70E);
            cameraTransitionState++;
            break;
        case 1:
            if (++introSequenceState > (s16)(0.5f * REFRESH_RATE_F)) {
                StartPlayerCelebrations();
                introSequenceState = 0;
                cameraTransitionState++;
            }
            break;
        case 2:
            cameraPresets = cameraPresets;
            transitionComplete = 0;
            cameraTransitionFrame++;
            cameraBlendProgress = cameraTransitionFrame / (2 * REFRESH_RATE_F);
            if (cameraBlendProgress > 1.0f) {
                cameraTransitionFrame = 0;
                cameraBlendProgress = 1.0f;
                transitionComplete = 1;
            }
            cameraBlendProgress = (sind(90.0f * cameraBlendProgress) * sind(90.0f * cameraBlendProgress));
            CZoom = InterpolateValue(cameraPresets[0].zoom, cameraPresets[1].zoom, cameraBlendProgress);
            Center.x = InterpolateValue(cameraPresets[0].center.x, cameraPresets[1].center.x, cameraBlendProgress);
            Center.y = InterpolateValue(cameraPresets[0].center.y, cameraPresets[1].center.y, cameraBlendProgress);
            Center.z = InterpolateValue(cameraPresets[0].center.z, cameraPresets[1].center.z, cameraBlendProgress);
            CRot.x = InterpolateValue(cameraPresets[0].rot.x, cameraPresets[1].rot.x, cameraBlendProgress);
            CRot.y = InterpolateValue(cameraPresets[0].rot.y, cameraPresets[1].rot.y, cameraBlendProgress);
            CRot.z = InterpolateValue(cameraPresets[0].rot.z, cameraPresets[1].rot.z, cameraBlendProgress);
            if (transitionComplete != 0) {
                introSequenceState = 0;
                cameraTransitionState = 0;
                return 1;
            }
            break;
    }
    return 0;
}

u8 UpdateCameraForPlayerSequence(OMOBJ *object)
{
    SequenceWork *sequenceWork;
    f32 cameraBlendProgress;
    f32 transitionProgress;
    s16 activePlayerIndex;
    u8 transitionComplete;
    PlayerSequenceWork *playerWork;
    CameraPreset *cameraPresets;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    activePlayerIndex = getFirstActivePlayer();
    playerWork = (PlayerSequenceWork *)playerObjects[activePlayerIndex]->data;
    switch (cameraTransitionState) {
        case 0:
            if (introSequenceState == 0) {
                Hu3DMotionShiftSet(playerObjects[activePlayerIndex]->mdlId[0], playerObjects[activePlayerIndex]->mtnId[1], 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
                playerWork->rotationTarget = playerWork->rotationStart;
            }
            introSequenceState++;
            transitionProgress = introSequenceState / (0.5f * REFRESH_RATE_F);
            if (transitionProgress >= 1.0f) {
                transitionProgress = 1.0f;
                if (highlightActive == 0) {
                    highlightActive = 1;
                    Hu3DMotionShiftSet(playerObjects[activePlayerIndex]->mdlId[0], playerObjects[activePlayerIndex]->mtnId[0], 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
                }
            }
            playerWork->rotationStart = (playerWork->rotationTarget + (transitionProgress * (360.0f - playerWork->rotationTarget)));
            cameraPresets = &cameraPresets[1]; // could be transition_camera_to_second_preset
            transitionComplete = 0;
            cameraTransitionFrame++;
            cameraBlendProgress = cameraTransitionFrame / (2 * REFRESH_RATE_F);
            if (cameraBlendProgress > 1.0f) {
                cameraTransitionFrame = 0;
                cameraBlendProgress = 1.0f;
                transitionComplete = 1;
            }
            cameraBlendProgress = (sind(90.0f * cameraBlendProgress) * sind(90.0f * cameraBlendProgress));
            CZoom = InterpolateValue(cameraPresets[0].zoom, cameraPresets[1].zoom, cameraBlendProgress);
            Center.x = InterpolateValue(cameraPresets[0].center.x, cameraPresets[1].center.x, cameraBlendProgress);
            Center.y = InterpolateValue(cameraPresets[0].center.y, cameraPresets[1].center.y, cameraBlendProgress);
            Center.z = InterpolateValue(cameraPresets[0].center.z, cameraPresets[1].center.z, cameraBlendProgress);
            CRot.x = InterpolateValue(cameraPresets[0].rot.x, cameraPresets[1].rot.x, cameraBlendProgress);
            CRot.y = InterpolateValue(cameraPresets[0].rot.y, cameraPresets[1].rot.y, cameraBlendProgress);
            CRot.z = InterpolateValue(cameraPresets[0].rot.z, cameraPresets[1].rot.z, cameraBlendProgress);
            if (transitionComplete != 0) {
                introSequenceState = 0;
                cameraTransitionState++;
            }
            break;
        case 1:
            Hu3DMotionShiftSet(playerObjects[activePlayerIndex]->mdlId[0], playerObjects[activePlayerIndex]->mtnId[5], 0.0f, 7.0f, HU3D_MOTATTR_NONE);
            HuAudSStreamPlay(1);
            cameraTransitionState++;
            introSequenceState = 0;
            break;
        case 2:
            if (++introSequenceState > (3.5f * REFRESH_RATE)) {
                cameraTransitionState = 0;
                return 1;
            }
            break;
    }
    return 0;
}

void StartPlayerReveal(void)
{
    SequenceWork *sequenceWork;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    sequenceWork->timer--;
}

void StartPlayerCelebration(void)
{
    SequenceWork *sequenceWork;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    if (sequenceWork->sequenceHandle != -1) {
        MGSeqParamSet(sequenceWork->sequenceHandle, 2, -1);
        sequenceWork->sequenceHandle = -1;
        sequenceWork->timer = 0;
    }
}

void StartPlayerExit(void)
{
    SequenceWork *sequenceWork;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    if (sequenceWork->sequenceHandle == -1) {
        sequenceWork->timer = 5 * REFRESH_RATE;
        sequenceWork->sequenceHandle = MGSeqCreate(1, 5, -1, -1);
    }
}

s16 GetCurrentSequencePlayer(void)
{
    SequenceWork *sequenceWork;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    return sequenceWork->timer;
}

s16 GetNextSequencePlayer(void)
{
    SequenceWork *sequenceWork;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    return sequenceWork->sequenceHandle;
}

void ResetPlayerSequenceState(void)
{
    SequenceWork *sequenceWork;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    if (sequenceWork->sequenceId == -1) {
        sequenceWork->sequenceId = MGSeqCreate(3, 0);
        MGSeqPosSet(sequenceWork->sequenceId, 320.0f, 240.0f);
    }
}

u8 IsPlayerSequenceComplete(void)
{
    SequenceWork *sequenceWork;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    if (sequenceWork->sequenceId != -1) {
        return MGSeqStatGet(sequenceWork->sequenceId);
    }
    else {
        return 0;
    }
}

void StartRewardSequence(void)
{
    SequenceWork *sequenceWork;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    sequenceWork->sequenceId = MGSeqCreate(3, 1);
    MGSeqPosSet(sequenceWork->sequenceId, 320.0f, 240.0f);
    HuAudSeqFadeOut(sequenceWork->audioHandle, 0x64);
}

u8 IsRewardSequenceReady(void)
{
    SequenceWork *sequenceWork;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    if ((debugState != 0) || (debugLightEnabled != 0)) {
        return 0;
    }
    else {
        return 1;
    }
}

void PositionRewardModels(void)
{
    f32 modelX;
    f32 modelYOffset;
    f32 modelZ;
    f32 modelRadius;
    f32 startingAngle;
    s16 modelIndex;

    startingAngle = (((rand8() << 8) | rand8()) % 361);

    for (modelIndex = 1; modelIndex < 4; modelIndex++, startingAngle += 120.0f) {
        modelRadius = 0.01f * ((((rand8() << 8) | rand8()) % 51) + 50);
        modelX = (modelRadius * (200.0 * sind(startingAngle)));
        modelYOffset = (modelRadius * (200.0 * cosd(startingAngle)));
        modelZ = 0.0f;
        Hu3DModelPosSet(effectModelIds[modelIndex], modelX, 300.0f + modelYOffset, modelZ);
        modelRadius = 0.1f * ((((rand8() << 8) | rand8()) % 11) + 25);
        Hu3DModelScaleSet(effectModelIds[modelIndex], modelRadius, modelRadius, modelRadius);
        Hu3DModelAttrReset(effectModelIds[modelIndex], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(effectModelIds[modelIndex], HU3D_MOTATTR_PAUSE);
    }
}

void HideRewardModels(void)
{
    s16 modelIndex;

    for (modelIndex = 1; modelIndex < 4; modelIndex++) {
        Hu3DMotionTimeSet(effectModelIds[modelIndex], 0.0f);
        Hu3DModelAttrSet(effectModelIds[modelIndex], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(effectModelIds[modelIndex], HU3D_MOTATTR_PAUSE);
    }
}

void CreateRewardParticle(f32 originX, f32 originY, f32 originZ, s16 particleCount, f32 launchAngle, s16 spriteCount)
{
    Vec particlePosition;
    Vec particleScale;
    GXColor particleColor;
    f32 directionSin;
    f32 directionCos;
    f32 particleAngle;
    s16 particleIndex;
    ParticleWork *particleWork;

    for (particleIndex = 0; particleIndex < particleCount; particleIndex++) {
        particleAngle = launchAngle + ((((rand8() << 8) | rand8()) % 11) - 5);
        directionSin = sind(particleAngle);
        directionCos = cosd(particleAngle);
        particlePosition.x = originX + ((((rand8() << 8) | rand8()) % 21) - 10);
        particlePosition.y = originY + ((((rand8() << 8) | rand8()) % 21) - 10);
        particlePosition.z = originZ;
        particleScale.x = particleScale.y = particleScale.z = 1.0f;
        particleColor.r = particleColor.g = particleColor.b = 0xFF;
        particleColor.a = 0x80;
        particleWork = CreateParticleWork(spriteCount, &particlePosition, &particleScale, 0.0f, &particleColor);
        if (!particleWork)
            break;

        particleWork->animationFrame = 0;
        particleWork->animationLength = ((rand8() << 8) | rand8()) % 2 + 2;
        particleWork->velocity.x = (directionSin * (0.1f * ((((rand8() << 8) | rand8()) % 41) + 0x28)));
        particleWork->velocity.y = (directionCos * (0.1f * ((((rand8() << 8) | rand8()) % 61) + 0x3C)));
        particleWork->velocity.z = 0.0f;
        particleWork->horizontalDrag = (0.01f * particleWork->velocity.x);
        particleWork->verticalAcceleration = 0.1f;
        particleWork->scale = 1.5f;
        particleWork->scaleStep = ((0.1f * ((((rand8() << 8) | rand8()) % 7) + 2)) / (particleWork->animationLength * 0xE));
        particleWork->alpha = particleColor.a;
        particleWork->alphaStep = (particleWork->alpha / (particleWork->animationLength * 0xE));
    }
}

void UpdateParticleModel(HU3DMODEL *data, SpriteModelWork *spriteWork, Mtx modelMatrix)
{
    ParticleWork *particleWork;
    GXColor *particleColor;
    s16 particleIndex;

    particleWork = spriteWork->particleData;
    particleColor = spriteWork->particleColors;

    for (particleIndex = 0; particleIndex < spriteWork->particleCount; particleIndex++, particleWork++, particleColor++) {
        if (particleWork->isActive != 0) {
            particleWork->position.x += particleWork->velocity.x;
            particleWork->position.y += particleWork->velocity.y;
            particleWork->position.z += particleWork->velocity.z;
            particleWork->velocity.x -= particleWork->horizontalDrag;
            particleWork->velocity.y += particleWork->verticalAcceleration;
            particleWork->horizontalDrag = (0.001f * particleWork->velocity.x);
            particleWork->verticalAcceleration *= 1.05f;
            particleWork->scale += particleWork->scaleStep;
            particleWork->renderScale.x = particleWork->renderScale.y = particleWork->renderScale.z = particleWork->scale;
            particleWork->alpha -= particleWork->alphaStep;
            particleColor->a = particleWork->alpha > 255.0f ? 255 : (u8)particleWork->alpha;
            particleWork->animationFrame++;
            if (particleWork->animationFrame >= particleWork->animationLength) {
                particleWork->animationFrame = 0;
                particleWork->textureFrame++;
            }
            if (particleWork->textureFrame >= spriteWork->textureFrameCount - 2) {
                particleWork->isActive = 0;
            }
        }
    }
}

u16 GetSequenceState(u16 stateMask)
{
    SequenceWork *sequenceWork;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    if (!sequenceWork) {
        return 0;
    }
    return sequenceWork->stateFlags & stateMask;
}

void SetSequenceState(u16 stateMask, u16 newState)
{
    SequenceWork *sequenceWork;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    sequenceWork->stateFlags = (sequenceWork->stateFlags & ~stateMask);
    sequenceWork->stateFlags = (sequenceWork->stateFlags | newState);
}

void InitializeEffectObject(OMOBJ *object)
{
    HU3DMODEL *modelData;
    HSFOBJECT *rootObject;
    f32 modelX;
    f32 modelRotation;
    f32 modelZ;
    s16 shadowModelId;
    s16 modelIndex;
    EffectWork *effectWork;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, 0x34, HU_MEMNUM_OVL);
    effectWork = object->data;
    object->stat |= 0x100;
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M440, 0x08));
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M440, 0x09));
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M440, 0x00));
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M440, 0x02));
    object->mdlId[8] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M440, 0x03));
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M440, 0x04));
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M440, 0x05));
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M440, 0x06));
    object->mdlId[7] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M440, 0x07));
    shadowModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M440, 0x01));
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_HILITE);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrSet(shadowModelId, HU3D_ATTR_DISPOFF);

    for (modelIndex = 0; modelIndex < 8; modelIndex++) {
        Hu3DModelLayerSet(object->mdlId[modelIndex], 1);
    }
    Hu3DModelPosSet(object->mdlId[2], 0.0f, 5000.0f, 0.0f);
    Hu3DModelPosSet(object->mdlId[3], 0.0f, 5000.0f, 0.0f);
    Hu3DModelShadowSet(object->mdlId[2]);
    Hu3DModelShadowSet(object->mdlId[3]);
    Hu3DModelShadowMapSet(object->mdlId[0]);

    for (modelIndex = 0; modelIndex < 5; modelIndex++) {
        modelX = 450.0 * cosd(playerOrbitAngles[modelIndex]);
        modelZ = 450.0 * sind(playerOrbitAngles[modelIndex]);
        Hu3DModelPosSet(object->mdlId[modelIndex + 4], modelX, 0.0f, modelZ);
        modelRotation = 90.0f - playerOrbitAngles[modelIndex];
        Hu3DModelRotSet(object->mdlId[modelIndex + 4], 0.0f, modelRotation, 0.0f);
        Hu3DMotionSpeedSet(object->mdlId[modelIndex + 4], 0.0f);
    }
    effectWork->stateFlags = 0;
    effectWork->modelCount = 5;
    modelData = &Hu3DData[shadowModelId];
    rootObject = modelData->hsf->root;
    meshCount = rootObject->mesh.childrenCount;
    meshWorkArray = HuMemDirectMalloc(HEAP_MODEL, meshCount * sizeof(MeshWork));
    OSReport("Koopa Object Count %d\n", modelData->hsf->objectNum);

    for (modelIndex = 0; modelIndex < meshCount; modelIndex++) {
        meshWorkArray[modelIndex].vertexGroupCount = 0;
    }

    for (modelIndex = 0; modelIndex < meshCount; modelIndex++) {
        meshWorkArray[modelIndex].meshIndex = modelIndex;
        ProcessMeshObject(shadowModelId, rootObject->mesh.children[modelIndex], &meshWorkArray[modelIndex], 2);
        if (meshWorkArray[modelIndex].vertexGroupCount != 0) {
            Hu3DModelAttrSet(meshWorkArray[modelIndex].modelId, HU3D_ATTR_DISPOFF);
            Hu3DModelPosSet(meshWorkArray[modelIndex].modelId, 0.0f, 250.0f, 0.0f);
            meshWorkArray[modelIndex].drawCallback = &UpdateMeshVertexNormals;
        }
    }
    object->objFunc = &UpdateEffectObject;
}

void UpdateEffectObject(OMOBJ *object)
{
    InitializeEffectAnimation(object);
    InitializeSecondaryEffect(object);
    InitializeHighlightObject(object);
}

void InitializeEffectAnimation(OMOBJ *object)
{
    EffectWork *objectWork;

    objectWork = (EffectWork *)object->data;

    switch (GetEffectFlags(0x20)) {
        case 0x20:
            UpdateEffectAnimation(object);
    }
}

s16 effectSequenceState[2] = { 0, 0 };
s16 effectSequenceFrames[2] = { 0, 0 };
s16 secondaryEffectState[2] = { 0, 0 };
s16 secondaryEffectFrames[2] = { 0, 0 };
s32 sceneMotionTimes[4] = { 20, 60, 100, 140 };
s16 sceneEffectState[2] = { 0, 0 };
s16 sceneEffectFrames[2] = { 0, 0 };
s16 triangleVertexOrder[9] = { 0, 3, 1, 0, 2, 3, 1, 3, 2 };

void UpdateEffectAnimation(OMOBJ *object)
{
    f32 animationProgress;
    CameraPreset *objectWork;

    objectWork = (CameraPreset *)object->data;
    switch (effectSequenceState[0]) {
        case 0:
            effectSequenceFrames[0]++;
            animationProgress = effectSequenceFrames[0] / (6 * REFRESH_RATE_F);
            if (animationProgress > 1.0f) {
                animationProgress = 1.0f;
                Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
                effectSequenceFrames[0] = 0;
                effectSequenceState[0] = 0;
                SetEffectFlags(0x20, 0);
                return;
            }
            animationProgress = sind(90.0f * animationProgress);
            objectWork->center.y = (250.0f + (1750.0f * animationProgress));
    }
    Hu3DModelPosSet(object->mdlId[3], objectWork->center.x, objectWork->center.y, objectWork->center.z);
    Hu3DModelRotSet(object->mdlId[3], objectWork->rot.x, objectWork->rot.y, objectWork->rot.z);
}

void InitializeSecondaryEffect(OMOBJ *object)
{
    EffectWork *objectWork;

    objectWork = (EffectWork *)object->data;

    switch (GetEffectFlags(7)) {
        case 1:
            UpdateSecondaryEffect(object);
            break;
        case 3:
            UpdateEffectParticles(object);
            break;
        case 4:
        case 5:
            InitializeCameraEffect(object);
    }
}

void UpdateSecondaryEffect(OMOBJ *object)
{
    f32 animationProgress;
    CameraPreset *objectWork;

    objectWork = (CameraPreset *)object->data;
    switch (secondaryEffectState[0]) {
        case 0x0:
            objectWork->center.x = objectWork->center.z = 0.0f;
            objectWork->center.y = 2000.0f;
            objectWork->rot.x = objectWork->rot.y = objectWork->rot.z = 0.0f;
            Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(object->mdlId[3], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(object->mdlId[3], HU3D_MOTATTR_PAUSE);
            Hu3DMotionTimeSet(object->mdlId[3], 0.0f);
            secondaryEffectState[0]++;
            secondaryEffectFrames[0] = 0;
            HuAudFXPlay(0x709);
            HuAudFXPlay(0x711);
            break;
        case 0x1:
            secondaryEffectFrames[0]++;
            animationProgress = secondaryEffectFrames[0] / (2 * REFRESH_RATE_F);
            if (animationProgress > 1.0f) {
                animationProgress = 1.0f;
                secondaryEffectFrames[0] = 0;
                secondaryEffectFrames[1] = REFRESH_RATE / 4;
                secondaryEffectState[1] = (secondaryEffectState[0] + 1);
                secondaryEffectState[0] = 0x63;
            }
            if ((11 * REFRESH_RATE_F / 6) == secondaryEffectFrames[0]) {
                HuAudFXPlay(0x70A);
            }
            animationProgress = sind(90.0f * animationProgress);
            objectWork->center.y = (2000.0f + (-1750.0f * animationProgress));
            break;
        case 0x2:
            if (secondaryEffectFrames[0] == 0) {
                HuAudFXPlay(0x713);
            }
            secondaryEffectFrames[0]++;
            animationProgress = secondaryEffectFrames[0] / REFRESH_RATE_F;
            if (animationProgress > 1.0f) {
                animationProgress = 1.0f;
                secondaryEffectFrames[0] = 0;
                secondaryEffectFrames[1] = REFRESH_RATE / 4;
                secondaryEffectState[1] = secondaryEffectState[0] + 1;
                secondaryEffectState[0] = 0x63;
                HuAudFXPlay(0x714);
            }
            objectWork->rot.y = (360.0f * animationProgress);
            break;
        case 0x3:
            Hu3DModelAttrReset(object->mdlId[3], HU3D_MOTATTR_PAUSE);
            Hu3DMotionSpeedSet(object->mdlId[3], 2.0f);
            if (secondaryEffectFrames[0] == 0) {
                HuAudFXPlay(0x711);
            }
            if (++secondaryEffectFrames[0] > (0.2 * REFRESH_RATE)) {
                if (GetEffectFlags(0x40) != 0) {
                    SetEffectFlags(0x40, 0);
                    ChoosePlayerRewards();
                }
                SetEffectFlags(0x20, 0x20);
                SetEffectFlags(7, 2);
                secondaryEffectFrames[0] = 0;
                secondaryEffectState[0] = 0;
            }
            break;
        case 0x4:
            secondaryEffectFrames[0]++;
            animationProgress = secondaryEffectFrames[0] / (6 * REFRESH_RATE_F);
            if (animationProgress > 1.0f) {
                animationProgress = 1.0f;
                Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
                secondaryEffectFrames[0] = 0;
                secondaryEffectState[0] = 0;
                SetEffectFlags(7, 2);
                return;
            }
            animationProgress = sind(90.0f * animationProgress);
            objectWork->center.y = (250.0f + (1750.0f * animationProgress));
            break;
        case 0x63:
            if (++secondaryEffectFrames[0] > secondaryEffectFrames[1]) {
                secondaryEffectFrames[0] = 0;
                secondaryEffectState[0] = secondaryEffectState[1];
            }
            break;
    }
    Hu3DModelPosSet(object->mdlId[3], objectWork->center.x, objectWork->center.y, objectWork->center.z);
    Hu3DModelRotSet(object->mdlId[3], objectWork->rot.x, objectWork->rot.y, objectWork->rot.z);
    Hu3DModelPosSet(object->mdlId[2], objectWork->center.x, objectWork->center.y, objectWork->center.z);
    Hu3DModelRotSet(object->mdlId[2], objectWork->rot.x, objectWork->rot.y, objectWork->rot.z);
    return;
}

void UpdateEffectParticles(OMOBJ *object)
{
    EffectWork *objectWork;

    objectWork = (EffectWork *)object->data;
    switch (secondaryEffectState[0]) {
        case 0:
            CreateRewardParticle(30.0f, 500.0f, 200.0f, 12, 135.0f, effectModelIds[0]);
            CreateRewardParticle(-30.0f, 500.0f, 200.0f, 12, 225.0f, effectModelIds[0]);
            SetEffectFlags(0x100, 0x100);
            SetEffectFlags(7, 2);
            HuAudFXPlay(0x710);
    }
}

void InitializeCameraEffect(OMOBJ *object)
{
    CameraPreset *objectWork;
    f32 modelZ;
    f32 modelY;
    f32 modelX;
    f32 animationEndTime;
    f32 startingAngle;
    f32 modelRadius;
    SequenceWork *sequenceWork;
    OMOBJ **playerList;
    s16 playerIndex;
    s16 modelIndex;
    s32 modelCount;
    SequenceWork *playerWork;
    EffectWork *effectWork;
    s16 effectIndex;

    sequenceWork = (SequenceWork *)sequenceObject->data;
    objectWork = (CameraPreset *)object->data;
    switch (secondaryEffectState[0]) {
        case 0:
            Hu3DModelAttrReset(object->mdlId[2], HU3D_MOTATTR_PAUSE);
            secondaryEffectState[0]++;
            return;
        case 1:
            if (++secondaryEffectFrames[0] == 0x1E) {
                SetEffectFlags(0x100, 0x100);
            }
            else if (secondaryEffectFrames[0] == 0x32) {
                RestorePlayerIdleMotions();
            }
            animationEndTime = Hu3DMotionMaxTimeGet(object->mdlId[2]);
            if (Hu3DMotionTimeGet(object->mdlId[2]) >= animationEndTime) {
                Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
                for (modelIndex = 0; modelIndex < meshCount; modelIndex++) {
                    Hu3DModelAttrReset(meshWorkArray[modelIndex].modelId, HU3D_ATTR_DISPOFF);
                    UpdateMeshVertexNormals(&meshWorkArray[modelIndex]);
                }
                secondaryEffectFrames[0] = 0;
                secondaryEffectState[0]++;
                HuAudFXPlay(0x70C);
            }

            for (modelIndex = 0; modelIndex < 4U; modelIndex++) {
                if (secondaryEffectFrames[0] == sceneMotionTimes[modelIndex] * 2) {
                    HuAudFXPlay(0x70D);
                }
            }
            return;
        case 2:
            startingAngle = (((rand8() << 8) | rand8()) % 361);

            for (playerIndex = 1; playerIndex < 4; playerIndex++, startingAngle += 120.0f) {
                modelRadius = 0.01f * ((((rand8() << 8) | rand8()) % 51) + 0x32);
                modelX = (modelRadius * (200.0 * sind(startingAngle)));
                modelY = (modelRadius * (200.0 * cosd(startingAngle)));
                modelZ = 0.0f;
                Hu3DModelPosSet(effectModelIds[playerIndex], modelX, 300.0f + modelY, modelZ);
                modelRadius = 0.1f * ((((rand8() << 8) | rand8()) % 11) + 0x19);
                Hu3DModelScaleSet(effectModelIds[playerIndex], modelRadius, modelRadius, modelRadius);
                Hu3DModelAttrReset(effectModelIds[playerIndex], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(effectModelIds[playerIndex], HU3D_MOTATTR_PAUSE);
            }
            UpdateHighlightEffect(object);
            SetEffectFlags(7, 5);
            sequenceWork->shakeFrame = 1;
            secondaryEffectState[0]++;
            playerList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
            for (modelIndex = 0; modelIndex < 4; modelIndex++) {
                if (getPlayerSlot(modelIndex) >= 0) {
                    effectWork = (EffectWork *)playerList[getPlayerSlot(modelIndex)]->data;
                    switch (effectWork->stateFlags & 0xF) {
                        case 6:
                            omVibrate(effectWork->ownerPlayer, 0x30, 0xC, 0);
                            break;
                        case 8:
                            omVibrate(effectWork->ownerPlayer, 0x30, 4, 2);
                            break;
                    }
                }
            }
            return;
        case 3:
            if (meshWorkArray[0].drawLayer == 0) {
                for (effectIndex = 1; effectIndex < 4; effectIndex++) {
                    Hu3DMotionTimeSet(effectModelIds[effectIndex], 0.0f);
                    Hu3DModelAttrSet(effectModelIds[effectIndex], HU3D_ATTR_DISPOFF);
                    Hu3DModelAttrSet(effectModelIds[effectIndex], HU3D_MOTATTR_PAUSE);
                }
                Hu3DModelAttrSet(object->mdlId[2], HU3D_MOTATTR_PAUSE);
                Hu3DMotionTimeSet(object->mdlId[2], 0.0f);
                for (modelIndex = 0; modelIndex < meshCount; modelIndex++) {
                    Hu3DModelAttrSet(meshWorkArray[modelIndex].modelId, HU3D_ATTR_DISPOFF);
                }
                InitializeHighlightEffect(object);
                if (remainingRewardCount >= 0) {
                    SetPlayersIdleMotions();
                    SetEffectFlags(0x18, 0x18);
                    SetEffectFlags(0x40, 0x40);
                    SetEffectFlags(7, 1);
                    HuAudFXPlay(0x70E);
                }
                else {
                    SetEffectFlags(7, 2);
                    playerWork = sequenceObject->data;
                    playerWork->stateFlags &= 0xFFFFFFF8;
                    playerWork->stateFlags |= 4;
                }
                secondaryEffectState[0] = 0;
            }
    }
}

void InitializeHighlightEffect(OMOBJ *object)
{
    s16 meshIndex;

    for (meshIndex = 0; meshIndex < meshCount; meshIndex++) {
        InitializeMeshVertices(&meshWorkArray[meshIndex]);
    }
}

void UpdateHighlightEffect(OMOBJ *object)
{
    Vec impulseOrigin;
    s16 meshIndex;

    impulseOrigin.x = 0.0f;
    impulseOrigin.y = 250.0f;
    impulseOrigin.z = 0.0f;

    for (meshIndex = 0; meshIndex < meshCount; meshIndex++) {
        TransformMeshVertices(&meshWorkArray[meshIndex], &impulseOrigin, 1500.0f);
    }
}

void InitializeHighlightObject(OMOBJ *object)
{
    void *objectWork;
    s16 eventMask;

    objectWork = object->data;
    switch (GetEffectFlags(0x18)) {
        case 8:
            UpdateHighlightObject(object);
            return;
        case 16:
            InitializeSceneModel(object);
            return;
        case 24:
            UpdateSceneModel(object);
    }
}

void UpdateHighlightObject(OMOBJ *object)
{
    f32 animationProgress;
    s16 modelIndex;
    s16 modelIndex;
    s32 selectedModel;
    EffectWork *objectWork;

    objectWork = (EffectWork *)object->data;
    switch (sceneEffectState[0]) {
        case 0x0:
            SetEffectParameter(-1, 0.0f);

            for (modelIndex = 0; modelIndex < 5; modelIndex++) {
                Hu3DMotionTimeSet(object->mdlId[modelIndex + 4], 0.0f);
                rewardModelVisibility[modelIndex] = 0;
            }
            objectWork->selectedModelIndex = (((rand8() << 8) | rand8()) % (s16)objectWork->modelCount);
            objectWork->modelHeight = 0.0f;
            sceneEffectFrames[0] = 0;
            sceneEffectState[0]++;
            break;
        case 0x1:
            sceneEffectFrames[0]++;
            animationProgress = sceneEffectFrames[0] / REFRESH_RATE_F;
            if (animationProgress > 1.0f) {
                animationProgress = 1.0f;
                sceneEffectFrames[0] = 0;
                sceneEffectState[0] = 0;
                SetEffectFlags(0x18, 0x10);
                HuAudFXPlay(0x70F);
            }
            animationProgress = sind(90.0f * animationProgress);
            objectWork->modelHeight = (150.0f * animationProgress);
            break;
        case 0x63:
            if (++sceneEffectFrames[0] > sceneEffectFrames[1]) {
                sceneEffectFrames[0] = 0;
                sceneEffectState[0] = sceneEffectState[1];
            }
            break;
    }

    for (modelIndex = 0; modelIndex < objectWork->modelCount; modelIndex++) {
        Hu3DData[object->mdlId[modelIndex + 4]].pos.y = objectWork->modelHeight;
        rewardModelVisibility[modelIndex] = 1;
    }
    Hu3DData[object->mdlId[1]].pos.y = objectWork->modelHeight;
}

void InitializeSceneModel(OMOBJ *object)
{
    HU3DMODEL *modelData;
    s16 modelIndex;
    EffectWork *effectWork;
    f32 motionDuration;

    effectWork = (EffectWork *)object->data;
    modelIndex = effectWork->sceneModelIndex;
    if (modelIndex != -1) {
        modelData = &Hu3DData[object->mdlId[modelIndex + 4]];
        if (-1.0f == effectWork->motionProgress) {
            if (0.0f != modelData->motWork.time) {
                modelData->motWork.time -= 4.0f;
                if (modelData->motWork.time < 0.0f) {
                    modelData->motWork.time = 0.0f;
                }
            }
        }
        else {
            motionDuration = Hu3DMotionMaxTimeGet(object->mdlId[modelIndex + 4]);
            modelData->motWork.time = motionDuration * effectWork->motionProgress;
        }
    }
}

void UpdateSceneModel(OMOBJ *object)
{
    f32 animationProgress;
    s16 modelIndex;
    EffectWork *effectWork;

    effectWork = (EffectWork *)object->data;
    switch (sceneEffectState[0]) {
        case 0:
            sceneEffectFrames[0]++;
            animationProgress = sceneEffectFrames[0] / REFRESH_RATE_F;
            if (animationProgress > 1.0f) {
                animationProgress = 1.0f;
                sceneEffectFrames[0] = 0;
                sceneEffectState[0]++;
            }
            animationProgress = sind(90.0f * animationProgress);
            effectWork->modelHeight = (150.0f + (-150.0f * animationProgress));
            break;
        case 1:
            sceneEffectFrames[0]++;
            animationProgress = (sceneEffectFrames[0] / (0.5f * REFRESH_RATE_F));
            if (animationProgress >= 1.0f) {
                sceneEffectFrames[0] = 0;
                sceneEffectState[0] = 0;
                if (GetEffectFlags(0x40) != 0) {
                    effectWork->modelCount--;
                }
                SetEffectFlags(0x18, 8);
            }
            break;
    }

    for (modelIndex = 0; modelIndex < effectWork->modelCount; modelIndex++) {
        Hu3DData[object->mdlId[modelIndex + 4]].pos.y = effectWork->modelHeight;
        rewardModelVisibility[modelIndex] = 1;
    }
    Hu3DData[object->mdlId[1]].pos.y = effectWork->modelHeight;
}

void SetEffectParameter(s16 modelIndex, f32 motionProgress)
{
    EffectWork *effectWork;

    effectWork = (EffectWork *)effectObject->data;
    effectWork->sceneModelIndex = modelIndex;
    effectWork->motionProgress = motionProgress;
}

s16 GetEffectParameter(void)
{
    EffectWork *effectWork;

    effectWork = (EffectWork *)effectObject->data;
    return effectWork->modelCount;
}

s16 GetEffectModelId(s16 modelIndex)
{
    EffectWork *effectWork;

    effectWork = (EffectWork *)effectObject->data;
    if (modelIndex == -1) {
        return effectWork->selectedModelIndex;
    }
    if (modelIndex == effectWork->selectedModelIndex) {
        return 1;
    }
    return 0;
}

u16 GetEffectFlags(u16 stateMask)
{
    EffectWork *effectWork;

    effectWork = (EffectWork *)effectObject->data;
    if (!effectWork) {
        return 0;
    }
    return effectWork->stateFlags & stateMask;
}

void SetEffectFlags(u16 stateMask, u16 newState)
{
    EffectWork *effectWork;

    effectWork = (EffectWork *)effectObject->data;
    effectWork->stateFlags &= ~stateMask;
    effectWork->stateFlags |= newState;
}

void InitializeMeshVertices(MeshWork *meshWork)
{
    s16 vertexIndex;
    TriangleMeshWork *vertexWork;

    vertexWork = meshWork->faceData;
    if (meshWork->isVisible != 0) {
        for (vertexIndex = 0; vertexIndex < meshWork->isVisible; vertexIndex++, vertexWork++) {
            vertexWork->translation.x = vertexWork->translation.y = vertexWork->translation.z = 0.0f;
            vertexWork->rotation.x = vertexWork->rotation.y = vertexWork->rotation.z = 0.0f;
            vertexWork->velocity.x = vertexWork->velocity.y = vertexWork->velocity.z = 0.0f;
            vertexWork->angularVelocity.x = vertexWork->angularVelocity.y = vertexWork->angularVelocity.z = 0.0f;
            vertexWork->acceleration.y = 0.0f;
        }
        meshWork->flags &= ~4;
        meshWork->drawLayer = 0xFF;
        meshWork->updateTimer = 0;
    }
}

void TransformMeshVertices(MeshWork *meshWork, Vec *lightPosition, f32 lightRadius)
{
    Vec vertexDirection;
    HU3DMODEL *modelData;
    f32 distanceFalloff;
    f32 xJitterRange;
    f32 yJitterRange;
    f32 zJitterRange;
    s32 vertexIndex;
    TriangleMeshWork *vertexWork;

    vertexWork = meshWork->faceData;
    if (meshWork->isVisible != 0) {
        modelData = &Hu3DData[meshWork->modelId];
        meshWork->flags |= 4;

        for (vertexIndex = 0; vertexIndex < meshWork->isVisible; vertexIndex++, vertexWork++) {
            vertexDirection.x = (vertexWork->center.x + modelData->pos.x) - lightPosition->x;
            vertexDirection.y = (vertexWork->center.y + modelData->pos.y) - lightPosition->y;
            vertexDirection.z = (vertexWork->center.z + modelData->pos.z) - lightPosition->z;
            distanceFalloff = sqrtf((vertexDirection.z * vertexDirection.z) + ((vertexDirection.x * vertexDirection.x) + (vertexDirection.y * vertexDirection.y)));
            vertexDirection.x /= distanceFalloff;
            vertexDirection.y /= distanceFalloff;
            vertexDirection.z /= distanceFalloff;

            distanceFalloff = (1.0f - (distanceFalloff / lightRadius));
            if (distanceFalloff < 0.0f) {
                distanceFalloff = 0.0f;
            }
            xJitterRange = (ABS(vertexDirection.x) < 0.1f) ? 100.0f : 100.0f * vertexDirection.x;
            yJitterRange = (ABS(vertexDirection.y) < 0.1f) ? 100.0f : 100.0f * vertexDirection.y;
            zJitterRange = (ABS(vertexDirection.z) < 0.1f) ? 100.0f : 100.0f * vertexDirection.z;
            vertexDirection.x += (0.001f * (-xJitterRange + (((rand8() << 8) | rand8()) % (s16)(1.0f + (xJitterRange - -xJitterRange)))));
            vertexDirection.y += (0.001f * (-yJitterRange + (((rand8() << 8) | rand8()) % (s16)(1.0f + (yJitterRange - -yJitterRange)))));
            vertexDirection.z += (0.001f * (-zJitterRange + (((rand8() << 8) | rand8()) % (s16)(1.0f + (zJitterRange - -zJitterRange)))));
            vertexWork->velocity.x = (vertexDirection.x * ((((rand8() << 8) | rand8()) % 21) + 0x50));
            vertexWork->velocity.y = (vertexDirection.y * ((((rand8() << 8) | rand8()) % 16) + 0x1E));
            vertexWork->velocity.z = (vertexDirection.z * ((((rand8() << 8) | rand8()) % 21) + 0x50));
            vertexWork->acceleration.y = 1.47f;
            vertexWork->angularVelocity.x = (120.0f * (-vertexDirection.z * ABS(vertexDirection.y)));
            vertexWork->angularVelocity.z = (120.0f * (-vertexDirection.x * ABS(vertexDirection.y)));
        }
    }
}

void UpdateMeshVertexNormals(MeshWork *meshWork)
{
    Mtx vertexTransform;
    Vec transformedVertex;
    s16 vertexIndex;
    Vec *sourceVertices;
    s32 vertexGroupIndex;
    s32 vertexSlot;
    TriangleMeshWork *vertexWork;

    vertexWork = meshWork->faceData;
    sourceVertices = meshWork->sourceVertices;

    for (vertexGroupIndex = 0; vertexGroupIndex < meshWork->isVisible; vertexGroupIndex++, vertexWork++) {
        vertexWork->velocity.y = (vertexWork->velocity.y - vertexWork->acceleration.y);
        vertexWork->translation.x = (vertexWork->translation.x + vertexWork->velocity.x);
        vertexWork->translation.y = (vertexWork->translation.y + vertexWork->velocity.y);
        vertexWork->translation.z = (vertexWork->translation.z + vertexWork->velocity.z);
        vertexWork->rotation.x = (vertexWork->rotation.x + vertexWork->angularVelocity.x);
        vertexWork->rotation.y = (vertexWork->rotation.y + vertexWork->angularVelocity.y);
        vertexWork->rotation.z = (vertexWork->rotation.z + vertexWork->angularVelocity.z);
        if ((vertexWork->translation.x < -500.0f) || (vertexWork->translation.x > 500.0f)) {
            vertexWork->velocity.x = (0.5f * -vertexWork->velocity.x);
            vertexWork->angularVelocity.x = (0.5f * -vertexWork->angularVelocity.x);
        }
        if (vertexWork->translation.z < -500.0f) {
            vertexWork->velocity.z = (0.5f * -vertexWork->velocity.z);
            vertexWork->angularVelocity.z = (0.5f * -vertexWork->angularVelocity.z);
        }
        if ((vertexWork->center.y + vertexWork->translation.y) < -250.0f) {
            if (vertexWork->velocity.y > -2.0f) {
                vertexWork->translation.y = (-250.0f - vertexWork->center.y);
                vertexWork->velocity.x = 0.0f;
                vertexWork->velocity.y = 0.0f;
                vertexWork->velocity.z = 0.0f;
                vertexWork->angularVelocity.x = 0.0f;
                vertexWork->angularVelocity.z = 0.0f;
            }
            else {
                vertexWork->velocity.x *= 0.5f;
                vertexWork->velocity.y = -(0.3f * vertexWork->velocity.y);
                vertexWork->velocity.z *= 0.5f;
                vertexWork->angularVelocity.x *= 0.5f;
                vertexWork->angularVelocity.z *= 0.5f;
            }
        }
        MTXScale(vertexTransform, vertexWork->scale.x, vertexWork->scale.y, vertexWork->scale.z);
        mtxTransCat(vertexTransform, -vertexWork->center.x, -vertexWork->center.y, -vertexWork->center.z);
        mtxRotCat(vertexTransform, vertexWork->rotation.x, vertexWork->rotation.y, vertexWork->rotation.z);
        mtxTransCat(vertexTransform, vertexWork->translation.x + vertexWork->center.x, vertexWork->translation.y + vertexWork->center.y, vertexWork->translation.z + vertexWork->center.z);

        for (vertexSlot = 0; vertexSlot < 3; vertexSlot++) {
            if ((meshWork->flags & 1) != 0) {
                vertexIndex = vertexWork->vertexIndices[vertexSlot].positionIndex;
            }
            else {
                vertexIndex = vertexWork->sourceVertexIndices[vertexSlot];
            }
            MTXMultVec(vertexTransform, &sourceVertices[vertexIndex], &transformedVertex);
            meshWork->transformedVertices[vertexIndex].x = transformedVertex.x;
            meshWork->transformedVertices[vertexIndex].y = transformedVertex.y;
            meshWork->transformedVertices[vertexIndex].z = transformedVertex.z;
        }
        if ((meshWork->flags & 2) != 0) {
            vertexIndex = vertexWork->additionalVertexIndex;
            MTXMultVec(vertexTransform, &sourceVertices[vertexIndex], &transformedVertex);
            meshWork->transformedVertices[vertexIndex].x = transformedVertex.x;
            meshWork->transformedVertices[vertexIndex].y = transformedVertex.y;
            meshWork->transformedVertices[vertexIndex].z = transformedVertex.z;
        }
    }
    if ((meshWork->flags & 4) != 0) {
        if (++meshWork->updateTimer > (4 * REFRESH_RATE / 3)) {
            meshWork->drawLayer = ((meshWork->drawLayer - 10) < 0) ? 0 : meshWork->drawLayer - 10;
        }
    }
    DCFlushRangeNoSync(meshWork->transformedVertices, meshWork->vertexCount * 0xC);
}

void ProcessMeshObject(s16 modelId, HSFOBJECT *sourceObject, MeshWork *meshWork, u16 meshFlags)
{
    Mtx modelTransform;
    Vec triangleVertices[3];
    Vec transformedVertex;
    Vec triangleCenter;
    HU3DMODEL *sourceModel;
    GXColor vertexColor = { 0xFF, 0xFF, 0xFF, 0xFF };
    Vec *displayListBuffer;
    Vec *displayListStart;
    s16 highestVertexIndex;
    HuVec2f *textureCoordinates;
    s16 generatedVertexIndex;
    s16 sourceVertexIndex;
    HSFTRANSFORM *meshTransform;
    s16 generatedIndex;
    HU3DMODEL *hookModel;
    s16 vertexSlot;
    s16 meshIndex;

    sourceModel = &Hu3DData[modelId];
    meshWork->vertexGroupCount = 0;
    if (sourceObject->type == 2) {
        meshIndex = Hu3DHookFuncCreate(&DrawModelMeshes);
        meshWork->modelId = meshIndex;
        Hu3DModelLayerSet(meshIndex, 1);
        hookModel = &Hu3DData[meshIndex];
        hookModel->hookData = (HU3DPARTICLE *)meshWork;
        hookModel->ambR = hookModel->ambG = hookModel->ambB = 1.0f;
        meshWork->flags = meshFlags;
        meshWork->material = &sourceObject->mesh.material[((s16 *)(sourceObject->mesh.face->data))[1] & 0xFFF];
        meshWork->attribute = sourceObject->mesh.attribute;
        meshWork->drawCallback = NULL;
        meshWork->drawLayer = 0xFF;
        meshWork->updateTimer = 0;
        meshWork->vertexNormals = HuMemDirectMallocNum(HEAP_MODEL, sourceObject->mesh.vertex->count * 0xC, hookModel->mallocNo);
        CalculateMeshFaceNormals(meshWork, sourceObject);
        ProcessMeshFaces(meshWork, sourceObject);
        meshWork->transformedVertices = HuMemDirectMallocNum(HEAP_MODEL, meshWork->vertexCount * 0xC, hookModel->mallocNo);
        meshWork->sourceVertices = HuMemDirectMallocNum(HEAP_MODEL, meshWork->vertexCount * 0xC, hookModel->mallocNo);
        meshWork->projectedVertices = HuMemDirectMallocNum(HEAP_MODEL, meshWork->vertexCount * 0xC, hookModel->mallocNo);
        if (meshWork->material->attrNum != 0) {
            meshWork->textureCoordinates = HuMemDirectMallocNum(HEAP_MODEL, sourceObject->mesh.st->count * 8, hookModel->mallocNo);
            textureCoordinates = sourceObject->mesh.st->data;
        }
        else {
            meshWork->textureCoordinates = NULL;
            textureCoordinates = NULL;
        }
        meshTransform = &sourceObject->mesh.base;
        MTXScale(modelTransform, meshTransform->scale.x, meshTransform->scale.y, meshTransform->scale.z);
        mtxRotCat(modelTransform, meshTransform->rot.x, meshTransform->rot.y, meshTransform->rot.z);
        mtxTransCat(modelTransform, meshTransform->pos.x, meshTransform->pos.y, meshTransform->pos.z);

        for (meshIndex = 0; meshIndex < meshWork->vertexGroupCount; meshIndex++) {
            triangleCenter.x = triangleCenter.y = triangleCenter.z = 0.0f;

            for (vertexSlot = 0; vertexSlot < 3; vertexSlot++) {
                if ((meshWork->flags & 1) != 0) {
                    sourceVertexIndex = meshWork->faceData[meshIndex].vertexIndices[vertexSlot].positionIndex;
                    generatedIndex = sourceVertexIndex;
                }
                else {
                    generatedIndex = meshWork->faceData[meshIndex].sourceVertexIndices[vertexSlot];
                    sourceVertexIndex = meshWork->faceData[meshIndex].vertexIndices[vertexSlot].positionIndex;
                }
                MTXMultVec(modelTransform, &((Vec *)(sourceObject->mesh.vertex->data))[sourceVertexIndex], &transformedVertex);
                meshWork->transformedVertices[generatedIndex] = transformedVertex;
                triangleVertices[vertexSlot] = meshWork->transformedVertices[generatedIndex];
                meshWork->projectedVertices[generatedIndex] = meshWork->vertexNormals[sourceVertexIndex];
                triangleCenter.x += transformedVertex.x;
                triangleCenter.y += transformedVertex.y;
                triangleCenter.z += transformedVertex.z;
                if (meshWork->material->attrNum != 0) {
                    sourceVertexIndex = meshWork->faceData[meshIndex].vertexIndices[vertexSlot].textureIndex;
                    generatedIndex = sourceVertexIndex;
                    meshWork->textureCoordinates[generatedIndex].x = textureCoordinates[sourceVertexIndex].x;
                    meshWork->textureCoordinates[generatedIndex].y = textureCoordinates[sourceVertexIndex].y;
                }
            }
            CalculateTriangleNormal(&triangleVertices[0], &triangleVertices[1], &triangleVertices[2], meshWork->faceData[meshIndex].faceProperties);
            if ((meshWork->flags & 2) != 0) {
                generatedIndex = meshWork->faceData[meshIndex].additionalVertexIndex;
                CalculateVertexTransform(meshWork, &meshWork->transformedVertices[generatedIndex], meshIndex, triangleCenter);
                triangleCenter.x += meshWork->transformedVertices[generatedIndex].x;
                triangleCenter.y += meshWork->transformedVertices[generatedIndex].y;
                triangleCenter.z += meshWork->transformedVertices[generatedIndex].z;
            }
            CalculateTriangleProjection(meshWork, &meshWork->faceData[meshIndex], &triangleCenter);
        }
        memcpy(meshWork->sourceVertices, meshWork->transformedVertices, meshWork->vertexCount * 0xC);
        DCFlushRangeNoSync(meshWork->transformedVertices, meshWork->vertexCount * 0xC);
        DCFlushRangeNoSync(meshWork->projectedVertices, meshWork->vertexCount * 0xC);
        if (meshWork->material->attrNum != 0) {
            DCFlushRangeNoSync(meshWork->textureCoordinates, sourceObject->mesh.st->count * 8);
        }
        highestVertexIndex = 0;
        displayListStart = displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, 0x20000, hookModel->mallocNo);
        GXBeginDisplayList(displayListStart, 0x20000);
        if ((meshWork->flags & 2) != 0) {
            GXBegin(GX_TRIANGLES, GX_VTXFMT0, (meshWork->vertexGroupCount * 0xC));

            for (meshIndex = 0; meshIndex < meshWork->vertexGroupCount; meshIndex++) {

                for (vertexSlot = 0; vertexSlot < 3; vertexSlot++) {
                    if ((meshWork->flags & 1) != 0) {
                        generatedIndex = meshWork->faceData[meshIndex].vertexIndices[vertexSlot].positionIndex;
                    }
                    else {
                        generatedIndex = meshWork->faceData[meshIndex].sourceVertexIndices[vertexSlot];
                    }
                    GXPosition1x16(generatedIndex);
                    GXNormal1x16(generatedIndex);
                    if (meshWork->material->attrNum != 0) {
                        GXTexCoord1x16(meshWork->faceData[meshIndex].vertexIndices[vertexSlot].textureIndex);
                    }
                }

                for (generatedVertexIndex = 0; generatedVertexIndex < 9; generatedVertexIndex++) {
                    if (highestVertexIndex < meshWork->faceData[meshIndex].triangleIndices[generatedVertexIndex].positionIndex) {
                        highestVertexIndex = meshWork->faceData[meshIndex].triangleIndices[generatedVertexIndex].positionIndex;
                    }
                    GXPosition1x16(meshWork->faceData[meshIndex].triangleIndices[generatedVertexIndex].positionIndex);
                    GXNormal1x16(meshWork->faceData[meshIndex].triangleIndices[generatedVertexIndex].positionIndex);
                    if (meshWork->material->attrNum != 0) {
                        GXTexCoord1x16(meshWork->faceData[meshIndex].triangleIndices[generatedVertexIndex].textureIndex);
                    }
                }
            }
        }
        else {
            GXBegin(GX_TRIANGLES, GX_VTXFMT0, (meshWork->vertexGroupCount * 3));
            for (meshIndex = 0; meshIndex < meshWork->vertexGroupCount; meshIndex++) {
                for (vertexSlot = 0; vertexSlot < 3; vertexSlot++) {
                    if ((meshWork->flags & 1) != 0) {
                        generatedIndex = meshWork->faceData[meshIndex].vertexIndices[vertexSlot].positionIndex;
                    }
                    else {
                        generatedIndex = meshWork->faceData[meshIndex].sourceVertexIndices[vertexSlot];
                    }
                    GXPosition1x16(generatedIndex);
                    GXNormal1x16(generatedIndex);
                    if (meshWork->material->attrNum != 0) {
                        GXTexCoord1x16(meshWork->faceData[meshIndex].vertexIndices[vertexSlot].textureIndex);
                    }
                }
            }
        }
        meshWork->meshFlags = GXEndDisplayList();
        DCFlushRangeNoSync(displayListBuffer, meshWork->meshFlags);
        meshWork->meshData = HuMemDirectMallocNum(HEAP_MODEL, meshWork->meshFlags, hookModel->mallocNo);
        memcpy(meshWork->meshData, displayListBuffer, meshWork->meshFlags);
        DCFlushRangeNoSync(meshWork->meshData, meshWork->meshFlags);
        HuMemDirectFree(displayListBuffer);
    }
}

void CalculateMeshFaceNormals(MeshWork *meshWork, HSFOBJECT *sourceObject)
{
    Vec faceVertices[3];
    f32 faceNormal[5];
    f32 normalLength;
    HSFBUFFER *faceBuffer;
    s16 vertexIndex;
    s16 faceVertexIndex;
    HSFFACE *faceData;

    faceBuffer = sourceObject->mesh.face;

    for (vertexIndex = 0; vertexIndex < sourceObject->mesh.vertex->count; vertexIndex++) {
        meshWork->vertexNormals[vertexIndex].x = 0.0f;
        meshWork->vertexNormals[vertexIndex].y = 0.0f;
        meshWork->vertexNormals[vertexIndex].z = 0.0f;
    }
    faceData = (HSFFACE *)faceBuffer->data;

    for (vertexIndex = 0; vertexIndex < faceBuffer->count; vertexIndex++, faceData++) {
        faceVertices[0] = ((Vec *)(sourceObject->mesh.vertex->data))[faceData->indices[0][0]];
        faceVertices[1] = ((Vec *)(sourceObject->mesh.vertex->data))[faceData->indices[1][0]];
        faceVertices[2] = ((Vec *)(sourceObject->mesh.vertex->data))[faceData->indices[2][0]];
        CalculateTriangleNormal(&faceVertices[0], &faceVertices[1], &faceVertices[2], faceNormal);
        faceNormal[0] = -faceNormal[0];
        faceNormal[1] = -faceNormal[1];
        faceNormal[2] = -faceNormal[2];
        switch (faceData->type & 7) {
            case 2:
                for (faceVertexIndex = 0; faceVertexIndex < 3; faceVertexIndex++) {
                    meshWork->vertexNormals[faceData->indices[faceVertexIndex][0]].x += faceNormal[0];
                    meshWork->vertexNormals[faceData->indices[faceVertexIndex][0]].y += faceNormal[1];
                    meshWork->vertexNormals[faceData->indices[faceVertexIndex][0]].z += faceNormal[2];
                }
                break;
            case 3:
                for (faceVertexIndex = 0; faceVertexIndex < 4; faceVertexIndex++) {
                    meshWork->vertexNormals[faceData->indices[faceVertexIndex][0]].x += faceNormal[0];
                    meshWork->vertexNormals[faceData->indices[faceVertexIndex][0]].y += faceNormal[1];
                    meshWork->vertexNormals[faceData->indices[faceVertexIndex][0]].z += faceNormal[2];
                }
                break;
            case 4:
                for (faceVertexIndex = 0; faceVertexIndex < 3; faceVertexIndex++) {
                    meshWork->vertexNormals[faceData->strip.indices[faceVertexIndex][0]].x += faceNormal[0];
                    meshWork->vertexNormals[faceData->strip.indices[faceVertexIndex][0]].y += faceNormal[1];
                    meshWork->vertexNormals[faceData->strip.indices[faceVertexIndex][0]].z += faceNormal[2];
                }
                for (faceVertexIndex = 0; faceVertexIndex < faceData->strip.count; faceVertexIndex++) {
                    meshWork->vertexNormals[((s16(*)[4])faceData->strip.data)[faceVertexIndex][0]].x += faceNormal[0];
                    meshWork->vertexNormals[((s16(*)[4])faceData->strip.data)[faceVertexIndex][0]].y += faceNormal[1];
                    meshWork->vertexNormals[((s16(*)[4])faceData->strip.data)[faceVertexIndex][0]].z += faceNormal[2];
                }
                break;
        }
    }

    for (vertexIndex = 0; vertexIndex < sourceObject->mesh.vertex->count; vertexIndex++) {
        faceVertices[0].x = meshWork->vertexNormals[vertexIndex].x;
        faceVertices[0].y = meshWork->vertexNormals[vertexIndex].y;
        faceVertices[0].z = meshWork->vertexNormals[vertexIndex].z;
        normalLength = sqrtf((faceVertices[0].z * faceVertices[0].z) + ((faceVertices[0].x * faceVertices[0].x) + (faceVertices[0].y * faceVertices[0].y)));
        meshWork->vertexNormals[vertexIndex].x /= normalLength;
        meshWork->vertexNormals[vertexIndex].y /= normalLength;
        meshWork->vertexNormals[vertexIndex].z /= normalLength;
    }
}

void ProcessMeshFaces(MeshWork *meshWork, HSFOBJECT *sourceObject)
{
    HSFBUFFER *faceBuffer;
    s32 faceIndex;
    s32 generatedVertexIndex;
    u8 reuseSourceVertices;
    HU3DMODEL *modelData;
    TriangleMeshWork *generatedVertexWork;
    TriangleIndices *stripIndices;
    HSFFACE *faceData;

    modelData = &Hu3DData[meshWork->modelId];
    faceBuffer = sourceObject->mesh.face;
    reuseSourceVertices = ((meshWork->flags & 1) != 0) ? 1 : 0;
    meshWork->isVisible = 0;

    faceIndex = 0;
    faceData = (HSFFACE *)faceBuffer->data;
    for (; faceIndex < faceBuffer->count; faceIndex++, faceData++) {
        switch (faceData->type & 7) {
            case 2:
                meshWork->isVisible += 1;
                break;
            case 3:
                meshWork->isVisible += 2;
                break;
            case 4:
                meshWork->isVisible += faceData->strip.count + 1;
                break;
        }
    }
    meshWork->faceData = HuMemDirectMallocNum(HEAP_MODEL, meshWork->isVisible * 0xD0, modelData->mallocNo);
    generatedVertexWork = meshWork->faceData;
    if (reuseSourceVertices != 0) {
        generatedVertexIndex = sourceObject->mesh.vertex->count;
    }
    else {
        generatedVertexIndex = 0;
    }

    faceIndex = 0;
    faceData = (HSFFACE *)faceBuffer->data;
    for (; faceIndex < faceBuffer->count; faceIndex++, faceData++) {
        switch (faceData->type & 7) {
            case 2:
                generatedVertexWork->vertexIndices[0] = *(TriangleIndices*)&faceData->indices[0];
                generatedVertexWork->vertexIndices[1] = *(TriangleIndices*)&faceData->indices[2];
                generatedVertexWork->vertexIndices[2] = *(TriangleIndices*)&faceData->indices[1];
                if (reuseSourceVertices == 0) {
                    generatedVertexWork->sourceVertexIndices[0] = generatedVertexIndex++;
                    generatedVertexWork->sourceVertexIndices[1] = generatedVertexIndex++;
                    generatedVertexWork->sourceVertexIndices[2] = generatedVertexIndex++;
                }
                generatedVertexWork->additionalVertexIndex = generatedVertexIndex++;
                generatedVertexWork++;
                break;
            case 3:
                generatedVertexWork->vertexIndices[0] = *(TriangleIndices*)&faceData->indices[0];
                generatedVertexWork->vertexIndices[1] = *(TriangleIndices*)&faceData->indices[2];
                generatedVertexWork->vertexIndices[2] = *(TriangleIndices*)&faceData->indices[1];
                if (reuseSourceVertices == 0) {
                    generatedVertexWork->sourceVertexIndices[0] = generatedVertexIndex++;
                    generatedVertexWork->sourceVertexIndices[1] = generatedVertexIndex++;
                    generatedVertexWork->sourceVertexIndices[2] = generatedVertexIndex++;
                }
                generatedVertexWork->additionalVertexIndex = generatedVertexIndex++;
                generatedVertexWork++;
                generatedVertexWork->vertexIndices[0] = *(TriangleIndices*)&faceData->indices[1];
                generatedVertexWork->vertexIndices[1] = *(TriangleIndices*)&faceData->indices[2];
                generatedVertexWork->vertexIndices[2] = *(TriangleIndices*)&faceData->indices[3];
                if (reuseSourceVertices == 0) {
                    generatedVertexWork->sourceVertexIndices[0] = generatedVertexIndex++;
                    generatedVertexWork->sourceVertexIndices[1] = generatedVertexIndex++;
                    generatedVertexWork->sourceVertexIndices[2] = generatedVertexIndex++;
                }
                generatedVertexWork->additionalVertexIndex = generatedVertexIndex++;
                generatedVertexWork++;
                break;
            case 4:
                generatedVertexWork->vertexIndices[0] = *(TriangleIndices*)&faceData->indices[0];
                generatedVertexWork->vertexIndices[1] = *(TriangleIndices*)&faceData->indices[2];
                generatedVertexWork->vertexIndices[2] = *(TriangleIndices*)&faceData->indices[1];
                if (reuseSourceVertices == 0) {
                    generatedVertexWork->sourceVertexIndices[0] = generatedVertexIndex++;
                    generatedVertexWork->sourceVertexIndices[1] = generatedVertexIndex++;
                    generatedVertexWork->sourceVertexIndices[2] = generatedVertexIndex++;
                }
                generatedVertexWork->additionalVertexIndex = generatedVertexIndex++;
                generatedVertexWork++;
                faceIndex = 0;
                stripIndices = (TriangleIndices*)faceData->strip.data;
                for (; faceIndex < faceData->strip.count; faceIndex++) {
                    if (faceIndex == 0) {
                        generatedVertexWork->vertexIndices[0] = generatedVertexWork->vertexIndices[1];
                        generatedVertexWork->vertexIndices[1] = generatedVertexWork->vertexIndices[2];
                        generatedVertexWork->vertexIndices[2] = stripIndices[0];
                    }
                    else if (faceIndex == 1) {
                        generatedVertexWork->vertexIndices[0] = generatedVertexWork->vertexIndices[2];
                        generatedVertexWork->vertexIndices[1] = stripIndices[1];
                        generatedVertexWork->vertexIndices[2] = stripIndices[0];
                    }
                    else {
                        if ((faceIndex % 2) != 0) {
                            generatedVertexWork->vertexIndices[0] = stripIndices[faceIndex - 2];
                            generatedVertexWork->vertexIndices[1] = stripIndices[faceIndex - 0];
                            generatedVertexWork->vertexIndices[2] = stripIndices[faceIndex - 1];
                        }
                        else {
                            generatedVertexWork->vertexIndices[0] = stripIndices[faceIndex - 2];
                            generatedVertexWork->vertexIndices[1] = stripIndices[faceIndex - 1];
                            generatedVertexWork->vertexIndices[2] = stripIndices[faceIndex - 0];
                        }
                    }
                    if (reuseSourceVertices == 0) {
                        generatedVertexWork->sourceVertexIndices[0] = generatedVertexIndex++;
                        generatedVertexWork->sourceVertexIndices[1] = generatedVertexIndex++;
                        generatedVertexWork->sourceVertexIndices[2] = generatedVertexIndex++;
                    }
                    generatedVertexWork->additionalVertexIndex = generatedVertexIndex++;
                    generatedVertexWork++;
                }
                break;
        }
    }
    meshWork->vertexCount = generatedVertexIndex;
}

void CalculateVertexTransform(MeshWork *meshWork, Vec *sourcePosition, s16 vertexIndex, Vec targetPosition)
{
    Vec edgeDifference;
    Vec edgeMidpoint;
    Vec triangleVertices[3];
    Vec surfaceNormal;
    TriangleMeshWork *vertexWork = &meshWork->faceData[vertexIndex];
    s16 edgeVertexPairs[3][2] = { { 0, 1 }, { 0, 2 }, { 1, 2 } };
    f32 meanEdgeDeltaX;
    f32 meanEdgeDeltaY;
    f32 meanEdgeDeltaZ;
    f32 triangleRadius;
    s16 edgeIndex;

    if ((meshWork->flags & 1) != 0) {
        triangleVertices[0] = meshWork->transformedVertices[vertexWork->vertexIndices[0].positionIndex];
        triangleVertices[1] = meshWork->transformedVertices[vertexWork->vertexIndices[1].positionIndex];
        triangleVertices[2] = meshWork->transformedVertices[vertexWork->vertexIndices[2].positionIndex];
    }
    else {
        triangleVertices[0] = meshWork->transformedVertices[vertexWork->sourceVertexIndices[0]];
        triangleVertices[1] = meshWork->transformedVertices[vertexWork->sourceVertexIndices[1]];
        triangleVertices[2] = meshWork->transformedVertices[vertexWork->sourceVertexIndices[2]];
    }
    surfaceNormal.x = vertexWork->faceProperties[0];
    surfaceNormal.y = vertexWork->faceProperties[1];
    surfaceNormal.z = vertexWork->faceProperties[2];
    VECNormalize(&surfaceNormal, &surfaceNormal);
    meanEdgeDeltaX = meanEdgeDeltaY = meanEdgeDeltaZ = 0.0f;

    for (edgeIndex = 0; edgeIndex < 3; edgeIndex++) {
        meanEdgeDeltaX += (triangleVertices[edgeVertexPairs[edgeIndex][0]].x - triangleVertices[edgeVertexPairs[edgeIndex][1]].x < 0.0f) ? -(triangleVertices[edgeVertexPairs[edgeIndex][0]].x - triangleVertices[edgeVertexPairs[edgeIndex][1]].x)
                                                                                : (triangleVertices[edgeVertexPairs[edgeIndex][0]].x - triangleVertices[edgeVertexPairs[edgeIndex][1]].x);

        meanEdgeDeltaY += (triangleVertices[edgeVertexPairs[edgeIndex][0]].y - triangleVertices[edgeVertexPairs[edgeIndex][1]].y < 0.0f) ? -(triangleVertices[edgeVertexPairs[edgeIndex][0]].y - triangleVertices[edgeVertexPairs[edgeIndex][1]].y)
                                                                                : (triangleVertices[edgeVertexPairs[edgeIndex][0]].y - triangleVertices[edgeVertexPairs[edgeIndex][1]].y);

        meanEdgeDeltaZ += (triangleVertices[edgeVertexPairs[edgeIndex][0]].z - triangleVertices[edgeVertexPairs[edgeIndex][1]].z < 0.0f) ? -(triangleVertices[edgeVertexPairs[edgeIndex][0]].z - triangleVertices[edgeVertexPairs[edgeIndex][1]].z)
                                                                                : (triangleVertices[edgeVertexPairs[edgeIndex][0]].z - triangleVertices[edgeVertexPairs[edgeIndex][1]].z);
    }
    meanEdgeDeltaX *= 0.3333f;
    meanEdgeDeltaY *= 0.3333f;
    meanEdgeDeltaZ *= 0.3333f;
    triangleRadius = 0.5f * sqrtf((meanEdgeDeltaZ * meanEdgeDeltaZ) + ((meanEdgeDeltaX * meanEdgeDeltaX) + (meanEdgeDeltaY * meanEdgeDeltaY)));
    triangleVertices[0].x = 0.3333f * targetPosition.x;
    triangleVertices[0].y = 0.3333f * targetPosition.y;
    triangleVertices[0].z = 0.3333f * targetPosition.z;
    sourcePosition->x = (triangleVertices[0].x - (surfaceNormal.x * triangleRadius));
    sourcePosition->y = (triangleVertices[0].y - (surfaceNormal.y * triangleRadius));
    sourcePosition->z = (triangleVertices[0].z - (surfaceNormal.z * triangleRadius));
}

void CalculateTriangleProjection(MeshWork *meshWork, TriangleMeshWork *triangleWork, Vec *surfaceNormal)
{
    Vec triangleVertices[3];
    f32 faceNormal[5];
    s16 positionIndices[4];
    s16 textureIndices[4];
    s16 triangleIndex;
    Vec *vertexData;
    Vec *firstVertex;
    Vec *secondVertex;
    Vec *thirdVertex;

    triangleWork->scale.x = triangleWork->scale.y = triangleWork->scale.z = 1.0f;
    triangleWork->translation.x = triangleWork->translation.y = triangleWork->translation.z = 0.0f;
    triangleWork->rotation.x = triangleWork->rotation.y = triangleWork->rotation.z = 0.0f;
    triangleWork->velocity.x = triangleWork->velocity.y = triangleWork->velocity.z = 0.0f;
    triangleWork->angularVelocity.x = triangleWork->angularVelocity.y = triangleWork->angularVelocity.z = 0.0f;
    triangleWork->acceleration.x = triangleWork->acceleration.y = triangleWork->acceleration.z = 0.0f;

    if ((meshWork->flags & 2) != 0) {
        surfaceNormal->x *= 0.25f;
        surfaceNormal->y *= 0.25f;
        surfaceNormal->z *= 0.25f;
    }
    else {
        surfaceNormal->x /= 3.0f;
        surfaceNormal->y /= 3.0f;
        surfaceNormal->z /= 3.0f;
    }
    triangleWork->center.x = surfaceNormal->x;
    triangleWork->center.y = surfaceNormal->y;
    triangleWork->center.z = surfaceNormal->z;
    if ((meshWork->flags & 2) != 0) {
        if ((meshWork->flags & 1) != 0) {
            positionIndices[0] = triangleWork->vertexIndices[0].positionIndex;
            positionIndices[1] = triangleWork->vertexIndices[1].positionIndex;
            positionIndices[2] = triangleWork->vertexIndices[2].positionIndex;
        }
        else {
            positionIndices[0] = triangleWork->sourceVertexIndices[0];
            positionIndices[1] = triangleWork->sourceVertexIndices[1];
            positionIndices[2] = triangleWork->sourceVertexIndices[2];
        }
        positionIndices[3] = triangleWork->additionalVertexIndex;
        textureIndices[0] = triangleWork->vertexIndices[0].textureIndex;
        textureIndices[1] = triangleWork->vertexIndices[1].textureIndex;
        textureIndices[2] = triangleWork->vertexIndices[2].textureIndex;
        textureIndices[3] = triangleWork->vertexIndices[0].textureIndex;

        for (triangleIndex = 0; triangleIndex < 9; triangleIndex++) {
            triangleWork->triangleIndices[triangleIndex].positionIndex = positionIndices[triangleVertexOrder[triangleIndex]];
            triangleWork->triangleIndices[triangleIndex].textureIndex = textureIndices[triangleVertexOrder[triangleIndex]];
        }
        vertexData = &meshWork->projectedVertices[triangleWork->additionalVertexIndex];
        vertexData->x = vertexData->y = vertexData->z = 0.0f;

        for (triangleIndex = 0; triangleIndex < 3; triangleIndex++) {
            triangleVertices[0] = meshWork->transformedVertices[positionIndices[triangleVertexOrder[triangleIndex]]];
            triangleVertices[1] = meshWork->transformedVertices[positionIndices[triangleVertexOrder[triangleIndex + 1]]];
            triangleVertices[2] = meshWork->transformedVertices[positionIndices[triangleVertexOrder[triangleIndex + 2]]];
            CalculateTriangleNormal(&triangleVertices[0], &triangleVertices[1], &triangleVertices[2], faceNormal);
            vertexData->x += faceNormal[0];
            vertexData->y += faceNormal[1];
            vertexData->z += faceNormal[2];
        }
        vertexData->x *= 0.3333f;
        vertexData->y *= 0.3333f;
        vertexData->z *= 0.3333f;
    }
}

void BindMaterialTexture(HSFBITMAP *bitmap, HSFATTRIBUTE *attribute, s16 textureMapId)
{
    GXTexObj textureObject;
    GXTlutObj paletteObject;
    s32 wrapS;
    s16 textureWidth;
    s16 textureHeight;
    s16 wrapSMode;
    s16 wrapTMode;

    if (!bitmap) {
        OSReport("Error: No Texture\n");
        return;
    }
    textureWidth = bitmap->sizeX;
    textureHeight = bitmap->sizeY;
    wrapSMode = (attribute->wrapS == 1) ? 1 : 0;
    wrapTMode = (attribute->wrapT == 1) ? 1 : 0;
    switch (bitmap->dataFmt) {
        case 6:
            GXInitTexObj(&textureObject, bitmap->data, textureWidth, textureHeight, GX_TF_RGBA8, wrapSMode, wrapTMode, GX_FALSE);
            break;
        case 4:
            GXInitTexObj(&textureObject, bitmap->data, textureWidth, textureHeight, GX_TF_RGB565, wrapSMode, wrapTMode, GX_FALSE);
            break;
        case 5:
            GXInitTexObj(&textureObject, bitmap->data, textureWidth, textureHeight, GX_TF_RGB5A3, wrapSMode, wrapTMode, GX_FALSE);
            break;
        case 9:
            if (bitmap->pixSize < 8) {
                GXInitTlutObj(&paletteObject, bitmap->palData, GX_TL_RGB565, bitmap->palSize);
                GXLoadTlut(&paletteObject, textureMapId);
                GXInitTexObjCI(&textureObject, bitmap->data, textureWidth, textureHeight, GX_TF_C4, wrapSMode, wrapTMode, GX_FALSE, textureMapId);
            }
            else {
                GXInitTlutObj(&paletteObject, bitmap->palData, GX_TL_RGB565, bitmap->palSize);
                GXLoadTlut(&paletteObject, textureMapId);
                GXInitTexObjCI(&textureObject, bitmap->data, textureWidth, textureHeight, GX_TF_C8, wrapSMode, wrapTMode, GX_FALSE, textureMapId);
            }
            break;
        case 10:
            if (bitmap->pixSize < 8) {
                GXInitTlutObj(&paletteObject, bitmap->palData, GX_TL_RGB5A3, bitmap->palSize);
                GXLoadTlut(&paletteObject, textureMapId);
                GXInitTexObjCI(&textureObject, bitmap->data, textureWidth, textureHeight, GX_TF_C4, wrapSMode, wrapTMode, GX_FALSE, textureMapId);
            }
            else {
                GXInitTlutObj(&paletteObject, bitmap->palData, GX_TL_RGB5A3, bitmap->palSize);
                GXLoadTlut(&paletteObject, textureMapId);
                GXInitTexObjCI(&textureObject, bitmap->data, textureWidth, textureHeight, GX_TF_C8, wrapSMode, wrapTMode, GX_FALSE, textureMapId);
            }
            break;
        default:
            OSReport("Error: Texture format\n");
            return;
    }
    GXInitTexObjLOD(&textureObject, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
    GXLoadTexObj(&textureObject, textureMapId);
    return;
}

void DrawModelMeshes(HU3DMODEL *model, Mtx modelMatrix)
{
    Mtx inverseNormalMatrix;
    GXColor channelColor;
    HSFATTRIBUTE *materialAttribute;
    ModelDrawWork *meshWork;
    ModelDrawCallback meshHook;

    meshWork = (ModelDrawWork *)model->hookData;
    if (((u8)omPauseChk() == 0) && (meshWork->drawCallback)) {
        meshHook = meshWork->drawCallback;
        meshHook(meshWork);
    }
    GXLoadPosMtxImm(modelMatrix, 0);
    MTXInvXpose(modelMatrix, inverseNormalMatrix);
    GXLoadNrmMtxImm(inverseNormalMatrix, 0);
    CalculateHighlightMatrix(modelMatrix);
    channelColor.r = (meshWork->material->litColor[0] * model->ambR);
    channelColor.g = (meshWork->material->litColor[1] * model->ambG);
    channelColor.b = (meshWork->material->litColor[2] * model->ambB);
    channelColor.a = 0xFF;
    GXSetChanAmbColor(GX_COLOR0A0, channelColor);
    channelColor.r = meshWork->material->color[0];
    channelColor.g = meshWork->material->color[1];
    channelColor.b = meshWork->material->color[2];
    channelColor.a = 0xFF;
    GXSetChanMatColor(GX_COLOR0A0, channelColor);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, meshWork->meshData, 12);
    GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_NRM, meshWork->vertexData, 12);
    modelVisibility = meshWork->vertexColorIndex;
    if (meshWork->material->attrNum == 0) {
        GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ONE, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetNumTexGens(0);
        GXSetNumChans(0);
    }
    else {
        GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
        GXSetArray(GX_VA_TEX0, meshWork->reserved1C, 8);
        materialAttribute = &meshWork->attribute[meshWork->material->attr[0]];
        BindMaterialTexture(materialAttribute->bitmap, materialAttribute, 0);
        highlightTextureMapId = 1;
        HuSprTexLoad(*hiliteAnim, 0, highlightTextureMapId, GX_CLAMP, GX_CLAMP, GX_LINEAR);
        ConfigureMaterialLighting(meshWork->material, materialAttribute);
    }
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZCompLoc(GX_FALSE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXCallDisplayList(meshWork->drawData, meshWork->drawFlags);
}

void ConfigureMaterialLighting(HSFMATERIAL *material, HSFATTRIBUTE *attribute)
{
    HU3DMODEL lightingModel;
    Mtx hiliteMatrix;
    GXColor hiliteColor;
    GXColor materialColor;
    f32 lightScale;
    f32 hiliteScale;
    s16 lightId;
    u16 tevStage;
    u32 materialFlags;
    u16 texCoordIndex;
    u16 usesNormalMapping;
    s32 lightingEnabled;

    lightingModel.attr = 0x20000;
    materialFlags = material->flags;
    if ((material->vtxMode == 2) || (material->vtxMode == 3)) {
        usesNormalMapping = 1;
    }
    else {
        usesNormalMapping = 0;
        if ((material->vtxMode == 0) || (material->vtxMode == 5)) {
            lightingEnabled = 0;
        }
        else {
            lightingEnabled = 1;
        }
    }
    texCoordIndex = tevStage = 1;
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    if (1.0f == attribute->unk20) {
        if (attribute->unk8[2] == 0) {
            GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
            GXSetTevOrder(tevStage, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
            GXSetTevColorIn(tevStage, GX_CC_CPREV, GX_CC_TEXC, GX_CC_TEXA, GX_CC_ZERO);
            GXSetTevColorOp(tevStage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(tevStage, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
            GXSetTevAlphaOp(tevStage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            tevStage++;
        }
        else {
            GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
            GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
            GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        }
    }
    else {
        GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    }
    if (usesNormalMapping != 0) {
        if (((lightingModel.attr & 0x20000) != 0) || ((materialFlags & 0x100) != 0)) {
            hiliteColor.a = modelVisibility;
            GXSetTevColor(GX_TEVREG0, hiliteColor);
            GXSetTexCoordGen2(texCoordIndex, GX_TG_MTX2x4, GX_TG_NRM, GX_TEXMTX2, GX_FALSE, GX_PTIDENTITY);
            GXSetTevOrder(tevStage, texCoordIndex, highlightTextureMapId, GX_COLOR0A0);
            GXSetTevColorIn(tevStage, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ONE, GX_CC_CPREV);
            GXSetTevColorOp(tevStage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(tevStage, GX_CA_ZERO, GX_CA_APREV, GX_CA_A0, GX_CA_ZERO);
            GXSetTevAlphaOp(tevStage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
            hiliteScale = 6.0f * (material->hiliteScale / 300.0f);
            if (hiliteScale < 0.1) {
                hiliteScale = 0.1f;
            }
            MTXCopy(highlightMatrix, hiliteMatrix);
            mtxScaleCat(hiliteMatrix, hiliteScale, hiliteScale, hiliteScale);
            GXLoadTexMtxImm(hiliteMatrix, 0x24, GX_MTX2x4);
            tevStage++;
            texCoordIndex++;
            usesNormalMapping = 0;
            lightingEnabled = 1;
        }
        else {
            if (1.0f == attribute->unk20) {
                GXSetTevOrder(tevStage, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR1A1);
                GXSetTevColorIn(tevStage, GX_CC_CPREV, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
            }
            else {
                GXSetTevOrder(tevStage, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR1A1);
                GXSetTevColorIn(tevStage, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_CPREV);
            }
            GXSetTevColorOp(tevStage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(tevStage, GX_CA_ZERO, GX_CA_APREV, GX_CA_A0, GX_CA_ZERO);
            GXSetTevAlphaOp(tevStage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            tevStage++;
        }
    }
    else if (0.0f != material->invAlpha) {
        GXSetTevOrder(tevStage, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevColorIn(tevStage, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
        GXSetTevColorOp(tevStage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(tevStage, GX_CA_ZERO, GX_CA_APREV, GX_CA_A0, GX_CA_ZERO);
        GXSetTevAlphaOp(tevStage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        tevStage++;
    }
    GXSetNumTexGens(texCoordIndex);
    GXSetNumTevStages(tevStage);
    if (usesNormalMapping != 0) {
        lightScale = material->hiliteScale;
    }
    else {
        lightScale = 0.0f;
    }
    lightId = Hu3DLightSet(&lightingModel, &Hu3DCameraMtx, &Hu3DCameraMtxXPose, lightScale);
    if (usesNormalMapping != 0) {
        GXSetNumChans(2);
        if (material->vtxMode == 5) {
            GXSetChanCtrl(GX_COLOR0A0, 1, GX_SRC_REG, GX_SRC_VTX, lightId, GX_DF_CLAMP, GX_AF_NONE);
            GXSetChanCtrl(GX_COLOR1A1, 1, GX_SRC_REG, GX_SRC_VTX, lightId, GX_DF_NONE, GX_AF_SPEC);
            return;
        }
        GXSetChanCtrl(GX_COLOR0A0, 1, GX_SRC_REG, GX_SRC_REG, lightId, GX_DF_CLAMP, GX_AF_NONE);
        GXSetChanCtrl(GX_COLOR1A1, 1, GX_SRC_REG, GX_SRC_REG, lightId, GX_DF_NONE, GX_AF_SPEC);
        return;
    }
    GXSetNumChans(1);
    if (material->vtxMode == 5) {
        GXSetChanCtrl(GX_COLOR0A0, lightingEnabled, GX_SRC_REG, GX_SRC_REG, lightId, GX_DF_CLAMP, GX_AF_SPOT);
        return;
    }
    GXSetChanCtrl(GX_COLOR0A0, lightingEnabled, GX_SRC_REG, GX_SRC_REG, lightId, GX_DF_CLAMP, GX_AF_SPOT);
}

void CalculateHighlightMatrix(Mtx modelMatrix)
{
    Mtx hiliteProjection;
    Mtx inverseModelMatrix;
    Mtx lightRotation;
    Vec lightDirection;
    Vec halfAngleDirection;
    Vec rotationAxis;
    Vec viewDirection = { 0, 0, -1 };
    f32 rotationAngle;
    f32 lightAlignment;
    HU3DLIGHT *directionalLight;
    s16 lightAlignmentS16;

    directionalLight = &Hu3DGlobalLight[0];
    lightDirection = directionalLight->dir;
    if ((directionalLight->type & 0x8000) != 0) {
        MTXMultVecSR(Hu3DCameraMtx, &lightDirection, &lightDirection);
    }
    lightAlignment = VECDotProduct(&lightDirection, &viewDirection);
    lightAlignment *= 10000.0f;
    OSf32tos16(&lightAlignment, &lightAlignmentS16);
    if (lightAlignmentS16 == -0x2710) {
        MTXScale(highlightMatrix, 0.0f, 0.0f, 0.0f);
        return;
    }
    C_VECHalfAngle(&lightDirection, &viewDirection, &halfAngleDirection);
    halfAngleDirection.x = -halfAngleDirection.x;
    halfAngleDirection.y = -halfAngleDirection.y;
    halfAngleDirection.z = -halfAngleDirection.z;
    MTXInvXpose(modelMatrix, inverseModelMatrix);
    if (lightAlignmentS16 == 0x2710) {
        MTXIdentity(lightRotation);
    }
    else {
        VECCrossProduct(&halfAngleDirection, &viewDirection, &rotationAxis);
        rotationAngle = acosf(VECDotProduct(&viewDirection, &halfAngleDirection));
        MTXRotAxisRad(lightRotation, &rotationAxis, rotationAngle);
    }
    MTXConcat(lightRotation, inverseModelMatrix, hiliteProjection);
    MTXTrans(lightRotation, 0.5f, 0.5f, 0.0f);
    MTXConcat(lightRotation, hiliteProjection, highlightMatrix);
}

void CaptureFrameTexture(void)
{
    Mtx44 orthographicProjection;
    Mtx identityMatrix;
    GXTexObj frameTexture;
    GXColor overlayColor;
    u16 effectMode;
    EffectWork *effectWork;

    effectWork = (EffectWork *)effectObject->data;
    if (!effectWork) {
        effectMode = 0;
    }
    else {
        effectMode = effectWork->stateFlags & 7;
    }

    if (effectMode == 5) {
        C_MTXOrtho(orthographicProjection, 0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 10.0f);
        GXSetProjection(orthographicProjection, GX_ORTHOGRAPHIC);
        MTXIdentity(identityMatrix);
        GXLoadPosMtxImm(identityMatrix, 0);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
        GXInitTexObj(&frameTexture, capturedFrameTexture, 0x280, 0x1E0, 6, GX_CLAMP, GX_CLAMP, GX_FALSE);
        GXInitTexObjLOD(&frameTexture, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
        GXLoadTexObj(&frameTexture, GX_TEXMAP0);
        GXSetNumTevStages(1);
        overlayColor.a = 0xC4;
        GXSetTevColor(GX_TEVREG0, overlayColor);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
        GXSetZMode(0, GX_LEQUAL, GX_FALSE);
        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        GXPosition3f32(0.0f, 0.0f, 0.0f);
        GXTexCoord2f32(0.0f, 0.0f);
        GXPosition3f32(640.0f, 0.0f, 0.0f);
        GXTexCoord2f32(1.0f, 0.0f);
        GXPosition3f32(640.0f, 480.0f, 0.0f);
        GXTexCoord2f32(1.0f, 1.0f);
        GXPosition3f32(0.0f, 480.0f, 0.0f);
        GXTexCoord2f32(0.0f, 1.0f);
    }
    GXSetTexCopySrc(0, 0, 0x280, 0x1E0);
    GXSetTexCopyDst(0x280, 0x1E0, GX_TF_RGBA8, GX_FALSE);
    GXCopyTex(capturedFrameTexture, 0);
    DCFlushRange(capturedFrameTexture, capturedFrameTextureSize);
}

void CalculateTriangleNormal(Vec *firstVertex, Vec *secondVertex, Vec *thirdVertex, f32 normalComponents[5])
{
    Vec firstEdge;
    Vec secondEdge;

    firstEdge.x = secondVertex->x - firstVertex->x;
    firstEdge.y = secondVertex->y - firstVertex->y;
    firstEdge.z = secondVertex->z - firstVertex->z;
    secondEdge.x = thirdVertex->x - secondVertex->x;
    secondEdge.y = thirdVertex->y - secondVertex->y;
    secondEdge.z = thirdVertex->z - secondVertex->z;
    normalComponents[0] = -((firstEdge.y * secondEdge.z) - (firstEdge.z * secondEdge.y));
    normalComponents[1] = -((firstEdge.z * secondEdge.x) - (firstEdge.x * secondEdge.z));
    normalComponents[2] = -((firstEdge.x * secondEdge.y) - (firstEdge.y * secondEdge.x));
}

f32 InterpolateAngle(f32 startAngle, f32 endAngle, f32 progress)
{
    f32 angleDifference;

    angleDifference = endAngle - startAngle;
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    else if (angleDifference < -180.0f) {
        angleDifference += 360.0f;
    }
    angleDifference = startAngle + (angleDifference * progress);
    if (angleDifference >= 360.0f) {
        angleDifference -= 360.0f;
    }
    else if (angleDifference < 0.0f) {
        angleDifference += 360.0f;
    }
    return angleDifference;
}

void TransposeMatrixRotation(Mtx sourceMatrix, Mtx destinationMatrix)
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

f32 InterpolateValue(f32 startValue, f32 endValue, f32 progress)
{
    return startValue + (progress * (endValue - startValue));
}

f32 QuadraticInterpolate(f32 startValue, f32 controlValue, f32 endValue, f32 progress)
{
    f32 inverseProgress;

    inverseProgress = 1.0f - progress;
    return (endValue * (progress * progress)) + ((startValue * (inverseProgress * inverseProgress)) + (controlValue * (2.0f * inverseProgress * progress)));
}

ParticleWork *CreateParticleWork(s16 spriteCount, Vec *position, Vec *scale, f32 gravity, GXColor *color)
{
    HU3DMODEL *modelData;
    s16 particleIndex;
    ParticleWork *particleWork;
    ModelDrawWork *hookData;

    modelData = &Hu3DData[spriteCount];
    hookData = modelData->hookData;
    particleIndex = 0;
    particleWork = hookData->particleData;
    for (; particleIndex < hookData->flags; particleIndex++, particleWork++) {
        if (particleWork->isActive == 0)
            break;
    }
    if (particleIndex == hookData->flags) {
        return NULL;
    }
    hookData->vertexColors[particleIndex].r = color->r;
    hookData->vertexColors[particleIndex].g = color->g;
    hookData->vertexColors[particleIndex].b = color->b;
    hookData->vertexColors[particleIndex].a = color->a;
    particleWork->reserved40 = particleIndex;
    particleWork->reserved44 = gravity;
    particleWork->renderScale = *scale;
    particleWork->position = *position;
    particleWork->textureFrame = 0;
    particleWork->reserved3C = 0;
    particleWork->isActive = 1;
    return particleWork;
}

s16 CreateSpriteParticleModel(ANIMDATA *animation, s16 particleCount, f32 particleSize, s16 textureWidth, s16 textureHeight)
{
    HU3DMODEL *hookModel;
    s16 hookModelId;
    s16 textureColumns;
    s16 textureRows;
    HuVec2f *spriteTextureCoordinates;
    s16 spriteIndex;
    SpriteModelWork *spriteModelWork;
    void *displayListBuffer;
    void *displayListStart;
    ParticleWork *spriteParticleWork;
    Vec *spritePositions;
    GXColor *spriteColors;

    hookModelId = Hu3DHookFuncCreate(DrawSpriteParticles);
    hookModel = &Hu3DData[hookModelId];
    spriteModelWork = HuMemDirectMallocNum(HEAP_MODEL, 0x68, hookModel->mallocNo);
    hookModel->hookData = spriteModelWork;
    animation->useNum += 1;
    spriteModelWork->animation = animation;
    spriteModelWork->particleCount = particleCount;
    spriteModelWork->isActive = 0;
    spriteModelWork->updateCallback = NULL;
    spriteParticleWork = HuMemDirectMallocNum(HEAP_MODEL, particleCount * 0x64, hookModel->mallocNo);
    spriteModelWork->particleData = spriteParticleWork;

    for (spriteIndex = 0; spriteIndex < particleCount; spriteIndex++, spriteParticleWork++) {
        spriteParticleWork->textureFrame = -1;
        spriteParticleWork->isActive = 0;
    }
    spritePositions = HuMemDirectMallocNum(HEAP_MODEL, particleCount * 0xC * 4, hookModel->mallocNo);
    spriteModelWork->particlePositions = spritePositions;

    for (spriteIndex = 0; spriteIndex < (particleCount * 4); spriteIndex++, spritePositions++) {
        spritePositions->x = spritePositions->y = spritePositions->z = 0.0f;
    }
    spriteColors = HuMemDirectMallocNum(HEAP_MODEL, particleCount * 4, hookModel->mallocNo);
    spriteModelWork->particleColors = spriteColors;

    for (spriteIndex = 0; spriteIndex < particleCount; spriteIndex++, spriteColors++) {
        spriteColors->r = spriteColors->g = spriteColors->b = spriteColors->a = 0xFF;
    }
    spriteTextureCoordinates = HuMemDirectMallocNum(HEAP_MODEL, particleCount * 8 * 4, hookModel->mallocNo);
    spriteModelWork->textureCoordinates = spriteTextureCoordinates;

    for (spriteIndex = 0; spriteIndex < particleCount; spriteIndex++) {
        spriteTextureCoordinates->x = 0.0f;
        spriteTextureCoordinates->y = 0.0f;
        spriteTextureCoordinates++;
        spriteTextureCoordinates->x = 1.0f;
        spriteTextureCoordinates->y = 0.0f;
        spriteTextureCoordinates++;
        spriteTextureCoordinates->x = 1.0f;
        spriteTextureCoordinates->y = 1.0f;
        spriteTextureCoordinates++;
        spriteTextureCoordinates->x = 0.0f;
        spriteTextureCoordinates->y = 1.0f;
        spriteTextureCoordinates++;
    }
    if ((textureWidth != 0) || (textureHeight != 0)) {
        textureColumns = (animation->bmp->sizeX / textureWidth);
        textureRows = animation->bmp->sizeY / textureHeight;
        spriteModelWork->textureFrameCount = (textureColumns * textureRows);
        spriteModelWork->textureWidth = (1.0f / textureColumns);
        spriteModelWork->textureHeight = (1.0f / textureRows);
    }
    else {
        textureColumns = 1;
        spriteModelWork->textureFrameCount = 1;
        spriteModelWork->textureWidth = 1.0f;
        spriteModelWork->textureHeight = 1.0f;
    }
    spriteModelWork->textureGrid = HuMemDirectMallocNum(HEAP_MODEL, spriteModelWork->textureFrameCount * 8, hookModel->mallocNo);
    CalculateSpriteTextureGrid(spriteModelWork->textureGrid, spriteModelWork->textureFrameCount, textureColumns, spriteModelWork->textureWidth, spriteModelWork->textureHeight);
    spriteModelWork->topLeft.x = spriteModelWork->bottomRight.x = spriteModelWork->bottomLeft.y = spriteModelWork->bottomRight.y = -particleSize;
    spriteModelWork->topRight.x = spriteModelWork->bottomLeft.x = spriteModelWork->topLeft.y = spriteModelWork->topRight.y = particleSize;
    spriteModelWork->topLeft.z = spriteModelWork->topRight.z = spriteModelWork->bottomLeft.z = spriteModelWork->bottomRight.z = 0.0f;
    displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, 0x20000, hookModel->mallocNo);
    displayListStart = displayListBuffer;
    GXBeginDisplayList(displayListStart, 0x20000);
    GXBegin(GX_QUADS, GX_VTXFMT0, (particleCount * 4));

    for (spriteIndex = 0; spriteIndex < particleCount; spriteIndex++) {
        GXPosition1x16(spriteIndex * 4);
        GXColor1x16(spriteIndex);
        GXTexCoord1x16(spriteIndex * 4);
        GXPosition1x16((spriteIndex * 4) + 1);
        GXColor1x16(spriteIndex);
        GXTexCoord1x16((spriteIndex * 4) + 1);
        GXPosition1x16((spriteIndex * 4) + 2);
        GXColor1x16(spriteIndex);
        GXTexCoord1x16((spriteIndex * 4) + 2);
        GXPosition1x16((spriteIndex * 4) + 3);
        GXColor1x16(spriteIndex);
        GXTexCoord1x16((spriteIndex * 4) + 3);
    }
    spriteModelWork->textureId = GXEndDisplayList();
    DCFlushRangeNoSync(displayListBuffer, spriteModelWork->textureId);
    spriteModelWork->textureData = HuMemDirectMallocNum(HEAP_MODEL, spriteModelWork->textureId, hookModel->mallocNo);
    memcpy(spriteModelWork->textureData, displayListBuffer, spriteModelWork->textureId);
    DCFlushRangeNoSync(spriteModelWork->textureData, spriteModelWork->textureId);
    HuMemDirectFree(displayListBuffer);
    return hookModelId;
}

void DestroyEffectModel(s16 modelId)
{
    HU3DMODEL *data;
    SpriteModelWork *spriteWork;

    data = &Hu3DData[modelId];
    spriteWork = (SpriteModelWork *)data->hookData;
    HuSprAnimKill(spriteWork->animation);
    Hu3DModelKill(modelId);
}

SpriteModelWork *GetSpriteModelWork(s16 modelId)
{
    HU3DMODEL *data;

    data = &Hu3DData[modelId];
    return (SpriteModelWork *)data->hookData;
}

ParticleWork *GetSpriteParticleWork(s16 modelId, s16 particleIndex)
{
    HU3DMODEL *data;
    SpriteModelWork *spriteWork;

    data = &Hu3DData[modelId];
    spriteWork = (SpriteModelWork *)data->hookData;

    if (particleIndex == -1) {
        return 0;
    }

    return &spriteWork->particleData[particleIndex];
}

void SetSpriteUpdateCallback(s16 modelId, SpriteUpdateCallback updateCallback)
{
    HU3DMODEL *data;
    SpriteModelWork *spriteWork;

    data = &Hu3DData[modelId];
    spriteWork = (SpriteModelWork *)data->hookData;
    spriteWork->updateCallback = updateCallback;
}

void SetSpriteModelFlags(s16 modelId, u8 modelFlags)
{
    HU3DMODEL *data;
    SpriteModelWork *spriteWork;

    data = &Hu3DData[modelId];
    spriteWork = (SpriteModelWork *)data->hookData;
    spriteWork->isActive = modelFlags;
}

void DrawSpriteParticles(HU3DMODEL *model, Mtx modelMatrix)
{
    Mtx modelMatrix;
    ROMtx orderedModelMatrix;
    Mtx particleRotation;
    Mtx particleTransform;
    Vec spriteCorners[4];
    Vec rotatedCorners[4];
    Vec worldCorners[4];
    Vec *spritePositions;
    s16 bitmapFormat;
    u8 bitmapPixelSize;
    SpriteModelWork *spriteWork;
    HuVec2f *textureCoordinates;
    s16 spriteIndex;
    ParticleWork *spriteParticleWork;
    SpriteUpdateCallback spriteUpdateCallback;

    spriteWork = (SpriteModelWork *)model->hookData;
    GXLoadPosMtxImm(modelMatrix, 0);
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    bitmapFormat = spriteWork->animation->bmp->dataFmt & 0xF;
    if ((bitmapFormat == 7) || (bitmapFormat == 8)) {
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXA, GX_CC_RASC, GX_CC_ZERO);
    }
    else {
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
    }
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    HuSprTexLoad(spriteWork->animation, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZCompLoc(0);
    switch (spriteWork->isActive) {
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
    GXSetArray(GX_VA_POS, spriteWork->particlePositions, 0xC);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, spriteWork->particleColors, 4);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_TEX0, spriteWork->textureCoordinates, 8);
    TransposeMatrixRotation(modelMatrix, modelMatrix);
    PSMTXReorder(modelMatrix, orderedModelMatrix);
    if (((u8)omPauseChk() == 0) && (spriteWork->updateCallback)) {
        spriteUpdateCallback = spriteWork->updateCallback;
        spriteUpdateCallback(model, spriteWork, modelMatrix);
    }
    spriteParticleWork = spriteWork->particleData;
    spritePositions = spriteWork->particlePositions;
    textureCoordinates = spriteWork->textureCoordinates;
    PSMTXROMultVecArray(orderedModelMatrix, &spriteWork->topLeft, (Vec *)&spriteCorners[0], 4);

    for (spriteIndex = 0; spriteIndex < spriteWork->particleCount; spriteIndex++, spriteParticleWork++, textureCoordinates += 4) {
        if (spriteParticleWork->isActive == 0) {
            spritePositions->x = spritePositions->y = spritePositions->z = 0.0f;
            spritePositions++;
            spritePositions->x = spritePositions->y = spritePositions->z = 0.0f;
            spritePositions++;
            spritePositions->x = spritePositions->y = spritePositions->z = 0.0f;
            spritePositions++;
            spritePositions->x = spritePositions->y = spritePositions->z = 0.0f;
            spritePositions++;
        }
        else {
            if (0.0f == spriteParticleWork->reserved44) {
                ScaleVectors(spriteCorners, rotatedCorners, &spriteParticleWork->renderScale, 4);
                VECAdd(&rotatedCorners[0], &spriteParticleWork->position, spritePositions++);
                VECAdd(&rotatedCorners[1], &spriteParticleWork->position, spritePositions++);
                VECAdd(&rotatedCorners[2], &spriteParticleWork->position, spritePositions++);
                VECAdd(&rotatedCorners[3], &spriteParticleWork->position, spritePositions++);
            }
            else {
                ScaleVectors(&spriteWork->topLeft, rotatedCorners, &spriteParticleWork->renderScale, 4);
                MTXRotRad(particleRotation, 0x5A, 0.017453292f * spriteParticleWork->reserved44);
                MTXConcat(modelMatrix, particleRotation, particleTransform);
                MTXMultVecArray(particleTransform, rotatedCorners, worldCorners, 4);
                VECAdd(&worldCorners[0], &spriteParticleWork->position, spritePositions++);
                VECAdd(&worldCorners[1], &spriteParticleWork->position, spritePositions++);
                VECAdd(&worldCorners[2], &spriteParticleWork->position, spritePositions++);
                VECAdd(&worldCorners[3], &spriteParticleWork->position, spritePositions++);
            }
            if (spriteParticleWork->textureFrame != -1) {
                CalculateSpriteTextureCoordinates(textureCoordinates, spriteWork->textureGrid, spriteParticleWork->textureFrame, spriteWork->textureWidth, spriteWork->textureHeight);
            }
        }
    }
    DCFlushRangeNoSync(spriteWork->particlePositions, spriteWork->particleCount * 0xC * 4);
    DCFlushRangeNoSync(spriteWork->textureCoordinates, spriteWork->particleCount * 8 * 4);
    GXCallDisplayList(spriteWork->textureData, spriteWork->textureId);
}

void CalculateSpriteTextureGrid(HuVec2f *textureGrid, s16 particleCount, s16 columnCount, f32 cellWidth, f32 cellHeight)
{
    s16 spriteIndex;
    s16 textureColumn;
    s16 textureRow;

    for (spriteIndex = 0; spriteIndex < particleCount; spriteIndex++, textureGrid++) {
        textureColumn = (spriteIndex % columnCount);
        textureRow = (spriteIndex / columnCount);
        textureGrid->x = (textureColumn * cellWidth);
        textureGrid->y = (textureRow * cellHeight);
    }
}

void CalculateSpriteTextureCoordinates(HuVec2f *coordinates, HuVec2f *textureGrid, s16 frameIndex, f32 frameWidth, f32 frameHeight)
{
    coordinates[0].x = textureGrid[frameIndex].x;
    coordinates[0].y = textureGrid[frameIndex].y;
    coordinates[1].x = (frameWidth + textureGrid[frameIndex].x);
    coordinates[1].y = textureGrid[frameIndex].y;
    coordinates[2].x = (frameWidth + textureGrid[frameIndex].x);
    coordinates[2].y = (frameHeight + textureGrid[frameIndex].y);
    coordinates[3].x = textureGrid[frameIndex].x;
    coordinates[3].y = (frameHeight + textureGrid[frameIndex].y);
}

void ScaleVectors(Vec *sourceVectors, Vec *scaledVectors, Vec *scale, s16 vectorCount)
{
    s16 vectorIndex;

    for (vectorIndex = 0; vectorIndex < vectorCount; vectorIndex++, sourceVectors++, scaledVectors++) {
        scaledVectors->x = (sourceVectors->x * scale->x);
        scaledVectors->y = (sourceVectors->y * scale->y);
        scaledVectors->z = (sourceVectors->z * scale->z);
    }
}

static s8 debugColorAlpha = 0xFF;

void UpdateDebugLight(HU3DMODEL *model, Mtx modelMatrix)
{
    if (debugState == 0) {
        if ((HuPadBtnDown[0] & 0x10) != 0) {
            debugLightEnabled ^= 1;
        }
        if (debugLightEnabled == 0)
            return;

        if ((HuPadBtn[0] & 1) != 0) {
            debugLightTarget.x -= 4.0f;
        }
        if ((HuPadBtn[0] & 2) != 0) {
            debugLightTarget.x += 4.0f;
        }
        debugLightPosition.x += HuPadSubStkX[0];
        if ((HuPadBtn[0] & 0x20) != 0) {
            if ((HuPadBtn[0] & 8) != 0) {
                debugLightTarget.y -= 4.0f;
            }
            if ((HuPadBtn[0] & 4) != 0) {
                debugLightTarget.y += 4.0f;
            }
            debugLightPosition.y += HuPadSubStkY[0];
        }
        else {
            if ((HuPadBtn[0] & 8) != 0) {
                debugLightTarget.z -= 4.0f;
            }
            if ((HuPadBtn[0] & 4) != 0) {
                debugLightTarget.z += 4.0f;
            }
            debugLightPosition.z -= HuPadSubStkY[0];
        }
        shadowPosition.x = debugLightPosition.x;
        shadowPosition.y = debugLightPosition.y;
        shadowPosition.z = debugLightPosition.z;
        shadowTarget.x = debugLightTarget.x;
        shadowTarget.y = debugLightTarget.y;
        shadowTarget.z = debugLightTarget.z - 500.0f;
        Hu3DGLightPosSet(directionalLightId, debugLightPosition.x, debugLightPosition.y, debugLightPosition.z, 0.0f, 0.0f, 0.0f);
        Hu3DGLightPosAimSetV(directionalLightId, &debugLightPosition, &debugLightTarget);
        Hu3DShadowPosSet(&shadowPosition, &shadowUpVector, &shadowTarget);
    }

    print8(8, 0x64, 1.5f, "InterXYZ: %.2f %.2f %.2f", debugLightTarget.x, debugLightTarget.y, debugLightTarget.z);
    print8(8, 0x70, 1.5f, "PositionXYZ: %.2f %.2f %.2f", debugLightPosition.x, debugLightPosition.y, debugLightPosition.z);
    GXLoadPosMtxImm(modelMatrix, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetZMode(1, GX_LEQUAL, 1);
    GXBegin(GX_LINES, GX_VTXFMT0, 2);
    GXPosition3f32(debugLightTarget.x, debugLightTarget.y, debugLightTarget.z);
    GXColor4u8(0xFF, 0xFF, 0xFF, 0xFF);
    GXPosition3f32(debugLightPosition.x, debugLightPosition.y, debugLightPosition.z);
    GXColor4u8(0xFF, 0x00, 0x00, 0xFF);
}

void UpdateDebugControls(void)
{
    if ((HuPadBtnDown[0] & 0x40) != 0) {
        debugState ^= 1;
    }
    if (debugState != 0) {
        if ((HuPadBtn[0] & 1) != 0) {
            CRot.y += 1.0f;
        }
        if ((HuPadBtn[0] & 2) != 0) {
            CRot.y -= 1.0f;
        }
        if ((HuPadBtn[0] & 8) != 0) {
            CRot.x -= 1.0f;
        }
        if ((HuPadBtn[0] & 4) != 0) {
            CRot.x += 1.0f;
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
