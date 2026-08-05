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
    float scale;
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
static void CreateBoardStatusSprite(s32 arg0, UnkUiStatusStruct *arg1);
static s32 UpdateBoardStatus(void);
static void BoardStatusProc(void);
static void MoveBoardStatus(s32 arg0);
static void UpdateStatusItem(UnkUiStatusStruct *arg0, s32 arg1, s32 arg2);
static void UpdatePicker(OMOBJ *arg0);
static void UpdatePickerInput(UnkUiWork01 *arg0, OMOBJ *arg1);
static void MovePicker(UnkUiWork01 *arg0, OMOBJ *arg1);
static void UpdateComPickerButton(UnkUiWork01 *arg0, OMOBJ *arg1, s32 *arg2);
static BOOL CheckPickerButton(UnkUiWork01 *arg0, OMOBJ *arg1, u32 arg2);
static void UpdateYourTurn(OMOBJ *arg0);
static void GrowYourTurn(UnkUiWork02 *arg0, OMOBJ *arg1);
static void WaitYourTurn(UnkUiWork02 *arg0, OMOBJ *arg1);
static void ShrinkYourTurn(UnkUiWork02 *arg0, OMOBJ *arg1);
static void ItemUseTeamProc(s32 arg0);
static void ItemUseProc(void);
static void FinishItemUse(s16 arg0, s32 arg1);
static void DestroyItemUse(void);
static void TeamItemPosSet(s32 arg0, s32 arg1, Vec *arg2);
static void ExecItemPick(void);
static void UpdateItemPick(OMOBJ *arg0);
static void MakeItemPickSpace(UnkUiWork03 *arg0);
static void SetItemUIStatus(s32 arg0);
static void ItemGetPos(s32 arg0, Vec *arg1);
static void UpdateItemPickGfx(OMOBJ *arg0);
static void UpdateItemPickup(OMOBJ *arg0);
static BOOL CheckItemWindow(void);
static void HideItemWindow(void);
static void CreateItemWindow(s32 arg0, s32 arg1);
static BOOL CheckItemWindowSlide(void);
static void KillItemWindow(void);
static void SetItemWindowCurr(s32 arg0);
static void UpdateItemWindow(OMOBJ *arg0);
static void CreatePickerWindow(UnkUiWork01 *arg0, s32 arg1);
static void KillPickerWindow(UnkUiWork01 *arg0);

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

/** @brief Gets the board model resource for an item ID. @param arg0 Item ID. @return Board-directory model resource ID. */
s32 BoardItemModelGet(s32 arg0) {
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

    return model_files[arg0];
}

/** @brief Gets the effect-directory name resource for an item ID. @param arg0 Item ID. @return Item-name resource ID. */
s32 BoardItemNameGet(s32 arg0) {
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

    return name_files[arg0];
}

/** @brief Checks whether a player's status panel has reached its target. @param arg0 Player index. @return `TRUE` when stationary. */
BOOL BoardStatusStopCheck(s32 arg0) {
    return (uiStatus[arg0].moving) ? FALSE : TRUE;
}

/** @brief Gets a player's status-panel visibility flag. @param arg0 Player index. @return Nonzero when shown. */
s32 BoardStatusVisibleGet(s32 arg0) {
    return uiStatus[arg0].visible;
}

/** @brief Requests the same visibility state for all four status panels. @param arg0 Nonzero to show; zero to hide. */
void BoardStatusShowSetAll(s32 arg0) {
    s32 i;

    for (i = 0; i < 4; i++) {
        BoardStatusShowSet(i, arg0);
    }
}

/** @brief Immediately shows or hides status panels and their 3D item models. @param arg0 Nonzero to show; zero to hide. */
void BoardStatusItemSet(s32 arg0) {
    UnkUiStatusStruct *status;
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        status = &uiStatus[i];
        BoardStatusShowSet(i, arg0);
        status->position.x = status->target_position.x;
        status->position.y = status->target_position.y;
        status->moving = 0;
        HuSprGrpPosSet(status->sprite_group, status->position.x, status->position.y);
        for (j = 0; j < ARRAY_COUNT(GWPlayer->items); j++) {
            if (itemMdlId[i][j] != -1) {
                BoardModelVisibilitySet(itemMdlId[i][j], arg0);
            }
        }
    }
}

/** @brief Marks a status panel so its next visibility change uses player zero's anchor. @param arg0 Player index. */
void BoardStatusShowSetForce(s32 arg0) {
    uiStatus[arg0].force_show = 1;
}

