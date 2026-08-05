#include "game/board/view.h"
#include "game/audio.h"
#include "game/disp.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hsfex.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/ui.h"

#include "ext_math.h"

typedef struct {
    struct {
        u8 destroy_requested : 1;
    };
    u8 markers_visible;
    s16 sprite_group;
    s16 star_model;
} OverheadWork;

static void ExecMapView(void);
static void ExecStarView(void);
static void ExecOverheadView(void);
static void DestroyView(void);
static void SetTargetView(void);
static void DestroyFocus(void);
static void CreateButtonWin(void);
static void DestroyButtonWin(void);
static void InitOverheadView(void);
static void DestroyOverheadView(void);
static void UpdateOverheadView(OMOBJ *object);
static void WaitOverheadView(void);

static Vec focusPos;
static Vec focusPosTarget;

static s8 viewControls;
static s8 viewPlayer;
static s8 viewMoveF;
static s8 focusDestroyF;
static u16 focusTimer;
static OMOBJ *overheadObj;
static s8 pauseDisableF;
static HUPROCESS *viewProc;

static s16 buttonWin = -1;
static s16 focusMdl = -1;

static float overhead2DPos[][2] = {
    {  48.0f,  72.0f },
    { HU_DISP_WIDTH-48,  72.0f },
    {  48.0f, HU_DISP_HEIGHT-72 },
    { HU_DISP_WIDTH-48, HU_DISP_HEIGHT-72 },
    { HU_DISP_CENTERX,  72.0f }
};

static float mapViewMinZ[] = {
    -2000.0f, -1500.0f, -2200.0f,
    -1800.0f, -2100.0f, -3000.0f,
    -3000.0f, -1800.0f, -1800.0f
};

static float mapViewMaxZ[] = {
    2300.0f, 2500.0f, 2600.0f,
    2000.0f, 2300.0f, 2500.0f,
    2500.0f, 2000.0f, 2000.0f
};

static float mapViewMinX[] = {
    -3000.0f, -3000.0f, -3000.0f,
    -3000.0f, -3000.0f, -3000.0f,
    -3000.0f, -2000.0f, -2000.0f
};

static float mapViewMaxX[] = {
    3000.0f, 3000.0f, 3000.0f,
    3000.0f, 3000.0f, 3000.0f,
    3000.0f, 1600.0f, 2000.0f
};

static float overheadCamZoom[] = {
    14370.0f, 14500.0f, 15000.0f,
    15000.0f, 13000.0f, 15000.0f,
      100.0f, 10000.0f, 10000.0f
};

static float overheadCamPosZ[] = {
    0.0f,    0.0f, 0.0f,
    0.0f,    0.0f, 0.0f,
    0.0f, -160.0f, 0.0f
};

/**
 * @brief Runs the interactive overhead-board view for a player.
 *
 * @details Temporarily disables pausing, hides item status UI, and waits for
 * the overhead-view process and its display object to finish.
 *
 * @param player Player index whose controller and board position are used.
 */
void BoardViewOverheadExec(s32 player) {
    if (BoardPauseDisableGet() == 0) {
        BoardPauseDisableSet(1);
        pauseDisableF = 1;
    } else {
        pauseDisableF = 0;
    }
    _SetFlag(0x1001C);
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_CROSS, 1);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    viewProc = HuPrcChildCreate(ExecOverheadView, 0x200A, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(viewProc, DestroyView);
    viewControls = 2;
    viewPlayer = player;
    focusTimer = 1;
    viewMoveF = 0;
    BoardPlayerPosGet(player, &focusPos);
    BoardStatusItemSet(0);
    while (viewProc != NULL) {
        HuPrcVSleep();
    }
    BoardStatusItemSet(1);
    HuPrcSleep(3);
    WaitOverheadView();
    _ClearFlag(0x1001C);
}

/**
 * @brief Gets the current position of the active view-focus model.
 *
 * @param[out] position Destination vector, or `NULL` to do nothing.
 */
void BoardViewFocusGetPos(Vec *position) {
    if (position) {
        BoardModelPosGet(focusMdl, position);
    }
}

/**
 * @brief Configures a scripted focus movement.
 *
 * @param[in] start_position Optional starting focus position.
 * @param[in] target_position Optional target focus position.
 * @param duration Movement duration in frames; zero is treated as one frame.
 */
static inline void BoardViewInlineFunc00(Vec *start_position, Vec *target_position, u16 duration) {
    if (duration == 0) {
        duration = 1;
    }
    viewControls = 1;
    viewPlayer = -1;
    focusTimer = duration;
    viewMoveF = 1;
    if (start_position) {
        focusPos = *start_position;
    }
    if (target_position) {
        focusPosTarget = *target_position;
    }
}

