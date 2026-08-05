#include "types.h"
#include "game/board/item.h"
#include "game/audio.h"
#include "game/board/audio.h"
#include "game/board/boo.h"
#include "game/board/char_wheel.h"
#include "game/board/com.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/star.h"
#include "game/board/ui.h"
#include "game/board/view.h"
#include "game/board/window.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/frand.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#include "ext_math.h"

#include "game/msm.h"

#ifndef __MWERKS__
#include "game/hsfex.h"
extern s32 rand8(void);
#endif

typedef struct {
    /* 0x00 */ s16 paused;
    /* 0x02 */ s16 model;
    /* 0x04 */ float bob_amplitude;
    /* 0x08 */ float bob_speed;
    /* 0x0C */ float bob_angle;
    /* 0x10 */ Vec rotation_step;
} UnkItemProcUserData; // Size unknown

typedef struct {
    /* 0x00 */ s16 paused;
    /* 0x02 */ s16 model;
    /* 0x04 */ float bob_amplitude;
    /* 0x08 */ float bob_speed;
    /* 0x0C */ char padding[4];
    /* 0x10 */ float rotation_step_x;
    /* 0x14 */ float rotation_step_y;
    /* 0x18 */ float rotation_step_z;
    /* 0x1C */ s16 show_smoke;
} UnkItemShowProcStruct; // Size unknown

typedef struct {
    /* 0x00 */ float duration;
    /* 0x04 */ Vec position;
    /* 0x10 */ Vec target;
} UnkGenieCameraStruct; // Size 0x1C

static void ItemProcess(void);
static void RestoreProcess(void);
static void ItemDestroy(void);
static void RestoreDestroy(void);
static void ItemSizeShowAnim(void);
static void ItemRotProc(void);
static HUPROCESS *ItemShowProc(UnkItemShowProcStruct *arg0, Vec *arg1);
static s16 ItemGetTarget(void);
static void ItemSizeSet(s32 arg0);
static void ExecItemMini(void);
static void ExecItemMega(void);
static void ExecItemMiniSuper(void);
static void ExecItemMegaSuper(void);
static void ExecItemHammer(void);
static void ExecItemPipe(void);
static void ExecItemSwap(void);
static void ExecItemSpark(void);
static void ExecItemLight(void);
static void ExecItemWhistle(void);
static void ExecItemBowser(void);
static void ExecItemBooBall(void);
static void LampParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
static void GenieParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
static void GenieCameraProc(void);
static void GenieCameraCalc(UnkGenieCameraStruct *arg0, s32 arg1, float arg2, Vec *arg3, Vec *arg4);
static void GenieSceneExec(void);
static void ExecItemGenie(void);
static void ExecItemBagJump(void);
static void ExecItemBagShow(void);
static void ExecItemBag(void);
static void RestoreItemNull(void);
static void RestoreItemMini(void);
static void RestoreItemMega(void);
static void RestoreItemMiniSuper(void);
static void RestoreItemMegaSuper(void);
static void RestoreItemBowser(void);

static Vec booBallScale;
static Vec booBallPos;
static Vec booCamUp;
static Vec booCamTarget;
static Vec booCamPos;

static s16 itemBagItems[3];
static s8 currItemRestore;
static s8 currItem;
static s16 suitMdl;
static s32 suitMot[2];
static s32 suitCommonMot[2];
static s16 booBallMdl;
static float booBallAlpha;
static char booCoinStr[8];
static float genieFov;
static ANIMDATA *genieParticleAnim;
static s16 geniePlayerMot[3];
static HUPROCESS *itemProc;

static s32 itemMotTbl[2] = { DATA_MAKE_NUM(DATADIR_BOARD, 117), -1 };

static void (*itemFuncTbl[])(void) = { ExecItemMini, ExecItemMega, ExecItemMiniSuper, ExecItemMegaSuper, ExecItemHammer, ExecItemPipe, ExecItemSwap,
    ExecItemSpark, ExecItemLight, ExecItemWhistle, ExecItemBowser, ExecItemBooBall, ExecItemGenie, ExecItemBag };

static void (*itemRestoreFuncTbl[])(void)
    = { RestoreItemMini, RestoreItemMega, RestoreItemMiniSuper, RestoreItemMegaSuper, RestoreItemNull, RestoreItemNull, RestoreItemNull,
          RestoreItemNull, RestoreItemNull, RestoreItemNull, RestoreItemBowser, RestoreItemNull, RestoreItemNull, RestoreItemNull };

static s32 itemMdlTbl[] = { DATA_MAKE_NUM(DATADIR_BOARD, 109), DATA_MAKE_NUM(DATADIR_BOARD, 110), DATA_MAKE_NUM(DATADIR_BOARD, 111),
    DATA_MAKE_NUM(DATADIR_BOARD, 112), DATA_MAKE_NUM(DATADIR_BOARD, 113), DATA_MAKE_NUM(DATADIR_BOARD, 114), DATA_MAKE_NUM(DATADIR_BOARD, 115),
    DATA_MAKE_NUM(DATADIR_BOARD, 116), DATA_MAKE_NUM(DATADIR_BOARD, 118), DATA_MAKE_NUM(DATADIR_BOARD, 119), DATA_MAKE_NUM(DATADIR_BOARD, 120),
    DATA_MAKE_NUM(DATADIR_BOARD, 121), DATA_MAKE_NUM(DATADIR_BOARD, 122), DATA_MAKE_NUM(DATADIR_BOARD, 123) };

static s32 forceMoveSfxTbl[] = { 0x38, 0x45, 0x42, 0x4D, 0x48, 0x3F };

static s32 callMotTbl[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 82), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 82), DATA_MAKE_NUM(DATADIR_PEACHMOT, 82),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 82), DATA_MAKE_NUM(DATADIR_WARIOMOT, 82), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 82),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 82), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 82) };

static char *callHookTbl[] = { "c000m01-itemhook-r", "c001m01-itemhook-r", "c002m01-itemhook-r", "c003m01-itemhook-r", "c004m01-itemhook-r",
    "c005m01-itemhook-r", "c006m01-itemhook-r", "c007m01-itemhook-r" };

static s32 callAttackMotTbl[] = { DATA_MAKE_NUM(DATADIR_W01, 31), DATA_MAKE_NUM(DATADIR_W02, 5), DATA_MAKE_NUM(DATADIR_W03, 29),
    DATA_MAKE_NUM(DATADIR_W04, 9), DATA_MAKE_NUM(DATADIR_W05, 7), DATA_MAKE_NUM(DATADIR_W06, 19) };

static s32 callAttackMotTbl2[] = { DATA_MAKE_NUM(DATADIR_W01, 32), DATA_MAKE_NUM(DATADIR_W02, 6), DATA_MAKE_NUM(DATADIR_W03, 30),
    DATA_MAKE_NUM(DATADIR_W04, 10), DATA_MAKE_NUM(DATADIR_W05, 8), DATA_MAKE_NUM(DATADIR_W06, 20) };

static s32 suitMotTbl[][2] = { { DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 1), DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 2) },
    { DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 4), DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 5) },
    { DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 7), DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 8) },
    { DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 10), DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 11) },
    { DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 13), DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 14) },
    { DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 16), DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 17) },
    { DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 19), DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 20) },
    { DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 22), DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 23) } };

/** @brief Start the use animation for a player's board item. @param arg0 Player using the item. @param arg1 Item index into the board item behavior table. */
void BoardItemStart(s32 arg0, s32 arg1)
{
    currItemRestore = arg0;
    currItem = arg1;
    suitMdl = -1;
    itemProc = HuPrcChildCreate(ItemProcess, 0x2004, 0x1000, 0, boardMainProc);
    HuPrcDestructorSet2(itemProc, ItemDestroy);
}

