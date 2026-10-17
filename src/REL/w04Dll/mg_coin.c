#include "REL/w04Dll.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/msm.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/window.h"
#include "game/board/audio.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/window.h"

static void UpdateW04CoinGameAudio(OMOBJ *coinObject);
static void ExitW04CoinGame(void);
static void CleanupW04CoinGameObjects(void);
static void InitializeW04CoinObject(OMOBJ *coinObject);
static void UpdateW04CoinGameMusic(OMOBJ *coinObject);
static void AnimateW04CoinObject(OMOBJ *coinObject);
static void SpinW04CoinObject(OMOBJ *coinObject);
static void RestoreW04CoinObject(OMOBJ *coinObject);
static void StartW04CoinCollection(OMOBJ *coinObject);
static void RunW04CoinCollectionProcess(void);
static void RunW04CoinRewardSequence(void);
static void DestroyW04CoinGameObject(OMOBJ *coinObject);
static void UpdateW04CoinGameObject(OMOBJ *coinObject);
static void AnimateW04CoinReward(OMOBJ *coinObject);
static void UpdateW04CoinRewardDisplay(OMOBJ *coinObject);
static void FinishW04CoinRewardDisplay(OMOBJ *coinObject);
static s16 GetW04CoinRewardIndex(OMOBJ *coinObject);

static u16 coinGameExitRequested;
static s16 coinGamePlayer;
static OMOBJ *coinGameAudioObject;
static OMOBJ *coinGameSequenceObject;
static OMOBJ *coinGameObject;
static s16 coinRewardCount;
static Vec coinGamePlayerPosition;
static Vec coinDisplayPosition;
static s16 coinMusicFadingOut;
static s16 coinDisplayModelId;
static s16 coinDisplayModelIds[20];
static u16 coinSpaceFlags[7];
static u16 coinSpaceOrder[7];
static s32 coinSoundEffectId;

static s32 coinMusicSequenceId = -1;

static char *coinDisplayHookNames[] = {
    "pia1", "pia2", "pia3", "pia4",
    "pia5", "pia6", "pia7", "pia8",
    "pia9"
};

static u16 coinSpaceFlagValues[] = {
    0x0100, 0x0200, 0x0040, 0x0020,
    0x0800, 0x0400, 0x0010
};

static s32 coinRewardSoundIds[] = {
    0x0000046D,
    0x0000046E,
    0x0000046F,
    0x00000470,
    0x00000471,
    0x00000472,
    0x00000473,
    0x00000474
};

void W04CoinSetup(void) {
    BoardModelAttrSet(w04BoardModelIds[7], 0x40000002);
}

