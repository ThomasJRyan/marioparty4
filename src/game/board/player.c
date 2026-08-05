#include "types.h"
#include "game/board/player.h"

#include "ext_math.h"
#include "game/board/audio.h"
#include "game/board/basic_space.h"
#include "game/board/com.h"
#include "game/board/item.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/roll.h"
#include "game/board/space.h"
#include "game/board/tutorial.h"
#include "game/board/ui.h"
#include "game/board/view.h"
#include "game/board/window.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"

#include "ext_math.h"
#include <stdlib.h>
#include <string.h>

static void InitJunction(s32, s32, f32);
static void UpdateJunctionGfx(OMOBJ *);
static void StopJunctionPlayer(s32);
static void RestoreJunction(f32, s32);

static s32 GetDefaultDirection(f32, f32 *, s32);
static s32 DoDebugMove(s32, s16 *);
static s32 ExecJunction(s32, s16 *);

static void PlayerPosLerpFunc(OMOBJ *);
static void DiceJumpFunc(OMOBJ *);

static void UpdateRollSprite(OMOBJ *);
static void UpdateRoll(OMOBJ *);
static void MoveAwayObjFunc(OMOBJ *);

static void MoveAwayObjFunc(OMOBJ *);
static void UpdateBowserSuit(OMOBJ *);

static void MegaSquishFunc(OMOBJ *object);

static s32 DoSparkSpace(s32 player, s32 pause_cam);
static void RemoveSparkSpace(s32 player);
static void SetPlayerSizeAuto(s32 player);
static s32 MegaPlayerPassFunc(s32 player, s32 space);
static s32 MegaExecJump(s32 player, s32 space);

typedef struct junction_work {
    struct {
        u8 kill : 1;
        u8 player : 2;
        u8 arrow_count : 2;
        u8 selected_arrow : 2;
        u8 remove_requested : 1;
    };
    u8 selection_delay;
    s16 pulse_angle;
    s16 window;
    s16 arrow_models[5];
} JunctionWork;

static HSFMATERIAL *playerMatCopy[4];
static s32 (*postTurnHook[4])();
static s32 (*preTurnHook[4])();

s16 boardPlayerMdl[4];
static s16 playerMot[4];
static s8 itemPrev;
static s8 moveAwayPlayer[4];
static s16 junctionArrowRot[4];
static OMOBJ *moveAwayObj;
static OMOBJ *rollObj;
static OMOBJ *junctionObj;
static s32 junctionMask;
static OMOBJ *bowserSuitObj;
static s32 megaDoubleDiceF;

static s16 suitMdl = -1;
static s16 suitPlayerMdl = -1;
static s16 suitCurrMot = -1;

static OMOBJ *diceJumpObj[4] = { 0, 0, 0, 0 };
static OMOBJ *motDoneF[4] = { 0, 0, 0, 0 };
static s16 bowserSuitMot[5] = { -1, -1, -1, -1, -1 };
static char *eyeMatTbl[8][2]
    = { { "eye1", "eye2" }, { "eye1", "eye2" }, { "mat14", "mat16" }, { "eye1", "eye2" }, { "Clswario_eye_l1_AUTO14", "Clswario_eye_l1_AUTO15" },
          { "m_donkey_eye4", "m_donkey_eye5" }, { "mat65", "mat66" }, { "Clswaluigi_eye_l1_AUTO1", "Clswaluigi_eye_l1_AUTO2" } };

s32 lbl_8013997C[] = {
    0x10D,
    0x14D,
    0x18D,
    0x1CD,
    0x20D,
    0x24D,
    0x28D,
    0x2CD,
};

static s32 boardMotTbl[] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x35),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x35),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x35),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x35),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x35),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x35),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x35),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x35),
};

static s32 boardMotRevTbl[] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x36),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x36),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x36),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x36),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x36),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x36),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x36),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x36),
};

static s32 bowserSuitCharMdlTbl[] = {
    DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x00),
    DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x03),
    DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x06),
    DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x09),
    DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x0C),
    DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x0F),
    DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x12),
    DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x15),
};

static s32 bowserSuitMotTbl[] = { DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x19), DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x1A),
    DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x1B), DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x1E), DATA_MAKE_NUM(DATADIR_BKOOPASUIT, 0x1F) };

#if VERSION_JP
static s32 sparkMotTbl[] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 0x94),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x95),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x96),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x97),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x98),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x99),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x9A),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x9B),
};
#else
static s32 sparkMotTbl[] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 0x95),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x96),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x97),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x98),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x99),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x9A),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x9B),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x9C),
};
#endif

static s32 boardSparkSfxTbl[] = {
    0x128,
    0x168,
    0x1A8,
    0x1E8,
    0x228,
    0x268,
    0x2A8,
    0x2E8,
};

static s32 boardSparkSfxTblAlt[] = {
    0x123,
    0x163,
    0x1A3,
    0x1E3,
    0x223,
    0x263,
    0x2A3,
    0x2E3,
};

static OMOBJ *megaSquishObj[4] = {};

static s32 megaSquishSfxTbl[] = {
    0x128,
    0x168,
    0x1A8,
    0x1E8,
    0x228,
    0x268,
    0x2A8,
    0x2E8,
};

/**
 * @brief Gets the item used before the current board movement.
 *
 * @return The stored item ID, or -1 when no prior item is active.
 */
s32 BoardItemPrevGet(void)
{
    return itemPrev;
}

/**
 * @brief Stores the item used before the current board movement.
 *
 * @param type Item ID to store.
 */
void BoardItemPrevSet(s32 type)
{
    itemPrev = type;
}

/**
 * @brief Gets the character message-table index for a player.
 *
 * @param player Player index.
 *
 * @return The character message-table index.
 */
s32 BoardPlayerGetCharMess(s32 player)
{
    s32 charMess[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

    return charMess[GWPlayer[player].character];
}

/**
 * @brief Gets the display name for a player character.
 *
 * @param player Player index.
 *
 * @return A pointer to the static character name string.
 */
char *BoardPlayerGetCharName(s32 player)
{
    char *charNames[8] = { "Mario", "Luigi", "Peach", "Yoshi", "Wario", "Donky", "Daisy", "Waluigi" };
    return charNames[GWPlayer[player].character];
}

/**
 * @brief Checks whether all four players are CPU-controlled.
 *
 * @return 1 if all players are CPU-controlled, otherwise 0.
 */
s32 BoardPlayerIsAllCom(void)
{
    if ((GWPlayer[0].com != 0) && (GWPlayer[1].com != 0) && (GWPlayer[2].com != 0) && (GWPlayer[3].com != 0)) {
        return 1;
    }
    return 0;
}

/**
 * @brief Initializes board-specific player state.
 */
void BoardPlayerInit(void)
{
    s32 start_space;
    s32 player;

    if (BoardStartCheck() == 0) {
        GWSystem.player_curr = 0;
        start_space = BoardSpaceFlagPosGet(0, 0x80000000, 0);

        for (player = 0; player < 4; player++) {
            GWPlayer[player].space_curr = start_space;
            BoardPlayerCornerPosSet(player);
            BoardPlayerSizeSet(player, 0);
            GWPlayer[player].color = 0;
            GWPlayer[player].bowser_suit = 0;
            GWPlayer[player].draw_ticket = 0;
            GWPlayer[player].jump = 0;
        }
        return;
    }

    for (player = 0; player < 4; player++) {
        BoardPlayerCornerPosSet(player);
        if (BoardPlayerAutoSizeGet(player) != 0) {
            BoardPlayerAutoSizeSet(player, BoardPlayerAutoSizeGet(player));
            BoardStatusHammerShowSet(player, 0);
        }
    }
}

/**
 * @brief Creates player models and initializes their board state.
 *
 * @details Restores team data when required, creates each character model and its board motions, initializes new-game inventory state, and records controller and CPU configuration.
 */
void BoardPlayerModelInit(void)
{
    s32 player;
    s32 i;
    s32 character;
    s32 saved_team;
    s32 configured_team;
    s16 model;
    s32 unused_model_value;
    PlayerState *player_state;

    s32 ro0[8] = { DATA_MAKE_NUM(DATADIR_MARIOMDL1, 0x00), DATA_MAKE_NUM(DATADIR_LUIGIMDL1, 0x00), DATA_MAKE_NUM(DATADIR_PEACHMDL1, 0x00),
        DATA_MAKE_NUM(DATADIR_YOSHIMDL1, 0x00), DATA_MAKE_NUM(DATADIR_WARIOMDL1, 0x00), DATA_MAKE_NUM(DATADIR_DONKEYMDL1, 0x00),
        DATA_MAKE_NUM(DATADIR_DAISYMDL1, 0), DATA_MAKE_NUM(DATADIR_WALUIGIMDL1, 0x00) };
    s32 ro1[8] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00), DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00), DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00) };
    u16 ro2[] = { 0, 2, 3, 5, 6, 22, 23, 24, 27, 31, 47, 72, 73 };
    s32 motion_list[ARRAY_COUNT(ro2)+1];

    memset(&boardPlayerMdl, 0, sizeof(boardPlayerMdl));
    suitMdl = -1;
    suitPlayerMdl = -1;
    if ((_CheckFlag(FLAG_ID_MAKE(1, 5)) != 0) || (_CheckFlag(FLAG_ID_MAKE(1, 6)) && (GWBoardGet() == BOARD_ID_MAIN6))) {
        for (player = 0; player < 4; player++) {
            saved_team = GWPlayer[player].team_backup;
            GWPlayer[player].team = saved_team;
            GWPlayerCfg[player].group = saved_team;
        }
    }
    for (player = 0; player < 4; player++) {
        preTurnHook[player] = 0;
        postTurnHook[player] = 0;
        motDoneF[player] = 0;
        diceJumpObj[player] = 0;
        player_state = BoardPlayerGet(player);
        character = GWPlayerCfg[player].character;
        player_state->player_idx = player;
        GWPlayer[player].character = character;
        GWPlayerCfg[player].character = character;

        for (i = 0; i < ARRAY_COUNT(ro2); i++) {
            motion_list[i] = ro1[character] | ro2[i];
        }
        motion_list[i] = -1;
        model = BoardModelCreateCharacter(character, ro0[character], motion_list, 0);
        boardPlayerMdl[player] = model;
        BoardPlayerCopyMat(player);
        playerMot[player] = 1;
        BoardModelMotionStart(model, playerMot[player], 0x40000001);
        if (BoardStartCheck() == 0) {
            for (i = 0; i < ARRAY_COUNT(player_state->items); i++) {
                player_state->items[i] = BOARD_ITEM_NONE;
            }
            configured_team = GWPlayerCfg[player].group;
            GWPlayer[player].team = configured_team;
            GWPlayerCfg[player].group = configured_team;
        }
        GWPlayer[player].show_next = 1;
        GWPlayer[player].bowser_suit = 0;
        BoardModelExistDupe(model, 1);
        BoardModelExistCheck(model, 1);
        fn_8006DDE8(model, -1.0f);
        GWPlayer[player].com = GWPlayerCfg[player].iscom;
        GWPlayerCfg[player].iscom = GWPlayerCfg[player].iscom;
        GWPlayer[player].port = GWPlayerCfg[player].pad_idx;
        GWPlayerCfg[player].pad_idx = GWPlayerCfg[player].pad_idx;
        GWPlayer[player].diff = GWPlayerCfg[player].diff;
        GWPlayerCfg[player].diff = GWPlayerCfg[player].diff;
        player_state->space_prev = -1;
        CharModelDataClose(GWPlayer[player].character);
    }
    CharEffectLayerSet(2);
}

/**
 * @brief Destroys player models and associated resources.
 */
void BoardPlayerModelKill(void)
{
    PlayerState *player_state;
    PlayerState *unused_player_state;
    s32 i;

    for (i = 0; i < 4; i++) {
        player_state = BoardPlayerGet(i);
        if (boardPlayerMdl[player_state->player_idx] != -1) {
            BoardModelKill(boardPlayerMdl[player_state->player_idx]);
            boardPlayerMdl[player_state->player_idx] = -1;
        }
        if (playerMatCopy[i] != 0) {
            HuMemDirectFree(playerMatCopy[i]);
            playerMatCopy[i] = 0;
        }
        BoardBowserSuitKill(i);
    }
}

/**
 * @brief Sets a player model render layer.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Render layer.
 */
void BoardPlayerLayerSet(s32 arg0, s32 arg1)
{
    BoardModelLayerSet(BoardPlayerModelGet(arg0), arg1);
}

/**
 * @brief Sets the camera mask for a player model.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Camera mask.
 */
void BoardPlayerCameraSet(s32 arg0, u16 arg1)
{
    BoardModelCameraSet(BoardPlayerModelGet(arg0), arg1);
}

/**
 * @brief Sets the player model's unidentified `unk_D4` value.
 *
 * @details The model layer normalizes every negative value to -1. The effect
 * of `BoardModel.unk_D4` is not yet identified.
 *
 * @param arg0 Player whose board model is updated.
 *
 * @param arg8 Property value; its exact semantics are not yet identified.
 */
void fn_80062A40(s32 arg0, f32 arg8)
{
    fn_8006DDE8(BoardPlayerModelGet(arg0), arg8);
}

/**
 * @brief Validates a player model through the model existence-check wrapper.
 *
 * @details The current model-layer implementation only verifies that the model
 * handle resolves; it does not inspect the forwarded flag.
 *
 * @param arg0 Player whose board model is checked.
 *
 * @param arg1 Flag forwarded to `BoardModelExistCheck`; currently unused there.
 */
void BoardPlayerExistCheck(s32 arg0, s32 arg1)
{
    if (arg1 == 0) {
        BoardModelExistCheck(BoardPlayerModelGet(arg0), 0);
    }
    else {
        BoardModelExistCheck(BoardPlayerModelGet(arg0), 1);
    }
}

/**
 * @brief Adds an item to the first free player inventory slot.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Item ID to add.
 *
 * @return The inventory slot used, or -1 if the inventory is full.
 */
s32 BoardPlayerItemAdd(s32 arg0, s32 arg1)
{
    PlayerState *player_state;
    s32 added_slot;
    s32 item_slot;

    added_slot = -1;
    player_state = BoardPlayerGet(arg0);
    for (item_slot = 0; item_slot < ARRAY_COUNT(GWPlayer[arg0].items); item_slot++) {
        if (GWPlayer[arg0].items[item_slot] == BOARD_ITEM_NONE) {
            HuAudFXPlay(0x360);
            player_state->items[item_slot] = arg1;
            BoardItemStatusKill(arg0);
            added_slot = item_slot;
            break;
        }
    }
    return added_slot;
}

/**
 * @brief Removes and returns an item from a player inventory slot.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Inventory slot to remove.
 *
 * @return The removed item ID, or BOARD_ITEM_NONE for an empty slot.
 */
s32 BoardPlayerItemRemove(s32 arg0, s32 arg1)
{
    s32 removed_item;
    PlayerState *player_state;

    player_state = BoardPlayerGet(arg0);
    removed_item = player_state->items[arg1];
    if (player_state->items[arg1] == BOARD_ITEM_NONE) {
        return removed_item;
    }
    if (arg1 == 0) {
        player_state->items[0] = player_state->items[1];
    }
    if ((arg1 == 1) || (arg1 == 0)) {
        player_state->items[1] = player_state->items[2];
    }
    player_state->items[2] = BOARD_ITEM_NONE;
    HuAudFXPlay(0x363);
    BoardItemStatusKill(arg0);
    return removed_item;
}

