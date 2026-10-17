#include "REL/mstory3Dll.h"
#include "game/disp.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/window.h"
#include "game/wipe.h"

#ifndef __MWERKS__
#include "game/objsub.h"
#endif

#include "ext_math.h"

u8 unreferencedDebugData[0x10]; // unused
StoryCameraState storyCameraState;
HUPROCESS* storyObjectManager;

s32 recordMessageIdTable[][4] = {
    { 0x37, 0x38, 0x36, 0x39 },
    { 0x43, 0x45, 0x44, 0x43 },
    { 0x40, 0x42, 0x41, 0x40 },
    { 0x4B, 0x4D, 0x4C, 0x4B },
    { 0x46, 0x48, 0x47, 0x46 },
    { 0x3E, 0x3F, 0x3E, 0x3E },
    { 0x3A, 0x3B, 0x3C,   -1 }
};

char* eventMenuOptions[] = {
    "S-1 : ALL STORY - BOARD RESULT EVENT",
    "--- : RESULT"
};

Void Story3FinishAfterWipe(OMOBJ* object) {
    if (WipeStatGet() == 0) {
        object->objFunc = NULL;
        omOvlReturnEx(1, 1);
    }
}

Void Story3HandleSystemExit(OMOBJ* object) {
    if (omSysExitReq != 0) {
        object->objFunc = Story3FinishAfterWipe;
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    }
}

