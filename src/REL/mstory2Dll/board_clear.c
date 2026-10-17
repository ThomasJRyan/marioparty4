#include "REL/mstory2Dll.h"
#include "game/armem.h"
#include "game/audio.h"
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
} BoardClearState; // Size 0x104

BoardClearState boardClearState;
StoryObjectProcessState boardClearMainProcess;
StoryObjectProcessState boardClearMapProcess;
StoryObjectProcessState boardClearKoopaProcess;
StoryObjectProcessState boardClearSubProcess;
StoryObjectProcessState boardClearPlayerProcesses[4];
HUPROCESS *boardClearProcess;

Void RunBoardClearMainProcess(Void);
Void RunBoardClearMapProcess(Void);
Void SetBoardClearPlayerMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags);
Void RunBoardClearPlayerProcess(Void);
Void SetBoardClearCharacterMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags);
Void AnimateBoardClearCharacter(OMOBJ *object, StoryObjectProcessState *processState);
Void RunBoardClearCharacterProcess(Void);
Void PrepareBoardClearPlayers(int selectedPlayerIndex);
Void SetBoardClearSelectedPlayerMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags);
Void AnimateBoardClearSelectedPlayer(OMOBJ *object, StoryObjectProcessState *processState);
Void RunBoardClearSelectedPlayerProcess(Void);
Void HandleBoardClearProcessEnd(Void);
Void UpdateBoardClearCamera(Void);
Void AnimateBoardClearCamera(Void);

Void UpdateBoardClearObjectMotion(OMOBJ *object, StoryObjectProcessState *processState)
{
    WaitFrames(30);
    SetBoardClearSelectedPlayerMotion(4, -1, 15, 1, 0);
    SetBoardClearCharacterMotion(4, -1, 15, 1, 0);
    processState->updateCallback = NULL;
}

Void StartBoardClearEvent(HUPROCESS *parentProcess, s32 activePadIndex)
{
    int i;
    boardClearProcess = parentProcess;
    boardClearState.activePadIndex = activePadIndex;
    InitializeWindowSystem();
    CreateStoryDirectionalLight(255);
    InitializeStoryCamera(boardClearProcess, HandleBoardClearProcessEnd);
    CreateStoryShadow();
    PrepareBoardClearPlayers(activePadIndex);
    HuPrcChildCreate(RunBoardClearMapProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunBoardClearPlayerProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunBoardClearCharacterProcess, 100, 8192, 0, HuPrcCurrentGet());
    for (i = 0; i < 4; i++) {
        HuPrcChildCreate(RunBoardClearSelectedPlayerProcess, 100, 8192, 0, HuPrcCurrentGet());
    }
    HuPrcChildCreate(RunBoardClearMainProcess, 100, 8192, 0, HuPrcCurrentGet());
}

Void UpdateBoardClearCharacterMotion(OMOBJ *object)
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

Void DisplayBoardClearDebugStatus(Void)
{
    int posX = 16;
    int posY = 40;
    GXColor winColor = { 0, 0, 128, 128 };
    int i;
    while (1) {
        WaitOneFrame();
        printWin(posX, posY, 220, 160, &winColor);
        if (boardClearMainProcess.updateCallback == NULL) {
            print8(posX, posY, 1, "PROC_MAIN    -> FALSE");
        }
        else {
            print8(posX, posY, 1, "PROC_MAIN    -> TRUE");
        }
        if (boardClearMapProcess.updateCallback == NULL) {
            print8(posX, posY + 10, 1, "PROC_MAP     -> FALSE");
        }
        else {
            print8(posX, posY + 10, 1, "PROC_MAP     -> TRUE");
        }
        if (boardClearKoopaProcess.updateCallback == NULL) {
            print8(posX, posY + 20, 1, "PROC_KOOPA   -> FALSE");
        }
        else {
            print8(posX, posY + 20, 1, "PROC_KOOPA   -> TRUE");
        }
        if (boardClearSubProcess.updateCallback == NULL) {
            print8(posX, posY + 30, 1, "PROC_MKOOPA  -> FALSE");
        }
        else {
            print8(posX, posY + 30, 1, "PROC_MKOOPA  -> TRUE");
        }
        for (i = 0; i < 4; i++) {
            if (boardClearPlayerProcesses[i].updateCallback == NULL) {
                print8(posX, posY + 40 + (i * 10), 1, "PROC_PLAYER%d -> FALSE", i);
            }
            else {
                print8(posX, posY + 40 + (i * 10), 1, "PROC_PLAYER%d -> TRUE", i);
            }
        }
        print8(posX, posY + 90, 1, "P:%d W:%d", boardClearState.selectedPlayerIndex, boardClearState.activePadIndex);
        print8(posX, posY + 110, 1, "NO) IDX:GRP:COM:DIF:PAD:CHR");
        for (i = 0; i < 4; i++) {
            print8(posX, posY + 120 + (i * 10), 1, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", i, boardClearState.playerInfo[i].playerIndex, boardClearState.playerInfo[i].group,
                boardClearState.playerInfo[i].isComputer, boardClearState.playerInfo[i].difficulty, boardClearState.playerInfo[i].padIndex, boardClearState.playerInfo[i].characterId);
        }
    }
}

