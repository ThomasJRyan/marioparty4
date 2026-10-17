#include "dolphin/gx/GXVert.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/flag.h"
#include "game/gamework.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include "math.h"

#include "string.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/frand.h"
#endif

#include "REL/m443Dll.h"

typedef struct M443DllWork {
    u32 gameState;
    s32 stateFrameCount;
    s32 isExiting;
    s32 cameraTransitionState;
    s32 mainFrameCount;
    u32 resultStage;
    u32 resultStageFrameCount;
    u32 countdownFramesRemaining;
    u32 countdownFrameTimer;
    char reservedWorkPadding[4];
    s32 rewardSequenceState;
    u32 rewardSequenceFrameCount;
    s32 winningPlayerIndex;
    s32 playerRankByIndex[4];
    float playerScores[4];
    u32 scoreSubmissionState;
    u32 finishEventFlags;
    s32 playerEventFlags;
} M443DllWork; /* size = 0x60 */

typedef struct M443DllSpriteConfig {
    u32 spriteResourceId;
    s32 spriteFrameCount;
} M443DllSpriteConfig; /* size = 0x08 */

typedef struct M443DllOverlayWork {
    s32 isVisible;
    s32 isScoreAnimating;
    float positionX;
    float positionY;
    u32 recordTimeAnimationState;
    u32 scoreValue;
    u32 animationFrame;
    s16 spriteIds[0x10];
} M443DllOverlayWork; /* size = 0x3C */

void UpdateMainSequence(OMOBJ *object);
void UpdateResultsSequence(OMOBJ *object);
OMOBJFUNC StartCameraProcess(HUPROCESS *process, OMOBJ *object);
void UpdateCameraSequence(OMOBJ *object);
OMOBJFUNC StartResultsProcess(HUPROCESS *process, OMOBJ *object);
void UpdateResultsProcess(OMOBJ *object);
void InitializeScoreOverlay(M443DllOverlayWork *overlay);
void UpdateScoreOverlay(M443DllOverlayWork *overlay);
void InitializePlayerObjects(HUPROCESS *process);
void UpdatePlayerObjects(OMOBJ *object);
void DestroyPlayerObjects(OMOBJ *object);
void UpdatePlayerObjectAnimations(OMOBJ *object);
void DrawShadowPlane(HU3DMODEL *model, float matrix[3][4]);
void InitializeMainObject(OMOBJ *object);
void AdvanceGameState(OMOBJ *object);
void UpdateIdleState(OMOBJ *object);
void UpdateCountdownState(OMOBJ *object);
void UpdateResultsState(OMOBJ *object);
void UpdateRewardState(OMOBJ *object);

Vec directionalLightPosition = { 100.0f, 800.0f, -100.0f };

Vec directionalLightDirection = { 0.3f, -0.8f, 0.3f };

Vec shadowLightPosition = { 10.0f, 45.0f, 3500.0f };

GXColor whiteLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };

Vec shadowVolume = { 50.0f, 150000.0f, 50.0f };

Vec shadowUpVector = { 0.0f, 1.0f, 0.0f };

Vec shadowTarget = { 0.0f, 0.0f, 0.0f };

HUPROCESS *mainObjectManager;
OMOBJ *mainObject;
OMOBJ *cameraUpdateObject;
OMOBJ *cameraObject;
s16 selectedCameraMask;
s16 cameraMaskByPlayer[4];
s16 resultSequenceId;
s16 minigameSequenceId;
s16 scoreSequenceId;
s16 rewardSequenceId;
s32 activePlayerMask;
M443DllOverlayWork recordTimeOverlay;
s16 cameraViewportSprites[1]; // why only 1 long?
s32 playerObjectId;
s32 mapObjectId;
s32 scoreObjectId;
s32 resultsObjectId;

void ObjectSetup(void)
{
    Vec lightTarget;
    Vec lightPosition;
    HUPROCESS *objectManager;
    HU3DLIGHT *globalLight;

    HuAudSndGrpSet(0x45);
    playerObjectId = mapObjectId = -1;
    scoreObjectId = resultsObjectId = -1;
    cameraMaskByPlayer[0] = Hu3DGLightCreateV(&directionalLightPosition, &directionalLightDirection, &whiteLightColor);
    Hu3DGLightInfinitytSet(cameraMaskByPlayer[0]);
    globalLight = &Hu3DGlobalLight[cameraMaskByPlayer[0]];
    globalLight->type |= 0x8000;

    shadowLightPosition.x = 55.74f;
    shadowLightPosition.y = 0.0f;
    shadowLightPosition.z = 3500.0f;

    lightTarget.x = lightTarget.y = lightTarget.z = 0.0f;
    lightPosition.x = lightPosition.z = 0.0f;
    lightPosition.y = shadowLightPosition.z;
    lightPosition.x = 0.0f;
    lightPosition.y = 3480.0002f;
    lightPosition.z = 900.00006f;
    Hu3DGLightPosAimSetV(cameraMaskByPlayer[0], &lightPosition, &lightTarget);

    shadowVolume.x = 100.0f;
    shadowVolume.y = 2000.0f;
    shadowVolume.z = 2000.0f;
    shadowUpVector.x = 0.0f;
    shadowUpVector.y = 1.0f;
    shadowUpVector.z = 0.0f;
    shadowTarget.x = 0.0f;
    shadowTarget.y = 0.0f;
    shadowTarget.z = 700.0f;
    Hu3DShadowCreate(30.0f, 20.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.625f);
    Hu3DShadowPosSet(&shadowVolume, &shadowUpVector, &shadowTarget);
    resultSequenceId = minigameSequenceId = scoreSequenceId = rewardSequenceId = -1;
    activePlayerMask = 0;
    objectManager = omInitObjMan(0x32, 0x2000);
    mainObjectManager = objectManager;
    omGameSysInit(objectManager);
    InitializeScoreOverlay(&recordTimeOverlay);
    InitializePlayerObjects(objectManager);
    mainObject = omAddObjEx(objectManager, 0xA, 0, 0, -1, InitializeMainObject);
    InitializeMapObject(objectManager);
    InitializePlayerRacers(objectManager);
}

