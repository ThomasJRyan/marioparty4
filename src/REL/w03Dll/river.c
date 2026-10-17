#include "REL/w03Dll.h"
#include "dolphin.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/hu3d.h"
#include "game/object.h"

#include "game/board/window.h"
#include "game/window.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/board/com.h"
#include "game/board/view.h"
#endif

#include "ext_math.h"

// BSS
char riverRewardText[8];
OMOBJ *riverRaftObject;
s16 riverEventSucceeded;
s32 riverEventState;
HUPROCESS *riverEventProcess;

// function signatures
void RiverRunPlayerEvent(void);
void RiverEventProcessCleanup(void);
void RiverRaftUpdate(OMOBJ *object);
s32 RiverCheckPlayerChoice(s32 playerIndex);
void RiverAnimatePlayerRide(s32 value);
void RiverMoveRaft(s32 playerIndex);
void RiverRaftUpdate(OMOBJ *object);
void RiverFinishPlayerRide(s32 playerIndex);
void RiverCheckStarPosition(s32 value);

typedef struct RiverRaftWork {
    struct {
        u8 shouldStop : 1;
    };
    s8 unreferencedRiverData[3];
    f32 rotation;
} RiverRaftWork;

// data
s32 riverPlayerMotionIds[] = { 0x00000105, 0x00000145, 0x00000185, 0x000001C5, 0x00000205, 0x00000245, 0x00000285, 0x000002C5 };

s16 riverModelId = -1;
s16 riverRaftModelId = -1;
s16 riverWaterModelId = -1;
s32 riverRaftMotionData[] = { DATA_MAKE_NUM(DATADIR_W03, 0x26), DATA_MAKE_NUM(DATADIR_W03, 0x27), DATA_MAKE_NUM(DATADIR_W03, 0x28), DATA_NUM_LISTEND };

void RiverSetWaterHeight(s32 value)
{
    Vec modelPosition;
    RiverRaftWork *work = omObjGetWork(riverRaftObject, RiverRaftWork);

    work->rotation = 0.0f;

    BoardModelPosGet(riverModelId, &modelPosition);
    if (value != 0) {
        modelPosition.y = -185.0f;
    }
    else {
        modelPosition.y = 10.0f;
    }

    BoardModelPosSetV(riverModelId, &modelPosition);
}

void RiverCreateRaft(void)
{
    Vec modelPosition;
    Vec spacePosition;
    RiverRaftWork *work;

    riverModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x0E), NULL, 0);
    riverWaterModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x0F), NULL, 0);
    riverRaftModelId = BoardModelCreate(0x2000D, &riverRaftMotionData[0], 0);
    BoardModelMotionStart(riverRaftModelId, 1, 0x40000001);
    BoardModelHookSet(riverRaftModelId, "itemhook_R", riverWaterModelId);
    BoardModelHookSet(riverModelId, "hei01", riverRaftModelId);
    Hu3DModelObjPosGet(BoardModelIDGet(boardModelIds[0]), "ikada", &modelPosition);
    BoardSpaceFlagPosGet(0, 0x200, &spacePosition);
    modelPosition.x = spacePosition.x + (400.0 * sin(-(M_PI / 2)));
    modelPosition.z = spacePosition.z + (400.0 * cos(-(M_PI / 2)));
    BoardModelPosSetV(riverModelId, &modelPosition);
    riverRaftObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, (void *)RiverRaftUpdate);
    work = omObjGetWork(riverRaftObject, RiverRaftWork);
    work->shouldStop = 0;
    work->rotation = 0.0f;
    riverEventState = 0;
}

s32 RiverResetRaftPosition(void)
{
    Vec modelPosition;
    Vec spacePosition;

    Hu3DModelObjPosGet(BoardModelIDGet(boardModelIds[0]), "ikada", &modelPosition);
    BoardSpaceFlagPosGet(0, 0x200, &spacePosition);
    modelPosition.x = spacePosition.x + (400.0 * sin(-(M_PI / 2)));
    modelPosition.z = spacePosition.z + (400.0 * cos(-(M_PI / 2)));
    BoardModelPosSetV(riverModelId, (Vec *)&modelPosition);
    return 1;
}

s16 RiverRunBoardEvent(void)
{
    BoardRollDispSet(0);
    riverEventProcess = HuPrcChildCreate(RiverRunPlayerEvent, 0x2003, 0x2000, 0, boardMainProc);
    HuPrcDestructorSet2(riverEventProcess, RiverEventProcessCleanup);

    while (riverEventProcess != 0) {
        HuPrcVSleep();
    }

    BoardRollDispSet(1);
    return riverEventSucceeded;
}

