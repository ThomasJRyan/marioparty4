#include "REL/m424Dll.h"

#include "game/audio.h"
#include "game/hu3d.h"

#include "math.h"
#include "version.h"
#include <string.h>


// STRUCT
typedef struct M424MapCameraState {
    f32 cameraZoom;
    f32 cameraHeight;
    Mtx cameraMatrix;
    s32 mapState;
    f32 cameraSpeed;
    s32 animationState;
    s32 animationTimer;
    s32 effectState;
    s32 effectTimer;
    s32 mapFlags;
} M424MapCameraState; // sizeof 0x54

// BSS
HUPROCESS *mapObjectManager;
OMOBJ *mapCameraObject;

// DATA
Vec mapShadowPosition = { 0.0f, 3000.0f, 1.0f };
Vec mapShadowDirection = { 0.0f, 1.0f, 0.0f };
Vec mapShadowTarget = { 0.0f, 0.0f, 0.0f };

// PROTO
void MapCameraObjectInit(OMOBJ *);
void MapCameraObjectUpdate(OMOBJ *);
void MapCameraObjectEnd(OMOBJ *);
void MapModelAttributeAnimate(f32);

OMOBJ *MapCameraCreate(HUPROCESS *processManager)
{
    mapObjectManager = processManager;
    mapCameraObject = omAddObjEx(processManager, 0x7F, 6, 0, -1, MapCameraObjectInit);
    mapCameraObject->data = NULL;
    Hu3DShadowCreate(45.0f, 1000.0f, 250000.0f);
    Hu3DShadowTPLvlSet(0.3f);
    Hu3DShadowPosSet(&mapShadowPosition, &mapShadowDirection, &mapShadowTarget);
    return mapCameraObject;
}

void MapCameraDestroy(void)
{
    MapCameraObjectEnd(mapCameraObject);
}

void MapCameraObjectInit(OMOBJ *cameraObject)
{
    M424MapCameraState *cameraState;

    cameraState = cameraObject->data = HuMemDirectMallocNum(HEAP_HEAP, 0x54, 0x10000000);
    memset(cameraState, 0, 0x54);
    cameraState->cameraZoom = 0.0f;
    cameraState->cameraHeight = 0.0f;
    cameraState->mapState = -1;
    cameraState->cameraSpeed = 0.0f;
    cameraState->animationState = 0;
    cameraState->animationTimer = -1;
    cameraState->effectState = 0;
    cameraState->effectTimer = 0;
    cameraState->mapFlags = -1;
    cameraObject->mdlId[0] = Hu3DModelCreateFile(0x370017);
    cameraObject->mdlId[1] = Hu3DModelCreateFile(0x370018);
    cameraObject->mdlId[2] = Hu3DModelCreateFile(0x370019);
    cameraObject->mdlId[3] = Hu3DModelCreateFile(0x37001A);
    cameraObject->mdlId[4] = Hu3DModelCreateFile(0x37001B);
    cameraObject->mdlId[5] = Hu3DModelCreateFile(0x37001C);
    Hu3DModelAttrSet(cameraObject->mdlId[4], 1);
    Hu3DMotionSpeedSet(cameraObject->mdlId[1], cameraState->cameraZoom);
    Hu3DModelAttrSet(cameraObject->mdlId[1], 0x40000001);
    Hu3DMotionSpeedSet(cameraObject->mdlId[0], 0.15f);
    Hu3DModelAttrSet(cameraObject->mdlId[0], 0x40000001);
    Hu3DModelPosSet(cameraObject->mdlId[2], 847.135f, 0.0f, 424.56f);
    Hu3DMotionSpeedSet(cameraObject->mdlId[4], 2.0f);
    Hu3DModelAttrSet(cameraObject->mdlId[4], 0x40000001);
    Hu3DModelShadowMapSet(cameraObject->mdlId[0]);
    Hu3DModelShadowMapSet(cameraObject->mdlId[1]);
    Hu3DModelShadowMapSet(cameraObject->mdlId[2]);
    cameraObject->objFunc = MapCameraObjectUpdate;
}

