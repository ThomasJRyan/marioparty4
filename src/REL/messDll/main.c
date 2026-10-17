#include "REL/executor.h"
#include "game/disp.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/window.h"
#include "game/wipe.h"

#include "ext_math.h"

static void MessageCameraWipeStart(OMOBJ *cameraObject);
static void MessageCameraControl(OMOBJ *cameraObject);
static void MessageBrowserProcess(void);
#if VERSION_PAL
static void MessageBrowserWindowsCreate();
#endif
static s32 MessageDisplayAndWait(s16 messageCategoryIndex, s16 messageIndex);

static HUPROCESS *messageProcessManager;
static OMOBJ *messageCameraObject;
static OMOBJ *messageViewObject;
static u8 messageProcessManagerPadding[0x20];
#if VERSION_PAL
static HUPROCESS *messageBrowserProcess;
#else
static HUPROCESS *messageBrowserProcess;
#endif
#if VERSION_PAL
static s16 languageWindow;
#else
static u8 languageWindowPadding[4];
#endif
static s16 messageListWindow;
static s16 messageDetailWindow;

void ObjectSetup(void)
{
    OSReport("******* SAFObjectSetup *********\n");
    messageProcessManager = omInitObjMan(0x32, 0x2000);
    CRot.x = -20.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    Center.x = 0.0f;
    Center.y = 120.0f;
    Center.z = 0.0f;
    CZoom = 500.0f;
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 45.0f, 20.0f, 15000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    messageCameraObject = omAddObjEx(messageProcessManager, 0, 0x40, 0, -1, MessageCameraWipeStart);
    messageViewObject = omAddObjEx(messageProcessManager, 0x7FDA, 0, 0, -1, omOutView);
#if VERSION_PAL
    GWGameStat.language = 1;
#endif
    HuWinInit(1);
    messageBrowserProcess = HuPrcChildCreate(MessageBrowserProcess, 100, 0x3000, 0, messageProcessManager);
}

static void MessageCameraWipeStart(OMOBJ *cameraObject)
{
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 10);
    cameraObject->objFunc = MessageCameraControl;
}

static void MessageCameraControl(OMOBJ *cameraObject)
{
    Vec cameraPosition;
    Vec cameraOffset;
    Vec rotationAxis;
    Vec rotatedDirection;
    float cameraBank;
    s8 stickInput;

    if (HuPadBtnDown[0] & 0x1000) {
        GWSystem.mg_next = 1;
        omOvlReturnEx(1, 1);
        return;
    }
    CRot.x += -HuPadSubStkY[0] / 20;
    CRot.y += HuPadSubStkX[0] / 20;
    CZoom += HuPadTrigL[0] / 2;
    CZoom -= HuPadTrigR[0] / 2;
    cameraPosition.x = Center.x + CZoom * (sind(CRot.y) * cosd(CRot.x));
    cameraPosition.y = Center.y + CZoom * -sind(CRot.x);
    cameraPosition.z = Center.z + CZoom * (cosd(CRot.y) * cosd(CRot.x));
    cameraOffset.x = Center.x - cameraPosition.x;
    cameraOffset.y = Center.y - cameraPosition.y;
    cameraOffset.z = Center.z - cameraPosition.z;
    rotationAxis.x = sind(CRot.y) * sind(CRot.x);
    rotationAxis.y = cosd(CRot.x);
    rotationAxis.z = cosd(CRot.y) * sind(CRot.x);
    cameraBank = CRot.z;
    rotatedDirection.x = rotationAxis.x * (cameraOffset.x * cameraOffset.x + (1.0f - cameraOffset.x * cameraOffset.x) * cosd(cameraBank))
        + rotationAxis.y * (cameraOffset.x * cameraOffset.y * (1.0 - cosd(cameraBank)) - cameraOffset.z * sind(cameraBank))
        + rotationAxis.z * (cameraOffset.x * cameraOffset.z * (1.0 - cosd(cameraBank)) + cameraOffset.y * sind(cameraBank));
    rotatedDirection.y = rotationAxis.x * (cameraOffset.x * cameraOffset.y * (1.0 - cosd(cameraBank)) + cameraOffset.z * sind(cameraBank))
        + rotationAxis.y * (cameraOffset.y * cameraOffset.y + (1.0f - cameraOffset.y * cameraOffset.y) * cosd(cameraBank))
        + rotationAxis.z * (cameraOffset.y * cameraOffset.z * (1.0 - cosd(cameraBank)) - cameraOffset.x * sind(cameraBank));
    rotatedDirection.z = rotationAxis.x * (cameraOffset.x * cameraOffset.z * (1.0 - cosd(cameraBank)) - cameraOffset.y * sind(cameraBank))
        + rotationAxis.y * (cameraOffset.y * cameraOffset.z * (1.0 - cosd(cameraBank)) + cameraOffset.x * sind(cameraBank))
        + rotationAxis.z * (cameraOffset.z * cameraOffset.z + (1.0f - cameraOffset.z * cameraOffset.z) * cosd(cameraBank));
    VECCrossProduct(&rotationAxis, &cameraOffset, &cameraOffset);
    VECNormalize(&cameraOffset, &cameraOffset);
    stickInput = HuPadSubStkX[0] & 0xF8;
    if (stickInput != 0) {
        Center.x += cameraOffset.x * stickInput * 0.05f;
        Center.y += cameraOffset.y * stickInput * 0.05f;
        Center.z += cameraOffset.z * stickInput * 0.05f;
    }
    VECNormalize(&rotatedDirection, &cameraOffset);
    stickInput = -(HuPadSubStkY[0] & 0xF8);
    if (stickInput != 0) {
        Center.x += cameraOffset.x * stickInput * 0.05f;
        Center.y += cameraOffset.y * stickInput * 0.05f;
        Center.z += cameraOffset.z * stickInput * 0.05f;
    }
}

