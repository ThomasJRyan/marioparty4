#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXVert.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/gamework.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/wipe.h"
#include "string.h"

#include "REL/m460Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

typedef struct M460DllWork {
    u32 gameState;
    s32 gameStateTimer;
    s32 isPaused;
    u32 resultState;
    s32 introTimer;
    u32 roundState;
    u32 roundTimer;
    u32 countdownTimer;
    u32 countdownFrameTimer;
    u32 elapsedFrames;
    s32 previousBestFrames;
    u32 currentFrames;
    s32 recordPacked;
    char unreferencedWorkData34[0x4];
    u32 selectionState;
    s32 isSelectionConfirmed;
    s32 isSelectionReady;
    s32 isRoundComplete;
    s32 newRecord;
    s32 resultSequence;
    u32 resultTimer;
    s32 playerResults[4];
    s32 playerRanks[4];
    s32 cameraState;
    s32 cameraMode;
    s32 cameraTimer;
    s32 cameraFlags;
    char unreferencedWorkData84[4];
    s32 gameComplete;
    u32 playerStateFlags;
    u32 cameraStateFlags;
    s32 resultFlags;
} M460DllWork; /* size = 0x98 */

void M460ReturnToMenu(OMOBJ *object);
void M460ShowRoundCountdown(OMOBJ *object);
void M460UpdateRound(OMOBJ *object);
void M460ShowRoundResult(OMOBJ *object);
OMOBJFUNC M460StartIntroCamera(HUPROCESS *process, OMOBJ *object);
void M460UpdateIntroCamera(OMOBJ *object);
OMOBJFUNC M460StartResultsCamera(HUPROCESS *process, OMOBJ *object);
void M460UpdateResultsCamera(OMOBJ *object);
void M460InitializeCameras(HUPROCESS *process);
void M460CreateSplitScreenCameras(OMOBJ *object);
void M460UpdateSplitScreenCameras(OMOBJ *object);
void M460ConfigureSplitScreenCameras(OMOBJ *object);
void M460DrawCameraBackground(HU3DMODEL *model, Mtx mtx);
M460CameraWork *M460FindPrimaryCamera(s32 cameraFlags);
M460CameraWork *M460FindSecondaryCamera(s32 cameraFlags);
void M460SetSelectionState(u32 selectionState);
s32 M460IsSelectionReady(void);
void M460InitializeGameObject(OMOBJ *object);
void M460AdvanceGameState(OMOBJ *object);
void M460GameObjectUpdate(OMOBJ *object);
void M460ResumeAfterWipe(OMOBJ *object);

Vec globalLightPosition = { 100.0f, 800.0f, -100.0f };
Vec globalLightDirection = { 0.3f, -0.8f, 0.3f };
Vec lightRotationAngles = { 10.0f, 45.0f, 3500.0f };
GXColor globalLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };
Vec shadowPosition = { 50.0f, 150000.0f, 50.0f };
Vec shadowUpVector = { 0.0f, 1.0f, 0.0f };
Vec shadowTarget = { 0.0f, 0.0f, 0.0f };
Vec introCameraCenters[3] = {
    { 0.0f, 442.0f, -927.00006f },
    { 0.0f, 442.0f, -927.00006f },
    { 0.0f, 325.0f, -600.0f },
};
Vec introCameraRotations[3] = {
    { 4.5f, 0.0f, 450.0f },
    { 4.5f, 0.0f, 450.0f },
    { 9.75f, 0.0f, 1275.0f },
};
float introCameraZoomLevels[3] = { 70.0f, 70.0f, 41.5f };
float introCameraSoundVolumes[3] = { 0.0f, REFRESH_FREQ, 0.0f };
s32 introCameraTransitionDurations[3] = { -REFRESH_RATE, REFRESH_RATE, 0 };
GXColor cameraBackgroundColor = { 0, 0, 0, 0 };

HUPROCESS *gameProcess;
// M460DllWork2
OMOBJ *gameObject;
OMOBJ *screenViewportObject;
// M460DllWork2
OMOBJ *cameraManagerObject;
s16 unusedSequenceId20;
s16 globalLightIds[3];
s16 roundCountdownSequence;
s16 roundCountdownDigitSequence;
s16 roundResultSequence;
s16 recordSequence;
s16 unusedSequenceId10;
s32 reservedGameState;
s32 roundMusicSequence;
s32 resultsMusicStream;
s32 gameActive;

