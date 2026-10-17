#include "game/board/model.h"
#include "game/data.h"
#ifndef __MWERKS__
#include "game/board/lottery.h"
#include "game/board/main.h"
#include "game/board/shop.h"
#include "game/board/space.h"
#include "game/board/star.h"
#endif
#include "game/gamework_data.h"
#include "math.h"

#include "REL/w20Dll.h"

s16 boardRuntimeState;
s32 boardEventTimer;
s32 boardStateTimer;
W20BoardData *boardData;

s16 backgroundModelId = 0xFFFF;
s16 secondaryBackgroundModelId = 0xFFFF;
s16 unreferencedModelIdA = 0xFFFF;
s16 shopAndStarHostModelId = 0xFFFF;
s16 lotteryHostModelId = 0xFFFF;
s16 unreferencedModelIdB = 0xFFFF;
s32 playerMotionData[] = {
	DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
	DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00),
	DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
	DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00),
	DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00),
	DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
	DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00),
	DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00),
};

s32 hostMotionTable[] = { DATA_MAKE_NUM(DATADIR_BGUEST, 42), -1 };

void BoardLightSetHook(void);
void BoardLightResetHook(void);
void BoardLandEvent(void);
s32 BoardWalkEvent(void);
void BoardUnusedCallbackA(void);
void BoardUnusedCallbackB(void);
void BoardWalkMiniEvent(void);
s32 RotateModelTowardAngle(s16 modelId, float targetRotationY, float rotationStep);

void BoardCreate(void)
{
    s32 boardId;

    boardId = GWSystem.board;
    boardData = (W20BoardData *)GWSystem.board_data;
    boardData->boardState = 0;
    boardData->boardSubstate = 0;
    boardData->boardEventState = 0;
    BoardSpaceInit(DATA_MAKE_NUM(DATADIR_W20, 0));
    (void)boardId;
    backgroundModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W20, 0x01), NULL, 0);
    fn_8006DDE8(backgroundModelId, -1.0f);
    BoardModelPosSet(backgroundModelId, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(backgroundModelId, 0, 0x40000001);
    BoardModelMotionSpeedSet(backgroundModelId, 1.0f);
    secondaryBackgroundModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W20, 0x02), NULL, 0);
    fn_8006DDE8(secondaryBackgroundModelId, -1.0f);
    BoardModelPosSet(secondaryBackgroundModelId, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(secondaryBackgroundModelId, 0, 0x40000001);
    shopAndStarHostModelId = BoardModelCreate(0x20029, hostMotionTable, 0);
    BoardModelMotionStart(shopAndStarHostModelId, 1, 0x40000001);
    lotteryHostModelId = BoardModelCreate(0x20029, hostMotionTable, 0);
    BoardModelMotionStart(lotteryHostModelId, 1, 0x40000001);
    BoardLightHookSet(BoardLightSetHook, BoardLightResetHook);
    BoardSpaceWalkEventFuncSet(BoardWalkEvent);
    BoardSpaceWalkMiniEventFuncSet(BoardWalkMiniEvent);
    BoardSpaceLandEventFuncSet(BoardLandEvent);
    BoardShopHostSet(shopAndStarHostModelId);
    BoardLotteryHostSet(lotteryHostModelId);
    BoardStarHostSet(shopAndStarHostModelId);
}

void BoardDestroy(void)
{
    if (shopAndStarHostModelId != -1) {
        BoardModelKill(shopAndStarHostModelId);
        shopAndStarHostModelId = -1;
    }
    if (backgroundModelId != -1) {
        BoardModelKill(backgroundModelId);
        backgroundModelId = -1;
    }
    if (secondaryBackgroundModelId != -1) {
        BoardModelKill(secondaryBackgroundModelId);
        secondaryBackgroundModelId = -1;
    }
    BoardSpaceDestroy();
}

void BoardLightSetHook(void)
{
    s16 lightModelId;

    lightModelId = BoardModelIDGet(backgroundModelId);
    Hu3DModelLightInfoSet(lightModelId, 1);
}

void BoardLightResetHook(void) { }

void BoardLandEvent(void) { }

s32 BoardWalkEvent(void) { return 0; }

void BoardUnusedCallbackA(void) { }

void BoardUnusedCallbackB(void) { }

void BoardWalkMiniEvent(void) { }

s32 RotateModelTowardAngle(s16 modelId, float targetRotationY, float rotationStep)
{
    float rotationDifference;
    float currentRotationY;
    s32 rotationComplete;

    rotationComplete = 0;
    currentRotationY = BoardModelRotYGet(modelId);
    targetRotationY = fmod(targetRotationY, 360.0f);
    if (currentRotationY < 0.0f) {
        currentRotationY += 360.0f;
    }
    if (targetRotationY < 0.0f) {
        targetRotationY += 360.0f;
    }
    if (currentRotationY != targetRotationY) {
        rotationDifference = targetRotationY - currentRotationY;
        if (rotationDifference < 0.0f) {
            rotationDifference += 360.0f;
        }
        if (rotationDifference < 180.0f) {
            if (rotationDifference > rotationStep) {
                currentRotationY += rotationStep;
            } else {
                currentRotationY = targetRotationY;
            }
        } else if ((360.0f - rotationDifference) > rotationStep) {
            currentRotationY -= rotationStep;
        } else {
            currentRotationY = targetRotationY;
        }
        if (currentRotationY < 0.0f) {
            currentRotationY += 360.0f;
        }
        if (currentRotationY >= 360.0f) {
            currentRotationY -= 360.0f;
        }
        BoardModelRotYSet(modelId, currentRotationY);
    } else {
        rotationComplete = 1;
    }
    return rotationComplete;
}
