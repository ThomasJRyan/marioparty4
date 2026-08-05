#include "game/board/char_wheel.h"
#include "game/audio.h"
#include "game/flag.h"
#include "game/gamework.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/board/com.h"
#include "game/board/main.h"
#include "game/board/tutorial.h"
#include "game/board/ui.h"

#include "ext_math.h"

typedef struct {
    struct {
        u8 kill : 1;
        u8 include_current_player : 1;
        u8 stopping : 1;
        u8 state : 4;
    };
    s8 player;
    s8 scale_angle;
    u8 result_flash_timer;
    s16 input_timeout;
    s16 wheel_angle;
    s16 wheel_velocity;
} CharWheelWork;

static void ExecCharWheel(OMOBJ *object);
static void InitCharWheel(CharWheelWork *work);
static void UpdateCharWheel(CharWheelWork *work);
static void StopCharWheel(CharWheelWork *work);
static void GrowCharWheel(CharWheelWork *work);
static void ShrinkCharWheel(CharWheelWork *work);
static void KillCharWheelSpr(void);
static void SetCharWheelChars(s32 include_current_player, s32 current_player, s8 *candidates);
static s32 GetInput(CharWheelWork *input_work);
static s16 GetComDelay(s32 current_player, s32 include_current_player);
static s32 GetComInput(CharWheelWork *work);

static s8 wheelChars[4];
static s16 comDelay;
static s16 wheelTimer;
static float wheelSpeed;
static s16 targetVal;
static float angleOfs;
static s16 targetF;
static OMOBJ *charWheelObj;

static s8 wheelPos = -1;
static s16 wheelSprGrp = -1;
static s32 wheelFXStat = -1;

static s32 wheelSprTbl[] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 0x36),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x38),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x38),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x38),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x37),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x37),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x37),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x37),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x3A),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x39)
};

static s8 wheelSprPrio[] = {
    0x3C, 0x50, 0x50, 0x50,
    0x50, 0x50, 0x50, 0x50,
    0x14, 0x1E,
    // wheelCharIconPrioTbl
    0x46, 0x46, 0x46, 0x46
};

static s32 wheelCharIconTbl[] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 0x3B),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x3C),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x3D),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x3E),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x3F),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x40),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x41),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x42)
};

/**
 * @brief Start the character-selection wheel for a player.
 *
 * @param player Player controlling the wheel.
 * @param mode Wheel mode; this implementation currently forces the
 *     three-candidate mode.
 * @return This legacy entry point has no meaningful return value.
 */
s32 BoardCharWheelInit(s32 player, s32 mode) {
    CharWheelWork *work;

    mode = 3;
    charWheelObj = omAddObjEx(boardObjMan, 0x104, 0, 0, -1, ExecCharWheel);
    work = omObjGetWork(charWheelObj, CharWheelWork);
    work->kill = 0;
    work->stopping = 0;
    work->include_current_player = (mode == 3) ? 0 : 1;
    work->player = player;
    work->scale_angle = 0;
    work->result_flash_timer = 0;
    work->wheel_angle = 0;
    wheelSpeed = 18.0f;
    if (GWPlayer[work->player].com) {
        work->input_timeout = 360;
    } else {
        work->input_timeout = 3600;
    }
}

/** @brief Check whether the character wheel has finished. @return TRUE once the wheel object is gone. */
BOOL BoardCharWheelCheck(void) {
    return (charWheelObj != NULL) ? FALSE : TRUE;
}

/** @brief Wait until the active character wheel has finished. */
void BoardCharWheelWait(void) {
    while (!BoardCharWheelCheck()) {
        HuPrcVSleep();
    }
}

/** @brief Get the player selected by the last wheel spin. @return Selected player index. */
s32 BoardCharWheelResultGet(void) {
    return wheelChars[wheelPos];
}

