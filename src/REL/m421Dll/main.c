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

#include "REL/m421Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

typedef struct M421DllWork {
    u32 flowState;
    s32 flowTimer;
    s32 skipIntro;
    u32 cameraMode;
    s32 introTimer;
    u32 resultState;
    u32 resultTimer;
    u32 resultCountdown;
    u32 countdownFrame;
    u32 reserved24;
    u32 exitState;
    u32 exitTimer;
    s32 playerOrder[4];
    s32 playerStatus[4];
    s32 winnerPlayerIds[4];
    u32 playerResultFlags;
    u32 resultMode;
    u32 playerReadyFlags;
    u32 playerConfirmFlags;
    s32 playerExitFlags;
} M421DllWork; /* size = 0x74 */

void MinigamePlayCallback(OMOBJ *object);
void MinigameResultCallback(OMOBJ *object);
void MinigameCountdownCallback(OMOBJ *object);
void MinigameResultWaitCallback(OMOBJ *object);
OMOBJFUNC StartCameraIntro(HUPROCESS *process, OMOBJ *object);
void UpdateCameraIntro(OMOBJ *object);
OMOBJFUNC StartResultsCamera(HUPROCESS *process, OMOBJ *object);
void UpdateResultsCamera(OMOBJ *object);
void CreateCameraManager(HUPROCESS *process);
void InitializeCameras(OMOBJ *object);
void UpdateCameraLayoutCallback(OMOBJ *object);
void UpdateCameraLayout(OMOBJ *object);
void DrawCameraBackground(HU3DMODEL *model, Mtx mtx);
void SetCameraViewMode(u32 cameraIndex);
M421DllCameraStruct *GetCameraForViewFlag(s32 cameraIndex);
M421DllCameraStruct *GetCameraByIndex(u32 cameraIndex);
float InterpolateAngle(float currentAngle, float targetAngle, float interpolation);
void CreateMinigameWork(OMOBJ *object);
void AdvanceMinigameFlow(OMOBJ *object);
void MinigameMainCallback(OMOBJ *object);
void MinigameExitCallback(OMOBJ *object);

Vec keyLightPosition = { 100.0f, 800.0f, -100.0f };
Vec keyLightDirection = { 0.3f, -0.8f, 0.3f };
Vec shadowLightRotation = { 10.0f, 45.0f, 3500.0f };
GXColor keyLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };
Vec shadowPosition = { 50.0f, 150000.0f, 50.0f };
Vec shadowUpDirection = { 0.0f, 1.0f, 0.0f };
Vec shadowTarget = { 0.0f, 0.0f, 0.0f };
Vec cameraIntroPositions[3] = {
    { 0.0f, 200.0f, -50.0f },
    { 0.0f, 200.0f, -50.0f },
    { 0.0f, 200.0f, -50.0f },
};
Vec cameraIntroTargets[3] = {
    { -37.5f, 0.0f, 1500.0f },
    { -37.5f, 0.0f, 1500.0f },
    { -37.5f, 0.0f, 2300.0f },
};
s32 cameraIntroDurations[3] = { REFRESH_RATE * 4, REFRESH_RATE / 2, 0 };
GXColor transparentBlack = { 0, 0, 0, 0 };

HUPROCESS *objectManager;
OMOBJ *minigameObject;
OMOBJ *cameraOutputObject;
// M421DllCameraStruct
OMOBJ *cameraManagerObject;
s16 resultSequenceId;
s16 globalLightIds[3];
s16 countdownSequenceId;
s16 introSequenceId;
s16 winnerSequenceId;
s32 cameraShakeTimer;
s32 backgroundMusicSequenceId;
s32 resultsMusicStreamId;

