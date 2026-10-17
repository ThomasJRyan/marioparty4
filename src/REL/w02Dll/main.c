#include "REL/w02Dll.h"
#ifndef __MWERKS__
#include "game/audio.h"
#endif
#include "game/frand.h"
#ifndef __MWERKS__
#include "game/board/boo_house.h"
#include "game/board/lottery.h"
#include "game/board/shop.h"
#include "game/board/star.h"
#endif
#include "game/board/player.h"

// // Temporary defines
// #define BOARD_ITEM_MINI 0x0
// #define BOARD_ITEM_MEGA 0x1
// #define BOARD_ITEM_SUPERMINI 0x2
// #define BOARD_ITEM_SUPERMEGA 0x3
// #define BOARD_ITEM_HAMMER 0x4
// #define BOARD_ITEM_PIPE 0x5
// #define BOARD_ITEM_CARD 0x6
// #define BOARD_ITEM_SPARKY 0x7
// #define BOARD_ITEM_GADDLIGHT 0x8
// #define BOARD_ITEM_CHOMPCALL 0x9
// #define BOARD_ITEM_SUIT 0xA
// #define BOARD_ITEM_BOO 0xB
// #define BOARD_ITEM_LAMP 0xC
// #define BOARD_ITEM_BAG 0xD
// #define BOARD_ITEM_MAX 0xE
// #define BOARD_ITEM_NONE -1

/* BSS */
s16 boardModelIds[0x10]; // Model List
s16 rouletteGambleMarkerModelId;
s16 shuffleboardGambleMarkerModelId;
Vec shuffleboardNorthModelPosition;
Vec shuffleboardSouthModelPosition;
s32 *boardModelMotionData;
s16 itemChoices[ARRAY_COUNT(GWPlayer->items)]; // Item List
HUPROCESS *itemEventProcess;
u8 *boardData;

/* DATA */
static BoardMapObject boardMapObjects[0x10]
    = { { { 2550.0f, 0.0f, -450.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W02, 0x000C) },
          { { -3150.0f, 0.0f, -1350.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W02, 0x000D) },
          { { -3150.0f, 0.0f, 1650.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W02, 0x000E) },
          { { -150.0f, 0.0f, 900.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W02, 0x0008) },
          { { -150.0f, 0.0f, 900.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W02, 0x0009) },
          { { -150.0f, 0.0f, 900.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W02, 0x000A) },
          { { -150.0f, 0.0f, 900.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W02, 0x000B) },
          { { 1350.0f, 0.0f, -300.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W02, 0x0011) },
          { { 1350.0f, 0.0f, -300.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W02, 0x0012) },
          { { 1350.0f, 0.0f, -300.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W02, 0x0015) },
          { { -300.0f, 0.0f, -900.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W02, 0x0016) },
          { { 1175.0f, 0.0f, -125.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_BGUEST, 0x0005) },
          { { -525.0f, 0.0f, -825.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_BGUEST, 0x0005) },
          { { -2225.0f, 0.0f, -2100.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_BGUEST, 0x0005) },
          { { 1925.0f, 0.0f, -2100.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_BGUEST, 0x0005) },
          { { 150.0f, 0.0f, 1650.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_BGUEST, 0x0005) } };

// Special Models
s16 backdropModelId = -1;
s16 cloudModelId = -1;
s16 starHostModelId = -1;
s16 goombaHostModelId = -1;

s32 backdropMotionData[] = { DATA_MAKE_NUM(DATADIR_W02, 0x0004), DATA_NUM_LISTEND };
s32 boardGuestMotionData[] = {
    DATA_MAKE_NUM(DATADIR_BGUEST, 0x0009), DATA_MAKE_NUM(DATADIR_BGUEST, 0x0006), DATA_MAKE_NUM(DATADIR_BGUEST, 0x0007),
    DATA_MAKE_NUM(DATADIR_BGUEST, 0x0008), DATA_NUM_LISTEND,
    0x00000000 // Padding
};

/* Functions */
// Actual externs
extern void BoardMusStart(s32, s32, s32, s32);
extern void BoardAudSeqPause(s32, s32, s32);
s8 BoardComPreferItemCheck(s32, s8, s8, s8);
extern void BoardComKeySetLeft();
extern void BoardComKeySetRight();
extern void CharModelDataClose(s16);

// function is probably global. only inlined in rels?
inline s32 GetCurrentBoard(void)
{
    return GWSystem.board;
}

