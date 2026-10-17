#include "ext_math.h"
#include "game/disp.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/process.h"

#ifndef __MWERKS__
#include <stdlib.h>
#endif


typedef struct CameraTransitionWork {
    /* 0x00 */ s32 transitionDuration;
    /* 0x04 */ s32 cameraTransitionActive;
    /* 0x08 */ Vec startCenter;
    /* 0x14 */ Vec startRotation;
    /* 0x20 */ f32 startZoom;
    /* 0x24 */ Vec targetCenter;
    /* 0x30 */ Vec targetRotation;
    /* 0x3C */ f32 targetZoom;
    /* 0x40 */ s32 transitionFrameCount;
    /* 0x44 */ f32 transitionProgress;
    /* 0x48 */ f32 transitionFrameStep;
} CameraTransitionWork; // sizeof 0x4C

// function signatures
void SetCameraCenter(f32, f32, f32);
void SetCameraRotation(f32, f32, f32);
void SetCameraZoom(f32);
void UpdateCameraTransition(OMOBJ *);

// bss
HUPROCESS *cameraProcess;
OMOBJ *cameraTransitionObject;

void InitializeCameraSystem(HUPROCESS *process)
{
    CameraTransitionWork *cameraWork;

    cameraProcess = process;
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 45.0f, 10.0f, 40000.0f, 1.2f);
    cameraTransitionObject = omAddObjEx(cameraProcess, 0x80, 0, 0, -1, UpdateCameraTransition);
    cameraTransitionObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(CameraTransitionWork), HU_MEMNUM_OVL);
    cameraWork = cameraTransitionObject->data;
    cameraWork->cameraTransitionActive = 0;
    SetCameraCenter(0.0f, 0.0f, 0.0f);
    SetCameraRotation(0.0f, 0.0f, 0.0f);
    SetCameraZoom(0.0f);
}

void ResetCameraTransition(void) { }

void SetCameraCenter(f32 centerX, f32 centerY, f32 centerZ)
{
    Center.x = centerX;
    Center.y = centerY;
    Center.z = centerZ;
}

void GetCameraCenter(f32 *centerX, f32 *centerY, f32 *centerZ)
{
    *centerX = Center.x;
    *centerY = Center.y;
    *centerZ = Center.z;
}

void SetCameraRotation(f32 rotationX, f32 rotationY, f32 rotationZ)
{
    CRot.x = rotationX;
    CRot.y = rotationY;
    CRot.z = rotationZ;
}

void GetCameraRotation(f32 *rotationX, f32 *rotationY, f32 *rotationZ)
{
    *rotationX = CRot.x;
    *rotationY = CRot.y;
    *rotationZ = CRot.z;
}

void SetCameraZoom(f32 zoom)
{
    CZoom = zoom;
}

void GetCameraZoom(f32 *zoom)
{
    *zoom = CZoom;
}

s32 IsCameraTransitionActive(void)
{
    CameraTransitionWork *cameraWork = cameraTransitionObject->data;
    return cameraWork->cameraTransitionActive;
}

void StartCameraTransition(Vec *startCenter, Vec *startRotation, f32 *startZoom, s32 frameCount, s32 transitionMode)
{
    CameraTransitionWork *workData;

    workData = cameraTransitionObject->data;
    workData->transitionDuration = transitionMode;
    workData->startCenter = (startCenter) ? *startCenter : Center;
    workData->startRotation = (startRotation) ? *startRotation : CRot;
    workData->startZoom = (startZoom) ? *startZoom : CZoom;
    workData->targetCenter = Center;
    workData->targetRotation = CRot;
    workData->targetZoom = CZoom;
    workData->transitionFrameCount = frameCount;
    workData->transitionProgress = 0.0f;
    workData->transitionFrameStep = 1.0f / frameCount;
    workData->cameraTransitionActive = 1;
}

f32 GetAngleDistance(f32 angleA, f32 angleB)
{
    if (((angleA > 0.0f) && (angleB > 0.0f)) || ((angleA < 0.0f) && (angleB < 0.0f))) {
        return abs(angleA - angleB);
    }

    return abs(angleA) + abs(angleB);
}

void UpdateCameraTransition(OMOBJ *object)
{
    Vec interpolatedCenter;
    Vec interpolatedRotation;
    f32 transitionBlend;
    f32 cZoomTemp;
    CameraTransitionWork *workData;

    workData = object->data;
    if (workData->cameraTransitionActive != 0) {
        switch (workData->transitionDuration) {
            case 0:
                transitionBlend = workData->transitionProgress;
                break;
            case 1:
                transitionBlend = workData->transitionProgress;
                if (workData->transitionFrameStep > 0.5) {
                    workData->transitionDuration = 2;
                }
                break;
            case 2:
                transitionBlend = sind(90.0f * workData->transitionProgress);
                break;
            case 3:
                transitionBlend = sind(90.0f * workData->transitionProgress);
                if (workData->transitionFrameStep > 0.5) {
                    workData->transitionDuration = 4;
                }
                break;
            case 4:
                transitionBlend = sind(90.0f * workData->transitionProgress);
                transitionBlend *= transitionBlend;
                break;
        }
        interpolatedCenter.x = workData->targetCenter.x + (transitionBlend * (workData->startCenter.x - workData->targetCenter.x));
        interpolatedCenter.y = workData->targetCenter.y + (transitionBlend * (workData->startCenter.y - workData->targetCenter.y));
        interpolatedCenter.z = workData->targetCenter.z + (transitionBlend * (workData->startCenter.z - workData->targetCenter.z));
        interpolatedRotation.x = workData->targetRotation.x + (transitionBlend * (workData->startRotation.x - workData->targetRotation.x));
        interpolatedRotation.y = workData->targetRotation.y + (transitionBlend * (workData->startRotation.y - workData->targetRotation.y));
        interpolatedRotation.z = workData->targetRotation.z + (transitionBlend * (workData->startRotation.z - workData->targetRotation.z));
        cZoomTemp = workData->targetZoom + (transitionBlend * (workData->startZoom - workData->targetZoom));
        workData->transitionProgress += workData->transitionFrameStep;
        Center = interpolatedCenter;
        CRot = interpolatedRotation;
        CZoom = cZoomTemp;
        if (--workData->transitionFrameCount <= 0) {
            Center = workData->startCenter;
            CRot = workData->startRotation;
            CZoom = workData->startZoom;
            workData->cameraTransitionActive = 0;
        }
    }
    omOutView(object);
}
