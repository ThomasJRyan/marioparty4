#ifndef REL_M443DLL_H
#define REL_M443DLL_H

#include "game/object.h"

typedef struct M443DllCameraState {
    s32 isEnabled;
    s32 sortPriority;
    s32 cameraIndex;
    u32 cameraMask;
    float nearPlane;
    float farPlane;
    float fieldOfView;
    Vec targetPosition;
    Vec rotation;
    float zoomDistance;
    s32 cameraSlot;
    s32 cameraId;
    float viewportScale;
    float viewportCenterX;
    float viewportCenterY;
    float viewportWidth;
    float viewportHeight;
    Vec targetOffset;
    char unreferencedCameraPadding[0xC];
    float cameraPanX;
    float cameraPanY;
    char unreferencedViewportPadding[0x4];
    float savedViewportCenterX;
    float savedViewportCenterY;
    float savedViewportScale;
    char unreferencedCameraTailPadding[0x1C];
    u32 cameraFlags;
} M443DllCameraState; /* size = 0xA4 */

void SetCameraTransitionState(u32 transitionState);
u32 GetCameraTransitionState(void);
M443DllCameraState *findResultCameraByMask(s32 cameraMask);
M443DllCameraState *getMainCamera(u32 cameraIndex);
M443DllCameraState *getResultCamera(u32 cameraIndex);
void SetModelCameraMask(s32 modelId, s32 cameraMask);
u32 GetCameraTransitionStage(void);
u32 GetResultStage(void);
u32 GetRewardSequenceState(void);
void SetPlayerScore(s32 playerIndex, float score);
s32 GetPlayerRank(s32 playerIndex);
void SetPlayerFinishEvent(s32 playerIndex);
void SetPlayerEvent(s32 playerIndex);
float InterpolateAngle(float currentAngle, float targetAngle, float interpolationFactor);
void InitializeMapObject(HUPROCESS *process);
void EmptyMapCallback(void);
void InitializePlayerRacers(HUPROCESS *process);
void EmptyPlayerCallback(void);

#endif
