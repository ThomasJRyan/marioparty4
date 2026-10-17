#include "REL/m447dll.h"

#include "ext_math.h"

float M447DirectionAngleGet(float* origin, float* target);
void M447PointRotate(float* origin, float rotationRadians, float* point, float* rotatedPoint);
s32 M447SweptCollisionCheck(float* movingCircle, float* targetCircle);

void M447SplinePositionGet(Vec* resultPosition, Vec* controlPoints, s32 pointCount, float splineProgress) {
    float angle;
    float movementMagnitude;
    float movementAngle;
    s32 cardIndex;
    s32 playerIndex;
    s32 i;

    cardIndex = pointCount - 1;
    resultPosition->x = 0.0f;
    resultPosition->y = 0.0f;
    resultPosition->z = 0.0f;
    splineProgress = (pointCount + 1) * splineProgress - 1.0f;
    for (i = -2; i <= cardIndex + 2; i++) {
        playerIndex = i;
        if (i < 0) {
            playerIndex = 0;
        }
        if (i > cardIndex) {
            playerIndex = cardIndex;
        }
        movementAngle = splineProgress - i;
        if (movementAngle < 0.0f) {
            movementAngle = -movementAngle;
        }
        if (movementAngle < 1.0f) {
            movementMagnitude = (3.0f * movementAngle * movementAngle * movementAngle - 6.0f * movementAngle * movementAngle + 4.0f) / 6.0f;
        } else if (movementAngle < 2.0f) {
            angle = movementAngle - 2.0f;
            movementMagnitude = -angle * angle * angle / 6.0f;
        } else {
            movementMagnitude = 0.0f;
        }
        resultPosition->x += movementMagnitude * controlPoints[playerIndex].x;
        resultPosition->y += movementMagnitude * controlPoints[playerIndex].y;
        resultPosition->z += movementMagnitude * controlPoints[playerIndex].z;
    }
}

float M447DirectionAngleGet(float* origin, float* target) {
    float movementMagnitude;
    float movementAngle;
    float angle;
    float rotationSpeed;
    s32 loopIndex;

    movementMagnitude = target[0] - origin[0];
    movementAngle = target[1] - origin[1];
    if (movementMagnitude >= 0.0f) {
        if (movementAngle >= 0.0f) {
            loopIndex = 0;
        } else {
            loopIndex = 3;
            angle = movementMagnitude;
            movementMagnitude = -movementAngle;
            movementAngle = angle;
        }
    } else {
        if (movementAngle >= 0.0f) {
            loopIndex = 1;
            angle = movementMagnitude;
            movementMagnitude = movementAngle;
            movementAngle = -angle;
        } else {
            loopIndex = 2;
            movementMagnitude = -movementMagnitude;
            movementAngle = -movementAngle;
        }
    }
    if (movementAngle > movementMagnitude) {
        rotationSpeed = M_PI / 2 - atan2d(movementMagnitude, movementAngle);
    } else {
        rotationSpeed = atan2d(movementAngle, movementMagnitude);
    }
    return rotationSpeed + (M_PI / 2) * loopIndex;
}

void M447PointRotate(float* origin, float rotationRadians, float* point, float* rotatedPoint) {
    float distance;
    float scale;
    float angle;
    float speed;

    // 57.29578 = 180 / M_PI (1 rad in degrees)
    distance = cosd(-(57.29578f * rotationRadians));
    scale = sind(-(57.29578f * rotationRadians));
    angle = point[0] - origin[0];
    speed = point[1] - origin[1];
    rotatedPoint[0] = angle * distance + speed * scale + origin[0];
    rotatedPoint[1] = -angle * scale + speed * distance + origin[1];
}

