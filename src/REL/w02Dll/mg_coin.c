#include "REL/w02Dll.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/frand.h"
#ifndef __MWERKS__
#include "game/msm.h"
#endif
#include "game/objsub.h"
#include "game/pad.h"

#include "ext_math.h"

#ifndef __MWERKS__
#include "game/board/audio.h"
#endif
#include "game/minigame_seq.h"

s16 coinModelIds[3];
s16 coinShadowModelIds[3];
float coinInitialAngles[3];
OMOBJ *coinUpdateObject;
OMOBJ *coinPlayerObject;
OMOBJ *coinTimerObject;
u16 coinTimerSequenceId;
Vec coinCenterPosition;
Vec coinPlayerTargetPosition;
s16 coinPlayerIndex;
s16 coinPlayerMotionId;
s16 coinModelPool[20];
s32 coinGameState;

Vec coinPlayerStartPositions[3] = { { -45, 155, 108 }, { 0, 155, 108 }, { 45, 155, 108 } };

Vec coinPlayerTargetOffsets[3] = { { -45, 25, 200 }, { 0, 25, 200 }, { 45, 25, 200 } };

s32 coinRewardAmounts[9] = { 5, 10, 20, 5, 10, 20, 5, 10, 20 };

s32 coinPlayerIdleMotions[8] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x08), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x08), DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x08),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x08), DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x08), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x08),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x08), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x08) };

s32 coinPlayerActionMotions[8] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x0A), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x0A), DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x0A),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x0A), DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x0A), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x0A),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x0A), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x0A) };

void CoinMinigamePositionModels(Vec *coinCenterPosition);

void CoinMinigameModelsInitialize(void)
{
    Vec modelRotation;
    s32 coinIndex;
    BoardModelVisibilitySet(boardModelIds[7], 1);
    BoardModelVisibilitySet(boardModelIds[8], 1);
    BoardModelVisibilitySet(boardModelIds[9], 0);
    BoardModelAttrSet(boardModelIds[8], 0x40000002);
    BoardModelAttrSet(boardModelIds[9], 0x40000003);
    BoardModelAttrSet(boardModelIds[7], 0x40000001);
    BoardModelPosGet(boardModelIds[8], &coinCenterPosition);
    for (coinIndex = 0; coinIndex < 3; coinIndex++) {
        coinModelIds[coinIndex] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W02, 0x13), NULL, 1);
        coinShadowModelIds[coinIndex] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W02, 0x14), NULL, 1);
        BoardModelAttrSet(coinShadowModelIds[coinIndex], 0x40000002);
        coinInitialAngles[coinIndex] = (s32)frandmod(9) * 40.0f;
        modelRotation.x = coinInitialAngles[coinIndex];
        modelRotation.y = modelRotation.z = 0;
        BoardModelRotSetV(coinModelIds[coinIndex], &modelRotation);
    }
    BoardModelPosSetV(boardModelIds[7], &coinCenterPosition);
    CoinMinigamePositionModels(&coinCenterPosition);
}

void CoinMinigameUpdatePlayerCoins(OMOBJ *object);
void CoinMinigameUpdateTimer(OMOBJ *object);

typedef struct CoinMinigamePlayerWork {
    float coinAngles[3];
    float coinSpinSpeeds[3];
    float coinMotionFrames[3];
    s16 coinModelIds[3];
} WorkMGCoinPlayer;

typedef struct CoinMinigameWork {
    float verticalLaunchSpeed;
    float bounceFrame;
    s16 playerIndex;
    Vec velocity;
    s16 countdownValue;
    s16 countdownFrame;
    s16 timerSequenceId;
    s16 messageWindowId;
    s16 inputDelay;
} WorkMGCoin;

