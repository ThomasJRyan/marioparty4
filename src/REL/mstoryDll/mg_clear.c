#include "REL/mstoryDll.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/flag.h"
#include "game/gamework.h"
#include "game/hu3d.h"
#include "game/objsub.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/msm.h"
#endif

#include "game/printfunc.h"
#include "game/saveload.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

typedef struct {
    /* 0x000 */ s32 boardId;
    /* 0x004 */ s32 currentPlayerIndex;
    /* 0x008 */ s32 messageWindowId;
    /* 0x00C */ char unreferencedResultPadding[4];
    /* 0x010 */ s32 winningPlayerIndex;
    /* 0x014 */ char unreferencedRewardStatePadding[0x20];
    /* 0x034 */ StoryPlayerResult playerResults[4];
    /* 0x104 */ s32 unreferencedSequenceState;
} MinigameClearResultState; // Size 0x108

typedef struct {
    /* 0x00 */ s32 modelDataId;
    /* 0x04 */ s32 hasRewardMotion;
    /* 0x08 */ s32 unreferencedRewardMetadata;
    /* 0x0C */ s32 unreferencedRewardMetadataFlag;
    /* 0x10 */ s32 isAvailable;
} MinigameClearRewardData; // Size 0x14

Void CreateMinigameClearController(Void);
Void AnimateRewardFloat(OMOBJ* object, StoryProcessTaskState* taskState);
Void AnimateRewardRise(OMOBJ* object, StoryProcessTaskState* taskState);
Void MoveRewardCarrierForward(OMOBJ* object, StoryProcessTaskState* taskState);
Void CreateMinigameClearStage(Void);
Void CreateMinigameClearCharacter(Void);
Void CaptureMinigameClearPlayerResults(s32 playerIndex);
Void CreateMinigameClearWinnerObject(Void);
Void SetMinigameClearCameraStart(Void);
Void MoveMinigameClearCameraToTarget(Void);
Void TrackRewardWithCamera(Void);
Void TransitionMinigameClearCamera(Void);

MinigameClearResultState resultState;
StoryProcessTaskState minigameClearControllerState;
StoryProcessTaskState minigameClearStageState;
StoryProcessTaskState minigameClearAudioState;
StoryProcessTaskState minigameClearWinnerState;
ANIMDATA* minigameClearRewardAnimations[2];
s32 minigameClearSaveComplete;
HUPROCESS* minigameClearObjectManager;

MinigameClearRewardData minigameRewardData[] = {
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
    { DATA_MAKE_NUM(DATADIR_MSTORY4, 0x2F), 0, 0, 0, 1 }
};

s32 minigameClearMessageIds[][4] = {
    { MAKE_MESSID(30, 72), MAKE_MESSID(30, 73) },
    { MAKE_MESSID(30, 74), MAKE_MESSID(30, 75) },
    { MAKE_MESSID(30, 76), MAKE_MESSID(30, 77) },
    { MAKE_MESSID(30, 78), MAKE_MESSID(30, 79) },
    { MAKE_MESSID(30, 80), MAKE_MESSID(30, 81) }
};

