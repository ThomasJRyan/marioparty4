#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXVert.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/init.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "string.h"

#include "REL/m428Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/ClusterExec.h"
#include "game/EnvelopeExec.h"
#include "game/ShapeExec.h"
#include "game/frand.h"
#endif

typedef struct M428DllCameraPreset {
    /* 0x00 */ Vec cameraRotation;
    /* 0x0C */ Vec cameraCenter;
    /* 0x18 */ Float cameraZoom;
} M428DllCameraPreset; /* size = 0x1C */

typedef struct M428DllShadowCameraState {
    s16 shadowModelIds[0x10];
    Float shadowAlpha[0x10];
    s32 activeShadowCount;
} M428DllShadowCameraState; /* size = 0x64 */

typedef struct M428DllShadowHookWork {
    M428DllShadowCameraState cameraShadowStates[0x10];
    HU3DSHADOW cameraShadowData[0x10];
    HU3DSHADOW originalShadowData;
} M428DllShadowHookWork; /* size = 0x1058 */

void UpdateMinigame(OMOBJ *);
void ReturnToMainGame(OMOBJ *object);
void CheckMinigameExitRequest(OMOBJ *object);
void UpdateCameraObject(OMOBJ *object);
void InitializeMultiCameraShadows(s32 shadowCameraMask);
void InitializeTeamPlayersAndLighting(OMOBJ *object);
void DrawMultiCameraShadowHook(HU3DMODEL *, Float[3][4]);
void RenderCameraShadowMap(s32 shadowCameraMask);
void DestroyMultiCameraShadows(void);

M428DllCameraPreset cameraPresets[3] = {
    { 65.0f, 0.0f, 0.0f, 0.0f, 400.0f, 0.0f, 2000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f, 400.0f, 0.0f, 2000.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f, 400.0f, 0.0f, 1575.0f },
};

Vec shadowLightPosition = { -1000.0f, 2000.0f, 1000.0f };
Vec shadowLightUp = { 0.0f, 1.0f, 0.0f };
Vec shadowLightTarget = { 0.0f, 0.0f, -450.0f };

HUPROCESS *objectManager;
s32 messageWindowId;
s32 timerMessageId;
u32 gamePhase;
s32 phaseFrame;
s32 jumpIntervalFrames;
s32 jumpDurationFrames;
s32 jumpWarningFrames;
s32 roundTimer;
s32 winningTeam;
s32 closeCameraView;
Float playerRunSpeed[2];
s16 playerIdsByTeam[4];
s16 characterIdsByTeam[4];
#if VERSION_PAL
s32 palTimingWork[2];
#endif
s32 soundEffectIds[4];
Float soundEffectPitchScale[4];
s32 musicSequenceId;
s32 exitRequested;
OMOBJ *mainObject;
s32 rumbleEnabled;
OMOBJ *cameraObject;
s32 cameraMode;
s32 cameraTransitionFrame;
char cameraTransitionWork[8];
s32 shadowModelId;

void ObjectSetup(void)
{
    s32 playerSlot;
    HUPROCESS *processManager;
    s32 rumbleSetting;

    OSReport("******* M428ObjectSetup *********\n");
    processManager = omInitObjMan(0x32, 0x2000);
    objectManager = processManager;
    omGameSysInit(processManager);
    Hu3DLightAllKill();
    Hu3DLighInit();
    mainObject = omAddObjEx(processManager, 0xFF, 2, 0, -1, InitializeTeamPlayersAndLighting);
    messageWindowId = timerMessageId = -1;
    HuAudSndGrpSet(0x35);
    gamePhase = 0;
    phaseFrame = 0;
    roundTimer = 0;
    winningTeam = -1;
    closeCameraView = 0;
    musicSequenceId = -1;
    exitRequested = 0;
    for (playerSlot = 0; playerSlot < 4; playerSlot++) {
        playerIdsByTeam[playerSlot] = -1;
        characterIdsByTeam[playerSlot] = -1;
    }
    for (playerSlot = 0; playerSlot < 4; playerSlot++) {
        soundEffectIds[playerSlot] = -1;
    }
    rumbleSetting = GWGameStat.rumble;
    rumbleEnabled = rumbleSetting;
    Hu3DShadowCreate(45.0f, 1000.0f, 250000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowPosSet(&shadowLightPosition, &shadowLightUp, &shadowLightTarget);
    InitializeMultiCameraShadows(3);
}

Vec keyLightPosition = { 0.0f, 0.0f, 0.0f };
Vec keyLightDirection = { -1.0f, -2.0f, -1.0f };
GXColor keyLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };

void InitializeTeamPlayersAndLighting(OMOBJ *object)
{
    s32 playerId;
    s32 teamZeroCount;
    s32 teamOneCount;
    s32 lightId;

    lightId = Hu3DGLightCreateV(&keyLightPosition, &keyLightDirection, &keyLightColor);
    Hu3DGLightStaticSet(lightId, 1);
    Hu3DGLightInfinitytSet(lightId);
    for (playerId = 0, teamZeroCount = 0, teamOneCount = 0; playerId < 4; playerId++) {
        if (GWPlayerCfg[playerId].group == 0) {
            playerIdsByTeam[teamZeroCount] = playerId;
            characterIdsByTeam[teamZeroCount] = GWPlayerCfg[playerId].character;
            teamZeroCount++;
        }
        else {
            playerIdsByTeam[teamOneCount + 2] = playerId;
            characterIdsByTeam[teamOneCount + 2] = GWPlayerCfg[playerId].character;
            teamOneCount++;
        }
    }
    if ((teamZeroCount != 2) || (teamOneCount != 2)) {
        for (playerId = 0; playerId < 4; playerId++) {
            playerIdsByTeam[playerId] = playerId;
            characterIdsByTeam[playerId] = GWPlayerCfg[playerId].character;
        }
    }
    InitializeMapAndCameras(objectManager);
    CreatePlayerTeamObjects(objectManager);
    object->objFunc = UpdateMinigame;
}

