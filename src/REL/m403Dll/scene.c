#include "REL/m403Dll.h"
#include "game/audio.h"
#include "game/frand.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/object.h"

#include "ext_math.h"
#include <string.h>

typedef struct {
    struct {
        u8 isActiveFrame : 1;
    };
    char padding[3];
    s32 sceneVariant;
    float animationProgress;
    s32 pieceAssignments[6];
    s32 pieceModelIds[12];
} PuzzleSceneWork; // Size 0x54

static void InitializeSceneDoor(OMOBJ *object);
static void UpdateSceneDoorIdle(OMOBJ *object);
static void InitializePuzzleDoor(OMOBJ *object);
static void UpdatePuzzleDoor(OMOBJ *object);
static void InitializePuzzlePiece(OMOBJ *object);
static void UpdatePuzzlePieceModels(OMOBJ *object);
static void AssignPuzzlePieceModels(OMOBJ *object);
static void UpdatePuzzlePiece(OMOBJ *object);
static void ReleasePuzzleSceneResources(void);
static void SetPieceModelVisibility(s16 parameter1);
static s32 GetGridCellIndex(float parameter1, float parameter2);
static void GetGridCellPosition(s32 parameter1, Vec *parameter2);
static s32 GetPieceAssignment(PuzzleSceneWork *parameter1, s32 parameter2);
static s32 GetPieceWorldPositions(float parameter1, float parameter2, Vec *parameter3);
static s32 GetScenePieceCount(void);
static s32 FindCompletedPieces(PuzzleSceneWork *parameter1, s32 *parameter2);

static OMOBJ *puzzleSceneObject;
static OMOBJ *doorObject;
static OMOBJ *puzzlePieceObjects[5];
static s32 sceneState;
static s32 sceneTimer;
static s32 sceneAnimationState;
static s32 activePieceCount;
static s32 sceneSpeed;
static s32 scenePieceLimit;
static float doorOpenAmount;
static s32 selectedPieceIndex;
static s32 completedPieceCount;
static s32 maximumPieceCount;
static s32 currentPieceCount;
static s32 pieceAssignmentBuffer[6];

static Vec pieceCircleOffsets100[] = { { 92.39f, 0.0f, 38.27f }, { 100.0f, 0.0f, 0.0f }, { 92.39f, 0.0f, -38.27f }, { 70.71f, 0.0f, -70.71f },
    { 38.27f, 0.0f, -92.39f }, { 0.0f, 0.0f, -100.0f }, { -38.27f, 0.0f, -92.39f }, { -70.71f, 0.0f, -70.71f }, { -92.39f, 0.0f, -38.27f },
    { -100.0f, 0.0f, 0.0f }, { -92.39f, 0.0f, 38.27f }, { -70.71f, 0.0f, 70.71f }, { -38.27f, 0.0f, 92.39f }, { 0.0f, 0.0f, 100.0f },
    { 38.27f, 0.0f, 92.39f }, { 70.71f, 0.0f, 70.71f } };

static Vec pieceCircleOffsets60[] = { { 55.43f, 0.0f, 22.96f }, { 60.0f, 0.0f, 0.0f }, { 55.43f, 0.0f, -22.96f }, { 42.43f, 0.0f, -42.43f },
    { 22.96f, 0.0f, -55.43f }, { 0.0f, 0.0f, -60.0f }, { -22.96f, 0.0f, -55.43f }, { -42.43f, 0.0f, -42.43f }, { -55.43f, 0.0f, -22.96f },
    { -60.0f, 0.0f, 0.0f }, { -55.43f, 0.0f, 22.96f }, { -42.43f, 0.0f, 42.43f }, { -22.96f, 0.0f, 55.43f }, { 0.0f, 0.0f, 60.0f },
    { 22.96f, 0.0f, 55.43f }, { 42.43f, 0.0f, 42.43f } };

static Vec pieceLayoutOffsetsLarge[]
    = { { 38.79f, 0.0f, -53.4f }, { 0.0f, 0.0f, -120.0f }, { -38.79f, 0.0f, -53.4f }, { -114.13f, 0.0f, -37.08f }, { -62.77f, 0.0f, 20.4f },
          { -70.53f, 0.0f, 97.08f }, { 0.0f, 0.0f, 66.0f }, { 70.53f, 0.0f, 97.08f }, { 62.77f, 0.0f, 20.4f }, { 114.13f, 0.0f, -37.08f } };

static Vec pieceLayoutOffsetsSmall[]
    = { { 21.34f, 0.0f, -29.37f }, { 0.0f, 0.0f, -66.0f }, { -21.34f, 0.0f, -29.37f }, { -62.77f, 0.0f, -20.4f }, { -34.52f, 0.0f, 11.22f },
          { -38.79f, 0.0f, 53.4f }, { 0.0f, 0.0f, 36.3f }, { 38.79f, 0.0f, 53.4f }, { 34.52f, 0.0f, 11.22f }, { 62.77f, 0.0f, -20.4f } };

static Vec pieceCircleOffsets120[] = { { 110.87f, 0.0f, 45.92f }, { 120.0f, 0.0f, 0.0f }, { 110.87f, 0.0f, -45.92f }, { 84.85f, 0.0f, -84.85f },
    { 45.92f, 0.0f, -110.87f }, { 0.0f, 0.0f, -120.0f }, { -45.92f, 0.0f, -110.87f }, { -84.85f, 0.0f, -84.85f }, { -110.87f, 0.0f, -45.92f },
    { -120.0f, 0.0f, 0.0f }, { -110.87f, 0.0f, 45.92f }, { -84.85f, 0.0f, 84.85f }, { -45.92f, 0.0f, 110.87f }, { 0.0f, 0.0f, 120.0f },
    { 45.92f, 0.0f, 110.87f }, { 84.85f, 0.0f, 84.85f } };