void CoinMinigameStart(void)
{
    Vec playerPosition;

    s32 modelIndex;
    WorkMGCoinPlayer *coinWork;
    coinPlayerIndex = GWSystem.player_curr;
    coinUpdateObject = omAddObjEx(boardObjMan, 257, 0, 0, -1, CoinMinigameUpdatePlayerCoins);
    coinTimerObject = omAddObjEx(boardObjMan, 257, 0, 0, -1, CoinMinigameUpdateTimer);
    coinPlayerObject = omAddObjEx(boardObjMan, 257, 0, 2, -1, NULL);
    coinPlayerObject->mtnId[0] = BoardPlayerMotionCreate(coinPlayerIndex, coinPlayerIdleMotions[GWPlayer[coinPlayerIndex].character]);
    coinPlayerObject->mtnId[1] = BoardPlayerMotionCreate(coinPlayerIndex, coinPlayerActionMotions[GWPlayer[coinPlayerIndex].character]);
    coinTimerObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(WorkMGCoinPlayer), HU_MEMNUM_OVL);
    coinWork = coinTimerObject->data;
    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
        coinWork->coinAngles[modelIndex] = coinInitialAngles[modelIndex];
        coinWork->coinSpinSpeeds[modelIndex] = 0;
        coinWork->coinMotionFrames[modelIndex] = 0;
        coinWork->coinModelIds[modelIndex] = 0;
    }
    coinPlayerObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(WorkMGCoin), HU_MEMNUM_OVL);
    BoardPlayerPosGet(coinPlayerIndex, &playerPosition);
    coinPlayerObject->trans.x = playerPosition.x;
    coinPlayerObject->trans.y = playerPosition.y;
    coinPlayerObject->trans.z = playerPosition.z;
    for (modelIndex = 0; modelIndex < 20; modelIndex++) {
        coinModelPool[modelIndex] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x0A), NULL, 1);
        BoardModelVisibilitySet(coinModelPool[modelIndex], 0);
    }
    coinTimerSequenceId = 0;
}

void CoinMinigameDestroyPlayerCoins(void);

void CoinMinigameUpdatePlayerCoins(OMOBJ *object)
{
    if (coinTimerSequenceId && BoardMGDoneFlagGet() == 1) {
        CoinMinigameDestroyPlayerCoins();
        omDelObjEx(boardObjMan, object);
    }
}

void CoinMinigamePlayerCoinsCleanup(void)
{
    coinTimerSequenceId = 1;
    BoardMGExit();
}

void CoinMinigameDestroyPlayerCoins(void)
{
    s32 coinIndex;
    BoardPlayerMotionKill(coinPlayerIndex, coinPlayerObject->mtnId[0]);
    BoardPlayerMotionKill(coinPlayerIndex, coinPlayerObject->mtnId[1]);
    omDelObjEx(boardObjMan, coinPlayerObject);
    for (coinIndex = 0; coinIndex < 20; coinIndex++) {
        BoardModelKill(coinModelPool[coinIndex]);
    }
    BoardMGDoneFlagSet(0);
}

void CoinMinigameTimerCleanup(OMOBJ *object);

void CoinMinigameUpdateTimer(OMOBJ *object)
{
    if (object->work[0] == 0) {
        object->work[0] = MGSeqStartCreate();
        return;
    }
    if (MGSeqStatGet(object->work[0])) {
        return;
    }
    object->objFunc = CoinMinigameTimerCleanup;
    BoardModelMotionTimeSet(boardModelIds[8], 0);
    BoardModelAttrReset(boardModelIds[8], 0x40000006);
    HuAudFXPlay(1062);
}

void CoinMinigameTimerDestroy(OMOBJ *object);

void CoinMinigameTimerCleanup(OMOBJ *object)
{
    if (BoardModelMotionTimeGet(boardModelIds[8]) >= BoardModelMotionMaxTimeGet(boardModelIds[8])) {
        object->work[0] = 0;
        object->work[1] = 0;
        object->work[2] = 1;
        object->work[3] = 0;
        object->objFunc = CoinMinigameTimerDestroy;
        BoardModelAttrSet(boardModelIds[8], 0x40000004);
        coinGameState = HuAudFXPlay(1063);
    }
}

void CoinMinigamePlayerCoinsAnimate(OMOBJ *object);
void CoinMinigamePlayerMotionUpdate(OMOBJ *object);

