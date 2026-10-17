#include "game/audio.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/process.h"

#include "REL/m407dll.h"

#define MAX_WHOMPS_PLAYER 160
#define MAX_WHOMPS_ALL_PLAYERS MAX_WHOMPS_PLAYER *ARRAY_COUNT(GWPlayer)
#define MAX_WHOMPS (MAX_WHOMPS_PLAYER + 1) * ARRAY_COUNT(GWPlayer)

typedef struct WhompWork {
    /* 0x00 */ u32 whompIndex;
    /* 0x04 */ s32 shadowInitialized;
    /* 0x08 */ Vec position;
    /* 0x14 */ Vec rotation;
    /* 0x20 */ char padding[4];
    /* 0x24 */ s32 behaviorIndex;
    /* 0x28 */ s32 hasAttachedShadow;
    /* 0x2C */ s32 behaviorTimer;
} WhompWork; // sizeof 0x30

// function signatures
void WhompStateIdle(OMOBJ *);
void WhompStateFall(OMOBJ *);
void WhompStateImpact(OMOBJ *);
void WhompStateReset(OMOBJ *);
void WhompStateRecover(OMOBJ *);
void CreateWhomp(OMOBJ *);
void SetWhompBehaviorState(OMOBJ *, s32);
void DestroyWhomp(OMOBJ *);
void SpawnEffect(s32, s32, f32, f32, f32);
void ShowWhompScorePopup(s16, f32, f32);
void VibratePlayerController(s16, s16, s16);

// bss
HUPROCESS *whompProcess;
OMOBJ *whompObjects[MAX_WHOMPS];
s16 whompModelIds[MAX_WHOMPS][3];
s16 whompMotionIds[MAX_WHOMPS][6];
s16 whompCount;
s16 whompState;
s16 playerWhompCounts[21];
s16 whompScoreMode;

// data
s32 whompModelAssetIds[] = {
    DATA_MAKE_NUM(DATADIR_M407, 0x1C),
    DATA_MAKE_NUM(DATADIR_M407, 0x1D),
    DATA_MAKE_NUM(DATADIR_M407, 0x1E),
    DATA_MAKE_NUM(DATADIR_M407, 0x1B),
};

s32 whompMotionAssetIds[] = {
    DATA_MAKE_NUM(DATADIR_M407, 0x2E),
    DATA_MAKE_NUM(DATADIR_M407, 0x2F),
    DATA_MAKE_NUM(DATADIR_M407, 0x2D),
    DATA_MAKE_NUM(DATADIR_M407, 0x2A),
    DATA_MAKE_NUM(DATADIR_M407, 0x2C),
    DATA_MAKE_NUM(DATADIR_M407, 0x27),
    DATA_MAKE_NUM(DATADIR_M407, 0x28),
    DATA_MAKE_NUM(DATADIR_M407, 0x29),
};

ObjFuncs whompBehaviorCallbacks[] = {
    WhompStateIdle,
    WhompStateFall,
    WhompStateImpact,
    WhompStateReset,
    WhompStateRecover,
};

void InitializeWhompSystem(HUPROCESS *process)
{
    HUPROCESS *unusedProcessAlias;
    u32 i;
    u32 j;

    whompProcess = process;

    for (j = 0, i = 0; i < MAX_WHOMPS_ALL_PLAYERS; i++) {
        whompObjects[i] = omAddObjEx(whompProcess, 0x200, 3, 6, 2, CreateWhomp);
        whompObjects[i]->work[0] = i;
        whompObjects[i]->work[1] = 0;
    }

    j = j + i;

    for (i = 0; i < 4; i++) {
        whompObjects[j + i] = omAddObjEx(whompProcess, 0x200, 3, 6, 2, CreateWhomp);
        whompObjects[j + i]->work[0] = j + i;
        whompObjects[j + i]->work[1] = 1;
    }

    for (i = 0; i < 4; i++) {
        playerWhompCounts[i] = 0;
    }

    whompState = 0;
}

void ResetWhompSystem(void)
{
    u32 i;

    for (i = 0; i < MAX_WHOMPS; i++) {
        DestroyWhomp(whompObjects[i]);
        omDelObjEx(whompProcess, whompObjects[i]);
    }
}

void ClearWhompScores(void)
{
    u32 i;

    HuAudFXPlay(0x542);
    for (i = 0; i < MAX_WHOMPS; i++) {
        SetWhompBehaviorState(whompObjects[i], 1);
    }
}

