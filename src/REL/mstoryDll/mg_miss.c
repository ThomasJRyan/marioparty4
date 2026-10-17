#include "REL/mstoryDll.h"
#include "game/chrman.h"

#ifndef __MWERKS__
#include "game/audio.h"
#endif

#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/printfunc.h"
#include "game/window.h"
#include "game/wipe.h"

typedef struct {
    /* 0x00 */ s32 boardId;
    /* 0x04 */ s32 currentPlayerIndex;
    /* 0x08 */ s32 messageWindowId;
    /* 0x0C */ s32 choiceWindowId;
    /* 0x10 */ s32 winningPlayerIndex;
    /* 0x14 */ char unreferencedStatePadding[0x20];
    /* 0x34 */ StoryPlayerResult playerResults[4];
} MinigameMissResultState; // Size 0x104

Void CreateMinigameMissController(Void);
Void CreateMinigameMissStage(Void);
Void CaptureMinigameMissPlayerResults(s32 playerIndex);
Void WaitForMinigameMissMotion(OMOBJ* object, StoryProcessTaskState* taskState);
Void AnimateMinigameMissWinnerTurn(OMOBJ*, StoryProcessTaskState* taskState);
Void CreateMinigameMissWinner(Void);
Void CreateMinigameMissCharacterObject(Void);
Void SetMinigameMissCameraStart(Void);
Void TransitionMinigameMissCameraToEnd(Void);

MinigameMissResultState storyMissState;
StoryProcessTaskState minigameMissControllerState;
StoryProcessTaskState minigameMissStageState;
StoryProcessTaskState minigameMissWinnerState;
StoryProcessTaskState minigameMissCharacterState;
HUPROCESS* minigameMissObjectManager;

s32 minigameMissMessageIds[][3] = {
    { MAKE_MESSID(30, 40), MAKE_MESSID(30, 45), MAKE_MESSID(30, 5) },
    { MAKE_MESSID(30, 41), MAKE_MESSID(30, 45), MAKE_MESSID(30, 5) },
    { MAKE_MESSID(30, 42), MAKE_MESSID(30, 45), MAKE_MESSID(30, 5) },
    { MAKE_MESSID(30, 43), MAKE_MESSID(30, 45), MAKE_MESSID(30, 5) },
    { MAKE_MESSID(30, 44), MAKE_MESSID(30, 45), MAKE_MESSID(30, 5) }
};

