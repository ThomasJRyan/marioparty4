#include "REL/mstory2Dll.h"
#include "game/disp.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/gamework.h"
#include "game/thpmain.h"
#endif

#include "ext_math.h"

s32 unreferencedStoryState[3]; // unused
StoryCameraMotionState storyCameraState;
s32 unreferencedStoryScratch[8];
HUPROCESS* storyObjectManager;

s32 storyAudioCueIds[][4] = {
    { 0x37, 0x38, 0x36, 0x39 },
    { 0x43, 0x45, 0x44, 0x43 },
    { 0x40, 0x42, 0x41, 0x40 },
    { 0x4B, 0x4D, 0x4C, 0x4B },
    { 0x46, 0x48, 0x47, 0x46 },
    { 0x3E, 0x3F, 0x3E, 0x3E },
    {0x3A, 0x3B, 0x3C, 0xFFFFFFFF}
};

char* storyEventMenuLabels[] = {
    "K-1 : KOOPA STORY - BOARD ENTRANCE EVENT",
    "K-3 : KOOPA STORY - BOARD CLEAR EVENT",
    "K-4 : KOOPA STORY - BOARD MISS EVENT",
    "K-5 : KOOPA STORY - MINIGAME CLEAR EVENT",
    "K-6 : KOOPA STORY - MINIGAME MISS EVENT",
    "--- : ENDING"
};

Void ReturnOverlayAfterWipe(OMOBJ* object) {
    if (WipeStatGet() == 0) {
        object->objFunc = NULL;
        omOvlReturnEx(1, 1);
    }
}

Void HandleOverlayExitRequest(OMOBJ* object) {
    if (omSysExitReq != 0) {
        object->objFunc = ReturnOverlayAfterWipe;
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    }
}

Void RunStoryEventSelection(Void) {
    s32 unreferencedLocal14;
    s32 selectedEventIndex;
    s32 unreferencedLocalRegister;
    s32 unreferencedLocal10;
    s32 unreferencedLocalC;
    s32 unreferencedLocal8;
    s32 i;

    selectedEventIndex = 0;
    omAddObjEx(storyObjectManager, 0x2000, 0, 0, -1, HandleOverlayExitRequest);
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
        if (HuPadStkY[0] <= -30) {
            WaitFrames(5);
            selectedEventIndex++;
            if (selectedEventIndex >= 6) {
                selectedEventIndex = 0;
            }
            
        }
        if (HuPadStkY[0] >= 30) {
            WaitFrames(5);
            selectedEventIndex--;
            if (selectedEventIndex < 0) {
                selectedEventIndex = 5;
            }
        }
        fontcolor = FONT_COLOR_WHITE;
        print8(24, 100, 1.5f, ">>>>> MPGC STORY 2 - EVENT SELECT <<<<<");
        for (i = 0; i < 6; i++) {
            fontcolor = FONT_COLOR_WHITE;
            if (selectedEventIndex == i) {
                fontcolor = FONT_COLOR_CYAN;
            }
            print8(24, 130 + i * 15, 1.5f, "%s", storyEventMenuLabels[i]);
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
    GWSystem.board = 5;
    switch (selectedEventIndex) {
        case 0:
            StartBoardEntranceEvent(storyObjectManager, 0);
            break;
        case 1:
            StartBoardClearEvent(storyObjectManager, 0);
            break;
        case 2:
            StartBoardMissEvent(storyObjectManager, 0);
            break;
        case 3:
            StartMinigameClearEvent(storyObjectManager, 0);
            break;
        case 4:
            StartMinigameMissEvent(storyObjectManager, 0);
            break;
        case 5:
            StartStoryEnding(storyObjectManager, 0);
            break;
    }
    while (TRUE) {
        WaitOneFrame();
    }
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

Void PlayStoryEndingMovie(Void)
{
    int gid = HuSprGrpCreate(1);
    int sprid = HuTHPSprCreate("movie/endmov_ma0.thp", FALSE, 10);
    HuSprGrpMemberSet(gid, 0, sprid);
    HuSprPosSet(gid, 0, 288, 240);
    HuPrcSleep(5);
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 10);
    while(WipeStatGet()) {
        HuPrcVSleep();
    }
    while(!HuTHPEndCheck()) {
        HuPrcVSleep();
    }
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 30);
    while(WipeStatGet()) {
        HuPrcVSleep();
    }
    HuTHPClose();
    HuPrcVSleep();
    HuSprGrpKill(gid);
    while(1) {
        WaitOneFrame();
    }
}

Void InitializeStoryOverlay(Void) {
    s32 i;

    storyObjectManager = omInitObjMan(62, 0x2000);
    switch (omovlevtno) {
        case 0:
            StartBoardEntranceEvent(storyObjectManager, 0);
            break;
            
        case 1:
            for (i = 0; i < 4; i++) {
                if (GWPlayerCfg[i].character == GWStoryCharGet()) {
                    break;
                }
            }
            StartBoardClearEvent(storyObjectManager, i);
            break;
        case 2:
            for (i = 0; i < 4; i++) {
                if (GWPlayerCfg[i].character == GWStoryCharGet()) {
                    break;
                }
            }
            StartBoardMissEvent(storyObjectManager, i);
            break;
        case 3:
            if (GWPlayerCoinWinGet(0) >= 10) {
                StartMinigameClearEvent(storyObjectManager, 0);
            } else {
                StartMinigameMissEvent(storyObjectManager, 0);
            }
            break;
        case 4:
            StartStoryEnding(storyObjectManager, 0);
            break;
            
        case 20:
            StartStorySaveProcess(storyObjectManager);
            break;
    }
}

Void ObjectSetup(Void) {
    InitializeStoryOverlay();
}

float InterpolateLinear(float startValue, float targetValue, float elapsedFrames, float durationFrames) {
    if (durationFrames <= elapsedFrames) {
        return targetValue;
    }
    return startValue + (elapsedFrames / durationFrames) * (targetValue - startValue);
}

float InterpolateWeightedAverage(float currentValue, float targetValue, float sampleCount) {
    return (targetValue + currentValue * (sampleCount - 1.0f)) / sampleCount;
}

float InterpolateCosineEaseOut(float startValue, float targetValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return targetValue;
    }
    return startValue + (targetValue - startValue) * (1.0 - cosd((90.0f / durationFrames) * elapsedFrames));
}

