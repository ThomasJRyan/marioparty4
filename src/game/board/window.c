#include "game/board/window.h"
#include "game/board/main.h"
#include "game/board/player.h"
#include "game/disp.h"

#include "game/gamework_data.h"

static s8 winChoice;
static s8 autoPickF;
static s8 winPortrait;
static s8 winPosIdx;
static u8 disablePlayer;
static s8 winPause;
static s8 winWait;
static s16 winPrio;
static s16 messSpeed;
static s16 defaultChoice;
static s32 winMess;
static s32 winAttr;
f32 lbl_801D3F70[2];
static BoardWinComKeyFunc comKeyFunc;
static HUPROCESS* winProc;
static s32 choiceDisableTbl[0x10];
static u32 winInsertMesTbl[8];

static s32 PortraitTbl[] = {
    0,  4,  2,  3,
    7,  6,  0,  12,
    12, 
};
static s32 StarPortraitTbl[] = {
    13, 16, 14, 15,
    18, 6,  13, 12,
    12
};

static s16 windowID = -1;
static const s8 winSpeedTbl[4] = {0, 1, 4, 0};

/**
 * @brief Initialize one engine window layer for board messages.
 */
void BoardWinInit(void) {
    HuWinInit(1);
}

/**
 * @brief Copy two caller values into the preserved board-window array, or clear it for NULL.
 * @param [in] arg0 Two-float source array, or NULL to clear both preserved values.
 */
void BoardWinUnusedArraySet(f32* arg0) {
    if (arg0 != 0) {
        lbl_801D3F70[0] = arg0[0];
        lbl_801D3F70[1] = arg0[1];
        return;
    }
    lbl_801D3F70[0] = lbl_801D3F70[1] = 0.0f;
}

/**
 * @brief Terminate the active board window process and destroy all engine windows.
 */
void BoardWinKillAll(void) {
    BoardWinKill();
    HuWinAllKill();
}

/**
 * @brief Clean up the active extended window and reset board-window state.
 */
void BoardWinDestroy(void) {
    if (windowID >= 0) {
        if (winPortrait == -1) {
            HuWinExCleanup(windowID);
        } else {
            HuWinExCleanup(windowID);
            winPortrait = -1;
        }
        windowID = -1;
    }
    winMess = 0;
    winProc = 0;
}

/**
 * @brief Create, configure, display, collect input for, and animate out the pending board message.
 */
