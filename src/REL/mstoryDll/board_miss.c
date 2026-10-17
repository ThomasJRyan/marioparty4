#include "REL/mstoryDll.h"
#include "game/armem.h"
#include "game/audio.h"
#include "game/board/main.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

extern s32 rand8(Void);

typedef struct {
    /* 0x000 */ s32 boardId;
    /* 0x004 */ s32 currentPlayerIndex;
    /* 0x008 */ s32 messageWindowId;
    /* 0x00C */ s32 choiceWindowId;
    /* 0x010 */ s32 winningPlayerIndex;
    /* 0x014 */ s32 playerObjectCount;
    /* 0x018 */ char unreferencedStatePadding[0x1C];
    /* 0x034 */ StoryPlayerResult playerResults[4];
    /* 0x104 */ float boardOffsetY;
} BoardMissResultState; // Size 0x108

typedef struct {
    /* 0x00 */ s32 playerIndex;
    /* 0x04 */ s32 unreferencedPlayerState;
    /* 0x08 */ s32 isComputer;
    /* 0x0C */ char unreferencedPlayerPadding[4];
    /* 0x10 */ s32 characterId;
    /* 0x14 */ s32 controllerPort;
} StoryPlayerResultSettings; // Size 0x18

Void CreateBoardMissController(Void);
Void CreateBoardMissStage(Void);
Void SetBoardMissCharacterMotion(s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 loopMode, s32 initialDelayFrames);
Void MoveWinnerToMissPosition(OMOBJ* object, StoryProcessTaskState* taskState);
Void CreateBoardMissCharacterObject(Void);
Void CaptureBoardMissPlayerResults(s32 playerIndex);
Void AnimateFallingPlayer(OMOBJ* object, StoryProcessTaskState* taskState);
Void MoveWinnerOffStage(OMOBJ* object, StoryProcessTaskState* taskState);
Void MovePlayerOffStage(OMOBJ* object, StoryProcessTaskState* taskState);
Void CreateBoardMissPlayerObject(Void);
Void SetBoardMissCameraStart(Void);
Void MoveBoardMissCameraToCharacter(Void);
Void TransitionBoardMissCameraToEnd(Void);
Void ResetBoardMissCamera(Void);

BoardMissResultState boardMissState;
StoryProcessTaskState boardMissControllerState;
StoryProcessTaskState boardMissStageState;
StoryProcessTaskState boardMissCharacterState;
StoryProcessTaskState boardMissPlayerStates[4];
ANIMDATA* boardMissPlayerAnimations[2];
StoryPlayerResultSettings boardMissPlayerConfigs[4];
s32 boardDataLoadComplete;
HUPROCESS* boardMissObjectManager;

s32 boardMissMessageIds[][4] = {
    { MAKE_MESSID(30, 28), MAKE_MESSID(30, 33), MAKE_MESSID(30, 34), MAKE_MESSID(30, 5) },
    { MAKE_MESSID(30, 29), MAKE_MESSID(30, 33), MAKE_MESSID(30, 34), MAKE_MESSID(30, 5) },
    { MAKE_MESSID(30, 30), MAKE_MESSID(30, 33), MAKE_MESSID(30, 34), MAKE_MESSID(30, 5) },
    { MAKE_MESSID(30, 31), MAKE_MESSID(30, 33), MAKE_MESSID(30, 34), MAKE_MESSID(30, 5) },
    { MAKE_MESSID(30, 32), MAKE_MESSID(30, 33), MAKE_MESSID(30, 34), MAKE_MESSID(30, 5) }
};

s32 boardDataDirectoryIds[] = {
    DATADIR_W01,
    DATADIR_W02,
    DATADIR_W03,
    DATADIR_W04,
    DATADIR_W05,
    DATADIR_W06,
    DATADIR_W10,
    DATADIR_MARIOMDL1,
    DATADIR_LUIGIMDL1,
    DATADIR_PEACHMDL1,
    DATADIR_YOSHIMDL1,
    DATADIR_WARIOMDL1,
    DATADIR_DONKEYMDL1,
    DATADIR_DAISYMDL1,
    DATADIR_WALUIGIMDL1
};

