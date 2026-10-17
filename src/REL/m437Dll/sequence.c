#include "REL/m437Dll.h"
#include "game/hu3d.h"

#include "ext_math.h"
#include "string.h"

void LoadCurrentMotionSequenceEntry(MotionSequenceState* sequence);
s32 RewindMotionSequence(MotionSequenceState* sequence);
void UpdateModelPositionAndFacing(OMOBJ* object, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 turnSmoothing, s32 frame, s32 duration, s32 updatePosition, s32 updateFacing);
void InterpolateModelPosition(OMOBJ* object, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 turnSmoothing, s32 frame, s32 duration);
void InterpolateModelFacing(OMOBJ* object, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 turnSmoothing, s32 frame, s32 duration);
void InterpolateModelPositionAndFacing(OMOBJ* object, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 turnSmoothing, s32 frame, s32 duration);
void SnapCameraToTarget(CameraState* camera);
void BlendCameraTowardTarget(CameraState* camera, CameraState* targetCamera, float blendWeight);
void InterpolateCameraTarget(CameraState* sourceCamera, CameraState* targetCamera, float frame, float duration);
void BlendCameraTargetAndSnap(CameraState* camera, CameraState* targetCamera, float frame, float duration);
void EaseCameraTarget(CameraState* sourceCamera, CameraState* targetCamera, float frame, float duration);
void EaseCameraTargetAndSnap(CameraState* camera, CameraState* targetCamera, float frame, float duration);
void EaseCameraTargetWithShake(CameraState* camera, CameraState* targetCamera, float frame, float duration, float shakeAmount);
void SmoothCameraTarget(CameraState* sourceCamera, CameraState* targetCamera, float frame, float duration);
void SmoothCameraTargetAndSnap(CameraState* camera, CameraState* targetCamera, float frame, float duration);
void RemoveCharacterEffect(CharacterEffectState* effectState, s32 effectId);
void SetCharacterEffectState(CharacterEffectState* effectState, s32 effectId);
void ClearCharacterEffectState(CharacterEffectState* effectState);
void SetCharacterEffectEnabled(CharacterEffectState* effectState, s32 enabled);
float GetRelativeCharacterAngle(s32 sourceModelId, s32 targetModelId, s32 useWorldRotation);
float GetCharacterDistance(s32 sourceModelId, s32 targetModelId);
float InterpolateByFrame(s32 duration, float startValue, float endValue);
void UpdateCharacterEffectObject(OMOBJ* effectObject);
void InitializeCharacterEffect(HUPROCESS* process, CharacterEffectState* effectState, s32 effectType, const char* effectName, s32 effectLifetime, s32 effectFlags);

const char* characterHeadMotionNames[] = {
    "c000m1-ske_head",
    "c001m1-ske_head",
    "c002m1-ske_head",
    "c003m1-ske_head",
    "c004m1-ske_head",
    "c005m1-ske_head",
    "c006m1-ske_head",
    "c007m1-ske_head"
};

s32 characterEffectSettings[][2] = {
    { 0x100, 1 },
    { 0x100, 1 },
    {  0x80, 0 },
    {  0x80, 0 },
    {  0x80, 0 },
    { 0x100, 1 },
    {  0x80, 0 },
    {  0x80, 0 }
};

// Similar to mgmodedll/main.c::fn_1_240E4 (identical in behavior)
float LerpClamped(float startValue, float endValue, float frame, float duration) {
    if (frame >= duration) {
        return endValue;
    }
    return startValue + (frame / duration) * (endValue - startValue);
}

float WeightedAverage(float currentValue, float targetValue, float weight) {
    if (weight <= 1.0f) {
        return targetValue;
    }
    return (targetValue + currentValue * (weight - 1.0f)) / weight;
}

// Identical to mgmodedll/main.c::fn_1_24204
float EaseInSine(float startValue, float endValue, float frame, float duration) {
    if (frame >= duration) {
        return endValue;
    }
    return startValue + (endValue - startValue) * sind((90.0f / duration) * frame);
}

// Identical to mgmodedll/main.c::fn_1_242CC
float EaseInOutSine(float startValue, float endValue, float frame, float duration) {
    if (frame >= duration) {
        return endValue;
    }
    return startValue + (endValue - startValue) * sind((360.0f / duration) * frame);
}

