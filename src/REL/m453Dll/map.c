#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/flag.h"
#include "game/frand.h"
#include "game/gamework.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include "math.h"
#include "string.h"

#include "REL/m453Dll.h"
#include "version.h"

typedef struct M453MapWork {
    struct {
        u8 introFinished : 1;
    };
    s32 mapState;
    Float mapRotation;
    s32 pieceOrder[6];
    s32 mapModelIds[0xC];
} M453MapWork; /* size = 0x54 */

void MapLoadInitialize(OMOBJ *object);
void MapLoadComplete(OMOBJ *object);
void MapStateInitialize(OMOBJ *object);
void MapStateUpdate(OMOBJ *object);
void MapObjectUpdate(OMOBJ *object);
void MapObjectEffectStart(OMOBJ *object);
void MapObjectEffectReset(OMOBJ *object);
void MapObjectEffectUpdate(OMOBJ *object);
void MapObjectsDestroy(void);
void MapModelUnload(s16 modelId);

Vec mapLayout1OuterSpaceOffsets[0x10] = {
    { 92.39f, 0.0f, 38.27f },
    { 100.0f, 0.0f, 0.0f },
    { 92.39f, 0.0f, -38.27f },
    { 70.71f, 0.0f, -70.71f },
    { 38.27f, 0.0f, -92.39f },
    { 0.0f, 0.0f, -100.0f },
    { -38.27f, 0.0f, -92.39f },
    { -70.71f, 0.0f, -70.71f },
    { -92.39f, 0.0f, -38.27f },
    { -100.0f, 0.0f, 0.0f },
    { -92.39f, 0.0f, 38.27f },
    { -70.71f, 0.0f, 70.71f },
    { -38.27f, 0.0f, 92.39f },
    { 0.0f, 0.0f, 100.0f },
    { 38.27f, 0.0f, 92.39f },
    { 70.71f, 0.0f, 70.71f },
};

Vec mapLayout1InnerSpaceOffsets[0x10] = {
    { 55.43f, 0.0f, 22.96f },
    { 60.0f, 0.0f, 0.0f },
    { 55.43f, 0.0f, -22.96f },
    { 42.43f, 0.0f, -42.43f },
    { 22.96f, 0.0f, -55.43f },
    { 0.0f, 0.0f, -60.0f },
    { -22.96f, 0.0f, -55.43f },
    { -42.43f, 0.0f, -42.43f },
    { -55.43f, 0.0f, -22.96f },
    { -60.0f, 0.0f, 0.0f },
    { -55.43f, 0.0f, 22.96f },
    { -42.43f, 0.0f, 42.43f },
    { -22.96f, 0.0f, 55.43f },
    { 0.0f, 0.0f, 60.0f },
    { 22.96f, 0.0f, 55.43f },
    { 42.43f, 0.0f, 42.43f },
};

Vec mapLayout3OuterSpaceOffsets[0xA] = {
    { 38.79f, 0.0f, -53.4f },
    { 0.0f, 0.0f, -120.0f },
    { -38.79f, 0.0f, -53.4f },
    { -114.13f, 0.0f, -37.08f },
    { -62.77f, 0.0f, 20.4f },
    { -70.53f, 0.0f, 97.08f },
    { 0.0f, 0.0f, 66.0f },
    { 70.53f, 0.0f, 97.08f },
    { 62.77f, 0.0f, 20.4f },
    { 114.13f, 0.0f, -37.08f },
};

Vec mapLayout3InnerSpaceOffsets[0xA] = {
    { 21.34f, 0.0f, -29.37f },
    { 0.0f, 0.0f, -66.0f },
    { -21.34f, 0.0f, -29.37f },
    { -62.77f, 0.0f, -20.4f },
    { -34.52f, 0.0f, 11.22f },
    { -38.79f, 0.0f, 53.4f },
    { 0.0f, 0.0f, 36.3f },
    { 38.79f, 0.0f, 53.4f },
    { 34.52f, 0.0f, 11.22f },
    { 62.77f, 0.0f, -20.4f },
};

Vec mapLayout0OuterSpaceOffsets[0x10] = {
    { 110.87f, 0.0f, 45.92f },
    { 120.0f, 0.0f, 0.0f },
    { 110.87f, 0.0f, -45.92f },
    { 84.85f, 0.0f, -84.85f },
    { 45.92f, 0.0f, -110.87f },
    { 0.0f, 0.0f, -120.0f },
    { -45.92f, 0.0f, -110.87f },
    { -84.85f, 0.0f, -84.85f },
    { -110.87f, 0.0f, -45.92f },
    { -120.0f, 0.0f, 0.0f },
    { -110.87f, 0.0f, 45.92f },
    { -84.85f, 0.0f, 84.85f },
    { -45.92f, 0.0f, 110.87f },
    { 0.0f, 0.0f, 120.0f },
    { 45.92f, 0.0f, 110.87f },
    { 84.85f, 0.0f, 84.85f },
};

Vec mapLayout0InnerSpaceOffsets[0x10] = {
    { 73.17f, 0.0f, 30.31f },
    { 79.2f, 0.0f, 0.0f },
    { 73.17f, 0.0f, -30.31f },
    { 56.0f, 0.0f, -56.0f },
    { 30.31f, 0.0f, -73.17f },
    { 0.0f, 0.0f, -79.2f },
    { -30.31f, 0.0f, -73.17f },
    { -56.0f, 0.0f, -56.0f },
    { -73.17f, 0.0f, -30.31f },
    { -79.2f, 0.0f, 0.0f },
    { -73.17f, 0.0f, 30.31f },
    { -56.0f, 0.0f, 56.0f },
    { -30.31f, 0.0f, 73.17f },
    { 0.0f, 0.0f, 79.2f },
    { 30.31f, 0.0f, 73.17f },
    { 56.0f, 0.0f, 56.0f },
};

