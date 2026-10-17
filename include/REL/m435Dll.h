#ifndef REL_M435DLL_H
#define REL_M435DLL_H

#include "game/object.h"

#include "dolphin.h"

// For future reference: m435 has many functions and structs in common with m436 and m437.

typedef struct {
    /* 0x00 */ void (*onEnter)(void);
    /* 0x04 */ s32 (*isComplete)(void);
    /* 0x08 */ s32 durationFrames;
} SequenceStep; // Size 0xC

typedef struct {
    /* 0x00 */ s32 stepIndex;
    /* 0x04 */ s32 elapsedFrames;
    /* 0x08 */ s32 stepDuration;
    /* 0x0C */ SequenceStep* steps;
    /* 0x10 */ void (*onStep)(void);
    /* 0x14 */ s32 (*onComplete)(void);
} SequencePlayer; // Size 0x18

typedef struct {
    /* 0x00 */ char unusedTweenPadding[4];
    /* 0x04 */ void (*positionCallback)(void);
    /* 0x08 */ void (*rotationCallback)(void);
    /* 0x0C */ float cameraCenterX;
    /* 0x10 */ float cameraCenterY;
    /* 0x14 */ float cameraCenterZ;
    /* 0x18 */ float targetPositionX;
    /* 0x1C */ float targetPositionY;
    /* 0x20 */ float targetPositionZ;
    /* 0x24 */ float cameraRotationX;
    /* 0x28 */ float cameraRotationY;
    /* 0x2C */ float cameraRotationZ;
    /* 0x30 */ float targetRotationX;
    /* 0x34 */ float targetRotationY;
    /* 0x38 */ float targetRotationZ;
    /* 0x3C */ float cameraZoom;
    /* 0x40 */ float targetDistance;
    /* 0x44 */ float elapsedTime;
    /* 0x48 */ s32 targetMotionIds[7];
    /* 0x64 */ float targetMotionValues[7];
} TransformTween; // Size 0x80

typedef struct {
    /* 0x000 */ OMOBJ* trackingObject;
    /* 0x004 */ s32 trackingState;
    /* 0x008 */ s32 motionId;
    /* 0x00C */ char unusedTrackingPadding[4];
    /* 0x010 */ char motionName[1024]; // Actual char array (unsure about length)
    /* 0x410 */ s32 targetCount;
    /* 0x414 */ s32 targetModelIds[20];
    /* 0x464 */ s32 targetMotionIndex;
    /* 0x468 */ s32 targetModelIndex;
    /* 0x46C */ s32 targetState;
    /* 0x470 */ float targetStartValue;
    /* 0x474 */ float targetValue;
    /* 0x478 */ float targetDuration;
    /* 0x47C */ float targetElapsedTime;
} MotionTargetState; // Size 0x480

float InterpolateLinearClamped(float startValue, float endValue, float elapsedFrames, float durationFrames);
float ApproachValueOverDuration(float currentValue, float targetValue, float remainingFrames);
float InterpolateSineEaseOut(float startValue, float endValue, float elapsedFrames, float durationFrames);
float InterpolateCosineEaseIn(float startValue, float endValue, float elapsedFrames, float durationFrames);
void StartSequence(SequencePlayer* sequencePlayer, SequenceStep* sequenceSteps);
s32 AdvanceSequence(SequencePlayer* sequencePlayer);
void SetSequenceStep(SequencePlayer* sequencePlayer, s32 stepIndex);
s32 UpdateSequenceTimer(SequencePlayer* sequencePlayer, s32 frameDelta);
s32 IsMotionAtFrame(OMOBJ* modelObject, s32 modelIndex, s32 motionIndex, s32 frameThreshold, s32 checkAfterFrame);
void SetModelMotion(OMOBJ* modelObject, s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 motionMode);
void SetModelMotionIfChanged(OMOBJ* modelObject, s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 motionMode);
void CopyTransformToTarget(TransformTween* tween);
void InterpolateTransformTowardSelf(TransformTween* tween, float blendFrames);
void InterpolateTransformLinearAndBlend(TransformTween* startTween, TransformTween* targetTween, float elapsedFrames, float durationFrames, float blendFrames);
void InterpolateTransformSineAndBlend(TransformTween* startTween, TransformTween* targetTween, float elapsedFrames, float durationFrames, float blendFrames);
void InterpolateTransformCosineAndBlend(TransformTween* startTween, TransformTween* targetTween, float elapsedFrames, float durationFrames, float blendFrames);
void UpdateModelTransformTweenPosition(OMOBJ* modelObject, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 unusedRotationFrames, s32 elapsedFrames, s32 durationFrames);
void UpdateModelTransformTweenPositionAndRotation(OMOBJ* modelObject, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 rotationSmoothingFrames, s32 elapsedFrames, s32 durationFrames);
void UpdateModelRotationTween(OMOBJ* modelObject, s32 modelIndex, float targetRotation, s32 smoothingFrames);
void ReserveMotionTarget(MotionTargetState* trackingState, s32 modelId);
void ReleaseMotionTarget(MotionTargetState* trackingState, s32 modelId);
void SetMotionTargetModel(MotionTargetState* trackingState, s32 modelIndex);
void ClearMotionTargetModel(MotionTargetState* trackingState, s32 unusedModelIndex);
void ResetMotionTarget(MotionTargetState* trackingState, s32 newTrackingMode);
void RunMotionTargetProcess(HUPROCESS* objectManager, MotionTargetState* trackingState, s32 motionId, char* motionName, s32 targetModelIndex, s32 trackingMode);

#endif
