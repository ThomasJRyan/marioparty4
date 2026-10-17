#include <game/audio.h>
#include "game/disp.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/wipe.h"
#include "ext_math.h"

#include "REL/m410Dll.h"
#include "version.h"
#include <string.h>

HUPROCESS *minigameProcess;
OMOBJ *minigameStateObject;
OMOBJ *outOfViewObject;
s16 activeResultSequence;
s32 minigameAudioState;
s16 globalLightId;
s16 resultSequenceId;
s16 introSequenceId;
s16 countdownSequenceId;
s32 goalSoundId;
s32 missSoundId;
s32 musicStreamId;
s32 ambientStreamId;

Vec globalLightPosition = { 100.0f, 800.0f, -100.0f };
Vec globalLightDirection = { 0.3f, -0.8f, 0.3f };
Vec rotatedLightAngles = { 10.0f, 45.0f, 3500.0f };
GXColor globalLightColor = { 255, 255, 255, 255 };
Vec shadowLightPosition = { 50.0f, 150000.0f, 50.0f };
Vec worldUpVector = { 0.0f, 1.0f, 0.0f };
Vec shadowLookAtPosition = { 0.0f, 0.0f, 0.0f };
Vec cameraPathControlPoints[] = {
    { -350.0f, 120.00001f, 0.0f },
    { -350.0f, 120.00001f, 0.0f },
    { 0.0f, 350.0f, -300.0f },
    { 0.0f, 350.0f, -300.0f },
    { 0.0f, 367.0f, 252.0f },
};
Vec cameraPathLookAtPoints[] = {
    { 20.0f, -25.0f, 300.0f },
    { 20.0f, -25.0f, 300.0f },
    { 15.0f, 15.0f, 1100.0f },
    { 15.0f, 15.0f, 1100.0f },
    { -11.2f, 0.0f, 592.0f },
};

s32 cameraPathDurations[] = {
    REFRESH_RATE,
    REFRESH_RATE * 3 / 2,
    REFRESH_RATE * 9 / 5,
    REFRESH_RATE / 2,
    0
};

void CreateGameStateObject(OMOBJ *object);
void UpdateGameStateObject(OMOBJ *object);
void UpdateMinigameController(OMOBJ *object);
void UpdateMinigameExitWipe(OMOBJ *object);
void CleanupMinigameState(OMOBJ *object);
void UpdateMinigameIntro(OMOBJ *object);
void UpdateMinigameCountdown(OMOBJ *object);
void UpdateMinigameResults(OMOBJ *object);
OMOBJFUNC CreateMinigameScene(HUPROCESS *objman, OMOBJ *object);
void UpdateMinigameCamera(OMOBJ *object);
OMOBJFUNC CreateMinigamePlayerScene(HUPROCESS *objman, OMOBJ *object);
void UpdateMinigameEndSequence(OMOBJ *object);

