#include "REL/w05Dll.h"

#include "game/audio.h"
#include "game/chrman.h"
#include "game/frand.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"


#include "game/board/audio.h"
#include "game/board/model.h"
#include "game/board/window.h"


#include "ext_math.h"


s16 coinPlayerMotions[3];
s16 coinEffectModel;
s16 coinFishModel;
s16 coinDropModels[20];
s16 coinTokenModel;
OMOBJ *coinPlayerObject;
OMOBJ *coinMinigameObject;
OMOBJ *coinControllerObject;
s16 coinRewardCount;
s16 coinMinigameExitRequested;
s16 coinFishChoice;
s16 coinPlayerIndex;
u32 coinFishObjectType;
s32 coinFishIndex;

char *coinFishObjectNames[3] = { "gyoei", "gyoei1", "gyoei2" };
s32 coinPlayerMotionIds[3] = { 0x7B, 0x7C, 0x7D };
s32 coinFishModels[3] = { DATA_MAKE_NUM(DATADIR_W05, 0x16), DATA_MAKE_NUM(DATADIR_W05, 0x17), DATA_MAKE_NUM(DATADIR_W05, 0x18) };
char *coinFishHookNames[3] = { "uo", "uo1", "uo2" };

void W05PrepareCoinBoardModel(void)
{
    BoardModelAttrSet(boardMapModelIds[1], 0x40000002U);
}

void W05AnimateCoinBoardModelIntro(void)
{
    BoardModelAttrReset(boardMapModelIds[1], 0x40000002);
    BoardModelMotionTimeSet(boardMapModelIds[1], 0.0f);

    while (BoardModelMotionTimeGet(boardMapModelIds[1]) < 60.0f) {
        HuPrcVSleep();
    }
    BoardModelAttrSet(boardMapModelIds[1], 0x40000001);
    BoardModelMotionStartEndSet(boardMapModelIds[1], 0x3C, 0x64);
}

void W05AnimateCoinBoardModelOutro(void)
{
    BoardModelAttrReset(boardMapModelIds[1], 0x40000001);
    BoardModelMotionStartEndSet(boardMapModelIds[1], 0, 0xA0);
}

void W05StartCoinMinigame(void)
{
    Vec scale;
    s32 loopIndex;

    coinPlayerIndex = GWSystem.player_curr;

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        coinPlayerMotions[loopIndex] = BoardPlayerMotionCreate(coinPlayerIndex, coinPlayerMotionIds[loopIndex] | characterIdleMotions[GWPlayer[coinPlayerIndex].character]);
    }
    scale.x = scale.y = scale.z = 1.0f;
    coinEffectModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x15), NULL, 0);
    BoardModelScaleSetV(coinEffectModel, &scale);
    BoardModelVisibilitySet(coinEffectModel, 0);
    coinTokenModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x2C), NULL, 0);
    BoardModelVisibilitySet(coinTokenModel, 0);
    coinFishChoice = frandmod(3U);
    coinFishModel = BoardModelCreate(coinFishModels[coinFishChoice], NULL, 0);
    BoardModelVisibilitySet(coinFishModel, 0);

    for (loopIndex = 0; loopIndex < 0x14; loopIndex++) {
        coinDropModels[loopIndex] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x0A), NULL, 1);
        BoardModelVisibilitySet(coinDropModels[loopIndex], 0);
    }
    BoardPlayerPostTurnHookSet(coinPlayerIndex, (s32(*)())W05FinishCoinMinigame);
    coinMinigameObject = omAddObjEx(boardObjMan, 0x101, 0U, 0U, -1, W05PollCoinMinigameEnd);
    coinPlayerObject = omAddObjEx(boardObjMan, 0x101, 0U, 0U, -1, W05PollCoinMinigameSequence);
    coinRewardCount = 0;
    coinMinigameExitRequested = 0;
}

void W05PollCoinMinigameEnd(OMOBJ *object)
{
    if ((coinMinigameExitRequested != 0) && (BoardMGDoneFlagGet() == 1)) {
        W05CleanupCoinMinigame();
        omDelObjEx(boardObjMan, object);
    }
}

