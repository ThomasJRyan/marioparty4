#include "ext_math.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/wipe.h"
#include "math.h"

#include "REL/m418Dll.h"
#include "version.h"


#ifndef __MWERKS__
s32 rand8(void);
#include "game/audio.h"
#endif

// types
typedef struct M418DllAnimationState {
    OMOBJ *object;
    s32 motionStates[4];
} M418DllAnimationState; // sizeof 0x14

typedef struct M418DllPlayerState {
    OMOBJ *object;
    s32 playerIndex;
    s32 team;
    s32 isComputer;
    s32 difficulty;
    s32 character;
    s32 controllerPort;
    s32 actionState;
    f32 verticalVelocity;
    s32 effectState;
    f32 effectTimer;
    f32 objectScale;
    f32 effectPhase;
    f32 effectScale;
    f32 effectRotation;
    f32 visibilityAlpha;
    char animationFlags[0xC];
    s32 targetIndex;
    s32 actionTimer;
    s32 effectStatus;
    s32 actionOrder;
    s32 completionState;
} M418DllPlayerState; // sizeof 0x60

typedef struct M418DllActorState {
    OMOBJ *object;
    f32 rotationX;
    f32 verticalVelocity;
    s32 actionState;
    s32 effectState;
    union {
        f32 jumpState;
        s32 jumpStateValue;
    };
    union {
        f32 jumpVelocity;
        s32 jumpVelocityValue;
    };
} M418DllActorState; // sizeof 0x1C

typedef struct M418DllPlayerSequence {
    s32 activePlayerIndex;
    s32 playerIndices[3];
    s32 secondaryPlayerIndices[3];
    s32 targetIndices[3];
    s32 orderedPlayerIndices[3];
    s32 playerCount;
    s32 sequenceState;
    s32 selectionState;
    s32 roundState;
} M418DllPlayerSequence; // sizeof 0x44

typedef struct M418DllParticleWork {
    OMOBJ *object;
    Vec origin;
    Vec targetPositions[10];
    s32 particleTimer;
    s32 particleDurations[10];
} M418DllParticleWork;

// bss
M418DllPlayerSequence gPlayerSequence;
M418DllPlayerState playerStates[4];
s32 soundEffectIds[11];
M418DllCameraState gCameraState;
M418DllAnimationState gAnimationState;
M418DllActorState actorStates[4];
M418DllActorState playerActorStates[3];
M418DllParticleWork gParticleWork;
M418DllSequenceState playerActionSequence;
M418DllSequenceState cameraActionSequence;
M418DllSequenceState menuActionSequence;
s32 activeSequenceIndex;
u32 sequenceFrameCounter;
s32 minigameSequenceId;
s32 activePlayer;
s32 minigameState;
s32 menuState;
s32 menuTimer;
HUPROCESS *objectManagerProcess;

// data
s32 modelFileIds[2] = { -1, -1 };
s32 countdownSequenceId = -1;

void CreateMinigameLighting(void)
{
    Hu3DGLightCreate(-2500.0f, 5000.0f, 5000.0f, 0.0f, -1.0f, -1.0f, 0xFF, 0xD8, 0xA0);
}

void InitializeCameraTarget(void)
{
    M418DllCameraState *cameraState = &gCameraState;
    Vec cameraPosition = { 0.0f, 1100.0f, 1800.0f };
    Vec focusPosition = { 0.0f, 600.0f, 600.0f };

    SetCameraTargetFromPositions(cameraState, cameraPosition, focusPosition);
    ApplyCameraTarget(cameraState);
    cameraState->rot.x++;
}

void SetCameraShakeOffset(s32 axis, f32 shakeOffset)
{
    M418DllCameraState *cameraState = &gCameraState;

    cameraState->shakeOffsets[axis] = shakeOffset;
}

f32 UpdateCameraShakeOffset(s32 axis)
{
    M418DllCameraState *cameraState = &gCameraState;

    if (cameraState->shakeOffsets[axis] != 0.0f) {
        cameraState->shakeOffsets[axis] *= -0.9f;
        if (cameraState->shakeOffsets[axis] >= -0.5f) {
            if (cameraState->shakeOffsets[axis] <= 0.5f) {
                cameraState->shakeOffsets[axis] = 0.0f;
            }
        }
    }
    return cameraState->shakeOffsets[axis];
}

void SetCameraUpdateCallback(M418DllFunc callback)
{
    M418DllCameraState *cameraState = &gCameraState;

    cameraState->updateCallback = callback;
}

void UpdateCamera(OMOBJ *object)
{
    M418DllCameraState *cameraState = &gCameraState;

    if (cameraState->updateCallback != NULL) {
        cameraState->updateCallback();
    }

    Center.x = cameraState->center.x + UpdateCameraShakeOffset(0);
    Center.y = cameraState->center.y + UpdateCameraShakeOffset(1);
    Center.z = cameraState->center.z + UpdateCameraShakeOffset(2);
    CRot.x = cameraState->rot.x;
    CRot.y = cameraState->rot.y;
    CRot.z = cameraState->rot.z;
    CZoom = cameraState->zoom;
}

void InitializeCamera(void)
{
    M418DllCameraState *cameraState;
    Hu3DCameraCreate(1);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 40.0f, 0.1f, 17500.0f, 1.2f);
    cameraState = &gCameraState;
    cameraState->updateCallback = InitializeCameraTarget;
    omAddObjEx(objectManagerProcess, 0x7FDA, 0, 0, -1, omOutView);
    omAddObjEx(objectManagerProcess, 0x7FD9, 0, 0, -1, UpdateCamera);
}

void InitializeShadow(void)
{
    Vec position = { -1500.0f, 2500.0f, 1000.0f };
    Vec position2 = { 0.0f, 0.0f, 0.0f };
    Vec upVector = { 0.0f, 1.0f, 0.0f };

    Hu3DShadowCreate(40.0f, 2000.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.675f);
    Hu3DShadowPosSet(&position, &upVector, &position2);
}

void StartCountdown(s32 durationSeconds)
{
    if (menuState == 0) {
        menuState = 1;
        menuTimer = durationSeconds * REFRESH_RATE;
    }
}

s32 IsCountdownComplete(void)
{
    s32 result = 0;

    if (menuState == 3) {
        result = 1;
    }
    return result;
}

void CompleteCountdown(void)
{
    if (menuState == 2) {
        menuState = 3;
        menuTimer = 30;
    }
}

void UpdateCountdownObject(OMOBJ *object)
{
    switch (menuState) {
        case 1:
            if (countdownSequenceId == -1) {
                countdownSequenceId = MGSeqCreate(1, menuTimer / REFRESH_RATE, -1, -1);
            }
            menuState = 2;
            break;
        case 2:
            menuTimer--;
            if (countdownSequenceId != -1) {
                MGSeqParamSet(countdownSequenceId, 1, ((menuTimer + REFRESH_RATE - 1) / REFRESH_RATE));
            }
            if (menuTimer <= 0) {
                menuTimer = 30;
                menuState = 3;
            }
            break;
        case 3:
            if (countdownSequenceId != -1) {
                MGSeqParamSet(countdownSequenceId, 2, -1);
                countdownSequenceId = -1;
            }
            menuTimer = 0;
            menuState = 0;
            break;
    }
}

s32 IsRandomChance(s32 divisor)
{

    if ((rand8() % divisor) == 0) {
        return 1;
    }
    else {
        return 0;
    }
}

void UpdateAnimationModels(OMOBJ *object)
{
    s32 index;
    M418DllAnimationState *animationState = &gAnimationState;

    for (index = 0; index < 3; index++) {
        switch (animationState->motionStates[index]) {
            case 1:
                SetModelMotion(object, index + 1, index + 1, 0, 2);
                animationState->motionStates[index] = 99;
                break;
            case 2:
                SetModelMotion(object, index + 1, index + 1, 0, 0);
                animationState->motionStates[index] = 99;
                break;
            case 3:
                HuAudFXPlay(0x5DD);
                Hu3DModelAttrSet(object->mdlId[index + 1], 0x40000004);
                animationState->motionStates[index] = 99;
                break;
        }
    }
}

void AttachAccessoryModels(OMOBJ *object)
{
    Mtx objectMatrix;
    char *jointNames[2] = { "scene02-dai1_null", "scene02-dai2_null1" };
    s32 index;

    for (index = 0; index < 2; index++) {
        Hu3DModelObjMtxGet(object->mdlId[5], jointNames[index], objectMatrix);
        Hu3DModelPosSet(object->mdlId[index + 6], objectMatrix[0][3], objectMatrix[1][3], objectMatrix[2][3]);
    }
    object->objFunc = UpdateAnimationModels;
}

void CreateAnimationModels(OMOBJ *object)
{
    s32 modelIds[3] = { 0x310021, 0x310022, 0x310023 };
    s32 playerIndex;

    object->mdlId[0] = Hu3DModelCreateFile(0x310011);
    Hu3DModelShadowMapSet(object->mdlId[0]);

    for (playerIndex = 1; playerIndex < 4; playerIndex++) {
        object->mdlId[playerIndex] = Hu3DModelCreateFile(modelIds[playerIndex - 1]);
        object->mtnId[playerIndex] = Hu3DMotionIDGet(object->mdlId[playerIndex]);
        Hu3DModelShadowMapSet(object->mdlId[playerIndex]);
        Hu3DModelPosSet(object->mdlId[playerIndex], 0.0f, 0.0f, 0.0f);
        SetModelMotion(object, playerIndex, playerIndex, 0, 2);
    }
    object->mdlId[4] = Hu3DModelCreateFile(0x310016);
    object->mdlId[5] = Hu3DModelCreateFile(0x310018);

    for (playerIndex = 6; playerIndex < 8; playerIndex++) {
        object->mdlId[playerIndex] = Hu3DModelCreateFile(0x310026);
        object->mtnId[playerIndex] = Hu3DMotionIDGet(object->mdlId[playerIndex]);
        Hu3DModelScaleSet(object->mdlId[playerIndex], 2.0f, 2.0f, 2.0f);
        Hu3DModelLayerSet(object->mdlId[playerIndex], 1);
        SetModelMotion(object, playerIndex, playerIndex, 0, 1);
    }

    for (playerIndex = 8; playerIndex < 11; playerIndex++) {
        object->mdlId[playerIndex] = Hu3DModelCreateFile(0x31002A);
        object->mtnId[playerIndex] = Hu3DJointMotion(object->mdlId[playerIndex], HuDataSelHeapReadNum(0x31002E, HU_MEMNUM_OVL, HEAP_MODEL));
        Hu3DModelPosSet(object->mdlId[playerIndex], -350.0f + 360.0f * (playerIndex - 8), -200.0f, -660.0f);
        SetModelMotion(object, playerIndex, playerIndex, 0, 1);
    }
    object->objFunc = AttachAccessoryModels;
}

void UpdateActorAnimation(OMOBJ *object)
{
    Mtx objectMatrix;
    f32 interpolatedValue;

    M418DllActorState *actorState;
    M418DllActorState *actorState2;
    M418DllPlayerState *playerState;
    M418DllActorState *actorState3;
    M418DllPlayerSequence *playerSequence;
    HSFOBJECT *modelObject;
    HU3DMODEL *modelData;

    playerSequence = &gPlayerSequence;
    playerState = &playerStates[playerSequence->activePlayerIndex];
    actorState = &actorStates[object->work[0]];
    if (object->work[1] == 0) {
        SetModelMotion(object, 2, 2, 0, 2);
    }
    else if (object->work[1] == 1) {
        Hu3DModelAttrReset(object->mdlId[3], 1);
        Hu3DModelAttrReset(object->mdlId[4], 1);
        Hu3DModelAttrReset(object->mdlId[5], 1);
        Hu3DModelAttrReset(object->mdlId[6], 1);
        SetModelMotion(object, 2, 2, 0, 0);
        SetModelMotion(object, 3, 3, 0, 0);
        SetModelMotion(object, 4, 4, 0, 1);
        object->work[1] = 2;
        soundEffectIds[5] = HuAudFXPlay(0x5D6);
    }
    else if (object->work[1] == 2) {
        if (Hu3DMotionEndCheck(object->mdlId[3]) != 0) {
            object->work[1] = 3;
            object->work[2] = 0;
            actorState->actionState = 1;
            HuAudFXFadeOut(soundEffectIds[5], 0x3E8);
        }
    }
    else if (object->work[1] == 3) {
        actorState2 = &actorStates[object->work[0]];
        actorState2->actionState = 2;
        actorState2->rotationX += 1.0f;
        if (actorState2->rotationX >= 20.0f) {
            actorState2->rotationX = 20.0f;
        }
        if (object->work[2]++ >= 0x5A) {
            if (actorState2->effectState == 1) {
                object->work[1] = 5;
                actorState2->verticalVelocity = 0.0f;
            }
            else {
                HuAudFXPlay(0x5D2);
                object->work[1] = 4;
                Hu3DModelObjMtxGet(object->mdlId[0], "taihou-null4", objectMatrix);
                Hu3DModelPosSet(object->mdlId[5], objectMatrix[0][3], objectMatrix[1][3], objectMatrix[2][3]);
                SetModelMotion(object, 5, 5, 0, 0);
                SetCameraShakeOffset(2, 256.0f);
            }
            actorState2->actionState = 3;
        }
    }
    else if (object->work[1] == 4) {
        if ((Hu3DMotionEndCheck(object->mdlId[5]) != 0) || (actorState->effectState == 1)) {
            object->work[1] = 99;
            Hu3DModelAttrSet(object->mdlId[3], 1);
            Hu3DModelAttrSet(object->mdlId[4], 1);
            Hu3DModelAttrSet(object->mdlId[5], 1);
        }
    }
    else if (object->work[1] == 5 || object->work[1] == 6) {
        if (actorState->verticalVelocity > 1.7f) {
            actorState->verticalVelocity = 1.7f;
        }
        if (object->work[1] == 5) {
            interpolatedValue = actorState->verticalVelocity * actorState->verticalVelocity * (actorState->verticalVelocity >= 0.0f ? -3.5f : 3.5f);
            actorState->rotationX += interpolatedValue;
        }
        else {
            interpolatedValue = actorState->verticalVelocity * actorState->verticalVelocity * (actorState->verticalVelocity >= 0.0f ? -1.25f : 1.25f);
            actorState->rotationX += interpolatedValue;
        }
        if (actorState->rotationX <= 0.0f && object->work[1] == 5) {
            actorState->rotationX = 0.0f;
            actorState->verticalVelocity = -1.47f;
            object->work[1] = 6;
            SetCameraShakeOffset(1, 4.0f);
            omVibrate(playerState->playerIndex, 0xC, 6, 6);
            HuAudFXPlay(0x5DA);
        }
        else {
            if ((actorState->rotationX <= 0.0f) && (object->work[1] == 6)) {
                Hu3DModelObjMtxGet(object->mdlId[0], "taihou-null4", objectMatrix);
                Hu3DModelPosSet(object->mdlId[6], objectMatrix[0][3], objectMatrix[1][3], objectMatrix[2][3]);
                SetCameraShakeOffset(1, 4.0f);
                omVibrate(playerState->playerIndex, 0xC, 6, 6);
                actorState->rotationX = 0.0f;
                actorState->verticalVelocity = 0.0f;
                object->work[1] = 4;
                SetModelMotion(object, 6, 6, 0, 0);
                HuAudFXPlay(0x5DA);
                HuAudFXPlay(0x5DB);
            }
        }
        actorState->verticalVelocity += 0.11f;
    }
    if (object->work[1] != 0) {
        const char *jointNames[4] = { "doukasen-null_B", "doukasen-null_A", "doukasen-null_X", "doukasen-null_Y" };
        Hu3DModelObjMtxGet(object->mdlId[3], (char*)jointNames[object->work[0]], objectMatrix);
        Hu3DModelPosSet(object->mdlId[4], objectMatrix[0][3], objectMatrix[1][3], objectMatrix[2][3]);
    }
    actorState3 = &actorStates[object->work[0]];
    modelObject = Hu3DModelObjPtrGet(object->mdlId[0], "taihou-taihou");
    modelObject->mesh.base.rot.x = actorState3->rotationX;
    modelData = &Hu3DData[object->mdlId[1]];
    modelData->rot.x = actorState3->rotationX;
}

