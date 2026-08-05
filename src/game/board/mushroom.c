#include "game/process.h"
#include "game/gamework_data.h"
#include "game/board/item.h"
#include "game/object.h"
#include "game/board/main.h"
#include "game/board/player.h"
#include "game/board/ui.h"
#include "game/board/model.h"

#include "game/objsub.h"

#include "ext_math.h"
#include "math.h"

#include "game/audio.h"
#include "game/board/tutorial.h"
#include "game/board/window.h"
#include "game/hsfex.h"

extern s32 boardTutorialData[4];


static void KillMushroom();
static void MushroomMain();
static void KillItemMdl(void);
static void MarkKillBox(void);
static void InitBox(s32 arg0);

static const float boxOfs[2] = {-150.0f, 150.0f};
static OMOBJ *boxObj[2] = {};
static s16 itemMdl = -1;
static s8 itemResult;
static u8 pickerChoice;
static OMOBJ* itemGiveObj;
static HUPROCESS* mushroomProc;

/**
 * @brief Runs and waits for a player's mushroom-space item-box event.
 * @param arg0 Player index landing on the mushroom space.
 */
void BoardMushroomExec(s32 arg0) {
    omVibrate(arg0, 0xC, 6, 6);
    mushroomProc = HuPrcChildCreate(MushroomMain, 0x2003U, 0x3800U, 0, boardMainProc);
    HuPrcDestructorSet2(mushroomProc, KillMushroom);
    
    while (mushroomProc != NULL) {
        HuPrcVSleep();
    }
    
    GWPlayer[arg0].color = 3;
}


typedef struct {
    struct {
        u8 kill : 1;
        u8 box_index : 1;
        u8 state : 3;
        u8 player : 2;
        u8 ready : 1;
    };
    char padding[1];
    s16 box_model;
    s16 alpha;
    s16 rotation;
    f32 arc_angle;
} boxObjWork;

static void SetBoxHide(s32 arg0);
static void SetBoxCenter(s32 arg0);
static void SetBoxShow(s32 arg0);
static void WaitBoxOpen(s32 arg0);
static void WaitBoxCenter(s32 arg0);
static s32 CheckBox(void);
static void StartItemGive(s32 arg0);
static void ShowItemMdl(void);
static void WaitItemGive(void);

/**
 * @brief Presents two boxes, accepts a choice, and awards the revealed mushroom item.
 */
