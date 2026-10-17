#define HUSPR_USE_OLD_DEFS
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/hsfex.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#include "REL/modeltestDll.h"

// -------------------------------------------------------------------------- //

s32 characterMotionAssetIds[8] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x1B),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x15),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x16),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x06),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x04),
};

OMOBJ *modelTestObject;
OMOBJ *modelTestViewObject;

// -------------------------------------------------------------------------- //

void ObjectSetup(void)
{
    HUPROCESS *prc;
    OSReport("******* MODELTESTObjectSetup *********\n");

    prc = omInitObjMan(0x32, 0x2000);
    omGameSysInit(prc);

    CRot.x = -30.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    Center.x = 0.0f;
    Center.y = 0.0f;
    Center.z = 0.0f;
    CZoom = 800.0f;

    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 45.0f, 20.0f, 20000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);

    modelTestObject = omAddObjEx(prc, 0, 0x20, 0x20, -1, ModelTestSceneInitialize);
    modelTestViewObject = omAddObjEx(prc, 0x7FDA, 0, 0, -1, omOutView);

    Hu3DBGColorSet(0x20U, 0x80U, 0x80U);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
}

// -------------------------------------------------------------------------- //

char *characterObjectNames[16] = { "target1A", "target1B", "target2A", "target2B", "target3A", "target3B", "target4A", "target4B", "target5A", "target5B",
    "target6A", "target6B", "target7A", "target7B", "target8A", "target8B" };

HU3DPARMANPARAM particleRotationParameters = {
    /* unk00 */ 40,
    /* unk02 */ { 0, 0 },
    /* unk04 */ 50.0f,
    /* unk08 */ 0.0f,
    /* unk0C */ 90.0f,
    /* unk10 */ { /* x */ 0.0f, /* y */ -0.05000000074505806f, /* z */ 0.0f },
    /* unk1C */ 50.0f,
    /* unk20 */ 0.800000011920929f,
    /* unk24 */ 20.0f,
    /* unk28 */ 0.9900000095367432f,
    /* unk2C */ 4,
    /* unk2E */
    { { /* r */ 255, /* g */ 255, /* b */ 32, /* a */ 255 }, { /* r */ 255, /* g */ 255, /* b */ 0, /* a */ 255 },
        { /* r */ 255, /* g */ 255, /* b */ 128, /* a */ 255 }, { /* r */ 255, /* g */ 255, /* b */ 255, /* a */ 255 } },
    /* unk3E */
    { { /* r */ 255, /* g */ 255, /* b */ 255, /* a */ 0 }, { /* r */ 255, /* g */ 255, /* b */ 255, /* a */ 0 },
        { /* r */ 255, /* g */ 255, /* b */ 255, /* a */ 0 }, { /* r */ 255, /* g */ 255, /* b */ 255, /* a */ 0 } },
};

HU3DPARMANPARAM particleRingParameters[2] = { {
                                          /* unk00 */ 50,
                                          /* unk02 */ { 0, 0 },
                                          /* unk04 */ 5.0f,
                                          /* unk08 */ 10.0f,
                                          /* unk0C */ 10.0f,
                                          /* unk10 */ { /* x */ 0.0f, /* y */ 0.0f, /* z */ 0.0f },
                                          /* unk1C */ 2.0f,
                                          /* unk20 */ 1.0099999904632568f,
                                          /* unk24 */ 30.0f,
                                          /* unk28 */ 0.9900000095367432f,
                                          /* unk2C */ 2,
                                          /* unk2E */
                                          { { /* r */ 48, /* g */ 48, /* b */ 8, /* a */ 255 }, { /* r */ 64, /* g */ 48, /* b */ 8, /* a */ 255 },
                                              { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 }, { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 } },
                                          /* unk3E */
                                          { { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 }, { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 },
                                              { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 }, { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 } },
                                      },
    {
        /* unk00 */ 30,
        /* unk02 */ { 0, 0 },
        /* unk04 */ 3.299999952316284f,
        /* unk08 */ 10.0f,
        /* unk0C */ 0.0f,
        /* unk10 */ { /* x */ 0.0f, /* y */ -0.05000000074505806f, /* z */ 0.0f },
        /* unk1C */ 2.0f,
        /* unk20 */ 1.0f,
        /* unk24 */ 20.0f,
        /* unk28 */ 0.9800000190734863f,
        /* unk2C */ 2,
        /* unk2E */
        { { /* r */ 255, /* g */ 255, /* b */ 255, /* a */ 255 }, { /* r */ 255, /* g */ 255, /* b */ 64, /* a */ 255 },
            { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 }, { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 } },
        /* unk3E */
        { { /* r */ 255, /* g */ 128, /* b */ 128, /* a */ 0 }, { /* r */ 255, /* g */ 64, /* b */ 32, /* a */ 0 },
            { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 }, { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 } },
    } };

