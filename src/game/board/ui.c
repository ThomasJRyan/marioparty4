#include "types.h"
#include "game/board/ui.h"
#include "game/audio.h"
#include "game/board/com.h"
#include "game/board/item.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/tutorial.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"

#include "ext_math.h"
#include <string.h>

typedef struct {
    /* 0x00 */ struct {
        u8 created : 1;
        u8 visible : 1;
        u8 moving : 1;
        u8 force_show : 1;
        u8 gray : 1;
        u8 items_visible : 1;
        u8 hammer_created : 1;
    };
    /* 0x01 */ s8 movement_divisor;
    /* 0x02 */ s16 sprite_group;
    /* 0x04 */ Vec position;
    /* 0x10 */ Vec target_position;
} UnkUiStatusStruct; // Size 0x1C

typedef struct {
    /* 0x00 */ s8 item_id[6];
    /* 0x06 */ s16 item_model[6];
    /* 0x12 */ s16 name_window[6];
    /* 0x1E */ s16 description_window[6];
    /* 0x2A */ char padding[2];
    /* 0x2C */ Vec position[6];
    /* 0x74 */ Vec target_position[6];
} UnkUiWindowStruct; // Size 0xBC

typedef struct {
    struct {
        u8 destroy_requested : 1;
        u8 state : 3;
        u8 player : 2;
        u8 input_locked : 1;
        u8 done : 1;
    };
    s8 target_choice;
    s8 choice;
    u8 input_delay;
    s16 sprite_group;
    s16 window;
} UnkUiWork01;

typedef struct {
    struct {
        u8 destroy_requested : 1;
        u8 state : 3;
        u8 player : 2;
    };
    s8 angle;
    char padding[2];
    float unidentified_04; // Observed only being written as 1.0f.
} UnkUiWork02;

typedef struct {
    struct {
        u8 destroy_requested : 1;
        u8 closing : 1;
    };
    s8 state;
    s8 delay;
    s8 angle;
    Vec *positions;
} UnkUiWork03;

typedef struct {
    struct {
        u8 destroy_requested : 1;
    };
    s8 current_item;
    s8 item_count;
    s8 moving;
    UnkUiWindowStruct *window_data;
} UnkUiWork04;

void BoardBowserSuitMotionCreate(void);
s16 BoardBowserSuitPlayerModelGet(void);

static void KillAllBoardStatus(void);
static void CreateBoardStatusSprite(s32 player, UnkUiStatusStruct *status);
static s32 UpdateBoardStatus(void);
static void BoardStatusProc(void);
static void MoveBoardStatus(s32 player);
static void UpdateStatusItem(UnkUiStatusStruct *status, s32 item_slot, s32 item);
static void UpdatePicker(OMOBJ *object);
static void UpdatePickerInput(UnkUiWork01 *work, OMOBJ *object);
static void MovePicker(UnkUiWork01 *work, OMOBJ *object);
static void UpdateComPickerButton(UnkUiWork01 *work, OMOBJ *object, s32 *input);
static BOOL CheckPickerButton(UnkUiWork01 *work, OMOBJ *object, u32 input);
static void UpdateYourTurn(OMOBJ *object);
static void GrowYourTurn(UnkUiWork02 *work, OMOBJ *object);
static void WaitYourTurn(UnkUiWork02 *work, OMOBJ *object);
static void ShrinkYourTurn(UnkUiWork02 *work, OMOBJ *object);
static void ItemUseTeamProc(s32 show_team_layout);
static void ItemUseProc(void);
static void FinishItemUse(s16 original_space, s32 gaddlight_count_before);
static void DestroyItemUse(void);
static void TeamItemPosSet(s32 player, s32 item_slot, Vec *position);
static void ExecItemPick(void);
static void UpdateItemPick(OMOBJ *object);
static void MakeItemPickSpace(UnkUiWork03 *work);
static void SetItemUIStatus(s32 state);
static void ItemGetPos(s32 player, Vec *positions);
static void UpdateItemPickGfx(OMOBJ *object);
static void UpdateItemPickup(OMOBJ *object);
static BOOL CheckItemWindow(void);
static void HideItemWindow(void);
static void CreateItemWindow(s32 player, s32 item_count);
static BOOL CheckItemWindowSlide(void);
static void KillItemWindow(void);
static void SetItemWindowCurr(s32 item_index);
static void UpdateItemWindow(OMOBJ *object);
static void CreatePickerWindow(UnkUiWork01 *work, s32 allow_back);
static void KillPickerWindow(UnkUiWork01 *work);

static s16 itemMdlId[4][ARRAY_COUNT(GWPlayer->items)];
static Vec pickerPos[4];
static Vec itemPosTemp[6];

static s8 itemTeam;
static s8 itemTeamF;
s8 boardItemUser;
static s8 pickerChoice;
static s8 pickerChoiceDefault;
static s32 pickerBackF;
s32 lbl_801D40D0;
static s16 yourTurnSpr;
static s8 itemPlayer;
static s8 itemUsed;
static s8 teamItemPlayer;
static s16 itemRandTbl[ARRAY_COUNT(GWPlayer->items)];
static float itemPickupPos;
static OMOBJ *itemPickObj;
static HUPROCESS *itemUseProc;
static OMOBJ *yourTurnObj;
static OMOBJ *pickerObj;
static OMOBJ *itemWindowObj;
static HUPROCESS *statusProc;

static s16 itemPickupMdlId = -1;
static s16 yourTurnSprGrp = -1;

static UnkUiStatusStruct uiStatus[4] = { 0 };

static s32 statusSprTbl[11] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 0x27),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x26),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x29),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x28),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x28),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x28),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x2D),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x28),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x28),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x28),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x2C)
};

static s16 statusSprPrioTbl[12] = {
    0x05F0, 0x05E6, 0x05DC, 0x05D2,
    0x05DC, 0x05DC, 0x05DC, 0x05DC,
    0x05DC, 0x05DC, 0x05DC, 0x05DC
};

static float statusHideOfsTbl[4] = {
    -230.0f, 230.0f, -230.0f, 230.0f
};

static float statusPosTbl[4][2] = {
    { 114.0f, 84.0f },
    { HU_DISP_WIDTH-114, 84.0f },
    { 114.0f, HU_DISP_HEIGHT-84 },
    { HU_DISP_WIDTH-114, HU_DISP_HEIGHT-84 }
};

static u8 statusColTbl[4][4] = {
    { 0x80, 0x80, 0x80, 0x00 },
    { 0x1A, 0x84, 0xFF, 0x00 },
    { 0xFF, 0x1A, 0x2D, 0x00 },
    { 0x0A, 0xB4, 0x3C, 0x00 }
};

static float statusSprPosTbl[17][2] = {
    {   0.0f,   0.0f },
    {   0.0f,   0.0f },
    { -68.0f,   0.0f },
    {  80.0f,  14.0f },
    {  64.0f,  14.0f },
    {  48.0f,  14.0f },
    {  26.0f,  14.0f },
    {  80.0f, -12.0f },
    {  64.0f, -12.0f },
    {  48.0f, -12.0f },
    {  26.0f, -12.0f },
    { -16.0f,   0.0f },
    { -18.0f,  32.0f },
    {  24.0f, -34.0f },
    {  48.0f, -34.0f },
    {  72.0f, -34.0f },
    { -32.0f, -16.0f }
};

static float statusItemPosTbl[6][2] = {
    { -56.0f,   0.0f },
    { -58.0f,  32.0f },
    {  -2.0f,   4.0f }, //item 0 pos
    {  34.0f,  -8.0f }, //item 1 pos
    {  70.0f,   4.0f }, //item 2 pos
    { -56.0f, -16.0f }
};

static s32 itemMdlTbl[14] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 0x83),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x84),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x85),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x86),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x87),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x88),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x89),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x8A),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x8B),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x8C),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x8D),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x8E),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x8F),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x90)
};

static Vec teamItemStatusPosTbl[2] = {
    { HU_DISP_CENTERX-98, 116.0f, 0.0f },
    { HU_DISP_CENTERX+98, 116.0f, 0.0f }
};

/** @brief Gets the board model resource for an item ID. @param item Item ID. @return Board-directory model resource ID. */
s32 BoardItemModelGet(s32 item) {
    s32 model_files[14] = {
        DATA_MAKE_NUM(DATADIR_BOARD, 0x6D),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x6E),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x6F),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x70),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x71),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x72),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x73),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x74),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x76),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x77),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x78),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x79),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x7A),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x7B)
    };

    return model_files[item];
}

/** @brief Gets the effect-directory name resource for an item ID. @param item Item ID. @return Item-name resource ID. */
s32 BoardItemNameGet(s32 item) {
    s32 name_files[15] = {
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x00),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x01),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x02),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x03),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x04),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x05),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x06),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x07),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x08),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x09),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x0A),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x0B),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x0C),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x0D),
        DATA_MAKE_NUM(DATADIR_EFFECT, 0x0E)
    };

    return name_files[item];
}

/** @brief Checks whether a player's status panel has reached its target. @param player Player index. @return `TRUE` when stationary. */
BOOL BoardStatusStopCheck(s32 player) {
    return (uiStatus[player].moving) ? FALSE : TRUE;
}

/** @brief Gets a player's status-panel visibility flag. @param player Player index. @return Nonzero when shown. */
s32 BoardStatusVisibleGet(s32 player) {
    return uiStatus[player].visible;
}

/** @brief Requests the same visibility state for all four status panels. @param show Nonzero to show; zero to hide. */
void BoardStatusShowSetAll(s32 show) {
    s32 i;

    for (i = 0; i < 4; i++) {
        BoardStatusShowSet(i, show);
    }
}

