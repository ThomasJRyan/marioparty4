#include "REL/m437Dll.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#include "math.h"
#include "string.h"
#include "version.h"

s32 rand8(void);
#ifndef __MWERKS__
#include "game/audio.h"
#endif

typedef struct {
    /* 0x000 */ OMOBJ *characterObject;
    /* 0x004 */ s32 modelVisible;
    /* 0x008 */ s32 fadeState;
    /* 0x00C */ s32 actionState;
    /* 0x010 */ s32 actionMode;
    /* 0x014 */ s32 selectedAction;
    /* 0x018 */ char reservedStatePadding[4];
    /* 0x01C */ s32 actionTimer;
    /* 0x020 */ s32 actionResult;
    /* 0x024 */ s32 actionDelay;
    /* 0x028 */ s32 actionFlag;
    /* 0x02C */ s32 playerIndex;
    /* 0x030 */ s32 playerGroup;
    /* 0x034 */ s32 isCpu;
    /* 0x038 */ s32 aiDifficulty;
    /* 0x03C */ s32 characterId;
    /* 0x040 */ s32 controllerIndex;
    /* 0x044 */ float materialOpacity;
    /* 0x048 */ float jumpVelocity;
    /* 0x04C */ float groundHeight;
    /* 0x050 */ CharacterEffectState characterEffects;
} CharacterActionState; // Size 0x138

typedef struct {
    /* 0x00 */ OMOBJ *mainObject;
    /* 0x04 */ s32 sceneState;
    /* 0x08 */ s32 sceneSubstate;
    /* 0x0C */ s32 sceneTimer;
    /* 0x10 */ s32 sceneEventState;
    /* 0x14 */ s32 sceneEventTimer;
    /* 0x18 */ s32 sceneActionState;
    /* 0x1C */ s32 dialogueState;
    /* 0x20 */ s32 dialogueTimer;
} SceneDialogueState; // Size 0x24

typedef struct {
    /* 0x00 */ s32 modelDataId;
    /* 0x04 */ s32 motionMode;
    /* 0x08 */ s32 jointMotionDataId;
    /* 0x0C */ s32 shadowMode;
    /* 0x10 */ s32 hiddenOnStart;
} CharacterModelSetup; // Size 0x14

typedef struct {
    /* 0x00 */ OMOBJ *characterObject;
    /* 0x04 */ s32 animationState;
    /* 0x08 */ s32 characterModeFlag;
    /* 0x0C */ s32 characterSubstate;
    /* 0x10 */ s32 characterMode;
    /* 0x14 */ s32 animationTimer;
    /* 0x18 */ s32 animationRepeatCount;
    /* 0x1C */ s32 characterAction;
    /* 0x20 */ s32 characterEvent;
    /* 0x24 */ float animationBlend;
} CharacterAnimationState; // Size 0x28

typedef struct {
    /* 0x000 */ OMOBJ *characterObject;
    /* 0x004 */ s32 animationIndex;
    /* 0x008 */ s32 animationState;
    /* 0x00C */ s32 animationMode;
    /* 0x010 */ s32 animationTimer;
    /* 0x014 */ s32 animationSubstate;
    /* 0x018 */ s32 animationRepeatCount;
    /* 0x01C */ s32 secondaryAnimationTimer;
    /* 0x020 */ s32 animationFlags;
    /* 0x024 */ CharacterEffectState characterEffects;
} CharacterMotionState; // Size 0x10C

typedef struct {
    /* 0x00 */ OMOBJ *characterObject;
    /* 0x04 */ s32 sceneState;
    /* 0x08 */ s32 sceneEventState;
    /* 0x0C */ s32 sceneSubstate;
    /* 0x10 */ s32 sceneFrameCount;
    /* 0x14 */ s32 sceneEventTimer;
    /* 0x18 */ s32 sceneActionState;
    /* 0x1C */ s32 cameraAnimationFrame;
    /* 0x20 */ s32 sceneFlags;
} CharacterSceneState; // Size 0x24

typedef struct {
    /* 0x00 */ OMOBJ *characterObject;
    /* 0x04 */ s32 sceneSubstate;
    /* 0x08 */ s32 scenePhase;
    /* 0x0C */ s32 sceneFrameCount;
    /* 0x10 */ s32 sceneEvent;
    /* 0x14 */ s32 sceneEventCount;
    /* 0x18 */ s32 sceneFlags;
    /* 0x1C */ s32 sceneMode;
    /* 0x20 */ s32 sceneValue;
    /* 0x24 */ float characterMotionTimes[7];
    /* 0x40 */ char reservedMotionPadding0[4];
    /* 0x44 */ float characterMotionDurations[7];
    /* 0x60 */ char reservedMotionPadding1[4];
    /* 0x64 */ float characterMotionThresholds[7];
    /* 0x80 */ char reservedMotionPadding2[4];
    /* 0x84 */ float characterMotionMarkers[7];
    /* 0xA0 */ char reservedMotionPadding3[4];
    /* 0xA4 */ float characterMotionEvents[7];
    /* 0xC0 */ char reservedMotionPadding4[4];
    /* 0xC4 */ float characterMotionSoundTimes[7];
    /* 0xE0 */ char reservedMotionPadding5[4];
} CharacterMotionTimeline; // Size 0xE4

typedef struct {
    /* 0x00 */ s32 roundPhase;
    /* 0x04 */ s32 roundPhaseStep;
    /* 0x08 */ s32 roundEvent;
    /* 0x0C */ s32 roundFrameCount;
    /* 0x10 */ s32 currentPlayerIndex;
    /* 0x14 */ s32 roundState;
    /* 0x18 */ s32 roundSubstate;
    /* 0x1C */ s32 roundEventState;
    /* 0x20 */ s32 roundEventCount;
} MinigameRoundState; // Size 0x24

typedef struct {
    /* 0x00 */ s32 currentSequenceId;
    /* 0x04 */ s32 resultSequenceId;
    /* 0x08 */ char reservedAudioState[0x38];
} AudioSequenceState; // Size 0x40

void MonitorCharacterMotionSoundCues(void);
void CreateSceneLight(void);
void StartCameraShake(s32 axisIndex, s32 duration, float amplitude);
void StartIndefiniteCameraShake(s32 axisIndex, float amplitude);
float UpdateCameraShake(s32 axisIndex);
void SetDefaultCameraView(void);
void ReadCameraTestControls(void);
void SetCameraUpdateCallback(void (*callback)(void));
void SetCameraInputCallback(void (*callback)(void));
void UpdateSceneCamera(OMOBJ *object);
void InitializeSecondaryCamera(void);
void InitializeSceneShadows(void);
void StartCountdown(s32 durationSeconds);
s32 IsCountdownComplete(void);
void FinishCountdown(void);
void UpdateCountdownSequence(OMOBJ *object);
void CreateCountdownObject(void);
void AnimateWindowTransition(s32 windowId, float horizontalAnchor, float verticalAnchor, s32 opening);
void InitializeMessageWindows(void);
void OpenMessageWindow(s32 windowId, float horizontalAnchor, float verticalAnchor);
s32 CreateStyledMessageWindow(float left, float top, float width, float height, s32 style);
s32 CreateMessageWindowVariant(float left, float top, float width, float height, s32 style);
void CloseMessageWindowAnimation(s32 windowId, float horizontalAnchor, float verticalAnchor);
void CloseMessageWindow(s32 windowId);
void DismissMessageWindow(s32 windowId);
void WaitForMessageConfirmation(s32 windowId, s32 delayFrames);
void ShowMessageChoice(s32 windowId);
void SetMessageWindowText(s32 windowId, s32 messageId);
void ExpandMessageDimensions(float *dimensions, s32 messageId);
void RotateDecorationModels(OMOBJ *object);
void SetSceneDialogueState(s32 state);
void UpdateSceneCharacter(OMOBJ *object);
void UpdateSceneCharacterAndProps(OMOBJ *object);
void InitializeSceneModels(SceneDialogueState *sceneState);
void LoadSceneModels(OMOBJ *object);
float GetMotionProgress(void);
float GetSpecialCharacterMotionProgress(void);
void StartBalloonScene(s32 state);
void UpdateBalloonMotion(OMOBJ *object);
s32 HasBalloonMotionFinished(void);
void AdvanceBalloonMotionFrame(float frameDelta);
void UpdateBalloonMotionTiming(OMOBJ *object);
void StartBalloonRotation(void);
void UpdateBalloonRotation(OMOBJ *object);
void AdvanceBalloonRotationTimer(s32 frameDelta);
void ApplyBalloonRotation(OMOBJ *object);
void UpdateBalloonScene(OMOBJ *object);
void InitializeBalloonObject(CharacterSceneState *sceneState);
void LoadCharacterAnimationModels(OMOBJ *object);
void SetCharacterAnimationState(s32 state);
void UpdateCharacterAnimation(OMOBJ *object);
void SetSpecialCharacterAnimation(s32 animationIndex);
void SetCharacterMotionForState(s32 motionIndex, s32 blendFrames, s32 motionMode, s32 applyToCharacter);
void UpdateCharacterAnimationObject(OMOBJ *object);
void UpdateCharacterAnimationState(CharacterAnimationState *value);
void LoadSecondaryCharacterModel(OMOBJ *object);
void SetFirstCharacterBodyMotion(s32 motionIndex, s32 blendFrames, s32 motionMode, s32 enabled);
void SetSecondCharacterBodyMotion(s32 motionIndex, s32 blendFrames, s32 motionMode, s32 enabled);
void SetFirstCharacterAccessoryMotion(s32 motionIndex, s32 blendFrames, s32 motionMode, s32 enabled);
void SetSpecialCharacterMotion(s32 motionIndex);
void InitializePlayerCharacterObject(OMOBJ *object);
void UpdatePlayerCharacterObject(OMOBJ *object);
void InitializeCharacterMotionState(CharacterMotionState *motionState);
void UpdateCharacterMotionObject(OMOBJ *object);
void SortPlayersByTurnOrder(void);
void SetPlayerActionState(s32 playerIndex, s32 actionState);
void UpdatePlayerCharacterState(OMOBJ *object);
void SetPlayerActionResult(s32 playerIndex, s32 actionResult);
void UpdatePlayerMaterialOpacity(OMOBJ *object);
void SetPlayerActionParameters(s32 firstValue, s32 secondValue, s32 thirdValue);
void UpdatePlayerCharacterAction(OMOBJ *object);
void SetPlayerCharacterMotionState(s32 firstValue, s32 secondValue);
void InitializePlayerCharacterAction(OMOBJ *object);
void UpdatePlayerActionObject(OMOBJ *object);
void UpdatePlayerActionState(CharacterActionState *playerState);
void UpdatePlayerActionObjectState(OMOBJ *object);
void StartCharacterMotionTimeline(void);
void UpdateCharacterMotionTimeline(OMOBJ *object);
void UpdateCharacterTimelineObject(OMOBJ *object);
void InitializeCharacterMotionTimeline(CharacterMotionTimeline *value);
void UpdateCharacterMotionMarkers(OMOBJ *object);
void SetCameraForIntro(void);
void TransitionCameraToIntro(void);
void SetCameraForCharacterScene(void);
void SetCameraForBalloonScene(void);
void SetCameraForPlayerScene(void);
void TransitionCameraToPlayerScene(void);
void SetCameraForResultScene(void);
void SetCameraForCloseup(void);
void TransitionCameraToPreset(s32 presetIndex);
void ShowPlayerIntroMessage(void);
void ShowPlayerTurnMessage(void);
void ShowPlayerResultMessage(void);
void ShowCharacterResultMessages(s32 firstValue);
void ShowRoundResultMessages(void);
void ShowMinigameResultMessage(void);
void EmptySequenceCallback(void);
s32 IsIntroMusicReady(void);
void ShowPlayerIntroDialogue(void);
s32 AdvanceIntroMotionSequence(void);
void ShowPlayerTurnDialogue(void);
s32 AdvanceTurnMotionSequence(void);
void SetTurnCameraPreset(void);
s32 AdvanceCharacterMotionSequence(void);
void ShowResultDialogue(void);
s32 AdvanceResultDialogue(void);
void PrepareCharacterCelebration(void);
s32 AdvanceCelebrationSequence(void);
void SetResultCameraAndMotion(void);
s32 AdvanceResultCameraSequence(void);
void ShowMinigameEndDialogue(void);
s32 AdvanceMinigameEndSequence(void);
void InitializeBalloonResultScene(void);
s32 PositionPlayersForResult(void);
void InitializePrimaryMotionSequence(void);
s32 RunPrimaryMotionCallbacks(void);
float GetRemainingMotionFrames(void);
void UpdateMinigameRound(void);
s32 UpdateActivePlayerJump(s32 playerIndex);
float UpdatePlayerJumpState(s32 playerIndex, s32 jumpState);
void TransitionCameraToRoundStart(void);
void SetCameraForCurrentPlayer(void);
void SetCameraForActiveCharacter(void);
void TransitionCameraToActiveCharacter(void);
void SetCameraForRoundPhase(s32 phaseIndex);
void ShowRoundAnnouncement(void);
void ShowTurnAnnouncement(void);
void ShowMinigameRoundMessage(void);
void ShowPlayerScoreMessage(void);
void ShowRoundSummaryMessage(void);
void ShowRoundChoiceMessage(void);
void ShowGameplayStatusMessage(void);
void UpdatePlayerTurnOrder(void);
s32 InitializePlayerRoundPositions(void);
void StartMinigameRound(void);
s32 AdvanceMinigameRound(void);
void SetSecondarySequenceDuration(void);
s32 AdvanceSecondaryMotionSequence(void);
void ShowRoundPhaseDialogue(void);
s32 AdvanceRoundPhaseSequence(void);
void UpdatePlayerRoundState(void);
s32 AdvancePlayerRoundState(void);
void StartSpecialCharacterAnimation(void);
s32 AdvanceActivePlayerAction(void);
void UpdateRoundResultState(void);
s32 AdvanceRoundResultState(void);
void PositionPlayersForNextRound(void);
s32 IsSecondarySequenceComplete(void);
void StartSpecialCharacterCelebration(void);
s32 AdvanceSecondarySequence(void);
void UpdatePlayerScoreState(void);
s32 AdvancePlayerScoreState(void);
void PositionPlayersForFinalScene(void);
s32 IsFinalMotionComplete(void);
void InitializeSecondaryMotionSequence(void);
s32 RunSecondaryMotionCallbacks(void);
void UpdateFinalPlayerState(void);
s32 AdvanceFinalPlayerState(void);
void InitializeTertiaryMotionSequence(void);
s32 RunTertiaryMotionCallbacks(void);
void UpdatePlayerResultObject(OMOBJ *object);
s32 IsExitRequested(void);
void FinalizeMinigameResults(void);
void UpdateResultSceneDialogue(OMOBJ *object);
void InitializeMinigameScene(OMOBJ *object);
void StartMinigame(void);

AudioSequenceState sequenceHandles;
MinigameRoundState minigameRoundState;
SceneDialogueState sceneDialogueState;
CharacterSceneState characterSceneState;
CharacterAnimationState characterAnimationState;
CharacterMotionState characterMotionStates[2];
CharacterActionState characterActionStates[4];
CameraState globalCameraState;
float motionEndFrame;
float currentMotionFrame;
float motionDuration;
float motionTargetFrame;
float blendedMotionFrame;
CharacterMotionTimeline characterMotionTimeline;
MotionSequenceState primaryMotionSequence;
MotionSequenceState secondaryMotionSequence;
MotionSequenceState tertiaryMotionSequence;
s32 activeCharacterIndex;
s32 activePlayerIndex;
float sceneAnimationTimer;
s32 windowChoiceResult;
s32 minigameResult;
s32 countdownState;
s32 countdownFrameCount;
HUPROCESS *minigameProcess;

s32 characterEffectFrames[][5] = { { 91, 105, 126, 141, -1 }, { 91, 105, 126, 141, -1 }, { 124, 137, -1, -1, -1 }, { 85, 100, 127, -1, -1 },
    { 124, 139, -1, -1, -1 }, { 85, 97, 127, 139, -1 }, { 127, 137, -1, -1, -1 }, { 79, 95, 109, 127, -1 } };

s32 trackedCharacterModelId = -1;
s32 trackedCharacterMotionId = -1;
s32 trackedCharacterIndex = -1;

void MonitorCharacterMotionSoundCues(void)
{
    s32 i;

    while (TRUE) {
        HuPrcVSleep();
        if (trackedCharacterModelId == -1 || trackedCharacterMotionId == -1 || trackedCharacterIndex == -1 || trackedCharacterMotionId != Hu3DMotionIDGet(trackedCharacterModelId)) {
            continue;
        }
        for (i = 0; i < 5; i++) {
            if (characterEffectFrames[trackedCharacterIndex][i] != -1 && Hu3DMotionTimeGet(trackedCharacterModelId) >= characterEffectFrames[trackedCharacterIndex][i]) {
                OSReport("KE-System  CHR:%d TIME:%d\n", trackedCharacterIndex, characterEffectFrames[trackedCharacterIndex][i]);
                HuAudFXPlay(0x6D8);
                characterEffectFrames[trackedCharacterIndex][i] = -1;
            }
        }
    }
}