Void LoadBoardDataAsync(Void) {
    s32 asyncReadHandle;

    asyncReadHandle = HuDataDirReadAsync(DATADIR_BOARD);
    if (asyncReadHandle != -1) {
        while (HuDataGetAsyncStat(asyncReadHandle) == 0) {
            HuPrcVSleep();
        }
    }
    HuAR_MRAMtoARAM(DATADIR_BOARD);
    while (HuARDMACheck()) {
        HuPrcVSleep();
    }
    HuDataDirClose(DATADIR_BOARD);
    asyncReadHandle = HuDataDirReadAsync(boardDataDirectoryIds[GWSystem.board]);
    if (asyncReadHandle != -1) {
        while (HuDataGetAsyncStat(asyncReadHandle) == 0) {
            HuPrcVSleep();
        }
    }
    boardDataLoadComplete = 1;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

Void PrepareBoardMissPlayers(Void) {
    s32 i;
    s32 j;

    boardMissPlayerConfigs[0].playerIndex = 0;
    boardMissPlayerConfigs[0].unreferencedPlayerState = 0;
    boardMissPlayerConfigs[0].characterId = boardMissState.playerResults[boardMissState.currentPlayerIndex].characterId;
    boardMissPlayerConfigs[0].controllerPort = boardMissState.playerResults[boardMissState.currentPlayerIndex].controllerPort;
    boardMissPlayerConfigs[0].isComputer = 0;
    for (i = 1; i < 4; i++) {
        boardMissPlayerConfigs[i].characterId = -1;
        boardMissPlayerConfigs[i].controllerPort = i;
        boardMissPlayerConfigs[i].isComputer = 1;
    }
    for (i = 1; i < 4; i++) {
        do {
            boardMissPlayerConfigs[i].characterId = rand8() % 8;
            for (j = 0; j < 4; j++) {
                if (i != j && boardMissPlayerConfigs[j].characterId != -1 && boardMissPlayerConfigs[i].characterId == boardMissPlayerConfigs[j].characterId) {
                    break;
                }
            }
        } while (j != 4);
    }
    for (i = 0; i < 4; i++) {
        GWPlayerCfg[i].character = boardMissPlayerConfigs[i].characterId;
        GWPlayerCfg[i].pad_idx = boardMissPlayerConfigs[i].controllerPort;
        GWPlayerCfg[i].group = 0;
        GWPlayerCfg[i].iscom = boardMissPlayerConfigs[i].isComputer;
    }
    BoardSaveInit(GWSystem.board);
    CharDataClose(-1);
    HuDataDirClose(DATADIR_MSTORY);
    HuPrcChildCreate(LoadBoardDataAsync, 100, 0x3000, 0, boardMissObjectManager);
}

Void LaunchBoardMinigameAfterMiss(Void) {
    OMOVLHIS* previousOverlayHistory;
    s32 boardOverlayDllIds[] = { DLL_w01dll, DLL_w02dll, DLL_w03dll, DLL_w04dll, DLL_w05dll, DLL_w06dll, DLL_w10dll };

    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        WaitOneFrame();
    }
    CharModelKill(-1);
    MGSeqKillAll();
    previousOverlayHistory = omOvlHisGet(0);
    omOvlHisChg(0, DLL_mstory3dll, 0, previousOverlayHistory->stat);
    do {
        WaitOneFrame();
    } while (boardDataLoadComplete != 1);
    CharMotionInit(GWPlayerCfg[0].character);
    CharMotionInit(GWPlayerCfg[1].character);
    CharMotionInit(GWPlayerCfg[2].character);
    CharMotionInit(GWPlayerCfg[3].character);
    HuAudSeqAllFadeOut(1000);
    HuAudSStreamAllFadeOut(1000);
    omOvlCallEx(boardOverlayDllIds[GWSystem.board], 1, 0, 0);
    while (TRUE) {
        WaitOneFrame();
    }
}

