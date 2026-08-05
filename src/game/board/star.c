#include "game/board/star.h"
#include "game/audio.h"
#include "game/data.h"
#include "game/disp.h"

#include "game/board/audio.h"
#include "game/board/com.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/tutorial.h"
#include "game/board/ui.h"
#include "game/board/window.h"
#include "game/flag.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/msm.h"
#include "game/objsub.h"
#include "game/window.h"
#include "game/wipe.h"
#include "game/hsfex.h"

#include "ext_math.h"

#ifndef __MWERKS__
#include "game/board/view.h"
#endif

typedef struct {
    struct {
        u8 kill : 1;
        u8 spin : 1;
        u8 motion_looped : 1;
        u8 player : 2;
        u8 sound_played : 1;
    };
    s8 state;
    s8 spin_speed;
    char padding[1];
    s16 delay;
    s16 write_only_field;
    s16 shrink_angle;
    s16 arc_frame;
    s16 star_model;
} GiveStarWork;

typedef struct {
    struct {
        u8 kill : 1;
    };
    s8 state;
} ShowNextWork;

static void ExecStar(void);
static void DestroyStar(void);
static void GiveStarMain(OMOBJ *arg0);
static void UpdateStarAngle(GiveStarWork *arg0, OMOBJ *arg1);
static void InitGiveStar(GiveStarWork *arg0, OMOBJ *arg1);
static void MoveGiveStar(GiveStarWork *arg0, OMOBJ *arg1);
static void ShrinkGiveStar(GiveStarWork *arg0, OMOBJ *arg1);
static void StopGiveStar(void);
static void InitGiveStarEffect(void);
static void KillGiveStarEffect(void);
static void ShowNextUpdate(OMOBJ *arg0);
static void HideNextHost(s32 arg0);

static Vec starCharPos;

static s16 hostMot[2];
static OMOBJ *showNextObj;
static s16 starDoneF;
static ANIMDATA *starEffAnim;
static OMOBJ *giveStarObj;
static HUPROCESS *starProc;

static s16 starParman = -1;
static s16 hostMdl = -1;

static const s32 starMesTbl1[9] = {
    MAKE_MESSID(12, 0),
    MAKE_MESSID(12, 9),
    MAKE_MESSID(12, 18),
    MAKE_MESSID(12, 27),
    MAKE_MESSID(12, 36),
    MAKE_MESSID(12, 45),
    MAKE_MESSID(12, 0),
    MAKE_MESSID(12, 0),
    MAKE_MESSID(12, 0)
};

static const s32 starMesTbl2[9][2] = {
    { MAKE_MESSID(21, 34), MAKE_MESSID(21, 40) },
    { MAKE_MESSID(21, 35), MAKE_MESSID(21, 41) },
    { MAKE_MESSID(21, 36), MAKE_MESSID(21, 42) },
    { MAKE_MESSID(21, 37), MAKE_MESSID(21, 43) },
    { MAKE_MESSID(21, 38), MAKE_MESSID(21, 44) },
    { MAKE_MESSID(21, 39), MAKE_MESSID(21, 45) },
    { MAKE_MESSID(21, 34), MAKE_MESSID(21, 40) },
    { MAKE_MESSID(21, 34), MAKE_MESSID(21, 40) },
    { MAKE_MESSID(21, 34), MAKE_MESSID(21, 40) }
};

s32 boardStarSndTbl[] = {
    0x00000120,
    0x00000160,
    0x000001A0,
    0x000001E0,
    0x00000220,
    0x00000260,
    0x000002A0,
    0x000002E0
};

static s32 hostMotTbl[9][2] = {
    { DATA_MAKE_NUM(DATADIR_W01, 33), DATA_MAKE_NUM(DATADIR_BOARD, 157) },
    { DATA_MAKE_NUM(DATADIR_W02, 7), DATA_MAKE_NUM(DATADIR_BOARD, 158) },
    { DATA_MAKE_NUM(DATADIR_W03, 31), DATA_MAKE_NUM(DATADIR_BOARD, 159) },
    { DATA_MAKE_NUM(DATADIR_W04, 11), DATA_MAKE_NUM(DATADIR_BOARD, 160) },
    { DATA_MAKE_NUM(DATADIR_W05, 9), DATA_MAKE_NUM(DATADIR_BOARD, 161) },
    { DATA_MAKE_NUM(DATADIR_W06, 21), DATA_MAKE_NUM(DATADIR_BOARD, 162) }
};