void CreatePlayerCharacter(OMOBJ *object)
{
    s32 modelIndex = object->work[0];
    s32 modelIds[4] = { 0x310012, 0x310013, 0x310015, 0x310014 };
    s32 modelIds2[4] = { 0x310019, 0x31001A, 0x31001B, 0x31001C };
    s32 modelIds3[4] = { 0x31001D, 0x31001E, 0x31001F, 0x310020 };

    object->mdlId[0] = Hu3DModelCreateFile(0x310010);
    omSetTra(object, -540.0f + (360.0f * modelIndex), 115.0f, -900.0f);
    object->mdlId[1] = Hu3DModelCreateFile(modelIds[modelIndex]);
    Hu3DModelPosSet(object->mdlId[1], -540.0f + (360.0f * modelIndex), 115.0f, -900.0f);
    object->mdlId[2] = Hu3DModelCreateFile(modelIds2[modelIndex]);
    object->mtnId[2] = Hu3DMotionIDGet(object->mdlId[2]);
    SetModelMotion(object, 2, 2, 0, 2);
    object->mdlId[3] = Hu3DModelCreateFile(modelIds3[modelIndex]);
    object->mtnId[3] = Hu3DMotionIDGet(object->mdlId[3]);
    Hu3DModelAttrSet(object->mdlId[3], 1);
    SetModelMotion(object, 3, 3, 0, 2);
    object->mdlId[4] = Hu3DModelCreateFile(0x310025);
    object->mtnId[4] = Hu3DMotionIDGet(object->mdlId[4]);
    Hu3DModelAttrSet(object->mdlId[4], 1);
    SetModelMotion(object, 4, 4, 0, 2);
    object->mdlId[5] = Hu3DModelCreateFile(0x310024);
    object->mtnId[5] = Hu3DMotionIDGet(object->mdlId[5]);
    Hu3DModelAttrSet(object->mdlId[5], 1);
    SetModelMotion(object, 5, 5, 0, 2);
    object->mdlId[6] = Hu3DModelCreateFile(0x310028);
    object->mtnId[6] = Hu3DMotionIDGet(object->mdlId[6]);
    Hu3DModelAttrSet(object->mdlId[6], 1);
    SetModelMotion(object, 6, 6, 0, 2);
    Hu3DModelScaleSet(object->mdlId[6], 3.0f, 3.0f, 3.0f);
    object->objFunc = UpdateActorAnimation;
}

void InitializePlayerActor(OMOBJ *object)
{
    M418DllActorState *actorState;

    actorState = &playerActorStates[object->work[0]];
    if (actorState->jumpState == 0.0f) {
        actorState->jumpState = 1.0f;
    }
}

s32 MoveObjectTowardPosition(OMOBJ *object, Vec targetPosition, f32 desiredAngle, f32 maxDistance)
{
    M418DllActorState *actorState;
    s32 result;
    f32 horizontalDeltaX;
    f32 horizontalDeltaZ;
    f32 distance;
    f32 movementAngle;
    f32 interpolatedValue;
    f32 interpolatedValue2;

    result = 0;
    actorState = &playerActorStates[object->work[0]];
    horizontalDeltaX = targetPosition.x - object->trans.x;
    horizontalDeltaZ = object->trans.z - targetPosition.z;
    movementAngle = 90.0 + atan2d(horizontalDeltaZ, horizontalDeltaX);
    distance = sqrtf((horizontalDeltaX * horizontalDeltaX) + (horizontalDeltaZ * horizontalDeltaZ));
    if (distance <= 15.0f) {
        horizontalDeltaX = horizontalDeltaZ = 0.0f;
    }
    else {
        horizontalDeltaX = 55.0f * (horizontalDeltaX / distance);
        horizontalDeltaZ = 55.0f * (horizontalDeltaZ / distance);
    }
    if (actorState->jumpState == 1.0f) {
        actorState->jumpVelocity = -1.47f;
    }
    distance = maxDistance * (sqrtf((horizontalDeltaX * horizontalDeltaX) + (horizontalDeltaZ * horizontalDeltaZ)) / 72.0f);
    if (distance >= maxDistance) {
        distance = maxDistance;
    }
    else if (distance <= 1.0f) {
        horizontalDeltaX = horizontalDeltaZ = distance = 0.0f;
        result = 1;
    }
    if (horizontalDeltaX != 0.0f || horizontalDeltaZ != 0.0f || desiredAngle >= 0.0f) {
        if (desiredAngle >= 0.0f && horizontalDeltaX == 0.0f && horizontalDeltaZ == 0.0f) {
            movementAngle = desiredAngle;
        }
        if (movementAngle >= 180.0f) {
            if (object->rot.y - movementAngle >= 180.0f) {
                object->rot.y -= 360.0f;
            }
            if (object->rot.y - movementAngle < -180.0f) {
                object->rot.y += 360.0f;
            }
        }
        else {
            if (object->rot.y - movementAngle > 180.0f) {
                object->rot.y -= 360.0f;
            }
            if (object->rot.y - movementAngle <= -180.0f) {
                object->rot.y += 360.0f;
            }
        }
        object->rot.y = (((9.0f * object->rot.y) + movementAngle) / 10.0f);
    }
    object->trans.x += distance * sind(object->rot.y);
    object->trans.z += distance * cosd(object->rot.y);
    if (actorState->jumpState) {
        actorState->jumpState = 2.0f;
        if (actorState->jumpVelocity > 1.7f) {
            actorState->jumpVelocity = 1.7f;
        }
        interpolatedValue2 = (actorState->jumpVelocity * actorState->jumpVelocity) * (actorState->jumpVelocity >= 0.0f ? -3.5f : 3.5f);
        object->trans.y += interpolatedValue2;
        if (object->trans.y <= targetPosition.y) {
            object->trans.y = targetPosition.y;
            actorState->jumpState = actorState->jumpVelocity = 0.0f;
            result = 2;
        }
        actorState->jumpVelocity += 0.11f;
    }
    if (distance > 0.0f) {
        SetModelMotionIfChanged(object, 0, 1, 10, 1);
    }
    else {
        SetModelMotionIfChanged(object, 0, 0, 10, 1);
    }
    return result;
}

s32 UpdateObjectMovement(OMOBJ *object, Vec targetPosition, f32 maxDistance, f32 desiredAngle, s32 initializeJump)
{
    if (initializeJump == 1)
        InitializePlayerActor(object);
    return MoveObjectTowardPosition(object, targetPosition, desiredAngle, maxDistance);
}

Vec movementWaypoints[9] = { { 140.0f, 0.0f, -90.0f }, { -200.0f, 0.0f, 50.0f }, { -300.0f, 0.0f, -150.0f }, { -250.0f, 0.0f, -500.0f },
    { 0.0f, 0.0f, 150.0f }, { 700.0f, 0.0f, 50.0f }, { 1000.0f, -50.0f, 500.0f }, { 1000.0f, -100.0f, 500.0f }, { 1000.0f, -150.0f, 500.0f } };

s32 UpdateObjectMotion(OMOBJ *object)
{
    s32 status;

    switch (object->work[1]) {
        case 1:
            object->work[2] = 0;
            object->work[1] = 10;
        case 10:
            UpdateObjectMovement(object, movementWaypoints[1], 0.0f, 25.0f, 0);
            if (object->work[2]++ >= 15) {
                object->work[1] = 11;
                object->work[2] = 0;
            }
            break;
        case 11:
            status = UpdateObjectMovement(object, movementWaypoints[1], 6.0f, -1.0f, 0);
            if (status != 0) {
                object->work[1] = 12;
                object->work[2] = 0;
            }
            break;
        case 12:
            UpdateObjectMovement(object, movementWaypoints[1], 0.0f, 0.0f, 0);
            if (object->work[2]++ >= 15) {
                object->work[1] = 13;
                object->work[2] = 0;
            }
            break;

        case 2:
            object->work[2] = 0;
            object->work[1] = 0x14;
        case 20:
            UpdateObjectMovement(object, movementWaypoints[0], 0.0f, 25.0f, 0);
            if (object->work[2]++ >= 15) {
                object->work[1] = 21;
                object->work[2] = 0;
            }
            break;
        case 21:
            status = UpdateObjectMovement(object, movementWaypoints[4], 6.0f, -1.0f, 0);
            if (status != 0) {
                object->work[1] = 22;
                object->work[2] = 0;
            }
            break;
        case 22:
            status = UpdateObjectMovement(object, movementWaypoints[0], 6.0f, 0.0f, 0);
            if (status != 0) {
                object->work[1] = 24;
                object->work[2] = 0;
            }
            break;
        case 23:
            UpdateObjectMovement(object, movementWaypoints[1], 0.0f, 0.0f, 0);
            if (object->work[2]++ >= 30) {
                object->work[1] = 24;
                object->work[2] = 0;
            }
            break;
        case 3:
            object->work[2] = 0;
            object->work[1] = 30;
        case 30:
            status = UpdateObjectMovement(object, movementWaypoints[5], 6.0f, -1.0f, 0);
            if (status == 1) {
                object->work[1] = 31;
                object->work[2] = 0;
            }
            break;
        case 31:
            status = UpdateObjectMovement(object, movementWaypoints[6], 6.0f, -1.0f, 1);
            if (status != 0) {
                object->work[1] = 32;
                object->work[2] = 0;
            }
            break;
        case 32:
            status = UpdateObjectMovement(object, movementWaypoints[7], 6.0f, -1.0f, 1);
            if (status != 0) {
                object->work[1] = 33;
                object->work[2] = 0;
            }
            break;
        case 33:
            status = UpdateObjectMovement(object, movementWaypoints[8], 6.0f, -1.0f, 1);
            if (status != 0) {
                object->work[1] = 34;
                object->work[2] = 0;
            }
            break;
    }
}

void UpdatePlayerAction(OMOBJ *object)
{
    Mtx objectMatrix;
    Mtx objectMatrix2;
    M418DllPlayerSequence *playerSequence;
    f32 cameraScale;

    playerSequence = &gPlayerSequence;
    cameraScale = 0.5f + (0.5f * (CRot.x / 90.0f));
    Hu3DModelObjMtxGet(object->mdlId[0], "g031m1-itemhook_R", objectMatrix2);
    Hu3DModelObjMtxGet(object->mdlId[5], "g031i00-itemhook_eff", objectMatrix);
    Hu3DModelRotSet(object->mdlId[6], 0.0f, -object->rot.y, 0.0f);
    Hu3DModelScaleSet(object->mdlId[6], 0.35f, cameraScale, 0.35f);
    UpdateObjectMotion(object);
}

void CreatePlayerActor(OMOBJ *object)
{
    u32 modelIndex;

    modelIndex = object->work[0];
    object->mdlId[0] = Hu3DModelCreateFile(0x31002A);
    object->mtnId[0] = Hu3DJointMotionFile(object->mdlId[0], 0x31002B);
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[0], 0x31002C);
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[0], 0x31002D);
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[0], 0x31002E);
    omSetTra(object, movementWaypoints[modelIndex].x, movementWaypoints[modelIndex].y, movementWaypoints[modelIndex].z);
    SetModelMotion(object, 0, 0, 0, 1);
    object->mdlId[5] = Hu3DModelCreateFile(0x31002F);
    Hu3DModelHookSet(object->mdlId[0], "g031m1-itemhook_R", object->mdlId[5]);
    object->mdlId[6] = Hu3DModelCreateFile(0x310027);
    object->mtnId[6] = Hu3DMotionIDGet(object->mdlId[6]);
    Hu3DModelHookSet(object->mdlId[5], "g031i00-itemhook_eff", object->mdlId[6]);
    SetModelMotion(object, 6, 6, 0, 1);
    object->work[1] = -1;
    Hu3DModelShadowSet(object->mdlId[0]);
    object->objFunc = UpdatePlayerAction;
}

