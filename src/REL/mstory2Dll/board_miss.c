#include "REL/mstory2Dll.h"
#include "game/armem.h"
#include "game/audio.h"

extern s32 rand8(Void);
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
} BoardMissState; // Size 0x104

typedef struct {
    /* 0x00 */ s32 reserved00;
    /* 0x04 */ s32 reserved04;
    /* 0x08 */ s32 isComputer;
    /* 0x0C */ char reserved0C[4];
    /* 0x10 */ s32 character;
    /* 0x14 */ s32 padIndex;
} BoardMissPlayerState; // Size 0x18

BoardMissState boardMissState;
StoryObjectProcessState boardMissMainProcess;
StoryObjectProcessState boardMissMapProcess;
StoryObjectProcessState boardMissKoopaProcess;
StoryObjectProcessState boardMissSecondaryKoopaProcess;
StoryObjectProcessState boardMissPlayerProcesses[4];
BoardMissPlayerState boardMissPlayerConfigs[4];
BOOL boardMissResourcesReady;
HUPROCESS *boardMissProcess;

s32 boardDllByBoardIndex[7] = {
    DATADIR_W01,
    DATADIR_W02,
    DATADIR_W03,
    DATADIR_W04,
    DATADIR_W05,
    DATADIR_W06,
    DATADIR_W10,
};

s32 boardMissCharacterDataDirectories[8] = { DATADIR_MARIOMDL1, DATADIR_LUIGIMDL1, DATADIR_PEACHMDL1, DATADIR_YOSHIMDL1, DATADIR_WARIOMDL1, DATADIR_DONKEYMDL1,
    DATADIR_DAISYMDL1, DATADIR_WALUIGIMDL1 };

Void RunBoardMissOverlayProcess(Void);
Void RunBoardMissMapProcess(Void);
Void AnimateBoardMissCharacter(OMOBJ *object, StoryObjectProcessState *processState);
Void RunBoardMissCharacterProcess(Void);
Void SetBoardMissKoopaMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags);
Void AnimateBoardMissKoopa(OMOBJ *object, StoryObjectProcessState *processState);
Void RunBoardMissKoopaProcess(Void);
Void PrepareBoardMissPlayers(int selectedPlayerIndex);
Void AnimateBoardMissPlayer(OMOBJ *object, StoryObjectProcessState *processState);
Void MoveBoardMissPlayer(OMOBJ *object, StoryObjectProcessState *processState);
Void RunBoardMissPlayerProcess(Void);
Void HandleBoardMissProcessEnd(Void);
Void UpdateBoardMissCamera(Void);
Void AnimateBoardMissCamera(Void);

Void LoadBoardMissResources(Void)
{
    int statId = HuDataDirReadAsync(DATADIR_BOARD);
    if (statId != -1) {
        while (!HuDataGetAsyncStat(statId)) {
            HuPrcVSleep();
        }
    }
    HuAR_MRAMtoARAM(DATADIR_BOARD);
    while (HuARDMACheck()) {
        HuPrcVSleep();
    }
    HuDataDirClose(DATADIR_BOARD);
    statId = HuDataDirReadAsync(boardDllByBoardIndex[GWSystem.board]);
    if (statId != -1) {
        while (!HuDataGetAsyncStat(statId)) {
            HuPrcVSleep();
        }
    }
    boardMissResourcesReady = TRUE;
    HuPrcEnd();
    while (1) {
        HuPrcVSleep();
    }
}