void BoardCreate(void)
{
    s32 i, boardData;
    BoardMapObject *modelTransform;

    boardData = GetCurrentBoard();
    boardData = GWSystem.board_data;
    BoardSpaceInit(DATA_MAKE_NUM(DATADIR_W02, 0x00));
    backdropModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W02, 0x01), NULL, 0);
    Fn8006DDE8(backdropModelId, -1.0f);
    BoardModelPosSet(backdropModelId, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(backdropModelId, 0, 0x40000001);
    BoardModelMotionSpeedSet(backdropModelId, 0.1f);
    cloudModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W02, 0x02), NULL, 0);
    Fn8006DDE8(cloudModelId, -1.0f);
    BoardModelPosSet(cloudModelId, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(cloudModelId, 0, 0x40000001);
    starHostModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W02, 0x03), backdropMotionData, 0);
    BoardModelPosSet(starHostModelId, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(starHostModelId, 1, 0x40000001);
    goombaHostModelId = BoardModelCreate(0x20005, boardGuestMotionData, 0);
    BoardModelMotionStart(goombaHostModelId, 1, 0x40000001);
    BoardLightHookSet(BoardAmbientLightEnable, BoardAmbientLightDisable);

    for (i = 0; i < 0x10; i++) {
        modelTransform = &boardMapObjects[i];
        if (modelTransform->data_num != -1) {
            if ((i == 0xB) || (i == 0xC) || (i == 0xD) || (i == 0xE) || (i == 0xF)) {
                boardModelMotionData = boardGuestMotionData;
            }
            else {
                boardModelMotionData = NULL;
            }
            boardModelIds[i] = BoardModelCreate(modelTransform->data_num, boardModelMotionData, 0);
            BoardModelPosSetV(boardModelIds[i], &modelTransform->pos);
            BoardModelRotSetV(boardModelIds[i], &modelTransform->rot);
            BoardModelScaleSetV(boardModelIds[i], &modelTransform->scale);
            BoardModelVisibilitySet(boardModelIds[i], 1);
            if (boardModelMotionData != NULL) {
                BoardModelMotionStart(boardModelIds[i], 1, 0x40000001);
            }
        }
    }

    BoardModelAttrSet(boardModelIds[0], 0x40000002);
    BoardModelAttrSet(boardModelIds[1], 0x40000002);
    BoardModelAttrSet(boardModelIds[2], 0x40000002);
    RouletteModelsInitialize();
    BoardMinigameAssetsCreate();
    BoardModelPosGet(boardModelIds[13], &shuffleboardNorthModelPosition);
    BoardModelPosGet(boardModelIds[14], &shuffleboardSouthModelPosition);
    rouletteGambleMarkerModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W02, 0x10), NULL, 1);
    shuffleboardGambleMarkerModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W02, 0x10), NULL, 1);
    ShuffleboardModelsPosition();
    BoardSpaceWalkEventFuncSet(BoardSpaceEventExecute);
    BoardSpaceWalkMiniEventFuncSet(BoardMinigameEventExecute);
    BoardSpaceLandEventFuncSet(BoardSpaceCounterDecrement);
    BoardStarHostSet(starHostModelId);
    BoardBooHouseHostSet(goombaHostModelId);
    BoardLotteryHostSet(goombaHostModelId);
    BoardShopHostSet(goombaHostModelId);
    BoardJunctionMaskSet(0xE);
    BoardJunctionMaskSet(0x30);
}

void BoardDestroy(void)
{
    /* Kill Model List */
    s32 i;
    for (i = 0; i < 0x10; i++) {
        if (boardModelIds[i] != 0) {
            BoardModelKill(boardModelIds[i]);
            boardModelIds[i] = 0;
        }
    }

    /* Kill Special Models */
    if (goombaHostModelId != -1) {
        BoardModelKill(goombaHostModelId);
        goombaHostModelId = -1;
    }
    if (starHostModelId != -1) {
        BoardModelKill(starHostModelId);
        starHostModelId = -1;
    }
    if (backdropModelId != -1) {
        BoardModelKill(backdropModelId);
        backdropModelId = -1;
    }
    if (cloudModelId != -1) {
        BoardModelKill(cloudModelId);
        cloudModelId = -1;
    }
    BoardSpaceDestroy();
}

void BoardAmbientLightEnable(void)
{
    s16 backdropModelObjectId = BoardModelIDGet(backdropModelId);
    Hu3DModelLightInfoSet(backdropModelObjectId, 1);
}

void BoardAmbientLightDisable(void) { }

/* Decrement current space's [UNKNOWN] flag */
s32 BoardSpaceCounterDecrement(void)
{
    u32 remainingShuffleboardSpaces;
    u32 currPlayer;
    u32 currSpace;
    u32 spaceFlags;
    currPlayer = GWSystem.player_curr;
    currSpace = GWPlayer[currPlayer].space_curr;
    spaceFlags = BoardSpaceFlagGet(0, currSpace);
    // temp_r31 = (temp_r)
    remainingShuffleboardSpaces = (spaceFlags & 0x1C0) >> 6;
    if (remainingShuffleboardSpaces != 0) {
        remainingShuffleboardSpaces--;
        ShuffleboardEventStart(remainingShuffleboardSpaces);
    }
}

