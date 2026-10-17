#include "REL/mentDll.h"
#include "ext_math.h"
#include "game/board/tutorial.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "math.h"

HUPROCESS *objectManagerProcess;
// TODO: unknown type
extern s32 menuLaunchParameters[];

s32 menuSoundEffectIds[][4] = {
    { 0x00000037, 0x00000038, 0x00000036, 0x00000039 },
    { 0x00000043, 0x00000045, 0x00000044, 0x00000043 },
    { 0x00000040, 0x00000042, 0x00000041, 0x00000040 },
    { 0x0000004B, 0x0000004D, 0x0000004C, 0x0000004B },
    { 0x00000046, 0x00000048, 0x00000047, 0x00000046 },
    { 0x0000003E, 0x0000003F, 0x0000003E, 0x0000003E },
    { 0x0000003A, 0x0000003B, 0x0000003C, 0xFFFFFFFF },
};

s32 persistentMessageWindow = -1;

// char cameraDebugHeader[] = ">>>>>>>>>> CAMERA DATA <<<<<<<<<<";
// char cameraCenterFormat[] = "CENTER : %.2f, %.2f, %.2f";
// char cameraRotationFormat[] = "ROT    : %.2f, %.2f, %.2f";
// char cameraZoomFormat[] = "ZOOM   : %.2f";

extern s32 _prolog();
extern void _epilog();
void MentInitializeOverlay(void);
void MentClosePersistentMessageWindow(s32);
typedef void (*VoidFunc)(void);
extern const VoidFunc _ctors[];
extern const VoidFunc _dtors[];

void MentFinishOverlayTransition(OMOBJ *object)
{
    if (WipeStatGet() == 0) {
        object->objFunc = NULL;
        omOvlReturnEx(1, 1);
    }
}

void MentHandleOverlayExitRequest(OMOBJ *object)
{
    if (omSysExitReq != 0) {
        object->objFunc = MentFinishOverlayTransition;
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    }
}

s32 _prolog(void)
{
    const VoidFunc *ctors = _ctors;
    while (*ctors != 0) {
        (**ctors)();
        ctors++;
    }
    MentInitializeOverlay();
    return 0;
}

void _epilog(void)
{
    const VoidFunc *dtors = _dtors;
    while (*dtors != 0) {
        (**dtors)();
        dtors++;
    }
}

void MentInitializeOverlay(void)
{
    _ClearFlag(0x1000BU);
    boardTutorialF = 0;
    objectManagerProcess = omInitObjMan(0x3E, 0x2000);
    omGameSysInit(objectManagerProcess);
    menuLaunchParameters[0] = omovlevtno;
    menuLaunchParameters[1] = omovlstat;
    MentInitializeModeScene(objectManagerProcess);
    omAddObjEx(objectManagerProcess, 0x2000, 0U, 0U, -1, MentHandleOverlayExitRequest);
}

f32 MentInterpolateLinear(f32 startValue, f32 endValue, f32 elapsedFrames, f32 durationFrames)
{
    if (durationFrames <= elapsedFrames) {
        return endValue;
    }
    return startValue + ((elapsedFrames / durationFrames) * (endValue - startValue));
}

f32 MentApproachValue(f32 currentValue, f32 targetValue, f32 smoothingDivisor)
{
    return (targetValue + (currentValue * (smoothingDivisor - 1.0f))) / smoothingDivisor;
}
f32 MentInterpolateCosineEaseOut(f32 startValue, f32 endValue, f32 elapsedFrames, f32 durationFrames)
{
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return (startValue + ((endValue - startValue) * (1.0 - cosd((90.0f / durationFrames) * elapsedFrames))));
}
f32 MentInterpolateSineEaseIn(f32 startValue, f32 endValue, f32 elapsedFrames, f32 durationFrames)
{
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return startValue + (endValue - startValue) * sind((90.0f / durationFrames) * elapsedFrames);
}

f32 MentInterpolateSineWave(f32 centerValue, f32 amplitudeTarget, f32 elapsedFrames, f32 durationFrames)
{
    if (elapsedFrames >= durationFrames) {
        return centerValue;
    }
    return (centerValue + ((amplitudeTarget - centerValue) * sind((360.0f / durationFrames) * elapsedFrames)));
}

void MentSleepTicks(s32 tickCount)
{
    HuPrcSleep(tickCount);
}

void MentSleepFrame(void)
{
    HuPrcVSleep();
}

void MentResetWindowSystem(void)
{
    while (1) {
        HuPrcVSleep();
    }
}

void MentCreateDirectionalLight(void)
{
    s32 createDirectionalLightLightId = 0;

    createDirectionalLightLightId = Hu3DGLightCreate(0.0f, 700.0f, 1200.0f, 0.0f, -1.0f, 0.0f, 255, 255, 255);
    Hu3DGLightPosAimSet(createDirectionalLightLightId, 0.0f, 700.0f, 1200.0f, 0.0f, 0.0f, 640.0f);
    Hu3DGLightInfinitytSet(createDirectionalLightLightId);
}

void MentCreateMenuShadow(s32 gameMode)
{
    Vec createMenuShadowTargetPosition = { 0.0f, 3000.0f, 800.0f };
    Vec createMenuShadowTargetPosition2 = { 0.0f, 0.0f, 640.0f };
    Vec createMenuShadowTargetPosition3 = { 0.0f, 1.0f, 0.0f };

    if (gameMode == 3) {
        createMenuShadowTargetPosition.x = createMenuShadowTargetPosition2.x = 1200.0f;
    }
    Hu3DShadowCreate(45.0f, 2000.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.8f);
    Hu3DShadowPosSet(&createMenuShadowTargetPosition, &createMenuShadowTargetPosition3, &createMenuShadowTargetPosition2);
}

void MentInitializeWindowSystem(void)
{
    HuWinInit(1);
}

