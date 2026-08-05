#include "types.h"
#include "game/board/lottery.h"
#include "game/audio.h"
#include "game/board/audio.h"
#include "game/board/com.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/tutorial.h"
#include "game/board/ui.h"
#include "game/board/window.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/esprite.h"
#include "game/gamework_data.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"

#include "ext_math.h"
#include "stdlib.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/board/shop.h"
#endif

typedef struct {
    struct {
        u8 kill : 1;
        s8 at_target : 1;
        u8 sprite_index : 4;
    };
    u8 delay;
    s16 prize_sprite;
    s16 cover_sprite;
    char padding[2];
    ANIMDATA *cover_anim;
} TicketWork;

typedef struct {
    struct {
        u8 kill : 1;
    };
    s8 character;
    s8 column;
    s8 row;
    u8 input_delay;
    char padding[1];
    s16 sprite;
    s16 sprite_group;
} LotteryTicketPickWork;

static void CreateModel(void);
static void KillModel(void);
static void DestroyLottery(void);
static void ExecLottery(void);
static void DoMiniJumpUp(s32 arg0);
static void DoMiniJumpDown(s32 arg0);
static void PayEnterFee(s32 arg0);
static void ShowTicket(void);
static void SetupTicket(s32 arg0);
static void ExecBallGame(void);
static void SetBallPrize(void);
static void ExecBallPrize(void);
static void ExecScratchTicket(s32 arg0);
static void ExecScratch(void);
static void KillScratch(void);
static void ExecScratchSpr(OMOBJ *arg0);
static void HideScratchSpr(void);
static void InitScratchSpr(void);
static void ExecScratchPick(OMOBJ *arg0);
static void InitScratchPick(void);
static void InitTicketPrizes(void);
static void ExecTicketFocus(s32 arg0);
static BOOL ScratchTicketCheckDone(ANIMBMP *arg0);
static u16 TicketGetPixel(u16 arg0, u16 arg1, u16 arg2);
static s32 TicketUpdate(ANIMBMP *arg0, Vec *arg1, s32 arg2);
static void ExecCoinPrize(void);
static void ExecItemPrize(void);
static void KillCoin(void);
static void ExecPrize(void);
static void ExecLose(s32 arg0);
static void CreateLotteryWin(s32 arg0);
static void KillLotteryWin(void);

static s16 lotteryMdl[6];
static s8 coinF[12];
static s16 coinMdl[10];

static s32 lotteryMessBase;
static s16 lotterySpace;
static s8 currPrize;
s8 lbl_801D401B;
static s8 comLotteryType;
static s8 comLotteryWinType;
static u8 comInputPos;
static s32 handUpdateF;
static OMOBJ *lotteryTicketPickObj;
static s8 (*comInputDrawP)[2];
static HUPROCESS *lotteryProc;

static s16 hostMdl = -1;
static s16 lotteryMot[4] = { -1, -1, -1, -1 };
static s16 lotterySpr = -1;
static s16 lotteryInstWin = -1;
static s16 gripMdl[2] = { 0, 0 };
static s16 turnMot = -1;
static s16 ballMdl[4] = { -1, -1, -1, -1 };
static s16 loseMot = -1;
static s16 ticketSprGrp = -1;

static s8 comInputDraw1[][2] = {
    { 0x0B, 0xF8 }, { 0x2B, 0xDD }, { 0x2A, 0xDC }, { 0x2B, 0xDC },
    { 0x2B, 0xDC }, { 0x29, 0xDB }, { 0x24, 0xD6 }, { 0x02, 0xE8 },
    { 0x00, 0x00 }, { 0xFC, 0x02 }, { 0xD8, 0x27 }, { 0xD8, 0x28 },
    { 0xD8, 0x28 }, { 0xD8, 0x27 }, { 0xDA, 0x29 }, { 0xDE, 0x2C },
    { 0xE4, 0x31 }, { 0xF0, 0x3A }, { 0x00, 0x3C }, { 0x06, 0x3A },
    { 0x1B, 0x32 }, { 0x23, 0x2B }, { 0x24, 0x2A }, { 0x25, 0x29 },
    { 0x26, 0x29 }, { 0x28, 0x27 }, { 0x28, 0x27 }, { 0x28, 0x26 },
    { 0x28, 0x26 }, { 0x2E, 0x20 }, { 0x39, 0x11 }, { 0x48, 0x00 },
    { 0x48, 0x00 }, { 0x35, 0xEA }, { 0x29, 0xDB }, { 0x24, 0xD5 },
    { 0x17, 0xCB }, { 0x00, 0xB8 }, { 0x00, 0xB8 }, { 0x00, 0xB8 },
    { 0x00, 0xB8 }, { 0x00, 0xB8 }, { 0xEB, 0xCA }, { 0xD6, 0xDB },
    { 0xD6, 0xDB }, { 0xD4, 0xDE }, { 0xD1, 0xE2 }, { 0xC4, 0xF2 },
    { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xB8, 0x00 },
    { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xC1, 0x0B },
    { 0xC6, 0x10 }, { 0xC9, 0x14 }, { 0xCB, 0x16 }, { 0xCB, 0x17 },
    { 0xCE, 0x1A }, { 0xD1, 0x1E }, { 0xD5, 0x23 }, { 0xD4, 0x22 },
    { 0xDB, 0x29 }, { 0xF5, 0x3E }, { 0x00, 0x48 }, { 0x00, 0x42 },
    { 0x0D, 0x3D }, { 0x24, 0x2A }, { 0x29, 0x26 }, { 0x29, 0x26 },
    { 0x29, 0x25 }, { 0x2D, 0x21 }, { 0x34, 0x18 }, { 0x40, 0x09 },
    { 0x48, 0x00 }, { 0x48, 0x00 }, { 0x43, 0xFA }, { 0x33, 0xE6 },
    { 0x29, 0xDB }, { 0x1B, 0xCF }, { 0x05, 0xBD }, { 0x00, 0xB8 },
    { 0x00, 0xB8 }, { 0x00, 0xB8 }, { 0xFB, 0xBD }, { 0xDD, 0xD5 },
    { 0xD6, 0xDC }, { 0xD6, 0xDC }, { 0xD4, 0xDE }, { 0xCB, 0xE9 },
    { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xC5, 0x0F },
    { 0xD0, 0x1D }, { 0xD5, 0x23 }, { 0xDC, 0x2A }, { 0xF0, 0x3B },
    { 0x00, 0x3D }, { 0x08, 0x3F }, { 0x15, 0x36 }, { 0x24, 0x2A },
    { 0x29, 0x26 }, { 0x2A, 0x24 }, { 0x30, 0x1C }, { 0x3B, 0x0F },
    { 0x48, 0x00 }, { 0x48, 0x00 }, { 0x3E, 0xF4 }, { 0x2D, 0xE0 },
    { 0x22, 0xD4 }, { 0x02, 0xBB }, { 0x00, 0xB8 }, { 0xFB, 0xBD },
    { 0xD9, 0xD8 }, { 0xD4, 0xDE }, { 0x7F, 0x7F }
};

static s8 comInputDraw2[][2] = {
    { 0x07, 0x00 }, { 0x1F, 0x00 }, { 0x3A, 0x00 }, { 0x48, 0x00 },
    { 0x48, 0x00 }, { 0x48, 0x00 }, { 0x48, 0x00 }, { 0x48, 0x00 },
    { 0x43, 0xFA }, { 0x35, 0xEA }, { 0x2A, 0xDB }, { 0x1A, 0xCE },
    { 0x00, 0xB9 }, { 0x00, 0xB8 }, { 0x00, 0xB8 }, { 0x00, 0xB8 },
    { 0x00, 0xBA }, { 0xE9, 0xCC }, { 0xD5, 0xDD }, { 0xD4, 0xDE },
    { 0xD4, 0xDE }, { 0xD0, 0xE3 }, { 0xBD, 0xFB }, { 0xB8, 0x00 },
    { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xB8, 0x00 },
    { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xBC, 0x03 }, { 0xC2, 0x0C },
    { 0xCB, 0x17 }, { 0xD5, 0x23 }, { 0xD8, 0x27 }, { 0xF0, 0x3A },
    { 0x00, 0x43 }, { 0x18, 0x34 }, { 0x29, 0x26 }, { 0x29, 0x26 },
    { 0x29, 0x26 }, { 0x29, 0x25 }, { 0x29, 0x26 }, { 0x29, 0x25 },
    { 0x29, 0x25 }, { 0x2C, 0x22 }, { 0x31, 0x1B }, { 0x39, 0x12 },
    { 0x47, 0x00 }, { 0x48, 0x00 }, { 0x48, 0x00 }, { 0x37, 0xEC },
    { 0x26, 0xD7 }, { 0x00, 0xB8 }, { 0x00, 0xB8 }, { 0x00, 0xB8 },
    { 0x00, 0xB8 }, { 0x00, 0xB8 }, { 0x00, 0xB8 }, { 0xFB, 0xBD },
    { 0xE5, 0xCE }, { 0xD6, 0xDC }, { 0xD6, 0xDC }, { 0xD6, 0xDC },
    { 0xD5, 0xDD }, { 0xD2, 0xE0 }, { 0xCC, 0xE8 }, { 0xC4, 0xF2 },
    { 0xBE, 0xFA }, { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xB8, 0x00 },
    { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xB8, 0x00 },
    { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xB8, 0x00 },
    { 0xC7, 0x12 }, { 0xD8, 0x27 }, { 0xF6, 0x3E }, { 0x00, 0x47 },
    { 0x00, 0x44 }, { 0x05, 0x42 }, { 0x16, 0x35 }, { 0x28, 0x27 },
    { 0x29, 0x26 }, { 0x29, 0x26 }, { 0x29, 0x26 }, { 0x29, 0x26 },
    { 0x7F, 0x7F }
};

