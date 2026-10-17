#include "REL/w02Dll.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/frand.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"

#include "ext_math.h"

#include "game/board/audio.h"

#if VERSION_JP
#define MDL_ID_SHIFT 0
#else
#define MDL_ID_SHIFT 1
#endif

s32 itemModelData[14] = {
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

char *itemModelHooks[3] = { "item", "item1", "item2" };

s32 itemMotionData[] = { DATA_MAKE_NUM(DATADIR_W02, 0x18), DATA_MAKE_NUM(DATADIR_W02, 0x19), DATA_MAKE_NUM(DATADIR_W02, 0x1A), -1 };

u16 minigameDone;
s16 itemPlayerIndex;
s16 itemPlayerModelIds[3];
s16 itemModelIds[3];
s16 itemEffectModelIds[3];
Vec itemPositions[3];
Vec itemDirections[3];
Vec itemScales[3];
float itemRotationSpeed;
float itemMovementSpeed;
OMOBJ *itemTimerObject;
OMOBJ *itemPlayerObject;
OMOBJ *itemMovementObject;
HUPROCESS *itemAnimationProcess;
Vec itemStartPositions[3];
s32 itemSelectedIndex;
s32 itemParticleIds[3];
ANIMDATA *itemParticleAnimationData;

void ItemMinigameModelsInitialize(void)
{
    BoardModelVisibilitySet(boardModelIds[10], 1);
}

typedef struct ItemMinigameWork {
    s16 remainingSeconds;
    s16 timerFrameCounter;
    s16 timerSequenceId;
    s16 instructionsWindowId;
    s16 playerPositionStep;
    s16 directionChangeDelay;
    s16 moveState;
} WorkMGItem;

typedef struct ItemMinigamePlayerWork {
    Vec itemDirectionVectors[3];
    s16 itemIconModelIds[3];
    s16 itemModelIds[3];
    float itemAngles[2];
    float itemAngularVelocities[2];
    s16 itemRotationModes[2];
    Vec itemPositions[3];
    s16 selectedItemIndex;
} WorkMGItemPlayer;

void ItemMinigameWaitForAnimation(void);

void ItemMinigamePlayerUpdate(OMOBJ *object);
void ItemMinigameTimerUpdate(OMOBJ *object);

void ItemMinigameStart(void)
{
    Vec playerPosition;
    WorkMGItem *modelPosition;
    itemPlayerIndex = GWSystem.player_curr;
    BoardPlayerPosGet(itemPlayerIndex, &playerPosition);
    BoardPlayerPosLerpStart(itemPlayerIndex, &playerPosition, &itemStartPositions[1], 20);
    while (GWPlayer[itemPlayerIndex].moving) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(itemPlayerIndex);
    HuPrcSleep(REFRESH_RATE*2);
    BoardAudSeqPause(0, 1, 1000);
    HuAudFXPlay(781);
    ItemMinigameWaitForAnimation();
    HuPrcSleep(REFRESH_RATE);
    itemMovementObject = omAddObjEx(boardObjMan, 257, 0, 0, -1, ItemMinigamePlayerUpdate);
    itemPlayerObject = omAddObjEx(boardObjMan, 257, 0, 0, -1, ItemMinigameTimerUpdate);
    itemTimerObject = omAddObjEx(boardObjMan, 257, 0, 0, -1, NULL);
    itemPlayerObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(WorkMGItemPlayer), HU_MEMNUM_OVL);
    itemTimerObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(WorkMGItem), HU_MEMNUM_OVL);
    modelPosition = itemTimerObject->data;
    minigameDone = 0;
}

void ItemMinigameTimerStart(void);

void ItemMinigamePlayerUpdate(OMOBJ *object)
{
    if (minigameDone && BoardMGDoneFlagGet() == 1) {
        ItemMinigameTimerStart();
        omDelObjEx(boardObjMan, object);
    }
}

void ItemMinigamePlayerCleanup(void)
{
    minigameDone = 1;
    BoardMGExit();
}

void ItemMinigamePlayerDestroy(void)
{
    s32 i;
    for (i = 0; i < 3; i++) {
        BoardModelKill(itemEffectModelIds[i]);
        BoardModelKill(itemModelIds[i]);
    }
    if (itemParticleAnimationData) {
        HuSprAnimKill(itemParticleAnimationData);
        itemParticleAnimationData = NULL;
    }
}

void ItemMinigameTimerStart(void)
{
    omDelObjEx(boardObjMan, itemTimerObject);
    BoardMGDoneFlagSet(0);
}

void ItemMinigamePlayerAnimate(OMOBJ *object);

void ItemMinigameTimerUpdate(OMOBJ *object)
{
    WorkMGItemPlayer *itemWork = object->data;
    itemWork->itemIconModelIds[0] = itemModelIds[0];
    itemWork->itemIconModelIds[1] = itemModelIds[1];
    itemWork->itemIconModelIds[2] = itemModelIds[2];
    BoardModelPosGet(itemModelIds[1], &itemWork->itemPositions[0]);
    itemWork->itemPositions[0].x -= itemMovementSpeed;
    BoardModelPosGet(itemModelIds[1], &itemWork->itemPositions[1]);
    itemWork->itemPositions[1].x += itemMovementSpeed;
    BoardModelPosGet(itemModelIds[1], &itemWork->itemPositions[2]);
    BoardMusStart(1, 19, 127, 0);
    object->work[0] = REFRESH_RATE*5;
    object->work[1] = 0;
    object->objFunc = ItemMinigamePlayerAnimate;
}

