#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXVert.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/hsfformat.h"
#include "game/hu3d.h"
#include "game/init.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "string.h"

#include "REL/m428Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

typedef struct M428DllCameraWork {
    u8 cameraTransitionActive;
    Vec cameraCenter;
    Vec cameraRotation;
    Vec startCameraCenter;
    Vec startCameraRotation;
    Vec targetCameraCenter;
    Vec targetCameraRotation;
    Vec centerControlPoint;
    Vec rotationControlPoint;
    Vec centerBezierPoints[3];
    Vec rotationBezierPoints[3];
    Vec rotationControlPoints[3];
    Vec rotationBezierControls[3];
    Float centerBezierSlopes[3];
    Float rotationBezierSlopes[3];
    Float zoomControlPoints[3];
    Float zoomBezierSlopes[3];
    Float zoomCumulativeDistances[3];
    Float cameraZoom;
    Float startCameraZoom;
    Float targetCameraZoom;
    Float zoomControlPoint;
    Float transitionSpeed;
    Float transitionDuration;
    Float transitionFramesRemaining;
    Float shakeAmplitude[3];
    Float shakeDecay[3];
    Float shakePhase[3];
    Float shakeAngularSpeed[3];
    Float cameraShakeOffset[3];
    u16 shakeFramesRemaining[3];
} M428DllCameraWork; /* size = 0x190 */

typedef struct M428DllParticleBounds {
    s16 particleStatePadding;
    s16 particleFlagPadding;
    s16 emissionState;
    s16 emissionCooldown;
    Float maxX;
    Float minX;
    Float maxY;
    Float minY;
    Float maxZ;
    Float minZ;
    char particleBoundsPadding[0x28];
} M428DllParticleBounds; /* size = 0x48 */

typedef struct M428DllCollisionFace {
    Vec faceCenter;
    s32 vertexIndices[3];
    s8 faceMaterialAttributes[4];
    s8 vertexCount;
    Float boundingRadius;
    Float planeNormalX;
    Float planeNormalY;
    Float planeNormalZ;
    Float planeOffset;
} M428DllCollisionFace; /* size = 0x34 */

typedef struct M428DllCollisionMesh {
    Vec *vertices;
    s32 vertexCount;
    s32 faceCount;
    M428DllCollisionFace *faces;
} M428DllCollisionMesh; /* size 0x10 */

typedef struct M428DllCollisionData {
    s32 faceCount;
    M428DllCollisionMesh *faces;
} M428DllCollisionData; /* size = 0x08 */

void InitializeCameraObject(OMOBJ *object);
void UpdateCameraObject(OMOBJ *object);
void InitializeMapObject(OMOBJ *object);
void UpdateMapObject(OMOBJ *object);
u8 FindMeshFaceAtPosition(M428DllCollisionMesh *collisionMesh, Vec *position);
void LoadMapCollisionMesh(s32 modelId, M428DllCollisionData *collisionData);
void ExtractCollisionMeshFromObject(HSFDATA *modelData, HSFOBJECT *object);
void UpdateMapParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
s16 CreateMapParticleHook(s32 animationDataId, s16 particleCount, HU3DPARTICLEHOOK particleHook);
void InitializeMapHeap(void);
void *AllocateMapHeap(s32 allocationSize);
void FreeCollisionMeshFaces(M428DllCollisionMesh *allocation);
void DestroyMapHeap(void);
s32 GetMapRandomValue(void);
void CalculateBezierPathDistances(Float *pathDistances, Float *controlValues, Float *secondDerivatives, s32 pointCount);
Float EvaluateBezierPath(Float pathProgress, Float *pathDistances, Float *controlValues, Float *secondDerivatives, s32 pointCount);
void calculateBezierPathSlopes(Float *pathDistances, Float (*controlPoints)[3], Float (*secondDerivatives)[3], s32 pointCount);
void interpolateBezierPath(Float pathProgress, Float *outputValues, Float *pathDistances, Float (*controlPoints)[3], Float (*secondDerivatives)[3], s32 pointCount);

M428DllCollisionData *mapMeshWork;
M428DllCollisionMesh *currentMeshWork;
void *heapAllocations[0x20];
OMOBJ *multiviewObject;
OMOBJ *mapObject;
OMOBJ *cameraObjects[2];
M428DllCollisionData mapCollisionData;