Void StartBoardMissResult(HUPROCESS* resultProcess, s32 boardId, s32 winningPlayerIndex) {
    s32 i;

    boardMissObjectManager = resultProcess;
    boardMissState.currentPlayerIndex = winningPlayerIndex;
    boardMissState.winningPlayerIndex = winningPlayerIndex;
    boardMissState.boardId = boardId;
    InitializeStoryWindows();
    CreateStoryLighting(0xFF);
    InitializeStoryCamera(boardMissObjectManager, SetBoardMissCameraStart);
    CreateStoryShadow();
    CaptureBoardMissPlayerResults(winningPlayerIndex);
    HuPrcChildCreate(CreateBoardMissStage, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(CreateBoardMissCharacterObject, 100, 0x2000, 0, HuPrcCurrentGet());
    for (i = 0; i < 4; i++) {
        HuPrcChildCreate(CreateBoardMissPlayerObject, 100, 0x2000, 0, HuPrcCurrentGet());
    }
    HuPrcChildCreate(CreateBoardMissController, 100, 0x2000, 0, HuPrcCurrentGet());
}

Void AdvanceCharacterMotion(OMOBJ* object) {
    if (object->work[2] != 0) {
        object->work[2]--;
        return;
    }
    if (object->work[2] == 0 && Hu3DMotionEndCheck(object->mdlId[1])) {
        SetObjectMotion(object, 1, object->work[1], 15, 1);
        object->work[0] = object->work[1] = object->work[2] = 0;
    }
}

Void DisplayBoardMissDebugPanel(Void) {
    s32 debugWindowX = 16;
    s32 debugWindowY = 40;
    GXColor debugPanelColor = { 0x00, 0x00, 0x80, 0x80 };
    s32 i;

    while (TRUE) {
        WaitOneFrame();
        printWin(debugWindowX, debugWindowY, 220, 160, &debugPanelColor);
        if (boardMissControllerState.callback == NULL) {
            print8(debugWindowX, debugWindowY, 1.0f, "PROC_MAIN    -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY, 1.0f, "PROC_MAIN    -> TRUE");
        }
        if (boardMissStageState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 10, 1.0f, "PROC_MAP     -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 10, 1.0f, "PROC_MAP     -> TRUE");
        }
        if (boardMissCharacterState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 30, 1.0f, "PROC_MKOOPA  -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 30, 1.0f, "PROC_MKOOPA  -> TRUE");
        }
        for (i = 0; i < 4; i++) {
            if (boardMissPlayerStates[i].callback == NULL) {
                print8(debugWindowX, i * 10 + (debugWindowY + 40), 1.0f, "PROC_PLAYER%d -> FALSE", i);
            } else {
                print8(debugWindowX, i * 10 + (debugWindowY + 40), 1.0f, "PROC_PLAYER%d -> TRUE", i);
            }
        }
        print8(debugWindowX, debugWindowY + 90, 1.0f, "P:%d W:%d", boardMissState.currentPlayerIndex, boardMissState.winningPlayerIndex);
        print8(debugWindowX, debugWindowY + 110, 1.0f, "NO) IDX:GRP:COM:DIF:PAD:CHR");
        for (i = 0; i < 4; i++) {
            print8(debugWindowX, debugWindowY + 120 + i * 10, 1.0f, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", i,
                boardMissState.playerResults[i].playerIndex, boardMissState.playerResults[i].group, boardMissState.playerResults[i].isComputer,
                boardMissState.playerResults[i].difficulty, boardMissState.playerResults[i].controllerPort, boardMissState.playerResults[i].characterId);
        }
    }
}

Void PlayBoardMissIntroMusic(Void) {
    storySequenceIds[0] = HuAudSeqPlay(0x34);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet() != 0);
    WaitFrames(60);
}

Void ShowBoardMissIntroduction(Void) {
    s32 i;

    HuAudFXPlay(storyEventSoundEffectIds[boardMissState.boardId][1]);
    boardMissState.messageWindowId = CreateCenteredStoryWindow(0, 0, 21, 2, boardMissState.winningPlayerIndex);
    SetBoardMissCharacterMotion(3, -1, 15, 1, 0);
    SetStoryMessage(boardMissState.messageWindowId, boardMissMessageIds[boardMissState.boardId][0]);
    WaitForStoryMessage(boardMissState.messageWindowId, -1);
    SetStoryMessage(boardMissState.messageWindowId, MAKE_MESSID(37, 4));
    SetBoardMissCharacterMotion(1, -1, 15, 1, 0);
    CloseCenteredStoryWindow(boardMissState.messageWindowId);
    for (i = 0; i < 4; i++) {
        boardMissPlayerStates[i].callback = (Void*) AnimateFallingPlayer;
    }
    do {
        WaitOneFrame();
        for (i = 0; i < 4; i++) {
            if (boardMissPlayerStates[i].callback != NULL) {
                break;
            }
        }
    } while (i != 4);
    boardMissPlayerStates[boardMissState.currentPlayerIndex].callback = (Void*) MoveWinnerOffStage;
    CaptureCameraStartState(&storyCameraState);
    SetStoryCameraCallback(MoveBoardMissCameraToCharacter);
    while (TRUE) {
        WaitOneFrame();
        if (boardMissPlayerStates[boardMissState.currentPlayerIndex].callback == NULL) {
            break;
        }
    }
    while (TRUE) {
        WaitOneFrame();
        if (boardMissCharacterState.callback == NULL) {
            break;
        }
    }
    WaitFrames(30);
}

BOOL PromptContinueStory(Void) {
    BOOL choiceAccepted = FALSE;

    SetStoryMessage(boardMissState.messageWindowId, boardMissMessageIds[boardMissState.boardId][1]);
    WaitForStoryMessage(boardMissState.messageWindowId, 5);
    boardMissState.choiceWindowId = CreateStoryChoiceWindow(MAKE_MESSID(30, 53), boardMissState.winningPlayerIndex);
    SetStoryMessage(boardMissState.choiceWindowId, MAKE_MESSID(30, 53));
    choiceAccepted = WaitForFirstStoryChoice(boardMissState.choiceWindowId);
    SetStoryMessage(boardMissState.choiceWindowId, MAKE_MESSID(37, 4));
    CloseStoryChoiceWindow(boardMissState.choiceWindowId);
    return choiceAccepted;
}

BOOL PromptRetryStory(Void) {
    BOOL choiceAccepted = FALSE;

    SetStoryMessage(boardMissState.messageWindowId, boardMissMessageIds[boardMissState.boardId][2]);
    WaitForStoryMessage(boardMissState.messageWindowId, 5);
    boardMissState.choiceWindowId = CreateStoryChoiceWindow(MAKE_MESSID(30, 53), boardMissState.winningPlayerIndex);
    SetStoryMessage(boardMissState.choiceWindowId, MAKE_MESSID(30, 53));
    choiceAccepted = WaitForFirstStoryChoice(boardMissState.choiceWindowId);
    SetStoryMessage(boardMissState.choiceWindowId, MAKE_MESSID(37, 4));
    CloseStoryChoiceWindow(boardMissState.choiceWindowId);
    return choiceAccepted;
}

BOOL PromptReturnToBoard(Void) {
    BOOL choiceAccepted = FALSE;

    SetBoardMissCharacterMotion(3, -1, 15, 1, 0);
    SetStoryMessage(boardMissState.messageWindowId, boardMissMessageIds[boardMissState.boardId][3]);
    WaitForStoryMessage(boardMissState.messageWindowId, 5);
    SetBoardMissCharacterMotion(1, -1, 15, 1, 0);
    boardMissState.choiceWindowId = CreateStoryChoiceWindow(MAKE_MESSID(30, 53), boardMissState.winningPlayerIndex);
    SetStoryMessage(boardMissState.choiceWindowId, MAKE_MESSID(30, 53));
    choiceAccepted = WaitForStoryChoice(boardMissState.choiceWindowId, 1);
    SetStoryMessage(boardMissState.choiceWindowId, MAKE_MESSID(37, 4));
    CloseStoryChoiceWindow(boardMissState.choiceWindowId);
    return choiceAccepted;
}

Void ShowBoardMissEnding(Void) {
    WaitFrames(60);
    PrepareBoardMissPlayers();
    Hu3DModelAttrReset(boardMissStageState.object->mdlId[5], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(boardMissStageState.object->mdlId[6], HU3D_ATTR_DISPOFF);
    SetObjectMotion(boardMissStageState.object, 2, 3, 0, 0);
    WaitFrames(30);
    SetObjectMotion(boardMissCharacterState.object, 1, 4, 15, 0);
    WaitFrames(30);
    SetObjectMotion(boardMissPlayerStates[boardMissState.currentPlayerIndex].object, 1, 3, 15, 0);
    WaitForObjectMotionEnd(boardMissStageState.object, 2, 3, 0);
    WaitFrames(60);
    HuAudSeqFadeOut(storySequenceIds[0], 3000);
    CaptureCameraStartState(&storyCameraState);
    SetStoryCameraCallback(TransitionBoardMissCameraToEnd);
    WaitFrames(150);
}

Void RunEmptyBoardMissCallback(Void) {
}

Void ShowBoardMissFinalSequence(Void) {
    SetObjectMotion(boardMissStageState.object, 2, 3, 0, 0);
    WaitForObjectMotionEnd(boardMissStageState.object, 2, 3, 0);
    CaptureCameraStartState(&storyCameraState);
    SetStoryCameraCallback(ResetBoardMissCamera);
    boardMissCharacterState.callback = (Void*) MoveWinnerToMissPosition;
    boardMissPlayerStates[boardMissState.currentPlayerIndex].callback = (Void*) MovePlayerOffStage;
    do {
        WaitOneFrame();
    } while (boardMissCharacterState.callback != NULL);
    SetObjectMotion(boardMissStageState.object, 2, 2, 0, 0);
    WaitForObjectMotionEnd(boardMissStageState.object, 2, 2, 0);
}

Void FinishBoardMissResponse(s32 boardId) {
    s32 i;

    WaitFrames(60);
    HuAudSeqFadeOut(storySequenceIds[0], 1000);
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (TRUE) {
        WaitOneFrame();
        if (WipeStatGet() == 0) {
            break;
        }
    }
    CharModelKill(-1);
    HuAudFadeOut(1);
    if (boardId == 1) {
        omOvlReturnEx(1, 1);
    } else {
        GWPlayerCfg->group = 0;
        GWPlayerCfg->character = boardMissState.playerResults[boardMissState.currentPlayerIndex].characterId;
        GWPlayerCfg->pad_idx = boardMissState.playerResults[boardMissState.currentPlayerIndex].controllerPort;
        GWPlayerCfg->iscom = 0;
        for (i = 0; i < 4; i++) {
            GWPlayer[i].character = GWPlayerCfg[i].character;
            GWPlayer[i].port = GWPlayerCfg[i].pad_idx;
            GWPlayer[i].com = GWPlayerCfg[i].iscom;
            GWPlayer[i].team = GWPlayerCfg[i].group;
        }
        omOvlGotoEx(DLL_mentdll, 1, 10, 0);
    }
    while (TRUE) {
        WaitOneFrame();
    }
}

Void RunBoardMissChoiceSequence(s32 windowId, StoryProcessTaskState* taskState) {
    s32 i;
    s32 selectedResponse = 0;

    PlayBoardMissIntroMusic();
    ShowBoardMissIntroduction();
    boardMissState.messageWindowId = CreateCenteredStoryWindow(1, 0, 21, 2, boardMissState.winningPlayerIndex);
    while (TRUE) {
        WaitOneFrame();
        if (PromptContinueStory()) {
            selectedResponse = 0;
            break;
        }
        if (PromptRetryStory()) {
            selectedResponse = 2;
            break;
        }
        if (PromptReturnToBoard()) {
            selectedResponse = 1;
            break;
        }
    }
    CloseCenteredStoryWindow(boardMissState.messageWindowId);
    for (i = 0; i < 4; i++) {
        if (boardMissState.playerResults[i].playerIndex != boardMissState.playerResults[boardMissState.currentPlayerIndex].playerIndex) {
            Hu3DModelAttrSet(boardMissPlayerStates[i].object->mdlId[1], HU3D_ATTR_DISPOFF);
        }
        Hu3DModelAttrSet(boardMissPlayerStates[i].object->mdlId[2], HU3D_ATTR_DISPOFF);
    }
    switch (selectedResponse) {
        case 1:
            RunEmptyBoardMissCallback();
            break;
        case 0:
            ShowBoardMissEnding();
            break;
        case 2:
            ShowBoardMissFinalSequence();
            break;
    }
    switch (selectedResponse) {
        case 0:
            LaunchBoardMinigameAfterMiss();
            break;
        case 1:
        case 2:
            FinishBoardMissResponse(selectedResponse);
            /* fallthrough */
        default:
            taskState->callback = 0;
            break;
    }
}

Void CreateBoardMissController(Void) {
    OMOBJ* gameObject;

    gameObject = omAddObjEx(boardMissObjectManager, 0x100, 16, 16, -1, NULL);
    boardMissControllerState.object = gameObject;
    boardMissControllerState.callback = (Void*) RunBoardMissChoiceSequence;
    boardMissControllerState.object->data = &boardMissControllerState;
    while (TRUE) {
        WaitOneFrame();
        if (boardMissControllerState.callback != NULL) {
            boardMissControllerState.callback(gameObject, &boardMissControllerState);
        }
    }
}

Void LoadBoardMissStageModels(OMOBJ* object, s32 eventIndex) {
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 56));
    object->mtnId[0] = Hu3DMotionIDGet(object->mdlId[1]);
    SetObjectMotion(object, 1, 0, 0, 2);
    Hu3DModelShadowMapObjSet(object->mdlId[1], "base_fix9-base");
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 59));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY, 60));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY, 61));
    SetObjectMotion(object, 2, 2, 0, 2);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 57));
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 63));
    object->mtnId[5] = Hu3DMotionIDGet(object->mdlId[5]);
    Hu3DModelPosSet(object->mdlId[5], 0.0f, 80.0f, 460.0f);
    Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowSet(object->mdlId[5]);
    SetObjectMotion(object, 5, 5, 0, 2);
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 64));
    object->mtnId[6] = Hu3DMotionIDGet(object->mdlId[6]);
    Hu3DModelHookSet(object->mdlId[5], "partybox_fix2-effect_fook1", object->mdlId[6]);
    Hu3DModelAttrSet(object->mdlId[6], HU3D_ATTR_DISPOFF);
    SetObjectMotion(object, 6, 6, 0, 2);
    boardMissPlayerAnimations[0] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY, 65 + eventIndex * 2), HU_MEMNUM_OVL));
    boardMissPlayerAnimations[1] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY, 66 + eventIndex * 2), HU_MEMNUM_OVL));
    Hu3DAnimCreate(boardMissPlayerAnimations[0], boardMissStageState.object->mdlId[5], "ys26_p0");
    Hu3DAnimCreate(boardMissPlayerAnimations[1], boardMissStageState.object->mdlId[5], "ys26_p1");
}