/**
 * @brief Finds an item in a player inventory.
 *
 * @param playerNo Player index.
 *
 * @param item Item ID to find.
 *
 * @return The matching inventory slot, or -1 if not found.
 */
s32 BoardPlayerItemFind(s32 playerNo, s32 item)
{
    s32 inv_slot;

    for (inv_slot = 0; inv_slot < ARRAY_COUNT(GWPlayer[playerNo].items); inv_slot++) {
        if (item == GWPlayer[playerNo].items[inv_slot]) {
            return inv_slot;
        }
    }

    return -1;
}

/**
 * @brief Counts the items in a player inventory.
 *
 * @param playerNo Player index.
 *
 * @return The number of occupied inventory slots.
 */
s32 BoardPlayerItemCount(s32 playerNo)
{
    s32 itemCount;
    s32 i;

    for (i = 0, itemCount = i; i < ARRAY_COUNT(GWPlayer[playerNo].items); i++) {
        if (GWPlayer[playerNo].items[i] != BOARD_ITEM_NONE) {
            itemCount++;
        }
    }

    return itemCount;
}

/**
 * @brief Moves a player to their assigned corner of the current space.
 *
 * @param arg0 Player index.
 */
void BoardPlayerCornerPosSet(s32 arg0)
{
    Vec corner_pos;

    BoardPlayerCornerPosGet(arg0, &corner_pos);
    BoardPlayerPosSetV(arg0, &corner_pos);
}

/**
 * @brief Gets a player corner position on the current space.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Output corner position.
 */
void BoardPlayerCornerPosGet(s32 arg0, Vec *arg1)
{
    s32 ordered_player;
    s32 players_checked;
    s32 corner_index;
    s32 space;

    if (arg1 != 0) {
        space = GWPlayer[arg0].space_curr;
        ordered_player = GWSystem.player_curr;
        if (ordered_player == -1) {
            ordered_player = 0;
        }
        for (players_checked = 0, corner_index = players_checked; players_checked < 4; ordered_player = (ordered_player + 1) & 3, players_checked++) {
            if (ordered_player == arg0)
                break;
            if (space == GWPlayer[ordered_player].space_curr) {
                corner_index += 1;
            }
        }
        BoardSpaceCornerPosGet(space, corner_index, arg1);
    }
}

/**
 * @brief Sets the ambient color of a player model.
 *
 * @param arg0 Player index.
 *
 * @param arg8 Red ambient component.
 *
 * @param arg9 Green ambient component.
 *
 * @param argA Blue ambient component.
 */
void BoardPlayerAmbSet(s32 arg0, f32 arg8, f32 arg9, f32 argA)
{
    BoardModelAmbSet(BoardPlayerModelGet(arg0), arg8, arg9, argA);
}

/**
 * @brief Sets the transform matrix of a player model.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Transform matrix.
 */
void BoardPlayerMtxSet(s32 arg0, Mtx *arg1)
{
    BoardModelMtxSet(BoardPlayerModelGet(arg0), arg1);
}

/**
 * @brief Sets a player position from a vector.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Position vector.
 */
void BoardPlayerPosSetV(s32 arg0, Vec *arg1)
{
    BoardModelPosSetV(BoardPlayerModelGet(arg0), arg1);

    if (GWPlayer[arg0].bowser_suit != 0) {
        BoardModelPosSetV(suitMdl, arg1);
    }
}

/**
 * @brief Sets a player position from individual coordinates.
 *
 * @param arg0 Player index.
 *
 * @param arg8 X coordinate.
 *
 * @param arg9 Y coordinate.
 *
 * @param argA Z coordinate.
 */
void BoardPlayerPosSet(s32 arg0, f32 arg8, f32 arg9, f32 argA)
{
    BoardModelPosSet(BoardPlayerModelGet(arg0), arg8, arg9, argA);

    if (GWPlayer[arg0].bowser_suit != 0) {
        BoardModelPosSet(suitMdl, arg8, arg9, argA);
    }
}

/**
 * @brief Gets a player position.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Output position.
 */
void BoardPlayerPosGet(s32 arg0, Vec *arg1)
{
    BoardModelPosGet(BoardPlayerModelGet(arg0), arg1);
}

/**
 * @brief Sets a player rotation from a vector.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Rotation vector.
 */
void BoardPlayerRotSetV(s32 arg0, Vec *arg1)
{
    BoardModelRotSetV(BoardPlayerModelGet(arg0), arg1);

    if (GWPlayer[arg0].bowser_suit != 0) {
        BoardModelRotSetV(suitMdl, arg1);
    }
}

/**
 * @brief Sets a player rotation from individual angles.
 *
 * @param arg0 Player index.
 *
 * @param arg8 X rotation.
 *
 * @param arg9 Y rotation.
 *
 * @param argA Z rotation.
 */
void BoardPlayerRotSet(s32 arg0, f32 arg8, f32 arg9, f32 argA)
{
    BoardModelRotSet(BoardPlayerModelGet(arg0), arg8, arg9, argA);

    if (GWPlayer[arg0].bowser_suit != 0) {
        BoardModelRotSet(suitMdl, arg8, arg9, argA);
    }
}

/**
 * @brief Gets a player rotation.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Output rotation.
 */
void BoardPlayerRotGet(s32 arg0, Vec *arg1)
{
    BoardModelRotGet(BoardPlayerModelGet(arg0), arg1);
}

/**
 * @brief Sets and normalizes a player yaw angle.
 *
 * @param arg0 Player index.
 *
 * @param arg8 Yaw angle in degrees.
 */
void BoardPlayerRotYSet(s32 arg0, f32 arg8)
{
    if (arg8 < 0.0f) {
        arg8 += 360.0f;
    }
    if (arg8 > 360.0f) {
        arg8 -= 360.0f;
    }
    BoardModelRotYSet(BoardPlayerModelGet(arg0), arg8);
    if (GWPlayer[arg0].bowser_suit != 0) {
        BoardModelRotYSet(suitMdl, arg8);
    }
}

/**
 * @brief Gets a player yaw angle.
 *
 * @param arg0 Player index.
 *
 * @return The player model yaw in degrees.
 */
f32 BoardPlayerRotYGet(s32 arg0)
{
    f32 rotation_y;

    if (GWPlayer[arg0].bowser_suit != 0) {
        rotation_y = BoardModelRotYGet(suitMdl);
    }
    else {
        rotation_y = BoardModelRotYGet(BoardPlayerModelGet(arg0));
    }
    return rotation_y;
}

/**
 * @brief Sets a player model scale from a vector.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Scale vector.
 */
void BoardPlayerScaleSetV(s32 arg0, Vec *arg1)
{
    BoardModelScaleSetV(BoardPlayerModelGet(arg0), arg1);
}

/**
 * @brief Sets a player model scale from individual components.
 *
 * @param arg0 Player index.
 *
 * @param arg8 X scale.
 *
 * @param arg9 Y scale.
 *
 * @param argA Z scale.
 */
void BoardPlayerScaleSet(s32 arg0, f32 arg8, f32 arg9, f32 argA)
{
    BoardModelScaleSet(BoardPlayerModelGet(arg0), arg8, arg9, argA);
}

/**
 * @brief Gets a player model scale.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Output scale.
 */
void BoardPlayerScaleGet(s32 arg0, Vec *arg1)
{
    BoardModelScaleGet(BoardPlayerModelGet(arg0), arg1);
}

/**
 * @brief Gets the tracked current motion for a player.
 *
 * @param arg0 Player index.
 *
 * @return The tracked player motion index.
 */
s16 BoardPlayerCurrMotionGet(s32 arg0)
{
    PlayerState *player = BoardPlayerGet(arg0);

    return playerMot[player->player_idx];
}

/**
 * @brief Enables or disables a player model voice channel.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Motion slot whose associated voice is toggled.
 *
 * @param arg2 Nonzero to enable the channel.
 */
void BoardPlayerVoiceEnableSet(s32 arg0, s32 arg1, s32 arg2)
{
    BoardModelVoiceEnableSet(BoardPlayerModelGet(arg0), arg1, arg2);
}

/**
 * @brief Creates a motion for a player model.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Motion data ID.
 *
 * @return The created motion handle.
 */
s32 BoardPlayerMotionCreate(s32 arg0, s32 arg1)
{
    return BoardModelMotionCreate(BoardPlayerModelGet(arg0), arg1);
}

/**
 * @brief Destroys a player model motion.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Motion handle.
 *
 * @return The underlying model motion removal result.
 */
s32 BoardPlayerMotionKill(s32 arg0, s32 arg1)
{
    return BoardModelMotionKill(BoardPlayerModelGet(arg0), arg1);
}

/**
 * @brief Checks whether a player motion has ended.
 *
 * @param arg0 Player index.
 *
 * @return Nonzero if the motion has ended, otherwise zero.
 */
s32 BoardPlayerMotionEndCheck(s32 arg0)
{
    return BoardModelMotionEndCheck(BoardPlayerModelGet(arg0));
}

/**
 * @brief Waits until a player motion has ended.
 *
 * @param arg0 Player index.
 */
void BoardPlayerMotionEndWait(s32 arg0)
{
    while (BoardModelMotionEndCheck(BoardPlayerModelGet(arg0)) == 0) {
        HuPrcVSleep();
    }
}

/**
 * @brief Starts a player motion when it differs from the tracked motion.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Motion index.
 *
 * @param arg2 Motion attributes.
 */
void BoardPlayerMotionStart(s32 arg0, s32 arg1, s32 arg2)
{
    PlayerState *player;
    s32 start_result;

    player = BoardPlayerGet(arg0);
    if (arg1 == 0) { }
    if (arg1 != playerMot[player->player_idx]) {
        start_result = BoardModelMotionStart(BoardPlayerModelGet(arg0), arg1, arg2);
        if (start_result == 0) {
            playerMot[player->player_idx] = arg1;
        }
    }
}

/**
 * @brief Blends a player model to a different motion.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Motion index.
 *
 * @param arg8 Starting motion time.
 *
 * @param arg9 Blend duration.
 *
 * @param arg2 Motion attributes.
 */
void BoardPlayerMotionShiftSet(s32 arg0, s32 arg1, f32 arg8, f32 arg9, u32 arg2)
{
    s32 shift_result;
    PlayerState *player;

    player = BoardPlayerGet(arg0);
    if (arg1 == 0) { }
    if (arg1 != playerMot[player->player_idx]) {
        shift_result = BoardModelMotionShiftSet(BoardPlayerModelGet(arg0), arg1, arg8, arg9, arg2);
        if (shift_result == 0) {
            playerMot[player->player_idx] = arg1;
        }
    }
}

/**
 * @brief Sets a player motion playback speed.
 *
 * @param arg0 Player index.
 *
 * @param arg8 Playback speed.
 */
void BoardPlayerMotionSpeedSet(s32 arg0, f32 arg8)
{
    BoardModelMotionSpeedSet(BoardPlayerModelGet(arg0), arg8);
}

/**
 * @brief Sets a player motion playback time.
 *
 * @param arg0 Player index.
 *
 * @param arg8 Playback time.
 */
void BoardPlayerMotionTimeSet(s32 arg0, f32 arg8)
{
    BoardModelMotionTimeSet(BoardPlayerModelGet(arg0), arg8);
}

/**
 * @brief Gets a player motion playback time.
 *
 * @param arg0 Player index.
 *
 * @return The current motion time.
 */
f32 BoardPlayerMotionTimeGet(s32 arg0)
{
    return BoardModelMotionTimeGet(BoardPlayerModelGet(arg0));
}

/**
 * @brief Gets the maximum time of a player motion.
 *
 * @param arg0 Player index.
 *
 * @return The maximum motion time.
 */
f32 BoardPlayerMotionMaxTimeGet(s32 arg0)
{
    return BoardModelMotionMaxTimeGet(BoardPlayerModelGet(arg0));
}

/**
 * @brief Sets the playback range of a player motion.
 *
 * @param arg0 Player index.
 *
 * @param arg8 Start time.
 *
 * @param arg9 End time.
 */
void BoardPlayerMotionStartEndSet(s32 arg0, f32 arg8, f32 arg9)
{
    BoardModelMotionStartEndSet(BoardPlayerModelGet(arg0), arg8, arg9);
}

/**
 * @brief Sets attributes on a player model.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Attribute mask.
 */
void BoardPlayerModelAttrSet(s32 arg0, s32 arg1)
{
    BoardModelAttrSet(BoardPlayerModelGet(arg0), arg1);
}

/**
 * @brief Clears attributes on a player model.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Attribute mask.
 */
void BoardPlayerModelAttrReset(s32 arg0, s32 arg1)
{
    BoardModelAttrReset(BoardPlayerModelGet(arg0), arg1);
}

/**
 * @brief Sets a player coin count.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Coin count.
 */
void BoardPlayerCoinsSet(s32 arg0, s32 arg1)
{
    GWCoinsSet(arg0, arg1);
}

/**
 * @brief Gets a player coin count.
 *
 * @param arg0 Player index.
 *
 * @return The current coin count.
 */
s32 BoardPlayerCoinsGet(s32 arg0)
{
    return GWCoinsGet(arg0);
}

/**
 * @brief Adds to a player coin count and lifetime total.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Signed coin delta.
 */
void BoardPlayerCoinsAdd(s32 arg0, s32 arg1)
{
    PlayerState *player;
    s16 coins;

    player = BoardPlayerGet(arg0);
    if ((arg1 > 0) && (player->coins_total < 0x3E7)) {
        player->coins_total += arg1;
        if (player->coins_total > 0x3E7) {
            player->coins_total = 0x3E7;
        }
    }
    BoardPlayerCoinsSet(arg0, arg1 + BoardPlayerCoinsGet(arg0));
}

/**
 * @brief Adds to a player star count.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Signed star delta.
 */
void BoardPlayerStarsAdd(s32 arg0, s32 arg1)
{
    GWStarsSet(arg0, (arg1 + GWStarsGet(arg0)));
}

/**
 * @brief Sets a player size state and model scale.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Size state.
 */
void BoardPlayerSizeSet(s32 arg0, s32 arg1)
{
    PlayerState *player_state;
    Vec size_scales[3] = { { 1, 1, 1 }, { 0.3, 0.3, 0.3 }, { 2.5, 2.5, 2.5 } };

    player_state = BoardPlayerGet(arg0);
    player_state->size = arg1;
    if (arg1 == 2) {
        CharModelStepFxSet(GWPlayer[arg0].character, 4);
    }
    else if (arg1 == 1) {
        CharModelStepFxSet(GWPlayer[arg0].character, 5);
    }
    else {
        CharModelStepFxSet(GWPlayer[arg0].character, 0);
    }
    BoardPlayerScaleSetV(arg0, &size_scales[arg1]);
}

/**
 * @brief Gets a player size state.
 *
 * @param idx Player index.
 *
 * @return The size state, or no defined value if the player state is unavailable.
 */
