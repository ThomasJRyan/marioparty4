#include "REL/w06Dll.h"
#include "game/audio.h"
#include "game/board/audio.h"
#include "game/board/com.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/ui.h"
#include "game/board/window.h"
#include "game/data.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"


#include "dolphin.h"
#include "ext_math.h"

typedef struct {
    struct {
        u8 isActive : 1;
        u8 eventPlayerIndex : 2;
        u8 promptPlayerIndex : 2;
        u8 isCollected : 1;
        u8 isVisible : 1;
    };
    u8 promptDelayFrames;
    s8 promptState;
    s8 reservedByte;
    char padding[2];
    s16 itemModelId;
    s16 animationFrames;
    char padding2[2];
    s32 soundEffectId;
} W06ItemBoardWork;

typedef struct {
    struct {
        u8 isActive : 1;
        u8 targetPlayerIndex : 2;
        u8 isFinished : 1;
    };
    s8 inputState;
    s8 inputProgress;
    s8 expectedState;
} W06ItemPromptWork;

typedef struct {
    struct {
        u8 isActive : 1;
        u8 animationState : 3;
        u8 itemType : 1;
        u8 isVisible : 1;
    };
    s8 playerIndexes[2];
    s8 currentPlayer;
    u8 selectionCooldown;
    char padding[1];
    s16 movementFrames;
    s16 motionId;
    s16 animationMotionId;
} W06ItemAnimationWork;

typedef struct {
    struct {
        u8 isActive : 1;
    };
    u8 timerValue;
    u8 tickDelay;
    char padding[1];
    s16 timerSequenceId;
} W06ItemTimerWork;

static void W06ItemEventCleanup(void);
static void W06ItemEventProcess(void);
static void W06ItemMessageWindowCreate(void);
static s32 W06ItemPlayerChoiceEvaluate(s32 playerIndex);
static void W06ItemPlayerApproach(s32 playerIndex);
static void W06ItemPromptSequence(s32 playerIndex, s32 shouldApproach);
static void W06ItemPlayerAnimationSet(s32 playerSlot);
static void W06ItemPlayerInputUpdate(s32 playerSlot);
static void W06ItemPlayerMotionUpdate(s32 playerSlot);
static void W06ItemPlayersPromptStart(s32 itemId);
static void W06ItemPromptStateSet(s32 playerIndex, s32 timeoutFrames);
static void W06ItemPromptTimeoutSet(s32 playerIndex);
static s32 W06ItemPromptTimeoutGet(s32 playerIndex);
static s16 W06ItemPlayerOrderGet(s32 playerIndex);
static void W06ItemPlayerPromptCreate(s32 itemId, s32 playerIndex, s32 itemType);
static void W06ItemPlayerPromptUpdate(OMOBJ *promptObject);
static s32 W06ItemPromptExpectedStateGet(s32 playerIndex);
static void W06ItemPromptObjectCreate(s32 targetPlayerIndex);
static void W06ItemPromptExpectedStateUpdate(void);
static void W06ItemPromptInputStateSet(s32 inputState);
static s32 W06ItemPromptInputStateGet(void);
static u32 W06ItemPromptInputRead(W06ItemPromptWork *promptWork);
static void W06ItemPromptObjectUpdate(OMOBJ *promptObject);
static void W06ItemAnimationObjectCreate(void);
static void W06ItemAnimationStateSet(s32 animationState);
static s32 W06ItemAnimationStateGet(void);
static void W06ItemPlayerOrderSet(s32 playerOrder);
static void W06ItemSelectionCursorMove(s8 playerIndex);
static void W06ItemAnimationObjectUpdate(OMOBJ *animationObject);
static void W06ItemTimerObjectCreate(void);
static void W06ItemTimerObjectStop(void);
static void W06ItemTimerObjectUpdate(OMOBJ *timerObject);
static void W06ItemParticleCreate(s16 emitCount);
static void W06ItemParticleDestroy(void);
static void W06ItemParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

static s16 itemPlayerModels[3];
static s16 itemPlayerProps[3];
static s16 itemSelectedIndex;
static s8 itemSelectedType;
static s8 itemPromptStates[3];
static OMOBJ *itemTimerObject;
static OMOBJ *itemAnimationObject;
static OMOBJ *itemPromptObject;
static ANIMDATA *itemParticleAnimation;
static s8 itemTimerExpired;
static HUPROCESS *itemEventProcess;

static s16 itemPromptWindow = -1;
static s8 itemPlayerChoices[3] = { -1, -1, -1 };
static s8 itemPlayerStates[3] = { 0, 0, 0 };
static s16 itemParticleModel = -1;
static OMOBJ *itemPlayerPromptObjects[3] = { NULL, NULL, NULL };

void W06ItemBoardInitialize(void)
{
    Vec boardPosition;
    Vec playerPosition;
    Vec targetPosition;
    s32 i;

    BoardModelPosGet(w06MapObjectModelIds[1], &boardPosition);
    targetPosition.x = boardPosition.x;
    targetPosition.y = boardPosition.y + 60.0f;
    targetPosition.z = boardPosition.z + -50.0f;
    BoardModelPosSetV(w06MapObjectModelIds[4], &targetPosition);
    for (i = 0; i < 3; i++) {
        itemPlayerModels[i] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W06, 9), NULL, 0);
        playerPosition.x = boardPosition.x + 150.0f * (i - 1.0f);
        playerPosition.y = boardPosition.y;
        playerPosition.z = boardPosition.z + 100.0f;
        BoardModelPosSetV(itemPlayerModels[i], &playerPosition);
        BoardModelMotionSpeedSet(itemPlayerModels[i], 0.0f);
        itemPlayerStates[i] = 0;
        itemPlayerProps[i] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W06, 10), NULL, 0);
        BoardModelVisibilitySet(itemPlayerProps[i], 0);
        BoardModelHookSet(itemPlayerModels[i], "batu", itemPlayerProps[i]);
    }
}