void UpdateMinigame(OMOBJ *object)
{
    Vec cameraPosition;
    Vec cameraTarget;
    Vec cameraUp;
    Vec cameraCenter;
    Vec cameraRotation;
    Float cameraBobOffset;
    s32 playerIndex;
    s32 playerSurvivalState;
    s32 phaseElapsed;
    s32 timerDisplayValue;
    s32 messageStatus;
    s32 leftPlayerFell;
    s32 rightPlayerFell;

    switch (gamePhase) {
        case 0:
            phaseFrame++;
            Hu3DCamera->fov = -1.0f;
            Hu3DCamera[1].fov = -1.0f;
            if (phaseFrame > 3) {
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
                exitRequested = 0;
                SetScoreDisplayVisible(1);
                soundEffectIds[0] = HuAudFXPlay(0x65F);
                if ((soundEffectIds[1] = HuAudFXPlay(0x660)) != -1) {
                    soundEffectPitchScale[1] = 0.0f;
                    HuAudFXVolSet(soundEffectIds[1], (s32)(127.0f * soundEffectPitchScale[1]));
                    HuAudFXPitchSet(soundEffectIds[1], (s32)(8191.0f * soundEffectPitchScale[1]));
                }
                if ((soundEffectIds[2] = HuAudFXPlay(0x661)) != -1) {
                    soundEffectPitchScale[2] = 0.0f;
                    HuAudFXVolSet(soundEffectIds[2], (s32)(127.0f * soundEffectPitchScale[2]));
                    HuAudFXPitchSet(soundEffectIds[2], (s32)(8191.0f * soundEffectPitchScale[2]));
                }
                Hu3DCamera->fov = 45.0f;
                Hu3DCamera[1].fov = 45.0f;
                gamePhase = 1;
                phaseFrame = 0;
            }
            break;
        case 1:
            if (WipeStatGet() == 0) {
                gamePhase = 2;
                phaseFrame = 0;
            }
            else {
                cameraRotation.x = cameraPresets->cameraRotation.x;
                cameraRotation.y = cameraPresets->cameraRotation.y;
                cameraRotation.z = cameraPresets->cameraRotation.z;
                cameraCenter.x = cameraPresets->cameraCenter.x;
                cameraCenter.y = cameraPresets->cameraCenter.y;
                cameraCenter.z = cameraPresets->cameraCenter.z;
                ConfigureCameraTransition(0, 1.0f, &cameraCenter, &cameraRotation, cameraPresets->cameraZoom);
                ConfigureCameraTransition(1, 1.0f, &cameraCenter, &cameraRotation, cameraPresets->cameraZoom);
            }
            break;
        case 2:
            if (++phaseFrame == REFRESH_RATE * 2) {
                cameraRotation.x = cameraPresets[2].cameraRotation.x;
                cameraRotation.y = cameraPresets[2].cameraRotation.y;
                cameraRotation.z = cameraPresets[2].cameraRotation.z;
                cameraCenter.x = cameraPresets[2].cameraCenter.x;
                cameraCenter.y = cameraPresets[2].cameraCenter.y;
                cameraCenter.z = cameraPresets[2].cameraCenter.z;
                ConfigureCameraTransition(0, (REFRESH_RATE_F * 5), &cameraCenter, &cameraRotation, cameraPresets[2].cameraZoom);
                ConfigureCameraTransition(1, (REFRESH_RATE_F * 5), &cameraCenter, &cameraRotation, cameraPresets[2].cameraZoom);
            }
            if ((GetPlayerCount() != 0) && (phaseFrame >= REFRESH_RATE * 2)) {
                cameraRotation.x = cameraPresets[1].cameraRotation.x;
                cameraRotation.y = cameraPresets[1].cameraRotation.y;
                cameraRotation.z = cameraPresets[1].cameraRotation.z;
                cameraCenter.x = cameraPresets[1].cameraCenter.x;
                cameraCenter.y = cameraPresets[1].cameraCenter.y;
                cameraCenter.z = cameraPresets[1].cameraCenter.z;
                ConfigureCameraTransition(0, (0.7f * REFRESH_RATE_F), &cameraCenter, &cameraRotation, cameraPresets[1].cameraZoom);
                ConfigureCameraTransition(1, (0.7f * REFRESH_RATE_F), &cameraCenter, &cameraRotation, cameraPresets[1].cameraZoom);
                messageWindowId = MGSeqCreate(3, 0);
                gamePhase = 3;
                phaseFrame = 0;
                roundTimer = 0;
            }
            break;
        case 3:
            if (messageWindowId != -1) {
                messageStatus = MGSeqStatGet(messageWindowId);
                if (((messageStatus & 0x10) != 0) && (musicSequenceId == -1)) {
                    musicSequenceId = HuAudSeqPlay(0x4B);
                }
            }
            else {
                messageStatus = 0;
            }
            if ((messageWindowId == -1) || (messageStatus == 0)) {
                messageWindowId = -1;
                gamePhase = 4;
                phaseFrame = 0;
                jumpIntervalFrames = (REFRESH_RATE_F * 3) + (REFRESH_RATE_F * 2) * (0.007874016f * (frand() & 0x7F));
                jumpDurationFrames = (REFRESH_RATE_F * 1.5f) + (REFRESH_RATE_F) * (0.007874016f * (frand() & 0x7F));
                jumpWarningFrames = (0.75f * REFRESH_RATE_F) + ((REFRESH_RATE_F * 2) * (0.007874016f * (frand() & 0x7F)));
#if VERSION_PAL
                palTimingWork[0] = 0;
                palTimingWork[1] = 0;
#endif
                playerRunSpeed[0] = 15.0f;
                playerRunSpeed[1] = 15.0f;
                roundTimer = REFRESH_RATE_F * 300.0f;
            }
            break;
        case 4:
            if (++phaseFrame >= jumpIntervalFrames) {
                phaseElapsed = phaseFrame - jumpIntervalFrames;
                for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                    if (playerRunSpeed[playerIndex] < 60.0f) {
                        playerSurvivalState = 0;
                    }
                    else {
                        playerSurvivalState = 1;
                    }
#if VERSION_PAL
#define _VALUE 20.0f
#else
#define _VALUE 15.0f
#endif
                    if (phaseElapsed <= (_VALUE + ((0.35f * REFRESH_RATE_F) + jumpWarningFrames))) {
                        SetCameraShake(playerIndex, 15.0f, 1);
                        SetTeamScoreAlpha(playerIndex, 0.05f);
                        if (phaseElapsed == 0) {
                            SetTeamRumbleModeValue(playerIndex, 0);
                        }
                        else if (phaseElapsed < jumpWarningFrames) {
                            SetTeamRumbleMode(playerIndex, 1);
                            if ((playerSurvivalState == 1) && (GetTeamRumbleMode(playerIndex) == 0)) {
                                playerRunSpeed[playerIndex] = 15.0f;
                            }
                        }
                        else if (phaseElapsed == jumpWarningFrames) {
                            if (playerSurvivalState == 1) {
                                SetTeamRumbleModeValue(playerIndex, playerSurvivalState);
                            }
#if VERSION_PAL
                            palTimingWork[playerIndex] = 0;
                        }
                        else if (palTimingWork[playerIndex] == 0 && phaseElapsed >= (s32)(jumpWarningFrames + 17.5f)) {
                            palTimingWork[playerIndex] = 1;
#else
                        } else if (phaseElapsed == (21.0f + jumpWarningFrames)) {
#endif
                            if (playerSurvivalState == 0) {
                                SetTeamRumbleMode(playerIndex, 2);
                            }
                            else {
                                SetTeamRumbleMode(playerIndex, 3);
                                if (rumbleEnabled == 0) {
                                    SetTeamScoreFlash(playerIndex, 1);
                                }
                                if (playerSurvivalState != 0) {
                                    StartTeamRumble(playerIndex, jumpDurationFrames, 3.125f);
                                }
                            }
                        }
                    }
                    else {
                        SetCameraShake(playerIndex, playerRunSpeed[playerIndex], 1);
                        if ((phaseElapsed < ((_VALUE + jumpWarningFrames + jumpDurationFrames) - (REFRESH_RATE_F / 10))) && (playerSurvivalState == 1)) {
                            cameraBobOffset = 0.10000000149011612 * sind((20.0f * phaseElapsed));
                            SetTeamScoreAlpha(playerIndex, 0.3f + cameraBobOffset);
                        }
                        if (playerSurvivalState == 0) {
                            SetTeamRumbleMode(playerIndex, 4);
                        }
                        else {
                            SetTeamRumbleMode(playerIndex, 5);
                        }
                    }
                }
                if (phaseElapsed == (_VALUE + jumpWarningFrames + jumpDurationFrames)) {
                    SetTeamRumbleMode(0, 6);
                    SetTeamRumbleMode(1, 6);
                }
                else if (phaseElapsed > (_VALUE + jumpWarningFrames + jumpDurationFrames)) {
                    ClearCameraShake(0);
                    ClearCameraShake(1);
                    SetTeamScoreAlpha(0, 0.0f);
                    SetTeamScoreAlpha(1, 0.0f);
                    SetTeamRumbleModeValue(0, -1);
                    SetTeamRumbleModeValue(1, -1);
                    SetTeamRumbleMode(0, 0);
                    SetTeamRumbleMode(1, 0);
                    if ((frand() & 3) == 0) {
                        playerRunSpeed[0] = playerRunSpeed[1] = 15.0f;
                    }
                    else {
                        playerRunSpeed[0] = playerRunSpeed[1] = 60.0f;
                    }
                    phaseFrame = 0;
                    jumpIntervalFrames = (REFRESH_RATE_F * 3) + (REFRESH_RATE_F * 2) * (0.007874016f * (frand() & 0x7F));
                    jumpDurationFrames = (REFRESH_RATE_F * 1.5f) + (REFRESH_RATE_F) * (0.007874016f * (frand() & 0x7F));
                    jumpWarningFrames = (REFRESH_RATE_F * 0.75f) + ((REFRESH_RATE_F * 2) * (0.007874016f * (frand() & 0x7F)));
                }
                for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                    if (playerRunSpeed[playerIndex] < 60.0f) {
                        playerSurvivalState = 0;
                    }
                    else {
                        playerSurvivalState = 1;
                    }
                    if ((phaseElapsed <= (_VALUE + ((0.35f * REFRESH_RATE_F) + jumpWarningFrames))) || (playerSurvivalState < 1)) {
                        if (soundEffectPitchScale[playerIndex + 1] < 0.5f) {
                            soundEffectPitchScale[playerIndex + 1] += 0.1f;
                        }
                        if (soundEffectPitchScale[playerIndex + 1] > 0.5f) {
                            soundEffectPitchScale[playerIndex + 1] -= 0.1f;
                        }
                    }
                    else {
                        if (soundEffectPitchScale[playerIndex + 1] < 1.0f) {
                            soundEffectPitchScale[playerIndex + 1] += 0.1f;
                        }
                        if (soundEffectPitchScale[playerIndex + 1] > 1.0f) {
                            soundEffectPitchScale[playerIndex + 1] = 1.0f;
                        }
                    }
                }
            }