s32 BoardPlayerSizeGet(s32 idx)
{
    PlayerState *player;

    player = BoardPlayerGet(idx);
    if (!player) {
        return;
    }
    return player->size;
}

/**
 * @brief Finds another player on the same team.
 *
 * @param arg0 Player index.
 *
 * @return The matching player index, or 4 if none is found.
 */
s32 BoardPlayerSameTeamFind(s32 arg0)
{
    s32 other_player;
    s32 team2;
    s32 team;

    for (other_player = 0; other_player < 4; other_player++) {
        if (other_player == arg0)
            continue;
        team2 = GWPlayer[other_player].team;
        team = GWPlayer[arg0].team;
        if (team == team2)
            break;
    }
    return other_player;
}

/**
 * @brief Finds another player on a different team.
 *
 * @param arg0 Player index.
 *
 * @return The matching player index, or 4 if none is found.
 */
s32 BoardPlayerTeamFind(s32 arg0)
{
    s32 other_player;
    s32 team2;
    s32 team;

    for (other_player = 0; other_player < 4; other_player++) {
        if (other_player == arg0)
            continue;
        team2 = GWPlayer[other_player].team;
        team = GWPlayer[arg0].team;
        if (team != team2)
            break;
    }
    return other_player;
}

/**
 * @brief Calculates a player rank from stars and coins.
 *
 * @param player Player index.
 *
 * @return A zero-based rank, where zero is first place.
 */
s32 BoardPlayerRankCalc(s32 player)
{
    s32 coins;
    s32 rank;
    s32 i;
    s32 score[4];

    for (i = 0; i < 4; i++) {
        coins = GWCoinsGet(i);
        score[i] = coins | (GWStarsGet(i) << 0xA);
    }
    for (rank = 0, i = 0; i < 4; i++) {
        if ((i != player) && (score[player] < score[i])) {
            rank++;
        }
    }
    return rank;
}

/**
 * @brief Sets a player pre-turn callback.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Callback invoked before the player rolls.
 */
void BoardPlayerPreTurnHookSet(s32 arg0, s32 (*arg1)())
{
    preTurnHook[arg0] = arg1;
}

/**
 * @brief Sets a player post-turn callback.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Callback invoked after the turn.
 */
void BoardPlayerPostTurnHookSet(s32 arg0, s32 (*arg1)())
{
    postTurnHook[arg0] = arg1;
}

/**
 * @brief Runs the main board-turn sequence for a player.
 *
 * @param arg0 Player index.
 */
void BoardPlayerTurnExec(s32 arg0)
{
    BoardPauseDisableSet(1);
    BoardComUseItemSet(arg0, -1);
    GWSystem.bowser_event = 0xF;
    _ClearFlag(FLAG_ID_MAKE(1, 22));
    _ClearFlag(FLAG_ID_MAKE(1, 14));
    BoardCameraMoveSet(1);
    if (_CheckFlag(FLAG_ID_MAKE(1, 6)) == 0) {
        BoardCameraViewSet(2);
        omVibrate((s16)arg0, 0xC, 4, 2);
        BoardItemPrevSet(-1);
        BoardYourTurnExec(arg0);
        megaDoubleDiceF = 0;
        SetPlayerSizeAuto(arg0);
        if (preTurnHook[arg0] != 0U) {
            if (preTurnHook[arg0]() != 0) {
                preTurnHook[arg0] = 0;
            }
        }
        if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
            BoardTutorialHookExec(5, 0);
        }
        BoardPlayerTurnRollExec(arg0);
    }
    else {
        BoardCameraViewSet(1);
        BoardCameraMotionWait();
    }
    BoardPlayerTurnMoveExec(arg0);
}

/**
 * @brief Runs the dice and item-selection portion of a turn.
 *
 * @param arg0 Player index.
 */
void BoardPlayerTurnRollExec(s32 arg0)
{
    s32 unused_roll_value;
    s32 roll_result;

    GWPlayer[arg0].roll = 0;
    GWPlayer[arg0].num_dice = 1;
    BoardPauseDisableSet(0);
    do {
        roll_result = BoardRollExec(arg0);
        switch (roll_result) {
            case -2:
                BoardPauseDisableSet(1);
                BoardRollKill();
                BoardCameraTargetPlayerSet(arg0);
                BoardCameraMotionWait();
                BoardItemPrevSet(BoardItemUseExec(arg0));
                if (BoardItemPrevGet() == 0xC) {
                    _SetFlag(0x10016);
                    BoardSpaceLandExec(arg0, GWPlayer[arg0].space_curr);
                    BoardCameraMoveSet(1);
                    BoardCameraViewSet(2);
                    _ClearFlag(0x10016);
                }
                BoardPauseDisableSet(0);
                break;
            case -3:
                BoardRollKill();
                BoardViewMapExec(arg0);
                break;
            case -4:
                BoardRollKill();
                BoardViewOverheadExec(arg0);
                break;
        }
    } while (roll_result <= 0);
}

/**
 * @brief Moves a player across spaces and resolves the landing sequence.
 *
 * @details Repeatedly selects links, moves between spaces, processes pass events and traps, then restores temporary item and size effects before running the landing event.
 *
 * @param arg0 Player index.
 */
void BoardPlayerTurnMoveExec(s32 arg0)
{
    s16 next_space;
    u32 space_flags;
    s32 space_type;
    s32 move_others;
    s32 resume_move;
    s32 unused_space_value_0;
    s32 unused_space_value_1;

    BoardPauseDisableSet(1);
    resume_move = 0;
    if (_CheckFlag(FLAG_ID_MAKE(1, 6)) == 0) {
        BoardCameraMoveSet(1);
        move_others = 1;
        BoardCameraViewSet(1);
        BoardCameraMotionWait();
        next_space = -1;
        GWPlayer[arg0].space_next = -1;
        if (GWPlayer[arg0].bowser_suit != 0) {
            BoardCameraTargetModelSet(suitMdl);
            BoardCameraOffsetSet(0.0f, 100.0f, 0.0f);
        }
    }
    else {
        move_others = 0;
        resume_move = 1;
        _ClearFlag(FLAG_ID_MAKE(1, 6));
        _ClearFlag(FLAG_ID_MAKE(1, 7));
        next_space = GWPlayer[arg0].space_next;
        goto block_14;
    }
    do {
        BoardPauseDisableSet(1);
        GWPlayer[arg0].space_prev = GWPlayer[arg0].space_curr;
        if (_CheckFlag(FLAG_ID_MAKE(2, 1)) == 0) {
            if (ExecJunction(arg0, &next_space) != 0) {
                break;
            }
        }
        else if (DoDebugMove(arg0, &next_space) != 0) {
            break;
        }

        GWPlayer[arg0].space_next = next_space;
        StopJunctionPlayer(0);
        if (MegaPlayerPassFunc(arg0, next_space) == 0) {
            BoardPauseDisableSet(0);
            BoardPlayerMoveTo(arg0, next_space);
            BoardPauseDisableSet(1);
        }
        GWPlayer[arg0].space_prev = GWPlayer[arg0].space_curr;
        GWPlayer[arg0].space_curr = next_space;
        if (move_others != 0) {
            BoardPlayerMoveAwayStartCurr(GWPlayer[arg0].space_prev, 0);
            move_others = 0;
        }
        DoSparkSpace(arg0, 0);
        if (BoardSpaceWalkEventExec() != 0) {
            continue;
        }
        if (BoardSpaceWalkExec(arg0, next_space) != 0) {
            continue;
        }
    block_14:
        space_type = BoardSpaceTypeGet(0, next_space);
        space_flags = BoardSpaceFlagGet(0, next_space);
        if ((space_type == 0 || space_type == 8 || space_type == 10) && ((space_flags & 0x20000000) == 0) || resume_move != 0) {
            resume_move = 0;
            continue;
        }
        HuAudFXPlay(0x301);

        if (--GWPlayer[arg0].roll == 0)
            break;
    } while (1);
    BoardPauseDisableSet(1);
    BoardRollUpdateSet(arg0);
    BoardPlayerIdleSet(arg0);
    if (GWPlayer[arg0].bowser_suit != 0) {
        BoardCameraTargetPlayerSet(arg0);
    }
    if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
        BoardTutorialHookExec(7, 0);
    }
    if (BoardItemPrevGet() != -1) {
        BoardItemPlayerRestore(arg0, BoardItemPrevGet());
        while (BoardItemDoneCheck() == 0) {
            HuPrcVSleep();
        }
        BoardItemPrevSet(-1);
    }
    BoardPlayerSizeRestore(arg0);
    megaDoubleDiceF = 0;
    if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
        BoardTutorialHookExec(8, 0);
    }
    BoardSpaceBlockExec(arg0, next_space);
    BoardSpaceLandExec(arg0, next_space);
    _SetFlag(FLAG_ID_MAKE(1, 14));
    if (GWSystem.bowser_event != 1) {
        BoardCameraViewSet(2);
        BoardCameraMotionWait();
    }
    else {
        GWSystem.bowser_event = 0xF;
    }
    BoardPlayerZoomRestore(arg0);
    return;
}

/**
 * @brief Runs a player post-turn callback.
 *
 * @param arg0 Player index.
 */
void BoardPlayerPostTurnHookExec(s32 arg0)
{
    if (postTurnHook[arg0] != 0U) {
        if (postTurnHook[arg0]() != 0) {
            postTurnHook[arg0] = 0;
        }
    }
}

/**
 * @brief Restores a player to normal size.
 *
 * @param arg0 Player index.
 */
void BoardPlayerSizeRestore(s32 arg0)
{
    if (BoardPlayerSizeGet(arg0) != 0) {
        if (BoardPlayerSizeGet(arg0) == 1) {
            HuAudFXPlay(0x313);
        }
        else {
            HuAudFXPlay(0x311);
        }
        BoardPlayerResizeAnimExec(arg0, 0);
        BoardPlayerSizeSet(arg0, 0);

        while (BoardItemDoneCheck() == 0) {
            HuPrcVSleep();
        }
    }
}

/**
 * @brief Turns a player to yaw zero and restores their idle motion.
 *
 * @param arg0 Player index.
 */
void BoardPlayerZoomRestore(s32 arg0)
{
    PlayerState *player_state;
    s32 ordered_player;
    s32 corner_index;
    s32 players_checked;
    s32 space;
    Vec corner_pos;
    Vec player_pos;

    player_state = BoardPlayerGet(arg0);
    BoardPlayerPosGet(arg0, &player_pos);
    space = GWPlayer[arg0].space_curr;
    ordered_player = GWSystem.player_curr;
    if (ordered_player == -1) {
        ordered_player = 0;
    }
    for (corner_index = players_checked = 0; players_checked < 4; players_checked++) {
        if (ordered_player == arg0)
            break;
        if (space == GWPlayer[ordered_player].space_curr) {
            corner_index++;
        }
        ordered_player = (ordered_player + 1) & 3;
    }
    BoardSpaceCornerPosGet(space, corner_index, &corner_pos);
    BoardPlayerMotBlendSet(arg0, 0, 0xF);

    while (BoardPlayerMotBlendCheck(arg0) == 0) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(arg0);
}

/**
 * @brief Adds flags to the junction exclusion mask.
 *
 * @param arg0 Flags to add.
 */
void BoardJunctionMaskSet(s32 arg0)
{
    junctionMask |= arg0;
}

/**
 * @brief Removes flags from the junction exclusion mask.
 *
 * @param arg0 Flags to remove.
 */
void BoardJunctionMaskReset(s32 arg0)
{
    junctionMask &= ~arg0;
}

/**
 * @brief Clears the junction exclusion mask.
 */
void BoardJunctionMaskZero(void)
{
    junctionMask = 0;
}

/**
 * @brief Gets the junction exclusion mask.
 *
 * @return The current exclusion mask.
 */
static inline u32 BoardJunctionMaskGet(void)
{
    return junctionMask;
}

/**
 * @brief Counts usable outgoing links from a board space.
 *
 * @param playerIdx Player index used for size checks.
 *
 * @param boardIdx Board space index.
 *
 * @return The number of usable links, or zero when the space is not a junction.
 */
static inline s32 GetLinkCount(s32 playerIdx, s32 boardIdx)
{
    s32 i;
    s32 linkCount;
    BoardSpace *boardSpaceLink;
    BoardSpace *boardSpaceFlag;
    PlayerState *player;
    s32 color;
    s32 temp;

    linkCount = 0;
    boardSpaceLink = BoardSpaceGet(0, boardIdx);

    if (boardSpaceLink->link_cnt <= 1) {
        return 0;
    }
    linkCount = boardSpaceLink->link_cnt;
    for (i = 0; i < boardSpaceLink->link_cnt; i++) {
        boardSpaceFlag = BoardSpaceGet(0, boardSpaceLink->link[i]);
        if (((boardSpaceFlag->flag & 0x20000000) != 0 && (BoardPlayerSizeGet(playerIdx) != 1)) || ((boardSpaceFlag->flag & 0x02000000) != 0)
            || ((boardSpaceFlag->flag & 0x04000000) != 0) || ((boardSpaceFlag->flag & BoardJunctionMaskGet()) != 0)) {

            linkCount--;
        }
    }
    return linkCount;
}

/**
 * @brief Creates direction arrows for a junction when needed.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Junction space index.
 *
 * @param arg8 Initial direction, or -1 to leave the default selection.
 */
