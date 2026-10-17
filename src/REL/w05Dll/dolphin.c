#include "REL/w05Dll.h"

#include "game/audio.h"
#include "game/chrman.h"
#include "game/frand.h"
#include "game/hu3d.h"
#include "game/objsub.h"

#include "game/board/audio.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/window.h"

#include "ext_math.h"

// bss
s16 dolphinPlayerMotionIds[2];
s16 dolphinEffectModelIds[3];
s32 dolphinPlayerIndex;
s16 dolphinBoardModel;
s16 dolphinSpaceList[6];

// data
char *dolphinObjectNames[6] = { "orange1", "orange2", "pine2", "pine1", "suika1", "suika2" };
s32 dolphinEffectModels[6] = {
    DATA_MAKE_NUM(DATADIR_W05, 0x27),
    DATA_MAKE_NUM(DATADIR_W05, 0x28),
    DATA_MAKE_NUM(DATADIR_W05, 0x29),
    DATA_MAKE_NUM(DATADIR_W05, 0x2A),
    DATA_MAKE_NUM(DATADIR_W05, 0x2B),
    DATA_NUM_LISTEND,
};
Vec dolphinRotations[6] = {
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 90.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 180.0f, 0.0f },
    { 0.0f, 270.0f, 0.0f },
    { 0.0f, 90.0f, 0.0f },
};
Vec dolphinPositionOffsets[6] = {
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 300.0f, 0.0f },
    { 0.0f, 450.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
};
s32 dolphinPlayerMotions[2] = { 0x76, 0x77 };
s32 dolphinEffectParameters[6] = {
    DATA_MAKE_NUM(DATADIR_W05, 0x2C),
    DATA_MAKE_NUM(DATADIR_W05, 0x2D),
    DATA_MAKE_NUM(DATADIR_W05, 0x2E),
    DATA_MAKE_NUM(DATADIR_W05, 0x2C),
    DATA_MAKE_NUM(DATADIR_W05, 0x2F),
    DATA_MAKE_NUM(DATADIR_W05, 0x30),
};

void W05PauseMusicAfterDelay(void)
{
    s32 loopIndex;

    BoardAudSeqPause(0, 1, 0x3E8);

    for (loopIndex = 0x3C; loopIndex != 0; loopIndex--) {
        HuPrcVSleep();
    }

    BoardMusStart(1, 0x24, 0x7F, 0);
    HuPrcEnd();
}

void W05FadeOutMusicAfterDelay(void)
{
    s32 loopIndex;

    BoardAudSeqFadeOut(1, 0x3E8);

    for (loopIndex = 0x3C; loopIndex != 0; loopIndex--) {
        HuPrcVSleep();
    }
    BoardAudSeqPause(0, 0, 0x3E8);
    HuPrcEnd();
}