Vec mapLayout2OuterSpaceOffsets[0xA] = {
    { 48.49f, 0.0f, -66.74f },
    { 0.0f, 0.0f, -150.0f },
    { -48.49f, 0.0f, -66.74f },
    { -142.66f, 0.0f, -46.35f },
    { -78.46f, 0.0f, 25.49f },
    { -88.17f, 0.0f, 121.35f },
    { 0.0f, 0.0f, 82.5f },
    { 88.17f, 0.0f, 121.35f },
    { 78.46f, 0.0f, 25.49f },
    { 142.66f, 0.0f, -46.35f },
};

Vec mapLayout2InnerSpaceOffsets[0xA] = {
    { 29.09f, 0.0f, -40.05f },
    { 0.0f, 0.0f, -90.0f },
    { -29.09f, 0.0f, -40.05f },
    { -85.59f, 0.0f, -27.81f },
    { -47.08f, 0.0f, 15.3f },
    { -52.9f, 0.0f, 72.81f },
    { 0.0f, 0.0f, 49.5f },
    { 52.9f, 0.0f, 72.81f },
    { 47.08f, 0.0f, 15.3f },
    { 85.59f, 0.0f, -27.81f },
};

Vec mapLayout4OuterSpaceOffsets[0x16] = {
    { 129.1f, 0.0f, 54.6f },
    { 140.0f, 0.0f, 0.0f },
    { 129.3f, 0.0f, -53.6f },
    { 99.0f, 0.0f, -99.0f },
    { 53.6f, 0.0f, -129.3f },
    { 0.0f, 0.0f, -140.0f },
    { -53.6f, 0.0f, -129.3f },
    { -99.0f, 0.0f, -99.0f },
    { -129.4f, 0.0f, -53.5f },
    { -140.0f, 0.0f, 0.0f },
    { -136.0f, 0.0f, 20.4f },
    { -102.1f, 0.0f, -7.6f },
    { -70.0f, 0.0f, -14.0f },
    { -37.8f, 0.0f, -7.6f },
    { -10.6f, 0.0f, 10.6f },
    { 7.6f, 0.0f, 37.8f },
    { 14.0f, 0.0f, 70.0f },
    { 7.6f, 0.0f, 102.2f },
    { -20.4f, 0.0f, 136.0f },
    { 0.1f, 0.0f, 140.1f },
    { 53.9f, 0.0f, 129.5f },
    { 99.5f, 0.0f, 99.0f },
};

Vec mapLayout4InnerSpaceOffsets[0x16] = {
    { 95.9f, 0.0f, 42.9f },
    { 106.6f, 0.0f, 0.4f },
    { 98.4f, 0.0f, -37.2f },
    { 81.7f, 0.0f, -65.9f },
    { 55.4f, 0.0f, -89.6f },
    { 11.5f, 0.0f, -101.7f },
    { -24.5f, 0.0f, -99.9f },
    { -60.9f, 0.0f, -88.6f },
    { -98.7f, 0.0f, -64.4f },
    { -110.4f, 0.0f, -41.5f },
    { -110.4f, 0.0f, -30.8f },
    { -90.4f, 0.0f, -42.3f },
    { -53.8f, 0.0f, -48.5f },
    { -14.6f, 0.0f, -40.5f },
    { 17.1f, 0.0f, -16.3f },
    { 35.8f, 0.0f, 28.2f },
    { 35.8f, 0.0f, 65.6f },
    { 24.7f, 0.0f, 101.6f },
    { 19.6f, 0.0f, 114.6f },
    { 34.9f, 0.0f, 114.5f },
    { 62.9f, 0.0f, 99.0f },
    { 83.0f, 0.0f, 73.2f },
};

Vec mapLayout5OuterSpaceOffsets[0x16] = {
    { 110.9f, 0.0f, 45.9f },
    { 120.0f, 0.0f, 0.0f },
    { 110.9f, 0.0f, -45.8f },
    { 84.8f, 0.0f, -84.9f },
    { 45.9f, 0.0f, -110.9f },
    { 0.0f, 0.0f, -120.0f },
    { -45.9f, 0.0f, -110.9f },
    { -84.9f, 0.0f, -84.8f },
    { -110.9f, 0.0f, -45.9f },
    { -120.0f, 0.0f, 0.0f },
    { -116.5f, 0.0f, 17.5f },
    { -87.6f, 0.0f, -6.6f },
    { -60.0f, 0.0f, -12.0f },
    { -32.4f, 0.0f, -6.5f },
    { -9.1f, 0.0f, 9.1f },
    { 6.5f, 0.0f, 32.5f },
    { 12.0f, 0.0f, 60.0f },
    { 6.5f, 0.0f, 87.6f },
    { -17.5f, 0.0f, 116.5f },
    { 0.0f, 0.0f, 120.0f },
    { 45.9f, 0.0f, 110.9f },
    { 84.8f, 0.0f, 84.9f },
};

Vec mapLayout5InnerSpaceOffsets[0x16] = {
    { 80.0f, 0.0f, 31.0f },
    { 83.9f, 0.0f, -0.1f },
    { 77.2f, 0.0f, -31.7f },
    { 60.4f, 0.0f, -56.8f },
    { 35.6f, 0.0f, -77.4f },
    { -0.5f, 0.0f, -84.1f },
    { -31.5f, 0.0f, -82.9f },
    { -58.9f, 0.0f, -71.4f },
    { -86.3f, 0.0f, -49.4f },
    { -91.5f, 0.0f, -36.4f },
    { -91.5f, 0.0f, -27.3f },
    { -82.1f, 0.0f, -29.8f },
    { -54.7f, 0.0f, -36.2f },
    { -21.9f, 0.0f, -30.9f },
    { 13.3f, 0.0f, -10.2f },
    { 28.3f, 0.0f, 18.3f },
    { 33.4f, 0.0f, 58.4f },
    { 28.0f, 0.0f, 81.6f },
    { 20.7f, 0.0f, 93.3f },
    { 30.4f, 0.0f, 93.3f },
    { 55.4f, 0.0f, 82.2f },
    { 70.4f, 0.0f, 59.3f },
};

s32 mapModelResourceIds[0xC] = {
    0x220004,
    0x220005,
    0x220006,
    0x220009,
    0x22000C,
    0x22000F,
    0x220007,
    0x220008,
    0x22000A,
    0x22000B,
    0x22000D,
    0x22000E,
};

