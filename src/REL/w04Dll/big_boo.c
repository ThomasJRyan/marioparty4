#include "REL/w04Dll.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/msm.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "game/board/audio.h"
#include "game/board/com.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/ui.h"
#include "game/board/view.h"
#include "game/board/window.h"

#include "dolphin.h"
#include "ext_math.h"

#ifndef __MWERKS__
#include "game/frand.h"
#include <stdlib.h>
#endif

// Remove this and include frand.h header once we decide what to do with frandmod.
#define frand8() (((s32)frand() & 0xFF))

static void RunW04BigBooEncounter(void);
static void ChooseW04BigBooTheft(void);
static void PrepareW04BigBooTheftScene(void);
static void RestoreW04BigBooScene(void);
static void DestroyW04BigBooLights(void);
static void AnimateW04BigBooLights(void);
static void StealW04BigBooCoins(void);
static void StealW04BigBooStars(void);
static void FinishW04BigBooTheft(void);
static void ResetW04BigBooModels(void);
static void PlayW04BigBooMusic(void);
static void UpdateW04BigBooModels(void);
static void FinishW04BigBooEvent(void);
static s16 CreateW04BigBooParticleEffect(void);
static void KillW04BigBooParticleEffect(s16 particleManagerId);

static s16 bigBooTargetPlayer;
static s16 bigBooHeadModelId;
static s16 bigBooBodyModelId;
static s16 bigBooBaseModelId;
static HUPROCESS *bigBooEventProcess;
static s16 bigBooModelId;
static s16 bigBooTargetSpace;
static s16 bigBooSceneModelId;
static Vec playerStartPositions[4];
static Vec playerStartRotations[4];
static s16 theftItemModelIds[105];
static s16 particleManagerCount;
static s16 theftParticleManagerIds[3];
static s16 bigBooPlayerMotionIds[4][4];
static Vec bigBooTargetPosition;
static HUPROCESS *bigBooLightProcess;
static ANIMDATA *bigBooAnimationData;

static char *bigBooModelHookNames[] = {
    "picture", "picture2", "clock", "hari", "papa"
};

static s32 bigBooModelResources[] = {
    DATA_MAKE_NUM(DATADIR_W04, 25),
    DATA_MAKE_NUM(DATADIR_W04, 26),
    DATA_MAKE_NUM(DATADIR_W04, 27),
    DATA_MAKE_NUM(DATADIR_W04, 28),
    -1
};

static s32 characterMotionDirectories[8] = {
    DATADIR_MARIOMOT,
    DATADIR_LUIGIMOT,
    DATADIR_PEACHMOT,
    DATADIR_YOSHIMOT,
    DATADIR_WARIOMOT,
    DATADIR_DONKEYMOT,
    DATADIR_DAISYMOT,
    DATADIR_WALUIGIMOT
};

static s32 bigBooPlayerMotionOffsets[] = {
    26, 28, 93, 12
};

static s16 coinTheftAnimationSteps[][2] = {
    {  0, 0 }, {  6, 1 }, {  7, 3 }, { 12, 2 },
    { 15, 3 }, { 18, 4 }, { 20, 6 }, { 23, 5 },
    { 26, 6 }, { 28, 7 }, { 29, 8 }, { -1, 9 }
};

void W04BigBooSetup(void) {
    bigBooBaseModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 19), NULL, 0);
    BoardModelHookSet(w04BoardModelIds[4], bigBooModelHookNames[0], bigBooBaseModelId);
    bigBooHeadModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 20), NULL, 0);
    BoardModelHookSet(w04BoardModelIds[4], bigBooModelHookNames[2], bigBooHeadModelId);
    bigBooBodyModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 21), NULL, 0);
    BoardModelHookSet(bigBooHeadModelId, bigBooModelHookNames[3], bigBooBodyModelId);
    BoardModelAttrSet(bigBooBodyModelId, 0x40000002);
    BoardModelAttrSet(bigBooBaseModelId, 0x40000002);
    BoardModelMotionTimeSet(bigBooBaseModelId, 20.0f);
    BoardModelAttrSet(bigBooHeadModelId, 0x40000003);
    BoardModelMotionSpeedSet(bigBooBaseModelId, 0.5f);
    if (w04BoardEventState->booEncounterCount != 0) {
        BoardModelMotionTimeSet(bigBooHeadModelId, 0.0f);
        BoardModelMotionSpeedSet(bigBooHeadModelId, 0.5f);
        BoardModelAttrReset(bigBooHeadModelId, 0x40000002);
        BoardModelMotionTimeSet(bigBooBodyModelId, 120.0f * w04BoardEventState->booEncounterCount);
        BoardModelMotionTimeSet(bigBooBaseModelId, 25.0f + 30.0f * w04BoardEventState->booEncounterCount);
    }
}

