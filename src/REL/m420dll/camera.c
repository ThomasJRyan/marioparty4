#include "ext_math.h"
#include "game/audio.h"
#include "game/flag.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/wipe.h"

#include "REL/m420dll.h"

HUPROCESS *cameraProcess;
M420DllCameraPose cameraPose;
M420DllCameraPose cameraStartPose;
M420DllCameraPose cameraTargetPose;
s32 cameraPositionTransitionActive;
s32 cameraAngleTransitionActive;
s32 cameraZoomTransitionActive;
float cameraPositionTransitionProgress;
float cameraAngleTransitionProgress;
float cameraZoomTransitionProgress;
float cameraPositionTransitionStep;
float cameraAngleTransitionStep;
float cameraZoomTransitionStep;

s32 InitializeCamera(void)
{
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 30.0f, 10.0f, 8000.0f, 1.2f);
    cameraPose.lookAtPosition.x = 0.0f;
    cameraPose.lookAtPosition.y = 0.0f;
    cameraPose.lookAtPosition.z = 0.0f;
    cameraPose.cameraAngles.x = -10.0f;
    cameraPose.cameraAngles.y = 0.0f;
    cameraPose.cameraAngles.z = 0.0f;
    cameraPose.cameraDistance = 1000.0f;
    cameraPositionTransitionActive = 0;
    cameraAngleTransitionActive = 0;
    cameraZoomTransitionActive = 0;
    cameraProcess = HuPrcChildCreate(UpdateCamera, 0x1F4, 0x2000, 0, minigameObjectManager);
    return 1;
}

void ShutdownCamera(void)
{
    HuPrcKill(cameraProcess);
}

void AnimateCameraTarget(const Vec *targetPosition, s32 durationFrames)
{
    cameraTargetPose.lookAtPosition = *targetPosition;
    cameraStartPose.lookAtPosition = cameraPose.lookAtPosition;
    cameraPositionTransitionProgress = 0.0f;
    cameraPositionTransitionStep = 1.0f / durationFrames;
    cameraPositionTransitionActive = 1;
}

void AnimateCameraAngles(const Vec *targetAngles, s32 durationFrames)
{
    cameraTargetPose.cameraAngles = *targetAngles;
    cameraStartPose.cameraAngles = cameraPose.cameraAngles;
    cameraAngleTransitionProgress = 0.0f;
    cameraAngleTransitionStep = 1.0f / durationFrames;
    cameraAngleTransitionActive = 1;
}

void AnimateCameraZoom(float targetDistance, s32 durationFrames)
{
    cameraTargetPose.cameraDistance = targetDistance;
    cameraStartPose.cameraDistance = cameraPose.cameraDistance;
    cameraZoomTransitionProgress = 0.0f;
    cameraZoomTransitionStep = 1.0f / durationFrames;
    cameraZoomTransitionActive = 1;
}

