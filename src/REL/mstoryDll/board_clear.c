#include "REL/mstoryDll.h"
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/printfunc.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/minigame_seq.h"
#endif

#include "game/window.h"
#include "game/wipe.h"

typedef struct {
    /* 0x00 */ s32 boardId;
    /* 0x04 */ s32 currentPlayerIndex;
    /* 0x08 */ s32 messageWindowId;
    /* 0x0C */ char unreferencedStatePadding[4];
    /* 0x10 */ s32 winningPlayerIndex;
    /* 0x14 */ s32 playerObjectCount;
    /* 0x18 */ char unreferencedResultPadding[0x1C];
    /* 0x34 */ StoryPlayerResult playerResults[4];
} BoardClearResultState; // Size 0x104

Void CreateBoardResultController(Void);
Void AnimateBoardClearStageEffect(OMOBJ* object, StoryProcessTaskState* taskState);
Void AnimateBoardClearCharacterEffect(OMOBJ* object, StoryProcessTaskState* taskState);
Void CreateBoardClearStageObject(Void);
Void CaptureBoardClearPlayerResults(s32 playerIndex);
Void MoveWinnerToBoardPosition(OMOBJ* object, StoryProcessTaskState* taskState);
Void WaitForWinnerMotion(OMOBJ* object, StoryProcessTaskState* taskState);
Void CreateBoardClearPlayer(Void);
Void MoveWinnerAwayFromBoard(OMOBJ* object, StoryProcessTaskState* taskState);
Void CreateBoardClearBoardObject(Void);
Void SetBoardClearCameraStart(Void);
Void MoveBoardClearCameraToTarget(Void);
Void TransitionBoardClearCameraToNextScene(Void);

BoardClearResultState resultState;
StoryProcessTaskState boardClearControllerState;
StoryProcessTaskState boardClearStageState;
StoryProcessTaskState boardClearPlayerStates[4];
StoryProcessTaskState boardClearBoardState;
HUPROCESS* boardClearObjectManager;

s32 boardClearMessageIds[][4] = {
    { MAKE_MESSID(30, 17), MAKE_MESSID(30, 23), MAKE_MESSID(30, 62), MAKE_MESSID(30, 63) },
    { MAKE_MESSID(30, 18), MAKE_MESSID(30, 24), MAKE_MESSID(30, 64), MAKE_MESSID(30, 65) },
    { MAKE_MESSID(30, 19), MAKE_MESSID(30, 25), MAKE_MESSID(30, 66), MAKE_MESSID(30, 67) },
    { MAKE_MESSID(30, 20), MAKE_MESSID(30, 26), MAKE_MESSID(30, 68), MAKE_MESSID(30, 69) },
    { MAKE_MESSID(30, 21), MAKE_MESSID(30, 27), MAKE_MESSID(30, 70), MAKE_MESSID(30, 71) }
};

s32 boardResultMessageIds[][3] = {
    { MAKE_MESSID(30, 101), MAKE_MESSID(30, 102), MAKE_MESSID(30, 103) },
    { MAKE_MESSID(30, 104), MAKE_MESSID(30, 105), MAKE_MESSID(30, 106) },
    { MAKE_MESSID(30, 107), MAKE_MESSID(30, 108), MAKE_MESSID(30, 109) },
    { MAKE_MESSID(30, 110), MAKE_MESSID(30, 111), MAKE_MESSID(30, 112) },
    { MAKE_MESSID(30, 113), MAKE_MESSID(30, 114), MAKE_MESSID(30, 115) }
};

Void PlayWinningCharacterVoice(s32 characterId, StoryProcessTaskState* taskState) {
    WaitFrames(10);
    SetObjectMotion(boardClearPlayerStates[resultState.currentPlayerIndex].object, 1, 5, 15, 0);
    WaitFrames(5);
    CharFXPlay(resultState.playerResults[resultState.currentPlayerIndex].characterId, 0x128);
    WaitFrames(25);
    taskState->callback = 0;
}