HU3DPARMANPARAM particleEmissionParameters = {
    /* unk00 */ 40,
    /* unk02 */ { 0, 0 },
    /* unk04 */ 50.0f,
    /* unk08 */ 0.0f,
    /* unk0C */ 180.0f,
    /* unk10 */ { /* x */ 0.0f, /* y */ 0.0f, /* z */ 0.0f },
    /* unk1C */ 30.0f,
    /* unk20 */ 0.8999999761581421f,
    /* unk24 */ 20.0f,
    /* unk28 */ 0.9900000095367432f,
    /* unk2C */ 4,
    /* unk2E */
    { { /* r */ 255, /* g */ 255, /* b */ 32, /* a */ 255 }, { /* r */ 255, /* g */ 255, /* b */ 0, /* a */ 255 },
        { /* r */ 255, /* g */ 255, /* b */ 128, /* a */ 255 }, { /* r */ 255, /* g */ 255, /* b */ 255, /* a */ 255 } },
    /* unk3E */
    { { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 }, { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 }, { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 },
        { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 } },
};

HU3DPARMANPARAM particleTrailParameters = {
    /* unk00 */ 30,
    /* unk02 */ { 0, 0 },
    /* unk04 */ 50.0f,
    /* unk08 */ 0.0f,
    /* unk0C */ 90.0f,
    /* unk10 */ { /* x */ 0.0f, /* y */ 0.0f, /* z */ 0.0f },
    /* unk1C */ 30.0f,
    /* unk20 */ 0.949999988079071f,
    /* unk24 */ 20.0f,
    /* unk28 */ 0.9900000095367432f,
    /* unk2C */ 2,
    /* unk2E */
    { { /* r */ 255, /* g */ 16, /* b */ 16, /* a */ 255 }, { /* r */ 255, /* g */ 255, /* b */ 16, /* a */ 255 },
        { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 }, { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 } },
    /* unk3E */
    { { /* r */ 255, /* g */ 16, /* b */ 16, /* a */ 255 }, { /* r */ 255, /* g */ 255, /* b */ 16, /* a */ 255 },
        { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 }, { /* r */ 0, /* g */ 0, /* b */ 0, /* a */ 0 } },
};

const Vec zeroVector = { 0.0f, 0.0f, 0.0f };
const Vec worldUpVector = { 0.0f, 1.0f, 0.0f };
s16 spriteGroupId;
s16 lastSpriteId;
s16 characterModelIds[8];
s16 characterMotionIds[8][16];
s16 particleManagerId;
s16 unreferencedParticleManagerScratch;
s16 linkedParticleManagerId;
s16 ringParticleManagerIds[16];
s16 shadowModelIds[256];
s16 modelIds[258];
s16 sequenceIds[3];
s16 sequenceTimerFrames;
ANIMDATA *screenSpriteAnimation;
s32 unreferencedAnimationScratch;
ObjectTransformWork objectTransforms[20];
void *framebufferBuffers[4];
u8 spriteTextureLoadCount;
f32 textureRotationDegrees;
s16 teropPatternIndex;
f32 characterRotationDegrees;
HUPROCESS *effectProcesses[2];
HUPROCESS *modelMotionProcess;
HUPROCESS *characterMotionProcess;
s32 testMode;
f32 effectRotationDegrees;
static u8 unreferencedScratchBytes[8];

// -------------------------------------------------------------------------- //