// Identical to mgmodedll/main.c::fn_1_2412C
float EaseOutSine(float startValue, float endValue, float frame, float duration) {
    if (frame >= duration) {
        return endValue;
    }
    return startValue + (endValue - startValue) * (1.0 - cosd((90.0f / duration) * frame));
}

void LoadCurrentMotionSequenceEntry(MotionSequenceState* sequence) {
    sequence->frameCounter = 0;
    sequence->duration = sequence->entries[sequence->entryIndex].duration;
    sequence->motionCallback = sequence->entries[sequence->entryIndex].motionCallback;
    sequence->completionCondition = sequence->entries[sequence->entryIndex].completionCondition;
}

void InitializeMotionSequence(MotionSequenceState* sequence, MotionSequenceEntry* entries) {
    sequence->entryIndex = 0;
    sequence->entries = entries;
    LoadCurrentMotionSequenceEntry(sequence);
}

s32 AdvanceMotionSequence(MotionSequenceState* sequence) {
    sequence->entryIndex++;
    LoadCurrentMotionSequenceEntry(sequence);
    if (sequence->motionCallback == NULL && sequence->completionCondition == NULL) {
        return 1;
    }
    return 0;
}

s32 RewindMotionSequence(MotionSequenceState* sequence) {
    sequence->entryIndex--;
    if (sequence->entryIndex < 0) {
        return 1;
    }
    LoadCurrentMotionSequenceEntry(sequence);
    return 0;
}

s32 SetMotionSequenceIndex(MotionSequenceState* sequence, s32 entryIndex) {
    sequence->entryIndex = entryIndex;
    if (sequence->entryIndex < 0) {
        return 1;
    }
    LoadCurrentMotionSequenceEntry(sequence);
    if (sequence->motionCallback == NULL && sequence->completionCondition == NULL) {
        return 1;
    }
    return 0;
}

s32 AdvanceMotionSequenceTimer(MotionSequenceState* sequence, s32 frameDelta) {
    if (frameDelta != 0) {
        sequence->frameCounter += frameDelta;
        if (sequence->frameCounter >= sequence->duration) {
            return 1;
        }
    } else {
        return sequence->frameCounter;
    }
    return 0;
}

s32 IsMotionAtTimeThreshold(OMOBJ* object, s32 modelIndex, s32 motionIndex, s32 frameThreshold, s32 compareAfter) {
    if (object->mtnId[motionIndex] == Hu3DMotionIDGet(object->mdlId[0])) {
        if (compareAfter != 0) {
            if (Hu3DMotionTimeGet(object->mdlId[modelIndex]) >= frameThreshold) {
                return 1;
            }
        } else if (Hu3DMotionTimeGet(object->mdlId[modelIndex]) <= frameThreshold) {
            return 1;
        }
    }
    return 0;
}

void SetModelMotion(OMOBJ* object, s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 motionMode) {
    switch (motionMode) {
        case 0:
            Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, blendFrames, HU3D_MOTATTR_NONE);
            break;
        case 1:
            Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, blendFrames, HU3D_MOTATTR_LOOP);
            break;
        case 2:
            Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, blendFrames, HU3D_MOTATTR_PAUSE);
            break;
    }
}

void EnsureModelMotion(OMOBJ* object, s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 motionMode) {
    if (object->mtnId[motionIndex] != Hu3DMotionIDGet(object->mdlId[modelIndex]) && object->mtnId[motionIndex] != Hu3DMotionShiftIDGet(object->mdlId[modelIndex])) {
        SetModelMotion(object, modelIndex, motionIndex, blendFrames, motionMode);
    }
}