/** @brief Check whether item use or restoration has finished. @return TRUE when no item process is active. */
BOOL BoardItemDoneCheck(void)
{
    if (itemProc) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

/** @brief Restore a player's persistent size or Bowser-suit state. @param arg0 Player to restore. @param arg1 Previous item index selecting the restoration routine. */
void BoardItemPlayerRestore(s32 arg0, s32 arg1)
{
    currItemRestore = arg0;
    currItem = arg1;
    itemProc = HuPrcChildCreate(RestoreProcess, 0x2004, 0x1000, 0, boardMainProc);
    HuPrcDestructorSet2(itemProc, RestoreDestroy);
}

/** @brief Run the selected item's child process and end after it exits. */
static void ItemProcess(void)
{
    void (*item_function)(void) = itemFuncTbl[currItem];

    HuPrcChildCreate(item_function, 0x2003, 0x1000, 0, HuPrcCurrentGet());
    HuPrcChildWatch();
    HuPrcEnd();
}

/** @brief Run the selected item-restoration child process and wait for it. */
static void RestoreProcess(void)
{
    void (*restore_function)(void) = itemRestoreFuncTbl[currItem];

    HuPrcChildCreate(restore_function, 0x2003, 0x1000, 0, HuPrcCurrentGet());
    HuPrcChildWatch();
    HuPrcEnd();
}

/** @brief Mark normal item execution as complete when its process is destroyed. */
static void ItemDestroy(void)
{
    itemProc = NULL;
}

/** @brief Mark item-state restoration as complete when its process is destroyed. */
static void RestoreDestroy(void)
{
    itemProc = NULL;
}

/** @brief Float the size-item model above the player, shrink it, and destroy it. */
static void ItemSizeShowAnim(void)
{
    Vec item_position;
    Vec item_scale;
    Vec player_rotation;
    float unused_initial_scale;
    float bob_angle;
    s32 i;

    unused_initial_scale = 1.0f;
    bob_angle = 180.0f;
    BoardPlayerPosGet(currItemRestore, &item_position);
    BoardPlayerRotGet(currItemRestore, &player_rotation);
    item_position.y += 300.0f;
    item_scale.x = item_scale.y = item_scale.z = 1.0f;
    BoardModelRotSetV(suitMdl, &player_rotation);
    BoardModelPosSetV(suitMdl, &item_position);
    for (i = 0; i < 120; i++) {
        item_position.y += 3.0 * sind(bob_angle);
        BoardModelPosSetV(suitMdl, &item_position);
        bob_angle += 9.0f;
        if (bob_angle >= 360.0f) {
            bob_angle -= 360.0f;
        }
        HuPrcVSleep();
    }
    for (i = 0; i < 30; i++) {
        item_position.y -= 6.6666665f;
        item_scale.x -= 0.03f;
        item_scale.y -= 0.03f;
        item_scale.z -= 0.03f;
        BoardModelScaleSetV(suitMdl, &item_scale);
        BoardModelPosSetV(suitMdl, &item_position);
        HuPrcVSleep();
    }
    BoardModelKill(suitMdl);
}

/** @brief Bob and rotate an item model until its process is killed. */
static void ItemRotProc(void)
{
    Vec base_position;
    Vec item_position;
    Vec item_rotation;
    UnkItemProcUserData *work;

    work = HuPrcCurrentGet()->user_data;
    BoardModelPosGet(work->model, &base_position);
    BoardModelRotGet(work->model, &item_rotation);
    work->bob_angle = 0.0f;
    while (1) {
        if (work->paused == 0) {
            item_position = base_position;
            item_position.y += work->bob_amplitude * sind(work->bob_angle);
            BoardModelPosSetV(work->model, &item_position);
            work->bob_angle += work->bob_speed;
            if (work->bob_angle >= 360.0f) {
                work->bob_angle -= 360.0f;
            }
            VECAdd(&item_rotation, &work->rotation_step, &item_rotation);
            if (item_rotation.x < 0.0f) {
                item_rotation.x += 360.0f;
            }
            if (item_rotation.x >= 360.0f) {
                item_rotation.x -= 360.0f;
            }
            if (item_rotation.y < 0.0f) {
                item_rotation.y += 360.0f;
            }
            if (item_rotation.y >= 360.0f) {
                item_rotation.y -= 360.0f;
            }
            if (item_rotation.z < 0.0f) {
                item_rotation.z += 360.0f;
            }
            if (item_rotation.z >= 360.0f) {
                item_rotation.z -= 360.0f;
            }
            BoardModelRotSetV(work->model, &item_rotation);
        }
        HuPrcVSleep();
    }
}

// TODO: needs the inline keyword to be inlined into ExecItemHammer, but adding it messes up sdata2.
// sdata2 order should be: 1000.0f, 0.15f, then 20.0f.
// With the inline keyword, it becomes 0.15f, 20.0f, then 1000.0f (workaround in BoardUiInlineFunc02 below).
/** @brief Drop the item model to its display height while applying angular velocity. @param arg0 Per-frame rotation in degrees on each axis. */
static inline void BoardUiInlineFunc01(Vec *arg0)
{
    Vec player_position;
    Vec item_position;
    Vec item_rotation;
    Vec item_scale;
    float unused_initial_scale;
    float fall_speed;
    s32 i;

    unused_initial_scale = 1.0f;
    BoardPlayerPosGet(currItemRestore, &player_position);
    BoardPlayerRotGet(currItemRestore, &item_rotation);
    item_position = player_position;
    item_position.y += 1000.0f;
    item_scale.x = item_scale.y = item_scale.z = 1.0f;
    BoardModelRotSetV(suitMdl, &item_rotation);
    BoardModelPosSetV(suitMdl, &item_position);
    while (1) {
        fall_speed = 0.15f * (item_position.y - (player_position.y + 300.0f));
        if (fall_speed > 20.0f) {
            fall_speed = 20.0f;
        }
        if (fall_speed < 1.0f) {
            fall_speed = 1.0f;
        }
        if (fabs(item_position.y - (player_position.y + 300.0f)) < fall_speed) {
            break;
        }
        item_position.y -= fall_speed;
        BoardModelPosSetV(suitMdl, &item_position);
        VECAdd(&item_rotation, arg0, &item_rotation);
        for (i = 0; i < 3; i++) {
            if (((float *)&item_rotation)[i] < 0.0f) {
                ((float *)&item_rotation)[i] += 360.0f;
            }
            if (((float *)&item_rotation)[i] >= 360.0f) {
                ((float *)&item_rotation)[i] -= 360.0f;
            }
        }
        BoardModelRotSetV(suitMdl, &item_rotation);
        HuPrcVSleep();
    }
    item_position.y = player_position.y + 300.0f;
    BoardModelPosSetV(suitMdl, &item_position);
}

/** @brief Raise the displayed item model to a world Y coordinate of 1000. */
static void BoardUiInlineFunc02(void)
{
    Vec player_position;
    Vec item_position;
    float unused_initial_scale;
    float rise_speed;

    (void)1000.0f; // TODO: fix this.
    unused_initial_scale = 1.0f;
    BoardModelPosGet(suitMdl, &item_position);
    BoardPlayerPosGet(currItemRestore, &player_position);
    while (1) {
        rise_speed = 0.15f * (item_position.y - player_position.y - 300.0f);
        if (rise_speed > 20.0f) {
            rise_speed = 20.0f;
        }
        if (rise_speed < 1.0f) {
            rise_speed = 1.0f;
        }
        if (fabs(item_position.y - 1000.0f) <= rise_speed) {
            break;
        }
        item_position.y += rise_speed;
        BoardModelPosSetV(suitMdl, &item_position);
        HuPrcVSleep();
    }
    item_position.y = 1000.0f;
    BoardModelPosSetV(suitMdl, &item_position);
}

/** @brief Move a player back to the center of their current space and return them to idle. @param arg0 Player to reposition. */
static inline void BoardUiInlineFunc03(s32 arg0)
{
    Vec player_position;
    Vec space_position;
    s32 space = GWPlayer[arg0].space_curr;

    BoardSpacePosGet(0, space, &space_position);
    BoardPlayerPosGet(arg0, &player_position);
    BoardPlayerPosLerpStart(arg0, &player_position, &space_position, 0x14);
    while (GWPlayer[arg0].moving) {
        HuPrcVSleep();
    }
    BoardPlayerMotBlendSet(arg0, 0, 7);
    while (!BoardPlayerMotBlendCheck(arg0)) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(arg0);
}

/** @brief Pause or resume an item model's bob-and-rotate process. @param arg0 Item rotation process. @param arg1 Zero to animate; nonzero to pause. */
static inline void BoardUiInlineFunc04(HUPROCESS *arg0, s32 arg1)
{
    UnkItemShowProcStruct *work = arg0->user_data;

    work->paused = arg1;
}

/** @brief Start the bob-and-rotate process for a displayed item. @param arg0 Persistent animation work owned by the caller. @return New process using @p arg0 as user data. */
static inline HUPROCESS *BoardUiInlineFunc05(UnkItemShowProcStruct *arg0)
{
    HUPROCESS *process;

    process = HuPrcCreate(ItemRotProc, 0x2004, 0x1000, 0);
    process->user_data = arg0;
    BoardUiInlineFunc04(process, 0);
    return process;
}

/**
 * @brief Reveal the current item above its user and optionally animate it.
 * @param arg0 Animation work that remains caller-owned, or NULL for a static model.
 * @param arg1 Model scale, or NULL for unit scale.
 * @return Bob-and-rotate process for the model, or NULL when no process was started.
 */
static HUPROCESS *ItemShowProc(UnkItemShowProcStruct *arg0, Vec *arg1)
{
    Vec item_position;
    Vec rotation;
    Vec item_scale;
    HUPROCESS *rotation_process;

    rotation_process = NULL;
    BoardPlayerPosGet(currItemRestore, &item_position);
    BoardPlayerRotGet(currItemRestore, &rotation);
    item_position.y += 300.0f;
    if (currItem == 0xD) {
        item_position.y -= 50.0f;
    }
    if (arg1 == NULL) {
        item_scale.x = item_scale.y = item_scale.z = 1.0f;
    }
    else {
        item_scale = *arg1;
    }
    if (suitMdl >= 0) {
        BoardModelLayerSet(suitMdl, 2);
        BoardModelRotSetV(suitMdl, &rotation);
        BoardModelPosSetV(suitMdl, &item_position);
        BoardModelScaleSetV(suitMdl, &item_scale);
        BoardModelVisibilitySet(suitMdl, 0);
    }
    BoardCameraRotGet(&rotation);
    if (arg0 == NULL || arg0->show_smoke != 0) {
        CharEffectLayerSet(3);
        item_position.y += 35.0f;
        item_position.z += 50.0 * cosd(rotation.y);
        item_position.x += 50.0 * sind(rotation.y);
        CharEffectSmokeCreate(1, &item_position);
        HuPrcSleep(10);
    }
    if (suitMdl >= 0) {
        BoardModelVisibilitySet(suitMdl, 1);
        if (arg0 != NULL) {
            rotation_process = BoardUiInlineFunc05(arg0);
        }
    }
    HuPrcSleep(30);
    return rotation_process;
}

/**
 * @brief Let the current player or CPU choose one of the other three players.
 * @return Selected opponent's player index.
 */
static s16 ItemGetTarget(void)
{
    s32 target_players[4];
    s32 com_keys[4];
    float window_size[2];
    s16 selected_player;
    s16 window;
    s32 message_delay;
    s32 insert_index;
    s32 com_move_count;
    s32 target_count;
    u8 disabled_pads;
    s32 com_move;
    s32 port;
    s32 player;
    s32 com_port;

    insert_index = 0;
    target_count = 0;
    for (player = 0; player < 4; player++) {
        if (player != currItemRestore) {
            target_players[target_count++] = player;
            HuWinInsertMesSizeGet(GWPlayerCfg[player].character, insert_index);
            insert_index++;
        }
    }
    HuWinMesMaxSizeGet(1, window_size, MAKE_MESSID(18, 14));
    window = HuWinCreate(-10000.0f, -10000.0f, window_size[0], window_size[1], 0);
    if (!GWPlayer[currItemRestore].com) {
        winData[window].active_pad = (1 << GWPlayer[currItemRestore].port);
    }
    for (player = 0; player < target_count; player++) {
        HuWinInsertMesSet(window, GWPlayerCfg[target_players[player]].character, player);
    }
    HuWinMesSet(window, MAKE_MESSID(18, 14));
    if (GWPlayer[currItemRestore].com) {
        disabled_pads = 0xF;
    }
    else {
        disabled_pads = 0;
        for (player = 0; player < 4; player++) {
            if (player != currItemRestore) {
                disabled_pads = (1 << GWPlayer[player].port);
            }
        }
    }
    HuWinDisablePlayerSet(window, disabled_pads);
    if (GWPlayer[currItemRestore].com) {
        com_port = GWPlayer[currItemRestore].port;
        com_move_count = frandmod(3);
        message_delay = GWMessDelayGet();
        for (com_move = 0; com_move < com_move_count; com_move++) {
            for (port = 0; port < 4; port++) {
                com_keys[port] = 0;
            }
            com_keys[com_port] = 4;
            HuWinComKeyWait(com_keys[0], com_keys[1], com_keys[2], com_keys[3], message_delay);
        }
        for (port = 0; port < 4; port++) {
            com_keys[port] = 0;
        }
        com_keys[com_port] = 0x100;
        HuWinComKeyWait(com_keys[0], com_keys[1], com_keys[2], com_keys[3], message_delay);
    }
    HuWinAttrSet(window, 0x10);
    HuWinMesSpeedSet(window, 0);
    selected_player = target_players[HuWinChoiceGet(window, 0)];
    HuWinKill(window);
    return selected_player;
}

/** @brief Animate the player between normal, Mini, and Mega scales. @param arg0 Destination size state: 0 normal, 1 Mini, or 2 Mega. */
static void ItemSizeSet(s32 arg0)
{
    float scale_sequence[9];
    float key_scales[4];
    float size_scales[3] = { 1.0f, 0.3f, 2.5f };
    float current_scale;
    float scale_step;
    s16 model_id;
    s32 sequence_index;
    s16 player_model;
    s32 i;
    s32 j;

    current_scale = size_scales[BoardPlayerSizeGet(currItemRestore)];
    key_scales[0] = current_scale;
    key_scales[1] = current_scale + (size_scales[arg0] - current_scale) / 3.0f;
    key_scales[2] = current_scale + (size_scales[arg0] - current_scale) / 1.5f;
    key_scales[3] = size_scales[arg0];
    scale_sequence[0] = key_scales[1];
    scale_sequence[1] = key_scales[0];
    scale_sequence[2] = key_scales[1];
    scale_sequence[3] = key_scales[2];
    scale_sequence[4] = key_scales[1];
    scale_sequence[5] = key_scales[2];
    scale_sequence[6] = key_scales[3];
    scale_sequence[7] = key_scales[2];
    scale_sequence[8] = key_scales[3];
    player_model = BoardPlayerModelGet(currItemRestore);
    model_id = BoardModelIDGet(player_model);
    Hu3DModelAttrSet(model_id, HU3D_MOTATTR_PAUSE);
    sequence_index = 0;
    for (i = 0; i < 9; i++) {
        scale_step = (scale_sequence[sequence_index++] - current_scale) / 10.0f;
        for (j = 0; j < 10; j++) {
            BoardPlayerScaleSet(currItemRestore, current_scale, current_scale, current_scale);
            current_scale += scale_step;
            HuPrcVSleep();
        }
    }
    Hu3DModelAttrReset(model_id, HU3D_MOTATTR_PAUSE);
    BoardPlayerSizeSet(currItemRestore, arg0);
}

/** @brief Use a Mini Mushroom and shrink the current player. */
static void ExecItemMini(void)
{
    suitMdl = BoardModelCreate(itemMdlTbl[currItem], NULL, 0);
    BoardModelLayerSet(suitMdl, 2);
    HuAudFXPlay(0x350);
    ItemShowProc(NULL, NULL);
    ItemSizeShowAnim();
    HuAudFXPlay(0x314);
    omVibrate(currItemRestore, 90, 6, 6);
    HuAudFXPlay(0x313);
    ItemSizeSet(1);
    HuPrcSleep(60);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Use a Mega Mushroom and enlarge the current player. */
static void ExecItemMega(void)
{
    suitMdl = BoardModelCreate(itemMdlTbl[currItem], NULL, 0);
    BoardModelLayerSet(suitMdl, 2);
    HuAudFXPlay(0x350);
    ItemShowProc(NULL, NULL);
    ItemSizeShowAnim();
    HuAudFXPlay(0x312);
    omVibrate(currItemRestore, 90, 4, 2);
    HuAudFXPlay(0x311);
    ItemSizeSet(2);
    HuPrcSleep(60);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Use a Super Mini Mushroom and shrink the current player. */
static void ExecItemMiniSuper(void)
{
    suitMdl = BoardModelCreate(itemMdlTbl[currItem], NULL, 0);
    BoardModelLayerSet(suitMdl, 2);
    HuAudFXPlay(0x350);
    ItemShowProc(NULL, NULL);
    ItemSizeShowAnim();
    HuAudFXPlay(0x314);
    omVibrate(currItemRestore, 90, 4, 2);
    HuAudFXPlay(0x313);
    ItemSizeSet(1);
    HuPrcSleep(60);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Use a Super Mega Mushroom and enlarge the current player. */
static void ExecItemMegaSuper(void)
{
    suitMdl = BoardModelCreate(itemMdlTbl[currItem], NULL, 0);
    BoardModelLayerSet(suitMdl, 2);
    HuAudFXPlay(0x350);
    ItemShowProc(NULL, NULL);
    ItemSizeShowAnim();
    HuAudFXPlay(0x312);
    omVibrate(currItemRestore, 90, 0xC, 0);
    HuAudFXPlay(0x311);
    ItemSizeSet(2);
    HuPrcSleep(60);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Use the MiniMega Hammer to select another player and randomly change their size. */
static void ExecItemHammer(void)
{
    Vec rotation_step;
    Vec hammer_scale;
    float window_size[2];
    s32 i;
    s16 window;
    s16 target_player;
    s16 com_delay;
    s16 hammer_side;

    HuAudFXPlay(0x350);
    ItemShowProc(NULL, NULL);
    target_player = ItemGetTarget();
    suitMdl = BoardModelCreate(itemMdlTbl[currItem], NULL, 0);
    BoardModelLayerSet(suitMdl, 2);
    hammer_side = frandBool();
    if (hammer_side != 0) {
        BoardModelMotionTimeSet(suitMdl, 1.5f);
    }
    else {
        BoardModelMotionTimeSet(suitMdl, 0.0f);
    }
    BoardModelAttrSet(suitMdl, 0x40000002);
    hammer_scale.x = hammer_scale.y = hammer_scale.z = 2.0f;
    BoardModelScaleSetV(suitMdl, &hammer_scale);
    rotation_step.x = rotation_step.y = rotation_step.z = 0.0f;
    BoardUiInlineFunc01(&rotation_step);
    HuWinMesMaxSizeGet(1, window_size, MAKE_MESSID(18, 17));
    window = HuWinCreate(-10000.0f, 320.0f, window_size[0], window_size[1], 0);
    HuWinMesSet(window, MAKE_MESSID(18, 17));
    HuWinBGTPLvlSet(window, 0.0f);
    HuWinMesSpeedSet(window, 0);
    if (GWPlayer[currItemRestore].com) {
        com_delay = frandmod(120) + 30;
    }
    for (i = 1; i <= 0x12C; i++) {
        if (i % 10 == 0) {
            HuAudFXPlay(0x315);
            hammer_side ^= 1;
            if (hammer_side != 0) {
                BoardModelMotionTimeSet(suitMdl, 1.5f);
            }
            else {
                BoardModelMotionTimeSet(suitMdl, 0.0f);
            }
        }
        if (GWPlayer[currItemRestore].com) {
            com_delay--;
            if (com_delay == 0) {
                break;
            }
        }
        else {
            if (HuPadBtnDown[GWPlayerCfg[currItemRestore].pad_idx] & 0x100) {
                break;
            }
        }
        HuPrcVSleep();
    }
    BoardModelAttrSet(suitMdl, 0x40000002);
    HuAudFXPlay(0x316);
    HuWinKill(window);
    omVibrate(target_player, 12, 6, 6);
    if (hammer_side != 0) {
        BoardPlayerAutoSizeSet(target_player, 2);
    }
    else {
        BoardPlayerAutoSizeSet(target_player, 1);
    }
    HuPrcSleep(30);
    BoardUiInlineFunc02();
    BoardModelKill(suitMdl);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Use a Warp Pipe to select another player and exchange their board positions. */
static void ExecItemPipe(void)
{
    Vec landing_positions[2];
    Vec player_positions[2];
    Vec horizontal_steps[2];
    Vec swap_position;
    Vec pipe_scale;
    Vec player_scale;
    Vec converted_target_a;
    Vec converted_target_b;
    Vec unused_position_a;
    Vec unused_position_b;
    s32 player_spaces[2];
    float start_heights[2];
    float window_size[2];
    s16 pipe_models[2];
    s16 players[2];
    float initial_vertical_velocity;
    float vertical_velocity;
    float flight_time;
    float donkey_scale;
    s16 target_player;
    s16 window;
    s32 unused_value_a;
    s32 unused_value_b;
    s32 unused_value_c;
    s32 unused_value_d;
    s16 scale_delay;
    s32 completed_axes;
    s32 index;

    HuAudFXPlay(0x350);
    ItemShowProc(NULL, NULL);
    for (index = 0; index < 2; index++) {
        pipe_models[index] = BoardModelCreate(itemMdlTbl[currItem], NULL, 1);
        BoardModelLayerSet(pipe_models[index], 2);
        pipe_scale.x = pipe_scale.z = 2.0f;
        pipe_scale.y = 0.0f;
        BoardModelScaleSetV(pipe_models[index], &pipe_scale);
        BoardModelVisibilitySet(pipe_models[index], 0);
    }
    BoardCharWheelInit(currItemRestore, 3);
    BoardCharWheelWait();
    target_player = BoardCharWheelResultGet();
    players[0] = currItemRestore;
    players[1] = target_player;
    for (index = 0; index < 2; index++) {
        player_spaces[index] = GWPlayer[players[index]].space_curr;
        BoardSpacePosGet(0, player_spaces[index], &landing_positions[index]);
        BoardModelPosSetV(pipe_models[index], &landing_positions[index]);
    }
    if (player_spaces[0] == player_spaces[1]) {
        HuWinMesMaxSizeGet(1, window_size, MAKE_MESSID(18, 18));
        window = HuWinCreate(-10000.0f, -10000.0f, window_size[0], window_size[1], 0);
        HuWinMesSet(window, MAKE_MESSID(18, 18));
        HuWinMesWait(window);
        HuPrcSleep(180);
        HuWinKill(window);
    }
    else {
        BoardUiInlineFunc03(players[1]);
        HuAudFXPlay(0x318);
        for (index = 0; index < 2; index++) {
            BoardModelVisibilitySet(pipe_models[index], 1);
            BoardPlayerPosGet(players[index], &player_positions[index]);
        }
        for (index = 0; index < 60; index++) {
            pipe_scale.y += 0.033333335f;
            for (completed_axes = 0; completed_axes < 2; completed_axes++) {
                BoardModelScaleSetV(pipe_models[completed_axes], &pipe_scale);
                player_positions[completed_axes].y += 3.3333333f;
                BoardPlayerPosSetV(players[completed_axes], &player_positions[completed_axes]);
            }
            HuPrcVSleep();
        }
        pipe_scale.y = 2.0f;
        for (index = 0; index < 2; index++) {
            BoardModelScaleSetV(pipe_models[index], &pipe_scale);
        }
        HuPrcSleep(10);
        HuAudFXPlay(0x35A);
        omVibrate(players[0], 0xC, 4, 2);
        vertical_velocity = 5.0f;
        start_heights[0] = player_positions[0].y;
        start_heights[1] = player_positions[1].y;
        donkey_scale = 1.0f;
        while (1) {
            for (index = 0; index < 2; index++) {
                player_positions[index].y -= vertical_velocity;
                BoardPlayerPosSetV(players[index], &player_positions[index]);
                if (GWPlayer[players[index]].character == 5) {
                    donkey_scale -= 0.05f;
                    if (donkey_scale < 0.65f) {
                        donkey_scale = 0.65f;
                    }
                    player_scale.x = player_scale.y = player_scale.z = donkey_scale;
                    BoardPlayerScaleSetV(players[index], &player_scale);
                }
            }
            vertical_velocity *= 1.08f;
            if (start_heights[0] - player_positions[0].y >= 300.0f && start_heights[1] - player_positions[1].y >= 300.0f) {
                break;
            }
            HuPrcVSleep();
        }
        WipeColorSet(0, 0, 0);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 30);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        BoardCameraMoveSet(0);
        swap_position = player_positions[0];
        player_positions[0] = player_positions[1];
        player_positions[1] = swap_position;
        index = player_spaces[0];
        player_spaces[0] = player_spaces[1];
        player_spaces[1] = index;
        for (index = 0; index < 2; index++) {
            GWPlayer[players[index]].space_curr = player_spaces[index];
        }
        for (index = 0; index < 2; index++) {
            BoardPlayerMoveAwayStartCurr(player_spaces[index], 1);
            BoardPlayerPosGet(players[index], &landing_positions[index]);
            BoardPlayerPosSetV(players[index], &player_positions[index]);
        }
        HuPrcVSleep();
        BoardCameraMoveSet(1);
        for (index = 0; index < 2; index++) {
            BoardPlayerMotionStart(players[index], 4, 0);
            BoardPlayerVoiceEnableSet(players[index], 4, 0);
            BoardModelVisibilitySet(BoardPlayerModelGet(players[index]), 0);
        }
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        initial_vertical_velocity = 19.0f;
        flight_time = 0.0f;
        for (index = 0; index < 2; index++) {
            VECSubtract(&landing_positions[index], &player_positions[index], &horizontal_steps[index]);
            VECScale(&horizontal_steps[index], &horizontal_steps[index], 0.03f);
            BoardModelVisibilitySet(BoardPlayerModelGet(players[index]), 1);
        }
        PlayerFXPlay(currItemRestore, 0x122);
        donkey_scale = 0.65f;
        scale_delay = 5;
        while (1) {
            vertical_velocity = initial_vertical_velocity - 0.06666667f * (flight_time * flight_time * 0.25f);
            completed_axes = 0;
            for (index = 0; index < 2; index++) {
                if (GWPlayer[players[index]].character == 5) {
                    if (scale_delay != 0) {
                        scale_delay--;
                    }
                    else {
                        donkey_scale += 0.05f;
                        if (donkey_scale > 1.0f) {
                            donkey_scale = 1.0f;
                        }
                        player_scale.x = player_scale.y = player_scale.z = donkey_scale;
                        BoardPlayerScaleSetV(players[index], &player_scale);
                    }
                }
                player_positions[index].y += vertical_velocity;
                if (fabs(player_positions[index].x - landing_positions[index].x) <= fabs(horizontal_steps[index].x)) {
                    player_positions[index].x = landing_positions[index].x;
                    completed_axes++;
                }
                else {
                    player_positions[index].x += horizontal_steps[index].x;
                }
                if (fabs(player_positions[index].z - landing_positions[index].z) <= fabs(horizontal_steps[index].z)) {
                    player_positions[index].z = landing_positions[index].z;
                    completed_axes++;
                }
                else {
                    player_positions[index].z += horizontal_steps[index].z;
                }
                if (player_positions[index].y < landing_positions[index].y) {
                    player_positions[index].y = landing_positions[index].y;
                    completed_axes++;
                }
                BoardPlayerPosSetV(players[index], &player_positions[index]);
            }
            flight_time += 1.0f;
            if (completed_axes == 6) {
                break;
            }
            HuPrcVSleep();
        }
        BoardPlayerMotionShiftSet(players[1], 6, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
        BoardModelVoiceEnableSet(BoardPlayerModelGet(players[1]), 6, 0);
        CharEffectLayerSet(2);
        BoardPlayerIdleSet(players[0]);
        for (index = 0; index < 45; index++) {
            pipe_scale.y -= 0.044444446f;
            for (completed_axes = 0; completed_axes < 2; completed_axes++) {
                BoardModelScaleSetV(pipe_models[completed_axes], &pipe_scale);
            }
            HuPrcVSleep();
        }
        for (index = 0; index < 2; index++) {
            BoardModelVisibilitySet(pipe_models[index], 0);
        }
        BoardUiInlineFunc03(players[0]);
        BoardWinCreate(2, MAKE_MESSID(18, 29), -1);
        BoardWinInsertMesSet(GWPlayerCfg[players[0]].character, 0);
        BoardWinInsertMesSet(GWPlayerCfg[players[1]].character, 1);
        BoardWinWait();
        BoardWinKill();
        BoardPlayerIdleSet(players[1]);
        HuPrcSleep(8);
        BoardModelVoiceEnableSet(BoardPlayerModelGet(players[1]), 6, 1);
    }
    BoardPlayerVoiceEnableSet(players[0], 4, 1);
    BoardPlayerVoiceEnableSet(players[1], 4, 1);
    for (index = 0; index < 2; index++) {
        BoardModelKill(pipe_models[index]);
    }
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Use a Swap Card to select another player and exchange all held items. */
static void ExecItemSwap(void)
{
    Vec current_item_positions[3];
    Vec target_item_positions[3];
    Vec current_item_screen_positions[3];
    Vec target_item_screen_positions[3];
    s32 swapped_items[6];
    Vec center_position;
    Vec model_rotation;
    Vec rotation_step;
    Vec reference_vector;
    Vec current_item_destination;
    Vec target_item_destination;
    Vec current_item_step;
    Vec target_item_step;
    s16 item_models[6];
    Vec camera_rotation;
    Vec item_rotation;
    float model_depth;
    float model_alpha;
    s16 item_model;
    s32 current_item_count;
    s32 target_item_count;
    s32 item_model_count;
    s32 axis;
    s32 rotation_axis;
    s32 frame;
    s32 index;
    s32 target_player;
    s8 fade_sfx_played;
    Mtx camera_matrix;
    Mtx inverse_camera_matrix;

    HuAudFXPlay(0x350);
    ItemShowProc(NULL, NULL);
    BoardCharWheelInit(currItemRestore, 3);
    BoardCharWheelWait();
    target_player = BoardCharWheelResultGet();
    suitMdl = BoardModelCreate(itemMdlTbl[currItem], NULL, 0);
    BoardModelLayerSet(suitMdl, 7);
    BoardModelAttrSet(suitMdl, 0x400000);
    rotation_step.x = rotation_step.y = rotation_step.z = 0.0f;
    rotation_step.y = 45.0f;
    HuAudFXPlay(0x364);
    BoardUiInlineFunc01(&rotation_step);
    BoardModelPosGet(suitMdl, &center_position);
    BoardPlayerRotGet(currItemRestore, &reference_vector);
    for (axis = 0; axis < 3; axis++) {
        if (((float *)&reference_vector)[axis] < 0.0f) {
            ((float *)&reference_vector)[axis] += 360.0f;
        }
        if (((float *)&reference_vector)[axis] >= 360.0f) {
            ((float *)&reference_vector)[axis] -= 360.0f;
        }
    }
    BoardModelRotGet(suitMdl, &model_rotation);
    while (1) {
        VECAdd(&model_rotation, &rotation_step, &model_rotation);
        for (rotation_axis = 0; rotation_axis < 3; rotation_axis++) {
            if (((float *)&model_rotation)[rotation_axis] < 0.0f) {
                ((float *)&model_rotation)[rotation_axis] += 360.0f;
            }
            if (((float *)&model_rotation)[rotation_axis] >= 360.0f) {
                ((float *)&model_rotation)[rotation_axis] -= 360.0f;
            }
        }
        rotation_step.y *= 0.94f;
        if (rotation_step.y <= 6.0f) {
            rotation_step.y = 6.0f;
            if (fabs(reference_vector.y - model_rotation.y) <= rotation_step.y) {
                model_rotation.y = reference_vector.y;
                break;
            }
        }
        BoardModelRotSetV(suitMdl, &model_rotation);
        HuPrcVSleep();
    }
    BoardModelRotSetV(suitMdl, &model_rotation);
    current_item_count = target_item_count = 0;
    for (index = 0; index < ARRAY_COUNT(GWPlayer->items); index++) {
        if (GWPlayer[currItemRestore].items[index] != -1) {
            current_item_count++;
        }
        if (GWPlayer[target_player].items[index] != -1) {
            target_item_count++;
        }
    }
    if (current_item_count >= target_item_count) {
        item_model_count = current_item_count * 2;
    }
    else {
        item_model_count = target_item_count * 2;
    }
    BoardModelRotGet(suitMdl, &item_rotation);
    Hu3DCameraSet(0, camera_matrix);
    PSMTXInverse(camera_matrix, inverse_camera_matrix);
    Hu3DMtxRotGet(inverse_camera_matrix, &camera_rotation);
    item_rotation.x = camera_rotation.x;
    for (index = 0; index < item_model_count / 2; index++) {
        item_model = item_models[index] = BoardModelCreate(itemMdlTbl[currItem], NULL, 1);
        BoardModelLayerSet(item_model, 6);
        BoardModelPassSet(item_model, 0);
        BoardModelRotSetV(item_model, &item_rotation);
        BoardModelVisibilitySet(item_model, 0);
        BoardModelAttrSet(item_model, 0x400000);
        item_model = item_models[index + 3] = BoardModelCreate(itemMdlTbl[currItem], NULL, 1);
        BoardModelLayerSet(item_model, 6);
        BoardModelPassSet(item_model, 0);
        BoardModelRotSetV(item_model, &item_rotation);
        BoardModelVisibilitySet(item_model, 0);
        BoardModelAttrSet(item_model, 0x400000);
    }
    BoardItemGetDestPos(currItemRestore, &current_item_screen_positions[0]);
    BoardItemGetDestPos(target_player, &target_item_screen_positions[0]);
    BoardModelPosGet(suitMdl, &reference_vector);
    VECSubtract(&reference_vector, &Hu3DCamera->pos, &reference_vector);
    model_depth = VECMag(&reference_vector);
    for (index = 0; index < 3; index++) {
        current_item_screen_positions[index].z = model_depth;
        target_item_screen_positions[index].z = model_depth;
    }
    for (index = 0; index < item_model_count / 2; index++) {
        HuAudFXPlay(0x319);
        BoardModelVisibilitySet(item_models[index], 1);
        BoardModelVisibilitySet(item_models[index + 3], 1);
        current_item_positions[index] = target_item_positions[index] = center_position;
        Hu3D2Dto3D(&current_item_screen_positions[index], 1, &current_item_destination);
        Hu3D2Dto3D(&target_item_screen_positions[index], 1, &target_item_destination);
        VECSubtract(&current_item_destination, &current_item_positions[index], &current_item_step);
        VECScale(&current_item_step, &current_item_step, 0.033333335f);
        VECSubtract(&target_item_destination, &target_item_positions[index], &target_item_step);
        VECScale(&target_item_step, &target_item_step, 0.033333335f);
        for (frame = 0; frame < 30; frame++) {
            BoardModelPosSetV(item_models[index], &current_item_positions[index]);
            BoardModelPosSetV(item_models[index + 3], &target_item_positions[index]);
            VECAdd(&current_item_positions[index], &current_item_step, &current_item_positions[index]);
            VECAdd(&target_item_positions[index], &target_item_step, &target_item_positions[index]);
            HuPrcVSleep();
        }
        current_item_positions[index] = current_item_destination;
        target_item_positions[index] = target_item_destination;
        BoardModelPosSetV(item_models[index], &current_item_positions[index]);
        BoardModelPosSetV(item_models[index + 3], &target_item_positions[index]);
    }
    omVibrate(currItemRestore, 180, 6, 6);
    omVibrate(target_player, 180, 6, 6);
    for (index = 0; index < 60; index++) {
        for (frame = 0; frame < item_model_count / 2; frame++) {
            current_item_step = current_item_positions[frame];
            current_item_step.x += 3.0 * sind(frandmods(360));
            current_item_step.y += 3.0 * sind(frandmods(360));
            BoardModelPosSetV(item_models[frame], &current_item_step);
            current_item_step = target_item_positions[frame];
            current_item_step.x += 3.0 * sind(frandmods(360));
            current_item_step.y += 3.0 * sind(frandmods(360));
            BoardModelPosSetV(item_models[frame + 3], &current_item_step);
        }
        HuPrcSleep(2);
    }
    for (index = 0; index < item_model_count / 2; index++) {
        BoardModelPosSetV(item_models[index], &current_item_positions[index]);
        BoardModelPosSetV(item_models[index + 3], &target_item_positions[index]);
    }
    for (index = 0; index < 3; index++) {
        swapped_items[index] = BoardPlayerItemRemove(currItemRestore, 0);
        swapped_items[index + 3] = BoardPlayerItemRemove(target_player, 0);
    }
    for (index = 0; index < 3; index++) {
        if (swapped_items[index + 3] != -1) {
            BoardPlayerItemAdd(currItemRestore, swapped_items[index + 3]);
        }
        if (swapped_items[index] != -1) {
            BoardPlayerItemAdd(target_player, swapped_items[index]);
        }
    }
    for (index = (item_model_count / 2) - 1; index >= 0; index--) {
        HuAudFXPlay(0x32C);
        VECSubtract(&center_position, &current_item_positions[index], &current_item_step);
        VECScale(&current_item_step, &current_item_step, 0.033333335f);
        VECSubtract(&center_position, &target_item_positions[index], &target_item_step);
        VECScale(&target_item_step, &target_item_step, 0.033333335f);
        for (frame = 0; frame < 30; frame++) {
            BoardModelPosSetV(item_models[index], &current_item_positions[index]);
            BoardModelPosSetV(item_models[index + 3], &target_item_positions[index]);
            VECAdd(&current_item_positions[index], &current_item_step, &current_item_positions[index]);
            VECAdd(&target_item_positions[index], &target_item_step, &target_item_positions[index]);
            HuPrcVSleep();
        }
        BoardModelVisibilitySet(item_models[index], 0);
        BoardModelVisibilitySet(item_models[index + 3], 0);
    }
    rotation_step.y = 0.6f;
    model_alpha = 255.0f;
    fade_sfx_played = 0;
    while (1) {
        BoardModelRotSetV(suitMdl, &model_rotation);
        BoardModelAlphaSet(suitMdl, model_alpha);
        VECAdd(&model_rotation, &rotation_step, &model_rotation);
        rotation_step.y *= 1.02f;
        if (rotation_step.y >= 35.0f) {
            if (fade_sfx_played == 0) {
                HuAudFXPlay(0x365);
                fade_sfx_played = 1;
            }
            rotation_step.y = 45.0f;
            model_alpha -= 5.6666665f;
            if (model_alpha <= 0.0f) {
                break;
            }
        }
        HuPrcVSleep();
    }
    BoardModelKill(suitMdl);
    for (index = 0; index < item_model_count / 2; index++) {
        BoardModelKill(item_models[index]);
        BoardModelKill(item_models[index + 3]);
    }
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Place a Sparky Sticker on the current space. */
static void ExecItemSpark(void)
{
    Vec item_position;
    Vec sparky_position;
    Vec space_corner;
    Vec player_position;
    Vec item_scale;
    Vec adjusted_position;
    Vec space_rotation;
    float model_alpha;
    float fall_velocity;
    s32 space;
    s16 sparky_model;
    s32 i;

    (void)106.0;
    sparky_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 103), NULL, 0);
    BoardModelLayerSet(sparky_model, 3);
    BoardModelVisibilitySet(sparky_model, 0);
    HuAudFXPlay(0x350);
    suitMdl = BoardModelCreate(itemMdlTbl[currItem], itemMotTbl, 0);
    BoardModelLayerSet(suitMdl, 2);
    ItemShowProc(NULL, NULL);
    space = GWPlayer[currItemRestore].space_curr;
    item_scale.x = item_scale.y = item_scale.z = 1.0f;
    for (i = 0; i < 0x1E; i++) {
        item_scale.x += 0.033333335f;
        item_scale.y += 0.033333335f;
        item_scale.z += 0.033333335f;
        BoardModelScaleSetV(suitMdl, &item_scale);
        HuPrcVSleep();
    }
    item_scale.x = item_scale.y = item_scale.z = 2.0f;
    BoardModelScaleSetV(suitMdl, &item_scale);
    HuAudFXPlay(0x31A);
    BoardPlayerPosGet(currItemRestore, &player_position);
    BoardSpaceCornerPosGet(space, 0, &space_corner);
    BoardPlayerPosLerpStart(currItemRestore, &player_position, &space_corner, 0x14);
    while (GWPlayer[currItemRestore].moving) {
        HuPrcVSleep();
    }
    BoardPlayerMotBlendSet(currItemRestore, 0, 7);
    while (!BoardPlayerMotBlendCheck(currItemRestore)) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(currItemRestore);
    BoardSpacePosGet(0, space, &sparky_position);
    BoardModelPosSetV(sparky_model, &sparky_position);
    sparky_position.y += 106.0f;
    BoardModelPosGet(suitMdl, &item_position);
    fall_velocity = -3.0f;
    while (1) {
        item_position.y += fall_velocity;
        if (item_position.y <= sparky_position.y) {
            break;
        }
        BoardModelPosSetV(suitMdl, &item_position);
        fall_velocity *= 1.08f;
        HuPrcVSleep();
    }
    item_position.y = sparky_position.y;
    BoardModelPosSetV(suitMdl, &item_position);
    sparky_position.y -= 106.0f;
    BoardSpaceRotGet(0, GWPlayer[currItemRestore].space_curr, &space_rotation);
    BoardModelRotSetV(suitMdl, &space_rotation);
    BoardModelPosGet(suitMdl, &adjusted_position);
    adjusted_position.z += 106.0 * sind(space_rotation.x);
    adjusted_position.x -= 106.0 * sind(space_rotation.z);
    BoardModelPosSetV(suitMdl, &adjusted_position);
    HuAudFXPlay(0x31B);
    BoardModelMotionStart(suitMdl, 1, 0);
    BoardModelMotionTimeSet(suitMdl, 0.0f);
    while (BoardModelMotionTimeGet(suitMdl) < BoardModelMotionMaxTimeGet(suitMdl)) {
        HuPrcVSleep();
    }
    for (i = 0; i < 4; i++) {
        omVibrate(i, 12, 4, 2);
    }
    HuAudFXPlay(0x31C);
    BoardModelVisibilitySet(sparky_model, 1);
    BoardModelMotionTimeSet(sparky_model, 0.0f);
    while (BoardModelMotionTimeGet(sparky_model) < BoardModelMotionMaxTimeGet(sparky_model)) {
        HuPrcVSleep();
    }
    model_alpha = 255.0f;
    for (i = 0; i < 45; i++) {
        model_alpha -= 5.6666665f;
        BoardModelAlphaSet(suitMdl, model_alpha);
        HuPrcVSleep();
    }
    BoardModelAlphaSet(suitMdl, 0);
    BoardUiInlineFunc03(currItemRestore);
    BoardModelKill(suitMdl);
    BoardModelKill(sparky_model);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Offer to use a Gaddlight; canceling returns it to the player's inventory. */
static void ExecItemLight(void)
{
    s16 choice;

    HuAudFXPlay(0x350);
    ItemShowProc(NULL, NULL);
    BoardWinCreateChoice(2, MAKE_MESSID(18, 15), -1, 0);
    BoardWinInsertMesSet(MAKE_MESSID(8, 8), 3);
    if (GWPlayer[currItemRestore].com) {
        BoardComKeySetRight();
    }
    BoardWinWait();
    choice = BoardWinChoiceGet();
    BoardWinKill();
    if (choice == -1 || choice == 0) {
        BoardPlayerItemAdd(boardItemUser, currItem);
    }
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Use a Chomp Call to drive away the current Star host and reveal the next Star. */
static void ExecItemWhistle(void)
{
    Vec boo_positions[4];
    Vec boo_rotations[4];
    Vec star_host_position;
    Vec boo_scale;
    Vec star_host_moving_position;
    Vec item_position;
    Vec hook_position;
    Vec item_step;
    float vertical_velocity[4];
    float bounce_velocity[4];
    float flight_time[4];
    float orbit_angles[4];
    float orbit_radii[4];
    s16 boo_models[4];
    s16 player_model;
    float host_velocity;
    float orbit_angle;
    s32 player_motion;
    char *item_hook;
    s32 star_space;
    s16 boo_motion;
    s16 attack_motion;
    s16 loop_motion;
    s32 unused_value;
    s16 star_host_model;
    s16 board;
    s32 wipe_started;
    s16 landed_count;
    s16 impact_started;
    s32 frame;
    s32 index;

    index = GWPlayerCfg[currItemRestore].character;
    player_motion = BoardPlayerMotionCreate(currItemRestore, callMotTbl[index]);
    item_hook = callHookTbl[index];
    star_host_model = BoardStarHostMdlGet();
    BoardModelPosGet(star_host_model, &star_host_position);
    star_host_moving_position = star_host_position;
    board = GWBoardGet();
    attack_motion = BoardModelMotionCreate(star_host_model, callAttackMotTbl[board]);
    loop_motion = BoardModelMotionCreate(star_host_model, callAttackMotTbl2[board]);
    orbit_angle = 0.003921569f * frand8() * 360.0f;
    boo_scale.x = boo_scale.y = boo_scale.z = 0.5f;
    for (index = 0; index < 4; index++) {
        boo_models[index] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BGUEST, 39), NULL, 1);
        BoardModelLayerSet(boo_models[index], 2);
        boo_motion = BoardModelMotionCreate(boo_models[index], DATA_MAKE_NUM(DATADIR_BGUEST, 40));
        BoardModelMotionStart(boo_models[index], boo_motion, 0x40000001);
        orbit_radii[index] = 120.0f;
        boo_positions[index].x = star_host_position.x + 120.0 * sind(orbit_angle);
        boo_positions[index].y = 1000.0f + star_host_position.y + 100.0f * index;
        boo_positions[index].z = star_host_position.z + 120.0 * cosd(orbit_angle);
        boo_rotations[index].x = boo_rotations[index].z = 0.0f;
        boo_rotations[index].y = orbit_angle + 90.0f;
        if (boo_rotations[index].y >= 360.0f) {
            boo_rotations[index].y -= 360.0f;
        }
        BoardModelRotSetV(boo_models[index], &boo_rotations[index]);
        BoardModelPosSetV(boo_models[index], &boo_positions[index]);
        BoardModelScaleSetV(boo_models[index], &boo_scale);
        orbit_angle += 90.0f;
        if (orbit_angle >= 360.0f) {
            orbit_angle -= 360.0f;
        }
        orbit_angles[index] = orbit_angle;
        BoardModelVisibilitySet(boo_models[index], 0);
    }
    HuAudFXPlay(0x350);
    suitMdl = BoardModelCreate(itemMdlTbl[currItem], NULL, 0);
    BoardModelLayerSet(suitMdl, 2);
    ItemShowProc(NULL, NULL);
    BoardModelPosGet(suitMdl, &item_position);
    BoardPlayerPosGet(currItemRestore, &hook_position);
    hook_position.y += 200.0f;
    hook_position.z += 100.0f;
    VECSubtract(&hook_position, &item_position, &item_step);
    VECScale(&item_step, &item_step, 0.03448276f);
    BoardPlayerMotionStart(currItemRestore, player_motion, 0);
    BoardPlayerMotionTimeSet(currItemRestore, 1.0f);
    for (index = 0; index < 29; index++) {
        item_position.y += item_step.y;
        item_position.z += item_step.z;
        BoardModelPosSetV(suitMdl, &item_position);
        HuPrcVSleep();
    }
    while (BoardPlayerMotionTimeGet(currItemRestore) < 30.0f) {
        HuPrcVSleep();
    }
    player_model = BoardPlayerModelGet(currItemRestore);
    BoardModelPosSetV(suitMdl, &hook_position);
    BoardModelHookSet(player_model, item_hook, suitMdl);
    while (BoardPlayerMotionTimeGet(currItemRestore) < BoardPlayerMotionMaxTimeGet(currItemRestore)) {
        if (BoardPlayerMotionTimeGet(currItemRestore) == 95.0f || BoardPlayerMotionTimeGet(currItemRestore) == 185.0f
            || BoardPlayerMotionTimeGet(currItemRestore) == 275.0f) {
            HuAudFXPlay(0x31F);
        }
        HuPrcVSleep();
    }
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    BoardModelHookObjReset(player_model, item_hook);
    BoardPlayerIdleSet(currItemRestore);
    BoardModelKill(suitMdl);
    BoardCameraMoveSet(0);
    star_space = BoardSpaceStarGetCurr();
    BoardCameraTargetSpaceSet(star_space);
    HuPrcVSleep();
    BoardCameraMoveSet(1);
    for (index = 0; index < 4; index++) {
        BoardModelVisibilitySet(boo_models[index], 1);
    }
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x320);
    for (index = 0; index < 4; index++) {
        bounce_velocity[index] = 0.0f;
        flight_time[index] = 1.0f;
    }
    impact_started = 0;
    for (landed_count = 0; landed_count < 4;) {
        for (index = 0; index < 4; index++) {
            if (flight_time[index] != -1.0f) {
                vertical_velocity[index] = bounce_velocity[index] - 0.06666667f * (flight_time[index] * flight_time[index] * 0.25f);
                if (vertical_velocity[index] < -35.0f) {
                    vertical_velocity[index] = -35.0f;
                }
                if (vertical_velocity[index] > 35.0f) {
                    vertical_velocity[index] = 35.0f;
                }
                flight_time[index] += 1.0f;
                boo_positions[index].y += vertical_velocity[index];
                if (boo_positions[index].y <= star_host_position.y) {
                    if (impact_started == 0) {
                        impact_started = 1;
                        for (frame = 0; frame < 4; frame++) {
                            omVibrate(frame, 12, 6, 6);
                        }
                        HuAudFXPlay(0x321);
                    }
                    boo_positions[index].y = star_host_position.y;
                    bounce_velocity[index] = 0.16f * -vertical_velocity[index];
                    if (fabs(bounce_velocity[index]) <= 1.0) {
                        flight_time[index] = -1.0f;
                        landed_count++;
                    }
                    flight_time[index] = 1.0f;
                }
                BoardModelPosSetV(boo_models[index], &boo_positions[index]);
            }
        }
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(star_host_model, loop_motion, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    HuAudFXPlay(0x322);
    for (frame = 0; frame < 180; frame++) {
        for (index = 0; index < 4; index++) {
            orbit_angles[index] += 4.0f;
            if (orbit_angles[index] >= 360.0f) {
                orbit_angles[index] -= 360.0f;
            }
            boo_rotations[index].y = 90.0f + orbit_angles[index];
            if (boo_rotations[index].y >= 360.0f) {
                boo_rotations[index].y -= 360.0f;
            }
            boo_positions[index].x = star_host_position.x + 120.0 * sind(orbit_angles[index]);
            boo_positions[index].z = star_host_position.z + 120.0 * cosd(orbit_angles[index]);
            BoardModelPosSetV(boo_models[index], &boo_positions[index]);
            BoardModelRotSetV(boo_models[index], &boo_rotations[index]);
        }
        HuPrcVSleep();
    }
    for (frame = 0; frame < 20; frame++) {
        for (index = 0; index < 4; index++) {
            boo_rotations[index].y += 4.5f;
            BoardModelRotSetV(boo_models[index], &boo_rotations[index]);
        }
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    host_velocity = 35.0f;
    wipe_started = 0;
    for (index = 0; index < 4; index++) {
        bounce_velocity[index] = 8.0f;
        flight_time[index] = 1.0f;
    }
    impact_started = 0;
    while (1) {
        for (index = 0; index < 4; index++) {
            if (flight_time[index] != -1.0f) {
                orbit_radii[index] -= 1.7f;
                vertical_velocity[index] = bounce_velocity[index] - 0.083333336f * (flight_time[index] * flight_time[index] * 0.25f);
                flight_time[index] += 1.0f;
                boo_positions[index].y += vertical_velocity[index];
                if (boo_positions[index].y <= star_host_position.y) {
                    boo_positions[index].y = star_host_position.y;
                    flight_time[index] = -1.0f;
                }
                boo_positions[index].x = star_host_position.x + orbit_radii[index] * sind(orbit_angles[index]);
                boo_positions[index].z = star_host_position.z + orbit_radii[index] * cosd(orbit_angles[index]);
                BoardModelPosSetV(boo_models[index], &boo_positions[index]);
            }
        }
        if (orbit_radii[0] < 100.0f) {
            if (impact_started == 0) {
                impact_started = 1;
                BoardModelMotionShiftSet(star_host_model, attack_motion, 0.0f, 3.0f, HU3D_MOTATTR_LOOP);
                HuAudFXPlay(forceMoveSfxTbl[GWBoardGet()]);
            }
            star_host_moving_position.y += host_velocity;
            BoardModelPosSetV(star_host_model, &star_host_moving_position);
            host_velocity *= 0.945f;
            if (star_host_moving_position.y >= 500.0f && wipe_started == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 45);
                BoardAudSeqFadeOut(0, 1000);
                wipe_started = 1;
            }
        }
        if (wipe_started != 0 && WipeStatGet() == 0) {
            break;
        }
        HuPrcVSleep();
    }
    BoardPlayerMotionKill(currItemRestore, player_motion);
    for (index = 0; index < 4; index++) {
        BoardModelKill(boo_models[index]);
    }
    BoardModelMotionKill(star_host_model, attack_motion);
    BoardModelMotionKill(star_host_model, loop_motion);
    BoardStarShowNext(currItemRestore);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Equip the current player with the Bowser Suit and its player-specific motions. */
static void ExecItemBowser(void)
{
    Vec suit_position;
    Vec suit_scale;
    Vec space_position;
    Vec player_scale;
    float fall_velocity;
    s16 suit_player_model;
    s32 space;
    s32 i;

    BoardBowserSuitInit(currItemRestore);
    suitMdl = BoardBowserSuitModelGet();
    suit_player_model = BoardBowserSuitPlayerModelGet();
    suitMot[0] = BoardModelMotionCreate(suitMdl, DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 28));
    suitMot[1] = BoardModelMotionCreate(suitMdl, DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 29));
    suitCommonMot[0] = BoardModelMotionCreate(suit_player_model, suitMotTbl[GWPlayer[currItemRestore].character][0]);
    suitCommonMot[1] = BoardModelMotionCreate(suit_player_model, suitMotTbl[GWPlayer[currItemRestore].character][1]);
    BoardModelVisibilitySet(suit_player_model, 0);
    HuAudFXPlay(0x350);
    suit_scale.x = suit_scale.y = suit_scale.z = 0.3f;
    ItemShowProc(NULL, &suit_scale);
    for (i = 0; i < 30; i++) {
        suit_scale.x += 0.023333333f;
        suit_scale.y += 0.023333333f;
        suit_scale.z += 0.023333333f;
        BoardModelScaleSetV(suitMdl, &suit_scale);
        HuPrcVSleep();
    }
    suit_scale.x = suit_scale.y = suit_scale.z = 1.0f;
    BoardModelScaleSetV(suitMdl, &suit_scale);
    HuAudFXPlay(0x325);
    space = GWPlayer[currItemRestore].space_curr;
    BoardSpacePosGet(0, space, &space_position);
    BoardModelPosGet(suitMdl, &suit_position);
    fall_velocity = -4.0f;
    player_scale.x = player_scale.y = player_scale.z = 1.0f;
    while (1) {
        suit_position.y += fall_velocity;
        if (suit_position.y <= space_position.y + 100.0f) {
            player_scale.y = (suit_position.y - space_position.y) / 100.0f;
            BoardPlayerScaleSetV(currItemRestore, &player_scale);
        }
        if (suit_position.y <= space_position.y) {
            break;
        }
        BoardModelPosSetV(suitMdl, &suit_position);
        fall_velocity *= 1.09f;
        HuPrcVSleep();
    }
    suit_position.y = space_position.y;
    BoardModelPosSetV(suitMdl, &suit_position);
    BoardModelVisibilitySet(BoardPlayerModelGet(currItemRestore), 0);
    omVibrate(currItemRestore, 0xC, 4, 2);
    HuAudFXPlay(0x326);
    BoardModelVisibilitySet(suit_player_model, 1);
    BoardModelHookSet(suitMdl, "itemhook_head", suit_player_model);
    BoardModelMotionShiftSet(suitMdl, suitMot[1], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    BoardModelMotionShiftSet(suit_player_model, suitCommonMot[1], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    while (!BoardModelMotionEndCheck(suitMdl)) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(suitMdl, suitMot[0], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    BoardModelMotionShiftSet(suit_player_model, suitCommonMot[0], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(0x28);
    PlayerFXPlay(currItemRestore, 0x129);
    omVibrate(currItemRestore, 12, 4, 2);
    HuPrcSleep(60);
    BoardModelMotionShiftSet(suitMdl, 0, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

static s32 booMotTbl[3] = { DATA_MAKE_NUM(DATADIR_BGUEST, 24), DATA_MAKE_NUM(DATADIR_BGUEST, 22), -1 };

/** @brief Fade and shrink the Boo Ball downward. @param speed Animation duration in frames. */
static inline void ExecItemBooBallInlineFunc01(s32 speed)
{
    Vec start_position;
    s32 frame;

    start_position = booBallPos;
    booBallScale.x = booBallScale.y = booBallScale.z = 1.0f;
    booBallAlpha = 255.0f;
    BoardModelPosSetV(booBallMdl, &booBallPos);
    BoardModelScaleSetV(booBallMdl, &booBallScale);
    BoardModelAlphaSet(booBallMdl, booBallAlpha);
    BoardModelVisibilitySet(booBallMdl, 1);
    for (frame = 0; frame < speed; frame++) {
        booBallAlpha -= (255.0f / speed);
        if (booBallAlpha < 0.0f) {
            booBallAlpha = 0.0f;
        }
        booBallScale.x -= (1.0f / speed);
        booBallScale.y -= (1.0f / speed);
        booBallScale.z -= (1.0f / speed);
        booBallPos.y -= (20.0f / speed);
        BoardModelAlphaSet(booBallMdl, booBallAlpha);
        BoardModelScaleSetV(booBallMdl, &booBallScale);
        BoardModelPosSetV(booBallMdl, &booBallPos);
        HuPrcVSleep();
    }
    booBallScale.x = booBallScale.y = booBallScale.z = 0.0f;
    booBallAlpha = 0.0f;
    BoardModelScaleSetV(booBallMdl, &booBallScale);
    BoardModelAlphaSet(booBallMdl, booBallAlpha);
}

/** @brief Fade and grow the Boo Ball upward. @param speed Animation duration in frames. */
static inline void ExecItemBooBallInlineFunc02(s32 speed)
{
    Vec start_position;
    s32 frame;

    start_position = booBallPos;
    booBallScale.x = booBallScale.y = booBallScale.z = 0.0f;
    booBallAlpha = 0.0f;
    BoardModelPosSetV(booBallMdl, &booBallPos);
    BoardModelScaleSetV(booBallMdl, &booBallScale);
    BoardModelAlphaSet(booBallMdl, booBallAlpha);
    BoardModelVisibilitySet(booBallMdl, 1);
    for (frame = 0; frame < speed; frame++) {
        booBallAlpha += (255.0f / speed);
        if (booBallAlpha > 255.0f) {
            booBallAlpha = 255.0f;
        }
        booBallScale.x += (1.0f / speed);
        booBallScale.y += (1.0f / speed);
        booBallScale.z += (1.0f / speed);
        booBallPos.y += (20.0f / speed);
        BoardModelAlphaSet(booBallMdl, booBallAlpha);
        BoardModelScaleSetV(booBallMdl, &booBallScale);
        BoardModelPosSetV(booBallMdl, &booBallPos);
        HuPrcVSleep();
    }
    booBallPos.y = start_position.y + 20.0f;
    booBallScale.x = booBallScale.y = booBallScale.z = 1.0f;
    booBallAlpha = 255.0f;
    BoardModelPosSetV(booBallMdl, &booBallPos);
    BoardModelScaleSetV(booBallMdl, &booBallScale);
    BoardModelAlphaSet(booBallMdl, booBallAlpha);
}

/** @brief Use a Boo's Crystal Ball to buy a coin or Star theft, or view the map. */
static void ExecItemBooBall(void)
{
    UnkItemShowProcStruct boo_work;
    UnkItemShowProcStruct ball_work;
    Vec boo_position;
    Vec camera_rotation;
    HUPROCESS *boo_process;
    HUPROCESS *ball_process;
    s16 stolen_coins;
    s16 affected_players;
    s32 star_stream;
    s32 available_coins;
    s32 available_stars;
    s32 result;
    s32 message;
    s32 index;

    BoardAudSeqPause(0, 1, 1000);
    suitMdl = BoardBooCreate(currItemRestore, &boo_position);
    booBallMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BGUEST, 18), booMotTbl, 0);
    BoardModelMotionStart(booBallMdl, 1, 0x40000001);
    BoardModelLayerSet(booBallMdl, 2);
    BoardModelAttrSet(booBallMdl, 2);
    BoardModelVisibilitySet(booBallMdl, 0);
    boo_work.model = suitMdl;
    boo_work.bob_amplitude = 4.0f;
    boo_work.bob_speed = 3.0f;
    boo_work.rotation_step_x = boo_work.rotation_step_y = boo_work.rotation_step_z = 0.0f;
    boo_work.show_smoke = 1;
    HuAudFXPlay(0x350);
    boo_process = ItemShowProc(&boo_work, NULL);
    BoardModelPosGet(suitMdl, &booBallPos);
    ExecItemBooBallInlineFunc02(60);
    ball_work.model = booBallMdl;
    ball_work.bob_amplitude = 4.0f;
    ball_work.bob_speed = 4.0f;
    ball_work.rotation_step_x = ball_work.rotation_step_y = ball_work.rotation_step_z = 0.0f;
    ball_work.show_smoke = 1;
    ball_process = BoardUiInlineFunc05(&ball_work);
    BoardMusStart(1, 1, 0x7F, 0);
    HuAudFXPlay(0x4C);
    BoardWinCreate(2, MAKE_MESSID(18, 21), 3);
    BoardWinWait();
    BoardWinKill();
    if (BoardPlayerCoinsGet(currItemRestore) < 5) {
        BoardWinCreate(2, MAKE_MESSID(18, 22), 3);
        BoardWinWait();
        BoardWinKill();
    }
    else {
        available_stars = 0;
        for (index = 0; index < 4; index++) {
            if (index != currItemRestore) {
                available_stars += GWStarsGet(index);
            }
        }
        available_coins = 0;
        for (index = 0; index < 4; index++) {
            if (index != currItemRestore) {
                available_coins += BoardPlayerCoinsGet(index);
            }
        }
        do {
            BoardWinCreateChoice(4, MAKE_MESSID(18, 24), 3, 0);
            BoardWinAttrSet(0x10);
            if (BoardPlayerCoinsGet(currItemRestore) < 50 || available_stars == 0) {
                BoardWinChoiceDisable(1);
            }
            if (available_coins == 0) {
                BoardWinChoiceDisable(0);
            }
            if (GWPlayer[currItemRestore].com) {
                if (available_stars != 0 && BoardPlayerCoinsGet(currItemRestore) >= 50) {
                    BoardComKeySetDown();
                }
                else {
                    BoardComKeySetUp();
                }
            }
            BoardWinWait();
            BoardWinKill();
            result = BoardWinChoiceGet();
            switch (result) {
                case 0:
                    for (index = 0; index < 5; index++) {
                        BoardPlayerCoinsAdd(currItemRestore, -1);
                        HuAudFXPlay(0xE);
                        HuPrcSleep(6);
                    }
                    HuAudFXPlay(0xF);
                    BoardBooStealTypeSet(0);
                    BoardStatusShowSetAll(0);
                    HuAudFXPlay(0x34F);
                    ExecItemBooBallInlineFunc01(30);
                    camera_rotation.x = 0.0f;
                    camera_rotation.y = 0.0f;
                    camera_rotation.z = 0.0f;
                    BoardCameraMotionStartEx(suitMdl, NULL, &camera_rotation, 325.0f, -1.0f, 21);
                    BoardBooStealMain();
                    BoardCameraViewSet(2);
                    BoardCameraMotionWait();
                    ExecItemBooBallInlineFunc02(30);
                    result = BoardBooStealValueGet(&stolen_coins, &affected_players);
                    if (result == 0) {
                        message = MAKE_MESSID(7, 11);
                    }
                    else if (affected_players == 0) {
                        message = MAKE_MESSID(7, 9);
                    }
                    else if (affected_players >= 1 && affected_players <= 3) {
                        message = MAKE_MESSID(7, 8);
                    }
                    else if (affected_players >= 4 && affected_players <= 6) {
                        message = MAKE_MESSID(7, 7);
                    }
                    else if (affected_players >= 7 && affected_players <= 8) {
                        message = MAKE_MESSID(7, 6);
                    }
                    else {
                        message = MAKE_MESSID(7, 5);
                    }
                    sprintf(booCoinStr, "%d", stolen_coins);
                    HuAudFXPlay(0x4C);
                    BoardWinCreate(2, message, 3);
                    BoardWinInsertMesSet(MAKE_MESSID_PTR(booCoinStr), 0);
                    BoardWinWait();
                    BoardWinKill();
                    BoardStatusShowSetAll(1);
                    if (result != 0) {
                        BoardPlayerMotionShiftSet(currItemRestore, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
                        HuPrcSleep(30);
                        for (index = 0; index < stolen_coins; index++) {
                            BoardPlayerCoinsAdd(currItemRestore, 1);
                            HuAudFXPlay(7);
                            HuPrcVSleep();
                        }
                        BoardPlayerMotionEndWait(currItemRestore);
                        HuPrcSleep(10);
                        BoardPlayerMotionShiftSet(currItemRestore, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                    }
                    break;
                case 1:
                    for (index = 0; index < 50; index++) {
                        BoardPlayerCoinsAdd(currItemRestore, -1);
                        HuAudFXPlay(0xE);
                        HuPrcSleep(1);
                    }
                    HuAudFXPlay(0xF);
                    BoardBooStealTypeSet(1);
                    BoardStatusShowSetAll(0);
                    ExecItemBooBallInlineFunc01(30);
                    camera_rotation.x = 0.0f;
                    camera_rotation.y = 0.0f;
                    camera_rotation.z = 0.0f;
                    BoardCameraMotionStartEx(suitMdl, NULL, &camera_rotation, 325.0f, -1.0f, 21);
                    BoardBooStealMain();
                    result = BoardBooStealLightCheck();
                    BoardCameraViewSet(2);
                    BoardCameraMotionWait();
                    ExecItemBooBallInlineFunc02(30);
                    if (result != 0) {
                        message = MAKE_MESSID(7, 10);
                    }
                    else {
                        message = MAKE_MESSID(7, 11);
                    }
                    HuAudFXPlay(0x4C);
                    BoardWinCreate(2, message, 3);
                    BoardWinWait();
                    BoardWinKill();
                    BoardStatusShowSetAll(1);
                    BoardAudSeqFadeOut(1, 1000);
                    HuPrcSleep(23);
                    if (result != 0) {
                        BoardPlayerVoiceEnableSet(currItemRestore, 7, 0);
                        star_stream = HuAudSStreamPlay(6);
                        BoardPlayerMotionShiftSet(currItemRestore, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
                        HuAudFXPlay(boardStarSndTbl[GWPlayer[currItemRestore].character]);
                        BoardPlayerStarsAdd(currItemRestore, 1);
                        HuAudFXPlay(8);
                        HuPrcVSleep();
                        BoardPlayerMotionEndWait(currItemRestore);
                        BoardPlayerVoiceEnableSet(currItemRestore, 7, 1);
                        while (msmStreamGetStatus(star_stream) != 0) {
                            HuPrcVSleep();
                        }
                    }
                    BoardPlayerMotionShiftSet(currItemRestore, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                    break;
                case 3:
                    BoardViewMapExec(currItemRestore);
                    break;
                case 2:
                case -1:
                    HuAudFXPlay(0x4C);
                    BoardWinCreate(2, MAKE_MESSID(18, 23), 3);
                    BoardWinWait();
                    BoardWinKill();
                    break;
            }
        } while (result == 3);
    }
    BoardAudSeqFadeOut(1, 1000);
    HuPrcKill(ball_process);
    BoardModelPosGet(booBallMdl, &booBallPos);
    HuAudFXPlay(0x34F);
    ExecItemBooBallInlineFunc01(60);
    HuPrcKill(boo_process);
    BoardAudSeqPause(0, 0, 1000);
    CharEffectLayerSet(3);
    BoardModelPosGet(suitMdl, &boo_position);
    CharEffectSmokeCreate(1, &boo_position);
    HuAudFXPlay(0x351);
    HuPrcSleep(10);
    BoardModelKill(suitMdl);
    BoardModelKill(booBallMdl);
    HuPrcSleep(15);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Retain floating-point constants required by the matched item object. */
static void ForceConsts(void)
{
    (void)-0.5f;
    (void)40.0f;
    (void)125.0f;
}

/** @brief Emit and animate the sparkling smoke around the Magic Lamp. @param model Particle model; not read by this callback. @param particle Particle emitter and element storage to update. @param matrix Model matrix supplied by the particle system; not read here. */
static void LampParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particle_data;
    Vec lamp_rotation;
    float unused_emission_speed;
    float direction_angle;
    float brightness;
    s32 i;
    s32 j;

    if (particle->count == 0) {
        particle_data = particle->data;
        for (i = 0; i < particle->maxCnt; i++, particle_data++) {
            particle_data->scale = 0.0f;
            particle_data->color.a = 0;
        }
        particle->pos.x = 0.0f;
        particle->dataCnt = 0;
    }
    if (particle->dataCnt == 0) {
        particle->dataCnt = 2;
        BoardModelRotGet(suitMdl, &lamp_rotation);
        for (i = 0; i < 2; i++) {
            particle_data = particle->data;
            for (j = 0; j < particle->maxCnt; j++, particle_data++) {
                if (particle_data->scale == 0.0f) {
                    break;
                }
            }
            if (j != particle->maxCnt) {
                particle_data->pos.x = 60.0 * sind(lamp_rotation.y - 90.0f);
                particle_data->pos.y = 30.0f;
                particle_data->pos.z = 60.0 * cosd(lamp_rotation.y - 90.0f);
                unused_emission_speed = 12.0f;
                particle_data->vel.x = 12.0 * cosd(i * 110.0f + 35.0f);
                particle_data->vel.y = 12.0 * sind(i * 110.0f + 35.0f);
                particle_data->vel.z = 3.0f + frand8() * 5.0f * 0.003921569f;
                particle_data->accel.y = 255.0f;
                particle_data->scale = 25.0f;
                brightness = 175.0f + frand8() * 0x50 * 0.003921569f;
                particle_data->color.r = particle_data->color.g = particle_data->color.b = brightness;
                particle_data->time = 0;
            }
        }
    }
    else {
        particle->dataCnt--;
    }
    particle_data = particle->data;
    for (i = 0; i < particle->maxCnt; i++, particle_data++) {
        if (particle_data->scale != 0.0f) {
            if (particle_data->time == 0) {
                VECAdd(&particle_data->pos, &particle_data->vel, &particle_data->pos);
                VECScale(&particle_data->vel, &particle_data->vel, 0.95f);
                particle_data->scale += 8.0f;
                particle_data->accel.y -= 2.8333333f;
                if (particle_data->accel.y <= 120.0f) {
                    particle_data->accel.y = 200.0f;
                    direction_angle = frand8() * 180.0f * 0.003921569f;
                    particle_data->vel.x = 6.0 * cosd(direction_angle);
                    particle_data->vel.y = -4.0f;
                    particle_data->vel.z = 6.0 * sind(direction_angle);
                    particle_data->time = 1;
                }
                particle_data->color.a = particle_data->accel.y;
            }
            else {
                VECAdd(&particle_data->pos, &particle_data->vel, &particle_data->pos);
                particle_data->scale += 0.2f;
                particle_data->accel.y -= 1.8214285f;
                if (particle_data->accel.y <= 55.0f) {
                    particle_data->accel.y = 0.0f;
                    particle_data->scale = 0.0f;
                }
                particle_data->color.a = particle_data->accel.y;
            }
        }
    }
}

/** @brief Initialize and fade the particles in the lamp-genie cutscene. @param model Particle model; not read by this callback. @param particle Particle emitter and element storage to update. @param matrix Model matrix supplied by the particle system; not read here. */
static void GenieParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particle_data;
    float speed;
    float brightness;
    s32 active_count;
    s32 i;

    if (particle->count == 0) {
        particle_data = particle->data;
        for (i = 0; i < particle->maxCnt; i++, particle_data++) {
            particle_data->pos.x = -50.0f + frand8() * 100.0f * 0.003921569f;
            particle_data->pos.y = -50.0f + frand8() * 100.0f * 0.003921569f;
            particle_data->pos.z = -10.0f + frand8() * 20.0f * 0.003921569f;
            speed = 0.5f + frand8() * 3.0f * 0.003921569f;
            particle_data->vel.x = speed * cosd(i * 110.0f + 35.0f);
            particle_data->vel.y = speed * sind(i * 110.0f + 35.0f);
            particle_data->vel.z = 0.0f;
            particle_data->accel.y = 255.0f;
            brightness = 125.0f + frand8() * 100 * 0.003921569f;
            particle_data->color.r = particle_data->color.g = particle_data->color.b = brightness;
            particle_data->time = 0;
            particle_data->scale = 80.0f + frand8() * 60.0f * 0.003921569f;
        }
    }
    particle_data = particle->data;
    active_count = 0;
    for (i = 0; i < particle->maxCnt; i++, particle_data++) {
        if (particle_data->scale != 0.0f) {
            active_count++;
            VECAdd(&particle_data->pos, &particle_data->vel, &particle_data->pos);
            VECScale(&particle_data->vel, &particle_data->vel, 0.995f);
            particle_data->scale -= 0.2f;
            particle_data->accel.y -= 1.59375f;
            if (particle_data->accel.y <= 20.0f) {
                particle_data->accel.y = 0.0f;
                particle_data->scale = 0.0f;
            }
            particle_data->color.a = particle_data->accel.y;
        }
    }
    if (active_count == 0) {
        particle->emitCnt = 0;
    }
}

static Vec shadowPos = { 0.0f, 4500.0f, 500.0f };
static Vec shadowTarget = { 0.0f, 0.9f, -0.1f };
static Vec shadowUp = { 0.0f, 0.0f, 500.0f };

/** @brief Apply the fixed light position, target, and up vector used for genie shadows. */
static void GenieCameraProcInlineFunc(void)
{
    Hu3DShadowPosSet(&shadowPos, &shadowTarget, &shadowUp);
}

/** @brief Continuously apply the genie cutscene camera, perspective, and shadow projection. */
static void GenieCameraProc(void)
{
    while (1) {
        Hu3DCameraPerspectiveSet(2, genieFov, 10.0f, 20000.0f, 1.2f);
        Hu3DCameraPosSetV(2, &booCamPos, &booCamTarget, &booCamUp);
        Hu3DShadowData.fov = genieFov;
        C_MTXLightPerspective(Hu3DShadowData.projMtx, Hu3DShadowData.fov, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
        GenieCameraProcInlineFunc();
        Hu3DShadowTPLvlSet(0.8f);
        HuPrcVSleep();
    }
}

/**
 * @brief Interpolate one segment of the genie camera path with spline control points.
 * @param arg0 Camera path containing segment durations, positions, and targets.
 * @param arg1 Path segment index; zero uses the three-point endpoint formula.
 * @param arg2 Normalized progress through the segment, from 0 to 1.
 * @param arg3 Receives the interpolated camera position.
 * @param arg4 Receives the interpolated camera target.
 */
static void GenieCameraCalc(UnkGenieCameraStruct *arg0, s32 arg1, float arg2, Vec *arg3, Vec *arg4)
{
    Vec unused_zero = { 0.0f, 0.0f, 0.0f };
    float t_squared = arg2 * arg2;
    float t_cubed = t_squared * arg2;
    Vec *position_current_or_previous;
    Vec *target_current_or_previous;
    Vec *position_next_or_current;
    Vec *position_following_or_next;
    Vec *target_next_or_current;
    Vec *target_following_or_next;
    Vec *position_following;
    Vec *target_following;

    if (arg1 == 0) {
        position_current_or_previous = &arg0[arg1].position;
        position_next_or_current = &arg0[arg1 + 1].position;
        position_following_or_next = &arg0[arg1 + 2].position;
        target_current_or_previous = &arg0[arg1].target;
        target_next_or_current = &arg0[arg1 + 1].target;
        target_following_or_next = &arg0[arg1 + 2].target;
        arg3->x = 0.5f
            * ((position_current_or_previous->x - position_next_or_current->x * 2.0f + position_following_or_next->x) * t_squared + (position_current_or_previous->x * -3.0f + position_next_or_current->x * 4.0f - position_following_or_next->x) * arg2
                + position_current_or_previous->x * 2.0f);
        arg3->y = 0.5f
            * ((position_current_or_previous->y - position_next_or_current->y * 2.0f + position_following_or_next->y) * t_squared + (position_current_or_previous->y * -3.0f + position_next_or_current->y * 4.0f - position_following_or_next->y) * arg2
                + position_current_or_previous->y * 2.0f);
        arg3->z = 0.5f
            * ((position_current_or_previous->z - position_next_or_current->z * 2.0f + position_following_or_next->z) * t_squared + (position_current_or_previous->z * -3.0f + position_next_or_current->z * 4.0f - position_following_or_next->z) * arg2
                + position_current_or_previous->z * 2.0f);
        arg4->x = 0.5f
            * ((target_current_or_previous->x - target_next_or_current->x * 2.0f + target_following_or_next->x) * t_squared + (target_current_or_previous->x * -3.0f + target_next_or_current->x * 4.0f - target_following_or_next->x) * arg2
                + target_current_or_previous->x * 2.0f);
        arg4->y = 0.5f
            * ((target_current_or_previous->y - target_next_or_current->y * 2.0f + target_following_or_next->y) * t_squared + (target_current_or_previous->y * -3.0f + target_next_or_current->y * 4.0f - target_following_or_next->y) * arg2
                + target_current_or_previous->y * 2.0f);
        arg4->z = 0.5f
            * ((target_current_or_previous->z - target_next_or_current->z * 2.0f + target_following_or_next->z) * t_squared + (target_current_or_previous->z * -3.0f + target_next_or_current->z * 4.0f - target_following_or_next->z) * arg2
                + target_current_or_previous->z * 2.0f);
    }
    else {
        position_current_or_previous = &arg0[arg1 - 1].position;
        position_next_or_current = &arg0[arg1].position;
        position_following_or_next = &arg0[arg1 + 1].position;
        position_following = &arg0[arg1 + 2].position;
        target_current_or_previous = &arg0[arg1 - 1].target;
        target_next_or_current = &arg0[arg1].target;
        target_following_or_next = &arg0[arg1 + 1].target;
        target_following = &arg0[arg1 + 2].target;
        arg3->x = 0.5f
            * ((-position_current_or_previous->x + position_next_or_current->x * 3.0f - position_following_or_next->x * 3.0f + position_following->x) * t_squared * arg2
                + (position_current_or_previous->x * 2.0f - position_next_or_current->x * 5.0f + position_following_or_next->x * 4.0f - position_following->x) * t_squared + (-position_current_or_previous->x + position_following_or_next->x) * arg2
                + position_next_or_current->x * 2.0f);
        arg3->y = 0.5f
            * ((-position_current_or_previous->y + position_next_or_current->y * 3.0f - position_following_or_next->y * 3.0f + position_following->y) * t_squared * arg2
                + (position_current_or_previous->y * 2.0f - position_next_or_current->y * 5.0f + position_following_or_next->y * 4.0f - position_following->y) * t_squared + (-position_current_or_previous->y + position_following_or_next->y) * arg2
                + position_next_or_current->y * 2.0f);
        arg3->z = 0.5f
            * ((-position_current_or_previous->z + position_next_or_current->z * 3.0f - position_following_or_next->z * 3.0f + position_following->z) * t_squared * arg2
                + (position_current_or_previous->z * 2.0f - position_next_or_current->z * 5.0f + position_following_or_next->z * 4.0f - position_following->z) * t_squared + (-position_current_or_previous->z + position_following_or_next->z) * arg2
                + position_next_or_current->z * 2.0f);
        arg4->x = 0.5f
            * ((-target_current_or_previous->x + target_next_or_current->x * 3.0f - target_following_or_next->x * 3.0f + target_following->x) * t_squared * arg2
                + (target_current_or_previous->x * 2.0f - target_next_or_current->x * 5.0f + target_following_or_next->x * 4.0f - target_following->x) * t_squared + (-target_current_or_previous->x + target_following_or_next->x) * arg2
                + target_next_or_current->x * 2.0f);
        arg4->y = 0.5f
            * ((-target_current_or_previous->y + target_next_or_current->y * 3.0f - target_following_or_next->y * 3.0f + target_following->y) * t_squared * arg2
                + (target_current_or_previous->y * 2.0f - target_next_or_current->y * 5.0f + target_following_or_next->y * 4.0f - target_following->y) * t_squared + (-target_current_or_previous->y + target_following_or_next->y) * arg2
                + target_next_or_current->y * 2.0f);
        arg4->z = 0.5f
            * ((-target_current_or_previous->z + target_next_or_current->z * 3.0f - target_following_or_next->z * 3.0f + target_following->z) * t_squared * arg2
                + (target_current_or_previous->z * 2.0f - target_next_or_current->z * 5.0f + target_following_or_next->z * 4.0f - target_following->z) * t_squared + (-target_current_or_previous->z + target_following_or_next->z) * arg2
                + target_next_or_current->z * 2.0f);
    }
}

static s32 armUpMotTbl[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 92), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 92), DATA_MAKE_NUM(DATADIR_PEACHMOT, 92),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 92), DATA_MAKE_NUM(DATADIR_WARIOMOT, 92), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 92),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 92), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 92) };

static s32 scareMotTbl[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 28), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 28), DATA_MAKE_NUM(DATADIR_PEACHMOT, 28),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 28), DATA_MAKE_NUM(DATADIR_WARIOMOT, 28), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 28),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 28), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 28) };

static s32 impactMotTbl[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 94), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 94), DATA_MAKE_NUM(DATADIR_PEACHMOT, 94),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 94), DATA_MAKE_NUM(DATADIR_WARIOMOT, 94), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 94),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 94), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 94) };

