#include "REL/m436Dll.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/process.h"
#include "game/window.h"
#include "game/wipe.h"

#include "ext_math.h"
#include "version.h"
#include <string.h>

s32 rand8(void);
#ifndef __MWERKS__
#include "game/audio.h"
#endif

typedef struct {
    /* 0x00 */ OMOBJ *characterObject;
    /* 0x04 */ s32 playerIndex;
    /* 0x08 */ s32 playerGroup;
    /* 0x0C */ s32 isCpu;
    /* 0x10 */ s32 aiDifficulty;
    /* 0x14 */ s32 characterId;
    /* 0x18 */ s32 controllerIndex;
    /* 0x1C */ s32 playerActionTarget;
    /* 0x20 */ s32 playerActionState;
    /* 0x24 */ s32 playerActionTimer;
    /* 0x28 */ s32 playerTargetSlot;
    /* 0x2C */ float materialOpacity;
} PlayerCharacterState; // Size 0x30

typedef struct {
    /* 0x00 */ s32 currentPlayerIndex;
    /* 0x04 */ s32 roundState;
    /* 0x08 */ s32 roundSubstate;
    /* 0x0C */ s32 roundEventState;
} RoundControlState; // Size 0x10

typedef struct {
    /* 0x00 */ s32 windowCount;
    /* 0x04 */ float windowScrollSpeed;
    /* 0x08 */ s32 windowIds[15];
    /* 0x44 */ s32 messageIndices[15];
    /* 0x80 */ float windowPositions[15][2];
} WindowAnimationState; // Size 0xF8

typedef struct {
    /* 0x00 */ OMOBJ *mainObject;
    /* 0x04 */ s32 dialogueState;
    /* 0x08 */ char unreferencedDialoguePadding[0xC];
} DialogueObjectState; // Size 0x14

typedef struct {
    /* 0x00 */ OMOBJ *mapObject;
    /* 0x04 */ Vec objectPositions[8];
} MapObjectState; // Size 0x64

typedef struct {
    /* 0x00 */ OMOBJ *audioObject;
    /* 0x04 */ s32 audioState;
    /* 0x08 */ float playerVolume[6];
    /* 0x20 */ char unreferencedAudioPadding20[8];
    /* 0x28 */ float effectVolume[6];
    /* 0x40 */ char unreferencedAudioPadding40[8];
    /* 0x48 */ float ambientVolume[6];
    /* 0x60 */ char unreferencedAudioPadding60[8];
    /* 0x68 */ float sequenceVolume[6];
    /* 0x80 */ char unreferencedAudioPadding80[8];
    /* 0x88 */ float musicVolume[6];
    /* 0xA0 */ char unreferencedAudioPaddingA0[8];
} AudioVolumeState; // Size 0xA8

typedef struct {
    /* 0x00 */ OMOBJ *characterObject;
    /* 0x04 */ char unreferencedCharacterState[0x10];
} HostCharacterState; // Size 0x14

typedef struct {
    /* 0x00 */ OMOBJ *characterObject;
    /* 0x04 */ char unreferencedCharacterState[0x10];
} HelperCharacterState; // Size 0x14

typedef struct {
    /* 0x00 */ char unreferencedIntroState[0x28];
    /* 0x28 */ s32 openingSoundEffectId;
    /* 0x2C */ char unreferencedIntroPadding[0xC];
} IntroSequenceState; // Size 0x38

s32 GetPlayerIndex(s32 playerIndex);

RoundControlState roundControlState;
PlayerCharacterState playerCharacterStates[4];
IntroSequenceState introSequenceState;
s32 audioSequenceIds[2];
CameraState globalCameraState;
WindowAnimationState windowAnimationState;
DialogueObjectState dialogueObjectState;
MapObjectState mapObjectState;
HostCharacterState hostCharacterState;
HelperCharacterState helperCharacterStates[2];
AudioVolumeState audioVolumeState;
SequencePlayer sceneSequenceState;
SequencePlayer openingSequenceState;
Mtx targetTransformMatrix;
Mtx sourceTransformMatrix;
Vec interpolatedPosition;
SequencePlayer roundSequenceState;
s32 unusedBssPadding; // unused
s32 minigameSequenceId;
s32 roundPhase;
s32 frameCounter;
s32 activePlayerIndex;
s32 selectedCharacterId;
s32 selectedMotionId;
s32 selectedModelId;
HUPROCESS *objectManager;

s32 characterResourceIds[]
    = { DATA_MAKE_NUM(DATADIR_M436, 13), DATA_MAKE_NUM(DATADIR_M436, 14), DATA_MAKE_NUM(DATADIR_M436, 15), DATA_MAKE_NUM(DATADIR_M436, 16),
          DATA_MAKE_NUM(DATADIR_M436, 17), DATA_MAKE_NUM(DATADIR_M436, 18), DATA_MAKE_NUM(DATADIR_M436, 19), DATA_MAKE_NUM(DATADIR_M436, 20) };

s32 characterMessageIds[] = { MAKE_MESSID(29, 23), MAKE_MESSID(29, 24), MAKE_MESSID(29, 30), MAKE_MESSID(29, 26), MAKE_MESSID(29, 28), MAKE_MESSID(29, 27),
    MAKE_MESSID(29, 25), MAKE_MESSID(29, 29) };

s32 characterSoundCueFrames[][5] = { { 91, 105, 126, 141, -1 }, { 91, 105, 126, 141, -1 }, { 124, 137, -1, -1, -1 }, { 85, 100, 127, -1, -1 },
    { 124, 139, -1, -1, -1 }, { 85, 97, 127, 139, -1 }, { 127, 137, -1, -1, -1 }, { 79, 95, 109, 127, -1 } };

s32 activeModelId = -1;
s32 activeMotionId = -1;
s32 activeCharacterId = -1;

void PlayCharacterStepSounds(void)
{
    s32 i;

    while (TRUE) {
        HuPrcVSleep();
        if (activeModelId == -1 || activeMotionId == -1 || activeCharacterId == -1 || activeMotionId != Hu3DMotionIDGet(activeModelId)) {
            continue;
        }
        for (i = 0; i < 5; i++) {
            if (characterSoundCueFrames[activeCharacterId][i] != -1 && Hu3DMotionTimeGet(activeModelId) >= characterSoundCueFrames[activeCharacterId][i]) {
                OSReport("KE-System  CHR:%d TIME:%d\n", activeCharacterId, characterSoundCueFrames[activeCharacterId][i]);
                HuAudFXPlay(0x6D8);
                characterSoundCueFrames[activeCharacterId][i] = -1;
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

void UpdateCameraDebugControls(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    if (HuPadBtn[0] & 0x200) {
        if (HuPadBtn[0] & 0x100) {
            cameraState->centerZ -= HuPadStkY[0] / 10.0f;
        }
        else {
            cameraState->centerX += HuPadStkX[0] / 10.0f;
            cameraState->centerY += HuPadStkY[0] / 10.0f;
        }
    }
    if (HuPadBtn[0] & 0x800) {
        cameraState->rotationX -= HuPadStkY[0] / 100.0f;
        cameraState->rotationY += HuPadStkX[0] / 100.0f;
    }
    if (HuPadBtn[0] & 0x400) {
        cameraState->zoom -= HuPadStkY[0] / 10.0f;
    }
    if (cameraState->rotationX >= 360.0f) {
        cameraState->rotationX -= 360.0f;
    }
    if (cameraState->rotationX < 0.0f) {
        cameraState->rotationX += 360.0f;
    }
    if (cameraState->rotationY >= 360.0f) {
        cameraState->rotationY -= 360.0f;
    }
    if (cameraState->rotationY < 0.0f) {
        cameraState->rotationY += 360.0f;
    }
    print8(16, 100, 1.0f, ">>>>>>>>>> CAMERA TEST <<<<<<<<<<");
    print8(16, 110, 1.0f, "CENTER : %.2f, %.2f, %.2f", cameraState->centerX, cameraState->centerY, cameraState->centerZ);
    print8(16, 120, 1.0f, "ROT    : %.2f, %.2f, %.2f", cameraState->rotationX, cameraState->rotationY, cameraState->rotationZ);
    print8(16, 130, 1.0f, "ZOOM   : %.2f", cameraState->zoom);
}

void SetCameraInterpolation(s32 cameraIndex, s32 cameraMode, float elapsedFrames)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->interpolationFrames[cameraIndex] = cameraMode;
    if (cameraState->interpolationDeltas[callback] < 0.0f) {
        cameraState->interpolationDeltas[callback] = -elapsedFrames;
    }
    else {
        cameraState->interpolationDeltas[cameraIndex] = elapsedFrames;
    }
}

void AccelerateCameraInterpolation(s32 cameraIndex, float endValue)
{
    SetCameraInterpolation(cameraIndex, -1, endValue);
}

float UpdateCameraInterpolation(s32 cameraIndex)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    if (cameraState->interpolationDeltas[callback] != 0.0f) {
        cameraState->interpolationDeltas[callback] *= -0.9f;
        if (cameraState->interpolationFrames[callback] == -1) {
            if (cameraState->interpolationDeltas[callback] >= -0.5f && cameraState->interpolationDeltas[callback] <= 0.5f) {
                cameraState->interpolationDeltas[callback] = 0.0f;
            }
        }
        else {
            cameraState->interpolationFrames[callback]--;
            if (cameraState->interpolationFrames[callback] <= 0) {
                cameraState->interpolationFrames[callback] = 0;
                cameraState->interpolationDeltas[callback] = 0.0f;
            }
        }
    }
    return cameraState->interpolationDeltas[cameraIndex];
}

void InitializeCameraPosition(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->centerX = 0.0f;
    cameraState->centerY = 225.0f;
    cameraState->centerZ = 1740.0f;
    cameraState->rotationX = -10.0f;
    cameraState->rotationY = 0.0f;
    cameraState->rotationZ = 0.0f;
    cameraState->zoom = 300.0f;
}

void SetCameraUpdateCallback(void (*cameraUpdateCallback)(void))
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->updateCallback = cameraUpdateCallback;
}

void UpdateCameraTransform(OMOBJ *sceneObject)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    if (cameraState->updateCallback != NULL) {
        cameraState->updateCallback();
    }
    Center.x = cameraState->centerX + UpdateCameraInterpolation(0);
    Center.y = cameraState->centerY + UpdateCameraInterpolation(1);
    Center.z = cameraState->centerZ + UpdateCameraInterpolation(2);
    CRot.x = cameraState->rotationX + UpdateCameraInterpolation(3);
    CRot.y = cameraState->rotationY + UpdateCameraInterpolation(4);
    CRot.z = cameraState->rotationZ + UpdateCameraInterpolation(5);
    CZoom = cameraState->zoom + UpdateCameraInterpolation(6);
}

void InitializeSceneCamera(void)
{
    Hu3DCameraCreate(1);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 45.0f, 0.1f, 10000.0f, 1.2f);
    SetCameraUpdateCallback(InitializeCameraPosition);
    omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutView);
    omAddObjEx(objectManager, 0x7FD9, 0, 0, -1, UpdateCameraTransform);
}

void InitializeSceneShadows(void)
{
    Vec shadowCameraPosition = { 0.0f, 2500.0f, 1.0f };
    Vec shadowLookAtTarget = { 0.0f, 0.0f, 0.0f };
    Vec shadowUpDirection = { 0.0f, 1.0f, 0.0f };

    Hu3DShadowCreate(45.0f, 2000.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.8f);
    Hu3DShadowPosSet(&shadowCameraPosition, &shadowUpDirection, &shadowLookAtTarget);
}

void AnimateWindowOpen(s32 windowId, float verticalScale, float targetScale, s32 windowMode)
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
    windowLeft = window->pos_x;
    windowTop = window->pos_y;
    windowWidth = window->w;
    windowHeight = window->h;
    targetLeft = windowLeft + windowWidth * verticalScale;
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
    targetTop = windowTop + windowHeight * targetScale;
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
    if (callback != 0) {
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
        if (callback != 0) {
            if (animationFrame <= 10) {
                currentLeft = sineEaseInterpolate(targetLeft, windowLeft, animationFrame, 10.0f);
                currentXScale = sineEaseInterpolate(collapsedXScale, 1.0f, animationFrame, 10.0f);
            }
            else {
                currentTop = sineEaseInterpolate(targetTop, windowTop, animationFrame - 10, 5.0f);
                currentYScale = sineEaseInterpolate(collapsedYScale, 1.0f, animationFrame - 10, 5.0f);
            }
        }
        else {
            if (animationFrame <= 10) {
                currentTop = sineEaseInterpolate(windowTop, targetTop, animationFrame, 10.0f);
                currentYScale = sineEaseInterpolate(1.0f, collapsedYScale, animationFrame, 10.0f);
            }
            else {
                currentLeft = sineEaseInterpolate(windowLeft, targetLeft, animationFrame - 10, 5.0f);
                currentXScale = sineEaseInterpolate(1.0f, collapsedXScale, animationFrame - 10, 5.0f);
            }
        }
        HuWinPosSet(windowId, currentLeft, currentTop);
        HuWinScaleSet(windowId, currentXScale, currentYScale);
    }
    if (callback != 0) {
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

void AnimateWindowClose(s32 windowId, float verticalScale, float targetScale)
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
    windowLeft = window->pos_x;
    windowTop = window->pos_y;
    windowWidth = window->w;
    windowHeight = window->h;
    targetLeft = windowLeft + windowWidth * verticalScale;
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
    targetTop = windowTop + windowHeight * targetScale;
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
    HuWinPosSet(windowId, targetLeft, targetTop);
    HuWinScaleSet(windowId, collapsedXScale, collapsedYScale);
    HuPrcVSleep();
    for (animationFrame = 0; animationFrame < 10; animationFrame++) {
        currentLeft = sineEaseInterpolate(targetLeft, windowLeft, animationFrame, 10.0f);
        currentXScale = sineEaseInterpolate(collapsedXScale, 1.0f, animationFrame, 10.0f);
        currentTop = sineEaseInterpolate(targetTop, windowTop, animationFrame, 10.0f);
        currentYScale = sineEaseInterpolate(collapsedYScale, 1.0f, animationFrame, 10.0f);
        HuWinPosSet(windowId, currentLeft, currentTop);
        HuWinScaleSet(windowId, currentXScale, currentYScale);
        HuPrcVSleep();
    }
    HuWinPosSet(windowId, windowLeft, windowTop);
    HuWinScaleSet(windowId, 1.0f, 1.0f);
    HuPrcVSleep();
}

void AnimateWindowResize(s32 windowId, float verticalScale, float targetScale)
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
    windowLeft = window->pos_x;
    windowTop = window->pos_y;
    windowWidth = window->w;
    windowHeight = window->h;
    targetLeft = windowLeft + windowWidth * verticalScale;
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
    targetTop = windowTop + windowHeight * targetScale;
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
        currentLeft = cosineEaseInterpolate(windowLeft, targetLeft, animationFrame, 10.0f);
        currentXScale = cosineEaseInterpolate(1.0f, collapsedXScale, animationFrame, 10.0f);
        currentTop = cosineEaseInterpolate(windowTop, targetTop, animationFrame, 10.0f);
        currentYScale = cosineEaseInterpolate(1.0f, collapsedYScale, animationFrame, 10.0f);
        HuWinPosSet(windowId, currentLeft, currentTop);
        HuWinScaleSet(windowId, currentXScale, currentYScale);
        HuPrcVSleep();
    }
    HuWinPosSet(windowId, targetLeft, targetTop);
    HuWinScaleSet(windowId, collapsedXScale, collapsedYScale);
    HuPrcVSleep();
}