Void ConfigureBoardMissPlayers(Void)
{
    int i, j;
    boardMissPlayerConfigs[0].reserved00 = 0;
    boardMissPlayerConfigs[0].reserved04 = 0;
    boardMissPlayerConfigs[0].character = boardMissState.playerInfo[boardMissState.selectedPlayerIndex].characterId;
    boardMissPlayerConfigs[0].padIndex = boardMissState.playerInfo[boardMissState.selectedPlayerIndex].padIndex;
    boardMissPlayerConfigs[0].isComputer = 0;
    for (i = 1; i < 4; i++) {
        boardMissPlayerConfigs[i].character = -1;
        boardMissPlayerConfigs[i].padIndex = i;
        boardMissPlayerConfigs[i].isComputer = 1;
    }
    for (i = 1; i < 4; i++) {
        do {
            boardMissPlayerConfigs[i].character = rand8() % 8;
            for (j = 0; j < 4; j++) {
                if (i != j && boardMissPlayerConfigs[j].character != -1 && boardMissPlayerConfigs[i].character == boardMissPlayerConfigs[j].character) {
                    break;
                }
            }
        } while (j != 4);
    }
    for (i = 0; i < 4; i++) {
        GWPlayerCfg[i].character = boardMissPlayerConfigs[i].character;
        GWPlayerCfg[i].pad_idx = boardMissPlayerConfigs[i].padIndex;
        GWPlayerCfg[i].group = 0;
        GWPlayerCfg[i].iscom = boardMissPlayerConfigs[i].isComputer;
    }
    BoardSaveInit(GWSystem.board);
    CharDataClose(-1);
    HuDataDirClose(DATADIR_MSTORY2);
    HuPrcChildCreate(LoadBoardMissResources, 100, 12288, 0, boardMissProcess);
}

Void LaunchBoardMissMinigame(Void)
{
    int ovlTbl[7] = { DLL_w01dll, DLL_w02dll, DLL_w03dll, DLL_w04dll, DLL_w05dll, DLL_w06dll, DLL_w10dll };
    OMOVLHIS *his;
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet()) {
        WaitOneFrame();
    }
    CharModelKill(-1);
    MGSeqKillAll();
    his = omOvlHisGet(0);
    omOvlHisChg(0, DLL_mstory3dll, 0, his->stat);
    do {
        WaitOneFrame();
    } while (boardMissResourcesReady != 1);
    CharMotionInit(GWPlayerCfg[0].character);
    CharMotionInit(GWPlayerCfg[1].character);
    CharMotionInit(GWPlayerCfg[2].character);
    CharMotionInit(GWPlayerCfg[3].character);
    HuAudSeqAllFadeOut(1000);
    HuAudSStreamAllFadeOut(1000);
    omOvlCallEx(ovlTbl[GWSystem.board], 1, 0, 0);
    while (1) {
        WaitOneFrame();
    }
}

Void StartBoardMissEvent(HUPROCESS *parentProcess, s32 activePadIndex)
{
    int i;
    boardMissProcess = parentProcess;
    boardMissState.activePadIndex = activePadIndex;
    InitializeWindowSystem();
    CreateStoryDirectionalLight(255);
    InitializeStoryCamera(boardMissProcess, HandleBoardMissProcessEnd);
    CreateStoryShadow();
    PrepareBoardMissPlayers(activePadIndex);
    HuPrcChildCreate(RunBoardMissMapProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunBoardMissCharacterProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunBoardMissKoopaProcess, 100, 8192, 0, HuPrcCurrentGet());
    for (i = 0; i < 4; i++) {
        HuPrcChildCreate(RunBoardMissPlayerProcess, 100, 8192, 0, HuPrcCurrentGet());
    }
    HuPrcChildCreate(RunBoardMissOverlayProcess, 100, 8192, 0, HuPrcCurrentGet());
}

Void UpdateBoardMissObjectMotion(OMOBJ *object)
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