Vec mapPieceOffsets[6] = {
    { -400.0f, 0.0f, 300.0f },
    { 0.0f, 0.0f, 300.0f },
    { 400.0f, 0.0f, 300.0f },
    { -400.0f, 0.0f, 700.0f },
    { 0.0f, 0.0f, 700.0f },
    { 400.0f, 0.0f, 700.0f },
};

s32 mapOuterPieceOrder[3] = { 6, 8, 0xA };

s32 mapInnerPieceOrder[3] = { 7, 9, 0xB };

s32 mapCenterPieceOrder[5] = { 1, 2, 3, 4, 5 };

Vec *mapLayoutOuterSpaceOffsets[6] = {
    mapLayout0OuterSpaceOffsets,
    mapLayout1OuterSpaceOffsets,
    mapLayout2OuterSpaceOffsets,
    mapLayout3OuterSpaceOffsets,
    mapLayout4OuterSpaceOffsets,
    mapLayout5OuterSpaceOffsets,
};

s32 mapLayoutSpaceCounts[6] = { 0x10, 0x10, 0xA, 0xA, 0x16, 0x16 };

Vec mapLayoutOrigins[6] = {
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, -20.0f },
    { 0.0f, 0.0f, -20.0f },
    { 40.0f, 0.0f, -20.0f },
    { 40.0f, 0.0f, -20.0f },
};

Vec *mapLayoutInnerOuterSpaceOffsets[6][2] = {
    { mapLayout0OuterSpaceOffsets, mapLayout0InnerSpaceOffsets },
    { mapLayout1OuterSpaceOffsets, mapLayout1InnerSpaceOffsets },
    { mapLayout2OuterSpaceOffsets, mapLayout2InnerSpaceOffsets },
    { mapLayout3OuterSpaceOffsets, mapLayout3InnerSpaceOffsets },
    { mapLayout4OuterSpaceOffsets, mapLayout4InnerSpaceOffsets },
    { mapLayout5OuterSpaceOffsets, mapLayout5InnerSpaceOffsets },
};

s32 mapLayoutPathPointCounts[6] = { 0x10, 0x10, 0xA, 0xA, 0x16, 0x16 };

OMOBJ *mapLoadObject;
OMOBJ *mapStateObject;
OMOBJ *mapObjects[5];
s32 mapObjectCreationIndex;
s32 activeMapObjectIndex;
s32 previousMapObjectIndex;
s32 mapAdvanceRequested;
s32 mapTransitionActive;
s32 mapTransitionTimer;
Float mapRotationStep;
s32 mapRotateRequested;
s32 mapRotationDirection;
s32 mapPieceCount;
s32 mapOuterPieceCount;
s32 mapPieceOrder[6];

void MapInitialize(HUPROCESS *objectManager)
{
    Vec shadowPosition;
    Vec shadowDirection;
    Vec shadowUp;
    s32 objectIndex;

    shadowPosition.x = 0.0f;
    shadowPosition.y = 3000.0f;
    shadowPosition.z = 50.0f;
    shadowDirection.x = 0.0f;
    shadowDirection.y = 1.0f;
    shadowDirection.z = 0.0f;
    shadowUp.x = 0.0f;
    shadowUp.y = 0.0f;
    shadowUp.z = 0.0f;
    mapLoadObject = omAddObjEx(objectManager, 0xA, 2, 0, -1, MapLoadInitialize);
    mapStateObject = omAddObjEx(objectManager, 0xC, 4, 0, -1, MapStateInitialize);
    for (objectIndex = 0; objectIndex < 5; objectIndex++) {
        mapObjects[objectIndex] = omAddObjEx(objectManager, 0xE, 7, 0, -1, MapObjectUpdate);
    }
    Hu3DShadowCreate(30.0f, 20.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.65f);
    Hu3DShadowPosSet(&shadowPosition, &shadowDirection, &shadowUp);
    mapObjectCreationIndex = 0;
    activeMapObjectIndex = previousMapObjectIndex = 2;
    mapTransitionTimer = 0;
    mapTransitionActive = 0;
    mapRotationStep = 45.0f / REFRESH_RATE_F;
    mapPieceCount = 3;
    mapOuterPieceCount = 0;
    mapRotateRequested = 0;
    mapRotationDirection = 0;
    memset(mapPieceOrder, 0, 0x18);
}

void MapCleanup(void)
{
    MapObjectsDestroy();
}

void MapLoadInitialize(OMOBJ *object)
{
    object->stat |= 0x100;
    object->mdlId[0] = Hu3DModelCreateFile(0x220010);
    object->mdlId[1] = Hu3DModelCreateFile(0x220011);
    object->objFunc = MapLoadComplete;
}

void MapLoadComplete(OMOBJ *object) { }

void MapStateInitialize(OMOBJ *object)
{
    object->stat |= 0x100;
    object->mdlId[0] = Hu3DModelCreateFile(0x220000);
    object->mdlId[1] = Hu3DModelCreateFile(0x220003);
    object->mdlId[2] = Hu3DModelCreateFile(0x220001);
    object->mdlId[3] = Hu3DModelCreateFile(0x220002);
    Hu3DMotionSpeedSet(object->mdlId[2], 0.0f);
    Hu3DModelShadowSet(object->mdlId[1]);
    Hu3DModelPosSet(object->mdlId[1], 0.0f, 0.0f, -500.0f);
    Hu3DData[object->mdlId[3]].motShapeWork.time = 0.0f;
    Hu3DData[object->mdlId[3]].motShapeWork.speed = 2.0f;
    object->work[3] = 0x1E;
    object->objFunc = MapStateUpdate;
}