Void AnimateBoardMissStage(OMOBJ* object, StoryProcessTaskState* taskState) {
    Hu3DData[object->mdlId[5]].pos.y = InterpolateSineCycle(80.0f, 90.0f, taskState->animationPhase++, 360.0f);
    if (taskState->animationPhase >= 360) {
        taskState->animationPhase = 0;
    }
}

Void CreateBoardMissStage(Void) {
    OMOBJ* overlayObject;

    overlayObject = omAddObjEx(boardMissObjectManager, 0x100, 16, 16, -1, NULL);
    boardMissStageState.object = overlayObject;
    boardMissStageState.callback = (Void*) AnimateBoardMissStage;
    boardMissStageState.object->data = &boardMissStageState;
    LoadBoardMissStageModels(overlayObject, boardMissState.boardId);
    while (TRUE) {
        WaitOneFrame();
        if (boardMissStageState.callback != NULL) {
            boardMissStageState.callback(overlayObject, &boardMissStageState);
        }
    }
}

Void LoadBoardMissCharacter(OMOBJ* object, s32 characterIndex) {
    s32 characterModelIds[] = {
        DATA_MAKE_NUM(DATADIR_MSTORY, 0x61),
        DATA_MAKE_NUM(DATADIR_MSTORY, 0x6C),
        DATA_MAKE_NUM(DATADIR_MSTORY, 0x74),
        DATA_MAKE_NUM(DATADIR_MSTORY, 0x7C),
        DATA_MAKE_NUM(DATADIR_MSTORY, 0x83)
    };
    s32 characterMotionIdsByBoard[][5] = {
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x62), DATA_MAKE_NUM(DATADIR_MSTORY, 0x64), DATA_MAKE_NUM(DATADIR_MSTORY, 0x65), DATA_MAKE_NUM(DATADIR_MSTORY, 0x67), DATA_MAKE_NUM(DATADIR_MSTORY, 0x68) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x6D), DATA_MAKE_NUM(DATADIR_MSTORY, 0x6F), DATA_MAKE_NUM(DATADIR_MSTORY, 0x70), DATA_MAKE_NUM(DATADIR_MSTORY, 0x72), DATA_MAKE_NUM(DATADIR_MSTORY, 0x73) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x75), DATA_MAKE_NUM(DATADIR_MSTORY, 0x77), DATA_MAKE_NUM(DATADIR_MSTORY, 0x78), DATA_MAKE_NUM(DATADIR_MSTORY, 0x7A), DATA_MAKE_NUM(DATADIR_MSTORY, 0x7B) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x7D), DATA_MAKE_NUM(DATADIR_MSTORY, 0x7E), DATA_MAKE_NUM(DATADIR_MSTORY, 0x7F), DATA_MAKE_NUM(DATADIR_MSTORY, 0x81), DATA_MAKE_NUM(DATADIR_MSTORY, 0x82) },
        { DATA_MAKE_NUM(DATADIR_MSTORY, 0x84), DATA_MAKE_NUM(DATADIR_MSTORY, 0x86), DATA_MAKE_NUM(DATADIR_MSTORY, 0x87), DATA_MAKE_NUM(DATADIR_MSTORY, 0x89), DATA_MAKE_NUM(DATADIR_MSTORY, 0x8A) }
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
    SetObjectMotion(object, 1, 1, 0, 1);
    {
        s32 characterDustEffectIds[] = { 9, 14, 10, 11, 12 };

        if (characterIndex != 3) {
            CharNpcDustSet(object->mdlId[1], object->mtnId[2], 1, characterDustEffectIds[characterIndex]);
        }
    }
}