#undef _VALUE
            else {
                if (soundEffectPitchScale[1] > 0.0f) {
                    soundEffectPitchScale[1] -= 0.01f;
                }
                if (soundEffectPitchScale[1] < 0.0f) {
                    soundEffectPitchScale[1] = 0.0f;
                }
                if (soundEffectPitchScale[2] > 0.0f) {
                    soundEffectPitchScale[2] -= 0.01f;
                }
                if (soundEffectPitchScale[2] < 0.0f) {
                    soundEffectPitchScale[2] = 0.0f;
                }
            }
            if (soundEffectIds[1] != -1) {
                HuAudFXVolSet(soundEffectIds[1], (s32)(127.0f * soundEffectPitchScale[1]));
                HuAudFXPitchSet(soundEffectIds[1], (s32)(8191.0f * soundEffectPitchScale[1]));
            }
            if (soundEffectIds[2] != -1) {
                HuAudFXVolSet(soundEffectIds[2], (s32)(127.0f * soundEffectPitchScale[2]));
                HuAudFXPitchSet(soundEffectIds[2], (s32)(8191.0f * soundEffectPitchScale[2]));
            }
            if ((roundTimer <= (REFRESH_RATE_F * 30)) && (timerMessageId == -1)) {
                timerMessageId = MGSeqCreate(1, roundTimer / REFRESH_RATE, -1, -1);
            }
            if (timerMessageId != -1) {
                timerDisplayValue = (roundTimer + REFRESH_RATE - 1) / REFRESH_RATE;
                if (timerDisplayValue < 0) {
                    timerDisplayValue = 0;
                }
                MGSeqParamSet(timerMessageId, 1, timerDisplayValue);
            }
            leftPlayerFell = GetTeamPlayerCount(0);
            rightPlayerFell = GetTeamPlayerCount(1);
            if ((--roundTimer < -1) || (leftPlayerFell != 0) || (rightPlayerFell != 0)) {
                if (timerMessageId != -1) {
                    MGSeqParamSet(timerMessageId, 2, -1);
                }
                timerMessageId = -1;
                messageWindowId = MGSeqCreate(3, 1);
                ClearCameraShake(0);
                ClearCameraShake(1);
                SetTeamScoreAlpha(0, 0.0f);
                SetTeamScoreAlpha(1, 0.0f);
                SetTeamRumbleModeValue(0, -1);
                SetTeamRumbleModeValue(1, -1);
                SetTeamRumbleMode(0, 0);
                SetTeamRumbleMode(1, 0);
                if (soundEffectIds[0] != -1) {
                    HuAudFXFadeOut(soundEffectIds[0], 0x64);
                }
                if (soundEffectIds[1] != -1) {
                    HuAudFXFadeOut(soundEffectIds[1], 0x64);
                }
                if (soundEffectIds[2] != -1) {
                    HuAudFXFadeOut(soundEffectIds[2], 0x64);
                }
                if (musicSequenceId != -1) {
                    HuAudSeqFadeOut(musicSequenceId, 0x64);
                }
                if ((leftPlayerFell != 0) && (rightPlayerFell != 0)) {
                    if ((frand() & 1) != 0) {
                        winningTeam = 0;
                    }
                    else {
                        winningTeam = 1;
                    }
                }
                else if (leftPlayerFell != 0) {
                    winningTeam = 0;
                }
                else if (rightPlayerFell != 0) {
                    winningTeam = 1;
                }
                gamePhase = 6;
                phaseFrame = 0;
            }
            break;
        case 6:
            if ((winningTeam == -1) || (messageWindowId == -1) || (MGSeqStatGet(messageWindowId) == 0)) {
                if (winningTeam != -1) {
                    messageWindowId = -1;
                    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, REFRESH_RATE / 2);
                    WipeColorSet(0xFF, 0xFF, 0xFF);
                    exitRequested = 1;
                }
                gamePhase = 5;
                phaseFrame = 0;
            }
            break;
        case 5:
            if (winningTeam != -1) {
                if (WipeStatGet() == 0) {
                    gamePhase = 7;
                    phaseFrame = 0;
                }
            }
            else if ((messageWindowId == -1) || (MGSeqStatGet(messageWindowId) == 0)) {
                messageWindowId = -1;
                gamePhase = 8;
                phaseFrame = 0;
            }
            break;
        case 7:
            phaseFrame++;
            if (phaseFrame == 1) {
                if (winningTeam == 0) {
                    SetCameraMode(0);
                    SetRoundEndFlag(0);
                    closeCameraView = 1;
                    SetScoreDisplayVisible(0);
                }
                else if (winningTeam == 1) {
                    SetCameraMode(1);
                    SetRoundEndFlag(1);
                    closeCameraView = 1;
                    SetScoreDisplayVisible(0);
                }
            }
            else if (phaseFrame >= 0xA) {
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, REFRESH_RATE);
                WipeColorSet(0xFF, 0xFF, 0xFF);
                exitRequested = 0;
                soundEffectIds[3] = HuAudFXPlay(0x662);
                gamePhase = 8;
                phaseFrame = 0;
            }
            break;
        case 8:
            if (winningTeam == 0) {
                if (++phaseFrame >= REFRESH_RATE_F * 2) {
                    gamePhase = 9;
                    phaseFrame = 0;
                }
            }
            else if (winningTeam == 1) {
                if (++phaseFrame >= REFRESH_RATE_F * 2) {
                    gamePhase = 9;
                    phaseFrame = 0;
                }
            }
            else {
                if (++phaseFrame >= REFRESH_RATE_F) {
                    gamePhase = 9;
                    phaseFrame = 0;
                }
            }
            break;
        case 9:
            if (winningTeam == 0) {
                messageWindowId = MGSeqCreate(5, 3, characterIdsByTeam[0], characterIdsByTeam[1], -1, -1);
                GWPlayerCoinWinAdd(playerIdsByTeam[0], 10);
                GWPlayerCoinWinAdd(playerIdsByTeam[1], 10);
                HuAudSStreamPlay(1);
                gamePhase = 0xA;
                roundTimer = 0;
            }
            else if (winningTeam == 1) {
                messageWindowId = MGSeqCreate(5, 3, characterIdsByTeam[2], characterIdsByTeam[3], -1, -1);
                GWPlayerCoinWinAdd(playerIdsByTeam[2], 10);
                GWPlayerCoinWinAdd(playerIdsByTeam[3], 10);
                HuAudSStreamPlay(1);
                gamePhase = 0xA;
                roundTimer = 0;
            }
            else {
                messageWindowId = MGSeqCreate(3, 2);
                gamePhase = 0xA;
                roundTimer = 0;
                HuAudSStreamPlay(4);
            }
            break;
        case 10:
            if (++roundTimer >= REFRESH_RATE_F * 3.5f) {
                messageWindowId = -1;
                gamePhase = 0xB;
                roundTimer = 0;
            }
            break;
        case 11:
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 0x3C);
            exitRequested = 1;
            if (soundEffectIds[3] != -1) {
                HuAudFXStop(soundEffectIds[3]);
            }
            object->objFunc = ReturnToMainGame;
            break;
    }
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        cameraTarget.x = CenterM[playerIndex].x;
        cameraTarget.y = CenterM[playerIndex].y;
        cameraTarget.z = CenterM[playerIndex].z;
        if (closeCameraView == 0) {
            cameraPosition.x = CenterM[playerIndex].x - 1200.0f;
            cameraPosition.y = 1200.0f + CenterM[playerIndex].y;
            cameraPosition.z = 1200.0f + CenterM[playerIndex].z;
        }
        else {
            cameraPosition.x = CenterM[playerIndex].x - 600.0f;
            cameraPosition.y = 1000.0f + CenterM[playerIndex].y;
            cameraPosition.z = 600.0f + CenterM[playerIndex].z;
        }
        cameraUp.x = 0.0f;
        cameraUp.y = 1.0f;
        cameraUp.z = 0.0f;
        SetCameraShadowView(playerIndex, &cameraPosition, &cameraUp, &cameraTarget);
    }
    CheckMinigameExitRequest(object);
}