void W05ExitCoinMinigame(void)
{
    coinMinigameExitRequested = 1;
    BoardMGExit();
}

void W05CleanupCoinMinigame(void)
{
    s32 loopIndex;

    omDelObjEx(boardObjMan, coinPlayerObject);

    for (loopIndex = 0; loopIndex < 0x14; loopIndex++) {
        BoardModelKill(coinDropModels[loopIndex]);
    }
    BoardModelKill(coinTokenModel);
    BoardMGDoneFlagSet(0);
}

void W05FinishCoinMinigame(void)
{
    HSFOBJECT *fishObject;
    omDelObjEx(boardObjMan, coinControllerObject);
    BoardModelKill(coinFishModel);
    fishObject = Hu3DModelObjPtrGet(BoardModelIDGet(boardMapModelIds[1]), coinFishObjectNames[coinFishIndex]);
    fishObject->type = coinFishObjectType;
    BoardModelAttrReset(boardMapModelIds[1], 0x40000004);
}

void W05PollCoinMinigameSequence(OMOBJ *object)
{
    if (object->work[0] == 0) {
        object->work[0] = MGSeqCreate(3, 0);
        return;
    }
    if (MGSeqStatGet(object->work[0]) == 0) {
        object->objFunc = W05UpdateCoinMinigameObject;
    }
}

void W05UpdateCoinModelPosition(CoinMinigameState *state)
{
    Vec position;
    Vec vector;

    position = state->fishPositions[state->selectedFishIndex];
    position.y += 50.0f;
    BoardModelPosSetV(state->fishModelId, &position);
}

void W05UpdateCoinMinigameObject(OMOBJ *object)
{
    Vec scale;
    Vec position;
    F32 messageSize[2];
    s32 loopIndex;
    CoinMinigameState *minigameState;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, 0x34, HU_MEMNUM_OVL);
    minigameState = object->data;
    minigameState->inputDelay = 5;
    minigameState->roundTimer = 0x3C;
    minigameState->sequenceId = MGSeqCreate(1, minigameState->inputDelay, 0x120, 0x40);
    HuWinMesMaxSizeGet(1, messageSize, MAKE_MESSID(0x27, 0x0E));
    minigameState->messageWindowId = HuWinCreate(-10000.0f, 368.0f, messageSize[0], messageSize[1], 0);
    HuWinMesSet(minigameState->messageWindowId, MAKE_MESSID(0x27, 0x0E));
    HuWinBGTPLvlSet(minigameState->messageWindowId, 0.0f);
    HuWinMesSpeedSet(minigameState->messageWindowId, 0);
    minigameState->selectedFishIndex = 1;
    minigameState->fishInputDelay = frandmod(0x1EU) + 15;
    minigameState->fishMissCount = 0;

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        Hu3DModelObjPosGet(BoardModelIDGet(boardMapModelIds[1]), coinFishHookNames[loopIndex], &minigameState->fishPositions[loopIndex]);
    }
    minigameState->fishModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x19), NULL, 0);
    BoardModelAttrSet(minigameState->fishModelId, 0x40000001);
    scale.x = scale.y = scale.z = 1.5f;
    BoardModelScaleSetV(minigameState->fishModelId, &scale);
    position = minigameState->fishPositions[minigameState->selectedFishIndex];
    position.y += 50.0f;
    BoardModelPosSetV(minigameState->fishModelId, &position);
    object->objFunc = W05UpdateCoinPickup;
}