/**
 * @brief Sets a target for the currently active scripted view.
 *
 * @param[in] target_position Target focus position.
 * @param duration Movement duration in frames.
 */
void BoardViewFocusSet(Vec *target_position, u16 duration) {
    u16 unused_duration;

    if (BoardPauseDisableGet() == 0) {
        BoardPauseDisableSet(1);
        pauseDisableF = 1;
    } else {
        pauseDisableF = 0;
    }
    focusDestroyF = 1;
    BoardViewInlineFunc00(NULL, target_position, duration);
    HuPrcSleep(2);
}

/**
 * @brief Starts a scripted camera-focus movement process.
 *
 * @param[in] start_position Optional starting focus position.
 * @param[in] target_position Optional target focus position.
 * @param duration Movement duration in frames.
 */
void BoardViewMoveStart(Vec *start_position, Vec *target_position, u16 duration) {
    u16 unused_duration;

    if (BoardPauseDisableGet() == 0) {
        BoardPauseDisableSet(1);
        pauseDisableF = 1;
    } else {
        pauseDisableF = 0;
    }
    viewProc = HuPrcChildCreate(ExecStarView, 0x200A, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(viewProc, DestroyView);
    focusDestroyF = 0;
    BoardViewInlineFunc00(start_position, target_position, duration);
    HuPrcSleep(2);
}

/**
 * @brief Checks whether the scripted view is waiting for a movement signal.
 *
 * @return `TRUE` when the movement flag is set, otherwise `FALSE`.
 */
BOOL BoardViewMoveCheck(void) {
    return (viewMoveF != 0) ? TRUE : FALSE;
}

/** @brief Clears the scripted view movement flag. */
void BoardViewMoveEnd(void) {
    viewMoveF = 0;
}

/** @brief Ends the current movement phase and waits for the view process. */
void BoardViewWait(void) {
    viewMoveF = 0;
    while (viewProc != NULL) {
        HuPrcVSleep();
    }
}

/**
 * @brief Runs the player-controlled board map view.
 *
 * @param player Player index whose position and controller are used.
 */
void BoardViewMapExec(s32 player) {
    if (BoardPauseDisableGet() == 0) {
        BoardPauseDisableSet(1);
        pauseDisableF = 1;
    } else {
        pauseDisableF = 0;
    }
    viewProc = HuPrcChildCreate(ExecMapView, 0x200A, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(viewProc, DestroyView);
    viewControls = 0;
    viewPlayer = player;
    focusTimer = 1;
    viewMoveF = 0;
    BoardPlayerPosGet(player, &focusPos);
    while (viewProc != NULL) {
        HuPrcVSleep();
    }
}

/** @brief Processes camera panning input for the board map view. */
static void ExecMapView(void) {
    Vec movement;
    float move_speed;
    float movement_angle;
    float max_x;
    float min_x;
    float max_z;
    float min_z;
    u32 buttons_down;
    s8 stick_x;
    s8 stick_y;

    SetTargetView();
    CreateButtonWin();
    max_z = mapViewMaxZ[GWBoardGet()];
    min_z = mapViewMinZ[GWBoardGet()];
    max_x = mapViewMaxX[GWBoardGet()];
    min_x = mapViewMinX[GWBoardGet()];
    while (1) {
        buttons_down = HuPadBtnDown[GWPlayer[viewPlayer].port];
        if (buttons_down == 0x200) {
            HuAudFXPlay(3);
            break;
        }
        if (HuPadBtn[GWPlayer[viewPlayer].port] & 0x100) {
            move_speed = 40.0f;
        } else {
            move_speed = 20.0f;
        }
        movement.x = movement.y = movement.z = 0.0f;
        stick_x = HuPadStkX[GWPlayer[viewPlayer].port];
        stick_y = HuPadStkY[GWPlayer[viewPlayer].port];
        if (stick_x != 0 || stick_y != 0) {
            OSs8tof32(&stick_x, &movement.x);
            OSs8tof32(&stick_y, &movement.z);
            movement_angle = atan2d(movement.x, -movement.z);
            movement.x = move_speed * sind(movement_angle);
            movement.z = move_speed * cosd(movement_angle);
        }
        VECAdd(&movement, &focusPos, &focusPos);
        if (focusPos.x < min_x) {
            focusPos.x = min_x;
        }
        if (focusPos.x > max_x) {
            focusPos.x = max_x;
        }
        if (focusPos.z < min_z) {
            focusPos.z = min_z;
        }
        if (focusPos.z > max_z) {
            focusPos.z = max_z;
        }
        BoardModelPosSetV(focusMdl, &focusPos);
        HuPrcVSleep();
    }
    DestroyFocus();
    DestroyButtonWin();
    HuPrcEnd();
}

/** @brief Moves the focus model between scripted positions. */
static void ExecStarView(void) {
    Vec frame_step;
    float duration;

    SetTargetView();
    do {
        OSu16tof32(&focusTimer, &duration);
        VECSubtract(&focusPosTarget, &focusPos, &frame_step);
        frame_step.x /= duration;
        frame_step.y /= duration;
        frame_step.z /= duration;
        viewMoveF = 1;
        while (viewMoveF != 0) {
            HuPrcVSleep();
        }
        focusDestroyF = 0;
        while (focusTimer != 0) {
            VECAdd(&frame_step, &focusPos, &focusPos);
            BoardModelPosSetV(focusMdl, &focusPos);
            HuPrcVSleep();
            focusTimer--;
        }
        BoardModelPosSetV(focusMdl, &focusPosTarget);
        BoardCameraMotionWait();
        viewMoveF = 1;
        while (viewMoveF != 0 && focusDestroyF == 0) {
            HuPrcVSleep();
        }
    } while (focusDestroyF != 0);
    DestroyFocus();
    HuPrcEnd();
}

/** @brief Runs the overhead view until the player presses the exit button. */
static void ExecOverheadView(void) {
    u32 buttons_down;

    if ((GWBoardGet() == BOARD_ID_MAIN3 || GWBoardGet() == BOARD_ID_MAIN6) && boardLightResetHook) {
        boardLightResetHook();
    }
    InitOverheadView();
    CreateButtonWin();
    HuPrcVSleep();
    SetTargetView();
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_CROSS, 10);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    while (1) {
        buttons_down = HuPadBtnDown[GWPlayer[viewPlayer].port];
        if (buttons_down == 0x200) {
            HuAudFXPlay(3);
            break;
        }
        HuPrcVSleep();
    }
    if ((GWBoardGet() == BOARD_ID_MAIN3 || GWBoardGet() == BOARD_ID_MAIN6) && boardLightSetHook) {
        boardLightSetHook();
    }
    DestroyButtonWin();
    DestroyFocus();
    HuPrcEnd();
}

/** @brief Restores camera and pause state when a view process is destroyed. */
static void DestroyView(void) {
    if (focusMdl != -1) {
        BoardModelKill(focusMdl);
        focusMdl = -1;
    }
    DestroyOverheadView();
    DestroyButtonWin();
    viewMoveF = 0;
    focusTimer = 0;
    viewPlayer = -1;
    if (pauseDisableF != 0) {
        BoardPauseDisableSet(0);
        pauseDisableF = 0;
    }
    viewProc = NULL;
}

/** @brief Creates the focus model and points the board camera at it. */
static void SetTargetView(void) {
    Vec target_offset;
    Vec camera_rotation;

    BoardCameraBackup();
    focusMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x0A), 0, 0);
    BoardModelVisibilitySet(focusMdl, 0);
    BoardModelPosSetV(focusMdl, &focusPos);
    BoardCameraNearFarSet(10.0f, 30000.0f);
    if (viewControls != 2) {
        camera_rotation.x = -45.0f;
        camera_rotation.y = 0.0f;
        camera_rotation.z = 0.0f;
        BoardCameraMotionStartEx(focusMdl, &camera_rotation, NULL, 3000.0f, -1.0f, 21);
        BoardCameraMotionWait();
    } else {
        BoardModelPosSet(focusMdl, 0.0f, 0.0f, 0.0f);
        target_offset.x = 0.0f;
        target_offset.y = 0.0f;
        target_offset.z = 0.0f;
        camera_rotation.x = -78.0f;
        camera_rotation.y = 0.0f;
        camera_rotation.z = 0.0f;
        target_offset.z = overheadCamPosZ[GWBoardGet()];
        BoardCameraMoveSet(0);
        BoardCameraMotionStartEx(focusMdl, &camera_rotation, &target_offset, overheadCamZoom[GWBoardGet()], -1.0f, 1);
        BoardCameraMotionWait();
        BoardCameraMoveSet(1);
    }
}

