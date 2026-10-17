#include "REL/mstory2Dll.h"
#include "game/armem.h"
#include "game/board/main.h"

#ifndef __MWERKS__
#include "game/audio.h"
#endif

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
    /* 0x100 */ s32 verticalOffset;
} MinigameMissState; // Size 0x104

MinigameMissState minigameMissState;
StoryObjectProcessState minigameMissPlayerProcess;
StoryObjectProcessState minigameMissKoopaProcess;
StoryObjectProcessState minigameMissCharacterProcess;
StoryObjectProcessState minigameMissMapProcess;
StoryObjectProcessState minigameMissMainProcess;
HUPROCESS *minigameMissProcess;

Void RunMinigameMissMainProcess(Void);
Void RunMinigameMissMapProcess(Void);
Void SetMinigameMissKoopaMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags);
Void RunMinigameMissKoopaProcess(Void);
Void RunMinigameMissCharacterProcess(Void);
Void PrepareMinigameMissPlayers(s32 selectedPlayerIndex);
Void SetMinigameMissPlayerMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags);
Void RunMinigameMissPlayerProcess(Void);
Void UpdateMinigameMissCamera(Void);
Void FinishMinigameMissCamera(Void);

Void StartMinigameMissEvent(HUPROCESS *parentProcess, s32 activePadIndex)
{
    minigameMissProcess = parentProcess;
    minigameMissState.activePadIndex = activePadIndex;
    InitializeWindowSystem();
    CreateStoryDirectionalLight(255);
    InitializeStoryCamera(minigameMissProcess, UpdateMinigameMissCamera);
    CreateStoryShadow();
    PrepareMinigameMissPlayers(activePadIndex);
    HuPrcChildCreate(RunMinigameMissMapProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunMinigameMissKoopaProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunMinigameMissCharacterProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunMinigameMissPlayerProcess, 100, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(RunMinigameMissMainProcess, 100, 8192, 0, HuPrcCurrentGet());
}

Void UpdateMinigameMissObjectMotion(OMOBJ *object)
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

Void DisplayMinigameMissDebugStatus(Void)
{
    int posX = 16;
    int posY = 40;
    GXColor winColor = { 0, 0, 128, 128 };
    int i;
    while (1) {
        WaitOneFrame();
        printWin(posX, posY, 220, 130, &winColor);
        if (minigameMissPlayerProcess.updateCallback == NULL) {
            print8(posX, posY, 1, "PROC_MAIN   -> FALSE");
        }
        else {
            print8(posX, posY, 1, "PROC_MAIN   -> TRUE");
        }
        if (minigameMissKoopaProcess.updateCallback == NULL) {
            print8(posX, posY + 10, 1, "PROC_MAP    -> FALSE");
        }
        else {
            print8(posX, posY + 10, 1, "PROC_MAP    -> TRUE");
        }
        if (minigameMissCharacterProcess.updateCallback == NULL) {
            print8(posX, posY + 20, 1, "PROC_KOOPA  -> FALSE");
        }
        else {
            print8(posX, posY + 20, 1, "PROC_KOOPA  -> TRUE");
        }
        if (minigameMissMapProcess.updateCallback == NULL) {
            print8(posX, posY + 30, 1, "PROC_MKOOPA -> FALSE");
        }
        else {
            print8(posX, posY + 30, 1, "PROC_MKOOPA -> TRUE");
        }
        if (minigameMissMainProcess.updateCallback == NULL) {
            print8(posX, posY + 40, 1, "PROC_PLAYER -> FALSE");
        }
        else {
            print8(posX, posY + 40, 1, "PROC_PLAYER -> TRUE");
        }
        print8(posX, posY + 60, 1, "P:%d W:%d", minigameMissState.selectedPlayerIndex, minigameMissState.activePadIndex);
        print8(posX, posY + 80, 1, "NO) IDX:GRP:COM:DIF:PAD:CHR");
        for (i = 0; i < 4; i++) {
            print8(posX, posY + 90 + (i * 10), 1, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", i, minigameMissState.playerInfo[i].playerIndex, minigameMissState.playerInfo[i].group,
                minigameMissState.playerInfo[i].isComputer, minigameMissState.playerInfo[i].difficulty, minigameMissState.playerInfo[i].padIndex, minigameMissState.playerInfo[i].characterId);
        }
    }
}

Void BeginMinigameMissScene(Void)
{
    storyAudioSequenceIds[0] = HuAudSeqPlay(52);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet());
    WaitFrames(60);
}