static Vec pieceCircleOffsets79[] = { { 73.17f, 0.0f, 30.31f }, { 79.2f, 0.0f, 0.0f }, { 73.17f, 0.0f, -30.31f }, { 56.0f, 0.0f, -56.0f },
    { 30.31f, 0.0f, -73.17f }, { 0.0f, 0.0f, -79.2f }, { -30.31f, 0.0f, -73.17f }, { -56.0f, 0.0f, -56.0f }, { -73.17f, 0.0f, -30.31f },
    { -79.2f, 0.0f, 0.0f }, { -73.17f, 0.0f, 30.31f }, { -56.0f, 0.0f, 56.0f }, { -30.31f, 0.0f, 73.17f }, { 0.0f, 0.0f, 79.2f },
    { 30.31f, 0.0f, 73.17f }, { 56.0f, 0.0f, 56.0f } };

static Vec pieceLayoutOffsets150[]
    = { { 48.49f, 0.0f, -66.74f }, { 0.0f, 0.0f, -150.0f }, { -48.49f, 0.0f, -66.74f }, { -142.66f, 0.0f, -46.35f }, { -78.46f, 0.0f, 25.49f },
          { -88.17f, 0.0f, 121.35f }, { 0.0f, 0.0f, 82.5f }, { 88.17f, 0.0f, 121.35f }, { 78.46f, 0.0f, 25.49f }, { 142.66f, 0.0f, -46.35f } };

static Vec pieceLayoutOffsets90[]
    = { { 29.09f, 0.0f, -40.05f }, { 0.0f, 0.0f, -90.0f }, { -29.09f, 0.0f, -40.05f }, { -85.59f, 0.0f, -27.81f }, { -47.08f, 0.0f, 15.3f },
          { -52.9f, 0.0f, 72.81f }, { 0.0f, 0.0f, 49.5f }, { 52.9f, 0.0f, 72.81f }, { 47.08f, 0.0f, 15.3f }, { 85.59f, 0.0f, -27.81f } };

static Vec pieceCircleOffsets140[] = { { 129.1f, 0.0f, 54.6f }, { 140.0f, 0.0f, 0.0f }, { 129.3f, 0.0f, -53.6f }, { 99.0f, 0.0f, -99.0f },
    { 53.6f, 0.0f, -129.3f }, { 0.0f, 0.0f, -140.0f }, { -53.6f, 0.0f, -129.3f }, { -99.0f, 0.0f, -99.0f }, { -129.4f, 0.0f, -53.5f },
    { -140.0f, 0.0f, 0.0f }, { -136.0f, 0.0f, 20.4f }, { -102.1f, 0.0f, -7.6f }, { -70.0f, 0.0f, -14.0f }, { -37.8f, 0.0f, -7.6f },
    { -10.6f, 0.0f, 10.6f }, { 7.6f, 0.0f, 37.8f }, { 14.0f, 0.0f, 70.0f }, { 7.6f, 0.0f, 102.2f }, { -20.4f, 0.0f, 136.0f }, { 0.1f, 0.0f, 140.1f },
    { 53.9f, 0.0f, 129.5f }, { 99.5f, 0.0f, 99.0f } };

static Vec pieceLayoutOffsets107[] = { { 95.9f, 0.0f, 42.9f }, { 106.6f, 0.0f, 0.4f }, { 98.4f, 0.0f, -37.2f }, { 81.7f, 0.0f, -65.9f },
    { 55.4f, 0.0f, -89.6f }, { 11.5f, 0.0f, -101.7f }, { -24.5f, 0.0f, -99.9f }, { -60.9f, 0.0f, -88.6f }, { -98.7f, 0.0f, -64.4f },
    { -110.4f, 0.0f, -41.5f }, { -110.4f, 0.0f, -30.8f }, { -90.4f, 0.0f, -42.3f }, { -53.8f, 0.0f, -48.5f }, { -14.6f, 0.0f, -40.5f },
    { 17.1f, 0.0f, -16.3f }, { 35.8f, 0.0f, 28.2f }, { 35.8f, 0.0f, 65.6f }, { 24.7f, 0.0f, 101.6f }, { 19.6f, 0.0f, 114.6f },
    { 34.9f, 0.0f, 114.5f }, { 62.9f, 0.0f, 99.0f }, { 83.0f, 0.0f, 73.2f } };

static Vec pieceCircleOffsets120Variant[] = { { 110.9f, 0.0f, 45.9f }, { 120.0f, 0.0f, 0.0f }, { 110.9f, 0.0f, -45.8f }, { 84.8f, 0.0f, -84.9f },
    { 45.9f, 0.0f, -110.9f }, { 0.0f, 0.0f, -120.0f }, { -45.9f, 0.0f, -110.9f }, { -84.9f, 0.0f, -84.8f }, { -110.9f, 0.0f, -45.9f },
    { -120.0f, 0.0f, 0.0f }, { -116.5f, 0.0f, 17.5f }, { -87.6f, 0.0f, -6.6f }, { -60.0f, 0.0f, -12.0f }, { -32.4f, 0.0f, -6.5f },
    { -9.1f, 0.0f, 9.1f }, { 6.5f, 0.0f, 32.5f }, { 12.0f, 0.0f, 60.0f }, { 6.5f, 0.0f, 87.6f }, { -17.5f, 0.0f, 116.5f }, { 0.0f, 0.0f, 120.0f },
    { 45.9f, 0.0f, 110.9f }, { 84.8f, 0.0f, 84.9f } };

