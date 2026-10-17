#include "REL/mstory2Dll.h"
#include "game/armem.h"
#include "game/board/main.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/esprite.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/saveload.h"
#include "game/thpmain.h"
#endif

typedef struct {
    /* 0x000 */ s32 selectedPlayerIndex;
    /* 0x004 */ s32 activeWindowId;
    /* 0x008 */ s32 eventWindowId;
    /* 0x00C */ s32 activePadIndex;
    /* 0x010 */ s32 playerOrderCount;
    /* 0x014 */ s32 reserved14;
    /* 0x018 */ char reserved18[0x18];
    /* 0x030 */ StoryPlayerConfig playerInfo[4];
    /* 0x100 */ float verticalOffset;
} StoryEndingState; // Size 0x104

StoryEndingState storyEndingState;
StoryObjectProcessState storyEndingMainProcess;
StoryObjectProcessState storyEndingMapProcess;
StoryObjectProcessState storyEndingPlayerProcess;
StoryObjectProcessState storyEndingSubProcess;
StoryObjectProcessState storyEndingKoopaProcess;
BOOL storyEndingMovieComplete;
BOOL storyEndingMotionState;
HUPROCESS *storyEndingProcess;

s32 storyEndingMotionCueFrames[2][2] = { 20, 70, 1, 1 };
s32 storyEndingMotionId = -1;
s32 storyEndingCueMotionIds[2] = { -1, -1 };
s32 storyEndingCueVolume = 127;

Void RunEndingMainProcess(Void);
Void RunEndingMapProcess(Void);
Void AnimateEndingCharacter(OMOBJ *object, StoryObjectProcessState *processState);
Void RunEndingCharacterProcess(Void);
Void AnimateEndingKoopa(OMOBJ *object, StoryObjectProcessState *processState);
Void RunEndingKoopaProcess(Void);
Void AnimateEndingPlayer(OMOBJ *object, StoryObjectProcessState *processState);
Void RunEndingPlayerProcess(Void);
Void PrepareEndingPlayers(int selectedPlayerIndex);
Void HandleEndingProcessEnd(Void);
Void UpdateEndingCamera(Void);
Void AnimateEndingCamera(Void);

Void WaitForEndingMotionCues(Void)
{
    int i;
    while (1) {
        HuPrcVSleep();
        if (storyEndingMotionId != -1 && storyEndingCueMotionIds[0] != -1 && storyEndingCueMotionIds[1] != -1 && storyEndingCueMotionIds[0] == Hu3DMotionIDGet(storyEndingMotionId)) {
            for (i = 0; i < 2; i++) {
                if (storyEndingMotionCueFrames[1][i] != -1) {
                    if (Hu3DMotionTimeGet(storyEndingMotionId) >= storyEndingMotionCueFrames[0][i]) {
                        OSReport("KE-System  TIME:%d\n", storyEndingMotionCueFrames[0][i]);
                        HuAudFXPlayVol(81, storyEndingCueVolume);
                        storyEndingMotionCueFrames[1][i] = -1;
                    }
                }
            }
            if (Hu3DMotionTimeGet(storyEndingMotionId) <= 10) {
                for (i = 0; i < 2; i++) {
                    storyEndingMotionCueFrames[1][i] = 1;
                }
            }
        }
        if (storyEndingMotionState == 1) {
            storyEndingCueVolume--;
            if (storyEndingCueVolume <= 0) {
                storyEndingCueVolume = 0;
            }
        }
        if (storyEndingMotionState == 2) {
            break;
        }
    }
    HuPrcEnd();
    while (1) {
        HuPrcVSleep();
    }
}

Void StartStoryEnding(HUPROCESS *parentProcess, s32 activePadIndex)
{
    storyEndingProcess = parentProcess;
    if (GWSystem.diff_story == 2) {
        GWGameStat.veryHardUnlock = TRUE;
    }
    storyEndingState.activePadIndex = activePadIndex;
    InitializeWindowSystem();
    CreateStoryDirectionalLight(255);
    InitializeStoryCamera(storyEndingProcess, HandleEndingProcessEnd);
    CreateStoryShadow();
    PrepareEndingPlayers(activePadIndex);
    HuPrcChildCreate(RunEndingPlayerProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunEndingMapProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunEndingKoopaProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunEndingCharacterProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunEndingMainProcess, 100, 12288, 0, HuPrcCurrentGet());
}

