#include "game/disp.h"
#include "game/hsfload.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/printfunc.h"


#include "game/audio.h"
#include "game/gamework_data.h"
#include "game/msm.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/wipe.h"

#include "ext_math.h"
#include <string.h>

typedef struct ModelTestAudioListenerWork {
    u32 unreferencedListenerSetting;
    Vec unreferencedListenerPosition;
    Vec unreferencedListenerDirection;
    u8 reservedPadding[0x88];
} ModelTestAudioListenerWork;

OMOBJ *audioTestViewObject;
OMOBJ *audioListenerCameraObject;
OMOBJ *audioListenerControlObject;
ModelTestAudioListenerWork audioListenerWork;
s32 audioEmitterId;
float unreferencedAudioScratch;
float cameraFieldOfView;
s16 buttonRepeatFrames;
HUPROCESS *audioListenerProcess;
HUPROCESS *characterMotionTestProcess;

void AudioListenerTestUpdate(OMOBJ *object);
void AudioListenerCameraUpdate(OMOBJ *object);
void CharacterMotionTestLoop(void);
void ModelGridDrawHook(HU3DMODEL *model, Mtx modelTransform);
void PauseAndZoomControlLoop(void);

static const Vec zeroVector = { 0.0f, 0.0f, 0.0f };
static const Vec listenerForwardVector = { 0.0f, 0.0f, -1.0f };

void AudioListenerTestInitialize(void)
{
    Vec listenerPosition;
    Vec listenerForward;
    HUPROCESS *objman;
    OSReport("******* MODELTESTObjectSetup *********\n");
    objman = omInitObjMan(50, 8192);
    CRot.x = -22;
    CRot.y = 0;
    CRot.z = 0;
    Center.x = 0;
    Center.y = 100;
    Center.z = 0;
    CZoom = 400;
    Hu3DCameraCreate(1);
    cameraFieldOfView = 45;
    Hu3DCameraPerspectiveSet(1, cameraFieldOfView, 20, 5000, 1.2f);
    Hu3DCameraViewportSet(1, 0, 0, 640, 480, 0, 1);
    Hu3DCameraScissorSet(1, 0, 0, 640, 480);
    audioListenerControlObject = omAddObjEx(objman, 0, 32, 32, -1, AudioListenerTestUpdate);
    audioListenerCameraObject = omAddObjEx(objman, 32730, 0, 0, -1, AudioListenerCameraUpdate);
    characterMotionTestProcess = HuPrcCreate(CharacterMotionTestLoop, 100, 8192, 0);
    Hu3DBGColorSet(16, 16, 16);
    Hu3DHookFuncCreate(ModelGridDrawHook);
    HuPrcChildCreate(PauseAndZoomControlLoop, 100, 12288, 0, objman);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
    listenerPosition = zeroVector;
    listenerForward = listenerForwardVector;
    audioListenerWork.unreferencedListenerSetting = 64;
    audioListenerWork.unreferencedListenerDirection.x = 0;
    audioListenerWork.unreferencedListenerDirection.y = 0;
    audioListenerWork.unreferencedListenerDirection.z = 0;
    msmSeSetListener(&listenerPosition, &listenerForward, 2000, 1000, 0);
}

