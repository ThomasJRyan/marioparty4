#include "REL/mstory2Dll.h"
#include "game/armem.h"
#include "game/board/main.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/msm.h"
#include "game/saveload.h"
#endif


typedef struct StructData850 {
    s32 dataNum;
    s32 resourceMode;
    s32 resourceState;
    s32 resourceFlags;
    s32 highlightName;
} StoryCharacterResource;

StoryCharacterResource storyCharacterResources[] = {
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x00), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x01), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x02), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x03), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x04), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x05), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x06), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x07), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x08), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x09), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x0A), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x0B), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x0C), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x0D), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x0E), 1, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x0F), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x10), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x11), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x12), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x13), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x14), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x15), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x16), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x17), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x18), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x19), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x1A), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x1B), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x1C), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x1D), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x1E), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x1F), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x20), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x21), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x22), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x23), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x24), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x25), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x26), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x27), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x28), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x29), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2A), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2B), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2C), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2D), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2E), 0, 0, 0, 1 },
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2F), 0, 0, 0, 1 },
};

s32 minigameClearMotionCueFrames[2][2] = { 20, 70, 1, 1 };
s32 minigameClearMotionId = -1;
s32 minigameClearCueMotionIds[2] = { -1, -1 };

typedef struct {
    /* 0x000 */ s32 selectedPlayerIndex;
    /* 0x004 */ s32 activeWindowId;
    /* 0x008 */ s32 eventWindowId;
    /* 0x00C */ s32 activePadIndex;
    /* 0x010 */ s32 playerOrderCount;
    /* 0x014 */ s32 reserved14;
    /* 0x018 */ char reserved18[0x18];
    /* 0x030 */ StoryPlayerConfig playerInfo[4];
    /* 0x100 */ s32 verticalOffset;
} MinigameClearState; // Size 0x104

MinigameClearState minigameClearState;
StoryObjectProcessState minigameClearMainProcess;
StoryObjectProcessState minigameClearRewardProcess;
StoryObjectProcessState minigameClearMapProcess;
StoryObjectProcessState minigameClearKoopaProcess;
StoryObjectProcessState minigameClearCharacterProcess;
BOOL minigameClearResourcesReady;
HUPROCESS *minigameClearProcess;

Void RunMinigameClearPlayerProcess(Void);
Void AnimateMinigameClearReward(OMOBJ *object, StoryObjectProcessState *processState);
Void MoveMinigameClearReward(OMOBJ *object, StoryObjectProcessState *processState);
Void RunMinigameClearItemProcess(Void);
Void SetMinigameClearMapMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags);
Void AnimateMinigameClearMapObject(OMOBJ *object, StoryObjectProcessState *processState);
Void RunMinigameClearMapProcess(Void);
Void SetMinigameClearKoopaMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags);
Void AnimateMinigameClearKoopa(OMOBJ *object, StoryObjectProcessState *processState);
Void AnimateMinigameClearKoopaExit(OMOBJ *object, StoryObjectProcessState *processState);
Void RunMinigameClearKoopaProcess(Void);
Void PrepareMinigameClearPlayers(int selectedPlayerIndex);
Void RunMinigameClearPlayerModelProcess(Void);
Void UpdateMinigameClearCamera(Void);
Void InterpolateMinigameClearCamera(Void);
Void AnimateMinigameClearCamera(Void);
Void FinishMinigameClearCamera(Void);

Void WaitForMinigameClearMotionCues(Void)
{
    int i;
    while (1) {
        do {
            HuPrcVSleep();
        } while (minigameClearMotionId == -1 || minigameClearCueMotionIds[0] == -1 || minigameClearCueMotionIds[1] == -1 || minigameClearCueMotionIds[0] != Hu3DMotionIDGet(minigameClearMotionId));
        for (i = 0; i < 2; i++) {
            if (minigameClearMotionCueFrames[1][i] != -1) {
                if (Hu3DMotionTimeGet(minigameClearMotionId) >= minigameClearMotionCueFrames[0][i]) {
                    OSReport("KE-System  TIME:%d\n", minigameClearMotionCueFrames[0][i]);
                    HuAudFXPlay(81);
                    minigameClearMotionCueFrames[1][i] = -1;
                }
            }
        }
        if (Hu3DMotionTimeGet(minigameClearMotionId) <= 10) {
            for (i = 0; i < 2; i++) {
                minigameClearMotionCueFrames[1][i] = 1;
            }
        }
    }
}