void InitializeMainObject(OMOBJ *object)
{
    M443DllWork *work;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M443DllWork), HU_MEMNUM_OVL);
    work = object->data;
    memset(work, 0, sizeof(M443DllWork));
    work->gameState = 0;
    work->stateFrameCount = 0;
    work->cameraTransitionState = 0;
    work->mainFrameCount = 0;
    work->resultStage = 0;
    work->resultStageFrameCount = 0;
    work->rewardSequenceState = 0;
    work->rewardSequenceFrameCount = 0;
    work->isExiting = 0;
    work->scoreSubmissionState = 2;
    work->finishEventFlags = 0;
    work->playerEventFlags = 0;
    object->objFunc = UpdateIdleState;
}

void UpdateMainObject(OMOBJ *object)
{
    M443DllWork *work;

    work = object->data;
    work->stateFrameCount++;
    work->mainFrameCount++;
    work->resultStageFrameCount++;
    work->rewardSequenceFrameCount++;
    UpdateScoreOverlay(&recordTimeOverlay);
    if ((omSysExitReq != 0) && (work->gameState != 5)) {
        work->gameState = 4;
        AdvanceGameState(object);
    }
}

void AdvanceGameState(OMOBJ *object)
{
    M443DllWork *work = object->data;

    if (work->isExiting == 0) {
        switch (work->gameState) {
            case 0:
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
                recordTimeOverlay.isEnabled = 1;
                work->gameState = 1;
                work->mainFrameCount = 0;
                object->objFunc = StartCameraProcess(mainObjectManager, object);
                break;
            case 1:
                work->gameState = 2;
                work->resultStageFrameCount = 0;
                SetCameraTransitionState(4);
                object->objFunc = UpdateRewardState;
                break;
            case 2:
                work->gameState = 3;
                work->rewardSequenceFrameCount = 0;
                object->objFunc = StartResultsProcess(mainObjectManager, object);
                break;
            case 3:
            case 4:
                work->isExiting = 1;
                work->gameState = 5;
                object->objFunc = UpdateCountdownState;
                break;
            case 5:
            default:
                work->gameState = 5;
                object->objFunc = UpdateResultsState;
                break;
        }
        work->stateFrameCount = 0;
    }
}

void UpdateIdleState(OMOBJ *object)
{
    UpdateMainObject(object);
    AdvanceGameState(object);
}

void UpdateCountdownState(OMOBJ *object)
{
    M443DllWork *work = object->data;

    UpdateMainObject(object);

    if (WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        if (mapObjectId >= 0) {
            HuAudFXStop(mapObjectId);
            mapObjectId = -1;
        }
        work->isExiting = 0;
        AdvanceGameState(object);
    }
}

void UpdateResultsState(OMOBJ *object)
{
    UpdateMainObject(object);

    if (WipeStatGet() == 0) {
        EmptyPlayerCallback();
        EmptyMapCallback();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void UpdateRewardState(OMOBJ *object)
{
    M443DllWork *work = object->data;

    UpdateMainObject(object);
    switch (work->resultStage) {
        case 0:
            work->countdownFramesRemaining = 0x12C;
            work->countdownFrameTimer = REFRESH_RATE;
            resultSequenceId = MGSeqCreate(3, 0);
            MGSeqPosSet(resultSequenceId, 320.0f, 240.0f);
            selectedCameraMask = -1;
            Center.x = 0.0f;
            Center.y = 200.0f;
            Center.z = 0.0f;
            CRot.x = -30.0f;
            CRot.y = 0.0f;
            CRot.z = 0.0f;
            CZoom = 1900.0f;
            recordTimeOverlay.isEnabled = 1;
            work->resultStage = 1;
            work->resultStageFrameCount = 0;
            break;
        case 1:
            if ((work->resultStageFrameCount > REFRESH_RATE_F) && (playerObjectId >= 0)) {
                HuAudFXFadeOut(playerObjectId, REFRESH_RATE / 2);
                playerObjectId = -1;
            }
            if ((MGSeqStatGet(resultSequenceId) == 0) && (work->isExiting == 0)) {
                work->resultStage = 2;
                work->resultStageFrameCount = 0;
                object->objFunc = UpdateMainSequence;
            }
            break;
    }
}

void UpdateMainSequence(OMOBJ *object)
{
    s32 playerRankingOrder[4];

    float scoreSwapTemp;

    s32 playerIndex;
    s32 nextPlayerIndex;
    s32 finalizeResults;
    s32 sortPass;
    s32 rankingOrderSwap;

    M443DllWork *work = object->data;
    finalizeResults = 0;
    UpdateMainObject(object);
    recordTimeOverlay.farPlane++;
    if (--work->countdownFrameTimer == 0) {
        work->countdownFrameTimer = REFRESH_RATE;
        work->countdownFramesRemaining--;
        if (work->countdownFramesRemaining == 0) {
            work->scoreSubmissionState = 0;
            finalizeResults = 1;
        }
    }
    if (work->countdownFramesRemaining <= 5) {
        if (selectedCameraMask < 0) {
            selectedCameraMask = MGSeqCreate(1, work->countdownFramesRemaining, -1, -1);
        }
        MGSeqParamSet(selectedCameraMask, 1, work->countdownFramesRemaining);
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (work->playerScores[playerIndex] >= 40000.0f) {
            work->scoreSubmissionState = 1;
        }
    }
    if (work->scoreSubmissionState != 2) {
        finalizeResults = 1;
    }
    if (finalizeResults != 0) {
        if (work->scoreSubmissionState == 1) {
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                playerRankingOrder[playerIndex] = playerIndex;
            }
            for (sortPass = 0; sortPass < 0xA; sortPass++) {
                for (playerIndex = 0; playerIndex < 3; playerIndex++) {
                    for (nextPlayerIndex = playerIndex + 1; nextPlayerIndex < 4; nextPlayerIndex++) {
                        if ((work->playerScores[playerIndex] < work->playerScores[nextPlayerIndex])
                            || ((work->playerScores[playerIndex] == work->playerScores[nextPlayerIndex]) && (frandmod(0x3E8) < 0x2BC))) {
                            scoreSwapTemp = work->playerScores[playerIndex];
                            work->playerScores[playerIndex] = work->playerScores[nextPlayerIndex];
                            work->playerScores[nextPlayerIndex] = scoreSwapTemp;
                            rankingOrderSwap = playerRankingOrder[playerIndex];
                            playerRankingOrder[playerIndex] = playerRankingOrder[nextPlayerIndex];
                            playerRankingOrder[nextPlayerIndex] = rankingOrderSwap;
                        }
                    }
                }
            }
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                work->playerRankByIndex[playerRankingOrder[playerIndex]] = playerIndex;
            }
            work->winningPlayerIndex = playerRankingOrder[0];
        }
        minigameSequenceId = MGSeqCreate(3, 1);
        MGSeqPosSet(minigameSequenceId, 320.0f, 240.0f);
        if (GWPlayerCfg[work->winningPlayerIndex].iscom == 0) {
            if (recordTimeOverlay.farPlane < recordTimeOverlay.nearPlane) {
                GWMGRecordSet(5, recordTimeOverlay.farPlane);
                activePlayerMask = 1;
            }
            if (recordTimeOverlay.farPlane <= REFRESH_RATE * 10) {
                GWGameStat.present[0x39] = 1;
                OSReport("Present For You!!!\n");
            }
        }
        mapObjectId = HuAudFXPlay(0x735);
        SetCameraTransitionState(3);
        work->resultStage = 3;
        work->resultStageFrameCount = 0;
        if (work->isExiting == 0) {
            object->objFunc = UpdateResultsSequence;
        }
    }
}

