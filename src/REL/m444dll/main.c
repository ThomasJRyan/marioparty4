#include "game/disp.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"

#include "game/pad.h"

#include "game/audio.h"
#include "game/chrman.h"
#include "game/esprite.h"
#include "game/window.h"
#include "game/wipe.h"

#include "game/board/ui.h"
#include "game/gamework.h"
#include "game/gamework_data.h"

#include "REL/m444dll.h"
#include "ext_math.h"

typedef struct CameraViewParams {
    Vec rot;
    Vec pos;
    float zoom;
} CameraViewParams;

CameraViewParams cameraViewPresets[] = {
    { { -55, 0, 0 }, { 0, 60, -57 }, 806 },
    { { -71, 0, 0 }, { 0, 100, -131 }, 900 },
    { { -87, 0, 0 }, { 0, 46, -117 }, 566 },
    { { -87, 0, 0 }, { 0, 46, -117 }, 566 },
    { { -90, 0, 0 }, { -22, 39, -153 }, 432 },
    { { -40, 0, 0 }, { 0, 130, -186 }, 6530 },
    { { -23, 0, 0 }, { -9, 1, -70 }, 1318 },
    { { -25, 0, 0 }, { 0, 242, 90 }, 343 },
    { { -30, 0, 0 }, { 0, 223, -60 }, 505 },
    { { -23, 0, 0 }, { 0, 65, -97 }, 1318 },
};

DataListSprite pinballSprites[] = {
    { DATA_MAKE_NUM(DATADIR_M444, 0x22), 0, 10, 288, -200, { 255, 255, 255, 255 } },
    { 0, 0, 0, 0, 0, { 0, 0, 0, 0 } },
};

float rewardMotionTimes[] = { 90, 250, 420 };

Vec awardCharacterPositions[] = {
    { -75, -300, 0 },
    { 25, -300, 0 },
    { -75, -300, 0 },
    { 25, -300, 0 },
    { -25, -400, 0 },
};

s16 awardPlayerCounts[] = { 1, 1, 2, 2, 2 };

s32 characterRewardMessages[] = {
    DATA_MAKE_NUM(DATADIR_M444, 0x24),
    DATA_MAKE_NUM(DATADIR_M444, 0x25),
    DATA_MAKE_NUM(DATADIR_M444, 0x26),
    DATA_MAKE_NUM(DATADIR_M444, 0x27),
    DATA_MAKE_NUM(DATADIR_M444, 0x28),
    DATA_MAKE_NUM(DATADIR_M444, 0x29),
    DATA_MAKE_NUM(DATADIR_M444, 0x2A),
    DATA_MAKE_NUM(DATADIR_M444, 0x2B),
};

OMOBJ *cameraObject;
HUPROCESS *objectManager;
s32 cameraDebugState[2];
s32 cameraTransitionActive;
Vec cameraTargetRotation;
Vec cameraTargetPosition;
float cameraTargetZoom;
s16 activePlayerConfigIndex;
s16 activeCharacterId;
s16 activeControllerId;
s16 firstAwardPlayer;
s16 secondAwardPlayer;
s16 thirdAwardPlayer;
s16 activeCharacterModel;
s16 characterMotions[8];
Vec cameraRotation[2];
Vec cameraPosition[2];
float cameraZoom[2];
s16 coinTransferModelState[20][8];
s16 starTransferModelState[20][8];
s16 gameState;
s16 activeTransferProcesses;
s32 cameraControlEnabled;

void WinnerPresentationProcess(void);
void GameExitMonitorProcess(void);
void CameraViewUpdate(OMOBJ *object);
void CameraControlProcess(OMOBJ *object);

void ObjectSetup(void)
{
    s32 light;
    OSReport("******* M444 ObjectSetup *********\n");
    objectManager = omInitObjMan(50, 8192);
    omGameSysInit(objectManager);
    cameraRotation[0].x = -70;
    cameraRotation[0].y = 0;
    cameraRotation[0].z = 0;
    cameraPosition[0].x = 0;
    cameraPosition[0].y = 130;
    cameraPosition[0].z = -186;
    cameraZoom[0] = 1360;
    cameraTargetRotation = cameraRotation[0];
    cameraTargetPosition = cameraPosition[0];
    cameraTargetZoom = cameraZoom[0];
    cameraTransitionActive = 0;
    firstAwardPlayer = -2;
    secondAwardPlayer = -2;
    thirdAwardPlayer = -2;
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 30, 20, 15000, 1.2f);
    Hu3DCameraViewportSet(1, 0, 0, 640, 480, 0, 1);
    light = Hu3DGLightCreate(-1000, 1000, 0, 1, -1, 0, 255, 255, 255);
    Hu3DGLightInfinitytSet(light);
    Hu3DGLightStaticSet(light, 1);
    light = Hu3DGLightCreate(0, 50, 1000, 0, -0.05f, -1, 96, 96, 96);
    Hu3DGLightInfinitytSet(light);
    HuPrcChildCreate(WinnerPresentationProcess, 100, 12288, 0, objectManager);
    HuPrcChildCreate(GameExitMonitorProcess, 200, 4096, 0, objectManager);
    cameraObject = omAddObjEx(objectManager, 32730, 0, 0, -1, CameraViewUpdate);
    omAddObjEx(objectManager, 0, 32, 32, -1, CameraControlProcess);
    Hu3DBGColorSet(0, 0, 0);
#if VERSION_JP
    GWGameStat.language = 0;
#endif
    HuWinInit(1);
    BoardStatusCreate();
}

void PinballGameProcess(void);

void RewardTransferProcess(void);

void PinballAssetsLoad(void);
void PlayerInputWindowOpen(s16 window);

void CameraViewTransition(s16 view);
void CameraViewSet(s16 view);
float CameraZoomGet(void);