void ReturnToMainGame(OMOBJ *object)
{
    if (WipeStatGet() == 0) {
        DestroyMapAndCameras();
        DestroyPlayerModuleObjects();
        DestroyMultiCameraShadows();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void CheckMinigameExitRequest(OMOBJ *object)
{
    s32 exitRequested = 0;
    if ((WipeStatGet() != 0) || (object->objFunc == ReturnToMainGame)) {
        return;
    }
    if (omSysExitReq != 0) {
        exitRequested = 1;
    }
    if (exitRequested != 0) {
        if (exitRequested == 0) {
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        }
        exitRequested = 1;
        HuAudSeqAllFadeOut(0x64);
        object->objFunc = ReturnToMainGame;
    }
}

s32 GetGamePhase(void)
{
    return gamePhase;
}

void InitializeCameras(void)
{
    s32 cameraIndex;
    OMOBJ *object;

    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 45.0f, 50.0f, 50000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 320.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 0x140, 0x1E0);
    Hu3DCameraCreate(2);
    Hu3DCameraPerspectiveSet(2, 45.0f, 50.0f, 50000.0f, 1.2f);
    Hu3DCameraViewportSet(2, 0.0f, 0.0f, 320.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(2, 0, 0, 0x140, 0x1E0);
    object = cameraObject = omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutViewMulti);
    object->work[0] = 2;
    for (cameraIndex = 0; cameraIndex < 2; cameraIndex++) {
        CRotM[cameraIndex].x = cameraPresets->cameraRotation.x;
        CRotM[cameraIndex].y = cameraPresets->cameraRotation.y;
        CRotM[cameraIndex].z = cameraPresets->cameraRotation.z;
        CenterM[cameraIndex].x = cameraPresets->cameraCenter.x;
        CenterM[cameraIndex].y = cameraPresets->cameraCenter.y;
        CenterM[cameraIndex].z = cameraPresets->cameraCenter.z;
        CZoomM[cameraIndex] = cameraPresets->cameraZoom;
    }
    cameraTransitionFrame = 0;
    cameraMode = -1;
    omAddObjEx(objectManager, 0xFE, 0, 0, -1, UpdateCameraObject);
}

void UpdateCameraObject(OMOBJ *object) { }

void EmptyCameraCallback(void) { }

s32 GetCameraTransitionFrame(void)
{
    return cameraTransitionFrame;
}

void InterpolateCameraPreset(s32 startCameraId, s32 endCameraId, Float blendFactor)
{
    M428DllCameraPreset *startCamera;
    M428DllCameraPreset *endCamera;

    startCamera = &cameraPresets[startCameraId];
    endCamera = &cameraPresets[endCameraId];
    Center.x = startCamera->cameraCenter.x + (blendFactor * (endCamera->cameraCenter.x - startCamera->cameraCenter.x));
    Center.y = startCamera->cameraCenter.y + (blendFactor * (endCamera->cameraCenter.y - startCamera->cameraCenter.y));
    Center.z = startCamera->cameraCenter.z + (blendFactor * (endCamera->cameraCenter.z - startCamera->cameraCenter.z));
    CZoom = startCamera->cameraZoom + (blendFactor * (endCamera->cameraZoom - startCamera->cameraZoom));
}

void InitializeMultiCameraShadows(s32 shadowCameraMask)
{
    HU3DSHADOW *shadowCamera;
    s32 cameraIndex;
    M428DllShadowCameraState *shadowState;
    HU3DMODEL *model;
    M428DllShadowHookWork *shadowWork;
    s32 modelSlot;
    s32 shadowSize;
    s32 modelId;
    u32 cameraMaskBit;

    shadowModelId = modelId = Hu3DHookFuncCreate(DrawMultiCameraShadowHook);
    model = &Hu3DData[modelId];
    Hu3DModelLayerSet(modelId, 0);
    shadowWork = model->hookData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(M428DllShadowHookWork), model->mallocNo);
    memset(shadowWork, 0, sizeof(M428DllShadowHookWork));
    shadowWork->originalShadowData = Hu3DShadowData;
    for (cameraIndex = 0; cameraIndex < 0x10; cameraIndex++) {
        shadowState = &shadowWork->cameraShadowStates[cameraIndex];
        shadowCamera = &shadowWork->cameraShadowData[cameraIndex];
        cameraMaskBit = 1 << cameraIndex;

        *shadowCamera = Hu3DShadowData;
        for (modelSlot = 0; modelSlot < 0x10; modelSlot++) {
            shadowState->shadowModelIds[modelSlot] = -1;
            shadowState->shadowAlpha[modelSlot] = 1.0f;
        }
        shadowState->activeShadowCount = 0;
        if ((cameraMaskBit & shadowCameraMask) != 0) {
            if (cameraIndex == 0) {
                shadowCamera->buf = Hu3DShadowData.buf;
            }
            else {
                shadowSize = shadowCamera->size;
                shadowCamera->buf = HuMemDirectMallocNum(HEAP_MODEL, shadowSize * shadowSize, model->mallocNo);
            }
        }
        else {
            shadowCamera->buf = NULL;
        }
    }
    Hu3DShadowCamBit++;
}