static UnkGenieCameraStruct cameraDataTbl[] = { 60.0f, { 0.0f, 100.0f, 950.0f }, { 0.0f, 100.0f, 600.0f }, 60.0f, { -353.0f, 100.0f, 662.5f },
    { 0.0f, 100.0f, 600.0f }, 60.0f, { -500.0f, 100.0f, 375.0f }, { 0.0f, 100.0f, 600.0f }, 60.0f, { -353.0f, 100.0f, 87.5f },
    { 0.0f, 100.0f, 600.0f }, 1.0f, { 0.0f, 100.0f, -200.0f }, { 0.0f, 100.0f, 600.0f }, 89.0f, { 0.0f, 100.0f, -200.0f }, { 0.0f, 100.0f, 600.0f },
    1.0f, { 0.0f, 100.0f, -200.0f }, { 0.0f, 100.0f, 600.0f }, 9.0f, { 0.0f, 100.0f, -200.0f }, { 0.0f, 100.0f, 600.0f }, 1.0f,
    { 0.0f, 100.0f, -200.0f }, { 0.0f, 500.0f, 600.0f }, 5.0f, { 0.0f, 100.0f, -200.0f }, { 0.0f, 500.0f, 600.0f }, 5.0f, { 0.0f, 100.0f, -200.0f },
    { 0.0f, 500.0f, 600.0f } };

