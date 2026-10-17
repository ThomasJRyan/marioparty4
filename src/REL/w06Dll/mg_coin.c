#include "REL/w06Dll.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/gamework_data.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/window.h"
#include "game/board/audio.h"
#include "game/board/com.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/window.h"

#include "ext_math.h"
#include <string.h>

typedef struct {
    struct {
        u8 isActive : 1;
        u8 dropState : 4;
        u8 isVisible : 1;
    };
    u8 animationFrames;
    u8 stateDelayFrames;
    char padding[1];
    s16 coinModelId;
} CoinDropObjectWork;

typedef struct {
    struct {
        u8 isActive : 1;
        u8 activePlayerIndex : 2;
        u8 minigameState : 4;
    };
    u8 frameCountdown;
    s16 playerMotionId;
    s16 playerModelId;
} CoinMinigameObjectWork;

// Coin reward particle state, matching the layout used in w03Dll.
typedef struct {
    /* 0x00 */ s8 isActive;
    /* 0x01 */ u8 animationFrames;
    /* 0x02 */ s16 coinModelId;
    /* 0x04 */ float rotation;
    /* 0x08 */ Vec position;
} CoinCollectionState;

// Coin reward object state, matching the layout used in w03Dll.
typedef struct {
    struct {
        u8 isActive : 1;
    };
    char padding[3];
    s8 playerIndex;
    char padding2[3];
    float coinLandingY;
    CoinCollectionState *coinParticles;
} CoinCollectionWork;

static void W06CoinEventCleanup(void);
static void W06CoinEventProcess(void);
static s32 W06CoinPlayerAccept(s32 playerIndex);
static void W06CoinMinigamePlay(s32 playerIndex);
static void W06CoinPlayerReturn(s32 playerIndex);
static void W06CoinMinigameInitialize(void);
static void W06CoinDropCreate(s32 playerIndex);
static void W06CoinDropUpdate(OMOBJ *coinObject);
static void W06CoinDropFall(OMOBJ *coinObject, CoinDropObjectWork *coinWork);
static void W06CoinDropCollect(OMOBJ *coinObject, CoinDropObjectWork *coinWork);
static void W06CoinDropExpire(OMOBJ *coinObject, CoinDropObjectWork *coinWork);
static void W06CoinPlayerPositionSet(Vec *position);
static void W06CoinCountDisplaySet(s32 coinCount);
static s32 W06CoinPlayersFinishedCheck(void);
static void W06CoinPlayerControl(s32 playerIndex);
static void W06CoinPlayerMotionSet(s32 playerIndex);
static s32 W06CoinCountGet(void);
static void W06CoinPositionGet(Vec *position);
static void W06CoinHudUpdate(s32 playerIndex, u32 *coinScore);
static void W06CoinMinigameObjectUpdate(OMOBJ *minigameObject);
static void W06CoinCollectionObjectUpdate(OMOBJ *collectionObject);
static void W06CoinRewardCreate(s32 playerIndex);
static void W06CoinParticleUpdate(CoinCollectionWork *collectionWork);

static s32 coinTotalCollected;
static char coinResultText[49];
static s8 coinWinnersCount;
static s8 coinCurrentRank;
static s8 coinRewardAmount;
static OMOBJ *coinRewardObject;
static OMOBJ *coinMinigameObject;
static OMOBJ *coinCollectionObject;
static HUPROCESS *coinEventProcess;

static const u8 coinRewardAmounts[] = {
    20, 15, 10, 5, 1
};

static const u8 coinPlacementRewards[] = {
    10, 7, 5, 3, 0, 0, 0
};

static s16 coinRewardWindow = -1;
static s16 coinPlayerMotion = -1;
static s16 coinBoardModel = -1;

void W06CoinBoardInitialize(void) {
    Vec boardModelPosition;
    float boardModelScale;

    coinBoardModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W06, 7), NULL, 0);
    boardModelScale = 0.3f;
    BoardModelPosGet(w06MapObjectModelIds[0], &boardModelPosition);
    boardModelPosition.x += 115.0 * sin(M_PI);
    boardModelPosition.z += 115.0 * cos(M_PI);
    boardModelPosition.x -= 225.0f;
    BoardModelPosSetV(coinBoardModel, &boardModelPosition);
    BoardModelRotYSet(coinBoardModel, 90.0f);
    BoardModelScaleSet(coinBoardModel, boardModelScale, boardModelScale, boardModelScale);
    BoardModelMotionStart(coinBoardModel, 0, 0);
    BoardModelMotionSpeedSet(coinBoardModel, 0.0f);
    BoardModelMotionStart(w06MapObjectModelIds[0], 0, 0);
    BoardModelMotionSpeedSet(w06MapObjectModelIds[0], 0.0f);
}