void W06ItemBoardCleanup(void)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        BoardModelHookReset(itemPlayerModels[i]);
        if (itemPlayerModels[i] != -1) {
            BoardModelKill(itemPlayerModels[i]);
            itemPlayerModels[i] = -1;
        }
        if (itemPlayerProps[i] != -1) {
            BoardModelKill(itemPlayerProps[i]);
            itemPlayerProps[i] = -1;
        }
    }
}

void W06ItemSpaceEventStart(void)
{
    BoardRollDispSet(0);
    itemEventProcess = HuPrcChildCreate(W06ItemEventProcess, 0x2003, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(itemEventProcess, W06ItemEventCleanup);
    while (itemEventProcess) {
        HuPrcVSleep();
    }
    BoardRollDispSet(1);
}

static void W06ItemEventCleanup(void)
{
    W06ItemParticleDestroy();
    if (itemPromptWindow != -1) {
        HuWinKill(itemPromptWindow);
        itemPromptWindow = -1;
    }
    itemEventProcess = NULL;
}

static void W06ItemEventProcess(void)
{
    s32 choiceAccepted;
    s32 choiceAccepted2;

    choiceAccepted2 = GWSystem.player_curr;
    choiceAccepted = W06ItemPlayerChoiceEvaluate(choiceAccepted2);
    if (choiceAccepted != 0) {
        W06ItemPlayerApproach(choiceAccepted2);
    }
    W06ItemPromptSequence(choiceAccepted2, choiceAccepted);
    HuPrcEnd();
}

static void W06ItemMessageWindowCreate(void)
{
    float windowSize[2];
    float windowWidth;
    float windowHeight;

    HuWinMesMaxSizeGet(1, windowSize, MAKE_MESSID(48, 25));
    windowWidth = -10000.0f;
    windowHeight = 352.0f;
    itemPromptWindow = HuWinCreate(windowWidth, windowHeight, windowSize[0], windowSize[1], 0);
    HuWinBGTPLvlSet(itemPromptWindow, 0.0f);
    HuWinMesSpeedSet(itemPromptWindow, 0);
    HuWinMesSet(itemPromptWindow, MAKE_MESSID(48, 25));
}

static s32 W06ItemPlayerChoiceEvaluate(s32 playerIndex)
{
    Vec playerPosition;
    Vec eventPosition;
    Vec movementVector;
    Vec cameraOffset;
    Vec targetPosition;
    s16 currentSpaceIndex;

    if (BoardPlayerItemCount(playerIndex) == 3) {
        BoardWinCreate(2, MAKE_MESSID(48, 17), 6);
        BoardWinWait();
        BoardWinKill();
        return 0;
    }
    HuAudFXPlay(0x362);
    W06ItemPlayerInputUpdate(0);
    W06ItemPlayerInputUpdate(2);
    W06ItemPlayerInputUpdate(1);
    W06ItemPlayerAnimationSet(0);
    W06ItemPlayerAnimationSet(1);
    W06ItemPlayerAnimationSet(2);
    itemPromptStates[0] = itemPromptStates[1] = itemPromptStates[2] = 0;
    W06ItemPlayersPromptStart(playerIndex);
    BoardWinCreateChoice(2, MAKE_MESSID(48, 18), 6, 0);
    if (GWPlayer[playerIndex].com) {
        BoardComKeySetLeft();
    }
    BoardWinWait();
    if (BoardWinChoiceGet() != 0) {
        BoardWinCreate(2, MAKE_MESSID(48, 19), 6);
        BoardWinWait();
        BoardWinKill();
        W06ItemPromptStateSet(-1, 1);
        while (W06ItemPromptTimeoutGet(0) != 0) {
            HuPrcVSleep();
        }
        W06ItemPlayerMotionUpdate(0);
        W06ItemPlayerMotionUpdate(2);
        W06ItemPlayerMotionUpdate(1);
        return 0;
    }
    BoardWinKill();
    BoardAudSeqPause(0, 1, 1000);
    cameraOffset.x = 0.0f;
    cameraOffset.y = 100.0f;
    cameraOffset.z = 0.0f;
    targetPosition.x = -20.0f;
    targetPosition.y = 0.0f;
    targetPosition.z = 0.0f;
    BoardCameraMotionStartEx(w06MapObjectModelIds[1], &targetPosition, &cameraOffset, 1600.0f, -1.0f, 21);
    currentSpaceIndex = GWPlayer[playerIndex].space_curr;
    itemSelectedIndex = BoardSpaceLinkFlagSearch(0, currentSpaceIndex, 0x02000000);
    BoardPlayerPosGet(playerIndex, &eventPosition);
    BoardSpacePosGet(0, itemSelectedIndex, &playerPosition);
    VECSubtract(&playerPosition, &eventPosition, &movementVector);
    VECNormalize(&movementVector, &movementVector);
    BoardPlayerRotYSet(playerIndex, atan2d(-movementVector.x, -movementVector.z));
    BoardPlayerPosLerpStart(playerIndex, &eventPosition, &playerPosition, 20);
    while (GWPlayer[playerIndex].moving) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(playerIndex);
    HuPrcSleep(60);
    return 1;
}

static void W06ItemPlayerApproach(s32 playerIndex)
{
    s8 responseFrames[] = { 80, 50 };
    s32 inputWasCorrect;
    s16 minigameSequenceId;
    s32 audioStreamId;
    s32 promptRoundIndex;
    s32 playerSlot;

    BoardMusStart(1, 0x26, 0x7F, 0);
    minigameSequenceId = MGSeqStartCreate();
    while (MGSeqStatGet(minigameSequenceId) != 0) {
        HuPrcVSleep();
    }
    W06ItemPromptStateSet(-1, 1);
    while (W06ItemPromptTimeoutGet(0) != 0) {
        HuPrcVSleep();
    }
    W06ItemPlayerMotionUpdate(0);
    W06ItemPlayerMotionUpdate(2);
    W06ItemPlayerMotionUpdate(1);
    W06ItemPlayerAnimationSet(0);
    W06ItemPlayerAnimationSet(1);
    W06ItemPlayerAnimationSet(2);
    itemPromptStates[0] = itemPromptStates[1] = itemPromptStates[2] = 0;
    W06ItemAnimationObjectCreate();
    W06ItemPromptObjectCreate(minigameSequenceId);
    for (promptRoundIndex = 0; promptRoundIndex < 2; promptRoundIndex++) {
        W06ItemTimerObjectCreate();
        W06ItemMessageWindowCreate();
        W06ItemPromptInputStateSet(2);
        W06ItemPromptExpectedStateUpdate();
        while (W06ItemPromptInputStateGet() != 3) {
            HuPrcVSleep();
        }
        W06ItemTimerObjectStop();
        HuWinDispOff(itemPromptWindow);
        W06ItemAnimationStateSet(1);
        W06ItemSelectionCursorMove(itemSelectedType);
        while (W06ItemAnimationStateGet() != 0) {
            HuPrcVSleep();
        }
        BoardWinCreate(2, MAKE_MESSID(48, 20), 6);
        BoardWinWait();
        inputWasCorrect = (BoardRandMod(100) >= responseFrames[promptRoundIndex]) ? 1 : 0;
        HuPrcSleep(60);
        if (inputWasCorrect != 0) {
            break;
        }
        BoardWinCreate(2, MAKE_MESSID(48, 21), 6);
        BoardWinWait();
        itemPromptStates[itemSelectedType] = 1;
        BoardModelVisibilitySet(itemPlayerProps[itemSelectedType], 1);
        HuAudFXPlay(0xB);
        HuPrcSleep(60);
        if (promptRoundIndex == 0) {
            BoardWinCreate(2, MAKE_MESSID(48, 41), 6);
            BoardWinWait();
            BoardWinKill();
        }
        else {
            for (playerSlot = 0; playerSlot < 3; playerSlot++) {
                if (itemPromptStates[playerSlot] == 0) {
                    itemSelectedType = playerSlot;
                    break;
                }
            }
            W06ItemAnimationStateSet(1);
            W06ItemSelectionCursorMove(itemSelectedType);
            while (W06ItemAnimationStateGet() != 0) {
                HuPrcVSleep();
            }
            break;
        }
    }
    W06ItemPromptInputStateSet(4);
    BoardAudSeqFadeOut(1, 1000);
    W06ItemPlayerInputUpdate(itemSelectedType);
    W06ItemParticleCreate(W06ItemPlayerOrderGet(itemSelectedType));
    W06ItemPlayerAnimationSet(itemSelectedType);
    W06ItemPromptTimeoutSet(itemSelectedType);
    W06ItemPromptStateSet(itemSelectedType, 2);
    BoardWinCreate(2, MAKE_MESSID(48, 23), 6);
    BoardWinInsertMesSet(MAKE_MESSID(8, itemPlayerChoices[itemSelectedType]), 0);
    BoardWinWait();
    BoardWinKill();
    W06ItemAnimationStateSet(1);
    W06ItemSelectionCursorMove(1);
    BoardPlayerMotBlendSet(playerIndex, 0, 0xF);
    while (!BoardPlayerMotBlendCheck(playerIndex)) {
        HuPrcVSleep();
    }
    W06ItemPromptStateSet(itemSelectedType, 3);
    audioStreamId = HuAudSStreamPlay(2);
    BoardPlayerMotionShiftSet(playerIndex, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(8);
    while (!BoardPlayerMotionEndCheck(playerIndex)) {
        HuPrcVSleep();
    }
    while (HuAudSStreamStatGet(audioStreamId) != 0) {
        HuPrcVSleep();
    }
    BoardAudSeqPause(0, 0, 1000);
    BoardPlayerItemAdd(playerIndex, itemPlayerChoices[itemSelectedType]);
    while (W06ItemAnimationStateGet() != 0) {
        HuPrcVSleep();
    }
    BoardWinCreate(2, MAKE_MESSID(48, 24), 6);
    BoardWinWait();
    BoardWinKill();
    W06ItemPlayerMotionUpdate(itemSelectedType);
    W06ItemPlayerAnimationSet(itemSelectedType);
    for (playerSlot = 0; playerSlot < 3; playerSlot++) {
        BoardModelVisibilitySet(itemPlayerProps[playerSlot], 0);
    }
}

static void W06ItemPromptSequence(s32 playerIndex, s32 shouldApproach)
{
    Vec playerPosition;
    Vec targetPosition;
    s16 currentSpaceIndex;

    currentSpaceIndex = GWPlayer[playerIndex].space_curr;
    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardSpacePosGet(0, currentSpaceIndex, &targetPosition);
    BoardCameraTargetPlayerSet(playerIndex);
    BoardCameraViewSet(1);
    if (shouldApproach != 0) {
        BoardPlayerPosLerpStart(playerIndex, &playerPosition, &targetPosition, 40);
        while (GWPlayer[playerIndex].moving) {
            HuPrcVSleep();
        }
    }
    BoardCameraMotionWait();
    W06ItemPromptStateSet(-1, 4);
}

static void W06ItemPlayerAnimationSet(s32 playerSlot)
{
    while (!BoardModelMotionEndCheck(itemPlayerModels[playerSlot])) {
        HuPrcVSleep();
    }
    if (itemPlayerStates[playerSlot] == 0) {
        BoardModelAttrReset(itemPlayerModels[playerSlot], 0x40000004);
        BoardModelAttrSet(itemPlayerModels[playerSlot], 0x40000002);
    }
}

static void W06ItemPlayerInputUpdate(s32 playerSlot)
{
    s8 playerOffsets[] = { 0x20, 0x40, 0x60 };
    s16 playerOffsets2[] = { -100, 0, 100 };
    s32 soundEffectId;

    if (itemPlayerStates[playerSlot] == 0) {
        soundEffectId = HuAudFXPlayVolPan(0x4A4, 0x7F, playerOffsets[playerSlot]);
        BoardModelMotionStart(itemPlayerModels[playerSlot], 0, 0);
        itemPlayerStates[playerSlot] = 1;
    }
}

static void W06ItemPlayerMotionUpdate(s32 playerSlot)
{
    s8 playerOffsets[] = { 0x20, 0x40, 0x60 };
    s16 playerOffsets2[] = { -100, 0, 100 };
    s32 soundEffectId;

    if (itemPlayerStates[playerSlot] != 0) {
        soundEffectId = HuAudFXPlayVolPan(0x4A5, 0x7F, playerOffsets[playerSlot]);
        BoardModelMotionStart(itemPlayerModels[playerSlot], 0, 0x40000004);
        itemPlayerStates[playerSlot] = 0;
    }
}

static void W06ItemPlayersPromptStart(s32 itemId)
{
    s32 i;
    s32 j;

    i = 0;
    while (i < 3) {
        itemPlayerChoices[i] = frandmod(14);
        if (itemPlayerChoices[i] != 10 && itemPlayerChoices[i] != 13) {
            for (j = 0; j < i; j++) {
                if (itemPlayerChoices[i] == itemPlayerChoices[j]) {
                    itemPlayerChoices[i] = -1;
                }
            }
            if (itemPlayerChoices[i] != -1) {
                i++;
            }
        }
    }
    for (i = 0; i < 3; i++) {
        W06ItemPlayerPromptCreate(itemId, i, itemPlayerChoices[i]);
    }
}

static void W06ItemPromptStateSet(s32 playerIndex, s32 timeoutFrames)
{
    W06ItemBoardWork *itemBoardWork;
    s32 i;

    for (i = 0; i < 3; i++) {
        if ((playerIndex == i || playerIndex == -1) && itemPlayerPromptObjects[i]) {
            itemBoardWork = omObjGetWork(itemPlayerPromptObjects[i], W06ItemBoardWork);
            itemBoardWork->promptState = timeoutFrames;
            itemBoardWork->animationFrames = 0;
        }
    }
}

static void W06ItemPromptTimeoutSet(s32 playerIndex)
{
    W06ItemBoardWork *itemBoardWork;
    s32 i;

    for (i = 0; i < 3; i++) {
        if ((playerIndex == i || playerIndex == -1) && itemPlayerPromptObjects[i]) {
            itemBoardWork = omObjGetWork(itemPlayerPromptObjects[i], W06ItemBoardWork);
            if (itemBoardWork->soundEffectId == -1) {
                itemBoardWork->soundEffectId = HuAudFXPlay(0x35F);
            }
        }
    }
}

static s32 W06ItemPromptTimeoutGet(s32 playerIndex)
{
    W06ItemBoardWork *itemBoardWork;

    if (!itemPlayerPromptObjects[playerIndex]) {
        return -1;
    }
    itemBoardWork = omObjGetWork(itemPlayerPromptObjects[playerIndex], W06ItemBoardWork);
    return itemBoardWork->promptState;
}

static s16 W06ItemPlayerOrderGet(s32 playerIndex)
{
    W06ItemBoardWork *itemBoardWork;

    if (!itemPlayerPromptObjects[playerIndex]) {
        return -1;
    }
    itemBoardWork = omObjGetWork(itemPlayerPromptObjects[playerIndex], W06ItemBoardWork);
    return itemBoardWork->itemModelId;
}

static void W06ItemPlayerPromptCreate(s32 itemId, s32 playerIndex, s32 itemType)
{
    Mtx playerTransform;
    Vec playerPosition;
    OMOBJ *itemObject;
    W06ItemBoardWork *itemBoardWork;
    s32 itemModelDataId;

    itemObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W06ItemPlayerPromptUpdate);
    itemPlayerPromptObjects[playerIndex] = itemObject;
    itemBoardWork = omObjGetWork(itemObject, W06ItemBoardWork);
    itemBoardWork->isActive = 0;
    itemBoardWork->promptState = 2;
    itemBoardWork->reservedByte = 0;
    itemBoardWork->promptPlayerIndex = playerIndex;
    itemBoardWork->animationFrames = 0;
    itemBoardWork->promptDelayFrames = 0;
    itemBoardWork->isVisible = 0;
    itemBoardWork->isCollected = 0;
    itemBoardWork->eventPlayerIndex = itemId;
    itemBoardWork->soundEffectId = -1;
    itemModelDataId = BoardItemModelGet(itemType);
    itemBoardWork->itemModelId = BoardModelCreate(itemModelDataId, NULL, 0);
    BoardModelMotionSpeedSet(itemBoardWork->itemModelId, 0.0f);
    BoardModelPosGet(itemPlayerModels[playerIndex], &playerPosition);
    itemObject->trans.x = playerPosition.x;
    itemObject->trans.y = playerPosition.y;
    itemObject->trans.z = playerPosition.z;
    itemObject->rot.x = 0.0f;
    itemObject->rot.y = 0.0f;
    itemObject->rot.z = 0.0f;
    if (itemType == 5) {
        MTXTrans(playerTransform, 0.0f, -50.0f, 0.0f);
        BoardModelMtxSet(itemBoardWork->itemModelId, &playerTransform);
    }
    BoardModelPosSet(itemBoardWork->itemModelId, itemObject->trans.x, itemObject->trans.y, itemObject->trans.z);
    BoardModelScaleSet(itemBoardWork->itemModelId, 0.1f, 0.1f, 0.1f);
    BoardModelLayerSet(itemBoardWork->itemModelId, 2);
}

static void W06ItemPlayerPromptUpdate(OMOBJ *promptObject)
{
    Vec playerPosition;
    Vec targetPosition;
    W06ItemBoardWork *itemBoardWork;

    itemBoardWork = omObjGetWork(promptObject, W06ItemBoardWork);
    if (itemBoardWork->isActive != 0 || BoardIsKill()) {
        if (itemBoardWork->soundEffectId != -1) {
            HuAudFXStop(itemBoardWork->soundEffectId);
            itemBoardWork->soundEffectId = -1;
        }
        BoardModelKill(itemBoardWork->itemModelId);
        itemPlayerPromptObjects[itemBoardWork->promptPlayerIndex] = NULL;
        omDelObjEx(HuPrcCurrentGet(), promptObject);
        return;
    }
    if (itemBoardWork->promptDelayFrames != 0) {
        itemBoardWork->promptDelayFrames--;
        return;
    }
    switch (itemBoardWork->promptState) {
        case 2:
            if (itemBoardWork->isCollected == 0) {
                BoardModelPosGet(itemBoardWork->itemModelId, &playerPosition);
                if (itemBoardWork->animationFrames == 0) {
                    BoardModelPosGet(itemPlayerModels[itemBoardWork->promptPlayerIndex], &targetPosition);
                    targetPosition.y += 200.0f;
                    promptObject->trans.y = (targetPosition.y - playerPosition.y) / 30.0f;
                    promptObject->rot.z = 0.01f;
                }
                else {
                    promptObject->rot.z += 0.033333335f;
                    if (promptObject->rot.z > 1.0f) {
                        promptObject->rot.z = 1.0f;
                    }
                    playerPosition.y += promptObject->trans.y;
                }
                BoardModelPosSetV(itemBoardWork->itemModelId, &playerPosition);
                BoardModelScaleSet(itemBoardWork->itemModelId, promptObject->rot.z, promptObject->rot.z, promptObject->rot.z);
                itemBoardWork->animationFrames++;
                if (itemBoardWork->animationFrames >= 31) {
                    itemBoardWork->isCollected = 1;
                }
                return;
            }
            W06ItemPromptStateSet(itemBoardWork->promptPlayerIndex, 0);
            break;
        case 1:
            if (itemBoardWork->isCollected != 0) {
                BoardModelPosGet(itemBoardWork->itemModelId, &playerPosition);
                if (itemBoardWork->animationFrames == 0) {
                    HuAudFXPlay(0x30D);
                    BoardModelPosGet(itemPlayerModels[itemBoardWork->promptPlayerIndex], &targetPosition);
                    promptObject->trans.y = (targetPosition.y - playerPosition.y) / 30.0f;
                    promptObject->rot.z = 1.0f;
                }
                else {
                    promptObject->rot.z -= 0.033333335f;
                    if (promptObject->rot.z < 0.001f) {
                        promptObject->rot.z = 0.001f;
                    }
                    playerPosition.y += promptObject->trans.y;
                }
                BoardModelPosSetV(itemBoardWork->itemModelId, &playerPosition);
                BoardModelScaleSet(itemBoardWork->itemModelId, promptObject->rot.z, promptObject->rot.z, promptObject->rot.z);
                itemBoardWork->animationFrames++;
                if (itemBoardWork->animationFrames >= 31) {
                    if (itemBoardWork->soundEffectId != -1) {
                        HuAudFXStop(itemBoardWork->soundEffectId);
                        itemBoardWork->soundEffectId = -1;
                    }
                    itemBoardWork->isCollected = 0;
                }
                return;
            }
            W06ItemPromptStateSet(itemBoardWork->promptPlayerIndex, 0);
            break;
        case 3:
            BoardModelPosGet(itemBoardWork->itemModelId, &playerPosition);
            if (itemBoardWork->animationFrames == 0) {
                BoardPlayerPosGet(itemBoardWork->eventPlayerIndex, &targetPosition);
                promptObject->rot.x = (targetPosition.x - playerPosition.x) / 30.0f;
                promptObject->rot.z = (targetPosition.z - playerPosition.z) / 30.0f;
            }
            else if (itemBoardWork->animationFrames < 30) {
                playerPosition.x += promptObject->rot.x;
                playerPosition.z += promptObject->rot.z;
                BoardModelPosSetV(itemBoardWork->itemModelId, &playerPosition);
            }
            else {
                BoardPlayerPosGet(itemBoardWork->eventPlayerIndex, &targetPosition);
                promptObject->trans.x = targetPosition.x;
                promptObject->trans.y = targetPosition.y;
                promptObject->trans.z = targetPosition.z;
                itemBoardWork->promptDelayFrames = 30;
                itemBoardWork->animationFrames = 0;
                promptObject->rot.z = 1.0f;
                W06ItemPromptStateSet(itemBoardWork->promptPlayerIndex, 1);
                break;
            }
            itemBoardWork->animationFrames++;
            break;
        case 4:
            itemBoardWork->isActive = 1;
            break;
        case 0:
            break;
    }
}

static s8 itemPromptOrder[14] = { 1, 1, 3, 3, 2, 2, 2, 1, 2, 3, 0, 4, 5, 0 };

static s32 W06ItemPromptExpectedStateGet(s32 playerIndex)
{
    s32 firstChoiceRank;
    s32 secondChoiceRank;
    s32 thirdChoiceRank;
    s32 winnerOrder;

    firstChoiceRank = itemPromptOrder[itemPlayerChoices[0]];
    secondChoiceRank = itemPromptOrder[itemPlayerChoices[1]];
    thirdChoiceRank = itemPromptOrder[itemPlayerChoices[2]];
    if (itemPromptStates[0] != 0) {
        firstChoiceRank = 0;
    }
    if (itemPromptStates[1] != 0) {
        secondChoiceRank = 0;
    }
    if (itemPromptStates[2] != 0) {
        thirdChoiceRank = 0;
    }
    if (firstChoiceRank > secondChoiceRank) {
        if (firstChoiceRank > thirdChoiceRank) {
            winnerOrder = 0;
        }
        else if (thirdChoiceRank > secondChoiceRank) {
            winnerOrder = 2;
        }
        else {
            winnerOrder = 1;
        }
    }
    else {
        if (secondChoiceRank > thirdChoiceRank) {
            winnerOrder = 1;
        }
        else if (thirdChoiceRank > firstChoiceRank) {
            winnerOrder = 2;
        }
        else {
            winnerOrder = 0;
        }
    }
    return winnerOrder;
}

static void W06ItemPromptObjectCreate(s32 targetPlayerIndex)
{
    OMOBJ *itemObject;
    W06ItemPromptWork *promptWork;

    itemObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W06ItemPromptObjectUpdate);
    itemPromptObject = itemObject;
    promptWork = omObjGetWork(itemObject, W06ItemPromptWork);
    promptWork->isActive = 0;
    promptWork->isFinished = 0;
    promptWork->targetPlayerIndex = targetPlayerIndex;
    promptWork->inputState = 0;
    promptWork->expectedState = 0;
    promptWork->inputProgress = 1;
}