void AudioListenerTestUpdate(OMOBJ *object)
{
    float cameraRoll;
    Vec pos;
    Vec offset;
    Vec dir;
    Vec yOffset;

    s8 substickDelta;

    if (HuPadBtnDown[0] & PAD_BUTTON_START) {
        if (characterMotionTestProcess) {
            HuPrcKill(characterMotionTestProcess);
        }
        if (audioListenerProcess) {
            HuPrcKill(audioListenerProcess);
        }
        HuAudFXListnerKill();
        omOvlReturnEx(1, 1);
        return;
    }
    CRot.y += 0.1f * HuPadStkX[0];
    CRot.x += 0.1f * HuPadStkY[0];
    CZoom += HuPadTrigL[0] / 2;
    CZoom -= HuPadTrigR[0] / 2;
    if (CZoom < 100) {
        CZoom = 100;
    }
    if (CZoom > 2000) {
        CZoom = 2000;
    }
    pos.x = Center.x + (CZoom * (sind(CRot.y) * cosd(CRot.x)));
    pos.y = (Center.y + (CZoom * -sind(CRot.x)));
    pos.z = (Center.z + (CZoom * (cosd(CRot.y) * cosd(CRot.x))));
    offset.x = Center.x - pos.x;
    offset.y = Center.y - pos.y;
    offset.z = Center.z - pos.z;
    dir.x = (sind(CRot.y) * sind(CRot.x));
    dir.y = cosd(CRot.x);
    dir.z = (cosd(CRot.y) * sind(CRot.x));
    cameraRoll = CRot.z;
    yOffset.x = dir.x * (offset.x * offset.x + (1.0f - offset.x * offset.x) * cosd(cameraRoll))
        + dir.y * (offset.x * offset.y * (1.0f - cosd(cameraRoll)) - offset.z * sind(cameraRoll))
        + dir.z * (offset.x * offset.z * (1.0f - cosd(cameraRoll)) + offset.y * sind(cameraRoll));

    yOffset.y = dir.y * (offset.y * offset.y + (1.0f - offset.y * offset.y) * cosd(cameraRoll))
        + dir.x * (offset.x * offset.y * (1.0f - cosd(cameraRoll)) + offset.z * sind(cameraRoll))
        + dir.z * (offset.y * offset.z * (1.0f - cosd(cameraRoll)) - offset.x * sind(cameraRoll));

    yOffset.z = dir.z * (offset.z * offset.z + (1.0f - offset.z * offset.z) * cosd(cameraRoll))
        + (dir.x * (offset.x * offset.z * (1.0 - cosd(cameraRoll)) - offset.y * sind(cameraRoll))
            + dir.y * (offset.y * offset.z * (1.0 - cosd(cameraRoll)) + offset.x * sind(cameraRoll)));
    VECCrossProduct(&dir, &offset, &offset);
    VECNormalize(&offset, &offset);
    substickDelta = (HuPadSubStkX[0] & 0xF8);
    if (substickDelta != 0) {
        Center.x += 0.05f * (offset.x * substickDelta);
        Center.y += 0.05f * (offset.y * substickDelta);
        Center.z += 0.05f * (offset.z * substickDelta);
    }
    VECNormalize(&yOffset, &offset);
    substickDelta = -(HuPadSubStkY[0] & 0xF8);
    if (substickDelta != 0) {
        Center.x += 0.05f * (offset.x * substickDelta);
        Center.y += 0.05f * (offset.y * substickDelta);
        Center.z += 0.05f * (offset.z * substickDelta);
    }
    Hu3DCameraPerspectiveSet(1, cameraFieldOfView, 20, 5000, 1.2f);
}

void AudioListenerCameraUpdate(OMOBJ *object)
{
    Mtx rotAxis;
    Vec pos, target, up;
    float x, y, z;

    x = CRot.x;
    y = CRot.y;
    z = CRot.z;

    pos.x = (((sind(y) * cosd(x)) * CZoom) + Center.x);
    pos.y = (-sind(x) * CZoom) + Center.y;
    pos.z = ((cosd(y) * cosd(x)) * CZoom) + Center.z;
    target.x = Center.x;
    target.y = Center.y;
    target.z = Center.z;
    up.x = sind(y) * sind(x);
    up.y = cosd(x);
    up.z = cosd(y) * sind(x);
    Hu3DCameraPosSet(1, pos.x, pos.y, pos.z, up.x, up.y, up.z, target.x, target.y, target.z);
    MTXRotDeg(rotAxis, 'y', -y);
    MTXMultVec(rotAxis, &pos, &up);
    VECSubtract(&target, &pos, &up);
    VECNormalize(&up, &up);
    HuAudFXListnerUpdate(&pos, &up);
}

u32 marioCharacterAssets[] = { DATA_MAKE_NUM(DATADIR_MARIOMDL0, 0x00), 0x00000000, DATA_MAKE_NUM(DATADIR_MARIOMDL1, 0x00), 0x00000000,
    DATA_MAKE_NUM(DATADIR_MARIOMDL1, 0x01), 0x00000000, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x06), 0x00000001, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    0x00000001, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02), 0x00000001, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03), 0x00000001,
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x05), 0x00000000, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17), 0x00000000, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18),
    0x00000000, 0x00000000, 0x00000000 };