void CoinMinigameTimerDestroy(OMOBJ *object)
{
    Vec coinRotation;
    WorkMGCoinPlayer *coinWork;
    s32 coinIndex;
    s32 spunCoinCount;
    u32 coinMask;
    coinWork = object->data;
    if (object->work[1] == 0) {
        object->work[0] |= object->work[2];
        object->work[1] = 12;
        object->work[2] <<= 1;
    }
    else {
        object->work[1]--;
    }
    coinRotation.y = coinRotation.z = 0;
    spunCoinCount = 0;
    for (coinIndex = 0; coinIndex < 3; coinIndex++) {
        coinMask = 1 << coinIndex;
        if (object->work[0] & coinMask) {
            coinWork->coinSpinSpeeds[coinIndex] += 0.25f;
            if (coinWork->coinSpinSpeeds[coinIndex] > 5.0f) {
                coinWork->coinSpinSpeeds[coinIndex] = 5.0f;
                spunCoinCount++;
            }
            coinWork->coinAngles[coinIndex] += coinWork->coinSpinSpeeds[coinIndex];
            if (coinWork->coinAngles[coinIndex] >= 360.0f) {
                coinWork->coinAngles[coinIndex] -= 360.0f;
            }
            coinRotation.x = coinWork->coinAngles[coinIndex];
            BoardModelRotSetV(coinModelIds[coinIndex], &coinRotation);
        }
    }
    if (spunCoinCount == 3) {
        object->objFunc = CoinMinigamePlayerCoinsAnimate;
        coinPlayerObject->objFunc = CoinMinigamePlayerMotionUpdate;
    }
}

void CoinMinigameCoinsFinish(OMOBJ *object);

void CoinMinigameTimerStart(void);

void CoinMinigamePlayerCoinsAnimate(OMOBJ *object)
{
    Vec coinRotation;
    float angleRemainder;
    WorkMGCoinPlayer *coinWork;
    s32 coinIndex;
    s32 coinMask;
    HUPROCESS *audioProcess;
    coinWork = object->data;
    coinRotation.y = coinRotation.z = 0;
    for (coinIndex = 0; coinIndex < 3; coinIndex++) {
        coinMask = 1 << coinIndex;
        if (object->work[0] & coinMask) {
            if (object->work[3] & coinMask) {
                coinWork->coinMotionFrames[coinIndex] = 360;
                coinWork->coinModelIds[coinIndex] = 1;
                object->work[3] &= ~coinMask;
            }
            if (coinWork->coinModelIds[coinIndex]) {
                if (0.0f == coinWork->coinMotionFrames[coinIndex]) {
                    angleRemainder = fmod(coinWork->coinAngles[coinIndex], 40.0);
                    if (angleRemainder + coinWork->coinSpinSpeeds[coinIndex] >= 40.0f) {
                        coinWork->coinAngles[coinIndex] += (angleRemainder + coinWork->coinSpinSpeeds[coinIndex]) - 40.0f;
                        angleRemainder = 0;
                    }
                    if (0.0f == angleRemainder) {
                        HuAudFXPlay(1067);
                        object->work[0] &= ~coinMask;
                        coinWork->coinSpinSpeeds[coinIndex] = 0;
                        coinPlayerObject->work[2] = REFRESH_RATE;
                        if (coinIndex != 2) {
                            coinPlayerObject->objFunc = CoinMinigamePlayerMotionUpdate;
                        }
                        else {
                            BoardAudSeqFadeOut(1, 100);
                            audioProcess = HuPrcCreate(CoinMinigameTimerStart, 8196, 6144, 0);
                            audioProcess->user_data = object;
                            HuAudFXStop(coinGameState);
                            object->objFunc = CoinMinigameCoinsFinish;
                        }
                    }
                }
                else {
                    coinWork->coinMotionFrames[coinIndex] -= coinWork->coinSpinSpeeds[coinIndex];
                    if (coinWork->coinMotionFrames[coinIndex] < 0.0f) {
                        coinWork->coinMotionFrames[coinIndex] = 0.0f;
                    }
                }
            }
            coinWork->coinAngles[coinIndex] += coinWork->coinSpinSpeeds[coinIndex];
            if (coinWork->coinAngles[coinIndex] >= 360.0f) {
                coinWork->coinAngles[coinIndex] -= 360.0f;
            }
        }
        coinRotation.x = coinWork->coinAngles[coinIndex];
        BoardModelRotSetV(coinModelIds[coinIndex], &coinRotation);
    }
}

void CoinMinigameCoinsFinish(OMOBJ *object)
{
    if (msmMusGetStatus(1) == 0) {
        BoardAudSeqPause(0, 0, 1000);
        object->objFunc = NULL;
    }
}

void CoinMinigameTimerUpdate(OMOBJ *object);