s32 MoveCharacterTowardPosition(OMOBJ* object, Vec targetPosition, float targetRotation, float moveSpeed, float stopDistance, float turnSmoothing, s32 motionMode) {
    float targetDeltaX;
    float targetDeltaZ;
    float targetRotation;
    float movementSpeed;

    targetDeltaX = targetPosition.x - object->trans.x;
    targetDeltaZ = object->trans.z - targetPosition.z;
    targetRotation = 90.0 + atan2d(targetDeltaZ, targetDeltaX);
    movementSpeed = sqrtf(targetDeltaX * targetDeltaX + targetDeltaZ * targetDeltaZ);
    if (movementSpeed <= stopDistance) {
        targetDeltaX = targetDeltaZ = 0.0f;
    } else {
        targetDeltaX = 72.0f * (targetDeltaX / movementSpeed);
        targetDeltaZ = 72.0f * (targetDeltaZ / movementSpeed);
    }
    movementSpeed = moveSpeed * (sqrtf(targetDeltaX * targetDeltaX + targetDeltaZ * targetDeltaZ) / 72.0f);
    if (movementSpeed >= moveSpeed) {
        movementSpeed = moveSpeed;
    } else if (movementSpeed <= 1.0f) {
        targetDeltaX = targetDeltaZ = movementSpeed = 0.0f;
    }
    if (targetDeltaX != 0.0f || targetDeltaZ != 0.0f || targetRotation >= 0.0f) {
        if (targetRotation >= 0.0f && targetDeltaX == 0.0f && targetDeltaZ == 0.0f) {
            targetRotation = targetRotation;
        }
        if (targetRotation >= 180.0f) {
            if (object->rot.y - targetRotation >= 180.0f) {
                object->rot.y -= 360.0f;
            }
            if (object->rot.y - targetRotation < -180.0f) {
                object->rot.y += 360.0f;
            }
        } else {
            if (object->rot.y - targetRotation > 180.0f) {
                object->rot.y -= 360.0f;
            }
            if (object->rot.y - targetRotation <= -180.0f) {
                object->rot.y += 360.0f;
            }
        }
        object->rot.y = (targetRotation + object->rot.y * (turnSmoothing - 1.0f)) / turnSmoothing;
    }
    object->trans.x += movementSpeed * sind(object->rot.y);
    object->trans.z += movementSpeed * cosd(object->rot.y);
    if (motionMode != 0) {
        if (movementSpeed > 0.0f) {
            if (motionMode == 1 || motionMode == 3) {
                if (object->mtnId[1] != Hu3DMotionIDGet(object->mdlId[0]) && object->mtnId[1] != Hu3DMotionShiftIDGet(object->mdlId[0])) {
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
                }
            } else if (object->mtnId[10] != Hu3DMotionIDGet(object->mdlId[0]) && object->mtnId[10] != Hu3DMotionShiftIDGet(object->mdlId[0])) {
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[10], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
            }
        } else {
            if (object->mtnId[0] != Hu3DMotionIDGet(object->mdlId[0]) && object->mtnId[0] != Hu3DMotionShiftIDGet(object->mdlId[0])) {
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
            }
            return 1;
        }
    }
    return 0;
}

void UpdateModelPositionAndFacing(OMOBJ* object, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 turnSmoothing, s32 frame, s32 duration, s32 updatePosition, s32 updateFacing) {
    Vec startPosition;
    Vec targetPosition;
    Vec interpolatedPosition;
    float currentRotation;
    float targetRotation;
    float animationFrame;
    float animationDuration;

    startPosition.x = startPosition->x;
    startPosition.y = startPosition->y;
    startPosition.z = startPosition->z;
    targetPosition.x = targetPosition->x;
    targetPosition.y = targetPosition->y;
    targetPosition.z = targetPosition->z;
    currentRotation = Hu3DData[object->mdlId[modelIndex]].rot.y;
    if (currentRotation < 0.0f) {
        currentRotation += 360.0f;
    } else if (currentRotation >= 360.0f) {
        currentRotation -= 360.0f;
    }
    animationFrame = frame;
    animationDuration = duration;
    if (updatePosition != 0) {
        interpolatedPosition.x = LerpClamped(startPosition.x, targetPosition.x, animationFrame, animationDuration);
        interpolatedPosition.y = LerpClamped(startPosition.y, targetPosition.y, animationFrame, animationDuration);
        interpolatedPosition.z = LerpClamped(startPosition.z, targetPosition.z, animationFrame, animationDuration);
        if (modelIndex == 0) {
            omSetTra(object, interpolatedPosition.x, interpolatedPosition.y, interpolatedPosition.z);
        } else {
            Hu3DModelPosSet(object->mdlId[modelIndex], interpolatedPosition.x, interpolatedPosition.y, interpolatedPosition.z);
        }
    }
    if (updateFacing == 0) {
        return;
    }
    targetRotation = -(atan2d(targetPosition.z - startPosition.z, targetPosition.x - startPosition.x) - 90.0);
    if (targetRotation < 0.0f) {
        targetRotation += 360.0f;
    } else if (targetRotation >= 360.0f) {
        targetRotation -= 360.0f;
    }
    if (targetRotation >= 180.0f) {
        if (currentRotation - targetRotation >= 180.0f) {
            currentRotation -= 360.0f;
        }
        if (currentRotation - targetRotation < -180.0f) {
            currentRotation += 360.0f;
        }
    } else {
        if (currentRotation - targetRotation > 180.0f) {
            currentRotation -= 360.0f;
        }
        if (currentRotation - targetRotation <= -180.0f) {
            currentRotation += 360.0f;
        }
    }
    currentRotation = (targetRotation + currentRotation * (turnSmoothing - 1)) / turnSmoothing;
    if (modelIndex == 0) {
        object->rot.y = currentRotation;
    } else {
        Hu3DData[object->mdlId[modelIndex]].rot.y = currentRotation;
    }
}