Void StartMinigameClearEvent(HUPROCESS *parentProcess, s32 activePadIndex)
{
    minigameClearProcess = parentProcess;
    minigameClearState.activePadIndex = activePadIndex;
    InitializeWindowSystem();
    minigameClearState.verticalOffset = CreateStoryDirectionalLight(255);
    InitializeStoryCamera(minigameClearProcess, UpdateMinigameClearCamera);
    CreateStoryShadow();
    PrepareMinigameClearPlayers(activePadIndex);
    HuPrcChildCreate(RunMinigameClearItemProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunMinigameClearMapProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunMinigameClearKoopaProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunMinigameClearPlayerModelProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunMinigameClearPlayerProcess, 100, 8192, 0, HuPrcCurrentGet());
}

Void UpdateMinigameClearObjectMotion(OMOBJ *object)
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

Void DisplayMinigameClearDebugStatus(Void)
{
    int posX = 16;
    int posY = 40;
    GXColor winColor = { 0, 0, 128, 128 };
    int i;
    while (1) {
        WaitOneFrame();
        printWin(posX, posY, 220, 130, &winColor);
        if (minigameClearMainProcess.updateCallback == NULL) {
            print8(posX, posY, 1, "PROC_MAIN   -> FALSE");
        }
        else {
            print8(posX, posY, 1, "PROC_MAIN   -> TRUE");
        }
        if (minigameClearRewardProcess.updateCallback == NULL) {
            print8(posX, posY + 10, 1, "PROC_MAP    -> FALSE");
        }
        else {
            print8(posX, posY + 10, 1, "PROC_MAP    -> TRUE");
        }
        if (minigameClearMapProcess.updateCallback == NULL) {
            print8(posX, posY + 20, 1, "PROC_KOOPA  -> FALSE");
        }
        else {
            print8(posX, posY + 20, 1, "PROC_KOOPA  -> TRUE");
        }
        if (minigameClearKoopaProcess.updateCallback == NULL) {
            print8(posX, posY + 30, 1, "PROC_MKOOPA -> FALSE");
        }
        else {
            print8(posX, posY + 30, 1, "PROC_MKOOPA -> TRUE");
        }
        if (minigameClearCharacterProcess.updateCallback == NULL) {
            print8(posX, posY + 40, 1, "PROC_PLAYER -> FALSE");
        }
        else {
            print8(posX, posY + 40, 1, "PROC_PLAYER -> TRUE");
        }
        print8(posX, posY + 60, 1, "P:%d W:%d", minigameClearState.selectedPlayerIndex, minigameClearState.activePadIndex);
        print8(posX, posY + 80, 1, "NO) IDX:GRP:COM:DIF:PAD:CHR");
        for (i = 0; i < 4; i++) {
            print8(posX, posY + 90 + (i * 10), 1, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", i, minigameClearState.playerInfo[i].playerIndex, minigameClearState.playerInfo[i].group,
                minigameClearState.playerInfo[i].isComputer, minigameClearState.playerInfo[i].difficulty, minigameClearState.playerInfo[i].padIndex, minigameClearState.playerInfo[i].highlightName);
        }
    }
}

Void BeginMinigameClearScene(Void)
{
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet());
    WaitFrames(30);
}

