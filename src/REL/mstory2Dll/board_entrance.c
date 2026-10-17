#include "REL/mstory2Dll.h"
#include "game/armem.h"
#include "game/audio.h"
#include "game/board/main.h"
#include "game/chrman.h"
#include "game/data.h"
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
} BoardEntranceState; // Size 0x104

BoardEntranceState boardEntranceState;
StoryObjectProcessState boardEntranceMainProcess;
StoryObjectProcessState boardEntranceMapProcess;
StoryObjectProcessState boardEntrancePlayerProcess;
StoryObjectProcessState boardEntranceSubProcess;
StoryObjectProcessState boardEntranceKoopaProcess;
HUPROCESS *boardEntranceProcess;
s32 boardEntranceMessageIds[16];
s32 boardEntranceSequenceIds[4];

u32 boardEntranceWindowFlags[11]
    = { 0x00250000, 0x00250000, 0x00250000, 0x00250000, 0x00250000, 0x00250000, 0x00250000, 0x00250000, 0x00250000, 0x00250000, 0x00250000 };

s32 boardEntranceMotionCueFrames[2][2] = { 20, 70, 1, 1 };

s32 boardEntranceMotionId = -1;
s32 boardEntranceCueMotionIds[2] = { -1, -1 };

Void RunBoardEntranceMainProcess(Void);
Void RunBoardEntranceMapProcess(Void);
Void PrepareBoardEntrancePlayers(s32 selectedPlayerIndex);
Void AnimateBoardEntrancePlayer(OMOBJ *object, StoryObjectProcessState *processState);
Void RunBoardEntrancePlayerProcess(Void);
Void AnimateBoardEntranceProp(OMOBJ *object, StoryObjectProcessState *processState);
Void RunBoardEntrancePropProcess(Void);
Void AnimateBoardEntranceKoopa(OMOBJ *object, StoryObjectProcessState *processState);

Void RunBoardEntranceKoopaProcess(Void);
Void HandleBoardEntranceProcessEnd(Void);
Void UpdateBoardEntranceCamera(Void);

Void WaitForBoardEntranceMotionCues(Void)
{
    int i;
    while (1) {
        do {
            HuPrcVSleep();
        } while (boardEntranceMotionId == -1 || boardEntranceCueMotionIds[0] == -1 || boardEntranceCueMotionIds[1] == -1 || boardEntranceCueMotionIds[0] != Hu3DMotionIDGet(boardEntranceMotionId));
        for (i = 0; i < 2; i++) {
            if (boardEntranceMotionCueFrames[1][i] != -1) {
                if (Hu3DMotionTimeGet(boardEntranceMotionId) >= boardEntranceMotionCueFrames[0][i]) {
                    OSReport("KE-System  TIME:%d\n", boardEntranceMotionCueFrames[0][i]);
                    HuAudFXPlay(81);
                    boardEntranceMotionCueFrames[1][i] = -1;
                }
            }
        }
        if (Hu3DMotionTimeGet(boardEntranceMotionId) <= 10) {
            for (i = 0; i < 2; i++) {
                boardEntranceMotionCueFrames[1][i] = 1;
            }
        }
    }
}

Void StartBoardEntranceEvent(HUPROCESS *parentProcess, s32 activePadIndex)
{
    boardEntranceProcess = parentProcess;
    boardEntranceState.activePadIndex = activePadIndex;
    InitializeWindowSystem();
    CreateStoryDirectionalLight(255);
    InitializeStoryCamera(boardEntranceProcess, HandleBoardEntranceProcessEnd);
    CreateStoryShadow();
    PrepareBoardEntrancePlayers(activePadIndex);
    HuPrcChildCreate(RunBoardEntrancePlayerProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunBoardEntranceMapProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunBoardEntrancePropProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunBoardEntranceKoopaProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunBoardEntranceMainProcess, 100, 8192, 0, HuPrcCurrentGet());
}

Void UpdateBoardEntranceMotion(OMOBJ *object)
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