void SpawnParticleAt(Vec particlePosition)
{
    M418DllParticleWork *particleSystem = &gParticleWork;
    particleSystem->origin.x = particlePosition.x;
    particleSystem->origin.y = particlePosition.y;
    particleSystem->origin.z = particlePosition.z;
    particleSystem->particleTimer = 0;
    particleSystem->object->work[0] = 1;
}

void UpdatePlayerActor(OMOBJ *object)
{
    HU3DMODEL *modelData;
    M418DllParticleWork *particleSystem = &gParticleWork;
    s32 index;

    switch (object->work[0]) {
        case 1:
            for (index = 0; index < 10; index++) {
                particleSystem->targetPositions[index].x = (particleSystem->origin.x + 50.0 * cosd(36.0f * index)) + (rand8() % 25);
                particleSystem->targetPositions[index].y = (particleSystem->origin.y + 50.0 * sind(36.0f * index)) + (rand8() % 25);
                particleSystem->targetPositions[index].z = particleSystem->origin.z;
                particleSystem->particleDurations[index] = (rand8() % 5) + 10;
                Hu3DModelPosSet(object->mdlId[index + 1], particleSystem->origin.x, particleSystem->origin.y, particleSystem->origin.z);
                Hu3DModelTPLvlSet(object->mdlId[index + 1], 1.0f);
            }
            object->work[0] = 2;
            break;
        case 2:
            for (index = 0; index < 10; index++) {
                modelData = &Hu3DData[object->mdlId[index + 1]];
                modelData->pos.x = InterpolateWeightedAverage(modelData->pos.x, particleSystem->targetPositions[index].x, particleSystem->particleDurations[index]);
                modelData->pos.y = InterpolateWeightedAverage(modelData->pos.y, particleSystem->targetPositions[index].y, particleSystem->particleDurations[index]);
                modelData->pos.z = InterpolateWeightedAverage(modelData->pos.z, particleSystem->targetPositions[index].z, particleSystem->particleDurations[index]);
                if (particleSystem->particleTimer >= 20) {
                    Hu3DModelTPLvlSet(object->mdlId[index + 1], 1.0f - (0.1f * (particleSystem->particleTimer - 0x14)));
                }
            }
            if (particleSystem->particleTimer >= 30) {
                object->work[0] = 0;
            }
            particleSystem->particleTimer++;
    }
}

void UpdateParticleSystem(OMOBJ *object)
{
    s32 index;

    for (index = 1; index < 11; index++) {
        object->mdlId[index] = Hu3DModelCreateFile(0x310029);
        Hu3DModelPosSet(object->mdlId[index], 0.0f, -500.0f, 0.0f);
        Hu3DModelScaleSet(object->mdlId[index], 10.0f, 10.0f, 10.0f);
    }
    object->work[0] = 0;
    object->objFunc = UpdatePlayerActor;
}

void InitializeMinigameSequences(void)
{
    M418DllPlayerState *playerState;
    s32 playerIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerState = &playerStates[playerIndex];
        playerState->playerIndex = playerIndex;
        playerState->team = GWPlayerCfg[playerState->playerIndex].group;
        if (playerState->team == 0) {
            playerState->team = 0;
        }
        else {
            playerState->team = 1;
        }
        playerState->isComputer = GWPlayerCfg[playerState->playerIndex].iscom;
        if (playerState->isComputer != 0) {
            playerState->difficulty = GWPlayerCfg[playerState->playerIndex].diff;
        }
        else {
            playerState->difficulty = 0;
        }
        playerState->character = GWPlayerCfg[playerState->playerIndex].character;
        playerState->controllerPort = GWPlayerCfg[playerState->playerIndex].pad_idx;
    }
}

char *characterJointNames[8] = { "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r",
    "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r" };

void UpdateGameObject(OMOBJ *object)
{
    Mtx objectMatrix;
    Vec position;
    M418DllPlayerState *playerState;
    HU3DMODEL *modelData;
    M418DllParticleWork *particleSystem;

    modelData = &Hu3DData[object->mdlId[11]];
    playerState = &playerStates[object->work[0]];
    switch (object->work[1]) {
        case 1:
            Hu3DModelHookReset(object->mdlId[0]);
            Hu3DModelObjMtxGet(object->mdlId[0], characterJointNames[playerState->character], objectMatrix);
            Hu3DModelPosSet(object->mdlId[11], objectMatrix[0][3], objectMatrix[1][3], objectMatrix[2][3]);
            modelData->rot.y = object->rot.y;
            object->work[1] = 2;
            object->work[2] = 0;
            break;
        case 2:
            SetModelMotionIfChanged(object, 0, 0, 0x1E, 1);
            modelData->pos.y = InterpolateWeightedAverage(modelData->pos.y, 350.0f, 30.0f);
            modelData->rot.x += 30.0f;
            if (object->work[2]++ >= 0x1EU) {
                HuAudFXPlay(0x5DE);
                SpawnParticleAt(modelData->pos);
                Hu3DModelAttrSet(object->mdlId[11], 1);
                object->work[1] = object->work[2] = 0;
            }
            break;
    }
}

void UpdatePlayerMovement(OMOBJ *object)
{
    M418DllPlayerState *playerState;

    playerState = &playerStates[object->work[0]];
    if (playerState->actionState == 0) {
        playerState->actionState = 1;
    }
}

void MoveObjectToPosition(OMOBJ *object, Vec targetPosition, f32 desiredAngle, f32 maxDistance)
{
    s32 stateValue;
    f32 interpolatedValue;
    f32 horizontalDeltaX;
    f32 horizontalDeltaZ;
    M418DllPlayerState *playerState;
    M418DllPlayerState *playerState2;

    f32 horizontalDeltaX2;
    f32 horizontalDeltaZ2;
    f32 horizontalDeltaX3;
    f32 horizontalDeltaZ3;
    f32 distance;
    f32 movementAngle;
    f32 interpolatedValue2;

    s32 status;
    s32 playerIndex;

    stateValue = 0;
    status = 0;
    playerState2 = &playerStates[object->work[0]];
    horizontalDeltaX2 = targetPosition.x - object->trans.x;
    horizontalDeltaZ2 = object->trans.z - targetPosition.z;
    movementAngle = 90.0 + atan2d(horizontalDeltaZ2, horizontalDeltaX2);
    distance = sqrtf((horizontalDeltaX2 * horizontalDeltaX2) + (horizontalDeltaZ2 * horizontalDeltaZ2));
    if (distance <= 30.0f) {
        horizontalDeltaX2 = horizontalDeltaZ2 = 0.0f;
    }
    else {
        horizontalDeltaX2 = 55.0f * (horizontalDeltaX2 / distance);
        horizontalDeltaZ2 = 55.0f * (horizontalDeltaZ2 / distance);
    }
    if (playerState2->actionState == 1) {
        playerState2->verticalVelocity = -1.47f;
    }
    distance = maxDistance * (sqrtf((horizontalDeltaX2 * horizontalDeltaX2) + (horizontalDeltaZ2 * horizontalDeltaZ2)) / 72.0f);
    if (distance >= maxDistance) {
        distance = maxDistance;
    }
    else if (distance <= 1.0f) {
        horizontalDeltaX2 = horizontalDeltaZ2 = distance = 0.0f;
    }
    if (horizontalDeltaX2 != 0.0f || horizontalDeltaZ2 != 0.0f || desiredAngle >= 0.0f) {
        if (desiredAngle >= 0.0f && horizontalDeltaX2 == 0.0f && horizontalDeltaZ2 == 0.0f) {
            movementAngle = desiredAngle;
        }
        if (movementAngle >= 180.0f) {
            if (object->rot.y - movementAngle >= 180.0f) {
                object->rot.y -= 360.0f;
            }
            if (object->rot.y - movementAngle < -180.0f) {
                object->rot.y += 360.0f;
            }
        }
        else {
            if (object->rot.y - movementAngle > 180.0f) {
                object->rot.y -= 360.0f;
            }
            if (object->rot.y - movementAngle <= -180.0f) {
                object->rot.y += 360.0f;
            }
        }
        object->rot.y = (((4.0f * object->rot.y) + movementAngle) / 5.0f);
    }
    status = 0;
    horizontalDeltaX = object->trans.x + (50.0 * cosd(object->rot.y - 90.0f));
    horizontalDeltaZ = object->trans.z - (50.0 * sind(object->rot.y - 90.0f));

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerIndex == object->work[0])
            continue;
        playerState = &playerStates[playerIndex];
        horizontalDeltaX3 = playerState->object->trans.x;
        horizontalDeltaZ3 = playerState->object->trans.z;
        horizontalDeltaX3 -= horizontalDeltaX;
        horizontalDeltaZ3 -= horizontalDeltaZ;
        if (sqrtf((horizontalDeltaX3 * horizontalDeltaX3) + (horizontalDeltaZ3 * horizontalDeltaZ3)) < 100.0f) {
            status = 1;
            break;
        }
    }
    if (status == 0) {
        object->trans.x = object->trans.x + (distance * sind(object->rot.y));
        object->trans.z = object->trans.z + (distance * cosd(object->rot.y));
    }
    if (playerState2->actionState != 0) {
        playerState2->actionState = 2;
        if (playerState2->verticalVelocity > 1.7f) {
            playerState2->verticalVelocity = 1.7f;
        }
        interpolatedValue = playerState2->verticalVelocity * playerState2->verticalVelocity * (playerState2->verticalVelocity >= 0.0f ? -17.5f : 17.5f);
        object->trans.y += interpolatedValue;
        if (object->trans.y <= 0.0f) {
            object->trans.y = 0.0f;
            playerState2->actionState = playerState2->verticalVelocity = 0.0f;
            omVibrate(playerState2->playerIndex, 0xC, 6, 6);
        }
        playerState2->verticalVelocity += 0.11f;
    }
    if (playerState2->actionState != 0) {
        SetModelMotionIfChanged(object, 0, 3, 10, 0);
        return;
    }
    if (distance >= 6.75f) {
        SetModelMotionIfChanged(object, 0, 2, 10, 1);
        return;
    }
    if (distance > 0.0f) {
        SetModelMotionIfChanged(object, 0, 1, 10, 1);
        return;
    }
    SetModelMotionIfChanged(object, 0, 0, 10, 1);
}

void MoveObjectAlongPath(OMOBJ *object, Vec targetPosition, f32 desiredAngle, f32 maxDistance)
{
    M418DllPlayerState *playerState;
    s32 stateValue;
    f32 horizontalDeltaX;
    f32 horizontalDeltaZ;
    f32 distance;
    f32 movementAngle;
    f32 interpolatedValue;
    f32 interpolatedValue2;

    stateValue = 0;
    playerState = &playerStates[object->work[0]];
    horizontalDeltaX = targetPosition.x - object->trans.x;
    horizontalDeltaZ = object->trans.z - targetPosition.z;
    movementAngle = 90.0 + atan2d(horizontalDeltaZ, horizontalDeltaX);
    distance = sqrtf((horizontalDeltaX * horizontalDeltaX) + (horizontalDeltaZ * horizontalDeltaZ));
    if (distance <= 30.0f) {
        horizontalDeltaX = horizontalDeltaZ = 0.0f;
    }
    else {
        horizontalDeltaX = 55.0f * (horizontalDeltaX / distance);
        horizontalDeltaZ = 55.0f * (horizontalDeltaZ / distance);
    }
    if (playerState->actionState == 1) {
        playerState->verticalVelocity = -1.47f;
    }
    distance = maxDistance * (sqrtf((horizontalDeltaX * horizontalDeltaX) + (horizontalDeltaZ * horizontalDeltaZ)) / 72.0f);
    if (distance >= maxDistance) {
        distance = maxDistance;
    }
    else if (distance <= 1.0f) {
        horizontalDeltaX = horizontalDeltaZ = distance = 0.0f;
    }
    if (horizontalDeltaX != 0.0f || horizontalDeltaZ != 0.0f || desiredAngle >= 0.0f) {
        if (desiredAngle >= 0.0f && horizontalDeltaX == 0.0f && horizontalDeltaZ == 0.0f) {
            movementAngle = desiredAngle;
        }
        if (movementAngle >= 180.0f) {
            if (object->rot.y - movementAngle >= 180.0f) {
                object->rot.y -= 360.0f;
            }
            if (object->rot.y - movementAngle < -180.0f) {
                object->rot.y += 360.0f;
            }
        }
        else {
            if (object->rot.y - movementAngle > 180.0f) {
                object->rot.y -= 360.0f;
            }
            if (object->rot.y - movementAngle <= -180.0f) {
                object->rot.y += 360.0f;
            }
        }
        object->rot.y = (((4.0f * object->rot.y) + movementAngle) / 5.0f);
    }
    object->trans.x += distance * sind(object->rot.y);
    object->trans.z += distance * cosd(object->rot.y);
    if (playerState->actionState) {
        playerState->actionState = 2.0f;
        if (playerState->verticalVelocity > 1.7f) {
            playerState->verticalVelocity = 1.7f;
        }
        interpolatedValue2 = (playerState->verticalVelocity * playerState->verticalVelocity) * (playerState->verticalVelocity >= 0.0f ? -17.5f : 17.5f);
        object->trans.y += interpolatedValue2;
        if (object->trans.y <= 0.0f) {
            object->trans.y = 0.0f;
            playerState->actionState = playerState->verticalVelocity = 0.0f;
            omVibrate(playerState->playerIndex, 0xC, 6, 6);
        }
        playerState->verticalVelocity += 0.11f;
    }
    if (playerState->actionState != 0) {
        SetModelMotionIfChanged(object, 0, 3, 10, 0);
    }
    else if (distance >= 6.75f) {
        SetModelMotionIfChanged(object, 0, 2, 10, 1);
    }
    else if (distance > 0.0f) {
        SetModelMotionIfChanged(object, 0, 1, 10, 1);
    }
    else {
        SetModelMotionIfChanged(object, 0, 0, 10, 1);
    }
}

