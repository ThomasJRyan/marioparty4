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

// data
s32 itemPlayerMotions[2] = { 0x79, 0x7A };
char *itemObjectNames[3] = { "suika1", "suika2", "suika3" };

// bss
s16 itemPlayerIndex;
s16 itemModels[3];
s16 itemModelMotions[3];
s16 itemMotionModels[3];
s16 itemEffectModel;
s16 itemPlayerMotionIds[2];
s16 itemMinigameExitRequested;
HUPROCESS *itemMinigameProcess;
OMOBJ *itemControllerObject;
OMOBJ *itemMinigameObject;
OMOBJ *itemInputObject;
s32 itemMinigameState;
s32 itemParticleModels[3];
ANIMDATA *itemAnimationData;

void W05InitializeItemMinigame(void)
{
    BoardModelAttrSet(*boardMapModelIds, 0x40000002U);
    itemMinigameProcess = NULL;
    itemAnimationData = NULL;
}

void W05StartItemMinigame(void)
{
    Vec scale;
    Vec position;
    s32 playerIndex;
    s32 loopIndex;

    itemPlayerIndex = GWSystem.player_curr;
    playerIndex = GWPlayer[itemPlayerIndex].character;

    for (loopIndex = 0; loopIndex < 2; loopIndex++) {
        itemPlayerMotionIds[loopIndex] = BoardPlayerMotionCreate(itemPlayerIndex, itemPlayerMotions[loopIndex] | characterIdleMotions[GWPlayer[itemPlayerIndex].character]);
    }
    itemEffectModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x13), NULL, 0);
    BoardModelVisibilitySet(itemEffectModel, 0);
    scale.x = scale.y = scale.z = 3.33333f;
    BoardModelScaleSetV(itemEffectModel, &scale);
    Hu3DModelObjPosGet(BoardModelIDGet(boardMapModelIds[0]), "chara1", &scale);
    BoardPlayerPosGet(itemPlayerIndex, &position);
    BoardPlayerPosLerpStart(itemPlayerIndex, &position, &scale, 0x1E);

    while (GWPlayer[itemPlayerIndex].moving != 0) {
        HuPrcVSleep();
    }
    BoardPlayerMotBlendSet(itemPlayerIndex, 0, 0xF);

    while (BoardPlayerMotBlendCheck(itemPlayerIndex) == 0) {
        HuPrcVSleep();
    }

    HuPrcSleep(0x3C);
    BoardAudSeqPause(0, 1, 0x3E8);
    HuAudFXPlay(0x30D);
    W05SetItemMinigameInputState();
    HuAudFXPlay(0x491);
    omVibrate(itemPlayerIndex, 0x7FFF, 6, 6);
    BoardModelHookSet(boardMapModelIds[0], "chara1", BoardPlayerModelGet(itemPlayerIndex));
    BoardModelMotionStartEndSet(boardMapModelIds[0], 0x1E, 0x5A);
    BoardModelAttrReset(boardMapModelIds[0], 0x40000002);
    BoardMusStart(1, 0x22, 0x7F, 0);
    BoardPlayerMotionShiftSet(itemPlayerIndex, itemPlayerMotionIds[0], 0.0f, 30.0f, HU3D_MOTATTR_LOOP);
    HuPrcSleep(REFRESH_RATE/2);
    BoardModelVisibilitySet(itemEffectModel, 1);
    BoardModelHookSet(BoardPlayerModelGet(itemPlayerIndex), "a-itemhook-r", itemEffectModel);
    Hu3DModelObjPosGet(BoardModelIDGet(*boardMapModelIds), "chara1", &position);
    Hu3DModelObjPosGet(BoardModelIDGet(BoardPlayerModelGet(itemPlayerIndex)), "a-itemhook-r", &scale);
    VECAdd(&scale, &position, &scale);
    scale.y += 25.0f;
    CharEffectLayerSet(7);
    CharEffectSmokeCreate(-1, &scale);
    HuPadRumbleStop(GWPlayer[itemPlayerIndex].port);
    BoardModelAttrSet(boardMapModelIds[0], 0x40000002);
    HuPrcSleep(0x1E);
    BoardModelAttrReset(boardMapModelIds[0], 0x40000002);
    omVibrate(itemPlayerIndex, 0x7FFF, 6, 6);
    HuAudFXPlay(0x492);

    while (BoardModelMotionTimeGet(boardMapModelIds[0]) < 90.0f) {
        HuPrcVSleep();
    }
    BoardModelMotionTimeSet(boardMapModelIds[0], 90.0f);
    HuPadRumbleStop(GWPlayer[itemPlayerIndex].port);
    BoardModelHookReset(boardMapModelIds[0]);

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        BoardModelHookSet(boardMapModelIds[0], itemObjectNames[loopIndex], itemModelMotions[loopIndex]);
    }
    Hu3DModelObjPosGet(BoardModelIDGet(boardMapModelIds[0]), "chara1", &position);
    BoardPlayerPosSetV(itemPlayerIndex, &position);
    HuPrcSleep(0x3C);
    itemControllerObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W05PollItemMinigameEnd);
    itemInputObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W05PollItemMinigameSequence);
    itemMinigameObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, NULL);
    itemMinigameObject->data = HuMemDirectMallocNum(HEAP_HEAP, 0x14, HU_MEMNUM_OVL);
    if (itemAnimationData == NULL) {
        itemAnimationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_EFFECT, 0x00));
        HuSprAnimLock(itemAnimationData);
    }
    itemMinigameExitRequested = 0;
}

