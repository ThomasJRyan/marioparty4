#ifndef REL_MENTDLL_H
#define REL_MENTDLL_H

#include "game/object.h"
extern s32 menuSoundEffectIds[][4];

// the prev and current struct member might be vice versa
typedef struct MentDllCameraWork {
    /* 0x00 */ void (*cameraUpdateCallback)(void);
    /* 0x04 */ void (*debugControlCallback)(float unused);
    /* 0x08 */ Vec center;
    /* 0x14 */ Vec prevCenter;
    /* 0x20 */ Vec rot;
    /* 0x2C */ Vec prevRot;
    /* 0x38 */ float zoom;
    /* 0x3C */ float prevZoom;
    /* 0x40 */ s32 transitionFrame;
} MentDllCameraWork; /* size = 0x44 */
extern MentDllCameraWork menuCameraWork;

float MentInterpolateLinear(float startValue, float endValue, float elapsedFrames, float durationFrames);
float MentApproachValue(float currentValue, float targetValue, float smoothingDivisor);
float MentInterpolateCosineEaseOut(float startValue, float endValue, float elapsedFrames, float durationFrames);
float MentInterpolateSineEaseIn(float startValue, float endValue, float elapsedFrames, float durationFrames);
float MentInterpolateSineWave(float centerValue, float amplitudeTarget, float elapsedFrames, float durationFrames);
void MentSleepTicks(s32 tickCount);
void MentSleepFrame(void);
void MentCreateDirectionalLight(void);
void MentCreateMenuShadow(s32 gameMode);
void MentInitializeWindowSystem(void);
s32 MentCreateStandardMessageWindow(s32 alignment, s32 messageId, s32 windowStyle);
void MentCloseMessageWindow(s32 windowId);
void MentSetWindowMessage(s32 windowId, s32 messageId, s32 waitFrames, s32 style);
s32 MentShowYesNoPrompt(s32 messageId, s32 promptStyle, s32 defaultChoice);
s32 MentShowAlternateYesNoPrompt(s32 messageId, s32 promptStyle, s32 defaultChoice);
s32 MentShowMessageWindow(s32 messageId);
void MentClosePersistentMessageWindow(s32 unused);
void MentSetMenuCameraCallback(void (*cameraUpdateCallback)(void));
void MentCreateMenuCamera(HUPROCESS *objectManagerProcess, void (*cameraUpdateCallback)(void));
void MentSetCameraState(MentDllCameraWork *camera);
void MentAnimateCameraState(MentDllCameraWork *sourceCamera, MentDllCameraWork *targetCamera, float elapsedFrames, float durationFrames, float smoothingDivisor);
void MentAnimateCameraZoom(MentDllCameraWork *sourceCamera, MentDllCameraWork *targetCamera, float elapsedFrames, float durationFrames, float smoothingDivisor);
void MentSetModelMotion(OMOBJ *object, s32 modelIndex, s32 motionIndex, s32 transitionFrames, s32 motionMode);
void MentSetModelMotionIfChanged(OMOBJ *object, s32 modelIndex, s32 motionIndex, s32 transitionFrames, s32 motionMode);
void MentUpdateModelMotionTimer(OMOBJ *object);
void MentWaitForModelMotion(OMOBJ *object, s32 modelIndex, s32 waitFrames);
void MentMoveAndTurnModelToTarget(OMOBJ *object, s32 modelIndex, Vec targetPosition, float targetYaw, float moveSpeed, float rotationFrames, s32 moveEnabled, s32 turnEnabled);
void MentSetSpritePosition(s32, s32, float, float);
void MentInitializeModeScene(HUPROCESS *objectManagerProcess);

#endif
