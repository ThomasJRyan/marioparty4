#include "REL/m436Dll.h"
#include "game/hu3d.h"

#include "ext_math.h"

float WeightedAverageStep(float previousValue, float currentValue, float elapsedFrames) {
    if (elapsedFrames <= 1.0f) {
        return currentValue;
    }
    return (currentValue + previousValue * (elapsedFrames - 1.0f)) / elapsedFrames;
}

float LinearInterpolate(float startValue, float endValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return startValue + (elapsedFrames / durationFrames) * (endValue - startValue);
}

float SineEaseInterpolate(float startValue, float endValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return startValue + (endValue - startValue) * sind((90.0f / durationFrames) * elapsedFrames);
}

float FullSineInterpolate(float startValue, float endValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return startValue + (endValue - startValue) * sind((360.0f / durationFrames) * elapsedFrames);
}

float CosineEaseInterpolate(float startValue, float endValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return startValue + (endValue - startValue) * (1.0 - cosd((90.0f / durationFrames) * elapsedFrames));
}

void TurnModelTowardTarget(OMOBJ* sourceObject, s32 sourceModelSlot, OMOBJ* targetObject, s32 targetModelSlot) {
    Vec sourcePosition;
    Vec targetPosition;
    Vec rotationValues;
    Vec positionDifference;

    targetPosition.x = Hu3DData[sourceObject->mdlId[sourceModelSlot]].pos.x;
    targetPosition.z = Hu3DData[sourceObject->mdlId[sourceModelSlot]].pos.z;
    sourcePosition.x = Hu3DData[targetObject->mdlId[targetModelSlot]].pos.x;
    sourcePosition.z = Hu3DData[targetObject->mdlId[targetModelSlot]].pos.z;
    rotationValues.y = Hu3DData[sourceObject->mdlId[sourceModelSlot]].rot.y;
    positionDifference.x = sourcePosition.x - targetPosition.x;
    positionDifference.z = sourcePosition.z - targetPosition.z;
    rotationValues.x = -(atan2d(positionDifference.z, positionDifference.x) - 90.0);
    if (rotationValues.x >= 360.0f) {
        rotationValues.x -= 360.0f;
    }
    if (rotationValues.x < 0.0f) {
        rotationValues.x += 360.0f;
    }
    if (rotationValues.x >= 180.0f) {
        if (rotationValues.y - rotationValues.x >= 180.0f) {
            rotationValues.y -= 360.0f;
        }
        if (rotationValues.y - rotationValues.x < -180.0f) {
            rotationValues.y += 360.0f;
        }
    } else {
        if (rotationValues.y - rotationValues.x > 180.0f) {
            rotationValues.y -= 360.0f;
        }
        if (rotationValues.y - rotationValues.x <= -180.0f) {
            rotationValues.y += 360.0f;
        }
    }
    rotationValues.y = (9.0f * rotationValues.y + rotationValues.x) / 10.0f;
    if (sourceModelSlot == 0) {
        sourceObject->rot.y = rotationValues.y;
    } else {
        Hu3DData[sourceObject->mdlId[sourceModelSlot]].rot.y = rotationValues.y;
    }
}

s32 IsMotionAtTime(OMOBJ* characterObject, s32 modelSlot, s32 motionSlot, s32 frameThreshold, s32 checkForward) {
    if (characterObject->mtnId[motionSlot] == Hu3DMotionIDGet(characterObject->mdlId[0])) {
        if (checkForward != 0) {
            if (Hu3DMotionTimeGet(characterObject->mdlId[modelSlot]) >= frameThreshold) {
                return 1;
            }
        } else if (Hu3DMotionTimeGet(characterObject->mdlId[modelSlot]) <= frameThreshold) {
            return 1;
        }
    }
    return 0;
}

