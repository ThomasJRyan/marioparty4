#include "game/board/roll.h"
#include "game/audio.h"
#include "game/board/audio.h"
#include "game/board/com.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/pause.h"
#include "game/board/player.h"
#include "game/board/tutorial.h"
#include "game/board/window.h"
#include "game/disp.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/hsfex.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/window.h"

#include "ext_math.h"

typedef struct {
    struct {
        u8 kill_requested : 1;
        u8 face_change_disabled : 1;
        u8 state : 3;
        u8 dice_index : 2;
    };
    s8 face_change_delay;
    s8 face_change_timer;
    char padding[1];
    s16 animation_timer;
    s16 rotation_angle;
} DiceWork;

typedef struct {
    struct {
        u8 kill_requested : 1;
        u8 move_to_player : 1;
        u8 state : 1;
        u8 dice_index : 2;
    };
    s8 value;
    s16 timer;
    s16 model_ids[2];
    float move_fraction;
} DiceDigitWork;

static void RollMain(void);
static void DiceCreate(s32 dice_index);
static void DoInput(s32 dice_index);
static void DiceWaitFull(s32 dice_index);
static void DiceSetHit(s32 dice_index);
static void RollDestroy(void);
static void DiceMain(OMOBJ *object);
static void DicePause(s32 dice_index);
static void DicePauseAll(void);
static void DiceHideEffect(s32 dice_index, s32 visible);
static void DiceDigitMain(OMOBJ *object);
static void DiceDigitKill(DiceDigitWork *work);
static void DiceKill(void);
static void DiceDigitMove(DiceDigitWork *work, s32 move_to_player);
static void DiceDigitRotate(DiceDigitWork *work, s32 move_to_player);
static void SameRollExec(void);
static s32 GetBtnRoll(s32 player);
static s32 GetComBtnRoll(s32 player);

static s32 tutorialRoll[3];

static s8 diceValue[3];
static s8 rollPlayer;
static s8 numDice;
static s8 inputTimer;
static s8 maxRoll;
static s8 destMode;
static s8 diceSize;
s32 lbl_801D4098;
static char sameRollCoinStr[8];
static s8 tutorialRollF;
static HUPROCESS *rollProc;

static s16 diceMdl[3] = { -1, -1, -1 };
static s16 diceEff[3] = { -1, -1, -1 };
static s16 rollWin = -1;

static OMOBJ *diceObj[3] = { NULL, NULL, NULL };
static OMOBJ *diceDigitObj[3] = { NULL, NULL, NULL };
static s32 diceSndStatus[3] = { -1, -1, -1 };

static HU3DPARMANPARAM diceEffParam = { 0x0096, { 0x00, 0x00 }, // padding?
    50.0f, 0.0f, 65.0f, { 0.0f, -0.195f, 0.0f }, 100.0f, 0.99f, 40.0f, 0.999f, 0x0004,
    { { 0xFF, 0xFF, 0x20, 0xFF }, { 0xFF, 0xFF, 0x00, 0xFF }, { 0xFF, 0xFF, 0x80, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF } },
    { { 0xFF, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 } } };

static s32 diceDigitMdlTbl[10] = { DATA_MAKE_NUM(DATADIR_BOARD, 0x0C), DATA_MAKE_NUM(DATADIR_BOARD, 0x0D), DATA_MAKE_NUM(DATADIR_BOARD, 0x0E),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x0F), DATA_MAKE_NUM(DATADIR_BOARD, 0x10), DATA_MAKE_NUM(DATADIR_BOARD, 0x11), DATA_MAKE_NUM(DATADIR_BOARD, 0x12),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x13), DATA_MAKE_NUM(DATADIR_BOARD, 0x14), DATA_MAKE_NUM(DATADIR_BOARD, 0x15) };

/**
 * @brief Runs one player's dice-roll interaction.
 *
 * @details Chooses the dice count and size from the active item and player
 * state, launches the roll process, and waits for both the process and dice
 * objects to finish.
 *
 * @param player Zero-based player index taking the turn.
 * @return A positive movement distance, or a negative action code selected by
 * input: -2 for item selection, -3 for the map, or -4 for the overhead view.
 */
s32 BoardRollExec(s32 player)
{
    rollPlayer = player;
    inputTimer = 0;
    numDice = 1;
    diceSize = 0;
    maxRoll = 0xA;
    switch (BoardItemPrevGet()) {
        case 0:
            numDice = 1;
            break;
        case 2:
            numDice = 2;
            break;
        case 1:
            numDice = 2;
            break;
        case 3:
            numDice = 3;
            break;
    }
    if (BoardPlayerSizeGet(player) == 1) {
        diceSize = 1;
        maxRoll = 5;
    }
    else if (BoardPlayerSizeGet(player) == 2) {
        diceSize = 2;
        maxRoll = 10;
        if (BoardMegaDoubleDiceCheck() != 0) {
            numDice = 2;
        }
    }
    if (GWPlayer[rollPlayer].bowser_suit != 0) {
        diceSize = 3;
    }
    GWPlayer[player].num_dice = numDice;
    rollProc = HuPrcChildCreate(RollMain, 0x2005, 0x6000, 0, boardMainProc);
    HuPrcDestructorSet2(rollProc, RollDestroy);
    while (rollProc != NULL) {
        HuPrcVSleep();
    }
    while (diceObj[0] || diceObj[1] || diceObj[2]) {
        HuPrcVSleep();
    }
    return destMode;
}