s32 CreateMessageWindow(s16 windowId, s16 messageId, s16 waitFrames, s16 windowMode, s32 option)
{
    s32 backgroundColorOptions[][5] = { { 0xFF, 0xFF, 0xCC, 0x00, 0x09 }, { 0xCC, 0xFF, 0xFF, 0x00, 0x09 }, { 0xFF, 0xCC, 0xFF, 0x00, 0x09 } };
    float backgroundOpacityOptions[] = { 0.9f, 0.9f, 0.9f };
    GXColor backgroundColor;
    s32 windowId;

    windowId = HuWinCreate(windowId, messageId, waitFrames, windowMode, 1);
    backgroundColor.r = backgroundColorOptions[option][0];
    backgroundColor.g = backgroundColorOptions[option][1];
    backgroundColor.b = backgroundColorOptions[option][2];
    HuWinBGColSet(windowId, &backgroundColor);
    HuWinBGTPLvlSet(windowId, backgroundOpacityOptions[option]);
    AnimateWindowOpen(windowId, 0.5f, 0.5f, 1);
    return windowId;
}

s32 CreateMessageWindowVariant(s16 windowId, s16 messageId, s16 waitFrames, s16 windowMode, s32 option)
{
    return CreateMessageWindow(windowId, messageId, waitFrames, windowMode, option);
}

void ShowCharacterNameMessage(s32 windowId)
{
    HuWinMesSet(windowId, MAKE_MESSID(29, 22));
    HuPrcVSleep();
    AnimateWindowOpen(windowId, 0.5f, 0.5f, 0);
    HuWinKill(windowId);
}

void ClearCharacterNameMessage(s32 windowId)
{
    ShowCharacterNameMessage(windowId);
}

void WaitForPlayerMessageInput(s32 windowId, s32 messageId)
{
    s32 playerIndex;
    s32 anyHumanPlayer;
    s32 messageDelay;

    anyHumanPlayer = 0;
    messageDelay = GWMessDelayGet();
    if (callback > 0) {
        HuPrcSleep(callback);
        return;
    }
    HuWinComKeyReset();
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerCharacterStates[playerIndex].isCpu == 0) {
            anyHumanPlayer = 1;
            winData[windowId].active_pad = 0xF;
        }
    }
    if (anyHumanPlayer == 0) {
        winData[windowId].active_pad = 0xF;
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, messageDelay);
    }
    HuWinMesWait(windowId);
    HuWinComKeyReset();
}

void WaitForAnyPlayerMessageInput(s32 windowId)
{
    s32 choice;
    s32 messageDelay;
    s32 playerIndex;

    choice = 0;
    messageDelay = GWMessDelayGet();
    HuWinComKeyReset();
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerCharacterStates[playerIndex].isCpu == 0) {
            choice = 1;
            winData[windowId].active_pad = 0xF;
        }
    }
    if (choice == 0) {
        winData[windowId].active_pad = 0xF;
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, messageDelay);
    }
    choice = HuWinChoiceGet(windowId, 1);
    if (choice == -1 || choice == 1) {
        selectedMotionId = 0;
    }
    else {
        selectedMotionId = 1;
    }
    HuWinComKeyReset();
}

void MeasureMessageDimensions(float *messageDimensions, s32 messageId)
{
    float messageSize[2];

    HuWinMesMaxSizeGet(1, messageSize, messageId);
    if (messageSize[0] > callback[0]) {
        callback[0] = messageSize[0];
    }
    if (messageSize[1] > callback[1]) {
        callback[1] = messageSize[1];
    }
}

void SetWindowMessage(s32 windowId, s32 messageId)
{
    winData[windowId].mess_color = 0;
    winData[windowId].mess_shadow_color = 9;
    winData[windowId].attr |= 0x80;
    HuWinMesSet(windowId, messageId);
}

void SelectFirstPlayer(s32 playerIndex)
{
    if (activePlayerIndex == 0) {
        activePlayerIndex = 1;
        selectedCharacterId = playerIndex * REFRESH_RATE;
    }
}

s32 IsLastPlayerSelected(void)
{
    if (activePlayerIndex == 3) {
        return 1;
    }
    else {
        return 0;
    }
}

void SelectFinalPlayer(void)
{
    if (activePlayerIndex == 2) {
        activePlayerIndex = 3;
        selectedCharacterId = 30;
    }
}

s32 activeSequenceId = -1;

void UpdatePlayerSelectionTimer(OMOBJ *timerObject)
{
    switch (activePlayerIndex) {
        case 1:
            if (activeSequenceId == -1) {
                activeSequenceId = MGSeqCreate(1, selectedCharacterId / REFRESH_RATE, -1, -1);
            }
            activePlayerIndex = 2;
            break;
        case 2:
            selectedCharacterId--;
            if (activeSequenceId != -1) {
                MGSeqParamSet(activeSequenceId, 1, (selectedCharacterId + REFRESH_RATE - 1) / REFRESH_RATE);
            }
            if (selectedCharacterId <= 0) {
                selectedCharacterId = 30;
                activePlayerIndex = 3;
            }
            break;
        case 3:
            if (activeSequenceId != -1) {
                MGSeqParamSet(activeSequenceId, 2, -1);
                activeSequenceId = -1;
            }
            selectedCharacterId = 0;
            activePlayerIndex = 0;
            break;
    }
}

void InitializePlayerSelection(void)
{
    selectedCharacterId = 0;
    activePlayerIndex = 0;
    omAddObjEx(objectManager, 0x2000, 0, 0, -1, UpdatePlayerSelectionTimer);
}

void CreateScrollingMessageWindow(s32 windowId)
{
    float messageDimensions[2];
    float baseLeft;
    float baseTop;
    float baseHeight;
    float baseWidth;
    WindowData *baseWindow;
    WindowAnimationState *scrollingText;
    s32 windowIndex;

    baseWindow = &winData[windowId];
    scrollingText = &windowAnimationState;
    baseLeft = baseWindow->pos_x;
    baseTop = baseWindow->pos_y;
    baseHeight = baseWindow->h;
    baseWidth = baseWindow->w;
    for (windowIndex = 0; windowIndex < scrollingText->windowCount; windowIndex++) {
        HuWinMesMaxSizeGet(1, messageDimensions, characterMessageIds[scrollingText->messageIndices[windowIndex]]);
        scrollingText->windowIds[windowIndex]
            = HuWinCreate(baseLeft + baseWidth + 16.0f + scrollingText->windowPositions[windowIndex][0], baseTop + 26.0f + scrollingText->windowPositions[windowIndex][1], messageDimensions[0], messageDimensions[1], 1);
        winData[scrollingText->windowIds[windowIndex]].mess_shadow_color = 9;
        HuWinMesColSet(scrollingText->windowIds[windowIndex], 0);
        HuWinScissorSet(scrollingText->windowIds[windowIndex], baseLeft + 20.0f, baseTop, baseWidth + 20.0f, baseHeight);
        HuWinBGTPLvlSet(scrollingText->windowIds[windowIndex], 0.0f);
        HuPrcVSleep();
        SetWindowMessage(scrollingText->windowIds[windowIndex], characterMessageIds[scrollingText->messageIndices[windowIndex]]);
    }
    scrollingText->windowScrollSpeed = 4.0f;
    HuPrcVSleep();
}

void UpdateScrollingMessageWindow(s32 windowId)
{
    float messageDimensions[2];
    float baseLeft;
    float baseTop;
    float baseHeight;
    float baseWidth;
    WindowData *baseWindow;
    WindowAnimationState *scrollingText;
    s32 windowIndex;

    baseWindow = &winData[windowId];
    scrollingText = &windowAnimationState;
    baseLeft = baseWindow->pos_x;
    baseTop = baseWindow->pos_y;
    baseHeight = baseWindow->h;
    baseWidth = baseWindow->w;
    while (TRUE) {
        for (windowIndex = 0; windowIndex < scrollingText->windowCount; windowIndex++) {
            scrollingText->windowPositions[windowIndex][0] -= scrollingText->windowScrollSpeed;
            HuWinPosSet(scrollingText->windowIds[windowIndex], baseLeft + baseWidth + 16.0f + scrollingText->windowPositions[windowIndex][0], 26.0f + baseTop + scrollingText->windowPositions[windowIndex][1]);
        }
        if (scrollingText->windowCount >= 2) {
            HuWinMesMaxSizeGet(1, messageDimensions, characterMessageIds[scrollingText->messageIndices[2]]);
            if (baseLeft + baseWidth + 16.0f + scrollingText->windowPositions[2][0] + messageDimensions[0] <= baseLeft + baseWidth - 16.0f) {
                scrollingText->windowScrollSpeed += 0.75f;
            }
        }
        HuWinMesMaxSizeGet(1, messageDimensions, characterMessageIds[scrollingText->messageIndices[scrollingText->windowCount - 1]]);
        if (baseLeft + baseWidth + 16.0f + scrollingText->windowPositions[scrollingText->windowCount - 1][0] + messageDimensions[0] <= baseLeft - 16.0f) {
            break;
        }
        HuPrcVSleep();
    }
    HuPrcVSleep();
}

void DestroyScrollingMessageWindows(void)
{
    s32 windowIndex;
    WindowAnimationState *scrollingText;

    scrollingText = &windowAnimationState;
    for (windowIndex = 0; windowIndex < scrollingText->windowCount; windowIndex++) {
        HuWinKill(scrollingText->windowIds[windowIndex]);
        HuPrcVSleep();
    }
    HuPrcVSleep();
}

void InitializeScrollingMessages(void)
{
    float messageDimensions[2];
    WindowAnimationState *scrollingText;
    s32 windowIndex;

    scrollingText = &windowAnimationState;
    scrollingText->windowCount = rand8() % 2 + 10;
    if (scrollingText->windowCount >= 15) {
        scrollingText->windowCount = 15;
    }
    for (windowIndex = 0; windowIndex < 15; windowIndex++) {
        scrollingText->messageIndices[windowIndex] = -1;
    }
    for (windowIndex = 0; windowIndex < scrollingText->windowCount; windowIndex++) {
        scrollingText->messageIndices[windowIndex] = rand8() % 8;
    }
    scrollingText->windowPositions[0][0] = 0.0f;
    scrollingText->windowPositions[0][1] = 0.0f;
    for (windowIndex = 1; windowIndex < scrollingText->windowCount; windowIndex++) {
        HuWinMesMaxSizeGet(1, messageDimensions, characterMessageIds[scrollingText->messageIndices[windowIndex - 1]]);
        scrollingText->windowPositions[windowIndex][0] = scrollingText->windowPositions[windowIndex - 1][0] + messageDimensions[0] + 8.0f;
        scrollingText->windowPositions[windowIndex][1] = scrollingText->windowPositions[0][1];
    }
}

void UpdateFloatingSceneModel(OMOBJ *sceneObject)
{
    DialogueObjectState *sceneState;
    HU3DMODEL *floatingModel;
    s32 modelIndex;

    sceneState = &dialogueObjectState;
    floatingModel = &Hu3DData[sceneObject->mdlId[8]];
    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
        Hu3DData[sceneObject->mdlId[modelIndex + 5]].rot.y += 360.0f / (2500.0f + 2500.0f * modelIndex);
    }
    if (callback->work[0] == 0 && callback->work[2]++ >= 600) {
        callback->work[2] = 0;
        if (rand8() % 2 == 0) {
            callback->work[0] = 1;
        }
    }
    switch (callback->work[0]) {
        case 10:
            sceneState->dialogueState = 11;
            SetCharacterMotion(callback, 10, 12, 1, 0);
            Hu3DModelAttrReset(callback->mdlId[10], HU3D_ATTR_DISPOFF);
            SetCameraInterpolation(1, 10, 4.0f);
            HuAudFXPlay(0x6CF);
            /* fallthrough */
        case 1:
        case 11:
            SetCharacterMotion(callback, 0, 0, 1, 0);
            SetCharacterMotion(callback, 3, 3, 1, 0);
            SetCharacterMotion(callback, 4, 4, 1, 0);
            SetCharacterMotion(callback, 5, 5, 1, 0);
            SetCharacterMotion(callback, 6, 6, 1, 0);
            SetCharacterMotion(callback, 7, 7, 1, 0);
            if (callback->work[0] == 1) {
                HuAudFXPlay(0x6CE);
            }
            callback->work[0] = 99;
            break;
        case 2:
            callback->work[0] = 3;
            SetCharacterMotion(callback, 0, 0, 1, 0);
            SetCharacterMotion(callback, 1, 1, 1, 0);
            SetCharacterMotion(callback, 2, 2, 1, 0);
            SetCharacterMotion(callback, 3, 3, 1, 0);
            SetCharacterMotion(callback, 4, 4, 1, 0);
            SetCharacterMotion(callback, 5, 5, 1, 0);
            SetCharacterMotion(callback, 6, 6, 1, 0);
            SetCharacterMotion(callback, 7, 7, 1, 0);
            SetCharacterMotion(callback, 9, 9, 1, 0);
            Hu3DModelAttrReset(callback->mdlId[9], HU3D_ATTR_DISPOFF);
            HuAudFXPlay(0x6D0);
            break;
        case 3:
            SetCameraInterpolation(6, -1, 8.0f);
            if (Hu3DMotionTimeGet(callback->mdlId[3]) >= 30.0f) {
                callback->work[0] = 99;
                SetCharacterMotion(callback, 8, 8, 0, 0);
                SetCameraInterpolation(6, -1, 128.0f);
                HuAudFXPlay(0x6DA);
                omVibrate(GetPlayerIndex(0), 12, 12, 0);
                omVibrate(GetPlayerIndex(1), 12, 12, 0);
                omVibrate(GetPlayerIndex(2), 12, 12, 0);
                omVibrate(GetPlayerIndex(3), 12, 12, 0);
                Hu3DModelAttrReset(callback->mdlId[8], HU3D_ATTR_DISPOFF);
            }
            break;
        case 99:
            if (Hu3DMotionEndCheck(callback->mdlId[3]) != 0) {
                callback->work[0] = 0;
                SetCharacterMotion(callback, 3, 10, 1, 1);
                SetCharacterMotion(callback, 4, 11, 1, 1);
                Hu3DModelAttrSet(callback->mdlId[9], HU3D_ATTR_DISPOFF);
            }
            break;
    }
    if (Hu3DMotionTimeGet(sceneObject->mdlId[10]) >= 25.0f && sceneState->dialogueState == 0) {
        sceneState->dialogueState = 1;
        SetCameraInterpolation(1, 10, 4.0f);
    }
    else if (Hu3DMotionTimeGet(sceneObject->mdlId[10]) >= 40.0f && sceneState->dialogueState == 1) {
        sceneState->dialogueState = 2;
        SetCameraInterpolation(1, 10, 4.0f);
    }
    switch (callback->work[1]) {
        case 1:
            if (callback->work[3] >= 360.0f) {
                callback->work[3] = 0;
            }
            floatingModel->pos.y = weightedAverageStep(floatingModel->pos.y, fullSineInterpolate(210.0f, 220.0f, callback->work[3]++, 360.0f), 30.0f);
            break;
        case 2:
            floatingModel->pos.y = weightedAverageStep(floatingModel->pos.y, 60.0f, 20.0f);
            callback->work[3] = 0;
            break;
        case 3:
            floatingModel->pos.y = weightedAverageStep(floatingModel->pos.y, 260.0f, 20.0f);
            callback->work[3] = 0;
            break;
    }
}

void PositionCharacterModels(OMOBJ *characterObject)
{
    SetCharacterMotion(characterObject, 0, 0, 1, 2);
    Hu3DModelPosSet(characterObject->mdlId[1], 0.0f, 0.0f, -680.0f);
    SetCharacterMotion(characterObject, 1, 1, 1, 2);
    Hu3DModelPosSet(characterObject->mdlId[2], 0.0f, 0.0f, -680.0f);
    SetCharacterMotion(characterObject, 2, 2, 1, 2);
    Hu3DModelPosSet(characterObject->mdlId[3], 525.0f, 0.0f, -1150.0f);
    SetCharacterMotion(characterObject, 3, 10, 1, 1);
    Hu3DModelPosSet(characterObject->mdlId[4], -525.0f, 0.0f, -1150.0f);
    SetCharacterMotion(characterObject, 4, 11, 1, 1);
    SetCharacterMotion(characterObject, 5, 5, 1, 2);
    SetCharacterMotion(characterObject, 6, 6, 1, 2);
    SetCharacterMotion(characterObject, 7, 7, 1, 2);
    Hu3DModelPosSet(characterObject->mdlId[8], 0.0f, 210.0f, -320.0f);
    Hu3DModelAttrSet(characterObject->mdlId[8], HU3D_ATTR_DISPOFF);
    SetCharacterMotion(characterObject, 8, 8, 1, 1);
    Hu3DModelAttrSet(characterObject->mdlId[9], HU3D_ATTR_DISPOFF);
    SetCharacterMotion(characterObject, 9, 9, 1, 2);
    characterObject->work[0] = characterObject->work[1] = characterObject->work[2] = characterObject->work[3] = 0;
}