void MapCameraObjectUpdate(OMOBJ *cameraObject)
{
    f32 normalizedZoom;
    M424MapCameraState *cameraState;

    cameraState = cameraObject->data;
    if (cameraState->animationState > 0) {
        MapModelAttributeAnimate(cameraState->cameraSpeed);
        if ((--cameraState->animationState <= 0) && (cameraState->animationTimer != -1)) {
            HuAudFXStop(cameraState->animationTimer);
            cameraState->animationTimer = -1;
        }
    }
    switch (M424SceneStateGet()) {
        case 0:
            cameraState->cameraZoom = VERSION_PAL ? 0.6f : 0.5f;
            break;
        case 1:
        case 2:
            cameraState->cameraZoom = 0.0f;
            break;
        case 3:
        case 4:
            cameraState->cameraZoom += 0.001f;
            if (cameraState->cameraZoom > 12.0f / (REFRESH_RATE_F * 5.0f)) {
                cameraState->cameraZoom = 12.0f / (REFRESH_RATE_F * 5.0f);
            }
            if (cameraState->mapState != -1)
                break;

            cameraState->mapState = HuAudFXPlay(0x626);
            break;
        default:
            if ((ClawMotionActive() == 0) && (ActiveBallExists() != 0)) {
                cameraState->cameraZoom *= 0.9f;
                if (cameraState->cameraZoom < 0.01f) {
                    cameraState->cameraZoom = 0.0f;
                }
                normalizedZoom = cameraState->cameraZoom / (12.0f / (REFRESH_RATE_F * 5.0f));
                if (cameraState->mapState != -1) {
                    HuAudFXVolSet(cameraState->mapState, (s16)(-8191.0f * (1.0f - normalizedZoom)));
                    HuAudFXPitchSet(cameraState->mapState, (s16)(127.0f * normalizedZoom));
                    if (normalizedZoom <= 0.0f) {
                        HuAudFXStop(cameraState->mapState);
                        cameraState->mapState = -1;
                    }
                }
            }
            break;
    }

    if (cameraState->effectState != 0) {
        if (cameraState->effectTimer != 0) {
            cameraState->effectTimer = (cameraState->effectTimer - 1);
        }
        else {
            cameraState->effectState = (cameraState->effectState - 1);
            if (cameraState->effectState != 0) {
                Hu3DModelAttrReset(cameraObject->mdlId[4], 1);
                Hu3DModelAttrSet(cameraObject->mdlId[5], 1);
            }
            else {
                Hu3DModelAttrSet(cameraObject->mdlId[4], 1);
                Hu3DModelAttrReset(cameraObject->mdlId[5], 1);
                if (cameraState->mapFlags != -1) {
                    HuAudFXStop(cameraState->mapFlags);
                    cameraState->mapFlags = -1;
                }
            }
        }
    }
    cameraState->cameraHeight = (20.0f * cameraState->cameraZoom);
    MTXRotRad(cameraState->cameraMatrix, 0x59, MTXDegToRad(cameraState->cameraHeight));
    Hu3DMotionSpeedSet(cameraObject->mdlId[1], cameraState->cameraZoom);
}

void MapCameraObjectEnd(OMOBJ *cameraObject)
{
    cameraObject->objFunc = NULL;
}

f32 MapCameraHeightGet(void)
{
    M424MapCameraState *cameraState;
    if (!mapCameraObject) {
        return 0.0f;
    }
    if (!mapCameraObject->data) {
        return 0.0f;
    }
    cameraState = mapCameraObject->data;
    return cameraState->cameraHeight;
}

void MapCameraMatrixGet(Mtx cameraMatrix)
{
    M424MapCameraState *cameraState;
    if (mapCameraObject && mapCameraObject->data) {
        cameraState = mapCameraObject->data;
        MTXCopy(cameraState->cameraMatrix, cameraMatrix);
    }
}