static s8 comInputBall[][2] = {
    { 0xE5, 0x00 }, { 0xC0, 0x03 }, { 0xBC, 0x04 }, { 0xBC, 0x03 },
    { 0xBF, 0x07 }, { 0xC3, 0x0C }, { 0xCB, 0x17 }, { 0xD5, 0x24 },
    { 0xDE, 0x2C }, { 0xF9, 0x3E }, { 0x00, 0x48 }, { 0x00, 0x45 },
    { 0x06, 0x40 }, { 0x21, 0x2C }, { 0x28, 0x27 }, { 0x28, 0x26 },
    { 0x29, 0x25 }, { 0x30, 0x1D }, { 0x43, 0x05 }, { 0x48, 0x00 },
    { 0x33, 0xE7 }, { 0x24, 0xD6 }, { 0x07, 0xBF }, { 0x00, 0xB8 },
    { 0x00, 0xB8 }, { 0xE4, 0xCF }, { 0xD4, 0xDE }, { 0xC6, 0xF0 },
    { 0xB8, 0x00 }, { 0xC7, 0x11 }, { 0xD2, 0x20 }, { 0xD5, 0x24 },
    { 0xD6, 0x25 }, { 0xD8, 0x27 }, { 0xE1, 0x2E }, { 0x00, 0x43 },
    { 0x00, 0x48 }, { 0x00, 0x44 }, { 0x1D, 0x30 }, { 0x28, 0x27 },
    { 0x28, 0x27 }, { 0x2C, 0x22 }, { 0x39, 0x12 }, { 0x48, 0x00 },
    { 0x34, 0xE8 }, { 0x23, 0xD5 }, { 0x0A, 0xC1 }, { 0x00, 0xB8 },
    { 0x00, 0xB8 }, { 0xF4, 0xC3 }, { 0xD4, 0xDE }, { 0xC3, 0xF4 },
    { 0xB8, 0x00 }, { 0xC6, 0x10 }, { 0xD3, 0x21 }, { 0xD6, 0x25 },
    { 0xD8, 0x27 }, { 0xDF, 0x2D }, { 0xEA, 0x36 }, { 0x00, 0x48 },
    { 0x00, 0x48 }, { 0x00, 0x48 }, { 0x17, 0x35 }, { 0x28, 0x27 },
    { 0x29, 0x25 }, { 0x33, 0x19 }, { 0x48, 0x00 }, { 0x48, 0x00 },
    { 0x34, 0xE8 }, { 0x23, 0xD5 }, { 0x00, 0xB9 }, { 0x00, 0xB8 },
    { 0x00, 0xB8 }, { 0xFA, 0xBE }, { 0xD4, 0xDE }, { 0xC8, 0xED },
    { 0xB8, 0x00 }, { 0xB8, 0x00 }, { 0xC0, 0x09 }, { 0xD0, 0x1C },
    { 0xD6, 0x24 }, { 0xDC, 0x2A }, { 0xED, 0x38 }, { 0x00, 0x48 },
    { 0x00, 0x48 }, { 0x00, 0x48 }, { 0x1B, 0x31 }, { 0x28, 0x27 },
    { 0x29, 0x26 }, { 0x2D, 0x21 }, { 0x3D, 0x0C }, { 0x48, 0x00 },
    { 0x33, 0xE7 }, { 0x20, 0xD3 }, { 0x00, 0xB8 }, { 0x00, 0xB8 },
    { 0x00, 0xBB }, { 0xDB, 0xD7 }, { 0xD4, 0xDE }, { 0xD3, 0xDF },
    { 0xBB, 0xFD }, { 0xB8, 0x00 }, { 0x7F, 0x7F }
};

/**
 * @brief Sets the board model used as the lottery host.
 * @param arg0 Board-model handle for the lottery host.
 */
void BoardLotteryHostSet(s16 arg0) {
    hostMdl = arg0;
}

/**
 * @brief Gets the board model used as the lottery host.
 * @return Lottery host board-model handle, or -1 when unset.
 */
s16 BoardLotteryHostGet(void) {
    return hostMdl;
}

/**
 * @brief Runs and waits for the lottery event process.
 */
void BoardLotteryExec(void) {
    if (BoardPlayerSizeGet(GWSystem.player_curr) == 2) {
        return;
    }
    BoardRollDispSet(0);
    lotteryProc = HuPrcChildCreate(ExecLottery, 0x2003, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(lotteryProc, DestroyLottery);
    while (lotteryProc) {
        HuPrcVSleep();
    }
    BoardRollDispSet(1);
}

/**
 * @brief Creates lottery models, motions, and initial placement for the current board.
 */
void BoardLotteryInit(void) {
    Vec lottery_position;
    Vec host_position;
    s32 i;

    for (i = 0; i < 5; i++) {
        lotteryMdl[i] = -1;
    }
    for (i = 0; i < 4; i++) {
        lotteryMot[i] = -1;
    }
    for (i = 0; i < 4; i++) {
        ballMdl[i] = -1;
    }
    turnMot = -1;
    loseMot = -1;
    if (GWBoardGet() != BOARD_ID_EXTRA2) {
        BoardModelVisibilitySet(BoardLotteryHostGet(), 0);
        BoardSpaceLinkTransformGet(0x10000000, &lottery_position, &host_position, NULL);
        lotteryMdl[0] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BKUJIYA, 0), NULL, 0);
        BoardModelMotionStart(lotteryMdl[0], 0, 0);
        BoardModelMotionSpeedSet(lotteryMdl[0], 0.0f);
        BoardModelPosSetV(lotteryMdl[0], &lottery_position);
        BoardModelRotSetV(lotteryMdl[0], &host_position);
        HuDataDirClose(DATA_MAKE_NUM(DATADIR_BKUJIYA, 0));
    }
}

/**
 * @brief Creates and configures the lottery scene's models, motions, and sprites.
 */
static void CreateModel(void) {
    s32 i;
    s32 lottery_resources[4] = {
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 5),
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 6),
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 7),
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 8)
    };
    s32 grip_resources[2] = {
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 3),
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 4)
    };
    s32 model_resources[9][4] = {
        {
            DATA_MAKE_NUM(DATADIR_BGUEST, 1),
            DATA_MAKE_NUM(DATADIR_BGUEST, 2),
            DATA_MAKE_NUM(DATADIR_BGUEST, 3),
            DATA_MAKE_NUM(DATADIR_BGUEST, 4)
        },
        {
            DATA_MAKE_NUM(DATADIR_BGUEST,  9),
            DATA_MAKE_NUM(DATADIR_BGUEST, 10),
            DATA_MAKE_NUM(DATADIR_BGUEST, 11),
            DATA_MAKE_NUM(DATADIR_BGUEST, 12)
        },
        {
            DATA_MAKE_NUM(DATADIR_BGUEST, 14),
            DATA_MAKE_NUM(DATADIR_BGUEST, 15),
            DATA_MAKE_NUM(DATADIR_BGUEST, 16),
            DATA_MAKE_NUM(DATADIR_BGUEST, 17)
        },
        {
            DATA_MAKE_NUM(DATADIR_BGUEST, 24),
            DATA_MAKE_NUM(DATADIR_BGUEST, 25),
            DATA_MAKE_NUM(DATADIR_BGUEST, 26),
            DATA_MAKE_NUM(DATADIR_BGUEST, 27)
        },
        {
            DATA_MAKE_NUM(DATADIR_BGUEST, 29),
            DATA_MAKE_NUM(DATADIR_BGUEST, 30),
            DATA_MAKE_NUM(DATADIR_BGUEST, 31),
            DATA_MAKE_NUM(DATADIR_BGUEST, 32)
        },
        {
            DATA_MAKE_NUM(DATADIR_BGUEST, 34),
            DATA_MAKE_NUM(DATADIR_BGUEST, 35),
            DATA_MAKE_NUM(DATADIR_BGUEST, 36),
            DATA_MAKE_NUM(DATADIR_BGUEST, 37)
        },
        {
            DATA_MAKE_NUM(DATADIR_BGUEST, 14),
            DATA_MAKE_NUM(DATADIR_BGUEST, 15),
            DATA_MAKE_NUM(DATADIR_BGUEST, 16),
            DATA_MAKE_NUM(DATADIR_BGUEST, 17)
        },
        {
            DATA_MAKE_NUM(DATADIR_BGUEST, 42),
            DATA_MAKE_NUM(DATADIR_BGUEST, 43),
            DATA_MAKE_NUM(DATADIR_BGUEST, 44),
            DATA_MAKE_NUM(DATADIR_BGUEST, 45)
        },
        {
            DATA_MAKE_NUM(DATADIR_BGUEST, 42),
            DATA_MAKE_NUM(DATADIR_BGUEST, 43),
            DATA_MAKE_NUM(DATADIR_BGUEST, 44),
            DATA_MAKE_NUM(DATADIR_BGUEST, 45)
        }
    };

    lotteryMdl[1] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BKUJIYA, 1), NULL, 0);
    fn_8006DDE8(lotteryMdl[1], -1.0f);
    BoardModelMotionSpeedSet(lotteryMdl[1], 0.0f);
    lotteryMdl[2] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BKUJIYA, 2), NULL, 0);
    BoardModelVisibilitySet(lotteryMdl[2], 0);
    fn_8006DDE8(lotteryMdl[2], -1.0f);
    BoardModelMotionStart(lotteryMdl[2], 0, 0);
    BoardModelMotionSpeedSet(lotteryMdl[2], 0.0f);
    for (i = 0; i < 4; i++) {
        ballMdl[i] = BoardModelCreate(lottery_resources[i], NULL, 0);
        BoardModelVisibilitySet(ballMdl[i], 0);
        fn_8006DDE8(ballMdl[i], -1.0f);
    }
    for (i = 0; i < 2; i++) {
        gripMdl[i] = BoardModelCreate(grip_resources[i], NULL, 0);
        BoardModelVisibilitySet(gripMdl[i], 0);
        fn_8006DDE8(gripMdl[i], -1.0f);
        BoardModelMotionSpeedSet(gripMdl[i], 0.0f);
    }
    lotteryMdl[3] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BKUJIYA, 17), NULL, 0);
    BoardModelVisibilitySet(lotteryMdl[3], 0);
    fn_8006DDE8(lotteryMdl[3], -1.0f);
    BoardModelPosSet(BoardLotteryHostGet(), 0.0f, 0.0f, 0.0f);
    BoardModelVisibilitySet(BoardLotteryHostGet(), 1);
    for (i = 0; i < 4; i++) {
        lotteryMot[i] = BoardModelMotionCreate(BoardLotteryHostGet(), model_resources[GWBoardGet()][i]);
    }
    BoardModelMotionStart(BoardLotteryHostGet(), lotteryMot[0], 0x40000001);
    BoardModelHookSet(lotteryMdl[0], "toto_heyhoo", BoardLotteryHostGet());
    BoardModelHookSet(lotteryMdl[0], "toto_gara", lotteryMdl[1]);
    BoardModelHookSet(lotteryMdl[0], "toto_efe", lotteryMdl[2]);
    BoardModelHookSet(lotteryMdl[0], "toto_chibidai", lotteryMdl[3]);
    BoardModelHookSet(lotteryMdl[1], "toto_grip", gripMdl[0]);
    BoardModelHookSet(lotteryMdl[1], "toto_grip1", gripMdl[1]);
}

/**
 * @brief Releases the lottery scene's models, motions, and sprites.
 */
static void KillModel(void) {
    s32 i;

    if (lotteryMdl[1] != -1) {
        BoardModelHookReset(lotteryMdl[0]);
    }
    BoardModelMotionStart(BoardLotteryHostGet(), 1, 0x40000001);
    if (turnMot != -1) {
        BoardPlayerMotionKill(GWSystem.player_curr, turnMot);
        turnMot = -1;
    }
    for (i = 0; i < 4; i++) {
        if (lotteryMot[i] != -1) {
            BoardModelMotionKill(BoardLotteryHostGet(), lotteryMot[i]);
            lotteryMot[i] = -1;
        }
    }
    for (i = 0; i < 4; i++) {
        if (ballMdl[i] != -1) {
            BoardModelKill(ballMdl[i]);
            ballMdl[i] = -1;
        }
    }
    for (i = 0; i < 2; i++) {
        if (gripMdl[i] != -1) {
            BoardModelKill(gripMdl[i]);
            gripMdl[i] = -1;
        }
    }
    for (i = 0; i < 5; i++) {
        if (i != 0 && lotteryMdl[i] != -1) {
            BoardModelKill(lotteryMdl[i]);
            lotteryMdl[i] = -1;
        }
    }
    BoardModelVisibilitySet(BoardLotteryHostGet(), 0);
}

