#include "REL/mstoryDll.h"
#include "game/data.h"
#include "game/esprite.h"
#include "game/flag.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/saveload.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

s32 saveProcessComplete;
HUPROCESS *storySaveProcess;

s32 IsBoardStoryFlagSet(s32 boardId)
{
    if (boardId == 0 && _CheckFlag(FLAG_ID_MAKE(0, 2))) {
        return 1;
    }
    if (boardId == 1 && _CheckFlag(FLAG_ID_MAKE(0, 3))) {
        return 1;
    }
    if (boardId == 2 && _CheckFlag(FLAG_ID_MAKE(0, 4))) {
        return 1;
    }
    if (boardId == 3 && _CheckFlag(FLAG_ID_MAKE(0, 5))) {
        return 1;
    }
    if (boardId == 4 && _CheckFlag(FLAG_ID_MAKE(0, 6))) {
        return 1;
    }
    if (boardId == 5 && _CheckFlag(FLAG_ID_MAKE(0, 7))) {
        return 1;
    }
    return 0;
}

Void ShowStorySavePrompt(Void)
{
    float savePromptDimensions[2];
    s32 saveChoice;
    s32 saveNoticeSpriteId;
    s32 saveWindowId;

    saveChoice = 1;
    Hu3DAllKill();
    HuSprClose();
    HuSprInit();
    if (SLSaveFlagGet() == 0) {
        saveProcessComplete = 1;
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
        HuWinMesMaxSizeGet(1, savePromptDimensions, MAKE_MESSID(30, 22));
        saveWindowId = HuWinExCreateStyled(-10000.0f, 150.0f, savePromptDimensions[0], savePromptDimensions[1], -1, 2);
        winData[saveWindowId].active_pad = 1;
        HuWinAttrSet(saveWindowId, 0x10);
        HuWinExAnimIn(saveWindowId);
        HuWinMesSet(saveWindowId, MAKE_MESSID(30, 22));
        saveChoice = HuWinChoiceGet(saveWindowId, 0);
        HuWinExAnimOut(saveWindowId);
        HuWinExCleanup(saveWindowId);
        if (SLSaveFlagGet() == 1 && saveChoice == 0) {
#if VERSION_REV1 || VERSION_REV2
            GWGameStat.story_continue = 1;
#endif
            SLCommonSet();
            SLSaveBoardStory();
            SLSave();
        }
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 20);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    saveProcessComplete = 1;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

Void SelectNextStoryOverlay(Void)
{
    _SetFlag(9);
    if (_CheckFlag(FLAG_ID_MAKE(0, 2)) && _CheckFlag(FLAG_ID_MAKE(0, 3)) && _CheckFlag(FLAG_ID_MAKE(0, 4)) && _CheckFlag(FLAG_ID_MAKE(0, 5))
        && _CheckFlag(FLAG_ID_MAKE(0, 6))) {
        OSReport(
            "########## Next to MapSelect Event 11 -> MapNo:%d, MapFlg:%d, MgFlg:%d\n", GWSystem.board, IsBoardStoryFlagSet(GWSystem.board), _CheckFlag(9));
        HuPrcChildCreate(ShowStorySavePrompt, 100, 0x2000, 0, HuPrcCurrentGet());
        do {
            WaitOneFrame();
        } while (saveProcessComplete != 1);
        omOvlGotoEx(DLL_mstory2dll, 1, 0, 0);
    }
    else {
        OSReport(
            "########## Next to MapSelect Event 11 -> MapNo:%d, MapFlg:%d, MgFlg:%d\n", GWSystem.board, IsBoardStoryFlagSet(GWSystem.board), _CheckFlag(9));
        HuPrcChildCreate(ShowStorySavePrompt, 100, 0x2000, 0, HuPrcCurrentGet());
        do {
            WaitOneFrame();
        } while (saveProcessComplete != 1);
        omOvlGotoEx(DLL_mentdll, 1, 10, 0);
    }
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

Void StartStorySaveSequence(HUPROCESS *process)
{
    storySaveProcess = process;
    HuPrcChildCreate(SelectNextStoryOverlay, 100, 0x2000, 0, HuPrcCurrentGet());
}
