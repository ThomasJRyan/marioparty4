#include "REL/executor.h"
#include "REL/w03Dll.h"
#include "dolphin.h"
#include "dolphin/os/OSFastCast.h"
#include "ext_math.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/board/audio.h"
#include "game/board/com.h"
#include "game/board/window.h"
#endif
#include "game/data.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "math.h"

typedef struct ItemEffectWork {
    /* 0x00 */ struct {
        u8 isActive : 1;
        u8 itemType : 3;
        u8 isVisible : 2;
        u8 isTimed : 1;
        u8 isComplete : 1;
    };
    u8 itemId;
    u8 itemTimer;
    u8 playerIndex;
    s16 timerHandle;
    s16 modelIds[3];
} ItemEffectWork;

typedef struct ItemObjectWork {
    /* 0x00 */ struct {
        u8 isActive : 1;
        u8 itemType : 3;
        u8 isVisible : 2;
        u8 isTimed : 1;
        u8 isComplete : 1;
    };
    s8 itemId;
    s8 itemTimer;
    u8 playerIndex[3];
    s16 modelIds[4];
} ItemObjectWork;

typedef struct ItemAnimationWork {
    /* 0x00 */ char unreferencedPayload[0x10];
    /* 0x10 */ s16 unreferencedWorkValue10;
    /* 0x12 */ s16 unreferencedWorkValue12;
    /* 0x14 */ s16 unreferencedWorkValue14;
    /* 0x16 */ s16 unreferencedWorkValue16;
} ItemAnimationWork;

// external symbols

extern u16 HuPadBtnDown[4];

// function signatures
s32 BoardVecDAngleCalcRange(float *value, float min, float range);

void ItemParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void ItemParticleEffectDestroy(void);
void ItemParticleEffectCreate(s16 state);
void ItemModelsDestroy(void);
s32 ItemSelectedGet(void);
void ItemNamesCopy(s8 *textBuffer);
s32 ItemEffectCheckComplete(void);
void ItemEffectStateSet(s32 effectState);
void ItemEffectAnimateDisappear(OMOBJ *object, ItemObjectWork *itemWork);
void ItemEffectAnimateCollect(OMOBJ *object, ItemObjectWork *itemWork);
void ItemEffectAnimateAppear(OMOBJ *object, ItemObjectWork *itemWork);
void ItemEffectUpdate(OMOBJ *object);
void ItemEffectStart(s32 playerIndex);
u32 ItemModelForStateGet(s32 playerIndex);
s32 ItemSelectForPlayer(s32 playerIndex, s8 *textBuffer);
s32 ItemCheckRevealComplete(void);
void ItemObjectStateSet(s32 itemState);
void ItemAnimateCollect(OMOBJ *object, ItemEffectWork *effectWork);
void ItemAnimateReveal(OMOBJ *object, ItemEffectWork *effectWork);
void ItemPlayerFaceItem(s32 playerIndex);
void ItemObjectCreate(s32 state);
void ItemObjectUpdate(OMOBJ *object);
void ItemReturnPlayerToSpace(s32 playerIndex);
void ItemResultWindowCreate(void);
void ItemGrantToPlayer(s32 playerIndex);
void ItemEventRunProcess(void);
void ItemEventCleanup(void);
s32 ItemCheckPlayerChoice(s32 playerIndex);
void ItemEventCreateModels(void);
void ItemEventPositionModels(void);

// data
s16 itemResultWindowId = -1;
s16 itemCollectMotionId = -1;
s16 itemAppearMotionId = -1;
s32 itemSoundHandle = -1;
s16 itemParticleId = -1;
s8 itemModelIndices[] = { 1, 1, 3, 3, 2, 2, 2, 1, 2, 3, 0, 4, 5, 0 };

char *itemModelHookNames[] = { "item", "item1", "item2" };

#if VERSION_JP
#define MDL_ID_SHIFT 0
#else
#define MDL_ID_SHIFT 1
#endif

