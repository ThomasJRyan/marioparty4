#include "game/board/boo.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/flag.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/window.h"

#include "ext_math.h"
#include <string.h>

typedef struct {
    struct {
        u8 kill : 1;
    };
    s8 mode;
    s8 owner_player;
    char padding[1];
    s16 ball_models[2];
} BooEventWork;

typedef struct {
    struct {
        s8 kill : 1;
    };
    s8 fade_step;
    s16 opacity;
    s16 render_model;
    s16 projection;
    ANIMDATA *animation;
} BallWork;

typedef struct {
    struct {
        s8 kill : 1;
    };
    char padding[1];
    u16 camera;
} BallCameraWork;

typedef struct {
    s16 light_model;
    s16 light_glow_model;
    s16 motions[6];
} BallPlayerData; // Size 0x10

typedef struct {
    struct {
        s8 kill : 1;
        u8 punching : 1;
    };
    s8 state;
    u8 phase;
} BallPlayerWork;

typedef struct {
    struct {
        s8 kill : 1;
        u8 state : 4;
        u8 motion_active : 1;
    };
    u8 delay;
    u8 timer;
    char padding0[1];
    s16 model;
    char padding1[2];
    float target_angle;
} BallBooWork;

typedef struct {
    /* 0x00 */ s16 model;
    /* 0x02 */ char padding0[2];
    /* 0x04 */ Vec position;
    /* 0x10 */ Vec gravity_velocity;
    /* 0x1C */ Vec drift_velocity;
    /* 0x28 */ char padding1[4];
} BallTakeCoinData; // Size 0x2C

typedef struct {
    struct {
        u8 kill : 1;
    };
    u8 elapsed_frames;
    s16 model;
    float floor_y;
} BallTakeCoinWork;

static BOOL CheckBallCoinDone(void);
static void TakeBallStar(void);
static void ComSetupStealPlayer(s32 arg0);
static void ComSetStealPlayerInput(s32 arg0, s32 arg1);
static void ExecCoinSteal(void);
static void ExecStarSteal(void);
static void ExecLightSteal(void);
static void SetBooMode(s32 arg0);
static void CreateBallMdl(s16 *arg0, Vec *arg1);
static void CreateBallView(void);
static void SetBallView(s32 arg0);
static void SetBallActive(s32 arg0);
static s32 CheckBallKill(void);
static void BallMain(OMOBJ *arg0);
static void BallRenderHook(void);
static void ExecBoo(OMOBJ *arg0);
static void UpdateBallCamera(OMOBJ *arg0);
static void CreateBallPlayer(void);
static s32 GetBallPlayerState(void);
static void SetBallPlayerState(s32 arg0);
static void ExecBallPlayer(OMOBJ *arg0);
static void BallPlayerZoomOut(OMOBJ *arg0, BallPlayerWork *arg1);
static void BallPlayerScare(OMOBJ *arg0, BallPlayerWork *arg1);
static void BallPlayerTurn(OMOBJ *arg0, BallPlayerWork *arg1);
static void BallPlayerPunch(OMOBJ *arg0, BallPlayerWork *arg1);
static void BallPlayerUseLight(OMOBJ *arg0, BallPlayerWork *arg1);
static void BallPlayerCatch(OMOBJ *arg0, BallPlayerWork *arg1);
static void BallBooCreate(void);
static void SetBallBooState(s32 arg0);
static s32 GetBallBooState(void);
static void ExecBallBoo(OMOBJ *arg0);
static void BallBooSpawn(OMOBJ *arg0, BallBooWork *arg1);
static void BallBooChase(OMOBJ *arg0, BallBooWork *arg1);
static void BallBooBattle(OMOBJ *arg0, BallBooWork *arg1);
static void BallBooAttack(OMOBJ *arg0, BallBooWork *arg1);
static void BallBooFlash(OMOBJ *arg0, BallBooWork *arg1);
static void TakeBallCoin(void);
static void ExecTakeBallCoin(OMOBJ *arg0);
static void ExecTakeBallStar(OMOBJ *arg0);
static void TakeBallCoinPosSet(BallTakeCoinData *arg0, Vec *arg1);
static void UpdatePlayerCoins(void);
static BOOL CheckTakeBallStarDone(void);

s8 lbl_801D41E2;
static s8 stealTarget;
static s8 stealType;
static s32 booKillF;
static u8 stealValue;
static u8 stealSuccess;
static u8 punchCount;
static s8 stealAvail[3];
static s8 starChoiceEnable[3];
static s32 battleTimer;
static s32 attackTimer;
static OMOBJ *ballTakeCoinObj;
static OMOBJ *ballPlayerObj;
static OMOBJ *ballBooObj;
static OMOBJ *ballCameraObj;
static OMOBJ *ballObj;
static OMOBJ *booEventObj;

static s32 booPlayerMotTbl[8][6] = {
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 11), DATA_MAKE_NUM(DATADIR_BYOKODORI, 19), DATA_MAKE_NUM(DATADIR_BYOKODORI, 27), DATA_MAKE_NUM(DATADIR_BYOKODORI, 35), DATA_MAKE_NUM(DATADIR_BYOKODORI, 43), DATA_MAKE_NUM(DATADIR_BYOKODORI, 51) },
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 12), DATA_MAKE_NUM(DATADIR_BYOKODORI, 20), DATA_MAKE_NUM(DATADIR_BYOKODORI, 28), DATA_MAKE_NUM(DATADIR_BYOKODORI, 36), DATA_MAKE_NUM(DATADIR_BYOKODORI, 44), DATA_MAKE_NUM(DATADIR_BYOKODORI, 52) },
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 13), DATA_MAKE_NUM(DATADIR_BYOKODORI, 21), DATA_MAKE_NUM(DATADIR_BYOKODORI, 29), DATA_MAKE_NUM(DATADIR_BYOKODORI, 37), DATA_MAKE_NUM(DATADIR_BYOKODORI, 45), DATA_MAKE_NUM(DATADIR_BYOKODORI, 53) },
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 14), DATA_MAKE_NUM(DATADIR_BYOKODORI, 22), DATA_MAKE_NUM(DATADIR_BYOKODORI, 30), DATA_MAKE_NUM(DATADIR_BYOKODORI, 38), DATA_MAKE_NUM(DATADIR_BYOKODORI, 46), DATA_MAKE_NUM(DATADIR_BYOKODORI, 54) },
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 15), DATA_MAKE_NUM(DATADIR_BYOKODORI, 23), DATA_MAKE_NUM(DATADIR_BYOKODORI, 31), DATA_MAKE_NUM(DATADIR_BYOKODORI, 39), DATA_MAKE_NUM(DATADIR_BYOKODORI, 47), DATA_MAKE_NUM(DATADIR_BYOKODORI, 55) },
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 16), DATA_MAKE_NUM(DATADIR_BYOKODORI, 24), DATA_MAKE_NUM(DATADIR_BYOKODORI, 32), DATA_MAKE_NUM(DATADIR_BYOKODORI, 40), DATA_MAKE_NUM(DATADIR_BYOKODORI, 48), DATA_MAKE_NUM(DATADIR_BYOKODORI, 56) },
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 17), DATA_MAKE_NUM(DATADIR_BYOKODORI, 25), DATA_MAKE_NUM(DATADIR_BYOKODORI, 33), DATA_MAKE_NUM(DATADIR_BYOKODORI, 41), DATA_MAKE_NUM(DATADIR_BYOKODORI, 49), DATA_MAKE_NUM(DATADIR_BYOKODORI, 57) },
    { DATA_MAKE_NUM(DATADIR_BYOKODORI, 18), DATA_MAKE_NUM(DATADIR_BYOKODORI, 26), DATA_MAKE_NUM(DATADIR_BYOKODORI, 34), DATA_MAKE_NUM(DATADIR_BYOKODORI, 42), DATA_MAKE_NUM(DATADIR_BYOKODORI, 50), DATA_MAKE_NUM(DATADIR_BYOKODORI, 58) }
};

