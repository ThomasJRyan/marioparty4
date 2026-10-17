#include "REL/m446Dll.h"

#include "math.h"
#include "ext_math.h"
#include "game/hu3d.h"
#include "game/printfunc.h"
#include "game/pad.h"

// bss
Vec m446CameraLookAt;
Vec m446CameraAngles;
f32 m446CameraDistance;
Vec m446CameraTargetPosition;
Vec m446CameraTargetAngles;
f32 m446CameraTargetDistance;
Vec m446CameraStartTarget;
Vec m446CameraStartAngles;
f32 m446CameraStartDistance;
f32 m446CameraTransitionProgress;
f32 m446CameraTransitionStep;
s32 m446CameraTransitionActive;

// data
Vec m446CameraWorldUp = {0, 1, 0};

s32 M446CameraInitialize(void) {
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 30.0f, 10.0f, 8000.0f, 1.2f);
    m446CameraLookAt.x = 0.0f;
    m446CameraLookAt.y = 0.0f;
    m446CameraLookAt.z = 0.0f;
    m446CameraAngles.x = 0.0f;
    m446CameraAngles.y = 0.0f;
    m446CameraAngles.z = 0.0f;
    m446CameraDistance = 2000.0f;
    m446CameraTransitionActive = 0;
    return 1;
}

void M446CameraTransitionStop(void) {
    return;
}

void M446CameraUpdate(void) {
    Vec deckPosition;
    Vec velocity;
    Vec rotation;
    f32 scaleVector;
    f32 angle;
    f32 scale;
    f32 distance;

    if (m446CameraTransitionActive != 0) {
        distance = sind(90.0f * m446CameraTransitionProgress);
        distance *= distance;
        m446CameraLookAt.x = m446CameraStartTarget.x + (distance * (m446CameraTargetPosition.x - m446CameraStartTarget.x));
        m446CameraLookAt.y = m446CameraStartTarget.y + (distance * (m446CameraTargetPosition.y - m446CameraStartTarget.y));
        m446CameraLookAt.z = m446CameraStartTarget.z + (distance * (m446CameraTargetPosition.z - m446CameraStartTarget.z));
        m446CameraAngles.x = m446CameraStartAngles.x + (distance * (m446CameraTargetAngles.x - m446CameraStartAngles.x));
        m446CameraAngles.y = m446CameraStartAngles.y + (distance * (m446CameraTargetAngles.y - m446CameraStartAngles.y));
        m446CameraAngles.z = m446CameraStartAngles.z + (distance * (m446CameraTargetAngles.z - m446CameraStartAngles.z));
        m446CameraDistance = m446CameraStartDistance + (distance * (m446CameraTargetDistance - m446CameraStartDistance));
        m446CameraTransitionProgress += m446CameraTransitionStep;
        if (m446CameraTransitionProgress >= 1.0f) {
            m446CameraLookAt = m446CameraTargetPosition;
            m446CameraAngles = m446CameraTargetAngles;
            m446CameraDistance = m446CameraTargetDistance;
            m446CameraTransitionActive = 0;
        }
    }
    scale = m446CameraAngles.x;
    angle = m446CameraAngles.y;
    scaleVector = m446CameraAngles.z;
    deckPosition.x = (m446CameraLookAt.x + (m446CameraDistance * (sind(angle) * cosd(scale))));
    deckPosition.y = (m446CameraLookAt.y + (m446CameraDistance * -sind(scale)));
    deckPosition.z = (m446CameraLookAt.z + (m446CameraDistance * (cosd(angle) * cosd(scale))));
    velocity.x = m446CameraLookAt.x;
    velocity.y = m446CameraLookAt.y;
    velocity.z = m446CameraLookAt.z;
    rotation.x = (sind(angle) * sind(scale));
    rotation.y = cosd(scale);
    rotation.z = (cosd(angle) * sind(scale));
    Hu3DCameraPosSet(1, deckPosition.x, deckPosition.y, deckPosition.z, rotation.x, rotation.y, rotation.z, velocity.x, velocity.y, velocity.z);
}

void M446CameraTargetSet(Vec* lookAtPosition) {
    m446CameraLookAt = *lookAtPosition;
}

void M446CameraRotationSet(Vec* cameraAngles) {
    m446CameraAngles = *cameraAngles;
}

void M446CameraDistanceSet(f32 cameraDistance) {
    m446CameraDistance = cameraDistance;
}

void M446CameraTransitionBegin(Vec* targetPosition, Vec* targetAngles, f32* targetDistance, s32 durationFrames) {
    Vec* loopIndex;
    Vec* playerIndex;
    f32 movementAngle;

    if (targetPosition) {
        loopIndex = targetPosition;
    } else {
        loopIndex = &m446CameraLookAt;
    }
    m446CameraTargetPosition = *loopIndex;
    if (targetAngles) {
        playerIndex = targetAngles;
    } else {
        playerIndex = &m446CameraAngles;
    }
    m446CameraTargetAngles = *playerIndex;
    if (targetDistance) {
        movementAngle = *targetDistance;
    } else {
        movementAngle = m446CameraDistance;
    }
    m446CameraTargetDistance = movementAngle;
    m446CameraStartTarget = m446CameraLookAt;
    m446CameraStartAngles = m446CameraAngles;
    m446CameraStartDistance = m446CameraDistance;
    m446CameraTransitionProgress = 0.0f;
    m446CameraTransitionStep = 1.0f / durationFrames;
    m446CameraTransitionActive = 1;
}

s32 M446CameraTransitionIsActive(void) {
    return m446CameraTransitionActive;
}

void M446CameraTransitionReset(void) {
    m446CameraAngles.x += 0.01f * *HuPadSubStkX;
    m446CameraAngles.z += 0.01f * *HuPadSubStkY;
    m446CameraDistance += 0.05f * *HuPadTrigL;
    m446CameraDistance -= 0.05f * *HuPadTrigR;
    print8(0x20, 0x84, 1.5f, "LOOKAT : x=%.2f y=%.2f z=%.2f", m446CameraLookAt.x, m446CameraLookAt.y, m446CameraLookAt.z);
    print8(0x20, 0x94, 1.5f, "ANGLE  : x=%.2f y=%.2f z=%.2f", m446CameraAngles.x, m446CameraAngles.y, m446CameraAngles.z);
    print8(0x20, 0xA4, 1.5f, "ZOOM   : %.2f", m446CameraDistance);
}