void RiverRunPlayerEvent(void)
{
    s32 currentPlayerIndex;

    currentPlayerIndex = GWSystem.player_curr;
    riverEventSucceeded = 0;

    if (RiverCheckPlayerChoice(currentPlayerIndex) != 0) {
        RiverMoveRaft(currentPlayerIndex);
        RiverAnimatePlayerRide(currentPlayerIndex);
        RiverFinishPlayerRide(currentPlayerIndex);
        riverEventSucceeded = 1;
    }

    BoardCameraViewSet(1);
    BoardCameraMotionWait();
    HuPrcEnd();
}

void RiverEventProcessCleanup(void)
{
    riverEventProcess = 0;
}

typedef struct CoinRewardRange {
    s32 x;
    s32 y;
    s32 z;
} CoinRewardRange;

CoinRewardRange coinRewardRanges[6] = {
    { 5, 20, 5 },
    { 21, 35, 7 },
    { 31, 50, 10 },
    { 51, 80, 15 },
    { 81, 100, 17 },
    { 101, 999, 20 },
};

s32 RiverCheckPlayerChoice(s32 playerIndex)
{
    Vec modelPosition;
    Vec playerPosition;
    Vec positionDelta;
    f32 headingAngle;
    s32 index;
    s32 coinCount;
    s32 i, j;
    s32 minimumCoinCount, maximumCoinCount;

    BoardPlayerIdleSet(playerIndex);
    BoardCameraViewSet(2);
    BoardCameraMotionWait();

    Hu3DModelObjPosGet(BoardModelIDGet(riverModelId), "hei01", &modelPosition);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    VECSubtract(&modelPosition, &playerPosition, &positionDelta);
    headingAngle = atan2d(positionDelta.x, positionDelta.z);
    BoardCameraTargetModelSet(riverModelId);
    BoardPlayerMotBlendSet(playerIndex, headingAngle, 15);

    while (BoardPlayerMotBlendCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }

    BoardCameraTargetModelSet(-1);
    BoardWinCreate(2, MAKE_MESSID(22, 9), 2);
    BoardWinWait();
    coinCount = BoardPlayerCoinsGet(playerIndex);

    if (coinCount < 5) {
        BoardWinCreate(2, MAKE_MESSID(22, 10), 2);
        BoardWinWait();
        BoardWinKill();
        return 0;
    }

    for (index = i = 0; i < ARRAY_COUNT(coinRewardRanges); i++) {
        minimumCoinCount = coinRewardRanges[i].x;
        maximumCoinCount = coinRewardRanges[i].y;
        if (coinCount >= minimumCoinCount) {
            if (coinCount <= maximumCoinCount) {
                index = coinRewardRanges[i].z;
                break;
            }
        }
    }

    sprintf(riverRewardText, "%d", index);

    j = 0;
    while (j == 0) {
        BoardWinCreateChoice(2, MAKE_MESSID(22, 11), 2, 0);
        BoardWinInsertMesSet(MAKE_MESSID_PTR(riverRewardText), 2);
        BoardWinAttrSet(0x10);

        if (GWPlayer[playerIndex].com != 0) {
            RiverCheckStarPosition(playerIndex);
        }

        BoardWinWait();
        BoardWinKill();

        switch (BoardWinChoiceGet()) {
            case 0:
                j = 1;
                break;
            case 1:
            case -1:
                BoardWinCreate(2, MAKE_MESSID(22, 14), 2);
                BoardWinWait();
                BoardWinKill();
                return 0;
            default:
                BoardViewMapExec(playerIndex);
        }
    }

    for (i = 0; i < index; i++) {
        BoardPlayerCoinsAdd(playerIndex, -1);
        HuAudFXPlay(0xE);
        HuPrcSleep(6);
    }

    HuAudFXPlay(0xF);
    BoardWinCreate(2, MAKE_MESSID(22, 12), 2);
    BoardWinWait();
    BoardWinKill();
    return 1;
}

s32 BoardDAngleCalcRange(float *value, float min, float range);

