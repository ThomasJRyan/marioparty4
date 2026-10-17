#include "REL/mstoryDll.h"
#include "game/disp.h"
#include "game/gamework_data.h"

#ifndef __MWERKS__
#include "game/gamework.h"
#endif

#include "game/hu3d.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/window.h"
#include "game/wipe.h"

#include "ext_math.h"

char unusedStoryDebugPadding[0x3C]; // unused
s32 currentStorySoundEffectId;
s32 storySequenceIds[4];
char unusedStoryStatePadding[0x10]; // unused
StoryCameraTransitionState storyCameraState;
HUPROCESS* storyObjectManager;

s32 storyEventSoundEffectIds[][4] = {
    { 0x37, 0x38, 0x36, 0x39 },
    { 0x43, 0x45, 0x44, 0x43 },
    { 0x40, 0x42, 0x41, 0x40 },
    { 0x4B, 0x4D, 0x4C, 0x4B },
    { 0x46, 0x48, 0x47, 0x46 },
    { 0x3E, 0x3F, 0x3E, 0x3E }
};

char* storyEventMenuLabels[] = {
    "S-2 : NORMAL STORY - BOARD CLEAR EVENT",
    "S-3 : NORMAL STORY - BOARD MISS EVENT",
    "S-4 : NORMAL STORY - MINIGAME CLEAR EVENT",
    "S-5 : NORMAL STORY - MINIGAME MISS EVENT"
};

Void FinishStoryMenuExit(OMOBJ* object) {
    if (WipeStatGet() == 0) {
        object->objFunc = NULL;
        omOvlReturnEx(1, 1);
    }
}

Void HandleStoryMenuExitRequest(OMOBJ* object) {
    if (omSysExitReq != 0) {
        object->objFunc = FinishStoryMenuExit;
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    }
}

Void SelectStoryEvent(Void) {
    s32 unreferencedMenuState;
    s32 selectedEventIndex;
    s32 selectedBoardIndex;
    s32 unreferencedMenuFlagA;
    s32 unreferencedMenuFlagB;
    s32 unreferencedMenuFlagC;
    s32 i;

    unreferencedMenuState = 0;
    selectedEventIndex = 0;
    selectedBoardIndex = 0;
    unreferencedMenuFlagA = 1;
    unreferencedMenuFlagB = 1;
    unreferencedMenuFlagC = 1;
    omAddObjEx(storyObjectManager, 0x2000, 0, 0, -1, HandleStoryMenuExitRequest);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    while (TRUE) {
        if (WipeStatGet() == 0) {
            break;
        }
        WaitOneFrame();
    }
    while (TRUE) {
        if (HuPadBtnDown[0] & PAD_BUTTON_A) {
            break;
        }
        if ((HuPadBtnDown[0] & PAD_BUTTON_DOWN) || HuPadStkY[0] >= 30) {
            selectedEventIndex++;
            if (selectedEventIndex >= 4) {
                selectedEventIndex = 0;
            }
            WaitFrames(5);
        }
        if ((HuPadBtnDown[0] & PAD_BUTTON_UP) || HuPadStkY[0] <= -30) {
            selectedEventIndex--;
            if (selectedEventIndex < 0) {
                selectedEventIndex = 3;
            }
            WaitFrames(5);
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_RIGHT) {
            selectedBoardIndex++;
            if (selectedBoardIndex > 4) {
                selectedBoardIndex = 0;
            }
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_LEFT) {
            selectedBoardIndex--;
            if (selectedBoardIndex < 0) {
                selectedBoardIndex = 4;
            }
        }
        fontcolor = FONT_COLOR_WHITE;
        print8(24, 100, 1.5f, ">>>>> MPGC STORY 1 - EVENT SELECT <<<<<");
        print8(24, 130, 1.5f, "MAP NUMBER : %d", selectedBoardIndex + 1);
        for (i = 0; i < 4; i++) {
            fontcolor = FONT_COLOR_WHITE;
            if (selectedEventIndex == i) {
                fontcolor = FONT_COLOR_CYAN;
            }
            print8(24, 160 + i * 15, 1.5f, "%s", storyEventMenuLabels[i]);
        }
        WaitOneFrame();
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (TRUE) {
        if (WipeStatGet() == 0) {
            break;
        }
        WaitOneFrame();
    }
    GWSystem.board = selectedBoardIndex;
    switch (selectedEventIndex) {
        case 0:
            StartBoardClearResult(storyObjectManager, selectedBoardIndex, 0);
            break;
        case 1:
            StartBoardMissResult(storyObjectManager, selectedBoardIndex, 0);
            break;
        case 2:
            StartMinigameClearResult(storyObjectManager, selectedBoardIndex, 0);
            break;
        case 3:
            StartMinigameMissResult(storyObjectManager, selectedBoardIndex, 0);
            break;
    }
    while (TRUE) {
        WaitOneFrame();
    }
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

Void InitializeStoryOverlay(Void) {
    s32 i;

    storyObjectManager = omInitObjMan(62, 0x2000);
    switch (omovlevtno) {
        case 0:
            for (i = 0; i < 4; i++) {
                if (GWPlayerCfg[i].character == GWStoryCharGet()) {
                    break;
                }
            }
            StartBoardClearResult(storyObjectManager, GWSystem.board, i);
            break;
        case 1:
            for (i = 0; i < 4; i++) {
                if (GWPlayerCfg[i].character == GWStoryCharGet()) {
                    break;
                }
            }
            StartBoardMissResult(storyObjectManager, GWSystem.board, i);
            break;
        case 2:
            if (GWPlayerCoinWinGet(0) >= 10) {
                StartMinigameClearResult(storyObjectManager, GWSystem.board, 0);
            } else {
                StartMinigameMissResult(storyObjectManager, GWSystem.board, 0);
            }
            break;
        case 20:
            StartStorySaveSequence(storyObjectManager);
            break;
    }
}

Void ObjectSetup(Void) {
    InitializeStoryOverlay();
}

float InterpolateLinear(float startValue, float endValue, float elapsedFrames, float durationFrames) {
    if (durationFrames <= elapsedFrames) {
        return endValue;
    }
    return startValue + (elapsedFrames / durationFrames) * (endValue - startValue);
}

float InterpolateWeightedAverage(float startValue, float endValue, float elapsedFrames) {
    return (endValue + startValue * (elapsedFrames - 1.0f)) / elapsedFrames;
}

float InterpolateCosineEaseOut(float startValue, float endValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return startValue + (endValue - startValue) * (1.0 - cosd((90.0f / durationFrames) * elapsedFrames));
}

float InterpolateSineEaseOut(float startValue, float endValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return startValue + (endValue - startValue) * sind((90.0f / durationFrames) * elapsedFrames);
}

float InterpolateSinePulse(float startValue, float endValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return startValue;
    }
    return startValue + (endValue - startValue) * sind((180.0f / durationFrames) * elapsedFrames);
}

float InterpolateSineCycle(float startValue, float endValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return startValue;
    }
    return startValue + (endValue - startValue) * sind((360.0f / durationFrames) * elapsedFrames);
}