Void BeginBoardClearScene(Void)
{
    storyAudioSequenceIds[0] = HuAudSeqPlay(51);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet());
    WaitFrames(60);
}

Void WaitForBoardClearPlayerMotion(Void)
{
    boardClearPlayerProcesses[boardClearState.selectedPlayerIndex].updateCallback = (Void *)AnimateBoardClearSelectedPlayer;
    CaptureCameraMotionStart(&storyCameraState);
    SetCameraUpdateCallback(UpdateBoardClearCamera);
    while (1) {
        WaitOneFrame();
        if (boardClearPlayerProcesses[boardClearState.selectedPlayerIndex].updateCallback == NULL) {
            break;
        }
    }
    while (1) {
        WaitOneFrame();
        if (boardClearSubProcess.updateCallback == NULL) {
            break;
        }
    }
    WaitFrames(30);
}

Void ShowBoardClearPlayerMessage(Void)
{
    int i;
    boardClearState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, boardClearState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[5][0]);
    SetBoardClearCharacterMotion(3, -1, 15, 1, 0);
    InsertWindowMessage(boardClearState.activeWindowId, boardClearState.playerInfo[boardClearState.selectedPlayerIndex].characterId, 0);
    SetWindowMessage(boardClearState.activeWindowId, 0x1F000C);
    WaitForWindowMessage(boardClearState.activeWindowId, -1);
    SetWindowMessage(boardClearState.activeWindowId, 0x250004);
    SetBoardClearCharacterMotion(1, -1, 15, 1, 0);
    CloseCenteredMessageWindow(boardClearState.activeWindowId);
    HuAudSeqFadeOut(storyAudioSequenceIds[0], 1000);
    WaitFrames(60);
    for (i = 0; i < 4; i++) {
        if (boardClearState.playerInfo[i].playerIndex != boardClearState.playerInfo[boardClearState.selectedPlayerIndex].playerIndex) {
            Hu3DModelAttrSet(boardClearPlayerProcesses[i].object->mdlId[1], 1);
        }
        Hu3DModelAttrSet(boardClearPlayerProcesses[i].object->mdlId[2], 1);
    }
    Hu3DModelAttrReset(boardClearKoopaProcess.object->mdlId[1], 1);
    Hu3DModelAttrReset(boardClearMapProcess.object->mdlId[5], 1);
    Hu3DModelAttrReset(boardClearMapProcess.object->mdlId[6], 1);
    boardClearState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, boardClearState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[6][0]);
    storyAudioSequenceIds[1] = HuAudSeqPlay(55);
    boardClearSubProcess.updateCallback = (Void *)UpdateBoardClearObjectMotion;
    SetWindowMessage(boardClearState.activeWindowId, 0x1F000D);
    WaitForWindowMessage(boardClearState.activeWindowId, -1);
    SetWindowMessage(boardClearState.activeWindowId, 0x250004);
    CloseCenteredMessageWindow(boardClearState.activeWindowId);
    ShiftObjectMotion(boardClearMapProcess.object, 2, 3, 0, 0);
    WaitFrames(60);
    boardClearSubProcess.updateCallback = (Void *)AnimateBoardClearCharacter;
    SetBoardClearSelectedPlayerMotion(5, -1, 15, 0, 0);
    SetBoardClearCharacterMotion(5, 1, 0, 0, 1);
    WaitForObjectMotionEnd(boardClearMapProcess.object, 2, 3, 0);
}