static void InitJunction(s32 arg0, s32 arg1, f32 arg8)
{
    Vec junction_pos;
    Vec arrow_pos;
    Vec linked_pos;
    Vec link_direction;
    f32 message_size[2];
    s32 link_count;
    s16 arrow_rotation;
    s16 player_size;
    BoardSpace *junction_space;
    BoardSpace *linked_space;
    f32 window_x;
    f32 window_y;
    f32 arrow_z_offset;
    OMOBJ *object;
    s32 player;
    JunctionWork *work;
    s32 arrow_count;
    s32 link_index;
    f32 angle;

    if (!junctionObj) {
        for (player = 0; player < 4; player++) {
            junctionArrowRot[player] = -1;
        }

        link_count = GetLinkCount(arg0, arg1);
        if (link_count > 1) {
            object = omAddObjEx(boardObjMan, 0x100, 0, 0, -1, UpdateJunctionGfx);
            junctionObj = object;
            work = omObjGetWork(object, JunctionWork);
            work->kill = 0;
            work->remove_requested = 0;
            work->player = arg0;
            work->pulse_angle = 0;
            work->selected_arrow = 0;
            work->selection_delay = 0;
            object->scale.x = object->scale.y = object->scale.z = 3.0f;
            HuWinMesMaxSizeGet(1, message_size, MAKE_MESSID(0x9, 0x03));
            window_x = -10000.0f;
            window_y = 304.0f;
            work->window = HuWinCreate(window_x, window_y, message_size[0], message_size[1], 0);
            HuWinBGTPLvlSet(work->window, 0.0f);
            HuWinMesSpeedSet(work->window, 0);
            HuWinMesSet(work->window, MAKE_MESSID(9, 3));
            if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
                HuWinDispOff(work->window);
            }
            junction_space = BoardSpaceGet(0, arg1);
            BoardSpacePosGet(0, arg1, &junction_pos);
            player_size = BoardPlayerSizeGet(arg0);

            for (arrow_count = 0, link_index = arrow_count; link_index < junction_space->link_cnt; link_index++) {
                if (junction_space->link[link_index] != 0) {
                    linked_space = BoardSpaceGet(0, junction_space->link[link_index]);
                    if (((((linked_space->flag & 0x20000000) == 0) != 0) || (player_size == 1)) && ((linked_space->flag & 0x02000000) == 0)
                        && ((linked_space->flag & 0x04000000) == 0)) {
                        if ((linked_space->flag & BoardJunctionMaskGet()) == 0) {
                            BoardSpacePosGet(0, junction_space->link[link_index], &linked_pos);
                            VECSubtract(&linked_pos, &junction_pos, &link_direction);
                            VECNormalize(&link_direction, &link_direction);

                            angle = atan2d(link_direction.x, link_direction.z);
                            if (angle < 0.0f) {
                                angle += 360.0f;
                            }
                            if (angle > 360.0f) {
                                angle -= 360.0f;
                            }

                            OSf32tos16(&angle, &arrow_rotation);
                            arrow_rotation = ((arrow_rotation + 0x16) / 45) * 0x2D;
                            ;
                            junctionArrowRot[arrow_count] = arrow_rotation;
                            arrow_z_offset = 0.0f;
                            if (arrow_rotation == 0) {
                                arrow_z_offset = 100.0f;
                            }
                            else if ((arrow_rotation == 0xB4) && ((player_size == 2) || (GWPlayer[arg0].bowser_suit != 0))) {
                                arrow_z_offset = -100.0f;
                            }
                            work->arrow_models[arrow_count] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0), NULL, 1);
                            BoardModelRotSet(work->arrow_models[arrow_count], 0.0f, angle, 0.0f);
                            BoardModelLayerSet(work->arrow_models[arrow_count], 6);
                            arrow_pos.x = junction_pos.x + 100.0 * sind(angle);
                            arrow_pos.y = 300.0f + junction_pos.y;
                            arrow_pos.z = (arrow_z_offset + (junction_pos.z + (100.0 * cosd(angle))));
                            BoardModelPosSetV(work->arrow_models[arrow_count], &arrow_pos);
                            BoardModelScaleSet(work->arrow_models[arrow_count], object->scale.x, object->scale.y, object->scale.z);
                            arrow_count++;
                        }
                    }
                }
            }
            work->arrow_count = arrow_count;
            if (-1.0f != arg8) {
                RestoreJunction(arg8, 0);
            }
        }
    }
    return;
}

/**
 * @brief Updates and destroys the junction direction display.
 *
 * @param arg0 Object being updated.
 */
