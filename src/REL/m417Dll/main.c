#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/wipe.h"

#include "REL/m417Dll.h"
#include "version.h"
#include <string.h>

typedef struct MinigameRuntimeState {
    /* 0x00 */ u32 minigameState;
    /* 0x04 */ u32 elapsedFrames;
    /* 0x08 */ s32 exitRequested;
    /* 0x0C */ s32 winnerIndex;
    /* 0x10 */ s32 introTimer;
    /* 0x14 */ s32 sequenceState;
    /* 0x18 */ s32 sequenceTimer;
    /* 0x1C */ u32 sequenceFlags;
    /* 0x20 */ u32 timerFrame;
    /* 0x24 */ s32 resultState;
    /* 0x28 */ s32 resultTimer;
    /* 0x2C */ s32 playerResults[4];
    /* 0x3C */ s32 playerCoinResults[4];
    /* 0x4C */ s32 playerWinSequences[4];
    /* 0x5C */ u32 minigameMode;
    /* 0x60 */ u32 collectedItemFlags;
    /* 0x64 */ s32 playerScoreFlags;
} MinigameRuntimeState; /* size = 0x68 */

void InitializeMinigameObject(OMOBJ *object);
void MinigameObjectUpdate(OMOBJ *object);
void FinishMinigameTransition(OMOBJ *object);
void CompleteMinigameExit(OMOBJ *object);
void RunIntroSequence(OMOBJ *object);
void RunCountdownSequence(OMOBJ *object);
void RunResultSequence(OMOBJ *object);
s32 IsResultSequenceFinished(void);

Vec directionalLightPosition = { 100.0f, 800.0f, -100.0f };
Vec directionalLightColor = { 0.3f, -0.8f, 0.3f };
Vec cameraLightRotation = { 10.0f, 45.0f, 1000.0f };
GXColor directionalLightTint = { 255, 255, 255, 255 };
Vec shadowCameraPosition = { 50.0f, 150000.0f, 50.0f };
Vec shadowCameraUp = { 0.0f, 1.0f, 0.0f };
Vec shadowCameraTarget = { 0.0f, 0.0f, 0.0f };

HUPROCESS *minigameSequence;
OMOBJ *minigameObject;
OMOBJ *viewObject;
s16 resultSequenceId;
s16 introSequenceId;
s16 itemSequenceId;
s16 globalLightIds[4];
s32 backgroundSequenceId;
s32 streamedMusicId;

void ObjectSetup(void)
{
    Mtx lightRotationMatrix;
    Vec listenerPosition = { 0.0f, 0.0f, 0.0f };
    Vec listenerDirection = { 0.0f, 0.0f, -1.0f };
    Vec lightTarget;
    Vec lightPosition;

    HUPROCESS *objectManager;
    HU3DLIGHT *directionalLight;

    HuAudSndGrpSet(42);
    HuAudFXListnerSetEX(&listenerPosition, &listenerDirection, 10000.0f, 1000.0f, 0.0f, 300.0f, 300.0f);
    backgroundSequenceId = streamedMusicId = -1;
    resultSequenceId = -1;
    globalLightIds[0] = Hu3DGLightCreateV(&directionalLightPosition, &directionalLightColor, &directionalLightTint);
    Hu3DGLightInfinitytSet(globalLightIds[0]);
    directionalLight = &Hu3DGlobalLight[globalLightIds[0]];
    directionalLight->type |= 32768;
    cameraLightRotation.x = 20.0f;
    cameraLightRotation.y = 45.0f;
    cameraLightRotation.z = 1000.0f;
    lightTarget.x = lightTarget.y = lightTarget.z = 0.0f;
    lightPosition.x = lightPosition.z = 0.0f;
    lightPosition.y = cameraLightRotation.z;
    mtxRot(lightRotationMatrix, cameraLightRotation.x, cameraLightRotation.y, 0.0f);
    MTXMultVec(lightRotationMatrix, &lightPosition, &lightPosition);
    Hu3DGLightPosAimSetV(globalLightIds[0], &lightPosition, &lightTarget);
    shadowCameraPosition.x = 200.0f;
    shadowCameraPosition.y = 3000.0f;
    shadowCameraPosition.z = 100.0f;
    shadowCameraUp.x = 0.0f;
    shadowCameraUp.y = 1.0f;
    shadowCameraUp.z = 0.0f;
    shadowCameraTarget.x = 0.0f;
    shadowCameraTarget.y = 0.0f;
    shadowCameraTarget.z = 0.0f;
    Hu3DShadowCreate(30.0f, 20.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.625f);
    Hu3DShadowPosSet(&shadowCameraPosition, &shadowCameraUp, &shadowCameraTarget);
    objectManager = omInitObjMan(50, 8192);
    minigameSequence = objectManager;
    omGameSysInit(objectManager);
    introSequenceId = itemSequenceId = -1;
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, -1.0f, 5.0f, 5000.0f, 1.2f);
    viewObject = omAddObjEx(objectManager, 32730, 0, 0, -1, omOutView);
    minigameObject = omAddObjEx(objectManager, 10, 0, 0, -1, InitializeMinigameObject);
    CreateWaterObjects(objectManager);
    CreatePlayerObjects(objectManager);
}