void UpdateCamera(void)
{
    Vec secondaryPositionVector;
    Vec positionVector;
    Vec positionVector;
    float interpolationFactor;

    while (TRUE) {
        if (cameraPositionTransitionActive != 0) {
            interpolationFactor = sind((90.0f * cameraPositionTransitionProgress));
            interpolationFactor *= interpolationFactor;
            cameraPose.lookAtPosition.x = cameraStartPose.lookAtPosition.x + (interpolationFactor * (cameraTargetPose.lookAtPosition.x - cameraStartPose.lookAtPosition.x));
            cameraPose.lookAtPosition.y = cameraStartPose.lookAtPosition.y + (interpolationFactor * (cameraTargetPose.lookAtPosition.y - cameraStartPose.lookAtPosition.y));
            cameraPose.lookAtPosition.z = cameraStartPose.lookAtPosition.z + (interpolationFactor * (cameraTargetPose.lookAtPosition.z - cameraStartPose.lookAtPosition.z));
            if ((cameraPositionTransitionProgress += cameraPositionTransitionStep) >= 1.0f) {
                cameraPose.lookAtPosition = cameraTargetPose.lookAtPosition;
                cameraPositionTransitionActive = 0;
            }
        }
        if (cameraAngleTransitionActive != 0) {
            interpolationFactor = sind((90.0f * cameraAngleTransitionProgress));
            interpolationFactor *= interpolationFactor;
            cameraPose.cameraAngles.x = cameraStartPose.cameraAngles.x + (interpolationFactor * (cameraTargetPose.cameraAngles.x - cameraStartPose.cameraAngles.x));
            cameraPose.cameraAngles.y = cameraStartPose.cameraAngles.y + (interpolationFactor * (cameraTargetPose.cameraAngles.y - cameraStartPose.cameraAngles.y));
            cameraPose.cameraAngles.z = cameraStartPose.cameraAngles.z + (interpolationFactor * (cameraTargetPose.cameraAngles.z - cameraStartPose.cameraAngles.z));
            if ((cameraAngleTransitionProgress += cameraAngleTransitionStep) >= 1.0f) {
                cameraPose.cameraAngles = cameraTargetPose.cameraAngles;
                cameraAngleTransitionActive = 0;
            }
        }
        if (cameraZoomTransitionActive != 0) {
            interpolationFactor = sind((90.0f * cameraZoomTransitionProgress));
            interpolationFactor *= interpolationFactor;
            cameraPose.cameraDistance = cameraStartPose.cameraDistance + (interpolationFactor * (cameraTargetPose.cameraDistance - cameraStartPose.cameraDistance));
            if ((cameraZoomTransitionProgress += cameraZoomTransitionStep) >= 1.0f) {
                cameraPose.cameraDistance = cameraTargetPose.cameraDistance;
                cameraZoomTransitionActive = 0;
            }
        }
        secondaryPositionVector.x = cameraPose.lookAtPosition.x + (cameraPose.cameraDistance * (sind(cameraPose.cameraAngles.y) * cosd(cameraPose.cameraAngles.x)));
        secondaryPositionVector.y = cameraPose.lookAtPosition.y + (cameraPose.cameraDistance * -sind(cameraPose.cameraAngles.x));
        secondaryPositionVector.z = cameraPose.lookAtPosition.z + (cameraPose.cameraDistance * (cosd(cameraPose.cameraAngles.y) * cosd(cameraPose.cameraAngles.x)));
        positionVector.x = sind(cameraPose.cameraAngles.y) * sind(cameraPose.cameraAngles.x);
        positionVector.y = cosd(cameraPose.cameraAngles.x);
        positionVector.z = cosd(cameraPose.cameraAngles.y) * sind(cameraPose.cameraAngles.x);
        temporaryValues = cameraPose.lookAtPosition;

        Hu3DCameraPosSet(1, secondaryPositionVector.x, secondaryPositionVector.y, secondaryPositionVector.z, positionVector.x, positionVector.y, positionVector.z, temporaryValues.x, temporaryValues.y, temporaryValues.z);
        HuPrcVSleep();
    }
}

void UpdateCameraFromController(void)
{
    cameraPose.lookAtPosition.x += (0.02f * *HuPadStkX);
    cameraPose.lookAtPosition.y += (0.02f * *HuPadStkY);
    cameraPose.cameraAngles.y -= (0.02f * *HuPadSubStkX);
    cameraPose.cameraAngles.x += (0.02f * *HuPadSubStkY);
    cameraPose.cameraDistance += (0.02f * *HuPadTrigL);
    cameraPose.cameraDistance -= (0.02f * *HuPadTrigR);
}

void DrawCameraDebugText(void)
{
    print8(0x20, 0x84, 1.5f, "LOOKAT : x=%.2f y=%.2f z=%.2f", cameraPose.lookAtPosition.x, cameraPose.lookAtPosition.y, cameraPose.lookAtPosition.z);
    print8(0x20, 0x94, 1.5f, "ANGLE  : x=%.2f y=%.2f z=%.2f", cameraPose.cameraAngles.x, cameraPose.cameraAngles.y, cameraPose.cameraAngles.z);
    print8(0x20, 0xA4, 1.5f, "ZOOM   : x=%.2f", cameraPose.cameraDistance);
}
