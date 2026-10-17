#include "REL/m435Dll.h"
#include "game/hu3d.h"

#include "ext_math.h"
#include "string.h"

float InterpolateLinearClamped(float startValue, float endValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return startValue + (elapsedFrames / durationFrames) * (endValue - startValue);
}

float ApproachValueOverDuration(float currentValue, float targetValue, float remainingFrames) {
    if (remainingFrames <= 1.0f) {
        return targetValue;
    }
    return (targetValue + currentValue * (remainingFrames - 1.0f)) / remainingFrames;
}

float InterpolateSineEaseOut(float startValue, float endValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return startValue + (endValue - startValue) * sind((90.0f / durationFrames) * elapsedFrames);
}

float InterpolateCosineEaseIn(float startValue, float endValue, float elapsedFrames, float durationFrames) {
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return startValue + (endValue - startValue) * (1.0 - cosd((90.0f / durationFrames) * elapsedFrames));
}

void UpdateSequenceStep(SequencePlayer* sequencePlayer) {
    sequencePlayer->elapsedFrames = 0;
    sequencePlayer->stepDuration = sequencePlayer->steps[sequencePlayer->stepIndex].durationFrames;
    sequencePlayer->onStep = sequencePlayer->cameraCenterX[sequencePlayer->stepIndex].onEnter;
    sequencePlayer->onComplete = sequencePlayer->cameraCenterX[sequencePlayer->stepIndex].isComplete;
}

void StartSequence(SequencePlayer* sequencePlayer, SequenceStep* sequenceSteps) {
    sequencePlayer->stepIndex = 0;
    sequencePlayer->steps = sequenceSteps;
    UpdateSequenceStep(sequencePlayer);
}

s32 AdvanceSequence(SequencePlayer* sequencePlayer) {
    sequencePlayer->stepIndex++;
    UpdateSequenceStep(sequencePlayer);
    if (sequencePlayer->onStep == NULL && sequencePlayer->onComplete == NULL) {
        return 1;
    }
    return 0;
}

s32 RewindSequence(SequencePlayer* sequencePlayer) {
    sequencePlayer->stepIndex--;
    if (sequencePlayer->stepIndex < 0) {
        return 1;
    }
    UpdateSequenceStep(sequencePlayer);
    return 0;
}

void SetSequenceStep(SequencePlayer* sequencePlayer, s32 stepIndex) {
    sequencePlayer->stepIndex = stepIndex;
    UpdateSequenceStep(sequencePlayer);
}

s32 UpdateSequenceTimer(SequencePlayer* sequencePlayer, s32 frameDelta) {
    if (frameDelta == 0) {
        return sequencePlayer->elapsedFrames;
    }
    sequencePlayer->elapsedFrames += frameDelta;
    if (sequencePlayer->elapsedFrames >= sequencePlayer->stepDuration) {
        return 1;
    }
    return 0;
}

s32 IsMotionAtFrame(OMOBJ* modelObject, s32 modelIndex, s32 motionIndex, s32 frameThreshold, s32 checkAfterFrame) {
    if (modelObject->mtnId[motionIndex] == Hu3DMotionIDGet(modelObject->mdlId[0])) {
        if (checkAfterFrame != 0) {
            if (Hu3DMotionTimeGet(modelObject->mdlId[modelIndex]) >= frameThreshold) {
                return 1;
            }
        } else if (Hu3DMotionTimeGet(modelObject->mdlId[modelIndex]) <= frameThreshold) {
            return 1;
        }
    }
    return 0;
}

void SetModelMotion(OMOBJ* modelObject, s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 motionMode) {
    switch (motionMode) {
        case 0:
            Hu3DMotionShiftSet(modelObject->mdlId[modelIndex], modelObject->mtnId[motionIndex], 0.0f, blendFrames, HU3D_MOTATTR_NONE);
            break;
        case 1:
            Hu3DMotionShiftSet(modelObject->mdlId[modelIndex], modelObject->mtnId[motionIndex], 0.0f, blendFrames, HU3D_MOTATTR_LOOP);
            break;
        case 2:
            Hu3DMotionShiftSet(modelObject->mdlId[modelIndex], modelObject->mtnId[motionIndex], 0.0f, blendFrames, HU3D_MOTATTR_PAUSE);
            break;
    }
}

