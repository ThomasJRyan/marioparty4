#include "REL/w10Dll.h"
#include "game/data.h"
#include "game/hu3d.h"
#include "game/board/lottery.h"
#include "game/board/model.h"
#include "game/board/shop.h"
#include "game/board/space.h"
#include "game/board/star.h"
#include "game/board/tutorial.h"
#include "game/board/main.h"
#include "game/board/boo_house.h"
#include "game/board/map_object.h"

#include "math.h"

//Map Objects
static BoardMapObject mapObjTbl[MAPOBJ_MAX] = {
    3300.0f, 100.0f, -900.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 1.0f,
    DATA_MAKE_NUM(DATADIR_W10, 3)
};

s16 boardMapObjMdl[MAPOBJ_MAX];

s16 tutorialExitWin;
s16 tutorialDoneF;
static s32 *mapObjectMotionTable;
s32 boardEventState;
W10TutorialBoardWork *tutorialBoardWork;

static s16 bgMdl = -1;
static s16 fgMdl = -1;
s16 boardStarHostMdl = -1;
s16 boardShopHostMdl = -1;
s16 boardLotteryHostMdl = -1;
s16 boardBooHouseHostMdl = -1;

static s32 charMotDirTbl[] = {
	DATADIR_MARIOMOT, DATADIR_LUIGIMOT,
	DATADIR_PEACHMOT, DATADIR_YOSHIMOT,
	DATADIR_WARIOMOT, DATADIR_DONKEYMOT,
	DATADIR_DAISYMOT, DATADIR_WALUIGIMOT,
};

static s32 starHostMot[2] = {
    DATA_MAKE_NUM(DATADIR_W10, 18),
    DATA_NUM_LISTEND
};

static s32 shopHostMot[2] = {
    DATA_MAKE_NUM(DATADIR_BGUEST, 9),
    DATA_NUM_LISTEND
};

static s32 lotteryHostMot[2] = {
    DATA_MAKE_NUM(DATADIR_BGUEST, 24),
    DATA_NUM_LISTEND
};

static s32 booHouseHostMot[2] = {
    DATA_MAKE_NUM(DATADIR_BGUEST, 14),
    DATA_NUM_LISTEND
};

static void LightSetHook(void);
static void LightResetHook(void);
static s32 WalkEvent(void);
static s32 WalkMiniEvent(void);
static s32 LandEvent(void);


void BoardCreate(void)
{
    float windowSize[2];
    
    s32 i;
    BoardMapObject *mapObject;
    s32 space;
    s32 boardIndex;

    boardIndex = GWBoardGet();
    tutorialBoardWork = (W10TutorialBoardWork *)&GWSystem.board_data[0];
    tutorialBoardWork->sceneIndex = 0;
    tutorialBoardWork->rollIndex = 0;
    tutorialBoardWork->messageIndex = 0;
    BoardTutorialHookSet(TutorialHookExec);
    HostCreate();
    TutorialSprCreate();
    tutorialDoneF = 0;
    BoardSpaceInit(DATA_MAKE_NUM(DATADIR_W10, 0));
    space = BoardSpaceFlagSearch(0, 1);
    BoardTutorialBlockSetPos(space, 1);
    bgMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W10, 1), NULL, 0);
    fn_8006DDE8(bgMdl, -1.0f);
    BoardModelPosSet(bgMdl, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(bgMdl, 0, 0x40000001);
    BoardModelMotionSpeedSet(bgMdl, 1.0f);
    fgMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W10, 2), NULL, 0);
    fn_8006DDE8(fgMdl, -1.0f);
    BoardModelPosSet(fgMdl, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(fgMdl, 0, 0x40000001);
    boardStarHostMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W10, 17), starHostMot, 0);
    BoardModelPosSet(boardStarHostMdl, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(boardStarHostMdl, 1, 0x40000001);
    boardShopHostMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BGUEST, 5), shopHostMot, 0);
    BoardModelMotionStart(boardShopHostMdl, 1, 0x40000001);
    boardBooHouseHostMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BGUEST, 18), lotteryHostMot, 0);
    BoardModelMotionStart(boardBooHouseHostMdl, 1, 0x40000001);
    boardLotteryHostMdl = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BGUEST, 13), booHouseHostMot, 0);
    BoardModelMotionStart(boardLotteryHostMdl, 1, 0x40000001);
    BoardLightHookSet(LightSetHook, LightResetHook);
    for (i = 0; i < MAPOBJ_MAX; i++) {
        mapObject = &mapObjTbl[i];
        if (mapObject->data_num != -1) {
            mapObjectMotionTable = NULL;
            boardMapObjMdl[i] = BoardModelCreate(mapObject->data_num, mapObjectMotionTable, 0);
            BoardModelPosSetV(boardMapObjMdl[i], &mapObject->pos);
            BoardModelRotSetV(boardMapObjMdl[i], &mapObject->rot);
            BoardModelScaleSetV(boardMapObjMdl[i], &mapObject->scale);
            BoardModelVisibilitySet(boardMapObjMdl[i], 1);
            if (mapObjectMotionTable != NULL) {
                BoardModelMotionStart(boardMapObjMdl[i], 0, 0x40000001);
            }
        }
    }
    BoardModelVisibilitySet(boardMapObjMdl[MAPOBJ_MELON], 0);
    tutorialBoardWork->focusModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 10), NULL, 0);
    BoardModelVisibilitySet(tutorialBoardWork->focusModel, 0);
    BoardSpaceWalkEventFuncSet(WalkEvent);
    BoardSpaceWalkMiniEventFuncSet(WalkMiniEvent);
    BoardSpaceLandEventFuncSet(LandEvent);
    BoardStarHostSet(boardStarHostMdl);
    BoardBooHouseHostSet(boardBooHouseHostMdl);
    BoardShopHostSet(boardShopHostMdl);
    BoardLotteryHostSet(boardLotteryHostMdl);
    HuWinMesMaxSizeGet(1, windowSize, MAKE_MESSID(0x2E, 0x3A));
    tutorialExitWin = HuWinCreate(-10000.0f, 390.0f, windowSize[0], windowSize[1], 1);
    HuWinMesSet(tutorialExitWin, MAKE_MESSID(0x2E, 0x3A));
    HuWinMesSpeedSet(tutorialExitWin, 0);
    HuWinPriSet(tutorialExitWin, 1);
    HuWinMesPalSet(tutorialExitWin, 7, 0, 0, 0);
}