void W04CoinGameEvent(void) {
    Vec playerStartPosition;
    Vec coinHookPosition;
    Vec playerDestination;
    Vec coinModelPosition;
    Vec coinBoardPosition;
    float jumpHeight;
    float jumpFrame;
    float distanceToLanding;
    s16 randomSpaceIndex;
    s32 i;

    BoardModelLayerSet(w04BoardModelIds[9], 1);
    coinGamePlayer = GWSystem.player_curr;
    coinMusicSequenceId = -1;
    coinMusicFadingOut = 0;
    for (i = 0; i < 7; i++) {
        coinSpaceOrder[i] = 0;
    }
    for (i = 0; i < 7; i++) {
        do {
            randomSpaceIndex = frandmod(7);
        } while (coinSpaceOrder[randomSpaceIndex] != 0);
        coinSpaceOrder[randomSpaceIndex] = i + 1;
        coinSpaceFlags[randomSpaceIndex] = coinSpaceFlagValues[i];
    }
    for (i = 0; i < 7; i++) {
        OSReport("%d: %d  %04X\n", i, coinSpaceOrder[i], coinSpaceFlags[i]);
    }
    coinDisplayModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 16), NULL, 0);
    BoardModelVisibilitySet(coinDisplayModelId, 0);
    BoardModelAttrSet(coinDisplayModelId, 0x40000002);
    for (i = 0; i < 20; i++) {
        coinDisplayModelIds[i] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 10), NULL, 1);
        BoardModelVisibilitySet(coinDisplayModelIds[i], 0);
    }
    BoardPlayerMotBlendSet(coinGamePlayer, 0xB4, 0xF);
    while (!BoardPlayerMotBlendCheck(coinGamePlayer)) {
        HuPrcVSleep();
    }
    BoardWinCreate(2, MAKE_MESSID(32, 19), 3);
    BoardWinWait();
    BoardWinKill();
    BoardPlayerPosGet(coinGamePlayer, &coinGamePlayerPosition);
    BoardModelPosGet(w04BoardModelIds[9], &coinDisplayPosition);
    BoardPlayerPosGet(coinGamePlayer, &playerStartPosition);
    Hu3DModelObjPosGet(BoardModelIDGet(w04BoardModelIds[7]), coinDisplayHookNames[0], &coinHookPosition);
    playerDestination = coinHookPosition;
    playerDestination.z = playerStartPosition.z;
    playerDestination.y = playerStartPosition.y;
    BoardPlayerPosLerpStart(coinGamePlayer, &playerStartPosition, &playerDestination, 0x3C);
    while (W04ModelRotateYToAngle(w04BoardModelIds[9], 270.0f, 30.0f) == 0) {
        HuPrcVSleep();
    }
    BoardModelPosGet(w04BoardModelIds[9], &coinBoardPosition);
    coinModelPosition = coinBoardPosition;
    coinModelPosition.x = coinHookPosition.x;
    VECSubtract(&coinModelPosition, &coinBoardPosition, &playerStartPosition);
    VECScale(&playerStartPosition, &playerStartPosition, 0.022727273f);
    for (i = 0; i < 44; i++) {
        BoardModelPosGet(w04BoardModelIds[9], &coinBoardPosition);
        VECAdd(&coinBoardPosition, &playerStartPosition, &coinBoardPosition);
        BoardModelPosSetV(w04BoardModelIds[9], &coinBoardPosition);
        HuPrcVSleep();
    }
    BoardModelPosSetV(w04BoardModelIds[9], &coinModelPosition);
    while (W04ModelRotateYToAngle(w04BoardModelIds[9], 0.0f, 30.0f) == 0) {
        HuPrcVSleep();
    }
    while (GWPlayer[coinGamePlayer].moving) {
        HuPrcVSleep();
    }
    BoardPlayerMotBlendSet(coinGamePlayer, 0xB4, 0xF);
    while (!BoardPlayerMotBlendCheck(coinGamePlayer)) {
        HuPrcVSleep();
    }
    BoardPlayerMotionShiftSet(coinGamePlayer, 4, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    jumpFrame = 0.0f;
    jumpHeight = 8.0f;
    VECSubtract(&coinHookPosition, &playerDestination, &playerStartPosition);
    VECScale(&playerStartPosition, &playerStartPosition, 0.05f);
    while (TRUE) {
        BoardPlayerPosGet(coinGamePlayer, &playerDestination);
        playerStartPosition.y = jumpHeight - jumpFrame * jumpFrame * 0.25f * 0.075f;
        jumpFrame += 1.0f;
        VECAdd(&playerStartPosition, &playerDestination, &playerDestination);
        coinModelPosition = coinHookPosition;
        coinModelPosition.y = playerDestination.y;
        VECSubtract(&coinModelPosition, &playerDestination, &coinModelPosition);
        distanceToLanding = VECMag(&coinModelPosition);
        if (distanceToLanding <= 1.0f) {
            playerStartPosition.x = playerStartPosition.z = 0.0f;
            playerDestination.x = coinHookPosition.x;
            playerDestination.z = coinHookPosition.z;
        }
        if (playerDestination.y <= coinHookPosition.y && playerStartPosition.y < 0.0f) {
            playerDestination.y = coinHookPosition.y;
            break;
        }
        BoardPlayerPosSetV(coinGamePlayer, &playerDestination);
        HuPrcVSleep();
    }
    BoardPlayerPosSetV(coinGamePlayer, &playerDestination);
    BoardPlayerMotBlendSet(coinGamePlayer, 0x5A, 0xF);
    while (!BoardPlayerMotBlendCheck(coinGamePlayer)) {
        HuPrcVSleep();
    }
    HuPrcSleep(60);
    coinGameAudioObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, UpdateW04CoinGameAudio);
    coinGameObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, InitializeW04CoinObject);
    coinGameSequenceObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, NULL);
    coinGameObject->scale.x = 255.0f;
    coinRewardCount = 0;
    coinGameExitRequested = 0;
}