Void Story3EventSelectMenu(Void) {
    s32 unreferencedMenuState;
    s32 unreferencedMenuFlagA;
    s32 unreferencedMenuFlagB;
    s32 unreferencedMenuFlagC;
    s32 menuOptionIndex;
    s32 selectedEvent;
    s32 selectedBoard;

    unreferencedMenuState = 0;
    selectedEvent = 0;
    selectedBoard = 0;
    unreferencedMenuFlagA = 1;
    unreferencedMenuFlagB = 1;
    unreferencedMenuFlagC = 1;
    omAddObjEx(storyObjectManager, 0x2000, 0, 0, -1, Story3HandleSystemExit);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    while (TRUE) {
        if (WipeStatGet() == 0) {
            break;
        }
        Story3SleepOneFrame();
    }
    while (TRUE) {
        if (HuPadBtnDown[0] & PAD_BUTTON_A) {
            break;
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_DOWN) {
            selectedEvent++;
            if (selectedEvent >= 2) {
                selectedEvent = 0;
            }
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_UP) {
            selectedEvent--;
            if (selectedEvent < 0) {
                selectedEvent = 1;
            }
        }
        if (HuPadStkX[0] <= -30) {
            selectedBoard++;
            if (selectedBoard > 6) {
                selectedBoard = 0;
            }
        }
        if (HuPadStkX[0] >= 30) {
            selectedBoard--;
            if (selectedBoard < 0) {
                selectedBoard = 5;
            }
        }
        fontcolor = FONT_COLOR_WHITE;
        print8(24, 100, 1.5f, ">>>>> MPGC STORY 3 - EVENT SELECT <<<<<");
        print8(24, 130, 1.5f, "MAP NUMBER : %d", selectedBoard + 1);
        for (menuOptionIndex = 0; menuOptionIndex < 2; menuOptionIndex++) {
            fontcolor = FONT_COLOR_WHITE;
            if (selectedEvent == menuOptionIndex) {
                fontcolor = FONT_COLOR_CYAN;
            }
            print8(24, 160 + menuOptionIndex * 15, 1.5f, "%s", eventMenuOptions[menuOptionIndex]);
        }
        Story3SleepOneFrame();
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (TRUE) {
        if (WipeStatGet() == 0) {
            break;
        }
        Story3SleepOneFrame();
    }
    GWSystem.board = selectedBoard;
    switch (selectedEvent) {
        case 0:
            Story3InitializeResultSequence(storyObjectManager, selectedBoard, 1, 0, 0);
            break;
        case 1:
            Story3StartBoardResult(storyObjectManager, selectedBoard, 0, 0);
            break;
    }
    while (TRUE) {
        Story3SleepOneFrame();
    }
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

Void Story3RunEventSelector(Void) {
    storyObjectManager = omInitObjMan(62, 0x2000);
    omGameSysInit(storyObjectManager);
    switch (omovlevtno) {
        case 0:
            Story3InitializeResultSequence(storyObjectManager, GWSystem.board, GWSystem.bonus_star, GWSystem.party, 0);
            break;
        case 1:
            Story3StartBoardResult(storyObjectManager, GWSystem.board, GWSystem.party, 0);
            break;
    }
}

Void ObjectSetup(Void) {
    Story3RunEventSelector();
}

float InterpolateLinearOverTime(float startValue, float targetValue, float frame, float duration) {
    if (duration <= frame) {
        return targetValue;
    }
    return startValue + (frame / duration) * (targetValue - startValue);
}

float InterpolateSmoothAverage(float startValue, float targetValue, float frame) {
    return (targetValue + startValue * (frame - 1.0f)) / frame;
}

float InterpolateCosineEaseOut(float startValue, float targetValue, float frame, float duration) {
    if (frame >= duration) {
        return targetValue;
    }
    return startValue + (targetValue - startValue) * (1.0 - cosd((90.0f / duration) * frame));
}

float InterpolateSineEaseIn(float startValue, float targetValue, float frame, float duration) {
    if (frame >= duration) {
        return targetValue;
    }
    return startValue + (targetValue - startValue) * sind((90.0f / duration) * frame);
}

float InterpolateSineEaseOut(float startValue, float targetValue, float frame, float duration) {
    if (frame >= duration) {
        return startValue;
    }
    return startValue + (targetValue - startValue) * sind((180.0f / duration) * frame);
}

float InterpolateSinePulse(float startValue, float targetValue, float frame, float duration) {
    if (frame >= duration) {
        return startValue;
    }
    return startValue + (targetValue - startValue) * sind((360.0f / duration) * frame);
}

Void Story3Noop(Void) {
}

Void Story3SleepOneFrame(Void) {
    HuPrcVSleep();
}

Void Story3SleepFrames(s32 frameCount) {
    HuPrcSleep(frameCount);
}

Void Story3EndProcess(Void) {
    HuPrcEnd();
}

Void Story3InitializeWindows(Void) {
    HuWinInit(1);
}

Void Story3KillAllWindows(Void) {
    HuWinAllKill();
}

Void Story3AnimateWindowFocus(s32 windowId, float targetValue, float frame, s32 mode) {
    WindowData* windowData;
    float originalWindowX;
    float originalWindowY;
    float windowWidth;
    float windowHeight;
    float targetWindowX;
    float targetWindowScaleX;
    float targetWindowY;
    float targetWindowScaleY;
    float animatedWindowX;
    float animatedWindowY;
    float animatedWindowScaleX;
    float animatedWindowScaleY;
    s32 i;

    windowData = &winData[windowId];
    originalWindowX = windowData->pos_x;
    originalWindowY = windowData->pos_y;
    windowWidth = windowData->w;
    windowHeight = windowData->h;
    targetWindowX = originalWindowX + windowWidth * targetValue;
    targetWindowScaleX = 4.0f / windowWidth;
    if (targetWindowX + 4.0f - (originalWindowX + windowWidth) >= 0.0f) {
        targetWindowX = (originalWindowX + windowWidth) - 4.0f;
    } else if (originalWindowX - (targetWindowX - 2.0f) >= 0.0f) {
        targetWindowX = originalWindowX;
    } else {
        targetWindowX -= 2.0f;
    }
    targetWindowY = originalWindowY + windowHeight * frame;
    targetWindowScaleY = 4.0f / windowHeight;
    if (targetWindowY + 4.0f - (originalWindowY + windowHeight) >= 0.0f) {
        targetWindowY = (originalWindowY + windowHeight) - 4.0f;
    } else if (originalWindowY - (targetWindowY - 2.0f) >= 0.0f) {
        targetWindowY = originalWindowY;
    } else {
        targetWindowY -= 2.0f;
    }
    if (mode != 0) {
        HuWinPosSet(windowId, targetWindowX, targetWindowY);
        HuWinScaleSet(windowId, targetWindowScaleX, targetWindowScaleY);
        animatedWindowX = targetWindowX;
        animatedWindowScaleX = targetWindowScaleX;
        animatedWindowY = targetWindowY;
        animatedWindowScaleY = targetWindowScaleY;
    } else {
        HuWinPosSet(windowId, originalWindowX, originalWindowY);
        HuWinScaleSet(windowId, 1.0f, 1.0f);
        animatedWindowX = originalWindowX;
        animatedWindowScaleX = 1.0f;
        animatedWindowY = originalWindowY;
        animatedWindowScaleY = 1.0f;
    }
    HuWinDispOn(windowId);
    for (i = 0; i <= 15; i++) {
        HuPrcVSleep();
        if (mode != 0) {
            if (i <= 10) {
                animatedWindowX = InterpolateSineEaseIn(targetWindowX, originalWindowX, i, 10.0f);
                animatedWindowScaleX = InterpolateSineEaseIn(targetWindowScaleX, 1.0f, i, 10.0f);;
            } else {
                animatedWindowY = InterpolateSineEaseIn(targetWindowY, originalWindowY, i - 10, 5.0f);
                animatedWindowScaleY = InterpolateSineEaseIn(targetWindowScaleY, 1.0f, i - 10, 5.0f);
            }
        } else {
            if (i <= 10) {
                animatedWindowY = InterpolateSineEaseIn(originalWindowY, targetWindowY, i, 10.0f);
                animatedWindowScaleY = InterpolateSineEaseIn(1.0f, targetWindowScaleY, i, 10.0f);
            } else {
                animatedWindowX = InterpolateSineEaseIn(originalWindowX, targetWindowX, i - 10, 5.0f);
                animatedWindowScaleX = InterpolateSineEaseIn(1.0f, targetWindowScaleX, i - 10, 5.0f);
            }
        }
        HuWinPosSet(windowId, animatedWindowX, animatedWindowY);
        HuWinScaleSet(windowId, animatedWindowScaleX, animatedWindowScaleY);
    }
    if (mode != 0) {
        HuWinPosSet(windowId, originalWindowX, originalWindowY);
        HuWinScaleSet(windowId, 1.0f, 1.0f);
    } else {
        HuWinPosSet(windowId, targetWindowX, targetWindowY);
        HuWinScaleSet(windowId, targetWindowScaleX, targetWindowScaleY);
    }
    HuPrcVSleep();
}

s32 Story3CreateMessageWindow(s32 windowId, s32 messageId, s32 mode, s32 mode3, float duration, float duration5, s32 mode6) {
    s32 windowId;

    mode = mode * 21 + 16;
    mode3 = mode3 * 26 + 16;
    if (windowId == 0) {
        windowId = 550 - mode;
    } else {
        windowId = 1.0f + (576.0f - mode) / 2;
    }
    if (messageId == 0) {
        messageId = 430 - mode3;
    } else {
        messageId = 445 - mode3;
    }
    windowId = HuWinExCreateStyled(windowId, messageId, mode, mode3, -1, 1);
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    if (mode6 != -1) {
        if (GWSystem.party != 1) {
            winData[windowId].active_pad = 1;
        } else {
            winData[windowId].active_pad = 0xF;
        }
    } else {
        winData[windowId].active_pad = 0xFF;
    }
    Story3AnimateWindowFocus(windowId, duration, duration5, 1);
    if (messageId != 0) {
        HuWinPriSet(windowId, 50);
    }
    return windowId;
}

s32 Story3CreateStandardMessageWindow(s32 windowId, s32 messageId, s32 mode, s32 mode3, s32 mode4) {
    return Story3CreateMessageWindow(windowId,    messageId,    mode,    mode3,    0.5f,    0.5f,    mode4);
}

s32 Story3CreateDataWindow(u32 windowId, s32 messageId) {
    float windowDimensions[2];
    float windowWidth;
    float windowHeight;
    float windowX;
    float windowY;
    s32 windowId;

    if (messageId == 0) {
        HuWinMesMaxSizeGet(1, windowDimensions, windowId);
    }
    windowWidth = windowDimensions[0];
    windowHeight = windowDimensions[1];
    windowX = (576.0f - windowWidth) / 2;
    windowY = 445.0f - windowHeight;
    if (messageId != 0) {
        windowWidth = 352.0f;
        windowHeight = 94.0f;
        windowX = 550.0f - windowWidth;
        windowY = 445.0f - windowHeight;
    }
    windowId = HuWinExCreateStyled(windowX, windowY, windowWidth, windowHeight, -1, 1);
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    if (messageId == 0) {
        HuWinBGTPLvlSet(windowId, 0.0f);
        HuWinMesSet(windowId, windowId);
        HuWinMesSpeedSet(windowId, 0);
        HuWinDispOn(windowId);
        HuWinMesPalSet(windowId, 7, 0xFF, 0xFF, 0xFF);
    } else {
        HuWinPriSet(windowId, 50);
        Story3AnimateWindowFocus(windowId, 0.5f, 0.5f, 1);
        HuWinMesSpeedSet(windowId, 0);
    }
    return windowId;
}

Void Story3CleanupWindow(s32 windowId) {
    HuWinExCleanup(windowId);
}

Void Story3ShowMessageWindow(s32 windowId, float targetValue, float frame) {
    HuWinMesSet(windowId, MAKE_MESSID(37, 4));
    HuPrcVSleep();
    Story3AnimateWindowFocus(windowId, targetValue, frame, 0);
    HuWinExCleanup(windowId);
}

Void Story3ShowCenteredMessage(s32 windowId) {
    Story3ShowMessageWindow(windowId, 0.5f, 0.5f);
}

Void Story3DisplayMessageAndWait(s32 windowId, s32 messageId) {
    s32 i;

    if (messageId > 0) {
        HuPrcSleep(messageId);
        return;
    }
    HuWinComKeyReset();
    if (winData[windowId].active_pad == 0xFF) {
        for (i = 0; i < 10; i++) {
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, GWMessDelayGet());
        }
    }
    HuWinMesWait(windowId);
    HuWinComKeyReset();
}

s32 Story3GetWindowChoice(s32 windowId, s32 messageId) {
    s32 unreferencedChoiceState;
    s32 choiceResult;
    s32 keyWaitFrame;

    unreferencedChoiceState = 0;
    HuWinComKeyReset();
    if (winData[windowId].active_pad == 0xFF) {
        for (keyWaitFrame = 0; keyWaitFrame < 10; keyWaitFrame++) {
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, GWMessDelayGet());
        }
    }
    choiceResult = HuWinChoiceGet(windowId, messageId);
    HuWinComKeyReset();
    if (choiceResult != 0) {
        choiceResult = 0;
    } else {
        choiceResult = 1;
    }
    return choiceResult;
}