static HU3DPARMANPARAM starEffParam = {
    0x001E,
    { 0x00, 0x00 }, // padding?
    3.3f,
    70.0f,
    7.0f,
    { 0.0f, -0.05f, 0.0f },
    2.0f,
    1.0f,
    20.0f,
    0.98f,
    0x0002,
    {
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0x40, 0xFF,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    },
    {
        0xFF, 0x80, 0x80, 0x00,
        0xFF, 0x40, 0x20, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    }
};

/**
 * @brief Sets the star-host model and associates it with the current star space.
 * @param arg0 Board-model handle for the star host.
 */
void BoardStarHostSet(s16 arg0) {
    hostMdl = arg0;
    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        return;
    }
    if (_CheckFlag(FLAG_ID_MAKE(1, 1))) {
        BoardSpaceHostSet(boardSpaceStarTbl[GWSystem.star_pos]);
    }
}

/**
 * @brief Gets the star-host board model handle.
 * @return Star-host board-model handle, or -1 when unset.
 */
s16 BoardStarHostMdlGet(void) {
    return hostMdl;
}

/**
 * @brief Runs and waits for the current player's star-purchase interaction.
 * @param arg0 Player index arriving at the star.
 * @param arg1 Board-space argument retained by the event interface but not read.
 */
void BoardStarExec(s32 arg0, s32 arg1) {
    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        return;
    }
    if (BoardPlayerSizeGet(arg0) != 2) {
        BoardRollDispSet(0);
        starProc = HuPrcChildCreate(ExecStar, 0x2003, 0x3800, 0, boardMainProc);
        HuPrcDestructorSet2(starProc, DestroyStar);
        while (starProc != NULL) {
            HuPrcVSleep();
        }
        BoardRollDispSet(1);
    }
}

/**
 * @brief Handles star eligibility, purchase choice, payment, award, and host reset.
 */