void ResetWhompCounts(void)
{
    u32 i, j;

    whompScoreMode = 0;

    for (i = 0; i < 4; i++) {
        if (whompScoreMode < playerWhompCounts[i]) {
            whompScoreMode = playerWhompCounts[i];
        }
    }

    whompCount = 0;

    for (i = 0; i < 4; i++) {
        if (playerWhompCounts[i] > 0) {
            SetWhompBehaviorState(whompObjects[i], 3);
            whompCount += 1;
        }
    }
}

void IncrementPlayerWhompCount(u8 playerIndex)
{
    u32 i;
    OMOBJ *whompObject;

    i = playerWhompCounts[playerIndex];
    if (i < MAX_WHOMPS_PLAYER) {
        playerWhompCounts[playerIndex] = i + 1;
        whompObject = whompObjects[playerIndex + i * ARRAY_COUNT(GWPlayer)];
        SetWhompBehaviorState(whompObject, 2);
    }
}

#ifdef __MWERKS__
void SpawnPlayerWhomp(u8 playerIndex, s16 whompType)
#else
void SpawnPlayerWhomp(u8 playerIndex)
#endif
{
    OMOBJ *playerWhompObject;
    WhompWork *workData;

    playerWhompObject = whompObjects[playerIndex + MAX_WHOMPS_ALL_PLAYERS];
    workData = playerWhompObject->data;
    workData->position.x = 600.0f - (400.0f * playerIndex);
    workData->position.y = -300.0f;
    workData->position.z = 200.0f * (playerWhompCounts[playerIndex] + 2);
    SetWhompBehaviorState(playerWhompObject, 2);
}

s32 GetWhompScoreTotal(void)
{
    return (whompCount == 0) ? 1 : 0;
}

s16 GetPlayerWhompCount(u8 playerIndex)
{
    return playerWhompCounts[playerIndex];
}

s16 GetWhompScoreDisplayMode(void)
{
    return whompState;
}

s16 GetPlayerWhompModelId(u8 playerIndex)
{
    return whompObjects[playerIndex + MAX_WHOMPS_ALL_PLAYERS]->mdlId[0];
}

void SetWhompBehaviorState(OMOBJ *object, s32 behaviorState)
{
    WhompWork *workData;

    workData = object->data;
    workData->behaviorIndex = behaviorState;
    workData->behaviorTimer = 0;
}

void UpdateWhomp(OMOBJ *object)
{
    WhompWork *shadowWork;
    OMOBJ *shadowObject;
    u32 i;
    WhompWork *workData;

    workData = object->data;
    whompBehaviorCallbacks[workData->behaviorIndex](object);
    if ((workData->position.z > (Center.z - 2000.0f)) && (workData->position.z < (2000.0f + Center.z)) && (workData->shadowInitialized == 0)) {
        for (i = 0; i < 8; i++) {
            shadowObject = whompObjects[(workData->whompIndex % 80) + (i * 80)];
            shadowObject->mdlId[0] = -1;
            shadowWork = shadowObject->data;
            shadowWork->shadowInitialized = 0;
        }
        object->mdlId[0] = whompModelIds[((workData->whompIndex % 80))][0];

        if (workData->position.y < 0.0f) {
            Hu3DModelShadowDispOff(object->mdlId[0]);
        }
        else {
            Hu3DModelShadowDispOn(object->mdlId[0]);
        }

        workData->shadowInitialized = 1;
    }

    if ((workData->position.y < 0.0f) || (workData->position.z < (Center.z - 2000.0f))) {
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    }
    else {
        Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    }

    omSetTra(object, workData->position.x, workData->position.y, workData->position.z);
    omSetRot(object, workData->rotation.x, workData->rotation.y, workData->rotation.z);

    if (workData->hasAttachedShadow == 1) {
        Hu3DModelPosSet(object->mdlId[2], workData->position.x, 1.0f + workData->position.y, workData->position.z);
    }
}