void WinnerPresentationProcess(void)
{
    float animationProgress;
    float rotationAngle;

    s16 frame;
    s16 windowId;
    s16 frameOffset;
    u32 messageId;
    s16 modelIndex;
    s16 winnerIndex;
    s16 showRewardText;
    u32 insertedMessageId;
    HU3DMODEL *playerModel;

    Vec modelPosition;
    Vec previousModelPosition;
    float windowSize[2];

    for (frame = 0; frame < 4; frame++) {
        if (GWPlayerCfg[frame].group == 0) {
            break;
        }
    }
    activePlayerConfigIndex = frame;
    activeControllerId = GWPlayerCfg[frame].pad_idx;
    activeCharacterId = GWPlayerCfg[frame].character;
    PinballAssetsLoad();
    CameraViewSet(5);
    CameraViewTransition(6);
    Hu3DModelPosSet(activeCharacterModel, 0, 0, 250);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
    HuAudSeqPlay(76);
    while (WipeStatGet()) {
        HuPrcVSleep();
    }
    while (CameraZoomGet() > 10.0f) {
        HuPrcVSleep();
    }
    Hu3DModelAttrReset(loadedModelIds[18], HU3D_ATTR_DISPOFF);
    Hu3DMotionSet(loadedModelIds[18], loadedMotionIds[20]);
    Hu3DModelRotSet(loadedModelIds[18], 0, 90, 0);
    for (frame = 0; frame <= 60; frame++) {
        if (frame <= 30) {
            espPosSet(loadedSpriteIds[0], 288, (200.0 * sind(3.0 * frame)) - 100.0);
        }
        Hu3DModelPosSet(loadedModelIds[18], (-300.0 * cosd(1.5 * frame)) + (-160.0), 0, 155);
        HuPrcVSleep();
    }
    for (frame = 0; frame <= 30; frame++) {
        if (frame <= 20) {
            Hu3DModelRotSet(loadedModelIds[18], 0, (30.0 * (1.0 - (frame / 20.0f))) + 60.0, 0);
        }
        if (frame > 10) {
            frameOffset = frame - 10;
            Hu3DModelRotSet(activeCharacterModel, 0, (-(frameOffset / 20.0f)) * 120.0f, 0);
        }
        if (frame == 10) {
            Hu3DMotionShiftSet(loadedModelIds[18], loadedMotionIds[19], 0, 16, HU3D_MOTATTR_LOOP);
            CharMotionShiftSet(activeCharacterId, characterMotions[1], 0, 8, HU3D_MOTATTR_LOOP);
        }
        HuPrcVSleep();
    }
    CharMotionShiftSet(activeCharacterId, characterMotions[0], 0, 8, HU3D_MOTATTR_LOOP);
    HuWinMesMaxSizeGet(1, windowSize, MAKE_MESSID(0x1C, 0x00));
    windowId = HuWinExCreateStyled(-10000, 340, windowSize[0], windowSize[1], -1, 1);
    HuWinExAnimIn(windowId);
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    PlayerInputWindowOpen(windowId);
    HuWinMesSet(windowId, MAKE_MESSID(0x1C, 0x00));
    HuWinMesWait(windowId);
    HuWinExAnimOut(windowId);
    HuWinExCleanup(windowId);
    HuWinComKeyReset();
    Hu3DMotionShiftSet(loadedModelIds[18], loadedMotionIds[20], 0, 10, HU3D_MOTATTR_LOOP);
    for (frame = 0; frame <= 20; frame++) {
        HuPrcVSleep();
    }
    for (frame = 0; frame <= 30; frame++) {
        if (frame <= 10) {
            Hu3DModelRotSet(loadedModelIds[18], 0, 30.0f - ((frame / 10.0f) * 120.0f), 0);
        }
        if (frame <= 20) {
            espPosSet(loadedSpriteIds[0], 288, (200.0 * cosd(frame * 4.5)) - 100.0);
        }
        Hu3DModelPosSet(loadedModelIds[18], ((1.0 - cosd(frame * 3.0)) * -200.0) + -160.0, 0, 155);
        HuPrcVSleep();
    }
    Hu3DModelAttrSet(loadedModelIds[18], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(loadedModelIds[22], HU3D_ATTR_DISPOFF);
    Hu3DMotionSet(loadedModelIds[22], loadedMotionIds[24]);
    Hu3DModelRotSet(loadedModelIds[22], 0, 90, 0);
    for (frame = 0; frame <= 60; frame++) {
        Hu3DModelPosSet(loadedModelIds[22], (-300.0 * cosd(1.5 * frame)) + (-160.0), 0, 155);
        if (frame > 20) {
            frameOffset = frame - 40;
            Hu3DModelRotSet(loadedModelIds[22], 0, (30.0 * (1.0 - (frameOffset / 20.0f))) + 60.0, 0);
        }
        if (frame == 50) {
            Hu3DMotionShiftSet(loadedModelIds[22], loadedMotionIds[23], 0, 10, HU3D_MOTATTR_LOOP);
        }
        if (frame > 40) {
            frameOffset = frame - 40;
            for (modelIndex = 0; modelIndex < 3; modelIndex++) {
                awardOrbPositions[modelIndex].y = (180.0 * sind(4.5 * frameOffset)) - 100.0;
            }
        }
        HuPrcVSleep();
    }
    HuWinMesMaxSizeGet(1, windowSize, MAKE_MESSID(0x1C, 0x01));
    windowId = HuWinExCreateStyled(-10000, 340, windowSize[0], windowSize[1], -1, 1);
    HuWinExAnimIn(windowId);
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    PlayerInputWindowOpen(windowId);
    HuWinMesSet(windowId, MAKE_MESSID(0x1C, 0x01));
    HuWinMesWait(windowId);
    HuWinExAnimOut(windowId);
    HuWinExCleanup(windowId);
    HuWinComKeyReset();
    Hu3DModelAttrReset(loadedModelIds[11], HU3D_MOTATTR_PAUSE);
    HuAudFXPlay(1854);
    Hu3DMotionShiftSet(loadedModelIds[22], loadedMotionIds[24], 0, 10, HU3D_MOTATTR_LOOP);
    for (frame = 0; frame < 20; frame++) {
        animationProgress = frame / 20.0f;
        Hu3DModelRotSet(loadedModelIds[22], 0, (150.0f * animationProgress) + 30.0f, 0);
        HuPrcVSleep();
    }
    Hu3DMotionShiftSet(loadedModelIds[22], loadedMotionIds[23], 0, 10, HU3D_MOTATTR_LOOP);
    CharMotionShiftSet(activeCharacterId, characterMotions[1], 0, 10, HU3D_MOTATTR_LOOP);
    for (frame = 0; frame < 20; frame++) {
        animationProgress = frame / 20.0f;
        Hu3DModelRotSet(activeCharacterModel, 0, -120.0f - (60.0f * animationProgress), 0);
        HuPrcVSleep();
    }
    HuAudFXPlay(1855);
    CharMotionShiftSet(activeCharacterId, characterMotions[0], 0, 10, HU3D_MOTATTR_LOOP);
    while (Hu3DMotionEndCheck(loadedModelIds[11]) == 0) {
        HuPrcVSleep();
    }
    HuAudFXPlay(1856);
    PinballRoundStart(0);
    Hu3DModelAttrReset(loadedModelIds[9], HU3D_MOTATTR_PAUSE);
    HuAudFXPlay(1863);
    while (Hu3DMotionTimeGet(loadedModelIds[9]) < rewardMotionTimes[0]) {
        HuPrcVSleep();
    }
    HuAudFXPlay(1857);
    Hu3DMotionTimeSet(loadedModelIds[9], rewardMotionTimes[0]);
    Hu3DModelAttrSet(loadedModelIds[9], HU3D_MOTATTR_PAUSE);
    HuPrcSleep(20);
    CharMotionShiftSet(activeCharacterId, characterMotions[1], 0, 8, HU3D_MOTATTR_LOOP);
    modelPosition.x = 124;
    modelPosition.y = 0;
    modelPosition.z = -180;
    for (frame = 0; frame <= 40; frame++) {
        animationProgress = frame / 40.0f;
        Hu3DModelPosSet(activeCharacterModel, animationProgress * modelPosition.x, animationProgress * modelPosition.y, (animationProgress * modelPosition.z) + 250.0f);
        if (frame <= 20) {
            rotationAngle = atan2d(modelPosition.x, modelPosition.z) - 180.0;
            Hu3DModelRotSet(activeCharacterModel, 0, (rotationAngle * (frame / 20.0)) + 180.0, 0);
        }
        if (frame > 30) {
            frameOffset = frame - 30;
            rotationAngle = atan2d(modelPosition.x, modelPosition.z) - 180.0;
            Hu3DModelRotSet(activeCharacterModel, 0, (rotationAngle * (1.0 - (frameOffset / 10.0))) + 180.0, 0);
        }
        HuPrcVSleep();
    }
    firstAwardPlayer = -1;
    PinballPlayerUpdate(0);
    winnerIndex = PinballBallUpdate(0);
    firstAwardPlayer = playerOrder[winnerIndex];
    HuAudFXPlay(1859);
    PinballTargetHit(0, winnerIndex);
    for (frame = 0; frame <= 60; frame++) {
        awardOrbScales[0].x = awardOrbScales[0].y = awardOrbScales[0].z = (0.1 * sind(frame * 18.0)) + 0.3;
        HuPrcVSleep();
    }
    HuPrcSleep(40);
    Hu3DModelAttrSet(loadedModelIds[64], HU3D_ATTR_DISPOFF);
    CameraViewTransition(9);
    Hu3DMotionShiftSet(loadedModelIds[22], loadedMotionIds[24], 0, 10, HU3D_MOTATTR_LOOP);
    for (frame = 0; frame <= 30; frame++) {
        if (frame <= 10) {
            Hu3DModelRotSet(loadedModelIds[22], 0, (90.0 * (frame / 10.0)) + 180.0, 0);
        }
        if (frame == 10) {
            CharMotionShiftSet(activeCharacterId, characterMotions[1], 0, 10, HU3D_MOTATTR_LOOP);
        }
        if (frame > 10 && frame <= 25) {
            frameOffset = frame - 10;
            Hu3DModelRotSet(activeCharacterModel, 0, (90.0 * (frameOffset / 15.0)) + 180.0, 0);
        }
        if (frame == 25) {
            CharMotionShiftSet(activeCharacterId, characterMotions[0], 0, 10, HU3D_MOTATTR_LOOP);
        }
        Hu3DModelPosSet(loadedModelIds[22], (-300.0 * (1.0 - cosd(3.0 * frame))) + (-160.0), 0, 155);
        HuPrcVSleep();
    }
    Hu3DModelAttrSet(loadedModelIds[22], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(loadedModelIds[26], HU3D_ATTR_DISPOFF);
    Hu3DMotionSet(loadedModelIds[26], loadedMotionIds[28]);
    Hu3DModelRotSet(loadedModelIds[26], 0, 90, 0);
    for (frame = 0; frame <= 60; frame++) {
        Hu3DModelPosSet(loadedModelIds[26], (-300.0 * cosd(1.5 * frame)) + (-160.0), 0, 155);
        if (frame == 50) {
            Hu3DMotionShiftSet(loadedModelIds[26], loadedMotionIds[27], 0, 10, HU3D_MOTATTR_LOOP);
        }
        HuPrcVSleep();
    }
    #if VERSION_PAL
    HuWinInsertMesSizeGet((GWPlayerCfg[firstAwardPlayer].character+1)-1, 0);
    #endif
    HuWinMesMaxSizeGet(1, windowSize, MAKE_MESSID(0x1C, 0x02));
    windowId = HuWinExCreateStyled(-10000, 340, windowSize[0], windowSize[1], -1, 1);
    HuWinExAnimIn(windowId);
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    PlayerInputWindowOpen(windowId);
    HuWinInsertMesSet(windowId, GWPlayerCfg[firstAwardPlayer].character, 0);
    HuWinMesSet(windowId, MAKE_MESSID(0x1C, 0x02));
    HuWinMesWait(windowId);
    HuWinExAnimOut(windowId);
    HuWinExCleanup(windowId);
    HuWinComKeyReset();
    Hu3DMotionShiftSet(loadedModelIds[26], loadedMotionIds[28], 0, 10, HU3D_MOTATTR_LOOP);
    for (frame = 0; frame < 20; frame++) {
        animationProgress = frame / 20.0f;
        Hu3DModelRotSet(loadedModelIds[26], 0, (animationProgress * 90.0f) + 90.0f, 0);
        HuPrcVSleep();
    }
    Hu3DMotionShiftSet(loadedModelIds[26], loadedMotionIds[27], 0, 10, HU3D_MOTATTR_LOOP);
    PinballRoundStart(1);
    Hu3DModelAttrReset(loadedModelIds[9], HU3D_MOTATTR_PAUSE);
    HuAudFXPlay(1862);
    CharMotionShiftSet(activeCharacterId, characterMotions[1], 0, 10, HU3D_MOTATTR_LOOP);
    for (frame = 0; frame < 20; frame++) {
        animationProgress = 1.0 - (frame / 20.0f);
        Hu3DModelRotSet(activeCharacterModel, 0, (animationProgress * 90.0f) + 180.0f, 0);
        HuPrcVSleep();
    }
    CharMotionShiftSet(activeCharacterId, characterMotions[0], 0, 10, HU3D_MOTATTR_LOOP);
    while (Hu3DMotionTimeGet(loadedModelIds[9]) < rewardMotionTimes[1]) {
        if (Hu3DMotionTimeGet(loadedModelIds[9]) == 128.0f) {
            HuAudFXPlay(1850);
        }
        if (Hu3DMotionTimeGet(loadedModelIds[9]) == 200.0f) {
            HuAudFXPlay(1863);
        }
        HuPrcVSleep();
    }
    HuAudFXPlay(1857);
    Hu3DMotionTimeSet(loadedModelIds[9], rewardMotionTimes[1]);
    Hu3DModelAttrSet(loadedModelIds[9], HU3D_MOTATTR_PAUSE);
    HuPrcSleep(20);
    secondAwardPlayer = -1;
    PinballPlayerUpdate(1);
    winnerIndex = PinballBallUpdate(1);
    secondAwardPlayer = playerOrder[winnerIndex];
    HuAudFXPlay(1860);
    PinballTargetHit(1, winnerIndex);
    for (frame = 0; frame <= 60; frame++) {
        awardOrbScales[1].x = awardOrbScales[1].y = awardOrbScales[1].z = (0.1 * sind(frame * 18.0)) + 0.3;
        HuPrcVSleep();
    }
    HuPrcSleep(40);
    Hu3DModelAttrSet(loadedModelIds[64], HU3D_ATTR_DISPOFF);
    CameraViewTransition(9);
    Hu3DMotionShiftSet(loadedModelIds[26], loadedMotionIds[28], 0, 10, HU3D_MOTATTR_LOOP);
    for (frame = 0; frame <= 30; frame++) {
        if (frame <= 10) {
            Hu3DModelRotSet(loadedModelIds[26], 0, (90.0 * (frame / 10.0)) + 180.0, 0);
        }
        if (frame == 10) {
            CharMotionShiftSet(activeCharacterId, characterMotions[1], 0, 10, HU3D_MOTATTR_LOOP);
        }
        if (frame > 10 && frame <= 25) {
            frameOffset = frame - 10;
            Hu3DModelRotSet(activeCharacterModel, 0, (90.0 * (frameOffset / 15.0)) + 180.0, 0);
        }
        if (frame == 25) {
            CharMotionShiftSet(activeCharacterId, characterMotions[0], 0, 10, HU3D_MOTATTR_LOOP);
        }
        Hu3DModelPosSet(loadedModelIds[26], (-400.0 * (1.0 - cosd(3.0 * frame))) + (-160.0), 0, 155);
        HuPrcVSleep();
    }
    Hu3DModelAttrSet(loadedModelIds[26], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(loadedModelIds[30], HU3D_ATTR_DISPOFF);
    Hu3DMotionSet(loadedModelIds[30], loadedMotionIds[32]);
    Hu3DModelRotSet(loadedModelIds[30], 0, 90, 0);
    for (frame = 0; frame <= 60; frame++) {
        Hu3DModelPosSet(loadedModelIds[30], (-400.0 * cosd(1.5 * frame)) + (-160.0), 100, 155);
        if (frame == 50) {
            Hu3DMotionShiftSet(loadedModelIds[30], loadedMotionIds[31], 0, 10, HU3D_MOTATTR_LOOP);
        }
        HuPrcVSleep();
    }
    #if VERSION_PAL
    HuWinInsertMesSizeGet((GWPlayerCfg[firstAwardPlayer].character+1)-1, 0);
    HuWinInsertMesSizeGet((GWPlayerCfg[secondAwardPlayer].character+1)-1, 2);
    #endif
    HuWinMesMaxSizeGet(1, windowSize, MAKE_MESSID(0x1C, 0x03));
    windowId = HuWinExCreateStyled(-10000, 340, windowSize[0], windowSize[1], -1, 1);
    HuWinExAnimIn(windowId);
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    PlayerInputWindowOpen(windowId);
    HuWinInsertMesSet(windowId, GWPlayerCfg[firstAwardPlayer].character, 0);
    HuWinInsertMesSet(windowId, GWPlayerCfg[secondAwardPlayer].character, 2);
    HuWinMesSet(windowId, MAKE_MESSID(0x1C, 0x03));
    HuWinMesWait(windowId);
    HuWinExAnimOut(windowId);
    HuWinExCleanup(windowId);
    HuWinComKeyReset();
    Hu3DMotionShiftSet(loadedModelIds[30], loadedMotionIds[32], 0, 10, HU3D_MOTATTR_LOOP);
    for (frame = 0; frame < 20; frame++) {
        animationProgress = frame / 20.0f;
        Hu3DModelRotSet(loadedModelIds[30], 0, (animationProgress * 90.0f) + 90.0f, 0);
        HuPrcVSleep();
    }
    Hu3DMotionShiftSet(loadedModelIds[30], loadedMotionIds[31], 0, 10, HU3D_MOTATTR_LOOP);
    PinballRoundStart(2);
    Hu3DModelAttrReset(loadedModelIds[9], HU3D_MOTATTR_PAUSE);
    HuAudFXPlay(1862);
    CharMotionShiftSet(activeCharacterId, characterMotions[1], 0, 10, HU3D_MOTATTR_LOOP);
    for (frame = 0; frame < 20; frame++) {
        animationProgress = 1.0 - (frame / 20.0f);
        Hu3DModelRotSet(activeCharacterModel, 0, (animationProgress * 90.0f) + 180.0f, 0);
        HuPrcVSleep();
    }
    CharMotionShiftSet(activeCharacterId, characterMotions[0], 0, 10, HU3D_MOTATTR_LOOP);
    while (Hu3DMotionTimeGet(loadedModelIds[9]) < rewardMotionTimes[2]) {
        if (Hu3DMotionTimeGet(loadedModelIds[9]) == 300.0f) {
            HuAudFXPlay(1850);
        }
        if (Hu3DMotionTimeGet(loadedModelIds[9]) == 372.0f) {
            HuAudFXPlay(1863);
        }
        HuPrcVSleep();
    }
    HuAudFXPlay(1857);
    Hu3DMotionTimeSet(loadedModelIds[9], rewardMotionTimes[2]);
    Hu3DModelAttrSet(loadedModelIds[9], HU3D_MOTATTR_PAUSE);
    HuPrcSleep(20);
    thirdAwardPlayer = -1;
    PinballPlayerUpdate(2);
    winnerIndex = PinballBallUpdate(2);
    thirdAwardPlayer = winnerIndex;
    HuAudFXPlay(1861);
    PinballTargetHit(2, winnerIndex);
    for (frame = 0; frame <= 60; frame++) {
        awardOrbScales[2].x = awardOrbScales[2].y = awardOrbScales[2].z = (0.1 * sind(frame * 18.0)) + 0.3;
        HuPrcVSleep();
    }
    HuPrcSleep(40);
    Hu3DModelAttrSet(loadedModelIds[64], HU3D_ATTR_DISPOFF);
    CameraViewTransition(9);
    Hu3DMotionShiftSet(loadedModelIds[30], loadedMotionIds[32], 0, 10, HU3D_MOTATTR_LOOP);
    for (frame = 0; frame <= 30; frame++) {
        if (frame <= 10) {
            Hu3DModelRotSet(loadedModelIds[30], 0, (90.0 * (frame / 10.0)) + 180.0, 0);
        }
        if (frame == 10) {
            CharMotionShiftSet(activeCharacterId, characterMotions[1], 0, 10, HU3D_MOTATTR_LOOP);
        }
        if (frame > 10 && frame <= 25) {
            frameOffset = frame - 10;
            Hu3DModelRotSet(activeCharacterModel, 0, (90.0 * (frameOffset / 15.0)) + 180.0, 0);
        }
        if (frame == 25) {
            CharMotionShiftSet(activeCharacterId, characterMotions[0], 0, 10, HU3D_MOTATTR_LOOP);
        }
        Hu3DModelPosSet(loadedModelIds[30], (-400.0 * (1.0 - cosd(3.0 * frame))) + (-160.0), 100, 155);
        HuPrcVSleep();
    }
    HuAudFXPlay(1862);
    Hu3DModelAttrReset(loadedModelIds[9], HU3D_MOTATTR_PAUSE);
    while (Hu3DMotionEndCheck(loadedModelIds[9]) == 0) {
        HuPrcVSleep();
    }
    Hu3DModelAttrReset(loadedModelIds[11], HU3D_MOTATTR_PAUSE);
    HuAudFXPlay(1855);
    Hu3DModelAttrSet(loadedModelIds[11], HU3D_MOTATTR_REV);
    while (Hu3DMotionTimeGet(loadedModelIds[11]) > 0.0f) {
        if (Hu3DMotionTimeGet(loadedModelIds[11]) == 25.0f) {
            HuAudFXPlay(1854);
        }
        HuPrcVSleep();
    }
    HuAudFXPlay(1856);
    Hu3DModelAttrReset(loadedModelIds[34], HU3D_ATTR_DISPOFF);
    Hu3DMotionSet(loadedModelIds[34], loadedMotionIds[36]);
    Hu3DModelRotSet(loadedModelIds[34], 0, 90, 0);
    for (frame = 0; frame <= 60; frame++) {
        Hu3DModelPosSet(loadedModelIds[34], -154.0 + (-500.0 * (1.0 - (frame / 60.0f))), 0, -403);
        if (frame > 20) {
            frameOffset = frame - 40;
            Hu3DModelRotSet(loadedModelIds[34], 0, (30.0 * (1.0 - (frameOffset / 20.0f))) + 60.0, 0);
        }
        if (frame == 50) {
            Hu3DMotionShiftSet(loadedModelIds[34], loadedMotionIds[35], 0, 10, HU3D_MOTATTR_LOOP);
        }
        if (frame == 40) {
            CharMotionShiftSet(activeCharacterId, characterMotions[1], 0, 10, HU3D_MOTATTR_LOOP);
        }
        if (frame >= 40) {
            frameOffset = frame - 40;
            Hu3DModelRotSet(activeCharacterModel, 0, -90.0 - (45.0 * (frameOffset / 20.0)), 0);
        }
        HuPrcVSleep();
    }
    CameraViewTransition(7);
    playerModel = &Hu3DData[activeCharacterModel];
    modelPosition.x = 0;
    modelPosition.y = 0;
    modelPosition.z = -200;
    previousModelPosition = playerModel->pos;
    VECSubtract(&modelPosition, &previousModelPosition, &modelPosition);
    for (frame = 0; frame <= 30; frame++) {
        animationProgress = frame / 30.0;
        Hu3DModelPosSet(activeCharacterModel, (animationProgress * modelPosition.x) + previousModelPosition.x, (animationProgress * modelPosition.y) + previousModelPosition.y, (animationProgress * modelPosition.z) + previousModelPosition.z);
        HuPrcVSleep();
    }
    CharMotionShiftSet(activeCharacterId, characterMotions[0], 0, 10, HU3D_MOTATTR_LOOP);
    BoardStatusShowSet(firstAwardPlayer, 1);
    BoardStatusShowSet(secondAwardPlayer, 1);
    modelPosition.x = -112;
    modelPosition.y = 200;
    modelPosition.z = 0;
    BoardStatusPosSet(firstAwardPlayer, &modelPosition);
    modelPosition.x = 688;
    modelPosition.y = 200;
    modelPosition.z = 0;
    BoardStatusPosSet(secondAwardPlayer, &modelPosition);
    for (frame = 0; frame <= 20; frame++) {
        if (frame > 10) {
            frameOffset = frame - 10;
            modelPosition.x = 138.0 - (250.0 * cosd(frameOffset * 9.0));
            modelPosition.y = 200;
            modelPosition.z = 0;
            BoardStatusTargetPosSet(firstAwardPlayer, &modelPosition);
            modelPosition.x = 438.0 + (250.0 * cosd(frameOffset * 9.0));
            modelPosition.y = 200;
            modelPosition.z = 0;
            BoardStatusTargetPosSet(secondAwardPlayer, &modelPosition);
        }
        HuPrcVSleep();
    }
    showRewardText = 1;
    switch (thirdAwardPlayer) {
        case 0:
            if (GWPlayer[firstAwardPlayer].coins <= 0 && GWPlayer[secondAwardPlayer].coins <= 0) {
                messageId = MAKE_MESSID(0x1C, 0x16);
                showRewardText = 0;
            }
            else {
                messageId = MAKE_MESSID(0x1C, 0x06);
            }
            insertedMessageId = MAKE_MESSID(0x1C, 0x09);
            break;

        case 1:
            if (GWPlayer[firstAwardPlayer].coins <= 0 && GWPlayer[secondAwardPlayer].coins <= 0 && GWPlayer[firstAwardPlayer].stars <= 0
                && GWPlayer[secondAwardPlayer].stars <= 0) {
                showRewardText = 0;
                messageId = MAKE_MESSID(0x1C, 0x17);
            }
            else {
                messageId = MAKE_MESSID(0x1C, 0x06);
            }
            insertedMessageId = MAKE_MESSID(0x1C, 0x0D);
            break;

        case 2:
            if (GWPlayer[firstAwardPlayer].stars > 0) {
                messageId = MAKE_MESSID(0x1C, 0x04);
            }
            else {
                messageId = MAKE_MESSID(0x1C, 0x08);
                showRewardText = 0;
            }
            insertedMessageId = MAKE_MESSID(0x1C, 0x0A);
            break;

        case 3:
            if (GWPlayer[firstAwardPlayer].stars <= 0 && GWPlayer[secondAwardPlayer].stars <= 0) {
                showRewardText = 0;
                messageId = MAKE_MESSID(0x1C, 0x15);
            }
            else {
                messageId = MAKE_MESSID(0x1C, 0x06);
            }
            insertedMessageId = MAKE_MESSID(0x1C, 0x0E);
            break;

        case 4:
            if (GWPlayer[firstAwardPlayer].stars > 0) {
                messageId = MAKE_MESSID(0x1C, 0x04);
            }
            else {
                messageId = MAKE_MESSID(0x1C, 0x08);
                showRewardText = 0;
            }
            insertedMessageId = MAKE_MESSID(0x1C, 0x0C);
            break;

        case 5:
        case 6:
            if (GWPlayer[firstAwardPlayer].coins > 0) {
                messageId = MAKE_MESSID(0x1C, 0x04);
            }
            else {
                messageId = MAKE_MESSID(0x1C, 0x07);
                showRewardText = 0;
            }
            insertedMessageId = MAKE_MESSID(0x1C, 0x0B);
            break;

        default:
            break;
    }
    HuWinInsertMesSizeGet((8 + GWPlayerCfg[firstAwardPlayer].character) - 8, 0);
    HuWinInsertMesSizeGet(insertedMessageId, 1);
    HuWinInsertMesSizeGet((8 + GWPlayerCfg[secondAwardPlayer].character) - 8, 2);
    HuWinMesMaxSizeGet(1, windowSize, messageId);
    windowId = HuWinExCreateStyled(-10000, 340, windowSize[0], windowSize[1], -1, 1);
    HuWinExAnimIn(windowId);
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    HuWinInsertMesSet(windowId, GWPlayerCfg[firstAwardPlayer].character, 0);
    HuWinInsertMesSet(windowId, insertedMessageId, 1);
    HuWinInsertMesSet(windowId, GWPlayerCfg[secondAwardPlayer].character, 2);
    PlayerInputWindowOpen(windowId);
    HuWinMesSet(windowId, messageId);
    HuWinMesWait(windowId);
    HuWinExAnimOut(windowId);
    HuWinExCleanup(windowId);
    HuWinComKeyReset();
    if (showRewardText) {
        RewardTransferProcess();
        HuPrcSleep(60);
    }
    else {
        HuPrcSleep(30);
    }
    modelPosition.x = -112;
    modelPosition.y = 200;
    modelPosition.z = 0;
    BoardStatusTargetPosSet(firstAwardPlayer, &modelPosition);
    modelPosition.x = 688;
    modelPosition.y = 200;
    modelPosition.z = 0;
    BoardStatusTargetPosSet(secondAwardPlayer, &modelPosition);
    while (!BoardStatusStopCheck(firstAwardPlayer)) {
        HuPrcVSleep();
    }
    for (frame = 0; frame <= 20; frame++) {
        for (modelIndex = 0; modelIndex < 3; modelIndex++) {
            awardOrbPositions[modelIndex].y = (180.0 * cosd(4.5 * frame)) - 100.0;
        }
        HuPrcVSleep();
    }
    PinballGameProcess();
    cameraTransitionActive = 1;
    while (1) {
        cameraZoom[0] += 10.0f;
        HuPrcVSleep();
    }
}

void PinballGameProcess(void)
{
    float rotationAngle;
    s16 animationFrame;
    s16 elapsedFrames;
    s16 frameOffset;
    s16 windowId;
    float windowSize[2];
    CameraViewTransition(8);
    for (elapsedFrames = 0; elapsedFrames < 100; elapsedFrames++) {
        animationFrame = elapsedFrames;
        if (animationFrame == 0) {
            Hu3DMotionShiftSet(loadedModelIds[34], loadedMotionIds[36], 0, 10, HU3D_MOTATTR_LOOP);
        }
        if (animationFrame <= 20) {
            Hu3DModelRotSet(loadedModelIds[34], 0, (30.0 * (animationFrame / 20.0)) + 60.0, 0);
        }
        if (animationFrame > 10 && animationFrame <= 60) {
            frameOffset = animationFrame - 10;
            Hu3DModelPosSet(loadedModelIds[34], -154.0f + (354.0 * (frameOffset / 50.0)), 0, -403);
        }
        if (animationFrame > 50 && animationFrame <= 70) {
            frameOffset = animationFrame - 50;
            Hu3DModelRotSet(loadedModelIds[34], 0, (110.0 * (1.0 - (frameOffset / 20.0))) + -20.0, 0);
        }
        if (animationFrame == 70) {
            Hu3DMotionShiftSet(loadedModelIds[34], loadedMotionIds[35], 0, 10, HU3D_MOTATTR_LOOP);
        }
        animationFrame = elapsedFrames;
        if (animationFrame == 0) {
            Hu3DMotionSet(loadedModelIds[18], loadedMotionIds[20]);
            rotationAngle = atan2d(400.0, 200.0) + 180.0;
            Hu3DModelRotSet(loadedModelIds[18], 0, rotationAngle, 0);
            Hu3DModelAttrReset(loadedModelIds[18], HU3D_ATTR_DISPOFF);
        }
        if (animationFrame <= 50) {
            Hu3DModelPosSet(loadedModelIds[18], 400.0 * (1.0 - (animationFrame / 50.0)), 0, -553.0 + (200.0 * (1.0 - (animationFrame / 50.0))));
        }
        if (animationFrame > 40 && animationFrame <= 60) {
            frameOffset = animationFrame - 40;
            rotationAngle = atan2d(400.0, 200.0) + 180.0;
            Hu3DModelRotSet(loadedModelIds[18], 0, rotationAngle * (1.0 - (frameOffset / 20.0)), 0);
        }
        if (animationFrame == 60) {
            Hu3DMotionShiftSet(loadedModelIds[18], loadedMotionIds[19], 0, 10, HU3D_MOTATTR_LOOP);
        }
        if (elapsedFrames >= 20) {
            animationFrame = elapsedFrames - 20;
            if (animationFrame == 0) {
                Hu3DMotionSet(loadedModelIds[26], loadedMotionIds[28]);
                rotationAngle = atan2d(-300.0, 200.0) + 180.0;
                Hu3DModelRotSet(loadedModelIds[26], 0, rotationAngle, 0);
                Hu3DModelAttrReset(loadedModelIds[26], HU3D_ATTR_DISPOFF);
            }
            if (animationFrame <= 40) {
                Hu3DModelPosSet(loadedModelIds[26], -100.0 + (-300.0 * (1.0 - (animationFrame / 40.0))), 0, -478.0 + (200.0 * (1.0 - (animationFrame / 40.0))));
            }
            if (animationFrame > 30 && animationFrame <= 50) {
                frameOffset = animationFrame - 30;
                rotationAngle = atan2d(-300.0, 200.0) + 180.0;
                Hu3DModelRotSet(loadedModelIds[26], 0, (rotationAngle - 10.0f) * (1.0 - (frameOffset / 20.0)) + 10.0, 0);
            }
            if (animationFrame == 50) {
                Hu3DMotionShiftSet(loadedModelIds[26], loadedMotionIds[27], 0, 10, HU3D_MOTATTR_LOOP);
            }
        }
        if (elapsedFrames >= 30) {
            animationFrame = elapsedFrames - 30;
            if (animationFrame == 0) {
                Hu3DMotionSet(loadedModelIds[30], loadedMotionIds[32]);
                rotationAngle = atan2d(200.0, 500.0) + 180.0;
                Hu3DModelRotSet(loadedModelIds[30], 0, rotationAngle, 0);
                Hu3DModelAttrReset(loadedModelIds[30], HU3D_ATTR_DISPOFF);
            }
            if (animationFrame <= 40) {
                Hu3DModelPosSet(loadedModelIds[30], 100.0 + (200.0 * (1.0 - (animationFrame / 40.0))), 100, -478.0 + (500.0 * (1.0 - (animationFrame / 40.0))));
            }
            if (animationFrame > 30 && animationFrame <= 50) {
                frameOffset = animationFrame - 30;
                rotationAngle = atan2d(200.0, 500.0) + 180.0;
                Hu3DModelRotSet(loadedModelIds[30], 0, (rotationAngle - -10.0f) * (1.0 - (frameOffset / 20.0)) + -10.0, 0);
            }
            if (animationFrame == 50) {
                Hu3DMotionShiftSet(loadedModelIds[30], loadedMotionIds[31], 0, 10, HU3D_MOTATTR_LOOP);
            }
        }
        if (elapsedFrames >= 40) {
            animationFrame = elapsedFrames - 40;
            if (animationFrame == 0) {
                Hu3DMotionSet(loadedModelIds[22], loadedMotionIds[24]);
                rotationAngle = atan2d(-300.0, 100.0) + 180.0;
                Hu3DModelRotSet(loadedModelIds[22], 0, rotationAngle, 0);
                Hu3DModelAttrReset(loadedModelIds[22], HU3D_ATTR_DISPOFF);
            }
            if (animationFrame <= 40) {
                Hu3DModelPosSet(loadedModelIds[22], -200.0 + (-300.0 * (1.0 - (animationFrame / 40.0))), 0, -403.0 + (100.0 * (1.0 - (animationFrame / 40.0))));
            }
            if (animationFrame > 30 && animationFrame <= 50) {
                frameOffset = animationFrame - 30;
                rotationAngle = atan2d(-300.0, 100.0) + 180.0;
                Hu3DModelRotSet(loadedModelIds[22], 0, (rotationAngle - 20.0f) * (1.0 - (frameOffset / 20.0)) + 20.0, 0);
            }
            if (animationFrame == 50) {
                Hu3DMotionShiftSet(loadedModelIds[22], loadedMotionIds[23], 0, 10, HU3D_MOTATTR_LOOP);
            }
        }
        if (elapsedFrames >= 30) {
            animationFrame = elapsedFrames - 30;
            if (animationFrame == 0) {
                Hu3DMotionShiftSet(activeCharacterModel, characterMotions[1], 0, 10, HU3D_MOTATTR_LOOP);
            }
            if (animationFrame <= 20) {
                Hu3DModelRotSet(activeCharacterModel, 0, -135.0 - (45.0 * (animationFrame / 20.0)), 0);
            }
            if (animationFrame == 20) {
                Hu3DMotionShiftSet(activeCharacterModel, characterMotions[0], 0, 10, HU3D_MOTATTR_LOOP);
            }
        }
        HuPrcVSleep();
    }
    HuWinMesMaxSizeGet(1, windowSize, MAKE_MESSID(0x1C, 0x05));
    windowId = HuWinExCreateStyled(-10000, 340, windowSize[0], windowSize[1], -1, 1);
    HuWinExAnimIn(windowId);
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    PlayerInputWindowOpen(windowId);
    HuAudFXPlay(55);
    HuAudFXPlay(64);
    HuAudFXPlay(67);
    HuAudFXPlay(70);
    HuAudFXPlay(75);
    HuWinMesSet(windowId, MAKE_MESSID(0x1C, 0x05));
    HuWinMesWait(windowId);
    HuWinExAnimOut(windowId);
    HuWinExCleanup(windowId);
    HuWinComKeyReset();
    Hu3DMotionShiftSet(loadedModelIds[18], loadedMotionIds[21], 0, 10, HU3D_MOTATTR_LOOP);
    HuPrcSleep(5);
    Hu3DMotionShiftSet(loadedModelIds[22], loadedMotionIds[25], 0, 10, HU3D_MOTATTR_LOOP);
    Hu3DMotionShiftSet(loadedModelIds[26], loadedMotionIds[29], 0, 10, HU3D_MOTATTR_LOOP);
    HuPrcSleep(10);
    Hu3DMotionShiftSet(loadedModelIds[30], loadedMotionIds[33], 0, 10, HU3D_MOTATTR_LOOP);
    HuPrcSleep(10);
    Hu3DMotionShiftSet(loadedModelIds[34], loadedMotionIds[37], 0, 10, HU3D_MOTATTR_LOOP);
}

void RewardTransferStart(s16 transferType, s16 sourcePlayer, s16 targetPlayer, s16 transferCount, s32 playTransferSounds);

void RewardTransferProcess(void)
{
    s16 coins;
    s16 stars;
    switch (thirdAwardPlayer) {
        case 0:
            activeTransferProcesses = 2;
            if (GWPlayer[firstAwardPlayer].coins >= GWPlayer[secondAwardPlayer].coins) {
                RewardTransferStart(2, firstAwardPlayer, secondAwardPlayer, GWPlayer[firstAwardPlayer].coins, 1);
                RewardTransferStart(3, secondAwardPlayer, firstAwardPlayer, GWPlayer[secondAwardPlayer].coins, 0);
            }
            else {
                RewardTransferStart(2, firstAwardPlayer, secondAwardPlayer, GWPlayer[firstAwardPlayer].coins, 0);
                RewardTransferStart(3, secondAwardPlayer, firstAwardPlayer, GWPlayer[secondAwardPlayer].coins, 1);
            }
            break;

        case 1:
            activeTransferProcesses = 2;
            if (GWPlayer[firstAwardPlayer].coins >= GWPlayer[secondAwardPlayer].coins) {
                RewardTransferStart(2, firstAwardPlayer, secondAwardPlayer, GWPlayer[firstAwardPlayer].coins, 1);
                RewardTransferStart(3, secondAwardPlayer, firstAwardPlayer, GWPlayer[secondAwardPlayer].coins, 0);
            }
            else {
                RewardTransferStart(2, firstAwardPlayer, secondAwardPlayer, GWPlayer[firstAwardPlayer].coins, 0);
                RewardTransferStart(3, secondAwardPlayer, firstAwardPlayer, GWPlayer[secondAwardPlayer].coins, 1);
            }
            while (activeTransferProcesses != 0) {
                HuPrcVSleep();
            }
            activeTransferProcesses = 2;
            if (GWPlayer[firstAwardPlayer].stars >= GWPlayer[secondAwardPlayer].stars) {
                RewardTransferStart(0, firstAwardPlayer, secondAwardPlayer, GWPlayer[firstAwardPlayer].stars, 1);
                RewardTransferStart(1, secondAwardPlayer, firstAwardPlayer, GWPlayer[secondAwardPlayer].stars, 0);
            }
            else {
                RewardTransferStart(0, firstAwardPlayer, secondAwardPlayer, GWPlayer[firstAwardPlayer].stars, 0);
                RewardTransferStart(1, secondAwardPlayer, firstAwardPlayer, GWPlayer[secondAwardPlayer].stars, 1);
            }
            break;

        case 2:
            activeTransferProcesses = 1;
            RewardTransferStart(0, firstAwardPlayer, secondAwardPlayer, 1, 1);
            break;

        case 3:
            activeTransferProcesses = 2;
            if (GWPlayer[firstAwardPlayer].stars >= GWPlayer[secondAwardPlayer].stars) {
                RewardTransferStart(0, firstAwardPlayer, secondAwardPlayer, GWPlayer[firstAwardPlayer].stars, 1);
                RewardTransferStart(1, secondAwardPlayer, firstAwardPlayer, GWPlayer[secondAwardPlayer].stars, 0);
            }
            else {
                RewardTransferStart(0, firstAwardPlayer, secondAwardPlayer, GWPlayer[firstAwardPlayer].stars, 0);
                RewardTransferStart(1, secondAwardPlayer, firstAwardPlayer, GWPlayer[secondAwardPlayer].stars, 1);
            }
            break;

        case 4:
            activeTransferProcesses = 1;
            if (GWPlayer[firstAwardPlayer].stars < 2) {
                stars = GWPlayer[firstAwardPlayer].stars;
            }
            else {
                stars = 2;
            }
            RewardTransferStart(1, firstAwardPlayer, secondAwardPlayer, stars, 1);
            break;

        case 5:
        case 6:
            activeTransferProcesses = 1;
            if (GWPlayer[firstAwardPlayer].coins < 20) {
                coins = GWPlayer[firstAwardPlayer].coins;
            }
            else {
                coins = 20;
            }
            RewardTransferStart(2, firstAwardPlayer, secondAwardPlayer, coins, 1);
            break;

        default:
            break;
    }
    while (activeTransferProcesses > 0) {
        HuPrcVSleep();
    }
}

typedef struct RewardTransferWork {
    s16 sourcePlayer;
    s16 targetPlayer;
    s16 transferCount;
    s16 activeModelIndices[20];
    s16 modelAnimationFrames[20];
    float arcHeight;
    s32 playTransferSounds;
} RewardTransferWork;

void CoinTransferProcess(void);
void StarTransferProcess(void);

void RewardTransferStart(s16 transferType, s16 sourcePlayer, s16 targetPlayer, s16 transferCount, s32 playTransferSounds)
{
    RewardTransferWork *work;
    HUPROCESS *newProcess;
    float arcHeight;

    if (transferCount == 0) {
        activeTransferProcesses--;
        return;
    }
    switch (transferType) {
        case 0:
            newProcess = HuPrcChildCreate(CoinTransferProcess, 100, 8192, 0, objectManager);
            arcHeight = -100;
            break;

        case 1:
            newProcess = HuPrcChildCreate(CoinTransferProcess, 100, 8192, 0, objectManager);
            arcHeight = -50;
            break;

        case 2:
            newProcess = HuPrcChildCreate(StarTransferProcess, 100, 8192, 0, objectManager);
            arcHeight = -100;
            break;

        case 3:
            newProcess = HuPrcChildCreate(StarTransferProcess, 100, 8192, 0, objectManager);
            arcHeight = -50;
            break;
    }
    work = HuMemDirectMallocNum(HEAP_HEAP, sizeof(RewardTransferWork), HU_MEMNUM_OVL);
    newProcess->user_data = work;
    work->sourcePlayer = sourcePlayer;
    work->targetPlayer = targetPlayer;
    work->transferCount = transferCount;
    work->arcHeight = arcHeight;
    work->playTransferSounds = playTransferSounds;
}

void CoinTransferProcess(void)
{
    float animationDuration;
    float scale;

    RewardTransferWork *transferWork;
    s16 modelSlot;
    s16 transferSlot;
    HUPROCESS *currentProcess;
    s16 remainingTransfers;
    s16 modelId;
    s16 pendingTransfers;
    s16 spawnInterval;
    s16 frameCounter;
    s32 coinSoundId;
    s32 starSoundId;

    Vec sourcePosition;
    Vec targetPosition;
    Vec modelPosition;

    currentProcess = HuPrcCurrentGet();
    transferWork = currentProcess->user_data;
    frameCounter = 0;
    BoardStatusPosGet(transferWork->sourcePlayer, &sourcePosition);
    BoardStatusPosGet(transferWork->targetPlayer, &targetPosition);
    for (modelSlot = 0; modelSlot < 20; modelSlot++) {
        transferWork->activeModelIndices[modelSlot] = -1;
    }
    pendingTransfers = transferWork->transferCount;
    remainingTransfers = pendingTransfers;
    animationDuration = 300 / remainingTransfers;
    if (animationDuration < 15.0f) {
        spawnInterval = 1;
    }
    else {
        spawnInterval = 3;
    }
    animationDuration -= spawnInterval + 1;
    if (animationDuration < 15.0f) {
        animationDuration = 15.0f;
    }
    if (animationDuration > 40.0f) {
        animationDuration = 40.0f;
    }
    if (transferWork->playTransferSounds) {
        coinSoundId = HuAudFXPlay(1864);
        starSoundId = HuAudFXPlay(24);
    }
    while (1) {
        if (pendingTransfers > 0) {
            if (((frameCounter++) & spawnInterval) == 0) {
                for (modelSlot = 0; modelSlot < 20; modelSlot++) {
                    if (coinTransferModelState[modelSlot][1] == 0) {
                        break;
                    }
                }
                if (modelSlot != 20) {
                    for (transferSlot = 0; transferSlot < 20; transferSlot++) {
                        if (transferWork->activeModelIndices[transferSlot] == -1) {
                            coinTransferModelState[modelSlot][1] = 1;
                            transferWork->activeModelIndices[transferSlot] = modelSlot;
                            transferWork->targetPlayerE[transferSlot] = 0;
                            GWStarsSet(transferWork->sourcePlayer, GWPlayer[transferWork->sourcePlayer].stars - 1);
                            pendingTransfers--;
                            break;
                        }
                    }
                }
            }
        }
        for (modelSlot = 0; modelSlot < 20; modelSlot++) {
            if (transferWork->activeModelIndices[modelSlot] == -1) {
                continue;
            }
            modelId = coinTransferModelState[transferWork->activeModelIndices[modelSlot]][0];
            Hu3DModelAttrReset(modelId, HU3D_ATTR_DISPOFF);
            transferWork->targetPlayerE[modelSlot]++;
            if (transferWork->targetPlayerE[modelSlot] > animationDuration) {
                Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
                coinTransferModelState[transferWork->activeModelIndices[modelSlot]][1] = 0;
                transferWork->activeModelIndices[modelSlot] = -1;
                GWStarsSet(transferWork->targetPlayer, GWPlayer[transferWork->targetPlayer].stars + 1);
                remainingTransfers--;
            }
            else {
                scale = 0.1f;
                if (transferWork->targetPlayerE[modelSlot] < animationDuration / 2.0f) {
                    scale = 0.1 * sind((90.0 / (animationDuration / 2.0f)) * transferWork->targetPlayerE[modelSlot]);
                }
                if (transferWork->targetPlayerE[modelSlot] > animationDuration / 2.0f) {
                    scale = 0.1 * cosd((90.0 / (animationDuration / 2.0f)) * (transferWork->targetPlayerE[modelSlot] - (animationDuration / 2.0f)));
                }
                Hu3DModelScaleSet(modelId, scale, scale, scale);
                modelPosition.x = (targetPosition.x - sourcePosition.x) * (transferWork->targetPlayerE[modelSlot] / animationDuration) + sourcePosition.x;
                modelPosition.y = sourcePosition.y + (transferWork->arcHeight * sind((180.0 / animationDuration) * transferWork->targetPlayerE[modelSlot]));
                modelPosition.z = 200;
                Hu3D2Dto3D(&modelPosition, 1, &modelPosition);
                Hu3DModelPosSetV(modelId, &modelPosition);
            }
        }
        if (remainingTransfers <= 0) {
            break;
        }
        HuPrcVSleep();
    }
    HuAudFXPlay(8);
    if (transferWork->playTransferSounds) {
        HuAudFXStop(coinSoundId);
        HuAudFXStop(starSoundId);
    }
    activeTransferProcesses--;
    HuMemDirectFree(currentProcess->user_data);
    HuPrcEnd();
    while (1) {
        HuPrcVSleep();
    }
}

void StarTransferProcess(void)
{
    float animationDuration;
    float scale;

    RewardTransferWork *transferWork;
    s16 modelSlot;
    s16 transferSlot;
    HUPROCESS *currentProcess;
    s16 remainingTransfers;
    s16 modelId;
    s16 pendingTransfers;
    s16 spawnInterval;
    s16 frameCounter;
    s32 coinSoundId;
    s32 starSoundId;

    Vec sourcePosition;
    Vec targetPosition;
    Vec modelPosition;

    currentProcess = HuPrcCurrentGet();
    transferWork = currentProcess->user_data;
    frameCounter = 0;
    BoardStatusPosGet(transferWork->sourcePlayer, &sourcePosition);
    BoardStatusPosGet(transferWork->targetPlayer, &targetPosition);
    for (modelSlot = 0; modelSlot < 20; modelSlot++) {
        transferWork->activeModelIndices[modelSlot] = -1;
    }
    pendingTransfers = transferWork->transferCount;
    remainingTransfers = pendingTransfers;
    animationDuration = 300 / remainingTransfers;
    if (animationDuration < 15.0f) {
        spawnInterval = 1;
    }
    else {
        spawnInterval = 3;
    }
    animationDuration -= spawnInterval + 1;
    if (animationDuration < 15.0f) {
        animationDuration = 15.0f;
    }
    if (animationDuration > 40.0f) {
        animationDuration = 40.0f;
    }
    if (transferWork->playTransferSounds) {
        coinSoundId = HuAudFXPlay(1864);
        starSoundId = HuAudFXPlay(23);
    }
    while (1) {
        if (pendingTransfers > 0) {
            if (((frameCounter++) & spawnInterval) == 0) {
                for (modelSlot = 0; modelSlot < 20; modelSlot++) {
                    if (starTransferModelState[modelSlot][1] == 0) {
                        break;
                    }
                }
                if (modelSlot != 20) {
                    for (transferSlot = 0; transferSlot < 20; transferSlot++) {
                        if (transferWork->activeModelIndices[transferSlot] == -1) {
                            starTransferModelState[modelSlot][1] = 1;
                            transferWork->activeModelIndices[transferSlot] = modelSlot;
                            transferWork->targetPlayerE[transferSlot] = 0;
                            GWCoinsSet(transferWork->sourcePlayer, GWPlayer[transferWork->sourcePlayer].coins - 1);
                            pendingTransfers--;
                            break;
                        }
                    }
                }
            }
        }
        for (modelSlot = 0; modelSlot < 20; modelSlot++) {
            if (transferWork->activeModelIndices[modelSlot] == -1) {
                continue;
            }
            modelId = starTransferModelState[transferWork->activeModelIndices[modelSlot]][0];
            Hu3DModelAttrReset(modelId, HU3D_ATTR_DISPOFF);
            transferWork->targetPlayerE[modelSlot]++;
            if (transferWork->targetPlayerE[modelSlot] > animationDuration) {
                Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
                starTransferModelState[transferWork->activeModelIndices[modelSlot]][1] = 0;
                transferWork->activeModelIndices[modelSlot] = -1;
                GWCoinsSet(transferWork->targetPlayer, GWPlayer[transferWork->targetPlayer].coins + 1);
                remainingTransfers--;
            }
            else {
                scale = 0.1f;
                if (transferWork->targetPlayerE[modelSlot] < animationDuration / 2.0f) {
                    scale = 0.1 * sind((90.0 / (animationDuration / 2.0f)) * transferWork->targetPlayerE[modelSlot]);
                }
                if (transferWork->targetPlayerE[modelSlot] > animationDuration / 2.0f) {
                    scale = 0.1 * cosd((90.0 / (animationDuration / 2.0f)) * (transferWork->targetPlayerE[modelSlot] - (animationDuration / 2.0f)));
                }
                Hu3DModelScaleSet(modelId, scale, scale, scale);
                modelPosition.x = (targetPosition.x - sourcePosition.x) * (transferWork->targetPlayerE[modelSlot] / animationDuration) + sourcePosition.x;
                modelPosition.y = sourcePosition.y + (transferWork->arcHeight * sind((180.0 / animationDuration) * transferWork->targetPlayerE[modelSlot]));
                modelPosition.z = 200;
                Hu3D2Dto3D(&modelPosition, 1, &modelPosition);
                Hu3DModelPosSetV(modelId, &modelPosition);
            }
        }
        if (remainingTransfers <= 0) {
            break;
        }
        HuPrcVSleep();
    }
    HuAudFXPlay(7);
    if (transferWork->playTransferSounds) {
        HuAudFXStop(coinSoundId);
        HuAudFXStop(starSoundId);
    }
    activeTransferProcesses--;
    HuMemDirectFree(currentProcess->user_data);
    HuPrcEnd();
    while (1) {
        HuPrcVSleep();
    }
}

void PinballAssetsLoad(void)
{
    s16 i;
    s16 model;

    Vec pos;
    Vec target;
    Vec up;
    Hu3DShadowCreate(20, 100, 5000);
    Hu3DShadowTPLvlSet(0.3f);
    Hu3DShadowSizeSet(192);
    target.x = target.y = target.z = 0;
    pos.x = -800;
    pos.z = 1600;
    pos.y = 3200;
    up.x = 0;
    up.y = 1;
    up.z = 0;
    Hu3DShadowPosSet(&pos, &up, &target);
    Hu3DReflectMapSet(HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M444, 0x23), HU_MEMNUM_OVL, HEAP_MODEL));
    for (i = 0; i < 4; i++) {
        pinballModelData[i + 47].datanum = pinballModelData[i + 51].datanum = characterRewardMessages[GWPlayerCfg[i].character];
    }
    ModelDataListLoad(pinballModelData);
    SpriteDataListLoad(pinballSprites);
    Hu3DModelHookSet(loadedModelIds[9], "itemhook1", loadedModelIds[3]);
    Hu3DModelHookSet(loadedModelIds[9], "itemhook2", loadedModelIds[5]);
    Hu3DModelHookSet(loadedModelIds[9], "itemhook3", loadedModelIds[7]);
    Hu3DModelHookSet(loadedModelIds[3], "d1_itemhook1", loadedModelIds[47]);
    Hu3DModelHookSet(loadedModelIds[3], "d1_itemhook2", loadedModelIds[48]);
    Hu3DModelHookSet(loadedModelIds[3], "d1_itemhook3", loadedModelIds[49]);
    Hu3DModelHookSet(loadedModelIds[3], "d1_itemhook4", loadedModelIds[50]);
    Hu3DModelHookSet(loadedModelIds[11], "L_itemhook", loadedModelIds[13]);
    Hu3DModelHookSet(loadedModelIds[11], "R_itemhook", loadedModelIds[14]);
    Hu3DModelAttrSet(loadedModelIds[13], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(loadedModelIds[14], HU3D_MOTATTR_LOOP);
    for (i = 0; i < 4; i++) {
        Hu3DModelAttrSet(loadedModelIds[i + 51], HU3D_ATTR_DISPOFF);
    }
    PinballEffectStart(loadedModelIds[4]);
    model = CharModelCreate(activeCharacterId, 2);
    activeCharacterModel = model;
    characterMotions[0] = CharMotionCreate(activeCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00));
    characterMotions[1] = CharMotionCreate(activeCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02));
    characterMotions[2] = CharMotionCreate(activeCharacterId, activeCharacterId + DATA_MAKE_NUM(DATADIR_M444, 0x45));
    characterMotions[3] = CharMotionCreate(activeCharacterId, activeCharacterId + DATA_MAKE_NUM(DATADIR_M444, 0x4D));
    CharMotionSet(activeCharacterId, characterMotions[0]);
    Hu3DModelPosSet(model, 0, 0, 200);
    Hu3DModelAttrSet(model, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(model);
    HuPrcChildCreate(PinballCollisionProcess, 50, 12288, 0, objectManager);
    HuPrcChildCreate(PinballPlayerProcess, 50, 12288, 0, objectManager);
    Hu3DModelAttrSet(loadedModelIds[18], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowSet(loadedModelIds[18]);
    Hu3DModelAttrSet(loadedModelIds[26], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowSet(loadedModelIds[26]);
    Hu3DModelAttrSet(loadedModelIds[30], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowSet(loadedModelIds[30]);
    Hu3DModelAttrSet(loadedModelIds[22], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowSet(loadedModelIds[22]);
    Hu3DModelAttrSet(loadedModelIds[34], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowSet(loadedModelIds[34]);
    Hu3DModelShadowMapSet(loadedModelIds[15]);
    Hu3DModelShadowMapSet(loadedModelIds[9]);
    Hu3DModelShadowMapSet(loadedModelIds[11]);
    Hu3DModelShadowMapSet(loadedModelIds[3]);
    Hu3DModelShadowMapSet(loadedModelIds[5]);
    Hu3DModelShadowMapSet(loadedModelIds[7]);
    CharEffectLayerSet(1);
    CharNpcDustSet(loadedModelIds[18], loadedMotionIds[20], 0, 9);
    CharNpcDustSet(loadedModelIds[26], loadedMotionIds[28], 0, 10);
    CharNpcDustSet(loadedModelIds[22], loadedMotionIds[24], 0, 14);
    CharNpcDustSet(loadedModelIds[34], loadedMotionIds[36], 0, 12);
    PinballStateInit();
    for (i = 0; i < 20; i++) {
        if (i == 0) {
            model = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M444, 0x13));
        }
        else {
            model = Hu3DModelLink(coinTransferModelState[0][0]);
        }
        coinTransferModelState[i][0] = model;
        coinTransferModelState[i][1] = 0;
        Hu3DModelAttrSet(model, HU3D_ATTR_DISPOFF);
        Hu3DModelScaleSet(model, 0.2, 0.2, 0.2);
        Hu3DModelLayerSet(model, 7);
    }
    for (i = 0; i < 20; i++) {
        if (i == 0) {
            model = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M444, 0x12));
        }
        else {
            model = Hu3DModelLink(starTransferModelState[0][0]);
        }
        starTransferModelState[i][0] = model;
        starTransferModelState[i][1] = 0;
        Hu3DModelAttrSet(model, HU3D_ATTR_DISPOFF);
        Hu3DModelScaleSet(model, 0.2, 0.2, 0.2);
        Hu3DModelLayerSet(model, 7);
    }
    ShadowLayerHookInstall();
}

void PlayerInputWindowOpen(s16 window)
{
    s16 i;
    WindowData *winPtr = &winData[window];
    winPtr->active_pad = 1 << activeControllerId;
    if (GWPlayerCfg[activePlayerConfigIndex].iscom) {
        for (i = 0; i < 10; i++) {
            HuWinComKeyWait(256, 256, 256, 256, GWMessDelayGet());
        }
    }
}

void GameExitMonitorProcess(void)
{
    while (1) {
        if (cameraTransitionActive || omSysExitReq) {
            break;
        }
        HuPrcVSleep();
    }
    WipeColorSet(255, 255, 255);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    HuAudFadeOut(1000);
    while (WipeStatGet()) {
        HuPrcVSleep();
    }
    BoardStatusKill();
    omOvlReturnEx(1, 1);
    HuPrcEnd();
}

u16 cameraIds[] = { 1, 2 };

void CameraViewUpdate(OMOBJ *object)
{
    s16 i;
    for (i = 0; i < 1; i++) {
        Vec pos, target, up;
        float x, y, z;

        x = cameraRotation[i].x;
        y = cameraRotation[i].y;
        z = cameraRotation[i].z;

        pos.x = (((sind(y) * cosd(x)) * cameraZoom[i]) + cameraPosition[i].x);
        pos.y = (-sind(x) * cameraZoom[i]) + cameraPosition[i].y;
        pos.z = ((cosd(y) * cosd(x)) * cameraZoom[i]) + cameraPosition[i].z;
        target.x = cameraPosition[i].x;
        target.y = cameraPosition[i].y;
        target.z = cameraPosition[i].z;
        up.x = sind(y) * sind(x);
        up.y = cosd(x);
        up.z = cosd(y) * sind(x);
        Hu3DCameraPosSet(cameraIds[i], pos.x, pos.y, pos.z, up.x, up.y, up.z, target.x, target.y, target.z);
    }
}

void CameraViewInterpolate(void);
void CameraControlProcess(OMOBJ *object)
{
    Vec pos;
    Vec offset;
    Vec dir;
    Vec yOffset;

    f32 zRot;
    s8 stickPos;
    if (cameraControlEnabled == 0) {
        CameraViewInterpolate();
        return;
    }
    cameraRotation[0].y += 0.1f * HuPadStkX[0];
    cameraRotation[0].x += 0.1f * HuPadStkY[0];
    cameraZoom[0] += HuPadTrigL[0] / 2;
    cameraZoom[0] -= HuPadTrigR[0] / 2;
    if (HuPadBtnDown[0] & PAD_BUTTON_B) {
        OSReport("%f,%f,%f\n", cameraRotation[0].x, cameraRotation[0].y, cameraRotation[0].z);
        OSReport("%f,%f,%f\n", cameraPosition[0].x, cameraPosition[0].y, cameraPosition[0].z);
        OSReport("%f\n", cameraZoom[0]);
    }
    if (cameraZoom[0] < 100.0f) {
        cameraZoom[0] = 100.0f;
    }
    pos.x = cameraPosition[0].x + (cameraZoom[0] * (sind(cameraRotation[0].y) * cosd(cameraRotation[0].x)));
    pos.y = (cameraPosition[0].y + (cameraZoom[0] * -sind(cameraRotation[0].x)));
    pos.z = (cameraPosition[0].z + (cameraZoom[0] * (cosd(cameraRotation[0].y) * cosd(cameraRotation[0].x))));
    offset.x = cameraPosition[0].x - pos.x;
    offset.y = cameraPosition[0].y - pos.y;
    offset.z = cameraPosition[0].z - pos.z;
    dir.x = (sind(cameraRotation[0].y) * sind(cameraRotation[0].x));
    dir.y = cosd(cameraRotation[0].x);
    dir.z = (cosd(cameraRotation[0].y) * sind(cameraRotation[0].x));
    zRot = cameraRotation[0].z;
    yOffset.x = dir.x * (offset.x * offset.x + (1.0f - offset.x * offset.x) * cosd(zRot))
        + dir.y * (offset.x * offset.y * (1.0f - cosd(zRot)) - offset.z * sind(zRot))
        + dir.z * (offset.x * offset.z * (1.0f - cosd(zRot)) + offset.y * sind(zRot));

    yOffset.y = dir.y * (offset.y * offset.y + (1.0f - offset.y * offset.y) * cosd(zRot))
        + dir.x * (offset.x * offset.y * (1.0f - cosd(zRot)) + offset.z * sind(zRot))
        + dir.z * (offset.y * offset.z * (1.0f - cosd(zRot)) - offset.x * sind(zRot));

    yOffset.z = dir.z * (offset.z * offset.z + (1.0f - offset.z * offset.z) * cosd(zRot))
        + (dir.x * (offset.x * offset.z * (1.0 - cosd(zRot)) - offset.y * sind(zRot))
            + dir.y * (offset.y * offset.z * (1.0 - cosd(zRot)) + offset.x * sind(zRot)));

    VECCrossProduct(&dir, &offset, &offset);
    VECNormalize(&offset, &offset);
    stickPos = (HuPadSubStkX[0] & 0xF8);
    if (stickPos != 0) {
        cameraPosition[0].x += 0.05f * (offset.x * stickPos);
        cameraPosition[0].y += 0.05f * (offset.y * stickPos);
        cameraPosition[0].z += 0.05f * (offset.z * stickPos);
    }
    VECNormalize(&yOffset, &offset);
    stickPos = -(HuPadSubStkY[0] & 0xF8);
    if (stickPos != 0) {
        cameraPosition[0].x += 0.05f * (offset.x * stickPos);
        cameraPosition[0].y += 0.05f * (offset.y * stickPos);
        cameraPosition[0].z += 0.05f * (offset.z * stickPos);
    }
}

void CameraViewInterpolate(void)
{
    Vec dist;
    float distZoom;
    if (cameraTransitionActive != 0) {
        return;
    }
    VECSubtract(&cameraTargetRotation, &cameraRotation[0], &dist);
    if (VECMag(&dist) > 0.2) {
        VECScale(&dist, &dist, 0.05f);
        VECAdd(&cameraRotation[0], &dist, &cameraRotation[0]);
    }
    else {
        cameraRotation[0] = cameraTargetRotation;
    }
    VECSubtract(&cameraTargetPosition, &cameraPosition[0], &dist);
    if (VECMag(&dist) > 0.2) {
        VECScale(&dist, &dist, 0.05f);
        VECAdd(&cameraPosition[0], &dist, &cameraPosition[0]);
    }
    else {
        cameraPosition[0] = cameraTargetPosition;
    }
    distZoom = cameraTargetZoom - cameraZoom[0];
    if (sqrtf(distZoom * distZoom) > 0.2) {
        distZoom *= 0.05f;
        cameraZoom[0] += distZoom;
    }
    else {
        cameraZoom[0] = cameraTargetZoom;
    }
}

void CameraViewTransition(s16 view)
{
    cameraTargetRotation = cameraViewPresets[view].rot;
    cameraTargetPosition = cameraViewPresets[view].pos;
    cameraTargetZoom = cameraViewPresets[view].zoom;
}

void CameraViewSet(s16 view)
{
    cameraRotation[0] = cameraViewPresets[view].rot;
    cameraTargetRotation = cameraRotation[0];
    cameraPosition[0] = cameraViewPresets[view].pos;
    cameraTargetPosition = cameraPosition[0];
    cameraTargetZoom = cameraZoom[0] = cameraViewPresets[view].zoom;
}

float CameraZoomGet(void)
{
    float zoom = cameraTargetZoom - cameraZoom[0];
    return sqrtf(zoom * zoom);
}
