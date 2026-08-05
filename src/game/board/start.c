/**
 * @file start.c
 * @brief Implements board-opening setup, turn-order dice rolls, player ordering, camera and host setup, and tutorial variants.
 * 
 * @details Creates the opening host and player objects, animates each
 * turn-order Dice Block, sorts players by their results, and supports the
 * shortened tutorial opening.
 */
#include "game/board/start.h"
#include "game/audio.h"
#include "game/gamework_data.h"
#include "game/flag.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include "game/board/audio.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/roll.h"
#include "game/board/space.h"
#include "game/board/star.h"
#include "game/board/tutorial.h"
#include "game/board/ui.h"
#include "game/board/window.h"

#include "dolphin.h"
#include "ext_math.h"
#include <string.h>

typedef struct {
    struct {
        u8 sequence_complete : 1; ///< Set after the result display finishes so the manager deletes this player object.
        u8 state : 3;
        u8 index : 2;
        u8 landing_complete : 1; ///< Set when PlayerFall reaches the board and consumes its landing delay.
        u8 has_rolled : 1;
    };
    u8 delay;
    u8 time;
    s8 unidentified_03;  // Written to -1 during creation; purpose not identified.
    s8 digit;
    s16 roll_interval_or_rotation_angle; ///< DiceRoll update interval, then result-model rotation in degrees.
    s16 dice_model;
    s16 model;
    s16 dice_particle_manager_id;
    s8 tutorial_jump_countdown;
} PlayerStartWork;

static void DestroyStart(void);
static void ExecStart(void);
static void ShowLogo(void);
static void FocusStart(void);
static void CreatePlayerStart(void);
static void ExecPlayerStart(OMOBJ *object);
static void PlayerFall(OMOBJ *object, PlayerStartWork *data);
static void PlayerWaitSpeak(OMOBJ *unused_object, PlayerStartWork *data);
static void PlayerDiceFall(OMOBJ *object, PlayerStartWork *data);
static void PlayerDiceRoll(OMOBJ *object, PlayerStartWork *data);
static void PlayerDiceNumShow(OMOBJ *unused_object, PlayerStartWork *data);
static void PlayerDiceNumHide(OMOBJ *object, PlayerStartWork *data);
static void SetPlayerStartState(s32 player, s32 state);
static s32 GetPlayerStartState(s32 player);
static void ExecStartRoll(void);
static void InitCamera(void);
static void InitHost(void);
static void CreateTapWin(void);
static void OrderPlayers(void);

static Vec spacePos;
static Vec camStartFocusPos;

static s16 hostMdl;
static s16 startSpace;
static s8 playerOrderNew[4];
static s8 playerOrderOld[4];
static HUPROCESS *startProc;

static s16 logoSprGrp = -1;
static s16 logoSpr = -1;
static s16 camFocus = -1;
static s16 tapWin = -1;
static s32 streamStatus = -1;

static s8 tutorialRollTbl[] = {
    0x02, 0x08, 0x06, 0x00
};

static OMOBJ *playerStartObj[4] = {
    NULL, NULL, NULL, NULL
};

static s32 hitFX[4] = {
    -1, -1, -1, -1
};

static s32 startMesTbl[9][5] = {
    { MAKE_MESSID(0x15, 0x01), MAKE_MESSID(0x15, 0x07), MAKE_MESSID(0x15, 0x0D), MAKE_MESSID(0x15, 0x16), MAKE_MESSID(0x15, 0x1C) },
    { MAKE_MESSID(0x15, 0x02), MAKE_MESSID(0x15, 0x08), MAKE_MESSID(0x15, 0x0E), MAKE_MESSID(0x15, 0x17), MAKE_MESSID(0x15, 0x1D) },
    { MAKE_MESSID(0x15, 0x03), MAKE_MESSID(0x15, 0x09), MAKE_MESSID(0x15, 0x0F), MAKE_MESSID(0x15, 0x18), MAKE_MESSID(0x15, 0x1E) },
    { MAKE_MESSID(0x15, 0x04), MAKE_MESSID(0x15, 0x0A), MAKE_MESSID(0x15, 0x10), MAKE_MESSID(0x15, 0x19), MAKE_MESSID(0x15, 0x1F) },
    { MAKE_MESSID(0x15, 0x05), MAKE_MESSID(0x15, 0x0B), MAKE_MESSID(0x15, 0x11), MAKE_MESSID(0x15, 0x1A), MAKE_MESSID(0x15, 0x20) },
    { MAKE_MESSID(0x15, 0x06), MAKE_MESSID(0x15, 0x0C), MAKE_MESSID(0x15, 0x12), MAKE_MESSID(0x15, 0x1B), MAKE_MESSID(0x15, 0x21) },
    { MAKE_MESSID(0x15, 0x01), MAKE_MESSID(0x15, 0x07), MAKE_MESSID(0x15, 0x0D), MAKE_MESSID(0x15, 0x16), MAKE_MESSID(0x15, 0x1C) },
    { MAKE_MESSID(0x15, 0x33), MAKE_MESSID(0x15, 0x34), MAKE_MESSID(0x15, 0x35), MAKE_MESSID(0x15, 0x36), MAKE_MESSID(0x15, 0x37) },
    { MAKE_MESSID(0x15, 0x3B), MAKE_MESSID(0x15, 0x3C), MAKE_MESSID(0x15, 0x3D), MAKE_MESSID(0x15, 0x3E), MAKE_MESSID(0x15, 0x3F) }
};