/**
 * @brief Creates the roll prompt window when tutorial mode is inactive.
 *
 * @details Selects a prompt based on item availability and prior input, then
 * positions it according to the current language's layout.
 *
 * @param player Zero-based player index whose usable items affect the prompt.
 */
static void RollWinCreate(s32 player)
{
    float message_size[2];
    float window_x;
    float window_y;
    s32 message_id;
    s32 item_count;
    s32 teammate;
    
    if (!_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        item_count = BoardPlayerItemCount(player);
        if (GWTeamGet()) {
            teammate = BoardPlayerSameTeamFind(player);
            item_count += BoardPlayerItemCount(teammate);
        }
        if (BoardItemPrevGet() != -1 || item_count == 0 || _CheckFlag(FLAG_ID_MAKE(1, 9)) || BoardMegaDoubleDiceCheck()) {
            message_id = MAKE_MESSID(0x09, 0x04);
        }
        else {
            message_id = MAKE_MESSID(0x09, 0x02);
        }
        if (inputTimer != 0) {
            message_id = MAKE_MESSID(0x30, 0x25);
        }
        HuWinMesMaxSizeGet(1, message_size, message_id);
        #if VERSION_PAL
        window_y = HU_DISP_HEIGHT - 176;
        switch (GWGameStat.language) {
            case 0:
            case 4:
                window_x = -10000.0f;
                break;
            
            case 3:
                window_x = HU_DISP_CENTERX - (message_size[0] / 2 - 32.0f);
                break;
            
            case 5:
                window_x = HU_DISP_CENTERX - (message_size[0] / 2 - 16.0f);
                window_y = 258;
                break;
            
            default:
                window_x = HU_DISP_CENTERX - (message_size[0] / 2 - 16.0f);
                break;
        }
        #else
        switch (GWGameStat.language) {
            case 0:
                window_x = -10000.0f;
                break;
            case 1:
                window_x = HU_DISP_CENTERX - (message_size[0] / 2 - 16.0f);
                break;
        }
        window_y = HU_DISP_HEIGHT - 176;
        #endif
        
        rollWin = HuWinCreate(window_x, window_y, message_size[0], message_size[1], 0);
        HuWinBGTPLvlSet(rollWin, 0.0f);
        HuWinMesSpeedSet(rollWin, 0);
        HuWinMesSet(rollWin, message_id);
    }
}

/**
 * @brief Stops the active roll process and requests destruction of its dice.
 */
void BoardRollKill(void)
{
    if (rollProc) {
        HuPrcKill(rollProc);
    }
    DicePauseAll();
    DiceKill();
}

/**
 * @brief Changes roll UI visibility while the board pause screen is active.
 *
 * @param visible Nonzero to show the prompt, value digits, and particle
 * effects; zero to hide them. The request is ignored when no roll is active.
 */
void BoardRollUPauseSet(s32 visible)
{
    if (!rollProc) {
        return;
    }
    BoardRollWinDispSet(visible);
    BoardDiceVisibleSet(0, visible);
    BoardDiceVisibleSet(1, visible);
    BoardDiceVisibleSet(2, visible);
    DiceHideEffect(0, visible);
    DiceHideEffect(1, visible);
    DiceHideEffect(2, visible);
}

/**
 * @brief Shows or hides the current roll prompt window.
 *
 * @param visible Nonzero to show the window or zero to hide it. No action is
 * taken when the window has not been created.
 */
void BoardRollWinDispSet(s32 visible)
{
    if (rollWin == -1) {
        return;
    }
    if (visible != 0) {
        HuWinDispOn(rollWin);
    }
    else {
        HuWinDispOff(rollWin);
    }
}

/**
 * @brief Drives dice creation, input, hits, and roll-total presentation.
 *
 * @details Processes each die in sequence, awards a same-roll coin bonus when
 * applicable, moves the resulting digits into the HUD, stores the final roll
 * on the player, and invokes the tutorial hook.
 */
