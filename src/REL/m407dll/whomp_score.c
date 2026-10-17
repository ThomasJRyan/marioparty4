#include "game/esprite.h"
#include "game/gamework_data.h"
#include "game/object.h"
#include "game/process.h"
#include "game/sprite.h"

#include "REL/m407dll.h"

typedef struct WhompScorePopupWork {
    /* 0x00 */ u32 playerIndex;
    /* 0x04 */ s32 animationState;
    /* 0x08 */ Vec screenPosition;
    /* 0x14 */ s32 frameCounter;
    /* 0x18 */ s16 scoreSpriteIds[4];
    /* 0x20 */ f32 spriteOpacity;
} WhompScorePopupWork; // sizeof 0x24

// function signatures
void Hu3D3Dto2D(Vec *worldPosition, s16 cameraId, Vec *screenPosition);
void SetWhompScoreAnimationState(OMOBJ *object, s32 animationState);
void CreateWhompScorePopup(OMOBJ *);
void DestroyWhompScorePopup(OMOBJ *object);
void WhompScoreStateIdle(OMOBJ *);
void WhompScoreStateRise(OMOBJ *object);
void WhompScoreStateFade(OMOBJ *object);

// bss
HUPROCESS *whompScoreProcess;
OMOBJ *whompScorePopupObjects[4];

// data
ObjFuncs whompScoreAnimationCallbacks[] = {
    WhompScoreStateIdle,
    WhompScoreStateRise,
    WhompScoreStateFade,
};

void InitializeWhompScoreSystem(HUPROCESS *process)
{
    u32 i;

    whompScoreProcess = process;

    for (i = 0; i < ARRAY_COUNT(whompScorePopupObjects); i++) {
        whompScorePopupObjects[i] = omAddObjEx(whompScoreProcess, 0x500, 0, 0, 4, CreateWhompScorePopup);
        whompScorePopupObjects[i]->work[0] = i;
    }
}

void ResetWhompScoreDisplay(void)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(whompScorePopupObjects); i++) {
        DestroyWhompScorePopup(whompScorePopupObjects[i]);
    }
}

void ShowWhompScorePopup(s16 score, f32 positionX, f32 positionZ)
{
    OMOBJ *popupObject;
    Vec scoreWorldPosition;
    WhompScorePopupWork *workData;
    u32 i;
    s16 scoreDigits;

    for (i = 0; i < ARRAY_COUNT(whompScorePopupObjects); i++) {
        popupObject = whompScorePopupObjects[i];
        workData = popupObject->data;
        if (workData->animationState == 0) {
            scoreDigits = score;
            espBankSet(workData->scoreSpriteIds[1], (scoreDigits / 100));
            scoreDigits = (scoreDigits - ((scoreDigits / 100) * 100));
            espBankSet(workData->scoreSpriteIds[2], (scoreDigits / 10));
            scoreDigits = (scoreDigits - ((scoreDigits / 10) * 10));
            espBankSet(workData->scoreSpriteIds[3], scoreDigits);
            scoreWorldPosition.x = positionX;
            scoreWorldPosition.y = 0.0f;
            scoreWorldPosition.z = positionZ;
            Hu3D3Dto2D(&scoreWorldPosition, 1, &workData->screenPosition);
            SetWhompScoreAnimationState(popupObject, 1);
            return;
        }
    }
}

void SetWhompScoreAnimationState(OMOBJ *object, s32 animationState)
{
    WhompScorePopupWork *workData;

    workData = object->data;
    workData->animationState = animationState;
    workData->frameCounter = 0;
}

void UpdateWhompScorePopup(OMOBJ *object)
{
    s32 loopCount;
    WhompScorePopupWork *workData;

    workData = object->data;
    whompScoreAnimationCallbacks[workData->animationState](object);

    espPosSet(workData->scoreSpriteIds[0], workData->screenPosition.x, workData->screenPosition.y);
    espPosSet(workData->scoreSpriteIds[1], workData->screenPosition.x - 18.0f, workData->screenPosition.y - 8.0f);
    espPosSet(workData->scoreSpriteIds[2], workData->screenPosition.x, workData->screenPosition.y - 8.0f);
    espPosSet(workData->scoreSpriteIds[3], 18.0f + workData->screenPosition.x, workData->screenPosition.y - 8.0f);
}