/** @brief Removes the focus model and restores the previous camera state. */
static void DestroyFocus(void) {
    if (focusMdl != -1) {
        BoardModelKill(focusMdl);
        focusMdl = -1;
    }
    BoardCameraMoveSet(0);
    BoardCameraRestore();
    BoardCameraMoveSet(1);
}

/** @brief Creates the on-screen window describing the view controls. */
static void CreateButtonWin(void) {
    float message_size[2];
    float pos_x;
    float pos_y;
    s32 message_id;

    if (viewControls != 2) {
        message_id = MAKE_MESSID(0x09, 0x05);
        #if VERSION_PAL
        HuWinMesMaxSizeGet(1, message_size, message_id);
        pos_x = 288-((message_size[0]/2)-32);
        #endif
        pos_y = 286.0f;
    } else {
        message_id = MAKE_MESSID(0x10, 0x4D);
        #if VERSION_PAL
        pos_x = -10000.0f;
        pos_y = 324.0f;
        #else
        pos_y = 328.0f;
        #endif
        
    }
    HuWinMesMaxSizeGet(1, message_size, message_id);
    #if VERSION_NTSC
    pos_x = -10000.0f;
    #endif
    buttonWin = HuWinCreate(pos_x, pos_y, message_size[0], message_size[1], 0);
    HuWinBGTPLvlSet(buttonWin, 0.0f);
    HuWinMesSpeedSet(buttonWin, 0);
    HuWinMesSet(buttonWin, message_id);
}