void UpdateResultsSequence(OMOBJ *object)
{
    M443DllWork *work = object->data;
    s32 cameraTransitionFinished = 0;

    if (selectedCameraMask >= 0) {
        MGSeqParamSet(selectedCameraMask, 2, -1);
        selectedCameraMask = -1;
    }
    UpdateMainObject(object);
    if (work->resultStage == 3) {
        if (work->scoreSubmissionState == 1) {
            M443DllCameraState *winningPlayerCamera = GetMainCamera(work->winningPlayerIndex + 1);
            if (winningPlayerCamera->cameraFlags == 0) {
                winningPlayerCamera->savedViewportCenterX = winningPlayerCamera->viewportCenterX;
                winningPlayerCamera->savedViewportCenterY = winningPlayerCamera->viewportCenterY;
                winningPlayerCamera->savedViewportScale = winningPlayerCamera->viewportScale;
                winningPlayerCamera->sortPriority = 0x3E8;
            }
            if (winningPlayerCamera->cameraFlags <= REFRESH_RATE) {
                float varF31 = REFRESH_FREQ * winningPlayerCamera->cameraFlags;
                winningPlayerCamera->viewportCenterX = winningPlayerCamera->savedViewportCenterX + ((320.0f - winningPlayerCamera->savedViewportCenterX) * varF31);
                winningPlayerCamera->viewportCenterY = winningPlayerCamera->savedViewportCenterY + ((240.0f - winningPlayerCamera->savedViewportCenterY) * varF31);
                winningPlayerCamera->viewportScale = winningPlayerCamera->savedViewportScale + ((2.0f - winningPlayerCamera->savedViewportScale) * varF31);
            }
            else {
                cameraTransitionFinished = 1;
            }
            winningPlayerCamera->cameraFlags = winningPlayerCamera->cameraFlags + 1;
        }
        else {
            cameraTransitionFinished = 1;
        }
        if ((MGSeqStatGet(minigameSequenceId) == 0) && (work->finishEventFlags == 0x1111) && (cameraTransitionFinished != 0)) {
            if (work->isExiting == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, REFRESH_RATE);
            }
            work->resultStage = 4;
            work->resultStageFrameCount = 0;
        }
    }
    else if (WipeStatGet() == 0) {
        work->resultStage = 5;
        work->resultStageFrameCount = 0;
        AdvanceGameState(object);
    }
}

OMOBJFUNC StartCameraProcess(HUPROCESS *process, OMOBJ *object)
{
    CRot.x = -80.5f;
    CRot.y = CRot.z = 0.0f;
    Center.x = 0.0f;
    Center.y = 0.0f;
    Center.z = 0.0f;
    CZoom = 1000.0f;
    object->work[0] = object->work[1] = 0;
    SetCameraTransitionState(1);
    playerObjectId = HuAudFXPlay(0x734);

    return UpdateCameraSequence;
}

Vec cameraViewportSettings[6] = {
    { -500.0f, 150.0f, 350.0f },
    { -500.0f, 150.0f, 350.0f },
    { 500.0f, 150.0f, 350.0f },
    { 500.0f, 150.0f, 350.0f },
    { 0.0f, 150.0f, 350.0f },
    { 0.0f, 150.0f, 350.0f },
};

