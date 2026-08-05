#include "ext_math.h"
#include "game/audio.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/star.h"
#include "game/board/window.h"
#include "game/flag.h"
#include "game/objsub.h"
#include "game/process.h"

#include "game/board/audio.h"
#include "game/board/tutorial.h"

#define BLOCK_SPAWN 0
#define BLOCK_HIT 2
#define BLOCK_OPEN 3

typedef struct {
    struct {
        u8 kill : 1;
        u8 state : 3;
        u8 returning_to_rest : 1;
        u8 contains_star : 1;
        u8 opened : 1;
        u8 hide_contents : 1;
    };

    s16 hit_y_velocity;
    s16 target_y_pos;
} BlockWork;

typedef struct {
    struct {
        u8 kill : 1;
    };

    u8 lifetime;
    s8 model_index;
    s16 model;
} CoinWork;

static void BlockProc(void);
static void DestroyBlock(void);
static void ExecBlockObj(OMOBJ *arg0);
static void CreateBlockObj(s32 arg0);
static void DestroyBlockObj(BlockWork *unused0, OMOBJ *unused1);
static void SpawnBlock(BlockWork *arg0, OMOBJ *arg1);
static void HitBlock(BlockWork *arg0, OMOBJ *arg1);
static void OpenBlock(BlockWork *arg0, OMOBJ *arg1);
static void SetBlockOpen(void);
static void SetBlockStop(void);
static void WaitBlockHit(void);
static void CreateCoinMdl(void);
static void KillCoinMdl(void);
static void PopupCoin(void);
static void PopupCoinExec(OMOBJ *arg0);

extern s8 boardTutorialBlockF;
extern s8 boardTutorialBlockItem;

static s16 jumpMot = -1;
static s16 starMdl = -1;

static f32 rotMax;
static f32 scaleAngle;
static f32 rotY;

static OMOBJ *blockObj;
static HUPROCESS *blockProc;

static s16 coinMdl[10] = { -1, -1, -1, -1 };
static s32 coinF[10];

/**
 * @brief Resolve a hidden-block event for a normal-sized player.
 *
 * @details Other eligible players wait for a child process that awards either a star or twenty coins.
 * @param arg0 Index of the player who triggered the block.
 * @return Always 0; Mega players return immediately without creating a block.
 */
s32 BoardBlockExec(s32 arg0)
{
    if (BoardPlayerSizeGet(arg0) == 2) {
        return 0;
    }

    blockProc = HuPrcChildCreate(BlockProc, 0x2003, 0x3800U, 0, boardMainProc);
    HuPrcDestructorSet2(blockProc, DestroyBlock);

    while (blockProc != NULL) {
        HuPrcVSleep();
    }

    return 0;
}

/**
 * @brief Drive the hidden block from its entrance through reward delivery.
 *
 * @details Creates the block above the current player and awards either one star or twenty individually animated coins before restoring the camera.
 */