/** @brief Set the wheel's maximum angular speed. @param speed Speed in degrees per frame, or a nonpositive value for the default. */
void BoardCharWheelSpeedSet(float speed) {
    if (speed <= 0.0f) {
        speed = 18.0f;
    }
    wheelSpeed = speed;
}

/** @brief Advance wheel creation, spinning, result flashing, or teardown for one frame. @param object Object that owns the character-wheel work state. */
static void ExecCharWheel(OMOBJ *object) {
    CharWheelWork *work;

    work = omObjGetWork(object, CharWheelWork);;
    if (work->kill != 0 || BoardIsKill()) {
        KillCharWheelSpr();
        #if VERSION_PAL
        if (wheelFXStat != -1) {
            HuAudFXStop(wheelFXStat);
            wheelFXStat = -1;
        }
        #endif
        charWheelObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    if (wheelTimer < work->input_timeout) {
        wheelTimer++;
    }
    switch (work->state) {
        case 0:
            InitCharWheel(work);
            break;
        case 1:
            GrowCharWheel(work);
            break;
        case 2:
            UpdateCharWheel(work);
            break;
        case 3:
            StopCharWheel(work);
            break;
        case 4:
            ShrinkCharWheel(work);
            break;
    }
}

/** @brief Create the three- or four-candidate wheel sprites and choose CPU timing. @param work Work state containing the controlling player and candidate mode. */
static void InitCharWheel(CharWheelWork *work) {
    GXColor character_color;
    float segment_angle;
    float segment_index;
    float unused_half_segment_angle;
    s32 segment_sprite_offset;
    s16 com_delay_jitter;
    s16 character;
    s16 i;
    s16 candidate_count;
    s16 sprite;

    wheelChars[0] = wheelChars[1] = wheelChars[2] = wheelChars[3] = -1;
    wheelFXStat = -1;
    wheelTimer = 0;
    if (work->include_current_player != 0) {
        candidate_count = 4;
        segment_sprite_offset = 4;
        unused_half_segment_angle = 45.0f;
    } else {
        candidate_count = 3;
        segment_sprite_offset = 1;
        unused_half_segment_angle = 60.0f;
    }
    wheelSprGrp = HuSprGrpCreate(candidate_count + 14);
    SetCharWheelChars(work->include_current_player, work->player, wheelChars);
    if (GWPlayer[work->player].com) {
        comDelay = GetComDelay(work->player, work->include_current_player);
        switch (GWPlayer[work->player].diff) {
            case 0:
                com_delay_jitter = BoardRandMod(120);
                break;
            case 1:
                com_delay_jitter = BoardRandMod(60);
                break;
            case 2:
                com_delay_jitter = BoardRandMod(30);
                break;
            case 3:
                com_delay_jitter = 0;
                break;
        }
        comDelay += com_delay_jitter;
        if (comDelay > 360.0f) {
            comDelay -= 360.0f;
        }
        if (comDelay < 0) {
            comDelay += 360.0f;
        }
    } else {
        comDelay = 0;
    }
    for (i = 0; i <= 9; i++) {
        BoardSpriteCreate(wheelSprTbl[i], wheelSprPrio[i], 0, &sprite);
        HuSprGrpMemberSet(wheelSprGrp, i, sprite);
        HuSprAttrSet(wheelSprGrp, i, 8);
    }
    if (work->include_current_player == 0) {
        for (i = 0; i < 4; i++) {
            HuSprAttrSet(wheelSprGrp, i + 4, 4);
        }
    } else {
        for (i = 0; i < 3; i++) {
            HuSprAttrSet(wheelSprGrp, i + 1, 4);
        }
    }
    for (i = 0; i < candidate_count; i++) {
        character = GWPlayer[wheelChars[i]].character;
        BoardSpriteCreate(wheelCharIconTbl[character], wheelSprPrio[i + 10], 0, &sprite);
        HuSprGrpMemberSet(wheelSprGrp, i + 10, sprite);
        HuSprAttrSet(wheelSprGrp, i + 10, 8);
    }
    HuSprPosSet(wheelSprGrp, 9, 3.0f, 3.0f);
    HuSprColorSet(wheelSprGrp, 9, 0x40, 0x40, 0x40);
    HuSprGrpPosSet(wheelSprGrp, 288.0f, 240.0f);
    HuSprGrpScaleSet(wheelSprGrp, 0.01f, 0.01f);
    OSs16tof32(&candidate_count, &segment_angle);
    segment_angle = 360.0f / segment_angle;
    for (i = 0; i < candidate_count; i++) {
        if (work->include_current_player != 0) {
            switch (i) {
                case 0:
                    HuSprPosSet(wheelSprGrp, i + 10, 50.0f, -57.0f);
                    break;
                case 1:
                    HuSprPosSet(wheelSprGrp, i + 10, -50.0f, -57.0f);
                    break;
                case 2:
                    HuSprPosSet(wheelSprGrp, i + 10, 50.0f, 57.0f);
                    break;
                case 3:
                    HuSprPosSet(wheelSprGrp, i + 10, -50.0f, 57.0f);
                    break;
            }
        } else {
            switch (i) {
                case 0:
                    HuSprPosSet(wheelSprGrp, i + 10, 50.0f, -35.0f);
                    break;
                case 1:
                    HuSprPosSet(wheelSprGrp, i + 10, 0.0f, 57.0f);
                    break;
                case 2:
                    HuSprPosSet(wheelSprGrp, i + 10, -50.0f, -35.0f);
                    break;
            }
        }
        OSs16tof32(&i, &segment_index);
        HuSprZRotSet(wheelSprGrp, segment_sprite_offset + i, segment_angle * segment_index);
        GWCharColorGet(GWPlayer[wheelChars[i]].character, &character_color);
        HuSprColorSet(wheelSprGrp, segment_sprite_offset + i, character_color.r, character_color.g, character_color.b);
    }
    for (i = 0; i < 3; i++) {
        if (wheelChars[i] == targetVal) {
            break;
        }
    }
    angleOfs = 120.0f * i;
    if (angleOfs > 180.0f) {
        angleOfs -= 360.0f;
    }
    work->state = 1;
}

/** @brief Accelerate the wheel, accept stop input, then decelerate to a candidate. @param work Work state holding fixed-point angle, velocity, and stop status. */
static void UpdateCharWheel(CharWheelWork *work) {
    float angle;
    float velocity;
    float pitch_offset;
    u32 input;
    s16 angle_in_segment;
    s16 segment_degrees;
    s16 pitch;

    OSs16tof32(&work->wheel_angle, &angle);
    OSs16tof32(&work->wheel_velocity, &velocity);
    angle = angle * 0.0625f;
    velocity = velocity * 0.0625f;
    if (work->stopping == 0) {
        if (velocity < 18.0f) {
            velocity += 0.7f;
            if (velocity > wheelSpeed) {
                velocity = wheelSpeed;
            }
        } else {
            input = GetInput(work);
            if (input & 0x100) {
                work->stopping = 1;
            }
        }
    } else {
        OSf32tos16(&angle, &angle_in_segment);
        if (work->include_current_player == 0) {
            segment_degrees = 120;
        } else {
            segment_degrees = 90;
        }
        angle_in_segment %= segment_degrees;
        if (velocity < 0.5f && (angle_in_segment < 2 || angle_in_segment >= segment_degrees - 2)) {
            velocity += velocity / 2;
        }
        velocity *= 0.92f;
    }
    pitch_offset = 8191.0f - 8191.0f * (velocity / wheelSpeed);
    OSf32tos16(&pitch_offset, &pitch);
    #if VERSION_PAL
    if(wheelFXStat != -1) {
        HuAudFXPitchSet(wheelFXStat, -pitch);
    }
    #else
    HuAudFXPitchSet(wheelFXStat, -pitch);
    #endif
    if (velocity > -0.0000001f && velocity < 0.0000001f) {
        work->state = 3;
        if (wheelFXStat != -1) {
            HuAudFXStop(wheelFXStat);
            wheelFXStat = -1;
        }
        HuAudFXPlay(0x30B);
    }
    angle += velocity;
    if (angle > 360.0f) {
        angle -= 360.0f;
    }
    HuSprZRotSet(wheelSprGrp, 8, angle);
    HuSprZRotSet(wheelSprGrp, 9, angle);
    angle *= 16.0f;
    velocity *= 16.0f;
    OSf32tos16(&angle, &work->wheel_angle);
    OSf32tos16(&velocity, &work->wheel_velocity);
}

/** @brief Resolve the selected segment and flash its portrait before closing. @param work Work state holding the final wheel angle and flash timer. */
static void StopCharWheel(CharWheelWork *work) {
    u8 flash_phase;

    if (work->result_flash_timer == 0) {
        work->wheel_angle >>= 4;
        if (work->include_current_player == 0) {
            wheelPos = work->wheel_angle / 120;
        } else {
            wheelPos = work->wheel_angle / 90;
        }
    }
    flash_phase = work->result_flash_timer % 6;
    if (flash_phase < 3) {
        HuSprAttrSet(wheelSprGrp, wheelPos + 10, HUSPR_ATTR_DISPOFF);
    } else {
        HuSprAttrReset(wheelSprGrp, wheelPos + 10, HUSPR_ATTR_DISPOFF);
    }
    if (work->result_flash_timer < 90) {
        work->result_flash_timer++;
    } else {
        HuSprAttrReset(wheelSprGrp, wheelPos + 10, HUSPR_ATTR_DISPOFF);
        work->state = 4;
    }
}

/** @brief Scale the wheel into view over 30 frames and begin its spin. @param work Work state whose scale angle and state are advanced. */
static void GrowCharWheel(CharWheelWork *work) {
    float scale_angle;

    if (work->scale_angle < 90) {
        work->scale_angle += 3;
    } else {
        if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
            BoardTutorialHookExec(0x14, 0);
        }
        work->scale_angle = 90;
        work->state = 2;
        #if VERSION_PAL
        if(!BoardIsKill()) {
            wheelFXStat = HuAudFXPlay(0x30A);
        }
        #else
        wheelFXStat = HuAudFXPlay(0x30A);
        #endif
        
    }
    OSs8tof32(&work->scale_angle, &scale_angle);
    HuSprGrpScaleSet(wheelSprGrp, sind(scale_angle), sind(scale_angle));
}