static s32 booSfxTbl[4][8] = {
    {
        0x0000012F, 0x0000016F, 0x000001AF, 0x000001EF,
        0x0000022F, 0x0000026F, 0x000002AF, 0x000002EF,
    },
    {
        0x00000130, 0x00000170, 0x000001B0, 0x000001F0,
        0x00000230, 0x00000270, 0x000002B0, 0x000002F0
    },
    {
        0x00000131, 0x00000171, 0x000001B1, 0x000001F1,
        0x00000231, 0x00000271, 0x000002B1, 0x000002F1,
    },
    {
        0x00000123, 0x00000163, 0x000001A3, 0x000001E3,
        0x00000223, 0x00000263, 0x000002A3, 0x000002E3
    }
};

static s32 hostMess[] = {
    MAKE_MESSID(7, 14),
    MAKE_MESSID(7, 20),
    MAKE_MESSID(7, 26),
    MAKE_MESSID(7, 32),
    MAKE_MESSID(7, 38),
    MAKE_MESSID(7, 44)
};

static s32 ballMdlTbl[] = {
    DATA_MAKE_NUM(DATADIR_BYOKODORI, 2),
    DATA_MAKE_NUM(DATADIR_BYOKODORI, 6)
};

/**
 * @brief Create the two-model Boo ball controller.
 *
 * @details Resets theft state and creates the object that owns both ball model handles.
 * @param arg0 Player index treated as the event owner.
 * @param arg1 Optional world position for the ball; NULL places it at the origin.
 * @return The primary ball model handle.
 */
s16 BoardBooCreate(s32 arg0, Vec *arg1) {
    BooEventWork *work;
    Vec position;

    if (arg1) {
        position = *arg1;
    } else {
        position.x = position.y = position.z = 0.0f;
    }
    booEventObj = omAddObjEx(boardObjMan, 0x1000, 0, 0, -1, ExecBoo);
    work = omObjGetWork(booEventObj, BooEventWork);
    work->kill = 0;
    work->mode = 0;
    work->owner_player = arg0;
    booKillF = 0;
    stealTarget = -1;
    lbl_801D41E2 = 0;
    punchCount = 0;
    stealValue = 0;
    stealSuccess = 0;
    booEventObj->trans.x = position.x;
    booEventObj->trans.y = position.y;
    booEventObj->trans.z = position.z;
    CreateBallMdl(work->ball_models, &position);
    return work->ball_models[0];
}

/**
 * @brief Request shutdown of the Boo controller and its dependent objects.
 */
void BoardBooKill(void) {
    booKillF = 1;
    if (booEventObj) {
        omObjGetWork(booEventObj, BooEventWork)->kill = 1;
    }
}

/**
 * @brief Present the target picker for a coin or star theft.
 *
 * @details Disables ineligible targets, supplies computer input when needed, records the selected player, and enables the inset theft view.
 * @param arg0 Theft type: 0 for coins or 1 for a star.
 * @return Zero when no opponent can supply the requested reward; the successful path has no explicit return and callers ignore its value.
 */
s32 BoardBooStealTypeSet(s32 arg0) {
    BooEventWork *work = omObjGetWork(booEventObj, BooEventWork);
    s32 portrait;
    s32 target_player;
    s32 available_count;
    s32 message_id;
    s32 choice_index;
    s32 i;

    stealType = arg0;
    for (available_count = choice_index = i = 0; i < 4; i++) {
        if (i != work->owner_player) {
            target_player = i;
            stealAvail[choice_index] = target_player;
            if ((stealType == 0 && BoardPlayerCoinsGet(target_player) == 0) || (stealType == 1 && GWStarsGet(target_player) == 0)) {
                starChoiceEnable[choice_index] = 0;
            } else {
                starChoiceEnable[choice_index] = 1;
                available_count++;
            }
            choice_index++;
        }
    }
    if (available_count == 0) {
        stealType = -1;
        return 0;
    }
    if (_CheckFlag(FLAG_ID_MAKE(1, 24))) {
        portrait = BoardWinPortraitGet();
        message_id = hostMess[GWBoardGet()];
    } else {
        portrait = 3;
        message_id = MAKE_MESSID(18, 25);
    }
    BoardWinCreateChoice(1, message_id, portrait, 0);
    BoardWinAttrSet(0x10);
    if (stealType == 0) {
        message_id = MAKE_MESSID(7, 48);
    } else {
        message_id = MAKE_MESSID(7, 49);
    }
    BoardWinInsertMesSet(message_id, 0);
    for (i = 0; i < 3; i++) {
        BoardWinInsertMesSet(BoardPlayerGetCharMess(stealAvail[i]), i + 1);
        if (starChoiceEnable[i] == 0) {
            BoardWinChoiceDisable(i);
        }
    }
    if (GWPlayer[work->owner_player].com) {
        ComSetupStealPlayer(work->owner_player);
    }
    BoardWinWait();
    stealTarget = BoardWinChoiceGet();
    if (stealTarget == 3) {
        stealTarget = -1;
        while (stealTarget == -1) {
            choice_index = BoardRandMod(3);
            if (starChoiceEnable[choice_index] != 0) {
                stealTarget = stealAvail[choice_index];
            }
        }
    } else {
        stealTarget = stealAvail[stealTarget];
    }
    BoardWinKill();
    CreateBallView();
    SetBallView(1);
}

/**
 * @brief Choose a target-selection index for a computer player.
 *
 * @details Scores eligible opponents by coins or stars while accounting for teams, difficulty, and Boo-repellent lights.
 * @param arg0 Index of the computer player initiating the theft.
 */
static void ComSetupStealPlayer(s32 arg0) {
    s32 best_value;
    s32 available_index;
    s32 light_ignore_chance;
    s32 target_player;
    s32 best_choice;
    s32 target_value;
    s32 ignore_light;
    s32 random_roll;
    s32 i;
    s16 target_values[3];

    switch (GWPlayer[arg0].diff) {
        case 0:
            light_ignore_chance = 95;
            break;
        case 1:
            light_ignore_chance = 50;
            break;
        case 2:
            light_ignore_chance = 5;
            break;
        case 3:
            light_ignore_chance = 0;
            break;
    }
    random_roll = BoardRandMod(100);
    if (random_roll >= light_ignore_chance) {
        ignore_light = 1;
    } else {
        ignore_light = 0;
    }
    for (i = 0; i < 3; i++) {
        target_player = stealAvail[i];
        if (stealType == 1) {
            target_value = GWStarsGet(target_player);
        } else {
            target_value = BoardPlayerCoinsGet(target_player);
        }
        if (starChoiceEnable[i] == 0) {
            target_value = -1;
        }
        if ((s32) GWSystem.team != 0 && target_player == BoardPlayerSameTeamFind(arg0)) {
            target_value = 0;
        }
        if (BoardPlayerItemFind(target_player, 8) != -1 && ignore_light != 0) {
            target_value = 0;
        }
        target_values[i] = target_value;
    }
    best_value = best_choice = -1;
    for (available_index = i = 0; i < 3; i++) {
        target_value = target_values[i];
        if (target_value != -1) {
            if (target_value > 0 && target_value >= best_value) {
                best_value = target_value;
                best_choice = available_index;
            }
            available_index++;
        }
    }
    if (best_choice == -1) {
        best_choice = 0;
    }
    ComSetStealPlayerInput(arg0, best_choice);
}

/**
 * @brief Send simulated window-navigation input for a computer target choice.
 * @param arg0 Index of the computer player whose controller port is used.
 * @param arg1 Number of downward choice movements before confirming.
 */
static void ComSetStealPlayerInput(s32 arg0, s32 arg1) {
    s32 controller_inputs[4];
    s32 port;
    s32 i;
    s16 input_delay;

    controller_inputs[0] = controller_inputs[1] = controller_inputs[2] = controller_inputs[3] = 0;
    port = GWPlayer[arg0].port;
    input_delay = GWMessDelayGet();
    controller_inputs[port] = 4;
    if (arg1 != 0) {
        for (i = 0; i < arg1; i++) {
            HuWinComKeyWait(controller_inputs[0], controller_inputs[1], controller_inputs[2], controller_inputs[3], input_delay);
        }
    }
    controller_inputs[port] = 0x100;
    HuWinComKeyWait(controller_inputs[0], controller_inputs[1], controller_inputs[2], controller_inputs[3], input_delay);
}