Void PlayBoardClearCharacterAnimation(Void)
{
    boardClearState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, boardClearState.activePadIndex);
    HuAudFXPlay(storyAudioCueIds[6][1]);
    SetBoardClearPlayerMotion(3, -1, 15, 1, 0);
    SetWindowMessage(boardClearState.activeWindowId, 0x1F000E);
    WaitForWindowMessage(boardClearState.activeWindowId, -1);
    SetWindowMessage(boardClearState.activeWindowId, 0x250004);
    SetBoardClearPlayerMotion(1, -1, 15, 1, 0);
    CloseCenteredMessageWindow(boardClearState.activeWindowId);
    SetBoardClearPlayerMotion(2, -1, 15, 0, 0);
    WaitFrames(45);
    HuAudFXPlay(storyAudioCueIds[6][0]);
    WaitFrames(15);
    HuAudSeqFadeOut(storyAudioSequenceIds[1], 3000);
    CaptureCameraMotionStart(&storyCameraState);
    SetCameraUpdateCallback(AnimateBoardClearCamera);
    WaitFrames(30);
    WaitFrames(45);
    WaitFrames(75);
}

Void SwapBoardClearPlayerConfiguration(Void)
{
    int i;
    OMOVLHIS *his;
    int characterId;
    int padNo;
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (1) {
        WaitOneFrame();
        if (!WipeStatGet()) {
            break;
        }
    }
    CharModelKill(-1);
    characterId = boardClearState.playerInfo[boardClearState.selectedPlayerIndex].characterId;
    padNo = boardClearState.playerInfo[boardClearState.selectedPlayerIndex].padIndex;
    GWPlayerCfg[boardClearState.selectedPlayerIndex].character = GWPlayerCfg[0].character;
    GWPlayerCfg[boardClearState.selectedPlayerIndex].pad_idx = GWPlayerCfg[0].pad_idx;
    GWPlayerCfg[boardClearState.selectedPlayerIndex].group = GWPlayerCfg[0].group;
    GWPlayerCfg[boardClearState.selectedPlayerIndex].iscom = GWPlayerCfg[0].iscom;
    GWPlayerCfg[0].character = characterId;
    GWPlayerCfg[0].pad_idx = padNo;
    GWPlayerCfg[0].group = 0;
    GWPlayerCfg[0].iscom = FALSE;
    for (i = 0; i < 4; i++) {
        GWPlayer[i].character = GWPlayerCfg[i].character;
        GWPlayer[i].port = GWPlayerCfg[i].pad_idx;
        GWPlayer[i].com = GWPlayerCfg[i].iscom;
        GWPlayer[i].team = GWPlayerCfg[i].group;
    }
    GWPlayerCfg[1].character = 8;
    for (i = 1; i < 4; i++) {
        GWPlayerCfg[i].iscom = GWPlayerCfg[i].group = 1;
    }
    his = omOvlHisGet(0);
    omOvlHisChg(0, DLL_mstory2dll, 3, his->stat);
    HuAudSeqAllFadeOut(1000);
    HuAudSStreamAllFadeOut(1000);
    GWSystem.mg_type = -1;
    GWMGAvailSet(450);
    GWSystem.mg_next = 450 - 401;
    omOvlCallEx(DLL_m450dll, 1, 0, 0);
    while (1) {
        WaitOneFrame();
    }
}