void W05RunDolphinBoardEvent(s32 eventIndex)
{
    Vec movementVector;
    Vec vector;
    Vec movementVector2;
    Vec movementVector3;
    s16 spaceIndex[4];
    s32 playerIndex;
    F32 floatValue;
    F32 floatValue2;
    F32 frameCount;
    F32 floatValue3;
    s16 spaceIndex2;
    s32 playerIndex2;
    BoardSpace *space;
    OMOBJ *effectObject;
    BoardSpace *space2;
    s32 loopIndex;
    s32 spaceIndex3;

    dolphinPlayerIndex = GWSystem.player_curr;
    playerIndex = GWPlayer[dolphinPlayerIndex].character;

    for (loopIndex = 0; loopIndex < 2; loopIndex++) {
        dolphinPlayerMotionIds[loopIndex] = BoardPlayerMotionCreate(dolphinPlayerIndex, dolphinPlayerMotions[loopIndex] | characterIdleMotions[GWPlayer[dolphinPlayerIndex].character]);
    }

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        dolphinEffectModelIds[loopIndex] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x0A), NULL, 1);
        BoardModelAttrSet(dolphinEffectModelIds[loopIndex], 0x40000001);
        BoardModelVisibilitySet(dolphinEffectModelIds[loopIndex], 0);
    }
    W05DolphinCreateEffect();
    playerIndex2 = BoardSpaceLinkFlagSearch(0, GWPlayer[dolphinPlayerIndex].space_curr, 0x02000000);
    BoardPlayerMoveBetween(dolphinPlayerIndex, GWPlayer[dolphinPlayerIndex].space_curr, playerIndex2);
    space2 = BoardSpaceGet(0, playerIndex2);

    loopIndex = 0;
    while (loopIndex < space2->link_cnt) {
        space = BoardSpaceGet(0, space2->link[loopIndex]);
        if ((space->flag & 0x02000000) == 0) {
            loopIndex++;
            continue;
        }
        break;
    }
    spaceIndex3 = space2->link[loopIndex];
    BoardPlayerMotBlendSet(dolphinPlayerIndex, dolphinRotations[eventIndex].y - 180.0f, 5);

    while (BoardPlayerMotBlendCheck(dolphinPlayerIndex) == 0) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(dolphinPlayerIndex);
    HuPrcSleep(8);
    dolphinBoardModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x26), dolphinEffectModels, 0);
    Hu3DModelObjPosGet(BoardModelIDGet(mainBoardModel), dolphinObjectNames[eventIndex], &movementVector);
    VECAdd(&movementVector, &dolphinPositionOffsets[eventIndex], &movementVector);
    BoardModelMotionStart(dolphinBoardModel, 4, 0);
    movementVector.y = -50.0f;
    BoardModelPosSetV(dolphinBoardModel, &movementVector);
    BoardModelRotSetV(dolphinBoardModel, &dolphinRotations[eventIndex]);
    W05DolphinSpawnEffect(0, &movementVector);

    spaceIndex2 = 0;
    while (BoardModelMotionTimeGet(dolphinBoardModel) < BoardModelMotionMaxTimeGet(dolphinBoardModel)) {
        if (BoardModelMotionTimeGet(dolphinBoardModel) > 50.0f && spaceIndex2 == 0) {
            spaceIndex2++;
            HuAudFXPlay(0x47D);
            HuAudFXPlay(0x48E);
        }
        if (BoardModelMotionTimeGet(dolphinBoardModel) > 70.0f && spaceIndex2 == 1) {
            spaceIndex2++;
            HuAudFXPlay(0x47E);
        }
        if (BoardModelMotionTimeGet(dolphinBoardModel) > 105.0f && spaceIndex2 == 2) {
            spaceIndex2++;
            HuAudFXPlay(0x481);
        }
        HuPrcVSleep();
    }
    BoardModelMotionStart(dolphinBoardModel, 1, 0x40000001);
    effectObject = W05DolphinSpawnEffect(2, &movementVector);
    BoardCameraViewSet(2);
    BoardCameraMotionWait();
    HuAudFXPlay(0x480);
    BoardWinCreate(2, MAKE_MESSID(39, 7), 0xA);
    BoardWinWait();
    BoardWinKill();
    if (eventIndex >= 4) {
        HuPrcChildCreate(W05PauseMusicAfterDelay, 0x2001, 0x800, 0, boardMainProc);
    }
    if ((eventIndex & 1) != 0) {
        playerIndex2 = eventIndex - 1;
    }
    else {
        playerIndex2 = eventIndex + 1;
    }
    Hu3DModelObjPosGet(BoardModelIDGet(mainBoardModel), dolphinObjectNames[playerIndex2], &vector);
    floatValue2 = atan2d(vector.x - movementVector.x, vector.z - movementVector.z);
    BoardModelMotionShiftSet(dolphinBoardModel, 2, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);

    while (W05TurnBoardModelTowardAngle(dolphinBoardModel, floatValue2, 10.0f) == 0) {
        HuPrcVSleep();
    }

    BoardPlayerPosGet(dolphinPlayerIndex, &movementVector2);
    floatValue3 = sqrtf(((movementVector.x - movementVector2.x) * (movementVector.x - movementVector2.x)) + ((movementVector.z - movementVector2.z) * (movementVector.z - movementVector2.z)));
    VECSubtract(&movementVector, &movementVector2, &movementVector3);
    VECNormalize(&movementVector3, &movementVector3);
    VECScale(&movementVector3, &movementVector3, floatValue3 / 50.0f);
    BoardPlayerMotionStart(dolphinPlayerIndex, 4, 0);
    frameCount = 0.0f;
    floatValue = 15.0f;