void InitializeSceneCharacter(OMOBJ *characterObject)
{
    characterObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 21));
    characterObject->mtnId[0] = Hu3DMotionIDGet(characterObject->mdlId[0]);
    Hu3DModelShadowMapSet(characterObject->mdlId[0]);
    SetCharacterMotion(characterObject, 0, 0, 1, 2);
    characterObject->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 22));
    characterObject->mtnId[1] = Hu3DMotionIDGet(characterObject->mdlId[1]);
    Hu3DModelPosSet(characterObject->mdlId[1], 0.0f, 0.0f, -680.0f);
    Hu3DModelShadowMapSet(characterObject->mdlId[1]);
    SetCharacterMotion(characterObject, 1, 1, 1, 2);
    characterObject->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 23));
    characterObject->mtnId[2] = Hu3DMotionIDGet(characterObject->mdlId[2]);
    Hu3DModelPosSet(characterObject->mdlId[2], 0.0f, 0.0f, -680.0f);
    Hu3DModelShadowSet(characterObject->mdlId[2]);
    SetCharacterMotion(characterObject, 2, 2, 1, 2);
    characterObject->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 24));
    characterObject->mtnId[3] = Hu3DMotionIDGet(characterObject->mdlId[3]);
    characterObject->mtnId[10] = Hu3DJointMotionFile(characterObject->mdlId[3], DATA_MAKE_NUM(DATADIR_M436, 26));
    Hu3DModelPosSet(characterObject->mdlId[3], 525.0f, 0.0f, -1150.0f);
    Hu3DModelShadowSet(characterObject->mdlId[3]);
    SetCharacterMotion(characterObject, 3, 10, 1, 1);
    characterObject->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 25));
    characterObject->mtnId[4] = Hu3DMotionIDGet(characterObject->mdlId[4]);
    characterObject->mtnId[11] = Hu3DJointMotionFile(characterObject->mdlId[4], DATA_MAKE_NUM(DATADIR_M436, 27));
    Hu3DModelPosSet(characterObject->mdlId[4], -525.0f, 0.0f, -1150.0f);
    Hu3DModelShadowSet(characterObject->mdlId[4]);
    SetCharacterMotion(characterObject, 4, 11, 1, 1);
    characterObject->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 10));
    characterObject->mtnId[5] = Hu3DMotionIDGet(characterObject->mdlId[5]);
    Hu3DModelPosSet(characterObject->mdlId[5], 0.0f, 150.0f, 0.0f);
    Hu3DModelLayerSet(characterObject->mdlId[5], 1);
    SetCharacterMotion(characterObject, 5, 5, 1, 2);
    characterObject->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 11));
    characterObject->mtnId[6] = Hu3DMotionIDGet(characterObject->mdlId[6]);
    Hu3DModelPosSet(characterObject->mdlId[6], 0.0f, 150.0f, 0.0f);
    SetCharacterMotion(characterObject, 6, 6, 1, 2);
    characterObject->mdlId[7] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 12));
    characterObject->mtnId[7] = Hu3DMotionIDGet(characterObject->mdlId[7]);
    Hu3DModelPosSet(characterObject->mdlId[7], 0.0f, 150.0f, 0.0f);
    SetCharacterMotion(characterObject, 7, 7, 1, 2);
    characterObject->mdlId[8] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 8));
    characterObject->mtnId[8] = Hu3DMotionIDGet(characterObject->mdlId[8]);
    Hu3DModelPosSet(characterObject->mdlId[8], 0.0f, 210.0f, -320.0f);
    Hu3DModelShadowSet(characterObject->mdlId[8]);
    Hu3DModelAttrSet(characterObject->mdlId[8], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(characterObject->mdlId[8], 2);
    SetCharacterMotion(characterObject, 8, 8, 1, 1);
    characterObject->mdlId[9] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 28));
    characterObject->mtnId[9] = Hu3DMotionIDGet(characterObject->mdlId[9]);
    Hu3DModelAttrSet(characterObject->mdlId[9], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(characterObject->mdlId[9], 2);
    SetCharacterMotion(characterObject, 9, 9, 1, 2);
    characterObject->mdlId[10] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 29));
    characterObject->mtnId[12] = Hu3DMotionIDGet(characterObject->mdlId[10]);
    Hu3DModelAttrSet(characterObject->mdlId[10], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(characterObject->mdlId[10], 2);
    SetCharacterMotion(characterObject, 10, 12, 1, 2);
    characterObject->work[0] = characterObject->work[1] = characterObject->work[2] = characterObject->work[3] = 0;
    characterObject->objFunc = UpdateFloatingSceneModel;
}

void UpdateSceneCharacterVisibility(OMOBJ *characterObject)
{
    s32 i;

    switch (callback->work[0]) {
        case 1:
            callback->work[0] = 0;
            for (i = 0; i < 8; i++) {
                Hu3DModelAttrReset(callback->mdlId[i + 1], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(callback->mdlId[i + 9], HU3D_ATTR_DISPOFF);
                SetCharacterMotion(callback, i + 1, i + 1, 1, 0);
            }
            break;
        case 2:
            callback->work[0] = 0;
            Hu3DModelAttrReset(callback->mdlId[callback->work[1] + 1], HU3D_ATTR_DISPOFF);
            SetCharacterMotion(callback, callback->work[1] + 1, callback->work[1] + 1, 1, 0);
            break;
    }
}

void InitializeMapObjectPositions(OMOBJ *sceneObject)
{
    MapObjectState *objectLayout;
    s32 objectIndex;

    objectLayout = &mapObjectState;
    for (objectIndex = 0; objectIndex < 8; objectIndex++) {
        objectLayout->objectPositions[objectIndex].x = -2000.0 * cosd(287.5f - 5.0f * objectIndex);
        objectLayout->objectPositions[objectIndex].y = 0.0f;
        objectLayout->objectPositions[objectIndex].z = -2000.0 * sind(287.5f - 5.0f * objectIndex) - 1500.0;
    }
    for (objectIndex = 0; objectIndex < 8; objectIndex++) {
        Hu3DModelPosSet(sceneObject->mdlId[objectIndex + 1], objectLayout->objectPositions[objectIndex].x, objectLayout->objectPositions[objectIndex].y + 65.0f, objectLayout->objectPositions[objectIndex].z);
        Hu3DModelAttrSet(sceneObject->mdlId[objectIndex + 1], HU3D_ATTR_DISPOFF);
        SetCharacterMotion(sceneObject, objectIndex + 1, objectIndex + 1, 1, 2);
    }
    for (objectIndex = 0; objectIndex < 8; objectIndex++) {
        Hu3DModelPosSet(sceneObject->mdlId[objectIndex + 9], objectLayout->objectPositions[objectIndex].x, objectLayout->objectPositions[objectIndex].y, objectLayout->objectPositions[objectIndex].z);
        Hu3DModelAttrSet(sceneObject->mdlId[objectIndex + 9], HU3D_ATTR_DISPOFF);
    }
    sceneObject->work[0] = sceneObject->work[1] = sceneObject->work[2] = sceneObject->work[3] = 0;
}

void UpdateMapObjectPositions(OMOBJ *sceneObject)
{
    MapObjectState *objectLayout;
    s32 objectIndex;

    objectLayout = &mapObjectState;
    for (objectIndex = 0; objectIndex < 8; objectIndex++) {
        objectLayout->objectPositions[objectIndex].x = -2000.0 * cosd(287.5f - 5.0f * objectIndex);
        objectLayout->objectPositions[objectIndex].y = 0.0f;
        objectLayout->objectPositions[objectIndex].z = -2000.0 * sind(287.5f - 5.0f * objectIndex) - 1500.0;
    }
    for (objectIndex = 0; objectIndex < 8; objectIndex++) {
        sceneObject->mdlId[objectIndex + 1] = Hu3DModelCreateFile(characterResourceIds[objectIndex]);
        sceneObject->mtnId[objectIndex + 1] = Hu3DMotionIDGet(sceneObject->mdlId[objectIndex + 1]);
        Hu3DModelPosSet(sceneObject->mdlId[objectIndex + 1], objectLayout->objectPositions[objectIndex].x, 65.0f + objectLayout->objectPositions[objectIndex].y, objectLayout->objectPositions[objectIndex].z);
        Hu3DModelAttrSet(sceneObject->mdlId[objectIndex + 1], HU3D_ATTR_DISPOFF);
        SetCharacterMotion(sceneObject, objectIndex + 1, objectIndex + 1, 1, 2);
    }
    for (objectIndex = 0; objectIndex < 8; objectIndex++) {
        sceneObject->mdlId[objectIndex + 9] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 9));
        Hu3DModelPosSet(sceneObject->mdlId[objectIndex + 9], objectLayout->objectPositions[objectIndex].x, objectLayout->objectPositions[objectIndex].y, objectLayout->objectPositions[objectIndex].z);
        Hu3DModelShadowSet(sceneObject->mdlId[objectIndex + 9]);
    }
    sceneObject->work[0] = sceneObject->work[1] = sceneObject->work[2] = sceneObject->work[3] = 0;
    sceneObject->objFunc = UpdateSceneCharacterVisibility;
}

void UpdateHostCharacterAnimation(OMOBJ *characterObject)
{
    HostCharacterState *characterState;

    characterState = &hostCharacterState;
    switch (callback->work[3]) {
        case 1:
            callback->work[3] = 0;
            SetCharacterMotion(callback, 0, 2, 5, 0);
            break;
        case 2:
            callback->work[3] = 0;
            SetCharacterMotion(callback, 0, 1, 30, 1);
            break;
        case 3:
            callback->work[3] = 0;
            SetCharacterMotion(callback, 0, 3, 5, 0);
            break;
        case 4:
            callback->work[3] = 0;
            SetCharacterMotion(callback, 0, 2, 5, 1);
            break;
        case 5:
            callback->work[3] = 0;
            SetCharacterMotion(callback, 0, 2, 5, 0);
            break;
    }
    if (callback->work[2] == 1) {
        callback->work[2] = 0;
        SetCharacterMotion(callback, 1, 11, 1, 0);
        Hu3DModelAttrReset(callback->mdlId[1], HU3D_ATTR_DISPOFF);
    }
}

void InitializeHostCharacter(OMOBJ *characterObject)
{
    omSetTra(characterObject, 0.0f, 37.0f, -565.0f);
    omSetSca(characterObject, 1.5f, 1.5f, 1.5f);
    Hu3DModelHookReset(characterObject->mdlId[0]);
    SetCharacterMotion(characterObject, 0, 1, 1, 1);
    characterObject->work[0] = characterObject->work[1] = characterObject->work[2] = characterObject->work[3] = 0;
}

void SetModelMaterialVisibility(s32 modelIndex)
{
    HU3DMODEL *model;
    HSFDATA *modelData;
    HSFMATERIAL *material;
    s32 materialIndex;

    model = &Hu3DData[modelIndex];
    modelData = model->hsf;
    material = modelData->material;
    for (materialIndex = 0; materialIndex < modelData->materialNum; materialIndex++, material++) {
        material->flags = 0x10;
    }
}

void LoadHostCharacterModels(OMOBJ *characterObject)
{
    HostCharacterState *characterState;

    characterState = &hostCharacterState;
    characterObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 31));
    characterObject->mtnId[0] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 32));
    characterObject->mtnId[1] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 33));
    characterObject->mtnId[2] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 34));
    characterObject->mtnId[3] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 35));
    characterObject->mtnId[4] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 36));
    characterObject->mtnId[5] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 37));
    characterObject->mtnId[6] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 38));
    characterObject->mtnId[7] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 39));
    characterObject->mtnId[8] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 40));
    characterObject->mtnId[9] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 41));
    characterObject->mtnId[10] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 42));
    characterObject->mtnId[12] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 43));
    omSetTra(characterObject, 0.0f, 37.0f, -565.0f);
    omSetSca(characterObject, 1.5f, 1.5f, 1.5f);
    Hu3DModelShadowSet(characterObject->mdlId[0]);
    SetCharacterMotion(characterObject, 0, 1, 1, 1);
    characterObject->work[0] = characterObject->work[1] = characterObject->work[2] = characterObject->work[3] = 0;
    characterObject->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 30));
    characterObject->mtnId[11] = Hu3DMotionIDGet(characterObject->mdlId[1]);
    SetCharacterMotion(characterObject, 0, 11, 1, 2);
    Hu3DModelPosSet(characterObject->mdlId[1], 0.0f, 300.0f, -770.0f);
    Hu3DModelAttrSet(characterObject->mdlId[1], HU3D_ATTR_DISPOFF);
    characterObject->objFunc = UpdateHostCharacterAnimation;
}

void UpdateHostCharacterAction(OMOBJ *characterObject)
{
    switch (callback->work[3]) {
        case 1:
            callback->work[3] = 0;
            if (callback->work[0] == 0) {
                SetCharacterMotion(callback, 0, 2, 5, 0);
                SetCharacterMotion(callback, 1, 5, 5, 2);
            }
            else {
                SetCharacterMotion(callback, 0, 3, 5, 0);
            }
            break;
        case 2:
            callback->work[3] = 0;
            if (callback->work[0] == 0) {
                SetCharacterMotion(callback, 0, 0, 30, 1);
                SetCharacterMotion(callback, 1, 5, 30, 1);
            }
            else {
                SetCharacterMotion(callback, 0, 0, 30, 1);
            }
            break;
    }
}

void InitializeHelperItem(OMOBJ *sceneObject)
{
    s32 itemType;

    itemType = sceneObject->work[0];
    sceneObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 46));
    if (itemType == 0) {
        sceneObject->mtnId[0] = Hu3DJointMotionFile(sceneObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 48));
        sceneObject->mtnId[1] = Hu3DJointMotionFile(sceneObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 49));
        sceneObject->mtnId[2] = Hu3DJointMotionFile(sceneObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 50));
        omSetTra(sceneObject, -400.0f, 37.0f, -425.0f);
        sceneObject->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 58));
        sceneObject->mtnId[5] = Hu3DJointMotionFile(sceneObject->mdlId[1], DATA_MAKE_NUM(DATADIR_M436, 59));
        sceneObject->mtnId[6] = Hu3DJointMotionFile(sceneObject->mdlId[1], DATA_MAKE_NUM(DATADIR_M436, 60));
        Hu3DModelHookSet(sceneObject->mdlId[0], "g007m0-itemhook_r", sceneObject->mdlId[1]);
        Hu3DModelLayerSet(sceneObject->mdlId[0], 2);
        Hu3DModelLayerSet(sceneObject->mdlId[1], 2);
        SetCharacterMotion(sceneObject, 0, 0, 1, 1);
        SetCharacterMotion(sceneObject, 1, 5, 1, 1);
    }
    else {
        sceneObject->mtnId[0] = Hu3DJointMotionFile(sceneObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 47));
        sceneObject->mtnId[1] = Hu3DJointMotionFile(sceneObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 51));
        sceneObject->mtnId[2] = Hu3DJointMotionFile(sceneObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 52));
        sceneObject->mtnId[3] = Hu3DJointMotionFile(sceneObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 53));
        sceneObject->mtnId[4] = Hu3DJointMotionFile(sceneObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 54));
        sceneObject->mtnId[5] = Hu3DJointMotionFile(sceneObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 55));
        sceneObject->mtnId[6] = Hu3DJointMotionFile(sceneObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 56));
        sceneObject->mtnId[7] = Hu3DJointMotionFile(sceneObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, 57));
        omSetTra(sceneObject, 400.0f, 37.0f, -475.0f);
        omSetRot(sceneObject, 0.0f, -15.0f, 0.0f);
        SetCharacterMotion(sceneObject, 0, 0, 1, 1);
    }
    Hu3DModelShadowSet(sceneObject->mdlId[0]);
    sceneObject->objFunc = UpdateHostCharacterAction;
}