static s32 logoSprTbl[] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 0x57), DATA_MAKE_NUM(DATADIR_BOARD, 0x58), DATA_MAKE_NUM(DATADIR_BOARD, 0x59),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x5A), DATA_MAKE_NUM(DATADIR_BOARD, 0x5B), DATA_MAKE_NUM(DATADIR_BOARD, 0x5C),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x57), DATA_MAKE_NUM(DATADIR_BOARD, 0x5D), DATA_MAKE_NUM(DATADIR_BOARD, 0x5E)
};

/**
 * @brief Run the board-opening presentation and determine player order.
 */
void BoardStartExec(void) {
    startProc = HuPrcChildCreate(ExecStart, 0x2007, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(startProc, DestroyStart);
    while (startProc != NULL) {
        HuPrcVSleep();
    }
}

/**
 * @brief Release opening-sequence models, windows, sprites, and process state.
 */
static void DestroyStart(void) {
    if (tapWin != -1) {
        HuWinKill(tapWin);
        tapWin = -1;
    }
    if (logoSprGrp != -1) {
        HuSprGrpKill(logoSprGrp);
        logoSprGrp = -1;
    }
    if (camFocus != -1) {
        BoardModelKill(camFocus);
        camFocus = -1;
    }
    BoardCameraNearFarSet(100.0f, 13000.0f);
    startProc = NULL;
}

/**
 * @brief Run the logo, host introduction, dice rolls, ordering, and transition into play.
 */
static void ExecStart(void) {
    Vec pos;
    s32 i;

    BoardCameraNearFarSet(100.0f, 23000.0f);
    GWSystem.player_curr = -1;
    startSpace = BoardSpaceFlagPosGet(0, 0x80000000, &spacePos);
    if ((GWBoardGet() == BOARD_ID_MAIN3 || GWBoardGet() == BOARD_ID_MAIN6) && boardLightResetHook) {
        boardLightResetHook();
    }
    for (i = 0; i < 4; i++) {
        BoardModelVisibilitySet(BoardPlayerModelGet(i), 0);
    }
    InitCamera();
    CreateTapWin();
    InitHost();
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 21);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    _ClearFlag(FLAG_ID_MAKE(1, 28));
    if (!_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        streamStatus = HuAudSStreamPlay(5);
        ShowLogo();
        FocusStart();
        if ((GWBoardGet() == BOARD_ID_MAIN3 || GWBoardGet() == BOARD_ID_MAIN6) && boardLightSetHook) {
            boardLightSetHook();
        }
        BoardCameraMotionWait();
    } else {
        BoardMusStart(0, 0x28, 0x7F, 0);
    }
    CreatePlayerStart();
    ExecStartRoll();
    if (!_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        BoardAudSeqFadeOut(0, 0x1F4);
    }
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        BoardTutorialHookExec(4, 0);
        WipeColorSet(0, 0, 0);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 21);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        for (i = 0; i < 4; i++) {
            BoardPlayerCornerPosSet(i);
        }
        BoardSpacePosGet(0, GWPlayer->space_curr, &pos);
        BoardPlayerPosSetV(0, &pos);
        BoardCameraMoveSet(0);
        BoardCameraViewSet(2);
        BoardCameraMotionWait();
        BoardCameraTargetPlayerSet(0);
        BoardCameraMotionWait();
        BoardCameraMoveSet(1);
        BoardModelVisibilitySet(BoardStarHostMdlGet(), 0);
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 21);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        HuPrcEnd();
    }
    BoardCameraNearFarSet(100.0f, 13000.0f);
    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        WipeColorSet(0, 0, 0);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 21);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        GWSystem.player_curr = 0;
        BoardModelVisibilitySet(hostMdl, 0);
        BoardStatusItemSet(1);
        BoardPlayerCornerPosSet(0);
        BoardPlayerCornerPosSet(1);
        BoardPlayerCornerPosSet(2);
        BoardPlayerCornerPosSet(3);
        BoardPlayerPosSetV(0, &spacePos);
        BoardCameraMoveSet(0);
        BoardCameraViewSet(2);
        BoardCameraMotionWait();
        BoardCameraMoveSet(1);
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 21);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
    } else {
        BoardStarShowNext(-1);
    }
    HuPrcEnd();
}