void W06CoinBoardCleanup(void) {
    if (coinBoardModel != -1) {
        BoardModelKill(coinBoardModel);
        coinBoardModel = -1;
    }
}

void W06CoinSpaceEventStart(void) {
    coinTotalCollected = 0;
    BoardRollDispSet(0);
    coinEventProcess = HuPrcChildCreate(W06CoinEventProcess, 0x2003, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(coinEventProcess, W06CoinEventCleanup);
    while (coinEventProcess) {
        HuPrcVSleep();
    }
    BoardRollDispSet(1);
}

static void W06CoinEventCleanup(void) {
    if (coinPlayerMotion != -1) {
        BoardModelMotionKill(w06MapObjectModelIds[5], coinPlayerMotion);
        coinPlayerMotion = -1;
    }
    if (coinRewardWindow != -1) {
        HuWinKill(coinRewardWindow);
        coinRewardWindow = -1;
    }
    coinEventProcess = NULL;
}

static void W06CoinEventProcess(void) {
    s32 choiceAccepted;

    choiceAccepted = GWSystem.player_curr;
    if (W06CoinPlayerAccept(choiceAccepted) != 0) {
        W06CoinMinigamePlay(choiceAccepted);
    }
    W06CoinPlayerReturn(choiceAccepted);
    HuPrcEnd();
}

static s32 W06CoinPlayerAccept(s32 playerIndex) {
    Vec playerPosition;
    Vec eventPosition;
    Vec movementVector;
    Vec cameraOffset;
    Vec targetPosition;

    BoardWinCreateChoice(2, MAKE_MESSID(48, 26), 6, 0);
    if (GWPlayer[playerIndex].com) {
        BoardComKeySetLeft();
    }
    BoardWinWait();
    if (BoardWinChoiceGet() != 0) {
        BoardWinCreate(2, MAKE_MESSID(48, 27), 6);
        BoardWinWait();
        BoardWinKill();
        coinTotalCollected = 1;
        return 0;
    }
    BoardWinKill();
    cameraOffset.x = 0.0f;
    cameraOffset.y = 120.0f;
    cameraOffset.z = 0.0f;
    targetPosition.x = -32.0f;
    targetPosition.y = 0.0f;
    targetPosition.z = 0.0f;
    BoardCameraMotionStartEx(w06MapObjectModelIds[0], &targetPosition, &cameraOffset, 1330.0f, -1.0f, 21);
    coinPlayerMotion = BoardModelMotionCreate(w06MapObjectModelIds[5], DATA_MAKE_NUM(DATADIR_W06, 22));
    BoardCameraMotionWait();
    BoardPlayerPosGet(playerIndex, &eventPosition);
    BoardModelPosGet(w06MapObjectModelIds[0], &playerPosition);
    playerPosition.x += 115.0 * sin(M_PI);
    playerPosition.z += 115.0 * cos(M_PI);
    VECSubtract(&playerPosition, &eventPosition, &movementVector);
    VECNormalize(&movementVector, &movementVector);
    BoardPlayerRotYSet(playerIndex, atan2d(-movementVector.x, -movementVector.z));
    BoardPlayerPosLerpStart(playerIndex, &eventPosition, &playerPosition, 40);
    BoardModelPosGet(w06MapObjectModelIds[5], &eventPosition);
    BoardModelMotionShiftSet(w06MapObjectModelIds[5], coinPlayerMotion, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    BoardModelMotionSpeedSet(w06MapObjectModelIds[5], 2.0f);
    while (GWPlayer[playerIndex].moving) {
        BoardPlayerPosGet(playerIndex, &playerPosition);
        VECSubtract(&playerPosition, &eventPosition, &movementVector);
        BoardModelRotYSet(w06MapObjectModelIds[5], atan2d(movementVector.x, movementVector.z));
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(w06MapObjectModelIds[5], 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    BoardPlayerIdleSet(playerIndex);
    BoardPlayerRotYSet(playerIndex, -90.0f);
    BoardAudSeqPause(0, 1, 1000);
    HuPrcSleep(60);
    return 1;
}

static void W06CoinMinigamePlay(s32 playerIndex) {
    s16 sequenceId;

    BoardMusStart(1, 0x27, 0x7F, 0);
    sequenceId = MGSeqCreate(3, 0);
    while (MGSeqStatGet(sequenceId) != 0) {
        HuPrcVSleep();
    }
    W06CoinMinigameInitialize();
    W06CoinDropCreate(playerIndex);
    W06CoinPlayerControl(playerIndex);
    while (TRUE) {
        if (W06CoinCountGet() == 4 && W06CoinPlayersFinishedCheck() == 4) {
            break;
        }
        HuPrcVSleep();
    }
    HuWinDispOff(coinRewardWindow);
    BoardAudSeqFadeOut(1, 1000);
    HuPrcSleep(30);
    coinRewardAmount = coinRewardAmounts[coinWinnersCount - 1];
    sprintf(coinResultText, "%d", coinRewardAmount);
    BoardAudSeqPause(0, 0, 1000);
    BoardWinCreate(2, MAKE_MESSID(48, 28), 6);
    BoardWinInsertMesSet(MAKE_MESSID_PTR(coinResultText), 0);
    BoardWinWait();
    BoardWinKill();
    W06CoinRewardCreate(playerIndex);
    while (coinRewardObject) {
        HuPrcVSleep();
    }
    BoardPlayerMotionShiftSet(playerIndex, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(8);
    while (!BoardPlayerMotionEndCheck(playerIndex)) {
        HuPrcVSleep();
    }
    BoardWinCreate(2, MAKE_MESSID(48, 29), 6);
    BoardWinWait();
    BoardWinKill();
}

static void W06CoinPlayerReturn(s32 playerIndex) {
    Vec playerPosition;
    Vec spacePosition;
    Vec movementDirection;
    s16 currentSpaceIndex;

    if (coinTotalCollected == 0) {
        currentSpaceIndex = GWPlayer[playerIndex].space_curr;
        BoardPlayerPosGet(playerIndex, &playerPosition);
        BoardSpacePosGet(0, currentSpaceIndex, &spacePosition);
    }
    BoardCameraTargetPlayerSet(playerIndex);
    BoardCameraViewSet(1);
    if (coinTotalCollected == 0) {
        BoardPlayerPosLerpStart(playerIndex, &playerPosition, &spacePosition, 40);
        BoardModelPosGet(w06MapObjectModelIds[5], &playerPosition);
        BoardModelMotionShiftSet(w06MapObjectModelIds[5], coinPlayerMotion, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
        BoardModelMotionSpeedSet(w06MapObjectModelIds[5], 2.0f);
        while (GWPlayer[playerIndex].moving) {
            BoardPlayerPosGet(playerIndex, &spacePosition);
            VECSubtract(&spacePosition, &playerPosition, &movementDirection);
            BoardModelRotYSet(w06MapObjectModelIds[5], atan2d(movementDirection.x, movementDirection.z));
            HuPrcVSleep();
        }
        BoardModelMotionShiftSet(w06MapObjectModelIds[5], 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    }
    BoardModelRotYSet(w06MapObjectModelIds[5], 0.0f);
    BoardCameraMotionWait();
    W06CoinCountDisplaySet(5);
    W06CoinPlayerMotionSet(5);
}

static void W06CoinMinigameInitialize(void) {
    float windowSize[2];
    float windowLeft;
    float windowTop;

    HuWinMesMaxSizeGet(1, windowSize, MAKE_MESSID(48, 30));
    windowLeft = -10000.0f;
    windowTop = 328.0f;
    coinRewardWindow = HuWinCreate(windowLeft, windowTop, windowSize[0], windowSize[1], 0);
    HuWinBGTPLvlSet(coinRewardWindow, 0.0f);
    HuWinMesSpeedSet(coinRewardWindow, 0);
    HuWinMesSet(coinRewardWindow, MAKE_MESSID(48, 30));
}

static void W06CoinDropCreate(s32 playerIndex) {
    Vec coinPosition;
    Vec playerPosition;
    Vec dropVector;
    Vec dropDirection;
    float modelScale;
    OMOBJ *itemObject;
    CoinDropObjectWork *dropWork;
    s32 coinModelResources[] = {
        DATA_MAKE_NUM(DATADIR_W06, 28),
        DATA_MAKE_NUM(DATADIR_W06, 29),
        -1
    };

    itemObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W06CoinDropUpdate);
    coinCollectionObject = itemObject;
    dropWork = omObjGetWork(itemObject, CoinDropObjectWork);
    dropWork->isActive = 0;
    dropWork->stateDelayFrames = 0;
    dropWork->isVisible = 0;
    dropWork->dropState = 2;
    BoardModelPosGet(coinBoardModel, &coinPosition);
    coinPosition.x += 50.0f;
    BoardPlayerPosGet(playerIndex, &playerPosition);
    playerPosition.x -= 10.0f;
    VECSubtract(&playerPosition, &coinPosition, &dropVector);
    VECNormalize(&dropVector, &dropDirection);
    itemObject->trans.x = dropVector.x / 60.0f;
    itemObject->trans.y = dropVector.y / 60.0f;
    itemObject->trans.z = dropVector.z / 60.0f;
    dropWork->coinModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W06, 27), coinModelResources, 0);
    modelScale = 0.3f;
    BoardModelPosSetV(dropWork->coinModelId, &coinPosition);
    BoardModelScaleSet(dropWork->coinModelId, modelScale, modelScale, modelScale);
    BoardModelVisibilitySet(dropWork->coinModelId, 0);
    BoardModelRotYSet(dropWork->coinModelId, atan2d(dropDirection.x, dropDirection.z));
    BoardModelMotionStart(dropWork->coinModelId, 1, 0x40000001);
    BoardModelMotionStart(coinBoardModel, 0, 0);
}

static void W06CoinDropUpdate(OMOBJ *coinObject) {
    CoinDropObjectWork *dropWork;

    dropWork = omObjGetWork(coinObject, CoinDropObjectWork);
    if (dropWork->isActive != 0 || BoardIsKill()) {
        BoardModelKill(dropWork->coinModelId);
        coinCollectionObject = NULL;
        omDelObjEx(HuPrcCurrentGet(), coinObject);
        return;
    }
    if (dropWork->stateDelayFrames != 0) {
        dropWork->stateDelayFrames--;
        return;
    }
    switch (dropWork->dropState) {
        case 0:
            break;
        case 1:
            W06CoinDropFall(coinObject, dropWork);
            break;
        case 4:
            W06CoinDropExpire(coinObject, dropWork);
            break;
        case 2:
            W06CoinDropCollect(coinObject, dropWork);
            break;
        case 5:
            dropWork->isActive = 1;
            break;
    }
}

static void W06CoinDropFall(OMOBJ *coinObject, CoinDropObjectWork *coinWork) {
    Vec playerPosition;

    if (coinWork->animationFrames == 0) {
        W06CoinCountDisplaySet(3);
        return;
    }
    BoardModelPosGet(coinWork->coinModelId, &playerPosition);
    playerPosition.x += coinObject->trans.x;
    playerPosition.y += coinObject->trans.y;
    playerPosition.z += coinObject->trans.z;
    BoardModelPosSetV(coinWork->coinModelId, &playerPosition);
    coinWork->animationFrames--;
}

static void W06CoinDropCollect(OMOBJ *coinObject, CoinDropObjectWork *coinWork) {
    if (BoardModelMotionTimeGet(coinBoardModel) < 32.0f) {
        return;
    }
    coinWork->dropState = 1;
    coinWork->animationFrames = 60;
    HuAudFXPlayVolPan(0x49E, 0x7F, 0x20);
    BoardModelVisibilitySet(coinWork->coinModelId, 1);
}

static void W06CoinDropExpire(OMOBJ *coinObject, CoinDropObjectWork *coinWork) {
    if (coinWork->isVisible == 0) {
        HuAudFXPlay(0x49F);
        coinWork->isVisible = 1;
    }
    if (BoardModelMotionEndCheck(coinWork->coinModelId)) {
        W06CoinCountDisplaySet(0);
    }
}

static void W06CoinPlayerPositionSet(Vec *position) {
    CoinDropObjectWork *dropWork;

    if (coinCollectionObject && position) {
        dropWork = omObjGetWork(coinCollectionObject, CoinDropObjectWork);
        BoardModelPosGet(dropWork->coinModelId, position);
    }
}

static void W06CoinCountDisplaySet(s32 coinCount) {
    CoinDropObjectWork *dropWork;

    if (!coinCollectionObject) {
        return;
    }
    dropWork = omObjGetWork(coinCollectionObject, CoinDropObjectWork);
    dropWork->dropState = coinCount;
    if (coinCount == 1) {
        coinCollectionObject->trans.x = 0.96666666f * sind(90);
        coinCollectionObject->trans.z = 0.96666666f * cosd(90);
        dropWork->animationFrames = 60;
        return;
    }
    if (coinCount == 0) {
        BoardPlayerMotionSpeedSet(dropWork->coinModelId, 1.0f);
        return;
    }
    if (coinCount == 4) {
        BoardModelMotionShiftSet(dropWork->coinModelId, 2, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
        dropWork->stateDelayFrames = 10;
        return;
    }
}

static s32 W06CoinPlayersFinishedCheck(void) {
    CoinDropObjectWork *dropWork;

    if (coinCollectionObject) {
        dropWork = omObjGetWork(coinCollectionObject, CoinDropObjectWork);
        return dropWork->dropState;
    }
    return -1;
}

static s32 coinCharacterMotions[8] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 126),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 126),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 126),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 126),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 126),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 126),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 126),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 126)
};

