#include "game/object.h"
#include "game/gamework_data.h"
#include "game/gamework.h"
#include "game/minigame_seq.h"
#include "game/wipe.h"
#include "game/audio.h"
#include "game/hu3d.h"

#include "REL/m407dll.h"

typedef struct MinigameSequenceWork {
/* 0x00 */ u32 sequenceIndex;
/* 0x04 */ s16 sequenceState;
/* 0x06 */ s16 primarySequenceId;
/* 0x08 */ s16 secondarySequenceId;
/* 0x0A */ char padding0A[6];
/* 0x10 */ s16 timerFrames;
/* 0x12 */ char padding12[10];
/* 0x1C */ s16 winningPlayerIds[4];
/* 0x24 */ s16 winnerCount;
/* 0x26 */ char padding26[2];
/* 0x28 */ Vec cameraStart;
/* 0x34 */ Vec cameraTarget;
/* 0x40 */ f32 cameraDuration;
/* 0x44 */ s32 audioSequenceId;
} MinigameSequenceWork; //sizeof 0x48

//function signatures
void FinishMinigameSequence(void);
void SetMinigameSequenceState(s32);
void UpdateMinigameSequence(OMOBJ*);
void SequenceStateIntro(void);
void SequenceStateCountdown(void);
void SequenceStateStart(void);
void SequenceStatePlay(void);
void SequenceStateWhompRound(void);
void SequenceStateResults(void);
void SequenceStateAward(void);
void SequenceStateCameraPan(void);
void SequenceStateWinnerReact(void);
void SequenceStateScoreSummary(void);
s16 GetSequenceState(void);
void SequenceStateFinish(void);
void SequenceStateCleanup(void);

//bss
HUPROCESS* minigameProcess;
OMOBJ* minigameSequenceObject;
s16 minigameSequenceState;

//data
VoidFuncs minigameSequenceCallbacks[] = {
    SequenceStateIntro,
    SequenceStateCountdown,
    SequenceStateStart,
    SequenceStatePlay,
    SequenceStateWhompRound,
    SequenceStateResults,
    SequenceStateAward,
    SequenceStateCameraPan,
    SequenceStateWinnerReact,
    SequenceStateScoreSummary,
    SequenceStateFinish,
    SequenceStateCleanup,
    FinishMinigameSequence
};

f32 sequenceCameraDurations[4] = {850.0f, 900.0f, 1300.0f, 1500.0f};
Vec sequenceCameraPosition1 = {540.0f, 0.0f, 10000.0f};
Vec sequenceCameraTarget1 = {-50.0f, -360.0f, 0.0f};
f32 sequenceCameraZoom1 = 2670.0f;
Vec sequenceCameraPosition2 = {437.0f, 0.0f, 5000.0f};
Vec sequenceCameraTarget2 = {-50.0f, -360.0f, 0.0f};
f32 sequenceCameraZoom2 = 2290.0f;
Vec sequenceCameraPosition3 = {0.0f, 0.0f, 100.0f};
Vec sequenceCameraTarget3 = {-45.0f, -540.0f, 0.0f};
f32 sequenceCameraZoom3 = 1910.0f;
Vec sequenceCameraPosition4 = {0.0f, 0.0f, 100.0f};
Vec sequenceCameraTarget4 = {-53.0f, -680.0f, 0.0f};
f32 sequenceCameraZoom4 = 1900.0f;

void InitializeMinigameSequence(HUPROCESS* process) {
    s32 i;
    MinigameSequenceWork* workData;

    minigameProcess = process;
    minigameSequenceObject = omAddObjEx(minigameProcess, 0x40, 0, 0, 0, UpdateMinigameSequence);
    minigameSequenceObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(MinigameSequenceWork), HU_MEMNUM_OVL);
    workData = minigameSequenceObject->data;
    Hu3DShadowCreate(45.0f, 10.0f, 10000.0f);
    Hu3DShadowTPLvlSet(0.6f);
    InitializeCameraSystem(minigameProcess);
    minigameSequenceState = GWMGRecordGet(1);
    
    if (minigameSequenceState == 0) {
        minigameSequenceState = 0x96;
    }
    
    InitializeMapScene(minigameProcess, minigameSequenceState);
    InitializePlayerSystem(minigameProcess);
    InitializeWhompSystem(minigameProcess);
    InitializeWhompScoreSystem(minigameProcess);
    InitializeEffectSystem(minigameProcess);
    InitializeScoreDisplay(minigameProcess, minigameSequenceState);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    workData->primarySequenceId = -1;
    workData->secondarySequenceId = -1;
    workData->winnerCount = 0;
    
    for (i = 0; i < ARRAY_COUNT(workData->winningPlayerIds); i++) {
        workData->winningPlayerIds[i] = -1;
    }
    
    SetScoreDisplayMode(0);
    SetMinigameSequenceState(0);
}