Void WaitForMinigameClearAnimations(Void)
{
    minigameClearState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, minigameClearState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[6][1]);
    SetMinigameClearMapMotion(4, 1, 15, 0, 1);
    InsertWindowMessage(minigameClearState.activeWindowId, minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName, 0);
    WaitForWindowMessageAndMotionSimple(minigameClearState.activeWindowId, 0x1F0014, 0, minigameClearMapProcess.object->mdlId[1]);
    HuAudFXPlay(storyAudioCueIds[6][2]);
    ShiftObjectMotion(minigameClearMapProcess.object, 1, 5, 10, 1);
    SetWindowMessage(minigameClearState.activeWindowId, 0x1F0015);
    WaitForWindowMessage(minigameClearState.activeWindowId, -1);
    SetWindowMessage(minigameClearState.activeWindowId, 0x250004);
    SetMinigameClearKoopaMotion(1, -1, 15, 1, 0);
    CloseCenteredMessageWindow(minigameClearState.activeWindowId);
    minigameClearMapProcess.updateCallback = (Void *)AnimateMinigameClearMapObject;
    WaitFrames(60);
    ShiftObjectMotion(minigameClearCharacterProcess.object, 1, 4, 15, 0);
    WaitFrames(16);
    while (1) {
        WaitOneFrame();
        if (Hu3DMotionTimeGet(minigameClearCharacterProcess.object->mdlId[1]) >= 20) {
            break;
        }
    }
    Hu3DMotionSpeedSet(minigameClearCharacterProcess.object->mdlId[1], 0);
    while (1) {
        WaitOneFrame();
        if (minigameClearMapProcess.updateCallback == NULL) {
            break;
        }
    }
    WaitFrames(30);
    CaptureCameraMotionStart(&storyCameraState);
    SetCameraUpdateCallback(InterpolateMinigameClearCamera);
    minigameClearKoopaProcess.updateCallback = (Void *)AnimateMinigameClearKoopaExit;
    WaitFrames(80);
    Hu3DMotionSpeedSet(minigameClearCharacterProcess.object->mdlId[1], 1);
    ShiftObjectMotion(minigameClearCharacterProcess.object, 1, 1, 15, 1);
    while (1) {
        WaitOneFrame();
        if (minigameClearKoopaProcess.updateCallback == NULL) {
            break;
        }
    }
}

Void RunMinigameClearSequence(Void)
{
    minigameClearState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, minigameClearState.activePadIndex);
    storyAudioSequenceIds[0] = HuAudSeqPlay(56);
    HuAudFXPlay(storyAudioCueIds[5][0]);
    ShiftObjectMotion(minigameClearKoopaProcess.object, 1, 4, 15, 0);
    SetWindowMessage(minigameClearState.activeWindowId, 0x1F0016);
    WaitForWindowMessage(minigameClearState.activeWindowId, -1);
    SetWindowMessage(minigameClearState.activeWindowId, 0x250004);
    HuAudFXPlay(storyAudioCueIds[5][0]);
    ShiftObjectMotion(minigameClearKoopaProcess.object, 1, 3, 15, 1);
    SetWindowMessage(minigameClearState.activeWindowId, 0x1F0017);
    WaitForWindowMessage(minigameClearState.activeWindowId, -1);
    SetWindowMessage(minigameClearState.activeWindowId, 0x250004);
    HuAudFXPlay(storyAudioCueIds[5][0]);
    SetWindowMessage(minigameClearState.activeWindowId, 0x1F0018);
    WaitForWindowMessage(minigameClearState.activeWindowId, -1);
    SetWindowMessage(minigameClearState.activeWindowId, 0x250004);
    ShiftObjectMotion(minigameClearKoopaProcess.object, 1, 1, 15, 1);
    CloseCenteredMessageWindow(minigameClearState.activeWindowId);
    WaitFrames(60);
    minigameClearState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, minigameClearState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[5][0]);
    ShiftObjectMotion(minigameClearKoopaProcess.object, 1, 3, 15, 1);
    InsertWindowMessage(minigameClearState.activeWindowId, minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName, 0);
    SetWindowMessage(minigameClearState.activeWindowId, 0x1F0019);
    WaitForWindowMessage(minigameClearState.activeWindowId, -1);
    SetWindowMessage(minigameClearState.activeWindowId, 0x250004);
    HuAudFXPlay(storyAudioCueIds[5][1]);
    SetWindowMessage(minigameClearState.activeWindowId, 0x1F001A);
    WaitForWindowMessage(minigameClearState.activeWindowId, -1);
    SetWindowMessage(minigameClearState.activeWindowId, 0x250004);
    minigameClearKoopaProcess.updateCallback = (Void *)AnimateMinigameClearKoopa;
    SetWindowMessage(minigameClearState.activeWindowId, 0x1F001B);
    WaitForWindowMessage(minigameClearState.activeWindowId, -1);
    SetWindowMessage(minigameClearState.activeWindowId, 0x250004);
    CloseCenteredMessageWindow(minigameClearState.activeWindowId);
    while (1) {
        WaitOneFrame();
        if (minigameClearKoopaProcess.updateCallback == NULL) {
            break;
        }
    }
}