Void UpdateEndingCharacterMotion(OMOBJ *object)
{
    if (object->work[2]) {
        object->work[2]--;
        return;
    }
    if (object->work[2] == 0 && Hu3DMotionEndCheck(object->mdlId[1])) {
        ShiftObjectMotion(object, 1, object->work[1], 15, 1);
        object->work[0] = object->work[1] = object->work[2] = 0;
    }
}

Void DisplayEndingDebugStatus(Void)
{
    int posX = 16;
    int posY = 40;
    GXColor winColor = { 0, 0, 128, 128 };
    int i;
    while (1) {
        WaitOneFrame();
        fontcolor = FONT_COLOR_WHITE;
        printWin(posX, posY, 220, 130, &winColor);
        if (storyEndingMainProcess.updateCallback == NULL) {
            print8(posX, posY, 1, "PROC_MAIN   -> FALSE");
        }
        else {
            print8(posX, posY, 1, "PROC_MAIN   -> TRUE");
        }
        if (storyEndingMapProcess.updateCallback == NULL) {
            print8(posX, posY + 10, 1, "PROC_MAP    -> FALSE");
        }
        else {
            print8(posX, posY + 10, 1, "PROC_MAP    -> TRUE");
        }
        if (storyEndingPlayerProcess.updateCallback == NULL) {
            print8(posX, posY + 20, 1, "PROC_PLAYER -> FALSE");
        }
        else {
            print8(posX, posY + 20, 1, "PROC_PLAYER -> TRUE");
        }
        if (storyEndingSubProcess.updateCallback == NULL) {
            print8(posX, posY + 30, 1, "PROC_SUB    -> FALSE");
        }
        else {
            print8(posX, posY + 30, 1, "PROC_SUB    -> TRUE");
        }
        if (storyEndingKoopaProcess.updateCallback == NULL) {
            print8(posX, posY + 40, 1, "PROC_KOOPA  -> FALSE");
        }
        else {
            print8(posX, posY + 40, 1, "PROC_KOOPA  -> TRUE");
        }

        print8(posX, posY + 60, 1, "P:%d W:%d", storyEndingState.selectedPlayerIndex, storyEndingState.activePadIndex);
        print8(posX, posY + 80, 1, "NO) IDX:GRP:COM:DIF:PAD:CHR");
        for (i = 0; i < 4; i++) {
            print8(posX, posY + 90 + (i * 10), 1, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", i, storyEndingState.playerInfo[i].playerIndex, storyEndingState.playerInfo[i].group,
                storyEndingState.playerInfo[i].isComputer, storyEndingState.playerInfo[i].difficulty, storyEndingState.playerInfo[i].padIndex, storyEndingState.playerInfo[i].characterId);
        }
    }
}

Void BeginStoryEndingScene(Void)
{
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet());
    WaitFrames(60);
}

Void ShowEndingIntroduction(Void)
{
    storyEndingState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, storyEndingState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[6][1]);
    SetWindowMessage(storyEndingState.activeWindowId, 0x1F001E);
    WaitForWindowMessage(storyEndingState.activeWindowId, -1);
    SetWindowMessage(storyEndingState.activeWindowId, 0x250004);
    CloseCenteredMessageWindow(storyEndingState.activeWindowId);
    storyEndingKoopaProcess.updateCallback = (Void *)AnimateEndingCharacter;
    storyEndingMotionState = 1;
    WaitFrames(60);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet());
    WaitFrames(60);
    storyEndingKoopaProcess.updateCallback = NULL;
    storyEndingMotionState = 2;
    ShiftObjectMotion(storyEndingKoopaProcess.object, 1, 1, 0, 1);
    SetCameraUpdateCallback(UpdateEndingCamera);
    Hu3DModelAttrReset(storyEndingMapProcess.object->mdlId[1], 1);
    Hu3DModelAttrReset(storyEndingMapProcess.object->mdlId[2], 1);
    Hu3DModelAttrReset(storyEndingMapProcess.object->mdlId[4], 1);
    Hu3DModelAttrReset(storyEndingSubProcess.object->mdlId[1], 1);
    Hu3DModelAttrReset(storyEndingPlayerProcess.object->mdlId[1], 1);
    Hu3DModelAttrSet(storyEndingKoopaProcess.object->mdlId[1], 1);
    Hu3DModelPosSet(storyEndingMapProcess.object->mdlId[1], 0, 0, 0);
    WaitFrames(60);
    storyAudioSequenceIds[0] = HuAudSeqPlay(56);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet());
    if (0) {
        WaitOneFrame();
    }
}