/** @brief Immediately shows or hides status panels and their 3D item models. @param show Nonzero to show; zero to hide. */
void BoardStatusItemSet(s32 show) {
    UnkUiStatusStruct *status;
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        status = &uiStatus[i];
        BoardStatusShowSet(i, show);
        status->position.x = status->target_position.x;
        status->position.y = status->target_position.y;
        status->moving = 0;
        HuSprGrpPosSet(status->sprite_group, status->position.x, status->position.y);
        for (j = 0; j < ARRAY_COUNT(GWPlayer->items); j++) {
            if (itemMdlId[i][j] != -1) {
                BoardModelVisibilitySet(itemMdlId[i][j], show);
            }
        }
    }
}

/** @brief Marks a status panel so its next visibility change uses player zero's anchor. @param player Player index. */
void BoardStatusShowSetForce(s32 player) {
    uiStatus[player].force_show = 1;
}

/** @brief Switches a status panel between bright gray and dim styling. @param player Player index. @param gray Nonzero for bright gray; zero for dim. */
void BoardStatusGraySet(s32 player, s32 gray) {
    UnkUiStatusStruct *status;
    s16 sprite_indices[9] = { 3, 4, 5, 7, 8, 9, 2, 6, 10 };
    s16 priority;
    u8 color_level;
    s32 i;

    status = &uiStatus[player];
    if (gray != 0) {
        color_level = 0xFF;
        status->gray = 1;
        priority = 1500;
    } else {
        color_level = 0x3F;
        status->gray = 0;
        priority = 1530;
    }
    for (i = 0; i < 9; i++) {
        HuSprPriSet(status->sprite_group, sprite_indices[i], priority);
        HuSprColorSet(status->sprite_group, sprite_indices[i], color_level, color_level, color_level);
    }
}

/** @brief Animates one status panel on or off screen. @param player Player index. @param show Nonzero to show; zero to hide. */
void BoardStatusShowSet(s32 player, s32 show) {
    UnkUiStatusStruct *status = &uiStatus[player];

    if (status->force_show) {
        player = 0;
    }
    if (show == 0) {
        if (status->visible) {
            status->visible = 0;
            status->target_position.x = statusPosTbl[player][0] + statusHideOfsTbl[player];
            status->target_position.y = statusPosTbl[player][1];
            status->force_show = 0;
        }
    } else {
        if (!status->visible) {
            status->visible = 1;
            status->target_position.x = statusPosTbl[player][0];
            status->target_position.y = statusPosTbl[player][1];
            status->position.x = statusPosTbl[player][0] + statusHideOfsTbl[player];
            status->position.y = statusPosTbl[player][1];
        }
    }
}

/** @brief Sets whether item icons are drawn in a status panel. @param player Player index. @param show_items Nonzero to allow item icons; zero to hide them. */
void BoardStatusItemHideSet(s32 player, s32 show_items) {
    UnkUiStatusStruct *status = &uiStatus[player];

    if (show_items != 0) {
        status->items_visible = 1;
    } else {
        status->items_visible = 0;
    }
}

/** @brief Sets a status panel's 2D animation target. @param player Player index. @param[in] position Target screen position in pixels. */
void BoardStatusTargetPosSet(s32 player, Vec *position) {
    UnkUiStatusStruct *status = &uiStatus[player];

    status->target_position.x = position->x;
    status->target_position.y = position->y;
    status->moving = 1;
}

/** @brief Gets a status panel's current 2D screen position. @param player Player index. @param[out] position Receives X and Y in pixels. */
void BoardStatusPosGet(s32 player, Vec *position) {
    UnkUiStatusStruct *status = &uiStatus[player];

    position->x = status->position.x;
    position->y = status->position.y;
}

/** @brief Sets both current and target position for a status panel. @param player Player index. @param[in] position Screen position in pixels. */
void BoardStatusPosSet(s32 player, Vec *position) {
    UnkUiStatusStruct *status = &uiStatus[player];

    status->position.x = position->x;
    status->position.y = position->y;
    BoardStatusTargetPosSet(player, position);
}

/** @brief Creates the size-dependent hammer icon for a player's status panel. @param player Player index. */
void BoardStatusHammerCreate(s32 player) {
    UnkUiStatusStruct *status = &uiStatus[player];
    float sprite_pos[2];
    s32 sprite_file;
    s32 player_size;
    s16 sprite;

    player_size = BoardPlayerAutoSizeGet(player);
    switch (player_size) {
        case 0:
            return;
        case 2:
            sprite_file = DATA_MAKE_NUM(DATADIR_BOARD, 0x93);
            break;
        case 1:
            sprite_file = DATA_MAKE_NUM(DATADIR_BOARD, 0x92);
            break;
    }
    if (!status->hammer_created) {
        BoardSpriteCreate(sprite_file, 0x5DC, NULL, &sprite);
        sprite_pos[0] = statusSprPosTbl[16][0];
        sprite_pos[1] = statusSprPosTbl[16][1];
        HuSprGrpMemberSet(status->sprite_group, 0x10, sprite);
        HuSprPosSet(status->sprite_group, 0x10, sprite_pos[0], sprite_pos[1]);
        HuSprScaleSet(status->sprite_group, 0x10, 2.0f, 2.0f);
        HuSprAttrSet(status->sprite_group, 0x10, 2);
        HuSprAttrSet(status->sprite_group, 0x10, 8);
        HuSprDrawNoSet(status->sprite_group, 0x10, 0x40);
        status->hammer_created = 1;
    }
}

/** @brief Starts or freezes a player's hammer-icon animation. @param player Player index. @param animate Nonzero to animate; zero to freeze. */
void BoardStatusHammerShowSet(s32 player, s32 animate) {
    UnkUiStatusStruct *status = &uiStatus[player];

    if (status->hammer_created) {
        if (animate != 0) {
            HuSprAttrSet(status->sprite_group, 0x10, HUSPR_ATTR_LOOP);
        } else {
            HuSprAttrReset(status->sprite_group, 0x10, HUSPR_ATTR_LOOP);
            HuSprAttrSet(status->sprite_group, 0x10, HUSPR_ATTR_NOANIM);
        }
    }
}

/** @brief Removes a player's hammer icon if present. @param player Player index. */
void BoardStatusHammerKill(s32 player) {
    UnkUiStatusStruct *status = &uiStatus[player];

    if (status->hammer_created) {
        HuSprGrpMemberKill(status->sprite_group, 0x10);
        status->hammer_created = 0;
    }
}

/** @brief Destroys sprite groups for status entries that were created. */
void BoardStatusKill(void) {
    UnkUiStatusStruct *status;
    s32 i;

    for (i = 0; i < 4; i++) {
        status = &uiStatus[i];
        if (status->created) {
            if (status->sprite_group != -1) {
                HuSprGrpKill(status->sprite_group);
                status->sprite_group = -1;
            }
            status->hammer_created = 0;
        }
    }
}

/** @brief Creates all four player status panels and starts their update process. */
void BoardStatusCreate(void) {
    s32 i;

    memset(uiStatus, 0, 0x70);
    for (i = 0; i < 4; i++) {
        CreateBoardStatusSprite(i, &uiStatus[i]);
        uiStatus[i].created = 1;
        uiStatus[i].moving = 0;
        uiStatus[i].force_show = 0;
        uiStatus[i].gray = 1;
        uiStatus[i].items_visible = 1;
        uiStatus[i].hammer_created = 0;
        uiStatus[i].movement_divisor = 9;
        uiStatus[i].position.x = statusPosTbl[i][0] + statusHideOfsTbl[i];
        uiStatus[i].position.y = statusPosTbl[i][1];
        uiStatus[i].position.z = 0.0f;
        uiStatus[i].target_position = uiStatus[i].position;
        HuSprGrpDrawNoSet(uiStatus[i].sprite_group, 0x40);
    }
    HuSprExecLayerSet(0x40, 5);
    if (statusProc == 0) {
        statusProc = HuPrcChildCreate(BoardStatusProc, 0x1FFF, 0x4000, 0, HuPrcCurrentGet());
        HuPrcDestructorSet2(statusProc, KillAllBoardStatus);
    }
}

/** @brief Destroys status resources when the status process exits. */
static void KillAllBoardStatus(void) {
    BoardStatusKill();
    statusProc = NULL;
}

/** @brief Builds the fixed sprites for one player's status panel. @param player Player index. @param[out] status Status record receiving the sprite group. */
static void CreateBoardStatusSprite(s32 player, UnkUiStatusStruct *status) {
    float sprite_x;
    float sprite_y;
    s16 sprite;
    s32 character;
    s32 sprite_file;
    s16 sprite_priority;
    s32 character_sprite_files[] = {
        DATA_MAKE_NUM(DATADIR_BOARD, 0x2E),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x2F),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x30),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x31),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x32),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x33),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x34),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x35)
    };
    s32 i;

    character = GWPlayerCfg[player].character;
    status->sprite_group = HuSprGrpCreate(0x11);
    for (i = 0; i < 11; i++) {
        sprite_file = statusSprTbl[i];
        sprite_priority = statusSprPrioTbl[i];
        BoardSpriteCreate(sprite_file, sprite_priority, NULL, &sprite);
        HuSprGrpMemberSet(status->sprite_group, i, sprite);
        HuSprAttrSet(status->sprite_group, i, 1);
        HuSprAttrSet(status->sprite_group, i, 8);
        HuSprPosSet(status->sprite_group, i, statusSprPosTbl[i][0], statusSprPosTbl[i][1]);
    }
    if ((s32) GWSystem.team != 0) {
        if (GWPlayerTeamGet(player) == 0) {
            HuSprBankSet(status->sprite_group, 1, 2);
        } else {
            HuSprBankSet(status->sprite_group, 1, 1);
        }
    }
    BoardSpriteCreate(character_sprite_files[character], 0x5DC, NULL, &sprite);
    HuSprGrpMemberSet(status->sprite_group, 0xB, sprite);
    HuSprAttrSet(status->sprite_group, 0xB, 8);
    sprite_x = statusSprPosTbl[11][0];
    sprite_y = statusSprPosTbl[11][1];
    HuSprPosSet(status->sprite_group, 11, sprite_x, sprite_y);
    BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x2A), 0x5DC, NULL, &sprite);
    HuSprGrpMemberSet(status->sprite_group, 0xC, sprite);
    sprite_x = statusSprPosTbl[12][0];
    sprite_y = statusSprPosTbl[12][1];
    HuSprPosSet(status->sprite_group, 12, sprite_x, sprite_y);
    HuSprAttrSet(status->sprite_group, 0xC, 8);
    for (i = 0; i < 3; i++) {
        BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x91), 0x5DC, NULL, &sprite);
        HuSprGrpMemberSet(status->sprite_group, i + 0xD, sprite);
        HuSprAttrSet(status->sprite_group, i + 0xD, 1);
        HuSprAttrSet(status->sprite_group, i + 0xD, 8);
    }
    status->hammer_created = 0;
    HuSprTPLvlSet(status->sprite_group, 0, 0.7f);
    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        HuSprAttrSet(status->sprite_group, 7, 4);
        HuSprAttrSet(status->sprite_group, 8, 4);
        HuSprAttrSet(status->sprite_group, 9, 4);
        HuSprAttrSet(status->sprite_group, 0xA, 4);
    }
}