u32 luigiCharacterAssets[] = { DATA_MAKE_NUM(DATADIR_LUIGI, 0x00), 0x00000000, DATA_MAKE_NUM(DATADIR_LUIGI, 0x02), 0x00000000,
    DATA_MAKE_NUM(DATADIR_LUIGI, 0x03), 0x00000000, DATA_MAKE_NUM(DATADIR_LUIGI, 0x0D), 0x00000001, DATA_MAKE_NUM(DATADIR_LUIGI, 0x05), 0x00000001,
    DATA_MAKE_NUM(DATADIR_LUIGI, 0x07), 0x00000001, DATA_MAKE_NUM(DATADIR_LUIGI, 0x08), 0x00000001, DATA_MAKE_NUM(DATADIR_LUIGI, 0x0C), 0x00000000,
    DATA_MAKE_NUM(DATADIR_LUIGI, 0x28), 0x00000000, DATA_MAKE_NUM(DATADIR_LUIGI, 0x29), 0x00000000, 0x00000000, 0x00000000 };

u32 peachCharacterAssets[] = { DATA_MAKE_NUM(DATADIR_PEACH, 0x00), 0x00000000, DATA_MAKE_NUM(DATADIR_PEACH, 0x02), 0x00000000,
    DATA_MAKE_NUM(DATADIR_PEACH, 0x03), 0x00000000, DATA_MAKE_NUM(DATADIR_PEACH, 0x0D), 0x00000001, DATA_MAKE_NUM(DATADIR_PEACH, 0x05), 0x00000001,
    DATA_MAKE_NUM(DATADIR_PEACH, 0x07), 0x00000001, DATA_MAKE_NUM(DATADIR_PEACH, 0x08), 0x00000001, DATA_MAKE_NUM(DATADIR_PEACH, 0x0C), 0x00000000,
    DATA_MAKE_NUM(DATADIR_PEACH, 0x28), 0x00000000, DATA_MAKE_NUM(DATADIR_PEACH, 0x29), 0x00000000, 0x00000000, 0x00000000 };

u32 yoshiCharacterAssets[] = { DATA_MAKE_NUM(DATADIR_YOSHI, 0x00), 0x00000000, DATA_MAKE_NUM(DATADIR_YOSHI, 0x02), 0x00000000,
    DATA_MAKE_NUM(DATADIR_YOSHI, 0x03), 0x00000000, DATA_MAKE_NUM(DATADIR_YOSHI, 0x0D), 0x00000001, DATA_MAKE_NUM(DATADIR_YOSHI, 0x05), 0x00000001,
    DATA_MAKE_NUM(DATADIR_YOSHI, 0x07), 0x00000001, DATA_MAKE_NUM(DATADIR_YOSHI, 0x08), 0x00000001, DATA_MAKE_NUM(DATADIR_YOSHI, 0x0C), 0x00000000,
    DATA_MAKE_NUM(DATADIR_YOSHI, 0x28), 0x00000000, DATA_MAKE_NUM(DATADIR_YOSHI, 0x29), 0x00000000, 0x00000000, 0x00000000 };

u32 warioCharacterAssets[] = { DATA_MAKE_NUM(DATADIR_WARIO, 0x00), 0x00000000, DATA_MAKE_NUM(DATADIR_WARIO, 0x02), 0x00000000,
    DATA_MAKE_NUM(DATADIR_WARIO, 0x03), 0x00000000, DATA_MAKE_NUM(DATADIR_WARIO, 0x0D), 0x00000001, DATA_MAKE_NUM(DATADIR_WARIO, 0x05), 0x00000001,
    DATA_MAKE_NUM(DATADIR_WARIO, 0x07), 0x00000001, DATA_MAKE_NUM(DATADIR_WARIO, 0x08), 0x00000001, DATA_MAKE_NUM(DATADIR_WARIO, 0x0C), 0x00000000,
    DATA_MAKE_NUM(DATADIR_WARIO, 0x28), 0x00000000, DATA_MAKE_NUM(DATADIR_WARIO, 0x29), 0x00000000, 0x00000000, 0x00000000 };

u32 donkeyKongCharacterAssets[] = { DATA_MAKE_NUM(DATADIR_DONKEY, 0x00), 0x00000000, DATA_MAKE_NUM(DATADIR_DONKEY, 0x02), 0x00000000,
    DATA_MAKE_NUM(DATADIR_DONKEY, 0x03), 0x00000000, DATA_MAKE_NUM(DATADIR_DONKEY, 0x0D), 0x00000001, DATA_MAKE_NUM(DATADIR_DONKEY, 0x05), 0x00000001,
    DATA_MAKE_NUM(DATADIR_DONKEY, 0x07), 0x00000001, DATA_MAKE_NUM(DATADIR_DONKEY, 0x08), 0x00000001, DATA_MAKE_NUM(DATADIR_DONKEY, 0x0C), 0x00000000,
    DATA_MAKE_NUM(DATADIR_DONKEY, 0x28), 0x00000000, DATA_MAKE_NUM(DATADIR_DONKEY, 0x29), 0x00000000, 0x00000000, 0x00000000 };

