#include "game/board/main.h"
#include "ext_math.h"
#include "game/board/battle.h"
#include "game/board/lottery.h"
#include "game/board/model.h"
#include "game/board/pause.h"
#include "game/board/player.h"
#include "game/board/shop.h"
#include "game/board/space.h"
#include "game/board/start.h"
#include "game/board/tutorial.h"
#include "game/board/ui.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/flag.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/msm.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/wipe.h"
#include "string.h"

#include "game/armem.h"
#include "game/audio.h"
#include "game/board/audio.h"
#include "game/board/boo_house.h"
#include "game/board/bowser.h"
#include "game/board/fortune.h"
#include "game/board/mg_setup.h"
#include "game/board/roll.h"
#include "game/board/window.h"
#include "game/chrman.h"

extern void BoardLast5Exec(void);

typedef struct camera_view {
    s16 x_rot;
    s16 zoom;
    s16 fov;
} CameraView;

OMOBJ *boardMainObj;
u32 boardRandSeed;
static OMOBJ *last5GfxObj;
static OMOBJ *confettiObj;
static OMOBJ *filterObj;
BoardTurnStartHook boardTurnStartFunc;
BoardBowserHook boardBowserHook;
void (*boardStarShowNextHook)(void);
void (*boardStarGiveHook)(void);
BoardFunc boardTurnFunc;
BoardLightHook boardLightResetHook;
BoardLightHook boardLightSetHook;
static BoardFunc destroyFunc;
static BoardFunc createFunc;
static s32 cameraUseBackup;
static OMOBJ *tauntObj;
static OMOBJ *cameraObj;
HUPROCESS *boardObjMan;
HUPROCESS *boardMainProc;

BoardCameraData boardCamera;
static BoardCameraData cameraBackup;

static OMOVL nextOvl = DLL_NONE;

static CameraView camViewTbl[] = {
    { 0, 0, 25 },
    { -33, 3200, 25 },
    { -33, 2100, 25 },
    { -33, 1800, 25 },
    { -33, 12640, 25 },
    { -33, 3200, 25 },
};


extern void BoardMGSetupPlayClear(void);

static void InitBoardFunc(OMOBJ *object);
static void ExecBoardFunc(OMOBJ *object);
static void KillBoardFunc(OMOBJ *unused_object);
static void UpdateCamera(OMOBJ *object);

static void CalcCameraTarget(BoardCameraData *camera);
static void CalcCameraPos(BoardCameraData *camera);

static void MainFunc(void);
static void DestroyMainFunc(void);

static s32 ExecTurnStart(void);

static void CreateBoard(void);
static void DestroyBoard(void);

/**
 * @brief Initialize board flags, callbacks, object management, and the board identifier for the current overlay.
 * @param [in] create Board-overlay callback that creates board-specific resources.
 * @param [in] destroy Board-overlay callback that destroys board-specific resources.
 */
void BoardObjectSetup(BoardFunc create, BoardFunc destroy)
{
    omSysPauseEnable(FALSE);
    if(!_CheckFlag(FLAG_ID_MAKE(1, 0))) {
        _SetFlag(FLAG_ID_MAKE(1, 0));
        _ClearFlag(FLAG_ID_MAKE(2, 1));
        _ClearFlag(FLAG_ID_MAKE(2, 3));
        _ClearFlag(FLAG_ID_MAKE(2, 0));
        _ClearFlag(FLAG_ID_MAKE(2, 2));
        _ClearFlag(FLAG_ID_MAKE(2, 4));
        _ClearFlag(FLAG_ID_MAKE(2, 5));
        _ClearFlag(FLAG_ID_MAKE(1, 6));
        _ClearFlag(FLAG_ID_MAKE(1, 9));
        _ClearFlag(FLAG_ID_MAKE(0, 8));
        _ClearFlag(FLAG_ID_MAKE(0, 10));
        BoardMGSetupPlayClear();
    }

    nextOvl = DLL_NONE;
    _SetFlag(FLAG_ID_MAKE(1, 28));
    _SetFlag(FLAG_ID_MAKE(1, 14));
    _ClearFlag(FLAG_ID_MAKE(1, 16));
    _ClearFlag(FLAG_ID_MAKE(1, 17));
    _ClearFlag(FLAG_ID_MAKE(1, 18));
    _ClearFlag(FLAG_ID_MAKE(1, 19));
    _ClearFlag(FLAG_ID_MAKE(1, 20));
    _ClearFlag(FLAG_ID_MAKE(1, 21));
    _ClearFlag(FLAG_ID_MAKE(1, 23));
    _ClearFlag(FLAG_ID_MAKE(1, 24));
    _ClearFlag(FLAG_ID_MAKE(1, 25));
    _ClearFlag(FLAG_ID_MAKE(1, 27));
    createFunc = create;
    destroyFunc = destroy;
    boardTurnFunc = NULL;
    boardStarShowNextHook = NULL;
    boardBowserHook = NULL;
    boardStarGiveHook = NULL;
    boardTurnStartFunc = NULL;
    boardObjMan = omInitObjMan(64, 8192);
    omSystemKeyCheckSetup(boardObjMan);
    boardMainObj = omAddObjEx(boardObjMan, 0, 0, 0, -1, InitBoardFunc);
    switch(omcurovl) {
        case DLL_w01dll:
            GWSystem.board = BOARD_ID_MAIN1;
            break;

        case DLL_w02dll:
            GWSystem.board = BOARD_ID_MAIN2;
            break;

        case DLL_w03dll:
            GWSystem.board = BOARD_ID_MAIN3;
            break;

        case DLL_w04dll:
            GWSystem.board = BOARD_ID_MAIN4;
            break;

        case DLL_w05dll:
            GWSystem.board = BOARD_ID_MAIN5;
            break;

        case DLL_w06dll:
            GWSystem.board = BOARD_ID_MAIN6;
            break;

        case DLL_w10dll:
            GWSystem.board = BOARD_ID_TUTORIAL;
            break;

        case DLL_w20dll:
            GWSystem.board = BOARD_ID_EXTRA1;
            break;

        case DLL_w21dll:
            GWSystem.board = BOARD_ID_EXTRA2;
            break;
    }
}

/**
 * @brief Start the main board process and switch the manager object to its runtime callback.
 * @param [in,out] object Board-manager object whose callback is changed to ExecBoardFunc after process creation.
 */
static void InitBoardFunc(OMOBJ *object)
{
    boardMainProc = HuPrcChildCreate(MainFunc, 8194, 0x6000, 0, boardObjMan);
    HuPrcDestructorSet2(boardMainProc, DestroyMainFunc);
    object->objFunc = ExecBoardFunc;
}

/**
 * @brief Process pause and board-exit requests after board initialization completes.
 * @param [in,out] object Board-manager object switched to KillBoardFunc after shutdown begins.
 */
static void ExecBoardFunc(OMOBJ *object)
{
    if(!_CheckFlag(FLAG_ID_MAKE(1, 16))) {
        return;
    }
    if(!_CheckFlag(FLAG_ID_MAKE(1, 17))) {
        if(omSysExitReq) {
            BoardKill();
        } else {
            if(BoardPauseReqCheck()) {
                BoardPauseStart();
            } else {
                return;
            }
        }
    } else {
        if(boardMainProc) {
            HuPrcKill(boardMainProc);
        }
        object->objFunc = KillBoardFunc;
    }
}

/**
 * @brief Return from or transfer away from the board overlay once processes, wipes, and ARAM DMA finish.
 * @param [in] unused_object Unused board-manager callback argument.
 */
static void KillBoardFunc(OMOBJ *unused_object)
{
    if(boardMainProc) {
        return;
    }
    if(WipeStatGet()) {
        return;
        return_alt:
        return;
    } else {
        if(HuARDMACheck()) {
            goto return_alt;
        }
        if(nextOvl != DLL_NONE) {
            omOvlCallEx(nextOvl, 1, 0, 0);
        } else {
            if(_CheckFlag(FLAG_ID_MAKE(1, 11))) {
                BoardTutorialKill();
                BoardTutorialWorkRestore();
            }
            _ClearFlag(FLAG_ID_MAKE(1, 0));
            HuARDirFree(DATADIR_BOARD);
            if(_CheckFlag(FLAG_ID_MAKE(1, 27))) {
                omOvlReturnEx(2, 1);
            } else {
                omOvlReturnEx(1, 1);
            }
        }
    }
}

/**
 * @brief Stop board audio, fade out when needed, signal shutdown, and destroy board subsystems.
 */
void BoardKill(void)
{
    _SetFlag(FLAG_ID_MAKE(1, 28));
    _SetFlag(FLAG_ID_MAKE(1, 14));
    if(_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        BoardTutorialHookExec(29, 0);
    }
    HuAudFXAllStop();
    if(!BoardStartCheck()) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
        boardTutorialF = 1;
        while(WipeStatGet()) {
            HuPrcVSleep();
        }
    }
    _SetFlag(FLAG_ID_MAKE(1, 17));
    HuPrcKill(boardMainProc);
    DestroyBoard();
}

/**
 * @brief Check whether board shutdown has been requested.
 * @return 1 when shutdown is requested; otherwise 0.
 */
s32 BoardIsKill(void)
{
    return (_CheckFlag(FLAG_ID_MAKE(1, 17))) ? 1 : 0;
}

/**
 * @brief Set or clear the board flag that suppresses pausing.
 * @param [in] value Nonzero disables board pausing; zero permits it outside tutorial mode.
 */
void BoardPauseDisableSet(s32 value)
{
    if(_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        _SetFlag(FLAG_ID_MAKE(1, 25));
        omSysPauseEnable(FALSE);
    } else {
        if(value) {
            _SetFlag(FLAG_ID_MAKE(1, 25));
        } else {
            _ClearFlag(FLAG_ID_MAKE(1, 25));
        }
    }
}

/**
 * @brief Check whether pausing is disabled for the board.
 * @return 1 when board pause is disabled; otherwise 0.
 */
s32 BoardPauseDisableGet()
{
    return (_CheckFlag(FLAG_ID_MAKE(1, 25))) ? 1 : 0;
}

/**
 * @brief Reset board and player progress fields for a newly selected board.
 * @param [in] board Board ID stored in GWSystem.
 */
