#include "game/board/battle.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/flag.h"
#include "game/gamework.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include "game/board/audio.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/tutorial.h"
#include "game/board/ui.h"
#include "game/board/window.h"

#include "ext_math.h"

#include "game/hsfex.h"

typedef struct {
    struct {
        u8 kill : 1;
        u8 state : 2;
    };
    s8 delay;
    s16 bomb_model;
    s16 burst_models[4];
} ExplodeWork;

typedef struct {
    struct {
        u8 kill : 1;
        u8 state : 4;
        u8 coin_display_initialized : 1;
        u8 coin_selection_slowing : 1;
    };
    u8 timer;
    s8 step_delay;
    s8 step_timer;
    s8 slowdown_level;
    char padding[3];
    s16 sprite_group;
} BattleWork;

extern s32 BoardMGSetupPlaySearch(s32, s16);
extern void BoardMGSetupPlayPush(s32, s16);

static void DestroyBattle(void);
static void ExecBattle(void);
static void TakeCoins(void);
static void InitExplode(void);
static s32 CheckExplode(void);
static void UpdateExplode(OMOBJ *arg0);
static void CreateBattleMain(void);
static void UpdateBattleMain(OMOBJ *arg0);
static void DescendBattleBomb(BattleWork *arg0, OMOBJ *arg1);
static void EndBattle(BattleWork *arg0, OMOBJ *arg1);
static void ShowBattleGame(BattleWork *arg0, OMOBJ *arg1);
static void RaiseBattleGame(BattleWork *arg0, OMOBJ *arg1);
static void UpdateBattleCoin(BattleWork *arg0, OMOBJ *arg1);
static void ShowBattleCoin(BattleWork *arg0, OMOBJ *arg1);
static void InitBattleGameSpr(BattleWork *arg0, OMOBJ *arg1);
static void HideBattleSpr(void);
static void VibratePad(BattleWork *arg0, OMOBJ *arg1);
static void StopBattleBomb(void);
static s32 GetBattleBombState(void);
static void SetBattleBombState(s32 arg0);
static void SetBattleCoinValue(void);

static s16 battleMGList[8];

static s8 currPlayer;
static s8 coinTakeMax;
static u8 mgChoice;
static s16 currSpace;
static s8 battleCoinIdx;
static s8 battleMGIdx[2];
static char totalCoinStr[8];
static float battleCoinSpeed;
static float battleCoinPosF;
static OMOBJ *explodeObj;
static OMOBJ *battleObj;
static HUPROCESS *battleProc;

static const s8 battleCoinTbl[] = {
    5, 10, 20, 30, 50
};

static float battleCursorPosTbl[] = {
    144.0f, 330.0f, 0.0f,
    432.0f, 330.0f, 0.0f
};

static s32 bombFXTbl[] = {
    0x00000123,
    0x00000163,
    0x000001A3,
    0x000001E3,
    0x00000223,
    0x00000263,
    0x000002A3,
    0x000002E3
};

static s16 battleSprPrioTbl[] = {
    1000, 990, 1000, 980,
     980, 970,  970, 970,
     970, 970,  970
};

/**
 * @brief Resolve the Battle Space event for the landing player.
 *
 * @details Normal play selects two candidate minigames, chooses and collects a coin stake, then launches the selected minigame. The tutorial path only plays its tutorial sequence.
 * @param player Index of the player who landed on the space.
 * @param space Index of the Battle Space, used to find all players caught in its opening blast.
 */
