#include "game/audio.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "REL/m434Dll.h"
#include "ext_math.h"
#include "game/gamework_data.h"
#include "game/minigame_seq.h"
#include "math.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#include <string.h>
#endif

typedef struct CameraView {
    float zoom;
    Vec pos;
    Vec rot;
} CameraView;

Vec keyLightPosition = { -700, 2500, -700 };
Vec keyLightDirection = { 0.3, -0.8, 0.3 };
Vec cameraFocusPosition = { 34.75, -24.75, 3500 };
GXColor keyLightColor = { 255, 255, 255, 255 };
Vec fillLightPosition = { -700, 2500, 700 };
Vec fillLightDirection = { 0, 1, 0 };
Vec originPosition = { 0, 0, 0 };

OMOBJ *mainControllerObject;
OMOBJ *roundControllerObject;
OMOBJ *outOfViewObject;
OMOBJ *backgroundFishObject;
OMOBJ *teamScoreObject;
s16 roundTimerSeconds;
s16 roundTimerFrameCounter;
s32 minigameSequenceId;
s32 roundTimerSequenceId;
s16 teamWinnerCharacters[3];
s16 keyLightId;
s16 winningTeamCharacters[2];
s16 winningTeam;
s16 roundFinished;
s16 playerTeamAssignments[4];
s32 cameraStackDepth;
CameraView cameraStack[32];
s32 cameraTransitionMode;
s32 roundStateTimers[2];
s32 backgroundFishState[2];
s32 backgroundFishFrame;
float backgroundFishTransitionProgress;

void InitializeRoundState(OMOBJ *object);
void InitializeBackgroundFish(OMOBJ *object);
s32 GetBackgroundFishState(void);
void CreateTeamScoreDisplay(HUPROCESS *objman);
void FreeTeamScoreDisplay(void);
void InitializeCameraStack(void);

void ObjectSetup(void)
{
    s32 i;
    HUPROCESS *objman;
    OSReport("******* M434ObjectSetup *********\n");
    objman = omInitObjMan(50, 8192);
    omGameSysInit(objman);
    CZoom = 2097.6f;
    Center.x = 0;
    Center.y = 172;
    Center.z = 361;
    CRot.x = -32;
    CRot.y = 0;
    CRot.z = 0;
    for (i = 0; i < 4; i++) {
        CZoomM[i] = CZoom;
        CenterM[i] = Center;
        CRotM[i] = CRot;
    }
    InitializeCameraStack();
    Hu3DCameraCreate(15);
    Hu3DCameraPerspectiveSet(2, 30, 20, 25000, 1.2f);
    Hu3DCameraViewportSet(2, 0, 0, 320, 240, 0, 1);
    Hu3DCameraPerspectiveSet(13, 30, 20, 25000, 1.2f);
    Hu3DCameraViewportSet(13, 0, 0, 640, 480, 0, 1);
    outOfViewObject = omAddObjEx(objman, 32730, 0, 0, -1, omOutViewMulti);
    outOfViewObject->work[0] = 4;
    HuAudSndGrpSet(59);
    keyLightDirection.x = -keyLightPosition.x;
    keyLightDirection.y = -keyLightPosition.y;
    keyLightDirection.z = -keyLightPosition.z;
    VECNormalize(&keyLightDirection, &keyLightDirection);
    keyLightId = Hu3DGLightCreateV(&keyLightPosition, &keyLightDirection, &keyLightColor);
    Hu3DGLightInfinitytSet(keyLightId);
    Hu3DGLightStaticSet(keyLightId, 1);
    for (i = 0; i < 4; i++) {
        playerTeamAssignments[i] = GWPlayerCfg[i].group;
    }
    mainControllerObject = omAddObjEx(objman, 0, 0, 0, -1, InitializeRoundState);
    roundControllerObject = omAddObjEx(objman, 0, 1, 0, -1, InitializeBackgroundFish);
    CreateEnvironmentObject(objman);
    CreatePlayerObjects(objman);
    InitializeFishManager(objman);
    CreateTeamScoreDisplay(objman);
}

void UpdateRoundFlow(OMOBJ *object);

void InitializeRoundState(OMOBJ *object)
{
    SetRoundState(0);
    winningTeam = 0;
    roundFinished = 0;
    object->objFunc = UpdateRoundFlow;
}

s32 backgroundMusicSequenceId = -1;

void ExitMinigame(OMOBJ *object);
void ShowTeamScores(s32 unusedDisplayMode);