void BoardSaveInit(s32 board)
{
    s32 i;
    GWSystem.board = board;
    _ClearFlag(FLAG_ID_MAKE(1, 0));
    _ClearFlag(FLAG_ID_MAKE(1, 1));
    _ClearFlag(FLAG_ID_MAKE(1, 5));
    _ClearFlag(FLAG_ID_MAKE(1, 4));
    _ClearFlag(FLAG_ID_MAKE(1, 6));
    _ClearFlag(FLAG_ID_MAKE(1, 2));
    _ClearFlag(FLAG_ID_MAKE(1, 9));
    _ClearFlag(FLAG_ID_MAKE(1, 7));
    _ClearFlag(FLAG_ID_MAKE(1, 8));
    _ClearFlag(FLAG_ID_MAKE(0, 8));
    _ClearFlag(FLAG_ID_MAKE(1, 12));
    _ClearFlag(FLAG_ID_MAKE(1, 3));
    GWSystem.turn = 1;
    GWSystem.star_flag = 0;
    GWSystem.star_pos = 0;
    GWSystem.star_total = 1;
    GWSystem.last5_effect = 0;
    GWSystem.player_curr = -1;
    GWSystem.bowser_loss = 0;
    GWSystem.bowser_event = 0;
    GWSystem.lucky_value = 1;
    GWSystem.mg_next = 0;
    GWMGTypeSet(0);
    GWSystem.unk_38 = 0;
    GWSystem.block_pos = 0;
    memset(GWSystem.board_data, 0, sizeof(GWSystem.board_data));
    for(i=0; i<4; i++) {
        s32 party_flag;
        BoardPlayerAutoSizeSet(i, 0);
        GWPlayer[i].draw_ticket = 0;
        GWPlayer[i].color = 0;
        GWPlayer[i].bowser_suit = 0;
        GWPlayer[i].jump = 0;
        GWPlayer[i].space_shock = 0;
        GWPlayer[i].spark = 0;
        GWPlayer[i].show_next = 1;
        BoardPlayerCoinsSet(i, 0);
        if(_CheckFlag(FLAG_ID_MAKE(1, 11))) {
            GWSystem.team = 0;
        }
        GWPlayer[i].blue_count = 0;
        GWPlayer[i].red_count = 0;
        GWPlayer[i].question_count = 0;
        GWPlayer[i].fortune_count = 0;
        GWPlayer[i].bowser_count = 0;
        GWPlayer[i].battle_count = 0;
        GWPlayer[i].mushroom_count = 0;
        GWPlayer[i].warp_count = 0;
        GWPlayer[i].coins_mg = 0;
        GWPlayer[i].coins_total = 0;
        GWPlayer[i].coins_max = 0;
        GWPlayer[i].stars_max = 0;
        GWPlayer[i].coins_battle = 0;
        GWPlayer[i].coin_collect = 0;
        GWPlayer[i].coin_win = 0;
        GWPlayer[i].items[0] = -1;
        GWPlayer[i].items[1] = -1;
        GWPlayer[i].items[2] = -1;
        if(GWPartyGet() == 0 || _CheckFlag(FLAG_ID_MAKE(1, 11))) {
            GWStarsSet(i, 0);
        } else {
            GWStarsSet(i, BoardPlayerHandicapGet(i));
        }
    }
}

/**
 * @brief Configure story mode, its difficulty and minigame list, with no teams, bonuses, or handicaps.
 * @param [in] mg_list Minigame-pack selection passed to GWMGListSet.
 * @param [in] diff_story Story-mode difficulty.
 */
void BoardStoryConfigSet(s32 mg_list, s32 diff_story)
{
    GWSystem.party = 0;
    GWSystem.team = 0;
    GWSystem.diff_story = diff_story;
    GWSystem.bonus_star = 0;
    GWMGListSet(mg_list);
    GWPlayer[0].handicap = 0;
    GWPlayer[1].handicap = 0;
    GWPlayer[2].handicap = 0;
    GWPlayer[3].handicap = 0;
    GWSystem.max_turn = 15;
    memset(GWPlayer, 0, 4*sizeof(PlayerState));
    _ClearFlag(FLAG_ID_MAKE(0, 2));
    _ClearFlag(FLAG_ID_MAKE(0, 3));
    _ClearFlag(FLAG_ID_MAKE(0, 4));
    _ClearFlag(FLAG_ID_MAKE(0, 5));
    _ClearFlag(FLAG_ID_MAKE(0, 6));
    _ClearFlag(FLAG_ID_MAKE(0, 7));
    _ClearFlag(FLAG_ID_MAKE(1, 11));
    _SetFlag(FLAG_ID_MAKE(0, 11));
    _SetFlag(FLAG_ID_MAKE(1, 10));
}

/**
 * @brief Configure party mode, teams, bonus stars, minigame list, turn count, and handicaps.
 * @param [in] team Nonzero to enable team play.
 * @param [in] bonus_star Nonzero to enable bonus stars.
 * @param [in] mg_list Minigame-pack selection passed to GWMGListSet.
 * @param [in] max_turn Configured number of turns.
 * @param [in] p1_handicap Player 1 starting-star handicap.
 * @param [in] p2_handicap Player 2 starting-star handicap.
 * @param [in] p3_handicap Player 3 starting-star handicap.
 * @param [in] p4_handicap Player 4 starting-star handicap.
 */
void BoardPartyConfigSet(s32 team, s32 bonus_star, s32 mg_list, s32 max_turn, s32 p1_handicap, s32 p2_handicap, s32 p3_handicap, s32 p4_handicap)
{
    GWSystem.party = 1;
    GWSystem.team = team;
    GWSystem.diff_story = 0;
    GWSystem.bonus_star = bonus_star;
    GWMGListSet(mg_list);
    GWSystem.max_turn = max_turn;
    memset(GWPlayer, 0, 4*sizeof(PlayerState));
    GWPlayer[0].handicap = p1_handicap;
    GWPlayer[1].handicap = p2_handicap;
    GWPlayer[2].handicap = p3_handicap;
    GWPlayer[3].handicap = p4_handicap;
    _ClearFlag(FLAG_ID_MAKE(1, 11));
    _SetFlag(FLAG_ID_MAKE(0, 11));
    _SetFlag(FLAG_ID_MAKE(1, 10));
}

/**
 * @brief Clear the global handle when the main board process is destroyed.
 */
static void DestroyMainFunc(void)
{
    boardMainProc = NULL;
}

#define CHECK_LAST5_TURN() ((s32)(GWSystem.max_turn-GWSystem.turn) < 5)

/**
 * @brief Run board creation, startup, player turns, minigame transitions, and end-of-game shutdown.
 */
static void MainFunc(void)
{
    s32 i;
    s32 fade_enable, turn_cont, fade_type;
    fade_enable = 0;
    turn_cont = 0;
    BoardPauseDisableSet(1);
    if(_CheckFlag(FLAG_ID_MAKE(0, 10))) {
        _ClearFlag(FLAG_ID_MAKE(0, 10));
        _SetFlag(FLAG_ID_MAKE(1, 16));
        BoardKill();
        HuPrcSleep(-1);
    }
    _ClearFlag(FLAG_ID_MAKE(0, 10));
    BoardTutorialInit();
    if(_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        BoardTutorialWorkSave();
        BoardTutorialPlayerInit();
        #if VERSION_PAL
        GWMessSpeedSet(3);
        #else
        GWMessSpeedSet(1);
        #endif
    }
    CreateBoard();
    if(!_CheckFlag(FLAG_ID_MAKE(1, 1))) {
        GWSystem.player_curr = -1;
        BoardStartExec();
        GWSystem.player_curr = 0;
        fade_enable = 1;
        _SetFlag(FLAG_ID_MAKE(1, 1));
    } else {
        fade_enable = 0;
    }
    if(ExecTurnStart()) {
        turn_cont = 1;
    }
    if(CHECK_LAST5_TURN() && GWSystem.player_curr == 0 && !turn_cont) {
        if(!_CheckFlag(FLAG_ID_MAKE(0, 8))) {
            BoardLast5Exec();
            _SetFlag(FLAG_ID_MAKE(0, 8));
        } else {
            BoardLast5GfxInit();
        }
    }
    if(!turn_cont) {
        fade_type = 1;
    } else {
        fade_type = 0;
    }
    _ClearFlag(FLAG_ID_MAKE(1, 28));
    do {
        BoardStatusShowSetAll(1);
        if(GWBoardGet() == BOARD_ID_MAIN6 && GWSystem.player_curr == 0 && !turn_cont && boardTurnFunc) {
            GWSystem.player_curr = -1;
            boardTurnFunc();
            GWSystem.player_curr = 0;
        }
        BoardMusStartBoard();
        for(i=GWSystem.player_curr; i<4; i++) {
            if(GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
                if(CHECK_LAST5_TURN() && i == 0 && !turn_cont) {
                    BoardLast5GfxInit();
                    _SetFlag(FLAG_ID_MAKE(0, 8));
                }
            }
            if(_CheckFlag(FLAG_ID_MAKE(1, 6))) {
                BoardPlayerTurnExec(i);
            } else {
                if(!turn_cont) {
                    BoardCameraMoveSet(0);
                    GWSystem.player_curr = i;
                    BoardCameraTargetPlayerSet(i);
                    BoardCameraMotionWait();
                    {
                        Vec pos;
                        BoardSpacePosGet(0, GWPlayer[i].space_curr, &pos);
                        BoardPlayerPosSetV(i, &pos);
                    }
                    while(WipeStatGet()) {
                        HuPrcVSleep();
                    }
                    if(!fade_enable) {
                        if(!fade_type) {
                            WipeCreate(WIPE_MODE_IN, WIPE_TYPE_CROSS, 30);
                            while(WipeStatGet()) {
                                HuPrcVSleep();
                            }
                        } else {
                            fade_type = 0;
                            WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 21);
                            while(WipeStatGet()) {
                                HuPrcVSleep();
                            }
                        }
                    } else {
                        fade_enable = 0;
                    }
                    BoardPlayerTurnExec(i);
                }
            }
            turn_cont = 0;
            if(_CheckFlag(FLAG_ID_MAKE(1, 11))) {
                BoardTutorialHookExec(9, 0);
            }
            if(i != 3 || _CheckFlag(FLAG_ID_MAKE(1, 11))) {
                WipeColorSet(255, 255, 255);
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_CROSS, 1);
                while(WipeStatGet()) {
                    HuPrcVSleep();
                }
                BoardPlayerMoveAwayStartCurr(GWPlayer[i].space_curr, 1);
                fade_type = 0;
            } else {
                if(GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
                    if(GWSystem.turn != GWSystem.max_turn) {
                        WipeColorSet(255, 255, 255);
                        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_CROSS, 1);
                        while(WipeStatGet()) {
                            HuPrcVSleep();
                        }
                        BoardPlayerMoveAwayStartCurr(GWPlayer[i].space_curr, 1);
                        fade_type = 0;
                    }
                } else {
                    WipeColorSet(0, 0, 0);
                    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 21);
                    while(WipeStatGet()) {
                        HuPrcVSleep();
                    }
                }
            }
            BoardPlayerPostTurnHookExec(i);
        }
        if(GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
            GWSystem.player_curr = 0;
            if(BoardTurnNext()) {
                BoardAudSeqFadeOut(0, 500);
                BoardKill();
                HuPrcEnd();
                HuPrcSleep(-1);
            }
        } else {
            _SetFlag(FLAG_ID_MAKE(1, 28));
            _SetFlag(FLAG_ID_MAKE(1, 14));
            BoardPauseDisableSet(1);
            _ClearFlag(FLAG_ID_MAKE(1, 9));
            if(_CheckFlag(FLAG_ID_MAKE(2, 0)) || _CheckFlag(FLAG_ID_MAKE(1, 11)) ) {
                for(i=0; i<4; i++) {
                    GWPlayer[i].color = 0;
                }
                GWSystem.player_curr = (GWSystem.player_curr+1)&3;
            } else {
                BoardMGSetupExec();
                HuPrcSleep(-1);
            }
        }
    } while(1);
}