static void W06ItemPromptExpectedStateUpdate(void)
{
    W06ItemPromptWork *promptWork;

    if (itemPromptObject) {
        promptWork = omObjGetWork(itemPromptObject, W06ItemPromptWork);
        promptWork->expectedState = W06ItemPromptExpectedStateGet(promptWork->targetPlayerIndex);
    }
}

static void W06ItemPromptInputStateSet(s32 inputState)
{
    W06ItemPromptWork *promptWork;

    if (itemPromptObject) {
        promptWork = omObjGetWork(itemPromptObject, W06ItemPromptWork);
        promptWork->inputState = inputState;
    }
}

static s32 W06ItemPromptInputStateGet(void)
{
    W06ItemPromptWork *promptWork;

    if (!itemPromptObject) {
        return 4;
    }
    promptWork = omObjGetWork(itemPromptObject, W06ItemPromptWork);
    return promptWork->inputState;
}

static u32 W06ItemPromptInputRead(W06ItemPromptWork *promptWork)
{
    s32 controllerPort;
    s32 pressedButtons;

    if (GWPlayer[promptWork->targetPlayerIndex].com) {
        if (promptWork->inputProgress == promptWork->expectedState) {
            return 0x100;
        }
        if (promptWork->inputProgress > promptWork->expectedState) {
            return 1;
        }
        return 2;
    }
    controllerPort = GWPlayer[promptWork->targetPlayerIndex].port;
    pressedButtons = HuPadDStk[controllerPort] | HuPadBtnDown[controllerPort];
    return pressedButtons;
}