void ObjectSetup(void)
{
    Mtx lightRotationMatrix;
    HUPROCESS *process;
    HU3DLIGHT *globalLight;

    Vec listenerPosition = { 0.0f, 0.0f, 0.0f };
    Vec listenerDirection = { 0.0f, 0.0f, -1.0f };
    Vec lightOffset;

    reservedGameState = 0;
    HuAudSndGrpSet(0x53);
    HuAudFXListnerSetEX(&listenerPosition, &listenerDirection, 10000.0f, 34000.0f * REFRESH_FREQ, 0.0f, 300.0f, 300.0f);
    roundMusicSequence = resultsMusicStream = -1;
    unusedSequenceId20 = -1;
    gameActive = 1;
    shadowPosition.x = 200.0f;
    shadowPosition.y = 1200.0f;
    shadowPosition.z = 160.0f;
    shadowUpVector.x = 0.0f;
    shadowUpVector.y = 1.0f;
    shadowUpVector.z = 0.0f;
    shadowTarget.x = 0.0f;
    shadowTarget.y = 0.0f;
    shadowTarget.z = 0.0f;
    Hu3DShadowCreate(30.0f, 20.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.625f);
    Hu3DShadowPosSet(&shadowPosition, &shadowUpVector, &shadowTarget);
    globalLightIds[0] = Hu3DGLightCreateV(&globalLightPosition, &globalLightDirection, &globalLightColor);
    Hu3DGLightInfinitytSet(globalLightIds[0]);
    globalLight = &Hu3DGlobalLight[globalLightIds[0]];
    globalLight->type |= 0x8000;
    lightRotationAngles.x = 160.0f;
    lightRotationAngles.y = 0.0f;
    lightRotationAngles.z = 4200.0f;
    lightOffset.x = lightOffset.z = 0.0f;
    lightOffset.y = lightRotationAngles.z;
    mtxRot(lightRotationMatrix, lightRotationAngles.x, lightRotationAngles.y, 0.0f);
    MTXMultVec(lightRotationMatrix, &lightOffset, &shadowPosition);
    Hu3DGLightPosAimSetV(globalLightIds[0], &shadowPosition, &shadowTarget);
    Hu3DShadowPosSet(&shadowPosition, &shadowUpVector, &shadowTarget);
    roundCountdownSequence = roundCountdownDigitSequence = roundResultSequence = recordSequence = unusedSequenceId10 = -1;
    process = omInitObjMan(0x32, 0x2000);
    gameProcess = process;
    omGameSysInit(process);
    M460InitializeCameras(process);
    gameObject = omAddObjEx(process, 0xA, 0, 0, -1, M460InitializeGameObject);
    M460CreateMapObject(process);
    M460CreatePlayerObjects(process);
}

void M460InitializeGameObject(OMOBJ *object)
{
    M460DllWork *work;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M460DllWork), HU_MEMNUM_OVL);
    work = object->data;
    memset(work, 0, sizeof(M460DllWork));
    work->gameState = 0;
    work->gameStateTimer = 0;
    work->resultState = 0;
    work->introTimer = 0;
    work->roundState = 0;
    work->roundTimer = 0;
    work->resultSequence = 0;
    work->resultTimer = 0;
    work->isPaused = 0;
    work->gameComplete = 0;
    work->cameraState = work->cameraMode = work->cameraTimer = work->cameraFlags = -1;
    work->playerStateFlags = 0;
    work->resultFlags = 0;
    work->recordPacked = 0;
    work->recordPacked = GWMGRecordGet(0xC);
    work->previousBestFrames = (work->recordPacked >> 0x10) & 0xFF;
    work->elapsedFrames = (work->recordPacked >> 0x18) & 0xFF;
    work->recordPacked = (u16)work->recordPacked;
    M460CreateScoreDisplay(gameProcess, work->recordPacked);
    object->objFunc = M460GameObjectUpdate;
}

void M460UpdateListenerPosition(OMOBJ *object)
{
    Vec listenerPosition;
    Vec listenerDirection;
    M460CameraWork *cameraWork;
    M460DllWork *work;

    work = object->data;
    work->gameStateTimer++;
    work->introTimer++;
    work->roundTimer++;
    work->resultTimer++;
    if ((omSysExitReq != 0) && (work->gameState != 5)) {
        work->gameState = 4;
        M460AdvanceGameState(object);
    }
    cameraWork = M460FindSecondaryCamera(1);
    listenerDirection.x = -(sind(cameraWork->cameraRotation.y) * cosd(cameraWork->cameraRotation.x));
    listenerDirection.y = -(-sind(cameraWork->cameraRotation.x));
    listenerDirection.z = -(cosd(cameraWork->cameraRotation.y) * cosd(cameraWork->cameraRotation.x));
    listenerPosition.x = cameraWork->cameraCenter.x - (listenerDirection.x * cameraWork->cameraZoom);
    listenerPosition.y = cameraWork->cameraCenter.y - (listenerDirection.y * cameraWork->cameraZoom);
    listenerPosition.z = cameraWork->cameraCenter.z - (listenerDirection.z * cameraWork->cameraZoom);
    HuAudFXListnerUpdate(&listenerPosition, &listenerDirection);
}

