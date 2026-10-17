#include "REL/w04Dll.h"
#include "REL/board_executor.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/gamework_data.h"
#include "game/object.h"
#include "game/process.h"
#include "game/window.h"
#include "game/board/boo_house.h"
#include "game/board/com.h"
#include "game/board/lottery.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/shop.h"
#include "game/board/space.h"
#include "game/board/star.h"
#include "game/board/window.h"

#include "dolphin.h"
#include "math.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

static void SetW04BoardLightInfo(void);
static void ResetW04BoardLightInfo(void);
static s32 RunW04BigBooEvent(void);
static s32 HandleW04BoardWalkEvent(void);
static void SetupW04Minigames(void);
static void ResetW04BoardEvent(void);
static void RunW04ItemShopEvent(void);
static void CleanupW04ItemShopEvent(void);
static void ShowW04ItemShop(void);
static void RunW04CoinShopEvent(void);
static s32 HandleW04BoardLandingEvent(void);

s16 w04BoardModelIds[MAPOBJ_MAX];

BoardMapObject w04BoardMapObjects[MAPOBJ_MAX] = {
    { {   750.0f,  0.0f,   1500.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 5) },
    { { -1500.0f,  0.0f,    150.0f }, { 0.0f,  89.999985f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 5) },
    { {  1200.0f,  0.0f,  -1650.0f }, { 0.0f, -89.999985f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 5) },
    { { -1950.0f,  0.0f,  -2100.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 4) },
    { {  2100.0f,  0.0f,  -2100.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 18) },
    { {  -150.0f,  0.0f,   -150.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 23) },
    { {  3000.0f,  0.0f,   -600.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 12) },
    { { -1500.0f,  0.0f,   -900.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 15) },
    { { -1950.0f,  0.0f,  -2000.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 34) },
    { { -1350.0f, 50.0f,   -900.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 34) },
    { {  2750.0f,  0.0f,   -600.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 34) },
    { {  -150.0f,  0.0f,   -100.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 29) },
    { {  2850.0f,  0.0f,  -1500.0f }, { 0.0f, -89.999985f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 17) },
    { { -3150.0f,  0.0f,    300.0f }, { 0.0f,  89.999985f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 17) },
    { {  2850.0f,  0.0f, 0.000003f }, { 0.0f, -89.999985f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 17) },
    { {   900.0f,  0.0f,   2250.0f }, { 0.0f,  180.00001f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W04, 17) }
};

static s32 *activeGuestModelResources;
static s16 itemShopChoices[4];
static HUPROCESS *itemShopProcess;
W04BoardEventState *w04BoardEventState;

static s16 boardLightModelId = -1;
static s16 boardBackdropModelId = -1;
static s16 starHostModelId = -1;
static s16 boardServiceHostModelId = -1;

static s32 boardServiceModelResources[] = {
    DATA_MAKE_NUM(DATADIR_W04, 8),
    -1
};

static s32 boardGuestModelResourceIds[] = {
    DATA_MAKE_NUM(DATADIR_BGUEST, 24),
    -1
};