static Vec pieceLayoutOffsets84[] = { { 80.0f, 0.0f, 31.0f }, { 83.9f, 0.0f, -0.1f }, { 77.2f, 0.0f, -31.7f }, { 60.4f, 0.0f, -56.8f },
    { 35.6f, 0.0f, -77.4f }, { -0.5f, 0.0f, -84.1f }, { -31.5f, 0.0f, -82.9f }, { -58.9f, 0.0f, -71.4f }, { -86.3f, 0.0f, -49.4f },
    { -91.5f, 0.0f, -36.4f }, { -91.5f, 0.0f, -27.3f }, { -82.1f, 0.0f, -29.8f }, { -54.7f, 0.0f, -36.2f }, { -21.9f, 0.0f, -30.9f },
    { 13.3f, 0.0f, -10.2f }, { 28.3f, 0.0f, 18.3f }, { 33.4f, 0.0f, 58.4f }, { 28.0f, 0.0f, 81.6f }, { 20.7f, 0.0f, 93.3f }, { 30.4f, 0.0f, 93.3f },
    { 55.4f, 0.0f, 82.2f }, { 70.4f, 0.0f, 59.3f } };

static s32 pieceModelAssetIds[]
    = { DATA_MAKE_NUM(DATADIR_M403, 4), DATA_MAKE_NUM(DATADIR_M403, 5), DATA_MAKE_NUM(DATADIR_M403, 6), DATA_MAKE_NUM(DATADIR_M403, 9),
          DATA_MAKE_NUM(DATADIR_M403, 12), DATA_MAKE_NUM(DATADIR_M403, 15), DATA_MAKE_NUM(DATADIR_M403, 7), DATA_MAKE_NUM(DATADIR_M403, 8),
          DATA_MAKE_NUM(DATADIR_M403, 10), DATA_MAKE_NUM(DATADIR_M403, 11), DATA_MAKE_NUM(DATADIR_M403, 13), DATA_MAKE_NUM(DATADIR_M403, 14) };

static Vec gridCellOrigins[] = { { -400.0f, 0.0f, 300.0f }, { 0.0f, 0.0f, 300.0f }, { 400.0f, 0.0f, 300.0f }, { -400.0f, 0.0f, 700.0f },
    { 0.0f, 0.0f, 700.0f }, { 400.0f, 0.0f, 700.0f } };

static s32 pieceStartIndices[] = { 6, 8, 10 };
static s32 pieceEndIndices[] = { 7, 9, 11 };
static s32 scenePieceCounts[] = { 1, 2, 3, 4, 5 };

static Vec *pieceCirclePositionTables[] = { pieceCircleOffsets120, pieceCircleOffsets100, pieceLayoutOffsets150, pieceLayoutOffsetsLarge, pieceCircleOffsets140, pieceCircleOffsets120Variant };

static s32 pieceCircleVertexCounts[] = { 16, 16, 10, 10, 22, 22 };

static Vec pieceModelOffsets[] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -20.0f }, { 0.0f, 0.0f, -20.0f }, { 40.0f, 0.0f, -20.0f },
    { 40.0f, 0.0f, -20.0f } };

static Vec *pieceLayoutPositionTables[][2] = { { pieceCircleOffsets120, pieceCircleOffsets79 }, { pieceCircleOffsets100, pieceCircleOffsets60 }, { pieceLayoutOffsets150, pieceLayoutOffsets90 },
    { pieceLayoutOffsetsLarge, pieceLayoutOffsetsSmall }, { pieceCircleOffsets140, pieceLayoutOffsets107 }, { pieceCircleOffsets120Variant, pieceLayoutOffsets84 } };

static s32 pieceLayoutVertexCounts[] = { 16, 16, 10, 10, 22, 22 };

void CreatePuzzleScene(HUPROCESS *objectManager)
{
    Vec shadowPosition;
    Vec shadowDirection;
    Vec shadowTarget;
    s32 i;

    shadowPosition.x = 0.0f;
    shadowPosition.y = 3000.0f;
    shadowPosition.z = 50.0f;
    shadowDirection.x = 0.0f;
    shadowDirection.y = 1.0f;
    shadowDirection.z = 0.0f;
    shadowTarget.x = 0.0f;
    shadowTarget.y = 0.0f;
    shadowTarget.z = 0.0f;
    puzzleSceneObject = omAddObjEx(objectManager, 10, 2, 0, -1, InitializeSceneDoor);
    doorObject = omAddObjEx(objectManager, 12, 4, 0, -1, InitializePuzzleDoor);
    for (i = 0; i < 5; i++) {
        puzzlePieceObjects[i] = omAddObjEx(objectManager, 14, 7, 0, -1, InitializePuzzlePiece);
    }
    Hu3DShadowCreate(30.0f, 20.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.65f);
    Hu3DShadowPosSet(&shadowPosition, &shadowDirection, &shadowTarget);
    sceneState = 0;
    sceneTimer = sceneAnimationState = 2;
    scenePieceLimit = 0;
    sceneSpeed = 0;
    doorOpenAmount = 45.0f/REFRESH_RATE;
    maximumPieceCount = 3;
    currentPieceCount = 0;
    selectedPieceIndex = 0;
    completedPieceCount = 0;
    memset(pieceAssignmentBuffer, 0, sizeof(pieceAssignmentBuffer));
}

void DestroyPuzzleScene(void)
{
    ReleasePuzzleSceneResources();
}