void InitializeMapAndCameras(HUPROCESS *processManager)
{
    s32 cameraIndex;
    OMOBJ *cameraObject;
    s32 randomWarmupCount;

    randomWarmupCount = frand() & 0x1F;
    for (cameraIndex = 0; cameraIndex < randomWarmupCount; cameraIndex++) {
        GetMapRandomValue();
    }
    InitializeMapHeap();
    Hu3DCameraCreate(3);
    Hu3DCameraPerspectiveSet(1, 45.0f, 100.0f, 7500.0f, 0.6f);
    Hu3DCameraPerspectiveSet(2, 45.0f, 100.0f, 7500.0f, 0.6f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 320.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraViewportSet(2, 320.0f, 0.0f, 320.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 0x140, 0x1E0);
    Hu3DCameraScissorSet(2, 0x140, 0, 0x140, 0x1E0);
    multiviewObject = omAddObjEx(processManager, 0x7FDA, 0, 0, -1, omOutViewMulti);
    multiviewObject->work[0] = 2;
    for (cameraIndex = 0; cameraIndex < 2; cameraIndex++) {
        cameraObject = omAddObjEx(processManager, 0xFE, 1, 0, -1, InitializeCameraObject);
        cameraObjects[cameraIndex] = cameraObject;
        cameraObject->work[0] = cameraIndex;
    }
    mapObject = omAddObjEx(processManager, 0x7F, 9, 0, -1, InitializeMapObject);
}

void DestroyMapAndCameras(void)
{
    DestroyMapHeap();
}

void InitializeCameraObject(OMOBJ *object)
{
    M428DllCameraWork *cameraWork;

    cameraWork = AllocateMapHeap(sizeof(M428DllCameraWork));
    object->data = cameraWork;
    cameraWork->transitionFramesRemaining = cameraWork->transitionDuration = 0.0f;
    cameraWork->transitionSpeed = 1.0f;
    cameraWork->cameraRotation.x = cameraWork->startCameraRotation.x = 0.0f;
    cameraWork->cameraRotation.y = cameraWork->startCameraRotation.y = 0.0f;
    cameraWork->cameraRotation.z = cameraWork->startCameraRotation.z = 0.0f;
    cameraWork->cameraCenter.x = cameraWork->startCameraCenter.x = 0.0f;
    cameraWork->cameraCenter.y = cameraWork->startCameraCenter.y = 80.0f;
    cameraWork->cameraCenter.z = cameraWork->startCameraCenter.z = 0.0f;
    cameraWork->cameraZoom = cameraWork->startCameraZoom = 600.0f;
    cameraWork->shakeFramesRemaining[0] = cameraWork->shakeFramesRemaining[1] = cameraWork->shakeFramesRemaining[2] = 0;
    object->objFunc = UpdateCameraObject;
}

void UpdateCameraObject(OMOBJ *object)
{
    Float transitionProgress;
    M428DllCameraWork *cameraWork;
    s32 shakeAxis;

    cameraWork = object->data;
    if (cameraWork->transitionFramesRemaining > 0.0f) {
        cameraWork->transitionFramesRemaining = cameraWork->transitionFramesRemaining - cameraWork->transitionSpeed;
        transitionProgress = (cameraWork->transitionDuration - cameraWork->transitionFramesRemaining) / cameraWork->transitionDuration;
        transitionProgress = sind((90.0f * transitionProgress));
        if (cameraWork->cameraTransitionActive == 0) {
            cameraWork->cameraCenter.x = cameraWork->startCameraCenter.x + (transitionProgress * (cameraWork->targetCameraCenter.x - cameraWork->startCameraCenter.x));
            cameraWork->cameraCenter.y = cameraWork->startCameraCenter.y + (transitionProgress * (cameraWork->targetCameraCenter.y - cameraWork->startCameraCenter.y));
            cameraWork->cameraCenter.z = cameraWork->startCameraCenter.z + (transitionProgress * (cameraWork->targetCameraCenter.z - cameraWork->startCameraCenter.z));
            cameraWork->cameraRotation.x = cameraWork->startCameraRotation.x + (transitionProgress * (cameraWork->targetCameraRotation.x - cameraWork->startCameraRotation.x));
            cameraWork->cameraRotation.y = cameraWork->startCameraRotation.y + (transitionProgress * (cameraWork->targetCameraRotation.y - cameraWork->startCameraRotation.y));
            cameraWork->cameraRotation.z = cameraWork->startCameraRotation.z + (transitionProgress * (cameraWork->targetCameraRotation.z - cameraWork->startCameraRotation.z));
            cameraWork->cameraZoom = cameraWork->startCameraZoom + (transitionProgress * (cameraWork->targetCameraZoom - cameraWork->startCameraZoom));
        }
        else {
            interpolateBezierPath(2.0f * transitionProgress, (Float *)&cameraWork->cameraCenter, cameraWork->centerBezierSlopes, (Float(*)[3])cameraWork->centerBezierPoints, (Float(*)[3])cameraWork->rotationBezierPoints, 3);
            interpolateBezierPath(2.0f * transitionProgress, (Float *)&cameraWork->cameraRotation, cameraWork->rotationBezierSlopes, (Float(*)[3])cameraWork->rotationControlPoints, (Float(*)[3])cameraWork->rotationBezierControls, 3);
            cameraWork->cameraZoom = EvaluateBezierPath(2.0f * transitionProgress, cameraWork->zoomCumulativeDistances, cameraWork->zoomControlPoints, cameraWork->zoomBezierSlopes, 3);
        }
    }
    CRotM[object->cameraWork[0]].x = cameraWork->cameraRotation.x;
    CRotM[object->cameraWork[0]].y = cameraWork->cameraRotation.y;
    CRotM[object->cameraWork[0]].z = cameraWork->cameraRotation.z;
    CenterM[object->cameraWork[0]].x = cameraWork->cameraCenter.x;
    CenterM[object->cameraWork[0]].y = cameraWork->cameraCenter.y;
    CenterM[object->cameraWork[0]].z = cameraWork->cameraCenter.z;
    CZoomM[object->cameraWork[0]] = cameraWork->cameraZoom;
    for (shakeAxis = 0; shakeAxis < 3; shakeAxis++) {
        cameraWork->cameraShakeOffset[shakeAxis] = 0.0f;
        if (cameraWork->shakeFramesRemaining[shakeAxis] != 0) {
            cameraWork->cameraShakeOffset[shakeAxis] = cameraWork->shakeAmplitude[shakeAxis] * sind(cameraWork->shakePhase[shakeAxis]);
            cameraWork->shakePhase[shakeAxis] += cameraWork->shakeAngularSpeed[shakeAxis];
            cameraWork->shakeAmplitude[shakeAxis] *= cameraWork->shakeDecay[shakeAxis];
            cameraWork->shakeFramesRemaining[shakeAxis] -= 1;
        }
    }
    CenterM[object->cameraWork[0]].x += cameraWork->cameraShakeOffset[0];
    CenterM[object->cameraWork[0]].y += cameraWork->cameraShakeOffset[1];
    CenterM[object->cameraWork[0]].z += cameraWork->cameraShakeOffset[2];
}

void StartCameraTransition(u8 cameraIndex, Float transitionFrames, Vec *targetCenter, Vec *targetRotation, Float targetZoom, Vec *centerControlPoint, Vec *rotationControlPoint, Float zoomControlPoint)
{
    Float firstZoomDistance;
    Float secondZoomDistance;
    M428DllCameraWork *cameraWork;

    cameraWork = cameraObjects[cameraIndex]->data;
    cameraWork->cameraTransitionActive = 1;
    cameraWork->transitionFramesRemaining = cameraWork->transitionDuration = transitionFrames;
    cameraWork->startCameraCenter = cameraWork->cameraCenter;
    cameraWork->targetCameraCenter.x = targetCenter->x;
    cameraWork->targetCameraCenter.y = targetCenter->y;
    cameraWork->targetCameraCenter.z = targetCenter->z;
    cameraWork->centerControlPoint.x = centerControlPoint->x;
    cameraWork->centerControlPoint.y = centerControlPoint->y;
    cameraWork->centerControlPoint.z = centerControlPoint->z;
    cameraWork->startCameraRotation = cameraWork->cameraRotation;
    cameraWork->targetCameraRotation.x = targetRotation->x;
    cameraWork->targetCameraRotation.y = targetRotation->y;
    cameraWork->targetCameraRotation.z = targetRotation->z;
    cameraWork->rotationControlPoint.x = rotationControlPoint->x;
    cameraWork->rotationControlPoint.y = rotationControlPoint->y;
    cameraWork->rotationControlPoint.z = rotationControlPoint->z;
    cameraWork->startCameraZoom = cameraWork->cameraZoom;
    cameraWork->targetCameraZoom = targetZoom;
    cameraWork->zoomControlPoint = zoomControlPoint;
    cameraWork->centerBezierPoints[0] = cameraWork->startCameraCenter;
    cameraWork->centerBezierPoints[1] = cameraWork->centerControlPoint;
    cameraWork->centerBezierPoints[2] = cameraWork->targetCameraCenter;
    cameraWork->rotationControlPoints[0] = cameraWork->startCameraRotation;
    cameraWork->rotationControlPoints[1] = cameraWork->rotationControlPoint;
    cameraWork->rotationControlPoints[2] = cameraWork->targetCameraRotation;
    cameraWork->zoomControlPoints[0] = cameraWork->startCameraZoom;
    cameraWork->zoomControlPoints[1] = cameraWork->zoomControlPoint;
    cameraWork->zoomControlPoints[2] = cameraWork->targetCameraZoom;
    cameraWork->zoomCumulativeDistances[0] = 0.0f;
    if ((cameraWork->zoomControlPoints[1] - cameraWork->zoomControlPoints[0]) < 0.0f) {
        firstZoomDistance = -(cameraWork->zoomControlPoints[1] - cameraWork->zoomControlPoints[0]);
    }
    else {
        firstZoomDistance = cameraWork->zoomControlPoints[1] - cameraWork->zoomControlPoints[0];
    }
    cameraWork->zoomCumulativeDistances[1] = firstZoomDistance;
    if ((cameraWork->zoomControlPoints[2] - cameraWork->zoomControlPoints[1]) < 0.0f) {
        secondZoomDistance = -(cameraWork->zoomControlPoints[2] - cameraWork->zoomControlPoints[1]);
    }
    else {
        secondZoomDistance = cameraWork->zoomControlPoints[2] - cameraWork->zoomControlPoints[1];
    }
    cameraWork->zoomCumulativeDistances[2] = cameraWork->zoomCumulativeDistances[1] + secondZoomDistance;
    calculateBezierPathSlopes(cameraWork->centerBezierSlopes, (Float(*)[3])cameraWork->centerBezierPoints, (Float(*)[3])cameraWork->rotationBezierPoints, 3);
    calculateBezierPathSlopes(cameraWork->rotationBezierSlopes, (Float(*)[3])cameraWork->rotationControlPoints, (Float(*)[3])cameraWork->rotationBezierControls, 3);
    CalculateBezierPathDistances(cameraWork->zoomCumulativeDistances, cameraWork->zoomControlPoints, cameraWork->zoomBezierSlopes, 3);
}

void SetCameraTransitionSpeed(u8 cameraIndex, Float transitionSpeed)
{
    M428DllCameraWork *cameraWork = cameraObjects[cameraIndex]->data;
    cameraWork->transitionSpeed = transitionSpeed;
}

void StartCameraShake(u8 cameraIndex, u8 shakeAxis, u16 shakeFrames, Float shakeAmplitude, Float shakeDecay, Float shakeAngularSpeed)
{
    M428DllCameraWork *cameraWork = cameraObjects[cameraIndex]->data;
    cameraWork->shakeFramesRemaining[shakeAxis] = shakeFrames;
    cameraWork->shakeAmplitude[shakeAxis] = shakeAmplitude;
    cameraWork->shakeDecay[shakeAxis] = shakeDecay;
    cameraWork->shakeAngularSpeed[shakeAxis] = shakeAngularSpeed;
    cameraWork->shakePhase[shakeAxis] = 90.0f;
}

static char goalShadowModelName[20] = "goal"; // explicitly declared because of padding

void InitializeMapObject(OMOBJ *object)
{
    s32 modelId;
    M428DllParticleBounds *particleBounds;
    s32 cameraIndex;

    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M428, 0));
    object->mdlId[0] = modelId;
    Hu3DModelShadowMapSet(modelId);
    Hu3DModelLayerSet(modelId, 2);
    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M428, 1));
    object->mdlId[1] = modelId;
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    LoadMapCollisionMesh(modelId, &mapCollisionData);
    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M428, 2));
    object->mdlId[2] = modelId;
    Hu3DModelLayerSet(modelId, 2);
    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M428, 3));
    object->mdlId[3] = modelId;
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelShadowMapObjSet(modelId, goalShadowModelName);
    Hu3DModelLayerSet(modelId, 2);
    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M428, 4));
    object->mdlId[4] = modelId;
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_SHAPE_LOOP);
    Hu3DModelLayerSet(modelId, 2);
    Hu3DModelCameraSet(modelId, 1);
    AddModelToCameraShadows(modelId, 1);
    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M428, 5));
    object->mdlId[5] = modelId;
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_SHAPE_LOOP);
    Hu3DModelLayerSet(modelId, 2);
    Hu3DModelCameraSet(modelId, 2);
    AddModelToCameraShadows(modelId, 2);
    for (cameraIndex = 0; cameraIndex < 2; cameraIndex++) {
        modelId = CreateMapParticleHook(DATA_MAKE_NUM(DATADIR_M428, 6), 0x3E8, UpdateMapParticle);
        object->mdlId[cameraIndex + 6] = modelId;
        Hu3DModelLayerSet(modelId, 4);
        Hu3DModelCameraSet(modelId, 1 << cameraIndex);
        particleBounds = ((HU3DPARTICLE *)Hu3DData[modelId].hookData)->work;
        particleBounds->maxX = 400.0f;
        particleBounds->minX = -400.0f;
        particleBounds->maxY = 1300.0f;
        particleBounds->minY = -800.0f;
        particleBounds->maxZ = 700.0f;
        particleBounds->minZ = 150.0f;
    }
    object->objFunc = UpdateMapObject;
}