s32 Story3GetWindowChoiceDefault(s32 windowId) {
    return Story3GetWindowChoice(windowId,    0);
}

Void Story3SetWindowMessage(s32 windowId, s32 messageId) {
    HuWinMesSet(windowId, messageId);
}

Void Story3InsertWindowMessage(s32 windowId, s32 messageId, s32 mode) {
    HuWinInsertMesSet(windowId, messageId, mode);
}

Void Story3SetModelMaterialLighting(s16 modelIndex) {
    HU3DMODEL* modelData;
    HSFMATERIAL* material;
    HSFDATA* modelFileData;
    s16 materialCount;
    s32 i;

    modelData = &Hu3DData[modelIndex];
    modelFileData = modelData->hsf;
    materialCount = modelFileData->materialNum;
    material = modelFileData->material;
    for (i = 0; i < materialCount; material++, i++) {
        material->vtxMode = 1;
    }
}

Void Story3SetModelMotion(OMOBJ* object, s32 messageId, s32 mode, s32 mode3, s32 mode4) {
    switch (mode4) {
        case 0:
            Hu3DMotionShiftSet(object->mdlId[messageId], object->mtnId[mode], 0.0f, mode3, HU3D_MOTATTR_NONE);
            break;
        case 1:
            Hu3DMotionShiftSet(object->mdlId[messageId], object->mtnId[mode], 0.0f, mode3, HU3D_MOTATTR_LOOP);
            break;
        case 2:
            Hu3DMotionShiftSet(object->mdlId[messageId], object->mtnId[mode], 0.0f, mode3, HU3D_MOTATTR_PAUSE);
            break;
    }
}