loop37:
    movementVector3.y = floatValue - (0.075f * (0.25f * (frameCount * frameCount)));
    frameCount++;
    VECAdd(&movementVector2, &movementVector3, &movementVector2);
    floatValue3 = sqrtf(((movementVector.x - movementVector2.x) * (movementVector.x - movementVector2.x)) + ((movementVector.z - movementVector2.z) * (movementVector.z - movementVector2.z)));
    if (floatValue3 < 5.0f) {
        movementVector3.x = movementVector3.z = 0.0f;
        movementVector2.x = movementVector.x;
        movementVector2.z = movementVector.z;
    }

    if (movementVector2.y <= 20.0f + movementVector.y) {
        movementVector2.y = 20.0f + movementVector.y;
    }
    else {
        BoardPlayerPosSetV(dolphinPlayerIndex, &movementVector2);
        HuPrcVSleep();
        goto loop37;
    }
    BoardPlayerPosSetV(dolphinPlayerIndex, &movementVector2);
    BoardPlayerMotionStart(dolphinPlayerIndex, dolphinPlayerMotionIds[0], 0);

    while (W05TurnBoardModelTowardAngle(BoardPlayerModelGet(dolphinPlayerIndex), floatValue2, 10.0f) == 0) {
        HuPrcVSleep();
    }

    while (BoardPlayerMotionEndCheck(dolphinPlayerIndex) == 0) {
        HuPrcVSleep();
    }

    BoardPlayerMotionStart(dolphinPlayerIndex, dolphinPlayerMotionIds[1], 0x40000001);
    HuPrcSleep(0x12);
    effectObject->work[1] = 1;
    if (eventIndex < 4) {
        W05DolphinMovePlayerToRewardSpace(eventIndex, spaceIndex3, &vector);
    }
    else {
        W05DolphinMovePlayerToItemSpace(eventIndex, spaceIndex3, &vector);
    }
    BoardModelPosGet(dolphinBoardModel, &movementVector);
    BoardPlayerPosGet(dolphinPlayerIndex, &movementVector2);
    floatValue3 = atan2d(movementVector.x - movementVector2.x, movementVector.z - movementVector2.z);
    BoardPlayerMotBlendSet(dolphinPlayerIndex, floatValue3, 0xF);

    while (BoardPlayerMotBlendCheck(dolphinPlayerIndex) == 0) {
        HuPrcVSleep();
    }

    W05DolphinSpawnEffect(1, &movementVector);
    BoardModelMotionShiftSet(dolphinBoardModel, 5, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    spaceIndex2 = 0;
    loopIndex = 0;

    while (loopIndex < 0x5AU) {
        if ((BoardModelMotionTimeGet(dolphinBoardModel) > 15.0f) && (spaceIndex2 == 0)) {
            spaceIndex2++;
            HuAudFXPlay(0x47D);
            if (eventIndex >= 4) {
                BoardAudSeqFadeOut(1, 0x3E8);
                HuPrcChildCreate(W05FadeOutMusicAfterDelay, 0x2001, 0x800, 0, boardMainProc);
            }
        }
        if ((BoardModelMotionTimeGet(dolphinBoardModel) > 30.0f) && (spaceIndex2 == 1)) {
            spaceIndex2++;
            HuAudFXPlay(0x47E);
        }
        if ((BoardModelMotionTimeGet(dolphinBoardModel) > 65.0f) && (spaceIndex2 == 2)) {
            spaceIndex2++;
            HuAudFXPlay(0x481);
        }
        HuPrcVSleep();
        loopIndex++;
    }
    BoardCameraMoveSet(0);
    GWPlayer[dolphinPlayerIndex].space_curr = spaceIndex3;
    playerIndex2 = BoardSpaceLinkTargetListGet(0, spaceIndex3, spaceIndex);

    for (loopIndex = 0; (s32)loopIndex < playerIndex2;) {
        space2 = BoardSpaceGet(0, spaceIndex[loopIndex]);
        if ((space2->flag & 0x80) == 0) {
            loopIndex++;
            continue;
        }
        break;
    }
    BoardPlayerMoveBetween(dolphinPlayerIndex, spaceIndex3, spaceIndex[loopIndex]);
    GWPlayer[dolphinPlayerIndex].space_curr = spaceIndex[loopIndex];
    BoardPlayerIdleSet(dolphinPlayerIndex);
    HuPrcSleep(8);
    if (eventIndex >= 4) {
        BoardStatusShowSetAll(1);
    }
    BoardModelKill(dolphinBoardModel);

    for (loopIndex = 0; loopIndex < 2; loopIndex++) {
        BoardPlayerMotionKill(dolphinPlayerIndex, dolphinPlayerMotionIds[loopIndex]);
    }

    W05DolphinDestroyEffects();

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        BoardModelKill(dolphinEffectModelIds[loopIndex]);
    }
}