void BoardBattleExec(s32 player, s32 space) {
    if (_CheckFlag(FLAG_ID_MAKE(1, 11))) {
        HuAudFXPlay(0x34A);
        BoardCameraViewSet(2);
        BoardPlayerMotBlendSet(player, 0, 0xF);
        while (!BoardPlayerMotBlendCheck(player)) {
            HuPrcVSleep();
        }
        BoardCameraMotionWait();
        BoardTutorialHookExec(0xF, 0);
        GWPlayer[player].color = 3;
        return;
    }
    currPlayer = player;
    currSpace = space;
    battleProc = HuPrcChildCreate(ExecBattle, 0x2004, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(battleProc, DestroyBattle);
    while (battleProc) {
        HuPrcVSleep();
    }
    GWPlayer[player].color = 3;
}

/**
 * @brief Close Battle Space data and signal that its child process has ended.
 */
static void DestroyBattle(void) {
    HuDataDirClose(DATADIR_BBATTLE);
    battleProc = NULL;
}

/**
 * @brief Select battle minigames, collect the stake, and transition to the chosen minigame.
 *
 * @details If nobody has at least ten coins, the event displays a message and returns to board play without collecting a stake.
 */
static void ExecBattle(void) {
    s16 minigame_id;
    s16 has_eligible_player;
    s32 battle_data_read_id;
    s32 minigame_data_read_id;
    s32 selection_index;
    s32 index;
    s32 default_choice;
    s32 minigame_data_dir;
    s32 random_choice;
    MgInfo *minigame_info;
    s32 no_minigame_available = 0;
    s32 board_data_dirs[] = {
        DATADIR_W01,
        DATADIR_W02,
        DATADIR_W03,
        DATADIR_W04,
        DATADIR_W05,
        DATADIR_W06,
        DATADIR_W10,
        DATADIR_W20,
        DATADIR_W21
    };

    minigame_info = mgInfoTbl;
    for (index = selection_index = 0; minigame_info->ovl != (u16) DLL_NONE; minigame_info++, index++) {
        if (minigame_info->type == 4) {
            battleMGList[selection_index] = index + 401;
            selection_index++;
        }
    }
    for (selection_index = 0; selection_index < 2; selection_index++) {
        while (1) {
            index = BoardRandMod(6);
            minigame_id = battleMGList[index];
            if (GWMGListGet() == 2) {
                if (GWMGCustomGet(minigame_id) == 0) {
                    continue;
                }
            } else {
                if (GWMGListGet() == 1) {
                    minigame_info = &mgInfoTbl[minigame_id - 401];
                    if (!(minigame_info->flag & 1)) {
                        continue;
                    }
                } else if (BoardMGSetupPlaySearch(4, minigame_id) != 0) {
                    continue;
                }
            }
            if (selection_index == 1 && index == battleMGIdx[selection_index - 1]) {
                continue;
            }
            break;
        }
        battleMGIdx[selection_index] = index;
        BoardMGSetupPlayPush(4, minigame_id);
    }
    if (_CheckFlag(FLAG_ID_MAKE(1, 4))) {
        BoardStatusItemSet(1);
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        _ClearFlag(FLAG_ID_MAKE(1, 28));
        HuPrcSleep(60);
        HuPrcEnd();
    }
    HuAudFXPlay(0x34A);
    omVibrate(GWSystem.player_curr, 12, 6, 6);
    BoardAudSeqPause(0, 1, 500);
    battle_data_read_id = BoardDataDirReadAsync(DATADIR_BBATTLE);
    BoardCameraViewSet(3);
    BoardCameraMotionWait();
    BoardPlayerMotBlendSet(currPlayer, 0, 0xF);
    while (!BoardPlayerMotBlendCheck(currPlayer)) {
        HuPrcVSleep();
    }
    BoardDataAsyncWait(battle_data_read_id);
    SetBattleCoinValue();
    BoardMusStart(1, 4, 0x7F, 0);
    omVibrate(GWSystem.player_curr, 12, 6, 6);
    InitExplode();
    while (CheckExplode() == 0) {
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    CreateBattleMain();
    while (GetBattleBombState() != 8) {
        HuPrcVSleep();
    }
    has_eligible_player = 0;
    for (index = 0; index < 4; index++) {
        if (BoardPlayerCoinsGet(index) >= 10) {
            has_eligible_player = 1;
            break;
        }
    }
    if (has_eligible_player == 0) {
        BoardWinCreate(2, MAKE_MESSID(2, 1), 1);
        BoardWinWait();
        BoardWinKill();
        no_minigame_available = 1;
    } else {
        HuAudFXPlay(0x4F);
        BoardWinCreate(2, MAKE_MESSID(2, 0), 1);
        BoardWinWait();
        BoardWinKill();
        SetBattleBombState(3);
        while (GetBattleBombState() != 8) {
            HuPrcVSleep();
        }
        SetBattleBombState(4);
        while (GetBattleBombState() != 8) {
            HuPrcVSleep();
        }
        TakeCoins();
        if (GWPlayer[currPlayer].com) {
            if (BoardRandFloat() < 0.5f) {
                random_choice = 0;
            } else {
                random_choice = 1;
            }
            default_choice = random_choice;
            HuPrcSleep(60);
        } else {
            default_choice = -1;
        }
        SetBattleBombState(5);
        BoardPickerCreate(currPlayer, 2, battleCursorPosTbl, default_choice);
        BoardPickerBackFlagSet(0);
        while (!BoardPickerDoneCheck()) {
            HuPrcVSleep();
        }
        mgChoice = BoardPickerChoiceGet();
        SetBattleBombState(6);
        while (GetBattleBombState() != 8) {
            HuPrcVSleep();
        }
        BoardAudSeqFadeOut(1, 1000);
        BoardMusVolPanSet(0, 0, 1);
        BoardAudSeqPause(0, 0, 1);
        if (no_minigame_available == 0) {
            HuDataDirClose(DATADIR_BOARD);
            HuDataDirClose(DATADIR_BGUEST);
            HuDataDirClose(board_data_dirs[GWBoardGet()]);
            HuPrcSleep(2);
            minigame_id = battleMGList[battleMGIdx[mgChoice]];
            GWMGAvailSet(minigame_id);
            GWSystem.mg_next = minigame_id - 401;
            if (_CheckFlag(FLAG_ID_MAKE(0, 11))) {
                minigame_data_dir = DATADIR_INST;
            } else {
                minigame_data_dir = mgInfoTbl[minigame_id - 401].data_dir;
            }
            minigame_data_read_id = BoardDataDirReadAsync(minigame_data_dir);
        }
        _SetFlag(FLAG_ID_MAKE(1, 14));
        WipeColorSet(0xFF, 0xFF, 0xFF);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        HuAudFXAllStop();
        _SetFlag(FLAG_ID_MAKE(1, 28));
        HideBattleSpr();
        for (index = 0; index < 4; index++) {
            BoardPlayerMotionStart(index, 1, 0x40000001);
        }
        BoardDataAsyncWait(minigame_data_read_id);
        if (no_minigame_available == 0) {
            _SetFlag(FLAG_ID_MAKE(1, 4));
            BoardNextOvlSet(DLL_instdll);
            BoardFilterFadeOut(30);
            StopBattleBomb();
            HuPrcEnd();
        }
    }
    BoardAudSeqFadeOut(1, 1000);
    SetBattleBombState(1);
    while (GetBattleBombState() != 8) {
        HuPrcVSleep();
    }
    StopBattleBomb();
    for (index = 0; index < 4; index++) {
        BoardPlayerIdleSet(index);
        BoardPlayerVoiceEnableSet(index, 6, 1);
    }
    BoardAudSeqPause(0, 0, 1000);
    HuPrcEnd();
}

/**
 * @brief Collect each player's available share of the selected battle stake.
 *
 * @details Each contribution is capped by the player's current coins and saved in GWPlayer.coins_battle; the displayed pot is the sum actually collected.
 */
static void TakeCoins(void) {
    u16 total_coins;
    s32 player_coins;
    s32 coins_to_take;
    s32 message_id;
    s32 coin_round;
    s32 remaining_coins;
    s32 coin_delay;
    s32 i;

    for (total_coins = i = 0; i < 4; i++) {
        player_coins = BoardPlayerCoinsGet(i);
        if (player_coins >= coinTakeMax) {
            total_coins += coinTakeMax;
        } else {
            total_coins += player_coins;
        }
    }
    if (coinTakeMax >= 50) {
        coin_delay = 1;
    } else if (coinTakeMax >= 20) {
        coin_delay = 3;
    } else {
        coin_delay = 6;
    }
    for (i = 0; i < 4; i++) {
        if (BoardPlayerCoinsGet(i) < coinTakeMax) {
            coins_to_take = BoardPlayerCoinsGet(i);
        } else {
            coins_to_take = coinTakeMax;
        }
        GWPlayer[i].coins_battle = coins_to_take;
    }
    for (coin_round = 0; coin_round < coinTakeMax; coin_round++) {
        for (remaining_coins = i = 0; i < 4; i++) {
            BoardPlayerCoinsAdd(i, -1);
            remaining_coins += BoardPlayerCoinsGet(i);
        }
        if (remaining_coins <= 0) {
            break;
        }
        HuAudFXPlay(0xE);
        HuPrcSleep(coin_delay);
    }
    HuAudFXPlay(0xF);
    sprintf(totalCoinStr, "%d", total_coins);
    if (total_coins != coinTakeMax * 4) {
        message_id = MAKE_MESSID(2, 3);
    } else {
        message_id = MAKE_MESSID(2, 2);
    }
    BoardWinCreate(2, message_id, 1);
    BoardWinInsertMesSet(MAKE_MESSID_PTR(totalCoinStr), 0);
    BoardWinWait();
    BoardWinKill();
}

/**
 * @brief Create the bomb and four burst models for the Battle Space opening.
 */
static void InitExplode(void) {
    Vec player_pos;
    Vec burst_pos;
    s32 i;
    ExplodeWork *work;
    float burst_offsets[4][2] = {
        { -50.0f, -50.0f },
        {  50.0f, -50.0f },
        { -50.0f,  50.0f },
        {  50.0f,  50.0f }
    };

    BoardPlayerPosGet(currPlayer, &player_pos);
    explodeObj = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, UpdateExplode);
    work = omObjGetWork(explodeObj, ExplodeWork);
    work->kill = 0;
    work->state = 0;
    work->delay = 0x10;
    work->bomb_model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BGUEST, 38), NULL, 0);
    BoardModelLayerSet(work->bomb_model, 2);
    explodeObj->trans.x = player_pos.x;
    explodeObj->trans.y = player_pos.y - 100.0f;
    explodeObj->trans.z = player_pos.z;
    BoardModelPosSet(work->bomb_model, explodeObj->trans.x, explodeObj->trans.y, explodeObj->trans.z);
    BoardModelMotionStart(work->bomb_model, 0, 0x40000001);
    for (i = 0; i < 4; i++) {
        work->burst_models[i] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 102), NULL, 0);
        burst_pos.x = player_pos.x + burst_offsets[i][0];
        burst_pos.y = player_pos.y;
        burst_pos.z = player_pos.z + burst_offsets[i][1];
        BoardModelPosSetV(work->burst_models[i], &burst_pos);
        BoardModelLayerSet(work->burst_models[i], 2);
    }
    HuAudFXPlay(0x33C);
}