void MentAnimateWindowScale(s32 window, f32 centerX, f32 centerY, s32 toSmallF)
{
    WindowData *winPtr;
    f32 smallPosX;
    f32 smallPosY;
    f32 winPosX;
    f32 winPosY;
    f32 width;
    f32 height;
    f32 smallScaleX;
    f32 smallScaleY;
    f32 posX;
    f32 posY;
    f32 scaleX;
    f32 scaleY;
    s32 time;

    winPtr = (WindowData *)&winData[window];
    winPosX = winPtr->pos_x;
    winPosY = winPtr->pos_y;
    width = winPtr->w;
    height = winPtr->h;
    smallPosX = winPosX + (width * centerX);
    smallScaleX = 4.0f / width;

    // adjust horizontal position
    if (((4.0f + smallPosX) - (winPosX + width)) >= 0.0f) {
        smallPosX = (winPosX + width) - 4.0f;
    }
    else {
        if ((winPosX - (smallPosX - 2.0f)) >= 0.0f) {
            smallPosX = winPosX;
        }
        else {
            smallPosX = smallPosX - 2.0f;
        }
    }

    smallPosY = winPosY + (height * centerY);
    smallScaleY = 4.0f / height;

    // adjust vertical position
    if (((4.0f + smallPosY) - (winPosY + height)) >= 0.0f) {
        smallPosY = (winPosY + height) - 4.0f;
    }
    else {
        if ((winPosY - (smallPosY - 2.0f)) >= 0.0f) {
            smallPosY = winPosY;
        }
        else {
            smallPosY = smallPosY - 2.0f;
        }
    }

    if (toSmallF != 0) {
        HuWinPosSet(window, smallPosX, smallPosY);
        HuWinScaleSet(window, smallScaleX, smallScaleY);
        posX = smallPosX;
        scaleX = smallScaleX;
        posY = smallPosY;
        scaleY = smallScaleY;
    }
    else {
        HuWinPosSet(window, winPosX, winPosY);
        HuWinScaleSet(window, 1.0f, 1.0f);
        posX = winPosX;
        scaleX = 1.0f;
        posY = winPosY;
        scaleY = 1.0f;
    }
    HuWinDispOn(window);

    // animation loop
    for (time = 0; time <= 15; ++time) {
        HuPrcVSleep();
        if (toSmallF != 0) {
            if (time <= 0xA) {
                posX = (time >= 10.0f) ? winPosX : (f32)(smallPosX + ((winPosX - smallPosX) * sind(9.0f * time)));
                scaleX = (time >= 10.0f) ? 1.0f : (f32)(smallScaleX + ((1.0f - smallScaleX) * sind(9.0f * time)));
            }
            else {
                posY = ((time - 0xA) >= 5.0f) ? winPosY : (f32)(smallPosY + ((winPosY - smallPosY) * sind(18.0f * (time - 0xA))));
                scaleY = ((time - 0xA) >= 5.0f) ? 1.0f : (f32)(smallScaleY + ((1.0f - smallScaleY) * sind(18.0f * (time - 0xA))));
            }
        }
        else if (time <= 0xA) {
            posY = (time >= 10.0f) ? smallPosY : (f32)(winPosY + ((smallPosY - winPosY) * sind(9.0f * time)));
            scaleY = (time >= 10.0f) ? smallScaleY : (f32)(1.0 + ((smallScaleY - 1.0f) * sind(9.0f * time)));
        }
        else {
            posX = ((time - 0xA) >= 5.0f) ? smallPosX : (f32)(winPosX + ((smallPosX - winPosX) * sind(18.0f * (time - 0xA))));
            scaleX = ((time - 0xA) >= 5.0f) ? smallScaleX : (f32)(1.0 + ((smallScaleX - 1.0f) * sind(18.0f * (time - 0xA))));
        }
        HuWinPosSet(window, posX, posY);
        HuWinScaleSet(window, scaleX, scaleY);
    }

    if (toSmallF != 0) {
        HuWinPosSet(window, winPosX, winPosY);
        HuWinScaleSet(window, 1.0f, 1.0f);
    }
    else {
        HuWinPosSet(window, smallPosX, smallPosY);
        HuWinScaleSet(window, smallScaleX, smallScaleY);
    }
    HuPrcVSleep();
}

s32 MentCreateMessageWindow(s32 x, s32 y, s32 width, s32 height, f32 centerX, f32 centerY, s32 windowStyle, s32 windowFlags, s32 messageId)
{
    f32 createMessageWindowCoordinateValue[2];
    s32 createMessageWindowStateValue2;

    if (windowStyle % 2 == 0) {
        width = (width * 0x15) + 0x10;
        height = (height * 0x1A) + 0x10;
        y = (445.0f - height);
        switch (x) {
            case -1:
                x = 0x10;
                break;
            case 0:
                x = (1.0f + ((576.0f - width) / 2));
                break;
            case 1:
                x = (556.0f - width);
                break;
        }
    }
    else {
        HuWinMesMaxSizeGet(1, createMessageWindowCoordinateValue, messageId);
        width = createMessageWindowCoordinateValue[0];
        height = createMessageWindowCoordinateValue[1];
        x = (556.0f - width);
        y = (445.0f - height);
    }
    createMessageWindowStateValue2 = HuWinExCreateStyled(x, y, width, height, -1, 1);
    if (windowStyle == 1) {
        HuWinAttrSet(createMessageWindowStateValue2, 0x10U);
    }
    HuWinMesPalSet(createMessageWindowStateValue2, 7U, 0U, 0U, 0U);
    winData[createMessageWindowStateValue2].active_pad = 1;
    MentAnimateWindowScale(createMessageWindowStateValue2, centerX, centerY, 1);
    return createMessageWindowStateValue2;
}

s32 MentCreateStandardMessageWindow(s32 alignment, s32 messageId, s32 windowStyle)
{
    return MentCreateMessageWindow(x, 0, 0x15, 2, 0.5f, 0.5f, 2, 0, 0);
}

void MentCloseMessageWindowAtPoint(s32 windowId, f32 centerX, f32 centerY)
{
    HuWinMesSet(windowId, 0x250004);
    HuPrcVSleep();
    MentAnimateWindowScale(windowId, centerX, centerY, 0);
    HuWinExCleanup(windowId);
}

void MentCloseMessageWindow(s32 windowId)
{
    HuWinMesSet(windowId, 0x250004);
    HuPrcVSleep();
    MentAnimateWindowScale(windowId, 0.5f, 0.5f, 0);
    HuWinExCleanup(windowId);
}

void MentWaitForMessageWindow(s32 windowId, s32 waitFrames)
{
    if ((waitFrames == -999) || (waitFrames == 0)) {
        return;
    }
    if (waitFrames == -1000) {
        do {
            HuPrcVSleep();
        } while (HuWinStatGet(windowId) == 1);
        return;
    }
    if (waitFrames > 0) {
        HuPrcSleep(waitFrames);
        return;
    }
    HuWinComKeyReset();
    HuWinMesWait(windowId);
    HuWinComKeyReset();
}

