#ifndef REL_MSTORY2DLL_H
#define REL_MSTORY2DLL_H

#include "game/object.h"
#include "game/process.h"

#include "dolphin/types.h"

typedef struct {
    /* 0x00 */ void (*cameraUpdateCallback)(void);
    /* 0x04 */ void (*cameraResetCallback)(void);
    /* 0x08 */ Vec cameraCenter;
    /* 0x14 */ float startCenterX;
    /* 0x18 */ float startCenterY;
    /* 0x1C */ float startCenterZ;
    /* 0x20 */ Vec cameraRotation;
    /* 0x2C */ float startRotationX;
    /* 0x30 */ float startRotationY;
    /* 0x34 */ float startRotationZ;
    /* 0x38 */ float cameraZoom;
    /* 0x3C */ float startZoom;
    /* 0x40 */ s32 animationFrame;
} StoryCameraMotionState; // Size 0x44

typedef struct {
    /* 0x00 */ s32 playerIndex;
    /* 0x04 */ s32 group;
    /* 0x08 */ s32 isComputer;
    /* 0x0C */ s32 difficulty;
    /* 0x10 */ s32 characterId;
    /* 0x14 */ s32 padIndex;
    /* 0x18 */ s32 rank;
    /* 0x1C */ char reserved1C[0x18];
} StoryPlayerConfig; // Size 0x34

typedef struct {
    /* 0x00 */ OMOBJ* object;
    /* 0x04 */ void (*updateCallback)(OMOBJ*, ...);
    /* 0x08 */ s32 animationFrame;
    /* 0x0C */ s32 auxiliaryValue;
    /* 0x10 */ char reserved10[0x18];
} StoryObjectProcessState; // Size 0x28

// main.c
float InterpolateLinear(float startValue, float targetValue, float elapsedFrames, float durationFrames);
float InterpolateWeightedAverage(float currentValue, float targetValue, float sampleCount);
float InterpolateCosineEaseOut(float startValue, float targetValue, float elapsedFrames, float durationFrames);
float InterpolateSineEaseIn(float startValue, float targetValue, float elapsedFrames, float durationFrames);
float InterpolateSinePulse(float startValue, float targetValue, float elapsedFrames, float durationFrames);
float InterpolateSineCycle(float startValue, float targetValue, float elapsedFrames, float durationFrames);
void WaitOneFrame(void);
void WaitFrames(s32 frameCount);
void InitializeWindowSystem(void);
s32 CreateCenteredMessageWindow(s32 left, s32 top, s32 widthCells, s32 heightCells, s32 padPlayerIndex);
void CloseCenteredMessageWindow(s32 windowId);
void WaitForWindowMessage(s32 windowId, s32 delayFrames);
BOOL GetWindowChoice(s32 windowId, s32 defaultChoice);
BOOL GetDefaultWindowChoice(s32 windowId);
void SetWindowMessage(s32 windowId, s32 messageId);
void InsertWindowMessage(s32 windowId, s32 messageId, s32 insertIndex);
void ShiftObjectMotion(OMOBJ* object, s32 modelSlot, s32 motionSlot, s32 blendFrames, s32 motionFlags);
void WaitForObjectMotionEnd(OMOBJ* object, s32 modelSlot, s32 motionSlot, s32 initialDelayFrames);
void SetCameraUpdateCallback(void (*callback)(void));
void InitializeStoryCamera(HUPROCESS* parentProcess, void (*cameraUpdateCallback)(void));
void MoveModelWithJump(OMOBJ* object, s32 modelSlot, Vec targetPosition, float rotationDegrees, float initialWaitFrames, float landingWaitFrames);
void MoveModelToPosition(OMOBJ* object, s32 modelSlot, Vec targetPosition, float rotationDegrees, float rotationDuration, float finalDuration);
void ReadStoryPlayerConfig(StoryPlayerConfig* playerConfig, s32 playerIndex);
void EaseCameraMotionLinear(StoryCameraMotionState* sourceCameraState, StoryCameraMotionState* targetCameraState, float elapsedFrames, float durationFrames, float smoothingFrames);
s32 CreateStoryDirectionalLight(s32 lightIntensity);
void CreateStoryShadow(void);
void CaptureCameraMotionStart(StoryCameraMotionState* cameraState);
void EaseCameraMotionInPlace(StoryCameraMotionState* cameraState, float smoothingFrames);
void EaseCameraMotionSineIn(StoryCameraMotionState* sourceCameraState, StoryCameraMotionState* targetCameraState, float elapsedFrames, float durationFrames, float smoothingFrames);
void EaseCameraMotionCosine(StoryCameraMotionState* sourceCameraState, StoryCameraMotionState* targetCameraState, float elapsedFrames, float durationFrames, float smoothingFrames);
int GetModelMotionDuration(int modelId);

void UpdateObjectMotionTransition(OMOBJ* object);
void SetObjectMotionTransition(OMOBJ* object, s32 motionSlot, s32 nextMotionSlot, s32 blendFrames, s32 motionFlags);
void WaitForWindowMessageAndMotion(s32 windowId, s32 messageId, s32 highlightKeyIndex, s32 modelId, s32 nextMotionId);
void WaitForWindowMessageAndMotionSimple(s32 windowId, s32 messageId, s32 highlightKeyIndex, s32 modelId);
void WaitForWindowMessageDuration(s32 windowId, s32 messageId, s32 highlightKeyIndex, s32 delayFrames);
s32 CreateAutoSizedMessageWindow(s32 messageId, s32 padPlayerIndex);
void CloseAutoSizedMessageWindow(s32 windowId);
void ApplyModelObjectTransform(s16 modelId, char *objectName, u32 constFlags, float offset);
void AnimateCharacterMeshOffset(int modelId, int characterId, float offset, s32 frameCount, BOOL moveOutward);

// board_clear.c
void StartBoardEntranceEvent(HUPROCESS* parentProcess, s32 activePadIndex);

// board_clear.c
void StartBoardClearEvent(HUPROCESS* parentProcess, s32 activePadIndex);

// board_miss.c
void StartBoardMissEvent(HUPROCESS* parentProcess, s32 activePadIndex);

// mg_clear.c
void StartMinigameClearEvent(HUPROCESS* parentProcess, s32 activePadIndex);

// mg_miss.c
void StartMinigameMissEvent(HUPROCESS* parentProcess, s32 activePadIndex);

// ending.c
void StartStoryEnding(HUPROCESS* parentProcess, s32 activePadIndex);

// ending.c
void StartStorySaveProcess(HUPROCESS* parentProcess);

extern s32 storySoundEffectIds[16];
extern s32 storyAudioSequenceIds[4];
extern s32 unreferencedStoryScratch68[];
extern StoryCameraMotionState storyCameraState;
extern s32 storyAudioCueIds[][4];

#endif