/**
 * @brief Read the bomb opening-animation state.
 * @return The current explosion state; zero during the initial delay and rise, and nonzero once it reaches its screen-space presentation.
 */
static s32 CheckExplode(void) {
    ExplodeWork *work = omObjGetWork(explodeObj, ExplodeWork);

    return work->state;
}

/**
 * @brief Advance or destroy the Battle Space bomb animation.
 *
 * @details Triggers reactions for players on the Battle Space, raises the bomb, then keeps it aligned to a screen-space point.
 * @param arg0 Bomb object carrying the transform and ExplodeWork state.
 */
static void UpdateExplode(OMOBJ *arg0) {
    Vec screen_pos;
    Vec camera_rot;
    float motion_time;
    ExplodeWork *work;
    Mtx model_mtx;
    s32 i;

    (void) 1000.0f; // TODO: 1000.0f needs to appear right before 20.0f in sdata2 to match.
    work = omObjGetWork(arg0, ExplodeWork);
    if (work->kill != 0 || BoardIsKill()) {
        BoardModelKill(work->bomb_model);
        for (i = 0; i < 4; i++) {
            BoardModelKill(work->burst_models[i]);
        }
        explodeObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    switch (work->state) {
        case 0:
            if (work->delay == 0) {
                for (i = 0; i < 4; i++) {
                    if (currSpace == GWPlayer[i].space_curr) {
                        motion_time = 20.0f * BoardRandFloat();
                        HuAudFXPlay(bombFXTbl[GWPlayer[i].character]);
                        BoardPlayerMotionShiftSet(i, 6, motion_time, 8.0f, HU3D_MOTATTR_LOOP);
                    }
                }
                CharEffectLayerSet(2);
            }
            if (work->delay-- <= 0) {
                arg0->trans.y += 50.0f;
                if (arg0->trans.y >= 800.0f) {
                    arg0->scale.x = 288.0f;
                    arg0->scale.y = -128.0f;
                    arg0->scale.z = 1000.0f;
                    for (i = 0; i < 4; i++) {
                        BoardModelVisibilitySet(work->burst_models[i], 0);
                    }
                    work->state = 2;
                }
            }
            break;
        case 3:
            BoardModelVisibilitySet(work->bomb_model, 0);
            work->kill = 1;
            break;
        case 2:
            BoardCameraRotGet(&camera_rot);
            PSMTXRotRad(model_mtx, 'x', MTXDegToRad(camera_rot.x + 10.0f));
            BoardModelMtxSet(work->bomb_model, &model_mtx);
            BoardModelRotSet(work->bomb_model, 0.0f, 0.0f, 0.0f);
            screen_pos.x = arg0->scale.x;
            screen_pos.y = arg0->scale.y;
            screen_pos.z = arg0->scale.z;
            Hu3D2Dto3D(&screen_pos, 1, &screen_pos);
            arg0->trans.x = screen_pos.x;
            arg0->trans.y = screen_pos.y;
            arg0->trans.z = screen_pos.z;
            break;
    }
    BoardModelPosSet(work->bomb_model, arg0->trans.x, arg0->trans.y, arg0->trans.z);
}

/**
 * @brief Create the eleven-sprite Battle Space overlay and initialize its state machine.
 */
static void CreateBattleMain(void) {
    s16 sprite;
    s32 resource;
    s32 i;
    BattleWork *work;
    s32 sprite_resources[] = {
        DATA_MAKE_NUM(DATADIR_BBATTLE, 0),
        DATA_MAKE_NUM(DATADIR_BBATTLE, 1),
        DATA_MAKE_NUM(DATADIR_BBATTLE, 2),
        DATA_MAKE_NUM(DATADIR_BBATTLE, 3),
        DATA_MAKE_NUM(DATADIR_BBATTLE, 3),
        DATA_MAKE_NUM(DATADIR_BBATTLE, 4),
        DATA_MAKE_NUM(DATADIR_BBATTLE, 5),
        DATA_MAKE_NUM(DATADIR_BBATTLE, 6),
        DATA_MAKE_NUM(DATADIR_BBATTLE, 7),
        DATA_MAKE_NUM(DATADIR_BBATTLE, 8),
        DATA_MAKE_NUM(DATADIR_BBATTLE, 9)
    };

    battleObj = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, UpdateBattleMain);
    work = omObjGetWork(battleObj, BattleWork);
    work->kill = 0;
    work->coin_display_initialized = 0;
    work->timer = 0;
    work->step_delay = 2;
    work->step_timer = 2;
    work->slowdown_level = 0;
    work->coin_selection_slowing = 0;
    work->state = 0;
    battleCoinSpeed = 1.0f;
    battleCoinPosF = 5.0f * BoardRandFloat();
    coinTakeMax = 0;
    work->sprite_group = HuSprGrpCreate(11);
    for (i = 0; i < 11; i++) {
        resource = sprite_resources[i];
        BoardSpriteCreate(resource, battleSprPrioTbl[i], 0, &sprite);
        HuSprGrpMemberSet(work->sprite_group, i, sprite);
        HuSprAttrSet(work->sprite_group, i, 9);
        HuSprAttrSet(work->sprite_group, i, 4);
    }
    HuSprGrpPosSet(work->sprite_group, 288.0f, 240.0f);
    battleObj->trans.x = -308.0f;
    BoardFilterFadeInit(30, 0xA0);
}

