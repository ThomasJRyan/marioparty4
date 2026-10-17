#include "REL/w03Dll.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/window.h"
#include "game/data.h"

#include "game/hu3d.h"

#include "game/pad.h"
#include "game/wipe.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/board/audio.h"
#include "game/board/com.h"
#include "game/board/view.h"
#include "game/chrman.h"
#include "game/hsfex.h"
#include "game/objsub.h"
#endif

#include "ext_math.h"
#include <string.h>

// structs

typedef struct CondorMotionWork {
    /* 0x00 */ struct {
        u8 isActive : 1;
        u8 condorState : 2;
        u8 isMoving : 1;
        u8 movementFlags : 4;
    };
    /* 0x01 */ char unreferencedCondorData[3];
    /* 0x04 */ f32 positionX;
    /* 0x08 */ f32 positionY;
} CondorMotionWork;

typedef struct CondorProcessWork {
    /* 0x00 */ struct {
        u8 isActive : 1;
    };
} CondorProcessWork;

typedef struct CondorPlayerPositionWork {
    /* 0x00 */ s16 playerIndex;
    /* 0x02 */ s16 model;
    /* 0x04 */ f32 state;
    /* 0x08 */ Vec startPosition;
    /* 0x14 */ Vec targetPosition;
} CondorPlayerPositionWork;

typedef struct CondorPlayerWork {
    /* 0x00 */ struct {
        u8 isActive : 1;
        u8 state : 2;
        u8 isMoving : 1;
        u8 isVisible : 1;
    };
    s8 playerIndex;
    s8 timer;
    u8 animationState;
    s16 model;
    char unreferencedCondorData[2];
    CondorPlayerPositionWork *playerWork;
} CondorPlayerWork;

// function signatures
void CondorEventStart(void);
void CondorEventProcessCleanup(void);
void CondorPositionAtSpace(void);
s32 CondorOfferPurchase(s32);
void CondorReturnPlayer(s32);
void CondorPlayPurchaseAnimation(s32);
void CondorAnimatePlayerRide(s32);
void CondorMoveToOtherSpace(s32);
void CondorCheckPlayerEligibility(s32);
void CondorSetupPlayerWork(s32);
void CondorWorkCleanup(void);
void CondorUpdate(OMOBJ *object);
void CondorMoveToDestination(s32);
void CondorMotionUpdate(OMOBJ *object);
void CondorFlyToTarget(OMOBJ *object, CondorMotionWork *motionWork);
void CondorLandAtTarget(OMOBJ *object, CondorMotionWork *motionWork);
f32 CondorDistanceToTarget(OMOBJ *object, CondorMotionWork *motionWork);
void CondorStateSet(s8 value);
s32 CondorCheckCompletion(void);
void CondorCleanup(void);

// bss
float condorPlayerPositions[20][2];
s8 condorDestinationSide;
s8 condorEventSucceeded;
OMOBJ *condorMotionObject;
OMOBJ *condorMovementObject;
HUPROCESS *condorEventProcess;

// data
s16 condorModelId = -1;
s16 condorPlayerMotionId = -1;
s16 condorCartModelId = -1;
s16 condorCartMotionId = -1;
s32 condorCartMotionData[] = { DATA_MAKE_NUM(DATADIR_W03, 0x0016), DATA_MAKE_NUM(DATADIR_W03, 0x0017), DATA_MAKE_NUM(DATADIR_W03, 0x0018),
    DATA_MAKE_NUM(DATADIR_W03, 0x0019), DATA_MAKE_NUM(DATADIR_W03, 0x001A), DATA_NUM_LISTEND };

void CondorCreate(void)
{
    condorModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x15), condorCartMotionData, 0);
    CondorPositionAtSpace();
}

s32 CondorCheckPlayerChoice(void)
{
    BoardRollDispSet(0);
    condorEventProcess = HuPrcChildCreate(CondorEventStart, 0x2003U, 0x2000U, 0, boardMainProc);
    HuPrcDestructorSet2(condorEventProcess, CondorEventProcessCleanup);

    while (condorEventProcess) {
        HuPrcVSleep();
    }

    BoardRollDispSet(1);
    return condorEventSucceeded;
}

void CondorEventStart(void)
{
    s32 currentPlayerIndex;

    currentPlayerIndex = GWSystem.player_curr;
    condorEventSucceeded = 0;
    boardState->eventActive = 1;

    if (CondorOfferPurchase(currentPlayerIndex) != 0) {
        BoardCameraNearFarSet(500.0f, 20000.0f);
        BoardStatusShowSetAll(0);
        CondorPlayPurchaseAnimation(currentPlayerIndex);
        CondorAnimatePlayerRide(currentPlayerIndex);
        CondorMoveToOtherSpace(currentPlayerIndex);
        condorEventSucceeded = 1;
    }

    BoardCameraNearFarSet(100.0f, 13000.0f);
    CondorReturnPlayer(currentPlayerIndex);
    HuPrcEnd();
}

void CondorEventProcessCleanup(void)
{
    if (condorCartMotionId != -1) {
        HuWinKill(condorCartMotionId);
        condorCartMotionId = -1;
    }

    if (condorCartModelId != -1) {
        BoardModelKill(condorCartModelId);
        condorCartModelId = -1;
    }

    if (condorPlayerMotionId != -1) {
        BoardPlayerMotionKill(GWSystem.player_curr, condorPlayerMotionId);
        condorPlayerMotionId = -1;
    }

    boardState->eventActive = 0;
    condorEventProcess = 0;
}