/**
 * @brief Play the inset theft encounter against the selected target.
 *
 * @details Temporarily hides other players, runs the player and Boo state machines, applies the selected theft animation, and restores the board view.
 * @return Zero when a Boo-repellent light prevents the theft; one after a coin or star theft.
 */
s32 BoardBooStealMain(void) {
    BooEventWork *work;
    s32 player_index;
    s32 message_id;

    stealType == -1;
    work = omObjGetWork(booEventObj, BooEventWork);
    if (stealType == 0) {
        SetBooMode(2);
    } else {
        SetBooMode(1);
    }
    player_index = BoardPlayerItemFind(stealTarget, 8);
    if (player_index != -1) {
        stealType = 2;
        BoardPlayerItemRemove(stealTarget, player_index);
        message_id = MAKE_MESSID(7, 4);
    } else {
        message_id = MAKE_MESSID(7, 3);
    }
    BoardCameraMotionWait();
    for (player_index = 0; player_index < 4; player_index++) {
        if (stealTarget != player_index) {
            BoardModelVisibilitySet(BoardPlayerModelGet(player_index), 0);
        }
    }
    CreateBallPlayer();
    omVibrate(stealTarget, 12, 4, 2);
    SetBallActive(1);
    while (!CheckBallKill()) {
        HuPrcVSleep();
    }
    if (stealType == 0) {
        BoardWinCreate(2, message_id, -1);
        BoardWinInsertMesSet(BoardPlayerGetCharMess(stealTarget), 0);
        BoardWinPause();
    }
    switch (stealType) {
        case 0:
            ExecCoinSteal();
            break;
        case 1:
            ExecStarSteal();
            break;
        case 2:
            ExecLightSteal();
            break;
    }
    BoardWinKill();
    SetBallActive(-1);
    while (!CheckBallKill()) {
        HuPrcVSleep();
    }
    booKillF = 1;
    SetBallView(0);
    for (player_index = 0; player_index < 4; player_index++) {
        if (stealTarget != player_index) {
            BoardModelVisibilitySet(BoardPlayerModelGet(player_index), 1);
        }
    }
    HuPrcSleep(2);
    if (stealType == 2) {
        return 0;
    } else {
        return 1;
    }
}

/**
 * @brief Sequence the Boo chase, struggle, coin capture, and retreat.
 */
static void ExecCoinSteal(void) {
    while (GetBallPlayerState() != 2) {
        HuPrcVSleep();
    }
    BallBooCreate();
    while (GetBallBooState() != 0) {
        HuPrcVSleep();
    }
    while (GetBallPlayerState() != 0) {
        HuPrcVSleep();
    }
    SetBallBooState(2);
    while (GetBallBooState() != 0) {
        HuPrcVSleep();
    }
    SetBallPlayerState(4);
    SetBallBooState(3);
    punchCount = 0;
    HuPrcSleep(120);
    while (GetBallBooState() != 0) {
        HuPrcVSleep();
    }
    while (GetBallPlayerState() != 0) {
        HuPrcVSleep();
    }
}

/**
 * @brief Sequence the Boo chase, star capture, and retreat.
 */
static void ExecStarSteal(void) {
    while (GetBallPlayerState() != 2) {
        HuPrcVSleep();
    }
    BallBooCreate();
    while (GetBallBooState() != 0) {
        HuPrcVSleep();
    }
    while (GetBallPlayerState() != 0) {
        HuPrcVSleep();
    }
    SetBallBooState(2);
    while (GetBallBooState() != 0) {
        HuPrcVSleep();
    }
    SetBallBooState(4);
    while (GetBallBooState() != 0) {
        HuPrcVSleep();
    }
    while (GetBallPlayerState() != 0) {
        HuPrcVSleep();
    }
}

/**
 * @brief Sequence the Boo encounter repelled by the target's light item.
 */
static void ExecLightSteal(void) {
    while (GetBallPlayerState() != 0) {
        HuPrcVSleep();
    }
    BallBooCreate();
    while (GetBallBooState() != 0) {
        HuPrcVSleep();
    }
    SetBallBooState(2);
    while (GetBallBooState() != 0) {
        HuPrcVSleep();
    }
    SetBallPlayerState(6);
    while (GetBallPlayerState() != 0) {
        HuPrcVSleep();
    }
    while (GetBallBooState() != 0) {
        HuPrcVSleep();
    }
}

/**
 * @brief Store the display mode used by the Boo ball controller.
 * @param arg0 Value stored in BooEventWork.mode; callers use 2 for coins and 1 for stars or a light defense, but this file contains no reader for the field.
 */
static void SetBooMode(s32 arg0) {
    omObjGetWork(booEventObj, BooEventWork)->mode = arg0;
}

/**
 * @brief Create and hook together the outer and inner Boo ball models.
 * @param arg0 Two-element array that receives the created model handles.
 * @param arg1 World position applied to both models.
 */
static void CreateBallMdl(s16 *arg0, Vec *arg1) {
    s32 i;

    for (i = 0; i < 2; i++) {
        arg0[i] = BoardModelCreate(ballMdlTbl[i], NULL, 0);
        BoardModelPosSetV(arg0[i], arg1);
    }
    BoardModelMotionStart(arg0[0], 0, 0x40000001);
    BoardModelHookSet(arg0[0], "sui", arg0[1]);
    BoardModelPosSet(arg0[1], 0.0f, 0.0f, 0.0f);
    BoardModelVisibilitySet(arg0[1], 1);
}

/**
 * @brief Create the projected inset view used during a Boo theft.
 *
 * @details Allocates an 80-by-80 capture texture, a projection, a render-hook model, and a dedicated camera object.
 */
static void CreateBallView(void) {
    ANIMBMP *bitmap;
    BooEventWork *boo_work;
    BallWork *ball_work;
    BallCameraWork *camera_work;

    boo_work = omObjGetWork(booEventObj, BooEventWork);
    ballObj = omAddObjEx(boardObjMan, 0x1001, 0, 0, -1, BallMain);
    ball_work = omObjGetWork(ballObj, BallWork);
    ball_work->kill = 0;
    ball_work->animation = HuSprAnimMake(80, 80, 2);
    bitmap = ball_work->animation->bmp;
    bitmap->data = HuMemDirectMallocNum(HEAP_HEAP, 0x3200, HU_MEMNUM_OVL);
    memset(bitmap->data, 0, 0x3200);
    ball_work->render_model = Hu3DHookFuncCreate((void*) BallRenderHook);
    Hu3DModelLayerSet(ball_work->render_model, 3);
    Hu3DModelCameraSet(ball_work->render_model, 1);
    ball_work->projection = Hu3DProjectionCreate(ball_work->animation, 25.0f, 100.0f, 13000.0f);
    Hu3DModelProjectionSet(BoardModelIDGet(boo_work->ball_models[1]), ball_work->projection);
    Hu3DProjectionTPLvlSet(ball_work->projection, 0.0f);
    ballCameraObj = omAddObjEx(boardObjMan, 0x7E03, 0, 0, -1, UpdateBallCamera);
    camera_work = omObjGetWork(ballCameraObj, BallCameraWork);
    camera_work->kill = 0;
    camera_work->camera = 1;
    ballCameraObj->rot.x = -8.0f;
    ballCameraObj->rot.y = 80.0f;
    ballCameraObj->rot.z = 300.0f;
    Hu3DCameraPerspectiveSet(camera_work->camera, 25.0f, 10.0f, 2000.0f, 1.2f);
    Hu3DCameraScissorSet(camera_work->camera, 0, 0, 0xA0, 0xA0);
    Hu3DCameraViewportSet(camera_work->camera, 0.0f, 0.0f, 160.0f, 160.0f, 0.0f, 1.0f);
}

/**
 * @brief Switch board rendering between the theft inset and normal camera.
 * @param arg0 Nonzero to use camera mask 2 for the inset; zero to restore camera mask 1.
 */