static void UpdateW04CoinGameAudio(OMOBJ *coinObject) {
    s32 sequenceVolume;

    if (coinMusicSequenceId != -1) {
        sequenceVolume = HuAudSeqMidiCtrlGet(coinMusicSequenceId, 0, 1);
        if (msmMusGetStatus(coinMusicSequenceId) == 0) {
            HuAudSeqFadeOut(coinMusicSequenceId, 100);
            coinMusicFadingOut = 1;
        }
        if (coinMusicFadingOut == 1 && msmMusGetStatus(coinMusicSequenceId) == 0) {
            coinMusicSequenceId = -1;
            BoardAudSeqPause(0, 0, 1000);
            OSReport("BGM1 Restore\n");
        }
    }
    if (coinGameExitRequested != 0 && BoardMGDoneFlagGet() == 1) {
        CleanupW04CoinGameObjects();
        omDelObjEx(boardObjMan, coinObject);
    }
}

static void ExitW04CoinGame(void) {
    coinGameExitRequested = 1;
    BoardMGExit();
}

static void CleanupW04CoinGameObjects(void) {
    s32 i;

    omDelObjEx(boardObjMan, coinGameSequenceObject);
    BoardModelKill(coinDisplayModelId);
    for (i = 0; i < 20; i++) {
        BoardModelKill(coinDisplayModelIds[i]);
    }
    BoardModelLayerSet(w04BoardModelIds[9], 0);
    BoardMGDoneFlagSet(0);
}

static void InitializeW04CoinObject(OMOBJ *coinObject) {
    MSM_MUSPARAM musParam;

    if (coinObject->work[0] == 0) {
        coinObject->work[0] = MGSeqCreate(3, 0);
        BoardAudSeqPause(0, 1, 1000);
        return;
    }
    if (MGSeqStatGet(coinObject->work[0]) == 0) {
        musParam.flag = MSM_MUSPARAM_CHAN;
        musParam.fadeSpeed = 0;
        musParam.vol = 0x7F;
        musParam.chan = 1;
        coinMusicSequenceId = msmMusPlay(0x1D, &musParam);
        coinObject->objFunc = UpdateW04CoinGameMusic;
    }
}

static void UpdateW04CoinGameMusic(OMOBJ *coinObject) {
    s32 midiControlValue;

    midiControlValue = HuAudSeqMidiCtrlGet(coinMusicSequenceId, 0, 1);
    if (midiControlValue >= 10 && midiControlValue <= 19) {
        coinObject->work[1]++;
        coinObject->objFunc = AnimateW04CoinObject;
    }
}

static void AnimateW04CoinObject(OMOBJ *coinObject) {
    Vec coinHookPosition;
    Vec coinModelPosition;
    float boardRotation;
    HU3DMODEL *coinModelData;
    s32 midiControlValue;

    coinModelData = &Hu3DData[BoardModelIDGet(coinDisplayModelId)];
    boardRotation = BoardModelRotYGet(w04BoardModelIds[9]);
    BoardModelPosGet(w04BoardModelIds[9], &coinModelPosition);
    if (coinObject->scale.x != 0.0f) {
        coinObject->scale.x -= 510.0f/REFRESH_RATE;
        if (coinObject->scale.x < 0.0f) {
            coinObject->scale.x = 0.0f;
        }
        BoardModelAlphaSet(w04BoardModelIds[9], coinObject->scale.x);
        return;
    }
    midiControlValue = HuAudSeqMidiCtrlGet(coinMusicSequenceId, 0, 1);
    if (midiControlValue >= 20 && midiControlValue <= 29) {
        Hu3DModelObjPosGet(BoardModelIDGet(w04BoardModelIds[7]), coinDisplayHookNames[coinObject->work[1]], &coinHookPosition);
        coinModelPosition.x = coinHookPosition.x;
        BoardModelPosSetV(w04BoardModelIds[9], &coinModelPosition);
        coinObject->objFunc = SpinW04CoinObject;
        if (coinObject->work[1] < 8) {
            BoardModelHookReset(w04BoardModelIds[7]);
            BoardModelVisibilitySet(coinDisplayModelId, 1);
            BoardModelMotionTimeSet(coinDisplayModelId, coinSpaceOrder[coinObject->work[1] - 1]);
            BoardModelHookSet(w04BoardModelIds[7], coinDisplayHookNames[coinObject->work[1]], coinDisplayModelId);
            Hu3DMotionExec(BoardModelIDGet(coinDisplayModelId), coinModelData->motId, coinModelData->motWork.time, 0);
        }
        coinGameSequenceObject->work[0] = 60;
        coinGameSequenceObject->mode = frandmod((REFRESH_RATE*2)/3) + (REFRESH_RATE/6);
        coinGameSequenceObject->work[1] = coinObject->work[1];
        coinGameSequenceObject->objFunc = UpdateW04CoinGameObject;
    }
}