void CreateSceneLight(void)
{
    s32 lightId;

    lightId = Hu3DGLightCreate(0.0f, 1000.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0xFF, 0xFF, 0xFF);
    Hu3DGLightInfinitytSet(lightId);
}

void StartCameraShake(s32 axisIndex, s32 duration, float amplitude)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->shakeDuration[axisIndex] = duration;
    if (cameraState->cameraShake[axisIndex] < 0.0f) {
        cameraState->cameraShake[axisIndex] = -amplitude;
    }
    else {
        cameraState->cameraShake[axisIndex] = amplitude;
    }
}

void StartIndefiniteCameraShake(s32 axisIndex, float amplitude)
{
    StartCameraShake(axisIndex, -1, amplitude);
}

float UpdateCameraShake(s32 axisIndex)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    if (cameraState->cameraShake[axisIndex] != 0.0f) {
        cameraState->cameraShake[axisIndex] *= -0.9f;
        if (cameraState->shakeDuration[axisIndex] == -1) {
            if (cameraState->cameraShake[axisIndex] >= -0.5f && cameraState->cameraShake[axisIndex] <= 0.5f) {
                cameraState->cameraShake[axisIndex] = 0.0f;
            }
        }
        else {
            cameraState->shakeDuration[axisIndex]--;
            if (cameraState->shakeDuration[axisIndex] <= 0) {
                cameraState->shakeDuration[axisIndex] = 0;
                cameraState->cameraShake[axisIndex] = 0.0f;
            }
        }
    }
    return cameraState->cameraShake[axisIndex];
}

void SetDefaultCameraView(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->cameraCenterX = 0.0f;
    cameraState->cameraCenterY = 225.0f;
    cameraState->cameraCenterZ = 1740.0f;
    cameraState->cameraRotationX = -10.0f;
    cameraState->cameraRotationY = 0.0f;
    cameraState->cameraRotationZ = 0.0f;
    cameraState->cameraZoom = 300.0f;
}

void ReadCameraTestControls(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    if (HuPadBtn[0] & 0x200) {
        if (HuPadBtn[0] & 0x100) {
            cameraState->cameraCenterZ -= HuPadStkY[0] / 10.0f;
        }
        else {
            cameraState->cameraCenterX += HuPadStkX[0] / 10.0f;
            cameraState->cameraCenterY += HuPadStkY[0] / 10.0f;
        }
    }
    if (HuPadBtn[0] & 0x800) {
        cameraState->cameraRotationX -= HuPadStkY[0] / 100.0f;
        cameraState->cameraRotationY += HuPadStkX[0] / 100.0f;
        if (cameraState->cameraRotationX < 0.0f) {
            cameraState->cameraRotationX += 360.0f;
        }
        if (cameraState->cameraRotationX >= 360.0f) {
            cameraState->cameraRotationX -= 360.0f;
        }
        if (cameraState->cameraRotationY < 0.0f) {
            cameraState->cameraRotationY += 360.0f;
        }
        if (cameraState->cameraRotationY >= 360.0f) {
            cameraState->cameraRotationY -= 360.0f;
        }
    }
    if (HuPadBtn[0] & 0x400) {
        cameraState->cameraZoom -= HuPadStkY[0] / 10.0f;
    }
    print8(16, 100, 1.0f, ">>>>>>>>>> CAMERA TEST <<<<<<<<<<");
    print8(16, 110, 1.0f, "CENTER : %.2f, %.2f, %.2f", cameraState->cameraCenterX, cameraState->cameraCenterY, cameraState->cameraCenterZ);
    print8(16, 120, 1.0f, "ROT    : %.2f, %.2f, %.2f", cameraState->cameraRotationX, cameraState->cameraRotationY, cameraState->cameraRotationZ);
    print8(16, 130, 1.0f, "ZOOM   : %.2f", cameraState->cameraZoom);
}

void SetCameraUpdateCallback(void (*callback)(void))
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->updateCallback = callback;
}

void SetCameraInputCallback(void (*callback)(void))
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->inputCallback = callback;
}

void UpdateSceneCamera(OMOBJ *object)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    if (cameraState->updateCallback != NULL) {
        cameraState->updateCallback();
    }
    Center.x = cameraState->cameraCenterX + UpdateCameraShake(0);
    Center.y = cameraState->cameraCenterY + UpdateCameraShake(1);
    Center.z = cameraState->cameraCenterZ + UpdateCameraShake(2);
    CRot.x = cameraState->cameraRotationX + UpdateCameraShake(3);
    CRot.y = cameraState->cameraRotationY + UpdateCameraShake(4);
    CRot.z = cameraState->cameraRotationZ + UpdateCameraShake(5);
    CZoom = cameraState->cameraZoom + UpdateCameraShake(6);
}

void InitializeSecondaryCamera(void)
{
    Hu3DCameraCreate(1);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 45.0f, 0.1f, 10000.0f, 1.2f);
    SetCameraUpdateCallback(SetDefaultCameraView);
    SetCameraInputCallback(ReadCameraTestControls);
    omAddObjEx(minigameProcess, 0x7FDA, 0, 0, -1, omOutView);
    omAddObjEx(minigameProcess, 0x7FD9, 0, 0, -1, UpdateSceneCamera);
}

void InitializeSceneShadows(void)
{
    Vec lightPosition = { 0.0f, 3000.0f, 1.0f };
    Vec lightTarget = { 0.0f, 0.0f, 0.0f };
    Vec lightDirection = { 0.0f, 1.0f, 0.0f };

    Hu3DShadowCreate(45.0f, 2000.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.8f);
    Hu3DShadowPosSet(&lightPosition, &lightDirection, &lightTarget);
}

void StartCountdown(s32 durationSeconds)
{
    if (countdownState == 0) {
        countdownState = 1;
        countdownFrameCount = durationSeconds * REFRESH_RATE;
    }
}

s32 IsCountdownComplete(void)
{
    if (countdownState == 3) {
        return 1;
    }
    else {
        return 0;
    }
}

void FinishCountdown(void)
{
    if (countdownState == 2) {
        countdownState = 3;
        countdownFrameCount = 30;
    }
}

s32 activeSequenceId = -1;

void UpdateCountdownSequence(OMOBJ *object)
{
    switch (countdownState) {
        case 1:
            if (activeSequenceId == -1) {
                activeSequenceId = MGSeqCreate(1, countdownFrameCount / REFRESH_RATE, -1, -1);
            }
            countdownState = 2;
            break;
        case 2:
            countdownFrameCount--;
            if (activeSequenceId != -1) {
                MGSeqParamSet(activeSequenceId, 1, (countdownFrameCount + REFRESH_RATE - 1) / REFRESH_RATE);
            }
            if (countdownFrameCount <= 0) {
                countdownFrameCount = 30;
                countdownState = 3;
            }
            break;
        case 3:
            if (activeSequenceId != -1) {
                MGSeqParamSet(activeSequenceId, 2, -1);
                activeSequenceId = -1;
            }
            countdownFrameCount = 0;
            countdownState = 0;
            break;
    }
}

void CreateCountdownObject(void)
{
    countdownFrameCount = 0;
    countdownState = 0;
    omAddObjEx(minigameProcess, 0x2000, 0, 0, -1, UpdateCountdownSequence);
}

// Identical to mgmodedll/main.c::fn_1_245CC
void AnimateWindowTransition(s32 windowId, float horizontalAnchor, float verticalAnchor, s32 opening)
{
    WindowData *window;
    float windowLeft;
    float windowTop;
    float windowWidth;
    float windowHeight;
    float collapsedXScale;
    float collapsedYScale;
    float targetLeft;
    float targetTop;
    float currentLeft;
    float currentTop;
    float currentXScale;
    float currentYScale;
    s32 animationFrame;

    window = &winData[windowId];
    windowLeft = window->posX;
    windowTop = window->posY;
    windowWidth = window->w;
    windowHeight = window->h;
    targetLeft = windowLeft + windowWidth * horizontalAnchor;
    collapsedXScale = 4.0f / windowWidth;
    if (targetLeft + 4.0f - (windowLeft + windowWidth) >= 0.0f) {
        targetLeft = windowLeft + windowWidth - 4.0f;
    }
    else {
        if (windowLeft - (targetLeft - 2.0f) >= 0.0f) {
            targetLeft = windowLeft;
        }
        else {
            targetLeft -= 2.0f;
        }
    }
    targetTop = windowTop + windowHeight * verticalAnchor;
    collapsedYScale = 4.0f / windowHeight;
    if (targetTop + 4.0f - (windowTop + windowHeight) >= 0.0f) {
        targetTop = windowTop + windowHeight - 4.0f;
    }
    else {
        if (windowTop - (targetTop - 2.0f) >= 0.0f) {
            targetTop = windowTop;
        }
        else {
            targetTop -= 2.0f;
        }
    }
    if (opening != 0) {
        HuWinPosSet(windowId, targetLeft, targetTop);
        HuWinScaleSet(windowId, collapsedXScale, collapsedYScale);
        currentLeft = targetLeft;
        currentXScale = collapsedXScale;
        currentTop = targetTop;
        currentYScale = collapsedYScale;
    }
    else {
        HuWinPosSet(windowId, windowLeft, windowTop);
        HuWinScaleSet(windowId, 1.0f, 1.0f);
        currentLeft = windowLeft;
        currentXScale = 1.0f;
        currentTop = windowTop;
        currentYScale = 1.0f;
    }
    HuWinDispOn(windowId);
    for (animationFrame = 0; animationFrame <= 15; animationFrame++) {
        HuPrcVSleep();
        if (opening != 0) {
            if (animationFrame <= 10) {
                currentLeft = easeInSine(targetLeft, windowLeft, animationFrame, 10.0f);
                currentXScale = easeInSine(collapsedXScale, 1.0f, animationFrame, 10.0f);
            }
            else {
                currentTop = easeInSine(targetTop, windowTop, animationFrame - 10, 5.0f);
                currentYScale = easeInSine(collapsedYScale, 1.0f, animationFrame - 10, 5.0f);
            }
        }
        else {
            if (animationFrame <= 10) {
                currentTop = easeInSine(windowTop, targetTop, animationFrame, 10.0f);
                currentYScale = easeInSine(1.0f, collapsedYScale, animationFrame, 10.0f);
            }
            else {
                currentLeft = easeInSine(windowLeft, targetLeft, animationFrame - 10, 5.0f);
                currentXScale = easeInSine(1.0f, collapsedXScale, animationFrame - 10, 5.0f);
            }
        }
        HuWinPosSet(windowId, currentLeft, currentTop);
        HuWinScaleSet(windowId, currentXScale, currentYScale);
    }
    if (opening != 0) {
        HuWinPosSet(windowId, windowLeft, windowTop);
        HuWinScaleSet(windowId, 1.0f, 1.0f);
    }
    else {
        HuWinPosSet(windowId, targetLeft, targetTop);
        HuWinScaleSet(windowId, collapsedXScale, collapsedYScale);
    }
    HuPrcVSleep();
}

void InitializeMessageWindows(void)
{
    HuWinInit(1);
}

void OpenMessageWindow(s32 windowId, float horizontalAnchor, float verticalAnchor)
{
    WindowData *window;
    float windowLeft;
    float windowTop;
    float collapsedXScale;
    float collapsedYScale;
    float windowWidth;
    float windowHeight;
    float targetLeft;
    float targetTop;
    float currentLeft;
    float currentTop;
    float currentXScale;
    float currentYScale;
    s32 animationFrame;

    window = &winData[windowId];
    windowLeft = window->posX;
    windowTop = window->posY;
    collapsedXScale = window->w;
    collapsedYScale = window->h;
    targetLeft = windowLeft + collapsedXScale * horizontalAnchor;
    windowWidth = 4.0f / collapsedXScale;
    if (targetLeft + 4.0f - (windowLeft + collapsedXScale) >= 0.0f) {
        targetLeft = windowLeft + collapsedXScale - 4.0f;
    }
    else {
        if (windowLeft - (targetLeft - 2.0f) >= 0.0f) {
            targetLeft = windowLeft;
        }
        else {
            targetLeft = targetLeft - 2.0f;
        }
    }
    targetTop = windowTop + collapsedYScale * verticalAnchor;
    windowHeight = 4.0f / collapsedYScale;
    if (targetTop + 4.0f - (windowTop + collapsedYScale) >= 0.0f) {
        targetTop = windowTop + collapsedYScale - 4.0f;
    }
    else {
        if (windowTop - (targetTop - 2.0f) >= 0.0f) {
            targetTop = windowTop;
        }
        else {
            targetTop = targetTop - 2.0f;
        }
    }
    HuWinPosSet(windowId, targetLeft, targetTop);
    HuWinScaleSet(windowId, windowWidth, windowHeight);
    HuPrcVSleep();
    for (animationFrame = 0; animationFrame < 10; animationFrame++) {
        currentLeft = easeInSine(targetLeft, windowLeft, animationFrame, 10.0f);
        currentXScale = easeInSine(windowWidth, 1.0f, animationFrame, 10.0f);
        currentTop = easeInSine(targetTop, windowTop, animationFrame, 10.0f);
        currentYScale = easeInSine(windowHeight, 1.0f, animationFrame, 10.0f);
        HuWinPosSet(windowId, currentLeft, currentTop);
        HuWinScaleSet(windowId, currentXScale, currentYScale);
        HuPrcVSleep();
    }
    HuWinPosSet(windowId, windowLeft, windowTop);
    HuWinScaleSet(windowId, 1.0f, 1.0f);
    HuPrcVSleep();
}

s32 CreateStyledMessageWindow(float left, float top, float width, float height, s32 style)
{
    s32 windowId;
    s32 windowStyleOptions[][6] = { { 0xFF, 0xFF, 0xCC, 0x00, 0x09, 0x09 }, { 0xCC, 0xFF, 0xFF, 0x00, 0x09, 0x09 }, { 0xFF, 0xCC, 0xFF, 0x00, 0x09, 0x09 } };

    windowId = HuWinCreate(left, top, width, height, 1);
    HuSprColorSet(winData[windowId].group, 0, windowStyleOptions[style][0], windowStyleOptions[style][1], windowStyleOptions[style][2]);
    winData[windowId].messColor = windowStyleOptions[style][3];
    winData[windowId].messShadowColor = windowStyleOptions[style][4];
    winData[windowId].attr |= 0x80;
    HuWinBGTPLvlSet(windowId, windowStyleOptions[style][5] / 10.0f);
    winData[windowId].activePad = 0;
    AnimateWindowTransition(windowId, 0.5f, 0.5f, 1);
    return windowId;
}

s32 CreateMessageWindowVariant(float left, float top, float width, float height, s32 style)
{
    return CreateStyledMessageWindow(left, top, width, height, style);
}

void CloseMessageWindowAnimation(s32 windowId, float horizontalAnchor, float verticalAnchor)
{
    WindowData *window;
    float windowLeft;
    float windowTop;
    float windowWidth;
    float windowHeight;
    float collapsedXScale;
    float collapsedYScale;
    float targetLeft;
    float targetTop;
    float currentLeft;
    float currentTop;
    float currentXScale;
    float currentYScale;
    s32 animationFrame;

    window = &winData[windowId];
    windowLeft = window->posX;
    windowTop = window->posY;
    windowWidth = window->w;
    windowHeight = window->h;
    targetLeft = windowLeft + windowWidth * horizontalAnchor;
    collapsedXScale = 4.0f / windowWidth;
    if (targetLeft + 4.0f - (windowLeft + windowWidth) >= 0.0f) {
        targetLeft = windowLeft + windowWidth - 4.0f;
    }
    else {
        if (windowLeft - (targetLeft - 2.0f) >= 0.0f) {
            targetLeft = windowLeft;
        }
        else {
            targetLeft = targetLeft - 2.0f;
        }
    }
    targetTop = windowTop + windowHeight * verticalAnchor;
    collapsedYScale = 4.0f / windowHeight;
    if (targetTop + 4.0f - (windowTop + windowHeight) >= 0.0f) {
        targetTop = windowTop + windowHeight - 4.0f;
    }
    else {
        if (windowTop - (targetTop - 2.0f) >= 0.0f) {
            targetTop = windowTop;
        }
        else {
            targetTop = targetTop - 2.0f;
        }
    }
    HuWinPosSet(windowId, windowLeft, windowTop);
    HuWinScaleSet(windowId, 1.0f, 1.0f);
    HuPrcVSleep();
    for (animationFrame = 0; animationFrame < 10; animationFrame++) {
        currentLeft = easeOutSine(windowLeft, targetLeft, animationFrame, 10.0f);
        currentXScale = easeOutSine(1.0f, collapsedXScale, animationFrame, 10.0f);
        currentTop = easeOutSine(windowTop, targetTop, animationFrame, 10.0f);
        currentYScale = easeOutSine(1.0f, collapsedYScale, animationFrame, 10.0f);
        HuWinPosSet(windowId, currentLeft, currentTop);
        HuWinScaleSet(windowId, currentXScale, currentYScale);
        HuPrcVSleep();
    }
    HuWinPosSet(windowId, targetLeft, targetTop);
    HuWinScaleSet(windowId, collapsedXScale, collapsedYScale);
    HuPrcVSleep();
}