/**
 * @brief Display and animate the board-start logo.
 */
static void ShowLogo(void) {
    float scale;
    float angle_degrees;
    s16 angle_step;
    s16 pulse_angle;

    logoSprGrp = HuSprGrpCreate(1);
    BoardSpriteCreate(logoSprTbl[GWBoardGet()], 1000, 0, &logoSpr);
    HuSprGrpMemberSet(logoSprGrp, 0, logoSpr);
    HuSprAttrSet(logoSprGrp, 0, 1);
    HuSprAttrSet(logoSprGrp, 0, 8);
    HuSprScaleSet(logoSprGrp, 0, 0.01f, 0.01f);
    HuSprGrpPosSet(logoSprGrp, 288.0f, 240.0f);
    for (angle_step = 0; angle_step < 90; angle_step += 4) {
        OSs16tof32(&angle_step, &angle_degrees);
        scale = sind(angle_degrees);
        HuSprScaleSet(logoSprGrp, 0, scale, scale);
        HuPrcVSleep();
    }
    HuSprScaleSet(logoSprGrp, 0, 1.0f, 1.0f);
    for (angle_step = 0; angle_step < 540; angle_step += 4) {
        pulse_angle = angle_step % 180;
        OSs16tof32(&pulse_angle, &angle_degrees);
        scale = 1.0 + 0.7f * sind(angle_degrees);
        HuSprScaleSet(logoSprGrp, 0, scale, scale);
        HuPrcVSleep();
    }
    HuSprScaleSet(logoSprGrp, 0, 1.0f, 1.0f);
    HuPrcSleep(0x78);
    for (angle_step = 0; angle_step < 90; angle_step += 4) {
        OSs16tof32(&angle_step, &angle_degrees);
        scale = cosd(angle_degrees);
        HuSprScaleSet(logoSprGrp, 0, scale, scale);
        HuPrcVSleep();
    }
    HuSprGrpKill(logoSprGrp);
    logoSprGrp = -1;
}

/**
 * @brief Move the camera from the establishing view to the waiting players.
 */
static void FocusStart(void) {
    Vec focus_position;
    Vec offset;
    Vec offset2;
    Vec camera_rotation;
    float zoom;
    u32 i;

    for (zoom = 14000.0f; zoom >= 2000.0f; zoom -= 50.0f) {
        BoardCameraZoomSet(zoom);
        HuPrcVSleep();
    }
    BoardCameraZoomSet(2000.0f);
    BoardModelPosGet(camFocus, &focus_position);
    offset.x = spacePos.x + 150.0f;
    offset.y = spacePos.y + 100.0f;
    offset.z = spacePos.z + 150.0f;
    VECSubtract(&offset, &focus_position, &offset2);
    offset2.x /= 120.0f;
    offset2.y /= 120.0f;
    offset2.z /= 120.0f;
    HuAudSStreamFadeOut(streamStatus, 0x1388);
    for (i = 0; i < 120; i++) {
        VECAdd(&offset2, &focus_position, &focus_position);
        BoardModelPosSetV(camFocus, &focus_position);
        HuPrcVSleep();
    }
    BoardModelPosSetV(camFocus, &offset);
    camera_rotation.x = -22.0f;
    camera_rotation.y = 0.0f;
    camera_rotation.z = 0.0f;
    BoardCameraMotionStartEx(camFocus, &camera_rotation, NULL, 1500.0f, -1.0f, 21);
}

/**
 * @brief Create per-player objects and work records for the opening roll sequence.
 */