Void Story3StartModelMotionIfNeeded(OMOBJ* object, s32 messageId, s32 mode, s32 mode3, s32 mode4) {
    if (object->mtnId[mode] == Hu3DMotionIDGet(object->mdlId[messageId])) {
        return;
    }
    if (object->mtnId[mode] == Hu3DMotionShiftIDGet(object->mdlId[messageId])) {
        return;
    }
    Story3StartModelMotionIfNeeded(object, messageId, mode, mode3, mode4);
}

Void Story3WaitForModelMotionEnd(OMOBJ* object, s32 messageId, s32 mode, s32 mode3) {
    HuPrcSleep(mode3 + 1);
    do {
        HuPrcVSleep();
    } while (object->mtnId[mode] != Hu3DMotionIDGet(object->mdlId[messageId]) || !Hu3DMotionEndCheck(object->mdlId[messageId]));
}

Void Story3WaitForModelMotionNearEnd(OMOBJ* object, s32 messageId, s32 mode, s32 mode3) {
    HuPrcSleep(mode3 + 1);
    do {
        HuPrcVSleep();
    } while (object->mtnId[mode] != Hu3DMotionIDGet(object->mdlId[messageId]) || !(Hu3DMotionMaxTimeGet(object->mdlId[messageId]) - 20.0f <= Hu3DMotionTimeGet(object->mdlId[messageId])));
}

s32 Story3FindPlayerByTeam(s32 playerIndex) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (playerIndex == GWPlayerCfg[i].group) {
            break;
        }
    }
    return i;
}

Void Story3WaitForControllerButton(s32 waitFrames, s32 messageId) {
    while (TRUE) {
        if (messageId & HuPadBtnDown[waitFrames]) {
            break;
        }
        HuPrcVSleep();
    }
}

Void Story3UpdateCameraDebugInput(Void) {
    StoryCameraState* cameraState;

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

Void Story3SetCameraCallback(Void (*input)(Void)) {
    storyCameraState.callback = input;
}

Void Story3SetCameraDebugCallback(Void (*input)(Void)) {
    storyCameraState.debugInputCallback = input;
}

Void Story3RunCameraCallback(OMOBJ* object) {
    StoryCameraState* cameraState;

    cameraState = &storyCameraState;
    if (cameraState->callback != NULL) {
        cameraState->callback();
    }
    Center.x = cameraState->cameraCenter.x;
    Center.y = cameraState->cameraCenter.y;
    Center.z = cameraState->cameraCenter.z;
    CRot.x = cameraState->cameraRotation.x;
    CRot.y = cameraState->cameraRotation.y;
    CRot.z = cameraState->cameraRotation.z;
    CZoom = cameraState->cameraZoom;
}

Void Story3InitializeResultCamera(HUPROCESS* objectManager, Void (*input2)(Void)) {
    Hu3DCameraCreate(1);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 42.0f, 1.0f, 2000.0f, 1.2f);
    storyCameraState.callback = input2;
    storyCameraState.debugInputCallback = Story3UpdateCameraDebugInput;
    omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutView);
    omAddObjEx(objectManager, 0x7FD9, 0, 0, -1, Story3RunCameraCallback);
}

Void Story3AnimateObjectTransform(OMOBJ* object, s32 messageId, Vec input3, float duration, float duration4, float duration5) {
    Vec animatedPosition;
    Vec currentPosition;
    Vec targetPosition;
    Vec modelRotation;
    float unreferencedJumpParameter;
    float unreferencedJumpHeight;
    float unreferencedJumpState;
    s32 unreferencedJumpDuration;
    s32 unreferencedJumpFrames;
    float jumpVelocity;

    currentPosition.x = Hu3DData[object->mdlId[messageId]].pos.x;
    animatedPosition.y = currentPosition.y = Hu3DData[object->mdlId[messageId]].pos.y;
    currentPosition.z = Hu3DData[object->mdlId[messageId]].pos.z;
    targetPosition.x = input3.x;
    targetPosition.y = input3.y;
    targetPosition.z = input3.z;
    unreferencedJumpDuration = duration4;
    modelRotation.x = Hu3DData[object->mdlId[messageId]].rot.x;
    modelRotation.y = Hu3DData[object->mdlId[messageId]].rot.y;
    modelRotation.z = Hu3DData[object->mdlId[messageId]].rot.z;
    unreferencedJumpState = 0.0f;
    unreferencedJumpHeight = duration;
    unreferencedJumpParameter = 0.0f;
    unreferencedJumpFrames = duration5;
    jumpVelocity = -1.1025f;
    Hu3DMotionShiftSet(object->mdlId[1], object->mtnId[3], 0.0f, 5.0f, 0U);
    HuPrcSleep(10);
    while (TRUE) {
        if (jumpVelocity >= 1.7f) {
            jumpVelocity = 1.7f;
        }
        animatedPosition.y += jumpVelocity * jumpVelocity * (jumpVelocity >= 0.0f ? -17.5f : 17.5f);
        if (animatedPosition.y <= 0.0f && jumpVelocity > 0.0f) {
            animatedPosition.y = 0.0f;
        }
        animatedPosition.x = currentPosition.x += 6.0 * sind(modelRotation.y);
        animatedPosition.z = currentPosition.z += 6.0 * cosd(modelRotation.y);
        Hu3DModelPosSet(object->mdlId[1], animatedPosition.x, animatedPosition.y, animatedPosition.z);
        Hu3DModelRotSet(object->mdlId[1], modelRotation.x, modelRotation.y, modelRotation.z);
        HuPrcVSleep();
        if (animatedPosition.y <= input3.y && jumpVelocity > 0.0f) {
            break;
        }
        jumpVelocity += 0.11f;
    }
}