static void ExecBoardWindow(void) {
    f32 size[2];
    f32 pos[2];
    s32 i;
    f32 *pos_win;
    f32 *size_win;
    WindowData *win_curr = NULL;

    f32 size_all[7][2] = {
        {384.0f,  64.0f}, {432.0f, 192.0f},
        {504.0f,  96.0f}, {432.0f, 128.0f},
        {288.0f, 192.0f}, {432.0f,  96.0f},
        {432.0f,  96.0f},
    };
    f32 pos_all[7][2] = {
        {96.0f, HU_DISP_HEIGHT-152},  {72.0f, 128.0f},
        {36.0f, HU_DISP_HEIGHT-136},  {128.0f, HU_DISP_HEIGHT-168},
        {144.0f, HU_DISP_CENTERY-96 }, {128.0f, HU_DISP_HEIGHT-216},
        {128.0f,  75.0f},
    };
    
    if (windowID < 0) {
        size_win = size_all[winPosIdx];
        pos_win = pos_all[winPosIdx];
        HuWinMesMaxSizeGet(1, size, winMess);
        if (size[0] <= size_win[0]) {
            size[0] = size_win[0];
            pos[0] = pos_win[0];
            #if VERSION_PAL
            if(winPosIdx == 1) {
                size[0] += 4;
            }
            #endif
        } else {
            pos[0] = -10000.0f;
        }
        if (size[1] <= size_win[1]) {
            size[1] = size_win[1];
        }
        pos[1] = pos_win[1];
        if ((winPosIdx == 5) || (winPosIdx == 6)) {
            windowID = HuWinExCreateStyled(pos[0], pos[1], size[0], size[1], -1, 1);
            HuWinMesPalSet(windowID, 7, 0, 0, 0);
        } else if (winPortrait == -1) {
            windowID = HuWinExCreate(pos[0], pos[1], size[0], size[1], -1);
        } else {
            windowID = HuWinExCreate(pos[0], pos[1], size[0], size[1], winPortrait);
        }
        if (windowID == -1) {
            HuPrcEnd();
        }
        if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
            HuWinPriSet(windowID, 3);
        }
    }
    if (disablePlayer != 0) {
        HuWinDisablePlayerSet(windowID, disablePlayer);
    }
    if (winPrio != -1) {
        HuWinPriSet(windowID, winPrio);
    }
    if (winPortrait == -1) {
        HuWinExAnimIn(windowID);
    } else {
        HuWinExAnimIn(windowID);
    }
    HuWinMesSpeedSet(windowID, messSpeed);
    HuWinMesSet(windowID, winMess);
    
    for (i = 0; i < 8; i++) {
        if (winInsertMesTbl[i] != -1) {
            HuWinInsertMesSet(windowID, winInsertMesTbl[i], i);
        }
    }
    HuWinAttrSet(windowID, winAttr);
    
    for (i = 0; i < 16; i++) {
        if (choiceDisableTbl[i] != 0) {
            HuWinChoiceDisable(windowID, i);
        }
    }
    if (autoPickF != 0) {
        BoardWinStartComKeySet();
        if (comKeyFunc) {
            comKeyFunc();
        }
    } else if ((_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) && ((winPosIdx == 5) || (winPosIdx == 6))) {
        win_curr = &winData[windowID];
        win_curr->active_pad = 0xF;
        HuWinComKeyWait(-1, -1, -1, -1, 0);
    } else {
        BoardWinStartComKeySet();
    }
    HuWinMesWait(windowID);
    if (winWait != 0) {
        HuWinKeyWaitEntry(windowID);
        HuWinMesWait(windowID);
    }
    if (winPause != 0) {
        HuPrcSleep(-1);
    }
    if (defaultChoice != -1) {
        winChoice = HuWinChoiceGet(windowID, defaultChoice);
    }
    if (winPortrait != -1) {
        HuWinExAnimOut(windowID);
    }
    HuPrcEnd();
}

/**
 * @brief Return the choice recorded after the active message completed.
 * @return Choice index recorded after message completion.
 */
s32 BoardWinChoiceGet(void) {
    return winChoice;
}

/**
 * @brief Keep the active board-window process asleep after its message completes.
 */
void BoardWinPause(void) {
    winPause = 1;
}

/**
 * @brief Queue a board message window with choice collection enabled.
 * @param [in] pos Window layout preset index from 0 through 6.
 * @param [in] mess Packed message ID.
 * @param [in] portrait Portrait ID, or -1 to create a portraitless window.
 * @param [in] choice Choice index.
 */
void BoardWinCreateChoice(s32 pos, u32 mess, s32 portrait, s32 choice) {
    s32 i;

    winPosIdx = pos;
    winChoice = 0;
    defaultChoice = choice;
    winPortrait = portrait;
    winAttr = 0;
    autoPickF = 1;
    winWait = 0;
    comKeyFunc = NULL;
    disablePlayer = 0;
    winPrio = -1;
    winPause = 0;
    HuWinComKeyReset();
    
    for (i = 0; i < 16; i++) {
        choiceDisableTbl[i] = 0;
    }
    
    for (i = 0; i < 8; i++) {
        winInsertMesTbl[i] = -1;
    }
    if (winProc == 0) {
        winProc = HuPrcChildCreate(&ExecBoardWindow, 0x2009, 0x4000, 0, boardMainProc);
        HuPrcDestructorSet2(winProc, &BoardWinDestroy);
    }
    winMess = mess;
    BoardWinSpeedSet(GWMessSpeedGet());
    BoardWinPlayerSet(GWSystem.player_curr);
}

/**
 * @brief Queue a board message window without a selectable choice.
 * @param [in] pos Window layout preset index from 0 through 6.
 * @param [in] mess Packed message ID.
 * @param [in] portrait Portrait ID, or -1 to create a portraitless window.
 */