static void UpdateJunctionGfx(OMOBJ *arg0)
{
    s32 arrow_index;
    JunctionWork *work;

    work = omObjGetWork(arg0, JunctionWork);
    if ((work->kill != 0) || (BoardIsKill() != 0)) {
        for (arrow_index = 0; arrow_index < work->arrow_count; arrow_index++) {
            BoardModelKill(work->arrow_models[arrow_index]);
        }
        HuWinKill(work->window);
        junctionObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    if (work->selection_delay != 0) {
        work->selection_delay--;
    }
    else if (work->remove_requested != 0) {
        work->kill = 1;
    }
    work->pulse_angle += 8;
    if (work->pulse_angle > 0xB4) {
        work->pulse_angle -= 0xB4;
    }
    OSs16tof32(&work->pulse_angle, &arg0->scale.x);
    arg0->scale.x = 3.0 + sind(arg0->scale.x);
    arg0->scale.y = arg0->scale.x;
    arg0->scale.z = arg0->scale.x;
    BoardModelScaleSet(work->arrow_models[work->selected_arrow], arg0->scale.x, arg0->scale.y, arg0->scale.z);
}

/**
 * @brief Schedules the junction display for removal.
 *
 * @param arg0 Nonzero to play the confirmation sound.
 */
static void StopJunctionPlayer(s32 arg0)
{
    JunctionWork *work;

    if (junctionObj != 0) {
        work = omObjGetWork(junctionObj, JunctionWork);
        work->remove_requested = 1;
        work->selection_delay = 0;
        if (arg0 != 0) {
            HuAudFXPlay(2);
        }
        HuPrcSleep(0xA);
    }
}

/**
 * @brief Selects the junction arrow nearest a direction.
 *
 * @param arg8 Direction angle in degrees.
 *
 * @param arg0 Nonzero to play the selection sound.
 */
static void RestoreJunction(f32 arg8, s32 arg0)
{
    Vec arrow_rot;
    f32 arrow_directions[4];
    s16 arrow_index;
    s16 selected_arrow;
    JunctionWork *work;

    if (junctionObj != 0) {
        work = omObjGetWork(junctionObj, JunctionWork);
        if (work->selection_delay == 0) {
            arg8 = 90.0f * (((90.0f + arg8) - 1.0f) / 90.0f);

            for (arrow_index = 0; arrow_index < work->arrow_count; arrow_index++) {
                BoardModelRotGet(work->arrow_models[arrow_index], &arrow_rot);
                arrow_directions[arrow_index] = 90.0f * (((90.0f + arrow_rot.y) - 1.0f) / 90.0f);
            }

            for (; arrow_index < 4; arrow_index++) {
                arrow_directions[arrow_index] = -999.0f;
            }

            selected_arrow = GetDefaultDirection(arg8, arrow_directions, work->arrow_count);
            if ((selected_arrow != -1) && (work->selected_arrow != selected_arrow)) {
                work->selected_arrow = selected_arrow;
                if (arg0 != 0) {
                    HuAudFXPlay(0);
                }
                work->selection_delay = 0x14;
            }
        }
    }
}

/**
 * @brief Finds the candidate direction closest to an angle.
 *
 * @param arg0 Target angle in degrees.
 *
 * @param arg1 Candidate angle array.
 *
 * @param arg2 Number of candidates.
 *
 * @return The index of the closest candidate direction.
 */
static s32 GetDefaultDirection(f32 arg0, f32 *arg1, s32 arg2)
{
    f32 angle_delta;
    f32 smallest_delta;
    f32 candidate_angle;
    f32 target_angle;
    s32 closest_index;
    s32 direction_index;

    target_angle = BoardDAngleCalc(arg0);
    smallest_delta = 999.0f;

    for (direction_index = 0, closest_index = direction_index; direction_index < arg2; direction_index++) {
        candidate_angle = BoardDAngleCalc(arg1[direction_index]);
        angle_delta = BOARD_FABS(BoardDAngleCalc(candidate_angle - target_angle));

        if (angle_delta < smallest_delta) {
            smallest_delta = angle_delta;
            closest_index = direction_index;
        }
    }
    return closest_index;
}

/**
 * @brief Gets the tracked player motion.
 *
 * @param player Player index.
 *
 * @return The tracked motion index.
 */
static inline s32 BoardPlayerMotGet(s32 player)
{
    PlayerState *player_ptr = BoardPlayerGet(player);
    return playerMot[player_ptr->player_idx];
}

/**
 * @brief Selects the next space using debug movement controls.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Output next-space index.
 *
 * @return Zero when a link is selected, or 1 when debug movement is cancelled.
 */
static s32 DoDebugMove(s32 arg0, s16 *arg1)
{
    s16 link_angles[10];
    s16 link_spaces[10];
    Vec stick_input;
    Vec link_delta;
    Vec player_pos;
    Vec camera_direction;
    BoardSpace *linked_space;
    s32 space_count;
    s32 current_space;
    BoardCameraData *camera;
    f32 input_angle;
    s32 link_index;
    s16 link_angle;
    PlayerState *player_state;
    BoardSpace *current_space_data;
    s32 link_count;
    s32 camera_relative_angle;
    s32 port;
    s32 result;
    JunctionWork *junction_work;

    result = -1;

    memset(link_angles, 0, 0x14);
    memset(link_spaces, 0, 0x14);
    while (1) {
    loop_1:
        player_state = BoardPlayerGet(arg0);
        current_space_data = BoardSpaceGet(0, player_state->space_curr);
        space_count = BoardSpaceCountGet(0);
        BoardPlayerPosGet(arg0, &player_pos);
        port = GWPlayer[arg0].port;

        for (link_count = 0, link_index = link_count; link_index < current_space_data->link_cnt; link_index++) {
            if ((BoardSpaceFlagGet(0, current_space_data->link[link_index]) & 0x06000000) == 0) {
                if ((BoardSpaceFlagGet(0, current_space_data->link[link_index]) & 0x20000000) == 0 || (BoardPlayerSizeGet(arg0) == 1)) {
                    link_spaces[link_count++] = current_space_data->link[link_index];
                }
            }
        }
        link_count += BoardSpaceLinkTargetListGet(0, player_state->space_curr, &link_spaces[link_count]);

        for (link_index = 0; link_index < link_count; link_index++) {
            linked_space = BoardSpaceGet(0, link_spaces[link_index]);
            if (linked_space == 0) {
                link_spaces[link_index] = 0;
            }
            else {
                VECSubtract(&linked_space->pos, &player_pos, &link_delta);
                input_angle = 90.0 - atan2d(link_delta.z, link_delta.x);
                OSf32tos16(&input_angle, &link_angle);
                if (link_angle < 0) {
                    link_angle += 0x168;
                }
                if (link_angle > 0x168) {
                    link_angle -= 0x168;
                }

                link_angles[link_index] = link_angle = link_angle = (((link_angle + 0x16) / 45) * 0x2D);
            }
        }
        current_space = GWPlayer[arg0].space_curr;
        InitJunction(arg0, player_state->space_curr, -1.0f);
        do {
        loop_21:
            if (current_space != GWPlayer[arg0].space_curr) {
                current_space = GWPlayer[arg0].space_curr;
                goto loop_1;
            }
            stick_input.x = HuPadStkX[port];
            stick_input.y = 0.0f;
            stick_input.z = HuPadStkY[port];
            if (HuPadBtnDown[port] == 0x400) {
                StopJunctionPlayer(0);
                result = 1;
                goto end;
            }
            else {
                if (HuPadBtnDown[port] == 0x10) {
                    BoardRollDispSet(0);
                    BoardPlayerIdleSet(arg0);
                    StopJunctionPlayer(0);
                    BoardViewMapExec(arg0);
                    InitJunction(arg0, player_state->space_curr, -1.0f);
                    BoardRollDispSet(1);
                    goto loop_21;
                }
                if ((0.0f != stick_input.x) || (0.0f != stick_input.z)) {
                    input_angle = 90.0 + atan2d(stick_input.z, stick_input.x);
                    if (input_angle < 0.0f) {
                        input_angle += 360.0f;
                    }
                    if (input_angle > 360.0f) {
                        input_angle -= 360.0f;
                    }
                    RestoreJunction(input_angle, 1);
                    camera = &boardCamera;
                    VECSubtract(&camera->target, &camera->pos, &camera_direction);
                    input_angle += 90.0 - atan2d(-camera_direction.z, -camera_direction.x);
                    if (input_angle < 0.0f) {
                        input_angle += 360.0f;
                    }
                    if (input_angle > 360.0f) {
                        input_angle -= 360.0f;
                    }
                    camera_relative_angle = input_angle;

                    for (link_index = 0; link_index < link_count; link_index++) {
                        if ((link_angles[link_index] + 0x2D > camera_relative_angle) && (link_angles[link_index] - 0x2D < camera_relative_angle)) {
                            *arg1 = link_spaces[link_index];
                            result = 0;
                            goto end;
                        }
                        else if ((camera_relative_angle <= 0x168) && (camera_relative_angle >= 0x13B) && ((s32)((&link_angles[0])[link_index] + 0x195) > camera_relative_angle)
                            && ((s32)((&link_angles[0])[link_index] + 0x13B) < camera_relative_angle)) {
                            *arg1 = link_spaces[link_index];
                            result = 0;
                            goto end;
                        }
                    }
                }
                else {
                    if (BoardPlayerMotGet(arg0) != 1) {
                        BoardPlayerIdleSet(arg0);
                    }
                    if (GWPlayer[arg0].bowser_suit != 0) {
                        BoardBowserSuitMotionSetWait();
                        BoardBowserSuitPlayerModelKill();
                    }
                }
                HuPrcVSleep();
                goto loop_21;
            }
        } while (1);
    }
end:
    return result;
}

/**
 * @brief Gets the rotation of the selected junction arrow.
 *
 * @return The selected arrow rotation, or 0 if no display exists.
 */
static inline f32 JunctionArrowRotGetCurr(void)
{
    JunctionWork *juncObj;
    f32 ret;

    if (!junctionObj) {
        return 0.0f;
    }
    else {
        juncObj = omObjGetWork(junctionObj, JunctionWork);
        OSs16tof32(&(junctionArrowRot[juncObj->selected_arrow]), &ret);
        return ret;
    }
}

/**
 * @brief Checks whether an angle is near an available junction arrow.
 *
 * @param value Angle in degrees.
 *
 * @return 1 if an available arrow is within the accepted range, otherwise 0.
 */
static inline s32 CheckArrowRot(float value)
{
    s32 arrow_index;
    JunctionWork *work;
    s16 rotation;
    if (!junctionObj) {
        return 0;
    }
    else {
        work = omObjGetWork(junctionObj, JunctionWork);
        OSf32tos16(&value, &rotation);
        if (rotation > 0x15E) {
            rotation = 0;
        }
        for (arrow_index = 0; arrow_index < work->arrow_count; arrow_index++) {
            if (abs(junctionArrowRot[arrow_index] - rotation) <= 0x1E) {
                return 1;
            }
        }
        return 0;
    }
}

/**
 * @brief Selects an outgoing link at a normal board junction.
 *
 * @details Builds the usable-link list, accepts tutorial, CPU, or controller
 * direction input, and temporarily opens the map or overhead view on request.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Output next-space index.
 *
 * @return Zero after a direction is confirmed. The function initializes its
 * result to -1, but its loop does not currently expose a cancellation exit.
 */
static s32 ExecJunction(s32 arg0, s16 *arg1)
{
    f32 link_angles[10];
    s16 link_spaces[10];
    Vec stick_input;
    Vec link_delta;
    Vec player_pos;
    s32 result;
    BoardSpace *linked_space;
    s32 space_count;
    PlayerState *player_state;
    s8 com_input_frames;
    s32 link_index;
    s32 port;
    BoardSpace *current_space;
    s32 link_count;
    u32 buttons;
    f32 input_angle;
    f32 last_angle;

    result = -1;
    memset(link_angles, 0, 40);
    memset(link_spaces, 0, 20);
    player_state = BoardPlayerGet(arg0);
    current_space = BoardSpaceGet(0, player_state->space_curr);
    space_count = BoardSpaceCountGet(0);
    BoardPlayerPosGet(arg0, &player_pos);
    port = GWPlayer[arg0].port;

    for (link_index = link_count = 0; link_index < current_space->link_cnt; link_index++) {
        if ((BoardSpaceFlagGet(0, current_space->link[link_index]) & 0x06000000) == 0) {
            if ((BoardSpaceFlagGet(0, current_space->link[link_index]) & 0x20000000) == 0 || (BoardPlayerSizeGet(arg0) == 1)) {
                link_spaces[link_count++] = current_space->link[link_index];
            }
        }
    }
    if (link_count == 1) {
        *arg1 = link_spaces[0];
        return 0;
    }
    if (link_count <= 0) {
        *arg1 = current_space->link[0];
        return 0;
    }

    for (link_index = 0; link_index < link_count; link_index++) {
        linked_space = BoardSpaceGet(0, link_spaces[link_index]);
        if (!linked_space) {
            link_spaces[link_index] = 0;
        }
        else {
            VECSubtract(&linked_space->pos, &player_pos, &link_delta);
            input_angle = 90.0 - atan2d(link_delta.z, link_delta.x);
            if (input_angle < 0.0f) {
                input_angle += 360.0f;
            }
            if (input_angle > 360.0f) {
                input_angle -= 360.0f;
            }
            link_angles[link_index] = 45.0f * ((22.0f + input_angle) / 45.0f);
        }
    }
    last_angle = -1.0f;
    com_input_frames = 0;
    InitJunction(arg0, player_state->space_curr, last_angle);
    BoardPlayerIdleSet(arg0);
    HuPrcSleep(10);
    while (1) {

        if ((_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) && (boardTutorialDirInputTime != -1)) {
            if (boardTutorialDirInputTime > 0) {
                stick_input.x = boardTutorialDirInputX;
                stick_input.z = boardTutorialDirInputY;
            }
            else {
                buttons = 0x100;
            }
            boardTutorialDirInputTime -= 1;
        }
        else if (GWPlayer[arg0].com != 0) {
            if (com_input_frames == 0) {
                BoardComJunctionInputGet(arg0, &stick_input, link_count, &link_angles[0]);
                buttons = 0;
            }
            else if (com_input_frames > 6U) {
                buttons = 0x100;
            }
            com_input_frames++;
        }
        else {
            if (abs(HuPadStkX[port]) > 10) {
                stick_input.x = HuPadStkX[port];
            }
            else {
                stick_input.x = 0.0f;
            }
            if (abs(HuPadStkY[port]) > 10) {
                stick_input.z = HuPadStkY[port];
            }
            else {
                stick_input.z = 0.0f;
            }
            buttons = HuPadBtnDown[port];
        }
        if (buttons == 0x100) {
            input_angle = JunctionArrowRotGetCurr();
            link_index = GetDefaultDirection(input_angle, link_angles, link_count);
            *arg1 = link_spaces[link_index];
            StopJunctionPlayer(1);
            result = 0;
            break;
        }
        else if (buttons == 0x20 || buttons == 0x10) {
            HuAudFXPlay(1);
            BoardRollDispSet(0);
            BoardPlayerIdleSet(arg0);
            StopJunctionPlayer(0);
            if (buttons == 0x10) {
                BoardViewMapExec(arg0);
            }
            else if (buttons == 0x20) {
                BoardViewOverheadExec(arg0);
            }
            InitJunction(arg0, player_state->space_curr, last_angle);
            BoardRollDispSet(1);
        }
        else {
            if ((0.0f != stick_input.x) || (0.0f != stick_input.z)) {
                input_angle = 90.0 + atan2d(stick_input.z, stick_input.x);
                if (input_angle < 0.0f) {
                    input_angle += 360.0f;
                }
                if (input_angle > 360.0f) {
                    input_angle -= 360.0f;
                }
                if (CheckArrowRot(input_angle) != 0) {
                    last_angle = input_angle;
                    RestoreJunction(input_angle, 1);
                }

                for (link_index = 0; link_index < link_count; link_index++) {
                    if (((45.0f + link_angles[link_index]) > input_angle) && ((link_angles[link_index] - 45.0f) < input_angle)) {
                        *arg1 = (&link_spaces[0])[link_index];
                    }
                    if (input_angle <= 360.0f) {
                        if ((input_angle >= 315.0f) && ((45.0f + (360.0f + link_angles[link_index])) > input_angle) && (((360.0f + link_angles[link_index]) - 45.0f) < input_angle)) {
                            *arg1 = link_spaces[link_index];
                        }
                    }
                }
            }
            else {
                if (BoardPlayerMotGet(arg0) != 1) {
                    BoardPlayerIdleSet(arg0);
                }
                if (GWPlayer[arg0].bowser_suit != 0) {
                    BoardBowserSuitMotionSetWait();
                    BoardBowserSuitPlayerModelKill();
                }
            }
            HuPrcVSleep();
        }
    }
    return result;
}

/**
 * @brief Moves a player to a board space and waits for completion.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Destination space index.
 */
void BoardPlayerMoveTo(s32 arg0, s32 arg1)
{
    Vec start_pos;
    Vec target_pos;
    PlayerState *player;

    BoardModelPosGet(BoardPlayerModelGet(arg0), &start_pos);
    BoardSpacePosGet(0, arg1, &target_pos);
    BoardPlayerPosLerpStart(arg0, &start_pos, &target_pos, 0x19);
    while (GWPlayer[arg0].moving != 0) {
        HuPrcVSleep();
    }
}

/**
 * @brief Moves a player between two board spaces and waits.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Starting space index.
 *
 * @param arg2 Destination space index.
 */
void BoardPlayerMoveBetween(s32 arg0, s32 arg1, s32 arg2)
{
    Vec start_pos;
    Vec target_pos;
    s32 unused_start_space;
    s32 unused_target_space;

    BoardSpacePosGet(0, arg1, &start_pos);
    BoardSpacePosGet(0, arg2, &target_pos);
    BoardPlayerPosLerpStart(arg0, &start_pos, &target_pos, 0x19);
    while (GWPlayer[arg0].moving != 0) {
        HuPrcVSleep();
    }
}

/**
 * @brief Starts moving a player to a board space.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Destination space index.
 */
void BoardPlayerMoveToAsync(s32 arg0, s32 arg1)
{
    Vec start_pos;
    Vec target_pos;
    s32 unused_space;

    BoardModelPosGet(BoardPlayerModelGet(arg0), &start_pos);
    BoardSpacePosGet(0, arg1, &target_pos);
    BoardPlayerPosLerpStart(arg0, &start_pos, &target_pos, 0x19);
}

/**
 * @brief Starts an object-driven linear player movement.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Starting position.
 *
 * @param arg2 Destination position.
 *
 * @param arg3 Movement duration in frames.
 */
void BoardPlayerPosLerpStart(s32 arg0, Vec *arg1, Vec *arg2, s16 arg3)
{
    Vec move_direction;
    f32 distance;
    f32 motion_speed;
    f32 facing_angle;
    f32 duration;
    f32 normalized_angle;
    OMOBJ *move_object;
    s32 motion_result;
    PlayerState *player_state;

    if ((arg1->x != arg2->x) || (arg1->y != arg2->y) || (arg1->z != arg2->z)) {
        distance = BoardVecDistXZCalc(arg2, arg1);
        if (arg3 != 0) {
            if (GWPlayer[arg0].bowser_suit != 0) {
                arg3 *= 2;
            }

            OSs16tof32(&arg3, &duration);
            motion_speed = distance / duration;
        }
        else {
            motion_speed = distance;
            duration = 1.0f;
        }
        player_state = BoardPlayerGet(arg0);

        if (playerMot[player_state->player_idx] != 3) {
            motion_result = BoardModelMotionShiftSet(BoardPlayerModelGet(arg0), 3, 0.0f, 4.0f, HU3D_MOTATTR_LOOP);
            if (motion_result == 0) {
                playerMot[player_state->player_idx] = 3;
            }
        }
        if (GWPlayer[arg0].bowser_suit != 0) {
            BoardBowserSuitMotionSetWalk();
            BoardModelMotionSpeedSet(suitMdl, 1.5f);
        }
        VECSubtract(arg2, arg1, &move_direction);
        VECNormalize(&move_direction, &move_direction);
        facing_angle = 90.0 - atan2d(move_direction.z, move_direction.x);
        normalized_angle = facing_angle;
        if (normalized_angle < 0.0f) {
            normalized_angle += 360.0f;
        }
        if (normalized_angle > 360.0f) {
            normalized_angle -= 360.0f;
        }
        BoardModelRotYSet(BoardPlayerModelGet(arg0), normalized_angle);
        if (GWPlayer[arg0].bowser_suit != 0) {
            BoardModelRotYSet(suitMdl, normalized_angle);
        }
        move_object = omAddObjEx(boardObjMan, 0x100, 0U, 0U, -1, &PlayerPosLerpFunc);
        move_object->trans.x = arg1->x;
        move_object->trans.y = arg1->y;
        move_object->trans.z = arg1->z;
        move_object->rot.x = arg2->x;
        move_object->rot.y = arg2->y;
        move_object->rot.z = arg2->z;
        move_object->scale.x = (arg2->x - arg1->x) / duration;
        move_object->scale.y = (arg2->y - arg1->y) / duration;
        move_object->scale.z = (arg2->z - arg1->z) / duration;
        BoardModelPosSetV(BoardPlayerModelGet(arg0), arg1);
        if (GWPlayer[arg0].bowser_suit != 0) {
            BoardModelPosSetV(suitMdl, arg1);
        }
        // TODO: Make work struct for this
        move_object->work[0] = arg0;
        *(f32 *)(&move_object->work[1]) = motion_speed;
        move_object->work[2] = arg3;
        GWPlayer[arg0].moving = 1;
    }
}

/**
 * @brief Updates an active linear player movement.
 *
 * @param arg0 Object being updated.
 */
static void PlayerPosLerpFunc(OMOBJ *arg0)
{
    f32 target_x;
    f32 target_y;
    f32 target_z;
    f32 current_x;
    f32 current_y;
    f32 current_z;
    s32 player;

    player = arg0->work[0];
    arg0->trans.x += arg0->scale.x;
    arg0->trans.y += arg0->scale.y;
    arg0->trans.z += arg0->scale.z;
    if (arg0->work[2] != 0U) {
        arg0->work[2] -= 1;
    }
    else {
        GWPlayer[player].moving = 0;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        target_z = arg0->rot.z;
        target_y = arg0->rot.y;
        target_x = arg0->rot.x;
        BoardPlayerPosSet(player, target_x, target_y, target_z);
        return;
    }
    current_z = arg0->trans.z;
    current_y = arg0->trans.y;
    current_x = arg0->trans.x;
    BoardPlayerPosSet(player, current_x, current_y, current_z);
}

typedef struct dice_jump_work {
    struct {
        u8 kill : 1;
        u8 player : 2;
        u8 vibration_played : 1;
        u8 reserved_bits : 4;
    };
    s8 reserved_01;
    s16 reserved_02;
    s32 hit_window;
    f32 jump_frame;
    f32 base_y;
} DiceJumpWork;

typedef struct motion_blend_work {
    struct {
        u8 kill : 1;
        u8 reserved_bits : 7;
    };
    s8 player;
    s16 frames_remaining;
    s16 motion;
    s16 reserved_06;
    union {
        f32 target_yaw;
        struct {
            s16 target_yaw_high;
            s16 target_yaw_low;
        };
    };
    f32 yaw_step;
} MotionBlendWork;

/**
 * @brief Starts the player dice-hit jump animation.
 *
 * @param arg0 Player index.
 */
void BoardPlayerDiceJumpStart(s32 arg0)
{
    Vec player_pos;
    PlayerState *player_state;
    DiceJumpWork *work;
    s32 motion_result;

    if (GWPlayer[arg0].bowser_suit != 0) {
        BoardBowserSuitMotionSetJump();
    }
    player_state = BoardPlayerGet(arg0);
    if (playerMot[player_state->player_idx] != 11) {
        motion_result = BoardModelMotionStart(BoardPlayerModelGet(arg0), 11, 0);
        if (motion_result == 0) {
            playerMot[player_state->player_idx] = 0xB;
        }
    }
    diceJumpObj[arg0] = omAddObjEx(boardObjMan, 0x100, 0, 0, -1, DiceJumpFunc);
    BoardModelPosGet(BoardPlayerModelGet(arg0), &player_pos);
    work = omObjGetWork(diceJumpObj[arg0], DiceJumpWork);
    work->kill = 0;
    work->player = arg0;
    work->hit_window = 0;
    work->vibration_played = 0;
    work->jump_frame = 0.0f;
    work->base_y = player_pos.y;
    GWPlayer[arg0].jump = 1;
}

/**
 * @brief Checks whether a dice jump is in its active hit window.
 *
 * @param arg0 Player index.
 *
 * @return 1 during the active dice-hit window, otherwise 0.
 */
s32 BoardPlayerDiceJumpCheck(s32 arg0)
{
    if (!diceJumpObj[arg0]) {
        return 0;
    }
    if (omObjGetWork(diceJumpObj[arg0], DiceJumpWork)->hit_window == 0) {
        return 0;
    }
    return 1;
}

/**
 * @brief Updates and cleans up a player dice-hit jump.
 *
 * @param arg0 Object being updated.
 */
static void DiceJumpFunc(OMOBJ *arg0)
{
    Vec player_pos;
    f32 motion_time;
    PlayerState *unused_player_state;
    DiceJumpWork *work;
    s16 model_id;
    s32 unused_motion_value;
    s32 unused_state_value;

    work = omObjGetWork(arg0, DiceJumpWork);
    if ((work->kill != 0) || (BoardIsKill() != 0)) {
        GWPlayer[work->player].jump = 0;
        BoardPlayerIdleSet(work->player);
        diceJumpObj[work->player] = 0;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }

    if (BoardPlayerMotionEndCheck(work->player)) {
        work->kill = 1;
        return;
    }
    model_id = BoardModelIDGet(BoardPlayerModelGet(work->player));
    if (Hu3DMotionShiftIDGet(model_id) == -1) {
        motion_time = BoardPlayerMotionTimeGet(work->player);
        if (BoardPlayerSizeGet(work->player) == 1) {
            BoardPlayerPosGet(work->player, &player_pos);
            player_pos.y += 10.0f + (-0.020416668f * work->jump_frame * work->jump_frame);
            work->jump_frame += 1.0f;
            if (player_pos.y < work->base_y) {
                player_pos.y = work->base_y;
            }
            BoardPlayerPosSetV(work->player, &player_pos);
        }
        if (work->hit_window != 0) {
            if (motion_time >= 35.0f) {
                work->hit_window = 0;
                return;
            }
        }
        if (work->hit_window == 0) {
            if (motion_time >= 30.0f) {
                work->hit_window = 1;
                if (work->vibration_played == 0) {
                    omVibrate(work->player, 0xC, 4, 2);
                    work->vibration_played = 1;
                }
                return;
            }
        }
    }
}

/**
 * @brief Returns a player to the looping idle motion.
 *
 * @param arg0 Player index.
 */
void BoardPlayerIdleSet(s32 arg0)
{
    if (GWPlayer[arg0].bowser_suit) {
        BoardBowserSuitMotionSetWait();
        BoardBowserSuitPlayerModelKill();
    }
    BoardPlayerMotionShiftSet(arg0, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
}

static void ExecMotBlend(OMOBJ *arg0);

/**
 * @brief Starts a timed player turn toward a yaw angle.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Target yaw angle.
 *
 * @param arg2 Turn duration in frames.
 */
void BoardPlayerMotBlendSet(s32 arg0, s16 arg1, s16 arg2)
{
    s16 angle_delta;
    f32 turn_angle;
    f32 turn_rate;
    f32 motion_start_time;
    f32 motion_speed;
    f32 current_angle;
    f32 blend_time;
    f32 unused_angle_value;
    OMOBJ *blend_object;
    s32 turn_motion;
    s32 motion_data;
    s16 wait_frame;
    s16 character;
    MotionBlendWork *work;

    motion_data = 0;
    if (motDoneF[arg0] != 0) {
        blend_object = motDoneF[arg0];
        work = omObjGetWork(blend_object, MotionBlendWork);
        work->kill = 1;

        for (wait_frame = 0; wait_frame < 180U; wait_frame++) {
            if (BoardPlayerMotBlendCheck(arg0) != 0)
                break;
            HuPrcVSleep();
        }
    }
    if (arg2 > 0) {
        character = GWPlayer[arg0].character;
        blend_object = omAddObjEx(boardObjMan, 0x100, 0, 0, -1, ExecMotBlend);
        work = omObjGetWork(blend_object, MotionBlendWork);
        work->kill = 0;
        work->frames_remaining = arg2;
        work->player = arg0;
        work->motion = 0;
        OSs16tof32(&arg1, &work->target_yaw);
        work->target_yaw = BoardDAngleCalc(work->target_yaw);
        current_angle = BoardDAngleCalc(BoardPlayerRotYGet(arg0));
        turn_angle = BoardDAngleCalc(work->target_yaw - current_angle);
        OSf32tos16(&turn_angle, &angle_delta);
        if (angle_delta > 0) {
            motion_start_time = 59.0f;
        }
        else {
            motion_start_time = 0.0f;
        }

        if (abs(angle_delta) == 0) {
            work->kill = 1;
            return;
        }
        if (abs(angle_delta) <= 90) {
            motion_data = boardMotTbl[character];
        }
        else if (abs(angle_delta) <= 180) {
            motion_data = boardMotRevTbl[character];
        }
        else {
            work->kill = 1;
            return;
        }
        work->motion = BoardPlayerMotionCreate(arg0, motion_data);
        OSs16tof32(&arg2, &motion_speed);
        turn_rate = motion_speed / 59.0f;
        if (turn_rate != 0.0f) {
            motion_speed = 1.0f / turn_rate;
        }
        else {
            motion_speed = 1.0f;
        }
        if (GWPlayer[arg0].bowser_suit != 0) {
            BoardBowserSuitMotionSetWalk();
            BoardModelMotionSpeedSet(BoardBowserSuitModelGet(), motion_speed / 2);
        }
        else if (blend_time = 8.0f * turn_rate, abs(angle_delta) > 45) {
            turn_motion = work->motion;
            BoardPlayerMotionShiftSet(arg0, turn_motion, motion_start_time, blend_time, HU3D_MOTATTR_NONE);
            BoardPlayerMotionSpeedSet(arg0, motion_speed);
        }
        OSs16tof32(&arg2, &motion_speed);
        work->yaw_step = turn_angle / motion_speed;
        motDoneF[arg0] = blend_object;
    }
}

/**
 * @brief Updates and completes a timed player turn.
 *
 * @param arg0 Object being updated.
 */
static void ExecMotBlend(OMOBJ *arg0)
{
    f32 unused_angle;
    f32 rotation_y;
    MotionBlendWork *work;

    work = omObjGetWork(arg0, MotionBlendWork);
    if ((work->kill != 0) || (BoardIsKill() != 0)) {
        if (work->motion > 0) {
            BoardPlayerMotionKill(work->player, work->motion);
        }
        BoardPlayerRotYSet(work->player, work->target_yaw);
        motDoneF[work->player] = 0;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    if (work->frames_remaining-- <= 0) {
        work->kill = 1;
        BoardPlayerRotYSet(work->player, work->target_yaw);
        if (GWPlayer[work->player].bowser_suit != 0) {
            BoardBowserSuitMotionSetWait();
            BoardBowserSuitPlayerModelKill();
        }
        else {
            BoardPlayerMotionStart(work->player, 1, 0x40000001);
        }
        return;
    }
    rotation_y = BoardPlayerRotYGet(work->player);
    OSs16tof32(&work->target_yaw_high, &unused_angle);
    rotation_y += work->yaw_step;
    BoardPlayerRotYSet(work->player, rotation_y);
}

/**
 * @brief Checks whether a timed player turn is complete.
 *
 * @param arg0 Player index.
 *
 * @return 1 when no turn object remains, otherwise 0.
 */
s32 BoardPlayerMotBlendCheck(s32 arg0)
{
    if (motDoneF[arg0] != 0) {
        return 0;
    }
    return 1;
}

typedef struct roll_work {
    struct {
        u8 kill : 1;
        u8 visible : 1;
        u8 reserved_bits : 6;
    };
    s8 reveal_angle;
    s8 player;
    s8 reveal_complete;
    s16 sprite_group;
    s16 sprites[5];
} RollWork;

typedef struct move_away_work {
    struct {
        u8 kill : 1;
        u8 reserved_bits : 7;
    };
} MoveAwayWork;

typedef struct bowser_suit_work {
    struct {
        u8 kill : 1;
        u8 reserved_bits : 7;
    };
    s8 player;
    s8 footstep_played;
} BowserSuitWork;

/**
 * @brief Creates the two-digit remaining-roll display for a player.
 *
 * @param arg0 Player whose `roll` value supplies the displayed digits.
 *
 * @param arg1 Nonzero to start the display's internal reveal phase; zero to
 * create it with that phase already complete.
 */
void BoardRollCreate(s32 arg0, s32 arg1)
{
    OMOBJ *roll_object;
    s32 digit;
    RollWork *work;

    roll_object = omAddObjEx(boardObjMan, 0x105, 0, 0, -1, &UpdateRoll);
    work = omObjGetWork(roll_object, RollWork);
    work->kill = 0;
    work->visible = 1;
    work->reveal_angle = 0;
    work->player = arg0;
    if (arg1 != 0) {
        work->reveal_complete = 0;
    }
    else {
        work->reveal_complete = 1;
    }
    work->sprite_group = HuSprGrpCreate(2);

    for (digit = 0; digit < 2; digit++) {
        BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x2B), 0x4B0, 0, &work->sprites[digit]);
        HuSprGrpMemberSet(work->sprite_group, digit, work->sprites[digit]);
        HuSprPosSet(work->sprite_group, digit, HU_DISP_CENTERX, HU_DISP_CENTERY);
        HuSprAttrSet(work->sprite_group, digit, 4);
    }
    rollObj = roll_object;
}

/**
 * @brief Marks the global remaining-roll display for removal.
 *
 * @param arg0 Unused player argument retained by the public interface.
 */
void BoardRollUpdateSet(s32 arg0)
{
    RollWork *work;

    if (rollObj != 0) {
        work = omObjGetWork(rollObj, RollWork);
        work->kill = 1;
    }
}

/**
 * @brief Shows or hides the remaining-roll display.
 *
 * @param arg0 Nonzero to show the display, zero to hide it.
 */
void BoardRollDispSet(s32 arg0)
{
    s32 digit;
    RollWork *work;

    if (rollObj != 0) {
        work = omObjGetWork(rollObj, RollWork);

        for (digit = 0; digit < 2; digit++) {
            if (arg0 != 0) {
                work->visible = 1;
                if ((digit == 1) && (GWPlayer[work->player].roll / 10 == 0)) {
                    HuSprAttrSet(work->sprite_group, digit, HUSPR_ATTR_DISPOFF);
                }
                else {
                    HuSprAttrReset(work->sprite_group, digit, HUSPR_ATTR_DISPOFF);
                }
            }
            else {
                work->visible = 0;
                HuSprAttrSet(work->sprite_group, digit, HUSPR_ATTR_DISPOFF);
            }
        }
        UpdateRollSprite(rollObj);
    }
}

/**
 * @brief Updates digits in the remaining-roll display.
 *
 * @param arg0 Object being updated.
 */
static void UpdateRollSprite(OMOBJ *arg0)
{
    Vec digit_pos;
    s32 digits[2];
    f32 digit_x[2] = { HU_DISP_CENTERX + 32, HU_DISP_CENTERX - 32 };
    s32 digit;
    s32 roll;
    RollWork *work;

    work = omObjGetWork(arg0, RollWork);
    roll = GWPlayer[work->player].roll;
    if (roll != 0) {
        digits[0] = roll % 10;
        digits[1] = roll / 10;
    }
    else {
        work->kill = 1;
    }

    for (digit = 0; digit < 2; digit++) {
        if (((digit == 1) && (digits[1] == 0)) || (work->visible == 0)) {
            HuSprAttrSet(work->sprite_group, digit, HUSPR_ATTR_DISPOFF);
        }
        else {
            if ((digits[1] == 0) && (digit == 0)) {
                digit_pos.x = HU_DISP_CENTERX;
            }
            else {
                digit_pos.x = digit_x[digit];
            }
            digit_pos.y = (HU_DISP_HEIGHTF * 176) / 480;
            HuSprAttrReset(work->sprite_group, digit, HUSPR_ATTR_DISPOFF);
            HuSprBankSet(work->sprite_group, digit, digits[digit]);
            HuSprPosSet(work->sprite_group, digit, digit_pos.x, digit_pos.y);
        }
    }
}

/**
 * @brief Updates and destroys the remaining-roll display object.
 *
 * @param arg0 Object being updated.
 */
static void UpdateRoll(OMOBJ *arg0)
{
    f32 display_angle;
    RollWork *work;

    work = omObjGetWork(rollObj, RollWork);
    if ((work->kill != 0) || (BoardIsKill() != 0)) {
        if (work->sprite_group != -1) {
            HuSprGrpKill(work->sprite_group);
            work->sprite_group = -1;
        }
        rollObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    if (work->reveal_complete == 0) {
        work->reveal_angle += 4;
        if (work->reveal_angle >= 0x5A) {
            work->reveal_complete = 1;
        }

        OSs8tof32(&work->reveal_angle, &display_angle);
        display_angle *= 2.0f;
        if (display_angle >= 90.0f) {
            display_angle -= 90.0f;
        }
    }
    UpdateRollSprite(arg0);
}

/**
 * @brief Waits for a player to press any requested button.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Button mask to wait for.
 */
void BoardPlayerBtnDownWait(s32 arg0, u32 arg1)
{
    s32 unused_button_state;
    s32 unused_port_state;
    s32 port;

    port = GWPlayer[arg0].port;
    while (1) {
        if ((HuPadBtnDown[port] & arg1) != 0)
            return;
        HuPrcVSleep();
    }
}

/**
 * @brief Sets the queued automatic size change for a player.
 *
 * @param player Player index.
 *
 * @param value Queued size state.
 */
void BoardPlayerAutoSizeSet(s32 player, s32 value)
{
    PlayerState *player_state;

    player_state = BoardPlayerGet(player);
    if ((player_state->auto_size != 0) || (value == 0)) {
        BoardStatusHammerKill(player);
    }
    player_state->auto_size = value;
    if (value != 0) {
        BoardStatusHammerCreate(player);
    }
}

/**
 * @brief Gets the queued automatic size change for a player.
 *
 * @param arg0 Player index.
 *
 * @return The queued size state, or no defined value if player state is unavailable.
 */
s32 BoardPlayerAutoSizeGet(s32 arg0)
{
    PlayerState *player;

    player = BoardPlayerGet(arg0);
    if (!player) {
        return;
    }
    return player->auto_size;
}

/**
 * @brief Checks whether players have finished moving to space corners.
 *
 * @return 1 when no displacement object remains, otherwise 0.
 */
u32 BoardPlayerMoveAwayIsDone(void)
{
    if (moveAwayObj) {
        return 0;
    }
    return 1;
}

/**
 * @brief Moves other occupants toward corners of a board space.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Occupied space index.
 *
 * @param arg2 Nonzero to reposition immediately.
 */
void BoardPlayerMoveAwayStart(s32 arg0, s32 arg1, s32 arg2)
{
    MoveAwayWork *work;
    Vec space_rot;
    Vec corner_pos;
    Vec player_pos;
    s32 player;
    s32 ordered_player;
    s32 player_index;
    s32 move_count;
    s32 players_checked;

    moveAwayPlayer[0] = moveAwayPlayer[1] = moveAwayPlayer[2] = moveAwayPlayer[3] = -1;
    ordered_player = arg0 + 1;

    players_checked = 0;
    // hack
    for (move_count = move_count = players_checked; players_checked < 4; players_checked++, ordered_player++) {
        ordered_player &= 3;

        for (player_index = 0; player_index < 4; player_index++) {
            if ((player_index == ordered_player) && (arg1 == GWPlayer[player_index].space_curr)) {
                moveAwayPlayer[move_count++] = player_index;
            }
        }
    }
    BoardSpaceRotGet(0, arg1, &space_rot);

    for (player_index = 0; player_index < move_count; player_index++) {
        BoardSpaceCornerPosGet(arg1, player_index, &corner_pos);
        player = moveAwayPlayer[player_index];
        if (arg2 != 0) {
            BoardPlayerPosSetV(player, &corner_pos);
            moveAwayPlayer[player_index] = -1;
        }
        else {
            BoardPlayerPosGet(player, &player_pos);
            BoardPlayerPosLerpStart(player, &player_pos, &corner_pos, 0x19);
        }
    }
    if (arg2 == 0) {
        moveAwayObj = omAddObjEx(boardObjMan, 0x100, 0U, 0U, -1, &MoveAwayObjFunc);
        work = omObjGetWork(moveAwayObj, MoveAwayWork);
        work->kill = 0;
    }
}

/**
 * @brief Moves occupants of a space away from the current player.
 *
 * @param arg0 Occupied space index.
 *
 * @param arg1 Nonzero to reposition immediately.
 */
void BoardPlayerMoveAwayStartCurr(s32 arg0, s32 arg1)
{
    BoardPlayerMoveAwayStart(GWSystem.player_curr, arg0, arg1);
}

/**
 * @brief Waits for displaced players to finish moving.
 *
 * @param arg0 Object being updated.
 */
static void MoveAwayObjFunc(OMOBJ *arg0)
{
    s32 move_index;
    MoveAwayWork *work;

    work = omObjGetWork(arg0, MoveAwayWork);
    if ((work->kill != 0) || (BoardIsKill() != 0)) {
        moveAwayObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }

    for (move_index = 0; move_index < 4; move_index++) {
        if (moveAwayPlayer[move_index] != -1) {
            if (GWPlayer[moveAwayPlayer[move_index]].moving != 0)
                return;
            BoardPlayerMotBlendSet(moveAwayPlayer[move_index], 0, 15);
            moveAwayPlayer[move_index] = -1;
        }
    }
    work->kill = 1;
}

/**
 * @brief Creates and enables the Bowser suit for a player.
 *
 * @param arg0 Player index.
 */
void BoardBowserSuitInit(s32 arg0)
{
    OMOBJ *suit_object;
    s32 character;
    s32 unused_player;
    s32 temp;
    BowserSuitWork *work;

    for (unused_player = 0; unused_player < 4; unused_player++) { }
    character = GWPlayer[arg0].character;
    temp = BoardDataDirReadAsync(0x40000);
    BoardDataAsyncWait(temp);
    suitMdl = BoardModelCreate(0x40018, NULL, 0);
    BoardModelScaleSet(suitMdl, 1.2f, 1.2f, 1.2f);
    suitPlayerMdl = BoardModelCreate(bowserSuitCharMdlTbl[character], NULL, 0);
    GWPlayer[arg0].bowser_suit = 1;
    suit_object = omAddObjEx(boardObjMan, 0x100, 0U, 0U, -1, &UpdateBowserSuit);
    bowserSuitObj = suit_object;
    work = omObjGetWork(suit_object, BowserSuitWork);
    work->kill = 0;
    work->player = arg0;
    work->footstep_played = 0;
    BoardPlayerVoiceEnableSet(arg0, 3, 0);
    BoardPlayerVoiceEnableSet(arg0, 4, 0);
}

/**
 * @brief Removes the Bowser suit from a player.
 *
 * @param arg0 Player index.
 */
void BoardBowserSuitKill(s32 arg0)
{
    BowserSuitWork *temp;

    if (bowserSuitObj != 0) {
        omObjGetWork(bowserSuitObj, BowserSuitWork)->kill = 1;
    }
    GWPlayer[arg0].bowser_suit = 0;
    BoardModelVoiceEnableSet(BoardPlayerModelGet(arg0), 3, 1);
    BoardModelVoiceEnableSet(BoardPlayerModelGet(arg0), 4, 1);
    if (suitMdl != -1) {
        BoardModelKill(suitMdl);
        suitMdl = -1;
    }
    if (suitPlayerMdl != -1) {
        BoardModelKill(suitPlayerMdl);
        suitPlayerMdl = -1;
    }
    HuDataDirClose(0x40000);
}

/**
 * @brief Gets the Bowser suit model handle.
 *
 * @return The suit model handle, or -1 when absent.
 */
s16 BoardBowserSuitModelGet(void)
{
    return suitMdl;
}

/**
 * @brief Gets the character model handle used by the Bowser suit.
 *
 * @return The inner character model handle, or -1 when absent.
 */
s16 BoardBowserSuitPlayerModelGet(void)
{
    return suitPlayerMdl;
}

/**
 * @brief Creates motions used by the Bowser suit.
 */
void BoardBowserSuitMotionCreate(void)
{
    s32 motion;

    for (motion = 0; motion < 5; motion++) {
        bowserSuitMot[motion] = (s16)BoardModelMotionCreate(suitMdl, bowserSuitMotTbl[motion]);
    }
    suitCurrMot = -1;
}

/**
 * @brief Sets the Bowser suit idle motion.
 */
void BoardBowserSuitMotionSetWait(void)
{
    if (suitCurrMot != 0) {
        BoardModelMotionShiftSet(suitMdl, bowserSuitMot[0], 0.0f, 4.0f, HU3D_MOTATTR_LOOP);
        suitCurrMot = 0;
    }
}

/**
 * @brief Sets the Bowser suit walk motion.
 */
void BoardBowserSuitMotionSetWalk(void)
{
    if (suitCurrMot != 1) {
        BoardModelMotionStart(suitMdl, bowserSuitMot[1], 0x40000001);
        suitCurrMot = 1;
    }
}

/**
 * @brief Sets the Bowser suit victory motion.
 */
void BoardBowserSuitMotionSetWin(void)
{
    BoardModelMotionShiftSet(suitMdl, bowserSuitMot[3], 0.0f, 4.0f, HU3D_MOTATTR_NONE);
    suitCurrMot = 3;
}

/**
 * @brief Sets the Bowser suit jump motion.
 */
void BoardBowserSuitMotionSetJump(void)
{
    BoardModelMotionShiftSet(suitMdl, bowserSuitMot[2], 0.0f, 4.0f, HU3D_MOTATTR_NONE);
    suitCurrMot = 2;
}

/**
 * @brief Freezes the character model used inside the Bowser suit.
 */
void BoardBowserSuitPlayerModelKill(void)
{
    if (suitPlayerMdl != -1) {
        BoardModelMotionStart(suitPlayerMdl, 1, 0);
        BoardModelMotionSpeedSet(suitPlayerMdl, 0.0f);
    }
}

/**
 * @brief Updates Bowser suit footstep sounds and object lifetime.
 *
 * @param arg0 Object being updated.
 */
static void UpdateBowserSuit(OMOBJ *arg0)
{
    s16 motion_time;
    BowserSuitWork *work;

    work = omObjGetWork(arg0, BowserSuitWork);
    if ((work->kill != 0) || (BoardIsKill() != 0)) {
        bowserSuitObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    if ((suitMdl != -1) && (suitCurrMot == 1)) {
        motion_time = BoardModelMotionTimeGet(suitMdl);
        if ((work->footstep_played == 0) && (((motion_time < 15.0f) && (motion_time >= 13.0f)) || ((motion_time < 40.0f) && (motion_time >= 38.0f)))) {
            HuAudFXPlay(0x328);
            work->footstep_played = 1;
        }
        if ((work->footstep_played != 0) && ((motion_time >= 20.0f) || (motion_time >= 49.0f))) {
            work->footstep_played = 0;
        }
    }
}

/**
 * @brief Marks the current space as containing a player Sparky trap.
 *
 * @param arg0 Player index.
 */
void BoardPlayerSparkSet(s32 arg0)
{
    s16 currSpace = GWPlayer[arg0].space_curr;

    GWPlayer[arg0].spark = 1;
    GWPlayer[arg0].space_shock = currSpace;
}

/**
 * @brief Executes a Sparky trap when a player reaches its space.
 *
 * @details If the current space has a Sparky trap, this function plays the shock sequence, removes up to ten coins from each occupant, clears the trap, and restores turn presentation.
 *
 * @param player Player entering the space.
 *
 * @param pause_cam Nonzero to restore the paused camera mode afterward.
 *
 * @return 1 if a Sparky trap was executed, otherwise 0.
 */
static s32 DoSparkSpace(s32 player, s32 pause_cam)
{
    s32 i;
    s16 spark_model;
    s32 coin;
    s32 previous_item;
    Vec pos, rot;
    Vec coin_pos;
    s16 shock_motions[4] = { -1, -1, -1, -1 };
    s32 spark_found;
    s32 affected_count;
    s32 affected_total;
    s32 total_coins;
    s32 suited_player;
    s8 affected_players[4] = { -1, -1, -1, -1 };
    s8 coin_displays[4] = { -1, -1, -1, -1 };
    s16 space;
    s16 spark_owner;
    s16 current_player;
    s16 effect_model;
    s16 alpha;
    current_player = GWSystem.player_curr;
    spark_found = 0;
    space = GWPlayer[player].space_curr;

    BoardSpacePosGet(0, space, &pos);
    BoardSpaceRotGet(0, space, &rot);
    for (i = 0; i < 4; i++) {
        if (GWPlayer[i].space_shock == space) {
            spark_owner = i;
            spark_found = 1;
        }
    }
    if (!spark_found) {
        return 0;
    }
    BoardRollDispSet(0);
    if (pause_cam) {
        BoardCameraMoveSet(0);
    }
    else {
        BoardCameraMoveSet(1);
    }
    BoardCameraMotionWait();
    BoardCameraTargetPlayerSet(player);
    BoardPlayerIdleSet(player);
    spark_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, VERSION_JP ? 0x73 : 0x74), NULL, 0);
    BoardModelMotionStart(spark_model, 0, 0x40000002);
    BoardModelMotionTimeSet(spark_model, 30.0f);
    BoardModelAttrSet(spark_model, 0x40000002);
    BoardModelRotSet(spark_model, rot.x - 90.0f, rot.y, rot.z);
    BoardModelScaleSet(spark_model, 2.0f, 2.0f, 2.0f);
    BoardModelVisibilitySet(spark_model, 0);
    BoardModelLayerSet(spark_model, 1);
    effect_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, VERSION_JP ? 0x66 : 0x67), NULL, 0);
    BoardModelLayerSet(effect_model, 1);
    BoardModelMotionStart(effect_model, 0, 0);
    HuAudFXPlay(796);
    HuPrcSleep(2);
    BoardModelAttrReset(spark_model, 0x40000002);
    BoardModelVisibilitySet(spark_model, 1);
    BoardModelPosSet(spark_model, pos.x, 8.0f + pos.y, pos.z);
    BoardModelPosSetV(effect_model, &pos);
    suited_player = -1;
    for (affected_count = i = 0; i < 4; i++) {
        if (GWPlayer[i].space_curr == space) {
            affected_players[affected_count++] = i;
            shock_motions[i] = BoardPlayerMotionCreate(i, sparkMotTbl[GWPlayer[i].character]);
            BoardPlayerMotionShiftSet(i, shock_motions[i], 0, 8, HU3D_MOTATTR_NONE);
            HuAudFXPlay(boardSparkSfxTblAlt[GWPlayer[i].character]);
            if (GWPlayer[i].bowser_suit) {
                BoardModelMotionShiftSet(suitMdl, bowserSuitMot[4], 0.0f, 4.0f, HU3D_MOTATTR_NONE);
                suited_player = i;
            }
        }
    }
    affected_total = affected_count;
    HuPrcSleep(60);
    while (!BoardModelMotionEndCheck(effect_model)) {
        HuPrcVSleep();
    }
    alpha = 255;
    for (i = 0; i < 45; i++) {
        alpha -= 5;
        if (alpha < 0) {
            alpha = 0;
        }
        BoardModelAlphaSet(spark_model, alpha);
        BoardModelAlphaSet(effect_model, alpha);
        HuPrcVSleep();
    }
    previous_item = BoardItemPrevGet();
    if (previous_item == 0 || previous_item == 1 || previous_item == 2 || previous_item == 3 || previous_item == 10) {
        BoardCameraTargetPlayerSet(player);
        BoardPlayerIdleSet(player);
        HuPrcSleep(12);
        BoardItemPlayerRestore(player, previous_item);
        while (!BoardItemDoneCheck()) {
            HuPrcVSleep();
        }
        BoardItemPrevSet(-1);
    }
    if (megaDoubleDiceF) {
        BoardPlayerSizeRestore(player);
        megaDoubleDiceF = 0;
    }
    if (suited_player != -1) {
        HuAudFXPlay(boardSparkSfxTbl[GWPlayer[suited_player].character]);
    }
    for (i = 0; i < affected_total; i++) {
        coin = BoardPlayerCoinsGet(affected_players[i]);
        if (coin > 10) {
            coin = 10;
        }
        if (coin > 0) {

            BoardPlayerPosGet(affected_players[i], &coin_pos);
            coin_pos.y += 250.0f;
            coin_displays[i] = BoardCoinChgCreate(&coin_pos, -coin);
        }
    }
    for (total_coins = i = 0; i < affected_total; i++) {
        total_coins += BoardPlayerCoinsGet(affected_players[i]);
    }
    if (total_coins) {
        HuAudFXPlay(840);
        for (coin = 0; coin < 10; coin++) {
            for (i = 0; i < affected_total; i++) {
                BoardPlayerCoinsAdd(affected_players[i], -1);
            }
            HuAudFXPlay(14);
            HuPrcSleep(6);
        }
        HuAudFXPlay(15);
        HuPrcSleep(90);
    }
    BoardModelKill(spark_model);
    BoardModelKill(effect_model);
    for (i = 0; i < 4; i++) {
        BoardPlayerIdleSet(affected_players[i]);
    }
    HuPrcSleep(10);
    GWSystem.player_curr = -1;
    RemoveSparkSpace(spark_owner);
    GWSystem.player_curr = current_player;
    for (i = 0; i < 4; i++) {
        if (shock_motions[i] != -1) {
            BoardPlayerMotionKill(i, shock_motions[i]);
        }
    }
    if (pause_cam) {
        BoardCameraMoveSet(0);
        BoardCameraMotionWait();
    }

    BoardCameraTargetPlayerSet(current_player);
    BoardCameraMotionWait();
    BoardCameraMoveSet(1);
    BoardRollDispSet(1);
    return 1;
}