Void NoopCallback(Void) {
}

Void WaitOneFrame(Void) {
    HuPrcVSleep();
}

Void WaitFrames(s32 waitFrames) {
    HuPrcSleep(waitFrames);
}

Void EndCurrentProcess(Void) {
    HuPrcEnd();
}

Void InitializeStoryWindows(Void) {
    HuWinInit(1);
}

Void CloseAllStoryWindows(Void) {
    HuWinAllKill();
}

Void AnimateStoryWindow(s32 windowId, float originRatioX, float originRatioY, s32 openWindow) {
    WindowData* windowData;
    float windowOriginX;
    float windowOriginY;
    float windowWidth;
    float windowHeight;
    float openingPositionX;
    float openingScaleX;
    float openingPositionY;
    float openingScaleY;
    float animatedPositionX;
    float animatedPositionY;
    float animatedScaleX;
    float animatedScaleY;
    s32 i;

    windowData = &winData[windowId];
    windowOriginX = windowData->pos_x;
    windowOriginY = windowData->pos_y;
    windowWidth = windowData->w;
    windowHeight = windowData->h;
    openingPositionX = windowOriginX + windowWidth * originRatioX;
    openingScaleX = 4.0f / windowWidth;
    if (openingPositionX + 4.0f - (windowOriginX + windowWidth) >= 0.0f) {
        openingPositionX = (windowOriginX + windowWidth) - 4.0f;
    } else if (windowOriginX - (openingPositionX - 2.0f) >= 0.0f) {
        openingPositionX = windowOriginX;
    } else {
        openingPositionX -= 2.0f;
    }
    openingPositionY = windowOriginY + windowHeight * originRatioY;
    openingScaleY = 4.0f / windowHeight;
    if (openingPositionY + 4.0f - (windowOriginY + windowHeight) >= 0.0f) {
        openingPositionY = (windowOriginY + windowHeight) - 4.0f;
    } else if (windowOriginY - (openingPositionY - 2.0f) >= 0.0f) {
        openingPositionY = windowOriginY;
    } else {
        openingPositionY -= 2.0f;
    }
    if (openWindow != 0) {
        HuWinPosSet(windowId, openingPositionX, openingPositionY);
        HuWinScaleSet(windowId, openingScaleX, openingScaleY);
        animatedPositionX = openingPositionX;
        animatedScaleX = openingScaleX;
        animatedPositionY = openingPositionY;
        animatedScaleY = openingScaleY;
    } else {
        HuWinPosSet(windowId, windowOriginX, windowOriginY);
        HuWinScaleSet(windowId, 1.0f, 1.0f);
        animatedPositionX = windowOriginX;
        animatedScaleX = 1.0f;
        animatedPositionY = windowOriginY;
        animatedScaleY = 1.0f;
    }
    HuWinDispOn(windowId);
    for (i = 0; i <= 15; i++) {
        HuPrcVSleep();
        if (openWindow != 0) {
            if (i <= 10) {
                animatedPositionX = InterpolateSineEaseOut(openingPositionX, windowOriginX, i, 10.0f);
                animatedScaleX = InterpolateSineEaseOut(openingScaleX, 1.0f, i, 10.0f);;
            } else {
                animatedPositionY = InterpolateSineEaseOut(openingPositionY, windowOriginY, i - 10, 5.0f);
                animatedScaleY = InterpolateSineEaseOut(openingScaleY, 1.0f, i - 10, 5.0f);
            }
        } else {
            if (i <= 10) {
                animatedPositionY = InterpolateSineEaseOut(windowOriginY, openingPositionY, i, 10.0f);
                animatedScaleY = InterpolateSineEaseOut(1.0f, openingScaleY, i, 10.0f);
            } else {
                animatedPositionX = InterpolateSineEaseOut(windowOriginX, openingPositionX, i - 10, 5.0f);
                animatedScaleX = InterpolateSineEaseOut(1.0f, openingScaleX, i - 10, 5.0f);
            }
        }
        HuWinPosSet(windowId, animatedPositionX, animatedPositionY);
        HuWinScaleSet(windowId, animatedScaleX, animatedScaleY);
    }
    if (openWindow != 0) {
        HuWinPosSet(windowId, windowOriginX, windowOriginY);
        HuWinScaleSet(windowId, 1.0f, 1.0f);
    } else {
        HuWinPosSet(windowId, openingPositionX, openingPositionY);
        HuWinScaleSet(windowId, openingScaleX, openingScaleY);
    }
    HuPrcVSleep();
}