Void DisplayBoardEntranceDebugStatus(Void)
{
    int posX = 16;
    int posY = 40;
    GXColor winColor = { 0, 0, 128, 128 };
    int i;
    while (1) {
        WaitOneFrame();
        fontcolor = FONT_COLOR_WHITE;
        printWin(posX, posY, 220, 130, &winColor);
        if (boardEntranceMainProcess.updateCallback == NULL) {
            print8(posX, posY, 1, "PROC_MAIN   -> FALSE");
        }
        else {
            print8(posX, posY, 1, "PROC_MAIN   -> TRUE");
        }
        if (boardEntranceMapProcess.updateCallback == NULL) {
            print8(posX, posY + 10, 1, "PROC_MAP    -> FALSE");
        }
        else {
            print8(posX, posY + 10, 1, "PROC_MAP    -> TRUE");
        }
        if (boardEntrancePlayerProcess.updateCallback == NULL) {
            print8(posX, posY + 20, 1, "PROC_PLAYER -> FALSE");
        }
        else {
            print8(posX, posY + 20, 1, "PROC_PLAYER -> TRUE");
        }
        if (boardEntranceSubProcess.updateCallback == NULL) {
            print8(posX, posY + 30, 1, "PROC_SUB    -> FALSE");
        }
        else {
            print8(posX, posY + 30, 1, "PROC_SUB    -> TRUE");
        }
        if (boardEntranceKoopaProcess.updateCallback == NULL) {
            print8(posX, posY + 40, 1, "PROC_KOOPA  -> FALSE");
        }
        else {
            print8(posX, posY + 40, 1, "PROC_KOOPA  -> TRUE");
        }

        print8(posX, posY + 60, 1, "P:%d W:%d", boardEntranceState.selectedPlayerIndex, boardEntranceState.activePadIndex);
        print8(posX, posY + 80, 1, "NO) IDX:GRP:COM:DIF:PAD:CHR");
        for (i = 0; i < 4; i++) {
            print8(posX, posY + 90 + (i * 10), 1, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", i, boardEntranceState.playerInfo[i].playerIndex, boardEntranceState.playerInfo[i].group,
                boardEntranceState.playerInfo[i].isComputer, boardEntranceState.playerInfo[i].difficulty, boardEntranceState.playerInfo[i].padIndex, boardEntranceState.playerInfo[i].characterId);
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_X) {
            AnimateCharacterMeshOffset(boardEntrancePlayerProcess.object->mdlId[1], boardEntranceState.playerInfo[boardEntranceState.selectedPlayerIndex].characterId, 30, 120, TRUE);
        }
        else if (HuPadBtnDown[0] & PAD_BUTTON_Y) {
            AnimateCharacterMeshOffset(boardEntrancePlayerProcess.object->mdlId[1], boardEntranceState.playerInfo[boardEntranceState.selectedPlayerIndex].characterId, 30, 120, FALSE);
        }
    }
}

Void BeginBoardEntranceScene(Void)
{
    boardEntranceSequenceIds[0] = HuAudSeqPlay(51);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet());
    WaitFrames(60);
}

Void PlayBoardEntranceIntroduction(Void)
{
    ShiftObjectMotion(boardEntranceMapProcess.object, 2, 3, 0, 0);
    WaitForObjectMotionEnd(boardEntranceMapProcess.object, 2, 3, 0);
    boardEntrancePlayerProcess.updateCallback = (Void *)AnimateBoardEntrancePlayer;
    boardEntranceSubProcess.updateCallback = (Void *)AnimateBoardEntranceProp;
    CaptureCameraMotionStart(&storyCameraState);
    SetCameraUpdateCallback(UpdateBoardEntranceCamera);
    WaitFrames(30);
    ShiftObjectMotion(boardEntranceMapProcess.object, 2, 2, 0, 0);
    do {
        WaitOneFrame();
    } while (boardEntrancePlayerProcess.updateCallback != NULL);
    do {
        WaitOneFrame();
    } while (boardEntranceSubProcess.updateCallback != NULL);
    boardEntranceState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, boardEntranceState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[0][2]);
    ShiftObjectMotion(boardEntranceSubProcess.object, 1, 3, 5, 1);
    SetWindowMessage(boardEntranceState.activeWindowId, 0x1F0000);
    WaitForWindowMessage(boardEntranceState.activeWindowId, -1);
    SetWindowMessage(boardEntranceState.activeWindowId, 0x1F0001);
    WaitForWindowMessage(boardEntranceState.activeWindowId, -1);
    SetWindowMessage(boardEntranceState.activeWindowId, 0x250004);
    ShiftObjectMotion(boardEntranceSubProcess.object, 1, 1, 5, 1);
    CloseCenteredMessageWindow(boardEntranceState.activeWindowId);
    WaitFrames(15);
}