void BoardCreate(void) {
    BoardMapObject *mapObject;
    s32 currentBoardId;
    s32 i;

    omDBGMenuButton = 1;
    currentBoardId = GWBoardGet();
    w04BoardEventState = (W04BoardEventState*) GWSystem.board_data;
    BoardSpaceInit(DATA_MAKE_NUM(DATADIR_W04, 0));
    boardLightModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 1), NULL, 0);
    Fn8006DDE8(boardLightModelId, -1.0f);
    BoardModelPosSet(boardLightModelId, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(boardLightModelId, 0, 0x40000001);
    BoardModelMotionSpeedSet(boardLightModelId, 1.0f);
    boardBackdropModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 2), NULL, 0);
    Fn8006DDE8(boardBackdropModelId, -1.0f);
    BoardModelPosSet(boardBackdropModelId, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(boardBackdropModelId, 0, 0x40000001);
    starHostModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 7), boardServiceModelResources, 0);
    BoardModelPosSet(starHostModelId, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(starHostModelId, 1, 0x40000001);
    boardServiceHostModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BGUEST, 18), boardGuestModelResourceIds, 0);
    BoardModelMotionStart(boardServiceHostModelId, 1, 0x40000001);
    BoardLightHookSet(SetW04BoardLightInfo, ResetW04BoardLightInfo);
    for (i = 0; i < 16; i++) {
        mapObject = &w04BoardMapObjects[i];
        if (mapObject->data_num != -1) {
            if (i == 8 || i == 9 || i == 10 || i == 11) {
                activeGuestModelResources = boardGuestModelResourceIds;
            } else {
                activeGuestModelResources = NULL;
            }
            w04BoardModelIds[i] = BoardModelCreate(mapObject->data_num, activeGuestModelResources, 0);
            BoardModelPosSetV(w04BoardModelIds[i], &mapObject->pos);
            BoardModelRotSetV(w04BoardModelIds[i], &mapObject->rot);
            BoardModelScaleSetV(w04BoardModelIds[i], &mapObject->scale);
            BoardModelVisibilitySet(w04BoardModelIds[i], 1);
            if (activeGuestModelResources != NULL) {
                BoardModelMotionStart(w04BoardModelIds[i], 1, 0x40000001);
            }
        }
    }
    BoardModelAlphaSet(w04BoardModelIds[8], 0);
    BoardModelVisibilitySet(w04BoardModelIds[8], 0);
    BoardModelMotionStartEndSet(w04BoardModelIds[5], 0, 60);
    BoardModelAttrSet(w04BoardModelIds[5], 0x40000001);
    BoardModelAttrSet(w04BoardModelIds[4], 0x40000001);
    SetupW04Minigames();
    W04BridgeSetup();
    W04BigBooSetup();
    BoardModelAttrSet(w04BoardModelIds[0], 0x40000001);
    BoardModelAttrSet(w04BoardModelIds[1], 0x40000001);
    BoardModelAttrSet(w04BoardModelIds[2], 0x40000001);
    BoardModelAttrSet(w04BoardModelIds[3], 0x40000001);
    BoardSpaceWalkEventFuncSet(HandleW04BoardWalkEvent);
    BoardSpaceWalkMiniEventFuncSet(HandleW04BoardLandingEvent);
    BoardSpaceLandEventFuncSet(RunW04BigBooEvent);
    BoardStarHostSet(starHostModelId);
    BoardBooHouseHostSet(boardServiceHostModelId);
    BoardShopHostSet(boardServiceHostModelId);
    BoardLotteryHostSet(boardServiceHostModelId);
    BoardJunctionMaskSet(0x100);
    BoardJunctionMaskSet(0x40);
}

void BoardDestroy(void) {
    s32 i;

    W04BridgeCleanup();
    for (i = 0; i < MAPOBJ_MAX; i++) {
        if (w04BoardModelIds[i] != 0) {
            BoardModelKill(w04BoardModelIds[i]);
            w04BoardModelIds[i] = 0;
        }
    }
    if (boardServiceHostModelId != -1) {
        BoardModelKill(boardServiceHostModelId);
        boardServiceHostModelId = -1;
    }
    if (starHostModelId != -1) {
        BoardModelKill(starHostModelId);
        starHostModelId = -1;
    }
    if (boardLightModelId != -1) {
        BoardModelKill(boardLightModelId);
        boardLightModelId = -1;
    }
    if (boardBackdropModelId != -1) {
        BoardModelKill(boardBackdropModelId);
        boardBackdropModelId = -1;
    }
    BoardSpaceDestroy();
}

static void SetW04BoardLightInfo(void) {
    s16 lightModelId;

    lightModelId = BoardModelIDGet(boardLightModelId);
    Hu3DModelLightInfoSet(lightModelId, 1);
}

static void ResetW04BoardLightInfo(void) {
}

static s32 RunW04BigBooEvent(void) {
    s32 playerIndex;
    s32 currentSpace;
    s32 spaceFlags;

    playerIndex = GWSystem.player_curr;
    currentSpace = GWPlayer[playerIndex].space_curr;
    spaceFlags = BoardSpaceFlagGet(0, currentSpace);
    W04BigBooEvent();
}

static s32 HandleW04BoardWalkEvent(void) {
    s32 currentPlayer;
    BoardSpace *boardSpace;

    currentPlayer = GWSystem.player_curr;
    if (BoardPlayerSizeGet(currentPlayer) == 2) {
        return 0;
    }
    if (GWPlayer[currentPlayer].bowser_suit) {
        return 0;
    }
    boardSpace = BoardSpaceGet(0, GWPlayer[currentPlayer].space_curr);
    if (boardSpace->flag & 0x80) {
        W04BridgeWalkEvent();
        return 1;
    }
    if (boardSpace->flag & 0x100) {
        if (w04BoardEventState->bridgeState & 1) {
            W04BooEvent();
        } else {
            BoardRollDispSet(0);
            BoardPlayerIdleSet(currentPlayer);
            BoardWinCreate(0, MAKE_MESSID(32, 25), -1);
            BoardWinWait();
            BoardWinKill();
            BoardRollDispSet(1);
        }
        return 1;
    }
    return 0;
}

