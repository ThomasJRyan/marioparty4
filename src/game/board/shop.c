#include "types.h"
#include "game/board/shop.h"
#include "game/audio.h"
#include "game/board/audio.h"
#include "game/board/com.h"
#include "game/board/com_path.h"
#include "game/board/item.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/tutorial.h"
#include "game/board/ui.h"
#include "game/board/view.h"
#include "game/board/window.h"
#include "game/disp.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"

#include "dolphin.h"
#include "ext_math.h"

#include "game/hsfex.h"

typedef struct {
    struct {
        u8 kill : 1;
        u8 item : 4;
    };
    u8 delay;
    s8 slot;
    s8 previous_slot;
    s8 scale_angle;
    char padding[1];
    s16 window;
} ShopWinWork;

typedef struct {
    struct {
        u8 kill : 1;
        u8 state : 3;
    };
    char padding[1];
    s16 frame;
    u16 delay;
} ItemGiveWork;

typedef struct {
    struct {
        u8 kill : 1;
        u8 player : 2;
    };
    u8 input_delay;
    s8 slot;
    s8 target_slot;
    u8 moving;
    u8 close_delay;
    s16 sprite_group;
} ItemChoiceWork;

static void ExecShop(void);
static void DestroyShop(void);
static void PopupShop(void);
static void CloseShop(void);
static void CreateShopWin(void);
static void UpdateShopWin(OMOBJ *window_object);
static void SetShopWinItem(ShopWinWork *window_work, OMOBJ *selector_object);
static void PauseShopWin(void);
static void StartItemGive(void);
static void ExecItemGive(OMOBJ *item_object);
static void MoveItemGive(OMOBJ *item_object, ItemGiveWork *item_work);
static void ShrinkItemGive(OMOBJ *item_object, ItemGiveWork *item_work);
static void WaitItemGive(OMOBJ *item_object, ItemGiveWork *item_work);
static void GetShopItems(s32 player);
static void GetDefaultShopItems(s32 unused_player);
static void SortShopItems(void);
static void DecideComEnter(s32 player);
static void DecideComBuy(s32 player);
static s32 GetComItemChoice(s32 player);
static void WaitItemChoice(void);
static void CreateShopItemChoice(s32 player, s32 initial_slot);
static s32 GetShopItemChoice(void);
static s32 GetShopItemWinChoice(void);
static void MoveShopItemChoice(OMOBJ *selector_object, ItemChoiceWork *choice_work);
static void UpdateShopItemChoice(OMOBJ *selector_object);
static void GetShopItemChoiceInput(ItemChoiceWork *choice_work, OMOBJ *selector_object, u32 *button_down);

static s32 activeItemTbl[5];
static s32 choiceEnableTbl[6];

static s16 shopMdlIdx[2][1];
static s16 *shopMdlPtr;
static s8 shopPlayer;
static s16 angleVal;
static s32 comF;
static OMOBJ *itemChoiceObj;
static OMOBJ *itemGiveObj;
static OMOBJ *shopWinObj;
static HUPROCESS *shopProc;

static s16 itemMdl = -1;
static s8 itemChoice = -1;

static BoardModelParam shopMdlParam[1] = {
    { 0x00070094, { 0, 1, 0, 0, 0 }, -1 }
};

static s16 hostMdl = -1;
static s16 shopMot[3] = { -1, -1, -1 };
static s8 itemCurChoice = -1;

static s8 defaultItemTbl[5] = {
    0x05, 0x07, 0x02, 0x03, 0x00
};

static s32 hostMessTbl[] = {
    MAKE_MESSID(0x0F, 0x04),
    MAKE_MESSID(0x0F, 0x07),
    MAKE_MESSID(0x0F, 0x0A),
    MAKE_MESSID(0x0F, 0x0D),
    MAKE_MESSID(0x0F, 0x10),
    MAKE_MESSID(0x0F, 0x13),
    MAKE_MESSID(0x0F, 0x04),
    MAKE_MESSID(0x0F, 0x24),
    MAKE_MESSID(0x0F, 0x24)
};

static s8 itemPriceTbl[] = {
     5,  5, 15, 15,
    10, 10, 15, 15,
    15, 15,  0, 25,
    30, 30,  0,  0
};

static s32 shopMotTbl[9][3] = {
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x01), DATA_MAKE_NUM(DATADIR_BGUEST, 0x03), DATA_MAKE_NUM(DATADIR_BGUEST, 0x04) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x09), DATA_MAKE_NUM(DATADIR_BGUEST, 0x0B), DATA_MAKE_NUM(DATADIR_BGUEST, 0x0C) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x0E), DATA_MAKE_NUM(DATADIR_BGUEST, 0x10), DATA_MAKE_NUM(DATADIR_BGUEST, 0x11) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x18), DATA_MAKE_NUM(DATADIR_BGUEST, 0x1A), DATA_MAKE_NUM(DATADIR_BGUEST, 0x1B) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x1D), DATA_MAKE_NUM(DATADIR_BGUEST, 0x1F), DATA_MAKE_NUM(DATADIR_BGUEST, 0x20) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x22), DATA_MAKE_NUM(DATADIR_BGUEST, 0x24), DATA_MAKE_NUM(DATADIR_BGUEST, 0x25) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x09), DATA_MAKE_NUM(DATADIR_BGUEST, 0x0B), DATA_MAKE_NUM(DATADIR_BGUEST, 0x0C) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x2A), DATA_MAKE_NUM(DATADIR_BGUEST, 0x2C), DATA_MAKE_NUM(DATADIR_BGUEST, 0x2D) },
    { DATA_MAKE_NUM(DATADIR_BGUEST, 0x2A), DATA_MAKE_NUM(DATADIR_BGUEST, 0x2C), DATA_MAKE_NUM(DATADIR_BGUEST, 0x2D) }
};

/**
 * @brief Sets the board model used as the shop host.
 * @param host_model Board-model handle for the shop host.
 */
void BoardShopHostSet(s16 host_model) {
    hostMdl = host_model;
}

/**
 * @brief Gets the board model used as the shop host.
 * @return Shop host board-model handle, or -1 when unset.
 */
s16 BoardShopHostGet(void) {
    return hostMdl;
}

/**
 * @brief Creates shop host motions and places the host at the board's shop space.
 */
