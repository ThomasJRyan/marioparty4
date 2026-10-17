#ifndef REL_M437DLL_H
#define REL_M437DLL_H

#include "game/object.h"

#include "dolphin.h"

typedef struct {
    /* 0x00 */ void (*motionCallback)(void);
    /* 0x04 */ s32 (*completionCondition)(void);
    /* 0x08 */ s32 duration;
} MotionSequenceEntry; // Size 0xC

typedef struct {
    /* 0x00 */ s32 entryIndex;
    /* 0x04 */ MotionSequenceEntry* entries;
    /* 0x08 */ void (*motionCallback)(void);
    /* 0x0C */ s32 (*completionCondition)(void);
    /* 0x10 */ s32 frameCounter;
    /* 0x14 */ s32 duration;
} MotionSequenceState; // Size 0x18

typedef struct {
    /* 0x00 */ void (*updateCallback)(void);
    /* 0x04 */ void (*inputCallback)(void);
    /* 0x08 */ float cameraCenterX;
    /* 0x0C */ float cameraCenterY;
    /* 0x10 */ float cameraCenterZ;
    /* 0x14 */ float targetCenterX;
    /* 0x18 */ float targetCenterY;
    /* 0x1C */ float targetCenterZ;
    /* 0x20 */ float cameraRotationX;
    /* 0x24 */ float cameraRotationY;
    /* 0x28 */ float cameraRotationZ;
    /* 0x2C */ float targetRotationX;
    /* 0x30 */ float targetRotationY;
    /* 0x34 */ float targetRotationZ;
    /* 0x38 */ float cameraZoom;
    /* 0x3C */ float targetZoom;
    /* 0x40 */ s32 animationFrame;
    /* 0x44 */ s32 shakeDuration[7];
    /* 0x60 */ float cameraShake[7];
} CameraState; // Size 0x7C

typedef struct {
    /* 0x00 */ s32 enabled;
    /* 0x04 */ char effectName[128];
    /* 0x84 */ s32 effectType;
    /* 0x88 */ s32 effectLifetime;
    /* 0x8C */ s32 effectFlags;
    /* 0x90 */ s32 activeEffectId;
    /* 0x94 */ s32 effectIds[20];
    /* 0xE4 */ OMOBJ* effectObject;
} CharacterEffectState; // Size 0xE8

float LerpClamped(float startValue, float endValue, float frame, float duration);
float WeightedAverage(float currentValue, float targetValue, float weight);
float EaseInSine(float startValue, float endValue, float frame, float duration);
float EaseInOutSine(float startValue, float endValue, float frame, float duration);
float EaseOutSine(float startValue, float endValue, float frame, float duration);
void InitializeMotionSequence(MotionSequenceState* sequence, MotionSequenceEntry* entries);
s32 AdvanceMotionSequence(MotionSequenceState* sequence);
s32 SetMotionSequenceIndex(MotionSequenceState* sequence, s32 entryIndex);
s32 AdvanceMotionSequenceTimer(MotionSequenceState* sequence, s32 frameDelta);
s32 IsMotionAtTimeThreshold(OMOBJ* object, s32 modelIndex, s32 motionIndex, s32 frameThreshold, s32 compareAfter);
void SetModelMotion(OMOBJ* object, s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 motionMode);
void EnsureModelMotion(OMOBJ* object, s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 motionMode);
s32 MoveCharacterTowardPosition(OMOBJ* object, Vec targetPosition, float targetRotation, float moveSpeed, float stopDistance, float turnSmoothing, s32 motionMode);
void ResetCameraTarget(CameraState* camera);
void ApplyCameraShake(CameraState* camera, float shakeAmount);
void BlendCameraTargetWithShake(CameraState* camera, CameraState* targetCamera, float frame, float duration, float shakeAmount);
void SmoothCameraTargetWithShake(CameraState* camera, CameraState* targetCamera, float frame, float duration, float shakeAmount);
void AddCharacterEffect(CharacterEffectState* effectState, s32 effectId);
void ResetCharacterEffects(CharacterEffectState* effectState);
void StartCharacterEffect(HUPROCESS* process, CharacterEffectState* effectState, s32 effectType, const char* effectName);
void StartCharacterEffectForPlayer(HUPROCESS* process, CharacterEffectState* effectState, s32 effectType, s32 playerIndex);

#endif