static s32 genieMotTbl[] = { DATA_MAKE_NUM(DATADIR_BOARD, 125), DATA_MAKE_NUM(DATADIR_BOARD, 126), DATA_MAKE_NUM(DATADIR_BOARD, 127), -1 };

/** @brief Run the isolated lamp-genie cutscene that carries the player toward the Star. */
static void GenieSceneExec(void)
{
    GXColor light_color = { 0xFF, 0xFF, 0xFF, 0xFF };
    Vec genie_position;
    Vec genie_rotation;
    Vec player_position;
    Vec saved_camera_position;
    Vec saved_camera_target;
    HUPROCESS *camera_process;
    float vertical_velocity;
    float elapsed_frames;
    s16 background_model;
    s16 genie_model;
    s16 floor_model;
    s16 particle_model;
    s16 fall_motion_started;
    s16 light;
    s32 camera_segment;
    s32 i;
    HU3DPARTICLE *particle;

    BoardModelHideSetAll(1);
    BoardSpaceHide(1);
    for (i = 0; i < 4; i++) {
        BoardStatusShowSet(i, 0);
    }
    BoardLightResetExec();
    background_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 128), NULL, 0);
    genie_position.x = genie_position.y = genie_position.z = 0.0f;
    BoardModelPosSetV(background_model, &genie_position);
    BoardModelCameraSet(background_model, 2);
    Hu3DModelAmbSet(BoardModelIDGet(background_model), 1.0f, 1.0f, 1.0f);
    floor_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 129), NULL, 0);
    BoardModelPosSetV(floor_model, &genie_position);
    BoardModelCameraSet(floor_model, 2);
    Hu3DModelAmbSet(BoardModelIDGet(floor_model), 1.0f, 1.0f, 1.0f);
    genie_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 124), genieMotTbl, 0);
    genie_position.x = genie_position.y = 0.0f;
    genie_position.z = 600.0f;
    genie_rotation.x = genie_rotation.z = 0.0f;
    genie_rotation.y = 180.0f;
    BoardModelPosSetV(genie_model, &genie_position);
    BoardModelRotSetV(genie_model, &genie_rotation);
    BoardModelCameraSet(genie_model, 2);
    BoardModelAttrSet(genie_model, 0x40000001);
    player_position.x = player_position.y = player_position.z = 0.0f;
    BoardPlayerCameraSet(currItemRestore, 2);
    BoardPlayerPosSetV(currItemRestore, &player_position);
    BoardModelVisibilitySet(BoardPlayerModelGet(currItemRestore), 1);
    VECSubtract(&shadowUp, &shadowPos, &saved_camera_position);
    VECNormalize(&saved_camera_position, &saved_camera_position);
    light = Hu3DGLightCreateV(&shadowPos, &saved_camera_position, &light_color);
    Hu3DShadowCreate(60.0f, 10.0f, 20000.0f);
    Hu3DModelShadowSet(BoardModelIDGet(BoardPlayerModelGet(currItemRestore)));
    Hu3DModelShadowSet(BoardModelIDGet(genie_model));
    Hu3DModelShadowMapSet(BoardModelIDGet(background_model));
    Hu3DShadowSizeSet(0xF0);
    Hu3DCameraCreate(2);
    booCamPos.x = 0.0f;
    booCamPos.y = 100.0f;
    booCamPos.z = 950.0f;
    booCamTarget.x = booCamTarget.z = 0.0f;
    booCamTarget.y = 1.0f;
    booCamUp.x = 0.0f;
    booCamUp.y = 100.0f;
    booCamUp.z = 600.0f;
    genieFov = 15.0f;
    Hu3DShadowData.fov = genieFov;
    C_MTXLightPerspective(Hu3DShadowData.projMtx, Hu3DShadowData.fov, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
    camera_process = HuPrcChildCreate(GenieCameraProc, 0x2005, 0x1000, 0, HuPrcCurrentGet());
    Hu3DBGColorSet(0xFF, 0xFF, 0xFF);
    particle_model = Hu3DParticleCreate(genieParticleAnim, 0x32);
    Hu3DParticleHookSet(particle_model, GenieParticleUpdate);
    Hu3DParticleBlendModeSet(particle_model, 1);
    Hu3DParticleColSet(particle_model, 0xFF, 0xFF, 0xFF);
    Hu3DParticleAnimModeSet(particle_model, 0);
    Hu3DParticleAttrSet(particle_model, 1);
    Hu3DModelLayerSet(particle_model, 7);
    Hu3DModelPosSet(particle_model, booCamPos.x, booCamPos.y, booCamPos.z - 175.0f);
    particle = Hu3DData[particle_model].hookData;
    particle->emitCnt = 1;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 45);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    BoardMusStart(1, 9, 0x7F, 0);
    while (particle->emitCnt != 0) {
        HuPrcVSleep();
    }
    HuPrcSleep(45);
    PlayerFXPlay(currItemRestore, 0x128);
    BoardPlayerMotionStart(currItemRestore, geniePlayerMot[1], 0);
    BoardPlayerMotionEndWait(currItemRestore);
    HuPrcSleep(25);
    Hu3DModelKill(particle_model);
    for (i = 0; i < 10; i++) {
        genieFov += 4.5f;
        HuPrcVSleep();
    }
    genieFov = 60.0f;
    HuPrcSleep(90);
    elapsed_frames = 0.0f;
    camera_segment = 0;
    while (1) {
        GenieCameraCalc(&cameraDataTbl[0], camera_segment, elapsed_frames / cameraDataTbl[camera_segment].duration, &booCamPos, &booCamUp);
        elapsed_frames += 1.0f;
        if (elapsed_frames >= cameraDataTbl[camera_segment].duration) {
            camera_segment++;
            elapsed_frames = 0.0f;
        }
        if (camera_segment >= 8) {
            break;
        }
        HuPrcVSleep();
    }
    GenieCameraCalc(&cameraDataTbl[0], camera_segment, 1.0f, &booCamPos, &booCamUp);
    HuPrcSleep(150);
    BoardPlayerIdleSet(currItemRestore);
    saved_camera_position = booCamPos;
    saved_camera_target = booCamUp;
    booCamUp.x = player_position.x + (genie_position.x - player_position.x) * 0.7f;
    booCamUp.y = player_position.y + 400.0f;
    booCamUp.z = player_position.z + (genie_position.z - player_position.z) * 0.7f;
    booCamPos.x = booCamUp.x + sind(-100) * 1100.0;
    booCamPos.y = booCamUp.y + 950.0f;
    booCamPos.z = booCamUp.z + cosd(-100) * 1100.0;
    HuAudFXPlay(0x356);
    BoardWinCreate(2, MAKE_MESSID(18, 27), -1);
    BoardWinWait();
    BoardWinKill();
    booCamUp = saved_camera_target;
    booCamPos = saved_camera_position;
    HuAudFXPlay(0x33E);
    HuAudFXPlay(0x367);
    BoardModelMotionStart(genie_model, 1, 0);
    fall_motion_started = 0;
    while (BoardModelMotionTimeGet(genie_model) <= 30.0f) {
        HuPrcVSleep();
    }
    elapsed_frames = 0.0f;
    while (1) {
        vertical_velocity = 40.0f - elapsed_frames * elapsed_frames * 0.25f * 0.0125f;
        if (vertical_velocity <= 0.0f && fall_motion_started == 0) {
            fall_motion_started = 1;
            BoardModelMotionShiftSet(genie_model, 3, 0.0f, 4.0f, HU3D_MOTATTR_NONE);
        }
        elapsed_frames += 1.0f;
        genie_position.y += vertical_velocity;
        if (genie_position.y <= 0.0f) {
            break;
        }
        BoardModelPosSetV(genie_model, &genie_position);
        booCamUp.y = genie_position.y + 140.0f;
        HuPrcVSleep();
    }
    HuAudFXPlay(0x368);
    genie_position.y = 0.0f;
    BoardModelPosSetV(genie_model, &genie_position);
    booCamUp.y = player_position.y + 100.0f;
    BoardModelMotionStart(genie_model, 2, 0);
    omVibrate(currItemRestore, 12, 0xC, 0);
    booCamUp.y = 500.0f;
    HuAudFXPlay(0x35B);
    HuAudFXPlay((GWPlayer[currItemRestore].character << 6) + 0x123);
    elapsed_frames = 0.0f;
    while (1) {
        vertical_velocity = 60.0f - elapsed_frames * elapsed_frames * 0.25f * 0.009166667f;
        elapsed_frames += 1.0f;
        player_position.y += vertical_velocity;
        BoardPlayerPosSetV(currItemRestore, &player_position);
        if (booCamUp.y < 1100.0f) {
            booCamUp.y = player_position.y + 160.0f;
        }
        if (player_position.y >= 2800.0f) {
            break;
        }
        HuPrcVSleep();
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 45);
    BoardAudSeqFadeOut(1, 1000);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    BoardModelKill(genie_model);
    BoardModelKill(background_model);
    BoardModelKill(floor_model);
    Hu3DCameraKill(2);
    Hu3DGLightKill(light);
    BoardModelHideSetAll(0);
    BoardSpaceHide(0);
    for (i = 0; i < 4; i++) {
        BoardStatusShowSet(i, 1);
    }
    HuPrcKill(camera_process);
    BoardPlayerCameraSet(currItemRestore, 1);
    BoardLightSetExec();
    HuAudFXPauseAll(0);
}