void BoardShopInit(void) {
    BoardModelParam *model_params;
    Vec shop_position;
    Vec host_position;
    s32 j;
    s32 i;
    s32 shop_space_flags[2] = {
        0x00080000,
        0x00100000
    };

    for (i = 0; i < 2; i++) {
        BoardSpaceLinkTransformGet(shop_space_flags[i], &shop_position, &host_position, 0);
        shop_position.y -= 5.0f;
        for (j = 0; j < 1; j++) {
            model_params = &shopMdlParam[j];
            shopMdlIdx[i][j] = BoardModelCreateParam(model_params, &shop_position, &host_position);
        }
    }
    BoardModelVisibilitySet(hostMdl, 0);
}

/**
 * @brief Stops the shop process and releases shop host motions.
 */
void BoardShopKill(void) {
    s32 i;
    s32 j;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 1; j++) {
            if (shopMdlIdx[i][j] != -1) {
                BoardModelKill(shopMdlIdx[i][j]);
                shopMdlIdx[i][j] = -1;
            }
        }
    }
}

/**
 * @brief Runs and waits for one player's shop interaction.
 * @param player Player index entering the shop.
 * @param space Board-space index containing the shop.
 */
void BoardShopExec(s32 player, s32 space) {
    if (BoardPlayerSizeGet(GWSystem.player_curr) == 2) {
        return;
    }
    shopPlayer = player;
    BoardRollDispSet(0);
    shopProc = HuPrcChildCreate(ExecShop, 0x2003, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(shopProc, DestroyShop);
    while (shopProc) {
        HuPrcVSleep();
    }
    BoardRollDispSet(1);
}

/**
 * @brief Handles shop admission, stock display, purchasing, and player exit.
 */
static void ExecShop(void) {
    Vec shop_position;
    Vec entrance_position;
    Vec entrance_direction;
    Vec camera_rotation;
    Vec camera_offset;
    float return_angle;
    float entrance_angle;
    s16 entrance_space;
    s16 shop_space;
    u32 message;
    s32 choice;
    s32 board;
    s32 portrait;
    s32 item_price;
    s32 coin_delay;
    s32 i;

    itemCurChoice = -1;
    board = GWBoardGet();
    portrait = BoardWinPortraitGet();
    comF = 0;
    shop_space = GWPlayer[shopPlayer].space_curr;
    if (BoardSpaceFlagGet(0, shop_space) & 0x80000) {
        shopMdlPtr = shopMdlIdx[0];
    } else {
        shopMdlPtr = shopMdlIdx[1];
    }
    BoardPlayerIdleSet(shopPlayer);
    message = 0;
    if (BoardPlayerCoinsGet(shopPlayer) < 5) {
        message = MAKE_MESSID(15, 3);
    }
    if (BoardPlayerItemCount(shopPlayer) == 3) {
        message = MAKE_MESSID(15, 2);
    }
    if (GWSystem.turn == GWSystem.max_turn) {
        message = MAKE_MESSID(15, 1);
    }
    if (message != 0) {
        BoardWinCreate(0, message, -1);
        BoardWinWait();
        BoardWinKill();
        HuPrcEnd();
    }
    BoardWinCreateChoice(0, MAKE_MESSID(15, 0), -1, 0);
    DecideComEnter(shopPlayer);
    BoardWinWait();
    if (BoardWinChoiceGet() != 0) {
        HuPrcEnd();
    }
    BoardWinKill();
    BoardAudSeqPause(0, 1, 1000);
    BoardStatusShowSetAll(0);
    entrance_space = BoardSpaceLinkFlagSearch(0, shop_space, 0x02000000);
    BoardSpacePosGet(0, shop_space, &shop_position);
    BoardSpacePosGet(0, entrance_space, &entrance_position);
    VECSubtract(&entrance_position, &shop_position, &entrance_direction);
    VECNormalize(&entrance_direction, &entrance_direction);
    entrance_angle = atan2d(-entrance_direction.x, -entrance_direction.z);
    camera_rotation.x = -10.0f;
    camera_rotation.y = entrance_angle;
    camera_rotation.z = 0.0f;
    camera_offset.x = 0.0f;
    camera_offset.y = 170.0f;
    camera_offset.z = 0.0f;
    BoardCameraMotionStartEx(shopMdlPtr[0], &camera_rotation, &camera_offset, 1360.0f, -1.0f, 27);
    HuAudFXPlay(0x330);
    HuAudFXPlay(0x32D);
    PopupShop();
    BoardModelMotionShiftSet(BoardShopHostGet(), shopMot[2], 0.0f, 4.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(4);
    BoardPlayerRotSet(shopPlayer, 0.0f, entrance_angle, 0.0f);
    BoardPlayerMoveBetween(shopPlayer, shop_space, entrance_space);
    BoardPlayerIdleSet(shopPlayer);
    BoardMusStart(1, 3, 0x7F, 0);
    while (!BoardModelMotionEndCheck(BoardShopHostGet())) {
        HuPrcVSleep();
    }
    BoardModelMotionStart(BoardShopHostGet(), shopMot[0], 0x40000001);
    BoardStatusShowSetForce(shopPlayer);
    BoardStatusShowSet(shopPlayer, 1);
    GetShopItems(shopPlayer);
    SortShopItems();
    while (1) {
        BoardWinCreateChoice(1, hostMessTbl[board], portrait, 0);
        BoardWinAttrSet(0x10);
        DecideComBuy(shopPlayer);
        BoardWinWait();
        choice = BoardWinChoiceGet();
        if (choice == 1) {
            BoardWinCreate(2, hostMessTbl[board] + 1, portrait);
            BoardWinWait();
            BoardWinKill();
            break;
        }
        if (choice != 0) {
            BoardViewMapExec(shopPlayer);
            continue;
        }
        CreateShopWin();
        BoardWinCreate(1, hostMessTbl[board] + 2, portrait);
        BoardWinPriSet(1000);
        for (i = 0; i < 5; i++) {
            message = MAKE_MESSID(15, activeItemTbl[i]);
            message += 0x16;
            BoardWinInsertMesSet(message, i);
            choice = itemPriceTbl[activeItemTbl[i]];
            if (choice > BoardPlayerCoinsGet(shopPlayer)) {
                BoardWinChoiceDisable(i);
                choiceEnableTbl[i] = 0;
            } else {
                choiceEnableTbl[i] = 1;
            }
        }
        choiceEnableTbl[5] = 1;
        BoardWinSpeedSet(0);
        BoardWinPause();
        BoardWinAttrSet(0x10);
        HuPrcSleep(0xF);
        choice = GetComItemChoice(shopPlayer);
        CreateShopItemChoice(shopPlayer, choice);
        WaitItemChoice();
        itemCurChoice = GetShopItemChoice();
        BoardWinKill();
        HuPrcSleep(0xF);
        if (itemCurChoice == -1 || itemCurChoice == 5 || itemCurChoice == 0x7F) {
            PauseShopWin();
        } else {
            PauseShopWin();
            while (!BoardStatusStopCheck(shopPlayer)) {
                HuPrcVSleep();
            }
            if (itemCurChoice == 5) {
                BoardWinCreate(2, hostMessTbl[board] + 1, portrait);
                BoardWinWait();
                BoardWinKill();
                break;
            }
            item_price = itemPriceTbl[activeItemTbl[itemCurChoice]];
            if (item_price >= 20) {
                coin_delay = 3;
            } else {
                coin_delay = 6;
            }
            return_angle = BoardDAngleCalc(180.0f + BoardPlayerRotYGet(shopPlayer));
            BoardPlayerMotBlendSet(shopPlayer, return_angle, 0xF);
            for (i = 0; i < item_price; i++) {
                BoardPlayerCoinsAdd(shopPlayer, -1);
                HuAudFXPlay(0xE);
                HuPrcSleep(coin_delay);
            }
            HuAudFXPlay(0xF);
            StartItemGive();
            while (!BoardPlayerMotBlendCheck(shopPlayer)) {
                HuPrcVSleep();
            }
            while (itemGiveObj) {
                HuPrcVSleep();
            }
            if (activeItemTbl[itemCurChoice] != 0xD) {
                BoardPlayerItemAdd(shopPlayer, activeItemTbl[itemCurChoice]);
            }
            BoardPlayerMotionShiftSet(shopPlayer, 0xC, 0.0f, 4.0f, HU3D_MOTATTR_NONE);
            HuPrcSleep(5);
            BoardPlayerMotionEndWait(shopPlayer);
            HuPrcSleep(15);
            break;
        }
    }
    BoardAudSeqFadeOut(1, 1000);
    BoardModelVisibilitySet(itemMdl, 0);
    BoardModelMotionShiftSet(BoardShopHostGet(), shopMot[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    BoardStatusShowSet(shopPlayer, 0);
    BoardCameraTargetPlayerSet(shopPlayer);
    BoardCameraMotionWait();
    BoardCameraViewSet(1);
    BoardPlayerRotSet(shopPlayer, 0.0f, entrance_angle, 0.0f);
    BoardPlayerMoveBetween(shopPlayer, entrance_space, shop_space);
    BoardPlayerIdleSet(shopPlayer);
    BoardStatusShowSetAll(1);
    BoardCameraMotionWait();
    BoardAudSeqPause(0, 0, 1000);
    CloseShop();
    HuPrcEnd();
}

/**
 * @brief Restores the player and host and marks shop UI objects for destruction.
 */
static void DestroyShop(void) {
    s16 host_model;
    s32 i;

    for (i = 0; i < 2; i++) {
        BoardModelHookReset(shopMdlIdx[0][i]);
    }
    host_model = BoardShopHostGet();
    for (i = 0; i < 3; i++) {
        if (shopMot[i] != -1) {
            BoardModelMotionKill(host_model, shopMot[i]);
            shopMot[i] = -1;
        }
    }
    PauseShopWin();
    if (itemMdl != -1) {
        BoardModelKill(itemMdl);
        itemMdl = -1;
    }
    shopProc = NULL;
}

/**
 * @brief Animates the shop host and item models upward into view.
 */
static void PopupShop(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        BoardModelHookSet(shopMdlIdx[i][0], "shop_noko", BoardShopHostGet());
    }
    for (i = 0; i < 3; i++) {
        shopMot[i] = BoardModelMotionCreate(BoardShopHostGet(), shopMotTbl[GWBoardGet()][i]);
    }
    BoardModelMotionStart(BoardShopHostGet(), shopMot[0], 0x40000001);
    BoardModelVisibilitySet(BoardShopHostGet(), 1);
    BoardModelMotionStart(BoardShopHostGet(), 1, 0x40000001);
    BoardModelMotionStart(shopMdlPtr[0], 0, 0);
    BoardModelMotionSpeedSet(shopMdlPtr[0], 1.0f);
    omVibrate(shopPlayer, 0xC, 6, 6);
    HuAudFXPlay(0x32D);
    HuAudFXPlay(0x344);
    while (BoardModelMotionTimeGet(shopMdlPtr[0]) < 14.0f) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x32E);
    while (BoardModelMotionTimeGet(shopMdlPtr[0]) < 28.0f) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x32F);
    while (!BoardModelMotionEndCheck(shopMdlPtr[0])) {
        HuPrcVSleep();
    }
}

/**
 * @brief Animates the shop host and item models downward out of view.
 */
static void CloseShop(void) {
    BoardModelMotionStart(shopMdlPtr[0], 0, 0x40000004);
    while (BoardModelMotionTimeGet(shopMdlPtr[0]) > 28.0f) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x32F);
    while (BoardModelMotionEndCheck(shopMdlPtr[0]) == 0) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x32E);
    BoardModelAttrReset(shopMdlPtr[0], 0x40000004);
    BoardModelMotionSpeedSet(shopMdlPtr[0], 0.0f);
    BoardModelVisibilitySet(BoardShopHostGet(), 0);
}

