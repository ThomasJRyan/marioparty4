#ifndef REL_M421DLL_H
#define REL_M421DLL_H

#include "game/object.h"

typedef struct M421DllViewport {
    float isActive;
    float sortPriority;
    float viewIndex;
    float viewFlags;
} M421DllViewport;

typedef struct M421DllCameraStruct {
    s32 isActive;
    s32 sortPriority;
    s32 viewIndex;
    u32 viewFlags;
    float fieldOfView;
    float nearClip;
    float farClip;
    Vec lookAtTarget;
    Vec rotation;
    float zoomDistance;
    s32 outputCameraIndex;
    s32 cameraId;
    float viewportScale;
    M421DllViewport viewport;
    float targetPositionX;
    float targetPositionY;
    float targetPositionZ;
    float targetPitch;
    float targetYaw;
    float targetZoom;
    char reserved6C[0x18];
    float shakeIntensity;
    char reserved88[0x4];
    float cameraVelocity;
    float cameraAcceleration;
    u32 shakeTimer;
    char reserved98[0xC];
} M421DllCameraStruct; /* size = 0xA4 */ // same as M443DllCameraStruct and M460

M421DllCameraStruct *GetRenderCameraForViewFlag(s32 cameraIndex);
u32 GetCameraMode(void);
u32 GetResultState(void);
u32 GetExitState(void);
s32 SetPlayerResultFlag(s32 playerIndex);
s32 GetWinnerPlayerId(s32 resultSlot);
void SetPlayerStatusAndOrder(s32 playerIndex, s32 playerStatus, s32 playerOrder);
void SetPlayerReadyFlag(s32 playerIndex);
void SetPlayerConfirmFlag(s32 playerIndex);
void SetPlayerExitFlag(s32 playerIndex);
s32 IsResultsSequenceFinished(void);
float InterpolateAngle(float currentAngle, float targetAngle, float interpolation);

void CreatePlayers(HUPROCESS *process);
void ResetPlayerMotion(void);
void CreateHost(HUPROCESS *process);
void EmptyHostCallback(void);
void ResetHostMotion(void);

#endif