s32 itemModelDataIds[] = {
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

// BSS
s8 itemHookNames[3];
ANIMDATA *itemParticleAnimation;
s8 selectedItemId;
OMOBJ *itemParticleObject;
OMOBJ *itemObject;
HUPROCESS *itemEventProcess;

void ItemEventPrepare(void)
{
    ItemModelsDestroy();
}

void ItemEventNoop(void) { }

void ItemEventStart(void)
{
    BoardRollDispSet(0);
    itemEventProcess = HuPrcChildCreate(ItemEventRunProcess, 0x2003, 0x2000, 0, boardMainProc);
    HuPrcDestructorSet2(itemEventProcess, ItemEventCleanup);

    while (itemEventProcess != 0) {
        HuPrcVSleep();
    }

    BoardRollDispSet(1);
}

void ItemEventCleanup(void)
{
    ItemParticleEffectDestroy();

    if (itemParticleObject) {
        itemParticleObject;
    }

    if (itemObject) {
        omObjGetWork(itemObject, ItemObjectWork)->isActive = 1;
    }

    if (itemSoundHandle != -1) {
        HuAudFXStop(itemSoundHandle);
        itemSoundHandle = -1;
    }

    if (itemAppearMotionId != -1) {
        BoardModelMotionKill(boardModelIds[11], itemAppearMotionId);
        itemAppearMotionId = -1;
    }

    if (itemCollectMotionId != -1) {
        BoardModelMotionKill(boardModelIds[11], itemCollectMotionId);
        itemCollectMotionId = -1;
    }

    if (itemResultWindowId != -1) {
        HuWinKill(itemResultWindowId);
        itemResultWindowId = -1;
    }

    itemEventProcess = 0;
}

void ItemEventRunProcess(void)
{
    s32 currentPlayerIndex;

    currentPlayerIndex = GWSystem.player_curr;
    itemSoundHandle = -1;

    if (ItemCheckPlayerChoice(currentPlayerIndex) != 0) {
        ItemGrantToPlayer(currentPlayerIndex);
        ItemReturnPlayerToSpace(currentPlayerIndex);
    }

    HuPrcEnd();
}

s32 ItemCheckPlayerChoice(s32 playerIndex)
{
    Vec spacePosition;
    Vec playerPosition;
    Vec positionDelta;
    Vec cameraRotation;
    Vec cameraRotation2;
    s16 spaceCur;
    s16 linkedSpaceIndex;

    if (BoardPlayerItemCount(playerIndex) == 3) {
        BoardWinCreate(2, MAKE_MESSID(22, 30), 2);
        BoardWinWait();
        BoardWinKill();
        return 0;
    }

    BoardWinCreateChoice(2, MAKE_MESSID(22, 26), 2, 0);

    if (GWPlayer[playerIndex].com != 0) {
        BoardComKeySetLeft();
    }

    BoardWinWait();

    if (BoardWinChoiceGet() != 0) {
        BoardWinCreate(2, MAKE_MESSID(22, 27), 2);
        BoardWinWait();
        BoardWinKill();
        return 0;
    }

    BoardWinKill();

    cameraRotation.x = 0.0f;
    cameraRotation.y = 200.0f;
    cameraRotation.z = 0.0f;
    cameraRotation2.x = -10.0f;
    cameraRotation2.y = 0.0f;
    cameraRotation2.z = 0.0f;

    BoardCameraMotionStartEx(boardModelIds[8], &cameraRotation2, &cameraRotation, 1500.0f, -1.0f, 21);
    spaceCur = GWPlayer[playerIndex].space_curr;
    linkedSpaceIndex = BoardSpaceLinkFlagSearch(0, spaceCur, 0x02000000);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardSpacePosGet(0, linkedSpaceIndex, &spacePosition);
    VECSubtract(&spacePosition, &playerPosition, &positionDelta);
    VECNormalize(&positionDelta, &positionDelta);
    BoardPlayerRotYSet(playerIndex, atan2d(-positionDelta.x, -positionDelta.z));
    BoardPlayerPosLerpStart(playerIndex, &playerPosition, &spacePosition, 0x14);

    while (GWPlayer[playerIndex].moving != 0) {
        HuPrcVSleep();
    }

    BoardPlayerIdleSet(playerIndex);
    HuPrcSleep(0x3C);
    ItemEffectStart(playerIndex);
    ItemEventCreateModels();
    ItemEventPositionModels();
    ItemPlayerFaceItem(playerIndex);
    return 1;
}

void ItemEventCreateModels(void)
{
    itemCollectMotionId = BoardModelMotionCreate(boardModelIds[11], DATA_MAKE_NUM(DATADIR_W03, 0x23));
    itemAppearMotionId = BoardModelMotionCreate(boardModelIds[11], DATA_MAKE_NUM(DATADIR_W03, 0x24));
}

void ItemEventPositionModels(void)
{
    Vec position;
    Vec modelPosition;
    Vec position2;
    Vec positionDelta;
    f32 floatValue;
    f32 headingAngle;
    f32 floatValue2;

    BoardModelPosGet(boardModelIds[11], &modelPosition);
    position2.x = -3179.8f + 200.0 * sin(0.0);
    position2.z = -907.7f + 200.0 * cos(0.0);
    position2.y = modelPosition.y;
    VECSubtract(&position2, &modelPosition, &positionDelta);
    headingAngle = atan2d(positionDelta.z, positionDelta.x);
    floatValue = BoardModelRotYGet(boardModelIds[11]);
    BoardModelMotionShiftSet(boardModelIds[11], itemCollectMotionId, 0.0f, 10.0, HU3D_MOTATTR_LOOP);

    while (BoardDAngleCalcRange(&floatValue, headingAngle, 10.0) == 0) {
        BoardModelRotYSet(boardModelIds[11], floatValue);
        HuPrcVSleep();
    }

    BoardModelRotYSet(boardModelIds[11], headingAngle);
    BoardModelMotionSpeedSet(boardModelIds[11], 2.0f);

    positionDelta.x /= 20.0f;
    positionDelta.z /= 20.0f;
    positionDelta.y = 0.0f;
    position = modelPosition;
    floatValue2 = 0.0f;

    while (floatValue2 < 20.0f) {
        VECAdd(&position, &positionDelta, &position);
        BoardModelPosSetV(boardModelIds[11], &position);
        HuPrcVSleep();
        floatValue2 += 1.0f;
    }

    BoardAudSeqPause(0, 1, 0x3E8);
    HuAudFXPlay(0x450);
    HuAudFXPlay(0x362);
    BoardCameraQuakeSet(3, 100.0f);
    ItemEffectStateSet(1);
    BoardModelMotionShiftSet(boardModelIds[11], itemAppearMotionId, 0.0f, 10.0, HU3D_MOTATTR_LOOP | HU3D_MOTATTR_REV);
    VECSubtract(&modelPosition, &position, &positionDelta);

    positionDelta.x /= 60.0f;
    positionDelta.z /= 60.0f;
    positionDelta.y = 0.0f;
    floatValue2 = 0.0f;

    while (floatValue2 < 60.0f) {
        VECAdd(&position, &positionDelta, &position);
        BoardModelPosSetV(boardModelIds[11], &position);
        HuPrcVSleep();
        floatValue2 += 1.0f;
    }

    headingAngle = 0.0f;
    floatValue = BoardModelRotYGet(boardModelIds[11]);
    BoardModelMotionShiftSet(boardModelIds[11], itemCollectMotionId, 0.0f, 10.0, HU3D_MOTATTR_LOOP);

    while (BoardDAngleCalcRange(&floatValue, headingAngle, 10.0) == 0) {
        BoardModelRotYSet(boardModelIds[11], floatValue);
        HuPrcVSleep();
    }

    BoardModelMotionShiftSet(boardModelIds[11], 1, 0.0f, 10.0, HU3D_MOTATTR_LOOP);
}

void ItemGrantToPlayer(s32 playerIndex)
{
    s16 integerValue;
    s32 integerValue2;
    s32 integerValue3;

    BoardMusStart(1, 0x17, 0x7F, 0);
    integerValue = MGSeqStartCreate();

    while (MGSeqStatGet(integerValue) != 0) {
        HuPrcVSleep();
    }

    ItemObjectCreate(playerIndex);
    ItemResultWindowCreate();
    ItemEffectStateSet(1);

    while (ItemCheckRevealComplete() != 0) {
        HuPrcVSleep();
    }

    integerValue2 = ItemSelectedGet() + 0x80000;
    BoardWinCreate(2, MAKE_MESSID(22, 28), 2);
    BoardWinInsertMesSet(integerValue2, 0);
    BoardWinWait();
    ItemEffectStateSet(4);

    while (ItemEffectCheckComplete() != 0) {
        HuPrcVSleep();
    }

    omVibrate(playerIndex, 0xC, 6, 6);
    BoardPlayerItemAdd(playerIndex, ItemSelectedGet());
    ItemEffectStateSet(3);
    integerValue3 = HuAudSStreamPlay(2);

    while (HuAudSStreamStatGet(integerValue3) != 0) {
        HuPrcVSleep();
    }

    BoardAudSeqPause(0, 0, 1000);

    while (itemParticleObject) {
        HuPrcVSleep();
    }

    HuPrcSleep(0x1E);
    BoardWinCreate(2, MAKE_MESSID(22, 29), 2);
    BoardWinWait();
    BoardWinKill();
}

void ItemReturnPlayerToSpace(s32 playerIndex)
{
    Vec playerPosition;
    Vec spacePosition;
    s16 spaceCur;

    spaceCur = GWPlayer[playerIndex].space_curr;
    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardSpacePosGet(0, spaceCur, &spacePosition);
    BoardCameraTargetPlayerSet(playerIndex);
    BoardCameraViewSet(1);
    BoardPlayerPosLerpStart(playerIndex, &playerPosition, &spacePosition, 20);
    while (GWPlayer[playerIndex].moving != 0) {
        HuPrcVSleep();
    }
    BoardModelRotYSet(boardModelIds[11], 0.0f);
    BoardCameraMotionWait();
}

void ItemResultWindowCreate(void)
{
    f32 floatValue[2];
    f32 val1, val2;

    HuWinMesMaxSizeGet(1, floatValue, MAKE_MESSID(0x16, 0x1F));
    val1 = -10000.0f;
    val2 = 392.0f;
    itemResultWindowId = HuWinCreate(val1, val2, floatValue[0], floatValue[1], 0);
    HuWinBGTPLvlSet(itemResultWindowId, 0.0f);
    HuWinMesSpeedSet(itemResultWindowId, 0);
    HuWinMesSet(itemResultWindowId, MAKE_MESSID(0x16, 0x1F));
}

void ItemObjectCreate(s32 state)
{
    ItemEffectWork *itemWork;
    OMOBJ *object;

    object = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, (void *)ItemObjectUpdate);
    itemParticleObject = object;
    itemWork = (ItemEffectWork *)&object->work[0];
    itemWork->isActive = 0;
    itemWork->isTimed = 0;
    itemWork->isVisible = state;
    itemWork->itemType = 0;
    itemWork->itemId = 5U;
    itemWork->itemTimer = 0x3C;
    itemWork->timerHandle = MGSeqTimerCreateXY(itemWork->itemId, 288, 64);
    ItemNamesCopy(itemHookNames);
    selectedItemId = ItemSelectForPlayer(state, itemHookNames);
}