static char *messageCategoryNames[]
    = { "001_Character_Name_ss", "002_Hidden_Block", "003_Battle_M", "004_Bowser_M", "005_Warp_M", "006_Mushroom_M", "007_Lot_house", "008_Boo_house",
          "009_ItemName", "010_Bord", "011_RoundItem", "012_Spaceamida", "013_Star_M", "014_RollerCoaster", "015_Last5", "016_ItemShop", "017_System",
          "018_ModeSelect", "019_Item", "020_Map2_event", "021_Lucky_Minigame", "022_Bord_Start", "023_Map3_event", "024_Minigame_Name", "025_mg_446",
          "026_debugmes", "027_Party_Mode", "028_SETUP", "029_Miracle", "030_mg_kuppa", "031_Story_Mode", "032_Bowser_Story", "033_Map4_event",
          "034_mg_inst", "035_E3_only", "036_saf", "037_Mg_inst_sys_", "038_Bord_Results", "039_Bord_Results2", "040_Map5_event", "041_Mg_Mode",
          "042_mg_445", "043_mg_447", "044_mg_448", "045_mg_449", "046_mg_450", "047_tutorial", "048_Option_Rooml", "049_Map6_event", "050_charley",
          "051_Present_Room", "052_Extra_Room", "053_Staff_Post", "054_Staff_Name", "055_Opening_Demo", "056_mgex_inst", NULL };

#if VERSION_PAL
char *languageNames[] = {
    "ENGLISH", "GERMANY", "FRENCH", "ITALY", "SPANISH"
};
#endif

static s16 activeMessageWindow = -1;