void ModelTestSceneInitialize(OMOBJ *object)
{
    Vec shadowPosition;
    Vec shadowUpDirection;
    Vec shadowTargetPosition;
    Mtx objectTransform;
    s16 modelIndex;
    s16 jointMotionId;
    u32 startTick;
    s16 characterId;
    void *assetBuffer;
    ANIMDATA *spriteAnimation;
    f32 elapsedMicroseconds;
    f32 unreferencedTimingScratch;
    s16 characterIndex;
    s32 unreferencedLoopScratch;

    shadowUpDirection = zeroVector;
    shadowTargetPosition = worldUpVector;

    modelIndex = Hu3DGLightCreate(0.0f, 500.0f, 1000.0f, 0.0f, -0.5f, -1.0f, 0xFFU, 0xFFU, 0xFFU);
    Hu3DGLightInfinitytSet(modelIndex);
    Hu3DShadowCreate(45.0f, 500.0f, 8000.0f);
    Hu3DShadowTPLvlSet(0.5f);

    Hu3DShadowSizeSet(0xC0U);
    shadowPosition.x = -500.0f;
    shadowPosition.y = 1000.0f;
    shadowPosition.z = 1000.0f;
    Hu3DShadowPosSet(&shadowPosition, &shadowTargetPosition, &shadowUpDirection);

    for (modelIndex = 0; modelIndex < 1; ++modelIndex) {
        characterIndex = 0;
        characterModelIds[modelIndex] = CharModelCreate(characterIndex, 2);
        if (modelIndex == 1) {
            Hu3DModelLayerSet(characterModelIds[modelIndex], 1);
        }

        characterMotionIds[modelIndex][0] = CharMotionCreate(characterIndex, characterMotionAssetIds[0]);
        characterMotionIds[modelIndex][1] = CharMotionCreate(characterIndex, characterMotionAssetIds[1]);
        characterMotionIds[modelIndex][2] = CharMotionCreate(characterIndex, characterMotionAssetIds[2]);
        characterMotionIds[modelIndex][3] = CharMotionCreate(characterIndex, characterMotionAssetIds[3]);
        characterMotionIds[modelIndex][4] = CharMotionCreate(characterIndex, characterMotionAssetIds[4]);
        characterMotionIds[modelIndex][5] = CharMotionCreate(characterIndex, characterMotionAssetIds[5]);
        characterMotionIds[modelIndex][6] = CharMotionCreate(characterIndex, characterMotionAssetIds[6]);
        characterMotionIds[modelIndex][7] = CharMotionCreate(characterIndex, characterMotionAssetIds[7]);
        characterMotionIds[modelIndex][0] = CharMotionCreate(characterIndex, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x4D));
        CharMotionSet(characterIndex, characterMotionIds[modelIndex][0]);
        Hu3DModelAttrSet(characterModelIds[modelIndex], HU3D_MOTATTR_LOOP);
        Hu3DModelPosSet(characterModelIds[modelIndex], (modelIndex / 4 * 0xC8) - 100, 0.0f, -(modelIndex % 4) * 0x96);
        CharMotionDataClose(characterIndex);
    }
    startTick = OSGetTick();

    for (modelIndex = 0; modelIndex < 8; ++modelIndex) {
        Hu3DModelObjMtxGet(characterModelIds[0], "test11_tex_we-ske_R_shoe1", objectTransform);
    }

    elapsedMicroseconds = OSTicksToMicroseconds(OSGetTick() - startTick);
    OSReport("time %f\n", US_TO_60TH_SEC(elapsedMicroseconds));
    modelIndex = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_SAF, 0x24));
    Hu3DModelScaleSet(modelIndex, 5.0f, 5.0f, 5.0f);
    Hu3DModelShadowMapSet(modelIndex);
    modelIndex = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M407, 0x1E));
    jointMotionId = Hu3DJointMotion(modelIndex, HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M407, 0x2F), HU_MEMNUM_OVL, 2));
    Hu3DMotionSet(modelIndex, jointMotionId);
    Hu3DModelAttrSet(modelIndex, HU3D_MOTATTR_LOOP);

    for (modelIndex = 0; modelIndex < 10; ++modelIndex) {
        if (modelIndex == 0) {
            shadowModelIds[modelIndex] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_SAF, 0x19));
        }
        else {
            shadowModelIds[modelIndex] = Hu3DModelLink(shadowModelIds[0]);
        }
        Hu3DModelShadowSet(shadowModelIds[modelIndex]);
        Hu3DModelPosSet(shadowModelIds[modelIndex], (((modelIndex / 5) * 0x1F4) + 0xC8), 0.0f, (-(modelIndex % 5) * 0x12C));
        Hu3DModelAttrSet(shadowModelIds[modelIndex], HU3D_ATTR_NOCULL);
        Hu3DModelAttrSet(shadowModelIds[modelIndex], HU3D_MOTATTR_LOOP);
    }
    assetBuffer = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_SAF, 0x13), HU_MEMNUM_OVL, 2);
    spriteAnimation = HuSprAnimRead(assetBuffer);
    spriteGroupId = HuSprGrpCreate(0x14);

    for (modelIndex = 0; modelIndex < 0x14; ++modelIndex) {
        lastSpriteId = HuSprCreate(spriteAnimation, 0, 0);
        HuSprGrpMemberSet(spriteGroupId, modelIndex, lastSpriteId);
        HuSprPosSet(spriteGroupId, modelIndex, 640.0f, 240.0f);
    }

    HuSprExecLayerSet(0x40, 1);
    HuSprGrpDrawNoSet(spriteGroupId, 0x40);
    sequenceTimerFrames = 0x293;
    sequenceIds[0] = MGSeqTimerCreate(sequenceTimerFrames / 60);
    assetBuffer = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 0x01), HU_MEMNUM_OVL, 2);
    spriteAnimation = HuSprAnimRead(assetBuffer);
    particleManagerId = Hu3DParManCreate(spriteAnimation, 0x3E8, &particleEmissionParameters);
    Hu3DParManAttrSet(particleManagerId, 0x45);
    Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(particleManagerId), 1U);
    Hu3DParManRotSet(particleManagerId, 0.0f, 0.0f, 0.0f);
    Hu3DParManVacumeSet(particleManagerId, 500.0f, 0.0f, 0.0f, 1.0f);
    linkedParticleManagerId = Hu3DParManLink(particleManagerId, &particleTrailParameters);
    Hu3DParManAttrSet(linkedParticleManagerId, 0x143);
    Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(linkedParticleManagerId), 1U);
    Hu3DParManVecSet(linkedParticleManagerId, 0.0f, 1.0f, 0.0f);
    Hu3DParManVacumeSet(linkedParticleManagerId, 500.0f, 0.0f, 0.0f, 1.0f);
    Hu3DParManColorSet(linkedParticleManagerId, 0);
    assetBuffer = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 0x02), HU_MEMNUM_OVL, 2);
    spriteAnimation = HuSprAnimRead(assetBuffer);

    for (modelIndex = 0; modelIndex < 10; ++modelIndex) {
        ringParticleManagerIds[modelIndex] = Hu3DParManCreate(spriteAnimation, 0x1F4, &particleRingParameters[0]);
        Hu3DParManAttrSet(ringParticleManagerIds[modelIndex], 0x864);
        Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(ringParticleManagerIds[modelIndex]), 1U);
        Hu3DParManPosSet(ringParticleManagerIds[modelIndex], (100.0 * sind(36.0f * modelIndex)), 0.0f, 100.0 * cosd(36.0f * modelIndex));
        Hu3DParManRotSet(ringParticleManagerIds[modelIndex], -90.0f, 0.0f, 0.0f);
    }

    testMode = 0;
    Hu3DFogSet(5000.0f, 10000.0f, 0x20U, 0x10U, 0x30U);
    object->objFunc = ModelTestSceneUpdate;
}