void ItemPlayerFaceItem(s32 playerIndex)
{
    Vec playerPosition;
    Vec modelPosition;
    f32 playerYRot;

    BoardModelPosGet(boardModelIds[9], &modelPosition);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    modelPosition.y = playerPosition.y;
    BoardPlayerPosLerpStart(playerIndex, &playerPosition, &modelPosition, 20);

    while (GWPlayer[playerIndex].moving != 0) {
        HuPrcVSleep();
    }

    playerYRot = BoardPlayerRotYGet(playerIndex) + 180.0f;
    BoardPlayerMotBlendSet(playerIndex, playerYRot, 15);

    while (BoardPlayerMotBlendCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }
}

void ItemObjectUpdate(OMOBJ *object)
{
    s32 integerValue;
    ItemEffectWork *itemWork;

    itemWork = (ItemEffectWork *)&itemParticleObject->work[0];
    if ((itemWork->isActive != 0) || (BoardIsKill() != 0)) {
        if (itemWork->timerHandle != -1) {
            MGSeqParamSet(itemWork->timerHandle, 2, -1U);
            itemWork->timerHandle = -1;
        }
        itemParticleObject = NULL;

        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }

    if (itemWork->isTimed == 0) {
        if (itemWork->itemTimer != 0) {
            itemWork->itemTimer--;
        }
        else {
            if (itemWork->itemId != 0) {
                itemWork->itemId--;
            }
            else {
                ItemEffectStateSet(2);
            }
            MGSeqParamSet(itemWork->timerHandle, 1, itemWork->itemId);
            itemWork->itemTimer = 0x3CU;
        }
    }

    switch (itemWork->itemType) {
        case 1:
            ItemAnimateReveal(object, value);
            return;
        case 2:
            ItemAnimateCollect(object, value);
            return;
        case 3:
            if (BoardPlayerMotionEndCheck(itemWork->isVisible) != 0) {
                itemWork->isActive = 1;
            }
        case 0:
            break;
    }
}