Void StartMinigameMissResult(HUPROCESS* resultProcess, s32 boardId, s32 winningPlayerIndex) {
    minigameMissObjectManager = resultProcess;
    storyMissState.boardId = boardId;
    storyMissState.winningPlayerIndex = winningPlayerIndex;
    InitializeStoryWindows();
    CreateStoryLighting(0xFF);
    InitializeStoryCamera(minigameMissObjectManager, SetMinigameMissCameraStart);
    CreateStoryShadow();
    CaptureMinigameMissPlayerResults(winningPlayerIndex);
    HuPrcChildCreate(CreateMinigameMissStage, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(CreateMinigameMissWinner, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(CreateMinigameMissCharacterObject, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(CreateMinigameMissController, 100, 0x2000, 0, HuPrcCurrentGet());
}

Void DisplayMinigameMissDebugPanel(Void) {
    s32 debugWindowX = 16;
    s32 debugWindowY = 40;
    GXColor debugPanelColor = { 0x00, 0x00, 0x80, 0x80 };
    s32 i;

    while (TRUE) {
        WaitOneFrame();
        fontcolor = FONT_COLOR_WHITE;
        printWin(debugWindowX, debugWindowY, 220, 120, &debugPanelColor);
        if (minigameMissControllerState.callback == NULL) {
            print8(debugWindowX, debugWindowY, 1.0f, "PROC_MAIN   -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY, 1.0f, "PROC_MAIN   -> TRUE");
        }
        if (minigameMissStageState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 10, 1.0f, "PROC_MAP    -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 10, 1.0f, "PROC_MAP    -> TRUE");
        }
        if (minigameMissWinnerState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 20, 1.0f, "PROC_PLAYER -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 20, 1.0f, "PROC_PLAYER -> TRUE");
        }
        if (minigameMissCharacterState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 30, 1.0f, "PROC_SUB    -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 30, 1.0f, "PROC_SUB    -> TRUE");
        }
        print8(debugWindowX, debugWindowY + 50, 1.0f, "M:%d P:%d W:%d", storyMissState.boardId, storyMissState.currentPlayerIndex, storyMissState.winningPlayerIndex);
        print8(debugWindowX, debugWindowY + 70, 1.0f, "NO) IDX:GRP:COM:DIF:PAD:CHR");
        for (i = 0; i < 4; i++) {
            print8(debugWindowX, debugWindowY + 80 + i * 10, 1.0f, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", i,
                storyMissState.playerResults[i].playerIndex, storyMissState.playerResults[i].group, storyMissState.playerResults[i].isComputer,
                storyMissState.playerResults[i].difficulty, storyMissState.playerResults[i].controllerPort, storyMissState.playerResults[i].characterId);
        }
    }
}

Void PlayMinigameMissIntroMusic(Void) {
    storySequenceIds[0] = HuAudSeqPlay(0x34);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet() != 0);
    WaitFrames(60);
}

Void ShowMinigameMissIntroduction(Void) {
    s32 unreferencedDialogState = 0;
    s32 unreferencedDialogFlags = 0;

    SetObjectMotion(minigameMissWinnerState.object, 1, 2, 15, 0);
    minigameMissWinnerState.callback = (Void*) WaitForMinigameMissMotion;
    HuAudFXPlay(storyEventSoundEffectIds[storyMissState.boardId][1]);
    CharFXPlay(storyMissState.playerResults[storyMissState.currentPlayerIndex].characterId, 0x12E);
    storyMissState.messageWindowId = CreateCenteredStoryWindow(1, 0, 21, 2, storyMissState.winningPlayerIndex);
    SetObjectMotion(minigameMissCharacterState.object, 1, 2, 15, 1);
    SetStoryMessage(storyMissState.messageWindowId, minigameMissMessageIds[storyMissState.boardId][0]);
    WaitForStoryMessage(storyMissState.messageWindowId, -1);
    SetStoryMessage(storyMissState.messageWindowId, MAKE_MESSID(37, 4));
    SetObjectMotion(minigameMissCharacterState.object, 1, 1, 15, 1);
    CloseCenteredStoryWindow(storyMissState.messageWindowId);
}

BOOL PromptContinueMinigameStory(Void) {
    BOOL continueStory = FALSE;

    HuWinInsertMesSet(storyMissState.messageWindowId, MAKE_MESSID(30, 54) + storyMissState.boardId, 3);
    SetStoryMessage(storyMissState.messageWindowId, minigameMissMessageIds[storyMissState.boardId][1]);
    WaitForStoryMessage(storyMissState.messageWindowId, 5);
    storyMissState.choiceWindowId = CreateStoryChoiceWindow(MAKE_MESSID(30, 53), storyMissState.winningPlayerIndex);
    SetStoryMessage(storyMissState.choiceWindowId, MAKE_MESSID(30, 53));
    continueStory = WaitForFirstStoryChoice(storyMissState.choiceWindowId);
    SetStoryMessage(storyMissState.choiceWindowId, MAKE_MESSID(37, 4));
    CloseStoryChoiceWindow(storyMissState.choiceWindowId);
    if (continueStory) {
        SetObjectMotion(minigameMissWinnerState.object, 1, 4, 15, 0);
        minigameMissWinnerState.callback = NULL;
        SetObjectMotion(minigameMissCharacterState.object, 1, 3, 15, 1);
    }
    return continueStory;
}

BOOL PromptRetryMinigame(Void) {
    s32 retrySelected = FALSE;

    SetStoryMessage(storyMissState.messageWindowId, minigameMissMessageIds[storyMissState.boardId][2]);
    WaitForStoryMessage(storyMissState.messageWindowId, 5);
    storyMissState.choiceWindowId = CreateStoryChoiceWindow(MAKE_MESSID(30, 53), storyMissState.winningPlayerIndex);
    SetStoryMessage(storyMissState.choiceWindowId, MAKE_MESSID(30, 53));
    retrySelected = WaitForStoryChoice(storyMissState.choiceWindowId, 1);
    SetStoryMessage(storyMissState.choiceWindowId, MAKE_MESSID(37, 4));
    CloseStoryChoiceWindow(storyMissState.choiceWindowId);
    return retrySelected;
}

Void ReturnToMinigameBoard(Void) {
    minigameMissWinnerState.callback = (Void*) AnimateMinigameMissWinnerTurn;
    HuAudSeqFadeOut(storySequenceIds[0], 3000);
    CaptureCameraStartState(&storyCameraState);
    SetStoryCameraCallback(TransitionMinigameMissCameraToEnd);
    WaitFrames(150);
}

Void RunEmptyMinigameMissCallback(Void) {
}

Void TransitionAfterMinigameMiss(s32 boardId) {
    OMOVLHIS* previousOverlayHistory;
    OMOVL boardOverlayIds[] = { DLL_m445dll, DLL_m448dll, DLL_m447dll, DLL_m446dll, DLL_m449dll };

    if (boardId != 0) {
        WaitFrames(60);
        HuAudSeqFadeOut(storySequenceIds[0], 1000);
    }
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet() != 0);
    WaitFrames(60);
    CharModelKill(-1);
    previousOverlayHistory = omOvlHisGet(0);
    omOvlHisChg(0, DLL_mstorydll, 2, previousOverlayHistory->stat);
    if (boardId == 0) {
        HuAudSeqAllFadeOut(1000);
        HuAudSStreamAllFadeOut(1000);
        omOvlCallEx(boardOverlayIds[GWSystem.board], 1, 0, 0);
    } else {
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

Void RunMinigameMissChoiceSequence(OMOBJ* object, StoryProcessTaskState* taskState) {
    s32 selectedStoryChoice;

    PlayMinigameMissIntroMusic();
    ShowMinigameMissIntroduction();
    storyMissState.messageWindowId = CreateCenteredStoryWindow(1, 0, 21, 2, storyMissState.winningPlayerIndex);
    while (TRUE) {
        WaitOneFrame();
        if (PromptContinueMinigameStory()) {
            selectedStoryChoice = 0;
            break;
        }
        if (PromptRetryMinigame()) {
            selectedStoryChoice = 1;
            break;
        }
    }
    CloseCenteredStoryWindow(storyMissState.messageWindowId);
    WaitFrames(60);
    switch (selectedStoryChoice) {
        case 0:
            ReturnToMinigameBoard();
            break;
        case 1:
            RunEmptyMinigameMissCallback();
            break;
    }
    TransitionAfterMinigameMiss(selectedStoryChoice);
    taskState->callback = NULL;
}

Void CreateMinigameMissController(Void) {
    OMOBJ* gameObject;

    gameObject = omAddObjEx(minigameMissObjectManager, 0x100, 16, 16, -1, NULL);
    minigameMissControllerState.object = gameObject;
    minigameMissControllerState.callback = (Void*) RunMinigameMissChoiceSequence;
    minigameMissControllerState.object->data = &minigameMissControllerState;
    while (TRUE) {
        WaitOneFrame();
        if (minigameMissControllerState.callback != NULL) {
            minigameMissControllerState.callback(gameObject, &minigameMissControllerState);
        }
    }
}

Void LoadMinigameMissStageModels(OMOBJ* object) {
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 56));
    object->mtnId[0] = Hu3DMotionIDGet(object->mdlId[1]);
    SetObjectMotion(object, 1, 0, 0, 2);
    Hu3DModelShadowMapObjSet(object->mdlId[1], "base_fix9-base");
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 59));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY, 60));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY, 61));
    SetObjectMotion(object, 2, 2, 0, 2);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 57));
    object->mdlId[5] = Hu3DModelCreateFile(storyMissState.boardId + DATA_MAKE_NUM(DATADIR_MSTORY, 76));
    object->mtnId[5] = Hu3DMotionIDGet(object->mdlId[5]);
    Hu3DModelPosSet(object->mdlId[5], 0.0f, 80.0f, 460.0f);
    Hu3DModelShadowSet(object->mdlId[5]);
    SetObjectMotion(object, 5, 5, 0, 2);
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 75));
    object->mtnId[6] = Hu3DMotionIDGet(object->mdlId[6]);
    Hu3DModelHookSet(object->mdlId[5], "stargbox_fix-effect_fook1", object->mdlId[6]);
    SetObjectMotion(object, 6, 6, 0, 2);
}