void CondorPositionAtSpace(void)
{
    Vec spacePosition;
    f32 floatValue;
    s32 linkedSpaceIndex;

    if (boardState->condorSide != 0) {
        linkedSpaceIndex = BoardSpaceFlagSearch(0, 0x4000);
    }
    else {
        linkedSpaceIndex = BoardSpaceFlagSearch(0, 0x8000);
    }

    linkedSpaceIndex = BoardSpaceLinkFlagSearch(0, linkedSpaceIndex, 0x02000000);
    BoardSpacePosGet(0, linkedSpaceIndex, &spacePosition);
    floatValue = 0.0f;
    spacePosition.x += -300.0 * sind(floatValue);
    spacePosition.z += -300.0 * cosd(floatValue);
    spacePosition.y = 45.0f + spacePosition.y;
    BoardModelPosSetV(condorModelId, &spacePosition);
    BoardModelMotionStart(condorModelId, 2, 0x40000001);
}

s32 CondorOfferPurchase(s32 playerIndex)
{
    Vec spacePosition;
    Vec playerPosition;
    Vec positionDelta;
    f32 floatValue;
    f32 floatValue2;
    Vec cameraRotation;
    Vec cameraRotation2;
    s32 currentSpaceIndex;
    s32 integerValue;
    s32 stateValue;
    s32 integerValue2;
    s32 linkedSpaceIndex;
    s32 integerValue3;
    f32 floatTemp;
    s32 i, j;

    cameraRotation.x = 0.0f;
    cameraRotation.y = 150.0f;
    cameraRotation.z = 0.0f;

    cameraRotation2.x = -15.0f;
    cameraRotation2.y = 0.0f;
    cameraRotation2.z = 0.0f;

    BoardCameraMotionStartEx(condorModelId, &cameraRotation2, &cameraRotation, 1900.0f, -1.0f, 21);
    currentSpaceIndex = GWPlayer[playerIndex].space_curr;
    BoardPlayerPosGet(playerIndex, &playerPosition);
    linkedSpaceIndex = BoardSpaceLinkFlagSearch(0, currentSpaceIndex, 0x02000000);
    BoardSpacePosGet(0, linkedSpaceIndex, &spacePosition);
    VECSubtract(&spacePosition, &playerPosition, &positionDelta);
    floatTemp = atan2d(positionDelta.x, positionDelta.z);
    BoardPlayerMotBlendSet(playerIndex, (floatTemp), 0xF);

    while (BoardPlayerMotBlendCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }

    BoardPlayerIdleSet(playerIndex);
    HuAudFXPlay(0x447);
    BoardModelMotionShiftSet(condorModelId, 4, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(6);

    while (BoardModelMotionEndCheck(condorModelId) == 0) {
        HuPrcVSleep();
    }

    BoardModelMotionShiftSet(condorModelId, 1, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    integerValue3 = BoardPlayerGetCharMess(playerIndex);
    BoardWinCreate(2, MAKE_MESSID(22, 16), 8);
    BoardWinInsertMesSet(integerValue3, 3);
    BoardWinWait();

    if (BoardPlayerCoinsGet(playerIndex) < 5) {
        BoardWinCreate(2, MAKE_MESSID(22, 19), 8);
        BoardWinWait();
        BoardWinKill();
        return 0;
    }

    stateValue = 0;
    while (stateValue == 0) {
        BoardWinCreateChoice(2, MAKE_MESSID(22, 17), 8, 0);
        BoardWinAttrSet(0x10);

        if (GWPlayer[playerIndex].com != 0) {
            CondorCheckPlayerEligibility(playerIndex);
        }

        BoardWinWait();
        BoardWinKill();
        switch (BoardWinChoiceGet()) {
            case 0:
                stateValue = 1;
                break;
            case 2:
                BoardViewMapExec(playerIndex);
                break;
            default:
            case 1:
                BoardWinCreate(2, MAKE_MESSID(22, 20), 8);
                BoardWinWait();
                BoardWinKill();
                return 0;
        }
    }

    BoardAudSeqPause(0, 1, 1000);

    for (j = 0; j < 5; j++) {
        BoardPlayerCoinsAdd(playerIndex, -1);
        HuAudFXPlay(14);
        HuPrcSleep(6);
    }

    HuAudFXPlay(15);
    BoardWinKill();
    return 1;
}

void CondorReturnPlayer(s32 playerIndex)
{
    Vec spacePosition;
    Vec playerPosition;
    f32 value;

    BoardModelMotionShiftSet(condorModelId, 2, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    value = (180.0f + BoardPlayerRotYGet(playerIndex));
    BoardPlayerMotBlendSet(playerIndex, value, 0xF);

    while (BoardPlayerMotBlendCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }

    BoardCameraViewSet(1);
    BoardCameraTargetPlayerSet(playerIndex);

    if (condorEventSucceeded != 0) {
        BoardSpacePosGet(0, GWPlayer[playerIndex].space_curr, &spacePosition);
        BoardPlayerPosGet(playerIndex, &playerPosition);
        BoardPlayerPosLerpStart(playerIndex, &playerPosition, &spacePosition, 0x14);
        while (GWPlayer[playerIndex].moving) {
            HuPrcVSleep();
        }
    }

    BoardPlayerIdleSet(playerIndex);
    BoardCameraMotionWait();
}

void CondorPlayPurchaseAnimation(s32 playerIndex)
{
    s32 stateValue = 0;
    s32 index = 0;
    s32 integerValue[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x65), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x65), DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x65),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x65), DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x65), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x65),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x65), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x65) };
    Vec modelPosition;
    Vec playerPosition;
    Vec positionDelta;
    Vec cameraRotation;
    s32 integerValue2[2] = { 0x00770013, 0x00770014 };
    f32 floatValue;
    s16 stateValue2;

    f32 distance;
    f32 floatValue2;
    f32 floatValue3;
    f32 floatValue4;

    s16 character;

    condorCartModelId = BoardModelCreate(integerValue2[boardState->condorSide], NULL, 0);
    character = GWPlayer[playerIndex].character;
    condorPlayerMotionId = BoardPlayerMotionCreate(playerIndex, integerValue[character]);

    Hu3DModelObjPosGet(BoardModelIDGet(condorModelId), "itemhook_oya", &modelPosition);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    distance = BoardVecDistXZCalc(&modelPosition, &playerPosition);
    VECSubtract(&modelPosition, &playerPosition, &positionDelta);
    VECNormalize(&positionDelta, &positionDelta);
    VECScale(&positionDelta, &positionDelta, distance / 45.0f);
    BoardPlayerMotionShiftSet(playerIndex, 4, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(5);

    floatValue = BoardPlayerRotYGet(playerIndex);
    floatValue4 = 180.0f + floatValue;
    stateValue = 0;
    stateValue2 = 0;
    floatValue2 = 40.0f;

    while (1) {
        floatValue3 = __OSs16tof32(&stateValue2);
        positionDelta.y = floatValue2 + (-0.08166667f * (0.75f * (floatValue3 * floatValue3)));
        VECAdd(&playerPosition, &positionDelta, &playerPosition);
        distance = BoardVecDistXZCalc(&modelPosition, &playerPosition);

        if (distance < 5.0f) {
            positionDelta.x = positionDelta.z = 0.0f;
            playerPosition.x = modelPosition.x;
            playerPosition.z = modelPosition.z;
        }

        if (stateValue == 0) {
            if (modelPosition.y < playerPosition.y) {
                stateValue = 1;
            }
        }
        else {
            BoardDAngleCalcRange(&floatValue, floatValue4, 5.142857f);
            BoardPlayerRotYSet(playerIndex, floatValue);
        }

        if (playerPosition.y <= modelPosition.y && stateValue != 0) {
            playerPosition.y = modelPosition.y;
            break;
        }

        BoardPlayerPosSetV(playerIndex, &playerPosition);
        HuPrcVSleep();
        stateValue2 += 1;
    }

    BoardMusStart(1, 0x18, 0x7F, 0);
    BoardPlayerPosSet(playerIndex, 0.0f, 0.0f, 0.0f);

    BoardModelHookSet(condorModelId, "itemhook_oya", BoardPlayerModelGet(playerIndex));
    BoardPlayerMotionStart(playerIndex, condorPlayerMotionId, 0x40000001);
    HuAudFXPlay(0x44F);
    BoardModelMotionShiftSet(condorModelId, 5, 0.0f, 5.0f, HU3D_MOTATTR_NONE);

    cameraRotation.x = -15.0f;
    cameraRotation.y = 180.0f;
    cameraRotation.z = 0.0f;

    BoardCameraMotionStartEx(condorModelId, &cameraRotation, NULL, 4000.0f, -1.0f, 336);

    for (index = 0, stateValue2 = 0;; stateValue2++) {
        if (index == 0) {
            if (stateValue2 == 0x78U) {
                WipeColorSet(0, 0, 0);
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
                index = 1;
            }
        }
        else if (WipeStatGet() == 0) {
            break;
        }
        HuPrcVSleep();
    }
}

