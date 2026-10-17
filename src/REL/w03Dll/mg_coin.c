#include "REL/w03Dll.h"
#include "dolphin.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/chrman.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/pad.h"

#include "game/board/window.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/board/audio.h"
#include "game/board/com.h"
#include "game/objsub.h"
#endif

#include "ext_math.h"
#include <string.h>

// structs
typedef struct CoinCollectibleWork {
    /* 0x00 */ struct {
        u8 isActive : 1;
        u8 coinCount : 4;
        u8 isCollected : 2;
        u8 isHidden : 1;
    };
    u8 playerIndex;
    u8 coinState;
    u8 timer;
    s16 modelId;
    s16 motionIds[3];
    s32 counter;
} CoinCollectibleWork;

typedef struct CoinPlayerAnimationWork {
    s8 isActive;
    u8 playerIndex;
    s16 timer;
    f32 rotation;
    Vec position;
} CoinPlayerAnimationWork;

typedef struct CoinCollectionWork {
    /* 0x00 */ struct {
        u8 isActive : 1;
    };
    char unreferencedCoinData[3];
    s8 state;
    char unreferencedCoinData[3];
    f32 rotation;
    CoinPlayerAnimationWork *coinWork;
} CoinCollectionWork;

// function signatures
void CoinEventRunProcess(void);
void CoinEventProcessCleanup(void);
s32 CoinCheckPlayerChoice(s32 playerIndex);
void CoinRunCollectSequence(s32 value);
void CoinCollectibleCreate(s32 playerIndex);
void CoinCollectibleUpdate(OMOBJ *object);
void CoinCollectibleStateSet(s32 value);
void CoinAnimatePlayerCollect(OMOBJ *object, CoinCollectibleWork *collectibleWork);
void CoinCollectReward(OMOBJ *object, CoinCollectibleWork *collectibleWork);
void CoinAnimateRewardDisappear(OMOBJ *object, CoinCollectibleWork *collectibleWork);
void CoinAnimateRewardSpin(OMOBJ *object, CoinCollectibleWork *collectibleWork);
void CoinRewardValueUpdate(void);
void CoinReturnPlayerToSpace(s32 playerIndex);
void CoinResultWindowCreate(void);
void CoinShowResult(s32 playerIndex);
void CoinRewardObjectsCreate(s32 playerIndex);
void CoinCollectionObjectDestroy(CoinCollectionWork *collectionWork);

// data
s32 coinSoundHandle = -1;
s16 coinResultWindowId = -1;
s16 coinPedestalModelId = -1;
s16 coinPedestalMotionId = -1;
s32 coinPedestalMotionData[] = { DATA_MAKE_NUM(DATADIR_W03, 0x21), DATA_MAKE_NUM(DATADIR_W03, 0x22), DATA_NUM_LISTEND };

// BSS
char coinRewardAmountText[0x30];
s16 coinRewardTimer;
s8 coinRewardAmount;
OMOBJ *coinRewardObject;
OMOBJ *coinCollectibleObject;
HUPROCESS *coinEventProcess;

void CoinPedestalCreate(void)
{
    coinPedestalModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x20), coinPedestalMotionData, 0);
    BoardModelHookSet(boardModelIds[7], "phei", coinPedestalModelId);
    BoardModelMotionStart(coinPedestalModelId, 2, 0x40000002);
    BoardModelRotYSet(coinPedestalModelId, -45.0f);
}

void CoinPedestalDestroy(void)
{
    if (coinPedestalModelId != -1) {
        BoardModelKill(coinPedestalModelId);
        coinPedestalModelId = -1;
    }
}

void CoinEventStart(void)
{
    BoardRollDispSet(0);
    coinEventProcess = HuPrcChildCreate(&CoinEventRunProcess, 0x2003, 0x2000, 0, boardMainProc);
    HuPrcDestructorSet2(coinEventProcess, &CoinEventProcessCleanup);
    while (coinEventProcess) {
        HuPrcVSleep();
    }
    BoardRollDispSet(1);
}