Void AnimateEndingCharacters(Void)
{
    ShiftObjectMotion(storyEndingMapProcess.object, 2, 3, 0, 0);
    WaitForObjectMotionEnd(storyEndingMapProcess.object, 2, 3, 0);
    storyEndingPlayerProcess.updateCallback = (Void *)AnimateEndingPlayer;
    storyEndingSubProcess.updateCallback = (Void *)AnimateEndingKoopa;
    CaptureCameraMotionStart(&storyCameraState);
    SetCameraUpdateCallback(AnimateEndingCamera);
    WaitFrames(30);
    ShiftObjectMotion(storyEndingMapProcess.object, 2, 2, 0, 0);
    while (1) {
        WaitOneFrame();
        if (storyEndingPlayerProcess.updateCallback == NULL) {
            break;
        }
    }
    while (1) {
        WaitOneFrame();
        if (storyEndingSubProcess.updateCallback == NULL) {
            break;
        }
    }
    storyEndingState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, storyEndingState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[0][2]);
    ShiftObjectMotion(storyEndingSubProcess.object, 1, 3, 15, 1);
    SetWindowMessage(storyEndingState.activeWindowId, 0x1F001F);
    WaitForWindowMessage(storyEndingState.activeWindowId, -1);
    SetWindowMessage(storyEndingState.activeWindowId, 0x1F0020);
    WaitForWindowMessage(storyEndingState.activeWindowId, -1);
    SetWindowMessage(storyEndingState.activeWindowId, 0x250004);
    ShiftObjectMotion(storyEndingSubProcess.object, 1, 1, 15, 1);
    CloseCenteredMessageWindow(storyEndingState.activeWindowId);
    WaitFrames(60);
}

char *storyEndingMovieFiles[8] = { "movie/endmov_ma0.thp", "movie/endmov_lu0.thp", "movie/endmov_pe0.thp", "movie/endmov_yo0.thp", "movie/endmov_wa0.thp",
    "movie/endmov_do0.thp", "movie/endmov_da0.thp", "movie/endmov_wl0.thp" };

Void ShowEndingCharacterMessage(Void)
{
    storyEndingState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, storyEndingState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[0][2]);
    ShiftObjectMotion(storyEndingSubProcess.object, 1, 3, 0, 1);
    InsertWindowMessage(storyEndingState.activeWindowId, storyEndingState.playerInfo[storyEndingState.selectedPlayerIndex].characterId, 0);
    SetWindowMessage(storyEndingState.activeWindowId, 0x1F0024);
    WaitForWindowMessage(storyEndingState.activeWindowId, -1);
    SetWindowMessage(storyEndingState.activeWindowId, 0x250004);
    ShiftObjectMotion(storyEndingSubProcess.object, 1, 1, 0, 1);
    CloseCenteredMessageWindow(storyEndingState.activeWindowId);
    ShiftObjectMotion(storyEndingPlayerProcess.object, 1, 4, 15, 0);
    WaitForObjectMotionEnd(storyEndingPlayerProcess.object, 1, 4, 15);
    ShiftObjectMotion(storyEndingPlayerProcess.object, 1, 3, 15, 1);
    WaitFrames(60);
    storyEndingState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, storyEndingState.activePadIndex);
    ShiftObjectMotion(storyEndingSubProcess.object, 1, 3, 0, 1);
    WaitForWindowMessageAndMotionSimple(storyEndingState.activeWindowId, 0x1F0023, 0, -1);
    SetWindowMessage(storyEndingState.activeWindowId, 0x250004);
    ShiftObjectMotion(storyEndingSubProcess.object, 1, 1, 0, 1);
    HuAudFXPlay(storyAudioCueIds[0][3]);
    CloseCenteredMessageWindow(storyEndingState.activeWindowId);
    WaitFrames(60);
    HuAudFXPlay(152);
    Hu3DModelAttrSet(storyEndingMapProcess.object->mdlId[1], 1);
    Hu3DModelAttrSet(storyEndingMapProcess.object->mdlId[2], 1);
    Hu3DModelAttrSet(storyEndingMapProcess.object->mdlId[4], 1);
    Hu3DModelAttrSet(storyEndingSubProcess.object->mdlId[1], 1);
    Hu3DModelAttrSet(storyEndingPlayerProcess.object->mdlId[1], 1);
    HuAudSeqFadeOut(storyAudioSequenceIds[0], 100);
    storyEndingMotionState = 2;
    GWGameStat.musicAllF = TRUE;
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet()) {
        HuPrcVSleep();
    }
    storyEndingMapProcess.updateCallback = NULL;
    storyEndingPlayerProcess.updateCallback = NULL;
    storyEndingKoopaProcess.updateCallback = NULL;
    storyEndingSubProcess.updateCallback = NULL;

    CharModelKill(-1);
    Hu3DAllKill();
    WaitFrames(60);
    {
        int time = 0;
#if VERSION_PAL
        s32 languageOffset = 0;
#endif
        int gid = HuSprGrpCreate(3);
        int sprid = HuTHPSprCreate(storyEndingMovieFiles[storyEndingState.playerInfo[storyEndingState.selectedPlayerIndex].characterId], FALSE, 1000);
        HuSprGrpMemberSet(gid, 0, sprid);
        HuSprPosSet(gid, 0, 288, 240);
#if VERSION_PAL
        switch (GWGameStat.language) {
            case 2:
                languageOffset = 1;
                break;
            case 3:
                languageOffset = 2;
                break;
            case 4:
                languageOffset = 3;
                break;
            case 5:
                languageOffset = 4;
                break;
        }
        sprid = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x60) + languageOffset, HU_MEMNUM_OVL)), 950, 0);