void CloseMessageWindow(s32 windowId)
{
    HuWinMesSet(windowId, MAKE_MESSID(29, 22));
    HuPrcVSleep();
    AnimateWindowTransition(windowId, 0.5f, 0.5f, 0);
    HuWinKill(windowId);
}

void DismissMessageWindow(s32 windowId)
{
    CloseMessageWindow(windowId);
}

void WaitForMessageConfirmation(s32 windowId, s32 delayFrames)
{
    s32 hasHumanPlayer;
    s32 messageDelay;
    s32 playerIndex;

    hasHumanPlayer = 0;
    messageDelay = GWMessDelayGet();
    if (delayFrames > 0) {
        HuPrcSleep(delayFrames);
        return;
    }
    HuWinComKeyReset();
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (characterActionStates[playerIndex].isCpu == 0) {
            hasHumanPlayer = 1;
            winData[windowId].activePad = 0xF;
        }
    }
    if (hasHumanPlayer == 0) {
        winData[windowId].activePad = 0xF;
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, messageDelay);
    }
    HuWinMesWait(windowId);
    HuWinComKeyReset();
}

void ShowMessageChoice(s32 windowId)
{
    s32 choice;
    s32 messageDelay;
    s32 playerIndex;

    choice = 0;
    messageDelay = GWMessDelayGet();
    HuWinComKeyReset();
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (characterActionStates[playerIndex].isCpu == 0) {
            choice = 1;
            winData[windowId].activePad = 0xF;
        }
    }
    if (choice == 0) {
        winData[windowId].activePad = 0xF;
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, messageDelay);
    }
    choice = HuWinChoiceGet(windowId, 1);
    if (choice == -1 || choice == 1) {
        windowChoiceResult = 0;
    }
    else {
        windowChoiceResult = 1;
    }
    HuWinComKeyReset();
}

void SetMessageWindowText(s32 windowId, s32 messageId)
{
    winData[windowId].messColor = 0;
    winData[windowId].messShadowColor = 9;
    winData[windowId].attr |= 0x80;
    HuWinMesSet(windowId, messageId);
}

void ExpandMessageDimensions(float *dimensions, s32 messageId)
{
    float messageDimensions[2];

    HuWinMesMaxSizeGet(1, messageDimensions, messageId);
    if (messageDimensions[0] > dimensions[0]) {
        dimensions[0] = messageDimensions[0];
    }
    if (messageDimensions[1] > dimensions[1]) {
        dimensions[1] = messageDimensions[1];
    }
}

void RotateDecorationModels(OMOBJ *object)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        Hu3DData[object->mdlId[i + 5]].rot.y += 360.0f / (2500.0f + 2500.0f * i);
    }
}

void SetSceneDialogueState(s32 state)
{
    sceneDialogueState.sceneState = state;
}

void UpdateSceneCharacter(OMOBJ *object)
{
    SceneDialogueState *sceneState;

    sceneState = object->data;
    switch (sceneState->sceneState) {
        case 2:
            SetModelMotion(object, 9, 9, 1, 0);
            Hu3DModelAttrReset(object->mdlId[9], HU3D_ATTR_DISPOFF);
            sceneState->dialogueState = 0;
            StartCameraShake(1, 10, 4.0f);
            HuAudFXPlay(0x6CF);
            /* fallthrough */
        case 1:
            SetModelMotion(object, 0, 0, 1, 0);
            SetModelMotion(object, 3, 3, 1, 0);
            SetModelMotion(object, 4, 4, 1, 0);
            SetModelMotion(object, 5, 5, 1, 0);
            SetModelMotion(object, 6, 6, 1, 0);
            SetModelMotion(object, 7, 7, 1, 0);
            if (sceneState->sceneState == 1) {
                HuAudFXPlay(0x6CE);
            }
            sceneState->sceneState = 10;
            break;
        case 3:
            sceneState->sceneState = 30;
            SetModelMotion(object, 0, 0, 1, 0);
            SetModelMotion(object, 1, 1, 1, 0);
            SetModelMotion(object, 2, 2, 1, 0);
            SetModelMotion(object, 3, 3, 1, 0);
            SetModelMotion(object, 4, 4, 1, 0);
            SetModelMotion(object, 5, 5, 1, 0);
            SetModelMotion(object, 6, 6, 1, 0);
            SetModelMotion(object, 7, 7, 1, 0);
            SetModelMotion(object, 8, 8, 0, 0);
            Hu3DModelAttrReset(object->mdlId[8], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(object->mdlId[9], HU3D_ATTR_DISPOFF);
            HuAudFXPlay(0x6D0);
            break;
        case 10:
            if (Hu3DMotionEndCheck(object->mdlId[3])) {
                sceneState->sceneState = 0;
                SetModelMotion(object, 3, 13, 1, 1);
                SetModelMotion(object, 4, 14, 1, 1);
                Hu3DModelAttrSet(object->mdlId[8], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrSet(object->mdlId[9], HU3D_ATTR_DISPOFF);
            }
            break;
        case 30:
            StartCameraShake(6, -1, 8.0f);
            if (Hu3DMotionTimeGet(object->mdlId[3]) >= 30.0f) {
                sceneState->sceneState = 10;
                StartCameraShake(6, -1, 128.0f);
                HuAudFXPlay(0x6DA);
                omVibrate(characterActionStates[0].playerIndex, 12, 12, 0);
                omVibrate(characterActionStates[1].playerIndex, 12, 12, 0);
                omVibrate(characterActionStates[2].playerIndex, 12, 12, 0);
                omVibrate(characterActionStates[3].playerIndex, 12, 12, 0);
                StartBalloonScene(1);
            }
            break;
    }
    if (Hu3DMotionTimeGet(object->mdlId[9]) >= 25.0f && sceneState->dialogueState == 0) {
        sceneState->dialogueState = 1;
        StartCameraShake(1, 10, 4.0f);
    }
    else if (Hu3DMotionTimeGet(object->mdlId[9]) >= 40.0f && sceneState->dialogueState == 1) {
        sceneState->dialogueState = 2;
        StartCameraShake(1, 10, 4.0f);
    }
    if (sceneState->sceneState == 0) {
        if (sceneState->sceneEventTimer++ >= 5 * REFRESH_RATE) {
            sceneState->sceneEventTimer = 0;
            if (rand8() % 2 != 0) {
                SetSceneDialogueState(1);
            }
        }
    }
}

void UpdateSceneCharacterAndProps(OMOBJ *object)
{
    RotateDecorationModels(object);
    UpdateSceneCharacter(object);
}

void InitializeSceneModels(SceneDialogueState *sceneState)
{
    OMOBJ *mainObject = sceneState->mainObject;
    s32 modelOptions[][3] = { { 2, -1, 0 }, { 2, -1, 0 }, { 2, -1, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, { 2, -1, 0 }, { 2, -1, 0 }, { 2, -1, 0 }, { 2, -1, 1 },
        { 2, -1, 1 } };
    Vec modelPositions[] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -680.0f }, { 0.0f, 0.0f, -680.0f }, { 525.0f, 0.0f, -1150.0f }, { -525.0f, 0.0f, -1150.0f },
        { 0.0f, 150.0f, 50.0f }, { 0.0f, 150.0f, 0.0f }, { 0.0f, 150.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    s32 modelIndex;

    for (modelIndex = 0; modelIndex < 10; modelIndex++) {
        Hu3DModelPosSet(mainObject->mdlId[modelIndex], modelPositions[modelIndex].x, modelPositions[modelIndex].y, modelPositions[modelIndex].z);
        if (modelOptions[modelIndex][2] == 1) {
            Hu3DModelAttrSet(mainObject->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
        }
        if (modelOptions[modelIndex][1] != -1) {
            SetModelMotion(mainObject, modelIndex, modelIndex + 10, 1, modelOptions[modelIndex][1]);
        }
        else {
            SetModelMotion(mainObject, modelIndex, modelIndex, 1, modelOptions[modelIndex][1]);
        }
    }
    mainObject->work[0] = mainObject->work[1] = mainObject->work[2] = mainObject->work[3] = 0;
    sceneState->sceneEventTimer = sceneState->sceneActionState = sceneState->dialogueState = sceneState->dialogueTimer = 0;
    sceneState->sceneState = sceneState->sceneSubstate = sceneState->sceneTimer = sceneState->sceneEventState = 0;
}

void LoadSceneModels(OMOBJ *object)
{
    CharacterModelSetup modelOptions[] = { { DATA_MAKE_NUM(DATADIR_M437, 36), 2, -1, 1, 0 }, { DATA_MAKE_NUM(DATADIR_M437, 37), 2, -1, 1, 0 },
        { DATA_MAKE_NUM(DATADIR_M437, 38), 2, -1, 2, 0 }, { DATA_MAKE_NUM(DATADIR_M437, 39), 1, DATA_MAKE_NUM(DATADIR_M437, 41), 2, 0 },
        { DATA_MAKE_NUM(DATADIR_M437, 40), 1, DATA_MAKE_NUM(DATADIR_M437, 42), 2, 0 }, { DATA_MAKE_NUM(DATADIR_M437, 33), 2, -1, 0, 0 },
        { DATA_MAKE_NUM(DATADIR_M437, 34), 2, -1, 0, 0 }, { DATA_MAKE_NUM(DATADIR_M437, 35), 2, -1, 0, 0 },
        { DATA_MAKE_NUM(DATADIR_M437, 43), 2, -1, 0, 1 }, { DATA_MAKE_NUM(DATADIR_M437, 44), 2, -1, 0, 1 } };
    Vec modelPositions[] = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -680.0f }, { 0.0f, 0.0f, -680.0f }, { 525.0f, 0.0f, -1150.0f }, { -525.0f, 0.0f, -1150.0f },
        { 0.0f, 150.0f, 50.0f }, { 0.0f, 150.0f, 0.0f }, { 0.0f, 150.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    s32 modelIndex;

    for (modelIndex = 0; modelIndex < 10; modelIndex++) {
        object->mdlId[modelIndex] = Hu3DModelCreateFile(modelOptions[modelIndex].modelDataId);
        object->mtnId[modelIndex] = Hu3DMotionIDGet(object->mdlId[modelIndex]);
        if (modelOptions[modelIndex].jointMotionDataId != -1) {
            object->mtnId[modelIndex + 10] = Hu3DJointMotionFile(object->mdlId[modelIndex], modelOptions[modelIndex].jointMotionDataId);
        }
        Hu3DModelPosSet(object->mdlId[modelIndex], modelPositions[modelIndex].x, modelPositions[modelIndex].y, modelPositions[modelIndex].z);
        if (modelOptions[modelIndex].shadowMode == 1) {
            Hu3DModelShadowMapSet(object->mdlId[modelIndex]);
        }
        else if (modelOptions[modelIndex].shadowMode == 2) {
            Hu3DModelShadowSet(object->mdlId[modelIndex]);
        }
        if (modelOptions[modelIndex].hiddenOnStart == 1) {
            Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
            Hu3DModelLayerSet(object->mdlId[modelIndex], 1);
        }
        if (modelOptions[modelIndex].jointMotionDataId != -1) {
            SetModelMotion(object, modelIndex, modelIndex + 10, 1, modelOptions[modelIndex].motionMode);
        }
        else {
            SetModelMotion(object, modelIndex, modelIndex, 1, modelOptions[modelIndex].motionMode);
        }
    }
    object->data = &sceneDialogueState;
    object->objFunc = UpdateSceneCharacterAndProps;
}

float GetMotionProgress(void)
{
    return motionTargetFrame / motionDuration;
}

float GetSpecialCharacterMotionProgress(void)
{
    float animationProgress;
    CharacterMotionState *specialCharacter;

    animationProgress = motionTargetFrame / motionDuration;
    specialCharacter = &characterMotionStates[1];
    if (characterAnimationState.characterModeFlag == 0) {
        if (animationProgress > 0.875f) {
            SetSpecialCharacterAnimation(3);
            SetSpecialCharacterMotion(3);
            SetFirstCharacterBodyMotion(3, 5, 0, 0);
            SetFirstCharacterAccessoryMotion(5, 10, 2, 1);
        }
        else if (animationProgress > 0.75f) {
            SetSpecialCharacterAnimation(2);
            SetSpecialCharacterMotion(2);
        }
        else if (animationProgress > 0.5f) {
            SetSpecialCharacterAnimation(1);
            SetSpecialCharacterMotion(1);
        }
        else {
            SetSpecialCharacterAnimation(0);
            SetSpecialCharacterMotion(0);
        }
    }
    return animationProgress;
}

void StartBalloonScene(s32 state)
{
    characterSceneState.sceneState = 1;
}

void UpdateBalloonMotion(OMOBJ *object)
{
    Mtx balloonHookMatrix;
    CharacterSceneState *balloonState;

    balloonState = &characterSceneState;
    switch (balloonState->sceneState) {
        case 1:
            balloonState->sceneState = 2;
            Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
            SetModelMotion(object, 0, 3, 1, 0);
            SetModelMotion(object, 2, 10, 1, 0);
            break;
        case 2:
            if (Hu3DMotionTimeGet(object->mdlId[0]) >= 20.0f) {
                object->trans.y -= 20.0f;
                if (object->trans.y <= 0.0f) {
                    object->trans.y = 0.0f;
                    balloonState->sceneState = 3;
                }
            }
            break;
        case 3:
            if (Hu3DMotionEndCheck(object->mdlId[0])) {
                balloonState->sceneState = 4;
                Hu3DModelHookReset(object->mdlId[2]);
                Hu3DModelObjMtxGet(object->mdlId[0], "m437a01-balloon-point", balloonHookMatrix);
                Hu3DModelPosSet(object->mdlId[1], balloonHookMatrix[0][3], 0.0f, -250.0f);
            }
            break;
    }
}

s32 HasBalloonMotionFinished(void)
{
    if (motionEndFrame <= currentMotionFrame) {
        return 1;
    }
    else {
        return 0;
    }
}

void AdvanceBalloonMotionFrame(float frameDelta)
{
    currentMotionFrame += frameDelta;
}

void UpdateBalloonMotionTiming(OMOBJ *object)
{
    if (motionEndFrame > currentMotionFrame) {
        motionTargetFrame = currentMotionFrame * (motionDuration / motionEndFrame);
        blendedMotionFrame = weightedAverage(blendedMotionFrame, motionTargetFrame, 5.0f);
        Hu3DMotionTimeSet(object->mdlId[1], blendedMotionFrame);
    }
    else {
        currentMotionFrame = motionEndFrame;
        motionTargetFrame = currentMotionFrame * (motionDuration / motionEndFrame);
        blendedMotionFrame = weightedAverage(blendedMotionFrame, motionTargetFrame, 5.0f);
        Hu3DMotionTimeSet(object->mdlId[1], blendedMotionFrame);
    }
}

void StartBalloonRotation(void)
{
    characterSceneState.sceneEventState = 1;
}

void UpdateBalloonRotation(OMOBJ *object)
{
    Mtx balloonHookMatrix;
    CharacterSceneState *balloonState;

    balloonState = &characterSceneState;
    if (balloonState->sceneEventState == 1) {
        balloonState->sceneEventState = 0;
        Hu3DModelAttrReset(object->mdlId[3], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
        Hu3DModelObjMtxGet(object->mdlId[0], "m437a01-balloon-point", balloonHookMatrix);
        Hu3DModelPosSet(object->mdlId[3], balloonHookMatrix[0][3], 0.0f, -250.0f);
        SetModelMotion(object, 3, 6, 1, 0);
        omVibrate(characterActionStates[0].playerIndex, 12, 12, 0);
        omVibrate(characterActionStates[1].playerIndex, 12, 12, 0);
        omVibrate(characterActionStates[2].playerIndex, 12, 12, 0);
        omVibrate(characterActionStates[3].playerIndex, 12, 12, 0);
        HuAudFXPlay(0x6F4);
        HuAudSeqAllFadeOut(100);
    }
}

void AdvanceBalloonRotationTimer(s32 frameDelta)
{
    sceneAnimationTimer += frameDelta;
    if (sceneAnimationTimer >= 3.0f) {
        sceneAnimationTimer = 3.0f;
    }
}

void ApplyBalloonRotation(OMOBJ *object)
{
    float balloonRotation;
    CharacterSceneState *balloonState;
    s32 rotationDuration;

    balloonState = &characterSceneState;
    rotationDuration = 100.0f * GetMotionProgress();
    balloonRotation = easeInOutSine(0.0f, sceneAnimationTimer, characterSceneState.cameraAnimationFrame++, rotationDuration);
    if (characterSceneState.cameraAnimationFrame >= rotationDuration) {
        characterSceneState.cameraAnimationFrame = 0;
        sceneAnimationTimer -= 0.5f;
        if (sceneAnimationTimer <= 0.0f) {
            sceneAnimationTimer = 0.0f;
        }
    }
    if (GetMotionProgress() >= 0.2f) {
        Hu3DModelRotSet(object->mdlId[1], 0.0f, 0.0f, balloonRotation);
    }
}

void UpdateBalloonScene(OMOBJ *object)
{
    UpdateBalloonMotion(object);
    UpdateBalloonMotionTiming(object);
    ApplyBalloonRotation(object);
    UpdateBalloonRotation(object);
}

void InitializeBalloonObject(CharacterSceneState *sceneState)
{
    OMOBJ *balloonObject;

    balloonObject = sceneState->characterObject;
    omSetTra(balloonObject, 0.0f, 210.0f, -250.0f);
    Hu3DModelAttrSet(balloonObject->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(balloonObject->mdlId[1], HU3D_ATTR_DISPOFF);
    motionTargetFrame = 0.0f;
    motionDuration = Hu3DMotionMaxTimeGet(balloonObject->mdlId[1]);
    blendedMotionFrame = 0.0f;
    currentMotionFrame = 0.0f;
    motionEndFrame = rand8() % 7 + 30;
    Hu3DModelAttrSet(balloonObject->mdlId[2], HU3D_ATTR_DISPOFF);
    balloonObject->work[0] = balloonObject->work[1] = balloonObject->work[2] = balloonObject->work[3] = 0;
    sceneState->sceneEventTimer = sceneState->sceneActionState = sceneState->cameraAnimationFrame = sceneState->sceneFlags = 0;
    sceneState->sceneState = sceneState->sceneEventState = sceneState->sceneSubstate = sceneState->sceneFrameCount = 0;
}

void LoadCharacterAnimationModels(OMOBJ *object)
{
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M437, 24));
    object->mtnId[0] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 28));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 29));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 30));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 31));
    Hu3DModelShadowMapSet(object->mdlId[0]);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M437, 26));
    object->mtnId[5] = Hu3DMotionIDGet(object->mdlId[1]);
    Hu3DModelLayerSet(object->mdlId[1], 2);
    SetModelMotion(object, 1, 5, 1, 2);
    motionEndFrame = Hu3DMotionMaxTimeGet(object->mdlId[1]);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M437, 25));
    object->mtnId[10] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 32));
    Hu3DModelShadowMapSet(object->mdlId[0]);
    Hu3DModelHookSet(object->mdlId[0], "m437a01-balloon-point", object->mdlId[2]);
    Hu3DModelHookSet(object->mdlId[2], "m437test04-b_point", object->mdlId[1]);
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M437, 27));
    object->mtnId[6] = Hu3DMotionIDGet(object->mdlId[3]);
    Hu3DModelLayerSet(object->mdlId[3], 2);
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    object->objFunc = UpdateBalloonScene;
}