/** @brief Loads sprite animation data and creates a sprite. @param file Data resource ID. @param prio Sprite priority. @param[out] anim Optional animation pointer; data remains managed by the overlay heap. @param[out] sprite Optional created sprite ID. */
void BoardSpriteCreate(s32 file, s32 prio, ANIMDATA **anim, s16 *sprite) {
    s16 sprite_id;
    void *sprite_data;
    ANIMDATA *animation;

    sprite_data = HuDataSelHeapReadNum(file, HU_MEMNUM_OVL, HEAP_MODEL);
    animation = HuSprAnimRead(sprite_data);
    sprite_id = HuSprCreate(animation, prio, 0);
    if (anim) {
        *anim = animation;
    }
    if (sprite) {
        *sprite = sprite_id;
    }
}

/** @brief Refreshes ranks, counts, items, colors, and positions for all status panels. @return `0` on success or `-1` when a sprite group is unavailable. */
static s32 UpdateBoardStatus(void) {
    UnkUiStatusStruct *status;
    u8 *panel_color;
    s32 item;
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        status = &uiStatus[i];
        if (status->sprite_group == -1) {
            return -1;
        }
        GWPlayer[i].rank = BoardPlayerRankCalc(i);
        HuSprBankSet(status->sprite_group, 2, GWPlayer[i].rank);
        BoardSpriteDigitUpdate(status->sprite_group, 3, BoardPlayerCoinsGet(i));
        BoardSpriteDigitUpdate(status->sprite_group, 7, GWStarsGet(i));
        for (j = 0; j < ARRAY_COUNT(GWPlayer->items); j++) {
            item = GWPlayer[i].items[j];
            if (item != -1 && status->items_visible) {
                UpdateStatusItem(status, j, item);
                HuSprAttrReset(status->sprite_group, j + 0xD, HUSPR_ATTR_DISPOFF);
            } else {
                HuSprAttrSet(status->sprite_group, j + 0xD, HUSPR_ATTR_DISPOFF);
            }
        }
        if (GWPlayer[i].com) {
            HuSprAttrReset(status->sprite_group, 0xC, HUSPR_ATTR_DISPOFF);
        } else {
            HuSprAttrSet(status->sprite_group, 0xC, HUSPR_ATTR_DISPOFF);
        }
        panel_color = statusColTbl[GWPlayer[i].color];
        HuSprColorSet(status->sprite_group, 0, panel_color[0], panel_color[1], panel_color[2]);
        MoveBoardStatus(i);
        HuSprGrpPosSet(status->sprite_group, status->position.x, status->position.y);
    }
    return 0;
}

/** @brief Runs status-panel updates until shutdown or resource loss. */
static void BoardStatusProc(void) {
    while (1) {
        if (omSysExitReq != 0 || UpdateBoardStatus() != 0) {
            break;
        }
        HuPrcVSleep();
    }
    HuPrcEnd();
}

/** @brief Moves one status panel toward its target. @param player Player index. */
static void MoveBoardStatus(s32 player) {
    UnkUiStatusStruct *status = &uiStatus[player];
    float step_scale;
    Vec delta;

    VECSubtract(&status->target_position, &status->position, &delta);
    if (ABS(delta.x) < 1.0f && ABS(delta.y) < 1.0f) {
        delta = status->target_position;
        status->moving = 0;
    } else {
        OSs8tof32(&status->movement_divisor, &step_scale);
        step_scale /= 32;
        VECScale(&delta, &delta, step_scale);
        VECAdd(&status->position, &delta, &delta);
        status->moving = 1;
    }
    status->position.x = delta.x;
    status->position.y = delta.y;
}

/** @brief Writes a nonnegative value into three digit sprites. @param sprite_group Sprite group ID. @param ones_sprite Ones-digit member index; tens and hundreds follow. @param value Value split into decimal digits. */
void BoardSpriteDigitUpdate(s16 sprite_group, s16 ones_sprite, s32 value) {
    s32 hundreds_digit;
    s32 tens_digit;
    s32 ones_digit;

    hundreds_digit = value / 100;
    tens_digit = (value - hundreds_digit * 100) / 10;
    ones_digit = (value - (hundreds_digit * 100 + tens_digit * 10)) % 10;
    if (hundreds_digit == 0) {
        HuSprBankSet(sprite_group, ones_sprite + 2, 0xA);
    } else {
        HuSprBankSet(sprite_group, ones_sprite + 2, hundreds_digit);
    }
    if (tens_digit == 0 && hundreds_digit == 0) {
        HuSprBankSet(sprite_group, ones_sprite + 1, ones_digit);
        HuSprAttrSet(sprite_group, ones_sprite, 4);
    } else {
        HuSprBankSet(sprite_group, ones_sprite + 1, tens_digit);
        HuSprBankSet(sprite_group, ones_sprite, ones_digit);
        HuSprAttrReset(sprite_group, ones_sprite, HUSPR_ATTR_DISPOFF);
    }
}

/** @brief Updates one item icon in a status panel. @param status Status record owning the group. @param item_slot Inventory slot 0 through 2. @param item Item ID used as the sprite bank. */
static void UpdateStatusItem(UnkUiStatusStruct *status, s32 item_slot, s32 item) {
    float sprite_x;
    float sprite_y;

    sprite_x = statusSprPosTbl[item_slot + 0xD][0];
    sprite_y = statusSprPosTbl[item_slot + 0xD][1];
    HuSprBankSet(status->sprite_group, item_slot + 0xD, item);
    HuSprPosSet(status->sprite_group, item_slot + 0xD, sprite_x, sprite_y);
}

/** @brief Creates a character cursor for choosing among screen positions. @param player Player index controlling the picker. @param choice_count Number of positions copied from positions. @param[in] positions Array of picker positions. @param target_choice Choice a CPU player should select, or `-1` for normal input. */
void BoardPickerCreate(s32 player, s8 choice_count, void *positions, s8 target_choice) {
    UnkUiWork01 *work;
    OMOBJ *object;
    s32 character;
    s16 sprite;
    s32 character_sprite_files[] = {
        DATA_MAKE_NUM(DATADIR_BOARD, 0x1E),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x1F),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x20),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x21),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x22),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x23),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x24),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x25)
    };

    character = GWPlayer[player].character;
    object = omAddObjEx(boardObjMan, 0x107, 0, 0, -1, &UpdatePicker);
    pickerObj = object;
    work = omObjGetWork(object, UnkUiWork01);
    work->destroy_requested = 0;
    work->input_locked = 0;
    work->player = player;
    work->target_choice = target_choice;
    work->done = 0;
    work->choice = 0;
    work->state = 0;
    work->input_delay = 0xA;
    pickerBackF = 0;
    pickerChoiceDefault = choice_count;
    memset(&pickerPos, 0, sizeof(pickerPos));
    memcpy(&pickerPos, positions, pickerChoiceDefault * sizeof(Vec));
    work->sprite_group = HuSprGrpCreate(1);
    pickerChoice = -1;
    object->trans.x = object->rot.x = pickerPos[work->choice].x;
    object->trans.y = object->rot.y = pickerPos[work->choice].y;
    object->trans.z = object->rot.z = 0.0f;
    BoardSpriteCreate(character_sprite_files[character], 90, NULL, &sprite);
    HuSprGrpMemberSet(work->sprite_group, 0, sprite);
    HuSprPosSet(work->sprite_group, 0, object->trans.x, object->trans.y);
    HuSprBankSet(work->sprite_group, 0, 0);
    HuSprAttrSet(work->sprite_group, 0, 1);
    HuSprAttrSet(work->sprite_group, 0, 2);
    HuSprAttrSet(work->sprite_group, 0, 8);
    CreatePickerWindow(work, 1);
}

/** @brief Enables or suppresses backing out of the active picker. @param allow_back Nonzero to allow Back; zero to require a choice. */
void BoardPickerBackFlagSet(s32 allow_back) {
    if (pickerObj) {
        if (allow_back != 0) {
            pickerBackF = 0;
        } else {
            pickerBackF = 1;
        }
        if (allow_back == 0) {
            KillPickerWindow(omObjGetWork(pickerObj, UnkUiWork01));
            CreatePickerWindow(omObjGetWork(pickerObj, UnkUiWork01), 0);
        }
    }
}