void W05UpdateCoinPickup(OMOBJ *object)
{
    Vec position;
    Vec vector;
    Vec position2;
    u16 buttonFlags;
    s16 stickDirection;
    F32 playerFacingAngle;
    u16 integerValue3;
    CoinMinigameState *minigameState;

    minigameState = object->data;
    if (--minigameState->roundTimer == 0) {
        minigameState->roundTimer = 0x3C;
        if (--minigameState->inputDelay >= 0) {
            MGSeqParamSet(minigameState->sequenceId, 1, minigameState->inputDelay);
        }
    }
    BoardPlayerPosGet(coinPlayerIndex, &position);
    integerValue3 = GWPlayer[coinPlayerIndex].port;
    if (GWPlayer[coinPlayerIndex].com != 0) {
        W05UpdateCoinResult(object, &buttonFlags, &stickDirection);
    }
    else {
        stickDirection = HuPadDStkRep[integerValue3];
        buttonFlags = HuPadBtnDown[integerValue3];
    }
    if (((buttonFlags & 0x100) != 0) || (minigameState->inputDelay < 0)) {
        HuAudFXPlay(2);
        MGSeqParamSet(minigameState->sequenceId, 2, -1);
        HuWinKill(minigameState->messageWindowId);
        BoardModelKill(minigameState->fishModelId);
        BoardPlayerMotionStart(coinPlayerIndex, coinPlayerMotions[0], 0);
        BoardPlayerMotionSpeedSet(coinPlayerIndex, 0.5f);
        object->objFunc = W05UpdateCoinMovement;
    }
    else if ((minigameState->selectedFishIndex != 0) && ((stickDirection & 1) != 0)) {
        HuAudFXPlay(0);
        minigameState->selectedFishIndex--;
    }
    else if ((minigameState->selectedFishIndex != 2) && ((stickDirection & 2) != 0)) {
        HuAudFXPlay(0);
        minigameState->selectedFishIndex++;
    }
    vector = minigameState->fishPositions[minigameState->selectedFishIndex];
    playerFacingAngle = atan2d(vector.x - position.x, vector.z - position.z);
    BoardPlayerRotYSet(coinPlayerIndex, playerFacingAngle);
    position2 = minigameState->fishPositions[minigameState->selectedFishIndex];
    position2.y += 50.0f;
    BoardModelPosSetV(minigameState->fishModelId, &position2);
}

void W05UpdateCoinMovement(OMOBJ *object)
{
    Vec position;
    Vec vector;
    F32 animationTime;

    animationTime = BoardPlayerMotionMaxTimeGet(coinPlayerIndex);
    if (BoardPlayerMotionTimeGet(coinPlayerIndex) >= animationTime) {
        HuAudFXPlay(0x350);
        BoardPlayerPosGet(coinPlayerIndex, &position);
        Hu3DModelObjPosGet(BoardModelIDGet(BoardPlayerModelGet(coinPlayerIndex)), "a-itemhook-r", &vector);
        vector.x -= 20.0f;
        vector.y += 25.0f;
        vector.z += 80.0f;
        CharEffectLayerSet(7);
        CharEffectSmokeCreate(-1, &vector);
        BoardModelVisibilitySet(coinEffectModel, 1);
        BoardModelMotionTimeSet(coinEffectModel, 0.0f);
        BoardModelAttrSet(coinEffectModel, 0x40000002);
        BoardModelHookSet(BoardPlayerModelGet(coinPlayerIndex), "a-itemhook-r", coinEffectModel);
        BoardModelMotionSpeedSet(coinEffectModel, 1.5f);
        BoardPlayerMotionStart(coinPlayerIndex, coinPlayerMotions[1], 0);
        BoardPlayerMotionSpeedSet(coinPlayerIndex, 0.5f);
        switch (coinFishChoice) {
            case 0:
                omVibrate(coinPlayerIndex, 0x7FFF, 0xC, 0);
                break;
            case 1:
                omVibrate(coinPlayerIndex, 0x7FFF, 4, 2);
                break;
            case 2:
                omVibrate(coinPlayerIndex, 0x7FFF, 6, 6);
                break;
        }
        object->objFunc = W05UpdateCoinAnimation;
    }
}

void W05UpdateCoinAnimation(OMOBJ *object)
{
    F32 animationTime;

    animationTime = BoardPlayerMotionMaxTimeGet(coinPlayerIndex);
    if ((6.0f + BoardPlayerMotionTimeGet(coinPlayerIndex)) >= animationTime) {
        BoardModelMotionTimeSet(coinEffectModel, 0.0f);
        BoardModelAttrReset(coinEffectModel, 0x40000002);
        object->objFunc = W05UpdateCoinEffect;
    }
}