Void SetBoardMissCharacterMotion(s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 loopMode, s32 initialDelayFrames) {
    SetObjectMotion(boardMissCharacterState.object, 1, modelIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        initialDelayFrames = 0;
    }
    boardMissCharacterState.object->work[0] = initialDelayFrames;
    boardMissCharacterState.object->work[1] = motionIndex;
    boardMissCharacterState.object->work[2] = blendFrames + 1;
}

Void MoveWinnerToMissPosition(OMOBJ* object, StoryProcessTaskState* taskState) {
    Vec targetPosition = { 0.0f, 0.0f, 460.0f };

    SetObjectMotion(object, 1, 2, 15, 1);
    AnimateCharacterRotation(object, 1, targetPosition, 0.0f, 60.0f, 10.0f);
    SetObjectMotion(object, 1, 1, 15, 1);
    taskState->callback = 0;
}

Void RotateBoardMissCharacter(OMOBJ* object, StoryProcessTaskState* taskState) {
    s32 i;

    for (i = 0; i < 15; i++) {
        WaitOneFrame();
        Hu3DData[object->mdlId[1]].rot.y = InterpolateLinear(0.0f, 135.0f, i, 15.0f);
    }
    taskState->callback = 0;
}

Void MoveWinnerAwayFromMissPosition(OMOBJ* object, StoryProcessTaskState* taskState) {
    Vec targetPosition = { -120.0f, 0.0f, 670.0f };

    SetObjectMotion(object, 1, 2, 15, 1);
    AnimateCharacterRotation(object, 1, targetPosition, 45.0f, 60.0f, 10.0f);
    SetObjectMotion(object, 1, 1, 15, 1);
    taskState->callback = 0;
}