void MapStateUpdate(OMOBJ *stateObject)
{
    HU3DMODEL *boardModel;
    M453MapWork *mapWork;

    if (mapRotateRequested != 0) {
        if (mapRotationDirection == 0) {
            if (stateObject->work[0] < 60.0f) {
                stateObject->work[0] += 1;
                boardModel = &Hu3DData[stateObject->mdlId[1]];
                boardModel->rot.x = (-120.0f * stateObject->work[0]) / 60.0f;
            }
            else {
                mapRotationDirection ^= 1;
                mapRotateRequested = 0;
            }
        }
        else if (stateObject->work[0] != 0) {
            stateObject->work[0] -= 1;
            boardModel = &Hu3DData[stateObject->mdlId[1]];
            boardModel->rot.x = (-120.0f * stateObject->work[0]) / 60.0f;
        }
        else {
            mapRotationDirection ^= 1;
            mapRotateRequested = 0;
        }
    }
    mapWork = mapObjects[activeMapObjectIndex]->data;
    if (mapAdvanceRequested != 0) {
        mapAdvanceRequested = 0;
        mapTransitionActive = 1;
    }
    if (activeMapObjectIndex != previousMapObjectIndex) {
        mapTransitionActive = 0;
        mapTransitionTimer = 0;
    }
    previousMapObjectIndex = activeMapObjectIndex;
    if (mapWork->introFinished) {
        activeMapObjectIndex = (activeMapObjectIndex + 1) % 5;
        mapTransitionTimer = 1;
    }
    if (stateObject->work[3] != 0) {
        if (--stateObject->work[3] == 0) {
            HuAudFXPlay(0x521);
        }
    }
}

void MapObjectUpdate(OMOBJ *mapObject)
{
    s32 modelIndex;
    M453MapWork *mapWork;

    mapWork = HuMemDirectMallocNum(HEAP_HEAP, 0x54, 0x10000000);
    mapObject->data = mapWork;
    memset(mapObject->data, 0, 0x54);
    for (modelIndex = 0; modelIndex < 0xC; modelIndex++) {
        mapWork->mapModelIds[modelIndex] = -1;
    }
    for (modelIndex = 0; (u32)modelIndex < 0xC; modelIndex++) {
        mapWork->mapModelIds[modelIndex] = Hu3DModelCreateFile(mapModelResourceIds[modelIndex]);
        Hu3DModelAttrSet(mapWork->mapModelIds[modelIndex], HU3D_ATTR_DISPOFF);
    }
    mapObject->mdlId[0] = mapWork->mapModelIds[0];
    Hu3DModelAttrReset(mapObject->mdlId[0], HU3D_ATTR_DISPOFF);
    mapWork->introFinished = 0;
    mapWork->mapState = 4 - mapObjectCreationIndex;
    MapObjectEffectReset(mapObject);
    mapObject->trans.y = 5.0f;
    mapObject->trans.z = -500.0f;
    switch (mapWork->mapState) {
        case 0:
        case 1:
            break;
        case 2:
            for (modelIndex = 0; modelIndex < 7; modelIndex++) {
                Hu3DModelShadowSet(mapObject->mdlId[modelIndex]);
            }
            break;
        case 3:
            for (modelIndex = 0; modelIndex < 7; modelIndex++) {
                Hu3DModelShadowMapSet(mapObject->mdlId[modelIndex]);
            }
            mapObject->trans.y = 2.5f;
            break;
        case 4:
            for (modelIndex = 0; modelIndex < 7; modelIndex++) {
                Hu3DModelShadowMapSet(mapObject->mdlId[modelIndex]);
            }
            mapObject->trans.y = 0.0f;
            break;
    }
    MapObjectEffectStart(mapObject);
    mapObjectCreationIndex++;
    mapObject->objFunc = MapObjectEffectUpdate;
}

void MapObjectEffectStart(OMOBJ *mapObject)
{
    Mtx objectTransform;
    Mtx pieceTransform;
    s32 pieceIndex;

    MTXTrans(objectTransform, mapObject->trans.x, mapObject->trans.y, mapObject->trans.z);
    mtxRot(pieceTransform, mapObject->rot.x, mapObject->rot.y, mapObject->rot.z);
    MTXConcat(objectTransform, pieceTransform, objectTransform);
    for (pieceIndex = 0; pieceIndex < 6; pieceIndex++) {
        MTXTrans(pieceTransform, mapPieceOffsets[pieceIndex].x, mapPieceOffsets[pieceIndex].y, mapPieceOffsets[pieceIndex].z);
        MTXConcat(objectTransform, pieceTransform, pieceTransform);
        Hu3DModelPosSet(mapObject->mdlId[pieceIndex + 1], pieceTransform[0][3], pieceTransform[1][3], pieceTransform[2][3]);
        Hu3DModelRotSet(mapObject->mdlId[pieceIndex + 1], mapObject->rot.x, mapObject->rot.y, mapObject->rot.z);
    }
}