Vec cameraViewportOffsets[6] = {
    { -1.5f, -40.0f, 880.0f },
    { -1.5f, -40.0f, 880.0f },
    { -20.0f, 40.0f, 1000.0f },
    { -40.0f, 130.0f, 1300.0f },
    { -40.0f, 180.0f, 1500.0f },
    { -40.0f, 180.0f, 1500.0f },
};

s32 cameraMasks[6] = {
    REFRESH_RATE, REFRESH_RATE * 1.5,
    REFRESH_RATE * 1.8, REFRESH_RATE * 2.5,
    REFRESH_RATE * 1.8, 0
};

void UpdateCameraSequence(OMOBJ *object)
{
    Vec interpolatedCameraValue;

    float cameraPathProgress;
    M443DllCameraState *mainCamera;

    M443DllWork *work = object->data;
    UpdateMainObject(object);
    work->cameraTransitionState = 1;
    object->work[0]++;
    if ((object->work[1] == 1) && (scoreObjectId < 0)) {
        scoreObjectId = HuAudSStreamPlay(0xB);
    }
    cameraPathProgress = (float)object->work[0] / cameraMasks[object->work[1]];
    VECSubtract(&cameraViewportSettings[object->work[1] + 1], &cameraViewportSettings[object->work[1]], &interpolatedCameraValue);
    VECScale(&interpolatedCameraValue, &interpolatedCameraValue, cameraPathProgress);
    VECAdd(&cameraViewportSettings[object->work[1]], &interpolatedCameraValue, &interpolatedCameraValue);
    Center.x = interpolatedCameraValue.x;
    Center.y = interpolatedCameraValue.y;
    Center.z = interpolatedCameraValue.z;
    VECSubtract(&cameraViewportOffsets[object->work[1] + 1], &cameraViewportOffsets[object->work[1]], &interpolatedCameraValue);
    VECScale(&interpolatedCameraValue, &interpolatedCameraValue, cameraPathProgress);
    VECAdd(&cameraViewportOffsets[object->work[1]], &interpolatedCameraValue, &interpolatedCameraValue);
    CRot.x = interpolatedCameraValue.x;
    CRot.y = interpolatedCameraValue.y;
    CRot.z = 0.0f;
    CZoom = interpolatedCameraValue.z;
    mainCamera = GetMainCamera(0);
    mainCamera->targetPosition = Center;
    mainCamera->rotation = CRot;
    mainCamera->zoomDistance = CZoom;
    if (object->work[1] == 3) {
        SetCameraTransitionState(2);
    }
    if (cameraPathProgress >= 1.0f) {
        object->work[0] = 0;
        object->work[1] += 1;
        if (cameraMasks[object->work[1]] == 0) {
            work->cameraTransitionState = 2;
            AdvanceGameState(object);
        }
    }
}

OMOBJFUNC StartResultsProcess(HUPROCESS *process, OMOBJ *object)
{
    M443DllCameraState *mainCamera;
    M443DllWork *work = object->data;

    if (work->isExiting == 0) {
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, REFRESH_RATE);
    }
    SetCameraTransitionState(1);
    mainCamera = GetMainCamera(0);
    if (work->scoreSubmissionState == 1) {
        mainCamera->targetPosition.x = 70.0f;
        mainCamera->targetPosition.y = 250.0f;
        mainCamera->targetPosition.z = 500.0f;
    }
    else {
        mainCamera->targetPosition.x = 0.0f;
        mainCamera->targetPosition.y = 150.0f;
        mainCamera->targetPosition.z = 1000.0f;
    }
    mainCamera->rotation.x = -10.0f;
    mainCamera->rotation.y = 0.0f;

    mainCamera->zoomDistance = 800.0f;
    return UpdateResultsProcess;
}

void UpdateResultsProcess(OMOBJ *object)
{
    M443DllWork *work = object->data;
    s32 resultSequenceReady = 1;

    UpdateMainObject(object);
    if (activePlayerMask != 0) {
        resultSequenceReady = 0;
        if (scoreSequenceId < 0) {
            scoreSequenceId = MGSeqCreate(0xE, recordTimeOverlay.farPlane);
            recordTimeOverlay.sortPriority = 1;
        }
        else if (MGSeqStatGet(scoreSequenceId) == 0) {
            resultSequenceReady = 1;
            activePlayerMask = 0;
        }
    }
    if ((WipeStatGet() != 0) || (resultSequenceReady == 0)) {
        work->rewardSequenceFrameCount = 0;
        return;
    }
    work->rewardSequenceState = 1;
    if (resultsObjectId < 0) {
        if (work->scoreSubmissionState == 1) {
            resultsObjectId = HuAudSStreamPlay(1);
        }
        else {
            resultsObjectId = HuAudSStreamPlay(4);
        }
    }
    if (rewardSequenceId < 0) {
        if (work->scoreSubmissionState == 1) {
            rewardSequenceId = MGSeqCreate(5, 3, GWPlayerCfg[work->winningPlayerIndex].character, -1, -1, -1);
            GWPlayerCoinWinAdd(work->winningPlayerIndex, 10);
        }
        else {
            rewardSequenceId = MGSeqCreate(5, 2);
        }
    }
    else if ((MGSeqStatGet(rewardSequenceId) == 0) && (work->rewardSequenceFrameCount >= REFRESH_RATE_F * 3.5f)) {
        work->rewardSequenceState = 2;
        AdvanceGameState(object);
    }
}

M443DllSpriteConfig recordTimeSpriteConfig[0x10] = {
    { 0x4A001F, 0x10E },
    { 0x610037, 0xFA },
    { 0x610030, 0xFA },
    { 0x610030, 0xFA },
    { 0x610030, 0xFA },
    { 0x610030, 0xFA },
    { 0x610030, 0xFA },
    { 0x610030, 0xFA },
    { 0x610030, 0xFA },
    { 0x610031, 0xFA },
    { 0x610031, 0xFA },
    { 0x610031, 0xFA },
    { 0x610031, 0xFA },
    { 0x610031, 0xFA },
    { 0x610031, 0xFA },
    { 0x610031, 0xFA },
};

