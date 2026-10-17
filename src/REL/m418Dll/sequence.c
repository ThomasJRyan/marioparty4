#include "REL/m418Dll.h"

f32 InterpolateWeightedAverage(f32 startValue, f32 targetValue, f32 elapsedFrames) {
    return (targetValue + (startValue * (elapsedFrames - 1.0f))) / elapsedFrames;
}

f32 InterpolateLinear(f32 startValue, f32 targetValue, f32 elapsedFrames, f32 totalFrames) {
    if (elapsedFrames >= totalFrames) {
        return targetValue;
    }
    return startValue + ((elapsedFrames / totalFrames) * (targetValue - startValue));
}

f32 InterpolateSineEaseOut(f32 startValue, f32 targetValue, f32 elapsedFrames, f32 totalFrames) {
    if (elapsedFrames >= totalFrames) {
        return targetValue;
    }
    return startValue + (targetValue - startValue) * sind(((90.0f / totalFrames) * elapsedFrames));
}

f32 InterpolateSineEaseInOut(f32 startValue, f32 targetValue, f32 elapsedFrames, f32 totalFrames) {
    if (elapsedFrames >= totalFrames) {
        return startValue;
    }
    return startValue + (targetValue - startValue) * sind((180.0f / totalFrames) * elapsedFrames);
}

f32 InterpolateCosineEaseOut(f32 startValue, f32 targetValue, f32 elapsedFrames, f32 totalFrames) {
    if (elapsedFrames >= totalFrames) {
        return targetValue;
    }
    return (startValue + (targetValue - startValue) * (1.0 - cosd((90.0f / totalFrames) * elapsedFrames)));
}

void SetModelMotion(OMOBJ* object, s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 shiftMode) {
    if (shiftMode == 1) {
        Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, blendFrames, 0x40000001);
        return;
    }
    if (shiftMode == 2) {
        Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, blendFrames, 0x40000002);
        return;
    }
    Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, blendFrames, 0);
}

void SetModelMotionIfChanged(OMOBJ* object, s32 modelIndex, s32 motionIndex, s32 blendFrames, s32 shiftMode) {
    if (object->work[3] != object->mtnId[motionIndex]) {
        object->work[3] = object->mtnId[motionIndex];
        if (shiftMode == 1) {
            Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, blendFrames, 0x40000001);
            return;
        }
        if (shiftMode == 2) {
            Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, blendFrames, 0x40000002);
            return;
        }
        Hu3DMotionShiftSet(object->mdlId[modelIndex], object->mtnId[motionIndex], 0.0f, blendFrames, 0);
    }
}

void LoadCurrentSequenceEntry(M418DllSequenceState* sequenceState) {
    sequenceState->elapsedFrames = 0;
    sequenceState->entryDuration = sequenceState->entries[sequenceState->entryIndex].duration;
    sequenceState->activeCallback = (M418DllFunc)sequenceState->entries[sequenceState->entryIndex].inputCallback;
    sequenceState->activeResultCallback = (M418DllRetFunc)sequenceState->entries[sequenceState->entryIndex].resultCallback;
}

void InitializeSequence(M418DllSequenceState* sequenceState, M418DllSequenceEntry* sequenceEntries) {
    sequenceState->entryIndex = 0;
    sequenceState->entries = sequenceEntries;
    LoadCurrentSequenceEntry(sequenceState);
}

s32 AdvanceSequence(M418DllSequenceState* sequenceState) {
    sequenceState->entryIndex++;
    LoadCurrentSequenceEntry(sequenceState);
    if ((sequenceState->activeCallback == NULL) && (sequenceState->activeResultCallback == NULL)) {
        return 1;
    }
    return 0;
}

s32 RewindSequence(M418DllSequenceState* sequenceState) {
    sequenceState->entryIndex--;
    if (sequenceState->entryIndex < 0) {
        return 1;
    }
    LoadCurrentSequenceEntry(sequenceState);
    return 0;
}