Void StartMinigameClearResult(HUPROCESS* resultProcess, s32 boardId, s32 winningPlayerIndex) {
    minigameClearObjectManager = resultProcess;
    resultState.boardId = boardId;
    resultState.winningPlayerIndex = winningPlayerIndex;
    InitializeStoryWindows();
    resultState.unreferencedSequenceState = CreateStoryLighting(0xFF);
    InitializeStoryCamera(minigameClearObjectManager, SetMinigameClearCameraStart);
    CreateStoryShadow();
    CaptureMinigameClearPlayerResults(winningPlayerIndex);
    HuPrcChildCreate(CreateMinigameClearStage, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(CreateMinigameClearCharacter, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(CreateMinigameClearWinnerObject, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(CreateMinigameClearController, 100, 0x2000, 0, HuPrcCurrentGet());
}

Void AdvanceMinigameClearObjectMotion(OMOBJ* object) {
    if (object->work[2] != 0) {
        object->work[2]--;
        return;
    }
    if (object->work[2] == 0 && Hu3DMotionEndCheck(object->mdlId[1])) {
        SetObjectMotion(object, 1, object->work[1], 15, 1);
        object->work[0] = object->work[1] = object->work[2] = 0;
    }
}

Void DisplayMinigameClearDebugPanel(Void) {
    s32 debugWindowX = 16;
    s32 debugWindowY = 40;
    GXColor debugPanelColor = { 0x00, 0x00, 0x80, 0x80 };
    s32 i;

    while (TRUE) {
        WaitOneFrame();
        printWin(debugWindowX, debugWindowY, 220, 130, &debugPanelColor);
        if (minigameClearControllerState.callback == NULL) {
            print8(debugWindowX, debugWindowY, 1.0f, "PROC_MAIN   -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY, 1.0f, "PROC_MAIN   -> TRUE");
        }
        if (minigameClearStageState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 10, 1.0f, "PROC_MAP    -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 10, 1.0f, "PROC_MAP    -> TRUE");
        }
        if (minigameClearAudioState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 20, 1.0f, "PROC_SUB    -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 20, 1.0f, "PROC_SUB    -> TRUE");
        }
        if (minigameClearWinnerState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 30, 1.0f, "PROC_PLAYER -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 30, 1.0f, "PROC_PLAYER -> TRUE");
        }
        print8(debugWindowX, debugWindowY + 60, 1.0f, "M:%d P:%d W:%d", resultState.boardId, resultState.currentPlayerIndex, resultState.winningPlayerIndex);
        print8(debugWindowX, debugWindowY + 80, 1.0f, "NO) IDX:GRP:COM:DIF:PAD:CHR");
        for (i = 0; i < 4; i++) {
            print8(debugWindowX, debugWindowY + 90 + i * 10, 1.0f, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", i,
                resultState.playerResults[i].playerIndex, resultState.playerResults[i].group, resultState.playerResults[i].isComputer,
                resultState.playerResults[i].difficulty, resultState.playerResults[i].controllerPort, resultState.playerResults[i].characterId);
        }
    }
}

Void PlayMinigameClearIntroMusic(Void) {
    storySequenceIds[0] = HuAudSeqPlay(0x36);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet() != 0);
    WaitFrames(60);
}

Void ShowMinigameClearSequence(Void) {
    resultState.messageWindowId = CreateCenteredStoryWindow(1, 0, 21, 2, resultState.winningPlayerIndex);
    InsertStoryMessage(resultState.messageWindowId, resultState.playerResults[resultState.currentPlayerIndex].characterId, 0);
    SetStoryMessage(resultState.messageWindowId, minigameClearMessageIds[resultState.boardId][0]);
    WaitForStoryMessage(resultState.messageWindowId, -1);
    SetObjectMotion(minigameClearAudioState.object, 1, 1, 15, 1);
    CloseCenteredStoryWindow(resultState.messageWindowId);
    SetObjectMotion(minigameClearStageState.object, 2, 2, 0, 0);
    WaitFrames(90);
    SetObjectMotion(minigameClearAudioState.object, 1, 4, 15, 0);
    WaitFrames(30);
    SetObjectMotion(minigameClearWinnerState.object, 1, 4, 15, 0);
    WaitForObjectMotionEnd(minigameClearStageState.object, 2, 2, 0);
    WaitFrames(60);
    SetObjectMotion(minigameClearWinnerState.object, 1, 1, 84, 1);
    SetObjectMotion(minigameClearAudioState.object, 1, 1, 84, 1);
    CaptureCameraStartState(&storyCameraState);
    SetStoryCameraCallback(MoveMinigameClearCameraToTarget);
    minigameClearStageState.callback = (Void*) MoveRewardCarrierForward;
    do {
        WaitOneFrame();
    } while (minigameClearStageState.callback != NULL);
    WaitFrames(10);
    resultState.messageWindowId = CreateCenteredStoryWindow(1, 0, 21, 2, resultState.winningPlayerIndex);
    SetObjectMotion(minigameClearAudioState.object, 1, 3, 15, 1);
    if (resultState.boardId == 0) {
        HuAudFXPlay(storyEventSoundEffectIds[resultState.boardId][2]);
    } else {
        HuAudFXPlay(storyEventSoundEffectIds[resultState.boardId][0]);
    }
    InsertStoryMessage(resultState.messageWindowId, MAKE_MESSID(50, resultState.playerResults[resultState.currentPlayerIndex].characterId * 6 + 5) + resultState.boardId, 2);
    SetStoryMessage(resultState.messageWindowId, minigameClearMessageIds[resultState.boardId][1]);
    WaitForStoryMessage(resultState.messageWindowId, -1);
    SetObjectMotion(minigameClearAudioState.object, 1, 1, 15, 1);
    CloseCenteredStoryWindow(resultState.messageWindowId);
}

Void PlayMinigameClearRewardAudio(s32 boardId, StoryProcessTaskState* taskState) {
    s32 audioStreamId;

    audioStreamId = HuAudSStreamPlay(7);
    do {
        WaitOneFrame();
    } while (HuAudSStreamStatGet(audioStreamId) != 0);
    HuAudSeqPause(storySequenceIds[0], 0, 1000);
    taskState->callback = 0;
}

Void PlayRewardRevealEffect(OMOBJ* object, StoryProcessTaskState* taskState) {
    s32 presentIndex;

    presentIndex = resultState.playerResults[resultState.currentPlayerIndex].characterId * 6 + resultState.boardId;
    WaitFrames(150);
    OSReport("########## PRESENT_SE : %d\n", presentIndex);
    HuAudFXPlay(0x8D4 + presentIndex);
    if (minigameRewardData[presentIndex].hasRewardMotion == 1) {
        SetObjectMotion(minigameClearStageState.object, 7, 7, 0, 0);
        WaitFrames(60);
        SetObjectMotion(minigameClearStageState.object, 7, 12, 0, 1);
    } else {
        SetObjectMotion(minigameClearStageState.object, 7, 7, 1, 0);
    }
    taskState->callback = 0;
}

Void AnimateMinigameClearWinner(Void) {
    s32 i;

    SetObjectMotion(minigameClearWinnerState.object, 1, 2, 15, 1);
    for (i = 0; i < 30; i++) {
        WaitOneFrame();
        if (resultState.playerResults[resultState.currentPlayerIndex].characterId != 5) {
            Hu3DData[minigameClearWinnerState.object->mdlId[1]].pos.x = InterpolateLinear(120.0f, 80.0f, i, 30.0f);
        }
        Hu3DData[minigameClearWinnerState.object->mdlId[1]].rot.y = InterpolateLinear(-60.0f, -90.0f, i, 30.0f);
    }
    SetObjectMotion(minigameClearWinnerState.object, 1, 5, 5, 0);
    while (TRUE) {
        WaitOneFrame();
        if (Hu3DMotionTimeGet(minigameClearWinnerState.object->mdlId[1]) >= 35.0f) {
            break;
        }
    }
    Hu3DModelPosSet(minigameClearStageState.object->mdlId[5], 0.0f, 0.0f, 0.0f);
    Hu3DModelHookReset(minigameClearStageState.object->mdlId[9]);
    Hu3DModelHookSet(minigameClearWinnerState.object->mdlId[1], CharModelItemHookGet(resultState.playerResults[resultState.currentPlayerIndex].characterId, 1, 1), minigameClearStageState.object->mdlId[5]);
    WaitFrames(60);
    minigameClearStageState.callback = (Void*) AnimateRewardFloat;
    HuAudFXPlay(0xA3);
    CharFXPlay(resultState.playerResults[resultState.currentPlayerIndex].characterId, 0x128);
    SetObjectMotion(minigameClearStageState.object, 6, 6, 0, 0);
    Hu3DModelAttrReset(minigameClearStageState.object->mdlId[6], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(minigameClearStageState.object->mdlId[5], HU3D_ATTR_DISPOFF);
    SetObjectMotion(minigameClearWinnerState.object, 1, 6, 5, 0);
    HuAudSeqPause(storySequenceIds[0], 1, 1000);
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    do {
        WaitOneFrame();
    } while (WipeStatGet() != 0);
    Hu3DMotionSpeedSet(minigameClearWinnerState.object->mdlId[1], 0.0f);
    Hu3DMotionTimeSet(minigameClearWinnerState.object->mdlId[1], 50.0f);
    Hu3DModelAttrReset(minigameClearStageState.object->mdlId[7], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(minigameClearStageState.object->mdlId[8], HU3D_ATTR_DISPOFF);
    SetObjectMotion(minigameClearStageState.object, 8, 8, 0, 1);
    WaitFrames(30);
    HuAudFXPlay(0xA9);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    do {
        WaitOneFrame();
    } while (WipeStatGet() != 0);
    CaptureCameraStartState(&storyCameraState);
    SetStoryCameraCallback(TransitionMinigameClearCamera);
    WaitFrames(60);
    minigameClearWinnerState.callback = (Void*) PlayRewardRevealEffect;
    resultState.messageWindowId = CreateCenteredStoryWindow(1, 0, 21, 2, resultState.winningPlayerIndex);
    minigameClearAudioState.callback = (Void*) PlayMinigameClearRewardAudio;
    InsertStoryMessage(resultState.messageWindowId, MAKE_MESSID(50, resultState.playerResults[resultState.currentPlayerIndex].characterId * 6 + 6) + resultState.boardId, 0);
    DisplayStoryMessagePages(resultState.messageWindowId, MAKE_MESSID(50, 134), 0, 300);
    SetStoryMessage(resultState.messageWindowId, MAKE_MESSID(37, 4));
    CloseCenteredStoryWindow(resultState.messageWindowId);
    CaptureCameraStartState(&storyCameraState);
    SetStoryCameraCallback(TrackRewardWithCamera);
    minigameClearStageState.callback = (Void*) AnimateRewardRise;
    WaitFrames(30);
    Hu3DMotionSpeedSet(minigameClearWinnerState.object->mdlId[1], 0.1f);
    WaitFrames(180);
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
}

s32 IsBoardStoryComplete(s32 boardId) {
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

Void SaveMinigameClearProgress(Void) {
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
    minigameClearSaveComplete = 1;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

Void LaunchPresentOverlay(Void) {
    OMOVLHIS* previousOverlayHistory;

    msmSeStopAll(FALSE, 1000);
    do {
        WaitOneFrame();
    } while (WipeStatGet() != 0);
    WaitFrames(60);
    CharModelKill(-1);
    HuAudFXAllStop();
    HuAudSStreamAllStop();
    {
        s32 boardStoryOverlayIds[] = { 53, 56, 55, 54, 57 };

        GWMGAvailSet(omMgIndexGet(boardStoryOverlayIds[resultState.boardId]) + 0x191);
    }
    GWGameStat.present[resultState.playerResults[resultState.currentPlayerIndex].characterId * 6 + resultState.boardId] = 1;
    OSReport("########## PRESENTFLG CHR:%d, MAP:%d, PRESENT:%d\n", resultState.playerResults[resultState.currentPlayerIndex].characterId, resultState.boardId, resultState.playerResults[resultState.currentPlayerIndex].characterId * 6 + resultState.boardId);
    previousOverlayHistory = omOvlHisGet(0);
    omOvlHisChg(0, DLL_mstorydll, 20, previousOverlayHistory->stat);
    omOvlCallEx(DLL_present, 1, resultState.playerResults[resultState.currentPlayerIndex].characterId * 6 + resultState.boardId + 1, 0);
}

Void RunMinigameClearEnding(OMOBJ* object, StoryProcessTaskState* taskState) {
    s32 unreferencedTransitionState = 0;

    storySequenceIds[0] = HuAudSeqPlay(0x36);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet() != 0);
    WaitFrames(60);
    ShowMinigameClearSequence();
    AnimateMinigameClearWinner();
    LaunchPresentOverlay();
    taskState->callback = 0;
}

Void CreateMinigameClearController(Void) {
    OMOBJ* gameObject;

    gameObject = omAddObjEx(minigameClearObjectManager, 0x100, 16, 16, -1, NULL);
    minigameClearControllerState.object = gameObject;
    minigameClearControllerState.callback = (Void*) RunMinigameClearEnding;
    minigameClearControllerState.object->data = &minigameClearControllerState;
    while (TRUE) {
        WaitOneFrame();
        if (minigameClearControllerState.callback != NULL) {
            minigameClearControllerState.callback(gameObject, &minigameClearControllerState);
        }
    }
}

Void LoadMinigameRewardModels(OMOBJ* object, s32 eventIndex) {
    s32 playerCharacterId;

    playerCharacterId = resultState.playerResults[resultState.currentPlayerIndex].characterId;
    object->mdlId[7] = Hu3DModelCreateFile(minigameRewardData[playerCharacterId * 6 + eventIndex].modelDataId);
    object->mtnId[7] = Hu3DMotionIDGet(object->mdlId[7]);
    if (minigameRewardData[playerCharacterId * 6 + eventIndex].hasRewardMotion == 1) {
        object->mtnId[12] = Hu3DJointMotionFile(object->mdlId[7], DATA_MAKE_NUM(DATADIR_MSTORY4, 48));
    }
    Hu3DModelPosSet(object->mdlId[7], 0.0f, 180.0f, 825.0f);
    Hu3DModelAttrSet(object->mdlId[7], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowSet(object->mdlId[7]);
    SetObjectMotion(object, 7, 7, 0, 2);
    HuDataDirClose(DATADIR_MSTORY4);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 56));
    object->mtnId[0] = Hu3DMotionIDGet(object->mdlId[1]);
    SetObjectMotion(object, 1, 0, 0, 2);
    Hu3DModelShadowMapObjSet(object->mdlId[1], "base_fix9-base");
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 59));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY, 60));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY, 61));
    SetObjectMotion(object, 2, 2, 0, 2);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 57));
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 81));
    Hu3DModelShadowSet(object->mdlId[5]);
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 83));
    object->mtnId[6] = Hu3DMotionIDGet(object->mdlId[6]);
    Hu3DModelPosSet(object->mdlId[6], 25.0f, 125.0f, 900.0f);
    Hu3DModelAttrSet(object->mdlId[6], HU3D_ATTR_DISPOFF);
    SetObjectMotion(object, 6, 6, 0, 2);
    object->mdlId[8] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 96));
    object->mtnId[8] = Hu3DMotionIDGet(object->mdlId[8]);
    Hu3DModelAttrSet(object->mdlId[8], HU3D_ATTR_DISPOFF);
    SetObjectMotion(object, 8, 8, 0, 2);
    object->mdlId[9] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 84));
    object->mtnId[9] = Hu3DMotionIDGet(object->mdlId[9]);
    Hu3DModelPosSet(object->mdlId[9], 0.0f, 0.0f, 460.0f);
    Hu3DModelShadowSet(object->mdlId[9]);
    SetObjectMotion(object, 9, 9, 0, 2);
    Hu3DModelHookSet(object->mdlId[9], "carrier2-target0", object->mdlId[5]);
    minigameClearRewardAnimations[0] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY, 85 + eventIndex * 2), HU_MEMNUM_OVL));
    minigameClearRewardAnimations[1] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY, 86 + eventIndex * 2), HU_MEMNUM_OVL));
    Hu3DAnimCreate(minigameClearRewardAnimations[0], minigameClearStageState.object->mdlId[5], "cylinder3");
    Hu3DAnimCreate(minigameClearRewardAnimations[1], minigameClearStageState.object->mdlId[5], "wakuwaku");
}