void W05DolphinMovePlayerToRewardSpace(s32 playerIndex, s32 targetSpace, Vec *targetPosition)
{
    Vec movementVector;
    Vec movementVector2;
    Vec movementVector3;
    Vec position;
    F32 floatValue;
    F32 floatValue2;
    F32 floatValue3;
    F32 floatValue4;
    F32 frameCount;
    F32 floatValue5;
    OMOBJ *effectObject;
    OMOBJ *effectObject2;
    s32 soundHandle;

    floatValue2 = 10.0f;
    BoardModelPosGet(dolphinBoardModel, &movementVector);
    targetPosition->y = movementVector.y;
    VECSubtract(targetPosition, &movementVector, &movementVector3);
    VECNormalize(&movementVector3, &movementVector3);
    VECScale(&movementVector3, &movementVector3, floatValue2);
    soundHandle = HuAudFXPlay(0x47F);
    BoardModelPosGet(dolphinBoardModel, &position);
    effectObject = W05DolphinSpawnEffect(4, &position);
    effectObject2 = W05DolphinSpawnEffect(5, &position);
    while (1) {
        floatValue5 = sqrtf((targetPosition->x - movementVector.x) * (targetPosition->x - movementVector.x) + (targetPosition->z - movementVector.z) * (targetPosition->z - movementVector.z));
        if (floatValue5 <= floatValue2) {
            movementVector.x = targetPosition->x;
            movementVector.z = targetPosition->z;
            BoardModelPosSetV(dolphinBoardModel, &movementVector);
            movementVector2 = movementVector;
            movementVector2.y += 20.0f;
            BoardPlayerPosSetV(dolphinPlayerIndex, &movementVector2);
            break;
        }
        else {
            VECAdd(&movementVector, &movementVector3, &movementVector);
            BoardModelPosSetV(dolphinBoardModel, &movementVector);
            movementVector2 = movementVector;
            movementVector2.y += 20.0f;
            BoardPlayerPosSetV(dolphinPlayerIndex, &movementVector2);
            HuPrcVSleep();
        }
    }
    HuAudFXStop(soundHandle);
    effectObject->work[1] = 1;
    effectObject2->work[1] = 1;
    BoardPlayerIdleSet(dolphinPlayerIndex);
    HuPrcSleep(8);
    BoardSpacePosGet(0, targetSpace, &movementVector);
    floatValue5 = atan2d(movementVector.x - movementVector2.x, movementVector.z - movementVector2.z);
    BoardPlayerMotBlendSet(dolphinPlayerIndex, floatValue5, 0xF);

    while (BoardPlayerMotBlendCheck(dolphinPlayerIndex) == 0) {
        HuPrcVSleep();
    }

    BoardPlayerPosGet(dolphinPlayerIndex, &movementVector2);
    floatValue5 = sqrtf(((movementVector.x - movementVector2.x) * (movementVector.x - movementVector2.x)) + ((movementVector.z - movementVector2.z) * (movementVector.z - movementVector2.z)));
    VECSubtract(&movementVector, &movementVector2, &movementVector3);
    VECNormalize(&movementVector3, &movementVector3);
    VECScale(&movementVector3, &movementVector3, floatValue5 / 45.0f);
    BoardPlayerMotionStart(dolphinPlayerIndex, 4, 0);

    frameCount = 0.0f;
    floatValue = 15.0f;

    while (1) {
        movementVector3.y = floatValue - (0.075f * (0.25f * (frameCount * frameCount)));
        frameCount++;
        VECAdd(&movementVector2, &movementVector3, &movementVector2);
        floatValue5 = sqrtf(((movementVector.x - movementVector2.x) * (movementVector.x - movementVector2.x)) + ((movementVector.z - movementVector2.z) * (movementVector.z - movementVector2.z)));
        if (floatValue5 < 5.0f) {
            movementVector3.x = movementVector3.z = 0.0f;
            movementVector2.x = movementVector.x;
            movementVector2.z = movementVector.z;
        }
        if (movementVector2.y <= movementVector.y && movementVector3.y < 0.0f) {
            movementVector2.y = movementVector.y;
        }
        else {
            BoardPlayerPosSetV(dolphinPlayerIndex, &movementVector2);
            HuPrcVSleep();
            continue;
        }
        BoardPlayerPosSetV(dolphinPlayerIndex, &movementVector2);
        break;
    }
}