void InterpolateModelPosition(OMOBJ* object, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 turnSmoothing, s32 frame, s32 duration) {
    UpdateModelPositionAndFacing(object, modelIndex, startPosition, targetPosition, 0, frame, duration, 1, 0);
}

void InterpolateModelFacing(OMOBJ* object, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 turnSmoothing, s32 frame, s32 duration) {
    UpdateModelPositionAndFacing(object, modelIndex, startPosition, targetPosition, turnSmoothing, frame, duration, 0, 1);
}

void InterpolateModelPositionAndFacing(OMOBJ* object, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 turnSmoothing, s32 frame, s32 duration) {
    UpdateModelPositionAndFacing(object, modelIndex, startPosition, targetPosition, turnSmoothing, frame, duration, 1, 1);
}

void SnapCameraToTarget(CameraState* camera) {
    camera->cameraCenterX = camera->targetCenterX;
    camera->cameraCenterY = camera->targetCenterY;
    camera->cameraCenterZ = camera->targetCenterZ;
    camera->cameraRotationX = camera->targetRotationX;
    camera->cameraRotationY = camera->targetRotationY;
    camera->cameraRotationZ = camera->targetRotationZ;
    camera->cameraZoom = camera->targetZoom;
}

void ResetCameraTarget(CameraState* camera) {
    camera->animationFrame = 0;
    camera->targetCenterX = camera->cameraCenterX;
    camera->targetCenterY = camera->cameraCenterY;
    camera->targetCenterZ = camera->cameraCenterZ;
    camera->targetRotationX = camera->cameraRotationX;
    camera->targetRotationY = camera->cameraRotationY;
    camera->targetRotationZ = camera->cameraRotationZ;
    camera->targetZoom = camera->cameraZoom;
}

void BlendCameraTowardTarget(CameraState* camera, CameraState* targetCamera, float blendWeight) {
    camera->cameraCenterX = WeightedAverage(camera->cameraCenterX, targetCamera->targetCenterX, blendWeight);
    camera->cameraCenterY = WeightedAverage(camera->cameraCenterY, targetCamera->targetCenterY, blendWeight);
    camera->cameraCenterZ = WeightedAverage(camera->cameraCenterZ, targetCamera->targetCenterZ, blendWeight);
    camera->cameraRotationX = WeightedAverage(camera->cameraRotationX, targetCamera->targetRotationX, blendWeight);
    camera->cameraRotationY = WeightedAverage(camera->cameraRotationY, targetCamera->targetRotationY, blendWeight);
    camera->cameraRotationZ = WeightedAverage(camera->cameraRotationZ, targetCamera->targetRotationZ, blendWeight);
    camera->cameraZoom = WeightedAverage(camera->cameraZoom, targetCamera->targetZoom, blendWeight);
}

void ApplyCameraShake(CameraState* camera, float shakeAmount) {
    BlendCameraTowardTarget(camera, camera, shakeAmount);
}