Void DisplayBoardMissDebugStatus(Void)
{
    int posX = 16;
    int posY = 40;
    GXColor winColor = { 0, 0, 128, 128 };
    int i;
    while (1) {
        WaitOneFrame();
        printWin(posX, posY, 220, 160, &winColor);
        if (boardMissMainProcess.updateCallback == NULL) {
            print8(posX, posY, 1, "PROC_MAIN    -> FALSE");
        }
        else {
            print8(posX, posY, 1, "PROC_MAIN    -> TRUE");
        }
        if (boardMissMapProcess.updateCallback == NULL) {
            print8(posX, posY + 10, 1, "PROC_MAP     -> FALSE");
        }
        else {
            print8(posX, posY + 10, 1, "PROC_MAP     -> TRUE");
        }
        if (boardMissKoopaProcess.updateCallback == NULL) {
            print8(posX, posY + 20, 1, "PROC_KOOPA   -> FALSE");
        }
        else {
            print8(posX, posY + 20, 1, "PROC_KOOPA   -> TRUE");
        }
        if (boardMissSecondaryKoopaProcess.updateCallback == NULL) {
            print8(posX, posY + 30, 1, "PROC_MKOOPA  -> FALSE");
        }
        else {
            print8(posX, posY + 30, 1, "PROC_MKOOPA  -> TRUE");
        }
        for (i = 0; i < 4; i++) {
            if (boardMissPlayerProcesses[i].updateCallback == NULL) {
                print8(posX, posY + 40 + (i * 10), 1, "PROC_PLAYER%d -> FALSE", i);
            }
            else {
                print8(posX, posY + 40 + (i * 10), 1, "PROC_PLAYER%d -> TRUE", i);
            }
        }
        print8(posX, posY + 90, 1, "P:%d W:%d", boardMissState.selectedPlayerIndex, boardMissState.activePadIndex);
        print8(posX, posY + 110, 1, "NO) IDX:GRP:COM:DIF:PAD:CHR");
        for (i = 0; i < 4; i++) {
            print8(posX, posY + 120 + (i * 10), 1, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", i, boardMissState.playerInfo[i].playerIndex, boardMissState.playerInfo[i].group,
                boardMissState.playerInfo[i].isComputer, boardMissState.playerInfo[i].difficulty, boardMissState.playerInfo[i].padIndex, boardMissState.playerInfo[i].characterId);
        }
    }
}

Void BeginBoardMissScene(Void)
{
    storyAudioSequenceIds[0] = HuAudSeqPlay(52);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet());
    WaitFrames(60);
}

Void WaitForBoardMissPlayerAnimations(Void)
{
    int i;
    for (i = 0; i < 4; i++) {
        boardMissPlayerProcesses[i].updateCallback = (Void *)AnimateBoardMissPlayer;
    }
    do {
        WaitOneFrame();
        for (i = 0; i < 4; i++) {
            if (boardMissPlayerProcesses[i].updateCallback != NULL) {
                break;
            }
        }
    } while (i != 4);
    boardMissPlayerProcesses[boardMissState.selectedPlayerIndex].updateCallback = (Void *)MoveBoardMissPlayer;
    CaptureCameraMotionStart(&storyCameraState);
    SetCameraUpdateCallback(UpdateBoardMissCamera);
    do {
        WaitOneFrame();
    } while (boardMissPlayerProcesses[boardMissState.selectedPlayerIndex].updateCallback != NULL);
    do {
        WaitOneFrame();
    } while (boardMissSecondaryKoopaProcess.updateCallback != NULL);
    WaitFrames(30);
    if (0) {
        // Dummy code to stop this function from inlining
        do {
            WaitOneFrame();
            for (i = 0; i < 4; i++) {
                if (boardMissPlayerProcesses[i].updateCallback != NULL) {
                    break;
                }
            }
        } while (i != 4);
        boardMissPlayerProcesses[boardMissState.selectedPlayerIndex].updateCallback = (Void *)MoveBoardMissPlayer;
        CaptureCameraMotionStart(&storyCameraState);
        SetCameraUpdateCallback(UpdateBoardMissCamera);
        do {
            WaitOneFrame();
        } while (boardMissPlayerProcesses[boardMissState.selectedPlayerIndex].updateCallback != NULL);
        do {
            WaitOneFrame();
        } while (boardMissSecondaryKoopaProcess.updateCallback != NULL);
        WaitFrames(30);
    }
}

Void ShowBoardMissOpeningMessage(Void)
{
    boardMissState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, boardMissState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[5][0]);
    SetBoardMissKoopaMotion(3, -1, 15, 1, 0);
    SetWindowMessage(boardMissState.activeWindowId, 0x1F0009);
    WaitForWindowMessage(boardMissState.activeWindowId, -1);
    SetWindowMessage(boardMissState.activeWindowId, 0x250004);
    SetBoardMissKoopaMotion(1, -1, 15, 1, 0);
    CloseCenteredMessageWindow(boardMissState.activeWindowId);
}

int AskBoardMissContinueChoice(Void)
{
    int result = 0;
    SetWindowMessage(boardMissState.activeWindowId, 0x1E0021);
    WaitForWindowMessage(boardMissState.activeWindowId, 5);
    boardMissState.eventWindowId = CreateAutoSizedMessageWindow(0x1E0035, boardMissState.activePadIndex);
    SetWindowMessage(boardMissState.eventWindowId, 0x1E0035);
    result = GetDefaultWindowChoice(boardMissState.eventWindowId);
    SetWindowMessage(boardMissState.eventWindowId, 0x250004);
    CloseAutoSizedMessageWindow(boardMissState.eventWindowId);
    return result;
}