Void AnimateWinningCharacterJump(s32 characterId, StoryProcessTaskState* taskState) {
    float characterJumpHeight;
    float characterRotation;
    float initialCharacterHeight;
    s32 i;

    initialCharacterHeight = Hu3DData[boardClearStageState.object->mdlId[7]].pos.y;
    for (i = 0; i <= 30; i++) {
        WaitOneFrame();
        characterJumpHeight = InterpolateCosineEaseOut(initialCharacterHeight, 1000.0f, i, 30.0f);
        characterRotation = InterpolateCosineEaseOut(0.0f, 1800.0f, i, 30.0f);
        Hu3DModelPosSet(boardClearStageState.object->mdlId[7], 0.0f, characterJumpHeight, 800.0f);
        Hu3DModelRotSet(boardClearStageState.object->mdlId[7], 0.0f, characterRotation, 0.0f);
        Hu3DModelPosSet(boardClearStageState.object->mdlId[8], 0.0f, characterJumpHeight + 15.0f, 775.0f);
    }
    taskState->callback = 0;
}

Void RunBoardClearVictorySequence(s32 boardId) {
    float characterHeight;
    float characterRotation;
    s32 boardResultEvent;
    s32 i;

    boardResultEvent = boardId;
    Hu3DModelAttrReset(boardClearStageState.object->mdlId[5], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(boardClearStageState.object->mdlId[6], HU3D_ATTR_DISPOFF);
    for (i = 0; i < 4; i++) {
        if (resultState.playerResults[i].playerIndex != resultState.playerResults[resultState.currentPlayerIndex].playerIndex) {
            Hu3DModelAttrSet(boardClearPlayerStates[i].object->mdlId[1], HU3D_ATTR_DISPOFF);
        }
        Hu3DModelAttrSet(boardClearPlayerStates[i].object->mdlId[2], HU3D_ATTR_DISPOFF);
    }
    resultState.messageWindowId = CreateCenteredStoryWindow(1, 0, 21, 2, resultState.winningPlayerIndex);
    SetObjectMotion(boardClearBoardState.object, 1, 3, 15, 1);
    SetStoryMessage(resultState.messageWindowId, boardResultMessageIds[boardResultEvent][0]);
    WaitForStoryMessage(resultState.messageWindowId, -1);
    SetStoryMessage(resultState.messageWindowId, MAKE_MESSID(37, 4));
    SetObjectMotion(boardClearBoardState.object, 1, 1, 15, 1);
    CloseCenteredStoryWindow(resultState.messageWindowId);
    Hu3DModelPosSet(boardClearStageState.object->mdlId[7], 0.0f, 1000.0f, 640.0f);
    Hu3DModelAttrReset(boardClearStageState.object->mdlId[7], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(boardClearStageState.object->mdlId[8], HU3D_ATTR_DISPOFF);
    for (i = 0; i <= 180; i++) {
        WaitOneFrame();
        characterHeight = InterpolateSineEaseOut(1000.0f, 150.0f, i, 180.0f);
        characterRotation = InterpolateSineEaseOut(1800.0f, 0.0f, i, 180.0f);
        Hu3DModelPosSet(boardClearStageState.object->mdlId[7], 0.0f, characterHeight, 800.0f);
        Hu3DModelRotSet(boardClearStageState.object->mdlId[7], 0.0f, characterRotation, 0.0f);
        Hu3DModelPosSet(boardClearStageState.object->mdlId[8], 0.0f, characterHeight + 15.0f, 775.0f);
    }
    boardClearStageState.callback = (Void*) AnimateBoardClearCharacterEffect;
    SetObjectMotion(boardClearPlayerStates[resultState.currentPlayerIndex].object, 1, 6, 15, 0);
    boardClearPlayerStates[resultState.currentPlayerIndex].callback = (Void*) WaitForWinnerMotion;
    WaitFrames(40);
    HuAudSeqFadeOut(storySequenceIds[0], 1000);
    WaitFrames(50);
    storySequenceIds[1] = HuAudSeqPlay(0x35);
    resultState.messageWindowId = CreateCenteredStoryWindow(1, 0, 21, 2, resultState.winningPlayerIndex);
    SetObjectMotion(boardClearBoardState.object, 1, 3, 15, 1);
    SetStoryMessage(resultState.messageWindowId, boardResultMessageIds[boardResultEvent][1]);
    WaitForStoryMessage(resultState.messageWindowId, -1);
    SetStoryMessage(resultState.messageWindowId, MAKE_MESSID(37, 4));
    SetObjectMotion(boardClearBoardState.object, 1, 1, 15, 1);
    CloseCenteredStoryWindow(resultState.messageWindowId);
    WaitFrames(30);
    SetObjectMotion(boardClearStageState.object, 2, 3, 0, 0);
    boardClearStageState.callback = (Void*) AnimateBoardClearStageEffect;
    WaitFrames(30);
    boardClearPlayerStates[resultState.currentPlayerIndex].callback = (Void*) PlayWinningCharacterVoice;
    WaitFrames(90);
    boardClearBoardState.callback = (Void*) AnimateWinningCharacterJump;
    resultState.messageWindowId = CreateCenteredStoryWindow(1, 0, 21, 2, resultState.winningPlayerIndex);
    SetObjectMotion(boardClearBoardState.object, 1, 5, 15, 1);
    SetStoryMessage(resultState.messageWindowId, boardResultMessageIds[resultState.boardId][2]);
    WaitForStoryMessage(resultState.messageWindowId, -1);
    SetStoryMessage(resultState.messageWindowId, MAKE_MESSID(37, 4));
    CloseCenteredStoryWindow(resultState.messageWindowId);
    HuAudFXPlay(storyEventSoundEffectIds[resultState.boardId][3]);
}

Void StartBoardClearResult(HUPROCESS* resultProcess, s32 boardId, s32 winningPlayerIndex) {
    s32 i;

    boardClearObjectManager = resultProcess;
    resultState.boardId = boardId;
    resultState.winningPlayerIndex = winningPlayerIndex;
    InitializeStoryWindows();
    CreateStoryLighting(0xFF);
    InitializeStoryCamera(boardClearObjectManager, SetBoardClearCameraStart);
    CreateStoryShadow();
    CaptureBoardClearPlayerResults(winningPlayerIndex);
    HuPrcChildCreate(CreateBoardClearStageObject, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(CreateBoardClearBoardObject, 100, 0x2000, 0, HuPrcCurrentGet());
    for (i = 0; i < 4; i++) {
        HuPrcChildCreate(CreateBoardClearPlayer, 100, 0x2000, 0, HuPrcCurrentGet());
    }
    HuPrcChildCreate(CreateBoardResultController, 100, 0x2000, 0, HuPrcCurrentGet());
}

Void DisplayBoardClearDebugPanel(Void) {
    s32 debugWindowX = 16;
    s32 debugWindowY = 40;
    GXColor debugPanelColor = { 0x00, 0x00, 0x80, 0x80 };
    s32 i;

    while (TRUE) {
        WaitOneFrame();
        fontcolor = FONT_COLOR_WHITE;
        printWin(debugWindowX, debugWindowY, 220, 150, &debugPanelColor);
        if (boardClearControllerState.callback == NULL) {
            print8(debugWindowX, debugWindowY, 1.0f, "PROC_MAIN    -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY, 1.0f, "PROC_MAIN    -> TRUE");
        }
        if (boardClearStageState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 10, 1.0f, "PROC_MAP     -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 10, 1.0f, "PROC_MAP     -> TRUE");
        }
        for (i = 0; i < 4; i++) {
            if (boardClearPlayerStates[i].callback == NULL) {
                print8(debugWindowX, i * 10 + (debugWindowY + 20), 1.0f, "PROC_PLAYER%d -> FALSE", i);
            } else {
                print8(debugWindowX, i * 10 + (debugWindowY + 20), 1.0f, "PROC_PLAYER%d -> TRUE", i);
            }
        }
        if (boardClearBoardState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 60, 1.0f, "PROC_SUB     -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 60, 1.0f, "PROC_SUB     -> TRUE");
        }
        print8(debugWindowX, debugWindowY + 80, 1.0f, "M:%d P:%d W:%d", resultState.boardId, resultState.currentPlayerIndex, resultState.winningPlayerIndex);
        print8(debugWindowX, debugWindowY + 100, 1.0f, "NO) IDX:GRP:COM:DIF:PAD:CHR");
        for (i = 0; i < 4; i++) {
            print8(debugWindowX, debugWindowY + 110 + i * 10, 1.0f, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", i,
                resultState.playerResults[i].playerIndex, resultState.playerResults[i].group, resultState.playerResults[i].isComputer,
                resultState.playerResults[i].difficulty, resultState.playerResults[i].controllerPort, resultState.playerResults[i].characterId);
        }
    }
}

Void PlayBoardClearIntroMusic(Void) {
    storySequenceIds[0] = HuAudSeqPlay(0x33);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet() != 0);
    WaitFrames(60);
}

Void ShowBoardClearIntroduction(Void) {
    s32 unreferencedWindowState = 0;
    s32 unreferencedWindowResult = 0;

    resultState.messageWindowId = CreateCenteredStoryWindow(0, 0, 21, 2, resultState.winningPlayerIndex);
    SetObjectMotion(boardClearBoardState.object, 1, 3, 15, 1);
    InsertStoryMessage(resultState.messageWindowId, resultState.playerResults[resultState.currentPlayerIndex].characterId, 0);
    SetStoryMessage(resultState.messageWindowId, boardClearMessageIds[resultState.boardId][0]);
    WaitForStoryMessage(resultState.messageWindowId, -1);
    SetStoryMessage(resultState.messageWindowId, MAKE_MESSID(37, 4));
    SetObjectMotion(boardClearBoardState.object, 1, 1, 15, 1);
    CloseCenteredStoryWindow(resultState.messageWindowId);
    boardClearPlayerStates[resultState.currentPlayerIndex].callback = (Void*) MoveWinnerToBoardPosition;
    CaptureCameraStartState(&storyCameraState);
    SetStoryCameraCallback(MoveBoardClearCameraToTarget);
    do {
        WaitOneFrame();
    } while (boardClearPlayerStates[resultState.currentPlayerIndex].callback != NULL);
    do {
        WaitOneFrame();
    } while (boardClearBoardState.callback != NULL);
}

Void ShowBoardClearVictoryMessage(Void) {
    resultState.messageWindowId = CreateCenteredStoryWindow(1, 0, 21, 2, resultState.winningPlayerIndex);
    SetObjectMotion(boardClearBoardState.object, 1, 3, 15, 1);
    SetStoryMessage(resultState.messageWindowId, boardClearMessageIds[resultState.boardId][2]);
    WaitForStoryMessage(resultState.messageWindowId, -1);
    SetObjectMotion(boardClearBoardState.object, 1, 1, 15, 1);
    CloseCenteredStoryWindow(resultState.messageWindowId);
    SetObjectMotion(boardClearPlayerStates[resultState.currentPlayerIndex].object, 1, 6, 15, 0);
    boardClearPlayerStates[resultState.currentPlayerIndex].callback = (Void*) WaitForWinnerMotion;
    Hu3DModelAttrReset(boardClearStageState.object->mdlId[5], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(boardClearStageState.object->mdlId[6], HU3D_ATTR_DISPOFF);
    WaitFrames(180);
    HuAudSeqFadeOut(storySequenceIds[0], 1000);
    SetObjectMotion(boardClearStageState.object, 2, 3, 0, 0);
    WaitFrames(75);
    storySequenceIds[1] = HuAudSeqPlay(0x35);
    resultState.messageWindowId = CreateCenteredStoryWindow(1, 0, 21, 2, resultState.winningPlayerIndex);
    SetObjectMotion(boardClearBoardState.object, 1, 4, 15, 1);
    SetStoryMessage(resultState.messageWindowId, boardClearMessageIds[resultState.boardId][3]);
    boardClearPlayerStates[resultState.currentPlayerIndex].callback = NULL;
    boardClearPlayerStates[resultState.currentPlayerIndex].callback = (Void*) PlayWinningCharacterVoice;
    WaitForStoryMessage(resultState.messageWindowId, -1);
    SetStoryMessage(resultState.messageWindowId, MAKE_MESSID(37, 4));
    HuAudFXPlay(storyEventSoundEffectIds[resultState.boardId][3]);
    CloseCenteredStoryWindow(resultState.messageWindowId);
    do {
        WaitOneFrame();
    } while (boardClearPlayerStates[resultState.currentPlayerIndex].callback != NULL);
}

Void PrepareBoardClearMinigameTransition(Void) {
    WaitFrames(60);
    HuAudSeqFadeOut(storySequenceIds[1], 3000);
    CaptureCameraStartState(&storyCameraState);
    SetStoryCameraCallback(TransitionBoardClearCameraToNextScene);
    WaitFrames(150);
}

Void LaunchBoardMinigame(Void) {
    OMOVLHIS* overlayHistory;
    u16 resultOverlayIds[] = { 53, 56, 55, 54, 57 };
    s32 boardCharacterIds[] = { 9, 14, 10, 11, 12 };
    s32 playerCharacterId;
    s32 playerControllerPort;
    s32 i;

    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet() != 0);
    CharModelKill(-1);
    MGSeqKillAll();
    playerCharacterId = resultState.playerResults[resultState.currentPlayerIndex].characterId;
    playerControllerPort = resultState.playerResults[resultState.currentPlayerIndex].controllerPort;
    GWPlayerCfg[resultState.currentPlayerIndex].character = GWPlayerCfg->character;
    GWPlayerCfg[resultState.currentPlayerIndex].pad_idx = GWPlayerCfg->pad_idx;
    GWPlayerCfg[resultState.currentPlayerIndex].group = GWPlayerCfg->group;
    GWPlayerCfg[resultState.currentPlayerIndex].iscom = GWPlayerCfg->iscom;
    GWPlayerCfg->character = playerCharacterId;
    GWPlayerCfg->pad_idx = playerControllerPort;
    GWPlayerCfg->group = 0;
    GWPlayerCfg->iscom = 0;
    for (i = 0; i < 4; i++) {
        GWPlayer[i].character = GWPlayerCfg[i].character;
        GWPlayer[i].port = GWPlayerCfg[i].pad_idx;
        GWPlayer[i].com = GWPlayerCfg[i].iscom;
        GWPlayer[i].team = GWPlayerCfg[i].group;
    }
    GWPlayerCfg[1].character = boardCharacterIds[GWSystem.board];
    for (i = 1; i < 4; i++) {
        GWPlayerCfg[i].iscom = GWPlayerCfg[i].group = 1;
    }
    overlayHistory = omOvlHisGet(0);
    omOvlHisChg(0, DLL_mstorydll, 2, overlayHistory->stat);
    OSReport("########## dllno - %d\n", resultOverlayIds[GWSystem.board], GWSystem.board);
    HuAudSeqAllFadeOut(1000);
    HuAudSStreamAllFadeOut(1000);
    omOvlCallEx(resultOverlayIds[GWSystem.board], 1, 0, 0);
    while (TRUE) {
        WaitOneFrame();
    }
}

Void RunBoardClearResultSequence(s32 boardId, StoryProcessTaskState* taskState) {
    PlayBoardClearIntroMusic();
    ShowBoardClearIntroduction();
    RunBoardClearVictorySequence(resultState.boardId);
    PrepareBoardClearMinigameTransition();
    LaunchBoardMinigame();
    taskState->callback = NULL;
}

Void CreateBoardResultController(Void) {
    OMOBJ* controllerObject;

    controllerObject = omAddObjEx(boardClearObjectManager, 0x100, 16, 16, -1, NULL);
    boardClearControllerState.object = controllerObject;
    boardClearControllerState.callback = (Void*) RunBoardClearResultSequence;
    while (TRUE) {
        WaitOneFrame();
        if (boardClearControllerState.callback != NULL) {
            boardClearControllerState.callback(controllerObject, &boardClearControllerState);
        }
    }
}

Void LoadBoardClearStageModels(OMOBJ* object) {
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 56));
    object->mtnId[0] = Hu3DMotionIDGet(object->mdlId[1]);
    SetObjectMotion(object, 1, 0, 0, 2);
    Hu3DModelShadowMapObjSet(object->mdlId[1], "base_fix9-base");
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 59));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY, 60));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY, 61));
    SetObjectMotion(object, 2, 2, 0, 2);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 57));
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 76 + resultState.boardId));
    object->mtnId[5] = Hu3DMotionIDGet(object->mdlId[5]);
    Hu3DModelPosSet(object->mdlId[5], 0.0f, 80.0f, 460.0f);
    Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowSet(object->mdlId[5]);
    SetObjectMotion(object, 5, 5, 0, 2);
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 75));
    object->mtnId[6] = Hu3DMotionIDGet(object->mdlId[6]);
    Hu3DModelHookSet(object->mdlId[5], "stargbox_fix-effect_fook1", object->mdlId[6]);
    Hu3DModelAttrSet(object->mdlId[6], HU3D_ATTR_DISPOFF);
    SetObjectMotion(object, 6, 6, 0, 2);
    object->mdlId[7] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 81));
    Hu3DModelAttrSet(object->mdlId[7], HU3D_ATTR_DISPOFF);
    object->mdlId[8] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 96));
    object->mtnId[8] = Hu3DMotionIDGet(object->mdlId[8]);
    Hu3DModelAttrSet(object->mdlId[8], HU3D_ATTR_DISPOFF);
    SetObjectMotion(object, 8, 8, 0, 1);
}