Void AnimateRewardFloat(OMOBJ* object, StoryProcessTaskState* taskState) {
    Mtx effectObjectMatrix;

    Hu3DData[object->mdlId[7]].pos.y = InterpolateSineCycle(180.0f, 185.0f, taskState->animationPhase++, 360.0f);
    Hu3DModelObjMtxGet(object->mdlId[7], "effect", effectObjectMatrix);
    Hu3DModelPosSet(object->mdlId[8], effectObjectMatrix[0][3], effectObjectMatrix[1][3], effectObjectMatrix[2][3]);
    if (taskState->animationPhase >= 360) {
        taskState->animationPhase = 0;
    }
}

Void AnimateRewardRise(OMOBJ* object, StoryProcessTaskState* taskState) {
    Mtx effectObjectMatrix;

    Hu3DData[object->mdlId[7]].pos.y += taskState->verticalVelocity / 100.0f;
    Hu3DModelObjMtxGet(object->mdlId[7], "effect", effectObjectMatrix);
    Hu3DModelPosSet(object->mdlId[8], effectObjectMatrix[0][3], effectObjectMatrix[1][3], effectObjectMatrix[2][3]);
    taskState->verticalVelocity += 2;
}

Void MoveRewardCarrierForward(OMOBJ* object, StoryProcessTaskState* taskState) {
    SetObjectMotion(object, 9, 9, 0, 1);
    while (TRUE) {
        WaitOneFrame();
        Hu3DData[object->mdlId[9]].pos.z += 2.5f;
        if (Hu3DData[object->mdlId[9]].pos.z >= 670.0f) {
            break;
        }
    }
    Hu3DData[object->mdlId[9]].pos.z = 670.0f;
    Hu3DMotionSpeedSet(object->mdlId[9], 0.0f);
    taskState->callback = 0;
}