void CoinEventProcessCleanup(void)
{
    if (coinResultWindowId != -1) {
        HuWinKill(coinResultWindowId);
        coinResultWindowId = -1;
    }
    coinEventProcess = 0;
}

void CoinEventRunProcess(void)
{
    s32 currentPlayerIndex;

    currentPlayerIndex = GWSystem.player_curr;
    coinRewardAmount = 0;
    coinRewardTimer = 0;
    if (CoinCheckPlayerChoice(currentPlayerIndex) != 0) {
        CoinRunCollectSequence(currentPlayerIndex);
        CoinShowResult(currentPlayerIndex);
        CoinReturnPlayerToSpace(currentPlayerIndex);
    }
    HuPrcEnd();
}

s32 CoinCheckPlayerChoice(s32 playerIndex)
{
    Vec spacePosition;
    Vec playerPosition;
    Vec positionDelta;
    Vec cameraRotation;
    Vec cameraRotation2;
    f32 angle;
    s16 model;
    s16 currentSpace;
    s16 linkedSpaceIndex;
    f32 arctan;

    BoardWinCreateChoice(2, MAKE_MESSID(22, 21), 2, 0);

    if (GWPlayer[playerIndex].com != 0) {
        BoardComKeySetLeft();
    }

    BoardWinWait();
    BoardWinKill();

    if (BoardWinChoiceGet() != 0) {
        BoardWinCreate(2, MAKE_MESSID(22, 24), 2);
        BoardWinWait();
        BoardWinKill();
        return 0;
    }

    BoardAudSeqPause(0, 1, 1000);

    cameraRotation.x = 0.0f;
    cameraRotation.y = 200.0f;
    cameraRotation.z = 0.0f;
    cameraRotation2.x = -10.0f;
    cameraRotation2.y = 0.0f;
    cameraRotation2.z = 0.0f;

    BoardCameraMotionStartEx(boardModelIds[7], &cameraRotation2, &cameraRotation, 1500.0f, -1.0f, 21);
    currentSpace = GWPlayer[playerIndex].space_curr;
    linkedSpaceIndex = BoardSpaceLinkFlagSearch(0, currentSpace, 0x02000000);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardSpacePosGet(0, linkedSpaceIndex, &spacePosition);
    VECSubtract(&spacePosition, &playerPosition, &positionDelta);
    VECNormalize(&positionDelta, &positionDelta);
    BoardPlayerRotYSet(playerIndex, atan2d(-positionDelta.x, -positionDelta.z));
    HuPrcSleep(0x1E);
    model = BoardModelIDGet(boardModelIds[7]);
    Hu3DModelObjPosGet(model, "phei", &spacePosition);

    if (GWPlayer[playerIndex].character == 5) {
        angle = 54.000004f;
    }
    else {
        angle = 39.0f;
    }

    spacePosition.x += angle * sind(180.0f + BoardModelRotYGet(coinPedestalModelId));
    spacePosition.z += angle * cosd(180.0f + BoardModelRotYGet(coinPedestalModelId));

    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardPlayerPosLerpStart(playerIndex, &playerPosition, &spacePosition, 0x14);

    while (GWPlayer[playerIndex].moving != 0) {
        BoardModelPosGet(boardModelIds[10], &playerPosition);
        BoardPlayerPosGet(playerIndex, &spacePosition);
        VECSubtract(&spacePosition, &playerPosition, &positionDelta);
        arctan = atan2d(positionDelta.x, positionDelta.z);
        BoardModelRotYSet(boardModelIds[10], arctan);
        HuPrcVSleep();
    }

    BoardPlayerMotBlendSet(playerIndex, -0x2D, 7);

    while (BoardPlayerMotBlendCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }

    return 1;
}

void CoinRunCollectSequence(s32 value)
{
    s16 integerValue;

    CoinCollectibleCreate(value);
    BoardMusStart(1, 0x16, 0x7F, 0);
    integerValue = MGSeqStartCreate();

    while (MGSeqStatGet(integerValue)) {
        HuPrcVSleep();
    }

    CoinResultWindowCreate();
    CoinCollectibleStateSet(0);

    while (coinCollectibleObject) {
        HuPrcVSleep();
    }

    CoinRewardValueUpdate();
}

