#ifndef REL_MSTORYDLL_H
#define REL_MSTORYDLL_H

#include "game/object.h"
#include "game/process.h"

#include "dolphin/types.h"

typedef struct {
    /* 0x00 */ void (*transitionCallback)(void);
    /* 0x04 */ void (*completionCallback)(void);
    /* 0x08 */ Vec centerPosition;
    /* 0x14 */ float targetCenterX;
    /* 0x18 */ float targetCenterY;
    /* 0x1C */ float targetCenterZ;
    /* 0x20 */ Vec rotation;
    /* 0x2C */ float targetRotationX;
    /* 0x30 */ float targetRotationY;
    /* 0x34 */ float targetRotationZ;
    /* 0x38 */ float zoom;
    /* 0x3C */ float targetZoom;
    /* 0x40 */ s32 transitionFrame;
} StoryCameraTransitionState; // Size 0x44

typedef struct {
    /* 0x00 */ s32 playerIndex;
    /* 0x04 */ s32 group;
    /* 0x08 */ s32 isComputer;
    /* 0x0C */ s32 difficulty;
    /* 0x10 */ s32 characterId;
    /* 0x14 */ s32 controllerPort;
    /* 0x18 */ s32 resultValue;
    /* 0x1C */ char unreferencedPlayerResultData[0x18];
} StoryPlayerResult; // Size 0x34

typedef struct {
    /* 0x00 */ OMOBJ* object;
    /* 0x04 */ void (*callback)(OMOBJ*, ...);
    /* 0x08 */ s32 animationPhase;
    /* 0x0C */ s32 verticalVelocity;
    /* 0x10 */ char unreferencedProcessData[0x18];
} StoryProcessTaskState; // Size 0x28

// main.c
float InterpolateLinear(float startValue, float endValue, float elapsedFrames, float durationFrames);
float InterpolateCosineEaseOut(float startValue, float endValue, float elapsedFrames, float durationFrames);
float InterpolateSineEaseOut(float startValue, float endValue, float elapsedFrames, float durationFrames);
float InterpolateSineCycle(float startValue, float endValue, float elapsedFrames, float durationFrames);
void WaitOneFrame(void);
void WaitFrames(s32 waitFrames);
void InitializeStoryWindows(void);
s32 CreateCenteredStoryWindow(s32 windowXMode, s32 windowYMode, s32 windowColumnCount, s32 windowRowCount, s32 activePlayerIndex);
void CloseCenteredStoryWindow(s32 windowId);
void WaitForStoryMessage(s32 windowId, s32 waitFrames);
BOOL WaitForStoryChoice(s32 windowId, s32 choiceIndex);
BOOL WaitForFirstStoryChoice(s32 windowId);
void SetStoryMessage(s32 windowId, s32 messageId);
void InsertStoryMessage(s32 windowId, s32 messageId, s32 choiceIndex);
void SetObjectMotion(OMOBJ* object, s32 modelSlot, s32 motionSlot, s32 blendFrames, s32 motionMode);
void WaitForObjectMotionEnd(OMOBJ* object, s32 modelSlot, s32 motionSlot, s32 sleepFrames);
void SetStoryCameraCallback(void (*callback)(void));
void InitializeStoryCamera(HUPROCESS* process, void (*callback)(void));
s32 CreateStoryChoiceWindow(s32 messageId, s32 playerIndex);
void AnimateCharacterPosition(OMOBJ* object, s32 characterIndex, Vec targetPosition, float motionSpeedParameter, float motionDurationParameter, float motionFlags);
void AnimateCharacterRotation(OMOBJ* object, s32 characterIndex, Vec destinationPosition, float targetHeadingDegrees, float turnParameter, float turnDurationFrames);
void CapturePlayerResult(StoryPlayerResult* playerResult, s32 playerOrderIndex);
s32 CreateStoryLighting(s32 lightIntensity);
void CreateStoryShadow(void);
void CaptureCameraStartState(StoryCameraTransitionState* cameraState);
void InterpolateCameraCenterSine(StoryCameraTransitionState* cameraState, float smoothingWeight);
void InterpolateCameraTransition(StoryCameraTransitionState* cameraState, StoryCameraTransitionState* targetCameraState, float elapsedFrames, float durationFrames, float secondaryDurationFrames);
void InterpolateCameraTransitionEaseOut(StoryCameraTransitionState* cameraState, StoryCameraTransitionState* targetCameraState, float elapsedFrames, float durationFrames, float secondaryDurationFrames);
void DisplayStoryMessagePages(s32 windowId, s32 messageId, s32 choiceIndex, s32 waitFrames);
void CloseStoryChoiceWindow(s32 windowId);

// board_clear.c
void StartBoardClearResult(HUPROCESS* resultProcess, s32 boardId, s32 winningPlayerIndex);

// board_miss.c
void StartBoardMissResult(HUPROCESS* resultProcess, s32 boardId, s32 winningPlayerIndex);

// mg_clear.c
void StartMinigameClearResult(HUPROCESS* resultProcess, s32 boardId, s32 winningPlayerIndex);

// mg_miss.c
void StartMinigameMissResult(HUPROCESS* resultProcess, s32 boardId, s32 winningPlayerIndex);

// save.c
void StartStorySaveSequence(HUPROCESS* process);

extern s32 currentStorySoundEffectId;
extern s32 storySequenceIds[];
extern StoryCameraTransitionState storyCameraState;
extern s32 storyEventSoundEffectIds[][4];

#endif