Void AnimateBoardClearStageEffect(OMOBJ* object, StoryProcessTaskState* taskState) {
    Hu3DData[object->mdlId[5]].pos.y = InterpolateSineCycle(80.0f, 90.0f, object->work[0]++, 360.0f);
    if (object->work[0] >= 360) {
        object->work[0] = 0;
    }
}

Void AnimateBoardClearCharacterEffect(OMOBJ* object, StoryProcessTaskState* taskState) {
    Hu3DData[object->mdlId[7]].pos.y += InterpolateSineCycle(0.0f, 0.25f, object->work[0]++, 360.0f);
    Hu3DData[object->mdlId[8]].pos.y = Hu3DData[object->mdlId[7]].pos.y + 15.0f;
    if (object->work[0] >= 360) {
        object->work[0] = 0;
    }
}

Void CreateBoardClearStageObject(Void) {
    OMOBJ* stageObject;

    stageObject = omAddObjEx(boardClearObjectManager, 0x100, 16, 16, -1, NULL);
    boardClearStageState.object = stageObject;
    boardClearStageState.callback = (Void*) AnimateBoardClearStageEffect;
    boardClearStageState.object->data = &boardClearStageState;
    LoadBoardClearStageModels(stageObject);
    while (TRUE) {
        WaitOneFrame();
        if (boardClearStageState.callback != NULL) {
            boardClearStageState.callback(stageObject, &boardClearStageState);
        }
    }
}