Void Story3UpdateObjectTransform(OMOBJ* object, s32 messageId, Vec input3, float duration, float duration4, float duration5) {
    Vec updatedPosition;
    Vec currentPosition;
    Vec targetPosition;
    Vec targetRotation;
    Vec currentRotation;
    Vec targetModelRotation;
    s32 turnDuration;
    s32 rotationFrameCount;
    s32 reachedTargetX;
    s32 reachedTargetZ;
    s32 movingPositiveX;
    s32 movingPositiveZ;
    s32 i;

    reachedTargetX = 0;
    reachedTargetZ = 0;
    currentPosition.x = Hu3DData[object->mdlId[messageId]].pos.x;
    currentPosition.y = Hu3DData[object->mdlId[messageId]].pos.y;
    currentPosition.z = Hu3DData[object->mdlId[messageId]].pos.z;
    targetPosition.x = input3.x;
    targetPosition.y = input3.y;
    targetPosition.z = input3.z;
    turnDuration = duration4;
    currentRotation.x = Hu3DData[object->mdlId[messageId]].rot.x;
    currentRotation.y = Hu3DData[object->mdlId[messageId]].rot.y;
    currentRotation.z = Hu3DData[object->mdlId[messageId]].rot.z;
    targetModelRotation.x = 0.0f;
    targetModelRotation.y = duration;
    targetModelRotation.z = 0.0f;
    if (targetPosition.x - currentPosition.x >= 0.0f) {
        movingPositiveX = 1;
    } else {
        movingPositiveX = 0;
    }
    if (targetPosition.z - currentPosition.z >= 0.0f) {
        movingPositiveZ = 1;
    } else {
        movingPositiveZ = 0;
    }
    rotationFrameCount = duration5;
    do {
        targetRotation.y = -(atan2d(targetPosition.z - currentPosition.z, targetPosition.x - currentPosition.x) - 90.0);
        if (targetRotation.y < 0.0f) {
            targetRotation.y += 360.0f;
        } else if (targetRotation.y >= 360.0f) {
            targetRotation.y -= 360.0f;
        }
        if (targetRotation.y >= 180.0f) {
            if (currentRotation.y - targetRotation.y >= 180.0f) {
                currentRotation.y -= 360.0f;
            }
            if (currentRotation.y - targetRotation.y < -180.0f) {
                currentRotation.y += 360.0f;
            }
        } else {
            if (currentRotation.y - targetRotation.y > 180.0f) {
                currentRotation.y -= 360.0f;
            }
            if (currentRotation.y - targetRotation.y <= -180.0f) {
                currentRotation.y += 360.0f;
            }
        }
        updatedPosition.x = currentPosition.x += 6.0 * sind(targetRotation.y);
        updatedPosition.z = currentPosition.z += 6.0 * cosd(targetRotation.y);
        if (movingPositiveX != 0) {
            if (currentPosition.x >= targetPosition.x) {
                updatedPosition.x = currentPosition.x = targetPosition.x;
                reachedTargetX = 1;
            }
        } else {
            if (currentPosition.x <= targetPosition.x) {
                updatedPosition.x = currentPosition.x = targetPosition.x;
                reachedTargetX = 1;
            }
        }
        if (movingPositiveZ != 0) {
            if (currentPosition.z >= targetPosition.z) {
                updatedPosition.z = currentPosition.z = targetPosition.z;
                reachedTargetZ = 1;
            }
        } else {
            if (currentPosition.z <= targetPosition.z) {
                updatedPosition.z = currentPosition.z = targetPosition.z;
                reachedTargetZ = 1;
            }
        }
        currentRotation.y = InterpolateSmoothAverage(currentRotation.y, targetRotation.y, 10.0f);
        Hu3DModelPosSet(object->mdlId[1], updatedPosition.x, currentPosition.y, updatedPosition.z);
        Hu3DModelRotSet(object->mdlId[1], currentRotation.x, currentRotation.y, currentRotation.z);
        HuPrcVSleep();
    } while (reachedTargetZ != 1 || reachedTargetX != 1);
    currentRotation.y = Hu3DData[object->mdlId[messageId]].rot.y;
    for (i = 0; i <= rotationFrameCount; i++) {
        if (targetModelRotation.y >= 180.0f) {
            if (currentRotation.y - targetModelRotation.y >= 180.0f) {
                currentRotation.y -= 360.0f;
            }
            if (currentRotation.y - targetModelRotation.y < -180.0f) {
                currentRotation.y += 360.0f;
            }
        } else {
            if (currentRotation.y - targetModelRotation.y > 180.0f) {
                currentRotation.y -= 360.0f;
            }
            if (currentRotation.y - targetModelRotation.y <= -180.0f) {
                currentRotation.y += 360.0f;
            }
        }
        currentRotation.y = InterpolateLinearOverTime(currentRotation.y, targetModelRotation.y, i, rotationFrameCount);
        Hu3DModelRotSet(object->mdlId[1], currentRotation.x, currentRotation.y, currentRotation.z);
        HuPrcVSleep();
    }
}