void W05UpdateCoinEffect(OMOBJ *object)
{
    Vec movementVector;
    Vec scale;
    HSFOBJECT *fishObject;
    CoinMinigameState *minigameState;
    OMOBJ *effectObject;
    Vec *objectPointer;
    F32 animationTime;
    F32 effectScale;

    minigameState = object->data;
    animationTime = BoardModelMotionMaxTimeGet(coinEffectModel);
    if (BoardModelMotionTimeGet(coinEffectModel) >= animationTime) {
        BoardAudSeqFadeOut(1, 0x64);
        HuAudFXPlay(0x485);
        fishObject = Hu3DModelObjPtrGet(BoardModelIDGet(boardMapModelIds[1]), coinFishObjectNames[minigameState->selectedFishIndex]);
        coinFishObjectType = fishObject->type;
        coinFishIndex = minigameState->selectedFishIndex;
        fishObject->type = -1;
        BoardModelAttrSet(coinEffectModel, 0x40000004);
        HuPadRumbleStop(GWPlayer[coinPlayerIndex].port);
        Hu3DModelObjPosGet(BoardModelIDGet(boardMapModelIds[1]), coinFishHookNames[minigameState->selectedFishIndex], &movementVector);
        BoardModelPosSetV(coinFishModel, &movementVector);
        BoardModelAttrSet(coinFishModel, 0x40000001);
        BoardModelVisibilitySet(coinFishModel, 1);
        scale.x = scale.y = scale.z = 0.8f;
        BoardModelScaleSetV(coinTokenModel, &scale);
        BoardModelPosSetV(coinTokenModel, &movementVector);
        BoardModelVisibilitySet(coinTokenModel, 1);
        BoardModelMotionTimeSet(coinTokenModel, 110.0f);
        coinControllerObject = effectObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W05FinishCoinMinigame);
        effectObject->data = HuMemDirectMallocNum(HEAP_HEAP, 0xC, HU_MEMNUM_OVL);
        objectPointer = effectObject->data;
        coinControllerObject->work[1] = 1;
        Hu3DModelObjPosGet(BoardModelIDGet(boardMapModelIds[1]), "fish", &movementVector);
        BoardModelPosGet(coinFishModel, &scale);
        movementVector.y = scale.y;
        VECSubtract(&movementVector, &scale, objectPointer);
        effectScale = VECMag(objectPointer);
        VECNormalize(objectPointer, objectPointer);
        effectScale /= 38.0f;
        VECScale(objectPointer, objectPointer, effectScale);
        objectPointer->y = 25.0f;
        effectObject->trans.x = effectScale;
        BoardPlayerMotionStart(coinPlayerIndex, coinPlayerMotions[2], 0);
        BoardPlayerMotionSpeedSet(coinPlayerIndex, 0.5f);
        BoardModelAttrReset(boardMapModelIds[1], 0x40000001);
        BoardModelMotionStartEndSet(boardMapModelIds[1], 0, 0xA0);
        object->objFunc = W05UpdateCoinMinigamePlayer;
        return;
    }
    effectScale = BoardModelMotionSpeedGet(coinEffectModel);
    effectScale -= 0.1f;
    if (effectScale < 1.0f) {
        effectScale = 1.0f;
    }
    BoardModelMotionSpeedSet(coinEffectModel, effectScale);
}

void W05UpdateCoinMinigamePlayer(OMOBJ *object)
{
    Vec vector;
    HUPROCESS *process;

    if (BoardModelMotionTimeGet(coinEffectModel) <= 0.0f) {
        Hu3DModelObjPosGet(BoardModelIDGet(BoardPlayerModelGet(coinPlayerIndex)), "a-itemhook-r", &vector);
        vector.x += 20.0f;
        vector.y += 25.0f;
        vector.z += 80.0f;
        CharEffectLayerSet(7);
        HuAudFXPlay(0x351);
        CharEffectSmokeCreate(-1, &vector);
        BoardModelHookReset(BoardPlayerModelGet(coinPlayerIndex));
        BoardModelKill(coinEffectModel);
        process = HuPrcCreate(W05RunCoinMinigame, 0x2004, 0x1800, 0);
        process->user_data = object;
        object->objFunc = NULL;
    }
}