void ItemMinigameItemMotionSet(s16 firstItemIndex, s16 secondItemIndex)
{
    s32 swappedModelId;
    swappedModelId = itemPlayerModelIds[firstItemIndex];
    itemPlayerModelIds[firstItemIndex] = itemPlayerModelIds[secondItemIndex];
    itemPlayerModelIds[secondItemIndex] = swappedModelId;
    swappedModelId = itemEffectModelIds[firstItemIndex];
    itemEffectModelIds[firstItemIndex] = itemEffectModelIds[secondItemIndex];
    itemEffectModelIds[secondItemIndex] = swappedModelId;
}

void ItemMinigamePlayerFinish(OMOBJ *object);

void ItemMinigamePlayerAnimate(OMOBJ *object)
{
    float distance;
    float duration;
    Vec itemPosition;
    Vec itemTargetPosition;
    Vec modelRotation;
    Vec modelScale;
    u8 modelPosition[8] = { 0, 1, 1, 2, 0, 1, 0, 2 };
    WorkMGItemPlayer *itemWork = object->data;
    s32 playerIndex;
    s32 itemModelId;
    if (object->work[0]) {
        object->work[0]--;
    }
    if (object->work[1] == 0) {
        if (object->work[0] == 0) {
            object->objFunc = ItemMinigamePlayerFinish;
        }
        itemModelId = modelPosition[frand() & 0x7];
        switch (itemModelId) {
            case 0:
                itemWork->itemModelIds[0] = itemWork->itemIconModelIds[0];
                itemWork->itemModelIds[1] = itemWork->itemIconModelIds[1];
                itemTargetPosition = itemPositions[0];
                itemPosition = itemPositions[1];
                distance = ((REFRESH_RATE+3)/4);
                break;

            case 1:
                itemWork->itemModelIds[0] = itemWork->itemIconModelIds[1];
                itemWork->itemModelIds[1] = itemWork->itemIconModelIds[2];
                itemTargetPosition = itemPositions[1];
                itemPosition = itemPositions[2];
                distance = ((REFRESH_RATE+3)/4);
                break;

            case 2:
                itemWork->itemModelIds[0] = itemWork->itemIconModelIds[0];
                itemWork->itemModelIds[1] = itemWork->itemIconModelIds[2];
                distance = ((REFRESH_RATE+3)/4)*1.5f;
                break;
        }
        itemWork->selectedItemIndex = itemModelId;
        itemWork->itemAngles[0] = 180;
        itemWork->itemAngles[1] = 0;
        if (itemModelId != 2) {
            itemWork->itemRotationModes[0] = frandmod(3);
            itemWork->itemRotationModes[1] = frandmod(3);
            while (itemWork->itemRotationModes[1] == itemWork->itemRotationModes[0]) {
                itemWork->itemRotationModes[1] = frandmod(3);
            }
            VECSubtract(&itemPosition, &itemTargetPosition, &itemWork->remainingSeconds[0]);
            VECScale(&itemWork->remainingSeconds[0], &itemWork->remainingSeconds[0], 1.0f / distance);
            VECSubtract(&itemTargetPosition, &itemPosition, &itemWork->remainingSeconds[1]);
            VECScale(&itemWork->remainingSeconds[1], &itemWork->remainingSeconds[1], 1.0f / distance);
        }
        else {
            itemWork->itemRotationModes[0] = frandmod(2);
            itemWork->itemRotationModes[1] = itemWork->itemRotationModes[0] ^ 1;
        }
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            if (itemWork->itemRotationModes[playerIndex] == 0) {
                itemWork->itemAngularVelocities[playerIndex] = 180.0f / distance;
            }
            else {
                itemWork->itemAngularVelocities[playerIndex] = -180.0f / distance;
            }
            if (playerIndex == 1) {
                itemWork->itemAngularVelocities[playerIndex] = -itemWork->itemAngularVelocities[playerIndex];
            }
        }
        object->work[2] = distance;
        object->work[1] = 1;
        BoardModelMotionStart(itemWork->itemModelIds[playerIndex], 2, 0x40000001);
        BoardModelMotionSpeedSet(itemWork->itemModelIds[playerIndex], 3.0f);
    }
    else {
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            BoardModelPosGet(itemWork->itemModelIds[playerIndex], &modelRotation);
            modelScale = modelRotation;
            if (itemWork->selectedItemIndex != 2) {
                duration = itemMovementSpeed;
            }
            else {
                duration = 2.0f * itemMovementSpeed;
            }
            if (itemWork->itemRotationModes[playerIndex] != 2) {
                modelRotation.x = itemWork->itemPositions[itemWork->selectedItemIndex].x + (duration * cosd(itemWork->itemAngles[playerIndex]));
                modelRotation.z = itemWork->itemPositions[itemWork->selectedItemIndex].z + (duration * sind(itemWork->itemAngles[playerIndex]));
                itemWork->itemAngles[playerIndex] += itemWork->itemAngularVelocities[playerIndex];
            }
            else {
                VECAdd(&modelRotation, &itemWork->remainingSeconds[playerIndex], &modelRotation);
            }
            BoardModelPosSetV(itemWork->itemModelIds[playerIndex], &modelRotation);
            VECSubtract(&modelRotation, &modelScale, &modelScale);
            BoardModelRotGet(itemWork->itemModelIds[playerIndex], &modelRotation);
            modelRotation.y = atan2d(modelScale.x, modelScale.z);
            BoardModelRotSetV(itemWork->itemModelIds[playerIndex], &modelRotation);
        }
        if (--object->work[2] == 0) {
            object->work[1] = 0;
            BoardModelMotionStart(itemWork->itemModelIds[playerIndex], 1, 0x40000001);
            BoardModelMotionSpeedSet(itemWork->itemModelIds[playerIndex], 1.0f);
            for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                if (itemWork->itemRotationModes[playerIndex] == 0) {
                    BoardModelRotGet(itemWork->itemModelIds[playerIndex], &modelRotation);
                    modelRotation.y = 0;
                    BoardModelRotSetV(itemWork->itemModelIds[playerIndex], &modelRotation);
                }
                else if (itemWork->itemRotationModes[playerIndex] == 1) {
                    BoardModelRotGet(itemWork->itemModelIds[playerIndex], &modelRotation);
                    modelRotation.y = 180;
                    BoardModelRotSetV(itemWork->itemModelIds[playerIndex], &modelRotation);
                }
            }
            switch (itemWork->selectedItemIndex) {
                case 0:
                    itemWork->itemIconModelIds[0] = itemWork->itemModelIds[1];
                    itemWork->itemIconModelIds[1] = itemWork->itemModelIds[0];
                    ItemMinigameItemMotionSet(0, 1);
                    break;

                case 1:
                    itemWork->itemIconModelIds[1] = itemWork->itemModelIds[1];
                    itemWork->itemIconModelIds[2] = itemWork->itemModelIds[0];
                    ItemMinigameItemMotionSet(1, 2);
                    break;

                case 2:
                    itemWork->itemIconModelIds[0] = itemWork->itemModelIds[1];
                    itemWork->itemIconModelIds[2] = itemWork->itemModelIds[0];
                    ItemMinigameItemMotionSet(0, 2);
                    break;
            }
            for (playerIndex = 0; playerIndex < 3; playerIndex++) {
                BoardModelPosSetV(itemWork->itemIconModelIds[playerIndex], &itemPositions[playerIndex]);
            }
        }
    }
}