/** @brief Use the Magic Lamp, play its genie cutscene, and move the player to the current Star space. */
static void ExecItemGenie(void)
{
    UnkItemShowProcStruct lamp_work;
    Vec landing_position;
    Vec player_position;
    HU3DPARTICLE *particles;
    HUPROCESS *item_rotation_process;
    float bounce_velocity;
    float flight_time;
    float vertical_velocity;
    s16 particle_model;
    s16 impact_started;
    s32 star_space;
    s32 lamp_sfx;
    s32 frame;

    suitMdl = BoardModelCreate(itemMdlTbl[currItem], NULL, 1);
    BoardModelLayerSet(suitMdl, 2);
    genieParticleAnim = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_BOARD, 105));
    HuSprAnimLock(genieParticleAnim);
    geniePlayerMot[0] = BoardPlayerMotionCreate(currItemRestore, armUpMotTbl[GWPlayer[currItemRestore].character]);
    geniePlayerMot[1] = BoardPlayerMotionCreate(currItemRestore, scareMotTbl[GWPlayer[currItemRestore].character]);
    geniePlayerMot[2] = BoardPlayerMotionCreate(currItemRestore, impactMotTbl[GWPlayer[currItemRestore].character]);
    lamp_work.model = suitMdl;
    lamp_work.bob_amplitude = 4.0f;
    lamp_work.bob_speed = 3.0f;
    lamp_work.rotation_step_x = lamp_work.rotation_step_y = lamp_work.rotation_step_z = 0.0f;
    lamp_work.rotation_step_y = 4.0f;
    lamp_work.show_smoke = 1;
    HuAudFXPlay(0x350);
    item_rotation_process = ItemShowProc(&lamp_work, NULL);
    BoardCameraViewSet(3);
    HuAudFXPlay(0x323);
    frame = 0;
    while (1) {
        if (frame == 105) {
            BoardPlayerMotionStart(currItemRestore, geniePlayerMot[0], 0);
        }
        BoardModelRotGet(suitMdl, &player_position);
        if (90.0f - player_position.y <= lamp_work.rotation_step_y && 90.0f - player_position.y >= 0.0f && frame >= 120) {
            break;
        }
        HuPrcVSleep();
        frame++;
    }
    PlayerFXPlay(currItemRestore, 0x125);
    lamp_sfx = HuAudFXPlay(0x34D);
    particle_model = Hu3DParticleCreate(genieParticleAnim, 0x96);
    Hu3DParticleHookSet(particle_model, LampParticleUpdate);
    Hu3DParticleBlendModeSet(particle_model, 0);
    Hu3DParticleColSet(particle_model, 0xFF, 0xFF, 0xFF);
    Hu3DParticleAnimModeSet(particle_model, 0);
    Hu3DParticleAttrSet(particle_model, 1);
    Hu3DModelLayerSet(particle_model, 7);
    BoardModelPosGet(suitMdl, &player_position);
    Hu3DModelPosSet(particle_model, player_position.x, player_position.y, player_position.z);
    particles = Hu3DData[particle_model].hookData;
    omVibrate(currItemRestore, 181, 6, 6);
    HuPrcSleep(135);
    BoardAudSeqPause(0, 1, 1000);
    HuAudFXStop(lamp_sfx);
    HuAudFXPauseAll(1);
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 45);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    Hu3DModelKill(particle_model);
    BoardModelKill(suitMdl);
    HuPrcKill(item_rotation_process);
    BoardPlayerIdleSet(currItemRestore);
    GenieSceneExec();
    BoardCameraMoveSet(0);
    BoardCameraViewSet(2);
    star_space = BoardSpaceStarGetCurr();
    BoardCameraTargetSpaceSet(star_space);
    GWPlayer[currItemRestore].space_curr = star_space;
    BoardSpacePosGet(0, star_space, &player_position);
    landing_position = player_position;
    player_position.y += 1000.0f;
    BoardPlayerPosSetV(currItemRestore, &player_position);
    BoardCameraMotionWait();
    BoardCameraMoveSet(1);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 45);
    HuPrcSleep(15);
    BoardAudSeqPause(0, 0, 1000);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x35E);
    CharModelFxFlagSet(GWPlayer[currItemRestore].character, 0);
    BoardPlayerMotionStart(currItemRestore, 6, 0x40000001);
    bounce_velocity = 0.0f;
    flight_time = 1.0f;
    impact_started = 0;
    while (1) {
        vertical_velocity = bounce_velocity - flight_time * flight_time * 0.25f * 0.06666667f;
        if (vertical_velocity < -35.0f) {
            vertical_velocity = -35.0f;
        }
        if (vertical_velocity > 35.0f) {
            vertical_velocity = 35.0f;
        }
        flight_time += 1.0f;
        player_position.y += vertical_velocity;
        if (player_position.y <= landing_position.y) {
            omVibrate(currItemRestore, 12, 6, 6);
            player_position.y = landing_position.y;
            bounce_velocity = -vertical_velocity * 0.2f;
            if (fabs(bounce_velocity) <= 5.0) {
                flight_time = -1.0f;
                break;
            }
            flight_time = 1.0f;
            if (impact_started == 0) {
                BoardPlayerMotionStart(currItemRestore, geniePlayerMot[2], 0);
            }
        }
        BoardPlayerPosSetV(currItemRestore, &player_position);
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(currItemRestore);
    CharModelFxFlagSet(GWPlayer[currItemRestore].character, 1);
    HuSprAnimKill(genieParticleAnim);
    BoardPlayerMotionKill(currItemRestore, geniePlayerMot[0]);
    BoardPlayerMotionKill(currItemRestore, geniePlayerMot[1]);
    BoardPlayerMotionKill(currItemRestore, geniePlayerMot[2]);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Set the three item indices dispensed by the next Item Bag. @param arg0 Array of three item indices; -1 entries are randomized when the bag is used. */
void BoardItemBagItemSet(s16 *arg0)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        itemBagItems[i] = arg0[i];
    }
}