void UpdateRoundFlow(OMOBJ *object)
{
    s32 playerIndex;
    s32 teamIndex;
    s32 teamPlayerIndices[2][4];
    s32 teamPlayerCounts[2];

    switch (GetRoundState()) {
        case 0:
            roundTimerSeconds = 60;
            roundTimerFrameCounter = REFRESH_RATE;
            SetRoundState(1);
            roundStateTimers[1] = REFRESH_RATE;
            WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
            HuAudFXPlay(1736);
            break;

        case 1:
            if (GetBackgroundFishState() == 2) {
                if (roundStateTimers[1] != 0) {
                    if (--roundStateTimers[1] == 0) {
                        minigameSequenceId = MGSeqStartCreate();
                        HuAudStreamPlay("sound/mu_047a.adp", 1);
                    }
                }
                else {
                    if (MGSeqStatGet(minigameSequenceId) == 0) {
                        SetRoundState(2);
                        HuAudFXPlay(1738);
                        roundTimerSequenceId = MGSeqTimerCreate(60);
                        ShowTeamScores(1);
                    }
                    if (backgroundMusicSequenceId < 0 && (MGSeqStatGet(minigameSequenceId) & 0x10)) {
                        backgroundMusicSequenceId = HuAudSeqPlay(67);
                    }
                }
            }
            break;

        case 2:
            if (--roundTimerFrameCounter == 0) {
                roundTimerSeconds--;
                MGSeqParamSet(roundTimerSequenceId, 1, roundTimerSeconds);
                roundTimerFrameCounter = REFRESH_RATE;
            }
            if (!roundTimerSeconds || roundFinished == 1) {
                SetRoundState(3);
                roundStateTimers[1] = 60;
                minigameSequenceId = MGSeqFinishCreate();
                HuAudStreamFadeOut(30);
                HuAudSeqFadeOut(backgroundMusicSequenceId, 100);
            }
            break;

        case 3:
            if (roundTimerSequenceId >= 0) {
                MGSeqParamSet(roundTimerSequenceId, 2, -1);
                roundTimerSequenceId = -1;
            }
            if (MGSeqStatGet(minigameSequenceId) == 0) {
                SetRoundState(4);
                roundStateTimers[1] = REFRESH_RATE;
            }
            break;

        case 4:
            if (--roundStateTimers[1] == 0) {
                teamPlayerCounts[0] = teamPlayerCounts[1] = 0;
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    teamIndex = GetPlayerTeam(playerIndex);
                    teamPlayerIndices[teamIndex][teamPlayerCounts[teamIndex]++] = playerIndex;
                }
                winningTeam = GetWinningTeam();
                switch (winningTeam) {
                    case -1:
                        winningTeamCharacters[0] = GWPlayerCfg[teamPlayerIndices[0][0]].character;
                        winningTeamCharacters[1] = GWPlayerCfg[teamPlayerIndices[0][1]].character;
                        MGSeqWinCreate(winningTeamCharacters[0], winningTeamCharacters[1], -1, -1);
                        GWPlayerCoinWinSet(teamPlayerIndices[0][0], 10);
                        GWPlayerCoinWinSet(teamPlayerIndices[0][1], 10);
                        HuAudSStreamPlay(1);
                        break;

                    case 0:
                        MGSeqDrawCreate();
                        HuAudSStreamPlay(4);
                        break;

                    case 1:
                        winningTeamCharacters[0] = GWPlayerCfg[teamPlayerIndices[1][0]].character;
                        winningTeamCharacters[1] = GWPlayerCfg[teamPlayerIndices[1][1]].character;
                        MGSeqWinCreate(winningTeamCharacters[0], winningTeamCharacters[1], -1, -1);
                        GWPlayerCoinWinSet(teamPlayerIndices[1][0], 10);
                        GWPlayerCoinWinSet(teamPlayerIndices[1][1], 10);
                        HuAudSStreamPlay(1);
                        break;
                }
                SetRoundState(5);
                roundStateTimers[1] = 3.5 * REFRESH_RATE;
            }
            break;

        case 5:
            if (--roundStateTimers[1] == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = ExitMinigame;
            }

            break;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        CZoomM[playerIndex] = CZoom;
        CRotM[playerIndex] = CRot;
        CenterM[playerIndex] = Center;
    }
    if (omSysExitReq && !WipeStatGet()) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = ExitMinigame;
    }
}

void ExitMinigame(OMOBJ *object)
{
    if (WipeStatGet()) {
        return;
    }
    ResetEnvironmentTextures();
    FreePlayerCharacterModels();
    FreeFishModels();
    FreeTeamScoreDisplay();
    MGSeqKillAll();
    HuAudFadeOut(1);
    omOvlReturnEx(1, 1);
    OSReport("******* M434Exit *********\n");
}