static void InitializeSceneDoor(OMOBJ *object)
{
    object->stat |= 0x100;
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M403, 16));
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M403, 17));
    object->objFunc = UpdateSceneDoorIdle;
}

static void UpdateSceneDoorIdle(OMOBJ *object) { }

static void InitializePuzzleDoor(OMOBJ *object)
{
    object->stat |= 0x100;
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M403, 0));
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M403, 3));
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M403, 1));
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M403, 2));
    Hu3DMotionSpeedSet(object->mdlId[2], 0.0f);
    Hu3DModelShadowSet(object->mdlId[1]);
    Hu3DModelPosSet(object->mdlId[1], 0.0f, 0.0f, -500.0f);
    Hu3DData[object->mdlId[3]].motShapeWork.time = 0.0f;
    Hu3DData[object->mdlId[3]].motShapeWork.speed = 2.0f;
    object->work[3] = 30;
    object->objFunc = UpdatePuzzleDoor;
}

static void UpdatePuzzleDoor(OMOBJ *object)
{
    HU3DMODEL *modelData;
    PuzzleSceneWork *activePieceState;

    if (selectedPieceIndex != 0) {
        if (completedPieceCount == 0) {
            if (object->work[0] < 60.0f) {
                object->work[0]++;
                modelData = &Hu3DData[object->mdlId[1]];
                modelData->rot.x = -120.0f * object->work[0] / 60.0f;
            }
            else {
                completedPieceCount ^= 1;
                selectedPieceIndex = 0;
            }
        }
        else if (object->work[0] != 0) {
            object->work[0]--;
            modelData = &Hu3DData[object->mdlId[1]];
            modelData->rot.x = -120.0f * object->work[0] / 60.0f;
        }
        else {
            completedPieceCount ^= 1;
            selectedPieceIndex = 0;
        }
    }
    activePieceState = puzzlePieceObjects[sceneTimer]->data;
    if (activePieceCount != 0) {
        activePieceCount = 0;
        sceneSpeed = 1;
    }
    if (sceneTimer != sceneAnimationState) {
        sceneSpeed = 0;
        scenePieceLimit = 0;
    }
    sceneAnimationState = sceneTimer;
    if (activePieceState->isActiveFrame != 0) {
        sceneTimer = (sceneTimer + 1) % 5;
        scenePieceLimit = 1;
    }
    if (object->work[3] != 0) {
        if (--object->work[3] == 0) {
            HuAudFXPlay(0x521);
        }
    }
}

static void InitializePuzzlePiece(OMOBJ *object)
{
    PuzzleSceneWork *pieceState;
    s32 i;

    pieceState = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*pieceState), HU_MEMNUM_OVL);
    object->data = pieceState;
    memset(object->data, 0, sizeof(*pieceState));
    for (i = 0; i < 12; i++) {
        pieceState->pieceModelIds[i] = -1;
    }
    for (i = 0; i < 12U; i++) {
        pieceState->pieceModelIds[i] = Hu3DModelCreateFile(pieceModelAssetIds[i]);
        Hu3DModelAttrSet(pieceState->pieceModelIds[i], HU3D_ATTR_DISPOFF);
    }
    object->mdlId[0] = pieceState->pieceModelIds[0];
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    pieceState->isActiveFrame = 0;
    pieceState->sceneVariant = 4 - sceneState;
    AssignPuzzlePieceModels(object);
    object->trans.y = 5.0f;
    object->trans.z = -500.0f;
    switch (pieceState->sceneVariant) {
        case 0:
        case 1:
            break;
        case 2:
            for (i = 0; i < 7; i++) {
                Hu3DModelShadowSet(object->mdlId[i]);
            }
            break;
        case 3:
            for (i = 0; i < 7; i++) {
                Hu3DModelShadowMapSet(object->mdlId[i]);
            }
            object->trans.y = 2.5f;
            break;
        case 4:
            for (i = 0; i < 7; i++) {
                Hu3DModelShadowMapSet(object->mdlId[i]);
            }
            object->trans.y = 0.0f;
            break;
    }
    UpdatePuzzlePieceModels(object);
    sceneState++;
    object->objFunc = UpdatePuzzlePiece;
}

static void UpdatePuzzlePieceModels(OMOBJ *object)
{
    Mtx objectTransform;
    Mtx modelTransform;
    s32 i;

    MTXTrans(objectTransform, object->trans.x, object->trans.y, object->trans.z);
    mtxRot(modelTransform, object->rot.x, object->rot.y, object->rot.z);
    MTXConcat(objectTransform, modelTransform, objectTransform);
    for (i = 0; i < 6; i++) {
        MTXTrans(modelTransform, gridCellOrigins[i].x, gridCellOrigins[i].y, gridCellOrigins[i].z);
        MTXConcat(objectTransform, modelTransform, modelTransform);
        Hu3DModelPosSet(object->mdlId[i + 1], modelTransform[0][3], modelTransform[1][3], modelTransform[2][3]);
        Hu3DModelRotSet(object->mdlId[i + 1], object->rot.x, object->rot.y, object->rot.z);
    }
}