static void SetBallView(s32 arg0) {
    u16 camera_mask;
    s32 i;

    if (arg0 != 0) {
        camera_mask = 2;
    } else {
        Hu3DCameraPerspectiveSet(2, -1.0f, 0.0f, 0.0f, 0.0f);
        camera_mask = 1;
    }
    for (i = 0; i < 4; i++) {
        BoardPlayerCameraSet(i, camera_mask);
    }
    BoardSpaceCameraSet(camera_mask);
    BoardModelCameraSetAll(camera_mask);
    BoardCameraMaskSet((u16) camera_mask);
    BoardCameraScissorSet(0, 0, 640, 480);
}

/**
 * @brief Start fading the projected theft image in or out.
 * @param arg0 Fade direction multiplier: 1 increases opacity and -1 decreases it, in steps of eight.
 */
static void SetBallActive(s32 arg0) {
    BallWork *work = omObjGetWork(ballObj, BallWork);

    work->fade_step = arg0 * 8;
}

/**
 * @brief Test whether the projected theft image has finished fading.
 * @return One when no ball-view object exists or its fade step has reached zero; otherwise zero.
 */
static s32 CheckBallKill(void) {
    BallWork *work;

    if (!ballObj) {
        return 1;
    }
    work = omObjGetWork(ballObj, BallWork);
    if (work->fade_step != 0) {
        return 0;
    } else {
        return 1;
    }
}

/**
 * @brief Update the theft-image projection and fade opacity.
 *
 * @details Destroys the projection and render-hook model when the Boo event ends.
 * @param arg0 Ball-view object containing the projection and captured texture state.
 */
static void BallMain(OMOBJ *arg0) {
    BallWork *work = omObjGetWork(arg0, BallWork);
    Vec camera_target;
    Vec camera_rot;
    Vec projection_up;
    Vec projection_pos;
    float camera_distance;
    float opacity;
    s16 *ball_models;

    if (booKillF != 0 || BoardIsKill() != 0) {
        ball_models = omObjGetWork(booEventObj, BooEventWork)->ball_models;
        Hu3DModelProjectionReset(BoardModelIDGet(ball_models[1]), work->projection);
        Hu3DProjectionKill(work->projection);
        Hu3DModelKill(work->render_model);
        ballObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    camera_distance = 240.0f;
    BoardCameraRotGet(&camera_rot);
    BoardCameraTargetGet(&camera_target);
    projection_pos.x = camera_target.x + sind(camera_rot.y) * cosd(camera_rot.x) * camera_distance;
    projection_pos.y = camera_target.y + -sind(camera_rot.x) * camera_distance;
    projection_pos.z = camera_target.z + cosd(camera_rot.y) * cosd(camera_rot.x) * camera_distance;
    projection_up.x = sind(camera_rot.y) * sind(camera_rot.x);
    projection_up.y = cosd(camera_rot.x);
    projection_up.z = cosd(camera_rot.y) * sind(camera_rot.x);
    if (work->fade_step != 0) {
        work->opacity += work->fade_step;
        if (work->opacity > 255) {
            work->opacity = 255;
            work->fade_step = 0;
        } else if (work->opacity < 0) {
            work->opacity = 0;
            work->fade_step = 0;
        }
    }
    Hu3DProjectionPosSet(work->projection, &projection_pos, &projection_up, &camera_target);
    OSs16tof32(&work->opacity, &opacity);
    opacity = opacity * 0.003921569f;
    Hu3DProjectionTPLvlSet(work->projection, opacity);
}

/**
 * @brief Render a black silhouette into the Boo theft capture texture.
 *
 * @details Copies a 160-by-160 render target into the 80-by-80 animation texture used by the projection.
 */
static void BallRenderHook(void) {
    BallWork *work;
    GXColor clear_color;
    Vec camera_pos;
    Vec camera_up;
    Vec camera_target;
    Mtx44 projection_mtx;
    Mtx view_mtx;

    if (!ballObj) {
        return;
    }
    work = omObjGetWork(ballObj, BallWork);
    clear_color.a = clear_color.r = clear_color.g = clear_color.b = 0;
    GXSetCopyClear(clear_color, -1);
    GXSetTexCopySrc(0, 0, 160, 160);
    GXSetTexCopyDst(80, 80, GX_TF_RGB5A3, GX_TRUE);
    GXCopyTex(work->animation->bmp->data, GX_TRUE);
    DCFlushRange(work->animation->bmp->data, 12800);
    GXPixModeSync();
    C_MTXPerspective(projection_mtx, 25.0f, 1.2f, 100.0f, 13000.0f);
    GXSetProjection(projection_mtx, GX_PERSPECTIVE);
    GXSetViewport(0.0f, 0.0f, 160.0f, 160.0f, 0.0f, 1.0f);
    GXSetScissor(0, 0, 160, 160);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_RGBA6, 0);
    clear_color.r = 0;
    clear_color.g = 0;
    clear_color.b = 0;
    clear_color.a = 0xFF;
    GXSetTevColor(GX_TEVREG0, clear_color);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetCullMode(GX_CULL_NONE);
    GXSetColorUpdate(GX_FALSE);
    GXSetAlphaUpdate(GX_FALSE);
    GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
    camera_pos.x = 0.0f;
    camera_pos.y = 0.0f;
    camera_pos.z = 100.0f;
    camera_target.x = 0.0f;
    camera_target.y = 0.0f;
    camera_target.z = 0.0f;
    camera_up.x = 0.0f;
    camera_up.y = 1.0f;
    camera_up.z = 0.0f;
    C_MTXLookAt(view_mtx, &camera_pos, &camera_up, &camera_target);
    GXLoadPosMtxImm(view_mtx, GX_PNMTX0);
    camera_pos.x = sind(12.5) * 20000.0 * 1.2f;
    camera_pos.y = sind(12.5) * 20000.0 * 0.8333333f;
    camera_pos.z = -9000.0f;
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-camera_pos.x, -camera_pos.y, camera_pos.z);
    GXPosition3f32(camera_pos.x, -camera_pos.y, camera_pos.z);
    GXPosition3f32(camera_pos.x, camera_pos.y, camera_pos.z);
    GXPosition3f32(-camera_pos.x, camera_pos.y, camera_pos.z);
    GXEnd();
    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_TRUE);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
}

/**
 * @brief Destroy the Boo ball models when the event is killed.
 * @param arg0 Boo controller object that owns the two ball model handles.
 */