Void AnimateMinigameMissStage(OMOBJ* object, StoryProcessTaskState* taskState) {
    Hu3DData[object->mdlId[5]].pos.y = InterpolateSineCycle(80.0f, 90.0f, taskState->animationPhase++, 360.0f);
    if (taskState->animationPhase >= 360) {
        taskState->animationPhase = 0;
    }
}

Void CreateMinigameMissStage(Void) {
    OMOBJ* gameObject;

    gameObject = omAddObjEx(minigameMissObjectManager, 0x100, 16, 16, -1, NULL);
    minigameMissStageState.object = gameObject;
    minigameMissStageState.callback = (Void*) AnimateMinigameMissStage;
    minigameMissStageState.object->data = &minigameMissStageState;
    LoadMinigameMissStageModels(gameObject);
    while (TRUE) {
        WaitOneFrame();
        if (minigameMissStageState.callback != NULL) {
            minigameMissStageState.callback(gameObject, &minigameMissStageState);
        }
    }
}

Void CaptureMinigameMissPlayerResults(s32 playerIndex) {
    s32 playerOrder[4];
    s32 i;

    playerOrder[0] = 0;
    for (i = 1; i < 4; i++) {
        playerOrder[i] = playerOrder[i - 1] + 1;
        if (playerOrder[i] >= 4) {
            playerOrder[i] = 0;
        }
        if (playerIndex == playerOrder[i]) {
            storyMissState.currentPlayerIndex = i;
        }
    }
    for (i = 0; i < 4; i++) {
        CapturePlayerResult(&storyMissState.playerResults[i], playerOrder[i]);
    }
}