void ItemAnimateReveal(OMOBJ *object, ItemEffectWork *effectWork)
{
    u32 playerIndex = 0;

    if (GWPlayer[effectWork->isVisible].com != 0) {
        playerIndex = ItemModelForStateGet(effectWork->isVisible);
    }
    else {
        playerIndex = HuPadBtnDown[GWPlayer[effectWork->isVisible].port];
    }
    if (playerIndex == 0x100) {
        ItemEffectStateSet(2);
    }
}

// TODO properly use inlines
void ItemAnimateCollect(OMOBJ *object, ItemEffectWork *effectWork)
{
    s16 boardModelID;
    PlayerState *playerStateCopy;
    s32 bit;
    PlayerState *playerState;
    s16 playerModelId, currentMotionId;

    bit = effectWork->isVisible;
    playerState = &GWPlayer[bit];
    playerStateCopy = playerState;
    playerModelId = boardPlayerMdl[playerStateCopy->player_idx];
    currentMotionId = playerModelId;
    boardModelID = BoardModelIDGet(currentMotionId);

    if (Hu3DMotionShiftIDGet(boardModelID) == -1) {
        if (!(effectWork->isComplete)) {
            if (BoardPlayerMotionTimeGet(effectWork->isVisible) >= 27.0f) {
                effectWork->isComplete = 1;
                BoardAudSeqFadeOut(1, 0x64);
                ItemEffectStateSet(2);
                return;
            }
        }
        if (BoardPlayerMotionEndCheck(effectWork->isVisible)) {
            ItemEffectStateSet(0);
        }
    }
}