/** @brief Checks whether the picker object has been destroyed. @return `TRUE` when no picker remains. */
BOOL BoardPickerDoneCheck(void) {
    if (pickerObj) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/** @brief Gets the active picker's current screen position. @param[out] position Destination vector. @return `0` on success or `-1` if no picker or destination exists. */
s32 BoardPickerPosGet(Vec *position) {
    if (pickerObj == 0 || position == 0) {
        return -1;
    }
    position->x = pickerObj->trans.x;
    position->y = pickerObj->trans.y;
    position->z = pickerObj->trans.z;
    return 0;
}

/** @brief Gets the last confirmed picker choice. @return Choice index, or `-1` before confirmation or after cancellation. */
s32 BoardPickerChoiceGet(void) {
    return pickerChoice;
}

/** @brief Updates picker lifetime, input, movement, and sprite position. @param object Picker object. */
static void UpdatePicker(OMOBJ *object) {
    UnkUiWork01 *work = omObjGetWork(object, UnkUiWork01);

    if (work->destroy_requested || BoardIsKill()) {
        HuWinKill(work->window);
        HuSprGrpKill(work->sprite_group);
        pickerObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
    } else {
        if (work->input_delay != 0) {
            work->input_delay--;
        } else if (pickerChoice != -1 || work->done) {
            work->destroy_requested = 1;
        } else {
            UpdatePickerInput(work, object);
        }
        MovePicker(work, object);
        HuSprPosSet(work->sprite_group, 0, object->trans.x, object->trans.y);
    }
}

/** @brief Reads human or CPU picker input and updates its target. @param work Picker work state. @param object Picker object receiving a target position. */
static void UpdatePickerInput(UnkUiWork01 *work, OMOBJ *object) {
    s32 input;
    s32 port;

    if (!work->input_locked) {
        input = 0;
        if (GWPlayer[work->player].com && work->target_choice != -1) {
            UpdateComPickerButton(work, object, &input);
        } else {
            port = GWPlayer[work->player].port;
            input = HuPadDStkRep[port] | HuPadBtnDown[port];
        }
        if (CheckPickerButton(work, object, input) != 0) {
            object->rot.x = pickerPos[work->choice].x;
            object->rot.y = pickerPos[work->choice].y;
            object->rot.z = pickerPos[work->choice].z;
        }
    }
}

/** @brief Moves the picker halfway toward its target each frame. @param work Picker work state receiving its input-lock flag. @param object Picker object containing current and target positions. */
static void MovePicker(UnkUiWork01 *work, OMOBJ *object) {
    float step_scale = 0.5f;
    Vec current_pos;
    Vec delta;

    delta.x = object->rot.x;
    delta.y = object->rot.y;
    delta.z = object->rot.z;
    current_pos.x = object->trans.x;
    current_pos.y = object->trans.y;
    current_pos.z = object->trans.z;
    VECSubtract(&delta, &current_pos, &delta);
    if (VECMag(&delta) < 1.0f) {
        object->trans.x = object->rot.x;
        object->trans.y = object->rot.y;
        object->trans.z = object->rot.z;
        work->input_locked = 0;
    } else {
        VECScale(&delta, &delta, step_scale);
        VECAdd(&current_pos, &delta, &delta);
        object->trans.x = delta.x;
        object->trans.y = delta.y;
        object->trans.z = delta.z;
        work->input_locked = 1;
    }
}

/** @brief Synthesizes CPU picker input toward its configured choice. @param work Picker work state. @param object Picker object; unused by this decision. @param[out] input Receives confirm or move-right input. */
static void UpdateComPickerButton(UnkUiWork01 *work, OMOBJ *object, s32 *input) {
    if (work->choice == work->target_choice) {
        *input = 0x100;
    } else {
        *input = 2;
    }
}

/** @brief Applies picker confirm, cancel, and horizontal input. @param work Picker work state. @param object Picker object retained by caller. @param input Button/directional bit mask. @return `TRUE` when input changed or completed the picker. */
static BOOL CheckPickerButton(UnkUiWork01 *work, OMOBJ *object, u32 input) {
    BOOL input_handled = FALSE;

    if (input == 0x100) {
        pickerChoice = work->choice;
        HuAudFXPlay(2);
        input_handled = TRUE;
    } else if (input == 0x200 && pickerBackF == 0) {
        work->destroy_requested = 1;
        work->done = 1;
        pickerChoice = -1;
        HuAudFXPlay(3);
        input_handled = TRUE;
    } else if (input & 1) {
        input_handled = TRUE;
        if (pickerChoiceDefault > 1) {
            HuAudFXPlay(0);
        }
        work->choice--;
    } else if (input & 2) {
        input_handled = TRUE;
        if (pickerChoiceDefault > 1) {
            HuAudFXPlay(0);
        }
        work->choice++;
    }
    if (work->choice < 0) {
        work->choice = pickerChoiceDefault - 1;
    }
    if (work->choice >= pickerChoiceDefault) {
        work->choice = 0;
    }
    if (GWPlayer[work->player].com && input_handled != 0) {
        work->input_delay = GWMessDelayGet();
    }
    return input_handled;
}

/** @brief Displays the animated "your turn" character graphic and waits for dismissal. @param player Active player index. */
void BoardYourTurnExec(s32 player) {
    s32 character;
    UnkUiWork02 *work;
    s32 character_sprite_files[] = {
        DATA_MAKE_NUM(DATADIR_BOARD, 0x43),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x44),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x45),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x46),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x47),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x48),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x49),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x4A)
    };

    character = GWPlayer[player].character;
    yourTurnObj = omAddObjEx(boardObjMan, 0x10B, 0, 0, -1, &UpdateYourTurn);
    work = omObjGetWork(yourTurnObj, UnkUiWork02);
    work->destroy_requested = 0;
    work->player = player;
    work->state = 0;
    work->angle = 0;
    yourTurnSprGrp = HuSprGrpCreate(3);
    BoardSpriteCreate(character_sprite_files[character], 0x5DC, NULL, &yourTurnSpr);
    HuSprGrpMemberSet(yourTurnSprGrp, 0, yourTurnSpr);
    HuSprPosSet(yourTurnSprGrp, 0, 0.0f, 0.0f);
    HuSprAttrSet(yourTurnSprGrp, 0, 1);
    HuSprScaleSet(yourTurnSprGrp, 0, 0.001f, 0.001f);
    HuSprAttrSet(yourTurnSprGrp, 0, 8);
    HuSprGrpPosSet(yourTurnSprGrp, HU_DISP_CENTERX, HU_DISP_CENTERY);
    HuAudFXPlay(0x307);
    while (yourTurnObj) {
        HuPrcVSleep();
    }
}

/** @brief Advances the grow, wait-for-input, and fade states of the turn graphic. @param object Turn graphic object. */
static void UpdateYourTurn(OMOBJ *object) {
    UnkUiWork02 *work = omObjGetWork(object, UnkUiWork02);

    if (work->destroy_requested || BoardIsKill()) {
        HuSprGrpKill(yourTurnSprGrp);
        yourTurnSprGrp = -1;
        yourTurnObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
    } else {
        switch (work->state) {
            case 0:
                GrowYourTurn(work, object);
                break;
            case 2:
                WaitYourTurn(work, object);
                break;
            case 1:
                ShrinkYourTurn(work, object);
                break;
        }
    }
}

/** @brief Scales the turn graphic in over a 90-degree sine curve. @param work Animation work state. @param object Turn object; unused by this phase. */
static void GrowYourTurn(UnkUiWork02 *work, OMOBJ *object) {
    float angle;

    work->angle += 7;
    if (work->angle > 90) {
        work->angle = 90;
        work->state = 1;
    }
    OSs8tof32(&work->angle, &angle);
    angle = 1.3f * sind(angle);
    HuSprScaleSet(yourTurnSprGrp, 0, angle, angle);
}

/** @brief Enlarges and fades the turn graphic after dismissal. @param work Animation work state. @param object Turn object; unused by this phase. */
static void WaitYourTurn(UnkUiWork02 *work, OMOBJ *object) {
    float fade;

    work->angle += 7;
    if (work->angle > 90) {
        work->angle = 90;
        work->destroy_requested = 1;
    }
    OSs8tof32(&work->angle, &fade);
    fade = sind(fade);
    HuSprScaleSet(yourTurnSprGrp, 0, 1.3f + fade, 1.3f + fade);
    HuSprTPLvlSet(yourTurnSprGrp, 0, 1.0f - fade);
}

/** @brief Waits for confirm, CPU control, or tutorial mode before fading. @param work Animation work state. @param object Turn object; unused by this phase. */
static void ShrinkYourTurn(UnkUiWork02 *work, OMOBJ *object) {
    s32 port = GWPlayer[work->player].port;

    if ((HuPadBtnDown[port] & 0x100) || GWPlayer[work->player].com || _CheckFlag(FLAG_ID_MAKE(1, 11))) {
        work->state = 2;
        work->angle = 0;
        work->unidentified_04 = 1.0f;
    }
}

/** @brief Runs item selection and use for a player. @param player Player index. @return Used item ID, or `BOARD_ITEM_NONE` when cancelled or ineffective. */
s32 BoardItemUseExec(s32 player) {
    BoardPauseDisableSet(1);
    itemPlayer = player;
    itemUseProc = HuPrcChildCreate(ItemUseProc, 0x2004, 0x4000, 0, boardMainProc);
    HuPrcDestructorSet2(itemUseProc, DestroyItemUse);
    while (itemUseProc) {
        HuPrcVSleep();
    }
    return itemUsed;
}

