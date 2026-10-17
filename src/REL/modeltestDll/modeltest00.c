#include "game/chrman.h"
#include "game/pad.h"
#include "game/wipe.h"
#include "math.h"


#include "REL/modeltestDll.h"

// -------------------------------------------------------------------------- //

OMOBJ *unreferencedObjectScratch;
OMOBJ *cameraUpdateObject;
OMOBJ *cameraControlObject;
s16 characterModelIds[4];
s16 characterMotionIds[4][8];
HUPROCESS *characterModelProcess;
u16 cameraIds[4] = { 1, 2, 4, 8 };

// -------------------------------------------------------------------------- //

void MultiCameraModelTestInitialize()
{
    HUPROCESS *objectManager;
    s16 cameraIndex;

    OSReport("******* MODELTESTObjectSetup *********\n");

    objectManager = omInitObjMan(0x32, 0x2000);
    CRot.x = -22.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    Center.x = 0.0f;
    Center.y = 60.0f;
    Center.z = 0.0f;
    CZoom = 400.0f;

    for (cameraIndex = 0; cameraIndex < 4; ++cameraIndex) {
        Hu3DCameraCreate(cameraIds[cameraIndex]);
        Hu3DCameraPerspectiveSet(cameraIds[cameraIndex], 45.0f, 20.0f, 20000.0f, 1.2f);
        Hu3DCameraViewportSet(cameraIds[cameraIndex], (cameraIndex & 1) * 320, ((cameraIndex & 2) != 0) ? 0xF0 : 0, 320.0f, 240.0f, 0.0f, 1.0f);
        Hu3DCameraScissorSet(cameraIds[cameraIndex], (cameraIndex & 1) * 320, ((cameraIndex & 2) != 0) ? 0xF0 : 0, 320, 240);
    }

    cameraControlObject = omAddObjEx(objectManager, 0, 0x20U, 0x20U, -1, MultiCameraModelTestUpdate);
    cameraUpdateObject = omAddObjEx(objectManager, 0x7FDA, 0U, 0U, -1, MultiCameraViewUpdate);
    characterModelProcess = HuPrcCreate(MultiCameraCharacterModelsInitialize, 0x64U, 0x2000U, 0);

    Hu3DBGColorSet(0x20U, 0x80U, 0x80U);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
}

// -------------------------------------------------------------------------- //

static inline void HuControlCamCenter(Vec *cameraRightDirection, Vec *normalizedCameraDirection)
{
    s8 stickInput;

    stickInput = HuPadSubStkX[0] & 0xF8;
    if (stickInput != 0) {
        Center.x += 0.05f * (normalizedCameraDirection->x * stickInput);
        Center.y += 0.05f * (normalizedCameraDirection->y * stickInput);
        Center.z += 0.05f * (normalizedCameraDirection->z * stickInput);
    }
    VECNormalize(cameraRightDirection, normalizedCameraDirection);
    stickInput = -(HuPadSubStkY[0] & 0xF8);
    if (stickInput != 0) {
        Center.x += 0.05f * (normalizedCameraDirection->x * stickInput);
        Center.y += 0.05f * (normalizedCameraDirection->y * stickInput);
        Center.z += 0.05f * (normalizedCameraDirection->z * stickInput);
    }
}

// -------------------------------------------------------------------------- //

void MultiCameraModelTestUpdate(OMOBJ *object)
{
    Vec cameraPosition;
    Vec cameraOffset;
    Vec cameraUpDirection;
    Vec cameraRightDirection;
    float cameraRoll;

    if ((HuPadBtnDown[0] & PAD_BUTTON_START) != 0) {
        if (characterModelProcess) {
            HuPrcKill(characterModelProcess);
        }
        omOvlReturnEx(1, 1);
        return;
    }

    if ((HuPadBtn[0] & PAD_BUTTON_UP) != 0) {
        CRot.x += 2.0f;
    }
    if ((HuPadBtn[0] & PAD_BUTTON_DOWN) != 0) {
        CRot.x -= 2.0f;
    }
    if ((HuPadBtn[0] & PAD_BUTTON_LEFT) != 0) {
        CRot.y += 2.0f;
    }
    if ((HuPadBtn[0] & PAD_BUTTON_RIGHT) != 0) {
        CRot.y -= 2.0f;
    }

    CZoom += HuPadTrigL[0] / 2;
    CZoom -= HuPadTrigR[0] / 2;

    cameraPosition.x = Center.x + CZoom * (sind(CRot.y) * cosd(CRot.x));
    cameraPosition.y = Center.y + CZoom * -sind(CRot.x);
    cameraPosition.z = Center.z + CZoom * (cosd(CRot.y) * cosd(CRot.x));
    cameraOffset.x = Center.x - cameraPosition.x;
    cameraOffset.y = Center.y - cameraPosition.y;
    cameraOffset.z = Center.z - cameraPosition.z;
    cameraUpDirection.x = sind(CRot.y) * sind(CRot.x);
    cameraUpDirection.y = cosd(CRot.x);
    cameraUpDirection.z = cosd(CRot.y) * sind(CRot.x);
    cameraRoll = CRot.z;

    cameraRightDirection.x = cameraUpDirection.x * (cameraOffset.x * cameraOffset.x + (1.0f - cameraOffset.x * cameraOffset.x) * cosd(cameraRoll))
        + cameraUpDirection.y * (cameraOffset.x * cameraOffset.y * (1.0f - cosd(cameraRoll)) - cameraOffset.z * sind(cameraRoll))
        + cameraUpDirection.z * (cameraOffset.x * cameraOffset.z * (1.0f - cosd(cameraRoll)) + cameraOffset.y * sind(cameraRoll));

    cameraRightDirection.y = cameraUpDirection.y * (cameraOffset.y * cameraOffset.y + (1.0f - cameraOffset.y * cameraOffset.y) * cosd(cameraRoll))
        + cameraUpDirection.x * (cameraOffset.x * cameraOffset.y * (1.0f - cosd(cameraRoll)) + cameraOffset.z * sind(cameraRoll))
        + cameraUpDirection.z * (cameraOffset.y * cameraOffset.z * (1.0f - cosd(cameraRoll)) - cameraOffset.x * sind(cameraRoll));

    cameraRightDirection.z = cameraUpDirection.z * (cameraOffset.z * cameraOffset.z + (1.0f - cameraOffset.z * cameraOffset.z) * cosd(cameraRoll))
        + (cameraUpDirection.x * (cameraOffset.x * cameraOffset.z * (1.0 - cosd(cameraRoll)) - cameraOffset.y * sind(cameraRoll))
            + cameraUpDirection.y * (cameraOffset.y * cameraOffset.z * (1.0 - cosd(cameraRoll)) + cameraOffset.x * sind(cameraRoll)));

    VECCrossProduct(&cameraUpDirection, &cameraOffset, &cameraOffset);
    VECNormalize(&cameraOffset, &cameraOffset);

    HuControlCamCenter(&cameraRightDirection, &cameraOffset);
}