static void RunW04BigBooEncounter(void) {
    Vec bigBooPosition;
    Vec cameraOffset;
    s32 nextSpace;

    w04BoardEventState->booEncounterCount++;
    if (w04BoardEventState->booEncounterCount == 3) {
        bigBooTargetSpace = GWPlayer[bigBooTargetPlayer].space_curr;
        nextSpace = BoardSpaceLinkFlagSearch(0, bigBooTargetSpace, 0x02000000);
        BoardPlayerMoveToAsync(bigBooTargetPlayer, nextSpace);
        while (GWPlayer[bigBooTargetPlayer].moving) {
            HuPrcVSleep();
        }
        bigBooModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 24), bigBooModelResources, 0);
        BoardModelMotionStart(bigBooModelId, 4, 0);
        BoardModelAttrSet(bigBooModelId, 0x40000002);
        Hu3DModelObjPosGet(BoardModelIDGet(w04BoardModelIds[4]), bigBooModelHookNames[4], &bigBooPosition);
        BoardModelPosSetV(bigBooModelId, &bigBooPosition);
        cameraOffset.x = cameraOffset.y = cameraOffset.z = 3.0f;
        BoardModelScaleSetV(bigBooModelId, &cameraOffset);
        BoardModelVisibilitySet(bigBooModelId, 0);
    }
    BoardPlayerMotBlendSet(bigBooTargetPlayer, 180, 0xF);
    while (!BoardPlayerMotBlendCheck(bigBooTargetPlayer)) {
        HuPrcVSleep();
    }
    UpdateW04BigBooModels();
    switch (w04BoardEventState->booEncounterCount) {
        case 1:
            HuAudFXPlay(0x4E);
            omVibrate(bigBooTargetPlayer, 0x7FFF, 6, 6);
            BoardModelMotionTimeSet(bigBooBaseModelId, 30.0f);
            BoardModelAttrReset(bigBooBaseModelId, 0x40000002);
            HuPrcSleep(40);
            BoardModelAttrSet(bigBooBaseModelId, 0x40000002);
            HuPadRumbleStop(GWPlayer[bigBooTargetPlayer].port);
            BoardWinCreate(0, MAKE_MESSID(32, 26), -1);
            BoardWinWait();
            BoardWinKill();
            break;
        case 2:
            HuAudFXPlay(0x4E);
            omVibrate(bigBooTargetPlayer, 0x7FFF, 6, 6);
            BoardModelMotionTimeSet(bigBooBaseModelId, 60.0f);
            BoardModelAttrReset(bigBooBaseModelId, 0x40000002);
            HuPrcSleep(40);
            BoardModelAttrSet(bigBooBaseModelId, 0x40000002);
            HuPadRumbleStop(GWPlayer[bigBooTargetPlayer].port);
            BoardWinCreate(0, MAKE_MESSID(32, 27), -1);
            BoardWinWait();
            BoardWinKill();
            break;
        case 3:
            BoardWinCreate(0, MAKE_MESSID(32, 28), -1);
            BoardWinWait();
            BoardWinKill();
            HuAudFXPlay(0x4E);
            bigBooPosition.x = 5.0f;
            bigBooPosition.y = 0.0f;
            bigBooPosition.z = 0.0f;
            cameraOffset.x = 0.0f;
            cameraOffset.y = 300.0f;
            cameraOffset.z = -200.0f;
            BoardCameraMotionStartEx(w04BoardModelIds[4], &bigBooPosition, &cameraOffset, 1400.0f, -1.0f, 30);
            BoardCameraMotionWait();
            HuPrcSleep(9);
            HuAudFXPlay(0x464);
            BoardModelMotionTimeSet(bigBooBaseModelId, 90.0f);
            BoardModelAttrReset(bigBooBaseModelId, 0x40000002);
            HuPrcSleep(10);
            omVibrate(bigBooTargetPlayer, 12, 4, 2);
            BoardModelVisibilitySet(bigBooModelId, 1);
            BoardModelAttrReset(bigBooModelId, 0x40000002);
            HuPrcSleep(39);
            BoardModelPosGet(bigBooModelId, &bigBooPosition);
            BoardModelScaleGet(bigBooModelId, &cameraOffset);
            while (!BoardModelMotionEndCheck(bigBooModelId)) {
                bigBooPosition.z += 15.0f;
                BoardModelPosSetV(bigBooModelId, &bigBooPosition);
                HuPrcVSleep();
            }
            BoardModelMotionStart(bigBooModelId, 1, 0x40000001);
            break;
    }
    if (w04BoardEventState->booEncounterCount == 3) {
        ChooseW04BigBooTheft();
        BoardCameraViewSet(1);
        BoardPlayerMoveToAsync(bigBooTargetPlayer, bigBooTargetSpace);
        while (GWPlayer[bigBooTargetPlayer].moving) {
            HuPrcVSleep();
        }
        BoardPlayerIdleSet(bigBooTargetPlayer);
        BoardCameraMotionWait();
    } else {
        HuPrcSleep(29);
    }
    HuPrcEnd();
}

static void ChooseW04BigBooTheft(void) {
    float modelFadeAlpha;
    s32 availableStars;
    s32 coinAnimationDelay;
    s32 availableCoins;
    s32 theftChoice;
    s32 i;

    BoardWinCreate(2, MAKE_MESSID(32, 3), 9);
    BoardWinWait();
    BoardWinKill();
    if (BoardPlayerCoinsGet(bigBooTargetPlayer) < 15) {
        BoardWinCreate(2, MAKE_MESSID(32, 4), 9);
        BoardWinWait();
        BoardWinKill();
        BoardAudSeqFadeOut(1, 1000);
        while (msmMusGetStatus(1) != 0) {
            HuPrcVSleep();
        }
        BoardAudSeqPause(0, 0, 1000);
        FinishW04BigBooTheft();
        return;
    }
    do {
        availableStars = 0;
        for (i = 0; i < 4; i++) {
            if (i != bigBooTargetPlayer) {
                availableStars += GWStarsGet(i);
            }
        }
        availableCoins = 0;
        for (i = 0; i < 4; i++) {
            if (i != bigBooTargetPlayer) {
                availableCoins += BoardPlayerCoinsGet(i);
            }
        }
        BoardWinCreateChoice(4, MAKE_MESSID(32, 5), 9, 0);
        if (BoardPlayerCoinsGet(bigBooTargetPlayer) < 150 || availableStars == 0) {
            BoardWinChoiceDisable(1);
        }
        if (availableCoins == 0) {
            BoardWinChoiceDisable(0);
        }
        if (GWPlayer[bigBooTargetPlayer].com) {
            if (availableStars != 0 && BoardPlayerCoinsGet(bigBooTargetPlayer) >= 150) {
                BoardComKeySetDown();
            } else {
                BoardComKeySetUp();
            }
        }
        BoardWinWait();
        BoardWinKill();
        theftChoice = BoardWinChoiceGet();
        coinAnimationDelay = 6;
        switch (theftChoice) {
            case 1:
                coinAnimationDelay /= 4;
                for (i = 0; i < 135; i++) {
                    BoardPlayerCoinsAdd(bigBooTargetPlayer, -1);
                    HuAudFXPlay(0xE);
                    HuPrcSleep(coinAnimationDelay);
                }
                /* fallthrough */
            case 0:
                for (i = 0; i < 15; i++) {
                    BoardPlayerCoinsAdd(bigBooTargetPlayer, -1);
                    HuAudFXPlay(0xE);
                    HuPrcSleep(coinAnimationDelay);
                }
                HuAudFXPlay(0xF);
                HuAudFXPlay(0x4E);
                BoardWinCreate(2, MAKE_MESSID(32, 7), 9);
                BoardWinWait();
                BoardWinKill();
                modelFadeAlpha = 255.0f;
                for (i = 0; i < 60; i++) {
                    modelFadeAlpha -= 4.25f;
                    if (modelFadeAlpha < 0.0f) {
                        modelFadeAlpha = 0.0f;
                    }
                    BoardModelAlphaSet(bigBooModelId, modelFadeAlpha);
                    HuPrcVSleep();
                }
                BoardModelAlphaSet(bigBooModelId, 0);
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                WipeColorSet(0, 0, 0);
                while (WipeStatGet() != 0) {
                    HuPrcVSleep();
                }
                BoardModelAlphaSet(bigBooModelId, 0xFF);
                if (theftChoice == 0) {
                    StealW04BigBooCoins();
                } else {
                    StealW04BigBooStars();
                }
                break;
            case -1:
            case 2:
                BoardWinCreate(2, MAKE_MESSID(32, 6), 9);
                BoardWinWait();
                BoardWinKill();
                BoardAudSeqFadeOut(1, 1000);
                while (msmMusGetStatus(1) != 0) {
                    HuPrcVSleep();
                }
                BoardAudSeqPause(0, 0, 1000);
                break;
            case 3:
                BoardViewMapExec(bigBooTargetPlayer);
                break;
        }
    } while (theftChoice == 3);
    FinishW04BigBooTheft();
}