Void CreateBoardMissCharacterObject(Void) {
    OMOBJ* characterObject;

    characterObject = omAddObjEx(boardMissObjectManager, 0x100, 16, 16, -1, NULL);
    boardMissCharacterState.object = characterObject;
    boardMissCharacterState.callback = NULL;
    boardMissCharacterState.object->data = &boardMissCharacterState;
    LoadBoardMissCharacter(characterObject, boardMissState.boardId);
    while (TRUE) {
        WaitOneFrame();
        if (boardMissCharacterState.callback != NULL) {
            boardMissCharacterState.callback(characterObject, &boardMissCharacterState);
        }
        if (characterObject->work[0] != 0) {
            AdvanceCharacterMotion(characterObject);
        }
    }
}

Void CaptureBoardMissPlayerResults(s32 playerIndex) {
    s32 playerOrder[4];
    s32 i;

    playerOrder[0] = 0;
    for (i = 1; i < 4; i++) {
        playerOrder[i] = playerOrder[i - 1] + 1;
        if (playerOrder[i] >= 4) {
            playerOrder[i] = 0;
        }
        if (playerIndex == playerOrder[i]) {
            boardMissState.currentPlayerIndex = i;
        }
    }
    for (i = 0; i < 4; i++) {
        CapturePlayerResult(&boardMissState.playerResults[i], playerOrder[i]);
    }
    for (i = 0; i < 4; i++) {
        boardMissState.playerResults[i].resultValue = GWPlayer[playerOrder[i]].rank + 1;
    }
}