float InterpolateSineEaseIn(float startValue, float targetValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return targetValue;
    }
    return startValue + (targetValue - startValue) * sind((90.0f / durationFrames) * elapsedFrames);
}

float InterpolateSinePulse(float startValue, float targetValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return startValue;
    }
    return startValue + (targetValue - startValue) * sind((180.0f / durationFrames) * elapsedFrames);
}

float InterpolateSineCycle(float startValue, float targetValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return startValue;
    }
    return startValue + (targetValue - startValue) * sind((360.0f / durationFrames) * elapsedFrames);
}

Void StoryNoOpCallback(Void) {
}

Void WaitOneFrame(Void) {
    HuPrcVSleep();
}

Void WaitFrames(s32 frameCount) {
    HuPrcSleep(frameCount);
}

Void EndCurrentProcess(Void) {
    HuPrcEnd();
}

Void InitializeWindowSystem(Void) {
    HuWinInit(1);
}

Void StoryNoOpCallback2(Void) {
}

Void AnimateWindowSlide(s32 windowId, float horizontalAnchor, float verticalAnchor, s32 opening) {
    WindowData* windowData;
    float windowLeft;
    float windowTop;
    float windowWidth;
    float windowHeight;
    float targetLeft;
    float targetScaleX;
    float targetTop;
    float targetScaleY;
    float currentLeft;
    float currentTop;
    float currentScaleX;
    float currentScaleY;
    s32 i;

    windowData = &winData[windowId];
    windowLeft = windowData->pos_x;
    windowTop = windowData->pos_y;
    windowWidth = windowData->w;
    windowHeight = windowData->h;
    targetLeft = windowLeft + windowWidth * horizontalAnchor;
    targetScaleX = 4.0f / windowWidth;
    if (targetLeft + 4.0f - (windowLeft + windowWidth) >= 0.0f) {
        targetLeft = (windowLeft + windowWidth) - 4.0f;
    } else if (windowLeft - (targetLeft - 2.0f) >= 0.0f) {
        targetLeft = windowLeft;
    } else {
        targetLeft -= 2.0f;
    }
    targetTop = windowTop + windowHeight * verticalAnchor;
    targetScaleY = 4.0f / windowHeight;
    if (targetTop + 4.0f - (windowTop + windowHeight) >= 0.0f) {
        targetTop = (windowTop + windowHeight) - 4.0f;
    } else if (windowTop - (targetTop - 2.0f) >= 0.0f) {
        targetTop = windowTop;
    } else {
        targetTop -= 2.0f;
    }
    if (opening != 0) {
        HuWinPosSet(windowId, targetLeft, targetTop);
        HuWinScaleSet(windowId, targetScaleX, targetScaleY);
        currentLeft = targetLeft;
        currentScaleX = targetScaleX;
        currentTop = targetTop;
        currentScaleY = targetScaleY;
    } else {
        HuWinPosSet(windowId, windowLeft, windowTop);
        HuWinScaleSet(windowId, 1.0f, 1.0f);
        currentLeft = windowLeft;
        currentScaleX = 1.0f;
        currentTop = windowTop;
        currentScaleY = 1.0f;
    }
    HuWinDispOn(windowId);
    for (i = 0; i <= 15; i++) {
        HuPrcVSleep();
        if (opening != 0) {
            if (i <= 10) {
                currentLeft = InterpolateSineEaseIn(targetLeft, windowLeft, i, 10.0f);
                currentScaleX = InterpolateSineEaseIn(targetScaleX, 1.0f, i, 10.0f);;
            } else {
                currentTop = InterpolateSineEaseIn(targetTop, windowTop, i - 10, 5.0f);
                currentScaleY = InterpolateSineEaseIn(targetScaleY, 1.0f, i - 10, 5.0f);
            }
        } else {
            if (i <= 10) {
                currentTop = InterpolateSineEaseIn(windowTop, targetTop, i, 10.0f);
                currentScaleY = InterpolateSineEaseIn(1.0f, targetScaleY, i, 10.0f);
            } else {
                currentLeft = InterpolateSineEaseIn(windowLeft, targetLeft, i - 10, 5.0f);
                currentScaleX = InterpolateSineEaseIn(1.0f, targetScaleX, i - 10, 5.0f);
            }
        }
        HuWinPosSet(windowId, currentLeft, currentTop);
        HuWinScaleSet(windowId, currentScaleX, currentScaleY);
    }
    if (opening != 0) {
        HuWinPosSet(windowId, windowLeft, windowTop);
        HuWinScaleSet(windowId, 1.0f, 1.0f);
    } else {
        HuWinPosSet(windowId, targetLeft, targetTop);
        HuWinScaleSet(windowId, targetScaleX, targetScaleY);
    }
    HuPrcVSleep();
}

s32 CreateMessageWindowAt(s32 left, s32 top, s32 widthCells, s32 heightCells, float horizontalAnchor, float verticalAnchor, s32 padPlayerIndex) {
    s32 windowId;

    widthCells = widthCells * 21 + 16;
    heightCells = heightCells * 26 + 16;
    if (left == 0) {
        left = 550 - widthCells;
    } else {
        left = 1.0f + (576.0f - widthCells) / 2.0f;
    }
    if (top == 0) {
        top = 430 - heightCells;
    }
    windowId = HuWinExCreateStyled(left, top, widthCells, heightCells, -1, 1);
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    if (padPlayerIndex != -1) {
        padPlayerIndex = GWPlayerCfg[padPlayerIndex].pad_idx;
        winData[windowId].active_pad = (1 << padPlayerIndex);
    } else {
        winData[windowId].active_pad = 0xF;
    }
    
    AnimateWindowSlide(windowId, horizontalAnchor, verticalAnchor, 1);
    return windowId;
}

s32 CreateCenteredMessageWindow(s32 left, s32 top, s32 widthCells, s32 heightCells, s32 padPlayerIndex) {
    return CreateMessageWindowAt(left, top, widthCells, heightCells, 0.5f, 0.5f, padPlayerIndex);
}

Void CloseMessageWindowAtAnchor(s32 windowId, float horizontalAnchor, float verticalAnchor) {
    HuWinMesSet(windowId, MAKE_MESSID(37, 4));
    HuPrcVSleep();
    AnimateWindowSlide(windowId, horizontalAnchor, verticalAnchor, 0);
    HuWinExCleanup(windowId);
}