/** @brief Scale the wheel out over 30 frames and mark the object for deletion. @param work Work state whose scale angle and kill flag are updated. */
static void ShrinkCharWheel(CharWheelWork *work) {
    float scale_angle;

    if (work->scale_angle > 0) {
        work->scale_angle -= 3;
    } else {
        work->scale_angle = 0;
        work->kill = 1;
        work->state = -1;
        targetF = 0;
    }
    OSs8tof32(&work->scale_angle, &scale_angle);
    HuSprGrpScaleSet(wheelSprGrp, sind(scale_angle), sind(scale_angle));
}

/** @brief Destroy the wheel sprite group if it exists. */
static void KillCharWheelSpr(void) {
    if (wheelSprGrp != -1) {
        HuSprGrpKill(wheelSprGrp);
        wheelSprGrp = -1;
    }
}

/**
 * @brief Fill the wheel with distinct random player indices.
 * @param include_current_player Whether the controlling player may appear.
 * @param current_player Controlling player index.
 * @param candidates Output candidate array.
 */
static void SetCharWheelChars(s32 include_current_player, s32 current_player, s8 *candidates) {
    s32 player;
    s32 candidate_index;
    s32 existing_index;
    s32 candidate;

    for (candidate_index = player = 0; player < 4; player++) {
        if (player == current_player && include_current_player == 0) {
            continue;
        }
        do {
            do {
                candidate = BoardRandMod(4);
            } while (candidate == current_player && include_current_player == 0);
            for (existing_index = 0; existing_index < 4; existing_index++) {
                if (candidate == candidates[existing_index]) {
                    candidate = -1;
                    break;
                }
            }
        } while (candidate == -1);
        candidates[candidate_index] = candidate;
        candidate_index++;
    }
}