/**
 * @brief Stops the lottery process and releases its scene resources.
 */
void BoardLotteryKill(void) {
    if (lotteryProc) {
        HuPrcKill(lotteryProc);
    }
    if (lotteryMdl[0] != -1) {
        BoardModelKill(lotteryMdl[0]);
        lotteryMdl[0] = -1;
    }
}

/**
 * @brief Restores the player and host state when the lottery process ends.
 */
static void DestroyLottery(void) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (i != GWSystem.player_curr) {
            BoardPlayerMotBlendSet(i, 0, 7);
        }
    }
    if (loseMot != -1) {
        BoardPlayerMotionKill(GWSystem.player_curr, loseMot);
        loseMot = -1;
    }
    KillLotteryWin();
    BoardConfettiKill();
    BoardWinKill();
    KillScratch();
    KillCoin();
    HuDataDirClose(DATA_MAKE_NUM(DATADIR_BKUJIYA, 0));
    lotteryProc = NULL;
}

/**
 * @brief Runs admission, game selection, play, prize, and exit for the lottery event.
 */
static void ExecLottery(void) {
    Vec player_position;
    Vec lottery_position;
    Vec entrance_position;
    Vec camera_rotation;
    Vec camera_offset;
    s16 player_motion;
    s16 host_motion;
    u32 input;
    s32 game_type;
    s32 player;
    s32 i;
    s32 message;

    switch (GWBoardGet()) {
        case BOARD_ID_MAIN1:
            lotteryMessBase = MAKE_MESSID(6, 2);
            break;
        case BOARD_ID_MAIN2:
            lotteryMessBase = MAKE_MESSID(6, 15);
            break;
        case BOARD_ID_MAIN3:
            lotteryMessBase = MAKE_MESSID(6, 28);
            break;
        case BOARD_ID_MAIN4:
            lotteryMessBase = MAKE_MESSID(6, 41);
            break;
        case BOARD_ID_MAIN5:
            lotteryMessBase = MAKE_MESSID(6, 54);
            break;
        case BOARD_ID_MAIN6:
            lotteryMessBase = MAKE_MESSID(6, 67);
            break;
        case BOARD_ID_TUTORIAL:
            lotteryMessBase = MAKE_MESSID(6, 2);
            break;
        case BOARD_ID_EXTRA1:
            lotteryMessBase = MAKE_MESSID(6, 82);
            break;
        case BOARD_ID_EXTRA2:
            lotteryMessBase = MAKE_MESSID(6, 82);
            break;
    }
    message = BoardDataDirReadAsync(DATADIR_BKUJIYA);
    player = GWSystem.player_curr;
    host_motion = GWPlayer[player].space_curr;
    BoardPlayerIdleSet(player);
    BoardWinCreateChoice(0, MAKE_MESSID(6, 0), -1, 0);
    if (GWPlayer[player].com) {
        if (BoardPlayerCoinsGet(player) >= 5) {
            BoardComKeySetLeft();
        } else {
            BoardComKeySetRight();
        }
    }
    BoardWinWait();
    if (BoardWinChoiceGet() != 0) {
        if (message != -1) {
            BoardDataAsyncWait(message);
        }
        HuPrcEnd();
    }
    if (BoardPlayerCoinsGet(player) < 5) {
        BoardWinCreate(0, MAKE_MESSID(6, 1), -1);
        BoardWinWait();
        BoardWinKill();
        if (message != -1) {
            BoardDataAsyncWait(message);
        }
        HuPrcEnd();
    }
    BoardWinKill();
    if (message != -1) {
        BoardDataAsyncWait(message);
    }
    BoardModelMotionStart(BoardLotteryHostGet(), 1, 0x40000001);
    BoardAudSeqPause(0, 1, 1000);
    CreateModel();
    BoardStatusShowSetAll(0);
    lotterySpace = BoardSpaceLinkFlagSearch(0, host_motion, 0x02000000);
    BoardSpacePosGet(0, host_motion, &player_position);
    BoardSpacePosGet(0, lotterySpace, &lottery_position);
    VECSubtract(&lottery_position, &player_position, &entrance_position);
    if (GWBoardGet() == BOARD_ID_MAIN3 || GWBoardGet() == BOARD_ID_MAIN4) {
        VECScale(&entrance_position, &entrance_position, 0.95f);
        VECAdd(&entrance_position, &player_position, &lottery_position);
    }
    player_motion = atan2d(-entrance_position.x, -entrance_position.z);
    if (BoardPlayerSizeGet(player) == 1) {
        BoardModelVisibilitySet(gripMdl[0], 0);
        BoardModelVisibilitySet(gripMdl[1], 1);
    } else {
        BoardModelVisibilitySet(gripMdl[0], 1);
        BoardModelVisibilitySet(gripMdl[1], 0);
    }
    camera_rotation.x = -3.0f;
    camera_rotation.y = player_motion;
    camera_rotation.z = 0.0f;
    camera_offset.x = 0.0f;
    camera_offset.y = 200.0f;
    camera_offset.z = 0.0f;
    BoardCameraMotionStartEx(lotteryMdl[0], &camera_rotation, &camera_offset, 1300.0f, -1.0f, 27);
    if (BoardPlayerSizeGet(player) == 1) {
        BoardModelVisibilitySet(lotteryMdl[3], 1);
    }
    omVibrate(player, 12, 6, 6);
    HuAudFXPlay(0x331);
    HuAudFXPlay(0x344);
    BoardModelMotionStart(lotteryMdl[0], 0, 0);
    while (!BoardModelMotionEndCheck(lotteryMdl[0])) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(BoardLotteryHostGet(), lotteryMot[3], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    BoardPlayerRotSet(player, 0.0f, player_motion, 0.0f);
    BoardPlayerPosLerpStart(player, &player_position, &lottery_position, 0x14);
    while (GWPlayer[player].moving) {
        HuPrcVSleep();
    }
    if (BoardPlayerSizeGet(player) == 1) {
        DoMiniJumpUp(player);
    }
    BoardPlayerPosGet(player, &lottery_position);
    for (i = 0; i < 4; i++) {
        if (i != player) {
            BoardPlayerPosGet(i, &player_position);
            VECSubtract(&lottery_position, &player_position, &entrance_position);
            VECNormalize(&entrance_position, &entrance_position);
            BoardPlayerMotBlendSet(i, atan2d(entrance_position.x, entrance_position.z), 7);
        }
    }
    BoardMusStart(1, 2, 0x7F, 0);
    BoardPlayerIdleSet(player);
    while (!BoardStatusStopCheck(player)) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(BoardLotteryHostGet(), lotteryMot[0], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    if (GWPlayer[player].draw_ticket == 0 && GWSystem.max_turn - GWSystem.turn >= 5) {
        SetupTicket(player);
        ShowTicket();
        game_type = 1;
    } else {
        game_type = 0;
    }
    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        game_type = 0;
    }
    PayEnterFee(game_type);
    input = BoardRandMod(100) & 0xFF;
    if (input < 50) {
        ExecBallGame();
    } else {
        ExecScratch();
    }
    ExecPrize();
    if (BoardPlayerSizeGet(player) == 1) {
        DoMiniJumpDown(player);
    }
    BoardPlayerMoveBetween(player, lotterySpace, host_motion);
    while (GWPlayer[player].moving) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(player);
    BoardModelVisibilitySet(ballMdl[currPrize & 3], 0);
    BoardAudSeqFadeOut(1, 1000);
    BoardCameraTargetPlayerSet(player);
    BoardCameraViewSet(1);
    BoardStatusShowSet(player, 0);
    BoardModelMotionStart(lotteryMdl[0], 0, 0x40000004);
    while (!BoardModelMotionEndCheck(lotteryMdl[0])) {
        HuPrcVSleep();
    }
    BoardModelMotionSpeedSet(lotteryMdl[0], 0.0f);
    BoardModelAttrReset(lotteryMdl[0], 0x40000004);
    HuAudFXPlay(0x32F);
    KillModel();
    BoardStatusShowSetAll(1);
    BoardAudSeqPause(0, 0, 1000);
    HuPrcEnd();
}

/**
 * @brief Animates a player jumping upward at the lottery entrance.
 * @param arg0 Player index to move upward.
 */
static void DoMiniJumpUp(s32 arg0) {
    Mtx player_transform;
    Vec player_position;
    Vec player_scale;
    Vec jump_offset;
    float jump_angle;
    float base_scale;
    float jump_height;
    s16 frame;
    s16 player_model;
    s16 i;

    frame = BoardModelIDGet(lotteryMdl[0]);
    Hu3DModelObjMtxGet(frame, "toto_chibidai", player_transform);
    Hu3DMtxTransGet(player_transform, &player_position);
    BoardPlayerMotionStart(arg0, 4, 0);
    jump_angle = BoardPlayerMotionMaxTimeGet(arg0);
    BoardPlayerPosGet(arg0, &player_scale);
    player_position.y += 52.5308f;
    VECSubtract(&player_position, &player_scale, &jump_offset);
    jump_offset.x /= jump_angle;
    jump_offset.z /= jump_angle;
    jump_offset.y /= jump_angle;
    OSf32tos16(&jump_angle, &player_model);
    for (i = 0; i < player_model; i++) {
        OSs16tof32(&i, &base_scale);
        if (i < 4) {
            jump_height = 5.0f;
        } else {
            jump_height = 0.0f;
        }
        player_scale.x += jump_offset.x;
        player_scale.y += jump_offset.y + jump_height - 0.016666668f * base_scale * base_scale;
        player_scale.z += jump_offset.z;
        BoardPlayerPosSetV(arg0, &player_scale);
        HuPrcVSleep();
    }
    player_scale.y = player_position.y;
    BoardPlayerPosSetV(arg0, &player_scale);
    BoardPlayerIdleSet(arg0);
}

/**
 * @brief Animates a player descending from the lottery entrance jump.
 * @param arg0 Player index to move downward.
 */
static void DoMiniJumpDown(s32 arg0) {
    Vec player_position;
    Vec landing_position;
    Vec jump_offset;
    float jump_angle;
    float base_height;
    s16 i;

    BoardPlayerPosGet(arg0, &landing_position);
    BoardSpacePosGet(0, lotterySpace, &player_position);
    VECSubtract(&player_position, &landing_position, &jump_offset);
    BoardPlayerMotionStart(arg0, 4, 0);
    jump_offset.x /= 30.0f;
    jump_offset.z /= 30.0f;
    for (i = 0; i < 30; i++) {
        OSs16tof32(&i, &jump_angle);
        if (i < 3) {
            base_height = 10.0f;
        } else {
            base_height = 0.0f;
        }
        landing_position.x += jump_offset.x;
        landing_position.y += base_height + -0.016666668f * jump_angle * jump_angle * 0.75f;
        landing_position.z += jump_offset.z;
        if (player_position.y > landing_position.y) {
            landing_position.y = player_position.y;
            break;
        }
        BoardPlayerPosSetV(arg0, &landing_position);
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(arg0, 5, 0);
    BoardPlayerPosSetV(arg0, &player_position);
    while (!BoardPlayerMotionEndCheck(arg0)) {
        HuPrcVSleep();
    }
}

/**
 * @brief Deducts the lottery admission fee from a player one coin at a time.
 * @param arg0 Player index charged five coins.
 */
static void PayEnterFee(s32 arg0) {
    s32 message;
    s32 player;
    s32 i;

    player = GWSystem.player_curr;
    BoardStatusShowSetForce(player);
    BoardStatusShowSet(player, 1);
    if (arg0 != 0) {
        message = lotteryMessBase + 12;
    } else {
        message = lotteryMessBase;
    }
    BoardWinCreate(2, message, BoardWinPortraitGet());
    BoardWinWait();
    BoardWinKill();
    while (!BoardStatusStopCheck(player)) {
        HuPrcVSleep();
    }
    for (i = 0; i < 5; i++) {
        BoardPlayerCoinsAdd(player, -1);
        HuAudFXPlay(0xE);
        HuPrcSleep(6);
    }
    HuAudFXPlay(0xF);
    BoardStatusShowSet(player, 0);
}

/**
 * @brief Animates the current player's lottery ticket into view.
 */
static void ShowTicket(void) {
    s16 ticket_model;
    s16 ticket_motion;
    s16 ticket_sprite;
    s16 ticket_group;
    s8 ticket_index;
    float scale;
    s16 player_model;
    s32 player;
    s32 i;
    u8 *ticket_data;
    s32 ticket_resources[] = {
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 21),
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 22),
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 23),
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 24),
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 25),
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 26),
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 27),
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 28)
    };

    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        return;
    }
    ticket_index = GWPlayer[GWSystem.player_curr].ticket_player;
    ticket_data = (u8*) &ticket_index;
    player_model = HuSprGrpCreate(4);
    BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BKUJIYA, 20), 30001, 0, &ticket_group);
    HuSprGrpMemberSet(player_model, 0, ticket_group);
    HuSprAttrSet(player_model, 0, 9);
    HuSprPosSet(player_model, 0, 288.0f, 240.0f);
    HuSprTPLvlSet(player_model, 0, 0.0f);
    player = GWPlayer[(*ticket_data >> 4) & 3].character;
    BoardSpriteCreate(ticket_resources[player], 30000, 0, &ticket_sprite);
    HuSprGrpMemberSet(player_model, 1, ticket_sprite);
    HuSprAttrSet(player_model, 1, 9);
    HuSprPosSet(player_model, 1, 200.0f, 237.0f);
    HuSprTPLvlSet(player_model, 1, 0.0f);
    player = GWPlayer[(*ticket_data >> 2) & 3].character;
    BoardSpriteCreate(ticket_resources[player], 30000, 0, &ticket_motion);
    HuSprGrpMemberSet(player_model, 2, ticket_motion);
    HuSprAttrSet(player_model, 2, 9);
    HuSprPosSet(player_model, 2, 294.0f, 237.0f);
    HuSprTPLvlSet(player_model, 2, 0.0f);
    player = GWPlayer[*ticket_data & 3].character;
    BoardSpriteCreate(ticket_resources[player], 30000, 0, &ticket_model);
    HuSprGrpMemberSet(player_model, 3, ticket_model);
    HuSprAttrSet(player_model, 3, 9);
    HuSprPosSet(player_model, 3, 388.0f, 237.0f);
    HuSprTPLvlSet(player_model, 3, 0.0f);
    BoardWinCreate(2, lotteryMessBase + 10, BoardWinPortraitGet());
    BoardWinWait();
    for (scale = 0.0f; scale <= 1.0f; scale += 0.15f) {
        if (scale >= 1.0f) {
            scale = 1.0f;
        }
        for (i = 0; i < 4; i++) {
            HuSprTPLvlSet(player_model, i, scale);
        }
        HuPrcVSleep();
    }
    BoardWinCreate(2, lotteryMessBase + 11, BoardWinPortraitGet());
    BoardWinWait();
    BoardWinKill();
    for (scale = 1.0f; scale > 0.0f; scale -= 0.15f) {
        if (scale < 0.0f) {
            scale = 0.0f;
        }
        for (i = 0; i < 4; i++) {
            HuSprTPLvlSet(player_model, i, scale);
        }
        HuPrcVSleep();
    }
    HuSprGrpKill(player_model);
    (void) player; // Required to match
}