static void SpinW04CoinObject(OMOBJ *coinObject) {
    float boardRotation;

    boardRotation = BoardModelRotYGet(w04BoardModelIds[9]);
    coinObject->scale.x += 510.0f/REFRESH_RATE;
    if (coinObject->scale.x > 255.0f) {
        coinObject->scale.x = 255.0f;
    }
    BoardModelAlphaSet(w04BoardModelIds[9], coinObject->scale.x);
    if (coinObject->scale.x == 255.0f) {
        coinObject->work[0] = 0;
        coinGameSequenceObject->work[1] = coinObject->work[1];
        coinObject->scale.x = 255.0f;
        BoardModelAlphaSet(w04BoardModelIds[9], coinObject->scale.x);
        if (coinObject->work[1] == 8) {
            coinObject->objFunc = StartW04CoinCollection;
            coinGameSequenceObject->work[2] = 1;
            coinGameSequenceObject->objFunc = AnimateW04CoinReward;
        } else {
            coinObject->objFunc = RestoreW04CoinObject;
        }
    }
}

static void RestoreW04CoinObject(OMOBJ *coinObject) {
    if (coinObject->work[0] != 0) {
        BoardModelHookReset(w04BoardModelIds[7]);
        BoardModelVisibilitySet(coinDisplayModelId, 0);
        coinObject->objFunc = UpdateW04CoinGameMusic;
    }
}

static void StartW04CoinCollection(OMOBJ *coinObject) {
    HUPROCESS *process;

    if (coinObject->work[0] == 1) {
        process = HuPrcCreate(RunW04CoinCollectionProcess, 0x2004, 0x1800, 0);
        process->user_data = coinObject;
        coinObject->objFunc = NULL;
    }
}

s16 coinRewardCountByChoice[] = {
     1, 3, 5, 7, 10, 14, 18, 20
};