s32 CreateStoryWindow(s32 windowXMode, s32 windowYMode, s32 windowColumnCount, s32 windowRowCount, float animationOriginX, float animationOriginY, s32 activePlayerIndex) {
    s32 windowX;
    s32 windowY;
    s32 windowId;

    windowColumnCount = windowColumnCount * 21 + 16;
    windowRowCount = windowRowCount * 26 + 16;
    if (windowXMode == 0) {
        windowX = 550 - windowColumnCount;
    } else {
        windowX = 1.0f + (576.0f - windowColumnCount) / 2.0f;
    }
    if (windowYMode == 0) {
        windowY = 430 - windowRowCount;
    } else {
        windowY = windowYMode;
    }
    windowId = HuWinExCreateStyled(windowX, windowY, windowColumnCount, windowRowCount, -1, 1);
    if (activePlayerIndex != -1) {
        activePlayerIndex = GWPlayerCfg[activePlayerIndex].pad_idx;
        winData[windowId].active_pad = 1;
    } else {
        winData[windowId].active_pad = 0xF;
    }
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    AnimateStoryWindow(windowId, animationOriginX, animationOriginY, 1);
    return windowId;
}

s32 CreateCenteredStoryWindow(s32 windowXMode, s32 windowYMode, s32 windowColumnCount, s32 windowRowCount, s32 activePlayerIndex) {
    return CreateStoryWindow(windowXMode, windowYMode, windowColumnCount, windowRowCount, 0.5f, 0.5f, activePlayerIndex);
}

Void CloseStoryWindowAt(s32 windowId, float originRatioX, float originRatioY) {
    HuWinMesSet(windowId, MAKE_MESSID(37, 4));
    HuPrcVSleep();
    AnimateStoryWindow(windowId, originRatioX, originRatioY, 0);
    HuWinExCleanup(windowId);
}

Void CloseCenteredStoryWindow(s32 windowId) {
    CloseStoryWindowAt(windowId, 0.5f, 0.5f);
}

Void WaitForStoryMessage(s32 windowId, s32 waitFrames) {
    s32 i;

    if (waitFrames > 0) {
        HuPrcSleep(waitFrames);
        return;
    }
    HuWinComKeyReset();
    if (winData[windowId].active_pad == 0xF) {
        for (i = 0; i < 10; i++) {
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, GWMessDelayGet());
        }
    }
    HuWinMesWait(windowId);
    HuWinComKeyReset();
}

BOOL WaitForStoryChoice(s32 windowId, s32 choiceIndex) {
    s32 choiceAccepted;
    s32 i;

    HuWinComKeyReset();
    if (winData[windowId].active_pad == 0xF) {
        for (i = 0; i < 10; i++) {
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, GWMessDelayGet());
        }
    }
    choiceAccepted = HuWinChoiceGet(windowId, choiceIndex);
    HuWinComKeyReset();
    if (choiceAccepted != 0) {
        choiceAccepted = FALSE;
    } else {
        choiceAccepted = TRUE;
    }
    return choiceAccepted;
}

BOOL WaitForFirstStoryChoice(s32 windowId) {
    return WaitForStoryChoice(windowId, 0);
}

Void SetStoryMessage(s32 windowId, s32 messageId) {
    HuWinMesSet(windowId, messageId);
}

Void InsertStoryMessage(s32 windowId, s32 messageId, s32 choiceIndex) {
    HuWinInsertMesSet(windowId, messageId, choiceIndex);
}

Void EnableModelVertexMode(s16 objectIndex) {
    HU3DMODEL* modelData;
    HSFMATERIAL* material;
    HSFDATA* modelHierarchy;
    s16 materialCount;
    s32 i;

    modelData = &Hu3DData[objectIndex];
    modelHierarchy = modelData->hsf;
    materialCount = modelHierarchy->materialNum;
    material = modelHierarchy->material;
    for (i = 0; i < materialCount; material++, i++) {
        material->vtxMode = 1;
    }
}

Void SetObjectMotion(OMOBJ* object, s32 modelSlot, s32 motionSlot, s32 blendFrames, s32 motionMode) {
    switch (motionMode) {
        case 0:
            Hu3DMotionShiftSet(object->mdlId[modelSlot], object->mtnId[motionSlot], 0.0f, blendFrames, HU3D_MOTATTR_NONE);
            break;
        case 1:
            Hu3DMotionShiftSet(object->mdlId[modelSlot], object->mtnId[motionSlot], 0.0f, blendFrames, HU3D_MOTATTR_LOOP);
            break;
        case 2:
            Hu3DMotionShiftSet(object->mdlId[modelSlot], object->mtnId[motionSlot], 0.0f, blendFrames, HU3D_MOTATTR_PAUSE);
            break;
    }
}

Void SetObjectMotionIfChanged(OMOBJ* object, s32 modelSlot, s32 motionSlot, s32 blendFrames, s32 motionMode) {
    if (object->mtnId[motionSlot] == Hu3DMotionIDGet(object->mdlId[modelSlot])) {
        return;
    }
    if (object->mtnId[motionSlot] == Hu3DMotionShiftIDGet(object->mdlId[modelSlot])) {
        return;
    }
    SetObjectMotionIfChanged(object, modelSlot, motionSlot, blendFrames, motionMode);
}

Void WaitForObjectMotionEnd(OMOBJ* object, s32 modelSlot, s32 motionSlot, s32 sleepFrames) {
    HuPrcSleep(sleepFrames + 1);
    do {
        HuPrcVSleep();
    } while (object->mtnId[motionSlot] != Hu3DMotionIDGet(object->mdlId[modelSlot]) || !Hu3DMotionEndCheck(object->mdlId[modelSlot]));
}

s32 FindPlayerIndexByGroup(s32 groupId) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (groupId == GWPlayerCfg[i].group) {
            break;
        }
    }
    return i;
}

