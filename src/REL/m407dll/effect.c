#include "game/hu3d.h"
#include "game/object.h"
#include "game/process.h"
#include "math.h"

#include "REL/m407dll.h"

typedef struct EffectObjectWork {
    /* 0x00 */ u32 effectPoolIndex;
    /* 0x04 */ u32 framesRemaining;
    /* 0x08 */ s32 effectType;
    /* 0x0C */ s32 effectState;
    /* 0x10 */ f32 positionX;
    /* 0x14 */ f32 positionY;
    /* 0x18 */ f32 positionZ;
    /* 0x1C */ s32 animationState;
} EffectObjectWork; // size of 0x20

// function signatures
void EffectStateIdle(OMOBJ *object);
void EffectStatePlay(OMOBJ *object);
void CreateEffectObject(OMOBJ *object);
void SetEffectState(OMOBJ *object, s32 effectState);
void DestroyEffectObject(OMOBJ *object);

// bss
HUPROCESS *effectProcess;
OMOBJ *effectObjects[100];

// data
s32 effectModelAssetIds[] = { DATA_MAKE_NUM(DATADIR_M407, 0x15), DATA_MAKE_NUM(DATADIR_M407, 0x16), DATA_MAKE_NUM(DATADIR_M407, 0x17),
    DATA_MAKE_NUM(DATADIR_M407, 0x18), DATA_MAKE_NUM(DATADIR_M407, 0x19) };

ObjFuncs effectStateCallbacks[] = {
    EffectStateIdle,
    EffectStatePlay,
};

void InitializeEffectSystem(HUPROCESS *process)
{
    HUPROCESS *unusedProcessAlias;
    u32 i;
    u32 effectObjectIndexOffset;

    effectProcess = process;
    effectObjectIndexOffset = 0;

    for (i = 0; i < 40; i++) {
        effectObjects[i] = omAddObjEx(effectProcess, 0x800, 1, 0, 5, CreateEffectObject);
        effectObjects[i]->work[0] = i;
        effectObjects[i]->work[1] = 0;
    }

    effectObjectIndexOffset += i;

    for (i = 0; i < 4; i++) {
        effectObjects[effectObjectIndexOffset + i] = omAddObjEx(effectProcess, 0x800, 1, 0, 5, CreateEffectObject);
        effectObjects[effectObjectIndexOffset + i]->work[0] = effectObjectIndexOffset + i;
        effectObjects[effectObjectIndexOffset + i]->work[1] = 1;
    }

    effectObjectIndexOffset += i;

    for (i = 0; i < 30; i++) {
        effectObjects[effectObjectIndexOffset + i] = omAddObjEx(effectProcess, 0x800, 1, 0, 5, CreateEffectObject);
        effectObjects[effectObjectIndexOffset + i]->work[0] = effectObjectIndexOffset + i;
        effectObjects[effectObjectIndexOffset + i]->work[1] = 2;
    }

    effectObjectIndexOffset += i;

    for (i = 0; i < 4; i++) {
        effectObjects[effectObjectIndexOffset + i] = omAddObjEx(effectProcess, 0x800, 1, 0, 5, CreateEffectObject);
        effectObjects[effectObjectIndexOffset + i]->work[0] = effectObjectIndexOffset + i;
        effectObjects[effectObjectIndexOffset + i]->work[1] = 3;
    }

    effectObjectIndexOffset += i;

    for (i = 0; i < 22; i++) {
        effectObjects[effectObjectIndexOffset + i] = omAddObjEx(effectProcess, 0x800, 1, 0, 5, CreateEffectObject);
        effectObjects[effectObjectIndexOffset + i]->work[0] = effectObjectIndexOffset + i;
        effectObjects[effectObjectIndexOffset + i]->work[1] = 4;
    }
}

void ResetEffectPool(void)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(effectObjects); i++) {
        DestroyEffectObject(effectObjects[i]);
    }
}

void SpawnEffect(s32 effectType, s32 durationFrames, f32 positionX, f32 positionY, f32 positionZ)
{
    OMOBJ *availableEffectObject;
    EffectObjectWork *workData;
    u32 i;

    if ((positionZ < (Center.z - 2000.0f)) || (positionZ > (2000.0f + Center.z))) {
        return;
    }

    for (i = 0; i < ARRAY_COUNT(effectObjects); i++) {
        availableEffectObject = effectObjects[i];
        workData = availableEffectObject->data;
        if ((workData->effectType == effectType) && (workData->effectState == 0)) {
            workData->framesRemaining = durationFrames;
            workData->positionX = positionX;
            workData->positionY = positionY;
            workData->positionZ = positionZ;
            SetEffectState(availableEffectObject, 1);
            return;
        }
    }
}

void SetEffectState(OMOBJ *object, s32 effectState)
{
    EffectObjectWork *workData;

    workData = object->data;
    workData->effectState = effectState;
    workData->animationState = 0;
}

void UpdateEffectObject(OMOBJ *object)
{
    EffectObjectWork *workData;

    workData = object->data;
    effectStateCallbacks[workData->effectState](object);
    omSetTra(object, workData->positionX, workData->positionY, workData->positionZ);
}

void CreateEffectObject(OMOBJ *object)
{
    EffectObjectWork *effectWork;

    object->objFunc = UpdateEffectObject;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(EffectObjectWork), HU_MEMNUM_OVL);
    effectWork = object->data;
    effectWork->effectPoolIndex = object->work[0];
    effectWork->effectType = object->work[1];
    effectWork->animationState = 0;
    object->mdlId[0] = Hu3DModelCreateFile(effectModelAssetIds[effectWork->effectType]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    effectWork->positionX = 0.0f;
    effectWork->positionY = 0.0f;
    effectWork->positionZ = 0.0f;
    SetEffectState(object, 0);
}

void DestroyEffectObject(OMOBJ *object)
{
    EffectObjectWork *effectWork;
    effectWork = object->data;

    Hu3DModelKill(object->mdlId[0]);

    if (object->data) {
        HuMemDirectFree(object->data);
    }
}

void EffectStateIdle(OMOBJ *object) { }

void EffectStatePlay(OMOBJ *object)
{
    EffectObjectWork *effectWork;

    effectWork = object->data;
    if (effectWork->framesRemaining != 0) {
        effectWork->framesRemaining--;
    }

    if (effectWork->animationState == 0) {
        Hu3DMotionTimeSet(*object->mdlId, 0.0f);
        Hu3DModelAttrReset(*object->mdlId, HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(*object->mdlId, HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrSet(*object->mdlId, HU3D_MOTATTR_LOOP);
        effectWork->animationState++;
    }
    else if (effectWork->animationState == 1) {
        Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
        effectWork->animationState++;
    }

    if (Hu3DMotionEndCheck(*object->mdlId) != 0) {
        Hu3DModelAttrSet(*object->mdlId, HU3D_ATTR_DISPOFF);
        SetEffectState(object, 0);
    }
}