/**
 * @brief Creates the item name-and-price window used by the shop selector.
 */
static void CreateShopWin(void) {
    Vec window_position;
    OMOBJ *window_object;
    ShopWinWork *window_work;

    window_object = omAddObjEx(boardObjMan, 0x109, 0, 0, -1, UpdateShopWin);
    window_work = omObjGetWork(window_object, ShopWinWork);
    window_work->kill = 0;
    window_work->window = -1;
    window_work->item = 0xFF;
    window_work->slot = -1;
    window_work->previous_slot = -1;
    window_work->delay = 0xC;
    window_work->scale_angle = 0;
    shopWinObj = window_object;
    window_work->window = HuWinCreate(36.0f, HU_DISP_HEIGHT-136, 0x1F8, 0x60, 0);
    HuWinMesSpeedSet(window_work->window, 0);
    HuWinDrawNoSet(window_work->window, 0x40);
    HuWinExAnimIn(window_work->window);
    window_position.x = 84.0f;
    window_position.y = HU_DISP_HEIGHT-88;
    window_position.z = 400.0f;
    window_object->scale.x = window_object->scale.y = window_object->scale.z = 0.25f;
    Hu3D2Dto3D(&window_position, 1, &window_position);
    window_object->trans.x = window_position.x;
    window_object->trans.y = window_position.y;
    window_object->trans.z = window_position.z;
    window_object->scale.x = window_object->scale.y = window_object->scale.z = 1.0f;
}