s32 animationModelIds[4][2] = { { -1, 127 }, { -1, 127 }, { -1, 127 }, { -1, 127 } };

void UpdateCharacterObject(OMOBJ *object)
{
    s32 modelIndex;
    M418DllPlayerState *playerState;

    modelIndex = 0;
    playerState = &playerStates[object->work[0]];

    switch (playerState->effectState) {
        case 0:
            playerState->objectScale = object->scale.x;
            playerState->effectScale = playerState->effectRotation = 0.0f;
            playerState->effectState = 1;
            break;
        case 1:
            playerState->effectTimer -= 1.0f;
            if (playerState->effectTimer <= 0.0f) {
                playerState->effectTimer = 15.0f;
                playerState->effectState = 2;
                omVibrate(playerState->playerIndex, 0xC, 0xC, 0);
                animationModelIds[object->work[0]][0] = CharFXPlay(playerState->character, 0x123);
                animationModelIds[object->work[0]][1] = 0x7F;
                HuAudFXPlay(0x5D4);
            }
            break;
        case 2:
            object->trans.x += 4.0f * playerState->effectPhase;
            object->trans.y += 125.0 * sind(playerState->effectTimer);
            object->trans.z -= 125.0 * cos(M_PI / 12.0);
            if (playerState->effectTimer <= 14.0f) {
                Hu3DModelAttrReset(object->mdlId[0], 1);
            }
            playerState->effectTimer -= 0.25f;
            if (playerState->effectTimer <= 0.0f) {
                playerState->effectState = 3;
            }
            break;
        case 3:
            object->trans.y += 110.0 * sind(playerState->effectTimer);
            playerState->effectTimer -= 0.0275f;
            playerState->objectScale -= 0.0025f;
            if (playerState->objectScale <= 0.3f) {
                playerState->objectScale = 0.0f;
                playerState->effectState = 4;
                playerState->effectTimer = 0.0f;
                HuAudFXPlay(0x5D5);
            }
            omSetSca(object, playerState->objectScale, playerState->objectScale, playerState->objectScale);
            break;
        case 4:
            playerState->effectRotation = InterpolateWeightedAverage(playerState->effectRotation, 360.0f, 15.0f);
            playerState->effectScale = InterpolateWeightedAverage(playerState->effectScale, 4.0f, 15.0f);
            Hu3DModelPosSet(object->mdlId[1], object->trans.x, object->trans.y, object->trans.z);
            Hu3DModelRotSet(object->mdlId[1], 0.0f, 0.0f, playerState->effectRotation);
            Hu3DModelScaleSet(object->mdlId[1], playerState->effectScale, playerState->effectScale, playerState->effectScale);
            if (playerState->effectTimer++ >= 30.0f) {
                playerState->effectState = 5;
                playerState->effectTimer = 0.0f;
            }
            break;
        case 5:
            playerState->effectRotation = InterpolateWeightedAverage(playerState->effectRotation, 720.0f, 15.0f);
            playerState->effectScale = InterpolateWeightedAverage(playerState->effectScale, 0.0f, 15.0f);
            Hu3DModelPosSet(object->mdlId[1], object->trans.x, object->trans.y, object->trans.z);
            Hu3DModelRotSet(object->mdlId[1], 0.0f, 0.0f, playerState->effectRotation);
            Hu3DModelScaleSet(object->mdlId[1], playerState->effectScale, playerState->effectScale, playerState->effectScale);
            if (playerState->effectTimer++ >= 60.0f) {
                playerState->effectState = 6;
                playerState->effectTimer = 0.0f;
            }
            break;
    }
    if ((playerState->effectState >= 1) && (playerState->effectState <= 5) && (animationModelIds[object->work[0]][0] != -1)) {
        HuAudFXVolSet(animationModelIds[object->work[0]][0], --animationModelIds[object->work[0]][1]);
        if (animationModelIds[object->work[0]][1] < 0) {
            animationModelIds[object->work[0]][1] = 0;
        }
    }
}

void UpdateCharacterEffects(OMOBJ *object)
{
    Mtx objectMatrix;
    Vec position;
    f32 cameraScale;
    M418DllPlayerState *playerState;
    M418DllParticleWork *particleSystem;
    M418DllPlayerState *playerState2;
    HU3DMODEL *modelData;
    HU3DMODEL *modelData2;
    s32 modelIndex;
    u32 modelIndex2;

    playerState2 = &playerStates[object->work[0]];
    if (playerState2->team == 0) {
        cameraScale = (0.5f * playerState2->visibilityAlpha) + ((CRot.x / 90.0f) * (0.5f * playerState2->visibilityAlpha));
        modelData2 = &Hu3DData[object->mdlId[12]];
        if ((playerState2->character == 7) || (playerState2->character == 5)) {
            modelData2->rot.y = -object->rot.y - 360.0f;
        }
        else {
            modelData2->rot.y = -object->rot.y - 90.0f;
        }
        modelData2->scale.x = 0.35f * playerState2->visibilityAlpha;
        modelData2->scale.y = cameraScale;
        modelData2->scale.z = 0.35f * playerState2->visibilityAlpha;
        UpdateGameObject(object);
    }
    UpdateCharacterObject(object);
}

void CreateCharacterObject(OMOBJ *object)
{
    M418DllPlayerState *playerState;

    playerState = &playerStates[object->work[0]];
    object->mdlId[0] = CharModelCreate(playerState->character, 2);
    object->mtnId[0] = CharMotionCreate(playerState->character, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00));
    object->mtnId[1] = CharMotionCreate(playerState->character, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02));
    object->mtnId[2] = CharMotionCreate(playerState->character, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03));
    object->mtnId[3] = CharMotionCreate(playerState->character, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x73));
    object->mtnId[4] = CharMotionCreate(playerState->character, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x74));
    object->mtnId[5] = CharMotionCreate(playerState->character, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17));
    object->mtnId[6] = CharMotionCreate(playerState->character, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18));
    object->mtnId[7] = CharMotionCreate(playerState->character, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x1B));
    object->mtnId[8] = CharMotionCreate(playerState->character, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x2A));
    object->mtnId[9] = Hu3DJointMotion(object->mdlId[0], HuDataSelHeapReadNum(playerState->character + 0x310000, 0x10000000, HEAP_MODEL));
    object->mtnId[10] = Hu3DJointMotion(object->mdlId[0], HuDataSelHeapReadNum(playerState->character + 0x310008, 0x10000000, HEAP_MODEL));
    object->mdlId[1] = Hu3DModelCreateFile(0x310017);
    Hu3DModelPosSet(object->mdlId[1], 0.0f, -5000.0f, 0.0f);
    Hu3DModelShadowSet(object->mdlId[0]);
    if (playerState->team == 0) {
        Hu3DMotionOverlaySet(object->mdlId[0], object->mtnId[9]);
        Hu3DMotionOverlaySpeedSet(object->mdlId[0], 0.0f);
        object->mdlId[11] = Hu3DModelCreateFile(0x31002F);
        Hu3DModelHookSet(object->mdlId[0], CharModelItemHookGet(playerState->character, 2, 0), object->mdlId[11]);
        if ((playerState->character == 4) || (playerState->character == 5)) {
            Hu3DModelScaleSet(object->mdlId[11], 1.0f, 1.2f, 1.0f);
        }
        object->mdlId[12] = Hu3DModelCreateFile(0x310027);
        object->mtnId[12] = Hu3DMotionIDGet(object->mdlId[12]);
        Hu3DModelHookSet(object->mdlId[11], "g031i00-itemhook_eff", object->mdlId[12]);
        Hu3DModelScaleSet(object->mdlId[12], 0.0f, 0.5f, 0.0f);
        SetModelMotion(object, 0xC, 0xC, 0, 1);
        object->mdlId[13] = Hu3DModelCreateFile(0x310028);
        object->mtnId[13] = Hu3DMotionIDGet(object->mdlId[13]);
        SetModelMotion(object, 0xD, 0xD, 0, 2);
    }
    animationModelIds[object->work[0]][0] = -1;
    CharMotionDataClose(playerState->character);
    CharModelLayerSetAll2(7);
    object->objFunc = UpdateCharacterEffects;
}

void InitializePlayerSequence(void)
{
    M418DllCameraState *cameraState = &gCameraState;
    Vec cameraPosition = { 0.0f, 1100.0f, 1800.0f };
    Vec focusPosition = { 0.0f, 600.0f, 600.0f };

    SetCameraTargetFromPositions(cameraState, cameraPosition, focusPosition);
    ApplyCameraTarget(cameraState);
    cameraState->rot.x++;
}

void AdvancePlayerSequence(void)
{
    M418DllCameraState targetCameraState;
    M418DllCameraState *cameraState = &gCameraState;
    Vec position = { -180.0f, 500.0f, 440.0f };
    Vec focusPosition = { -180.0f, 330.0f, -760.0f };

    SetCameraTargetFromPositions(&targetCameraState, position, focusPosition);
    ApplyCameraTarget(&targetCameraState);
    targetCameraState.center.x = 0.0f;
    targetCameraState.center.y -= 75.0f;
    targetCameraState.rot.x = -10.0f;
    targetCameraState.rot.y = 10.0f;
    TransitionCameraStateCosine(cameraState, &targetCameraState, cameraState->interpolationFactor++, 30.0f, 10.0f);
}

void UpdatePlayerSequence(void)
{
    M418DllCameraState targetCameraState;
    M418DllCameraState *cameraState = &gCameraState;
    Vec cameraPosition = { 0.0f, 1100.0f, 1800.0f };
    Vec focusPosition = { 0.0f, 600.0f, 600.0f };

    SetCameraTargetFromPositions(&targetCameraState, cameraPosition, focusPosition);
    ApplyCameraTarget(&targetCameraState);
    targetCameraState.rot.x++;
    TransitionCameraStateSine(cameraState, &targetCameraState, cameraState->interpolationFactor++, 30.0f, 10.0f);
}

void SetPlayerSequenceStage(s32 stageIndex)
{
    M418DllCameraState *cameraState = &gCameraState;
    ResetCameraTarget(cameraState);
    switch (stageIndex) {
        case 0:
            SetCameraUpdateCallback(InitializePlayerSequence);
            break;
        case 1:
            SetCameraUpdateCallback(AdvancePlayerSequence);
            break;
        case 2:
            SetCameraUpdateCallback(UpdatePlayerSequence);
            break;
    }
}

void StartPlayerSequenceStage(s32 )
{
    s32 index;
    M418DllActorState *actorState;
    M418DllCameraState *cameraState;

    for (index = 0; index < 3; index++) {
        actorState = &actorStates[index];
        actorState->actionState = 0;
    }

    SetPlayerSequenceStage(0);
}

s32 UpdatePlayerSequenceStage(s32 )
{
    M418DllActorState *actorState;
    M418DllCameraState *cameraState;
    s32 stateValue;
    s32 index;

    if (AdvanceSequenceTimer(&playerActionSequence, 0) >= 0x3C) {
        for (index = 0; index < 3; index++) {
            actorState = &actorStates[index];
            if (actorState->actionState == 0) {
                if ((rand8() % 5) == 0) {
                    stateValue = 1;
                }
                else {
                    stateValue = 0;
                }
                if (stateValue || (AdvanceSequenceTimer(&playerActionSequence, 0) >= 0x41)) {
                    HuAudFXPlay(0x5D3);
                    actorState->actionState = 1;
                }
            }
            else {
                actorState->rotationX += 5.0f;
                if (actorState->rotationX >= 90.0f) {
                    actorState->rotationX = 90.0f;
                }
            }
        }
    }
    if (AdvanceSequenceTimer(&playerActionSequence, 0) == 0x1E) {
        SetPlayerSequenceStage(1);
    }
    if (AdvanceSequenceTimer(&playerActionSequence, 1) != 0) {
        return AdvanceSequence(&playerActionSequence);
    }
    else {
        return 0;
    }
}

s32 characterModelIds[16] = { 0x310000, 0x310001, 0x310002, 0x310003, 0x310004, 0x310005, 0x310006, 0x310007, 0x310008, 0x310009, 0x31000A, 0x31000B,
    0x31000C, 0x31000D, 0x31000E, 0x31000F };

Vec playerStartPositions[3] = {
    { -540.0f, 225.0f, -900.0f },
    { -180.0f, 225.0f, -900.0f },
    { 180.0f, 225.0f, -900.0f },
};

void StartPlayerMovementStage(s32 )
{
    M418DllPlayerState *playerState;
    M418DllPlayerSequence *playerSequence;
    M418DllActorState *actorState;
    s32 index;

    playerSequence = &gPlayerSequence;

    for (index = 0; index < 3; index++) {
        playerState = &playerStates[playerSequence->playerIndices[index]];
        omSetTra(playerState->object, playerStartPositions[index].x, playerStartPositions[index].y, playerStartPositions[index].z - 50.0f);
        playerState->targetIndex = 0;
    }

    for (index = 0; index < 3; index++) {
        actorState = &actorStates[index];
        actorState->actionState = 0;
    }
}

Vec playerTargetPositions[4] = {
    { -355.0f, 0.0f, -680.0f },
    { 5.0f, 0.0f, -680.0f },
    { 365.0f, 0.0f, -680.0f },
};