static void W06CoinPlayerControl(s32 playerIndex) {
    Vec playerPosition;
    float modelScale;
    OMOBJ *itemObject;
    CoinMinigameObjectWork *minigameWork;

    itemObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W06CoinMinigameObjectUpdate);
    coinMinigameObject = itemObject;
    minigameWork = omObjGetWork(itemObject, CoinMinigameObjectWork);
    minigameWork->isActive = 0;
    minigameWork->activePlayerIndex = playerIndex;
    minigameWork->frameCountdown = 0;
    minigameWork->minigameState = 1;
    minigameWork->playerModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W06, 13), NULL, 0);
    minigameWork->playerMotionId = BoardPlayerMotionCreate(playerIndex, coinCharacterMotions[GWPlayer[playerIndex].character]);
    modelScale = 0.3f;
    BoardModelScaleSet(minigameWork->playerModelId, modelScale, modelScale, modelScale);
    BoardPlayerPosGet(minigameWork->activePlayerIndex, &playerPosition);
    BoardModelPosSetV(minigameWork->playerModelId, &playerPosition);
    BoardModelVisibilitySet(minigameWork->playerModelId, 0);
    itemObject->trans.x = playerPosition.x;
    itemObject->trans.y = playerPosition.y;
    itemObject->trans.z = playerPosition.z;
    coinCurrentRank = 0;
    coinWinnersCount = 0;
}