/** @brief Rearranges status panels and item models for team item selection. @param show_team_layout Nonzero to enter the team layout; zero to restore all panels. */
static void ItemUseTeamProc(s32 show_team_layout) {
    UnkUiStatusStruct *status;
    Vec status_pos;
    Vec item_pos;
    s32 opposing_players[2];
    s32 i;
    s32 j;

    if (GWTeamGet()) {
        if (show_team_layout != 0) {
            teamItemPlayer = BoardPlayerSameTeamFind(itemPlayer);
            if (itemTeamF != 0) {
                BoardStatusTargetPosSet(itemPlayer, &teamItemStatusPosTbl[0]);
                BoardStatusTargetPosSet(teamItemPlayer, &teamItemStatusPosTbl[1]);
            } else {
                BoardStatusTargetPosSet(itemPlayer, (Vec*) statusPosTbl[0]);
                BoardStatusTargetPosSet(teamItemPlayer, (Vec*) statusPosTbl[1]);
            }
            opposing_players[0] = BoardPlayerTeamFind(itemPlayer);
            opposing_players[1] = BoardPlayerSameTeamFind(opposing_players[0]);
            for (i = 0; i < 2; i++) {
                status_pos.x = statusPosTbl[opposing_players[i]][0] + statusHideOfsTbl[opposing_players[i]];
                status_pos.y = statusPosTbl[opposing_players[i]][1];
                BoardStatusTargetPosSet(opposing_players[i], &status_pos);
            }
            while (!BoardStatusStopCheck(itemPlayer) || !BoardStatusStopCheck(teamItemPlayer)) {
                HuPrcVSleep();
            }
        } else {
            for (i = 0; i < 4; i++) {
                status_pos.x = statusPosTbl[i][0];
                status_pos.y = statusPosTbl[i][1];
                BoardStatusTargetPosSet(i, &status_pos);
            }
            while (!BoardStatusStopCheck(0)
                || !BoardStatusStopCheck(1)
                || !BoardStatusStopCheck(2)
                || !BoardStatusStopCheck(3)) {
                item_pos.z = 0.0f;
                for (i = 0; i < 4; i++) {
                    status = &uiStatus[i];
                    status_pos.x = status->position.x;
                    status_pos.y = status->position.y;
                    for (j = 0; j < BoardPlayerItemCount(i); j++) {
                        item_pos.x = status_pos.x + statusItemPosTbl[j + 2][0];
                        item_pos.y = status_pos.y + statusItemPosTbl[j + 2][1];
                        TeamItemPosSet(i, j, &item_pos);
                    }
                }
                HuPrcVSleep();
            }
        }
    }
}

/** @brief Coordinates item picking, resource loading, execution, and cleanup. */
static void ItemUseProc(void) {
    s32 data_read_id;
    s16 original_space;
    s32 gaddlight_count;
    s32 data_dir;
    s32 i;

    data_dir = -1;
    itemTeam = GWPlayerTeamGet(itemPlayer);
    if (GWTeamGet()) {
        itemTeamF = 1;
    } else {
        itemTeamF = 0;
    }
    for (i = 0; i < 4; i++) {
        GWPlayer[i].show_next = 0;
    }
    for (gaddlight_count = i = 0; i < ARRAY_COUNT(GWPlayer->items); i++) {
        if (GWPlayer[itemPlayer].items[i] == BOARD_ITEM_GADDLIGHT) {
            gaddlight_count++;
        }
    }
    original_space = GWPlayer[itemPlayer].space_curr;
    ItemUseTeamProc(1);
    BoardFilterFadeInit(30, 0xA0);
    ExecItemPick();
    ItemUseTeamProc(0);
    itemTeamF = 0;
    BoardFilterFadeOut(30);
    if (itemUsed == BOARD_ITEM_BOWSER_SUIT) {
        data_dir = DATADIR_BKOOPASUIT;
    }
    if (itemUsed == BOARD_ITEM_BOOS_CRYSTAL_BALL) {
        data_dir = DATADIR_BYOKODORI;
    }
    if (data_dir != -1) {
        data_read_id = BoardDataDirReadAsync(data_dir);
        BoardDataAsyncWait(data_read_id);
    }
    HideItemWindow();
    BoardItemPrevSet(itemUsed);
    if (itemUsed != BOARD_ITEM_NONE) {
        switch (itemUsed) {
            case 13:
                BoardMakeRandomItem();
                break;
            case 11:
                SetItemUIStatus(3);
                break;
            case 12:
                SetItemUIStatus(3);
                HuPrcSleep(0xA);
                BoardStatusShowSetAll(0);
                _SetFlag(0x1001C);
                _SetFlag(0x1000E);
                break;
        }
        BoardItemStart(itemPlayer, itemUsed);
        while (BoardItemDoneCheck() == 0) {
            HuPrcVSleep();
        }
        if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
            BoardTutorialHookExec(0x17, 0);
        }
        FinishItemUse(original_space, gaddlight_count);
    }
    if (itemUsed != BOARD_ITEM_BOOS_CRYSTAL_BALL && itemUsed != BOARD_ITEM_MAGIC_LAMP) {
        SetItemUIStatus(3);
    }
    while (itemPickObj) {
        HuPrcVSleep();
    }
    while (!BoardFilterFadePauseCheck()) {
        HuPrcVSleep();
    }
    HuPrcEnd();
}

/** @brief Applies post-use effects for special items. @param original_space Player's space before item execution; retained for matching but not read here. @param gaddlight_count_before Gaddlight count before item execution. */
static void FinishItemUse(s16 original_space, s32 gaddlight_count_before) {
    s32 gaddlight_count;
    s32 i;
    s16 player_model;

    switch (itemUsed) {
        case 12:
            player_model = BoardModelIDGet(BoardPlayerModelGet(itemPlayer));
            Hu3DModelShadowReset(player_model);
            _ClearFlag(0x1001C);
            _ClearFlag(0x1000E);
            Hu3DShadowF = 0;
            break;
        case 10:
            HuPrcSleep(2);
            BoardBowserSuitMotionCreate();
            BoardPlayerIdleSet(itemPlayer);
            BoardModelLayerSet(BoardBowserSuitModelGet(), 0);
            BoardModelLayerSet(BoardBowserSuitPlayerModelGet(), 0);
            break;
        case 7:
            BoardPlayerSparkSet(itemPlayer);
            break;
        case 8:
            for (gaddlight_count = i = 0; i < ARRAY_COUNT(GWPlayer->items); i++) {
                if (GWPlayer[itemPlayer].items[i] == BOARD_ITEM_GADDLIGHT) {
                    gaddlight_count++;
                }
            }
            if (gaddlight_count == gaddlight_count_before) {
                itemUsed = BOARD_ITEM_NONE;
            }
            break;
    }
}

/** @brief Restores player effects and closes item-specific data directories. */
static void DestroyItemUse(void) {
    s32 i;

    CharEffectLayerSet(1);
    for (i = 0; i < 4; i++) {
        GWPlayer[i].show_next = 1;
    }
    HuDataDirClose(DATADIR_BKOOPASUIT);
    HuDataDirClose(DATADIR_BYOKODORI);
    itemUseProc = NULL;
}

/** @brief Fills the item-bag table with random items excluding Bowser Suit and Item Bag. */
void BoardMakeRandomItem(void) {
    s32 chosen_item_index;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(itemRandTbl); i++) {
        chosen_item_index = BoardRandMod(BOARD_ITEMS_END);
        if (chosen_item_index != BOARD_ITEM_BOWSER_SUIT && chosen_item_index != BOARD_ITEM_ITEM_BAG) {
            itemRandTbl[i] = chosen_item_index;
        }
    }
    BoardItemBagItemSet(itemRandTbl);
}

/** @brief Stores an item's current team-layout position. @param player Player index. @param item_slot Inventory slot. @param[in] position Screen position. */
static inline void TeamItemPosSetInlineFunc01(s32 player, s32 item_slot, Vec *position) {
    UnkUiWork03 *work = omObjGetWork(itemPickObj, UnkUiWork03);
    Vec (*positions)[6] = (void*) work->positions;

    positions[player][item_slot + 2].x = position->x;
    positions[player][item_slot + 2].y = position->y;
    positions[player][item_slot + 2].z = position->z;
}

/** @brief Stores an item's destination team-layout position. @param player Player index. @param item_slot Inventory slot. @param[in] position Screen position. */
static inline void TeamItemPosSetInlineFunc02(s32 player, s32 item_slot, Vec *position) {
    UnkUiWork03 *work = omObjGetWork(itemPickObj, UnkUiWork03);
    Vec (*positions)[6] = (void*) work->positions;

    (positions + 4)[player][item_slot + 2].x = position->x;
    (positions + 4)[player][item_slot + 2].y = position->y;
    (positions + 4)[player][item_slot + 2].z = position->z;
}

/** @brief Updates both current and destination item positions for team layout. @param player Player index. @param item_slot Inventory slot. @param[in] position Screen position. */
static void TeamItemPosSet(s32 player, s32 item_slot, Vec *position) {
    TeamItemPosSetInlineFunc01(player, item_slot, position);
    TeamItemPosSetInlineFunc02(player, item_slot, position);
}

/** @brief Creates 3D inventory item models at supplied 2D panel positions. @param[in] positions Per-player position table; item slots begin at index 2. */
static inline void ExecItemPickInlineFunc01(Vec (*positions)[6]) {
    Vec camera_rotation;
    Vec screen_pos;
    Vec world_pos;
    s16 model;
    s32 item;
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < ARRAY_COUNT(GWPlayer->items); j++) {
            item = GWPlayer[i].items[j];
            if (item == BOARD_ITEM_NONE) {
                itemMdlId[i][j] = -1;
            } else {
                BoardCameraRotGet(&camera_rotation);
                model = BoardModelCreate(itemMdlTbl[item], 0, 0);
                itemMdlId[i][j] = model;
                screen_pos.x = positions[i][j + 2].x;
                screen_pos.y = positions[i][j + 2].y;
                screen_pos.z = 4000.0f;
                Hu3D2Dto3D(&screen_pos, 1, &world_pos);
                BoardModelPosSetV(model, &world_pos);
                BoardModelLayerSet(model, 5);
                BoardModelRotSet(model, camera_rotation.x, 0.0f, 0.0f);
                BoardModelScaleSet(model, 1.0f, 1.0f, 1.0f);
                BoardModelMotionStart(model, 0, 0x400000);
                BoardModelMotionStartEndSet(model, 0, 0x27);
                BoardModelMotionSpeedSet(model, 0.0f);
            }
        }
    }
}

