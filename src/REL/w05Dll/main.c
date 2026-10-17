#include "REL/w05Dll.h"

#include "game/chrman.h"
#include "game/frand.h"
#include "game/gamework_data.h"


#include "game/board/audio.h"
#include "game/board/boo_house.h"
#include "game/board/com.h"
#include "game/board/lottery.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/shop.h"
#include "game/board/lottery.h"
#include "game/board/star.h"
#include "game/board/window.h"

// data
BoardMapObject boardMapObjects[7] = {
    { { 3300.0f, 100.0f, -900.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W05, 0x11) },
    { { -2550.0f, 0.0f, 1350.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W05, 0x14) },
    { { -3000.0f, 100.0f, -1050.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W05, 0x1A) },
    { { 2100.0f, 100.0f, -450.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W05, 0x1B) },
    { { -450.0f, 100.0f, -750.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W05, 0x0A) },
    { { -2710.0f, 0.0f, 1315.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W05, 0x0A) },
    { { 3055.0f, 100.0f, -815.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W05, 0x0A) },
};
s16 mainBoardModel = 0xFFFF;
s16 secondaryBoardModel = 0xFFFF;
s16 starHostModel = 0xFFFF;
s16 eventHostModel = 0xFFFF;
s16 upperBoardModel = 0xFFFF;
s32 characterIdleMotions[8] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00),
};
s32 starModelMotionList[2] = { DATA_MAKE_NUM(DATADIR_W05, 0x06), DATA_NUM_LISTEND };
s32 boardModelMotionListEnd = 0xFFFFFFFF;

// bss
s16 boardMapModelIds[7];
s32 *boardMotionList;
s16 itemChoiceIds[4];
HUPROCESS *boardEventProcess;
HotelBoardState *hotelBoardState;

void BoardCreate(void)
{
    s32 spaceIndex;
    s32 loopIndex;
    BoardMapObject *mapObject;

    spaceIndex = GWBoardGet();
    hotelBoardState = (HotelBoardState *)GWSystem.board_data;
    BoardSpaceInit(DATA_MAKE_NUM(DATADIR_W05, 0x00));
    mainBoardModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x01), NULL, 0);
    upperBoardModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x02), NULL, 0);
    Fn8006DDE8(mainBoardModel, -1.0f);
    BoardModelPosSet(mainBoardModel, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(mainBoardModel, 0, 0x40000001);
    BoardModelMotionSpeedSet(mainBoardModel, 1.0f);
    Fn8006DDE8(upperBoardModel, -1.0f);
    BoardModelPosSet(upperBoardModel, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(upperBoardModel, 0, 0x40000001);
    BoardModelMotionSpeedSet(upperBoardModel, 1.0f);
    BoardModelLayerSet(upperBoardModel, 1);
    secondaryBoardModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x03), NULL, 0);
    Fn8006DDE8(secondaryBoardModel, -1.0f);
    BoardModelPosSet(secondaryBoardModel, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(secondaryBoardModel, 0, 0x40000001);
    starHostModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x05), starModelMotionList, 0);
    BoardModelPosSet(starHostModel, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(starHostModel, 1, 0x40000001);
    eventHostModel = BoardModelCreate(0x2001C, &boardModelMotionListEnd, 0);
    BoardModelMotionStart(eventHostModel, 1, 0x40000001);
    BoardLightHookSet(&W05SetBoardModelLighting, &W05BoardLightingNoop);

    for (loopIndex = 0; loopIndex < 7; loopIndex++) {
        mapObject = &boardMapObjects[loopIndex];
        if (mapObject->data_num != -1) {
            if ((loopIndex == 4) || (loopIndex == 5) || (loopIndex == 6)) {
                boardMotionList = &boardModelMotionListEnd;
            }
            else {
                boardMotionList = NULL;
            }
            boardMapModelIds[loopIndex] = BoardModelCreate(mapObject->data_num, boardMotionList, 0);
            BoardModelPosSetV(boardMapModelIds[loopIndex], &mapObject->pos);
            BoardModelRotSetV(boardMapModelIds[loopIndex], &mapObject->rot);
            BoardModelScaleSetV(boardMapModelIds[loopIndex], &mapObject->scale);
            BoardModelVisibilitySet(boardMapModelIds[loopIndex], 1);
            if (boardMotionList != NULL) {
                BoardModelMotionStart(boardMapModelIds[loopIndex], 0, 0x40000001);
            }
        }
    }
    W05InitializeBoardMinigames();
    W05HotelCreateLevelModel();
    W05CreateMonkeyModels();
    BoardSpaceWalkEventFuncSet(&W05HandleBoardWalkEvent);
    BoardSpaceWalkMiniEventFuncSet(&W05HandleBoardMiniEventSpace);
    BoardSpaceLandEventFuncSet(&W05HandleBoardLandingEvent);
    BoardStarHostSet(starHostModel);
    BoardBooHouseHostSet(eventHostModel);
    BoardShopHostSet(eventHostModel);
    BoardLotteryHostSet(eventHostModel);
}