void UpdateBackgroundFish(OMOBJ *object);
void PushCameraSettings(void);
void SetCameraViewFromValues(float zoom, float positionX, float positionY, float positionZ, float rotationX, float rotationY, float rotationZ);
void SetCameraTransitionMode(s32 transitionMode);
void InterpolateCameraView(float transitionProgress);

void InitializeBackgroundFish(OMOBJ *object)
{
    backgroundFishState[0] = 0;
    backgroundFishState[1] = 140;
    PushCameraSettings();
    SetCameraViewFromValues(302, 0, 125, -188, -24, 28, 0);
    SetCameraTransitionMode(1);
    object->objFunc = UpdateBackgroundFish;
    omSetStatBit(object, 0x100);
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M434, 0x07));
    Hu3DModelCameraSet(object->mdlId[0], 4);
    Hu3DModelLayerSet(object->mdlId[0], 4);
    Hu3DModelPosSet(object->mdlId[0], -50, -40, -540);
    Hu3DMotionSpeedSet(object->mdlId[0], 1);
    Hu3DModelRotSet(object->mdlId[0], 0, 70, 0);
}

s32 backgroundFishSplashFrames[] = { 74, 98 };
float backgroundFishSplashOffsets[] = { -120, -267.9 };

void FinishBackgroundFishAnimation(OMOBJ *object);

void UpdateBackgroundFish(OMOBJ *object)
{
    HU3DMODEL *modelData;
    s32 effectIndex;
    Vec effectPosition;
    switch (backgroundFishState[0]) {
        case 0:
            if (--backgroundFishState[1] == 0) {
                Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
                backgroundFishState[0] = 1;
            }
            for (effectIndex = 0; effectIndex < 2u; effectIndex++) {
                if (backgroundFishFrame == backgroundFishSplashFrames[effectIndex]) {
                    modelData = &Hu3DData[object->mdlId[0]];
                    effectPosition.x = modelData->pos.x + (backgroundFishSplashOffsets[effectIndex] * cosd(modelData->rot.y));
                    effectPosition.y = -15.000001f;
                    effectPosition.z = modelData->pos.z - (backgroundFishSplashOffsets[effectIndex] * sind(modelData->rot.y));
                    if (effectIndex == 0) {
                        CreateWaterSplash(&effectPosition);
                        HuAudFXPlay(1731);
                    }
                    else {
                        ClampPositionToWaterline(&effectPosition);
                        HuAudFXPlay(1739);
                    }
                }
            }
            backgroundFishFrame++;
            break;

        case 1:
            backgroundFishTransitionProgress += 0.02f;
            if (backgroundFishTransitionProgress >= 1.0f) {
                backgroundFishState[0] = 2;
                backgroundFishTransitionProgress = 1.0f;
                object->objFunc = FinishBackgroundFishAnimation;
            }
            InterpolateCameraView(backgroundFishTransitionProgress);
            break;
    }
}

void PopCameraSettings(void);

void FinishBackgroundFishAnimation(OMOBJ *object)
{
    PopCameraSettings();
    PopCameraSettings();
    object->objFunc = NULL;
}

s32 GetBackgroundFishState(void)
{
    return backgroundFishState[0];
}

void InitializeTeamScoreDisplay(OMOBJ *object);

u16 scoreSpriteTypes[6] = { 0, 1, 2, 3, 4, 4 };

float scoreSpriteOffsets[6][2] = { 0, 0, 0, 0, -33, -2, -9, -2, 11, -2, 31, -2 };

float teamScoreOrigins[2][2] = { 104, 56, 472, 56 };

typedef struct TeamScoreDisplayWork {
    s32 displayState;
    s16 score;
    s16 digitSprites[6];
    u16 scorePulseFrames;
} TeamScoreDisplay;

void CreateTeamScoreDisplay(HUPROCESS *objman)
{
    teamScoreObject = omAddObjEx(objman, 0, 0, 0, -1, InitializeTeamScoreDisplay);
}

void UpdateTeamScoreDisplay(OMOBJ *object);