void FinishMinigameSequence(void) {
    MinigameSequenceWork* workData;

    workData = minigameSequenceObject->data;
    if (workData->sequenceState == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        workData->sequenceState++;
    }
    if (WipeStatGet() == 0) {
        HuAudFadeOut(100);
        
        if (workData->primarySequenceId != -1) {
            MGSeqKill(workData->primarySequenceId);
        }
        
        if (workData->secondarySequenceId != -1) {
            MGSeqKill(workData->secondarySequenceId);
        }
        
        PlayPlayerIdleMotion();
        ResetCameraTransition();
        GetMapSceneState();
        ResetWhompScoreDisplay();
        ResetEffectPool();
        ResetScoreDisplay();
        omOvlReturnEx(1, 1);
    }
}

void SetMinigameSequenceState(s32 sequenceState) {
    MinigameSequenceWork* workData;

    workData = minigameSequenceObject->data;
    workData->sequenceIndex = sequenceState;
    workData->sequenceState = 0;
}

void UpdateMinigameSequence(OMOBJ* obj) {
    MinigameSequenceWork* sequenceWork;
    
    sequenceWork = minigameSequenceObject->data;
    minigameSequenceCallbacks[sequenceWork->sequenceIndex]();

    if (omSysExitReq != 0) {
        SetMinigameSequenceState(12);
        omSysExitReq = 0;
    }
}

void SequenceStateIntro(void) {
    MinigameSequenceWork* sequenceWork;

    sequenceWork = minigameSequenceObject->data;
    SetMinigameSequenceState(1);
}

void SequenceStateCountdown(void) {
    MinigameSequenceWork* workData;

    workData = minigameSequenceObject->data;
    switch (workData->sequenceState) {
    case 0:
        HuAudFXPlay(0x545);
        StartCameraTransition(&sequenceCameraPosition1, &sequenceCameraTarget1, &sequenceCameraZoom1, 0, 0);
        workData->sequenceState = 1;
        break;
    case 1:
        StartCameraTransition(&sequenceCameraPosition2, &sequenceCameraTarget2, &sequenceCameraZoom2, 0x64, 1);
        workData->sequenceState = 2;
        break;
    case 2:
        if (isCameraTransitionActive() == 0) {
            StartCameraTransition(&sequenceCameraPosition3, &sequenceCameraTarget3, &sequenceCameraZoom3, 0x6E, 2);
            workData->sequenceState = 3;
        case 3:
            if (isCameraTransitionActive() == 0) {
                SetMinigameSequenceState(2);
                return;
            }
        }
    }
}

void SequenceStateStart(void) {
    s32 i;
    MinigameSequenceWork* workData;

    workData = minigameSequenceObject->data;
    if (workData->primarySequenceId == -1) {
        workData->primarySequenceId = MGSeqStartCreate();
        workData->audioSequenceId = HuAudSeqPlay(0x46);
        ClearWhompScores();
        for (i = 0; i < 4; i++) {
            VibratePlayerController(i, 1, 0x3C);
        }
    }
    if (MGSeqStatGet(workData->primarySequenceId) == 0) {
        MGSeqKill(workData->primarySequenceId);
        workData->primarySequenceId = -1;
        workData->timerFrames = REFRESH_RATE*10;
        workData->secondarySequenceId = MGSeqTimerCreate(workData->timerFrames / REFRESH_RATE);
        PlayPlayerWalkMotion();
        SetMinigameSequenceState(3);
    }
}

void SequenceStatePlay(void) {
    MinigameSequenceWork* workData;
    
    workData = minigameSequenceObject->data;
    if (workData->timerFrames > 0) {
        if ((--workData->timerFrames % REFRESH_RATE) == 0) {
            MGSeqParamSet(workData->secondarySequenceId, 1, (workData->timerFrames / REFRESH_RATE));
        }
        return;
    }
    
    MGSeqParamSet(workData->secondarySequenceId, 2, -1);
    workData->secondarySequenceId = -1;
    HuAudSeqFadeOut(workData->audioSequenceId, 0x64);
    PlayPlayerTurnMotion();
    GetSequenceState();
    
    if (workData->winnerCount > 0) {
        SetMinigameSequenceState(4);
        return;
    }

    SetMinigameSequenceState(6);
}