s16 coinRewardValues[3] = { 20, 10, 5 };
s32 coinRewardMessages[3] = { MAKE_MESSID(0x27, 0x13), MAKE_MESSID(0x27, 0x14), MAKE_MESSID(0x27, 0x15) };

void W05RunCoinMinigame(void)
{
    Vec position[20];
    Vec rotation[20];
    Vec position2;
    Vec position3;
    Vec vector;
    void *objectPointer;
    char rewardCountText;
    F32 coinFallSpeed;
    OMOBJ *effectObject;
    s32 nextCoinIndex;
    s32 loopIndex;
    F32 playerFacingAngle;
    Vec *objectPointer2;

    effectObject = HuPrcCurrentGet()->user_data;
    objectPointer = effectObject->data;
    BoardPlayerPosGet(coinPlayerIndex, &position2);
    BoardModelPosGet(boardMapModelIds[5], &position3);
    playerFacingAngle = atan2d(position3.z - position2.z, position3.x - position2.x);
    BoardPlayerMotBlendSet(coinPlayerIndex, playerFacingAngle, 0xF);

    while (BoardPlayerMotBlendCheck(coinPlayerIndex) == 0) {
        HuPrcVSleep();
    }
    HuPrcSleep(0x1E);
    coinRewardCount = coinRewardValues[coinFishChoice];
    BoardAudSeqPause(0, 0, 1000);
    sprintf(&rewardCountText, "%d", coinRewardCount);
    BoardWinCreate(2, MAKE_MESSID(39, 17), 7);
    BoardWinInsertMesSet((u32)&rewardCountText, 0);
    BoardWinInsertMesSet(coinRewardMessages[coinFishChoice], 1);
    BoardWinWait();
    BoardWinKill();
    BoardPlayerMotBlendSet(coinPlayerIndex, 0, 0xF);

    while (BoardPlayerMotBlendCheck(coinPlayerIndex) == 0) {
        HuPrcVSleep();
    }
    BoardPlayerPosGet(coinPlayerIndex, &position2);

    for (loopIndex = 0; loopIndex < coinRewardCount; loopIndex++) {
        BoardModelVisibilitySet(coinDropModels[loopIndex], 1);
        position[loopIndex] = position2;
        position[loopIndex].y += (800.0f + (loopIndex * 0x78));
        position[loopIndex].x += (-25.0f + (0.003921569f * (50.0f * (s32)(u8)frand())));
        position[loopIndex].z += (-25.0f + (0.003921569f * (50.0f * (s32)(u8)frand())));
        rotation[loopIndex].x = rotation[loopIndex].z = 0.0f;
        rotation[loopIndex].y = 0.003921569f * (360.0f * (s32)(u8)frand());
        BoardModelPosSetV(coinDropModels[loopIndex], &position[loopIndex]);
        BoardModelRotSetV(coinDropModels[loopIndex], &rotation[loopIndex]);
    }
    coinFallSpeed = -1.0f;
    nextCoinIndex = 0;

    while (1) {
        for (loopIndex = 0; loopIndex < coinRewardCount; loopIndex++) {
            if (loopIndex >= nextCoinIndex) {
                position[loopIndex].y += coinFallSpeed;
                BoardModelPosSetV(coinDropModels[loopIndex], &position[loopIndex]);
                rotation[loopIndex].y += 45.0f;
                if (rotation[loopIndex].y >= 360.0f) {
                    rotation[loopIndex].y -= 360.0f;
                }
                BoardModelRotSetV(coinDropModels[loopIndex], &rotation[loopIndex]);
                if (position[loopIndex].y <= (80.0f + position2.y)) {
                    vector = position[loopIndex];
                    CharEffectCoinGlowCreate(1, &vector);
                    BoardModelVisibilitySet(coinDropModels[loopIndex], 0);
                    nextCoinIndex++;
                    BoardPlayerCoinsAdd(coinPlayerIndex, 1);
                    HuAudFXPlay(7);
                    omVibrate(coinPlayerIndex, 0xC, 6, 6);
                }
            }
        }
        coinFallSpeed *= 1.05f;
        if (coinFallSpeed < -20.0f) {
            coinFallSpeed = -20.0f;
        }
        if (nextCoinIndex == coinRewardCount) {
            BoardPlayerMotionShiftSet(coinPlayerIndex, 7, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            break;
        }
        else {
            HuPrcVSleep();
        }
    }

    while (BoardPlayerMotionEndCheck(coinPlayerIndex) == 0) {
        HuPrcVSleep();
    }
    BoardWinCreate(2, MAKE_MESSID(39, 18), 7);
    BoardWinWait();
    BoardWinKill();
    effectObject->objFunc = W05UpdateCoinMinigameTimer;
    HuPrcEnd();
}

void W05UpdateCoinMinigameTimer(OMOBJ *object)
{
    s32 loopIndex;

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        BoardPlayerMotionKill(coinPlayerIndex, coinPlayerMotions[loopIndex]);
    }
    coinMinigameExitRequested = 1;
    BoardMGExit();
    coinControllerObject->work[1] = 0;
    omDelObjEx(boardObjMan, object);
}