void ObjectSetup(void)
{
    Mtx lightRotationMatrix;
    Vec listenerPosition = { 0.0f, 0.0f, 0.0f };
    Vec listenerForward = { 0.0f, 0.0f, -1.0f };
    Vec lightPosition;
    HUPROCESS *process;
    HU3DLIGHT *lightData;

    cameraShakeTimer = 0;
    HuAudSndGrpSet(0x2E);
    HuAudFXListnerSetEX(
        &listenerPosition, &listenerForward,
        11200.0f, (VERSION_PAL ? 4280.0f : 4166.6665f),
        0.0f, 300.0f, 0.0f
    );
    backgroundMusicSequenceId = resultsMusicStreamId = -1;
    resultSequenceId = -1;
    shadowPosition.x = 200.0f;
    shadowPosition.y = 1200.0f;
    shadowPosition.z = 160.0f;
    shadowUpDirection.x = 0.0f;
    shadowUpDirection.y = 1.0f;
    shadowUpDirection.z = 0.0f;
    shadowTarget.x = 0.0f;
    shadowTarget.y = 0.0f;
    shadowTarget.z = 0.0f;
    Hu3DShadowCreate(22.0f, 20.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.625f);
    Hu3DShadowPosSet(&shadowPosition, &shadowUpDirection, &shadowTarget);
    globalLightIds[0] = Hu3DGLightCreateV(&keyLightPosition, &keyLightDirection, &keyLightColor);
    Hu3DGLightInfinitytSet(globalLightIds[0]);
    lightData = &Hu3DGlobalLight[globalLightIds[0]];
    lightData->type |= 0x8000;
    shadowLightRotation.x = 1.0f;
    shadowLightRotation.y = 0.0f;
    shadowLightRotation.z = 4200.0f;
    lightPosition.x = lightPosition.z = 0.0f;
    lightPosition.y = shadowLightRotation.z;
    mtxRot(lightRotationMatrix, shadowLightRotation.x, shadowLightRotation.y, 0.0f);
    MTXMultVec(lightRotationMatrix, &lightPosition, &shadowPosition);
    Hu3DGLightPosAimSetV(globalLightIds[0], &shadowPosition, &shadowTarget);
    Hu3DShadowPosSet(&shadowPosition, &shadowUpDirection, &shadowTarget);
    countdownSequenceId = introSequenceId = winnerSequenceId = -1;
    process = omInitObjMan(0x32, 0x2000);
    objectManager = process;
    omGameSysInit(process);
    CreateCameraManager(process);
    minigameObject = omAddObjEx(process, 0xA, 0, 0, -1, CreateMinigameWork);
    CreateHost(process);
    CreatePlayers(process);
}

void CreateMinigameWork(OMOBJ *object)
{
    M421DllWork *work;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M421DllWork), HU_MEMNUM_OVL);
    work = object->data;
    memset(work, 0, sizeof(M421DllWork));
    work->flowState = 0;
    work->flowTimer = 0;
    work->cameraMode = 0;
    work->introTimer = 0;
    work->resultState = 0;
    work->resultTimer = 0;
    work->exitState = 0;
    work->exitTimer = 0;
    work->skipIntro = 0;
    work->resultMode = 2;
    work->winnerPlayerIds[0] = work->winnerPlayerIds[1] = work->winnerPlayerIds[2] = work->winnerPlayerIds[3] = -1;
    work->playerReadyFlags = 0;
    work->playerExitFlags = 0;
    object->objFunc = MinigameMainCallback;
}

void UpdateMinigame(OMOBJ *object)
{
    Vec listenerPosition;
    Vec listenerDirection;
    M421DllCameraStruct *cameraWork;

    M421DllWork *work = object->data;
    work->flowTimer++;
    work->introTimer++;
    work->resultTimer++;
    work->exitTimer++;
    if ((omSysExitReq != 0) && (work->flowState != 5)) {
        work->flowState = 4;
        AdvanceMinigameFlow(object);
    }
    cameraWork = GetCameraForViewFlag(1);
    listenerDirection.x = -(sind(cameraWork->rotation.y) * cosd(cameraWork->rotation.x));
    listenerDirection.y = -(-sind(cameraWork->rotation.x));
    listenerDirection.z = -(cosd(cameraWork->rotation.y) * cosd(cameraWork->rotation.x));
    listenerPosition.x = cameraWork->lookAtTarget.x - (listenerDirection.x * cameraWork->zoomDistance);
    listenerPosition.y = cameraWork->lookAtTarget.y - (listenerDirection.y * cameraWork->zoomDistance);
    listenerPosition.z = cameraWork->lookAtTarget.z - (listenerDirection.z * cameraWork->zoomDistance);
    HuAudFXListnerUpdate(&listenerPosition, &listenerDirection);
}

void AdvanceMinigameFlow(OMOBJ *object)
{
    M421DllWork *work = object->data;

    if (work->skipIntro == 0) {
        switch (work->flowState) {
            case 0:
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 0x3C);
                work->flowState = 1;
                work->introTimer = 0;
                object->objFunc = StartCameraIntro(objectManager, object);
                break;
            case 1:
                work->flowState = 2;
                work->resultTimer = 0;
                object->objFunc = MinigameResultCallback;
                break;
            case 2:
                work->flowState = 3;
                work->exitTimer = 0;
                object->objFunc = StartResultsCamera(objectManager, object);
                break;
            case 3:
            case 4:
                work->skipIntro = 1;
                work->flowState = 5;
                object->objFunc = MinigameExitCallback;
                break;
            case 5:
            default:
                work->flowState = 5;
                object->objFunc = MinigamePlayCallback;
        }
        work->flowTimer = 0;
    }
}