Void CaptureBoardClearPlayerResults(s32 playerIndex) {
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
    for (i = 0; i < 4; i++) {
        resultState.playerResults[i].resultValue = GWPlayer[playerOrder[i]].rank + 1;
    }
}

Vec boardClearPlayerPositions[] = {
    { -300.0f,   0.0f, 460.0f },
    { -100.0f, -30.0f, 460.0f },
    {  100.0f, -60.0f, 460.0f },
    {  300.0f, -90.0f, 460.0f }
};

Void CreateBoardClearPlayerObject(OMOBJ* object, s32 playerOrderIndex) {
    s32 playerIndex;
    s32 playerCharacterId;

    playerIndex = playerOrderIndex;
    playerCharacterId = resultState.playerResults[playerIndex].characterId;
    object->mdlId[1] = CharModelCreate(playerCharacterId, 4);
    object->mtnId[1] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0));
    object->mtnId[2] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 3));
    object->mtnId[3] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 5));
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY, 48 + playerCharacterId));
    object->mtnId[5] = CharMotionCreate(playerCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 109));
    object->mtnId[6] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY, 40 + playerCharacterId));
    CharMotionDataClose(playerCharacterId);
    Hu3DModelPosSet(object->mdlId[1], boardClearPlayerPositions[playerIndex].x, boardClearPlayerPositions[resultState.playerResults[playerIndex].resultValue - 1].y + 90.0f, boardClearPlayerPositions[playerIndex].z - 15.0f);
    Hu3DModelRotSet(object->mdlId[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(object->mdlId[1], 1.0f, 1.0f, 1.0f);
    Hu3DModelShadowSet(object->mdlId[1]);
    SetObjectMotion(object, 1, 1, 0, 1);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 62));
    Hu3DModelPosSet(object->mdlId[2], boardClearPlayerPositions[playerIndex].x, boardClearPlayerPositions[resultState.playerResults[playerIndex].resultValue - 1].y, boardClearPlayerPositions[playerIndex].z);
    if (resultState.playerResults[playerIndex].resultValue == 4) {
        Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    }
    Hu3DModelShadowMapSet(object->mdlId[2]);
    Hu3DModelScaleSet(object->mdlId[2], 1.5f, 1.0f, 1.5f);
}