void InitializeScoreOverlay(M443DllOverlayWork *overlay)
{
    s32 spriteIndex;

    memset(overlay, 0, sizeof(M443DllOverlayWork));
    overlay->displayedRecordTime = GWMGRecordGet(5);
    if (overlay->displayedRecordTime == 0) {
        overlay->displayedRecordTime = REFRESH_RATE * 60;
    }
    overlay->positionX = 320.0f;
    overlay->positionY = 64.0f;
    for (spriteIndex = 0; spriteIndex < 0x10; spriteIndex++) {
        overlay->spriteIds[spriteIndex] = -1;
        if (recordTimeSpriteConfig[spriteIndex].spriteResourceId != 0) {
            overlay->spriteIds[spriteIndex] = espEntry(recordTimeSpriteConfig[spriteIndex].spriteResourceId, recordTimeSpriteConfig[spriteIndex].spriteFrameCount, 0);
            espAttrSet(overlay->spriteIds[spriteIndex], HUSPR_ATTR_LINEAR);
            espDispOff(overlay->spriteIds[spriteIndex]);
        }
    }
    espColorSet(overlay->spriteIds[0], 0, 0, 0);
    espTPLvlSet(overlay->spriteIds[0], 0.699999988079071);
    espBankSet(overlay->spriteIds[7], 0xB);
    espBankSet(overlay->spriteIds[8], 0xA);
    espBankSet(overlay->spriteIds[14], 0xB);
    espBankSet(overlay->spriteIds[15], 0xA);
}

Vec2f recordTimeDigitOffsets[0x10] = {
    { 0.0f, 0.0f },
    { -52.0f, -9.0f },
    { 53.0f, -8.0f },
    { 35.0f, -8.0f },
    { 9.0f, -8.0f },
    { -9.0f, -8.0f },
    { -31.0f, -8.0f },
    { 23.0f, -8.0f },
    { -17.0f, -8.0f },
    { 53.0f, 8.0f },
    { 35.0f, 8.0f },
    { 9.0f, 8.0f },
    { -9.0f, 8.0f },
    { -31.0f, 8.0f },
    { 23.0f, 8.0f },
    { -17.0f, 8.0f },
};

void UpdateScoreOverlay(M443DllOverlayWork *overlay)
{
    u32 recordTimeDigits[3];
    float pulseScale;

    s32 spriteIndex;
    u32 recordTime;
    s32 digitIndex;
    s32 nextSpriteIndex;

    for (spriteIndex = 0; spriteIndex < 0x10; spriteIndex++) {
        if (overlay->spriteIds[spriteIndex] >= 0) {
            if (overlay->isVisible == 0) {
                espDispOff(overlay->spriteIds[spriteIndex]);
            }
            else {
                espDispOn(overlay->spriteIds[spriteIndex]);
                espPosSet(overlay->spriteIds[spriteIndex], 0.9f * (overlay->positionX + recordTimeDigitOffsets[spriteIndex].x), overlay->positionY + recordTimeDigitOffsets[spriteIndex].y);
            }
        }
    }
    overlay->animationFrame++;
    if (overlay->isScoreAnimating != 0) {
        recordTime = overlay->targetRecordTime;
        overlay->displayedRecordTime = recordTime;
        pulseScale = 1.100000023841858 + (0.10000000149011612 * sind((22.5f * (overlay->animationFrame & 0xF))));
        espScaleSet(overlay->spriteIds[1], pulseScale, pulseScale);
        for (spriteIndex = 2; spriteIndex <= 6; spriteIndex++) {
            espScaleSet(overlay->spriteIds[spriteIndex], pulseScale, pulseScale);
        }
        for (spriteIndex = 7; spriteIndex <= 8; spriteIndex++) {
            espScaleSet(overlay->spriteIds[spriteIndex], pulseScale, pulseScale);
        }
    }
    for (spriteIndex = 0; spriteIndex < 2; spriteIndex++) {
        if (spriteIndex != 0) {
            recordTime = overlay->targetRecordTime;
            nextSpriteIndex = 9;
        }
        else {
            recordTime = overlay->displayedRecordTime;
            nextSpriteIndex = 2;
        }
        recordTimeDigits[0] = recordTime % REFRESH_RATE;
        recordTime = (recordTime - recordTimeDigits[0]) / REFRESH_RATE;
#if VERSION_PAL
        recordTimeDigits[0] = (100.0f * (recordTimeDigits[0] / REFRESH_RATE_F));
#else
        recordTimeDigits[0] = 100.0f * (REFRESH_FREQ * recordTimeDigits[0]);
#endif
        recordTimeDigits[1] = recordTime % 60;
        recordTime = (recordTime - recordTimeDigits[1]) / 60;
        recordTimeDigits[2] = recordTime;
        for (digitIndex = 0; digitIndex < 3; digitIndex++) {
            espBankSet(overlay->spriteIds[nextSpriteIndex++], recordTimeDigits[digitIndex] % 10);
            if (digitIndex < 2) {
                espBankSet(overlay->spriteIds[nextSpriteIndex++], recordTimeDigits[digitIndex] / 10);
            }
        }
    }
}

void InitializePlayerObjects(HUPROCESS *process)
{
    Hu3DCameraCreate(0x1F);
    cameraUpdateObject = omAddObjEx(process, 0x7FDA, 0, 0, -1, omOutViewMulti);
    cameraUpdateObject->work[0] = 5;
    cameraObject = omAddObjEx(process, 0xC, 1, 0, -1, UpdatePlayerObjects);
    cameraObject->work[0] = 0;
}

