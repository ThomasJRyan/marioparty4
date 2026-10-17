#include "REL/m427Dll.h"

#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hsfformat.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/wipe.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#endif

typedef struct RecordTimerDisplay {
    s32 teamSprites[3];
    s32 teamNameSprite;
    s32 recordLabelSprite;
    s32 recordTimeDigits[7];
    s32 elapsedTimeDigits[7];
    HuVec2f recordDigitOffsets[7];
    f32 recordDigitOffsetX;
    f32 recordDigitOffsetY;
    s32 digitScaleDirection;
    f32 recordDigitsOriginX;
    f32 recordDigitsOriginY;
    f32 digitScale;
    s32 recordTimeFrames;
    s32 elapsedTimeFrames;
} RecordTimerDisplay; // sizeof 0xA4

typedef struct CameraPreset {
    Vec rot;
    Vec cen;
    f32 zoom;
} CameraPreset;

HUPROCESS* gameplayObjectManager;
OMOBJ* gameplayControllerObject;
s32 gameplayState;
s32 gameplayStateFrames;
s32 gameplaySubstateFrames;
s32 bestRecordFrames;
s32 previousRecordFrames;
f32 teamProgress[2];
s32 completedTeamMask;
s32 recordTieState;
s32 activeMinigameSequence;
s32 countdownSequence;
s32 resultMusicHandle;
s32 introSoundHandle;
s32 countdownSoundHandle;
s32 wipeActive;
OMOBJ* resultCameraObject;
s32 activeCameraIndex;
RecordTimerDisplay* recordTimerDisplay;
SoundEffectLifetime soundEffectLifetimes[8];

void InitializeGameplayObjects(OMOBJ*);
void UpdateGameplayState(OMOBJ*);
void CleanupAndReturnToMenu(OMOBJ*);
void HandleSystemExitRequest(OMOBJ*);
void SignalTeamCompletion(s32);
void CreateRecordTimerDisplay(void);
void UpdateRecordTimerDisplay(void);
void SetRecordTimerVisible(void);
void SetPlayerTimerState(s32, s32);
void SetRecordTimeFrames(s32);
void SetElapsedTimeFrames(s32);
void ShowRecordTimer(void);
void HideRecordTimer(void);
void InitializeResultCameras(void);
void EmptyCameraObjectCallback(OMOBJ* object);
void SetResultCameraPreset(void);
void InterpolateResultCamera(f32);
void SetCameraForResults(void);
void InterpolateCameraToWinner(f32);
void RestoreDefaultCamera(void);

CameraPreset cameraPresets[4] = {
    {
        { -25.0f,   0.0f, 0.0f },
        {   0.0f, 275.0f, 0.0f },
        1975.0f
    },
    {
        { 4.0f,  167.0f, 0.0f },
        { 57.0f, 475.0f, 198.0f },
        3375.0f
    },
    {
        {  -0.0f, -190.0f, 0.0f },
        { -90.0f,   50.0f, 800.0f },
        1700.0f
    },
    {
        {    2.0f, 170.0f,   0.0f },
        { -400.0f, 175.0f, 127.0f },
        2425.0f
    }
};

void ObjectSetup(void) {
    HUPROCESS* objectManager;
    s32 initializationIndex;

    OSReport("******* M426ObjectSetup *********\n");
    objectManager = omInitObjMan(0x32, 0x2000);
    gameplayObjectManager = objectManager;
    omGameSysInit(objectManager);
    InitializeResultCameras();
    Hu3DLightAllKill();
    Hu3DLighInit();
    HuAudSndGrpSet(0x34);
    gameplayControllerObject = omAddObjEx(objectManager, 0xFF, 2, 0, -1, InitializeGameplayObjects);
    gameplayState = 0;
    gameplayStateFrames = 0;
    gameplaySubstateFrames = 0;
    bestRecordFrames = previousRecordFrames = GWMGRecordGet(2);
    if (bestRecordFrames <= 0) {
        bestRecordFrames = previousRecordFrames = REFRESH_RATE_F * 60;
    }
    
    completedTeamMask = recordTieState = 0;
    
    activeMinigameSequence = countdownSequence = -1;
    resultMusicHandle = -1;
    introSoundHandle = countdownSoundHandle = -1;
    
    for (initializationIndex = 0; initializationIndex < 2; initializationIndex++) {
        teamProgress[initializationIndex] = 0.0f;
    }
    
    for (initializationIndex = 0; initializationIndex < 8; initializationIndex++) {
        soundEffectLifetimes[initializationIndex].soundEffectId = -1;
        soundEffectLifetimes[initializationIndex].remainingFrames = 0;
    }
}

void InitializeGameplayObjects(OMOBJ* controllerObject) {
    InitializeMapSystem(gameplayObjectManager);
    InitializePlayerSystem(gameplayObjectManager);
    CreateRecordTimerDisplay();
    SetPlayerTimerState(0, 1);
    controllerObject->objFunc = UpdateGameplayState;
}