Void MoveWinnerToBoardPosition(OMOBJ* object, StoryProcessTaskState* taskState) {
    Vec targetPosition = { 120.0f, 0.0f, 670.0f };

    AnimateCharacterPosition(object, 1, targetPosition, -1.0f, 30.0f, 10.0f);
    boardClearBoardState.callback = (Void*) MoveWinnerAwayFromBoard;
    SetObjectMotion(boardClearStageState.object, 2, 2, 0, 0);
    SetObjectMotion(object, 1, 2, 15, 1);
    AnimateCharacterRotation(object, 1, targetPosition, -45.0f, 30.0f, 10.0f);
    SetObjectMotion(object, 1, 1, 15, 1);
    WaitForObjectMotionEnd(boardClearStageState.object, 2, 2, 0);
    taskState->callback = 0;
}

Void WaitForWinnerMotion(OMOBJ* object, StoryProcessTaskState* taskState) {
    if (object->mtnId[6] == Hu3DMotionIDGet(object->mdlId[1]) && Hu3DMotionEndCheck(object->mdlId[1])) {
        SetObjectMotion(object, 1, 4, 15, 1);
        taskState->callback = 0;
    }
}

Void CreateBoardClearPlayer(Void) {
    OMOBJ* playerObject;
    s32 playerIndex;

    playerIndex = resultState.playerObjectCount++;
    playerObject = omAddObjEx(boardClearObjectManager, 0x100, 16, 16, -1, NULL);
    boardClearPlayerStates[playerIndex].object = playerObject;
    boardClearPlayerStates[playerIndex].callback = NULL;
    boardClearPlayerStates[playerIndex].object->data = &boardClearPlayerStates[playerIndex];
    CreateBoardClearPlayerObject(playerObject, playerIndex);
    while (TRUE) {
        WaitOneFrame();
        if (boardClearPlayerStates[playerIndex].callback != NULL) {
            boardClearPlayerStates[playerIndex].callback(playerObject, &boardClearPlayerStates[playerIndex]);
        }
    }
}