static void MushroomMain(void) {
    Vec picker_positions[2];
    Vec box_position;
    s32 _unused;
    s32 _unused_alt;
    s32 _unused_aux;
    s32 current_player;
    s8 cpu_choice;
    boxObjWork* box;
    s32 message;
    s32 default_choice;
    s32 i;

    cpu_choice = -1;
    current_player = GWSystem.player_curr;
    BoardCameraViewSet(3);
    HuAudFXPlay(0x34A);
    BoardPlayerMotBlendSet(current_player, 0, 15);
    
    while (BoardPlayerMotBlendCheck(current_player) == 0) {
        HuPrcVSleep();
    }
    
    BoardCameraMotionWait();
    BoardPlayerIdleSet(current_player);
    
    if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
        BoardTutorialHookExec(13, 0);
        boardTutorialData[0] = 0;
    }
    
    if ((GWSystem.max_turn - GWSystem.turn) == 0) {
        BoardWinCreate(0, MAKE_MESSID(5, 3), -1);
        BoardWinWait();
        BoardWinKill();
    } else if (BoardPlayerItemCount(current_player) == 3) {
        BoardWinCreate(0, MAKE_MESSID(5, 0), -1);
        BoardWinWait();
        BoardWinKill();
    } else {
        while (1) {
            InitBox(current_player);
            
            while (CheckBox() == 0) {
                HuPrcVSleep();
            }
    
            for (i = 0; i < 2; i++) {
                box = omObjGetWork(boxObj[i], boxObjWork);
                BoardModelPosGet(box->box_model, &box_position);
                Hu3D3Dto2D(&box_position, 1, &picker_positions[i]);
                picker_positions[i].y += 32.0f;
                picker_positions[i].z = 0.0f;
            }
            
            if (GWPlayer[current_player].com != 0) {
                BoardRand();
                default_choice = 0; //inline?
                cpu_choice = default_choice;
                if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
                    cpu_choice = 0;
                }
            }
            
            BoardPickerCreate(current_player, 2, &picker_positions[0], cpu_choice);
            BoardPickerBackFlagSet(0);
            
            while (BoardPickerDoneCheck() == 0) {
                HuPrcVSleep();
            }
            
            pickerChoice = BoardPickerChoiceGet();
            SetBoxShow(pickerChoice);
            SetBoxHide(pickerChoice ^ 1);
            WaitBoxCenter(pickerChoice);
            BoardPlayerMotionStart(current_player, 11, 0);
            
            while (BoardPlayerMotionTimeGet(current_player) < 27.0f) {
                HuPrcVSleep();
            }
            
            HuAudFXPlay(0x30C);
            StartItemGive(current_player);
            WaitBoxOpen(pickerChoice);
            ShowItemMdl();
            SetBoxCenter(pickerChoice);
            
            while (BoardPlayerMotionEndCheck(current_player) == 0) {
                HuPrcVSleep();
            }
            
            WaitItemGive();
            
            if (itemResult != 0) {
                message = MAKE_MESSID(5, 1);
            } else {
                message = MAKE_MESSID(5, 2);
            }
            
            BoardPlayerMotionShiftSet(current_player, 0xC, 0.0f, 4.0f, HU3D_MOTATTR_NONE);
            HuPrcSleep(4);
            BoardWinCreate(0, message, -1);
            BoardWinWait();
            BoardWinKill();
            MarkKillBox();
            BoardPlayerMotionEndWait(current_player);
            BoardPlayerIdleSet(current_player);
            if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
                BoardPlayerIdleSet(current_player);
                BoardTutorialHookExec(14, 0);
                if (++boardTutorialData[0] < 2) {
                    KillItemMdl();
                    continue;
                }
            }
            break;
        }
    }
    HuPrcEnd();
}

typedef struct {
    struct {
        u8 kill : 1;
        u8 box_index : 1;
        u8 state : 3;
        u8 player : 2;
        u8 ready : 1;
    };
    char padding[1];
    s16 box_model;
    s16 alpha;
    s16 rotation;
    f32 arc_angle;
} ItemGiveWork2;

/**
 * @brief Marks mushroom-event objects for destruction and releases the item model.
 */
static void KillMushroom(void) {
    ItemGiveWork2* item_give_work;

    MarkKillBox();
    
    if (itemGiveObj) {
        item_give_work = omObjGetWork(itemGiveObj, ItemGiveWork2);
        item_give_work->kill = 1;
    }
    
    KillItemMdl();
    mushroomProc = NULL;
}

typedef struct {
    struct {
        u8 kill : 1;
        u8 player : 2;
        u8 state : 5;
    };
    char padding[1];
    s16 frame;
    f32 vertical_velocity;
    f32 scale;
} itemObjWork;

static void CenterBox(ItemGiveWork2*, OMOBJ*);
static void ExitBox(ItemGiveWork2* arg0, OMOBJ* arg1);
static void HideBox(ItemGiveWork2*, OMOBJ*);
static void ShowBox(ItemGiveWork2*, OMOBJ*);


/**
 * @brief Updates one mushroom-event box according to its animation state.
 * @param arg0 Object controlling one item box.
 */
