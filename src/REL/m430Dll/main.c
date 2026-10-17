#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/flag.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include "string.h"

#include "REL/m430Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#endif

typedef struct M430DllMainWork {
    u32 sceneState;
    s32 stateFrame;
    s32 sequenceComplete;
    u32 introPhase;
    s32 stateElapsedFrames;
    u32 roundState;
    s32 roundElapsedFrames;
    s32 countdownValue;
    s32 countdownFrame;
    u32 resultState;
    u32 resultElapsedFrames;
    s32 minigameParameter;
    u32 playerInputFlags;
    s32 playerActionFlags;
} M430DllMainWork; /* size = 0x38 */

void UpdateIntroCameraState(OMOBJ *object);
void UpdateMinigameResultState(OMOBJ *object);
void UpdateRoundCountdownState(OMOBJ *object);
void UpdateRoundPlayState(OMOBJ *object);
void UpdateRoundResultState(OMOBJ *object);
OMOBJFUNC BeginMinigameSequence(HUPROCESS *process, OMOBJ *object);
void UpdateMinigameSequence(OMOBJ *object);
OMOBJFUNC BeginMinigameFailureSequence(HUPROCESS *cameraTarget, OMOBJ *object);
void UpdateMinigameFailureSequence(OMOBJ *object);
s32 IsMinigameActive(void);
void CreateMinigameController(OMOBJ *object);
void AdvanceMinigameSceneState(OMOBJ *playerIndex);
void MainControllerCallback(OMOBJ *object);

Vec globalLightPosition = { 100.0f, 800.0f, -100.0f };
Vec globalLightDirection = { 0.3f, -0.8f, 0.3f };
Vec lightAimRotation = { 10.0f, 45.0f, 3500.0f };
GXColor globalLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };
Vec shadowCameraPosition = { 50.0f, 150000.0f, 50.0f };
Vec worldUpVector = { 0.0f, 1.0f, 0.0f };
Vec shadowLookAtTarget = { 0.0f, 0.0f, 0.0f };

HUPROCESS *minigameProcess;
OMOBJ *minigameControllerObject;
OMOBJ *multiviewCameraObject;
s16 countdownSequenceId;
char unusedMainStateBytes[4];
s16 globalLightId;
s16 roundSequenceId;
s16 resultSequenceId;
s16 unusedSequenceId;
s32 minigameActive;
s32 sceneTransitionActive;
s32 continueScene;
s16 countdownSpriteId;
#if VERSION_PAL
s16 palCountdownSpriteId;
#endif
s32 minigameSoundId;
s32 failureSoundId;
s32 roundSoundId;
s32 resultSoundId;

#if VERSION_PAL
#define CAMERA_Y 0x10
#define CAMERA_H 0x1D0
#else
#define CAMERA_Y 0x0
#define CAMERA_H 0x1E0
#endif