void CoinMinigameTimerStart(void)
{
    Vec coinPositions[20];
    Vec coinVelocities[20];
    Vec coinAngles[20];
    s32 coinValues[3];
    Vec modelRotation;
    Vec coinPosition;
    Vec playerPosition;
    char resultText[4];
    float angleRemainder;
    s32 coinIndex;
    s32 playerScore;
    s32 coinValue;
    s32 coinTotal;
    OMOBJ *coinObject;
    for (coinIndex = 0; coinIndex < 3; coinIndex++) {
        BoardModelRotGet(coinModelIds[coinIndex], &modelRotation);
        coinValues[coinIndex] = coinRewardAmounts[(int)(modelRotation.x / 40.0f)];
    }
    if (coinValues[0] == coinValues[1] && coinValues[0] == coinValues[2]) {
        coinPlayerMotionId = coinValues[0];
    }
    else {
        coinPlayerMotionId = 1;
    }
    if (coinPlayerMotionId != 1) {
        BoardModelVisibilitySet(boardModelIds[9], 1);
        BoardModelAttrReset(boardModelIds[9], 0x40000002);
    }
    sprintf(resultText, "%d", coinPlayerMotionId);
    BoardWinCreate(2, MAKE_MESSID(19, 19), 4);
    BoardWinInsertMesSet((u32)resultText, 0);
    BoardWinWait();
    BoardWinKill();
    BoardCameraTargetModelSet(-1);
    playerScore = 0;
    coinValue = 45;
    angleRemainder = 0;
    coinPosition = coinCenterPosition;
    HuAudFXPlay(1065);
    while (1) {
        coinTotal = 0;
        if (coinValue == 0 && playerScore < coinPlayerMotionId) {
            HuAudFXPlay(18);
            BoardModelVisibilitySet(coinModelPool[playerScore], 1);
            coinPositions[playerScore].x = -5.0f + ((1.0f / 255.0f) * (10.0f * frand8()));
            coinPositions[playerScore].y = 212.5f;
            coinPositions[playerScore].z = 130.0f;
            VECAdd(&coinPositions[playerScore], &coinPosition, &coinPositions[playerScore]);
            BoardModelPosSetV(coinModelPool[playerScore], &coinPositions[playerScore]);
            coinVelocities[playerScore].x = -1.0f + ((1.0f / 255.0f) * (2.0f * frand8()));
            coinVelocities[playerScore].y = 18.0f;
            coinVelocities[playerScore].z = 0.0f;
            coinValue = frandmod(3) + 5;
            playerScore++;
        }
        else {
            coinValue--;
        }
        for (coinIndex = 0; coinIndex < playerScore; coinIndex++) {
            if (coinPositions[coinIndex].y >= 500.0f) {
                BoardModelVisibilitySet(coinModelPool[coinIndex], 0);
                coinTotal++;
            }
            else {
                VECAdd(&coinPositions[coinIndex], &coinVelocities[coinIndex], &coinPositions[coinIndex]);
            }
            BoardModelPosSetV(coinModelPool[coinIndex], &coinPositions[coinIndex]);
        }
        coinPosition.y += 0.8f * sind(angleRemainder);
        angleRemainder += 45.0f;
        if (angleRemainder >= 360.0f) {
            angleRemainder -= 360.0f;
        }
        CoinMinigamePositionModels(&coinPosition);
        if (coinTotal == coinPlayerMotionId) {
            break;
        }
        HuPrcVSleep();
    }
    CoinMinigamePositionModels(&coinCenterPosition);
    BoardModelAttrSet(boardModelIds[9], 0x40000002);
    BoardModelMotionTimeSet(boardModelIds[9], 0);
    BoardModelVisibilitySet(boardModelIds[9], 0);
    BoardPlayerPosGet(coinPlayerIndex, &coinPosition);
    for (coinIndex = 0; coinIndex < coinPlayerMotionId; coinIndex++) {
        BoardModelVisibilitySet(coinModelPool[coinIndex], 1);
        coinPositions[coinIndex] = coinPosition;
        coinPositions[coinIndex].y += (coinIndex * 120) + 600.0f;
        coinPositions[coinIndex].x += -25.0f + ((1.0f / 255.0f) * (50.0f * frand8()));
        coinPositions[coinIndex].z += -25.0f + ((1.0f / 255.0f) * (50.0f * frand8()));
        coinAngles[coinIndex].x = coinAngles[coinIndex].z = 0;
        coinAngles[coinIndex].y = ((1.0f / 255.0f) * (360.0f * frand8()));
        BoardModelPosSetV(coinModelPool[coinIndex], &coinPositions[coinIndex]);
        BoardModelRotSetV(coinModelPool[coinIndex], &coinAngles[coinIndex]);
    }
    angleRemainder = -1;
    playerScore = 0;
    while (1) {
        for (coinIndex = 0; coinIndex < coinPlayerMotionId; coinIndex++) {
            if (coinIndex < playerScore) {
                continue;
            }
            coinPositions[coinIndex].y += angleRemainder;
            BoardModelPosSetV(coinModelPool[coinIndex], &coinPositions[coinIndex]);
            coinAngles[coinIndex].y += 45.0f;
            if (coinAngles[coinIndex].y >= 360.0f) {
                coinAngles[coinIndex].y -= 360.0f;
            }
            BoardModelRotSetV(coinModelPool[coinIndex], &coinAngles[coinIndex]);
            if (coinPositions[coinIndex].y <= 80.0f + coinPosition.y) {
                playerPosition = coinPositions[coinIndex];
                CharEffectCoinGlowCreate(1, &playerPosition);
                BoardModelVisibilitySet(coinModelPool[coinIndex], 0);
                playerScore++;
                BoardPlayerCoinsAdd(coinPlayerIndex, 1);
                HuAudFXPlay(7);
                omVibrate(coinPlayerIndex, 12, 6, 6);
            }
        }
        angleRemainder *= 1.05f;
        if (angleRemainder < -20.0f) {
            angleRemainder = -20.0f;
        }
        if (playerScore == coinPlayerMotionId) {
            BoardPlayerMotionShiftSet(coinPlayerIndex, 7, 0, 10, HU3D_MOTATTR_NONE);
            break;
        }
        HuPrcVSleep();
    }
    while (!BoardPlayerMotionEndCheck(coinPlayerIndex)) {
        HuPrcVSleep();
    }
    W02MesExec(MAKE_MESSID(0x13, 0x14));
    coinObject = HuPrcCurrentGet()->user_data;
    coinObject->objFunc = CoinMinigameTimerUpdate;
    HuPrcEnd();
}