int AskBoardMissRetryChoice(Void)
{
    int result = 0;
    SetWindowMessage(boardMissState.activeWindowId, 0x1E0005);
    WaitForWindowMessage(boardMissState.activeWindowId, 5);
    boardMissState.eventWindowId = CreateAutoSizedMessageWindow(0x1E0035, boardMissState.activePadIndex);
    SetWindowMessage(boardMissState.eventWindowId, 0x1E0035);
    result = GetWindowChoice(boardMissState.eventWindowId, 1);
    SetWindowMessage(boardMissState.eventWindowId, 0x250004);
    CloseAutoSizedMessageWindow(boardMissState.eventWindowId);
    return result;
}

Void RunBoardMissSequence(Void)
{
    boardMissSecondaryKoopaProcess.updateCallback = (Void *)AnimateBoardMissKoopa;
    boardMissState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, boardMissState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[5][0]);
    SetWindowMessage(boardMissState.activeWindowId, 0x1F000A);
    WaitForWindowMessage(boardMissState.activeWindowId, -1);
    SetWindowMessage(boardMissState.activeWindowId, 0x250004);
    CloseCenteredMessageWindow(boardMissState.activeWindowId);
    HuAudSeqFadeOut(storyAudioSequenceIds[0], 1000);
    do {
        WaitOneFrame();
    } while (boardMissSecondaryKoopaProcess.updateCallback != NULL);
    boardMissKoopaProcess.updateCallback = (Void *)AnimateBoardMissCharacter;
    do {
        WaitOneFrame();
    } while (boardMissKoopaProcess.updateCallback != NULL);
    AnimateCharacterMeshOffset(boardMissPlayerProcesses[boardMissState.selectedPlayerIndex].object->mdlId[1], boardMissState.playerInfo[boardMissState.selectedPlayerIndex].characterId, 1, 20, 1);
    storyAudioSequenceIds[1] = HuAudSeqPlay(6);
    WaitFrames(30);
    ShiftObjectMotion(boardMissKoopaProcess.object, 1, 4, 30, 0);
    WaitFrames(30);
    boardMissState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, boardMissState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[6][0]);
    HuWinAttrSet(boardMissState.activeWindowId, 0x400);
    HuWinMesSet(boardMissState.activeWindowId, 0x1F000B);
    WaitForObjectMotionEnd(boardMissKoopaProcess.object, 1, 4, 10);
    ShiftObjectMotion(boardMissKoopaProcess.object, 1, 1, 10, 1);
    HuWinAttrReset(boardMissState.activeWindowId, 0x400);
    HuWinKeyWaitEntry(boardMissState.activeWindowId);
    WaitOneFrame();
    WaitForWindowMessage(boardMissState.activeWindowId, -1);
    SetWindowMessage(boardMissState.activeWindowId, 0x250004);
    CloseCenteredMessageWindow(boardMissState.activeWindowId);
    WaitFrames(60);
    ConfigureBoardMissPlayers();
    Hu3DModelAttrReset(boardMissMapProcess.object->mdlId[5], 1);
    Hu3DModelAttrReset(boardMissMapProcess.object->mdlId[6], 1);
    ShiftObjectMotion(boardMissKoopaProcess.object, 1, 4, 15, 1);
    ShiftObjectMotion(boardMissMapProcess.object, 2, 3, 0, 0);
    WaitFrames(45);
    HuAudFXPlay(storyAudioCueIds[6][0]);
    WaitFrames(15);
    ShiftObjectMotion(boardMissPlayerProcesses[boardMissState.selectedPlayerIndex].object, 1, 5, 0, 0);
    WaitOneFrame();
    AnimateCharacterMeshOffset(boardMissPlayerProcesses[boardMissState.selectedPlayerIndex].object->mdlId[1], boardMissState.playerInfo[boardMissState.selectedPlayerIndex].characterId, 1, 10, 0);
    WaitForObjectMotionEnd(boardMissMapProcess.object, 2, 3, 0);
    HuAudSeqFadeOut(storyAudioSequenceIds[1], 3000);
    CaptureCameraMotionStart(&storyCameraState);
    SetCameraUpdateCallback(AnimateBoardMissCamera);
    WaitFrames(150);
}