void SetModelMotionIfChanged(OMOBJ* modelObject, s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 motionMode) {
    if (modelObject->mtnId[motionIndex] != Hu3DMotionIDGet(modelObject->mdlId[modelIndex]) && modelObject->mtnId[motionIndex] != Hu3DMotionShiftIDGet(modelObject->mdlId[modelIndex])) {
        SetModelMotion(modelObject, modelIndex, motionIndex, blendFrames, motionMode);
    }
}

void SetTransformFromPositions(TransformTween* tween, Vec* startPosition, Vec* targetPosition) {
    float deltaX;
    float deltaY;
    float deltaZ;
    float horizontalAngle;

    deltaX = startPosition->x - targetPosition->x;
    deltaY = startPosition->y - targetPosition->y;
    deltaZ = startPosition->z - targetPosition->z;
    tween->targetPositionX = targetPosition->x;
    tween->targetPositionY = targetPosition->y;
    tween->targetPositionZ = targetPosition->z;
    tween->targetDistance = sqrtf(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
    horizontalAngle = atan2d(deltaZ, deltaX);
    tween->targetRotationX = -atan2d(deltaY, deltaX * cosd(-horizontalAngle) + deltaZ * -sind(-horizontalAngle));
    tween->targetRotationY = -(horizontalAngle - 90.0f);
    tween->targetRotationZ = 0.0f;
}

void CopyTransformToStart(TransformTween* tween) {
    tween->cameraCenterX = tween->targetPositionX;
    tween->cameraCenterY = tween->targetPositionY;
    tween->cameraCenterZ = tween->targetPositionZ;
    tween->cameraRotationX = tween->targetRotationX;
    tween->cameraRotationY = tween->targetRotationY;
    tween->cameraRotationZ = tween->targetRotationZ;
    tween->cameraZoom = tween->targetDistance;
}

void CopyTransformToTarget(TransformTween* tween) {
    tween->elapsedTime = 0.0f;
    tween->targetPositionX = tween->cameraCenterX;
    tween->targetPositionY = tween->cameraCenterY;
    tween->targetPositionZ = tween->cameraCenterZ;
    tween->targetRotationX = tween->cameraRotationX;
    tween->targetRotationY = tween->cameraRotationY;
    tween->targetRotationZ = tween->cameraRotationZ;
    tween->targetDistance = tween->cameraZoom;
}

void InterpolateTransformToward(TransformTween* currentTween, TransformTween* targetTween, float blendFrames) {
    currentTween->cameraCenterX = ApproachValueOverDuration(currentTween->cameraCenterX, targetTween->targetPositionX, blendFrames);
    currentTween->cameraCenterY = ApproachValueOverDuration(currentTween->cameraCenterY, targetTween->targetPositionY, blendFrames);
    currentTween->cameraCenterZ = ApproachValueOverDuration(currentTween->cameraCenterZ, targetTween->targetPositionZ, blendFrames);
    currentTween->cameraRotationX = ApproachValueOverDuration(currentTween->cameraRotationX, targetTween->targetRotationX, blendFrames);
    currentTween->cameraRotationY = ApproachValueOverDuration(currentTween->cameraRotationY, targetTween->targetRotationY, blendFrames);
    currentTween->cameraRotationZ = ApproachValueOverDuration(currentTween->cameraRotationZ, targetTween->targetRotationZ, blendFrames);
    currentTween->cameraZoom = ApproachValueOverDuration(currentTween->cameraZoom, targetTween->targetDistance, blendFrames);
}

void InterpolateTransformTowardSelf(TransformTween* tween, float blendFrames) {
    InterpolateTransformToward(tween, tween, blendFrames);
}

void InterpolateTransformLinear(TransformTween* startTween, TransformTween* targetTween, float elapsedFrames, float durationFrames) {
    targetTween->targetPositionX = InterpolateLinearClamped(startTween->targetPositionX, targetTween->cameraCenterX, elapsedFrames, durationFrames);
    targetTween->targetPositionY = InterpolateLinearClamped(startTween->targetPositionY, targetTween->cameraCenterY, elapsedFrames, durationFrames);
    targetTween->targetPositionZ = InterpolateLinearClamped(startTween->targetPositionZ, targetTween->cameraCenterZ, elapsedFrames, durationFrames);
    targetTween->targetRotationX = InterpolateLinearClamped(startTween->targetRotationX, targetTween->cameraRotationX, elapsedFrames, durationFrames);
    targetTween->targetRotationY = InterpolateLinearClamped(startTween->targetRotationY, targetTween->cameraRotationY, elapsedFrames, durationFrames);
    targetTween->targetRotationZ = InterpolateLinearClamped(startTween->targetRotationZ, targetTween->cameraRotationZ, elapsedFrames, durationFrames);
    targetTween->targetDistance = InterpolateLinearClamped(startTween->targetDistance, targetTween->cameraZoom, elapsedFrames, durationFrames);
}

void InterpolateTransformLinearAndStore(TransformTween* startTween, TransformTween* targetTween, float elapsedFrames, float durationFrames) {
    InterpolateTransformLinear(startTween, targetTween, elapsedFrames, durationFrames);
    startTween->cameraCenterX = targetTween->targetPositionX;
    startTween->cameraCenterY = targetTween->targetPositionY;
    startTween->cameraCenterZ = targetTween->targetPositionZ;
    startTween->cameraRotationX = targetTween->targetRotationX;
    startTween->cameraRotationY = targetTween->targetRotationY;
    startTween->cameraRotationZ = targetTween->targetRotationZ;
    startTween->cameraZoom = targetTween->targetDistance;
}

void InterpolateTransformLinearAndBlend(TransformTween* startTween, TransformTween* targetTween, float elapsedFrames, float durationFrames, float blendFrames) {
    InterpolateTransformLinear(startTween, targetTween, elapsedFrames, durationFrames);
    InterpolateTransformToward(startTween, targetTween, blendFrames);
}

void InterpolateTransformSine(TransformTween* startTween, TransformTween* targetTween, float elapsedFrames, float durationFrames) {
    targetTween->targetPositionX = InterpolateSineEaseOut(startTween->targetPositionX, targetTween->cameraCenterX, elapsedFrames, durationFrames);
    targetTween->targetPositionY = InterpolateSineEaseOut(startTween->targetPositionY, targetTween->cameraCenterY, elapsedFrames, durationFrames);
    targetTween->targetPositionZ = InterpolateSineEaseOut(startTween->targetPositionZ, targetTween->cameraCenterZ, elapsedFrames, durationFrames);
    targetTween->targetRotationX = InterpolateSineEaseOut(startTween->targetRotationX, targetTween->cameraRotationX, elapsedFrames, durationFrames);
    targetTween->targetRotationY = InterpolateSineEaseOut(startTween->targetRotationY, targetTween->cameraRotationY, elapsedFrames, durationFrames);
    targetTween->targetRotationZ = InterpolateSineEaseOut(startTween->targetRotationZ, targetTween->cameraRotationZ, elapsedFrames, durationFrames);
    targetTween->targetDistance = InterpolateSineEaseOut(startTween->targetDistance, targetTween->cameraZoom, elapsedFrames, durationFrames);
}

void InterpolateTransformSineAndStore(TransformTween* startTween, TransformTween* targetTween, float elapsedFrames, float durationFrames) {
    InterpolateTransformSine(startTween, targetTween, elapsedFrames, durationFrames);
    startTween->cameraCenterX = targetTween->targetPositionX;
    startTween->cameraCenterY = targetTween->targetPositionY;
    startTween->cameraCenterZ = targetTween->targetPositionZ;
    startTween->cameraRotationX = targetTween->targetRotationX;
    startTween->cameraRotationY = targetTween->targetRotationY;
    startTween->cameraRotationZ = targetTween->targetRotationZ;
    startTween->cameraZoom = targetTween->targetDistance;
}

void InterpolateTransformSineAndBlend(TransformTween* startTween, TransformTween* targetTween, float elapsedFrames, float durationFrames, float blendFrames) {
    InterpolateTransformSine(startTween, targetTween, elapsedFrames, durationFrames);
    InterpolateTransformToward(startTween, targetTween, blendFrames);
}

void InterpolateTransformCosine(TransformTween* startTween, TransformTween* targetTween, float elapsedFrames, float durationFrames) {
    targetTween->targetPositionX = InterpolateCosineEaseIn(startTween->targetPositionX, targetTween->cameraCenterX, elapsedFrames, durationFrames);
    targetTween->targetPositionY = InterpolateCosineEaseIn(startTween->targetPositionY, targetTween->cameraCenterY, elapsedFrames, durationFrames);
    targetTween->targetPositionZ = InterpolateCosineEaseIn(startTween->targetPositionZ, targetTween->cameraCenterZ, elapsedFrames, durationFrames);
    targetTween->targetRotationX = InterpolateCosineEaseIn(startTween->targetRotationX, targetTween->cameraRotationX, elapsedFrames, durationFrames);
    targetTween->targetRotationY = InterpolateCosineEaseIn(startTween->targetRotationY, targetTween->cameraRotationY, elapsedFrames, durationFrames);
    targetTween->targetRotationZ = InterpolateCosineEaseIn(startTween->targetRotationZ, targetTween->cameraRotationZ, elapsedFrames, durationFrames);
    targetTween->targetDistance = InterpolateCosineEaseIn(startTween->targetDistance, targetTween->cameraZoom, elapsedFrames, durationFrames);
}

void InterpolateTransformCosineAndStore(TransformTween* startTween, TransformTween* targetTween, float elapsedFrames, float durationFrames) {
    InterpolateTransformCosine(startTween, targetTween, elapsedFrames, durationFrames);
    startTween->cameraCenterX = targetTween->targetPositionX;
    startTween->cameraCenterY = targetTween->targetPositionY;
    startTween->cameraCenterZ = targetTween->targetPositionZ;
    startTween->cameraRotationX = targetTween->targetRotationX;
    startTween->cameraRotationY = targetTween->targetRotationY;
    startTween->cameraRotationZ = targetTween->targetRotationZ;
    startTween->cameraZoom = targetTween->targetDistance;
}

void InterpolateTransformCosineAndBlend(TransformTween* startTween, TransformTween* targetTween, float elapsedFrames, float durationFrames, float blendFrames) {
    InterpolateTransformCosine(startTween, targetTween, elapsedFrames, durationFrames);
    InterpolateTransformToward(startTween, targetTween, blendFrames);
}

void UpdateModelTransformTween(OMOBJ* modelObject, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 rotationSmoothingFrames, s32 elapsedFrames, s32 durationFrames, s32 updatePosition, s32 updateRotation) {
    Vec startPosition;
    Vec targetPosition;
    Vec interpolatedPosition;
    float currentRotation;
    float targetRotation;
    float interpolationFrame;
    float interpolationDuration;

    startPosition.x = startPosition->x;
    startPosition.y = startPosition->y;
    startPosition.z = startPosition->z;
    targetPosition.x = targetPosition->x;
    targetPosition.y = targetPosition->y;
    targetPosition.z = targetPosition->z;
    currentRotation = Hu3DData[modelObject->mdlId[modelIndex]].rot.y;
    if (currentRotation < 0.0f) {
        currentRotation += 360.0f;
    } else if (currentRotation >= 360.0f) {
        currentRotation -= 360.0f;
    }
    interpolationFrame = elapsedFrames;
    interpolationDuration = durationFrames;
    if (updatePosition != 0) {
        interpolatedPosition.x = InterpolateLinearClamped(startPosition.x, targetPosition.x, interpolationFrame, interpolationDuration);
        interpolatedPosition.y = InterpolateLinearClamped(startPosition.y, targetPosition.y, interpolationFrame, interpolationDuration);
        interpolatedPosition.z = InterpolateLinearClamped(startPosition.z, targetPosition.z, interpolationFrame, interpolationDuration);
        if (modelIndex == 0) {
            omSetTra(modelObject, interpolatedPosition.x, interpolatedPosition.y, interpolatedPosition.z);
        } else {
            Hu3DModelPosSet(modelObject->mdlId[modelIndex], interpolatedPosition.x, interpolatedPosition.y, interpolatedPosition.z);
        }
    }
    if (updateRotation == 0) {
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
    currentRotation = (targetRotation + currentRotation * (rotationSmoothingFrames - 1)) / rotationSmoothingFrames;
    if (modelIndex == 0) {
        modelObject->rot.y = currentRotation;
    } else {
        Hu3DData[modelObject->mdlId[modelIndex]].rot.y = currentRotation;
    }
}

void UpdateModelTransformTweenPosition(OMOBJ* modelObject, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 unusedRotationFrames, s32 elapsedFrames, s32 durationFrames) {
    UpdateModelTransformTween(modelObject, modelIndex, startPosition, targetPosition, 0, elapsedFrames, durationFrames, 1, 0);
}

void UpdateModelTransformTweenRotation(OMOBJ* modelObject, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 rotationSmoothingFrames, s32 elapsedFrames, s32 durationFrames) {
    UpdateModelTransformTween(modelObject, modelIndex, startPosition, targetPosition, rotationSmoothingFrames, elapsedFrames, durationFrames, 0, 1);
}

void UpdateModelTransformTweenPositionAndRotation(OMOBJ* modelObject, s32 modelIndex, Vec* startPosition, Vec* targetPosition, s32 rotationSmoothingFrames, s32 elapsedFrames, s32 durationFrames) {
    UpdateModelTransformTween(modelObject, modelIndex, startPosition, targetPosition, rotationSmoothingFrames, elapsedFrames, durationFrames, 1, 1);
}

void UpdateModelRotationTween(OMOBJ* modelObject, s32 modelIndex, float targetRotation, s32 smoothingFrames) {
    float currentRotation;
    float targetRotation;

    currentRotation = Hu3DData[modelObject->mdlId[modelIndex]].rot.y;
    if (currentRotation < 0.0f) {
        currentRotation += 360.0f;
    } else if (currentRotation >= 360.0f) {
        currentRotation -= 360.0f;
    }
    targetRotation = targetRotation;
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
    currentRotation = (targetRotation + currentRotation * (smoothingFrames - 1)) / smoothingFrames;
    if (modelIndex == 0) {
        modelObject->rot.y = currentRotation;
    } else {
        Hu3DData[modelObject->mdlId[modelIndex]].rot.y = currentRotation;
    }
}

void ReserveMotionTarget(MotionTargetState* trackingState, s32 modelId) {
    s32 availableTargetSlot;
    s32 targetSlotIndex;

    availableTargetSlot = -1;
    for (targetSlotIndex = 0; targetSlotIndex < 20; targetSlotIndex++) {
        if (trackingState->targetModelIds[targetSlotIndex] == -1) {
            availableTargetSlot = targetSlotIndex;
            break;
        }
    }
    if (availableTargetSlot != -1) {
        trackingState->targetModelIds[availableTargetSlot] = modelId;
    }
}

void ReleaseMotionTarget(MotionTargetState* trackingState, s32 modelId) {
    s32 i;

    for (i = 0; i < 20; i++) {
        if (modelId == -1) {
            trackingState->targetModelIds[i] = -1;
        } else if (trackingState->targetModelIds[i] == modelId) {
            trackingState->targetModelIds[i] = -1;
        }
    }
}

void SetMotionTargetModel(MotionTargetState* trackingState, s32 modelIndex) {
    trackingState->targetMotionIndex = modelIndex;
}

void ClearMotionTargetModel(MotionTargetState* trackingState, s32 unusedModelIndex) {
    trackingState->targetMotionIndex = -1;
}

void ResetMotionTarget(MotionTargetState* trackingState, s32 newTrackingMode) {
    trackingState->trackingState = newTrackingMode;
}

float GetMotionTargetValue(s32 sourceModelId, s32 targetModelId, s32 rotationDirection) {
    Vec sourcePosition;
    Vec targetPosition;
    Vec positionDifference;
    float relativeAngle;
    float sourceRotation;

    sourcePosition.x = Hu3DData[sourceModelId].pos.x;
    sourcePosition.z = Hu3DData[sourceModelId].pos.z;
    targetPosition.x = Hu3DData[targetModelId].pos.x;
    targetPosition.z = Hu3DData[targetModelId].pos.z;
    positionDifference.x = targetPosition.x - sourcePosition.x;
    positionDifference.z = targetPosition.z - sourcePosition.z;
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
    relativeAngle = atan2d(positionDifference.z, positionDifference.x) - 90.0;
    if (rotationDirection == 0) {
        relativeAngle *= -1.0f;
    }
    if (rotationDirection != 0) {
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

float GetMotionTargetValueByIndex(s32 sourceModelId, s32 targetModelId) {
    Vec sourcePosition;
    Vec targetPosition;
    Vec positionDifference;

    sourcePosition.x = Hu3DData[sourceModelId].pos.x;
    sourcePosition.z = Hu3DData[sourceModelId].pos.z;
    targetPosition.x = Hu3DData[targetModelId].pos.x;
    targetPosition.z = Hu3DData[targetModelId].pos.z;
    positionDifference.x = targetPosition.x - sourcePosition.x;
    positionDifference.z = targetPosition.z - sourcePosition.z;
    return sqrtf(positionDifference.x * positionDifference.x + positionDifference.z * positionDifference.z);
}

void UpdateMotionTarget(OMOBJ* trackingObject) {
    MotionTargetState* trackingWork;
    float targetRotation;
    float bestRotationDifference;
    float targetDistance;
    float nearestTargetDistance;
    s32 targetIndex;

    trackingWork = trackingObject->data;
    targetRotation = 0.0f;
    bestRotationDifference = 0.0f;
    targetDistance = 0.0f;
    nearestTargetDistance = trackingWork->targetValue;
    if (trackingWork->trackingState != 11) {
        if (trackingWork->trackingState == 0 || trackingWork->trackingState == 10) {
            trackingWork->trackingState = 10;
            trackingWork->targetElapsedTime = ApproachValueOverDuration(trackingWork->targetElapsedTime, 0.0f, 5.0f);
            Hu3DMotionForceSet(trackingWork->motionId, trackingWork->motionName, trackingWork->targetModelIndex, trackingWork->targetElapsedTime);
            if (trackingWork->targetCount++ >= 10) {
                trackingWork->targetCount = 0;
                trackingWork->trackingState = 11;
                Hu3DMotionNoMotReset(trackingWork->motionId, trackingWork->motionName, trackingWork->targetModelIndex);
            }
            return;
        }
        if (trackingWork->targetMotionIndex >= 0) {
            targetRotation = GetMotionTargetValue(trackingWork->motionId, trackingWork->targetMotionIndex, trackingWork->targetState);
            if (targetRotation >= trackingWork->targetDuration || targetRotation <= -trackingWork->targetDuration) {
                targetRotation = 0.0f;
            }
            trackingWork->targetElapsedTime = ApproachValueOverDuration(trackingWork->targetElapsedTime, targetRotation, trackingWork->targetStartValue);
        } else {
            for (targetIndex = 0; targetIndex < 20; targetIndex++) {
                if (trackingWork->targetModelIds[targetIndex] < 0) {
                    continue;
                }
                targetDistance = GetMotionTargetValueByIndex(trackingWork->motionId, trackingWork->targetModelIds[targetIndex]);
                if (targetDistance <= trackingWork->targetValue) {
                    targetRotation = GetMotionTargetValue(trackingWork->motionId, trackingWork->targetModelIds[targetIndex], trackingWork->targetState);
                    if (targetRotation <= trackingWork->targetDuration && targetRotation >= -trackingWork->targetDuration && targetDistance <= nearestTargetDistance) {
                        bestRotationDifference = targetRotation;
                        nearestTargetDistance = targetDistance;
                    }
                }
            }
            trackingWork->targetElapsedTime = ApproachValueOverDuration(trackingWork->targetElapsedTime, bestRotationDifference, trackingWork->targetStartValue);
        }
        Hu3DMotionForceSet(trackingWork->motionId, trackingWork->motionName, trackingWork->targetModelIndex, trackingWork->targetElapsedTime);
    }
}

void RunMotionTargetProcess(HUPROCESS* objectManager, MotionTargetState* trackingState, s32 motionId, char* motionName, s32 targetModelIndex, s32 trackingMode) {
    ResetMotionTarget(trackingState, 1);
    trackingState->motionId = motionId;
    strcpy(trackingState->motionName, motionName);
    ClearMotionTargetModel(trackingState, -1);
    ReleaseMotionTarget(trackingState, -1);
    trackingState->targetStartValue = 5.0f;
    trackingState->targetValue = 10000.0f;
    trackingState->targetDuration = 90.0f;
    trackingState->targetCount = 0;
    trackingState->targetElapsedTime = 0.0f;
    trackingState->targetModelIndex = targetModelIndex;
    trackingState->targetState = trackingMode;
    trackingState->trackingObject = omAddObjEx(objectManager, 0x3000, 0, 0, -1, UpdateMotionTarget);
    trackingState->trackingObject->data = trackingState;
}