static void W06CoinPlayerMotionSet(s32 playerIndex) {
    Vec playerPosition;
    Vec targetPosition;
    CoinMinigameObjectWork *minigameWork;

    if (!coinMinigameObject) {
        return;
    }
    minigameWork = omObjGetWork(coinMinigameObject, CoinMinigameObjectWork);
    minigameWork->minigameState = playerIndex;
    if (playerIndex == 2) {
        minigameWork->frameCountdown = 60;
        BoardPlayerPosGet(minigameWork->activePlayerIndex, &targetPosition);
        if (coinWinnersCount >= 1) {
            BoardModelVisibilitySet(minigameWork->playerModelId, 1);
        }
        BoardModelMotionStart(minigameWork->playerModelId, 0, 0x40000001);
        BoardPlayerMotionStart(minigameWork->activePlayerIndex, minigameWork->playerMotionId, 0x40000001);
        omVibrate(minigameWork->activePlayerIndex, 12, 4, 2);
        W06CoinPlayerPositionSet(&playerPosition);
        targetPosition.x = playerPosition.x + 18.0 * sind(90);
        targetPosition.z = playerPosition.z + 18.0 * cosd(90);
        BoardPlayerPosSetV(minigameWork->activePlayerIndex, &targetPosition);
        BoardModelPosSetV(minigameWork->playerModelId, &targetPosition);
        return;
    }
    if (playerIndex == 3) {
        BoardModelVisibilitySet(minigameWork->playerModelId, 0);
        BoardPlayerMotionStart(minigameWork->activePlayerIndex, minigameWork->playerMotionId, 0x40000001);
        return;
    }
}