Void BoardMissNoOpCallback(Void) { }

Void FinishBoardMissEvent(Void)
{
    WaitFrames(60);
    HuAudSeqFadeOut(storyAudioSequenceIds[0], 1000);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet());
    CharModelKill(-1);
    HuAudFadeOut(1);
    omOvlReturnEx(1, 1);
}

Void RunBoardMissMainProcess(OMOBJ *object, StoryObjectProcessState *processState)
{
    int mode = 0;
    int i;
    BeginBoardMissScene();
    WaitForBoardMissPlayerAnimations();
    ShowBoardMissOpeningMessage();
    boardMissState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, boardMissState.activePadIndex);
    while (1) {
        WaitOneFrame();
        if (AskBoardMissContinueChoice()) {
            mode = 0;
            break;
        }
        else if (AskBoardMissRetryChoice()) {
            mode = 1;
            break;
        }
    }

    CloseCenteredMessageWindow(boardMissState.activeWindowId);
    for (i = 0; i < 4; i++) {
        if (boardMissState.playerInfo[i].playerIndex != boardMissState.playerInfo[boardMissState.selectedPlayerIndex].playerIndex) {
            Hu3DModelAttrSet(boardMissPlayerProcesses[i].object->mdlId[1], 1);
        }
        Hu3DModelAttrSet(boardMissPlayerProcesses[i].object->mdlId[2], 1);
    }
    switch (mode) {
        case 0:
            RunBoardMissSequence();
            break;
        case 1:
            break;
    }
    switch (mode) {
        case 0:
            LaunchBoardMissMinigame();
            break;

        case 1:
            FinishBoardMissEvent();
            break;
    }
    processState->updateCallback = NULL;
}

Void RunBoardMissOverlayProcess(Void)
{
    OMOBJ *object;
    boardMissMainProcess.object = object = omAddObjEx(boardMissProcess, 256, 16, 16, -1, NULL);
    boardMissMainProcess.updateCallback = (Void *)RunBoardMissMainProcess;
    boardMissMainProcess.object->data = &boardMissMainProcess;
    while (1) {
        WaitOneFrame();
        if (boardMissMainProcess.updateCallback != NULL) {
            boardMissMainProcess.updateCallback(object, &boardMissMainProcess);
        }
    }
}

Void CreateBoardMissMapObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x52));
    object->mtnId[1] = Hu3DMotionIDGet(object->mdlId[1]);
    Hu3DModelShadowMapObjSet(object->mdlId[1], "base_story-base");
    ShiftObjectMotion(object, 1, 1, 0, 1);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x53));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x54));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x55));
    ShiftObjectMotion(object, 2, 2, 0, 2);
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x57));
    object->mtnId[5] = Hu3DMotionIDGet(object->mdlId[5]);
    Hu3DModelPosSet(object->mdlId[5], 0, 80, 460);
    Hu3DModelAttrSet(object->mdlId[5], 1);
    Hu3DModelShadowSet(object->mdlId[5]);
    ShiftObjectMotion(object, 5, 5, 0, 2);
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x58));
    object->mtnId[6] = Hu3DMotionIDGet(object->mdlId[6]);
    Hu3DModelHookSet(object->mdlId[5], "koopabox_fix-effect_fook2", object->mdlId[6]);
    Hu3DModelAttrSet(object->mdlId[6], 1);
    ShiftObjectMotion(object, 6, 6, 0, 2);
}

Void AnimateBoardMissMapObject(OMOBJ *object, StoryObjectProcessState *processState)
{
    Hu3DData[object->mdlId[5]].pos.y = InterpolateSineCycle(80, 90, processState->animationFrame++, 360);
    if (processState->animationFrame >= 360) {
        processState->animationFrame = 0;
    }
}