static void SetupW04Minigames(void) {
    W04ItemSetup();
    W04CoinSetup();
}

static void ResetW04BoardEvent(void) {
}

static void RunW04ItemShopEvent(void) {
    Vec playerPosition;
    u32 shopChoice;
    s32 destinationSpace;
    s32 currentPlayer;
    s32 nextSpace;
    s32 currentSpace;
    s32 duplicateChoiceIndex;

    currentPlayer = GWSystem.player_curr;
    if (BoardPlayerItemCount(currentPlayer) == 3) {
        BoardWinCreate(2, MAKE_MESSID(32, 12), 3);
        BoardWinWait();
        BoardWinKill();
        HuPrcEnd();
    }
    itemShopChoices[0] = 0;
    currentSpace = 1;
    while (currentSpace < 3) {
        itemShopChoices[currentSpace] = frandmod(14);
        if (itemShopChoices[currentSpace] != 10 && itemShopChoices[currentSpace] != 13) {
            for (duplicateChoiceIndex = 0; duplicateChoiceIndex < currentSpace; duplicateChoiceIndex++) {
                if (itemShopChoices[currentSpace] == itemShopChoices[duplicateChoiceIndex]) {
                    itemShopChoices[currentSpace] = -1;
                }
            }
            if (itemShopChoices[currentSpace] != -1) {
                currentSpace++;
            }
        }
    }
    W04ItemModelsSpawn(itemShopChoices);
    BoardWinCreateChoice(2, MAKE_MESSID(32, 13), 3, 0);
    if (GWPlayer[currentPlayer].com) {
        if (BoardComPreferItemCheck(currentPlayer, itemShopChoices[0], itemShopChoices[1], itemShopChoices[2]) != -1) {
            BoardComKeySetLeft();
        } else {
            BoardComKeySetRight();
        }
    }
    BoardWinWait();
    BoardWinKill();
    shopChoice = BoardWinChoiceGet();
    if (shopChoice == 1 || shopChoice == -1) {
        BoardWinCreate(2, MAKE_MESSID(32, 14), 3);
        BoardWinWait();
        BoardWinKill();
        BoardMGExit();
        W04ItemModelsShrink();
        while (BoardMGDoneFlagGet() != 1) {
            HuPrcVSleep();
        }
        BoardMGDoneFlagSet(0);
        W04ItemModelsCleanup();
        HuPrcEnd();
    }
    destinationSpace = GWPlayer[currentPlayer].space_curr;
    nextSpace = BoardSpaceLinkFlagSearch(0, destinationSpace, 0x02000000);
    BoardPlayerMoveBetween(currentPlayer, destinationSpace, nextSpace);
    while (GWPlayer[currentPlayer].moving) {
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(currentPlayer, 1, 0x40000001);
    BoardPlayerPosGet(currentPlayer, &playerPosition);
    playerPosition.y += 5.0f;
    BoardPlayerPosSetV(currentPlayer, &playerPosition);
    BoardCameraViewSet(2);
    BoardPlayerMotBlendSet(currentPlayer, 180, 15);
    while (!BoardPlayerMotBlendCheck(currentPlayer)) {
        HuPrcVSleep();
    }
    BoardCameraMotionWait();
    W04ItemAcquisitionEvent();
    while (BoardEventFlagGet() != 0) {
        HuPrcVSleep();
    }
    while (GWPlayer[currentPlayer].moving) {
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(currentPlayer, 1, 0x40000001);
    HuPrcEnd();
}

static void CleanupW04ItemShopEvent(void) {
    BoardWinKill();
    CharModelDataClose(-1);
    itemShopProcess = NULL;
}

static void ShowW04ItemShop(void) {
    itemShopProcess = HuPrcChildCreate(RunW04ItemShopEvent, 0x2003, 0x2000, 0, boardMainProc);
    HuPrcDestructorSet2(itemShopProcess, CleanupW04ItemShopEvent);
    while (itemShopProcess) {
        HuPrcVSleep();
    }
}

static void RunW04CoinShopEvent(void) {
    Vec cameraStartPosition;
    Vec cameraTargetPosition;
    Vec destinationSpacePosition;
    s32 currentPlayer;
    s32 choice;
    s32 currentSpace;
    s32 destinationSpace;

    currentPlayer = GWSystem.player_curr;
    BoardWinCreateChoice(2, MAKE_MESSID(32, 17), 3, 0);
    if (GWPlayer[currentPlayer].com) {
        if (frand() & 1) {
            BoardComKeySetLeft();
        } else {
            BoardComKeySetRight();
        }
    }
    BoardWinWait();
    BoardWinKill();
    choice = BoardWinChoiceGet();
    if (choice == 1 || choice == -1) {
        BoardWinCreate(2, MAKE_MESSID(32, 18), 3);
        BoardWinWait();
        BoardWinKill();
        BoardMGExit();
        while (BoardMGDoneFlagGet() != 1) {
            HuPrcVSleep();
        }
        BoardMGDoneFlagSet(0);
        return;
    }
    cameraStartPosition.x = -45.0f;
    cameraStartPosition.y = cameraStartPosition.z = 0.0f;
    cameraTargetPosition.y = 50.0f;
    cameraTargetPosition.x = cameraTargetPosition.z = 0.0f;
    BoardCameraMotionStartEx(w04BoardModelIds[7], &cameraStartPosition, &cameraTargetPosition, 1200.0f, 25.0f, 21);
    currentSpace = GWPlayer[currentPlayer].space_curr;
    destinationSpace = BoardSpaceLinkFlagSearch(0, currentSpace, 0x02000000);
    BoardSpacePosGet(0, destinationSpace, &destinationSpacePosition);
    BoardPlayerMoveBetween(currentPlayer, currentSpace, destinationSpace);
    while (GWPlayer[currentPlayer].moving) {
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(currentPlayer, 1, 0x40000001);
    W04CoinGameEvent();
    while (GWPlayer[currentPlayer].moving) {
        HuPrcVSleep();
    }
    while (BoardEventFlagGet() != 0) {
        HuPrcVSleep();
    }
    while (GWPlayer[currentPlayer].moving) {
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(currentPlayer, 1, 0x40000001);
}

static s32 HandleW04BoardLandingEvent(void) {
    s16 currentSpace;
    u32 spaceEventFlags;

    currentSpace = GWPlayer[GWSystem.player_curr].space_curr;
    spaceEventFlags = BoardSpaceFlagGet(0, currentSpace) & 0x600000;
    BoardRollDispSet(0);
    if (spaceEventFlags & 0x200000) {
        itemShopProcess = HuPrcChildCreate(RunW04ItemShopEvent, 0x2003, 0x2000, 0, boardMainProc);
        HuPrcDestructorSet2(itemShopProcess, CleanupW04ItemShopEvent);
        while (itemShopProcess) {
            HuPrcVSleep();
        }
    } else if (spaceEventFlags & 0x400000) {
        RunW04CoinShopEvent();
    }
    BoardRollDispSet(1);
}

s32 W04ModelRotateYToAngle(s16 modelId, float targetAngle, float rotationStep) {
    float angleDifference;
    float currentAngle;
    s32 reachedTargetAngle;

    reachedTargetAngle = 0;
    currentAngle = BoardModelRotYGet(modelId);
    targetAngle = fmod(targetAngle, 360.0);
    if (currentAngle < 0.0f) {
        currentAngle += 360.0f;
    }
    if (targetAngle < 0.0f) {
        targetAngle += 360.0f;
    }
    if (currentAngle != targetAngle) {
        angleDifference = targetAngle - currentAngle;
        if (angleDifference < 0.0f) {
            angleDifference += 360.0f;
        }
        if (angleDifference < 180.0f) {
            if (angleDifference > rotationStep) {
                currentAngle += rotationStep;
            } else {
                currentAngle = targetAngle;
            }
        } else if (360.0f - angleDifference > rotationStep) {
            currentAngle -= rotationStep;
        } else {
            currentAngle = targetAngle;
        }
        if (currentAngle < 0.0f) {
            currentAngle += 360.0f;
        }
        if (currentAngle >= 360.0f) {
            currentAngle -= 360.0f;
        }
        BoardModelRotYSet(modelId, currentAngle);
    } else {
        reachedTargetAngle = 1;
    }
    return reachedTargetAngle;
}