s32 GetPlayerIndex(s32 playerIndex)
{
    PlayerCharacterState *playerState;

    playerState = &playerCharacterStates[playerIndex];
    return playerState->playerIndex;
}

s32 CountCpuPlayers(void)
{
    PlayerCharacterState *playerState;
    s32 cpuPlayerCount;
    s32 playerIndex;

    cpuPlayerCount = 0;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerState = &playerCharacterStates[playerIndex];
        if (playerState->isCpu != 0) {
            cpuPlayerCount++;
        }
    }
    return cpuPlayerCount;
}

s32 IsCpuPlayer(s32 playerIndex)
{
    PlayerCharacterState *playerState;

    playerState = &playerCharacterStates[playerIndex];
    if (playerState->isCpu != 0) {
        return 1;
    }
    return 0;
}

s32 IsPlayerConfirmed(s32 playerIndex)
{
    PlayerCharacterState *playerState;

    playerState = &playerCharacterStates[playerIndex];
    if (HuPadBtnDown[playerState->controllerIndex] == 0x100) {
        return 1;
    }
    return 0;
}

void InitializePlayerOrder(void)
{
    PlayerCharacterState *playerState;
    s32 playerOrder[4];
    s32 orderIndex;

    playerOrder[0] = GWPlayerCurrGet();
    for (orderIndex = 1; orderIndex < 4; orderIndex++) {
        playerOrder[orderIndex] = playerOrder[orderIndex - 1] + 1;
        if (playerOrder[orderIndex] >= 4) {
            playerOrder[orderIndex] = 0;
        }
    }
    for (orderIndex = 0; orderIndex < 4; orderIndex++) {
        playerState = &playerCharacterStates[orderIndex];
        playerState->playerIndex = playerOrder[orderIndex];
        playerState->playerGroup = GWPlayerCfg[playerState->playerIndex].group;
        playerState->isCpu = GWPlayerCfg[playerState->playerIndex].iscom;
        if (playerState->isCpu != 0) {
            playerState->aiDifficulty = GWPlayerCfg[playerState->playerIndex].diff;
        }
        else {
            playerState->aiDifficulty = 0;
        }
        playerState->characterId = GWPlayerCfg[playerState->playerIndex].character;
        playerState->controllerIndex = GWPlayerCfg[playerState->playerIndex].pad_idx;
    }
}

s32 MoveCharacterTowardPosition(OMOBJ *characterObject, Vec targetPosition, float elapsedFrames, float durationFrames, float smoothingFactor, float smoothingFactor, s32 option)
{
    float targetDeltaX;
    float targetDeltaZ;
    float movementSpeed;
    float targetRotation;
    s32 reachedTarget;
    PlayerCharacterState *playerState;

    reachedTarget = 0;
    playerState = &playerCharacterStates[characterObject->work[0]];
    targetDeltaX = targetPosition.x - characterObject->trans.x;
    targetDeltaZ = characterObject->trans.z - targetPosition.z;
    targetRotation = 90.0 + atan2d(targetDeltaZ, targetDeltaX);
    movementSpeed = sqrtf(targetDeltaX * targetDeltaX + targetDeltaZ * targetDeltaZ);
    if (movementSpeed <= callback) {
        targetDeltaX = targetDeltaZ = 0.0f;
        reachedTarget = 1;
    }
    else {
        targetDeltaX = 72.0f * (targetDeltaX / movementSpeed);
        targetDeltaZ = 72.0f * (targetDeltaZ / movementSpeed);
    }
    movementSpeed = durationFrames * (sqrtf(targetDeltaX * targetDeltaX + targetDeltaZ * targetDeltaZ) / 72.0f);
    if (movementSpeed >= callback) {
        movementSpeed = callback;
    }
    else if (movementSpeed <= 1.0f) {
        targetDeltaX = targetDeltaZ = movementSpeed = 0.0f;
        reachedTarget = 1;
    }
    if (targetDeltaX != 0.0f || targetDeltaZ != 0.0f || callback >= 0.0f) {
        if (callback >= 0.0f && targetDeltaX == 0.0f && targetDeltaZ == 0.0f) {
            targetRotation = callback;
        }
        if (targetRotation >= 180.0f) {
            if (callback->rot.y - targetRotation >= 180.0f) {
                callback->rot.y -= 360.0f;
            }
            if (callback->rot.y - targetRotation < -180.0f) {
                callback->rot.y += 360.0f;
            }
        }
        else {
            if (callback->rot.y - targetRotation > 180.0f) {
                callback->rot.y -= 360.0f;
            }
            if (callback->rot.y - targetRotation <= -180.0f) {
                callback->rot.y += 360.0f;
            }
        }
        characterObject->rot.y = (targetRotation + characterObject->rot.y * (smoothingFactor - 1.0f)) / smoothingFactor;
    }
    characterObject->trans.x += movementSpeed * sind(characterObject->rot.y);
    characterObject->trans.z += movementSpeed * cosd(characterObject->rot.y);
    if (callback == 0) {
        if (movementSpeed >= 6.75f) {
            UpdateCharacterMotionIfChanged(characterObject, 0, 2, 15, 1);
        }
        else if (movementSpeed > 0.0f) {
            UpdateCharacterMotionIfChanged(characterObject, 0, 1, 15, 1);
        }
        else {
            UpdateCharacterMotionIfChanged(characterObject, 0, 0, 15, 1);
        }
    }
    return reachedTarget;
}

void UpdateCharacterEyeMaterials(OMOBJ *characterObject)
{
    PlayerCharacterState *playerState;
    char **eyeTextures;
    s32 materialVisible;
    s32 materialIndex, attributeIndex;
    HU3DMODEL *characterModel;
    HSFDATA *modelData;
    HSFMATERIAL *material;
    PlayerCharacterState *playerStateForFade;
    HSFATTRIBUTE *materialAttribute;

    playerState = &playerCharacterStates[characterObject->work[0]];
    if (callback->work[2] != 1) {
        return;
    }
    materialVisible = 1;
    characterModel = &Hu3DData[characterObject->mdlId[0]];
    modelData = characterModel->hsf;
    material = modelData->material;
    playerStateForFade = &playerCharacterStates[characterObject->work[0]];
    if (playerStateForFade->characterId == 3) {
        for (materialIndex = 0; materialIndex < characterModel->hsf->materialNum; materialIndex++, material++) {
            if (materialIndex != 1 && materialIndex != 2) {
                material->color[0] *= playerStateForFade->materialOpacity;
                if (material->color[0] == 0) {
                    material->color[0] = 0;
                }
                material->color[1] *= playerStateForFade->materialOpacity;
                if (material->color[1] == 0) {
                    material->color[1] = 0;
                }
                material->color[2] *= playerStateForFade->materialOpacity;
                if (material->color[2] == 0) {
                    material->color[2] = 0;
                }
            }
        }
    }
    else {
        eyeTextures = CharModelEyeBmpGet(playerStateForFade->characterId, 2);
        for (materialIndex = 0; materialIndex < modelData->materialNum; materialIndex++, material++) {
            materialVisible = 1;
            for (attributeIndex = 0; attributeIndex < material->attrNum; attributeIndex++) {
                materialAttribute = &modelData->attribute[material->attr[attributeIndex]];
                if (strcmp(eyeTextures[0], materialAttribute->bitmap->name) == 0 || strcmp(eyeTextures[1], materialAttribute->bitmap->name) == 0) {
                    materialVisible = 0;
                }
            }
            if (materialVisible != 0) {
                material->color[0] *= playerStateForFade->materialOpacity;
                if (material->color[0] == 0) {
                    material->color[0] = 0;
                }
                material->color[1] *= playerStateForFade->materialOpacity;
                if (material->color[1] == 0) {
                    material->color[1] = 0;
                }
                material->color[2] *= playerStateForFade->materialOpacity;
                if (material->color[2] == 0) {
                    material->color[2] = 0;
                }
            }
        }
    }
    playerStateForFade->materialOpacity -= 0.01;
    if (playerStateForFade->materialOpacity < 0.0f) {
        playerStateForFade->materialOpacity = 0.0f;
    }
}

void InitializePlayerCharacter(OMOBJ *characterObject)
{
    u32 playerIndex = characterObject->work[0];
    PlayerCharacterState *playerState = &playerCharacterStates[playerIndex];
    Vec playerStartPositions[] = { { -600.0f, 0.0f, 300.0f }, { -600.0f, 0.0f, 150.0f }, { -450.0f, 0.0f, 150.0f }, { -300.0f, 0.0f, 150.0f } };

    characterObject->mdlId[0] = CharModelCreate(playerState->characterId, 2);
    characterObject->mtnId[0] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0));
    characterObject->mtnId[1] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 2));
    characterObject->mtnId[2] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 3));
    characterObject->mtnId[3] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 88));
    characterObject->mtnId[4] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 89));
    characterObject->mtnId[5] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 90));
    characterObject->mtnId[6] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 72));
    characterObject->mtnId[7] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 28));
    characterObject->mtnId[8] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M436, playerState->characterId));
    Hu3DModelShadowSet(characterObject->mdlId[0]);
    Hu3DModelAttrSet(characterObject->mdlId[0], HU3D_ATTR_DISPOFF);
    UpdateCharacterMotionIfChanged(characterObject, 0, 0, 0, 1);
    omSetTra(characterObject, playerStartPositions[playerIndex].x, playerStartPositions[playerIndex].y, playerStartPositions[playerIndex].z);
    CharMotionDataClose(playerState->characterId);
    playerState->materialOpacity = 1.0f;
    characterObject->objFunc = UpdateCharacterEyeMaterials;
}

// unused
s16 gap0400000182Data[] = { 0, 0x43, 0, 0x43, 1, 0x43, 2, 0x43, 3, 0x43, 4, 0x43, 5, 0x43, 6, 0x43, 7 };

s32 UpdateItemHookAndAudio(OMOBJ *sceneObject)
{
    Mtx itemHookMatrix;
    RoundControlState *roundState;
    AudioVolumeState *audioEffects;
    HostCharacterState *hostCharacter;
    PlayerCharacterState *activePlayer;
    s32 effectIndex;

    roundState = &roundControlState;
    audioEffects = &audioVolumeState;
    hostCharacter = &hostCharacterState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    if (callback->work[3] == 1) {
        callback->work[3] = 0;
        callback->work[1] = 1;
        callback->work[2] = 0;
        audioEffects->audioState = 1;
        for (effectIndex = 0; effectIndex < 6; effectIndex++) {
            audioEffects->playerVolume[effectIndex] = -(50.0f * effectIndex);
        }
    }
    switch (callback->work[1]) {
        case 1:
            callback->work[1] = 2;
            SetCharacterMotion(hostCharacter->characterObject, 0, 8, 5, 0);
            break;
        case 2:
            if (Hu3DMotionTimeGet(hostCharacter->characterObject->mdlId[0]) == 90.0f) {
                callback->work[1] = 3;
                SetCharacterMotion(activePlayer->characterObject, 0, 7, 5, 0);
                Hu3DMotionTimeSet(hostCharacter->characterObject->mdlId[0], 90.0f);
                Hu3DMotionSpeedSet(hostCharacter->characterObject->mdlId[0], 0.0f);
                CharFXPlay(activePlayer->characterId, 0x123);
                HuAudFXPlay(0x6D5);
                omVibrate(activePlayer->playerIndex, 120, 12, 0);
            }
            break;
        case 3:
            Hu3DModelObjMtxGet(hostCharacter->characterObject->mdlId[0], "g000m0-itemhook_M", itemHookMatrix);
            activePlayer->characterObject->work[2] = 1;
            for (effectIndex = 0; effectIndex < 6; effectIndex++) {
                if (audioEffects->audioState == 1 || (audioEffects->audioState == 0 && audioEffects->playerVolume[effectIndex] > 0.0f)) {
                    audioEffects->playerVolume[effectIndex] += 10.0f;
                }
                if (audioEffects->playerVolume[effectIndex] > 0.0f) {
                    audioEffects->ambientVolume[effectIndex] += 0.1f;
                    audioEffects->effectVolume[effectIndex] -= 5.0f;
                    if (audioEffects->musicVolume[effectIndex] == 0.0f) {
                        SetCharacterMotion(callback, effectIndex + 1, effectIndex + 1, 1, 1);
                    }
                    if (audioEffects->musicVolume[effectIndex] < 8.0f) {
                        audioEffects->sequenceVolume[effectIndex] += 0.025f;
                    }
                    else if (audioEffects->musicVolume[effectIndex] < 23.0f) {
                        audioEffects->sequenceVolume[effectIndex] = 0.7f;
                    }
                    else if (audioEffects->musicVolume[effectIndex] < 31.0f) {
                        audioEffects->sequenceVolume[effectIndex] -= 0.1f;
                        if (audioEffects->sequenceVolume[effectIndex] < 0.0f) {
                            audioEffects->sequenceVolume[effectIndex] = 0.0f;
                        }
                    }
                    audioEffects->musicVolume[effectIndex] += 1.0f;
                }
                Hu3DModelPosSet(callback->mdlId[effectIndex + 1], itemHookMatrix[0][3], itemHookMatrix[1][3] + audioEffects->effectVolume[effectIndex], itemHookMatrix[2][3] + audioEffects->playerVolume[effectIndex]);
                Hu3DModelScaleSet(callback->mdlId[effectIndex + 1], 1.5f * audioEffects->ambientVolume[effectIndex], 1.5f * audioEffects->ambientVolume[effectIndex], 1.5f * audioEffects->ambientVolume[effectIndex]);
                Hu3DModelTPLvlSet(callback->mdlId[effectIndex + 1], audioEffects->sequenceVolume[effectIndex]);
                if (audioEffects->musicVolume[effectIndex] == 31.0f) {
                    audioEffects->playerVolume[effectIndex] = 0.0f;
                    audioEffects->effectVolume[effectIndex] = 0.0f;
                    audioEffects->ambientVolume[effectIndex] = 0.0f;
                    audioEffects->sequenceVolume[effectIndex] = 0.5f;
                    audioEffects->musicVolume[effectIndex] = 0.0f;
                }
            }
            if (callback->work[2]++ >= 120) {
                audioEffects->audioState = 0;
                Hu3DMotionSpeedSet(hostCharacter->characterObject->mdlId[0], 1.0f);
                activePlayer->characterObject->work[2] = 0;
            }
            if (callback->work[2] >= 10 && callback->work[2] <= 120) {
                AccelerateCameraInterpolation(6, 8.0f);
            }
            break;
    }
}

void PlayHostCharacterReaction(OMOBJ *characterObject)
{
    HostCharacterState *characterState;

    characterState = &hostCharacterState;
    SetCharacterMotion(characterState->characterObject, 0, 8, 5, 0);
    characterObject->objFunc = (void *)UpdateItemHookAndAudio; // update_item_hook_and_audio must return s32 to match
}