Void ShowMinigameMissOpeningMessage(Void)
{
    minigameMissState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, minigameMissState.activePadIndex);
    SetMinigameMissKoopaMotion(2, 1, 15, 0, 1);
    SetMinigameMissPlayerMotion(2, 3, 15, 0, 1);
    CharFXPlay(minigameMissState.playerInfo[minigameMissState.selectedPlayerIndex].characterId, 302);
    WaitForWindowMessageAndMotionSimple(minigameMissState.activeWindowId, 0x1F0010, -1, minigameMissCharacterProcess.object->mdlId[1]);
    SetWindowMessage(minigameMissState.activeWindowId, 0x250004);
    SetMinigameMissKoopaMotion(1, -1, 15, 1, 0);
    CloseCenteredMessageWindow(minigameMissState.activeWindowId);
}

int AskMinigameMissContinueChoice(Void)
{
    int result = 0;
    HuWinInsertMesSet(minigameMissState.activeWindowId, 0x1E003C, 3);
    SetWindowMessage(minigameMissState.activeWindowId, 0x1E002D);
    WaitForWindowMessage(minigameMissState.activeWindowId, 5);
    minigameMissState.eventWindowId = CreateAutoSizedMessageWindow(0x1E0035, minigameMissState.activePadIndex);
    SetWindowMessage(minigameMissState.eventWindowId, 0x1E0035);
    result = GetDefaultWindowChoice(minigameMissState.eventWindowId);
    SetWindowMessage(minigameMissState.eventWindowId, 0x250004);
    CloseAutoSizedMessageWindow(minigameMissState.eventWindowId);
    if (result) {
        HuAudFXPlay(storyAudioCueIds[6][2]);
        SetMinigameMissKoopaMotion(3, 1, 15, 1, 0);
        SetWindowMessage(minigameMissState.activeWindowId, 0x1F0012);
        WaitForWindowMessage(minigameMissState.activeWindowId, -1);
        SetWindowMessage(minigameMissState.activeWindowId, 0x250004);
        SetMinigameMissKoopaMotion(1, -1, 15, 1, 0);
        SetMinigameMissPlayerMotion(4, 1, 15, 0, 1);
    }
    return result;
}

int AskMinigameMissRetryChoice(Void)
{
    int result = 0;
    SetWindowMessage(minigameMissState.activeWindowId, 0x1E0005);
    WaitForWindowMessage(minigameMissState.activeWindowId, 5);
    minigameMissState.eventWindowId = CreateAutoSizedMessageWindow(0x1E0035, minigameMissState.activePadIndex);
    SetWindowMessage(minigameMissState.eventWindowId, 0x1E0035);
    result = GetWindowChoice(minigameMissState.eventWindowId, 1);
    SetWindowMessage(minigameMissState.eventWindowId, 0x250004);
    CloseAutoSizedMessageWindow(minigameMissState.eventWindowId);
    return result;
}

Void FinishMinigameMissChoice(Void)
{
    HuAudSeqFadeOut(storyAudioSequenceIds[0], 3000);
    CaptureCameraMotionStart(&storyCameraState);
    SetCameraUpdateCallback(FinishMinigameMissCamera);
    WaitFrames(150);
}

Void MinigameMissNoOpCallback(Void) { }