void CondorAnimatePlayerRide(s32 value)
{
    condorDestinationSide = 0;
    CondorSetupPlayerWork(value);
    CondorMoveToDestination(value);
    condorDestinationSide = 1;
    BoardStatusShowSetForce(value);
    BoardStatusShowSet(value, 1);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 21);

    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }

    while (CondorCheckCompletion() != 3) {
        HuPrcVSleep();
    }

    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }

    CondorWorkCleanup();
    BoardAudSeqPause(0, 0, 0x3E8);
}

void CondorMoveToOtherSpace(s32 playerIndex)
{
    Mtx modelMatrix;
    Vec spacePosition;
    Vec cameraRotation;
    Vec cameraRotation2;
    Vec spacePosition2;
    f32 floatValue;
    s16 linkedSpaceIndex;
    s32 linkedSpaceIndex2;
    s16 spaceIndex;

    if (boardState->condorSide != 0) {
        boardState->condorSide = 0;
        spaceIndex = BoardSpaceFlagSearch(0, 0x8000);
    }
    else {
        boardState->condorSide = 1;
        spaceIndex = BoardSpaceFlagSearch(0, 0x4000);
    }

    linkedSpaceIndex = BoardSpaceLinkFlagSearch(0, spaceIndex, 0x02000000);
    BoardCameraMoveSet(0);
    BoardModelHookReset(condorModelId);

    if (boardState->condorSide != 0) {
        linkedSpaceIndex2 = BoardSpaceFlagSearch(0, 0x4000);
    }
    else {
        linkedSpaceIndex2 = BoardSpaceFlagSearch(0, 0x8000);
    }

    linkedSpaceIndex2 = BoardSpaceLinkFlagSearch(0, linkedSpaceIndex2, 0x02000000);
    BoardSpacePosGet(0, linkedSpaceIndex2, &spacePosition2);
    floatValue = 0.0f;
    spacePosition2.x += -300.0 * sind(floatValue);
    spacePosition2.z += -300.0 * cosd(floatValue);
    spacePosition2.y = 45.0f + spacePosition2.y;
    BoardModelPosSetV(condorModelId, &spacePosition2);
    BoardModelMotionStart(condorModelId, 2, 0x40000001);
    BoardModelMotionStart(condorModelId, 1, 0x40000001);
    BoardModelRotYSet(condorModelId, 0.0f);
    MTXIdentity(modelMatrix);
    BoardModelMtxSet(condorModelId, &modelMatrix);

    cameraRotation.x = 0.0f;
    cameraRotation.y = 150.0f;
    cameraRotation.z = 0.0f;
    cameraRotation2.x = -15.0f;
    cameraRotation2.y = 0.0f;
    cameraRotation2.z = 0.0f;

    BoardCameraMotionStartEx(condorModelId, &cameraRotation2, &cameraRotation, 1900.0f, -1.0f, 1);
    BoardSpacePosGet(0, linkedSpaceIndex, &spacePosition);
    BoardPlayerPosSetV(playerIndex, &spacePosition);
    GWPlayer[playerIndex].space_curr = spaceIndex;
    BoardPlayerRotYSet(playerIndex, 180.0f);
    BoardPlayerIdleSet(playerIndex);
    HuPrcSleep(2);
    BoardCameraMoveSet(1);
    BoardModelMotionStart(boardModelIds[2], 0, 0x40000001);
    BoardModelMotionStartEndSet(boardModelIds[2], 1, 0x64);
    BoardStatusItemSet(0);
    HuPrcSleep(2);
    BoardStatusItemSet(1);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 21);

    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }

    BoardWinCreate(2, MAKE_MESSID(22, 15), 8);
    BoardWinWait();
    BoardWinKill();
}