static void CreatePlayerStart(void) {
    Vec focus_position;
    float player_x_offset;
    OMOBJ *player_object;
    PlayerStartWork *player_data;
    s8 i;

    BoardModelPosGet(camFocus, &focus_position);
    for (i = 0; i < 4; i++) {
        playerOrderOld[i] = i;
        player_object = omAddObjEx(boardObjMan, 0x100, 0, 0, -1, ExecPlayerStart);
        playerStartObj[i] = player_object;
        player_data = omObjGetWork(player_object, PlayerStartWork);
        player_data->sequence_complete = 0;
        player_data->index = i;
        player_data->unidentified_03 = -1;
        player_data->model = -1;
        player_data->dice_particle_manager_id = -1;
        player_data->state = 1;
        player_data->delay = i * 30;
        player_data->landing_complete = 0;
        player_data->has_rolled = 0;
        player_data->roll_interval_or_rotation_angle = 2;
        player_data->digit = 1;
        player_data->tutorial_jump_countdown = 0x3C;
        player_data->dice_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x18), NULL, 0);
        BoardModelVisibilitySet(player_data->dice_model, 0);
        BoardModelMotionSpeedSet(player_data->dice_model, 0.0f);
        OSs8tof32(&i, &player_x_offset);
        player_x_offset = 135.0f * (player_x_offset - 2.0f + 0.5f);
        BoardModelVisibilitySet(BoardPlayerModelGet(playerOrderOld[player_data->index]), 1);
        player_object->trans.x = focus_position.x + player_x_offset;
        player_object->trans.y = focus_position.y + 700.0f;
        player_object->trans.z = focus_position.z + 100.0f;
        BoardPlayerPosSet(playerOrderOld[player_data->index], player_object->trans.x, player_object->trans.y, player_object->trans.z);
        BoardPlayerMotionShiftSet(playerOrderOld[player_data->index], 4, 10.0f, 1.0f, HU3D_MOTATTR_NONE);
    }
    if (!_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        BoardMusStart(0, 0xC, 0x7F, 0);
    }
    while (GetPlayerStartState(3) != 0) {
        HuPrcVSleep();
    }
}

/**
 * @brief Dispatch one player's opening-roll state machine.
 * @param [in] object Per-player opening object that owns the dispatched PlayerStartWork record.
 */
static void ExecPlayerStart(OMOBJ *object) {
    PlayerStartWork *data = omObjGetWork(object, PlayerStartWork);

    if (data->sequence_complete != 0 || BoardIsKill()) {
        if (hitFX[data->index] != -1) {
            HuAudFXStop(hitFX[data->index]);
            hitFX[data->index] = -1;
        }
        playerStartObj[data->index] = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    if (data->delay != 0) {
        data->delay--;
        return;
    }
    switch (data->state) {
        case 0:
            break;
        case 1:
            PlayerFall(object, data);
            break;
        case 2:
            PlayerWaitSpeak(object, data);
            break;
        case 3:
            PlayerDiceFall(object, data);
            break;
        case 4:
            PlayerDiceRoll(object, data);
            break;
        case 5:
            PlayerDiceNumShow(object, data);
            break;
        case 6:
            PlayerDiceNumHide(object, data);
            break;
    }
}

/**
 * @brief Animate a player falling into the opening lineup.
 * @param [in,out] object Per-player opening object whose Y translation tracks the falling player.
 * @param [in,out] data Player record whose index selects the model, whose time and delay advance the fall, and whose landing_complete flag records touchdown.
 */
static void PlayerFall(OMOBJ *object, PlayerStartWork *data) {
    float fall_duration;

    if (data->landing_complete != 0) {
        SetPlayerStartState(playerOrderOld[data->index], 0);
        BoardPlayerIdleSet(playerOrderOld[data->index]);
        if (_CheckFlag(FLAG_ID_MAKE(1, 11)) && data->index == 3) {
            BoardTutorialHookExec(0, 0);
        }
        return;
    }
    if (data->time > 200) {
        data->time = -56;
    }
    OSu8tof32(&data->time, &fall_duration);
    object->trans.y += -0.08166667f * fall_duration * fall_duration;
    if (object->trans.y < spacePos.y) {
        object->trans.y = spacePos.y;
        data->time = 0;
        BoardPlayerMotionShiftSet(playerOrderOld[data->index], 5, 0.0f, 4.0f, HU3D_MOTATTR_NONE);
        data->delay = 0xC;
        data->landing_complete = 1;
    }
    data->time++;
    BoardPlayerPosSet(playerOrderOld[data->index], object->trans.x, object->trans.y, object->trans.z);
}

/**
 * @brief Play the waiting player's voice and advance to the dice-drop state.
 * @param [in] unused_object Unused object callback argument.
 * @param [in,out] data Player record whose index selects the speaker and whose time gates the wait before advancing state.
 */
static void PlayerWaitSpeak(OMOBJ *unused_object, PlayerStartWork *data) {
    float target_y_rotation;

    if (data->time == 0) {
        target_y_rotation = 180.0f + BoardPlayerRotYGet(playerOrderOld[data->index]);
        BoardPlayerMotBlendSet(playerOrderOld[data->index], target_y_rotation, 0xF);
        data->time = 1;
        return;
    }
    if (BoardPlayerMotBlendCheck(playerOrderOld[data->index])) {
        BoardPlayerIdleSet(playerOrderOld[data->index]);
        SetPlayerStartState(playerOrderOld[data->index], 0);
    }
}

/**
 * @brief Animate the opening-order Dice Block falling above a player.
 * @param [in,out] object Per-player opening object whose X rotation stores the Dice Block's per-frame fall step.
 * @param [in,out] data Player record whose index and dice_model select the falling die and whose time advances the fall and state transition.
 */
static void PlayerDiceFall(OMOBJ *object, PlayerStartWork *data) {
    Vec dice_position;
    float max;
    float min;
    s32 i;

    if (data->time == 0) {
        HuAudFXPlay(0x302);
        BoardPlayerPosGet(playerOrderOld[data->index], &dice_position);
        min = 250.0f;
        max = 700.0f;
        object->rot.x = (min - max) / 30.0f;
        dice_position.y += max;
        BoardModelPosSetV(data->dice_model, &dice_position);
        BoardModelVisibilitySet(data->dice_model, 1);
    } else {
        if (data->time > 30.0f) {
            if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
                if (data->index == 3) {
                    BoardTutorialHookExec(1, 0);
                    for (i = 0; i < 4; i++) {
                        SetPlayerStartState(playerOrderOld[i], 4);
                    }
                }
                return;
            }
            SetPlayerStartState(playerOrderOld[data->index], 4);
            return;
        }
        BoardModelPosGet(data->dice_model, &dice_position);
        dice_position.y += object->rot.x;
        BoardModelPosSetV(data->dice_model, &dice_position);
    }
    data->time++;
}

