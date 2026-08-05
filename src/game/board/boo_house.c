#include "game/audio.h"
#include "game/board/boo_house.h"
#include "game/board/audio.h"
#include "game/board/boo.h"
#include "game/board/com.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/star.h"
#include "game/board/ui.h"
#include "game/board/view.h"
#include "game/board/window.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/msm.h"

#include "ext_math.h"

#include "game/board/tutorial.h"
#include "game/hsfex.h"

// bss
static s16 houseMdl[6];

// data
static s32 hostMesTbl[6][6] = {
    {
        DATA_MAKE_NUM(DATADIR_BOARD, 0x0C),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x0D),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x0E),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x0F),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x10),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x11),
    },
    {
        DATA_MAKE_NUM(DATADIR_BOARD, 0x12),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x13),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x14),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x15),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x16),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x17),
    },
    {
        DATA_MAKE_NUM(DATADIR_BOARD, 0x18),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x19),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x1A),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x1B),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x1C),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x1D),
    },
    {
        DATA_MAKE_NUM(DATADIR_BOARD, 0x1E),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x1F),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x20),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x21),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x22),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x23),
    },
    {
        DATA_MAKE_NUM(DATADIR_BOARD, 0x24),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x25),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x26),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x27),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x28),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x29),
    },
    {
        DATA_MAKE_NUM(DATADIR_BOARD, 0x2A),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x2B),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x2C),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x2D),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x2E),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x2F),
    },
};
static BoardModelParam houseMdlTbl[6] = {
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 0x00), { 0, 1, 0, 0, 0 }, 0xFFFF },
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 0x01), { 1, 1, 0, 0, 0 }, 0xFFFF },
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 0x03), { 1, 1, 0, 0, 0 }, 0xFFFF },
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 0x04), { 1, 1, 0, 0, 0 }, 0xFFFF },
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 0x04), { 1, 1, 0, 0, 0 }, 0xFFFF },
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 0x07), { 0, 1, 0, 0, 0 }, 0xFFFF },
};
static s32 booMotTbl[3] = {
    DATA_MAKE_NUM(DATADIR_BGUEST, 0x13),
    DATA_MAKE_NUM(DATADIR_BGUEST, 0x16),
    DATA_NUM_LISTEND,
};
static s32 hostMotTbl[9][3] = {
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x01), DATA_MAKE_NUM(DATADIR_BGUEST, 0x03), DATA_MAKE_NUM(DATADIR_BGUEST, 0x04) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x09), DATA_MAKE_NUM(DATADIR_BGUEST, 0x0B), DATA_MAKE_NUM(DATADIR_BGUEST, 0x0C) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x0E), DATA_MAKE_NUM(DATADIR_BGUEST, 0x10), DATA_MAKE_NUM(DATADIR_BGUEST, 0x11) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x18), DATA_MAKE_NUM(DATADIR_BGUEST, 0x1A), DATA_MAKE_NUM(DATADIR_BGUEST, 0x1B) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x1D), DATA_MAKE_NUM(DATADIR_BGUEST, 0x1F), DATA_MAKE_NUM(DATADIR_BGUEST, 0x20) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x22), DATA_MAKE_NUM(DATADIR_BGUEST, 0x24), DATA_MAKE_NUM(DATADIR_BGUEST, 0x25) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x18), DATA_MAKE_NUM(DATADIR_BGUEST, 0x1A), DATA_MAKE_NUM(DATADIR_BGUEST, 0x1B) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x01), DATA_MAKE_NUM(DATADIR_BGUEST, 0x03), DATA_MAKE_NUM(DATADIR_BGUEST, 0x04) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x01), DATA_MAKE_NUM(DATADIR_BGUEST, 0x03), DATA_MAKE_NUM(DATADIR_BGUEST, 0x04) },
};

// sbss
static s8 currPlayer;
static s8 stealTarget;
static s8 comStarStealTarget;
static s32 comCoinStealTarget;
static char stealAmountStr[8];
static HUPROCESS *houseProc;

// sdata
static s16 houseShowMot = 0xFFFF;
static s16 houseHideMot = 0xFFFF;
static s16 ballMdl = 0xFFFF;
static s16 booMdl = 0xFFFF;
static s16 smokeMdl = 0xFFFF;
static s16 hostMdl = 0xFFFF;
static s16 hostMot[3] = { 0xFFFF, 0xFFFF, 0xFFFF };

// determined functions
static void ExecBooHouse(void);
static void DestroyBooHouse(void);
static void PickComStealType(s32);
static void OpenBooHouse(void);
static void CloseBooHouse(void);
static void CreateBooHouseModel(void);
static void KillBooHouseModel(void);
static void ExecNullSteal(void);
static void ExecStarSteal(void);
static void ExecCoinSteal(void);
static void ApplySteal(void);
static void ApplyCoinSteal(s16 stolen_coins, s16 punch_reduction);
static void ApplyStarSteal(void);
static void ApplyBooLight(void);
static void SetBooBallPos(void);