void ItemMinigameItemAnimate(OMOBJ *object);

void ItemMinigamePlayerFinish(OMOBJ *object)
{
    Vec modelPosition;
    s32 itemWork;
    s32 playerIndex;
    playerIndex = 0;
    for (itemWork = 0; itemWork < 3; itemWork++) {
        BoardModelRotGet(itemModelIds[itemWork], &modelPosition);
        if (modelPosition.y < 0.0f) {
            modelPosition.y += 360.0f;
        }
        if (0.0f == modelPosition.y) {
            playerIndex++;
            continue;
        }
        if (modelPosition.y < 180.0f) {
            if (modelPosition.y < -10.0f) {
                modelPosition.y = 0;
            }
            else {
                modelPosition.y -= 10.0f;
            }
        }
        else {
            if (360.0f - modelPosition.y < 10.0f) {
                modelPosition.y = 0;
            }
            else {
                modelPosition.y += 10.0f;
            }
        }
        BoardModelRotSetV(itemModelIds[itemWork], &modelPosition);
    }
    if (playerIndex == 3) {
        for (itemWork = 0; itemWork < 3; itemWork++) {
            BoardModelPosSetV(itemModelIds[itemWork], &itemPositions[itemWork]);
        }
        object->work[0] = MGSeqStartCreate();
        object->objFunc = ItemMinigameItemAnimate;
    }
}

void ItemMinigameItemSelect(OMOBJ *object);

void ItemMinigameItemAnimate(OMOBJ *object)
{
    if (!MGSeqStatGet(object->work[0])) {
        itemTimerObject->objFunc = ItemMinigameItemSelect;
        object->objFunc = NULL;
    }
}

void ItemMinigameItemMotionUpdate(OMOBJ *object);
void ItemMinigameItemsCollect(void);
void ItemMinigameItemsCleanup(void);

void ItemMinigameItemMotionStart(OMOBJ *object)
{
    Vec modelPosition;
    BoardModelRotGet(itemModelIds[object->work[0]], &modelPosition);
    modelPosition.y += 8.0f;
    if (modelPosition.y >= 360.0f) {
        modelPosition.y = 0.0f;
        HuAudFXPlay(784);
        itemAnimationProcess = HuPrcCreate(ItemMinigameItemsCollect, 8196, 8192, 0);
        HuPrcDestructorSet2(itemAnimationProcess, ItemMinigameItemsCleanup);
        *((u32 *)&itemAnimationProcess->user_data) = (1 << object->work[0]) | 0x10000000;
        itemSelectedIndex = HuAudFXPlay(863);
        object->objFunc = ItemMinigameItemMotionUpdate;
    }
    BoardModelRotSetV(itemModelIds[object->work[0]], &modelPosition);
}

void ItemMinigameTimerFinish(void);