void DrawMultiCameraShadowHook(HU3DMODEL *model, Mtx matrix)
{
    HU3DSHADOW *shadowCamera;
    s32 cameraIndex;
    M428DllShadowHookWork *shadowWork;
    M428DllShadowCameraState *shadowState;

    shadowWork = model->hookData;
    if (Hu3DCameraNo == 0) {
        for (cameraIndex = 0; cameraIndex < 0x10; cameraIndex++) {
            shadowState = &shadowWork->cameraShadowStates[cameraIndex];
            shadowCamera = &shadowWork->cameraShadowData[cameraIndex];
            if ((shadowCamera->buf != NULL) && !(shadowCamera->fov <= 0.0f) && (shadowState->activeShadowCount > 0)) {
                Hu3DShadowData = *shadowCamera;
                RenderCameraShadowMap(cameraIndex);
                *shadowCamera = Hu3DShadowData;
            }
        }
        HuSprDispInit();
        HuSprExec(0x7F);
    }
    shadowState = &shadowWork->cameraShadowStates[Hu3DCameraNo];
    shadowCamera = &shadowWork->cameraShadowData[Hu3DCameraNo];
    if ((shadowCamera->buf == NULL) || (shadowCamera->fov <= 0.0f)) {
        return;
    }
    Hu3DShadowData = *shadowCamera;
}