void W05PollItemMinigameEnd(OMOBJ *object)
{
    if ((itemMinigameExitRequested != 0) && (BoardMGDoneFlagGet() == 1)) {
        W05PlayItemSpaceExitAnimation();
        W05CleanupItemMinigame();
        W05FinishItemMinigameCleanup();
        omDelObjEx(boardObjMan, object);
    }
}

void W05ExitItemMinigame(void)
{
    itemMinigameExitRequested = 1;
    BoardMGExit();
}

void W05CleanupItemMinigame(void)
{
    s32 loopIndex;

    BoardModelHookReset(boardMapModelIds[0]);

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        BoardModelKill(itemMotionModels[loopIndex]);
        BoardModelKill(itemModelMotions[loopIndex]);
    }
    if (itemAnimationData != 0) {
        HuSprAnimKill(itemAnimationData);
        itemAnimationData = NULL;
    }
}

void W05FinishItemMinigameCleanup(void)
{
    omDelObjEx(boardObjMan, itemMinigameObject);
    BoardMGDoneFlagSet(0);
}

void W05WaitForItemModelAnimation(void)
{

    while (BoardModelMotionTimeGet(boardMapModelIds[0]) < 30.0f) {
        HuPrcVSleep();
    }
    BoardModelAttrSet(boardMapModelIds[0], 0x40000002U);
    BoardModelMotionTimeSet(boardMapModelIds[0], 30.0f);
    itemMinigameProcess = HuPrcCreate(W05RunItemMinigameIntro, 0x2004, 0x1000, 0);
    HuPrcDestructorSet2(itemMinigameProcess, W05DestroyItemMinigameModels);
    itemMinigameProcess->user_data = (void *)7;
    HuPrcEnd();
}

#if VERSION_JP
#define MDL_ID_SHIFT 0
#else
#define MDL_ID_SHIFT 1
#endif

s32 itemMinigameModels[14] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 0x6C + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x6D + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x6E + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x6F + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x70 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x71 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x72 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x73 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x75 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x76 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x77 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x78 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x79 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 0x7A + MDL_ID_SHIFT),
};

s16 itemChoiceSequence[92] = { 0, 1, -1, 2, 0, -1, 1, 2, 0, -1, 2, 1, 0, 2, -1, 1, 0, 2, 1, -1, 1, 2, -1, 0, 1, 2, 0, -1, 1, -1, 0, 2, 1, -1, 2, 0, 1, 0,
    2, -1, 1, 0, -1, 2, 2, -1, 0, 1, 2, -1, 1, 0, 2, 0, 1, -1, 2, 0, -1, 1, 2, 1, -1, 0, 2, 1, 0, -1, -1, 0, 1, 2, -1, 0, 2, 1, -1, 1, 2, 0, -1, 1, 0,
    2, -1, 2, 0, 1, -1, 2, 1, 0 };

F32 itemChoiceRotations[3] = { -90.0f, 0.0f, 90.0f };