void M460AdvanceGameState(OMOBJ *object)
{
    M460DllWork *work = object->data;

    if (work->isPaused == 0) {
        switch (work->gameState) {
            case 0:
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 0x3C);
                gameActive = 1;
                M460SetScoreDisplayState(0);
                work->gameState = 1;
                work->introTimer = 0;
                object->objFunc = M460StartIntroCamera(gameProcess, object);
                break;
            case 1:
                work->gameState = 2;
                work->roundTimer = 0;
                object->objFunc = M460ShowRoundCountdown;
                break;
            case 2:
                work->gameState = 3;
                work->resultTimer = 0;
                object->objFunc = M460StartResultsCamera(gameProcess, object);
                break;
            case 3:
            case 4:
                work->isPaused = 1;
                work->gameState = 5;
                object->objFunc = M460ResumeAfterWipe;
                break;
            case 5:
            default:
                work->gameState = 5;
                object->objFunc = M460ReturnToMenu;
        }
        work->gameStateTimer = 0;
    }
}

void M460GameObjectUpdate(OMOBJ *object)
{
    M460UpdateListenerPosition(object);
    M460AdvanceGameState(object);
}

void M460ResumeAfterWipe(OMOBJ *object)
{
    M460DllWork *work = object->data;
    M460UpdateListenerPosition(object);
    if (WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 0x3C);
        WipeColorSet(0xFF, 0xFF, 0xFF);
        work->isPaused = 0;
        M460AdvanceGameState(object);
    }
}

void M460ReturnToMenu(OMOBJ *object)
{
    M460UpdateListenerPosition(object);
    if (WipeStatGet() == 0) {
        M460DestroyPlayerObjects();
        M460DestroyMapObject();
        M460DestroyScoreDisplay();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void M460ShowRoundCountdown(OMOBJ *object)
{
    M460DllWork *work = object->data;
    M460UpdateListenerPosition(object);
    switch (work->roundState) {
        case 0:
            work->countdownTimer = 0x3C;
            work->countdownFrameTimer = REFRESH_RATE;
            roundCountdownSequence = MGSeqCreate(3, 0);
            MGSeqPosSet(roundCountdownSequence, 320.0f, 240.0f);
            work->roundState = 1;
            work->roundTimer = 0;
            break;
        case 1:
            if ((roundMusicSequence < 0) && ((MGSeqStatGet(roundCountdownSequence) & 0x10) != 0)) {
                roundMusicSequence = HuAudSeqPlay(0x49);
            }
            if (MGSeqStatGet(roundCountdownSequence) == 0) {
                work->currentFrames = 0;
                work->isRoundComplete = 0;
                if (work->isPaused == 0) {
                    work->roundTimer = 0;
                    object->objFunc = M460UpdateRound;
                }
            }
            break;
    }
}

void M460UpdateRound(OMOBJ *object)
{
    M460DllWork *work = object->data;
    s32 roundIsComplete = 0;
    s32 shouldShowResults = 0;
    M460UpdateListenerPosition(object);
    switch (work->roundState) {
        case 1:
            if (WipeStatGet() == 0) {
                work->selectionState = 3;
                work->isSelectionConfirmed = 0;
                work->isSelectionReady = 0;
                work->countdownTimer = 0x3C;
                work->countdownFrameTimer = REFRESH_RATE;
                work->roundState = 2;
                work->roundTimer = 0;
            }
            break;
        case 2:
            if (work->selectionState == 3) {
                if ((--work->countdownFrameTimer) == 0) {
                    work->countdownFrameTimer = REFRESH_RATE;
                    work->countdownTimer--;
                    if (work->countdownTimer == 0) {
                        work->isRoundComplete = 1;
                        M460SetSelectionState(frandmod(2));
                    }
                }
                if ((roundCountdownDigitSequence < 0) && (work->countdownTimer < 0xA)) {
                    roundCountdownDigitSequence = MGSeqCreate(1, work->countdownTimer, -1, -1);
                }
                MGSeqParamSet(roundCountdownDigitSequence, 1, work->countdownTimer);
            }
            else {
                if (roundCountdownDigitSequence >= 0) {
                    MGSeqParamSet(roundCountdownDigitSequence, 2, -1);
                    roundCountdownDigitSequence = -1;
                }
                if (frandmod(0x3E8) < 0x1F4) {
                    work->isRoundComplete = 1;
                }
                if (work->currentFrames >= 0x1E) {
                    work->isRoundComplete = 1;
                }
                if (work->isRoundComplete != 0) {
                    work->isRoundComplete = 1;
                    work->gameComplete = 1;
                }
                work->roundState = 3;
                work->roundTimer = 0;
            }
            break;
        case 3:
            if (M460IsSelectionReady() != 0) {
                if (work->isRoundComplete != 0) {
                    roundIsComplete = 1;
                }
                else if (work->roundTimer > REFRESH_RATE_F) {
                    if (work->isPaused == 0) {
                        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, REFRESH_RATE / 2);
                        WipeColorSet(0, 0, 0);
                    }
                    work->roundState = 4;
                    work->roundTimer = 0;
                }
            }
            else {
                work->roundTimer = 0;
            }
            break;
        case 4:
            if (WipeStatGet() == 0) {
                work->currentFrames++;
                if (work->isPaused == 0) {
                    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, REFRESH_RATE / 2);
                }
                work->roundState = 1;
                work->roundTimer = 0;
            }
            break;
    }
    if (roundIsComplete != 0) {
        roundResultSequence = MGSeqCreate(3, 1);
        HuAudSeqFadeOut(roundMusicSequence, 0x64);
        work->elapsedFrames++;
        if (work->elapsedFrames > 0xFF) {
            work->elapsedFrames = 0xFF;
        }
        if (work->currentFrames > work->recordPacked) {
            work->previousBestFrames = work->elapsedFrames;
            work->recordPacked = work->currentFrames;
            work->newRecord = 1;
            GWMGRecordSet(0xC, work->recordPacked);
        }
        mgRecordExtra = work->currentFrames;
        work->roundState = 5;
        work->roundTimer = 0;
        if (work->isPaused == 0) {
            object->objFunc = M460ShowRoundResult;
        }
    }
}