Void RunBoardMissMapProcess(Void)
{
    OMOBJ *object;
    boardMissMapProcess.object = object = omAddObjEx(boardMissProcess, 256, 16, 16, -1, NULL);
    boardMissMapProcess.updateCallback = (Void *)AnimateBoardMissMapObject;
    boardMissMapProcess.object->data = &boardMissMapProcess;
    CreateBoardMissMapObject(object);
    while (1) {
        WaitOneFrame();
        if (boardMissMapProcess.updateCallback != NULL) {
            boardMissMapProcess.updateCallback(object, &boardMissMapProcess);
        }
    }
}

Void CreateBoardMissCharacterObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x62 : 0x66));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x63 : 0x67));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x65 : 0x69));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x69 : 0x6C));
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x66 : 0x6A));
    Hu3DModelPosSet(object->mdlId[1], -180, 0, 800);
    Hu3DModelRotSet(object->mdlId[1], 0, 60, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelAttrSet(object->mdlId[1], 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 1, 0, 1);
}

Void SetBoardMissCharacterMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(boardMissKoopaProcess.object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    boardMissKoopaProcess.object->work[0] = motionFlags;
    boardMissKoopaProcess.object->work[1] = motionSlot;
    boardMissKoopaProcess.object->work[2] = blendFrames + 1;
}

Void AnimateBoardMissCharacter(OMOBJ *object, StoryObjectProcessState *processState)
{
    int i = 0;
    float cameraMoveSpeed = 50;

    HuAudFXPlay(153);
    SetBoardMissCharacterMotion(2, 1, 0, 0, 1);
    WaitFrames(5);
    Hu3DMotionTimeSet(boardMissKoopaProcess.object->mdlId[1], 100);
    WaitOneFrame();
    Hu3DModelAttrReset(object->mdlId[1], 1);
    while (1) {
        WaitOneFrame();
        if (Hu3DMotionTimeGet(boardMissKoopaProcess.object->mdlId[1]) >= 137) {
            break;
        }
    }
    HuAudFXPlay(156);
    omVibrate(boardMissState.activePadIndex, 45, 12, 0);
    SetObjectMotionTransition(boardMissPlayerProcesses[boardMissState.selectedPlayerIndex].object, 6, 1, 5, 0);
    for (i = 0; i < 60; i++) {
        float posY;
        WaitOneFrame();
        posY = InterpolateSineCycle(0, cameraMoveSpeed, i % 15, 15);
        cameraMoveSpeed = InterpolateWeightedAverage(cameraMoveSpeed, 0, 30);
        boardMissState.verticalOffset = InterpolateWeightedAverage(boardMissState.verticalOffset, posY, 5);
    }
    boardMissState.verticalOffset = 0;
    processState->updateCallback = NULL;
}

Void RunBoardMissCharacterProcess(Void)
{
    OMOBJ *object;
    boardMissKoopaProcess.object = object = omAddObjEx(boardMissProcess, 256, 16, 16, -1, NULL);
    boardMissKoopaProcess.updateCallback = NULL;
    boardMissKoopaProcess.object->data = &boardMissMapProcess;
    CreateBoardMissCharacterObject(object);
    while (1) {
        WaitOneFrame();
        if (boardMissKoopaProcess.updateCallback != NULL) {
            boardMissKoopaProcess.updateCallback(object, &boardMissKoopaProcess);
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

Void CreateBoardMissKoopaObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x7D : 0x7B));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x7E : 0x7C));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x80 : 0x7D));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x81 : 0x7E));
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x84 : 0x7F));
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x85 : 0x80));
    Hu3DModelPosSet(object->mdlId[1], -300, 0, 890);
    Hu3DModelRotSet(object->mdlId[1], 0, 30, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 1, 0, 1);
    CharNpcDustSet(object->mdlId[1], object->mtnId[2], 1, 13);
}

Void SetBoardMissKoopaMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(boardMissSecondaryKoopaProcess.object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    boardMissSecondaryKoopaProcess.object->work[0] = motionFlags;
    boardMissSecondaryKoopaProcess.object->work[1] = motionSlot;
    boardMissSecondaryKoopaProcess.object->work[2] = blendFrames + 1;
}

Void AnimateBoardMissKoopa(OMOBJ *object, StoryObjectProcessState *processState)
{
    Vec pos = { -600, 0, 800 };
    ShiftObjectMotion(object, 1, 2, 15, 1);
    MoveModelToPosition(object, 1, pos, -90, 60, 10);
    ShiftObjectMotion(object, 1, 1, 15, 1);
    processState->updateCallback = NULL;
}