static void PrepareW04BigBooTheftScene(void) {
    Vec playerPosition;
    s16 otherPlayerIds[4];
    s16 otherPlayerCount;
    s16 playerEffectCountdown;
    s32 characterMotionDirectory;
    s32 motionIndex;
    s32 i;

    otherPlayerCount = 0;
    for (i = 0; i < 4; i++) {
        if (i != bigBooTargetPlayer) {
            BoardPlayerPosGet(i, &playerStartPositions[i]);
            BoardPlayerRotGet(i, &playerStartRotations[i]);
            otherPlayerIds[otherPlayerCount++] = i;
        }
    }
    BoardModelHideSetAll(1);
    BoardSpaceHide(1);
    for (i = 0; i < 4; i++) {
        BoardStatusShowSet(i, 0);
    }
    BoardLightResetExec();
    bigBooSceneModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 22), NULL, 0);
    BoardModelAttrSet(bigBooSceneModelId, 0x40000001);
    playerPosition.y = 0.0f;
    for (i = -1; i < 2; i++) {
        playerPosition.z = 220.0f + -20.0f * abs(i);
        playerPosition.x = 150.0f * i;
        BoardPlayerPosSetV(otherPlayerIds[i + 1], &playerPosition);
        BoardPlayerRotYSet(otherPlayerIds[i + 1], 180.0f);
        BoardModelVisibilitySet(BoardPlayerModelGet(otherPlayerIds[i + 1]), 1);
        characterMotionDirectory = characterMotionDirectories[GWPlayer[otherPlayerIds[i + 1]].character];
        for (motionIndex = 0; motionIndex < 4; motionIndex++) {
            bigBooPlayerMotionIds[otherPlayerIds[i + 1]][motionIndex] = BoardPlayerMotionCreate(otherPlayerIds[i + 1], characterMotionDirectory | bigBooPlayerMotionOffsets[motionIndex]);
        }
        BoardPlayerMotionStart(otherPlayerIds[i + 1], bigBooPlayerMotionIds[otherPlayerIds[i + 1]][0], 0x40000001);
    }
    BoardModelPosGet(bigBooModelId, &bigBooTargetPosition);
    BoardModelVisibilitySet(bigBooModelId, 1);
    playerPosition.x = playerPosition.z = 0.0f;
    playerPosition.y = 800.0f;
    BoardModelPosSetV(bigBooModelId, &playerPosition);
    playerPosition.x = playerPosition.y = playerPosition.z = 4.0f;
    BoardModelScaleSetV(bigBooModelId, &playerPosition);
    BoardModelMotionStart(bigBooModelId, 0, 0);
    BoardCameraMoveSet(0);
    BoardCameraTargetModelSet(bigBooSceneModelId);
    BoardCameraOffsetSet(0.0f, 200.0f, 0.0f);
    BoardCameraXRotZoomSet(1600.0f, -5.0f);
    bigBooLightProcess = HuPrcChildCreate(AnimateW04BigBooLights, 0x2003, 0x1000, 0, boardMainProc);
    HuPrcDestructorSet2(bigBooLightProcess, DestroyW04BigBooLights);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    HuPrcSleep(0x78);
    for (i = 0; i < 3; i++) {
        BoardPlayerModelAttrReset(otherPlayerIds[i], 0x40000001);
        BoardPlayerMotionShiftSet(otherPlayerIds[i], bigBooPlayerMotionIds[otherPlayerIds[i]][1], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    }
    BoardModelPosGet(bigBooModelId, &playerPosition);
    playerEffectCountdown = 10;
    for (i = 0; i < 60; i++) {
        if (--playerEffectCountdown == 0) {
            HuAudFXPlay(0x4E);
            for (motionIndex = 0; motionIndex < 3; motionIndex++) {
                PlayerFXPlay(otherPlayerIds[motionIndex], 0x12F);
            }
        }
        playerPosition.y -= 10.0f;
        BoardModelPosSetV(bigBooModelId, &playerPosition);
        HuPrcVSleep();
    }
    for (i = 0; i < 3; i++) {
        BoardPlayerMotionShiftSet(otherPlayerIds[i], 1, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    }
}

static void RestoreW04BigBooScene(void) {
    Vec cameraRotation;
    Vec cameraOffset;
    s32 i;
    s32 j;

    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 30);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    BoardModelKill(bigBooSceneModelId);
    for (i = 0; i < 4; i++) {
        if (i != bigBooTargetPlayer) {
            BoardPlayerPosSetV(i, &playerStartPositions[i]);
            BoardPlayerRotSetV(i, &playerStartRotations[i]);
            for (j = 0; j < 4; j++) {
                BoardPlayerMotionKill(i, bigBooPlayerMotionIds[i][j]);
            }
            BoardPlayerMotionStart(i, 1, 0x40000001);
        }
    }
    HuPrcKill(bigBooLightProcess);
    HuPrcVSleep();
    BoardModelHideSetAll(0);
    BoardSpaceHide(0);
    for (i = 0; i < 4; i++) {
        BoardStatusShowSet(i, 1);
    }
    BoardLightSetExec();
    BoardCameraMoveSet(0);
    cameraRotation.x = 5.0f;
    cameraRotation.y = 0.0f;
    cameraRotation.z = 0.0f;
    cameraOffset.x = 0.0f;
    cameraOffset.y = 220.0f;
    cameraOffset.z = -200.0f;
    BoardCameraMotionStartEx(w04BoardModelIds[4], &cameraRotation, &cameraOffset, 1400.0f, -1.0f, 0);
    BoardCameraMoveSet(1);
    BoardModelMotionStart(bigBooModelId, 1, 0x40000001);
    cameraOffset.x = cameraOffset.y = cameraOffset.z = 3.0f;
    BoardModelScaleSetV(bigBooModelId, &cameraOffset);
    BoardModelPosSetV(bigBooModelId, &bigBooTargetPosition);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
}