Void Story3ConfigureModelMaterials(s16 modelIndex) {
    HU3DMODEL* modelData;
    HSFDATA* modelFileData;
    HSFMATERIAL* materialData;
    s16 materialCount;
    s32 i;

    modelData = &Hu3DData[modelIndex];
    modelFileData = modelData->hsf;
    materialCount = modelFileData->materialNum;
    materialData = modelFileData->material;
    for (i = 0; i < materialCount; materialData++, i++) {
        materialData->vtxMode = 1;
        materialData->litColor[0] = 0xFF;
        materialData->litColor[1] = 0xFF;
        materialData->litColor[2] = 0xFF;
    }
}

Void Story3CopyPlayerStats(StoryPlayerStats* playerStats, s32 playerIndex) {
    playerStats->playerIndex = playerIndex;
    playerStats->team = GWPlayerCfg[playerStats->playerIndex].group;
    playerStats->isComputer = GWPlayerCfg[playerStats->playerIndex].iscom;
    playerStats->difficulty = GWPlayerCfg[playerStats->playerIndex].diff;
    playerStats->characterId = GWPlayerCfg[playerStats->playerIndex].character;
    playerStats->controllerPort = GWPlayerCfg[playerStats->playerIndex].pad_idx;
}

s32 Story3CountHumanPlayers(Void) {
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

s32 Story3FindFirstHumanPlayer(Void) {
    s32 candidateControllerPort;
    s32 playerIndex;

    candidateControllerPort = 0;
    do {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (candidateControllerPort == GWPlayerCfg[playerIndex].pad_idx) {
                break;
            }
        }
        if (!GWPlayerCfg[playerIndex].iscom) {
            break;
        }
        candidateControllerPort++;
    } while (candidateControllerPort != 5);
    if (candidateControllerPort == 5) {
        return -1;
    }
    return playerIndex;
}

s32 Story3CreateGlobalLight(s32 lightId) {
    s32 globalLightId = 0;

    globalLightId = Hu3DGLightCreate(0.0f, 700.0f, 1200.0f, 0.0f, -1.0f, 0.0f, lightId, lightId, lightId);
    Hu3DGLightPosAimSet(globalLightId, 0.0f, 700.0f, 1200.0f, 0.0f, 0.0f, 640.0f);
    Hu3DGLightInfinitytSet(globalLightId);
    return globalLightId;
}

Void Story3SetDefaultCamera(Void) {
    Vec cameraPosition = { 0.0f, 3000.0f, 800.0f };
    Vec cameraFocus = { 0.0f, 0.0f, 640.0f };
    Vec cameraUp = { 0.0f, 1.0f, 0.0f };

    Hu3DShadowCreate(45.0f, 2000.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.8f);
    Hu3DShadowPosSet(&cameraPosition, &cameraUp, &cameraFocus);
}

Void Story3ApplyCameraTarget(StoryCameraState* cameraState) {
    cameraState->cameraCenter.x = cameraState->targetCenterX;
    cameraState->cameraCenter.y = cameraState->targetCenterY;
    cameraState->cameraCenter.z = cameraState->targetCenterZ;
    cameraState->cameraRotation.x = cameraState->targetRotationX;
    cameraState->cameraRotation.y = cameraState->targetRotationY;
    cameraState->cameraRotation.z = cameraState->targetRotationZ;
    cameraState->cameraZoom = cameraState->targetCameraZoom;
}

Void Story3ResetCameraTarget(StoryCameraState* cameraState) {
    cameraState->cameraMotionFrame = 0;
    cameraState->targetCenterX = cameraState->cameraCenter.x;
    cameraState->targetCenterY = cameraState->cameraCenter.y;
    cameraState->targetCenterZ = cameraState->cameraCenter.z;
    cameraState->targetRotationX = cameraState->cameraRotation.x;
    cameraState->targetRotationY = cameraState->cameraRotation.y;
    cameraState->targetRotationZ = cameraState->cameraRotation.z;
    cameraState->targetCameraZoom = cameraState->cameraZoom;
}

Void Story3SmoothCameraTowardTarget(StoryCameraState* cameraState, StoryCameraState* targetCameraState, float frame) {
    cameraState->cameraCenter.x = InterpolateSmoothAverage(cameraState->cameraCenter.x, targetCameraState->targetCenterX, frame);
    cameraState->cameraCenter.y = InterpolateSmoothAverage(cameraState->cameraCenter.y, targetCameraState->targetCenterY, frame);
    cameraState->cameraCenter.z = InterpolateSmoothAverage(cameraState->cameraCenter.z, targetCameraState->targetCenterZ, frame);
    cameraState->cameraRotation.x = InterpolateSmoothAverage(cameraState->cameraRotation.x, targetCameraState->targetRotationX, frame);
    cameraState->cameraRotation.y = InterpolateSmoothAverage(cameraState->cameraRotation.y, targetCameraState->targetRotationY, frame);
    cameraState->cameraRotation.z = InterpolateSmoothAverage(cameraState->cameraRotation.z, targetCameraState->targetRotationZ, frame);
    cameraState->cameraZoom = InterpolateSmoothAverage(cameraState->cameraZoom, targetCameraState->targetCameraZoom, frame);
}