void InterpolateCameraTarget(CameraState* sourceCamera, CameraState* targetCamera, float frame, float duration) {
    targetCamera->targetCenterX = LerpClamped(sourceCamera->targetCenterX, targetCamera->cameraCenterX, frame, duration);
    targetCamera->targetCenterY = LerpClamped(sourceCamera->targetCenterY, targetCamera->cameraCenterY, frame, duration);
    targetCamera->targetCenterZ = LerpClamped(sourceCamera->targetCenterZ, targetCamera->cameraCenterZ, frame, duration);
    targetCamera->targetRotationX = LerpClamped(sourceCamera->targetRotationX, targetCamera->cameraRotationX, frame, duration);
    targetCamera->targetRotationY = LerpClamped(sourceCamera->targetRotationY, targetCamera->cameraRotationY, frame, duration);
    targetCamera->targetRotationZ = LerpClamped(sourceCamera->targetRotationZ, targetCamera->cameraRotationZ, frame, duration);
    targetCamera->targetZoom = LerpClamped(sourceCamera->targetZoom, targetCamera->cameraZoom, frame, duration);
}

void BlendCameraTargetAndSnap(CameraState* camera, CameraState* targetCamera, float frame, float duration) {
    InterpolateCameraTarget(camera, targetCamera, frame, duration);
    camera->cameraCenterX = targetCamera->targetCenterX;
    camera->cameraCenterY = targetCamera->targetCenterY;
    camera->cameraCenterZ = targetCamera->targetCenterZ;
    camera->cameraRotationX = targetCamera->targetRotationX;
    camera->cameraRotationY = targetCamera->targetRotationY;
    camera->cameraRotationZ = targetCamera->targetRotationZ;
    camera->cameraZoom = targetCamera->targetZoom;
}

void BlendCameraTargetWithShake(CameraState* camera, CameraState* targetCamera, float frame, float duration, float shakeAmount) {
    InterpolateCameraTarget(camera, targetCamera, frame, duration);
    BlendCameraTowardTarget(camera, targetCamera, shakeAmount);
}

void EaseCameraTarget(CameraState* sourceCamera, CameraState* targetCamera, float frame, float duration) {
    targetCamera->targetCenterX = EaseInSine(sourceCamera->targetCenterX, targetCamera->cameraCenterX, frame, duration);
    targetCamera->targetCenterY = EaseInSine(sourceCamera->targetCenterY, targetCamera->cameraCenterY, frame, duration);
    targetCamera->targetCenterZ = EaseInSine(sourceCamera->targetCenterZ, targetCamera->cameraCenterZ, frame, duration);
    targetCamera->targetRotationX = EaseInSine(sourceCamera->targetRotationX, targetCamera->cameraRotationX, frame, duration);
    targetCamera->targetRotationY = EaseInSine(sourceCamera->targetRotationY, targetCamera->cameraRotationY, frame, duration);
    targetCamera->targetRotationZ = EaseInSine(sourceCamera->targetRotationZ, targetCamera->cameraRotationZ, frame, duration);
    targetCamera->targetZoom = EaseInSine(sourceCamera->targetZoom, targetCamera->cameraZoom, frame, duration);
}

void EaseCameraTargetAndSnap(CameraState* camera, CameraState* targetCamera, float frame, float duration) {
    EaseCameraTarget(camera, targetCamera, frame, duration);
    camera->cameraCenterX = targetCamera->targetCenterX;
    camera->cameraCenterY = targetCamera->targetCenterY;
    camera->cameraCenterZ = targetCamera->targetCenterZ;
    camera->cameraRotationX = targetCamera->targetRotationX;
    camera->cameraRotationY = targetCamera->targetRotationY;
    camera->cameraRotationZ = targetCamera->targetRotationZ;
    camera->cameraZoom = targetCamera->targetZoom;
}

void EaseCameraTargetWithShake(CameraState* camera, CameraState* targetCamera, float frame, float duration, float shakeAmount) {
    EaseCameraTarget(camera, targetCamera, frame, duration);
    BlendCameraTowardTarget(camera, targetCamera, shakeAmount);
}

