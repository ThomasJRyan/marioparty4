#ifndef REL_M460DLL_H
#define REL_M460DLL_H

#include "game/object.h"

typedef struct M460ViewportRect {
    float left;
    float top;
    float width;
    float height;
} M460ViewportRect;

typedef struct M460CameraWork {
    s32 isCameraActive;
    s32 cameraSortOrder;
    s32 cameraSlot;
    u32 cameraFlags;
    float fieldOfView;
    float nearClipDistance;
    float farClipDistance;
    Vec cameraCenter;
    Vec cameraRotation;
    float cameraZoom;
    s32 cameraIndex;
    s32 cameraMask;
    float viewportScale;
    M460ViewportRect viewportRect;
    char unreferencedCameraData54[0x30];
    float cameraEffectStrength;
    char unreferencedCameraData88[0x4];
    float opacityChangeRate;
    float cameraOpacity;
    s32 reservedCameraValue;
    char unreferencedCameraData98[0xC];
} M460CameraWork; /* size = 0xA4 */ // same as M443DllCameraStruct

void M460SetCameraMode(u32 cameraMode);
M460CameraWork *M460FindPrimaryCamera(s32 cameraFlags);
M460CameraWork *M460FindSecondaryCamera(s32 cameraFlags);
u32 M460GetResultState(void);
u32 M460GetRoundState(void);
s32 M460IsGameComplete(void);
void M460SetPlayerResult(s32 playerIndex, s32 playerRank, s32 playerResult);
void M460SetPlayerStateFlag(s32 playerIndex);
void M460SetCameraStateFlag(s32 cameraIndex);
void M460SetSelectionState(u32 selectionState);
s32 M460GetSelectionState(void);
void M460ConfirmSelection(void);
s32 M460IsSelectionConfirmed(void);
void M460MarkSelectionReady(void);
s32 M460IsSelectionReady(void);
u32 M460GetCurrentFrameCount(void);
float M460ApproachAngle(float currentAngle, float targetAngle, float approachRate);

void M460CreatePlayerObjects(HUPROCESS *process);
void M460DestroyPlayerObjects(void);
void M460CreateMapObject(HUPROCESS *process);
void M460DestroyMapObject(void);
void M460StartMapCameraFade(float volumeChangeRate);
void M460CreateScoreDisplay(HUPROCESS *process, s16 recordScore);
void M460DestroyScoreDisplay(void);
void M460SetScoreDisplayState(s32 displayState);

#endif