Void CreateMinigameClearStage(Void) {
    OMOBJ* gameObject;

    gameObject = omAddObjEx(minigameClearObjectManager, 0x100, 16, 16, -1, NULL);
    minigameClearStageState.object = gameObject;
    minigameClearStageState.callback = NULL;
    LoadMinigameRewardModels(gameObject, resultState.boardId);
    while (TRUE) {
        WaitOneFrame();
        if (minigameClearStageState.callback != NULL) {
            minigameClearStageState.callback(gameObject, &minigameClearStageState);
        }
    }
}

Void LoadMinigameClearCharacter(OMOBJ* object, s32 characterIndex) {
    s32 characterModelIds[] = {
        DATA_MAKE_NUM(DATADIR_MSTORY, 0x0061),
        DATA_MAKE_NUM(DATADIR_MSTORY, 0x006C),
        DATA_MAKE_NUM(DATADIR_MSTORY, 0x0074),
        DATA_MAKE_NUM(DATADIR_MSTORY, 0x007C),
        DATA_MAKE_NUM(DATADIR_MSTORY, 0x0083)
    };
    s32 characterMotionIdsByBoard[][5] = {
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x0062), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0064), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0065), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0067), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0068) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x006D), DATA_MAKE_NUM(DATADIR_MSTORY, 0x006F), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0070), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0072), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0073) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x0075), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0077), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0078), DATA_MAKE_NUM(DATADIR_MSTORY, 0x007A), DATA_MAKE_NUM(DATADIR_MSTORY, 0x007B) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x007D), DATA_MAKE_NUM(DATADIR_MSTORY, 0x007E), DATA_MAKE_NUM(DATADIR_MSTORY, 0x007F), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0081), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0082) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x0084), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0086), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0087), DATA_MAKE_NUM(DATADIR_MSTORY, 0x0089), DATA_MAKE_NUM(DATADIR_MSTORY, 0x008A) }
    };

    object->mdlId[1] = Hu3DModelCreateFile(characterModelIds[characterIndex]);
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][0]);
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][1]);
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][2]);
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][3]);
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][4]);
    Hu3DModelPosSet(object->mdlId[1], -120.0f, 0.0f, 670.0f);
    Hu3DModelRotSet(object->mdlId[1], 0.0f, 30.0f, 0.0f);
    Hu3DModelScaleSet(object->mdlId[1], 1.0f, 1.0f, 1.0f);
    if (characterIndex == 3) {
        Hu3DData[object->mdlId[1]].pos.y = 50.0f;
    }
    Hu3DModelShadowSet(object->mdlId[1]);
    SetObjectMotion(object, 1, 5, 0, 1);
    {
        s32 characterDustEffectIds[] = { 9, 14, 10, 11, 12 };

        if (characterIndex != 3) {
            CharNpcDustSet(object->mdlId[1], object->mtnId[2], 1, characterDustEffectIds[characterIndex]);
        }
    }
}