void CoinMinigameTimerUpdate(OMOBJ *object)
{
    Vec coinRotation;
    s32 coinIndex;
    for (coinIndex = 0; coinIndex < 3; coinIndex++) {
        BoardModelRotGet(coinModelIds[coinIndex], &coinRotation);
        coinInitialAngles[coinIndex] = coinRotation.x;
    }
    CoinMinigamePlayerCoinsCleanup();
    omDelObjEx(boardObjMan, object);
}

void CoinMinigamePositionModels(Vec *coinCenterPosition)
{
    s32 coinIndex;
    Vec coinPosition;
    BoardModelPosSetV(boardModelIds[8], coinCenterPosition);
    BoardModelPosSetV(boardModelIds[9], coinCenterPosition);
    for (coinIndex = 0; coinIndex < 3; coinIndex++) {
        VECAdd(coinCenterPosition, &coinPlayerStartPositions[coinIndex], &coinPosition);
        BoardModelPosSetV(coinModelIds[coinIndex], &coinPosition);
        VECAdd(coinCenterPosition, &coinPlayerTargetOffsets[coinIndex], &coinPosition);
        BoardModelPosSetV(coinShadowModelIds[coinIndex], &coinPosition);
    }
}

void CoinMinigamePlayerMotionCleanup(OMOBJ *object);

void CoinMinigamePlayerMotionUpdate(OMOBJ *object)
{
    WorkMGCoin *coinWork;
    float windowDimensions[2];
    if (object->work[2] == 0) {
        coinWork = object->data;
        coinWork->countdownValue = 5;
        coinWork->countdownFrame = REFRESH_RATE;
        coinWork->timerSequenceId = MGSeqTimerCreateXY(coinWork->countdownValue, 288, 64);
        HuWinMesMaxSizeGet(1, windowDimensions, MAKE_MESSID(0x13, 0x15));
        coinWork->messageWindowId = HuWinCreate(-10000, 400, windowDimensions[0], windowDimensions[1], 0);
        HuWinMesSet(coinWork->messageWindowId, MAKE_MESSID(0x13, 0x15));
        HuWinBGTPLvlSet(coinWork->messageWindowId, 0);
        HuWinMesSpeedSet(coinWork->messageWindowId, 0);
        coinWork->inputDelay = frandmod(100) + 20;
        object->objFunc = CoinMinigamePlayerMotionCleanup;
    }
    else {
        object->work[2]--;
    }
}

