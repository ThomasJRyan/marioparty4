#ifndef REL_MSTORY3DLL_H
#define REL_MSTORY3DLL_H

#include "game/object.h"
#include "game/process.h"

#include "dolphin.h"

// For future reference: seems to be identical to StoryPlayerStats in mstoryDll.
typedef struct {
    /* 0x00 */ s32 playerIndex;
    /* 0x04 */ s32 team;
    /* 0x08 */ s32 isComputer;
    /* 0x0C */ s32 difficulty;
    /* 0x10 */ s32 characterId;
    /* 0x14 */ s32 controllerPort;
    /* 0x18 */ s32 rank;
    /* 0x1C */ s32 coins;
    /* 0x20 */ s32 stars;
    /* 0x24 */ s32 minigameCoins;
    /* 0x28 */ s32 maxCoins;
    /* 0x2C */ s32 questionSpaces;
    /* 0x30 */ char unreferencedPlayerPadding[4];
} StoryPlayerStats; // Size 0x34

// For future reference: seems to be identical to StoryCameraState in mstoryDll.
typedef struct {
    /* 0x00 */ void (*callback)(void);
    /* 0x04 */ void (*debugInputCallback)(void);
    /* 0x08 */ Vec cameraCenter;
    /* 0x14 */ float targetCenterX;
    /* 0x18 */ float targetCenterY;
    /* 0x1C */ float targetCenterZ;
    /* 0x20 */ Vec cameraRotation;
    /* 0x2C */ float targetRotationX;
    /* 0x30 */ float targetRotationY;
    /* 0x34 */ float targetRotationZ;
    /* 0x38 */ float cameraZoom;
    /* 0x3C */ float targetCameraZoom;
    /* 0x40 */ s32 cameraMotionFrame;
} StoryCameraState; // Size 0x44

typedef struct {
    /* 0x00 */ OMOBJ* object;
    /* 0x04 */ void (*callback)(OMOBJ*, ...);
    /* 0x08 */ s32 workValue0;
    /* 0x0C */ s32 workValue1;
    /* 0x10 */ s32 workValue2;
    /* 0x14 */ s32 spriteId;
    /* 0x18 */ s32 callbackState;
    /* 0x1C */ s32 callbackResultState;
    /* 0x20 */ char unreferencedProcessPadding[8];
} StorySequenceProcessState; // Size 0x28

// main.c
float InterpolateLinearOverTime(float startValue,    float targetValue,    float frame,    float duration);
float InterpolateSmoothAverage(float startValue,    float targetValue,    float frame);
float InterpolateCosineEaseOut(float startValue,    float targetValue,    float frame,    float duration);
float InterpolateSineEaseIn(float startValue,    float targetValue,    float frame,    float duration);
float InterpolateSineEaseOut(float startValue,    float targetValue,    float frame,    float duration);
float InterpolateSinePulse(float startValue,    float targetValue,    float frame,    float duration);
void Story3SleepOneFrame(void);
void Story3SleepFrames(s32 frameCount);
void Story3InitializeWindows(void);
void Story3KillAllWindows(void);
s32 Story3CreateStandardMessageWindow(s32 windowId,    s32 messageId,    s32 mode,    s32 mode3,    s32 mode4);
s32 Story3CreateDataWindow(u32 windowId,    s32 messageId);
void Story3CleanupWindow(s32 windowId);
void Story3ShowCenteredMessage(s32 windowId);
void Story3DisplayMessageAndWait(s32 windowId,    s32 messageId);
void Story3SetWindowMessage(s32 windowId,    s32 messageId);
void Story3SetModelMotion(OMOBJ* object,    s32 messageId,    s32 mode,    s32 mode3,    s32 mode4);
void Story3WaitForModelMotionEnd(OMOBJ* object,    s32 messageId,    s32 mode,    s32 mode3);
void Story3WaitForModelMotionNearEnd(OMOBJ* object,    s32 messageId,    s32 mode,    s32 mode3);
void Story3WaitForControllerButton(s32 waitFrames,    s32 messageId);
void Story3SetCameraCallback(void (*input)(void));
void Story3InitializeResultCamera(HUPROCESS* objectManager,    void (*input2)(void));
void Story3UpdateObjectTransform(OMOBJ* object,    s32 messageId,    Vec input3,    float duration,    float duration4,    float duration5);
void Story3ConfigureModelMaterials(s16 modelIndex);
void Story3CopyPlayerStats(StoryPlayerStats* playerStats,    s32 count);
s32 Story3CountHumanPlayers(void);
s32 Story3FindFirstHumanPlayer(void);
s32 Story3CreateGlobalLight(s32 lightId);
void Story3SetDefaultCamera(void);
void Story3ResetCameraTarget(StoryCameraState* cameraState);
void Story3LerpAndSmoothCamera(StoryCameraState* cameraState,    StoryCameraState* targetCameraState,    float frame,    float duration,    float duration4);
void Story3ApplyEasedCameraTarget(StoryCameraState* cameraState,    StoryCameraState* targetCameraState,    float frame,    float duration);
void Story3EaseAndSmoothCamera(StoryCameraState* cameraState,    StoryCameraState* targetCameraState,    float frame,    float duration,    float duration4);

// result_seq.c
void Story3InitializeResultSequence(HUPROCESS* objectManager, s32 boardId, s32 bonusStarMode, s32 sequenceVariant, s32 playerPresentationMode);

// result.c
void Story3StartBoardResult(HUPROCESS* objectManager,    s32 count,    s32 mode,    s32 mode3);

extern StoryCameraState storyCameraState;
extern s32 recordMessageIdTable[][4];

#endif