static void RollMain(void)
{
    
    s32 same_roll;
    
    s32 roll_total;
    
    s32 i;

    diceValue[0] = diceValue[1] = diceValue[2] = 0;
    if (BoardPlayerSizeGet(rollPlayer) == 2 || GWPlayer[rollPlayer].bowser_suit != 0) {
        BoardCameraViewSet(2);
    }
    BoardCameraMotionWait();
    BoardCameraTargetModelSet(-1);
    for (i = 0; i < numDice; i++) {
        RollWinCreate(rollPlayer);
        DiceCreate(i);
        DiceWaitFull(i);
        DoInput(i);
        if (rollWin != -1) {
            HuWinKill(rollWin);
            rollWin = -1;
        }
        if (destMode == -2 || destMode == -1 || destMode == -3 || destMode == -4) {
            HuPrcEnd();
        }
        BoardPlayerDiceJumpStart(rollPlayer);
        while (!BoardPlayerDiceJumpCheck(rollPlayer)) {
            HuPrcVSleep();
        }
        DiceSetHit(i);
        while (GWPlayer[rollPlayer].jump != 0) {
            HuPrcVSleep();
        }
    }
    for (roll_total = i = 0; i < numDice; i++) {
        roll_total += diceValue[i];
    }
    same_roll = 0;
    switch (numDice) {
        case 2:
            if (diceValue[0] == diceValue[1]) {
                same_roll = 1;
            }
            break;
        case 3:
            if (diceValue[0] == diceValue[1] && diceValue[0] == diceValue[2]) {
                same_roll = 1;
            }
            break;
    }
    if (same_roll != 0) {
        SameRollExec();
    }
    BoardDicePauseAll();
    while (!BoardDiceDoneCheck()) {
        HuPrcVSleep();
    }
    for (i = 0; i < numDice; i++) {
        BoardDiceStop(i);
    }
    destMode = roll_total;
    GWPlayer[rollPlayer].roll = roll_total;
    BoardRollCreate(rollPlayer, 1);
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        BoardTutorialHookExec(6, 0);
    }
    if (tutorialRollF != 0) {
        tutorialRollF = 0;
    }
    HuPrcEnd();
}

/**
 * @brief Creates and positions one animated dice block.
 *
 * @param dice_index Zero-based index within the current roll's one to three dice.
 */
static void DiceCreate(s32 dice_index)
{
    Vec position;
    float initial_face;
    float height_offset;
    OMOBJ *object;
    DiceWork *work;
    s32 unused_value;
    s32 dice_model_resources[4] = { DATA_MAKE_NUM(DATADIR_BOARD, 0x18), DATA_MAKE_NUM(DATADIR_BOARD, 0x19), DATA_MAKE_NUM(DATADIR_BOARD, 0x1A),
        DATA_MAKE_NUM(DATADIR_BOARD, 0x1B) };

    unused_value = 0;
    diceMdl[dice_index] = BoardModelCreate(dice_model_resources[diceSize], NULL, 0);
    initial_face = BoardRandMod(maxRoll) + 0.5f;
    BoardPlayerPosGet(rollPlayer, &position);
    HuAudFXPlay(0x302);
    BoardModelMotionStart(diceMdl[dice_index], 0, 0);
    BoardModelMotionSpeedSet(diceMdl[dice_index], 0.0f);
    BoardModelMotionTimeSet(diceMdl[dice_index], initial_face);
    BoardModelScaleSet(diceMdl[dice_index], 0.001f, 0.001f, 0.001f);
    object = omAddObjEx(boardObjMan, 0x102, 0, 0, -1, DiceMain);
    work = omObjGetWork(object, DiceWork);
    work->kill_requested = 0;
    work->face_change_disabled = 0;
    work->state = 0;
    work->dice_index = dice_index;
    work->animation_timer = 0;
    work->face_change_delay = 4;
    work->face_change_timer = 0;
    diceObj[dice_index] = object;
    height_offset = 250.0f;
    if (diceSize == 2) {
        height_offset = 625.0f;
    }
    else if (GWPlayer[rollPlayer].bowser_suit != 0) {
        height_offset = 575.0f;
    }
    position.y += height_offset;
    object->trans.x = position.x;
    object->trans.y = position.y;
    object->trans.z = position.z;
    object->scale.x = 0.1f;
    object->scale.y = 0.1f;
    object->scale.z = 0.1f;
    position.z += 70.0f;
}

/**
 * @brief Waits for the player or CPU to choose a roll action.
 *
 * @details A roll press advances the dice hit sequence. Before the first die,
 * item selection and the two board-view actions set negative destination codes.
 *
 * @param dice_index Index of the die awaiting input; retained for the original
 * call interface but not read by this routine.
 */