Void CreateMinigameClearCharacter(Void) {
    OMOBJ* gameObject;

    gameObject = omAddObjEx(minigameClearObjectManager, 0x100, 16, 16, -1, NULL);
    minigameClearAudioState.object = gameObject;
    minigameClearAudioState.callback = NULL;
    LoadMinigameClearCharacter(gameObject, resultState.boardId);
    while (TRUE) {
        WaitOneFrame();
        if (minigameClearAudioState.callback != NULL) {
            minigameClearAudioState.callback(gameObject, &minigameClearAudioState);
        }
    }
}

Void CaptureMinigameClearPlayerResults(s32 playerIndex) {
    s32 playerOrder[4];
    s32 i;

    playerOrder[0] = 0;
    for (i = 1; i < 4; i++) {
        playerOrder[i] = playerOrder[i - 1] + 1;
        if (playerOrder[i] >= 4) {
            playerOrder[i] = 0;
        }
        if (playerIndex == playerOrder[i]) {
            resultState.currentPlayerIndex = i;
        }
    }
    for (i = 0; i < 4; i++) {
        CapturePlayerResult(&resultState.playerResults[i], playerOrder[i]);
    }
}

Void LoadMinigameClearWinner(OMOBJ* object, s32 eventIndex) {
    s32 playerCharacterId;

    playerCharacterId = resultState.playerResults[resultState.currentPlayerIndex].characterId;
    object->mdlId[1] = CharModelCreate(playerCharacterId, 1);
    object->mtnId[1] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0));
    object->mtnId[2] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 2));
    object->mtnId[3] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 53));
    object->mtnId[4] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 109));
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY, 24 + playerCharacterId));
    object->mtnId[6] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY, 32 + playerCharacterId));
    CharMotionDataClose(playerCharacterId);
    Hu3DModelPosSet(object->mdlId[1], 120.0f, 0.0f, 670.0f);
    Hu3DModelRotSet(object->mdlId[1], 0.0f, -60.0f, 0.0f);
    Hu3DModelScaleSet(object->mdlId[1], 1.0f, 1.0f, 1.0f);
    Hu3DModelShadowSet(object->mdlId[1]);
    SetObjectMotion(object, 1, 1, 0, 1);
}