/**
 * @brief Dispatch one frame of the Battle Space overlay state machine.
 *
 * @details Also aligns the bomb presentation with the moving overlay.
 * @param arg0 Overlay object whose transform and BattleWork state are updated.
 */
static void UpdateBattleMain(OMOBJ *arg0) {
    BattleWork *work;
    ExplodeWork *explode_work;
    float bomb_screen_y;

    work = omObjGetWork(arg0, BattleWork);
    if (work->kill != 0 || BoardIsKill()) {
        HuSprGrpKill(work->sprite_group);
        battleObj = NULL;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }
    switch (work->state) {
        case 0:
            DescendBattleBomb(work, arg0);
            break;
        case 1:
            EndBattle(work, arg0);
            break;
        case 2:
            ShowBattleGame(work, arg0);
            break;
        case 3:
            RaiseBattleGame(work, arg0);
            break;
        case 4:
            UpdateBattleCoin(work, arg0);
            break;
        case 5:
            InitBattleGameSpr(work, arg0);
            break;
        case 6:
            VibratePad(work, arg0);
            break;
        case 7:
            ShowBattleCoin(work, arg0);
            break;
        case 8:
            break;
    }
    bomb_screen_y = arg0->trans.x + 240.0f;
    explode_work = omObjGetWork(explodeObj, ExplodeWork);
    if (explode_work->state == 2) {
        explodeObj->scale.x = 288.0f;
        explodeObj->scale.y = bomb_screen_y;
        explodeObj->scale.z = 1000.0f;
    }
    HuSprPosSet(work->sprite_group, 0, 0.0f, arg0->trans.y);
    HuSprScaleSet(work->sprite_group, 0, arg0->trans.z, arg0->trans.z);
}