s32 playerCoinCollectMotionData[] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x66),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x66),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x66),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x66),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x66),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x66),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x66),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x66),
};

s32 playerCoinRewardMotionData[] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x67),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x67),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x67),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x67),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x67),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x67),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x67),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x67),
};

void CoinCollectibleCreate(s32 playerIndex)
{
    Vec playerPosition;
    OMOBJ *object;
    f32 floatValue;
    s32 characterIndex;
    s32 integerValue;
    s32 integerValue2;
    CoinCollectibleWork *work;

    object = omAddObjEx(boardObjMan, 0x101, 0U, 0U, -1, CoinCollectibleUpdate);
    coinCollectibleObject = object;
    work = omObjGetWork(object, CoinCollectibleWork);
    work->isActive = 0;
    work->isCollected = playerIndex;
    work->coinCount = 4; //?
    work->playerIndex = 0;
    work->coinState = 5;
    work->timer = 0x3C;
    work->counter = -1;
    coinSoundHandle = -1;
    object->trans.x = 0.0f;
    object->trans.y = 0.0f;
    characterIndex = GWPlayer[playerIndex].character;
    work->motionIds[1] = BoardPlayerMotionCreate(playerIndex, playerCoinCollectMotionData[characterIndex]);
    work->motionIds[2] = BoardPlayerMotionCreate(playerIndex, playerCoinRewardMotionData[characterIndex]);
    floatValue = 0.3f;
    work->motionIds[0] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x09), NULL, 0);
    BoardModelVisibilitySet(work->motionIds[0], 0);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardModelPosSetV(work->motionIds[0], &playerPosition);
    BoardModelScaleSet(work->motionIds[0], floatValue, floatValue, floatValue);
    BoardModelMotionStart(work->motionIds[0], 0, 0x40000001);
    BoardModelMotionStart(coinPedestalModelId, 0, 0x40000001);
    BoardModelMotionSpeedSet(coinPedestalModelId, object->trans.x);
    BoardPlayerMotionStart(playerIndex, work->motionIds[1], 0x40000001);
    BoardPlayerMotionSpeedSet(playerIndex, object->trans.x);
    coinPedestalMotionId = BoardModelMotionCreate(boardModelIds[10], DATA_MAKE_NUM(DATADIR_W03, 0x25));
}

void CoinCollectibleDestroy(CoinCollectibleWork *collectibleWork)
{
    if (collectibleWork->motionIds[1] != -1) {
        BoardPlayerMotionKill(collectibleWork->isCollected, collectibleWork->motionIds[1]);
    }
    if (collectibleWork->motionIds[2] != -1) {
        BoardPlayerMotionKill(collectibleWork->isCollected, collectibleWork->motionIds[2]);
    }
    if (collectibleWork->motionIds[0] != -1) {
        BoardModelKill(collectibleWork->motionIds[0]);
    }
}

void CoinCollectibleStateSet(s32 value)
{
    CoinCollectibleWork *work;

    if (coinCollectibleObject) {
        work = omObjGetWork(coinCollectibleObject, CoinCollectibleWork);
        work->coinCount = value;
        if (value == 0) {
            work->modelId = MGSeqTimerCreateXY(work->coinState, 288, 64);
        }
    }
}

void CoinCollectibleUpdate(OMOBJ *object)
{
    s32 integerValue;
    CoinCollectibleWork *work;

    work = omObjGetWork(object, CoinCollectibleWork);

    if ((work->isActive != 0) || (BoardIsKill() != 0)) {
        if (work->modelId != -1) {
            MGSeqParamSet(work->modelId, 2, -1);
        }
        if (work->motionIds[1] != -1) {
            BoardPlayerMotionKill(work->isCollected, work->motionIds[1]);
        }
        if (work->motionIds[2] != -1) {
            BoardPlayerMotionKill(work->isCollected, work->motionIds[2]);
        }
        if (work->motionIds[0] != -1) {
            BoardModelKill(work->motionIds[0]);
        }
        coinCollectibleObject = NULL;

        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }

    if (work->playerIndex != 0) {
        work->playerIndex--;
        return;
    }

    switch (work->coinCount) {
        case 0:
            CoinAnimatePlayerCollect(object, work);
            return;
        case 1:
            CoinCollectReward(object, work);
            return;
        case 2:
            CoinAnimateRewardDisappear(object, work);
            return;
        case 3:
            CoinAnimateRewardSpin(object, work);
            /* fallthrough */
        case 4:
            break;
    }
}