static void AssignPuzzlePieceModels(OMOBJ *object)
{
    PuzzleSceneWork *pieceState;
    s32 firstRandomIndex;
    s32 secondRandomIndex;
    s32 missingPieceIndex;
    s32 availablePieceCount;
    s32 swappedPieceValue;
    s32 availablePieceIndices[6];
    s32 i;

    pieceState = object->data;
    for (i = 0; i < 100; i++) {
        firstRandomIndex = (frand8() * 3) >> 8;
        secondRandomIndex = (frand8() * 3) >> 8;
        swappedPieceValue = pieceStartIndices[firstRandomIndex];
        pieceStartIndices[firstRandomIndex] = pieceStartIndices[secondRandomIndex];
        pieceStartIndices[secondRandomIndex] = swappedPieceValue;
        firstRandomIndex = (frand8() * 3) >> 8;
        secondRandomIndex = (frand8() * 3) >> 8;
        swappedPieceValue = pieceEndIndices[firstRandomIndex];
        pieceEndIndices[firstRandomIndex] = pieceEndIndices[secondRandomIndex];
        pieceEndIndices[secondRandomIndex] = swappedPieceValue;
        firstRandomIndex = (frand8() * 5) >> 8;
        secondRandomIndex = (frand8() * 5) >> 8;
        swappedPieceValue = scenePieceCounts[firstRandomIndex];
        scenePieceCounts[firstRandomIndex] = scenePieceCounts[secondRandomIndex];
        scenePieceCounts[secondRandomIndex] = swappedPieceValue;
    }
    availablePieceCount = 0;
    for (i = 0; i < 6; i++) {
        if (pieceAssignmentBuffer[i] < 6) {
            availablePieceIndices[availablePieceCount++] = i;
        }
    }
    for (i = 0; i < 100; i++) {
        firstRandomIndex = (availablePieceCount * frand8()) >> 8;
        secondRandomIndex = (availablePieceCount * frand8()) >> 8;
        swappedPieceValue = availablePieceIndices[firstRandomIndex];
        availablePieceIndices[firstRandomIndex] = availablePieceIndices[secondRandomIndex];
        availablePieceIndices[secondRandomIndex] = swappedPieceValue;
    }
    for (i = 0; i < 6; i++) {
        pieceState->pieceAssignments[i] = 0;
    }
    for (i = 0; i < currentPieceCount; i++) {
        pieceState->pieceAssignments[availablePieceIndices[i]] = pieceEndIndices[i];
    }
    for (; i < maximumPieceCount; i++) {
        pieceState->pieceAssignments[availablePieceIndices[i]] = pieceStartIndices[i];
    }
    missingPieceIndex = 0;
    for (i = 0; i < 6; i++) {
        if (pieceState->pieceAssignments[i] == 0) {
            pieceState->pieceAssignments[i] = scenePieceCounts[missingPieceIndex++];
        }
    }
    memcpy(pieceAssignmentBuffer, &pieceState->pieceAssignments, sizeof(pieceState->pieceAssignments));
    for (i = 0; i < 6; i++) {
        if (object->mdlId[i + 1] >= 0) {
            Hu3DModelAttrSet(object->mdlId[i + 1], HU3D_ATTR_DISPOFF);
        }
    }
    for (i = 0; i < 6; i++) {
        object->mdlId[i + 1] = pieceState->pieceModelIds[pieceState->pieceAssignments[i]];
        Hu3DModelAttrReset(object->mdlId[i + 1], HU3D_ATTR_DISPOFF);
    }
}

static void UpdatePuzzlePiece(OMOBJ *object)
{
    PuzzleSceneWork *pieceState;
    HU3DMODEL *doorModelData;
    s32 i;

    pieceState = object->data;
    if (sceneSpeed == 0) {
        if (selectedPieceIndex != 0 && pieceState->sceneVariant < 3) {
            doorModelData = &Hu3DData[doorObject->mdlId[1]];
            object->rot.x = doorModelData->rot.x;
            switch (pieceState->sceneVariant) {
                case 0:
                    object->trans.z = -500.0f + 2.5f * doorObject->work[0] / 60.0f;
                    break;
                case 1:
                    object->trans.z = -500.0f + 5.0f * doorObject->work[0] / 60.0f;
                    break;
                case 2:
                    object->trans.z = -500.0f + 7.5f * doorObject->work[0] / 60.0f;
                    break;
            }
        }
        else {
            return;
        }
    }
    else {
        switch (pieceState->sceneVariant) {
            case 0:
                if (scenePieceLimit != 0) {
                    object->trans.z = -495.0f;
                    pieceState->sceneVariant++;
                }
                break;
            case 1:
                if (scenePieceLimit != 0) {
                    object->trans.z = -492.5f;
                    pieceState->isActiveFrame = 0;
                    pieceState->sceneVariant++;
                    pieceState->animationProgress = 0.0f;
                    for (i = 0; i < 7; i++) {
                        Hu3DModelShadowSet(object->mdlId[i]);
                    }
                }
                break;
            case 2:
                if (scenePieceLimit != 0) {
                    pieceState->sceneVariant++;
                    object->trans.y = 2.5f;
                    for (i = 0; i < 7; i++) {
                        Hu3DModelShadowReset(object->mdlId[i]);
                        Hu3DModelShadowMapSet(object->mdlId[i]);
                    }
                }
                else {
                    pieceState->animationProgress += doorOpenAmount;
                    if (pieceState->animationProgress > 90.0f) {
                        pieceState->isActiveFrame = 1;
                        pieceState->animationProgress = 90.0f;
                        HuAudFXPlay(0x522);
                    }
                    object->rot.x = -120.0 * cosd(pieceState->animationProgress);
                    object->trans.z = 7.5 * cosd(pieceState->animationProgress) + -500.0;
                }
                break;
            case 3:
                if (scenePieceLimit != 0) {
                    pieceState->sceneVariant++;
                    object->trans.y = 0.0f;
                }
                break;
            case 4:
                if (scenePieceLimit != 0) {
                    pieceState->sceneVariant = 0;
                    object->trans.y = 5.0f;
                    object->trans.z = -497.5f;
                    object->rot.x = -120.0f;
                    for (i = 0; i < 7; i++) {
                        SetPieceModelVisibility(object->mdlId[i]);
                    }
                    AssignPuzzlePieceModels(object);
                }
                break;
        }
    }
    UpdatePuzzlePieceModels(object);
}