void BoardDestroy(void)
{
    s32 loopIndex;

    for (loopIndex = 0; loopIndex < 7; loopIndex++) {
        if (boardMapModelIds[loopIndex] != 0) {
            BoardModelKill(boardMapModelIds[loopIndex]);
            boardMapModelIds[loopIndex] = 0;
        }
    }
    if (eventHostModel != -1) {
        BoardModelKill(eventHostModel);
        eventHostModel = -1;
    }
    if (starHostModel != -1) {
        BoardModelKill(starHostModel);
        starHostModel = -1;
    }
    if (mainBoardModel != -1) {
        BoardModelKill(mainBoardModel);
        mainBoardModel = -1;
    }
    if (upperBoardModel != -1) {
        BoardModelKill(upperBoardModel);
        upperBoardModel = -1;
    }
    if (secondaryBoardModel != -1) {
        BoardModelKill(secondaryBoardModel);
        secondaryBoardModel = -1;
    }
    BoardSpaceDestroy();
}

void W05SetBoardModelLighting(void)
{
    s16 modelId = BoardModelIDGet(mainBoardModel);
    Hu3DModelLightInfoSet(modelId, 1);
}

void W05BoardLightingNoop(void) { }

s32 W05HandleBoardLandingEvent(void)
{
    s32 playerIndex = GWSystem.player_curr;
    s32 playerIndex2 = GWPlayer[playerIndex].space_curr;
    s32 spaceIndex;
    u32 spaceIndex2;

    spaceIndex2 = BoardSpaceFlagGet(0, playerIndex2);
    if ((spaceIndex2 & 2) != 0) {
        W05HotelUpgrade();
        return;
    }
    if ((spaceIndex2 & 0x40) != 0) {
        spaceIndex = (spaceIndex2 & 0x38) >> 3;
        W05RunDolphinBoardEvent(spaceIndex);
    }
}

s32 W05HandleBoardWalkEvent(void)
{
    s32 playerIndex;
    BoardSpace *space;

    playerIndex = GWSystem.player_curr;
    space = BoardSpaceGet(0, GWPlayer[playerIndex].space_curr);
    if ((space->flag & 1) != 0) {
        W05RunMonkeyBoardEvent(0);
        return 1;
    }
    if ((space->flag & 0x800) != 0) {
        W05RunMonkeyBoardEvent(1);
        return 1;
    }
    if (BoardPlayerSizeGet(playerIndex) == 2) {
        return 0;
    }
    if (GWPlayer[playerIndex].bowser_suit != 0) {
        return 0;
    }
    if ((space->flag & 4) != 0) {
        W05HotelCollectToll();
        return 1;
    }
    return 0;
}

void W05InitializeBoardMinigames(void)
{
    W05InitializeItemMinigame();
    W05PrepareCoinBoardModel();
}

void W05BoardEventNoop(void) { }

