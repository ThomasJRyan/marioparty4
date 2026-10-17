#ifndef REL_M436DLL_H
#define REL_M436DLL_H

#include "game/object.h"

#include "dolphin.h"

// For future reference: m436 has many functions and structs in common with m437.

typedef struct {
    /* 0x00 */ void (*startCallback)(void);
    /* 0x04 */ s32 (*completionCallback)(void);
    /* 0x08 */ s32 durationFrames;
} SequenceEntry; // Size 0xC

typedef struct {
    /* 0x00 */ s32 entryIndex;
    /* 0x04 */ SequenceEntry* entries;
    /* 0x08 */ s32 elapsedFrames;
    /* 0x0C */ s32 durationFrames;
    /* 0x10 */ void (*entryCallback)(void);
    /* 0x14 */ s32 (*completionCallback)(void);
} SequencePlayer; // Size 0x18

typedef struct {
    /* 0x00 */ void (*updateCallback)(void);
    /* 0x04 */ float centerX;
    /* 0x08 */ float centerY;
    /* 0x0C */ float centerZ;
    /* 0x10 */ float targetCenterX;
    /* 0x14 */ float targetCenterY;
    /* 0x18 */ float targetCenterZ;
    /* 0x1C */ float rotationX;
    /* 0x20 */ float rotationY;
    /* 0x24 */ float rotationZ;
    /* 0x28 */ float targetRotationX;
    /* 0x2C */ float targetRotationY;
    /* 0x30 */ float targetRotationZ;
    /* 0x34 */ float zoom;
    /* 0x38 */ float targetZoom;
    /* 0x3C */ float interpolationTimer;
    /* 0x40 */ s32 interpolationFrames[7];
    /* 0x5C */ float interpolationDeltas[7];
} CameraState; // Size 0x78

float WeightedAverageStep(float previousValue, float currentValue, float elapsedFrames);
float LinearInterpolate(float startValue, float endValue, float elapsedFrames, float durationFrames);
float SineEaseInterpolate(float startValue, float endValue, float elapsedFrames, float durationFrames);
float FullSineInterpolate(float startValue, float endValue, float elapsedFrames, float durationFrames);
float CosineEaseInterpolate(float startValue, float endValue, float elapsedFrames, float durationFrames);
void TurnModelTowardTarget(OMOBJ* sourceObject, s32 sourceModelSlot, OMOBJ* targetObject, s32 targetModelSlot);
s32 IsMotionAtTime(OMOBJ* characterObject, s32 modelSlot, s32 motionSlot, s32 frameThreshold, s32 checkForward);
void SetCharacterMotion(OMOBJ* characterObject, s32 modelSlot, s32 motionSlot, s32 blendFrames, s32 motionMode);
void SetCharacterMotionCached(OMOBJ* characterObject, s32 modelSlot, s32 motionSlot, s32 blendFrames, s32 motionMode);
void UpdateCharacterMotionIfChanged(OMOBJ* characterObject, s32 modelSlot, s32 motionSlot, s32 blendFrames, s32 motionMode);
void InitializeSequence(SequencePlayer* sequenceState, SequenceEntry* entries);
s32 AdvanceSequence(SequencePlayer* sequenceState);
s32 SetSequenceEntry(SequencePlayer* sequenceState, s32 entryIndex);
s32 UpdateSequenceTimer(SequencePlayer* sequenceState, s32 frameIncrement);
void CopyCameraCurrentToTarget(CameraState* cameraState);
void InterpolateCameraStateInPlace(CameraState* cameraState, float interpolationFactor);
void SineInterpolateCameraStateWithSmoothing(CameraState* currentState, CameraState* targetState, float elapsedFrames, float durationFrames, float smoothingFactor);
void CosineInterpolateCameraStateWithSmoothing(CameraState* currentState, CameraState* targetState, float elapsedFrames, float durationFrames, float smoothingFactor);

#endif