s32 UpdatePlayerMovementStage(s32 )
{
    s32 index;
    M418DllPlayerState *playerState;
    M418DllActorState *actorState;
    M418DllPlayerState *playerState2;
    M418DllPlayerSequence *playerSequence;
    s32 stateValue;
    OMOBJ *gameObject;
    s32 status;
    M418DllCameraState *cameraState;

    playerSequence = &gPlayerSequence;
    for (index = 0; index < 3; index++) {
        playerState = &playerStates[playerSequence->playerIndices[index]];
        if (playerState->targetIndex == 0) {
            if ((rand8() % 5) == 0) {
                stateValue = 1;
            }
            else {
                stateValue = 0;
            }
            if (stateValue || (AdvanceSequenceTimer(&playerActionSequence, 0) >= 5)) {
                playerState->targetIndex = 1;
                gameObject = playerState->object;
                playerState2 = &playerStates[gameObject->work[0]];
                if (playerState2->actionState == 0) {
                    playerState2->actionState = 1;
                }
                CharFXPlay(playerState->character, 0x122);
            }
        }
        else {
            MoveObjectAlongPath(playerState->object, playerTargetPositions[index], 0.0f, 9.0f);
        }
    }
    if (AdvanceSequenceTimer(&playerActionSequence, 0) >= 0x3C) {

        for (index = 0; index < 3; index++) {
            actorState = &actorStates[index];
            if (actorState->actionState == 0) {
                if ((rand8() % 5) == 0) {
                    status = 1;
                }
                else {
                    status = 0;
                }
                if ((status != 0) || (AdvanceSequenceTimer(&playerActionSequence, 0) >= 0x41)) {
                    HuAudFXPlay(0x5D3);
                    actorState->actionState = 1;
                }
            }
            else {
                actorState->rotationX -= 2.5f;
                if (actorState->rotationX <= 0.0f) {
                    actorState->rotationX = 0.0f;
                }
            }
        }
    }
    if (AdvanceSequenceTimer(&playerActionSequence, 0) == 0x3C) {
        SetPlayerSequenceStage(2);
    }
    if (AdvanceSequenceTimer(&playerActionSequence, 1) != 0) {
        return AdvanceSequence(&playerActionSequence);
    }
    return 0;
}

M418DllSequenceEntry playerActionEntries[3] = { { StartPlayerSequenceStage, UpdatePlayerSequenceStage, 0x5A }, { StartPlayerMovementStage, UpdatePlayerMovementStage, 0xB4 }, { NULL, NULL, -1 } };

void InitializePlayerActions(void)
{
    InitializeSequence(&playerActionSequence, &playerActionEntries[0]);
}

s32 IsPlayerActionsComplete(void)
{
    s32 result;

    result = 0;
    if (playerActionSequence.activeCallback != NULL) {
        playerActionSequence.activeCallback();
        playerActionSequence.activeCallback = NULL;
    }
    if (playerActionSequence.activeResultCallback != NULL) {
        result = playerActionSequence.activeResultCallback();
    }
    return result;
}

s32 ReadTargetSelectionInput(OMOBJ *object, s32 object)
{
    M418DllPlayerState *playerState;
    s32 result;

    playerState = &playerStates[object->work[0]];
    result = playerState->targetIndex;
    if (playerState->isComputer != 0) {
        switch (playerState->difficulty) {
            case 0:
                if ((object % 100) == 0) {
                    result = rand8() % 4;
                }
                break;
            case 1:
                if ((object % 50) == 0) {
                    result = rand8() % 4;
                }
                break;
            case 2:
                if ((object % 25) == 0) {
                    result = rand8() % 4;
                }
                break;
            case 3:
                if ((object % 25) == 0) {
                    result = rand8() % 4;
                }
                break;
        }
    }
    else {
        if ((HuPadBtnDown[playerState->controllerPort] & PAD_BUTTON_A) != 0) {
            result = 1;
        }
        if ((HuPadBtnDown[playerState->controllerPort] & PAD_BUTTON_B) != 0) {
            result = 0;
        }
        if ((HuPadBtnDown[playerState->controllerPort] & PAD_BUTTON_X) != 0) {
            result = 3;
        }
        if ((HuPadBtnDown[playerState->controllerPort] & PAD_BUTTON_Y) != 0) {
            result = 2;
        }
    }
    return result;
}

s32 SelectAndReserveTarget(OMOBJ *object, s32 object)
{
    M418DllPlayerState *playerState;
    M418DllActorState *actorState;
    M418DllActorState *actorState2;
    M418DllActorState *actorState3;
    s32 playerIndex;
    s32 result;

    playerState = &playerStates[object->work[0]];
    result = playerState->targetIndex;
    if (playerState->isComputer != 0) {
        switch (playerState->difficulty) {
            case 0:
                if ((object % 200) == 1) {
                    result = rand8() % 4;
                }
                break;
            case 1:
                if ((object % 150) == 1) {
                    result = rand8() % 4;
                }
                break;
            case 2:
                if ((object % 100) == 1) {
                    result = rand8() % 4;
                }
                break;
            case 3:
                if ((object % 100) == 1) {
                    result = rand8() % 4;
                }
                break;
        }
        if (result != -1) {
            actorState = &actorStates[result];
            if (actorState->jumpVelocityValue == 1) {
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    actorState2 = &actorStates[playerIndex];
                    if (actorState2->jumpVelocityValue != 1) {
                        result = playerIndex;
                        actorState2->jumpVelocityValue = 1;
                        break;
                    }
                }
            }
            else {
                actorState->jumpVelocityValue = 1;
            }
        }
    }
    else {
        if ((HuPadBtnDown[playerState->controllerPort] & PAD_BUTTON_A) != 0) {
            result = 1;
        }
        if ((HuPadBtnDown[playerState->controllerPort] & PAD_BUTTON_B) != 0) {
            result = 0;
        }
        if ((HuPadBtnDown[playerState->controllerPort] & PAD_BUTTON_X) != 0) {
            result = 3;
        }
        if ((HuPadBtnDown[playerState->controllerPort] & PAD_BUTTON_Y) != 0) {
            result = 2;
        }
        if (result != -1) {
            actorState3 = &actorStates[result];
            if (actorState3->jumpVelocityValue == 1) {
                result = -1;
            }
            else {
                actorState3->jumpVelocityValue = 1;
            }
        }
    }
    return result;
}

void SetOverviewCamera(void)
{
    M418DllCameraState *cameraState = &gCameraState;
    Vec cameraPosition = { 0.0f, 1100.0f, 1800.0f };
    Vec focusPosition = { 0.0f, 600.0f, 600.0f };

    SetCameraTargetFromPositions(cameraState, cameraPosition, focusPosition);
    ApplyCameraTarget(cameraState);
    cameraState->rot.x++;
}

void TransitionToWideCamera(void)
{
    M418DllCameraState *cameraState = &gCameraState;
    M418DllCameraState cameraState2;
    Vec cameraPosition = { 0.0f, 1800.0f, 480.0f };
    Vec focusPosition = { 0.0f, 600.0f, 250.0f };

    SetCameraTargetFromPositions(&cameraState2, cameraPosition, focusPosition);
    ApplyCameraTarget(&cameraState2);
    cameraState2.zoom = 750.0f;
    TransitionCameraStateEaseOut(cameraState, &cameraState2, cameraState->interpolationFactor++, 120.0f, 10.0f);
}

void TransitionToStageCamera(void)
{
    M418DllCameraState *cameraState = &gCameraState;
    M418DllCameraState cameraState2;
    Vec focusPosition = { 0.0f, 980.0f, 1330.0f };
    Vec focusPosition2 = { 0.0f, 480.0f, 130.0f };

    SetCameraTargetFromPositions(&cameraState2, focusPosition, focusPosition2);
    ApplyCameraTarget(&cameraState2);
    TransitionCameraStateEaseOut(cameraState, &cameraState2, cameraState->interpolationFactor++, 30.0f, 10.0f);
}

Vec playerCameraPositions[8] = { { -540.0f, 225.0f, -900.0f }, { -180.0f, 225.0f, -900.0f }, { 180.0f, 225.0f, -900.0f }, { 540.0f, 225.0f, -900.0f },
    { -360.0f, 0.0f, -680.0f }, { 0.0f, 0.0f, -680.0f }, { 360.0f, 0.0f, -680.0f }, { 0.0f, 0.0f, 0.0f } };

void TransitionToPlayerCamera(void)
{
    M418DllCameraState *cameraState;
    M418DllPlayerState *playerState;
    M418DllPlayerSequence *playerSequence;

    playerSequence = &gPlayerSequence;
    playerState = &playerStates[playerSequence->activePlayerIndex];
    cameraState = &gCameraState;
    cameraState->center.x = playerCameraPositions[playerState->targetIndex].x;
    cameraState->center.y = playerCameraPositions[playerState->targetIndex].y;
    cameraState->center.z = playerCameraPositions[playerState->targetIndex].z;
    cameraState->rot.x = 0.0f;
    cameraState->rot.y = 0.0f;
    cameraState->rot.z = 0.0f;
    cameraState->zoom = 725.0f;
    TransitionCameraStateSine(cameraState, cameraState, cameraState->interpolationFactor++, 30.0f, 5.0f);
}

void SelectCameraTransition(s32 cameraStage)
{
    M418DllCameraState *cameraState = &gCameraState;

    ResetCameraTarget(cameraState);
    switch (cameraStage) {
        case 0:
            SetCameraUpdateCallback(SetOverviewCamera);
            break;
        case 1:
            SetCameraUpdateCallback(TransitionToWideCamera);
            break;
        case 2:
            SetCameraUpdateCallback(TransitionToStageCamera);
            break;
        case 3:
            SetCameraUpdateCallback(TransitionToPlayerCamera);
            break;
    }
}

void BeginPlayerSelection(s32 )
{
    s32 index;
    M418DllPlayerSequence *playerSequence = &gPlayerSequence;
    M418DllPlayerState *playerState;
    M418DllActorState *actorState;
    M418DllCameraState *cameraState;

    HuAudAUXVolSet(0x40, 0x40);

    for (index = 0; index < 3; index++) {
        playerState = &playerStates[playerSequence->playerIndices[index]];
        playerState->targetIndex = -1;
    }

    for (index = 0; index < 3; index++) {
        actorState = &playerActorStates[index];
        SetModelMotion(actorState->object, 0, 3, 30, 1);
    }
    if (menuState == 0) {
        menuState = 1;
        menuTimer = REFRESH_RATE * 5;
    }

    SelectCameraTransition(0);
}

Vec selectionLanePositions[4] = { { -540.0f, 115.0f, -700.0f }, { -180.0f, 115.0f, -700.0f }, { 180.0f, 115.0f, -700.0f }, { 540.0f, 115.0f, -700.0f } };

s32 UpdatePlayerSelection(s32 )
{
    Vec position;
    M418DllPlayerState *playerState;
    M418DllPlayerState *playerState2;
    s32 index;
    s32 stateValue;
    M418DllPlayerSequence *playerSequence;
    M418DllActorState *actorState;
    M418DllCameraState *cameraState;

    playerSequence = &gPlayerSequence;

    for (index = 0; index < 3; index++) {
        playerState = &playerStates[playerSequence->playerIndices[index]];
        stateValue = AdvanceSequenceTimer(&cameraActionSequence, 0);
        playerState->targetIndex = ReadTargetSelectionInput(playerState->object, stateValue);
        if (playerState->targetIndex != -1) {
            position.x = selectionLanePositions[playerState->targetIndex].x;
            position.y = selectionLanePositions[playerState->targetIndex].y;
            position.z = (75.0f * index) + selectionLanePositions[playerState->targetIndex].z;
            MoveObjectToPosition(playerState->object, position, -1.0f, 9.0f);
            if (playerState->object->trans.z <= -720.0f) {
                playerState->object->trans.z = -720.0f;
            }
            if (playerState->object->trans.z >= -480.0f) {
                playerState->object->trans.z = -480.0f;
            }
            if (AdvanceSequenceTimer(&cameraActionSequence, 0) >= REFRESH_RATE * 4) {
                playerState->object->trans.x = playerState->object->trans.y = -5000.0f;
                SetModelMotion(playerState->object, 0, 0, 0xA, 1);
            }
        }
    }

    for (index = 0; index < 3; index++) {
        actorState = &playerActorStates[index];
#if VERSION_PAL
        actorState->object->rot.y += 1.44f;
#else
        actorState->object->rot.y += 1.2f;
#endif
    }
    if (AdvanceSequenceTimer(&cameraActionSequence, 0) == REFRESH_RATE) {
        SelectCameraTransition(1);
    }
    if (AdvanceSequenceTimer(&cameraActionSequence, 1) != 0) {
        return AdvanceSequence(&cameraActionSequence);
    }
    return 0;
}

void BeginSelectionResult(s32 )
{
    s32 index;
    M418DllActorState *actorState;

    for (index = 0; index < 3; index++) {
        actorState = &playerActorStates[index];
        actorState->object->rot.y = 0.0f;
        SetModelMotion(actorState->object, 0, 0, 0x1E, 1);
    }
    SelectCameraTransition(2);
}

s32 UpdateSelectionResult(s32 )
{
    if (AdvanceSequenceTimer(&cameraActionSequence, 1) != 0) {
        return AdvanceSequence(&cameraActionSequence);
    }
    return 0;
}

void CountRemainingPlayers(s32 )
{
    M418DllPlayerSequence *playerSequence = &gPlayerSequence;
    s32 index;
    M418DllPlayerState *playerState;

    playerSequence->playerCount = 0;

    for (index = 0; index < 3; index++) {
        playerState = &playerStates[playerSequence->playerIndices[index]];
        if (playerState->targetIndex == -1) {
            playerSequence->playerCount++;
            playerState->actionTimer = 0;
        }
    }
}