static void DoInput(s32 dice_index)
{
    s32 item_count;
    s32 teammate;
    u16 buttons[1];

    destMode = 0;
    lbl_801D4098 = 0;
    while (1) {
        HuPrcVSleep();
        *buttons = GetBtnRoll(rollPlayer);
        if (BoardPauseActiveCheck()) {
            *buttons = 0;
            continue;
        }
        if (*buttons == 0x100) {
            inputTimer++;
            break;
        }
        if (inputTimer == 0) {
            if (*buttons == 0x10) {
                destMode = -3;
                HuAudFXPlay(1);
                break;
            }
            if (*buttons == 0x20) {
                destMode = -4;
                HuAudFXPlay(1);
                break;
            }
            item_count = BoardPlayerItemCount(rollPlayer);
            if (GWTeamGet()) {
                teammate = BoardPlayerSameTeamFind(rollPlayer);
                item_count += BoardPlayerItemCount(teammate);
            }
            if (item_count == 0 || BoardItemPrevGet() != -1 || BoardMegaDoubleDiceCheck() || _CheckFlag(FLAG_ID_MAKE(1, 9))) {
                if (_CheckFlag(FLAG_ID_MAKE(1, 9)) && GWBoardGet() == BOARD_ID_MAIN6 && *buttons == 0x200 && item_count != 0) {
                    HuAudFXPlay(4);
                }
            }
            else if (*buttons == 0x200) {
                destMode = -2;
                HuAudFXPlay(3);
                BoardPauseDisableSet(1);
                break;
            }
        }
    }
}

/**
 * @brief Waits until one dice block finishes its entrance animation.
 *
 * @param dice_index Zero-based index of the dice object to wait for.
 */
static void DiceWaitFull(s32 dice_index)
{
    DiceWork *work = omObjGetWork(diceObj[dice_index], DiceWork);

    while (work->state != 1) {
        HuPrcVSleep();
    }
}

/**
 * @brief Starts a dice block's hit animation and captures its face value.
 *
 * @details The visible motion frame becomes a value from 1 through maxRoll,
 * unless a nonnegative tutorial value overrides it.
 *
 * @param dice_index Zero-based index of the struck die.
 */
static void DiceSetHit(s32 dice_index)
{
    DiceWork *work = omObjGetWork(diceObj[dice_index], DiceWork);

    work->state = 2;
    work->animation_timer = 0;
    BoardModelMotionSpeedSet(diceMdl[dice_index], 0.0f);
    diceValue[dice_index] = (s32)BoardModelMotionTimeGet(diceMdl[dice_index]) + 1;
    if (tutorialRollF != 0 && tutorialRoll[dice_index] != -1) {
        BoardModelMotionTimeSet(diceMdl[dice_index], tutorialRoll[dice_index] - 1);
        diceValue[dice_index] = tutorialRoll[dice_index];
    }
}

/**
 * @brief Releases windows, dice, confetti, and process state for a roll.
 */
static void RollDestroy(void)
{
    if (rollWin != -1) {
        HuWinKill(rollWin);
        rollWin = -1;
    }
    if (rollProc) {
        HuPrcKill(rollProc);
    }
    DicePauseAll();
    DiceKill();
    BoardConfettiKill();
    rollProc = NULL;
}

/**
 * @brief Updates a dice block's entrance, idle spin, hit, and cleanup states.
 *
 * @param object Dice object whose transform and DiceWork state are updated.
 */