void CoinAnimatePlayerCollect(OMOBJ *object, CoinCollectibleWork *collectibleWork)
{
    s32 randomValue;
    u32 stateValue;
    u32 playerIndex;
    f32 animationFrame;
    Vec modelPosition;

    if (collectibleWork->timer != 0) {
        collectibleWork->timer--;
    }
    else {
        if (collectibleWork->coinState != 0) {
            collectibleWork->coinState--;
        }
        else {
            MGSeqParamSet(collectibleWork->modelId, 2, -1);
            collectibleWork->modelId = -1;
            collectibleWork->coinCount = 1;
            animationFrame = BoardModelMotionTimeGet(boardModelIds[7]);

            if (animationFrame == 0.0f) {
                animationFrame = 1.0f;
            }

            if (coinRewardTimer > 30) {
                coinRewardTimer = 30;
            }

            OSu16tof32((u16 *)&coinRewardTimer, &object->rot.x); //??
            BoardModelAttrReset(coinPedestalModelId, 0x40000002);
            BoardModelAttrReset(boardModelIds[7], 0x40000002);
            BoardModelAttrReset(boardModelIds[10], 0x40000002);
            BoardPlayerModelAttrReset(collectibleWork->isCollected, 0x40000002);
            BoardModelMotionStart(coinPedestalModelId, 1, 0x40000001);
            BoardModelMotionStart(boardModelIds[7], 0, 0);
            BoardModelMotionStart(boardModelIds[10], coinPedestalMotionId, 0);
            BoardPlayerMotionStart(collectibleWork->isCollected, collectibleWork->motionIds[2], 0);
            BoardModelVisibilitySet(collectibleWork->motionIds[0], 0);
            collectibleWork->counter = HuAudFXPlay(0x43F);
            BoardModelPosGet(coinPedestalModelId, &modelPosition);
            object->trans.z = modelPosition.y;
            object->rot.y = 16.666666f * object->rot.x;
            object->scale.x = BoardModelMotionMaxTimeGet(boardModelIds[7]);
            HuWinDispOff(coinResultWindowId);
            BoardCameraMotionStartEx(-1, NULL, NULL, 1900.0f, -1.0f, 21);
            return;
        }

        MGSeqParamSet(collectibleWork->modelId, 1, collectibleWork->coinState);
        collectibleWork->timer = 60;
    }

    playerIndex = GWPlayer[collectibleWork->isCollected].port;
    stateValue = 0;

    if ((GWPlayer[collectibleWork->isCollected].com) != 0) {
        randomValue = BoardRandMod(100);

        switch (GWPlayer[collectibleWork->isCollected].diff) {
            case 0:
                if (randomValue < 12) {
                    stateValue = 0x100;
                }
                break;
            case 1:
                if (randomValue < 22) {
                    stateValue = 0x100;
                }
                break;
            case 2:
                if (randomValue < 50) {
                    stateValue = 0x100;
                }
                break;
            case 3:
                stateValue = 0x100;
                break;
        }
    }
    else {
        stateValue = HuPadBtnDown[playerIndex] & 0x100;
    }

    if ((stateValue & 0x100)) {
        object->trans.y += 0.5f;
        if (object->trans.y > 2.0f) {
            object->trans.y = 2.0f;
        }
        if (coinSoundHandle == -1) {
            coinSoundHandle = HuAudFXPlay(0x43E);
        }
        coinRewardTimer += 1;
        BoardModelVisibilitySet(collectibleWork->motionIds[0], 1);
        BoardModelRotYSet(collectibleWork->motionIds[0], BoardPlayerRotYGet(collectibleWork->isCollected));
    }
    else {
        object->trans.x *= 0.98f;
        object->trans.y = 0.0f;
        if (object->trans.x <= 1.0f) {
            BoardModelVisibilitySet(collectibleWork->motionIds[0], 0);
            if (coinSoundHandle != -1) {
                HuAudFXStop(coinSoundHandle);
                coinSoundHandle = -1;
            }
        }
    }

    if ((coinSoundHandle != -1) && (HuAudFXStatusGet(coinSoundHandle) == 0)) {
        coinSoundHandle = -1;
    }

    object->trans.x += object->trans.y;

    if (object->trans.x > 3.0f) {
        object->trans.x = 3.0f;
    }

    BoardPlayerMotionSpeedSet(collectibleWork->isCollected, object->trans.x);
    BoardModelMotionSpeedSet(coinPedestalModelId, object->trans.x);
}

