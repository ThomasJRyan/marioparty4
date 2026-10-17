#include "ext_math.h"
#include "game/armem.h"
#include "game/audio.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/wipe.h"

#include "REL/mpexDll.h"

void InitializeMpexOverlayResources(void);

#ifndef __MWERKS__
float InterpolateLinear(float startValue, float endValue, float currentFrame, float duration);
float InterpolateByStep(float, float, float);
float InterpolateCosine(float startValue, float endValue, float currentFrame, float duration);
float InterpolateSine(float startValue, float endValue, float currentFrame, float duration);
#endif

void CopyCurrentCameraToTarget(MpexCameraWork *cameraWork);

MpexCameraWork cameraWork;
s32 reservedBssWords[0x18];
HUPROCESS *overlayObjectManager;

void DumpMemoryHeapsOnInput(void)
{
    if (HuPadBtnDown[0] & PAD_BUTTON_Y) {
        OSReport("\n###################################################################### HEAP\n\n");
        HuMemHeapDump(HuMemHeapPtrGet(HEAP_HEAP), -1);
        OSReport("\n###################################################################### SOUND\n\n");
        HuMemHeapDump(HuMemHeapPtrGet(HEAP_SOUND), -1);
        OSReport("\n###################################################################### MODEL\n\n");
        HuMemHeapDump(HuMemHeapPtrGet(HEAP_MODEL), -1);
        OSReport("\n###################################################################### DVD\n\n");
        HuMemHeapDump(HuMemHeapPtrGet(HEAP_DVD), -1);
        OSReport("\n###################################################################### SPACE\n\n");
        HuMemHeapDump(HuMemHeapPtrGet(HEAP_SPACE), -1);
        OSReport("\n###################################################################### ARAM\n\n");
        HuAMemDump();
    }
}

void ReturnFromOverlayAfterWipe(OMOBJ *object)
{
    if (WipeStatGet() == 0) {
        omOvlReturnEx(1, 1);
    }
}

void HandleOverlayExitRequest(OMOBJ *object)
{
    if (omSysExitReq != 0) {
        HuAudFadeOut(0x3E8);
        WipeColorSet(0, 0, 0);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
        object->objFunc = ReturnFromOverlayAfterWipe;
    }
}

void ObjectSetup(void)
{
    mgPracticeEnableF = 0;
    overlayObjectManager = omInitObjMan(0x3E, 0x2000);
    GWSystem.mg_next = 0xFFFF;
    InitializeMinigamePracticeOverlay(overlayObjectManager);
}

void CreateDirectionalLight(void)
{
    s32 lightId;

    lightId = -1;
    lightId = Hu3DGLightCreate(0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0xFF, 0xFF, 0xFF);
    Hu3DGLightPosAimSet(lightId, 0.0f, 1000.0f, 1000.0f, 0.0f, 0.0f, 0.0f);
    Hu3DGLightInfinitytSet(lightId);
}

void CreateOverlayShadow(void)
{
    Vec shadowCameraPosition = { 0.0f, 3000.0f, 3000.0f };
    Vec shadowTargetPosition = { 0.0f, 0.0f, 0.0f };
    Vec shadowUpVector = { 0.0f, 1.0f, 0.0f };

    Hu3DShadowCreate(45.0f, 2000.0f, 25000.0f);
    Hu3DShadowPosSet(&shadowCameraPosition, &shadowUpVector, &shadowTargetPosition);
    Hu3DShadowTPLvlSet(0.8f);
}