static void ReleasePuzzleSceneResources(void)
{
    OMOBJ *pieceObject;
    PuzzleSceneWork *pieceState;
    s32 i;
    s32 j;

    for (i = 0; i < 5; i++) {
        pieceObject = puzzlePieceObjects[i];
        pieceState = pieceObject->data;
        if (pieceState != NULL) {
            for (j = 0; j < 12; j++) {
                if (pieceState->pieceModelIds[j] >= 0) {
                    Hu3DModelKill(pieceState->pieceModelIds[j]);
                }
            }
            HuMemDirectFree(pieceState);
            puzzlePieceObjects[i]->data = NULL;
        }
        for (i = 0; i < pieceObject->mdlcnt; i++) {
            pieceObject->mdlId[i] = -1;
        }
    }
}

static void SetPieceModelVisibility(s16 state)
{
    s16 i;
    HSFDATA *modelFileData;
    HSFOBJECT *modelObject;
    HSFOBJECT *currentObject;
    HSFCONSTDATA *objectConstants;

    modelFileData = Hu3DData[state].hsf;
    modelObject = modelFileData->object;
    for (i = 0; i < modelFileData->objectNum; i++, modelObject++) {
        currentObject = modelObject;
        if (currentObject->constData) {
            objectConstants = currentObject->constData;
            objectConstants->attr &= ~8;
        }
    }
}

static s32 GetGridCellIndex(float value, float value2)
{
    s32 columnIndex;
    s32 rowIndex;

    columnIndex = (value + 600.0f) / 400.0f;
    rowIndex = (value2 + 400.0f) / 400.0f;
    if (columnIndex < 0) {
        columnIndex = 0;
    }
    else if (columnIndex > 2) {
        columnIndex = 2;
    }
    if (rowIndex < 0) {
        rowIndex = 0;
    }
    else if (rowIndex > 1) {
        rowIndex = 1;
    }
    return columnIndex + rowIndex * 3;
}

static void GetGridCellPosition(s32 index, Vec *position)
{
    position->x = gridCellOrigins[index].x;
    position->y = gridCellOrigins[index].y;
    position->z = gridCellOrigins[index].z;
}

static s32 GetPieceAssignment(PuzzleSceneWork *sceneWork, s32 index)
{
    return sceneWork->pieceAssignments[index];
}

void SetSceneSpeed(float value)
{
    activePieceCount = 1;
    doorOpenAmount = value;
}

void SetScenePieceCount(s32 index)
{
    maximumPieceCount = index;
}

void ResetSceneState(void)
{
    float motionSpeed;

    selectedPieceIndex = 1;
    motionSpeed = (completedPieceCount == 0) ? 1.6666666f : -1.6666666f;
    Hu3DMotionSpeedSet(doorObject->mdlId[2], motionSpeed);
    HuAudFXPlay(0x524);
}

static s32 GetPieceWorldPositions(float value, float value2, Vec *position)
{
    Mtx pieceTransform;
    Mtx localTransform;
    Vec pieceOrigin;
    Vec *pieceVertex;
    s32 pieceIndex;
    s32 gridIndex;
    s32 vertexCount;
    OMOBJ *pieceObject;
    PuzzleSceneWork *pieceState;
    s32 i;

    pieceObject = puzzlePieceObjects[sceneTimer];
    pieceState = pieceObject->data;
    gridIndex = GetGridCellIndex(value, value2);
    pieceIndex = GetPieceAssignment(pieceState, gridIndex);
    if (pieceIndex < 6) {
        return 0;
    }
    pieceIndex -= 6;
    GetGridCellPosition(gridIndex, &pieceOrigin);
    pieceOrigin = gridCellOrigins[gridIndex];
    MTXTrans(pieceTransform, pieceObject->trans.x, pieceObject->trans.y, pieceObject->trans.z);
    mtxRot(localTransform, pieceObject->rot.x, pieceObject->rot.y, pieceObject->rot.z);
    MTXConcat(pieceTransform, localTransform, pieceTransform);
    MTXTrans(localTransform, pieceOrigin.x, pieceOrigin.y, pieceOrigin.z);
    MTXConcat(pieceTransform, localTransform, pieceTransform);
    pieceVertex = pieceCirclePositionTables[pieceIndex];
    vertexCount = pieceCircleVertexCounts[pieceIndex];
    for (i = 0; i < vertexCount; i++) {
        MTXTrans(localTransform, pieceVertex->x, pieceVertex->y, pieceVertex->z);
        MTXConcat(pieceTransform, localTransform, localTransform);
        position->x = localTransform[0][3];
        position->y = localTransform[1][3];
        position->z = localTransform[2][3];
        position++;
        pieceVertex++;
    }
    return vertexCount;
}

float GetDistanceToPiece(float value, float value2)
{
    float verticalDistance;
    float projectedHeight;
    OMOBJ *pieceObject;

    pieceObject = puzzlePieceObjects[sceneTimer];
    verticalDistance = -(value2 - pieceObject->trans.z);
    if (cosd(pieceObject->rot.x) <= 0.0) {
        projectedHeight = 9999.0f;
    }
    else {
        projectedHeight = verticalDistance * sind(pieceObject->rot.x) / cosd(pieceObject->rot.x);
        if (projectedHeight > 9999.0f) {
            projectedHeight = 9999.0f;
        }
    }
    return projectedHeight;
}