void MentWaitForMessageWindowAdvance(s32 windowId, s32 messageId, s32 highlightIndex, s32 waitFrames)
{
    s32 i;
    s32 waitForMessageWindowAdvanceStateValue = 0;
    s32 waitForMessageWindowAdvanceStateValue2 = 0;

    (void)windowId;

    waitForMessageWindowAdvanceStateValue = HuWinKeyWaitNumGet(messageId);
    if ((highlightIndex == -1) || (highlightIndex >= (s32)(waitForMessageWindowAdvanceStateValue - 1))) {
        highlightIndex = waitForMessageWindowAdvanceStateValue - 1;
    }
    HuWinComKeyReset();
    for (i = 0; i < waitForMessageWindowAdvanceStateValue; i++) {
        waitForMessageWindowAdvanceStateValue2 = 0;
        do {
            HuPrcVSleep();
        } while (HuWinStatGet(windowId) != 1);

        if (i == highlightIndex) {
            HuWinAttrSet(windowId, 0x400);
            waitForMessageWindowAdvanceStateValue2 = 1;
        }

        while (1) {
            HuPrcVSleep();
            if (waitForMessageWindowAdvanceStateValue2 != 0) {
                if (waitForMessageWindowAdvanceStateValue2 == 1 && HuWinStatGet(windowId) != 1) {
                    waitForMessageWindowAdvanceStateValue2 = 2;
                }
                else if (waitForMessageWindowAdvanceStateValue2 == 2) {
                    HuPrcSleep(waitFrames);
                    HuWinAttrReset(windowId, 0x400);
                    HuWinKeyWaitEntry(windowId);
                    break;
                }
            }
            else if (HuWinStatGet(windowId) != 1) {
                break;
            }
        }
    }
    HuWinComKeyReset();
    HuWinMesWait(windowId);
    HuWinComKeyReset();
    HuWinComKeyReset();
}

s32 MentGetWindowChoice(s32 windowId, s32 choiceCount)
{
    s32 getWindowChoiceChoice;

    HuWinComKeyReset();
    getWindowChoiceChoice = HuWinChoiceGet(windowId, choiceCount);
    HuWinComKeyReset();
    return getWindowChoiceChoice;
}

void MentSetWindowMessageAndWait(s32 windowId, s32 messageId, s32 waitFrames)
{
    HuWinInsertMesSet(windowId, messageId, waitFrames);
}

void MentSetWindowMessage(s32 windowId, s32 messageId, s32 waitFrames, s32 style)
{
    if (style == -999) {
        HuWinMesSpeedSet(windowId, 0);
    }
    else {
        HuWinMesSpeedSet(windowId, 1);
    }
    HuWinMesSet(windowId, messageId);
    if (waitFrames == -1) {
        if ((style != -999) && (style != 0)) {
            if (style == -1000) {
                do {
                    HuPrcVSleep();
                } while (HuWinStatGet(windowId) == 1);
                return;
            }
            if (style > 0) {
                HuPrcSleep(style);
                return;
            }
            HuWinComKeyReset();
            HuWinMesWait(windowId);
            HuWinComKeyReset();
        }
    }
    else {
        MentWaitForMessageWindowAdvance(windowId, messageId, highlightIndex, waitFrames);
    }
}

s32 MentShowYesNoPrompt(s32 messageId, s32 promptStyle, s32 defaultChoice)
{
    s32 showYesNoPromptStateValue;
    s32 showYesNoPromptStateValue2 = 0;

    showYesNoPromptStateValue = MentCreateMessageWindow(0, 0, 0, 0, 0.5f, 0.5f, y, 0, x);
    HuWinMesSet(showYesNoPromptStateValue, messageId);
    showYesNoPromptStateValue2 = MentGetWindowChoice(showYesNoPromptStateValue, 0);
    MentCloseMessageWindowAtPoint(showYesNoPromptStateValue, 0.5f, 0.5f);
    return showYesNoPromptStateValue2;
}

s32 MentShowAlternateYesNoPrompt(s32 messageId, s32 promptStyle, s32 defaultChoice)
{
    s32 showAlternateYesNoPromptStateValue;
    s32 showAlternateYesNoPromptStateValue2 = 0;

    showAlternateYesNoPromptStateValue = MentCreateMessageWindow(0, 0, 0, 0, 0.5f, 0.5f, y, 0, x);
    HuWinMesSet(showAlternateYesNoPromptStateValue, messageId);
    showAlternateYesNoPromptStateValue2 = MentGetWindowChoice(showAlternateYesNoPromptStateValue, 1);
    MentCloseMessageWindowAtPoint(showAlternateYesNoPromptStateValue, 0.5f, 0.5f);
    return showAlternateYesNoPromptStateValue2;
}

s32 MentShowMessageWindow(s32 messageId)
{
    f32 showMessageWindowCoordinateValue[2];
    f32 showMessageWindowInterpolationValue;
    f32 showMessageWindowInterpolationValue2;
    s32 ret;
    f32 sizeX;
    f32 sizeY;

    HuWinMesMaxSizeGet(1, showMessageWindowCoordinateValue, messageId);
    sizeX = showMessageWindowCoordinateValue[0];
    sizeY = showMessageWindowCoordinateValue[1];
    showMessageWindowInterpolationValue = (576.0f - sizeX) / 2;
    showMessageWindowInterpolationValue2 = 385.0f - sizeY;
    if (persistentMessageWindow == -1) {
        persistentMessageWindow = ret = HuWinExCreateStyled(showMessageWindowInterpolationValue, showMessageWindowInterpolationValue2, sizeX, sizeY, -1, 1);
    }
    else {
        MentClosePersistentMessageWindow(0);
        persistentMessageWindow = ret = HuWinExCreateStyled(showMessageWindowInterpolationValue, showMessageWindowInterpolationValue2, sizeX, sizeY, -1, 1);
    }
    HuWinBGTPLvlSet(persistentMessageWindow, 0.0f);
    HuWinMesSet(persistentMessageWindow, messageId);
    HuWinMesSpeedSet(persistentMessageWindow, 0);
    HuWinDispOn(persistentMessageWindow);
    return persistentMessageWindow;
}

void MentClosePersistentMessageWindow(s32 unused)
{
    if (persistentMessageWindow != -1) {
        HuWinExCleanup(persistentMessageWindow);
        persistentMessageWindow = -1;
    }
}

void MentUpdateDebugCameraControls(f32 unused)
{
    MentDllCameraWork *cameraWork = &menuCameraWork;

    if ((HuPadBtn[0] & 0x200)) {
        if ((HuPadBtn[0] & 0x20)) {
            cameraWork->center.z = cameraWork->center.z - (HuPadStkY[0] / 10.0f);
        }
        else {
            cameraWork->center.x += HuPadStkX[0] / 10.0f;
            cameraWork->center.y = cameraWork->center.y + (HuPadStkY[0] / 10.0f);
        }
    }
    if ((HuPadBtn[0] & 0x800)) {
        cameraWork->rot.x -= HuPadStkY[0] / 100.0f;
        cameraWork->rot.y = cameraWork->rot.y + (HuPadStkX[0] / 100.0f);
        if (cameraWork->rot.x < 0.0f) {
            cameraWork->rot.x += 360.0f;
        }

        if (cameraWork->rot.x >= 360.0f) {
            cameraWork->rot.x -= 360.0f;
        }
        if (cameraWork->rot.y < 0.0f) {
            cameraWork->rot.y += 360.0f;
        }
        if (cameraWork->rot.y >= 360.0f) {
            cameraWork->rot.y -= 360.0f;
        }
    }
    if ((HuPadBtn[0] & 0x400)) {
        cameraWork->zoom = cameraWork->zoom - (HuPadStkY[0] / 10.0f);
    }
    print8(0x18, 0x28, 1.0f, ">>>>>>>>>> CAMERA DATA <<<<<<<<<<");
    print8(0x18, 0x32, 1.0f, "CENTER : %.2f, %.2f, %.2f", cameraWork->center.x, cameraWork->center.y, cameraWork->center.z);
    print8(0x18, 0x3C, 1.0f, "ROT    : %.2f, %.2f, %.2f", cameraWork->rot.x, cameraWork->rot.y, cameraWork->rot.z);
    print8(0x18, 0x46, 1.0f, "ZOOM   : %.2f", cameraWork->zoom);
}