void UpdateMapObject(OMOBJ *object)
{
    M428DllParticleBounds *particleBounds;
    s32 cameraIndex;

    for (cameraIndex = 0; cameraIndex < 2; cameraIndex++) {
        particleBounds = ((HU3DPARTICLE *)Hu3DData[object->mdlId[cameraIndex + 6]].hookData)->work;
        particleBounds->maxX = 500.0f + CenterM[cameraIndex].x;
        particleBounds->minX = -500.0f + CenterM[cameraIndex].x;
        particleBounds->maxY = 900.0f + CenterM[cameraIndex].y;
        particleBounds->minY = -1200.0f + CenterM[cameraIndex].y;
    }
}

void EmptyMapCallback(OMOBJ *object) { }

u8 FindGroundFaceAtPosition(M428DllCollisionData *collisionData, Vec *position)
{
    u32 faceIndex;
    M428DllCollisionMesh *faceData;
    s32 containsPoint;

    containsPoint = 0;
    for (faceData = collisionData->faces, faceIndex = 0; faceIndex < collisionData->faceCount; faceIndex++, faceData++) {
        if (FindMeshFaceAtPosition(faceData, position) != 0) {
            break;
        }
    }
    if (faceIndex < collisionData->faceCount) {
        containsPoint = 1;
    }
    return containsPoint;
}

u8 FindMeshFaceAtPosition(M428DllCollisionMesh *collisionMesh, Vec *position)
{
    Vec pointToFaceCenter;
    Vec edgeToPoint;
    Vec pointOnGround;
    Float edgeCrossProduct;
    Float distanceToFace;
    M428DllCollisionFace *face;
    s32 edgeIndex;
    u32 faceIndex;
    s32 nextEdgeIndex;
    Vec *currentVertex;
    s32 insideFace;
    Vec *nextVertex;

    face = collisionMesh->faces;
    for (faceIndex = 0; faceIndex < collisionMesh->faceCount; faceIndex++, face++) {
        pointOnGround = *position;
        pointOnGround.y = 0.0f;
        VECSubtract(&pointOnGround, &face->faceCenter, &pointToFaceCenter);
        distanceToFace = VECMag(&pointToFaceCenter);
        if (!(distanceToFace > (400.0f + face->boundingRadius)) && (face->planeNormalZ != 0.0f)) {
            insideFace = 0;
            for (edgeIndex = 0; edgeIndex < 3; edgeIndex++) {
                nextEdgeIndex = edgeIndex + 1;
                if (nextEdgeIndex == 3) {
                    nextEdgeIndex = 0;
                }
                nextVertex = &collisionMesh->vertices[face->vertexIndices[edgeIndex]];
                currentVertex = &collisionMesh->vertices[face->vertexIndices[nextEdgeIndex]];
                VECSubtract(currentVertex, nextVertex, &pointToFaceCenter);
                VECSubtract(position, currentVertex, &edgeToPoint);
                edgeCrossProduct = (pointToFaceCenter.x * edgeToPoint.y) - (pointToFaceCenter.y * edgeToPoint.x);
                if ((edgeCrossProduct * face->planeNormalZ) < 0.0) {
                    insideFace = 1;
                    break;
                }
            }
            if (insideFace == 0) {
                position->z = ((face->planeOffset - (face->planeNormalX * position->x)) - (face->planeNormalY * position->y)) / face->planeNormalZ;
                return 1;
            }
        }
    }
    return 0;
}

void LoadMapCollisionMesh(s32 modelId, M428DllCollisionData *collisionData)
{
    HSFDATA *modelData;
    u32 meshDataSize;

    modelData = Hu3DData[modelId].hsf;
    mapMeshWork = collisionData;
    mapMeshWork->faceCount = 0;
    mapMeshWork->faces = AllocateMapHeap(256 * sizeof(M428DllCollisionMesh));
    currentMeshWork = mapMeshWork->faces;
    ExtractCollisionMeshFromObject(modelData, modelData->root);
    meshDataSize = mapMeshWork->faceCount * sizeof(M428DllCollisionMesh);
    currentMeshWork = mapMeshWork->faces;
    mapMeshWork->faces = AllocateMapHeap(meshDataSize);
    memcpy(mapMeshWork->faces, currentMeshWork, meshDataSize);
    FreeCollisionMeshFaces(currentMeshWork);
}