static s32 W06CoinCountGet(void) {
    CoinMinigameObjectWork *minigameWork;

    if (!coinMinigameObject) {
        return;
    }
    minigameWork = omObjGetWork(coinMinigameObject, CoinMinigameObjectWork);
    return minigameWork->minigameState;
}

static void W06CoinPositionGet(Vec *position) {
    CoinMinigameObjectWork *minigameWork;

    if (coinMinigameObject) {
        minigameWork = omObjGetWork(coinMinigameObject, CoinMinigameObjectWork);
        BoardPlayerPosGet(minigameWork->activePlayerIndex, position);
    }
}

static void W06CoinHudUpdate(s32 playerIndex, u32 *coinScore) {
    u32 controllerPort;
    s32 controllerPort2;
    s32 controllerPort3;

    controllerPort = 0;
    controllerPort2 = GWPlayer[playerIndex].port;
    if (GWPlayer[playerIndex].com) {
        controllerPort3 = BoardRandMod(100);
        switch (GWPlayer[playerIndex].diff) {
            case 0:
                if (controllerPort3 < 12) {
                    controllerPort = 0x100;
                }
                break;
            case 1:
                if (controllerPort3 < 22) {
                    controllerPort = 0x100;
                }
                break;
            case 2:
                if (controllerPort3 < 50) {
                    controllerPort = 0x100;
                }
                break;
            case 3:
                controllerPort = 0x100;
                break;
        }
    } else {
        controllerPort = HuPadBtnDown[controllerPort2] & 0x100;
    }
    *coinScore = controllerPort;
}