void UpdateGameplayState(OMOBJ* controllerObject) {
    s32 winnerCharacterIds[2];
    s32 winnerPlayerIndices[2];
    f32 transitionProgress;
    f32 cameraTransitionProgress;
    s32 soundEffectIndex;
    s32 countdownFrames;
    s32 winningTeamIndex;
    s32 sequenceStatus;
    s32 remainingFrames;

    switch (gameplayState) {
        case 0:
            gameplayStateFrames++;
            if (gameplayStateFrames == 3) {
                WipeCreate(1, 0, 0x3C);
                wipeActive = 0;
                SetPlayerTimerState(1, 1);
                SetResultCameraPreset();
            } else if ((gameplayStateFrames > 3) && (WipeStatGet() == 0)) {
                introSoundHandle = HuAudFXPlay(0x657);
                countdownSoundHandle = HuAudFXPlay(0x651);
                gameplayState = 1;
                gameplayStateFrames = 0;
                gameplaySubstateFrames = 0;
            }
            break;
        case 1:
            gameplayStateFrames++;
            transitionProgress = gameplayStateFrames / (REFRESH_RATE_F * 5.0f);
            if (transitionProgress > 1.0f) {
                transitionProgress = 1.0f;
            }
            SetPlayerModelOpacity(sind(90.0f * transitionProgress));
            if (++gameplaySubstateFrames >= REFRESH_RATE_F) {
                gameplayState = 2;
                gameplaySubstateFrames = 0;
            }
            break;
        case 2:
            gameplayStateFrames++;
            transitionProgress = gameplayStateFrames / (REFRESH_RATE_F * 5);
            if (transitionProgress > 1.0f) {
                transitionProgress = 1.0f;
            }
            SetPlayerModelOpacity(sind(90.0f * transitionProgress));
            cameraTransitionProgress = sind(90.0f * (gameplaySubstateFrames / (REFRESH_RATE_F * 4)));
            InterpolateResultCamera(cameraTransitionProgress * cameraTransitionProgress);
            if (++gameplaySubstateFrames >= (REFRESH_RATE_F * 4)) {
                activeMinigameSequence = MGSeqCreate(3, 0);
                SetCameraForResults();
                gameplayState = 3;
                gameplayStateFrames = 0;
                gameplaySubstateFrames = 0;
            }
            break;
        case 3:
            if (activeMinigameSequence != -1) {
                sequenceStatus = MGSeqStatGet(activeMinigameSequence);
                if (((sequenceStatus & 0x10) != 0) && (resultMusicHandle == -1)) {
                    resultMusicHandle = HuAudSeqPlay(0x49);
                }
            } else {
                sequenceStatus = 0;
            }
            if ((activeMinigameSequence == -1) || (sequenceStatus == 0)) {
                gameplayState = 4;
                gameplayStateFrames = 0;
                gameplaySubstateFrames = 0;
            }
            break;
        case 4:
            gameplayStateFrames++;
            countdownFrames = gameplayStateFrames;
            if (countdownFrames > REFRESH_RATE * 300) {
                countdownFrames = REFRESH_RATE * 300;
            }
            
            SetElapsedTimeFrames(countdownFrames);
            for (soundEffectIndex = 0; soundEffectIndex < 8; soundEffectIndex++) {
                if (soundEffectLifetimes[soundEffectIndex].soundEffectId != -1) {
                    if (--soundEffectLifetimes[soundEffectIndex].remainingFrames <= 0) {
                        soundEffectLifetimes[soundEffectIndex].soundEffectId = -1;
                        soundEffectLifetimes[soundEffectIndex].remainingFrames = 0;
                    }
                }
            }
            if (gameplayStateFrames == REFRESH_RATE * 270) {
                countdownSequence = MGSeqCreate(1, REFRESH_RATE * 30, -1, -1);
            }
            if (countdownSequence != -1) {
                remainingFrames = (REFRESH_RATE * 300 - gameplayStateFrames);
                countdownFrames = (remainingFrames + REFRESH_RATE - 2) / REFRESH_RATE;
                if (countdownFrames < 0) {
                    countdownFrames = 0;
                }
                MGSeqParamSet(countdownSequence, 1, countdownFrames);
            }
            if (completedTeamMask != 0) {
                if (completedTeamMask != 1 && completedTeamMask != 2) {
                    if (((s32)frand() & 1) != 0) {
                        completedTeamMask = 1;
                    } else {
                        completedTeamMask = 2;
                    }
                }
                if (countdownSequence != -1) {
                    MGSeqParamSet(countdownSequence, 2, -1);
                }
                countdownSequence = -1;
                activeMinigameSequence = MGSeqCreate(3, 1);
                if (resultMusicHandle != -1) {
                    HuAudSeqFadeOut(resultMusicHandle, 0x64);
                }
                
                for (soundEffectIndex = 0; soundEffectIndex < 8; soundEffectIndex++) {
                    if ((soundEffectLifetimes[soundEffectIndex].soundEffectId != -1) && (soundEffectLifetimes[soundEffectIndex].remainingFrames > 0)) {
                        HuAudFXFadeOut(soundEffectLifetimes[soundEffectIndex].soundEffectId, 0x64);
                    }
                }
                HuAudFXFadeOut(introSoundHandle, 0x64);
                HuAudFXFadeOut(countdownSoundHandle, 0x64);
                if ((completedTeamMask != 0) && (gameplayStateFrames < bestRecordFrames)) {
                    if (completedTeamMask == 1) {
                        winningTeamIndex = 0;
                    } else {
                        winningTeamIndex = 1;
                    }
                    if (getTeamComputerPlayerCount(winningTeamIndex) == 0) {
                        previousRecordFrames = gameplayStateFrames;
                    }
                }
                if ((completedTeamMask != 0) && (gameplayStateFrames <= 45 * REFRESH_RATE)) {
                    if (completedTeamMask == 1) {
                        winningTeamIndex = 0;
                    } else {
                        winningTeamIndex = 1;
                    }
                    if (getTeamComputerPlayerCount(winningTeamIndex) == 0) {
                        GWGameStat.present[0x34] = 1;
                    }
                }
                gameplayState = 5;
                gameplayStateFrames = 0;
                gameplaySubstateFrames = 0;
            } else {
                if (gameplayStateFrames >= REFRESH_RATE * 300 + 1) {
                    MGSeqParamSet(countdownSequence, 1, 0);
                    MGSeqParamSet(countdownSequence, 2, -1);
                    countdownSequence = -1;
                    activeMinigameSequence = MGSeqCreate(3, 1);
                    if (resultMusicHandle != -1) {
                        HuAudSeqFadeOut(resultMusicHandle, 0x64);
                    }
                    
                    for (soundEffectIndex = 0; soundEffectIndex < 8; soundEffectIndex++) {
                        if ((soundEffectLifetimes[soundEffectIndex].soundEffectId != -1) && (soundEffectLifetimes[soundEffectIndex].remainingFrames > 0)) {
                            HuAudFXFadeOut(soundEffectLifetimes[soundEffectIndex].soundEffectId, 0x64);
                        }
                    }
                    HuAudFXFadeOut(introSoundHandle, 0x64);
                    HuAudFXFadeOut(countdownSoundHandle, 0x64);
                    recordTieState = 1;
                    gameplayState = 5;
                    gameplayStateFrames = 0;
                    gameplaySubstateFrames = 0;
                }
            }
            break;
        case 5:
            if (completedTeamMask != 0) {
                WipeCreate(2, 0, REFRESH_RATE);
#if VERSION_PAL
                WipeColorSet(0xA0, 0xA0, 0xA0);
#else
                WipeColorSet(0xFF, 0xFF, 0xFF);
#endif
                wipeActive = 1;
                gameplayState = 6;
                gameplayStateFrames = 0;
                gameplaySubstateFrames = 0;
            } else if ((activeMinigameSequence == -1) || (MGSeqStatGet(activeMinigameSequence) == 0)) {
                gameplayState = 0xA;
                gameplayStateFrames = 0;
                gameplaySubstateFrames = 0;
            }
            break;
        case 6:
            if ((WipeStatGet() == 0) && ((activeMinigameSequence == -1) || (MGSeqStatGet(activeMinigameSequence) == 0))) {
                activeMinigameSequence = -1;
                SetPlayerTimerState(0, 0);
                RestoreDefaultCamera();
                HuAudFXAllStop();
                HuAudFXPlay(0x65E);
                gameplayState = 7;
                gameplayStateFrames = 0;
                gameplaySubstateFrames = 0;
            }
            break;
        case 7:
            if (++gameplayStateFrames > 5) {
                WipeCreate(1, 0, REFRESH_RATE / 2);
#if VERSION_PAL
                WipeColorSet(0xA0, 0xA0, 0xA0);
#else
                WipeColorSet(0xFF, 0xFF, 0xFF);
#endif
                wipeActive = 0;
                HuAudAUXVolSet(-1, -1);
                HideRecordTimer();
                gameplayState = 8;
                gameplayStateFrames = 0;
                gameplaySubstateFrames = 0;
            }
            break;
        case 8:
            if (WipeStatGet() == 0) {
                gameplayState = 0xA;
                gameplayStateFrames = 0;
                gameplaySubstateFrames = 0;
            }
            break;
        case 10:
            if (completedTeamMask != 0) {
                transitionProgress = sind((90.0f * (gameplayStateFrames / (REFRESH_RATE_F * 3))));
                if (transitionProgress >= 1.0f) {
                    transitionProgress = 1.0f;
                }
                SetPlayerModelOpacity(transitionProgress);
                if (gameplayStateFrames > REFRESH_RATE_F / 2) {
                    cameraTransitionProgress = gameplaySubstateFrames / (REFRESH_RATE_F * 2.5f);
                    gameplaySubstateFrames++;
                    if (cameraTransitionProgress >= 1.0f) {
                        cameraTransitionProgress = 1.0f;
                    }
                    cameraTransitionProgress = sind(90.0f * cameraTransitionProgress);
                    InterpolateCameraToWinner(cameraTransitionProgress);
                }
                if (++gameplayStateFrames > REFRESH_RATE_F * 3) {
                    if (completedTeamMask == 1) {
                        GetTeamCharacterIds(0, winnerCharacterIds);
                        GetTeamPlayerIndices(0, winnerPlayerIndices);
                    } else {
                        GetTeamCharacterIds(1, winnerCharacterIds);
                        GetTeamPlayerIndices(1, winnerPlayerIndices);
                    }
                    GWPlayerCoinWinAdd(winnerPlayerIndices[0], 10);
                    GWPlayerCoinWinAdd(winnerPlayerIndices[1], 10);
                    if (previousRecordFrames < bestRecordFrames) {
                        bestRecordFrames = previousRecordFrames;
                        SetRecordTimeFrames(bestRecordFrames);
                        GWMGRecordSet(2, bestRecordFrames);
                        ShowRecordTimer();
                        activeMinigameSequence = MGSeqCreate(0xE, bestRecordFrames);
                        gameplaySubstateFrames = REFRESH_RATE * 3;
                        gameplayState = 0xB;
                        gameplayStateFrames = 0;
                    } else {
                        activeMinigameSequence = MGSeqCreate(5, 3, winnerCharacterIds[0], winnerCharacterIds[1], -1, -1);
                        HuAudSStreamPlay(1);
                        gameplaySubstateFrames = REFRESH_RATE * 3;
                        gameplayState = 0xC;
                        gameplayStateFrames = 0;
                    }
                }
            } else {
                activeMinigameSequence = MGSeqCreate(3, 2);
                HuAudSStreamPlay(4);
                gameplaySubstateFrames = REFRESH_RATE * 3;
                gameplayState = 0xC;
                gameplayStateFrames = 0;
            }
            break;
        case 11:
            if ((activeMinigameSequence == -1) || (MGSeqStatGet(activeMinigameSequence) == 0)) {
                if (completedTeamMask == 1) {
                    GetTeamCharacterIds(0, winnerCharacterIds);
                } else {
                    GetTeamCharacterIds(1, winnerCharacterIds);
                }
                activeMinigameSequence = MGSeqCreate(5, 3, winnerCharacterIds[0], winnerCharacterIds[1], -1, -1);
                HuAudSStreamPlay(1);
                gameplaySubstateFrames = REFRESH_RATE * 3;
                gameplayState = 0xC;
                gameplayStateFrames = 0;
            }
            break;
        case 12:
            if (++gameplayStateFrames >= REFRESH_RATE_F * 3.5f) {
                WipeCreate(2, 0, 0x3C);
                wipeActive = 1;
                gameplayState = 0xD;
                gameplayStateFrames = 0;
                gameplaySubstateFrames = 0;
            }
            break;
        case 13:
            if (WipeStatGet() == 0) {
                gameplayState = 0xE;
                gameplayStateFrames = 0;
                gameplaySubstateFrames = 0;
            }
            break;
        case 14:
            controllerObject->objFunc = CleanupAndReturnToMenu;
            break;
    }
    UpdateRecordTimerDisplay();
    HandleSystemExitRequest(controllerObject);
}