/** @brief Read human or CPU stop input for the wheel. @param input_work Wheel state used for player and timeout data. @return Button mask. */
static s32 GetInput(CharWheelWork *input_work) {
    CharWheelWork *wheel_work;
    float angle;
    float velocity;
    float target_delta;
    float predicted_angle;
    s32 input;

    if (wheelTimer >= input_work->input_timeout) {
        return 0x100;
    }
    if (!GWPlayer[input_work->player].com) {
        input = HuPadBtnDown[GWPlayer[input_work->player].port];
        return  input;
    }
    input = GetComInput(input_work);
    if (targetF != 0) {
        wheel_work = omObjGetWork(charWheelObj, CharWheelWork);
        OSs16tof32(&wheel_work->wheel_angle, &angle);
        OSs16tof32(&wheel_work->wheel_velocity, &velocity);
        angle = angle * 0.0625f;
        velocity = velocity * 0.0625f;
        predicted_angle = angle + 46.0f * velocity;
        predicted_angle = fmod(predicted_angle, 360.0);
        if (predicted_angle > 180.0f) {
            predicted_angle -= 360.0f;
        }
        target_delta = angleOfs - predicted_angle;
        if (target_delta >= 360.0f) {
            target_delta += 360.0f;
        }
        if (target_delta >= 0.0f && target_delta < 120.0f - 2.0f * velocity) {
            input = 0x100;
        } else {
            input = 0;
        }
        OSReport("%f %f : %f %f\n", angle, velocity, predicted_angle, target_delta);
    }
    return input;
}