// -------------------------------------------------------------------------- //

s32 unrefData[] = { 0, 0, 0 };

// -------------------------------------------------------------------------- //

void ModelTestMotionRotationLoop(void)
{
    f32 modelRotationY;
    modelRotationY = 0.0f;

    while (TRUE) {
        Hu3DModelRotSet(modelIds[0], 0.0f, modelRotationY, 0.0f);
        modelRotationY += 1.0f;
        HuPrcVSleep();
    }
}

// -------------------------------------------------------------------------- //

static inline void HuControlCamCenter(Vec *cameraRightDirection, Vec *normalizedCameraDirection)
{
    s8 substickDelta;

    substickDelta = HuPadSubStkX[0] & 0xF8;
    if (substickDelta != 0) {
        Center.x += 0.05f * (normalizedCameraDirection->x * substickDelta);
        Center.y += 0.05f * (normalizedCameraDirection->y * substickDelta);
        Center.z += 0.05f * (normalizedCameraDirection->z * substickDelta);
    }
    VECNormalize(cameraRightDirection, normalizedCameraDirection);
    substickDelta = -(HuPadSubStkY[0] & 0xF8);
    if (substickDelta != 0) {
        Center.x += 0.05f * (normalizedCameraDirection->x * substickDelta);
        Center.y += 0.05f * (normalizedCameraDirection->y * substickDelta);
        Center.z += 0.05f * (normalizedCameraDirection->z * substickDelta);
    }
}

// -------------------------------------------------------------------------- //

const TeropPatternSequence teropPatternSequenceIds = { { 8, 6, 4, 7, 9, 10, 11 } };

// -------------------------------------------------------------------------- //