/**
 * @brief Updates or destroys the shop item information window.
 * @param window_object Object controlling the item name-and-price window.
 */
static void UpdateShopWin(OMOBJ *window_object) {
    Vec camera_rotation;
    float height_offset;
    float scale_angle;
    ShopWinWork *window_work;
    Mtx item_transform;
    Mtx camera_transform;

    window_work = omObjGetWork(window_object, ShopWinWork);
    if (window_work->kill != 0 || BoardIsKill()) {
        if (window_work->window != -1) {
            HuWinKill(window_work->window);
        }
        shopWinObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), window_object);
        return;
    }
    if (window_work->delay != 0) {
        window_work->delay--;
        return;
    }
    window_work->slot = GetShopItemWinChoice();
    if (window_work->slot != -1) {
        if (window_work->scale_angle == 90 && window_work->previous_slot != window_work->slot) {
            if (window_work->slot == 5) {
                HuWinDispOff(window_work->window);
                if (window_work->kill == 0) {
                    BoardModelVisibilitySet(itemMdl, 0);
                }
            } else {
                HuWinDispOn(window_work->window);
                SetShopWinItem(window_work, window_object);
                BoardModelVisibilitySet(itemMdl, 1);
                window_work->scale_angle = 0;
            }
            window_work->previous_slot = window_work->slot;
        }
        if (itemChoice == 0x7F) {
            BoardModelVisibilitySet(itemMdl, 0);
            HuWinDispOff(window_work->window);
        }
        height_offset = 0.0f;
        if (window_work->scale_angle < 90) {
            window_work->scale_angle += 15;
            if (window_work->scale_angle > 90) {
                window_work->scale_angle = 90;
            }
            OSs8tof32(&window_work->scale_angle, &scale_angle);
            window_object->scale.x = window_object->scale.y = window_object->scale.z = 0.25 * sind(scale_angle);
        }
        if (window_work->item == 5) {
            height_offset = -12.5f;
        }
        window_object->rot.y = BoardDAngleCalc(window_object->rot.y + 2.0f);
        BoardCameraRotGet(&camera_rotation);
        PSMTXRotRad(item_transform, 'y', MTXDegToRad(window_object->rot.y));
        PSMTXRotRad(camera_transform, 'x', MTXDegToRad(camera_rotation.x + 10.0f));
        PSMTXConcat(camera_transform, item_transform, item_transform);
        BoardModelMtxSet(itemMdl, &item_transform);
        BoardModelRotSet(itemMdl, 0.0f, 0.0f, 0.0f);
        BoardModelPosSet(itemMdl, window_object->trans.x, window_object->trans.y + height_offset, window_object->trans.z);
        BoardModelScaleSet(itemMdl, window_object->scale.x, window_object->scale.y, window_object->scale.z);
        OSf32tos16(&window_object->rot.y, &angleVal);
    }
}

/**
 * @brief Updates the shop window for the currently highlighted item.
 * @param window_work Mutable shop-window state.
 * @param selector_object Item selector whose current slot is displayed.
 */
static void SetShopWinItem(ShopWinWork *window_work, OMOBJ *selector_object) {
    Vec camera_direction;
    s32 item_resource;
    s8 message;

    message = activeItemTbl[window_work->slot];
    if (window_work->item != message) {
        if (itemMdl != -1) {
            BoardModelKill(itemMdl);
            itemMdl = -1;
        }
        item_resource = BoardItemModelGet(message);
        itemMdl = BoardModelCreate(item_resource, NULL, 0);
        BoardModelLayerSet(itemMdl, 6);
        HuWinMesSet(window_work->window, BoardItemNameGet(message));
        window_work->scale_angle = 0;
        window_work->item = message;
        if (message == 4) {
            BoardModelMotionStart(itemMdl, 0, 0x40000001);
            BoardModelMotionSpeedSet(itemMdl, 0.033333335f);
        }
        BoardCameraDirGet(&camera_direction);
        selector_object->rot.y = BoardDAngleCalc(atan2d(-camera_direction.x, -camera_direction.z));
        OSf32tos16(&selector_object->rot.y, &angleVal);
    }
}

/**
 * @brief Marks the shop item information window for destruction.
 */
static void PauseShopWin(void) {
    if (shopWinObj) {
        omObjGetWork(shopWinObj, ShopWinWork)->kill = 1;
        shopWinObj = NULL;
    }
}

/**
 * @brief Creates the shop item-award object and hooks the purchased model to the host.
 */
void StartItemGive(void) {
    Vec item_position;
    Vec player_position;
    OMOBJ *item_object;
    ItemGiveWork *item_work;
    Mtx identity_transform;

    item_object = omAddObjEx(boardObjMan, 0x109, 0, 0, -1, ExecItemGive);
    itemGiveObj = item_object;
    item_work = omObjGetWork(item_object, ItemGiveWork);
    item_work->kill = 0;
    item_work->state = 0;
    item_work->delay = 0;
    item_work->frame = 0;
    BoardModelMotionStart(itemMdl, 0, 0);
    BoardModelMotionSpeedSet(itemMdl, 0.0f);
    BoardModelPosGet(itemMdl, &item_position);
    PSMTXIdentity(identity_transform);
    BoardModelMtxSet(itemMdl, &identity_transform);
    OSs16tof32(&angleVal, &item_object->scale.z);
    item_object->scale.z = -item_object->scale.z / 40.0f;
    item_object->scale.x = 0.25f;
    item_object->scale.y = 0.015f;
    BoardPlayerPosGet(shopPlayer, &player_position);
    item_object->trans.x = item_position.x;
    item_object->trans.y = item_position.y;
    item_object->trans.z = item_position.z;
    item_object->rot.y = player_position.y + 250.0f;
    item_object->rot.x = (player_position.x - item_position.x) / 50.0f;
    item_object->rot.z = (player_position.z - item_position.z) / 50.0f;
    itemGiveObj = item_object;
}