static void ExecStar(void) {
    Vec player_position;
    Vec linked_space_position;
    Vec host_direction;
    float host_angle;
    float player_angle;
    float target_angle;
    float angle_distance;
    float host_angle_step;
    s16 host_model;
    u32 space_variant;
    s32 current_space;
    s32 message;
    s32 rotate_host;
    s32 turn_frames;
    s32 i;
    s32 board;
    s32 player;
    s32 linked_space;
    BoardSpace *current_space_data;

    player = GWSystem.player_curr;
    board = GWBoardGet();
    host_model = BoardStarHostMdlGet();
    HuAudFXPlay(0x33A);
    for (i = 0; i < 2; i++) {
        hostMot[i] = BoardModelMotionCreate(host_model, hostMotTbl[board][i]);
    }
    BoardPlayerPosGet(player, &player_position);
    current_space = GWPlayer[player].space_curr;
    current_space_data = BoardSpaceGet(0, current_space);
    space_variant = (current_space_data->flag & 0x70000) >> 16;
    starDoneF = 0;
    linked_space = BoardSpaceLinkFlagSearch(0, current_space, 0x04000000);
    BoardSpacePosGet(0, linked_space, &linked_space_position);
    VECSubtract(&linked_space_position, &player_position, &host_direction);
    target_angle = 90.0 - atan2d(host_direction.z, host_direction.x);
    target_angle = 90.0f * ((target_angle + 3.0f) / 90.0f);
    player_angle = BoardPlayerRotYGet(player);
    BoardPlayerIdleSet(player);
    if (BoardDAngleCalc(target_angle - player_angle) < 0.0f) {
        angle_distance = -BoardDAngleCalc(target_angle - player_angle);
    } else {
        angle_distance = BoardDAngleCalc(target_angle - player_angle);
    }
    if (angle_distance > 90.0f) {
        turn_frames = 30;
    } else {
        turn_frames = 15;
    }
    BoardPlayerMotBlendSet(player, target_angle, turn_frames);
    host_angle = BoardModelRotYGet(host_model);
    host_angle_step = BoardDAngleCalc(target_angle + 180.0f) - host_angle;
    if ((host_angle_step <= 4.0f && host_angle_step >= 0.0f)
        || (host_angle_step <= 360.0f && host_angle_step >= 355.0f)
        || (host_angle_step <= 0.0f && host_angle_step >= -4.0f)) {
        rotate_host = 0;
    } else {
        rotate_host = 1;
    }
    if (rotate_host != 0) {
        host_angle_step /= turn_frames;
        BoardModelMotionShiftSet(host_model, hostMot[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
        BoardModelMotionSpeedSet(host_model, 2.0f);
    }
    while (!BoardPlayerMotBlendCheck(player)) {
        if (rotate_host != 0) {
            target_angle = host_angle_step + BoardModelRotYGet(host_model);
            BoardModelRotYSet(host_model, target_angle);
        }
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(host_model, 1, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    BoardCameraViewSet(3);
    BoardCameraMotionWait();
    if (GWStarsGet(player) >= 999) {
        message = starMesTbl1[board] + 5;
        BoardWinCreate(0, message, BoardWinPortraitGetStar());
        BoardWinWait();
        BoardWinKill();
    } else if (BoardPlayerCoinsGet(player) < 20 && GWSystem.last5_effect != 4) {
        message = starMesTbl1[board] + 2;
        BoardWinCreate(2, message, BoardWinPortraitGetStar());
        BoardWinWait();
        BoardWinKill();
    } else {
        BoardStatusShowSetAll(0);
        while (!BoardStatusStopCheck(0) || !BoardStatusStopCheck(1) || !BoardStatusStopCheck(2) || !BoardStatusStopCheck(3)) {
            HuPrcVSleep();
        }
        message = starMesTbl1[board];
        BoardWinCreate(2, message, BoardWinPortraitGetStar());
        BoardWinInsertMesSet(BoardPlayerGetCharMess(player), 0);
        BoardWinWait();
        BoardWinKill();
        while (1) {
            BoardStatusShowSetForce(player);
            BoardStatusShowSet(player, 1);
            while (!BoardStatusStopCheck(player)) {
                HuPrcVSleep();
            }
            if (GWSystem.last5_effect == 4) {
                message = starMesTbl1[board] + 8;
                BoardWinCreate(2, message, BoardWinPortraitGetStar());
                BoardWinWait();
                break;
            }
            message = starMesTbl1[board] + 1;
            BoardWinCreateChoice(1, message, BoardWinPortraitGetStar(), 0);
            BoardWinAttrSet(0x10);
            if (GWPlayer[player].com) {
                if (BoardPlayerCoinsGet(player) >= 20) {
                    BoardComKeySetUp();
                } else {
                    BoardComKeySetDown();
                }
            }
            BoardWinWait();
            switch (BoardWinChoiceGet()) {
                case 0:
                    if (BoardPlayerCoinsGet(player) < 20) {
                        message = starMesTbl1[board] + 2;
                        BoardWinCreate(2, message, BoardWinPortraitGetStar());
                        BoardWinWait();
                        BoardStatusShowSet(player, 0);
                        while (!BoardStatusStopCheck(player)) {
                            HuPrcVSleep();
                        }
                        BoardStatusShowSetAll(1);
                        goto block_B;
                    }
                    goto block_A;
                case 1:
                    message = starMesTbl1[board] + 4;
                    BoardWinCreate(2, message, BoardWinPortraitGetStar());
                    BoardWinWait();
                    BoardStatusShowSet(player, 0);
                    while (!BoardStatusStopCheck(player)) {
                        HuPrcVSleep();
                    }
                    BoardStatusShowSetAll(1);
                    goto block_B;
                default:
                    BoardStatusShowSet(player, 0);
                    while (!BoardStatusStopCheck(player)) {
                        HuPrcVSleep();
                    }
                    BoardWinKill();
                    BoardViewMapExec(player);
                    break;
            }
        }
block_A:
        message = starMesTbl1[board] + 3;
        BoardWinCreate(2, message, BoardWinPortraitGetStar());
        BoardWinWait();
        BoardWinKill();
        if (GWSystem.last5_effect != 4) {
            for (i = 0; i < 20; i++) {
                BoardPlayerCoinsAdd(player, -1);
                HuAudFXPlay(0xE);
                HuPrcSleep(3);
            }
            HuAudFXPlay(0xF);
        }
        _SetFlag(0x10017);
        BoardModelMotionShiftSet(host_model, hostMot[0], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
        HuPrcSleep(30);
        starDoneF = 1;
        BoardAudSeqFadeOut(0, 1000);
        BoardModelPosGet(host_model, &player_position);
        player_position.y += 130.0f;
        BoardStarGive(player, &player_position);
        BoardStatusShowSet(player, 0);
        while (!BoardStatusStopCheck(player)) {
            HuPrcVSleep();
        }
        BoardModelMotionShiftSet(host_model, 1, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
        BoardStarShowNext(player);
        BoardMusStartBoard();
        BoardModelRotYSet(host_model, 0.0f);
    }
block_B:
    BoardWinKill();
    host_angle_step = -BoardModelRotYGet(host_model);
    if (rotate_host != 0) {
        host_angle_step /= turn_frames;
        BoardModelMotionShiftSet(host_model, hostMot[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
        BoardModelMotionSpeedSet(host_model, 3.0f);
    }
    BoardPlayerMotBlendSet(player, 0, turn_frames);
    while (!BoardPlayerMotBlendCheck(player)) {
        if (rotate_host != 0) {
            target_angle = host_angle_step + BoardModelRotYGet(host_model);
            BoardModelRotYSet(host_model, target_angle);
        }
        HuPrcVSleep();
    }
    BoardModelRotYSet(host_model, 0.0f);
    BoardModelMotionShiftSet(host_model, 1, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    if (!_CheckFlag(FLAG_ID_MAKE(1, 22))) {
        BoardCameraViewSet(1);
        BoardCameraTargetPlayerSet(player);
        BoardCameraMotionWait();
    }
    HuPrcEnd();
}

/**
 * @brief Releases star-host motions and stops star-award effects.
 */
static void DestroyStar(void) {
    s32 i;

    _ClearFlag(0x10017);
    for (i = 0; i < 2; i++) {
        BoardModelMotionKill(BoardStarHostMdlGet(), hostMot[i]);
        hostMot[i] = -1;
    }
    StopGiveStar();
    KillGiveStarEffect();
    starProc = NULL;
}

/**
 * @brief Updates or destroys the animated star-award object.
 * @param arg0 Object controlling the awarded star model and animation state.
 */
static void GiveStarMain(OMOBJ *arg0) {
    GiveStarWork *star_work;

    star_work = omObjGetWork(arg0, GiveStarWork);
    if (star_work->kill != 0 || BoardIsKill()) {
        KillGiveStarEffect();
        BoardModelKill(star_work->star_model);
        giveStarObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    if (star_work->delay != 0) {
        star_work->delay--;
    } else {
        switch (star_work->state) {
            case 4:
                break;
            case 0:
                InitGiveStar(star_work, arg0);
                break;
            case 1:
                MoveGiveStar(star_work, arg0);
                break;
            case 2:
                ShrinkGiveStar(star_work, arg0);
                break;
        }
        UpdateStarAngle(star_work, arg0);
    }
    BoardModelPosSet(star_work->star_model, arg0->trans.x, arg0->trans.y, arg0->trans.z);
    BoardModelRotSet(star_work->star_model, 0.0f, arg0->rot.y, 0.0f);
    BoardModelScaleSet(star_work->star_model, arg0->scale.x, arg0->scale.y, arg0->scale.z);
    if (starParman != -1) {
        Hu3DParManPosSet(starParman, arg0->trans.x, -70.0f + arg0->trans.y, arg0->trans.z);
    }
}

/**
 * @brief Accelerates the awarded star's rotation when spinning is enabled.
 * @param arg0 Mutable star-award state.
 * @param arg1 Star object whose Y rotation is updated.
 */
static void UpdateStarAngle(GiveStarWork *arg0, OMOBJ *arg1) {
    float spin_speed;

    if (arg0->spin != 0) {
        if (arg0->spin_speed < 16.0f) {
            arg0->spin_speed += 2.0f;
            if (arg0->spin_speed > 16.0f) {
                arg0->spin_speed = 16;
            }
        }
        OSs8tof32(&arg0->spin_speed, &spin_speed);
        arg1->rot.y = BoardDAngleCalc(arg1->rot.y + spin_speed);
    }
}

/**
 * @brief Reveals the star and initializes its path from the host toward the player.
 * @param arg0 Mutable star-award state.
 * @param arg1 Star object initialized at the source position.
 */
static void InitGiveStar(GiveStarWork *arg0, OMOBJ *arg1) {
    Vec player_position;

    BoardPlayerPosGet(arg0->player, &player_position);
    BoardModelVisibilitySet(arg0->star_model, 1);
    BoardModelMotionStart(arg0->star_model, 0, 0x40000001);
    BoardModelMotionTimeSet(arg0->star_model, 2.0f);
    arg0->state = 1;
    arg0->arc_frame = 0;
    arg1->trans.x = starCharPos.x;
    arg1->trans.y = starCharPos.y;
    arg1->trans.z = starCharPos.z;
    arg1->rot.x = (player_position.x - starCharPos.x) / 10.0f;
    arg1->rot.z = (player_position.z - starCharPos.z) / 10.0f;
    arg1->rot.y = 0.0f;
    arg1->scale.x = arg1->scale.y = arg1->scale.z = 1.0f;
    InitGiveStarEffect();
}

/**
 * @brief Moves the awarded star along its initial arc toward the player.
 * @param arg0 Mutable star-award state.
 * @param arg1 Star object moved toward the player.
 */
static void MoveGiveStar(GiveStarWork *arg0, OMOBJ *arg1) {
    Vec player_position;
    float arc_frame;

    BoardPlayerPosGet(arg0->player, &player_position);
    if (arg0->motion_looped == 0) {
        if (BoardModelMotionTimeGet(arg0->star_model) >= 50.0f) {
            BoardModelMotionStartEndSet(arg0->star_model, 50, 150);
            arg0->motion_looped = 1;
        }
    }
    if (arg0->arc_frame < 10) {
        OSs16tof32(&arg0->arc_frame, &arc_frame);
        arg1->trans.x += arg1->rot.x;
        arg1->trans.z += arg1->rot.z;
        arg1->trans.y += -0.08166667f * arc_frame * arc_frame * 0.97f + 50.0f;
        arg0->arc_frame++;
    } else {
        arg1->trans.y += -4.0f;
        if (arg1->trans.y <= player_position.y + 300.0f) {
            arg1->trans.y = player_position.y + 300.0f;
            arg0->state = 4;
        }
    }
}

/**
 * @brief Lowers and shrinks the awarded star into the player.
 * @param arg0 Mutable star-award state.
 * @param arg1 Star object lowered and scaled into the player.
 */
static void ShrinkGiveStar(GiveStarWork *arg0, OMOBJ *arg1) {
    Vec player_position;
    float shrink_angle;

    if (arg0->sound_played == 0) {
        HuAudFXPlay(0x339);
        arg0->sound_played = 1;
    }
    BoardPlayerPosGet(arg0->player, &player_position);
    arg1->trans.y += -4.0f;
    if (arg1->trans.y > player_position.y + 80.0f) {
        if (arg0->shrink_angle < 90) {
            if (BoardPlayerSizeGet(arg0->player) != 1) {
                arg0->shrink_angle += 2;
            } else {
                arg0->shrink_angle += 1.4f;
            }
            if (arg0->shrink_angle > 90) {
                arg0->shrink_angle = 90;
            }
        }
        OSs16tof32(&arg0->shrink_angle, &shrink_angle);
        arg1->scale.x = sind(shrink_angle + 90.0f);
        if (arg1->scale.x <= 0.0f) {
            arg1->scale.x = 0.001f;
        }
        arg1->scale.y = arg1->scale.x;
        arg1->scale.z = arg1->scale.x;
    } else {
        omVibrate(arg0->player, 12, 0xC, 0);
        arg0->kill = 1;
        arg0->state = 4;
        arg1->scale.x = arg1->scale.y = arg1->scale.z = 0.001f;
        KillGiveStarEffect();
        BoardModelVisibilitySet(arg0->star_model, 0);
    }
}

/**
 * @brief Marks the active star-award object for destruction.
 */
static void StopGiveStar(void) {
    GiveStarWork *star_work;

    if (giveStarObj) {
        star_work = omObjGetWork(giveStarObj, GiveStarWork);
        star_work->kill = 1;
    }
}

/**
 * @brief Creates the particle trail used by the star-award animation.
 */
static void InitGiveStarEffect(void) {
    s16 particle_model;
    void *effect_data;

    effect_data = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 1), HU_MEMNUM_OVL, HEAP_MODEL);
    starEffAnim = HuSprAnimRead(effect_data);
    starParman = Hu3DParManCreate(starEffAnim, 0x64, &starEffParam);
    Hu3DParManAttrSet(starParman, 0x64);
    Hu3DParManRotSet(starParman, 90.0f, 0.0f, 0.0f);
    particle_model = Hu3DParManModelIDGet(starParman);
    Hu3DParticleBlendModeSet(particle_model, 1);
    Hu3DModelLayerSet(particle_model, 2);
    HuDataDirClose(DATADIR_EFFECT);
}

/**
 * @brief Destroys the star-award particle trail if it exists.
 */
static void KillGiveStarEffect(void) {
    if (starParman != -1) {
        Hu3DParManKill(starParman);
        starParman = -1;
        starEffAnim = NULL;
    }
}

/**
 * @brief Starts the star-award object's initial movement state.
 */
static inline void StarInlineFunc00(void) {
    GiveStarWork *star_work = omObjGetWork(giveStarObj, GiveStarWork);

    star_work->state = 0;
}

/**
 * @brief Waits for the active star-award movement state to finish.
 */
static inline void StarInlineFunc01(void) {
    GiveStarWork *star_work = omObjGetWork(giveStarObj, GiveStarWork);

    while (star_work->state != 4) {
        HuPrcVSleep();
    }
}

/**
 * @brief Starts the spinning shrink phase of the star-award animation.
 */
static inline void StarInlineFunc02(void) {
    GiveStarWork *star_work = omObjGetWork(giveStarObj, GiveStarWork);

    star_work->spin = 1;
    star_work->delay = 45;
    star_work->shrink_angle = 0;
    star_work->state = 2;
}

/**
 * @brief Animates a star from a source position and credits it to a player.
 * @param arg0 Player index receiving the star.
 * @param arg1 World-space position from which the star animation begins.
 */
void BoardStarGive(s32 arg0, Vec *arg1) {
    Vec camera_rotation;
    s32 stream;
    GiveStarWork *star_work;

    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        return;
    }
    starCharPos = *arg1;
    giveStarObj = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, GiveStarMain);
    BoardCameraRotGet(&camera_rotation);
    star_work = omObjGetWork(giveStarObj, GiveStarWork);
    star_work->kill = 0;
    star_work->spin = 0;
    star_work->motion_looped = 0;
    star_work->sound_played = 0;
    star_work->state = 4;
    star_work->player = arg0;
    star_work->write_only_field = 0;
    star_work->spin_speed = 0;
    star_work->shrink_angle = 0;
    star_work->star_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 11), NULL, 0);
    BoardModelVisibilitySet(star_work->star_model, 0);
    BoardModelPassSet(star_work->star_model, 0);
    BoardModelMotionSpeedSet(star_work->star_model, 0.0f);
    BoardModelLayerSet(star_work->star_model, 2);
    StarInlineFunc00();
    HuAudFXPlay(0x338);
    HuAudFXPlay(0x353);
    StarInlineFunc01();
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        BoardTutorialHookExec(0x15, 0);
    }
    BoardPlayerMotBlendSet(arg0, 0, 0xF);
    while (!BoardPlayerMotBlendCheck(arg0)) {
        HuPrcVSleep();
    }
    StarInlineFunc02();
    StarInlineFunc01();
    BoardPlayerVoiceEnableSet(arg0, 7, 0);
    BoardPlayerMotionShiftSet(arg0, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuAudFXPlay(boardStarSndTbl[GWPlayer[arg0].character]);
    BoardPlayerStarsAdd(arg0, 1);
    HuAudFXPlay(8);
    HuPrcSleep(10);
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        BoardPlayerMotionEndWait(arg0);
    } else {
        stream = HuAudSStreamPlay(6);
        BoardPlayerMotionEndWait(arg0);
        while (msmStreamGetStatus(stream) != 0) {
            HuPrcVSleep();
        }
    }
    BoardPlayerVoiceEnableSet(arg0, 7, 1);
}

/**
 * @brief Moves the star, presents its new location, and restores the board view.
 * @param arg0 Player index used as the restored camera target; -1 selects a non-turn presentation path.
 */
void BoardStarShowNext(s32 arg0) {
    Mtx host_transform;
    Mtx camera_pitch_transform;
    Mtx host_yaw_transform;
    Vec host_position;
    Vec overlay_position;
    Vec player_position;
    Vec camera_rotation;
    Vec view_offset;
    float view_distance;
    WipeState *wipe;
    s32 initial_display;
    s32 message;
    s16 view_frames;
    s16 player_index;

    initial_display = 0;
    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        return;
    }
    _SetFlag(0x1001C);
    wipe = &wipeData;
    if (wipe->mode == 0) {
        WipeColorSet(0, 0, 0);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 21);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
    } else {
        HuPrcSleep(18);
    }
    BoardStatusItemSet(0);
    if (GWBoardGet() == BOARD_ID_MAIN6 && BoardItemPrevGet() != 9 && boardStarGiveHook) {
        boardStarGiveHook();
    }
    BoardSpaceStarMove();
    if (arg0 == -1) {
        initial_display = 1;
        arg0 = 0;
        for (player_index = 0; player_index < 4; player_index++) {
            BoardPlayerCornerPosSet(player_index);
        }
        BoardSpacePosGet(0, GWPlayer[arg0].space_curr, &player_position);
        BoardPlayerPosSetV(arg0, &player_position);
    }
    if (!_CheckFlag(FLAG_ID_MAKE(1, 23))) {
        BoardAudSeqFadeOut(0, 1000);
    }
    showNextObj = omAddObjEx(boardObjMan, 0x7E03, 0, 0, -1, ShowNextUpdate);
    BoardPlayerIdleSet(arg0);
    BoardModelMotionStart(BoardStarHostMdlGet(), 1, 0x40000001);
    BoardModelPosGet(BoardStarHostMdlGet(), &host_position);
    showNextObj->scale.x = host_position.x;
    showNextObj->scale.y = host_position.y;
    showNextObj->scale.z = host_position.z;
    BoardPlayerPosGet(arg0, &player_position);
    host_position.y = player_position.y;
    VECSubtract(&host_position, &player_position, &view_offset);
    view_offset.y = 0.0f;
    view_distance = VECMag(&view_offset);
    if (view_distance > 3000.0f) {
        view_frames = 0xF0;
    } else if (view_distance > 1500.0f) {
        view_frames = 0x78;
    } else {
        view_frames = 0x78;
    }
    BoardViewMoveStart(&player_position, &host_position, view_frames);
    showNextObj->trans.x = 68.0f;
    showNextObj->trans.y = HU_DISP_HEIGHT-50;
    showNextObj->trans.z = 100.0f;
    overlay_position.x = showNextObj->trans.x;
    overlay_position.y = showNextObj->trans.y;
    overlay_position.z = showNextObj->trans.z;
    Hu3D2Dto3D(&overlay_position, 1, &overlay_position);
    BoardModelPosSetV(BoardStarHostMdlGet(), &overlay_position);
    BoardCameraRotGet(&camera_rotation);
    PSMTXRotRad(host_yaw_transform, 'y', MTXDegToRad(10.0f));
    PSMTXRotRad(camera_pitch_transform, 'x', MTXDegToRad(camera_rotation.x));
    PSMTXConcat(camera_pitch_transform, host_yaw_transform, host_transform);
    BoardModelMtxSet(BoardStarHostMdlGet(), &host_transform);
    BoardModelRotSet(BoardStarHostMdlGet(), 0.0f, 0.0f, 0.0f);
    BoardModelScaleSet(BoardStarHostMdlGet(), 0.09f, 0.09f, 0.09f);
    BoardMusStart(1, 7, 0x7F, 0);
    BoardCameraMoveSet(0);
    BoardCameraMotionWait();
    BoardCameraMoveSet(1);
    HuPrcSleep(1);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 21);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    if (initial_display == 0) {
        message = starMesTbl1[GWBoardGet()] + 6;
    } else {
        message = starMesTbl2[GWBoardGet()][0];
    }
    BoardWinCreate(3, message, -1);
    BoardWinPause();
    BoardViewMoveEnd();
    while (!BoardViewMoveCheck()) {
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    BoardWinKill();
    HuPrcSleep(3);
    if (initial_display == 0) {
        message = starMesTbl1[GWBoardGet()] + 7;
    } else {
        message = starMesTbl2[GWBoardGet()][1];
    }
    BoardWinCreate(3, message, -1);
    BoardWinWait();
    BoardWinKill();
    if (GWBoardGet() == BOARD_ID_MAIN6 && BoardItemPrevGet() != 9 && boardStarShowNextHook) {
        boardStarShowNextHook();
    }
    BoardAudSeqFadeOut(1, 1000);
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 21);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    BoardViewWait();
    PSMTXIdentity(host_transform);
    BoardModelMtxSet(BoardStarHostMdlGet(), &host_transform);
    HideNextHost(1);
    while (showNextObj) {
        HuPrcVSleep();
    }
    BoardCameraMoveSet(0);
    BoardCameraTargetPlayerSet(arg0);
    BoardCameraMotionWait();
    BoardCameraMoveSet(1);
    BoardStatusItemSet(1);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 21);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    if (!_CheckFlag(FLAG_ID_MAKE(1, 23))) {
        BoardMusStartBoard();
    }
    _ClearFlag(FLAG_ID_MAKE(1, 28));
}