void SetCharacterAnimationState(s32 state)
{
    characterAnimationState.animationState = state;
}

void UpdateCharacterAnimation(OMOBJ *object)
{
    float animationBlend;
    CharacterAnimationState *characterState;

    animationBlend = 0.0f;
    characterState = object->data;
    switch (characterState->animationState) {
        case 1:
            characterState->animationState = 2;
            characterState->animationTimer = characterState->animationRepeatCount = 0;
            characterState->animationBlend = 0.0f;
            /* fallthrough */
        case 2:
            characterState->animationBlend = easeInOutSine(0.0f, 10.0f, characterState->animationTimer, 30.0f);
            if (characterState->animationTimer++ >= 30) {
                characterState->animationTimer = 0;
                characterState->animationRepeatCount++;
                if (characterState->animationRepeatCount >= 2) {
                    characterState->animationState = 3;
                }
            }
            break;
        case 3:
            characterState->animationBlend = weightedAverage(characterState->animationBlend, 0.0f, 5.0f);
            if (characterState->animationTimer++ >= 15) {
                characterState->animationState = 0;
                characterState->animationTimer = characterState->animationRepeatCount = 0;
                characterState->animationBlend = 0.0f;
                Hu3DMotionNoMotReset(object->mdlId[0], "g000m0-ns_ago", 0x200);
            }
            break;
    }
    if (characterState->animationState != 0) {
        Hu3DMotionForceSet(object->mdlId[0], "g000m0-ns_ago", 0x200, characterState->animationBlend);
    }
}

void SetSpecialCharacterAnimation(s32 animationIndex)
{
    s32 motionIds[] = { 4, 5, 6, 7, 8 };

    if (animationIndex <= 3) {
        EnsureModelMotion(characterAnimationState.characterObject, 0, motionIds[animationIndex], 120, 1);
    }
    else {
        EnsureModelMotion(characterAnimationState.characterObject, 0, motionIds[animationIndex], 5, 0);
    }
}

void SetCharacterMotionForState(s32 motionIndex, s32 blendFrames, s32 motionMode, s32 applyToCharacter)
{
    if (applyToCharacter != 0) {
        SetModelMotion(characterAnimationState.characterObject, 0, motionIndex, blendFrames, motionMode);
    }
    else {
        EnsureModelMotion(characterAnimationState.characterObject, 0, motionIndex, blendFrames, motionMode);
    }
}

void UpdateCharacterAnimationObject(OMOBJ *object)
{
    UpdateCharacterAnimation(object);
}

void UpdateCharacterAnimationState(CharacterAnimationState *value)
{
    OMOBJ *characterObject;

    characterObject = value->characterObject;
    omSetTra(characterObject, 0.0f, 37.0f, -565.0f);
    omSetSca(characterObject, 1.5f, 1.5f, 1.5f);
    SetModelMotion(characterObject, 0, 0, 1, 1);
    characterObject->work[0] = characterObject->work[1] = characterObject->work[2] = characterObject->work[3] = 0;
    value->animationTimer = value->animationRepeatCount = value->characterAction = value->characterEvent = 0;
    value->animationState = value->characterModeFlag = value->characterSubstate = value->characterMode = 0;
}

void LoadSecondaryCharacterModel(OMOBJ *object)
{
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M437, 45));
    object->mtnId[0] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 46));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 47));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 48));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 49));
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 50));
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 51));
    object->mtnId[6] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 52));
    object->mtnId[7] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 53));
    object->mtnId[8] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 54));
    omSetTra(object, 0.0f, 37.0f, -565.0f);
    omSetSca(object, 1.5f, 1.5f, 1.5f);
    Hu3DModelShadowSet(object->mdlId[0]);
    SetModelMotion(object, 0, 0, 1, 1);
    object->data = &characterAnimationState;
    object->objFunc = UpdateCharacterAnimationObject;
}

void SetFirstCharacterBodyMotion(s32 motionIndex, s32 blendFrames, s32 motionMode, s32 enabled)
{
    if (enabled != 0) {
        SetModelMotion(characterMotionStates[0].characterObject, 0, motionIndex, blendFrames, motionMode);
    }
    else {
        EnsureModelMotion(characterMotionStates[0].characterObject, 0, motionIndex, blendFrames, motionMode);
    }
}

void SetSecondCharacterBodyMotion(s32 motionIndex, s32 blendFrames, s32 motionMode, s32 enabled)
{
    if (enabled != 0) {
        SetModelMotion(characterMotionStates[1].characterObject, 0, motionIndex, blendFrames, motionMode);
    }
    else {
        EnsureModelMotion(characterMotionStates[1].characterObject, 0, motionIndex, blendFrames, motionMode);
    }
}

void SetFirstCharacterAccessoryMotion(s32 motionIndex, s32 blendFrames, s32 motionMode, s32 enabled)
{
    if (enabled != 0) {
        SetModelMotion(characterMotionStates[0].characterObject, 1, motionIndex, blendFrames, motionMode);
    }
    else {
        EnsureModelMotion(characterMotionStates[0].characterObject, 1, motionIndex, blendFrames, motionMode);
    }
}

void SetSpecialCharacterMotion(s32 motionIndex)
{
    CharacterMotionState *characterState;

    characterState = &characterMotionStates[1];
    characterState->animationIndex = motionIndex + 1;
}

void InitializePlayerCharacterObject(OMOBJ *object)
{
    CharacterMotionState *characterState = object->data;
    Vec characterPositions[] = { { 480.0f, 0.0f, -1250.0f }, { 400.0f, 37.0f, -850.0f }, { 400.0f, 37.0f, -600.0f }, { 400.0f, 37.0f, -425.0f } };

    if (object->work[0] == 0) {
        return;
    }
    if (characterState->animationIndex == 7) {
        characterState->animationState = characterState->animationIndex;
    }
    if (characterState->animationIndex != 4 || characterState->animationState <= 4) {
        characterState->animationState = characterState->animationIndex;
    }
    switch (characterState->animationState) {
        case 1:
        case 2:
        case 3:
            MoveCharacterTowardPosition(object, characterPositions[4 - characterState->animationState], 0.0f, 6.0f, 10.0f, 10.0f, 1);
            characterState->characterObject->trans.x = 400.0f;
            break;
        case 4:
            MoveCharacterTowardPosition(object, characterPositions[0], 0.0f, 6.0f, 10.0f, 10.0f, 1);
            characterState->characterObject->trans.x = 400.0f;
            if (characterState->animationRepeatCount++ >= 60) {
                characterState->animationState = 5;
                characterState->animationRepeatCount = 0;
            }
            break;
        case 5:
            MoveCharacterTowardPosition(object, characterPositions[0], 0.0f, 6.0f, 10.0f, 10.0f, 1);
            if (characterState->animationRepeatCount++ >= 60) {
                characterState->animationState = 6;
                characterState->animationRepeatCount = 0;
            }
            break;
        case 6:
            EnsureModelMotion(object, 0, 6, 10, 0);
            break;
        case 7:
            MoveCharacterTowardPosition(object, characterPositions[3], -15.0f, 6.0f, 10.0f, 10.0f, 1);
            break;
    }
}

void UpdatePlayerCharacterObject(OMOBJ *object)
{
    InitializePlayerCharacterObject(object);
}

void InitializeCharacterMotionState(CharacterMotionState *motionState)
{
    s32 characterIndex;
    OMOBJ *characterObject;

    characterIndex = motionState->characterObject->work[0];
    characterObject = motionState->characterObject;
    if (characterIndex != 0) {
        omSetTra(characterObject, 400.0f, 37.0f, -425.0f);
        omSetRot(characterObject, 0.0f, -15.0f, 0.0f);
        SetModelMotion(characterObject, 0, 0, 1, 1);
    }
    else {
        omSetTra(characterObject, -400.0f, 37.0f, -425.0f);
        SetModelMotion(characterObject, 0, 0, 1, 1);
        SetModelMotion(characterObject, 1, 5, 1, 1);
    }
    characterObject->work[1] = characterObject->work[2] = characterObject->work[3] = 0;
    motionState->animationSubstate = motionState->animationRepeatCount = motionState->secondaryAnimationTimer = motionState->animationFlags = 0;
    motionState->animationIndex = motionState->animationState = motionState->animationMode = motionState->animationTimer = 0;
}

void UpdateCharacterMotionObject(OMOBJ *object)
{
    CharacterMotionState *characterState;
    s32 characterIndex;

    characterIndex = object->work[0];
    characterState = &characterMotionStates[characterIndex];
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M437, 57));
    if (characterIndex != 0) {
        object->mtnId[0] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 58));
        object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 59));
        object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 63));
        object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 64));
        object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 65));
        object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 66));
        object->mtnId[6] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 67));
        object->mtnId[7] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 68));
        object->mtnId[8] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 69));
        object->mtnId[9] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 71));
        omSetTra(object, 400.0f, 37.0f, -425.0f);
        omSetRot(object, 0.0f, -15.0f, 0.0f);
        SetModelMotion(object, 0, 0, 1, 1);
        StartCharacterEffect(minigameProcess, &characterState->characterEffects, object->mdlId[0], "g007m0-skl_at");
        ResetCharacterEffects(&characterState->characterEffects);
    }
    else {
        object->mtnId[0] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 60));
        object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 61));
        object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 62));
        object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 70));
        object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 72));
        omSetTra(object, -400.0f, 37.0f, -425.0f);
        SetModelMotion(object, 0, 0, 1, 1);
        object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M437, 73));
        object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_M437, 74));
        object->mtnId[6] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_M437, 75));
        Hu3DModelHookSet(object->mdlId[0], "g007m0-itemhook_r", object->mdlId[1]);
        Hu3DModelLayerSet(object->mdlId[0], 2);
        Hu3DModelLayerSet(object->mdlId[1], 2);
        SetModelMotion(object, 1, 5, 1, 1);
    }
    Hu3DModelShadowSet(object->mdlId[0]);
    object->data = &characterMotionStates[characterIndex];
    object->objFunc = UpdatePlayerCharacterObject;
}

