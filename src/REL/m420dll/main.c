#include "ext_math.h"
#include "game/flag.h"
#include "game/frand.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/wipe.h"

#include "REL/m420dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/audio.h"
#endif

s32 InitializeMinigame(void);
void CleanupMinigame(void);
void RunMinigameSequence(void);
void AnimateScreenWipe(s32 presetIndex, s32 durationFrames);
s32 InitializeCamera(void);
void ShutdownCamera(void);
void AnimateCameraTarget(const Vec *targetPosition, s32 durationFrames);
void AnimateCameraAngles(const Vec *targetAngles, s32 durationFrames);
void AnimateCameraZoom(float targetDistance, s32 durationFrames);
void WaitForMinigameExit(void);

s32 unreferencedStateCounter;
HUPROCESS *minigameSequenceProcess;
M420DllPlayerStruct *minigamePlayers[4];
s16 timerSequenceId;
s16 timerFrameCount;
s16 messageSequenceId;
s32 minigamePhase;
s32 minigameAudioSequenceId;
HUPROCESS *minigameObjectManager;
HUPROCESS *minigameExitProcess;

static const M420DllCameraPose cameraPresetPoses[3] = {
    { { 0.0f, 1040.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 680.0f },
    { { 0.0f, 800.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, 680.0f },
    { { 0.0f, 310.0f, 0.0f }, { -10.0f, 0.0f, 0.0f }, 1950.0f },
};

void ObjectSetup(void)
{
    minigameObjectManager = omInitObjMan(1, 0x2000);
    omGameSysInit(minigameObjectManager);
    minigameExitProcess = HuPrcChildCreate(WaitForMinigameExit, 0x3E8, 0x4000, 0, minigameObjectManager);
    InitializeMinigame();
}

void WaitForMinigameExit(void)
{
    while (omSysExitReq == 0) {
        HuPrcVSleep();
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);

    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    HuAudFadeOut(0x64);
    CleanupMinigame();
    HuPrcVSleep();
    omOvlReturnEx(1, 1);
    HuPrcEnd();
}

s32 InitializeMinigame(void)
{
    s32 selectedPlayerSlots[3];
    s32 playerIndex;
    s32 stateValue;

    Vec positionVector[3] = { { 0.0f, 4000.0f, 100.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    frand();
    InitializeRandomGenerator();
    InitializeMapSystem();
    InitializeCamera();
    AnimateScreenWipe(0, 1);
    Hu3DShadowCreate(30.0f, 20.0f, 10000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowPosSet(&positionVector[0], &positionVector[1], &positionVector[2]);
    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        selectedPlayerSlots[playerIndex] = 0;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        switch (GWPlayerCfg[playerIndex].group) {
            case 0:
                minigamePlayers[0] = CreatePlayer(playerIndex, 0);
                break;

            default:
                do {
                    stateValue = getRandomInteger() % 3;
                } while (selectedPlayerSlots[stateValue] != 0);
                selectedPlayerSlots[stateValue] = 1;
                minigamePlayers[stateValue + 1] = CreatePlayer(playerIndex, stateValue + 1);
                break;
        }
    }
    minigamePlayers[1]->linkedPlayer = NULL;
    minigamePlayers[2]->linkedPlayer = minigamePlayers[1];
    minigamePlayers[3]->linkedPlayer = minigamePlayers[2];
    minigameSequenceProcess = HuPrcChildCreate(RunMinigameSequence, 0x3E9, 0x4000, 0, minigameObjectManager);
    return 1;
}

void CleanupMinigame(void)
{
    s32 playerIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        DestroyPlayer(minigamePlayers[playerIndex]);
    }
    ShutdownMapSystem();
    ShutdownCamera();
    HuPrcKill(minigameSequenceProcess);
}

void RunMinigameSequence(void)
{
    s32 secondaryPlayerIndex;
    s32 playerIndex;

    minigamePhase = 0;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 0x3C);

    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    StartMapEvent(0);
    AnimateScreenWipe(1, 0x50);
    HuPrcSleep(0x4E);
    AnimateScreenWipe(2, 0x3C);
    HuPrcSleep(0xB4);
    StartMapEvent(2);
    for (secondaryPlayerIndex = 0; secondaryPlayerIndex < 4; secondaryPlayerIndex++) {
        minigamePlayers[secondaryPlayerIndex]->actionState = 9;
    }
    minigameAudioSequenceId = HuAudSeqPlay(0x48);
    messageSequenceId = MGSeqCreate(3, 0);

    while (MGSeqStatGet(messageSequenceId) != 0) {
        HuPrcVSleep();
    }
    MGSeqKill(messageSequenceId);
    timerFrameCount = 60 * REFRESH_RATE;
    timerSequenceId = MGSeqCreate(1, timerFrameCount / REFRESH_RATE, -1, -1);
    playerIndex = 1;
    while (TRUE) {
        if (minigamePlayers[0]->actionState == 0) {
            minigamePlayers[0]->actionState = 1;
        }
        if ((minigamePlayers[playerIndex]->actionState == 0) && (timerFrameCount > 5)) {
            minigamePlayers[playerIndex]->actionState = 3;
        }
        if ((minigamePlayers[playerIndex]->itemHookAttached) && (playerIndex != 1)) {
            minigamePlayers[playerIndex - 1]->actionState = 4;
        }
        if (minigamePlayers[playerIndex]->actionState == 6) {
            playerIndex++;
            if (playerIndex > 3) {
                playerIndex = 1;
                minigamePlayers[3]->actionState = 0;
            }
        }
        if (timerFrameCount >= 0) {
            if (((--timerFrameCount) % REFRESH_RATE) == 0) {
                MGSeqParamSet(timerSequenceId, 1, timerFrameCount / REFRESH_RATE);
            }
        }
        if ((getMapEffectFlags(0) == 0) && (getMapEffectFlags(1) == 0) && (timerFrameCount > 0)) {
            HuPrcVSleep();
        }
        else {
            break;
        }
    }
    if ((getMapEffectFlags(0) != 0) && (getMapEffectFlags(1) != 0)) {
        minigamePhase = 3;
    }
    else if (getMapEffectFlags(0) != 0) {
        minigamePhase = 1;
    }
    else if (getMapEffectFlags(1) != 0) {
        minigamePhase = 2;
    }
    else {
        minigamePhase = 3;
    }
    for (secondaryPlayerIndex = 0; secondaryPlayerIndex < 4; secondaryPlayerIndex++) {
        minigamePlayers[secondaryPlayerIndex]->actionState = 0xB;
    }
    HuPrcVSleep();
    MGSeqParamSet(timerSequenceId, 2, -1);
    HuAudSeqFadeOut(minigameAudioSequenceId, 0x64);
    messageSequenceId = MGSeqCreate(3, 1);

    while (MGSeqStatGet(messageSequenceId) != 0) {
        HuPrcVSleep();
    }
    MGSeqKill(messageSequenceId);
    HideMapMarker();
    HuPrcSleep(0x3C);
    for (secondaryPlayerIndex = 0; secondaryPlayerIndex < 4; secondaryPlayerIndex++) {
        minigamePlayers[secondaryPlayerIndex]->actionState = 0xA;
    }
    HuPrcSleep(0x3C);
    if (minigamePhase == 3) {
        messageSequenceId = MGSeqCreate(3, 2);
        HuAudSStreamPlay(4);
        minigamePlayers[0]->actionState = 8;
        minigamePlayers[1]->actionState = 8;
        minigamePlayers[2]->actionState = 8;
        minigamePlayers[3]->actionState = 8;
    }
    else {
        StartMapEvent(1);
        while (getMapRoundIndex() == 0) {
            HuPrcVSleep();
        }
        if (minigamePhase == 1) {
            messageSequenceId = MGSeqCreate(5, 3, minigamePlayers[0]->characterId, -1, -1, -1);
            HuAudSStreamPlay(1);
            minigamePlayers[0]->actionState = 7;
            minigamePlayers[1]->actionState = 8;
            minigamePlayers[2]->actionState = 8;
            minigamePlayers[3]->actionState = 8;
            GWPlayerCoinWinAdd(minigamePlayers[0]->playerIndex, 10);
        }
        else {
            messageSequenceId = MGSeqCreate(5, 3, minigamePlayers[1]->characterId, minigamePlayers[2]->characterId, minigamePlayers[3]->characterId, -1);
            HuAudSStreamPlay(1);
            minigamePlayers[0]->actionState = 8;
            minigamePlayers[1]->actionState = 7;
            minigamePlayers[2]->actionState = 7;
            minigamePlayers[3]->actionState = 7;
            GWPlayerCoinWinAdd(minigamePlayers[1]->playerIndex, 10);
            GWPlayerCoinWinAdd(minigamePlayers[2]->playerIndex, 10);
            GWPlayerCoinWinAdd(minigamePlayers[3]->playerIndex, 10);
        }
    }
    HuPrcSleep(0xD2);
    omSysExitReq = 1;
    while (TRUE) {
        HuPrcVSleep();
    }
}

void AnimateScreenWipe(s32 presetIndex, s32 durationFrames)
{
    AnimateCameraTarget(&cameraPresetPoses[presetIndex].lookAtPosition, durationFrames);
    AnimateCameraAngles(&cameraPresetPoses[presetIndex].cameraAngles, durationFrames);
    AnimateCameraZoom(cameraPresetPoses[presetIndex].cameraDistance, durationFrames);
}