void ModelTestSceneUpdate(struct omObj_s *object)
{
    Vec cameraPosition;
    Vec cameraOffset;
    Vec cameraUpDirection;
    Vec cameraRightDirection;
    f32 cameraRoll;
    s32 unreferencedCameraScratch;
    s16 loopIndex;
    TeropPatternSequence teropPatterns;

    if (omSysExitReq != 0) {
        if (characterMotionProcess != 0) {
            HuPrcKill(characterMotionProcess);
        }
        if (modelMotionProcess != 0) {
            HuPrcKill(modelMotionProcess);
        }
        if (effectProcesses[0] != 0) {
            HuPrcKill(effectProcesses[0]);
        }
        CharModelKill(-1);
        omOvlReturnEx(1, 1);
        return;
    }

    CRot.y += 0.1f * HuPadStkX[0];
    CRot.x += 0.1f * HuPadStkY[0];
    CZoom += HuPadTrigL[0] / 2;
    CZoom -= HuPadTrigR[0] / 2;

    if (((HuPadBtn[0] & PAD_BUTTON_LEFT) != 0) && (particleRotationParameters.angleRange < 360.0f)) {
        particleRotationParameters.angleRange += 1.0f;
    }

    if (((HuPadBtn[0] & PAD_BUTTON_RIGHT) != 0) && (particleRotationParameters.angleRange > 0.0f)) {
        particleRotationParameters.angleRange -= 1.0f;
    }

    if ((HuPadBtnDown[0] & PAD_BUTTON_X) != 0) {
        Hu3DParManAttrSet(particleManagerId, 0x81);
        Hu3DParManAttrSet(linkedParticleManagerId, 0x81);
    }

    characterRotationDegrees += 2.0f;
    if (characterRotationDegrees > 360.0f) {
        characterRotationDegrees -= 360.0f;
    }

    for (loopIndex = 0; loopIndex < 10; ++loopIndex) {
        f32 unreferencedLoopScratch;
        s32 unreferencedLoopScratch2;
        s32 unreferencedLoopScratch3;

        while (0) {
            (void)unreferencedLoopScratch;
            (void)unreferencedLoopScratch;
            (void)unreferencedCameraScratch;
            (void)unreferencedCameraScratch;
            (void)unreferencedCameraScratch;
            (void)unreferencedCameraScratch;
            (void)unreferencedCameraScratch;
            (void)unreferencedCameraScratch;
            (void)unreferencedCameraScratch;
            (void)unreferencedCameraScratch;
            (void)unreferencedCameraScratch;
            (void)unreferencedCameraScratch;
            (void)loopIndex;
            (void)loopIndex;
            (void)loopIndex;
            (void)unreferencedLoopScratch2;
            (void)unreferencedLoopScratch2;
            (void)unreferencedLoopScratch2;
            (void)unreferencedLoopScratch2;
            (void)unreferencedLoopScratch2;
            (void)unreferencedLoopScratch2;
            (void)unreferencedLoopScratch2;
            (void)unreferencedLoopScratch2;
            (void)unreferencedLoopScratch2;
            (void)unreferencedLoopScratch2;
            (void)unreferencedLoopScratch3;
            (void)unreferencedLoopScratch3;
            (void)unreferencedLoopScratch3;
            (void)unreferencedLoopScratch3;
            (void)unreferencedLoopScratch3;
            (void)unreferencedLoopScratch3;
            (void)unreferencedLoopScratch3;
            (void)unreferencedLoopScratch3;
            (void)unreferencedLoopScratch3;
        }
    }

    if (sequenceTimerFrames > 0) {
        MGSeqParamSet(sequenceIds[0], 1, sequenceTimerFrames / 60);
        --sequenceTimerFrames;
        if (sequenceTimerFrames <= 0) {
            MGSeqParamSet(sequenceIds[0], 2, -1);
        }
    }

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

    if (testMode == 0) {
        HuControlCamCenter(&cameraRightDirection, &cameraOffset);
    }

    teropPatterns = teropPatternSequenceIds;

    if ((HuPadBtnDown[0] & PAD_BUTTON_X) != 0) {
        ++teropPatternIndex;
        if (teropPatternIndex >= 7) {
            teropPatternIndex = 0;
        }
    }

    if ((HuPadBtnDown[0] & PAD_BUTTON_B) != 0) {
        loopIndex = MGSeqStartCreateType(teropPatterns.sequenceIds[teropPatternIndex]);
    }
    if ((HuPadBtnDown[0] & PAD_BUTTON_A) != 0) {
        loopIndex = MGSeqDrawCreateType(teropPatterns.sequenceIds[teropPatternIndex]);
    }
    if ((HuPadBtnDown[0] & PAD_BUTTON_Y) != 0) {
        loopIndex = MGSeqRecordCreate(59);
    }

    if ((HuPadBtnDown[0] & (PAD_BUTTON_LEFT | PAD_BUTTON_RIGHT | PAD_BUTTON_UP | PAD_BUTTON_DOWN)) != 0) {
        if ((HuPadBtnDown[0] & PAD_BUTTON_UP) != 0) {
            MGSeqChampionCreate(GWPlayerCfg[0].character, -1, -1, -1);
        }
        if ((HuPadBtnDown[0] & PAD_BUTTON_LEFT) != 0) {
            MGSeqChampionCreate(GWPlayerCfg[0].character, GWPlayerCfg[1].character, -1, -1);
        }
        if ((HuPadBtnDown[0] & PAD_BUTTON_DOWN) != 0) {
            MGSeqChampionCreate(GWPlayerCfg[0].character, GWPlayerCfg[1].character, GWPlayerCfg[2].character, -1);
        }
        if ((HuPadBtnDown[0] & PAD_BUTTON_RIGHT) != 0) {
            MGSeqChampionCreate(GWPlayerCfg[0].character, GWPlayerCfg[1].character, GWPlayerCfg[2].character, GWPlayerCfg[3].character);
        }
    }
    fontcolor = 5;
    print8(0x10, 0x190, 1.5f, "Terop Pattern%d", teropPatternIndex + 1);
    effectRotationDegrees += 5.0f;
    if (effectRotationDegrees > 360.0f) {
        effectRotationDegrees -= 360.0f;
    }
}