s32 SetSequenceIndex(M418DllSequenceState* sequenceState, s32 entryIndex) {
    sequenceState->entryIndex = entryIndex;
    if (sequenceState->entryIndex < 0) {
        return 1;
    }
    LoadCurrentSequenceEntry(sequenceState);
    if ((sequenceState->activeCallback == NULL) && (sequenceState->activeResultCallback == NULL)) {
        return 1;
    }
    return 0;
}

s32 AdvanceSequenceTimer(M418DllSequenceState* sequenceState, s32 frameIncrement) {
    if (frameIncrement == 0) {
        return sequenceState->elapsedFrames;
    }
    sequenceState->elapsedFrames += frameIncrement;
    if (sequenceState->elapsedFrames >= sequenceState->entryDuration) {
        return 1;
    }
    return 0;
}

void SetCameraTargetFromPositions(M418DllCameraState* cameraState, Vec cameraPosition, Vec focusPosition) {
    f32 deltaX;
    f32 deltaZ;
    f32 deltaY;
    f32 horizontalAngle;

    deltaX = cameraPosition.x - focusPosition.x;
    deltaY = cameraPosition.y - focusPosition.y;
    deltaZ = cameraPosition.z - focusPosition.z;
    cameraState->targetCenter.x = focusPosition.x;
    cameraState->targetCenter.y = focusPosition.y;
    cameraState->targetCenter.z = focusPosition.z;
    cameraState->targetZoom = sqrtf((deltaZ * deltaZ) + ((deltaX * deltaX) + (deltaY * deltaY)));
    horizontalAngle = atan2d(deltaZ, deltaX);
    cameraState->targetRotation.x = -atan2d(deltaY, (deltaX * cosd(-horizontalAngle)) + (deltaZ * -sind(-horizontalAngle)));
    cameraState->targetRotation.y = -(horizontalAngle - 90.0f);
    cameraState->targetRotation.z = 0.0f;
}

void ApplyCameraTarget(M418DllCameraState* cameraState) {
    cameraState->center.x = cameraState->targetCenter.x;
    cameraState->center.y = cameraState->targetCenter.y;
    cameraState->center.z = cameraState->targetCenter.z;
    cameraState->rot.x = cameraState->targetRotation.x;
    cameraState->rot.y = cameraState->targetRotation.y;
    cameraState->rot.z = cameraState->targetRotation.z;
    cameraState->zoom = cameraState->targetZoom;
}

void ResetCameraTarget(M418DllCameraState* cameraState) {
    cameraState->interpolationFactor = 0.0f;
    cameraState->targetCenter.x = cameraState->center.x;
    cameraState->targetCenter.y = cameraState->center.y;
    cameraState->targetCenter.z = cameraState->center.z;
    cameraState->targetRotation.x = cameraState->rot.x;
    cameraState->targetRotation.y = cameraState->rot.y;
    cameraState->targetRotation.z = cameraState->rot.z;
    cameraState->targetZoom = cameraState->zoom;
}

void InterpolateCameraTarget(M418DllCameraState* startValue, M418DllCameraState* targetValue, f32 elapsedFrames) {
    startValue->center.x = InterpolateWeightedAverage(startValue->center.x, targetValue->targetCenter.x, elapsedFrames);
    startValue->center.y = InterpolateWeightedAverage(startValue->center.y, targetValue->targetCenter.y, elapsedFrames);
    startValue->center.z = InterpolateWeightedAverage(startValue->center.z, targetValue->targetCenter.z, elapsedFrames);
    startValue->rot.x = InterpolateWeightedAverage(startValue->rot.x, targetValue->targetRotation.x, elapsedFrames);
    startValue->rot.y = InterpolateWeightedAverage(startValue->rot.y, targetValue->targetRotation.y, elapsedFrames);
    startValue->rot.z = InterpolateWeightedAverage(startValue->rot.z, targetValue->targetRotation.z, elapsedFrames);
    startValue->zoom = InterpolateWeightedAverage(startValue->zoom, targetValue->targetZoom, elapsedFrames);
}