s32 UpdateRemainingPlayerReactions(s32 )
{
    M418DllPlayerState *playerState;
    M418DllAnimationState *animationState;
    M418DllPlayerSequence *playerSequence;
    M418DllAnimationState *animationState2;

    s32 modelIndex;
    s32 index;
    f32 interpolatedValue;

    animationState = &gAnimationState;
    playerSequence = &gPlayerSequence;

    if (playerSequence->playerCount == 0) {
        return SetSequenceIndex(&cameraActionSequence, 9);
    }

    for (index = 0; index < 3; index++) {
        playerState = &playerStates[playerSequence->playerIndices[index]];
        if (playerState->targetIndex == -1) {
            if (animationState->motionStates[index] == 0) {
                if ((rand8() % 10) == 0) {
                    modelIndex = 1;
                }
                else {
                    modelIndex = 0;
                }
                if ((modelIndex != 0) || (AdvanceSequenceTimer(&cameraActionSequence, 0) >= 10)) {
                    HuAudFXPlay(0x5DC);
                    CharFXPlay(playerState->character, 0x128);
                    animationState->motionStates[index] = 2;
                    Hu3DModelShadowDispOff(playerState->object->mdlId[0]);
                    SetModelMotion(playerState->object, 0, 7, 0x1E, 0);
                    playerState->actionTimer = AdvanceSequenceTimer(&cameraActionSequence, 0);
                    omVibrate(playerState->playerIndex, 0xC, 6, 6);
                }
            }
            else if (AdvanceSequenceTimer(&cameraActionSequence, 0) >= (playerState->actionTimer + 0x1E)) {
                playerState->object->trans.y -= 15.0f;
            }
            if (AdvanceSequenceTimer(&cameraActionSequence, 0) >= (playerState->actionTimer + 0x3C)) {
                interpolatedValue = InterpolateSineEaseInOut(-200.0f, -110.0f, AdvanceSequenceTimer(&cameraActionSequence, 0) - (playerState->actionTimer + 0x3C), 90.0f);
                Hu3DModelPosSet(animationState->object->mdlId[index + 8], -350.0f + (360.0f * index), interpolatedValue, -660.0f);
                Hu3DModelRotSet(animationState->object->mdlId[index + 8], 0.0f, 0.0f, 20.0f);
            }
        }
    }
    if (AdvanceSequenceTimer(&cameraActionSequence, 1) != 0) {
        animationState2 = &gAnimationState;
        animationState2->motionStates[0] = animationState2->motionStates[1] = animationState2->motionStates[2] = 3;
        if (playerSequence->playerCount >= 3) {
            return 1;
        }
        return SetSequenceIndex(&cameraActionSequence, 9);
    }
    else {
        return 0;
    }
}

void BeginTargetPresentation(s32 ) { }

s32 UpdateTargetPresentation(s32 )
{
    M418DllPlayerState *playerState;
    M418DllActorState *actorState;
    M418DllPlayerSequence *playerSequence;

    playerSequence = &gPlayerSequence;
    playerState = &playerStates[playerSequence->activePlayerIndex];
    actorState = &playerActorStates[playerSequence->sequenceState];
    if (AdvanceSequenceTimer(&cameraActionSequence, 0) < 0x3C) {
        if (((actorState->object->work[0] == 0) || (actorState->object->work[1] == 0x18)) && (actorState->object->rot.y > -50.0f)) {
            actorState->object->rot.y -= 5.0f;
            if (actorState->object->rot.y <= -50.0f) {
                actorState->object->rot.y = -50.0f;
            }
        }
        if (playerState->object->rot.y < 180.0f) {
            playerState->object->rot.y += 5.0f;
            if (actorState->object->rot.y <= -50.0f) {
                actorState->object->rot.y = -50.0f;
            }
        }
    }
    else if (AdvanceSequenceTimer(&cameraActionSequence, 0) == 0x3C) {
        HuAudFXPlay(0x40);
        SetModelMotion(actorState->object, 0, 2, 0xA, 0);
        SetModelMotionIfChanged(playerState->object, 0, 0, 0xA, 1);
        Hu3DMotionOverlaySet(playerState->object->mdlId[0], playerState->object->mtnId[9]);
        Hu3DMotionOverlaySpeedSet(playerState->object->mdlId[0], 1.0f);
        soundEffectIds[8] = HuAudFXPlay(0x5D9);
    }
    else if (AdvanceSequenceTimer(&cameraActionSequence, 0) <= 0xAA) {
        if (AdvanceSequenceTimer(&cameraActionSequence, 0) >= 0x5A) {
            playerState->visibilityAlpha += 0.025f;
            if (playerState->visibilityAlpha >= 1.0f) {
                playerState->visibilityAlpha = 1.0f;
            }
        }
    }
    else {
        Hu3DMotionOverlaySpeedSet(playerState->object->mdlId[0], -1.0f);
    }
    if (AdvanceSequenceTimer(&cameraActionSequence, 1) != 0) {
        return AdvanceSequence(&cameraActionSequence);
    }
    else {
        return 0;
    }
}

void BeginTargetSelection(s32 )
{
    M418DllPlayerSequence *playerSequence = &gPlayerSequence;
    M418DllPlayerState *playerState = &playerStates[playerSequence->activePlayerIndex];
    playerState->targetIndex = -1;
    if (menuState == 0) {
        menuState = 1;
        menuTimer = REFRESH_RATE * 5;
    }
}

s32 UpdateTargetSelection(s32 )
{
    Mtx objectMatrix;
    Mtx objectMatrix2;
    M418DllPlayerState *playerState;
    M418DllPlayerSequence *playerSequence;
    s32 modelIndex;

    playerSequence = &gPlayerSequence;
    playerState = &playerStates[playerSequence->activePlayerIndex];
    if (AdvanceSequenceTimer(&cameraActionSequence, 0) >= 10) {
        playerState->targetIndex = SelectAndReserveTarget(playerState->object, AdvanceSequenceTimer(&cameraActionSequence, 0));
        if (playerState->targetIndex != -1) {
            if (menuState == 2) {
                menuState = 3;
                menuTimer = 0x1E;
            }
            return AdvanceSequence(&cameraActionSequence);
        }
    }
    AdvanceSequenceTimer(&cameraActionSequence, 1);
    modelIndex = 0;
    if (menuState == 3) {
        modelIndex = 1;
    }
    if (modelIndex != 0) {
        Hu3DModelObjMtxGet(playerState->object->mdlId[0], characterJointNames[playerState->character], objectMatrix2);
        Hu3DModelObjMtxGet(playerState->object->mdlId[12], "g031i00-itemhook_eff", objectMatrix);
        Hu3DModelPosSet(playerState->object->mdlId[13], objectMatrix[0][3] + objectMatrix2[0][3], 80.0f + (objectMatrix[1][3] + objectMatrix2[1][3]), objectMatrix[2][3] + objectMatrix2[2][3]);
        Hu3DModelScaleSet(playerState->object->mdlId[13], 0.5f, 0.5f, 0.5f);
        Hu3DMotionSpeedSet(playerState->object->mdlId[13], 2.0f);
        SetModelMotion(playerState->object, 0xD, 0xD, 0, 0);
        return AdvanceSequence(&cameraActionSequence);
    }
    else {
        return 0;
    }
}

void BeginTargetSelectionResult(s32 )
{
    M418DllPlayerSequence *playerSequence;
    M418DllPlayerState *playerState;

    playerSequence = &gPlayerSequence;
    playerState = &playerStates[playerSequence->activePlayerIndex];
    if (playerState->targetIndex == -1) {
        HuAudFXFadeOut(soundEffectIds[8], 0x3E8);
    }
}

s32 UpdateTargetSelectionResult(s32 )
{
    M418DllPlayerState *playerState;
    M418DllPlayerSequence *playerSequence;

    playerSequence = &gPlayerSequence;
    playerState = &playerStates[playerSequence->activePlayerIndex];
    if (playerState->targetIndex == -1) {
        playerState->visibilityAlpha -= 0.025f;
        if (playerState->visibilityAlpha <= 0.0f) {
            playerState->visibilityAlpha = 0.0f;
        }
        if (AdvanceSequenceTimer(&cameraActionSequence, 1) != 0) {
            return AdvanceSequence(&cameraActionSequence);
        }
    }
    else {
        playerState->object->rot.y = InterpolateLinear(180.0f, 200.0f - (10.0f * playerState->targetIndex), AdvanceSequenceTimer(&cameraActionSequence, 0), 10.0f);
        if (AdvanceSequenceTimer(&cameraActionSequence, 0) == 0xA) {
            Hu3DMotionOverlaySet(playerState->object->mdlId[0], playerState->object->mtnId[10]);
            Hu3DMotionOverlaySpeedSet(playerState->object->mdlId[0], 1.0f);
            playerState->completionState = 1;
        }
        if (AdvanceSequenceTimer(&cameraActionSequence, 1) != 0) {
            Hu3DMotionOverlaySpeedSet(playerState->object->mdlId[0], -1.0f);
            playerState->completionState = 2;
            return AdvanceSequence(&cameraActionSequence);
        }
    }
    return 0;
}

void BeginTargetReaction(s32 )
{
    M418DllPlayerSequence *playerSequence;
    M418DllPlayerState *playerState;
    M418DllActorState *actorState;
    s32 index;
    M418DllPlayerState *playerState2;
    s32 count;

    count = 0;
    playerSequence = &gPlayerSequence;
    playerState = &playerStates[playerSequence->activePlayerIndex];
    if (playerState->targetIndex != -1) {
        playerSequence->secondaryPlayerIndices[0] = playerSequence->secondaryPlayerIndices[1] = playerSequence->secondaryPlayerIndices[2] = -1;

        for (index = 0; index < 3; index++) {
            playerState2 = &playerStates[playerSequence->playerIndices[index]];
            if (playerState->targetIndex == playerState2->targetIndex) {
                playerSequence->secondaryPlayerIndices[count++] = playerState2->object->work[0];
            }
        }
        actorState = &actorStates[playerState->targetIndex];
        actorState->actionState = 0;
        if (playerSequence->secondaryPlayerIndices[0] == -1) {
            actorState->effectState = 1;
        }
        actorState->object->work[1] = 1;
    }
}

s32 UpdateTargetReaction(s32 )
{
    Mtx objectMatrix;
    M418DllPlayerState *playerState;
    M418DllPlayerSequence *playerSequence;
    M418DllPlayerState *playerState2;
    M418DllActorState *actorState;
    s32 index;
    s32 count;
    M418DllCameraState *cameraState;

    count = -1;
    playerSequence = &gPlayerSequence;
    playerState2 = &playerStates[playerSequence->activePlayerIndex];
    actorState = &actorStates[playerState2->targetIndex];
    if (playerState2->targetIndex == -1) {
        return AdvanceSequence(&cameraActionSequence);
    }
    if (actorState->actionState == 1) {
        SelectCameraTransition(3);
    }
    if (actorState->actionState == 3) {

        for (index = 0; index < 3; index++) {
            if (playerSequence->secondaryPlayerIndices[index] != -1) {
                playerState = &playerStates[playerSequence->secondaryPlayerIndices[index]];
                Hu3DModelObjMtxGet(*actorState->object->mdlId, "taihou-null4", (f32(*)[4]) & objectMatrix[0]);
                omSetTra(playerState->object, objectMatrix[0][3], objectMatrix[1][3] - 25.0f, 50.0f + objectMatrix[2][3]);
                omSetRot(playerState->object, 0.0f, 180.0f, 0.0f);
                SetModelMotionIfChanged(playerState->object, 0, 8, 0, 2);
                playerState->targetIndex = -1;
                playerState->effectState = 0;
                if (count >= 2) {
                    count = 1;
                }
                playerState->effectPhase = count++;
                playerState->effectTimer = 0.0f;
                omVibrate((s16)playerState2->playerIndex, 0xC, 4, 2);
            }
        }
        return AdvanceSequence(&cameraActionSequence);
    }
    return 0;
}

void BeginSelectionTransition(s32 )
{
    s32 index;
    M418DllPlayerState *playerState;
    M418DllPlayerSequence *playerSequence;
    M418DllActorState *actorState;

    playerSequence = &gPlayerSequence;
    playerState = &playerStates[playerSequence->activePlayerIndex];
    playerState->object->rot.y = 180.0f;
    playerState->visibilityAlpha = 0.0f;

    for (index = 0; index < 3; index++) {
        actorState = &playerActorStates[index];
        actorState->object->rot.y = 0.0f;
    }
}

s32 UpdateSelectionTransition(s32 )
{
    M418DllPlayerSequence *playerSequence;
    s32 index;
    s32 count;
    M418DllActorState *actorState;
    M418DllActorState *actorState2;
    M418DllActorState *actorState3;
    M418DllPlayerState *playerState;

    count = 0;
    playerSequence = &gPlayerSequence;
    actorState = &playerActorStates[0];
    actorState2 = &playerActorStates[1];
    actorState3 = &playerActorStates[2];
    if (playerSequence->sequenceState == 2) {
        actorState->object->work[1] = 0;
        actorState2->object->work[1] = 0;
        actorState3->object->work[1] = 3;
        return 1;
    }
    playerSequence->sequenceState++;

    for (index = 0; index < 3; index++) {
        playerState = &playerStates[playerSequence->playerIndices[index]];
        if (playerState->targetIndex != -1) {
            count++;
        }
    }
    if (count == 0) {
        return 1;
    }
    else {
        return AdvanceSequence(&cameraActionSequence);
    }
}

void BeginSelectionFinale(s32 )
{
    M418DllPlayerSequence *playerSequence;
    M418DllActorState *actorState;
    M418DllActorState *actorState2;
    M418DllActorState *actorState3;

    playerSequence = &gPlayerSequence;
    actorState = &playerActorStates[0];
    actorState2 = &playerActorStates[1];
    actorState3 = &playerActorStates[2];
    if (playerSequence->sequenceState == 1) {
        actorState->object->work[1] = 3;
        actorState2->object->work[1] = 2;
        actorState3->object->work[1] = 1;
    }
    else if (playerSequence->sequenceState == 2) {
        actorState->object->work[1] = 0;
        actorState2->object->work[1] = 3;
        actorState3->object->work[1] = 2;
    }
}