/**
 * @brief Notifies the owner and removes their Sparky trap.
 *
 * @param player Player who owns the trap.
 */
static void RemoveSparkSpace(s32 player)
{
    if (GWPlayer[player].spark) {
        BoardWinCreate(0, MAKE_MESSID(18, 28), -1);
        BoardWinWait();
        BoardWinKill();
        GWPlayer[player].spark = 0;
        GWPlayer[player].space_shock = 0;
    }
}

/**
 * @brief Applies a queued automatic player size change.
 *
 * @param player Player index.
 */
static void SetPlayerSizeAuto(s32 player)
{
    s32 auto_size = BoardPlayerAutoSizeGet(player);
    if (auto_size != 0) {
        BoardStatusHammerKill(player);
        BoardPlayerAutoSizeSet(player, 0);
        HuPrcSleep(30);
        omVibrate(player, 90, 6, 6);
        if (auto_size == 1) {
            HuAudFXPlay(788);
            HuAudFXPlay(787);
        }
        else if (auto_size == 2) {
            HuAudFXPlay(786);
            HuAudFXPlay(785);
        }
        BoardPlayerResizeAnimExec(player, auto_size);
        BoardPlayerSizeSet(player, auto_size);
        HuPrcSleep(30);
        megaDoubleDiceF = 1;
    }
}