static s32 digitMdlTbl[9] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 0x0D), DATA_MAKE_NUM(DATADIR_BOARD, 0x0E), DATA_MAKE_NUM(DATADIR_BOARD, 0x0F),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x10), DATA_MAKE_NUM(DATADIR_BOARD, 0x11), DATA_MAKE_NUM(DATADIR_BOARD, 0x12),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x13), DATA_MAKE_NUM(DATADIR_BOARD, 0x14), DATA_MAKE_NUM(DATADIR_BOARD, 0x15)
};

static s32 playerOrderMesTbl[3] = {
    MAKE_MESSID(0x15, 0x13),
    MAKE_MESSID(0x15, 0x14),
    MAKE_MESSID(0x15, 0x15)
};

/**
 * @brief Spin the Dice Block until the player or computer confirms a result.
 * @param [in,out] object Per-player opening object whose rotation fields later hold result-model scale factors.
 * @param [in,out] data Player record whose time, roll interval, digit, model handles, particle manager, roll flag, and tutorial countdown drive the dice roll.
 */
static void PlayerDiceRoll(OMOBJ *object, PlayerStartWork *data) {
    Vec dice_position;
    float time;
    u32 jump_input;
    s8 player;
    s8 digit;

    jump_input = 0;
    if (data->time > data->roll_interval_or_rotation_angle) {
        data->time = 0;
        digit = data->digit;
        do {
            data->digit = BoardRandMod(9);
        } while (data->digit == digit);
        OSs8tof32(&data->digit, &time);
        BoardModelMotionTimeSet(data->dice_model, time + 0.5f);
    }
    if (data->has_rolled == 0) {
        if (GWPlayer[playerOrderOld[data->index]].com) {
            if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
                if (data->tutorial_jump_countdown != 0) {
                    data->tutorial_jump_countdown--;
                } else {
                    jump_input = PAD_BUTTON_A;
                }
            } else if (BoardRandMod(100) < 5) {
                jump_input = PAD_BUTTON_A;
            }
        } else {
            jump_input = HuPadBtnDown[GWPlayer[playerOrderOld[data->index]].port];
        }
        if (jump_input == PAD_BUTTON_A) {
            BoardPlayerDiceJumpStart(playerOrderOld[data->index]);
            data->has_rolled = 1;
        }
    } else {
        if (BoardPlayerDiceJumpCheck(playerOrderOld[data->index])) {
            SetPlayerStartState(playerOrderOld[data->index], 5);
            data->roll_interval_or_rotation_angle = 360;
            object->rot.x = object->rot.y = 1.0f;
            data->digit = BoardRandMod(9);
            HuAudFXPlay(0x303);
            if (hitFX[data->index] != -1) {
                HuAudFXStop(hitFX[data->index]);
                hitFX[data->index] = -1;
            }
            player = 0;
            while (player < 4) {
                if (playerOrderNew[player] == data->digit && player != playerOrderOld[data->index]) {
                    data->digit = BoardRandMod(9);
                    player = 0;
                } else {
                    player++;
                }
            }
            if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
                data->digit = tutorialRollTbl[data->index];
            }
            playerOrderNew[playerOrderOld[data->index]] = data->digit;
            data->model = BoardModelCreate(digitMdlTbl[data->digit], NULL, 0);
            BoardModelPosGet(data->dice_model, &dice_position);
            dice_position.x += 50.0 * sin(0.0);
            dice_position.z += 50.0 * cos(0.0);
            BoardModelPosSetV(data->model, &dice_position);
            BoardModelMotionStart(data->model, 0, 0);
            BoardModelMotionSpeedSet(data->model, 0.0f);
            BoardModelMotionTimeSet(data->model, 0.0f);
            BoardModelLayerSet(data->model, 2);
            data->dice_particle_manager_id = BoardDiceEffectCreate();
            Hu3DParManPosSet(data->dice_particle_manager_id, dice_position.x, dice_position.y, dice_position.z);
            Hu3DParManTimeLimitSet(data->dice_particle_manager_id, 0x96);
            BoardModelVisibilitySet(data->dice_model, 0);
            return;
        }
    }
    data->time++;
}