Void Story3SmoothCameraInPlace(StoryCameraState* cameraState, float targetValue) {
    Story3SmoothCameraTowardTarget(cameraState, cameraState, targetValue);
}

Void Story3LerpCameraTarget(StoryCameraState* cameraState, StoryCameraState* targetCameraState, float frame, float duration) {
    targetCameraState->targetCenterX = InterpolateLinearOverTime(cameraState->targetCenterX, targetCameraState->cameraCenter.x, frame, duration);
    targetCameraState->targetCenterY = InterpolateLinearOverTime(cameraState->targetCenterY, targetCameraState->cameraCenter.y, frame, duration);
    targetCameraState->targetCenterZ = InterpolateLinearOverTime(cameraState->targetCenterZ, targetCameraState->cameraCenter.z, frame, duration);
    targetCameraState->targetRotationX = InterpolateLinearOverTime(cameraState->targetRotationX, targetCameraState->cameraRotation.x, frame, duration);
    targetCameraState->targetRotationY = InterpolateLinearOverTime(cameraState->targetRotationY, targetCameraState->cameraRotation.y, frame, duration);
    targetCameraState->targetRotationZ = InterpolateLinearOverTime(cameraState->targetRotationZ, targetCameraState->cameraRotation.z, frame, duration);
    targetCameraState->targetCameraZoom = InterpolateLinearOverTime(cameraState->targetCameraZoom, targetCameraState->cameraZoom, frame, duration);
}

Void Story3ApplyLerpCameraTarget(StoryCameraState* cameraState, StoryCameraState* targetCameraState, float frame, float duration) {
    Story3LerpCameraTarget(cameraState, targetCameraState, frame, duration);
    cameraState->cameraCenter.x = targetCameraState->targetCenterX;
    cameraState->cameraCenter.y = targetCameraState->targetCenterY;
    cameraState->cameraCenter.z = targetCameraState->targetCenterZ;
    cameraState->cameraRotation.x = targetCameraState->targetRotationX;
    cameraState->cameraRotation.y = targetCameraState->targetRotationY;
    cameraState->cameraRotation.z = targetCameraState->targetRotationZ;
    cameraState->cameraZoom = targetCameraState->targetCameraZoom;
}

Void Story3LerpAndSmoothCamera(StoryCameraState* cameraState, StoryCameraState* targetCameraState, float frame, float duration, float duration4) {
    Story3LerpCameraTarget(cameraState, targetCameraState, frame, duration);
    Story3SmoothCameraTowardTarget(cameraState, targetCameraState, duration4);
}

Void Story3EaseCameraTarget(StoryCameraState* cameraState, StoryCameraState* targetCameraState, float frame, float duration) {
    targetCameraState->targetCenterX = InterpolateSineEaseIn(cameraState->targetCenterX, targetCameraState->cameraCenter.x, frame, duration);
    targetCameraState->targetCenterY = InterpolateSineEaseIn(cameraState->targetCenterY, targetCameraState->cameraCenter.y, frame, duration);
    targetCameraState->targetCenterZ = InterpolateSineEaseIn(cameraState->targetCenterZ, targetCameraState->cameraCenter.z, frame, duration);
    targetCameraState->targetRotationX = InterpolateSineEaseIn(cameraState->targetRotationX, targetCameraState->cameraRotation.x, frame, duration);
    targetCameraState->targetRotationY = InterpolateSineEaseIn(cameraState->targetRotationY, targetCameraState->cameraRotation.y, frame, duration);
    targetCameraState->targetRotationZ = InterpolateSineEaseIn(cameraState->targetRotationZ, targetCameraState->cameraRotation.z, frame, duration);
    targetCameraState->targetCameraZoom = InterpolateSineEaseIn(cameraState->targetCameraZoom, targetCameraState->cameraZoom, frame, duration);
}

Void Story3ApplyEasedCameraTarget(StoryCameraState* cameraState, StoryCameraState* targetCameraState, float frame, float duration) {
    Story3EaseCameraTarget(cameraState, targetCameraState, frame, duration);
    cameraState->cameraCenter.x = targetCameraState->targetCenterX;
    cameraState->cameraCenter.y = targetCameraState->targetCenterY;
    cameraState->cameraCenter.z = targetCameraState->targetCenterZ;
    cameraState->cameraRotation.x = targetCameraState->targetRotationX;
    cameraState->cameraRotation.y = targetCameraState->targetRotationY;
    cameraState->cameraRotation.z = targetCameraState->targetRotationZ;
    cameraState->cameraZoom = targetCameraState->targetCameraZoom;
}

Void Story3EaseAndSmoothCamera(StoryCameraState* cameraState, StoryCameraState* targetCameraState, float frame, float duration, float duration4) {
    Story3EaseCameraTarget(cameraState, targetCameraState, frame, duration);
    Story3SmoothCameraTowardTarget(cameraState, targetCameraState, duration4);
}