Void CloseCenteredMessageWindow(s32 windowId) {
    CloseMessageWindowAtAnchor(windowId, 0.5f, 0.5f);
}

Void WaitForWindowMessage(s32 windowId, s32 delayFrames) {
    s32 i;

    if (delayFrames > 0) {
        HuPrcSleep(delayFrames);
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

BOOL GetWindowChoice(s32 windowId, s32 defaultChoice) {
    s32 choice;
    s32 i;

    HuWinComKeyReset();
    if (winData[windowId].active_pad == 0xF) {
        for (i = 0; i < 10; i++) {
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, GWMessDelayGet());
        }
    }
    choice = HuWinChoiceGet(windowId, defaultChoice);
    HuWinComKeyReset();
    if (choice != 0) {
        choice = FALSE;
    } else {
        choice = TRUE;
    }
    return choice;
}

BOOL GetDefaultWindowChoice(s32 windowId) {
    return GetWindowChoice(windowId, 0);
}

Void SetWindowMessage(s32 windowId, s32 messageId) {
    HuWinMesSet(windowId, messageId);
}

Void InsertWindowMessage(s32 windowId, s32 messageId, s32 insertIndex) {
    HuWinInsertMesSet(windowId, messageId, insertIndex);
}

Void SetModelMaterialsOpaque(s16 modelId) {
    HU3DMODEL* model;
    HSFMATERIAL* material;
    HSFDATA* modelData;
    s16 materialCount;
    s32 i;

    model = &Hu3DData[modelId];
    modelData = model->hsf;
    materialCount = modelData->materialNum;
    material = modelData->material;
    for (i = 0; i < materialCount; material++, i++) {
        material->vtxMode = 1;
    }
}

Void ShiftObjectMotion(OMOBJ* object, s32 modelSlot, s32 motionSlot, s32 blendFrames, s32 motionFlags) {
    switch (motionFlags) {
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
    object->work[0] = object->work[1] = object->work[2] = 0;
}

Void WaitForObjectMotionChange(OMOBJ* object, s32 modelSlot, s32 motionSlot, s32 blendFrames, s32 motionFlags) {
    if (object->mtnId[motionSlot] == Hu3DMotionIDGet(object->mdlId[modelSlot])) {
        return;
    }
    if (object->mtnId[motionSlot] == Hu3DMotionShiftIDGet(object->mdlId[modelSlot])) {
        return;
    }
    WaitForObjectMotionChange(object, modelSlot, motionSlot, blendFrames, motionFlags);
}

Void WaitForObjectMotionEnd(OMOBJ* object, s32 modelSlot, s32 motionSlot, s32 initialDelayFrames) {
    HuPrcSleep(initialDelayFrames + 1);
    do {
        HuPrcVSleep();
    } while (object->mtnId[motionSlot] != Hu3DMotionIDGet(object->mdlId[modelSlot]) || !Hu3DMotionEndCheck(object->mdlId[modelSlot]));
}

s32 FindFirstPlayerInGroup(s32 playerGroup) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (playerGroup == GWPlayerCfg[i].group) {
            break;
        }
    }
    return i;
}

Void WaitForPadButton(s32 padIndex, s32 buttonMask) {
    while (TRUE) {
        if (buttonMask & HuPadBtnDown[padIndex]) {
            break;
        }
        HuPrcVSleep();
    }
}

Void UpdateDebugCamera(Void) {
    StoryCameraMotionState* cameraState;

    cameraState = &storyCameraState;
    if (HuPadBtn[0] & PAD_BUTTON_B) {
        if (HuPadBtn[0] & PAD_TRIGGER_R) {
            cameraState->cameraCenter.z -= HuPadStkY[0] / 10.0f;
        } else {
            cameraState->cameraCenter.x += HuPadStkX[0] / 10.0f;
            cameraState->cameraCenter.y += HuPadStkY[0] / 10.0f;
        }
    }
    if (HuPadBtn[0] & PAD_BUTTON_Y) {
        cameraState->cameraRotation.x -= HuPadStkY[0] / 100.0f;
        cameraState->cameraRotation.y += HuPadStkX[0] / 100.0f;
        if (cameraState->cameraRotation.x < 0.0f) {
            cameraState->cameraRotation.x += 360.0f;
        }
        if (cameraState->cameraRotation.x >= 360.0f) {
            cameraState->cameraRotation.x -= 360.0f;
        }
        if (cameraState->cameraRotation.y < 0.0f) {
            cameraState->cameraRotation.y += 360.0f;
        }
        if (cameraState->cameraRotation.y >= 360.0f) {
            cameraState->cameraRotation.y -= 360.0f;
        }
    }
    if (HuPadBtn[0] & PAD_BUTTON_X) {
        cameraState->cameraZoom -= HuPadStkY[0] / 10.0f;
    }
    print8(24, 40, 1.5f, "CENTER : %.2f, %.2f, %.2f", cameraState->cameraCenter.x, cameraState->cameraCenter.y, cameraState->cameraCenter.z);
    print8(24, 55, 1.5f, "ROT    : %.2f, %.2f, %.2f", cameraState->cameraRotation.x, cameraState->cameraRotation.y, cameraState->cameraRotation.z);
    print8(24, 70, 1.5f, "ZOOM   : %.2f", cameraState->cameraZoom);
}

Void SetCameraUpdateCallback(Void (*callback)(Void)) {
    storyCameraState.cameraUpdateCallback = callback;
}

Void SetCameraResetCallback(Void (*callback)(Void)) {
    storyCameraState.cameraResetCallback = callback;
}

Void ApplyStoryCamera(OMOBJ* object) {
    StoryCameraMotionState* cameraState;

    cameraState = &storyCameraState;
    if (cameraState->cameraUpdateCallback != NULL) {
        cameraState->cameraUpdateCallback();
    }
    Center.x = cameraState->cameraCenter.x;
    Center.y = cameraState->cameraCenter.y;
    Center.z = cameraState->cameraCenter.z;
    CRot.x = cameraState->cameraRotation.x;
    CRot.y = cameraState->cameraRotation.y;
    CRot.z = cameraState->cameraRotation.z;
    CZoom = cameraState->cameraZoom;
}