void ItemMinigameItemMotionUpdate(OMOBJ *object)
{
    HUPROCESS *itemWork;
    if ((u32)itemAnimationProcess->user_data & 0x40000000) {
        itemWork = HuPrcCreate(ItemMinigameTimerFinish, 8196, 6144, 0);
        itemWork->user_data = object;
        object->objFunc = NULL;
    }
}

void ItemMinigameEffectUpdate(OMOBJ *object);

void ItemMinigameTimerFinish(void)
{
    Vec itemDirection;
    Vec positionDelta;
    Vec position;
    Vec modelPosition;
    float distance;
    float duration;
    float angle;
    s32 itemWork;
    s32 playerIndex;
    s32 itemIndex;
    s32 itemModelId;

    itemWork = itemPlayerObject->work[0];
    BoardWinCreate(2, MAKE_MESSID(19, 24), 4);
    BoardWinInsertMesSet(MAKE_MESSID(8, itemPlayerModelIds[itemWork]), 1);
    BoardWinWait();
    BoardWinKill();
    BoardPlayerPosGet(itemPlayerIndex, &positionDelta);
    BoardModelPosGet(itemEffectModelIds[itemWork], &itemDirection);
    positionDelta.y = itemDirection.y = 0;
    VECSubtract(&positionDelta, &itemDirection, &position);
    duration = VECMag(&position);
    VECScale(&position, &position, 1.0f / REFRESH_RATE);
    for (playerIndex = 0; playerIndex < REFRESH_RATE; playerIndex++) {
        BoardModelPosGet(itemEffectModelIds[itemWork], &itemDirection);
        VECAdd(&itemDirection, &position, &itemDirection);
        BoardModelPosSetV(itemEffectModelIds[itemWork], &itemDirection);
        HuPrcVSleep();
    }
    BoardModelPosGet(itemEffectModelIds[itemWork], &itemDirection);
    itemDirection.x = positionDelta.x;
    itemDirection.z = positionDelta.z;
    BoardModelPosSetV(itemEffectModelIds[itemWork], &itemDirection);
    BoardPlayerMotBlendSet(itemPlayerIndex, 0, 15);
    while (!BoardPlayerMotBlendCheck(itemPlayerIndex)) {
        HuPrcVSleep();
    }
    HuPrcKill(itemAnimationProcess);
    itemAnimationProcess = NULL;
    HuPrcVSleep();
    HuAudFXPlay(781);
    BoardModelPosGet(itemEffectModelIds[itemWork], &itemDirection);
    BoardModelScaleGet(itemEffectModelIds[itemWork], &modelPosition);
    distance = modelPosition.x;
    angle = -0.04f;
    duration = (positionDelta.y - itemDirection.y) / (REFRESH_RATE/2);
    itemIndex = 0;
    HuAudFXFadeOut(itemSelectedIndex, (((REFRESH_RATE*500)+30)/60)-1);
    while (!itemIndex) {
        itemDirection.y += duration;
        distance += angle;
        if (distance < 0.0f) {
            distance = 0.0f;
            itemIndex = 1;
        }
        modelPosition.x = modelPosition.y = modelPosition.z = distance;
        BoardModelPosSetV(itemEffectModelIds[itemWork], &itemDirection);
        BoardModelScaleSetV(itemEffectModelIds[itemWork], &modelPosition);
        HuPrcVSleep();
    }
    HuAudFXStop(itemSelectedIndex);
    BoardModelVisibilitySet(itemEffectModelIds[itemWork], 0);
    BoardPlayerItemAdd(itemPlayerIndex, itemPlayerModelIds[itemWork]);
    omVibrate(itemPlayerIndex, 12, 6, 6);
    itemModelId = HuAudSStreamPlay(2);
    BoardPlayerMotionShiftSet(itemPlayerIndex, 7, 0, 5, HU3D_MOTATTR_NONE);
    while (!BoardPlayerMotionEndCheck(itemPlayerIndex)) {
        HuPrcVSleep();
    }
    HuPrcSleep(REFRESH_RATE/2);
    while (HuAudSStreamStatGet(itemModelId)) {
        HuPrcVSleep();
    }
    BoardAudSeqPause(0, 0, 1000);
    HuAudFXPlay(68);
    W02MesExec(MAKE_MESSID(0x13, 0x19));
    itemPlayerObject->objFunc = ItemMinigameEffectUpdate;
    HuPrcKill(NULL);
    while (1) {
        HuPrcVSleep();
    }
}

void ItemMinigameEffectUpdate(OMOBJ *object)
{
    ItemMinigamePlayerCleanup();
    omDelObjEx(boardObjMan, object);
}

void ItemMinigameItemReset(s32 itemIndex)
{
    itemPlayerObject->work[0] = itemIndex;
    itemPlayerObject->objFunc = ItemMinigameItemMotionStart;
}

void ItemMinigamePlayerReset(WorkMGItem *timerWork)
{
    if (--timerWork->timerFrameCounter == 0) {
        timerWork->timerFrameCounter = REFRESH_RATE;
        if (--timerWork->remainingSeconds >= 0) {
            MGSeqParamSet(timerWork->timerSequenceId, 1, timerWork->remainingSeconds);
        }
    }
}

void ItemMinigameItemsAnimate(OMOBJ *object);