void InitializeMinigameObject(OMOBJ *object)
{
    MinigameRuntimeState *minigameState;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, 0x68, 268435456);
    minigameState = object->data;
    memset(minigameState, 0, 0x68);
    minigameState->minigameState = 0;
    minigameState->elapsedFrames = 0;
    minigameState->winnerIndex = 0;
    minigameState->introTimer = 0;
    minigameState->sequenceState = 0;
    minigameState->sequenceTimer = 0;
    minigameState->resultState = 0;
    minigameState->resultTimer = 0;
    minigameState->exitRequested = 0;
    minigameState->minigameMode = 3;
    minigameState->playerWinSequences[0] = minigameState->playerWinSequences[1] = minigameState->playerWinSequences[2] = minigameState->playerWinSequences[3] = -1;
    minigameState->collectedItemFlags = 0;
    minigameState->playerScoreFlags = 0;
    object->objFunc = MinigameObjectUpdate;
}

void UpdateMinigameFrame(OMOBJ *object)
{
    Vec cameraPosition;
    Vec cameraDirection;

    MinigameRuntimeState *minigameState;

    minigameState = object->data;
    minigameState->elapsedFrames++;
    minigameState->introTimer++;
    minigameState->sequenceTimer++;
    minigameState->resultTimer++;
    cameraDirection.x = -(sind(CRot.y) * cosd(CRot.x));
    cameraDirection.y = -(-sind(CRot.x));
    cameraDirection.z = -(cosd(CRot.y) * cosd(CRot.x));
    cameraPosition.x = Center.x - (cameraDirection.x * CZoom);
    cameraPosition.y = Center.y - (cameraDirection.y * CZoom);
    cameraPosition.z = Center.z - (cameraDirection.z * CZoom);
    HuAudFXListnerUpdate(&cameraPosition, &cameraDirection);
    if (omSysExitReq && (minigameState->minigameState != 5)) {
        minigameState->minigameState = 4;
        AdvanceMinigameState(object);
    }
}

void AdvanceMinigameState(OMOBJ *object)
{
    MinigameRuntimeState *minigameState;

    minigameState = object->data;
    if (minigameState->exitRequested) {
        return;
    }
    switch (minigameState->minigameState) {
        case 0:
            WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
            Hu3DCameraPerspectiveSet(1, 35.0f, 5.0f, 5000.0f, 1.2f);
            minigameState->minigameState = 1;
            minigameState->introTimer = 0;
            object->objFunc = startIntroCameraSequence(minigameSequence);
            break;
        case 1:
            minigameState->minigameState = 2;
            minigameState->sequenceTimer = 0;
            object->objFunc = RunIntroSequence;
            break;
        case 2:
            if (streamedMusicId < 0) {
                streamedMusicId = HuAudSStreamPlay(1);
            }
            minigameState->minigameState = 3;
            minigameState->resultTimer = 0;
            object->objFunc = startResultsSequence(minigameSequence, object);
            break;
        case 3:
        case 4:
            minigameState->exitRequested = 1;
            minigameState->minigameState = 5;
            object->objFunc = FinishMinigameTransition;
            break;
        case 5:
        default:
            minigameState->minigameState = 5;
            object->objFunc = CompleteMinigameExit;
            break;
    }
    minigameState->elapsedFrames = 0;
}

void MinigameObjectUpdate(OMOBJ *object)
{
    UpdateMinigameFrame(object);
    AdvanceMinigameState(object);
}

void FinishMinigameTransition(OMOBJ *object)
{
    MinigameRuntimeState *minigameState;

    minigameState = object->data;

    UpdateMinigameFrame(object);
    if (!WipeStatGet()) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        minigameState->exitRequested = 0;
        AdvanceMinigameState(object);
    }
}