// -------------------------------------------------------------------------- //

void ModelTestCharacterObjectSpritesUpdate(void)
{
    s16 characterModelId;
    s16 objectIndex;
    HSFOBJECT *characterObject;
    ObjectTransformWork *objectWork;
    Mtx parentObjectMatrix;
    Vec objectPosition;
    Mtx childObjectMatrix;
    Mtx inverseParentMatrix;
    Mtx skeletonMatrix;
    Vec objectLocalPosition;
    Vec screenPosition;
    f32 objectRotationY;
    f32 stickMagnitude; // ! - uninitialized
    s16 stickDirection; // ! - uninitialized
    s32 absoluteStickX;
    f32 stickLength;

    characterModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_PEACHMDL1, 0x00));
    Hu3DModelPosSet(characterModelId, 0.0f, 0.0f, 100.0f);

    for (objectIndex = 0; objectIndex < 16; ++objectIndex) {
        OSReport("%s\n", characterObjectNames[objectIndex]);
        characterObject = Hu3DModelObjPtrGet(characterModelId, characterObjectNames[objectIndex]);
        objectTransforms[objectIndex].obj = characterObject;

        Hu3DMotionNoMotSet(characterModelId, characterObjectNames[objectIndex], 0x380);

        objectTransforms[objectIndex].pos.x = characterObject->mesh.base.pos.x;
        objectTransforms[objectIndex].pos.y = characterObject->mesh.base.pos.y;
        objectTransforms[objectIndex].pos.z = characterObject->mesh.base.pos.z;
        objectTransforms[objectIndex].rot.x = characterObject->mesh.base.rot.x;
        objectTransforms[objectIndex].rot.y = characterObject->mesh.base.rot.y;
        objectTransforms[objectIndex].rot.z = characterObject->mesh.base.rot.z;
        objectTransforms[objectIndex].scale.x = characterObject->mesh.base.scale.x;
        objectTransforms[objectIndex].scale.y = characterObject->mesh.base.scale.y;
        objectTransforms[objectIndex].scale.z = characterObject->mesh.base.scale.z;

        OSReport("%d:(%3.2f,%3.2f,%3.2f),(%3.2f,%3.2f,%3.2f)\n", objectIndex, characterObject->mesh.base.pos.x, characterObject->mesh.base.pos.y,
            characterObject->mesh.base.pos.z, characterObject->mesh.base.rot.x, characterObject->mesh.base.rot.y, characterObject->mesh.base.rot.z);
    }
    while (TRUE) {
        Hu3DModelObjMtxGet(characterModelId, "ske_skirt", skeletonMatrix);

        for (objectIndex = 0; objectIndex < 8; ++objectIndex) {
            objectWork = &objectTransforms[objectIndex * 2];
            objectWork->obj->mesh.curr.rot.x = objectWork->rot.x;
            objectWork->obj->mesh.curr.rot.y = objectWork->rot.y;
            objectWork->obj->mesh.curr.rot.z = objectWork->rot.z;

            Hu3DModelObjMtxGet(characterModelId, characterObjectNames[objectIndex * 2], childObjectMatrix);

            objectWork = &objectTransforms[(objectIndex * 2) + 1];
            objectWork->obj->mesh.curr.rot.x = objectWork->rot.x;
            objectWork->obj->mesh.curr.rot.y = objectWork->rot.y;
            objectWork->obj->mesh.curr.rot.z = objectWork->rot.z;

            Hu3DModelObjMtxGet(characterModelId, characterObjectNames[(objectIndex * 2) + 1], parentObjectMatrix);
            Hu3DMtxTransGet(parentObjectMatrix, &objectLocalPosition);
            MTXInverse(skeletonMatrix, inverseParentMatrix);
            Hu3DModelObjMtxGet(characterModelId, characterObjectNames[(objectIndex * 2) + 1], parentObjectMatrix);
            Hu3DMtxTransGet(parentObjectMatrix, &objectLocalPosition);
            MTXMultVec(inverseParentMatrix, &objectLocalPosition, &objectLocalPosition);

            objectRotationY = atan2d(objectLocalPosition.z, objectLocalPosition.x);
            objectWork = &objectTransforms[objectIndex * 2];
            objectWork->obj->mesh.curr.rot.x = objectWork->rot.x + (stickMagnitude * (2.0 * (sind(objectRotationY) * sind(stickDirection))));
            objectWork->obj->mesh.curr.rot.y = objectWork->rot.y;
            objectWork->obj->mesh.curr.rot.z = objectWork->rot.z + (stickMagnitude * cosd(stickDirection));

            Hu3DModelObjMtxGet(characterModelId, characterObjectNames[(objectIndex * 2) + 1], parentObjectMatrix);
            Hu3DMtxTransGet(parentObjectMatrix, &objectPosition);
            WorldPositionToScreenPosition(&objectPosition, &screenPosition);
            HuSprPosSet(spriteGroupId, objectIndex, screenPosition.x, screenPosition.y);
        }

        stickDirection += 5;
        if (stickDirection > 0x168) {
            stickDirection -= 0x168;
        }

        if (HuPadSubStkX[1] < 0) {
            absoluteStickX = -HuPadSubStkX[1];
        }
        else {
            absoluteStickX = HuPadSubStkX[1];
        }

        if (absoluteStickX < 0xA && ABS(HuPadSubStkY[1]) < 0xA) {
            stickDirection = 0x168;
            stickMagnitude = 0.0f;
        }
        else {
            stickDirection = atan2d(HuPadSubStkX[1], HuPadSubStkY[1]);
            stickLength = (HuPadSubStkX[1] * HuPadSubStkX[1]) + (HuPadSubStkY[1] * HuPadSubStkY[1]);
            stickMagnitude = sqrtf(stickLength);
            if (stickMagnitude > 80.0f) {
                stickMagnitude = 80.0f;
            }
        }

        HuPrcVSleep();
    }
}