/**
 * @brief Advances or destroys the shop item-award animation.
 * @param item_object Object controlling the purchased-item animation.
 */
static void ExecItemGive(OMOBJ *item_object) {
    ItemGiveWork *item_work;
    float item_angle;

    item_work = omObjGetWork(item_object, ItemGiveWork);
    if (item_work->kill != 0 || BoardIsKill()) {
        itemGiveObj = NULL;
        BoardModelVisibilitySet(itemMdl, 0);
        omDelObjEx(HuPrcCurrentGet(), item_object);
        return;
    }
    if (item_work->delay != 0) {
        item_work->delay--;
        return;
    }
    switch (item_work->state) {
        case 0:
            MoveItemGive(item_object, item_work);
            break;
        case 1:
            ShrinkItemGive(item_object, item_work);
            break;
        case 2:
            WaitItemGive(item_object, item_work);
            break;
    }
    OSs16tof32(&angleVal, &item_angle);
    BoardModelPosSet(itemMdl, item_object->trans.x, item_object->trans.y, item_object->trans.z);
    BoardModelScaleSet(itemMdl, item_object->scale.x, item_object->scale.x, item_object->scale.x);
    BoardModelRotYSet(itemMdl, item_angle);
}

/**
 * @brief Moves the purchased item from the host toward the player.
 * @param item_object Object carrying the item's current transform.
 * @param item_work Mutable item-award state.
 */
static void MoveItemGive(OMOBJ *item_object, ItemGiveWork *item_work) {
    Vec player_position;
    Vec camera_direction;
    float frame;
    float item_angle;

    if (item_work->frame >= 50.0f) {
        item_work->frame = 0;
        if (activeItemTbl[itemCurChoice] == 0xD) {
            item_work->state = 2;
            BoardMakeRandomItem();
            BoardItemStart(GWSystem.player_curr, 0xD);
        } else {
            item_work->state = 1;
            item_work->delay = 0xF;
            BoardPlayerPosGet(shopPlayer, &player_position);
            if (BoardPlayerSizeGet(shopPlayer) == 1) {
                item_object->rot.x = (player_position.y + 30.0f - item_object->trans.y) / 22.5f;
            } else {
                item_object->rot.x = (player_position.y + 70.0f - item_object->trans.y) / 22.5f;
            }
            item_object->trans.y = item_object->rot.y;
            item_object->scale.x = 1.0f;
        }
        return;
    }
    OSs16tof32(&item_work->frame, &frame);
    item_object->trans.y += 12.0f;
    if (item_work->frame >= 40.0f) {
        BoardPlayerPosGet(shopPlayer, &player_position);
        item_object->scale.x = 1.0f;
        item_object->trans.x = player_position.x;
        item_object->trans.z = player_position.z;
        frame = 40.0f - (50.0f - frame);
        frame = -0.08166667f * frame * frame;
        item_object->trans.y += frame;
        BoardCameraDirGet(&camera_direction);
        item_angle = BoardDAngleCalc(atan2d(-camera_direction.x, -camera_direction.z));
        OSf32tos16(&item_angle, &angleVal);
    } else {
        item_object->scale.x += item_object->scale.y;
        item_object->trans.x += item_object->rot.x;
        item_object->trans.z += item_object->rot.z;
        OSs16tof32(&angleVal, &item_angle);
        item_angle += item_object->scale.z;
        OSf32tos16(&item_angle, &angleVal);
    }
    if (item_object->trans.y < item_object->rot.y) {
        item_object->trans.y = item_object->rot.y;
    }
    item_work->frame++;
}

/**
 * @brief Shrinks and lowers the purchased item into the player.
 * @param item_object Object carrying the item's current transform.
 * @param item_work Mutable item-award state.
 */
static void ShrinkItemGive(OMOBJ *item_object, ItemGiveWork *item_work) {
    float scale_angle;

    if (item_work->frame == 0) {
        HuAudFXPlay(0x30D);
    }
    if (item_work->frame >= 90) {
        item_work->kill = 1;
        BoardModelVisibilitySet(itemMdl, 0);
        return;
    }
    OSs16tof32(&item_work->frame, &scale_angle);
    if (BoardPlayerSizeGet(shopPlayer) != 1) {
        scale_angle *= 1.5f;
    }
    if (scale_angle > 90.0f) {
        scale_angle = 90.0f;
    }
    item_object->scale.x = cosd(scale_angle);
    item_object->trans.y += item_object->rot.x;
    angleVal += 8;
    if (angleVal > 360) {
        angleVal -= 360;
    }
    item_work->frame += 4;
}

/**
 * @brief Waits briefly before hiding the completed item-award model.
 * @param item_object Item object retained for the shared state-handler signature; not read by this state.
 * @param item_work Mutable item-award state.
 */
static void WaitItemGive(OMOBJ *item_object, ItemGiveWork *item_work) {
    if (item_work->frame > 20) {
        BoardModelVisibilitySet(itemMdl, 0);
    } else {
        item_work->frame++;
    }
    if (BoardItemDoneCheck()) {
        item_work->kill = 1;
    }
}