Void LoadBoardClearCharacter(OMOBJ* object, s32 characterIndex) {
    s32 characterModelIds[] = {
        DATA_MAKE_NUM(DATADIR_MSTORY, 97),
        DATA_MAKE_NUM(DATADIR_MSTORY, 108),
        DATA_MAKE_NUM(DATADIR_MSTORY, 116),
        DATA_MAKE_NUM(DATADIR_MSTORY, 124),
        DATA_MAKE_NUM(DATADIR_MSTORY, 131)
    };
    s32 characterMotionIdsByBoard[][5] = {
        { DATA_MAKE_NUM(DATADIR_MSTORY, 98), DATA_MAKE_NUM(DATADIR_MSTORY, 100), DATA_MAKE_NUM(DATADIR_MSTORY, 101), DATA_MAKE_NUM(DATADIR_MSTORY, 103), DATA_MAKE_NUM(DATADIR_MSTORY, 104) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 109), DATA_MAKE_NUM(DATADIR_MSTORY, 111), DATA_MAKE_NUM(DATADIR_MSTORY, 112), DATA_MAKE_NUM(DATADIR_MSTORY, 114), DATA_MAKE_NUM(DATADIR_MSTORY, 115) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 117), DATA_MAKE_NUM(DATADIR_MSTORY, 119), DATA_MAKE_NUM(DATADIR_MSTORY, 120), DATA_MAKE_NUM(DATADIR_MSTORY, 122), DATA_MAKE_NUM(DATADIR_MSTORY, 123) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 125), DATA_MAKE_NUM(DATADIR_MSTORY, 126), DATA_MAKE_NUM(DATADIR_MSTORY, 127), DATA_MAKE_NUM(DATADIR_MSTORY, 129), DATA_MAKE_NUM(DATADIR_MSTORY, 130) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 132), DATA_MAKE_NUM(DATADIR_MSTORY, 134), DATA_MAKE_NUM(DATADIR_MSTORY, 135), DATA_MAKE_NUM(DATADIR_MSTORY, 137), DATA_MAKE_NUM(DATADIR_MSTORY, 138) }
    };

    object->mdlId[1] = Hu3DModelCreateFile(characterModelIds[characterIndex]);
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][0]);
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][1]);
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][2]);
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][3]);
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], characterMotionIdsByBoard[characterIndex][4]);
    Hu3DModelPosSet(object->mdlId[1], -300.0f, 0.0f, 890.0f);
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