void SequenceStateWhompRound(void) {
    MinigameSequenceWork* workData;

    workData = minigameSequenceObject->data;
    switch (workData->sequenceState++) {
    case 0xA:
        StartCameraTransition(&sequenceCameraPosition4, &sequenceCameraTarget4, &sequenceCameraZoom4, 0xB4, 4);
        return;
    case 0x32:
        PlayPlayerReactionMotion();
        return;
    case 0x78:
        PlayPlayerJumpMotion();
        return;
    case 0x9A:
        PlayPlayerLandMotion();
        return;
    case 0xBE:
        workData->audioSequenceId = HuAudSeqPlay(0x3A);
        ResetWhompCounts();
        SetMinigameSequenceState(5);
        return;
    }
}

void SequenceStateResults(void) {
    Vec cameraCenter;
    MinigameSequenceWork* sequenceWork;

    sequenceWork = minigameSequenceObject->data;
    if ((sequenceWork->sequenceState++ >= 15) || (getWhompScoreTotal() != 0)) {
        if (getWhompScoreTotal() == 0) {
            GetCameraCenter(&cameraCenter.z, &cameraCenter.y, &cameraCenter.x);
            cameraCenter.x += 33.5f;
            SetCameraCenter(cameraCenter.z, cameraCenter.y, cameraCenter.x);
            return;
        }
        SetMinigameSequenceState(6);
    }
}

void SequenceStateAward(void) {
    MinigameSequenceWork* workData;

    workData = minigameSequenceObject->data;
    if (workData->sequenceState == 0) {
        workData->primarySequenceId = MGSeqFinishCreate();
        workData->sequenceState++;
        HuAudSeqFadeOut(workData->audioSequenceId, 100);
    }
    if (MGSeqStatGet(workData->primarySequenceId) == 0) {
        MGSeqKill(workData->primarySequenceId);
        workData->primarySequenceId = -1;

        if (workData->secondarySequenceId != 0) {
            MGSeqKill(workData->secondarySequenceId);
            workData->secondarySequenceId = -1;
        }
        
        if (workData->winnerCount > 0) {
            SetMinigameSequenceState(7);
            return;
        }
        SetMinigameSequenceState(10);
    }
}

void SequenceStateCameraPan(void) {
    s32 i;
    MinigameSequenceWork* workData;
    
    workData = minigameSequenceObject->data;
    if (workData->sequenceState == 0) {
        StartCameraTransition(&workData->cameraStart, &workData->cameraTarget, &workData->cameraDuration, 60, 4);
        workData->sequenceState++;
        for (i = 0; i < 4; i++) {
            if ((getPlayerActionState(i) == 0) && (getPlayerWhompCount(i) >= 100)) {
                GWGameStat.present[56] = 1;
            }
        }
    }

    if (workData->sequenceState == 40) {
        StopPlayerMotion();
        for (i = 0; i < workData->winnerCount; i++) {
            SpawnPlayerWhomp(workData->winningPlayerIds[i]);
            VibratePlayerController(workData->winningPlayerIds[i], 2, 60);
        }
    }

    if (workData->sequenceState == 70) {
       for (i = 0; i < workData->winnerCount; i++) {
            if (!(getPlayerActionState(workData->winningPlayerIds[i])) && (minigameSequenceState < getWhompScoreDisplayMode())) {
                SetMinigameSequenceState(8);
                return;
            }
        }
        SetMinigameSequenceState(9);
    } else {
        workData->sequenceState++;
    }
}

void SequenceStateWinnerReact(void) {
    MinigameSequenceWork* workData;
    
    workData = minigameSequenceObject->data;
    if (workData->sequenceState == 0) {
        GWMGRecordSet(1, getWhompScoreDisplayMode());
        workData->primarySequenceId = MGSeqRecordCreate(getWhompScoreDisplayMode());
        SetScoreDisplayMode(1);
        workData->sequenceState++;
        return;
    }
    if (MGSeqStatGet(workData->primarySequenceId) == 0) {
        SetMinigameSequenceState(9);
    }
}