void ItemEffectStateSet(s32 effectState)
{
    ItemEffectWork *effectWork;

    effectWork = (ItemEffectWork *)&itemParticleObject->work[0];
    effectWork->itemType = effectState;

    switch (effectState) {
        case 0:
            BoardPlayerIdleSet(effectWork->isVisible);
            return;
        case 2:
            effectWork->isTimed = 1;
            if (itemResultWindowId != -1) {
                HuWinKill(itemResultWindowId);
                itemResultWindowId = -1;
            }
            if (effectWork->timerHandle != -1) {
                MGSeqParamSet(effectWork->timerHandle, 2, -1U);
                effectWork->timerHandle = -1;
            }
            BoardPlayerMotionShiftSet(effectWork->isVisible, 0xB, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            return;
        case 3:
            BoardPlayerMotionShiftSet(effectWork->isVisible, 7, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            break;
    }
}

s32 ItemCheckRevealComplete(void)
{
    ItemEffectWork *effectWork = (ItemEffectWork *)&itemParticleObject->work[0];
    return effectWork->itemType;
}

s32 ItemSelectForPlayer(s32 playerIndex, s8 *textBuffer)
{
    s32 integerValue;
    s32 integerValue2;
    s32 integerValue3;
    s32 stateValue;

    integerValue2 = itemModelIndices[textBuffer[0]];
    integerValue3 = itemModelIndices[textBuffer[1]];
    integerValue = itemModelIndices[textBuffer[2]];

    if (integerValue2 > integerValue3) {
        if (integerValue2 > integerValue) {
            stateValue = 0;
        }
        else if (integerValue > integerValue3) {
            stateValue = 2;
        }
        else {
            stateValue = 1;
        }
    }
    else if (integerValue3 > integerValue) {
        stateValue = 1;
    }
    else if (integerValue > integerValue2) {
        stateValue = 2;
    }
    else {
        stateValue = 0;
    }

    return stateValue;
}

u32 ItemModelForStateGet(s32 playerIndex)
{
    s32 stateValue;
    s32 integerValue;
    s16 animationFrame;
    s16 stateValue2;
    s8 integerValue2;

    stateValue = 0;

    switch (GWPlayer[playerIndex].diff) {
        case 3:
            stateValue2 = 0;
            break;
        case 2:
            stateValue2 = 5;
            break;
        case 1:
            stateValue2 = 15;
            break;
        case 0:
            stateValue2 = 40;
            break;
    }
    if ((stateValue2 != 0) && (BoardRandMod(100) < stateValue2)) {
        return 0x100U;
    }

    animationFrame = BoardModelMotionTimeGet(boardModelIds[9]);

    switch (selectedItemId) {
        case 0:
            if ((animationFrame < 33) || (animationFrame > 35)) {
                break;
            }
            return 0x100U;
        case 1:
            if (((animationFrame >= 40) && (animationFrame <= 42)) || ((animationFrame >= 20) && (animationFrame <= 22))) {
                return 0x100U;
            }
            break;
        case 2:
            if (animationFrame >= 50) {
                return 0x100U;
            }
            break;
    }
    return 0;
}

void ItemEffectStart(s32 playerIndex)
{
    Mtx modelMatrix;
    s32 integerValue[3];
    s32 j;
    s32 i;
    s32 integerValue2;
    s32 integerValue3;
    ItemObjectWork *itemWork;
    OMOBJ *object;

    object = omAddObjEx(boardObjMan, 0x101, 0U, 0U, -1, (void *)ItemEffectUpdate);
    itemObject = (void *)object;
    itemWork = (ItemObjectWork *)&object->work[0];

    itemWork->isActive = 0;
    itemWork->itemType = 0;
    itemWork->isVisible = playerIndex;

    for (i = 0; i < ARRAY_COUNT(integerValue);) {
        integerValue[i] = BoardRandMod(14);
        if ((integerValue[i] != 10) && (integerValue[i] != 13)) {
            for (j = 0; j < i; j++) {
                if (integerValue[i] == integerValue[j]) {
                    integerValue[i] = -1;
                }
            }
            if (integerValue[i] == -1) {
                continue;
            }
        }
        else {
            continue;
        }
        i++;
    }

    ItemModelsDestroy();

    for (i = 0; i < ARRAY_COUNT(integerValue); i++) {
        integerValue3 = integerValue[i];
        itemWork->playerIndex[i] = integerValue3;
        itemWork->modelIds[i] = BoardModelCreate(itemModelDataIds[integerValue3], NULL, 0);
        BoardModelHookSet(boardModelIds[9], itemModelHookNames[i], itemWork->modelIds[i]);
        BoardModelMotionSpeedSet(itemWork->modelIds[i], 0.0f);

        if (integerValue3 == 11) {
            BoardModelMotionStart(itemWork->modelIds[i], 0, 0x40000001);
        }

        if (integerValue3 == 5) {
            MTXTrans(modelMatrix, 0.0f, -50.0f, 0.0f);
            BoardModelMtxSet(itemWork->modelIds[i], &modelMatrix);
        }

        if ((integerValue3 == 6) || (integerValue3 == 7)) {
            MTXTrans(modelMatrix, 0.0f, 0.0f, 8.0f);
            BoardModelMtxSet(itemWork->modelIds[i], &modelMatrix);
        }
    }
}

void ItemEffectUpdate(OMOBJ *object)
{
    s32 i;
    ItemObjectWork *animationTime;

    animationTime = (ItemObjectWork *)&object->work[0];

    if ((animationTime->isActive != 0) || (BoardIsKill() != 0)) {
        BoardModelHookReset(boardModelIds[9]);
        for (i = 0; i < 3; i++) {
            BoardModelKill(animationTime->modelIds[i]);
        }
        ItemModelsDestroy();
        omDelObjEx(HuPrcCurrentGet(), object);
        itemObject = NULL;
        return;
    }

    switch (animationTime->itemType) {
        case 1:
            if (!(BoardModelMotionTimeGet(boardModelIds[9]) < 20.0f)) {
                BoardModelMotionStartEndSet(boardModelIds[9], 0x14, 0x3A);
                ItemObjectStateSet(0);
                return;
            }
        case 0:
            break;
        case 2:
            ItemEffectAnimateAppear(object, animationTime);
            return;
        case 3:
            ItemEffectAnimateCollect(object, animationTime);
            return;
        case 4:
            ItemEffectAnimateDisappear(object, animationTime);
            break;
    }
}

void ItemEffectAnimateAppear(OMOBJ *object, ItemObjectWork *itemWork)
{
    Vec modelPosition;
    Vec playerPosition;
    Vec position;
    f32 animationTime;
    s8 integerValue;
    s32 i;

    itemWork->itemTimer = -1;

    for (i = 0; i < 3; i++) {
        Hu3DModelObjPosGet(BoardModelIDGet(boardModelIds[9]), itemModelHookNames[i], &modelPosition);
        BoardPlayerPosGet(itemWork->isVisible, &playerPosition);
        playerPosition.y += 66.0f;
        if (BoardVecMinDistCheck(&modelPosition, &playerPosition, 100.0f) != 0) {
            position = modelPosition;
            itemWork->itemTimer = i;
            break;
        }
    }

    HuAudFXPlay(0x30C);
    BoardModelHookObjReset(boardModelIds[9], itemModelHookNames[itemWork->itemTimer]);
    animationTime = BoardModelMotionTimeGet(boardModelIds[9]);
    BoardModelMotionShiftSet(boardModelIds[9], 0, 15.0f, 10.0f, HU3D_MOTATTR_REV);
    BoardModelMotionSpeedSet(boardModelIds[9], 0.5f);
    BoardModelMotionTimeSet(boardModelIds[9], animationTime);
    BoardModelPosSetV(itemWork->modelIds[itemWork->itemTimer], &position);
    ItemObjectStateSet(3);
    ItemParticleEffectCreate(itemWork->modelIds[itemWork->itemTimer]);
}

void ItemEffectAnimateCollect(OMOBJ *object, ItemObjectWork *itemWork)
{
    Vec modelPosition;

    if (itemWork->itemId-- != 0) {
        BoardModelPosGet(itemWork->modelIds[itemWork->itemTimer], &modelPosition);
        modelPosition.x = modelPosition.x + object->trans.x;
        modelPosition.y += object->trans.y;
        modelPosition.z = modelPosition.z + object->trans.z;
        BoardModelPosSetV(itemWork->modelIds[itemWork->itemTimer], &modelPosition);
        return;
    }

    ItemObjectStateSet(0);
}

void ItemEffectAnimateDisappear(OMOBJ *object, ItemObjectWork *itemWork)
{
    Vec position;
    Vec modelPosition;
    f32 floatValue;
    s16 integerValue;

    integerValue = itemWork->modelIds[itemWork->itemTimer];

    if (itemWork->itemId == 0) {
        HuAudFXPlay(781);
    }

    if (itemWork->itemId < 90) {
        OSu8tof32((u8 *)&itemWork->itemId, &floatValue);
        BoardModelPosGet(integerValue, &modelPosition);
        modelPosition.y += object->trans.y;
        BoardModelPosSetV(integerValue, &modelPosition);
        BoardModelRotYSet(integerValue, 8.0f + BoardModelRotYGet(integerValue));
        BoardModelScaleGet(integerValue, &position);
        position.x = position.y = position.z = cosd(floatValue);
        BoardModelScaleSetV(integerValue, &position);
        itemWork->itemId += 4;
        return;
    }

    if (itemSoundHandle != -1) {
        HuAudFXStop(itemSoundHandle);
        itemSoundHandle = -1;
    }

    ItemObjectStateSet(0);
}

void ItemObjectStateSet(s32 itemState)
{
    Vec playerPosition;
    Vec modelPosition;
    f32 floatValue;
    ItemObjectWork *itemWork;
    f32 targetHeight;

    itemWork = (ItemObjectWork *)&itemObject->work[0];
    itemWork->itemType = itemState;

    switch (itemState) {
        case 1:
            floatValue = 0.31666666f;
            BoardModelMotionStart(boardModelIds[9], 0, 0x40000001);
            BoardModelMotionSpeedSet(boardModelIds[9], floatValue);
            BoardModelMotionTimeSet(boardModelIds[9], 8.0f);
            return;
        case 2:
            itemWork->isTimed = 1;
            return;
        case 3:
            itemSoundHandle = HuAudFXPlay(0x35F);
            BoardModelPosGet(itemWork->modelIds[itemWork->itemTimer], &modelPosition);
            BoardPlayerPosGet(itemWork->isVisible, &playerPosition);
            targetHeight = 50.0f + modelPosition.y;
            itemObject->trans.x = (playerPosition.x - modelPosition.x) / 10.0f;
            itemObject->trans.z = (playerPosition.z - modelPosition.z) / 10.0f;
            itemObject->trans.y = (targetHeight - modelPosition.y) / 10.0f;
            itemWork->itemId = 10;
            return;
        case 4:
            BoardPlayerPosGet(itemWork->isVisible, &playerPosition);
            playerPosition.y += 30.000002f;
            BoardModelPosGet(itemWork->modelIds[itemWork->itemTimer], &modelPosition);
            itemObject->trans.y = (playerPosition.y - modelPosition.y) / 22.0f;
            itemWork->itemId = 0;
    }
}

s32 ItemEffectCheckComplete(void)
{
    ItemObjectWork *work = (ItemObjectWork *)&itemObject->work[0];
    return work->itemType;
}

void ItemNamesCopy(s8 *textBuffer)
{
    ItemObjectWork *bits = omObjGetWork(itemObject, ItemObjectWork);
    s32 i;

    for (i = 0; i < ARRAY_COUNT(bits->playerIndex); i++) {
        textBuffer[i] = bits->playerIndex[i];
    }
}

s32 ItemSelectedGet(void)
{
    ItemObjectWork *bits = omObjGetWork(itemObject, ItemObjectWork);
    return bits->playerIndex[bits->itemTimer];
}

void ItemModelsDestroy(void)
{
    BoardModelMotionStart(boardModelIds[9], 0, 0);
    BoardModelMotionSpeedSet(boardModelIds[9], 0.0f);
    BoardModelMotionTimeSet(boardModelIds[9], 0.0f);
    BoardModelAttrReset(boardModelIds[9], 0x40000004);
}

void ItemParticleEffectCreate(s16 state)
{
    HU3DPARTICLE *particle;

    itemParticleAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_EFFECT, 0x00));
    HuSprAnimLock(itemParticleAnimation);
    itemParticleId = Hu3DParticleCreate(itemParticleAnimation, 100);
    Hu3DParticleHookSet(itemParticleId, ItemParticleUpdate);
    Hu3DParticleColSet(itemParticleId, 0xFF, 0xFF, 0);
    Hu3DModelPosSet(itemParticleId, 0.0f, 0.0f, 0.0f);
    Hu3DParticleBlendModeSet(itemParticleId, 1);
    Hu3DModelLayerSet(itemParticleId, 2);
    particle = Hu3DData[itemParticleId].hookData;
    particle->emitCnt = state;
}