void CreateWhompScorePopup(OMOBJ *object)
{
    WhompScorePopupWork *workData;

    object->objFunc = UpdateWhompScorePopup;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(WhompScorePopupWork), HU_MEMNUM_OVL);
    workData = object->data;
    workData->playerIndex = object->work[0];
    workData->frameCounter = 0;
    workData->animationState = 0;
    workData->scoreSpriteIds[0] = espEntry(DATA_MAKE_NUM(DATADIR_M407, 0x26), 1, 0);
    workData->scoreSpriteIds[1] = espEntry(DATA_MAKE_NUM(DATADIR_M407, 0x20), 0, 0);
    workData->scoreSpriteIds[2] = espEntry(DATA_MAKE_NUM(DATADIR_M407, 0x20), 0, 0);
    workData->scoreSpriteIds[3] = espEntry(DATA_MAKE_NUM(DATADIR_M407, 0x20), 0, 0);
    espDispOff(workData->scoreSpriteIds[0]);
    espDispOff(workData->scoreSpriteIds[1]);
    espDispOff(workData->scoreSpriteIds[2]);
    espDispOff(workData->scoreSpriteIds[3]);
    espScaleSet(workData->scoreSpriteIds[0], 1.8f, 1.8f);
    workData->screenPosition.x = (f32)0.0f;
    workData->screenPosition.y = (f32)0.0f;
    SetWhompScoreAnimationState(object, 0);
}

void DestroyWhompScorePopup(OMOBJ *object)
{
    s32 i;
    WhompScorePopupWork *popupWork;

    popupWork = object->data;

    for (i = 0; i < ARRAY_COUNT(popupWork->scoreSpriteIds); i++) {
        espKill(popupWork->scoreSpriteIds[i]);
    }

    if (object->data) {
        HuMemDirectFree(object->data);
    }
}

void WhompScoreStateIdle(OMOBJ *object) { }

void WhompScoreStateRise(OMOBJ *object)
{
    WhompScorePopupWork *workData;

    workData = object->data;
    if (workData->frameCounter == 0) {
        espDispOn(workData->scoreSpriteIds[0]);
        espDispOn(workData->scoreSpriteIds[1]);
        espDispOn(workData->scoreSpriteIds[2]);
        espDispOn(workData->scoreSpriteIds[3]);
        workData->spriteOpacity = 0.0f;
    }
    if (workData->spriteOpacity < 1.0f) {
        espTPLvlSet(workData->scoreSpriteIds[0], workData->spriteOpacity);
        espTPLvlSet(workData->scoreSpriteIds[1], workData->spriteOpacity);
        espTPLvlSet(workData->scoreSpriteIds[2], workData->spriteOpacity);
        espTPLvlSet(workData->scoreSpriteIds[3], workData->spriteOpacity);
        workData->spriteOpacity += 0.1f;
    }

    if (workData->frameCounter++ >= 60) {
        SetWhompScoreAnimationState(object, 2);
    }
}

void WhompScoreStateFade(OMOBJ *object)
{
    WhompScorePopupWork *workData;

    workData = object->data;

    if (workData->spriteOpacity > 0.0f) {
        espTPLvlSet(workData->scoreSpriteIds[0], workData->spriteOpacity);
        espTPLvlSet(workData->scoreSpriteIds[1], workData->spriteOpacity);
        espTPLvlSet(workData->scoreSpriteIds[2], workData->spriteOpacity);
        espTPLvlSet(workData->scoreSpriteIds[3], workData->spriteOpacity);
        workData->spriteOpacity -= 0.05;
        return;
    }

    espDispOff(workData->scoreSpriteIds[0]);
    espDispOff(workData->scoreSpriteIds[1]);
    espDispOff(workData->scoreSpriteIds[2]);
    espDispOff(workData->scoreSpriteIds[3]);
    SetWhompScoreAnimationState(object, 0);
}