void ApproachCameraTarget(M418DllCameraState* cameraState, f32 smoothingFactor) {
    cameraState->center.x = InterpolateWeightedAverage(cameraState->center.x, cameraState->targetCenter.x, smoothingFactor);
    cameraState->center.y = InterpolateWeightedAverage(cameraState->center.y, cameraState->targetCenter.y, smoothingFactor);
    cameraState->center.z = InterpolateWeightedAverage(cameraState->center.z, cameraState->targetCenter.z, smoothingFactor);
    cameraState->rot.x = InterpolateWeightedAverage(cameraState->rot.x, cameraState->targetRotation.x, smoothingFactor);
    cameraState->rot.y = InterpolateWeightedAverage(cameraState->rot.y, cameraState->targetRotation.y, smoothingFactor);
    cameraState->rot.z = InterpolateWeightedAverage(cameraState->rot.z, cameraState->targetRotation.z, smoothingFactor);
    cameraState->zoom = InterpolateWeightedAverage(cameraState->zoom, cameraState->targetZoom, smoothingFactor);
}

void InterpolateCameraStateEaseOut(M418DllCameraState* startValue, M418DllCameraState* targetValue, f32 elapsedFrames, f32 totalFrames) {
    targetValue->targetCenter.x = InterpolateLinear(startValue->targetCenter.x, targetValue->center.x, elapsedFrames, totalFrames);
    targetValue->targetCenter.y = InterpolateLinear(startValue->targetCenter.y, targetValue->center.y, elapsedFrames, totalFrames);
    targetValue->targetCenter.z = InterpolateLinear(startValue->targetCenter.z, targetValue->center.z, elapsedFrames, totalFrames);
    targetValue->targetRotation.x = InterpolateLinear(startValue->targetRotation.x, targetValue->rot.x, elapsedFrames, totalFrames);
    targetValue->targetRotation.y = InterpolateLinear(startValue->targetRotation.y, targetValue->rot.y, elapsedFrames, totalFrames);
    targetValue->targetRotation.z = InterpolateLinear(startValue->targetRotation.z, targetValue->rot.z, elapsedFrames, totalFrames);
    targetValue->targetZoom = InterpolateLinear(startValue->targetZoom, targetValue->zoom, elapsedFrames, totalFrames);
}

void ApplyInterpolatedCameraStateEaseOut(M418DllCameraState* sourceCamera, M418DllCameraState* targetCamera, f32 elapsedFrames, f32 totalFrames) {
    InterpolateCameraStateEaseOut(sourceCamera, targetCamera, elapsedFrames, totalFrames);
    sourceCamera->center.x = targetCamera->targetCenter.x;
    sourceCamera->center.y = targetCamera->targetCenter.y;
    sourceCamera->center.z = targetCamera->targetCenter.z;
    sourceCamera->rot.x = targetCamera->targetRotation.x;
    sourceCamera->rot.y = targetCamera->targetRotation.y;
    sourceCamera->rot.z = targetCamera->targetRotation.z;
    sourceCamera->zoom = targetCamera->targetZoom;
}

void TransitionCameraStateEaseOut(M418DllCameraState* sourceCamera, M418DllCameraState* targetCamera, f32 elapsedFrames, f32 totalFrames, f32 smoothingFactor) {
    InterpolateCameraStateEaseOut(sourceCamera, targetCamera, elapsedFrames, totalFrames);
    InterpolateCameraTarget(sourceCamera, targetCamera, smoothingFactor);
}

void InterpolateCameraStateSine(M418DllCameraState* startValue, M418DllCameraState* targetValue, f32 elapsedFrames, f32 totalFrames) {
    targetValue->targetCenter.x = InterpolateSineEaseOut(startValue->targetCenter.x, targetValue->center.x, elapsedFrames, totalFrames);
    targetValue->targetCenter.y = InterpolateSineEaseOut(startValue->targetCenter.y, targetValue->center.y, elapsedFrames, totalFrames);
    targetValue->targetCenter.z = InterpolateSineEaseOut(startValue->targetCenter.z, targetValue->center.z, elapsedFrames, totalFrames);
    targetValue->targetRotation.x = InterpolateSineEaseOut(startValue->targetRotation.x, targetValue->rot.x, elapsedFrames, totalFrames);
    targetValue->targetRotation.y = InterpolateSineEaseOut(startValue->targetRotation.y, targetValue->rot.y, elapsedFrames, totalFrames);
    targetValue->targetRotation.z = InterpolateSineEaseOut(startValue->targetRotation.z, targetValue->rot.z, elapsedFrames, totalFrames);
    targetValue->targetZoom = InterpolateSineEaseOut(startValue->targetZoom, targetValue->zoom, elapsedFrames, totalFrames);
}