void UpdateCameraFromController(void)
{
    MpexCameraWork *cameraState = &cameraWork;

    if (HuPadBtn[0] & PAD_BUTTON_B) {
        if (HuPadBtn[0] & 0x20) {
            cameraState->currentCenter.z -= (HuPadStkY[0] / 10.0f);
        }
        else {
            cameraState->currentCenter.x += HuPadStkX[0] / 10.0f;
            cameraState->currentCenter.y += (HuPadStkY[0] / 10.0f);
        }
    }
    if (HuPadBtn[0] & PAD_BUTTON_Y) {
        cameraState->currentRotation.x -= HuPadStkY[0] / 100.0f;
        cameraState->currentRotation.y += (HuPadStkX[0] / 100.0f);
        if (cameraState->currentRotation.x < 0.0f) {
            cameraState->currentRotation.x += 360.0f;
        }
        if (cameraState->currentRotation.x >= 360.0f) {
            cameraState->currentRotation.x -= 360.0f;
        }
        if (cameraState->currentRotation.y < 0.0f) {
            cameraState->currentRotation.y += 360.0f;
        }
        if (cameraState->currentRotation.y >= 360.0f) {
            cameraState->currentRotation.y -= 360.0f;
        }
    }
    if (HuPadBtn[0] & PAD_BUTTON_X) {
        cameraState->currentZoom -= (HuPadStkY[0] / 10.0f);
    }
    print8(0x18, 0x28, 1.0f, ">>>>>>>>>> CAMERA DATA <<<<<<<<<<");
    print8(0x18, 0x32, 1.0f, "CENTER : %.2f, %.2f, %.2f", cameraState->currentCenter.x, cameraState->currentCenter.y, cameraState->currentCenter.z);
    print8(0x18, 0x3C, 1.0f, "ROT    : %.2f, %.2f, %.2f", cameraState->currentRotation.x, cameraState->currentRotation.y, cameraState->currentRotation.z);
    print8(0x18, 0x46, 1.0f, "ZOOM   : %.2f", cameraState->currentZoom);
}

void ApplyCameraWorkToView(OMOBJ *object)
{
    MpexCameraWork *cameraState = &cameraWork;
    if (cameraState->updateCameraInput != NULL) {
        cameraState->updateCameraInput();
    }
    Center.x = cameraState->currentCenter.x;
    Center.y = cameraState->currentCenter.y;
    Center.z = cameraState->currentCenter.z;
    CRot.x = cameraState->currentRotation.x;
    CRot.y = cameraState->currentRotation.y;
    CRot.z = cameraState->currentRotation.z;
    CZoom = cameraState->currentZoom;
}

void SetCameraInputCallback(CameraInputCallback cameraInputCallback)
{
    cameraWork.updateCameraInput = cameraInputCallback;
    CopyCurrentCameraToTarget(&cameraWork);
}

void CreateOverlayCamera(CameraInputCallback cameraInputCallback)
{
    Hu3DCameraCreate(1);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 45.0f, 20.0f, 5000.0f, 1.2f);
    cameraWork.updateCameraInput = cameraInputCallback;
    cameraWork.controlCamera = UpdateCameraFromController;
    omAddObjEx(overlayObjectManager, 0x7FDA, 0, 0, -1, omOutView);
    omAddObjEx(overlayObjectManager, 0x7FD9, 0, 0, -1, ApplyCameraWorkToView);
}

void CopyTargetCameraToCurrent(MpexCameraWork *cameraWork)
{
    cameraWork->currentCenter.x = cameraWork->targetCenter.x;
    cameraWork->currentCenter.y = cameraWork->targetCenter.y;
    cameraWork->currentCenter.z = cameraWork->targetCenter.z;
    cameraWork->currentRotation.x = cameraWork->targetRotation.x;
    cameraWork->currentRotation.y = cameraWork->targetRotation.y;
    cameraWork->currentRotation.z = cameraWork->targetRotation.z;
    cameraWork->currentZoom = cameraWork->targetZoom;
}

void CopyCurrentCameraToTarget(MpexCameraWork *cameraWork)
{
    cameraWork->interpolationState = 0;
    cameraWork->targetCenter.x = cameraWork->currentCenter.x;
    cameraWork->targetCenter.y = cameraWork->currentCenter.y;
    cameraWork->targetCenter.z = cameraWork->currentCenter.z;
    cameraWork->targetRotation.x = cameraWork->currentRotation.x;
    cameraWork->targetRotation.y = cameraWork->currentRotation.y;
    cameraWork->targetRotation.z = cameraWork->currentRotation.z;
    cameraWork->targetZoom = cameraWork->currentZoom;
}

