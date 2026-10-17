#include "REL/m447dll.h"
#include "game/hu3d.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/process.h"

#include "ext_math.h"

typedef struct {
    /* 0x00 */ Vec position;
    /* 0x0C */ Vec lookDirection;
    /* 0x18 */ float zoom;
} M447CameraState; // Size 0x1C

void M447CameraUpdate(void);
void M447CameraDebugControl(void);
void M447CameraDebugPrint(void);

HUPROCESS* cameraProcess;
M447CameraState currentCamera;
M447CameraState cameraStart;
M447CameraState cameraTarget;
s32 positionTransitionActive;
s32 lookTransitionActive;
s32 zoomTransitionActive;
float positionTransitionProgress;
float lookTransitionProgress;
float zoomTransitionProgress;
float positionTransitionStep;
float lookTransitionStep;
float zoomTransitionStep;

s32 M447CameraInit(void) {
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 55.0f, 10.0f, 8000.0f, 1.2f);
    currentCamera.position.x = 0.0f;
    currentCamera.position.y = 400.0f;
    currentCamera.position.z = 0.0f;
    currentCamera.lookDirection.x = 0.0f;
    currentCamera.lookDirection.y = 0.0f;
    currentCamera.lookDirection.z = 0.0f;
    currentCamera.zoom = 200.0f;
    positionTransitionActive = 0;
    lookTransitionActive = 0;
    zoomTransitionActive = 0;
    cameraProcess = HuPrcChildCreate(M447CameraUpdate, 100, 0x2000, 0, objectManager);
    return 1;
}

void M447CameraShutdown(void) {
    HuPrcKill(cameraProcess);
}

void M447CameraPositionTargetSet(const Vec* target, s32 duration) {
    cameraTarget.position = *target;
    cameraStart.position = currentCamera.position;
    positionTransitionProgress = 0.0f;
    positionTransitionStep = 1.0f / duration;
    positionTransitionActive = 1;
}

void M447CameraLookTargetSet(const Vec* target, s32 duration) {
    cameraTarget.lookDirection = *target;
    cameraStart.lookDirection = currentCamera.lookDirection;
    lookTransitionProgress = 0.0f;
    lookTransitionStep = 1.0f / duration;
    lookTransitionActive = 1;
}

void M447CameraZoomTargetSet(float zoom, s32 duration) {
    cameraTarget.zoom = zoom;
    cameraStart.zoom = currentCamera.zoom;
    zoomTransitionProgress = 0.0f;
    zoomTransitionStep = 1.0f / duration;
    zoomTransitionActive = 1;
}

void M447CameraPositionGet(Vec* position) {
    *position = currentCamera.position;
}

void M447CameraPositionSet(Vec* position) {
    currentCamera.position = *position;
}

void M447CameraUpdate(void) {
    Vec cursorPosition;
    Vec playerPosition;
    Vec scaleVector;
    float distance;

    while (TRUE) {
        if (positionTransitionActive != 0) {
            distance = sind(90.0f * positionTransitionProgress);
            distance *= distance;
            currentCamera.position.x = cameraStart.position.x + distance * (cameraTarget.position.x - cameraStart.position.x);
            currentCamera.position.y = cameraStart.position.y + distance * (cameraTarget.position.y - cameraStart.position.y);
            currentCamera.position.z = cameraStart.position.z + distance * (cameraTarget.position.z - cameraStart.position.z);
            if ((positionTransitionProgress += positionTransitionStep) >= 1.0f) {
                currentCamera.position = cameraTarget.position;
                positionTransitionActive = 0;
            }
        }
        if (lookTransitionActive != 0) {
            distance = sind(90.0f * lookTransitionProgress);
            distance *= distance;
            currentCamera.lookDirection.x = cameraStart.lookDirection.x + distance * (cameraTarget.lookDirection.x - cameraStart.lookDirection.x);
            currentCamera.lookDirection.y = cameraStart.lookDirection.y + distance * (cameraTarget.lookDirection.y - cameraStart.lookDirection.y);
            currentCamera.lookDirection.z = cameraStart.lookDirection.z + distance * (cameraTarget.lookDirection.z - cameraStart.lookDirection.z);
            if ((lookTransitionProgress += lookTransitionStep) >= 1.0f) {
                currentCamera.lookDirection = cameraTarget.lookDirection;
                lookTransitionActive = 0;
            }
        }
        if (zoomTransitionActive != 0) {
            distance = sind(90.0f * zoomTransitionProgress);
            distance *= distance;
            currentCamera.zoom = cameraStart.zoom + distance * (cameraTarget.zoom - cameraStart.zoom);
            if ((zoomTransitionProgress += zoomTransitionStep) >= 1.0f) {
                currentCamera.zoom = cameraTarget.zoom;
                zoomTransitionActive = 0;
            }
        }
        cursorPosition.x = currentCamera.position.x + sind(currentCamera.lookDirection.y) * cosd(currentCamera.lookDirection.x) * currentCamera.zoom;
        cursorPosition.y = currentCamera.position.y + -sind(currentCamera.lookDirection.x) * currentCamera.zoom;
        cursorPosition.z = currentCamera.position.z + cosd(currentCamera.lookDirection.y) * cosd(currentCamera.lookDirection.x) * currentCamera.zoom;
        scaleVector.x = sind(currentCamera.lookDirection.y) * sind(currentCamera.lookDirection.x);
        scaleVector.y = cosd(currentCamera.lookDirection.x);
        scaleVector.z = cosd(currentCamera.lookDirection.y) * sind(currentCamera.lookDirection.x);
        playerPosition = currentCamera.position;
        Hu3DCameraPosSet(1, cursorPosition.x, cursorPosition.y, cursorPosition.z, scaleVector.x, scaleVector.y, scaleVector.z, playerPosition.x, playerPosition.y, playerPosition.z);
        HuPrcVSleep();
    }
}

void M447CameraDebugControl(void) {
    if (HuPadBtn[0] & 4) {
        currentCamera.position.x += 0.02f * HuPadStkX[0];
        currentCamera.position.y += 0.02f * HuPadStkY[0];
        currentCamera.lookDirection.y -= 0.02f * HuPadSubStkX[0];
        currentCamera.lookDirection.x += 0.02f * HuPadSubStkY[0];
        currentCamera.zoom += 0.02f * HuPadTrigL[0];
        currentCamera.zoom -= 0.02f * HuPadTrigR[0];
    }
}

void M447CameraDebugPrint(void) {
    print8(32, 132, 1.5f, "LOOKAT : x=%.2f y=%.2f z=%.2f", currentCamera.position.x, currentCamera.position.y, currentCamera.position.z);
    print8(32, 148, 1.5f, "ANGLE  : x=%.2f y=%.2f z=%.2f", currentCamera.lookDirection.x, currentCamera.lookDirection.y, currentCamera.lookDirection.z);
    print8(32, 164, 1.5f, "ZOOM   : x=%.2f", currentCamera.zoom);
}