void MapCameraVectorTransform(Vec *inputVector, Vec *transformedOffset)
{
    Vec cameraOffset;
    M424MapCameraState *cameraState;

    if (mapCameraObject && mapCameraObject->data) {
        cameraState = mapCameraObject->data;
        if (sqrtf((inputVector->x * inputVector->x) + (inputVector->z * inputVector->z)) < 700.0f) {
            cameraOffset.x = (cameraState->cameraMatrix[0][0] * inputVector->x) + (cameraState->cameraMatrix[0][2] * inputVector->z);
            cameraOffset.z = (cameraState->cameraMatrix[2][0] * inputVector->x) + (cameraState->cameraMatrix[2][2] * inputVector->z);
            cameraOffset.y = inputVector->y;
        }
        else {
            cameraOffset = *inputVector;
        }
        VECSubtract(&cameraOffset, inputVector, &cameraOffset);
        *transformedOffset = cameraOffset;
    }
}

void MapModelAttributeAnimate(f32 horizontalOffset)
{
    M424MapCameraState *cameraState;
    HSFATTRIBUTE *attribute;
    HU3DMODEL *cameraModel;
    HSFDATA *modelData;
    s32 attributeIndex;
    HSFBITMAP *attributeBitmap;

    if (mapCameraObject && mapCameraObject->data) {
        cameraState = mapCameraObject->data;
        cameraModel = &Hu3DData[mapCameraObject->mdlId[2]];
        modelData = cameraModel->hsf;
        attribute = cameraModel->hsf->attribute;
        for (attributeIndex = 0; attributeIndex < modelData->attributeNum; attributeIndex++, attribute++) {
            attributeBitmap = attribute->bitmap;
            if (strcmp("cr_dk2", attributeBitmap->name) == 0)
                break;
        }
        if (attributeIndex < modelData->attributeNum) {
            attribute->trans.x += horizontalOffset;
        }
    }
}

void MapCameraAnimationStart(f32 cameraSpeed, s32 durationFrames)
{
    OMOBJ *cameraObject;
    M424MapCameraState *cameraState;

    if (mapCameraObject && mapCameraObject->data) {
        cameraObject = mapCameraObject;
        cameraState = mapCameraObject->data;
        cameraState->animationTimer = HuAudFXPlay(0x627);
        cameraState->cameraSpeed = cameraSpeed;
        cameraState->animationState = durationFrames;
    }
}

f32 MapCameraAnimationOffsetGet(void)
{
    M424MapCameraState *cameraState;

    if (!mapCameraObject) {
        return 0.0f;
    }
    if (!mapCameraObject->data) {
        return 0.0f;
    }
    cameraState = mapCameraObject->data;
    if (cameraState->animationState != 0) {
        return 250.0f * cameraState->cameraSpeed;
    }
    return 0.0f;
}

void MapEffectStart(s32 effectDuration, s32 effectDelayFrames)
{
    OMOBJ *cameraObject;
    M424MapCameraState *cameraState;

    if (mapCameraObject && mapCameraObject->data) {
        cameraObject = mapCameraObject;
        cameraState = mapCameraObject->data;
        cameraState->mapFlags = HuAudFXPlay(0x629);
        cameraState->effectState = effectDuration;
        cameraState->effectTimer = effectDelayFrames;
        Hu3DMotionTimeSet(cameraObject->mdlId[4], 0.0f);
    }
}

// UNUSED (guessing types)
M424CameraConfig unusedMapCameraConfig = { { 0.0f, 0.0f, 5500.0f }, { -3950.0f, 0.0f, -0.7f }, 0.6f };

GXColor unusedMapColor = { 0xFF, 0xFF, 0xFF, 0xFF };
M424CameraConfig unusedMapCameraConfig2 = { { 0.0f, 5500.0f, -3950.0f }, { 0.0f, 1.0f, 0.0f }, 0.0f };

s32 unusedMapData[2] = { 0 };