Void WaitForControllerButton(s32 controllerPort, s32 buttonMask) {
    while (TRUE) {
        if (buttonMask & HuPadBtnDown[controllerPort]) {
            break;
        }
        HuPrcVSleep();
    }
}

Void UpdateStoryDebugCamera(Void) {
    StoryCameraTransitionState* cameraSettings;

    cameraSettings = &storyCameraState;
    if (HuPadBtn[0] & PAD_BUTTON_B) {
        if (HuPadBtn[0] & PAD_TRIGGER_R) {
            cameraSettings->centerPosition.z -= HuPadStkY[0] / 10.0f;
        } else {
            cameraSettings->centerPosition.x += HuPadStkX[0] / 10.0f;
            cameraSettings->centerPosition.y += HuPadStkY[0] / 10.0f;
        }
    }
    if (HuPadBtn[0] & PAD_BUTTON_Y) {
        cameraSettings->rotation.x -= HuPadStkY[0] / 100.0f;
        cameraSettings->rotation.y += HuPadStkX[0] / 100.0f;
        if (cameraSettings->rotation.x < 0.0f) {
            cameraSettings->rotation.x += 360.0f;
        }
        if (cameraSettings->rotation.x >= 360.0f) {
            cameraSettings->rotation.x -= 360.0f;
        }
        if (cameraSettings->rotation.y < 0.0f) {
            cameraSettings->rotation.y += 360.0f;
        }
        if (cameraSettings->rotation.y >= 360.0f) {
            cameraSettings->rotation.y -= 360.0f;
        }
    }
    if (HuPadBtn[0] & PAD_BUTTON_X) {
        cameraSettings->zoom -= HuPadStkY[0] / 10.0f;
    }
    print8(24, 40, 1.5f, "CENTER : %.2f, %.2f, %.2f", cameraSettings->centerPosition.x, cameraSettings->centerPosition.y, cameraSettings->centerPosition.z);
    print8(24, 55, 1.5f, "ROT    : %.2f, %.2f, %.2f", cameraSettings->rotation.x, cameraSettings->rotation.y, cameraSettings->rotation.z);
    print8(24, 70, 1.5f, "ZOOM   : %.2f", cameraSettings->zoom);
}

Void SetStoryCameraCallback(Void (*callback)(Void)) {
    storyCameraState.transitionCallback = callback;
}

Void SetStoryCameraInputCallback(Void (*callback)(Void)) {
    storyCameraState.completionCallback = callback;
}

Void UpdateStoryCamera(OMOBJ* object) {
    StoryCameraTransitionState* cameraSettings;

    cameraSettings = &storyCameraState;
    if (cameraSettings->transitionCallback != NULL) {
        cameraSettings->transitionCallback();
    }
    Center.x = cameraSettings->centerPosition.x;
    Center.y = cameraSettings->centerPosition.y;
    Center.z = cameraSettings->centerPosition.z;
    CRot.x = cameraSettings->rotation.x;
    CRot.y = cameraSettings->rotation.y;
    CRot.z = cameraSettings->rotation.z;
    CZoom = cameraSettings->zoom;
}

Void InitializeStoryCamera(HUPROCESS* process, Void (*callback)(Void)) {
    Hu3DCameraCreate(1);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 42.0f, 0.1f, 10000.0f, 1.2f);
    storyCameraState.transitionCallback = callback;
    storyCameraState.completionCallback = UpdateStoryDebugCamera;
    omAddObjEx(process, 0x7FDA, 0, 0, -1, omOutView);
    omAddObjEx(process, 0x7FD9, 0, 0, -1, UpdateStoryCamera);
}

Void AnimateCharacterPosition(OMOBJ* object, s32 characterIndex, Vec targetPosition, float motionSpeedParameter, float motionDurationParameter, float motionFlags) {
    Vec modelPosition;
    Vec startingModelPosition;
    Vec unreferencedTargetPositionCopy;
    Vec modelRotation;
    float unreferencedMotionParameter;
    float unreferencedMotionParameterFloat;
    float unreferencedMotionParameterPadding;
    s32 unreferencedMotionParameterFrames;
    s32 unreferencedMotionParameterFlags;
    float verticalVelocity;

    startingModelPosition.x = Hu3DData[object->mdlId[characterIndex]].pos.x;
    modelPosition.y = startingModelPosition.y = Hu3DData[object->mdlId[characterIndex]].pos.y;
    startingModelPosition.z = Hu3DData[object->mdlId[characterIndex]].pos.z;
    unreferencedTargetPositionCopy.x = targetPosition.x;
    unreferencedTargetPositionCopy.y = targetPosition.y;
    unreferencedTargetPositionCopy.z = targetPosition.z;
    unreferencedMotionParameterFrames = motionDurationParameter;
    modelRotation.x = Hu3DData[object->mdlId[characterIndex]].rot.x;
    modelRotation.y = Hu3DData[object->mdlId[characterIndex]].rot.y;
    modelRotation.z = Hu3DData[object->mdlId[characterIndex]].rot.z;
    unreferencedMotionParameterPadding = 0.0f;
    unreferencedMotionParameterFloat = motionSpeedParameter;
    unreferencedMotionParameter = 0.0f;
    unreferencedMotionParameterFlags = motionFlags;
    verticalVelocity = -1.1025f;
    Hu3DMotionShiftSet(object->mdlId[1], object->mtnId[3], 0.0f, 5.0f, 0U);
    HuPrcSleep(10);
    while (TRUE) {
        if (verticalVelocity >= 1.7f) {
            verticalVelocity = 1.7f;
        }
        modelPosition.y += verticalVelocity * verticalVelocity * (verticalVelocity >= 0.0f ? -17.5f : 17.5f);
        if (modelPosition.y <= 0.0f && verticalVelocity > 0.0f) {
            modelPosition.y = 0.0f;
        }
        modelPosition.x = startingModelPosition.x += 6.0 * sind(modelRotation.y);
        modelPosition.z = startingModelPosition.z += 6.0 * cosd(modelRotation.y);
        Hu3DModelPosSet(object->mdlId[1], modelPosition.x, modelPosition.y, modelPosition.z);
        Hu3DModelRotSet(object->mdlId[1], modelRotation.x, modelRotation.y, modelRotation.z);
        HuPrcVSleep();
        if (modelPosition.y <= targetPosition.y && verticalVelocity > 0.0f) {
            break;
        }
        verticalVelocity += 0.11f;
    }
}