Void AnimateBoardEntranceArrival(Void)
{
    int i;
    float cameraMoveSpeed;
    HuAudSeqFadeOut(boardEntranceSequenceIds[0], 500);
    HuAudFXPlay(153);
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 2, 0, 0);
    WaitOneFrame();
    Hu3DMotionTimeSet(boardEntranceKoopaProcess.object->mdlId[1], 100);
    Hu3DModelAttrReset(boardEntranceKoopaProcess.object->mdlId[1], 1);
    while (1) {
        WaitOneFrame();
        if (Hu3DMotionTimeGet(boardEntranceKoopaProcess.object->mdlId[1]) >= 132) {
            break;
        }
    }
    ShiftObjectMotion(boardEntranceSubProcess.object, 1, 4, 0, 0);
    WaitOneFrame();
    while (1) {
        WaitOneFrame();
        if (Hu3DMotionTimeGet(boardEntranceSubProcess.object->mdlId[1]) >= 5) {
            break;
        }
    }
    Hu3DMotionSpeedSet(boardEntranceSubProcess.object->mdlId[1], 0);
    Hu3DData[boardEntranceSubProcess.object->mdlId[1]].rot.y = 0;
    SetObjectMotionTransition(boardEntrancePlayerProcess.object, 3, 1, 5, 0);
    HuAudFXPlay(156);
    omVibrate(boardEntranceState.activePadIndex, 45, 12, 0);
    cameraMoveSpeed = 50;
    for (i = 0; i < 60; i++) {
        float cameraOffsetStep = InterpolateSineCycle(0, cameraMoveSpeed, i % 15, 15);
        cameraMoveSpeed = InterpolateWeightedAverage(cameraMoveSpeed, 0, 30);
        boardEntranceState.verticalOffset = InterpolateWeightedAverage(boardEntranceState.verticalOffset, cameraOffsetStep, 5);
        WaitOneFrame();
    }
    boardEntranceState.verticalOffset = 0;
}

Void ShowBoardEntranceWelcome(Void)
{
    AnimateCharacterMeshOffset(boardEntrancePlayerProcess.object->mdlId[1], boardEntranceState.playerInfo[boardEntranceState.selectedPlayerIndex].characterId, 1, 20, 1);
    boardEntranceSequenceIds[1] = HuAudSeqPlay(6);
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 3, 10, 0);
    WaitFrames(30);
    boardEntranceState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, boardEntranceState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[6][0]);
    HuWinAttrSet(boardEntranceState.activeWindowId, 0x400);
    HuWinMesSet(boardEntranceState.activeWindowId, 0x1F0002);
    WaitForObjectMotionEnd(boardEntranceKoopaProcess.object, 1, 3, 10);
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 3, 10, 0);
    WaitFrames(45);
    HuAudFXPlay(storyAudioCueIds[6][0]);
    WaitForObjectMotionEnd(boardEntranceKoopaProcess.object, 1, 3, 10);
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 1, 10, 1);
    HuWinAttrReset(boardEntranceState.activeWindowId, 0x400);
    HuWinKeyWaitEntry(boardEntranceState.activeWindowId);
    WaitOneFrame();
    WaitForWindowMessage(boardEntranceState.activeWindowId, -1);
    HuAudFXPlay(storyAudioCueIds[6][2]);
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 4, 10, 1);
    SetWindowMessage(boardEntranceState.activeWindowId, 0x1F0003);
    WaitForWindowMessage(boardEntranceState.activeWindowId, -1);
    SetWindowMessage(boardEntranceState.activeWindowId, 0x250004);
    CloseCenteredMessageWindow(boardEntranceState.activeWindowId);
    WaitFrames(30);
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 1, 30, 1);
    WaitFrames(60);
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 5, 10, 0);
    WaitFrames(25);
    HuAudFXPlay(81);
    WaitFrames(20);
    boardEntranceState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, boardEntranceState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[6][2]);
    InsertWindowMessage(boardEntranceState.activeWindowId, boardEntranceState.playerInfo[boardEntranceState.selectedPlayerIndex].characterId, 0);
    SetWindowMessage(boardEntranceState.activeWindowId, 0x1F0004);
    WaitForWindowMessage(boardEntranceState.activeWindowId, -1);
    SetWindowMessage(boardEntranceState.activeWindowId, 0x1F0005);
    WaitForWindowMessage(boardEntranceState.activeWindowId, -1);
    SetWindowMessage(boardEntranceState.activeWindowId, 0x250004);
    CloseCenteredMessageWindow(boardEntranceState.activeWindowId);
    SetObjectMotionTransition(boardEntrancePlayerProcess.object, 7, 8, 10, 0);
    AnimateCharacterMeshOffset(boardEntrancePlayerProcess.object->mdlId[1], boardEntranceState.playerInfo[boardEntranceState.selectedPlayerIndex].characterId, 3, 5, 0);
    WaitFrames(90);
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 3, 30, 0);
    WaitFrames(30);
    boardEntranceState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, boardEntranceState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[6][0]);
    HuWinAttrSet(boardEntranceState.activeWindowId, 0x400);
    HuWinMesSet(boardEntranceState.activeWindowId, 0x1F0002);
    WaitForObjectMotionEnd(boardEntranceKoopaProcess.object, 1, 3, 10);
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 1, 10, 1);
    HuWinAttrReset(boardEntranceState.activeWindowId, 0x400);
    HuWinKeyWaitEntry(boardEntranceState.activeWindowId);
    WaitOneFrame();
    WaitForWindowMessage(boardEntranceState.activeWindowId, -1);
    SetWindowMessage(boardEntranceState.activeWindowId, 0x250004);
    CloseCenteredMessageWindow(boardEntranceState.activeWindowId);
}