static void MessageBrowserProcess(void)
{
    s16 messageLengths[256]; // unknown size
    char messageSize[4];
    char messageCountBuffer[4];
    s16 windowId;
    s16 messageIdIndex;
    s16 messageCategoryCount;
#if VERSION_PAL
    s16 hasEndMarker = 0;
#endif
    s16 messageIndex;
    s32 messageOffset;
    s32 selectedMessageIndex;
    s32 pressedButtons;
    char *messageText;

    messageIndex = messageCategoryCount = 0;
    while (messageCategoryNames[messageIndex] != 0) {
        messageText = messageCategoryNames[messageIndex];
        while (*messageText != 0) {
            if (*messageText == '_') {
                *messageText = 0x86;
            }
            messageText++;
        }
        messageLengths[messageIndex] = 0;
        messageOffset = (messageIndex << 16);
        messageIdIndex = 0;
        while (1) {
            if ((s32)MessData_MesPtrGet(messDataPtr, messageOffset + messageIdIndex) == 0) {
                break;
            }
            messageLengths[messageIndex]++;
            messageIdIndex++;
        }
        messageCategoryCount++;
        messageIndex++;
    }
#if VERSION_NTSC
    windowId = HuWinExCreateStyled(-10000.0f, 32.0f, 316, 40, -1, 0);
    HuWinExAnimIn(windowId);
    HuWinAttrSet(windowId, 0x800);
    HuWinMesSpeedSet(windowId, 0);
    HuWinMesSet(windowId, MAKE_MESSID_PTR(messageCategoryNames[0]));
    messageListWindow = HuWinCreate(460.0f, 32.0f, 100, 40, 0);
    HuWinAttrSet(messageListWindow, 0x800);
    HuWinMesSpeedSet(messageListWindow, 0);
    HuWinMesSet(messageListWindow, MAKE_MESSID_PTR("\x1F\x01_\x1F\x02"));
#else
    MAKE_MESSID_PTR("\x1F\x01_\x1F\x02");
    MessageBrowserWindowsCreate();
    HuWinMesSet(messageDetailWindow, (u32)languageNames[0]);
#endif
    selectedMessageIndex = 0;
    messageOffset = 0;
    while (1) {
#if VERSION_PAL
#define _WINDOW messageListWindow
#else
#define _WINDOW messageListWindow
#endif
        HuWinHomeClear(_WINDOW);
        sprintf(messageSize, "%03d", messageOffset + 1);
        HuWinInsertMesSet(_WINDOW, MAKE_MESSID_PTR(messageSize), 0);
        sprintf(messageCountBuffer, "%03d", messageLengths[selectedMessageIndex]);
        HuWinInsertMesSet(_WINDOW, MAKE_MESSID_PTR(messageCountBuffer), 1);
        HuWinMesSet(_WINDOW, MAKE_MESSID_PTR("\x1F\x01_\x1F\x02"));
        pressedButtons = MessageDisplayAndWait(selectedMessageIndex, messageOffset);
#undef _WINDOW
#if VERSION_PAL
        if (pressedButtons & 0x400) {
            HuWinAllKill();
            HuPrcVSleep();
            hasEndMarker++;
            if (hasEndMarker >= 5) {
                hasEndMarker = 0;
            }
            GWGameStat.language = hasEndMarker + 1;
            HuWinInit(1);
            HuPrcVSleep();
            MessageBrowserWindowsCreate();
            HuWinMesSet(messageDetailWindow, (u32)languageNames[hasEndMarker]);
        }
        else
#endif
        if (pressedButtons & 0x20) {
            selectedMessageIndex++;
            if (selectedMessageIndex >= messageCategoryCount) {
                selectedMessageIndex = 0;
            }
            messageOffset = 0;
        }
        else if (pressedButtons & 0x40) {
            selectedMessageIndex--;
            if (selectedMessageIndex < 0) {
                selectedMessageIndex = messageCategoryCount - 1;
            }
            messageOffset = 0;
        }
        else if (pressedButtons & 0x200) {
            messageOffset--;
            if (messageOffset < 0) {
                selectedMessageIndex--;
                if (selectedMessageIndex < 0) {
                    selectedMessageIndex = messageCategoryCount - 1;
                }
                messageOffset = messageLengths[selectedMessageIndex] - 1;
            }
        }
        else {
            messageOffset++;
            if (messageOffset >= messageLengths[selectedMessageIndex]) {
                selectedMessageIndex++;
                if (selectedMessageIndex >= messageCategoryCount) {
                    selectedMessageIndex = 0;
                }
                messageOffset = 0;
            }
        }
        HuPrcVSleep();
    }
}

static char *messagePositionInsertions[] = { "]1^", "]2^", "]3^", "]4^", "]5^", "]6^", "]7^", "]8^" };