void BoardWinCreate(s32 pos, u32 mess, s32 portrait) {
    s32 i;

    winPosIdx = pos;
    winChoice = 0;
    defaultChoice = -1;
    winPortrait = portrait;
    winAttr = 0;
    autoPickF = 1;
    winWait = 0;
    comKeyFunc = NULL;
    disablePlayer = 0;
    winPrio = -1;
    winPause = 0;
    HuWinComKeyReset();
    
    for (i = 0; i < 0x10; i++) {
        choiceDisableTbl[i] = 0;
    }
    
    for (i = 0; i < 8; i++) {
        winInsertMesTbl[i] = -1;
    }
    if (winProc == 0) {
        winProc = HuPrcChildCreate(&ExecBoardWindow, 0x2009U, 0x4000U, 0, boardMainProc);
        HuPrcDestructorSet2(winProc, &BoardWinDestroy);
    }
    winMess = mess;
    BoardWinSpeedSet(GWMessSpeedGet());
    BoardWinPlayerSet(GWSystem.player_curr);
    autoPickF = 0;
}

/**
 * @brief Set one message-substitution entry for the pending board window.
 * @param [in] value Value applied by the operation.
 * @param [in] index One-based entry index unless the function documents a table index.
 */
void BoardWinInsertMesSet(u32 value, s32 index) {
    winInsertMesTbl[index] = value;
}

/**
 * @brief Check whether the board-window process has terminated.
 * @return TRUE when no board-window process exists; otherwise FALSE.
 */
static inline BOOL BoardWinDone(void) {
    if(winProc != NULL) {
        return 0;
    } else {
        return 1;
    }
}

/**
 * @brief Yield until the active board-window process terminates.
 */
void BoardWinWait(void) {
    while(!BoardWinDone()) {
        HuPrcVSleep();
    }
}

/**
 * @brief Terminate the active board-window process if one exists.
 */
void BoardWinKill(void) {
    if (winProc != 0) {
        HuPrcKill(winProc);
        winProc = NULL;
    }
}

/**
 * @brief Add window attribute bits to the pending and active board window.
 * @param [in] attr Engine attribute bit mask.
 */
void BoardWinAttrSet(s32 attr) {
    winAttr |= attr;
    if (windowID >= 0) {
        HuWinAttrSet(windowID, winAttr);
    }
}

/**
 * @brief Remove window attribute bits from the pending and active board window.
 * @param [in] attr Engine attribute bit mask.
 */
void BoardWinAttrReset(s32 attr) {
    winAttr &= ~attr;
    if (windowID >= 0) {
        HuWinAttrSet(windowID, winAttr);
    }
}

/**
 * @brief Disable a choice index in the pending board window.
 * @param [in] choice Choice index.
 */
void BoardWinChoiceDisable(s32 choice) {
    choiceDisableTbl[choice] = 1;
}

/**
 * @brief Require an additional key press after the pending message finishes printing.
 */
void BoardWinKeyWait(void) {
    winWait = 1;
}

/**
 * @brief Map the configured message-speed setting to an engine text delay.
 * @return Engine text delay mapped from the current global message-speed setting.
 */
s32 BoardWinSpeedGet(void) {
    return winSpeedTbl[GWMessSpeedGet()];
}

/**
 * @brief Apply a configured message-speed setting to pending and active text.
 * @param [in] value Value applied by the operation.
 */
void BoardWinSpeedSet(s32 value) {
    messSpeed = winSpeedTbl[value];
    if (windowID != -1) {
        HuWinMesSpeedSet(windowID, messSpeed);
    }
}

/**
 * @brief Read the currently highlighted choice from the active board window.
 * @return Highlighted choice index, or -1 when no window exists.
 */
s16 BoardWinChoiceNowGet(void) {
    if (windowID == -1) {
        return -1;
    }
    return HuWinChoiceNowGet(windowID);
}

/**
 * @brief Set the pending and active board window's draw priority.
 * @param [in] prio Window draw priority.
 */
void BoardWinPriSet(s16 prio) {
    winPrio = prio;
    if (windowID != -1) {
        HuWinPriSet(windowID, prio);
    }
}