static s8 itemPrioTbl[2][5][14] = {
    {
        { 0x14, 0x14, 0x0F, 0x0F, 0x05, 0x07, 0x05, 0x05, 0x00, 0x05, 0x00, 0x03, 0x00, 0x00 },
        { 0x0F, 0x0F, 0x0D, 0x0A, 0x0A, 0x0A, 0x0A, 0x05, 0x00, 0x07, 0x00, 0x05, 0x00, 0x00 },
        { 0x07, 0x07, 0x0E, 0x0C, 0x0A, 0x0A, 0x0A, 0x08, 0x05, 0x09, 0x00, 0x08, 0x00, 0x00 },
        { 0x0A, 0x0A, 0x0D, 0x0C, 0x0A, 0x0A, 0x07, 0x0A, 0x05, 0x05, 0x00, 0x08, 0x00, 0x00 },
        { 0x07, 0x07, 0x0D, 0x0A, 0x0D, 0x07, 0x0A, 0x0A, 0x07, 0x08, 0x00, 0x08, 0x00, 0x00 }
    },
    {
        { 0x0F, 0x0F, 0x12, 0x0F, 0x05, 0x05, 0x05, 0x05, 0x00, 0x05, 0x00, 0x02, 0x05, 0x05 },
        { 0x0F, 0x0F, 0x0D, 0x0A, 0x07, 0x0A, 0x07, 0x03, 0x00, 0x07, 0x00, 0x00, 0x05, 0x08 },
        { 0x05, 0x04, 0x0D, 0x0C, 0x0A, 0x05, 0x0A, 0x08, 0x05, 0x05, 0x00, 0x05, 0x08, 0x0A },
        { 0x07, 0x04, 0x0D, 0x0C, 0x08, 0x08, 0x07, 0x0A, 0x05, 0x05, 0x00, 0x05, 0x08, 0x08 },
        { 0x03, 0x02, 0x0D, 0x0A, 0x0D, 0x03, 0x08, 0x08, 0x07, 0x07, 0x00, 0x08, 0x08, 0x0A }
    }
};

static s8 rankItemGroupTbl[2][4][3] = {
    {
        { 0x00, 0x01, 0x03 }, { 0x00, 0x02, 0x04 },
        { 0x00, 0x02, 0x04 }, { 0x00, 0x02, 0x04 }
    },
    {
        { 0x00, 0x01, 0x03 }, { 0x00, 0x02, 0x04 },
        { 0x00, 0x02, 0x04 }, { 0x00, 0x02, 0x04 }
    }
};

static float cursorPosTbl[6][2] = {
    { 190.0f, 182.0f },
    { 190.0f, 208.0f },
    { 190.0f, 234.0f },
    { 190.0f, 260.0f },
    { 190.0f, 286.0f },
    { 190.0f, 312.0f }
};

/**
 * @brief Builds shop stock from board hooks or the default item table.
 * @param player Player index used by board-specific stock selection hooks.
 */
static void GetShopItems(s32 player) {
    s32 item_weight;
    s32 player_coins;
    s32 rank_group;
    s32 random_roll;
    s32 duplicate;
    s32 affordable_item_found;
    s32 turn_group;
    s32 coin_group;
    s32 cumulative_weight;
    s32 existing_slot;
    s32 stock_slot;
    s32 item_id;

    turn_group = 3.0f * ((float) GWSystem.turn / GWSystem.max_turn);
    if (BoardPlayerCoinsGet(player) < 30) {
        coin_group = 0;
    } else {
        coin_group = 1;
    }
    rank_group = rankItemGroupTbl[coin_group][GWPlayer[player].rank][turn_group];
    activeItemTbl[0] = activeItemTbl[1] = activeItemTbl[2] = activeItemTbl[3] = activeItemTbl[4] = -1;
    player_coins = BoardPlayerCoinsGet(shopPlayer);
    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        GetDefaultShopItems(player);
        return;
    }
    affordable_item_found = 0;
    stock_slot = 0;
    while (stock_slot < 5) {
        random_roll = BoardRandMod(100);
        cumulative_weight = 0;
        for (item_id = 13; item_id >= 0; item_id--) {
            item_weight = itemPrioTbl[coin_group][rank_group][item_id];
            if (item_weight != 0) {
                cumulative_weight += item_weight;
                if (random_roll <= cumulative_weight) {
                    duplicate = 0;
                    for (existing_slot = 0; existing_slot < stock_slot; existing_slot++) {
                        if (item_id == activeItemTbl[existing_slot]) {
                            duplicate = 1;
                            break;
                        }
                    }
                    if (duplicate == 0 && (player_coins >= itemPriceTbl[item_id] || affordable_item_found != 0)) {
                        affordable_item_found = 1;
                        activeItemTbl[stock_slot] = item_id;
                        break;
                    }
                }
            }
        }
        if (activeItemTbl[stock_slot] != -1) {
            stock_slot++;
        }
    }
}

/**
 * @brief Chooses a default five-item stock table using board randomness.
 * @param unused_player Player index forwarded by stock selection; the default-table path does not read it.
 */
static void GetDefaultShopItems(s32 unused_player) {
    s32 rand;
    s32 slot;
    s32 table;

    rand = BoardRandMod(100);
    if (rand > 90) {
        table = 3;
    } else if (rand > 40) {
        table = BoardRandMod(2) + 1;
    } else {
        table = 0;
    }
    for (slot = 0; slot < ARRAY_COUNT(activeItemTbl); slot++) {
        activeItemTbl[slot] = defaultItemTbl[table];
    }
}

/**
 * @brief Sorts active shop items by item identifier.
 */
static void SortShopItems(void) {
    s32 sorted_items[5];
    s32 item;
    s32 i;
    s32 j;

    for (i = 0; i < 5; i++) {
        sorted_items[i] = itemPriceTbl[activeItemTbl[i]];
    }
    for (i = 0; i < 4; i++) {
        for (j = 4; j >= i + 1; j--) {
            if (sorted_items[j] < sorted_items[j - 1]) {
                item = activeItemTbl[j];
                activeItemTbl[j] = activeItemTbl[j - 1];
                activeItemTbl[j - 1] = item;
                item = sorted_items[j];
                sorted_items[j] = sorted_items[j - 1];
                sorted_items[j - 1] = item;
            }
        }
    }
}

/**
 * @brief Chooses whether a CPU player should enter the shop.
 * @param player Player index whose CPU shop-entry decision is prepared.
 */