/** @brief Grow and launch one dispensed Item Bag model, then hide it in smoke. */
static void ExecItemBagJump(void)
{
    Vec item_scale;
    Vec item_position;
    Vec smoke_position;
    float vertical_velocity;
    float flight_time;
    float scale;
    s16 item_model;

    item_model = (s16)HuPrcCurrentGet()->user_data;
    BoardModelScaleGet(item_model, &item_scale);
    BoardModelPosGet(item_model, &item_position);
    smoke_position = item_position;
    smoke_position.y += 80.0f;
    scale = 0.0f;
    flight_time = 0.0f;
    HuAudFXPlay(0x310);
    while (1) {
        vertical_velocity = 8.0f - flight_time * flight_time * 0.25f * 0.075f;
        scale += 0.1f;
        if (scale > 1.0f) {
            scale = 1.0f;
        }
        item_scale.x = item_scale.y = item_scale.z = scale;
        item_position.y += vertical_velocity;
        BoardModelPosSetV(item_model, &item_position);
        BoardModelScaleSetV(item_model, &item_scale);
        flight_time += 1.0f;
        if (vertical_velocity < 0.0f) {
            break;
        }
        HuPrcVSleep();
    }
    CharEffectLayerSet(3);
    CharEffectSmokeCreate(1, &item_position);
    BoardModelVisibilitySet(item_model, 0);
    HuAudFXPlay(0x30D);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Briefly pause the bag's rotation while bobbing it for an item reveal. */
static void ExecItemBagShow(void)
{
    Vec base_position;
    Vec item_position;
    float bob_angle;
    HUPROCESS *process;
    UnkItemShowProcStruct *work;
    UnkItemShowProcStruct *resume_work;

    process = HuPrcCurrentGet()->user_data;
    work = process->user_data;
    work->paused = 1;
    BoardModelPosGet(suitMdl, &base_position);
    bob_angle = 0.0f;
    while (1) {
        if (bob_angle >= 180.0f) {
            break;
        }
        item_position = base_position;
        item_position.y += 15.0 * sind(bob_angle);
        BoardModelPosSetV(suitMdl, &item_position);
        bob_angle += 36.0f;
        HuPrcVSleep();
    }
    BoardModelPosSetV(suitMdl, &base_position);
    resume_work = process->user_data;
    resume_work->paused = 0;
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Use an Item Bag and add enough configured or random items to fill the inventory. */
static void ExecItemBag(void)
{
    UnkItemShowProcStruct bag_work;
    Vec item_position;
    Vec zero_scale;
    Vec camera_rotation;
    float model_offset_y;
    float window_size[2];
    s16 item_models[3];
    s16 window;
    s16 items_to_add;
    s16 com_delay;
    s16 items_added;
    s16 player_port;
    s16 i;
    u16 input;
    HUPROCESS *child_process;
    HUPROCESS *bag_rotation_process;

    for (i = 0; i < 3; i++) {
        if (itemBagItems[i] == -1) {
            itemBagItems[i] = rand8() % 13;
        }
    }
    player_port = GWPlayer[currItemRestore].port;
    suitMdl = BoardModelCreate(itemMdlTbl[currItem], NULL, 1);
    BoardModelLayerSet(suitMdl, 2);
    BoardCameraRotGet(&camera_rotation);
    BoardModelRotYSet(suitMdl, camera_rotation.y);
    items_to_add = items_added = 3 - BoardPlayerItemCount(currItemRestore);
    for (i = 0; i < items_to_add; i++) {
        item_models[i] = BoardModelCreate(itemMdlTbl[itemBagItems[i]], NULL, 1);
        if (itemBagItems[i] == 0xB) {
            BoardModelAttrSet(item_models[i], 0x40000001);
        }
        if (itemBagItems[i] == 5) {
            model_offset_y = -50.0f;
            Hu3DData[BoardModelIDGet(item_models[i])].mtx[1][3] = model_offset_y;
        }
        BoardModelRotYSet(item_models[i], camera_rotation.y);
        BoardModelLayerSet(item_models[i], 2);
        BoardModelVisibilitySet(item_models[i], 0);
    }
    bag_work.model = suitMdl;
    bag_work.bob_amplitude = 2.0f;
    bag_work.bob_speed = 5.0f;
    bag_work.rotation_step_x = bag_work.rotation_step_y = bag_work.rotation_step_z = 0.0f;
    bag_work.show_smoke = 0;
    HuAudFXPlay(0x350);
    bag_rotation_process = ItemShowProc(&bag_work, NULL);
    HuWinMesMaxSizeGet(1, window_size, MAKE_MESSID(18, 20));
    window = HuWinCreate(-10000.0f, 320.0f, window_size[0], window_size[1], 0);
    HuWinMesSet(window, MAKE_MESSID(18, 20));
    HuWinBGTPLvlSet(window, 0.0f);
    HuWinMesSpeedSet(window, 0);
    com_delay = frandmod(40) + 30;
    items_added = 0;
    zero_scale.x = zero_scale.y = zero_scale.z = 0.0f;
    while (items_added < items_to_add) {
        if (GWPlayerCfg[currItemRestore].iscom == 1) {
            if (--com_delay == 0) {
                input = 0x100;
                com_delay = frandmod(40) + 30;
            }
        }
        else {
            input = HuPadBtnDown[player_port];
        }
        if (!(input & 0x100)) {
            HuPrcVSleep();
        }
        else {
            BoardPlayerDiceJumpStart(currItemRestore);
            while (!BoardPlayerDiceJumpCheck(currItemRestore)) {
                HuPrcVSleep();
            }
            HuAudFXPlay(0x324);
            BoardModelPosGet(suitMdl, &item_position);
            BoardModelPosSetV(item_models[items_added], &item_position);
            BoardModelScaleSetV(item_models[items_added], &zero_scale);
            BoardModelVisibilitySet(item_models[items_added], 1);
            child_process = HuPrcChildCreate(ExecItemBagJump, 0x2004, 0x1000, 0, HuPrcCurrentGet());
            child_process->user_data = (void *)item_models[items_added];
            child_process = HuPrcChildCreate(ExecItemBagShow, 0x2004, 0x1000, 0, HuPrcCurrentGet());
            child_process->user_data = bag_rotation_process;
            while (GWPlayer[currItemRestore].jump) {
                HuPrcVSleep();
            }
            HuPrcChildWatch();
            BoardPlayerItemAdd(currItemRestore, itemBagItems[items_added]);
            BoardPlayerMotionShiftSet(currItemRestore, 1, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
            HuPrcSleep(10);
            items_added++;
        }
    }
    HuWinKill(window);
    BoardModelPosGet(suitMdl, &item_position);
    CharEffectLayerSet(3);
    CharEffectSmokeCreate(1, &item_position);
    HuAudFXPlay(0x351);
    HuPrcSleep(10);
    HuPrcKill(bag_rotation_process);
    BoardModelKill(suitMdl);
    for (i = 0; i < items_to_add; i++) {
        BoardModelKill(item_models[i]);
    }
    HuPrcSleep(30);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Finish restoration for items that leave no persistent player state. */
static void RestoreItemNull(void)
{
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Restore a player from Mini Mushroom size to normal size. */
static void RestoreItemMini(void)
{
    HuAudFXPlay(0x311);
    ItemSizeSet(0);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Restore a player from Mega Mushroom size to normal size. */
static void RestoreItemMega(void)
{
    HuAudFXPlay(0x313);
    ItemSizeSet(0);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Restore a player from Super Mini Mushroom size to normal size. */
static void RestoreItemMiniSuper(void)
{
    HuAudFXPlay(0x311);
    ItemSizeSet(0);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Restore a player from Super Mega Mushroom size to normal size. */
static void RestoreItemMegaSuper(void)
{
    HuAudFXPlay(0x313);
    ItemSizeSet(0);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

/** @brief Remove the Bowser Suit and reveal the player's normal model. */
static void RestoreItemBowser(void)
{
    Vec effect_scale;
    Vec effect_position;
    Vec player_scale;
    s16 effect_model;
    s16 suit_player_model;
    s16 i;

    effect_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 99), NULL, 0);
    BoardModelVisibilitySet(effect_model, 0);
    BoardModelLayerSet(effect_model, 3);
    BoardPlayerPosGet(currItemRestore, &effect_position);
    effect_scale.x = effect_scale.y = effect_scale.z = 0.0f;
    effect_position.y += 100.0f;
    BoardModelPosSetV(effect_model, &effect_position);
    BoardModelScaleSetV(effect_model, &effect_scale);
    BoardModelAttrSet(effect_model, 0x40000001);
    BoardModelAttrSet(effect_model, 0x400000);
    suitMdl = BoardBowserSuitModelGet();
    suit_player_model = BoardBowserSuitPlayerModelGet();
    BoardModelMotionShiftSet(suitMdl, suitMot[0], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    BoardModelMotionShiftSet(suit_player_model, suitCommonMot[0], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(40);
    PlayerFXPlay(currItemRestore, 0x129);
    HuPrcSleep(20);
    HuAudFXPlay(0x32B);
    BoardModelVisibilitySet(effect_model, 1);
    for (i = 0; i < 60; i++) {
        effect_scale.x += 0.15f;
        effect_scale.y += 0.15f;
        effect_scale.z += 0.15f;
        BoardModelScaleSetV(effect_model, &effect_scale);
        HuPrcVSleep();
    }
    effect_scale.x = effect_scale.y = effect_scale.z = 9.0f;
    BoardModelScaleSetV(effect_model, &effect_scale);
    HuPrcSleep(60);
    HuAudFXPlay(0x313);
    BoardModelHookReset(suitMdl);
    BoardBowserSuitKill(currItemRestore);
    player_scale.x = player_scale.y = player_scale.z = 1.0f;
    BoardPlayerScaleSetV(currItemRestore, &player_scale);
    BoardModelVisibilitySet(BoardPlayerModelGet(currItemRestore), 1);
    for (i = 0; i < 60; i++) {
        effect_scale.x -= 0.15f;
        effect_scale.y -= 0.15f;
        effect_scale.z -= 0.15f;
        BoardModelScaleSetV(effect_model, &effect_scale);
        HuPrcVSleep();
    }
    BoardModelKill(effect_model);
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}