void InitializeTeamScoreDisplay(OMOBJ *object)
{
    TeamScoreDisplay *teamDisplay;
    u32 spriteIndex;
    s32 teamIndex;
    teamDisplay = HuMemDirectMallocNum(HEAP_HEAP, 2 * sizeof(TeamScoreDisplay), HU_MEMNUM_OVL);
    object->data = teamDisplay;
    memset(object->data, 0, 2 * sizeof(TeamScoreDisplay));
    for (teamIndex = 0; teamIndex < 2; teamDisplay++, teamIndex++) {
        for (spriteIndex = 0; spriteIndex < 6; spriteIndex++) {
            switch (scoreSpriteTypes[spriteIndex]) {
                case 0:
                    teamDisplay->digitSprites[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x1D), 1, 0);
                    espPriSet(teamDisplay->digitSprites[spriteIndex], 256);
                    espColorSet(teamDisplay->digitSprites[spriteIndex], 0, 0, 0);
                    espTPLvlSet(teamDisplay->digitSprites[spriteIndex], 0.5);
                    break;

                case 1:
                    teamDisplay->digitSprites[spriteIndex]
                        = espEntry((teamIndex == 0) ? DATA_MAKE_NUM(DATADIR_MGCONST, 0x1B) : DATA_MAKE_NUM(DATADIR_MGCONST, 0x1C), 1, 0);
                    espPriSet(teamDisplay->digitSprites[spriteIndex], 256);
                    break;

                case 2:
                    teamDisplay->digitSprites[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_M434, 0x0C), 1, 0);
                    espPriSet(teamDisplay->digitSprites[spriteIndex], 256);
                    espScaleSet(teamDisplay->digitSprites[spriteIndex], 0.75, 0.75);
                    break;

                case 3:
                    teamDisplay->digitSprites[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_M434, 0x0D), 1, 0);
                    espPriSet(teamDisplay->digitSprites[spriteIndex], 256);
                    break;

                case 4:
                    teamDisplay->digitSprites[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x33), 1, 0);
                    espPriSet(teamDisplay->digitSprites[spriteIndex], 255);
                    break;
            }
            espDispOff(teamDisplay->digitSprites[spriteIndex]);
            espDrawNoSet(teamDisplay->digitSprites[spriteIndex], 0);
            espAttrSet(teamDisplay->digitSprites[spriteIndex], HUSPR_ATTR_NOANIM);
            espPosSet(teamDisplay->digitSprites[spriteIndex], teamScoreOrigins[teamIndex][0] + scoreSpriteOffsets[spriteIndex][0],
                teamScoreOrigins[teamIndex][1] + scoreSpriteOffsets[spriteIndex][1]);
        }
        teamDisplay->displayState = -1;
    }
    HuDataDirClose(DATADIR_MGCONST);
    object->objFunc = UpdateTeamScoreDisplay;
}

void UpdateTeamScoreDisplay(OMOBJ *object)
{
    float pulseScale;
    TeamScoreDisplay *teamDisplay;
    s32 teamIndex;
    s32 spriteIndex;
    teamDisplay = object->data;
    for (spriteIndex = 0; spriteIndex < 2; spriteIndex++, teamDisplay++) {
        if (teamDisplay->displayState >= 0) {
            if (teamDisplay->displayState == 0) {
                for (teamIndex = 0; teamIndex < 6; teamIndex++) {
                    espDispOn(teamDisplay->digitSprites[teamIndex]);
                }
                teamDisplay->displayState++;
            }
            if (teamDisplay->scorePulseFrames) {
                teamDisplay->scorePulseFrames--;
                pulseScale = (0.5 * sind((180.0f * teamDisplay->scorePulseFrames) / 16.0f)) + 1.0;
                espScaleSet(teamDisplay->digitSprites[4], pulseScale, pulseScale);
                espScaleSet(teamDisplay->digitSprites[5], pulseScale, pulseScale);
            }
            espBankSet(teamDisplay->digitSprites[4], teamDisplay->score / 10);
            espBankSet(teamDisplay->digitSprites[5], teamDisplay->score % 10);
        }
    }
}

void ShowTeamScores(s32 unusedDisplayMode)
{
    TeamScoreDisplay *teamDisplay;
    s32 teamIndex;
    teamDisplay = teamScoreObject->data;
    for (teamIndex = 0; teamIndex < 2; teamIndex++, teamDisplay++) {
        teamDisplay->displayState = 0;
    }
}

void FreeTeamScoreDisplay(void)
{
    HuMemDirectFree(teamScoreObject->data);
}

void InitializeCameraStack(void)
{
    cameraStackDepth = 0;
    cameraTransitionMode = 0;
}

void PushCameraSettings(void)
{
    CameraView *cameraView = &cameraStack[cameraStackDepth++];
    cameraView->zoom = CZoom;
    cameraView->pos.x = Center.x;
    cameraView->pos.y = Center.y;
    cameraView->pos.z = Center.z;
    cameraView->rot.x = CRot.x;
    cameraView->rot.y = CRot.y;
    cameraView->rot.z = CRot.z;
}