/**
 * @brief Increment the turn counter after clearing player minigame colors.
 * @return 1 when the incremented turn exceeds max_turn; otherwise 0.
 */
s32 BoardTurnNext(void)
{
    s32 i;
    for(i=0; i<4; i++) {
        GWPlayer[i].color = 0;
    }
    GWSystem.turn++;
    if(GWSystem.turn > GWSystem.max_turn) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief Resume any pending board event and prepare the current player at turn start.
 * @return 1 when normal turn processing should continue; 0 when no resumable event exists or turn advancement was handled.
 */
static s32 ExecTurnStart(void)
{
    s32 player;
    s32 space;
    if(!BoardStartCheck()) {
        return 0;
    }
    player = GWSystem.player_curr;
    space = GWPlayer[player].space_curr;
    if(_CheckFlag(FLAG_ID_MAKE(1, 5))) {
        BoardFortuneExec(player, space);
        _ClearFlag(FLAG_ID_MAKE(1, 5));
    } else if(_CheckFlag(FLAG_ID_MAKE(1, 4))) {
        BoardMusStartBoard();
        BoardBattleExec(player, space);
        _ClearFlag(FLAG_ID_MAKE(1, 4));
    } else if(_CheckFlag(FLAG_ID_MAKE(1, 3))) {
        BoardBowserExec(player, space);
        _ClearFlag(FLAG_ID_MAKE(1, 3));
    } else if(_CheckFlag(FLAG_ID_MAKE(1, 2))) {
        s32 turn_end = 0;
        BoardCameraMoveSet(0);
        BoardCameraViewSet(2);
        BoardCameraMotionWait();
        turn_end = BoardTurnNext();
        if(turn_end) {
            BoardKill();
            HuPrcEnd();
        }
        _ClearFlag(FLAG_ID_MAKE(1, 2));
        return 0;
    } else if(_CheckFlag(FLAG_ID_MAKE(1, 6))) {
        boardTurnStartFunc(player, space);
        return 1;
    }
    BoardPlayerZoomRestore(player);
    return 1;
}

/**
 * @brief Select a destination overlay, fade board music, and begin board shutdown.
 * @param [in] overlay Overlay ID to call after board shutdown.
 */
void BoardNextOvlSet(OMOVL overlay)
{
    nextOvl = overlay;
    BoardAudSeqFadeOut(0, 1000);
    BoardKill();
}

/**
 * @brief Check for any flag indicating a board event must resume at startup.
 * @return 1 when a resumable board-event flag is set; otherwise 0.
 */
s32 BoardStartCheck(void)
{
    if(_CheckFlag(FLAG_ID_MAKE(1, 2)) || _CheckFlag(FLAG_ID_MAKE(1, 3)) || _CheckFlag(FLAG_ID_MAKE(1, 4)) || _CheckFlag(FLAG_ID_MAKE(1, 5)) || _CheckFlag(FLAG_ID_MAKE(1, 6))) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief Load and initialize common board data, models, spaces, players, camera, UI, and hooks.
 */
static void CreateBoard(void)
{
    s32 guest_status;

    GWSystem.mg_next = -1;
    if(!GWGameStat.customPackEnable && GWMGListGet() == 2) {
        GWMGListSet(0);
    }
    GWMessSpeedSet(GWMessSpeedGet());
    if(GWMGExplainGet()) {
        _SetFlag(FLAG_ID_MAKE(0, 11));
    } else {
        _ClearFlag(FLAG_ID_MAKE(0, 11));
    }
    if(!BoardStartCheck()) {
        _ClearFlag(FLAG_ID_MAKE(1, 2));
        _ClearFlag(FLAG_ID_MAKE(1, 3));
        _ClearFlag(FLAG_ID_MAKE(1, 4));
        _ClearFlag(FLAG_ID_MAKE(1, 5));
        _ClearFlag(FLAG_ID_MAKE(1, 6));
    }
    _ClearFlag(FLAG_ID_MAKE(1, 8));
    if(!BoardStartCheck()) {
        GWSystem.lucky_value = 1;
    }
    guest_status = BoardDataDirReadAsync(DATADIR_BGUEST);
    if(guest_status != -1) {
        BoardDataAsyncWait(guest_status);
    }
    BoardAudSeqClear();
    BoardModelInit();
    BoardRandInit();
    BoardWinInit();
    BoardPlayerModelInit();
    createFunc();
    BoardLightSetExec();
    BoardLotteryInit();
    BoardShopInit();
    BoardBooHouseCreate();
    BoardCameraInit();
    BoardStatusCreate();
    CharModelDataClose(-1);
    BoardPlayerInit();
    if(GWSystem.last5_effect == 2) {
        BoardSpaceTypeForce(2, 3);
    }
    if(GWSystem.last5_effect == 3) {
        BoardSpaceTypeForce(2, 7);
    }
    BoardCameraMoveSet(0);
    BoardCameraTargetPlayerSet(0);
    BoardCameraViewSet(2);
    BoardCameraMotionWait();
    BoardTauntInit();
    _SetFlag(FLAG_ID_MAKE(1, 14));
    HuDataDirClose(DATADIR_BKOOPASUIT);
    HuDataDirClose(DATADIR_BBATTLE);
    HuDataDirClose(DATADIR_BKOOPA);
    HuDataDirClose(DATADIR_BKUJIYA);
    HuDataDirClose(DATADIR_BYOKODORI);
    HuDataDirClose(DATADIR_BPAUSE);
    HuDataDirClose(DATADIR_BLAST5);
    HuDataDirClose(DATADIR_EFFECT);
    _SetFlag(FLAG_ID_MAKE(1, 16));
}

/**
 * @brief Destroy common board subsystems and close board-specific data directories.
 */
static void DestroyBoard(void)
{
    s32 dir_table[] = {
        DATADIR_W01,
        DATADIR_W02,
        DATADIR_W03,
        DATADIR_W04,
        DATADIR_W05,
        DATADIR_W06,
        DATADIR_W10,
        DATADIR_W20,
        DATADIR_W21
    };
    BoardTauntKill();
    BoardAudSeqFadeOutAll();
    HuAudAllStop();
    BoardRollKill();
    BoardStatusKill();
    BoardBooHouseKill();
    BoardShopKill();
    BoardLotteryKill(); 
    BoardSpaceDestroy();
    BoardPlayerModelKill();
    if(destroyFunc) {
        destroyFunc();
    }
    BoardModelKillAll();
    BoardWinKillAll();
    HuDataDirClose(dir_table[GWBoardGet()]);
    HuDataDirClose(DATADIR_EFFECT);
    HuDataDirClose(DATADIR_BGUEST);
    HuDataDirClose(DATADIR_BKOOPASUIT);
    HuDataDirClose(DATADIR_BBATTLE);
    HuDataDirClose(DATADIR_BKOOPA);
    HuDataDirClose(DATADIR_BKUJIYA);
    HuDataDirClose(DATADIR_BYOKODORI);
    HuDataDirClose(DATADIR_BOARD);
    createFunc = destroyFunc = NULL;
}

/**
 * @brief Install board-specific callbacks for applying and resetting lighting.
 * @param [in] set Callback that applies board lighting.
 * @param [in] reset Callback that restores board lighting.
 */
void BoardLightHookSet(BoardLightHook set, BoardLightHook reset)
{
    boardLightSetHook = set;
    boardLightResetHook = reset;
}

/**
 * @brief Invoke the installed board lighting callback when present.
 */
void BoardLightSetExec(void)
{
    Hu3DBGColorSet(0, 0, 0);
    if(boardLightSetHook) {
        boardLightSetHook();
    }
}

/**
 * @brief Invoke the installed board lighting-reset callback when present.
 */
void BoardLightResetExec(void)
{
    if(boardLightResetHook) {
        boardLightResetHook();
    }
    Hu3DBGColorSet(0, 0, 0);
    Hu3DFogClear();
    Hu3DReflectNoSet(0);
}

//Fixes order of boardCamera and cameraBackup
/**
 * @brief Return the singleton board-camera state.
 * @return Pointer to the singleton board-camera state; never NULL.
 */
static BoardCameraData *BoardCameraGet(void)
{
    return &boardCamera;
}

/**
 * @brief Copy the current board-camera state into the backup slot.
 */
void BoardCameraBackup(void)
{
    cameraUseBackup = TRUE;
    memcpy(&cameraBackup, &boardCamera, sizeof(BoardCameraData));
}

/**
 * @brief Restore the board-camera state saved by BoardCameraBackup.
 */
void BoardCameraRestore(void)
{
    if(cameraUseBackup) {
        memcpy(&boardCamera, &cameraBackup, sizeof(BoardCameraData));
        cameraUseBackup = FALSE;
    }
}

/**
 * @brief Set the board camera's viewport and scissor rectangle in pixels.
 * @param [in] x Left scissor coordinate in pixels.
 * @param [in] y Top scissor coordinate in pixels.
 * @param [in] w Scissor width in pixels.
 * @param [in] h Scissor height in pixels.
 */
void BoardCameraScissorSet(s32 x, s32 y, s32 w, s32 h)
{
    BoardCameraData *camera = &boardCamera;
    Hu3DCameraScissorSet(camera->mask, x, y, w, h);
}

/**
 * @brief Configure one of the predefined board camera rotation and zoom presets.
 * @param [in] type Camera preset index selecting predefined rotation and zoom values.
 */
void BoardCameraViewSet(s32 type)
{
    BoardCameraData *camera = &boardCamera;
    BoardFocusData *focus;
    float size;
    if(!cameraObj) {
        return;
    }
    focus = &camera->focus;
    if(type == 0) {
        focus->view_type = 0;
        return;
    }
    OSs16tof32(&camViewTbl[type].fov, &focus->fov_end);
    focus->fov_start = camera->fov;
    OSs16tof32(&camViewTbl[type].zoom, &focus->zoom_end);
    focus->zoom_start = camera->zoom;
    OSs16tof32(&camViewTbl[type].x_rot, &focus->rot_end.x);
    focus->rot_end.y = 0;
    focus->rot_end.z = 0;
    focus->rot_start = camera->rot;
    focus->target_start = camera->target;
    if(BoardPlayerSizeGet(GWSystem.player_curr) == 2 || GWPlayer[GWSystem.player_curr].bowser_suit) {
        focus->zoom_end += 400.0f;
        size = 2.5f;
    } else {
        size = 1.0f;
    }
    BoardCameraTargetModelSet(BoardPlayerModelGet(GWSystem.player_curr));
    BoardPlayerPosGet(GWSystem.player_curr, &focus->target_end);
    BoardCameraOffsetSet(0.0f, 100.0f*size, 0.0f);
    focus->target_end.y += 100.0f*size;
    _SetFlag(FLAG_ID_MAKE(1, 21));
    focus->view_type = type;
    focus->time = 0;
    if(camera->moving) {
        focus->max_time = 21;
    } else {
        focus->max_time = 1;
    }
}

/**
 * @brief Copy the current calculated camera position.
 * @param [out] dst Caller-owned vector receiving the current camera position in world coordinates.
 * @return 0 after copying the position, or -1 if dst is NULL.
 */
s32 BoardCameraPosGet(Vec *dst)
{
    BoardCameraData *camera;
    if(!dst) {
        return -1;
    }
    camera = &boardCamera;
    *dst = camera->pos;
    return 0;
}

/**
 * @brief Copy the current camera target position.
 * @param [out] dst Caller-owned vector receiving the current camera target in world coordinates.
 * @return 0 after copying the target, or -1 if dst is NULL.
 */
s32 BoardCameraTargetGet(Vec *dst)
{
    BoardCameraData *camera;
    if(!dst) {
        return -1;
    }
    camera = &boardCamera;
    *dst = camera->target;
    return 0;
}

/**
 * @brief Copy the current camera Euler rotation.
 * @param [out] dst Caller-owned vector receiving the current Euler rotation in degrees.
 * @return 0 after copying the rotation, or -1 if dst is NULL.
 */
s32 BoardCameraRotGet(Vec *dst)
{
    BoardCameraData *camera;
    if(!dst) {
        return -1;
    }
    camera = &boardCamera;
    *dst = camera->rot;
    return 0;
}

/**
 * @brief Get the current camera zoom distance.
 * @return Current camera zoom distance.
 */
float BoardCameraZoomGet()
{
    BoardCameraData *camera;
    camera = &boardCamera;
    return camera->zoom;
}

/**
 * @brief Calculate a normalized 3D direction from the camera position to its target.
 * @param [out] dst Caller-owned vector receiving the normalized camera-to-target direction, or the zero vector when both positions coincide.
 * @return -1 when dst is NULL; otherwise 0 after writing the normalized or zero direction.
 */
s32 BoardCameraDirGet(Vec *dst)
{
    BoardCameraData *camera;
    if(!dst) {
        return -1;
    }
    camera = &boardCamera;
    if(!camera) {
        return -1;
    }
    VECSubtract(&camera->target, &camera->pos, dst);
    if(dst->x != 0 || dst->y != 0 || dst->z != 0) {
        VECNormalize(dst, dst);
    }
    return 0;
}

/**
 * @brief Calculate a normalized 3D direction from the camera position to a world point.
 * @param [in] point World-space point.
 * @param [out] dst Caller-owned vector receiving the normalized camera-to-point direction, or the zero vector when the positions coincide.
 * @return -1 when point or dst is NULL; otherwise 0 after writing the normalized or zero direction.
 */
s32 BoardCameraPointDirGet(Vec *point, Vec *dst)
{
    BoardCameraData *camera;
    if(!dst || !point) {
        return -1;
    }
    camera = &boardCamera;
    if(!camera) {
        return -1;
    }
    VECSubtract(point, &camera->pos, dst);
    if(dst->x != 0 || dst->y != 0 || dst->z != 0) {
        VECNormalize(dst, dst);
    }
    return 0;
}

/**
 * @brief Set the camera visibility mask used by the board camera.
 * @param [in] mask Engine camera visibility mask used for board rendering.
 */
void BoardCameraMaskSet(u16 mask)
{
    BoardCameraData *camera = &boardCamera;
    if(!camera) {
        return;
    }
    camera->mask = mask;
}

/**
 * @brief Enable or disable per-frame camera target and motion updates.
 * @param [in] move Nonzero to enable camera target tracking; zero freezes it.
 */
void BoardCameraMoveSet(s32 move)
{
    BoardCameraData *camera = &boardCamera;
    if(!camera) {
        return;
    }
    if(!move) {
        camera->moving = 0;
    } else {
        camera->moving = 1;
    }
}

/**
 * @brief Set the camera target offset in world units.
 * @param [in] x World-space X offset from the tracked target.
 * @param [in] y World-space Y offset from the tracked target.
 * @param [in] z World-space Z offset from the tracked target.
 */
void BoardCameraOffsetSet(float x, float y, float z)
{
    BoardCameraData *camera = &boardCamera;
    if(!camera) {
        return;
    }
    camera->offset.x = x;
    camera->offset.y = y;
    camera->offset.z = z;
}

/**
 * @brief Track a player's position as the camera target.
 * @param [in] player Player whose board model becomes the tracked camera target; an invalid player clears model tracking.
 */
void BoardCameraTargetPlayerSet(s32 player)
{
    PlayerState *player_ptr = BoardPlayerGet(player);
    if(!player_ptr) {
        BoardCameraTargetModelSet(-1);
        return;
    }
    BoardCameraTargetModelSet(BoardPlayerModelGet(player));
    BoardCameraOffsetSet(0, 100, 0);
}

/**
 * @brief Track a board model as the camera target.
 * @param [in] model Board-model wrapper to track as the camera target, or -1 to clear model tracking.
 */
void BoardCameraTargetModelSet(s16 model)
{
    BoardCameraData *camera = &boardCamera;
    if(!camera) {
        return;
    }
    camera->target_mdl = model;
    camera->target_space = -1;
    camera->offset.x = camera->offset.y = camera->offset.z = 0;
}

/**
 * @brief Track a board space as the camera target.
 * @param [in] space One-based board space whose position becomes the camera target, or -1 to clear space tracking.
 */
void BoardCameraTargetSpaceSet(s32 space)
{
    BoardCameraData *camera = &boardCamera;
    if(!camera) {
        return;
    }
    camera->target_mdl = -1;
    camera->target_space = space;
    camera->offset.x = camera->offset.y = camera->offset.z = 0;
}

/**
 * @brief Install or clear a callback that overrides camera-position calculation.
 * @param [in] func Callback to install, or NULL to clear it.
 */
void BoardCameraPosCalcFuncSet(BoardCameraPosCalcFunc func)
{
    BoardCameraData *camera = &boardCamera;
    if(!camera) {
        return;
    }
    camera->pos_calc = func;
}

/**
 * @brief Start a camera quake with a duration and displacement strength.
 * @param [in] duration Quake duration in frames.
 * @param [in] strength Quake displacement magnitude.
 */
void BoardCameraQuakeSet(s32 duration, float strength)
{
    BoardCameraData *camera = &boardCamera;
    if(!camera) {
        return;
    }
    camera->quaking = 1;
    camera->quake_strength = strength;
    camera->quake_timer = duration;
}

/**
 * @brief Stop the active camera quake and clear its offset.
 */
void BoardCameraQuakeReset()
{
    BoardCameraData *camera = &boardCamera;
    camera->quaking = 0;
    camera->quake_strength = 0;
    camera->quake_timer = 0;
}

/**
 * @brief Set an explicit camera target position.
 * @param [in] x Target world-space X coordinate.
 * @param [in] y Target world-space Y coordinate.
 * @param [in] z Target world-space Z coordinate.
 */
void BoardCameraTargetSet(float x, float y, float z)
{
    BoardCameraData *camera = &boardCamera;
    camera->target.x = x;
    camera->target.y = y;
    camera->target.z = z;
}

/**
 * @brief Set an explicit camera eye position.
 * @param [in] x Camera-eye world-space X coordinate.
 * @param [in] y Camera-eye world-space Y coordinate.
 * @param [in] z Camera-eye world-space Z coordinate.
 */
void BoardCameraPosSet(float x, float y, float z)
{
    BoardCameraData *camera = &boardCamera;
    camera->pos.x = x;
    camera->pos.y = y;
    camera->pos.z = z;
}

/**
 * @brief Set camera X rotation and zoom while preserving Y rotation.
 * @param [in] zoom Camera zoom distance.
 * @param [in] x_rot Camera X rotation in degrees.
 */
void BoardCameraXRotZoomSet(float zoom, float x_rot)
{
    BoardCameraData *camera = &boardCamera;
    camera->zoom = zoom;
    camera->rot.x = x_rot;
}

/**
 * @brief Set camera zoom distance.
 * @param [in] zoom Camera zoom distance.
 */
void BoardCameraZoomSet(float zoom)
{
    BoardCameraData *camera = &boardCamera;
    camera->zoom = zoom;
}

/**
 * @brief Set camera X and Y rotation in degrees.
 * @param [in] x X-axis Euler rotation in degrees.
 * @param [in] y Y-axis Euler rotation in degrees.
 */
void BoardCameraRotSet(float x, float y)
{
    BoardCameraData *camera = &boardCamera;
    camera->rot.x = x;
    camera->rot.y = y;
}

/**
 * @brief Set near and far clipping distances.
 * @param [in] near Near clipping distance.
 * @param [in] far Far clipping distance.
 */
void BoardCameraNearFarSet(float near, float far)
{
    BoardCameraData *camera = &boardCamera;
    camera->near = near;
    camera->far = far;
}

/**
 * @brief Copy the current near and far clipping distances.
 * @param [out] near Near clipping distance.
 * @param [out] far Far clipping distance.
 */
void BoardCameraNearFarGet(float *near, float *far)
{
    BoardCameraData *camera = &boardCamera;
    if(near) {
        *near = camera->near;
    }
    if(far) {
        *far = camera->far;
    }
}

/**
 * @brief Start a 21-frame camera interpolation toward model, rotation, zoom, and field-of-view targets.
 * @param [in] model_target Board-model handle to track, or -1 to retain the current target mode.
 * @param [in] rot_target Target camera Euler rotation in degrees; NULL keeps the current rotation.
 * @param [in] zoom_target Target camera zoom.
 * @param [in] fov_target Target vertical field of view in degrees.
 */
void BoardCameraMotionStart(s16 model_target, Vec *rot_target, float zoom_target, float fov_target)
{
    BoardCameraMotionStartEx(model_target, rot_target, NULL, zoom_target, fov_target, 21);
}

/**
 * @brief Start a timed camera interpolation toward model, rotation, offset, zoom, and field-of-view targets.
 * @param [in] model_target Board-model handle to track, or -1 to retain the current target mode.
 * @param [in] rot_target Target camera Euler rotation in degrees; NULL keeps the current rotation.
 * @param [in] offset_end Target camera offset; NULL uses zero offset.
 * @param [in] zoom_target Target camera zoom.
 * @param [in] fov_target Target vertical field of view in degrees.
 * @param [in] max_time Interpolation duration in frames.
 */
void BoardCameraMotionStartEx(s16 model_target, Vec *rot_target, Vec *offset_end, float zoom_target, float fov_target, s16 max_time)
{
    BoardCameraData *camera = &boardCamera;
    BoardFocusData *focus;
    if(!cameraObj) {
        return;
    }
    focus = &camera->focus;
    focus->fov_start = camera->fov;
    focus->zoom_start = camera->zoom;
    focus->rot_start = camera->rot;
    focus->target_start = camera->target;
    if(fov_target == -1) {
        focus->fov_end = focus->fov_start;
    } else {
        focus->fov_end = fov_target;
    }
    if(zoom_target == -1) {
        focus->zoom_end = focus->zoom_start;
    } else {
        focus->zoom_end = zoom_target;
    }
    if(!rot_target) {
        focus->rot_end = focus->rot_start;
    } else {
        focus->rot_end = *rot_target;
    }
    if(model_target == -1) {
        focus->target_end = focus->target_start;
    } else {
        BoardCameraTargetModelSet(model_target);
        BoardModelPosGet(model_target, &focus->target_end);
    }
    if(offset_end) {
        BoardCameraOffsetSet(offset_end->x, offset_end->y, offset_end->z);
        VECAdd(offset_end, &focus->target_end, &focus->target_end);
    }
    _SetFlag(FLAG_ID_MAKE(1, 21));
    focus->view_type = 5;
    focus->time = 0;
    if(max_time < 0) {
        max_time = 1;
    }
    focus->max_time = max_time;
}

/**
 * @brief Set the board camera's vertical field of view in degrees.
 * @param [in] fov Vertical field of view in degrees.
 */
void BoardCameraFovSet(float fov)
{
    BoardCameraData *camera = &boardCamera;
    camera->fov = fov;
}

/**
 * @brief Test a world-space bounding sphere against the board camera frustum.
 * @param [in] point World-space point.
 * @param [in] radius Bounding-sphere radius in world units.
 * @return 1 when the bounding sphere intersects the view frustum; otherwise 0.
 */
s32 BoardCameraCullCheck(Vec *point, float radius)
{
    Vec dir;
    Vec pos;
    float dist;
    float dot;
    BoardCameraData *camera = &boardCamera;
    if(!camera->hide_all) {
        return 0;
    }
    BoardCameraDirGet(&dir);
    VECSubtract(point, &camera->pos, &pos);
    dist = sqrtf((pos.x*pos.x)+(pos.y*pos.y)+(pos.z*pos.z));
    if(25000 < dist-(radius*2.0f)) {
        return 0;
    }
    BoardCameraPointDirGet(point, &pos);
    dot = VECDotProduct(&dir, &pos);
    if(BOARD_FABS(dot) < cosd(camera->fov)) {
        return 0;
    } else {
        return 1;
    }
}

/**
 * @brief Check whether the active camera interpolation has completed.
 * @return 1 when no camera interpolation is active; otherwise 0.
 */
s32 BoardCameraMotionIsDone(void)
{
    return (_CheckFlag(FLAG_ID_MAKE(1, 21))) ? 0 : 1;
}

/**
 * @brief Yield until the active camera interpolation completes.
 */
void BoardCameraMotionWait(void)
{
    while(!BoardCameraMotionIsDone()) {
        HuPrcVSleep();
    }
    HuPrcVSleep();
}

/**
 * @brief Initialize camera state, engine camera parameters, and the per-frame camera object.
 */
void BoardCameraInit(void)
{
    BoardCameraData *camera;
    cameraUseBackup = FALSE;
    memset(&cameraBackup, 0, sizeof(BoardCameraData));
    memset(&boardCamera, 0, sizeof(BoardCameraData));
    camera = &boardCamera;
    camera->fov = 25;
    camera->near = 100;
    camera->far = 13000;
    camera->aspect = HU_DISP_ASPECT;
    camera->viewport_x = 0;
    camera->viewport_y = 0;
    camera->viewport_h = 480;
    camera->viewport_w = 640;
    camera->viewport_near = 0;
    camera->viewport_far = 1;
    camera->up.y = 1;
    camera->moving = 0;
    camera->quaking = 0;
    camera->quake_timer = 0;
    camera->pos.x = 0;
    camera->pos.y = 5000;
    camera->pos.z = 5000;
    camera->offset.x = 0;
    camera->offset.y = 0;
    camera->offset.z = 0;
    camera->pos_calc = NULL;
    BoardSpaceFlagPosGet(0, 0x80000000, &camera->target);
    camera->rot.x = camViewTbl[1].x_rot;
    camera->zoom = camViewTbl[1].zoom;
    camera->hide_all = 1;
    camera->mask = 1;
    Hu3DCameraCreate(1);
    Hu3DCameraScissorSet(1, camera->viewport_x, camera->viewport_y, camera->viewport_w, camera->viewport_h);
    Hu3DCameraScissorSet(2, 0, 0, 0, 0);
    Hu3DCameraPerspectiveSet(2, -1, 100, 13000, 1.2);
    cameraObj = omAddObjEx(boardObjMan, 32258, 0, 0, -1, UpdateCamera);
}

/**
 * @brief Recalculate camera position, target, and view matrix for the current frame.
 */
static inline void CalcCameraView(void)
{
    BoardCameraData *camera = &boardCamera;
    CalcCameraTarget(camera);
    CalcCameraPos(camera);
}

/**
 * @brief Update camera motion, target tracking, quake offsets, and the engine camera.
 * @param [in] object Camera-manager object deleted when board shutdown begins.
 */
static void UpdateCamera(OMOBJ *object)
{
    BoardCameraData *camera;
    Vec *target;
    float x, y, z;
    if(BoardIsKill()) {
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    CalcCameraView();
    camera = &boardCamera;
    Hu3DCameraPerspectiveSet(camera->mask, camera->fov, camera->near, camera->far, camera->aspect);
    Hu3DCameraViewportSet(camera->mask, camera->viewport_x, camera->viewport_y, camera->viewport_w, camera->viewport_h, camera->viewport_near, camera->viewport_far);
    target = &camera->target;
    if(camera->pos_calc) {
        camera->pos_calc(camera);
    } else {

        x = camera->rot.x;
        y = camera->rot.y;
        camera->pos.x = target->x+(sind(y)*cosd(x)*camera->zoom);
        camera->pos.y = target->y+(-sind(x)*camera->zoom);
        camera->pos.z = target->z+(cosd(y)*cosd(x)*camera->zoom);
        camera->up.x = sind(y)*sind(x);
        camera->up.y = cosd(x);
        camera->up.z = cosd(y)*sind(x);
        if(camera->quaking) {
            x = BoardRandFloat();
            y = BoardRandFloat();
            z = BoardRandFloat();
            camera->pos.x += (x-0.5f)*camera->quake_strength;
            camera->pos.y += (y-0.5f)*camera->quake_strength;
            camera->pos.z += (z-0.5f)*camera->quake_strength;
            if(--camera->quake_timer <= 0) {
                camera->quaking = 0;
                camera->quake_timer = 0;
                camera->quake_strength = 0;
            }
        }
    }
    Hu3DCameraPosSetV(camera->mask, &camera->pos, &camera->up, target);
}

/**
 * @brief Resolve the camera target from a player, model, space, or explicit target.
 * @param [in,out] camera Singleton camera state whose target is resolved and updated.
 */
static void CalcCameraTarget(BoardCameraData *camera)
{
    Vec offset;
    Vec pos = { 0, 0, 0 };
    BoardFocusData *focus = &camera->focus;
    if(camera->target_mdl != -1) {
        BoardModelPosGet(camera->target_mdl, &pos);
    } else {
        if(camera->target_space != -1) {
            BoardSpacePosGet(0, camera->target_space, &pos);
        } else {
            return;
        }
    }
    VECAdd(&camera->offset, &pos, &pos);
    VECSubtract(&pos, &camera->target, &offset);
    if(camera->moving) {
        VECScale(&offset, &offset, 0.15f);
    }
    VECAdd(&offset, &camera->target, &camera->target);
}

/**
 * @def CAM_LERP
 * @brief Linearly interpolate one scalar camera property over the active focus interval.
 * @param t Current interpolation frame.
 * @param x1 Property value at the start of the interval.
 * @param x2 Property value at the end of the interval.
 * @param out Lvalue receiving the interpolated property.
 */
#define CAM_LERP(t, x1, x2, out) \
{ \
    float result; \
    float offset; \
    float unit_scale; \
    float lerp_t; \
    lerp_x2 = x2; \
    lerp_x1 = x1; \
    lerp_t = t; \
    if(lerp_t2 == lerp_t1) { \
        result = lerp_x2; \
    } else { \
        unit_scale = (lerp_x1-lerp_x2)/((float)lerp_t1-(float)lerp_t2); \
        offset = lerp_x2-(unit_scale*lerp_t2); \
        result = offset+(unit_scale*lerp_t); \
    } \
    out = result; \
}

/**
 * @def CAM_LERP_VEC
 * @brief Apply CAM_LERP independently to the X, Y, and Z components of a camera vector.
 * @param t Current interpolation frame.
 * @param x1 Vector value at the start of the interval.
 * @param x2 Vector value at the end of the interval.
 * @param out Vector lvalue receiving the interpolated components.
 */
#define CAM_LERP_VEC(t, x1, x2, out) \
CAM_LERP(t, (x1).x, (x2).x, (out).x) \
CAM_LERP(t, (x1).y, (x2).y, (out).y) \
CAM_LERP(t, (x1).z, (x2).z, (out).z)

/**
 * @brief Interpolate active focus values and end the camera motion after its final frame.
 * @param [in,out] camera Camera state whose focus timer, zoom, rotation, field of view, and target are updated.
 */
static void CalcCameraPos(BoardCameraData *camera)
{
    //These variables must be volatile to match
    volatile u32 lerp_t1, lerp_t2;
    volatile float lerp_x1, lerp_x2;
    float time;

    BoardFocusData *focus = &camera->focus;
    if(focus->time > focus->max_time) {
        focus->view_type = 0;
        _ClearFlag(FLAG_ID_MAKE(1, 21));
        return;
    }
    OSs16tof32(&focus->time, &time);
    focus->time++;
    lerp_t2 = focus->max_time;
    lerp_t1 = 0;
    CAM_LERP(time, focus->zoom_start, focus->zoom_end, camera->zoom)
    CAM_LERP_VEC(time, focus->rot_start, focus->rot_end, camera->rot)
    CAM_LERP(time, focus->fov_start, focus->fov_end, camera->fov)
    CAM_LERP_VEC(time, focus->target_start, focus->target_end, camera->target)
}

/**
 * @brief Set or clear the in-board minigame completion flag.
 * @param [in] flag Nonzero marks the minigame complete; zero clears the completion state.
 */
void BoardMGDoneFlagSet(s32 flag)
{
    if(flag) {
        _SetFlag(FLAG_ID_MAKE(1, 20));
    } else {
        _ClearFlag(FLAG_ID_MAKE(1, 20));
    }
}

/**
 * @brief Check the board-minigame completion flag.
 * @return 1 when the in-board minigame is complete; otherwise 0.
 */
s32 BoardMGDoneFlagGet()
{
    return (_CheckFlag(FLAG_ID_MAKE(1, 20))) ? 1 : 0;
}

/**
 * @brief Mark a board minigame complete and wait for its object to terminate.
 */
void BoardMGExit(void)
{
    s32 player = GWSystem.player_curr;
    BoardPlayerMoveToAsync(player, GWPlayer[GWSystem.player_curr].space_curr);
    BoardCameraTargetPlayerSet(player);
    BoardCameraMoveSet(1);
    BoardCameraViewSet(1);
    _ClearFlag(FLAG_ID_MAKE(1, 19));
}

/**
 * @brief Clear the tracked board-minigame object during destruction.
 * @param [in] object In-board minigame manager object deleted after completion is cleared.
 */
static void KillBoardMG(OMOBJ *object)
{
    if(!BoardMGDoneFlagGet()) {
        BoardEventFlagReset();
        omDelObjEx(HuPrcCurrentGet(), object);
    }
}

/**
 * @brief Wait for the board-minigame completion flag, then delete its manager object.
 * @param [in,out] object In-board minigame manager object switched to its cleanup callback after player movement and camera motion finish.
 */
static void ExecBoardMG(OMOBJ *object)
{
    if(_CheckFlag(FLAG_ID_MAKE(1, 19))) {
        return;
    }
    if(GWPlayer[GWSystem.player_curr].moving == 0) {
        BoardPlayerMotionShiftSet(GWSystem.player_curr, 1, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
        if(!_CheckFlag(FLAG_ID_MAKE(1, 21))) {
            _SetFlag(FLAG_ID_MAKE(1, 20));
            object->objFunc = KillBoardMG;
        }
    }
}

/**
 * @brief Create the object that tracks completion of an in-board minigame.
 * @param [in] minigame_param Value retained in GWSystem.unk_38 while the in-board minigame runs; its higher-level meaning is not established here.
 */
void BoardMGCreate(s32 minigame_param)
{
    _SetFlag(FLAG_ID_MAKE(1, 19));
    _ClearFlag(FLAG_ID_MAKE(1, 20));
    GWSystem.unk_38 = minigame_param;
    omAddObjEx(boardObjMan, 0x201, 0, 0, -1, ExecBoardMG);
    BoardEventFlagSet();
    BoardSpaceWalkMiniEventExec();
}

/**
 * @brief Mark a board event as active.
 */
void BoardEventFlagSet(void)
{
    _SetFlag(FLAG_ID_MAKE(1, 18));
}

/**
 * @brief Clear the board-event-active flag.
 */
void BoardEventFlagReset(void)
{
    _ClearFlag(FLAG_ID_MAKE(1, 18));
}

/**
 * @brief Check whether a board event is active.
 * @return 1 when a board event is active; otherwise 0.
 */
s32 BoardEventFlagGet(void)
{
    return _CheckFlag(FLAG_ID_MAKE(1, 18)) ? 1 : 0;
}

/**
 * @brief Build a look-at matrix from eye, up, and target vectors.
 * @param [in] dest Destination look-at matrix.
 * @param [in] eye Camera eye position.
 * @param [in] up Camera up vector.
 * @param [in] target World-space point the camera looks toward.
 */
void BoardMTXCalcLookAt(Mtx dest, Vec *eye, Vec *up, Vec *target)
{
    Vec f, u, s;
    f.x = eye->x-target->x;
    f.y = eye->y-target->y;
    f.z = eye->z-target->z;
    VECNormalize(&f, &f);
    VECCrossProduct(up, &f, &u);
    VECNormalize(&u, &u);
    VECCrossProduct(&f, &u, &s);
    dest[0][0] = u.x;
    dest[0][1] = u.y;
    dest[0][2] = u.z;
    dest[0][3] = 0;
    dest[1][0] = s.x;
    dest[1][1] = s.y;
    dest[1][2] = s.z;
    dest[1][3] = 0;
    dest[2][0] = f.x;
    dest[2][1] = f.y;
    dest[2][2] = f.z;
    dest[2][3] = 0;
}

/**
 * @brief Approximate the magnitude of arcsine in radians for an input in [-1, 1].
 * @param [in] value Input ratio; the routine evaluates its absolute value.
 * @return Nonnegative arcsine magnitude in radians, or 0 for inputs outside [-1, 1].
 */
float BoardArcSin(float value)
{
    float result;
    s32 sign;
    if(value < 0) {
        sign = 1;
        value = BOARD_FABS(value);
    } else {
        sign = 0;
    }
    if(value > 1.0f) {
        return 0;
    }
    if(value <= (float)(M_PI/2)) {
        result = atanf(value/sqrtf(1-(value*value)));
    } else {
        result = (float)(M_PI/2)-atanf(sqrtf(1-(value*value))/value);
    }
    if(sign) {
        result = BOARD_FABS(result);
    }
    return result;
}

/**
 * @brief Compute pi/2 minus BoardArcSin's nonnegative angle magnitude.
 * @param [in] value Cosine ratio expected in [-1, 1].
 * @return pi/2 minus BoardArcSin(value), or 0 for inputs outside [-1, 1].
 */
float BoardArcCos(float value)
{
    if(BOARD_FABS(value) > 1) {
        return 0;
    }
    return (float)(M_PI/2)-BoardArcSin(value);
}

/**
 * @brief Seed the board pseudorandom generator from the system tick.
 */
void BoardRandInit(void)
{
    boardRandSeed = OSGetTime();
}

/**
 * @brief Advance and return the board's linear-congruential pseudorandom state.
 * @return Updated 32-bit pseudorandom state.
 */
u32 BoardRand(void)
{
    boardRandSeed = (boardRandSeed*0x19660D)+(0x3C6EF35F);
    return boardRandSeed;
}

/**
 * @brief Return the nonnegative board random state modulo a bound.
 * @param [in] value Nonzero upper bound for the modulo result.
 * @return Random integer in [0, value); value must be nonzero.
 */
u32 BoardRandMod(u32 value)
{
    return (BoardRand() & 0x7FFFFFFF)%value;
}

/**
 * @brief Return a board pseudorandom fraction in the half-open interval [0, 1).
 * @return Random fraction in [0, 1).
 */
float BoardRandFloat(void)
{
    float value;
    *((u32 *)&value) = (BoardRand() & 0x7FFFFF)|0x3F800000; //Generate float from 1.0f to 2.0f
    return value-1.0f;
}

/**
 * @brief Calculate Euclidean distance between two positions in the XZ plane.
 * @param [in] vec1 First world position; only X and Z are read.
 * @param [in] vec2 Second world position; only X and Z are read.
 * @return XZ-plane distance in world units.
 */
float BoardVecDistXZCalc(Vec *vec1, Vec *vec2)
{
    float dx = vec1->x-vec2->x;
    float dz = vec1->z-vec2->z;
    return sqrtf((dx*dx)+(dz*dz));
}

/**
 * @brief Check whether the XZ-plane distance between two positions is within a maximum.
 * @param [in] vec1 First world position; only X and Z are read.
 * @param [in] vec2 Second world position; only X and Z are read.
 * @param [in] max_dist Maximum XZ distance in world units.
 * @return 1 when distance is less than or equal to max_dist; otherwise 0.
 */
s32 BoardVecMaxDistXZCheck(Vec *vec1, Vec *vec2, float max_dist)
{
    float dz = vec1->z-vec2->z;
    float dx = vec1->x-vec2->x;
    float dist = sqrtf((dx*dx)+(dz*dz));
    if(dist <= max_dist) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief Normalize each component of an Euler-angle vector to [-180, 180].
 * @param [in,out] vec1 Euler-angle vector normalized in place, component by component.
 */
void BoardDAngleCalcVec(Vec *vec1)
{
    s32 i;
    float *data = &vec1->x;
    for(i=0; i<3; i++) {
        while(*data > 180.0f) {
            *data -= 360.0f;
        }
        while(*data < -180.0f) {
            *data += 360.0f;
        }
        data++;
    }
}

/**
 * @brief Normalize an angle in degrees to [-180, 180].
 * @param [in] value Angle in degrees to normalize.
 * @return Equivalent angle in degrees within [-180, 180].
 */
float BoardDAngleCalc(float value)
{
    while(value > 180.0f) {
        value -= 360.0f;
    }
    while(value < -180.0f) {
        value += 360.0f;
    }
    return value;
}

/**
 * @brief Step an angle toward a target through the shortest signed direction.
 * @param [in,out] value Angle in degrees advanced in place and normalized to [-180, 180].
 * @param [in] min Target angle in degrees, adjusted by one revolution to select the shortest direction.
 * @param [in] range Positive angular step in degrees.
 * @return 1 when the target is reached on this step; otherwise 0.
 */
s32 BoardDAngleCalcRange(float *value, float min, float range)
{
    float diff = min-(*value);
    if(diff >= 180.0f) {
        min -= 360.0f;
    }
    if(diff <= -180.0f) {
        min += 360.0f;
    }
    if(min > *value) {
        *value += range;
        if(*value >= min) {
            *value = BoardDAngleCalc(min);
            return 1;
        }
    } else {
        *value -= range;
        if(*value <= min) {
            *value = BoardDAngleCalc(min);
            return 1;
        }
    }
    *value = BoardDAngleCalc(*value);
    return 0;
}

/**
 * @brief Check whether the 3D distance between two positions is below a minimum.
 * @param [in] vec1 First world position.
 * @param [in] vec2 Second world position.
 * @param [in] min_dist Minimum 3D distance in world units.
 * @return 1 when distance is less than min_dist; otherwise 0.
 */
s32 BoardVecMinDistCheck(Vec *vec1, Vec *vec2, float min_dist)
{
    Vec position_delta;
    VECSubtract(vec1, vec2, &position_delta);
    if(VECSquareMag(&position_delta) >= (min_dist*min_dist)) {
        return 0;
    } else {
        return 1;
    }
}

typedef struct filter_work {
    struct {
        u8 kill : 1;
        u8 paused : 1;
    };
    u8 max_alpha;
    s16 time;
    s16 len;
    s16 model;
    GXColor color;
    float speed;
} FilterWork;

static void UpdateFilter(OMOBJ *object);
static void DrawFilter(HU3DMODEL *model, Mtx matrix);

/**
 * @brief Resume the screen filter with a linear fade to transparent.
 * @param [in] len Fade duration in frames; values below one are clamped to one.
 */
void BoardFilterFadeOut(s16 len)
{
    FilterWork *work;
    float speed;
    if(!filterObj) {
        return;
    }
    if(len <= 0) {
        len = 1;
    }
    work = omObjGetWork(filterObj, FilterWork);
    work->len = len;
    OSs16tof32(&len, &speed);
    work->speed = -(work->color.a)/speed;
    work->paused = 0;
    work->time = work->len;
}

/**
 * @brief Create a black screen filter and fade it to the requested alpha.
 * @param [in] len Fade-in duration in frames; values below one are clamped to one.
 * @param [in] max_alpha Target filter alpha from 0 to 255.
 */
void BoardFilterFadeInit(s16 len, u8 max_alpha)
{
    FilterWork *work;
    if(filterObj) {
        work = omObjGetWork(filterObj, FilterWork);
        work->kill = 1;
        while(filterObj) {
            HuPrcVSleep();
        }
    }
    filterObj = omAddObjEx(boardObjMan, 32000, 0, 0, -1, UpdateFilter);
    omSetStatBit(filterObj, OM_STAT_NOPAUSE|0x80);
    if(len <= 0) {
        len = 1;
    }
    work = omObjGetWork(filterObj, FilterWork);
    work->kill = 0;
    work->paused = 0;
    work->color.r = 0;
    work->color.g = 0;
    work->color.b = 0;
    work->color.a = 0;
    work->max_alpha = max_alpha;
    work->speed = (float)(max_alpha-work->color.a)/(float)len;
    work->time = len;
    work->len = len;
    work->model = Hu3DHookFuncCreate(DrawFilter);
    Hu3DModelLayerSet(work->model, 1);
}

/**
 * @brief Check whether the filter is absent or paused at its target alpha.
 * @return 1 when no filter exists or the active filter is paused; otherwise 0.
 */
s32 BoardFilterFadePauseCheck(void)
{
    FilterWork *work;
    if(!filterObj) {
        return 1;
    }
    work = omObjGetWork(filterObj, FilterWork);
    return (work->paused) ? 1 : 0;
}

/**
 * @brief Check whether the filter object has finished and been removed.
 * @return 1 when no filter object exists; otherwise 0.
 */
s32 BoardFilterFadeCheck(void)
{
    return (filterObj != NULL) ? 0 : 1;
}

/**
 * @brief Advance the filter alpha or destroy the filter when its work completes.
 * @param [in] object Filter object that owns FilterWork and is deleted when fading completes or shutdown begins.
 */
static void UpdateFilter(OMOBJ *object)
{
    float alpha;
    FilterWork *work = omObjGetWork(object, FilterWork);
    if(work->kill || BoardIsKill()) {
        if(work->model != -1) {
            Hu3DModelKill(work->model);
        }
        filterObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    if(work->paused) {
        return;
    }
    OSu8tof32(&work->color.a, &alpha);
    alpha += work->speed;
    OSf32tou8(&alpha, &work->color.a);
    if(work->time > 0) {
        work->time--;
        return;
    }
    if(work->speed > 0) {
        work->paused = 1;
        work->color.a = work->max_alpha;
    } else {
        work->kill = 1;
    }
}

/**
 * @brief Draw the full-screen colored filter quad with the current alpha.
 * @param [in] model Engine-model pointer supplied by the draw hook; unused by this callback.
 * @param [in] matrix Unused draw-hook view matrix; the callback loads its own orthographic projection and identity model-view matrix.
 */
static void DrawFilter(HU3DMODEL *model, Mtx matrix)
{
    static GXColor white_material_color = { 0xFF, 0xFF, 0xFF, 0xFF };
    Mtx44 proj;
    Mtx modelview;

    float x1, x2, y1, y2;
    FilterWork *work;
    if(!filterObj) {
        return;
    }
    work = omObjGetWork(filterObj, FilterWork);
    x1 = 0.0f;
    x2 = HU_FB_WIDTH;
    y1 = 0.0f;
    y2 = HU_FB_HEIGHT;
    MTXOrtho(proj, y1, y2, x1, x2, 0, 10);
    GXSetProjection(proj, GX_ORTHOGRAPHIC);
    MTXIdentity(modelview);
    GXLoadPosMtxImm(modelview, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    GXSetViewport(0, 0, x2, 1.0f+y2, 0, 1);
    GXSetScissor(0, 0, x2, 1.0f+y2);
    GXClearVtxDesc();
    GXSetChanMatColor(GX_COLOR0A0, work->color);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GXSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
    GXSetAlphaUpdate(GX_FALSE);
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition2u16(x1, y1);
    GXPosition2u16(x2, y1);
    GXPosition2u16(x2, y2);
    GXPosition2u16(x1, y2);
    GXEnd();
    GXSetChanMatColor(GX_COLOR0A0, white_material_color);

}

typedef struct confetti_particle {
    s16 time;
    u8 alpha;
    u8 light_col;
    Vec pos;
    Vec rot;
    Vec pos_vel;
    Vec rot_vel;
} ConfettiParticle;

typedef struct confetti_work {
    struct {
        u8 kill : 1;
        u8 paused : 1;
    };
    s8 spawn_speed;
    s8 time;
    s8 delay;
    s16 count;
    s16 gfx_mdl;
    s16 draw_mdl;
    ConfettiParticle *data;
} ConfettiWork;

static void UpdateConfetti(OMOBJ *object);
static void SpawnConfetti(OMOBJ *object);
static void MoveConfetti(OMOBJ *object);
static void DrawConfetti(HU3DMODEL *model, Mtx matrix);


/**
 * @brief Create a confetti emitter centered at a world position.
 * @param [in] pos Center of the confetti emitter in world coordinates.
 * @param [in] count Number of ConfettiParticle records allocated and simulated.
 * @param [in] range Emitter radius in world units.
 */
void BoardConfettiCreate(Vec *pos, s16 count, float range)
{
    OMOBJ *object;
    ConfettiWork *work;
    if(confettiObj) {
        BoardConfettiStop();
        HuPrcSleep(17);
    }
    object = omAddObjEx(boardObjMan, 257, 0, 0, -1, UpdateConfetti);
    confettiObj = object;
    work = omObjGetWork(object, ConfettiWork);
    work->kill = 0;
    work->paused = 0;
    work->count = count;
    work->spawn_speed = 1;
    work->time = 0;
    work->delay = 10;
    work->draw_mdl = Hu3DHookFuncCreate(DrawConfetti);
    work->data = HuMemDirectMallocNum(HEAP_HEAP, work->count*sizeof(ConfettiParticle), HU_MEMNUM_OVL);
    object->trans.x = pos->x;
    object->trans.y = pos->y;
    object->trans.z = pos->z;
    object->rot.x = range;
    work->gfx_mdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 7), NULL, 0);
    BoardModelLayerSet(work->gfx_mdl, 2);
    BoardModelVisibilitySet(work->gfx_mdl, 0);
    {
        ConfettiParticle *particle;
        s32 i;
        particle = work->data;
        for(i=0; i<work->count; i++, particle++) {
            particle->time = -1;
        }
    }
    HuAudFXPlay(774);
}

/**
 * @brief Request immediate destruction of the confetti emitter and its resources.
 */
void BoardConfettiKill(void)
{
    if(confettiObj) {
        omObjGetWork(confettiObj, ConfettiWork)->kill = 1;
    }
}

/**
 * @brief Stop spawning confetti and shorten remaining particle lifetimes for a graceful finish.
 */
void BoardConfettiStop(void)
{
    if(confettiObj) {
        s32 i;
        ConfettiParticle *particle;
        ConfettiWork *work = omObjGetWork(confettiObj, ConfettiWork);
        work->paused = 1;
        particle = work->data;
        for(i=0; i<work->count; i++, particle++) {
            if(particle->time != -1) { 
                if(particle->time > 16) {
                    particle->time = 16;
                }
            }
        }
    }
}

/**
 * @brief Spawn and move confetti particles, or release the emitter's resources on shutdown.
 * @param [in] object Confetti emitter object that owns particle work and is deleted during shutdown.
 */
static void UpdateConfetti(OMOBJ *object)
{
    ConfettiWork *work = omObjGetWork(object, ConfettiWork);
    if(work->kill || BoardIsKill()) {
        BoardModelKill(work->gfx_mdl);
        Hu3DModelKill(work->draw_mdl);
        HuMemDirectFree(work->data);
        confettiObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
    } else {
        SpawnConfetti(object);
        MoveConfetti(object);
    }
}

/**
 * @brief Activate free confetti particles around the emitter with randomized motion and color.
 * @param [in] object Confetti emitter whose position and radius define each spawned particle's initial position.
 */
static void SpawnConfetti(OMOBJ *object)
{
    ConfettiWork *work = omObjGetWork(object, ConfettiWork);
    s32 i;
    if(work->paused) {
        return;
    }
    if(work->spawn_speed < 5) {
        if(work->time++ > work->delay) {
            work->time = 0;
            work->spawn_speed++;
        }
    }
    for(i=0; i<work->spawn_speed; i++) {
        ConfettiParticle *particle;
        float angle;
        s32 j;
        particle = work->data;
        for(j=0; j<work->count; j++, particle++) {
            if(particle->time == -1) {
                break;
            }
        }
        if(j == work->count) {
            break;
        }
        particle->time = BoardRandMod(60)+120;
        angle = BoardRandFloat()*360.0f;
        particle->pos.x = (sind(angle)*object->rot.x)+object->trans.x;
        particle->pos.y = object->trans.y;
        particle->pos.z = (cosd(angle)*object->rot.x)+object->trans.z;
        particle->pos_vel.x = 2.0f*(BoardRandFloat()-0.5f);
        particle->pos_vel.y = (-98.00001f/15.0f)*BoardRandFloat();
        particle->pos_vel.z = 2.0f*(BoardRandFloat()-0.5f);
        particle->rot_vel.x = 8.0f+((BoardRandFloat()-0.5f)*20.0f);
        particle->rot_vel.y = 8.0f+((BoardRandFloat()-0.5f)*20.0f);
        particle->rot_vel.z = 8.0f+((BoardRandFloat()-0.5f)*20.0f);
        particle->rot.x = 0;
        particle->rot.y = 0;
        particle->rot.z = 0;
        particle->alpha = 255;
        particle->light_col = BoardRandMod(6);
    }
}

/**
 * @brief Integrate active confetti particles, fade them near expiry, and stop an empty paused emitter.
 * @param [in] object Confetti emitter that owns the particle array being integrated.
 */
static void MoveConfetti(OMOBJ *object)
{
    ConfettiWork *work = omObjGetWork(object, ConfettiWork);
    s32 i;
    s32 has_active_particle;
    ConfettiParticle *particle;

    has_active_particle = 0;
    particle = work->data;

    for(i=0; i<work->count; i++, particle++) {
        if(particle->time == -1) {
            continue;
        }
        if(particle->time <= 0) {
            particle->time = -1;
            continue;
        }
        particle->time--;
        particle->pos.x += particle->pos_vel.x;
        particle->pos.y += particle->pos_vel.y;
        particle->pos.z += particle->pos_vel.z;
        particle->rot.x += particle->rot_vel.x;
        particle->rot.y += particle->rot_vel.y;
        particle->rot.z += particle->rot_vel.z;
        if(particle->time < 16) {
            if(particle->alpha >= 15) {
                particle->alpha -= 15;
            } else {
                particle->alpha = 0;
            }
        }
        if(!has_active_particle) {
            has_active_particle = 1;
        }
    }
    if(has_active_particle == 0 && work->paused) {
        work->kill = 1;
    }
}

static Vec confettiLightTbl[6] = {
    { 0.1, 0.4, 1 },
    { 0.2, 1, 0.1 },
    { 0.3, 1, 1 },
    { 1, 0.2, 0.1 },
    { 1, 0.2, 0.8 },
    { 1, 8, 0.3 }
};

/**
 * @brief Draw each active confetti particle with its transform, alpha, and color multiplier.
 * @param [in] model Engine-model pointer supplied by the draw hook; unused by this callback.
 * @param [in] matrix Current view matrix supplied to the draw hook.
 */
static void DrawConfetti(HU3DMODEL *model, Mtx matrix)
{
    if(!confettiObj || BoardIsKill()) {
        return;
    } else {
        ConfettiWork *work = omObjGetWork(confettiObj, ConfettiWork);
        HU3DMODEL *model = &Hu3DData[work->gfx_mdl];
        ConfettiParticle *particle;
        s32 i;
        if(!model->hsf) {
            return;
        }
        particle = work->data;
        for(i=0; i<work->count; i++, particle++) {
            Mtx particle_transform, axis_transform;
            float r, g, b, a;
            if(particle->time == -1) {
                continue;
            }
            MTXRotDeg(axis_transform, 'z', particle->rot.z);
            MTXRotDeg(particle_transform, 'x', particle->rot.x);
            MTXConcat(axis_transform, particle_transform, particle_transform);
            MTXRotDeg(axis_transform, 'y', particle->rot.y);
            MTXConcat(axis_transform, particle_transform, particle_transform);
            MTXTrans(axis_transform, particle->pos.x, particle->pos.y, particle->pos.z);
            MTXConcat(axis_transform, particle_transform, particle_transform);
            MTXConcat(matrix, particle_transform, particle_transform);
            r = confettiLightTbl[particle->light_col].x;
            g = confettiLightTbl[particle->light_col].y;
            b = confettiLightTbl[particle->light_col].z;
            OSu8tof32(&particle->alpha, &a);
            a = a*(1.0f/255.0f);
            Hu3DModelTPLvlSet(BoardModelIDGet(work->gfx_mdl), a);
            Hu3DModelAmbSet(BoardModelIDGet(work->gfx_mdl), r, g, b);
            Hu3DModelObjDraw(BoardModelIDGet(work->gfx_mdl), "grid2", particle_transform);
        }
    }
}

typedef struct last5_gfx_work {
    struct {
        u8 kill : 1;
        u8 state : 3;
        u8 is_last : 1;
        u8 : 4;
    };
    u8 stop_time;
    s16 time;
    s16 group;
    s16 sprites[3];
} Last5GfxWork;

static s32 last5GfxSprTbl[3] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 95),
    DATA_MAKE_NUM(DATADIR_BOARD, 97),
    DATA_MAKE_NUM(DATADIR_BOARD, 96),
};

static float last5GfxPosTbl[2][3][2] = {
    {
        { -80, 0 },
        { 0, 0 },
        { 80, 0 }
    },
    {
        { -52, 0 },
        { 0, 0 },
        { 52, 0 }
    }
};

static void UpdateLast5Gfx(OMOBJ *object);

/**
 * @brief Create the final-five-turn or final-turn board banner for the current turn.
 */
void BoardLast5GfxInit(void)
{
    Last5GfxWork *work;
    OMOBJ *object;
    s32 turn_remain;
    s32 last_turn_layout;
    turn_remain = GWSystem.max_turn-GWSystem.turn;
    if(turn_remain > 4 || turn_remain < 0) {
        return;
    } else {
        s32 i;
        turn_remain = 4-turn_remain;
        object = omAddObjEx(boardObjMan, 0, 0, 0, -1, UpdateLast5Gfx);
        last5GfxObj = object;
        work = omObjGetWork(object, Last5GfxWork);
        work->kill = 0;
        work->stop_time = 0;
        work->time = 0;
        work->group = HuSprGrpCreate(3);
        if((s32)(GWSystem.max_turn-GWSystem.turn) == 0) {
            work->is_last = 1;
            last_turn_layout = 1;
        } else {
            work->is_last = 0;
            last_turn_layout = 0;
        }
        for(i=0; i<3; i++) {
            s32 prio;
            s32 spr_file;

            if(i == 1) {
                prio = 1000;
            } else {
                prio = 1400;
            }
#if !VERSION_JP
            spr_file = last5GfxSprTbl[i];
            if(i == 2 && work->is_last && GWLanguageGet() != 0) {
                spr_file = DATA_MAKE_NUM(DATADIR_BOARD, 98);
            }
            BoardSpriteCreate(spr_file, prio, NULL, &work->sprites[i]);
#else
            BoardSpriteCreate(last5GfxSprTbl[i], prio, NULL, &work->sprites[i]);
#endif
            HuSprGrpMemberSet(work->group, i, work->sprites[i]);
            HuSprAttrSet(work->group, i, HUSPR_ATTR_LINEAR);
            HuSprPosSet(work->group, i, last5GfxPosTbl[last_turn_layout][i][0], last5GfxPosTbl[last_turn_layout][i][1]);
        }
        if(!work->is_last) {
            HUSPRITE *sprite = &HuSprData[HuSprGrpData[work->group].members[1]];
            HuSprBankSet(work->group, 1, 0);
            sprite->animNo = turn_remain;
        } else {
            HuSprAttrSet(work->group, 1, HUSPR_ATTR_DISPOFF);
        }
        HuSprAttrSet(work->group, 1, HUSPR_ATTR_NOANIM);
        object->trans.x = 0.0f;
        HuSprGrpTPLvlSet(work->group, object->trans.x);
        HuSprGrpPosSet(work->group, HU_DISP_CENTERX, 72);
        HuAudFXPlay(838);
        work->time = 0;
    }
}

/**
 * @brief Fade, pulse, hold, and remove the final-turn-count banner.
 * @param [in,out] object Final-turn banner object whose X translation stores the current group translucency.
 */
static void UpdateLast5Gfx(OMOBJ *object)
{
    Last5GfxWork *work = omObjGetWork(object, Last5GfxWork);
    if(work->kill || BoardIsKill()) {
        HuSprGrpKill(work->group);
        last5GfxObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    if(work->stop_time != 0) {
        work->stop_time--;
        return;
    }
    switch(work->state) {
        case 0:
            object->trans.x += 1.0f/30.0f;
            if(object->trans.x > 1.0f) {
                object->trans.x = 1.0f;
                work->state = 1;
            }
            HuSprGrpTPLvlSet(work->group, object->trans.x);
            break;

        case 1:
            if(work->time >= 720) {
                work->state = 2;
                work->stop_time = 90;
                if(work->is_last) {
                    HuSprGrpScaleSet(work->group, 1.0f, 1.0f);
                } else {
                    HuSprScaleSet(work->group, 1, 1.0f, 1.0f);
                }
            } else {
                s16 angle;
                angle = work->time%180;
                OSs16tof32(&angle, &object->trans.y);
                object->trans.y = sind(object->trans.y)+0.5;
                if(work->is_last) {
                    HuSprGrpScaleSet(work->group, object->trans.y, object->trans.y);
                } else {
                    HuSprScaleSet(work->group, 1, object->trans.y, object->trans.y);
                }
                work->time += 9;
            }
            break;

        case 2:
            object->trans.x -= 1.0f/30.0f;
            if(object->trans.x < 0.0f) {
                object->trans.x = 0.0f;
                work->kill = 1;
            }
            HuSprGrpTPLvlSet(work->group, object->trans.x);
            break;
    }
}

/**
 * @brief Show or hide the final-turn-count banner sprites.
 * @param [in] show Nonzero to show the sprites; zero to hide them.
 */
void BoardLast5GfxShowSet(s32 show)
{
    s32 i;
    Last5GfxWork *work;
    if(!last5GfxObj) {
        return;
    }

    work = omObjGetWork(last5GfxObj, Last5GfxWork);
    for(i=0; i<3; i++) {
        if(show) {
            HuSprAttrReset(work->group, i, HUSPR_ATTR_DISPOFF);
        } else {
            HuSprAttrSet(work->group, i, HUSPR_ATTR_DISPOFF);
        }
        if(work->is_last) {
            HuSprAttrSet(work->group, 1, HUSPR_ATTR_DISPOFF);
        }
    }
}

static s32 tauntActiveFXTbl[4] = { -1, -1, -1, -1 };
static s32 tauntFXTbl[8] = {
    294,
    358,
    422,
    486,
    550,
    614,
    678,
    742
};

typedef struct taunt_work {
    u8 kill : 1;
} TauntWork;

static void TauntUpdate(OMOBJ *object);

/**
 * @brief Create the board taunt-input monitor and clear active voice handles.
 */
void BoardTauntInit(void)
{
    s32 i;
    tauntObj = omAddObjEx(boardObjMan, 32258, 0, 0, -1, TauntUpdate);
    for(i=0; i<4; i++) {
        tauntActiveFXTbl[i] = -1;
    }
    _SetFlag(FLAG_ID_MAKE(1, 14));
}

/**
 * @brief Request destruction of the board taunt-input monitor.
 */
void BoardTauntKill(void)
{
    TauntWork *work;
    if(!tauntObj) {
        return;
    }
    work = omObjGetWork(tauntObj, TauntWork);
    work->kill = 1;
    _SetFlag(FLAG_ID_MAKE(1, 14));
}

/**
 * @brief Start and modulate non-current human players' character taunts from controller input.
 * @param [in] object Taunt manager object that owns the kill flag and per-frame controller-processing work.
 */
static void TauntUpdate(OMOBJ *object)
{
    s32 i;
    s32 port;
    s32 character;
    TauntWork *work;
    work = omObjGetWork(object, TauntWork);
    if(work->kill || BoardIsKill()) {
        for(i=0; i<4; i++) {
            if(tauntActiveFXTbl[i] >= 0) {
                HuAudFXStop(tauntActiveFXTbl[i]);
                tauntActiveFXTbl[i] = -1;
            }
        }
        tauntObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    for(i=0; i<4; i++) {
        if(tauntActiveFXTbl[i] >= 0 && HuAudFXStatusGet(tauntActiveFXTbl[i]) == 0) {
            tauntActiveFXTbl[i] = -1;
        }
    }
    if(BoardPauseActiveCheck()) {
        return;
    }
    if(_CheckFlag(FLAG_ID_MAKE(1, 14))) {
        return;
    }
    if(WipeStatGet()) {
        return;
    }
    if(GWSystem.player_curr == -1) {
        return;
    }
    for(i=0; i<4; i++) {
        if(i == GWSystem.player_curr || GWPlayer[i].com) {
            continue;
        }
        port = GWPlayer[i].port & 0x3;
        character = GWPlayer[i].character & 0x7;
        if(tauntActiveFXTbl[port] >= 0) {
            MSM_SEPARAM param;
            float vol, pan;
            vol = (64.0f*(HuPadSubStkX[port]/59.0f))+64.0f;
            pan = 8191.0f*(HuPadSubStkY[port]/59.0f);
            memset(&param, 0, sizeof(MSM_SEPARAM));
            param.flag = MSM_SEPARAM_PITCH|MSM_SEPARAM_PAN;
            OSf32tos8(&vol, &param.pan);
            OSf32tos16(&pan, &param.pitch);
            msmSeSetParam(tauntActiveFXTbl[port], &param);
        } else {
            if(HuPadBtnDown[port] & PAD_TRIGGER_L) {
                tauntActiveFXTbl[port] = HuAudFXPlay(tauntFXTbl[character]);
            }
        }
    }
}

/**
 * @brief Begin an asynchronous read of a board data directory.
 * @param [in] data_num Packed data-directory or file ID passed directly to HuDataDirReadAsync.
 * @return Asynchronous request handle, or -1 when the request could not start.
 */
s32 BoardDataDirReadAsync(s32 data_num)
{
    s32 status = HuDataDirReadAsync(data_num);
    return status;
}

/**
 * @brief Yield until an asynchronous board-data request completes.
 * @param [in] status Asynchronous request handle; -1 means no request was started.
 */
void BoardDataAsyncWait(s32 status)
{
    if(status == -1) {
        return;
    }
    while(!HuDataGetAsyncStat(status)) {
        HuPrcVSleep();
    }
}