void MentApplyMenuCamera(OMOBJ *object)
{
    MentDllCameraWork *cameraWork = &menuCameraWork;
    if (cameraWork->cameraUpdateCallback != NULL) {
        cameraWork->cameraUpdateCallback();
    }
    Center.x = cameraWork->center.x;
    Center.y = cameraWork->center.y;
    Center.z = cameraWork->center.z;
    CRot.x = cameraWork->rot.x;
    CRot.y = cameraWork->rot.y;
    CRot.z = cameraWork->rot.z;
    CZoom = cameraWork->zoom;
}

void MentSetMenuCameraCallback(void (*cameraUpdateCallback)(void))
{
    MentSetCameraState(&menuCameraWork);
    menuCameraWork.cameraUpdateCallback = cameraUpdateCallback;
}

void MentCreateMenuCamera(HUPROCESS *objectManagerProcess, void (*cameraUpdateCallback)(void))
{
    Hu3DCameraCreate(1);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 42.0f, 20.0f, 5000.0f, 1.2f);
    menuCameraWork.cameraUpdateCallback = cameraUpdateCallback;
    menuCameraWork.debugControlCallback = &MentUpdateDebugCameraControls;
    omAddObjEx(objectManagerProcess, 0x7FDA, 0U, 0U, -1, omOutView);
    omAddObjEx(objectManagerProcess, 0x7FD9, 0U, 0U, -1, MentApplyMenuCamera);
}

void MentRestorePreviousCamera(MentDllCameraWork *camera)
{
    camera->center.x = camera->prevCenter.x;
    camera->center.y = camera->prevCenter.y;
    camera->center.z = camera->prevCenter.z;
    camera->rot.x = camera->prevRot.x;
    camera->rot.y = camera->prevRot.y;
    camera->rot.z = camera->prevRot.z;
    camera->zoom = camera->prevZoom;
}

void MentSetCameraState(MentDllCameraWork *camera)
{
    camera->transitionFrame = 0.0f;
    camera->prevCenter.x = camera->center.x;
    camera->prevCenter.y = camera->center.y;
    camera->prevCenter.z = camera->center.z;
    camera->prevRot.x = camera->rot.x;
    camera->prevRot.y = camera->rot.y;
    camera->prevRot.z = camera->rot.z;
    camera->prevZoom = camera->zoom;
}

static inline f32 SmoothInterpolate(f32 currentValue, f32 targetValue, f32 smoothingDivisor)
{
    return (targetValue + currentValue * (smoothingDivisor - 1.0f)) / smoothingDivisor;
}

void MentInterpolateCameraState(MentDllCameraWork *sourceCamera, MentDllCameraWork *targetCamera, f32 blendAmount)
{
    sourceCamera->center.x = SmoothInterpolate(sourceCamera->center.x, targetCamera->prevCenter.x, blendAmount);
    sourceCamera->center.y = SmoothInterpolate(sourceCamera->center.y, targetCamera->prevCenter.y, blendAmount);
    sourceCamera->center.z = SmoothInterpolate(sourceCamera->center.z, targetCamera->prevCenter.z, blendAmount);
    sourceCamera->rot.x = SmoothInterpolate(sourceCamera->rot.x, targetCamera->prevRot.x, blendAmount);
    sourceCamera->rot.y = SmoothInterpolate(sourceCamera->rot.y, targetCamera->prevRot.y, blendAmount);
    sourceCamera->rot.z = SmoothInterpolate(sourceCamera->rot.z, targetCamera->prevRot.z, blendAmount);
    sourceCamera->zoom = SmoothInterpolate(sourceCamera->zoom, targetCamera->prevZoom, blendAmount);
}

void MentCopyCameraState(MentDllCameraWork *camera, f32 blendAmount)
{
    camera->center.x = SmoothInterpolate(camera->center.x, camera->prevCenter.x, blendAmount);
    camera->center.y = SmoothInterpolate(camera->center.y, camera->prevCenter.y, blendAmount);
    camera->center.z = SmoothInterpolate(camera->center.z, camera->prevCenter.z, blendAmount);
    camera->rot.x = SmoothInterpolate(camera->rot.x, camera->prevRot.x, blendAmount);
    camera->rot.y = SmoothInterpolate(camera->rot.y, camera->prevRot.y, blendAmount);
    camera->rot.z = SmoothInterpolate(camera->rot.z, camera->prevRot.z, blendAmount);
    camera->zoom = SmoothInterpolate(camera->zoom, camera->prevZoom, blendAmount);
}

inline f32 LinearInterpolation(f32 startValue, f32 endValue, f32 elapsedFrames, f32 durationFrames)
{
    if (durationFrames <= elapsedFrames) {
        return endValue;
    }
    else {
        return startValue + ((elapsedFrames / durationFrames) * (endValue - startValue));
    }
}

void MentInterpolateCameraCenter(MentDllCameraWork *sourceCamera, MentDllCameraWork *targetCamera, f32 elapsedFrames, f32 durationFrames)
{
    targetCamera->prevCenter.x = LinearInterpolation(sourceCamera->prevCenter.x, targetCamera->center.x, elapsedFrames, durationFrames);
    targetCamera->prevCenter.y = LinearInterpolation(sourceCamera->prevCenter.y, targetCamera->center.y, elapsedFrames, durationFrames);
    targetCamera->prevCenter.z = LinearInterpolation(sourceCamera->prevCenter.z, targetCamera->center.z, elapsedFrames, durationFrames);

    targetCamera->prevRot.x = LinearInterpolation(sourceCamera->prevRot.x, targetCamera->rot.x, elapsedFrames, durationFrames);
    targetCamera->prevRot.y = LinearInterpolation(sourceCamera->prevRot.y, targetCamera->rot.y, elapsedFrames, durationFrames);
    targetCamera->prevRot.z = LinearInterpolation(sourceCamera->prevRot.z, targetCamera->rot.z, elapsedFrames, durationFrames);

    targetCamera->prevZoom = LinearInterpolation(sourceCamera->prevZoom, targetCamera->zoom, elapsedFrames, durationFrames);
}