#else
        sprid = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x60), HU_MEMNUM_OVL)), 950, 0);
#endif
        HuSprGrpMemberSet(gid, 1, sprid);
        HuSprPosSet(gid, 1, 288, 226);
        HuSprAttrSet(gid, 1, HUSPR_ATTR_DISPOFF);
        sprid = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x61 : 0x65), HU_MEMNUM_OVL)), 950, 0);
        HuSprGrpMemberSet(gid, 2, sprid);
        HuSprBankSet(gid, 2, storyEndingState.playerInfo[storyEndingState.selectedPlayerIndex].characterId);
        if (GWLanguageGet() == 0 && storyEndingState.playerInfo[storyEndingState.selectedPlayerIndex].characterId == 5) {
            HuSprBankSet(gid, 2, 8);
        }
        HuSprPosSet(gid, 2, 288, 254);
        HuSprAttrSet(gid, 2, HUSPR_ATTR_DISPOFF);
        HuPrcSleep(5);
        WipeColorSet(0, 0, 0);
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 10);
        while (WipeStatGet()) {
            HuPrcVSleep();
        }
        while (!HuTHPEndCheck()) {
            if (time >= (VERSION_NTSC ? 3300 : 2725)) {
                HuSprAttrReset(gid, 1, HUSPR_ATTR_DISPOFF);
                HuSprAttrReset(gid, 2, HUSPR_ATTR_DISPOFF);
            }
            else {
                time++;
            }
            HuPrcVSleep();
        }
        WipeColorSet(0, 0, 0);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 30);
        while (WipeStatGet()) {
            HuPrcVSleep();
        }
        HuTHPClose();
        HuPrcVSleep();
        HuSprGrpKill(gid);
    }
}

Void PlayEndingCelebration(Void)
{
    float messageSize[2];
    s32 saveChoice;
    s32 saveNoticeSpriteId;
    s32 windowId;
    s32 storyCharacter;
    saveChoice = 1;
    Hu3DAllKill();
    HuSprClose();
    HuSprInit();
    if (SLSaveFlagGet() == 0) {
        storyEndingMovieComplete = 1;
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
    WaitFrames(60);
    storyCharacter = GWStoryCharGet();
    GWSystem.storyCharBit |= 1 << storyCharacter;
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
            GWGameStat.story_continue = FALSE;
            SLCommonSet();
            SLSaveBoardStory();
            SLSave();
        }
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 20);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    storyEndingMovieComplete = 1;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

Void FinishEndingSequence(Void)
{
    OMOVLHIS *his;
    WaitFrames(120);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet());
    CharModelKill(-1);
    HuAudFadeOut(1);
    his = omOvlHisGet(0);
    omOvlHisChg(0, DLL_mstory2dll, 20, 1);
    omOvlCallEx(DLL_staffdll, 1, 0, 0);
    while (1) {
        WaitOneFrame();
    }
}