void UpdatePlayerObjects(OMOBJ *object)
{
    M443DllCameraState *camera;
    s32 cameraIndex;
    M443DllCameraState *cameraList;

    float cameraViewportSettings[5][4] = {
        { 320.0f, 240.0f, 640.0f, 416.0f },
        { 160.0f, 136.0f, 320.0f, 208.0f },
        { 480.0f, 136.0f, 320.0f, 208.0f },
        { 160.0f, 344.0f, 320.0f, 208.0f },
        { 480.0f, 344.0f, 320.0f, 208.0f },
    };
    s32 cameraMasks[5] = { 1, 2, 4, 8, 16 };

    object->data = HuMemDirectMallocNum(HEAP_HEAP, 10 * sizeof(M443DllCameraState), HU_MEMNUM_OVL);
    cameraList = object->data;
    memset(cameraList, 0, 10 * sizeof(M443DllCameraState));
    object->mdlId[0] = Hu3DHookFuncCreate(DrawShadowPlane);
    Hu3DModelLayerSet(*object->mdlId, 0);
    Hu3DModelCameraSet(*object->mdlId, 0x1E);
    for (camera = varR28, cameraIndex = 0; cameraIndex < 5; cameraIndex++, camera++) {
        camera->isEnabled = 1;
        camera->sortPriority = cameraIndex * 0x64;
        camera->nearPlane = 41.5f;
        camera->farPlane = 100.0f;
        camera->fieldOfView = 60000.0f;
        camera->targetPosition.x = 0.0f;
        camera->targetPosition.y = 0.0f;
        camera->targetPosition.z = 0.0f;
        camera->rotation.x = -15.0f;
        camera->rotation.y = 0.0f;
        camera->rotation.z = 0.0f;
        camera->zoomDistance = 1000.0f;
        camera->viewportScale = 1.0f;
        camera->viewportCenterX = cameraViewportSettings[cameraIndex][0];
        camera->viewportCenterY = cameraViewportSettings[cameraIndex][1];
        camera->viewportWidth = cameraViewportSettings[cameraIndex][2];
        camera->viewportHeight = cameraViewportSettings[cameraIndex][3];
        camera->cameraId = camera->cameraMask = cameraMasks[cameraIndex];
        camera->cameraIndex = cameraIndex;
        camera->cameraSlot = cameraIndex;
    }
    object->work[1] = 0;
    for (cameraIndex = 0; cameraIndex < 2; cameraIndex++) {
        cameraViewportSprites[cameraIndex] = espEntry(0x4A001E, 0x12C, 0);
        espPosSet(cameraViewportSprites[cameraIndex], 288.0, 240.0);
        if (cameraIndex != 0) {
            espScaleSet(cameraViewportSprites[cameraIndex], 0.22499999403953552, 60.0);
        }
        else {
            espScaleSet(cameraViewportSprites[cameraIndex], 72.0, 0.25);
        }
        espColorSet(cameraViewportSprites[cameraIndex], 0, 0, 0);
        espTPLvlSet(cameraViewportSprites[cameraIndex], 1.0);
        espDispOff(cameraViewportSprites[cameraIndex]);
    }
    object->work[0]++;
    UpdatePlayerObjectAnimations(object);
    object->objFunc = DestroyPlayerObjects;
}

void DestroyPlayerObjects(OMOBJ *object)
{
    M443DllWork *unusedGameWorkPointer = object->data;
    s32 cameraMaskLookup[4] = { 0x100, 0x200, 0x400, 0x800 };
    UpdatePlayerObjectAnimations(object);
}