void W05RunItemSpaceEvent(void)
{
    Vec vector;
    Vec vector2;
    s32 spaceIndex;
    u32 spaceIndex2;
    s32 playerIndex;
    s32 loopIndex;
    s32 loopIndex2;
    s32 playerIndex2;

    playerIndex2 = GWSystem.player_curr;
    if (BoardPlayerItemCount(playerIndex2) == 3) {
        BoardWinCreate(2, MAKE_MESSID(39, 9), 7);
        BoardWinWait();
        BoardWinKill();
        HuPrcEnd();
    }
    itemChoiceIds[0] = 0;

    for (loopIndex2 = 1; loopIndex2 < 3;) {
        itemChoiceIds[loopIndex2] = (u32)frandmod(0xE);
        if ((itemChoiceIds[loopIndex2] == 0xA) || (itemChoiceIds[loopIndex2] == 0xD)) {
            continue;
        }
        for (loopIndex = 0; loopIndex < loopIndex2; loopIndex++) {
            if (itemChoiceIds[loopIndex2] == itemChoiceIds[loopIndex]) {
                itemChoiceIds[loopIndex2] = -1;
            }
        }
        if (itemChoiceIds[loopIndex2] == -1) {
            continue;
        }
        loopIndex2++;
    }
    W05ShuffleItemChoices(itemChoiceIds);
    BoardWinCreateChoice(2, MAKE_MESSID(39, 10), 7, 0);
    if (GWPlayer[playerIndex2].com != 0) {
        if (BoardComPreferItemCheck(playerIndex2, itemChoiceIds[0], itemChoiceIds[1], itemChoiceIds[2]) != -1) {
            BoardComKeySetLeft();
        }
        else {
            BoardComKeySetRight();
        }
    }
    BoardWinWait();
    BoardWinKill();
    spaceIndex2 = BoardWinChoiceGet();
    if (spaceIndex2 == 1 || spaceIndex2 == -1) {
        while (((u32)W05GetItemMinigameInput() & 0x10) == 0) {
            HuPrcVSleep();
        }
        BoardWinCreate(2, MAKE_MESSID(39, 11), 7);
        BoardWinWait();
        BoardWinKill();
        BoardMGExit();
        W05SetItemMinigameInputState();
        W05PlayItemSpaceExitAnimation();
        W05CleanupItemMinigame();
        while (BoardMGDoneFlagGet() != 1) {
            HuPrcVSleep();
        }
        BoardMGDoneFlagSet(0);
        HuPrcEnd();
    }
    vector.x = -30.0f;
    vector.y = vector.z = 0.0f;
    vector2.x = 0.0f;
    vector2.y = 0.0f;
    vector2.z = 0.0f;
    BoardCameraMotionStartEx(boardMapModelIds[0], &vector, &vector2, 1250.0f, 25.0f, 0x15);

    while (((u32)W05GetItemMinigameInput() & 0x10) == 0) {
        HuPrcVSleep();
    }
    playerIndex = GWPlayer[playerIndex2].space_curr;
    spaceIndex = BoardSpaceLinkFlagSearch(0, playerIndex, 0x02000000);
    BoardPlayerMoveBetween(playerIndex2, playerIndex, spaceIndex);

    while (GWPlayer[playerIndex2].moving != 0) {
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(playerIndex2, 1, 0x40000001);
    W05StartItemMinigame();

    while (BoardEventFlagGet() != 0) {
        HuPrcVSleep();
    }
    while ((GWPlayer[playerIndex2].moving) != 0) {
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(playerIndex2, 1, 0x40000001);
    HuPrcEnd();
}

void W05CleanupItemSpaceEvent(void)
{
    BoardWinKill();
    CharModelDataClose(-1);
    boardEventProcess = NULL;
}

void W05StartItemSpaceEvent(void)
{
    boardEventProcess = HuPrcChildCreate(W05RunItemSpaceEvent, 0x2003U, 0x3000U, 0, boardMainProc);
    HuPrcDestructorSet2(boardEventProcess, W05CleanupItemSpaceEvent);

    while (boardEventProcess != 0) {
        HuPrcVSleep();
    }
}

void W05StartBoardMusicAfterDelay(void)
{
    HuPrcSleep(0x3C);
    BoardMusStart(1, 0x23, 0x7F, 0);
    HuPrcEnd();
}

void W05RunCoinSpaceEvent(void)
{
    Vec vector;
    Vec vector2;
    Vec vector3;
    s32 spaceIndex;
    s32 playerIndex;
    s32 localinteger32;
    s32 playerIndex2;

    playerIndex2 = GWSystem.player_curr;
    W05AnimateCoinBoardModelIntro();
    BoardWinCreateChoice(2, MAKE_MESSID(39, 15), 7, 0);
    if (GWPlayer[playerIndex2].com != 0) {
        if (frandBool() != 0) {
            BoardComKeySetLeft();
        }
        else {
            BoardComKeySetRight();
        }
    }
    BoardWinWait();
    BoardWinKill();
    spaceIndex = BoardWinChoiceGet();
    if ((spaceIndex == 1) || (spaceIndex == -1)) {
        W05AnimateCoinBoardModelOutro();
        BoardWinCreate(2, MAKE_MESSID(39, 16), 7);
        BoardWinWait();
        BoardWinKill();
        BoardMGExit();

        while (BoardMGDoneFlagGet() != 1) {
            HuPrcVSleep();
        }
        BoardMGDoneFlagSet(0);
        return;
    }
    BoardAudSeqPause(0, 1, 0x3E8);
    HuPrcChildCreate(W05StartBoardMusicAfterDelay, 0x2003, 0x1000, 0, boardMainProc);
    vector.x = -35.0f;
    vector.y = vector.z = 0.0f;
    vector2.x = 0.0f;
    vector2.y = 0.0f;
    vector2.z = -250.0f;
    BoardCameraMotionStartEx(boardMapModelIds[1], &vector, &vector2, 1550.0f, 25.0f, 0x15);
    playerIndex = GWPlayer[playerIndex2].space_curr;
    localinteger32 = BoardSpaceLinkFlagSearch(0, playerIndex, 0x02000000);
    BoardSpacePosGet(0, localinteger32, &vector3);
    BoardPlayerMoveBetween(playerIndex2, playerIndex, localinteger32);
    while (GWPlayer[playerIndex2].moving != 0) {
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(playerIndex2, 1, 0x40000001);
    W05StartCoinMinigame();
    while (GWPlayer[playerIndex2].moving != 0) {
        HuPrcVSleep();
    }
    while (BoardEventFlagGet() != 0) {
        HuPrcVSleep();
    }
    while (GWPlayer[playerIndex2].moving != 0) {
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(playerIndex2, 1, 0x40000001);
}

s32 W05HandleBoardMiniEventSpace(void)
{
    u32 spaceIndex;
    s16 playerIndex = GWPlayer[GWSystem.player_curr].space_curr;

    spaceIndex = BoardSpaceFlagGet(0, playerIndex) & 0x600000;
    BoardRollDispSet(0);
    if ((spaceIndex & 0x200000) != 0) {
        boardEventProcess = HuPrcChildCreate(W05RunItemSpaceEvent, 0x2003, 0x3000, 0, boardMainProc);
        HuPrcDestructorSet2(boardEventProcess, W05CleanupItemSpaceEvent);

        while (boardEventProcess != 0) {
            HuPrcVSleep();
        }
    }
    else if ((spaceIndex & 0x400000) != 0) {
        W05RunCoinSpaceEvent();
    }
    BoardRollDispSet(1);
}

s32 W05TurnBoardModelTowardAngle(s16 modelId, F32 targetAngle, F32 maxTurnStep)
{
    F32 frameCount;
    F32 frameCount2;
    s32 integerValue;

    integerValue = 0;
    frameCount2 = BoardModelRotYGet(modelId);
    targetAngle = fmod(targetAngle, 360.0);
    if (frameCount2 < 0.0f) {
        frameCount2 += 360.0f;
    }
    if (targetAngle < 0.0f) {
        targetAngle += 360.0f;
    }
    if (frameCount2 != targetAngle) {
        frameCount = targetAngle - frameCount2;
        if (frameCount < 0.0f) {
            frameCount += 360.0f;
        }
        if (frameCount < 180.0f) {
            if (frameCount > maxTurnStep) {
                frameCount2 = frameCount2 + maxTurnStep;
            }
            else {
                frameCount2 = targetAngle;
            }
        }
        else if ((360.0f - frameCount) > maxTurnStep) {
            frameCount2 = frameCount2 - maxTurnStep;
        }
        else {
            frameCount2 = targetAngle;
        }
        if (frameCount2 < 0.0f) {
            frameCount2 += 360.0f;
        }
        if (frameCount2 >= 360.0f) {
            frameCount2 -= 360.0f;
        }
        BoardModelRotYSet(modelId, frameCount2);
    }
    else {
        integerValue = 1;
    }
    return integerValue;
}