void ObjectSetup(void)
{
    Mtx lightRotationMatrix;
    Vec lightTarget;
    Vec lightPosition;
    HUPROCESS *objectManager;
    HU3DLIGHT *globalLight;

    HuAudSndGrpSet(0x37);
    minigameSoundId = failureSoundId = -1;
    roundSoundId = resultSoundId = -1;
    countdownSequenceId = -1;
    continueScene = 1;
    shadowCameraPosition.x = 200.0f;
    shadowCameraPosition.y = 1200.0f;
    shadowCameraPosition.z = 160.0f;
    worldUpVector.x = 0.0f;
    worldUpVector.y = 1.0f;
    worldUpVector.z = 0.0f;
    shadowLookAtTarget.x = 0.0f;
    shadowLookAtTarget.y = 0.0f;
    shadowLookAtTarget.z = 0.0f;
    Hu3DShadowCreate(30.0f, 20.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.625f);
    Hu3DShadowPosSet(&shadowCameraPosition, &worldUpVector, &shadowLookAtTarget);
    globalLightId = Hu3DGLightCreateV(&globalLightPosition, &globalLightDirection, &globalLightColor);
    Hu3DGLightInfinitytSet(globalLightId);
    globalLight = &Hu3DGlobalLight[globalLightId];
    globalLight->type |= 0x8000;
    lightAimRotation.x = 34.75f;
    lightAimRotation.y = -24.75f;
    lightAimRotation.z = 3500.0f;
    lightTarget.x = lightTarget.y = lightTarget.z = 0.0f;
    lightPosition.x = lightPosition.z = 0.0f;
    lightPosition.y = lightAimRotation.z;
    mtxRot(lightRotationMatrix, lightAimRotation.x, lightAimRotation.y, 0.0f);
    MTXMultVec(lightRotationMatrix, &lightPosition, &lightPosition);
    Hu3DGLightPosAimSetV(globalLightId, &lightPosition, &lightTarget);
    Hu3DShadowPosSet(&lightPosition, &worldUpVector, &lightTarget);
    roundSequenceId = resultSequenceId = unusedSequenceId = -1;
    minigameActive = 1;
    sceneTransitionActive = 1;
    objectManager = omInitObjMan(0x32, 0x2000);
    minigameProcess = objectManager;
    omGameSysInit(objectManager);
    Hu3DCameraCreate(3);
    Hu3DCameraPerspectiveSet(1, -1.0f, 5.0f, 25000.0f, 0.6f);
    Hu3DCameraPerspectiveSet(2, -1.0f, 5.0f, 25000.0f, 0.6f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 320.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraViewportSet(2, 320.0f, 0.0f, 320.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, CAMERA_Y, 0x140, CAMERA_H);
    Hu3DCameraScissorSet(2, 0x140, CAMERA_Y, 0x140, CAMERA_H);
    multiviewCameraObject = omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutViewMulti);
    multiviewCameraObject->work[0] = 2;
    minigameControllerObject = omAddObjEx(objectManager, 0xA, 0, 0, -1, CreateMinigameController);
    InitializeWaterAssets(objectManager);
    InitializePlayers(objectManager);
    countdownSpriteId = espEntry(DATA_MAKE_NUM(DATADIR_M430, 0x1B), 0x64, 0);
    espPosSet(countdownSpriteId, 288.0, 240.0);
    espScaleSet(countdownSpriteId, 0.22499999403953552, 60.0);
    espDispOff(countdownSpriteId);
#if VERSION_PAL
    palCountdownSpriteId = espEntry(DATA_MAKE_NUM(DATADIR_M430, 0x1B), 100, 0);
    espPosSet(palCountdownSpriteId, 288.0, 475.20001220703125);
    espScaleSet(palCountdownSpriteId, 72.0, 4.0);
#endif
}

void CreateMinigameController(OMOBJ *object)
{
    M430DllMainWork *work;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M430DllMainWork), HU_MEMNUM_OVL);
    work = object->data;
    memset(work, 0, sizeof(M430DllMainWork));
    work->sceneState = 0;
    work->stateFrame = 0;
    work->introPhase = 0;
    work->stateElapsedFrames = 0;
    work->roundState = 0;
    work->roundElapsedFrames = 0;
    work->resultState = 0;
    work->resultElapsedFrames = 0;
    work->sequenceComplete = 0;
    work->minigameParameter = 0;
    work->playerInputFlags = 0;
    work->playerActionFlags = 0;
    object->objFunc = MainControllerCallback;
}

void UpdateMinigameController(OMOBJ *object)
{
    M430DllMainWork *work = object->data;
    work->stateFrame++;
    work->stateElapsedFrames++;
    work->roundElapsedFrames++;
    work->resultElapsedFrames++;
    if ((omSysExitReq != 0) && (work->sceneState != 5)) {
        work->sceneState = 4;
        AdvanceMinigameSceneState(object);
    }
    if (sceneTransitionActive == 0) {
        if (IsMinigameActive() != 0) {
            if ((Hu3DCamera->fov < 0.0f) || (Hu3DCamera[1].fov < 0.0f)) {
                Hu3DCameraPerspectiveSet(1, 41.5f, 5.0f, 25000.0f, 0.6f);
                Hu3DCameraPerspectiveSet(2, 41.5f, 5.0f, 25000.0f, 0.6f);
                Hu3DCameraViewportSet(1, 0.0f, 0.0f, 320.0f, 480.0f, 0.0f, 1.0f);
                Hu3DCameraViewportSet(2, 320.0f, 0.0f, 320.0f, 480.0f, 0.0f, 1.0f);
                Hu3DCameraScissorSet(1, 0, CAMERA_Y, 0x140, CAMERA_H);
                Hu3DCameraScissorSet(2, 0x140, CAMERA_Y, 0x140, CAMERA_H);
            }
        }
        else if ((Hu3DCamera->fov < 0.0f) || (Hu3DCamera[1].fov >= 0.0f)) {
            Hu3DCamera[1].fov = -1.0f;
            Hu3DCameraPerspectiveSet(1, 41.5f, 5.0f, 25000.0f, 1.2f);
            Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
            Hu3DCameraScissorSet(1, 0, CAMERA_Y, 0x280, CAMERA_H);
        }
    }
}