Vec boardMissPlayerPositions[] = {
    { -300.0f,   0.0f, 460.0f },
    { -100.0f, -30.0f, 460.0f },
    {  100.0f, -60.0f, 460.0f },
    {  300.0f, -90.0f, 460.0f }
};

Void CreateBoardMissPlayer(OMOBJ* object, s32 playerOrderIndex) {
    s32 playerIndex;
    s32 playerCharacterId;

    playerIndex = playerOrderIndex;
    playerCharacterId = boardMissState.playerResults[playerIndex].characterId;
    object->mdlId[1] = CharModelCreate(boardMissState.playerResults[playerOrderIndex].characterId, 4);
    object->mtnId[1] = CharMotionCreate(boardMissState.playerResults[playerOrderIndex].characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0));
    object->mtnId[2] = CharMotionCreate(boardMissState.playerResults[playerOrderIndex].characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 3));
    object->mtnId[3] = CharMotionCreate(boardMissState.playerResults[playerOrderIndex].characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 109));
    CharMotionDataClose(playerCharacterId);
    Hu3DModelPosSet(object->mdlId[1], boardMissPlayerPositions[playerIndex].x, 90.0f + boardMissPlayerPositions[boardMissState.playerResults[playerIndex].resultValue - 1].y, boardMissPlayerPositions[playerIndex].z - 15.0f);
    Hu3DModelRotSet(object->mdlId[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(object->mdlId[1], 1.0f, 1.0f, 1.0f);
    Hu3DModelShadowSet(object->mdlId[1]);
    SetObjectMotion(object, 1, 1, 0, 1);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY, 62));
    Hu3DModelPosSet(object->mdlId[2], boardMissPlayerPositions[playerIndex].x, boardMissPlayerPositions[boardMissState.playerResults[playerIndex].resultValue - 1].y, boardMissPlayerPositions[playerIndex].z);
    if (boardMissState.playerResults[playerIndex].resultValue == 4) {
        Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    }
    Hu3DModelScaleSet(object->mdlId[2], 1.5f, 1.0f, 1.5f);
    Hu3DModelShadowMapSet(object->mdlId[2]);
}

Void SetBoardMissPlayerMotion(s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 loopMode, s32 initialDelayFrames) {
    SetObjectMotion(boardMissPlayerStates[boardMissState.currentPlayerIndex].object, 1, modelIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        initialDelayFrames = 0;
    }
    boardMissPlayerStates[boardMissState.currentPlayerIndex].object->work[0] = initialDelayFrames;
    boardMissPlayerStates[boardMissState.currentPlayerIndex].object->work[1] = motionIndex;
    boardMissPlayerStates[boardMissState.currentPlayerIndex].object->work[2] = blendFrames + 1;
}

Void AnimateFallingPlayer(OMOBJ* object, StoryProcessTaskState* taskState) {
    float startingModelY;
    s32 i;

    startingModelY = Hu3DData[object->mdlId[1]].pos.y;
    for (i = 0; i < 60; i++) {
        WaitOneFrame();
        Hu3DData[object->mdlId[1]].pos.y = InterpolateSineEaseOut(startingModelY, 0.0f, i, 60.0f);
        Hu3DData[object->mdlId[2]].pos.y = InterpolateSineEaseOut(startingModelY - 90.0f, -90.0f, i, 60.0f);
    }
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    taskState->callback = 0;
}

