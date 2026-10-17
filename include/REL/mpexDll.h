#ifndef REL_MPEXDLL_H
#define REL_MPEXDLL_H

#include "game/object.h"

typedef void (*CameraInputCallback)(void);

typedef struct MpexCameraWork {
    /* 0x00 */ CameraInputCallback updateCameraInput;
    /* 0x04 */ void *controlCamera; // func, unknown args
    /* 0x08 */ Vec currentCenter;
    /* 0x14 */ Vec targetCenter;
    /* 0x20 */ Vec currentRotation;
    /* 0x2C */ Vec targetRotation;
    /* 0x38 */ float currentZoom;
    /* 0x3C */ float targetZoom;
    /* 0x40 */ s32 interpolationState;
} MpexCameraWork; /* size = 0x44 */

void CreateOverlayShadow(void);
void CreateDirectionalLight(void);
void CreateOverlayCamera(CameraInputCallback cameraInputCallback);
void SetModelMotionWithTransition(OMOBJ *object, s32 modelIndex, s32 motionIndex, s32 transitionFrames, s32 loopMode);
void MoveModelTowardPosition(OMOBJ *object, s32 modelIndex, Vec targetPosition, float startHeight, float movementSpeed, float rotationStep, s32 movementFrames, s32 rotateWhileMoving);
HUPROCESS *InitializeMinigamePracticeOverlay(HUPROCESS *parentProcess);

void CreateCharacterSelectionProcesses(HUPROCESS *parentProcess);
s32 InitializeCharacterSelection(s32 gameMode, s32 loadSavedSettings);
void SelectCharacterMode(s32 selectionResult);
void SelectDifficultyMode(s32 selectionResult);
void UpdateCharacterSelectionDisplay(s32 displayMode);
float CalculateSineWaveValue(float startValue, float endValue, float currentFrame, float duration);
void InitializeMessageWindows(void);
void CreateCharacterSelectMessageWindow(void);
void DestroyCharacterSelectMessageWindow(void);
void ShowCharacterSelectMessage(u32 messageId, s32 waitFrames, s32 instant);
void InsertCharacterSelectMessage(u32 messageId, s32 insertIndex);
s32 ShowCharacterSelectChoice(s32 timeoutFrames);
void ShowCharacterSelectWindowMessage(u32 messageId, s32 windowPlacement);
void ShowCharacterSelectTopMessage(u32 messageId);
void DestroyCharacterSelectTextWindow(void);

void WaitFrames(s32 frameCount);

s32 CreateMinigameNameSprites(s32 messageId);

extern u8 minigameNameLanguage;
extern s32 characterSelectSpriteGroups[0x1C];
extern MpexCameraWork cameraWork;

#endif