/** @brief Destroys the view-control window if it exists. */
static void DestroyButtonWin(void) {
    if (buttonWin != -1) {
        HuWinKill(buttonWin);
        buttonWin = -1;
    }
}

/** @brief Creates the player markers, guide lines, and star marker. */
static void InitOverheadView(void) {
    Vec star_screen_pos;
    GXColor marker_color;
    s16 sprite;
    OMOBJ *object;
    OverheadWork *work;
    float *screen_pos;
    s32 i;
    s32 character_sprite_files[8] = {
        DATA_MAKE_NUM(DATADIR_BOARD, 0x2E),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x2F),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x30),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x31),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x32),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x33),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x34),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x35)
    };

    object = omAddObjEx(boardObjMan, 0x1000, 0, 0, -1, UpdateOverheadView);
    overheadObj = object;
    work = omObjGetWork(object, OverheadWork);
    work->destroy_requested = 0;
    work->markers_visible = 1;
    work->sprite_group = HuSprGrpCreate(9);
    for (i = 0; i < 4; i++) {
        screen_pos = overhead2DPos[i];
        BoardSpriteCreate(character_sprite_files[GWPlayer[i].character], 0x1388, 0, &sprite);
        HuSprGrpMemberSet(work->sprite_group, i, sprite);
        HuSprAttrSet(work->sprite_group, i, 1);
        HuSprAttrSet(work->sprite_group, i, 8);
        HuSprPosSet(work->sprite_group, i, screen_pos[0], screen_pos[1]);
        if (GWPlayer[i].character == 7) {
            marker_color.r = 0x50;
            marker_color.g = 0x50;
            marker_color.b = 0x50;
        } else {
            GWCharColorGet(GWPlayer[i].character, &marker_color);
        }
        BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x6C), 0x1770, 0, &sprite);
        HuSprGrpMemberSet(work->sprite_group, i + 4, sprite);
        HuSprPosSet(work->sprite_group, i + 4, screen_pos[0], screen_pos[1]);
        HuSprColorSet(work->sprite_group, i + 4, marker_color.r, marker_color.g, marker_color.b);
    }
    screen_pos = overhead2DPos[4];
    work->star_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x08), NULL, 0);
    BoardModelMotionSpeedSet(work->star_model, 0.0f);
    BoardModelLayerSet(work->star_model, 6);
    BoardModelScaleSet(work->star_model, 0.4f, 0.4f, 0.4f);
    BoardModelPassSet(work->star_model, 1);
    star_screen_pos.x = screen_pos[0];
    star_screen_pos.y = screen_pos[1];
    star_screen_pos.z = 1000.0f;
    marker_color.r = 0xF8;
    marker_color.g = 0xF0;
    marker_color.b = 0x80;
    BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x6C), 0x1770, 0, &sprite);
    HuSprGrpMemberSet(work->sprite_group, 8, sprite);
    HuSprPosSet(work->sprite_group, 8, screen_pos[0], screen_pos[1]);
    HuSprColorSet(work->sprite_group, 8, marker_color.r, marker_color.g, marker_color.b);
    Hu3D2Dto3D(&star_screen_pos, 1, &star_screen_pos);
    BoardModelPosSetV(work->star_model, &star_screen_pos);
    HuSprGrpDrawNoSet(work->sprite_group, 0x40);
    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        BoardModelVisibilitySet(work->star_model, 0);
        HuSprTPLvlSet(work->sprite_group, 8, 0.0f);
    }
}