Void InitializeStoryCamera(HUPROCESS* parentProcess, Void (*cameraUpdateCallback)(Void)) {
    Hu3DCameraCreate(1);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 42.0f, 0.1f, 10000.0f, 1.2f);
    storyCameraState.cameraUpdateCallback = cameraUpdateCallback;
    storyCameraState.cameraResetCallback = UpdateDebugCamera;
    omAddObjEx(parentProcess, 0x7FDA, 0, 0, -1, omOutView);
    omAddObjEx(parentProcess, 0x7FD9, 0, 0, -1, ApplyStoryCamera);
}

Void MoveModelWithJump(OMOBJ* object, s32 modelSlot, Vec targetPosition, float rotationDegrees, float initialWaitFrames, float landingWaitFrames) {
    Vec movingPosition;
    Vec currentPosition;
    Vec targetPosition;
    Vec modelRotation;
    float unreferencedLocal24;
    float unreferencedLocal20;
    float unreferencedLocal1C;
    s32 unreferencedLocal18;
    s32 unreferencedLocal14;
    float verticalVelocity;

    currentPosition.x = Hu3DData[object->mdlId[modelSlot]].pos.x;
    movingPosition.y = currentPosition.y = Hu3DData[object->mdlId[modelSlot]].pos.y;
    currentPosition.z = Hu3DData[object->mdlId[modelSlot]].pos.z;
    targetPosition.x = targetPosition.x;
    targetPosition.y = targetPosition.y;
    targetPosition.z = targetPosition.z;
    unreferencedLocal18 = initialWaitFrames;
    modelRotation.x = Hu3DData[object->mdlId[modelSlot]].rot.x;
    modelRotation.y = Hu3DData[object->mdlId[modelSlot]].rot.y;
    modelRotation.z = Hu3DData[object->mdlId[modelSlot]].rot.z;
    unreferencedLocal1C = 0.0f;
    unreferencedLocal20 = rotationDegrees;
    unreferencedLocal24 = 0.0f;
    unreferencedLocal14 = landingWaitFrames;
    verticalVelocity = -1.1025f;
    Hu3DMotionShiftSet(object->mdlId[1], object->mtnId[3], 0.0f, 5.0f, 0U);
    object->work[0] = object->work[1] = object->work[2] = 0;
    HuPrcSleep(10);
    while (TRUE) {
        if (verticalVelocity >= 1.7f) {
            verticalVelocity = 1.7f;
        }
        movingPosition.y += verticalVelocity * verticalVelocity * (verticalVelocity >= 0.0f ? -17.5f : 17.5f);
        if (movingPosition.y <= 0.0f && verticalVelocity > 0.0f) {
            movingPosition.y = 0.0f;
        }
        movingPosition.x = currentPosition.x += 6.0 * sind(modelRotation.y);
        movingPosition.z = currentPosition.z += 6.0 * cosd(modelRotation.y);
        Hu3DModelPosSet(object->mdlId[1], movingPosition.x, movingPosition.y, movingPosition.z);
        Hu3DModelRotSet(object->mdlId[1], modelRotation.x, modelRotation.y, modelRotation.z);
        HuPrcVSleep();
        if (movingPosition.y <= targetPosition.y && verticalVelocity > 0.0f) {
            break;
        }
        verticalVelocity += 0.11f;
    }
}

Void MoveModelToPosition(OMOBJ* object, s32 modelSlot, Vec targetPosition, float rotationDegrees, float rotationDuration, float finalDuration) {
    Vec movingPosition;
    Vec currentPosition;
    Vec targetPosition;
    Vec targetHeading;
    Vec modelRotation;
    Vec targetRotation;
    s32 unreferencedLocal14;
    s32 rotationDuration;
    s32 reachedTargetX;
    s32 reachedTargetZ;
    s32 movingTowardTargetX;
    s32 movingTowardTargetZ;
    s32 i;

    reachedTargetX = 0;
    reachedTargetZ = 0;
    currentPosition.x = Hu3DData[object->mdlId[modelSlot]].pos.x;
    currentPosition.y = Hu3DData[object->mdlId[modelSlot]].pos.y;
    currentPosition.z = Hu3DData[object->mdlId[modelSlot]].pos.z;
    targetPosition.x = targetPosition.x;
    targetPosition.y = targetPosition.y;
    targetPosition.z = targetPosition.z;
    unreferencedLocal14 = rotationDuration;
    modelRotation.x = Hu3DData[object->mdlId[modelSlot]].rot.x;
    modelRotation.y = Hu3DData[object->mdlId[modelSlot]].rot.y;
    modelRotation.z = Hu3DData[object->mdlId[modelSlot]].rot.z;
    targetRotation.x = 0.0f;
    targetRotation.y = rotationDegrees;
    targetRotation.z = 0.0f;
    if (targetPosition.x - currentPosition.x >= 0.0f) {
        movingTowardTargetX = 1;
    } else {
        movingTowardTargetX = 0;
    }
    if (targetPosition.z - currentPosition.z >= 0.0f) {
        movingTowardTargetZ = 1;
    } else {
        movingTowardTargetZ = 0;
    }
    rotationDuration = finalDuration;
    do {
        targetHeading.y = -(atan2d(targetPosition.z - currentPosition.z, targetPosition.x - currentPosition.x) - 90.0);
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
        movingPosition.x = currentPosition.x += 6.0 * sind(targetHeading.y);
        movingPosition.z = currentPosition.z += 6.0 * cosd(targetHeading.y);
        if (movingTowardTargetX != 0) {
            if (currentPosition.x >= targetPosition.x) {
                movingPosition.x = currentPosition.x = targetPosition.x;
                reachedTargetX = 1;
            }
        } else {
            if (currentPosition.x <= targetPosition.x) {
                movingPosition.x = currentPosition.x = targetPosition.x;
                reachedTargetX = 1;
            }
        }
        if (movingTowardTargetZ != 0) {
            if (currentPosition.z >= targetPosition.z) {
                movingPosition.z = currentPosition.z = targetPosition.z;
                reachedTargetZ = 1;
            }
        } else {
            if (currentPosition.z <= targetPosition.z) {
                movingPosition.z = currentPosition.z = targetPosition.z;
                reachedTargetZ = 1;
            }
        }
        modelRotation.y = InterpolateWeightedAverage(modelRotation.y, targetHeading.y, 10.0f);
        Hu3DModelPosSet(object->mdlId[1], movingPosition.x, currentPosition.y, movingPosition.z);
        Hu3DModelRotSet(object->mdlId[1], modelRotation.x, modelRotation.y, modelRotation.z);
        HuPrcVSleep();
    } while (reachedTargetZ != 1 || reachedTargetX != 1);
    modelRotation.y = Hu3DData[object->mdlId[modelSlot]].rot.y;
    for (i = 0; i <= rotationDuration; i++) {
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
        modelRotation.y = InterpolateLinear(modelRotation.y, targetRotation.y, i, rotationDuration);
        Hu3DModelRotSet(object->mdlId[1], modelRotation.x, modelRotation.y, modelRotation.z);
        HuPrcVSleep();
    }
}