void EaseCameraTowardWorkTarget(MpexCameraWork *currentCamera, MpexCameraWork *targetCamera, float interpolationStep)
{
    currentCamera->currentCenter.x = InterpolateByStep(currentCamera->currentCenter.x, targetCamera->targetCenter.x, interpolationStep);
    currentCamera->currentCenter.y = InterpolateByStep(currentCamera->currentCenter.y, targetCamera->targetCenter.y, interpolationStep);
    currentCamera->currentCenter.z = InterpolateByStep(currentCamera->currentCenter.z, targetCamera->targetCenter.z, interpolationStep);
    currentCamera->currentRotation.x = InterpolateByStep(currentCamera->currentRotation.x, targetCamera->targetRotation.x, interpolationStep);
    currentCamera->currentRotation.y = InterpolateByStep(currentCamera->currentRotation.y, targetCamera->targetRotation.y, interpolationStep);
    currentCamera->currentRotation.z = InterpolateByStep(currentCamera->currentRotation.z, targetCamera->targetRotation.z, interpolationStep);
    currentCamera->currentZoom = InterpolateByStep(currentCamera->currentZoom, targetCamera->targetZoom, interpolationStep);
}

void EaseCameraTowardOwnTarget(MpexCameraWork *cameraWork, float interpolationStep, float unusedFrameCount)
{
    cameraWork->currentCenter.x = InterpolateByStep(cameraWork->currentCenter.x, cameraWork->targetCenter.x, interpolationStep);
    cameraWork->currentCenter.y = InterpolateByStep(cameraWork->currentCenter.y, cameraWork->targetCenter.y, interpolationStep);
    cameraWork->currentCenter.z = InterpolateByStep(cameraWork->currentCenter.z, cameraWork->targetCenter.z, interpolationStep);
    cameraWork->currentRotation.x = InterpolateByStep(cameraWork->currentRotation.x, cameraWork->targetRotation.x, interpolationStep);
    cameraWork->currentRotation.y = InterpolateByStep(cameraWork->currentRotation.y, cameraWork->targetRotation.y, interpolationStep);
    cameraWork->currentRotation.z = InterpolateByStep(cameraWork->currentRotation.z, cameraWork->targetRotation.z, interpolationStep);
    cameraWork->currentZoom = InterpolateByStep(cameraWork->currentZoom, cameraWork->targetZoom, interpolationStep);
}

void InterpolateCameraWorkTargets(MpexCameraWork *sourceCamera, MpexCameraWork *targetCamera, float frameIndex, float frameCount, float unusedBlendFactor)
{
    targetCamera->targetCenter.x = InterpolateLinear(sourceCamera->targetCenter.x, targetCamera->currentCenter.x, frameIndex, frameCount);
    targetCamera->targetCenter.y = InterpolateLinear(sourceCamera->targetCenter.y, targetCamera->currentCenter.y, frameIndex, frameCount);
    targetCamera->targetCenter.z = InterpolateLinear(sourceCamera->targetCenter.z, targetCamera->currentCenter.z, frameIndex, frameCount);
    targetCamera->targetRotation.x = InterpolateLinear(sourceCamera->targetRotation.x, targetCamera->currentRotation.x, frameIndex, frameCount);
    targetCamera->targetRotation.y = InterpolateLinear(sourceCamera->targetRotation.y, targetCamera->currentRotation.y, frameIndex, frameCount);
    targetCamera->targetRotation.z = InterpolateLinear(sourceCamera->targetRotation.z, targetCamera->currentRotation.z, frameIndex, frameCount);
    targetCamera->targetZoom = InterpolateLinear(sourceCamera->targetZoom, targetCamera->currentZoom, frameIndex, frameCount);
}