Void LoadMinigameMissWinner(OMOBJ* object) {
    s32 playerCharacterId;

    playerCharacterId = storyMissState.playerResults[storyMissState.currentPlayerIndex].characterId;
    object->mdlId[1] = CharModelCreate(playerCharacterId, 1);
    object->mtnId[1] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0));
    object->mtnId[2] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 24));
    object->mtnId[3] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 52));
    object->mtnId[4] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 72));
    object->mtnId[5] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 53));
    CharMotionVoiceOnSet(playerCharacterId, object->mtnId[2], 0);
    CharMotionDataClose(playerCharacterId);
    Hu3DModelPosSet(object->mdlId[1], 120.0f, 0.0f, 670.0f);
    Hu3DModelRotSet(object->mdlId[1], 0.0f, -45.0f, 0.0f);
    Hu3DModelScaleSet(object->mdlId[1], 1.0f, 1.0f, 1.0f);
    Hu3DModelShadowSet(object->mdlId[1]);
    SetObjectMotion(object, 1, 1, 0, 1);
}

Void WaitForMinigameMissMotion(OMOBJ* object, StoryProcessTaskState* taskState) {
    if (object->mtnId[2] == Hu3DMotionIDGet(object->mdlId[1]) && Hu3DMotionEndCheck(object->mdlId[1])) {
        SetObjectMotion(object, 1, 3, 15, 1);
        taskState->callback = 0;
    }
}

Void AnimateMinigameMissWinnerTurn(OMOBJ* object, StoryProcessTaskState* taskState) {
    s32 i;

    WaitFrames(10);
    SetObjectMotion(object, 1, 5, 15, 0);
    WaitOneFrame();
    Hu3DMotionTimeSet(object->mdlId[1], 60.0f);
    for (i = 0; i < 50; i++) {
        WaitOneFrame();
        Hu3DData[object->mdlId[1]].rot.y = InterpolateLinear(-60.0f, -120.0f, i, 60.0f);
    }
    SetObjectMotion(object, 1, 1, 15, 1);
    taskState->callback = 0;
}

Void CreateMinigameMissWinner(Void) {
    OMOBJ* gameObject;

    gameObject = omAddObjEx(minigameMissObjectManager, 0x100, 16, 16, -1, NULL);
    minigameMissWinnerState.object = gameObject;
    minigameMissWinnerState.callback = NULL;
    minigameMissWinnerState.object->data = &minigameMissWinnerState;
    LoadMinigameMissWinner(gameObject);
    while (TRUE) {
        WaitOneFrame();
        if (minigameMissWinnerState.callback != NULL) {
            minigameMissWinnerState.callback(gameObject, &minigameMissWinnerState);
        }
    }
}