void M460ShowRoundResult(OMOBJ *object)
{
    M460DllWork *work = object->data;

    if (roundCountdownDigitSequence >= 0) {
        MGSeqParamSet(roundCountdownDigitSequence, 2, -1);
        roundCountdownDigitSequence = -1;
    }
    M460UpdateListenerPosition(object);
    if (MGSeqStatGet(roundResultSequence) == 0) {
        if (work->newRecord != 0) {
            M460SetScoreDisplayState(1);
            recordSequence = MGSeqCreate(0xE, work->recordPacked);
            work->newRecord = 0;
        }
        work->roundState = 6;
        work->roundTimer = 0;
        M460AdvanceGameState(object);
    }
}

OMOBJFUNC M460StartIntroCamera(HUPROCESS *process, OMOBJ *object)
{
    M460CameraWork *cameraWork;

    object->work[0] = object->work[1] = 0;
    M460SetCameraMode(1);
    cameraWork = M460FindPrimaryCamera(1);
    cameraWork->cameraTransitionBlend = 1.0f;
    cameraWork->reservedCameraValue = 0;
    return M460UpdateIntroCamera;
}

void M460UpdateIntroCamera(OMOBJ *object)
{
    Vec cameraValueDelta;
    M460CameraWork *cameraWork;
    M460DllWork *work;
    s32 transitionDirection;
    float transitionProgress;

    work = object->data;
    M460UpdateListenerPosition(object);
    object->work[0]++;
    transitionProgress = introCameraTransitionDurations[object->work[1]];
    transitionDirection = 1;
    if (transitionProgress < 0.0f) {
        transitionDirection = 0;
        transitionProgress *= -1.0f;
    }
    transitionProgress = object->work[0] / transitionProgress;
    VECSubtract(&introCameraCenters[object->work[1] + transitionDirection], &introCameraCenters[object->work[1]], &cameraValueDelta);
    VECScale(&cameraValueDelta, &cameraValueDelta, transitionProgress);
    VECAdd(&introCameraCenters[object->work[1]], &cameraValueDelta, &cameraValueDelta);
    Center.x = cameraValueDelta.x;
    Center.y = cameraValueDelta.y;
    Center.z = cameraValueDelta.z;
    VECSubtract(&introCameraRotations[object->work[1] + transitionDirection], &introCameraRotations[object->work[1]], &cameraValueDelta);
    VECScale(&cameraValueDelta, &cameraValueDelta, transitionProgress);
    VECAdd(&introCameraRotations[object->work[1]], &cameraValueDelta, &cameraValueDelta);
    CRot.x = cameraValueDelta.x;
    CRot.y = cameraValueDelta.y;
    CRot.z = 0.0f;
    CZoom = cameraValueDelta.z;
    cameraWork = M460FindPrimaryCamera(1);
    cameraWork->cameraCenter = Center;
    cameraWork->cameraRotation = CRot;
    cameraWork->cameraZoom = CZoom;
    cameraWork->fieldOfView = introCameraZoomLevels[object->work[1]] + (transitionProgress * (introCameraZoomLevels[object->work[1] + transitionDirection] - introCameraZoomLevels[object->work[1]]));
    if (transitionProgress >= 1.0f) {
        object->work[0] = 0;
        object->work[1]++;
        if (introCameraSoundVolumes[object->work[1]] > 0.0f) {
            M460StartMapCameraFade(introCameraSoundVolumes[object->work[1]]);
        }
        if (introCameraTransitionDurations[object->work[1]] == 0) {
            work->resultState = 2;
            M460AdvanceGameState(object);
        }
        else if (introCameraTransitionDurations[object->work[1] + 1] == 0) {
            work->resultState = 1;
        }
    }
}

OMOBJFUNC M460StartResultsCamera(HUPROCESS *process, OMOBJ *object)
{
    return M460UpdateResultsCamera;
}