/** @brief Switches a status panel between bright gray and dim styling. @param arg0 Player index. @param arg1 Nonzero for bright gray; zero for dim. */
void BoardStatusGraySet(s32 arg0, s32 arg1) {
    UnkUiStatusStruct *status;
    s16 sprite_indices[9] = { 3, 4, 5, 7, 8, 9, 2, 6, 10 };
    s16 priority;
    u8 color_level;
    s32 i;

    status = &uiStatus[arg0];
    if (arg1 != 0) {
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

/** @brief Animates one status panel on or off screen. @param arg0 Player index. @param arg1 Nonzero to show; zero to hide. */
void BoardStatusShowSet(s32 arg0, s32 arg1) {
    UnkUiStatusStruct *status = &uiStatus[arg0];

    if (status->force_show) {
        arg0 = 0;
    }
    if (arg1 == 0) {
        if (status->visible) {
            status->visible = 0;
            status->target_position.x = statusPosTbl[arg0][0] + statusHideOfsTbl[arg0];
            status->target_position.y = statusPosTbl[arg0][1];
            status->force_show = 0;
        }
    } else {
        if (!status->visible) {
            status->visible = 1;
            status->target_position.x = statusPosTbl[arg0][0];
            status->target_position.y = statusPosTbl[arg0][1];
            status->position.x = statusPosTbl[arg0][0] + statusHideOfsTbl[arg0];
            status->position.y = statusPosTbl[arg0][1];
        }
    }
}

/** @brief Sets whether item icons are drawn in a status panel. @param arg0 Player index. @param arg1 Nonzero to allow item icons; zero to hide them. */
void BoardStatusItemHideSet(s32 arg0, s32 arg1) {
    UnkUiStatusStruct *status = &uiStatus[arg0];

    if (arg1 != 0) {
        status->items_visible = 1;
    } else {
        status->items_visible = 0;
    }
}

/** @brief Sets a status panel's 2D animation target. @param arg0 Player index. @param[in] arg1 Target screen position in pixels. */
void BoardStatusTargetPosSet(s32 arg0, Vec *arg1) {
    UnkUiStatusStruct *status = &uiStatus[arg0];

    status->target_position.x = arg1->x;
    status->target_position.y = arg1->y;
    status->moving = 1;
}

/** @brief Gets a status panel's current 2D screen position. @param arg0 Player index. @param[out] arg1 Receives X and Y in pixels. */
void BoardStatusPosGet(s32 arg0, Vec *arg1) {
    UnkUiStatusStruct *status = &uiStatus[arg0];

    arg1->x = status->position.x;
    arg1->y = status->position.y;
}

/** @brief Sets both current and target position for a status panel. @param arg0 Player index. @param[in] arg1 Screen position in pixels. */
void BoardStatusPosSet(s32 arg0, Vec *arg1) {
    UnkUiStatusStruct *status = &uiStatus[arg0];

    status->position.x = arg1->x;
    status->position.y = arg1->y;
    BoardStatusTargetPosSet(arg0, arg1);
}

/** @brief Creates the size-dependent hammer icon for a player's status panel. @param arg0 Player index. */
void BoardStatusHammerCreate(s32 arg0) {
    UnkUiStatusStruct *status = &uiStatus[arg0];
    float sprite_pos[2];
    s32 sprite_file;
    s32 player_size;
    s16 sprite;

    player_size = BoardPlayerAutoSizeGet(arg0);
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

/** @brief Starts or freezes a player's hammer-icon animation. @param arg0 Player index. @param arg1 Nonzero to animate; zero to freeze. */
void BoardStatusHammerShowSet(s32 arg0, s32 arg1) {
    UnkUiStatusStruct *status = &uiStatus[arg0];

    if (status->hammer_created) {
        if (arg1 != 0) {
            HuSprAttrSet(status->sprite_group, 0x10, HUSPR_ATTR_LOOP);
        } else {
            HuSprAttrReset(status->sprite_group, 0x10, HUSPR_ATTR_LOOP);
            HuSprAttrSet(status->sprite_group, 0x10, HUSPR_ATTR_NOANIM);
        }
    }
}

/** @brief Removes a player's hammer icon if present. @param arg0 Player index. */
void BoardStatusHammerKill(s32 arg0) {
    UnkUiStatusStruct *status = &uiStatus[arg0];

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

/** @brief Builds the fixed sprites for one player's status panel. @param arg0 Player index. @param[out] arg1 Status record receiving the sprite group. */
static void CreateBoardStatusSprite(s32 arg0, UnkUiStatusStruct *arg1) {
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

    character = GWPlayerCfg[arg0].character;
    arg1->sprite_group = HuSprGrpCreate(0x11);
    for (i = 0; i < 11; i++) {
        sprite_file = statusSprTbl[i];
        sprite_priority = statusSprPrioTbl[i];
        BoardSpriteCreate(sprite_file, sprite_priority, NULL, &sprite);
        HuSprGrpMemberSet(arg1->sprite_group, i, sprite);
        HuSprAttrSet(arg1->sprite_group, i, 1);
        HuSprAttrSet(arg1->sprite_group, i, 8);
        HuSprPosSet(arg1->sprite_group, i, statusSprPosTbl[i][0], statusSprPosTbl[i][1]);
    }
    if ((s32) GWSystem.team != 0) {
        if (GWPlayerTeamGet(arg0) == 0) {
            HuSprBankSet(arg1->sprite_group, 1, 2);
        } else {
            HuSprBankSet(arg1->sprite_group, 1, 1);
        }
    }
    BoardSpriteCreate(character_sprite_files[character], 0x5DC, NULL, &sprite);
    HuSprGrpMemberSet(arg1->sprite_group, 0xB, sprite);
    HuSprAttrSet(arg1->sprite_group, 0xB, 8);
    sprite_x = statusSprPosTbl[11][0];
    sprite_y = statusSprPosTbl[11][1];
    HuSprPosSet(arg1->sprite_group, 11, sprite_x, sprite_y);
    BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x2A), 0x5DC, NULL, &sprite);
    HuSprGrpMemberSet(arg1->sprite_group, 0xC, sprite);
    sprite_x = statusSprPosTbl[12][0];
    sprite_y = statusSprPosTbl[12][1];
    HuSprPosSet(arg1->sprite_group, 12, sprite_x, sprite_y);
    HuSprAttrSet(arg1->sprite_group, 0xC, 8);
    for (i = 0; i < 3; i++) {
        BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x91), 0x5DC, NULL, &sprite);
        HuSprGrpMemberSet(arg1->sprite_group, i + 0xD, sprite);
        HuSprAttrSet(arg1->sprite_group, i + 0xD, 1);
        HuSprAttrSet(arg1->sprite_group, i + 0xD, 8);
    }
    arg1->hammer_created = 0;
    HuSprTPLvlSet(arg1->sprite_group, 0, 0.7f);
    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        HuSprAttrSet(arg1->sprite_group, 7, 4);
        HuSprAttrSet(arg1->sprite_group, 8, 4);
        HuSprAttrSet(arg1->sprite_group, 9, 4);
        HuSprAttrSet(arg1->sprite_group, 0xA, 4);
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

/** @brief Moves one status panel toward its target. @param arg0 Player index. */
static void MoveBoardStatus(s32 arg0) {
    UnkUiStatusStruct *status = &uiStatus[arg0];
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

/** @brief Writes a nonnegative value into three digit sprites. @param arg0 Sprite group ID. @param arg1 Ones-digit member index; tens and hundreds follow. @param arg2 Value split into decimal digits. */
void BoardSpriteDigitUpdate(s16 arg0, s16 arg1, s32 arg2) {
    s32 hundreds_digit;
    s32 tens_digit;
    s32 ones_digit;

    hundreds_digit = arg2 / 100;
    tens_digit = (arg2 - hundreds_digit * 100) / 10;
    ones_digit = (arg2 - (hundreds_digit * 100 + tens_digit * 10)) % 10;
    if (hundreds_digit == 0) {
        HuSprBankSet(arg0, arg1 + 2, 0xA);
    } else {
        HuSprBankSet(arg0, arg1 + 2, hundreds_digit);
    }
    if (tens_digit == 0 && hundreds_digit == 0) {
        HuSprBankSet(arg0, arg1 + 1, ones_digit);
        HuSprAttrSet(arg0, arg1, 4);
    } else {
        HuSprBankSet(arg0, arg1 + 1, tens_digit);
        HuSprBankSet(arg0, arg1, ones_digit);
        HuSprAttrReset(arg0, arg1, HUSPR_ATTR_DISPOFF);
    }
}

/** @brief Updates one item icon in a status panel. @param arg0 Status record owning the group. @param arg1 Inventory slot 0 through 2. @param arg2 Item ID used as the sprite bank. */
static void UpdateStatusItem(UnkUiStatusStruct *arg0, s32 arg1, s32 arg2) {
    float sprite_x;
    float sprite_y;

    sprite_x = statusSprPosTbl[arg1 + 0xD][0];
    sprite_y = statusSprPosTbl[arg1 + 0xD][1];
    HuSprBankSet(arg0->sprite_group, arg1 + 0xD, arg2);
    HuSprPosSet(arg0->sprite_group, arg1 + 0xD, sprite_x, sprite_y);
}

/** @brief Creates a character cursor for choosing among screen positions. @param arg0 Player index controlling the picker. @param arg1 Number of positions copied from arg2. @param[in] arg2 Array of picker positions. @param arg3 Choice a CPU player should select, or `-1` for normal input. */
void BoardPickerCreate(s32 arg0, s8 arg1, void *arg2, s8 arg3) {
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

    character = GWPlayer[arg0].character;
    object = omAddObjEx(boardObjMan, 0x107, 0, 0, -1, &UpdatePicker);
    pickerObj = object;
    work = omObjGetWork(object, UnkUiWork01);
    work->destroy_requested = 0;
    work->input_locked = 0;
    work->player = arg0;
    work->target_choice = arg3;
    work->done = 0;
    work->choice = 0;
    work->state = 0;
    work->input_delay = 0xA;
    pickerBackF = 0;
    pickerChoiceDefault = arg1;
    memset(&pickerPos, 0, sizeof(pickerPos));
    memcpy(&pickerPos, arg2, pickerChoiceDefault * sizeof(Vec));
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

/** @brief Enables or suppresses backing out of the active picker. @param arg0 Nonzero to suppress back; zero to allow it and show the back hint. */
void BoardPickerBackFlagSet(s32 arg0) {
    if (pickerObj) {
        if (arg0 != 0) {
            pickerBackF = 0;
        } else {
            pickerBackF = 1;
        }
        if (arg0 == 0) {
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

/** @brief Gets the active picker's current screen position. @param[out] arg0 Destination vector. @return `0` on success or `-1` if no picker or destination exists. */
s32 BoardPickerPosGet(Vec *arg0) {
    if (pickerObj == 0 || arg0 == 0) {
        return -1;
    }
    arg0->x = pickerObj->trans.x;
    arg0->y = pickerObj->trans.y;
    arg0->z = pickerObj->trans.z;
    return 0;
}

/** @brief Gets the last confirmed picker choice. @return Choice index, or `-1` before confirmation or after cancellation. */
s32 BoardPickerChoiceGet(void) {
    return pickerChoice;
}

/** @brief Updates picker lifetime, input, movement, and sprite position. @param arg0 Picker object. */
static void UpdatePicker(OMOBJ *arg0) {
    UnkUiWork01 *work = omObjGetWork(arg0, UnkUiWork01);

    if (work->destroy_requested || BoardIsKill()) {
        HuWinKill(work->window);
        HuSprGrpKill(work->sprite_group);
        pickerObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
    } else {
        if (work->input_delay != 0) {
            work->input_delay--;
        } else if (pickerChoice != -1 || work->done) {
            work->destroy_requested = 1;
        } else {
            UpdatePickerInput(work, arg0);
        }
        MovePicker(work, arg0);
        HuSprPosSet(work->sprite_group, 0, arg0->trans.x, arg0->trans.y);
    }
}

/** @brief Reads human or CPU picker input and updates its target. @param arg0 Picker work state. @param arg1 Picker object receiving a target position. */
static void UpdatePickerInput(UnkUiWork01 *arg0, OMOBJ *arg1) {
    s32 input;
    s32 port;

    if (!arg0->input_locked) {
        input = 0;
        if (GWPlayer[arg0->player].com && arg0->target_choice != -1) {
            UpdateComPickerButton(arg0, arg1, &input);
        } else {
            port = GWPlayer[arg0->player].port;
            input = HuPadDStkRep[port] | HuPadBtnDown[port];
        }
        if (CheckPickerButton(arg0, arg1, input) != 0) {
            arg1->rot.x = pickerPos[arg0->choice].x;
            arg1->rot.y = pickerPos[arg0->choice].y;
            arg1->rot.z = pickerPos[arg0->choice].z;
        }
    }
}

/** @brief Moves the picker halfway toward its target each frame. @param arg0 Picker work state receiving its input-lock flag. @param arg1 Picker object containing current and target positions. */
static void MovePicker(UnkUiWork01 *arg0, OMOBJ *arg1) {
    float step_scale = 0.5f;
    Vec current_pos;
    Vec delta;

    delta.x = arg1->rot.x;
    delta.y = arg1->rot.y;
    delta.z = arg1->rot.z;
    current_pos.x = arg1->trans.x;
    current_pos.y = arg1->trans.y;
    current_pos.z = arg1->trans.z;
    VECSubtract(&delta, &current_pos, &delta);
    if (VECMag(&delta) < 1.0f) {
        arg1->trans.x = arg1->rot.x;
        arg1->trans.y = arg1->rot.y;
        arg1->trans.z = arg1->rot.z;
        arg0->input_locked = 0;
    } else {
        VECScale(&delta, &delta, step_scale);
        VECAdd(&current_pos, &delta, &delta);
        arg1->trans.x = delta.x;
        arg1->trans.y = delta.y;
        arg1->trans.z = delta.z;
        arg0->input_locked = 1;
    }
}

/** @brief Synthesizes CPU picker input toward its configured choice. @param arg0 Picker work state. @param arg1 Picker object; unused by this decision. @param[out] arg2 Receives confirm or move-right input. */
static void UpdateComPickerButton(UnkUiWork01 *arg0, OMOBJ *arg1, s32 *arg2) {
    if (arg0->choice == arg0->target_choice) {
        *arg2 = 0x100;
    } else {
        *arg2 = 2;
    }
}

/** @brief Applies picker confirm, cancel, and horizontal input. @param arg0 Picker work state. @param arg1 Picker object retained by caller. @param arg2 Button/directional bit mask. @return `TRUE` when input changed or completed the picker. */
static BOOL CheckPickerButton(UnkUiWork01 *arg0, OMOBJ *arg1, u32 arg2) {
    BOOL input_handled = FALSE;

    if (arg2 == 0x100) {
        pickerChoice = arg0->choice;
        HuAudFXPlay(2);
        input_handled = TRUE;
    } else if (arg2 == 0x200 && pickerBackF == 0) {
        arg0->destroy_requested = 1;
        arg0->done = 1;
        pickerChoice = -1;
        HuAudFXPlay(3);
        input_handled = TRUE;
    } else if (arg2 & 1) {
        input_handled = TRUE;
        if (pickerChoiceDefault > 1) {
            HuAudFXPlay(0);
        }
        arg0->choice--;
    } else if (arg2 & 2) {
        input_handled = TRUE;
        if (pickerChoiceDefault > 1) {
            HuAudFXPlay(0);
        }
        arg0->choice++;
    }
    if (arg0->choice < 0) {
        arg0->choice = pickerChoiceDefault - 1;
    }
    if (arg0->choice >= pickerChoiceDefault) {
        arg0->choice = 0;
    }
    if (GWPlayer[arg0->player].com && input_handled != 0) {
        arg0->input_delay = GWMessDelayGet();
    }
    return input_handled;
}

/** @brief Displays the animated "your turn" character graphic and waits for dismissal. @param arg0 Active player index. */
void BoardYourTurnExec(s32 arg0) {
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

    character = GWPlayer[arg0].character;
    yourTurnObj = omAddObjEx(boardObjMan, 0x10B, 0, 0, -1, &UpdateYourTurn);
    work = omObjGetWork(yourTurnObj, UnkUiWork02);
    work->destroy_requested = 0;
    work->player = arg0;
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

/** @brief Advances the grow, wait-for-input, and fade states of the turn graphic. @param arg0 Turn graphic object. */
static void UpdateYourTurn(OMOBJ *arg0) {
    UnkUiWork02 *work = omObjGetWork(arg0, UnkUiWork02);

    if (work->destroy_requested || BoardIsKill()) {
        HuSprGrpKill(yourTurnSprGrp);
        yourTurnSprGrp = -1;
        yourTurnObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
    } else {
        switch (work->state) {
            case 0:
                GrowYourTurn(work, arg0);
                break;
            case 2:
                WaitYourTurn(work, arg0);
                break;
            case 1:
                ShrinkYourTurn(work, arg0);
                break;
        }
    }
}

/** @brief Scales the turn graphic in over a 90-degree sine curve. @param arg0 Animation work state. @param arg1 Turn object; unused by this phase. */
static void GrowYourTurn(UnkUiWork02 *arg0, OMOBJ *arg1) {
    float angle;

    arg0->angle += 7;
    if (arg0->angle > 90) {
        arg0->angle = 90;
        arg0->state = 1;
    }
    OSs8tof32(&arg0->angle, &angle);
    angle = 1.3f * sind(angle);
    HuSprScaleSet(yourTurnSprGrp, 0, angle, angle);
}

/** @brief Enlarges and fades the turn graphic after dismissal. @param arg0 Animation work state. @param arg1 Turn object; unused by this phase. */
static void WaitYourTurn(UnkUiWork02 *arg0, OMOBJ *arg1) {
    float fade;

    arg0->angle += 7;
    if (arg0->angle > 90) {
        arg0->angle = 90;
        arg0->destroy_requested = 1;
    }
    OSs8tof32(&arg0->angle, &fade);
    fade = sind(fade);
    HuSprScaleSet(yourTurnSprGrp, 0, 1.3f + fade, 1.3f + fade);
    HuSprTPLvlSet(yourTurnSprGrp, 0, 1.0f - fade);
}

/** @brief Waits for confirm, CPU control, or tutorial mode before fading. @param arg0 Animation work state. @param arg1 Turn object; unused by this phase. */
static void ShrinkYourTurn(UnkUiWork02 *arg0, OMOBJ *arg1) {
    s32 port = GWPlayer[arg0->player].port;

    if ((HuPadBtnDown[port] & 0x100) || GWPlayer[arg0->player].com || _CheckFlag(FLAG_ID_MAKE(1, 11))) {
        arg0->state = 2;
        arg0->angle = 0;
        arg0->scale = 1.0f;
    }
}

/** @brief Runs item selection and use for a player. @param arg0 Player index. @return Used item ID, or `BOARD_ITEM_NONE` when cancelled or ineffective. */
s32 BoardItemUseExec(s32 arg0) {
    BoardPauseDisableSet(1);
    itemPlayer = arg0;
    itemUseProc = HuPrcChildCreate(ItemUseProc, 0x2004, 0x4000, 0, boardMainProc);
    HuPrcDestructorSet2(itemUseProc, DestroyItemUse);
    while (itemUseProc) {
        HuPrcVSleep();
    }
    return itemUsed;
}

/** @brief Rearranges status panels and item models for team item selection. @param arg0 Nonzero to enter the team layout; zero to restore all panels. */
static void ItemUseTeamProc(s32 arg0) {
    UnkUiStatusStruct *status;
    Vec status_pos;
    Vec item_pos;
    s32 opposing_players[2];
    s32 i;
    s32 j;

    if (GWTeamGet()) {
        if (arg0 != 0) {
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
    s32 dataDir;
    s32 i;

    dataDir = -1;
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
        dataDir = DATADIR_BKOOPASUIT;
    }
    if (itemUsed == BOARD_ITEM_BOOS_CRYSTAL_BALL) {
        dataDir = DATADIR_BYOKODORI;
    }
    if (dataDir != -1) {
        data_read_id = BoardDataDirReadAsync(dataDir);
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

/** @brief Applies post-use effects for special items. @param arg0 Player's space before item execution; retained for matching but not read here. @param arg1 Gaddlight count before item execution. */
static void FinishItemUse(s16 arg0, s32 arg1) {
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
            if (gaddlight_count == arg1) {
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
    s32 chosenItemIndex;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(itemRandTbl); i++) {
        chosenItemIndex = BoardRandMod(BOARD_ITEMS_END);
        if (chosenItemIndex != BOARD_ITEM_BOWSER_SUIT && chosenItemIndex != BOARD_ITEM_ITEM_BAG) {
            itemRandTbl[i] = chosenItemIndex;
        }
    }
    BoardItemBagItemSet(itemRandTbl);
}

/** @brief Stores an item's current team-layout position. @param arg0 Player index. @param arg1 Inventory slot. @param[in] arg2 Screen position. */
static inline void TeamItemPosSetInlineFunc01(s32 arg0, s32 arg1, Vec *arg2) {
    UnkUiWork03 *work = omObjGetWork(itemPickObj, UnkUiWork03);
    Vec (*positions)[6] = (void*) work->positions;

    positions[arg0][arg1 + 2].x = arg2->x;
    positions[arg0][arg1 + 2].y = arg2->y;
    positions[arg0][arg1 + 2].z = arg2->z;
}

/** @brief Stores an item's destination team-layout position. @param arg0 Player index. @param arg1 Inventory slot. @param[in] arg2 Screen position. */
static inline void TeamItemPosSetInlineFunc02(s32 arg0, s32 arg1, Vec *arg2) {
    UnkUiWork03 *work = omObjGetWork(itemPickObj, UnkUiWork03);
    Vec (*positions)[6] = (void*) work->positions;

    (positions + 4)[arg0][arg1 + 2].x = arg2->x;
    (positions + 4)[arg0][arg1 + 2].y = arg2->y;
    (positions + 4)[arg0][arg1 + 2].z = arg2->z;
}

/** @brief Updates both current and destination item positions for team layout. @param arg0 Player index. @param arg1 Inventory slot. @param[in] arg2 Screen position. */
static void TeamItemPosSet(s32 arg0, s32 arg1, Vec *arg2) {
    TeamItemPosSetInlineFunc01(arg0, arg1, arg2);
    TeamItemPosSetInlineFunc02(arg0, arg1, arg2);
}

/** @brief Creates 3D inventory item models at supplied 2D panel positions. @param[in] arg0 Per-player position table; item slots begin at index 2. */
static inline void ExecItemPickInlineFunc01(Vec (*arg0)[6]) {
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
                screen_pos.x = arg0[i][j + 2].x;
                screen_pos.y = arg0[i][j + 2].y;
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

/** @brief Creates the picker and item-description carousel, then waits until ready. @param arg0 Player index. @param arg1 Number of selectable items. @param arg2 CPU target choice, or `-1`. */
static inline void ExecItemPickInlineFunc02(s32 arg0, s8 arg1, s32 arg2) {
    UnkUiWork03 *item_pick_work;

    BoardPickerCreate(arg0, arg1, itemPosTemp, arg2);
    CreateItemWindow(arg0, arg1);
    item_pick_work = omObjGetWork(pickerObj, UnkUiWork03);
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

/** @brief Updates item-panel transitions and synchronizes sprites and 3D models. @param arg0 Item-pick layout object. */
static void UpdateItemPick(OMOBJ *arg0) {
    UnkUiWork03 *work = omObjGetWork(arg0, UnkUiWork03);
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
        omDelObjEx(HuPrcCurrentGet(), arg0);
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

/** @brief Moves all status and item positions toward open or closed layout targets. @param arg0 Item-pick work state containing current and target tables. */
static void MakeItemPickSpace(UnkUiWork03 *arg0) {
    Vec delta;
    float step_scale;
    s32 i;
    s32 j;
    Vec (*positions)[6];

    positions = (void*) arg0->positions;
    if (!arg0->closing) {
        if (arg0->angle < 90) {
            if (arg0->angle == 0) {
                for (j = 0; j < 4; j++) {
                    BoardStatusGraySet(j, 0);
                }
            }
            arg0->angle += 6;
            if (arg0->angle > 90) {
                arg0->angle = 90;
            }
        }
    } else {
        if (arg0->angle > 0) {
            arg0->angle -= 6;
            if (arg0->angle < 0) {
                arg0->angle = 0;
            }
            if (arg0->angle == 0) {
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
                if (arg0->closing && arg0->angle == 0) {
                    arg0->destroy_requested = 1;
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

/** @brief Selects the item-layout target state for all status panels. @param arg0 Layout state: 0 or 3 for normal status positions, 2 for expanded item positions. */
static void SetItemUIStatus(s32 arg0) {
    UnkUiWork03 *work = omObjGetWork(itemPickObj, UnkUiWork03);
    Vec (*positions)[6] = (void*) work->positions;
    float panel_x;
    float panel_y;
    s32 i;
    s32 j;

    switch (arg0) {
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
    work->state = arg0;
}

/** @brief Computes the three normal item-slot positions for a status panel. @param arg0 Player index. @param[out] arg1 Array of three destination vectors in screen pixels. */
void BoardItemGetDestPos(s32 arg0, Vec *arg1) {
    Vec panel_pos;
    s32 i;

    panel_pos.x = statusPosTbl[arg0][0];
    panel_pos.y = statusPosTbl[arg0][1];
    panel_pos.z = 0.0f;
    for (i = 0; i < 3; i++) {
        VECAdd((Vec*) &statusItemPosTbl[i + 2], &panel_pos, &arg1[i]);
    }
}

/** @brief Computes item-slot positions in the active normal or team layout. @param arg0 Player index. @param[out] arg1 Array of three screen-space item positions. */
static void ItemGetPos(s32 arg0, Vec *arg1) {
    Vec panel_pos;
    s32 team_slot;
    s32 i;

    if (itemTeamF != 0) {
        if (itemPlayer == arg0) {
            team_slot = 0;
        } else {
            team_slot = 1;
        }
        panel_pos.x = teamItemStatusPosTbl[team_slot].x;
        panel_pos.y = teamItemStatusPosTbl[team_slot].y;
    } else {
        panel_pos.x = statusPosTbl[arg0][0];
        panel_pos.y = statusPosTbl[arg0][1];
    }
    panel_pos.z = 0.0f;
    for (i = 0; i < 3; i++) {
        VECAdd((Vec*) statusItemPosTbl[i + 2], &panel_pos, &arg1[i]);
    }
}

/** @brief Recreates one player's item models after inventory changes, preserving transforms. @param arg0 Player index. */
void BoardItemStatusKill(s32 arg0) {
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
            if (arg0 == itemPlayer) {
                panel_pos.x = teamItemStatusPosTbl[0].x;
                panel_pos.y = teamItemStatusPosTbl[0].y;
            } else {
                panel_pos.x = teamItemStatusPosTbl[1].x;
                panel_pos.y = teamItemStatusPosTbl[1].y;
            }
        } else {
            panel_pos.x = statusPosTbl[arg0][0];
            panel_pos.y = statusPosTbl[arg0][1];
        }
        panel_pos.z = 0.0f;
        for (i = 0; i < ARRAY_COUNT(GWPlayer->items); i++) {
            VECAdd((Vec*) statusItemPosTbl[i + 2], &panel_pos, &item_positions[i]);
            if (itemMdlId[arg0][i] != -1) {
                BoardModelRotGet(itemMdlId[arg0][i], &item_rotations[i]);
                BoardModelScaleGet(itemMdlId[arg0][i], &item_scales[i]);
                BoardModelKill(itemMdlId[arg0][i]);
                itemMdlId[arg0][i] = -1;
            }
        }
        for (i = 0; i < ARRAY_COUNT(GWPlayer->items); i++) {
            item = GWPlayer[arg0].items[i];
            if (item != BOARD_ITEM_NONE) {
                model = BoardModelCreate(itemMdlTbl[item], 0, 0);
                itemMdlId[arg0][i] = model;
                BoardCameraRotGet(&camera_rotation);
                BoardModelPosSetV(model, &item_positions[i]);
                BoardModelRotSet(model, camera_rotation.x, 0.0f, 0.0f);
                BoardModelScaleSetV(model, &item_scales[i]);
                BoardModelLayerSet(model, 6);
                BoardModelMotionStart(model, 0, 0x400000);
                BoardModelMotionSpeedSet(model, 0.0f);
                TeamItemPosSet(arg0, i, &item_positions[i]);
            }
        }
    }
}

/** @brief Updates the item picker and switches to pickup animation after confirmation. @param arg0 Picker object. */
static void UpdateItemPickGfx(OMOBJ *arg0) {
    UnkUiWork01 *work = omObjGetWork(arg0, UnkUiWork01);

    if (work->destroy_requested || BoardIsKill()) {
        HuWinKill(work->window);
        HuSprGrpKill(work->sprite_group);
        pickerObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
    } else {
        if (work->input_delay != 0) {
            work->input_delay--;
        } else {
            if (!CheckItemWindowSlide()) {
                work->input_locked = 1;
            }
            UpdatePickerInput(work, arg0);
        }
        MovePicker(work, arg0);
        SetItemWindowCurr(work->choice);
        if (pickerChoice != -1 || work->done) {
            KillItemWindow();
            if (!work->done) {
                work->state = 1;
                HuSprBankSet(work->sprite_group, 0, 1);
                HuSprAttrSet(work->sprite_group, 0, HUSPR_ATTR_LOOP);
                HuSprAttrReset(work->sprite_group, 0, HUSPR_ATTR_NOANIM);
                arg0->objFunc = UpdateItemPickup;
                HuWinDispOff(work->window);
            } else {
                itemUsed = -1;
            }
        }
        HuSprPosSet(work->sprite_group, 0, arg0->trans.x, arg0->trans.y);
    }
}

/** @brief Animates the selected item from its panel into the using player. @param arg0 Picker object reused for pickup state and screen position. */
static void UpdateItemPickup(OMOBJ *arg0) {
    UnkUiWork01 *work = omObjGetWork(arg0, UnkUiWork01);
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
        omDelObjEx(HuPrcCurrentGet(), arg0);
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
                    arg0->trans.x -= itemPickupPos;
                    arg0->trans.y += itemPickupPos;
                    itemPickupPos *= 0.98f;
                } else {
                    model_pos.x = arg0->trans.x;
                    model_pos.y = arg0->trans.y;
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
                    arg0->trans.x -= itemPickupPos;
                    arg0->trans.y += itemPickupPos;
                    itemPickupPos *= 0.9f;
                    model_pos.x = arg0->trans.x;
                    model_pos.y = arg0->trans.y;
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
                picker_pos.x = arg0->trans.x;
                picker_pos.y = arg0->trans.y;
                VECSubtract(&player_screen_pos, &picker_pos, &movement_delta);
                if (ABS(movement_delta.x) < 1.0f && ABS(movement_delta.y) < 1.0f) {
                    HuSprAttrSet(work->sprite_group, 0, 4);
                    work->state = 6;
                } else {
                    VECScale(&movement_delta, &movement_delta, 0.1f);
                    VECAdd(&picker_pos, &movement_delta, &model_pos);
                    model_pos.z = 3200.0f;
                    arg0->trans.x = model_pos.x;
                    arg0->trans.y = model_pos.y;
                    Hu3D2Dto3D(&model_pos, 1, &model_pos);
                    BoardModelPosSetV(itemPickupMdlId, &model_pos);
                }
                break;
            case 6:
                break;
        }
        HuSprPosSet(work->sprite_group, 0, arg0->trans.x, arg0->trans.y);
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

/** @brief Creates the scrolling item-name, description, and model carousel. @param arg0 Initial item owner player index. @param arg1 Total number of selectable items, including teammate items. */
static void CreateItemWindow(s32 arg0, s32 arg1) {
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
    work->item_count = arg1;
    work->window_data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(UnkUiWindowStruct), HU_MEMNUM_OVL);
    object->trans.x = 0.0f;
    if (GWTeamGet()) {
        team_y_offset = 32.0f;
    } else {
        team_y_offset = 0.0f;
    }
    window_data = work->window_data;
    for (i = 0; i < arg1; i++) {
        item_slot = i;
        if (GWTeamGet() && i >= BoardPlayerItemCount(itemPlayer)) {
            arg0 = teamItemPlayer;
            item_slot -= BoardPlayerItemCount(itemPlayer);
        }
        item = GWPlayer[arg0].items[item_slot];
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

/** @brief Starts a horizontal carousel slide to another item. @param arg0 New item index. */
static void SetItemWindowCurr(s32 arg0) {
    UnkUiWork04 *work;
    UnkUiWindowStruct *window_data;

    if (itemWindowObj) {
        work = omObjGetWork(itemWindowObj, UnkUiWork04);
        if (work->item_count > 1 && arg0 != work->current_item) {
            window_data = work->window_data;
            window_data->target_position[work->current_item].x = -576.0f;
            window_data->target_position[arg0].x = window_data->position[work->current_item].x;
            window_data->position[arg0].x = 576.0f;
            work->current_item = arg0;
        }
    }
}

/** @brief Slides item carousel entries, aligns models to the camera, and destroys resources on request. @param arg0 Carousel object. */
static void UpdateItemWindow(OMOBJ *arg0) {
    Vec movement_delta;
    Vec model_pos;
    Vec camera_rotation;
    UnkUiWork04 *work;
    UnkUiWindowStruct *window_data;
    s32 is_moving;
    s32 i;
    Mtx x_rotation;
    Mtx model_matrix;

    work = omObjGetWork(arg0, UnkUiWork04);
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
        omDelObjEx(HuPrcCurrentGet(), arg0);
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
                PSMTXRotRad(model_matrix, 'y', MTXDegToRad(arg0->trans.x));
                PSMTXRotRad(x_rotation, 'x', MTXDegToRad(camera_rotation.x + 10.0f));
                PSMTXConcat(x_rotation, model_matrix, model_matrix);
                BoardModelMtxSet(window_data->item_model[i], &model_matrix);
                BoardModelRotSet(window_data->item_model[i], 0.0f, 0.0f, 0.0f);
            }
        }
        work->moving = is_moving;
        arg0->trans.x = BoardDAngleCalc(arg0->trans.x + 2.0f);
    }
}

/** @brief Creates the picker control-hint window. @param arg0 Picker work receiving the window ID. @param arg1 Nonzero for confirm-only text; zero for confirm/back text. */
static void CreatePickerWindow(UnkUiWork01 *arg0, s32 arg1) {
    float message_size[2];
    
    float posX;
    float posY;
    float yOfs;
    s32 message_id;
    #if VERSION_NTSC
    switch (GWGameStat.language) {
        case 1:
            yOfs = 0.0f;
            break;
        case 0:
            yOfs = 0.0f;
            break;
    }
    #else
    yOfs = 0;
    #endif
    if (arg1 != 0) {
        message_id = MAKE_MESSID(0x10, 0x1A);
    } else {
        message_id = MAKE_MESSID(0x10, 0x1B);
    }
    HuWinMesMaxSizeGet(1, message_size, message_id);
    posX = -10000.0f;
    posY = yOfs + (HU_DISP_HEIGHT-128);
    arg0->window = HuWinCreate(posX, posY, message_size[0], message_size[1], 0);
    HuWinBGTPLvlSet(arg0->window, 0.0f);
    HuWinMesSpeedSet(arg0->window, 0);
    HuWinMesSet(arg0->window, message_id);
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        HuWinDispOff(arg0->window);
    }
}

/** @brief Destroys a picker's control-hint window. @param arg0 Picker work containing the window ID. */
static void KillPickerWindow(UnkUiWork01 *arg0) {
    if (arg0->window != -1) {
        HuWinKill(arg0->window);
        arg0->window = -1;
    }
}