Void AnimateMinigameClearPlayer(OMOBJ *object, StoryObjectProcessState *processState)
{
    int streamNo = HuAudSStreamPlay(7);
    while (1) {
        WaitOneFrame();
        if (HuAudSStreamStatGet(streamNo) == 0) {
            break;
        }
    }
    HuAudSeqPause(storyAudioSequenceIds[0], 0, 1000);
    processState->updateCallback = NULL;
}

Void AnimateMinigameClearItem(OMOBJ *object, StoryObjectProcessState *processState)
{
    int presentSe = (minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName * 6) + 5;
    WaitFrames(150);
    OSReport("########## PRESENT_SE : %d\n", presentSe);
    HuAudFXPlay(presentSe + 2260);
    ShiftObjectMotion(minigameClearRewardProcess.object, 7, 7, 1, 0);
    processState->updateCallback = NULL;
}

Void ShowMinigameClearReward(Void)
{
    int i;
    ShiftObjectMotion(minigameClearCharacterProcess.object, 1, 2, 15, 1);
    for (i = 0; i < 30; i++) {
        WaitOneFrame();
        if (minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName != 5) {
            Hu3DData[minigameClearCharacterProcess.object->mdlId[1]].pos.x = InterpolateLinear(120, 80, i, 30);
        }
        Hu3DData[minigameClearCharacterProcess.object->mdlId[1]].rot.y = InterpolateLinear(-60, -90, i, 30);
    }
    ShiftObjectMotion(minigameClearCharacterProcess.object, 1, 5, 5, 0);
    WaitFrames(10);
    while (1) {
        WaitOneFrame();
        if (Hu3DMotionTimeGet(minigameClearCharacterProcess.object->mdlId[1]) >= 30) {
            break;
        }
    }
    Hu3DModelPosSet(minigameClearRewardProcess.object->mdlId[5], 0, 0, 0);
    Hu3DModelHookSet(
        minigameClearCharacterProcess.object->mdlId[1], CharModelItemHookGet(minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName, 1, 1), minigameClearRewardProcess.object->mdlId[5]);
    WaitFrames(60);
    minigameClearRewardProcess.updateCallback = (Void *)AnimateMinigameClearReward;
    HuAudFXPlay(163);
    CharFXPlay(minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName, 296);
    ShiftObjectMotion(minigameClearRewardProcess.object, 6, 6, 0, 0);
    Hu3DModelAttrReset(minigameClearRewardProcess.object->mdlId[6], 1);
    Hu3DModelAttrSet(minigameClearRewardProcess.object->mdlId[5], 1);
    ShiftObjectMotion(minigameClearCharacterProcess.object, 1, 6, 5, 0);
    HuAudSeqPause(storyAudioSequenceIds[0], 1, 1000);
    WipeColorSet(255, 255, 255);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    while (1) {
        WaitOneFrame();
        if (!WipeStatGet()) {
            break;
        }
    }
    Hu3DMotionSpeedSet(minigameClearCharacterProcess.object->mdlId[1], 0);
    Hu3DMotionTimeSet(minigameClearCharacterProcess.object->mdlId[1], 50);
    Hu3DModelAttrReset(minigameClearRewardProcess.object->mdlId[7], 1);
    Hu3DModelAttrReset(minigameClearRewardProcess.object->mdlId[8], 1);
    ShiftObjectMotion(minigameClearRewardProcess.object, 8, 8, 0, 1);
    WaitFrames(30);
    HuAudFXPlay(169);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    while (1) {
        WaitOneFrame();
        if (!WipeStatGet()) {
            break;
        }
    }
    CaptureCameraMotionStart(&storyCameraState);
    SetCameraUpdateCallback(FinishMinigameClearCamera);
    WaitFrames(60);
    minigameClearCharacterProcess.updateCallback = (Void *)AnimateMinigameClearItem;
    minigameClearState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, minigameClearState.activePadIndex);
    minigameClearKoopaProcess.updateCallback = (Void *)AnimateMinigameClearPlayer;
    InsertWindowMessage(minigameClearState.activeWindowId, (minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName * 6) + 0x32000B, 0);
    WaitForWindowMessageDuration(minigameClearState.activeWindowId, 0x320086, 0, 300);
    SetWindowMessage(minigameClearState.activeWindowId, 0x250004);
    CloseCenteredMessageWindow(minigameClearState.activeWindowId);
    CaptureCameraMotionStart(&storyCameraState);
    SetCameraUpdateCallback(AnimateMinigameClearCamera);
    minigameClearRewardProcess.updateCallback = (Void *)MoveMinigameClearReward;
    WaitFrames(30);
    Hu3DMotionSpeedSet(minigameClearCharacterProcess.object->mdlId[1], 0.1f);
    WaitFrames(180);
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
}