void M460UpdateResultsCamera(OMOBJ *object)
{
    M460DllWork *work = object->data;
    s32 isReadyForResults = 1;
    M460UpdateListenerPosition(object);
    if (recordSequence >= 0) {
        if (MGSeqStatGet(recordSequence) != 0) {
            isReadyForResults = 0;
        }
        else {
            recordSequence = -1;
        }
    }
    if (work->playerStateFlags != 1) {
        isReadyForResults = 0;
    }
    if (isReadyForResults != 0) {
        work->resultSequence = 1;
        if (resultsMusicStream < 0) {
            resultsMusicStream = HuAudSStreamPlay(4);
        }
        if (work->resultTimer >= REFRESH_RATE_F * 3.5f) {
            work->resultSequence = 2;
            M460AdvanceGameState(object);
        }
    }
    else {
        work->resultTimer = 0;
    }
}

void M460InitializeCameras(HUPROCESS *process)
{
    Hu3DCameraCreate(0x1F);
    screenViewportObject = omAddObjEx(process, 0x7FDA, 0, 0, -1, omOutViewMulti);
    screenViewportObject->work[0] = 5;
    cameraManagerObject = omAddObjEx(process, 0xC, 1, 0, -1, M460CreateSplitScreenCameras);
    cameraManagerObject->work[0] = 0;
}

void M460CreateSplitScreenCameras(OMOBJ *object)
{
    M460ViewportRect cameraViewports[5] = {
        { 320.0f, 240.0f, 640.0f, 480.0f },
        { 160.0f, 120.0f, 320.0f, 240.0f },
        { 160.0f, 360.0f, 320.0f, 240.0f },
        { 480.0f, 120.0f, 320.0f, 240.0f },
        { 480.0f, 360.0f, 320.0f, 240.0f },
    };
    s32 cameraFlags[5] = { 1, 2, 4, 8, 16 };
    M460CameraWork *cameraWork;
    s32 cameraIndex;
    M460CameraWork *cameraWorks;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, 10 * sizeof(M460CameraWork), HU_MEMNUM_OVL);
    cameraWorks = object->data;
    memset(cameraWorks, 0, 10 * sizeof(M460CameraWork));
    object->mdlId[0] = Hu3DHookFuncCreate(M460DrawCameraBackground);
    Hu3DModelLayerSet(object->mdlId[0], 0);
    Hu3DModelCameraSet(object->mdlId[0], 0x1E);
    for (cameraWork = cameraWorks, cameraIndex = 0; cameraIndex < 5; cameraIndex++, cameraWork++) {
        cameraWork->isCameraActive = 0;
        cameraWork->cameraSortOrder = cameraIndex * 0x64;
        cameraWork->fieldOfView = 41.5f;
        cameraWork->nearClipDistance = 100.0f;
        cameraWork->farClipDistance = 5500.0f;
        cameraWork->cameraCenter.x = 0.0f;
        cameraWork->cameraCenter.y = 0.0f;
        cameraWork->cameraCenter.z = 0.0f;
        cameraWork->cameraRotation.x = -15.0f;
        cameraWork->cameraRotation.y = 0.0f;
        cameraWork->cameraRotation.z = 0.0f;
        cameraWork->cameraZoom = 1000.0f;
        cameraWork->viewportScale = 1.0f;
        cameraWork->viewportRect.left = cameraViewports[cameraIndex].left;
        cameraWork->viewportRect.top = cameraViewports[cameraIndex].top;
        cameraWork->viewportRect.width = cameraViewports[cameraIndex].width;
        cameraWork->viewportRect.height = cameraViewports[cameraIndex].height;
        cameraWork->cameraMask = cameraWork->cameraFlags = cameraFlags[cameraIndex];
        cameraWork->cameraSlot = cameraIndex;
        cameraWork->cameraIndex = cameraIndex;
    }
    cameraWork = cameraWorks;
    cameraWork->isCameraActive = 1;
    object->work[1] = object->work[2] = 0;
    object->work[0]++;
    M460ConfigureSplitScreenCameras(object);
    object->objFunc = M460UpdateSplitScreenCameras;
}

void M460UpdateSplitScreenCameras(OMOBJ *object)
{
    M460ConfigureSplitScreenCameras(object);
}