static void BlockProc(void)
{
    s32 jump_motion_data[8] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x60), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x60), DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x60),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x60), DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x60), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x60),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x60), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x60) };
    Vec star_pos;
    s32 unused_value;
    s32 i;
    s32 player;
    BlockWork *work;
    s32 *unused_motion_data;
    s32 player_character;

    player = GWSystem.player_curr;

    player_character = GWPlayer[player].character;
    jumpMot = BoardPlayerMotionCreate(player, jump_motion_data[player_character]);
    BoardPlayerIdleSet(player);
    BoardPlayerMotBlendSet(player, 0, 0xF);
    omVibrate(player, 12, 12, 0);
    CreateBlockObj(player);

    work = omObjGetWork(blockObj, BlockWork);
    while (work->state != BLOCK_HIT) {
        HuPrcVSleep();
    }

    BoardCameraViewSet(3);
    BoardCameraMotionWait();
    BoardWinCreate(0, 0x10000, -1);
    BoardWinWait();
    BoardWinKill();
    BoardPlayerMotionShiftSet(player, 0xB, 0, 8.0, HU3D_MOTATTR_NONE);
    HuPrcSleep(9);
    while (BoardPlayerMotionTimeGet(player) < 30.0f) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x30C);
    SetBlockOpen();
    while (BoardPlayerMotionEndCheck(player) == 0) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(player);

    if (work->contains_star != 0) {
        SetBlockStop();
        BoardModelPosGet(starMdl, &star_pos);
        if (_CheckFlag(FLAG_ID_MAKE(1, 11)) == 0) {
            BoardAudSeqPause(0, 1, 1000);
        }
        BoardStarGive(player, &star_pos);
        if (_CheckFlag(FLAG_ID_MAKE(1, 11)) == 0) {
            BoardAudSeqPause(0, 0, 1000);
        }
    }
    else {
        for (i = 0; i < 0x14; i++) {
            WaitBlockHit();
            BoardPlayerMotionStart(player, (s32)jumpMot, 0);
            BoardPlayerMotionSpeedSet(player, 2);
            while (BoardPlayerMotionTimeGet(player) < 4) {
                HuPrcVSleep();
            }
            SetBlockOpen();
            PopupCoin();
            while (BoardPlayerMotionEndCheck(player) == 0) {
                HuPrcVSleep();
            }
            BoardPlayerMotionStart(player, 1, 0);
            BoardPlayerCoinsAdd(player, 1);
            HuAudFXPlay(7);
            HuPrcVSleep();
        }

        SetBlockStop();
    }

    BoardWinCreate(0, work->contains_star != 0 ? MAKE_MESSID(1, 1) : MAKE_MESSID(1, 2), -1);
    BoardWinWait();
    BoardWinKill();
    KillCoinMdl();
    work->kill = 1;

    BoardPlayerIdleSet((s32)player);
    HuPrcVSleep();

    if ((_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) && work->contains_star == 0) {
        BoardTutorialHookExec(0x16, 0);
    }

    BoardCameraViewSet(2);
    BoardCameraMotionWait();
    HuPrcEnd();
}

/**
 * @brief Release the jump motion and pooled coin models owned by the block process.
 */
static void DestroyBlock(void)
{
    if (jumpMot != -1) {
        BoardPlayerMotionKill(GWSystem.player_curr, jumpMot);
        jumpMot = -1;
    }
    KillCoinMdl();
    blockProc = 0;
}

/**
 * @brief Create and position a hidden block above a player.
 *
 * @details Randomly chooses a star or coin reward unless the tutorial override supplies the reward type.
 * @param arg0 Index of the player beneath the block.
 */
static void CreateBlockObj(s32 arg0)
{
    Vec player_pos;
    BlockWork *work;
    s8 contains_star;

    BoardPlayerPosGet(arg0, &player_pos);
    starMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 4), NULL, 0);
    BoardModelVisibilitySet(starMdl, 0);
    BoardModelMotionSpeedSet(starMdl, 0);
    BoardModelLayerSet(starMdl, 2U);

    blockObj = omAddObjEx(boardObjMan, 0x101, 0U, 0U, -1, ExecBlockObj);

    work = omObjGetWork(blockObj, BlockWork);
    work->kill = 0;
    work->returning_to_rest = 0;
    work->opened = 0;
    work->hide_contents = 0;
    work->state = BLOCK_SPAWN;

    blockObj->trans.x = player_pos.x;
    blockObj->trans.y = 270 + player_pos.y;
    blockObj->trans.z = player_pos.z;

    blockObj->scale.x = blockObj->scale.y = blockObj->scale.z = 0.01;
    blockObj->rot.x = blockObj->rot.y = blockObj->rot.z = 0.0;

    scaleAngle = 0;
    rotY = 0;
    rotMax = 30;

    BoardModelVisibilitySet(starMdl, 1);

    if (BoardRandFloat() > 0.5f) {
        contains_star = 0;
    }
    else {
        contains_star = 1;
    }

    work->contains_star = contains_star;

    if (boardTutorialBlockF != 0) {
        work->contains_star = boardTutorialBlockItem;
        boardTutorialBlockF = 0;
    }

    HuAudFXPlay(0x302);

    if (work->contains_star == 0) {
        CreateCoinMdl();
    }
}