void UpdateAudioVolumeFades(OMOBJ *sceneObject)
{
    AudioVolumeState *audioEffects;
    s32 effectIndex;

    audioEffects = &audioVolumeState;
    for (effectIndex = 0; effectIndex < 6; effectIndex++) {
        if (effectIndex % 2 == 0) {
            sceneObject->mdlId[effectIndex + 1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 44));
            sceneObject->mtnId[effectIndex + 1] = Hu3DMotionIDGet(sceneObject->mdlId[effectIndex + 1]);
        }
        else {
            sceneObject->mdlId[effectIndex + 1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M436, 45));
            sceneObject->mtnId[effectIndex + 1] = Hu3DMotionIDGet(sceneObject->mdlId[effectIndex + 1]);
        }
        SetCharacterMotion(sceneObject, effectIndex + 1, effectIndex + 1, 5, 1);
        Hu3DModelPosSet(sceneObject->mdlId[effectIndex + 1], 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(sceneObject->mdlId[effectIndex + 1], 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(sceneObject->mdlId[effectIndex + 1], 0.0f, 0.0f, 0.0f);
        Hu3DModelLayerSet(sceneObject->mdlId[effectIndex + 1], 2);
        audioEffects->playerVolume[effectIndex] = -(50.0f * effectIndex);
        audioEffects->effectVolume[effectIndex] = 0.0f;
        audioEffects->ambientVolume[effectIndex] = 0.0f;
        audioEffects->sequenceVolume[effectIndex] = 0.5f;
        audioEffects->musicVolume[effectIndex] = 0.0f;
    }
    audioEffects->audioState = 1;
    sceneObject->objFunc = (void *)UpdateItemHookAndAudio; // update_item_hook_and_audio must return s32 to match
}

void SetCameraPresetHigh(void)
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
    InterpolateCameraStateInPlace(cameraState, 200.0f);
}

void BlendCameraToPresetHigh(void)
{
    CameraState *cameraState;
    CameraState cameraPreset;

    cameraState = &globalCameraState;
    cameraPreset.centerX = 0.0f;
    cameraPreset.centerY = 200.0f;
    cameraPreset.centerZ = -50.0f;
    cameraPreset.rotationX = 5.0f;
    cameraPreset.rotationY = 0.0f;
    cameraPreset.rotationZ = 0.0f;
    cameraPreset.zoom = 200.0f;
    CosineInterpolateCameraStateWithSmoothing(cameraState, &cameraPreset, cameraState->interpolationTimer++, 30.0f, 5.0f);
}

void SetCameraPresetClose(void)
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
    InterpolateCameraStateInPlace(cameraState, 200.0f);
}

void SetCameraPresetWide(void)
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
    InterpolateCameraStateInPlace(cameraState, 45.0f);
}

void SetCameraPresetLow(void)
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
    InterpolateCameraStateInPlace(cameraState, 30.0f);
}

void BlendCameraToPresetLow(void)
{
    CameraState *cameraState;
    CameraState cameraPreset;

    cameraState = &globalCameraState;
    cameraPreset.centerX = 0.0f;
    cameraPreset.centerY = 100.0f;
    cameraPreset.centerZ = -550.0f;
    cameraPreset.rotationX = -20.0f;
    cameraPreset.rotationY = 0.0f;
    cameraPreset.rotationZ = 0.0f;
    cameraPreset.zoom = 2400.0f;
    SineInterpolateCameraStateWithSmoothing(cameraState, &cameraPreset, cameraState->interpolationTimer++, 10.0f, 5.0f);
}

void SetCameraPresetFollow(void)
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
    InterpolateCameraStateInPlace(cameraState, 20.0f);
}

void SetCameraPresetOverview(void)
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
    InterpolateCameraStateInPlace(cameraState, 20.0f);
}

void SelectCameraCallback(s32 cameraIndex)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    CopyCameraCurrentToTarget(cameraState);
    switch (callback) {
        case 0:
            SetCameraUpdateCallback(SetCameraPresetWide);
            break;
        case 1:
            SetCameraUpdateCallback(SetCameraPresetLow);
            break;
        case 2:
            SetCameraUpdateCallback(BlendCameraToPresetLow);
            break;
        case 3:
            SetCameraUpdateCallback(BlendCameraToPresetHigh);
            break;
        case 4:
            SetCameraUpdateCallback(SetCameraPresetHigh);
            break;
        case 5:
            SetCameraUpdateCallback(SetCameraPresetClose);
            break;
        case 10:
            SetCameraUpdateCallback(SetCameraPresetFollow);
            break;
        case 11:
            SetCameraUpdateCallback(SetCameraPresetOverview);
            break;
    }
}

void ShowHostCharacterMessage(void)
{
    float messageSize[] = { 0.0f, 0.0f };
    HostCharacterState *hostCharacter;
    s32 windowId;

    hostCharacter = &hostCharacterState;
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 0));
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 1));
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 80.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D1);
    hostCharacter->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 0));
    WaitForPlayerMessageInput(windowId, 0);
    hostCharacter->characterObject->work[3] = 2;
    hostCharacter->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 1));
    WaitForPlayerMessageInput(windowId, 0);
    hostCharacter->characterObject->work[3] = 2;
    ClearCharacterNameMessage(windowId);
    selectedModelId = 1;
    HuPrcEnd();
}

void ShowHostCharacterChoicePrompt(void)
{
    float messageSize[] = { 0.0f, 0.0f };
    HostCharacterState *hostCharacter;
    s32 windowId;

    hostCharacter = &hostCharacterState;
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 3));
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 100.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D2);
    hostCharacter->characterObject->work[3] = 3;
    SetWindowMessage(windowId, MAKE_MESSID(29, 3));
    WaitForPlayerMessageInput(windowId, REFRESH_RATE);
    selectedModelId = 2;
    WaitForPlayerMessageInput(windowId, REFRESH_RATE);
    hostCharacter->characterObject->work[3] = 2;
    ClearCharacterNameMessage(windowId);
    selectedModelId = 1;
    HuPrcEnd();
}

void ShowHelperCharacterMessage(s32 windowId)
{
    float messageSize[2] = { 0.0f, 0.0f };
    HelperCharacterState *characterState = &helperCharacterStates[0];
    s32 messageIds[] = { MAKE_MESSID(29, 6), MAKE_MESSID(29, 8), MAKE_MESSID(29, 10), MAKE_MESSID(29, 12), MAKE_MESSID(29, 15), MAKE_MESSID(29, 17) };
    s32 windowId;

    characterState->characterObject->work[3] = 1;
    HuPrcSleep(15);
    MeasureMessageDimensions(messageSize, messageIds[windowId]);
    HuAudFXPlayVolPan(0x3E, 0x7F, 0x20);
    windowId = CreateMessageWindowVariant(20.0f, 360.0f, messageSize[0], messageSize[1], 0);
    SetWindowMessage(windowId, messageIds[windowId]);
    WaitForPlayerMessageInput(windowId, 0);
    characterState->characterObject->work[3] = 2;
    ClearCharacterNameMessage(windowId);
}

void ShowSecondHelperMessage(void)
{
    float messageSize[2] = { 0.0f, 0.0f };
    HelperCharacterState *characterState;
    s32 windowId;

    characterState = &helperCharacterStates[1];
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 5));
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 7));
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 9));
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 11));
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 13));
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 14));
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 16));
    windowId = CreateMessageWindowVariant(550.0f - messageSize[0], 340.0f, messageSize[0], messageSize[1], 1);
    HuAudFXPlayVolPan(0x65, 0x7F, 0x60);
    characterState->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 5));
    WaitForPlayerMessageInput(windowId, 0);
    characterState->characterObject->work[3] = 2;
    ShowHelperCharacterMessage(0);
    characterState->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 7));
    WaitForPlayerMessageInput(windowId, 0);
    characterState->characterObject->work[3] = 2;
    ShowHelperCharacterMessage(1);
    characterState->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 9));
    WaitForPlayerMessageInput(windowId, 0);
    characterState->characterObject->work[3] = 2;
    ShowHelperCharacterMessage(2);
    characterState->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 11));
    WaitForPlayerMessageInput(windowId, 0);
    characterState->characterObject->work[3] = 2;
    ShowHelperCharacterMessage(3);
    characterState->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 13));
    WaitForPlayerMessageInput(windowId, 0);
    characterState->characterObject->work[3] = 2;
    characterState->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 14));
    WaitForPlayerMessageInput(windowId, 0);
    characterState->characterObject->work[3] = 2;
    ShowHelperCharacterMessage(4);
    characterState->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 16));
    WaitForPlayerMessageInput(windowId, 0);
    characterState->characterObject->work[3] = 2;
    ShowHelperCharacterMessage(5);
    characterState->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 18));
    WaitForPlayerMessageInput(windowId, 0);
    characterState->characterObject->work[3] = 2;
    ClearCharacterNameMessage(windowId);
    selectedModelId = 1;
    HuPrcEnd();
}

void ShowSceneDialogueMessage(void)
{
    float messageSize[2] = { 0.0f, 0.0f };
    DialogueObjectState *sceneState;
    HostCharacterState *hostCharacter;
    s32 windowId;

    sceneState = &dialogueObjectState;
    hostCharacter = &hostCharacterState;
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 19));
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 20));
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 21));
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 22));
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 37));
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 80.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D2);
    hostCharacter->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 19));
    WaitForPlayerMessageInput(windowId, 0);
    hostCharacter->characterObject->work[3] = 2;
    hostCharacter->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 20));
    WaitForPlayerMessageInput(windowId, 0);
    hostCharacter->characterObject->work[3] = 2;
    hostCharacter->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 21));
    WaitForPlayerMessageInput(windowId, 3 * REFRESH_RATE);
    hostCharacter->characterObject->work[3] = 2;
    sceneState->mainObject->work[0] = 10;
    SetWindowMessage(windowId, MAKE_MESSID(29, 22));
    WaitForPlayerMessageInput(windowId, REFRESH_RATE);
    hostCharacter->characterObject->work[3] = 4;
    CreateScrollingMessageWindow(windowId);
    UpdateScrollingMessageWindow(windowId);
    DestroyScrollingMessageWindows();
    hostCharacter->characterObject->work[3] = 2;
    HuPrcSleep(2 * REFRESH_RATE);
    hostCharacter->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 37));
    WaitForPlayerMessageInput(windowId, 0);
    hostCharacter->characterObject->work[3] = 2;
    ClearCharacterNameMessage(windowId);
    selectedModelId = 1;
    HuPrcEnd();
}

void ShowHelperReactionMessage(void)
{
    float messageSize[2] = { 0.0f, 0.0f };
    HelperCharacterState *characterState;
    s32 windowId;

    characterState = &helperCharacterStates[1];
    selectedModelId = 0;
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 64));
    windowId = CreateMessageWindowVariant(550.0f - messageSize[0], 340.0f, messageSize[0], messageSize[1], 1);
    HuAudFXPlayVolPan(0x65, 0x7F, 0x60);
    characterState->characterObject->work[3] = 1;
    SetWindowMessage(windowId, MAKE_MESSID(29, 64));
    WaitForAnyPlayerMessageInput(windowId);
    characterState->characterObject->work[3] = 2;
    ClearCharacterNameMessage(windowId);
    selectedModelId = 1;
    HuPrcEnd();
}

void BeginPlayerIntroSequence(void) { }

s32 UpdatePlayerIntroSequence(void)
{
    DialogueObjectState *sceneState;

    sceneState = &dialogueObjectState;
    if (UpdateSequenceTimer(&sceneSequenceState, 0) == 80) {
        SelectCameraCallback(3);
        audioSequenceIds[0] = HuAudSeqPlay(6);
    }
    if (UpdateSequenceTimer(&sceneSequenceState, 0) == 115) {
        SelectCameraCallback(5);
    }
    if (UpdateSequenceTimer(&sceneSequenceState, 0) == 180) {
        SelectCameraCallback(0);
    }
    if (UpdateSequenceTimer(&sceneSequenceState, 0) == 0) {
        sceneState->mainObject->work[0] = 10;
    }
    if (UpdateSequenceTimer(&sceneSequenceState, 0) == 150) {
        sceneState->mainObject->work[0] = 1;
    }
    if (UpdateSequenceTimer(&sceneSequenceState, 1) != 0) {
        return AdvanceSequence(&sceneSequenceState);
    }
    return 0;
}