GXColor transparentShadowClearColor = { 0, 0, 0, 0 };

void RenderCameraShadowMap(s32 cameraIndex)
{
    Mtx shadowProjection;
    Mtx identityMatrix;
    Mtx shadowCameraMatrix;
    Mtx44 shadowProjectionMatrix;
    HU3DSHADOW *shadowData;
    HU3DMODEL *model;
    s16 shadowAlpha;
    M428DllShadowCameraState *shadowState;
    M428DllShadowHookWork *shadowWork;
    s32 shadowSlot;
    s32 vertexIndex;
    u32 savedShadowAlpha;

    model = &Hu3DData[shadowModelId];
    shadowWork = model->hookData;
    shadowState = &shadowWork->cameraShadowStates[cameraIndex];
    shadowData = &shadowWork->cameraShadowData[cameraIndex];
    savedShadowAlpha = Hu3DShadowData.alpha;
    if (cameraIndex == 0) {
        GXSetViewport(0.0f, 0.0f, RenderMode->fbWidth, RenderMode->xfbHeight, 0.0f, 1.0f);
        GXSetScissor(0, 0, RenderMode->fbWidth, RenderMode->efbHeight);
        C_MTXOrtho(shadowProjectionMatrix, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        GXSetProjection(shadowProjectionMatrix, GX_ORTHOGRAPHIC);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_U8, 0);
        GXSetTevColor(GX_TEVREG0, BGColor);
        GXSetNumTexGens(0);
        GXSetNumTevStages(1);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetNumChans(0);
        MTXIdentity(identityMatrix);
        GXLoadPosMtxImm(identityMatrix, 0);
        GXSetZMode(0, GX_ALWAYS, GX_TRUE);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        GXPosition1x8(0);
        GXPosition1x8(0);
        GXPosition1x8(0);
        GXPosition1x8(1);
        GXPosition1x8(0);
        GXPosition1x8(0);
        GXPosition1x8(1);
        GXPosition1x8(1);
        GXPosition1x8(0);
        GXPosition1x8(0);
        GXPosition1x8(1);
        GXPosition1x8(0);
    }
    Hu3DDrawPreInit();
    GXSetCopyClear(transparentShadowClearColor, 0xFFFFFF);
    C_MTXPerspective(shadowProjectionMatrix, Hu3DShadowData.fov, 1.2f, Hu3DShadowData.nnear, Hu3DShadowData.ffar);
    GXSetProjection(shadowProjectionMatrix, GX_PERSPECTIVE);
    if (Hu3DShadowData.size <= 0xF0) {
        GXSetScissor(2, 2, (Hu3DShadowData.size * 2) - 4, (Hu3DShadowData.size * 2) - 4);
        GXSetViewport(0.0f, 0.0f, Hu3DShadowData.size * 2, Hu3DShadowData.size * 2, 0.0f, 1.0f);
        vertexIndex = (Hu3DShadowData.size / 2) * (Hu3DShadowData.size / 2);
    }
    else {
        GXSetScissor(1, 1, Hu3DShadowData.size - 2, Hu3DShadowData.size - 2);
        GXSetViewport(0.0f, 0.0f, Hu3DShadowData.size, Hu3DShadowData.size, 0.0f, 1.0f);
        vertexIndex = Hu3DShadowData.size * Hu3DShadowData.size;
    }
    C_MTXLookAt(Hu3DCameraMtx, &Hu3DShadowData.camPos, &Hu3DShadowData.camUp, &Hu3DShadowData.camTarget);
    MTXCopy(Hu3DCameraMtx, Hu3DShadowData.lookAtMtx);
    shadowModelDrawF = 1;
    GXInvalidateTexAll();
    GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, BGColor);
    for (shadowAlpha = 0; shadowAlpha < 0x10; shadowAlpha++) {
        if (shadowState->shadowModelIds[shadowAlpha] != -1) {
            model = &Hu3DData[shadowState->shadowModelIds[shadowAlpha]];
            if (((model->attr & 1) == 0) && ((model->attr & 0x8000) == 0)) {
                if ((model->attr & 8) != 0) {
                    shadowSlot = 0;
                    if (model->motId != -1) {
                        Hu3DMotionExec(shadowAlpha, model->motId, model->motWork.time, 0);
                    }
                    if (model->motIdShift != -1) {
                        Hu3DSubMotionExec(shadowAlpha);
                    }
                    if (model->motIdOvl != -1) {
                        Hu3DMotionExec(shadowAlpha, model->motIdOvl, model->motOvlWork.time, 1);
                    }
                    if ((model->attr & 0x400) != 0) {
                        ClusterMotionExec(model);
                        shadowSlot = 1;
                    }
                    if (model->motIdShape != -1) {
                        if (model->motId == -1) {
                            Hu3DMotionExec(shadowAlpha, model->motIdShape, model->motShapeWork.time, 0);
                        }
                        else {
                            Hu3DMotionExec(shadowAlpha, model->motIdShape, model->motShapeWork.time, 1);
                        }
                    }
                    if (((model->attr & 0x90) == 0) || ((model->motAttr & 0x40000002) == 0)) {
                        shadowSlot = 1;
                        InitVtxParm(model->hsf);
                        if (model->motIdShape != -1) {
                            ShapeProc(model->hsf);
                        }
                        if ((model->attr & 0x400) != 0) {
                            ClusterProc(model);
                        }
                        if (model->hsf->cenvNum != 0) {
                            EnvelopeProc(model->hsf);
                        }
                        PPCSync();
                    }
                    model->attr |= 0x800;
                }
                Hu3DModelShadowSet(shadowState->shadowModelIds[shadowAlpha]);
                Hu3DShadowData.alpha = (s8)(shadowState->shadowAlpha[shadowAlpha] * savedShadowAlpha);
                mtxRot(shadowCameraMatrix, model->rot.x, model->rot.y, model->rot.z);
                MTXScale(shadowProjection, model->scale.x, model->scale.y, model->scale.z);
                MTXConcat(shadowCameraMatrix, shadowProjection, shadowProjection);
                mtxTransCat(shadowProjection, model->pos.x, model->pos.y, model->pos.z);
                MTXConcat(Hu3DCameraMtx, shadowProjection, identityMatrix);
                MTXConcat(identityMatrix, model->mtx, identityMatrix);
                Hu3DDraw(model, identityMatrix, &model->scale);
                Hu3DModelShadowReset(shadowState->shadowModelIds[shadowAlpha]);
            }
        }
    }
    Hu3DDrawPost();
    GXDrawDone();
    shadowModelDrawF = 0;
    if (Hu3DShadowData.size <= 0xF0) {
        GXSetTexCopySrc(0, 0, Hu3DShadowData.size * 2, Hu3DShadowData.size * 2);
        GXSetTexCopyDst(Hu3DShadowData.size, Hu3DShadowData.size, GX_CTF_R8, 1);
    }
    else {
        GXSetTexCopySrc(0, 0, Hu3DShadowData.size, Hu3DShadowData.size);
        GXSetTexCopyDst(Hu3DShadowData.size, Hu3DShadowData.size, GX_CTF_R8, 0);
    }
    GXCopyTex(Hu3DShadowData.buf, 1);
    GXSetViewport(0.0f, 0.0f, RenderMode->fbWidth, RenderMode->xfbHeight, 0.0f, 1.0f);
    GXSetScissor(0, 0, RenderMode->fbWidth, RenderMode->efbHeight);
    C_MTXOrtho(shadowProjectionMatrix, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    GXSetProjection(shadowProjectionMatrix, GX_ORTHOGRAPHIC);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_U8, 0);
    GXSetTevColor(GX_TEVREG0, BGColor);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumChans(0);
    MTXIdentity(identityMatrix);
    GXLoadPosMtxImm(identityMatrix, 0);
    GXSetZMode(0, GX_ALWAYS, 1);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition1x8(0);
    GXPosition1x8(0);
    GXPosition1x8(0);
    GXPosition1x8(1);
    GXPosition1x8(0);
    GXPosition1x8(0);
    GXPosition1x8(1);
    GXPosition1x8(1);
    GXPosition1x8(0);
    GXPosition1x8(0);
    GXPosition1x8(1);
    GXPosition1x8(0);
}