Void AnimateCharacterRotation(OMOBJ* object, s32 characterIndex, Vec destinationPosition, float targetHeadingDegrees, float turnParameter, float turnDurationFrames) {
    Vec nextModelPosition;
    Vec currentModelPosition;
    Vec targetPosition;
    Vec targetHeading;
    Vec modelRotation;
    Vec targetRotation;
    s32 unreferencedTurnParameter;
    s32 turnDuration;
    s32 reachedTargetX;
    s32 reachedTargetZ;
    s32 targetIsPositiveX;
    s32 targetIsPositiveZ;
    s32 i;

    reachedTargetX = 0;
    reachedTargetZ = 0;
    currentModelPosition.x = Hu3DData[object->mdlId[characterIndex]].pos.x;
    currentModelPosition.y = Hu3DData[object->mdlId[characterIndex]].pos.y;
    currentModelPosition.z = Hu3DData[object->mdlId[characterIndex]].pos.z;
    targetPosition.x = destinationPosition.x;
    targetPosition.y = destinationPosition.y;
    targetPosition.z = destinationPosition.z;
    unreferencedTurnParameter = turnParameter;
    modelRotation.x = Hu3DData[object->mdlId[characterIndex]].rot.x;
    modelRotation.y = Hu3DData[object->mdlId[characterIndex]].rot.y;
    modelRotation.z = Hu3DData[object->mdlId[characterIndex]].rot.z;
    targetRotation.x = 0.0f;
    targetRotation.y = targetHeadingDegrees;
    targetRotation.z = 0.0f;
    if (targetPosition.x - currentModelPosition.x >= 0.0f) {
        targetIsPositiveX = 1;
    } else {
        targetIsPositiveX = 0;
    }
    if (targetPosition.z - currentModelPosition.z >= 0.0f) {
        targetIsPositiveZ = 1;
    } else {
        targetIsPositiveZ = 0;
    }
    turnDuration = turnDurationFrames;
    do {
        targetHeading.y = -(atan2d(targetPosition.z - currentModelPosition.z, targetPosition.x - currentModelPosition.x) - 90.0);
        if (targetHeading.y < 0.0f) {
            targetHeading.y += 360.0f;
        } else if (targetHeading.y >= 360.0f) {
            targetHeading.y -= 360.0f;
        }
        if (targetHeading.y >= 180.0f) {
            if (modelRotation.y - targetHeading.y >= 180.0f) {
                modelRotation.y -= 360.0f;
            }
            if (modelRotation.y - targetHeading.y < -180.0f) {
                modelRotation.y += 360.0f;
            }
        } else {
            if (modelRotation.y - targetHeading.y > 180.0f) {
                modelRotation.y -= 360.0f;
            }
            if (modelRotation.y - targetHeading.y <= -180.0f) {
                modelRotation.y += 360.0f;
            }
        }
        nextModelPosition.x = currentModelPosition.x += 6.0 * sind(targetHeading.y);
        nextModelPosition.z = currentModelPosition.z += 6.0 * cosd(targetHeading.y);
        if (targetIsPositiveX != 0) {
            if (currentModelPosition.x >= targetPosition.x) {
                nextModelPosition.x = currentModelPosition.x = targetPosition.x;
                reachedTargetX = 1;
            }
        } else {
            if (currentModelPosition.x <= targetPosition.x) {
                nextModelPosition.x = currentModelPosition.x = targetPosition.x;
                reachedTargetX = 1;
            }
        }
        if (targetIsPositiveZ != 0) {
            if (currentModelPosition.z >= targetPosition.z) {
                nextModelPosition.z = currentModelPosition.z = targetPosition.z;
                reachedTargetZ = 1;
            }
        } else {
            if (currentModelPosition.z <= targetPosition.z) {
                nextModelPosition.z = currentModelPosition.z = targetPosition.z;
                reachedTargetZ = 1;
            }
        }
        modelRotation.y = InterpolateWeightedAverage(modelRotation.y, targetHeading.y, 10.0f);
        Hu3DModelPosSet(object->mdlId[1], nextModelPosition.x, currentModelPosition.y, nextModelPosition.z);
        Hu3DModelRotSet(object->mdlId[1], modelRotation.x, modelRotation.y, modelRotation.z);
        HuPrcVSleep();
    } while (reachedTargetZ != 1 || reachedTargetX != 1);
    modelRotation.y = Hu3DData[object->mdlId[characterIndex]].rot.y;
    for (i = 0; i <= turnDuration; i++) {
        if (targetRotation.y >= 180.0f) {
            if (modelRotation.y - targetRotation.y >= 180.0f) {
                modelRotation.y -= 360.0f;
            }
            if (modelRotation.y - targetRotation.y < -180.0f) {
                modelRotation.y += 360.0f;
            }
        } else {
            if (modelRotation.y - targetRotation.y > 180.0f) {
                modelRotation.y -= 360.0f;
            }
            if (modelRotation.y - targetRotation.y <= -180.0f) {
                modelRotation.y += 360.0f;
            }
        }
        modelRotation.y = InterpolateLinear(modelRotation.y, targetRotation.y, i, turnDuration);
        Hu3DModelRotSet(object->mdlId[1], modelRotation.x, modelRotation.y, modelRotation.z);
        HuPrcVSleep();
    }
}