/** @brief Force the wheel to aim for a player. @param player Target player index. */
void BoardCharWheelTargetSet(s16 player) {
    targetF = 1;
    targetVal = player;
}

/**
 * @brief Choose the angular interval in which a CPU player stops the wheel.
 * @param current_player CPU player spinning the wheel.
 * @param include_current_player Nonzero when all four players are wheel candidates.
 * @return Start angle of the chosen candidate's stop interval, in degrees.
 */
static s16 GetComDelay(s32 current_player, s32 include_current_player) {
    float stop_angle;
    s16 stop_angle_int;
    s32 candidate_count;
    s32 target_player;
    s32 i;

    target_player = BoardComFarPlayerFind();
    if (target_player == -1 || target_player == current_player) {
        do {
            target_player = BoardRandMod(4);
        } while (target_player == current_player);
    }
    if (include_current_player != 0) {
        candidate_count = 4;
    } else {
        candidate_count = 3;
    }
    for (stop_angle_int = i = 0; i < candidate_count; i++) {
        if (target_player == wheelChars[i]) {
            stop_angle = i * (360 / candidate_count);
            stop_angle += 45.0f;
            stop_angle -= 180.0f;
            if (stop_angle < 0.0f) {
                stop_angle += 360.0f;
            }
            if (stop_angle >= 360.0f) {
                stop_angle -= 360.0f;
            }
            OSf32tos16(&stop_angle, &stop_angle_int);
            break;
        }
    }
    return stop_angle_int;
}

/** @brief Press A while the wheel crosses the CPU's selected angular interval. @param work Wheel state containing the current angle and mode. @return A-button mask while inside the interval, otherwise zero. */
static s32 GetComInput(CharWheelWork *work) {
    s16 angle;
    s32 interval_end;
    s32 interval_start;
    s32 segment_degrees;

    angle = (work->wheel_angle >> 4);
    segment_degrees = (work->include_current_player != 0) ? 90 : 120;
    interval_start = comDelay;
    interval_end = comDelay + segment_degrees;
    if (angle >= interval_start && angle < interval_end) {
        return 0x100;
    } else {
        return 0;
    }
}