/**
 * @brief Ease the overlay from off-screen to its opening position.
 * @param arg0 Overlay work state, changed to title reveal when the destination is reached.
 * @param arg1 Overlay object whose x position approaches -132.
 */
static void DescendBattleBomb(BattleWork *arg0, OMOBJ *arg1) {
    float distance = -132.0f - arg1->trans.x;

    if (distance < 0.1f) {
        arg1->trans.x = -132.0f;
        arg0->state = 2;
    } else {
        arg1->trans.x += 0.3f * distance;
    }
}

/**
 * @brief Move the overlay off-screen and finish its fade.
 * @param arg0 Overlay work state, changed to idle after the exit completes.
 * @param arg1 Overlay object whose x position approaches 548.
 */
static void EndBattle(BattleWork *arg0, OMOBJ *arg1) {
    float distance = 548.0f - arg1->trans.x;

    if (distance < 0.1f) {
        BoardFilterFadeOut(30);
        arg0->state = 8;
    } else {
        arg1->trans.x += 0.3f * distance;
    }
}

/**
 * @brief Reveal the Battle Game title with a sine-scaled entrance.
 * @param arg0 Overlay work containing the reveal timer and sprite group.
 * @param arg1 Overlay object supplying the title scale and position.
 */
static void ShowBattleGame(BattleWork *arg0, OMOBJ *arg1) {
    float angle;

    if (arg0->timer == 0) {
        if (BoardFilterFadePauseCheck()) {
            arg0->timer = 1;
            arg1->trans.z = 0.01f;
            arg1->trans.y = 0.0f;
            HuSprAttrReset(arg0->sprite_group, 0, HUSPR_ATTR_DISPOFF);
            HuSprPosSet(arg0->sprite_group, 0, 0.0f, 0.0f);
        } else {
            return;
        }
    }
    if (arg0->timer >= 90) {
        arg0->state = 8;
        arg0->timer = 90;
    }
    OSs8tof32((s8*) &arg0->timer, &angle);
    arg1->trans.z = sind(angle);
    arg0->timer += 4;
}