Void LoadMinigameMissCharacter(OMOBJ* object, s32 characterIndex) {
    s32 characterModelDataIds[] = {
        DATA_MAKE_NUM(DATADIR_MSTORY, 97),
        DATA_MAKE_NUM(DATADIR_MSTORY, 108),
        DATA_MAKE_NUM(DATADIR_MSTORY, 116),
        DATA_MAKE_NUM(DATADIR_MSTORY, 124),
        DATA_MAKE_NUM(DATADIR_MSTORY, 131)
    };
    s32 characterMotionIdsByBoard[][3] = {
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x62), DATA_MAKE_NUM(DATADIR_MSTORY, 0x65), DATA_MAKE_NUM(DATADIR_MSTORY, 0x67) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x6D), DATA_MAKE_NUM(DATADIR_MSTORY, 0x70), DATA_MAKE_NUM(DATADIR_MSTORY, 0x72) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x75), DATA_MAKE_NUM(DATADIR_MSTORY, 0x78), DATA_MAKE_NUM(DATADIR_MSTORY, 0x7A) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x7D), DATA_MAKE_NUM(DATADIR_MSTORY, 0x7F), DATA_MAKE_NUM(DATADIR_MSTORY, 0x81) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x84), DATA_MAKE_NUM(DATADIR_MSTORY, 0x87), DATA_MAKE_NUM(DATADIR_MSTORY, 0x89) }
    };

    object->mdlId[1] = Hu3DModelCreateFile(characterModelDataIds[characterIndex]);
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][0]);
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][1]);
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][2]);
    Hu3DModelPosSet(object->mdlId[1], -120.0f, 0.0f, 670.0f);
    Hu3DModelRotSet(object->mdlId[1], 0.0f, 45.0f, 0.0f);
    Hu3DModelScaleSet(object->mdlId[1], 1.0f, 1.0f, 1.0f);
    if (characterIndex == 3) {
        Hu3DData[object->mdlId[1]].pos.y = 50.0f;
    }
    Hu3DModelShadowSet(object->mdlId[1]);
    SetObjectMotion(object, 1, 1, 0, 1);
}

Void CreateMinigameMissCharacterObject(Void) {
    OMOBJ* gameObject;

    gameObject = omAddObjEx(minigameMissObjectManager, 0x100, 16, 16, -1, NULL);
    minigameMissCharacterState.object = gameObject;
    minigameMissCharacterState.callback = NULL;
    minigameMissCharacterState.object->data = &minigameMissCharacterState;
    LoadMinigameMissCharacter(gameObject, storyMissState.boardId);
    while (TRUE) {
        WaitOneFrame();
        if (minigameMissCharacterState.callback != NULL) {
            minigameMissCharacterState.callback(gameObject, &minigameMissCharacterState);
        }
    }
}

Void SetMinigameMissCameraStart(Void) {
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

Void TransitionMinigameMissCameraToEnd(Void) {
    StoryCameraTransitionState* cameraState;
    StoryCameraTransitionState cameraTarget;

    cameraState = &storyCameraState;
    if (cameraState->transitionFrame == 50) {
        SetObjectMotion(minigameMissStageState.object, 5, 5, 0, 0);
        SetObjectMotion(minigameMissStageState.object, 6, 6, 0, 0);
        HuAudFXPlay(0xA1);
    }
    cameraTarget.centerPosition.x = 0.0f;
    cameraTarget.centerPosition.y = 130.0f;
    cameraTarget.centerPosition.z = 460.0f;
    cameraTarget.rotation.x = 0.0f;
    cameraTarget.rotation.y = 0.0f;
    cameraTarget.rotation.z = 0.0f;
    cameraTarget.zoom = 150.0f;
    InterpolateCameraTransition(cameraState, &cameraTarget, cameraState->transitionFrame, 180.0f, 10.0f);
    if (cameraState->transitionFrame++ >= 90) {
        if (cameraState->transitionFrame == 91) {
            currentStorySoundEffectId = HuAudFXPlay(0x21);
        }
        cameraState->rotation.x = InterpolateLinear(cameraState->rotation.x, -45.0f, cameraState->transitionFrame - 90, 90.0f);
        Hu3DData[minigameMissStageState.object->mdlId[5]].rot.x = InterpolateLinear(0.0f, 45.0f, cameraState->transitionFrame - 90, 90.0f);
    }
}