void SortPlayersByTurnOrder(void)
{
    CharacterActionState *playerState;
    s32 playerOrder[4];
    s32 playerIndex;

    playerOrder[0] = GWPlayerCurrGet();
    for (playerIndex = 1; playerIndex < 4; playerIndex++) {
        playerOrder[playerIndex] = playerOrder[playerIndex - 1] + 1;
        if (playerOrder[playerIndex] >= 4) {
            playerOrder[playerIndex] = 0;
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerState = &characterActionStates[playerIndex];
        playerState->playerIndex = playerOrder[playerIndex];
        playerState->playerGroup = GWPlayerCfg[playerState->playerIndex].group;
        playerState->isCpu = GWPlayerCfg[playerState->playerIndex].iscom;
        playerState->aiDifficulty = GWPlayerCfg[playerState->playerIndex].diff;
        playerState->characterId = GWPlayerCfg[playerState->playerIndex].character;
        playerState->controllerIndex = GWPlayerCfg[playerState->playerIndex].padIdx;
    }
}

void SetPlayerActionState(s32 playerIndex, s32 actionState)
{
    CharacterActionState *playerState;

    playerState = &characterActionStates[playerIndex];
    playerState->modelVisible = actionState;
}

void UpdatePlayerCharacterState(OMOBJ *object)
{
    CharacterActionState *playerState;

    playerState = object->data;
    if (playerState->modelVisible != 0) {
        Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    }
    else {
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    }
}

void SetPlayerActionResult(s32 playerIndex, s32 actionResult)
{
    CharacterActionState *playerState;

    playerState = &characterActionStates[playerIndex];
    playerState->fadeState = actionResult;
    ResetCharacterEffects(&playerState->characterEffects);
    EnsureModelMotion(playerState->characterObject, 0, 2, 10, 0);
}

void UpdatePlayerMaterialOpacity(OMOBJ *object)
{
    CharacterActionState *playerState;
    HSFMATERIAL *material;
    HSFDATA *modelData;
    char **eyeTextures;
    s32 materialVisible;
    s32 materialIndex, attributeIndex;
    HU3DMODEL *characterModel;
    HSFATTRIBUTE *materialAttribute;

    playerState = &characterActionStates[object->work[0]];
    if (playerState->fadeState != 1) {
        return;
    }
    materialVisible = 1;
    characterModel = &Hu3DData[object->mdlId[0]];
    modelData = characterModel->hsf;
    material = modelData->material;
    if (playerState->characterId == 3) {
        for (materialIndex = 0; materialIndex < characterModel->hsf->materialNum; materialIndex++, material++) {
            if (materialIndex == 1 || materialIndex == 2) {
                continue;
            }
            material->color[0] *= playerState->materialOpacity;
            if (material->color[0] == 0) {
                material->color[0] = 0;
            }
            material->color[1] *= playerState->materialOpacity;
            if (material->color[1] == 0) {
                material->color[1] = 0;
            }
            material->color[2] *= playerState->materialOpacity;
            if (material->color[2] == 0) {
                material->color[2] = 0;
            }
        }
    }
    else {
        eyeTextures = CharModelEyeBmpGet(playerState->characterId, 2);
        for (materialIndex = 0; materialIndex < modelData->materialNum; materialIndex++, material++) {
            materialVisible = 1;
            for (attributeIndex = 0; attributeIndex < material->attrNum; attributeIndex++) {
                materialAttribute = &modelData->attribute[material->attr[attributeIndex]];
                if (strcmp(eyeTextures[0], materialAttribute->bitmap->name) == 0 || strcmp(eyeTextures[1], materialAttribute->bitmap->name) == 0) {
                    materialVisible = 0;
                }
            }
            if (materialVisible == 0) {
                continue;
            }
            material->color[0] *= playerState->materialOpacity;
            if (material->color[0] == 0) {
                material->color[0] = 0;
            }
            material->color[1] *= playerState->materialOpacity;
            if (material->color[1] == 0) {
                material->color[1] = 0;
            }
            material->color[2] *= playerState->materialOpacity;
            if (material->color[2] == 0) {
                material->color[2] = 0;
            }
        }
    }
    playerState->materialOpacity -= 0.01;
    if (playerState->materialOpacity < 0.0f) {
        playerState->fadeState = 99;
        playerState->materialOpacity = 0.0f;
    }
}

void SetPlayerActionParameters(s32 firstValue, s32 secondValue, s32 thirdValue)
{
    CharacterActionState *playerState;

    playerState = &characterActionStates[firstValue];
    if (playerState->actionState == 0) {
        playerState->actionState = secondValue;
        playerState->actionMode = thirdValue;
        playerState->actionFlag = 0;
    }
    else if (playerState->actionState == 2) {
        playerState->actionMode = 0;
        playerState->actionFlag = 1;
    }
}

float lastPlayerJumpHeight = -1.0f;

void UpdatePlayerCharacterAction(OMOBJ *object)
{
    Mtx hookMatrix;
    CharacterActionState *playerState;
    CharacterActionState *otherPlayer;
    CharacterActionState *otherPlayerState;
    CharacterSceneState *specialCharacter;
    CharacterSceneState *specialCharacterState;
    s32 playerIndex;

    playerState = object->data;
    switch (playerState->actionState) {
        case 1:
            playerState->actionState = 2;
            playerState->jumpVelocity = -1.5f;
            lastPlayerJumpHeight = -1.0f;
            SetModelMotion(object, 0, 5, 15, 0);
            break;
        case 2:
            if (playerState->jumpVelocity >= 1.7f) {
                playerState->jumpVelocity = 1.7f;
            }
            object->trans.y += playerState->jumpVelocity * playerState->jumpVelocity * (playerState->jumpVelocity >= 0.0f ? -25.0f : 25.0f);
            if (playerState->actionFlag != 0 && object->trans.y >= 25.0f + playerState->groundHeight && playerState->actionMode == 0) {
                playerState->actionState = 5;
                playerState->jumpVelocity = 0.0f;
                lastPlayerJumpHeight = object->trans.y - playerState->groundHeight;
                EnsureModelMotion(object, 0, 7, 15, 0);
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if (playerIndex != object->work[0]) {
                        otherPlayer = &characterActionStates[playerIndex];
                        otherPlayer->characterObject->work[2] = rand8() % 10;
                    }
                }
            }
            if (((!(HuPadBtn[playerState->controllerIndex] & 0x100) && playerState->actionMode == 0) || playerState->actionMode == 2) && playerState->jumpVelocity < -0.8f) {
                playerState->jumpVelocity = -0.8f;
            }
            if (object->trans.y <= playerState->groundHeight && playerState->jumpVelocity > 0.0f) {
                object->trans.y = playerState->groundHeight;
                playerState->actionState = 4;
                playerState->jumpVelocity = 0.0f;
                EnsureModelMotion(object, 0, 6, 5, 0);
            }
            playerState->jumpVelocity += 0.09f;
            break;
        case 4:
            if (playerState->jumpVelocity++ >= 15.0f) {
                playerState->actionState = 0;
                playerState->actionFlag = 0;
                EnsureModelMotion(object, 0, 0, 15, 1);
            }
            break;
        case 5:
            if (GetSpecialCharacterMotionProgress() > 0.75f) {
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if (playerIndex != object->work[0]) {
                        otherPlayerState = &characterActionStates[playerIndex];
                        if (otherPlayerState->characterObject->work[2]-- == 0) {
                            EnsureModelMotion(otherPlayerState->characterObject, 0, 11, 5, 0);
                        }
                    }
                }
            }
            if (playerState->jumpVelocity++ >= 25.0f) {
                playerState->actionState = 6;
            }
            break;
        case 6:
            if (playerState->jumpVelocity >= 1.7f) {
                playerState->jumpVelocity = 1.7f;
            }
            object->trans.y += playerState->jumpVelocity * playerState->jumpVelocity * (playerState->jumpVelocity >= 0.0f ? -17.5f : 17.5f);
            if (object->trans.y <= playerState->groundHeight && playerState->jumpVelocity > 0.0f) {
                object->trans.y = playerState->groundHeight;
                playerState->actionState = 7;
                playerState->jumpVelocity = 0.0f;
                specialCharacter = &characterSceneState;
                if (lastPlayerJumpHeight <= 135.0f) {
                    SetModelMotion(specialCharacter->characterObject, 0, 2, 5, 0);
                }
                else if (lastPlayerJumpHeight <= 245.0f) {
                    SetModelMotion(specialCharacter->characterObject, 0, 1, 5, 0);
                }
                else {
                    SetModelMotion(specialCharacter->characterObject, 0, 0, 5, 0);
                }
                HuAudFXPlay(0x6F5);
                omVibrate(playerState->playerIndex, 12, 6, 6);
                EnsureModelMotion(object, 0, 8, 15, 0);
            }
            playerState->jumpVelocity += 0.1f;
            break;
        case 7:
            if (playerState->jumpVelocity++ >= 15.0f) {
                specialCharacterState = &characterSceneState;
                if (lastPlayerJumpHeight <= 135.0f) {
                    AdvanceBalloonMotionFrame(1.0f);
                    AdvanceBalloonRotationTimer(1.0f);
                    HuAudFXPlay(0x6F6);
                }
                else if (lastPlayerJumpHeight <= 245.0f) {
                    AdvanceBalloonMotionFrame(2.0f);
                    AdvanceBalloonRotationTimer(2.0f);
                    HuAudFXPlay(0x6F7);
                }
                else {
                    AdvanceBalloonMotionFrame(3.0f);
                    AdvanceBalloonRotationTimer(3.0f);
                    HuAudFXPlay(0x6F8);
                }
                EnsureModelMotion(object, 0, 9, 15, 0);
                playerState->jumpVelocity = 0.0f;
                playerState->actionState = 4;
            }
            break;
    }
    if ((playerState->actionState == 4 || playerState->actionState == 7) && playerState->actionMode == 0) {
        CharacterSceneState *temp = &characterSceneState;
        Hu3DModelObjMtxGet(temp->characterObject->mdlId[0], "m437a01-chara-point", hookMatrix);
        object->trans.y = hookMatrix[1][3];
    }
}

void SetPlayerCharacterMotionState(s32 firstValue, s32 secondValue)
{
    CharacterActionState *playerState;

    playerState = &characterActionStates[firstValue];
    playerState->selectedAction = secondValue;
    playerState->actionDelay = 15 - secondValue * 5;
    playerState->actionFlag = 0;
}

void InitializePlayerCharacterAction(OMOBJ *object)
{
    CharacterActionState *playerState = &characterActionStates[object->work[0]];
    Vec characterPositions[] = { { 0.0f, 0.0f, 0.0f }, { -1000.0f, 0.0f, 800.0f }, { -200.0f, 0.0f, 800.0f }, { 1000.0f, 0.0f, 800.0f } };

    if (playerState->selectedAction < 1) {
        return;
    }
    if (playerState->actionDelay-- <= 0) {
        EnsureModelMotion(object, 0, 4, 15, 1);
        playerState->actionFlag = 1;
    }
    if (playerState->actionFlag != 0) {
        MoveCharacterTowardPosition(object, characterPositions[playerState->selectedAction], -1.0f, playerState->actionState + 7.0f, 30.0f, 5.0f, 0);
    }
}

void UpdatePlayerActionObject(OMOBJ *object)
{
    UpdatePlayerCharacterState(object);
    UpdatePlayerMaterialOpacity(object);
    UpdatePlayerCharacterAction(object);
    InitializePlayerCharacterAction(object);
}

void UpdatePlayerActionState(CharacterActionState *playerState)
{
    s32 playerIndex;
    OMOBJ *characterObject;

    playerIndex = playerState->characterObject->work[0];
    characterObject = playerState->characterObject;
    omSetTra(characterObject, playerIndex * -150 - (rand8() % 100) * 2, 0.0f, (rand8() % 100) * 2 + 250);
    omSetRot(characterObject, 0.0f, 180.0f, 0.0f);
    SetModelMotion(characterObject, 0, 0, 1, 1);
    ResetCharacterEffects(&playerState->characterEffects);
    AddCharacterEffect(&playerState->characterEffects, characterSceneState.characterObject->mdlId[0]);
    playerState->materialOpacity = 1.0f;
    characterObject->work[1] = characterObject->work[2] = characterObject->work[3] = 0;
    playerState->actionTimer = playerState->actionResult = playerState->actionDelay = playerState->actionFlag = 0;
    playerState->modelVisible = playerState->fadeState = playerState->actionState = playerState->actionMode = 0;
    playerState->selectedAction = 0;
}

void UpdatePlayerActionObjectState(OMOBJ *object)
{
    CharacterActionState *playerState;
    s32 playerIndex;

    playerIndex = object->work[0];
    playerState = &characterActionStates[playerIndex];
    object->mdlId[0] = CharModelCreate(playerState->characterId, 2);
    object->mtnId[0] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0));
    object->mtnId[1] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 3));
    object->mtnId[2] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 28));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, playerState->characterId));
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M437, 8 + playerState->characterId));
    object->mtnId[5] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 5));
    object->mtnId[6] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 6));
    object->mtnId[7] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 8));
    object->mtnId[8] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 10));
    object->mtnId[9] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 9));
    object->mtnId[10] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 2));
    object->mtnId[11] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 78));
    object->mtnId[12] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 27));
    omSetTra(object, playerIndex * -150 + (rand8() % 100) * 5, 0.0f, (rand8() % 100) * 5 + 250);
    omSetRot(object, 0.0f, 180.0f, 0.0f);
    Hu3DModelShadowSet(object->mdlId[0]);
    SetModelMotion(object, 0, 0, 1, 1);
    CharMotionDataClose(playerState->characterId);
    StartCharacterEffectForPlayer(minigameProcess, &playerState->characterEffects, object->mdlId[0], playerState->characterId);
    ResetCharacterEffects(&playerState->characterEffects);
    playerState->modelVisible = 1;
    object->data = &characterActionStates[playerIndex];
    object->objFunc = UpdatePlayerActionObject;
}

void StartCharacterMotionTimeline(void)
{
    characterMotionTimeline.sceneSubstate = 1;
    characterMotionTimeline.scenePhase = 1;
    characterMotionTimeline.sceneEventCount = 0;
}

void UpdateCharacterMotionTimeline(OMOBJ *object)
{
    Mtx modelHookMatrix;
    CharacterMotionTimeline *sceneState;
    CharacterAnimationState *characterState;
    CharacterMotionState *itemCharacter;
    s32 modelIndex;
    s32 modelIndex;

    sceneState = object->data;
    characterState = &characterAnimationState;
    switch (sceneState->sceneSubstate) {
        case 1:
            sceneState->sceneSubstate = 2;
            for (modelIndex = 1; modelIndex < 7; modelIndex++) {
                Hu3DModelPosSet(object->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
                Hu3DModelRotSet(object->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
                Hu3DModelScaleSet(object->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
                sceneState->characterMotionTimes[modelIndex] = -1.0f + rand8() % 3;
                sceneState->characterMotionDurations[modelIndex] = 0.0f;
                sceneState->characterMotionThresholds[modelIndex] = -(50.0f * (modelIndex - 1));
                sceneState->characterMotionMarkers[modelIndex] = 0.0f;
                sceneState->characterMotionEvents[modelIndex] = 0.5f;
                sceneState->characterMotionSoundTimes[modelIndex] = 0.0f;
            }
            SetModelMotion(characterState->characterObject, 0, 3, 10, 0);
            itemCharacter = &characterMotionStates[0];
            SetModelMotion(itemCharacter->characterObject, 0, 1, 60, 1);
            SetModelMotion(itemCharacter->characterObject, 1, 6, 60, 1);
            break;
        case 2:
            if (isMotionAtTimeThreshold(characterState->characterObject, 0, 3, 90, 1) != 0) {
                sceneState->sceneSubstate = 3;
                Hu3DMotionTimeSet(characterState->characterObject->mdlId[0], 90.0f);
                Hu3DMotionSpeedSet(characterState->characterObject->mdlId[0], 0.0f);
                CharFXPlay(characterActionStates[minigameRoundState.currentPlayerIndex].characterId, 0x123);
                HuAudFXPlay(0x6D5);
                omVibrate(characterActionStates[minigameRoundState.currentPlayerIndex].playerIndex, 120, 12, 0);
            }
            break;
        case 3:
            Hu3DModelObjMtxGet(characterState->characterObject->mdlId[0], "g000m0-itemhook_M", modelHookMatrix);
            modelIndex = minigameRoundState.currentPlayerIndex;
            SetPlayerActionResult(modelIndex, 1);
            for (modelIndex = 1; modelIndex < 7; modelIndex++) {
                if (sceneState->scenePhase == 1 || (sceneState->scenePhase == 0 && sceneState->characterMotionThresholds[modelIndex] > 0.0f)) {
                    sceneState->characterMotionThresholds[modelIndex] += 10.0f;
                }
                if (sceneState->characterMotionThresholds[modelIndex] > 0.0f) {
                    sceneState->characterMotionMarkers[modelIndex] += 0.1f;
                    sceneState->characterMotionTimes[modelIndex] += rand8() % 5 - 2;
                    sceneState->characterMotionDurations[modelIndex] -= rand8() % 5 - 2 + 2.5f;
                    if (sceneState->characterMotionSoundTimes[modelIndex] == 0.0f) {
                        SetModelMotion(object, modelIndex, modelIndex, 1, 1);
                    }
                    if (sceneState->characterMotionSoundTimes[modelIndex] < 8.0f) {
                        sceneState->characterMotionEvents[modelIndex] += 0.025f;
                    }
                    else if (sceneState->characterMotionSoundTimes[modelIndex] < 23.0f) {
                        sceneState->characterMotionEvents[modelIndex] = 0.7f;
                    }
                    else if (sceneState->characterMotionSoundTimes[modelIndex] < 31.0f) {
                        sceneState->characterMotionEvents[modelIndex] -= 0.1f;
                        if (sceneState->characterMotionEvents[modelIndex] < 0.0f) {
                            sceneState->characterMotionEvents[modelIndex] = 0.0f;
                        }
                    }
                    sceneState->characterMotionSoundTimes[modelIndex] += 1.0f;
                }
                Hu3DModelPosSet(object->mdlId[modelIndex], modelHookMatrix[0][3] + sceneState->characterMotionTimes[modelIndex], modelHookMatrix[1][3] + sceneState->characterMotionDurations[modelIndex], modelHookMatrix[2][3] + sceneState->characterMotionThresholds[modelIndex]);
                Hu3DModelScaleSet(object->mdlId[modelIndex], 1.5f * sceneState->characterMotionMarkers[modelIndex], 1.5f * sceneState->characterMotionMarkers[modelIndex], 1.5f * sceneState->characterMotionMarkers[modelIndex]);
                Hu3DModelTPLvlSet(object->mdlId[modelIndex], sceneState->characterMotionEvents[modelIndex]);
                if (sceneState->characterMotionSoundTimes[modelIndex] >= 31.0f) {
                    sceneState->characterMotionTimes[modelIndex] = 0.0f;
                    sceneState->characterMotionDurations[modelIndex] = 0.0f;
                    sceneState->characterMotionThresholds[modelIndex] = 0.0f;
                    sceneState->characterMotionMarkers[modelIndex] = 0.0f;
                    sceneState->characterMotionEvents[modelIndex] = 0.5f;
                    sceneState->characterMotionSoundTimes[modelIndex] = 0.0f;
                }
            }
            if (sceneState->sceneEventCount++ == 120) {
                sceneState->scenePhase = 0;
                Hu3DMotionSpeedSet(characterState->characterObject->mdlId[0], 1.0f);
            }
            else if (sceneState->sceneEventCount >= 180) {
                sceneState->sceneSubstate = 4;
            }
            break;
        case 4:
            sceneState->sceneSubstate = 0;
            SetModelMotion(characterState->characterObject, 0, 0, 30, 1);
            break;
    }
    if (sceneState->sceneEventCount >= 10 && sceneState->sceneEventCount <= 120) {
        StartIndefiniteCameraShake(6, 8.0f);
    }
}

void UpdateCharacterTimelineObject(OMOBJ *object)
{
    UpdateCharacterMotionTimeline(object);
}

void InitializeCharacterMotionTimeline(CharacterMotionTimeline *value)
{
    OMOBJ *characterObject;
    s32 modelIndex;

    characterObject = value->characterObject;
    for (modelIndex = 1; modelIndex < 7; modelIndex++) {
        Hu3DModelPosSet(characterObject->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(characterObject->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(characterObject->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
        SetModelMotion(characterObject, modelIndex, modelIndex, 1, 2);
        value->characterMotionTimes[modelIndex] = 0.0f;
        value->characterMotionDurations[modelIndex] = 0.0f;
        value->characterMotionThresholds[modelIndex] = 0.0f;
        value->characterMotionMarkers[modelIndex] = 0.0f;
        value->characterMotionEvents[modelIndex] = 0.0f;
        value->characterMotionSoundTimes[modelIndex] = 0.0f;
    }
    value->sceneSubstate = value->scenePhase = value->sceneFrameCount = value->sceneEvent = 0;
    value->sceneEventCount = value->sceneFlags = value->sceneMode = value->sceneValue = 0;
}

void UpdateCharacterMotionMarkers(OMOBJ *object)
{
    s32 i;

    for (i = 1; i < 7; i++) {
        if (i % 2 == 0) {
            object->mdlId[i] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M437, 55));
            object->mtnId[i] = Hu3DMotionIDGet(object->mdlId[i]);
        }
        else {
            object->mdlId[i] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M437, 56));
            object->mtnId[i] = Hu3DMotionIDGet(object->mdlId[i]);
        }
        Hu3DModelPosSet(object->mdlId[i], 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(object->mdlId[i], 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(object->mdlId[i], 0.0f, 0.0f, 0.0f);
        SetModelMotion(object, i, i, 1, 2);
        Hu3DModelLayerSet(object->mdlId[i], 2);
    }
    object->data = &characterMotionTimeline;
    object->objFunc = UpdateCharacterTimelineObject;
}

void SetCameraForIntro(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->targetCenterX = 0.0f;
    cameraState->targetCenterY = 300.0f;
    cameraState->targetCenterZ = 1740.0f;
    cameraState->targetRotationX = -10.0f;
    cameraState->targetRotationY = 0.0f;
    cameraState->targetRotationZ = 0.0f;
    cameraState->targetZoom = 300.0f;
    ApplyCameraShake(cameraState, 200.0f);
}

void TransitionCameraToIntro(void)
{
    CameraState *cameraState;
    CameraState cameraPreset;

    cameraState = &globalCameraState;
    cameraPreset.cameraCenterX = 0.0f;
    cameraPreset.cameraCenterY = 200.0f;
    cameraPreset.cameraCenterZ = -50.0f;
    cameraPreset.cameraRotationX = 5.0f;
    cameraPreset.cameraRotationY = 0.0f;
    cameraPreset.cameraRotationZ = 0.0f;
    cameraPreset.cameraZoom = 200.0f;
    SmoothCameraTargetWithShake(cameraState, &cameraPreset, cameraState->animationFrame++, 30.0f, 5.0f);
}

void SetCameraForCharacterScene(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->targetCenterX = 0.0f;
    cameraState->targetCenterY = 200.0f;
    cameraState->targetCenterZ = -50.0f;
    cameraState->targetRotationX = 5.0f;
    cameraState->targetRotationY = 0.0f;
    cameraState->targetRotationZ = 0.0f;
    cameraState->targetZoom = 200.0f;
    ApplyCameraShake(cameraState, 200.0f);
}

void SetCameraForBalloonScene(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->targetCenterX = 0.0f;
    cameraState->targetCenterY = 300.0f;
    cameraState->targetCenterZ = -550.0f;
    cameraState->targetRotationX = -15.0f;
    cameraState->targetRotationY = 0.0f;
    cameraState->targetRotationZ = 0.0f;
    cameraState->targetZoom = 1200.0f;
    ApplyCameraShake(cameraState, 45.0f);
}

void SetCameraForPlayerScene(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->targetCenterX = 0.0f;
    cameraState->targetCenterY = 150.0f;
    cameraState->targetCenterZ = -550.0f;
    cameraState->targetRotationX = -15.0f;
    cameraState->targetRotationY = 0.0f;
    cameraState->targetRotationZ = 0.0f;
    cameraState->targetZoom = 1200.0f;
    ApplyCameraShake(cameraState, 30.0f);
}

void TransitionCameraToPlayerScene(void)
{
    CameraState *cameraState;
    CameraState cameraPreset;

    cameraState = &globalCameraState;
    cameraPreset.cameraCenterX = 0.0f;
    cameraPreset.cameraCenterY = 300.0f;
    cameraPreset.cameraCenterZ = 0.0f;
    cameraPreset.cameraRotationX = 0.0f;
    cameraPreset.cameraRotationY = 0.0f;
    cameraPreset.cameraRotationZ = 0.0f;
    cameraPreset.cameraZoom = 1200.0f;
    BlendCameraTargetWithShake(cameraState, &cameraPreset, cameraState->animationFrame++, 30.0f, 10.0f);
}

void SetCameraForResultScene(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->targetCenterX = 0.0f;
    cameraState->targetCenterY = 150.0f;
    cameraState->targetCenterZ = -550.0f;
    cameraState->targetRotationX = -15.0f;
    cameraState->targetRotationY = 0.0f;
    cameraState->targetRotationZ = 0.0f;
    cameraState->targetZoom = 1200.0f;
    ApplyCameraShake(cameraState, 20.0f);
}

void SetCameraForCloseup(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->targetCenterX = 0.0f;
    cameraState->targetCenterY = 300.0f;
    cameraState->targetCenterZ = -550.0f;
    cameraState->targetRotationX = -15.0f;
    cameraState->targetRotationY = 0.0f;
    cameraState->targetRotationZ = 0.0f;
    cameraState->targetZoom = 1200.0f;
    ApplyCameraShake(cameraState, 20.0f);
}

void TransitionCameraToPreset(s32 presetIndex)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    ResetCameraTarget(cameraState);
    switch (presetIndex) {
        case 0:
            SetCameraUpdateCallback(SetCameraForIntro);
            break;
        case 1:
            SetCameraUpdateCallback(TransitionCameraToIntro);
            break;
        case 2:
            SetCameraUpdateCallback(SetCameraForCharacterScene);
            break;
        case 3:
            SetCameraUpdateCallback(SetCameraForBalloonScene);
            break;
        case 4:
            SetCameraUpdateCallback(SetCameraForPlayerScene);
            break;
        case 5:
            SetCameraUpdateCallback(TransitionCameraToPlayerScene);
            break;
        case 10:
            SetCameraUpdateCallback(SetCameraForResultScene);
            break;
        case 11:
            SetCameraUpdateCallback(SetCameraForCloseup);
            break;
    }
}

void ShowPlayerIntroMessage(void)
{
    float messageSize[] = { 0.0f, 0.0f };
    s32 windowId;

    minigameResult = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 0));
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 1));
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 80.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D1);
    SetCharacterMotionForState(1, 10, 0, 1);
    SetMessageWindowText(windowId, MAKE_MESSID(29, 0));
    WaitForMessageConfirmation(windowId, 0);
    SetCharacterMotionForState(1, 10, 0, 1);
    SetMessageWindowText(windowId, MAKE_MESSID(29, 1));
    WaitForMessageConfirmation(windowId, 0);
    SetCharacterMotionForState(0, 10, 1, 1);
    DismissMessageWindow(windowId);
    minigameResult = 1;
    HuPrcEnd();
}