static void BoxMain(OMOBJ* arg0) {
    s32 unused;
    ItemGiveWork2* box_work;

    box_work = omObjGetWork(arg0, ItemGiveWork2);

    if ((box_work->kill) || (BoardIsKill() != 0)) {
        BoardModelKill(box_work->box_model);
        boxObj[box_work->box_index] = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    switch (box_work->state) {
    case 0:
        ShowBox(box_work, arg0);
        break;
    case 2:
        CenterBox(box_work, arg0);
        break;
    case 3:
        ExitBox(box_work, arg0);
        break;
    case 4:
        HideBox(box_work, arg0);
        break;
    }
    BoardModelPosSet(box_work->box_model, arg0->trans.x, arg0->trans.y, arg0->trans.z);
    BoardModelRotSet(box_work->box_model, arg0->rot.x, arg0->rot.y, arg0->rot.z);
}

/**
 * @brief Drops and spins a box into its selectable position.
 * @param arg0 Mutable box animation state.
 * @param arg1 Box object whose transform is updated.
 */
static void ShowBox(ItemGiveWork2* arg0, OMOBJ* arg1) {
    Vec player_position;

    BoardPlayerPosGet(arg0->player, &player_position);
    player_position.y += 300.0f;
    
    if (arg0->arc_angle >= 90.0f) {
        arg0->arc_angle = 90.0f;
        arg0->ready = 1;
        arg0->state = 1;
    }
    
    OSs16tof32(&arg0->rotation, &arg1->rot.y);
    arg1->trans.y = (player_position.y + (700.0f * cosd(arg0->arc_angle)));
    arg0->arc_angle += 1.5f;
    arg0->rotation += 30;
}


/**
 * @brief Raises and spins the unselected box out of view.
 * @param arg0 Mutable box animation state.
 * @param arg1 Box object whose transform is updated.
 */
static void ExitBox(ItemGiveWork2* arg0, OMOBJ* arg1) {
    Vec player_position;
    s16 wrapped_rotation;
    f32 rotation_degrees;

    wrapped_rotation = (arg0->rotation % 360);
    OSs16tof32(&wrapped_rotation, &rotation_degrees);
    BoardPlayerPosGet(arg0->player, &player_position);
    player_position.y += 300.0f;

    if (arg0->arc_angle <= 0.0f) {
        arg0->arc_angle = 0.0f;
        arg0->state = 1;
    }

    arg1->trans.y = player_position.y + (700.0f * cosd(arg0->arc_angle));
    arg1->rot.y = sind(rotation_degrees);
    arg0->arc_angle -= 1.5f;
    arg0->rotation -= 30;
}

/**
 * @brief Fades out the selected box after it opens.
 * @param arg0 Mutable box animation state.
 * @param arg1 Box object retained for the common state-handler signature.
 */
static void HideBox(ItemGiveWork2* arg0, OMOBJ* arg1) {
    arg0->alpha -= 4;

    if (arg0->alpha < 0) {
        arg0->state = 1;
        arg0->alpha = 0;
    }

    BoardModelAlphaSet(arg0->box_model, arg0->alpha);
}

/**
 * @brief Moves the selected box horizontally to the player.
 * @param arg0 Mutable box animation state.
 * @param arg1 Box object moved toward the player.
 */
static void CenterBox(ItemGiveWork2* arg0, OMOBJ* arg1) {
    Vec player_position;
    Vec box_position;
    Vec offset;

    box_position.x = arg1->trans.x;
    box_position.y = arg1->trans.y;
    box_position.z = arg1->trans.z;
    BoardPlayerPosGet(arg0->player, &player_position);
    VECSubtract(&player_position, &box_position, &offset);
    VECScale(&offset, &offset, 0.2f);
    VECAdd(&offset, &box_position, &box_position);

    if (BoardVecMaxDistXZCheck(&player_position, &box_position, 3.0f) != 0) {
        arg1->trans.x = player_position.x;
        arg1->trans.z = player_position.z;
        arg0->state = 1;
    }
    
    arg1->trans.x = box_position.x;
    arg1->trans.z = box_position.z;
}

/**
 * @brief Creates both boxes and randomly assigns the item and enlarged box.
 * @param arg0 Player index used to position and own both boxes.
 */
static void InitBox(s32 arg0) {
    Vec player_position;
    s32 item_model_resources[2] = {DATA_MAKE_NUM(DATADIR_BOARD, 0x6D), DATA_MAKE_NUM(DATADIR_BOARD, 0x6E)};
    s32 enlarged_box;
    s32 item_result;
    s32 random_box;
    s32 i;
    boxObjWork* box_work;

    BoardPlayerPosGet(arg0, &player_position);

    for (i = 0; i < 2; i++) {
        boxObj[i] = omAddObjEx(boardObjMan, 0x101, 0U, 0U, -1, BoxMain);
        box_work = omObjGetWork(boxObj[i], boxObjWork);
        box_work->kill = 0;
        box_work->ready = 0;
        box_work->player = arg0;
        box_work->box_index = i;
        box_work->state = 0;
        box_work->rotation = 0;
        box_work->arc_angle = 0.0f;
        box_work->alpha = 0xFF;
        boxObj[i]->trans.x = player_position.x + boxOfs[i];
        boxObj[i]->trans.y = 700.0f + player_position.y;
        boxObj[i]->trans.z = player_position.z;
        boxObj[i]->rot.x = 0.0f;
        boxObj[i]->rot.y = 180.0f;
        boxObj[i]->rot.z = 0.0f;
        box_work->box_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 5), NULL, 0);
        BoardModelPosSet(box_work->box_model, boxObj[i]->trans.x, boxObj[i]->trans.y, boxObj[i]->trans.z);
        BoardModelMotionSpeedSet(box_work->box_model, 0.0f);
        BoardModelLayerSet(box_work->box_model, 2U);
    }
    
    HuAudFXPlay(0x340);
    if (BoardRandFloat() < 0.5f) {
        item_result = 0;
    } else {
        item_result = 1;
    }
    itemResult = (s8) item_result;
    if (BoardRandFloat() < 0.5f) {
        random_box = 0;
    } else {
        random_box = 1;
    }
    enlarged_box = random_box;
    if (GWBoardGet() == BOARD_ID_EXTRA2) {
        itemResult = 0;
    }
    if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
        itemResult = (boardTutorialData[0] ^ 1);
        enlarged_box = 0;
    }
    itemMdl = BoardModelCreate(item_model_resources[itemResult], NULL, 0);
    BoardModelVisibilitySet(itemMdl, 0);
    box_work = omObjGetWork(boxObj[enlarged_box], boxObjWork);
    BoardModelScaleSet(box_work->box_model, 1.5f, 1.5f, 1.5f);
}