void W05ShuffleItemChoices(s16 itemChoices[3])
{
    F32 floatValue;
    s32 loopIndex;

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        itemModels[loopIndex] = itemChoices[loopIndex];
    }

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        itemMotionModels[loopIndex] = BoardModelCreate(itemMinigameModels[itemModels[loopIndex]], NULL, 0);
        if (itemModels[loopIndex] == 0xB) {
            BoardModelAttrSet(itemMotionModels[loopIndex], 0x40000001U);
        }
        if (itemModels[loopIndex] == 5) {
            floatValue = -50.0f;
            Hu3DData[BoardModelIDGet(itemMotionModels[loopIndex])].mtx[1][3] = floatValue;
        }
        if (itemModels[loopIndex] == 4) {
            BoardModelMotionSpeedSet(itemMotionModels[loopIndex], 0.0f);
        }
        BoardModelVisibilitySet(itemMotionModels[loopIndex], 0);
    }

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        itemModelMotions[loopIndex] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x12), NULL, 1);
        BoardModelHookSet(boardMapModelIds[0], itemObjectNames[loopIndex], itemModelMotions[loopIndex]);
        BoardModelAttrSet(itemModelMotions[loopIndex], 0x40000002);
        BoardModelMotionTimeSet(itemModelMotions[loopIndex], 10.0f);
    }

    HuAudFXPlay(0x488);
    BoardModelMotionTimeSet(boardMapModelIds[0], 0.0f);
    BoardModelMotionStartEndSet(boardMapModelIds[0], 0, 0x1E);
    BoardModelAttrReset(boardMapModelIds[0], 0x40000002);
    HuPrcCreate(W05WaitForItemModelAnimation, 0x2004, 0x1000, 0);
}

void W05PlayItemSpaceExitAnimation(void)
{
    HuAudFXPlay(0x489);
    BoardModelMotionStartEndSet(boardMapModelIds[0], 0x5A, 0x78);
    BoardModelMotionTimeSet(boardMapModelIds[0], 90.0f);
    BoardModelAttrReset(boardMapModelIds[0], 0x40000002);

    while (BoardModelMotionEndCheck(boardMapModelIds[0]) == 0) {
        W05TurnBoardModelTowardAngle(boardMapModelIds[6], 0.0f, 6.0f);
        HuPrcVSleep();
    }

    while (W05TurnBoardModelTowardAngle(boardMapModelIds[6], 0.0f, 6.0f) == 0) {
        HuPrcVSleep();
    }
}

void W05PollItemMinigameSequence(OMOBJ *object)
{
    if (object->work[0] == 0) {
        object->work[0] = MGSeqCreate(3, 0);
        return;
    }
    if (MGSeqStatGet(object->work[0]) == 0) {
        object->objFunc = NULL;
        itemMinigameObject->objFunc = W05UpdateItemTargets;
    }
}

void W05PollItemInput(OMOBJ *object)
{
    HUPROCESS *process;
    if (((u32)itemMinigameProcess->user_data & 0x10) != 0) {
        process = HuPrcCreate(W05RunItemMinigameRound, 0x2004, 0x1800, 0);
        process->user_data = object;
        object->objFunc = NULL;
    }
}