Void SetMinigameClearWinnerMotion(s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 loopMode, s32 initialDelayFrames) {
    SetObjectMotion(minigameClearWinnerState.object, 1, modelIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        initialDelayFrames = 0;
    }
    minigameClearWinnerState.object->work[0] = initialDelayFrames;
    minigameClearWinnerState.object->work[1] = motionIndex;
    minigameClearWinnerState.object->work[2] = blendFrames + 1;
}

Void CreateMinigameClearWinnerObject(Void) {
    OMOBJ* gameObject;

    gameObject = omAddObjEx(minigameClearObjectManager, 0x100, 16, 16, -1, NULL);
    minigameClearWinnerState.object = gameObject;
    minigameClearWinnerState.callback = NULL;
    minigameClearWinnerState.object->data = &minigameClearWinnerState;
    LoadMinigameClearWinner(gameObject, resultState.currentPlayerIndex); // Second argument required to match.
    while (TRUE) {
        WaitOneFrame();
        if (minigameClearWinnerState.callback != NULL) {
            minigameClearWinnerState.callback(gameObject, &minigameClearWinnerState);
        }
        if (gameObject->work[0] != 0) {
            AdvanceMinigameClearObjectMotion(gameObject);
        }
    }
}

Void SetMinigameClearCameraStart(Void) {
    StoryCameraTransitionState* cameraState;

    cameraState = &storyCameraState;
    cameraState->centerPosition.x = 0.0f;
    cameraState->centerPosition.y = 125.0f;
    cameraState->centerPosition.z = 0.0f;
    cameraState->rotation.x = -5.0f;
    cameraState->rotation.y = 0.0f;
    cameraState->rotation.z = 0.0f;
    cameraState->zoom = 1750.0f;
}