void MapObjectEffectReset(OMOBJ *mapObject)
{
    s32 availablePieceSlots[6];

    s32 pieceIndex;
    M453MapWork *mapWork;
    s32 randomIndex;
    s32 swapIndex;
    s32 swappedPiece;
    s32 availableSlotCount;
    s32 centerPieceIndex;

    mapWork = mapObject->data;
    for (pieceIndex = 0; pieceIndex < 0x64; pieceIndex++) {
        randomIndex = ((u8)frand() * 3) >> 8;
        swapIndex = ((u8)frand() * 3) >> 8;
        swappedPiece = mapOuterPieceOrder[randomIndex];
        mapOuterPieceOrder[randomIndex] = mapOuterPieceOrder[swapIndex];
        mapOuterPieceOrder[swapIndex] = swappedPiece;
        randomIndex = ((u8)frand() * 3) >> 8;
        swapIndex = ((u8)frand() * 3) >> 8;
        swappedPiece = mapInnerPieceOrder[randomIndex];
        mapInnerPieceOrder[randomIndex] = mapInnerPieceOrder[swapIndex];
        mapInnerPieceOrder[swapIndex] = swappedPiece;
        randomIndex = ((u8)frand() * 5) >> 8;
        swapIndex = ((u8)frand() * 5) >> 8;
        swappedPiece = mapCenterPieceOrder[randomIndex];
        mapCenterPieceOrder[randomIndex] = mapCenterPieceOrder[swapIndex];
        mapCenterPieceOrder[swapIndex] = swappedPiece;
    }
    availableSlotCount = 0;
    for (pieceIndex = 0; pieceIndex < 6; pieceIndex++) {
        if (mapPieceOrder[pieceIndex] < 6) {
            availablePieceSlots[availableSlotCount++] = pieceIndex;
        }
    }
    for (pieceIndex = 0; pieceIndex < 0x64; pieceIndex++) {
        randomIndex = (availableSlotCount * (u8)frand()) >> 8;
        swapIndex = (availableSlotCount * (u8)frand()) >> 8;
        swappedPiece = availablePieceSlots[randomIndex];
        availablePieceSlots[randomIndex] = availablePieceSlots[swapIndex];
        availablePieceSlots[swapIndex] = swappedPiece;
    }
    for (pieceIndex = 0; pieceIndex < 6; pieceIndex++) {
        mapWork->pieceOrder[pieceIndex] = 0;
    }
    for (pieceIndex = 0; pieceIndex < mapOuterPieceCount; pieceIndex++) {
        mapWork->pieceOrder[availablePieceSlots[pieceIndex]] = mapInnerPieceOrder[pieceIndex];
    }
    while (pieceIndex < mapPieceCount) {
        mapWork->pieceOrder[availablePieceSlots[pieceIndex]] = mapOuterPieceOrder[pieceIndex];
        pieceIndex++;
    }
    centerPieceIndex = 0;
    for (pieceIndex = 0; pieceIndex < 6; pieceIndex++) {
        if (mapWork->pieceOrder[pieceIndex] == 0) {
            mapWork->pieceOrder[pieceIndex] = mapCenterPieceOrder[centerPieceIndex++];
        }
    }
    memcpy(mapPieceOrder, &mapWork->pieceOrder, 0x18);
    for (pieceIndex = 0; pieceIndex < 6; pieceIndex++) {
        if (mapObject->mdlId[pieceIndex + 1] >= 0) {
            Hu3DModelAttrSet(mapObject->mdlId[pieceIndex + 1], HU3D_ATTR_DISPOFF);
        }
    }
    for (pieceIndex = 0; pieceIndex < 6; pieceIndex++) {
        mapObject->mdlId[pieceIndex + 1] = mapWork->mapModelIds[mapWork->pieceOrder[pieceIndex]];
        Hu3DModelAttrReset(mapObject->mdlId[pieceIndex + 1], HU3D_ATTR_DISPOFF);
    }
}

void MapObjectEffectUpdate(OMOBJ *mapObject)
{
    Mtx pieceTransform;
    Mtx objectTransform;
    M453MapWork *mapWork;
    s32 modelIndex;
    s32 pieceIndex;
    HU3DMODEL *boardModel;

    mapWork = mapObject->data;
    if (mapTransitionActive == 0) {
        if ((mapRotateRequested != 0) && (mapWork->mapState < 3)) {
            boardModel = &Hu3DData[mapStateObject->mdlId[1]];
            mapObject->rot.x = boardModel->rot.x;
            switch (mapWork->mapState) {
                case 0:
                    mapObject->trans.z = -500.0f + ((2.5f * mapStateObject->work[0]) / 60.0f);
                    break;
                case 1:
                    mapObject->trans.z = -500.0f + ((5.0f * mapStateObject->work[0]) / 60.0f);
                    break;
                case 2:
                    mapObject->trans.z = -500.0f + ((7.5f * mapStateObject->work[0]) / 60.0f);
                    break;
            }
        }
        else {
            return;
        }
    }
    else {
        switch (mapWork->mapState) {
            case 0:
                if (mapTransitionTimer != 0) {
                    mapObject->trans.z = -495.0f;
                    mapWork->mapState++;
                }
                break;
            case 1:
                if (mapTransitionTimer != 0) {
                    mapObject->trans.z = -492.5f;
                    mapWork->introFinished = 0;
                    mapWork->mapState++;
                    mapWork->mapRotation = 0.0f;
                    for (modelIndex = 0; modelIndex < 7; modelIndex++) {
                        Hu3DModelShadowSet(mapObject->mdlId[modelIndex]);
                    }
                }
                break;
            case 2:
                if (mapTransitionTimer != 0) {
                    mapWork->mapState = mapWork->mapState + 1;
                    mapObject->trans.y = 2.5f;
                    for (modelIndex = 0; modelIndex < 7; modelIndex++) {
                        Hu3DModelShadowReset(mapObject->mdlId[modelIndex]);
                        Hu3DModelShadowMapSet(mapObject->mdlId[modelIndex]);
                    }
                }
                else {
                    mapWork->mapRotation = mapWork->mapRotation + mapRotationStep;
                    if (mapWork->mapRotation > 90.0f) {
                        mapWork->introFinished = 1;
                        mapWork->mapRotation = 90.0f;
                        HuAudFXPlay(0x522);
                    }
                    mapObject->rot.x = -120.0 * cosd(mapWork->mapRotation);
                    mapObject->trans.z = -500.0 + (7.5 * cosd(mapWork->mapRotation));
                }
                break;
            case 3:
                if (mapTransitionTimer != 0) {
                    mapWork->mapState++;
                    mapObject->trans.y = 0.0f;
                }
                break;
            case 4:
                if (mapTransitionTimer != 0) {
                    mapWork->mapState = 0;
                    mapObject->trans.y = 5.0f;
                    mapObject->trans.z = -497.5f;
                    mapObject->rot.x = -120.0f;
                    for (modelIndex = 0; modelIndex < 7; modelIndex++) {
                        MapModelUnload(mapObject->mdlId[modelIndex]);
                    }
                    MapObjectEffectReset(mapObject);
                }
                break;
        }
    }
    MTXTrans(objectTransform, mapObject->trans.x, mapObject->trans.y, mapObject->trans.z);
    mtxRot(pieceTransform, mapObject->rot.x, mapObject->rot.y, mapObject->rot.z);
    MTXConcat(objectTransform, pieceTransform, objectTransform);
    for (pieceIndex = 0; pieceIndex < 6; pieceIndex++) {
        MTXTrans(pieceTransform, mapPieceOffsets[pieceIndex].x, mapPieceOffsets[pieceIndex].y, mapPieceOffsets[pieceIndex].z);
        MTXConcat(objectTransform, pieceTransform, pieceTransform);
        Hu3DModelPosSet(mapObject->mdlId[pieceIndex + 1], pieceTransform[0][3], pieceTransform[1][3], pieceTransform[2][3]);
        Hu3DModelRotSet(mapObject->mdlId[pieceIndex + 1], mapObject->rot.x, mapObject->rot.y, mapObject->rot.z);
    }
}