Void SetModelMaterialsWhite(s16 modelId) {
    HU3DMODEL* model;
    HSFDATA* modelData;
    HSFMATERIAL* material;
    s16 materialCount;
    s32 i;

    model = &Hu3DData[modelId];
    modelData = model->hsf;
    materialCount = modelData->materialNum;
    material = modelData->material;
    for (i = 0; i < materialCount; material++, i++) {
        material->vtxMode = 1;
        material->litColor[0] = 0xFF;
        material->litColor[1] = 0xFF;
        material->litColor[2] = 0xFF;
    }
}

Void ReadStoryPlayerConfig(StoryPlayerConfig* playerConfig, s32 playerIndex) {
    playerConfig->playerIndex = playerIndex;
    playerConfig->group = GWPlayerCfg[playerConfig->playerIndex].group;
    playerConfig->isComputer = GWPlayerCfg[playerConfig->playerIndex].iscom;
    playerConfig->difficulty = GWPlayerCfg[playerConfig->playerIndex].diff;
    playerConfig->characterId = GWPlayerCfg[playerConfig->playerIndex].character;
    playerConfig->padIndex = GWPlayerCfg[playerConfig->playerIndex].pad_idx;
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

s32 CreateStoryDirectionalLight(s32 lightIntensity) {
    s32 lightId = 0;

    lightId = Hu3DGLightCreate(0.0f, 700.0f, 1200.0f, 0.0f, -1.0f, 0.0f, lightIntensity, lightIntensity, lightIntensity);
    Hu3DGLightPosAimSet(lightId, 0.0f, 700.0f, 1200.0f, 0.0f, 0.0f, 640.0f);
    Hu3DGLightInfinitytSet(lightId);
    return lightId;
}

Void CreateStoryShadow(Void) {
    Vec shadowPosition = { 0.0f, 3000.0f, 800.0f };
    Vec shadowTarget = { 0.0f, 0.0f, 640.0f };
    Vec shadowUp = { 0.0f, 1.0f, 0.0f };

    Hu3DShadowCreate(45.0f, 2000.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.8f);
    Hu3DShadowPosSet(&shadowPosition, &shadowUp, &shadowTarget);
}

Void ApplyCameraMotionTargets(StoryCameraMotionState* cameraState) {
    cameraState->cameraCenter.x = cameraState->startCenterX;
    cameraState->cameraCenter.y = cameraState->startCenterY;
    cameraState->cameraCenter.z = cameraState->startCenterZ;
    cameraState->cameraRotation.x = cameraState->startRotationX;
    cameraState->cameraRotation.y = cameraState->startRotationY;
    cameraState->cameraRotation.z = cameraState->startRotationZ;
    cameraState->cameraZoom = cameraState->startZoom;
}

Void CaptureCameraMotionStart(StoryCameraMotionState* cameraState) {
    cameraState->animationFrame = 0;
    cameraState->startCenterX = cameraState->cameraCenter.x;
    cameraState->startCenterY = cameraState->cameraCenter.y;
    cameraState->startCenterZ = cameraState->cameraCenter.z;
    cameraState->startRotationX = cameraState->cameraRotation.x;
    cameraState->startRotationY = cameraState->cameraRotation.y;
    cameraState->startRotationZ = cameraState->cameraRotation.z;
    cameraState->startZoom = cameraState->cameraZoom;
}

Void EaseCameraMotionWeighted(StoryCameraMotionState* cameraState, StoryCameraMotionState* targetCameraState, float smoothingFactor) {
    cameraState->cameraCenter.x = InterpolateWeightedAverage(cameraState->cameraCenter.x, targetCameraState->startCenterX, smoothingFactor);
    cameraState->cameraCenter.y = InterpolateWeightedAverage(cameraState->cameraCenter.y, targetCameraState->startCenterY, smoothingFactor);
    cameraState->cameraCenter.z = InterpolateWeightedAverage(cameraState->cameraCenter.z, targetCameraState->startCenterZ, smoothingFactor);
    cameraState->cameraRotation.x = InterpolateWeightedAverage(cameraState->cameraRotation.x, targetCameraState->startRotationX, smoothingFactor);
    cameraState->cameraRotation.y = InterpolateWeightedAverage(cameraState->cameraRotation.y, targetCameraState->startRotationY, smoothingFactor);
    cameraState->cameraRotation.z = InterpolateWeightedAverage(cameraState->cameraRotation.z, targetCameraState->startRotationZ, smoothingFactor);
    cameraState->cameraZoom = InterpolateWeightedAverage(cameraState->cameraZoom, targetCameraState->startZoom, smoothingFactor);
}

Void EaseCameraMotionInPlace(StoryCameraMotionState* cameraState, float smoothingFrames) {
    EaseCameraMotionWeighted(cameraState, cameraState, smoothingFrames);
}

Void InterpolateCameraMotionLinear(StoryCameraMotionState* sourceCameraState, StoryCameraMotionState* targetCameraState, float elapsedFrames, float durationFrames) {
    targetCameraState->startCenterX = InterpolateLinear(sourceCameraState->startCenterX, targetCameraState->cameraCenter.x, elapsedFrames, durationFrames);
    targetCameraState->startCenterY = InterpolateLinear(sourceCameraState->startCenterY, targetCameraState->cameraCenter.y, elapsedFrames, durationFrames);
    targetCameraState->startCenterZ = InterpolateLinear(sourceCameraState->startCenterZ, targetCameraState->cameraCenter.z, elapsedFrames, durationFrames);
    targetCameraState->startRotationX = InterpolateLinear(sourceCameraState->startRotationX, targetCameraState->cameraRotation.x, elapsedFrames, durationFrames);
    targetCameraState->startRotationY = InterpolateLinear(sourceCameraState->startRotationY, targetCameraState->cameraRotation.y, elapsedFrames, durationFrames);
    targetCameraState->startRotationZ = InterpolateLinear(sourceCameraState->startRotationZ, targetCameraState->cameraRotation.z, elapsedFrames, durationFrames);
    targetCameraState->startZoom = InterpolateLinear(sourceCameraState->startZoom, targetCameraState->cameraZoom, elapsedFrames, durationFrames);
}

Void UpdateCameraMotionLinear(StoryCameraMotionState* sourceCameraState, StoryCameraMotionState* targetCameraState, float elapsedFrames, float durationFrames) {
    InterpolateCameraMotionLinear(sourceCameraState, targetCameraState, elapsedFrames, durationFrames);
    sourceCameraState->cameraCenter.x = targetCameraState->startCenterX;
    sourceCameraState->cameraCenter.y = targetCameraState->startCenterY;
    sourceCameraState->cameraCenter.z = targetCameraState->startCenterZ;
    sourceCameraState->cameraRotation.x = targetCameraState->startRotationX;
    sourceCameraState->cameraRotation.y = targetCameraState->startRotationY;
    sourceCameraState->cameraRotation.z = targetCameraState->startRotationZ;
    sourceCameraState->cameraZoom = targetCameraState->startZoom;
}

Void EaseCameraMotionLinear(StoryCameraMotionState* sourceCameraState, StoryCameraMotionState* targetCameraState, float elapsedFrames, float durationFrames, float smoothingFrames) {
    InterpolateCameraMotionLinear(sourceCameraState, targetCameraState, elapsedFrames, durationFrames);
    EaseCameraMotionWeighted(sourceCameraState, targetCameraState, smoothingFrames);
}

Void InterpolateCameraMotionSineIn(StoryCameraMotionState* sourceCameraState, StoryCameraMotionState* targetCameraState, float elapsedFrames, float durationFrames) {
    targetCameraState->startCenterX = InterpolateSineEaseIn(sourceCameraState->startCenterX, targetCameraState->cameraCenter.x, elapsedFrames, durationFrames);
    targetCameraState->startCenterY = InterpolateSineEaseIn(sourceCameraState->startCenterY, targetCameraState->cameraCenter.y, elapsedFrames, durationFrames);
    targetCameraState->startCenterZ = InterpolateSineEaseIn(sourceCameraState->startCenterZ, targetCameraState->cameraCenter.z, elapsedFrames, durationFrames);
    targetCameraState->startRotationX = InterpolateSineEaseIn(sourceCameraState->startRotationX, targetCameraState->cameraRotation.x, elapsedFrames, durationFrames);
    targetCameraState->startRotationY = InterpolateSineEaseIn(sourceCameraState->startRotationY, targetCameraState->cameraRotation.y, elapsedFrames, durationFrames);
    targetCameraState->startRotationZ = InterpolateSineEaseIn(sourceCameraState->startRotationZ, targetCameraState->cameraRotation.z, elapsedFrames, durationFrames);
    targetCameraState->startZoom = InterpolateSineEaseIn(sourceCameraState->startZoom, targetCameraState->cameraZoom, elapsedFrames, durationFrames);
}

Void UpdateCameraMotionSineIn(StoryCameraMotionState* sourceCameraState, StoryCameraMotionState* targetCameraState, float elapsedFrames, float durationFrames) {
    InterpolateCameraMotionSineIn(sourceCameraState, targetCameraState, elapsedFrames, durationFrames);
    sourceCameraState->cameraCenter.x = targetCameraState->startCenterX;
    sourceCameraState->cameraCenter.y = targetCameraState->startCenterY;
    sourceCameraState->cameraCenter.z = targetCameraState->startCenterZ;
    sourceCameraState->cameraRotation.x = targetCameraState->startRotationX;
    sourceCameraState->cameraRotation.y = targetCameraState->startRotationY;
    sourceCameraState->cameraRotation.z = targetCameraState->startRotationZ;
    sourceCameraState->cameraZoom = targetCameraState->startZoom;
}

Void EaseCameraMotionSineIn(StoryCameraMotionState* sourceCameraState, StoryCameraMotionState* targetCameraState, float elapsedFrames, float durationFrames, float smoothingFrames) {
    InterpolateCameraMotionSineIn(sourceCameraState, targetCameraState, elapsedFrames, durationFrames);
    EaseCameraMotionWeighted(sourceCameraState, targetCameraState, smoothingFrames);
}

Void InterpolateCameraMotionCosine(StoryCameraMotionState* sourceCameraState, StoryCameraMotionState* targetCameraState, float elapsedFrames, float durationFrames) {
    targetCameraState->startCenterX = InterpolateCosineEaseOut(sourceCameraState->startCenterX, targetCameraState->cameraCenter.x, elapsedFrames, durationFrames);
    targetCameraState->startCenterY = InterpolateCosineEaseOut(sourceCameraState->startCenterY, targetCameraState->cameraCenter.y, elapsedFrames, durationFrames);
    targetCameraState->startCenterZ = InterpolateCosineEaseOut(sourceCameraState->startCenterZ, targetCameraState->cameraCenter.z, elapsedFrames, durationFrames);
    targetCameraState->startRotationX = InterpolateCosineEaseOut(sourceCameraState->startRotationX, targetCameraState->cameraRotation.x, elapsedFrames, durationFrames);
    targetCameraState->startRotationY = InterpolateCosineEaseOut(sourceCameraState->startRotationY, targetCameraState->cameraRotation.y, elapsedFrames, durationFrames);
    targetCameraState->startRotationZ = InterpolateCosineEaseOut(sourceCameraState->startRotationZ, targetCameraState->cameraRotation.z, elapsedFrames, durationFrames);
    targetCameraState->startZoom = InterpolateCosineEaseOut(sourceCameraState->startZoom, targetCameraState->cameraZoom, elapsedFrames, durationFrames);
}

Void UpdateCameraMotionCosine(StoryCameraMotionState* sourceCameraState, StoryCameraMotionState* targetCameraState, float elapsedFrames, float durationFrames) {
    InterpolateCameraMotionCosine(sourceCameraState, targetCameraState, elapsedFrames, durationFrames);
    sourceCameraState->cameraCenter.x = targetCameraState->startCenterX;
    sourceCameraState->cameraCenter.y = targetCameraState->startCenterY;
    sourceCameraState->cameraCenter.z = targetCameraState->startCenterZ;
    sourceCameraState->cameraRotation.x = targetCameraState->startRotationX;
    sourceCameraState->cameraRotation.y = targetCameraState->startRotationY;
    sourceCameraState->cameraRotation.z = targetCameraState->startRotationZ;
    sourceCameraState->cameraZoom = targetCameraState->startZoom;
}

Void EaseCameraMotionCosine(StoryCameraMotionState* sourceCameraState, StoryCameraMotionState* targetCameraState, float elapsedFrames, float durationFrames, float smoothingFrames) {
    InterpolateCameraMotionCosine(sourceCameraState, targetCameraState, elapsedFrames, durationFrames);
    EaseCameraMotionWeighted(sourceCameraState, targetCameraState, smoothingFrames);
}

int GetModelMotionDuration(int modelId)
{
    return Hu3DMotionMaxTimeGet(modelId);
}

Void UpdateObjectMotionTransition(OMOBJ* object) {
    s32 motionIndex;
    if(object->work[0] == 0) {
        return;
    }
    if (object->work[2] != 0) {
        object->work[2]--;
        return;
    }
    if (object->work[2] == 0 && Hu3DMotionEndCheck(object->mdlId[1])) {
        motionIndex = object->work[1];
        Hu3DMotionShiftSet(object->mdlId[1], object->mtnId[motionIndex], 0.0f, 15.0f, 0x40000001U);
        object->work[0] = object->work[1] = object->work[2] = 0;
        object->work[0] = object->work[1] = object->work[2] = 0;
    }
}

Void SetObjectMotionTransition(OMOBJ* object, s32 motionSlot, s32 nextMotionSlot, s32 blendFrames, s32 motionFlags) {
    ShiftObjectMotion(object, 1, motionSlot, blendFrames, motionFlags);
    if (motionFlags == 0) {
        object->work[0] = 1;
        object->work[1] = nextMotionSlot;
        object->work[2] = blendFrames + 1;
    }
}

Void SetWindowMessageDirect(s32 windowId, s32 messageId) {
    HuWinMesSet(windowId, messageId);
}

Void InsertWindowMessageDirect(s32 windowId, s32 messageId, s32 insertIndex) {
    HuWinInsertMesSet(windowId, messageId, insertIndex);
}

Void WaitForWindowMessageDirect(s32 windowId, s32 delayFrames) {
    WaitForWindowMessage(windowId, delayFrames);
}

Void WaitForWindowMessageAndMotion(s32 windowId, s32 messageId, s32 highlightKeyIndex, s32 modelId, s32 nextMotionId) {
    s32 keyMessageCount = 0;
    s32 highlightState = 0;
    s32 resetMessageAfterMotion = 0;
    s32 motionWaitFrames;
    s32 i;
    if(highlightKeyIndex == -1) {
        resetMessageAfterMotion = 1;
        highlightKeyIndex = 0;
    }
    HuWinMesSet(windowId, messageId);
    keyMessageCount = HuWinKeyWaitNumGet(messageId);
    if (highlightKeyIndex == -1 || highlightKeyIndex >= keyMessageCount - 1) {
        highlightKeyIndex = keyMessageCount - 1;
    }
    HuWinComKeyReset();
    if (winData[windowId].active_pad == 0xF) {
        for (i = 0; i < 10; i++) {
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, GWMessDelayGet());
        }
    }
    HuWinMesSet(windowId, messageId);
    for (i = 0; i < keyMessageCount; i++) {
        highlightState = 0;
        do {
            HuPrcVSleep();
        } while (HuWinStatGet(windowId) != 1);

        if (i == highlightKeyIndex) {
            HuWinAttrSet(windowId, 0x400);
            highlightState = 1;
        }
        while (TRUE) {
            HuPrcVSleep();
            if (highlightState != 0) {
                if (highlightState == 1 && HuWinStatGet(windowId) != 1) {
                    highlightState = 2;
                } else if (highlightState == 2) {
                    if(modelId != -1) {
                        if(nextMotionId != -1) {
                            i=0;
                            do {
                                HuPrcVSleep();
                                if(i++ == 25) {
                                    HuAudFXPlay(storyAudioCueIds[6][0]);
                                }
                            } while(!Hu3DMotionEndCheck(modelId));
                            Hu3DMotionShiftSet(modelId, nextMotionId, 0, 1, 0);
                            HuPrcSleep(15);
                        }
                        motionWaitFrames = GetModelMotionDuration(modelId)-15;
                        if(resetMessageAfterMotion == 1) {
                            for(i=0; i<motionWaitFrames; i++) {
                                HuPrcVSleep();
                                if(i == 25) {
                                    HuAudFXPlay(storyAudioCueIds[6][0]);
                                }
                            }
                        } else {
                            if(motionWaitFrames > 0) {
                                HuPrcSleep(motionWaitFrames);
                            }
                        }
                    } else {
                        HuPrcSleep(60);
                    }
                    HuWinAttrReset(windowId, 0x400);
                    HuWinKeyWaitEntry(windowId);
                    break;
                }
            } else if (HuWinStatGet(windowId) != 1) {
                break;
            }
        }
    }
    WaitForWindowMessage(windowId, 0);
    HuWinMesSet(windowId, MAKE_MESSID(37, 4));
    HuWinComKeyReset();
}