void CondorCheckPlayerEligibility(s32 playerIndex)
{
    s32 integerValue;
    s32 starPos;
    s32 stateValue;

    stateValue = 0;
    starPos = GWSystem.star_pos;

    if (boardState->condorSide != 0) {
        if ((starPos == 1) || (starPos == 2) || (starPos == 3) || (starPos == 4)) {
            stateValue = 1;
        }
    }
    else if ((starPos == 0) || (starPos == 5) || (starPos == 6) || (starPos == 7)) {
        stateValue = 1;
    }

    if (BoardPlayerCoinsGet(playerIndex) < 0x14) {
        stateValue = 1;
    }

    if (stateValue != 0) {
        BoardComKeySetLeft();
        return;
    }

    BoardComKeySetRight();
}

char *condorHookNames[] = { "jyanA", "jyanB" };

void CondorUpdatePlayerAnimation(CondorPlayerWork *playerWork)
{
    Mtx modelMatrix;
    Mtx modelMatrix2;
    Mtx modelMatrix3;
    Mtx modelMatrix4;
    Vec spacePosition;
    Vec spacePosition2;
    Vec modelPosition;
    Vec modelPosition2;
    Vec positionDelta;
    Vec position;
    Vec position2;
    Vec position3;
    s32 stateValue;
    f32 unusedFloatValue;
    s16 integerValue;
    s16 integerValue2;
    f32 floatValue;
    s16 modelId;
    s16 stateValue2;
    f32 floatValue2;
    f32 headingAngle;
    s16 i;
    char *hookName;
    CondorPlayerPositionWork *value2;
    HU3DMODEL *value3;

    memset(&condorPlayerPositions, 0, 0xA0);
    modelPosition2.x = modelPosition.x = 0.0f;
    modelPosition2.y = modelPosition.y = 0.0f;
    modelPosition2.z = modelPosition.z = 0.0f;
    stateValue = 0;
    BoardSpaceFlagPosGet(0, 0x8000U, &spacePosition);
    BoardSpaceFlagPosGet(0, 0x8000U, &spacePosition2);

    if (boardState->condorSide != 0) {
        VECSubtract(&spacePosition, &spacePosition2, &positionDelta);
    }
    else {
        VECSubtract(&spacePosition2, &spacePosition, &positionDelta);
    }

    headingAngle = atan2d(-positionDelta.x, -positionDelta.z);
    BoardModelMotionStart(condorCartModelId, 0, 0U);
    BoardModelMotionSpeedSet(condorCartModelId, 8.0f);
    HuPrcSleep(1);
    modelId = BoardModelIDGet(condorCartModelId);
    value3 = &Hu3DData[modelId];
    hookName = condorHookNames[boardState->condorSide];
    Hu3DModelObjPosGet(BoardModelIDGet(condorCartModelId), hookName, &modelPosition2);
    position2.x = 0.0f;
    position2.y = 1.0f;
    position2.z = 0.0f;

    for (i = stateValue2 = 0; i < 20;) {
        Hu3DMotionExec(modelId, value3->motId, value3->motWork.time, 0);
        Hu3DModelObjPosGet(BoardModelIDGet(condorCartModelId), hookName, &modelPosition);
        value3->motWork.time += value3->motWork.value4;

        if (value3->motWork.time > 400.0f) {
            break;
        }

        if (stateValue2 % 4 == 0 && ((s16)stateValue2 != 0)) {
            value2 = &playerWork->positionY[i];
            value2->playerIndex = 1;
            floatValue = (0.5f - BoardRandFloat()) * 90;
            OSf32tos16(&floatValue, &integerValue2);
            integerValue2 = (integerValue2 / 10) * 10;
            OSs16tof32(&integerValue2, &floatValue);
            condorPlayerPositions[i + 1][0] = floatValue;
            condorPlayerPositions[i + 1][1] = value3->motWork.time;
            position = modelPosition2;
            position3 = modelPosition;
            BoardMTXCalcLookAt(modelMatrix3, &position3, &position2, &position);
            MTXTranspose(modelMatrix3, modelMatrix3);
            MTXRotDeg(modelMatrix2, 'z', floatValue);
            MTXTrans(modelMatrix, 0.0f, -350.0f, 0.0f);
            MTXConcat(modelMatrix2, modelMatrix, modelMatrix);
            MTXConcat(modelMatrix3, modelMatrix, modelMatrix);
            MTXTrans(modelMatrix4, modelPosition.x, modelPosition.y, modelPosition.z);
            MTXConcat(modelMatrix4, modelMatrix, modelMatrix4);
            Hu3DMtxTransGet(modelMatrix4, &value2->positionY);
            BoardModelPosSetV(value2->model, &value2->positionY);
            BoardModelVisibilitySet(value2->model, 1);
            i++;
        }
        modelPosition2 = modelPosition;
        stateValue2++;
    }

    playerWork->animationState = i;
}