/**
 * @brief Checks whether an automatic size effect was applied this turn.
 *
 * @return Nonzero if an automatic size effect was applied, otherwise zero.
 */
s32 BoardMegaDoubleDiceCheck(void)
{
    return megaDoubleDiceF;
}

/**
 * @brief Animates a player model between normal, Mini, and Mega sizes.
 *
 * @param player Player index.
 *
 * @param size Target size state: 0 for normal, 1 for Mini, or 2 for Mega.
 */
void BoardPlayerResizeAnimExec(s32 player, s32 size)
{
    s32 j;
    s32 i;
    s16 modelid;
    s32 frame;
    s16 model;
    float mdl_size;
    float frame_data[9];
    float steps[4];
    float player_sizes[] = { 1, 0.3, 2.5 };

    mdl_size = player_sizes[BoardPlayerSizeGet(player)];
    steps[0] = mdl_size;
    steps[1] = mdl_size + ((player_sizes[size] - mdl_size) / 3);
    steps[2] = mdl_size + ((player_sizes[size] - mdl_size) / 1.5f);
    steps[3] = player_sizes[size];
    frame_data[0] = steps[1];
    frame_data[1] = steps[0];
    frame_data[2] = steps[1];
    frame_data[3] = steps[2];
    frame_data[4] = steps[1];
    frame_data[5] = steps[2];
    frame_data[6] = steps[3];
    frame_data[7] = steps[2];
    frame_data[8] = steps[3];
    model = BoardPlayerModelGet(player);
    modelid = BoardModelIDGet(model);
    Hu3DModelAttrSet(modelid, HU3D_MOTATTR_PAUSE);
    frame = 0;
    for (j = 0; j < 9; j++) {
        float size_vel;
        size_vel = (frame_data[frame++] - mdl_size) / 10.0f;
        for (i = 0; i < 10; i++) {
            BoardPlayerScaleSet(player, mdl_size, mdl_size, mdl_size);
            mdl_size += size_vel;
            HuPrcVSleep();
        }
    }
    Hu3DModelAttrReset(modelid, HU3D_MOTATTR_PAUSE);
    BoardPlayerSizeSet(player, size);
}