void W05RunItemMinigameRound(void)
{
    Vec movementVector;
    Vec movementVector2;
    Vec movementVector3;
    Vec scale;
    F32 floatValue;
    F32 floatValue2;
    F32 frameCount;
    s32 integerValue;
    s32 integerValue2;
    s32 integerValue3;
    s32 loopIndex;
    OMOBJ *effectObject;

    effectObject = HuPrcCurrentGet()->user_data;
    BoardPlayerPosGet(itemPlayerIndex, &movementVector);
    integerValue2 = effectObject->work[0];
    BoardModelPosGet(itemMotionModels[integerValue2], &movementVector2);
    movementVector.y = movementVector2.y = 0.0f;
    VECSubtract(&movementVector, &movementVector2, &movementVector3);
    floatValue2 = VECMag(&movementVector3);
    VECScale(&movementVector3, &movementVector3, 0.016666668);

    for (loopIndex = 0; loopIndex < 0x3C; loopIndex++) {
        BoardModelPosGet(itemMotionModels[integerValue2], &movementVector2);
        VECAdd(&movementVector2, &movementVector3, &movementVector2);
        BoardModelPosSetV(itemMotionModels[integerValue2], &movementVector2);
        HuPrcVSleep();
    }
    BoardModelPosGet(itemMotionModels[integerValue2], &movementVector2);
    movementVector2.x = movementVector.x;
    movementVector2.z = movementVector.z;
    BoardModelPosSetV(itemMotionModels[integerValue2], &movementVector2);
    BoardPlayerMotBlendSet(itemPlayerIndex, 0, 0xF);

    while (BoardPlayerMotBlendCheck(itemPlayerIndex) == 0) {
        W05TurnBoardModelTowardAngle(boardMapModelIds[6], 90.0f, 6.0f);
        HuPrcVSleep();
    }

    while (W05TurnBoardModelTowardAngle(boardMapModelIds[6], 90.0f, 6.0f) == 0) {
        HuPrcVSleep();
    }
    BoardWinCreate(2, MAKE_MESSID(39, 12), 7);
    BoardWinInsertMesSet(itemModels[integerValue2] + 0x80000, 0);
    BoardWinWait();
    BoardWinKill();
    HuPrcKill(itemMinigameProcess);
    itemMinigameProcess = NULL;
    HuPrcVSleep();
    HuAudFXPlay(0x30D);
    BoardModelPosGet(itemMotionModels[integerValue2], &movementVector2);
    BoardModelScaleGet(itemMotionModels[integerValue2], &scale);
    frameCount = scale.x;
    floatValue = -0.075f;
    floatValue2 = (movementVector.y - movementVector2.y) / 30.0f;
    integerValue3 = 0;
    HuAudFXFadeOut(itemMinigameState, 0x1F3);
    while (integerValue3 == 0) {
        movementVector2.y += floatValue2;
        frameCount += floatValue;
        if (frameCount < 0.0f) {
            frameCount = 0.0f;
            integerValue3 = 1;
        }
        scale.x = scale.y = scale.z = frameCount;
        BoardModelPosSetV(itemMotionModels[integerValue2], &movementVector2);
        BoardModelScaleSetV(itemMotionModels[integerValue2], &scale);
        HuPrcVSleep();
    }
    HuAudFXStop(itemMinigameState);
    BoardModelVisibilitySet(itemMotionModels[integerValue2], 0);
    BoardPlayerItemAdd(itemPlayerIndex, itemModels[integerValue2]);
    omVibrate(itemPlayerIndex, 0xC, 6, 6);
    integerValue = HuAudSStreamPlay(2);
    BoardPlayerMotionShiftSet(itemPlayerIndex, 7, 0.0f, 5.0f, HU3D_MOTATTR_NONE);

    while (BoardPlayerMotionEndCheck(itemPlayerIndex) == 0) {
        HuPrcVSleep();
    }
    HuPrcSleep(REFRESH_RATE/2);

    while (HuAudSStreamStatGet(integerValue) != 0) {
        HuPrcVSleep();
    }
    BoardAudSeqPause(0, 0, 0x3E8);
    BoardWinCreate(2, MAKE_MESSID(39, 13), 7);
    BoardWinWait();
    BoardWinKill();
    effectObject->objFunc = &W05ExitItemMinigameFromObject;
    HuPrcEnd();
}

void W05ExitItemMinigameFromObject(OMOBJ *object)
{
    itemMinigameExitRequested = 1;
    BoardMGExit();
    omDelObjEx(boardObjMan, object);
}

void W05UpdateItemMinigameState(ItemMinigameState *state)
{
    if (--state->roundTimer == 0) {
        state->roundTimer = 0x3C;
        if (--state->remainingRounds >= 0) {
            MGSeqParamSet(state->sequenceId, 1, state->remainingRounds);
        }
    }
}