void SetCharacterMotion(OMOBJ* characterObject, s32 modelSlot, s32 motionSlot, s32 blendFrames, s32 motionMode) {
    switch (motionMode) {
        case 0:
            Hu3DMotionShiftSet(characterObject->mdlId[modelSlot], characterObject->mtnId[motionSlot], 0.0f, blendFrames, HU3D_MOTATTR_NONE);
            break;
        case 1:
            Hu3DMotionShiftSet(characterObject->mdlId[modelSlot], characterObject->mtnId[motionSlot], 0.0f, blendFrames, HU3D_MOTATTR_LOOP);
            break;
        case 2:
            Hu3DMotionShiftSet(characterObject->mdlId[modelSlot], characterObject->mtnId[motionSlot], 0.0f, blendFrames, HU3D_MOTATTR_PAUSE);
            break;
    }
}

void SetCharacterMotionCached(OMOBJ* characterObject, s32 modelSlot, s32 motionSlot, s32 blendFrames, s32 motionMode) {
    characterObject->work[3] = characterObject->mtnId[motionSlot];
    SetCharacterMotion(characterObject, modelSlot, motionSlot, blendFrames, motionMode);
}

void UpdateCharacterMotionIfChanged(OMOBJ* characterObject, s32 modelSlot, s32 motionSlot, s32 blendFrames, s32 motionMode) {
    if (characterObject->work[3] != characterObject->mtnId[motionSlot]) {
        characterObject->work[3] = characterObject->mtnId[motionSlot];
        SetCharacterMotionCached(characterObject, modelSlot, motionSlot, blendFrames, motionMode);
    }
}

void LoadSequenceEntry(SequencePlayer* sequenceState) {
    sequenceState->elapsedFrames = 0;
    sequenceState->durationFrames = sequenceState->entries[sequenceState->entryIndex].durationFrames;
    sequenceState->entryCallback = sequenceState->entries[sequenceState->entryIndex].startCallback;
    sequenceState->completionCallback = sequenceState->entries[sequenceState->entryIndex].completionCallback;
}

void InitializeSequence(SequencePlayer* sequenceState, SequenceEntry* entries) {
    sequenceState->entryIndex = 0;
    sequenceState->entries = entries;
    LoadSequenceEntry(sequenceState);
}

s32 AdvanceSequence(SequencePlayer* sequenceState) {
    sequenceState->entryIndex++;
    LoadSequenceEntry(sequenceState);
    if (sequenceState->entryCallback == NULL && sequenceState->completionCallback == NULL) {
        return 1;
    }
    return 0;
}

s32 RewindSequence(SequencePlayer* sequenceState) {
    sequenceState->entryIndex--;
    if (sequenceState->entryIndex < 0) {
        return 1;
    }
    LoadSequenceEntry(sequenceState);
    return 0;
}

s32 SetSequenceEntry(SequencePlayer* sequenceState, s32 entryIndex) {
    sequenceState->entryIndex = entryIndex;
    if (sequenceState->entryIndex < 0) {
        return 1;
    }
    LoadSequenceEntry(sequenceState);
    if (sequenceState->entryCallback == NULL && sequenceState->completionCallback == NULL) {
        return 1;
    }
    return 0;
}

s32 UpdateSequenceTimer(SequencePlayer* sequenceState, s32 frameIncrement) {
    if (frameIncrement == 0) {
        return sequenceState->elapsedFrames;
    }
    sequenceState->elapsedFrames += frameIncrement;
    if (sequenceState->elapsedFrames >= sequenceState->durationFrames) {
        return 1;
    }
    return 0;
}

