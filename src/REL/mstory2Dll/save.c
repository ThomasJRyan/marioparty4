#include "REL/mstory2Dll.h"
#include "game/data.h"
#include "game/esprite.h"
#include "game/flag.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/saveload.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

s32 storySaveComplete;
HUPROCESS *storySaveProcess;

s32 IsStorySaveSlotUnlocked(s32 saveSlotIndex)
{
    if (saveSlotIndex == 0 && _CheckFlag(FLAG_ID_MAKE(0, 2))) {
        return 1;
    }
    if (saveSlotIndex == 1 && _CheckFlag(FLAG_ID_MAKE(0, 3))) {
        return 1;
    }
    if (saveSlotIndex == 2 && _CheckFlag(FLAG_ID_MAKE(0, 4))) {
        return 1;
    }
    if (saveSlotIndex == 3 && _CheckFlag(FLAG_ID_MAKE(0, 5))) {
        return 1;
    }
    if (saveSlotIndex == 4 && _CheckFlag(FLAG_ID_MAKE(0, 6))) {
        return 1;
    }
    if (saveSlotIndex == 5 && _CheckFlag(FLAG_ID_MAKE(0, 7))) {
        return 1;
    }
    return 0;
}

Void RunStorySavePrompt(Void)
{
    float messageSize[2];
    s32 saveChoice;
    s32 saveNoticeSpriteId;
    s32 windowId;

    saveChoice = 1;
    Hu3DAllKill();
    HuSprClose();
    HuSprInit();
    if (SLSaveFlagGet() == 0) {
        storySaveComplete = 1;
        HuPrcEnd();
        while (TRUE) {
            HuPrcVSleep();
        }
    }
    espInit();
    HuPrcVSleep();
    saveNoticeSpriteId = espEntry(DATA_MAKE_NUM(DATADIR_WIN, 32), 5000, 0);
    espPosSet(saveNoticeSpriteId, 288.0f, 240.0f);
    espAttrReset(saveNoticeSpriteId, 4);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 20);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    if (SLSaveFlagGet() == 1) {
        HuWinInit(1);
        HuWinMesMaxSizeGet(1, messageSize, MAKE_MESSID(30, 22));
        windowId = HuWinExCreateStyled(-10000.0f, 150.0f, messageSize[0], messageSize[1], -1, 2);
        winData[windowId].active_pad = 1;
        HuWinAttrSet(windowId, 0x10);
        HuWinExAnimIn(windowId);
        HuWinMesSet(windowId, MAKE_MESSID(30, 22));
        saveChoice = HuWinChoiceGet(windowId, 0);
        HuWinExAnimOut(windowId);
        HuWinExCleanup(windowId);
        if (SLSaveFlagGet() == 1 && saveChoice == 0) {
#if VERSION_REV1 || VERSION_REV2
            GWGameStat.story_continue = 1;
#endif
            if (omovlstat) {
                GWGameStat.story_continue = 0;
            }
            SLCommonSet();
            SLSaveBoardStory();
            SLSave();
        }
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 20);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    storySaveComplete = 1;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

Void FinishStorySaveProcess(Void)
{
    _SetFlag(9);
    HuPrcChildCreate(RunStorySavePrompt, 100, 0x2000, 0, HuPrcCurrentGet());
    do {
        WaitOneFrame();
    } while (storySaveComplete != 1);
    if (omovlstat == 0) {
        omOvlGotoEx(DLL_mstory2dll, 1, 4, 0);
    }
    else {
        omOvlReturnEx(omovlhisidx, 1);
    }
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

Void StartStorySaveProcess(HUPROCESS *parentProcess)
{
    storySaveProcess = parentProcess;
    HuPrcChildCreate(FinishStorySaveProcess, 100, 0x2000, 0, HuPrcCurrentGet());
}