void ShowPlayerTurnMessage(void)
{
    float messageSize[] = { 0.0f, 0.0f };
    s32 windowId;

    minigameResult = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 4));
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 100.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D2);
    SetCharacterMotionForState(2, 10, 0, 1);
    SetMessageWindowText(windowId, MAKE_MESSID(29, 4));
    WaitForMessageConfirmation(windowId, REFRESH_RATE);
    SetSceneDialogueState(3);
    WaitForMessageConfirmation(windowId, REFRESH_RATE);
    SetCharacterMotionForState(0, 30, 1, 1);
    DismissMessageWindow(windowId);
    minigameResult = 1;
    HuPrcEnd();
}

void ShowPlayerResultMessage(void)
{
    float messageSize[] = { 0.0f, 0.0f };
    s32 windowId;

    minigameResult = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 64));
    windowId = CreateMessageWindowVariant(550.0f - messageSize[0], 340.0f, messageSize[0], messageSize[1], 1);
    HuAudFXPlayVolPan(0x65, 0x7F, 0x60);
    SetSecondCharacterBodyMotion(4, 10, 0, 1);
    SetMessageWindowText(windowId, MAKE_MESSID(29, 64));
    ShowMessageChoice(windowId);
    SetSecondCharacterBodyMotion(0, 10, 1, 1);
    DismissMessageWindow(windowId);
    minigameResult = 1;
    HuPrcEnd();
}

void ShowCharacterResultMessages(s32 firstValue)
{
    float messageSize[2] = { 0.0f, 0.0f };
    s32 messageIds[] = { MAKE_MESSID(29, 6), MAKE_MESSID(29, 66), MAKE_MESSID(29, 68), MAKE_MESSID(29, 70), MAKE_MESSID(29, 72), MAKE_MESSID(29, 74),
        MAKE_MESSID(29, 76), -1 };
    s32 windowId;

    if (messageIds[firstValue] == -1) {
        return;
    }
    ExpandMessageDimensions(messageSize, messageIds[firstValue]);
    windowId = CreateMessageWindowVariant(20.0f, 360.0f, messageSize[0], messageSize[1], 0);
    HuAudFXPlayVolPan(0x3E, 0x7F, 0x20);
    SetFirstCharacterBodyMotion(2, 10, 0, 1);
    SetFirstCharacterAccessoryMotion(5, 10, 2, 1);
    SetMessageWindowText(windowId, messageIds[firstValue]);
    WaitForMessageConfirmation(windowId, 0);
    SetFirstCharacterBodyMotion(0, 10, 1, 1);
    SetFirstCharacterAccessoryMotion(5, 10, 1, 1);
    DismissMessageWindow(windowId);
}

void ShowRoundResultMessages(void)
{
    float messageSize[2] = { 0.0f, 0.0f };
    s32 messageIds[] = { MAKE_MESSID(29, 5), MAKE_MESSID(29, 65), MAKE_MESSID(29, 67), MAKE_MESSID(29, 69), MAKE_MESSID(29, 71), MAKE_MESSID(29, 73),
        MAKE_MESSID(29, 75), MAKE_MESSID(29, 77) };
    s32 messageIndex;
    s32 windowId;

    minigameResult = 0;
    for (messageIndex = 0; messageIndex < 8; messageIndex++) {
        ExpandMessageDimensions(messageSize, messageIds[messageIndex]);
    }
    windowId = CreateMessageWindowVariant(550.0f - messageSize[0], 340.0f, messageSize[0], messageSize[1], 1);
    HuAudFXPlayVolPan(0x65, 0x7F, 0x60);
    for (messageIndex = 0; messageIndex < 8; messageIndex++) {
        SetSecondCharacterBodyMotion(4, 10, 0, 1);
        SetMessageWindowText(windowId, messageIds[messageIndex]);
        WaitForMessageConfirmation(windowId, 0);
        SetSecondCharacterBodyMotion(0, 10, 1, 1);
        ShowCharacterResultMessages(messageIndex);
    }
    DismissMessageWindow(windowId);
    minigameResult = 1;
    HuPrcEnd();
}

void ShowMinigameResultMessage(void)
{
    float messageSize[] = { 0.0f, 0.0f };
    s32 windowId;

    minigameResult = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 78));
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 80.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D2);
    SetCharacterMotionForState(1, 10, 0, 1);
    SetMessageWindowText(windowId, MAKE_MESSID(29, 78));
    WaitForMessageConfirmation(windowId, 0);
    SetCharacterMotionForState(0, 10, 1, 1);
    DismissMessageWindow(windowId);
    minigameResult = 1;
    HuPrcEnd();
}

void EmptySequenceCallback(void) { }

s32 IsIntroMusicReady(void)
{
    if (AdvanceMotionSequenceTimer(&primaryMotionSequence, 0) == 80) {
        TransitionCameraToPreset(1);
        sequenceHandles.currentSequenceId = HuAudSeqPlay(6);
    }
    if (AdvanceMotionSequenceTimer(&primaryMotionSequence, 0) == 115) {
        TransitionCameraToPreset(2);
    }
    if (AdvanceMotionSequenceTimer(&primaryMotionSequence, 0) == 180) {
        TransitionCameraToPreset(3);
    }
    if (AdvanceMotionSequenceTimer(&primaryMotionSequence, 0) == 0) {
        SetSceneDialogueState(2);
    }
    if (AdvanceMotionSequenceTimer(&primaryMotionSequence, 0) == 150) {
        SetSceneDialogueState(1);
    }
    if (AdvanceMotionSequenceTimer(&primaryMotionSequence, 1) != 0) {
        return AdvanceMotionSequence(&primaryMotionSequence);
    }
    return 0;
}