/**
 * @brief Raise the Battle Game title to its selection position.
 * @param arg0 Overlay work, changed to idle when the title reaches y = -60.
 * @param arg1 Overlay object whose y position is eased upward.
 */
static void RaiseBattleGame(BattleWork *arg0, OMOBJ *arg1) {
    float distance = -60.0f - arg1->trans.y;

    if (ABS(distance) < 0.1f) {
        arg1->trans.y = -60.0f;
        arg0->state = 8;
    } else {
        arg1->trans.y += 0.2f * distance;
    }
}

/**
 * @brief Animate and progressively slow the coin-stake roulette.
 *
 * @details The roulette stops only on the stake index chosen by SetBattleCoinValue.
 * @param arg0 Overlay work containing the roulette timers, slowdown state, and sprite group.
 * @param arg1 Overlay object used when initializing the selector.
 */
static void UpdateBattleCoin(BattleWork *arg0, OMOBJ *arg1) {
    s8 coin_index;

    if (arg0->coin_display_initialized != 0) {
        if (arg0->step_timer++ < arg0->step_delay) {
            arg0->slowdown_level++;
            if (arg0->slowdown_level > 20) {
                arg0->slowdown_level = 20;
            }
            return;
        }
        if (arg0->coin_selection_slowing == 0 && arg0->slowdown_level >= 20 && BoardRandMod(100) > 50) {
            arg0->coin_selection_slowing = 1;
        }
        arg0->step_timer = 0;
        if (arg0->coin_selection_slowing != 0) {
            arg0->step_delay += 4;
            if (arg0->step_delay > 30) {
                arg0->step_delay = 30;
            }
        }
        OSf32tos8(&battleCoinPosF, &coin_index);
        if (arg0->step_delay >= 30 && coin_index == battleCoinIdx) {
            arg0->state = 7;
            arg0->timer = 0;
            coinTakeMax = battleCoinTbl[coin_index];
            HuAudFXPlay(0x305);
        } else {
            battleCoinPosF += 1.0f;
            HuAudFXPlay(0x304);
            if (battleCoinPosF >= 5.0f) {
                battleCoinPosF = 0.0f;
            }
        }
        HuSprBankSet(arg0->sprite_group, 1, coin_index);
        return;
    }
    HuSprAttrReset(arg0->sprite_group, 2, HUSPR_ATTR_DISPOFF);
    HuSprAttrReset(arg0->sprite_group, 1, HUSPR_ATTR_NOANIM | HUSPR_ATTR_DISPOFF);
    HuSprPosSet(arg0->sprite_group, 2, 0.0f, 40.0f);
    HuSprPosSet(arg0->sprite_group, 1, 34.0f, 40.0f);
    HuSprScaleSet(arg0->sprite_group, 1, 0.9f, 0.9f);
    arg0->coin_display_initialized = 1;
    arg0->timer = 0;
    arg1->rot.x = 50.0f;
}