Void PrepareMinigameClearTransition(Void)
{
    Hu3DAllKill();
    HuSprClose();
    HuSprInit();
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 10);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    WaitFrames(60);
    SLCommonSet();
    SLSaveBoardStory();
    if (SLSaveFlagGet() == 1) {
        SLSave();
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 10);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    minigameClearResourcesReady = TRUE;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

Void FinishMinigameClearEvent(Void)
{
    OMOVLHIS *overlayHistory;

    msmSeStopAll(FALSE, 1000);
    do {
        WaitOneFrame();
    } while (WipeStatGet() != 0);
    CharModelKill(-1);
    HuAudFXAllStop();
    HuAudSStreamAllStop();
    GWMGAvailSet(omMgIndexGet(58) + 401);
    GWGameStat.present[minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName * 6 + 5] = 1;
    OSReport("########## PRESENTFLG CHR:%d, MAP:5, PRESENT:%d\n", minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName,
        minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName * 6 + 5);
    overlayHistory = omOvlHisGet(0);
    omOvlHisChg(0, DLL_mstory2dll, 20, overlayHistory->stat);
    omOvlCallEx(DLL_present, 1, minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName * 6 + 5 + 1, 0);
}

Void UpdateMinigameClearPlayer(OMOBJ *object, StoryObjectProcessState *processState)
{
    int unusedInliningGuard = 0;
    BeginMinigameClearScene();
    WaitForMinigameClearAnimations();
    RunMinigameClearSequence();
    ShowMinigameClearReward();
    FinishMinigameClearEvent();
    processState->updateCallback = 0;
}

Void RunMinigameClearPlayerProcess(Void)
{
    OMOBJ *object;
    minigameClearMainProcess.object = object = omAddObjEx(minigameClearProcess, 256, 16, 16, -1, NULL);
    minigameClearMainProcess.updateCallback = (Void *)UpdateMinigameClearPlayer;
    minigameClearMainProcess.object->data = &minigameClearMainProcess;
    while (1) {
        WaitOneFrame();
        if (minigameClearMainProcess.updateCallback != NULL) {
            minigameClearMainProcess.updateCallback(object, &minigameClearMainProcess);
        }
    }
}

Void CreateMinigameClearRewardModel(OMOBJ *object)
{
    s32 characterId;
    characterId = minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName;
    object->mdlId[7] = Hu3DModelCreateFile(storyCharacterResources[characterId * 6 + 5].dataNum);
    object->mtnId[7] = Hu3DMotionIDGet(object->mdlId[7]);
    Hu3DModelPosSet(object->mdlId[7], 0, 180, 825);
    Hu3DModelAttrSet(object->mdlId[7], 1);
    Hu3DModelShadowSet(object->mdlId[7]);
    ShiftObjectMotion(object, 7, 7, 0, 2);
    HuDataDirClose(DATADIR_MSTORY4);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x52));
    object->mtnId[1] = Hu3DMotionIDGet(object->mdlId[1]);
    Hu3DModelShadowMapObjSet(object->mdlId[1], "base_story-base");
    ShiftObjectMotion(object, 1, 1, 0, 1);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x53));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x54));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x55));
    ShiftObjectMotion(object, 2, 3, 0, 2);
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x5B));
    Hu3DModelPosSet(object->mdlId[5], 0, 125, 800);
    Hu3DModelAttrSet(object->mdlId[5], 1);
    Hu3DModelShadowSet(object->mdlId[5]);
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x5D));
    object->mtnId[6] = Hu3DMotionIDGet(object->mdlId[6]);
    Hu3DModelPosSet(object->mdlId[6], 25, 125, 900);
    Hu3DModelAttrSet(object->mdlId[6], 1);
    ShiftObjectMotion(object, 6, 6, 0, 2);
    object->mdlId[8] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x5F));
    object->mtnId[8] = Hu3DMotionIDGet(object->mdlId[8]);
    Hu3DModelAttrSet(object->mdlId[8], 1);
    ShiftObjectMotion(object, 8, 8, 0, 1);
}