void CleanupAndReturnToMenu(OMOBJ* controllerObject) {
    if (WipeStatGet() == 0) {
        DestroyPlayerSystem();
        DestroyMapSystem();
        SetRecordTimerVisible();
        CharModelKill(-1);
        HuDataDirClose(0x3A0000);
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void HandleSystemExitRequest(OMOBJ* controllerObject) {
    s32 exitRequested;

    exitRequested = 0;
    if ((WipeStatGet() != 0) || (controllerObject->objFunc == CleanupAndReturnToMenu)) {
        return;
    }
    if (omSysExitReq != 0) {
        exitRequested = 1;
    }
    if (exitRequested != 0) {
        if ((s32) wipeActive == 0) {
            WipeCreate(2, 0, 0x3C);
        }
        wipeActive = 1;
        HuAudStreamFadeOut(0x1E);
        controllerObject->objFunc = CleanupAndReturnToMenu;
    }
}

s32 GetGameplayState(void) {
    return gameplayState;
}

void SetTeamProgress(s32 teamIndex, f32 progress) {
    
    if (progress < 0.0f) {
        progress = 0.0f;
    } else if (progress > 1.0f) {
        progress = 1.0f;
    }
    teamProgress[teamIndex] = progress;
    if (teamProgress[teamIndex] >= 1.0f) {
        SignalTeamCompletion(1 << teamIndex);
    }
}

void SignalTeamCompletion(s32 teamCompletionMask) {
    completedTeamMask |= teamCompletionMask;
}

s32 GetWinningTeamResult(void) {
    if (completedTeamMask != 0) {
        if (completedTeamMask != 1 && completedTeamMask != 2) {
            if (((s32)frand() & 1) != 0) {
                completedTeamMask = 1;
            } else {
                completedTeamMask = 2;
            }
        }
        if (completedTeamMask == 1) {
            return 0;
        } else {
            return 1;
        }
    } else {
        if (recordTieState != 0) {
            return 3;
        } else {
            return -1;
        }
    }
}

s32 teamLabelAssetIds[2] = { 0x3A0029, 0x3A002A };
s16 teamLabelXOffsets[2] = { -24, 24 };
s16 teamLabelYOffsets[2] = { -10, 10 };
s16 timerDigitScaleBounds[2] = { 350, 96 };

void CreateRecordTimerDisplay(void) {
    f32 recordDigitsOriginX;
    f32 recordDigitsOriginY;
    f32 spriteX;
    f32 spriteY;
    s32 digitIndex;
    s32 spriteId;
    RecordTimerDisplay* timerDisplay;

    timerDisplay = HuMemDirectMallocNum(HEAP_HEAP, 0xA4, 0x10000000);
    recordTimerDisplay = timerDisplay;
    timerDisplay->recordTimeFrames = 0;
    timerDisplay->elapsedTimeFrames = 0;
    spriteId = espEntry(0x3A0028, 1, 0);
    timerDisplay->teamSprites[0] = spriteId;
    espPriSet(spriteId, 0xFF);
    espDispOn(spriteId);
    espDrawNoSet(spriteId, 0);
    espAttrSet(spriteId, 1);
    espBankSet(spriteId, 0);
    espPosSet(spriteId, 288.0, 240.0);
    
    for (digitIndex = 0; digitIndex < 2; digitIndex++) {
        spriteId = espEntry(teamLabelAssetIds[digitIndex], 1, 0);
        timerDisplay->teamSprites[digitIndex + 1] = spriteId;
        espPriSet(spriteId, 0xC8);
        espDispOn(spriteId);
        espDrawNoSet(spriteId, 0);
        espAttrSet(spriteId, 1);
        espBankSet(spriteId, 0);
        espPosSet(spriteId, 288.0f + teamLabelXOffsets[digitIndex], 240.0);
    }
    
    recordDigitsOriginX = 288.0f;
    recordDigitsOriginY = 400.0f;
    spriteX = 288.0f;
    spriteY = recordDigitsOriginY;
    spriteId = espEntry(0x3A002B, 1, 0);
    timerDisplay->teamNameSprite = spriteId;
    espPriSet(spriteId, 0xBE);
    espDispOn(spriteId);
    espDrawNoSet(spriteId, 0);
    espAttrSet(spriteId, 1);
    espBankSet(spriteId, 0);
    espTPLvlSet(spriteId, 0.5);
    espColorSet(spriteId, 0x80, 0x80, 0x80);
    espScaleSet(spriteId, 1.25, 1.5);
    espPosSet(spriteId, spriteX, spriteY);
    spriteX = 248.0f;
    spriteY = recordDigitsOriginY - 8.0f;
    spriteId = espEntry(0x610037, 1, 0);
    timerDisplay->recordLabelSprite = spriteId;
    espPriSet(spriteId, 0xBE);
    espDispOn(spriteId);
    espDrawNoSet(spriteId, 0);
    espAttrSet(spriteId, 1);
    espBankSet(spriteId, 0);
    espScaleSet(spriteId, 1.0, 1.0);
    espPosSet(spriteId, spriteX - 18.0f, spriteY);
    timerDisplay->recordDigitOffsetX = ((spriteX - 18.0f) - recordDigitsOriginX);
    timerDisplay->recordDigitOffsetY = (spriteY - recordDigitsOriginY);
    timerDisplay->digitScaleDirection = -1;
    timerDisplay->digitScale = 1.0f;
    timerDisplay->recordDigitsOriginX = recordDigitsOriginX;
    timerDisplay->recordDigitsOriginY = recordDigitsOriginY;
    spriteX = 248.0f;
    spriteY = recordDigitsOriginY - 8.0f;
    
    for (digitIndex = 0; digitIndex < 7; digitIndex++) {
        spriteId = espEntry(0x610030, 1, 0);
        timerDisplay->recordTimeDigits[digitIndex] = spriteId;
        espPriSet(spriteId, 0xB4);
        espDispOn(spriteId);
        espDrawNoSet(spriteId, 0);
        espAttrSet(spriteId, 9);
        espScaleSet(spriteId, 1.0, 1.0);
        espPosSet(spriteId, spriteX, spriteY);
        if (digitIndex == 1) {
            espBankSet(spriteId, 10);
        } else if (digitIndex == 4) {
            espBankSet(spriteId, 11);
        } else {
            espBankSet(spriteId, 0);
        }
        timerDisplay->recordDigitOffsets[digitIndex].x = spriteX - recordDigitsOriginX;
        timerDisplay->recordDigitOffsets[digitIndex].y = spriteY - recordDigitsOriginY;
        spriteX += 16.0f;
    }
    spriteX = 248.0f;
    spriteY = 8.0f + recordDigitsOriginY;
    
    for (digitIndex = 0; digitIndex < 7; digitIndex++) {
        spriteId = espEntry(0x610031, 1, 0);
        timerDisplay->elapsedTimeDigits[digitIndex] = spriteId;
        espPriSet(spriteId, 0xB4);
        espDispOn(spriteId);
        espDrawNoSet(spriteId, 0);
        espAttrSet(spriteId, 1);
        espScaleSet(spriteId, 1.0, 1.0);
        espPosSet(spriteId, spriteX, spriteY);
        if (digitIndex == 1) {
            espBankSet(spriteId, 10);
        } else if (digitIndex == 4) {
            espBankSet(spriteId, 11);
        } else {
            espBankSet(spriteId, 0);
        }
        spriteX += 16.0f;
    }
    SetRecordTimeFrames(bestRecordFrames);
    SetElapsedTimeFrames(0);
}

void UpdateRecordTimerDisplay(void) {
    f32 scaleFactor;
    f32 spriteScale;
    s32 playerIndex;
    s32 digitIndex;
    RecordTimerDisplay* timerDisplay;

    timerDisplay = recordTimerDisplay;
    for (digitIndex = 0; digitIndex < 2; digitIndex++) {
        playerIndex = timerDisplay->teamSprites[digitIndex + 1];
        scaleFactor = timerDigitScaleBounds[0] + (teamProgress[digitIndex] * (timerDigitScaleBounds[1] - timerDigitScaleBounds[0]));
        espPosSet(playerIndex, 288.0f + teamLabelYOffsets[digitIndex], scaleFactor);
    }
    if (timerDisplay->digitScaleDirection != -1) {
        if (timerDisplay->digitScaleDirection == 0) {
            if ((timerDisplay->digitScale += 0.02f) >= 1.2f) {
                timerDisplay->digitScale = 1.2f;
                timerDisplay->digitScaleDirection = 1;
            }
        } else {
            if ((timerDisplay->digitScale -= 0.04f) <= 1.0f) {
                timerDisplay->digitScale = 1.0f;
                timerDisplay->digitScaleDirection = 0;
            }
        }
        spriteScale = timerDisplay->digitScale;
        
        for (digitIndex = 0; digitIndex < 7; digitIndex++) {
            playerIndex = timerDisplay->recordTimeDigits[digitIndex];
            espScaleSet(playerIndex, spriteScale, spriteScale);
        }
        playerIndex = timerDisplay->recordLabelSprite;
        espScaleSet(playerIndex, spriteScale, spriteScale);
    }
}

void SetRecordTimerVisible(void) {
    RecordTimerDisplay* timerDisplay;
    s32 spriteIndex;

    timerDisplay = recordTimerDisplay;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        espKill(timerDisplay->teamSprites[spriteIndex]);
    }
    espKill(timerDisplay->teamNameSprite);
}

void SetPlayerTimerState(s32 teamIndex, s32 timerVisible) {
    RecordTimerDisplay* timerDisplay;
    s32 spriteIndex;

    timerDisplay = recordTimerDisplay;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        if (teamIndex == 0) {
            espDispOff(timerDisplay->teamSprites[spriteIndex]);
        } else {
            espDispOn(timerDisplay->teamSprites[spriteIndex]);
        }
    }
    if (timerVisible != 0) {
        if (teamIndex == 0) {
            espDispOff(timerDisplay->teamNameSprite);
        } else {
            espDispOn(timerDisplay->teamNameSprite);
        }
        if (teamIndex == 0) {
            espDispOff(timerDisplay->recordLabelSprite);
        } else {
            espDispOn(timerDisplay->recordLabelSprite);
        }
        
        for (spriteIndex = 0; spriteIndex < 7; spriteIndex++) {
            if (teamIndex == 0) {
                espDispOff(timerDisplay->recordTimeDigits[spriteIndex]);
            } else {
                espDispOn(timerDisplay->recordTimeDigits[spriteIndex]);
            }
        }
        
        for (spriteIndex = 0; spriteIndex < 7; spriteIndex++) {
            if (teamIndex == 0) {
                espDispOff(timerDisplay->elapsedTimeDigits[spriteIndex]);
            } else {
                espDispOn(timerDisplay->elapsedTimeDigits[spriteIndex]);
            }
        }
    }
}