Void WaitForWindowMessageAndMotionSimple(s32 windowId, s32 messageId, s32 highlightKeyIndex, s32 modelId)
{
    WaitForWindowMessageAndMotion(windowId, messageId, highlightKeyIndex, modelId, -1);
}

Void WaitForWindowMessageDuration(s32 windowId, s32 messageId, s32 highlightKeyIndex, s32 delayFrames) {
    s32 keyMessageCount = 0;
    s32 highlightState = 0;
    s32 i;
    
    HuWinMesSet(windowId, messageId);
    keyMessageCount = HuWinKeyWaitNumGet(messageId);
    if (highlightKeyIndex == -1 || highlightKeyIndex >= keyMessageCount - 1) {
        highlightKeyIndex = keyMessageCount - 1;
    }
    HuWinComKeyReset();
    if (winData[windowId].active_pad == 0xF) {
        for (i = 0; i < 10; i++) {
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, GWMessDelayGet());
        }
    }
    HuWinMesSet(windowId, messageId);
    for (i = 0; i < keyMessageCount; i++) {
        highlightState = 0;
        do {
            HuPrcVSleep();
        } while (HuWinStatGet(windowId) != 1);
        if (i == highlightKeyIndex) {
            HuWinAttrSet(windowId, 0x400);
            highlightState = 1;
        }
        while (TRUE) {
            HuPrcVSleep();
            if (highlightState != 0) {
                if (highlightState == 1 && HuWinStatGet(windowId) != 1) {
                    highlightState = 2;
                } else if (highlightState == 2) {
                    HuPrcSleep(delayFrames);
                    HuWinAttrReset(windowId, 0x400);
                    HuWinKeyWaitEntry(windowId);
                    break;
                }
            } else if (HuWinStatGet(windowId) != 1) {
                break;
            }
        }
    }
    WaitForWindowMessage(windowId, 0);
    HuWinMesSet(windowId, MAKE_MESSID(37, 4));
    HuWinComKeyReset();
}