Void RunStoryEndingSequence(OMOBJ *object, StoryObjectProcessState *processState)
{
    int unusedInliningGuard = 0;
    BeginStoryEndingScene();
    ShowEndingIntroduction();
    AnimateEndingCharacters();
    ShowEndingCharacterMessage();
    FinishEndingSequence();
    processState->updateCallback = NULL;
}

Void RunEndingMainProcess(Void)
{
    OMOBJ *object;
    storyEndingMainProcess.object = object = omAddObjEx(storyEndingProcess, 256, 16, 16, -1, NULL);
    storyEndingMainProcess.updateCallback = (Void *)RunStoryEndingSequence;
    storyEndingMainProcess.object->data = &storyEndingMainProcess;
    while (1) {
        WaitOneFrame();
        if (storyEndingMainProcess.updateCallback != NULL) {
            storyEndingMainProcess.updateCallback(object, &storyEndingMainProcess);
        }
    }
}

Void CreateEndingMapObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x50));
    object->mtnId[0] = Hu3DMotionIDGet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 0, 0, 2);
    Hu3DModelShadowMapObjSet(object->mdlId[1], "base_fix9-base");
    Hu3DModelAttrSet(object->mdlId[1], 1);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x53));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x54));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x55));
    Hu3DModelAttrSet(object->mdlId[2], 1);
    ShiftObjectMotion(object, 2, 3, 0, 2);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x51));
    Hu3DModelAttrSet(object->mdlId[4], 1);
    Hu3DModelPosSet(object->mdlId[1], 0, -1000, 0);
}

Void RunEndingMapProcess(Void)
{
    OMOBJ *object;
    storyEndingMapProcess.object = object = omAddObjEx(storyEndingProcess, 256, 16, 16, -1, NULL);
    storyEndingMapProcess.updateCallback = NULL;
    CreateEndingMapObject(object);
    while (1) {
        WaitOneFrame();
        if (storyEndingMapProcess.updateCallback != NULL) {
            storyEndingMapProcess.updateCallback(object, &storyEndingMapProcess);
        }
    }
}

Void CreateEndingCharacterObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x62 : 0x66));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x63 : 0x67));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x70 : 0x71));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x71 : 0x72));
    Hu3DModelPosSet(object->mdlId[1], 0, 0, 600);
    Hu3DModelRotSet(object->mdlId[1], 0, 180, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    ShiftObjectMotion(object, 1, 3, 0, 1);
    storyEndingMotionId = object->mdlId[1];
    storyEndingCueMotionIds[0] = object->mtnId[3];
    storyEndingCueMotionIds[1] = object->mtnId[3];
    HuPrcChildCreate(WaitForEndingMotionCues, 100, 8192, 0, HuPrcCurrentGet());
}

Void AnimateEndingCharacter(OMOBJ *object, StoryObjectProcessState *processState)
{
    Hu3DData[storyEndingKoopaProcess.object->mdlId[1]].pos.z -= 2.5f;
}

Void RunEndingCharacterProcess(Void)
{
    OMOBJ *object;
    storyEndingKoopaProcess.object = object = omAddObjEx(storyEndingProcess, 256, 16, 16, -1, NULL);
    storyEndingKoopaProcess.updateCallback = NULL;
    CreateEndingCharacterObject(object);
    while (1) {
        WaitOneFrame();
        if (storyEndingKoopaProcess.updateCallback != NULL) {
            storyEndingKoopaProcess.updateCallback(object, &storyEndingKoopaProcess);
        }
        if (object->work[0]) {
            UpdateObjectMotionTransition(object);
        }
    }
}

Void CreateEndingKoopaObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x72 : 0x73));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x73 : 0x74));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x75));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x76));
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x7C : 0x7A));
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x79 : 0x77));
    Hu3DModelPosSet(object->mdlId[1], 0, 0, 460);
    Hu3DModelRotSet(object->mdlId[1], 0, 0, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelAttrSet(object->mdlId[1], 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 5, 0, 1);
    CharNpcDustSet(object->mdlId[1], object->mtnId[2], 1, 9);
}

Void AnimateEndingKoopa(OMOBJ *object, StoryObjectProcessState *processState)
{
    Vec pos = { -120, 0, 800 };
    ShiftObjectMotion(object, 1, 2, 15, 1);
    MoveModelToPosition(object, 1, pos, 90, 60, 10);
    ShiftObjectMotion(object, 1, 1, 15, 1);
    processState->updateCallback = NULL;
}