// -------------------------------------------------------------------------- //

void WorldPositionToScreenPosition(Vec *worldPosition, Vec *screenPosition)
{
    Vec cameraSpacePosition;
    Mtx cameraViewMatrix;
    HU3DCAMERA *cam;
    f32 horizontalProjectionScale;
    f32 verticalProjectionScale;

    cam = &Hu3DCamera[0];
    C_MTXLookAt(cameraViewMatrix, &cam->pos, &cam->up, &cam->target);
    MTXMultVec(cameraViewMatrix, worldPosition, &cameraSpacePosition);

    horizontalProjectionScale = (1.2f * (cameraSpacePosition.z * (sind(cam->fov / 2) / (cosd(cam->fov / 2)))));
    verticalProjectionScale = (cameraSpacePosition.z * (sind(cam->fov / 2) / (cosd(cam->fov / 2))));
    screenPosition->x = 288.0f + (cameraSpacePosition.x * (288.0f / -horizontalProjectionScale));
    screenPosition->y = 240.0f + (cameraSpacePosition.y * (240.0f / verticalProjectionScale));
}

void ModelTestMessageWindowLoop(void)
{
    s16 messageWindowId;

    HuWinInit(1);
    messageWindowId = HuWinCreate(-10000.0f, -10000.0f, 480, 0x80, 0);
    HuWinMesSpeedSet(messageWindowId, 3);

    while (TRUE) {
        HuWinMesSet(messageWindowId, MAKE_MESSID(0x20, 0x11));
        HuWinMesWait(messageWindowId);
    }
}

// -------------------------------------------------------------------------- //