/**
 * @brief Moves the star host between its overlay and board-world positions.
 * @param arg0 Object controlling the star host's temporary presentation position.
 */
static void ShowNextUpdate(OMOBJ *arg0) {
    ShowNextWork *show_next_work = omObjGetWork(arg0, ShowNextWork);
    Vec host_position;

    if (show_next_work->kill != 0 || BoardIsKill()) {
        showNextObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    switch (show_next_work->state) {
        case 0:
            host_position.x = arg0->trans.x;
            host_position.y = arg0->trans.y;
            host_position.z = arg0->trans.z;
            Hu3D2Dto3D(&host_position, 1, &host_position);
            break;
        case 1:
            host_position.x = arg0->scale.x;
            host_position.y = arg0->scale.y;
            host_position.z = arg0->scale.z;
            BoardModelLayerSet(BoardStarHostMdlGet(), 0);
            BoardModelRotSet(BoardStarHostMdlGet(), 0.0f, 0.0f, 0.0f);
            BoardModelScaleSet(BoardStarHostMdlGet(), 1.0f, 1.0f, 1.0f);
            show_next_work->kill = 1;
            break;
    }
    BoardModelPosSetV(BoardStarHostMdlGet(), &host_position);
}

/**
 * @brief Requests restoration of the star host to its board-world position.
 * @param arg0 Host display state; 1 restores the saved board-world transform.
 */
static void HideNextHost(s32 arg0) {
    ShowNextWork *show_next_work = omObjGetWork(showNextObj, ShowNextWork);

    show_next_work->state = arg0;
}