void DrawCameraShadowOverlay(void)
{
    Mtx44 orthographicProjection;
    Mtx modelViewMatrix;
    GXTexObj shadowTexture;
    HU3DSHADOW *shadowCamera;
    GXColor shadowColor;
    M428DllShadowHookWork *shadowWork;
    M428DllShadowCameraState *shadowState;
    HU3DMODEL *model;
    u8 *shadowTextureData;

    if (shadowModelId != -1) {
        model = &Hu3DData[shadowModelId];
        shadowWork = model->hookData;
        shadowState = &shadowWork->cameraShadowStates[Hu3DCameraNo];
        shadowCamera = &shadowWork->cameraShadowData[Hu3DCameraNo];
        if (shadowState->activeShadowCount > 0) {
            GXInitTexObj(&shadowTexture, Hu3DShadowData.buf, Hu3DShadowData.size, Hu3DShadowData.size, GX_TF_I8, GX_CLAMP, GX_CLAMP, 0);
            GXInitTexObjLOD(&shadowTexture, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
            GXLoadTexObj(&shadowTexture, GX_TEXMAP0);
            C_MTXOrtho(orthographicProjection, 0.0f, 480.0f, 0.0f, 320.0f, 0.0f, 10.0f);
            GXSetProjection(orthographicProjection, GX_ORTHOGRAPHIC);
            GXClearVtxDesc();
            GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
            GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
            shadowColor.a = 0x80;
            GXSetTevColor(GX_TEVREG0, shadowColor);
            GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, GX_FALSE, 0x7D);
            GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
            GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0);
            GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetNumTexGens(1);
            GXSetNumTevStages(1);
            MTXIdentity(modelViewMatrix);
            GXLoadPosMtxImm(modelViewMatrix, 0);
            GXSetZMode(0, GX_LEQUAL, GX_FALSE);
            GXBegin(GX_QUADS, GX_VTXFMT0, 4);
            GXPosition3f32(0.0f, 0.0f, 0.0f);
            GXTexCoord2f32(0.0f, 0.0f);
            GXPosition3f32(Hu3DShadowData.size, 0.0f, 0.0f);
            GXTexCoord2f32(1.0f, 0.0f);
            GXPosition3f32(Hu3DShadowData.size, Hu3DShadowData.size, 0.0f);
            GXTexCoord2f32(1.0f, 1.0f);
            GXPosition3f32(0.0f, Hu3DShadowData.size, 0.0f);
            GXTexCoord2f32(0.0f, 1.0f);
        }
    }
}