void ItemMinigameItemSelect(OMOBJ *object)
{
    float modelPosition[2];
    WorkMGItem *itemWork;
    itemWork = object->data;
    itemWork->remainingSeconds = 5;
    itemWork->timerFrameCounter = REFRESH_RATE;
    itemWork->timerSequenceId = MGSeqTimerCreateXY(itemWork->remainingSeconds, 288, 64);
    HuWinMesMaxSizeGet(1, modelPosition, MAKE_MESSID(0x13, 0x1B));
    itemWork->instructionsWindowId = HuWinCreate(-10000, 368, modelPosition[0], modelPosition[1], 0);
    HuWinMesSet(itemWork->instructionsWindowId, MAKE_MESSID(0x13, 0x1B));
    HuWinBGTPLvlSet(itemWork->instructionsWindowId, 0);
    HuWinMesSpeedSet(itemWork->instructionsWindowId, 0);
    itemWork->playerPositionStep = 1;
    itemWork->directionChangeDelay = frandmod(30) + 15;
    itemWork->moveState = 0;
    object->objFunc = ItemMinigameItemsAnimate;
}

void ItemMinigameItemEffectsUpdate(OMOBJ *timerObject, s32 *directionButtons, s32 *actionButtons);

void ItemMinigameItemsCleanup(OMOBJ *object);
void ItemMinigameItemsDestroy(OMOBJ *object);

void ItemMinigameItemsAnimate(OMOBJ *object)
{
    s32 playerPosition;
    s32 modelPosition;
    WorkMGItem *itemWork;
    s32 itemIndex;
    itemWork = object->data;
    ItemMinigamePlayerReset(itemWork);
    itemIndex = GWPlayer[itemPlayerIndex].port;
    if (GWPlayer[itemPlayerIndex].com) {
        ItemMinigameItemEffectsUpdate(object, &playerPosition, &modelPosition);
    }
    else {
        playerPosition = HuPadDStkRep[itemIndex];
        modelPosition = HuPadBtnDown[itemIndex];
    }
    if ((modelPosition & PAD_BUTTON_A) || itemWork->remainingSeconds < 0) {
        MGSeqParamSet(itemWork->timerSequenceId, 2, -1);
        HuWinKill(itemWork->instructionsWindowId);
        BoardPlayerMotBlendSet(itemPlayerIndex, 180, 10);
        BoardAudSeqFadeOut(1, 1000);
        object->objFunc = ItemMinigameItemsDestroy;
    }
    else if (playerPosition & PAD_BUTTON_LEFT) {
        if (itemWork->playerPositionStep != 0) {
            itemWork->playerPositionStep--;
            BoardPlayerPosLerpStart(itemPlayerIndex, &itemStartPositions[itemWork->playerPositionStep + 1], &itemStartPositions[itemWork->playerPositionStep], REFRESH_RATE/5);
            object->objFunc = ItemMinigameItemsCleanup;
        }
    }
    else if (playerPosition & PAD_BUTTON_RIGHT) {
        if (itemWork->playerPositionStep != 2) {
            itemWork->playerPositionStep++;
            BoardPlayerPosLerpStart(itemPlayerIndex, &itemStartPositions[itemWork->playerPositionStep - 1], &itemStartPositions[itemWork->playerPositionStep], REFRESH_RATE/5);
            object->objFunc = ItemMinigameItemsCleanup;
        }
    }
}

void ItemMinigameItemsCleanup(OMOBJ *object)
{
    WorkMGItem *itemWork = object->data;
    ItemMinigamePlayerReset(itemWork);
    if (!GWPlayer[itemPlayerIndex].moving) {
        BoardPlayerIdleSet(itemPlayerIndex);
        object->objFunc = ItemMinigameItemsAnimate;
    }
}

void ItemMinigameItemsDestroy(OMOBJ *object)
{
    if (BoardPlayerMotBlendCheck(itemPlayerIndex)) {
        ItemMinigameItemReset(((WorkMGItem *)object->data)->playerPositionStep);
        object->objFunc = NULL;
    }
}

void ItemMinigameItemEffectsUpdate(OMOBJ *timerObject, s32 *directionButtons, s32 *actionButtons)
{
    WorkMGItem *itemWork;
    s32 itemModelId;
    itemWork = timerObject->data;
    *directionButtons = *actionButtons = 0;
    if (itemWork->directionChangeDelay) {
        itemWork->directionChangeDelay--;
    }
    else {
        itemWork->directionChangeDelay = frandmod(30) + 15;
        if (itemWork->moveState == 4) {
            *actionButtons |= PAD_BUTTON_A;
        }
        else if ((s32)frandmod(10) < 4) {
            *actionButtons |= PAD_BUTTON_A;
        }
        else {
            itemModelId = frandBool();
            if ((itemModelId == 0 && itemWork->playerPositionStep != 2) || itemWork->playerPositionStep == 0) {
                *directionButtons |= PAD_BUTTON_RIGHT;
            }
            else if ((itemModelId == 1 && itemWork->playerPositionStep != 0) || itemWork->playerPositionStep == 2) {
                *directionButtons |= PAD_BUTTON_LEFT;
            }
            itemWork->moveState++;
        }
    }
}