Void SetModelMaterialWhite(s16 objectIndex) {
    HU3DMODEL* modelData;
    HSFDATA* modelHierarchy;
    HSFMATERIAL* material;
    s16 materialCount;
    s32 i;

    modelData = &Hu3DData[objectIndex];
    modelHierarchy = modelData->hsf;
    materialCount = modelHierarchy->materialNum;
    material = modelHierarchy->material;
    for (i = 0; i < materialCount; material++, i++) {
        material->vtxMode = 1;
        material->litColor[0] = 0xFF;
        material->litColor[1] = 0xFF;
        material->litColor[2] = 0xFF;
    }
}

Void CapturePlayerResult(StoryPlayerResult* playerResult, s32 playerOrderIndex) {
    playerResult->playerIndex = playerOrderIndex;
    playerResult->group = GWPlayerCfg[playerResult->playerIndex].group;
    playerResult->isComputer = GWPlayerCfg[playerResult->playerIndex].iscom;
    playerResult->difficulty = GWPlayerCfg[playerResult->playerIndex].diff;
    playerResult->characterId = GWPlayerCfg[playerResult->playerIndex].character;
    playerResult->controllerPort = GWPlayerCfg[playerResult->playerIndex].pad_idx;
}

s32 FindFirstHumanPlayer(Void) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (!GWPlayerCfg[i].iscom) {
            break;
        }
    }
    if (i == 4) {
        return -1;
    }
    return i;
}

s32 CreateStoryLighting(s32 lightIntensity) {
    s32 lightId = 0;

    lightId = Hu3DGLightCreate(0.0f, 700.0f, 1200.0f, 0.0f, -1.0f, 0.0f, lightIntensity, lightIntensity, lightIntensity);
    Hu3DGLightPosAimSet(lightId, 0.0f, 700.0f, 1200.0f, 0.0f, 0.0f, 640.0f);
    Hu3DGLightInfinitytSet(lightId);
    return lightId;
}

Void CreateStoryShadow(Void) {
    Vec shadowCameraPosition = { 0.0f, 3000.0f, 800.0f };
    Vec shadowLookAtPosition = { 0.0f, 0.0f, 640.0f };
    Vec shadowUpDirection = { 0.0f, 1.0f, 0.0f };

    Hu3DShadowCreate(45.0f, 2000.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.8f);
    Hu3DShadowPosSet(&shadowCameraPosition, &shadowUpDirection, &shadowLookAtPosition);
}

Void SetCameraToTarget(StoryCameraTransitionState* cameraState) {
    cameraState->centerPosition.x = cameraState->targetCenterX;
    cameraState->centerPosition.y = cameraState->targetCenterY;
    cameraState->centerPosition.z = cameraState->targetCenterZ;
    cameraState->rotation.x = cameraState->targetRotationX;
    cameraState->rotation.y = cameraState->targetRotationY;
    cameraState->rotation.z = cameraState->targetRotationZ;
    cameraState->zoom = cameraState->targetZoom;
}

Void CaptureCameraStartState(StoryCameraTransitionState* cameraState) {
    cameraState->transitionFrame = 0;
    cameraState->targetCenterX = cameraState->centerPosition.x;
    cameraState->targetCenterY = cameraState->centerPosition.y;
    cameraState->targetCenterZ = cameraState->centerPosition.z;
    cameraState->targetRotationX = cameraState->rotation.x;
    cameraState->targetRotationY = cameraState->rotation.y;
    cameraState->targetRotationZ = cameraState->rotation.z;
    cameraState->targetZoom = cameraState->zoom;
}

Void InterpolateCameraCenter(StoryCameraTransitionState* cameraState, StoryCameraTransitionState* targetCameraState, s32 includeZoom, float smoothingWeight) {
    cameraState->centerPosition.x = InterpolateWeightedAverage(cameraState->centerPosition.x, targetCameraState->targetCenterX, smoothingWeight);
    cameraState->centerPosition.y = InterpolateWeightedAverage(cameraState->centerPosition.y, targetCameraState->targetCenterY, smoothingWeight);
    cameraState->centerPosition.z = InterpolateWeightedAverage(cameraState->centerPosition.z, targetCameraState->targetCenterZ, smoothingWeight);
    cameraState->rotation.x = InterpolateWeightedAverage(cameraState->rotation.x, targetCameraState->targetRotationX, smoothingWeight);
    cameraState->rotation.y = InterpolateWeightedAverage(cameraState->rotation.y, targetCameraState->targetRotationY, smoothingWeight);
    cameraState->rotation.z = InterpolateWeightedAverage(cameraState->rotation.z, targetCameraState->targetRotationZ, smoothingWeight);
    if (includeZoom != 0) {
        cameraState->zoom = InterpolateWeightedAverage(cameraState->zoom, targetCameraState->targetZoom, smoothingWeight);
    }
}

Void InterpolateCameraCenterSine(StoryCameraTransitionState* cameraState, float smoothingWeight) {
    InterpolateCameraCenter(cameraState, cameraState, 1, smoothingWeight);
}

Void InterpolateCameraCenterCosine(StoryCameraTransitionState* cameraState, float smoothingWeight) {
    InterpolateCameraCenter(cameraState, cameraState, 0, smoothingWeight);
}