s32 shadowRenderSetupData[] = {
    0,
    0,
    0x45ABE000,
    0xC576E000,
    0,
    0xBF333333,
    0x3F19999A,
    0xFFFFFFFF,
    0,
    0x45ABE000,
    0xC576E000,
    0,
    0x3F800000,
    0,
    0,
    0,
    0,
};

void DestroyMultiCameraShadows(void)
{
    M428DllShadowHookWork *shadowWork;
    HU3DMODEL *model;

    model = &Hu3DData[shadowModelId];
    shadowWork = model->hookData;
    Hu3DShadowData = shadowWork->originalShadowData;
    Hu3DModelKill(shadowModelId);
    shadowModelId = -1;
    if (Hu3DShadowCamBit > 0) {
        Hu3DShadowCamBit--;
    }
}

void AddModelToCameraShadows(s16 modelId, s32 shadowCameraMask)
{
    HU3DSHADOW *shadowCamera;
    M428DllShadowCameraState *shadowState;
    s32 shadowSlot;
    s32 cameraIndex;
    M428DllShadowHookWork *shadowWork;
    HU3DMODEL *model;
    u32 shadowCameraBit;

    model = &Hu3DData[shadowModelId];
    shadowWork = model->hookData;
    for (cameraIndex = 0; cameraIndex < 0x10; cameraIndex++) {
        shadowState = &shadowWork->cameraShadowStates[cameraIndex];
        shadowCamera = &shadowWork->cameraShadowData[cameraIndex];
        shadowCameraBit = 1 << cameraIndex;
        if ((shadowCameraBit & shadowCameraMask) != 0) {
            for (shadowSlot = 0; shadowSlot < 0x10; shadowSlot++) {
                if (modelId != shadowState->shadowModelIds[shadowSlot]) {
                    if (shadowState->shadowModelIds[shadowSlot] == -1) {
                        shadowState->shadowModelIds[shadowSlot] = modelId;
                        shadowState->shadowAlpha[shadowSlot] = 1.0f;
                        shadowState->activeShadowCount++;
                        break;
                    }
                }
                else {
                    break;
                }
            }
        }
    }
}

void SetModelCameraShadowAlpha(s16 modelId, s32 shadowCameraMask, Float shadowAlpha)
{
    HU3DSHADOW *shadowCamera;
    s32 cameraIndex;
    s32 shadowSlot;
    M428DllShadowCameraState *shadowState;
    M428DllShadowHookWork *shadowWork;
    HU3DMODEL *model;
    u32 shadowCameraBit;

    model = &Hu3DData[shadowModelId];
    shadowWork = model->hookData;
    for (cameraIndex = 0; cameraIndex < 0x10; cameraIndex++) {
        shadowState = &shadowWork->cameraShadowStates[cameraIndex];
        shadowCamera = &shadowWork->cameraShadowData[cameraIndex];
        shadowCameraBit = 1 << cameraIndex;
        if ((shadowCameraBit & shadowCameraMask) != 0) {
            for (shadowSlot = 0; shadowSlot < 0x10; shadowSlot++) {
                if (modelId == shadowState->shadowModelIds[shadowSlot]) {
                    shadowState->shadowAlpha[shadowSlot] = shadowAlpha;
                }
            }
        }
    }
}

void SetCameraShadowView(s32 cameraIndex, Vec *cameraPosition, Vec *cameraUp, Vec *cameraTarget)
{
    M428DllShadowCameraState *shadowState;
    HU3DSHADOW *shadowCamera;
    M428DllShadowHookWork *shadowWork;
    HU3DMODEL *model;

    model = &Hu3DData[shadowModelId];
    shadowWork = model->hookData;
    shadowState = &shadowWork->cameraShadowStates[cameraIndex];
    shadowCamera = &shadowWork->cameraShadowData[cameraIndex];
    shadowCamera->camPos = *cameraPosition;
    shadowCamera->camTarget = *cameraTarget;
    shadowCamera->camUp = *cameraUp;
}