/**
 * @brief Marks both mushroom-event box objects for destruction.
 */
static void MarkKillBox(void) {
    s32 i;
    boxObjWork* box_work;

    for (i = 0; i < 2; i++) {
        if (boxObj[i]) {
            box_work = omObjGetWork(boxObj[i], boxObjWork);
            box_work->kill = 1;
        }
    }
}

/**
 * @brief Starts a box's upward exit animation.
 * @param arg0 Index of the unselected box that should leave the scene.
 */
static void SetBoxHide(s32 arg0) {
    boxObjWork* box_work = omObjGetWork(boxObj[arg0], boxObjWork);

    box_work->state = 3;
}

/**
 * @brief Starts fading the opened box and plays its close sound.
 * @param arg0 Index of the opened box to fade out.
 */
static void SetBoxCenter(s32 arg0) {
    boxObjWork* box_work = omObjGetWork(boxObj[arg0], boxObjWork);
    
    box_work->state = 4;
    HuAudFXPlay(0x341);
}

/**
 * @brief Starts moving the selected box to the player.
 * @param arg0 Index of the box to move toward the player.
 */
static void SetBoxShow(s32 arg0) {
    boxObjWork* box_work = omObjGetWork(boxObj[arg0], boxObjWork);

    box_work->state = 2;
}

/**
 * @brief Starts the box-opening motion and waits for it to finish.
 * @param arg0 Index of the box whose opening motion is awaited.
 */
static void WaitBoxOpen(s32 arg0) {
    boxObjWork* box_work = omObjGetWork(boxObj[arg0], boxObjWork);
    
    BoardModelMotionSpeedSet(box_work->box_model, 2.0f);
    do {
        HuPrcVSleep();
    } while (BoardModelMotionEndCheck(box_work->box_model) == 0);
}

/**
 * @brief Waits until a box finishes its current movement.
 * @param arg0 Index of the box whose movement is awaited.
 */
static void WaitBoxCenter(s32 arg0) {
    boxObjWork* box_work = omObjGetWork(boxObj[arg0], boxObjWork);

    while (box_work->state != 1) {
        HuPrcVSleep();
    }
}