void SetRecordTimeFrames(s32 recordFrames) {
    RecordTimerDisplay* timerDisplay;
    f32 timerDigitScratch;
    s32 timeDigit;

    timerDisplay = recordTimerDisplay;
    timerDisplay->recordTimeFrames = recordFrames;
    timeDigit = recordFrames / (REFRESH_RATE * 60);
    recordFrames -= timeDigit * (REFRESH_RATE * 60);
    if (timeDigit > 9) {
        timeDigit = 9;
    } else if (timeDigit < 0) {
        timeDigit = 0;
    }
    espBankSet(timerDisplay->recordTimeDigits[0], timeDigit);
    timeDigit = (recordFrames / (REFRESH_RATE * 10));
    recordFrames -= timeDigit * (REFRESH_RATE * 10);
    if (timeDigit > 9) {
        timeDigit = 9;
    } else if (timeDigit < 0) {
        timeDigit = 0;
    }
    espBankSet(timerDisplay->recordTimeDigits[2], timeDigit);
    timeDigit = recordFrames / REFRESH_RATE;
    recordFrames -= timeDigit * REFRESH_RATE;
    if (timeDigit > 9) {
        timeDigit = 9;
    } else if (timeDigit < 0) {
        timeDigit = 0;
    }
    espBankSet(timerDisplay->recordTimeDigits[3], timeDigit);
    recordFrames = 100.0f * (recordFrames / REFRESH_RATE_F);
    timeDigit = recordFrames / 10;
    recordFrames -= timeDigit * 10;
    if (timeDigit > 9) {
        timeDigit = 9;
    } else if (timeDigit < 0) {
        timeDigit = 0;
    }
    espBankSet(timerDisplay->recordTimeDigits[5], timeDigit);
    timeDigit = recordFrames;
    if (timeDigit > 9) {
        timeDigit = 9;
    } else if (timeDigit < 0) {
        timeDigit = 0;
    }
    espBankSet(timerDisplay->recordTimeDigits[6], timeDigit);
}