static inline void WriteTrans(s32 index, float value)
{
    Hu3DData[BoardModelIDGet(itemEffectModelIds[index])].mtx[1][3] = value;
}

void ItemMinigameItemsRandomize(s16 *itemChoices)
{
    float distance;
    float duration;
    float angle;
    float scale;
    Vec modelPosition;
    s32 itemWork;
    s32 itemIndex;
    for (itemWork = 0; itemWork < 3; itemWork++) {
        itemPlayerModelIds[itemWork] = itemChoices[itemWork];
        itemEffectModelIds[itemWork] = BoardModelCreate(itemModelData[itemPlayerModelIds[itemWork]], NULL, 0);
        if (itemPlayerModelIds[itemWork] == 5) {
            WriteTrans(itemWork, -50.0f);
        }
        if (itemChoices[itemWork] == 4) {
            BoardModelMotionSpeedSet(itemEffectModelIds[itemWork], 0);
        }
        BoardModelVisibilitySet(itemEffectModelIds[itemWork], 0);
        if (itemChoices[itemWork] == 11) {
            BoardModelAttrSet(itemEffectModelIds[itemWork], 0x40000001);
        }
    }
    if (itemParticleAnimationData == NULL) {
        itemParticleAnimationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_EFFECT, 0x00));
        HuSprAnimLock(itemParticleAnimationData);
    }
    for (itemWork = 0; itemWork < 3; itemWork++) {
        itemModelIds[itemWork] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W02, 0x17), itemMotionData, 0);
        itemScales[itemWork].x = itemScales[itemWork].z = 0.1f;
        itemScales[itemWork].y = 0.1f;
        itemDirections[itemWork].x = 0;
        itemDirections[itemWork].y = itemDirections[itemWork].z = 0;
        Hu3DModelObjPosGet(BoardModelIDGet(boardModelIds[10]), itemModelHooks[itemWork], &itemPositions[itemWork]);
        BoardModelPosSetV(itemModelIds[itemWork], &itemPositions[itemWork]);
        BoardModelRotSetV(itemModelIds[itemWork], &itemDirections[itemWork]);
        BoardModelScaleSetV(itemModelIds[itemWork], &itemScales[itemWork]);
    }
    itemRotationSpeed = itemPositions[0].y;
    for (itemWork = 0; itemWork < 3; itemWork++) {
        BoardModelPosGet(boardModelIds[10], &itemStartPositions[itemWork]);
        itemStartPositions[itemWork].x = itemPositions[itemWork].x;
        itemStartPositions[itemWork].z = itemPositions[itemWork].z + 200.0f;
        BoardModelMotionStart(itemModelIds[itemWork], 3, 0);
    }
    itemMovementSpeed = fabs(itemPositions[0].x - itemPositions[1].x) / 2;
    distance = 0;
    angle = 20;
    modelPosition.x = 0.04f;
    modelPosition.y = 0.04f;
    modelPosition.z = 0.04f;
    for (itemIndex = 0; itemIndex < 3;) {
        duration = angle - ((7.0f / 60.0f) * (0.25f * (distance * distance)));
        distance++;
        itemIndex = 0;
        for (itemWork = 0; itemWork < 3; itemWork++) {
            itemPositions[itemWork].y += duration;
            if (itemPositions[itemWork].y < itemRotationSpeed) {
                itemPositions[itemWork].y = itemRotationSpeed;
                itemIndex++;
            }
            BoardModelPosSetV(itemModelIds[itemWork], &itemPositions[itemWork]);
            VECAdd(&itemScales[itemWork], &modelPosition, &itemScales[itemWork]);
            if (itemScales[itemWork].x > 0.7f) {
                itemScales[itemWork].x = 0.7f;
            }
            if (itemScales[itemWork].y > 0.7f) {
                itemScales[itemWork].y = 0.7f;
            }
            if (itemScales[itemWork].z > 0.7f) {
                itemScales[itemWork].z = 0.7f;
            }
            BoardModelScaleSetV(itemModelIds[itemWork], &itemScales[itemWork]);
        }
        HuPrcVSleep();
    }
    for (itemWork = 0; itemWork < 3; itemWork++) {
        itemScales[itemWork].x = itemScales[itemWork].y = itemScales[itemWork].z = 0.7f;
        BoardModelScaleSetV(itemModelIds[itemWork], &itemScales[itemWork]);
        BoardModelMotionStart(itemModelIds[itemWork], 1, 0x40000001);
    }
    itemAnimationProcess = HuPrcCreate(ItemMinigameItemsCollect, 8196, 8192, 0);
    HuPrcDestructorSet2(itemAnimationProcess, ItemMinigameItemsCleanup);
    *((u32 *)&itemAnimationProcess->user_data) = 7;
    HuAudFXPlay(866);
    while (((u32)itemAnimationProcess->user_data & 0x40000000) == 0) {
        HuPrcVSleep();
    }
}