void MapObjectsDestroy(void)
{
    s32 objectIndex;
    OMOBJ *mapObject;
    M453MapWork *mapWork;
    s32 modelIndex;

    for (objectIndex = 0; objectIndex < 5; objectIndex++) {
        mapObject = mapObjects[objectIndex];
        mapWork = mapObject->data;
        if (mapWork != NULL) {
            for (modelIndex = 0; modelIndex < 0xC; modelIndex++) {
                if (mapWork->mapModelIds[modelIndex] >= 0) {
                    Hu3DModelKill(mapWork->mapModelIds[modelIndex]);
                }
            }
            HuMemDirectFree(mapWork);
            mapObjects[objectIndex]->data = NULL;
        }
        for (objectIndex = 0; objectIndex < mapObject->mdlcnt; objectIndex++) {
            mapObject->mdlId[objectIndex] = -1;
        }
    }
}

void MapModelUnload(s16 modelId)
{
    HSFDATA *modelData;
    HSFOBJECT *hsfObject;
    s16 objectIndex;
    HSFOBJECT *currentObject;
    HSFCONSTDATA *objectConstData;

    modelData = Hu3DData[modelId].hsf;
    for (currentObject = modelData->object, objectIndex = 0; objectIndex < modelData->objectNum; objectIndex++, currentObject++) {
        hsfObject = currentObject;
        if (hsfObject->constData) {
            objectConstData = hsfObject->constData;
            objectConstData->attr &= 0xFFFFFFF7;
        }
    }
}

s32 MapSpaceIndexGet(Float worldX, Float worldZ)
{
    s32 gridColumn;
    s32 gridRow;

    gridColumn = (600.0f + worldX) / 400.0f;
    gridRow = (400.0f + worldZ) / 400.0f;
    if (gridColumn < 0) {
        gridColumn = 0;
    }
    else if (gridColumn > 2) {
        gridColumn = 2;
    }
    if (gridRow < 0) {
        gridRow = 0;
    }
    else if (gridRow > 1) {
        gridRow = 1;
    }
    return gridColumn + (gridRow * 3);
}

void MapSpacePositionGet(s32 spaceIndex, Vec *position)
{
    position->x = mapPieceOffsets[spaceIndex].x;
    position->y = mapPieceOffsets[spaceIndex].y;
    position->z = mapPieceOffsets[spaceIndex].z;
}

s32 MapSpaceNeighborGet(M453MapWork *mapWork, s32 spaceIndex)
{
    return mapWork->pieceOrder[spaceIndex];
}

void MapTransitionRotationSpeedSet(Float rotationStep)
{
    mapAdvanceRequested = 1;
    mapRotationStep = rotationStep;
}

void MapSpaceSelect(s32 spaceCount)
{
    mapPieceCount = spaceCount;
}

void MapUpdate(void)
{
    Float rotationSpeed;
    Float motionSpeed;

    mapRotateRequested = 1;
    if (mapRotationDirection == 0) {
        rotationSpeed = 1.6666666f;
    }
    else {
        rotationSpeed = -1.6666666f;
    }
    motionSpeed = rotationSpeed;
    Hu3DMotionSpeedSet(mapStateObject->mdlId[2], motionSpeed);
    HuAudFXPlay(0x524);
}

s32 MapPathFind(Float worldX, Float worldZ, Vec *pathPoints)
{
    Mtx objectTransform;
    Mtx pointTransform;
    Vec spaceOffset;

    OMOBJ *mapObject;
    Vec *spaceOffsets;
    s32 neighborSpace;
    s32 spaceIndex;
    s32 pointIndex;
    s32 pointCount;
    M453MapWork *mapWork;

    mapObject = mapObjects[activeMapObjectIndex];
    mapWork = mapObject->data;
    spaceIndex = MapSpaceIndexGet(worldX, worldZ);
    neighborSpace = MapSpaceNeighborGet(mapWork, spaceIndex);
    if (neighborSpace < 6) {
        return 0;
    }
    neighborSpace -= 6;
    MapSpacePositionGet(spaceIndex, &spaceOffset);
    spaceOffset = mapPieceOffsets[spaceIndex];
    MTXTrans(objectTransform, mapObject->trans.x, mapObject->trans.y, mapObject->trans.z);
    mtxRot(pointTransform, mapObject->rot.x, mapObject->rot.y, mapObject->rot.z);
    MTXConcat(objectTransform, pointTransform, objectTransform);
    MTXTrans(pointTransform, spaceOffset.x, spaceOffset.y, spaceOffset.z);
    MTXConcat(objectTransform, pointTransform, objectTransform);
    spaceOffsets = mapLayoutOuterSpaceOffsets[neighborSpace];
    pointCount = mapLayoutSpaceCounts[neighborSpace];
    for (pointIndex = 0; pointIndex < pointCount; pointIndex++) {
        MTXTrans(pointTransform, spaceOffsets->x, spaceOffsets->y, spaceOffsets->z);
        MTXConcat(objectTransform, pointTransform, pointTransform);
        pathPoints->x = pointTransform[0][3];
        pathPoints->y = pointTransform[1][3];
        pathPoints->z = pointTransform[2][3];
        pathPoints++;
        spaceOffsets++;
    }
    return pointCount;
}

Float MapPlaneDistance(Float worldX, Float worldZ)
{
    Float planeDistance;

    OMOBJ *mapObject = mapObjects[activeMapObjectIndex];
    Float relativeZ = -(worldZ - mapObject->trans.z);
    if (cosd(mapObject->rot.x) <= 0.0) {
        planeDistance = 9999.0f;
    }
    else {
        planeDistance = (relativeZ * sind(mapObject->rot.x)) / cosd(mapObject->rot.x);
        if (planeDistance > 9999.0f) {
            planeDistance = 9999.0f;
        }
    }
    return planeDistance;
}