/** @brief Creates the picker and item-description carousel, then waits until ready. @param player Player index. @param item_count Number of selectable items. @param cpu_choice CPU target choice, or `-1`. */
static inline void ExecItemPickInlineFunc02(s32 player, s8 item_count, s32 cpu_choice) {
    UnkUiWork03 *unused_picker_work;

    BoardPickerCreate(player, item_count, itemPosTemp, cpu_choice);
    CreateItemWindow(player, item_count);
    unused_picker_work = omObjGetWork(pickerObj, UnkUiWork03);
    pickerObj->objFunc = UpdateItemPickGfx;
    while (!CheckItemWindow()) {
        HuPrcVSleep();
    }
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        BoardTutorialHostHide(1);
    }
}

/** @brief Creates item models, computes panel destinations, and opens the item picker. */
static void ExecItemPick(void) {
    Vec panel_pos;
    s32 cpu_choice;
    s32 team_slot;
    s32 item_count;
    s32 i;
    s32 j;
    UnkUiWork03 *work;
    Vec (*positions)[6];

    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        BoardTutorialHostHide(0);
    }
    itemPickObj = omAddObjEx(boardObjMan, 0x7E03, 0, 0, -1, &UpdateItemPick);
    work = omObjGetWork(itemPickObj, UnkUiWork03);
    work->positions = HuMemDirectMallocNum(HEAP_HEAP, 8 * sizeof(Vec[6]), HU_MEMNUM_OVL);
    work->destroy_requested = 0;
    work->state = 0;
    work->delay = 0;
    work->angle = 0;
    work->closing = 0;
    BoardStatusItemHideSet(0, 0);
    BoardStatusItemHideSet(1, 0);
    BoardStatusItemHideSet(2, 0);
    BoardStatusItemHideSet(3, 0);
    positions = (void*) work->positions;
    for (team_slot = i = 0; i < 4; i++) {
        if (itemTeamF != 0) {
            if (itemTeam == GWPlayerTeamGet(i)) {
                if (itemPlayer == i) {
                    team_slot = 0;
                } else {
                    team_slot = 1;
                }
                panel_pos.x = teamItemStatusPosTbl[team_slot].x;
                panel_pos.y = teamItemStatusPosTbl[team_slot].y;
            } else {
                panel_pos.x = statusPosTbl[i][0] + statusHideOfsTbl[i];
                panel_pos.y = statusPosTbl[i][1];
            }
        } else {
            panel_pos.x = statusPosTbl[i][0];
            panel_pos.y = statusPosTbl[i][1];
        }
        for (j = 0; j < ARRAY_COUNT(statusItemPosTbl); j++) {
            positions[i][j].x = statusSprPosTbl[j + 0xB][0];
            positions[i][j].y = statusSprPosTbl[j + 0xB][1];
            (positions + 4)[i][j].x = statusItemPosTbl[j][0];
            (positions + 4)[i][j].y = statusItemPosTbl[j][1];
            if (j >= 2 && j <= 4) {
                positions[i][j].x += panel_pos.x;
                positions[i][j].y += panel_pos.y;
                (positions + 4)[i][j].x += panel_pos.x;
                (positions + 4)[i][j].y += panel_pos.y;
            }
            (positions + 4)[i][j].z = positions[i][j].z = 0.0f;
        }
    }
    ExecItemPickInlineFunc01((void*) work->positions);
    memset(itemPosTemp, 0, 0x48);
    item_count = BoardPlayerItemCount(itemPlayer);
    ItemGetPos(itemPlayer, &itemPosTemp[0]);
    if (GWTeamGet()) {
        ItemGetPos(teamItemPlayer, &itemPosTemp[item_count]);
        item_count += BoardPlayerItemCount(teamItemPlayer);
    }
    for (j = 0; j < item_count; j++) {
        itemPosTemp[j].x -= 8.0f;
        itemPosTemp[j].y += 16.0f;
    }
    cpu_choice = BoardComUseItemSlotGet(itemPlayer);
    ExecItemPickInlineFunc02(itemPlayer, item_count, cpu_choice);
}

/** @brief Updates item-panel transitions and synchronizes sprites and 3D models. @param object Item-pick layout object. */
static void UpdateItemPick(OMOBJ *object) {
    UnkUiWork03 *work = omObjGetWork(object, UnkUiWork03);
    UnkUiStatusStruct *status;
    Vec (*positions)[6];
    Vec screen_pos;
    Vec world_pos;
    float model_scale;
    s32 i;
    s32 j;

    if (work->destroy_requested || BoardIsKill()) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 3; j++) {
                if (itemMdlId[i][j] != -1) {
                    BoardModelKill(itemMdlId[i][j]);
                    itemMdlId[i][j] = -1;
                }
            }
        }
        BoardStatusItemHideSet(0, 1);
        BoardStatusItemHideSet(1, 1);
        BoardStatusItemHideSet(2, 1);
        BoardStatusItemHideSet(3, 1);
        HuMemDirectFree(work->positions);
        itemPickObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
    } else {
        if (work->delay != 0) {
            work->delay--;
        } else {
            switch (work->state) {
                case 3:
                    work->closing = 1;
                    /* fallthrough */
                case 0:
                    work->state = 1;
                    break;
                case 1:
                    break;
                default:
                    return;
            }
        }
        MakeItemPickSpace(work);
        positions = (void*) work->positions;
        for (i = 0; i < 4; i++) {
            status = &uiStatus[i];
            for (j = 0; j < 6; j++) {
                if (j != 5 || status->hammer_created) {
                    HuSprPosSet(status->sprite_group, j + 0xB, positions[i][j].x, positions[i][j].y);
                }
            }
            for (j = 0; j < 3; j++) {
                if (itemMdlId[i][j] != -1) {
                    OSs8tof32(&work->angle, &model_scale);
                    model_scale = 1.0 + 0.5 * sind(model_scale);
                    BoardModelScaleSet(itemMdlId[i][j], model_scale, model_scale, model_scale);
                    screen_pos.x = positions[i][j + 2].x;
                    screen_pos.y = positions[i][j + 2].y;
                    screen_pos.z = 4000.0f;
                    Hu3D2Dto3D(&screen_pos, 1, &world_pos);
                    BoardModelPosSetV(itemMdlId[i][j], &world_pos);
                }
            }
        }
    }
}

/** @brief Moves all status and item positions toward open or closed layout targets. @param work Item-pick work state containing current and target tables. */
static void MakeItemPickSpace(UnkUiWork03 *work) {
    Vec delta;
    float step_scale;
    s32 i;
    s32 j;
    Vec (*positions)[6];

    positions = (void*) work->positions;
    if (!work->closing) {
        if (work->angle < 90) {
            if (work->angle == 0) {
                for (j = 0; j < 4; j++) {
                    BoardStatusGraySet(j, 0);
                }
            }
            work->angle += 6;
            if (work->angle > 90) {
                work->angle = 90;
            }
        }
    } else {
        if (work->angle > 0) {
            work->angle -= 6;
            if (work->angle < 0) {
                work->angle = 0;
            }
            if (work->angle == 0) {
                for (j = 0; j < 4; j++) {
                    BoardStatusGraySet(j, 1);
                }
            }
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 6; j++) {
            VECSubtract(&(positions + 4)[i][j], &positions[i][j], &delta);
            if (ABS(delta.x) < 1.0f && ABS(delta.y) < 1.0f) {
                delta = (positions + 4)[i][j];
                if (work->closing && work->angle == 0) {
                    work->destroy_requested = 1;
                }
            } else {
                step_scale = 0.3f;
                VECScale(&delta, &delta, step_scale);
                VECAdd(&positions[i][j], &delta, &delta);
            }
            positions[i][j].x = delta.x;
            positions[i][j].y = delta.y;
        }
    }
}

/** @brief Selects the item-layout target state for all status panels. @param state Layout state: 0 or 3 for normal status positions, 2 for expanded item positions. */
static void SetItemUIStatus(s32 state) {
    UnkUiWork03 *work = omObjGetWork(itemPickObj, UnkUiWork03);
    Vec (*positions)[6] = (void*) work->positions;
    float panel_x;
    float panel_y;
    s32 i;
    s32 j;

    switch (state) {
        case 3:
        case 0:
            for (i = 0; i < 4; i++) {
                panel_x = statusPosTbl[i][0];
                panel_y = statusPosTbl[i][1];
                for (j = 0; j < 6; j++) {
                    (positions + 4)[i][j].x = statusSprPosTbl[j + 11][0];
                    (positions + 4)[i][j].y = statusSprPosTbl[j + 11][1];
                    if (j + 11 >= 13 && j + 11 <= 15) {
                        (positions + 4)[i][j].x += panel_x;
                        (positions + 4)[i][j].y += panel_y;
                    }
                }
            }
            break;
        case 2:
            for (i = 0; i < 4; i++) {
                panel_x = statusPosTbl[i][0];
                panel_y = statusPosTbl[i][1];
                for (j = 0; j < ARRAY_COUNT(statusItemPosTbl); j++) {
                    (positions + 4)[i][j].x = statusItemPosTbl[j][0];
                    (positions + 4)[i][j].y = statusItemPosTbl[j][1];
                    if ((j >= 2) && (j <= 4)) {
                        (positions + 4)[i][j].x += panel_x;
                        (positions + 4)[i][j].y += panel_y;
                    }
                }
            }
            break;
        default:
            return;
    }
    work->state = state;
}

/** @brief Computes the three normal item-slot positions for a status panel. @param player Player index. @param[out] positions Array of three destination vectors in screen pixels. */
void BoardItemGetDestPos(s32 player, Vec *positions) {
    Vec panel_pos;
    s32 i;

    panel_pos.x = statusPosTbl[player][0];
    panel_pos.y = statusPosTbl[player][1];
    panel_pos.z = 0.0f;
    for (i = 0; i < 3; i++) {
        VECAdd((Vec*) &statusItemPosTbl[i + 2], &panel_pos, &positions[i]);
    }
}