void SetElapsedTimeFrames(s32 elapsedFrames) {
    RecordTimerDisplay* timerDisplay;
    f32 timerDigitScratch;
    s32 timeDigit;

    timerDisplay = recordTimerDisplay;
    timerDisplay->elapsedTimeFrames = elapsedFrames;
    timeDigit = elapsedFrames / (REFRESH_RATE * 60);
    elapsedFrames -= timeDigit * (REFRESH_RATE * 60);
    if (timeDigit > 9) {
        timeDigit = 9;
    } else if (timeDigit < 0) {
        timeDigit = 0;
    }
    espBankSet(timerDisplay->elapsedTimeDigits[0], timeDigit);
    timeDigit = (elapsedFrames / (REFRESH_RATE * 10));
    elapsedFrames -= timeDigit * (REFRESH_RATE * 10);
    if (timeDigit > 9) {
        timeDigit = 9;
    } else if (timeDigit < 0) {
        timeDigit = 0;
    }
    espBankSet(timerDisplay->elapsedTimeDigits[2], timeDigit);
    timeDigit = elapsedFrames / REFRESH_RATE;
    elapsedFrames -= timeDigit * REFRESH_RATE;
    if (timeDigit > 9) {
        timeDigit = 9;
    } else if (timeDigit < 0) {
        timeDigit = 0;
    }
    espBankSet(timerDisplay->elapsedTimeDigits[3], timeDigit);
    elapsedFrames = 100.0f * (elapsedFrames / REFRESH_RATE_F);
    timeDigit = elapsedFrames / 10;
    elapsedFrames -= timeDigit * 10;
    if (timeDigit > 9) {
        timeDigit = 9;
    } else if (timeDigit < 0) {
        timeDigit = 0;
    }
    espBankSet(timerDisplay->elapsedTimeDigits[5], timeDigit);
    timeDigit = elapsedFrames;
    if (timeDigit > 9) {
        timeDigit = 9;
    } else if (timeDigit < 0) {
        timeDigit = 0;
    }
    espBankSet(timerDisplay->elapsedTimeDigits[6], timeDigit);
}