/**
 * @brief Assigns a random nonduplicating lottery-ticket code to a player.
 * @details The candidate is compared with ticket codes already assigned to the other players before it is stored and the player's draw-ticket flag is set.
 * @param arg0 Board-player index receiving the lottery ticket.
 */
static void SetupTicket(s32 arg0) {
    u8 existing_ticket_codes[4];
    u8 candidate_code;
    s32 candidate_attempt;
    s32 existing_ticket_count;
    s32 entry_index;

    for (existing_ticket_count = entry_index = 0; entry_index < 4; entry_index++) {
        if (arg0 != entry_index && GWPlayer[entry_index].draw_ticket != 0) {
            existing_ticket_codes[existing_ticket_count] = GWPlayer[entry_index].ticket_player;
            existing_ticket_count++;
        }
    }
    for (candidate_attempt = 0; candidate_attempt < 1000; candidate_attempt++) {
        candidate_code = BoardRandMod(64) & 0x3F;
        for (entry_index = 0; existing_ticket_count != 0 && entry_index < existing_ticket_count; entry_index++) {
            if (existing_ticket_codes[entry_index] == candidate_code) {
                candidate_code = 0x80;
                break;
            }
        }
        if (candidate_code != 0x80) {
            break;
        }
    }
    GWPlayer[arg0].ticket_player = candidate_code;
    GWPlayer[arg0].draw_ticket = 1;
}

/**
 * @brief Places a sprite group at one of the two scratch-ticket screen anchors.
 * @param arg0 Sprite-group handle to reposition.
 * @param arg1 Anchor index selecting the normal or focused ticket position.
 */
static void LotteryInlineFunc00(s32 arg0, s32 arg1) {
    s32 anchor_resources[2] = {
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 18),
        DATA_MAKE_NUM(DATADIR_BKUJIYA, 19)
    };
    s16 anchor_x[2] = { 152, 120 };
    s16 anchor_y[2] = { 120, 120 };
    Vec screen_position;
    float interpolation;
    s16 anchor_index;

    screen_position.x = (anchor_y[arg0] / 2) + 32;
    if (arg1 == 0) {
        screen_position.y = -anchor_x[arg0] / 2;
        interpolation = 4.0f;
        if (lotterySpr == -1) {
            lotterySpr = espEntry(anchor_resources[arg0], 30001, 0);
            espAttrSet(lotterySpr, HUSPR_ATTR_NOANIM);
        }
    } else {
        screen_position.y = (anchor_x[arg0] / 2) + 40;
        interpolation = -4.0f;
    }
    for (anchor_index = anchor_x[arg0] + 40; anchor_index > 0; anchor_index -= 4) {
        screen_position.y += interpolation;
        espPosSet(lotterySpr, screen_position.x, screen_position.y);
        HuPrcVSleep();
    }
    if (arg1 != 0 && lotterySpr >= 0) {
        espKill(lotterySpr);
        lotterySpr = -1;
    }
}

/**
 * @brief Runs the crank-controlled lottery ball drawing sequence.
 */
static void ExecBallGame(void) {
    Vec crank_position;
    float crank_angle;
    float angle_step;
    float ball_angle;
    float ball_speed;
    float handle_angle;
    float handle_speed;
    float player_angle;
    s32 frame;
    s32 ball_index;
    s32 turn_count = 0;
    s32 player_model;
    s32 controller_port;
    s32 player;
    u8 input_index;
    s8 stick_x;
    s8 stick_y;
    s8 previous_stick_x;
    s8 previous_stick_y;
    s8 *cpu_input;
    s32 ball_resources[] = {
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 29),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 29),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 29),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 29),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 29),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 29),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 29),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 29)
    };

    player = GWSystem.player_curr;
    currPrize = -1;
    crank_position.x = 0.0f;
    crank_position.y = 210.0f;
    crank_position.z = 0.0f;
    BoardCameraMotionStartEx(lotteryMdl[0], NULL, &crank_position, 980.0f, -1.0f, 21);
    LotteryInlineFunc00(0, 0);
    BoardCameraMotionWait();
    BoardWinCreate(2, lotteryMessBase + 1, BoardWinPortraitGet());
    BoardWinWait();
    BoardWinKill();
    HuPrcSleep(0x12);
    CreateLotteryWin(0);
    turnMot = BoardPlayerMotionCreate(player, ball_resources[GWPlayer[player].character]);
    BoardPlayerMotionSpeedSet(player, 0.0f);
    BoardModelAttrSet(lotteryMdl[1], 0x40000001);
    BoardModelMotionSpeedSet(lotteryMdl[1], 0.0f);
    BoardModelMotionTimeSet(lotteryMdl[1], 0.0f);
    BoardModelAttrSet(gripMdl[0], 0x40000001);
    BoardModelAttrSet(gripMdl[1], 0x40000001);
    BoardPlayerMotionShiftSet(player, turnMot, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    BoardPlayerMotionSpeedSet(player, 0.0f);
    crank_angle = BoardModelMotionMaxTimeGet(lotteryMdl[1]);
    crank_angle -= 4.0f;
    comInputPos = 0;
    turn_count = 0;
    stick_y = previous_stick_x = previous_stick_y = stick_x = 0;
    input_index = player_angle = handle_angle = ball_angle = angle_step = ball_index = 0;
    HuPrcSleep(24);
    controller_port = frame = 0;
    while (1) {
        HuPrcVSleep();
        BoardModelMotionSpeedSet(lotteryMdl[1], player_angle);
        BoardModelMotionSpeedSet(gripMdl[0], player_angle);
        BoardModelMotionSpeedSet(gripMdl[1], player_angle);
        BoardPlayerMotionSpeedSet(player, player_angle);
        if (frame != 0 && player_angle <= 0.1f) {
            BoardModelMotionSpeedSet(lotteryMdl[1], 0.0f);
            BoardModelMotionSpeedSet(gripMdl[0], 0.0f);
            BoardModelMotionSpeedSet(gripMdl[1], 0.0f);
            BoardPlayerMotionSpeedSet(player, 0.0f);
            break;
        }
        if (controller_port == 0) {
            if (BoardModelMotionTimeGet(lotteryMdl[1]) >= crank_angle) {
                ball_index++;
                if ((input_index++) != 0 || ball_index >= 1 || turn_count != 0) {
                    frame = 1;
                    if (currPrize == -1) {
                        handle_angle *= 128.0f;
                        OSf32tos8(&handle_angle, &lbl_801D401B);
                        SetBallPrize();
                    }
                }
                controller_port = 1;
            }
        }
        if (controller_port != 0 && BoardModelMotionTimeGet(lotteryMdl[1]) < 2.0f) {
            controller_port = 0;
        }
        if (frame == 0) {
            stick_x = stick_y;
            previous_stick_y = previous_stick_x;
            if (!GWPlayer[player].com) {
                player_model = GWPlayer[player].port;
                stick_y = HuPadStkX[player_model];
                previous_stick_x = HuPadStkY[player_model];
            } else {
                cpu_input = comInputBall[comInputPos++];
                stick_y = cpu_input[0];
                previous_stick_x = cpu_input[1];
            }
        } else {
            stick_y = previous_stick_x = 0;
        }
        if (stick_y == 0x7F || previous_stick_x == 0x7F) {
            turn_count = 1;
        } else {
            if ((abs(stick_y) < 57.600002f && abs(previous_stick_x) < 57.600002f) || (stick_x == stick_y && previous_stick_y == previous_stick_x) || (stick_y == 0 && previous_stick_x == 0)) {
                player_angle *= 0.93f;
                handle_angle *= 0.8f;
                if (handle_angle < 0.01f && input_index != 0) {
                    input_index--;
                }
            } else {
                OSs8tof32(&stick_y, &ball_speed);
                OSs8tof32(&previous_stick_x, &handle_speed);
                ball_angle = atan2d(ball_speed, handle_speed);
                if (ball_angle < 0.0f) {
                    ball_angle += 360.0f;
                }
                if (ball_angle < 20.0f) {
                    HuAudFXPlay(0x333);
                }
                ball_speed = ball_angle - angle_step;
                if (!(ball_speed < 0.0f)) {
                    if (ABS(ball_speed) > 30.0f) {
                        ball_speed = 10.0f;
                    }
                    handle_angle += 0.01f * ball_speed;
                    if (player_angle + handle_angle < 2.0f) {
                        player_angle += handle_angle;
                    } else {
                        player_angle = 2.0f;
                    }
                }
            }
        }
        angle_step = ball_angle;
    }
    KillLotteryWin();
    LotteryInlineFunc00(0, 1);
}