Void RunEndingKoopaProcess(Void)
{
    OMOBJ *object;
    storyEndingSubProcess.object = object = omAddObjEx(storyEndingProcess, 256, 16, 16, -1, NULL);
    storyEndingSubProcess.updateCallback = NULL;
    CreateEndingKoopaObject(object);
    while (1) {
        WaitOneFrame();
        if (storyEndingSubProcess.updateCallback != NULL) {
            storyEndingSubProcess.updateCallback(object, &storyEndingSubProcess);
        }
        if (object->work[0]) {
            UpdateObjectMotionTransition(object);
        }
    }
}

Void PrepareEndingPlayers(int selectedPlayerIndex)
{
    int i;
    int playerOrder[4];
    playerOrder[0] = 0;
    for (i = 1; i < 4; i++) {
        playerOrder[i] = playerOrder[i - 1] + 1;
        if (playerOrder[i] >= 4) {
            playerOrder[i] = 0;
        }
        if (selectedPlayerIndex == playerOrder[i]) {
            storyEndingState.selectedPlayerIndex = i;
        }
    }
    for (i = 0; i < 4; i++) {
        ReadStoryPlayerConfig(&storyEndingState.playerInfo[i], playerOrder[i]);
    }
}

Void CreateEndingPlayerModel(OMOBJ *object)
{
    int characterId = storyEndingState.playerInfo[storyEndingState.selectedPlayerIndex].characterId;
    object->mdlId[1] = CharModelCreate(characterId, 1);
    object->mtnId[1] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00));
    object->mtnId[2] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x35));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x10) + characterId);
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x08) + characterId);
    CharMotionDataClose(characterId);
    Hu3DModelPosSet(object->mdlId[1], 120, 0, 800);
    Hu3DModelRotSet(object->mdlId[1], 0, -135, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelAttrSet(object->mdlId[1], 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 1, 0, 1);
}

Void AnimateEndingPlayer(OMOBJ *object, StoryObjectProcessState *processState)
{
    int i;
    ShiftObjectMotion(object, 1, 2, 0, 0);
    WaitOneFrame();
    Hu3DMotionTimeSet(object->mdlId[1], 60);
    for (i = 0; i < 60; i++) {
        WaitOneFrame();
        Hu3DData[object->mdlId[1]].rot.y = InterpolateLinear(-135, -90, i, 60);
    }
    ShiftObjectMotion(object, 1, 1, 15, 1);
    processState->updateCallback = NULL;
}

Void RunEndingPlayerProcess(Void)
{
    OMOBJ *object;
    storyEndingPlayerProcess.object = object = omAddObjEx(storyEndingProcess, 256, 16, 16, -1, NULL);
    storyEndingPlayerProcess.updateCallback = NULL;
    CreateEndingPlayerModel(object);
    while (1) {
        WaitOneFrame();
        if (storyEndingPlayerProcess.updateCallback != NULL) {
            storyEndingPlayerProcess.updateCallback(object, &storyEndingPlayerProcess);
        }
        if (object->work[0]) {
            UpdateObjectMotionTransition(object);
        }
    }
}

Void HandleEndingProcessEnd(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    work->cameraCenter.x = 0;
    work->cameraCenter.y = 120;
    work->cameraCenter.z = 0;
    work->cameraRotation.x = 0;
    work->cameraRotation.y = 0;
    work->cameraRotation.z = 0;
    work->cameraZoom = 1400;
}

Void UpdateEndingCamera(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    work->cameraCenter.x = 0;
    work->cameraCenter.y = 125;
    work->cameraCenter.z = 0;
    work->cameraRotation.x = -4.9f;
    work->cameraRotation.y = 0;
    work->cameraRotation.z = 0;
    work->cameraZoom = 1750;
}

Void AnimateEndingCamera(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    StoryCameraMotionState targetCameraState;
    targetCameraState.cameraCenter.x = 0;
    targetCameraState.cameraCenter.y = 100;
    targetCameraState.cameraCenter.z = 0;
    targetCameraState.cameraRotation.x = 0;
    targetCameraState.cameraRotation.y = 0;
    targetCameraState.cameraRotation.z = 0;
    targetCameraState.cameraZoom = 1400;
    EaseCameraMotionLinear(work, &targetCameraState, work->animationFrame++, 60, 10);
    if (0.0f != storyEndingState.verticalOffset) {
        work->cameraCenter.y = 125 + storyEndingState.verticalOffset;
    }
}