// unreferenced data
u8 unusedModelTestByteData[54] = {
    255,
    255,
    255,
    255,
    0,
    0,
    60,
    68,
    155,
    166,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    60,
    68,
    155,
    166,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    63,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    63,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

// -------------------------------------------------------------------------- //

void ModelTestFramebufferTextureDraw(HU3DMODEL *model, Mtx modelTransform)
{
    GXColor vertexColor;
    Mtx textureMatrix;
    Mtx textureScaleMatrix;
    Mtx textureRotationMatrix;
    Mtx textureOffsetMatrix;
    Mtx normalMatrix;
    GXTexObj framebufferTexture;
    f32 indirectTextureMatrix[2][3];
    Mtx44 orthographicProjection;
    ANIMBMP *framebufferBitmap;
    f32 textureTexelScale;
    f32 unreferencedTextureScaleX; // ! - uninitialized
    f32 unreferencedTextureScaleY; // ! - uninitialized
    f32 unreferencedTextureOffsetX; // ! - uninitialized
    f32 unreferencedTextureOffsetY; // ! - uninitialized

    if (!framebufferBuffers[0]) {
        framebufferBuffers[0] = HuMemDirectMallocNum(HEAP_HEAP, FB_SIZE(640, 480), HU_MEMNUM_OVL);
    }

    GXSetTexCopySrc(0, 0, 640, 480);
    GXSetTexCopyDst(640, 480, GX_TF_RGB565, GX_FALSE);
    GXCopyTex(framebufferBuffers[0], GX_FALSE);
    DCFlushRangeNoSync(framebufferBuffers[0], FB_SIZE(640, 480));
    C_MTXOrtho(orthographicProjection, 0.0f, 480.0f, 0.0f, 576.0f, 0.0f, 10.0f);
    GXSetProjection(orthographicProjection, GX_ORTHOGRAPHIC);
    GXSetViewport(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    GXSetScissor(0, 0, 640, 480);
    GXInvalidateTexAll();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxDesc(GX_VA_NRM, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GXSetCullMode(GX_CULL_NONE);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetNumTevStages(1);

    vertexColor.r = vertexColor.g = vertexColor.b = vertexColor.a = 255;
    GXSetChanMatColor(GX_COLOR0A0, vertexColor);

    vertexColor.r = vertexColor.g = vertexColor.b = 128;
    vertexColor.a = 255;
    GXSetChanAmbColor(GX_COLOR0A0, vertexColor);

    GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    GXSetNumChans(1U);
    GXSetChanCtrl(GX_COLOR0, 1U, GX_SRC_REG, GX_SRC_VTX, 1U, GX_DF_CLAMP, GX_AF_SPOT);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetAlphaCompare(GX_GEQUAL, 1U, GX_AOP_AND, GX_GEQUAL, 1U);
    GXSetZCompLoc(0U);
    MTXIdentity(modelTransform);
    GXLoadPosMtxImm(modelTransform, 0U);

    MTXInvXpose(modelTransform, normalMatrix);
    GXLoadNrmMtxImm(normalMatrix, 0);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

    framebufferBitmap = screenSpriteAnimation->bmp;
    spriteTextureLoadCount += 1;
    HuSprTexLoad(screenSpriteAnimation, 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);

    PPCSync();
    GXInitTexObj(&framebufferTexture, framebufferBuffers[0], 0x280U, 0x1E0U, 4U, GX_CLAMP, GX_CLAMP, 0U);
    GXInitTexObjLOD(&framebufferTexture, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, 0U, 0U, GX_ANISO_1);
    GXLoadTexObj(&framebufferTexture, GX_TEXMAP0);
    GXSetNumIndStages(1U);

    MTXScale(textureMatrix, 0.1f, 0.1f, 0.0f);
    indirectTextureMatrix[0][0] = textureMatrix[0][0];
    indirectTextureMatrix[0][1] = textureMatrix[0][1];
    indirectTextureMatrix[0][2] = 0.0f;
    indirectTextureMatrix[1][0] = textureMatrix[1][0];
    indirectTextureMatrix[1][1] = textureMatrix[1][1];
    indirectTextureMatrix[1][2] = 0.0f;

    MTXRotRad(textureRotationMatrix, 0x58, -1.5707964f);
    mtxRot(textureRotationMatrix, 0.0f, 0.0f, 0.0f);
    textureTexelScale = 0.0015625f;
    MTXScale(textureScaleMatrix, textureTexelScale, textureTexelScale, textureTexelScale);
    MTXConcat(textureScaleMatrix, textureRotationMatrix, textureMatrix);
    MTXTrans(textureOffsetMatrix, 0.5f, 0.0f, 0.5f);
    GXLoadTexMtxImm(textureMatrix, 0x1EU, GX_MTX2x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_POS, 0x1EU, 0U, 0x7DU);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD0, GX_TEXMAP1);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_4, GX_ITS_4);
    GXSetIndTexMtx(GX_ITM_0, indirectTextureMatrix, 0);
    GXSetTevIndWarp(GX_TEVSTAGE0, GX_INDTEXSTAGE0, 1, 0, GX_ITM_0);

    textureRotationDegrees += 5.0f;
    if (textureRotationDegrees > 360.0f) {
        textureRotationDegrees -= 360.0f;
    }

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    GXPosition3f32(0.0f, 0.0f, 0.0f);
    GXTexCoord2f32(0.0f, 1.0f);
    GXNormal3f32(0.0f, unreferencedTextureScaleX / 640.0f, unreferencedTextureScaleY / 480.0f);

    GXPosition3f32(640.0f, 0.0f, 0.0f);
    GXTexCoord2f32(0.0f, 1.0f);
    GXNormal3f32(0.0f, unreferencedTextureOffsetX / 640.0f, unreferencedTextureScaleY / 480.0f);

    GXPosition3f32(640.0f, 480.0f, 0.0f);
    GXTexCoord2f32(0.0f, 1.0f);
    GXNormal3f32(0.0f, unreferencedTextureOffsetX / 640.0f, unreferencedTextureOffsetY / 480.0f);

    GXPosition3f32(0.0f, 480.0f, 0.0f);
    GXTexCoord2f32(0.0f, 1.0f);
    GXNormal3f32(0.0f, unreferencedTextureScaleX / 640.0f, unreferencedTextureOffsetY / 480.0f);

    GXEnd();
    GXSetNumIndStages(0U);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevDirect(GX_TEVSTAGE1);
}

// -------------------------------------------------------------------------- //