static void ExecBoo(OMOBJ *arg0) {
    BooEventWork *work = omObjGetWork(arg0, BooEventWork);
    s32 i;

    if (work->kill != 0 || BoardIsKill()) {
        for (i = 0; i < 2; i++) {
            BoardModelKill(((s16*) work->ball_models)[i]);
        }
        BoardWinKill();
        booEventObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
}

/**
 * @brief Update the dedicated theft camera from its object transform.
 * @param arg0 Camera object whose translation, pitch, height, and distance define the view.
 */
static void UpdateBallCamera(OMOBJ *arg0) {
    BallCameraWork *work = omObjGetWork(arg0, BallCameraWork);
    Vec camera_target;
    Vec camera_pos;
    Vec camera_up;

    if (booKillF != 0 || BoardIsKill()) {
        ballCameraObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    camera_target.x = arg0->trans.x;
    camera_target.y = arg0->trans.y + arg0->rot.y;
    camera_target.z = arg0->trans.z;
    camera_pos.x = camera_target.x + sin(0.0) * cosd(arg0->rot.x) * arg0->rot.z;
    camera_pos.y = camera_target.y + -sind(arg0->rot.x) * arg0->rot.z;
    camera_pos.z = camera_target.z + cos(0.0) * cosd(arg0->rot.x) * arg0->rot.z;
    camera_up.x = sin(0.0) * sind(arg0->rot.x);
    camera_up.y = cosd(arg0->rot.x);
    camera_up.z = cos(0.0) * sind(arg0->rot.x);
    Hu3DCameraPosSetV(work->camera, &camera_pos, &camera_up, &camera_target);
}

/**
 * @brief Copy a world position into the optional theft-camera target object.
 * @param x World x coordinate.
 * @param y World y coordinate.
 * @param z World z coordinate.
 */
static inline void CreateBallPlayerInlineFunc(float x, float y, float z) {
    if (ballCameraObj) {
        ballCameraObj->trans.x = x;
        ballCameraObj->trans.y = y;
        ballCameraObj->trans.z = z;
    }
}

/**
 * @brief Create the selected player's theft-scene controller and motions.
 *
 * @details Saves the player position in the object transform, creates six character motions, and creates and hooks the light models when a repellent light is being used.
 */
static void CreateBallPlayer(void) {
    char *item_hook;
    Vec player_pos;
    s32 character;
    s32 motion_data;
    s32 i;
    OMOBJ *object;
    BallPlayerData *data;
    BallPlayerWork *work;

    ballPlayerObj = object = omAddObjEx(boardObjMan, 0x1005, 0, 0, -1, ExecBallPlayer);
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(BallPlayerData), HU_MEMNUM_OVL);
    work = omObjGetWork(object, BallPlayerWork);
    work->kill = 0;
    SetBallPlayerState(1);
    object->trans.x = 0.0f;
    object->trans.y = 0.0f;
    object->trans.z = 0.0f;
    BoardPlayerPosGet(stealTarget, &player_pos);
    object->scale.x = player_pos.x;
    object->scale.y = player_pos.y;
    object->scale.z = player_pos.z;
    object->rot.x = 300.0f;
    object->rot.y = 750.0f;
    character = GWPlayer[stealTarget].character;
    data = object->data;
    for (i = 0; i < 6; i++) {
        motion_data = booPlayerMotTbl[character][i];
        data->motions[i] = BoardPlayerMotionCreate(stealTarget, motion_data);
    }
    if (stealType == 2) {
        data->light_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 118), 0, 0);
        data->light_glow_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 130), 0, 0);
        BoardModelLayerSet(data->light_model, 0);
        BoardModelLayerSet(data->light_glow_model, 0);
        BoardModelCameraSet(data->light_model, 1);
        BoardModelCameraSet(data->light_glow_model, 1);
        BoardModelScaleSet(data->light_model, 0.4f, 0.4f, 0.4f);
        item_hook = CharModelItemHookGet(character, 2, 0);
        BoardModelHookSet(data->light_model, "b00p_item30", data->light_glow_model);
        BoardModelHookSet(BoardPlayerModelGet(stealTarget), item_hook, data->light_model);
    } else {
        data->light_model = -1;
        data->light_glow_model = -1;
    }
    if (stealType != 2) {
        BoardPlayerMotionStart(stealTarget, data->motions[1], 0x40000001);
    } else {
        BoardPlayerMotionStart(stealTarget, data->motions[3], 0x40000001);
    }
    BoardPlayerPosSet(stealTarget, object->trans.x, object->trans.y, object->trans.z);
    BoardPlayerLayerSet(stealTarget, 0);
    BoardPlayerCameraSet(stealTarget, 1);
    if (ballCameraObj) {
        ballCameraObj->rot.x = -8.0f;
        ballCameraObj->rot.y = 80.0f;
        ballCameraObj->rot.z = 300.0f;
    }
    CreateBallPlayerInlineFunc(object->trans.x, object->trans.y, object->trans.z);
}

/**
 * @brief Read the selected player controller state.
 * @return The current BallPlayerWork state number.
 */
static s32 GetBallPlayerState(void) {
    BallPlayerWork *work = omObjGetWork(ballPlayerObj, BallPlayerWork);

    return work->state;
}

/**
 * @brief Enter a selected-player animation state and reset its phase.
 * @param arg0 Player state number dispatched by ExecBallPlayer.
 */
static void SetBallPlayerState(s32 arg0) {
    BallPlayerWork *work = omObjGetWork(ballPlayerObj, BallPlayerWork);

    work->state = arg0;
    work->phase = 0;
}

/**
 * @brief Dispatch the selected player's theft-scene state machine.
 *
 * @details Cleanup restores the player model, camera, layer, position, and default motion.
 * @param arg0 Player controller object containing saved position, motions, and BallPlayerWork.
 */