/* Check if can iteract with board object? */
s32 BoardSpaceEventExecute(void)
{
    BoardSpace *currSpace;
    s32 currPlayer;
    s32 gambleSpaceIndex;

    currPlayer = GWSystem.player_curr;
    currSpace = BoardSpaceGet(0, GWPlayer[currPlayer].space_curr);
    if ((currSpace->flag & 0x30) != 0) {

        // If mega mushroom state return
        if (BoardPlayerSizeGet(currPlayer) == 2) {
            return 0;
        }
        // If using the bowser suit return
        if (GWPlayer[currPlayer].bowser_suit) {
            return 0;
        }

        if ((currSpace->flag & 0x20) != 0 && (currSpace->flag & 0x10) != 0) {
            gambleSpaceIndex = 2; // Southwest gamble
        }
        else if ((currSpace->flag & 0x20) != 0) {
            gambleSpaceIndex = 1; // Northwest gamble
        }
        else {
            gambleSpaceIndex = 0; // Northeast gamble
        }
        GambleMain(gambleSpaceIndex);
        return 1;
    }
    if ((currSpace->flag & 1) != 0) {
        RouletteEventStart();
        return 1;
    }
    return 0;
}

void BoardMinigameAssetsCreate(void)
{
    CoinMinigameModelsInitialize();
    ItemMinigameModelsInitialize();
}

void BoardEventCleanup(void) { }

void BoardMinigameMusicStart(void)
{
    HuPrcSleep(0x3C);
    BoardMusStart(1, 0x12, 0x7F, 0);
    HuPrcEnd();
}