void M460ConfigureSplitScreenCameras(OMOBJ *object)
{
    float transitionProgress;
    M460CameraWork *cameraWork;
    s32 cameraIndex;
    s32 scissorCameraIndex;
    M460CameraWork *cameraWorks;
    u32 visibleCameraFlags;
    u32 sortOrder;
    s32 showCameraBackground;
    u32 scissorTop;
    u32 scissorWidth;
    u32 scissorHeight;

    cameraWorks = object->data;
    {
        s32 cameraMasks[4] = { 2, 4, 8, 16 };
        u32 cameraSortOrders[4];
        s32 cameraSortIndices[4];
        float viewportBounds[4];
        float viewportEdges[2];
        showCameraBackground = 0;
        visibleCameraFlags = 0;
        memcpy(&cameraWorks[5], cameraWorks, 5 * sizeof(M460CameraWork));
        object->work[1] = object->work[2];
        switch (object->work[1]) {
            case 0:
                break;
            case 1:
                visibleCameraFlags |= 1;
                break;
            case 2:
                visibleCameraFlags |= 1;
            case 3:
                showCameraBackground = 1;
            case 4:
                visibleCameraFlags |= 0x1E;
                break;
        }
        if (showCameraBackground != 0) {
            Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        }
        for (cameraIndex = 0; cameraIndex < 4; cameraIndex++) {
            cameraSortOrders[cameraIndex] = cameraWorks[cameraIndex + 6].cameraSortOrder;
            cameraSortIndices[cameraIndex] = cameraIndex;
        }
        for (cameraIndex = 0; cameraIndex < 3; cameraIndex++) {
            for (scissorCameraIndex = cameraIndex + 1; scissorCameraIndex < 4; scissorCameraIndex++) {
                if ((cameraIndex != scissorCameraIndex) && (cameraSortOrders[cameraIndex] > cameraSortOrders[scissorCameraIndex])) {
                    sortOrder = cameraSortOrders[cameraIndex];
                    cameraSortOrders[cameraIndex] = cameraSortOrders[scissorCameraIndex];
                    cameraSortOrders[scissorCameraIndex] = sortOrder;
                    sortOrder = cameraSortIndices[cameraIndex];
                    cameraSortIndices[cameraIndex] = cameraSortIndices[scissorCameraIndex];
                    cameraSortIndices[scissorCameraIndex] = sortOrder;
                }
            }
        }
        for (cameraIndex = 0; cameraIndex < 4; cameraIndex++) {
            cameraWorks[cameraSortIndices[cameraIndex] + 6].cameraMask = cameraMasks[cameraIndex];
            cameraWorks[cameraSortIndices[cameraIndex] + 6].cameraIndex = cameraIndex + 1;
        }

        for (cameraWork = &cameraWorks[5], cameraIndex = 0; cameraIndex < 5; cameraIndex++, cameraWork++) {
            if ((cameraWork->isCameraActive == 0) || ((visibleCameraFlags & cameraWork->cameraFlags) == 0)) {
                Hu3DCamera[cameraWork->cameraIndex].fov = -1.0f;
            }
            else {
                transitionProgress = 1.2f * ((cameraWork->viewportRect.width / 640.0f) / (cameraWork->viewportRect.height / 480.0f));
                viewportBounds[2] = cameraWork->viewportRect.width * cameraWork->viewportScale;
                viewportBounds[3] = cameraWork->viewportRect.height * cameraWork->viewportScale;
                Hu3DCameraPerspectiveSet(cameraWork->cameraMask, cameraWork->fieldOfView, cameraWork->nearClipDistance, cameraWork->farClipDistance, transitionProgress);
                Hu3DCameraViewportSet(cameraWork->cameraMask, cameraWork->viewportRect.left - (0.5f * viewportBounds[2]), cameraWork->viewportRect.top - (0.5f * viewportBounds[3]), viewportBounds[2],
                    viewportBounds[3], 0.0f, 1.0f);
                viewportBounds[0] = cameraWork->viewportRect.left - (0.5f * viewportBounds[2]);
                viewportBounds[1] = cameraWork->viewportRect.top - (0.5f * viewportBounds[3]);
                viewportEdges[0] = cameraWork->viewportRect.left + (0.5f * viewportBounds[2]);
                viewportEdges[1] = cameraWork->viewportRect.top + (0.5f * viewportBounds[3]);
                if (viewportBounds[0] < 0.0f) {
                    viewportBounds[0] = 0.0f;
                }
                if (viewportBounds[1] < 0.0f) {
                    viewportBounds[1] = 0.0f;
                }
                if (viewportEdges[0] > 640.0f) {
                    viewportEdges[0] = 640.0f;
                }
                if (viewportEdges[1] > 480.0f) {
                    viewportEdges[1] = 480.0f;
                }
                viewportEdges[0] -= viewportBounds[0];
                viewportEdges[1] -= viewportBounds[1];
                scissorHeight = viewportEdges[1];
                scissorWidth = viewportEdges[0];
                scissorTop = viewportBounds[1];
                Hu3DCameraScissorSet(cameraWork->cameraMask, viewportBounds[0], scissorTop, scissorWidth, scissorHeight);
                scissorCameraIndex = cameraWork->cameraIndex;
                CenterM[scissorCameraIndex].x = cameraWork->cameraCenter.x;
                CenterM[scissorCameraIndex].y = cameraWork->cameraCenter.y;
                CenterM[scissorCameraIndex].z = cameraWork->cameraCenter.z;
                CRotM[scissorCameraIndex].x = cameraWork->cameraRotation.x;
                CRotM[scissorCameraIndex].y = cameraWork->cameraRotation.y;
                CRotM[scissorCameraIndex].z = cameraWork->cameraRotation.z;
                CZoomM[scissorCameraIndex] = cameraWork->cameraZoom;
            }
        }
    }
}