/** @brief Computes item-slot positions in the active normal or team layout. @param player Player index. @param[out] positions Array of three screen-space item positions. */
static void ItemGetPos(s32 player, Vec *positions) {
    Vec panel_pos;
    s32 team_slot;
    s32 i;

    if (itemTeamF != 0) {
        if (itemPlayer == player) {
            team_slot = 0;
        } else {
            team_slot = 1;
        }
        panel_pos.x = teamItemStatusPosTbl[team_slot].x;
        panel_pos.y = teamItemStatusPosTbl[team_slot].y;
    } else {
        panel_pos.x = statusPosTbl[player][0];
        panel_pos.y = statusPosTbl[player][1];
    }
    panel_pos.z = 0.0f;
    for (i = 0; i < 3; i++) {
        VECAdd((Vec*) statusItemPosTbl[i + 2], &panel_pos, &positions[i]);
    }
}

/** @brief Recreates one player's item models after inventory changes, preserving transforms. @param player Player index. */
void BoardItemStatusKill(s32 player) {
    Vec item_positions[3];
    Vec item_rotations[3];
    Vec item_scales[3];
    Vec camera_rotation;
    Vec panel_pos;
    s16 model;
    s16 item;
    s16 i;

    if (itemPickObj) {
        if (itemTeamF != 0) {
            if (player == itemPlayer) {
                panel_pos.x = teamItemStatusPosTbl[0].x;
                panel_pos.y = teamItemStatusPosTbl[0].y;
            } else {
                panel_pos.x = teamItemStatusPosTbl[1].x;
                panel_pos.y = teamItemStatusPosTbl[1].y;
            }
        } else {
            panel_pos.x = statusPosTbl[player][0];
            panel_pos.y = statusPosTbl[player][1];
        }
        panel_pos.z = 0.0f;
        for (i = 0; i < ARRAY_COUNT(GWPlayer->items); i++) {
            VECAdd((Vec*) statusItemPosTbl[i + 2], &panel_pos, &item_positions[i]);
            if (itemMdlId[player][i] != -1) {
                BoardModelRotGet(itemMdlId[player][i], &item_rotations[i]);
                BoardModelScaleGet(itemMdlId[player][i], &item_scales[i]);
                BoardModelKill(itemMdlId[player][i]);
                itemMdlId[player][i] = -1;
            }
        }
        for (i = 0; i < ARRAY_COUNT(GWPlayer->items); i++) {
            item = GWPlayer[player].items[i];
            if (item != BOARD_ITEM_NONE) {
                model = BoardModelCreate(itemMdlTbl[item], 0, 0);
                itemMdlId[player][i] = model;
                BoardCameraRotGet(&camera_rotation);
                BoardModelPosSetV(model, &item_positions[i]);
                BoardModelRotSet(model, camera_rotation.x, 0.0f, 0.0f);
                BoardModelScaleSetV(model, &item_scales[i]);
                BoardModelLayerSet(model, 6);
                BoardModelMotionStart(model, 0, 0x400000);
                BoardModelMotionSpeedSet(model, 0.0f);
                TeamItemPosSet(player, i, &item_positions[i]);
            }
        }
    }
}

/** @brief Updates the item picker and switches to pickup animation after confirmation. @param object Picker object. */
static void UpdateItemPickGfx(OMOBJ *object) {
    UnkUiWork01 *work = omObjGetWork(object, UnkUiWork01);

    if (work->destroy_requested || BoardIsKill()) {
        HuWinKill(work->window);
        HuSprGrpKill(work->sprite_group);
        pickerObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
    } else {
        if (work->input_delay != 0) {
            work->input_delay--;
        } else {
            if (!CheckItemWindowSlide()) {
                work->input_locked = 1;
            }
            UpdatePickerInput(work, object);
        }
        MovePicker(work, object);
        SetItemWindowCurr(work->choice);
        if (pickerChoice != -1 || work->done) {
            KillItemWindow();
            if (!work->done) {
                work->state = 1;
                HuSprBankSet(work->sprite_group, 0, 1);
                HuSprAttrSet(work->sprite_group, 0, HUSPR_ATTR_LOOP);
                HuSprAttrReset(work->sprite_group, 0, HUSPR_ATTR_NOANIM);
                object->objFunc = UpdateItemPickup;
                HuWinDispOff(work->window);
            } else {
                itemUsed = -1;
            }
        }
        HuSprPosSet(work->sprite_group, 0, object->trans.x, object->trans.y);
    }
}

/** @brief Animates the selected item from its panel into the using player. @param object Picker object reused for pickup state and screen position. */
static void UpdateItemPickup(OMOBJ *object) {
    UnkUiWork01 *work = omObjGetWork(object, UnkUiWork01);
    Vec model_pos;
    Vec model_scale;
    Vec model_rotation;
    Vec player_screen_pos;
    Vec picker_pos;
    Vec movement_delta;
    Vec player_world_pos;
    s16 item_owner;
    s16 item_slot;
    s16 motion_start;
    s16 motion_end;
    s16 motion_threshold;

    if (work->destroy_requested || BoardIsKill()) {
        HuWinKill(work->window);
        BoardModelKill(itemPickupMdlId);
        itemPickupMdlId = -1;
        HuSprGrpKill(work->sprite_group);
        pickerObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
    } else if (work->input_delay != 0) {
        work->input_delay--;
    } else if (pickerChoice == -1) {
        work->destroy_requested = 1;
    } else {
        item_owner = work->player;
        item_slot = pickerChoice;
        if (GWTeamGet() && pickerChoice >= BoardPlayerItemCount(work->player)) {
            item_owner = teamItemPlayer;
            item_slot -= BoardPlayerItemCount(work->player);
        }
        switch (work->state) {
            case 1:
                BoardModelVisibilitySet(itemMdlId[item_owner][item_slot], 0);
                BoardModelPosGet(itemMdlId[item_owner][item_slot], &model_pos);
                BoardModelRotGet(itemMdlId[item_owner][item_slot], &model_rotation);
                BoardModelScaleGet(itemMdlId[item_owner][item_slot], &model_scale);
                itemUsed = GWPlayer[item_owner].items[item_slot];
                boardItemUser = item_owner;
                itemPickupMdlId = BoardModelCreate(itemMdlTbl[itemUsed], NULL, 0);
                BoardModelPosSetV(itemPickupMdlId, &model_pos);
                BoardModelRotSetV(itemPickupMdlId, &model_rotation);
                BoardModelScaleSetV(itemPickupMdlId, &model_scale);
                BoardModelLayerSet(itemPickupMdlId, 6);
                BoardModelMotionStart(itemPickupMdlId, 0, 0x400000);
                BoardModelMotionSpeedSet(itemPickupMdlId, 0.0f);
                work->state = 2;
                break;
            case 2:
                motion_start = 0x27;
                motion_end = motion_start + 0x27;
                BoardModelMotionStart(itemPickupMdlId, 0, 0);
                BoardModelMotionTimeSet(itemPickupMdlId, motion_start);
                BoardModelMotionStartEndSet(itemPickupMdlId, motion_start, motion_end);
                HuSprBankSet(work->sprite_group, 0, 1);
                HuSprAttrSet(work->sprite_group, 0, 2);
                HuSprSpeedSet(work->sprite_group, 0, 1.0f);
                itemPickupPos = 1.0f;
                work->state = 3;
                HuAudFXPlay(0x30E);
                break;
            case 3:
                motion_threshold = 0x27;
                if (BoardModelMotionTimeGet(itemPickupMdlId) < motion_threshold + 0x17) {
                    object->trans.x -= itemPickupPos;
                    object->trans.y += itemPickupPos;
                    itemPickupPos *= 0.98f;
                } else {
                    model_pos.x = object->trans.x;
                    model_pos.y = object->trans.y;
                    model_pos.z = 3200.0f;
                    Hu3D2Dto3D(&model_pos, 1, &model_pos);
                    BoardModelPosSetV(itemPickupMdlId, &model_pos);
                    itemPickupPos = 4.0f;
                    work->state = 4;
                    HuAudFXPlay(0x30F);
                }
                break;
            case 4:
                motion_threshold = 0x4E;
                if (BoardModelMotionTimeGet(itemPickupMdlId) >= motion_threshold) {
                    work->input_delay = 0x12;
                    BoardPlayerItemRemove(item_owner, item_slot);
                    work->state = 5;
                } else {
                    object->trans.x -= itemPickupPos;
                    object->trans.y += itemPickupPos;
                    itemPickupPos *= 0.9f;
                    model_pos.x = object->trans.x;
                    model_pos.y = object->trans.y;
                    model_pos.z = 3200.0f;
                    Hu3D2Dto3D(&model_pos, 1, &model_pos);
                    BoardModelPosSetV(itemPickupMdlId, &model_pos);
                }
                break;
            case 5:
                BoardPlayerPosGet(work->player, &player_screen_pos);
                player_screen_pos.y += 300.0f;
                player_world_pos = player_screen_pos;
                Hu3D3Dto2D(&player_screen_pos, 1, &player_screen_pos);
                picker_pos.x = object->trans.x;
                picker_pos.y = object->trans.y;
                VECSubtract(&player_screen_pos, &picker_pos, &movement_delta);
                if (ABS(movement_delta.x) < 1.0f && ABS(movement_delta.y) < 1.0f) {
                    HuSprAttrSet(work->sprite_group, 0, 4);
                    work->state = 6;
                } else {
                    VECScale(&movement_delta, &movement_delta, 0.1f);
                    VECAdd(&picker_pos, &movement_delta, &model_pos);
                    model_pos.z = 3200.0f;
                    object->trans.x = model_pos.x;
                    object->trans.y = model_pos.y;
                    Hu3D2Dto3D(&model_pos, 1, &model_pos);
                    BoardModelPosSetV(itemPickupMdlId, &model_pos);
                }
                break;
            case 6:
                break;
        }
        HuSprPosSet(work->sprite_group, 0, object->trans.x, object->trans.y);
    }
}