Void InterpolateCameraRotation(StoryCameraTransitionState* cameraState, StoryCameraTransitionState* targetCameraState, float elapsedFrames, float durationFrames) {
    targetCameraState->targetCenterX = InterpolateLinear(cameraState->targetCenterX, targetCameraState->centerPosition.x, elapsedFrames, durationFrames);
    targetCameraState->targetCenterY = InterpolateLinear(cameraState->targetCenterY, targetCameraState->centerPosition.y, elapsedFrames, durationFrames);
    targetCameraState->targetCenterZ = InterpolateLinear(cameraState->targetCenterZ, targetCameraState->centerPosition.z, elapsedFrames, durationFrames);
    targetCameraState->targetRotationX = InterpolateLinear(cameraState->targetRotationX, targetCameraState->rotation.x, elapsedFrames, durationFrames);
    targetCameraState->targetRotationY = InterpolateLinear(cameraState->targetRotationY, targetCameraState->rotation.y, elapsedFrames, durationFrames);
    targetCameraState->targetRotationZ = InterpolateLinear(cameraState->targetRotationZ, targetCameraState->rotation.z, elapsedFrames, durationFrames);
    targetCameraState->targetZoom = InterpolateLinear(cameraState->targetZoom, targetCameraState->zoom, elapsedFrames, durationFrames);
}

Void InterpolateCameraRotationSine(StoryCameraTransitionState* cameraState, StoryCameraTransitionState* targetCameraState, float elapsedFrames, float durationFrames) {
    InterpolateCameraRotation(cameraState, targetCameraState, elapsedFrames, durationFrames);
    cameraState->centerPosition.x = targetCameraState->targetCenterX;
    cameraState->centerPosition.y = targetCameraState->targetCenterY;
    cameraState->centerPosition.z = targetCameraState->targetCenterZ;
    cameraState->rotation.x = targetCameraState->targetRotationX;
    cameraState->rotation.y = targetCameraState->targetRotationY;
    cameraState->rotation.z = targetCameraState->targetRotationZ;
    cameraState->zoom = targetCameraState->targetZoom;
}

Void InterpolateCameraRotationCosine(StoryCameraTransitionState* cameraState, StoryCameraTransitionState* targetCameraState, float elapsedFrames, float durationFrames, float secondaryDurationFrames) {
    InterpolateCameraRotation(cameraState, targetCameraState, elapsedFrames, durationFrames);
    InterpolateCameraCenter(cameraState, targetCameraState, 1, secondaryDurationFrames);
}

Void InterpolateCameraZoom(StoryCameraTransitionState* cameraState, StoryCameraTransitionState* targetCameraState, float elapsedFrames, float durationFrames) {
    targetCameraState->targetCenterX = InterpolateSineEaseOut(cameraState->targetCenterX, targetCameraState->centerPosition.x, elapsedFrames, durationFrames);
    targetCameraState->targetCenterY = InterpolateSineEaseOut(cameraState->targetCenterY, targetCameraState->centerPosition.y, elapsedFrames, durationFrames);
    targetCameraState->targetCenterZ = InterpolateSineEaseOut(cameraState->targetCenterZ, targetCameraState->centerPosition.z, elapsedFrames, durationFrames);
    targetCameraState->targetRotationX = InterpolateSineEaseOut(cameraState->targetRotationX, targetCameraState->rotation.x, elapsedFrames, durationFrames);
    targetCameraState->targetRotationY = InterpolateSineEaseOut(cameraState->targetRotationY, targetCameraState->rotation.y, elapsedFrames, durationFrames);
    targetCameraState->targetRotationZ = InterpolateSineEaseOut(cameraState->targetRotationZ, targetCameraState->rotation.z, elapsedFrames, durationFrames);
    targetCameraState->targetZoom = InterpolateSineEaseOut(cameraState->targetZoom, targetCameraState->zoom, elapsedFrames, durationFrames);
}

Void InterpolateCameraZoomSine(StoryCameraTransitionState* cameraState, StoryCameraTransitionState* targetCameraState, float elapsedFrames, float durationFrames) {
    InterpolateCameraZoom(cameraState, targetCameraState, elapsedFrames, durationFrames);
    cameraState->centerPosition.x = targetCameraState->targetCenterX;
    cameraState->centerPosition.y = targetCameraState->targetCenterY;
    cameraState->centerPosition.z = targetCameraState->targetCenterZ;
    cameraState->rotation.x = targetCameraState->targetRotationX;
    cameraState->rotation.y = targetCameraState->targetRotationY;
    cameraState->rotation.z = targetCameraState->targetRotationZ;
    cameraState->zoom = targetCameraState->targetZoom;
}

Void InterpolateCameraTransition(StoryCameraTransitionState* cameraState, StoryCameraTransitionState* targetCameraState, float elapsedFrames, float durationFrames, float secondaryDurationFrames) {
    InterpolateCameraZoom(cameraState, targetCameraState, elapsedFrames, durationFrames);
    InterpolateCameraCenter(cameraState, targetCameraState, 1, secondaryDurationFrames);
}

Void InterpolateCameraTransitionSine(StoryCameraTransitionState* cameraState, StoryCameraTransitionState* targetCameraState, float elapsedFrames, float durationFrames) {
    targetCameraState->targetCenterX = InterpolateCosineEaseOut(cameraState->targetCenterX, targetCameraState->centerPosition.x, elapsedFrames, durationFrames);
    targetCameraState->targetCenterY = InterpolateCosineEaseOut(cameraState->targetCenterY, targetCameraState->centerPosition.y, elapsedFrames, durationFrames);
    targetCameraState->targetCenterZ = InterpolateCosineEaseOut(cameraState->targetCenterZ, targetCameraState->centerPosition.z, elapsedFrames, durationFrames);
    targetCameraState->targetRotationX = InterpolateCosineEaseOut(cameraState->targetRotationX, targetCameraState->rotation.x, elapsedFrames, durationFrames);
    targetCameraState->targetRotationY = InterpolateCosineEaseOut(cameraState->targetRotationY, targetCameraState->rotation.y, elapsedFrames, durationFrames);
    targetCameraState->targetRotationZ = InterpolateCosineEaseOut(cameraState->targetRotationZ, targetCameraState->rotation.z, elapsedFrames, durationFrames);
    targetCameraState->targetZoom = InterpolateCosineEaseOut(cameraState->targetZoom, targetCameraState->zoom, elapsedFrames, durationFrames);
}