void ExtractCollisionFacesFromModel(HSFDATA *modelData, HSFOBJECT *meshObject)
{
    Vec vertexOffset;
    Vec nextVertexOffset;
    Float vertexDistance;
    M428DllCollisionFace *faceData;
    HSFFACE *face;
    HSFBUFFER *vertexBuffer;
    s32 vertexIndex;
    s32 faceIndex;
    HSFBUFFER *faceBuffer;

    vertexBuffer = meshObject->mesh.vertex;
    faceBuffer = meshObject->mesh.face;
    currentMeshWork->faceCount = faceBuffer->count;
    currentMeshWork->vertexCount = vertexBuffer->count;
    currentMeshWork->vertices = vertexBuffer->data;
    currentMeshWork->faces = AllocateMapHeap(currentMeshWork->faceCount * sizeof(M428DllCollisionFace));
    face = faceBuffer->data;
    faceData = currentMeshWork->faces;
    for (faceIndex = 0; faceIndex < faceBuffer->count; faceIndex++, face++, faceData++) {
        switch (face->type & 7) {
            case 2:
                faceData->vertexCount = 3;
                faceData->faceCenter.x = faceData->faceCenter.y = faceData->faceCenter.z = 0.0f;
                for (vertexIndex = 0; vertexIndex < 3; vertexIndex++) {
                    faceData->vertexIndices[vertexIndex] = face->indices[vertexIndex][0];
                    faceData->faceCenter.x += ((Vec *)vertexBuffer->data)[face->indices[vertexIndex][0]].x;
                    faceData->faceCenter.y += ((Vec *)vertexBuffer->data)[face->indices[vertexIndex][0]].y;
                    faceData->faceCenter.z += ((Vec *)vertexBuffer->data)[face->indices[vertexIndex][0]].z;
                }
                faceData->faceCenter.x /= 3.0f;
                faceData->faceCenter.y /= 3.0f;
                faceData->faceCenter.z /= 3.0f;
                faceData->boundingRadius = 0.0f;
                for (vertexIndex = 0; vertexIndex < 3; vertexIndex++) {
                    VECSubtract(&((Vec *)vertexBuffer->data)[face->indices[vertexIndex][0]], &faceData->faceCenter, &vertexOffset);
                    vertexDistance = VECMag(&vertexOffset);
                    if (vertexDistance > faceData->boundingRadius) {
                        faceData->boundingRadius = vertexDistance;
                    }
                }
                faceData->faceCenter.y = 0.0f;
                VECSubtract(&((Vec *)vertexBuffer->data)[face->strip.indices[1][0]], &((Vec *)vertexBuffer->data)[face->strip.indices[0][0]], &vertexOffset);
                VECSubtract(&((Vec *)vertexBuffer->data)[face->strip.indices[2][0]], &((Vec *)vertexBuffer->data)[face->strip.indices[0][0]], &nextVertexOffset);
                faceData->planeNormalX = (vertexOffset.y * nextVertexOffset.z) - (vertexOffset.z * nextVertexOffset.y);
                faceData->planeNormalY = (vertexOffset.z * nextVertexOffset.x) - (vertexOffset.x * nextVertexOffset.z);
                faceData->planeNormalZ = (vertexOffset.x * nextVertexOffset.y) - (vertexOffset.y * nextVertexOffset.x);
                faceData->planeOffset = (faceData->planeNormalZ * ((Vec *)vertexBuffer->data)[face->strip.indices[0][0]].z)
                    + ((faceData->planeNormalX * ((Vec *)vertexBuffer->data)[face->strip.indices[0][0]].x)
                        + (faceData->planeNormalY * ((Vec *)vertexBuffer->data)[face->strip.indices[0][0]].y));
                break;
            case 3:
                faceData->vertexCount = 4;
                faceData->faceCenter.x = faceData->faceCenter.y = faceData->faceCenter.z = 0.0f;
                for (vertexIndex = 0; vertexIndex < 4; vertexIndex++) {
                    faceData->vertexIndices[vertexIndex] = face->indices[vertexIndex][0];
                    faceData->faceCenter.x += ((Vec *)vertexBuffer->data)[face->indices[vertexIndex][0]].x;
                    faceData->faceCenter.y += ((Vec *)vertexBuffer->data)[face->indices[vertexIndex][0]].y;
                    faceData->faceCenter.z += ((Vec *)vertexBuffer->data)[face->indices[vertexIndex][0]].z;
                }
                faceData->faceCenter.x *= 0.25f;
                faceData->faceCenter.y *= 0.25f;
                faceData->faceCenter.z *= 0.25f;
                faceData->boundingRadius = 0.0f;
                for (vertexIndex = 0; vertexIndex < 4; vertexIndex++) {
                    VECSubtract(&((Vec *)vertexBuffer->data)[face->indices[vertexIndex][0]], &faceData->faceCenter, &vertexOffset);
                    vertexDistance = VECMag(&vertexOffset);
                    if (vertexDistance > faceData->boundingRadius) {
                        faceData->boundingRadius = vertexDistance;
                    }
                }
                VECSubtract(&((Vec *)vertexBuffer->data)[face->strip.indices[1][0]], &((Vec *)vertexBuffer->data)[face->strip.indices[0][0]], &vertexOffset);
                VECSubtract(&((Vec *)vertexBuffer->data)[face->strip.indices[2][0]], &((Vec *)vertexBuffer->data)[face->strip.indices[1][0]], &nextVertexOffset);
                faceData->planeNormalX = (vertexOffset.y * nextVertexOffset.z) - (vertexOffset.z * nextVertexOffset.y);
                faceData->planeNormalY = (vertexOffset.z * nextVertexOffset.x) - (vertexOffset.x * nextVertexOffset.z);
                faceData->planeNormalZ = (vertexOffset.x * nextVertexOffset.y) - (vertexOffset.y * nextVertexOffset.x);
                faceData->planeOffset = (faceData->planeNormalX * ((Vec *)vertexBuffer->data)[face->strip.indices[0][0]].x)
                    + ((faceData->planeNormalY * ((Vec *)vertexBuffer->data)[face->strip.indices[0][0]].y))
                    + (faceData->planeNormalZ * ((Vec *)vertexBuffer->data)[face->strip.indices[0][0]].z);
                break;
            default:
                OSReport("Error\n");
                break;
        }
    }
    for (faceIndex = 0; faceIndex < meshObject->mesh.childrenCount; faceIndex++) {
        ExtractCollisionMeshFromObject(modelData, meshObject->mesh.children[faceIndex]);
    }
}

void ExtractCollisionMeshFromObject(HSFDATA *modelData, HSFOBJECT *object)
{
    s16 childIndex;

    switch (object->type) {
        case 0:
        case 1:
        case 3:
        case 4:
        case 5:
        case 6:
        case 9:
            for (childIndex = 0; childIndex < object->mesh.childrenCount; childIndex++) {
                ExtractCollisionMeshFromObject(modelData, object->mesh.children[childIndex]);
            }
            break;
        case 2:
            ExtractCollisionFacesFromModel(modelData, object);
            mapMeshWork->faceCount++;
            currentMeshWork++;
            break;
    }
}

void UpdateMapParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    s32 particleCallbackScratch3C;
    s32 particleCallbackScratch38;
    Vec particlePositionOffset;
    Float particleVelocityDelta;
    Float particleBoundsYRange;
    Float particleBoundsXRange;
    Float particleBoundsZRange;
    Float absoluteParticleOffset;
    Float absoluteParticleVelocity;
    Float absoluteVelocityDelta;
    HU3DPARTICLEDATA *particleData;
    M428DllParticleBounds *particleBounds;
    s32 particleIndex;
    s32 activeParticleCount;

    particleBounds = particle->work;
    particleBoundsXRange = particleBounds->maxX - particleBounds->minX;
    particleBoundsYRange = particleBounds->maxY - particleBounds->minY;
    particleBoundsZRange = particleBounds->maxZ - particleBounds->minZ;
    switch (particleBounds->emissionState) {
        case 2:
            break;
        case 0:
            for (particleData = particle->data, particleIndex = 0; particleIndex < (0.3f * particle->maxCnt); particleIndex++, particleData++) {
                particleData->time = 1;
                particleData->pos.x = particleBounds->minX + (particleBoundsXRange * (GetMapRandomValue() / 65536.0f));
                particleData->pos.y = (particleBounds->minY + ((200.0f + particleBoundsYRange) * (GetMapRandomValue() / 65536.0f))) - 100.0f;
                particleData->pos.z = particleBounds->minZ + (particleBoundsZRange * (GetMapRandomValue() / 65536.0f));
                particleData->color.r = particleData->color.g = particleData->color.b = particleData->color.a = 0xFF;
                particleData->scale = 26.0f + (10.0f * (GetMapRandomValue() / 65536.0f));
                particleData->vel.x = 0.0f;
                particleData->vel.y = (-98.0f / REFRESH_RATE_F) * (GetMapRandomValue() / 65536.0f);
                particleData->vel.z = 0.0f;
                particleData->accel.x = particleData->accel.y = particleData->accel.z = 0.0f;
                particleData->zRot = 360.0f * (GetMapRandomValue() / 65536.0f);
                particleData->parManId = 0;
            }
            for (; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->time = 0;
                particleData->color.a = 0;
                particleData->vel.x = 0.0f;
                particleData->vel.y = (-98.0f / REFRESH_RATE_F) * (GetMapRandomValue() / 65536.0f);
                particleData->vel.z = 0.0f;
                particleData->accel.x = particleData->accel.y = particleData->accel.z = 0.0f;
            }
            particle->pos.x = particle->pos.y = particle->pos.z = 0.0f;
            particleBounds->emissionState++;
            break;
        case 1:
            particle->pos.x = particle->pos.y = particle->pos.z = 0.0f;
            particleBounds->emissionCooldown = 0;
            break;
        case 3:
            particle->pos.x *= 0.95f;
            if (particle->pos.x < 0.0f) {
                absoluteParticleOffset = -particle->pos.x;
            }
            else {
                absoluteParticleOffset = particle->pos.x;
            }
            if (absoluteParticleOffset < 0.01f) {
                particleBounds->emissionState = 1;
            }
            break;
    }
    for (particleData = particle->data, activeParticleCount = 0, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        switch (particleData->time) {
            case 0:
                if (((particleIndex > (0.4f * particle->maxCnt)) && (particle->pos.x == 0.0f)) || (activeParticleCount >= (10.0f + (0.75f * particle->pos.x)))) {
                    particleData->scale = 0.01f;
                }
                else {
                    if (((GetMapRandomValue() & 3) == 0) || (particle->pos.x == 0.0f)) {
                        particleData->pos.x = particleBounds->minX + (particleBoundsXRange * (GetMapRandomValue() / 65536.0f));
                        particleData->pos.y = particleBounds->maxY + (100.0f * (GetMapRandomValue() / 65536.0f));
                        particleData->color.a = 7;
                    }
                    else {
                        if (particle->pos.x > 0.0f) {
                            particleData->pos.x = particleBounds->minX - (100.0f * (GetMapRandomValue() / 65536.0f));
                        }
                        else {
                            particleData->pos.x = particleBounds->maxX + (100.0f * (GetMapRandomValue() / 65536.0f));
                        }
                        particleData->pos.y = particleBounds->maxY - (particleBoundsYRange * (GetMapRandomValue() / 65536.0f));
                        particleData->color.a = 0xFF;
                    }
                    particleData->pos.z = particleBounds->minZ + (particleBoundsZRange * (GetMapRandomValue() / 65536.0f));
                    particleData->scale = 28.0f + (10.0f * (GetMapRandomValue() / 65536.0f));
                    particleData->vel.x = 0.0f;
                    particleData->vel.y = (-98.0f / REFRESH_RATE_F) * (GetMapRandomValue() / 65536.0f);
                    particleData->vel.z = 0.0f;
                    particleData->vel.x = 0.25f * particleData->vel.y * ((GetMapRandomValue() - 0x8000) / 32768.0f);
                    particleData->accel.x = particleData->accel.y = particleData->accel.z = 0.0f;
                    particleData->color.r = particleData->color.g = particleData->color.b = 0xFF;
                    particleData->parManId = 0;
                    activeParticleCount++;
                    particleData->time++;
                }
                break;
            case 1:
                particleData->vel.y += (-98.0f / REFRESH_RATE_F);
                particleData->vel.y += 0.19998f * ((98.0f / REFRESH_RATE_F) * (4.81f + (0.005f * particleData->scale)));
                if (particleData->parManId != 0) {
                    particleData->parManId--;
                    particleData->vel.y *= 0.8f;
                    particleData->vel.y += 0.0025f * ((-98.0f / REFRESH_RATE_F) * particleData->scale);
                }
                else if ((GetMapRandomValue() & 0x7F) == 0) {
                    particleData->parManId = (s32)(30.0f * (GetMapRandomValue() / 65536.0f));
                }
                if (particleData->vel.x < 0.0f) {
                    absoluteParticleVelocity = -particleData->vel.x;
                }
                else {
                    absoluteParticleVelocity = particleData->vel.x;
                }
                if (absoluteParticleVelocity < 0.02f) {
                    particleData->accel.x = 0.025f * (particleData->scale * ((-98.0f / REFRESH_RATE_F) * ((GetMapRandomValue() - 0x8000) / 32768.0f)));
                    if (particleData->accel.x > 0.0f) {
                        particleData->vel.x = 0.02f + (0.04f * particleData->accel.x);
                    }
                    else {
                        particleData->vel.x = (0.04f * particleData->accel.x) - 0.02f;
                    }
                }
                else {
                    particleVelocityDelta = particleData->vel.x - particleData->accel.x;
                    particleData->vel.x = particleData->accel.x + (0.98f * particleVelocityDelta);
                    if (particleVelocityDelta < 0.0f) {
                        absoluteVelocityDelta = -particleVelocityDelta;
                    }
                    else {
                        absoluteVelocityDelta = particleVelocityDelta;
                    }
                    if (absoluteVelocityDelta < 0.02f) {
                        particleData->accel.x = 0.0f;
                    }
                }
                if ((particleData->pos.y < (particleBounds->minY - 100.0f)) || (particleData->pos.x > (100.0f + particleBounds->maxX))
                    || (particleData->pos.x < (particleBounds->minX - 100.0f))) {
                    particleData->time = 0;
                }
                if (particleData->pos.y > (100.0f + particleBounds->maxY)) {
                    particleData->pos.y = particleBounds->minY - (100.0f * (GetMapRandomValue() / 65536.0f));
                }
                if (particleData->pos.y < particleBounds->minY) {
                    particleData->color.a *= 0.9f;
                }
                else if (particleData->color.a < 0xFF) {
                    particleData->color.a += 8;
                }
                break;
        }
        VECAdd(&particleData->pos, &particleData->vel, &particleData->pos);
        if (particle->pos.x != 0.0f) {
            particlePositionOffset = particle->pos;
            particlePositionOffset.x *= particleData->scale / 28.0f;
            particlePositionOffset.y = (((u8)particleData->scale & 1) != 0 ? 1.0f : -1.0f) * (particlePositionOffset.x * (0.05f + (s32)(0.2f * (GetMapRandomValue() / 65536.0f))));
            VECAdd(&particleData->pos, &particlePositionOffset, &particleData->pos);
        }
    }
    DCFlushRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

s16 CreateMapParticleHook(s32 animationDataId, s16 particleCount, HU3DPARTICLEHOOK hook)
{
    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particle;
    s32 particleIndex;
    s32 particleModelId;
    M428DllParticleBounds *particleBounds;
    ANIMDATA *animationData;

    animationData = HuSprAnimRead(HuDataReadNum(animationDataId, HU_MEMNUM_OVL));
    particleModelId = Hu3DParticleCreate(animationData, particleCount);
    Hu3DParticleHookSet(particleModelId, hook);
    particle = Hu3DData[particleModelId].hookData;
    particleBounds = AllocateMapHeap(sizeof(M428DllParticleBounds));
    particle->work = particleBounds;
    particleBounds->emissionState = 0;
    particleData = particle->data;
    memset(particleData, 0, particleCount * 0x44);
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particleData++) {
        particleData->cameraBit = -1;
        particleData->color.r = particleData->color.g = particleData->color.b = particleData->color.a = 0xFF;
    }
    return particleModelId;
}