/**
 * @brief Update the hidden block state and synchronize its reward model.
 * @param arg0 Block object whose spawn, bob, or opening animation is processed.
 */
static void ExecBlockObj(OMOBJ *arg0)
{
    BlockWork *work;

    work = omObjGetWork(arg0, BlockWork);

    if (work->kill != 0 || BoardIsKill() != 0) {
        DestroyBlockObj(work, arg0);
        blockObj = 0;
        omDelObjEx(HuPrcCurrentGet(), arg0);
        return;
    }

    switch (work->state) {
        case BLOCK_SPAWN:
            SpawnBlock(work, arg0);
            break;
        case BLOCK_HIT:
            HitBlock(work, arg0);
            break;
        case BLOCK_OPEN:
            OpenBlock(work, arg0);
            break;
    }

    if (work->hide_contents != 0) {
        BoardModelVisibilitySet(starMdl, 0);
    }

    BoardModelPosSet(starMdl, arg0->trans.x, arg0->trans.y, arg0->trans.z);
    BoardModelRotSet(starMdl, arg0->rot.x, arg0->rot.y, arg0->rot.z);
    BoardModelScaleSet(starMdl, arg0->scale.x, arg0->scale.y, arg0->scale.z);
}

/**
 * @brief Destroy the star model owned by the hidden block.
 * @param unused0 Unused BlockWork pointer required by the state-handler signature.
 * @param unused1 Unused block object pointer required by the state-handler signature.
 */
static void DestroyBlockObj(BlockWork *unused0, OMOBJ *unused1)
{
    if (starMdl != -1) {
        BoardModelKill(starMdl);
        starMdl = -1;
    }
}

/**
 * @brief Scale and spin the hidden block into view.
 * @param arg0 Block work changed to the hittable state once the entrance settles.
 * @param arg1 Block object whose rotation and scale are animated.
 */
static void SpawnBlock(BlockWork *arg0, OMOBJ *arg1)
{
    if (scaleAngle < 90.0f) {
        scaleAngle += 3.75f;
        if (scaleAngle > 90.0f) {
            scaleAngle = 90.0f;
        }
    }
    else {
        rotMax *= 0.5f;
        if (rotMax < 0.8f) {
            rotY = 0.0f;
            rotMax = 0.0f;
            arg0->hit_y_velocity = 0;
            arg0->state = BLOCK_HIT;
        }
    }

    if (rotY < 360.0f) {
        rotY += rotMax;
        if (rotY > 360.0f) {
            rotY = 360.0f;
        }
    }

    arg1->rot.y = rotY;
    arg1->scale.x = arg1->scale.y = arg1->scale.z = sind(scaleAngle);
}

/**
 * @brief Apply the block's continuous vertical bob.
 * @param arg0 Block work containing the wrapped bobbing angle.
 * @param arg1 Block object whose y position is offset each frame.
 */
static void HitBlock(BlockWork *arg0, OMOBJ *arg1)
{
    float angle;

    arg0->hit_y_velocity += 3;
    if (arg0->hit_y_velocity > 0x168) {
        arg0->hit_y_velocity -= 0x168;
    }

    OSs16tof32(&arg0->hit_y_velocity, &angle);
    arg1->trans.y += 0.2f * sind(angle);
}

/**
 * @brief Bounce the block upward once, then return it to its saved height.
 *
 * @details The first update starts the reward-model motion.
 * @param arg0 Block work containing the resting height and opening flags.
 * @param arg1 Block object whose y position is animated.
 */
static void OpenBlock(BlockWork *arg0, OMOBJ *arg1)
{
    f32 target_y_pos;

    if (arg0->opened == 0) {
        arg0->opened = 1;
        HuAudFXPlay(0x33B);
        BoardModelMotionStart(starMdl, 0, 0);
    }

    if (arg0->returning_to_rest == 0) {
        OSf32tos16(&arg1->trans.y, &arg0->target_y_pos);

        arg1->trans.y += 80.0f;
        arg0->returning_to_rest = 1;
    }
    else {
        OSs16tof32(&arg0->target_y_pos, &target_y_pos);
        arg1->trans.y += -8.0f;

        if (arg1->trans.y < target_y_pos) {
            arg1->trans.y = target_y_pos;
            arg0->state = BLOCK_HIT;
        }
    }
}