Void InterpolateCameraTransitionCosine(StoryCameraTransitionState* cameraState, StoryCameraTransitionState* targetCameraState, float elapsedFrames, float durationFrames) {
    InterpolateCameraTransitionSine(cameraState, targetCameraState, elapsedFrames, durationFrames);
    cameraState->centerPosition.x = targetCameraState->targetCenterX;
    cameraState->centerPosition.y = targetCameraState->targetCenterY;
    cameraState->centerPosition.z = targetCameraState->targetCenterZ;
    cameraState->rotation.x = targetCameraState->targetRotationX;
    cameraState->rotation.y = targetCameraState->targetRotationY;
    cameraState->rotation.z = targetCameraState->targetRotationZ;
    cameraState->zoom = targetCameraState->targetZoom;
}

Void InterpolateCameraTransitionEaseOut(StoryCameraTransitionState* cameraState, StoryCameraTransitionState* targetCameraState, float elapsedFrames, float durationFrames, float secondaryDurationFrames) {
    InterpolateCameraTransitionSine(cameraState, targetCameraState, elapsedFrames, durationFrames);
    InterpolateCameraCenter(cameraState, targetCameraState, 1, secondaryDurationFrames);
}

Void CreateStoryStageModels(OMOBJ* object) {
    s32 nextMotionIndex;

    if (object->work[0] == 0 && object->work[2] != 0) {
        object->work[2]--;
        return;
    }
    if (object->work[2] == 0 && Hu3DMotionEndCheck(object->mdlId[1])) {
        nextMotionIndex = object->work[1];
        Hu3DMotionShiftSet(object->mdlId[1], object->mtnId[nextMotionIndex], 0.0f, 15.0f, 0x40000001U);
        object->work[0] = object->work[1] = object->work[2] = 0;
    }
}

Void SetObjectMotionAndWait(OMOBJ* object, s32 motionIndex, s32 blendFrames, s32 loopMode, s32 initialDelayFrames) {
    SetObjectMotion(object, 1, motionIndex, loopMode, initialDelayFrames);
    if (initialDelayFrames == 0) {
        object->work[0] = 1;
        object->work[1] = blendFrames;
        object->work[2] = loopMode + 1;
    }
}

Void SetStoryWindowMessage(s32 windowId, s32 messageId) {
    HuWinMesSet(windowId, messageId);
}

Void InsertStoryWindowMessage(s32 windowId, s32 messageId, s32 choiceIndex) {
    HuWinInsertMesSet(windowId, messageId, choiceIndex);
}

Void WaitForStoryWindow(s32 windowId, s32 messageId) {
    WaitForStoryMessage(windowId, messageId);
}

Void DisplayStoryMessagePages(s32 windowId, s32 messageId, s32 choiceIndex, s32 waitFrames) {
    s32 messagePageCount = 0;
    s32 pageWaitState = 0;
    s32 i;

    messagePageCount = HuWinKeyWaitNumGet(messageId);
    if (choiceIndex == -1 || choiceIndex >= messagePageCount - 1) {
        choiceIndex = messagePageCount - 1;
    }
    HuWinComKeyReset();
    if (winData[windowId].active_pad == 0xF) {
        for (i = 0; i < 10; i++) {
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, GWMessDelayGet());
        }
    }
    HuWinMesSet(windowId, messageId);
    for (i = 0; i < messagePageCount; i++) {
        pageWaitState = 0;
        do {
            HuPrcVSleep();
        } while (HuWinStatGet(windowId) != 1);
        if (i == choiceIndex) {
            HuWinAttrSet(windowId, 0x400);
            pageWaitState = 1;
        }
        while (TRUE) {
            HuPrcVSleep();
            if (pageWaitState != 0) {
                if (pageWaitState == 1 && HuWinStatGet(windowId) != 1) {
                    pageWaitState = 2;
                } else if (pageWaitState == 2) {
                    HuPrcSleep(waitFrames);
                    HuWinAttrReset(windowId, 0x400);
                    HuWinKeyWaitEntry(windowId);
                    break;
                }
            } else if (HuWinStatGet(windowId) != 1) {
                break;
            }
        }
    }
    WaitForStoryMessage(windowId, 0);
    HuWinMesSet(windowId, MAKE_MESSID(37, 4));
    HuWinComKeyReset();
}

BOOL WaitForFirstStoryChoice(s32 windowId) {
    return WaitForStoryChoice(windowId, 0);
}

s32 CreateStoryChoiceWindow(s32 messageId, s32 playerIndex) {
    float messageSize[2];
    float messageWidth;
    float messageHeight;
    float windowPositionX;
    float windowPositionY;
    s32 windowId;

    HuWinMesMaxSizeGet(1, messageSize, messageId);
    messageWidth = messageSize[0];
    messageHeight = messageSize[1];
    windowPositionX = 550.0f - messageWidth;
    windowPositionY = 430.0f - messageHeight;
    windowId = HuWinExCreateStyled(windowPositionX, windowPositionY, messageWidth, messageHeight, -1, 1);
    if (playerIndex != -1) {
        playerIndex = GWPlayerCfg[playerIndex].pad_idx;
        winData[windowId].active_pad = 1;
    } else {
        winData[windowId].active_pad = 0xF;
    }
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    AnimateStoryWindow(windowId, 0.5f, 0.5f, 1);
    return windowId;
}

Void CloseStoryChoiceWindow(s32 windowId) {
    CloseStoryWindowAt(windowId, 0.5f, 0.5f);
}