void ShowPlayerIntroDialogue(void)
{
    HuPrcChildCreate(ShowPlayerIntroMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 AdvanceIntroMotionSequence(void)
{
    if (minigameResult != 0 && AdvanceMotionSequenceTimer(&primaryMotionSequence, 1) != 0) {
        return AdvanceMotionSequence(&primaryMotionSequence);
    }
    return 0;
}

void ShowPlayerTurnDialogue(void)
{
    HuPrcChildCreate(ShowPlayerTurnMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 AdvanceTurnMotionSequence(void)
{
    if (minigameResult != 0 && AdvanceMotionSequenceTimer(&primaryMotionSequence, 1) != 0) {
        return AdvanceMotionSequence(&primaryMotionSequence);
    }
    return 0;
}

void SetTurnCameraPreset(void)
{
    TransitionCameraToPreset(10);
}

s32 AdvanceCharacterMotionSequence(void)
{
    if (AdvanceMotionSequenceTimer(&primaryMotionSequence, 1) != 0) {
        return AdvanceMotionSequence(&primaryMotionSequence);
    }
    return 0;
}

void ShowResultDialogue(void)
{
    HuPrcChildCreate(ShowPlayerResultMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 AdvanceResultDialogue(void)
{
    if (minigameResult != 0 && AdvanceMotionSequenceTimer(&primaryMotionSequence, 1) != 0) {
        if (windowChoiceResult != 0) {
            return AdvanceMotionSequence(&primaryMotionSequence);
        }
        else {
            return SetMotionSequenceIndex(&primaryMotionSequence, 6);
        }
    }
    return 0;
}

void PrepareCharacterCelebration(void)
{
    SetCharacterMotionForState(4, 60, 1, 1);
    HuPrcChildCreate(ShowRoundResultMessages, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 AdvanceCelebrationSequence(void)
{
    if (minigameResult != 0 && AdvanceMotionSequenceTimer(&primaryMotionSequence, 1) != 0) {
        return AdvanceMotionSequence(&primaryMotionSequence);
    }
    return 0;
}

void SetResultCameraAndMotion(void)
{
    TransitionCameraToPreset(11);
    SetCharacterMotionForState(0, 60, 1, 0);
}

s32 AdvanceResultCameraSequence(void)
{
    if (AdvanceMotionSequenceTimer(&primaryMotionSequence, 1) != 0) {
        return AdvanceMotionSequence(&primaryMotionSequence);
    }
    return 0;
}

void ShowMinigameEndDialogue(void)
{
    HuPrcChildCreate(ShowMinigameResultMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 AdvanceMinigameEndSequence(void)
{
    if (minigameResult != 0 && AdvanceMotionSequenceTimer(&primaryMotionSequence, 1) != 0) {
        return AdvanceMotionSequence(&primaryMotionSequence);
    }
    return 0;
}

void InitializeBalloonResultScene(void)
{
    CharacterMotionState *specialCharacter;
    CharacterActionState *unusedPlayerState;
    CharacterSceneState *balloonState;
    s32 playerIndex;

    TransitionCameraToPreset(5);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        SetPlayerActionState(playerIndex, 1);
    }
    specialCharacter = &characterMotionStates[1];
    unusedPlayerState = &characterActionStates[playerIndex];
    balloonState = &characterSceneState;
    ResetCharacterEffects(&specialCharacter->characterEffects);
    AddCharacterEffect(&specialCharacter->characterEffects, balloonState->characterObject->mdlId[2]);
    HuAudSeqFadeOut(sequenceHandles.currentSequenceId, 1000);
}

s32 PositionPlayersForResult(void)
{
    Vec playerPositions[] = { { -200.0f, 0.0f, -250.0f }, { -340.0f, 0.0f, -250.0f }, { -490.0f, 0.0f, -250.0f }, { -640.0f, 0.0f, -250.0f } };
    s32 playerIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (characterActionStates[playerIndex].characterId == 7) {
            MoveCharacterTowardPosition(characterActionStates[playerIndex].characterObject, playerPositions[playerIndex], 90.0f, 9.0f, 30.0f, 15.0f, 3);
        }
        else {
            MoveCharacterTowardPosition(characterActionStates[playerIndex].characterObject, playerPositions[playerIndex], 90.0f, 9.0f, 30.0f, 15.0f, 1);
        }
    }
    if (AdvanceMotionSequenceTimer(&primaryMotionSequence, 1) != 0) {
        SetCharacterMotionForState(4, 60, 1, 1);
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            EnsureModelMotion(characterActionStates[playerIndex].characterObject, 0, 0, 10, 1);
        }
        return AdvanceMotionSequence(&primaryMotionSequence);
    }
    return 0;
}

MotionSequenceEntry primaryMotionSequenceEntries[] = { { EmptySequenceCallback, IsIntroMusicReady, 300 }, { ShowPlayerIntroDialogue, AdvanceIntroMotionSequence, 60 }, { ShowPlayerTurnDialogue, AdvanceTurnMotionSequence, 60 },
    { SetTurnCameraPreset, AdvanceCharacterMotionSequence, 45 }, { ShowResultDialogue, AdvanceResultDialogue, 60 }, { PrepareCharacterCelebration, AdvanceCelebrationSequence, 60 }, { SetResultCameraAndMotion, AdvanceResultCameraSequence, 45 },
    { ShowMinigameEndDialogue, AdvanceMinigameEndSequence, 60 }, { InitializeBalloonResultScene, PositionPlayersForResult, 120 }, { NULL, NULL, -1 } };

void InitializePrimaryMotionSequence(void)
{
    InitializeMotionSequence(&primaryMotionSequence, primaryMotionSequenceEntries);
}

s32 RunPrimaryMotionCallbacks(void)
{
    if (primaryMotionSequence.motionCallback != NULL) {
        primaryMotionSequence.motionCallback();
        primaryMotionSequence.motionCallback = NULL;
    }
    if (primaryMotionSequence.completionCondition != NULL) {
        return primaryMotionSequence.completionCondition();
    }
    return 0;
}

float GetRemainingMotionFrames(void)
{
    return motionEndFrame - currentMotionFrame;
}

void UpdateMinigameRound(void)
{
    MinigameRoundState *roundState;
    CharacterActionState *activePlayer;

    roundState = &minigameRoundState;
    activePlayer = &characterActionStates[roundState->currentPlayerIndex];
    if (GetRemainingMotionFrames() <= 3.0f) {
        activePlayer->characterObject->work[1] = GetRemainingMotionFrames() - 1.0f;
        if (activePlayer->characterObject->work[1] == 0) {
            activePlayer->characterObject->work[1] = rand8() % 3 + 1;
        }
    }
    else {
        activePlayer->characterObject->work[1] = rand8() % 3 + 1;
    }
    activePlayer->characterObject->work[2] = 0;
    activePlayer->characterObject->work[3] = rand8() % 60 + 60;
}

s32 UpdateActivePlayerJump(s32 playerIndex)
{
    MinigameRoundState *roundState;
    CharacterActionState *activePlayer;
    float jumpHeight;

    roundState = &minigameRoundState;
    activePlayer = &characterActionStates[roundState->currentPlayerIndex];
    jumpHeight = activePlayer->characterObject->trans.y - activePlayer->groundHeight;
    switch (playerIndex) {
        case 1:
            if (jumpHeight > 80.0f && jumpHeight <= 135.0f) {
                return 1;
            }
            break;
        case 2:
            if (jumpHeight > 190.0f && jumpHeight <= 245.0f) {
                return 1;
            }
            break;
        case 3:
            if (jumpHeight > 300.0f) {
                return 1;
            }
            break;
    }
    return 0;
}

float UpdatePlayerJumpState(s32 playerIndex, s32 jumpState)
{
    CharacterActionState *playerState;

    playerState = &characterActionStates[playerIndex];
    if (playerState->isCpu != 0) {
        if (jumpState > playerState->characterObject->work[3]) {
            if (playerState->characterObject->work[2] == 0) {
                playerState->characterObject->work[2] = 1;
                SetPlayerActionParameters(playerIndex, 1, 1);
            }
            else if (UpdateActivePlayerJump(playerState->characterObject->work[1]) != 0) {
                SetPlayerActionParameters(playerIndex, 1, 0);
            }
        }
    }
    else if ((HuPadBtnDown[playerState->controllerIndex] & 0x100) || ((HuPadBtnDown[playerState->controllerIndex] & 0x40) && playerState->actionState == 2)) {
        SetPlayerActionParameters(playerIndex, 1, 0);
    }
    return playerState->actionState;
}

void TransitionCameraToRoundStart(void)
{
    CameraState *cameraState;
    CameraState cameraPreset;

    cameraState = &globalCameraState;
    cameraPreset.cameraCenterX = 0.0f;
    cameraPreset.cameraCenterY = 300.0f;
    cameraPreset.cameraCenterZ = 0.0f;
    cameraPreset.cameraRotationX = 0.0f;
    cameraPreset.cameraRotationY = 0.0f;
    cameraPreset.cameraRotationZ = 0.0f;
    cameraPreset.cameraZoom = 1200.0f;
    BlendCameraTargetWithShake(cameraState, &cameraPreset, cameraState->animationFrame++, 30.0f, 10.0f);
}

void SetCameraForCurrentPlayer(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->targetCenterX = 0.0f;
    cameraState->targetCenterY = 300.0f;
    cameraState->targetCenterZ = -350.0f + 450.0f * GetSpecialCharacterMotionProgress();
    cameraState->targetRotationX = 0.0f;
    cameraState->targetRotationY = 0.0f;
    cameraState->targetRotationZ = 0.0f;
    cameraState->targetZoom = 1200.0f;
    ApplyCameraShake(cameraState, 10.0f);
}

void SetCameraForActiveCharacter(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->targetCenterX = 0.0f;
    cameraState->targetCenterY = 300.0f;
    cameraState->targetCenterZ = -50.0f;
    cameraState->targetRotationX = -15.0f;
    cameraState->targetRotationY = 0.0f;
    cameraState->targetRotationZ = 0.0f;
    cameraState->targetZoom = 1200.0f;
    ApplyCameraShake(cameraState, 5.0f);
}

void TransitionCameraToActiveCharacter(void)
{
    CameraState *cameraState;
    CameraState cameraPreset;

    cameraState = &globalCameraState;
    cameraPreset.cameraCenterX = 0.0f;
    cameraPreset.cameraCenterY = 225.0f;
    cameraPreset.cameraCenterZ = -800.0f;
    cameraPreset.cameraRotationX = 0.0f;
    cameraPreset.cameraRotationY = 0.0f;
    cameraPreset.cameraRotationZ = 0.0f;
    cameraPreset.cameraZoom = 1200.0f;
    SmoothCameraTargetWithShake(cameraState, &cameraPreset, cameraState->animationFrame++, 30.0f, 10.0f);
}

void SetCameraForRoundPhase(s32 phaseIndex)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    ResetCameraTarget(cameraState);
    switch (phaseIndex) {
        case 0:
            SetCameraUpdateCallback(TransitionCameraToRoundStart);
            break;
        case 1:
            SetCameraUpdateCallback(SetCameraForCurrentPlayer);
            break;
        case 2:
            SetCameraUpdateCallback(SetCameraForActiveCharacter);
            break;
        case 3:
            SetCameraUpdateCallback(TransitionCameraToActiveCharacter);
            break;
    }
}

void ShowRoundAnnouncement(void)
{
    s32 messageIds[] = { MAKE_MESSID(29, 79), MAKE_MESSID(29, 80), MAKE_MESSID(29, 81), MAKE_MESSID(29, 82) };
    float messageSize[2] = { 0.0f, 0.0f };
    s32 messageChoice;
    s32 windowId;

    messageChoice = rand8() % 4;
    minigameResult = 0;
    ExpandMessageDimensions(messageSize, messageIds[messageChoice]);
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 100.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D2);
    SetCharacterAnimationState(1);
    SetMessageWindowText(windowId, messageIds[messageChoice]);
    WaitForMessageConfirmation(windowId, REFRESH_RATE);
    minigameResult = 1;
    WaitForMessageConfirmation(windowId, REFRESH_RATE);
    DismissMessageWindow(windowId);
    HuPrcEnd();
}

void ShowTurnAnnouncement(void)
{
    s32 messageIds[] = { MAKE_MESSID(29, 83), MAKE_MESSID(29, 84), MAKE_MESSID(29, 85) };
    float messageSize[2] = { 0.0f, 0.0f };
    s32 messageChoice;
    s32 windowId;

    messageChoice = rand8() % 3;
    minigameResult = 0;
    ExpandMessageDimensions(messageSize, messageIds[messageChoice]);
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 100.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D2);
    SetCharacterAnimationState(1);
    SetMessageWindowText(windowId, messageIds[messageChoice]);
    WaitForMessageConfirmation(windowId, REFRESH_RATE);
    minigameResult = 1;
    WaitForMessageConfirmation(windowId, REFRESH_RATE);
    DismissMessageWindow(windowId);
    HuPrcEnd();
}

void ShowMinigameRoundMessage(void)
{
    float messageSize[2] = { 0.0f, 0.0f };
    s32 windowId;

    minigameResult = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 86));
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 80.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D2);
    SetSpecialCharacterAnimation(4);
    minigameResult = 1;
    SetMessageWindowText(windowId, MAKE_MESSID(29, 86));
    WaitForMessageConfirmation(windowId, 30);
    DismissMessageWindow(windowId);
    minigameResult = 2;
    HuPrcEnd();
}

void ShowPlayerScoreMessage(void)
{
    float messageSize[] = { 0.0f, 0.0f };
    s32 windowId;

    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 87));
    windowId = CreateMessageWindowVariant(580.0f - messageSize[0] - 40.0f, 200.0f, messageSize[0], messageSize[1], 1);
    HuAudFXPlayVolPan(0x65, 0x7F, 0x60);
    SetSecondCharacterBodyMotion(9, 10, 0, 0);
    SetMessageWindowText(windowId, MAKE_MESSID(29, 87));
    WaitForMessageConfirmation(windowId, 30);
    DismissMessageWindow(windowId);
    HuPrcEnd();
}

void ShowRoundSummaryMessage(void)
{
    float messageSize[] = { 0.0f, 0.0f };
    s32 windowId;

    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 88));
    windowId = CreateMessageWindowVariant(40.0f, 200.0f, messageSize[0], messageSize[1], 0);
    HuAudFXPlayVolPan(0x3E, 0x7F, 0x20);
    SetFirstCharacterBodyMotion(4, 10, 0, 0);
    SetMessageWindowText(windowId, MAKE_MESSID(29, 88));
    WaitForMessageConfirmation(windowId, 30);
    DismissMessageWindow(windowId);
    HuPrcEnd();
}

void ShowRoundChoiceMessage(void)
{
    s32 messageIds[] = { MAKE_MESSID(29, 89), MAKE_MESSID(29, 90), MAKE_MESSID(29, 91) };
    float messageSize[2] = { 0.0f, 0.0f };
    s32 messageChoice;
    s32 windowId;

    messageChoice = rand8() % 3;
    ExpandMessageDimensions(messageSize, messageIds[messageChoice]);
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 100.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D6);
    SetMessageWindowText(windowId, messageIds[messageChoice]);
    WaitForMessageConfirmation(windowId, 2 * REFRESH_RATE);
    DismissMessageWindow(windowId);
    HuPrcEnd();
}

void ShowGameplayStatusMessage(void)
{
    float messageSize[] = { 0.0f, 0.0f };
    s32 windowId;

    minigameResult = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 92));
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 80.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D6);
    SetCharacterMotionForState(1, 10, 0, 1);
    SetMessageWindowText(windowId, MAKE_MESSID(29, 92));
    WaitForMessageConfirmation(windowId, 2 * REFRESH_RATE);
    SetCharacterMotionForState(0, 30, 1, 1);
    DismissMessageWindow(windowId);
    minigameResult = 1;
    HuPrcEnd();
}

void UpdatePlayerTurnOrder(void)
{
    Mtx modelHookMatrix;
    s32 previousPlayerIndex;
    s32 playerIndex;
    MinigameRoundState *roundState;
    CharacterSceneState *specialCharacter;
    CharacterActionState *activePlayer;
    CharacterActionState *otherPlayer;
    CharacterActionState *previousPlayer;

    roundState = &minigameRoundState;
    specialCharacter = &characterSceneState;
    activePlayer = &characterActionStates[roundState->currentPlayerIndex];
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerIndex != roundState->currentPlayerIndex) {
            otherPlayer = &characterActionStates[playerIndex];
            ResetCharacterEffects(&otherPlayer->characterEffects);
            AddCharacterEffect(&otherPlayer->characterEffects, activePlayer->characterObject->mdlId[0]);
            otherPlayer->characterEffects.effectObject->trans.z = 10.0f;
        }
        else {
            ResetCharacterEffects(&activePlayer->characterEffects);
        }
    }
    if (roundState->roundEventCount != 0) {
        secondaryMotionSequence.duration = 180;
        previousPlayerIndex = roundState->currentPlayerIndex - 1;
        if (previousPlayerIndex < 0) {
            previousPlayerIndex = 3;
        }
        previousPlayer = &characterActionStates[previousPlayerIndex];
        previousPlayer->groundHeight = 0.0f;
        SetPlayerActionParameters(previousPlayerIndex, 1, 2);
    }
    Hu3DModelObjMtxGet(specialCharacter->characterObject->mdlId[0], "m437a01-chara-point", modelHookMatrix);
    activePlayer->groundHeight = modelHookMatrix[1][3];
    SetPlayerActionParameters(roundState->currentPlayerIndex, 1, 1);
}

s32 InitializePlayerRoundPositions(void)
{
    s32 playerCount = 4;
    MinigameRoundState *roundState = &minigameRoundState;
    Vec playerPositions[] = { { 5.0f, 0.0f, -250.0f }, { -175.0f, 0.0f, -250.0f }, { -325.0f, 0.0f, -250.0f }, { -490.0f, 0.0f, -250.0f } };
    CharacterActionState *playerState;
    CharacterActionState *previousPlayer;
    s32 playerIndex;
    s32 previousPlayerIndex;
    s32 positionIndex;

    playerIndex = roundState->currentPlayerIndex;
    if (roundState->roundEventCount != 0) {
        playerCount = 3;
    }
    for (positionIndex = 0; positionIndex < playerCount; positionIndex++) {
        playerState = &characterActionStates[playerIndex];
        if (positionIndex == 0) {
            MoveCharacterTowardPosition(playerState->characterObject, playerPositions[positionIndex], 0.0f, 4.5f, 10.0f, 5.0f, 0);
        }
        else if (AdvanceMotionSequenceTimer(&secondaryMotionSequence, 0) >= positionIndex * 5 + 10) {
            MoveCharacterTowardPosition(playerState->characterObject, playerPositions[positionIndex], 90.0f, 9.0f, 30.0f, 15.0f, 2);
        }
        playerIndex++;
        if (playerIndex >= 4) {
            playerIndex = 0;
        }
    }
    if (roundState->roundEventCount != 0) {
        Vec previousPlayerPositions[] = { { 0.0f, 0.0f, -50.0f }, { -510.0f, 0.0f, -250.0f } };

        previousPlayerIndex = roundState->currentPlayerIndex - 1;
        if (previousPlayerIndex < 0) {
            previousPlayerIndex = 3;
        }
        previousPlayer = &characterActionStates[previousPlayerIndex];
        if (AdvanceMotionSequenceTimer(&secondaryMotionSequence, 0) <= 60) {
            MoveCharacterTowardPosition(previousPlayer->characterObject, previousPlayerPositions[0], -1.0f, 9.0f, 10.0f, 15.0f, 0);
        }
        else if (previousPlayer->characterId == 7) {
            MoveCharacterTowardPosition(previousPlayer->characterObject, previousPlayerPositions[1], 90.0f, 9.0f, 30.0f, 15.0f, 3);
        }
        else {
            MoveCharacterTowardPosition(previousPlayer->characterObject, previousPlayerPositions[1], 90.0f, 9.0f, 30.0f, 15.0f, 1);
        }
    }
    if (AdvanceMotionSequenceTimer(&secondaryMotionSequence, 1) != 0) {
        roundState->roundEventCount = 1;
        return AdvanceMotionSequence(&secondaryMotionSequence);
    }
}

void StartMinigameRound(void)
{
    UpdateMinigameRound();
    SetCameraForRoundPhase(1);
    StartCountdown(5);
}

s32 AdvanceMinigameRound(void)
{
    MinigameRoundState *roundState;

    roundState = &minigameRoundState;
    AdvanceMotionSequenceTimer(&secondaryMotionSequence, 1);
    if (AdvanceMotionSequenceTimer(&secondaryMotionSequence, 0) <= 10) {
        return 0;
    }
    if (IsCountdownComplete() != 0) {
        return SetMotionSequenceIndex(&secondaryMotionSequence, 8);
    }
    if (UpdatePlayerJumpState(roundState->currentPlayerIndex, AdvanceMotionSequenceTimer(&secondaryMotionSequence, 0)) == 5.0f) {
        FinishCountdown();
        return AdvanceMotionSequence(&secondaryMotionSequence);
    }
    return 0;
}

void SetSecondarySequenceDuration(void)
{
    if (GetSpecialCharacterMotionProgress() < 0.75f) {
        secondaryMotionSequence.duration = 60;
    }
    else {
        secondaryMotionSequence.duration = 120;
    }
}

s32 AdvanceSecondaryMotionSequence(void)
{
    if (AdvanceMotionSequenceTimer(&secondaryMotionSequence, 1) != 0) {
        if (HasBalloonMotionFinished()) {
            return SetMotionSequenceIndex(&secondaryMotionSequence, 5);
        }
        else {
            return AdvanceMotionSequence(&secondaryMotionSequence);
        }
    }
    return 0;
}

void ShowRoundPhaseDialogue(void)
{
    if (GetSpecialCharacterMotionProgress() < 0.75f) {
        HuPrcChildCreate(ShowRoundAnnouncement, 100, 0x2000, 0, HuPrcCurrentGet());
    }
    else {
        HuPrcChildCreate(ShowTurnAnnouncement, 100, 0x2000, 0, HuPrcCurrentGet());
    }
}

s32 AdvanceRoundPhaseSequence(void)
{
    if (minigameResult != 0 && AdvanceMotionSequenceTimer(&secondaryMotionSequence, 1) != 0) {
        return AdvanceMotionSequence(&secondaryMotionSequence);
    }
    return 0;
}

