#include "game/board/tutorial.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/space.h"
#include "game/flag.h"
#include "game/gamework_data.h"
#include "game/process.h"

#include <string.h>

s32 boardTutorialData[4]; // maybe a single s32 + padding?
static void (*tutorialHook)(s16, s32);
s8 boardTutorialDirInputX;
s8 boardTutorialDirInputY;
s16 boardTutorialDirInputTime;
s32 boardTutorialBlockPos;
s8 boardTutorialBlockItem;
s8 boardTutorialBlockF;
s8 boardTutorialUseItem;
static s16 hostMdl;
s8 boardTutorialF;
static HUPROCESS *tutorialProcess;
static void *playerCfgCopy;
static void *playerCopy;
static void *systemCopy;
static void *gameStatCopy;

/**
 * @brief Invoke the tutorial frame hook after board initialization on every frame.
 */
static void TutorialProcFunc(void) {
    while (1) {
        if (_CheckFlag(FLAG_ID_MAKE(1, 16))) {
            BoardTutorialHookExec(0x1E, 0);
        }
        HuPrcVSleep();
    }
}

/**
 * @brief Reset tutorial controls and start the tutorial hook process in tutorial mode.
 */
void BoardTutorialInit(void) {
    boardTutorialDirInputTime = -1;
    boardTutorialBlockF = 0;
    boardTutorialUseItem = -1;
    tutorialHook = NULL;
    tutorialProcess = NULL;
    boardTutorialF = 0;
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        tutorialProcess = HuPrcChildCreate(TutorialProcFunc, 0x2002, 0x2000, 0, boardMainProc);
    }
}

/**
 * @brief Stop the tutorial hook process and clear its active flag.
 */
void BoardTutorialKill(void) {
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        if (tutorialProcess) {
            HuPrcKill(tutorialProcess);
        }
        boardTutorialF = 0;
    }
}

/**
 * @brief Install or clear the board-specific tutorial event hook.
 * @param [in] arg0 Tutorial callback taking an event code and event-specific integer argument; NULL clears it.
 */
void BoardTutorialHookSet(void *arg0) {
    tutorialHook = arg0;
}

/**
 * @brief Invoke the installed tutorial hook with an event code and event argument.
 * @param [in] arg0 Tutorial event code forwarded to the installed board hook.
 * @param [in] arg1 Event-specific integer forwarded unchanged to the tutorial hook.
 */
void BoardTutorialHookExec(s16 arg0, s32 arg1) {
    if (tutorialHook) {
        tutorialHook(arg0, arg1);
    }
}

/**
 * @brief Mark all players as computer-controlled and select the tutorial board.
 */
void BoardTutorialPlayerInit(void) {
    s32 i;

    for (i = 0; i < 4; i++) {
        GWPlayer[i].com = 1;
        GWPlayerCfg[i].iscom = 1;
    }
    GWSystem.board = BOARD_ID_TUTORIAL;
}

/**
 * @brief Allocate snapshots of global game, board, player, and player-configuration state.
 */
void BoardTutorialWorkSave(void) {
    OSReport("-O-O-O-O-O-O-O  Tutorial Save Work\n");
    if (gameStatCopy) {
        HuMemDirectFree(gameStatCopy);
    }
    if (systemCopy) {
        HuMemDirectFree(systemCopy);
    }
    if (playerCopy) {
        HuMemDirectFree(playerCopy);
    }
    if (playerCfgCopy) {
        HuMemDirectFree(playerCfgCopy);
    }
    gameStatCopy = HuMemDirectMalloc(HEAP_HEAP, sizeof(GWGameStat));
    systemCopy = HuMemDirectMalloc(HEAP_HEAP, sizeof(GWSystem));
    playerCopy = HuMemDirectMalloc(HEAP_HEAP, sizeof(GWPlayer));
    playerCfgCopy = HuMemDirectMalloc(HEAP_HEAP, sizeof(GWPlayerCfg));
    memcpy(gameStatCopy, &GWGameStat, sizeof(GWGameStat));
    memcpy(systemCopy, &GWSystem, sizeof(GWSystem));
    memcpy(playerCopy, GWPlayer, sizeof(GWPlayer));
    memcpy(playerCfgCopy, GWPlayerCfg, sizeof(GWPlayerCfg));
}

/**
 * @brief Restore and free the global-state snapshots created for tutorial mode.
 */
void BoardTutorialWorkRestore(void) {
    OSReport("-O-O-O-O-O-O-O  Tutorial Load Work\n");
    memcpy(&GWGameStat, gameStatCopy, sizeof(GWGameStat));
    memcpy(&GWSystem, systemCopy, sizeof(GWSystem));
    memcpy(GWPlayer, playerCopy, sizeof(GWPlayer));
    memcpy(GWPlayerCfg, playerCfgCopy, sizeof(GWPlayerCfg));
    HuMemDirectFree(gameStatCopy);
    HuMemDirectFree(systemCopy);
    HuMemDirectFree(playerCopy);
    HuMemDirectFree(playerCfgCopy);
    gameStatCopy = NULL;
    systemCopy = NULL;
    playerCopy = NULL;
    playerCfgCopy = NULL;
}

/**
 * @brief Set the synthetic tutorial stick direction and its duration.
 * @param [in] arg0 Synthetic horizontal stick input.
 * @param [in] arg1 Synthetic vertical stick input.
 * @param [in] arg2 Number of frames for which the synthetic direction remains active.
 */
void BoardTutorialDirInputSet(s8 arg0, s8 arg1, s16 arg2) {
    boardTutorialDirInputX = arg0;
    boardTutorialDirInputY = arg1;
    boardTutorialDirInputTime = arg2;
}

/**
 * @brief Configure the tutorial block's board position and contained item, then reposition it.
 * @param [in] arg0 Board-space index at which to place the tutorial block.
 * @param [in] arg1 Item identifier stored in the tutorial block.
 */
void BoardTutorialBlockSetPos(s32 arg0, s8 arg1) {
    boardTutorialBlockF = 1;
    boardTutorialBlockPos = arg0;
    boardTutorialBlockItem = arg1;
    BoardSpaceBlockPosSet();
}

/**
 * @brief Select the item that the tutorial computer player must use next.
 * @param [in] arg0 Item identifier the tutorial should force the CPU to use, or -1 for none.
 */
void BoardTutorialItemSet(s8 arg0) {
    boardTutorialUseItem = arg0;
}

/**
 * @brief Store the board-model handle used as the tutorial host.
 * @param [in] arg0 Board-model handle for the tutorial host.
 */
void BoardTutorialHostSet(s16 arg0) {
    hostMdl = arg0;
}

/**
 * @brief Set the tutorial host model's visibility state.
 * @param [in] arg0 Visibility value passed to BoardModelVisibilitySet.
 */
void BoardTutorialHostHide(s8 arg0) {
    BoardModelVisibilitySet(hostMdl, arg0);
}