void MentInterpolateCameraRotation(MentDllCameraWork *sourceCamera, MentDllCameraWork *targetCamera, f32 elapsedFrames, f32 durationFrames)
{
    targetCamera->prevCenter.x = LinearInterpolation(sourceCamera->prevCenter.x, targetCamera->center.x, elapsedFrames, durationFrames);
    targetCamera->prevCenter.y = LinearInterpolation(sourceCamera->prevCenter.y, targetCamera->center.y, elapsedFrames, durationFrames);
    targetCamera->prevCenter.z = LinearInterpolation(sourceCamera->prevCenter.z, targetCamera->center.z, elapsedFrames, durationFrames);

    targetCamera->prevRot.x = LinearInterpolation(sourceCamera->prevRot.x, targetCamera->rot.x, elapsedFrames, durationFrames);
    targetCamera->prevRot.y = LinearInterpolation(sourceCamera->prevRot.y, targetCamera->rot.y, elapsedFrames, durationFrames);
    targetCamera->prevRot.z = LinearInterpolation(sourceCamera->prevRot.z, targetCamera->rot.z, elapsedFrames, durationFrames);

    targetCamera->prevZoom = LinearInterpolation(sourceCamera->prevZoom, targetCamera->zoom, elapsedFrames, durationFrames);

    sourceCamera->center.x = targetCamera->prevCenter.x;
    sourceCamera->center.y = targetCamera->prevCenter.y;
    sourceCamera->center.z = targetCamera->prevCenter.z;
    sourceCamera->rot.x = targetCamera->prevRot.x;
    sourceCamera->rot.y = targetCamera->prevRot.y;
    sourceCamera->rot.z = targetCamera->prevRot.z;
    sourceCamera->zoom = targetCamera->prevZoom;
}

void MentInterpolateCameraZoom(MentDllCameraWork *sourceCamera, MentDllCameraWork *targetCamera, f32 elapsedFrames, f32 durationFrames, f32 smoothingDivisor)
{
    targetCamera->prevCenter.x = LinearInterpolation(sourceCamera->prevCenter.x, targetCamera->center.x, elapsedFrames, durationFrames);
    targetCamera->prevCenter.y = LinearInterpolation(sourceCamera->prevCenter.y, targetCamera->center.y, elapsedFrames, durationFrames);
    targetCamera->prevCenter.z = LinearInterpolation(sourceCamera->prevCenter.z, targetCamera->center.z, elapsedFrames, durationFrames);

    targetCamera->prevRot.x = LinearInterpolation(sourceCamera->prevRot.x, targetCamera->rot.x, elapsedFrames, durationFrames);
    targetCamera->prevRot.y = LinearInterpolation(sourceCamera->prevRot.y, targetCamera->rot.y, elapsedFrames, durationFrames);
    targetCamera->prevRot.z = LinearInterpolation(sourceCamera->prevRot.z, targetCamera->rot.z, elapsedFrames, durationFrames);

    targetCamera->prevZoom = LinearInterpolation(sourceCamera->prevZoom, targetCamera->zoom, elapsedFrames, durationFrames);

    sourceCamera->center.x = SmoothInterpolate(sourceCamera->center.x, targetCamera->prevCenter.x, smoothingDivisor);
    sourceCamera->center.y = SmoothInterpolate(sourceCamera->center.y, targetCamera->prevCenter.y, smoothingDivisor);
    sourceCamera->center.z = SmoothInterpolate(sourceCamera->center.z, targetCamera->prevCenter.z, smoothingDivisor);

    sourceCamera->rot.x = SmoothInterpolate(sourceCamera->rot.x, targetCamera->prevRot.x, smoothingDivisor);
    sourceCamera->rot.y = SmoothInterpolate(sourceCamera->rot.y, targetCamera->prevRot.y, smoothingDivisor);
    sourceCamera->rot.z = SmoothInterpolate(sourceCamera->rot.z, targetCamera->prevRot.z, smoothingDivisor);

    sourceCamera->zoom = SmoothInterpolate(sourceCamera->zoom, targetCamera->prevZoom, smoothingDivisor);
}

void MentMoveCameraCenter(MentDllCameraWork *sourceCamera, MentDllCameraWork *targetCamera, f32 elapsedFrames, f32 durationFrames)
{
    targetCamera->prevCenter.x = MentInterpolateSineEaseIn(sourceCamera->prevCenter.x, targetCamera->center.x, startValue, endValue);
    targetCamera->prevCenter.y = MentInterpolateSineEaseIn(sourceCamera->prevCenter.y, targetCamera->center.y, startValue, endValue);
    targetCamera->prevCenter.z = MentInterpolateSineEaseIn(sourceCamera->prevCenter.z, targetCamera->center.z, startValue, endValue);

    targetCamera->prevRot.x = MentInterpolateSineEaseIn(sourceCamera->prevRot.x, targetCamera->rot.x, startValue, endValue);
    targetCamera->prevRot.y = MentInterpolateSineEaseIn(sourceCamera->prevRot.y, targetCamera->rot.y, startValue, endValue);
    targetCamera->prevRot.z = MentInterpolateSineEaseIn(sourceCamera->prevRot.z, targetCamera->rot.z, startValue, endValue);

    targetCamera->prevZoom = MentInterpolateSineEaseIn(sourceCamera->prevZoom, targetCamera->zoom, startValue, endValue);
}

void MentRotateCamera(MentDllCameraWork *sourceCamera, MentDllCameraWork *targetCamera, f32 elapsedFrames, f32 durationFrames)
{
    targetCamera->prevCenter.x = MentInterpolateSineEaseIn(sourceCamera->prevCenter.x, targetCamera->center.x, startValue, endValue);
    targetCamera->prevCenter.y = MentInterpolateSineEaseIn(sourceCamera->prevCenter.y, targetCamera->center.y, startValue, endValue);
    targetCamera->prevCenter.z = MentInterpolateSineEaseIn(sourceCamera->prevCenter.z, targetCamera->center.z, startValue, endValue);

    targetCamera->prevRot.x = MentInterpolateSineEaseIn(sourceCamera->prevRot.x, targetCamera->rot.x, startValue, endValue);
    targetCamera->prevRot.y = MentInterpolateSineEaseIn(sourceCamera->prevRot.y, targetCamera->rot.y, startValue, endValue);
    targetCamera->prevRot.z = MentInterpolateSineEaseIn(sourceCamera->prevRot.z, targetCamera->rot.z, startValue, endValue);

    targetCamera->prevZoom = MentInterpolateSineEaseIn(sourceCamera->prevZoom, targetCamera->zoom, startValue, endValue);

    sourceCamera->center.x = targetCamera->prevCenter.x;
    sourceCamera->center.y = targetCamera->prevCenter.y;
    sourceCamera->center.z = targetCamera->prevCenter.z;

    sourceCamera->rot.x = targetCamera->prevRot.x;
    sourceCamera->rot.y = targetCamera->prevRot.y;
    sourceCamera->rot.z = targetCamera->prevRot.z;

    sourceCamera->zoom = targetCamera->prevZoom;
}