Float MapPositionDistance(Vec *position, Float heightOffset)
{
    return MapPlaneDistance(0.0f, position->z - heightOffset);
}

s32 MapRotationRequestedGet(void)
{
    return mapRotateRequested;
}

s32 MapTransitionActiveGet(void)
{
    return mapTransitionActive;
}

s32 MapSpaceNeighborsGet(M453MapWork *mapWork, s32 *neighborSpaceIndices)
{
    s32 spaceIndex;
    s32 neighborCount;

    neighborCount = 0;
    for (spaceIndex = 0; spaceIndex < 6; spaceIndex++) {
        if (mapWork->pieceOrder[spaceIndex] >= 6) {
            neighborSpaceIndices[neighborCount++] = spaceIndex;
        }
    }
    return neighborCount;
}

s32 MapNearestSpaceGet(Vec *neighborPositions)
{
    s32 neighborSpaceIndices[6];

    OMOBJ *mapObject;
    s32 neighborIndex;
    M453MapWork *mapWork;
    s32 layoutIndex;
    s32 neighborCount;

    mapObject = mapObjects[activeMapObjectIndex];
    mapWork = mapObject->data;
    neighborCount = MapSpaceNeighborsGet(mapWork, neighborSpaceIndices);
    for (neighborIndex = 0; neighborIndex < neighborCount; neighborIndex++) {
        MapSpacePositionGet(neighborSpaceIndices[neighborIndex], neighborPositions);
        layoutIndex = MapSpaceNeighborGet(mapWork, neighborSpaceIndices[neighborIndex]) - 6;
        neighborPositions->x += mapObject->trans.x + mapLayoutOrigins[layoutIndex].x;
        neighborPositions->y += mapObject->trans.y + mapLayoutOrigins[layoutIndex].y;
        neighborPositions->z += mapObject->trans.z + mapLayoutOrigins[layoutIndex].z;
        neighborPositions++;
    }
    return neighborCount;
}

s32 MapNearestPathGet(Vec *neighborPathPoints)
{
    Mtx objectTransform;
    Mtx pointTransform;
    s32 neighborSpaceIndices[6];

    OMOBJ *mapObject;
    s32 neighborIndex;
    M453MapWork *mapWork;
    s32 layoutIndex;
    s32 neighborCount;

    mapObject = mapObjects[activeMapObjectIndex];
    mapWork = mapObject->data;
    neighborCount = MapSpaceNeighborsGet(mapWork, neighborSpaceIndices);
    MTXTrans(objectTransform, mapObject->trans.x, mapObject->trans.y, mapObject->trans.z);
    mtxRot(pointTransform, mapObject->rot.x, mapObject->rot.y, mapObject->rot.z);
    MTXConcat(objectTransform, pointTransform, objectTransform);
    for (neighborIndex = 0; neighborIndex < neighborCount; neighborIndex++) {
        MapSpacePositionGet(neighborSpaceIndices[neighborIndex], neighborPathPoints);
        layoutIndex = MapSpaceNeighborGet(mapWork, neighborSpaceIndices[neighborIndex]) - 6;
        neighborPathPoints->x += mapLayoutOrigins[layoutIndex].x;
        neighborPathPoints->y += mapLayoutOrigins[layoutIndex].y;
        neighborPathPoints->z += mapLayoutOrigins[layoutIndex].z;
        MTXTrans(pointTransform, neighborPathPoints->x, neighborPathPoints->y, neighborPathPoints->z);
        MTXConcat(objectTransform, pointTransform, pointTransform);
        neighborPathPoints->x = pointTransform[0][3];
        neighborPathPoints->y = pointTransform[1][3];
        neighborPathPoints->z = pointTransform[2][3];
        neighborPathPoints++;
    }
    return neighborCount;
}

void MapSpaceHighlightSet(s32 highlightedSpaceCount)
{
    if (highlightedSpaceCount > mapPieceCount) {
        highlightedSpaceCount = mapPieceCount;
    }
    mapOuterPieceCount = highlightedSpaceCount;
}