/**
 * @brief Randomly chooses and stores the ball-game prize tier.
 */
static void SetBallPrize(void) {
    HUPROCESS *prize_process;
    s32 prize_roll;

    while (1) {
        prize_roll = BoardRandMod(100);
        if (prize_roll > 97) {
            currPrize = 0;
        } else if (prize_roll > 87) {
            currPrize = 1;
        } else if (prize_roll > 67) {
            currPrize = 2;
        } else {
            currPrize = 3;
        }
        if (GWSystem.max_turn - GWSystem.turn != 0 || currPrize != 2) {
            break;
        }
    }
    prize_process = HuPrcChildCreate(ExecBallPrize, 0x2004, 0x3800, 0, lotteryProc);
}

/**
 * @brief Animates the selected lottery ball and announces its prize.
 */
static void ExecBallPrize(void) {
    Vec ball_position;
    Vec host_position;
    Vec camera_rotation;
    Vec camera_offset;
    float height;
    float fall_speed;
    float bounce_speed;
    float rotation_speed;
    float angle;
    float bounce_height;
    s32 frame;
    s16 ball_model;
    s16 ball_motion;
    s16 bounce_count;
    HSFOBJECT *model_data;
    Mtx ball_transform;
    Mtx camera_transform;

    frame = 0;
    HuAudFXPlay(0x34C);
    ball_model = ballMdl[currPrize & 3];
    BoardModelPosGet(lotteryMdl[0], &ball_position);
    BoardModelRotGet(lotteryMdl[0], &camera_rotation);
    PSMTXRotRad(ball_transform, 'Y', MTXDegToRad(camera_rotation.y));
    ball_motion = BoardModelIDGet(lotteryMdl[0]);
    model_data = Hu3DModelObjPtrGet(ball_motion, "toto_gara");
    fall_speed = ball_position.y + model_data->mesh.curr.pos.y + 100.0f;
    PSMTXTrans(camera_transform, model_data->mesh.curr.pos.x, model_data->mesh.curr.pos.y + 210.0f, model_data->mesh.curr.pos.z + -40.0f);
    PSMTXConcat(ball_transform, camera_transform, camera_transform);
    Hu3DMtxTransGet(camera_transform, &camera_offset);
    VECAdd(&camera_offset, &ball_position, &ball_position);
    BoardModelVisibilitySet(ball_model, 1);
    host_position.x = 10.5f;
    host_position.y = -3.0f;
    host_position.z = 0.0f;
    PSMTXMultVec(ball_transform, &host_position, &host_position);
    angle = 0.0f;
    bounce_height = 1.8f;
    bounce_speed = 0.016666668f;
    bounce_count = 0;
    while (1) {
        OSs16tof32(&bounce_count, &height);
        rotation_speed = 4.9f * (2.0f * height - bounce_speed) * bounce_speed;
        host_position.y -= rotation_speed;
        host_position.x *= 0.92f;
        host_position.z *= 0.92f;
        VECAdd(&host_position, &ball_position, &ball_position);
        angle += host_position.z;
        if (ball_position.y - 25.0f < fall_speed) {
            if (frame == 0) {
                HuAudFXPlay(0x334);
                frame = 1;
            }
            ball_position.y = 25.0f + fall_speed + rotation_speed * bounce_height;
            if (bounce_height > 0.0f) {
                bounce_height -= 0.1f;
            }
        }
        BoardModelPosSetV(ball_model, &ball_position);
        BoardModelRotSet(ball_model, 0.0f, angle, 0.0f);
        HuPrcVSleep();
        bounce_count++;
    }
}

static const s32 pickSpr[] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 30),
    DATA_MAKE_NUM(DATADIR_BOARD, 31),
    DATA_MAKE_NUM(DATADIR_BOARD, 32),
    DATA_MAKE_NUM(DATADIR_BOARD, 33),
    DATA_MAKE_NUM(DATADIR_BOARD, 34),
    DATA_MAKE_NUM(DATADIR_BOARD, 35),
    DATA_MAKE_NUM(DATADIR_BOARD, 36),
    DATA_MAKE_NUM(DATADIR_BOARD, 37)
};

static const s32 handMdl[] = {
    DATA_MAKE_NUM(DATADIR_BKUJIYA,  9),
    DATA_MAKE_NUM(DATADIR_BKUJIYA, 10),
    DATA_MAKE_NUM(DATADIR_BKUJIYA, 11),
    DATA_MAKE_NUM(DATADIR_BKUJIYA, 12),
    DATA_MAKE_NUM(DATADIR_BKUJIYA, 13),
    DATA_MAKE_NUM(DATADIR_BKUJIYA, 14),
    DATA_MAKE_NUM(DATADIR_BKUJIYA, 15),
    DATA_MAKE_NUM(DATADIR_BKUJIYA, 16)
};

static const s32 ticketSpr[] = {
    DATA_MAKE_NUM(DATADIR_BKUJIYA, 29),
    DATA_MAKE_NUM(DATADIR_BKUJIYA, 30),
    DATA_MAKE_NUM(DATADIR_BKUJIYA, 31),
    DATA_MAKE_NUM(DATADIR_BKUJIYA, 32)
};

static OMOBJ *ticketObj[12] = { NULL };
static s8 ticketPrize[12] = { 0 };
static Vec handLastPos = { 0.0f, 0.0f, 0.0f };

static s32 loseSoundTbl[] = {
    0x0000012E,
    0x0000016E,
    0x000001AE,
    0x000001EE,
    0x0000022E,
    0x0000026E,
    0x000002AE,
    0x000002EE
};

static s32 loseMotTbl[] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 51),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 51),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 51),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 51),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 51),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 51),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 51),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 51)
};

/**
 * @brief Runs scratch-ticket setup, play, result evaluation, and cleanup.
 * @param arg0 Scratch-ticket grid index in the range 0 through 11.
 */
static void ExecScratchTicket(s32 arg0) {
    Vec cursor_position;
    Vec hand_position;
    Vec camera_direction;
    u16 pixel_x;
    u16 pixel_y;
    s16 *pixel_data;
    s32 character;
    s32 sound;
    u16 pixel_offset;
    OMOBJ *ticket_object;
    TicketWork *ticket_work;
    ANIMBMP *ticket_bitmap;

    character = GWPlayer[GWSystem.player_curr].character;
    ticket_object = ticketObj[arg0];
    ticket_work = omObjGetWork(ticket_object, TicketWork);
    lotteryMdl[4] = BoardModelCreate(handMdl[character], NULL, 0);
    BoardModelLayerSet(lotteryMdl[4], 6);
    BoardModelPassSet(lotteryMdl[4], 0);
    BoardCameraDirGet(&camera_direction);
    BoardModelRotYSet(lotteryMdl[4], atan2d(-camera_direction.x, -camera_direction.z));
    BoardModelScaleSet(lotteryMdl[4], 3.0f, 3.0f, 3.0f);
    cursor_position.x = 320.0f;
    cursor_position.y = 480.0f;
    cursor_position.z = 500.0f;
    CreateLotteryWin(1);
    while (1) {
        HuPrcVSleep();
        if (cursor_position.y <= 240.0f) {
            break;
        }
        cursor_position.y -= 4.0f;
        Hu3D2Dto3D(&cursor_position, 1, &hand_position);
        BoardModelPosSetV(lotteryMdl[4], &hand_position);
    }
    comInputPos = 0;
    if (BoardRandMod(100) < 50) {
        comInputDrawP = comInputDraw1;
    } else {
        comInputDrawP = comInputDraw2;
    }
    sound = HuAudFXPlay(0x335);
    HuAudFXVolSet(sound, 0);
    while (1) {
        HuPrcVSleep();
        if (TicketUpdate(ticket_work->cover_anim->bmp, &cursor_position, sound) != 0) {
            break;
        }
        if (ScratchTicketCheckDone(ticket_work->cover_anim->bmp)) {
            break;
        }
    }
    HuAudFXStop(sound);
    while (1) {
        HuPrcVSleep();
        if (cursor_position.y >= 480.0f) {
            break;
        }
        cursor_position.y += 4.0f;
        Hu3D2Dto3D(&cursor_position, 1, &hand_position);
        BoardModelPosSetV(lotteryMdl[4], &hand_position);
    }
    BoardModelVisibilitySet(lotteryMdl[4], 0);
    ticket_bitmap = ticket_work->cover_anim->bmp;
    pixel_data = ticket_bitmap->data;
    for (pixel_y = 0; pixel_y < ticket_bitmap->sizeY; pixel_y++) {
        for (pixel_x = 0; pixel_x < ticket_bitmap->sizeX; pixel_x++) {
            pixel_offset = TicketGetPixel(ticket_bitmap->sizeX, pixel_x, pixel_y);
            pixel_data[pixel_offset] = 0;
        }
        HuPrcVSleep();
    }
    KillLotteryWin();
    BoardModelKill(lotteryMdl[4]);
    lotteryMdl[4] = -1;
}

/**
 * @brief Processes scratching until enough ticket area is uncovered or play is aborted.
 */