void InterpolateAndApplyCameraWorkTargets(MpexCameraWork *sourceCamera, MpexCameraWork *targetCamera, float frameIndex, float frameCount, float unusedBlendFactor)
{
    targetCamera->targetCenter.x = InterpolateLinear(sourceCamera->targetCenter.x, targetCamera->currentCenter.x, frameIndex, frameCount);
    targetCamera->targetCenter.y = InterpolateLinear(sourceCamera->targetCenter.y, targetCamera->currentCenter.y, frameIndex, frameCount);
    targetCamera->targetCenter.z = InterpolateLinear(sourceCamera->targetCenter.z, targetCamera->currentCenter.z, frameIndex, frameCount);
    targetCamera->targetRotation.x = InterpolateLinear(sourceCamera->targetRotation.x, targetCamera->currentRotation.x, frameIndex, frameCount);
    targetCamera->targetRotation.y = InterpolateLinear(sourceCamera->targetRotation.y, targetCamera->currentRotation.y, frameIndex, frameCount);
    targetCamera->targetRotation.z = InterpolateLinear(sourceCamera->targetRotation.z, targetCamera->currentRotation.z, frameIndex, frameCount);
    targetCamera->targetZoom = InterpolateLinear(sourceCamera->targetZoom, targetCamera->currentZoom, frameIndex, frameCount);
    sourceCamera->currentCenter.x = targetCamera->targetCenter.x;
    sourceCamera->currentCenter.y = targetCamera->targetCenter.y;
    sourceCamera->currentCenter.z = targetCamera->targetCenter.z;
    sourceCamera->currentRotation.x = targetCamera->targetRotation.x;
    sourceCamera->currentRotation.y = targetCamera->targetRotation.y;
    sourceCamera->currentRotation.z = targetCamera->targetRotation.z;
    sourceCamera->currentZoom = targetCamera->targetZoom;
}

void InterpolateAndEaseCameraWorkTargets(MpexCameraWork *sourceCamera, MpexCameraWork *targetCamera, float frameIndex, float frameCount, float interpolationStep, float unusedBlendFactor)
{
    targetCamera->targetCenter.x = InterpolateLinear(sourceCamera->targetCenter.x, targetCamera->currentCenter.x, frameIndex, frameCount);
    targetCamera->targetCenter.y = InterpolateLinear(sourceCamera->targetCenter.y, targetCamera->currentCenter.y, frameIndex, frameCount);
    targetCamera->targetCenter.z = InterpolateLinear(sourceCamera->targetCenter.z, targetCamera->currentCenter.z, frameIndex, frameCount);
    targetCamera->targetRotation.x = InterpolateLinear(sourceCamera->targetRotation.x, targetCamera->currentRotation.x, frameIndex, frameCount);
    targetCamera->targetRotation.y = InterpolateLinear(sourceCamera->targetRotation.y, targetCamera->currentRotation.y, frameIndex, frameCount);
    targetCamera->targetRotation.z = InterpolateLinear(sourceCamera->targetRotation.z, targetCamera->currentRotation.z, frameIndex, frameCount);
    targetCamera->targetZoom = InterpolateLinear(sourceCamera->targetZoom, targetCamera->currentZoom, frameIndex, frameCount);
    sourceCamera->currentCenter.x = InterpolateByStep(sourceCamera->currentCenter.x, targetCamera->targetCenter.x, interpolationStep);
    sourceCamera->currentCenter.y = InterpolateByStep(sourceCamera->currentCenter.y, targetCamera->targetCenter.y, interpolationStep);
    sourceCamera->currentCenter.z = InterpolateByStep(sourceCamera->currentCenter.z, targetCamera->targetCenter.z, interpolationStep);
    sourceCamera->currentRotation.x = InterpolateByStep(sourceCamera->currentRotation.x, targetCamera->targetRotation.x, interpolationStep);
    sourceCamera->currentRotation.y = InterpolateByStep(sourceCamera->currentRotation.y, targetCamera->targetRotation.y, interpolationStep);
    sourceCamera->currentRotation.z = InterpolateByStep(sourceCamera->currentRotation.z, targetCamera->targetRotation.z, interpolationStep);
    sourceCamera->currentZoom = InterpolateByStep(sourceCamera->currentZoom, targetCamera->targetZoom, interpolationStep);
}

void SineInterpolateCameraWorkTargets(MpexCameraWork *sourceCamera, MpexCameraWork *targetCamera, float frameIndex, float frameCount, float unusedBlendFactor)
{
    targetCamera->targetCenter.x = InterpolateSine(sourceCamera->targetCenter.x, targetCamera->currentCenter.x, frameIndex, frameCount);
    targetCamera->targetCenter.y = InterpolateSine(sourceCamera->targetCenter.y, targetCamera->currentCenter.y, frameIndex, frameCount);
    targetCamera->targetCenter.z = InterpolateSine(sourceCamera->targetCenter.z, targetCamera->currentCenter.z, frameIndex, frameCount);
    targetCamera->targetRotation.x = InterpolateSine(sourceCamera->targetRotation.x, targetCamera->currentRotation.x, frameIndex, frameCount);
    targetCamera->targetRotation.y = InterpolateSine(sourceCamera->targetRotation.y, targetCamera->currentRotation.y, frameIndex, frameCount);
    targetCamera->targetRotation.z = InterpolateSine(sourceCamera->targetRotation.z, targetCamera->currentRotation.z, frameIndex, frameCount);
    targetCamera->targetZoom = InterpolateSine(sourceCamera->targetZoom, targetCamera->currentZoom, frameIndex, frameCount);
}