s32 MapSpaceContainsPosition(Vec *position, Vec *hitPosition)
{
    Vec spaceOrigin;
    Vec relativePosition;
    Vec edgeDirection;
    Vec edgeVector;
    Vec relativeEdgePoint;
    Vec crossProduct;

    Float intersectionZ;
    Float projectionScale;
    Float intersectionX;
    Float intersectionY;
    Float edgeLengthSquared;
    Float intersectionScale;
    Float worldX;
    Float worldZ;

    s32 edgeIndex;
    Vec *outerBoundary;
    Vec *innerBoundary;
    OMOBJ *mapObject;
    s32 nextEdgeIndex;
    s32 spaceIndex;
    s32 pathPointCount;
    s32 intersectionCount;
    s32 gridSpaceIndex;
    M453MapWork *mapWork;

    mapObject = mapObjects[activeMapObjectIndex];
    mapWork = mapObject->data;
    if ((mapObject->rot.x > 0.0f) || (mapObject->rot.x <= -90.0f)) {
        return 0;
    }
    VECSubtract(position, &mapObject->trans, &relativePosition);
    relativePosition.z = relativePosition.z / cosd(mapObject->rot.x);
    worldZ = position->z;
    worldX = position->x;
    gridSpaceIndex = MapSpaceIndexGet(worldX, worldZ);
    spaceIndex = MapSpaceNeighborGet(mapWork, gridSpaceIndex);
    if (spaceIndex < 6) {
        return 0;
    }
    spaceIndex -= 6;
    spaceOrigin = mapPieceOffsets[gridSpaceIndex];
    VECSubtract(&relativePosition, &spaceOrigin, &relativePosition);
    outerBoundary = mapLayoutInnerOuterSpaceOffsets[spaceIndex][0];
    innerBoundary = mapLayoutInnerOuterSpaceOffsets[spaceIndex][1];
    pathPointCount = mapLayoutPathPointCounts[spaceIndex];
    for (edgeIndex = 0; edgeIndex < pathPointCount; edgeIndex++) {
        nextEdgeIndex = (edgeIndex + 1) % pathPointCount;
        VECSubtract(outerBoundary + nextEdgeIndex, outerBoundary + edgeIndex, &edgeVector);
        VECSubtract(&relativePosition, outerBoundary + nextEdgeIndex, &relativeEdgePoint);
        VECCrossProduct(&edgeVector, &relativeEdgePoint, &crossProduct);
        if (!(crossProduct.y < 0.0f)) {
            VECSubtract(innerBoundary + nextEdgeIndex, outerBoundary + nextEdgeIndex, &edgeVector);
            VECSubtract(&relativePosition, innerBoundary + nextEdgeIndex, &relativeEdgePoint);
            VECCrossProduct(&edgeVector, &relativeEdgePoint, &crossProduct);
            if (!(crossProduct.y < 0.0f)) {
                VECSubtract(innerBoundary + edgeIndex, innerBoundary + nextEdgeIndex, &edgeVector);
                VECSubtract(&relativePosition, innerBoundary + edgeIndex, &relativeEdgePoint);
                VECCrossProduct(&edgeVector, &relativeEdgePoint, &crossProduct);
                if (!(crossProduct.y < 0.0f)) {
                    VECSubtract(outerBoundary + edgeIndex, innerBoundary + edgeIndex, &edgeVector);
                    VECSubtract(&relativePosition, outerBoundary + edgeIndex, &relativeEdgePoint);
                    VECCrossProduct(&edgeVector, &relativeEdgePoint, &crossProduct);
                    if (!(crossProduct.y < 0.0f)) {
                        VECSubtract(outerBoundary + nextEdgeIndex, outerBoundary + edgeIndex, &edgeVector);
                        projectionScale = (((relativePosition.z * edgeVector.z)
                                       + (((relativePosition.y * edgeVector.y) + ((relativePosition.x * edgeVector.x) - (edgeVector.x * outerBoundary[edgeIndex].x))) - (edgeVector.y * outerBoundary[edgeIndex].y)))
                                      - (edgeVector.z * outerBoundary[edgeIndex].z))
                            / ((edgeVector.z * edgeVector.z) + ((edgeVector.x * edgeVector.x) + (edgeVector.y * edgeVector.y)));
                        edgeDirection.x = outerBoundary[edgeIndex].x + (projectionScale * edgeVector.x);
                        edgeDirection.y = outerBoundary[edgeIndex].y + (projectionScale * edgeVector.y);
                        edgeDirection.z = outerBoundary[edgeIndex].z + (projectionScale * edgeVector.z);
                        VECSubtract(&edgeDirection, &relativePosition, &crossProduct);
                        intersectionY = VECMag(&crossProduct);
                        VECSubtract(innerBoundary + nextEdgeIndex, innerBoundary + edgeIndex, &relativeEdgePoint);
                        intersectionX = (((relativePosition.z * relativeEdgePoint.z)
                                       + (((relativePosition.y * relativeEdgePoint.y) + ((relativePosition.x * relativeEdgePoint.x) - (relativeEdgePoint.x * innerBoundary[edgeIndex].x))) - (relativeEdgePoint.y * innerBoundary[edgeIndex].y)))
                                      - (relativeEdgePoint.z * innerBoundary[edgeIndex].z))
                            / ((relativeEdgePoint.z * relativeEdgePoint.z) + ((relativeEdgePoint.x * relativeEdgePoint.x) + (relativeEdgePoint.y * relativeEdgePoint.y)));
                        edgeDirection.x = innerBoundary[edgeIndex].x + (intersectionX * relativeEdgePoint.x);
                        edgeDirection.y = innerBoundary[edgeIndex].y + (intersectionX * relativeEdgePoint.y);
                        edgeDirection.z = innerBoundary[edgeIndex].z + (intersectionX * relativeEdgePoint.z);
                        VECSubtract(&edgeDirection, &relativePosition, &crossProduct);
                        edgeLengthSquared = VECMag(&crossProduct);
                        if (intersectionY == 0.0f) {
                            intersectionZ = projectionScale;
                        }
                        else if (edgeLengthSquared == 0.0f) {
                            intersectionZ = intersectionX;
                        }
                        else {
                            intersectionScale = 1.0f - (intersectionY / (intersectionY + edgeLengthSquared));
                            intersectionZ = (projectionScale * intersectionScale) + (intersectionX * (1.0f - intersectionScale));
                        }
                        if (intersectionZ < 0.0f) {
                            intersectionZ = 0.0f;
                        }
                        else if (intersectionZ > 1.0f) {
                            intersectionZ = 1.0f;
                        }
                        relativePosition.x = innerBoundary[edgeIndex].x + (intersectionZ * relativeEdgePoint.x);
                        relativePosition.y = innerBoundary[edgeIndex].y + (intersectionZ * relativeEdgePoint.y);
                        relativePosition.z = innerBoundary[edgeIndex].z + (intersectionZ * relativeEdgePoint.z);
                        hitPosition->x = relativePosition.x + (mapObject->trans.x + spaceOrigin.x);
                        hitPosition->y = relativePosition.y + (mapObject->trans.y + spaceOrigin.y);
                        hitPosition->z = mapObject->trans.z + ((spaceOrigin.z + relativePosition.z) * cosd(mapObject->rot.x));
                        return 1;
                    }
                }
            }
        }
    }
    intersectionCount = 0;
    for (edgeIndex = 0; edgeIndex < pathPointCount; edgeIndex++) {
        nextEdgeIndex = (edgeIndex + 1) % pathPointCount;
        intersectionZ = (relativePosition.z - outerBoundary[edgeIndex].z) / (outerBoundary[nextEdgeIndex].z - outerBoundary[edgeIndex].z);
        crossProduct.x = outerBoundary[edgeIndex].x + (intersectionZ * (outerBoundary[nextEdgeIndex].x - outerBoundary[edgeIndex].x));
        if ((intersectionZ >= 0.0f) && (intersectionZ < 1.0f) && (crossProduct.x > relativePosition.x)) {
            intersectionCount++;
        }
    }
    if ((intersectionCount & 1) != 0) {
        hitPosition->x = position->x;
        hitPosition->y = position->y;
        hitPosition->z = position->z;
        return 1;
    }
    return 0;
}