// FUNCTIONS //
/**
 * @brief Register the board-specific Boo House host model.
 * @param host_model Model handle supplied by the active board overlay.
 */
void BoardBooHouseHostSet(s16 host_model)
{
    hostMdl = host_model;
}

/**
 * @brief Get the registered Boo House host model.
 * @return The current host model handle, or -1 when no host is registered.
 */
s16 BoardBooHouseHostGet(void)
{
    return hostMdl;
}

/**
 * @brief Create the stationary Boo House shell at its flagged board link.
 *
 * @details Extra boards omit the house; main boards also hide the separately supplied host model.
 */
void BoardBooHouseCreate(void)
{
    Vec house_pos;
    Vec house_rot;

    if ((GWBoardGet() == BOARD_ID_EXTRA1) || (GWBoardGet() == BOARD_ID_EXTRA2)) {
        return;
    }
    BoardSpaceLinkTransformGet(0x08000000, &house_pos, &house_rot, 0);
    houseMdl[0] = BoardModelCreateParam(&houseMdlTbl[0], &house_pos, &house_rot);
    if (hostMdl != -1) {
        BoardModelVisibilitySet(hostMdl, 0);
    }
    HuDataDirClose(DATADIR_BYOKODORI);
}

/**
 * @brief Destroy the stationary Boo House shell model.
 */
void BoardBooHouseKill(void)
{
    if (houseMdl[0] != -1) {
        BoardModelKill(houseMdl[0]);
        houseMdl[0] = -1;
    }
}

/**
 * @brief Resolve a player's visit to the Boo House.
 *
 * @details Mini players can enter; Mega players skip the event. The routine hides the roll display while its child process runs.
 * @param visitor_player Index of the visiting player.
 */
void BoardBooHouseExec(s32 visitor_player)
{
    currPlayer = visitor_player;
    if (BoardPlayerSizeGet(currPlayer) != 2) {
        BoardRollDispSet(0);
        houseProc = HuPrcChildCreate(&ExecBooHouse, 0x2003, 0x3800, 0, boardMainProc);
        HuPrcDestructorSet2(houseProc, &DestroyBooHouse);
        while (houseProc != 0U) {
            HuPrcVSleep();
        }
        BoardRollDispSet(1);
    }
}

/**
 * @brief Drive payment, theft selection, Boo animation, and departure from the house.
 *
 * @details Rejects players with fewer than five coins, charges five or fifty coins according to theft type, and restores the player, camera, status UI, and board audio before exiting.
 */