float GetDistanceToPieceFromPosition(Vec *position, float value)
{
    return GetDistanceToPiece(0.0f, position->z - value);
}

static s32 GetScenePieceCount(void)
{
    return selectedPieceIndex;
}

s32 GetScenePieceCountPublic(void)
{
    return sceneSpeed;
}

static s32 FindCompletedPieces(PuzzleSceneWork *sceneWork, s32 *indices)
{
    s32 pieceCount;
    s32 i;

    pieceCount = 0;
    for (i = 0; i < 6; i++) {
        if (sceneWork->pieceAssignments[i] >= 6) {
            indices[pieceCount++] = i;
        }
    }
    return pieceCount;
}

s32 GetPieceCenterPositions(Vec *position)
{
    OMOBJ *pieceObject;
    PuzzleSceneWork *pieceState;
    s32 availablePieceIndices[6];
    s32 availablePieceCount;
    s32 pieceIndex;
    s32 i;

    pieceObject = puzzlePieceObjects[sceneTimer];
    pieceState = pieceObject->data;
    availablePieceCount = FindCompletedPieces(pieceState, availablePieceIndices);
    for (i = 0; i < availablePieceCount; i++) {
        GetGridCellPosition(availablePieceIndices[i], position);
        pieceIndex = GetPieceAssignment(pieceState, availablePieceIndices[i]) - 6;
        position->x += pieceObject->trans.x + pieceModelOffsets[pieceIndex].x;
        position->y += pieceObject->trans.y + pieceModelOffsets[pieceIndex].y;
        position->z += pieceObject->trans.z + pieceModelOffsets[pieceIndex].z;
        position++;
    }
    return availablePieceCount;
}

s32 GetPieceTransformedPositions(Vec *position)
{
    Mtx pieceTransform;
    Mtx localTransform;
    OMOBJ *pieceObject;
    PuzzleSceneWork *pieceState;
    s32 availablePieceIndices[6];
    s32 pieceIndex;
    s32 availablePieceCount;
    s32 i;

    pieceObject = puzzlePieceObjects[sceneTimer];
    pieceState = pieceObject->data;
    availablePieceCount = FindCompletedPieces(pieceState, availablePieceIndices);
    MTXTrans(pieceTransform, pieceObject->trans.x, pieceObject->trans.y, pieceObject->trans.z);
    mtxRot(localTransform, pieceObject->rot.x, pieceObject->rot.y, pieceObject->rot.z);
    MTXConcat(pieceTransform, localTransform, pieceTransform);
    for (i = 0; i < availablePieceCount; i++) {
        GetGridCellPosition(availablePieceIndices[i], position);
        pieceIndex = GetPieceAssignment(pieceState, availablePieceIndices[i]) - 6;
        position->x += pieceModelOffsets[pieceIndex].x;
        position->y += pieceModelOffsets[pieceIndex].y;
        position->z += pieceModelOffsets[pieceIndex].z;
        MTXTrans(localTransform, position->x, position->y, position->z);
        MTXConcat(pieceTransform, localTransform, localTransform);
        position->x = localTransform[0][3];
        position->y = localTransform[1][3];
        position->z = localTransform[2][3];
        position++;
    }
    return availablePieceCount;
}

void SetActivePieceCount(s32 index)
{
    if (index > maximumPieceCount) {
        index = maximumPieceCount;
    }
    currentPieceCount = index;
}