void ApplyInterpolatedCameraStateSine(M418DllCameraState* sourceCamera, M418DllCameraState* targetCamera, f32 elapsedFrames, f32 totalFrames) {
    InterpolateCameraStateSine(sourceCamera, targetCamera, elapsedFrames, totalFrames);
    sourceCamera->center.x = targetCamera->targetCenter.x;
    sourceCamera->center.y = targetCamera->targetCenter.y;
    sourceCamera->center.z = targetCamera->targetCenter.z;
    sourceCamera->rot.x = targetCamera->targetRotation.x;
    sourceCamera->rot.y = targetCamera->targetRotation.y;
    sourceCamera->rot.z = targetCamera->targetRotation.z;
    sourceCamera->zoom = targetCamera->targetZoom;
}

void TransitionCameraStateSine(M418DllCameraState* sourceCamera, M418DllCameraState* targetCamera, f32 elapsedFrames, f32 totalFrames, f32 smoothingFactor) {
    InterpolateCameraStateSine(sourceCamera, targetCamera, elapsedFrames, totalFrames);
    InterpolateCameraTarget(sourceCamera, targetCamera, smoothingFactor);
}

void InterpolateCameraStateCosine(M418DllCameraState* startValue, M418DllCameraState* targetValue, f32 elapsedFrames, f32 totalFrames) {
    targetValue->targetCenter.x = InterpolateCosineEaseOut(startValue->targetCenter.x, targetValue->center.x, elapsedFrames, totalFrames);
    targetValue->targetCenter.y = InterpolateCosineEaseOut(startValue->targetCenter.y, targetValue->center.y, elapsedFrames, totalFrames);
    targetValue->targetCenter.z = InterpolateCosineEaseOut(startValue->targetCenter.z, targetValue->center.z, elapsedFrames, totalFrames);
    targetValue->targetRotation.x = InterpolateCosineEaseOut(startValue->targetRotation.x, targetValue->rot.x, elapsedFrames, totalFrames);
    targetValue->targetRotation.y = InterpolateCosineEaseOut(startValue->targetRotation.y, targetValue->rot.y, elapsedFrames, totalFrames);
    targetValue->targetRotation.z = InterpolateCosineEaseOut(startValue->targetRotation.z, targetValue->rot.z, elapsedFrames, totalFrames);
    targetValue->targetZoom = InterpolateCosineEaseOut(startValue->targetZoom, targetValue->zoom, elapsedFrames, totalFrames);
}

void ApplyInterpolatedCameraStateCosine(M418DllCameraState* sourceCamera, M418DllCameraState* targetCamera, f32 elapsedFrames, f32 totalFrames) {
    InterpolateCameraStateCosine(sourceCamera, targetCamera, elapsedFrames, totalFrames);
    sourceCamera->center.x = targetCamera->targetCenter.x;
    sourceCamera->center.y = targetCamera->targetCenter.y;
    sourceCamera->center.z = targetCamera->targetCenter.z;
    sourceCamera->rot.x = targetCamera->targetRotation.x;
    sourceCamera->rot.y = targetCamera->targetRotation.y;
    sourceCamera->rot.z = targetCamera->targetRotation.z;
    sourceCamera->zoom = targetCamera->targetZoom;
}

void TransitionCameraStateCosine(M418DllCameraState* sourceCamera, M418DllCameraState* targetCamera, f32 elapsedFrames, f32 totalFrames, f32 smoothingFactor) {
    InterpolateCameraStateCosine(sourceCamera, targetCamera, elapsedFrames, totalFrames);
    InterpolateCameraTarget(sourceCamera, targetCamera, smoothingFactor);
}