static void DestroyW04BigBooLights(void) {
    s16 firstLightId;
    s16 secondLightId;

    firstLightId = ((s32) HuPrcCurrentGet()->user_data) >> 16;
    secondLightId = (u16) HuPrcCurrentGet()->user_data;
    Hu3DGLightKill(firstLightId);
    Hu3DGLightKill(secondLightId);
}

static void AnimateW04BigBooLights(void) {
    Vec lightPosition;
    Vec lightDirection;
    GXColor lightColor;
    float lightScale;
    float lightAngle;
    s16 firstLightId;
    s16 secondLightId;

    firstLightId = Hu3DGLightCreate(0.0f, 1000.0f, 0.0f, 0.0f, 0.9f, -0.1f, 0xFF, 0x64, 0xC8);
    Hu3DGLightInfinitytSet(firstLightId);
    Hu3DGLightStaticSet(firstLightId, 1);
    lightColor.r = 0xCD;
    lightColor.g = 0x32;
    lightColor.b = 0x96;
    lightPosition.x = lightPosition.y = lightPosition.z = 0.0f;
    lightDirection.x = 0.0f;
    lightDirection.y = -1.0f;
    lightDirection.z = 0.0f;
    secondLightId = Hu3DGLightCreateV(&lightPosition, &lightDirection, &lightColor);
    Hu3DGLightInfinitytSet(secondLightId);
    Hu3DGLightStaticSet(secondLightId, 1);
    HuPrcCurrentGet()->user_data = (void*) (((firstLightId << 16) & 0xFFFF0000) | (secondLightId & 0xFFFF));
    lightAngle = 0.0f;
    lightScale = lightAngle;
    while (TRUE) {
        lightDirection.x = cosd(lightAngle - 180.0f);
        lightDirection.y = sind(lightAngle);
        lightDirection.z = cosd(lightAngle) * sind(lightAngle);
        lightAngle += 3.0f + 6.0f * frand8() * 0.003921569f;
        if (lightAngle >= 360.0f) {
            lightAngle -= 360.0f;
        }
        Hu3DGLightPosSetV(secondLightId, &lightPosition, &lightDirection);
        HuPrcVSleep();
    }
}