Void MoveWinnerAwayFromBoard(OMOBJ* object, StoryProcessTaskState* taskState) {
    Vec targetPosition = { -120.0f, 0.0f, 670.0f };

    SetObjectMotion(object, 1, 2, 15, 1);
    AnimateCharacterRotation(object, 1, targetPosition, 45.0f, 60.0f, 10.0f);
    SetObjectMotion(object, 1, 1, 15, 1);
    taskState->callback = 0;
}

Void CreateBoardClearBoardObject(Void) {
    OMOBJ* boardObject;
    s32 boardId;

    boardObject = omAddObjEx(boardClearObjectManager, 0x100, 16, 16, -1, NULL);
    boardClearBoardState.object = boardObject;
    boardClearBoardState.callback = NULL;
    boardClearBoardState.object->data = &boardClearBoardState;
    boardId = resultState.boardId;
    LoadBoardClearCharacter(boardObject, boardId);
    while (TRUE) {
        WaitOneFrame();
        if (boardClearBoardState.callback != NULL) {
            boardClearBoardState.callback(boardObject, &boardClearBoardState);
        }
    }
}

Void SetBoardClearCameraStart(Void) {
    StoryCameraTransitionState* cameraState;

    cameraState = &storyCameraState;
    cameraState->centerPosition.x = 0.0f;
    cameraState->centerPosition.y = 170.0f;
    cameraState->centerPosition.z = 0.0f;
    cameraState->rotation.x = -5.0f;
    cameraState->rotation.y = 0.0f;
    cameraState->rotation.z = 0.0f;
    cameraState->zoom = 1700.0f;
}