/**
 * @brief Display the player's opening roll and transition to result cleanup.
 * @param [in] unused_object Unused object callback argument.
 * @param [in,out] data Player record whose result model and index are displayed while its rotation angle is reduced toward zero before advancing state.
 */
static void PlayerDiceNumShow(OMOBJ *unused_object, PlayerStartWork *data) {
    float rotation_degrees;

    data->roll_interval_or_rotation_angle -= 18;
    if (data->roll_interval_or_rotation_angle < 0) {
        data->roll_interval_or_rotation_angle = 0;
        SetPlayerStartState(playerOrderOld[data->index], 0);
        if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
            BoardTutorialHookExec(2, 0);
        }
    }
    OSs16tof32(&data->roll_interval_or_rotation_angle, &rotation_degrees);
    BoardModelRotYSet(data->model, rotation_degrees);
}

/**
 * @brief Hide the opening roll, lower the player, and mark that player complete.
 * @param [in,out] object Per-player opening object whose rotation fields supply the result model's animated X/Y scale.
 * @param [in,out] data Player record whose time and rotation angle animate and hide the result model, then set sequence_complete.
 */
static void PlayerDiceNumHide(OMOBJ *object, PlayerStartWork *data) {
    float rotation_degrees;

    switch (data->time) {
        case 0:
            data->roll_interval_or_rotation_angle += 18;
            if (data->roll_interval_or_rotation_angle > 360) {
                data->roll_interval_or_rotation_angle = 0;
                data->time = 1;
                object->rot.x = 1.0f;
                object->rot.y = 1.0f;
            }
            OSs16tof32(&data->roll_interval_or_rotation_angle, &rotation_degrees);
            BoardModelRotYSet(data->model, rotation_degrees);
            return;
        case 1:
            data->roll_interval_or_rotation_angle += 9;
            if (data->roll_interval_or_rotation_angle > 180) {
                data->roll_interval_or_rotation_angle = 0;
                data->time = 2;
            }
            OSs16tof32(&data->roll_interval_or_rotation_angle, &rotation_degrees);
            object->rot.x = 1.0 + 1.3f * sind(rotation_degrees);
            break;
        case 2:
            data->roll_interval_or_rotation_angle += 9;
            if (data->roll_interval_or_rotation_angle > 180) {
                data->roll_interval_or_rotation_angle = 179;
                data->time = 3;
            }
            OSs16tof32(&data->roll_interval_or_rotation_angle, &rotation_degrees);
            object->rot.x = cosd(0.5f * rotation_degrees);
            object->rot.y = 1.0 + 1.3f * sind(rotation_degrees);
            break;
        case 3:
            BoardModelVisibilitySet(data->model, 0);
            data->sequence_complete = 1;
            return;
    }
    BoardModelScaleSet(data->model, object->rot.x, object->rot.y, 1.0f);
}

/**
 * @brief Set one player's opening-roll state and reset its state timer.
 * @param [in] player Player whose opening-roll state is changed and whose state timer is reset.
 * @param [in] state Next PlayerStartWork state dispatched by ExecPlayerStart.
 */
static void SetPlayerStartState(s32 player, s32 state) {
    PlayerStartWork *data;
    OMOBJ *object;

    object = playerStartObj[player];
    data = omObjGetWork(object, PlayerStartWork);
    data->state = state;
    data->time = 0;
    data->delay = 0;
    switch (state) {
        case 4:
            hitFX[player] = HuAudFXPlay(0x300);
            break;
        case 5:
            break;
        case 6:
            data->roll_interval_or_rotation_angle = 0;
            break;
    }
}

/**
 * @brief Read one player's current opening-roll state.
 * @param [in] player Player whose current opening-roll state is read.
 * @return Current state from the player's opening-sequence work record.
 */
static s32 GetPlayerStartState(s32 player) {
    PlayerStartWork *data;
    OMOBJ *object;

    object = playerStartObj[player];
    data = omObjGetWork(object, PlayerStartWork);
    return data->state;
}

/**
 * @brief Run all four opening Dice Block rolls and wait for completion.
 */