void CoinMinigamePlayerMotionDestroy(OMOBJ *object);

void CoinMinigamePlayerMotionCleanup(OMOBJ *object)
{
    Vec playerPosition;
    Vec coinTargetPosition;
    WorkMGCoin *coinWork;
    s32 pressedButtons;
    s32 controllerPort;
    coinWork = object->data;
    controllerPort = GWPlayer[coinPlayerIndex].port;
    if (--coinWork->countdownFrame == 0) {
        coinWork->countdownFrame = REFRESH_RATE;
        if (--coinWork->countdownValue >= 0) {
            MGSeqParamSet(coinWork->timerSequenceId, 1, coinWork->countdownValue);
        }
    }
    if (!GWPlayer[coinPlayerIndex].com) {
        pressedButtons = HuPadBtnDown[controllerPort];
    }
    else {
        if (--coinWork->inputDelay == 0) {
            pressedButtons = PAD_BUTTON_A;
        }
    }
    if ((pressedButtons & PAD_BUTTON_A) || coinWork->countdownValue < 0) {
        MGSeqParamSet(coinWork->timerSequenceId, 2, -1);
        HuWinKill(coinWork->messageWindowId);
        BoardPlayerMotionShiftSet(coinPlayerIndex, 4, 0, 5, HU3D_MOTATTR_NONE);
        coinWork->verticalLaunchSpeed = 10;
        coinWork->bounceFrame = 0;
        if (coinTimerObject->work[0] & 0x1) {
            coinWork->playerIndex = 0;
        }
        else {
            if (coinTimerObject->work[0] & 0x2) {
                coinWork->playerIndex = 1;
            }
            else {
                coinWork->playerIndex = 2;
            }
        }
        BoardPlayerPosGet(coinPlayerIndex, &playerPosition);
        coinTargetPosition = coinCenterPosition;
        VECAdd(&coinTargetPosition, &coinPlayerTargetOffsets[coinWork->playerIndex], &coinTargetPosition);
        VECSubtract(&coinTargetPosition, &playerPosition, &coinWork->velocity);
        VECScale(&coinWork->velocity, &coinWork->velocity, 0.05f);
        object->work[0] = 1;
        object->objFunc = CoinMinigamePlayerMotionDestroy;
    }
}

void CoinMinigameCoinSelect(OMOBJ *object);

void CoinMinigamePlayerMotionDestroy(OMOBJ *object)
{
    Vec playerPosition;
    Vec coinTargetPosition;
    float distanceToTarget;
    WorkMGCoin *coinWork;
    coinWork = object->data;
    if (object->work[0] == 1) {
        BoardPlayerPosGet(coinPlayerIndex, &playerPosition);
        coinWork->velocity.y = coinWork->verticalLaunchSpeed - (0.075f * (0.25f * (coinWork->bounceFrame * coinWork->bounceFrame)));
        coinWork->bounceFrame++;
        VECAdd(&playerPosition, &coinWork->velocity, &playerPosition);
        if (playerPosition.y <= coinCenterPosition.y) {
            playerPosition.y = coinCenterPosition.y;
        }
        BoardPlayerPosSetV(coinPlayerIndex, &playerPosition);
        coinTargetPosition = coinCenterPosition;
        VECAdd(&coinTargetPosition, &coinPlayerTargetOffsets[coinWork->playerIndex], &coinTargetPosition);
        coinTargetPosition.y = playerPosition.y = 0;
        VECSubtract(&coinTargetPosition, &playerPosition, &playerPosition);
        distanceToTarget = VECMag(&playerPosition);
        if (distanceToTarget <= 1.0f) {
            BoardPlayerMotionStart(coinPlayerIndex, object->mtnId[0], 0);
            object->work[0] = 0;
        }
    }
    else {
        if (BoardPlayerMotionTimeGet(coinPlayerIndex) >= BoardPlayerMotionMaxTimeGet(coinPlayerIndex)) {
            coinWork->verticalLaunchSpeed = -5;
            coinWork->bounceFrame = 0;
            coinWork->velocity.x = coinWork->velocity.z = 0;
            object->objFunc = CoinMinigameCoinSelect;
        }
    }
}