void M460DrawCameraBackground(HU3DMODEL *model, Mtx mtx)
{
    Mtx projectionMatrix;
#ifdef NON_MATCHING
    Mtx44 projectionMatrix2;
#else
    Mtx projectionMatrix2;
#endif
    float transitionProgress;
    float viewportAspect;
    float cameraDepth;

    DCStoreRange(&cameraBackgroundColor, 4);
    C_MTXPerspective(projectionMatrix2, 41.5f, 1.2f, 100.0f, 5500.0f);
    GXSetProjection(projectionMatrix2, GX_PERSPECTIVE);
    MTXIdentity(projectionMatrix);
    GXLoadPosMtxImm(projectionMatrix, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevColor(GX_TEVREG0, cameraBackgroundColor);
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
    transitionProgress = 4125.0f;
    viewportAspect = 2750.0f;
    cameraDepth = 2530.0f;
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-transitionProgress, viewportAspect, -cameraDepth);
    GXPosition3f32(transitionProgress, viewportAspect, -cameraDepth);
    GXPosition3f32(transitionProgress, -viewportAspect, -cameraDepth);
    GXPosition3f32(-transitionProgress, -viewportAspect, -cameraDepth);
}

void M460SetCameraMode(u32 cameraMode)
{
    cameraManagerObject->work[2] = cameraMode;
}

u32 M460GetCameraMode(void)
{
    return cameraManagerObject->work[1];
}

M460CameraWork *M460FindPrimaryCamera(s32 cameraFlags)
{
    M460CameraWork *camera;
    s32 cameraIndex;
    M460CameraWork *cameraWorks;

    cameraWorks = cameraManagerObject->data;
    for (camera = cameraWorks, cameraIndex = 0; cameraIndex < 5; cameraIndex++, camera++) {
        if ((camera->cameraFlags & cameraFlags) != 0) {
            break;
        }
    }
    if (cameraIndex >= 5) {
        OSReport("M460:Camera Flag Error!!\n");
        return NULL;
    }
    return camera;
}

M460CameraWork *M460FindSecondaryCamera(s32 cameraFlags)
{
    M460CameraWork *camera;
    s32 cameraIndex;
    M460CameraWork *cameraWorks;

    cameraWorks = cameraManagerObject->data;
    for (camera = &cameraWorks[5], cameraIndex = 0; cameraIndex < 5; cameraIndex++, camera++) {
        if ((camera->cameraFlags & cameraFlags) != 0) {
            break;
        }
    }
    if (cameraIndex >= 5) {
        OSReport("M460:Camera Flag Error!!\n");
        return NULL;
    }
    return camera;
}

M460CameraWork *M460GetPrimaryCameraByIndex(u32 cameraIndex)
{
    M460CameraWork *cameraWorks = cameraManagerObject->data;
    if (cameraIndex >= 5) {
        OSReport("M460:Camera No. Error!!\n");
        return NULL;
    }
    return &cameraWorks[cameraIndex];
}

M460CameraWork *M460GetSecondaryCameraByIndex(u32 cameraIndex)
{
    M460CameraWork *cameraWorks = cameraManagerObject->data;
    if (cameraIndex >= 5) {
        OSReport("M460:Camera No. Error!!\n");
        return NULL;
    }
    return &cameraWorks[cameraIndex + 5];
}

void M460SetModelCameraMask(s32 modelId, s32 cameraFlags)
{
    M460CameraWork *cameraWork;
    s32 cameraIndex;
    s32 cameraMask;
    M460CameraWork *secondaryCameraWorks;
    M460CameraWork *cameraWorks;

    cameraMask = 0;
    if (modelId >= 0) {
        cameraWorks = cameraManagerObject->data;
        secondaryCameraWorks = &cameraWorks[5];
        for (cameraWork = secondaryCameraWorks, cameraIndex = 0; cameraIndex < 5; cameraIndex++, cameraWork++) {
            if ((cameraWork->cameraFlags & cameraFlags) != 0) {
                cameraMask |= cameraWork->cameraMask;
            }
        }
        Hu3DModelCameraSet(modelId, cameraMask);
    }
}

s32 M460GetGameState(void)
{
    return ((M460DllWork *)gameObject->data)->gameState;
}

u32 M460GetResultState(void)
{
    return ((M460DllWork *)gameObject->data)->resultState;
}

u32 M460GetRoundState(void)
{
    return ((M460DllWork *)gameObject->data)->roundState;
}

s32 M460GetResultSequence(void)
{
    return ((M460DllWork *)gameObject->data)->resultSequence;
}