void CondorSetupPlayerWork(s32 playerIndex)
{
    Vec modelPosition;
    OMOBJ *obj;
    s32 i;
    CondorPlayerWork *work;
    CondorPlayerPositionWork *positionWork;

    obj = omAddObjEx(boardObjMan, 0x101, 0U, 0U, -1, CondorUpdate);
    condorMovementObject = obj;
    work = omObjGetWork(obj, CondorPlayerWork);
    work->isActive = 0;
    work->unreferencedCondorData = 0;
    work->isMoving = 0;
    work->isVisible = 0;
    work->timer = -1;
    work->state = playerIndex;
    work->positionY = HuMemDirectMallocNum(HEAP_HEAP, 0xA00, HU_MEMNUM_OVL);
    Hu3DModelObjPosGet(BoardModelIDGet(boardModelIds[2]), "coin", &modelPosition);
    work->model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x07), NULL, 0);
    BoardModelVisibilitySet(work->model, 0);
    BoardModelPosSetV(work->model, &modelPosition);
    BoardModelLayerSet(work->model, 2U);
    memset(work->positionY, 0, 0xA00U);

    for (i = 0; i < 80; i++) {
        positionWork = &work->positionY[i];
        positionWork->model = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x09), 0, 1);
        BoardModelVisibilitySet(positionWork->model, 0);
        BoardModelMotionStart(positionWork->model, 0, 0x40000001U);
    }

    CondorUpdatePlayerAnimation(work);
}

void CondorWorkCleanup(void)
{
    omObjGetWork(condorMovementObject, CondorProcessWork)->isActive = 1;
}

void CondorPlayerWorkUpdate(CondorPlayerWork *playerWork)
{
    Vec modelPosition;
    f32 floatValue;
    CondorPlayerPositionWork *value;
    s32 i;

    BoardModelPosGet(condorModelId, &modelPosition);
    modelPosition.y += 250.0f;

    for (i = 0; i < playerWork->animationState; i++) {
        value = &playerWork->positionY[i];
        if (value->playerIndex == 1) {
            if (BoardVecMinDistCheck(&modelPosition, &value->positionY, 200.0f) != 0) {
                value->playerIndex = -1;
                BoardModelVisibilitySet(value->model, 0);
                CharEffectLayerSet(2);
                CharEffectCoinGlowCreate(1, &value->positionY);
                BoardPlayerCoinsAdd(playerWork->state, 1);
                HuAudFXPlay(7);
            }
            BoardModelPosSetV(value->model, &value->positionY);
        }
    }

    // reg alloc hack
    (void)i;
    (void)i;

    BoardModelPosGet(condorModelId, &modelPosition);

    for (i = playerWork->animationState; i < 80; i++) {
        value = &playerWork->positionY[i];
        if (value->playerIndex == 1) {
            value->positionX += 1.0f;
            floatValue = 0.8f * (value->positionX * value->positionX);
            VECAdd(&value->targetPosition, &value->positionY, &value->positionY);
            value->positionY.y += -0.0375f * floatValue;
            if (value->positionY.y < -300.0f) {
                value->playerIndex = -1;
                BoardModelVisibilitySet(value->model, 0);
            }
            else if (BoardVecMinDistCheck(&modelPosition, &value->positionY, 200.0f) != 0) {
                value->playerIndex = -1;
                BoardModelVisibilitySet(value->model, 0);
                BoardPlayerCoinsAdd(playerWork->state, 1);
                CharEffectLayerSet(2);
                CharEffectCoinGlowCreate(1, &value->positionY);
                HuAudFXPlay(7);
            }
            else {
                BoardModelPosSetV(value->model, &value->positionY);
            }
        }
    }
}