void InitializeMapHeap(void)
{
    s32 allocationIndex;

    for (allocationIndex = 0; allocationIndex < 0x20; allocationIndex++) {
        heapAllocations[allocationIndex] = NULL;
    }
}

void *AllocateMapHeap(s32 allocationSize)
{
    s32 i;

    for (i = 0; i < 0x20; i++) {
        if (!heapAllocations[i]) {
            break;
        }
    }
    if (i == 0x20) {
        return NULL;
    }
    heapAllocations[i] = HuMemDirectMallocNum(HEAP_HEAP, allocationSize, HU_MEMNUM_OVL);
    memset(heapAllocations[i], 0, allocationSize);
    return heapAllocations[i];
}

void FreeCollisionMeshFaces(M428DllCollisionMesh *allocation)
{
    s32 allocationIndex;
    for (allocationIndex = 0; allocationIndex < 0x20; allocationIndex++) {
        if (heapAllocations[allocationIndex] == allocation) {
            break;
        }
    }
    if (allocationIndex != 0x20) {
        HuMemDirectFree(heapAllocations[allocationIndex]);
        heapAllocations[allocationIndex] = NULL;
    }
}

void DestroyMapHeap(void)
{
    s32 allocationIndex;

    for (allocationIndex = 0; allocationIndex < 0x20; allocationIndex++) {
        if (heapAllocations[allocationIndex]) {
            HuMemDirectFree(heapAllocations[allocationIndex]);
            heapAllocations[allocationIndex] = NULL;
        }
    }
}

void SetCameraMatrixFromAngles(s16 modelId, Mtx rotationMatrix)
{
    HU3DMODEL *camera = &Hu3DData[modelId];
    MTXCopy(rotationMatrix, camera->mtx);
}

u32 randomSeed = 0x41C64E6D;

s32 GetMapRandomValue(void)
{
    randomSeed *= 0x41C64E6D;
    randomSeed += 0x3039;
    return randomSeed >> 0x10;
}

void CalculateCumulativeDistances(s32 itemCount, s32 *shuffledIndices)
{
    s32 shuffleIndex;
    s32 swapIndex;
    s32 randomValue;
    s32 randomIndex;
    s32 randomSample;

    for (shuffleIndex = 0; shuffleIndex < itemCount; shuffleIndex++) {
        shuffledIndices[shuffleIndex] = shuffleIndex;
    }
    for (shuffleIndex = 0; shuffleIndex < (itemCount - 1); shuffleIndex++) {
        randomSeed *= 0x41C64E6D;
        randomSeed += 0x3039;
        randomIndex = randomSeed >> 0x10;
        randomSample = randomIndex;
        swapIndex = itemCount * (randomSample / 65536.0f);
        randomValue = shuffledIndices[shuffleIndex];
        shuffledIndices[shuffleIndex] = shuffledIndices[swapIndex];
        shuffledIndices[swapIndex] = randomValue;
    }
}

Float CalculateBezierSegmentLength(Float startAngle, Float endAngle, Float progress)
{
    Float angleDifference;

    if (startAngle > 180.0f) {
        startAngle -= 360.0f;
    }
    else if (startAngle <= -180.0f) {
        startAngle += 360.0f;
    }
    if (endAngle > 180.0f) {
        endAngle -= 360.0f;
    }
    else if (endAngle <= -180.0f) {
        endAngle += 360.0f;
    }
    angleDifference = startAngle - endAngle;
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    else if (angleDifference <= -180.0f) {
        angleDifference += 360.0f;
    }
    startAngle = endAngle + (angleDifference * progress);
    if (startAngle > 180.0f) {
        return startAngle - 360.0f;
    }
    if (startAngle <= -180.0f) {
        startAngle += 360.0f;
    }
    return startAngle;
}

void RotateMatrixByAngles(Mtx rotationMatrix, Float xRotation, Float yRotation, Float zRotation)
{
    Mtx xRotationMatrix;
    Mtx yRotationMatrix;

    if (zRotation != 0.0f) {
        MTXRotRad(rotationMatrix, 0x5A, MTXDegToRad(zRotation));
    }
    else {
        MTXIdentity(rotationMatrix);
    }
    if (xRotation != 0.0f) {
        MTXRotRad(xRotationMatrix, 0x58, MTXDegToRad(xRotation));
        MTXConcat(xRotationMatrix, rotationMatrix, rotationMatrix);
    }
    if (yRotation != 0.0f) {
        MTXRotRad(yRotationMatrix, 0x59, MTXDegToRad(yRotation));
        MTXConcat(yRotationMatrix, rotationMatrix, rotationMatrix);
    }
}

void SetMatrixForwardVector(Mtx rotationMatrix, Vec *eulerAngles)
{
    Float sinePitch;
    Float cosinePitch;

    eulerAngles->x = atan2d(rotationMatrix[1][2], rotationMatrix[2][2]);
    if (eulerAngles->x < 0.0f) {
        eulerAngles->x += 360.0f;
    }
    eulerAngles->z = atan2d(rotationMatrix[0][1], rotationMatrix[0][0]);
    if (eulerAngles->z < 0.0f) {
        eulerAngles->z += 360.0f;
    }
    sinePitch = -rotationMatrix[0][2];
    cosinePitch = sqrtf((1.0 - (sinePitch * sinePitch)) < 0.0 ? (-(1.0 - (sinePitch * sinePitch))) : (1.0 - (sinePitch * sinePitch)));
    if ((eulerAngles->x > 90.0f) && (eulerAngles->x < 270.0f) && (eulerAngles->z > 90.0f) && (eulerAngles->z < 270.0f)) {
        eulerAngles->x = fmod(180.0f + eulerAngles->x, 360.0);
        eulerAngles->z = fmod(180.0f + eulerAngles->z, 360.0);
        cosinePitch = -cosinePitch;
    }
    eulerAngles->y = atan2d(sinePitch, cosinePitch);
    if (eulerAngles->y < 0.0f) {
        eulerAngles->y += 360.0f;
    }
}

void SetMatrixTranslation(Mtx rotationMatrix, Vec *eulerAngles)
{
    Float sineRoll;
    Float cosineRoll;

    eulerAngles->z = atan2d(rotationMatrix[0][1], rotationMatrix[1][1]);
    if (eulerAngles->z < 0.0f) {
        eulerAngles->z += 360.0f;
    }
    eulerAngles->y = atan2d(rotationMatrix[0][2], rotationMatrix[2][2]);
    if (eulerAngles->y < 0.0f) {
        eulerAngles->y += 360.0f;
    }
    sineRoll = -rotationMatrix[2][1];
    cosineRoll = sqrtf((1.0 - (sineRoll * sineRoll)) < 0.0 ? (-(1.0 - (sineRoll * sineRoll))) : (1.0 - (sineRoll * sineRoll)));
    eulerAngles->x = atan2d(sineRoll, cosineRoll);
    if (eulerAngles->x < 0.0f) {
        eulerAngles->x += 360.0f;
    }
}

Float GetVectorLength(Vec *vector)
{
    Float vectorLength = (vector->z * vector->z) + ((vector->x * vector->x) + (vector->y * vector->y));
    vectorLength = sqrtf(vectorLength);
    if (vectorLength != 0.0f) {
        Float inverseVectorLength = 1.0f / vectorLength;
        vector->x = vector->x * inverseVectorLength;
        vector->y = vector->y * inverseVectorLength;
        vector->z = vector->z * inverseVectorLength;
    }
    else {
        vector->x = vector->y = vector->z = 0.0f;
    }
    return vectorLength;
}