static void W06CoinMinigameObjectUpdate(OMOBJ *minigameObject) {
    Vec playerPosition;
    Vec targetPosition;
    u32 scoreInput;
    CoinMinigameObjectWork *minigameWork;

    scoreInput = 0;
    minigameWork = omObjGetWork(minigameObject, CoinMinigameObjectWork);
    if (minigameWork->isActive != 0 || BoardIsKill()) {
        BoardPlayerMotionKill(minigameWork->activePlayerIndex, minigameWork->playerMotionId);
        BoardModelKill(minigameWork->playerModelId);
        coinMinigameObject = NULL;
        omDelObjEx(HuPrcCurrentGet(), minigameObject);
        return;
    }
    switch (minigameWork->minigameState) {
        case 0:
            break;
        case 1:
            if (W06CoinPlayersFinishedCheck() == 3) {
                W06CoinPlayerMotionSet(2);
            }
            break;
        case 2:
            if (minigameWork->frameCountdown == 0) {
                if (coinWinnersCount >= 5) {
                    W06CoinPlayerMotionSet(4);
                    W06CoinCountDisplaySet(4);
                } else {
                    W06CoinPlayerMotionSet(3);
                    W06CoinCountDisplaySet(1);
                    coinCurrentRank = 0;
                    coinWinnersCount++;
                }
            } else {
                W06CoinHudUpdate(minigameWork->activePlayerIndex, &scoreInput);
                if (scoreInput == 0x100) {
                    coinCurrentRank++;
                    if (coinWinnersCount != 0 && coinCurrentRank >= coinPlacementRewards[coinWinnersCount - 1]) {
                        W06CoinPlayerMotionSet(4);
                        W06CoinCountDisplaySet(4);
                    }
                    BoardPlayerMotionStart(minigameWork->activePlayerIndex, minigameWork->playerMotionId, 0x40000001);
                }
                minigameWork->frameCountdown--;
            }
            break;
        case 3:
            if (W06CoinPlayersFinishedCheck() == 3) {
                W06CoinPlayerMotionSet(2);
                return;
            }
            W06CoinPlayerPositionSet(&playerPosition);
            BoardPlayerPosGet(minigameWork->activePlayerIndex, &targetPosition);
            targetPosition.x = playerPosition.x + 18.0 * sind(90);
            targetPosition.z = playerPosition.z + 18.0 * cosd(90);
            BoardPlayerPosSetV(minigameWork->activePlayerIndex, &targetPosition);
            BoardModelPosSetV(minigameWork->playerModelId, &targetPosition);
            BoardModelRotYSet(minigameWork->playerModelId, BoardPlayerRotYGet(minigameWork->activePlayerIndex));
            break;
        case 5:
            minigameWork->isActive = 1;
            /* fallthrough */
        case 4:
            BoardPlayerIdleSet(minigameWork->activePlayerIndex);
            BoardPlayerRotYSet(minigameWork->activePlayerIndex, 0.0f);
            BoardModelVisibilitySet(minigameWork->playerModelId, 0);
            W06CoinPlayerMotionSet(0);
            break;
    }
}