static void DiceMain(OMOBJ *object)
{
    DiceWork *work = omObjGetWork(object, DiceWork);
    float angle;
    float scale_amplitude;
    float scale_base;
    s16 current_face;
    s16 next_face;

    if (work->kill_requested != 0 || BoardIsKill()) {
        if (diceSndStatus[work->dice_index] != -1) {
            HuAudFXStop(diceSndStatus[work->dice_index]);
            diceSndStatus[work->dice_index] = -1;
        }
        if (diceMdl[work->dice_index] != -1) {
            BoardModelKill(diceMdl[work->dice_index]);
            diceMdl[work->dice_index] = -1;
        }
        if (diceEff[work->dice_index] != -1) {
            Hu3DParManKill(diceEff[work->dice_index]);
            diceEff[work->dice_index] = -1;
        }
        DicePause(work->dice_index);
        diceObj[work->dice_index] = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    switch (work->state) {
        case 0:
            if (work->rotation_angle >= 0x438) {
                work->state = 1;
                object->scale.x = object->scale.y = object->scale.z = 1.0f;
                object->rot.y = work->animation_timer = 0;
            }
            else {
                if (work->animation_timer == 0 && diceSndStatus[work->dice_index] == -1) {
                    diceSndStatus[work->dice_index] = HuAudFXPlay(0x300);
                }
                object->rot.y = work->rotation_angle % 360;
                work->rotation_angle += 0x19;
                if (work->animation_timer < 0xB4) {
                    scale_base = 0.0f;
                    scale_amplitude = 1.0f;
                }
                else {
                    scale_base = 1.0f;
                    scale_amplitude = 0.5f;
                }
                object->scale.x = scale_base + scale_amplitude * sind((float)(work->animation_timer % 180));
                object->scale.y = object->scale.x;
                object->scale.z = object->scale.x;
                if (work->animation_timer <= 360) {
                    if (work->animation_timer < 0xB4) {
                        work->animation_timer += 6;
                    }
                    else {
                        work->animation_timer += 0x12;
                    }
                    if (work->animation_timer > 360) {
                        work->animation_timer = 360;
                    }
                }
            }
            break;
        case 1:
            work->animation_timer += 4;
            if (work->animation_timer > 360) {
                work->animation_timer -= 360;
            }
            OSs16tof32(&work->animation_timer, &angle);
            object->trans.y += 0.2f * sind(angle);
            break;
        case 2:
            work->animation_timer++;
            if (work->animation_timer >= 8) {
                if (diceSndStatus[work->dice_index] != -1) {
                    HuAudFXStop(diceSndStatus[work->dice_index]);
                    diceSndStatus[work->dice_index] = -1;
                }
                HuAudFXPlay(0x303);
                HuAudFXPlay(0x30C);
                work->state = 3;
                work->animation_timer = 0;
                BoardDiceValueSet(work->dice_index, diceValue[work->dice_index]);
                diceEff[work->dice_index] = BoardDiceEffectCreate();
                Hu3DParManPosSet(diceEff[work->dice_index], object->trans.x, object->trans.y, object->trans.z);
                BoardModelVisibilitySet(diceMdl[work->dice_index], 0);
            }
            else {
                OSs16tof32(&work->animation_timer, &angle);
                if (work->animation_timer < 4) {
                    object->trans.y += 10.0f + (0.5f * angle);
                    object->scale.x += 0.1f * sind(4.0f * angle);
                    object->scale.y = object->scale.x;
                    object->scale.z = object->scale.x;
                }
                else {
                    BoardModelAlphaSet(diceMdl[work->dice_index], 0xFF - (work->animation_timer << 5));
                }
                object->trans.y += -0.49f * angle * angle;
            }
            break;
        case 3:
            break;
    }
    if (diceEff[work->dice_index] != -1 && ++work->animation_timer >= 150) {
        work->kill_requested = 1;
        DiceHideEffect(work->dice_index, 0);
    }
    if (work->face_change_disabled == 0 || work->state != 3) {
        if (work->face_change_timer++ > work->face_change_delay) {
            work->face_change_timer = 0;
            current_face = BoardModelMotionTimeGet(diceMdl[work->dice_index]);
            do {
                next_face = BoardRandMod(maxRoll);
            } while (next_face == current_face);
            BoardModelMotionTimeSet(diceMdl[work->dice_index], next_face + 0.5f);
        }
    }
    BoardModelPosSet(diceMdl[work->dice_index], object->trans.x, object->trans.y, object->trans.z);
    BoardModelRotSet(diceMdl[work->dice_index], object->rot.x, object->rot.y, object->rot.z);
    BoardModelScaleSet(diceMdl[work->dice_index], object->scale.x, object->scale.y, object->scale.z);
}

/**
 * @brief Requests cleanup of one dice block object.
 *
 * @param dice_index Zero-based dice slot; empty slots are ignored.
 */
static void DicePause(s32 dice_index)
{
    OMOBJ *object = diceObj[dice_index];

    if (object) {
        omObjGetWork(object, DiceWork)->kill_requested = 1;
    }
}

/**
 * @brief Requests cleanup of all three possible dice block objects.
 */
static void DicePauseAll(void)
{
    DicePause(0);
    DicePause(1);
    DicePause(2);
}

/**
 * @brief Creates the particle burst displayed when a die is struck.
 *
 * @return Particle-manager ID for subsequent positioning, visibility, or cleanup.
 */
s16 BoardDiceEffectCreate(void)
{
    s16 particle_manager;
    s16 model_id;
    ANIMDATA *animation;
    void *animation_data;

    animation_data = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_BOARD, 0x6B), HU_MEMNUM_OVL, HEAP_MODEL);
    animation = HuSprAnimRead(animation_data);
    particle_manager = Hu3DParManCreate(animation, 0x64, &diceEffParam);
    Hu3DParManAttrSet(particle_manager, 0x108);
    Hu3DParManRotSet(particle_manager, 0.0f, 0.0f, 0.0f);
    Hu3DParManVecSet(particle_manager, 0.0f, 0.4f, 0.2f);
    Hu3DParManTimeLimitSet(particle_manager, 0x96);
    model_id = Hu3DParManModelIDGet(particle_manager);
    Hu3DModelLayerSet(model_id, 1);
    Hu3DParticleBlendModeSet(model_id, 0);
    return particle_manager;
}

/**
 * @brief Shows or hides one die's particle effect model.
 *
 * @param dice_index Zero-based dice slot containing the particle manager ID.
 * @param visible Nonzero to show the effect or zero to hide it.
 */