void CreateWhomp(OMOBJ *object)
{
    u32 i;
    WhompWork *whompWork;

    object->objFunc = UpdateWhomp;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(WhompWork), HU_MEMNUM_OVL);
    whompWork = object->data;

    for (i = 0; i < 3; i++) {
        object->mdlId[i] = -1;
    }

    for (i = 0; i < 6; i++) {
        object->mtnId[i] = -1;
    }

    whompWork->whompIndex = (u32)object->work[0];
    whompWork->hasAttachedShadow = (u32)object->work[1];
    whompWork->shadowInitialized = 0;
    whompWork->behaviorTimer = 0;
    switch (whompWork->hasAttachedShadow) {
        case 0:
            whompWork->rotation.x = 0.0f;
            whompWork->rotation.y = 180.0f;
            whompWork->rotation.z = 0.0f;
            if (whompWork->whompIndex < 80) {
                object->mdlId[0] = Hu3DModelCreateFile(whompModelAssetIds[1]);
                Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_NOCULL);
                Hu3DModelAmbSet(object->mdlId[0], 1.0f, 1.0f, 1.0f);
                if (whompWork->whompIndex == 0) {
                    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[0], whompMotionAssetIds[5]);
                    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[0], whompMotionAssetIds[7]);
                    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[0], whompMotionAssetIds[6]);
                }
                else {
                    object->mtnId[5] = whompMotionIds[0][5];
                    object->mtnId[3] = whompMotionIds[0][3];
                    object->mtnId[4] = whompMotionIds[0][4];
                }
                Hu3DModelShadowSet(object->mdlId[0]);
                for (i = 0; i < 3; i++) {
                    whompModelIds[whompWork->whompIndex][i] = object->mdlId[i];
                }

                for (i = 0; i < 6; i++) {
                    whompMotionIds[whompWork->whompIndex][i] = object->mtnId[i];
                }
                Hu3DMotionSet(object->mdlId[0], object->mtnId[5]);
                Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
                whompWork->shadowInitialized = 1;
            }
            else {
                whompWork->shadowInitialized = 0;
            }
            break;
        case 1:
            whompWork->rotation.x = 0.0f;
            whompWork->rotation.y = 0.0f;
            whompWork->rotation.z = 0.0f;
            object->mdlId[0] = Hu3DModelCreateFile(whompModelAssetIds[0]);
            object->mdlId[1] = Hu3DModelCreateFile(whompModelAssetIds[2]);
            object->mdlId[2] = Hu3DModelCreateFile(whompModelAssetIds[3]);
            Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_NOCULL);
            Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_NOCULL);
            Hu3DModelAmbSet(object->mdlId[0], 1.0f, 1.0f, 1.0f);
            Hu3DModelAmbSet(object->mdlId[1], 1.0f, 1.0f, 1.0f);
            object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[0], whompMotionAssetIds[2]);
            object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[0], whompMotionAssetIds[4]);
            object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[0], whompMotionAssetIds[3]);
            object->mtnId[0] = Hu3DJointMotionFile(object->mdlId[1], whompMotionAssetIds[0]);
            object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], whompMotionAssetIds[1]);
            Hu3DModelHookSet(object->mdlId[0], "itemhook_r", object->mdlId[1]);
            Hu3DModelShadowSet(object->mdlId[0]);
            Hu3DModelShadowSet(object->mdlId[1]);
            Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(object->mdlId[2], HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrSet(object->mdlId[2], HU3D_MOTATTR_LOOP);
            whompWork->shadowInitialized = 1;
            break;
    }
    whompWork->position.x = (600.0f - (400.0f * (whompWork->whompIndex & 3)));
    whompWork->position.y = 0.0f;
    whompWork->position.z = (200.0f * (whompWork->whompIndex >> 2));
    SetWhompBehaviorState(object, 0);
}

void DestroyWhomp(OMOBJ *object)
{
    s32 i;
    WhompWork *whompWork;

    whompWork = object->data;
    switch (whompWork->hasAttachedShadow) { /* irregular */
        case 0:
            if (whompWork->whompIndex == 0) {
                for (i = 0; i < 6; i++) {
                    if (object->mtnId[i] != -1) {
                        Hu3DMotionKill(object->mtnId[i]);
                    }
                }
            }
            if (whompWork->shadowInitialized != 0) {
                for (i = 0; i < 3; i++) {
                    if (object->mdlId[i] != -1) {
                        Hu3DModelKill(object->mdlId[i]);
                    }
                }
            }
            break;
        case 1:
            for (i = 0; i < 6; i++) {
                if (object->mtnId[i] != -1) {
                    Hu3DMotionKill(object->mtnId[i]);
                }
            }
            for (i = 0; i < 3; i++) {
                if (object->mdlId[i] != -1) {
                    Hu3DModelKill(object->mdlId[i]);
                }
            }
            break;
    }
    if (object->data) {
        HuMemDirectFree(object->data);
    }
}

void WhompStateIdle(OMOBJ *) { }