Void ShowBoardEntranceNextMessage(Void)
{
    WaitFrames(60);
    boardEntranceKoopaProcess.updateCallback = (Void *)AnimateBoardEntranceKoopa;
    boardEntranceState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, boardEntranceState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[6][1]);
    SetWindowMessage(boardEntranceState.activeWindowId, 0x1F0007);
    WaitForWindowMessage(boardEntranceState.activeWindowId, -1);
    CloseCenteredMessageWindow(boardEntranceState.activeWindowId);
    while (1) {
        WaitOneFrame();
        if (boardEntranceKoopaProcess.updateCallback == NULL) {
            break;
        }
    }
    HuAudFXPlay(149);
    Hu3DMotionSpeedSet(boardEntranceSubProcess.object->mdlId[1], 1);
    WaitOneFrame();
    WaitForObjectMotionEnd(boardEntranceSubProcess.object, 1, 4, 0);
    boardEntranceMessageIds[0] = HuAudFXPlay(98);
    HuAudFXPlay(storyAudioCueIds[0][1]);
    ShiftObjectMotion(boardEntranceSubProcess.object, 1, 5, 15, 1);
    WaitFrames(60);
}

Void FinishBoardEntranceSequence(Void)
{
    WaitFrames(60);
    HuAudSeqFadeOut(boardEntranceSequenceIds[0], 1000);
    HuAudFXFadeOut(boardEntranceMessageIds[0], 1000);
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (1) {
        WaitOneFrame();
        if (!WipeStatGet()) {
            break;
        }
    }
    CharModelKill(-1);
    HuAudFadeOut(1);
    GWPlayerCfg[0].group = 0;
    GWPlayerCfg[0].character = boardEntranceState.playerInfo[boardEntranceState.selectedPlayerIndex].characterId;
    GWPlayerCfg[0].pad_idx = boardEntranceState.playerInfo[boardEntranceState.selectedPlayerIndex].padIndex;
    GWPlayerCfg[0].iscom = FALSE;
    omOvlGotoEx(DLL_mentdll, 1, 11, 0);
}

Void RunBoardEntranceSequence(OMOBJ *object, StoryObjectProcessState *processState)
{
    int unusedInliningGuard = 0;
    BeginBoardEntranceScene();
    PlayBoardEntranceIntroduction();
    AnimateBoardEntranceArrival();
    ShowBoardEntranceWelcome();
    ShowBoardEntranceNextMessage();
    FinishBoardEntranceSequence();
    processState->updateCallback = NULL;
}

Void RunBoardEntranceMainProcess(Void)
{
    OMOBJ *object;
    boardEntranceMainProcess.object = object = omAddObjEx(boardEntranceProcess, 256, 16, 16, -1, NULL);
    boardEntranceMainProcess.updateCallback = (Void *)RunBoardEntranceSequence;
    boardEntranceMainProcess.object->data = &boardEntranceMainProcess;
    while (1) {
        WaitOneFrame();
        if (boardEntranceMainProcess.updateCallback != NULL) {
            boardEntranceMainProcess.updateCallback(object, &boardEntranceMainProcess);
        }
    }
}

Void CreateBoardEntranceMapObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x50));
    object->mtnId[0] = Hu3DMotionIDGet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 0, 0, 2);
    Hu3DModelShadowMapObjSet(object->mdlId[1], "base_fix9-base");
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x53));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x54));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x55));
    ShiftObjectMotion(object, 2, 3, 0, 2);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x51));
}

Void AnimateBoardEntranceMapObject(OMOBJ *object, StoryObjectProcessState *processState)
{
    Hu3DData[object->mdlId[5]].pos.y = InterpolateSineCycle(125, 130, processState->animationFrame++, 360);
    if (processState->animationFrame >= 360) {
        processState->animationFrame = 0;
    }
}

Void RunBoardEntranceMapProcess(Void)
{
    OMOBJ *object;
    boardEntranceMapProcess.object = object = omAddObjEx(boardEntranceProcess, 256, 16, 16, -1, NULL);
    boardEntranceMapProcess.updateCallback = (Void *)AnimateBoardEntranceMapObject;
    boardEntranceMapProcess.object->data = &boardEntranceMapProcess;
    CreateBoardEntranceMapObject(object);
    while (1) {
        WaitOneFrame();
        if (boardEntranceMapProcess.updateCallback != NULL) {
            boardEntranceMapProcess.updateCallback(object, &boardEntranceMapProcess);
        }
    }
}

Void PrepareBoardEntrancePlayers(s32 selectedPlayerIndex)
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
            boardEntranceState.selectedPlayerIndex = i;
        }
    }
    for (i = 0; i < 4; i++) {
        ReadStoryPlayerConfig(&boardEntranceState.playerInfo[i], playerOrder[i]);
    }
}

Void CreateBoardEntrancePlayerModel(OMOBJ *object)
{
    int characterId = boardEntranceState.playerInfo[boardEntranceState.selectedPlayerIndex].characterId;
    object->mdlId[1] = CharModelCreate(characterId, 1);
    object->mtnId[1] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00));
    object->mtnId[2] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x35));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x18) + characterId);
    object->mtnId[7] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x48) + characterId);
    object->mtnId[8] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x40) + characterId);
    object->mtnId[9] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x20) + characterId);
    object->mtnId[10] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x12));
    CharMotionDataClose(characterId);
    Hu3DModelPosSet(object->mdlId[1], 120, 0, 800);
    Hu3DModelRotSet(object->mdlId[1], 0, -135, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 1, 0, 1);
}

Void AnimateBoardEntrancePlayer(OMOBJ *object, StoryObjectProcessState *processState)
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

Void RunBoardEntrancePlayerProcess(Void)
{
    OMOBJ *object;
    boardEntrancePlayerProcess.object = object = omAddObjEx(boardEntranceProcess, 256, 16, 16, -1, NULL);
    boardEntrancePlayerProcess.updateCallback = NULL;
    boardEntrancePlayerProcess.object->data = &boardEntrancePlayerProcess;
    CreateBoardEntrancePlayerModel(object);
    while (1) {
        WaitOneFrame();
        if (boardEntrancePlayerProcess.updateCallback != NULL) {
            boardEntrancePlayerProcess.updateCallback(object, &boardEntrancePlayerProcess);
        }
        if (object->work[0]) {
            UpdateObjectMotionTransition(object);
        }
    }
}

Void CreateBoardEntranceProp(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x72 : 0x73));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x73 : 0x74));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x75));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x76));
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x7A : 0x78));
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x7B : 0x79));
    object->mtnId[6] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x79 : 0x77));
    Hu3DModelPosSet(object->mdlId[1], 0, 0, 460);
    Hu3DModelRotSet(object->mdlId[1], 0, 0, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 6, 0, 1);
    CharNpcDustSet(object->mdlId[1], object->mtnId[2], 1, 9);
}

Void AnimateBoardEntranceProp(OMOBJ *object, StoryObjectProcessState *processState)
{
    Vec pos = { -120, 0, 800 };
    ShiftObjectMotion(object, 1, 2, 15, 1);
    MoveModelToPosition(object, 1, pos, 90, 45, 10);
    ShiftObjectMotion(object, 1, 1, 15, 1);
    processState->updateCallback = NULL;
}

