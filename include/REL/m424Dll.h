#ifndef REL_M424DLL_H
#define REL_M424DLL_H

#include "game/object.h"

typedef struct _M424CameraConfig {
    Vec rot;
    Vec center;
    float zoom;
} M424CameraConfig; // sizeof 1C

typedef struct _M424BallPhysicsState {
    OMOBJ *object;
    u8 state;
    u8 controllerState;
    u8 effectState;
    u16 modelId;
    u16 shadowModelId;
    Mtx modelMatrix;
    Vec position;
    Vec rotatedPosition;
    Vec previousPosition;
    Vec currentPosition;
    Vec velocity;
    Vec positionDelta;
    Vec targetVelocity;
    u8 collisionDetected;
    u8 playerIndex;
} M424BallPhysicsState; // sizeof 0x94

extern M424BallPhysicsState ballPhysicsStates[9];
extern u8 ballCount;
extern s16 playerOrder[4];

s32 M424SceneStateGet(void);
float AngleDeltaDegrees(float, float);
float AngleApproachDegrees(float, float, float);
OMOBJ *MapCameraCreate(HUPROCESS *);
void MapCameraDestroy(void);
float MapCameraHeightGet(void);
void MapCameraAnimationStart(float, s32);
void MapEffectStart(s32, s32);
void M424BallObjectsCreate(HUPROCESS *);
void M424BallObjectsDestroy(void);
Vec *BallVelocityPointerGet(u8);
s32 ActiveBallExists(void);
s32 BallPositionClamp(Vec *position, Vec *clampedPosition);
OMOBJ *ClawObjectCreate(HUPROCESS *);
void ClawObjectDestroy(void);
s32 ClawMotionActive(void);
s32 ClawPlayerThresholdReached(void);
s32 ClawMotionPhase10Reached(void);
s32 ClawMotionPhase13Reached(void);
void ClawBallCollisionsUpdate(s32 collisionFlags);

#endif