void WhompStateFall(OMOBJ *object)
{
    WhompWork *workData;

    workData = object->data;
    if (workData->behaviorTimer == 0) {
        Hu3DMotionSet(object->mdlId[0], object->mtnId[4]);
        Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
        switch (workData->hasAttachedShadow) {
            case 0:
                SpawnEffect(0, 0, workData->position.x, 0.0f, workData->position.z);
                break;
            case 1:
                SpawnEffect(1, 0, workData->position.x, 0.0f, workData->position.z);
                break;
        }
    }

    if (workData->behaviorTimer == 0x2A) {
        Hu3DModelShadowDispOff(object->mdlId[0]);
    }

    if (Hu3DMotionEndCheck(*object->mdlId) == 0) {
        workData->behaviorTimer++;
        return;
    }

    workData->position.y = -300.0f;
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    SetWhompBehaviorState(object, 0);
}

void WhompStateImpact(OMOBJ *object)
{
    WhompWork *whompWork;

    whompWork = object->data;
    if (whompWork->behaviorTimer == 0) {
        if (whompWork->hasAttachedShadow == 1) {
            Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
        }
        Hu3DMotionSet(object->mdlId[0], object->mtnId[3]);
        Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
        switch (whompWork->hasAttachedShadow) { /* irregular */
            case 0:
                SpawnEffect(0, 0, whompWork->position.x, 0.0f, whompWork->position.z);
                HuAudFXPlay(0x543);
                break;
            case 1:
                HuAudFXPlay(0x544);
                SpawnEffect(1, 0, whompWork->position.x, 0.0f, whompWork->position.z);
                break;
        }
        whompWork->position.y = 0.0f;
    }

    if (whompWork->behaviorTimer == 0xA) {
        Hu3DModelShadowDispOn(object->mdlId[0]);
    }

    if ((whompWork->behaviorTimer == 0xA) && (whompWork->hasAttachedShadow == 1)) {
        Hu3DMotionSet(object->mdlId[1], object->mtnId[1]);
        Hu3DModelAttrReset(object->mdlId[1], HU3D_MOTATTR_LOOP);
    }

    whompWork->behaviorTimer++;

    if (Hu3DMotionEndCheck(object->mdlId[0]) != 0) {
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[5], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
        switch (whompWork->hasAttachedShadow) {
            case 0:
                SetWhompBehaviorState(object, 0);
                return;
            case 1:
                SetWhompBehaviorState(object, 4);
                break;
        }
    }
}

void WhompStateRecover(OMOBJ *object)
{
    WhompWork *whompWork;

    whompWork = object->data;
    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[2], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    Hu3DMotionSet(object->mdlId[1], object->mtnId[0]);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[2], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_MOTATTR_LOOP);
    SetWhompBehaviorState(object, 0);
}

void WhompStateReset(OMOBJ *object)
{
    WhompWork *workData;

    workData = object->data;
    if (workData->behaviorTimer == 0) {
        workData->rotation.x = 0.0f;
        workData->position.y = 0.0f;
        Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
        workData->behaviorTimer++;
    }
    workData->rotation.x -= 4.0f;
    workData->position.y += 0.9f;
    if ((workData->rotation.x <= -25.0f) && (workData->behaviorTimer == 1)) {
        if ((workData->whompIndex < 640) && ((workData->whompIndex >> 2) < (playerWhompCounts[workData->whompIndex & 3] - 1))) {
            SetWhompBehaviorState(whompObjects[workData->whompIndex + 4], 3);
            workData->behaviorTimer++;
        }
        if (whompState < (workData->whompIndex >> 2) + 1) {
            whompState = (workData->whompIndex >> 2) + 1;
        }
    }
    if ((workData->behaviorTimer == 1) && (workData->rotation.x > -90.0f) || (workData->behaviorTimer == 2) && (workData->rotation.x > -68.0f)) {
        return;
    }

    workData->rotation.x = (workData->behaviorTimer == 1) ? -90.0f : -68.0f;

    if (workData->behaviorTimer == 1) {
        SpawnEffect(3, 0, workData->position.x, 0.0f, 200.0f + workData->position.z);
        HuAudFXPlay(0x544);
        if (whompScoreMode > (workData->whompIndex >> 2) + 1) {
            ShowWhompScorePopup((workData->whompIndex >> 2) + 1, workData->position.x, workData->position.z);
        }
        whompCount -= 1;
    }
    else {
        SpawnEffect(2, 0, workData->position.x, 0.0f, 200.0f + workData->position.z);
        HuAudFXPlay(0x543);
    }
    VibratePlayerController(workData->whompIndex & 3, 2, 4);
    SetWhompBehaviorState(object, 0);
}