s32 UpdateSelectionFinale(s32 )
{
    M418DllPlayerState *playerState;
    M418DllPlayerSequence *playerSequence;
    M418DllActorState *actorState;

    playerSequence = &gPlayerSequence;
    playerState = &playerStates[playerSequence->activePlayerIndex];
    if (playerState->targetIndex == -1) {
        if (AdvanceSequenceTimer(&cameraActionSequence, 0) == 30) {
            SelectCameraTransition(2);
        }
        if (AdvanceSequenceTimer(&cameraActionSequence, 0) == 120) {
            return SetSequenceIndex(&cameraActionSequence, 9);
        }
    }
    else {
        actorState = &actorStates[playerState->targetIndex];
        if (actorState->effectState != 0) {
            if (AdvanceSequenceTimer(&cameraActionSequence, 0) == 210) {
                SelectCameraTransition(2);
            }
            if (AdvanceSequenceTimer(&cameraActionSequence, 0) == 300) {
                return SetSequenceIndex(&cameraActionSequence, 9);
            }
        }
        else {
            if (AdvanceSequenceTimer(&cameraActionSequence, 0) == 30) {
                SelectCameraTransition(2);
            }
            if (AdvanceSequenceTimer(&cameraActionSequence, 0) == 120) {
                return SetSequenceIndex(&cameraActionSequence, 3);
            }
        }
    }
    AdvanceSequenceTimer(&cameraActionSequence, 1);
    return 0;
}

void BeginSelectionOutro(s32 ) { }

s32 UpdateSelectionOutro(s32 )
{
    M418DllPlayerState *playerState;
    M418DllActorState *actorState;
    M418DllPlayerSequence *playerSequence;

    playerSequence = &gPlayerSequence;
    playerState = &playerStates[playerSequence->activePlayerIndex];
    actorState = &playerActorStates[playerSequence->sequenceState];
    if (AdvanceSequenceTimer(&cameraActionSequence, 0) < 0x3C) {
        if (((actorState->object->work[0] == 0) || (actorState->object->work[1] == 0x18)) && (actorState->object->rot.y > -50.0f)) {
            actorState->object->rot.y -= 5.0f;
            if (actorState->object->rot.y <= -50.0f) {
                actorState->object->rot.y = -50.0f;
            }
        }
        if (playerState->object->rot.y < 180.0f) {
            playerState->object->rot.y += 5.0f;
            if (actorState->object->rot.y <= -50.0f) {
                actorState->object->rot.y = -50.0f;
            }
        }
    }
    else if (AdvanceSequenceTimer(&cameraActionSequence, 0) == 0x3C) {
        HuAudFXPlay(0x40);
        SetModelMotion(actorState->object, 0, 2, 0xA, 0);
        SetModelMotionIfChanged(playerState->object, 0, 0, 0xA, 1);
        Hu3DMotionOverlaySet(playerState->object->mdlId[0], playerState->object->mtnId[9]);
        Hu3DMotionOverlaySpeedSet(playerState->object->mdlId[0], 1.0f);
        soundEffectIds[8] = HuAudFXPlay(0x5D9);
    }
    else if (AdvanceSequenceTimer(&cameraActionSequence, 0) <= 0xAA) {
        if (AdvanceSequenceTimer(&cameraActionSequence, 0) >= 0x5A) {
            playerState->visibilityAlpha += 0.025f;
            if (playerState->visibilityAlpha >= 1.0f) {
                playerState->visibilityAlpha = 1.0f;
            }
        }
    }
    else {
        Hu3DMotionOverlaySpeedSet(playerState->object->mdlId[0], -1.0f);
    }
    if (AdvanceSequenceTimer(&cameraActionSequence, 1) != 0) {
        return SetSequenceIndex(&cameraActionSequence, 4);
    }
    else {
        return 0;
    }
}

M418DllSequenceEntry selectionSequenceEntries[11] = { { BeginPlayerSelection, UpdatePlayerSelection, VERSION_PAL ? 250 : 300 }, { BeginSelectionResult, UpdateSelectionResult, 60 }, { CountRemainingPlayers, UpdateRemainingPlayerReactions, 180 },
    { BeginTargetPresentation, UpdateTargetPresentation, 300 }, { BeginTargetSelection, UpdateTargetSelection, VERSION_PAL ? 250 : 300 }, { BeginTargetSelectionResult, UpdateTargetSelectionResult, 60 }, { BeginTargetReaction, UpdateTargetReaction, 180 },
    { BeginSelectionTransition, UpdateSelectionTransition, -1 }, { BeginSelectionFinale, UpdateSelectionFinale, 300 }, { BeginSelectionOutro, UpdateSelectionOutro, 240 }, { NULL, NULL, -1 } };

void InitializeSelectionSequence(void)
{
    InitializeSequence(&cameraActionSequence, &selectionSequenceEntries[0]);
}

s32 IsSelectionSequenceComplete(void)
{
    s32 result;

    result = 0;
    if (cameraActionSequence.activeCallback != NULL) {
        cameraActionSequence.activeCallback();
        cameraActionSequence.activeCallback = NULL;
    }
    if (cameraActionSequence.activeResultCallback != NULL) {
        result = cameraActionSequence.activeResultCallback();
    }
    return result;
}

void SetSelectionCameraWide(void)
{
    M418DllCameraState targetCameraState;
    M418DllCameraState *cameraState = &gCameraState;
    Vec cameraPosition = { 0.0f, 1100.0f, 1800.0f };
    Vec focusPosition = { 0.0f, 600.0f, 600.0f };

    SetCameraTargetFromPositions(&targetCameraState, cameraPosition, focusPosition);
    ApplyCameraTarget(&targetCameraState);
    targetCameraState.rot.x++;
    TransitionCameraStateSine(cameraState, &targetCameraState, cameraState->interpolationFactor++, 60.0f, 10.0f);
}

void SetSelectionCameraClose(void)
{
    M418DllCameraState targetCameraState;
    f32 interpolatedValue;

    M418DllCameraState *cameraState = &gCameraState;

    targetCameraState.center.x = targetCameraState.center.y = 0.0f;
    targetCameraState.center.z = -500.0f;
    targetCameraState.rot.x = -20.0f;
    targetCameraState.rot.y = targetCameraState.rot.z = 0.0f;
    targetCameraState.zoom = 1000.0f;
    TransitionCameraStateSine(cameraState, &targetCameraState, cameraState->interpolationFactor++, 10.0f, 5.0f);
}

void SetSelectionCameraResults(void)
{
    M418DllCameraState targetCameraState;
    M418DllCameraState *cameraState;

    cameraState = &gCameraState;

    targetCameraState.center.x = cameraState->center.x;
    targetCameraState.center.y = cameraState->center.y;
    targetCameraState.center.z = cameraState->center.z;
    targetCameraState.rot.x = cameraState->rot.x;
    targetCameraState.rot.y = cameraState->rot.y;
    targetCameraState.rot.z = cameraState->rot.z;
    targetCameraState.zoom = 500.0f;
    TransitionCameraStateSine(cameraState, &targetCameraState, cameraState->interpolationFactor++, 10.0f, 5.0f);
}

void SelectSelectionCamera(s32 cameraStage)
{
    M418DllCameraState *cameraState;

    cameraState = &gCameraState;

    ResetCameraTarget(cameraState);
    switch (cameraStage) {
        case 0:
            SetCameraUpdateCallback(SetSelectionCameraWide);
            break;
        case 1:
            SetCameraUpdateCallback(SetSelectionCameraClose);
            break;
        case 2:
            SetCameraUpdateCallback(SetSelectionCameraResults);
            break;
    }
}

void RestoreAudioVolume(s32 )
{
    HuAudAUXVolSet(-1, -1);
}

s32 BeginResultsSequence(s32 )
{
    s32 index;
    M418DllPlayerSequence *playerSequence;
    s32 resultStageIndex;
    M418DllPlayerState *playerState;
    M418DllPlayerState *activePlayerState;

    resultStageIndex = 5;
    playerSequence = &gPlayerSequence;

    SelectSelectionCamera(0);
    activePlayer = 0;

    for (index = 0; index < 3; index++) {
        playerState = &playerStates[playerSequence->playerIndices[index]];
        if (playerState->targetIndex != -1) {
            resultStageIndex = 1;
            activePlayer = 1;
        }
    }
    if (resultStageIndex == 5) {
        activePlayerState = &playerStates[playerSequence->activePlayerIndex];
        activePlayerState->object->rot.y = 0.0f;
    }
    SetSequenceIndex(&menuActionSequence, resultStageIndex);
}

void PrepareResultActors(s32 )
{
    M418DllPlayerSequence *playerSequence;
    s32 playerIndex;
    s32 count;
    M418DllPlayerState *playerState;
    M418DllActorState *actorState;

    count = 0;
    playerSequence = &gPlayerSequence;
    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        playerState = &playerStates[playerSequence->playerIndices[playerIndex]];
        if (playerState->targetIndex != -1) {
            playerSequence->targetIndices[count] = playerSequence->orderedPlayerIndices[count] = playerState->targetIndex;
            count++;
        }
    }
    switch (count) { /* irregular */
        case 1:
            playerSequence->targetIndices[0] = 3;
            break;
        case 2:
            playerSequence->targetIndices[0] = 4;
            playerSequence->targetIndices[1] = 5;
            break;
        case 3:
            playerSequence->targetIndices[0] = 0;
            playerSequence->targetIndices[1] = 1;
            playerSequence->targetIndices[2] = 2;
            break;
    }
    if (playerSequence->orderedPlayerIndices[0] == playerSequence->orderedPlayerIndices[1]) {
        playerSequence->orderedPlayerIndices[1] = -1;
    }
    if (playerSequence->orderedPlayerIndices[0] == playerSequence->orderedPlayerIndices[1]) {
        playerSequence->orderedPlayerIndices[1] = -1;
    }
    if (playerSequence->orderedPlayerIndices[1] == playerSequence->orderedPlayerIndices[2]) {
        playerSequence->orderedPlayerIndices[2] = -1;
    }

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        actorState = &actorStates[playerIndex];
        actorState->actionState = 0;
    }
}

s32 UpdateResultActorAnimation(s32 )
{
    M418DllActorState *actorState;
    s32 index;
    M418DllPlayerSequence *playerSequence;
    s32 stateValue;

    playerSequence = &gPlayerSequence;
    for (index = 0; index < 3; index++) {
        if (playerSequence->orderedPlayerIndices[index] != -1) {
            actorState = &actorStates[playerSequence->orderedPlayerIndices[index]];
            if (actorState->actionState == 0) {
                if ((rand8() % 5) == 0) {
                    stateValue = 1;
                }
                else {
                    stateValue = 0;
                }
                if ((stateValue != 0) || (AdvanceSequenceTimer(&menuActionSequence, 0) >= 5)) {
                    actorState->actionState = 1;
                    HuAudFXPlay(0x5D3);
                }
            }
            else {
                actorState->rotationX += 5.0f;
                if (actorState->rotationX >= 90.0f) {
                    actorState->rotationX = 90.0f;
                }
            }
        }
    }
    if (AdvanceSequenceTimer(&menuActionSequence, 1) != 0) {
        return AdvanceSequence(&menuActionSequence);
    }
    else {
        return 0;
    }
}

void BeginResultsPresentation(s32 )
{
    M418DllPlayerState *playerState;
    s32 index;
    M418DllPlayerSequence *playerSequence;
    M418DllPlayerState *playerState2;
    s32 count;
    s32 modelIndex;

    count = 0;
    playerSequence = &gPlayerSequence;
    for (index = 0; index < 3; index++) {
        playerState = &playerStates[playerSequence->playerIndices[index]];
        if (playerState->targetIndex != -1) {
            omSetRot(playerState->object, 0.0f, 0.0f, 0.0f);
            playerState->actionTimer = count * -10;
            playerState->effectStatus = 0;
            playerState->actionOrder = count++;
        }
    }
    playerState2 = &playerStates[playerSequence->activePlayerIndex];
    Hu3DMotionOverlayReset(*playerState2->object->mdlId);
    playerState2->object->work[1] = 1;
}

Vec resultActorPositions[4] = { { -540.0f, 225.0f, -900.0f }, { -180.0f, 225.0f, -900.0f }, { 180.0f, 225.0f, -900.0f }, { 540.0f, 225.0f, -900.0f } };

Vec resultTargetPositions[6] = { { -200.0f, 0.0f, -450.0f }, { 0.0f, 0.0f, -400.0f }, { 200.0f, 0.0f, -450.0f }, { 0.0f, 0.0f, -400.0f },
    { -100.0f, 0.0f, -425.0f }, { 100.0f, 0.0f, -425.0f } };

s32 UpdateResultsPresentation(s32 )
{
    s32 stateValue;
    s32 stateValue2;
    s32 stateValue3;
    M418DllPlayerState *playerState;
    M418DllPlayerSequence *playerSequence;
    s32 index;
    M418DllPlayerState *playerState2;
    M418DllPlayerState *playerState3;
    OMOBJ *gameObject;
    s32 stateValue4;
    void *unreferencedPointer;

    playerSequence = &gPlayerSequence;

    for (index = 0; index < 3; index++) {
        playerState = &playerStates[playerSequence->playerIndices[index]];
        if (playerState->targetIndex != -1) {
            if (playerState->effectStatus == 0) {
                if (playerState->actionTimer++ == 0) {
                    playerState->effectStatus = 1;
                    omSetTra(playerState->object, resultActorPositions[playerState->targetIndex].x, resultActorPositions[playerState->targetIndex].y,
                        resultActorPositions[playerState->targetIndex].z - 50.0f);
                    gameObject = playerState->object;
                    playerState2 = &playerStates[gameObject->work[0]];
                    if (playerState2->actionState == 0) {
                        playerState2->actionState = 1;
                    }
                }
            }
            else if (playerState->effectStatus == 1) {
                MoveObjectAlongPath(playerState->object, resultTargetPositions[playerSequence->targetIndices[playerState->actionOrder]], 0.0f, 12.0f);
            }
        }
    }
    playerState3 = &playerStates[playerSequence->activePlayerIndex];
    if (AdvanceSequenceTimer(&menuActionSequence, 0) == 0x1E) {
        SetModelMotion(playerState3->object, 0, 0, 0xA, 1);
    }
    if (AdvanceSequenceTimer(&menuActionSequence, 1) != 0) {
        return AdvanceSequence(&menuActionSequence);
    }
    else {
        return 0;
    }
}