s32 CondorFindPlayerPosition(CondorPlayerPositionWork **positionWorkOut, s16 *animationStateOut)
{
    CondorPlayerWork *work;

    if (!(condorMovementObject)) {
        return 0;
    }

    work = omObjGetWork(condorMovementObject, CondorPlayerWork);
    *positionWorkOut = work->positionY;
    *animationStateOut = work->animationState;
    return 1;
}

void CondorPlayerStateUpdate(CondorPlayerWork *playerWork)
{
    Vec modelPosition;
    f32 floatValue;
    f32 radialDistance;
    s32 i;
    s32 j;
    CondorPlayerPositionWork *positionWork;

    if (playerWork->timer == 1) {
        Hu3DModelObjPosGet(BoardModelIDGet(boardModelIds[2]), "coin", &modelPosition);
        for (i = 0; i < 5; i++) {
            for (j = playerWork->animationState; j < 80; j++) {
                positionWork = &playerWork->positionY[j];
                if (positionWork->playerIndex == 0) {
                    positionWork->playerIndex = 1;
                    floatValue = 360.0f * BoardRandFloat();
                    radialDistance = 5.0f * BoardRandFloat();
                    positionWork->targetPosition.x = radialDistance * sind(floatValue);
                    positionWork->targetPosition.y = 50.0f + (20.0f * BoardRandFloat());
                    positionWork->targetPosition.z = radialDistance * cosd(floatValue);
                    positionWork->positionY.x = modelPosition.x;
                    positionWork->positionY.y = modelPosition.y;
                    positionWork->positionY.z = modelPosition.z;
                    BoardModelVisibilitySet(positionWork->model, 1);
                    BoardModelMotionStart(positionWork->model, 0, 0x40000001);
                    break;
                }
            }
        }
    }
}

void CondorPlayerMotionUpdate(CondorPlayerWork *playerWork)
{
    s32 playerIndex = 0;
    s32 playerIndex2[4] = { 5, 10, 30, 40 };
    s32 randomValue;
    s32 randomValue2;

    if (playerWork->timer == -1) {
        if (GWTeamGet()) {
            if (GWPlayer[playerWork->state].rank == 0) {
                playerIndex = 5;
            }
            else if (GWPlayer[playerWork->state].rank == 1) {
                playerIndex = 20;
            }
        }
        else {
            playerIndex = playerIndex2[GWPlayer[playerWork->state].rank];
        }

        randomValue = BoardRandMod(100);
        randomValue2 = BoardRandMod(100);

        if (randomValue < playerIndex) {
            if (randomValue2 < 30) {
                playerWork->timer = 2;
            }
            else {
                playerWork->timer = 1;
            }
        }
        else {
            playerWork->timer = 0;
        }

        if (playerWork->timer != 0) {
            BoardModelMotionShiftSet(boardModelIds[2], 0, 101.0f, 10.0f, HU3D_MOTATTR_NONE);
        }
    }
}