Void AnimateBoardMissKoopaSecondary(OMOBJ *object, StoryObjectProcessState *processState)
{
    int i;
    for (i = 0; i < 15; i++) {
        WaitOneFrame();
        Hu3DData[object->mdlId[1]].rot.y = InterpolateLinear(0, 135, i, 15);
    }
    processState->updateCallback = NULL;
}

Void AnimateBoardMissKoopaExit(OMOBJ *object, StoryObjectProcessState *processState)
{
    Vec pos = { -120, 0, 800 };
    ShiftObjectMotion(object, 1, 2, 15, 1);
    MoveModelToPosition(object, 1, pos, 60, 60, 10);
    ShiftObjectMotion(object, 1, 1, 15, 1);
    processState->updateCallback = NULL;
}

Void RunBoardMissKoopaProcess(Void)
{
    OMOBJ *object;
    boardMissSecondaryKoopaProcess.object = object = omAddObjEx(boardMissProcess, 256, 16, 16, -1, NULL);
    boardMissSecondaryKoopaProcess.updateCallback = NULL;
    boardMissSecondaryKoopaProcess.object->data = &boardMissSecondaryKoopaProcess;
    CreateBoardMissKoopaObject(object);
    while (1) {
        WaitOneFrame();
        if (boardMissSecondaryKoopaProcess.updateCallback != NULL) {
            boardMissSecondaryKoopaProcess.updateCallback(object, &boardMissSecondaryKoopaProcess);
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

Void PrepareBoardMissPlayers(int selectedPlayerIndex)
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
            boardMissState.selectedPlayerIndex = i;
        }
    }
    for (i = 0; i < 4; i++) {
        ReadStoryPlayerConfig(&boardMissState.playerInfo[i], playerOrder[i]);
    }
    for (i = 0; i < 4; i++) {
        boardMissState.playerInfo[i].rank = GWPlayer[playerOrder[i]].rank + 1;
    }
}

Vec boardMissPlayerPositions[4] = {
    { -300, 0, 460 },
    { -100, -30, 460 },
    { 100, -60, 460 },
    { 300, -90, 460 },
};

Void CreateBoardMissPlayerModel(OMOBJ *object, int player)
{
    int playerIndex = player;
    int characterId = boardMissState.playerInfo[playerIndex].characterId;
    object->mdlId[1] = CharModelCreate(characterId, 4);
    object->mtnId[1] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00));
    object->mtnId[2] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03));
    object->mtnId[3] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x05));
    object->mtnId[4] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x1A));
    object->mtnId[5] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x6D));
    object->mtnId[6] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x18) + characterId);
    CharMotionDataClose(characterId);
    Hu3DModelPosSet(
        object->mdlId[1], boardMissPlayerPositions[playerIndex].x, boardMissPlayerPositions[boardMissState.playerInfo[playerIndex].rank - 1].y + 90, boardMissPlayerPositions[playerIndex].z);
    Hu3DModelRotSet(object->mdlId[1], 0, 0, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 1, 0, 1);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x56));
    Hu3DModelPosSet(
        object->mdlId[2], boardMissPlayerPositions[playerIndex].x, boardMissPlayerPositions[boardMissState.playerInfo[playerIndex].rank - 1].y, boardMissPlayerPositions[playerIndex].z);
    if (boardMissState.playerInfo[playerIndex].rank == 4) {
        Hu3DModelAttrSet(object->mdlId[2], 1);
    }
    Hu3DModelScaleSet(object->mdlId[2], 1.5, 1, 1.5);
    Hu3DModelShadowMapSet(object->mdlId[2]);
}

Void SetBoardMissPlayerMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(boardMissPlayerProcesses[boardMissState.selectedPlayerIndex].object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    boardMissPlayerProcesses[boardMissState.selectedPlayerIndex].object->work[0] = motionFlags;
    boardMissPlayerProcesses[boardMissState.selectedPlayerIndex].object->work[1] = motionSlot;
    boardMissPlayerProcesses[boardMissState.selectedPlayerIndex].object->work[2] = blendFrames + 1;
}