void SmoothCameraTarget(CameraState* sourceCamera, CameraState* targetCamera, float frame, float duration) {
    targetCamera->targetCenterX = EaseOutSine(sourceCamera->targetCenterX, targetCamera->cameraCenterX, frame, duration);
    targetCamera->targetCenterY = EaseOutSine(sourceCamera->targetCenterY, targetCamera->cameraCenterY, frame, duration);
    targetCamera->targetCenterZ = EaseOutSine(sourceCamera->targetCenterZ, targetCamera->cameraCenterZ, frame, duration);
    targetCamera->targetRotationX = EaseOutSine(sourceCamera->targetRotationX, targetCamera->cameraRotationX, frame, duration);
    targetCamera->targetRotationY = EaseOutSine(sourceCamera->targetRotationY, targetCamera->cameraRotationY, frame, duration);
    targetCamera->targetRotationZ = EaseOutSine(sourceCamera->targetRotationZ, targetCamera->cameraRotationZ, frame, duration);
    targetCamera->targetZoom = EaseOutSine(sourceCamera->targetZoom, targetCamera->cameraZoom, frame, duration);
}

void SmoothCameraTargetAndSnap(CameraState* camera, CameraState* targetCamera, float frame, float duration) {
    SmoothCameraTarget(camera, targetCamera, frame, duration);
    camera->cameraCenterX = targetCamera->targetCenterX;
    camera->cameraCenterY = targetCamera->targetCenterY;
    camera->cameraCenterZ = targetCamera->targetCenterZ;
    camera->cameraRotationX = targetCamera->targetRotationX;
    camera->cameraRotationY = targetCamera->targetRotationY;
    camera->cameraRotationZ = targetCamera->targetRotationZ;
    camera->cameraZoom = targetCamera->targetZoom;
}

void SmoothCameraTargetWithShake(CameraState* camera, CameraState* targetCamera, float frame, float duration, float shakeAmount) {
    SmoothCameraTarget(camera, targetCamera, frame, duration);
    BlendCameraTowardTarget(camera, targetCamera, shakeAmount);
}

void AddCharacterEffect(CharacterEffectState* effectState, s32 effectId) {
    s32 freeEntryIndex;
    s32 entryIndex;

    freeEntryIndex = -1;
    for (entryIndex = 0; entryIndex < 20; entryIndex++) {
        if (effectState->effectIds[entryIndex] == -1) {
            freeEntryIndex = entryIndex;
            break;
        }
    }
    if (freeEntryIndex != -1) {
        effectState->effectIds[freeEntryIndex] = effectId;
        effectState->enabled = 1;
    }
}

void RemoveCharacterEffect(CharacterEffectState* effectState, s32 effectId) {
    s32 i;

    for (i = 0; i < 20; i++) {
        if (effectId == -1) {
            effectState->effectIds[i] = -1;
        } else if (effectState->effectIds[i] == effectId) {
            effectState->effectIds[i] = -1;
        }
    }
}

void SetCharacterEffectState(CharacterEffectState* effectState, s32 effectId) {
    effectState->activeEffectId = effectId;
    effectState->enabled = 1;
}

void ClearCharacterEffectState(CharacterEffectState* effectState) {
    effectState->activeEffectId = -1;
}

void SetCharacterEffectEnabled(CharacterEffectState* effectState, s32 enabled) {
    effectState->enabled = enabled;
}

void ResetCharacterEffects(CharacterEffectState* effectState) {
    SetCharacterEffectEnabled(effectState, 0);
    RemoveCharacterEffect(effectState, -1);
    ClearCharacterEffectState(effectState);
}