void ProjectPointToLine(Vec lineStart, Vec lineVector, Vec *projectedPoint, Float lineProgress)
{
    if (lineProgress <= 0.0f) {
        projectedPoint->x = lineStart.x;
        projectedPoint->y = lineStart.y;
        projectedPoint->z = lineStart.z;
        lineProgress = 0.0f;
    }
    else if (lineProgress >= 1.0f) {
        projectedPoint->x = lineStart.x + lineVector.x;
        projectedPoint->y = lineStart.y + lineVector.y;
        projectedPoint->z = lineStart.z + lineVector.z;
        lineProgress = 1.0f;
    }
    else {
        projectedPoint->x = lineStart.x + (lineProgress * lineVector.x);
        projectedPoint->y = lineStart.y + (lineProgress * lineVector.y);
        projectedPoint->z = lineStart.z + (lineProgress * lineVector.z);
    }
}

Float DistanceBetweenPoints(Vec lineStart, Vec point, Vec lineDirection)
{
    Float projectionDistance;
    Float negativeDirectionLengthSquared;

    projectionDistance = lineDirection.x * (point.x - lineStart.x) + lineDirection.y * (point.y - lineStart.y) + lineDirection.z * (point.z - lineStart.z);
    negativeDirectionLengthSquared = -(lineDirection.x * lineDirection.x + lineDirection.y * lineDirection.y + lineDirection.z * lineDirection.z);
    if (negativeDirectionLengthSquared != 0.0f) {
        projectionDistance /= negativeDirectionLengthSquared;
    }
    return projectionDistance;
}

Float DistancePointToSegment(Vec *point, Vec *segmentStart, Vec *segmentVector)
{
    Vec closestPoint;
    Float segmentProgress;

    if (segmentVector->x * segmentVector->x + segmentVector->y * segmentVector->y + segmentVector->z * segmentVector->z == 0.0f) {
        return (point->x - segmentStart->x) * (point->x - segmentStart->x) + (point->y - segmentStart->y) * (point->y - segmentStart->y) + (point->z - segmentStart->z) * (point->z - segmentStart->z);
    }
    segmentProgress = DistanceBetweenPoints(*point, *segmentStart, *segmentVector);
    ProjectPointToLine(*segmentStart, *segmentVector, &closestPoint, segmentProgress);
    return (point->x - closestPoint.x) * (point->x - closestPoint.x) + (point->y - closestPoint.y) * (point->y - closestPoint.y) + (point->z - closestPoint.z) * (point->z - closestPoint.z);
}

Float DistanceBetweenSegments(Vec *point, Vec *segmentStart, Vec *segmentVector, Vec *closestPoint, Vec *separationVector)
{
    Float segmentDistance;

    if (((segmentVector->z * segmentVector->z) + ((segmentVector->x * segmentVector->x) + (segmentVector->y * segmentVector->y))) == 0.0f) {
        return ((point->z - segmentStart->z) * (point->z - segmentStart->z))
            + (((point->x - segmentStart->x) * (point->x - segmentStart->x)) + ((point->y - segmentStart->y) * (point->y - segmentStart->y)));
    }
    segmentDistance = DistanceBetweenPoints(*point, *segmentStart, *segmentVector);
    ProjectPointToLine(*segmentStart, *segmentVector, closestPoint, segmentDistance);

    separationVector->x = point->x - closestPoint->x;
    separationVector->y = point->y - closestPoint->y;
    separationVector->z = point->z - closestPoint->z;
    return GetVectorLength(separationVector);
}

Float EvaluateCubicBezier(Float progress, Float startValue, Float controlValue, Float endValue)
{
    Float inverseProgress = 1.0 - progress;
    Float interpolatedValue = (endValue * (progress * progress)) + ((startValue * (inverseProgress * inverseProgress)) + (2.0 * (controlValue * (inverseProgress * progress))));
    return interpolatedValue;
}

void CalculateCubicBezierSlopes(Float *startValues, Float *controlValues, Float *endValues, Float *slopes, Float progress)
{
    s32 coordinateIndex;

    for (coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++) {
        *slopes++ = EvaluateCubicBezier(progress, *startValues++, *controlValues++, *endValues++);
    }
}

Float EvaluateQuadraticBezier(Float progress, Float startValue, Float controlValue, Float endValue)
{
    Float splineDerivative = 2.0 * ((progress * endValue) + (((progress - 1.0) * startValue) + ((1.0 - (2.0 * progress)) * controlValue)));
    return splineDerivative;
}

void CalculateQuadraticBezierSlopes(Float *startValues, Float *controlValues, Float *endValues, Float *slopes, Float progress)
{
    Float splineDerivative[3];
    Float derivativeMagnitude;
    s32 axisIndex;

    for (axisIndex = 0; axisIndex < 3; axisIndex++) {
        splineDerivative[axisIndex] = EvaluateQuadraticBezier(progress, *startValues++, *controlValues++, *endValues++);
    }
    derivativeMagnitude = sqrtf((splineDerivative[2] * splineDerivative[2]) + ((splineDerivative[0] * splineDerivative[0]) + (splineDerivative[1] * splineDerivative[1])));
    if (derivativeMagnitude) {
        derivativeMagnitude = 1.0 / derivativeMagnitude;
        for (axisIndex = 0; axisIndex < 3; axisIndex++) {
            *slopes++ = derivativeMagnitude * splineDerivative[axisIndex];
        }
        return;
    }
    *slopes++ = 0.0f;
    *slopes++ = 0.0f;
    *slopes++ = 1.0f;
}

void CalculateBezierPathDistances(Float *distanceValues, Float *controlValues, Float *secondDerivatives, s32 pointCount)
{
    Float intervalWidths[16];
    Float secantSlopes[16];
    double splineFactor;
    s32 pointIndex;

    secondDerivatives[0] = 0.0f;
    secondDerivatives[pointCount - 1] = 0.0f;
    for (pointIndex = 0; pointIndex < (pointCount - 1); pointIndex++) {
        intervalWidths[pointIndex] = distanceValues[pointIndex + 1] - distanceValues[pointIndex];
        if (intervalWidths[pointIndex] == 0.0f) {
            secantSlopes[pointIndex + 1] = 0.0f;
        }
        else {
            secantSlopes[pointIndex + 1] = (controlValues[pointIndex + 1] - controlValues[pointIndex]) / intervalWidths[pointIndex];
        }
    }
    secondDerivatives[1] = secantSlopes[2] - secantSlopes[1];
    secantSlopes[1] = 2.0f * (distanceValues[2] - distanceValues[0]);
    for (pointIndex = 1; pointIndex < (pointCount - 2); pointIndex++) {
        splineFactor = intervalWidths[pointIndex] / secantSlopes[pointIndex];
        secondDerivatives[pointIndex + 1] = (secantSlopes[pointIndex + 2] - secantSlopes[pointIndex + 1]) - (splineFactor * secondDerivatives[pointIndex]);
        secantSlopes[pointIndex + 1] = (2.0f * (distanceValues[pointIndex + 2] - distanceValues[pointIndex])) - (splineFactor * intervalWidths[pointIndex]);
    }
    secondDerivatives[pointCount - 2] -= intervalWidths[pointCount - 2] * secondDerivatives[pointCount - 1];
    for (pointIndex = pointCount - 2; pointIndex > 0; pointIndex--) {
        if (secantSlopes[pointIndex] == 0.0f) {
            secondDerivatives[pointIndex] = 0.0f;
        }
        else {
            secondDerivatives[pointIndex] = (secondDerivatives[pointIndex] - (intervalWidths[pointIndex] * secondDerivatives[pointIndex + 1])) / secantSlopes[pointIndex];
        }
    }
}