s32 M447CircleLineIntersectionGet(float* circle, float* line, float* intersections) {
    float blockRotations[2];
    float blockVelocities[2];
    float blockPositions[2];
    float blockPosition;
    float distance;

    blockRotations[0] = 0.0f;
    blockRotations[1] = 0.0f;
    blockVelocities[0] = circle[0] - line[0];
    blockVelocities[1] = circle[1] - line[1];
    blockPosition = M447DirectionAngleGet(line, &line[2]);
    M447PointRotate(blockRotations, -blockPosition, blockVelocities, blockPositions);
    if (ABS(blockPositions[1]) > circle[2]) {
        return 0;
    }
    distance = sqrtf(circle[2] * circle[2] - blockPositions[1] * blockPositions[1]);
    blockVelocities[0] = circle[0] + distance;
    blockVelocities[1] = circle[1] - blockPositions[1];
    M447PointRotate(circle, blockPosition, blockVelocities, intersections);
    if (distance == 0.0f) {
        return 1;
    }
    blockVelocities[0] = circle[0] - distance;
    blockVelocities[1] = circle[1] - blockPositions[1];
    M447PointRotate(circle, blockPosition, blockVelocities, &intersections[2]);
    return 2;
}

s32 M447CircleCollisionResolve(float* movingCircle, float* targetCircle, s32 moveFirst, s32 adjustTarget) {
    float targetPosition[2];
    float rotationSpeed;

    targetPosition[0] = movingCircle[0] - targetCircle[0];
    targetPosition[1] = movingCircle[1] - targetCircle[1];
    rotationSpeed = sqrtf(targetPosition[0] * targetPosition[0] + targetPosition[1] * targetPosition[1]);
    if (rotationSpeed < movingCircle[2] + targetCircle[2]) {
        if (rotationSpeed == 0.0f) {
            targetPosition[0] = 0.0f;
            targetPosition[1] = -1.0f;
        } else {
            targetPosition[0] /= rotationSpeed;
            targetPosition[1] /= rotationSpeed;
            if (adjustTarget != 0) {
                targetCircle[0] -= targetPosition[0];
                targetCircle[1] -= targetPosition[1];
            }
        }
        if (moveFirst != 0) {
            movingCircle[0] = targetCircle[0] + targetPosition[0] * (movingCircle[2] + targetCircle[2]);
            movingCircle[1] = targetCircle[1] + targetPosition[1] * (movingCircle[2] + targetCircle[2]);
        }
        return 1;
    }
    return 0;
}

s32 M447SweptCollisionCheck(float* movingCircle, float* targetCircle) {
    float scale;
    float angle;
    float speed;
    float ratio;
    float frameValue;
    float distanceDelta;

    ratio = movingCircle[2] * movingCircle[2] - 2.0f * movingCircle[0] * movingCircle[2] + movingCircle[0] * movingCircle[0] + movingCircle[3] * movingCircle[3] - 2.0f * movingCircle[1] * movingCircle[3] + movingCircle[1] * movingCircle[1];
    frameValue = 2.0f * (movingCircle[0] * movingCircle[2] - movingCircle[2] * targetCircle[0] + movingCircle[0] * movingCircle[0] + movingCircle[1] * movingCircle[3] - movingCircle[3] * targetCircle[1] + movingCircle[1] * movingCircle[1]);
    distanceDelta = movingCircle[0] * movingCircle[0] - 2.0f * movingCircle[0] * targetCircle[0] + targetCircle[0] * targetCircle[0] + movingCircle[1] * movingCircle[1] - 2.0f * movingCircle[1] * targetCircle[1] + targetCircle[1] * targetCircle[1] - targetCircle[2] * targetCircle[2];
    if (ratio == 0.0f) {
        if (frameValue != 0.0f) {
            return 1;
        } else {
            return 0;
        }
    }
    scale = frameValue * frameValue - 4.0f * ratio * distanceDelta;
    if (scale < 0.0f) {
        return 0;
    }
    scale = sqrtf(scale);
    angle = (-frameValue + scale) / (2.0f * ratio);
    speed = (-frameValue - scale) / (2.0f * ratio);
    if (angle < speed) {
        if (angle >= 0.0f && angle > 1.0f) {
            return 1;
        }
        if (speed >= 0.0f && speed < 1.0f) {
            return 1;
        }
    } else {
        if (speed >= 0.0f && speed < 1.0f) {
            return 1;
        }
        if (angle >= 0.0f && angle < 1.0f) {
            return 1;
        }
    }
    return 0;
}