void SineInterpolateAndApplyCameraWorkTargets(MpexCameraWork *sourceCamera, MpexCameraWork *targetCamera, float frameIndex, float frameCount, float unusedBlendFactor)
{
    targetCamera->targetCenter.x = InterpolateSine(sourceCamera->targetCenter.x, targetCamera->currentCenter.x, frameIndex, frameCount);
    targetCamera->targetCenter.y = InterpolateSine(sourceCamera->targetCenter.y, targetCamera->currentCenter.y, frameIndex, frameCount);
    targetCamera->targetCenter.z = InterpolateSine(sourceCamera->targetCenter.z, targetCamera->currentCenter.z, frameIndex, frameCount);
    targetCamera->targetRotation.x = InterpolateSine(sourceCamera->targetRotation.x, targetCamera->currentRotation.x, frameIndex, frameCount);
    targetCamera->targetRotation.y = InterpolateSine(sourceCamera->targetRotation.y, targetCamera->currentRotation.y, frameIndex, frameCount);
    targetCamera->targetRotation.z = InterpolateSine(sourceCamera->targetRotation.z, targetCamera->currentRotation.z, frameIndex, frameCount);
    targetCamera->targetZoom = InterpolateSine(sourceCamera->targetZoom, targetCamera->currentZoom, frameIndex, frameCount);
    sourceCamera->currentCenter.x = targetCamera->targetCenter.x;
    sourceCamera->currentCenter.y = targetCamera->targetCenter.y;
    sourceCamera->currentCenter.z = targetCamera->targetCenter.z;
    sourceCamera->currentRotation.x = targetCamera->targetRotation.x;
    sourceCamera->currentRotation.y = targetCamera->targetRotation.y;
    sourceCamera->currentRotation.z = targetCamera->targetRotation.z;
    sourceCamera->currentZoom = targetCamera->targetZoom;
}

void SineInterpolateAndEaseCameraWorkTargets(MpexCameraWork *sourceCamera, MpexCameraWork *targetCamera, float frameIndex, float frameCount, float interpolationStep, float unusedBlendFactor)
{
    targetCamera->targetCenter.x = InterpolateSine(sourceCamera->targetCenter.x, targetCamera->currentCenter.x, frameIndex, frameCount);
    targetCamera->targetCenter.y = InterpolateSine(sourceCamera->targetCenter.y, targetCamera->currentCenter.y, frameIndex, frameCount);
    targetCamera->targetCenter.z = InterpolateSine(sourceCamera->targetCenter.z, targetCamera->currentCenter.z, frameIndex, frameCount);
    targetCamera->targetRotation.x = InterpolateSine(sourceCamera->targetRotation.x, targetCamera->currentRotation.x, frameIndex, frameCount);
    targetCamera->targetRotation.y = InterpolateSine(sourceCamera->targetRotation.y, targetCamera->currentRotation.y, frameIndex, frameCount);
    targetCamera->targetRotation.z = InterpolateSine(sourceCamera->targetRotation.z, targetCamera->currentRotation.z, frameIndex, frameCount);
    targetCamera->targetZoom = InterpolateSine(sourceCamera->targetZoom, targetCamera->currentZoom, frameIndex, frameCount);
    sourceCamera->currentCenter.x = InterpolateByStep(sourceCamera->currentCenter.x, targetCamera->targetCenter.x, interpolationStep);
    sourceCamera->currentCenter.y = InterpolateByStep(sourceCamera->currentCenter.y, targetCamera->targetCenter.y, interpolationStep);
    sourceCamera->currentCenter.z = InterpolateByStep(sourceCamera->currentCenter.z, targetCamera->targetCenter.z, interpolationStep);
    sourceCamera->currentRotation.x = InterpolateByStep(sourceCamera->currentRotation.x, targetCamera->targetRotation.x, interpolationStep);
    sourceCamera->currentRotation.y = InterpolateByStep(sourceCamera->currentRotation.y, targetCamera->targetRotation.y, interpolationStep);
    sourceCamera->currentRotation.z = InterpolateByStep(sourceCamera->currentRotation.z, targetCamera->targetRotation.z, interpolationStep);
    sourceCamera->currentZoom = InterpolateByStep(sourceCamera->currentZoom, targetCamera->targetZoom, interpolationStep);
}