void W05DolphinEventCleanup(void)
{
    Vec rotation;
    s32 loopIndex;

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        BoardModelRotGet(dolphinEffectModelIds[loopIndex], &rotation);
        rotation.y = 0.003921569f * (360.0f * frand8());
        BoardModelRotSetV(dolphinEffectModelIds[loopIndex], &rotation);
    }
    while (1) {
        for (loopIndex = 0; loopIndex < 3; loopIndex++) {
            BoardModelRotGet(dolphinEffectModelIds[loopIndex], &rotation);
            rotation.y += 10.0f;
            if (rotation.y >= 360.0f) {
                rotation.y -= 360.0f;
            }
            BoardModelRotSetV(dolphinEffectModelIds[loopIndex], &rotation);
        }
        HuPrcVSleep();
    }
}

void W05DolphinMovePlayerToItemSpace(s32 eventIndex, s32 targetSpace, Vec *targetPosition)
{
    Vec vector;
    Vec movementVector;
    Vec movementVector2;
    Vec movementVector3;
    Vec rotation;
    Vec rotation2;
    Vec position;
    F32 floatValue;
    HUPROCESS *process;
    F32 floatValue2;
    F32 floatValue3;
    F32 floatValue4;
    F32 frameCount;
    F32 floatValue5;
    OMOBJ *effectObject;
    OMOBJ *effectObject2;
    OMOBJ *effectObject3;
    OMOBJ *effectObject4;
    s32 soundHandle;
    s32 integerValue;
    s32 integerValue2;

    BoardModelPosGet(dolphinBoardModel, &movementVector);
    process = HuPrcChildCreate(W05DolphinEventCleanup, 0x2001, 0x800, 0, boardMainProc);
    BoardCameraMoveSet(0);
    BoardCameraTargetPlayerSet(dolphinPlayerIndex);
    BoardCameraRotSet(-15.0f, 180.0f + BoardModelRotYGet(dolphinBoardModel));
    BoardCameraZoomSet(1400.0f);
    BoardStatusShowSetAll(0);
    W05DolphinFinishPlayerMovement(eventIndex, targetPosition);
    floatValue3 = 10.0f;
    targetPosition->y = movementVector.y;
    VECSubtract(targetPosition, &movementVector, &movementVector3);
    VECNormalize(&movementVector3, &movementVector3);
    VECScale(&movementVector3, &movementVector3, floatValue3);
    if (eventIndex == 5) {
        integerValue = 0x96;
    }
    else {
        integerValue = 0x5A;
    }
    floatValue5 = 0.0f;
    vector = movementVector;
    BoardModelRotGet(dolphinBoardModel, &rotation);
    BoardPlayerRotGet(dolphinPlayerIndex, &rotation2);
    BoardCameraTargetModelSet(-1);
    integerValue2 = 0;
    soundHandle = HuAudFXPlay(0x47F);
    BoardModelPosGet(dolphinBoardModel, &position);
    effectObject = W05DolphinSpawnEffect(4, &position);
    effectObject3 = W05DolphinSpawnEffect(5, &position);

    while (1) {
        floatValue = BoardModelRotYGet(dolphinBoardModel);
        movementVector = vector;
        floatValue4 = sqrtf(((targetPosition->x - movementVector.x) * (targetPosition->x - movementVector.x)) + ((targetPosition->z - movementVector.z) * (targetPosition->z - movementVector.z)));
        if (floatValue4 <= floatValue3) {
            movementVector.x = targetPosition->x;
            movementVector.z = targetPosition->z;
            BoardModelPosSetV(dolphinBoardModel, &movementVector);
            movementVector2 = movementVector;
            movementVector2.y += 20.0f;
            BoardPlayerPosSetV(dolphinPlayerIndex, &movementVector2);
            break;
        }
        else {
            VECAdd(&movementVector, &movementVector3, &movementVector);
            vector = movementVector;
            if (integerValue == 0) {
                movementVector.y += 250.0 * sind(floatValue5);
                floatValue5 -= 2.5f;
                if (floatValue5 < -540.0f) {
                    floatValue5 = -540.0f;
                }

                if (integerValue2 == 0) {
                    effectObject->work[1] = 1;
                    effectObject3->work[1] = 1;
                    integerValue2++;
                }
                else if (integerValue2 == 1 && floatValue5 < -180.0f) {
                    HuAudFXStop(soundHandle);
                    HuAudFXPlay(0x47D);
                    PlayerFXPlay(dolphinPlayerIndex, 0x125);
                    W05DolphinSpawnEffect(3, &movementVector);
                    integerValue2++;
                }
                else if ((integerValue2 == 2) && (floatValue5 < -360.0f)) {
                    soundHandle = HuAudFXPlay(0x47F);
                    HuAudFXPlay(0x481);
                    W05DolphinSpawnEffect(3, &movementVector);
                    integerValue2++;
                }
                else if ((integerValue2 == 3) && (-540.0f == floatValue5)) {
                    integerValue2++;
                    BoardModelPosGet(dolphinBoardModel, &position);
                    effectObject2 = W05DolphinSpawnEffect(4, &position);
                    effectObject4 = W05DolphinSpawnEffect(5, &position);
                }
            }
            else {
                integerValue--;
            }
            BoardModelPosSetV(dolphinBoardModel, &movementVector);
            rotation2.x = rotation.x = 45.0 * -sind(floatValue5);
            BoardModelRotSetV(dolphinBoardModel, &rotation);
            BoardPlayerRotSetV(dolphinPlayerIndex, &rotation2);
            movementVector2 = movementVector;
            movementVector2.y += 20.0f;
            BoardPlayerPosSetV(dolphinPlayerIndex, &movementVector2);
            W05DolphinInitializeModels();
            BoardCameraTargetSet(movementVector.x, -50.0f, movementVector.z);
            HuPrcVSleep();
        }
    }
    HuAudFXStop(soundHandle);
    effectObject2->work[1] = 1;
    effectObject4->work[1] = 1;
    BoardCameraMoveSet(1);
    BoardCameraTargetPlayerSet(dolphinPlayerIndex);
    BoardPlayerIdleSet(dolphinPlayerIndex);
    HuPrcSleep(8);
    BoardSpacePosGet(0, targetSpace, &movementVector);
    floatValue4 = atan2d(movementVector.x - movementVector2.x, movementVector.z - movementVector2.z);
    BoardPlayerMotBlendSet(dolphinPlayerIndex, floatValue4, 0xF);

    while (BoardPlayerMotBlendCheck(dolphinPlayerIndex) == 0) {
        HuPrcVSleep();
    }
    BoardPlayerPosGet(dolphinPlayerIndex, &movementVector2);
    floatValue4 = sqrtf(((movementVector.x - movementVector2.x) * (movementVector.x - movementVector2.x)) + ((movementVector.z - movementVector2.z) * (movementVector.z - movementVector2.z)));
    VECSubtract(&movementVector, &movementVector2, &movementVector3);
    VECNormalize(&movementVector3, &movementVector3);
    VECScale(&movementVector3, &movementVector3, floatValue4 / 45.0f);
    BoardPlayerMotionStart(dolphinPlayerIndex, 4, 0);
    frameCount = 0.0f;
    floatValue2 = 15.0f;
    while (1) {
        movementVector3.y = floatValue2 - (0.075f * (0.25f * (frameCount * frameCount)));
        frameCount++;
        VECAdd(&movementVector2, &movementVector3, &movementVector2);
        floatValue4 = sqrtf(((movementVector.x - movementVector2.x) * (movementVector.x - movementVector2.x)) + ((movementVector.z - movementVector2.z) * (movementVector.z - movementVector2.z)));
        if (floatValue4 < 5.0f) {
            movementVector3.x = movementVector3.z = 0.0f;
            movementVector2.x = movementVector.x;
            movementVector2.z = movementVector.z;
        }
        if (movementVector2.y <= movementVector.y && movementVector3.y < 0.0f) {
            movementVector2.y = movementVector.y;
            break;
        }
        else {
            BoardPlayerPosSetV(dolphinPlayerIndex, &movementVector2);
            HuPrcVSleep();
        }
    }
    BoardPlayerPosSetV(dolphinPlayerIndex, &movementVector2);
    HuPrcKill(process);
}