/** @brief Requests destruction of the overhead-view display object. */
static void DestroyOverheadView(void) {
    if (overheadObj) {
        omObjGetWork(overheadObj, OverheadWork)->destroy_requested = 1;
    }
}

/**
 * @brief Updates overhead markers and their guide lines.
 *
 * @param object Object that owns the overhead-view work data.
 */
static void UpdateOverheadView(OMOBJ *object) {
    Vec marker_screen_pos;
    Vec board_screen_pos;
    Vec guide_delta;
    float guide_length_scale;
    float guide_angle;
    float *marker_pos;
    s32 guide_sprite;
    s32 hide_star_marker;
    s32 i;
    u32 buttons_down;
    OverheadWork *work;
    Mtx camera_x_rotation;
    Mtx camera_rotation;

    work = omObjGetWork(object, OverheadWork);
    if (work->destroy_requested != 0 || BoardIsKill()) {
        if (work->sprite_group != -1) {
            HuSprGrpKill(work->sprite_group);
        }
        if (work->star_model != -1) {
            BoardModelKill(work->star_model);
        }
        overheadObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    buttons_down = HuPadBtnDown[GWPlayer[viewPlayer].port];
    if (buttons_down == 0x20) {
        work->markers_visible ^= 1;
        HuAudFXPlay(0x1C);
    }
    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        hide_star_marker = 1;
    } else {
        hide_star_marker = 0;
    }
    for (i = 0; i < 5; i++) {
        marker_pos = overhead2DPos[i];
        marker_screen_pos.x = marker_pos[0] - HU_DISP_CENTERX;
        marker_screen_pos.y = marker_pos[1] - 240.0f;
        marker_screen_pos.z = 1000.0f;
        if (i != 4) {
            BoardSpacePosGet(0, GWPlayer[i].space_curr, &board_screen_pos);
        } else if (hide_star_marker != 0) {
            board_screen_pos.x = board_screen_pos.y = board_screen_pos.z = 0.0f;
        } else {
            BoardSpacePosGet(0, BoardSpaceStarGetCurr(), &board_screen_pos);
        }
        Hu3D3Dto2D(&board_screen_pos, 1, &board_screen_pos);
        board_screen_pos.x -= HU_DISP_CENTERX;
        board_screen_pos.y -= 240.0f;
        VECSubtract(&marker_screen_pos, &board_screen_pos, &guide_delta);
        guide_delta.z = 0.0f;
        guide_angle = BoardDAngleCalc(atan2d(guide_delta.y, guide_delta.x) - 90.0);
        guide_length_scale = VECMag(&guide_delta) / 32;
        if (i != 4) {
            guide_sprite = i + 4;
        } else {
            guide_sprite = 8;
        }
        HuSprZRotSet(work->sprite_group, guide_sprite, guide_angle);
        HuSprScaleSet(work->sprite_group, guide_sprite, 0.5f, guide_length_scale);
        HuSprPosSet(work->sprite_group, guide_sprite, marker_pos[0], marker_pos[1]);
        if (i == 4 && (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2)) {
            BoardModelVisibilitySet(work->star_model, 0);
        } else if (work->markers_visible != 0) {
            if (i != 4) {
                HuSprAttrReset(work->sprite_group, i, HUSPR_ATTR_DISPOFF);
            }
            HuSprAttrReset(work->sprite_group, guide_sprite, HUSPR_ATTR_DISPOFF);
            BoardModelVisibilitySet(work->star_model, 1);
        } else {
            if (i != 4) {
                HuSprAttrSet(work->sprite_group, i, HUSPR_ATTR_DISPOFF);
            }
            HuSprAttrSet(work->sprite_group, guide_sprite, HUSPR_ATTR_DISPOFF);
            BoardModelVisibilitySet(work->star_model, 0);
        }
    }
    marker_screen_pos.x = marker_pos[0];
    marker_screen_pos.y = marker_pos[1];
    marker_screen_pos.z = 1000.0f;
    BoardCameraRotGet(&guide_delta);
    PSMTXRotRad(camera_x_rotation, 'x', MTXDegToRad(guide_delta.x));
    PSMTXRotRad(camera_rotation, 'y', MTXDegToRad(guide_delta.y));
    PSMTXConcat(camera_rotation, camera_x_rotation, camera_rotation);
    BoardModelMtxSet(work->star_model, &camera_rotation);
    Hu3D2Dto3D(&marker_screen_pos, 1, &marker_screen_pos);
    BoardModelPosSetV(work->star_model, &marker_screen_pos);
}

/** @brief Waits until the overhead-view display object has been destroyed. */
static void WaitOverheadView(void) {
    while (overheadObj) {
        HuPrcVSleep();
    }
}