Void MoveMinigameClearCameraToTarget(Void) {
    StoryCameraTransitionState* cameraState;
    StoryCameraTransitionState cameraTarget;

    cameraState = &storyCameraState;
    cameraTarget.centerPosition.x = 0.0f;
    cameraTarget.centerPosition.y = 100.0f;
    cameraTarget.centerPosition.z = 0.0f;
    cameraTarget.rotation.x = 0.0f;
    cameraTarget.rotation.y = 0.0f;
    cameraTarget.rotation.z = 0.0f;
    cameraTarget.zoom = 1400.0f;
    InterpolateCameraTransition(cameraState, &cameraTarget, cameraState->transitionFrame++, 120.0f, 10.0f);
}

Void TrackRewardWithCamera(Void) {
    StoryCameraTransitionState* cameraState;

    cameraState = &storyCameraState;
    cameraState->targetCenterX = 0.0f;
    cameraState->targetCenterY = Hu3DData[minigameClearStageState.object->mdlId[7]].pos.y;
    cameraState->targetCenterZ = 0.0f;
    cameraState->targetRotationX = 0.0f;
    cameraState->targetRotationY = 0.0f;
    cameraState->targetRotationZ = 0.0f;
    cameraState->targetZoom = InterpolateLinear(1100.0f, 1400.0f, cameraState->transitionFrame++, 240.0f);
    InterpolateCameraCenterSine(cameraState, 30.0f);
}

Void TransitionMinigameClearCamera(Void) {
    StoryCameraTransitionState* cameraState;
    StoryCameraTransitionState cameraTarget;

    cameraState = &storyCameraState;
    cameraTarget.centerPosition.x = 0.0f;
    cameraTarget.centerPosition.y = 160.0f;
    cameraTarget.centerPosition.z = 0.0f;
    cameraTarget.rotation.x = 0.0f;
    cameraTarget.rotation.y = 0.0f;
    cameraTarget.rotation.z = 0.0f;
    cameraTarget.zoom = 1100.0f;
    InterpolateCameraTransitionEaseOut(cameraState, &cameraTarget, cameraState->transitionFrame++, 10.0f, 5.0f);
}