#if VERSION_PAL
static void MessageBrowserWindowsCreate()
{
    languageWindow = HuWinExCreateStyled(-10000.0f, 32.0f, 316, 40, -1, 0);
    HuWinExAnimIn(languageWindow);
    HuWinAttrSet(languageWindow, 0x800);
    HuWinMesSpeedSet(languageWindow, 0x0);
    
    messageListWindow = HuWinCreate(460.0f, 32.0f, 100, 40, 0);
    HuWinAttrSet(messageListWindow, 0x800);
    HuWinMesSpeedSet(messageListWindow, 0x0);

    messageDetailWindow = HuWinExCreateStyled(16.0f, 32.0f, 100, 40, -1, 0);
    HuWinExAnimIn(messageDetailWindow);
    HuWinAttrSet(messageDetailWindow, 0x800);
    HuWinMesSpeedSet(messageDetailWindow, 0x0);
}
#endif

static s32 MessageDisplayAndWait(s16 messageCategoryIndex, s16 messageIndex)
{
    WindowData *windowData;
    float messageSize[2];
    s32 messageId;
    s32 hasWaitControl;
    s32 hasEndMarker;
    u8 *messageText;
    s16 characterIndex;

    hasWaitControl = 0;
    hasEndMarker = 0;
#if VERSION_PAL
    HuWinHomeClear(languageWindow);
    HuWinMesSet(languageWindow, MAKE_MESSID_PTR(messageCategoryNames[messageCategoryIndex]));
#else
    HuWinHomeClear(messageListWindow);
    HuWinMesSet(messageListWindow, MAKE_MESSID_PTR(messageCategoryNames[messageCategoryIndex]));
#endif
    HuPrcVSleep();
    if (activeMessageWindow != -1) {
        HuWinKill(activeMessageWindow);
    }
    messageId = MAKE_MESSID(messageCategoryIndex, messageIndex);
    for (characterIndex = 0; characterIndex < 8; characterIndex++) {
        HuWinInsertMesSizeGet(MAKE_MESSID_PTR(messagePositionInsertions[characterIndex]), (s16)characterIndex);
    }
    HuWinMesMaxSizeGet(1, &messageSize[0], messageId);
    if (messageSize[0] <= 16.0f) {
        messageSize[0] = 32.0f;
    }
    activeMessageWindow = HuWinCreate(-10000.0f, -10000.0f, messageSize[0], messageSize[1], (VERSION_PAL) ? 1 : 0);
#if VERSION_PAL
    HuWinMesPalSet(activeMessageWindow, 7, 0, 0, 0);
#endif
    for (characterIndex = 0; characterIndex < 8; characterIndex++) {
        HuWinInsertMesSet(activeMessageWindow, MAKE_MESSID_PTR(messagePositionInsertions[characterIndex]), (s16)characterIndex);
    }
    windowData = &winData[activeMessageWindow];
    windowData->push_key |= (VERSION_PAL) ? 0x760 : 0x360;
    windowData->key_auto = (VERSION_PAL) ? 0x460 : 0x60;
    HuWinMesSet(activeMessageWindow, messageId);
    messageText = MessData_MesPtrGet(messDataPtr, messageId);
    while (*messageText != 0) {
        if (*messageText == 0xF) {
            hasWaitControl = 1;
        }

#if VERSION_PAL
        if (*messageText == 0xFF) {
#else
        if (*messageText == 0) {
#endif
            hasEndMarker = 1;
        }
        messageText++;
    }
    if (hasWaitControl != 0) {
        HuWinMesWait(activeMessageWindow);
        HuWinChoiceGet(activeMessageWindow, 0);
        return windowData->key_down;
    }
    if (hasEndMarker == 0) {
        while (!(HuPadBtnRep[0] & ((VERSION_PAL) ? 0x760 : 0x360))) {
            HuPrcVSleep();
        }
        return HuPadBtnRep[0];
    }
    while (windowData->stat != 0) {
        if (HuPadBtnRep[0] & ((VERSION_PAL) ? 0x460 : 0x60)) {
            return HuPadBtnDown[0];
        }
        HuPrcVSleep();
    }
    return windowData->key_down;
}