void CosineInterpolateCameraWorkTargets(MpexCameraWork *sourceCamera, MpexCameraWork *targetCamera, float frameIndex, float frameCount, float unusedBlendFactor)
{
    targetCamera->targetCenter.x = InterpolateCosine(sourceCamera->targetCenter.x, targetCamera->currentCenter.x, frameIndex, frameCount);
    targetCamera->targetCenter.y = InterpolateCosine(sourceCamera->targetCenter.y, targetCamera->currentCenter.y, frameIndex, frameCount);
    targetCamera->targetCenter.z = InterpolateCosine(sourceCamera->targetCenter.z, targetCamera->currentCenter.z, frameIndex, frameCount);
    targetCamera->targetRotation.x = InterpolateCosine(sourceCamera->targetRotation.x, targetCamera->currentRotation.x, frameIndex, frameCount);
    targetCamera->targetRotation.y = InterpolateCosine(sourceCamera->targetRotation.y, targetCamera->currentRotation.y, frameIndex, frameCount);
    targetCamera->targetRotation.z = InterpolateCosine(sourceCamera->targetRotation.z, targetCamera->currentRotation.z, frameIndex, frameCount);
    targetCamera->targetZoom = InterpolateCosine(sourceCamera->targetZoom, targetCamera->currentZoom, frameIndex, frameCount);
}

void CosineInterpolateAndApplyCameraWorkTargets(MpexCameraWork *sourceCamera, MpexCameraWork *targetCamera, float frameIndex, float frameCount, float unusedBlendFactor)
{
    targetCamera->targetCenter.x = InterpolateCosine(sourceCamera->targetCenter.x, targetCamera->currentCenter.x, frameIndex, frameCount);
    targetCamera->targetCenter.y = InterpolateCosine(sourceCamera->targetCenter.y, targetCamera->currentCenter.y, frameIndex, frameCount);
    targetCamera->targetCenter.z = InterpolateCosine(sourceCamera->targetCenter.z, targetCamera->currentCenter.z, frameIndex, frameCount);
    targetCamera->targetRotation.x = InterpolateCosine(sourceCamera->targetRotation.x, targetCamera->currentRotation.x, frameIndex, frameCount);
    targetCamera->targetRotation.y = InterpolateCosine(sourceCamera->targetRotation.y, targetCamera->currentRotation.y, frameIndex, frameCount);
    targetCamera->targetRotation.z = InterpolateCosine(sourceCamera->targetRotation.z, targetCamera->currentRotation.z, frameIndex, frameCount);
    targetCamera->targetZoom = InterpolateCosine(sourceCamera->targetZoom, targetCamera->currentZoom, frameIndex, frameCount);
    sourceCamera->currentCenter.x = targetCamera->targetCenter.x;
    sourceCamera->currentCenter.y = targetCamera->targetCenter.y;
    sourceCamera->currentCenter.z = targetCamera->targetCenter.z;
    sourceCamera->currentRotation.x = targetCamera->targetRotation.x;
    sourceCamera->currentRotation.y = targetCamera->targetRotation.y;
    sourceCamera->currentRotation.z = targetCamera->targetRotation.z;
    sourceCamera->currentZoom = targetCamera->targetZoom;
}