Float EvaluateBezierPath(Float pathProgress, Float *distanceValues, Float *controlValues, Float *secondDerivatives, s32 pointCount)
{
    Float intervalWidth;
    Float scaledProgress;
    Float interpolatedValue;

    s32 intervalIndex = pathProgress;
    pathProgress = pathProgress - intervalIndex;
    intervalWidth = distanceValues[intervalIndex + 1] - distanceValues[intervalIndex];
    scaledProgress = intervalWidth * pathProgress;
    interpolatedValue = controlValues[intervalIndex]
        + (scaledProgress
            * ((scaledProgress * ((3.0f * secondDerivatives[intervalIndex]) + ((scaledProgress * (secondDerivatives[intervalIndex + 1] - secondDerivatives[intervalIndex])) / intervalWidth)))
                + (((controlValues[intervalIndex + 1] - controlValues[intervalIndex]) / intervalWidth) - (intervalWidth * ((2.0f * secondDerivatives[intervalIndex]) + secondDerivatives[intervalIndex + 1])))));
    if (intervalWidth == 0.0f) {
        interpolatedValue = controlValues[intervalIndex];
    }
    return interpolatedValue;
}

void calculateBezierPathSlopes(Float *distances, Float (*controlPoints)[3], Float (*secondDerivatives)[3], s32 pointCount)
{
    Float coordinateSamples[16];
    Float splineSecondDerivatives[16];
    Vec segmentDelta;
    s32 axisIndex;
    s32 sampleIndex;
    s32 coordinateIndex;

    distances[0] = 0.0f;
    for (sampleIndex = 1; sampleIndex < pointCount; sampleIndex++) {
        VECSubtract((Vec *)&controlPoints[sampleIndex], (Vec *)&controlPoints[sampleIndex - 1], &segmentDelta);
        distances[sampleIndex] = distances[sampleIndex - 1] + VECMag(&segmentDelta);
    }
    for (coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++) {
        for (sampleIndex = 0; sampleIndex < pointCount; sampleIndex++) {
            coordinateSamples[sampleIndex] = controlPoints[sampleIndex][coordinateIndex];
        }
        CalculateBezierPathDistances(distances, coordinateSamples, splineSecondDerivatives, pointCount);
        for (sampleIndex = 0; sampleIndex < pointCount; sampleIndex++) {
            secondDerivatives[sampleIndex][coordinateIndex] = splineSecondDerivatives[sampleIndex];
        }
    }
}

void interpolateBezierPath(Float pathProgress, Float *outputValues, Float *distanceValues, Float (*controlPoints)[3], Float (*secondDerivatives)[3], s32 pointCount)
{
    Float controlPoints[16];
    Float secondDerivatives[16];
    s32 sampleIndex;
    s32 axisIndex;

    for (axisIndex = 0; axisIndex < 3; axisIndex++) {
        for (sampleIndex = 0; sampleIndex < pointCount; sampleIndex++) {
            controlPoints[sampleIndex] = controlPoints[sampleIndex][axisIndex];
            secondDerivatives[sampleIndex] = secondDerivatives[sampleIndex][axisIndex];
        }
        outputValues[axisIndex] = EvaluateBezierPath(pathProgress, distanceValues, controlPoints, secondDerivatives, 3);
    }
}

u8 IsPositionInCameraBounds(Vec *position)
{
    u32 faceIndex;
    M428DllCollisionMesh *face;
    s32 collisionFound;

    collisionFound = 0;
    face = mapCollisionData.faces;
    for (faceIndex = 0; faceIndex < mapCollisionData.faceCount; faceIndex++, face++) {
        if (FindMeshFaceAtPosition(face, position) != 0) {
            break;
        }
    }
    if (faceIndex < mapCollisionData.faceCount) {
        collisionFound = 1;
    }
    return collisionFound;
}

void ConfigureCameraTransition(u8 cameraIndex, Float transitionFrames, Vec *targetCenter, Vec *targetRotation, Float targetZoom)
{
    M428DllCameraWork *cameraWork;

    cameraWork = cameraObjects[cameraIndex]->data;
    cameraWork->cameraTransitionActive = 0;
    cameraWork->transitionDuration = transitionFrames;
    cameraWork->transitionFramesRemaining = transitionFrames;
    cameraWork->startCameraCenter = cameraWork->cameraCenter;
    cameraWork->targetCameraCenter.x = targetCenter->x;
    cameraWork->targetCameraCenter.y = targetCenter->y;
    cameraWork->targetCameraCenter.z = targetCenter->z;
    cameraWork->startCameraRotation = cameraWork->cameraRotation;
    cameraWork->targetCameraRotation.x = targetRotation->x;
    cameraWork->targetCameraRotation.y = targetRotation->y;
    cameraWork->targetCameraRotation.z = targetRotation->z;
    cameraWork->startCameraZoom = cameraWork->cameraZoom;
    cameraWork->targetCameraZoom = targetZoom;
}

void SetCameraMode(u8 cameraIndex)
{
    Vec targetCameraCenter;
    Vec targetCameraRotation;
    Float targetCameraZoom;
    M428DllCameraWork *cameraWork;
    OMOBJ *mapObject;

    targetCameraCenter.x = 0.0f;
    targetCameraCenter.y = 80.0f;
    targetCameraCenter.z = 0.0f;
    targetCameraRotation.x = 0.0f;
    targetCameraRotation.y = 0.0f;
    targetCameraRotation.z = 0.0f;
    targetCameraZoom = 600.0f;
    cameraWork = cameraObjects[cameraIndex]->data;
    cameraWork->cameraTransitionActive = 0;
    cameraWork->transitionFramesRemaining = cameraWork->transitionDuration = 1.0f;
    cameraWork->startCameraCenter = cameraWork->cameraCenter;
    cameraWork->targetCameraCenter.x = targetCameraCenter.x;
    cameraWork->targetCameraCenter.y = targetCameraCenter.y;
    cameraWork->targetCameraCenter.z = targetCameraCenter.z;
    cameraWork->startCameraRotation = cameraWork->cameraRotation;
    cameraWork->targetCameraRotation.x = targetCameraRotation.x;
    cameraWork->targetCameraRotation.y = targetCameraRotation.y;
    cameraWork->targetCameraRotation.z = targetCameraRotation.z;
    cameraWork->startCameraZoom = cameraWork->cameraZoom;
    cameraWork->targetCameraZoom = targetCameraZoom;
    Hu3DCameraPerspectiveSet(1 << cameraIndex, 45.0f, 100.0f, 5000.0f, 1.2f);
    Hu3DCameraViewportSet(1 << cameraIndex, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraViewportSet(2 >> cameraIndex, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1 << cameraIndex, 0, 0, 0x280, 0x1E0);
    Hu3DCameraScissorSet(2 >> cameraIndex, 0, 0, 0, 0);
    mapObject = mapObject;
    Hu3DModelAttrSet(mapObject->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapObject->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapObject->mdlId[6], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapObject->mdlId[7], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(mapObject->mdlId[3], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(mapObject->mdlId[cameraIndex + 4], HU3D_ATTR_DISPOFF);
    mapObject->objFunc = EmptyMapCallback;
}

void SetCameraShake(u8 cameraIndex, Float shakeOffset, s8 shakeDirection)
{
    HU3DPARTICLE *particle;
    M428DllParticleBounds *particleBounds;

    particle = Hu3DData[mapObject->mdlId[cameraIndex + 6]].hookData;
    particleBounds = ((HU3DPARTICLE *)Hu3DData[mapObject->mdlId[cameraIndex + 6]].hookData)->work;
    particleBounds->emissionState = 2;
    particle->pos.x = shakeOffset * shakeDirection;
}

void ClearCameraShake(u8 cameraIndex)
{
    M428DllParticleBounds *particleBounds = ((HU3DPARTICLE *)Hu3DData[mapObject->mdlId[cameraIndex + 6]].hookData)->work;
    if (particleBounds->emissionState == 2) {
        particleBounds->emissionState = 3;
    }
}

u8 GetCameraMode(u8 cameraIndex)
{
    M428DllParticleBounds *particleBounds = ((HU3DPARTICLE *)Hu3DData[mapObject->mdlId[cameraIndex + 6]].hookData)->work;
    return particleBounds->emissionState;
}