static void ExecBallPlayer(OMOBJ *arg0) {
    BallPlayerWork *work = omObjGetWork(arg0, BallPlayerWork);
    BallPlayerData *data = arg0->data;
    BallWork *ball_work;
    s32 i;

    if (booKillF != 0 || BoardIsKill()) {
        BoardPlayerPosSet(stealTarget, arg0->scale.x, arg0->scale.y, arg0->scale.z);
        BoardPlayerRotYSet(stealTarget, 0.0f);
        BoardPlayerLayerSet(stealTarget, 0);
        BoardPlayerCameraSet(stealTarget, 1);
        BoardModelHookReset(data->light_model);
        BoardModelHookReset(BoardPlayerModelGet(stealTarget));
        BoardModelKill(data->light_model);
        BoardModelKill(data->light_glow_model);
        BoardPlayerMotionStart(stealTarget, 1, 0x40000001);
        for (i = 0; i < 6; i++) {
            BoardPlayerMotionKill(stealTarget, data->motions[i]);
        }
        if (arg0->data) {
            HuMemDirectFree(arg0->data);
            arg0->data = NULL;
        }
        ballPlayerObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    ball_work = omObjGetWork(ballObj, BallWork);
    if (ball_work->fade_step == 0) {
        switch (work->state) {
            case 1:
                BallPlayerZoomOut(arg0, work);
                break;
            case 3:
                BallPlayerScare(arg0, work);
                break;
            case 2:
                BallPlayerTurn(arg0, work);
                break;
            case 4:
                BallPlayerPunch(arg0, work);
                break;
            case 6:
                BallPlayerUseLight(arg0, work);
                break;
            case 5:
                BallPlayerCatch(arg0, work);
                break;
        }
    }
}

/**
 * @brief Move the theft camera outward before the encounter begins.
 * @param arg0 Player controller object holding the current and target camera distances.
 * @param arg1 Player work changed to the scare state when the zoom completes.
 */
static void BallPlayerZoomOut(OMOBJ *arg0, BallPlayerWork *arg1) {
    Vec start_pos;
    Vec end_pos;
    float camera_distance;
    s32 character;

    arg0->rot.x += 8.0f;
    if (arg0->rot.y < arg0->rot.x) {
        arg0->rot.x = arg0->rot.y;
        start_pos.x = arg0->trans.x;
        start_pos.y = arg0->trans.y;
        start_pos.z = arg0->trans.z;
        end_pos.x = start_pos.x + 120.0 * sind(-90);
        end_pos.y = start_pos.y;
        end_pos.z = start_pos.z + 120.0 * cosd(-90);
        BoardPlayerPosLerpStart(stealTarget, &start_pos, &end_pos, 20);
        character = GWPlayer[stealTarget].character;
        HuAudFXPlay(booSfxTbl[0][character]);
        SetBallPlayerState(3);
    }
    camera_distance = arg0->rot.x;
    if (ballCameraObj && camera_distance != -1.0f) {
        ballCameraObj->rot.z = camera_distance;
    }
}

/**
 * @brief Wait for player movement to stop, then face the encounter.
 *
 * @details A light defense uses its held-light motion; other thefts rotate the player toward Boo.
 * @param arg0 Player controller object containing the character motions.
 * @param arg1 Player work advanced to the next encounter state.
 */
static void BallPlayerScare(OMOBJ *arg0, BallPlayerWork *arg1) {
    BallPlayerData *data;
    float target_angle;

    if (!GWPlayer[stealTarget].moving) {
        data = arg0->data;
        target_angle = -BoardPlayerRotYGet(stealTarget);
        if (stealType == 2) {
            BoardPlayerMotionShiftSet(stealTarget, data->motions[3], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
            SetBallPlayerState(0);
        } else {
            BoardPlayerMotBlendSet(stealTarget, target_angle, 15);
            SetBallPlayerState(2);
        }
    }
}

/**
 * @brief Finish the selected player's turn toward Boo.
 * @param arg0 Unused player object retained by the state-handler interface.
 * @param arg1 Player work whose state and punch flag are cleared when blending ends.
 */
static void BallPlayerTurn(OMOBJ *arg0, BallPlayerWork *arg1) {
    if (BoardPlayerMotBlendCheck(stealTarget)) {
        SetBallPlayerState(0);
        arg1->punching = 0;
    }
}

/**
 * @brief Read or synthesize punch input during a coin-theft struggle.
 *
 * @details Computer punch probability depends on difficulty; accepted presses increment punchCount before the 120-frame battle limit.
 * @param arg0 Player controller object containing the punch motion.
 * @param arg1 Player work tracking whether the punch motion is active.
 */
static void BallPlayerPunch(OMOBJ *arg0, BallPlayerWork *arg1) {
    BallPlayerData *data;
    s32 random_roll;
    u32 punch_input;

    punch_input = 0;
    data = arg0->data;
    if (GWPlayer[stealTarget].com) {
        random_roll = BoardRandMod(100);
        switch (GWPlayer[stealTarget].diff) {
            case 0:
                if (random_roll > 90) {
                    punch_input = 0x100;
                }
                break;
            case 1:
                if (random_roll > 83) {
                    punch_input = 0x100;
                }
                break;
            case 2:
                if (random_roll > 75) {
                    punch_input = 0x100;
                }
                break;
            case 3:
                if (random_roll > 68) {
                    punch_input = 0x100;
                }
                break;
        }
    } else {
        punch_input = HuPadBtnDown[GWPlayer[stealTarget].port] & 0x100;
    }
    if ((punch_input & 0x100) && GetBallBooState() == 3 && battleTimer < 120) {
        punchCount++;
    }
    if (arg1->punching != 0) {
        if (BoardPlayerMotionEndCheck(stealTarget)) {
            arg1->punching = 0;
            BoardPlayerMotionStart(stealTarget, 1, 0x40000001);
        }
    } else {
        if (punch_input & 0x100) {
            arg1->punching = 1;
            BoardPlayerMotionStart(stealTarget, data->motions[2], 0);
        }
    }
}

/**
 * @brief Play the two-stage Boo-repellent light animation.
 * @param arg0 Player controller object containing the light models and motions.
 * @param arg1 Player work whose phase selects raise-light, repel-Boo, or completion.
 */
static void BallPlayerUseLight(OMOBJ *arg0, BallPlayerWork *arg1) {
    BallPlayerData *data = arg0->data;

    switch (arg1->phase) {
        case 0:
            BoardModelScaleSet(data->light_glow_model, 2.5f, 2.5f, 2.5f);
            BoardPlayerMotionStart(stealTarget, data->motions[4], 0);
            arg1->phase = 1;
            break;
        case 1:
            if (BoardPlayerMotionEndCheck(stealTarget)) {
                SetBallBooState(7);
                BoardPlayerMotionStart(stealTarget, data->motions[5], 0);
                arg1->phase = 2;
            }
            break;
        case 2:
            if (BoardPlayerMotionEndCheck(stealTarget)) {
                SetBallPlayerState(0);
            }
            break;
    }
}

/**
 * @brief Apply the coin or star loss and wait for its pickup effect.
 * @param arg0 Player controller object containing the reaction motion.
 * @param arg1 Player work whose phase prevents applying the loss twice.
 */
static void BallPlayerCatch(OMOBJ *arg0, BallPlayerWork *arg1) {
    s32 character;
    BallPlayerData *data = arg0->data;

    character = GWPlayer[stealTarget].character;
    if (arg1->phase == 0) {
        if (stealType == 0) {
            UpdatePlayerCoins();
            HuAudFXPlay(booSfxTbl[2][character]);
            omVibrate(stealTarget, 12, 4, 2);
            TakeBallCoin();
        }
        if (stealType == 1) {
            BoardPlayerStarsAdd(stealTarget, -1);
            HuAudFXPlay(booSfxTbl[2][character]);
            omVibrate(stealTarget, 12, 4, 2);
            TakeBallStar();
        }
        BoardPlayerMotionShiftSet(stealTarget, data->motions[0], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
        arg1->phase = 1;
    }
    if (stealType == 0) {
        if (CheckBallCoinDone()) {
            SetBallPlayerState(0);
        }
    } else {
        if (stealType == 1 && CheckTakeBallStarDone()) {
            SetBallPlayerState(0);
        }
    }
}

static s32 ballBooMdl[] = {
    DATA_MAKE_NUM(DATADIR_BGUEST, 19),
    DATA_MAKE_NUM(DATADIR_BGUEST, 20),
    DATA_MAKE_NUM(DATADIR_BGUEST, 21),
    DATA_MAKE_NUM(DATADIR_BGUEST, 22),
    DATA_MAKE_NUM(DATADIR_BGUEST, 23),
    -1
};

/**
 * @brief Create Boo in front of the selected player and set its approach vector.
 *
 * @details The destination height and distance differ for a normal theft and a light-defense encounter.
 */
static void BallBooCreate(void) {
    BallBooWork *work;
    Vec player_pos;
    Vec target_pos;
    float player_angle;
    float target_distance;
    float target_height;

    ballBooObj = omAddObjEx(boardObjMan, 0x1004, 0, 0, -1, ExecBallBoo);
    work = omObjGetWork(ballBooObj, BallBooWork);
    work->kill = 0;
    work->motion_active = 0;
    work->delay = 0;
    SetBallBooState(1);
    work->model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BGUEST, 18), ballBooMdl, 0);
    BoardModelAlphaSet(work->model, 0xFF);
    BoardModelLayerSet(work->model, 0);
    BoardModelCameraSet(work->model, 1);
    player_angle = BoardPlayerRotYGet(stealTarget);
    BoardPlayerPosGet(stealTarget, &player_pos);
    ballBooObj->trans.x = player_pos.x + 240.0 * sind(-player_angle);
    ballBooObj->trans.y = player_pos.y + 160.0f;
    ballBooObj->trans.z = player_pos.z + 240.0 * cosd(-player_angle);
    BoardModelPosSet(work->model, ballBooObj->trans.x, ballBooObj->trans.y, ballBooObj->trans.z);
    if (stealType == 2) {
        target_distance = 126.0f;
        target_height = 80.0f;
    } else {
        target_distance = 90.0f;
        target_height = 100.0f;
    }
    target_pos.x = player_pos.x + target_distance * sind(-player_angle);
    target_pos.y = player_pos.y + target_height;
    target_pos.z = player_pos.z + target_distance * cosd(-player_angle);
    ballBooObj->rot.x = (target_pos.x - ballBooObj->trans.x) / 60.0f;
    ballBooObj->rot.y = (target_pos.y - ballBooObj->trans.y) / 60.0f;
    ballBooObj->rot.z = (target_pos.z - ballBooObj->trans.z) / 60.0f;
    work->timer = 60;
    ballBooObj->scale.x = 1.0f;
    ballBooObj->scale.z = 1.0f;
    target_pos.x = player_pos.x - ballBooObj->trans.x;
    target_pos.z = player_pos.z - ballBooObj->trans.z;
    ballBooObj->scale.y = atan2d(target_pos.x, target_pos.z);
    BoardModelRotYSet(work->model, ballBooObj->scale.y);
    BoardModelMotionStart(work->model, 1, 0x40000001);
}

/**
 * @brief Enter a Boo encounter state and initialize its movement or animation.
 * @param arg0 Boo state number; states 2 and 4 configure approaches, 3 starts the punch battle, 5 starts attack scaling, and 7 starts the light retreat.
 */
static void SetBallBooState(s32 arg0) {
    BallBooWork *work = omObjGetWork(ballBooObj, BallBooWork);
    Vec target_pos;

    work->state = arg0;
    if (arg0 == 2) {
        HuAudFXPlay(0x64);
    }
    if (arg0 == 4) {
        BoardPlayerPosGet(stealTarget, &target_pos);
        target_pos.y += 100.0f;
        ballBooObj->rot.x = (target_pos.x - ballBooObj->trans.x) / 30.0f;
        ballBooObj->rot.y = (target_pos.y - ballBooObj->trans.y) / 30.0f;
        ballBooObj->rot.z = (target_pos.z - ballBooObj->trans.z) / 30.0f;
        work->timer = 30;
        BoardModelMotionShiftSet(work->model, 1, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    }
    if (arg0 == 5) {
        HuAudFXPlay(0x64);
        work->timer = 0;
    }
    if (arg0 == 7) {
        work->target_angle = -ballBooObj->scale.y;
        HuAudFXPlay(0x64);
        BoardModelMotionShiftSet(work->model, 5, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    }
    if (arg0 == 3) {
        battleTimer = 0;
        attackTimer = 0;
    }
}

/**
 * @brief Read Boo's encounter state.
 * @return The current BallBooWork state number.
 */
static s32 GetBallBooState(void) {
    BallBooWork *work = omObjGetWork(ballBooObj, BallBooWork);

    return work->state;
}

/**
 * @brief Dispatch Boo's encounter state and synchronize its model transform.
 * @param arg0 Boo object containing movement deltas and BallBooWork.
 */
static void ExecBallBoo(OMOBJ *arg0) {
    BallBooWork *work = omObjGetWork(arg0, BallBooWork);

    if (booKillF != 0 || BoardIsKill()) {
        BoardModelKill(work->model);
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    if (work->delay != 0) {
        work->delay--;
        return;
    }
    switch (work->state) {
        case 1:
            BallBooSpawn(arg0, work);
            break;
        case 2:
        case 4:
        case 6:
            BallBooChase(arg0, work);
            break;
        case 3:
            BallBooBattle(arg0, work);
            break;
        case 5:
            BallBooAttack(arg0, work);
            break;
        case 7:
            BallBooFlash(arg0, work);
            break;
    }
    BoardModelRotYSet(work->model, arg0->scale.y);
    BoardModelPosSet(work->model, arg0->trans.x, arg0->trans.y, arg0->trans.z);
    BoardModelScaleSet(work->model, arg0->scale.z, arg0->scale.z, arg0->scale.z);
}

/**
 * @brief Make Boo fully opaque and mark its initial spawn complete.
 * @param arg0 Boo object retained by the state-handler interface.
 * @param arg1 Boo work containing the model handle.
 */
static void BallBooSpawn(OMOBJ *arg0, BallBooWork *arg1) {
    BoardModelAlphaSet(arg1->model, 0xFF);
    SetBallBooState(0);
}

/**
 * @brief Move Boo along its precomputed per-frame approach vector.
 * @param arg0 Boo object containing the position and movement delta.
 * @param arg1 Boo work containing the remaining frames and destination state.
 */
static void BallBooChase(OMOBJ *arg0, BallBooWork *arg1) {
    if (arg1->timer != 0) {
        arg1->timer--;
        arg0->trans.x += arg0->rot.x;
        arg0->trans.y += arg0->rot.y;
        arg0->trans.z += arg0->rot.z;
    } else {
        if (arg1->state == 4) {
            SetBallBooState(5);
        } else {
            SetBallBooState(0);
        }
        BoardModelMotionShiftSet(arg1->model, 1, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    }
}

/**
 * @brief Animate Boo attacks during the 120-frame coin struggle.
 *
 * @details Random attacks can trigger the selected character's reaction sound and controller vibration.
 * @param arg0 Unused Boo object retained by the state-handler interface.
 * @param arg1 Boo work containing the attack-motion flag and model.
 */
static void BallBooBattle(OMOBJ *arg0, BallBooWork *arg1) {
    s32 character;

    battleTimer++;
    if (attackTimer != 0) {
        attackTimer--;
    }
    if (battleTimer >= 120) {
        SetBallBooState(4);
        return;
    }
    if (arg1->motion_active != 0) {
        if (BoardModelMotionEndCheck(arg1->model)) {
            arg1->motion_active = 0;
            BoardModelMotionStart(arg1->model, 1, 0x40000001);
        }
        return;
    }
    if (BoardRandMod(100) <= 70) {
        BoardModelMotionStart(arg1->model, 2, 0);
        arg1->motion_active = 1;
        if (BoardRandMod(100) > 50 && attackTimer == 0) {
            character = GWPlayer[stealTarget].character;
            HuAudFXPlay(booSfxTbl[1][character]);
            omVibrate(stealTarget, 12, 6, 6);
            attackTimer = 48;
        }
    }
}

/**
 * @brief Scale Boo through its post-capture attack pulse.
 * @param arg0 Boo object whose scale is animated.
 * @param arg1 Boo work containing the attack angle timer.
 */
static void BallBooAttack(OMOBJ *arg0, BallBooWork *arg1) {
    float attack_angle;

    if (arg1->timer < 90) {
        arg1->timer += 6;
        if (arg1->timer >= 90) {
            arg1->timer = 90;
            SetBallBooState(0);
            SetBallPlayerState(5);
        }
    }
    OSu8tof32(&arg1->timer, &attack_angle);
    arg0->scale.z = 1.0 + sind(attack_angle);
}

/**
 * @brief Rotate Boo toward its light-repelled retreat path.
 * @param arg0 Boo object whose heading and movement delta are updated.
 * @param arg1 Boo work containing the target angle, model, and retreat timer.
 */
static void BallBooFlash(OMOBJ *arg0, BallBooWork *arg1) {
    Vec target_pos;
    float player_angle;

    if (BoardDAngleCalcRange(&arg0->scale.y, arg1->target_angle, 8.0f) != 0) {
        SetBallBooState(6);
        BoardModelMotionShiftSet(arg1->model, 3, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
        player_angle = BoardPlayerRotYGet(stealTarget);
        BoardPlayerPosGet(stealTarget, &target_pos);
        target_pos.x += 500.0 * sind(-player_angle);
        target_pos.y = target_pos.y + 160.0f;
        target_pos.z += 500.0 * cosd(-player_angle);
        arg0->rot.x = (target_pos.x - arg0->trans.x) / 72.0f;
        arg0->rot.y = (target_pos.y - arg0->trans.y) / 72.0f;
        arg0->rot.z = (target_pos.z - arg0->trans.z) / 72.0f;
        arg1->timer = 72;
    }
}

/**
 * @brief Create ten coin models scattered from the target player.
 *
 * @details Each entry stores its model, position, gravity velocity, and lateral drift for ExecTakeBallCoin.
 */
static void TakeBallCoin(void) {
    BallTakeCoinData *coin_data;
    Vec player_pos;
    s32 i;

    ballTakeCoinObj = omAddObjEx(boardObjMan, 0x1002, 0, 0, -1, ExecTakeBallCoin);
    ballTakeCoinObj->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(BallTakeCoinData) * 10, HU_MEMNUM_OVL);
    memset(ballTakeCoinObj->data, 0, sizeof(BallTakeCoinData) * 10);
    BoardPlayerPosGet(stealTarget, &player_pos);
    ballTakeCoinObj->trans.x = player_pos.x;
    ballTakeCoinObj->trans.y = player_pos.y;
    ballTakeCoinObj->trans.z = player_pos.z;
    coin_data = ballTakeCoinObj->data;
    for (i = 0; i < 10; i++, coin_data++) {
        coin_data->model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 10), NULL, 1);
        BoardModelCameraSet(coin_data->model, 1);
        fn_8006DDE8(coin_data->model, -1.0f);
        BoardModelScaleSet(coin_data->model, 0.5f, 0.5f, 0.5f);
        coin_data->position.x = player_pos.x;
        coin_data->position.y = player_pos.y + 100.0f;
        coin_data->position.z = player_pos.z;
        coin_data->drift_velocity.x = 3.0f * (BoardRandFloat() - 0.5f);
        coin_data->drift_velocity.y = 3.0 * sin(BoardRandFloat());
        coin_data->drift_velocity.z = 3.0f * (BoardRandFloat() - 0.5f);
        VECScale(&coin_data->drift_velocity, &coin_data->drift_velocity, 4.0f);
        BoardModelPosSetV(coin_data->model, &coin_data->position);
        coin_data->gravity_velocity = coin_data->drift_velocity;
    }
}

/**
 * @brief Update or clean up the ten scattered coin pickups.
 * @param arg0 Coin-effect object owning the BallTakeCoinData array.
 */
static void ExecTakeBallCoin(OMOBJ *arg0) {
    BallTakeCoinData *coin_data;
    Vec player_pos;
    s32 i;

    if (CheckBallCoinDone() || booKillF != 0 || BoardIsKill()) {
        if (arg0->data) {
            HuMemDirectFree(arg0->data);
            arg0->data = NULL;
        }
        ballTakeCoinObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    player_pos.x = arg0->trans.x;
    player_pos.y = arg0->trans.y;
    player_pos.z = arg0->trans.z;
    coin_data = arg0->data;
    for (i = 0; i < 10; i++, coin_data++) {
        if (coin_data->model != -1) {
            TakeBallCoinPosSet(coin_data, &player_pos);
        }
    }
}

/**
 * @brief Integrate one scattered coin's position and gravity.
 *
 * @details Kills the model once it falls more than 50 world units below the reference.
 * @param arg0 Coin entry containing its model and motion vectors.
 * @param arg1 Reference player position used as the vertical cleanup threshold.
 */
static void TakeBallCoinPosSet(BallTakeCoinData *arg0, Vec *arg1) {
    if (arg1->y > arg0->position.y + 50.0f) {
        BoardModelKill(arg0->model);
        arg0->model = -1;
        return;
    }
    VECAdd(&arg0->position, &arg0->gravity_velocity, &arg0->position);
    VECAdd(&arg0->position, &arg0->drift_velocity, &arg0->position);
    arg0->gravity_velocity.y += -0.98f;
    BoardModelPosSetV(arg0->model, &arg0->position);
}

/**
 * @brief Test whether every scattered coin model has been removed.
 * @return TRUE when the effect object is absent or no entry has a live model; otherwise FALSE.
 */
static BOOL CheckBallCoinDone(void) {
    BallTakeCoinData *coin_data;
    s32 i;

    if (!ballTakeCoinObj) {
        return TRUE;
    }
    coin_data = ballTakeCoinObj->data;
    for (i = 0; i < 10; i++, coin_data++) {
        if (coin_data->model > 0) {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * @brief Create the stolen-star model and its launch trajectory.
 *
 * @details The star begins 150 world units above the target and follows a spinning ballistic arc.
 */
static void TakeBallStar(void) {
    BallTakeCoinWork *work;
    Vec player_pos;

    ballTakeCoinObj = omAddObjEx(boardObjMan, 0x1003, 0, 0, -1, ExecTakeBallStar);
    work = omObjGetWork(ballTakeCoinObj, BallTakeCoinWork);
    work->kill = 0;
    work->elapsed_frames = 0;
    work->model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 11), 0, 0);
    BoardPlayerPosGet(stealTarget, &player_pos);
    ballTakeCoinObj->trans.x = player_pos.x;
    ballTakeCoinObj->trans.y = player_pos.y + 150.0f;
    ballTakeCoinObj->trans.z = player_pos.z;
    work->floor_y = player_pos.y;
    ballTakeCoinObj->rot.x = 3.0 * sind(80);
    ballTakeCoinObj->rot.z = 3.0 * cosd(80);
    ballTakeCoinObj->rot.y = 0.0f;
    BoardModelPosSet(work->model, ballTakeCoinObj->trans.x, ballTakeCoinObj->trans.y, ballTakeCoinObj->trans.z);
    BoardModelVisibilitySet(work->model, 1);
    BoardModelCameraSet(work->model, 1);
    fn_8006DDE8(work->model, -1.0f);
    BoardModelScaleSet(work->model, 0.5f, 0.5f, 0.5f);
    HuAudFXPlay(0x361);
}

/**
 * @brief Advance and clean up the stolen-star ballistic effect.
 * @param arg0 Star-effect object containing its trajectory and BallTakeCoinWork.
 */
static void ExecTakeBallStar(OMOBJ *arg0) {
    BallTakeCoinWork *work = omObjGetWork(arg0, BallTakeCoinWork);
    Vec player_pos;
    float elapsed_frames;

    if (work->kill != 0) {
        if (arg0->data) {
            HuMemDirectFree(arg0->data);
            arg0->data = NULL;
        }
        BoardModelKill(work->model);
        ballTakeCoinObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    BoardPlayerPosGet(stealTarget, &player_pos);
    work->elapsed_frames++;
    OSu8tof32(&work->elapsed_frames, &elapsed_frames);
    arg0->trans.x += arg0->rot.x;
    arg0->trans.z += arg0->rot.z;
    arg0->trans.y += 20.0f + elapsed_frames * elapsed_frames * -0.08166667f * 0.3f;
    arg0->rot.y = BoardDAngleCalc(arg0->rot.y + 24.0f);
    if (work->floor_y > arg0->trans.y) {
        work->kill = 1;
    }
    BoardModelPosSet(work->model, arg0->trans.x, arg0->trans.y, arg0->trans.z);
    BoardModelRotYSet(work->model, arg0->rot.y);
}

/**
 * @brief Test whether the stolen-star effect object has finished.
 * @return TRUE after the star object has deleted itself; otherwise FALSE.
 */
static BOOL CheckTakeBallStarDone(void) {
    if (ballTakeCoinObj) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/**
 * @brief Return the resolved coin-theft amount and punch reduction.
 * @param arg0 Receives the number of coins removed from the target.
 * @param arg1 Receives the reduction tier derived from the target's punch count.
 * @return Zero when a repellent light prevented theft; one when the output values are valid.
 */
s32 BoardBooStealValueGet(s16 *arg0, s16 *arg1) {
    if (stealType == 2) {
        return 0;
    }
    *arg0 = stealValue;
    *arg1 = stealSuccess;
    return 1;
}

/**
 * @brief Report whether the encounter completed without a repellent light.
 * @return TRUE for coin or star theft; FALSE when the target consumed a light item.
 */
BOOL BoardBooStealLightCheck(void) {
    if (stealType == 2) {
        return FALSE;
    } else {
        return TRUE;
    }
}

static s8 stealSuccessTbl[][3] = {
    {  0,  0, 0 },
    {  1,  6, 1 },
    {  7,  7, 3 },
    {  8, 12, 2 },
    { 13, 15, 3 },
    { 16, 18, 4 },
    { 19, 20, 6 },
    { 21, 23, 5 },
    { 24, 26, 6 },
    { 27, 28, 7 },
    { 29, 29, 8 },
    { 30, 99, 9 }
};

static s8 stealTbl[][3] = {
    {  1,  5, 15 },
    {  6, 15, 25 },
    { 16, 25, 27 },
    { 26, 35, 30 },
    { 36, 45, 32 },
    { 46, 50, 35 }
};

/**
 * @brief Compute and remove the coins stolen after the punch struggle.
 *
 * @details Caps punchCount and the turn at 99 and 50, maps them through the success and base tables, and never removes more coins than the target owns.
 */
static void UpdatePlayerCoins(void) {
    s32 original_punch_count;
    s32 base_steal_value;
    s32 turn;
    s32 i;

    original_punch_count = punchCount;
    if (punchCount > 99) {
        punchCount = 99;
    }
    turn = GWSystem.turn;
    if (turn > 50) {
        turn = 50;
    }
    for (stealSuccess = i = 0; i < 12; i++) {
        if (punchCount >= stealSuccessTbl[i][0] && punchCount <= stealSuccessTbl[i][1]) {
            stealSuccess = stealSuccessTbl[i][2];
            break;
        }
    }
    for (base_steal_value = i = 0; i < 6; i++) {
        if (turn >= stealTbl[i][0] && turn <= stealTbl[i][1]) {
            base_steal_value = stealTbl[i][2];
            break;
        }
    }
    stealValue = base_steal_value - stealSuccess;
    if (BoardPlayerCoinsGet(stealTarget) < stealValue) {
        stealValue = BoardPlayerCoinsGet(stealTarget);
    }
    BoardPlayerCoinsAdd(stealTarget, -stealValue);
}