void ObjectSetup(void)
{
    Mtx temporaryTransform;
    Vec temporaryPosition;
    Vec temporaryPosition2;
    HUPROCESS *objectManager;
    HU3DLIGHT *light;

    HuAudSndGrpSet(35);
    musicStreamId = ambientStreamId = -1;
    goalSoundId = missSoundId = -1;
    activeResultSequence = -1;
    shadowLightPosition.x = 200.0f;
    shadowLightPosition.y = 1200.0f;
    shadowLightPosition.z = 160.0f;
    worldUpVector.x = 0.0f;
    worldUpVector.y = 1.0f;
    worldUpVector.z = 0.0f;
    shadowLookAtPosition.x = 0.0f;
    shadowLookAtPosition.y = 0.0f;
    shadowLookAtPosition.z = 0.0f;
    Hu3DShadowCreate(45.0f, 20.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.625f);
    Hu3DShadowPosSet(&shadowLightPosition, &worldUpVector, &shadowLookAtPosition);
    globalLightId = Hu3DGLightCreateV(&globalLightPosition, &globalLightDirection, &globalLightColor);
    Hu3DGLightInfinitytSet(globalLightId);
    light = &Hu3DGlobalLight[globalLightId];
    light->type = light->type | 0x8000;
    rotatedLightAngles.x = 34.75f;
    rotatedLightAngles.y = -24.75f;
    rotatedLightAngles.z = 3500.0f;
    temporaryPosition.x = temporaryPosition.y = temporaryPosition.z = 0.0f;
    temporaryPosition2.x = temporaryPosition2.z = 0.0f;
    temporaryPosition2.y = rotatedLightAngles.z;
    mtxRot(temporaryTransform, rotatedLightAngles.x, rotatedLightAngles.y, 0.0f);
    MTXMultVec(temporaryTransform, &temporaryPosition2, &temporaryPosition2);
    Hu3DGLightPosAimSetV(globalLightId, &temporaryPosition2, &temporaryPosition);
    Hu3DShadowPosSet(&temporaryPosition2, &worldUpVector, &temporaryPosition);
    resultSequenceId = introSequenceId = countdownSequenceId = -1;
    objectManager = omInitObjMan(50, 8192);
    minigameProcess = objectManager;
    omGameSysInit(objectManager);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, -1.0f, 5.0f, 5000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    outOfViewObject = omAddObjEx(objectManager, 32730, 0, 0, -1, omOutView);
    minigameStateObject = omAddObjEx(objectManager, 10, 0, 0, -1, CreateGameStateObject);
    InitializeStageObjects(objectManager);
    InitializeGameObjects(objectManager);
    InitializePlayers(objectManager);
}

void CreateGameStateObject(OMOBJ *object)
{
    MinigameStateWork *minigameState;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(MinigameStateWork), HU_MEMNUM_OVL);
    minigameState = object->data;
    memset(minigameState, 0, sizeof(MinigameStateWork));
    minigameState->sequenceState = 0;
    minigameState->elapsedFrames = 0;
    minigameState->minigameStateFlags = 0;
    minigameState->introTimer = 0;
    minigameState->resultState = 0;
    minigameState->resultTimer = 0;
    minigameState->resultPhase = 0;
    minigameState->resultPhaseTimer = 0;
    minigameState->transitionBlocked = 0;
    minigameState->playerFlagMask = 0;
    minigameState->playerControlFlags = 0;
    minigameState->playerSequenceFlags = 0;
    object->objFunc = UpdateMinigameController;
}

void IncrementGameStateTimers(OMOBJ *object)
{
    MinigameStateWork *minigameState;

    minigameState = object->data;
    minigameState->elapsedFrames++;
    minigameState->introTimer++;
    minigameState->resultTimer++;
    minigameState->resultPhaseTimer++;
    if (omSysExitReq && minigameState->sequenceState != 5) {
        minigameState->sequenceState = 4;
        UpdateGameStateObject(object);
    }
}

void UpdateGameStateObject(OMOBJ *object)
{
    MinigameStateWork *minigameState;

    minigameState = object->data;

    if (minigameState->transitionBlocked) {
        return;
    }
    switch (minigameState->sequenceState) {
        case 0:
            WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
            Hu3DCameraPerspectiveSet(1, 41.5f, 5.0f, 5000.0f, 1.2f);
            minigameState->sequenceState = 1;
            minigameState->introTimer = 0;
            object->objFunc = CreateMinigameScene(minigameProcess, object);
            break;
        case 1:
            minigameState->sequenceState = 2;
            minigameState->resultTimer = 0;
            object->objFunc = UpdateMinigameIntro;
            break;
        case 2:
            minigameState->sequenceState = 3;
            minigameState->resultPhaseTimer = 0;
            object->objFunc = CreateMinigamePlayerScene(minigameProcess, object);
            break;
        case 3:
        case 4:
            minigameState->transitionBlocked = 1;
            minigameState->sequenceState = 5;
            object->objFunc = UpdateMinigameExitWipe;
            break;
        case 5:
        default:
            minigameState->sequenceState = 5;
            object->objFunc = CleanupMinigameState;
            break;
    }
    minigameState->elapsedFrames = 0;
}

void UpdateMinigameController(OMOBJ *object)
{
    IncrementGameStateTimers(object);
    UpdateGameStateObject(object);
}