static void RunW04CoinCollectionProcess(void) {
    Vec coinParticlePositions[20];
    Vec coinParticleRotations[20];
    Vec playerPosition;
    Vec coinParticleVelocity;
    float coinParticleAge;
    float coinParticleSpeed;
    float coinParticleScale;
    char coinCountText[4];
    HUPROCESS *process;
    OMOBJ *object;
    s32 coinParticleCount;
    s32 i;

    coinRewardCount = coinRewardCountByChoice[coinRewardCount - 1];
    sprintf(coinCountText, "%d", coinRewardCount);
    BoardWinCreate(2, MAKE_MESSID(32, 20), 3);
    BoardWinInsertMesSet(MAKE_MESSID_PTR(coinCountText), 0);
    BoardWinWait();
    BoardWinKill();
    BoardPlayerMotBlendSet(coinGamePlayer, 0, 0xF);
    while (!BoardPlayerMotBlendCheck(coinGamePlayer)) {
        HuPrcVSleep();
    }
    BoardPlayerPosGet(coinGamePlayer, &playerPosition);
    for (i = 0; i < coinRewardCount; i++) {
        BoardModelVisibilitySet(coinDisplayModelIds[i], 1);
        coinParticlePositions[i] = playerPosition;
        coinParticlePositions[i].y += 800.0f + i * 120;
        coinParticlePositions[i].x += -25.0f + 50.0f * frand8() * 0.003921569f;
        coinParticlePositions[i].z += -25.0f + 50.0f * frand8() * 0.003921569f;
        coinParticleRotations[i].x = coinParticleRotations[i].z = 0.0f;
        coinParticleRotations[i].y = 360.0f * frand8() * 0.003921569f;
        BoardModelPosSetV(coinDisplayModelIds[i], &coinParticlePositions[i]);
        BoardModelRotSetV(coinDisplayModelIds[i], &coinParticleRotations[i]);
    }
    coinParticleScale = -1.0f;
    coinParticleCount = 0;
    while (TRUE) {
        for (i = 0; i < coinRewardCount; i++) {
            if (i < coinParticleCount) {
                continue;
            }
            coinParticlePositions[i].y += coinParticleScale;
            BoardModelPosSetV(coinDisplayModelIds[i], &coinParticlePositions[i]);
            coinParticleRotations[i].y += 45.0f;
            if (coinParticleRotations[i].y >= 360.0f) {
                coinParticleRotations[i].y -= 360.0f;
            }
            BoardModelRotSetV(coinDisplayModelIds[i], &coinParticleRotations[i]);
            if (coinParticlePositions[i].y <= playerPosition.y + 80.0f) {
                coinParticleVelocity = coinParticlePositions[i];
                CharEffectCoinGlowCreate(1, &coinParticleVelocity);
                BoardModelVisibilitySet(coinDisplayModelIds[i], 0);
                coinParticleCount += 1;
                BoardPlayerCoinsAdd(coinGamePlayer, 1);
                HuAudFXPlay(7);
                omVibrate(coinGamePlayer, 12, 6, 6);
            }
        }
        coinParticleScale *= 1.05f;
        if (coinParticleScale < -20.0f) {
            coinParticleScale = -20.0f;
        }
        if (coinParticleCount == coinRewardCount) {
            BoardPlayerMotionShiftSet(coinGamePlayer, 7, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            break;
        }
        HuPrcVSleep();
    }
    while (!BoardPlayerMotionEndCheck(coinGamePlayer)) {
        HuPrcVSleep();
    }
    BoardWinCreate(2, MAKE_MESSID(32, 21), 3);
    BoardWinWait();
    BoardWinKill();
    process = HuPrcCreate(RunW04CoinRewardSequence, 0x2004, 0x1000, 0);
    process->user_data = NULL;
    BoardPlayerMotionShiftSet(coinGamePlayer, 4, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    coinParticleSpeed = 0.0f;
    coinParticleAge = 8.0f;
    coinGamePlayerPosition.z = playerPosition.z + 100.0f;
    coinGamePlayerPosition.x = playerPosition.x + 50.0f;
    VECSubtract(&coinGamePlayerPosition, &playerPosition, &coinParticleVelocity);
    VECScale(&coinParticleVelocity, &coinParticleVelocity, 0.04f);
    while (TRUE) {
        BoardPlayerPosGet(coinGamePlayer, &playerPosition);
        coinParticleVelocity.y = coinParticleAge - coinParticleSpeed * coinParticleSpeed * 0.25f * 0.075f;
        coinParticleSpeed += 1.0f;
        VECAdd(&coinParticleVelocity, &playerPosition, &playerPosition);
        coinParticlePositions[0] = coinGamePlayerPosition;
        coinParticlePositions[0].y = playerPosition.y;
        VECSubtract(&coinParticlePositions[0], &playerPosition, &coinParticlePositions[0]);
        coinParticleScale = VECMag(&coinParticlePositions[0]);
        if (coinParticleScale <= 1.0f) {
            coinParticleVelocity.x = coinParticleVelocity.z = 0.0f;
            playerPosition.x = coinGamePlayerPosition.x;
            playerPosition.z = coinGamePlayerPosition.z;
        }
        if (playerPosition.y <= coinGamePlayerPosition.y && coinParticleVelocity.y < 0.0f) {
            playerPosition.y = coinGamePlayerPosition.y;
            break;
        }
        BoardPlayerPosSetV(coinGamePlayer, &playerPosition);
        HuPrcVSleep();
    }
    BoardPlayerPosSetV(coinGamePlayer, &playerPosition);
    omVibrate(coinGamePlayer, 12, 6, 6);
    while (!process->user_data) {
        HuPrcVSleep();
    }
    HuPrcKill(process);
    object = HuPrcCurrentGet()->user_data;
    object->objFunc = DestroyW04CoinGameObject;
    HuPrcEnd();
}

static void RunW04CoinRewardSequence(void) {
    Vec coinPosition;
    Vec movementStep;
    s32 i;

    while (W04ModelRotateYToAngle(w04BoardModelIds[9], 90.0f, 30.0f) == 0) {
        HuPrcVSleep();
    }
    BoardModelPosGet(w04BoardModelIds[9], &coinPosition);
    VECSubtract(&coinDisplayPosition, &coinPosition, &movementStep);
    VECScale(&movementStep, &movementStep, 0.1f);
    for (i = 0; i < 10; i++) {
        BoardModelPosGet(w04BoardModelIds[9], &coinPosition);
        VECAdd(&coinPosition, &movementStep, &coinPosition);
        BoardModelPosSetV(w04BoardModelIds[9], &coinPosition);
        HuPrcVSleep();
    }
    BoardModelPosSetV(w04BoardModelIds[9], &coinDisplayPosition);
    while (W04ModelRotateYToAngle(w04BoardModelIds[9], 0.0f, 30.0f) == 0) {
        HuPrcVSleep();
    }
    HuPrcCurrentGet()->user_data = (void*) 1;
    while (TRUE) {
        HuPrcVSleep();
    }
}

static void DestroyW04CoinGameObject(OMOBJ *coinObject) {
    coinGameExitRequested = 1;
    BoardMGExit();
    omDelObjEx(boardObjMan, coinObject);
}

static void UpdateW04CoinGameObject(OMOBJ *coinObject) {
    s32 midiControlValue;
    s16 controllerPort;
    s16 pressedButtons;

    midiControlValue = HuAudSeqMidiCtrlGet(coinMusicSequenceId, 0, 1);
    if (midiControlValue >= 20 && midiControlValue <= 29) {
        controllerPort = GWPlayer[coinGamePlayer].port;
        pressedButtons = 0;
        if (GWPlayer[coinGamePlayer].com) {
            if (--coinObject->mode == 0) {
                pressedButtons = GetW04CoinRewardIndex(coinObject);
            }
        } else {
            pressedButtons = HuPadBtnDown[controllerPort];
        }
        if (pressedButtons & 0xF70) {
            if ((pressedButtons & coinSpaceFlags[coinObject->work[1] - 1]) && !(pressedButtons & ~coinSpaceFlags[coinObject->work[1] - 1])) {
                coinObject->work[2] = 1;
            } else {
                coinObject->work[2] = 0;
            }
            BoardModelVisibilitySet(coinDisplayModelId, 0);
            coinObject->objFunc = AnimateW04CoinReward;
        }
    } else {
        BoardModelVisibilitySet(coinDisplayModelId, 0);
        coinObject->work[2] = 0;
        coinObject->objFunc = AnimateW04CoinReward;
        coinGameObject->work[0] = 1;
        AnimateW04CoinReward(coinObject);
    }
}

static void AnimateW04CoinReward(OMOBJ *coinObject) {
    Vec playerPosition;
    Vec coinHookPosition;
    Vec coinVelocity;
    s32 midiControlValue;

    midiControlValue = HuAudSeqMidiCtrlGet(coinMusicSequenceId, 0, 1);
    if (midiControlValue >= 30 && midiControlValue <= 39) {
        Hu3DModelObjPosGet(BoardModelIDGet(w04BoardModelIds[7]), coinDisplayHookNames[coinObject->work[1]], &coinHookPosition);
        BoardPlayerPosGet(coinGamePlayer, &playerPosition);
        VECSubtract(&coinHookPosition, &playerPosition, &coinVelocity);
        VECScale(&coinVelocity, &coinVelocity, 0.04f);
        coinObject->trans.x = coinVelocity.x;
        coinObject->trans.z = coinVelocity.z;
        coinObject->trans.y = 5.0f;
        coinObject->rot.x = 0.0f;
        coinObject->scale.x = coinHookPosition.x;
        coinObject->scale.y = coinHookPosition.y;
        coinObject->scale.z = coinHookPosition.z;
        BoardPlayerMotionShiftSet(coinGamePlayer, 4, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
        coinObject->objFunc = UpdateW04CoinRewardDisplay;
    }
}

static void UpdateW04CoinRewardDisplay(OMOBJ *coinObject) {
    Vec playerPosition;
    Vec coinOffset;
    Vec coinDestination;
    Vec updatedPlayerPosition;
    float distanceToCoin;

    BoardPlayerPosGet(coinGamePlayer, &playerPosition);
    coinDestination.x = coinObject->scale.x;
    coinDestination.y = coinObject->scale.y;
    coinDestination.z = coinObject->scale.z;
    coinOffset.x = coinObject->trans.x;
    coinOffset.z = coinObject->trans.z;
    coinOffset.y = coinObject->trans.y - coinObject->rot.x * coinObject->rot.x * 0.25f * ((VERSION_PAL) ? 0.1125f : 0.075f);
    coinObject->rot.x += 1.0f;
    VECAdd(&playerPosition, &coinOffset, &playerPosition);
    updatedPlayerPosition = playerPosition;
    updatedPlayerPosition.y = coinDestination.y;
    VECSubtract(&coinDestination, &updatedPlayerPosition, &updatedPlayerPosition);
    distanceToCoin = VECMag(&updatedPlayerPosition);
    if (distanceToCoin <= 1.0f) {
        coinObject->trans.x = coinObject->trans.z = 0.0f;
        playerPosition.x = coinDestination.x;
        playerPosition.z = coinDestination.z;
    }
    if (playerPosition.y <= coinDestination.y && coinOffset.y < 0.0f) {
        playerPosition.y = coinDestination.y;
        BoardPlayerIdleSet(coinGamePlayer);
        coinObject->work[3] = 0;
        if (coinObject->work[2] != 0) {
            coinObject->objFunc = FinishW04CoinRewardDisplay;
        } else {
            coinGameObject->work[0] = 1;
            HuAudFXPlay(0x475);
            coinObject->objFunc = NULL;
        }
    }
    BoardPlayerPosSetV(coinGamePlayer, &playerPosition);
}

static void FinishW04CoinRewardDisplay(OMOBJ *coinObject) {
    Vec coinHookPosition;
    float motionTime;

    if (coinObject->work[3] == 0) {
        coinSoundEffectId = -1;
        motionTime = 4.0f + 20.0f * (coinObject->work[1] - 1);
        BoardModelMotionTimeSet(w04BoardModelIds[7], motionTime);
        BoardModelAttrReset(w04BoardModelIds[7], 0x40000002);
        coinRewardCount++;
        coinObject->work[3] = 1;
        coinObject->work[0] = 0x10;
        coinSoundEffectId = HuAudFXPlay(coinRewardSoundIds[coinObject->work[1] - 1]);
    }
    if (--coinObject->work[0] != 0) {
        Hu3DModelObjPosGet(BoardModelIDGet(w04BoardModelIds[7]), coinDisplayHookNames[coinObject->work[1]], &coinHookPosition);
        BoardPlayerPosSetV(coinGamePlayer, &coinHookPosition);
        return;
    }
    if (coinSoundEffectId >= 0) {
        HuAudFXStop(coinSoundEffectId);
    }
    coinGameObject->work[0] = 1;
    BoardModelAttrSet(w04BoardModelIds[7], 0x40000002);
    coinObject->objFunc = NULL;
}

static s16 GetW04CoinRewardIndex(OMOBJ *coinObject) {
    s16 playerDifficulty;
    s16 rewardIndex;
    s32 randomRoll;

    playerDifficulty = GWPlayer[coinGamePlayer].diff;
    rewardIndex = 0;
    randomRoll = frandmod(100);
    switch (playerDifficulty) {
        case 0:
            if (randomRoll < 35) {
                rewardIndex = coinSpaceFlags[coinObject->work[1] - 1];
            } else if (randomRoll < 90) {
                rewardIndex = -1;
            } else {
                coinObject->mode = 100;
            }
            break;
        case 1:
            if (randomRoll < 55) {
                rewardIndex = coinSpaceFlags[coinObject->work[1] - 1];
            } else if (randomRoll < 90) {
                rewardIndex = -1;
            } else {
                coinObject->mode = 100;
            }
            break;
        case 2:
            if (randomRoll < 85) {
                rewardIndex = coinSpaceFlags[coinObject->work[1] - 1];
            } else {
                rewardIndex = -1;
            }
            break;
        case 3:
            rewardIndex = coinSpaceFlags[coinObject->work[1] - 1];
            break;
    }
    return rewardIndex;
}