void MentAnimateCameraState(MentDllCameraWork *sourceCamera, MentDllCameraWork *targetCamera, f32 elapsedFrames, f32 durationFrames, f32 smoothingDivisor)
{
    targetCamera->prevCenter.x = MentInterpolateSineEaseIn(sourceCamera->prevCenter.x, targetCamera->center.x, startValue, endValue);
    targetCamera->prevCenter.y = MentInterpolateSineEaseIn(sourceCamera->prevCenter.y, targetCamera->center.y, startValue, endValue);
    targetCamera->prevCenter.z = MentInterpolateSineEaseIn(sourceCamera->prevCenter.z, targetCamera->center.z, startValue, endValue);

    targetCamera->prevRot.x = MentInterpolateSineEaseIn(sourceCamera->prevRot.x, targetCamera->rot.x, startValue, endValue);
    targetCamera->prevRot.y = MentInterpolateSineEaseIn(sourceCamera->prevRot.y, targetCamera->rot.y, startValue, endValue);
    targetCamera->prevRot.z = MentInterpolateSineEaseIn(sourceCamera->prevRot.z, targetCamera->rot.z, startValue, endValue);

    targetCamera->prevZoom = MentInterpolateSineEaseIn(sourceCamera->prevZoom, targetCamera->zoom, startValue, endValue);

    sourceCamera->center.x = SmoothInterpolate(sourceCamera->center.x, targetCamera->prevCenter.x, smoothingDivisor);
    sourceCamera->center.y = SmoothInterpolate(sourceCamera->center.y, targetCamera->prevCenter.y, smoothingDivisor);
    sourceCamera->center.z = SmoothInterpolate(sourceCamera->center.z, targetCamera->prevCenter.z, smoothingDivisor);

    sourceCamera->rot.x = SmoothInterpolate(sourceCamera->rot.x, targetCamera->prevRot.x, smoothingDivisor);
    sourceCamera->rot.y = SmoothInterpolate(sourceCamera->rot.y, targetCamera->prevRot.y, smoothingDivisor);
    sourceCamera->rot.z = SmoothInterpolate(sourceCamera->rot.z, targetCamera->prevRot.z, smoothingDivisor);

    sourceCamera->zoom = SmoothInterpolate(sourceCamera->zoom, targetCamera->prevZoom, smoothingDivisor);
}

void MentAnimateCameraCenter(MentDllCameraWork *sourceCamera, MentDllCameraWork *targetCamera, f32 elapsedFrames, f32 durationFrames)
{
    targetCamera->prevCenter.x = MentInterpolateCosineEaseOut(sourceCamera->prevCenter.x, targetCamera->center.x, startValue, endValue);
    targetCamera->prevCenter.y = MentInterpolateCosineEaseOut(sourceCamera->prevCenter.y, targetCamera->center.y, startValue, endValue);
    targetCamera->prevCenter.z = MentInterpolateCosineEaseOut(sourceCamera->prevCenter.z, targetCamera->center.z, startValue, endValue);

    targetCamera->prevRot.x = MentInterpolateCosineEaseOut(sourceCamera->prevRot.x, targetCamera->rot.x, startValue, endValue);
    targetCamera->prevRot.y = MentInterpolateCosineEaseOut(sourceCamera->prevRot.y, targetCamera->rot.y, startValue, endValue);
    targetCamera->prevRot.z = MentInterpolateCosineEaseOut(sourceCamera->prevRot.z, targetCamera->rot.z, startValue, endValue);

    targetCamera->prevZoom = MentInterpolateCosineEaseOut(sourceCamera->prevZoom, targetCamera->zoom, startValue, endValue);
}

void MentAnimateCameraRotation(MentDllCameraWork *sourceCamera, MentDllCameraWork *targetCamera, f32 elapsedFrames, f32 durationFrames)
{
    targetCamera->prevCenter.x = MentInterpolateCosineEaseOut(sourceCamera->prevCenter.x, targetCamera->center.x, startValue, endValue);
    targetCamera->prevCenter.y = MentInterpolateCosineEaseOut(sourceCamera->prevCenter.y, targetCamera->center.y, startValue, endValue);
    targetCamera->prevCenter.z = MentInterpolateCosineEaseOut(sourceCamera->prevCenter.z, targetCamera->center.z, startValue, endValue);

    targetCamera->prevRot.x = MentInterpolateCosineEaseOut(sourceCamera->prevRot.x, targetCamera->rot.x, startValue, endValue);
    targetCamera->prevRot.y = MentInterpolateCosineEaseOut(sourceCamera->prevRot.y, targetCamera->rot.y, startValue, endValue);
    targetCamera->prevRot.z = MentInterpolateCosineEaseOut(sourceCamera->prevRot.z, targetCamera->rot.z, startValue, endValue);

    targetCamera->prevZoom = MentInterpolateCosineEaseOut(sourceCamera->prevZoom, targetCamera->zoom, startValue, endValue);

    sourceCamera->center.x = targetCamera->prevCenter.x;
    sourceCamera->center.y = targetCamera->prevCenter.y;
    sourceCamera->center.z = targetCamera->prevCenter.z;

    sourceCamera->rot.x = targetCamera->prevRot.x;
    sourceCamera->rot.y = targetCamera->prevRot.y;
    sourceCamera->rot.z = targetCamera->prevRot.z;

    sourceCamera->zoom = targetCamera->prevZoom;
}