static void DiceHideEffect(s32 dice_index, s32 visible)
{
    s16 model_id;

    if (diceEff[dice_index] == -1) {
        return;
    }
    model_id = Hu3DParManModelIDGet(diceEff[dice_index]);
    if (visible != 0) {
        Hu3DModelAttrReset(model_id, HU3D_ATTR_DISPOFF);
    }
    else {
        Hu3DModelAttrSet(model_id, HU3D_ATTR_DISPOFF);
    }
}

/**
 * @brief Moves all active roll-value digits toward the player display target.
 *
 * @details Also resets their Y rotation so BoardDiceDoneCheck() can report
 * completion after the movement interpolation settles.
 */
void BoardDicePauseAll(void)
{
    DiceDigitWork *work;
    s32 i;

    for (i = 0; i < numDice; i++) {
        if (diceDigitObj[i]) {
            work = omObjGetWork(diceDigitObj[i], DiceDigitWork);
            work->move_to_player = 1;
            work->state = 1;
            work->timer = 0;
        }
    }
}

/**
 * @brief Checks whether every active roll-value digit has reached its target.
 *
 * @return TRUE when no active digit object is still moving; otherwise FALSE.
 */
BOOL BoardDiceDoneCheck(void)
{
    DiceDigitWork *work;
    s32 i;

    for (i = 0; i < numDice; i++) {
        if (diceDigitObj[i]) {
            work = omObjGetWork(diceDigitObj[i], DiceDigitWork);
            if (work->state != 0) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

/**
 * @brief Requests destruction of one roll-value digit object.
 *
 * @param dice_index Zero-based digit-object slot; empty slots are ignored.
 */
void BoardDiceStop(s32 dice_index)
{
    DiceDigitWork *work;

    if (!diceDigitObj[dice_index]) {
        return;
    }
    work = omObjGetWork(diceDigitObj[dice_index], DiceDigitWork);
    work->kill_requested = 1;
}

/**
 * @brief Sets visibility of one roll result's digit models.
 *
 * @param dice_index Zero-based digit-object slot.
 * @param visible Visibility value passed to BoardModelVisibilitySet(). The tens
 * digit remains hidden for values below 10.
 */
void BoardDiceVisibleSet(s32 dice_index, s32 visible)
{
    DiceDigitWork *work;

    if (!diceDigitObj[dice_index]) {
        return;
    }
    work = omObjGetWork(diceDigitObj[dice_index], DiceDigitWork);
    if ((work->value / 10) == 0) {
        BoardModelVisibilitySet(work->model_ids[1], 0);
    }
    else {
        BoardModelVisibilitySet(work->model_ids[1], visible);
    }
    BoardModelVisibilitySet(work->model_ids[0], visible);
}

/**
 * @brief Creates the one- or two-digit model for a rolled value.
 *
 * @details Values above 99 are clamped to 99. The models begin above the
 * current player and are later moved into their HUD positions.
 *
 * @param dice_index Zero-based slot associated with the source die.
 * @param value Rolled value to display; expected to be nonnegative and capped
 * to two decimal digits by this routine.
 */
void BoardDiceValueSet(s32 dice_index, s32 value)
{
    OMOBJ *object;
    DiceDigitWork *work;
    Vec position;
    s16 digits[2];
    s16 model_id;
    s16 i;

    if (value > 0x63) {
        value = 0x63;
    }
    digits[0] = (value % 10);
    digits[1] = (value / 10);
    BoardPlayerPosGet(GWSystem.player_curr, &position);
    position.y += 300.0f;
    object = omAddObjEx(boardObjMan, 0x102, 0, 0, -1, DiceDigitMain);
    work = omObjGetWork(object, DiceDigitWork);
    work->kill_requested = 0;
    work->move_to_player = 0;
    work->state = 0;
    work->dice_index = dice_index;
    work->move_fraction = 0.3f;
    work->value = value;
    work->timer = 360;
    for (i = 0; i < 2; i++) {
        model_id = BoardModelCreate(diceDigitMdlTbl[digits[i]], NULL, 0);
        BoardModelPosSetV(model_id, &position);
        BoardModelMotionStart(model_id, 0, 0);
        BoardModelMotionSpeedSet(model_id, 0.0f);
        BoardModelMotionTimeSet(model_id, 0.0f);
        BoardModelScaleSet(model_id, 0.143f, 0.143f, 0.143f);
        work->model_ids[i] = model_id;
    }
    if (digits[1] == 0) {
        BoardModelVisibilitySet(work->model_ids[1], 0);
    }
    diceDigitObj[dice_index] = object;
}

/**
 * @brief Updates movement and rotation for one roll-value digit object.
 *
 * @param object Digit object whose DiceDigitWork state is updated.
 */
static void DiceDigitMain(OMOBJ *object)
{
    DiceDigitWork *work = omObjGetWork(object, DiceDigitWork);

    if (work->kill_requested != 0 || BoardIsKill()) {
        DiceDigitKill(work);
        diceDigitObj[work->dice_index] = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    DiceDigitMove(work, work->move_to_player);
    DiceDigitRotate(work, work->move_to_player);
}

/**
 * @brief Destroys both model slots owned by a roll-value digit object.
 *
 * @param work Digit state containing the model IDs to destroy.
 */
static void DiceDigitKill(DiceDigitWork *work)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        if (work->model_ids[i] != -1) {
            BoardModelKill(work->model_ids[i]);
        }
    }
}

/**
 * @brief Immediately destroys models for all active roll-value digit objects.
 */
static void DiceKill(void)
{
    DiceDigitWork *work;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (diceDigitObj[i]) {
            work = omObjGetWork(diceDigitObj[i], DiceDigitWork);
            DiceDigitKill(work);
        }
    }
}

/**
 * @brief Interpolates roll-value digit models toward a screen-layout target.
 *
 * @param work Digit state containing value, dice index, models, and movement status.
 * @param move_to_player Nonzero to use the shared player-display target; zero
 * to use the one-, two-, or three-dice result layout.
 */
static void DiceDigitMove(DiceDigitWork *work, s32 move_to_player)
{
    s32 unused_value = 0;
    Vec current_pos;
    Vec offset;
    Vec target_positions[2];
    Vec single_dice_pos = { (HU_DISP_WIDTH / 2), (HU_DISP_HEIGHT * 155) / 480, 240.0f };
    Vec two_dice_positions[2]
        = { { (HU_DISP_WIDTH * 1) / 4, (HU_DISP_HEIGHT * 155) / 480, 240.0f }, { (HU_DISP_WIDTH * 3) / 4, (HU_DISP_HEIGHT * 155) / 480, 240.0f } };
    Vec three_dice_positions[3] = { { (HU_DISP_WIDTH * 1) / 4, (HU_DISP_HEIGHT * 155) / 480, 240.0f },
        { (HU_DISP_WIDTH * 3) / 4, (HU_DISP_HEIGHT * 155) / 480, 240.0f }, { (HU_DISP_WIDTH / 2), (HU_DISP_HEIGHT * 155) / 480, 240.0f } };
    Vec player_target_pos = { (HU_DISP_WIDTH / 2), (HU_DISP_HEIGHT * 176) / 480, 240.0f };
    Vec *base_target;
    float depth_offset;
    s16 tens_digit;
    s16 i;

    tens_digit = (work->value / 10);
    if (move_to_player != 0) {
        base_target = &player_target_pos;
        depth_offset = -20.0f * work->dice_index;
    }
    else {
        switch (numDice) {
            case 1:
                base_target = &single_dice_pos;
                break;
            case 2:
                base_target = &two_dice_positions[work->dice_index];
                break;
            case 3:
                base_target = &three_dice_positions[work->dice_index];
                break;
        }
        depth_offset = 0.0f;
    }
    for (i = 0; i < 2; i++) {
        if (tens_digit == 0) {
            target_positions[i].x = base_target->x;
        }
        else if (i != 0) {
            target_positions[i].x = base_target->x - 30.0f;
        }
        else {
            target_positions[i].x = base_target->x + 30.0f;
        }
        target_positions[i].y = base_target->y;
        target_positions[i].z = base_target->z + depth_offset;
    }
    for (i = 0; i < 2; i++) {
        BoardModelPosGet(work->model_ids[i], &current_pos);
        Hu3D2Dto3D(&target_positions[i], 1, &target_positions[i]);
        VECSubtract(&target_positions[i], &current_pos, &offset);
        if (VECMag(&offset) < 0.01f) {
            work->state = 0;
            if (move_to_player != 0 && work->dice_index != 0) {
                BoardModelVisibilitySet(work->model_ids[i], 0);
            }
        }
        else {
            work->state = 1;
        }
        VECScale(&offset, &offset, work->move_fraction);
        VECAdd(&current_pos, &offset, &current_pos);
        BoardModelPosSetV(work->model_ids[i], &current_pos);
    }
}

/**
 * @brief Rotates roll-value digits toward their target orientation.
 *
 * @param work Digit state containing the current degree angle and model IDs.
 * @param move_to_player Nonzero to rotate toward 360 degrees; zero to rotate
 * back toward 0 degrees.
 */
static void DiceDigitRotate(DiceDigitWork *work, s32 move_to_player)
{
    float rotation;
    s32 i;

    if (move_to_player != 0) {
        work->timer += 0x12;
        if (work->timer >= 360) {
            work->timer = 360;
        }
    }
    else {
        work->timer -= 0x12;
        if (work->timer <= 0) {
            work->timer = 0;
        }
    }
    OSs16tof32(&work->timer, &rotation);
    for (i = 0; i < 2; i++) {
        if (work->model_ids[i] != -1) {
            BoardModelRotSet(work->model_ids[i], 0.0f, rotation, 0.0f);
        }
    }
}

/**
 * @brief Awards the coin bonus for matching values on multiple dice.
 *
 * @details Presents confetti and a message, then awards 10, 30, or 50 coins
 * according to dice count, parity, and the special all-sevens result.
 */
static void SameRollExec(void)
{
    Vec confetti_pos;
    float confetti_radius;
    s32 confetti_count;
    s32 stream_id;
    s32 coin_reward;
    s32 i;
    s32 coin_delay;

    stream_id = -1;
    BoardPauseDisableSet(1);
    confetti_count = 0x50;
    switch (numDice) {
        case 2:
            if (diceValue[0] == 7) {
                coin_reward = 0x1E;
            }
            else {
                coin_reward = 0xA;
            }
            break;
        case 3:
            if (diceValue[0] == 7) {
                coin_reward = 0x32;
                confetti_count = 0x118;
            }
            else if (diceValue[0] & 1) {
                coin_reward = 0x1E;
            }
            else {
                coin_reward = 0xA;
            }
            break;
        default:
            return;
    }
    sprintf(sameRollCoinStr, "%d", coin_reward);
    BoardPlayerPosGet(rollPlayer, &confetti_pos);
    if (BoardPlayerSizeGet(rollPlayer) == 2 || GWPlayer[rollPlayer].bowser_suit != 0) {
        confetti_pos.y += 700.0f;
        confetti_radius = 450.0f;
    }
    else {
        confetti_pos.y += 600.0f;
        confetti_radius = 300.0f;
    }
    BoardConfettiCreate(&confetti_pos, confetti_count, confetti_radius);
    BoardAudSeqPause(0, 1, 0x3E8);
    HuPrcSleep(0xA);
    stream_id = HuAudSStreamPlay(9);
    while (HuAudSStreamStatGet(stream_id) != 0) {
        HuPrcVSleep();
    }
    BoardAudSeqPause(0, 0, 0x3E8);
    BoardWinCreate(0, MAKE_MESSID(9, 0), -1);
    BoardWinInsertMesSet(MAKE_MESSID_PTR(sameRollCoinStr), 0);
    BoardWinWait();
    BoardWinKill();
    if (coin_reward >= 0x32) {
        coin_delay = 1;
    }
    else if (coin_reward >= 0x14) {
        coin_delay = 3;
    }
    else {
        coin_delay = 6;
    }
    for (i = 0; i < coin_reward; i++) {
        BoardPlayerCoinsAdd(rollPlayer, 1);
        HuAudFXPlay(7);
        HuPrcSleep(coin_delay);
    }
    HuAudFXPlay(0xF);
    BoardConfettiStop();
    BoardPauseDisableSet(0);
}

/**
 * @brief Gets the next roll-related button action for a human, CPU, or tutorial.
 *
 * @param player Zero-based player index whose controller or CPU policy is read.
 * @return A button mask: 0x100 rolls, 0x200 selects an item, and human input
 * may also return the board-view masks handled by DoInput().
 */
static s32 GetBtnRoll(s32 player)
{
    s32 buttons = 0;
    s32 port;

    if (boardTutorialUseItem >= 0 && boardTutorialUseItem <= 0xD) {
        buttons = 0x200;
        return buttons;
    }
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        buttons = 0x100;
        return buttons;
    }
    if (!GWPlayer[player].com) {
        port = GWPlayer[player].port;
        buttons = HuPadBtnDown[port];
        return buttons;
    }
    if (_CheckFlag(FLAG_ID_MAKE(1, 9)) || BoardItemPrevGet() != -1) {
        return 0x100;
    }
    buttons = GetComBtnRoll(player);
    return buttons;
}

/**
 * @brief Chooses whether a CPU player rolls or opens item selection.
 *
 * @param player Zero-based CPU player index.
 * @return 0x200 when CPU item logic chooses an item; otherwise 0x100 to roll.
 */
static s32 GetComBtnRoll(s32 player)
{
    if (BoardItemPrevGet() != -1 || BoardPlayerItemCount(player) == 0 || BoardMegaDoubleDiceCheck() || _CheckFlag(FLAG_ID_MAKE(1, 9))) {
        return 0x100;
    }
    if (BoardComUseItemCheck(player)) {
        return 0x200;
    }
    return 0x100;
}

/**
 * @brief Configures fixed results for the next tutorial roll.
 *
 * @param roll_values Array of three desired die values. A value of -1 leaves
 * the corresponding die's naturally selected face unchanged.
 */
void BoardRollTutorialSet(s16 *roll_values)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        tutorialRoll[i] = roll_values[i];
    }
    tutorialRollF = 1;
}