void RiverAnimatePlayerRide(s32 value)
{
    Vec spacePosition;
    Vec modelPosition;
    Vec position;
    Vec positionDelta;
    f32 floatValue;
    f32 animationTime;
    f32 floatValue2;
    s16 stateValue;
    s16 i;

    BoardModelMotionShiftSet(riverRaftModelId, 2, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    floatValue = BoardModelRotYGet(riverRaftModelId);
    floatValue2 = floatValue + 180.0f;

    while (BoardDAngleCalcRange(&floatValue, floatValue2, 3.0f) == 0) {
        BoardModelRotYSet(riverRaftModelId, floatValue);
        HuPrcVSleep();
    }

    while (BoardModelMotionEndCheck(riverRaftModelId) == 0) {
        HuPrcVSleep();
    }

    BoardModelPosGet(riverModelId, &modelPosition);
    BoardSpaceFlagPosGet(0, 0x100, &spacePosition);
    position.x = spacePosition.x + (400.0 * sin(M_PI / 2));
    position.z = spacePosition.z + (400.0 * cos(M_PI / 2));
    position.y = modelPosition.y;
    VECSubtract(&position, &modelPosition, &positionDelta);
    positionDelta.x = positionDelta.x / 120.0f;
    positionDelta.y /= 120.0f;
    positionDelta.z = positionDelta.z / 120.0f;
    BoardCameraTargetModelSet(riverModelId);
    BoardModelMotionShiftSet(riverRaftModelId, 3, 0.0f, 4.0f, HU3D_MOTATTR_LOOP);
    HuPrcSleep(4);

    stateValue = 0;
    for (i = 0; i < 0x78U; i++) {
        VECAdd(&positionDelta, &modelPosition, &modelPosition);
        animationTime = BoardModelMotionTimeGet(riverRaftModelId);

        if ((animationTime >= 5.0f) && (animationTime < 15.0f) && ((s16)stateValue == 0)) {
            HuAudFXPlay(0x451);
            stateValue = 1;
        }

        if ((BoardModelMotionTimeGet(riverRaftModelId) >= 21.0f) && ((s16)stateValue != 0)) {
            stateValue = 0;
        }

        BoardModelPosSetV(riverModelId, &modelPosition);
        HuPrcVSleep();
    }

    BoardModelPosSetV(riverModelId, &position);
    BoardCameraTargetModelSet(-1);
    BoardModelMotionShiftSet(riverRaftModelId, 2, 0.0f, 10.0f, HU3D_MOTATTR_REV);
    floatValue = BoardModelRotYGet(riverRaftModelId);
    floatValue2 = 0.0f;

    while (BoardDAngleCalcRange(&floatValue, floatValue2, 3.0f) == 0) {
        BoardModelRotYSet(riverRaftModelId, floatValue);
        HuPrcVSleep();
    }

    while (BoardModelMotionEndCheck(riverRaftModelId) == 0) {
        HuPrcVSleep();
    }

    BoardModelMotionShiftSet(riverRaftModelId, 1, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
}

extern s32 riverPlayerMotionIds[];
// extern W03BoardState* boardState;

void RiverMoveRaft(s32 playerIndex)
{
    Vec modelPosition;
    Vec playerPosition;
    Vec positionDelta;
    f32 floatValue;
    f32 floatValue2;
    f32 floatValue3;
    s32 integerValue;
    s32 hasLanded;
    s16 model;

    BoardPlayerPosGet(playerIndex, &playerPosition);
    Hu3DModelObjPosGet(BoardModelIDGet(riverModelId), "chara01", &modelPosition);
    VECSubtract(&modelPosition, &playerPosition, &positionDelta);

    positionDelta.x /= 60.0f;
    positionDelta.z /= 60.0f;
    positionDelta.y = 0.0f;

    if (boardState->statueRewardCount != 0) {
        floatValue = 10.0f;
        floatValue2 = 0.21f;
        playerPosition.y += modelPosition.y;
    }
    else {
        floatValue = 0.0f;
        floatValue2 = 0.15f;
    }

    riverEventState = 1;
    HuAudFXPlay(riverPlayerMotionIds[GWPlayer[playerIndex].character]);
    BoardPlayerMotionShiftSet(playerIndex, 4, 0.0f, 10.0f, HU3D_MOTATTR_NONE);

    for (hasLanded = 0, floatValue3 = 0.0f; floatValue3 < 60.0f; floatValue3 += 1.0f) {
        VECAdd(&playerPosition, &positionDelta, &playerPosition);
        playerPosition.y += 10.0f + floatValue + (floatValue2 * (-0.08166667f * floatValue3 * floatValue3));
        if (playerPosition.y <= modelPosition.y) {
            playerPosition.y = modelPosition.y;
            if (hasLanded == 0) {
                BoardPlayerMotionShiftSet(playerIndex, 5, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
                hasLanded = 1;
            }
        }
        BoardPlayerPosSetV(playerIndex, &playerPosition);
        HuPrcVSleep();
    }

    BoardPlayerMotionShiftSet(playerIndex, 5, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(10);
    BoardPlayerIdleSet(playerIndex);

    if (GWPlayer[playerIndex].bowser_suit != 0) {
        BoardModelHookSet(riverModelId, "chara01", BoardPlayerModelGet(playerIndex));
    }
    else {
        BoardModelHookSet(riverModelId, "chara01", BoardPlayerModelGet(playerIndex));
    }

    BoardPlayerPosSet(playerIndex, 0.0f, 0.0f, 0.0f);
    riverEventState = 0;
}

void RiverFinishPlayerRide(s32 playerIndex)
{
    Vec spacePosition;
    Vec modelPosition;
    Vec positionDelta;
    f32 floatValue;
    f32 floatValue2;
    f32 floatValue3;
    s32 integerValue;
    s32 integerValue2;
    s32 hasLanded;

    integerValue = BoardSpaceFlagPosGet(0, 0x100, &spacePosition);
    Hu3DModelObjPosGet(BoardModelIDGet(riverModelId), "chara01", &modelPosition);
    BoardPlayerPosSetV(playerIndex, &modelPosition);
    BoardModelHookObjReset(riverModelId, "chara01");
    VECSubtract(&spacePosition, &modelPosition, &positionDelta);

    positionDelta.x = positionDelta.x / 60.0f;
    positionDelta.z = positionDelta.z / 60.0f;
    positionDelta.y = 0.0f;

    HuAudFXPlay(riverPlayerMotionIds[GWPlayer[playerIndex].character]);
    BoardPlayerMotionShiftSet(playerIndex, 4, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    if (boardState->statueRewardCount != 0) {
        floatValue2 = 0.25f;
    }
    else {
        floatValue2 = 0.2f;
    }

    for (hasLanded = 0, floatValue3 = 0.0f; floatValue3 < 60.0f; floatValue3 += 1.0f) {
        VECAdd(&modelPosition, &positionDelta, &modelPosition);
        modelPosition.y += 20.0f + (floatValue2 * (-0.08166667f * floatValue3 * floatValue3));
        if (modelPosition.y <= spacePosition.y && floatValue3 > 48.0f) {
            modelPosition.y = spacePosition.y;
            if (hasLanded == 0) {
                BoardPlayerMotionShiftSet(playerIndex, 5, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
                hasLanded = 1;
            }
        }
        BoardPlayerPosSetV(playerIndex, &modelPosition);
        HuPrcVSleep();
    }

    BoardPlayerPosSetV(playerIndex, &spacePosition);
    HuPrcSleep(0xA);
    BoardPlayerMotBlendSet(playerIndex, 0, 15);

    while (BoardPlayerMotBlendCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }

    BoardPlayerIdleSet(playerIndex);
    GWPlayer[playerIndex].space_curr = integerValue;
    GWPlayer[playerIndex].space_prev = -1;
    BoardPlayerPostTurnHookSet(playerIndex, RiverResetRaftPosition);
    BoardWinCreate(2, MAKE_MESSID(22, 13), 2);
    BoardWinWait();
    BoardWinKill();
}

void RiverRaftUpdate(OMOBJ *object)
{
    Vec modelPosition;
    s32 integerValue;
    RiverRaftWork *work;

    work = omObjGetWork(object, RiverRaftWork);

    if ((work->shouldStop != 0) || (BoardIsKill() != 0)) {
        riverRaftObject = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }

    if ((riverModelId != -1) && (riverEventState == 0)) {
        work->rotation += 3.0f;
        if (work->rotation >= 360.0f) {
            work->rotation -= 360.0f;
        }
        BoardModelPosGet(riverModelId, &modelPosition);
        modelPosition.y += 0.25 * (2.0 * sind(work->rotation));
        BoardModelPosSetV(riverModelId, &modelPosition);
    }
}

void RiverCheckStarPosition(s32 value)
{
    s32 boardIndex;
    s32 stateValue = 0;

    boardIndex = GWSystem.star_pos;

    if ((boardIndex == 1) || (boardIndex == 2) || (boardIndex == 3) || (boardIndex == 4)) {
        stateValue = 1;
    }

    if (stateValue != 0) {
        BoardComKeySetLeft();
        return;
    }

    BoardComKeySetRight();
}