void AdvanceMinigameSceneState(OMOBJ *object)
{
    M430DllMainWork *work = object->data;
    if (work->sequenceComplete == 0) {
        switch (work->sceneState) {
            case 0:
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
                sceneTransitionActive = 0;
                work->sceneState = 1;
                work->stateElapsedFrames = 0;
                object->objFunc = BeginMinigameSequence(minigameProcess, object);
                break;
            case 1:
                work->sceneState = 2;
                work->roundElapsedFrames = 0;
                object->objFunc = UpdateRoundCountdownState;
                break;
            case 2:
                work->sceneState = 3;
                work->resultElapsedFrames = 0;
                object->objFunc = BeginMinigameFailureSequence(minigameProcess, object);
                break;
            case 3:
            case 4:
                work->sequenceComplete = 1;
                work->sceneState = 5;
                object->objFunc = UpdateIntroCameraState;
                break;
            case 5:
            default:
                work->sceneState = 5;
                object->objFunc = UpdateMinigameResultState;
                break;
        }
        work->stateFrame = 0;
    }
}

void MainControllerCallback(OMOBJ *object)
{
    UpdateMinigameController(object);
    AdvanceMinigameSceneState(object);
}

void UpdateIntroCameraState(OMOBJ *object)
{
    M430DllMainWork *work = object->data;
    UpdateMinigameController(object);

    if (WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        continueScene = 1;
        work->sequenceComplete = 0;
        AdvanceMinigameSceneState(object);
    }
}