static void ExecStartRoll(void) {
    s32 unused_roll_index_table[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    s32 id;
    s32 time;
    s32 starting_coin_count;
    s32 j;
    s32 i;

    if (!_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        for (i = 0; i < 4; i++) {
            SetPlayerStartState(i, 2);
        }
        while (GetPlayerStartState(3) != 0) {
            HuPrcVSleep();
        }
        _CheckFlag(FLAG_ID_MAKE(1, 11));
        id = startMesTbl[GWBoardGet()][0];
        BoardWinCreate(2, id, BoardWinPortraitGetStar());
        BoardWinWait();
        id = startMesTbl[GWBoardGet()][1];
        BoardWinCreate(2, id, BoardWinPortraitGetStar());
        BoardWinWait();
        BoardWinKill();
        for (i = 0; i < 4; i++) {
            SetPlayerStartState(i, 2);
        }
        while (GetPlayerStartState(3) != 0) {
            HuPrcVSleep();
        }
    }
    for (i = 0; i < 4; i++) {
        SetPlayerStartState(i, 3);
    }
    if (tapWin != -1) {
        HuWinDispOn(tapWin);
    }
    while (1) {
        if (GetPlayerStartState(0) == 0 && GetPlayerStartState(1) == 0
            && GetPlayerStartState(2) == 0 && GetPlayerStartState(3) == 0) {
            break;
        }
        HuPrcVSleep();
    }
    if (tapWin != -1) {
        HuWinDispOff(tapWin);
    }
    HuPrcSleep(30);
    HuPadRumbleAllStop();
    OrderPlayers();
    BoardStatusKill();
    BoardStatusCreate();
    BoardStatusItemSet(0);
    HuPrcVSleep();
    for (i = 0; i < 4; i++) {
        if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
            BoardTutorialHookExec(3, i);
        } else {
            if (i == 0) {
                id = startMesTbl[GWBoardGet()][2];
            } else {
                id = playerOrderMesTbl[i - 1];
            }
            BoardWinCreate(0, id, -1);
            BoardWinInsertMesSet(BoardPlayerGetCharMess(i), i);
            BoardWinWait();
        }
        BoardStatusShowSet(i, 1);
        SetPlayerStartState(playerOrderOld[i], 6);
        BoardPlayerMotionShiftSet(i, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
        HuPrcSleep(8);
        while (!BoardPlayerMotionEndCheck(i)) {
            HuPrcVSleep();
        }
    }
    BoardWinKill();
    for (i = 0; i < 4; i++) {
        BoardPlayerIdleSet(i);
    }
    if (!_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        id = startMesTbl[GWBoardGet()][3];
        BoardWinCreate(2, id, BoardWinPortraitGetStar());
        BoardWinWait();
        HuPrcSleep(30);
        if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
            starting_coin_count = 100;
            time = 1;
        } else {
            starting_coin_count = 10;
            time = 6;
        }
        for (i = 0; i < starting_coin_count; i++) {
            for (j = 0; j < 4; j++) {
                BoardPlayerCoinsAdd(j, 1);
            }
            HuAudFXPlay(7);
            HuPrcSleep(time);
        }
        for (i = 0; i < 4; i++) {
            BoardPlayerMotionShiftSet(i, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
        }
        HuAudFXPlay(0xF);
        while (1) {
            if (BoardPlayerMotionEndCheck(0) && BoardPlayerMotionEndCheck(1)
                && BoardPlayerMotionEndCheck(2) && BoardPlayerMotionEndCheck(3)) {
                break;
            }
            HuPrcVSleep();
        }
        HuPrcSleep(60);
        for (i = 0; i < 4; i++) {
            BoardPlayerIdleSet(i);
        }
        id = startMesTbl[GWBoardGet()][4];
        BoardWinCreate(2, id, BoardWinPortraitGetStar());
        BoardWinWait();
        BoardWinKill();
    }
}

/**
 * @brief Configure the camera used by the opening board presentation.
 */
static void InitCamera(void) {
    Vec offset;
    Vec target_rotation;

    camStartFocusPos.x = 0.0f;
    camStartFocusPos.y = 100.0f;
    camStartFocusPos.z = 500.0f;
    switch (GWBoardGet()) {
        case BOARD_ID_MAIN1:
            camStartFocusPos.z -= 200.0f;
            camStartFocusPos.x -= 150.0f;
            break;
        case BOARD_ID_MAIN4:
            camStartFocusPos.z -= 250.0f;
            camStartFocusPos.x -= 150.0f;
            break;
        case BOARD_ID_MAIN5:
            camStartFocusPos.z -= 400.0f;
            camStartFocusPos.x -= 150.0f;
            break;
        case BOARD_ID_MAIN6:
            camStartFocusPos.z -= 500.0f;
            break;
        case BOARD_ID_EXTRA1:
            camStartFocusPos.z -= 520.0f;
            camStartFocusPos.x -= 150.0f;
            break;
        case BOARD_ID_EXTRA2:
            camStartFocusPos.z -= 250.0f;
            camStartFocusPos.x -= 150.0f;
            break;
    }
    camFocus = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x0A), NULL, 0);
    BoardModelVisibilitySet(camFocus, 0);
    BoardModelPosSetV(camFocus, &camStartFocusPos);
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        BoardCameraMoveSet(0);
        offset.x = spacePos.x + 150.0f;
        offset.y = spacePos.y + 100.0f;
        offset.z = spacePos.z + 150.0f;
        BoardModelPosSetV(camFocus, &offset);
        target_rotation.x = -22.0f;
        target_rotation.y = 0.0f;
        target_rotation.z = 0.0f;
        offset.x = offset.y = offset.z = 0.0f;
        BoardCameraMotionStartEx(camFocus, &target_rotation, NULL, 1500.0f, -1.0f, 1);
        BoardCameraMotionWait();
        BoardCameraMoveSet(1);
    } else {
        offset.x = 0.0f;
        offset.y = 0.0f;
        offset.z = 0.0f;
        target_rotation.x = -70.0f;
        target_rotation.y = 0.0f;
        target_rotation.z = 0.0f;
        BoardCameraMoveSet(0);
        BoardCameraMotionStartEx(camFocus, &target_rotation, &offset, 14000.0f, -1.0f, 1);
        BoardCameraMotionWait();
        BoardCameraMoveSet(1);
    }
}