// -------------------------------------------------------------------------- //

void MultiCameraViewUpdate(OMOBJ *object)
{
    s16 cameraIndex;
    f32 cameraYaw;
    f32 cameraPitch;
    f32 cameraRoll;
    Vec cameraPosition;
    Vec cameraTarget;
    Vec cameraUpDirection;
    for (cameraIndex = 0; cameraIndex < 4; ++cameraIndex) {
        cameraPitch = CRot.x;
        cameraYaw = CRot.y + (cameraIndex * 90);
        cameraRoll = CRot.z;
        cameraPosition.x = Center.x + CZoom * (sind(cameraYaw) * (cosd(cameraPitch)));
        cameraPosition.y = Center.y + CZoom * -sind(cameraPitch);
        cameraPosition.z = Center.z + CZoom * (cosd(cameraYaw) * (cosd(cameraPitch)));

        cameraTarget.x = Center.x;
        cameraTarget.y = Center.y;
        cameraTarget.z = Center.z;

        cameraUpDirection.x = sind(cameraYaw) * (sind(cameraPitch));
        cameraUpDirection.y = cosd(cameraPitch);
        cameraUpDirection.z = cosd(cameraYaw) * (sind(cameraPitch));

        Hu3DCameraPosSet(cameraIds[cameraIndex], cameraPosition.x, cameraPosition.y, cameraPosition.z, cameraUpDirection.x, cameraUpDirection.y, cameraUpDirection.z, cameraTarget.x, cameraTarget.y, cameraTarget.z);
    }
}

// -------------------------------------------------------------------------- //

const Vec zeroVector = { 0.0f, 0.0f, 0.0f };
const Vec worldUpVector = { 0.0f, 1.0f, 0.0f };

// -------------------------------------------------------------------------- //

void MultiCameraCharacterModelsInitialize(void)
{
    Vec shadowPosition;
    Vec shadowTarget;
    Vec shadowUpDirection;
    s16 characterIndex;
    s16 characterId;

    shadowTarget = zeroVector;
    shadowUpDirection = worldUpVector;

    Hu3DGLightCreate(0.0f, 500.0f, 1000.0f, 0.0f, -0.5f, -1.0f, 0xFFU, 0xFFU, 0xFFU);
    Hu3DShadowCreate(45.0f, 500.0f, 8000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowSizeSet(0xC0U);

    shadowPosition.x = -500.0f;
    shadowPosition.y = 1000.0f;
    shadowPosition.z = 1000.0f;
    Hu3DShadowPosSet(&shadowPosition, &shadowUpDirection, &shadowTarget);

    for (characterIndex = 0; characterIndex < 4; ++characterIndex) {
        characterId = characterIndex;
        characterModelIds[characterIndex] = CharModelCreate(characterId, 8);
        Hu3DModelShadowSet(characterModelIds[characterIndex]);
        characterMotionIds[characterIndex][0] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02));
        CharMotionSet(characterId, characterMotionIds[characterIndex][0]);
        Hu3DModelAttrSet(characterModelIds[characterIndex], HU3D_MOTATTR_LOOP);
        Hu3DModelCameraSet(characterModelIds[characterIndex], cameraIds[characterIndex]);
        Hu3DModelPosSet(characterModelIds[characterIndex], (((characterIndex / 4) * 200) - 100), 0.0f, (-(characterIndex % 4) * 150));
        Hu3DModelPosSet(characterModelIds[characterIndex], 0.0f, 0.0f, 0.0f);
        CharMotionDataClose(characterId);
    }

    characterIndex = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_SAF, 0x24));
    Hu3DModelScaleSet(characterIndex, 5.0f, 5.0f, 5.0f);
    Hu3DModelShadowMapSet(characterIndex);

    while (TRUE) {
        if ((HuPadBtnDown[0] & PAD_BUTTON_Y) != 0) {
            Hu3DCameraKill(2);
            Hu3DCameraKill(4);
            Hu3DCameraKill(8);
            shadowPosition.x = shadowPosition.z = shadowPosition.y = 0.0f;
            shadowPosition.y += 200.0f;
            CharEffectCoinGlowCreate(4, &shadowPosition);
        }
        HuPrcVSleep();
    }
}

// -------------------------------------------------------------------------- //

// -------------------------------------------------------------------------- //