static void StealW04BigBooCoins(void) {
    Vec coinParticlePositions[105];
    Vec coinParticleVelocities[105];
    float coinParticleAges[105];
    float coinParticleAngles[105];
    u8 coinParticleActive[105];
    Vec bigBooPosition;
    float coinParticleLaunchAngles[3] = { 270.0f, 0.0f, 90.0f };
    float messageWindowSize[2];
    float coinTheftAnimationPhase;
    char stolenCoinCountText[16];
    s16 playerButtonPressCounts[4];
    s16 playerMotionActive[4];
    s16 coinsToStealByPlayer[4];
    s16 coinTheftReductionByPlayer[4];
    u16 pressedButtons;
    u16 controllerPort;
    s16 computerButtonPressChance;
    s16 totalCoinsStolen;
    s16 launchAngleIndex;
    s16 messageWindowId;
    s16 playerEffectTimer;
    s16 selectedMotionIndex;
    s32 coinTheftTarget;
    s32 i;
    s32 j;
    s32 k;

    PrepareW04BigBooTheftScene();
    HuWinMesMaxSizeGet(1, messageWindowSize, MAKE_MESSID(32, 8));
    messageWindowId = HuWinCreate(-10000.0f, 32.0f, messageWindowSize[0], messageWindowSize[1], 1);
    HuWinMesSet(messageWindowId, MAKE_MESSID(32, 8));
    HuWinMesSpeedSet(messageWindowId, 0);
    BoardModelMotionStart(bigBooModelId, 3, 0x40000001);
    for (i = 0; i < 4; i++) {
        playerButtonPressCounts[i] = 0;
        playerMotionActive[i] = 0;
    }
    BoardModelPosGet(bigBooModelId, &bigBooPosition);
    playerEffectTimer = 48;
    for (i = 0; i < 120; i++) {
        if (--playerEffectTimer == 0) {
            playerEffectTimer = 48;
            for (j = 0; j < 4; j++) {
                if (j != bigBooTargetPlayer) {
                    PlayerFXPlay(j, 0x130);
                }
            }
        }
        for (j = 0; j < 4; j++) {
            if (j == bigBooTargetPlayer) {
                continue;
            }
            if (GWPlayer[j].com) {
                switch (GWPlayer[j].diff) {
                    case 0:
                        computerButtonPressChance = 12;
                        break;
                    case 1:
                        computerButtonPressChance = 22;
                        break;
                    case 2:
                        computerButtonPressChance = 50;
                        break;
                    case 3:
                        computerButtonPressChance = 1000;
                        break;
                }
                if (frandmod(100) < computerButtonPressChance) {
                    pressedButtons = 0x100;
                } else {
                    pressedButtons = 0;
                }
            } else {
                controllerPort = GWPlayer[j].port;
                pressedButtons = HuPadBtnDown[controllerPort];
            }
            if (pressedButtons & 0x100) {
                playerButtonPressCounts[j]++;
            }
            if (playerMotionActive[j] != 0) {
                if (BoardPlayerMotionEndCheck(j)) {
                    playerMotionActive[j] = 0;
                    BoardPlayerMotionStart(j, 1, 0x40000001);
                }
            } else if (pressedButtons & 0x100) {
                playerMotionActive[j] = 1;
                BoardPlayerMotionStart(j, bigBooPlayerMotionIds[j][2], 0);
            }
        }
        HuPrcVSleep();
    }
    for (i = 0; i < 4; i++) {
        if (i == bigBooTargetPlayer) {
            continue;
        }
        BoardPlayerMotionStart(i, bigBooPlayerMotionIds[i][3], 0);
        OSReport("%d:%dhit  ", i, playerButtonPressCounts[i]);
        omVibrate(i, 12, 4, 2);
    }
    OSReport("\n");
    HuWinKill(messageWindowId);
    totalCoinsStolen = 0;
    selectedMotionIndex = GWSystem.turn;
    if (selectedMotionIndex <= 5) {
        coinTheftTarget = 15;
    } else if (selectedMotionIndex <= 15) {
        coinTheftTarget = 25;
    } else if (selectedMotionIndex <= 25) {
        coinTheftTarget = 27;
    } else if (selectedMotionIndex <= 35) {
        coinTheftTarget = 30;
    } else if (selectedMotionIndex <= 45) {
        coinTheftTarget = 32;
    } else {
        coinTheftTarget = 35;
    }
    for (i = 0; i < 4; i++) {
        if (i == bigBooTargetPlayer) {
            continue;
        }
        j = 0;
        while (TRUE) {
            if (coinTheftAnimationSteps[j][0] == -1) {
                coinTheftReductionByPlayer[i] = coinTheftAnimationSteps[j][1];
                break;
            } else if (playerButtonPressCounts[i] <= coinTheftAnimationSteps[j][0]) {
                coinTheftReductionByPlayer[i] = coinTheftAnimationSteps[j][1];
                break;
            }
            j++;
        }
        coinsToStealByPlayer[i] = coinTheftTarget - coinTheftReductionByPlayer[i];
        if (BoardPlayerCoinsGet(i) < coinsToStealByPlayer[i]) {
            coinsToStealByPlayer[i] = BoardPlayerCoinsGet(i);
        }
        totalCoinsStolen += coinsToStealByPlayer[i];
        BoardPlayerCoinsAdd(i, -coinsToStealByPlayer[i]);
        OSReport("%d:%dlost  ", i, coinsToStealByPlayer[i]);
    }
    OSReport("\n");
    for (i = 0; i < totalCoinsStolen; i++) {
        theftItemModelIds[i] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 10), 0, 1);
        BoardModelVisibilitySet(theftItemModelIds[i], 0);
        coinParticleActive[i] = 0;
    }
    for (i = 0; i < 10; i++) {
        bigBooPosition.z += 21.0f - i;
        BoardModelPosSetV(bigBooModelId, &bigBooPosition);
        HuPrcVSleep();
    }
    for (i = 0; i < 4; i++) {
        if (i == bigBooTargetPlayer) {
            continue;
        }
        omVibrate(i, 12, 4, 2);
        PlayerFXPlay(i, 0x131);
    }
    HuAudFXPlay(0x4E);
    computerButtonPressChance = 0;
    controllerPort = totalCoinsStolen;
    while (TRUE) {
        if (computerButtonPressChance == 0) {
            launchAngleIndex = 0;
            for (i = 0; i < 4; i++) {
                if (i == bigBooTargetPlayer) {
                    continue;
                }
                for (j = 0; j < 3; j++) {
                    if (coinsToStealByPlayer[i] == 0) {
                        continue;
                    }
                    coinsToStealByPlayer[i]--;
                    controllerPort--;
                    for (k = 0; k < totalCoinsStolen; k++) {
                        if (coinParticleActive[k] == 0) {
                            break;
                        }
                    }
                    if (k == totalCoinsStolen) {
                        OSReport("\x82\xB1\x82\xA2\x82\xF1\x82\xAA\x8F\x6F\x82\xB9\x82\xC8\x82\xA2 total %d\n", totalCoinsStolen);
                        while (TRUE) {
                            HuPrcVSleep();
                        }
                    }
                    HuAudFXPlay(0x12);
                    coinParticleActive[k] = 1;
                    BoardPlayerPosGet(i, &coinParticlePositions[k]);
                    coinParticlePositions[k].y += 180.0f;
                    BoardModelVisibilitySet(theftItemModelIds[k], 1);
                    BoardModelPosSetV(theftItemModelIds[k], &coinParticlePositions[k]);
                    coinParticleVelocities[k].x = 10.0 * sind(coinParticleLaunchAngles[launchAngleIndex] - 20.0f + 40.0f * j);
                    coinParticleVelocities[k].z = 10.0 * cosd(coinParticleLaunchAngles[launchAngleIndex] - 20.0f + 40.0f * j);
                    coinParticleVelocities[k].y = 6.0f + 6.0f * frand8() * 0.003921569f;
                    coinParticleAges[k] = 0.0f;
                    coinParticleAngles[k] = 360.0f * frand8() * 0.003921569f;
                }
                launchAngleIndex++;
            }
            computerButtonPressChance = 8;
        } else {
            computerButtonPressChance--;
        }
        pressedButtons = 0;
        for (i = 0; i < totalCoinsStolen; i++) {
            if (coinParticleActive[i] == 0) {
                continue;
            }
            pressedButtons++;
            coinParticlePositions[i].x += coinParticleVelocities[i].x;
            coinParticlePositions[i].z += coinParticleVelocities[i].z;
            coinParticlePositions[i].y += coinParticleVelocities[i].y - coinParticleAges[i] * coinParticleAges[i] * 0.25f * 0.075f;
            BoardModelPosSetV(theftItemModelIds[i], &coinParticlePositions[i]);
            BoardModelRotYSet(theftItemModelIds[i], coinParticleAngles[i]);
            coinParticleAges[i] += 1.0f;
            coinParticleAngles[i] += 12.0f;
            if (coinParticleAngles[i] >= 360.0f) {
                coinParticleAngles[i] -= 360.0f;
            }
            if (coinParticlePositions[i].y < -450.0f) {
                coinParticleActive[i] = 0;
                BoardModelVisibilitySet(theftItemModelIds[i], 0);
            }
        }
        if (pressedButtons == 0 && controllerPort == 0) {
            break;
        }
        HuPrcVSleep();
    }
    RestoreW04BigBooScene();
    sprintf(stolenCoinCountText, "%d", totalCoinsStolen);
    BoardWinCreate(2, MAKE_MESSID(32, 9), 9);
    BoardWinInsertMesSet(MAKE_MESSID_PTR(stolenCoinCountText), 0);
    BoardWinWait();
    BoardWinKill();
    BoardPlayerMotBlendSet(bigBooTargetPlayer, 0, 0xF);
    while (!BoardPlayerMotBlendCheck(bigBooTargetPlayer)) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(bigBooTargetPlayer);
    BoardPlayerPosGet(bigBooTargetPlayer, &bigBooPosition);
    for (i = 0; i < totalCoinsStolen; i++) {
        BoardModelVisibilitySet(theftItemModelIds[i], 1);
        coinParticlePositions[i].x = bigBooPosition.x - 25.0f + 50.0f * frand8() * 0.003921569f;
        coinParticlePositions[i].y = bigBooPosition.y + 1200.0f + 130.0f * i + 60.0f * frand8() * 0.003921569f;
        coinParticlePositions[i].z = bigBooPosition.z - 25.0f + 50.0f * frand8() * 0.003921569f;
        BoardModelPosSetV(theftItemModelIds[i], &coinParticlePositions[i]);
    }
    coinTheftAnimationPhase = -4.0f;
    computerButtonPressChance = 0;
    while (TRUE) {
        for (i = 0; i < totalCoinsStolen; i++) {
            if (i < computerButtonPressChance) {
                continue;
            }
            coinParticlePositions[i].y += coinTheftAnimationPhase;
            coinParticleAngles[i] += 45.0f;
            BoardModelPosSetV(theftItemModelIds[i], &coinParticlePositions[i]);
            BoardModelRotYSet(theftItemModelIds[i], coinParticleAngles[i]);
            if (coinParticlePositions[i].y <= bigBooPosition.y + 180.0f) {
                CharEffectCoinGlowCreate(1, &coinParticlePositions[i]);
                BoardModelVisibilitySet(theftItemModelIds[i], 0);
                computerButtonPressChance++;
                BoardPlayerCoinsAdd(bigBooTargetPlayer, 1);
                HuAudFXPlay(7);
                omVibrate(bigBooTargetPlayer, 12, 6, 6);
            }
        }
        coinTheftAnimationPhase *= 1.05f;
        if (coinTheftAnimationPhase < -40.0f) {
            coinTheftAnimationPhase = -40.0f;
        }
        if (computerButtonPressChance == totalCoinsStolen) {
            BoardPlayerMotionShiftSet(bigBooTargetPlayer, 7, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            break;
        }
        HuPrcVSleep();
    }
    while (!BoardPlayerMotionEndCheck(bigBooTargetPlayer)) {
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    for (i = 0; i < totalCoinsStolen; i++) {
        BoardModelKill(theftItemModelIds[i]);
    }
    BoardPlayerMotBlendSet(bigBooTargetPlayer, 0xB4, 0xF);
    while (!BoardPlayerMotBlendCheck(bigBooTargetPlayer)) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(bigBooTargetPlayer);
    BoardWinCreate(2, MAKE_MESSID(32, 10), 9);
    BoardWinWait();
    BoardWinKill();
    BoardAudSeqFadeOut(1, 1000);
    while (msmMusGetStatus(1) != 0) {
        HuPrcVSleep();
    }
    BoardAudSeqPause(0, 0, 1000);
}

static void StealW04BigBooStars(void) {
    Vec starParticlePositions[3];
    Vec starParticleVelocities[3];
    Vec bigBooPosition;
    Vec playerPosition;
    float starParticleAges[3];
    float starArcAngles[3];
    float starLaunchAngles[3] = { 315.0f, 0.0f, 45.0f };
    char stolenStarCountText[12];
    float starFallSpeed;
    float starScale;
    s16 starsToStealByPlayer[4];
    s16 particleIndex;
    s32 soundStreamHandle;
    s32 scaleStep;
    s32 playersWithStars;
    s32 i;

    PrepareW04BigBooTheftScene();
    particleManagerCount = 0;
    BoardModelMotionStart(bigBooModelId, 3, 0x40000001);
    BoardModelPosGet(bigBooModelId, &bigBooPosition);
    for (i = 0; i < 120; i++) {
        HuPrcVSleep();
    }
    for (i = 0; i < 4; i++) {
        if (i == bigBooTargetPlayer) {
            continue;
        }
        BoardPlayerMotionStart(i, bigBooPlayerMotionIds[i][3], 0);
        omVibrate(i, 12, 4, 2);
    }
    playersWithStars = 0;
    for (i = 0; i < 4; i++) {
        if (i == bigBooTargetPlayer) {
            continue;
        }
        if (GWStarsGet(i) > 0) {
            starsToStealByPlayer[i] = 1;
            playersWithStars++;
        } else {
            starsToStealByPlayer[i] = 0;
        }
        BoardPlayerStarsAdd(i, -starsToStealByPlayer[i]);
    }
    OSReport("Total Star %d\n", playersWithStars);
    for (i = 0; i < playersWithStars; i++) {
        theftItemModelIds[i] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 8), NULL, 0);
        BoardModelAttrSet(theftItemModelIds[i], 0x40000002);
        BoardModelVisibilitySet(theftItemModelIds[i], 0);
    }
    for (i = 0; i < 10; i++) {
        bigBooPosition.z += 21.0f - i;
        BoardModelPosSetV(bigBooModelId, &bigBooPosition);
        HuPrcVSleep();
    }
    for (i = 0; i < 4; i++) {
        if (i == bigBooTargetPlayer) {
            continue;
        }
        omVibrate(i, 12, 4, 2);
        PlayerFXPlay(i, 0x131);
    }
    HuAudFXPlay(0x4E);
    particleIndex = 0;
    for (i = 0; i < 4; i++) {
        if (i == bigBooTargetPlayer || starsToStealByPlayer[i] == 0) {
            continue;
        }
        BoardPlayerPosGet(i, &starParticlePositions[particleIndex]);
        starParticlePositions[particleIndex].y += 180.0f;
        starParticleAges[particleIndex] = 0.0f;
        starParticleVelocities[particleIndex].x = 10.0 * sind(starLaunchAngles[particleIndex]);
        starParticleVelocities[particleIndex].z = 10.0 * cosd(starLaunchAngles[particleIndex]);
        starParticleVelocities[particleIndex].y = 6.0f + 6.0f * frand8() * 0.003921569f;
        starParticleAges[particleIndex] = 360.0f * frand8() * 0.003921569f;
        starArcAngles[particleIndex] = 0.0f;
        BoardModelVisibilitySet(theftItemModelIds[particleIndex], 1);
        theftParticleManagerIds[particleIndex] = CreateW04BigBooParticleEffect();
        particleIndex++;
    }
    while (TRUE) {
        particleIndex = 0;
        for (i = 0; i < playersWithStars; i++) {
            starParticlePositions[i].x += starParticleVelocities[i].x;
            starParticlePositions[i].z += starParticleVelocities[i].z;
            starParticlePositions[i].y += starParticleVelocities[i].y - starArcAngles[i] * starArcAngles[i] * 0.25f * 0.075f;
            BoardModelPosSetV(theftItemModelIds[i], &starParticlePositions[i]);
            BoardModelRotYSet(theftItemModelIds[i], starParticleAges[i]);
            starArcAngles[i] += 1.0f;
            starParticleAges[i] += 12.0f;
            if (starParticleAges[i] >= 360.0f) {
                starParticleAges[i] -= 360.0f;
            }
            if (starParticlePositions[i].y < -500.0f) {
                BoardModelVisibilitySet(theftItemModelIds[i], 0);
                particleIndex++;
            }
            Hu3DParManPosSet(theftParticleManagerIds[i], starParticlePositions[i].x, starParticlePositions[i].y + -70.0f, starParticlePositions[i].z);
        }
        if (particleIndex == playersWithStars) {
            break;
        }
        HuPrcVSleep();
    }
    RestoreW04BigBooScene();
    #if VERSION_ENG
    sprintf(stolenStarCountText, "%d", playersWithStars);
    BoardWinCreate(2, MAKE_MESSID(32, 11), 9);
    BoardWinInsertMesSet(MAKE_MESSID_PTR(stolenStarCountText), 0);
    #else
    BoardWinCreate(2, MAKE_MESSID(32, 11), 9);
    #endif
    BoardWinWait();
    BoardWinKill();
    BoardAudSeqFadeOut(1, 1000);
    BoardPlayerMotBlendSet(bigBooTargetPlayer, 0, 0xF);
    while (!BoardPlayerMotBlendCheck(bigBooTargetPlayer)) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(bigBooTargetPlayer);
    BoardPlayerPosGet(bigBooTargetPlayer, &bigBooPosition);
    for (i = 0; i < playersWithStars; i++) {
        BoardModelVisibilitySet(theftItemModelIds[i], 1);
        starParticlePositions[i] = bigBooPosition;
        starParticlePositions[i].y += 600.0f;
        BoardModelPosSetV(theftItemModelIds[i], &starParticlePositions[i]);
        starParticleAges[i] = 0.0f;
        BoardModelRotYSet(theftItemModelIds[i], starParticleAges[i]);
    }
    for (i = 0; i < playersWithStars; i++) {
        starFallSpeed = -4.0f;
        HuAudFXPlay(0x338);
        HuAudFXPlay(0x353);
        while (TRUE) {
            starParticlePositions[i].y += starFallSpeed;
            BoardModelPosSetV(theftItemModelIds[i], &starParticlePositions[i]);
            Hu3DParManPosSet(theftParticleManagerIds[i], starParticlePositions[i].x, starParticlePositions[i].y + -70.0f, starParticlePositions[i].z);
            if (starParticlePositions[i].y <= bigBooPosition.y + 270.0f) {
                starParticlePositions[i].y = bigBooPosition.y + 270.0f;
                BoardModelPosSetV(theftItemModelIds[i], &starParticlePositions[i]);
                Hu3DParManPosSet(theftParticleManagerIds[i], starParticlePositions[i].x, starParticlePositions[i].y + -70.0f, starParticlePositions[i].z);
                BoardModelAttrReset(theftItemModelIds[i], 0x40000002);
                while (BoardModelMotionTimeGet(theftItemModelIds[i]) < BoardModelMotionMaxTimeGet(theftItemModelIds[i]) - 10.0f) {
                    HuPrcVSleep();
                }
                starFallSpeed = -5.0f;
                starScale = 1.0f;
                HuAudFXPlay(0x339);
                for (scaleStep = 0; scaleStep < 30; scaleStep++) {
                    starScale -= 0.033333335f;
                    playerPosition.x = playerPosition.y = playerPosition.z = starScale;
                    BoardModelScaleSetV(theftItemModelIds[i], &playerPosition);
                    starParticlePositions[i].y += starFallSpeed;
                    BoardModelPosSetV(theftItemModelIds[i], &starParticlePositions[i]);
                    starParticleAges[i] += 25.0f;
                    BoardModelRotYSet(theftItemModelIds[i], starParticleAges[i]);
                    Hu3DParManPosSet(theftParticleManagerIds[i], starParticlePositions[i].x, starParticlePositions[i].y + -70.0f, starParticlePositions[i].z);
                    HuPrcVSleep();
                }
                omVibrate(bigBooTargetPlayer, 12, 0xC, 0);
                BoardPlayerStarsAdd(bigBooTargetPlayer, 1);
                BoardModelVisibilitySet(theftItemModelIds[i], 0);
                KillW04BigBooParticleEffect(theftParticleManagerIds[i]);
                break;
            }
            starFallSpeed *= 1.05f;
            if (starFallSpeed < -20.0f) {
                starFallSpeed = -20.0f;
            }
            HuPrcVSleep();
        }
    }
    soundStreamHandle = HuAudSStreamPlay(6);
    BoardPlayerMotionShiftSet(bigBooTargetPlayer, 7, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    while (!BoardPlayerMotionEndCheck(bigBooTargetPlayer)) {
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    while (HuAudSStreamStatGet(soundStreamHandle) != 0) {
        HuPrcVSleep();
    }
    BoardAudSeqPause(0, 0, 1000);
    for (i = 0; i < playersWithStars; i++) {
        BoardModelKill(theftItemModelIds[i]);
    }
    BoardPlayerMotBlendSet(bigBooTargetPlayer, 0xB4, 0xF);
    while (!BoardPlayerMotBlendCheck(bigBooTargetPlayer)) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(bigBooTargetPlayer);
    BoardWinCreate(2, MAKE_MESSID(32, 10), 9);
    BoardWinWait();
    BoardWinKill();
}

static void FinishW04BigBooTheft(void) {
    Vec bigBooPosition;
    Vec hookPosition;
    float bigBooRotation;
    float modelAlpha;
    s32 i;

    BoardModelPosGet(bigBooModelId, &bigBooPosition);
    bigBooRotation = BoardModelRotYGet(bigBooModelId);
    BoardModelMotionShiftSet(bigBooModelId, 2, 0.0f, 25.0f, HU3D_MOTATTR_LOOP);
    for (i = 0; i < 25; i++) {
        bigBooRotation += 7.2f;
        BoardModelRotYSet(bigBooModelId, bigBooRotation);
        HuPrcVSleep();
    }
    BoardModelRotYSet(bigBooModelId, 180.0f);
    BoardModelMotionTimeSet(bigBooBaseModelId, 0.0f);
    BoardModelAttrReset(bigBooBaseModelId, 0x40000002);
    HuAudFXPlay(0x464);
    HuAudFXPlay(0x4E);
    Hu3DModelObjPosGet(BoardModelIDGet(w04BoardModelIds[4]), bigBooModelHookNames[4], &hookPosition);
    modelAlpha = 255.0f;
    while (TRUE) {
        bigBooPosition.z -= 10.0f;
        if (bigBooPosition.z <= hookPosition.z + 200.0f) {
            modelAlpha -= 20.0f;
            if (modelAlpha <= 0.0f) {
                break;
            }
            BoardModelAlphaSet(bigBooModelId, modelAlpha);
        }
        BoardModelPosSetV(bigBooModelId, &bigBooPosition);
        if (BoardModelMotionTimeGet(bigBooBaseModelId) >= 20.0f) {
            BoardModelAttrSet(bigBooBaseModelId, 0x40000002);
        }
        HuPrcVSleep();
    }
    BoardModelAlphaSet(bigBooModelId, 0);
    BoardModelMotionTimeSet(bigBooBaseModelId, 20.0f);
    BoardModelAttrSet(bigBooBaseModelId, 0x40000002);
    ResetW04BigBooModels();
}

static void ResetW04BigBooModels(void) {
    BoardModelKill(bigBooModelId);
    w04BoardEventState->booEncounterCount = 0;
    BoardModelMotionTimeSet(bigBooHeadModelId, 0.0f);
    BoardModelAttrSet(bigBooHeadModelId, 0x40000002);
    BoardModelMotionTimeSet(bigBooBodyModelId, 0.0f);
    BoardModelAttrSet(bigBooHeadModelId, 0x40000002);
}

static void PlayW04BigBooMusic(void) {
    HuPrcSleep(240);
    BoardMusStart(1, 0x1F, 0x7F, 0);
    HuPrcEnd();
}

static void UpdateW04BigBooModels(void) {
    Vec cameraTarget;
    Vec cameraRotation;
    s32 soundEffectId;

    cameraTarget.x = -150.0f;
    cameraTarget.y = 0.0f;
    cameraTarget.z = 120.0f;
    cameraRotation.x = -45.0f;
    cameraRotation.y = 0.0f;
    cameraRotation.z = 0.0f;
    BoardCameraMotionStartEx(w04BoardModelIds[4], &cameraRotation, &cameraTarget, 2300.0f, -1.0f, 30);
    BoardCameraMotionWait();
    if (w04BoardEventState->booEncounterCount == 1) {
        BoardModelMotionTimeSet(bigBooHeadModelId, 0.0f);
        BoardModelMotionSpeedSet(bigBooHeadModelId, 0.5f);
        BoardModelAttrReset(bigBooHeadModelId, 0x40000002);
    }
    HuAudFXPlay(0x461);
    soundEffectId = HuAudFXPlay(0x462);
    BoardModelAttrReset(bigBooBodyModelId, 0x40000002);
    HuPrcSleep(110);
    BoardModelAttrSet(bigBooBodyModelId, 0x40000002);
    HuAudFXStop(soundEffectId);
    switch (w04BoardEventState->booEncounterCount) {
        case 1:
        case 2:
            HuAudFXPlay(0x46A);
            break;
        case 3:
            HuAudFXPlay(0x46B);
            BoardAudSeqPause(0, 1, 1000);
            HuPrcChildCreate(PlayW04BigBooMusic, 0x2003, 0x1000, 0, boardMainProc);
            break;
    }
    HuPrcSleep(9);
    cameraTarget.x = -150.0f;
    cameraTarget.y = 250.0f;
    cameraTarget.z = 120.0f;
    cameraRotation.x = -5.0f;
    cameraRotation.y = 0.0f;
    cameraRotation.z = 0.0f;
    BoardCameraMotionStartEx(w04BoardModelIds[4], &cameraRotation, &cameraTarget, 1800.0f, -1.0f, 30);
    BoardCameraMotionWait();
    HuPrcSleep(30);
}

static void FinishW04BigBooEvent(void) {
    bigBooEventProcess = NULL;
}

void W04BigBooEvent(void) {
    bigBooTargetPlayer = GWSystem.player_curr;
    bigBooEventProcess = HuPrcCreate(RunW04BigBooEncounter, 0x2004, 0x2000, 0);
    HuPrcDestructorSet2(bigBooEventProcess, FinishW04BigBooEvent);
    while (bigBooEventProcess) {
        HuPrcVSleep();
    }
    BoardEventFlagReset();
}

static HU3DPARMANPARAM bigBooParticleParameters = {
    0x001E,
    { 0x00, 0x00 }, // padding?
     3.3f,
    70.0f,
     7.0f,
    { 0.0f, -0.05f, 0.0f },
    2.0f,
    1.0f,
    20.0f,
    0.98f,
    0x0002,
    {
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0x40, 0xFF },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00 }
    },
    {
        { 0xFF, 0x80, 0x80, 0x00 },
        { 0xFF, 0x40, 0x20, 0x00 },
        { 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00 }
    }
};

static s16 CreateW04BigBooParticleEffect(void) {
    void *userData;
    s16 particleManagerId;
    s16 particleModelId;

    if (bigBooAnimationData == NULL) {
        userData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 1), HU_MEMNUM_OVL, HEAP_MODEL);
        bigBooAnimationData = HuSprAnimRead(userData);
        HuSprAnimLock(bigBooAnimationData);
    }
    particleManagerId = Hu3DParManCreate(bigBooAnimationData, 0x64, &bigBooParticleParameters);
    Hu3DParManAttrSet(particleManagerId, 0x64);
    Hu3DParManRotSet(particleManagerId, 90.0f, 0.0f, 0.0f);
    particleModelId = Hu3DParManModelIDGet(particleManagerId);
    Hu3DParticleBlendModeSet(particleModelId, 1);
    Hu3DModelLayerSet(particleModelId, 2);
    particleManagerCount++;
    return particleManagerId;
}

static void KillW04BigBooParticleEffect(s16 particleManagerId) {
    if (particleManagerId == -1) {
        return;
    }
    Hu3DParManKill(particleManagerId);
    if (--particleManagerCount == 0) {
        HuSprAnimKill(bigBooAnimationData);
        bigBooAnimationData = NULL;
    }
}