void W05UpdateItemTargets(OMOBJ *object)
{
    F32 floatValue[2];
    s32 loopIndex;
    s32 integerValue;
    ItemMinigameState *minigameState;

    minigameState = object->data;
    minigameState->remainingRounds = 5;
    minigameState->roundTimer = 0x3C;
    minigameState->sequenceId = MGSeqCreate(1, minigameState->remainingRounds, 0x120, 0x40);
    HuWinMesMaxSizeGet(1, floatValue, MAKE_MESSID(0x27, 0x0E));
    minigameState->messageWindowId = HuWinCreate(-10000.0f, 344.0f, floatValue[0], floatValue[1], 0);
    HuWinMesSet(minigameState->messageWindowId, MAKE_MESSID(0x27, 0x0E));
    HuWinBGTPLvlSet(minigameState->messageWindowId, 0.0f);
    HuWinMesSpeedSet(minigameState->messageWindowId, 0);
    minigameState->selectedChoice = 1;

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        minigameState->choiceToItem[loopIndex] = -1;
    }

    for (loopIndex = 0; loopIndex < 2; loopIndex++) {
        do {
            integerValue = frandmod(3);
        } while (minigameState->choiceToItem[integerValue] != -1);
        minigameState->choiceToItem[integerValue] = loopIndex;
    }

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        if (minigameState->choiceToItem[loopIndex] == -1) {
            minigameState->choiceToItem[loopIndex] = 2;
        }
    }
    minigameState->cpuInputCountdown = frandmod(5);
    minigameState->cpuInputDelay = (frandmod(0x1E) + 0xF);
    object->objFunc = W05UpdateItemMovement;
}

void W05UpdateItemMovement(OMOBJ *object)
{
    s32 integerValue;
    s32 integerValue2;
    s32 integerValue3;
    F32 frameCount;
    s32 integerValue4;
    s32 integerValue5;
    ItemMinigameState *minigameState;

    minigameState = object->data;
    W05UpdateItemMinigameState(minigameState);
    integerValue4 = GWPlayer[itemPlayerIndex].port;
    if (GWPlayer[itemPlayerIndex].com != 0) {
        W05GetItemMinigameResult(minigameState, &integerValue, &integerValue2, &integerValue3);
    }
    else {
        integerValue = HuPadStkX[integerValue4];
        integerValue2 = HuPadStkY[integerValue4];
        integerValue3 = HuPadBtnDown[integerValue4];
    }

    if (((integerValue3 & 0x100) != 0) || (minigameState->remainingRounds < 0)) {
        MGSeqParamSet(minigameState->sequenceId, 2, -1);
        HuWinKill(minigameState->messageWindowId);
        BoardPlayerMotionStart(itemPlayerIndex, itemPlayerMotionIds[1], 0);
        object->objFunc = W05UpdateItemSelection;
        return;
    }
    if ((integerValue2 != 0) || (integerValue != 0)) {
        integerValue5 = -1;
        frameCount = atan2d(integerValue2, integerValue);
        if (frameCount < 0.0f) {
            frameCount += 360.0f;
        }

        if ((frameCount > 170.0f) && (frameCount < 190.0f)) {
            integerValue5 = 0;
        }
        else if ((frameCount > 260.0f) && (frameCount < 280.0f)) {
            integerValue5 = 1;
        }
        else if ((frameCount < 10.0f) || (frameCount > 350.0f)) {
            integerValue5 = 2;
        }
        if (integerValue5 != -1) {
            minigameState->selectedChoice = minigameState->choiceToItem[integerValue5];
            BoardPlayerRotYSet(itemPlayerIndex, itemChoiceRotations[minigameState->selectedChoice]);
        }
    }
}

void W05UpdateItemSelection(OMOBJ *object)
{
    s32 integerValue;
    ItemMinigameState *minigameState;

    minigameState = object->data;
    if (BoardPlayerMotionTimeGet(itemPlayerIndex) >= 30.0f) {
        BoardAudSeqFadeOut(1, 0x64);
        HuAudFXPlay(0x486);
        HuAudFXPlay(0x487);
        integerValue = (1 << minigameState->selectedChoice);
        integerValue |= 0x1C0;
        itemMinigameProcess = HuPrcCreate(W05RunItemMinigameIntro, 0x2004, 0x4000, 0);
        HuPrcDestructorSet2(itemMinigameProcess, W05DestroyItemMinigameModels);
        itemMinigameProcess->user_data = (void *)integerValue;
        itemInputObject->work[0] = minigameState->selectedChoice;
        object->objFunc = W05MoveItemModels;
    }
}

void W05MoveItemModels(OMOBJ *object)
{
    Vec vector;
    Vec vector2;
    F32 animationTime;

    animationTime = BoardPlayerMotionMaxTimeGet(itemPlayerIndex);
    if (BoardPlayerMotionTimeGet(itemPlayerIndex) >= animationTime) {
        BoardPlayerMotionStart(itemPlayerIndex, itemPlayerMotionIds[0], 0x40000001);
        Hu3DModelObjPosGet(BoardModelIDGet(boardMapModelIds[0]), "chara1", &vector2);
        Hu3DModelObjPosGet(BoardModelIDGet(BoardPlayerModelGet(itemPlayerIndex)), "a-itemhook-r", &vector);
        VECAdd(&vector, &vector2, &vector);
        vector.y += 25.0f;
        CharEffectLayerSet(7);
        CharEffectSmokeCreate(-1, &vector);
        object->work[0] = 0xA;
        object->objFunc = W05UpdateItemMinigameResult;
    }
}