static void ExecBooHouse(void)
{
    Vec current_space_pos;
    Vec linked_space_pos;
    Vec travel_direction;
    Vec camera_rot;
    Vec camera_offset;
    float facing_angle;
    s16 linked_space;
    s32 player_index;
    s16 original_space;
    s32 data_read_id;
    s32 opponent_coins;
    s32 opponent_stars;

    _SetFlag(0x10018);
    data_read_id = BoardDataDirReadAsync(DATADIR_BYOKODORI);
    original_space = GWPlayer[currPlayer].space_curr;
    BoardPlayerIdleSet(currPlayer);
    if (BoardPlayerCoinsGet(currPlayer) < 5) {
        BoardWinCreate(0, MAKE_MESSID(7, 1), -1);
        BoardWinWait();
        BoardWinKill();
        BoardDataAsyncWait(data_read_id);
        HuPrcEnd();
    }
    BoardWinCreateChoice(0, MAKE_MESSID(7, 0), -1, 0);
    if (GWPlayer[currPlayer].com != 0) {
        if (BoardBooComUseCheck(currPlayer) != 0) {
            BoardComKeySetLeft();
        }
        else {
            BoardComKeySetRight();
        }
    }
    BoardWinWait();
    if (BoardWinChoiceGet() != 0) {
        BoardDataAsyncWait(data_read_id);
        HuPrcEnd();
    }

    for (opponent_stars = player_index = 0; player_index < 4; player_index++) {
        if (currPlayer != player_index) {
            opponent_stars += GWStarsGet(player_index);
        }
    }

    for (opponent_coins = player_index = 0; player_index < 4; player_index++) {
        if (currPlayer != player_index) {
            opponent_coins += BoardPlayerCoinsGet(player_index);
        }
    }
    BoardWinKill();
    BoardRollDispSet(0);
    BoardAudSeqPause(0, 1, 0x3E8);
    BoardDataAsyncWait(data_read_id);
    linked_space = BoardSpaceLinkFlagSearch(0, original_space, 0x02000000);
    BoardSpacePosGet(0, original_space, &current_space_pos);
    BoardSpacePosGet(0, linked_space, &linked_space_pos);
    VECSubtract(&linked_space_pos, &current_space_pos, &travel_direction);
    VECNormalize(&travel_direction, &travel_direction);
    facing_angle = atan2d(-travel_direction.x, -travel_direction.z);
    camera_rot.x = -15.0f;
    camera_rot.y = facing_angle;
    camera_rot.z = 0.0f;
    camera_offset.x = 0.0f;
    camera_offset.y = 150.0f;
    camera_offset.z = 0.0f;
    BoardCameraMotionStartEx(houseMdl[0], &camera_rot, &camera_offset, 1370.0f, -1.0f, 0x1B);
    CreateBooHouseModel();
    OpenBooHouse();
    BoardPlayerRotSet(currPlayer, 0.0f, facing_angle, 0.0f);
    BoardPlayerMoveBetween(currPlayer, original_space, linked_space);
    BoardPlayerIdleSet(currPlayer);
    BoardMusStart(1, 1, 0x7F, 0);
    BoardModelMotionShiftSet(BoardBooHouseHostGet(), hostMot[0], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    BoardWinCreate(2, hostMesTbl[GWBoardGet()][0], BoardWinPortraitGet());
    BoardWinWait();

repeatWindowView:
    HuAudFXPlay(0x4C);
    BoardWinCreateChoice(1, hostMesTbl[GWBoardGet()][1], BoardWinPortraitGet(), 0);
    if (opponent_coins == 0) {
        BoardWinChoiceDisable(0);
    }
    if ((BoardPlayerCoinsGet(currPlayer) < 0x32) || (opponent_stars == 0)) {
        BoardWinChoiceDisable(1);
    }
    BoardWinAttrSet(0x10);
    if (GWPlayer[currPlayer].com != 0) {
        PickComStealType(currPlayer);
    }
    BoardWinWait();
    stealTarget = BoardWinChoiceGet();
    BoardWinKill();
    while (BoardStatusStopCheck(currPlayer) == 0) {
        HuPrcVSleep();
    }
    switch (stealTarget) {
        case 0:
            ExecCoinSteal();
            break;
        case 1:
            ExecStarSteal();
            break;
        case 2:
            ExecNullSteal();
            break;
        case 3:
            BoardViewMapExec(currPlayer);
            goto repeatWindowView;
    }
    if (stealTarget != 2) {
        ApplySteal();
        BoardStatusShowSet(currPlayer, 0);
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(BoardBooHouseHostGet(), hostMot[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    CloseBooHouse();
    BoardCameraViewSet(1);
    BoardCameraRotSet(0.0f, 0.0f);
    BoardCameraTargetPlayerSet(currPlayer);
    BoardAudSeqPause(0, 0, 0x3E8);
    BoardPlayerRotSet(currPlayer, 0.0f, facing_angle, 0.0f);
    BoardPlayerMoveBetween(currPlayer, linked_space, original_space);
    KillBooHouseModel();
    BoardStatusShowSetAll(1);
    BoardPlayerIdleSet(currPlayer);
    HuPrcEnd();
}

/**
 * @brief Release motions, temporary models, windows, data, and flags owned by a house visit.
 *
 * @details Also clears houseProc so BoardBooHouseExec can resume.
 */
static void DestroyBooHouse(void)
{
    s32 motion_index;

    for (motion_index = 0; motion_index < 3; motion_index++) {
        if (hostMot[motion_index] != -1) {
            BoardModelMotionKill(BoardBooHouseHostGet(), hostMot[motion_index]);
            hostMot[motion_index] = -1;
        }
    }
    if (houseShowMot != -1) {
        BoardModelMotionKill(houseMdl[0], houseShowMot);
        houseShowMot = -1;
    }
    if (houseHideMot != -1) {
        BoardModelMotionKill(houseMdl[1], houseHideMot);
        houseHideMot = -1;
    }
    BoardWinKill();
    KillBooHouseModel();
    HuDataDirClose(DATADIR_BYOKODORI);
    _ClearFlag(0x10018);
    houseProc = 0;
}

/**
 * @brief Choose coin or star theft input for a computer visitor.
 *
 * @details Prefers a star when the player can afford fifty coins and an eligible opponent owns one; otherwise selects coins.
 * @param player Index of the computer player visiting the house.
 */
static void PickComStealType(s32 player)
{
    s32 player_index;
    s32 opponent_coins;
    s32 opponent_stars;

    opponent_coins = opponent_stars = 0;
    for (player_index = 0; player_index < 4; player_index++) {
        if (player_index != player) {
            opponent_coins += BoardPlayerCoinsGet(player_index);
            opponent_stars += GWStarsGet(player_index);
        }
    }
    if (BoardPlayerCoinsGet(player) >= 0x32) {
        for (player_index = 0; player_index < 4; player_index++) {
            if ((player_index != player) && (GWStarsGet(player_index) != 0) && (((s32)GWSystem.team == 0) || (player_index != BoardPlayerSameTeamFind(player)))) {
                if (opponent_coins != 0) {
                    BoardComKeySetDown();
                }
                else {
                    BoardComKeySetUp();
                }
                return;
            }
        }
    }
    BoardComKeySetUp();
}

/**
 * @brief Animate the house opening and reveal its host, Boo ball, Boo, and smoke models.
 *
 * @details Creates host motions, hooks temporary models to the house, and detaches the Boo ball after the opening motion.
 */
static void OpenBooHouse(void)
{
    s16 host_model;
    s32 open_boo_house_unused_temp_r5;
    s32 motion_index;

    ballMdl = BoardBooCreate(currPlayer, 0);
    host_model = BoardBooHouseHostGet();
    BoardModelMotionStart(host_model, 1, 0x40000001);

    for (motion_index = 0; motion_index < 3; motion_index++) {
        hostMot[motion_index] = BoardModelMotionCreate(host_model, hostMotTbl[GWBoardGet()][motion_index]);
    }
    BoardModelMotionStart(host_model, hostMot[0], 0x40000001);
    BoardModelVisibilitySet(host_model, 1);
    HuAudFXPlay(0x337);
    HuAudFXPlay(0x344);
    BoardModelMotionStart(houseMdl[0], 0, 0);
    BoardModelMotionSpeedSet(houseMdl[0], 1.0f);
    BoardModelHookSet(houseMdl[0], "yoko01", houseMdl[1]);
    BoardModelHookSet(houseMdl[0], "yoko02", ballMdl);
    BoardModelHookSet(houseMdl[0], "yoko03", houseMdl[2]);
    BoardModelHookSet(houseMdl[0], "yoko04", houseMdl[3]);
    BoardModelHookSet(houseMdl[0], "yoko05", houseMdl[4]);
    BoardModelHookSet(houseMdl[0], "yoko9", houseMdl[5]);
    BoardModelHookSet(houseMdl[0], "yoko06", host_model);
    if (GWBoardGet() == BOARD_ID_MAIN6) {
        BoardModelPosSet(host_model, 0.0f, 0.0f, -25.0f);
    }
    else {
        BoardModelPosSet(host_model, 0.0f, 0.0f, 0.0f);
    }
    BoardModelMotionStart(houseMdl[3], 0, 0x40000001);
    BoardModelMotionStart(houseMdl[4], 0, 0x40000001);
    booMdl = BoardModelCreate(0x20012, booMotTbl, 0);
    BoardModelMotionStart(booMdl, 1, 0x40000001);
    BoardModelVisibilitySet(booMdl, 1);
    BoardModelHookSet(houseMdl[0], "yoko8", booMdl);
    smokeMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BYOKODORI, 0x08), NULL, 0);
    BoardModelVisibilitySet(smokeMdl, 0);
    BoardModelLayerSet(smokeMdl, 2);
    BoardModelMotionStart(houseMdl[1], 0, 0x40000001);

    while (BoardModelMotionTimeGet(houseMdl[0]) < 90.0f) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(houseMdl[0], houseShowMot, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    BoardModelMotionShiftSet(houseMdl[1], houseHideMot, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    BoardModelHookObjReset(houseMdl[0], "yoko02");
    SetBooBallPos();
    BoardModelMotionShiftSet(BoardBooHouseHostGet(), hostMot[2], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
}

/**
 * @brief Animate the house closing and destroy the Boo ball controller.
 *
 * @details Rehooks the ball for the closing motion, waits for the shell animation, then resets model hooks.
 */
static void CloseBooHouse(void)
{
    BoardModelPosSet(ballMdl, 0.0f, 0.0f, 0.0f);
    BoardModelHookSet(houseMdl[0], "yoko02", ballMdl);
    BoardModelMotionShiftSet(houseMdl[1], 0, 90.0f, 10.0f, HU3D_MOTATTR_NONE);
    BoardModelMotionShiftSet(houseMdl[0], 0, 90.0f, 10.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(0xA);

    while (BoardModelMotionShiftIDGet(houseMdl[0]) != -1) {
        HuPrcVSleep();
    }
    while (BoardModelMotionTimeGet(houseMdl[0]) < 165.0f) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x336);
    BoardAudSeqFadeOut(1, 0x3E8);

    while (BoardModelMotionEndCheck(houseMdl[0]) == 0) {
        HuPrcVSleep();
    }
    BoardModelHookReset(houseMdl[0]);
    BoardBooKill();
    ballMdl = -1;
}

/**
 * @brief Create the five temporary house components and two shell motions.
 *
 * @details The persistent shell in houseMdl[0] is reused.
 */
static void CreateBooHouseModel(void)
{
    s32 model_index;

    for (model_index = 0; model_index < 6; model_index++) {
        if (model_index != 0) {
            houseMdl[model_index] = BoardModelCreateParam(&houseMdlTbl[model_index], 0, 0);
        }
    }
    houseShowMot = BoardModelMotionCreate(houseMdl[0], DATA_MAKE_NUM(DATADIR_BYOKODORI, 0x09));
    houseHideMot = BoardModelMotionCreate(houseMdl[1], DATA_MAKE_NUM(DATADIR_BYOKODORI, 0x0A));
}

/**
 * @brief Destroy temporary house, Boo, and smoke models and hide the host.
 *
 * @details Leaves the persistent house shell in houseMdl[0] intact.
 */
static void KillBooHouseModel(void)
{
    s32 kill_boo_house_model_unused_temp_r4;
    s32 model_index;

    for (model_index = 1; model_index < 6; model_index++) {
        if (houseMdl[model_index] != -1) {
            BoardModelKill(houseMdl[model_index]);
            houseMdl[model_index] = -1;
        }
    }
    if (booMdl != -1) {
        BoardModelKill(booMdl);
        booMdl = -1;
    }
    if (smokeMdl != -1) {
        BoardModelKill(smokeMdl);
        smokeMdl = -1;
    }
    BoardModelVisibilitySet(BoardBooHouseHostGet(), 0);
}

/**
 * @brief Display the host message for leaving without a theft.
 */
static void ExecNullSteal(void)
{
    HuAudFXPlay(0x4C);
    BoardWinCreate(2, hostMesTbl[GWBoardGet()][5], BoardWinPortraitGet());
    BoardWinWait();
    BoardWinKill();
}

/**
 * @brief Charge fifty coins, choose a star target, and play Boo's departure and return.
 *
 * @details The return animation changes when a repellent light prevents the theft.
 */
static void ExecStarSteal(void)
{
    Vec smoke_pos;
    Vec camera_offset;
    s32 steal_result;
    s32 coin_index;
    s32 alpha;

    for (coin_index = 0; coin_index < 0x32; coin_index++) {
        BoardPlayerCoinsAdd(currPlayer, -1);
        HuAudFXPlay(0xE);
        HuPrcSleep(1);
    }
    HuAudFXPlay(0xF);
    BoardBooStealTypeSet(1);
    BoardWinCreate(2, hostMesTbl[GWBoardGet()][3], BoardWinPortraitGet());
    BoardWinWait();
    BoardWinKill();
    BoardStatusShowSetAll(0);

    while (BoardPickerDoneCheck() == 0) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x4C);
    Hu3DModelObjPosGet(BoardModelIDGet(houseMdl[0]), "yoko8", &smoke_pos);
    smoke_pos.y += 20.0f;
    BoardModelMotionStart(smokeMdl, 0, 0);
    BoardModelVisibilitySet(smokeMdl, 1);
    BoardModelPosSetV(smokeMdl, &smoke_pos);
    HuPrcSleep(5);

    for (alpha = 0xFF; alpha > 0; alpha -= 0x33) {
        if (alpha < 0) {
            alpha = 0;
        }
        BoardModelAlphaSet(booMdl, alpha);
        HuPrcVSleep();
    }
    BoardModelVisibilitySet(booMdl, 0);

    while (BoardModelMotionEndCheck(smokeMdl) == 0) {
        HuPrcVSleep();
    }
    BoardModelVisibilitySet(smokeMdl, 0);
    BoardCameraMotionStartEx(ballMdl, NULL, NULL, 325.0f, -1.0f, 0x15);
    steal_result = BoardBooStealMain();
    camera_offset.x = 0.0f;
    camera_offset.y = 150.0f;
    camera_offset.z = 0.0f;
    BoardCameraMotionStartEx(houseMdl[0], NULL, &camera_offset, 1370.0f, -1.0f, 0x15);
    BoardCameraMotionWait();
    if (steal_result == 0) {
        BoardModelMotionStart(booMdl, 2, 0x40000001);
    }
    BoardModelMotionStart(smokeMdl, 0, 0);
    BoardModelVisibilitySet(smokeMdl, 1);
    HuPrcSleep(0xB);
    BoardModelVisibilitySet(booMdl, 1);

    for (alpha = 0; alpha < 0xFF; alpha += 0x33) {
        if (alpha > 0xFF) {
            alpha = 0xFF;
        }
        BoardModelAlphaSet(booMdl, alpha);
        HuPrcVSleep();
    }
    BoardModelAlphaSet(booMdl, -1);

    while (BoardModelMotionEndCheck(smokeMdl) == 0) {
        HuPrcVSleep();
    }
    BoardModelVisibilitySet(smokeMdl, 0);
}

/**
 * @brief Charge five coins, choose a coin target, and play Boo's departure and return.
 *
 * @details The return animation changes when a repellent light prevents the theft.
 */
static void ExecCoinSteal(void)
{
    Vec smoke_pos;
    Vec camera_offset;
    s32 steal_result;
    s32 coin_index;
    s32 alpha;

    for (coin_index = 0; coin_index < 5; coin_index++) {
        BoardPlayerCoinsAdd(currPlayer, -1);
        HuAudFXPlay(0xE);
        HuPrcSleep(6);
    }
    HuAudFXPlay(0xF);
    BoardBooStealTypeSet(0);
    BoardWinCreate(2, hostMesTbl[GWBoardGet()][3], BoardWinPortraitGet());
    BoardWinWait();
    BoardWinKill();
    BoardStatusShowSetAll(0);

    while (BoardPickerDoneCheck() == 0) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x4C);
    Hu3DModelObjPosGet(BoardModelIDGet(houseMdl[0]), "yoko8", &smoke_pos);
    smoke_pos.y += 20.0f;
    BoardModelMotionStart(smokeMdl, 0, 0);
    BoardModelVisibilitySet(smokeMdl, 1);
    BoardModelPosSetV(smokeMdl, &smoke_pos);
    HuPrcSleep(5);

    for (alpha = 0xFF; alpha > 0; alpha -= 0x33) {
        if (alpha < 0) {
            alpha = 0;
        }
        BoardModelAlphaSet(booMdl, alpha);
        HuPrcVSleep();
    }
    BoardModelVisibilitySet(booMdl, 0);

    while (BoardModelMotionEndCheck(smokeMdl) == 0) {
        HuPrcVSleep();
    }
    BoardModelVisibilitySet(smokeMdl, 0);
    BoardCameraMotionStartEx(ballMdl, NULL, NULL, 325.0f, -1.0f, 0x15);
    steal_result = BoardBooStealMain();
    camera_offset.x = 0.0f;
    camera_offset.y = 150.0f;
    camera_offset.z = 0.0f;
    BoardCameraMotionStartEx(houseMdl[0], NULL, &camera_offset, 1370.0f, -1.0f, 0x15);
    BoardCameraMotionWait();
    if (steal_result == 0) {
        BoardModelMotionStart(booMdl, 2, 0x40000001);
    }
    BoardModelMotionStart(smokeMdl, 0, 0);
    BoardModelVisibilitySet(smokeMdl, 1);
    HuPrcSleep(0xB);
    BoardModelVisibilitySet(booMdl, 1);

    for (alpha = 0; alpha < 0xFF; alpha += 0x33) {
        if (alpha > 0xFF) {
            alpha = 0xFF;
        }
        BoardModelAlphaSet(booMdl, alpha);
        HuPrcVSleep();
    }
    BoardModelAlphaSet(booMdl, -1);

    while (BoardModelMotionEndCheck(smokeMdl) == 0) {
        HuPrcVSleep();
    }
    BoardModelVisibilitySet(smokeMdl, 0);
}

/**
 * @brief Apply the completed Boo encounter to the visiting player.
 *
 * @details Turns the visitor toward the house, queries the theft result, then awards coins, a star, or the repelled-light response.
 */
void ApplySteal(void)
{
    s16 stolen_value;
    s16 punch_reduction;
    s32 apply_steal_unused_temp_r5;
    s32 apply_steal_unused_var_r30;
    s32 steal_succeeded;
    float facing_angle;

    BoardModelHookSet(houseMdl[0], "yoko8", booMdl);
    BoardModelVisibilitySet(booMdl, 1);
    BoardModelPosSet(booMdl, 0.0f, 0.0f, 0.0f);
    BoardModelRotSet(booMdl, 0.0f, 0.0f, 0.0f);
    BoardModelScaleSet(booMdl, 1.0f, 1.0f, 1.0f);
    BoardModelAlphaSet(booMdl, 0xFF);
    facing_angle = BoardDAngleCalc(180.0f + BoardPlayerRotYGet(currPlayer));
    BoardPlayerMotBlendSet(currPlayer, facing_angle, 0xF);

    while (BoardPlayerMotBlendCheck(currPlayer) == 0) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(currPlayer);
    steal_succeeded = BoardBooStealValueGet(&stolen_value, &punch_reduction);
    if (steal_succeeded == 0) {
        ApplyBooLight();
    }
    else if (stealTarget == 0) {
        ApplyCoinSteal(stolen_value, punch_reduction);
    }
    else {
        ApplyStarSteal();
    }
    BoardWinCreate(2, hostMesTbl[GWBoardGet()][4], BoardWinPortraitGet());
    BoardWinWait();
    BoardWinKill();
}

/**
 * @brief Show the coin-theft result and add the stolen coins to the visitor.
 *
 * @details Coins are added one at a time while the visitor's status panel is forced visible.
 * @param stolen_coins Number of coins successfully stolen.
 * @param punch_reduction Punch-reduction tier used to choose the result message.
 */
void ApplyCoinSteal(s16 stolen_coins, s16 punch_reduction)
{
    u32 message_id;
    s32 coin_index;

    if ((s16)punch_reduction == 0) {
        message_id = MAKE_MESSID(7, 9);
    }
    else if ((s16)punch_reduction < 3) {
        message_id = MAKE_MESSID(7, 8);
    }
    else if ((s16)punch_reduction < 6) {
        message_id = MAKE_MESSID(7, 7);
    }
    else if ((s16)punch_reduction < 8) {
        message_id = MAKE_MESSID(7, 6);
    }
    else {
        message_id = MAKE_MESSID(7, 5);
    }
    sprintf(stealAmountStr, "%d", stolen_coins);
    HuAudFXPlay(0x4B);
    BoardWinCreate(2, message_id, 3);
    BoardWinInsertMesSet(MAKE_MESSID_PTR(stealAmountStr), 0);
    BoardWinWait();
    BoardWinKill();
    BoardStatusShowSetForce(currPlayer);
    BoardStatusShowSet(currPlayer, 1);
    BoardPlayerMotionShiftSet(currPlayer, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);

    for (coin_index = 0; coin_index < stolen_coins; coin_index++) {
        BoardPlayerCoinsAdd(currPlayer, 1);
        HuAudFXPlay(0xE);
        HuPrcSleep(6);
    }
    HuAudFXPlay(0xF);
    BoardPlayerMotionEndWait(currPlayer);
}

/**
 * @brief Show the star-theft result and award one star to the visitor.
 *
 * @details Temporarily pauses the theft music, plays the character star voice and stream, and restores voice playback.
 */
void ApplyStarSteal(void)
{
    s32 stream_handle;

    HuAudFXPlay(0x4C);
    BoardWinCreate(2, MAKE_MESSID(7, 10), 3);
    BoardWinWait();
    BoardWinKill();
    BoardStatusShowSetForce(currPlayer);
    BoardStatusShowSet(currPlayer, 1);
    BoardAudSeqPause(1, 1, 0x3E8);
    HuPrcSleep(0x17);
    BoardPlayerVoiceEnableSet(currPlayer, 7, 0);
    BoardPlayerMotionShiftSet(currPlayer, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuAudFXPlay(boardStarSndTbl[GWPlayer[currPlayer].character]);
    BoardPlayerStarsAdd(currPlayer, 1);
    HuAudFXPlay(8);
    HuPrcSleep(0xA);
    stream_handle = HuAudSStreamPlay(6);
    BoardPlayerMotionEndWait(currPlayer);

    while (msmStreamGetStatus(stream_handle) != 0) {
        HuPrcVSleep();
    }
    BoardAudSeqPause(1, 0, 0x3E8);
    BoardPlayerMotionEndWait(currPlayer);
    BoardPlayerVoiceEnableSet(currPlayer, 7, 1);
}

/**
 * @brief Show the failed-theft message and play the visitor's reaction.
 *
 * @details Used when the target's Boo-repellent light was consumed.
 */
void ApplyBooLight(void)
{
    BoardWinCreate(2, MAKE_MESSID(7, 11), 3);
    BoardWinWait();
    BoardWinKill();
    BoardPlayerMotionShiftSet(currPlayer, 8, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    while (BoardPlayerMotionEndCheck(currPlayer) == 0) {
        HuPrcVSleep();
    }
}

/**
 * @brief Place the detached Boo ball at the house's yoko02 hook position.
 *
 * @details Reads the hook matrix from the persistent shell and converts its translation to a world position.
 */
void SetBooBallPos(void)
{
    Mtx hook_mtx;
    Vec ball_pos;
    s16 house_model_id;

    house_model_id = BoardModelIDGet(houseMdl[0]);
    Hu3DModelObjMtxGet(house_model_id, "yoko02", hook_mtx);
    Hu3DMtxTransGet(hook_mtx, &ball_pos);
    BoardModelPosSetV(ballMdl, &ball_pos);
}

/**
 * @brief Find whether a computer player has a viable Boo House target.
 *
 * @details Ignores teammates and may avoid opponents carrying a repellent light according to the acting player's difficulty.
 * @param player Index of the computer player considering the house.
 * @return One when an eligible opponent with coins or stars is found; zero otherwise.
 */
s32 BoardBooComUseCheck(s32 player)
{
    s32 player_index;
    u32 light_ignore_chance;
    s32 target_coins;
    s32 target_stars;
    s32 max_target_coins;
    s32 max_target_stars;
    s32 player_coins;

    player_coins = BoardPlayerCoinsGet(player);
    comStarStealTarget = comCoinStealTarget = -1;
    max_target_stars = max_target_coins = -1;
    switch (GWPlayer[player].diff) {
        case 0:
            light_ignore_chance = 0x5F;
            break;
        case 1:
            light_ignore_chance = 0x32;
            break;
        case 2:
            light_ignore_chance = 5;
            break;
        case 3:
            light_ignore_chance = 0;
            break;
    }

    for (player_index = 0; player_index < 4; player_index++) {
        if (player_index != player) {
            if ((s32)GWSystem.team == 0 || player_index != BoardPlayerSameTeamFind(player)) {
                if (BoardPlayerItemFind(player_index, 8) == -1 || BoardRandMod(0x64) < light_ignore_chance) {
                    target_coins = BoardPlayerCoinsGet(player_index);
                    target_stars = GWStarsGet(player_index);
                    if (target_stars != 0 || target_coins != 0) {
                        if (target_stars != 0 || target_coins >= 5 || player_coins <= 10 || BoardRandMod(0x64) <= 0x32) {
                            if (max_target_coins < target_coins) {
                                max_target_coins = target_coins;
                                comCoinStealTarget = player_index;
                            }
                            if (max_target_stars < target_stars) {
                                max_target_stars = target_stars;
                                comStarStealTarget = (s8)player_index;
                            }
                        }
                    }
                }
            }
        }
    }
    if ((max_target_stars <= 0) && (max_target_coins <= 0)) {
        return 0;
    }
    return 1;
}

/**
 * @brief Play the scripted Boo House opening and closing tutorial.
 *
 * @details Loads the house assets, frames the linked house spaces, runs tutorial hook 0x1C, and performs the same model cleanup as a normal visit.
 */
void BoardBooHouseTutorialExec(void)
{
    Vec house_space_pos;
    Vec linked_space_pos;
    Vec travel_direction;
    Vec camera_rot;
    Vec camera_offset;
    float facing_angle;
    s16 linked_space;
    s16 house_space;
    s32 data_read_id;
    s32 board_boo_house_tutorial_exec_unused_temp_r4;
    s32 model_index;
    s32 cleanup_model_index;

    data_read_id = BoardDataDirReadAsync(DATADIR_BYOKODORI);
    house_space = BoardSpaceFlagSearch(0, 0x08000000);
    BoardDataAsyncWait(data_read_id);
    linked_space = BoardSpaceLinkFlagSearch(0, house_space, 0x02000000);
    BoardSpacePosGet(0, house_space, &house_space_pos);
    BoardSpacePosGet(0, linked_space, &linked_space_pos);
    VECSubtract(&linked_space_pos, &house_space_pos, &travel_direction);
    VECNormalize(&travel_direction, &travel_direction);
    facing_angle = atan2d(-travel_direction.x, -travel_direction.z);
    camera_rot.x = -15.0f;
    camera_rot.y = facing_angle;
    camera_rot.z = 0.0f;
    camera_offset.x = 0.0f;
    camera_offset.y = 150.0f;
    camera_offset.z = 0.0f;
    BoardCameraMotionStartEx(houseMdl[0], &camera_rot, &camera_offset, 1370.0f, -1.0f, 0x15);

    for (model_index = 0; model_index < 6; model_index++) {
        if (model_index != 0) {
            houseMdl[model_index] = BoardModelCreateParam(&houseMdlTbl[model_index], NULL, NULL);
        }
    }
    houseShowMot = BoardModelMotionCreate(houseMdl[0], DATA_MAKE_NUM(DATADIR_BYOKODORI, 0x09));
    houseHideMot = BoardModelMotionCreate(houseMdl[1], DATA_MAKE_NUM(DATADIR_BYOKODORI, 0x0A));
    OpenBooHouse();

    while (BoardModelMotionEndCheck(BoardBooHouseHostGet()) == 0) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(BoardBooHouseHostGet(), hostMot[0], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    BoardTutorialHookExec(0x1C, 0);
    BoardModelMotionShiftSet(BoardBooHouseHostGet(), hostMot[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    BoardModelPosSet(ballMdl, 0.0f, 0.0f, 0.0f);
    BoardModelHookSet(houseMdl[0], "yoko02", ballMdl);
    BoardModelMotionShiftSet(houseMdl[1], 0, 90.0f, 10.0f, HU3D_MOTATTR_NONE);
    BoardModelMotionShiftSet(houseMdl[0], 0, 90.0f, 10.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(0xA);
    while (BoardModelMotionShiftIDGet(houseMdl[0]) != -1) {
        HuPrcVSleep();
    }
    while (BoardModelMotionTimeGet(houseMdl[0]) < 165.0f) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x336);
    BoardAudSeqFadeOut(1, 0x3E8);
    while (BoardModelMotionEndCheck(houseMdl[0]) == 0) {
        HuPrcVSleep();
    }
    BoardModelHookReset(houseMdl[0]);
    BoardBooKill();
    ballMdl = -1;

    for (cleanup_model_index = 1; cleanup_model_index < 6; cleanup_model_index++) {
        if (houseMdl[cleanup_model_index] != -1) {
            BoardModelKill(houseMdl[cleanup_model_index]);
            houseMdl[cleanup_model_index] = -1;
        }
    }
    if (booMdl != -1) {
        BoardModelKill(booMdl);
        booMdl = -1;
    }
    if (smokeMdl != -1) {
        BoardModelKill(smokeMdl);
        smokeMdl = -1;
    }
    BoardModelVisibilitySet(BoardBooHouseHostGet(), 0);
}