void PopCameraSettings(void)
{
    CameraView *cameraView = &cameraStack[--cameraStackDepth];
    CZoom = cameraView->zoom;
    Center.x = cameraView->pos.x;
    Center.y = cameraView->pos.y;
    Center.z = cameraView->pos.z;
    CRot.x = cameraView->rot.x;
    CRot.y = cameraView->rot.y;
    CRot.z = cameraView->rot.z;
}

void SetCameraView(CameraView *cameraView)
{
    CameraView *savedCameraView;
    CameraView cameraViewCopy;
    cameraViewCopy = cameraStack[cameraStackDepth++];
    savedCameraView = &cameraViewCopy;
    memcpy(savedCameraView, cameraView, sizeof(CameraView));
    CZoom = cameraView->zoom;
    Center.x = cameraView->pos.x;
    Center.y = cameraView->pos.y;
    Center.z = cameraView->pos.z;
    CRot.x = cameraView->rot.x;
    CRot.y = cameraView->rot.y;
    CRot.z = cameraView->rot.z;
}

void SetCameraViewFromValues(float zoom, float positionX, float positionY, float positionZ, float rotationX, float rotationY, float rotationZ)
{
    CameraView *cameraView = &cameraStack[cameraStackDepth++];
    cameraView->zoom = CZoom = zoom;
    cameraView->pos.x = Center.x = positionX;
    cameraView->pos.y = Center.y = positionY;
    cameraView->pos.z = Center.z = positionZ;
    cameraView->rot.x = CRot.x = rotationX;
    cameraView->rot.y = CRot.y = rotationY;
    cameraView->rot.z = CRot.z = rotationZ;
}

void SetCameraTransitionMode(s32 transitionMode)
{
    cameraTransitionMode = transitionMode;
}

#define LERP_PARAM(view1, view2, field, weight) ((view1)->field +)

void InterpolateCameraView(float transitionProgress)
{
    float weight;
    CameraView *view1 = &cameraStack[cameraStackDepth - 1];
    CameraView *view2 = view1 - 1;
    if (cameraStackDepth < 2) {
        return;
    }
    switch (cameraTransitionMode) {
        case 0:
            weight = transitionProgress;
            break;

        case 1:
            weight = sind(90.0f * transitionProgress);
            break;

        case 2:
            weight = sind(90.0f * transitionProgress);
            weight *= weight;
            break;
    }

    CZoom = view1->zoom + (weight * (view2->zoom - view1->zoom));
    Center.x = view1->pos.x + (weight * (view2->pos.x - view1->pos.x));
    Center.y = view1->pos.y + (weight * (view2->pos.y - view1->pos.y));
    Center.z = view1->pos.z + (weight * (view2->pos.z - view1->pos.z));
    CRot.x = view1->rot.x + (weight * (view2->rot.x - view1->rot.x));
    CRot.y = view1->rot.y + (weight * (view2->rot.y - view1->rot.y));
    CRot.z = view1->rot.z + (weight * (view2->rot.z - view1->rot.z));
}

void AddTeamScore(s32 teamIndex, s32 pointsToAdd)
{
    TeamScoreDisplay *teamDisplay = &((TeamScoreDisplay *)teamScoreObject->data)[teamIndex];
    teamDisplay->score += pointsToAdd;
    teamDisplay->scorePulseFrames = 16;
}

s32 GetTeamScore(s32 teamIndex)
{
    TeamScoreDisplay *firstTeamScore = &((TeamScoreDisplay *)teamScoreObject->data)[teamIndex];
    return firstTeamScore->score;
}

s32 GetWinningTeam(void)
{
    s32 firstTeamScore = GetTeamScore(0);
    s32 secondTeamScore = GetTeamScore(1);
    if (firstTeamScore == secondTeamScore) {
        return 0;
    }
    if (firstTeamScore > secondTeamScore) {
        return -1;
    }
    else {
        return 1;
    }
}

s16 GetRoundTimer(void)
{
    return roundTimerSeconds;
}

s32 GetRoundState(void)
{
    return roundStateTimers[0];
}

void SetRoundState(s32 roundState)
{
    roundStateTimers[0] = roundState;
}

s32 GetPlayerTeam(s32 playerIndex)
{
    return playerTeamAssignments[playerIndex];
}

s32 GetRoundWinner(void)
{
    return winningTeam;
}

void SetRoundFinished(void)
{
    roundFinished = 1;
}