static void W06ItemPromptObjectUpdate(OMOBJ *promptObject)
{
    Vec playerPosition;
    Vec targetPosition;
    float selectedPositionX;
    s32 inputDirection;
    s32 previousInputProgress;
    s32 lowerChoiceBound;
    s32 upperChoiceBound;
    u32 inputSelection;
    W06ItemPromptWork *promptWork;

    inputSelection = 0;
    promptWork = omObjGetWork(promptObject, W06ItemPromptWork);
    if (promptWork->isActive != 0 || BoardIsKill()) {
        itemPromptObject = NULL;
        omDelObjEx(HuPrcCurrentGet(), promptObject);
        return;
    }
    if (itemPromptStates[0] != 0) {
        upperChoiceBound = 1;
    }
    else {
        upperChoiceBound = 0;
    }
    if (itemPromptStates[2] != 0) {
        lowerChoiceBound = 1;
    }
    else {
        lowerChoiceBound = 2;
    }
    switch (promptWork->inputState) {
        case 2:
            inputSelection = W06ItemPromptInputRead(promptWork);
            if (inputSelection == 0x100 || itemTimerExpired != 0) {
                if (itemPromptStates[promptWork->inputProgress] == 0) {
                    HuAudFXPlay(2);
                    itemSelectedType = promptWork->inputProgress;
                    W06ItemPromptInputStateSet(3);
                    return;
                }
                if (itemTimerExpired == 0) {
                    HuAudFXPlay(4);
                    break;
                }
                if (promptWork->inputProgress == 1) {
                    if (itemPromptStates[0] == 0 && itemPromptStates[1] == 0) {
                        if (BoardRandMod(100) < 50) {
                            inputSelection = 1;
                        }
                        else {
                            inputSelection = 2;
                        }
                    }
                    else {
                        if (itemPromptStates[0] == 0) {
                            inputSelection = 2;
                        }
                        else if (itemPromptStates[1] == 0) {
                            inputSelection = 1;
                        }
                    }
                }
                if (promptWork->inputProgress == 0) {
                    inputSelection = 2;
                }
                if (promptWork->inputProgress == 2) {
                    inputSelection = 1;
                }
            }
            previousInputProgress = promptWork->inputProgress;
            inputDirection = 0;
            if (inputSelection & 1) {
                inputDirection = -1;
            }
            if (inputSelection & 2) {
                inputDirection = 1;
            }
            promptWork->inputProgress += inputDirection;
            if (promptWork->inputProgress == 1 && itemPromptStates[1] != 0) {
                promptWork->inputProgress += inputDirection;
            }
            if (promptWork->inputProgress > lowerChoiceBound) {
                promptWork->inputProgress = lowerChoiceBound;
            }
            if (promptWork->inputProgress < upperChoiceBound) {
                promptWork->inputProgress = upperChoiceBound;
            }
            if (promptWork->inputProgress == previousInputProgress) {
                break;
            }
            W06ItemPromptInputStateSet(1);
            BoardSpacePosGet(0, itemSelectedIndex, &targetPosition);
            BoardPlayerPosGet(promptWork->targetPlayerIndex, &playerPosition);
            OSs8tof32(&promptWork->inputProgress, &selectedPositionX);
            targetPosition.x += 150.0f * (selectedPositionX - 1.0f);
            BoardPlayerPosLerpStart(promptWork->targetPlayerIndex, &playerPosition, &targetPosition, 20);
            break;
        case 0:
            break;
        case 1:
            if (!GWPlayer[promptWork->targetPlayerIndex].moving) {
                W06ItemPromptInputStateSet(2);
                BoardPlayerRotYSet(promptWork->targetPlayerIndex, 180.0f);
                BoardPlayerIdleSet(promptWork->targetPlayerIndex);
            }
            break;
        case 3:
            W06ItemPromptInputStateSet(0);
            break;
        case 4:
            promptWork->isActive = 1;
            break;
    }
}