/**
 * @brief Create, position, and animate the board host for the opening presentation.
 */
static void InitHost(void) {
    Vec host_position;

    if (!_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        hostMdl = BoardStarHostMdlGet();
        host_position.x = spacePos.x + 150.0f;
        host_position.y = spacePos.y;
        host_position.z = spacePos.z - 50.0f;
        BoardModelPosSetV(hostMdl, &host_position);
        BoardModelVisibilitySet(hostMdl, 1);
        BoardModelMotionStart(hostMdl, 1, 0x40000001);
    }
}

/**
 * @brief Create the prompt window used to advance the opening sequence.
 */
static void CreateTapWin(void) {
    float dimensions[2];
    float x;
    float y;
    s32 id;

    if (!_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        id = MAKE_MESSID(0x12, 0x14);
        HuWinMesMaxSizeGet(1, dimensions, id);
        x = -10000.0f;
        y = 368.0f;
        tapWin = HuWinCreate(x, y, dimensions[0], dimensions[1], 0);
        HuWinBGTPLvlSet(tapWin, 0.0f);
        HuWinMesSpeedSet(tapWin, 0);
        HuWinMesSet(tapWin, id);
        HuWinDispOff(tapWin);
    }
}

/**
 * @brief Sort opening roll results and assign each player a turn position.
 */
static void OrderPlayers(void) {
    s32 player_order_temp;
    PlayerConfig *player_config;
    PlayerState *state;
    s32 i;
    s32 j;

    player_config = HuMemDirectMallocNum(HEAP_HEAP, sizeof(GWPlayerCfg), HU_MEMNUM_OVL);
    state = HuMemDirectMallocNum(HEAP_HEAP, sizeof(GWPlayer), HU_MEMNUM_OVL);
    for (i = 0; i < 4; i++) {
        playerOrderOld[i] = i;
        memcpy(player_config + i, &GWPlayerCfg[i], sizeof(PlayerConfig));
        memcpy(state + i, &GWPlayer[i], sizeof(PlayerState));
    }
    for (i = 0; i < 3; i++) {
        for (j = 3; j > i; j--) {
            if (playerOrderNew[j - 1] < playerOrderNew[j]) {
                player_order_temp = playerOrderNew[j];
                playerOrderNew[j] = playerOrderNew[j - 1];
                playerOrderNew[j - 1] = player_order_temp;
                player_order_temp = playerOrderOld[j];
                playerOrderOld[j] = playerOrderOld[j - 1];
                playerOrderOld[j - 1] = player_order_temp;
            }
        }
    }
    for (i = 0; i < 4; i++) {
        memcpy(&GWPlayerCfg[i], player_config + playerOrderOld[i], sizeof(PlayerConfig));
        memcpy(&GWPlayer[i], state + playerOrderOld[i], sizeof(PlayerState));
        GWPlayer[i].port = GWPlayerCfg[i].pad_idx;
        GWPlayerCfg[i].pad_idx = GWPlayerCfg[i].pad_idx;
    }
    HuMemDirectFree(player_config);
    HuMemDirectFree(state);
}