void SequenceStateScoreSummary(void) {
    MinigameSequenceWork* workData;
    s32 i;

    workData = minigameSequenceObject->data;
    if (workData->sequenceState == 0) {
        for (i = 0; i < 4; i++) {
            if (isPlayerIdle(i) == 0) {
                return;
            }
        }
        if (workData->winnerCount == 0) {
            workData->primarySequenceId = MGSeqDrawCreate();
            HuAudSStreamPlay(4);
        } else {
            workData->primarySequenceId = MGSeqWinCreate(getPlayerMotionState(workData->winningPlayerIds[0]), getPlayerMotionState(workData->winningPlayerIds[1]), getPlayerMotionState(workData->winningPlayerIds[2]), getPlayerMotionState(workData->winningPlayerIds[3]));
            for (i = 0; i < ARRAY_COUNT(workData->winningPlayerIds); i++) {
                if (workData->winningPlayerIds[i] != -1) {
                    PlayPlayerMotionAction(workData->winningPlayerIds[i]);
                    GWPlayerCoinWinAdd(workData->winningPlayerIds[i], 10);
                }
            }
            HuAudSStreamPlay(1);
        }
        
    }
    if (workData->sequenceState++ >= 210) {
        MGSeqKill(workData->primarySequenceId);
        workData->primarySequenceId = -1;
        SetMinigameSequenceState(12);
    }
}

s16 GetSequenceState(void) {
    f32 firstWinnerPosition;
    f32 secondWinnerPosition;
    s16 playerWhompCount;
    s16 loopIndex;
    s16 maximumWhompCount;
    s32 i;
    s32 hasFirstWinner;
    s32 hasSecondWinner;
    s32 unusedWinnerIndex;
    MinigameSequenceWork* workData;

    hasFirstWinner = 0;
    hasSecondWinner = 0;
    workData = minigameSequenceObject->data;
    maximumWhompCount = 0;

    for (i = 0; i < 4; i++) {
        playerWhompCount = getPlayerWhompCount(i);
        if (playerWhompCount > maximumWhompCount) {
            maximumWhompCount = playerWhompCount;
        }
    }

    for (i = 0; i < 4; i++) {
        playerWhompCount = getPlayerWhompCount(i);
        if (maximumWhompCount == playerWhompCount) {
            if (maximumWhompCount > 0) {
                workData->winningPlayerIds[workData->winnerCount++] = i;
                SetPlayerMotionAction(i, 1);
            }
            if (hasFirstWinner == 0) {
                hasFirstWinner = 1;
                firstWinnerPosition = 400.0f * i;
            } else {
                hasSecondWinner = 1;
                secondWinnerPosition = 400.0f * i;
            }
        } else {
            SetPlayerMotionAction(i, 2);
        }        
    }

    if (hasSecondWinner == 0) {
        secondWinnerPosition = firstWinnerPosition;
    }

    workData->cameraStart.x = (600.0f - (0.5f * (secondWinnerPosition + firstWinnerPosition)));
    workData->cameraStart.y = 250.0f;
    workData->cameraStart.z = (200.0f * ((s16) maximumWhompCount + 2));
    workData->cameraTarget.x = -42.0f;
    workData->cameraTarget.y = -720.0f;
    workData->cameraTarget.z = 0.0f;
    workData->cameraDuration = sequenceCameraDurations[(s32) ((secondWinnerPosition - firstWinnerPosition) / 400.0f)];
    return maximumWhompCount;
}

void SequenceStateFinish(void) {
    s32 unusedInteger20;
    s32 unusedInteger1c;
    Vec cameraPositionOffset = {0.0f, 0.0f, -300.0f};
    Vec cameraTarget = {-40.0f, -720.0f, 0.0f};
    f32 cameraZoom;
    s16 unusedFlag;
    s16 loopIndex;
    MinigameSequenceWork* workData;

    cameraZoom = 1420.0f;
    workData = minigameSequenceObject->data;
    switch (workData->sequenceState++) {
    case 0:
        MGSeqKill(workData->secondarySequenceId);
        workData->secondarySequenceId = -1;
        StartCameraTransition(&cameraPositionOffset, &cameraTarget, &cameraZoom, 0x78, 4);
        return;
    case 10:
        SetScoreDisplayMode(0);
        PlayPlayerReactionMotion();
        return;
    case 120:
        StopPlayerMotion();
        SetMinigameSequenceState(9);
        return;
        /* fallthrough */
    case 0x3C:
        break;
    }
    
}

void SequenceStateCleanup(void) {
    MinigameSequenceWork* workData;

    workData = minigameSequenceObject->data;
    if (workData->sequenceState == 0) {
        workData->primarySequenceId = MGSeqDrawCreate();
        workData->sequenceState++;
    }
    if (MGSeqStatGet(workData->primarySequenceId) == 0) {
        MGSeqKill(workData->primarySequenceId);
        workData->primarySequenceId = -1;
        SetMinigameSequenceState(12);
    }
}