Void AnimateMinigameClearReward(OMOBJ *object, StoryObjectProcessState *processState)
{
    Mtx matrix;
    Hu3DData[object->mdlId[7]].pos.y = InterpolateSineCycle(180, 185, processState->animationFrame++, 360);
    Hu3DModelObjMtxGet(object->mdlId[7], "effect", matrix);
    Hu3DModelPosSet(object->mdlId[8], matrix[0][3], matrix[1][3], matrix[2][3]);
    if (processState->animationFrame >= 360) {
        processState->animationFrame = 0;
    }
}

Void MoveMinigameClearReward(OMOBJ *object, StoryObjectProcessState *processState)
{
    Mtx matrix;
    Hu3DData[object->mdlId[7]].pos.y += processState->auxiliaryValue / 100.0f;
    Hu3DModelObjMtxGet(object->mdlId[7], "effect", matrix);
    Hu3DModelPosSet(object->mdlId[8], matrix[0][3], matrix[1][3], matrix[2][3]);
    processState->auxiliaryValue += 2;
}

Void AnimateMinigameClearItemOrbit(OMOBJ *object, StoryObjectProcessState *processState)
{
    int i;
    Vec pos;
    float rotY;
    Hu3DModelPosSet(object->mdlId[5], -180, 100, 800);
    Hu3DModelRotSet(object->mdlId[5], 0, 0, 0);
    Hu3DModelAttrReset(object->mdlId[5], 1);
    for (i = 0; i < 60; i++) {
        WaitOneFrame();
        if (i == 30) {
            HuAudFXPlay(151);
        }
        if (i <= 30) {
            pos.y = InterpolateSinePulse(0, 100, i + 30, 60);
            pos.x = InterpolateLinear(-180, -45, i, 30);
            rotY = InterpolateSineEaseIn(0, -90, i, 30);
        }
        else {
            pos.x = InterpolateLinear(-45, 0, i - 30, 30);
            pos.y = InterpolateSinePulse(0, 25, i - 30, 30);
            rotY = InterpolateSineEaseIn(-90, -360, i - 30, 30);
        }
        Hu3DModelPosSet(object->mdlId[5], pos.x, pos.y, 800);
        Hu3DModelRotSet(object->mdlId[5], 0, rotY, 0);
    }
    processState->updateCallback = NULL;
}

Void RunMinigameClearItemProcess(Void)
{
    OMOBJ *object;
    minigameClearRewardProcess.object = object = omAddObjEx(minigameClearProcess, 256, 16, 16, -1, NULL);
    minigameClearRewardProcess.updateCallback = NULL;
    minigameClearRewardProcess.object->data = &minigameClearRewardProcess;
    CreateMinigameClearRewardModel(object);
    while (1) {
        WaitOneFrame();
        if (minigameClearRewardProcess.updateCallback != NULL) {
            minigameClearRewardProcess.updateCallback(object, &minigameClearRewardProcess);
        }
    }
}

Void CreateMinigameClearMapObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x62 : 0x66));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x63 : 0x67));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x64 : 0x68));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x6E : 0x6F));
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x6F : 0x70));
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x69 : 0x6C));
    Hu3DModelPosSet(object->mdlId[1], -180, 0, 800);
    Hu3DModelRotSet(object->mdlId[1], 0, 60, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 3, 0, 1);
    minigameClearMotionId = object->mdlId[1];
    minigameClearCueMotionIds[0] = object->mtnId[2];
    minigameClearCueMotionIds[1] = object->mtnId[2];
    HuPrcChildCreate(WaitForMinigameClearMotionCues, 100, 8192, 0, HuPrcCurrentGet());
}