typedef struct mega_squish_work {
    struct {
        u8 kill : 1;
        u8 no_coinchg : 1;
        u8 played_snd : 1;
        u8 gain_player : 2;
    };
    s8 player;
    s8 state;
    s8 coinchg;
    s8 loss;
    s8 disappear_time;
    s8 disappear_len;
    u16 hide_time;
} MegaSquishWork;

/**
 * @brief Updates a player being squashed by a giant player.
 *
 * @param object Squish controller object.
 */
static void MegaSquishFunc(OMOBJ *object)
{
    MegaSquishWork *work = omObjGetWork(object, MegaSquishWork);
    Vec pos;
    if (work->kill || BoardIsKill()) {
        if (work->coinchg != -1) {
            BoardCoinChgKill(work->coinchg);
        }
        megaSquishObj[work->player] = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    if (work->hide_time != 0) {
        work->hide_time--;
        return;
    }
    switch (work->state) {
        case 1:
            if (!work->played_snd) {
                HuAudFXPlay(798);
                HuAudFXPlay(megaSquishSfxTbl[GWPlayer[work->player].character]);
                work->played_snd = 1;
            }
            BoardPlayerMotionStart(work->player, 10, 0);
            BoardPlayerMotionSpeedSet(work->player, 2.0f);
            work->state = 2;
            break;

        case 2: {
            if (!BoardModelMotionTimeGet(BoardPlayerModelGet(work->player) >= 50.0f)) {
                return;
            }
            BoardPlayerMotionSpeedSet(work->player, 0.0f);
            BoardPlayerPosGet(work->player, &pos);
            pos.y += 10.0f;
            BoardPlayerPosSetV(work->player, &pos);
            work->hide_time = 20;
            work->state = 3;
            work->coinchg = -1;
            work->disappear_time = 0;
        }

        break;

        case 3:
            if (work->coinchg == -1) {
                BoardPlayerPosGet(work->player, &pos);

                pos.y += 250.0f;
                if (!work->no_coinchg) {
                    work->coinchg = BoardCoinChgCreate(&pos, -work->loss);
                }
                else {
                    work->state = 4;
                    work->hide_time = work->disappear_len;
                }
            }
            if (work->disappear_time++ < work->disappear_len) {
                return;
            }
            work->disappear_time = 0;
            if (work->loss <= 0) {
                work->state = 4;
                HuAudFXPlay(15);
            }
            else {
                work->loss--;
                BoardPlayerCoinsAdd(work->player, -1);
                HuAudFXPlay(14);
                BoardPlayerCoinsAdd(work->gain_player, 1);
            }
            break;

        case 4:
            BoardPlayerMotionSpeedSet(work->player, 2.0f);
            BoardPlayerPosGet(work->player, &pos);
            pos.y -= 10.0f;
            BoardPlayerPosSetV(work->player, &pos);
            work->state = 5;
            break;

        case 5:
            if (!BoardPlayerMotionEndCheck(work->player)) {
                return;
            }
            BoardPlayerIdleSet(work->player);
            work->kill = 1;
            break;

        case 0:
            break;

        default:
            break;
    }
}

/**
 * @brief Squashes players occupying a giant player's destination space.
 *
 * @details Only giant players use this path. Occupants of the destination space are squashed and can transfer up to ten coins each to the moving player.
 *
 * @param player Moving player index.
 *
 * @param space Destination space index.
 *
 * @return Zero when no special pass action is needed; the completed-action return value is not explicitly set.
 */
static s32 MegaPlayerPassFunc(s32 player, s32 space)
{
    float jump_frame;
    float previous_y;
    MegaSquishWork *squish_work;
    s32 other_player;
    MegaSquishWork *other_work;
    s32 squish_index;
    s32 squish_count;
    s32 descending;
    OMOBJ *squish_object;
    s32 squished_players[4];
    Vec space_pos;
    Vec player_pos;
    Vec move_step;
    Vec other_pos;
    float jump_data[2];

    if (GWPlayer[player].bowser_suit) {
        return MegaExecJump(player, space);
    }
    if (BoardPlayerSizeGet(player) != 2) {
        return 0;
    }
    squished_players[0] = squished_players[1] = squished_players[2] = -1;
    for (squish_count = other_player = 0; other_player < 4; other_player++) {
        if (other_player != player && GWPlayer[other_player].space_curr == space) {
            squished_players[squish_count++] = other_player;
            BoardPlayerPosGet(other_player, &other_pos);
            squish_object = omAddObjEx(boardObjMan, 256, 0, 0, -1, MegaSquishFunc);
            megaSquishObj[other_player] = squish_object;
            squish_work = omObjGetWork(squish_object, MegaSquishWork);
            squish_work->kill = 0;
            squish_work->player = other_player;
            squish_work->gain_player = player;
            squish_work->state = 0;
            squish_work->hide_time = 0;
            squish_work->loss = 0;
            squish_work->disappear_len = 3;
            squish_work->disappear_time = 0;
            squish_work->played_snd = 0;
            if (BoardPlayerCoinsGet(squish_work->player) <= 0) {
                squish_work->no_coinchg = 1;
            }
            else {
                squish_work->no_coinchg = 0;
            }
            if (BoardPlayerCoinsGet(squish_work->player) >= 10) {
                squish_work->loss = 10;
            }
            else {
                squish_work->loss = BoardPlayerCoinsGet(squish_work->player);
            }
        }
    }
    if (squish_count == 0) {
        return 0;
    }
    if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
        BoardTutorialHookExec(24, 0);
    }
    BoardPlayerPosGet(player, &player_pos);
    BoardSpacePosGet(0, space, &space_pos);
    jump_data[0] = player_pos.y;
    if (space_pos.y > player_pos.y) {
        player_pos.y += 300.0f;
    }
    else {
        player_pos.y += 200.0f;
    }
    VECSubtract(&space_pos, &player_pos, &move_step);
    move_step.x /= 60.0f;
    move_step.z /= 60.0f;
    BoardPlayerRotYSet(player, 90.0 - atan2d(move_step.z, move_step.x));
    jump_data[1] = BoardVecDistXZCalc(&space_pos, &player_pos);
    BoardPlayerMotionShiftSet(player, 4, 0, 8, HU3D_MOTATTR_NONE);
    HuPrcSleep(8);
    descending = 0;
    for (jump_frame = 0; jump_frame < 60.0f; jump_frame++) {
        previous_y = player_pos.y;
        player_pos.x += move_step.x;
        player_pos.z += move_step.z;
        player_pos.y += 10.0f + (0.9f * (-0.020416668f * jump_frame * jump_frame));
        if (previous_y > player_pos.y) {
            descending = 1;
        }
        if (player_pos.y <= 200.0f + space_pos.y && descending) {
            for (squish_index = 0; squish_index < 4; squish_index++) {
                if (megaSquishObj[squish_index]) {
                    other_work = omObjGetWork(megaSquishObj[squish_index], MegaSquishWork);
                    if ((s16)space == GWPlayer[other_work->player].space_curr && other_work->state != 1) {
                        other_work->state = 1;
                    }
                }
            }
        }
        if (player_pos.y <= space_pos.y) {
            player_pos.y = space_pos.y;
            BoardPlayerMotionShiftSet(player, 5, 0, 4, HU3D_MOTATTR_NONE);
        }
        BoardPlayerPosSetV(player, &player_pos);
        HuPrcVSleep();
    }
    BoardCameraQuakeSet(30, 500.0f);
    omVibrate(0, 12, 4, 2);
    omVibrate(1, 12, 4, 2);
    omVibrate(2, 12, 4, 2);
    omVibrate(3, 12, 4, 2);
    BoardPlayerIdleSet(player);
    HuPrcSleep(30);
}

/**
 * @brief Jumps a Bowser-suited player onto and squashes space occupants.
 *
 * @details This Bowser-suit path squashes occupants of the destination space and can transfer up to thirty coins from each affected player.
 *
 * @param player Moving player index.
 *
 * @param space Destination space index.
 *
 * @return Zero when no occupants are affected; the completed-action return value is not explicitly set.
 */
static s32 MegaExecJump(s32 player, s32 space)
{
    float jump_frame;
    float previous_y;
    MegaSquishWork *squish_work;
    s32 other_player;
    MegaSquishWork *other_work;
    s32 squish_index;
    s32 squish_count;
    s32 descending;
    s32 landed;
    OMOBJ *squish_object;
    s32 squished_players[4];
    Vec space_pos;
    Vec player_pos;
    Vec move_step;
    Vec other_pos;
    float jump_data[2];
    squished_players[0] = squished_players[1] = squished_players[2] = -1;
    for (squish_count = other_player = 0; other_player < 4; other_player++) {
        if (other_player != player && GWPlayer[other_player].space_curr == space) {
            squished_players[squish_count++] = other_player;
            BoardPlayerPosGet(other_player, &other_pos);
            squish_object = omAddObjEx(boardObjMan, 256, 0, 0, -1, MegaSquishFunc);
            megaSquishObj[other_player] = squish_object;
            squish_work = omObjGetWork(squish_object, MegaSquishWork);
            squish_work->kill = 0;
            squish_work->player = other_player;
            squish_work->gain_player = player;
            squish_work->state = 0;
            squish_work->hide_time = 0;
            squish_work->loss = 0;
            squish_work->disappear_len = 3;
            squish_work->disappear_time = 0;
            if (BoardPlayerCoinsGet(squish_work->player) <= 0) {
                squish_work->no_coinchg = 1;
            }
            else {
                squish_work->no_coinchg = 0;
            }
            if (BoardPlayerCoinsGet(squish_work->player) >= 30) {
                squish_work->loss = 30;
            }
            else {
                squish_work->loss = BoardPlayerCoinsGet(squish_work->player);
            }
        }
    }
    if (squish_count == 0) {
        return 0;
    }
    BoardPlayerPosGet(player, &player_pos);
    BoardSpacePosGet(0, space, &space_pos);
    jump_data[0] = player_pos.y;
    if (space_pos.y > player_pos.y) {
        player_pos.y += 300.0f;
    }
    else {
        player_pos.y += 200.0f;
    }
    VECSubtract(&space_pos, &player_pos, &move_step);
    move_step.x /= 60.0f;
    move_step.z /= 60.0f;
    BoardPlayerRotYSet(player, 90.0 - atan2d(move_step.z, move_step.x));
    BoardPlayerIdleSet(player);
    HuPrcSleep(10);
    jump_data[1] = BoardVecDistXZCalc(&space_pos, &player_pos);
    BoardModelMotionShiftSet(suitMdl, bowserSuitMot[3], 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    BoardModelMotionSpeedSet(suitMdl, 2.0f);
    HuPrcSleep(8);
    HuAudFXPlay(809);
    landed = 0;
    descending = 0;
    for (jump_frame = 0; jump_frame < 60.0f; jump_frame++) {
        previous_y = player_pos.y;
        player_pos.x += move_step.x;
        player_pos.z += move_step.z;
        player_pos.y += 10.0f + (0.9f * (-0.020416668f * jump_frame * jump_frame));
        if (previous_y > player_pos.y) {
            descending = 1;
        }
        if (player_pos.y <= 200.0f + space_pos.y && descending) {
            for (squish_index = 0; squish_index < 4; squish_index++) {
                if (megaSquishObj[squish_index]) {
                    other_work = omObjGetWork(megaSquishObj[squish_index], MegaSquishWork);
                    if ((s16)space == GWPlayer[other_work->player].space_curr && other_work->state != 1) {
                        other_work->state = 1;
                    }
                }
            }
        }
        if (player_pos.y <= space_pos.y) {
            player_pos.y = space_pos.y;
            if (!landed) {
                BoardPlayerIdleSet(player);
                HuAudFXPlay(810);
                landed = 1;
            }
        }
        BoardPlayerPosSetV(player, &player_pos);
        HuPrcVSleep();
    }
    BoardCameraQuakeSet(30, 500.0f);
    omVibrate(0, 12, 4, 2);
    omVibrate(1, 12, 4, 2);
    omVibrate(2, 12, 4, 2);
    omVibrate(3, 12, 4, 2);
    BoardPlayerIdleSet(player);
    HuPrcSleep(30);
}

/**
 * @brief Blacks out or restores non-eye player materials.
 *
 * @details When enabled, material attributes are checked against character-specific eye bitmap names so only non-eye materials are blackened. Passing zero restores the saved material array.
 *
 * @param arg0 Player index.
 *
 * @param arg1 Nonzero to blacken non-eye materials, zero to restore them.
 */
void BoardPlayerCopyEyeMat(s32 arg0, s32 arg1)
{
    HSFMATERIAL *saved_material;
    s32 is_not_eye_material;
    s32 material_index;
    u32 attribute_index;
    HSFATTRIBUTE *attribute;
    char **eye_material_names;
    HSFDATA *hsfData;
    HSFMATERIAL *material;
    HU3DMODEL *model;
    s16 modelId = BoardModelIDGet(BoardPlayerModelGet(arg0));
    model = &Hu3DData[modelId];
    hsfData = model->hsf;
    material = hsfData->material;
    saved_material = playerMatCopy[arg0];
    if (arg1 != 0) {
        eye_material_names = &eyeMatTbl[GWPlayer[arg0].character][0];
        (void)eye_material_names;
        for (material_index = 0; material_index < hsfData->materialNum; material_index++, material++, saved_material++) {
            is_not_eye_material = 1;

            for (attribute_index = 0; attribute_index < material->attrNum; attribute_index++) {
                attribute = &hsfData->attribute[(s32)material->attr[attribute_index]];
                if ((strcmp(eye_material_names[0], attribute->bitmap->name) == 0) || (strcmp(eye_material_names[1], attribute->bitmap->name) == 0)) {
                    is_not_eye_material = 0;
                }
            }
            if (is_not_eye_material) {
                if (arg1 != 0) {
                    material->color[0] *= 0.0f;
                    material->color[1] *= 0.0f;
                    material->color[2] *= 0.0f;
                }
                else {
                    material->color[0] = saved_material->color[0];
                    material->color[1] = saved_material->color[1];
                    material->color[2] = saved_material->color[2];
                }
            }
        }
    }
    else {
        memcpy(hsfData->material, saved_material, hsfData->materialNum * 0x3C);
    }
    DCStoreRange(hsfData->material, hsfData->materialNum * 0x3C);
}

/**
 * @brief Copies player model materials for later eye-effect restoration.
 *
 * @details Allocates a full material-array copy from the overlay heap. The copy
 * is stored per player and released by `BoardPlayerModelKill`.
 *
 * @param arg0 Player index.
 */
void BoardPlayerCopyMat(s32 arg0)
{
    s16 modelID;
    HU3DMODEL *model;
    void *material_copy;
    HSFDATA *model_data;

    modelID = BoardModelIDGet(BoardPlayerModelGet(arg0));
    model = &Hu3DData[modelID];
    model_data = model->hsf;
    material_copy = HuMemDirectMallocNum(HEAP_HEAP, model_data->materialNum * sizeof(HSFMATERIAL), HU_MEMNUM_OVL);
    memcpy(material_copy, model_data->material, model_data->materialNum * sizeof(HSFMATERIAL));
    playerMatCopy[arg0] = material_copy;
}