static void ExecScratch(void) {
    TicketWork *ticket_work;
    float opacity;
    s32 player;
    s32 selected_ticket;

    currPrize = -1;
    BoardFilterFadeInit(30, 0xA0);
    InitTicketPrizes();
    InitScratchSpr();
    player = GWSystem.player_curr;
    if (GWPlayer[player].com) {
        comLotteryType = BoardRandMod(4);
        comLotteryWinType = BoardRandMod(3);
    }
    BoardWinCreate(2, lotteryMessBase + 9, BoardWinPortraitGet());
    BoardWinWait();
    BoardWinKill();
    CreateLotteryWin(2);
    InitScratchPick();
    while (currPrize == -1) {
        HuPrcVSleep();
    }
    selected_ticket = currPrize;
    currPrize = ticketPrize[selected_ticket];
    ExecTicketFocus(selected_ticket);
    KillLotteryWin();
    LotteryInlineFunc00(1, 0);
    ExecScratchTicket(selected_ticket);
    LotteryInlineFunc00(1, 1);
    ticket_work = omObjGetWork(ticketObj[selected_ticket], TicketWork);
    for (opacity = 1.0f; opacity > 0.0f; opacity -= 0.078125f) {
        if (opacity < 0.0f) {
            opacity = 0.0f;
        }
        HuSprTPLvlSet(ticketSprGrp, ticket_work->sprite_index, opacity);
        HuPrcVSleep();
    }
    HideScratchSpr();
    BoardFilterFadeOut(30);
}

/**
 * @brief Marks the scratch cursor and ticket objects for destruction.
 */
static void KillScratch(void) {
    if (ticketSprGrp != -1) {
        HuSprGrpKill(ticketSprGrp);
        ticketSprGrp = -1;
    }
    if (lotteryTicketPickObj) {
        omObjGetWork(lotteryTicketPickObj, LotteryTicketPickWork)->kill = 1;
    }
    memset(ticketObj, 0, sizeof(ticketObj));
}

/**
 * @brief Updates the scratch cursor position and erases ticket pixels beneath it.
 * @param arg0 Object controlling the visible scratch cursor.
 */
static void ExecScratchSpr(OMOBJ *arg0) {
    Vec target_position;
    Vec current_position;
    Vec position_delta;
    s32 sprite_index;
    TicketWork *ticket_work;

    ticket_work = omObjGetWork(arg0, TicketWork);
    if (ticket_work->kill != 0 || BoardIsKill()) {
        sprite_index = ticket_work->sprite_index;
        HuSprAttrSet(ticketSprGrp, sprite_index, 4);
        HuSprAttrSet(ticketSprGrp, sprite_index + 12, 4);
        ticketObj[sprite_index] = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    if (ticket_work->delay != 0) {
        ticket_work->delay--;
        return;
    }
    current_position.x = arg0->trans.x;
    current_position.y = arg0->trans.y;
    target_position.x = arg0->rot.x;
    target_position.y = arg0->rot.y;
    VECSubtract(&current_position, &target_position, &position_delta);
    if (ABS(position_delta.x) < 1.0f && ABS(position_delta.y) < 1.0f) {
        position_delta = current_position;
        ticket_work->at_target = 1;
    } else {
        VECScale(&position_delta, &position_delta, 0.2f);
        VECAdd(&target_position, &position_delta, &position_delta);
        ticket_work->at_target = 0;
    }
    HuSprPosSet(ticketSprGrp, ticket_work->sprite_index, position_delta.x, position_delta.y);
    HuSprPosSet(ticketSprGrp, ticket_work->sprite_index + 12, position_delta.x, position_delta.y);
    arg0->rot.x = position_delta.x;
    arg0->rot.y = position_delta.y;
}

/**
 * @brief Hides the scratch cursor sprite.
 */
static void HideScratchSpr(void) {
    s32 i;

    for (i = 0; i < 12; i++) {
        if (ticketObj[i]) {
            omObjGetWork(ticketObj[i], TicketWork)->kill = 1;
        }
    }
}

/**
 * @brief Creates and positions the scratch cursor sprite object.
 */
static void InitScratchSpr(void) {
    Vec player_rotation;
    Vec camera_rotation;
    OMOBJ *ticket_object;
    TicketWork *ticket_work;
    s32 player;
    s16 sprite_group;
    s16 sprite;
    s16 i;

    player = GWSystem.player_curr;
    BoardPlayerRotGet(player, &player_rotation);
    memset(ticketObj, 0, sizeof(ticketObj));
    BoardCameraRotGet(&camera_rotation);
    sprite_group = HuSprGrpCreate(24);
    ticketSprGrp = sprite_group;
    for (i = 0; i < 12; i++) {
        ticket_object = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, ExecScratchSpr);
        ticketObj[i] = ticket_object;
        ticket_work = omObjGetWork(ticket_object, TicketWork);
        ticket_work->kill = 0;
        ticket_work->sprite_index = i;
        ticket_work->delay = (i / 4) * 2;
        ticket_work->prize_sprite = ticket_work->cover_sprite = -1;
        ticket_object->trans.x = 107.0f + 110.0f * (i % 4);
        ticket_object->trans.y = 98.0f + 110.0f * (i / 4);
        ticket_object->rot.x = -56.0f;
        ticket_object->rot.y = ticket_object->trans.y;
        BoardSpriteCreate(ticketSpr[ticketPrize[i]], 5500, NULL, &sprite);
        HuSprGrpMemberSet(sprite_group, i, sprite);
        HuSprPosSet(sprite_group, i, ticket_object->rot.x, ticket_object->rot.y);
        HuSprAttrSet(sprite_group, i, 8);
        ticket_work->prize_sprite = sprite;
        BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BKUJIYA, 33), 5000, &ticket_work->cover_anim, &sprite);
        HuSprGrpMemberSet(sprite_group, i + 12, sprite);
        HuSprPosSet(sprite_group, i + 12, ticket_object->rot.x, ticket_object->rot.y);
        HuSprAttrSet(sprite_group, i + 12, 8);
        ticket_work->cover_sprite = sprite;
    }
    HuSprGrpDrawNoSet(sprite_group, 0x40);
}

/**
 * @brief Builds the direction-button mask for scratch-ticket selection input.
 * @param picker_work Scratch-picker state containing the current grid position.
 * @return Bit mask of directional buttons to apply this frame.
 */
static inline u32 ExecStratchPickInlineFunc(LotteryTicketPickWork *picker_work) {
    s32 player;
    s32 controller_port;
    u32 input;

    player = GWSystem.player_curr;
    input = 0;
    if (!GWPlayer[player].com) {
        controller_port = GWPlayer[GWSystem.player_curr].port;
        input = HuPadDStkRep[controller_port] | HuPadBtnDown[controller_port];
    } else if (comLotteryType != picker_work->column) {
        input = 2;
    } else if (comLotteryWinType != picker_work->row) {
        input = 4;
    } else if (comLotteryType == picker_work->column && comLotteryWinType == picker_work->row) {
        input = 0x100;
    }
    return input;
}

/**
 * @brief Moves the scratch-ticket selector and accepts a ticket choice.
 * @param arg0 Object controlling the scratch-ticket selection cursor.
 */
static void ExecScratchPick(OMOBJ *arg0) {
    float cursor_x;
    float cursor_y;
    s8 horizontal_step;
    s8 vertical_step;
    s8 row_step;
    u32 input;
    LotteryTicketPickWork *picker_work;

    picker_work = omObjGetWork(arg0, LotteryTicketPickWork);
    if (picker_work->kill || BoardIsKill()) {
        HuSprGrpKill(picker_work->sprite_group);
        lotteryTicketPickObj = 0;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    if (picker_work->input_delay != 0) {
        picker_work->input_delay--;
        return;
    }
    vertical_step = row_step = 0;
    input = ExecStratchPickInlineFunc(picker_work);
    if (input == 0x100) {
        currPrize = (picker_work->column + (picker_work->row * 4));
        picker_work->kill = 1;
        HuAudFXPlay(2);
        return;
    }
    horizontal_step = 0;
    if (input == 8) {
        row_step = -1;
        horizontal_step = 1;
    }
    if (input == 4) {
        row_step = 1;
        horizontal_step = 1;
    }
    if (input == 1) {
        vertical_step = -1;
        horizontal_step = 1;
    }
    if (input == 2) {
        vertical_step = 1;
        horizontal_step = 1;
    }
    if ((vertical_step != 0 || row_step != 0) && GWPlayer[GWSystem.player_curr].com) {
        picker_work->input_delay = 0x14;
    }
    picker_work->column += vertical_step;
    picker_work->row += row_step;
    if (picker_work->column < 0) {
        picker_work->column = 0;
        horizontal_step = 0;
    } else if (picker_work->row < 0) {
        picker_work->row = 0;
        horizontal_step = 0;
    }
    if (picker_work->column >= 4) {
        picker_work->column = 3;
        horizontal_step = 0;
    } else if (picker_work->row >= 3) {
        picker_work->row = 2;
        horizontal_step = 0;
    }
    if (horizontal_step != 0) {
        HuAudFXPlay(0);
    }
    OSs8tof32(&picker_work->column, &cursor_x);
    OSs8tof32(&picker_work->row, &cursor_y);
    cursor_x = 107.0f + 110.0f * cursor_x - 32.0f + 16.0f;
    cursor_y = 98.0f + 110.0f * cursor_y + 32.0f - 24.0f;
    HuSprPosSet(picker_work->sprite_group, 0, cursor_x, cursor_y);
}

/**
 * @brief Creates the scratch-ticket selection cursor object.
 */
static void InitScratchPick(void) {
    float cursor_x = 91.0f;
    float cursor_y = 106.0f;
    OMOBJ *picker_object;
    LotteryTicketPickWork *picker_work;

    picker_object = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, ExecScratchPick);
    lotteryTicketPickObj = picker_object;
    picker_work = omObjGetWork(picker_object, LotteryTicketPickWork);
    picker_work->column = picker_work->row = 0;
    picker_work->input_delay = 6;
    picker_work->kill = 0;
    picker_work->character = GWPlayer[GWSystem.player_curr].character;
    picker_work->sprite_group = HuSprGrpCreate(1);
    BoardSpriteCreate(pickSpr[picker_work->character], 5000, 0, &picker_work->sprite);
    HuSprGrpMemberSet(picker_work->sprite_group, 0, picker_work->sprite);
    HuSprAttrSet(picker_work->sprite_group, 0, 8);
    HuSprPosSet(picker_work->sprite_group, 0, cursor_x, cursor_y);
    HuSprGrpDrawNoSet(picker_work->sprite_group, 0x40);
}

/**
 * @brief Shuffles the three scratch-ticket prize assignments.
 */
static void InitTicketPrizes(void) {
    s32 swap_value;
    s32 i;

    memset(ticketPrize, 3, sizeof(ticketPrize));
    for (i = 0; i < 3; i++) {
        do {
            swap_value = BoardRandMod(12);
        } while (ticketPrize[swap_value] != 3);
        ticketPrize[swap_value] = i;
        if (GWSystem.max_turn - GWSystem.turn == 0 && i == 2) {
            ticketPrize[swap_value] = 3;
        }
    }
}