Void MoveWinnerOffStage(OMOBJ* object, StoryProcessTaskState* taskState) {
    Vec targetPosition = { 120.0f, 0.0f, 670.0f };

    boardMissCharacterState.callback = (Void*) MoveWinnerAwayFromMissPosition;
    SetObjectMotion(boardMissStageState.object, 2, 2, 0, 0);
    SetObjectMotion(object, 1, 2, 15, 1);
    AnimateCharacterRotation(object, 1, targetPosition, -45.0f, 30.0f, 10.0f);
    SetObjectMotion(object, 1, 1, 15, 1);
    taskState->callback = 0;
}

Void MovePlayerOffStage(OMOBJ* object, StoryProcessTaskState* taskState) {
    Vec targetPosition = { 600.0f, 0.0f, 700.0f };

    SetObjectMotion(object, 1, 2, 15, 1);
    AnimateCharacterRotation(object, 1, targetPosition, -45.0f, 30.0f, 10.0f);
    SetObjectMotion(object, 1, 1, 15, 1);
    taskState->callback = 0;
}

Void CreateBoardMissPlayerObject(Void) {
    OMOBJ* playerObject;
    s32 playerObjectIndex;

    playerObjectIndex = boardMissState.playerObjectCount++;
    playerObject = omAddObjEx(boardMissObjectManager, 0x100, 16, 16, -1, NULL);
    boardMissPlayerStates[playerObjectIndex].object = playerObject;
    boardMissPlayerStates[playerObjectIndex].callback = NULL;
    boardMissPlayerStates[playerObjectIndex].object->data = boardMissPlayerStates;
    CreateBoardMissPlayer(playerObject, playerObjectIndex);
    while (TRUE) {
        WaitOneFrame();
        if (boardMissPlayerStates[playerObjectIndex].callback != NULL) {
            boardMissPlayerStates[playerObjectIndex].callback(playerObject, &boardMissPlayerStates[playerObjectIndex]);
        }
        if (playerObject->work[0] != 0) {
            AdvanceCharacterMotion(playerObject);
        }
    }
}

Void SetBoardMissCameraStart(Void) {
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

Void MoveBoardMissCameraToCharacter(Void) {
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
    if (boardMissState.boardOffsetY != 0.0f) {
        cameraState->centerPosition.y = 125.0f + boardMissState.boardOffsetY;
    }
}

Void TransitionBoardMissCameraToEnd(Void) {
    StoryCameraTransitionState* cameraState;
    StoryCameraTransitionState cameraTarget;

    cameraState = &storyCameraState;
    if (cameraState->transitionFrame == 50) {
        SetObjectMotion(boardMissStageState.object, 5, 5, 0, 0);
        SetObjectMotion(boardMissStageState.object, 6, 6, 0, 0);
        HuAudFXPlay(0xA1);
    }
    cameraTarget.centerPosition.x = 0.0f;
    cameraTarget.centerPosition.y = 100.0f;
    cameraTarget.centerPosition.z = 460.0f;
    cameraTarget.rotation.x = 0.0f;
    cameraTarget.rotation.y = 0.0f;
    cameraTarget.rotation.z = 0.0f;
    cameraTarget.zoom = 125.0f;
    InterpolateCameraTransition(cameraState, &cameraTarget, cameraState->transitionFrame, 180.0f, 10.0f);
    if (cameraState->transitionFrame++ >= 90) {
        if (cameraState->transitionFrame == 91) {
            currentStorySoundEffectId = HuAudFXPlay(0x21);
        }
        cameraState->rotation.x = InterpolateLinear(cameraState->rotation.x, -45.0f, cameraState->transitionFrame - 90, 90.0f);
        Hu3DData[boardMissStageState.object->mdlId[5]].rot.x = InterpolateLinear(0.0f, 45.0f, cameraState->transitionFrame - 90, 90.0f);
    }
}

Void ResetBoardMissCamera(Void) {
    StoryCameraTransitionState* cameraState;
    StoryCameraTransitionState cameraTarget;

    cameraState = &storyCameraState;
    cameraTarget.centerPosition.x = 0.0f;
    cameraTarget.centerPosition.y = 220.0f;
    cameraTarget.centerPosition.z = 0.0f;
    cameraTarget.rotation.x = 0.0f;
    cameraTarget.rotation.y = 0.0f;
    cameraTarget.rotation.z = 0.0f;
    cameraTarget.zoom = 1600.0f;
    InterpolateCameraTransition(cameraState, &cameraTarget, cameraState->transitionFrame++, 60.0f, 30.0f);
}