Void SetMinigameClearMapMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(minigameClearMapProcess.object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    minigameClearMapProcess.object->work[0] = motionFlags;
    minigameClearMapProcess.object->work[1] = motionSlot;
    minigameClearMapProcess.object->work[2] = blendFrames + 1;
}

Void AnimateMinigameClearMapObject(OMOBJ *object, StoryObjectProcessState *processState)
{
    Vec pos = { -1000, 0, 810 };
    minigameClearRewardProcess.updateCallback = (Void *)AnimateMinigameClearItemOrbit;
    ShiftObjectMotion(object, 1, 2, 15, 1);
    MoveModelToPosition(object, 1, pos, -1, 45, 10);
    ShiftObjectMotion(object, 1, 1, 15, 1);
    processState->updateCallback = NULL;
}

Void RunMinigameClearMapProcess(Void)
{
    OMOBJ *object;
    minigameClearMapProcess.object = object = omAddObjEx(minigameClearProcess, 256, 16, 16, -1, NULL);
    minigameClearMapProcess.updateCallback = NULL;
    minigameClearMapProcess.object->data = &minigameClearRewardProcess;
    CreateMinigameClearMapObject(object);
    while (1) {
        WaitOneFrame();
        if (minigameClearMapProcess.updateCallback != NULL) {
            minigameClearMapProcess.updateCallback(object, &minigameClearMapProcess);
        }
        if (object->work[0]) {
            if (object->work[2]) {
                object->work[2]--;
            }
            else if (object->work[2] == 0 && Hu3DMotionEndCheck(object->mdlId[1])) {
                ShiftObjectMotion(object, 1, object->work[1], 15, 1);
                object->work[0] = object->work[1] = object->work[2] = 0;
            }
        }
    }
}

Void CreateMinigameClearKoopaObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x7D : 0x7B));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x7E : 0x7C));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x80 : 0x7D));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x81 : 0x7E));
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x86 : 0x81));
    Hu3DModelPosSet(object->mdlId[1], -600, 0, 800);
    Hu3DModelRotSet(object->mdlId[1], 0, 60, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 1, 0, 1);
    CharNpcDustSet(object->mdlId[1], object->mtnId[2], 1, 13);
}

Void SetMinigameClearKoopaMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(minigameClearKoopaProcess.object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    minigameClearKoopaProcess.object->work[0] = motionFlags;
    minigameClearKoopaProcess.object->work[1] = motionSlot;
    minigameClearKoopaProcess.object->work[2] = blendFrames + 1;
}

Void AnimateMinigameClearKoopa(OMOBJ *object, StoryObjectProcessState *processState)
{
    Vec pos = { -800, 0, 800 };
    ShiftObjectMotion(object, 1, 2, 15, 1);
    MoveModelToPosition(object, 1, pos, 60, 60, 10);
    ShiftObjectMotion(object, 1, 1, 15, 1);
    processState->updateCallback = NULL;
}

Void AnimateMinigameClearKoopaExit(OMOBJ *object, StoryObjectProcessState *processState)
{
    Vec pos = { -120, 0, 800 };
    ShiftObjectMotion(object, 1, 2, 15, 1);
    MoveModelToPosition(object, 1, pos, 60, 60, 10);
    ShiftObjectMotion(object, 1, 1, 15, 1);
    processState->updateCallback = NULL;
}

Void RunMinigameClearKoopaProcess(Void)
{
    OMOBJ *object;
    minigameClearKoopaProcess.object = object = omAddObjEx(minigameClearProcess, 256, 16, 16, -1, NULL);
    minigameClearKoopaProcess.updateCallback = NULL;
    minigameClearKoopaProcess.object->data = &minigameClearKoopaProcess;
    CreateMinigameClearKoopaObject(object);
    while (1) {
        WaitOneFrame();
        if (minigameClearKoopaProcess.updateCallback != NULL) {
            minigameClearKoopaProcess.updateCallback(object, &minigameClearKoopaProcess);
        }
        if (object->work[0]) {
            if (object->work[2]) {
                object->work[2]--;
            }
            else if (object->work[2] == 0 && Hu3DMotionEndCheck(object->mdlId[1])) {
                ShiftObjectMotion(object, 1, object->work[1], 15, 1);
                object->work[0] = object->work[1] = object->work[2] = 0;
            }
        }
    }
}