void SetCameraTargetFromPositions(CameraState* cameraState, float* sourcePosition, float* targetPosition) {
    float deltaX;
    float deltaY;
    float deltaZ;
    float horizontalAngle;

    deltaX = sourcePosition[0] - targetPosition[0];
    deltaY = sourcePosition[1] - targetPosition[1];
    deltaZ = sourcePosition[2] - targetPosition[2];
    cameraState->targetCenterX = targetPosition[0];
    cameraState->targetCenterY = targetPosition[1];
    cameraState->targetCenterZ = targetPosition[2];
    cameraState->targetZoom = sqrtf(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
    horizontalAngle = atan2d(deltaZ, deltaX);
    cameraState->targetRotationX = -(atan2d(deltaY, deltaX * cosd(-horizontalAngle) + deltaZ * -sind(-horizontalAngle)));
    cameraState->targetRotationY = -(horizontalAngle - 90.0f);
    cameraState->targetRotationZ = 0.0f;
}

void CopyCameraTargetToCurrent(CameraState* cameraState) {
    cameraState->centerX = cameraState->targetCenterX;
    cameraState->centerY = cameraState->targetCenterY;
    cameraState->centerZ = cameraState->targetCenterZ;
    cameraState->rotationX = cameraState->targetRotationX;
    cameraState->rotationY = cameraState->targetRotationY;
    cameraState->rotationZ = cameraState->targetRotationZ;
    cameraState->zoom = cameraState->targetZoom;
}

void CopyCameraCurrentToTarget(CameraState* cameraState) {
    cameraState->interpolationTimer = 0.0f;
    cameraState->targetCenterX = cameraState->centerX;
    cameraState->targetCenterY = cameraState->centerY;
    cameraState->targetCenterZ = cameraState->centerZ;
    cameraState->targetRotationX = cameraState->rotationX;
    cameraState->targetRotationY = cameraState->rotationY;
    cameraState->targetRotationZ = cameraState->rotationZ;
    cameraState->targetZoom = cameraState->zoom;
}

void InterpolateCameraState(CameraState* currentState, CameraState* targetState, float interpolationFactor) {
    currentState->centerX = WeightedAverageStep(currentState->centerX, targetState->targetCenterX, interpolationFactor);
    currentState->centerY = WeightedAverageStep(currentState->centerY, targetState->targetCenterY, interpolationFactor);
    currentState->centerZ = WeightedAverageStep(currentState->centerZ, targetState->targetCenterZ, interpolationFactor);
    currentState->rotationX = WeightedAverageStep(currentState->rotationX, targetState->targetRotationX, interpolationFactor);
    currentState->rotationY = WeightedAverageStep(currentState->rotationZ, targetState->targetRotationY, interpolationFactor);
    currentState->rotationZ = WeightedAverageStep(currentState->rotationY, targetState->targetRotationZ, interpolationFactor);
    currentState->zoom = WeightedAverageStep(currentState->zoom, targetState->targetZoom, interpolationFactor);
}

void InterpolateCameraStateInPlace(CameraState* cameraState, float interpolationFactor) {
    InterpolateCameraState(cameraState, cameraState, interpolationFactor);
}

void InterpolateCameraStateOverTime(CameraState* startState, CameraState* endState, float elapsedFrames, float durationFrames) {
    endState->targetCenterX = LinearInterpolate(startState->targetCenterX, endState->centerX, elapsedFrames, durationFrames);
    endState->targetCenterY = LinearInterpolate(startState->targetCenterY, endState->centerY, elapsedFrames, durationFrames);
    endState->targetCenterZ = LinearInterpolate(startState->targetCenterZ, endState->centerZ, elapsedFrames, durationFrames);
    endState->targetRotationX = LinearInterpolate(startState->targetRotationX, endState->rotationX, elapsedFrames, durationFrames);
    endState->targetRotationY = LinearInterpolate(startState->targetRotationZ, endState->rotationY, elapsedFrames, durationFrames);
    endState->targetRotationZ = LinearInterpolate(startState->targetRotationY, endState->rotationZ, elapsedFrames, durationFrames);
    endState->targetZoom = LinearInterpolate(startState->targetZoom, endState->zoom, elapsedFrames, durationFrames);
}

void InterpolateAndCommitCameraState(CameraState* cameraState, CameraState* targetState, float elapsedFrames, float durationFrames) {
    InterpolateCameraStateOverTime(cameraState, targetState, elapsedFrames, durationFrames);
    cameraState->centerX = targetState->targetCenterX;
    cameraState->centerY = targetState->targetCenterY;
    cameraState->centerZ = targetState->targetCenterZ;
    cameraState->rotationX = targetState->targetRotationX;
    cameraState->rotationY = targetState->targetRotationY;
    cameraState->rotationZ = targetState->targetRotationZ;
    cameraState->zoom = targetState->targetZoom;
}

void InterpolateCameraStateWithSmoothing(CameraState* currentState, CameraState* targetState, float elapsedFrames, float durationFrames, float smoothingFactor) {
    InterpolateCameraStateOverTime(currentState, targetState, elapsedFrames, durationFrames);
    InterpolateCameraState(currentState, targetState, smoothingFactor);
}

void SineInterpolateCameraState(CameraState* startState, CameraState* endState, float elapsedFrames, float durationFrames) {
    endState->targetCenterX = SineEaseInterpolate(startState->targetCenterX, endState->centerX, elapsedFrames, durationFrames);
    endState->targetCenterY = SineEaseInterpolate(startState->targetCenterY, endState->centerY, elapsedFrames, durationFrames);
    endState->targetCenterZ = SineEaseInterpolate(startState->targetCenterZ, endState->centerZ, elapsedFrames, durationFrames);
    endState->targetRotationX = SineEaseInterpolate(startState->targetRotationX, endState->rotationX, elapsedFrames, durationFrames);
    endState->targetRotationY = SineEaseInterpolate(startState->targetRotationZ, endState->rotationY, elapsedFrames, durationFrames);
    endState->targetRotationZ = SineEaseInterpolate(startState->targetRotationY, endState->rotationZ, elapsedFrames, durationFrames);
    endState->targetZoom = SineEaseInterpolate(startState->targetZoom, endState->zoom, elapsedFrames, durationFrames);
}

void SineInterpolateAndCommitCameraState(CameraState* cameraState, CameraState* targetState, float elapsedFrames, float durationFrames) {
    SineInterpolateCameraState(cameraState, targetState, elapsedFrames, durationFrames);
    cameraState->centerX = targetState->targetCenterX;
    cameraState->centerY = targetState->targetCenterY;
    cameraState->centerZ = targetState->targetCenterZ;
    cameraState->rotationX = targetState->targetRotationX;
    cameraState->rotationY = targetState->targetRotationY;
    cameraState->rotationZ = targetState->targetRotationZ;
    cameraState->zoom = targetState->targetZoom;
}

void SineInterpolateCameraStateWithSmoothing(CameraState* currentState, CameraState* targetState, float elapsedFrames, float durationFrames, float smoothingFactor) {
    SineInterpolateCameraState(currentState, targetState, elapsedFrames, durationFrames);
    InterpolateCameraState(currentState, targetState, smoothingFactor);
}

void CosineInterpolateCameraState(CameraState* startState, CameraState* endState, float elapsedFrames, float durationFrames) {
    endState->targetCenterX = CosineEaseInterpolate(startState->targetCenterX, endState->centerX, elapsedFrames, durationFrames);
    endState->targetCenterY = CosineEaseInterpolate(startState->targetCenterY, endState->centerY, elapsedFrames, durationFrames);
    endState->targetCenterZ = CosineEaseInterpolate(startState->targetCenterZ, endState->centerZ, elapsedFrames, durationFrames);
    endState->targetRotationX = CosineEaseInterpolate(startState->targetRotationX, endState->rotationX, elapsedFrames, durationFrames);
    endState->targetRotationY = CosineEaseInterpolate(startState->targetRotationZ, endState->rotationY, elapsedFrames, durationFrames);
    endState->targetRotationZ = CosineEaseInterpolate(startState->targetRotationY, endState->rotationZ, elapsedFrames, durationFrames);
    endState->targetZoom = CosineEaseInterpolate(startState->targetZoom, endState->zoom, elapsedFrames, durationFrames);
}

void CosineInterpolateAndCommitCameraState(CameraState* cameraState, CameraState* targetState, float elapsedFrames, float durationFrames) {
    CosineInterpolateCameraState(cameraState, targetState, elapsedFrames, durationFrames);
    cameraState->centerX = targetState->targetCenterX;
    cameraState->centerY = targetState->targetCenterY;
    cameraState->centerZ = targetState->targetCenterZ;
    cameraState->rotationX = targetState->targetRotationX;
    cameraState->rotationY = targetState->targetRotationY;
    cameraState->rotationZ = targetState->targetRotationZ;
    cameraState->zoom = targetState->targetZoom;
}

void CosineInterpolateCameraStateWithSmoothing(CameraState* currentState, CameraState* targetState, float elapsedFrames, float durationFrames, float smoothingFactor) {
    CosineInterpolateCameraState(currentState, targetState, elapsedFrames, durationFrames);
    InterpolateCameraState(currentState, targetState, smoothingFactor);
}