/**
 * @brief Moves the selected scratch ticket to or from the focused position.
 * @param arg0 Selected scratch-ticket index in the range 0 through 11.
 */
static void ExecTicketFocus(s32 arg0) {
    float phase_angle;
    float ticket_scale;
    OMOBJ *ticket_object;
    TicketWork *ticket_work;
    s32 i;

    ticket_object = ticketObj[arg0];
    ticket_object->trans.x = 288.0f;
    ticket_object->trans.y = 240.0f;
    for (i = 0; i < 12; i++) {
        if (i != arg0) {
            omObjGetWork(ticketObj[i], TicketWork)->kill = 1;
        }
    }
    ticket_work = omObjGetWork(ticketObj[arg0], TicketWork);
    for (phase_angle = 0.0f; phase_angle < 90.0f; phase_angle += 2.0f) {
        ticket_scale = 1.0 + 1.3f * sind(phase_angle);
        HuSprScaleSet(ticketSprGrp, ticket_work->sprite_index, ticket_scale, ticket_scale);
        HuSprScaleSet(ticketSprGrp, ticket_work->sprite_index + 12, ticket_scale, ticket_scale);
        HuPrcVSleep();
    }
}

/**
 * @brief Checks whether the required fraction of scratchable pixels is uncovered.
 * @param arg0 Bitmap whose scratchable pixels are counted.
 * @return Nonzero once the uncovered pixel threshold is reached; otherwise zero.
 */