void CompleteMinigameExit(OMOBJ *object)
{
    UpdateMinigameFrame(object);
    if (!WipeStatGet()) {
        DestroyWaterObjects();
        CharModelKill(-1);
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void RunIntroSequence(OMOBJ *object)
{
    s16 introSequenceId; // ! - uninitialized

    MinigameRuntimeState *minigameState;

    minigameState = object->data;
    UpdateMinigameFrame(object);

    switch (minigameState->sequenceState) {
        case 0:
            minigameState->sequenceFlags = 30;
            minigameState->timerFrame = REFRESH_RATE;
            introSequenceId = MGSeqCreate(3, 0);
            MGSeqPosSet(introSequenceId, 320.0f, 240.0f);
            minigameState->sequenceState = 1;
            minigameState->sequenceTimer = 0;
            break;
        case 1:
            if (backgroundSequenceId < 0 && MGSeqStatGet(introSequenceId) & 16) {
                backgroundSequenceId = HuAudSeqPlay(72);
            }
            if (!MGSeqStatGet(introSequenceId) && !minigameState->exitRequested) {
                resultSequenceId = MGSeqCreate(1, minigameState->sequenceFlags, -1, -1);
                minigameState->sequenceState = 2;
                minigameState->sequenceTimer = 0;
                object->objFunc = RunCountdownSequence;
            }
            break;
        default:
            break;
    }
}

void RunCountdownSequence(OMOBJ *object)
{
    MinigameRuntimeState *minigameState;
    s32 sequenceFinished;

    minigameState = object->data;
    sequenceFinished = 0;
    UpdateMinigameFrame(object);

    if (--minigameState->timerFrame == 0) {
        minigameState->timerFrame = REFRESH_RATE;
        minigameState->sequenceFlags--;
        if (minigameState->sequenceFlags == 0) {
            sequenceFinished = 1;
        }
    }
    if (minigameState->minigameMode != 3) {
        sequenceFinished = 1;
    }
    if (sequenceFinished) {
        if (minigameState->minigameMode == 3) {
            minigameState->minigameMode = 1;
        }
        itemSequenceId = MGSeqCreate(3, 1);
        MGSeqPosSet(itemSequenceId, 320.0f, 240.0f);
        HuAudSeqFadeOut(backgroundSequenceId, 100);
        minigameState->sequenceState = 3;
        minigameState->sequenceTimer = 0;
        if (minigameState->exitRequested == 0) {
            object->objFunc = RunResultSequence;
        }
    }
    MGSeqParamSet(resultSequenceId, 1, minigameState->sequenceFlags);
}

void RunResultSequence(OMOBJ *object)
{
    MinigameRuntimeState *minigameState;

    minigameState = object->data;
    if (resultSequenceId >= 0) {
        MGSeqParamSet(resultSequenceId, 2, -1);
        resultSequenceId = -1;
    }
    UpdateMinigameFrame(object);
    if (IsResultSequenceFinished() && minigameState->collectedItemFlags == 4369) {
        minigameState->sequenceState = 4;
        minigameState->sequenceTimer = 0;
        AdvanceMinigameState(object);
    }
}

u32 GetMinigameState(void)
{
    return ((MinigameRuntimeState *)minigameObject->data)->minigameState;
}

s32 GetWinnerIndex(void)
{
    return ((MinigameRuntimeState *)minigameObject->data)->winnerIndex;
}

u32 GetSequenceState(void)
{
    return ((MinigameRuntimeState *)minigameObject->data)->sequenceState;
}

u32 GetResultState(void)
{
    return ((MinigameRuntimeState *)minigameObject->data)->resultState;
}

void SetMinigameMode(u32 mode)
{
    ((MinigameRuntimeState *)minigameObject->data)->minigameMode = mode;
}

s32 GetPlayerWinSequence(s32 playerIndex)
{
    MinigameRuntimeState *minigameState;

    minigameState = minigameObject->data;
    return minigameState->playerWinSequences[playerIndex];
}

void MarkPlayerItemCollected(s32 playerIndex)
{
    ((MinigameRuntimeState *)minigameObject->data)->collectedItemFlags |= (1 << ((playerIndex & 3) * 4));
}

void MarkPlayerScore(s32 playerIndex)
{
    ((MinigameRuntimeState *)minigameObject->data)->playerScoreFlags |= (1 << ((playerIndex & 3) * 4));
}

s32 IsResultSequenceFinished(void)
{
    return !MGSeqStatGet(itemSequenceId);
}

void SetPlayerResult(s32 playerIndex, s32 result, s32 coinResult)
{
    MinigameRuntimeState *minigameState;

    minigameState = minigameObject->data;
    playerIndex &= 3;
    minigameState->playerResults[playerIndex] = result;
    minigameState->playerCoinResults[playerIndex] = coinResult;
}

void SetCameraBetweenPoints(Vec *cameraStart, Vec *cameraTarget)
{
    Vec cameraDelta;

    Float horizontalDistanceSquared;
    Float distanceSquared;

    cameraDelta.x = cameraTarget->x - cameraStart->x;
    cameraDelta.y = cameraTarget->y - cameraStart->y;
    cameraDelta.z = cameraTarget->z - cameraStart->z;

    Center.x = cameraTarget->x;
    Center.y = cameraTarget->y;
    Center.z = cameraTarget->z;

    horizontalDistanceSquared = (cameraDelta.x * cameraDelta.x) + (cameraDelta.z * cameraDelta.z);
    CRot.x = atan2d(cameraDelta.y, sqrtf(horizontalDistanceSquared));
    CRot.y = atan2d(-cameraDelta.x, -cameraDelta.z);
    CRot.z = 0.0f;

    distanceSquared = (cameraDelta.z * cameraDelta.z) + ((cameraDelta.x * cameraDelta.x) + (cameraDelta.y * cameraDelta.y));
    CZoom = sqrtf(distanceSquared);
}

void SetCameraBetweenPositions(Float startX, Float startY, Float startZ, Float targetX, Float targetY, Float targetZ)
{
    Vec cameraStart;
    Vec cameraTarget;
    Vec cameraDelta;

    Float horizontalDistanceSquared;
    Float distanceSquared;

    cameraStart.x = startX;
    cameraStart.y = startY;
    cameraStart.z = startZ;

    cameraTarget.x = targetX;
    cameraTarget.y = targetY;
    cameraTarget.z = targetZ;

    cameraDelta.x = cameraTarget.x - cameraStart.x;
    cameraDelta.y = cameraTarget.y - cameraStart.y;
    cameraDelta.z = cameraTarget.z - cameraStart.z;

    Center.x = cameraTarget.x;
    Center.y = cameraTarget.y;
    Center.z = cameraTarget.z;

    horizontalDistanceSquared = (cameraDelta.x * cameraDelta.x) + (cameraDelta.z * cameraDelta.z);
    CRot.x = atan2d(cameraDelta.y, sqrtf(horizontalDistanceSquared));
    CRot.y = atan2d(-cameraDelta.x, -cameraDelta.z);
    CRot.z = 0.0f;

    distanceSquared = (cameraDelta.z * cameraDelta.z) + ((cameraDelta.x * cameraDelta.x) + (cameraDelta.y * cameraDelta.y));
    CZoom = sqrtf(distanceSquared);
}

Float InterpolateAngle(Float startAngle, Float targetAngle, Float interpolationFactor)
{
    Float normalizedAngleDifference;
    Float angleDifference;
    Float interpolatedAngle;

    angleDifference = fmod(targetAngle - startAngle, 360.0);
    normalizedAngleDifference = angleDifference;
    if (normalizedAngleDifference < 0.0f) {
        normalizedAngleDifference += 360.0f;
    }
    if (normalizedAngleDifference > 180.0f) {
        normalizedAngleDifference -= 360.0f;
    }
    interpolatedAngle = fmod(startAngle + (interpolationFactor * normalizedAngleDifference), 360.0);
    normalizedAngleDifference = interpolatedAngle;
    if (normalizedAngleDifference < 0.0f) {
        normalizedAngleDifference += 360.0f;
    }
    return normalizedAngleDifference;
}

Float AngleDifference(Float startAngle, Float targetAngle)
{
    Float normalizedAngleDifference;
    Float angleDifference;

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

void ClearVectorComponents(Float *unusedOutput, Float *xOutput, Float *yOutput, Float *zOutput)
{
    *xOutput = *yOutput = *zOutput = 0.0f;
    *xOutput = *yOutput = *zOutput = 0.0f;
}

void RotateVectorComponents(Float xRotation, Float yRotation, Float zRotation, Float *rotatedX, Float *rotatedY, Float *rotatedZ)
{
    Mtx rotationMatrix;
    Mtx axisRotationMatrix;

    if (xRotation != 0.0f) {
        MTXRotRad(rotationMatrix, 88, 0.017453292f * xRotation);
    }
    else {
        MTXIdentity(rotationMatrix);
    }
    if (yRotation != 0.0f) {
        MTXRotRad(axisRotationMatrix, 89, 0.017453292f * yRotation);
        MTXConcat(rotationMatrix, axisRotationMatrix, rotationMatrix);
    }
    if (zRotation != 0.0f) {
        MTXRotRad(axisRotationMatrix, 90, 0.017453292f * zRotation);
        MTXConcat(rotationMatrix, axisRotationMatrix, rotationMatrix);
    }
    ClearVectorComponents(NULL, rotatedX, rotatedY, rotatedZ);
}