Void RunBoardClearSequence(OMOBJ *object, StoryObjectProcessState *processState)
{
    int unusedInliningGuard = 0;
    BeginBoardClearScene();
    WaitForBoardClearPlayerMotion();
    ShowBoardClearPlayerMessage();
    PlayBoardClearCharacterAnimation();
    SwapBoardClearPlayerConfiguration();
    processState->updateCallback = NULL;
}

Void RunBoardClearMainProcess(Void)
{
    OMOBJ *object;
    boardClearMainProcess.object = object = omAddObjEx(boardClearProcess, 256, 16, 16, -1, NULL);
    boardClearMainProcess.updateCallback = (Void *)RunBoardClearSequence;
    boardClearMainProcess.object->data = &boardClearMainProcess;
    while (1) {
        WaitOneFrame();
        if (boardClearMainProcess.updateCallback != NULL) {
            boardClearMainProcess.updateCallback(object, &boardClearMainProcess);
        }
    }
}

Void CreateBoardClearMapObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x52));
    object->mtnId[1] = Hu3DMotionIDGet(object->mdlId[1]);
    Hu3DModelShadowMapObjSet(object->mdlId[1], "base_story-base");
    ShiftObjectMotion(object, 1, 1, 0, 1);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x53));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x54));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY2, 0x55));
    ShiftObjectMotion(object, 2, 2, 0, 2);
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x59));
    object->mtnId[5] = Hu3DMotionIDGet(object->mdlId[5]);
    Hu3DModelPosSet(object->mdlId[5], 0, 80, 460);
    Hu3DModelAttrSet(object->mdlId[5], 1);
    Hu3DModelShadowSet(object->mdlId[5]);
    ShiftObjectMotion(object, 5, 5, 0, 2);
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x5A));
    object->mtnId[6] = Hu3DMotionIDGet(object->mdlId[6]);
    Hu3DModelHookSet(object->mdlId[5], "koopasgbox_fix-effect_fook2", object->mdlId[6]);
    Hu3DModelAttrSet(object->mdlId[6], 1);
    ShiftObjectMotion(object, 6, 6, 0, 2);
}

Void AnimateBoardClearMapObject(OMOBJ *object, StoryObjectProcessState *processState)
{
    Hu3DData[object->mdlId[5]].pos.y = InterpolateSineCycle(80, 90, processState->animationFrame++, 360);
    if (processState->animationFrame >= 360) {
        processState->animationFrame = 0;
    }
}

Void RunBoardClearMapProcess(Void)
{
    OMOBJ *object;
    boardClearMapProcess.object = object = omAddObjEx(boardClearProcess, 256, 16, 16, -1, NULL);
    boardClearMapProcess.updateCallback = (Void *)AnimateBoardClearMapObject;
    boardClearMapProcess.object->data = &boardClearMapProcess;
    CreateBoardClearMapObject(object);
    while (1) {
        WaitOneFrame();
        if (boardClearMapProcess.updateCallback != NULL) {
            boardClearMapProcess.updateCallback(object, &boardClearMapProcess);
        }
    }
}

Void CreateBoardClearPlayerObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x62 : 0x66));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x63 : 0x67));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x66 : 0x6A));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x69 : 0x6C));
    Hu3DModelPosSet(object->mdlId[1], 0, 0, 150);
    Hu3DModelRotSet(object->mdlId[1], 0, 0, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1.25f, 1.25f, 1.25f);
    Hu3DModelScaleSet(object->mdlId[1], 1.6f, 1.6f, 1.6f);
    Hu3DModelAttrSet(object->mdlId[1], 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 1, 0, 1);
}

Void SetBoardClearPlayerMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(boardClearKoopaProcess.object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    boardClearKoopaProcess.object->work[0] = motionFlags;
    boardClearKoopaProcess.object->work[1] = motionSlot;
    boardClearKoopaProcess.object->work[2] = blendFrames + 1;
}