void W05DolphinFinishPlayerMovement(s32 dolphinVariant, Vec *targetPosition)
{
    Vec movementVector;
    Vec movementVector2;
    Vec movementVector3;
    Vec movementVector4;
    F32 floatValue[2] = { 1950.0f, 2550.0f };
    F32 floatValue2;
    F32 frameCount;
    s32 integerValue;

    BoardModelPosGet(dolphinBoardModel, &movementVector);
    dolphinVariant -= 4;
    targetPosition->y = movementVector.y;
    VECSubtract(targetPosition, &movementVector, &movementVector2);
    VECNormalize(&movementVector2, &movementVector2);
    frameCount = 45.0f;
    integerValue = 0;
    while (integerValue < 3) {
        movementVector3 = movementVector2;
        movementVector4 = movementVector2;
        VECScale(&movementVector3, &movementVector3, floatValue[dolphinVariant]);
        movementVector3.y += 250.0 * sind(frameCount);
        floatValue2 = 250.0 * -cosd(frameCount);
        VECScale(&movementVector4, &movementVector4, floatValue2);
        VECAdd(&movementVector4, &movementVector3, &movementVector3);
        VECAdd(&movementVector3, &movementVector, &movementVector3);
        BoardModelPosSetV(dolphinEffectModelIds[integerValue], &movementVector3);
        BoardModelVisibilitySet(dolphinEffectModelIds[integerValue], 1);
        frameCount += 45.0f;
        integerValue++;
    }
}