Void Story3CosineEaseCameraTarget(StoryCameraState* cameraState, StoryCameraState* targetCameraState, float frame, float duration) {
    targetCameraState->targetCenterX = InterpolateCosineEaseOut(cameraState->targetCenterX, targetCameraState->cameraCenter.x, frame, duration);
    targetCameraState->targetCenterY = InterpolateCosineEaseOut(cameraState->targetCenterY, targetCameraState->cameraCenter.y, frame, duration);
    targetCameraState->targetCenterZ = InterpolateCosineEaseOut(cameraState->targetCenterZ, targetCameraState->cameraCenter.z, frame, duration);
    targetCameraState->targetRotationX = InterpolateCosineEaseOut(cameraState->targetRotationX, targetCameraState->cameraRotation.x, frame, duration);
    targetCameraState->targetRotationY = InterpolateCosineEaseOut(cameraState->targetRotationY, targetCameraState->cameraRotation.y, frame, duration);
    targetCameraState->targetRotationZ = InterpolateCosineEaseOut(cameraState->targetRotationZ, targetCameraState->cameraRotation.z, frame, duration);
    targetCameraState->targetCameraZoom = InterpolateCosineEaseOut(cameraState->targetCameraZoom, targetCameraState->cameraZoom, frame, duration);
}

Void Story3ApplyCosineEasedCamera(StoryCameraState* cameraState, StoryCameraState* targetCameraState, float frame, float duration) {
    Story3CosineEaseCameraTarget(cameraState, targetCameraState, frame, duration);
    cameraState->cameraCenter.x = targetCameraState->targetCenterX;
    cameraState->cameraCenter.y = targetCameraState->targetCenterY;
    cameraState->cameraCenter.z = targetCameraState->targetCenterZ;
    cameraState->cameraRotation.x = targetCameraState->targetRotationX;
    cameraState->cameraRotation.y = targetCameraState->targetRotationY;
    cameraState->cameraRotation.z = targetCameraState->targetRotationZ;
    cameraState->cameraZoom = targetCameraState->targetCameraZoom;
}

Void Story3CosineEaseAndSmoothCamera(StoryCameraState* cameraState, StoryCameraState* targetCameraState, float frame, float duration, float duration4) {
    Story3CosineEaseCameraTarget(cameraState, targetCameraState, frame, duration);
    Story3SmoothCameraTowardTarget(cameraState, targetCameraState, duration4);
}

Void Story3UpdatePlayerMotion(OMOBJ* object) {
    s32 motionIndex;

    if (object->work[0] == 0) {
        return;
    }
    if (object->work[2] != 0) {
        object->work[2]--;
        return;
    }
    if (object->work[2] == 0 && Hu3DMotionEndCheck(object->mdlId[1])) {
        motionIndex = object->work[1];
        Hu3DMotionShiftSet(object->mdlId[1], object->mtnId[motionIndex], 0.0f, 15.0f, HU3D_MOTATTR_LOOP);
        object->work[0] = object->work[1] = object->work[2] = 0;
    }
}

Void Story3StartObjectMotion(OMOBJ* object, s32 messageId, s32 mode, s32 mode3, s32 mode4) {
    Story3SetModelMotion(object, 1, messageId, mode3, mode4);
    if (mode4 == 0) {
        object->work[0] = 1;
        object->work[1] = mode;
        object->work[2] = mode3 + 1;
    }
}

Void Story3SetMessage(s32 windowId, s32 messageId) {
    HuWinMesSet(windowId, messageId);
}

Void Story3InsertMessage(s32 windowId, s32 messageId, s32 mode) {
    HuWinInsertMesSet(windowId, messageId, mode);
}

Void Story3WaitForMessage(s32 windowId, s32 messageId) {
    Story3DisplayMessageAndWait(windowId, messageId);
}

Void Story3ShowPlayerChoiceMessage(s32 windowId, s32 messageId, s32 mode, s32 mode3) {
    s32 messageLineCount = 0;
    s32 waitForChoiceState = 0;
    s32 i;

    messageLineCount = HuWinKeyWaitNumGet(messageId);
    if (mode == -1 || mode >= messageLineCount - 1) {
        mode = messageLineCount - 1;
    }
    HuWinComKeyReset();
    if (winData[windowId].active_pad == 0xFF) {
        for (i = 0; i < 10; i++) {
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, GWMessDelayGet());
        }
    }
    HuWinMesSet(windowId, messageId);
    for (i = 0; i < messageLineCount; i++) {
        waitForChoiceState = 0;
        do {
            HuPrcVSleep();
        } while (HuWinStatGet(windowId) != 1);
        if (i == mode) {
            HuWinAttrSet(windowId, 0x400);
            waitForChoiceState = 1;
        }
        while (TRUE) {
            HuPrcVSleep();
            if (waitForChoiceState != 0) {
                if (waitForChoiceState == 1 && HuWinStatGet(windowId) != 1) {
                    waitForChoiceState = 2;
                } else if (waitForChoiceState == 2) {
                    HuPrcSleep(mode3);
                    HuWinAttrReset(windowId, 0x400);
                    HuWinKeyWaitEntry(windowId);
                    break;
                }
            } else if (HuWinStatGet(windowId) != 1) {
                break;
            }
        }
    }
    Story3WaitForMessage(windowId, 0);
    HuWinMesSet(windowId, MAKE_MESSID(37, 4));
    HuWinComKeyReset();
}

BOOL Story3ReadPlayerChoice(s32 windowId) {
    s32 choiceAccepted;
    s32 i;

    HuWinComKeyReset();
    if (winData[windowId].active_pad == 0xFF) {
        for (i = 0; i < 10; i++) {
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, GWMessDelayGet());
        }
    }
    choiceAccepted = HuWinChoiceGet(windowId, 0);
    HuWinComKeyReset();
    if (choiceAccepted != 0) {
        choiceAccepted = FALSE;
    } else {
        choiceAccepted = TRUE;
    }
    return choiceAccepted;
}