/** @brief Checks whether item pickup reached its terminal state. @return `TRUE` when no picker exists or pickup state is 6. */
static BOOL CheckItemWindow(void) {
    UnkUiWork01 *work;

    if (pickerObj == 0) {
        return TRUE;
    }
    work = omObjGetWork(pickerObj, UnkUiWork01);
    if (work->state == 6) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/** @brief Requests destruction of the active item picker. */
static void HideItemWindow(void) {
    UnkUiWork01 *work;

    if (pickerObj) {
        work = omObjGetWork(pickerObj, UnkUiWork01);
        work->destroy_requested = 1;
    }
}

/** @brief Creates the scrolling item-name, description, and model carousel. @param player Initial item owner player index. @param item_count Total number of selectable items, including teammate items. */
static void CreateItemWindow(s32 player, s32 item_count) {
    Vec model_pos;
    float message_size[2];
    float model_scale;
    float team_y_offset;
    s32 item_slot;
    s32 item;
    s32 i;
    OMOBJ *object;
    UnkUiWork04 *work;
    UnkUiWindowStruct *window_data;

    model_scale = 0.25f;
    object = omAddObjEx(boardObjMan, 0x107, 0, 0, -1, UpdateItemWindow);
    itemWindowObj = object;
    work = omObjGetWork(object, UnkUiWork04);
    work->destroy_requested = 0;
    work->current_item = 0;
    work->moving = 0;
    work->item_count = item_count;
    work->window_data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(UnkUiWindowStruct), HU_MEMNUM_OVL);
    object->trans.x = 0.0f;
    if (GWTeamGet()) {
        team_y_offset = 32.0f;
    } else {
        team_y_offset = 0.0f;
    }
    window_data = work->window_data;
    for (i = 0; i < item_count; i++) {
        item_slot = i;
        if (GWTeamGet() && i >= BoardPlayerItemCount(itemPlayer)) {
            player = teamItemPlayer;
            item_slot -= BoardPlayerItemCount(itemPlayer);
        }
        item = GWPlayer[player].items[item_slot];
        if (item == BOARD_ITEM_NONE) {
            window_data->name_window[i] = window_data->description_window[i]
                = window_data->item_model[i] = window_data->item_id[i] = -1;
        } else {
            window_data->item_id[i] = item;
            window_data->target_position[i].x = i * HU_DISP_WIDTHF + 36.0f;
            window_data->target_position[i].y = team_y_offset + HU_DISP_CENTERY;
            window_data->target_position[i].z = 0.0f;
            window_data->position[i] = window_data->target_position[i];
            window_data->name_window[i] = HuWinCreate(window_data->position[i].x, window_data->position[i].y, 0x1F8, 0x60, 0);
            HuWinDrawNoSet(window_data->name_window[i], 0x40);
            HuWinCenterPosSet(window_data->name_window[i], 0.0f, 48.0f);
            HuWinMesSpeedSet(window_data->name_window[i], 0);
            HuWinMesSet(window_data->name_window[i], BoardItemNameGet(item));
            HuWinMesMaxSizeGet(1, message_size, item + MAKE_MESSID(0x08, 0x0E));
            window_data->description_window[i] = HuWinCreate(window_data->position[i].x, window_data->position[i].y - (-94.0f), message_size[0], message_size[1], 0);
            HuWinDrawNoSet(window_data->description_window[i], 0x40);
            HuWinMesSpeedSet(window_data->description_window[i], 0);
            HuWinMesSet(window_data->description_window[i], item + MAKE_MESSID(0x08, 0x0E));
            model_pos.x = window_data->position[i].x + 48.0f;
            model_pos.y = window_data->position[i].y;
            model_pos.z = 400.0f;
            Hu3D2Dto3D(&model_pos, 1, &model_pos);
            window_data->item_model[i] = BoardModelCreate(BoardItemModelGet(item), NULL, 0);
            BoardModelLayerSet(window_data->item_model[i], 6);
            BoardModelPosSetV(window_data->item_model[i], &model_pos);
            BoardModelScaleSet(window_data->item_model[i], model_scale, model_scale, model_scale);
            if (item == 4) {
                BoardModelMotionStart(window_data->item_model[i], 0, 0x40000001);
                BoardModelMotionSpeedSet(window_data->item_model[i], 0.033333335f);
            }
        }
    }
    for (; i < 6; i++) {
        window_data->item_model[i] = -1;
        window_data->name_window[i] = -1;
        window_data->description_window[i] = -1;
    }
}

/** @brief Checks whether the item carousel has stopped sliding. @return `TRUE` when absent or stationary. */
static BOOL CheckItemWindowSlide(void) {
    UnkUiWork04 *work;

    if (itemWindowObj == 0) {
        return TRUE;
    }
    work = omObjGetWork(itemWindowObj, UnkUiWork04);
    return (work->moving != 0) ? FALSE : TRUE;
}

/** @brief Requests destruction of the item-description carousel. */
static void KillItemWindow(void) {
    UnkUiWork04 *work;

    if (itemWindowObj) {
        work = omObjGetWork(itemWindowObj, UnkUiWork04);
        work->destroy_requested = 1;
    }
}

/** @brief Starts a horizontal carousel slide to another item. @param item_index New item index. */
static void SetItemWindowCurr(s32 item_index) {
    UnkUiWork04 *work;
    UnkUiWindowStruct *window_data;

    if (itemWindowObj) {
        work = omObjGetWork(itemWindowObj, UnkUiWork04);
        if (work->item_count > 1 && item_index != work->current_item) {
            window_data = work->window_data;
            window_data->target_position[work->current_item].x = -576.0f;
            window_data->target_position[item_index].x = window_data->position[work->current_item].x;
            window_data->position[item_index].x = 576.0f;
            work->current_item = item_index;
        }
    }
}

/** @brief Slides item carousel entries, aligns models to the camera, and destroys resources on request. @param object Carousel object. */
static void UpdateItemWindow(OMOBJ *object) {
    Vec movement_delta;
    Vec model_pos;
    Vec camera_rotation;
    UnkUiWork04 *work;
    UnkUiWindowStruct *window_data;
    s32 is_moving;
    s32 i;
    Mtx x_rotation;
    Mtx model_matrix;

    work = omObjGetWork(object, UnkUiWork04);
    window_data = work->window_data;
    if (work->destroy_requested || (BoardIsKill() != 0)) {
        for (i = 0; i < work->item_count; i++) {
            if (window_data->name_window[i] != -1) {
                HuWinKill(window_data->name_window[i]);
            }
            if (window_data->description_window[i] != -1) {
                HuWinKill(window_data->description_window[i]);
            }
            if (window_data->item_model[i] != -1) {
                BoardModelKill(window_data->item_model[i]);
            }
        }
        HuMemDirectFree(work->window_data);
        itemWindowObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
    } else {
        is_moving = 0;
        for (i = 0; i < work->item_count; i++) {
            if ((window_data->item_model[i] != -1) && (window_data->name_window[i] != -1)) {
                VECSubtract(&window_data->target_position[i], &window_data->position[i], &movement_delta);
                if (VECMag(&movement_delta) <= 1.0f) {
                    window_data->position[i] = window_data->target_position[i];
                } else {
                    VECScale(&movement_delta, &movement_delta, 0.3f);
                    VECAdd(&movement_delta, &window_data->position[i], &window_data->position[i]);
                    is_moving = 1;
                }
                HuWinPosSet(window_data->name_window[i], window_data->position[i].x, window_data->position[i].y);
                HuWinPosSet(window_data->description_window[i], window_data->position[i].x, window_data->position[i].y + (-94.0f));
                model_pos.x = window_data->position[i].x + 48.0f;
                model_pos.y = window_data->position[i].y;
                model_pos.z = 400.0f;
                if (window_data->item_id[i] == 5) {
                    model_pos.y += 35.0f;
                }
                Hu3D2Dto3D(&model_pos, 1, &model_pos);
                BoardModelPosSetV(window_data->item_model[i], &model_pos);
                BoardCameraRotGet(&camera_rotation);
                PSMTXRotRad(model_matrix, 'y', MTXDegToRad(object->trans.x));
                PSMTXRotRad(x_rotation, 'x', MTXDegToRad(camera_rotation.x + 10.0f));
                PSMTXConcat(x_rotation, model_matrix, model_matrix);
                BoardModelMtxSet(window_data->item_model[i], &model_matrix);
                BoardModelRotSet(window_data->item_model[i], 0.0f, 0.0f, 0.0f);
            }
        }
        work->moving = is_moving;
        object->trans.x = BoardDAngleCalc(object->trans.x + 2.0f);
    }
}

/** @brief Creates the picker control-hint window. @param work Picker work receiving the window ID. @param allow_back Nonzero for the confirm/back hint; zero for confirm-only text. */
static void CreatePickerWindow(UnkUiWork01 *work, s32 allow_back) {
    float message_size[2];
    
    float pos_x;
    float pos_y;
    float y_offset;
    s32 message_id;
    #if VERSION_NTSC
    switch (GWGameStat.language) {
        case 1:
            y_offset = 0.0f;
            break;
        case 0:
            y_offset = 0.0f;
            break;
    }
    #else
    y_offset = 0;
    #endif
    if (allow_back != 0) {
        message_id = MAKE_MESSID(0x10, 0x1A);
    } else {
        message_id = MAKE_MESSID(0x10, 0x1B);
    }
    HuWinMesMaxSizeGet(1, message_size, message_id);
    pos_x = -10000.0f;
    pos_y = y_offset + (HU_DISP_HEIGHT-128);
    work->window = HuWinCreate(pos_x, pos_y, message_size[0], message_size[1], 0);
    HuWinBGTPLvlSet(work->window, 0.0f);
    HuWinMesSpeedSet(work->window, 0);
    HuWinMesSet(work->window, message_id);
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        HuWinDispOff(work->window);
    }
}

/** @brief Destroys a picker's control-hint window. @param work Picker work containing the window ID. */
static void KillPickerWindow(UnkUiWork01 *work) {
    if (work->window != -1) {
        HuWinKill(work->window);
        work->window = -1;
    }
}