static void W06ItemAnimationObjectCreate(void)
{
    OMOBJ *itemObject;
    W06ItemAnimationWork *animationWork;

    itemObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W06ItemAnimationObjectUpdate);
    itemAnimationObject = itemObject;
    animationWork = omObjGetWork(itemObject, W06ItemAnimationWork);
    animationWork->isActive = 0;
    animationWork->animationState = 0;
    animationWork->currentPlayer = 0;
    animationWork->itemType = 0;
    animationWork->selectionCooldown = 0;
    animationWork->movementFrames = 0;
    animationWork->motionId = BoardModelMotionCreate(w06MapObjectModelIds[4], DATA_MAKE_NUM(DATADIR_W06, 22));
    animationWork->animationMotionId = BoardModelMotionCreate(w06MapObjectModelIds[4], DATA_MAKE_NUM(DATADIR_W06, 23));
}

static void W06ItemAnimationStateSet(s32 animationState)
{
    W06ItemAnimationWork *animationWork;

    if (!itemAnimationObject) {
        return;
    }
    animationWork = omObjGetWork(itemAnimationObject, W06ItemAnimationWork);
    animationWork->animationState = animationState;
    if (animationState != 1) {
        (void)0;
    }
    else {
        animationWork->movementFrames = 20;
    }
}