void ItemMinigameItemsDrop(u16 waitForAnimation)
{
    float distance;
    float duration;
    float angle;
    Vec playerPosition;

    s32 itemWork;
    s32 playerIndex;
    if (waitForAnimation) {
        ItemMinigameWaitForAnimation();
    }
    for (itemWork = 0; itemWork < 3; itemWork++) {
        BoardModelMotionStart(itemModelIds[itemWork], 3, 0);
    }
    distance = 0;
    angle = 20;
    playerPosition.x = -0.02f;
    playerPosition.y = -0.02f;
    playerPosition.z = -0.02f;
    for (playerIndex = 0; playerIndex < 3;) {
        duration = angle - ((7.0f / 60.0f) * (0.25f * (distance * distance)));
        distance++;
        playerIndex = 0;
        for (itemWork = 0; itemWork < 3; itemWork++) {
            itemPositions[itemWork].y += duration;
            BoardModelPosSetV(itemModelIds[itemWork], &itemPositions[itemWork]);
            if (duration < 0.0f) {
                VECAdd(&itemScales[itemWork], &playerPosition, &itemScales[itemWork]);
                if (itemScales[itemWork].x < 0.0f) {
                    itemScales[itemWork].x = 0.0f;
                }
                if (itemScales[itemWork].y < 0.0f) {
                    itemScales[itemWork].y = 0.0f;
                }
                if (itemScales[itemWork].z < 0.0f) {
                    itemScales[itemWork].z = 0.0f;
                }
                BoardModelScaleSetV(itemModelIds[itemWork], &itemScales[itemWork]);
                if (itemScales[itemWork].x == 0.0f) {
                    playerIndex++;
                }
            }
        }
        HuPrcVSleep();
    }
    for (itemWork = 0; itemWork < 3; itemWork++) {
        BoardModelVisibilitySet(itemModelIds[itemWork], 0);
    }
}

void ItemMinigameWaitForAnimation(void)
{
    u32 itemWork;
    itemWork = (u32)itemAnimationProcess->user_data;
    itemWork |= 0x20000000;
    *(u32 *)&itemAnimationProcess->user_data = itemWork;
    while (((u32)itemAnimationProcess->user_data & 0x80000000) == 0) {
        HuPrcVSleep();
    }
    HuPrcKill(itemAnimationProcess);
    itemAnimationProcess = NULL;
}