void UpdateMinigameResultState(OMOBJ *object)
{
    UpdateMinigameController(object);
    if (WipeStatGet() == 0) {
        ReleasePlayerAssets();
        ReleaseWaterAssets();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void UpdateRoundCountdownState(OMOBJ *object)
{
    M430DllMainWork *work = object->data;
    UpdateMinigameController(object);
    switch (work->roundState) {
        case 0:
            minigameActive = 1;
            WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
            continueScene = 0;
            espDispOn(countdownSpriteId);
            work->countdownValue = 0x2D;
            work->countdownFrame = REFRESH_RATE;
            CenterM->x = -450.0f;
            CenterM[1].x = 450.0f;
            CenterM->y = CenterM[1].y = 300.0f;
            CenterM->z = CenterM[1].z = 950.0f;
            CRotM->x = CRotM[1].x = -5.0f;
            CRotM->y = CRotM[1].y = 0.0f;
            CZoomM[0] = CZoomM[1] = 700.0f;
            work->roundState = 1;
            work->roundElapsedFrames = 0;
            object->work[0] = 0;
            return;
        case 1:
            if (object->work[0] == 0) {
                if (WipeStatGet() == 0) {
                    roundSequenceId = MGSeqCreate(3, 0);
                    MGSeqPosSet(roundSequenceId, 320.0f, 240.0f);
                    object->work[0]++;
                }
            }
            else {
                if ((roundSoundId < 0) && ((MGSeqStatGet(roundSequenceId) & 0x10) != 0)) {
                    roundSoundId = HuAudSeqPlay(0x4A);
                }
                if ((MGSeqStatGet(roundSequenceId) == 0) && (work->sequenceComplete == 0)) {
                    countdownSequenceId = MGSeqCreate(1, work->countdownValue, -1, -1);
                    work->roundState = 2;
                    work->roundElapsedFrames = 0;
                    object->objFunc = UpdateRoundPlayState;
                }
            }
            break;
    }
}

void UpdateRoundPlayState(OMOBJ *object)
{
    M430DllMainWork *work = object->data;
    s32 countdownExpired = 0;
    UpdateMinigameController(object);
    if (--work->countdownFrame == 0) {
        work->countdownFrame = REFRESH_RATE;
        work->countdownValue--;
        if (work->countdownValue <= 0) {
            countdownExpired = 1;
        }
    }
    if (countdownExpired != 0) {
        resultSequenceId = MGSeqCreate(3, 1);
        MGSeqPosSet(resultSequenceId, 320.0f, 240.0f);
        HuAudSeqFadeOut(roundSoundId, 0x64);
        object->work[1] = 0;
        work->roundState = 3;
        work->roundElapsedFrames = 0;
        if (work->sequenceComplete == 0) {
            object->objFunc = UpdateRoundResultState;
        }
    }
    MGSeqParamSet(countdownSequenceId, 1, work->countdownValue);
}

void UpdateRoundResultState(OMOBJ *object)
{
    M430DllMainWork *work = object->data;
    if (countdownSequenceId >= 0) {
        MGSeqParamSet(countdownSequenceId, 2, -1);
        countdownSequenceId = -1;
    }
    UpdateMinigameController(object);
    if (object->work[1] == 0) {
        if (MGSeqStatGet(resultSequenceId) == 0) {
            if (work->sequenceComplete == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
                continueScene = 1;
            }
            object->work[1]++;
            HuAudFXFadeOut(minigameSoundId, REFRESH_RATE / 2);
        }
    }
    else if ((WipeStatGet() == 0) && (work->playerInputFlags == 0x1111)) {
        work->roundState = 4;
        work->roundElapsedFrames = 0;
        AdvanceMinigameSceneState(object);
    }
}

OMOBJFUNC BeginMinigameSequence(HUPROCESS *process, OMOBJ *object)
{
    CenterM->x = CenterM[1].x = 0.0f;
    CenterM->y = CenterM[1].y = 300.0f;
    CenterM->z = CenterM[1].z = 950.0f;
    CRotM->x = CRotM[1].x = -5.0f;
    CRotM->y = CRotM[1].y = 0.0f;
    CZoomM[0] = CZoomM[1] = 700.0f;
    object->work[0] = object->work[1] = 0;
    minigameActive = 0;
    minigameSoundId = HuAudFXPlay(0x674);
    return UpdateMinigameSequence;
}

void UpdateMinigameSequence(OMOBJ *object)
{
    M430DllMainWork *work = object->data;
    UpdateMinigameController(object);
    work->introPhase = 1;
    object->work[0]++;
    switch (object->work[1]) {
        case 0:
            if (object->work[0] > REFRESH_RATE_F * 3) {
                if (work->sequenceComplete == 0) {
                    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
                    continueScene = 1;
                }
                object->work[1]++;
                object->work[0] = 0;
            }
            break;
        case 1:
            if (WipeStatGet() == 0) {
                object->work[0] = 0;
                work->introPhase = 2;
                AdvanceMinigameSceneState(object);
            }
            break;
    }
}

OMOBJFUNC BeginMinigameFailureSequence(HUPROCESS *playerIndex, OMOBJ *object)
{
    M430DllMainWork *work = object->data;
    s32 unusedSequenceFlagValue = 0;
    s32 unusedSequenceCountValue = 1;
    minigameActive = 0;
    espDispOff(countdownSpriteId);
    SetWaterSurfaceMoving(1);
    if (work->sequenceComplete == 0) {
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
        continueScene = 0;
    }
    CenterM->x = 0.0f;
    failureSoundId = HuAudFXPlay(0x679);
    return UpdateMinigameFailureSequence;
}

void UpdateMinigameFailureSequence(OMOBJ *object)
{
    M430DllMainWork *work = object->data;

    UpdateMinigameController(object);
    if (WipeStatGet() == 0) {
        work->resultState = 1;
        if (work->resultElapsedFrames >= REFRESH_RATE_F * 3.5f) {
            work->resultState = 2;
            AdvanceMinigameSceneState(object);
        }
    }
    else {
        work->resultElapsedFrames = 0;
    }
}

s32 IsMinigameActive(void)
{
    return minigameActive;
}

u32 GetSceneState(void)
{
    return omObjGetDataAs(minigameControllerObject, M430DllMainWork)->sceneState;
}

u32 GetIntroPhase(void)
{
    return omObjGetDataAs(minigameControllerObject, M430DllMainWork)->introPhase;
}

u32 GetRoundState(void)
{
    return omObjGetDataAs(minigameControllerObject, M430DllMainWork)->roundState;
}

u32 GetResultState(void)
{
    return omObjGetDataAs(minigameControllerObject, M430DllMainWork)->resultState;
}

void SetMinigameParameter(s32 playerIndex)
{
    omObjGetDataAs(minigameControllerObject, M430DllMainWork)->minigameParameter = playerIndex;
}

s32 GetMinigameParameter(void)
{
    return omObjGetDataAs(minigameControllerObject, M430DllMainWork)->minigameParameter;
}

void SetPlayerInputFlag(s32 playerIndex)
{

    omObjGetDataAs(minigameControllerObject, M430DllMainWork)->playerInputFlags |= (1 << ((playerIndex & 3) * 4));
}

void SetPlayerActionFlag(s32 playerIndex)
{
    omObjGetDataAs(minigameControllerObject, M430DllMainWork)->playerActionFlags |= (1 << ((playerIndex & 3) * 4));
}

float GetCountdownTime(void)
{
    M430DllMainWork *work = minigameControllerObject->data;
    float countdownSeconds = work->countdownValue - 1;
    if (countdownSeconds < 0.0f) {
        countdownSeconds = 0.0f;
    }
    countdownSeconds += REFRESH_FREQ * work->countdownFrame;
    return countdownSeconds;
}

void SetCameraFromLookAt(Vec *cameraPosition, Vec *cameraTarget)
{
    Vec cameraOffset;
    float unusedCameraScalarA;
    float unusedCameraScalarB;
    float unusedCameraScalarC;
    float unusedCameraScalarD;

    cameraOffset.x = cameraTarget->x - cameraPosition->x;
    cameraOffset.y = cameraTarget->y - cameraPosition->y;
    cameraOffset.z = cameraTarget->z - cameraPosition->z;
    Center.x = cameraTarget->x;
    Center.y = cameraTarget->y;
    Center.z = cameraTarget->z;
    CRot.x = atan2d(cameraOffset.y, sqrtf((cameraOffset.x * cameraOffset.x) + (cameraOffset.z * cameraOffset.z)));
    CRot.y = atan2d(-cameraOffset.x, -cameraOffset.z);
    CRot.z = 0.0f;
    CZoom = sqrtf((cameraOffset.z * cameraOffset.z) + ((cameraOffset.x * cameraOffset.x) + (cameraOffset.y * cameraOffset.y)));
}

void SetCameraPositionAndTarget(float cameraPositionX, float cameraPositionY, float cameraPositionZ, float cameraTargetX, float cameraTargetY, float cameraTargetZ)
{
    Vec cameraPosition;
    Vec cameraTarget;
    cameraPosition.x = cameraPositionX;
    cameraPosition.y = cameraPositionY;
    cameraPosition.z = cameraPositionZ;

    cameraTarget.x = cameraTargetX;
    cameraTarget.y = cameraTargetY;
    cameraTarget.z = cameraTargetZ;
    SetCameraFromLookAt(&cameraPosition, &cameraTarget);
}

float InterpolateAngle(float startAngle, float targetAngle, float blendFactor)
{
    float normalizedAngleDifference;
    float angleDifference;
    float interpolatedAngle;

    angleDifference = fmod(targetAngle - startAngle, 360.0);
    normalizedAngleDifference = angleDifference;
    if (normalizedAngleDifference < 0.0f) {
        normalizedAngleDifference += 360.0f;
    }
    if (normalizedAngleDifference > 180.0f) {
        normalizedAngleDifference -= 360.0f;
    }
    interpolatedAngle = fmod(startAngle + (blendFactor * normalizedAngleDifference), 360.0);
    normalizedAngleDifference = interpolatedAngle;
    if (normalizedAngleDifference < 0.0f) {
        normalizedAngleDifference += 360.0f;
    }
    return normalizedAngleDifference;
}

float GetShortestAngleDifference(float startAngle, float targetAngle)
{
    float normalizedAngleDifference;
    float angleDifference;

    angleDifference = fmod(targetAngle - startAngle, 360.0);
    normalizedAngleDifference = angleDifference;
    if (normalizedAngleDifference < 0.0f) {
        normalizedAngleDifference += 360.0f;
    }
    if (normalizedAngleDifference > 180.0f) {
        normalizedAngleDifference -= 360.0f;
    }
    return normalizedAngleDifference;
}