Void RunBoardEntrancePropProcess(Void)
{
    OMOBJ *object;
    boardEntranceSubProcess.object = object = omAddObjEx(boardEntranceProcess, 256, 16, 16, -1, NULL);
    boardEntranceSubProcess.updateCallback = NULL;
    boardEntranceSubProcess.object->data = &boardEntranceSubProcess;
    CreateBoardEntranceProp(object);
    while (1) {
        WaitOneFrame();
        if (boardEntranceSubProcess.updateCallback != NULL) {
            boardEntranceSubProcess.updateCallback(object, &boardEntranceSubProcess);
        }
        if (object->work[0]) {
            UpdateObjectMotionTransition(object);
        }
    }
}

Void CreateBoardEntranceKoopa(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x62 : 0x66));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x63 : 0x67));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x65 : 0x69));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x66 : 0x6A));
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x69 : 0x6C));
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x68 : 0x6B));
    object->mtnId[6] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x6C : 0x6D));
    object->mtnId[7] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x6D : 0x6E));
    object->mtnId[8] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x64 : 0x68));
    Hu3DModelPosSet(object->mdlId[1], -120, 0, 800);
    Hu3DModelRotSet(object->mdlId[1], 0, 90, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelAttrSet(object->mdlId[1], 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 1, 0, 1);
    boardEntranceMotionId = object->mdlId[1];
    boardEntranceCueMotionIds[0] = object->mtnId[7];
    boardEntranceCueMotionIds[1] = object->mtnId[8];
    HuPrcChildCreate(WaitForBoardEntranceMotionCues, 100, 8192, 0, HuPrcCurrentGet());
}

Void AnimateBoardEntranceKoopa(OMOBJ *object, StoryObjectProcessState *processState)
{
    int i;
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 8, 15, 1);
    for (i = 0; i < 30; i++) {
        WaitOneFrame();
        Hu3DData[boardEntranceKoopaProcess.object->mdlId[1]].pos.x = InterpolateLinear(-120, -60, i, 30);
    }
    ShiftObjectMotion(boardEntrancePlayerProcess.object, 1, 9, 15, 0);
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 6, 15, 0);
    WaitForObjectMotionEnd(boardEntranceKoopaProcess.object, 1, 6, 15);
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 7, 15, 1);
    ShiftObjectMotion(boardEntrancePlayerProcess.object, 1, 10, 15, 1);
    for (i = 0; i < 240; i++) {
        WaitOneFrame();
        Hu3DData[boardEntranceKoopaProcess.object->mdlId[1]].pos.x = InterpolateLinear(-60, 540, i, 180);
        Hu3DData[boardEntrancePlayerProcess.object->mdlId[1]].pos.x = InterpolateLinear(120, 840, i, 180);
    }
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, 1, 15, 1);
    processState->updateCallback = NULL;
}

Void SetBoardEntranceKoopaMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(boardEntranceKoopaProcess.object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    boardEntranceKoopaProcess.object->work[0] = motionFlags;
    boardEntranceKoopaProcess.object->work[1] = motionSlot;
    boardEntranceKoopaProcess.object->work[2] = blendFrames + 1;
}

Void RunBoardEntranceKoopaProcess(Void)
{
    OMOBJ *object;
    boardEntranceKoopaProcess.object = object = omAddObjEx(boardEntranceProcess, 256, 16, 16, -1, NULL);
    boardEntranceKoopaProcess.updateCallback = NULL;
    boardEntranceKoopaProcess.object->data = &boardEntranceKoopaProcess;
    CreateBoardEntranceKoopa(object);
    while (1) {
        WaitOneFrame();
        if (boardEntranceKoopaProcess.updateCallback != NULL) {
            boardEntranceKoopaProcess.updateCallback(object, &boardEntranceKoopaProcess);
        }
        if (object->work[0]) {
            UpdateObjectMotionTransition(object);
        }
    }
}

Void HandleBoardEntranceProcessEnd(Void)
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

Void UpdateBoardEntranceCamera(Void)
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
    if (0.0f != boardEntranceState.verticalOffset) {
        work->cameraCenter.y = 100 + boardEntranceState.verticalOffset;
    }
}