s32 M460IsGameActive(void)
{
    return 1;
}

s32 M460IsGameComplete(void)
{
    return ((M460DllWork *)gameObject->data)->gameComplete;
}

void M460SetPlayerResult(s32 playerIndex, s32 playerRank, s32 playerResult)
{
    M460DllWork *work = gameObject->data;
    work->playerRanks[playerIndex &= 3] = playerRank;
    work->playerResults[playerIndex] = playerResult;
}

void M460SetPlayerStateFlag(s32 playerIndex)
{
    ((M460DllWork *)gameObject->data)->playerStateFlags |= (1 << ((playerIndex & 7) * 4));
}

void M460SetCameraStateFlag(s32 cameraIndex)
{
    ((M460DllWork *)gameObject->data)->cameraStateFlags |= (1 << ((cameraIndex & 3) * 4));
}

void M460SetResultFlag(s32 resultIndex)
{
    ((M460DllWork *)gameObject->data)->resultFlags |= (1 << ((resultIndex & 3) * 4));
}

s32 M460IsRoundResultSequenceDone(void)
{
    return MGSeqStatGet(roundResultSequence) == 0;
}

void M460SetSelectionState(u32 selectionState)
{
    M460DllWork *work = gameObject->data;
    if (work->selectionState == 3) {
        work->selectionState = selectionState;
    }
}

s32 M460GetSelectionState(void)
{
    return ((M460DllWork *)gameObject->data)->selectionState;
}

void M460ConfirmSelection(void)
{
    ((M460DllWork *)gameObject->data)->isSelectionConfirmed = 1;
}

s32 M460IsSelectionConfirmed(void)
{
    return ((M460DllWork *)gameObject->data)->isSelectionConfirmed;
}

void M460MarkSelectionReady(void)
{
    ((M460DllWork *)gameObject->data)->isSelectionReady = 1;
}

s32 M460IsSelectionReady(void)
{
    return ((M460DllWork *)gameObject->data)->isSelectionReady;
}

u32 M460GetCurrentFrameCount(void)
{
    return ((M460DllWork *)gameObject->data)->currentFrames;
}

void M460SetCameraLookAt(Vec *cameraPosition, Vec *lookAtPosition)
{
    Vec cameraOffset;
    float unusedCameraTransitionValue;
    float unusedCameraAspectValue;

    VECSubtract(lookAtPosition, cameraPosition, &cameraOffset);
    Center = *lookAtPosition;
    CRot.x = atan2d(cameraOffset.y, sqrtf((cameraOffset.x * cameraOffset.x) + (cameraOffset.z * cameraOffset.z)));
    CRot.y = atan2d(-cameraOffset.x, -cameraOffset.z);
    CRot.z = 0.0f;
    CZoom = VECMag(&cameraOffset);
}

void M460SetCameraLookAtPoints(float cameraPositionX, float cameraPositionY, float cameraPositionZ, float lookAtX, float lookAtY, float lookAtZ)
{
    Vec cameraPosition;
    Vec lookAtPosition;
    Vec viewVector;

    cameraPosition.x = cameraPositionX;
    cameraPosition.y = cameraPositionY;
    cameraPosition.z = cameraPositionZ;
    lookAtPosition.x = lookAtX;
    lookAtPosition.y = lookAtY;
    lookAtPosition.z = lookAtZ;
    VECSubtract(&lookAtPosition, &cameraPosition, &viewVector);
    Center = lookAtPosition;
    CRot.x = atan2d(viewVector.y, sqrtf((viewVector.x * viewVector.x) + (viewVector.z * viewVector.z)));
    CRot.y = atan2d(-viewVector.x, -viewVector.z);
    CRot.z = 0.0f;
    CZoom = VECMag(&viewVector);
}

float M460ApproachAngle(float currentAngle, float targetAngle, float approachRate)
{
    float angleDelta;
    float normalizedAngle;
    float interpolatedAngle;

    normalizedAngle = fmod(targetAngle - currentAngle, 360.0);
    angleDelta = normalizedAngle;
    if (angleDelta < 0.0f) {
        angleDelta += 360.0f;
    }
    if (angleDelta > 180.0f) {
        angleDelta -= 360.0f;
    }
    interpolatedAngle = fmod(currentAngle + (approachRate * angleDelta), 360.0);
    angleDelta = interpolatedAngle;
    if (angleDelta < 0.0f) {
        angleDelta += 360.0f;
    }
    return angleDelta;
}

float M460ApproachAngleDirect(float currentAngle, float targetAngle)
{
    float angleDelta;
    float normalizedAngle;

    normalizedAngle = fmod(targetAngle - currentAngle, 360.0);
    angleDelta = normalizedAngle;
    if (angleDelta < 0.0f) {
        angleDelta += 360.0f;
    }
    if (angleDelta > 180.0f) {
        angleDelta -= 360.0f;
    }
    return angleDelta;
}