s8 soundFrameIndex = -1;
s32 coinRewardSoundIds[] = { 0x00000440, 0x00000441, 0x00000442, 0x00000443, 0x00000444 };

void CoinCollectReward(OMOBJ *object, CoinCollectibleWork *collectibleWork)
{
    Vec modelPosition;
    f32 floatValue;
    f32 floatValue2;
    s8 integerValue;

    BoardModelPosGet(coinPedestalModelId, &modelPosition);
    modelPosition.y += 1.6666666f;

    if (modelPosition.y >= (object->trans.z + object->rot.y)) {
        modelPosition.y = object->trans.z + object->rot.y;
        BoardModelMotionSpeedSet(boardModelIds[10], 0.0f);
        BoardPlayerMotionSpeedSet(collectibleWork->isCollected, 0.0f);
        BoardModelAttrSet(boardModelIds[7], 0x40000002);
        BoardModelAttrSet(coinPedestalModelId, 0x40000002);
        collectibleWork->coinCount = 2;
        collectibleWork->playerIndex = 0x3C;
        collectibleWork->coinState = 0;
    }

    BoardModelPosSetV(coinPedestalModelId, &modelPosition);
    floatValue2 = object->scale.x * ((modelPosition.y - object->trans.z) / 500.0f);
    OSf32tos8(&floatValue2, &integerValue);
    integerValue = integerValue / 30;

    if (soundFrameIndex != integerValue) {
        soundFrameIndex = integerValue;
        HuAudFXPlay(coinRewardSoundIds[integerValue]);
    }

    BoardModelMotionTimeSet(boardModelIds[7], floatValue2);
    BoardModelMotionTimeSet(boardModelIds[10], 0.2f * floatValue2);
    BoardPlayerMotionTimeSet(collectibleWork->isCollected, 2.0f * floatValue2);
}

void CoinAnimateRewardDisappear(OMOBJ *object, CoinCollectibleWork *collectibleWork)
{
    Vec modelPosition;
    f32 floatValue;
    f32 floatValue2;

    if (collectibleWork->isHidden == 0) {
        HuAudFXPlay(0x448);
        collectibleWork->isHidden = 1;
        if (collectibleWork->counter != -1) {
            HuAudFXStop(collectibleWork->counter);
            collectibleWork->counter = -1;
        }
        BoardAudSeqFadeOut(1, 0x3E8);
    }

    OSu8tof32(&collectibleWork->coinState, &floatValue2);
    BoardModelPosGet(coinPedestalModelId, &modelPosition);

    modelPosition.y = (modelPosition.y + (-0.08166666666666668 * floatValue2 * floatValue2));

    if (modelPosition.y < object->trans.z) {
        modelPosition.y = object->trans.z;
        collectibleWork->coinCount = 3;
        object->trans.x = object->rot.y / 20.0f;
        object->trans.y = 0.0f;
        object->trans.z = 24.0f;
        BoardModelAttrReset(boardModelIds[10], 0x40000003);
        BoardModelMotionSpeedSet(boardModelIds[10], -2.0f);
        BoardPlayerModelAttrReset(collectibleWork->isCollected, 0x40000003);
        BoardPlayerMotionSpeedSet(collectibleWork->isCollected, -2.0f);
        HuAudFXPlay(0x449);
        BoardAudSeqPause(0, 0, 0x3E8);
    }

    collectibleWork->coinState += 1;
    BoardModelPosSetV(coinPedestalModelId, &modelPosition);
}