void CosineInterpolateAndEaseCameraWorkTargets(MpexCameraWork *sourceCamera, MpexCameraWork *targetCamera, float frameIndex, float frameCount, float interpolationStep, float unusedBlendFactor)
{
    targetCamera->targetCenter.x = InterpolateCosine(sourceCamera->targetCenter.x, targetCamera->currentCenter.x, frameIndex, frameCount);
    targetCamera->targetCenter.y = InterpolateCosine(sourceCamera->targetCenter.y, targetCamera->currentCenter.y, frameIndex, frameCount);
    targetCamera->targetCenter.z = InterpolateCosine(sourceCamera->targetCenter.z, targetCamera->currentCenter.z, frameIndex, frameCount);
    targetCamera->targetRotation.x = InterpolateCosine(sourceCamera->targetRotation.x, targetCamera->currentRotation.x, frameIndex, frameCount);
    targetCamera->targetRotation.y = InterpolateCosine(sourceCamera->targetRotation.y, targetCamera->currentRotation.y, frameIndex, frameCount);
    targetCamera->targetRotation.z = InterpolateCosine(sourceCamera->targetRotation.z, targetCamera->currentRotation.z, frameIndex, frameCount);
    targetCamera->targetZoom = InterpolateCosine(sourceCamera->targetZoom, targetCamera->currentZoom, frameIndex, frameCount);
    sourceCamera->currentCenter.x = InterpolateByStep(sourceCamera->currentCenter.x, targetCamera->targetCenter.x, interpolationStep);
    sourceCamera->currentCenter.y = InterpolateByStep(sourceCamera->currentCenter.y, targetCamera->targetCenter.y, interpolationStep);
    sourceCamera->currentCenter.z = InterpolateByStep(sourceCamera->currentCenter.z, targetCamera->targetCenter.z, interpolationStep);
    sourceCamera->currentRotation.x = InterpolateByStep(sourceCamera->currentRotation.x, targetCamera->targetRotation.x, interpolationStep);
    sourceCamera->currentRotation.y = InterpolateByStep(sourceCamera->currentRotation.y, targetCamera->targetRotation.y, interpolationStep);
    sourceCamera->currentRotation.z = InterpolateByStep(sourceCamera->currentRotation.z, targetCamera->targetRotation.z, interpolationStep);
    sourceCamera->currentZoom = InterpolateByStep(sourceCamera->currentZoom, targetCamera->targetZoom, interpolationStep);
}

void SetModelMotionWithTransition(OMOBJ *object, s32 modelIndex, s32 motionIndex, s32 transitionFrames, s32 loopMode)
{
    switch (loopMode) {
        case 0:
            Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, transitionFrames, 0);
            break;
        case 1:
            Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, transitionFrames, 0x40000001);
            break;
        case 2:
            Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, transitionFrames, 0x40000002);
            break;
    }
}

void SetModelMotionWithTransitionIfChanged(OMOBJ *object, s32 modelIndex, s32 motionIndex, s32 transitionFrames, s32 loopMode)
{
    if (object->work[3] != object->mtnId[motionIndex]) {
        object->work[3] = object->mtnId[motionIndex];
        switch (loopMode) {
            case 0:
                Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, transitionFrames, 0);
                break;
            case 1:
                Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, transitionFrames, 0x40000001);
                break;
            case 2:
                Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, transitionFrames, 0x40000002);
                break;
        }
    }
}

void EmptyCameraHelper(void) { }