void W05UpdateItemMinigameResult(OMOBJ *object)
{
    s32 loopIndex;

    if (object->work[0] == 0) {
        BoardModelHookReset(BoardPlayerModelGet(itemPlayerIndex));
        BoardPlayerMotionShiftSet(itemPlayerIndex, 1, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        object->objFunc = NULL;
        itemInputObject->objFunc = W05PollItemInput;
        BoardModelKill(itemEffectModel);

        for (loopIndex = 0; loopIndex < 2; loopIndex++) {
            BoardPlayerMotionKill(itemPlayerIndex, itemPlayerMotionIds[loopIndex]);
        }
        return;
    }
    object->work[0] -= 1;
}

void W05GetItemMinigameResult(ItemMinigameState *state, s32 *stickX, s32 *stickY, s32 *buttonFlags)
{
    *stickX = *stickY = *buttonFlags = 0;
    if (state->cpuInputDelay != 0) {
        state->cpuInputDelay -= 1;
        return;
    }
    state->cpuInputDelay = frandmod(0xF) + 10;
    if (state->cpuInputCountdown == 0) {
        *buttonFlags = 0x100;
        return;
    }
    state->cpuInputCountdown -= 1;
    if (frandBool() != 0) {
        *stickX = (frandBool() * 0x8C) - 0x46;
        return;
    }
    *stickY = (frandBool() * 0x8C) - 0x46;
}

void *W05GetItemMinigameInput(void)
{
    if (itemMinigameProcess == NULL) {
        return NULL;
    }
    return itemMinigameProcess->user_data;
}

void W05SetItemMinigameInputState(void)
{
    s32 userData = (s32)itemMinigameProcess->user_data;
    userData |= 8;
    itemMinigameProcess->user_data = (void *)(userData);

    while (((u32)W05GetItemMinigameInput() & 0x20) == 0) {
        HuPrcVSleep();
    }
    HuPrcKill(itemMinigameProcess);
    itemMinigameProcess = NULL;
}

void W05RunItemMinigameIntro(void)
{
    Vec position;
    Vec scale;
    F32 frameCount;
    F32 floatValue;
    F32 frameCount2;
    F32 floatValue2;
    u32 integerValue;
    s32 loopIndex;
    s32 loopIndex2;
    s32 integerValue2;
    HU3DPARTICLE *objectPointer;

    integerValue = (u32)HuPrcCurrentGet()->user_data;

    for (loopIndex2 = 0; loopIndex2 < 3; loopIndex2++) {
        integerValue2 = (1 << loopIndex2);
        if ((integerValue & integerValue2) != 0) {
            BoardModelAttrReset(itemModelMotions[loopIndex2], 0x40000002);
            if ((integerValue & 0x80) != 0) {
                BoardModelMotionStartEndSet(itemModelMotions[loopIndex2], 0, 0x28);
                BoardModelMotionTimeSet(itemModelMotions[loopIndex2], 0.0f);
            }
            else {
                BoardModelMotionStartEndSet(itemModelMotions[loopIndex2], 0xA, 0x28);
                BoardModelMotionTimeSet(itemModelMotions[loopIndex2], 10.0f);
            }
        }
    }
    if ((integerValue & 0x80) != 0) {
        HuPrcSleep(((REFRESH_RATE*5)+6)/12);
    }
    else {
        HuPrcSleep((REFRESH_RATE+2)/4);
    }
    scale.x = scale.y = scale.z = 0.0f;

    for (loopIndex2 = 0; loopIndex2 < 3; loopIndex2++) {
        integerValue2 = (1 << loopIndex2);
        if ((integerValue & integerValue2) != 0) {
            BoardModelVisibilitySet(itemMotionModels[loopIndex2], 1);
            BoardModelScaleSetV(itemMotionModels[loopIndex2], &scale);
            Hu3DModelObjPosGet(BoardModelIDGet(boardMapModelIds[0]), itemObjectNames[loopIndex2], &position);
            BoardModelPosSetV(itemMotionModels[loopIndex2], &position);
            if ((integerValue & 0x40) != 0) {
                itemParticleModels[loopIndex2] = Hu3DParticleCreate(itemAnimationData, 0x64);
                objectPointer = Hu3DData[itemParticleModels[loopIndex2]].hookData;
                objectPointer->emitCnt = itemMotionModels[loopIndex2];
                Hu3DParticleHookSet(itemParticleModels[loopIndex2], w05UpdateItemParticle);
                Hu3DParticleColSet(itemParticleModels[loopIndex2], 0xFF, 0xFF, 0);
                Hu3DModelPosSet(itemParticleModels[loopIndex2], 0.0f, 0.0f, 0.0f);
                Hu3DParticleBlendModeSet(itemParticleModels[loopIndex2], 1);
                Hu3DModelLayerSet(itemParticleModels[loopIndex2], 2);
            }
            if ((integerValue & 0x100) != 0) {
                itemMinigameState = HuAudFXPlay(0x35F);
                HuAudFXPlay(0x310);
            }
            else {
                HuAudFXPlay(0x362);
            }
        }
    }
    floatValue = 150.0f + position.y;
    frameCount = 0.0f;
    floatValue2 = 12.0f;

    while (1) {
        for (loopIndex = 0, loopIndex2 = 0; loopIndex2 < 3; loopIndex2++) {
            integerValue2 = (1 << loopIndex2);
            if ((integerValue & integerValue2) != 0) {
                loopIndex++;
                frameCount += 0.1f;
                if (frameCount > 1.0f) {
                    frameCount = 1.0f;
                }

                scale.x = scale.y = scale.z = frameCount;
                BoardModelScaleSetV(itemMotionModels[loopIndex2], (Vec *)&frameCount); //!
                BoardModelPosGet(itemMotionModels[loopIndex2], &position);
                position.y += floatValue2;
                if (position.y >= floatValue) {
                    position.y = floatValue;
                    loopIndex -= 1;
                }
                BoardModelPosSetV(itemMotionModels[loopIndex2], &position);
            }
        }
        floatValue2 *= 0.95f;
        if (loopIndex > 0) {
            HuPrcVSleep();
        }
        else {
            break;
        }
    }
    scale.x = scale.y = scale.z = 1.0f;
    for (loopIndex2 = 0; loopIndex2 < 3; loopIndex2++) {
        integerValue2 = (1 << loopIndex2);
        if ((integerValue & integerValue2) != 0) {
            BoardModelScaleSetV(itemMotionModels[loopIndex2], &scale);
        }
    }
    integerValue = (u32)HuPrcCurrentGet()->user_data | 0x10;
    HuPrcCurrentGet()->user_data = (void *)integerValue;
    frameCount2 = 0.0f;

    while (1) {
        integerValue = (u32)HuPrcCurrentGet()->user_data;
        if ((integerValue & 8) == 0) {
            for (loopIndex2 = 0; loopIndex2 < 3; loopIndex2++) {
                integerValue2 = (1 << loopIndex2);
                if ((integerValue & integerValue2) != 0) {
                    BoardModelPosGet(itemMotionModels[loopIndex2], &position);
                    position.y += 0.25 * sind(frameCount2);
                    BoardModelPosSetV(itemMotionModels[loopIndex2], &position);
                }
            }
            frameCount2 += 5.0f;
            if (frameCount2 >= 360.0f) {
                frameCount2 -= 360.0f;
            }
            HuPrcVSleep();
        }
        else {
            break;
        }
    }
    for (loopIndex2 = 0; loopIndex2 < 3; loopIndex2++) {
        BoardModelMotionStartEndSet(itemModelMotions[loopIndex2], 0x28, 0x46);
    }
    HuPrcSleep((REFRESH_RATE*2)/9);
    floatValue2 = -8.0f;
    while (1) {
        for (loopIndex = 0, loopIndex2 = 0; loopIndex2 < 3; loopIndex2++) {
            integerValue2 = (1 << loopIndex2);
            if ((integerValue & integerValue2) != 0) {
                loopIndex++;
                frameCount -= 0.02f;
                if (frameCount < 0.0f) {
                    frameCount = 0.0f;
                }
                scale.x = scale.y = scale.z = frameCount;
                BoardModelScaleSetV(itemMotionModels[loopIndex2], (Vec *)&frameCount); //!
                BoardModelPosGet(itemMotionModels[loopIndex2], &position);
                position.y += floatValue2;
                if (position.y <= (floatValue - 150.0f)) {
                    position.y = floatValue - 150.0f;
                }
                if (BoardModelMotionTimeGet(itemModelMotions[loopIndex2]) >= 70.0f) {
                    loopIndex--;
                }
                BoardModelPosSetV(itemMotionModels[loopIndex2], &position);
            }
        }
        floatValue2 *= 0.98f;
        if (loopIndex > 0) {
            HuPrcVSleep();
        }
        else {
            break;
        }
    }

    for (loopIndex2 = 0; loopIndex2 < 3; loopIndex2++) {
        integerValue2 = (1 << loopIndex2);
        if ((integerValue & integerValue2) != 0) {
            scale.x = scale.y = scale.z = 1.0f;
            BoardModelScaleSetV(itemMotionModels[loopIndex2], &scale);
            BoardModelVisibilitySet(itemMotionModels[loopIndex2], 0);
        }
    }
    integerValue = (u32)HuPrcCurrentGet()->user_data;
    integerValue |= 0x20;
    HuPrcCurrentGet()->user_data = (void *)integerValue;
    while (1) {
        HuPrcVSleep();
    }
}

void W05DestroyItemMinigameModels(void)
{
    u32 integerValue;
    s32 integerValue2;
    s32 loopIndex;

    integerValue = (u32)HuPrcCurrentGet()->user_data;
    if ((integerValue & 0x40) != 0) {
        for (loopIndex = 0; loopIndex < 3; loopIndex++) {
            integerValue2 = (1 << loopIndex);
            if ((integerValue & integerValue2) != 0) {
                Hu3DModelKill(itemParticleModels[loopIndex]);
            }
        }
    }
}

void w05UpdateItemParticle(HU3DMODEL *model, HU3DPARTICLE *particle, F32 (*matrix)[4])
{
    Vec scale;
    HU3DPARTICLEDATA *particleData;
    F32 floatValue;
    F32 floatValue2;
    F32 floatValue3;
    s32 loopIndex;
    s32 loopIndex2;

    if (particle->count == 0) {
        particleData = particle->data;
        for (loopIndex2 = 0; loopIndex2 < particle->maxCnt; loopIndex2++, particleData++) {
            particleData->color.a = 0;
            particleData->scale = 0.0f;
        }
        particle->dataCnt = 0;
    }
    BoardModelScaleGet(particle->emitCnt, &scale);
    floatValue3 = scale.x;
    BoardModelPosGet(particle->emitCnt, &scale);

    for (loopIndex2 = 0; loopIndex2 < 0x16; loopIndex2++) {
        particleData = particle->data;

        for (loopIndex = 0; loopIndex < particle->maxCnt; loopIndex++, particleData++) {
            if (particleData->scale == 0.0f)
                break;
        }
        if (loopIndex != particle->maxCnt) {
            floatValue = 360.0f * (0.003921569f * (s32)(frand() & 0xFF));
            floatValue2 = floatValue3 * (20.0f + (60.0f * (0.003921569f * (s32)(frand() & 0xFF))));
            particleData->pos.x = scale.x + floatValue2 * sind(floatValue);
            particleData->pos.z = scale.z + floatValue2 * cosd(floatValue);
            particleData->pos.y = scale.y + (floatValue3 * (-30.0f + (40.0f * (0.003921569f * (s32)(frand() & 0xFF)))));
            particleData->vel.x = 0.5f + (3.0f * (0.003921569f * (s32)(frand() & 0xFF)));
            particleData->vel.y = 0.3f + (2.0f * (0.003921569f * (s32)(frand() & 0xFF)));
            particleData->color.a = 0xB4;
            particleData->scale = 15.0f * floatValue3;
        }
    }
    particleData = particle->data;
    for (loopIndex2 = 0; loopIndex2 < particle->maxCnt; loopIndex2++, particleData++) {
        if (0.0f != particleData->scale) {
            particleData->pos.y -= particleData->vel.x;
            particleData->scale -= particleData->vel.y;
            if (particleData->scale <= 0.0f) {
                particleData->scale = 0.0f;
            }
        }
    }
}