static void W06CoinCollectionObjectUpdate(OMOBJ *collectionObject) {
    float fallDistance;
    CoinCollectionWork *collectionWork;
    CoinCollectionState *coinParticle;
    s32 playerIndex;
    s32 inactiveParticleCount;
    s32 i;

    collectionWork = omObjGetWork(collectionObject, CoinCollectionWork);
    if (collectionWork->isActive != 0 || BoardIsKill()) {
        W06CoinParticleUpdate(collectionWork);
        HuMemDirectFree(collectionWork->coinParticles);
        coinRewardObject = NULL;
        omDelObjEx(HuPrcCurrentGet(), collectionObject);
        return;
    }
    coinParticle = collectionWork->coinParticles;
    for (inactiveParticleCount = i = 0; i < 20; i++, coinParticle++) {
        if (coinParticle->isActive == 0) {
            inactiveParticleCount++;
        }
    }
    if (inactiveParticleCount == 20) {
        collectionWork->isActive = 1;
        return;
    }
    playerIndex = 0;
    for (i = 0; i < 20; i++) {
        coinParticle = &collectionWork->coinParticles[i];
        if (coinParticle->isActive == 1 && playerIndex == 0) {
            coinParticle->isActive = 2;
            playerIndex = 1;
        }
        if (coinParticle->isActive == 2) {
            coinParticle->animationFrames++;
            OSu8tof32(&coinParticle->animationFrames, &fallDistance);
            coinParticle->rotation += 45.0f;
            coinParticle->position.y += -0.08166667f * fallDistance * fallDistance * 0.75f;
            if (coinParticle->position.y < collectionWork->coinLandingY) {
                coinParticle->position.y = collectionWork->coinLandingY;
                CharEffectCoinGlowCreate(1, &coinParticle->position);
                BoardModelVisibilitySet(coinParticle->coinModelId, 0);
                HuAudFXPlay(7);
                BoardPlayerCoinsAdd(collectionWork->playerIndex, 1);
                omVibrate(collectionWork->playerIndex, 12, 6, 6);
                coinParticle->isActive = 0;
            }
            BoardModelPosSetV(coinParticle->coinModelId, &coinParticle->position);
            BoardModelRotYSet(coinParticle->coinModelId, coinParticle->rotation);
        }
    }
}

static void W06CoinRewardCreate(s32 playerIndex) {
    Vec playerPosition;
    float randomAngle;
    OMOBJ *itemObject;
    CoinCollectionWork *collectionWork;
    CoinCollectionState *coinParticle;
    s32 i;

    itemObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W06CoinCollectionObjectUpdate);
    coinRewardObject = itemObject;
    collectionWork = omObjGetWork(itemObject, CoinCollectionWork);
    collectionWork->isActive = 0;
    collectionWork->playerIndex = playerIndex;
    collectionWork->coinParticles = HuMemDirectMallocNum(HEAP_HEAP, 20 * sizeof(CoinCollectionState), HU_MEMNUM_OVL);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    collectionWork->coinLandingY = playerPosition.y + 80.0f;
    memset(collectionWork->coinParticles, 0, 20 * sizeof(CoinCollectionState));
    for (i = 0; i < coinRewardAmount; i++) {
        if (i == 0) {
            randomAngle = 0.0f;
        } else {
            randomAngle = BoardRandFloat();
        }
        coinParticle = &collectionWork->coinParticles[i];
        coinParticle->isActive = 1;
        coinParticle->position.x = playerPosition.x + 50.0 * sind(360.0f * randomAngle);
        coinParticle->position.z = playerPosition.z + 50.0 * cosd(360.0f * randomAngle);
        coinParticle->position.y = playerPosition.y + 600.0f + 500.0f * BoardRandFloat();
        coinParticle->animationFrames = 0;
        coinParticle->rotation = 360.0f * BoardRandFloat();
        coinParticle->coinModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 10), NULL, 1);
    }
}

static void W06CoinParticleUpdate(CoinCollectionWork *collectionWork) {
    CoinCollectionState *coinParticle;
    s32 i;

    for (i = 0; i < 20; i++) {
        coinParticle = &collectionWork->coinParticles[i];
        BoardModelKill(coinParticle->coinModelId);
    }
}