void MoveModelTowardPosition(OMOBJ *object, s32 modelIndex, Vec targetPosition, float startHeight, float movementSpeed, float rotationStep, s32 movementFrames, s32 rotateWhileMoving)
{
    Vec modelPosition;
    Vec currentPosition;
    Vec destinationPosition;
    Vec targetRotation;
    Vec currentRotation;
    Vec verticalOffset;

    s32 xDirection = 0;
    s32 zDirection = 0;
    s32 rotationFrame;

    currentPosition.x = Hu3DData[object->mdlId[modelIndex]].pos.x;
    currentPosition.y = Hu3DData[object->mdlId[modelIndex]].pos.y;
    currentPosition.z = Hu3DData[object->mdlId[modelIndex]].pos.z;

    destinationPosition.x = targetPosition.x;
    destinationPosition.y = targetPosition.y;
    destinationPosition.z = targetPosition.z;

    currentRotation.x = Hu3DData[object->mdlId[modelIndex]].rot.x;
    currentRotation.y = Hu3DData[object->mdlId[modelIndex]].rot.y;
    currentRotation.z = Hu3DData[object->mdlId[modelIndex]].rot.z;

    verticalOffset.x = 0.0f;
    verticalOffset.y = startHeight;
    verticalOffset.z = 0.0f;

    if ((destinationPosition.x - currentPosition.x) >= 0.0f) {
        xDirection = 1;
    }
    else {
        xDirection = 0;
    }
    if ((destinationPosition.z - currentPosition.z) >= 0.0f) {
        zDirection = 1;
    }
    else {
        zDirection = 0;
    }
    while (movementFrames != 0) {
        WaitFrames(1);
        targetRotation.y = -((atan2d(destinationPosition.z - currentPosition.z, destinationPosition.x - currentPosition.x)) - 90.0);
        if (targetRotation.y < 0.0f) {
            targetRotation.y += 360.0f;
        }
        else if (targetRotation.y >= 360.0f) {
            targetRotation.y -= 360.0f;
        }
        if (rotateWhileMoving != 0) {
            if (targetRotation.y >= 180.0f) {
                if ((currentRotation.y - targetRotation.y) >= 180.0f) {
                    currentRotation.y -= 360.0f;
                }
                if ((currentRotation.y - targetRotation.y) < -180.0f) {
                    currentRotation.y += 360.0f;
                }
            }
            else {
                if ((currentRotation.y - targetRotation.y) > 180.0f) {
                    currentRotation.y -= 360.0f;
                }
                if ((currentRotation.y - targetRotation.y) <= -180.0f) {
                    currentRotation.y += 360.0f;
                }
            }
        }
        modelPosition.x = (currentPosition.x += (movementSpeed * sind(targetRotation.y)));
        modelPosition.z = (currentPosition.z += (movementSpeed * cosd(targetRotation.y)));
        if (xDirection == 1) {
            if (currentPosition.x >= destinationPosition.x) {
                modelPosition.x = currentPosition.x = destinationPosition.x;
                xDirection = 2;
            }
        }
        else if ((xDirection == 0) && (currentPosition.x <= destinationPosition.x)) {
            modelPosition.x = currentPosition.x = destinationPosition.x;
            xDirection = 2;
        }
        if (zDirection == 1) {
            if (currentPosition.z >= destinationPosition.z) {
                modelPosition.z = currentPosition.z = destinationPosition.z;
                zDirection = 2;
            }
        }
        else if ((zDirection == 0) && (currentPosition.z <= destinationPosition.z)) {
            modelPosition.z = currentPosition.z = destinationPosition.z;
            zDirection = 2;
        }
        if (rotateWhileMoving != 0) {
            currentRotation.y = InterpolateByStep(currentRotation.y, targetRotation.y, rotationStep);
        }
        Hu3DModelPosSet(object->mdlId[modelIndex], modelPosition.x, currentPosition.y, modelPosition.z);
        Hu3DModelRotSet(object->mdlId[modelIndex], currentRotation.x, currentRotation.y, currentRotation.z);

        if (xDirection == 2 && zDirection == 2) {
            break;
        }
    };

    currentRotation.y = Hu3DData[object->mdlId[modelIndex]].rot.y;
    if (rotateWhileMoving != 0) {
        for (rotationFrame = 0; rotationFrame <= rotationStep; rotationFrame++) {
            WaitFrames(1);
            if (verticalOffset.y >= 180.0f) {
                if ((currentRotation.y - verticalOffset.y) >= 180.0f) {
                    currentRotation.y -= 360.0f;
                }
                if ((currentRotation.y - verticalOffset.y) < -180.0f) {
                    currentRotation.y += 360.0f;
                }
            }
            else {
                if ((currentRotation.y - verticalOffset.y) > 180.0f) {
                    currentRotation.y -= 360.0f;
                }
                if ((currentRotation.y - verticalOffset.y) <= -180.0f) {
                    currentRotation.y += 360.0f;
                }
            }
            verticalOffset.z = InterpolateLinear(currentRotation.y, verticalOffset.y, rotationFrame, rotationStep);
            Hu3DModelRotSet(object->mdlId[modelIndex], currentRotation.x, verticalOffset.z, currentRotation.z);
        }
    }
}