Void FinishMinigameMissEvent(int transitionMode)
{
    OMOVLHIS *his;

    if (transitionMode != 0) {
        HuAudSeqFadeOut(storyAudioSequenceIds[0], 1000);
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    do {
        WaitOneFrame();
    } while (WipeStatGet() != 0);
    CharModelKill(-1);
    his = omOvlHisGet(0);
    omOvlHisChg(0, DLL_mstory2dll, 3, his->stat);
    if (transitionMode == 0) {
        HuAudSeqAllFadeOut(1000);
        HuAudSStreamAllFadeOut(1000);
        GWMGAvailSet(450);
        GWSystem.mg_next = 450 - 401;
        omOvlCallEx(DLL_m450dll, 1, 0, 0);
    }
    else {
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

Void RunMinigameMissSequence(OMOBJ *object, StoryObjectProcessState *processState)
{
    int mode = 0;
    BeginMinigameMissScene();
    ShowMinigameMissOpeningMessage();
    minigameMissState.activeWindowId = CreateCenteredMessageWindow(1, 0, 21, 2, minigameMissState.activePadIndex);
    while (1) {
        WaitOneFrame();
        if (AskMinigameMissContinueChoice()) {
            mode = 0;
            break;
        }
        else if (AskMinigameMissRetryChoice()) {
            mode = 1;
            break;
        }
    }
    CloseCenteredMessageWindow(minigameMissState.activeWindowId);
    switch (mode) {
        case 0:
            FinishMinigameMissChoice();
            break;
        case 1:
            break;
    }
    SetMinigameMissPlayerMotion(mode);
    processState->updateCallback = NULL;
}

Void RunMinigameMissMainProcess(Void)
{
    OMOBJ *object;
    minigameMissPlayerProcess.object = object = omAddObjEx(minigameMissProcess, 256, 16, 16, -1, NULL);
    minigameMissPlayerProcess.updateCallback = (Void *)RunMinigameMissSequence;
    minigameMissPlayerProcess.object->data = &minigameMissPlayerProcess;
    while (1) {
        WaitOneFrame();
        if (minigameMissPlayerProcess.updateCallback != NULL) {
            minigameMissPlayerProcess.updateCallback(object, &minigameMissPlayerProcess);
        }
    }
}

Void CreateMinigameMissMapObject(OMOBJ *object)
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
    Hu3DModelShadowSet(object->mdlId[5]);
    ShiftObjectMotion(object, 5, 5, 0, 2);
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, 0x5A));
    object->mtnId[6] = Hu3DMotionIDGet(object->mdlId[6]);
    Hu3DModelHookSet(object->mdlId[5], "koopasgbox_fix-effect_fook2", object->mdlId[6]);
    ShiftObjectMotion(object, 6, 6, 0, 2);
}

Void AnimateMinigameMissMapObject(OMOBJ *object, StoryObjectProcessState *processState)
{
    Hu3DData[object->mdlId[5]].pos.y = InterpolateSineCycle(80, 90, processState->animationFrame++, 360);
    if (processState->animationFrame >= 360) {
        processState->animationFrame = 0;
    }
}

Void RunMinigameMissMapProcess(Void)
{
    OMOBJ *object;
    minigameMissKoopaProcess.object = object = omAddObjEx(minigameMissProcess, 256, 16, 16, -1, NULL);
    minigameMissKoopaProcess.updateCallback = (Void *)AnimateMinigameMissMapObject;
    minigameMissKoopaProcess.object->data = &minigameMissKoopaProcess;
    CreateMinigameMissMapObject(object);
    while (1) {
        WaitOneFrame();
        if (minigameMissKoopaProcess.updateCallback != NULL) {
            minigameMissKoopaProcess.updateCallback(object, &minigameMissKoopaProcess);
        }
    }
}

Void CreateMinigameMissKoopaObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x62 : 0x66));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x63 : 0x67));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x66 : 0x6A));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x69 : 0x6C));
    Hu3DModelPosSet(object->mdlId[1], 0, 0, 200);
    Hu3DModelRotSet(object->mdlId[1], 0, 0, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1.6f, 1.6f, 1.6f);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 1, 0, 1);
}

Void SetMinigameMissKoopaMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(minigameMissCharacterProcess.object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    minigameMissCharacterProcess.object->work[0] = motionFlags;
    minigameMissCharacterProcess.object->work[1] = motionSlot;
    minigameMissCharacterProcess.object->work[2] = blendFrames + 1;
}

Void RunMinigameMissKoopaProcess(Void)
{
    OMOBJ *object;
    minigameMissCharacterProcess.object = object = omAddObjEx(minigameMissProcess, 256, 16, 16, -1, NULL);
    minigameMissCharacterProcess.updateCallback = NULL;
    minigameMissCharacterProcess.object->data = &minigameMissKoopaProcess;
    CreateMinigameMissKoopaObject(object);
    while (1) {
        WaitOneFrame();
        if (minigameMissCharacterProcess.updateCallback != NULL) {
            minigameMissCharacterProcess.updateCallback(object, &minigameMissCharacterProcess);
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

Void CreateMinigameMissCharacterObject(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x7D : 0x7B));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MSTORY2, VERSION_NTSC ? 0x7E : 0x7C));
    Hu3DModelPosSet(object->mdlId[1], -120, 0, 800);
    Hu3DModelRotSet(object->mdlId[1], 0, 135, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 1, 0, 1);
}

Void SetMinigameMissCharacterMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(minigameMissMapProcess.object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    minigameMissMapProcess.object->work[0] = motionFlags;
    minigameMissMapProcess.object->work[1] = motionSlot;
    minigameMissMapProcess.object->work[2] = blendFrames + 1;
}

Void RunMinigameMissCharacterProcess(Void)
{
    OMOBJ *object;
    minigameMissMapProcess.object = object = omAddObjEx(minigameMissProcess, 256, 16, 16, -1, NULL);
    minigameMissMapProcess.updateCallback = NULL;
    minigameMissMapProcess.object->data = &minigameMissMapProcess;
    CreateMinigameMissCharacterObject(object);
    while (1) {
        WaitOneFrame();
        if (minigameMissMapProcess.updateCallback != NULL) {
            minigameMissMapProcess.updateCallback(object, &minigameMissMapProcess);
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

Void PrepareMinigameMissPlayers(s32 selectedPlayerIndex)
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
            minigameMissState.selectedPlayerIndex = i;
        }
    }
    for (i = 0; i < 4; i++) {
        ReadStoryPlayerConfig(&minigameMissState.playerInfo[i], playerOrder[i]);
    }
}

Void CreateMinigameMissPlayerModel(OMOBJ *object)
{
    int characterId = minigameMissState.playerInfo[minigameMissState.selectedPlayerIndex].characterId;
    object->mdlId[1] = CharModelCreate(characterId, 1);
    object->mtnId[1] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00));
    object->mtnId[2] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18));
    object->mtnId[3] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x34));
    object->mtnId[4] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x48));
    CharMotionVoiceOnSet(characterId, object->mtnId[2], FALSE);
    CharMotionDataClose(characterId);
    Hu3DModelPosSet(object->mdlId[1], 120, 0, 800);
    Hu3DModelRotSet(object->mdlId[1], 0, -135, 0);
    Hu3DModelScaleSet(object->mdlId[1], 1, 1, 1);
    Hu3DModelShadowSet(object->mdlId[1]);
    ShiftObjectMotion(object, 1, 1, 0, 1);
}

Void SetMinigameMissPlayerMotion(int motionIndex, int motionSlot, int blendFrames, int loopMode, int motionFlags)
{
    ShiftObjectMotion(minigameMissMainProcess.object, 1, motionIndex, blendFrames, loopMode);
    if (loopMode == 1) {
        motionFlags = 0;
    }
    minigameMissMainProcess.object->work[0] = motionFlags;
    minigameMissMainProcess.object->work[1] = motionSlot;
    minigameMissMainProcess.object->work[2] = blendFrames + 1;
}

Void RunMinigameMissPlayerProcess(Void)
{
    OMOBJ *object;
    minigameMissMainProcess.object = object = omAddObjEx(minigameMissProcess, 256, 16, 16, -1, NULL);
    minigameMissMainProcess.updateCallback = NULL;
    minigameMissMainProcess.object->data = &minigameMissMainProcess;
    CreateMinigameMissPlayerModel(object);
    while (1) {
        WaitOneFrame();
        if (minigameMissMainProcess.updateCallback != NULL) {
            minigameMissMainProcess.updateCallback(object, &minigameMissMainProcess);
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

Void UpdateMinigameMissCamera(Void)
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

Void FinishMinigameMissCamera(Void)
{
    StoryCameraMotionState *work = &storyCameraState;
    StoryCameraMotionState targetCameraState;
    if (work->animationFrame == 50) {
        ShiftObjectMotion(minigameMissKoopaProcess.object, 5, 5, 0, 0);
        ShiftObjectMotion(minigameMissKoopaProcess.object, 6, 6, 0, 0);
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
        Hu3DData[minigameMissKoopaProcess.object->mdlId[5]].rot.x = InterpolateLinear(0, 45, work->animationFrame - 90, 90);
    }
}