void BoardItemSpaceEvent(void)
{
    Vec rotation;
    Vec offset;
    Vec boardSpacePos;

    s32 textChoice;
    s32 currPlayer;
    s32 currSpace;
    s32 spaceLinkFlag;

    currPlayer = GWSystem.player_curr;
    BoardWinCreateChoice(2, MAKE_MESSID(19, 17), 4, 0);
    if (GWPlayer[currPlayer].com) {
        if (frandBool() != 0) {
            BoardComKeySetLeft();
        }
        else {
            BoardComKeySetRight();
        }
    }
    BoardWinWait();
    BoardWinKill();
    textChoice = BoardWinChoiceGet();
    if ((textChoice == 1) || (textChoice == -1)) {
        BoardWinCreate(2, MAKE_MESSID(19, 18), 4);
        BoardWinWait();
        BoardWinKill();
        BoardMGExit();
        while (BoardMGDoneFlagGet() != 1) {
            HuPrcVSleep();
        }
        BoardMGDoneFlagSet(0);
        return;
    }
    BoardAudSeqPause(0, 1, 1000);
    HuPrcChildCreate(BoardMinigameMusicStart, 0x2003U, 0x1000U, 0, boardMainProc);
    rotation.x = 0.0f;
    rotation.y = rotation.z = 0.0f;
    offset.y = 135.0f;
    offset.x = offset.z = 0.0f;
    BoardCameraMotionStartEx(boardModelIds[8], &rotation, &offset, 1200.0f, 25.0f, 0x15);
    currSpace = GWPlayer[currPlayer].space_curr;
    spaceLinkFlag = BoardSpaceLinkFlagSearch(0, currSpace, 0x02000000U);
    BoardSpacePosGet(0, spaceLinkFlag, &boardSpacePos);
    BoardPlayerMoveBetween(currPlayer, currSpace, spaceLinkFlag);
    while (GWPlayer[currPlayer].moving) {
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(currPlayer, 1, 0x40000001);
    CoinMinigameStart();

    while (GWPlayer[currPlayer].moving) {
        HuPrcVSleep();
    }
    while (BoardEventFlagGet() != 0) {
        HuPrcVSleep();
    }
    while (GWPlayer[currPlayer].moving) {
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(currPlayer, 1, 0x40000001);
}

void BoardMinigameItemEvent(void)
{
    Vec rotation;
    Vec offset;
    s32 currSpace;
    s32 spaceLinkFlags;
    s32 textChoice;
    s32 currPlayer;
    s32 i, j;

    currPlayer = GWSystem.player_curr;
    if (BoardPlayerItemCount((s32)currPlayer) == 3) {
        BoardWinCreate(2, MAKE_MESSID(19, 26), 4);
        BoardWinWait();
        BoardWinKill();
        HuPrcEnd();
    }
    BoardCameraTargetModelSet(boardModelIds[10]);
    BoardCameraMotionWait();

    /* Generate three random unique items where
       the first item is MINI_MUSHROOM and the
       other two are not BOWSER_SUIT or ITEM_BAG.
    */
    itemChoices[0] = BOARD_ITEM_MINI_MUSHROOM;
    for (i = 1; i < ARRAY_COUNT(itemChoices);) {
        itemChoices[i] = frandmod(BOARD_ITEMS_END);
        if (itemChoices[i] != BOARD_ITEM_BOWSER_SUIT && itemChoices[i] != BOARD_ITEM_ITEM_BAG) {
            for (j = 0; j < i; j++) {
                if (itemChoices[i] == itemChoices[j]) {
                    itemChoices[i] = BOARD_ITEM_NONE;
                }
            }
            if (itemChoices[i] != BOARD_ITEM_NONE) {
                i++;
            }
        }
    }
    ItemMinigameItemsRandomize(itemChoices);

    HuAudFXPlay(0x43);
    BoardWinCreateChoice(2, MAKE_MESSID(19, 22), 4, 0);
    if (GWPlayer[currPlayer].com) {
        if (BoardComPreferItemCheck(currPlayer, itemChoices[0], itemChoices[1], itemChoices[2]) != -1) {
            BoardComKeySetLeft();
        }
        else {
            BoardComKeySetRight();
        }
    }
    BoardWinWait();
    BoardWinKill();
    textChoice = BoardWinChoiceGet();
    if ((textChoice == 1) || (textChoice == -1)) {
        HuAudFXPlay(0x44);
        BoardWinCreate(2, MAKE_MESSID(19, 23), 4);
        BoardWinWait();
        BoardWinKill();
        BoardMGExit();
        ItemMinigameItemsDrop(1);
        while (BoardMGDoneFlagGet() != 1) {
            HuPrcVSleep();
        }
        BoardMGDoneFlagSet(0);
        ItemMinigamePlayerDestroy();
        HuPrcEnd();
    }
    rotation.x = -30.0f;
    rotation.y = rotation.z = 0.0f;
    offset.x = 0.0f;
    offset.y = 0.0f;
    offset.z = -250.0f;
    BoardCameraMotionStartEx(boardModelIds[10], (Vec *)&rotation, (Vec *)&offset, 1500.0f, 25.0f, 0x15);
    currSpace = GWPlayer[currPlayer].space_curr;
    spaceLinkFlags = BoardSpaceLinkFlagSearch(0, currSpace, 0x02000000U);
    BoardPlayerMoveBetween(currPlayer, currSpace, spaceLinkFlags);
    while (GWPlayer[currPlayer].moving) {
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(currPlayer, 1, 0x40000001);
    BoardPlayerMotBlendSet(currPlayer, 0xB4, 0xF);
    while (BoardPlayerMotBlendCheck(currPlayer) == 0) {
        HuPrcVSleep();
    }
    BoardCameraMotionWait();
    ItemMinigameStart();
    while (BoardEventFlagGet() != 0) {
        HuPrcVSleep();
    }
    ItemMinigameItemsDrop(0);
    while (GWPlayer[currPlayer].moving) {
        HuPrcVSleep();
    }
    ItemMinigamePlayerDestroy();
    BoardPlayerMotionStart(currPlayer, 1, 0x40000001);
    HuPrcEnd();
}

void BoardMinigameItemCleanup(void)
{
    BoardWinKill();
    CharModelDataClose(-1);
    itemEventProcess = 0;
}

void BoardMinigameItemEventStart(void)
{
    itemEventProcess = HuPrcChildCreate(BoardMinigameItemEvent, 0x2003U, 0x2000U, 0, boardMainProc);
    HuPrcDestructorSet2(itemEventProcess, BoardMinigameItemCleanup);
    while (itemEventProcess) {
        HuPrcVSleep();
    }
}

s32 BoardMinigameEventExecute(void)
{
    u32 spaceFlags;
    s16 currSpace;
    currSpace = GWPlayer[GWSystem.player_curr].space_curr;
    spaceFlags = BoardSpaceFlagGet(0, currSpace) & 0x600000;
    BoardRollDispSet(0);

    if (spaceFlags & 0x200000) {
        BoardItemSpaceEvent();
    }
    else if (spaceFlags & 0x400000) {
        itemEventProcess = HuPrcChildCreate(BoardMinigameItemEvent, 0x2003U, 0x2000U, 0, boardMainProc);
        HuPrcDestructorSet2(itemEventProcess, BoardMinigameItemCleanup);
        while (itemEventProcess) {
            HuPrcVSleep();
        }
    }

    BoardRollDispSet(1);
}

/**
* @brief Display a message in a board window
*
* @details Handles the entire lifecycle of a text box displaying a message on 
* the board. Creates the window, waits for user input, and then kills the window.
*
* @param mesg Message ID to display
*/
void W02MesExec(u32 mesg)
{
    BoardWinCreate(2, mesg, 4);
    BoardWinWait();
    BoardWinKill();
}

f32 const padMain = 0.0f;