void ShowResultsSequence(s32 )
{
    s32 playerIndices[3];
    M418DllPlayerState *playerState;
    s32 index;
    M418DllPlayerSequence *playerSequence;
    M418DllPlayerState *playerState2;

    playerSequence = &gPlayerSequence;
    for (index = 0; index < 3; index++) {
        playerState = &playerStates[playerSequence->playerIndices[index]];
        if (playerState->targetIndex != -1) {
            SetModelMotion(playerState->object, 0, 5, 10, 0);
        }
        playerIndices[index] = playerState->character;
    }
    playerState2 = &playerStates[playerSequence->activePlayerIndex];
    SetModelMotion(playerState2->object, 0, 6, 10, 0);
    SelectSelectionCamera(1);
    minigameSequenceId = MGSeqCreate(5, 3, playerIndices[0], playerIndices[1], playerIndices[2], -1);
    HuAudSStreamPlay(1);
}

s32 UpdateResultsSequence(s32 )
{
    if (AdvanceSequenceTimer(&menuActionSequence, 1) != 0) {
        return AdvanceSequence(&menuActionSequence);
    }
    else {
        return 0;
    }
}

void ResetWinnerMotion(s32 )
{
    M418DllPlayerState *playerState;
    M418DllPlayerSequence *playerSequence;

    playerSequence = &gPlayerSequence;
    playerState = &playerStates[playerSequence->activePlayerIndex];
    Hu3DMotionOverlayReset(playerState->object->mdlId[0]);
    playerState->object->work[1] = 1;
}

s32 UpdateWinnerReveal(s32 )
{
    M418DllPlayerSequence *playerSequence;
    M418DllPlayerState *playerState;

    playerSequence = &gPlayerSequence;

    playerState = &playerStates[playerSequence->activePlayerIndex];
    if (AdvanceSequenceTimer(&menuActionSequence, 0) == 0x1E) {
        SetModelMotion(playerState->object, 0, 0, 0xA, 1);
    }
    if (AdvanceSequenceTimer(&menuActionSequence, 1) != 0) {
        return AdvanceSequence(&menuActionSequence);
    }
    else {
        return 0;
    }
}

void ShowWinnerResults(s32 )
{
    M418DllPlayerState *playerState;
    s32 index;
    M418DllPlayerSequence *playerSequence;
    M418DllActorState *actorState;

    playerSequence = &gPlayerSequence;
    playerState = &playerStates[playerSequence->activePlayerIndex];
    Hu3DMotionOverlayReset(playerState->object->mdlId[0]);
    SetModelMotion(playerState->object, 0, 5, 10, 0);

    for (index = 0; index < 3; index++) {
        actorState = &playerActorStates[index];
        SetModelMotion(actorState->object, 0, 3, 10, 1);
    }
    SelectSelectionCamera(2);
    minigameSequenceId = MGSeqCreate(5, 3, playerState->character, -1, -1, -1);
    HuAudSStreamPlay(1);
}

s32 UpdateWinnerResults(s32 )
{
    if (AdvanceSequenceTimer(&menuActionSequence, 1) != 0) {
        return AdvanceSequence(&menuActionSequence);
    }
    else {
        return 0;
    }
}

M418DllSequenceEntry resultsSequenceEntries[8] = { { RestoreAudioVolume, BeginResultsSequence, -1 }, { PrepareResultActors, UpdateResultActorAnimation, 90 }, { BeginResultsPresentation, UpdateResultsPresentation, 180 },
    { ShowResultsSequence, UpdateResultsSequence, 210 }, { NULL, NULL, -1 }, { ResetWinnerMotion, UpdateWinnerReveal, 180 }, { ShowWinnerResults, UpdateWinnerResults, 240 }, { NULL, NULL, -1 } };

void InitializeResultsSequence(void)
{
    InitializeSequence(&menuActionSequence, &resultsSequenceEntries[0]);
}

s32 UpdateResultsCallbacks(void)
{
    s32 result;

    result = 0;
    if (menuActionSequence.activeCallback != NULL) {
        menuActionSequence.activeCallback();
        menuActionSequence.activeCallback = NULL;
    }
    if (menuActionSequence.activeResultCallback != NULL) {
        result = menuActionSequence.activeResultCallback();
    }
    return result;
}

void ExitMinigame(OMOBJ *object)
{
    s32 index;
    M418DllPlayerSequence *playerSequence;
    M418DllPlayerState *playerState;
    M418DllPlayerState *playerState2;
    s32 stateValue;
    s32 stateValue2;

    if (WipeStatGet() == 0) {
        playerSequence = &gPlayerSequence;
        if (activePlayer != 0) {
            for (index = 0; index < 3; index++) {
                playerState = &playerStates[playerSequence->playerIndices[index]];
                stateValue = playerState->playerIndex;
                if (_CheckFlag(0x1000C) == 0) {
                    GWPlayer[stateValue].coin_win = 10;
                }
            }
        }
        else {
            playerState2 = &playerStates[playerSequence->activePlayerIndex];
            stateValue2 = playerState2->playerIndex;
            if (_CheckFlag(0x1000CU) == 0) {
                GWPlayer[stateValue2].coin_win = 10;
            }
        }
        HuAudFadeOut(0x3E8);
        CharModelKill(-1);
        MGSeqKillAll();
        HuAudAllStop();
        omOvlReturnEx(1, 1);
        object->objFunc = NULL;
    }
}

void HandleSystemExit(OMOBJ *object)
{
    if (omSysExitReq == 1) {
        HuAudSeqAllFadeOut(0xFA);
        WipeCreate(2, 0, -1);
        object->objFunc = ExitMinigame;
    }
}

void InitializeMinigameState(void)
{
    M418DllPlayerState *playerState;
    s32 playerIndex;
    M418DllPlayerSequence *playerSequence;
    M418DllAnimationState *animationState;
    M418DllActorState *actorState;
    M418DllActorState *actorState2;
    M418DllPlayerState *playerState2;
    s32 modelIndex;
    s32 count;

    count = 0;
    animationState = &gAnimationState;
    for (playerIndex = 1; playerIndex < 4; playerIndex++) {
        SetModelMotion(animationState->object, playerIndex, playerIndex, 0, 2);
    }

    for (playerIndex = 8; playerIndex < 11; playerIndex++) {
        SetModelMotion(animationState->object, playerIndex, playerIndex, 0, 1);
        Hu3DModelPosSet(animationState->object->mdlId[playerIndex + 8], -360.0f + (360.0f * (playerIndex - 8)), -200.0f, -660.0f);
        Hu3DModelRotSet(animationState->object->mdlId[playerIndex], 0.0f, 0.0f, 20.0f);
    }

    animationState->motionStates[0] = animationState->motionStates[1] = animationState->motionStates[2] = animationState->motionStates[3] = 0;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        actorState = &actorStates[playerIndex];
        actorState->object->work[1] = actorState->object->work[2] = 0;
        actorState->rotationX = 0.0f;
        actorState->actionState = actorState->effectState = actorState->jumpStateValue = actorState->jumpVelocityValue = 0;
    }

    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        actorState2 = &playerActorStates[playerIndex];
        omSetTra(actorState2->object, movementWaypoints[playerIndex].x, movementWaypoints[playerIndex].y, movementWaypoints[playerIndex].z);
        omSetRot(actorState2->object, 0.0f, 0.0f, 0.0f);
        omSetSca(actorState2->object, 1.0f, 1.0f, 1.0f);
        SetModelMotionIfChanged(actorState2->object, 0, 0, 10, 1);
        actorState2->object->work[1] = -1U;
    }

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerState = &playerStates[playerIndex];
        if (playerState->team != 0) {
            omSetTra(playerState->object, 0.0f, -5000.0f, 0.0f);
        }
        else {
            omSetTra(playerState->object, 0.0f, 0.0f, 0.0f);
        }
        omSetRot(playerState->object, 0.0f, 0.0f, 0.0f);
        omSetSca(playerState->object, 1.0f, 1.0f, 1.0f);
        if (playerState->team == 0) {
            Hu3DModelAttrReset(playerState->object->mdlId[11], 1);
            Hu3DModelHookSet(playerState->object->mdlId[0], characterJointNames[playerState->character], playerState->object->mdlId[11]);
            Hu3DModelPosSet(playerState->object->mdlId[11], 0.0f, 0.0f, 0.0f);
            Hu3DModelRotSet(playerState->object->mdlId[11], 0.0f, 0.0f, 0.0f);
            Hu3DMotionOverlaySet(playerState->object->mdlId[0], playerState->object->mtnId[9]);
            Hu3DMotionOverlaySpeedSet(playerState->object->mdlId[0], 0.0f);
        }
        SetModelMotion(playerState->object, 0, 0, 0, 1);
        Hu3DModelShadowDispOn(playerState->object->mdlId[0]);
        playerState->targetIndex = playerState->actionTimer = playerState->effectStatus = playerState->actionOrder = playerState->completionState = 0;
        playerState->effectState = -1;
        playerState->effectTimer = playerState->objectScale = playerState->effectPhase = playerState->effectScale = playerState->effectRotation = 0.0f;
    }

    playerSequence = &gPlayerSequence;
    for (playerIndex = 0, count = 0; playerIndex < 4; playerIndex++) {
        playerState2 = &playerStates[playerIndex];
        if (playerState2->team != 0) {
            playerSequence->playerIndices[count++] = playerState2->playerIndex;
        }
        else {
            playerSequence->activePlayerIndex = playerState2->playerIndex;
        }
    }

    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        playerSequence->targetIndices[playerIndex] = playerSequence->orderedPlayerIndices[playerIndex] = -1;
        playerSequence->secondaryPlayerIndices[playerIndex] = -1;
    }

    playerSequence->playerCount = playerSequence->sequenceState = playerSequence->selectionState = playerSequence->roundState = 0;
    InitializePlayerActions();
    InitializeSelectionSequence();
    InitializeResultsSequence();

    for (playerIndex = 0; playerIndex < 10; playerIndex++) {
        soundEffectIds[playerIndex] = -1;
    }
    soundEffectIds[6] = HuAudFXPlay(0x5D7);
    soundEffectIds[7] = HuAudFXPlay(0x5D8);
}

void UpdateMinigameState(OMOBJ *object)
{
    switch (sequenceFrameCounter) {
        case 0:
            InitializeMinigameState();
            sequenceFrameCounter = 1;
            break;
        case 1:
            if (WipeStatGet() == 0) {
                sequenceFrameCounter = 2;
            }
            break;
        case 2:
            if (IsPlayerActionsComplete()) {
                sequenceFrameCounter = 3;
            }
            break;
        case 3:
            activeSequenceIndex = MGSeqCreate(3, 0);
            sequenceFrameCounter = 4;
            modelFileIds[1] = -1;
            break;
        case 4:
            if (((MGSeqStatGet(activeSequenceIndex) & 0x10) != 0) && (modelFileIds[1] == -1)) {
                modelFileIds[1] = HuAudSeqPlay(0x49);
            }
            if (MGSeqStatGet(activeSequenceIndex) == 0) {
                sequenceFrameCounter = 5;
            }
            break;
        case 5:
            if (IsSelectionSequenceComplete()) {
                sequenceFrameCounter = 6;
            }
            break;
        case 6:
            activeSequenceIndex = MGSeqCreate(3, 1);
            HuAudSeqPauseAll(0);
            HuAudSeqAllFadeOut(0x64);
            sequenceFrameCounter = 7;
            break;
        case 7:
            if (MGSeqStatGet(activeSequenceIndex) == 0) {
                sequenceFrameCounter = 8;
            }
            break;
        case 8:
            if (UpdateResultsCallbacks()) {
                sequenceFrameCounter = 9;
            }
            break;
        case 9:
            WipeCreate(2, 0, -1);
            object->objFunc = ExitMinigame;
            break;
        default:
            sequenceFrameCounter = 0;
            break;
    }
    if (omSysExitReq == 1) {
        HuAudSeqAllFadeOut(0xFA);
        WipeCreate(2, 0, -1);
        object->objFunc = ExitMinigame;
    }
}

void CreateMinigameObjects(OMOBJ *object)
{
    s32 playerIndex;
    M418DllActorState *actorState;
    M418DllActorState *actorState2;
    M418DllPlayerState *playerState;
    M418DllAnimationState *animationState;
    M418DllParticleWork *particleSystem;

    animationState = &gAnimationState;
    animationState->object = omAddObjEx(objectManagerProcess, 0x100, 0x14U, 0x14U, -1, CreateAnimationModels);

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        actorState = &actorStates[playerIndex];
        actorState->object = omAddObjEx(objectManagerProcess, 0x200, 0x14U, 0x14U, -1, CreatePlayerCharacter);
        actorState->object->work[0] = playerIndex;
    }

    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        actorState2 = &playerActorStates[playerIndex];
        actorState2->object = omAddObjEx(objectManagerProcess, 0x300, 0x14U, 0x14U, -1, CreatePlayerActor);
        actorState2->object->work[0] = playerIndex;
    }

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerState = &playerStates[playerIndex];
        playerState->object = omAddObjEx(objectManagerProcess, 0x400, 0x14U, 0x14U, -1, CreateCharacterObject);
        playerState->object->work[0] = playerIndex;
    }

    particleSystem = &gParticleWork;
    particleSystem->object = omAddObjEx(objectManagerProcess, 0x500, 0xBU, 0xBU, -1, UpdateParticleSystem);
    WipeCreate(1, 0, -1);
    object->objFunc = UpdateMinigameState;
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

void InitializeMinigame(void)
{
    objectManagerProcess = omInitObjMan(0x3E, 0x2000);
    omGameSysInit(objectManagerProcess);
    HuAudAUXVolSet(-1, -1);
    CreateMinigameLighting();
    InitializeCamera();
    InitializeShadow();
    InitializeMinigameSequences();
    omAddObjEx(objectManagerProcess, 0x1000, 0U, 0U, -1, CreateMinigameObjects);
    omAddObjEx(objectManagerProcess, 0x2000, 0U, 0U, -1, UpdateCountdownObject);
}

void ObjectSetup(void)
{
    InitializeMinigame();
}