void ShowFirstPlayerIntro(void)
{
    selectedModelId = 0;
    HuPrcChildCreate(ShowHostCharacterMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 AdvanceFirstPlayerIntro(void)
{
    if (selectedModelId == 1 && UpdateSequenceTimer(&sceneSequenceState, 1) != 0) {
        return AdvanceSequence(&sceneSequenceState);
    }
    return 0;
}

void ShowSecondPlayerIntro(void)
{
    PlayerCharacterState *playerState;
    s32 playerIndex;

    selectedModelId = 0;
    HuPrcChildCreate(showHostCharacterPrompt, 100, 0x2000, 0, HuPrcCurrentGet());
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerState = &playerCharacterStates[playerIndex];
        Hu3DModelAttrReset(playerState->characterObject->mdlId[0], HU3D_ATTR_DISPOFF);
    }
}

s32 UpdateSecondPlayerIntro(void)
{
    DialogueObjectState *sceneState;
    MapObjectState *objectLayout;
    DialogueObjectState *introState;

    sceneState = &dialogueObjectState;
    objectLayout = &mapObjectState;
    if (selectedModelId == 2) {
        selectedModelId = 0;
        sceneState->mainObject->work[0] = 2;
        objectLayout->mapObject->work[0] = 1;
    }
    if (selectedModelId == 1 && UpdateSequenceTimer(&sceneSequenceState, 0) == 0) {
        introState = &dialogueObjectState;
        introState->mainObject->work[1] = 1;
    }
    if (selectedModelId == 1 && UpdateSequenceTimer(&sceneSequenceState, 1) != 0) {
        return AdvanceSequence(&sceneSequenceState);
    }
    return 0;
}

void SetCameraForPlayerIntro(void)
{
    SelectCameraCallback(10);
}

s32 AdvanceSecondPlayerIntro(void)
{
    if (UpdateSequenceTimer(&sceneSequenceState, 1) != 0) {
        return AdvanceSequence(&sceneSequenceState);
    }
    return 0;
}

void ShowFinalPlayerIntro(void)
{
    HuPrcChildCreate(ShowHelperReactionMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 AdvanceFinalPlayerIntro(void)
{
    if (selectedModelId != 0 && UpdateSequenceTimer(&sceneSequenceState, 1) != 0) {
        if (selectedMotionId != 0) {
            return AdvanceSequence(&sceneSequenceState);
        }
        SetSequenceEntry(&sceneSequenceState, 6);
    }
    return 0;
}

void PlayHostCharacterGreeting(void)
{
    HostCharacterState *characterState;

    characterState = &hostCharacterState;
    SetCharacterMotion(characterState->characterObject, 0, 12, 60, 1);
    selectedModelId = 0;
    HuPrcChildCreate(ShowSecondHelperMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 AdvanceHostCharacterGreeting(void)
{
    HostCharacterState *characterState;

    characterState = &hostCharacterState;
    if (selectedModelId == 1 && UpdateSequenceTimer(&sceneSequenceState, 0) == 0) {
        SetCharacterMotion(characterState->characterObject, 0, 1, 60, 1);
    }
    if (selectedModelId == 1 && UpdateSequenceTimer(&sceneSequenceState, 1) != 0) {
        return AdvanceSequence(&sceneSequenceState);
    }
    return 0;
}

void SetCameraForHostGreeting(void)
{
    SelectCameraCallback(11);
}

s32 FinishHostCharacterGreeting(void)
{
    if (UpdateSequenceTimer(&sceneSequenceState, 1) != 0) {
        return AdvanceSequence(&sceneSequenceState);
    }
    return 0;
}

void StartHostCharacterPromptDialogue(void)
{
    selectedModelId = 0;
    HuPrcChildCreate(ShowSceneDialogueMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 AdvanceHostCharacterPrompt(void)
{
    if (selectedModelId == 1) {
        selectedModelId = 2;
        HuAudSeqFadeOut(audioSequenceIds[0], 1000);
    }
    if (selectedModelId != 0 && UpdateSequenceTimer(&sceneSequenceState, 1) != 0) {
        SelectCameraCallback(2);
        return AdvanceSequence(&sceneSequenceState);
    }
    return 0;
}

SequenceEntry openingSequenceEntries[] = { { BeginPlayerIntroSequence, UpdatePlayerIntroSequence, 300 }, { ShowFirstPlayerIntro, AdvanceFirstPlayerIntro, 60 }, { ShowSecondPlayerIntro, UpdateSecondPlayerIntro, 60 },
    { SetCameraForPlayerIntro, AdvanceSecondPlayerIntro, 45 }, { ShowFinalPlayerIntro, AdvanceFinalPlayerIntro, 60 }, { PlayHostCharacterGreeting, AdvanceHostCharacterGreeting, 60 }, { SetCameraForHostGreeting, FinishHostCharacterGreeting, 45 },
    { StartHostCharacterPromptDialogue, AdvanceHostCharacterPrompt, 15 }, { NULL, NULL, -1 } };

void InitializeSceneSequence(void)
{
    InitializeSequence(&sceneSequenceState, openingSequenceEntries);
}

s32 UpdateSceneSequence(void)
{
    s32 transitionResult;

    transitionResult = 0;
    if (sceneSequenceState.entryCallback != NULL) {
        sceneSequenceState.entryCallback();
        sceneSequenceState.entryCallback = NULL;
    }
    if (sceneSequenceState.completionCallback != NULL) {
        transitionResult = sceneSequenceState.completionCallback();
    }
    return transitionResult;
}

s32 UpdatePlayerTurnInteraction(s32 playerIndex, s32 characterId, s32 motionIndex)
{
    RoundControlState *roundState;
    PlayerCharacterState *playerState;
    MapObjectState *objectLayout;
    WindowAnimationState *scrollingText;
    s32 selectedTarget;
    s32 targetSlot;
    s32 initialTargetSlot;
    s32 targetFound;
    s32 candidateIndex;
    s32 attemptIndex;

    selectedTarget = 0;
    targetSlot = characterId;
    initialTargetSlot = characterId;
    roundState = &roundControlState;
    playerState = &playerCharacterStates[playerIndex];
    objectLayout = &mapObjectState;
    scrollingText = &windowAnimationState;
    if (playerState->isCpu != 0) {
        if (callback % 27 == 2 && playerState->playerActionTimer == 0) {
            switch (playerState->aiDifficulty) {
                case 0:
                    if (callback <= 2 * REFRESH_RATE) {
                        selectedTarget = 0;
                    }
                    else if (roundState->roundState <= 3) {
                        playerState->playerActionTimer = selectedTarget = 1;
                    }
                    else {
                        selectedTarget = 0;
                    }
                    break;
                case 1:
                    if (callback <= 2 * REFRESH_RATE) {
                        selectedTarget = 0;
                    }
                    else if (roundState->roundState <= 6) {
                        playerState->playerActionTimer = selectedTarget = 1;
                    }
                    else {
                        selectedTarget = 0;
                    }
                    break;
                case 2:
                    if (callback <= REFRESH_RATE) {
                        selectedTarget = 0;
                    }
                    else if (roundState->roundState <= 9) {
                        playerState->playerActionTimer = selectedTarget = 1;
                    }
                    else {
                        selectedTarget = 0;
                    }
                    break;
                case 3:
                    if (callback <= REFRESH_RATE) {
                        selectedTarget = 0;
                    }
                    else if (roundState->roundState <= 12) {
                        playerState->playerActionTimer = selectedTarget = 1;
                    }
                    else {
                        selectedTarget = 0;
                    }
                    break;
            }
            if (selectedTarget == 0) {
                playerState->playerTargetSlot = rand8() % 8;
            }
            else {
                targetFound = 0;
                for (attemptIndex = 0; attemptIndex <= playerState->aiDifficulty; attemptIndex++) {
                    candidateIndex = rand8() % 10;
                    if (scrollingText->messageIndices[candidateIndex] != -1) {
                        playerState->playerTargetSlot = scrollingText->messageIndices[candidateIndex];
                        targetFound = 1;
                        break;
                    }
                }
                if (targetFound == 0) {
                    playerState->playerTargetSlot = rand8() % 8;
                }
            }
        }
        if (playerState->playerActionTimer == 1 && playerState->playerTargetSlot == targetSlot) {
            targetSlot = 99;
        }
        else if (playerState->playerTargetSlot > targetSlot && playerState->characterObject->trans.x >= objectLayout->objectPositions[targetSlot].x - 7.5f && targetSlot < 7) {
            targetSlot++;
        }
        else if (playerState->playerTargetSlot < targetSlot && playerState->characterObject->trans.x <= objectLayout->objectPositions[targetSlot].x + 7.5f && targetSlot > 0) {
            targetSlot--;
        }
    }
    else {
        targetSlot = characterId;
        if (HuPadStkX[playerState->controllerIndex] > 20.0f && playerState->characterObject->trans.x >= objectLayout->objectPositions[targetSlot].x - 7.5f && targetSlot < 7) {
            targetSlot++;
        }
        else if (HuPadStkX[playerState->controllerIndex] < -20.0f && playerState->characterObject->trans.x <= objectLayout->objectPositions[targetSlot].x + 7.5f && targetSlot > 0) {
            targetSlot--;
        }
        else if (HuPadBtnDown[playerState->controllerIndex] & 0x100) {
            targetSlot = 99;
        }
    }
    return targetSlot;
}

void UpdateCameraForActivePlayer(void)
{
    RoundControlState *roundState;
    PlayerCharacterState *activePlayer;
    MapObjectState *objectLayout;
    CameraState *cameraState;

    roundState = &roundControlState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    objectLayout = &mapObjectState;
    cameraState = &globalCameraState;
    cameraState->targetCenterX = activePlayer->characterObject->trans.x;
    cameraState->targetCenterY = activePlayer->characterObject->trans.y + 75.0f;
    cameraState->targetCenterZ = objectLayout->objectPositions[0].z - 100.0f;
    cameraState->targetRotationX = -10.0f;
    cameraState->targetRotationY = 0.0f;
    cameraState->targetRotationZ = 0.0f;
    cameraState->targetZoom = 750.0f;
    InterpolateCameraStateInPlace(cameraState, 10.0f);
}

void SetCameraPresetPlayerClose(void)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    cameraState->targetCenterX = 0.0f;
    cameraState->targetCenterY = 100.0f;
    cameraState->targetCenterZ = -550.0f;
    cameraState->targetRotationX = -20.0f;
    cameraState->targetRotationY = 0.0f;
    cameraState->targetRotationZ = 0.0f;
    cameraState->targetZoom = 2400.0f;
    InterpolateCameraStateInPlace(cameraState, 360.0f);
}

void BlendCameraToPlayerClose(void)
{
    CameraState *cameraState;
    CameraState cameraPreset;

    cameraState = &globalCameraState;
    cameraPreset.centerX = 0.0f;
    cameraPreset.centerY = 200.0f;
    cameraPreset.centerZ = -550.0f;
    cameraPreset.rotationX = -15.0f;
    cameraPreset.rotationY = 0.0f;
    cameraPreset.rotationZ = 0.0f;
    cameraPreset.zoom = 1200.0f;
    SineInterpolateCameraStateWithSmoothing(cameraState, &cameraPreset, cameraState->interpolationTimer++, 10.0f, 5.0f);
}

void BlendCameraToPlayerWide(void)
{
    CameraState *cameraState;
    CameraState cameraPreset;

    cameraState = &globalCameraState;
    cameraPreset.centerX = 0.0f;
    cameraPreset.centerY = 100.0f;
    cameraPreset.centerZ = -550.0f;
    cameraPreset.rotationX = -20.0f;
    cameraPreset.rotationY = 0.0f;
    cameraPreset.rotationZ = 0.0f;
    cameraPreset.zoom = 2400.0f;
    SineInterpolateCameraStateWithSmoothing(cameraState, &cameraPreset, cameraState->interpolationTimer++, 60.0f, 10.0f);
}

void SelectRoundCamera(s32 cameraIndex)
{
    CameraState *cameraState;

    cameraState = &globalCameraState;
    CopyCameraCurrentToTarget(cameraState);
    switch (callback) {
        case 0:
            SetCameraUpdateCallback(UpdateCameraForActivePlayer);
            break;
        case 1:
            SetCameraUpdateCallback(SetCameraPresetPlayerClose);
            break;
        case 2:
            SetCameraUpdateCallback(BlendCameraToPlayerClose);
            break;
        case 3:
            SetCameraUpdateCallback(BlendCameraToPlayerWide);
            break;
    }
}

void ShowHostRoundMessage(void)
{
    float messageSize[] = { 0.0f, 0.0f };
    s32 messageChoice;
    s32 windowId;
    HostCharacterState *characterState = &hostCharacterState;
    s32 messageIds[] = { MAKE_MESSID(29, 31), MAKE_MESSID(29, 32), MAKE_MESSID(29, 33) };

    messageChoice = rand8() % 3;
    HuPrcSleep(REFRESH_RATE + 10);
    MeasureMessageDimensions(messageSize, messageIds[messageChoice]);
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 60.0f, messageSize[0], messageSize[1], 2);
    characterState->characterObject->work[2] = 1;
    HuAudFXPlay(0x6D2);
    HuAudFXPlay(0x6E3);
    SetWindowMessage(windowId, messageIds[messageChoice]);
    WaitForPlayerMessageInput(windowId, 2 * REFRESH_RATE);
    ClearCharacterNameMessage(windowId);
    selectedModelId = 1;
    HuPrcEnd();
}

void ShowRandomHostMessage(void)
{
    float messageSize[] = { 0.0f, 0.0f };
    s32 messageChoice;
    s32 windowId;
    s32 messageIds[] = { MAKE_MESSID(29, 34), MAKE_MESSID(29, 35) };

    messageChoice = rand8() % 2;
    HuPrcSleep(40);
    MeasureMessageDimensions(messageSize, messageIds[messageChoice]);
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 60.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D6);
    HuAudFXPlay(0x6D9);
    HuAudSeqAllFadeOut(100);
    SetWindowMessage(windowId, messageIds[messageChoice]);
    WaitForPlayerMessageInput(windowId, 2 * REFRESH_RATE);
    ClearCharacterNameMessage(windowId);
    selectedModelId = 1;
    HuPrcEnd();
}

void ShowHostFinalMessage(void)
{
    float messageSize[] = { 0.0f, 0.0f };
    s32 windowId;

    HuPrcSleep(40);
    MeasureMessageDimensions(messageSize, MAKE_MESSID(29, 36));
    windowId = CreateMessageWindowVariant((580.0f - messageSize[0]) / 2, 60.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D6);
    HuAudFXPlay(0x6D9);
    HuAudSeqAllFadeOut(100);
    SetWindowMessage(windowId, MAKE_MESSID(29, 36));
    WaitForPlayerMessageInput(windowId, 2 * REFRESH_RATE);
    ClearCharacterNameMessage(windowId);
    selectedModelId = 1;
    HuPrcEnd();
}

void BeginPlayerTurn(void)
{
    RoundControlState *roundState;
    PlayerCharacterState *activePlayer;
    HostCharacterState *hostCharacter;

    roundState = &roundControlState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    activePlayer->playerActionTarget = activePlayer->playerActionState = activePlayer->playerActionTimer = activePlayer->playerTargetSlot = 0;
    hostCharacter = &hostCharacterState;
    SetCharacterMotion(hostCharacter->characterObject, 0, 12, 60, 1);
    SelectRoundCamera(0);
    SelectFirstPlayer(5);
}

s32 UpdatePlayerTurn(void)
{
    Vec targetPosition;
    RoundControlState *roundState;
    PlayerCharacterState *activePlayer;
    MapObjectState *objectLayout;
    HelperCharacterState *helperCharacter;
    PlayerCharacterState *otherPlayer;
    s32 targetIndex;
    s32 playerIndex;

    targetIndex = 0;
    roundState = &roundControlState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    objectLayout = &mapObjectState;
    UpdateSequenceTimer(&openingSequenceState, 1);
    if (UpdateSequenceTimer(&openingSequenceState, 0) < 10) {
        return 0;
    }
    if (activePlayer->playerActionState == 0) {
        if (IsLastPlayerSelected()) {
            activePlayer->playerActionState = 1;
            activePlayer->playerActionTimer = 0;
        }
        else {
            targetIndex = UpdatePlayerTurnInteraction(roundState->currentPlayerIndex, activePlayer->playerActionTarget, UpdateSequenceTimer(&openingSequenceState, 0));
            if (targetIndex == 99) {
                activePlayer->playerActionState = 1;
                activePlayer->playerActionTimer = 0;
                SelectFinalPlayer();
            }
            else {
                activePlayer->playerActionTarget = targetIndex;
            }
        }
    }
    else if (activePlayer->playerActionTimer++ >= 60) {
        return AdvanceSequence(&openingSequenceState);
    }
    targetPosition.x = objectLayout->objectPositions[activePlayer->playerActionTarget].x;
    targetPosition.y = activePlayer->characterObject->trans.y;
    targetPosition.z = objectLayout->objectPositions[0].z - 100.0f;
    MoveCharacterTowardPosition(activePlayer->characterObject, targetPosition, 0.0f, 9.0f, 5.0f, 2.0f, 0);
    if (activePlayer->characterObject->trans.z > targetPosition.z) {
        activePlayer->characterObject->trans.z = targetPosition.z;
    }
    if (activePlayer->characterObject->trans.z < targetPosition.z) {
        activePlayer->characterObject->trans.z = targetPosition.z;
    }
    helperCharacter = &helperCharacterStates[1];
    TurnModelTowardTarget(helperCharacter->characterObject, 0, activePlayer->characterObject, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (roundState->currentPlayerIndex != playerIndex) {
            otherPlayer = &playerCharacterStates[playerIndex];
            TurnModelTowardTarget(otherPlayer->characterObject, 0, activePlayer->characterObject, 0);
        }
    }
    return 0;
}

void BeginPlayerTurnTransition(void)
{
    RoundControlState *roundState;

    roundState = &roundControlState;
    roundState->roundEventState = 0;
}

s32 UpdatePlayerTurnTransition(void)
{
    RoundControlState *roundState;
    PlayerCharacterState *activePlayer;
    MapObjectState *objectLayout;
    HelperCharacterState *helperCharacter;
    PlayerCharacterState *otherPlayer;
    s32 playerIndex;

    roundState = &roundControlState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    objectLayout = &mapObjectState;
    if (UpdateSequenceTimer(&openingSequenceState, 0) < 30) {
        if (objectLayout->objectPositions[activePlayer->playerActionTarget].z - 100.0f > activePlayer->characterObject->trans.z) {
            activePlayer->characterObject->trans.z += 9.0f;
            UpdateCharacterMotionIfChanged(activePlayer->characterObject, 0, 1, 10, 1);
            if (objectLayout->objectPositions[activePlayer->playerActionTarget].z - 100.0f < activePlayer->characterObject->trans.z) {
                activePlayer->characterObject->trans.z = objectLayout->objectPositions[activePlayer->playerActionTarget].z - 100.0f;
                UpdateCharacterMotionIfChanged(activePlayer->characterObject, 0, 0, 10, 1);
            }
        }
        else {
            UpdateCharacterMotionIfChanged(activePlayer->characterObject, 0, 0, 10, 1);
        }
    }
    else if (UpdateSequenceTimer(&openingSequenceState, 0) == 30) {
        SetCharacterMotionCached(activePlayer->characterObject, 0, 5, 30, 0);
    }
    else if (isMotionAtTime(activePlayer->characterObject, 0, 5, 35, 1) != 0 && roundState->roundEventState == 0) {
        roundState->roundEventState = 1;
        Hu3DModelHookSet(activePlayer->characterObject->mdlId[0], CharModelItemHookGet(activePlayer->characterId, 2, 0), objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1]);
        if (activePlayer->characterId != 7) {
            Hu3DModelPosSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 0.0f, 5.0f, 0.0f);
        }
        else {
            Hu3DModelPosSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 0.0f, -15.0f, 0.0f);
        }
        Hu3DModelScaleSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 0.75f, 0.75f, 0.75f);
        omVibrate(activePlayer->playerIndex, 12, 4, 2);
    }
    helperCharacter = &helperCharacterStates[1];
    TurnModelTowardTarget(helperCharacter->characterObject, 0, activePlayer->characterObject, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (roundState->currentPlayerIndex != playerIndex) {
            otherPlayer = &playerCharacterStates[playerIndex];
            TurnModelTowardTarget(otherPlayer->characterObject, 0, activePlayer->characterObject, 0);
        }
    }
    if (UpdateSequenceTimer(&openingSequenceState, 1) != 0) {
        return AdvanceSequence(&openingSequenceState);
    }
    // Required to match.
    if (0) {
        s32 unreferencedMatchPadding = 0;
        (void)unreferencedMatchPadding;
        (void)unreferencedMatchPadding;
        (void)unreferencedMatchPadding;
        (void)unreferencedMatchPadding;
        (void)unreferencedMatchPadding;
        (void)unreferencedMatchPadding;
    }
    return 0;
}