Void PrepareMinigameClearPlayers(int selectedPlayerIndex)
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
            minigameClearState.selectedPlayerIndex = i;
        }
    }
    for (i = 0; i < 4; i++) {
        ReadStoryPlayerConfig(&minigameClearState.playerInfo[i], playerOrder[i]);
    }
}

Void CreateMinigameClearPlayerModel(OMOBJ *object)
{
    int characterId = minigameClearState.playerInfo[minigameClearState.selectedPlayerIndex].highlightName;
    object->mdlId[1] = CharModelCreate(characterId, 1);
    object->mtnId[1] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00));
    object->mtnId[2] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x28) + characterId);
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x30) + characterId);
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x38) + characterId);
    object->mtnId[6] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x00) + characterId);
    CharMotionDataClose(characterId);
    Hu3DModelPosSet(object->mdlId[1], 120, 0, 800);
    Hu3DModelRotSet(object->mdlId[1], 0, -60, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 3, 0, 1);
}

Void SetMinigameClearPlayerMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(minigameClearCharacterProcess.object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    minigameClearCharacterProcess.object->work[0] = motionFlags;
    minigameClearCharacterProcess.object->work[1] = motionSlot;
    minigameClearCharacterProcess.object->work[2] = blendFrames + 1;
}

Void RunMinigameClearPlayerModelProcess(Void)
{
    OMOBJ *object;
    minigameClearCharacterProcess.object = object = omAddObjEx(minigameClearProcess, 256, 16, 16, -1, NULL);
    minigameClearCharacterProcess.updateCallback = NULL;
    minigameClearCharacterProcess.object->data = &minigameClearCharacterProcess;
    CreateMinigameClearPlayerModel(object);
    while (1) {
        WaitOneFrame();
        if (minigameClearCharacterProcess.updateCallback != NULL) {
            minigameClearCharacterProcess.updateCallback(object, &minigameClearCharacterProcess);
        }
        if (object->work[0]) {
            if (object->work[2]) {
                object->work[2]--;
            }
            else if (object->work[2] == 0 && Hu3DMotionEndCheck(object->mdlId[1])) {
                ShiftObjectMotion(object, 1, object->work[1], 15, 1);
                object->work[0] = object->work[1] = object->work[2] = 0;
            }
        }
    }
}

Void UpdateMinigameClearCamera(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    work->cameraCenter.x = 0;
    work->cameraCenter.y = 125;
    work->cameraCenter.z = 0;
    work->cameraRotation.x = -5;
    work->cameraRotation.y = 0;
    work->cameraRotation.z = 0;
    work->cameraZoom = 1750;
}

Void InterpolateMinigameClearCamera(Void)
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
    EaseCameraMotionSineIn(work, &targetCameraState, work->animationFrame++, 120, 10);
}

Void AnimateMinigameClearCamera(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    work->startCenterX = 0;
    work->startCenterY = Hu3DData[minigameClearRewardProcess.object->mdlId[7]].pos.y;
    work->startCenterZ = 0;
    work->startRotationX = 0;
    work->startRotationY = 0;
    work->startRotationZ = 0;
    work->startZoom = InterpolateLinear(1200, 1400, work->animationFrame++, 240);
    EaseCameraMotionInPlace(work, 30);
}

Void FinishMinigameClearCamera(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    StoryCameraMotionState targetCameraState;
    targetCameraState.cameraCenter.x = 0;
    targetCameraState.cameraCenter.y = 160;
    targetCameraState.cameraCenter.z = 0;
    targetCameraState.cameraRotation.x = 0;
    targetCameraState.cameraRotation.y = 0;
    targetCameraState.cameraRotation.z = 0;
    targetCameraState.cameraZoom = 1200;
    EaseCameraMotionCosine(work, &targetCameraState, work->animationFrame++, 10, 5);
}