void ShowRecordTimer(void) {
    RecordTimerDisplay* timerDisplay;

    timerDisplay = recordTimerDisplay;
    timerDisplay->digitScaleDirection = 0;
}

void HideRecordTimer(void) {
    RecordTimerDisplay* timerDisplay;
    s32 labelSprite;

    timerDisplay = recordTimerDisplay;
    labelSprite = timerDisplay->teamNameSprite;
    espTPLvlSet(labelSprite, 0.5);
    espColorSet(labelSprite, 0, 0, 0);
}

void InitializeResultCameras(void) {
    OMOBJ* cameraObject;
    s32 cameraIndex;

    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, -1.0f, 300.0f, 50000.0f, 0.6f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 320.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 0x140, 0x1E0);
    Hu3DCameraCreate(2);
    Hu3DCameraPerspectiveSet(2, -1.0f, 300.0f, 50000.0f, 0.6f);
    Hu3DCameraViewportSet(2, 320.0f, 0.0f, 320.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(2, 0x140, 0, 0x140, 0x1E0);
    Hu3DCameraCreate(4);
    Hu3DCameraPerspectiveSet(4, -1.0f, 100.0f, 50000.0f, 1.2f);
    Hu3DCameraViewportSet(4, 0.0f, 0.0f, 576.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(4, 0, 0, 0x280, 0x1E0);
    cameraObject = resultCameraObject = omAddObjEx(gameplayObjectManager, 0x7FDA, 0, 0, -1, omOutViewMulti);
    cameraObject->work[0] = 3;
    
    for (cameraIndex = 0; cameraIndex < 3; cameraIndex++) {
        CRotM[cameraIndex].x = cameraPresets[0].rot.x;
        CRotM[cameraIndex].y = cameraPresets[0].rot.y;
        CRotM[cameraIndex].z = cameraPresets[0].rot.z;
        CenterM[cameraIndex].x = cameraPresets[0].cen.x;
        CenterM[cameraIndex].y = cameraPresets[0].cen.y;
        CenterM[cameraIndex].z = cameraPresets[0].cen.z;
        CZoomM[cameraIndex] = cameraPresets[0].zoom;
    }
    activeCameraIndex = 0;
    omAddObjEx(gameplayObjectManager, 0x3F, 0, 0, -1, EmptyCameraObjectCallback);
}

void EmptyCameraObjectCallback(OMOBJ* object) {}

void EmptyResultCameraCallback(void) {}

void SetResultCameraPreset(void) {
    s32 cameraIndex;

    Hu3DCameraPerspectiveSet(1, 45.0f, 300.0f, 50000.0f, 0.6f);
    Hu3DCameraPerspectiveSet(2, 45.0f, 300.0f, 50000.0f, 0.6f);
    Hu3DCameraPerspectiveSet(4, 45.0f, 100.0f, 50000.0f, 1.2f);
    
    for (cameraIndex = 0; cameraIndex < 2; cameraIndex++) {
        CRotM[cameraIndex].x = cameraPresets[2].rot.x;
        CRotM[cameraIndex].y = cameraPresets[2].rot.y;
        CRotM[cameraIndex].z = cameraPresets[2].rot.z;
        CenterM[cameraIndex].x = cameraPresets[2].cen.x;
        CenterM[cameraIndex].y = cameraPresets[2].cen.y;
        CenterM[cameraIndex].z = cameraPresets[2].cen.z;
        CZoomM[cameraIndex] = cameraPresets[2].zoom;
    }
}

void InterpolateResultCamera(f32 interpolationProgress) {
    s32 cameraIndex;

    for (cameraIndex = 0; cameraIndex < 2; cameraIndex++) {
        CRotM[cameraIndex].x = interpolateAngle(interpolationProgress, cameraPresets[2].rot.x, cameraPresets->rot.x);
        CRotM[cameraIndex].y = interpolateAngle(interpolationProgress, cameraPresets[2].rot.y, cameraPresets->rot.z);
        CRotM[cameraIndex].z = interpolateAngle(interpolationProgress, cameraPresets[2].rot.z, cameraPresets->rot.z);
        CenterM[cameraIndex].x = cameraPresets[2].cen.x + (interpolationProgress * (cameraPresets->cen.x - cameraPresets[2].cen.x));
        CenterM[cameraIndex].y = cameraPresets[2].cen.y + (interpolationProgress * (cameraPresets->cen.y - cameraPresets[2].cen.y));
        CenterM[cameraIndex].z = cameraPresets[2].cen.z + (interpolationProgress * (cameraPresets->cen.z - cameraPresets[2].cen.z));
        CZoomM[cameraIndex] = cameraPresets[2].zoom + (interpolationProgress * (cameraPresets->zoom - cameraPresets[2].zoom));
    }
}

void SetCameraForResults(void) {
    Hu3DCameraPerspectiveSet(1, 45.0f, 700.0f, 50000.0f, 0.6f);
    Hu3DCameraPerspectiveSet(2, 45.0f, 700.0f, 50000.0f, 0.6f);
}

void InterpolateCameraToWinner(f32 interpolationProgress) {
    s32 cameraIndex;

    for (cameraIndex = 0; cameraIndex < 2; cameraIndex++) {
        CRotM[cameraIndex].x = interpolateAngle(interpolationProgress, cameraPresets[1].rot.x, cameraPresets[3].rot.x);
        CRotM[cameraIndex].y = interpolateAngle(interpolationProgress, cameraPresets[1].rot.y, cameraPresets[3].rot.y);
        CRotM[cameraIndex].z = interpolateAngle(interpolationProgress, cameraPresets[1].rot.z, cameraPresets[3].rot.z);
        CenterM[cameraIndex].x = cameraPresets[1].cen.x + (interpolationProgress * (cameraPresets[3].cen.x - cameraPresets[1].cen.x));
        CenterM[cameraIndex].y = cameraPresets[1].cen.y + (interpolationProgress * (cameraPresets[3].cen.y - cameraPresets[1].cen.y));
        CenterM[cameraIndex].z = cameraPresets[1].cen.z + (interpolationProgress * (cameraPresets[3].cen.z - cameraPresets[1].cen.z));
        CZoomM[cameraIndex] = cameraPresets[1].zoom + (interpolationProgress * (cameraPresets[3].zoom - cameraPresets[1].zoom));
    }
}

void RestoreDefaultCamera(void) {
    resultCameraObject->work[0] = 1;
    Hu3DCameraKill(2);
    Hu3DCameraKill(4);
    Hu3DCameraPerspectiveSet(1, 45.0f, 100.0f, 50000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 0x280, 0x1E0);
    CRotM->x = cameraPresets[1].rot.x;
    CRotM->y = cameraPresets[1].rot.y;
    CRotM->z = cameraPresets[1].rot.z;
    CenterM->x = cameraPresets[1].cen.x;
    CenterM->y = cameraPresets[1].cen.y;
    CenterM->z = cameraPresets[1].cen.z;
    *CZoomM = cameraPresets[1].zoom;
}

s32 GetResultCameraState(void) {
    return 0;
}