void CondorUpdate(OMOBJ *object)
{
    Vec playerPosition;
    Vec modelPosition;
    Vec positionDelta;
    PlayerState *value3;
    PlayerState *value2;
    s16 integerValue[2];
    s16 integerValue2[2];
    f32 animationTime;
    f32 floatValue;
    f32 floatValue2;
    s32 i, j, k;
    s32 integerValue3;
    s32 integerValue4;
    CondorPlayerPositionWork *unusedPositionWork;
    CondorPlayerWork *condorWork;
    CondorPlayerPositionWork *positionWork;

    condorWork = omObjGetWork(condorMovementObject, CondorPlayerWork);

    if ((condorWork->isActive != 0) || (BoardIsKill() != 0)) {
        for (i = 0; i < 80; i++) {
            positionWork = &condorWork->positionY[i];
            BoardModelKill(positionWork->model);
        }

        for (i = 0; i < 4; i++) {
            BoardModelVisibilitySet(BoardPlayerModelGet(i), 1);
        }

        BoardModelKill(condorWork->model);
        HuMemDirectFree(condorWork->positionY);
        condorMovementObject = 0;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }

    if (condorCartModelId != -1) {
        animationTime = BoardModelMotionTimeGet(condorCartModelId);
        if (condorWork->unreferencedCondorData == 0) {
            if ((boardState->condorSide != 0) && ((animationTime >= 212.0f)) || (boardState->condorSide == 0) && ((animationTime >= 86.0f))) {

                condorWork->unreferencedCondorData = 1;
            }
        }
        else {
            CondorPlayerMotionUpdate(condorWork);
        }

        if (condorWork->unreferencedCondorData != 0) {
            if (BoardModelMotionEndCheck(boardModelIds[2]) != 0) {
                BoardModelMotionStart(boardModelIds[2], 0, 0x40000001);
                BoardModelMotionStartEndSet(boardModelIds[2], 1, 100);
            }
            if (BoardModelMotionEndCheck(condorWork->model) != 0) {
                BoardModelVisibilitySet(condorWork->model, 0);
            }
        }

        if (BoardModelMotionTimeGet(boardModelIds[2]) >= 170.0f) {
            if (condorWork->isVisible == 0) {
                if (condorWork->isMoving == 0) {
                    HuAudFXPlay(0x43C);
                }
                else {
                    HuAudFXPlay(0x43D);
                }
                condorWork->isVisible = 1;
            }
            if (condorWork->timer == 1) {
                omVibrate(GWSystem.player_curr, 0xC, 4, 2);
                CondorPlayerStateUpdate(condorWork);
            }
            else if ((condorWork->timer == 2) && (condorWork->isMoving == 0)) {
                omVibrate(GWSystem.player_curr, 0xC, 0xC, 0);
                BoardModelVisibilitySet(condorWork->model, 1);
                BoardModelMotionStart(condorWork->model, 0, 0);
                condorWork->isMoving = 1;
            }
        }

        CondorPlayerWorkUpdate(condorWork);

        for (i = 0; i < 4; i++) {
            if (i != GWSystem.player_curr) {
                BoardPlayerPosGet(i, &playerPosition);
                BoardModelPosGet(condorModelId, &modelPosition);
                VECSubtract(&playerPosition, &modelPosition, &positionDelta);
                if (VECMag(&positionDelta) >= 2000.0f) {
                    BoardModelVisibilitySet(BoardPlayerModelGet(i), 0);
                }
                else {
                    BoardModelVisibilitySet(BoardPlayerModelGet(i), 1);
                }
            }
        }
    }
}

void CondorMoveToDestination(s32 playerIndex)
{
    Vec cameraRotation;
    Vec cameraRotation2;
    OMOBJ *obj;
    CondorMotionWork *work;

    obj = omAddObjEx(boardObjMan, 0x7E01, 0, 0, -1, &CondorMotionUpdate);
    condorMotionObject = obj;
    work = omObjGetWork(obj, CondorMotionWork);
    work->isActive = 0;
    work->condorState = playerIndex;
    work->movementFlags = 1;
    work->isMoving = 0;

    switch (GWPlayer[work->condorState].diff) {
        case 0:
            work->positionY = (-15.0f + (-30.0f * BoardRandFloat()));
            break;
        case 1:
            work->positionY = (-5.0f + (-10.0f * BoardRandFloat()));
            break;
        case 2:
            work->positionY = (-2.0f + (-5.0f * BoardRandFloat()));
            break;
        case 3:
            work->positionY = 0.0f;
            break;
    }

    BoardCameraMoveSet(0);

    cameraRotation.x = 0.0f;
    cameraRotation.y = -350.0f;
    cameraRotation.z = 0.0f;
    cameraRotation2.x = -20.0f;
    cameraRotation2.y = 0.0f;
    cameraRotation2.z = 0.0f;

    BoardCameraMotionStartEx(condorModelId, &cameraRotation2, &cameraRotation, 3000.0f, -1.0f, 1);
    BoardModelMotionStart(condorModelId, 3, 0x40000001U);
    BoardModelMotionStart(condorCartModelId, 0, 0U);
    BoardModelMotionSpeedSet(condorCartModelId, 0.7f);
    CondorFlyToTarget(obj, work);
    BoardCameraMotionWait();
    BoardCameraTargetModelSet(-1);
}

void CondorMotionUpdate(OMOBJ *object)
{
    s32 integerValue;
    CondorMotionWork *work;

    work = omObjGetWork(object, CondorMotionWork);

    if ((work->isActive != 0) || (BoardIsKill() != 0)) {
        condorMotionObject = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        if (condorCartMotionId != -1) {
            HuWinKill(condorCartMotionId);
            condorCartMotionId = -1;
        }
        return;
    }

    if ((10.0f == BoardModelMotionTimeGet(condorModelId)) || (50.0f == BoardModelMotionTimeGet(condorModelId))) {
        HuAudFXPlay(0x43B);
    }

    switch (work->movementFlags) {
        case 1:
            if (condorDestinationSide != 0) {
                CondorStateSet(2);
                CondorCleanup();
                BoardCameraTargetModelSet(condorModelId);
                BoardCameraMoveSet(1);
            }
            break;
        case 2:
            CondorFlyToTarget(object, (CondorMotionWork *)work);
            break;
        case 3:
            BoardCameraTargetModelSet(-1);
            BoardCameraMoveSet(0);
            work->isActive = 1;
            break;
    }
}

char *condorPlayerHookNames[] = { "jyanA", "jyanB" };