void ItemParticleEffectDestroy(void)
{
    if (itemParticleId != -1) {
        Hu3DModelKill(itemParticleId);
        itemParticleId = -1;
    }
    if (itemParticleAnimation) {
        HuSprAnimKill(itemParticleAnimation);
        itemParticleAnimation = NULL;
    }
}

void ItemParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    Vec modelPosition;
    f32 floatValue;
    f32 particleRadius;
    f32 floatValue2;
    HU3DPARTICLEDATA *particleData;
    s32 i, j;

    if (particle->count == 0) {
        particleData = particle->data;
        for (i = 0; i < particle->maxCnt; i++, particleData++) {
            particleData->color.a = 0;
            particleData->scale = 0.0f;
        }
        particle->dataCnt = 0;
    }

    BoardModelScaleGet(particle->emitCnt, &modelPosition);
    floatValue2 = modelPosition.x;
    BoardModelPosGet(particle->emitCnt, &modelPosition);

    for (i = 0; i < 22; i++) {
        particleData = particle->data;
        for (j = 0; j < particle->maxCnt; j++, particleData++) {
            if (0.0f == particleData->scale) {
                break;
            }
        }
        if (j != particle->maxCnt) {
            floatValue = 360.0f * ((1.0f / 255.0f) * frand8());
            particleRadius = floatValue2 * (20.0f + (60.0f * ((1.0f / 255.0f) * frand8())));
            particleData->pos.x = (modelPosition.x + (particleRadius * sind(floatValue)));
            particleData->pos.z = (modelPosition.z + (particleRadius * cosd(floatValue)));
            particleData->pos.y = (modelPosition.y + (floatValue2 * (-30.0f + (40.0f * ((1.0f / 255.0f) * frand8())))));
            particleData->vel.x = (0.5f + (3.0f * ((1.0f / 255.0f) * frand8())));
            particleData->vel.y = (0.3f + (2.0f * ((1.0f / 255.0f) * frand8())));
            particleData->color.a = 180;
            particleData->scale = (15.0f * floatValue2);
        }
    }

    particleData = particle->data;

    for (i = 0; i < particle->maxCnt; i++, particleData++) {
        if (0.0f != particleData->scale) {
            particleData->pos.y = (particleData->pos.y - particleData->vel.x);
            particleData->scale = (particleData->scale - particleData->vel.y);
            if (particleData->scale <= 0.0f) {
                particleData->scale = 0.0f;
            }
        }
    }
}