void MentAnimateCameraZoom(MentDllCameraWork *sourceCamera, MentDllCameraWork *targetCamera, f32 elapsedFrames, f32 durationFrames, f32 smoothingDivisor)
{
    targetCamera->prevCenter.x = MentInterpolateCosineEaseOut(sourceCamera->prevCenter.x, targetCamera->center.x, startValue, endValue);
    targetCamera->prevCenter.y = MentInterpolateCosineEaseOut(sourceCamera->prevCenter.y, targetCamera->center.y, startValue, endValue);
    targetCamera->prevCenter.z = MentInterpolateCosineEaseOut(sourceCamera->prevCenter.z, targetCamera->center.z, startValue, endValue);

    targetCamera->prevRot.x = MentInterpolateCosineEaseOut(sourceCamera->prevRot.x, targetCamera->rot.x, startValue, endValue);
    targetCamera->prevRot.y = MentInterpolateCosineEaseOut(sourceCamera->prevRot.y, targetCamera->rot.y, startValue, endValue);
    targetCamera->prevRot.z = MentInterpolateCosineEaseOut(sourceCamera->prevRot.z, targetCamera->rot.z, startValue, endValue);

    targetCamera->prevZoom = MentInterpolateCosineEaseOut(sourceCamera->prevZoom, targetCamera->zoom, startValue, endValue);

    sourceCamera->center.x = SmoothInterpolate(sourceCamera->center.x, targetCamera->prevCenter.x, smoothingDivisor);
    sourceCamera->center.y = SmoothInterpolate(sourceCamera->center.y, targetCamera->prevCenter.y, smoothingDivisor);
    sourceCamera->center.z = SmoothInterpolate(sourceCamera->center.z, targetCamera->prevCenter.z, smoothingDivisor);

    sourceCamera->rot.x = SmoothInterpolate(sourceCamera->rot.x, targetCamera->prevRot.x, smoothingDivisor);
    sourceCamera->rot.y = SmoothInterpolate(sourceCamera->rot.y, targetCamera->prevRot.y, smoothingDivisor);
    sourceCamera->rot.z = SmoothInterpolate(sourceCamera->rot.z, targetCamera->prevRot.z, smoothingDivisor);

    sourceCamera->zoom = SmoothInterpolate(sourceCamera->zoom, targetCamera->prevZoom, smoothingDivisor);
}

void MentSetModelMotion(OMOBJ *object, s32 modelIndex, s32 motionIndex, s32 transitionFrames, s32 motionMode)
{
    switch (motionMode) {
        case 0:
            Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, transitionFrames, 0);
            break;
        case 1:
            Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, transitionFrames, 0x40000001);
            break;
        case 2:
            Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, transitionFrames, 0x40000002);
            break;
    }
    object->work[0] = object->work[1] = object->work[2] = 0;
}

void MentSetModelMotionIfChanged(OMOBJ *object, s32 modelIndex, s32 motionIndex, s32 transitionFrames, s32 motionMode)
{
    if (object->work[3] != object->mtnId[motionIndex]) {
        object->work[3] = object->mtnId[motionIndex];
        switch (motionMode) {
            case 0:
                Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, transitionFrames, 0);
                break;
            case 1:
                Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, transitionFrames, 0x40000001);
                break;
            case 2:
                Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, transitionFrames, 0x40000002);
                break;
        }
        object->work[0] = object->work[1] = object->work[2] = 0;
    }
}

void MentStartModelMotionTimer(OMOBJ *object, s32 motionIndex, s32 nextMotionIndex, s32 timerFrames)
{
    (void)motionIndex;
    Hu3DMotionShiftSet(object->mdlId[1], object->mtnId[motionIndex], 0.0f, timerFrames, 0);
    object->work[0] = object->work[1] = object->work[2] = 0;
    object->work[0] = 1;
    object->work[1] = nextMotionIndex;
    object->work[2] = timerFrames + 1;
}

void MentUpdateModelMotionTimer(OMOBJ *object)
{
    s32 cameraWork;

    if (object->work[0] != 0) {
        if (object->work[2] != 0) {
            object->work[2] -= 1;
            return;
        }
        if ((object->work[2] == 0) && (Hu3DMotionEndCheck(object->mdlId[1]) != 0)) {
            cameraWork = object->work[1];
            Hu3DMotionShiftSet(object->mdlId[1], object->mtnId[cameraWork], 0.0f, 15.0f, 0x40000001);
            object->work[0] = object->work[1] = object->work[2] = 0;
            object->work[0] = object->work[1] = object->work[2] = 0;
        }
    }
}

void MentWaitForModelMotion(OMOBJ *object, s32 modelIndex, s32 waitFrames)
{
    HuPrcSleep(waitFrames + 1);
    do {
        HuPrcVSleep();
    } while (Hu3DMotionEndCheck(object->mdlId[modelIndex]) == 0);
}