u32 daisyCharacterAssets[] = { DATA_MAKE_NUM(DATADIR_DAISY, 0x00), 0x00000000, DATA_MAKE_NUM(DATADIR_DAISY, 0x02), 0x00000000,
    DATA_MAKE_NUM(DATADIR_DAISY, 0x03), 0x00000000, DATA_MAKE_NUM(DATADIR_DAISY, 0x0D), 0x00000001, DATA_MAKE_NUM(DATADIR_DAISY, 0x05), 0x00000001,
    DATA_MAKE_NUM(DATADIR_DAISY, 0x07), 0x00000001, DATA_MAKE_NUM(DATADIR_DAISY, 0x08), 0x00000001, DATA_MAKE_NUM(DATADIR_DAISY, 0x0C), 0x00000000,
    DATA_MAKE_NUM(DATADIR_DAISY, 0x28), 0x00000000, DATA_MAKE_NUM(DATADIR_DAISY, 0x29), 0x00000000, 0x00000000, 0x00000000 };

u32 waluigiCharacterAssets[]
    = { DATA_MAKE_NUM(DATADIR_WALUIGI, 0x00), 0x00000000, DATA_MAKE_NUM(DATADIR_WALUIGI, 0x02), 0x00000000, DATA_MAKE_NUM(DATADIR_WALUIGI, 0x03),
          0x00000000, DATA_MAKE_NUM(DATADIR_WALUIGI, 0x0D), 0x00000001, DATA_MAKE_NUM(DATADIR_WALUIGI, 0x05), 0x00000001,
          DATA_MAKE_NUM(DATADIR_WALUIGI, 0x07), 0x00000001, DATA_MAKE_NUM(DATADIR_WALUIGI, 0x08), 0x00000001, DATA_MAKE_NUM(DATADIR_WALUIGI, 0x0C),
          0x00000000, DATA_MAKE_NUM(DATADIR_WALUIGI, 0x28), 0x00000000, DATA_MAKE_NUM(DATADIR_WALUIGI, 0x29), 0x00000000, 0x00000000, 0x00000000 };

u32 *characterAssetLists[]
    = { marioCharacterAssets, luigiCharacterAssets, peachCharacterAssets, yoshiCharacterAssets, warioCharacterAssets, donkeyKongCharacterAssets, daisyCharacterAssets, waluigiCharacterAssets };

void CharacterAttributeAnimationReset(s16 characterId, s16 modelId, s16 attributeIndex);