void BeginMapItemAnimation(void)
{
    RoundControlState *roundState;
    HostCharacterState *hostCharacter;
    PlayerCharacterState *activePlayer;

    roundState = &roundControlState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    hostCharacter = &hostCharacterState;
    roundState->roundEventState = 0;
    activePlayer->playerActionState = activePlayer->playerActionTimer = activePlayer->playerTargetSlot = 0;
    Hu3DMotionOverlaySet(activePlayer->characterObject->mdlId[0], activePlayer->characterObject->mtnId[4]);
    Hu3DModelAttrSet(activePlayer->characterObject->mdlId[0], HU3D_MOTATTR_OVL_LOOP);
}

Vec playerDestination = { 0.0f, 0.0f, 0.0f };
Vec unusedPlayerDestination = { 0.0f, 0.0f, 0.0f }; // unused

s32 UpdateMapItemAnimation(void)
{
    RoundControlState *roundState;
    DialogueObjectState *sceneState;
    MapObjectState *objectLayout;
    PlayerCharacterState *activePlayer;
    HostCharacterState *hostCharacter;
    HelperCharacterState *helperCharacter;
    PlayerCharacterState *otherPlayer;
    s32 playerIndex;

    roundState = &roundControlState;
    sceneState = &dialogueObjectState;
    objectLayout = &mapObjectState;
    hostCharacter = &hostCharacterState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    if (activePlayer->playerActionState == 0 && UpdateSequenceTimer(&openingSequenceState, 0) <= 180) {
        playerDestination.x = 0.0f;
        playerDestination.y = activePlayer->characterObject->trans.y;
        playerDestination.z = 300.0f;
    }
    else if (activePlayer->playerActionState == 1 && UpdateSequenceTimer(&openingSequenceState, 0) <= 320) {
        playerDestination.x = 0.0f;
        playerDestination.y = activePlayer->characterObject->trans.y;
        playerDestination.z = -240.0f;
    }
    activePlayer->playerActionState += MoveCharacterTowardPosition(activePlayer->characterObject, playerDestination, 180.0f, 8.0f, 30.0f, 5.0f, 0);
    if (activePlayer->playerActionState == 1 && roundState->roundEventState == 0) {
        roundState->roundEventState = 1;
        SelectRoundCamera(1);
        UpdateCharacterMotionIfChanged(hostCharacter->characterObject, 0, 1, 60, 0);
        sceneState->mainObject->work[1] = 2;
    }
    helperCharacter = &helperCharacterStates[1];
    TurnModelTowardTarget(helperCharacter->characterObject, 0, activePlayer->characterObject, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (roundState->currentPlayerIndex != playerIndex) {
            otherPlayer = &playerCharacterStates[playerIndex];
            TurnModelTowardTarget(otherPlayer->characterObject, 0, activePlayer->characterObject, 0);
        }
    }
    UpdateSequenceTimer(&openingSequenceState, 1);
    if (activePlayer->playerActionState == 2) {
        Hu3DModelAttrReset(*activePlayer->characterObject->mdlId, HU3D_MOTATTR_OVL_LOOP);
        return AdvanceSequence(&openingSequenceState);
    }
    return 0;
}

void BeginPlayerItemAction(void)
{
    RoundControlState *roundState;
    HostCharacterState *hostCharacter;
    PlayerCharacterState *activePlayer;

    roundState = &roundControlState;
    hostCharacter = &hostCharacterState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    roundState->roundEventState = 0;
    hostCharacter->characterObject->work[3] = 0;
    Hu3DMotionSet(activePlayer->characterObject->mdlId[0], activePlayer->characterObject->mtnId[5]);
    Hu3DMotionTimeSet(activePlayer->characterObject->mdlId[0], 999.0f);
    Hu3DModelAttrReset(activePlayer->characterObject->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(activePlayer->characterObject->mdlId[0], HU3D_MOTATTR_REV);
    Hu3DMotionOverlayReset(activePlayer->characterObject->mdlId[0]);
    activePlayer->playerActionState = activePlayer->playerActionTimer = activePlayer->playerTargetSlot = 0;
}

s32 UpdatePlayerItemAction(void)
{
    RoundControlState *roundState;
    DialogueObjectState *sceneState;
    MapObjectState *objectLayout;
    PlayerCharacterState *activePlayer;
    HU3DMODEL *sceneModel;
    HU3DMODEL *targetModel;
    HelperCharacterState *helperCharacter;

    roundState = &roundControlState;
    sceneState = &dialogueObjectState;
    objectLayout = &mapObjectState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    sceneModel = &Hu3DData[sceneState->mainObject->mdlId[8]];
    if (isMotionAtTime(activePlayer->characterObject, 0, 5, 30, 0) != 0 && roundState->roundEventState == 0) {
        Hu3DModelHookReset(activePlayer->characterObject->mdlId[0]);
        Hu3DModelPosSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 0.0f, sceneModel->pos.y, -355.0f);
        Hu3DModelScaleSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 1.0f, 1.0f, 1.0f);
        roundState->roundEventState = activePlayer->playerActionState = 1;
    }
    if (activePlayer->playerActionState == 1 && activePlayer->playerActionTimer++ >= REFRESH_RATE) {
        targetModel = &Hu3DData[objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1]];
        sceneState->mainObject->work[1] = 3;
        targetModel->pos.y = sceneModel->pos.y;
        if (roundState->roundEventState == 1) {
            roundState->roundEventState = 2;
            helperCharacter = &helperCharacterStates[1];
            SetCharacterMotion(helperCharacter->characterObject, 0, 6, 60, 1);
        }
    }
    if (activePlayer->playerActionState == 1 && UpdateSequenceTimer(&openingSequenceState, 1) != 0) {
        return AdvanceSequence(&openingSequenceState);
    }
    return 0;
}

void BeginCharacterReaction(void)
{
    RoundControlState *roundState;
    HostCharacterState *hostCharacter;
    PlayerCharacterState *activePlayer;

    roundState = &roundControlState;
    hostCharacter = &hostCharacterState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    roundState->roundSubstate = roundState->roundEventState = 0;
    SetCharacterMotionCached(activePlayer->characterObject, 0, 0, 30, 1);
    SetCharacterMotion(hostCharacter->characterObject, 0, 4, 1, 0);
}

s32 UpdateCharacterReaction(void)
{
    RoundControlState *roundState;
    DialogueObjectState *sceneState;
    MapObjectState *objectLayout;
    HostCharacterState *hostCharacter;
    PlayerCharacterState *activePlayer;
    HelperCharacterState *helperCharacter;

    roundState = &roundControlState;
    sceneState = &dialogueObjectState;
    objectLayout = &mapObjectState;
    hostCharacter = &hostCharacterState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    if (isMotionAtTime(hostCharacter->characterObject, 0, 4, 80, 1) != 0 && roundState->roundEventState == 0) {
        roundState->roundEventState = 1;
        Hu3DModelHookSet(hostCharacter->characterObject->mdlId[0], "g000m0-itemhook_R", objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1]);
        Hu3DModelPosSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 0.0f, 90.0f, 0.0f);
        Hu3DModelScaleSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 0.5f, 0.5f, 0.5f);
        sceneState->mainObject->work[1] = 1;
    }
    if (UpdateSequenceTimer(&openingSequenceState, 0) == 180) {
        SelectRoundCamera(2);
    }
    if (roundState->roundEventState == 1) {
        helperCharacter = &helperCharacterStates[1];
        helperCharacter->characterObject->rot.y = weightedAverageStep(helperCharacter->characterObject->rot.y, 280.0f, 10.0f);
    }
    if (UpdateSequenceTimer(&openingSequenceState, 1) != 0) {
        sceneState->mainObject->work[0] = 10;
        return AdvanceSequence(&openingSequenceState);
    }
    return 0;
}

void BeginRoundDialogue(void)
{
    RoundControlState *roundState;
    DialogueObjectState *sceneState;
    HostCharacterState *hostCharacter;

    sceneState = &dialogueObjectState;
    roundState = &roundControlState;
    hostCharacter = &hostCharacterState;
    if (roundState->roundSubstate == 0) {
        roundState->roundSubstate = 1;
        if (roundState->roundState > 3) {
            SetCharacterMotion(hostCharacter->characterObject, 0, 9, 15, 0);
        }
        if (roundState->roundState > 3 && rand8() % 2 == 0) {
            roundState->roundSubstate = 2;
        }
    }
    else if (roundState->roundSubstate == 2) {
        roundState->roundSubstate = 1;
        SetCharacterMotion(hostCharacter->characterObject, 0, 10, 15, 0);
        if (roundState->roundState > 6 && rand8() % 2 == 0) {
            roundState->roundSubstate = 3;
        }
    }
    else if (roundState->roundSubstate == 3) {
        roundState->roundSubstate = 1;
        SetCharacterMotion(hostCharacter->characterObject, 0, 10, 15, 0);
    }
}

s32 UpdateRoundDialogue(void)
{
    RoundControlState *roundState;
    HostCharacterState *hostCharacter;

    roundState = &roundControlState;
    hostCharacter = &hostCharacterState;
    if (Hu3DMotionEndCheck(hostCharacter->characterObject->mdlId[0]) && UpdateSequenceTimer(&openingSequenceState, 1) != 0) {
        if (roundState->roundSubstate != 1) {
            return SetSequenceEntry(&openingSequenceState, 5);
        }
        else {
            return AdvanceSequence(&openingSequenceState);
        }
    }
    return 0;
}

void BeginRoundMessage(void)
{
    RoundControlState *roundState;
    WindowAnimationState *scrollingText;
    PlayerCharacterState *activePlayer;
    s32 messageIndex;

    roundState = &roundControlState;
    scrollingText = &windowAnimationState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    roundState->roundSubstate = 1;
    for (messageIndex = 0; messageIndex < scrollingText->windowCount; messageIndex++) {
        if (scrollingText->messageIndices[messageIndex] == activePlayer->playerActionTarget) {
            roundState->roundSubstate = 0;
            scrollingText->messageIndices[messageIndex] = -1;
            break;
        }
    }
    if (scrollingText->windowCount == roundState->roundState) {
        roundState->roundSubstate = 2;
    }
}

s32 UpdateRoundMessage(void)
{
    RoundControlState *roundState;

    roundState = &roundControlState;
    if (roundState->roundSubstate != 0) {
        SetSequenceEntry(&openingSequenceState, 10);
    }
    else {
        AdvanceSequence(&openingSequenceState);
    }
}

void BeginRoundTransition(void)
{
    RoundControlState *roundState;
    HostCharacterState *hostCharacter;

    roundState = &roundControlState;
    hostCharacter = &hostCharacterState;
    roundState->roundSubstate = roundState->roundEventState = 0;
    SetCharacterMotion(hostCharacter->characterObject, 0, 7, 15, 0);
    selectedModelId = 0;
    HuPrcChildCreate(ShowHostRoundMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 UpdateRoundTransition(void)
{
    RoundControlState *roundState;
    HostCharacterState *hostCharacter;
    MapObjectState *objectLayout;
    PlayerCharacterState *activePlayer;
    HelperCharacterState *helperCharacter;

    roundState = &roundControlState;
    objectLayout = &mapObjectState;
    hostCharacter = &hostCharacterState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    if (isMotionAtTime(hostCharacter->characterObject, 0, 7, 30, 1) != 0 && roundState->roundSubstate == 0) {
        roundState->roundSubstate = 1;
    }
    if (isMotionAtTime(hostCharacter->characterObject, 0, 7, 40, 1) != 0 && roundState->roundEventState == 0) {
        roundState->roundEventState = 1;
        helperCharacter = &helperCharacterStates[1];
        SetCharacterMotion(helperCharacter->characterObject, 0, 7, 10, 0);
        Hu3DModelHookReset(hostCharacter->characterObject->mdlId[0]);
        Hu3DModelPosSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], objectLayout->objectPositions[activePlayer->playerActionTarget].x, objectLayout->objectPositions[activePlayer->playerActionTarget].y + 65.0f,
            objectLayout->objectPositions[activePlayer->playerActionTarget].z);
        Hu3DModelRotSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 1.0f, 1.0f, 1.0f);
    }
    if (selectedModelId == 1) {
        selectedModelId = 2;
    }
    if (UpdateSequenceTimer(&openingSequenceState, 1) != 0) {
        return AdvanceSequence(&openingSequenceState);
    }
    return 0;
}

void PrepareNextPlayer(void)
{
    RoundControlState *roundState;
    PlayerCharacterState *nextPlayer;
    HelperCharacterState *helperCharacter;
    s32 nextPlayerIndex;

    roundState = &roundControlState;
    nextPlayerIndex = roundState->currentPlayerIndex + 1;
    if (nextPlayerIndex == 4) {
        nextPlayerIndex = 0;
    }
    nextPlayer = &playerCharacterStates[nextPlayerIndex];
    SetCharacterMotionCached(nextPlayer->characterObject, 0, 1, 15, 1);
    helperCharacter = &helperCharacterStates[1];
    SetCharacterMotion(helperCharacter->characterObject, 0, 0, 1, 1);
    Hu3DMotionSpeedSet(helperCharacter->characterObject->mdlId[0], 1.0f);
    SelectRoundCamera(3);
}

s32 PositionRoundPlayers(void)
{
    Vec playerPositions[] = { { -600.0f, 0.0f, 300.0f }, { -600.0f, 0.0f, 150.0f }, { -450.0f, 0.0f, 150.0f }, { -300.0f, 0.0f, 150.0f } };
    RoundControlState *roundState;
    MapObjectState *objectLayout;
    PlayerCharacterState *playerState;
    PlayerCharacterState *activePlayer;
    s32 positionIndex;
    s32 playerIndex;

    roundState = &roundControlState;
    objectLayout = &mapObjectState;
    playerIndex = roundState->currentPlayerIndex + 1;
    if (playerIndex == 4) {
        playerIndex = 0;
    }
    for (positionIndex = 0; positionIndex < 4; positionIndex++) {
        playerState = &playerCharacterStates[playerIndex];
        if (positionIndex != 0) {
            MoveCharacterTowardPosition(playerState->characterObject, playerPositions[positionIndex], 0.0f, 9.0f, 10.0f, 8.0f, 0);
        }
        else {
            playerState->characterObject->trans.x -= 1.0f;
            playerState->characterObject->trans.z += 5.0f;
            playerState->characterObject->rot.y = weightedAverageStep(playerState->characterObject->rot.y, 0.0f, 10.0f);
            if (playerState->characterObject->trans.x <= -600.0f) {
                playerState->characterObject->trans.x = -600.0f;
            }
            if (playerState->characterObject->trans.z >= objectLayout->objectPositions[0].z - 100.0f) {
                UpdateCharacterMotionIfChanged(playerState->characterObject, 0, 0, 15, 1);
                playerState->characterObject->trans.z = objectLayout->objectPositions[0].z - 100.0f;
            }
        }
        if (positionIndex != 0 && positionIndex != 3) {
            if (playerState->characterObject->trans.z > 150.0f) {
                playerState->characterObject->trans.z = 150.0f;
            }
            if (playerState->characterObject->trans.z < 150.0f) {
                playerState->characterObject->trans.z = 150.0f;
            }
        }
        playerIndex++;
        if (playerIndex == 4) {
            playerIndex = 0;
        }
    }
    if (UpdateSequenceTimer(&openingSequenceState, 1) != 0) {
        activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
        activePlayer->characterObject->rot.y = 0.0f;
        roundState->currentPlayerIndex++;
        if (roundState->currentPlayerIndex == 4) {
            roundState->currentPlayerIndex = 0;
        }
        roundState->roundState++;
        return SetSequenceEntry(&openingSequenceState, 0);
    }
    return 0;
}