/**
 * @brief Checks whether both boxes have reached their initial positions.
 * @return Nonzero when both boxes are ready for selection; otherwise zero.
 */
static s32 CheckBox(void) {
    s32 i;
    boxObjWork* box;
    
    for (i = 0; i < 2; i++) {
        box = omObjGetWork(boxObj[i], boxObjWork);
        if (box->ready == 0) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Animates the revealed item and adds it to the player when it descends.
 * @param arg0 Object controlling the revealed mushroom item.
 */
static void DoItemGive(OMOBJ* arg0) {
    Vec item_position;
    Vec item_rotation;
    Vec player_position;
    s32 item_ids[2] = {0, 1};
    itemObjWork* item_work;

    item_work = omObjGetWork(arg0, itemObjWork);
    
    if ((item_work->kill != 0) || (BoardIsKill() != 0)) {
        KillItemMdl();
        itemGiveObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    
    BoardModelPosGet(itemMdl, &item_position);
    BoardModelRotGet(itemMdl, &item_rotation);
    BoardPlayerPosGet(item_work->player, &player_position);
    player_position.y += 180.0f;
    
    switch (item_work->state) {
    case 2:
        if (item_work->frame < 12) {
            item_work->vertical_velocity += 1.47f;
            item_position.y += item_work->vertical_velocity;
        } else if (item_work->frame <= 24) {
            BoardModelRotSet(itemMdl, 30.0f * item_work->frame, 0.0f, 0.0f);
        } else if (item_work->frame > 36) {
            item_work->state = 0;
            return;
        }
        item_work->frame++;
        break;
    case 0:
        item_work->vertical_velocity += -1.47f;
        
        if (item_work->scale > 0.001f) {
            item_work->scale -= 0.005f;
        }
        
        item_position.y += item_work->vertical_velocity;
        
        if (player_position.y > item_position.y) {
            BoardModelVisibilitySet(itemMdl, 0);
            HuAudFXPlay(0x30D);
            BoardPlayerItemAdd(item_work->player, item_ids[itemResult]);
            item_work->state = 1;
        }
        /* fallthrough */
        case 1:
        default:
            break;
    }

    BoardModelPosSetV(itemMdl, &item_position);
    BoardModelScaleSet(itemMdl, item_work->scale, item_work->scale, item_work->scale);
}

/**
 * @brief Creates the mushroom reward object at the chosen box.
 * @param arg0 Player index stored as the reward owner and eventual item recipient.
 */
static void StartItemGive(s32 arg0) {
    Vec box_position;
    itemObjWork* item_work;
    boxObjWork* box;

    itemGiveObj = omAddObjEx(boardObjMan, 0x102, 0U, 0U, -1, DoItemGive);
    item_work = omObjGetWork(itemGiveObj, itemObjWork);

    item_work->kill = 0;
    item_work->player = arg0;
    item_work->state = 1;
    item_work->frame = 0;
    item_work->vertical_velocity = 0.0f;
    item_work->scale = 1.0f;
    box = omObjGetWork(boxObj[pickerChoice], boxObjWork);
    BoardModelPosGet(box->box_model, &box_position);
    BoardModelPosSetV(itemMdl, &box_position);
}

/**
 * @brief Destroys the temporary mushroom item model if it exists.
 */
static void KillItemMdl(void) {
    if (itemMdl != -1) {
        BoardModelKill(itemMdl);
        itemMdl = -1;
    }
}

/**
 * @brief Reveals the mushroom item model and starts its upward animation.
 */
static void ShowItemMdl(void) {
    itemObjWork* item_work = omObjGetWork(itemGiveObj, itemObjWork);
    item_work->state = 2;

    BoardModelVisibilitySet(itemMdl, 1);
}

/**
 * @brief Waits until the mushroom reward object reports delivery complete.
 */
static void WaitItemGive(void) {
    itemObjWork* item_work = omObjGetWork(itemGiveObj, itemObjWork);

    while (item_work->state != 1) {
        HuPrcVSleep();
    }
}