float GetRelativeCharacterAngle(s32 sourceModelId, s32 targetModelId, s32 useWorldRotation) {
    Vec sourcePosition;
    Vec targetPosition;
    Vec direction;
    float relativeAngle;
    float sourceRotation;

    sourcePosition.x = Hu3DData[sourceModelId].pos.x;
    sourcePosition.z = Hu3DData[sourceModelId].pos.z;
    targetPosition.x = Hu3DData[targetModelId].pos.x;
    targetPosition.z = Hu3DData[targetModelId].pos.z;
    direction.x = targetPosition.x - sourcePosition.x;
    direction.z = targetPosition.z - sourcePosition.z;
    sourceRotation = Hu3DData[sourceModelId].rot.y;
    if (sourceRotation >= 360.0f) {
        sourceRotation -= 360.0f;
    } else if (sourceRotation < 0.0f) {
        sourceRotation += 360.0f;
    }
    if (sourceRotation >= 360.0f) {
        sourceRotation -= 360.0f;
    } else if (sourceRotation < 0.0f) {
        sourceRotation += 360.0f;
    }
    if (sourceRotation >= 360.0f) {
        sourceRotation -= 360.0f;
    } else if (sourceRotation < 0.0f) {
        sourceRotation += 360.0f;
    }
    relativeAngle = atan2d(direction.z, direction.x) - 90.0;
    if (useWorldRotation == 0) {
        relativeAngle *= -1.0f;
    }
    if (useWorldRotation != 0) {
        relativeAngle += sourceRotation;
    } else {
        relativeAngle -= sourceRotation;
    }
    if (relativeAngle >= 180.0f) {
        relativeAngle -= 360.0f;
    } else if (relativeAngle < -180.0f) {
        relativeAngle += 360.0f;
    }
    if (relativeAngle >= 180.0f) {
        relativeAngle -= 360.0f;
    } else if (relativeAngle < -180.0f) {
        relativeAngle += 360.0f;
    }
    if (relativeAngle >= 180.0f) {
        relativeAngle -= 360.0f;
    } else if (relativeAngle < -180.0f) {
        relativeAngle += 360.0f;
    }
    return relativeAngle;
}

float GetCharacterDistance(s32 sourceModelId, s32 targetModelId) {
    Vec sourcePosition;
    Vec targetPosition;
    Vec direction;

    sourcePosition.x = Hu3DData[sourceModelId].pos.x;
    sourcePosition.z = Hu3DData[sourceModelId].pos.z;
    targetPosition.x = Hu3DData[targetModelId].pos.x;
    targetPosition.z = Hu3DData[targetModelId].pos.z;
    direction.x = targetPosition.x - sourcePosition.x;
    direction.z = targetPosition.z - sourcePosition.z;
    return sqrtf(direction.x * direction.x + direction.z * direction.z);
}

float InterpolateByFrame(s32 duration, float startValue, float endValue) {
    return (endValue + startValue * (duration - 1)) / duration;
}

void UpdateCharacterEffectObject(OMOBJ* effectObject) {
}

void InitializeCharacterEffect(HUPROCESS* process, CharacterEffectState* effectState, s32 effectType, const char* effectName, s32 effectLifetime, s32 effectFlags) {
    effectState->enabled = 0;
    effectState->effectType = effectType;
    strcpy(effectState->effectName, effectName);
    effectState->effectLifetime = effectLifetime;
    effectState->effectFlags = effectFlags;
    effectState->activeEffectId = -1;
    RemoveCharacterEffect(effectId, -1);
    effectState->effectObject = omAddObjEx(process, 0x3000, 0, 0, -1, UpdateCharacterEffectObject);
    effectState->effectObject->data = effectState;
    effectState->effectObject->trans.x = 10.0f;
    effectState->effectObject->trans.y = 10000.0f;
    effectState->effectObject->trans.z = 10.0f;
    effectState->effectObject->rot.x = 0.0f;
    effectState->effectObject->rot.y = 0.0f;
    effectState->effectObject->rot.z = 0.0f;
    effectState->effectObject->work[0] = effectState->effectObject->work[1] = effectState->effectObject->work[2] = effectState->effectObject->work[3] = 0;
}

void StartCharacterEffect(HUPROCESS* process, CharacterEffectState* effectState, s32 effectType, const char* effectName) {
    InitializeCharacterEffect(process, effectState, effectType, effectName, 0x100, 0);
}

void StartCharacterEffectForPlayer(HUPROCESS* process, CharacterEffectState* effectState, s32 effectType, s32 playerIndex) {
    InitializeCharacterEffect(process, effectState, effectType, characterHeadMotionNames[playerIndex], characterEffectSettings[playerIndex][0], characterEffectSettings[playerIndex][1]);
}