void CondorFlyToTarget(OMOBJ *object, CondorMotionWork *motionWork)
{
    Mtx modelMatrix;
    Mtx modelMatrix2;
    Mtx modelMatrix3;
    Mtx modelMatrix4;
    Mtx modelMatrix5;
    Vec modelPosition;
    Vec position;
    Vec positionDelta;
    Vec position2;
    Vec position3;
    Vec position4;
    s16 modelId;
    char *value;
    HU3DMODEL *value2;

    if (((WipeStatGet() == 0) && (motionWork->isMoving) || (BoardModelMotionEndCheck(condorCartModelId) != 0))) {
        CondorStateSet(3);
        return;
    }

    if ((BoardModelMotionTimeGet(condorCartModelId) >= 410.0f) && (motionWork->isMoving) == 0) {
        WipeColorSet(0, 0, 0);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
        motionWork->isMoving = 1;
        BoardAudSeqFadeOut(1, 0x3E8);
    }

    position.x = object->rot.x;
    position.y = object->rot.y;
    position.z = object->rot.z;

    modelId = BoardModelIDGet(condorCartModelId);
    value2 = &Hu3DData[modelId];
    value = condorPlayerHookNames[boardState->condorSide];

    Hu3DMotionExec(modelId, value2->motId, value2->motWork.time, 0);
    Hu3DModelObjPosGet(BoardModelIDGet(condorCartModelId), value, &modelPosition);

    if (0.0f != value2->motWork.time) {
        VECSubtract(&position, &modelPosition, &positionDelta);
    }
    else {
        positionDelta.x = positionDelta.y = positionDelta.z = 0.0f;
    }

    object->rot.x = modelPosition.x;
    object->rot.y = modelPosition.y;
    object->rot.z = modelPosition.z;

    CondorLandAtTarget(object, motionWork);

    position4 = position;
    position3 = modelPosition;
    position2.x = position2.z = 0.0f;
    position2.y = 1.0f;

    BoardMTXCalcLookAt(modelMatrix, &position3, &position2, &position4);
    MTXTranspose(modelMatrix, modelMatrix);
    MTXRotDeg(modelMatrix2, 'z', -motionWork->positionX);
    MTXTrans(modelMatrix5, 0.0f, -600.0f, 0.0f);
    MTXConcat(modelMatrix2, modelMatrix5, modelMatrix5);
    MTXConcat(modelMatrix, modelMatrix5, modelMatrix5);
    MTXTrans(modelMatrix4, modelPosition.x, modelPosition.y, modelPosition.z);
    MTXConcat(modelMatrix4, modelMatrix5, modelMatrix4);
    Hu3DMtxTransGet(modelMatrix4, &modelPosition);
    MTXRotDeg(modelMatrix3, 'y', atan2d(-positionDelta.x, -positionDelta.z));
    MTXConcat(modelMatrix3, modelMatrix2, modelMatrix);
    BoardModelMtxSet(condorModelId, &modelMatrix);
    BoardModelPosSetV(condorModelId, &modelPosition);
    BoardCameraRotSet(-20.0f, atan2d(positionDelta.x, positionDelta.z));
}

void CondorLandAtTarget(OMOBJ *object, CondorMotionWork *motionWork)
{
    f32 floatValue = 0.0f;
    f32 floatValue2;
    f32 floatValue3;

    if (GWPlayer[motionWork->condorState].com == 0) {
        floatValue2 = HuPadStkX[GWPlayer[motionWork->condorState].port];
    }
    else {
        floatValue2 = CondorDistanceToTarget(object, motionWork);
    }

    floatValue = 0.625f * floatValue2;

    if (floatValue < 0.0f) {
        floatValue3 = -floatValue;
    }
    else {
        floatValue3 = floatValue;
    }

    if (floatValue3 < 2.0f) {
        motionWork->positionX *= 0.97f;
        return;
    }

    BoardDAngleCalcRange(&motionWork->positionX, floatValue, 1.0f);
}

f32 CondorDistanceToTarget(OMOBJ *object, CondorMotionWork *motionWork)
{
    CondorPlayerPositionWork *value2;
    f32 animationTime;
    s16 i;
    CondorPlayerWork *work;
    s16 integerValue;
    f32 *value;

    if (condorMovementObject) {
        work = omObjGetWork(condorMovementObject, CondorPlayerWork);
        value2 = work->positionY;
        integerValue = work->animationState;
    }

    animationTime = BoardModelMotionTimeGet(condorCartModelId);
    animationTime = animationTime + motionWork->positionY;

    for (i = 0; i < integerValue; i++) {
        value = condorPlayerPositions[i];
        if ((value[1] < animationTime) && (value[3] > animationTime)) {
            return -1.5f * value[2];
        }
    }

    return 0.0f;
}

void CondorStateSet(s8 value)
{
    CondorMotionWork *work = omObjGetWork(condorMotionObject, CondorMotionWork);
    work->movementFlags = value;
}

s32 CondorCheckCompletion(void)
{
    CondorMotionWork *work = omObjGetWork(condorMotionObject, CondorMotionWork);
    return work->movementFlags;
}

void CondorCleanup(void)
{
    f32 widthHeight[2];
    s32 messageID = DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x05);
    f32 x;
    f32 y;

    HuWinMesMaxSizeGet(1, widthHeight, messageID);
    x = -10000.0f;
    y = 392.0f;
    condorCartMotionId = HuWinCreate(x, y, widthHeight[0], widthHeight[1], 0);
    HuWinBGTPLvlSet(condorCartMotionId, 0.0f);
    HuWinMesSpeedSet(condorCartMotionId, 0);
    HuWinMesSet(condorCartMotionId, messageID);
}