void MentMoveAndTurnModelToTarget(OMOBJ *object, s32 modelIndex, Vec targetPosition, float targetYaw, float moveSpeed, float rotationFrames, s32 moveEnabled, s32 turnEnabled)
{
    s32 moveAndTurnModelToTargetStateValue = 0;
    s32 moveAndTurnModelToTargetStateValue2 = 0;

    Vec moveAndTurnModelToTargetTargetPosition;
    Vec moveAndTurnModelToTargetTargetPosition2;
    Vec moveAndTurnModelToTargetTargetPosition3;
    Vec moveAndTurnModelToTargetTargetPosition4;
    Vec moveAndTurnModelToTargetTargetPosition5;
    Vec moveAndTurnModelToTargetTargetPosition6;
    moveAndTurnModelToTargetTargetPosition2.x = Hu3DData[object->mdlId[modelIndex]].pos.x;
    moveAndTurnModelToTargetTargetPosition2.y = Hu3DData[object->mdlId[modelIndex]].pos.y;
    moveAndTurnModelToTargetTargetPosition2.z = Hu3DData[object->mdlId[modelIndex]].pos.z;
    moveAndTurnModelToTargetTargetPosition3.x = targetPosition.x;
    moveAndTurnModelToTargetTargetPosition3.y = targetPosition.y;
    moveAndTurnModelToTargetTargetPosition3.z = targetPosition.z;
    moveAndTurnModelToTargetTargetPosition5.x = Hu3DData[object->mdlId[modelIndex]].rot.x;
    moveAndTurnModelToTargetTargetPosition5.y = Hu3DData[object->mdlId[modelIndex]].rot.y;
    moveAndTurnModelToTargetTargetPosition5.z = Hu3DData[object->mdlId[modelIndex]].rot.z;
    moveAndTurnModelToTargetTargetPosition6.x = 0;
    moveAndTurnModelToTargetTargetPosition6.y = targetYaw;
    moveAndTurnModelToTargetTargetPosition6.z = 0;
    if (moveAndTurnModelToTargetTargetPosition3.x - moveAndTurnModelToTargetTargetPosition2.x >= 0.0f) {
        moveAndTurnModelToTargetStateValue = 1;
    }
    else {
        moveAndTurnModelToTargetStateValue = 0;
    }
    if (moveAndTurnModelToTargetTargetPosition3.z - moveAndTurnModelToTargetTargetPosition2.z >= 0.0f) {
        moveAndTurnModelToTargetStateValue2 = 1;
    }
    else {
        moveAndTurnModelToTargetStateValue2 = 0;
    }
    while (moveEnabled) {
        MentSleepFrame();
        moveAndTurnModelToTargetTargetPosition4.y = -(atan2d(moveAndTurnModelToTargetTargetPosition3.z - moveAndTurnModelToTargetTargetPosition2.z, moveAndTurnModelToTargetTargetPosition3.x - moveAndTurnModelToTargetTargetPosition2.x) - 90);
        if (moveAndTurnModelToTargetTargetPosition4.y < 0.0f) {
            moveAndTurnModelToTargetTargetPosition4.y += 360.0f;
        }
        else if (moveAndTurnModelToTargetTargetPosition4.y >= 360.0f) {
            moveAndTurnModelToTargetTargetPosition4.y -= 360.0f;
        }
        if (turnEnabled) {
            if (moveAndTurnModelToTargetTargetPosition4.y >= 180.0f) {
                if (moveAndTurnModelToTargetTargetPosition5.y - moveAndTurnModelToTargetTargetPosition4.y >= 180.0f) {
                    moveAndTurnModelToTargetTargetPosition5.y -= 360.0f;
                }
                if (moveAndTurnModelToTargetTargetPosition5.y - moveAndTurnModelToTargetTargetPosition4.y < -180.0f) {
                    moveAndTurnModelToTargetTargetPosition5.y += 360.0f;
                }
            }
            else {
                if (moveAndTurnModelToTargetTargetPosition5.y - moveAndTurnModelToTargetTargetPosition4.y > 180.0f) {
                    moveAndTurnModelToTargetTargetPosition5.y -= 360.0f;
                }
                if (moveAndTurnModelToTargetTargetPosition5.y - moveAndTurnModelToTargetTargetPosition4.y <= -180.0f) {
                    moveAndTurnModelToTargetTargetPosition5.y += 360.0f;
                }
            }
        }
        moveAndTurnModelToTargetTargetPosition.x = moveAndTurnModelToTargetTargetPosition2.x = moveAndTurnModelToTargetTargetPosition2.x + (moveSpeed * sind(moveAndTurnModelToTargetTargetPosition4.y));
        moveAndTurnModelToTargetTargetPosition.z = moveAndTurnModelToTargetTargetPosition2.z = moveAndTurnModelToTargetTargetPosition2.z + (moveSpeed * cosd(moveAndTurnModelToTargetTargetPosition4.y));
        if (moveAndTurnModelToTargetStateValue == 1) {
            if (moveAndTurnModelToTargetTargetPosition2.x >= moveAndTurnModelToTargetTargetPosition3.x) {
                moveAndTurnModelToTargetTargetPosition.x = moveAndTurnModelToTargetTargetPosition2.x = moveAndTurnModelToTargetTargetPosition3.x;
                moveAndTurnModelToTargetStateValue = 2;
            }
        }
        else if (moveAndTurnModelToTargetStateValue == 0) {
            if (moveAndTurnModelToTargetTargetPosition2.x <= moveAndTurnModelToTargetTargetPosition3.x) {
                moveAndTurnModelToTargetTargetPosition.x = moveAndTurnModelToTargetTargetPosition2.x = moveAndTurnModelToTargetTargetPosition3.x;
                moveAndTurnModelToTargetStateValue = 2;
            }
        }
        if (moveAndTurnModelToTargetStateValue2 == 1) {
            if (moveAndTurnModelToTargetTargetPosition2.z >= moveAndTurnModelToTargetTargetPosition3.z) {
                moveAndTurnModelToTargetTargetPosition.z = moveAndTurnModelToTargetTargetPosition2.z = moveAndTurnModelToTargetTargetPosition3.z;
                moveAndTurnModelToTargetStateValue2 = 2;
            }
        }
        else if (moveAndTurnModelToTargetStateValue2 == 0) {
            if (moveAndTurnModelToTargetTargetPosition2.z <= moveAndTurnModelToTargetTargetPosition3.z) {
                moveAndTurnModelToTargetTargetPosition.z = moveAndTurnModelToTargetTargetPosition2.z = moveAndTurnModelToTargetTargetPosition3.z;
                moveAndTurnModelToTargetStateValue2 = 2;
            }
        }
        if (turnEnabled) {
            moveAndTurnModelToTargetTargetPosition5.y = MentApproachValue(moveAndTurnModelToTargetTargetPosition5.y, moveAndTurnModelToTargetTargetPosition4.y, smoothingDivisor);
        }
        Hu3DModelPosSet(object->mdlId[modelIndex], moveAndTurnModelToTargetTargetPosition.x, moveAndTurnModelToTargetTargetPosition2.y, moveAndTurnModelToTargetTargetPosition.z);
        Hu3DModelRotSet(object->mdlId[modelIndex], moveAndTurnModelToTargetTargetPosition5.x, moveAndTurnModelToTargetTargetPosition5.y, moveAndTurnModelToTargetTargetPosition5.z);
        if (moveAndTurnModelToTargetStateValue == 2 && moveAndTurnModelToTargetStateValue2 == 2) {
            break;
        }
    }
    moveAndTurnModelToTargetTargetPosition5.y = Hu3DData[object->mdlId[modelIndex]].rot.y;
    if (turnEnabled) {
        s32 moveAndTurnModelToTargetIndex;
        for (moveAndTurnModelToTargetIndex = 0; moveAndTurnModelToTargetIndex <= rotationFrames; moveAndTurnModelToTargetIndex++) {
            MentSleepFrame();
            if (moveAndTurnModelToTargetTargetPosition6.y >= 180.0f) {
                if (moveAndTurnModelToTargetTargetPosition5.y - moveAndTurnModelToTargetTargetPosition6.y >= 180.0f) {
                    moveAndTurnModelToTargetTargetPosition5.y -= 360.0f;
                }
                if (moveAndTurnModelToTargetTargetPosition5.y - moveAndTurnModelToTargetTargetPosition6.y < -180.0f) {
                    moveAndTurnModelToTargetTargetPosition5.y += 360.0f;
                }
            }
            else {
                if (moveAndTurnModelToTargetTargetPosition5.y - moveAndTurnModelToTargetTargetPosition6.y > 180.0f) {
                    moveAndTurnModelToTargetTargetPosition5.y -= 360.0f;
                }
                if (moveAndTurnModelToTargetTargetPosition5.y - moveAndTurnModelToTargetTargetPosition6.y <= -180.0f) {
                    moveAndTurnModelToTargetTargetPosition5.y += 360.0f;
                }
            }
            moveAndTurnModelToTargetTargetPosition6.z = MentInterpolateLinear(moveAndTurnModelToTargetTargetPosition5.y, moveAndTurnModelToTargetTargetPosition6.y, moveAndTurnModelToTargetIndex, elapsedFrames);
            Hu3DModelRotSet(object->mdlId[modelIndex], moveAndTurnModelToTargetTargetPosition5.x, moveAndTurnModelToTargetTargetPosition6.z, moveAndTurnModelToTargetTargetPosition5.z);
        }
    }
}

void MentSetSpritePosition(s32 spriteGroup, s32 spriteIndex, float positionX, float positionY)
{
    HuSprPosSet(spriteGroup, spriteIndex, positionX, positionY + 0.5f);
}