/**
 * @brief Pulse the selected stake sprite for ninety frames.
 * @param arg0 Overlay work containing the display timer and sprite group.
 * @param arg1 Unused overlay object retained by the state-handler interface.
 */
static void ShowBattleCoin(BattleWork *arg0, OMOBJ *arg1) {
    float coin_scale;
    float angle;

    if (arg0->timer >= 90) {
        arg0->timer = 0;
        arg0->state = 8;
        angle = 90.0f;
    } else {
        arg0->timer++;
        OSu8tof32(&arg0->timer, &angle);
    }
    if (coinTakeMax != 50) {
        angle *= 6.0f;
    } else {
        angle *= 10.0f;
    }
    while (angle > 180.0f) {
        angle -= 180.0f;
    }
    coin_scale = 0.9f + sind(angle);
    HuSprScaleSet(arg0->sprite_group, 1, coin_scale, coin_scale);
}

/**
 * @brief Reveal and position the two minigame choices and their cursors.
 * @param arg0 Overlay work containing the candidate sprite indices and group.
 * @param arg1 Unused overlay object retained by the state-handler interface.
 */
static void InitBattleGameSpr(BattleWork *arg0, OMOBJ *arg1) {
    s8 left_minigame;
    s8 right_minigame;
    float cursor_positions[2][2] = {
        { -130.0f, 30.0f },
        {  130.0f, 30.0f }
    };

    left_minigame = battleMGIdx[0];
    right_minigame = battleMGIdx[1];
    HuSprAttrReset(arg0->sprite_group, 3, HUSPR_ATTR_DISPOFF);
    HuSprAttrReset(arg0->sprite_group, 4, HUSPR_ATTR_DISPOFF);
    HuSprAttrReset(arg0->sprite_group, left_minigame + 5, HUSPR_ATTR_DISPOFF);
    HuSprAttrReset(arg0->sprite_group, right_minigame + 5, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(arg0->sprite_group, 3, cursor_positions[0][0], cursor_positions[0][1]);
    HuSprPosSet(arg0->sprite_group, 4, cursor_positions[1][0], cursor_positions[1][1]);
    HuSprPosSet(arg0->sprite_group, left_minigame + 5, cursor_positions[0][0], cursor_positions[0][1]);
    HuSprPosSet(arg0->sprite_group, right_minigame + 5, cursor_positions[1][0], cursor_positions[1][1]);
    arg0->state = 8;
}

/**
 * @brief Hide all eleven sprites in the Battle Space overlay.
 */
static void HideBattleSpr(void) {
    BattleWork *work = omObjGetWork(battleObj, BattleWork);
    s32 i;

    for (i = 0; i < 11; i++) {
        HuSprAttrSet(work->sprite_group, i, 4);
    }
}


/**
 * @brief Start the battle-reveal vibration on all four controller ports.
 * @param arg0 Overlay work returned to idle after vibration starts.
 * @param arg1 Unused overlay object retained by the state-handler interface.
 */
static void VibratePad(BattleWork *arg0, OMOBJ *arg1) {
    s32 i;

    for (i = 0; i < 4; i++) {
        omVibrate(i, 12, 0xC, 0);
    }
    arg0->state = 8;
}

/**
 * @brief Request deletion of the Battle Space overlay object.
 */
static void StopBattleBomb(void) {
    if (battleObj) {
        omObjGetWork(battleObj, BattleWork)->kill = 1;
    }
}

/**
 * @brief Read the current Battle Space overlay state.
 * @return The state number; state 8 is idle and is used by the controlling process as a completion signal.
 */
static s32 GetBattleBombState(void) {
    return omObjGetWork(battleObj, BattleWork)->state;
}

/**
 * @brief Enter a Battle Space overlay state and reset its frame timer.
 * @param arg0 State number from 0 through 8; 0-7 select animation handlers and 8 is idle.
 */
static void SetBattleBombState(s32 arg0) {
    BattleWork *work = omObjGetWork(battleObj, BattleWork);

    work->state = arg0;
    work->timer = 0;
}

static s8 battleChanceTbl[][3][5] = {
    {
        { 10, 85,  5, 0, 0 },
        { 10, 75, 15, 0, 0 },
        { 10, 65, 25, 0, 0 },
    },
    {
        { 5, 70, 20,  5, 0 },
        { 5, 60, 25, 10, 0 },
        { 5, 50, 30, 10, 0 },
    },
    {
        { 5, 60, 20, 10,  5 },
        { 5, 45, 30, 15,  5 },
        { 5, 35, 30, 20, 10 },
    },
    {
        { 5, 45, 30, 15,  5 },
        { 5, 30, 35, 20, 10 },
        { 5, 20, 35, 25, 15 }
    }
};

/**
 * @brief Choose the hidden stake index on which the roulette will stop.
 *
 * @details Uses the richest player's balance, the current third of the game, and battleChanceTbl to select an index into battleCoinTbl.
 */
static void SetBattleCoinValue(void) {
    s32 random_roll;
    s32 turn_phase;
    s32 cumulative_chance;
    s32 coin_band;
    s32 max_coins;
    s32 selected_coin_index;
    s32 i;

    max_coins = -1;
    for (i = 0; i < 4; i++) {
        if (BoardPlayerCoinsGet(i) > max_coins) {
            max_coins = BoardPlayerCoinsGet(i);
        }
    }
    if (max_coins >= 50) {
        coin_band = 3;
    } else if (max_coins >= 30) {
        coin_band = 2;
    } else if (max_coins >= 20) {
        coin_band = 1;
    } else {
        coin_band = 0;
    }
    turn_phase = 3.0f * ((float) GWSystem.turn / GWSystem.max_turn);
    random_roll = BoardRandMod(100);
    selected_coin_index = cumulative_chance = 0;
    for (i = 4; i >= 0; i--) {
        cumulative_chance += battleChanceTbl[coin_band][turn_phase][i];
        if (random_roll < cumulative_chance) {
            selected_coin_index = i;
            break;
        }
    }
    battleCoinIdx = selected_coin_index;
}