void W05DolphinInitializeModels(void)
{
    Vec movementVector;
    Vec movementVector2;
    Vec movementVector3;
    s32 loopIndex;
    F32 floatValue;

    BoardPlayerPosGet(dolphinPlayerIndex, &movementVector);
    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        if (BoardModelVisibilityGet(dolphinEffectModelIds[loopIndex]) != 0) {
            BoardModelPosGet(dolphinEffectModelIds[loopIndex], &movementVector2);
            VECSubtract(&movementVector2, &movementVector, &movementVector3);
            floatValue = VECMag(&movementVector3);
            if (floatValue < 60.0f) {
                CharEffectCoinGlowCreate(1, &movementVector2);
                BoardModelVisibilitySet(dolphinEffectModelIds[loopIndex], 0);
                BoardPlayerCoinsAdd(dolphinPlayerIndex, 1);
                HuAudFXPlay(7);
                omVibrate(dolphinPlayerIndex, 0xC, 6, 6);
            }
        }
    }
}

void W05DolphinCreateEffect(void)
{
    s32 loopIndex;

    for (loopIndex = 0; loopIndex < 6; loopIndex++) {
        dolphinSpaceList[loopIndex] = BoardModelCreate(dolphinEffectParameters[loopIndex], NULL, 0);
        BoardModelVisibilitySet(dolphinSpaceList[loopIndex], 0);
    }
}