Void RunBoardClearPlayerProcess(Void)
{
    OMOBJ *object;
    boardClearKoopaProcess.object = object = omAddObjEx(boardClearProcess, 256, 16, 16, -1, NULL);
    boardClearKoopaProcess.updateCallback = NULL;
    boardClearKoopaProcess.object->data = &boardClearMapProcess;
    CreateBoardClearPlayerObject(object);
    while (1) {
        WaitOneFrame();
        if (boardClearKoopaProcess.updateCallback != NULL) {
            boardClearKoopaProcess.updateCallback(object, &boardClearKoopaProcess);
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

Void CreateBoardClearCharacterObject(OMOBJ *object)
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

Void SetBoardClearCharacterMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(boardClearSubProcess.object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    boardClearSubProcess.object->work[0] = motionFlags;
    boardClearSubProcess.object->work[1] = motionSlot;
    boardClearSubProcess.object->work[2] = blendFrames + 1;
}

Void AnimateBoardClearCharacter(OMOBJ *object, StoryObjectProcessState *processState)
{
    int i;
    for (i = 0; i < 15; i++) {
        WaitOneFrame();
        Hu3DData[object->mdlId[1]].rot.y = InterpolateLinear(0, 135, i, 15);
    }
    processState->updateCallback = NULL;
}

Void MoveBoardClearCharacter(OMOBJ *object, StoryObjectProcessState *processState)
{
    Vec pos = { -120, 0, 800 };
    ShiftObjectMotion(object, 1, 2, 15, 1);
    MoveModelToPosition(object, 1, pos, 60, 60, 10);
    ShiftObjectMotion(object, 1, 1, 15, 1);
    processState->updateCallback = NULL;
}

Void RunBoardClearCharacterProcess(Void)
{
    OMOBJ *object;
    boardClearSubProcess.object = object = omAddObjEx(boardClearProcess, 256, 16, 16, -1, NULL);
    boardClearSubProcess.updateCallback = NULL;
    boardClearSubProcess.object->data = &boardClearSubProcess;
    CreateBoardClearCharacterObject(object);
    while (1) {
        WaitOneFrame();
        if (boardClearSubProcess.updateCallback != NULL) {
            boardClearSubProcess.updateCallback(object, &boardClearSubProcess);
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

Void PrepareBoardClearPlayers(int selectedPlayerIndex)
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
            boardClearState.selectedPlayerIndex = i;
        }
    }
    for (i = 0; i < 4; i++) {
        ReadStoryPlayerConfig(&boardClearState.playerInfo[i], playerOrder[i]);
    }
    for (i = 0; i < 4; i++) {
        boardClearState.playerInfo[i].rank = GWPlayer[playerOrder[i]].rank + 1;
    }
}

Vec boardClearPlayerPositions[4] = {
    { -300, 0, 460 },
    { -100, -30, 460 },
    { 100, -60, 460 },
    { 300, -90, 460 },
};

Void CreateBoardClearPlayerModel(OMOBJ *object, int player)
{
    int playerIndex = player;
    int characterId = boardClearState.playerInfo[playerIndex].characterId;
    object->mdlId[1] = CharModelCreate(characterId, 4);
    object->mtnId[1] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00));
    object->mtnId[2] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03));
    object->mtnId[3] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x05));
    object->mtnId[4] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x1A));
    object->mtnId[5] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x6D));
    CharMotionDataClose(characterId);
    Hu3DModelPosSet(
        object->mdlId[1], boardClearPlayerPositions[playerIndex].x, boardClearPlayerPositions[boardClearState.playerInfo[playerIndex].rank - 1].y + 90, boardClearPlayerPositions[playerIndex].z);
    Hu3DModelRotSet(object->mdlId[1], 0, 0, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 1, 0, 1);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x56));
    Hu3DModelPosSet(
        object->mdlId[2], boardClearPlayerPositions[playerIndex].x, boardClearPlayerPositions[boardClearState.playerInfo[playerIndex].rank - 1].y, boardClearPlayerPositions[playerIndex].z);
    if (boardClearState.playerInfo[playerIndex].rank == 4) {
        Hu3DModelAttrSet(object->mdlId[2], 1);
    }
    Hu3DModelScaleSet(object->mdlId[2], 1.5, 1, 1.5);
    Hu3DModelShadowMapSet(object->mdlId[2]);
}

Void SetBoardClearSelectedPlayerMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(boardClearPlayerProcesses[boardClearState.selectedPlayerIndex].object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    boardClearPlayerProcesses[boardClearState.selectedPlayerIndex].object->work[0] = motionFlags;
    boardClearPlayerProcesses[boardClearState.selectedPlayerIndex].object->work[1] = motionSlot;
    boardClearPlayerProcesses[boardClearState.selectedPlayerIndex].object->work[2] = blendFrames + 1;
}

Void AnimateBoardClearSelectedPlayer(OMOBJ *object, StoryObjectProcessState *processState)
{
    Vec pos = { 120, 0, 800 };
    MoveModelWithJump(object, 1, pos, -1, 30, 10);
    boardClearSubProcess.updateCallback = (Void *)MoveBoardClearCharacter;
    ShiftObjectMotion(boardClearMapProcess.object, 2, 2, 0, 0);
    ShiftObjectMotion(object, 1, 2, 15, 1);
    MoveModelToPosition(object, 1, pos, -60, 30, 10);
    ShiftObjectMotion(object, 1, 1, 15, 1);
    processState->updateCallback = NULL;
}

Void RunBoardClearSelectedPlayerProcess(Void)
{
    OMOBJ *object;
    int playerIndex = boardClearState.playerOrderCount++;
    boardClearPlayerProcesses[playerIndex].object = object = omAddObjEx(boardClearProcess, 256, 16, 16, -1, NULL);
    boardClearPlayerProcesses[playerIndex].updateCallback = NULL;
    boardClearPlayerProcesses[playerIndex].object->data = &boardClearPlayerProcesses[0];
    CreateBoardClearPlayerModel(object, playerIndex);
    while (1) {
        WaitOneFrame();
        if (boardClearPlayerProcesses[playerIndex].updateCallback != NULL) {
            boardClearPlayerProcesses[playerIndex].updateCallback(object, &boardClearPlayerProcesses[playerIndex]);
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

Void HandleBoardClearProcessEnd(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    work->cameraCenter.x = 0;
    work->cameraCenter.y = 170;
    work->cameraCenter.z = -20;
    work->cameraRotation.x = -5;
    work->cameraRotation.y = 0;
    work->cameraRotation.z = 0;
    work->cameraZoom = 1700;
}

Void UpdateBoardClearCamera(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    StoryCameraMotionState targetCameraState;
    targetCameraState.cameraCenter.x = 0;
    targetCameraState.cameraCenter.y = 125;
    targetCameraState.cameraCenter.z = -20;
    targetCameraState.cameraRotation.x = -5;
    targetCameraState.cameraRotation.y = 0;
    targetCameraState.cameraRotation.z = 0;
    targetCameraState.cameraZoom = 1750;
    EaseCameraMotionSineIn(work, &targetCameraState, work->animationFrame++, 60, 30);
}

Void AnimateBoardClearCamera(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    StoryCameraMotionState targetCameraState;
    if (work->animationFrame == 50) {
        ShiftObjectMotion(boardClearMapProcess.object, 5, 5, 0, 0);
        ShiftObjectMotion(boardClearMapProcess.object, 6, 6, 0, 0);
        HuAudFXPlay(161);
    }
    targetCameraState.cameraCenter.x = 0;
    targetCameraState.cameraCenter.y = 130;
    targetCameraState.cameraCenter.z = 460;
    targetCameraState.cameraRotation.x = 0;
    targetCameraState.cameraRotation.y = 0;
    targetCameraState.cameraRotation.z = 0;
    targetCameraState.cameraZoom = 150;
    EaseCameraMotionSineIn(work, &targetCameraState, work->animationFrame, 180, 10);
    if (work->animationFrame++ >= 90) {
        if (work->animationFrame == 91) {
            storySoundEffectIds[0] = HuAudFXPlay(32);
        }
        work->cameraRotation.x = InterpolateLinear(work->cameraRotation.x, -45, work->animationFrame - 90, 90);
        Hu3DData[boardClearMapProcess.object->mdlId[5]].rot.x = InterpolateLinear(0, 45, work->animationFrame - 90, 90);
    }
}