Void MoveBoardClearCameraToTarget(Void) {
    StoryCameraTransitionState* cameraState;
    StoryCameraTransitionState cameraTarget;

    cameraState = &storyCameraState;
    cameraTarget.centerPosition.x = 0.0f;
    cameraTarget.centerPosition.y = 125.0f;
    cameraTarget.centerPosition.z = 0.0f;
    cameraTarget.rotation.x = -5.0f;
    cameraTarget.rotation.y = 0.0f;
    cameraTarget.rotation.z = 0.0f;
    cameraTarget.zoom = 1750.0f;
    InterpolateCameraTransition(cameraState, &cameraTarget, cameraState->transitionFrame++, 60.0f, 30.0f);
}

Void TransitionBoardClearCameraToNextScene(Void) {
    StoryCameraTransitionState* cameraState;
    StoryCameraTransitionState cameraTarget;

    cameraState = &storyCameraState;
    if (cameraState->transitionFrame == 50) {
        SetObjectMotion(boardClearStageState.object, 5, 5, 0, 0);
        SetObjectMotion(boardClearStageState.object, 6, 6, 0, 0);
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
        Hu3DData[boardClearStageState.object->mdlId[5]].rot.x = InterpolateLinear(0.0f, 45.0f, cameraState->transitionFrame - 90, 90.0f);
    }
}