static s32 W06ItemAnimationStateGet(void)
{
    W06ItemAnimationWork *animationWork;

    if (!itemAnimationObject) {
        return 5;
    }
    animationWork = omObjGetWork(itemAnimationObject, W06ItemAnimationWork);
    return animationWork->animationState;
}

static void W06ItemPlayerOrderSet(s32 playerOrder)
{
    W06ItemAnimationWork *animationWork;

    if (!itemAnimationObject) {
        return;
    }
    animationWork = omObjGetWork(itemAnimationObject, W06ItemAnimationWork);
    switch (playerOrder) {
        case 0:
            animationWork->playerIndexes[0] = 1;
            animationWork->playerIndexes[1] = 2;
            break;
        case 2:
            animationWork->playerIndexes[0] = 1;
            animationWork->playerIndexes[1] = 0;
            break;
        default:
            animationWork->playerIndexes[0] = 0;
            animationWork->playerIndexes[1] = 2;
            break;
    }
}

static void W06ItemSelectionCursorMove(s8 playerIndex)
{
    Vec cursorPosition;
    Vec playerPosition;
    Vec cursorOffset;
    float horizontalDistance;
    float movementFrames;
    W06ItemAnimationWork *animationWork;

    if (!itemAnimationObject) {
        return;
    }
    animationWork = omObjGetWork(itemAnimationObject, W06ItemAnimationWork);
    BoardModelPosGet(w06MapObjectModelIds[4], &cursorPosition);
    BoardModelPosGet(itemPlayerModels[1], &playerPosition);
    itemAnimationObject->trans.x = playerIndex - 1;
    playerPosition.x += 150.0f * itemAnimationObject->trans.x;
    playerPosition.y = cursorPosition.y;
    playerPosition.z = cursorPosition.z;
    horizontalDistance = playerPosition.x - cursorPosition.x;
    if (ABS(horizontalDistance) < 1.0f) {
        BoardModelMotionShiftSet(w06MapObjectModelIds[4], 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
        itemAnimationObject->trans.y = 0.0f;
        return;
    }
    if (ABS(horizontalDistance) >= 300.0f) {
        movementFrames = 40.0f;
        animationWork->movementFrames *= 2;
    }
    else {
        movementFrames = 20.0f;
    }
    VECSubtract(&playerPosition, &cursorPosition, &cursorOffset);
    itemAnimationObject->trans.y = horizontalDistance / movementFrames;
    BoardModelRotYSet(w06MapObjectModelIds[4], atan2d(cursorOffset.x, cursorOffset.z));
    BoardModelMotionShiftSet(w06MapObjectModelIds[4], animationWork->motionId, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    BoardModelMotionSpeedSet(w06MapObjectModelIds[4], 2.0f);
}

static void W06ItemAnimationObjectUpdate(OMOBJ *animationObject)
{
    Vec modelPosition;
    Vec targetPosition;
    s32 movementResult;
    W06ItemAnimationWork *animationWork;

    movementResult = 0;
    animationWork = omObjGetWork(animationObject, W06ItemAnimationWork);
    if (animationWork->isActive != 0 || BoardIsKill()) {
        itemAnimationObject = NULL;
        omDelObjEx(HuPrcCurrentGet(), animationObject);
        return;
    }
    if (animationWork->selectionCooldown != 0) {
        animationWork->selectionCooldown--;
        return;
    }
    switch (animationWork->animationState) {
        case 0:
            break;
        case 3:
            if (animationWork->currentPlayer >= 2) {
                if (BoardRandMod(100) < 60) {
                    W06ItemAnimationStateSet(4);
                    return;
                }
            }
            else {
                animationWork->currentPlayer++;
            }
            W06ItemAnimationStateSet(1);
            animationWork->itemType ^= 1;
            W06ItemSelectionCursorMove(animationWork->playerIndexes[animationWork->itemType]);
            animationWork->isVisible = 1;
            break;
        case 1:
            if (animationWork->movementFrames != 0) {
                BoardModelPosGet(w06MapObjectModelIds[4], &modelPosition);
                modelPosition.x += animationObject->trans.y;
                BoardModelPosSetV(w06MapObjectModelIds[4], &modelPosition);
                animationWork->movementFrames--;
                break;
            }
            if (animationWork->isVisible != 0) {
                W06ItemAnimationStateSet(2);
                animationWork->movementFrames = 0;
                animationWork->selectionCooldown = 10;
                break;
            }
            W06ItemAnimationStateSet(0);
            animationWork->movementFrames = 0;
            BoardModelRotYSet(w06MapObjectModelIds[4], 0.0f);
            BoardModelMotionShiftSet(w06MapObjectModelIds[4], 1, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
            break;
        case 2:
            if (animationWork->movementFrames++ == 0) {
                BoardModelMotionShiftSet(w06MapObjectModelIds[4], animationWork->animationMotionId, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
                break;
            }
            if (animationWork->movementFrames < 50) {
                BoardModelRotYSet(w06MapObjectModelIds[4], 0.0f);
                break;
            }
            W06ItemAnimationStateSet(3);
            break;
        case 4:
            animationWork->isVisible = 0;
            itemSelectedType = animationWork->playerIndexes[animationWork->itemType];
            BoardModelMotionShiftSet(w06MapObjectModelIds[4], 1, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
            W06ItemAnimationStateSet(0);
            break;
        case 5:
            animationWork->isActive = 1;
            break;
    }
}

static void W06ItemTimerObjectCreate(void)
{
    W06ItemTimerWork *timerWork;

    itemTimerExpired = 0;
    itemTimerObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W06ItemTimerObjectUpdate);
    timerWork = omObjGetWork(itemTimerObject, W06ItemTimerWork);
    timerWork->isActive = 0;
    timerWork->timerValue = 5;
    timerWork->tickDelay = 60;
    timerWork->timerSequenceId = MGSeqTimerCreateXY(timerWork->timerValue, 288, 64);
}

static void W06ItemTimerObjectStop(void)
{
    W06ItemTimerWork *timerWork;

    if (!itemTimerObject) {
        return;
    }
    timerWork = omObjGetWork(itemTimerObject, W06ItemTimerWork);
    MGSeqParamSet(timerWork->timerSequenceId, 2, -1);
    timerWork->timerSequenceId = -1;
    timerWork->isActive = 1;
}

static void W06ItemTimerObjectUpdate(OMOBJ *timerObject)
{
    W06ItemTimerWork *timerWork;

    timerWork = omObjGetWork(timerObject, W06ItemTimerWork);
    if (timerWork->isActive != 0 || BoardIsKill()) {
        if (timerWork->timerSequenceId != -1) {
            MGSeqParamSet(timerWork->timerSequenceId, 2, -1);
        }
        itemTimerObject = NULL;
        omDelObjEx(HuPrcCurrentGet(), timerObject);
        return;
    }
    if (timerWork->timerSequenceId == -1) {
        return;
    }
    if (timerWork->tickDelay != 0) {
        timerWork->tickDelay--;
        return;
    }
    if (timerWork->timerValue != 0) {
        timerWork->timerValue--;
    }
    else {
        MGSeqParamSet(timerWork->timerSequenceId, 2, -1);
        timerWork->timerSequenceId = -1;
        itemTimerExpired = 1;
        return;
    }
    MGSeqParamSet(timerWork->timerSequenceId, 1, timerWork->timerValue);
    timerWork->tickDelay = 60;
}

static void W06ItemParticleCreate(s16 emitCount)
{
    HU3DPARTICLE *particleEmitter;

    itemParticleAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_EFFECT, 0));
    HuSprAnimLock(itemParticleAnimation);
    itemParticleModel = Hu3DParticleCreate(itemParticleAnimation, 100);
    Hu3DParticleHookSet(itemParticleModel, W06ItemParticleUpdate);
    Hu3DParticleColSet(itemParticleModel, 0xFF, 0xFF, 0);
    Hu3DModelPosSet(itemParticleModel, 0.0f, 0.0f, 0.0f);
    Hu3DParticleBlendModeSet(itemParticleModel, 1);
    Hu3DModelLayerSet(itemParticleModel, 2);
    particleEmitter = Hu3DData[itemParticleModel].hookData;
    particleEmitter->emitCnt = emitCount;
}

static void W06ItemParticleDestroy(void)
{
    if (itemParticleModel != -1) {
        Hu3DModelKill(itemParticleModel);
        itemParticleModel = -1;
    }
    if (itemParticleAnimation) {
        HuSprAnimKill(itemParticleAnimation);
        itemParticleAnimation = NULL;
    }
}

static void W06ItemParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    Vec emitterPosition;
    float emitterScale;
    float particleAngle;
    float modelScale;
    s32 i;
    s32 j;

    if (particle->count == 0) {
        particleData = particle->data;
        for (i = 0; i < particle->maxCnt; i++, particleData++) {
            particleData->color.a = 0;
            particleData->scale = 0.0f;
        }
        particle->dataCnt = 0;
    }
    BoardModelScaleGet(particle->emitCnt, &emitterPosition);
    emitterScale = emitterPosition.x;
    BoardModelPosGet(particle->emitCnt, &emitterPosition);
    for (i = 0; i < 22; i++) {
        particleData = particle->data;
        for (j = 0; j < particle->maxCnt; j++, particleData++) {
            if (particleData->scale == 0.0f) {
                break;
            }
        }
        if (j != particle->maxCnt) {
            particleAngle = 0.003921569f * frand8() * 360.0f;
            modelScale = emitterScale * (20.0f + 0.003921569f * frand8() * 60.0f);
            particleData->pos.x = emitterPosition.x + modelScale * sind(particleAngle);
            particleData->pos.z = emitterPosition.z + modelScale * cosd(particleAngle);
            particleData->pos.y = emitterPosition.y + emitterScale * (-30.0f + 0.003921569f * frand8() * 40.0f);
            particleData->vel.x = 0.5f + 0.003921569f * frand8() * 3.0f;
            particleData->vel.y = 0.3f + 0.003921569f * frand8() * 2.0f;
            particleData->color.a = 0xB4;
            particleData->scale = 15.0f * emitterScale;
        }
    }
    particleData = particle->data;
    for (i = 0; i < particle->maxCnt; i++, particleData++) {
        if (particleData->scale != 0.0f) {
            particleData->pos.y -= particleData->vel.x;
            particleData->scale -= particleData->vel.y;
            if (particleData->scale <= 0.0f) {
                particleData->scale = 0.0f;
            }
        }
    }
}