void W05DolphinDestroyEffects(void)
{
    s32 loopIndex;

    for (loopIndex = 0; loopIndex < 6; loopIndex++) {
        BoardModelKill(dolphinSpaceList[loopIndex]);
    }
}

OMOBJ *W05DolphinSpawnEffect(u32 effectType, Vec *effectPosition)
{
    Vec boardPosition;
    OMOBJ *effectObject;
    F32 boardRotation;

    effectObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W05DolphinEffectUpdate);
    effectObject->trans.x = effectPosition->x;
    effectObject->trans.y = effectPosition->y;
    effectObject->trans.z = effectPosition->z;
    effectObject->work[0] = BoardModelCreate(dolphinEffectParameters[effectType], NULL, 1);
    effectObject->work[2] = effectType;
    BoardModelPosSetV(effectObject->work[0], effectPosition);
    BoardModelMotionTimeSet(effectObject->work[0], 0.0f);
    switch (effectType) {
        case 0:
        case 1:
            break;
        case 2:
            BoardModelAttrSet(effectObject->work[0], 0x40000001);
            break;
        case 3:
            BoardModelMotionTimeSet(effectObject->work[0], 110.0f);
            break;
        case 4:
            boardRotation = BoardModelRotYGet(dolphinBoardModel);
            BoardModelRotYSet(effectObject->work[0], boardRotation);
            BoardModelAttrSet(effectObject->work[0], 0x40000001);
            BoardModelPosGet(dolphinBoardModel, &boardPosition);
            effectObject->rot.x = effectPosition->x - boardPosition.x;
            effectObject->rot.y = effectPosition->y - boardPosition.y;
            effectObject->rot.z = effectPosition->z - boardPosition.z;
            BoardModelLayerSet(effectObject->work[0], 2);
            break;
        case 5:
            boardRotation = BoardModelRotYGet(dolphinBoardModel);
            BoardModelRotYSet(effectObject->work[0], boardRotation);
            BoardModelAttrSet(effectObject->work[0], 0x40000001);
            BoardModelPosGet(dolphinBoardModel, &boardPosition);
            effectObject->rot.x = effectPosition->x - boardPosition.x;
            effectObject->rot.y = effectPosition->y - boardPosition.y;
            effectObject->rot.z = effectPosition->z - boardPosition.z;
            break;
    }
    return effectObject;
}

void W05DolphinEffectUpdate(OMOBJ *object)
{
    Vec position;

    if (object->work[1] != 0) {
        if (object->work[2] != 4) {
            BoardModelKill(object->work[0]);
            omDelObjEx(boardObjMan, object);
            return;
        }
        BoardModelAttrReset(object->work[0], 0x40000001);
        BoardModelMotionStartEndSet(object->work[0], 0x14, 0x50);
        if (BoardModelMotionEndCheck(object->work[0]) != 0) {
            BoardModelKill(object->work[0]);
            omDelObjEx(boardObjMan, object);
        }
    }
    else {
        switch (object->work[2]) {
            case 0:
            case 1:
            case 3:
                if (BoardModelMotionEndCheck(object->work[0]) != 0) {
                    BoardModelKill(object->work[0]);
                    omDelObjEx(boardObjMan, object);
                    return;
                }
                break;
            case 4:
                if (BoardModelMotionTimeGet(object->work[0]) >= 20.0f) {
                    BoardModelMotionStartEndSet(object->work[0], 0x14, 0x3C);
                }
                BoardModelPosGet(dolphinBoardModel, &position);
                position.x += object->rot.x;
                position.y += object->rot.y;
                position.z += object->rot.z;
                BoardModelPosSetV(object->work[0], &position);
                break;
            case 5:
                BoardModelPosGet(dolphinBoardModel, &position);
                position.x += object->rot.x;
                position.y += object->rot.y;
                position.z += object->rot.z;
                BoardModelPosSetV(object->work[0], &position);
                break;
        }
    }
}