void CharacterMotionTestLoop(void)
{
    u32 *characterAssetList;
    s16 motionIndex;
    s16 motionListIndex;
    s16 characterId;
    s16 motionCount;
    s16 visibleModelIndex;
    s16 motionChanged;

    s32 unreferencedMotionScratch = 0;
    s16 characterMotionIds[3][32];
    Vec cameraPosition;

    Vec worldOrigin = { 0, 0, 0 };
    Vec worldUpDirection = { 0, 1, 0 };
    s16 characterModelIds[3];
    s16 specialMotionIds[3];
    float cameraDistance;
    s16 unreferencedModelScratch = 0;
    Hu3DGLightCreate(0, 500, 1000, 0, -0.5, -1, 255, 255, 255);
    characterId = GWPlayerCfg[0].character;
    characterAssetList = characterAssetLists[characterId];
    characterModelIds[0] = Hu3DModelCreateFile(characterAssetList[0]);
    characterModelIds[1] = Hu3DModelCreateFile(characterAssetList[2]);
    characterModelIds[2] = Hu3DModelCreateFile(characterAssetList[4]);
    HuDataDirClose(characterAssetList[0]);
    HuDataDirClose(characterAssetList[2]);
    HuDataDirClose(characterAssetList[4]);
    motionCount = 0;
    motionListIndex = 0;
    while (1) {
        if (characterAssetList[(motionListIndex * 2) + 8] == 0) {
            if (motionCount == 0) {
                motionCount = motionListIndex;
            }
            break;
        }
        if (characterAssetList[(motionListIndex * 2) + 8] == 0xFFFFFFFF) {
            motionCount = motionListIndex;
        }
        else {
            characterMotionIds[0][motionListIndex] = Hu3DJointMotionFile(characterModelIds[0], characterAssetList[(motionListIndex * 2) + 8]);
            characterMotionIds[1][motionListIndex] = Hu3DJointMotionFile(characterModelIds[1], characterAssetList[(motionListIndex * 2) + 8]);
            characterMotionIds[2][motionListIndex] = Hu3DJointMotionFile(characterModelIds[2], characterAssetList[(motionListIndex * 2) + 8]);
        }
        motionListIndex++;
    }
    specialMotionIds[0] = Hu3DJointMotionFile(characterModelIds[0], characterAssetList[6]);
    specialMotionIds[1] = Hu3DJointMotionFile(characterModelIds[1], characterAssetList[6]);
    specialMotionIds[2] = Hu3DJointMotionFile(characterModelIds[2], characterAssetList[6]);
    visibleModelIndex = 0;
    motionIndex = 0;
    Hu3DMotionSet(characterModelIds[0], characterMotionIds[0][motionIndex]);
    Hu3DModelAttrSet(characterModelIds[0], HU3D_MOTATTR_LOOP);
    Hu3DMotionSet(characterModelIds[1], characterMotionIds[1][motionIndex]);
    Hu3DModelAttrSet(characterModelIds[1], HU3D_MOTATTR_LOOP);
    Hu3DMotionSet(characterModelIds[2], characterMotionIds[2][motionIndex]);
    Hu3DModelAttrSet(characterModelIds[2], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(characterModelIds[1], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(characterModelIds[2], HU3D_ATTR_DISPOFF);
    while (1) {
        motionChanged = 0;
        if (HuPadBtnDown[0] & PAD_BUTTON_X) {
            motionIndex++;
            if (motionIndex >= motionCount) {
                motionIndex = 0;
            }
            Hu3DMotionSet(characterModelIds[0], characterMotionIds[0][motionIndex]);
            Hu3DMotionSet(characterModelIds[1], characterMotionIds[1][motionIndex]);
            Hu3DMotionSet(characterModelIds[2], characterMotionIds[2][motionIndex]);
            if (characterAssetList[(motionIndex * 2) + 9] != 0) {
                Hu3DModelAttrSet(characterModelIds[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrSet(characterModelIds[1], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrSet(characterModelIds[2], HU3D_MOTATTR_LOOP);
            }
            else {
                Hu3DModelAttrReset(characterModelIds[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(characterModelIds[1], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(characterModelIds[2], HU3D_MOTATTR_LOOP);
            }
            CharacterAttributeAnimationReset(characterId, characterModelIds[0], 0);
            CharacterAttributeAnimationReset(characterId, characterModelIds[1], 1);
            CharacterAttributeAnimationReset(characterId, characterModelIds[2], 2);
            motionChanged = 1;
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_Y) {
            motionIndex--;
            if (motionIndex < 0) {
                motionIndex = motionCount - 1;
            }
            Hu3DMotionSet(characterModelIds[0], characterMotionIds[0][motionIndex]);
            Hu3DMotionSet(characterModelIds[1], characterMotionIds[1][motionIndex]);
            Hu3DMotionSet(characterModelIds[2], characterMotionIds[2][motionIndex]);
            if (characterAssetList[(motionIndex * 2) + 9] != 0) {
                Hu3DModelAttrSet(characterModelIds[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrSet(characterModelIds[1], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrSet(characterModelIds[2], HU3D_MOTATTR_LOOP);
            }
            else {
                Hu3DModelAttrReset(characterModelIds[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(characterModelIds[1], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(characterModelIds[2], HU3D_MOTATTR_LOOP);
            }
            CharacterAttributeAnimationReset(characterId, characterModelIds[0], 0);
            CharacterAttributeAnimationReset(characterId, characterModelIds[1], 1);
            CharacterAttributeAnimationReset(characterId, characterModelIds[2], 2);
            motionChanged = 1;
        }
        if (characterAssetList[(motionIndex * 2) + 9] == 0) {
            if (HuPadBtn[0] & PAD_BUTTON_A) {
                Hu3DModelAttrSet(characterModelIds[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrSet(characterModelIds[1], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrSet(characterModelIds[2], HU3D_MOTATTR_LOOP);
                motionChanged = 1;
            }
            else {
                if (motionIndex != 3 && motionIndex != 2) {
                    Hu3DModelAttrReset(characterModelIds[0], HU3D_MOTATTR_LOOP);
                    Hu3DModelAttrReset(characterModelIds[1], HU3D_MOTATTR_LOOP);
                    Hu3DModelAttrReset(characterModelIds[2], HU3D_MOTATTR_LOOP);
                }
            }
        }
        if (motionChanged != 0) {
            if (motionIndex == 3) {
                Hu3DMotionSet(characterModelIds[0], characterMotionIds[0][motionIndex]);
                Hu3DMotionSet(characterModelIds[1], characterMotionIds[1][motionIndex]);
                Hu3DMotionSet(characterModelIds[2], characterMotionIds[2][motionIndex]);
                Hu3DModelAttrReset(characterModelIds[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(characterModelIds[1], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(characterModelIds[2], HU3D_MOTATTR_LOOP);
                for (motionListIndex = 0; motionListIndex <= 30; motionListIndex++) {
                    Hu3DModelPosSet(characterModelIds[0], 0, sind(motionListIndex * 6.0f) * 150, 0);
                    Hu3DModelPosSet(characterModelIds[1], 0, sind(motionListIndex * 6.0f) * 150, 0);
                    Hu3DModelPosSet(characterModelIds[2], 0, sind(motionListIndex * 6.0f) * 150, 0);
                    if (motionListIndex == 25) {
                        Hu3DMotionSet(characterModelIds[0], specialMotionIds[0]);
                        Hu3DMotionSet(characterModelIds[1], specialMotionIds[1]);
                        Hu3DMotionSet(characterModelIds[2], specialMotionIds[2]);
                    }
                    HuPrcVSleep();
                }
                while (!Hu3DMotionEndCheck(characterModelIds[0])) {
                    HuPrcVSleep();
                }
                CharacterAttributeAnimationReset(characterId, characterModelIds[0], 0);
                CharacterAttributeAnimationReset(characterId, characterModelIds[1], 1);
                CharacterAttributeAnimationReset(characterId, characterModelIds[2], 2);
                Hu3DMotionShiftSet(characterModelIds[0], characterMotionIds[0][0], 0, 16, HU3D_MOTATTR_LOOP);
                Hu3DMotionShiftSet(characterModelIds[1], characterMotionIds[1][0], 0, 16, HU3D_MOTATTR_LOOP);
                Hu3DMotionShiftSet(characterModelIds[2], characterMotionIds[2][0], 0, 16, HU3D_MOTATTR_LOOP);
            }
            unreferencedMotionScratch = 0;
        }
        cameraPosition.x = Center.x + (CZoom * (sind(CRot.y) * cosd(CRot.x)));
        cameraPosition.y = (Center.y + (CZoom * -sind(CRot.x)));
        cameraPosition.z = (Center.z + (CZoom * (cosd(CRot.y) * cosd(CRot.x))));

        cameraDistance = VECMag(&cameraPosition);
        if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            visibleModelIndex++;
            if (visibleModelIndex >= 3) {
                visibleModelIndex = 0;
            }
        }
        Hu3DModelAttrSet(characterModelIds[0], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(characterModelIds[1], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(characterModelIds[2], HU3D_ATTR_DISPOFF);
        switch (visibleModelIndex) {
            case 0:
                Hu3DModelAttrReset(characterModelIds[0], HU3D_ATTR_DISPOFF);
                break;

            case 1:
                Hu3DModelAttrReset(characterModelIds[1], HU3D_ATTR_DISPOFF);
                break;

            case 2:
                Hu3DModelAttrReset(characterModelIds[2], HU3D_ATTR_DISPOFF);
                break;
        }
        HuPrcVSleep();
    }
}

char *characterAttributeNames[] = {
    "s3_mario_eyes_1",
    "mario_eyes_2",
    "mario_eyes_2",
    "S3c001m0_eye",
    "S3c001m1_eye",
    "c001m3_eye",
    "GC-eyes",
    "s3tc_GC-eyes_2",
    "s3tc_GC-eyes_2",
    "S3c003m1",
    "S3c003m1",
    "S3c003m1",
    "GC-eyes",
    "s3tc_GC-eyes_2",
    "s3tc_GC-eyes_2",
    "GC-eyes",
    "s3tc_GC-eyes_2",
    "s3tc_GC-eyes_2",
    "GC-eyes",
    "s3tc_GC-eyes_2",
    "s3tc_GC-eyes_2",
    "GC-eyes",
    "s3tc_GC-eyes_2",
    "s3tc_GC-eyes_2",
};

void CharacterAttributeAnimationReset(s16 characterId, s16 modelId, s16 attributeIndex)
{
    HU3DATTRANIM *attributeAnimation;
    HSFATTRIBUTE *modelAttribute;
    HU3DMODEL *modelWork;
    s16 attributeIndex;
    char *attributeName;
    modelWork = &Hu3DData[modelId];
    modelAttribute = modelWork->hsf->attribute;
    attributeName = MakeObjectName(characterAttributeNames[attributeIndex + (characterId * 3)]);
    for (attributeIndex = 0; attributeIndex < modelWork->hsf->attributeNum; attributeIndex++, modelAttribute++) {
        if (modelAttribute->bitmap->name[0] == attributeName[0] && strcmp(modelAttribute->bitmap->name, attributeName) == 0 && modelAttribute->animWorkP) {
            attributeAnimation = modelAttribute->animWorkP;
            attributeAnimation->trans3D.x = attributeAnimation->trans3D.y = attributeAnimation->trans3D.z = 0;
            attributeAnimation->rot.x = attributeAnimation->rot.y = attributeAnimation->rot.z = 0;
        }
    }
}

s32 modelGridExtent = 20;

void ModelGridDrawHook(HU3DMODEL *model, Mtx modelTransform)
{
    s32 gridLineIndex;
    u8 lineColorRed, lineColorGreen, lineColorBlue;
    s32 unreferencedRendererScratch = 0;
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXLoadPosMtxImm(modelTransform, GX_PNMTX0);
    GXBegin(GX_LINES, GX_VTXFMT0, (modelGridExtent + 1) * 4);
    for (gridLineIndex = 0; gridLineIndex <= modelGridExtent; gridLineIndex++) {
        lineColorRed = lineColorGreen = lineColorBlue = 255;
        GXPosition3f32((gridLineIndex * 100) - ((modelGridExtent / 2) * 100), 0, (-(modelGridExtent / 2)) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32((gridLineIndex * 100) - ((modelGridExtent / 2) * 100), 0, (modelGridExtent / 2) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32(-(modelGridExtent / 2) * 100, 0, (gridLineIndex * 100) - ((modelGridExtent / 2) * 100));
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32((modelGridExtent / 2) * 100, 0, (gridLineIndex * 100) - ((modelGridExtent / 2) * 100));
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
    }
    GXBegin(GX_LINES, GX_VTXFMT0, (modelGridExtent + 1) * 4);
    for (gridLineIndex = 0; gridLineIndex <= modelGridExtent; gridLineIndex++) {
        lineColorRed = lineColorGreen = lineColorBlue = 255;
        GXPosition3f32((gridLineIndex * 100) - ((modelGridExtent / 2) * 100), modelGridExtent * 100, (-(modelGridExtent / 2)) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32((gridLineIndex * 100) - ((modelGridExtent / 2) * 100), 0, -(modelGridExtent / 2) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32(-(modelGridExtent / 2) * 100, gridLineIndex * 100, -(modelGridExtent / 2) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32((modelGridExtent / 2) * 100, gridLineIndex * 100, -(modelGridExtent / 2) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
    }
    GXBegin(GX_LINES, GX_VTXFMT0, (modelGridExtent + 1) * 4);
    for (gridLineIndex = 0; gridLineIndex <= modelGridExtent; gridLineIndex++) {
        lineColorRed = lineColorGreen = lineColorBlue = 255;
        GXPosition3f32((gridLineIndex * 100) - ((modelGridExtent / 2) * 100), modelGridExtent * 100, (modelGridExtent / 2) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32((gridLineIndex * 100) - ((modelGridExtent / 2) * 100), 0, (modelGridExtent / 2) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32(-(modelGridExtent / 2) * 100, gridLineIndex * 100, (modelGridExtent / 2) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32((modelGridExtent / 2) * 100, gridLineIndex * 100, (modelGridExtent / 2) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
    }
    GXBegin(GX_LINES, GX_VTXFMT0, (modelGridExtent + 1) * 4);
    for (gridLineIndex = 0; gridLineIndex <= modelGridExtent; gridLineIndex++) {
        lineColorRed = lineColorGreen = lineColorBlue = 255;
        GXPosition3f32((modelGridExtent / 2) * 100, modelGridExtent * 100, (gridLineIndex * 100) - ((modelGridExtent / 2) * 100));
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32((modelGridExtent / 2) * 100, 0, (gridLineIndex * 100) - ((modelGridExtent / 2) * 100));
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32((modelGridExtent / 2) * 100, gridLineIndex * 100, -(modelGridExtent / 2) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32((modelGridExtent / 2) * 100, gridLineIndex * 100, (modelGridExtent / 2) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
    }
    GXBegin(GX_LINES, GX_VTXFMT0, (modelGridExtent + 1) * 4);
    for (gridLineIndex = 0; gridLineIndex <= modelGridExtent; gridLineIndex++) {
        lineColorRed = lineColorGreen = lineColorBlue = 255;
        GXPosition3f32(-(modelGridExtent / 2) * 100, modelGridExtent * 100, (gridLineIndex * 100) - ((modelGridExtent / 2) * 100));
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32(-(modelGridExtent / 2) * 100, 0, (gridLineIndex * 100) - ((modelGridExtent / 2) * 100));
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32(-(modelGridExtent / 2) * 100, gridLineIndex * 100, -(modelGridExtent / 2) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
        GXPosition3f32(-(modelGridExtent / 2) * 100, gridLineIndex * 100, (modelGridExtent / 2) * 100);
        GXColor3u8(lineColorRed, lineColorGreen, lineColorBlue);
    }
}

s16 musicSequenceIds[] = { 1, 3, 2, 13, 14, 15, 16, 68, -1 };

s16 soundEffectIds[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, -1 };

void AudioPlaybackTestLoop(void)
{
    s16 musicIndex = 0;
    s16 soundEffectIndex = 0;
    s16 statusDisplayFrames = 0;
    s16 sequenceId;
    Vec soundPosition = { 0, 0, 0 };
    while (1) {
        audioEmitterId = HuAudFXEmiterPlay(1282, &soundPosition);
        HuPrcSleep(60);
        if (HuPadBtnDown[1] & PAD_BUTTON_A) {
            HuAudSeqFadeOut(sequenceId, 60);
            sequenceId = HuAudSeqPlay(musicSequenceIds[musicIndex]);
            musicIndex++;
            if (musicSequenceIds[musicIndex] == -1) {
                musicIndex = 0;
            }
            statusDisplayFrames = 30;
        }
        if (HuPadBtnDown[1] & PAD_BUTTON_B) {
            HuAudFXPlay(soundEffectIds[soundEffectIndex]);
            soundEffectIndex++;
            if (soundEffectIds[soundEffectIndex] == -1) {
                soundEffectIndex = 0;
            }
            statusDisplayFrames = 30;
        }
        if (statusDisplayFrames) {
            statusDisplayFrames--;
            print8(16, 430, 2.0f, "MusicNo:%d SENo:%d", musicIndex, soundEffectIndex);
        }
        HuPrcVSleep();
    }
}

s16 previousButtonState = -1;

u32 DebouncedButtonRead(void)
{
    s16 buttonState;
    buttonState = HuPadBtn[0];
    if (buttonRepeatFrames) {
        buttonRepeatFrames--;
        if (!buttonState) {
            buttonRepeatFrames = 0;
        }
        else {
            buttonState = 0;
        }
    }
    else if (buttonState) {
        if (previousButtonState == buttonState) {
            buttonRepeatFrames = 3;
        }
        else {
            buttonRepeatFrames = 20;
        }
        previousButtonState = buttonState;
    }
    else {
        previousButtonState = 0;
    }
    return buttonState;
}

void PauseAndZoomControlLoop(void)
{
    u32 buttonState;
    HUPROCESS *currentProcess;
    currentProcess = HuPrcCurrentGet();
    HuPrcSetStat(currentProcess, 0xC);
    while (1) {
        buttonState = DebouncedButtonRead();
        if (HuPadBtn[0] & PAD_BUTTON_RIGHT) {
            while (1) {
                buttonState = DebouncedButtonRead();
                if (HuPadBtn[0] & PAD_BUTTON_LEFT) {
                    break;
                }
                if (!buttonState) {
                    HuPrcAllPause(1);
                    Hu3DPauseSet(1);
                }
                else {
                    HuPrcAllPause(0);
                    Hu3DPauseSet(0);
                }
                HuPrcVSleep();
            }
            HuPrcAllPause(0);
            Hu3DPauseSet(0);
        }
        if (buttonState & PAD_BUTTON_UP) {
            if (cameraFieldOfView < 120.0f) {
                cameraFieldOfView++;
            }
        }
        if (buttonState & PAD_BUTTON_DOWN) {
            if (cameraFieldOfView > 5.0f) {
                cameraFieldOfView--;
            }
        }
        HuPrcVSleep();
    }
}