void UpdateMinigameExitWipe(OMOBJ *object)
{
    MinigameStateWork *minigameState;

    minigameState = object->data;
    IncrementGameStateTimers(object);
    if (!WipeStatGet()) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        minigameState->transitionBlocked = 0;
        UpdateGameStateObject(object);
    }
}

void CleanupMinigameState(OMOBJ *object)
{
    IncrementGameStateTimers(object);
    if (!WipeStatGet()) {
        CleanupPlayers();
        SetGameObjectsHidden();
        InitializeStageState();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void UpdateMinigameIntro(OMOBJ *object)
{
    void *stateValue;
    MinigameStateWork *minigameState;
    void *stateValue2;

    minigameState = object->data;

    IncrementGameStateTimers(object);
    switch (minigameState->resultState) {
        case 0:
            minigameState->resultSequenceParameter = 30;
            minigameState->countdownTimer = REFRESH_RATE;
            resultSequenceId = MGSeqCreate(3, 0);
            MGSeqPosSet(resultSequenceId, 320.0f, 240.0f);
            CRot.x = -11.2f;
            CRot.y = CRot.z = 0.0f;
            Center.x = 0.0f;
            Center.y = 367.0f;
            Center.z = 252.0f;
            CZoom = 592.0f;
            minigameState->resultState = 1;
            minigameState->resultTimer = 0;
            break;

        case 1:
            if (musicStreamId < 0 && MGSeqStatGet(resultSequenceId) & 16) {
                musicStreamId = HuAudSeqPlay(71);
            }
            if (!MGSeqStatGet(resultSequenceId) && !minigameState->transitionBlocked) {
                activeResultSequence = MGSeqCreate(1, minigameState->resultSequenceParameter, -1, -1);
                minigameState->resultState = 2;
                minigameState->resultTimer = 0;
                object->objFunc = UpdateMinigameCountdown;
            }
            break;

        default:
            break;
    }
}

void UpdateMinigameCountdown(OMOBJ *object)
{
    MinigameStateWork *minigameState;
    s32 stateValue;

    minigameState = object->data;
    stateValue = 0;
    IncrementGameStateTimers(object);

    if (!--minigameState->countdownTimer) {
        minigameState->countdownTimer = REFRESH_RATE;
        minigameState->resultSequenceParameter--;
        if (!minigameState->resultSequenceParameter) {
            stateValue = 1;
        }
    }
    if (stateValue) {
        introSequenceId = MGSeqCreate(3, 1);
        MGSeqPosSet(introSequenceId, 320.0f, 240.0f);
        HuAudSeqFadeOut(musicStreamId, 100);
        minigameState->resultState = 3;
        minigameState->resultTimer = 0;
        if (minigameState->transitionBlocked == 0) {
            object->objFunc = UpdateMinigameResults;
        }
    }
    MGSeqParamSet(activeResultSequence, 1, minigameState->resultSequenceParameter);
}

void UpdateMinigameResults(OMOBJ *object)
{
    MinigameStateWork *minigameState;

    minigameState = object->data;
    if (activeResultSequence >= 0) {
        MGSeqParamSet(activeResultSequence, 2, -1);
        activeResultSequence = -1;
    }
    IncrementGameStateTimers(object);

    if (!MGSeqStatGet(introSequenceId) && (minigameState->playerControlFlags + 0xFFFF0000 == 0x1111)) { // -4294897391
        minigameState->resultState = 4;
        minigameState->resultTimer = 0;
        UpdateGameStateObject(object);
    }
}

OMOBJFUNC CreateMinigameScene(HUPROCESS *objman, OMOBJ *object)
{
    CRot.x = -80.5f;
    CRot.y = CRot.z = 0.0f;
    Center.x = 0.0f;
    Center.y = 0.0f;
    Center.z = 0.0f;
    CZoom = 1000.0f;
    object->work[0] = object->work[1] = 0;
    return UpdateMinigameCamera;
}

void UpdateMinigameCamera(OMOBJ *object)
{
    Vec temporaryPosition;
    float motionValue;
    MinigameStateWork *minigameState;

    minigameState = object->data;
    IncrementGameStateTimers(object);
    minigameState->minigameStateFlags = 1;
    object->work[0]++;

    if (!object->work[1] && object->work[0] > REFRESH_RATE_F / 5.0f) {
        if (goalSoundId < 0) {
            goalSoundId = HuAudFXPlay(1382);
        }
        if (missSoundId < 0) {
            missSoundId = HuAudFXPlay(1383);
        }
    }
    motionValue = (float)object->work[0] / cameraPathDurations[object->work[1]];

    VECSubtract(&cameraPathControlPoints[object->work[1] + 1], &cameraPathControlPoints[object->work[1]], &temporaryPosition);
    VECScale(&temporaryPosition, &temporaryPosition, motionValue);
    VECAdd(&cameraPathControlPoints[object->work[1]], &temporaryPosition, &temporaryPosition);

    Center.x = temporaryPosition.x;
    Center.y = temporaryPosition.y;
    Center.z = temporaryPosition.z;

    VECSubtract(&cameraPathLookAtPoints[object->work[1] + 1], &cameraPathLookAtPoints[object->work[1]], &temporaryPosition);
    VECScale(&temporaryPosition, &temporaryPosition, motionValue);
    VECAdd(&cameraPathLookAtPoints[object->work[1]], &temporaryPosition, &temporaryPosition);

    CRot.x = temporaryPosition.x;
    CRot.y = temporaryPosition.y;
    CRot.z = 0.0f;
    CZoom = temporaryPosition.z;

    if (motionValue >= 1.0f) {
        object->work[0] = 0;
        object->work[1] += 1;
        if (!cameraPathDurations[object->work[1]]) {
            minigameState->minigameStateFlags = 2;
            UpdateGameStateObject(object);
        }
    }
}

OMOBJFUNC CreateMinigamePlayerScene(HUPROCESS *objman, OMOBJ *object)
{
    u32 temporaryValue = object->work[4];
    return UpdateMinigameEndSequence;
}

void UpdateMinigameEndSequence(OMOBJ *object)
{
    s32 temporaryValue[5];
    MinigameStateWork *minigameState;
    s32 stateValue;
    u32 stateValue2;
    s32 loopIndex;

    minigameState = object->data;
    IncrementGameStateTimers(object);

    minigameState->resultPhase = 1;
    if (minigameState->resultPhaseTimer < REFRESH_RATE_F) {
#if VERSION_PAL
        Center.y -= 100.0f / REFRESH_RATE_F;
#else
        Center.y -= 100.0f / REFRESH_RATE_OFF_BY_1;
#endif
    }
    if (countdownSequenceId < 0) {
        for (loopIndex = 0, stateValue2 = 1, stateValue = 0; stateValue < 4; stateValue2 *= 2, stateValue++) {
            temporaryValue[stateValue] = -1;
            if (minigameState->playerFlagMask & stateValue2) {
                temporaryValue[loopIndex++] = GWPlayerCfg[stateValue].character;
                GWPlayerCoinWinAdd(stateValue, 10);
            }
        }
        if (temporaryValue[0] == -1) {
            countdownSequenceId = MGSeqCreate(3, 2);
            if (ambientStreamId >= 0) {
                return;
            }
            ambientStreamId = HuAudSStreamPlay(4);
            return;
        }
        countdownSequenceId = MGSeqCreate(5, 3, temporaryValue[0], temporaryValue[1], temporaryValue[2], temporaryValue[3]);
        if (ambientStreamId >= 0) {
            return;
        }
        ambientStreamId = HuAudSStreamPlay(1);
        return;
    }
    if (!MGSeqStatGet(countdownSequenceId)) {
        if (minigameState->resultPhaseTimer >= 3.5f * REFRESH_RATE_F) {
            minigameState->resultPhase = 2;
            UpdateGameStateObject(object);
        }
    }
}

s32 GetMinigameState(void)
{
    return ((MinigameStateWork *)minigameStateObject->data)->sequenceState;
}

u32 GetMinigamePhase(void)
{
    return ((MinigameStateWork *)minigameStateObject->data)->minigameStateFlags;
}

u32 GetMinigameSubstate(void)
{
    return ((MinigameStateWork *)minigameStateObject->data)->resultState;
}

u32 GetMinigameFlags(void)
{
    return ((MinigameStateWork *)minigameStateObject->data)->resultPhase;
}

void SetMinigameFlags(s32 flags)
{
    ((MinigameStateWork *)minigameStateObject->data)->playerFlagMask = flags;
}

u32 GetMinigameControlFlags(void)
{
    return ((MinigameStateWork *)minigameStateObject->data)->playerFlagMask;
}

void SetMinigameControlFlag(s32 playerIndex)
{
    ((MinigameStateWork *)minigameStateObject->data)->playerControlFlags |= (1 << ((playerIndex & 7) * 4));
}

void SetMinigamePlayerFlag(s32 playerIndex)
{
    ((MinigameStateWork *)minigameStateObject->data)->playerSequenceFlags |= (1 << ((playerIndex & 3) * 4));
}

s32 IsMinigameSequenceDone(void)
{
    return MGSeqStatGet(introSequenceId) == 0;
}

void SetCameraLookDirection(Vec *position, Vec *velocity)
{
    Vec temporaryPosition;
    float motionValue;
    float motionValue2;
    float motionValue3;

    temporaryPosition.x = velocity->x - position->x;
    temporaryPosition.y = velocity->y - position->y;
    temporaryPosition.z = velocity->z - position->z;

    Center.x = velocity->x;
    Center.y = velocity->y;
    Center.z = velocity->z;

    CRot.x = atan2d(temporaryPosition.y, VECMagXZ(&temporaryPosition));
    CRot.y = atan2d(-temporaryPosition.x, -temporaryPosition.z);
    CRot.z = 0.0f;

    CZoom = VECMagPoint(temporaryPosition.x, temporaryPosition.y, temporaryPosition.z);
}

void SetCameraTarget(float startAngle, float targetAngle, float angleStep, float radius, float duration, float height)
{
    Vec temporaryPosition;
    Vec temporaryPosition2;
    Vec temporaryPosition3;

    temporaryPosition.x = startAngle;
    temporaryPosition.y = targetAngle;
    temporaryPosition.z = angleStep;

    temporaryPosition2.x = radius;
    temporaryPosition2.y = duration;
    temporaryPosition2.z = height;

    temporaryPosition3.x = temporaryPosition2.x - temporaryPosition.x;
    temporaryPosition3.y = temporaryPosition2.y - temporaryPosition.y;
    temporaryPosition3.z = temporaryPosition2.z - temporaryPosition.z;

    Center.x = temporaryPosition2.x;
    Center.y = temporaryPosition2.y;
    Center.z = temporaryPosition2.z;

    CRot.x = atan2d(temporaryPosition3.y, VECMagXZ(&temporaryPosition3));
    CRot.y = atan2d(-temporaryPosition3.x, -temporaryPosition3.z);
    CRot.z = 0.0f;

    CZoom = VECMagPoint(temporaryPosition3.x, temporaryPosition3.y, temporaryPosition3.z);
}

float ApproachAngle(float currentAngle, float targetAngle, float stepAngle)
{
    float motionValue;
    float motionValue2;
    float motionValue3;

    motionValue2 = fmod(targetAngle - currentAngle, 360.0);
    motionValue = motionValue2;
    if (motionValue < 0.0f) {
        motionValue += 360.0f;
    }
    if (motionValue > 180.0f) {
        motionValue -= 360.0f;
    }
    motionValue3 = fmod(currentAngle + (stepAngle * motionValue), 360.0);
    motionValue = motionValue3;
    if (motionValue < 0.0f) {
        motionValue += 360.0f;
    }
    return motionValue;
}

float AngleDifference(float firstAngle, float secondAngle)
{
    float motionValue;
    float motionValue2;

    motionValue2 = fmod(secondAngle - firstAngle, 360.0);
    motionValue = motionValue2;
    if (motionValue < 0.0f) {
        motionValue += 360.0f;
    }
    if (motionValue > 180.0f) {
        motionValue -= 360.0f;
    }
    return motionValue;
}