Void AnimateBoardMissPlayer(OMOBJ *object, StoryObjectProcessState *processState)
{
    float posY = Hu3DData[object->mdlId[1]].pos.y;
    int i;
    for (i = 0; i < 60; i++) {
        WaitOneFrame();
        Hu3DData[object->mdlId[1]].pos.y = InterpolateSineEaseIn(posY, 0, i, 60);
        Hu3DData[object->mdlId[2]].pos.y = InterpolateSineEaseIn(posY - 90, -90, i, 60);
    }
    Hu3DModelAttrSet(object->mdlId[2], 1);
    processState->updateCallback = NULL;
}

Void MoveBoardMissPlayer(OMOBJ *object, StoryObjectProcessState *processState)
{
    Vec pos = { 120, 0, 800 };
    boardMissSecondaryKoopaProcess.updateCallback = (Void *)AnimateBoardMissKoopaExit;
    ShiftObjectMotion(boardMissMapProcess.object, 2, 2, 0, 0);
    ShiftObjectMotion(object, 1, 2, 15, 1);
    MoveModelToPosition(object, 1, pos, -60, 30, 10);
    ShiftObjectMotion(object, 1, 1, 15, 1);
    processState->updateCallback = NULL;
}

Void RunBoardMissPlayerProcess(Void)
{
    OMOBJ *object;
    int playerIndex = boardMissState.playerOrderCount++;
    boardMissPlayerProcesses[playerIndex].object = object = omAddObjEx(boardMissProcess, 256, 16, 16, -1, NULL);
    boardMissPlayerProcesses[playerIndex].updateCallback = NULL;
    boardMissPlayerProcesses[playerIndex].object->data = &boardMissPlayerProcesses[0];
    CreateBoardMissPlayerModel(object, playerIndex);
    while (1) {
        WaitOneFrame();
        if (boardMissPlayerProcesses[playerIndex].updateCallback != NULL) {
            boardMissPlayerProcesses[playerIndex].updateCallback(object, &boardMissPlayerProcesses[playerIndex]);
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

Void HandleBoardMissProcessEnd(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    work->cameraCenter.x = 0;
    work->cameraCenter.y = 170;
    work->cameraCenter.z = 0;
    work->cameraRotation.x = -5;
    work->cameraRotation.y = 0;
    work->cameraRotation.z = 0;
    work->cameraZoom = 1700;
}

Void UpdateBoardMissCamera(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    StoryCameraMotionState targetCameraState;
    targetCameraState.cameraCenter.x = 0;
    targetCameraState.cameraCenter.y = 125;
    targetCameraState.cameraCenter.z = 0;
    targetCameraState.cameraRotation.x = -5;
    targetCameraState.cameraRotation.y = 0;
    targetCameraState.cameraRotation.z = 0;
    targetCameraState.cameraZoom = 1750;
    EaseCameraMotionSineIn(work, &targetCameraState, work->animationFrame++, 60, 30);
    if (0.0f != boardMissState.verticalOffset) {
        work->cameraCenter.y = boardMissState.verticalOffset + 125;
    }
}

Void AnimateBoardMissCamera(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    StoryCameraMotionState targetCameraState;
    if (work->animationFrame == 50) {
        ShiftObjectMotion(boardMissMapProcess.object, 5, 5, 0, 0);
        ShiftObjectMotion(boardMissMapProcess.object, 6, 6, 0, 0);
        HuAudFXPlay(161);
    }
    targetCameraState.cameraCenter.x = 0;
    targetCameraState.cameraCenter.y = 100;
    targetCameraState.cameraCenter.z = 460;
    targetCameraState.cameraRotation.x = 0;
    targetCameraState.cameraRotation.y = 0;
    targetCameraState.cameraRotation.z = 0;
    targetCameraState.cameraZoom = 125;
    EaseCameraMotionSineIn(work, &targetCameraState, work->animationFrame, 180, 10);
    if (work->animationFrame++ >= 90) {
        if (work->animationFrame == 91) {
            storySoundEffectIds[0] = HuAudFXPlay(32);
        }
        work->cameraRotation.x = InterpolateLinear(work->cameraRotation.x, -45, work->animationFrame - 90, 90);
        Hu3DData[boardMissMapProcess.object->mdlId[5]].rot.x = InterpolateLinear(0, 45, work->animationFrame - 90, 90);
    }
}