BOOL GetWindowChoiceDefault(s32 windowId) {
    return GetWindowChoice(windowId, 0);
}

s32 CreateAutoSizedMessageWindow(s32 messageId, s32 padPlayerIndex) {
    float messageSize[2];
    float windowWidth;
    float windowHeight;
    float windowLeft;
    float windowTop;
    s32 windowId;

    HuWinMesMaxSizeGet(1, messageSize, messageId);
    windowWidth = messageSize[0];
    windowHeight = messageSize[1];
    windowLeft = 550.0f - windowWidth;
    windowTop = 430.0f - windowHeight;
    windowId = HuWinExCreateStyled(windowLeft, windowTop, windowWidth, windowHeight, -1, 1);
    if (padPlayerIndex != -1) {
        padPlayerIndex = GWPlayerCfg[padPlayerIndex].pad_idx;
        winData[windowId].active_pad = (1 << padPlayerIndex);
    } else {
        winData[windowId].active_pad = 0xF;
    }
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    AnimateWindowSlide(windowId, 0.5f, 0.5f, 1);
    return windowId;
}

Void CloseAutoSizedMessageWindow(s32 windowId) {
    CloseMessageWindowAtAnchor(windowId, 0.5f, 0.5f);
}

char *characterModelNodeNames[8] = {
    "c000m1-ske_head",
    "c001m1-ske_head",
    "c002m1-ske_head",
    "c003m1-ske_head",
    "c004m1-ske_head",
    "c005m1-ske_head",
    "c006m1-ske_head",
    "c007m1-ske_head"
};