void BoardDestroy(void)
{
    s32 i;
    for (i = 0; i < 1; i++) {
        if (boardMapObjMdl[i] != 0) {
            BoardModelKill(boardMapObjMdl[i]);
            boardMapObjMdl[i] = 0;
        }   
    }
    if (boardBooHouseHostMdl != -1) {
        BoardModelKill(boardBooHouseHostMdl);
        boardBooHouseHostMdl = -1;
    }
    if (boardShopHostMdl != -1) {
        BoardModelKill(boardShopHostMdl);
        boardShopHostMdl = -1;
    }
    if (boardLotteryHostMdl != -1) {
        BoardModelKill(boardLotteryHostMdl);
        boardLotteryHostMdl = -1;
    }
    if (boardStarHostMdl != -1) {
        BoardModelKill(boardStarHostMdl);
        boardStarHostMdl = -1;
    }
    if (bgMdl != -1) {
        BoardModelKill(bgMdl);
        bgMdl = -1;
    }
    if (fgMdl != -1) {
        BoardModelKill(fgMdl);
        fgMdl = -1;
    }
    BoardModelKill(tutorialBoardWork->focusModel);
    TutorialSprKill();
    HuWinKill(tutorialExitWin);
    BoardSpaceDestroy();
}

static void LightSetHook(void)
{
    s16 id = BoardModelIDGet(bgMdl);
    Hu3DModelLightInfoSet(id, 1);
}

static void LightResetHook(void)
{
	
}

static s32 LandEvent(void)
{
	
}

static s32 WalkEvent(void)
{
    return 0;
}

void TutorialEventInit(void)
{
	
}

void TutorialEventReset(void)
{
	
}

static s32 WalkMiniEvent(void)
{
	
}

s32 BoardModelRotateTowardAngle(s16 modelId, f32 targetAngle, f32 maxAngleStep) {
    s32 reachedTarget;
    f32 targetAngleDifference;
    f32 currentAngle;

    reachedTarget = 0;
    currentAngle = BoardModelRotYGet(modelId);
    targetAngle = fmod(targetAngle, 360.0);
    if (currentAngle < 0.0f) {
        currentAngle += 360.0f;
    }
    if (targetAngle < 0.0f) {
        targetAngle += 360.0f;
    }
    if (currentAngle != targetAngle) {
        targetAngleDifference = targetAngle - currentAngle;
        if (targetAngleDifference < 0.0f) {
            targetAngleDifference += 360.0f;
        }
        if (targetAngleDifference < 180.0f) {
            if (targetAngleDifference > maxAngleStep) {
                currentAngle = currentAngle + maxAngleStep;
            } else {
                currentAngle = targetAngle;
            }
        } else if ((360.0f - targetAngleDifference) > maxAngleStep) {
            currentAngle = currentAngle - maxAngleStep;
        } else {
            currentAngle = targetAngle;
        }
        if (currentAngle < 0.0f) {
            currentAngle += 360.0f;
        }
        if (currentAngle >= 360.0f) {
            currentAngle -= 360.0f;
        }
        BoardModelRotYSet(modelId, currentAngle);
    } else {
        reachedTarget = 1;
    }
    return reachedTarget;
}