/**
 * @brief Read the currently highlighted choice from the active board window.
 * @return Highlighted choice index, or -1 when no window exists.
 */
s16 BoardWinChoiceNowGet2(void) {
    if (windowID == -1) {
        return -1;
    }
    return HuWinChoiceNowGet(windowID);
}

/**
 * @brief Get the normal host portrait configured for the current board.
 * @return Portrait ID for normal host dialogue on the current board.
 */
s32 BoardWinPortraitGet(void) {
    s32 temp = GWSystem.board;
    return PortraitTbl[temp];
}

/**
 * @brief Get the star-dialogue portrait configured for the current board.
 * @return Portrait ID for star dialogue on the current board.
 */
s32 BoardWinPortraitGetStar(void) {
    s32 temp = GWSystem.board;
    return StarPortraitTbl[temp];
}

/**
 * @brief Restrict window input to the requested human player's port, accounting for tutorial and CPU state.
 * @param [in] player Player index.
 */
void BoardWinPlayerSet(s32 player) {
    s32 i;
    #if VERSION_NTSC
    if (player == -1) {
        if (GWPartyGet() != 1) {
            for (i = 0, disablePlayer = i; i < 4; i++) {
                if (GWPlayer[i].com != 0) {
                    disablePlayer |= 1 << GWPlayer[i].port;
                }
            }
        } else {
            disablePlayer = 0;
        }
    } else if (GWPlayer[player].com != 0) {
        disablePlayer = 0xF;
    } else {
        disablePlayer = ~(1 << GWPlayer[player].port);
    }
    #else
    if (player == -1) {
        if (GWPartyGet() != 1) {
            for (i = 0, disablePlayer = i; i < 4; i++) {
                if (GWPlayer[i].com != 0) {
                    disablePlayer |= 1 << GWPlayer[i].port;
                }
            }
        } else {
            disablePlayer = 0;
        }
    } else {
        disablePlayer = ~(1 << GWPlayer[player].port);
    }
    #endif
    if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
        disablePlayer = 0;
    }
    if (windowID != -1) {
        HuWinDisablePlayerSet(windowID, disablePlayer);
    }
}

/**
 * @brief Queue automatic confirmation presses for computer-controlled message input.
 */
void BoardWinStartComKeySet(void) {
    s32 comkey[4];
    s32 delay;
    s32 waitnum;
    s32 i;

    if (BoardPlayerIsAllCom() != 0) {
        comkey[0] = comkey[1] = comkey[2] = comkey[3] = 0x100;
    } else {
        if (GWSystem.player_curr == -1) {
            if ((GWPlayer[0].com == 0) || (GWPlayer[1].com == 0) || (GWPlayer[2].com == 0) || (GWPlayer[3].com == 0)) {
                return;
            }
        } else if (GWPlayer[GWSystem.player_curr].com == 0) {
            return;
        }
        if (GWSystem.player_curr != -1) {
            for (i = 0; i < 4; i++) {
                s32 port = GWPlayer[i].port;
                if (GWSystem.player_curr == i) {
                    comkey[port] = 0x100;
                } else {
                    comkey[port] = 0;
                }
            }
        } else {
            for (i = 0; i < 4; i++) {
                comkey[i] = 0x100;
            }
        }
    }
    delay = GWMessDelayGet();
    waitnum = HuWinKeyWaitNumGet(winMess);
    if (waitnum != 0) {
        HuWinComKeyReset();
        for (i = 0; i < waitnum; i++) {
            HuWinComKeyWait(comkey[0], comkey[1], comkey[2], comkey[3], delay);
        }
    }
}

/**
 * @brief Install or clear the callback that supplies automatic choice input.
 * @param [in] func Callback to install, or NULL to clear it.
 */
void BoardWinComKeyFuncSet(BoardWinComKeyFunc func) {
    comKeyFunc = func;
}

/**
 * @brief Get the active engine window ID.
 * @return Active engine window ID, or -1 when no window exists.
 */
s16 BoardWinIDGet(void)
{
    return windowID;
}