void UpdatePlayerRoundState(void)
{
    MinigameRoundState *roundState;
    CharacterActionState *activePlayer;
    s32 playerIndex;

    roundState = &minigameRoundState;
    activePlayer = &characterActionStates[roundState->currentPlayerIndex];
    SetCameraForRoundPhase(0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerIndex != roundState->currentPlayerIndex) {
            CharacterActionState *otherPlayer = &characterActionStates[playerIndex];

            ResetCharacterEffects(&activePlayer->characterEffects);
            AddCharacterEffect(&activePlayer->characterEffects, otherPlayer->characterObject->mdlId[0]);
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerIndex != activePlayer->characterObject->work[0]) {
            CharacterActionState *otherPlayerState = &characterActionStates[playerIndex];

            EnsureModelMotion(otherPlayerState->characterObject, 0, 0, 15, 0);
        }
    }
}

s32 AdvancePlayerRoundState(void)
{
    MinigameRoundState *roundState;
    CharacterActionState *activePlayer;

    roundState = &minigameRoundState;
    activePlayer = &characterActionStates[roundState->currentPlayerIndex];
    if (AdvanceMotionSequenceTimer(&secondaryMotionSequence, 1) != 0) {
        roundState->currentPlayerIndex++;
        if (roundState->currentPlayerIndex >= 4) {
            roundState->currentPlayerIndex = 0;
        }
        return SetMotionSequenceIndex(&secondaryMotionSequence, 0);
    }
    return 0;
}

void StartSpecialCharacterAnimation(void)
{
    CharacterAnimationState *characterState;
    CharacterMotionState *characterEffects;

    characterAnimationState.characterModeFlag = 1;
    characterEffects = &characterMotionStates[1];
    characterEffects->animationIndex = 0;
    characterEffects->animationState = 0;
    characterEffects->animationSubstate = 0;
    minigameResult = 0;
    StartBalloonRotation();
    HuPrcChildCreate(ShowMinigameRoundMessage, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(ShowPlayerScoreMessage, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(ShowRoundSummaryMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 AdvanceActivePlayerAction(void)
{
    CharacterActionState *playerState;
    CharacterAnimationState *specialCharacter;
    s32 playerIndex;

    if (minigameResult == 1) {
        minigameResult = 0;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerState = &characterActionStates[playerIndex];
            SetModelMotion(playerState->characterObject, 0, 12, 1, 0);
        }
    }
    specialCharacter = &characterAnimationState;
    if (specialCharacter->characterObject->mtnId[8] == Hu3DMotionIDGet(specialCharacter->characterObject->mdlId[0]) && Hu3DMotionTimeGet(specialCharacter->characterObject->mdlId[0]) >= 5.0f) {
        Hu3DMotionTimeSet(specialCharacter->characterObject->mdlId[0], 5.0f);
        Hu3DMotionSpeedSet(specialCharacter->characterObject->mdlId[0], 0.0f);
        SetFirstCharacterBodyMotion(0, 30, 1, 0);
        SetSecondCharacterBodyMotion(0, 30, 1, 0);
        SetFirstCharacterAccessoryMotion(5, 10, 1, 1);
    }
    if (minigameResult == 2 && AdvanceMotionSequenceTimer(&secondaryMotionSequence, 1) != 0) {
        return AdvanceMotionSequence(&secondaryMotionSequence);
    }
    return 0;
}

void UpdateRoundResultState(void)
{
    MinigameRoundState *roundState;
    CharacterAnimationState *specialCharacter;
    CharacterActionState *activePlayer;

    roundState = &minigameRoundState;
    specialCharacter = &characterAnimationState;
    activePlayer = &characterActionStates[roundState->currentPlayerIndex];
    SetCharacterMotionForState(0, 60, 1, 1);
    Hu3DMotionSpeedSet(specialCharacter->characterObject->mdlId[0], 1.0f);
    activePlayer->characterObject->rot.y = 0.0f;
    activePlayer->actionTimer = 0;
    SetCameraForRoundPhase(3);
}

s32 AdvanceRoundResultState(void)
{
    MinigameRoundState *roundState;
    CharacterActionState *activePlayer;

    roundState = &minigameRoundState;
    activePlayer = &characterActionStates[roundState->currentPlayerIndex];
    if (activePlayer->actionTimer++ == 0) {
        EnsureModelMotion(activePlayer->characterObject, 0, 10, 30, 1);
    }
    else if (activePlayer->actionTimer > 0) {
        activePlayer->characterObject->rot.y = lerpClamped(0.0f, 180.0f, activePlayer->actionTimer, 30.0f);
        if (activePlayer->actionTimer >= 30) {
            EnsureModelMotion(activePlayer->characterObject, 0, 0, 30, 1);
        }
    }
    if (AdvanceMotionSequenceTimer(&secondaryMotionSequence, 1) != 0) {
        return AdvanceMotionSequence(&secondaryMotionSequence);
    }
    return 0;
}

void PositionPlayersForNextRound(void)
{
    MinigameRoundState *roundState;
    s32 playerIndex;
    s32 positionIndex;

    StartCharacterMotionTimeline();
    roundState = &minigameRoundState;
    playerIndex = roundState->currentPlayerIndex;
    for (positionIndex = 0; positionIndex < 4; positionIndex++) {
        if (positionIndex >= 1) {
            SetPlayerCharacterMotionState(playerIndex, positionIndex);
        }
        playerIndex++;
        if (playerIndex >= 4) {
            playerIndex = 0;
        }
    }
    HuPrcChildCreate(ShowRoundChoiceMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 IsSecondarySequenceComplete(void)
{
    if (AdvanceMotionSequenceTimer(&secondaryMotionSequence, 1) != 0) {
        return 1;
    }
    return 0;
}

void StartSpecialCharacterCelebration(void)
{
    characterAnimationState.characterModeFlag = 1;
    SetCharacterMotionForState(0, 60, 1, 1);
}

s32 AdvanceSecondarySequence(void)
{
    if (AdvanceMotionSequenceTimer(&secondaryMotionSequence, 1) != 0) {
        return AdvanceMotionSequence(&secondaryMotionSequence);
    }
    return 0;
}

void UpdatePlayerScoreState(void)
{
    MinigameRoundState *roundState;
    CharacterActionState *activePlayer;

    roundState = &minigameRoundState;
    activePlayer = &characterActionStates[roundState->currentPlayerIndex];
    HuPrcChildCreate(ShowGameplayStatusMessage, 100, 0x2000, 0, HuPrcCurrentGet());
    activePlayer->characterObject->rot.y = 0.0f;
    activePlayer->actionTimer = 0;
    SetFirstCharacterBodyMotion(0, 10, 1, 0);
    SetSecondCharacterBodyMotion(0, 10, 1, 0);
    SetFirstCharacterAccessoryMotion(5, 10, 1, 1);
    SetCameraForRoundPhase(3);
}

s32 AdvancePlayerScoreState(void)
{
    MinigameRoundState *roundState;
    CharacterActionState *activePlayer;

    roundState = &minigameRoundState;
    activePlayer = &characterActionStates[roundState->currentPlayerIndex];
    if (activePlayer->actionTimer++ == 60) {
        EnsureModelMotion(activePlayer->characterObject, 0, 10, 30, 1);
    }
    else if (activePlayer->actionTimer > 60) {
        activePlayer->characterObject->rot.y = lerpClamped(0.0f, 180.0f, activePlayer->actionTimer - 60, 30.0f);
        if (activePlayer->actionTimer >= 90) {
            EnsureModelMotion(activePlayer->characterObject, 0, 0, 30, 1);
        }
    }
    if (minigameResult != 0 && (AdvanceMotionSequenceTimer(&secondaryMotionSequence, 1) != 0)) {
        return AdvanceMotionSequence(&secondaryMotionSequence);
    }
    return 0;
}

void PositionPlayersForFinalScene(void)
{
    MinigameRoundState *roundState;
    s32 playerIndex;
    s32 positionIndex;

    StartCharacterMotionTimeline();
    roundState = &minigameRoundState;
    playerIndex = roundState->currentPlayerIndex;
    for (positionIndex = 0; positionIndex < 4; positionIndex++) {
        if (positionIndex >= 1) {
            SetPlayerCharacterMotionState(playerIndex, positionIndex);
        }
        playerIndex++;
        if (playerIndex >= 4) {
            playerIndex = 0;
        }
    }
}

s32 IsFinalMotionComplete(void)
{
    if (AdvanceMotionSequenceTimer(&secondaryMotionSequence, 1) != 0) {
        return 1;
    }
    return 0;
}

MotionSequenceEntry secondaryMotionSequenceEntries[] = { { UpdatePlayerTurnOrder, InitializePlayerRoundPositions, 60 }, { StartMinigameRound, AdvanceMinigameRound, 60 }, { SetSecondarySequenceDuration, AdvanceSecondaryMotionSequence, 60 },
    { ShowRoundPhaseDialogue, AdvanceRoundPhaseSequence, 10 }, { UpdatePlayerRoundState, AdvancePlayerRoundState, 30 }, { StartSpecialCharacterAnimation, AdvanceActivePlayerAction, 60 }, { UpdateRoundResultState, AdvanceRoundResultState, 60 },
    { PositionPlayersForNextRound, IsSecondarySequenceComplete, 90 }, { StartSpecialCharacterCelebration, AdvanceSecondarySequence, 60 }, { UpdatePlayerScoreState, AdvancePlayerScoreState, 60 }, { PositionPlayersForFinalScene, IsFinalMotionComplete, 90 }, { NULL, NULL, -1 } };

void InitializeSecondaryMotionSequence(void)
{
    InitializeMotionSequence(&secondaryMotionSequence, secondaryMotionSequenceEntries);
}

s32 RunSecondaryMotionCallbacks(void)
{
    if (secondaryMotionSequence.motionCallback != NULL) {
        secondaryMotionSequence.motionCallback();
        secondaryMotionSequence.motionCallback = NULL;
    }
    if (secondaryMotionSequence.completionCondition != NULL) {
        return secondaryMotionSequence.completionCondition();
    }
    return 0;
}

void UpdateFinalPlayerState(void)
{
    MinigameRoundState *roundState;
    CharacterActionState *activePlayer;

    roundState = &minigameRoundState;
    activePlayer = &characterActionStates[roundState->currentPlayerIndex];
    roundState->roundEventState = 0;
    trackedCharacterModelId = activePlayer->characterObject->mdlId[0];
    trackedCharacterMotionId = activePlayer->characterObject->mtnId[3];
    trackedCharacterIndex = activePlayer->characterId;
    HuPrcChildCreate(MonitorCharacterMotionSoundCues, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 AdvanceFinalPlayerState(void)
{
    MinigameRoundState *roundState;
    CharacterActionState *activePlayer;

    roundState = &minigameRoundState;
    activePlayer = &characterActionStates[roundState->currentPlayerIndex];
    if (AdvanceMotionSequenceTimer(&tertiaryMotionSequence, 0) == 120) {
        SetModelMotion(activePlayer->characterObject, 0, 3, 30, 0);
        HuAudSStreamPlay(3);
    }
    if (AdvanceMotionSequenceTimer(&tertiaryMotionSequence, 1) != 0) {
        return AdvanceMotionSequence(&tertiaryMotionSequence);
    }
    return 0;
}

MotionSequenceEntry tertiaryMotionSequenceEntries[] = { { UpdateFinalPlayerState, AdvanceFinalPlayerState, 360 }, { NULL, NULL, -1 } };

void InitializeTertiaryMotionSequence(void)
{
    InitializeMotionSequence(&tertiaryMotionSequence, tertiaryMotionSequenceEntries);
}

s32 RunTertiaryMotionCallbacks(void)
{
    if (tertiaryMotionSequence.motionCallback != NULL) {
        tertiaryMotionSequence.motionCallback();
        tertiaryMotionSequence.motionCallback = NULL;
    }
    if (tertiaryMotionSequence.completionCondition != NULL) {
        return tertiaryMotionSequence.completionCondition();
    }
    return 0;
}

void UpdatePlayerResultObject(OMOBJ *object)
{
    MinigameRoundState *roundState;
    CharacterActionState *activePlayer;
    s32 playerIndex;

    if (WipeStatGet() == 0) {
        HuAudFadeOut(1);
        CharModelKill(-1);
        MGSeqKillAll();
        roundState = &minigameRoundState;
        activePlayer = &characterActionStates[roundState->currentPlayerIndex];
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            GWPlayerCoinWinSet(playerIndex, 0);
        }
        GWPlayerCoinWinSet(activePlayer->playerIndex, 1);
        omOvlReturnEx(1, 1);
        object->objFunc = NULL;
    }
}

s32 IsExitRequested(void)
{
    return omSysExitReq;
}

void FinalizeMinigameResults(void)
{
    MinigameRoundState *roundState;
    s32 playerIndex;

    InitializeSceneModels(&sceneDialogueState);
    InitializeBalloonObject(&characterSceneState);
    UpdateCharacterAnimationState(&characterAnimationState);
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        InitializeCharacterMotionState(&characterMotionStates[playerIndex]);
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        UpdatePlayerActionState(&characterActionStates[playerIndex]);
    }
    InitializeCharacterMotionTimeline(&characterMotionTimeline);
    roundState = &minigameRoundState;
    roundState->roundPhase = roundState->roundPhaseStep = roundState->roundEvent = roundState->roundFrameCount = 0;
    roundState->currentPlayerIndex = roundState->roundState = roundState->roundSubstate = roundState->roundEventState = 0;
    InitializePrimaryMotionSequence();
    InitializeSecondaryMotionSequence();
    InitializeTertiaryMotionSequence();
    TransitionCameraToPreset(0);
    HuAudFXPlay(0x6D7);
}

void UpdateResultSceneDialogue(OMOBJ *object)
{
    SceneDialogueState *sceneState;

    switch (activePlayerIndex) {
        case 0:
            FinalizeMinigameResults();
            activePlayerIndex++;
            break;
        case 1:
            if (WipeStatGet() == 0) {
                activePlayerIndex++;
            }
            break;
        case 2:
            if (RunPrimaryMotionCallbacks() != 0) {
                activePlayerIndex++;
            }
            break;
        case 3:
            activeCharacterIndex = MGSeqStartCreate();
            activePlayerIndex++;
            sequenceHandles.resultSequenceId = -1;
            break;
        case 4:
            if ((MGSeqStatGet(activeCharacterIndex) & 0x10) && sequenceHandles.resultSequenceId == -1) {
                sequenceHandles.resultSequenceId = HuAudSeqPlay(0x4D);
            }
            if (MGSeqStatGet(activeCharacterIndex) == 0) {
                activePlayerIndex++;
            }
            break;
        case 5:
            if (RunSecondaryMotionCallbacks() != 0) {
                activePlayerIndex++;
            }
            break;
        case 6:
            activeCharacterIndex = MGSeqFinishCreate();
            activePlayerIndex++;
            HuAudSeqPauseAll(0);
            HuAudSeqAllFadeOut(0xFA);
            break;
        case 7:
            if (MGSeqStatGet(activeCharacterIndex) == 0) {
                activePlayerIndex++;
            }
            break;
        case 8:
            sceneState = &sceneDialogueState;
            sceneState->sceneState = 999;
            if (RunTertiaryMotionCallbacks() != 0) {
                activePlayerIndex++;
            }
            break;
        case 9:
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
            object->objFunc = UpdatePlayerResultObject;
            break;
        case 10:
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
            object->objFunc = UpdatePlayerResultObject;
            break;
    }
    if (omSysExitReq != 0) {
        HuAudSeqAllFadeOut(0xFA);
        activePlayerIndex = 10;
    }
}

void InitializeMinigameScene(OMOBJ *object)
{
    s32 i;

    sceneDialogueState.mainObject = omAddObjEx(minigameProcess, 0x100, 16, 16, -1, LoadSceneModels);
    characterSceneState.characterObject = omAddObjEx(minigameProcess, 0x200, 16, 16, -1, LoadCharacterAnimationModels);
    characterAnimationState.characterObject = omAddObjEx(minigameProcess, 0x300, 16, 16, -1, LoadSecondaryCharacterModel);
    for (i = 0; i < 2; i++) {
        characterMotionStates[i].characterObject = omAddObjEx(minigameProcess, 0x400, 16, 16, -1, UpdateCharacterMotionObject);
        characterMotionStates[i].characterObject->work[0] = i;
    }
    for (i = 0; i < 4; i++) {
        characterActionStates[i].characterObject = omAddObjEx(minigameProcess, 0x500, 16, 16, -1, UpdatePlayerActionObjectState);
        characterActionStates[i].characterObject->work[0] = i;
    }
    characterMotionTimeline.characterObject = omAddObjEx(minigameProcess, 0x600, 16, 16, -1, UpdateCharacterMotionMarkers);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    object->objFunc = UpdateResultSceneDialogue;
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

void StartMinigame(void)
{
    minigameProcess = omInitObjMan(62, 0x2000);
    omGameSysInit(minigameProcess);
    CreateSceneLight();
    InitializeSecondaryCamera();
    InitializeSceneShadows();
    CreateCountdownObject();
    InitializeMessageWindows();
    SortPlayersByTurnOrder();
    omAddObjEx(minigameProcess, 0x1000, 0, 0, -1, InitializeMinigameScene);
}

void ObjectSetup(void)
{
    StartMinigame();
}