static void DecideComEnter(s32 player) {
    s32 space;
    s32 star_distance_via_pipe;
    s32 star_distance_without_pipe;
    s32 roll;
    u32 skip_chance;

    if (!GWPlayer[player].com) {
        return;
    }
    roll = GWPlayer[player].roll;
    switch (GWPlayer[player].diff) {
        case 0: //Easy
            skip_chance = 40;
            break;
        case 1: // Medium
            skip_chance = 20;
            break;
        case 2: // Hard
            skip_chance = 4;
            break;
        case 3: //Expert
            skip_chance = 0;
            break;
    }
    if (GWBoardGet() == BOARD_ID_EXTRA1 || GWBoardGet() == BOARD_ID_EXTRA2) {
        if (BoardPlayerCoinsGet(player) < 15) {
            BoardComKeySetRight(); // ie select NO
            return;
        }
    } else {
        space = GWPlayer[player].space_curr;
        star_distance_via_pipe = BoardComPathShortcutLenGet(space, 8, 0);
        star_distance_without_pipe = BoardComPathShortcutLenGet(space, 8, 1);
        if ((star_distance_via_pipe != 0 || star_distance_without_pipe != 0)
            && (BoardPlayerCoinsGet(player) >= 17 || (star_distance_without_pipe >= roll && star_distance_via_pipe >= roll))
            && BoardPlayerCoinsGet(player) < 40
            && ((star_distance_via_pipe < 20 && star_distance_via_pipe > 0) || (star_distance_without_pipe < 10 && star_distance_without_pipe > 0))
            && BoardRandMod(100) > skip_chance) {
            BoardComKeySetRight();
            return;
        }
    }
    BoardComKeySetLeft();
}

/**
 * @brief Chooses whether a CPU player should buy an affordable shop item.
 * @param player Player index whose CPU purchase decision is prepared.
 */
static void DecideComBuy(s32 player) {
    if (!GWPlayer[player].com) {
        return;
    }
    if (comF != 0) {
        BoardComKeySetDown();
    } else {
        BoardComKeySetUp();
    }
}

// Return the index of the computer player's most desired item in the shop.
// If called for a non-computer player, returns 0
// If all items cannot be purchased (either because they cannot afford it, or because they already own it) then returns 5.
/**
 * @brief Chooses an enabled shop item for a CPU player.
 * @param player Player index used to test item affordability.
 * @return Affordable desired slot, 5 when none is desired, or 0 for a human player.
 */
static s32 GetComItemChoice(s32 player) {
    s16 _space;
    s32 num_coins;
    s32 choice = 0;
    s8 i_most_desired;
    s8 max_desire;
    s8 desire_for_item;
    s32 i;

    if (!GWPlayer[player].com) {
        return 0;
    }
    _space = GWPlayer[player].space_curr;
    num_coins = BoardPlayerCoinsGet(player);
    for (max_desire = i = 0; i < 5; i++) {
        if (num_coins >= itemPriceTbl[activeItemTbl[i]]) {
            desire_for_item = BoardComItemWeightGet(player, activeItemTbl[i]);
        } else {
            desire_for_item = 0;
        }
        if (BoardPlayerItemFind(player, activeItemTbl[i]) != -1) { // If the player has the item already
            desire_for_item = 0;
        }
        if (desire_for_item > max_desire) {
            max_desire = desire_for_item;
            i_most_desired = i;
        }
    }
    if (max_desire == 0) {
        comF = 1;
        choice = 5;
    } else {
        choice = i_most_desired;
    }
    return choice;
}

/**
 * @brief Waits for the item-choice object to report a selection.
 */
static void WaitItemChoice(void) {
    while (itemChoiceObj) {
        HuPrcVSleep();
    }
}

/**
 * @brief Creates the rotating shop item selector for a player.
 * @param player Player index controlling the selector.
 * @param initial_slot Initial carousel slot index.
 */
static void CreateShopItemChoice(s32 player, s32 initial_slot) {
    OMOBJ *choice_object;
    ItemChoiceWork *choice_work;
    s16 cursor_sprite;
    s16 i;
    #if VERSION_PAL
    for(i=0; i<6; i++) {
        cursorPosTbl[i][0] = 190;
    }
    #endif
    choice_object = omAddObjEx(boardObjMan, 0x7E01, 0, 0, -1, UpdateShopItemChoice);
    itemChoiceObj = choice_object;
    itemChoice = -1;
    choice_work = omObjGetWork(choice_object, ItemChoiceWork);
    choice_work->kill = 0;
    choice_work->player = player;
    choice_work->input_delay = 0xA;
    choice_work->close_delay = 0;
    choice_work->slot = 0;
    choice_work->target_slot = initial_slot;
    choice_work->sprite_group = HuSprGrpCreate(1);
    #if VERSION_PAL
    if (GWLanguageGet() != 0) {
        s16 window_id = BoardWinIDGet();
        if(window_id != -1) {
            WindowData *window = &winData[window_id];
            for(i=0; i<6; i++) {
                cursorPosTbl[i][0] = window->pos_x+96;
            }
        } else {
            for(i=0; i<6; i++) {
                cursorPosTbl[i][0] = 166;
            }
        }
    }
    choice_object->trans.x = cursorPosTbl[0][0];
    choice_object->trans.y = cursorPosTbl[0][1];
    #else
    choice_object->trans.x = cursorPosTbl[0][0];
    choice_object->trans.y = cursorPosTbl[0][1];
    if (GWLanguageGet() != 0) {
        choice_object->trans.x -= 24.0f;
    }
    #endif
    if (GWPlayer[player].com) {
        choice_work->input_delay = GWMessDelayGet();
    }
    BoardSpriteCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x55), 0x3DE, NULL, &cursor_sprite);
    HuSprGrpMemberSet(choice_work->sprite_group, 0, cursor_sprite);
    HuSprAttrSet(choice_work->sprite_group, 0, 8);
    HuSprPosSet(choice_work->sprite_group, 0, choice_object->trans.x, choice_object->trans.y);
}

/**
 * @brief Gets the selected carousel slot from the shop selector.
 * @return Selected slot index, -1 before selection, or 0x7F after cancellation.
 */
static s32 GetShopItemChoice(void) {
    return itemChoice;
}

/**
 * @brief Gets the selected selector slot, including the cancel slot.
 * @return Selected carousel slot index, including the cancel slot.
 */
static s32 GetShopItemWinChoice(void) {
    ItemChoiceWork *choice_work;

    if (!itemChoiceObj) {
        return -1;
    }
    choice_work = omObjGetWork(itemChoiceObj, ItemChoiceWork);
    return choice_work->slot;
}