void UpdatePlayerObjectAnimations(OMOBJ *object)
{
    float cameraAspectRatio;

    M443DllCameraState *camera;
    s32 cameraIndex;
    s32 comparisonIndex;
    M443DllCameraState *cameraList;
    s32 selectedCameraMask;
    u32 sortSwapTemp;
    s32 showPrimaryModel;
    s32 unusedCameraOrder;
    u32 unusedCameraWorkValue;
    u32 unusedCameraStateValue;

    cameraList = object->data;
    {
        s32 cameraOrderPriorities[4] = { 2, 4, 8, 16 };
        u32 cameraSortPriorities[4];
        s32 cameraSortIndices[4];
        float viewportSize[2];
        float scissorOrigin[2];
        float scissorSize[2];

        showPrimaryModel = 0;
        selectedCameraMask = 0;
        memcpy(&cameraList[5], cameraList, 5 * sizeof(M443DllCameraState));
        switch (object->work[1]) {
            case 0:
                break;
            case 1:
                selectedCameraMask |= 1;
                break;
            case 2:
                selectedCameraMask |= 1;
            case 3:
                showPrimaryModel = 1;
            case 4:
                selectedCameraMask |= 0x1E;
                break;
        }
        if (showPrimaryModel != 0) {
            Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        }
        for (cameraIndex = 0; cameraIndex < 4; cameraIndex++) {
            cameraSortPriorities[cameraIndex] = cameraList[cameraIndex + 6].sortPriority;
            cameraSortIndices[cameraIndex] = cameraIndex;
        }
        for (cameraIndex = 0; cameraIndex < 3; cameraIndex++) {
            for (comparisonIndex = cameraIndex + 1; comparisonIndex < 4; comparisonIndex++) {
                if ((cameraIndex != comparisonIndex) && (cameraSortPriorities[cameraIndex] > cameraSortPriorities[comparisonIndex])) {
                    sortSwapTemp = cameraSortPriorities[cameraIndex];
                    cameraSortPriorities[cameraIndex] = cameraSortPriorities[comparisonIndex];
                    cameraSortPriorities[comparisonIndex] = sortSwapTemp;
                    sortSwapTemp = cameraSortIndices[cameraIndex];
                    cameraSortIndices[cameraIndex] = cameraSortIndices[comparisonIndex];
                    cameraSortIndices[comparisonIndex] = sortSwapTemp;
                }
            }
        }
        for (cameraIndex = 0; cameraIndex < 4; cameraIndex++) {
            cameraList[cameraSortIndices[cameraIndex] + 6].cameraId = cameraOrderPriorities[cameraIndex];
            cameraList[cameraSortIndices[cameraIndex] + 6].cameraSlot = cameraIndex + 1;
        }
        Hu3DCamera[5].fov = -1.0f;
        for (camera = &cameraList[5], cameraIndex = 0; cameraIndex < 5; cameraIndex++, camera++) {
            if ((camera->isEnabled == 0) || ((selectedCameraMask & camera->cameraMask) == 0)) {
                Hu3DCamera[camera->cameraSlot].fov = -1.0f;
            }
            else {
                cameraAspectRatio = 1.2f * ((camera->viewportWidth / 640.0f) / (camera->viewportHeight / 480.0f));
                viewportSize[0] = camera->viewportWidth * camera->viewportScale;
                viewportSize[1] = camera->viewportHeight * camera->viewportScale;
                Hu3DCameraPerspectiveSet(camera->cameraId, camera->nearPlane, camera->farPlane, camera->fieldOfView, cameraAspectRatio);
                Hu3DCameraViewportSet(
                    camera->cameraId, camera->viewportCenterX - (0.5f * viewportSize[0]), camera->viewportCenterY - (0.5f * viewportSize[1]), viewportSize[0], viewportSize[1], 0.0f, 1.0f);
                scissorOrigin[0] = camera->viewportCenterX - (0.5f * viewportSize[0]);
                scissorOrigin[1] = camera->viewportCenterY - (0.5f * viewportSize[1]);
                scissorSize[0] = camera->viewportCenterX + (0.5f * viewportSize[0]);
                scissorSize[1] = camera->viewportCenterY + (0.5f * viewportSize[1]);
                if (scissorOrigin[0] < 0.0f) {
                    scissorOrigin[0] = 0.0f;
                }
                if (scissorOrigin[1] < 0.0f) {
                    scissorOrigin[1] = 0.0f;
                }
                if (scissorSize[0] > 640.0f) {
                    scissorSize[0] = 640.0f;
                }
                if (scissorSize[1] > 480.0f) {
                    scissorSize[1] = 480.0f;
                }
                scissorSize[0] -= scissorOrigin[0];
                scissorSize[1] -= scissorOrigin[1];
                Hu3DCameraScissorSet(camera->cameraId, scissorOrigin[0], scissorOrigin[1], scissorSize[0], scissorSize[1]);
                comparisonIndex = camera->cameraSlot;
                CenterM[comparisonIndex].x = camera->targetPosition.x;
                CenterM[comparisonIndex].y = camera->targetPosition.y;
                CenterM[comparisonIndex].z = camera->targetPosition.z;
                CRotM[comparisonIndex].x = camera->rotation.x;
                CRotM[comparisonIndex].y = camera->rotation.y;
                CRotM[comparisonIndex].z = camera->rotation.z;
                CZoomM[comparisonIndex] = camera->zoomDistance;
            }
        }
    }
    if (object->work[1] == 4) {
        espDispOn(cameraViewportSprites[0]);
        espDispOn(cameraViewportSprites[1]);
        return;
    }
    espDispOff(cameraViewportSprites[0]);
    espDispOff(cameraViewportSprites[1]);
}

GXColor transparentShadowColor = { 0x00, 0x00, 0x00, 0x00 };

void DrawShadowPlane(HU3DMODEL *model, Mtx matrix)
{
    Mtx identityMatrix;

    MTXIdentity(identityMatrix);
    GXLoadPosMtxImm(identityMatrix, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevColor(GX_TEVREG0, transparentShadowColor);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);
    GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-45000.0f, 30000.0f, -59900.0f);
    GXPosition3f32(45000.0f, 30000.0f, -59900.0f);
    GXPosition3f32(45000.0f, -30000.0f, -59900.0f);
    GXPosition3f32(-45000.0f, -30000.0f, -59900.0f);
}

void SetCameraTransitionState(u32 transitionState)
{
    cameraObject->work[1] = transitionState;
}

u32 GetCameraTransitionState(void)
{
    return cameraObject->work[1];
}

M443DllCameraState *FindMainCameraByMask(s32 cameraMask)
{
    M443DllCameraState *camera;
    s32 cameraIndex;
    M443DllCameraState *cameraList;

    cameraList = cameraObject->data;
    for (camera = cameraList, cameraIndex = 0; cameraIndex < 5; cameraIndex++, camera++) {
        if (camera->cameraMask & cameraMask) {
            break;
        }
    }
    if (cameraIndex >= 5) {
        OSReport("M443:Camera Flag Error!!\n");
        return NULL;
    }
    return camera;
}

M443DllCameraState *FindResultCameraByMask(s32 cameraMask)
{
    M443DllCameraState *camera;
    s32 cameraIndex;
    M443DllCameraState *cameraList;

    cameraList = cameraObject->data;
    for (camera = &cameraList[5], cameraIndex = 0; cameraIndex < 5; cameraIndex++, camera++) {
        if (camera->cameraMask & cameraMask) {
            break;
        }
    }
    if (cameraIndex >= 5) {
        OSReport("M443:Camera Flag Error!!\n");
        return NULL;
    }
    return camera;
}