void CoinMinigameCoinCollect(OMOBJ *object);

void CoinMinigameCoinSelect(OMOBJ *object)
{
    Vec playerPosition;
    Vec coinTargetPosition;
    WorkMGCoin *coinWork;
    coinWork = object->data;
    BoardPlayerPosGet(coinPlayerIndex, &playerPosition);
    coinWork->velocity.y = coinWork->verticalLaunchSpeed - (0.075f * (0.25f * (coinWork->bounceFrame * coinWork->bounceFrame)));
    coinWork->bounceFrame++;
    VECAdd(&playerPosition, &coinWork->velocity, &playerPosition);
    coinTargetPosition = coinCenterPosition;
    VECAdd(&coinTargetPosition, &coinPlayerTargetOffsets[coinWork->playerIndex], &coinTargetPosition);
    coinTargetPosition.y += 16.0f;
    if (playerPosition.y <= coinTargetPosition.y) {
        playerPosition.y = coinTargetPosition.y;
    }
    BoardPlayerPosSetV(coinPlayerIndex, &playerPosition);
    if (playerPosition.y == coinTargetPosition.y) {
        BoardModelMotionTimeSet(coinShadowModelIds[coinWork->playerIndex], 0);
        BoardModelAttrReset(coinShadowModelIds[coinWork->playerIndex], 0x40000002);
        BoardPlayerMotionShiftSet(coinPlayerIndex, object->mtnId[1], 0, 5, HU3D_MOTATTR_NONE);
        coinTimerObject->work[3] |= 1 << coinWork->playerIndex;
        coinWork->verticalLaunchSpeed = 5;
        coinWork->bounceFrame = 0;
        coinTargetPosition.x = object->trans.x;
        coinTargetPosition.y = object->trans.y;
        coinTargetPosition.z = object->trans.z;
        VECSubtract(&coinTargetPosition, &playerPosition, &coinWork->velocity);
        VECScale(&coinWork->velocity, &coinWork->velocity, 0.03f);
        HuAudFXPlay(1064);
        object->objFunc = CoinMinigameCoinCollect;
    }
}

void CoinMinigameFinish(OMOBJ *object);

void CoinMinigameCoinCollect(OMOBJ *object)
{
    float distanceToCoin;
    float playerHeight;
    Vec playerPosition;
    Vec coinPosition;
    WorkMGCoin *coinWork;
    coinWork = object->data;
    BoardPlayerPosGet(coinPlayerIndex, &playerPosition);
    coinWork->velocity.y = coinWork->verticalLaunchSpeed - (0.075f * (0.25f * (coinWork->bounceFrame * coinWork->bounceFrame)));
    coinWork->bounceFrame++;
    VECAdd(&playerPosition, &coinWork->velocity, &playerPosition);
    coinPosition.x = object->trans.x;
    coinPosition.y = 0;
    coinPosition.z = object->trans.z;
    playerHeight = playerPosition.y;
    playerPosition.y = 0;
    VECSubtract(&coinPosition, &playerPosition, &coinPosition);
    playerPosition.y = playerHeight;
    distanceToCoin = VECMag(&coinPosition);
    if (distanceToCoin <= 1.0f) {
        coinWork->velocity.x = coinWork->velocity.z = 0;
    }
    if (playerPosition.y <= object->trans.y) {
        playerPosition.x = object->trans.x;
        playerPosition.y = object->trans.y;
        playerPosition.z = object->trans.z;
        BoardPlayerIdleSet(coinPlayerIndex);
        if (coinWork->playerIndex != 2) {
            object->objFunc = NULL;
        }
        else {
            object->work[1] = 8;
            object->objFunc = CoinMinigameFinish;
        }
    }
    BoardPlayerPosSetV(coinPlayerIndex, &playerPosition);
}

void CoinMinigameFinish(OMOBJ *object)
{
    WorkMGCoin *coinWork;
    coinWork = object->data;
    if (object->work[1] != 0) {
        if (--object->work[1] == 0) {
            BoardPlayerMotBlendSet(coinPlayerIndex, 0, 15);
            object->objFunc = NULL;
        }
        (void)object;
    }
    else {
        (void)object;
    }
}