/**
 * @brief Rotates the shop item carousel toward the selected slot.
 * @param selector_object Selector object whose angle is updated.
 * @param choice_work Mutable selector state and target angle.
 */
static void MoveShopItemChoice(OMOBJ *selector_object, ItemChoiceWork *choice_work) {
    u32 input;
    s32 previous_slot;
    s32 controller_port;

    previous_slot = choice_work->slot;
    selector_object->trans.x = cursorPosTbl[choice_work->slot][0];
    selector_object->trans.y = cursorPosTbl[choice_work->slot][1];
    #if VERSION_NTSC
    if (GWLanguageGet() != 0) {
        selector_object->trans.x -= 24.0f;
    }
    #endif
    if (GWPlayer[choice_work->player].com) {
        GetShopItemChoiceInput(choice_work, selector_object, &input);
    } else {
        controller_port = GWPlayer[choice_work->player].port;
        input = HuPadDStkRep[controller_port] | HuPadBtnDown[controller_port];
    }
    if (input == 0x100) {
        if (choiceEnableTbl[choice_work->slot] == 0) {
            HuAudFXPlay(4);
            return;
        }
        if (choice_work->slot != 5) {
            itemChoice = choice_work->slot;
        }
        HuAudFXPlay(2);
        if (GWPlayer[choice_work->player].com) {
            choice_work->close_delay = 0x32;
        } else {
            choice_work->close_delay = 5;
        }
        return;
    }
    if (input == 0x200) {
        itemChoice = 0x7F;
        choice_work->close_delay = 5;
        HuAudFXPlay(3);
    }
    if (input == 4) {
        choice_work->slot++;
    }
    if (input == 8) {
        choice_work->slot--;
    }
    if (choice_work->slot < 0) {
        choice_work->slot = 0;
    }
    if (choice_work->slot >= 6) {
        choice_work->slot = 5;
    }
    if (choice_work->slot != previous_slot) {
        HuAudFXPlay(0);
        choice_work->moving = 4;
    }
}

/**
 * @brief Processes shop selector movement and controller or CPU input.
 * @param selector_object Object controlling the shop item carousel.
 */
static void UpdateShopItemChoice(OMOBJ *selector_object) {
    ItemChoiceWork *choice_work;

    choice_work = omObjGetWork(selector_object, ItemChoiceWork);
    if (choice_work->kill != 0 || BoardIsKill()) {
        HuSprGrpKill(choice_work->sprite_group);
        itemChoiceObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), selector_object);
        return;
    }
    if (choice_work->close_delay != 0) {
        choice_work->close_delay--;
        if (choice_work->close_delay == 0) {
            choice_work->kill = 1;
        }
    } else if (choice_work->input_delay != 0) {
        choice_work->input_delay--;
    } else if (choice_work->moving != 0) {
        choice_work->moving--;
    } else {
        MoveShopItemChoice(selector_object, choice_work);
    }
    HuSprPosSet(choice_work->sprite_group, 0, selector_object->trans.x, selector_object->trans.y);
}

/**
 * @brief Converts shop selection input into a target carousel angle.
 * @param choice_work Mutable selector state containing the player and current slot.
 * @param selector_object Selector object retained for the helper signature; not read by this helper.
 * @param button_down Output pointer receiving a synthetic confirm or downward-navigation button mask.
 */
static void GetShopItemChoiceInput(ItemChoiceWork *choice_work, OMOBJ *selector_object, u32 *button_down) {
    if (choice_work->slot == choice_work->target_slot) {
        *button_down = 0x100;
    } else {
        *button_down = 4;
    }
}

/**
 * @brief Runs a scripted shop interaction for board tutorial mode.
 * @param shop_space Board-space index containing the tutorial shop entrance.
 */
void BoardShopTutorialExec(s32 shop_space) {
    Vec shop_position;
    Vec entrance_position;
    Vec entrance_direction;
    Vec camera_rotation;
    Vec camera_offset;
    s32 board;
    s32 portrait;
    s16 host_model;
    float entrance_angle;
    s16 entrance_space;
    s32 i;

    board = GWBoardGet();
    portrait = BoardWinPortraitGet();
    if (BoardSpaceFlagGet(0, shop_space) & 0x80000) {
        shopMdlPtr = shopMdlIdx[0];
    } else {
        shopMdlPtr = shopMdlIdx[1];
    }
    entrance_space = BoardSpaceLinkFlagSearch(0, shop_space, 0x02000000);
    BoardSpacePosGet(0, shop_space, &shop_position);
    BoardSpacePosGet(0, entrance_space, &entrance_position);
    VECSubtract(&entrance_position, &shop_position, &entrance_direction);
    VECNormalize(&entrance_direction, &entrance_direction);
    entrance_angle = atan2d(-entrance_direction.x, -entrance_direction.z);
    camera_rotation.x = -10.0f;
    camera_rotation.y = entrance_angle;
    camera_rotation.z = 0.0f;
    camera_offset.x = 0.0f;
    camera_offset.y = 170.0f;
    camera_offset.z = 0.0f;
    BoardCameraMotionStartEx(shopMdlPtr[0], &camera_rotation, &camera_offset, 1360.0f, -1.0f, 21);
    HuAudFXPlay(0x330);
    HuAudFXPlay(0x32D);
    PopupShop();
    BoardModelMotionShiftSet(BoardShopHostGet(), shopMot[2], 0.0f, 4.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(4);
    while (!BoardModelMotionEndCheck(BoardShopHostGet())) {
        HuPrcVSleep();
    }
    BoardModelMotionStart(BoardShopHostGet(), shopMot[0], 0x40000001);
    BoardTutorialHookExec(0x1C, 0);
    BoardModelMotionShiftSet(BoardShopHostGet(), shopMot[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    HuPrcSleep(30);
    CloseShop();
    for (i = 0; i < 2; i++) {
        BoardModelHookReset(shopMdlIdx[0][i]);
    }
    host_model = BoardShopHostGet();
    for (i = 0; i < 3; i++) {
        if (shopMot[i] != -1) {
            BoardModelMotionKill(host_model, shopMot[i]);
            shopMot[i] = -1;
        }
    }
    PauseShopWin();
    if (itemMdl != -1) {
        BoardModelKill(itemMdl);
        itemMdl = -1;
    }
}