s32 FindPieceAtPosition(Vec *position, Vec *position2)
{
    Vec cellOrigin;
    Vec pointInPieceSpace;
    Vec projectedPoint;
    Vec edgeVector;
    Vec adjacentEdgeVector;
    Vec crossProduct;
    float edgeInterpolation;
    float firstEdgeFraction;
    float secondEdgeFraction;
    float firstEdgeDistance;
    float secondEdgeDistance;
    float distanceBlend;
    s32 gridCellIndex;
    s32 edgeCount;
    s32 pieceAssignment;
    s32 nextEdgeIndex;
    s32 intersectionCount;
    Vec *lowerOutline;
    Vec *upperOutline;
    OMOBJ *pieceObject;
    PuzzleSceneWork *pieceState;
    s32 i;

    pieceObject = puzzlePieceObjects[sceneTimer];
    pieceState = pieceObject->data;
    if (pieceObject->rot.x > 0.0f || pieceObject->rot.x <= -90.0f) {
        return 0;
    }
    VECSubtract(position, &pieceObject->trans, &pointInPieceSpace);
    pointInPieceSpace.z /= cosd(pieceObject->rot.x);
    gridCellIndex = GetGridCellIndex(position->x, position->z);
    pieceAssignment = GetPieceAssignment(pieceState, gridCellIndex);
    if (pieceAssignment < 6) {
        return 0;
    }
    pieceAssignment -= 6;
    cellOrigin = gridCellOrigins[gridCellIndex];
    VECSubtract(&pointInPieceSpace, &cellOrigin, &pointInPieceSpace);
    upperOutline = pieceLayoutPositionTables[pieceAssignment][0];
    lowerOutline = pieceLayoutPositionTables[pieceAssignment][1];
    edgeCount = pieceLayoutVertexCounts[pieceAssignment];
    for (i = 0; i < edgeCount; i++) {
        nextEdgeIndex = (i + 1) % edgeCount;
        VECSubtract(&upperOutline[nextEdgeIndex], &upperOutline[i], &edgeVector);
        VECSubtract(&pointInPieceSpace, &upperOutline[nextEdgeIndex], &adjacentEdgeVector);
        VECCrossProduct(&edgeVector, &adjacentEdgeVector, &crossProduct);
        if (crossProduct.y < 0.0f) {
            continue;
        }
        VECSubtract(&lowerOutline[nextEdgeIndex], &upperOutline[nextEdgeIndex], &edgeVector);
        VECSubtract(&pointInPieceSpace, &lowerOutline[nextEdgeIndex], &adjacentEdgeVector);
        VECCrossProduct(&edgeVector, &adjacentEdgeVector, &crossProduct);
        if (crossProduct.y < 0.0f) {
            continue;
        }
        VECSubtract(&lowerOutline[i], &lowerOutline[nextEdgeIndex], &edgeVector);
        VECSubtract(&pointInPieceSpace, &lowerOutline[i], &adjacentEdgeVector);
        VECCrossProduct(&edgeVector, &adjacentEdgeVector, &crossProduct);
        if (crossProduct.y < 0.0f) {
            continue;
        }
        VECSubtract(&upperOutline[i], &lowerOutline[i], &edgeVector);
        VECSubtract(&pointInPieceSpace, &upperOutline[i], &adjacentEdgeVector);
        VECCrossProduct(&edgeVector, &adjacentEdgeVector, &crossProduct);
        if (crossProduct.y < 0.0f) {
            continue;
        }
        VECSubtract(&upperOutline[nextEdgeIndex], &upperOutline[i], &edgeVector);
        firstEdgeFraction = (pointInPieceSpace.x * edgeVector.x - edgeVector.x * upperOutline[i].x + pointInPieceSpace.y * edgeVector.y - edgeVector.y * upperOutline[i].y + pointInPieceSpace.z * edgeVector.z - edgeVector.z * upperOutline[i].z)
            / (edgeVector.x * edgeVector.x + edgeVector.y * edgeVector.y + edgeVector.z * edgeVector.z);
        projectedPoint.x = upperOutline[i].x + firstEdgeFraction * edgeVector.x;
        projectedPoint.y = upperOutline[i].y + firstEdgeFraction * edgeVector.y;
        projectedPoint.z = upperOutline[i].z + firstEdgeFraction * edgeVector.z;
        VECSubtract(&projectedPoint, &pointInPieceSpace, &crossProduct);
        firstEdgeDistance = VECMag(&crossProduct);
        VECSubtract(&lowerOutline[nextEdgeIndex], &lowerOutline[i], &adjacentEdgeVector);
        secondEdgeFraction = (pointInPieceSpace.x * adjacentEdgeVector.x - adjacentEdgeVector.x * lowerOutline[i].x + pointInPieceSpace.y * adjacentEdgeVector.y - adjacentEdgeVector.y * lowerOutline[i].y + pointInPieceSpace.z * adjacentEdgeVector.z - adjacentEdgeVector.z * lowerOutline[i].z)
            / (adjacentEdgeVector.x * adjacentEdgeVector.x + adjacentEdgeVector.y * adjacentEdgeVector.y + adjacentEdgeVector.z * adjacentEdgeVector.z);
        projectedPoint.x = lowerOutline[i].x + secondEdgeFraction * adjacentEdgeVector.x;
        projectedPoint.y = lowerOutline[i].y + secondEdgeFraction * adjacentEdgeVector.y;
        projectedPoint.z = lowerOutline[i].z + secondEdgeFraction * adjacentEdgeVector.z;
        VECSubtract(&projectedPoint, &pointInPieceSpace, &crossProduct);
        secondEdgeDistance = VECMag(&crossProduct);
        if (firstEdgeDistance == 0.0f) {
            edgeInterpolation = firstEdgeFraction;
        }
        else if (secondEdgeDistance == 0.0f) {
            edgeInterpolation = secondEdgeFraction;
        }
        else {
            distanceBlend = 1.0f - firstEdgeDistance / (firstEdgeDistance + secondEdgeDistance);
            edgeInterpolation = firstEdgeFraction * distanceBlend + secondEdgeFraction * (1.0f - distanceBlend);
        }
        if (edgeInterpolation < 0.0f) {
            edgeInterpolation = 0.0f;
        }
        else if (edgeInterpolation > 1.0f) {
            edgeInterpolation = 1.0f;
        }
        pointInPieceSpace.x = lowerOutline[i].x + edgeInterpolation * adjacentEdgeVector.x;
        pointInPieceSpace.y = lowerOutline[i].y + edgeInterpolation * adjacentEdgeVector.y;
        pointInPieceSpace.z = lowerOutline[i].z + edgeInterpolation * adjacentEdgeVector.z;
        position2->x = pieceObject->trans.x + cellOrigin.x + pointInPieceSpace.x;
        position2->y = pieceObject->trans.y + cellOrigin.y + pointInPieceSpace.y;
        position2->z = pieceObject->trans.z + (cellOrigin.z + pointInPieceSpace.z) * cosd(pieceObject->rot.x);
        return 1;
    }
    intersectionCount = 0;
    for (i = 0; i < edgeCount; i++) {
        nextEdgeIndex = (i + 1) % edgeCount;
        edgeInterpolation = (pointInPieceSpace.z - upperOutline[i].z) / (upperOutline[nextEdgeIndex].z - upperOutline[i].z);
        crossProduct.x = upperOutline[i].x + edgeInterpolation * (upperOutline[nextEdgeIndex].x - upperOutline[i].x);
        if (edgeInterpolation >= 0.0f && edgeInterpolation < 1.0f && crossProduct.x > pointInPieceSpace.x) {
            intersectionCount++;
        }
    }
    if (intersectionCount & 1) {
        position2->x = position->x;
        position2->y = position->y;
        position2->z = position->z;
        return 1;
    }
    return 0;
}