s32 characterMeshTransformFlags[8][2] = {
    0x200, 0,
    0x200, 0,
    0x200, 0,
    0x200, 0,
    0x200, 0,
    0x200, 0,
    0x200, 0,
    0x200, 0,
};

float characterMeshOffsetValues[8] = {
    194,
    191,
    87.2,
    50.1,
    72,
    83.5,
    87.2,
    64.5
};

Void ApplyModelObjectTransform(s16 modelId, char *objectName, u32 constFlags, float offset)
{
    HSFOBJECT *object = Hu3DModelObjPtrGet(modelId, objectName);
    HSFCONSTDATA *constData;
    if(!object->constData) {
        constData = ObjConstantMake(object, Hu3DData[modelId].mallocNo);
    } else {
        constData = object->constData;
    }
    constData->attr |= constFlags;
    if(constFlags & 0x10) {
        object->mesh.curr.pos.x += offset;
    }
    if(constFlags & 0x20) {
        object->mesh.curr.pos.y += offset;
    }
    if(constFlags & 0x40) {
        object->mesh.curr.pos.z += offset;
    }
    if(constFlags & 0x80) {
        object->mesh.curr.rot.x += offset;
    }
    if(constFlags & 0x100) {
        object->mesh.curr.rot.y += offset;
    }
    if(constFlags & 0x200) {
        object->mesh.curr.rot.z += offset;
    }
}

Void AnimateCharacterMeshOffset(int modelId, int characterId, float offset, s32 frameCount, BOOL moveOutward)
{
    float currentOffset = 0;
    float targetOffset = offset;
    s32 i;
    if(characterId == 7) {
        targetOffset *= 2.5f;
    }
    if(characterMeshTransformFlags[characterId][1] == 0) {
        targetOffset *= -1;
    }
    if(moveOutward) {
        for(i=0; i<frameCount; i++) {
            HuPrcVSleep();
            currentOffset = InterpolateLinear(0, targetOffset, i, frameCount);
            ApplyModelObjectTransform(modelId, characterModelNodeNames[characterId], characterMeshTransformFlags[characterId][0], currentOffset);
            if(characterId == 2) {
                ApplyModelObjectTransform(modelId, "c002m1-ske_hair1", characterMeshTransformFlags[characterId][0], -currentOffset);
            } else if(characterId == 6) {
                
            }
        }
        ApplyModelObjectTransform(modelId, characterModelNodeNames[characterId], characterMeshTransformFlags[characterId][0], targetOffset);
        if(characterId == 2) {
            ApplyModelObjectTransform(modelId, "c002m1-ske_hair1", characterMeshTransformFlags[characterId][0], -targetOffset);
        }
        OSReport("\n");
    } else {
        for(i=0; i<frameCount; i++) {
            HuPrcVSleep();
            currentOffset = InterpolateLinear(0, targetOffset, i, frameCount);
            ApplyModelObjectTransform(modelId, characterModelNodeNames[characterId], characterMeshTransformFlags[characterId][0], -currentOffset);
            if(characterId == 2) {
                ApplyModelObjectTransform(modelId, "c002m1-ske_hair1", characterMeshTransformFlags[characterId][0], currentOffset);
            } else if(characterId == 6) {
                
            }
        }
        Hu3DMotionNoMotReset(modelId, characterModelNodeNames[characterId], characterMeshTransformFlags[characterId][0]);
        if(characterId == 2) {
            Hu3DMotionNoMotReset(modelId, "c002m1-ske_hair1", characterMeshTransformFlags[characterId][0]);
        }
    }
}