M443DllCameraState *GetMainCamera(u32 cameraIndex)
{
    M443DllCameraState *cameraList = cameraObject->data;
    if (cameraIndex >= 5) {
        OSReport("M443:Camera No. Error!!\n");
        return NULL;
    }
    return &cameraList[cameraIndex];
}

M443DllCameraState *GetResultCamera(u32 cameraIndex)
{
    M443DllCameraState *cameraList = cameraObject->data;
    if (cameraIndex >= 5) {
        OSReport("M443:Camera No. Error!!\n");
        return NULL;
    }
    return &cameraList[cameraIndex + 5];
}

void SetModelCameraMask(s32 modelId, s32 cameraMask)
{
    M443DllCameraState *camera;
    s32 cameraIndex;
    M443DllCameraState *cameraList;
    M443DllCameraState *cameraArray;

    s32 combinedCameraMask = 0;
    if (modelId >= 0) {
        cameraArray = cameraObject->data;
        cameraList = &cameraArray[5];
        for (camera = cameraList, cameraIndex = 0; cameraIndex < 5; cameraIndex++, camera++) {
            if (camera->cameraMask & cameraMask) {
                combinedCameraMask |= camera->cameraId;
            }
        }
        Hu3DModelCameraSet(modelId, combinedCameraMask);
    }
}

s32 GetGameState(void)
{
    return ((M443DllWork *)mainObject->data)->gameState;
}

u32 GetCameraTransitionStage(void)
{
    return ((M443DllWork *)mainObject->data)->cameraTransitionState;
}

u32 GetResultStage(void)
{
    return ((M443DllWork *)mainObject->data)->resultStage;
}

u32 GetRewardSequenceState(void)
{
    return ((M443DllWork *)mainObject->data)->rewardSequenceState;
}

void SetPlayerScore(s32 playerIndex, float score)
{
    M443DllWork *work = mainObject->data;

    if (GetResultStage() == 2) {
        work->playerScores[playerIndex] = score;
    }
}

s32 GetPlayerRank(s32 playerIndex)
{
    M443DllWork *work = mainObject->data;

    if (work->scoreSubmissionState == 1) {
        return work->playerRankByIndex[playerIndex];
    }
    return -1;
}

void SetPlayerFinishEvent(s32 playerIndex)
{
    ((M443DllWork *)mainObject->data)->finishEventFlags |= (1 << ((playerIndex & 7) * 4));
}

void SetPlayerEvent(s32 playerIndex)
{
    ((M443DllWork *)mainObject->data)->playerEventFlags |= (1 << ((playerIndex & 3) * 4));
}

s32 IsMinigameSequenceDone(void)
{
    return MGSeqStatGet(minigameSequenceId) == 0;
}

void SetCameraFromPositionTarget(Vec *cameraPosition, Vec *cameraTarget)
{
    Vec cameraDirection;
    float horizontalDistanceSquared;

    VECSubtract(cameraTarget, cameraPosition, &cameraDirection);
    Center = *cameraTarget;
    horizontalDistanceSquared = (cameraDirection.x * cameraDirection.x) + (cameraDirection.z * cameraDirection.z);
    CRot.x = atan2d(cameraDirection.y, sqrtf(horizontalDistanceSquared));
    CRot.y = atan2d(-cameraDirection.x, -cameraDirection.z);
    CRot.z = 0.0f;
    CZoom = VECMag(&cameraDirection);
}

void SetCameraFromCoordinates(float cameraX, float cameraY, float cameraZ, float targetX, float targetY, float targetZ)
{
    Vec cameraPosition;
    Vec cameraTarget;
    Vec cameraDirection;
    float horizontalDistanceSquared;

    cameraPosition.x = cameraX;
    cameraPosition.y = cameraY;
    cameraPosition.z = cameraZ;
    cameraTarget.x = targetX;
    cameraTarget.y = targetY;
    cameraTarget.z = targetZ;
    VECSubtract(&cameraTarget, &cameraPosition, &cameraDirection);
    Center = cameraTarget;
    horizontalDistanceSquared = (cameraDirection.x * cameraDirection.x) + (cameraDirection.z * cameraDirection.z);
    CRot.x = atan2d(cameraDirection.y, sqrtf(horizontalDistanceSquared));
    CRot.y = atan2d(-cameraDirection.x, -cameraDirection.z);
    CRot.z = 0.0f;
    CZoom = VECMag(&cameraDirection);
}

float InterpolateAngle(float currentAngle, float targetAngle, float interpolationFactor)
{
    float shortestAngleDelta;
    float normalizedAngleDelta;
    float interpolatedAngle;

    normalizedAngleDelta = fmod(targetAngle - currentAngle, 360.0);
    shortestAngleDelta = normalizedAngleDelta;
    if (shortestAngleDelta < 0.0f) {
        shortestAngleDelta += 360.0f;
    }
    if (shortestAngleDelta > 180.0f) {
        shortestAngleDelta -= 360.0f;
    }
    interpolatedAngle = fmod(currentAngle + (interpolationFactor * shortestAngleDelta), 360.0);
    shortestAngleDelta = interpolatedAngle;
    if (shortestAngleDelta < 0.0f) {
        shortestAngleDelta += 360.0f;
    }
    return shortestAngleDelta;
}

float GetShortestAngleDelta(float currentAngle, float targetAngle)
{
    float shortestAngleDelta;
    float normalizedAngleDelta;

    normalizedAngleDelta = fmod(targetAngle - currentAngle, 360.0);
    shortestAngleDelta = normalizedAngleDelta;
    if (shortestAngleDelta < 0.0f) {
        shortestAngleDelta += 360.0f;
    }
    if (shortestAngleDelta > 180.0f) {
        shortestAngleDelta -= 360.0f;
    }
    return shortestAngleDelta;
}