void CoinAnimateRewardSpin(OMOBJ *object, CoinCollectibleWork *collectibleWork)
{
    Vec position;

    BoardModelRotGet(coinPedestalModelId, &position);
    object->trans.y = BoardDAngleCalc(object->trans.y + object->trans.z);

    if (object->trans.y <= 2.0f) {
        if (object->trans.y >= -2.0) {
            object->trans.x *= 0.5f;
            if (object->trans.x <= 1.0f) {
                object->trans.y = 0.0f;
                BoardPlayerIdleSet(collectibleWork->isCollected);
                BoardModelMotionShiftSet(boardModelIds[10], 1, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
                collectibleWork->isActive = 1;
            }
        }
    }

    position.z = object->trans.x * sind(object->trans.y);
    BoardModelRotSetV(coinPedestalModelId, &position);
}

s8 coinRewardAmounts[] = { 1, 5, 10, 15, 20, 20 };

void CoinRewardValueUpdate(void)
{
    s8 integerValue;

    if (coinRewardTimer == 0) {
        coinRewardTimer = 1;
    }

    integerValue = coinRewardTimer / 6;
    coinRewardAmount = coinRewardAmounts[integerValue];
}

void CoinReturnPlayerToSpace(s32 playerIndex)
{
    Vec playerPosition;
    Vec spacePosition;
    Vec positionDelta;
    s16 currentSpaceIndex;
    f32 rotY;

    currentSpaceIndex = GWPlayer[playerIndex].space_curr;
    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardSpacePosGet(0, currentSpaceIndex, &spacePosition);
    BoardCameraTargetPlayerSet(playerIndex);
    BoardCameraViewSet(1);
    BoardPlayerPosLerpStart(playerIndex, &playerPosition, &spacePosition, 0x14);

    while (GWPlayer[playerIndex].moving != 0) {
        BoardModelPosGet(boardModelIds[10], &playerPosition);
        BoardPlayerPosGet(playerIndex, &spacePosition);
        VECSubtract(&spacePosition, &playerPosition, &positionDelta);
        rotY = atan2d(positionDelta.x, positionDelta.z);
        BoardModelRotYSet(boardModelIds[10], rotY);
        HuPrcVSleep();
    }

    BoardModelRotYSet(boardModelIds[10], 0.0f);
    BoardCameraMotionWait();
}

void CoinResultWindowCreate(void)
{
    f32 widthHeight[2];
    f32 x, y;

    HuWinMesMaxSizeGet(1, widthHeight, MAKE_MESSID(0x16, 0x19));
    x = -10000.0f;
    y = 288.0f;
    coinResultWindowId = HuWinCreate(x, y, widthHeight[0], widthHeight[1], 0);
    HuWinBGTPLvlSet(coinResultWindowId, 0.0f);
    HuWinMesSpeedSet(coinResultWindowId, 0);
    HuWinMesSet(coinResultWindowId, MAKE_MESSID(0x16, 0x19));
}

void CoinShowResult(s32 playerIndex)
{
    sprintf(coinRewardAmountText, "%d", coinRewardAmount);
    BoardWinCreate(2, MAKE_MESSID(22, 23), 2);
    BoardWinInsertMesSet((u32)coinRewardAmountText, 4);
    BoardWinWait();
    BoardWinKill();
    BoardPlayerMotBlendSet(playerIndex, 0, 0xF);

    while (BoardPlayerMotBlendCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }

    CoinRewardObjectsCreate(playerIndex);

    while (coinRewardObject) {
        HuPrcVSleep();
    }

    BoardPlayerMotionShiftSet(playerIndex, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(8);

    while (BoardPlayerMotionEndCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }

    BoardWinCreate(2, MAKE_MESSID(22, 24), 2);
    BoardWinWait();
    BoardWinKill();
    BoardModelMotionStart(boardModelIds[7], 0, 0x40000002);
}

void CoinRewardObjectUpdate(OMOBJ *object)
{
    f32 floatValue;
    s32 stateValue;
    s32 integerValue;
    s32 index;
    s32 integerValue2;
    CoinPlayerAnimationWork *coinWork;
    CoinCollectionWork *work;
    s32 i;
    s32 activeCoinCount;

    work = omObjGetWork(object, CoinCollectionWork);

    if ((work->isActive != 0) || (BoardIsKill() != 0)) {
        CoinCollectionObjectDestroy(work);
        HuMemDirectFree((void *)work->counter);
        coinRewardObject = 0;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }

    coinWork = work->counter;

    for (activeCoinCount = i = 0; i < 20; i++, coinWork++) {
        if (coinWork->isActive == 0) {
            activeCoinCount++;
        }
    }

    if (activeCoinCount == 20) {
        work->isActive = 1;
        return;
    }

    stateValue = 0;
    for (i = 0; i < 20; i++) {
        coinWork = &work->counter[i];
        if ((coinWork->isActive == 1) && (stateValue == 0)) {
            coinWork->isActive = 2U;
            stateValue = 1;
        }
        if (coinWork->isActive == 2) {
            coinWork->playerIndex++;
            OSu8tof32(&coinWork->playerIndex, &floatValue);
            coinWork->rotation += 45.0f;
            coinWork->position.y += -0.08166667f * floatValue * floatValue * 0.75f;
            if (coinWork->position.y < work->position) {
                coinWork->position.y = work->position;
                CharEffectCoinGlowCreate(1, &coinWork->position);
                BoardModelVisibilitySet(coinWork->timer, 0);
                HuAudFXPlay(7);
                BoardPlayerCoinsAdd(work->rotation, 1);
                omVibrate(work->rotation, 0xC, 6, 6);
                coinWork->isActive = 0;
            }
            BoardModelPosSetV(coinWork->timer, &coinWork->position);
            BoardModelRotYSet(coinWork->timer, coinWork->rotation);
        }
    }
}

void CoinRewardObjectsCreate(s32 playerIndex)
{
    Vec playerPosition;
    f32 floatValue;
    f32 floatValue2;
    f64 unusedDoubleValue;
    OMOBJ *object;
    s32 i;
    CoinCollectionWork *work;
    CoinPlayerAnimationWork *coinWork;

    object = omAddObjEx(boardObjMan, 0x101, 0U, 0U, -1, CoinRewardObjectUpdate);
    coinRewardObject = object;
    work = omObjGetWork(object, CoinCollectionWork);
    work->isActive = 0;
    work->rotation = playerIndex;
    work->counter = HuMemDirectMallocNum(0, 0x190, HU_MEMNUM_OVL);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    work->position = (80.0f + playerPosition.y);
    memset(work->counter, 0, 0x190);

    for (i = 0; i < coinRewardAmount; i++) {
        if (i == 0) {
            floatValue2 = 0.0f;
        }
        else {
            floatValue2 = BoardRandFloat();
        }
        coinWork = &work->counter[i];
        coinWork->isActive = 1;
        coinWork->position.x = playerPosition.x + (50.0f * sind(floatValue2));
        coinWork->position.z = playerPosition.z + (50.0f * cosd(floatValue2));
        coinWork->position.y = 600.0f + playerPosition.y + (500.0f * BoardRandFloat());
        coinWork->playerIndex = 0;
        coinWork->rotation = BoardRandFloat() * 360.0f;
        coinWork->timer = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x0A), NULL, 1);
    }
}

void CoinCollectionObjectDestroy(CoinCollectionWork *collectionWork)
{
    s32 i;
    CoinPlayerAnimationWork *work;

    for (i = 0; i < 20; i++) {
        work = &collectionWork->counter[i];
        BoardModelKill(work->timer);
    }
}