static BOOL ScratchTicketCheckDone(ANIMBMP *arg0) {
    s32 erased_count;
    s32 column;
    s32 row;
    u16 *pixel_data;

    pixel_data = arg0->data;
    for (erased_count = row = 0; row < arg0->sizeY; row++) {
        for (column = 0; column < arg0->sizeX; column++) {
            if (pixel_data[column + row * arg0->sizeX] == 0) {
                erased_count++;
            }
        }
    }
    if (erased_count >= arg0->sizeX * arg0->sizeY - 300) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * @brief Calculates the swizzled texture-data index for a ticket pixel.
 * @param arg0 Texture width in pixels.
 * @param arg1 Pixel X coordinate.
 * @param arg2 Pixel Y coordinate.
 * @return Element index for the requested pixel in the tiled texture data.
 */
static u16 TicketGetPixel(u16 arg0, u16 arg1, u16 arg2) {
    u16 tile_x;
    u16 tile_y;
    u16 local_x;
    u16 local_y;

    local_x = (arg1 & 3);
    local_y = (arg2 & 3);
    tile_x = (arg1 >> 2);
    tile_y = (arg2 >> 2);
    return local_x + (local_y << 2) + ((tile_x + tile_y * (arg0 >> 2)) << 4);
}

/**
 * @brief Erases scratch-ticket pixels covered by the cursor and updates the texture.
 * @param arg0 Mutable ticket bitmap to erase.
 * @param arg1 Scratch cursor position in ticket coordinates.
 * @param arg2 Sound-effect handle whose volume follows scratch input strength.
 * @return -1 when scripted CPU input is exhausted; otherwise zero.
 */
static s32 TicketUpdate(ANIMBMP *arg0, Vec *arg1, s32 arg2) {
    Vec input_direction;
    Vec next_cursor;
    float input_strength;
    float angle;
    s32 cursor_clamped;
    s16 scratch_x;
    s32 brush_x;
    s32 brush_y;
    s8 *cpu_input;
    s16 *texture_data;
    s32 controller_port;
    s16 scratch_y;
    s16 texture_width;
    s16 texture_index;
    s8 sound_volume;

    if (GWPlayer[GWSystem.player_curr].com) {
        cpu_input = comInputDrawP[comInputPos++];
        if (cpu_input[0] == 0x7F && cpu_input[1] == 0x7F) {
            return -1;
        }
        OSs8tof32(&cpu_input[0], &input_direction.x);
        OSs8tof32(&cpu_input[1], &input_direction.y);
    } else {
        controller_port = GWPlayer[GWSystem.player_curr].port;
        input_direction.x = HuPadStkX[controller_port];
        input_direction.y = HuPadStkY[controller_port];
    }
    input_direction.z = 0.0f;
    if (input_direction.x == 0.0f && input_direction.y == 0.0f) {
        handLastPos = *arg1;
        handUpdateF = 0;
        HuAudFXVolSet(arg2, 0);
        return 0;
    }
    if (handUpdateF == 0) {
        handUpdateF = 1;
    }
    VECNormalize(&input_direction, &input_direction);
    angle = atan2f(input_direction.x, -input_direction.y);
    input_direction.x = sin(angle);
    input_direction.y = cos(angle);
    input_strength = 127.0f * VECMag(&input_direction);
    OSf32tos8(&input_strength, &sound_volume);
    HuAudFXVolSet(arg2, sound_volume);
    input_direction.x *= 8.0f;
    input_direction.y *= 8.0f;
    VECAdd(&input_direction, arg1, &next_cursor);
    cursor_clamped = 0;
    if (next_cursor.x < 192.0f) {
        next_cursor.x = 192.0f;
        cursor_clamped = 1;
    }
    if (next_cursor.y < 192.0f) {
        next_cursor.y = 192.0f;
        cursor_clamped = 1;
    }
    if (next_cursor.x >= 384.0f) {
        next_cursor.x = 384.0f;
        cursor_clamped = 1;
    }
    if (next_cursor.y >= 288.0f) {
        next_cursor.y = 288.0f;
        cursor_clamped = 1;
    }
    if (cursor_clamped != 0) {
        HuAudFXVolSet(arg2, 0);
    }
    *arg1 = next_cursor;
    Hu3D2Dto3D(&next_cursor, 1, &next_cursor);
    BoardModelPosSetV(lotteryMdl[4], &next_cursor);
    next_cursor = *arg1;
    next_cursor.x -= 192.0f;
    next_cursor.y -= 192.0f;
    next_cursor.x *= arg0->sizeX / 192.0f;
    next_cursor.y *= arg0->sizeY / 96.0f;
    texture_width = arg0->sizeX;
    texture_data = arg0->data;
    for (brush_y = -8; brush_y < 9; brush_y++) {
        for (brush_x = -8; brush_x < 9; brush_x++) {
            OSf32tos16(&next_cursor.x, &scratch_x);
            OSf32tos16(&next_cursor.y, &scratch_y);
            if (scratch_x + brush_x < arg0->sizeX && scratch_y + brush_y < arg0->sizeY && scratch_x + brush_x >= 0 && scratch_y + brush_y >= 0) {
                scratch_x += brush_x;
                scratch_y += brush_y;
                texture_index = TicketGetPixel(texture_width, scratch_x, scratch_y);
                if (texture_index < arg0->sizeX * arg0->sizeY) {
                    texture_data[texture_index] = 0;
                }
            }
        }
    }
    DCFlushRange(arg0->data, (u32) (arg0->sizeX * arg0->sizeY * 2) >> 3);
    handLastPos = *arg1;
    return 0;
}

/**
 * @brief Calculates the coin award for the current lottery prize tier.
 * @return Coin count associated with the current prize tier.
 */
static s32 ExecCoinPrizeInlineFunc(void) {
    s32 i;

    for (i = 0; i < 10; i++) {
        if (coinF[i] == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Animates lottery coins and credits them to the current player.
 */
static void ExecCoinPrize(void) {
    Vec coin_positions[10];
    Vec coin_position;
    float coin_velocities[10];
    float coin_heights[10];
    float landing_height;
    s32 coins_active;
    s32 coins_remaining;
    s32 player;
    s32 free_coin_slot;
    s32 i;

    player = GWSystem.player_curr;
    if (currPrize == 0) {
        coins_remaining = 100;
    } else {
        coins_remaining = 30;
    }
    memset(coinMdl, 0, sizeof(coinMdl));
    memset(coinF, 0, 10);
    BoardPlayerPosGet(player, &coin_position);
    landing_height = 150.0f + coin_position.y;
    if (BoardPlayerSizeGet(player) == 1) {
        landing_height = 52.5308f + 0.3f * landing_height + 110.0f;
    }
    for (i = 0; i < 10; i++) {
        coinMdl[i] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 10), NULL, 0);
        BoardModelVisibilitySet(coinMdl[i], 0);
    }
    while (1) {
        HuPrcVSleep();
        if (coins_remaining > 0) {
            free_coin_slot = ExecCoinPrizeInlineFunc();
            if (free_coin_slot != -1) {
                coinF[free_coin_slot] = 1;
                coin_positions[free_coin_slot].x = coin_position.x + 100.0f * (BoardRandFloat() - 0.5f);
                coin_positions[free_coin_slot].y = coin_position.y + 700.0f;
                coin_positions[free_coin_slot].z = coin_position.z + 100.0f * (BoardRandFloat() - 0.5f);
                coin_heights[free_coin_slot] = -20.0f + -10.0f * BoardRandFloat();
                coin_velocities[free_coin_slot] = BoardRandMod(360);
                BoardModelVisibilitySet(coinMdl[free_coin_slot], 1);
                BoardModelScaleSet(coinMdl[free_coin_slot], 0.5f, 0.5f, 0.5f);
                coins_remaining--;
            }
        } else {
            coins_active = 0;
            for (i = 0; i < 10; i++) {
                if (coinF[i] != 0) {
                    coins_active = 1;
                }
            }
            if (coins_active == 0) {
                return;
            }
        }
        for (i = 0; i < 10; i++) {
            if (coinF[i] != 0) {
                if (coin_positions[i].y < landing_height) {
                    coin_positions[i].y = landing_height;
                    CharEffectLayerSet(2);
                    CharEffectCoinGlowCreate(1, &coin_positions[i]);
                    BoardModelVisibilitySet(coinMdl[i], 0);
                    coinF[i] = 0;
                    BoardPlayerCoinsAdd(player, 1);
                    HuAudFXPlay(7);
                    omVibrate(player, 12, 6, 6);
                } else {
                    BoardModelPosSetV(coinMdl[i], &coin_positions[i]);
                    BoardModelRotSet(coinMdl[i], 0.0f, coin_velocities[i], 0.0f);
                    coin_velocities[i] = BoardDAngleCalc(45.0f + coin_velocities[i]);
                    coin_positions[i].y += coin_heights[i];
                }
            }
        }
    }
}

/**
 * @brief Animates the lottery item prize and adds it to the current player.
 */
static void ExecItemPrize(void) {
    Vec item_position;
    Vec player_position;
    float vertical_step;
    float shrink_phase;
    float item_angle;
    float item_scale;
    s32 sound;
    s32 prize_type;
    s32 player;
    s16 *prize_model;
    s16 i;
    s32 item_resources[2] = {
        DATA_MAKE_NUM(DATADIR_BOARD, 111),
        DATA_MAKE_NUM(DATADIR_BOARD, 112)
    };
    s8 item_ids[2] = { 2, 3 };

    player = GWSystem.player_curr;
    prize_type = (BoardRandFloat() < 0.5f) ? 1 : 0;
    memset(coinMdl, 0, sizeof(coinMdl));
    prize_model = &coinMdl[0];
    *prize_model = BoardModelCreate(item_resources[prize_type], NULL, 0);
    item_angle = BoardPlayerRotYGet(player);
    BoardModelRotYSet(*prize_model, item_angle);
    BoardModelScaleSet(*prize_model, 1.0f, 1.0f, 1.0f);
    BoardSpacePosGet(0, GWPlayer[player].space_curr, &item_position);
    BoardPlayerPosGet(player, &player_position);
    player_position.y = item_position.y;
    item_position = player_position;
    player_position.y += 700.0f;
    item_position.y += 280.0f;
    vertical_step = (item_position.y - player_position.y) / 60.0f;
    sound = HuAudFXPlay(0x35F);
    for (i = 0; i < 60; i++) {
        player_position.y += vertical_step;
        BoardModelPosSetV(*prize_model, &player_position);
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    BoardPlayerPosGet(player, &item_position);
    if (BoardPlayerSizeGet(player) == 0) {
        item_position.y += 100.0f;
    } else {
        item_position.y += 30.000002f;
    }
    vertical_step = (item_position.y - player_position.y) / 30.0f;
    shrink_phase = 0.0f;
    HuAudFXPlay(0x30D);
    for (i = 0; i < 30; i++) {
        player_position.y += vertical_step;
        item_angle = BoardDAngleCalc(15.0f + item_angle);
        item_scale = cosd(shrink_phase);
        if (item_scale <= 0.0f) {
            item_scale = 0.01f;
        }
        shrink_phase += 3.3333333f;
        BoardModelPosSetV(*prize_model, &player_position);
        BoardModelRotYSet(*prize_model, item_angle);
        BoardModelScaleSet(*prize_model, item_scale, item_scale, item_scale);
        HuPrcVSleep();
    }
    HuAudFXStop(sound);
    BoardModelKill(*prize_model);
    *prize_model = -1;
    BoardPlayerItemAdd(GWSystem.player_curr, item_ids[prize_type]);
    omVibrate(GWSystem.player_curr, 12, 6, 6);
}

/**
 * @brief Hides all temporary lottery coin models.
 */
static void KillCoin(void) {
    s32 i;

    for (i = 0; i < 10; i++) {
        BoardModelKill(coinMdl[i]);
        coinMdl[i] = -1;
    }
}

/**
 * @brief Dispatches the current lottery result to its coin or item award sequence.
 */
static void ExecPrize(void) {
    Vec prize_position;
    Vec camera_offset;
    s16 host_motion;
    s16 prize_motion;
    s32 stream;
    s32 message;
    s32 sound;
    s32 player;
    s32 player_motion;
    s32 i;
    s32 frame;

    message = -1;
    host_motion = 0;
    sound = -1;
    prize_position.x = 0.0f;
    prize_position.y = 200.0f;
    prize_position.z = 0.0f;
    BoardCameraMotionStartEx(lotteryMdl[0], NULL, &prize_position, 1300.0f, -1.0f, 21);
    player = GWSystem.player_curr;
    BoardStatusShowSetForce(player);
    BoardStatusShowSet(player, 1);
    for (frame = i = 0; i < ARRAY_COUNT(GWPlayer->items); i++) {
        if (GWPlayer[GWSystem.player_curr].items[i] != -1) {
            frame++;
        }
    }
    player_motion = currPrize;
    if (player_motion == 0) {
        BoardModelVisibilitySet(lotteryMdl[2], 1);
        BoardModelMotionStart(lotteryMdl[2], 0, 0x40000001);
        BoardModelMotionSpeedSet(lotteryMdl[2], 2.0f);
    }
    stream = 0;
    switch (player_motion) {
        case 0:
            message = lotteryMessBase + 2;
            host_motion = 7;
            stream = 100;
            break;
        case 1:
            message = lotteryMessBase + 4;
            host_motion = 7;
            stream = 80;
            break;
        case 2:
            message = lotteryMessBase + 5;
            if (frame < 3) {
                host_motion = 7;
                stream = 80;
            } else {
                host_motion = 8;
            }
            break;
        case 3:
            message = lotteryMessBase + 7;
            host_motion = 8;
            break;
    }
    if (player_motion != 3) {
        HuAudFXPlay(0x332);
    }
    if (host_motion == 7) {
        BoardModelMotionShiftSet(BoardLotteryHostGet(), lotteryMot[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    }
    if (stream != 0) {
        BoardPlayerPosGet(player, &camera_offset);
        camera_offset.y = 700.0f;
        BoardConfettiCreate(&camera_offset, stream, 150.0f);
    }
    if (player_motion == 0 || player_motion == 1) {
        BoardAudSeqPause(1, 1, 0x1F4);
        HuPrcSleep(0x14);
        if (player_motion == 0) {
            sound = HuAudSStreamPlay(0xA);
        } else {
            sound = HuAudSStreamPlay(9);
        }
    }
    prize_motion = (s32) BoardPlayerRotYGet(player) + 180;
    BoardPlayerMotBlendSet(player, prize_motion, 0xF);
    while (!BoardPlayerMotBlendCheck(player)) {
        HuPrcVSleep();
    }
    if (player_motion == 0 || player_motion == 1) {
        while (HuAudSStreamStatGet(sound) != 0) {
            HuPrcVSleep();
        }
        sound = -1;
        BoardAudSeqPause(1, 0, 1000);
    }
    BoardWinCreate(2, message, BoardWinPortraitGet());
    BoardWinWait();
    if (player_motion == 2) {
        if (frame == 3) {
            BoardWinCreate(2, lotteryMessBase + 6, BoardWinPortraitGet());
            BoardWinWait();
        } else {
            BoardAudSeqPause(1, 1, 1000);
            HuPrcSleep(0x30);
            sound = HuAudSStreamPlay(2);
        }
    }
    if (host_motion == 8) {
        HuAudFXPlay(loseSoundTbl[GWPlayer[player].character]);
        BoardPlayerVoiceEnableSet(player, host_motion, 0);
    }
    BoardPlayerMotionShiftSet(player, host_motion, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    switch (player_motion) {
        case 0:
        case 1:
            ExecCoinPrize();
            break;
        case 2:
            if (frame < 3) {
                ExecItemPrize();
            } else {
                player_motion = 3;
            }
            break;
    }
    if (sound != -1) {
        while (HuAudSStreamStatGet(sound) != 0) {
            HuPrcVSleep();
        }
        sound = -1;
        BoardAudSeqPause(1, 0, 1000);
    }
    while (!BoardPlayerMotionEndCheck(player)) {
        HuPrcVSleep();
    }
    if (player_motion != 3) {
        message = lotteryMessBase + 3;
    } else {
        message = lotteryMessBase + 8;
    }
    BoardModelMotionShiftSet(BoardLotteryHostGet(), lotteryMot[2], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    BoardWinCreate(2, message, BoardWinPortraitGet());
    BoardWinWait();
    BoardWinKill();
    if (stream != 0) {
        BoardConfettiStop();
    }
    if (player_motion == 3) {
        ExecLose(player);
    }
}

/**
 * @brief Plays the losing lottery reaction for a player.
 * @param arg0 Player index that performs the losing reaction.
 */
static void ExecLose(s32 arg0) {
    s32 player_motion;

    player_motion = GWPlayer[arg0].character;
    loseMot = BoardPlayerMotionCreate(arg0, loseMotTbl[player_motion]);
    BoardPlayerMotionShiftSet(arg0, loseMot, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(10);
    while (!BoardPlayerMotionEndCheck(arg0)) {
        HuPrcVSleep();
    }
}

/**
 * @brief Creates the lottery result message window for a prize tier.
 * @param arg0 Prize tier used to select the result message.
 */
static void CreateLotteryWin(s32 arg0) {
    float window_size[2];
    float width;
    float height;
    s32 message;

    width = -10000.0f;
    if (arg0 == 0) {
        message = MAKE_MESSID(6, 80);
        height = 352.0f;
    } else if (arg0 == 1) {
        height = 352.0f;
        message = MAKE_MESSID(6, 81);
    } else {
        height = 364.0f;
        message = MAKE_MESSID(32, 22);
    }
    HuWinMesMaxSizeGet(1, window_size, message);
    lotteryInstWin = HuWinCreate(width, height, window_size[0], window_size[1], 0);
    HuWinBGTPLvlSet(lotteryInstWin, 0.0f);
    HuWinMesSpeedSet(lotteryInstWin, 0);
    HuWinMesSet(lotteryInstWin, message);
}

/**
 * @brief Closes the lottery result message window if it exists.
 */
static void KillLotteryWin(void) {
    if (lotteryInstWin != -1) {
        HuWinKill(lotteryInstWin);
        lotteryInstWin = -1;
    }
}

/**
 * @brief Runs the scripted lottery demonstration used by board tutorial mode.
 */
void BoardLotteryTutorialExec(void) {
    Vec player_position;
    Vec lottery_position;
    Vec entrance_position;
    Vec camera_rotation;
    Vec camera_offset;
    s16 player_motion;
    s16 host_motion;
    s16 entrance_space;
    s32 player;

    player = BoardDataDirReadAsync(DATADIR_BKUJIYA);
    entrance_space = BoardSpaceFlagSearch(0, 0x10000000);
    if (player != -1) {
        BoardDataAsyncWait(player);
    }
    BoardModelMotionStart(BoardLotteryHostGet(), 1, 0x40000001);
    CreateModel();
    player_motion = BoardSpaceLinkFlagSearch(0, entrance_space, 0x02000000);
    BoardSpacePosGet(0, entrance_space, &player_position);
    BoardSpacePosGet(0, player_motion, &lottery_position);
    VECSubtract(&lottery_position, &player_position, &entrance_position);
    VECNormalize(&entrance_position, &entrance_position);
    host_motion = atan2d(-entrance_position.x, -entrance_position.z);
    camera_rotation.x = -3.0f;
    camera_rotation.y = host_motion;
    camera_rotation.z = 0.0f;
    camera_offset.x = 0.0f;
    camera_offset.y = 200.0f;
    camera_offset.z = 0.0f;
    BoardCameraMotionStartEx(lotteryMdl[0], &camera_rotation, &camera_offset, 1300.0f, -1.0f, 21);
    HuAudFXPlay(0x331);
    HuAudFXPlay(0x344);
    BoardModelMotionStart(lotteryMdl[0], 0, 0);
    while (!BoardModelMotionEndCheck(lotteryMdl[0])) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(BoardLotteryHostGet(), lotteryMot[3], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(10);
    while (!BoardModelMotionEndCheck(BoardShopHostGet())) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(BoardLotteryHostGet(), lotteryMot[0], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    BoardTutorialHookExec(0x1C, 0);
    BoardModelMotionShiftSet(BoardLotteryHostGet(), lotteryMot[2], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    HuPrcSleep(30);
    BoardModelMotionStart(lotteryMdl[0], 0, 0x40000004);
    while (!BoardModelMotionEndCheck(lotteryMdl[0])) {
        HuPrcVSleep();
    }
    BoardModelMotionSpeedSet(lotteryMdl[0], 0.0f);
    BoardModelAttrReset(lotteryMdl[0], 0x40000004);
    HuAudFXPlay(0x32F);
    KillModel();
}