void ItemMinigameParticleHook(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

void ItemMinigameItemsCollect(void)
{
    float distance;
    float duration;
    float angle;
    float effectRiseHeight;

    Vec positionDelta[3];
    Vec position;
    float modelPosition[3];
    s32 itemWork;
    u32 playerIndex;
    u32 itemIndex;
    s32 itemModelId;
    HU3DPARTICLE *selection;
    playerIndex = (u32)HuPrcCurrentGet()->user_data;
    for (itemWork = 0; itemWork < 3; itemWork++) {
        itemIndex = 1 << itemWork;
        if (playerIndex & itemIndex) {
            BoardModelPosGet(itemModelIds[itemWork], &positionDelta[itemWork]);
            positionDelta[itemWork].y += 50.0f;
            modelPosition[itemWork] = 0.0f;
            BoardModelPosSetV(itemEffectModelIds[itemWork], &positionDelta[itemWork]);
            position.x = position.y = position.z = modelPosition[itemWork];
            BoardModelScaleSetV(itemEffectModelIds[itemWork], &position);
            BoardModelVisibilitySet(itemEffectModelIds[itemWork], 1);
            effectRiseHeight = 130.0f + positionDelta[itemWork].y;
            if (playerIndex & 0x10000000) {
                itemParticleIds[itemWork] = Hu3DParticleCreate(itemParticleAnimationData, 100);
                selection = Hu3DData[itemParticleIds[itemWork]].hookData;
                selection->emitCnt = itemEffectModelIds[itemWork];
                Hu3DParticleHookSet(itemParticleIds[itemWork], ItemMinigameParticleHook);
                Hu3DParticleColSet(itemParticleIds[itemWork], 255, 255, 0);
                Hu3DModelPosSet(itemParticleIds[itemWork], 0, 0, 0);
                Hu3DParticleBlendModeSet(itemParticleIds[itemWork], 1);
                Hu3DModelLayerSet(itemParticleIds[itemWork], 2);
            }
        }
    }
    distance = 12;
    angle = 0.05f;
    while (1) {
        itemModelId = 0;
        for (itemWork = 0; itemWork < 3; itemWork++) {
            itemIndex = 1 << itemWork;
            if (playerIndex & itemIndex) {
                itemModelId++;
                BoardModelPosGet(itemEffectModelIds[itemWork], &positionDelta[itemWork]);
                positionDelta[itemWork].y += distance;
                if (positionDelta[itemWork].y >= effectRiseHeight) {
                    positionDelta[itemWork].y = effectRiseHeight;
                    itemModelId--;
                }
                BoardModelPosSetV(itemEffectModelIds[itemWork], &positionDelta[itemWork]);
                modelPosition[itemWork] += angle;
                if (modelPosition[itemWork] > 1.0f) {
                    modelPosition[itemWork] = 1.0f;
                }
                position.x = position.y = position.z = modelPosition[itemWork];
                BoardModelScaleSetV(itemEffectModelIds[itemWork], &position);
            }
        }
        distance *= 0.92f;
        if (itemModelId == 0) {
            break;
        }
        HuPrcVSleep();
    }
    for (itemWork = 0; itemWork < 3; itemWork++) {
        itemIndex = 1 << itemWork;
        if (playerIndex & itemIndex) {
            modelPosition[itemWork] = 1.0f;
            position.x = position.y = position.z = modelPosition[itemWork];
            BoardModelScaleSetV(itemEffectModelIds[itemWork], &position);
        }
    }
    playerIndex |= 0x40000000;
    *((u32 *)&HuPrcCurrentGet()->user_data) = playerIndex;
    duration = 0;
    while (1) {

        playerIndex = (u32)HuPrcCurrentGet()->user_data;
        if (playerIndex & 0x20000000) {
            break;
        }
        for (itemWork = 0; itemWork < 3; itemWork++) {
            itemIndex = 1 << itemWork;
            if (playerIndex & itemIndex) {
                BoardModelPosGet(itemEffectModelIds[itemWork], &positionDelta[itemWork]);
                positionDelta[itemWork].y += sind(duration) * 0.25f;
                BoardModelPosSetV(itemEffectModelIds[itemWork], &positionDelta[itemWork]);
            }
        }
        duration += 5.0f;
        if (duration >= 360.0f) {
            duration -= 360.0f;
        }

        HuPrcVSleep();
    }
    distance = -12;
    angle = -0.05f;
    while (1) {
        itemModelId = 0;
        for (itemWork = 0; itemWork < 3; itemWork++) {
            itemIndex = 1 << itemWork;
            if (playerIndex & itemIndex) {
                itemModelId++;
                BoardModelPosGet(itemEffectModelIds[itemWork], &positionDelta[itemWork]);
                positionDelta[itemWork].y += distance;
                BoardModelPosSetV(itemEffectModelIds[itemWork], &positionDelta[itemWork]);
                modelPosition[itemWork] += angle;
                if (modelPosition[itemWork] < 0.0f) {
                    modelPosition[itemWork] = 0.0f;
                }
                if (modelPosition[itemWork] == 0.0f) {
                    itemModelId--;
                }
                position.x = position.y = position.z = modelPosition[itemWork];
                BoardModelScaleSetV(itemEffectModelIds[itemWork], &position);
            }
        }
        distance *= 0.92f;
        if (itemModelId == 0) {
            break;
        }
        HuPrcVSleep();
    }
    for (itemWork = 0; itemWork < 3; itemWork++) {
        itemIndex = 1 << itemWork;
        if (playerIndex & itemIndex) {
            position.x = position.y = position.z = 1.0f;
            BoardModelScaleSetV(itemEffectModelIds[itemWork], &position);
            BoardModelVisibilitySet(itemEffectModelIds[itemWork], 0);
        }
    }
    playerIndex |= 0x80000000;
    *((u32 *)&HuPrcCurrentGet()->user_data) = playerIndex;
    while (1) {
        HuPrcVSleep();
    }
}

void ItemMinigameItemsCleanup(void)
{
    s32 itemWork;
    u32 playerIndex = (u32)HuPrcCurrentGet()->user_data;
    u32 itemIndex;
    if (playerIndex & 0x10000000) {
        for (itemWork = 0; itemWork < 3; itemWork++) {
            itemIndex = 1 << itemWork;
            if (playerIndex & itemIndex) {
                Hu3DModelKill(itemParticleIds[itemWork]);
            }
        }
    }
}

void ItemMinigameParticleHook(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    Vec modelPosition;
    f32 duration;
    f32 angle;
    f32 distance;
    HU3DPARTICLEDATA *playerIndex;
    s32 i, j;

    if (particle->count == 0) {
        playerIndex = particle->data;
        for (i = 0; i < particle->maxCnt; i++, playerIndex++) {
            playerIndex->color.a = 0;
            playerIndex->scale = 0.0f;
        }
        particle->dataCnt = 0;
    }

    BoardModelScaleGet(particle->emitCnt, &modelPosition);
    distance = modelPosition.x;
    BoardModelPosGet(particle->emitCnt, &modelPosition);

    for (i = 0; i < 22; i++) {
        playerIndex = particle->data;
        for (j = 0; j < particle->maxCnt; j++, playerIndex++) {
            if (0.0f == playerIndex->scale) {
                break;
            }
        }
        if (j != particle->maxCnt) {
            duration = 360.0f * ((1.0f / 255.0f) * frand8());
            angle = distance * (20.0f + (60.0f * ((1.0f / 255.0f) * frand8())));
            playerIndex->pos.x = (modelPosition.x + (angle * sind(duration)));
            playerIndex->pos.z = (modelPosition.z + (angle * cosd(duration)));
            playerIndex->pos.y = (modelPosition.y + (distance * (-30.0f + (40.0f * ((1.0f / 255.0f) * frand8())))));
            playerIndex->vel.x = (0.5f + (3.0f * ((1.0f / 255.0f) * frand8())));
            playerIndex->vel.y = (0.3f + (2.0f * ((1.0f / 255.0f) * frand8())));
            playerIndex->color.a = 180;
            playerIndex->scale = (15.0f * distance);
        }
    }

    playerIndex = particle->data;

    for (i = 0; i < particle->maxCnt; i++, playerIndex++) {
        if (0.0f != playerIndex->scale) {
            playerIndex->pos.y = (playerIndex->pos.y - playerIndex->vel.x);
            playerIndex->scale = (playerIndex->scale - playerIndex->vel.y);
            if (playerIndex->scale <= 0.0f) {
                playerIndex->scale = 0.0f;
            }
        }
    }
}