/**
 * @brief Put the active block into its opening state.
 */
static void SetBlockOpen(void)
{
    BlockWork *work;

    work = omObjGetWork(blockObj, BlockWork);
    work->state = BLOCK_OPEN;
}

/**
 * @brief Hide the active block's reward model after payout.
 */
static void SetBlockStop(void)
{
    BlockWork *work;

    work = omObjGetWork(blockObj, BlockWork);
    work->hide_contents = 1;
}

/**
 * @brief Wait until the active block returns to its hittable state.
 */
static void WaitBlockHit(void)
{
    BlockWork *work;

    work = omObjGetWork(blockObj, BlockWork);

    while (work->state != BLOCK_HIT) {
        HuPrcVSleep();
    }
}

/**
 * @brief Create the ten-model pool used for popup coins.
 */
static void CreateCoinMdl(void)
{
    s32 i;

    for (i = 0; i < 10; i += 1) {
        coinMdl[i] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 10), NULL, 1);
        BoardModelVisibilitySet(coinMdl[i], 0);
        coinF[i] = 0;
    }
}

/**
 * @brief Destroy every pooled coin model and mark every slot free.
 */
static void KillCoinMdl(void)
{
    s32 i;

    for (i = 0; i < 0xA; i += 1) {
        if (coinMdl[i] != -1) {
            BoardModelKill(coinMdl[i]);
            coinMdl[i] = -1;
        }
        coinF[i] = 0;
    }
}

/**
 * @brief Find the first free popup-coin pool slot.
 * @param out_model Receives the model handle stored in the free slot.
 * @return The free slot index, or -1 when all ten slots are occupied.
 */
static inline s32 FindCoinModel(s16 *out_model)
{
    s32 i;

    for (i = 0; i < 10; i++) {
        if (coinF[i] == 0) {
            *out_model = coinMdl[i];
            return i;
        }
    }

    return -1;
}

/**
 * @brief Launch a ten-frame coin popup from the block.
 *
 * @details Waits for a free pooled model and makes it visible at the reward-model position.
 */
static void PopupCoin(void)
{
    Vec star_pos;
    OMOBJ *coinObj;
    s32 model_index;
    s16 model;
    CoinWork *work;

    do {
        HuPrcVSleep();
        model_index = FindCoinModel(&model);
    } while (model_index == -1);

    coinObj = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, PopupCoinExec);
    work = omObjGetWork(coinObj, CoinWork);

    work->kill = 0;
    work->lifetime = 10;
    work->model = model;
    work->model_index = model_index;

    coinObj->trans.x = 15.0f;
    coinObj->trans.y = 40.0f;

    BoardModelPosGet(starMdl, &star_pos);
    BoardModelPosSetV(work->model, &star_pos);
    BoardModelVisibilitySet(work->model, 1);
}

/**
 * @brief Move and spin one popup coin until its lifetime expires.
 *
 * @details Cleanup hides the model and releases its pool slot.
 * @param obj Popup object containing the pooled model handle, pool index, and remaining lifetime.
 */
static void PopupCoinExec(OMOBJ *obj)
{
    Vec coin_pos;
    Vec coin_rot;
    CoinWork *work;

    work = omObjGetWork(obj, CoinWork);

    if (work->kill != 0 || BoardIsKill() != 0) {
        BoardModelVisibilitySet(work->model, 0);
        coinF[work->model_index] = 0;
        omDelObjEx(HuPrcCurrentGet(), obj);
        return;
    }

    if (work->lifetime != 0) {
        work->lifetime -= 1;
    }
    else {
        work->kill = 1;
        return;
    }

    BoardModelPosGet(work->model, &coin_pos);
    coin_pos.y += obj->trans.y;
    BoardModelPosSetV(work->model, &coin_pos);
    BoardModelRotGet(work->model, &coin_rot);
    coin_rot.y += obj->trans.x;

    if (coin_rot.y > 360.0f) {
        coin_rot.y -= 360.0f;
    }

    BoardModelRotSetV(work->model, &coin_rot);
}