void MinigameMainCallback(OMOBJ *object)
{
    UpdateMinigame(object);
    AdvanceMinigameFlow(object);
}

void MinigameExitCallback(OMOBJ *object)
{
    M421DllWork *work = object->data;
    UpdateMinigame(object);
    if (WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 0x3C);
        work->skipIntro = 0;
        AdvanceMinigameFlow(object);
    }
}

void MinigamePlayCallback(OMOBJ *object)
{
    UpdateMinigame(object);
    if (WipeStatGet() == 0) {
        ResetPlayerMotion();
        EmptyHostCallback();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void MinigameResultCallback(OMOBJ *object)
{
    M421DllWork *work = object->data;
    UpdateMinigame(object);
    switch (work->resultState) {
        case 0:
            work->resultCountdown = 0x2D;
            work->countdownFrame = REFRESH_RATE;
            countdownSequenceId = MGSeqCreate(3, 0);
            MGSeqPosSet(countdownSequenceId, 320.0f, 240.0f);
            resultSequenceId = -1;
            work->resultState = 1;
            work->resultTimer = 0;
            break;
        case 1:
            if ((backgroundMusicSequenceId < 0) && ((MGSeqStatGet(countdownSequenceId) & 0x10) != 0)) {
                backgroundMusicSequenceId = HuAudSeqPlay(0x48);
            }
            if (MGSeqStatGet(countdownSequenceId) == 0 && (work->skipIntro == 0)) {
                work->resultState = 2;
                work->resultTimer = 0;
                object->objFunc = MinigameCountdownCallback;
            }
            break;
    }
}

void MinigameCountdownCallback(OMOBJ *object)
{
    M421DllWork *work = object->data;
    s32 resultReady = 0;
    UpdateMinigame(object);
    if (--work->countdownFrame == 0) {
        work->countdownFrame = REFRESH_RATE;
        work->resultCountdown--;
        if ((work->resultCountdown == 0) && (work->resultMode == 2)) {
            work->resultMode = 1;
            resultReady = 1;
        }
    }
    if (resultSequenceId < 0) {
        resultSequenceId = MGSeqCreate(1, work->resultCountdown, -1, -1);
    }
    MGSeqParamSet(resultSequenceId, 1, work->resultCountdown);
    if (work->resultMode != 2) {
        resultReady = 1;
    }
    if (resultReady != 0) {
        introSequenceId = MGSeqCreate(3, 1);
        MGSeqPosSet(introSequenceId, 320.0f, 240.0f);
        HuAudSeqFadeOut(backgroundMusicSequenceId, 0x64);
        work->resultState = 3;
        work->resultTimer = 0;
        if (work->skipIntro == 0) {
            object->objFunc = MinigameResultWaitCallback;
        }
    }
}

void MinigameResultWaitCallback(OMOBJ *object)
{
    M421DllWork *work = object->data;
    s32 resultReady = 0;
    if (resultSequenceId >= 0) {
        MGSeqParamSet(resultSequenceId, 2, -1);
        resultSequenceId = -1;
    }
    UpdateMinigame(object);
    if (work->resultState == 3) {
        resultReady = 1;
        if ((MGSeqStatGet(introSequenceId) == 0) && (work->playerReadyFlags == 0x1111) && (resultReady != 0)) {
            work->resultState = 4;
            work->resultTimer = 0;
        }
    }
    else {
        work->resultState = 5;
        work->resultTimer = 0;
        AdvanceMinigameFlow(object);
    }
}

OMOBJFUNC StartCameraIntro(HUPROCESS *process, OMOBJ *object)
{
    M421DllCameraStruct *cameraWork;

    object->work[0] = object->work[1] = 0;
    SetCameraViewMode(1);
    cameraWork = GetCameraByIndex(0);
    cameraWork->shakeIntensity = 1.0f;
    cameraWork->shakeTimer = 0;
    return UpdateCameraIntro;
}

void UpdateCameraIntro(OMOBJ *object)
{
    Vec cameraPosition;
    float transitionProgress;
    M421DllCameraStruct *cameraWork;
    M421DllWork *work;

    work = object->data;
    UpdateMinigame(object);
    work->cameraMode = 1;
    object->work[0]++;
    transitionProgress = (float)object->work[0] / cameraIntroDurations[object->work[1]];
    VECSubtract(&cameraIntroPositions[object->work[1] + 1], &cameraIntroPositions[object->work[1]], &cameraPosition);
    VECScale(&cameraPosition, &cameraPosition, transitionProgress);
    VECAdd(&cameraIntroPositions[object->work[1]], &cameraPosition, &cameraPosition);
    Center.x = cameraPosition.x;
    Center.y = cameraPosition.y;
    Center.z = cameraPosition.z;
    VECSubtract(&cameraIntroTargets[object->work[1] + 1], &cameraIntroTargets[object->work[1]], &cameraPosition);
    VECScale(&cameraPosition, &cameraPosition, transitionProgress);
    VECAdd(&cameraIntroTargets[object->work[1]], &cameraPosition, &cameraPosition);
    CRot.x = cameraPosition.x;
    CRot.y = cameraPosition.y;
    CRot.z = 0.0f;
    CZoom = cameraPosition.z;
    cameraWork = GetCameraByIndex(0);
    cameraWork->lookAtTarget = Center;
    cameraWork->rotation = CRot;
    cameraWork->zoomDistance = CZoom;
    cameraWork->shakeTimer++;
    if (cameraWork->shakeTimer > REFRESH_RATE_F * 3) {
        cameraWork->shakeIntensity -= 1.5f * REFRESH_FREQ;
        if (cameraWork->shakeIntensity < 0.0f) {
            cameraWork->shakeIntensity = 0.0f;
        }
        cameraWork->lookAtTarget.x += cameraWork->shakeIntensity * ((0.02f * frandmod(0x3E8)) - 10.0f);
        cameraWork->lookAtTarget.y += cameraWork->shakeIntensity * ((0.02f * frandmod(0x3E8)) - 10.0f);
        cameraWork->lookAtTarget.z += cameraWork->shakeIntensity * ((0.02f * frandmod(0x3E8)) - 10.0f);
    }
    if (transitionProgress >= 1.0f) {
        object->work[0] = 0;
        object->work[1]++;
        if (cameraIntroDurations[object->work[1]] == 0) {
            work->cameraMode = 2;
            AdvanceMinigameFlow(object);
        }
    }
}

OMOBJFUNC StartResultsCamera(HUPROCESS *process, OMOBJ *object)
{
    M421DllCameraStruct *cameraWork;
    s32 playerIndex;
    M421DllWork *resultWork;
    s32 awardWinners;

    resultWork = object->data;
    awardWinners = 1;
    cameraWork = GetCameraByIndex(0);
    if (resultWork->resultMode == 1) {
        awardWinners = 0;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        resultWork->winnerPlayerIds[playerIndex] = -1;
        if ((resultWork->playerStatus[playerIndex] == 0) == awardWinners) {
            resultWork->winnerPlayerIds[playerIndex] = resultWork->playerOrder[playerIndex];
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (resultWork->winnerPlayerIds[playerIndex] >= 0) {
            GWPlayerCoinWinAdd(playerIndex, 10);
        }
    }
    cameraWork->targetPositionX = 0.0f;
    cameraWork->targetPositionY = 150.0f;
    cameraWork->targetPositionZ = 0.0f;
    cameraWork->targetPitch = -30.0f;
    cameraWork->targetYaw = 0.0f;
    cameraWork->targetZoom = 1000.0f;
    cameraWork->shakeIntensity = 0.0f;
    return UpdateResultsCamera;
}

void UpdateResultsCamera(OMOBJ *object)
{
    M421DllCameraStruct *cameraWork;
    M421DllWork *resultWork;

    resultWork = object->data;
    cameraWork = GetCameraByIndex(0);
    UpdateMinigame(object);
    cameraWork->shakeIntensity += REFRESH_FREQ * 0.1f;
    if (cameraWork->shakeIntensity > 0.1f) {
        cameraWork->shakeIntensity = 0.1f;
    }
    cameraWork->lookAtTarget.y += cameraWork->shakeIntensity * (cameraWork->targetPositionY - cameraWork->lookAtTarget.y);
    cameraWork->lookAtTarget.z += cameraWork->shakeIntensity * (cameraWork->targetPositionZ - cameraWork->lookAtTarget.z);
    cameraWork->rotation.x = InterpolateAngle(cameraWork->rotation.x, cameraWork->targetPitch, cameraWork->shakeIntensity);
    cameraWork->zoomDistance += cameraWork->shakeIntensity * (cameraWork->targetZoom - cameraWork->zoomDistance);
    if (resultWork->playerConfirmFlags == 0x1111) {
        resultWork->exitState = 1;
        if (resultsMusicStreamId < 0) {
            resultsMusicStreamId = HuAudSStreamPlay(1);
        }
        if (winnerSequenceId < 0) {
            winnerSequenceId = MGSeqCreate(5, 3, resultWork->winnerPlayerIds[0], resultWork->winnerPlayerIds[1], resultWork->winnerPlayerIds[2], resultWork->winnerPlayerIds[3]);
            return;
        }
        if ((MGSeqStatGet(winnerSequenceId) == 0) && (resultWork->exitTimer >= REFRESH_RATE_F * 3.5f)) {
            resultWork->exitState = 2;
            AdvanceMinigameFlow(object);
        }
    }
}

void CreateCameraManager(HUPROCESS *process)
{
    Hu3DCameraCreate(0x1F);
    cameraOutputObject = omAddObjEx(process, 0x7FDA, 0, 0, -1, omOutViewMulti);
    cameraOutputObject->work[0] = 5;
    cameraManagerObject = omAddObjEx(process, 0xC, 1, 0, -1, InitializeCameras);
    cameraManagerObject->work[0] = 0;
}

void InitializeCameras(OMOBJ *object)
{
    M421DllViewport viewportLayouts[5] = {
        { 320.0f, 240.0f, 640.0f, 480.0f },
        { 160.0f, 120.0f, 320.0f, 240.0f },
        { 160.0f, 360.0f, 320.0f, 240.0f },
        { 480.0f, 120.0f, 320.0f, 240.0f },
        { 480.0f, 360.0f, 320.0f, 240.0f },
    };
    s32 cameraFlags[5] = { 1, 2, 4, 8, 16 };
    M421DllCameraStruct *cameraWork;
    s32 cameraIndex;
    M421DllCameraStruct *cameraWorkArray;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, 10 * sizeof(M421DllCameraStruct), HU_MEMNUM_OVL);
    cameraWorkArray = object->data;
    memset(cameraWorkArray, 0, 10 * sizeof(M421DllCameraStruct));
    object->mdlId[0] = Hu3DHookFuncCreate(DrawCameraBackground);
    Hu3DModelLayerSet(object->mdlId[0], 0);
    Hu3DModelCameraSet(object->mdlId[0], 0x1E);
    for (cameraWork = cameraWorkArray, cameraIndex = 0; cameraIndex < 5; cameraIndex++, cameraWork++) {
        cameraWork->isActive = 1;
        cameraWork->sortPriority = cameraIndex * 0x64;
        cameraWork->fieldOfView = 41.5f;
        cameraWork->nearClip = 100.0f;
        cameraWork->farClip = 5500.0f;
        cameraWork->lookAtTarget.x = 0.0f;
        cameraWork->lookAtTarget.y = 0.0f;
        cameraWork->lookAtTarget.z = 0.0f;
        cameraWork->rotation.x = -15.0f;
        cameraWork->rotation.y = 0.0f;
        cameraWork->rotation.z = 0.0f;
        cameraWork->zoomDistance = 1000.0f;
        cameraWork->viewportScale = 1.0f;
        cameraWork->viewport.centerX = viewportLayouts[cameraIndex].centerX;
        cameraWork->viewport.centerY = viewportLayouts[cameraIndex].centerY;
        cameraWork->viewport.width = viewportLayouts[cameraIndex].width;
        cameraWork->viewport.height = viewportLayouts[cameraIndex].height;
        cameraWork->cameraId = cameraWork->viewFlags = cameraFlags[cameraIndex];
        cameraWork->viewIndex = cameraIndex;
        cameraWork->outputCameraIndex = cameraIndex;
    }
    object->work[1] = 0;
    object->work[0]++;
    UpdateCameraLayout(object);
    object->objFunc = UpdateCameraLayoutCallback;
}

void UpdateCameraLayoutCallback(OMOBJ *object)
{
    UpdateCameraLayout(object);
}

void UpdateCameraLayout(OMOBJ *object)
{
    float aspectRatio;
    M421DllCameraStruct *cameraWork;
    s32 cameraIndex;
    s32 splitIndex;
    M421DllCameraStruct *cameraWorkArray;
    u32 activeViewFlags;
    u32 sortValue;
    s32 layoutMode;
    u32 scissorTop;
    u32 scissorWidth;
    u32 scissorHeight;

    cameraWorkArray = object->data;
    {
        s32 layoutPriority[4] = { 2, 4, 8, 16 };
        u32 layoutSortValues[4];
        s32 layoutOrder[4];
        float viewport[4];
        float scissorSize[2];
        layoutMode = 0;
        activeViewFlags = 0;
        memcpy(&cameraWorkArray[5], cameraWorkArray, 5 * sizeof(M421DllCameraStruct));
        switch (object->work[1]) {
            case 0:
                break;
            case 1:
                activeViewFlags |= 1;
                break;
            case 2:
                activeViewFlags |= 1;
            case 3:
                layoutMode = 1;
            case 4:
                activeViewFlags |= 0x1E;
                break;
        }
        if (layoutMode != 0) {
            Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        }
        for (cameraIndex = 0; cameraIndex < 4; cameraIndex++) {
            layoutSortValues[cameraIndex] = cameraWorkArray[cameraIndex + 6].sortPriority;
            layoutOrder[cameraIndex] = cameraIndex;
        }
        for (cameraIndex = 0; cameraIndex < 3; cameraIndex++) {
            for (splitIndex = cameraIndex + 1; splitIndex < 4; splitIndex++) {
                if ((cameraIndex != splitIndex) && (layoutSortValues[cameraIndex] > layoutSortValues[splitIndex])) {
                    sortValue = layoutSortValues[cameraIndex];
                    layoutSortValues[cameraIndex] = layoutSortValues[splitIndex];
                    layoutSortValues[splitIndex] = sortValue;
                    sortValue = layoutOrder[cameraIndex];
                    layoutOrder[cameraIndex] = layoutOrder[splitIndex];
                    layoutOrder[splitIndex] = sortValue;
                }
            }
        }
        for (cameraIndex = 0; cameraIndex < 4; cameraIndex++) {
            cameraWorkArray[layoutOrder[cameraIndex] + 6].cameraId = layoutPriority[cameraIndex];
            cameraWorkArray[layoutOrder[cameraIndex] + 6].outputCameraIndex = cameraIndex + 1;
        }

        for (cameraWork = &cameraWorkArray[5], cameraIndex = 0; cameraIndex < 5; cameraIndex++, cameraWork++) {
            if ((cameraWork->isActive == 0) || ((activeViewFlags & cameraWork->viewFlags) == 0)) {
                Hu3DCamera[cameraWork->outputCameraIndex].fov = -1.0f;
            }
            else {
                aspectRatio = 1.2f * ((cameraWork->viewport.width / 640.0f) / (cameraWork->viewport.height / 480.0f));
                viewport[2] = cameraWork->viewport.width * cameraWork->viewportScale;
                viewport[3] = cameraWork->viewport.height * cameraWork->viewportScale;
                Hu3DCameraPerspectiveSet(cameraWork->cameraId, cameraWork->fieldOfView, cameraWork->nearClip, cameraWork->farClip, aspectRatio);
                Hu3DCameraViewportSet(cameraWork->cameraId, cameraWork->viewport.centerX - (0.5f * viewport[2]), cameraWork->viewport.centerY - (0.5f * viewport[3]), viewport[2],
                    viewport[3], 0.0f, 1.0f);
                viewport[0] = cameraWork->viewport.centerX - (0.5f * viewport[2]);
                viewport[1] = cameraWork->viewport.centerY - (0.5f * viewport[3]);
                scissorSize[0] = cameraWork->viewport.centerX + (0.5f * viewport[2]);
                scissorSize[1] = cameraWork->viewport.centerY + (0.5f * viewport[3]);
                if (viewport[0] < 0.0f) {
                    viewport[0] = 0.0f;
                }
                if (viewport[1] < 0.0f) {
                    viewport[1] = 0.0f;
                }
                if (scissorSize[0] > 640.0f) {
                    scissorSize[0] = 640.0f;
                }
                if (scissorSize[1] > 480.0f) {
                    scissorSize[1] = 480.0f;
                }
                scissorSize[0] -= viewport[0];
                scissorSize[1] -= viewport[1];
                scissorHeight = scissorSize[1];
                scissorWidth = scissorSize[0];
                scissorTop = viewport[1];
                Hu3DCameraScissorSet(cameraWork->cameraId, viewport[0], scissorTop, scissorWidth, scissorHeight);
                splitIndex = cameraWork->outputCameraIndex;
                CenterM[splitIndex].x = cameraWork->lookAtTarget.x;
                CenterM[splitIndex].y = cameraWork->lookAtTarget.y;
                CenterM[splitIndex].z = cameraWork->lookAtTarget.z;
                CRotM[splitIndex].x = cameraWork->rotation.x;
                CRotM[splitIndex].y = cameraWork->rotation.y;
                CRotM[splitIndex].z = cameraWork->rotation.z;
                CZoomM[splitIndex] = cameraWork->zoomDistance;
            }
        }
    }
}

void DrawCameraBackground(HU3DMODEL *model, Mtx mtx)
{
    Mtx identityMatrix;
    float quadHalfWidth;
    float quadHalfHeight;
    float quadDepth;

    MTXIdentity(identityMatrix);
    GXLoadPosMtxImm(identityMatrix, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevColor(GX_TEVREG0, transparentBlack);
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
    quadHalfWidth = 4125.0f;
    quadHalfHeight = 2750.0f;
    quadDepth = 5494.5f;
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-quadHalfWidth, quadHalfHeight, -quadDepth);
    GXPosition3f32(quadHalfWidth, quadHalfHeight, -quadDepth);
    GXPosition3f32(quadHalfWidth, -quadHalfHeight, -quadDepth);
    GXPosition3f32(-quadHalfWidth, -quadHalfHeight, -quadDepth);
}

void SetCameraViewMode(u32 viewMode)
{
    cameraManagerObject->work[1] = viewMode;
}

u32 GetCameraViewMode(void)
{
    return cameraManagerObject->work[1];
}

M421DllCameraStruct *GetCameraForViewFlag(s32 requestedViewFlags)
{
    M421DllCameraStruct *cameraWork;
    s32 cameraIndex;
    M421DllCameraStruct *cameraWorkArray;

    cameraWorkArray = cameraManagerObject->data;
    for (cameraWork = cameraWorkArray, cameraIndex = 0; cameraIndex < 5; cameraIndex++, cameraWork++) {
        if ((cameraWork->viewFlags & requestedViewFlags) != 0) {
            break;
        }
    }
    if (cameraIndex >= 5) {
        OSReport("M421:Camera Flag Error!!\n");
        return NULL;
    }
    return cameraWork;
}

M421DllCameraStruct *GetRenderCameraForViewFlag(s32 requestedViewFlags)
{
    M421DllCameraStruct *cameraWork;
    s32 cameraIndex;
    M421DllCameraStruct *cameraWorkArray;

    cameraWorkArray = cameraManagerObject->data;
    for (cameraWork = &cameraWorkArray[5], cameraIndex = 0; cameraIndex < 5; cameraIndex++, cameraWork++) {
        if ((cameraWork->viewFlags & requestedViewFlags) != 0) {
            break;
        }
    }
    if (cameraIndex >= 5) {
        OSReport("M421:Camera Flag Error!!\n");
        return NULL;
    }
    return cameraWork;
}

M421DllCameraStruct *GetCameraByIndex(u32 cameraIndex)
{
    M421DllCameraStruct *cameraWorkArray = cameraManagerObject->data;
    if (cameraIndex >= 5) {
        OSReport("M421:Camera No. Error!!\n");
        return NULL;
    }
    return &cameraWorkArray[cameraIndex];
}

M421DllCameraStruct *GetRenderCameraByIndex(u32 cameraIndex)
{
    M421DllCameraStruct *cameraWorkArray = cameraManagerObject->data;
    if (cameraIndex >= 5) {
        OSReport("M421:Camera No. Error!!\n");
        return NULL;
    }
    return &cameraWorkArray[cameraIndex + 5];
}

void SetModelCameraForViewFlag(s32 modelId, s32 requestedViewFlags)
{
    M421DllCameraStruct *cameraWork;
    s32 cameraIndex;
    s32 cameraMask;
    M421DllCameraStruct *secondaryCameraWork;
    M421DllCameraStruct *cameraWorkArray;

    cameraMask = 0;
    if (modelId >= 0) {
        cameraWorkArray = cameraManagerObject->data;
        secondaryCameraWork = &cameraWorkArray[5];
        for (cameraWork = secondaryCameraWork, cameraIndex = 0; cameraIndex < 5; cameraIndex++, cameraWork++) {
            if ((cameraWork->viewFlags & requestedViewFlags) != 0) {
                cameraMask |= cameraWork->cameraId;
            }
        }
        Hu3DModelCameraSet(modelId, cameraMask);
    }
}

s32 GetMinigameFlowState(void)
{
    return ((M421DllWork *)minigameObject->data)->flowState;
}

u32 GetCameraMode(void)
{
    return ((M421DllWork *)minigameObject->data)->cameraMode;
}

u32 GetResultState(void)
{
    return ((M421DllWork *)minigameObject->data)->resultState;
}

u32 GetExitState(void)
{
    return ((M421DllWork *)minigameObject->data)->exitState;
}

s32 SetPlayerResultFlag(s32 playerIndex)
{
    M421DllWork *work = minigameObject->data;

    if (work->resultMode != 2) {
        return 0;
    }
    work->playerResultFlags |= (1 << ((playerIndex & 3) * 4));
    if (((work->playerResultFlags & 0x1110) == 0x1110) && (work->resultMode == 2)) {
        work->resultMode = 0;
    }
    return 1;
}

s32 GetWinnerPlayerId(s32 resultSlot)
{
    M421DllWork *work = minigameObject->data;
    return work->winnerPlayerIds[resultSlot];
}

void SetPlayerStatusAndOrder(s32 playerIndex, s32 playerStatus, s32 playerOrder)
{
    M421DllWork *work = minigameObject->data;
    playerIndex &= 3;
    work->playerStatus[playerIndex] = playerStatus;
    work->playerOrder[playerIndex] = playerOrder;
}

void SetPlayerReadyFlag(s32 playerIndex)
{
    ((M421DllWork *)minigameObject->data)->playerReadyFlags |= (1 << ((playerIndex & 7) * 4));
}

void SetPlayerConfirmFlag(s32 playerIndex)
{
    ((M421DllWork *)minigameObject->data)->playerConfirmFlags |= (1 << ((playerIndex & 3) * 4));
}

void SetPlayerExitFlag(s32 playerIndex)
{
    ((M421DllWork *)minigameObject->data)->playerExitFlags |= (1 << ((playerIndex & 3) * 4));
}

s32 IsResultsSequenceFinished(void)
{
    return MGSeqStatGet(introSequenceId) == 0;
}

void SetCameraLookAt(Vec *cameraPosition, Vec *cameraTarget)
{
    Vec viewDirection;
    float horizontalDistance;
    float verticalDistance;

    VECSubtract(cameraTarget, cameraPosition, &viewDirection);
    Center = *cameraTarget;
    CRot.x = atan2d(viewDirection.y, sqrtf((viewDirection.x * viewDirection.x) + (viewDirection.z * viewDirection.z)));
    CRot.y = atan2d(-viewDirection.x, -viewDirection.z);
    CRot.z = 0.0f;
    CZoom = VECMag(&viewDirection);
}

void SetCameraPositionAndTarget(float currentAngle, float targetAngle, float interpolation, float targetX, float targetY, float targetZ)
{
    Vec cameraPosition;
    Vec cameraTarget;
    Vec viewDirection;

    cameraPosition.x = currentAngle;
    cameraPosition.y = targetAngle;
    cameraPosition.z = interpolation;
    cameraTarget.x = targetX;
    cameraTarget.y = targetY;
    cameraTarget.z = targetZ;
    VECSubtract(&cameraTarget, &cameraPosition, &viewDirection);
    Center = cameraTarget;
    CRot.x = atan2d(viewDirection.y, sqrtf((viewDirection.x * viewDirection.x) + (viewDirection.z * viewDirection.z)));
    CRot.y = atan2d(-viewDirection.x, -viewDirection.z);
    CRot.z = 0.0f;
    CZoom = VECMag(&viewDirection);
}

float InterpolateAngle(float currentAngle, float targetAngle, float interpolation)
{
    float interpolatedAngle;
    float angleDifference;
    float normalizedAngle;

    angleDifference = fmod(targetAngle - currentAngle, 360.0);
    interpolatedAngle = angleDifference;
    if (interpolatedAngle < 0.0f) {
        interpolatedAngle += 360.0f;
    }
    if (interpolatedAngle > 180.0f) {
        interpolatedAngle -= 360.0f;
    }
    normalizedAngle = fmod(currentAngle + (interpolation * interpolatedAngle), 360.0);
    interpolatedAngle = normalizedAngle;
    if (interpolatedAngle < 0.0f) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

float GetShortestAngleDifference(float currentAngle, float targetAngle)
{
    float shortestAngle;
    float angleDifference;

    angleDifference = fmod(targetAngle - currentAngle, 360.0);
    shortestAngle = angleDifference;
    if (shortestAngle < 0.0f) {
        shortestAngle += 360.0f;
    }
    if (shortestAngle > 180.0f) {
        shortestAngle -= 360.0f;
    }
    return shortestAngle;
}