void ShowRoundEndMessage(void)
{
    RoundControlState *roundState;
    HostCharacterState *hostCharacter;

    roundState = &roundControlState;
    hostCharacter = &hostCharacterState;
    SetCharacterMotion(hostCharacter->characterObject, 0, 6, 1, 0);
    selectedModelId = 0;
    if (roundState->roundSubstate == 1) {
        HuPrcChildCreate(ShowRandomHostMessage, 100, 0x2000, 0, HuPrcCurrentGet());
    }
    else {
        HuPrcChildCreate(ShowHostFinalMessage, 100, 0x2000, 0, HuPrcCurrentGet());
    }
    roundState->roundState = roundState->roundSubstate = roundState->roundEventState = 0;
}

s32 UpdateRoundEndSequence(void)
{
    RoundControlState *roundState;
    DialogueObjectState *sceneState;
    MapObjectState *objectLayout;
    HostCharacterState *hostCharacter;
    HelperCharacterState *helperCharacter;
    PlayerCharacterState *activePlayer;

    roundState = &roundControlState;
    sceneState = &dialogueObjectState;
    objectLayout = &mapObjectState;
    hostCharacter = &hostCharacterState;
    helperCharacter = &helperCharacterStates[1];
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    sourceTransformMatrix[0][3] = 400.0f;
    sourceTransformMatrix[1][3] = 91.0f;
    sourceTransformMatrix[2][3] = -436.0f;
    helperCharacter->characterObject->rot.y = weightedAverageStep(helperCharacter->characterObject->rot.y, 360.0f, 10.0f);
    if (isMotionAtTime(hostCharacter->characterObject, 0, 6, 40, 1) != 0 && roundState->roundEventState == 0) {
        roundState->roundEventState = 1;
        Hu3DModelHookReset(hostCharacter->characterObject->mdlId[0]);
        Hu3DModelObjMtxGet(hostCharacter->characterObject->mdlId[0], "g000m0-itemhook_R", targetTransformMatrix);
        Hu3DModelPosSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], targetTransformMatrix[0][3], targetTransformMatrix[1][3], targetTransformMatrix[2][3]);
        Hu3DModelRotSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 1.0f, 1.0f, 1.0f);
        Hu3DMotionSpeedSet(sceneState->mainObject->mdlId[8], -1.0f);
        roundState->roundState = 1;
        HuAudFXPlay(0x6E0);
    }
    if (roundState->roundState == 1) {
        interpolatedPosition.x = linearInterpolate(targetTransformMatrix[0][3], sourceTransformMatrix[0][3], frameCounter, 30.0f);
        if (frameCounter < 13) {
            interpolatedPosition.y = sineEaseInterpolate(targetTransformMatrix[1][3], 500.0f, frameCounter, 13.0f);
        }
        else {
            interpolatedPosition.y = cosineEaseInterpolate(500.0f, sourceTransformMatrix[1][3], frameCounter - 13, 17.0f);
        }
        interpolatedPosition.z = linearInterpolate(targetTransformMatrix[2][3], sourceTransformMatrix[2][3], frameCounter, 30.0f);
        if (frameCounter == 10) {
            SetCharacterMotion(helperCharacter->characterObject, 0, 4, 1, 0);
            Hu3DMotionSpeedSet(helperCharacter->characterObject->mdlId[0], 1.0f);
        }
        if (isMotionAtTime(helperCharacter->characterObject, 0, 4, 21, 1) != 0 && roundState->roundSubstate == 0) {
            roundState->roundSubstate = 1;
            Hu3DMotionSpeedSet(helperCharacter->characterObject->mdlId[0], 0.0f);
        }
        else {
            Hu3DModelPosSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], interpolatedPosition.x, interpolatedPosition.y, interpolatedPosition.z);
        }
        if (frameCounter++ >= 30) {
            roundState->roundState = 2;
        }
    }
    else if (roundState->roundState == 2) {
        Hu3DModelHookSet(helperCharacter->characterObject->mdlId[0], "g007m0-itemhook_c", objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1]);
        Hu3DModelPosSet(objectLayout->mapObject->mdlId[activePlayer->playerActionTarget + 1], 0.0f, 0.0f, 0.0f);
        Hu3DMotionSpeedSet(helperCharacter->characterObject->mdlId[0], 1.0f);
    }
    if (selectedModelId == 1 && Hu3DMotionEndCheck(helperCharacter->characterObject->mdlId[0])) {
        frameCounter = 0;
        SetCharacterMotion(helperCharacter->characterObject, 0, 5, 1, 0);
        return AdvanceSequence(&openingSequenceState);
    }
    return 0;
}

void BeginFinalRoundScene(void)
{
    RoundControlState *roundState;
    HostCharacterState *hostCharacter;
    AudioVolumeState *audioEffects;
    HelperCharacterState *helperCharacter;
    HelperCharacterState *secondHelper;
    PlayerCharacterState *activePlayer;

    roundState = &roundControlState;
    hostCharacter = &hostCharacterState;
    audioEffects = &audioVolumeState;
    helperCharacter = &helperCharacterStates[1];
    secondHelper = &helperCharacterStates[0];
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    audioEffects->audioObject->work[3] = 1;
    helperCharacter->characterObject->rot.y = 0.0f;
    SetCharacterMotion(helperCharacter->characterObject, 0, 5, 30, 1);
    SetCharacterMotion(secondHelper->characterObject, 0, 1, 30, 1);
    SetCharacterMotion(secondHelper->characterObject, 1, 6, 30, 1);
}

s32 UpdateFinalRoundScene(void)
{
    if (UpdateSequenceTimer(&openingSequenceState, 1) != 0) {
        return AdvanceSequence(&openingSequenceState);
    }
    return 0;
}

SequenceEntry roundSequenceEntries[]
    = { { BeginPlayerTurn, UpdatePlayerTurn, 600 }, { BeginPlayerTurnTransition, UpdatePlayerTurnTransition, 90 }, { BeginMapItemAnimation, UpdateMapItemAnimation, 300 }, { BeginPlayerItemAction, UpdatePlayerItemAction, 150 },
          { BeginCharacterReaction, UpdateCharacterReaction, 240 }, { BeginRoundDialogue, UpdateRoundDialogue, 90 }, { BeginRoundMessage, UpdateRoundMessage, 120 }, { BeginRoundTransition, UpdateRoundTransition, 120 },
          { PrepareNextPlayer, PositionRoundPlayers, 120 }, { NULL, NULL, -1 }, { ShowRoundEndMessage, UpdateRoundEndSequence, -1 }, { BeginFinalRoundScene, UpdateFinalRoundScene, 60 }, { NULL, NULL, -1 } };

void InitializeRoundSequence(void)
{
    InitializeSequence(&openingSequenceState, roundSequenceEntries);
}

s32 UpdateRoundSequence(void)
{
    s32 transitionResult;

    transitionResult = 0;
    if (openingSequenceState.entryCallback != NULL) {
        openingSequenceState.entryCallback();
        openingSequenceState.entryCallback = NULL;
    }
    if (openingSequenceState.completionCallback != NULL) {
        transitionResult = openingSequenceState.completionCallback();
    }
    return transitionResult;
}

void BeginFinalCharacterAnimation(void)
{
    RoundControlState *roundState;
    PlayerCharacterState *activePlayer;

    roundState = &roundControlState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    roundState->roundEventState = 0;
    activeModelId = activePlayer->characterObject->mdlId[0];
    activeMotionId = activePlayer->characterObject->mtnId[8];
    activeCharacterId = activePlayer->characterId;
    HuPrcChildCreate(PlayCharacterStepSounds, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 UpdateFinalCharacterAnimation(void)
{
    RoundControlState *roundState;
    PlayerCharacterState *activePlayer;

    roundState = &roundControlState;
    activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
    if (UpdateSequenceTimer(&roundSequenceState, 0) == 120) {
        SetCharacterMotion(activePlayer->characterObject, 0, 8, 30, 0);
        HuAudSStreamPlay(3);
    }
    if (UpdateSequenceTimer(&roundSequenceState, 1) != 0) {
        return AdvanceSequence(&roundSequenceState);
    }
    return 0;
}

SequenceEntry finalSequenceEntries[] = { { BeginFinalCharacterAnimation, UpdateFinalCharacterAnimation, 360 }, { NULL, NULL, -1 } };

void InitializeFinalSequence(void)
{
    InitializeSequence(&roundSequenceState, finalSequenceEntries);
}

s32 UpdateFinalSequence(void)
{
    s32 transitionResult;

    transitionResult = 0;
    if (roundSequenceState.entryCallback != NULL) {
        roundSequenceState.entryCallback();
        roundSequenceState.entryCallback = NULL;
    }
    if (roundSequenceState.completionCallback != NULL) {
        transitionResult = roundSequenceState.completionCallback();
    }
    return transitionResult;
}

void FinishSceneAndExit(OMOBJ *sceneObject)
{
    HostCharacterState *hostCharacter;
    RoundControlState *roundState;
    PlayerCharacterState *activePlayer;
    s32 playerIndex;

    hostCharacter = &hostCharacterState;
    if (WipeStatGet() == 0) {
        CharModelKill(-1);
        MGSeqKillAll();
        HuAudFadeOut(1);
        roundState = &roundControlState;
        activePlayer = &playerCharacterStates[roundState->currentPlayerIndex];
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            GWPlayerCoinWinSet(playerIndex, 0);
        }
        GWPlayerCoinWinSet(activePlayer->playerIndex, 1);
        omOvlReturnEx(1, 1);
        sceneObject->objFunc = NULL;
    }
}

void CheckSceneExitRequest(OMOBJ *sceneObject)
{
    if (omSysExitReq == 1) {
        HuAudSeqAllFadeOut(250);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
        sceneObject->objFunc = FinishSceneAndExit;
    }
}

void InitializeRoundScene(void)
{
    HelperCharacterState *helperCharacter;
    RoundControlState *roundState;
    PlayerCharacterState *playerState;
    s32 playerIndex;

    InitializeScrollingMessages();
    PositionCharacterModels(dialogueObjectState.mainObject);
    InitializeMapObjectPositions(mapObjectState.mapObject);
    InitializeHostCharacter(hostCharacterState.characterObject);
    helperCharacter = &helperCharacterStates[1];
    Hu3DModelHookReset(helperCharacter->characterObject->mdlId[0]);
    SetCharacterMotion(helperCharacter->characterObject, 0, 0, 1, 1);
    roundState = &roundControlState;
    roundState->currentPlayerIndex = roundState->roundState = roundState->roundSubstate = roundState->roundEventState = 0;
    {
        MapObjectState *objectLayout = &mapObjectState;
        Vec playerPositions[] = { { -600.0f, 0.0f, 300.0f }, { -600.0f, 0.0f, 150.0f }, { -450.0f, 0.0f, 150.0f }, { -300.0f, 0.0f, 150.0f } };

        playerPositions[0].z = objectLayout->objectPositions[0].z - 100.0f;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerState = &playerCharacterStates[playerIndex];
            if (playerIndex == 0) {
                playerPositions[playerIndex].z = objectLayout->objectPositions[0].z - 100.0f;
            }
            omSetTra(playerState->characterObject, playerPositions[playerIndex].x, playerPositions[playerIndex].y, playerPositions[playerIndex].z);
            omSetRot(playerState->characterObject, 0.0f, 0.0f, 0.0f);
            omSetSca(playerState->characterObject, 1.0f, 1.0f, 1.0f);
            playerState->playerActionTarget = playerState->playerActionState = playerState->playerActionTimer = playerState->playerTargetSlot = 0;
        }
    }
    InitializeSceneSequence();
    InitializeRoundSequence();
    InitializeFinalSequence();
    SelectCameraCallback(4);
    introSequenceState.openingSoundEffectId = HuAudFXPlay(0x6D7);
}

void UpdateRoundScene(OMOBJ *sceneObject)
{
    switch (roundPhase) {
        case 0:
            InitializeRoundScene();
            roundPhase = 1;
            break;
        case 1:
            if (WipeStatGet() == 0) {
                roundPhase = 2;
            }
            break;
        case 2:
            if (UpdateSceneSequence()) {
                roundPhase = 3;
            }
            break;
        case 3:
            minigameSequenceId = MGSeqStartCreate();
            roundPhase = 4;
            audioSequenceIds[1] = -1;
            break;
        case 4:
            if ((MGSeqStatGet(minigameSequenceId) & 0x10) && audioSequenceIds[1] == -1) {
                audioSequenceIds[1] = HuAudSeqPlay(0x4D);
            }
            if (MGSeqStatGet(minigameSequenceId) == 0) {
                roundPhase = 5;
            }
            break;
        case 5:
            if (UpdateRoundSequence()) {
                roundPhase = 6;
            }
            break;
        case 6:
            minigameSequenceId = MGSeqFinishCreate();
            roundPhase = 7;
            HuAudSeqPauseAll(0);
            HuAudSeqAllFadeOut(250);
            break;
        case 7:
            if (MGSeqStatGet(minigameSequenceId) == 0) {
                roundPhase = 8;
            }
            break;
        case 8:
            dialogueObjectState.mainObject->work[0] = 999;
            if (UpdateFinalSequence()) {
                roundPhase = 9;
            }
            break;
        case 9:
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
            sceneObject->objFunc = FinishSceneAndExit;
            HuAudFXFadeOut(introSequenceState.openingSoundEffectId, 500);
            break;
    }
    CheckSceneExitRequest(sceneObject);
}

void CreateRoundSceneObjects(OMOBJ *sceneObject)
{
    DialogueObjectState *sceneState;
    MapObjectState *objectLayout;
    HostCharacterState *hostCharacter;
    HelperCharacterState *helperCharacter;
    PlayerCharacterState *playerState;
    AudioVolumeState *audioEffects;
    s32 playerIndex;

    sceneState = &dialogueObjectState;
    sceneState->mainObject = omAddObjEx(objectManager, 0x100, 16, 16, -1, InitializeSceneCharacter);
    objectLayout = &mapObjectState;
    objectLayout->mapObject = omAddObjEx(objectManager, 0x200, 20, 20, -1, UpdateMapObjectPositions);
    hostCharacter = &hostCharacterState;
    hostCharacter->characterObject = omAddObjEx(objectManager, 0x300, 16, 16, -1, LoadHostCharacterModels);
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        helperCharacter = &helperCharacterStates[playerIndex];
        helperCharacter->characterObject = omAddObjEx(objectManager, 0x400, 16, 16, -1, InitializeHelperItem);
        helperCharacter->characterObject->work[0] = playerIndex;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerState = &playerCharacterStates[playerIndex];
        playerState->characterObject = omAddObjEx(objectManager, 0x500, 16, 16, -1, InitializePlayerCharacter);
        playerState->characterObject->work[0] = playerIndex;
    }
    audioEffects = &audioVolumeState;
    audioEffects->audioObject = omAddObjEx(objectManager, 0x600, 16, 16, -1, UpdateAudioVolumeFades);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    sceneObject->objFunc = UpdateRoundScene;
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

void InitializeRoundMinigame(void)
{
    objectManager = omInitObjMan(62, 0x2000);
    omGameSysInit(objectManager);
    CreateSceneLight();
    InitializeSceneCamera();
    InitializeSceneShadows();
    InitializeMessageWindows();
    InitializePlayerSelection();
    InitializePlayerOrder();
    omAddObjEx(objectManager, 0x1000, 0, 0, -1, CreateRoundSceneObjects);
}

void ObjectSetup(void)
{
    InitializeRoundMinigame();
}