void W05UpdateCoinResult(OMOBJ *object, u16 *buttonFlags, s16 *fishDirection)
{
    CoinMinigameState *minigameState;

    minigameState = object->data;
    *buttonFlags = 0;
    *fishDirection = 0;
    if (minigameState->fishInputDelay != 0) {
        minigameState->fishInputDelay -= 1;
        return;
    }
    minigameState->fishInputDelay = (frandmod(0x32U) + 15);
    if (minigameState->fishMissCount == 4) {
        *buttonFlags = 0x100;
        return;
    }
    if ((s32)frandmod(0xAU) < 2) {
        *buttonFlags = 0x100;
        return;
    }
    if (minigameState->selectedFishIndex == 0) {
        *fishDirection = 2;
        return;
    }
    if (minigameState->selectedFishIndex == 2) {
        *fishDirection = 1;
        return;
    }
    if (frandBool() != 0) {
        *fishDirection = 1;
        return;
    }
    *fishDirection = 2;
}

void W05FinishCoinMinigame(OMOBJ *object)
{
    Vec vector;
    Vec movementVector;
    Vec movementVector2;
    Vec rotation;
    F32 floatValue;
    F32 floatValue2;
    F32 floatValue3;

    if (BoardIsKill() == 0) {
        Hu3DModelObjPosGet(BoardModelIDGet(boardMapModelIds[1]), "fish", &movementVector2);
        vector = *(Vec *)object->data;
        floatValue3 = object->work[0];
        floatValue = vector.y;
        vector.y = floatValue - (0.11666667f * (0.25f * (floatValue3 * floatValue3)));
        BoardModelPosGet(coinFishModel, &movementVector);
        rotation.x = 0.0f;
        rotation.y = 180.0f;
        rotation.z = 0.0f;
        BoardModelRotSetV(coinFishModel, &rotation);
        VECAdd(&movementVector, &vector, &movementVector);
        movementVector2.y = movementVector.y;
        VECSubtract(&movementVector2, &movementVector, &movementVector2);
        floatValue2 = VECMag(&movementVector2);
        Hu3DModelObjPosGet(BoardModelIDGet(boardMapModelIds[1]), "fish", &movementVector2);
        if (floatValue2 <= object->trans.x) {
            movementVector.x = movementVector2.x;
            movementVector.z = movementVector2.z;
            vector.x = vector.z = 0.0f;
            *(Vec *)object->data = vector;
        }
        if ((movementVector.y <= movementVector2.y) && (vector.y < 0.0f)) {
            movementVector.y = movementVector2.y;
            if ((0.0f == BoardModelMotionTimeGet(coinFishModel)) && (object->work[1] != 0)) {
                HuAudFXPlay(0x484);
            }
        }
        else {
            object->work[0] += 1;
        }
        BoardModelPosSetV(coinFishModel, &movementVector);
    }
}
