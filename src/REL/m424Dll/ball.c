#include "REL/m424Dll.h"

#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "string.h"
#include "version.h"

// STRUCT
typedef struct M424CharacterBallState {
    u8 playerIndex;
    u8 characterId;
    u8 ballType;
    u8 state;
    u8 controllerPort;
    u8 movementState;
    u8 effectState;
    u8 collisionFlags;
    u8 actionState;
    u8 actionTimer;
    u8 isActive;
    char reservedPadding0B[0x1];
    u8 isGrounded;
    u8 bounceState;
    u8 ballFlags;
    s16 modelId;
    s16 shadowModelId;
    s16 pressedButtons;
    s16 animationId;
    s16 actionId;
    s16 effectId;
    f32 scale;
    f32 movementSpeed;
    f32 targetHeading;
    f32 motionFrame;
    f32 motionDuration;
    f32 targetScale;
    Vec worldPosition;
    Vec rotation;
    Vec motionState;
    Vec frameTimer;
    void *animationTimer; // sizeof 0x40
    char reservedPadding68[0x3C];
    u8 ballModel;
    void *animationStateData; // sizeof 0x40
    char reservedPaddingAC[0x3C];
    s32 transitionTimer;
    s32 transitionState;
} M424CharacterBallState; // sizeof 0xF0

typedef struct M424BallObjectState {
    u8 collisionType;
    M424BallPhysicsState *firstBall;
    M424BallPhysicsState *secondBall;
} M424BallObjectState;

typedef struct M424BallCollisionState {
    f32 positionX;
    f32 positionY;
    f32 positionZ;
    char ballOwner[0x8];
    f32 targetPosition;
    f32 targetRotation;
    char reservedPadding1C[0x8];
    f32 targetScale;
    f32 motionFrame;
} M424BallCollisionState;

// BSS
void *ballAllocationSlots[0x20];
OMOBJ *playerBallObjects[4];
OMOBJ *ambientBallObjects[6];
M424BallPhysicsState ballPhysicsStates[9];
s32 unusedBallCounter;
u8 ballCount;

// PROTO
f32 MapCameraAnimationOffsetGet(void);
void PlayerBallInit(OMOBJ *object);
void PlayerBallUpdate(OMOBJ *object);
void PlayerBallActionUpdate(OMOBJ *object);
void PlayerBallMotionUpdate(OMOBJ *object);
void CpuBallInit(OMOBJ *object);
void CpuBallUpdate(OMOBJ *object);
u8 BallModelCreate(OMOBJ *object, u8, u8);
void BallPhysicsUpdate(s16);
void BallObjectsSetup(void);
void *BallStateAlloc(s32);
void BallObjectsDestroy(void);
s32 RandomU16Get(void);
void ModelMatrixApply(s16, Mtx);
void RandomIndicesShuffle(s32, u32 *);
f32 AngleApproachDegrees(f32, f32, f32);
f32 SphereCollisionTime3d(Vec, Vec, f32, Vec, Vec, f32);
f32 SphereCollisionTimeXz(Vec, Vec, f32, f32);
f32 SphereBoundaryTimeXz(Vec, Vec, f32, f32);
void VelocitySplitByCollision(Vec, Vec, Vec, f32, Vec *, Vec *);
void SpheresResolveCollision(Vec *, Vec *, f32, Vec *, Vec *, f32, f32);
void VectorReflect(Vec, Vec, f32, Vec *);
void BallStateSet(u8, u8);
void BallOrderAdjust(s32, s32);
s32 BallCollisionResolve(s32, s32);
s32 BallPositionClamp(Vec *, Vec *);

void M424BallObjectsCreate(HUPROCESS *processManager)
{
    u32 ballOrder[10];
    OMOBJ *ballObject;
    s32 randomOffsetCount;
    s32 ballOrderIndex;
    s32 playerIndex;

    randomOffsetCount = frand() & 0x1F;
    for (playerIndex = 0; playerIndex < randomOffsetCount; playerIndex++) {
        RandomU16Get();
    }
    RandomIndicesShuffle(8, ballOrder);
    ballOrder[8] = 8;

    for (playerIndex = 0, ballOrderIndex = 0; playerIndex < 4; playerIndex++) {
        if (GWPlayerCfg[playerIndex].group != 0) {
            ballObject = omAddObjEx(processManager, 0x3E, 3, 11, 0, PlayerBallInit);
            playerBallObjects[ballOrderIndex] = ballObject;
            ballObject->work[0] = playerIndex;
            ballObject->work[1] = ballOrder[ballOrderIndex];
            ballOrderIndex++;
        }
    }

    for (playerIndex = 0; playerIndex < 6; ballOrderIndex++, playerIndex++) {
        ballObject = omAddObjEx(processManager, 0x3F, 2, 5, 0, CpuBallInit);
        ambientBallObjects[playerIndex] = ballObject;
        ballObject->work[0] = playerIndex;
        ballObject->work[1] = ballOrder[ballOrderIndex];
    }
    Hu3DLayerHookSet(7, BallPhysicsUpdate);
    BallObjectsSetup();
}

void M424BallObjectsDestroy(void)
{
    BallObjectsDestroy();
}

u32 ballMotionIds[11] = { 0, 2, 3, 8, 10, 9, 20, 22, 23, 24, 0x370021 };
Vec ballSpawnPositions[4] = { { -100.0f, 200.0f, -100.0f }, { 300.0f, 200.0f, -100.0f }, { -300.0f, 200.0f, 300.0f }, { 300.0f, 200.0f, 0.0f } };

void PlayerBallInit(OMOBJ *object)
{
    Vec spawnPosition;
    f32 spawnAngle;
    f32 spawnRadius;
    M424CharacterBallState *ballState;
    s32 motionIndex;
    s32 spawnIndex;
    s32 characterModelId;

    object->data = ballState = BallStateAlloc(0xF0);
    memset(ballState, 0, 0xF0);
    ballState->playerIndex = object->work[0];
    ballState->characterId = GWPlayerCfg[ballState->playerIndex].character;
    ballState->ballType = GWPlayerCfg[ballState->playerIndex].group;
    ballState->state = 0;
    ballState->controllerPort = GWPlayerCfg[ballState->playerIndex].pad_idx;
    ballState->movementState = GWPlayerCfg[ballState->playerIndex].iscom;
    ballState->effectState = GWPlayerCfg[ballState->playerIndex].diff;
    ballState->actionId = ballState->effectId = 0;
    ballState->targetScale = 1.0f;
    ballState->collisionFlags = 1;
    ballState->isGrounded = 1;
    ballState->bounceState = 0;
    ballState->isActive = 0;
    ballState->ballModel = 0;
    ballState->ballFlags = 0;
    memset(&ballState->animationTimer, 0, 0x40);
    ballState->transitionTimer = 0;
    ballState->transitionState = -1;
    spawnIndex = object->work[1];
    spawnAngle = 40.0f;
    spawnAngle = (spawnAngle * spawnIndex) + (spawnAngle * (RandomU16Get() / 65536.0f)) + 105.0f;
    spawnRadius = 125.0f;
    spawnRadius = (spawnRadius * (spawnIndex & 1)) + (100.0f + (250.0f + (spawnRadius * (RandomU16Get() / 65536.0f))));
    spawnPosition.x = spawnRadius * sind(spawnAngle);
    spawnPosition.y = 200.0f;
    spawnPosition.z = spawnRadius * cosd(spawnAngle);
    ballState->worldPosition = spawnPosition;
    ballState->motionState.x = ballState->motionState.y = ballState->motionState.z = 0.0f;
    ballState->rotation.x = ballState->rotation.y = ballState->rotation.z = 0.0f;
    ballState->scale = 0.8f;
    ballState->targetHeading = ballState->rotation.y;
    ballState->motionDuration = 1.0f;
    ballState->motionFrame = 0.0f;
    ballState->movementSpeed = 0.0f;
    memset(&ballState->animationStateData, 0, 0x40);
    object->mdlId[0] = characterModelId = CharModelCreate(ballState->characterId, 8);
    Hu3DModelAttrSet(characterModelId, 0x40000001);
    CharModelStepFxSet(ballState->characterId, 1);
    CharEffectLayerSet(2);
    for (motionIndex = 0; motionIndex < 11; motionIndex++) {
        if (ballMotionIds[motionIndex] + 0xFFC90000 != 0x21) {
            object->mtnId[motionIndex] = CharMotionCreate(ballState->characterId, ballMotionIds[motionIndex]);
        }
        else if (ballState->characterId == 5) {
            object->mtnId[motionIndex]
                = Hu3DJointMotion(object->mdlId[0], HuDataSelHeapReadNum(ballState->characterId + ballMotionIds[motionIndex], 0x10000000, HEAP_MODEL));
        }
        else {
            object->mtnId[motionIndex] = object->mtnId[0];
        }
        if (ballMotionIds[motionIndex] == 0x16 || ballMotionIds[motionIndex] == 0x14) {
            CharMotionVoiceOnSet(ballState->characterId, object->mtnId[motionIndex], 0);
        }
    }
    CharMotionDataClose(ballState->characterId);
    CharMotionSet(ballState->characterId, object->mtnId[ballState->actionId]);
    omSetTra(object, ballState->worldPosition.x, ballState->worldPosition.y, ballState->worldPosition.z);
    omSetRot(object, ballState->rotation.x, ballState->rotation.y, ballState->rotation.z);
    omSetSca(object, ballState->scale, ballState->scale, ballState->scale);
    object->mdlId[1] = BallModelCreate(object, ballState->characterId, 1);
    object->objFunc = PlayerBallUpdate;
}

void EmptyBallCallback(void) { }

void PlayerBallUpdate(OMOBJ *object)
{
    s32 characterModelId;
    M424CharacterBallState *ballState;

    characterModelId = object->mdlId[0];
    ballState = object->data;
    if (ballState->collisionFlags != 0) {
        if ((M424SceneStateGet() == 4) && ((ballState->collisionFlags & 2) == 0)) {
            if (ballState->movementState != 0) {
                PlayerBallActionUpdate(object);
            }
            else {
                ballState->modelId = HuPadStkX[ballState->controllerPort];
                ballState->shadowModelId = HuPadStkY[ballState->controllerPort];
                ballState->pressedButtons = HuPadBtnDown[ballState->controllerPort];
                ballState->animationId = HuPadBtn[ballState->controllerPort];
            }
        }
        else {
            ballState->modelId = ballState->shadowModelId = 0;
            ballState->pressedButtons = ballState->animationId = 0;
        }
        switch (ballState->actionState) {
            case 1:
                ballState->isGrounded = 1;
                if (M424SceneStateGet() == 6) {
                    BallStateSet(object->mdlId[1], 5);
                }
                break;
            case 5:
                ballState->isGrounded = 0;
                ballState->scale *= 1.1f;
                if (ballState->scale >= 1.0f) {
                    ballState->scale = 1.0f;
                }
                ballState->rotation.y = AngleApproachDegrees(ballState->rotation.y, 0.0f, 0.9f);
                if (ballState->worldPosition.y <= 200.0f) {
                    ballState->worldPosition.y = 200.0f;
                    ballState->isGrounded = 1;
                }
                Hu3DModelShadowSet(characterModelId);
                break;
            case 4:
                ballState->movementSpeed = 0.0f;
                ballState->motionState.y = 0.0f;
                ballState->collisionFlags |= 2;
                break;
            case 6:
                ballState->modelId = ballState->shadowModelId = 0;
                ballState->pressedButtons = ballState->animationId = 0;
                ballState->scale = 1.0f;
                ballState->rotation.y = ballState->targetScale = 0.0f;
                if (ballState->worldPosition.y <= 50.0f) {
                    ballState->isGrounded = 1;
                    ballState->movementSpeed = 0.0f;
                    ballState->motionState.y = 0.0f;
                    ballState->actionId = 7;
                    ballState->worldPosition.x += MapCameraAnimationOffsetGet();
                    ballState->worldPosition.y = 50.0f;
                }
                else {
                    ballState->isGrounded = 0;
                    ballState->movementSpeed += VERSION_PAL ? 3.6000001f : 3.0f; // 180.0f * REFRESH_FREQ, doesn't match PAL
                    ballState->motionState.y += VERSION_PAL ? -1.7520001f : -1.46f; // −87.6f * REFRESH_FREQ, doesn't match PAL
                    if (ballState->characterId != 5) {
                        ballState->actionId = 6;
                    }
                    else {
                        ballState->actionId = 0xA;
                    }
                }
                break;
        }
        if ((M424SceneStateGet() == 7) && ((ballState->collisionFlags & 2) == 0)) {
            ballState->actionId = 8;
        }
        PlayerBallMotionUpdate(object);
    }
}

void PlayerBallActionUpdate(OMOBJ *object)
{
    Vec directionVector;
    s32 characterModelId;
    s16 currentMotionId;
    f32 movementAngle;
    f32 cameraHeight;
    M424CharacterBallState *ballState;
    s16 playerDifficulty;
    s16 horizontalImpulseX;
    s16 horizontalImpulseZ;
    s16 buttonMask;
    s16 animationIndex;

    ballState = object->data;
    characterModelId = object->mdlId[0];
    currentMotionId = ballState->actionId;
    playerDifficulty = ballState->effectState;

    horizontalImpulseX = horizontalImpulseZ = buttonMask = animationIndex = 0;
    switch (ballState->actionState) {
        case 2:
            break;
        case 1:
            if ((RandomU16Get() & 0x1F) == 0) {
                movementAngle = atan2d(ballState->worldPosition.x, ballState->worldPosition.z);
                movementAngle += ((30.0f - (5.0f * playerDifficulty)) * ((RandomU16Get() - 0x8000) / 32768.0f));
                directionVector.x = sind(movementAngle);
                directionVector.z = cosd(movementAngle);
                cameraHeight = MapCameraHeightGet();
                if ((movementAngle > 70.0f) && (movementAngle < 100.0f)) {
                    if (cameraHeight < 0.0f) {
                        horizontalImpulseX = (-directionVector.z * (48.0f + (6.0f * playerDifficulty)));
                        horizontalImpulseZ = (-directionVector.x * (48.0f + (6.0f * playerDifficulty)));
                    }
                    else {
                        horizontalImpulseX = (directionVector.z * (48.0f + (6.0f * playerDifficulty)));
                        horizontalImpulseZ = (directionVector.x * (48.0f + (6.0f * playerDifficulty)));
                    }
                }
                else if (cameraHeight < 0.0f) {
                    horizontalImpulseX = (directionVector.z * (48.0f + (6.0f * playerDifficulty)));
                    horizontalImpulseZ = (directionVector.x * (48.0f + (6.0f * playerDifficulty)));
                }
                else {
                    horizontalImpulseX = (-directionVector.z * (48.0f + (6.0f * playerDifficulty)));
                    horizontalImpulseZ = (-directionVector.x * (48.0f + (6.0f * playerDifficulty)));
                }
            }
            else {
                horizontalImpulseX = ballState->modelId;
                horizontalImpulseZ = ballState->shadowModelId;
            }
            break;
        case 3:
            if ((RandomU16Get() & 0x1F) == 0) {
                movementAngle = 180.0f * ((RandomU16Get() - 0x8000) / 32768.0f);
                directionVector.x = sind(movementAngle);
                directionVector.z = cosd(movementAngle);
                horizontalImpulseX = (48.0f * directionVector.x);
                horizontalImpulseZ = (48.0f * directionVector.z);
            }
            else {
                horizontalImpulseX = ballState->modelId;
                horizontalImpulseZ = ballState->shadowModelId;
            }
            break;
    }
    ballState->modelId = horizontalImpulseX;
    ballState->shadowModelId = horizontalImpulseZ;
    ballState->pressedButtons = buttonMask;
    ballState->animationId = animationIndex;
}

void PlayerBallMotionUpdate(OMOBJ *object)
{
    s16 savedButtonDown;
    s16 savedButtons;
    f32 targetScale;
    f32 movementHeading;
    f32 motionMaxTime;
    M424CharacterBallState *ballState;
    s16 velocityX;
    s16 velocityZ;
    s16 nextMotionId;
    s16 motionTransitionFlags;
    s32 characterModelId;

    targetScale = 1.0f;
    ballState = object->data;
    characterModelId = object->mdlId[0];
    nextMotionId = ballState->actionId;
    ballState->frameTimer = ballState->worldPosition;
    velocityX = ballState->modelId;
    velocityZ = ballState->shadowModelId;
    savedButtonDown = ballState->pressedButtons;
    savedButtons = ballState->animationId;
    switch (ballState->actionId) {
        case 0:
        case 1:
        case 2:
            ballState->movementSpeed = (0.025f * sqrtf((velocityX * velocityX) + (velocityZ * velocityZ)));
            if (ballState->movementSpeed > 0.01f) {
                if (ballState->movementSpeed >= 6.0f) {
                    nextMotionId = 2;
                    motionTransitionFlags = 1;
                }
                else {
                    nextMotionId = 1;
                    motionTransitionFlags = 1;
                }
                movementHeading = atan2d(velocityX, -velocityZ);
                ballState->rotation.y = AngleApproachDegrees(ballState->rotation.y, movementHeading, 0.5f);
                ballState->targetHeading = ballState->rotation.y;
                ballState->motionFrame++;
                if (ballState->motionFrame >= (0.5f * ballState->motionDuration)) {
                    ballState->motionFrame -= 0.5f * ballState->motionDuration;
                }
                ballState->movementSpeed *= 1.0 + 0.25 * cosd(90.0f * (ballState->motionFrame / (0.5f * ballState->motionDuration)));
            }
            else {
                ballState->motionFrame = 0.0f;
                ballState->movementSpeed = 0.0f;
                nextMotionId = 0;
                motionTransitionFlags = 1;
            }
            if (ballState->isGrounded != 0) {
                ballState->motionState.y = -146.0f / REFRESH_RATE_F;
            }
            else {
                ballState->motionState.y += -146.0f / REFRESH_RATE_F;
            }
            break;
        case 3:
            motionTransitionFlags = 0;
            if ((Hu3DData[characterModelId].motIdShift == -1) && (CharMotionEndCheck(ballState->characterId) != 0)) {
                ballState->motionState.y += -146.0f / REFRESH_RATE_F;
            }
            else {
                ballState->motionState.y = 0.0f;
            }
            if (ballState->isGrounded != 0) {
                if (ballState->bounceState != 0) {
                    ballState->motionState.y = 0.0f;
                    ballState->bounceState = 0;
                    nextMotionId = 5;
                    motionTransitionFlags = 0;
                }
                else {
                    ballState->motionState.y *= -0.1f;
                    ballState->isGrounded = 0;
                    nextMotionId = 4;
                    motionTransitionFlags = 0;
                }
            }
            break;
        case 4:
            ballState->motionState.y += -146.0f / REFRESH_RATE_F;
            if ((ballState->isGrounded != 0) && (CharMotionEndCheck(ballState->characterId) != 0)) {
                nextMotionId = 5;
                motionTransitionFlags = 0;
            }
            break;
        case 5:
            if (CharMotionEndCheck(ballState->characterId) != 0) {
                nextMotionId = 0;
                motionTransitionFlags = 1;
            }
            break;
        case 6:
            nextMotionId = 6;
            motionTransitionFlags = 0;
            if (ballState->characterId == 7) {
                targetScale = 0.85f;
                ballState->targetScale = 0.9f;
            }
            break;
        case 7:
            nextMotionId = 7;
            motionTransitionFlags = 1;
            if (ballState->transitionTimer == 1) {
                ballState->transitionState = CharFXPlay(ballState->characterId, 0x11A);
            }
            else if ((ballState->transitionTimer > REFRESH_RATE_F * 1.5f) && (ballState->transitionState != -1)) {
                HuAudFXStop(ballState->transitionState);
                ballState->transitionState = -1;
            }
            ballState->transitionTimer++;
            break;
        case 8:
        case 9:
            motionTransitionFlags = 0;
            ballState->motionState.y = -146.0f / REFRESH_RATE_F;
            break;
        case 10:
            nextMotionId = 10;
            motionTransitionFlags = 4;
            targetScale = 0.8f;
            ballState->targetScale = 0.85f;
            if ((nextMotionId == ballState->actionId) && (Hu3DMotionShiftIDGet(characterModelId) != -1)) {
                motionMaxTime = Hu3DMotionMaxTimeGet(characterModelId);
                if (Hu3DMotionTimeGet(characterModelId) >= motionMaxTime) {
                    nextMotionId = 7;
                    motionTransitionFlags = 1;
                }
            }
            break;
    }
    if ((ballState->collisionFlags & 4) != 0) {
        if ((ballState->isActive & 2) != 0) {
            Hu3DModelAttrSet(characterModelId, 1);
        }
        else {
            Hu3DModelAttrReset(characterModelId, 1);
        }
        if (--ballState->isActive == 0) {
            ballState->collisionFlags &= ~4;
        }
    }
    ballState->motionState.x = ballState->movementSpeed * sind(ballState->targetHeading);
    ballState->motionState.z = ballState->movementSpeed * cosd(ballState->targetHeading);
    ballState->worldPosition.x += ballState->motionState.x;
    if (ballState->isGrounded == 0) {
        ballState->worldPosition.y += ballState->motionState.y;
    }
    ballState->worldPosition.z += ballState->motionState.z;
    if (nextMotionId != ballState->effectId) {
        ballState->effectId = nextMotionId;
        ballState->actionId = nextMotionId;
        CharMotionShiftSet(ballState->characterId, object->mtnId[ballState->actionId], 0.0f, 8.0f, motionTransitionFlags);
        ballState->motionDuration = CharMotionMaxTimeGet(ballState->characterId);
    }
    if (ballState->targetScale != targetScale) {
        if (targetScale > ballState->targetScale) {
            if ((ballState->targetScale += 0.05f) > targetScale) {
                ballState->targetScale = targetScale;
            }
        }
        if (targetScale < ballState->targetScale) {
            if ((ballState->targetScale -= 0.05f) < targetScale) {
                ballState->targetScale = targetScale;
            }
        }
        ballState->scale = ballState->targetScale;
    }
    omSetTra(object, ballState->worldPosition.x, ballState->worldPosition.y, ballState->worldPosition.z);
    omSetRot(object, ballState->rotation.x, ballState->rotation.y, ballState->rotation.z);
    omSetSca(object, ballState->scale, ballState->scale, ballState->scale);
}

void CpuBallInit(OMOBJ *object)
{
    s32 ambientMotionIds[3] = { 0x370013, 0x370014, 0x370015 };
    Vec spawnPosition;
    f32 spawnAngle;
    f32 spawnRadius;
    M424CharacterBallState *ballState;
    s32 motionIndex;
    s32 spawnIndex;
    s32 characterModelId;

    object->mdlId[0] = characterModelId = Hu3DModelCreateFile(0x370012);

    for (motionIndex = 0; motionIndex < 3; motionIndex++) {
        object->mtnId[motionIndex] = Hu3DJointMotion(characterModelId, HuDataSelHeapReadNum(ambientMotionIds[motionIndex], 0x10000000, HEAP_MODEL));
    }
    Hu3DMotionSet(characterModelId, object->mtnId[0]);
    Hu3DModelAttrSet(characterModelId, 0x40000001);
    ballState = BallStateAlloc(0xF0);
    object->data = ballState;
    memset(ballState, 0, 0xF0);
    ballState->playerIndex = object->work[0];
    ballState->characterId = 0xFF;
    ballState->ballType = 0xFF;
    ballState->state = 0;
    ballState->controllerPort = 0xFF;
    ballState->movementState = 0xFF;
    ballState->effectState = 0xFF;
    ballState->actionId = ballState->effectId = 0;
    ballState->collisionFlags = 1;
    ballState->isGrounded = 1;
    ballState->bounceState = 0;
    ballState->isActive = 0;
    ballState->ballModel = 0;
    ballState->ballFlags = 0;
    memset(&ballState->animationTimer, 0, 0x40);
    spawnIndex = object->work[1];
    if (spawnIndex == 8) {
        spawnPosition.x = 350.0f;
        spawnPosition.y = 200.0f;
        spawnPosition.z = 50.0f;
    }
    else {
        spawnAngle = 40.0f;
        spawnAngle = 105.0f + ((spawnAngle * spawnIndex) + (spawnAngle * (RandomU16Get() / 65536.0f)));
        spawnRadius = 125.0f;
        spawnRadius = (spawnRadius * (spawnIndex & 1)) + (100.0f + (250.0f + (spawnRadius * (RandomU16Get() / 65536.0f))));
        spawnPosition.x = spawnRadius * sind(spawnAngle);
        spawnPosition.y = 200.0f;
        spawnPosition.z = spawnRadius * cosd(spawnAngle);
    }
    ballState->worldPosition = spawnPosition;
    ballState->motionState.x = ballState->motionState.y = ballState->motionState.z = 0.0f;
    ballState->rotation.x = ballState->rotation.y = ballState->rotation.z = 0.0f;
    ballState->targetHeading = ballState->rotation.y;
    ballState->motionDuration = 1.0f;
    ballState->motionFrame = 0.0f;
    ballState->movementSpeed = 0.0f;
    memset(&ballState->animationStateData, 0, 0x40);
    omSetTra(object, ballState->worldPosition.x, ballState->worldPosition.y, ballState->worldPosition.z);
    omSetRot(object, ballState->rotation.x, ballState->rotation.y, ballState->rotation.z);
    omSetSca(object, 0.8f, 0.8f, 0.8f);
    object->mdlId[1] = BallModelCreate(object, 8, 0);
    object->objFunc = CpuBallUpdate;
}

EmptyBallEffectCallback(void) { }

void CpuBallUpdate(OMOBJ *object)
{
    s32 characterModelId;
    s16 motionTransitionFlags;
    s16 nextMotionId;
    M424CharacterBallState *ballState;

    ballState = object->data;
    characterModelId = object->mdlId[0];
    nextMotionId = ballState->actionId;
    ballState->frameTimer = ballState->worldPosition;

    ballState->motionState.x = ballState->motionState.z = 0.0f;
    switch (ballState->actionState) { /* irregular */
        case 3:
            nextMotionId = 2;
            motionTransitionFlags = 1;
            ballState->motionState.y = -146.0f / REFRESH_RATE_F;
            if ((RandomU16Get() & 0xF) == 0) {
                ballState->targetHeading = (180.0f * ((RandomU16Get() - 0x8000) / 32768.0f));
            }
            break;
        case 1:
            nextMotionId = 1;
            motionTransitionFlags = 1;
            ballState->motionState.y = -146.0f / REFRESH_RATE_F;
            if ((RandomU16Get() & 0x3F) == 0) {
                ballState->targetHeading = (180.0f * ((RandomU16Get() - 0x8000) / 32768.0f));
            }
            break;
        case 2:
            nextMotionId = 0;
            motionTransitionFlags = 1;

            ballState->motionState.x = ballState->motionState.z = 0.0f;
            ballState->motionState.y += VERSION_PAL ? -1.1680001f : -0.97333336f; // −58.4f * REFRESH_FREQ, doesn't match PAL
            break;
    }
    ballState->rotation.y = AngleApproachDegrees(ballState->rotation.y, ballState->targetHeading, 0.95f);
    if (nextMotionId != ballState->effectId) {
        ballState->effectId = nextMotionId;
        ballState->actionId = nextMotionId;
        Hu3DMotionShiftSet(characterModelId, object->mtnId[ballState->actionId], 0.0f, 8.0f, (u32)motionTransitionFlags);
        ballState->motionDuration = Hu3DMotionMaxTimeGet(ballState->characterId);
    }
}

u8 BallModelCreate(OMOBJ *object, u8 characterId, u8 isPlayerControlled)
{
    M424BallPhysicsState *ballPhysicsState;
    s32 modelId;
    s32 ballIndex;

    ballIndex = ballCount;
    ballCount++;
    ballPhysicsState = &ballPhysicsStates[ballIndex];
    memset(ballPhysicsState, 0, 0x94);
    ballPhysicsState->object = object;
    modelId = Hu3DModelCreate(HuDataSelHeapReadNum((characterId * 2) + 0x370000, 0x10000000, HEAP_MODEL));
    ballPhysicsState->modelId = modelId;
    Hu3DModelLayerSet(modelId, 2);
    Hu3DModelShadowMapSet(modelId);
    Hu3DModelAttrSet(modelId, 0x20000);
    Hu3DModelHiliteMapSet(modelId, HuSprAnimRead(HuDataReadNum(0x370016, 0x10000000)));
    modelId = Hu3DModelCreate(HuDataSelHeapReadNum((characterId * 2) + 0x370001, 0x10000000, HEAP_MODEL));
    ballPhysicsState->shadowModelId = modelId;
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelShadowSet(modelId);
    MTXIdentity(ballPhysicsState->modelMatrix);
    ballPhysicsState->position.x = object->trans.x;
    ballPhysicsState->position.y = 100.0f + object->trans.y;
    ballPhysicsState->position.z = object->trans.z;
    ballPhysicsState->previousPosition = ballPhysicsState->position;
    ballPhysicsState->velocity.x = ballPhysicsState->velocity.y = ballPhysicsState->velocity.z = 0.0f;
    ballPhysicsState->controllerState = isPlayerControlled;
    ballPhysicsState->state = 2;
    ballPhysicsState->effectState = 0;
    if (isPlayerControlled != 0) {
        ballPhysicsState->playerIndex = characterId;
    }
    else {
        ballPhysicsState->playerIndex = 0xFFU;
    }
    return ballIndex;
}

void BallPhysicsUpdate(s16 updateFlags)
{
    Mtx modelTransformMatrix;
    Mtx cameraRotationMatrix;
    Vec positionDelta;
    Vec rotationAxis;
    f32 cameraYaw;
    f32 rotationMagnitude;
    M424BallPhysicsState *physicsState;
    M424CharacterBallState *characterState;
    s32 ballIndex;

    if (omPauseChk() == 0) {
        physicsState = ballPhysicsStates;
        cameraYaw = MapCameraHeightGet();
        mtxRot(cameraRotationMatrix, 0.0f, cameraYaw, 0.0f);

        for (ballIndex = 0; ballIndex < ballCount; ballIndex++, physicsState++) {
            characterState = physicsState->object->data;
            physicsState->rotatedPosition = physicsState->previousPosition;
            physicsState->currentPosition = physicsState->rotatedPosition;
            physicsState->positionDelta.x = physicsState->positionDelta.y = physicsState->positionDelta.z = 0.0f;
            switch (physicsState->state) {
                case 1:
                    MTXTrans(modelTransformMatrix, physicsState->position.x, physicsState->position.y, physicsState->position.z);
                    MTXConcat(cameraRotationMatrix, modelTransformMatrix, modelTransformMatrix);
                    physicsState->rotatedPosition.x = modelTransformMatrix[0][3];
                    physicsState->rotatedPosition.y = modelTransformMatrix[1][3];
                    physicsState->rotatedPosition.z = modelTransformMatrix[2][3];
                    VECSubtract(&physicsState->rotatedPosition, &physicsState->position, &physicsState->positionDelta);
                    VECSubtract(&physicsState->position, &physicsState->previousPosition, &positionDelta);
                    VECAdd(&characterState->motionState, &positionDelta, &physicsState->targetVelocity);
                    VECAdd(&physicsState->targetVelocity, &physicsState->positionDelta, &physicsState->velocity);
                    physicsState->targetVelocity.y = 0.0f;
                    physicsState->state = 2;
                    break;
                case 3:
                    VECSubtract(&physicsState->position, &physicsState->currentPosition, &physicsState->velocity);
                    physicsState->targetVelocity = physicsState->velocity;
                    break;
                case 4:
                    physicsState->velocity.y += -146.0f / REFRESH_RATE_F;
                    physicsState->velocity.x = physicsState->velocity.z = 0.0f;
                    physicsState->targetVelocity = physicsState->velocity;
                    break;
                case 2:
                    physicsState->velocity.y = 0.0f;
                    physicsState->velocity.x = physicsState->velocity.z = 0.0f;
                    VECSubtract(&physicsState->position, &physicsState->previousPosition, &positionDelta);
                    VECAdd(&characterState->motionState, &positionDelta, &physicsState->targetVelocity);
                    physicsState->velocity = physicsState->targetVelocity;
                    break;
                case 5:
                    if ((physicsState->effectState & 2) != 0) {
                        Hu3DModelAttrSet(physicsState->modelId, 1);
                        Hu3DModelAttrSet(physicsState->shadowModelId, 1);
                    }
                    else {
                        Hu3DModelAttrReset(physicsState->modelId, 1);
                        Hu3DModelAttrReset(physicsState->shadowModelId, 1);
                    }
                    if (++physicsState->effectState >= 0x3C) {
                        physicsState->state = 6;
                    }
                    break;
                default:
                    physicsState->velocity.x = physicsState->velocity.y = physicsState->velocity.z = 0.0f;
                    break;
            }
        }
        BallOrderAdjust(0x20, 1);
        physicsState = ballPhysicsStates;
        for (ballIndex = 0; ballIndex < ballCount; ballIndex++, physicsState++) {
            if (physicsState->state != 6) {
                if (physicsState->state == 1) {
                    VECSubtract(&physicsState->position, &physicsState->rotatedPosition, &positionDelta);
                    rotationAxis.x = positionDelta.z;
                    rotationAxis.y = 0.0f;
                    rotationAxis.z = -positionDelta.x;
                    rotationMagnitude = VECMag(&rotationAxis);
                    if (rotationMagnitude > 0.01f) {
                        MTXRotAxisRad(modelTransformMatrix, &rotationAxis, MTXDegToRad(rotationMagnitude));
                        MTXConcat(cameraRotationMatrix, modelTransformMatrix, modelTransformMatrix);
                    }
                    else {
                        MTXCopy(cameraRotationMatrix, modelTransformMatrix);
                    }
                    MTXConcat(modelTransformMatrix, physicsState->modelMatrix, physicsState->modelMatrix);
                    ModelMatrixApply(physicsState->modelId, physicsState->modelMatrix);
                    ModelMatrixApply(physicsState->shadowModelId, physicsState->modelMatrix);
                }
                if (physicsState->position.y < 250.0f) {
                    physicsState->position.y = 250.0f;
                    physicsState->state = 6;
                    Hu3DModelAttrSet(physicsState->modelId, 1);
                    Hu3DModelAttrSet(physicsState->shadowModelId, 1);
                }
                Hu3DModelPosSet(physicsState->modelId, physicsState->position.x, physicsState->position.y, physicsState->position.z);
                Hu3DModelPosSet(physicsState->shadowModelId, physicsState->position.x, physicsState->position.y, physicsState->position.z);
                VECSubtract(&physicsState->position, &physicsState->previousPosition, &positionDelta);
                physicsState->velocity = positionDelta;
                physicsState->previousPosition = physicsState->position;
                characterState = physicsState->object->data;
                characterState->actionState = physicsState->state;
                characterState->rotation.y += cameraYaw;
                if (physicsState->state != 5) {
                    characterState->worldPosition = physicsState->position;
                    characterState->worldPosition.y -= 80.0f;
                    omSetTra(physicsState->object, characterState->worldPosition.x, characterState->worldPosition.y, characterState->worldPosition.z);
                    omSetRot(physicsState->object, characterState->rotation.x, characterState->rotation.y, characterState->rotation.z);
                }
            }
        }
    }
}

void BallCollisionUpdate(void)
{
    M424BallObjectState collisionEvents[32];
    Vec secondBallVelocityResult;
    Vec secondBallImpulseResult;
    Vec firstBallImpulseResult;
    Vec firstBallVelocityResult;
    Vec horizontalPosition;
    Vec bounceVelocity;
    Vec collisionNormal;
    f32 collisionTime;
    f32 earliestCollisionTime;
    f32 remainingFrameTime;
    f32 boundaryPenetration;
    M424BallPhysicsState *ballState;
    M424BallPhysicsState *otherBallState;
    s32 collisionEventCount;
    s32 ballIndex;
    s32 duplicateEventIndex;
    s32 collisionIteration;
    s32 otherBallIndex;

    ballState = &ballPhysicsStates[0];
    for (ballIndex = 0; ballIndex < ballCount; ballIndex++, ballState++) {
        ballState->collisionDetected = 1;
    }

    for (collisionIteration = 0, remainingFrameTime = 1.0f; collisionIteration < 0x80; remainingFrameTime -= earliestCollisionTime, collisionIteration++) {
        collisionEventCount = 0;
        earliestCollisionTime = remainingFrameTime;
        ballState = &ballPhysicsStates[0];
        for (ballIndex = 0; ballIndex < ballCount; ballIndex++, ballState++) {
            if ((ballState->collisionDetected != 0) && (ballState->state < 4)) {
                ballState->collisionDetected = 0;
                if (ballState->state < 3) {
                    if (ballState->velocity.y < 0.0f) {
                        collisionTime = (200.0f - (ballState->currentPosition.y - 100.0f)) / ballState->velocity.y;
                        if (collisionTime <= remainingFrameTime) {
                            if (collisionTime < 0.0f) {
                                collisionTime = 0.0f;
                            }
                            ballState->collisionDetected = 1;
                            if (collisionTime <= earliestCollisionTime) {
                                if (collisionTime < earliestCollisionTime) {
                                    collisionEventCount = 0;
                                }
                                collisionEvents[collisionEventCount].collisionType = 0;
                                collisionEvents[collisionEventCount].firstBall = ballState;
                                collisionEvents[collisionEventCount].secondBall = 0;
                                collisionEventCount++;
                                earliestCollisionTime = collisionTime;
                            }
                        }
                    }
                    if (ballState->state == 1) {
                        collisionTime = SphereBoundaryTimeXz(ballState->currentPosition, ballState->targetVelocity, 100.0f, 700.0f);
                    }
                    else {
                        collisionTime = SphereBoundaryTimeXz(ballState->currentPosition, ballState->velocity, 100.0f, 700.0f);
                    }
                    if (collisionTime >= 0.0f && collisionTime <= remainingFrameTime) {
                        ballState->collisionDetected = 1;
                        if (collisionTime <= earliestCollisionTime) {
                            if (collisionTime < earliestCollisionTime) {
                                collisionEventCount = 0;
                            }
                            collisionEvents[collisionEventCount].collisionType = 1;
                            collisionEvents[collisionEventCount].firstBall = ballState;
                            collisionEvents[collisionEventCount].secondBall = 0;
                            collisionEventCount++;
                            earliestCollisionTime = collisionTime;
                        }
                    }
                    if (ballState->state == 1) {
                        collisionTime = SphereCollisionTimeXz(ballState->currentPosition, ballState->targetVelocity, 100.0f, 250.0f);
                    }
                    else {
                        collisionTime = SphereCollisionTimeXz(ballState->currentPosition, ballState->velocity, 100.0f, 250.0f);
                    }
                    if (collisionTime >= 0.0f && collisionTime <= remainingFrameTime) {
                        ballState->collisionDetected = 1;
                        if (collisionTime <= earliestCollisionTime) {
                            if (collisionTime < earliestCollisionTime) {
                                collisionEventCount = 0;
                            }
                            collisionEvents[collisionEventCount].collisionType = 2;
                            collisionEvents[collisionEventCount].firstBall = ballState;
                            collisionEvents[collisionEventCount].secondBall = 0;
                            collisionEventCount++;
                            earliestCollisionTime = collisionTime;
                        }
                    }
                }

                otherBallState = ballPhysicsStates;
                for (otherBallIndex = 0; otherBallIndex < ballCount; otherBallIndex++, otherBallState++) {
                    if ((ballState != otherBallState) && (otherBallState->state < 4)) {
                        if ((ballState->state == 1) && (otherBallState->state == 1)) {
                            collisionTime = SphereCollisionTime3d(ballState->currentPosition, ballState->targetVelocity, 100.0f, otherBallState->currentPosition, otherBallState->targetVelocity, 100.0f);
                        }
                        else {
                            collisionTime = SphereCollisionTime3d(ballState->currentPosition, ballState->velocity, 100.0f, otherBallState->currentPosition, otherBallState->velocity, 100.0f);
                        }
                        if ((collisionTime < 0.0f) || (collisionTime > remainingFrameTime))
                            continue;
                        ballState->collisionDetected = 1;
                        otherBallState->collisionDetected = 1;
                        if (collisionTime > earliestCollisionTime)
                            continue;
                        if (collisionTime < earliestCollisionTime) {
                            collisionEventCount = 0;
                        }
                        else {
                            for (duplicateEventIndex = 0; duplicateEventIndex < collisionEventCount; duplicateEventIndex++) {
                                if ((collisionEvents[duplicateEventIndex].firstBall == otherBallState) && (collisionEvents[duplicateEventIndex].secondBall == ballState))
                                    break;
                            }
                            if (duplicateEventIndex < collisionEventCount)
                                continue;
                        }
                        collisionEvents[collisionEventCount].collisionType = 3;
                        collisionEvents[collisionEventCount].firstBall = ballState;
                        collisionEvents[collisionEventCount].secondBall = otherBallState;
                        collisionEventCount++;
                        earliestCollisionTime = collisionTime;
                    }
                }
            }
        }
        if (collisionEventCount == 0)
            break;
        ballState = ballPhysicsStates;
        for (ballIndex = 0; ballIndex < ballCount; ballIndex++, ballState++) {
            ballState->currentPosition.x += ballState->velocity.x * earliestCollisionTime;
            ballState->currentPosition.y += ballState->velocity.y * earliestCollisionTime;
            ballState->currentPosition.z += ballState->velocity.z * earliestCollisionTime;
        }
        for (ballIndex = 0; ballIndex < collisionEventCount; ballIndex++) {
            switch (collisionEvents[ballIndex].collisionType) {
                case 0:
                    ballState = collisionEvents[ballIndex].firstBall;
                    ballState->state = 1;
                    bounceVelocity.y = 300.0f - ballState->currentPosition.y;
                    ballState->currentPosition.y = 300.0f;
                    ballState->velocity.y = bounceVelocity.y;
                    ballState->targetVelocity.y = bounceVelocity.y;
                    break;
                case 1:
                    ballState = collisionEvents[ballIndex].firstBall;
                    horizontalPosition = ballState->currentPosition;
                    horizontalPosition.y = 0.0f;
                    VECNormalize(&horizontalPosition, &collisionNormal);
                    collisionNormal.x = -collisionNormal.x;
                    collisionNormal.y = 0.0f;
                    collisionNormal.z = -collisionNormal.z;
                    boundaryPenetration = VECMag(&horizontalPosition);
                    boundaryPenetration -= 600.0f;
                    ballState->currentPosition.x += boundaryPenetration * collisionNormal.x;
                    ballState->currentPosition.z += boundaryPenetration * collisionNormal.z;
                    horizontalPosition = ballState->currentPosition;
                    horizontalPosition.y = 0.0f;
                    if (ballState->state == 1) {
                        VectorReflect(ballState->targetVelocity, collisionNormal, 0.8f, &firstBallVelocityResult);
                        ballState->targetVelocity = firstBallVelocityResult;
                        VECAdd(&ballState->targetVelocity, &ballState->positionDelta, &ballState->velocity);
                    }
                    else {
                        VectorReflect(ballState->velocity, collisionNormal, 0.8f, &firstBallVelocityResult);
                        ballState->velocity = firstBallVelocityResult;
                        ballState->targetVelocity = ballState->velocity;
                    }
                    break;
                case 2:
                    ballState = collisionEvents[ballIndex].firstBall;
                    horizontalPosition = ballState->currentPosition;
                    horizontalPosition.y = 0.0f;
                    VECNormalize(&horizontalPosition, &collisionNormal);
                    boundaryPenetration = VECMag(&horizontalPosition);
                    boundaryPenetration = 350.0f;
                    ballState->currentPosition.x = boundaryPenetration * collisionNormal.x;
                    ballState->currentPosition.z = boundaryPenetration * collisionNormal.z;
                    horizontalPosition = ballState->currentPosition;
                    horizontalPosition.y = 0.0f;
                    if (ballState->state == 1) {
                        VectorReflect(ballState->targetVelocity, collisionNormal, 0.8f, &firstBallVelocityResult);
                        ballState->targetVelocity = firstBallVelocityResult;
                        VECAdd(&ballState->targetVelocity, &ballState->positionDelta, &ballState->velocity);
                    }
                    else {
                        VectorReflect(ballState->velocity, collisionNormal, 0.8f, &firstBallVelocityResult);
                        ballState->velocity = firstBallVelocityResult;
                        ballState->targetVelocity = ballState->velocity;
                    }
                    break;
                case 3:
                    ballState = collisionEvents[ballIndex].firstBall;
                    otherBallState = collisionEvents[ballIndex].secondBall;
                    if (ballState->state == 3) {
                        SpheresResolveCollision(&otherBallState->position, &otherBallState->velocity, 100.0f, &ballState->position, &ballState->velocity, 100.0f, 0.8f);
                        ballState->targetVelocity = ballState->velocity;
                        otherBallState->targetVelocity = otherBallState->velocity;
                    }
                    else if (otherBallState->state == 3) {
                        SpheresResolveCollision(&ballState->position, &ballState->velocity, 100.0f, &otherBallState->position, &otherBallState->velocity, 100.0f, 0.8f);
                        ballState->targetVelocity = ballState->velocity;
                        otherBallState->targetVelocity = otherBallState->velocity;
                    }
                    else if ((ballState->state == 1) && (otherBallState->state == 1)) {
                        VelocitySplitByCollision(ballState->currentPosition, ballState->targetVelocity, otherBallState->currentPosition, 0.8f, &firstBallVelocityResult, &firstBallImpulseResult);
                        VelocitySplitByCollision(otherBallState->currentPosition, otherBallState->targetVelocity, ballState->currentPosition, 0.8f, &secondBallVelocityResult, &secondBallImpulseResult);
                        VECAdd(&firstBallVelocityResult, &secondBallImpulseResult, &ballState->targetVelocity);
                        VECAdd(&firstBallImpulseResult, &secondBallVelocityResult, &otherBallState->targetVelocity);
                        VECAdd(&ballState->targetVelocity, &ballState->positionDelta, &ballState->velocity);
                        VECAdd(&otherBallState->targetVelocity, &otherBallState->positionDelta, &otherBallState->velocity);
                    }
                    else {
                        VelocitySplitByCollision(ballState->currentPosition, ballState->velocity, otherBallState->currentPosition, 0.8f, &firstBallVelocityResult, &firstBallImpulseResult);
                        VelocitySplitByCollision(otherBallState->currentPosition, otherBallState->velocity, ballState->currentPosition, 0.8f, &secondBallVelocityResult, &secondBallImpulseResult);
                        VECAdd(&firstBallVelocityResult, &secondBallImpulseResult, &ballState->velocity);
                        VECAdd(&firstBallImpulseResult, &secondBallVelocityResult, &otherBallState->velocity);
                        ballState->targetVelocity = ballState->velocity;
                        otherBallState->targetVelocity = otherBallState->velocity;
                    }
                    break;
            }
        }
    }
    ballState = ballPhysicsStates;
    for (ballIndex = 0; ballIndex < ballCount; ballIndex++, ballState++) {
        ballState->position.x = ballState->currentPosition.x + (ballState->velocity.x * remainingFrameTime);
        ballState->position.y = ballState->currentPosition.y + (ballState->velocity.y * remainingFrameTime);
        ballState->position.z = ballState->currentPosition.z + (ballState->velocity.z * remainingFrameTime);
    }
}

void BallObjectsSetup(void)
{
    s32 ballIndex;

    for (ballIndex = 0; ballIndex < 0x20; ballIndex++) {
        ballAllocationSlots[ballIndex] = 0;
    }
}

void *BallStateAlloc(s32 allocationSize)
{
    s32 allocationSlot;

    for (allocationSlot = 0; allocationSlot < 0x20; allocationSlot++) {
        if (ballAllocationSlots[allocationSlot] == 0)
            break;
    }
    if (allocationSlot == 0x20) {
        return NULL;
    }
    ballAllocationSlots[allocationSlot] = HuMemDirectMallocNum(HEAP_HEAP, allocationSize, 0x10000000);
    return ballAllocationSlots[allocationSlot];
}

void BallStateFree(void *stateAllocation)
{
    s32 slotIndex;

    for (slotIndex = 0; slotIndex < 0x20; slotIndex++) {
        if (ballAllocationSlots[slotIndex] == stateAllocation)
            break;
    }
    if (slotIndex != 0x20) {
        HuMemDirectFree(ballAllocationSlots[slotIndex]);
        ballAllocationSlots[slotIndex] = NULL;
    }
}

void BallObjectsDestroy(void)
{
    s32 slotIndex;

    for (slotIndex = 0; slotIndex < 0x20; slotIndex++) {
        if (ballAllocationSlots[slotIndex]) {
            HuMemDirectFree(ballAllocationSlots[slotIndex]);
            ballAllocationSlots[slotIndex] = NULL;
        }
    }
}

void ModelMatrixApply(s16 modelId, Mtx modelMatrix)
{
    HU3DMODEL *model;

    model = &Hu3DData[modelId];
    MTXCopy(modelMatrix, model->mtx);
}

u32 randomSeed = 0x41C64E6D;

s32 RandomU16Get(void)
{
    randomSeed *= 0x41C64E6D;
    randomSeed += 0x3039;
    return randomSeed >> 0x10;
}

void RandomIndicesShuffle(s32 itemCount, u32 *indices)
{
    s32 index;
    s32 swapIndex;
    u32 savedIndexValue;

    for (index = 0; index < itemCount; index++) {
        indices[index] = index;
    }

    for (index = 0; index < itemCount - 1; index++) {
        swapIndex = (itemCount * (RandomU16Get() / 65536.0f));
        savedIndexValue = indices[index];
        indices[index] = indices[swapIndex];
        indices[swapIndex] = savedIndexValue;
    }
}

f32 AngleApproachDegrees(f32 currentAngle, f32 targetAngle, f32 approachFactor)
{
    f32 angleDifference;

    if (currentAngle > 180.0f) {
        currentAngle -= 360.0f;
    }
    else {
        if (currentAngle <= -180.0f) {
            currentAngle += 360.0f;
        }
    }
    if (targetAngle > 180.0f) {
        targetAngle -= 360.0f;
    }
    else {
        if (targetAngle <= -180.0f) {
            targetAngle += 360.0f;
        }
    }
    angleDifference = currentAngle - targetAngle;
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    else {
        if (angleDifference <= -180.0f) {
            angleDifference += 360.0f;
        }
    }
    currentAngle = targetAngle + (angleDifference * approachFactor);
    if (currentAngle > 180.0f) {
        return currentAngle - 360.0f;
    }
    if (currentAngle <= -180.0f) {
        currentAngle += 360.0f;
    }
    return currentAngle;
}

void RotationMatrixCompose(Mtx rotationMatrix, f32 xRotationDegrees, f32 yRotationDegrees, f32 zRotationDegrees)
{
    Mtx xRotationMatrix;
    Mtx yRotationMatrix;

    if (zRotationDegrees != 0.0f) {
        MTXRotRad(rotationMatrix, 0x5A, MTXDegToRad(zRotationDegrees));
    }
    else {
        MTXIdentity(rotationMatrix);
    }
    if (xRotationDegrees != 0.0f) {
        MTXRotRad(xRotationMatrix, 0x58, MTXDegToRad(xRotationDegrees));
        MTXConcat(xRotationMatrix, rotationMatrix, rotationMatrix);
    }
    if (yRotationDegrees != 0.0f) {
        MTXRotRad(yRotationMatrix, 0x59, MTXDegToRad(yRotationDegrees));
        MTXConcat(yRotationMatrix, rotationMatrix, rotationMatrix);
    }
}

void BallCollisionStateToEuler(M424BallCollisionState *collisionState, Vec *eulerAngles)
{
    f32 negativeZComponent;
    f32 horizontalComponent;

    eulerAngles->x = atan2d(collisionState->targetRotation, collisionState->motionFrame);
    if (eulerAngles->x < 0.0f) {
        eulerAngles->x += 360.0f;
    }
    eulerAngles->z = atan2d(collisionState->positionY, collisionState->positionX);
    if (eulerAngles->z < 0.0f) {
        eulerAngles->z += 360.0f;
    }
    negativeZComponent = -collisionState->positionZ;
    horizontalComponent = sqrtf(ABS(1.0 - (negativeZComponent * negativeZComponent)));
    if ((eulerAngles->x > 90.0f) && (eulerAngles->x < 270.0f) && (eulerAngles->z > 90.0f) && (eulerAngles->z < 270.0f)) {
        eulerAngles->x = fmod((180.0f + eulerAngles->x), 360.0);
        eulerAngles->z = fmod((180.0f + eulerAngles->z), 360.0);
        horizontalComponent = -horizontalComponent;
    }
    eulerAngles->y = atan2d(negativeZComponent, horizontalComponent);
    if (eulerAngles->y < 0.0f) {
        eulerAngles->y += 360.0f;
    }
}

void BallCollisionStateToEulerAlt(M424BallCollisionState *collisionState, Vec *eulerAngles)
{
    f32 negativeScaleComponent;
    f32 horizontalComponent;

    eulerAngles->z = atan2d(collisionState->positionY, collisionState->targetPosition);
    if (eulerAngles->z < 0.0f) {
        eulerAngles->z += 360.0f;
    }
    eulerAngles->y = atan2d(collisionState->positionZ, collisionState->motionFrame);
    if (eulerAngles->y < 0.0f) {
        eulerAngles->y += 360.0f;
    }
    negativeScaleComponent = -collisionState->targetScale;
    horizontalComponent = sqrtf(ABS(1.0 - (negativeScaleComponent * negativeScaleComponent)));
    eulerAngles->x = atan2d(negativeScaleComponent, horizontalComponent);
    if (eulerAngles->x < 0.0f) {
        eulerAngles->x += 360.0f;
    }
}

f32 SphereCollisionTime3d(Vec firstPosition, Vec firstVelocity, f32 firstRadius, Vec secondPosition, Vec secondVelocity, f32 secondRadius)
{
    f32 discriminant;
    f32 combinedRadius;
    f32 laterCollisionTime;
    f32 velocitySquared;
    f32 twicePositionVelocityDot;
    f32 earlierCollisionTime;
    f32 positionSquaredMinusRadius;

    VECSubtract(&secondPosition, &firstPosition, &secondPosition);
    VECSubtract(&secondVelocity, &firstVelocity, &secondVelocity);
    combinedRadius = firstRadius + secondRadius;
    if (VECMag(&secondPosition) > combinedRadius + VECMag(&secondVelocity)) {
        return -1.0f;
    }
    combinedRadius *= combinedRadius;
    velocitySquared = (secondVelocity.z * secondVelocity.z) + ((secondVelocity.x * secondVelocity.x) + (secondVelocity.y * secondVelocity.y));
    twicePositionVelocityDot = 2.0f * ((secondVelocity.z * secondPosition.z) + ((secondVelocity.x * secondPosition.x) + (secondVelocity.y * secondPosition.y)));
    positionSquaredMinusRadius = ((secondPosition.z * secondPosition.z) + ((secondPosition.x * secondPosition.x) + (secondPosition.y * secondPosition.y))) - combinedRadius;
    if (velocitySquared == 0.0f) {
        return -1.0f;
    }
    discriminant = (twicePositionVelocityDot * twicePositionVelocityDot) - (4.0f * velocitySquared * positionSquaredMinusRadius);
    if (discriminant <= 0.0f) {
        return -1.0f;
    }
    discriminant = sqrtf(discriminant);
    earlierCollisionTime = (-twicePositionVelocityDot + discriminant) / (2.0f * velocitySquared);
    laterCollisionTime = (-twicePositionVelocityDot - discriminant) / (2.0f * velocitySquared);
    if (((earlierCollisionTime * laterCollisionTime) < 0.0f) && (laterCollisionTime < 0.0f) && (earlierCollisionTime > fabs(laterCollisionTime))) {
        return 0.0f;
    }
    return laterCollisionTime;
}

f32 SphereCollisionTimeXz(Vec startPosition, Vec velocity, f32 firstRadius, f32 secondRadius)
{
    Vec flatPosition;
    Vec flatVelocity;
    f32 discriminant;
    f32 combinedRadius;
    f32 laterCollisionTime;
    f32 velocitySquared;
    f32 twicePositionVelocityDot;
    f32 earlierCollisionTime;
    f32 positionSquaredMinusRadius;

    flatPosition = startPosition;
    flatPosition.y = 0.0f;
    flatVelocity = velocity;
    flatVelocity.y = 0.0f;
    combinedRadius = secondRadius + firstRadius;
    if (VECMag(&flatPosition) > combinedRadius + VECMag(&flatVelocity)) {
        return -1.0f;
    }
    combinedRadius *= combinedRadius;
    velocitySquared = (velocity.x * velocity.x) + (velocity.z * velocity.z);
    twicePositionVelocityDot = 2.0f * ((velocity.x * startPosition.x) + (velocity.z * startPosition.z));
    positionSquaredMinusRadius = (((startPosition.x * startPosition.x) + (startPosition.z * startPosition.z)) - combinedRadius);
    if (0.0f == velocitySquared) {
        return -1.0f;
    }
    discriminant = (twicePositionVelocityDot * twicePositionVelocityDot) - (4.0f * velocitySquared * positionSquaredMinusRadius);
    if (discriminant <= 0.0f) {
        return -1.0f;
    }
    discriminant = sqrtf(discriminant);
    earlierCollisionTime = (-twicePositionVelocityDot + discriminant) / (2.0f * velocitySquared);
    laterCollisionTime = (-twicePositionVelocityDot - discriminant) / (2.0f * velocitySquared);
    if (((earlierCollisionTime * laterCollisionTime) < 0.0f) && (laterCollisionTime < 0.0f) && (earlierCollisionTime > fabs(laterCollisionTime))) {
        return 0.0f;
    }
    return laterCollisionTime;
}

f32 SphereBoundaryTimeXz(Vec startPosition, Vec velocity, f32 sphereRadius, f32 boundaryRadius)
{
    Vec flatPosition;
    Vec flatVelocity;
    f32 discriminant;
    f32 availableRadius;
    f32 laterCollisionTime;
    f32 velocitySquared;
    f32 twicePositionVelocityDot;
    f32 earlierCollisionTime;
    f32 positionSquaredMinusRadius;

    flatPosition = startPosition;
    flatPosition.y = 0.0f;
    flatVelocity = velocity;
    flatVelocity.y = 0.0f;
    availableRadius = boundaryRadius - sphereRadius;
    if (VECMag(&flatPosition) > availableRadius) {
        return 0.0f;
    }
    if (VECMag(&flatPosition) > availableRadius + VECMag(&flatVelocity)) {
        return -1.0f;
    }
    availableRadius *= availableRadius;
    velocitySquared = (velocity.x * velocity.x) + (velocity.z * velocity.z);
    twicePositionVelocityDot = 2.0f * ((velocity.x * startPosition.x) + (velocity.z * startPosition.z));
    positionSquaredMinusRadius = (startPosition.x * startPosition.x) + (startPosition.z * startPosition.z) - availableRadius;
    if (0.0f == velocitySquared) {
        return -1.0f;
    }
    discriminant = (twicePositionVelocityDot * twicePositionVelocityDot) - (4.0f * velocitySquared * positionSquaredMinusRadius);
    if (discriminant <= 0.0f) {
        return -1.0f;
    }
    discriminant = sqrtf(discriminant);
    earlierCollisionTime = (-twicePositionVelocityDot - discriminant) / (2.0f * velocitySquared);
    laterCollisionTime = (-twicePositionVelocityDot + discriminant) / (2.0f * velocitySquared);
    if (((earlierCollisionTime * laterCollisionTime) < 0.0f) && (laterCollisionTime < 0.0f) && (earlierCollisionTime > fabs(laterCollisionTime))) {
        return 0.0f;
    }
    return laterCollisionTime;
}

void VelocitySplitByCollision(Vec position, Vec incomingVelocity, Vec otherPosition, f32 restitution, Vec *tangentialVelocity, Vec *normalVelocity)
{
    Vec normalizedVelocity;
    Vec collisionNormal;
    Vec tangentDirection;
    Vec crossProduct;
    f32 incomingSpeed;
    f32 tangentialSpeed;
    f32 normalSpeed;
    f32 normalDotProduct;

    normalizedVelocity = incomingVelocity;
    incomingSpeed = VECMag(&normalizedVelocity);
    if (incomingSpeed < 0.01f) {
        tangentialVelocity->x = tangentialVelocity->y = tangentialVelocity->z = 0.0f;
        normalVelocity->x = normalVelocity->y = normalVelocity->z = 0.0f;
        return;
    }
    VECNormalize(&normalizedVelocity, &normalizedVelocity);
    incomingSpeed = incomingSpeed * restitution;
    VECSubtract(&otherPosition, &position, &collisionNormal);
    VECNormalize(&collisionNormal, &collisionNormal);
    normalDotProduct = VECDotProduct(&normalizedVelocity, &collisionNormal);
    if (normalDotProduct < 0.0f) {
        tangentialVelocity->x = incomingVelocity.x;
        tangentialVelocity->y = incomingVelocity.y;
        tangentialVelocity->z = incomingVelocity.z;
        normalVelocity->x = normalVelocity->y = normalVelocity->z = 0.0f;
        return;
    }
    normalSpeed = incomingSpeed * normalDotProduct;
    tangentialSpeed = incomingSpeed - normalSpeed;
    if (tangentialSpeed < 0.01f) {
        tangentialVelocity->x = tangentialVelocity->y = tangentialVelocity->z = 0.0f;
    }
    else {
        VECCrossProduct(&normalizedVelocity, &collisionNormal, &crossProduct);
        VECNormalize(&crossProduct, &crossProduct);
        VECCrossProduct(&collisionNormal, &crossProduct, &tangentDirection);
        VECNormalize(&tangentDirection, &tangentDirection);
        tangentialVelocity->x = tangentDirection.x * tangentialSpeed;
        tangentialVelocity->y = tangentDirection.y * tangentialSpeed;
        tangentialVelocity->z = tangentDirection.z * tangentialSpeed;
    }
    normalVelocity->x = collisionNormal.x * normalSpeed;
    normalVelocity->y = collisionNormal.y * normalSpeed;
    normalVelocity->z = collisionNormal.z * normalSpeed;
}

void SpheresResolveCollision(Vec *firstPosition, Vec *firstVelocity, f32 firstRadius, Vec *secondPosition, Vec *secondVelocity, f32 secondRadius, f32 restitution)
{
    Vec relativeVelocity;
    Vec collisionNormal;
    f32 penetrationDepth;
    f32 relativeSpeed;
    f32 normalSpeed;

    firstVelocity->x *= restitution;
    firstVelocity->y *= restitution;
    firstVelocity->z *= restitution;
    VECSubtract(firstPosition, secondPosition, &relativeVelocity);
    VECNormalize(&relativeVelocity, &collisionNormal);
    penetrationDepth = (firstRadius + secondRadius) - VECMag(&relativeVelocity);
    if (penetrationDepth > 0.01f) {
        VECSubtract(firstVelocity, secondVelocity, &relativeVelocity);
        if (VECMag(&relativeVelocity) < 0.01f) {
            firstPosition->x += collisionNormal.x * penetrationDepth;
            firstPosition->y += collisionNormal.y * penetrationDepth;
            firstPosition->z += collisionNormal.z * penetrationDepth;
        }
        else {
            VECNormalize(&relativeVelocity, &relativeVelocity);
            firstPosition->x -= relativeVelocity.x * penetrationDepth;
            firstPosition->y -= relativeVelocity.y * penetrationDepth;
            firstPosition->z -= relativeVelocity.z * penetrationDepth;
        }
    }
    VectorReflect(*firstVelocity, collisionNormal, 1.0f, firstVelocity);
    VECSubtract(secondVelocity, firstVelocity, &relativeVelocity);
    relativeSpeed = VECMag(&relativeVelocity);
    if (relativeSpeed < 0.01f)
        return;
    VECNormalize(&relativeVelocity, &relativeVelocity);
    normalSpeed = VECDotProduct(&collisionNormal, &relativeVelocity);
    if (normalSpeed > 0.0f) {
        firstVelocity->x += relativeSpeed * (collisionNormal.x * normalSpeed);
        firstVelocity->y += relativeSpeed * (collisionNormal.y * normalSpeed);
        firstVelocity->z += relativeSpeed * (collisionNormal.z * normalSpeed);
    }
}

void VectorReflect(Vec incomingVector, Vec surfaceNormal, f32 restitution, Vec *reflectedVector)
{
    f32 normalSpeed;

    *reflectedVector = incomingVector;
    reflectedVector->x *= restitution;
    reflectedVector->y *= restitution;
    reflectedVector->z *= restitution;
    normalSpeed = VECDotProduct(&surfaceNormal, reflectedVector);
    if (normalSpeed < 0.0f) {
        reflectedVector->x -= 2.0f * surfaceNormal.x * normalSpeed;
        reflectedVector->y -= 2.0f * surfaceNormal.y * normalSpeed;
        reflectedVector->z -= 2.0f * surfaceNormal.z * normalSpeed;
    }
}

f32 VectorNormalize(Vec *vector)
{
    f32 vectorLength;
    f32 inverseLength;

    vectorLength = (vector->x * vector->x) + (vector->y * vector->y) + (vector->z * vector->z);
    vectorLength = sqrtf(vectorLength);
    if (vectorLength != 0.0f) {
        inverseLength = 1.0f / vectorLength;
        vector->x *= inverseLength;
        vector->y *= inverseLength;
        vector->z *= inverseLength;
    }
    else {
        vector->x = vector->y = vector->z = 0.0f;
    }
    return vectorLength;
}

f32 LineSegmentPointGet(Vec segmentStart, Vec segmentDirection, Vec *pointOnSegment, f32 segmentFraction)
{
    if (segmentFraction <= 0.0f) {
        pointOnSegment->x = segmentStart.x;
        pointOnSegment->y = segmentStart.y;
        pointOnSegment->z = segmentStart.z;
        segmentFraction = 0.0f;
    }
    else if (segmentFraction >= 1.0f) {
        pointOnSegment->x = (segmentStart.x + segmentDirection.x);
        pointOnSegment->y = (segmentStart.y + segmentDirection.y);
        pointOnSegment->z = (segmentStart.z + segmentDirection.z);
        segmentFraction = 1.0f;
    }
    else {
        pointOnSegment->x = (segmentStart.x + (segmentFraction * segmentDirection.x));
        pointOnSegment->y = (segmentStart.y + (segmentFraction * segmentDirection.y));
        pointOnSegment->z = (segmentStart.z + (segmentFraction * segmentDirection.z));
    }
    return segmentFraction;
}

f32 LineSegmentProjectionGet(Vec segmentStart, Vec segmentEnd, Vec point)
{
    f32 negativeSegmentLengthSquared;
    f32 segmentFraction;

    segmentFraction = (point.z * (segmentEnd.z - segmentStart.z)) + ((point.x * (segmentEnd.x - segmentStart.x)) + (point.y * (segmentEnd.y - segmentStart.y)));
    negativeSegmentLengthSquared = -((point.z * point.z) + ((point.x * point.x) + (point.y * point.y)));
    if (negativeSegmentLengthSquared != 0.0f) {
        segmentFraction /= negativeSegmentLengthSquared;
    }
    return segmentFraction;
}

f32 PointLineSegmentDistanceSquared(Vec *point, Vec *segmentStart, Vec *segmentDirection)
{
    Vec closestSegmentPoint;
    f32 segmentFraction;

    if (0.0f == (segmentDirection->x * segmentDirection->x) + (segmentDirection->y * segmentDirection->y) + (segmentDirection->z * segmentDirection->z)) {
        return ((point->x - segmentStart->x) * (point->x - segmentStart->x)) + ((point->y - segmentStart->y) * (point->y - segmentStart->y))
            + ((point->z - segmentStart->z) * (point->z - segmentStart->z));
    }
    segmentFraction = LineSegmentProjectionGet(*point, *segmentStart, *segmentDirection);
    LineSegmentPointGet(*segmentStart, *segmentDirection, &closestSegmentPoint, segmentFraction);
    return ((point->x - closestSegmentPoint.x) * (point->x - closestSegmentPoint.x)) + ((point->y - closestSegmentPoint.y) * (point->y - closestSegmentPoint.y)) + ((point->z - closestSegmentPoint.z) * (point->z - closestSegmentPoint.z));
}

f32 PointLineSegmentDistanceGet(Vec *point, Vec *segmentStart, Vec *segmentDirection, Vec *closestSegmentPoint, Vec *differenceVector)
{
    f32 segmentFraction;

    if (0.0f == ((segmentDirection->z * segmentDirection->z) + ((segmentDirection->x * segmentDirection->x) + (segmentDirection->y * segmentDirection->y)))) {
        return ((point->z - segmentStart->z) * (point->z - segmentStart->z))
            + (((point->x - segmentStart->x) * (point->x - segmentStart->x)) + ((point->y - segmentStart->y) * (point->y - segmentStart->y)));
    }
    segmentFraction = LineSegmentProjectionGet(*point, *segmentStart, *segmentDirection);
    LineSegmentPointGet(*segmentStart, *segmentDirection, closestSegmentPoint, segmentFraction);
    differenceVector->x = point->x - closestSegmentPoint->x;
    differenceVector->y = point->y - closestSegmentPoint->y;
    differenceVector->z = point->z - closestSegmentPoint->z;
    return VectorNormalize(differenceVector);
}

void BallPositionSet(u8 ballIndex, f32 positionX, f32 positionY, f32 positionZ)
{
    M424BallPhysicsState *ballState;

    ballState = &ballPhysicsStates[ballIndex];
    ballState->state = 3;
    ballState->position.x = positionX;
    ballState->position.y = positionY;
    ballState->position.z = positionZ;
}

void BallStateSet(u8 ballIndex, u8 newState)
{
    M424BallPhysicsState *ballState;

    ballState = &ballPhysicsStates[ballIndex];
    ballState->state = newState;
}

u8 BallStateGet(u8 ballIndex)
{
    M424BallPhysicsState *ballState;

    ballState = &ballPhysicsStates[ballIndex];
    return ballState->state;
}

u8 BallControllerStateGet(u8 ballIndex)
{
    M424BallPhysicsState *ballState;

    ballState = &ballPhysicsStates[ballIndex];
    return ballState->controllerState;
}

u8 BallPlayerIndexGet(u8 ballIndex)
{
    M424BallPhysicsState *ballState;

    ballState = &ballPhysicsStates[ballIndex];
    return ballState->playerIndex;
}

Vec *BallPositionPointerGet(u8 ballIndex)
{
    M424BallPhysicsState *ballState;

    ballState = &ballPhysicsStates[ballIndex];
    return &ballState->position;
}

Vec *BallVelocityPointerGet(u8 ballIndex)
{
    M424BallPhysicsState *ballState;

    ballState = &ballPhysicsStates[ballIndex];
    return &ballState->velocity;
}

s32 ActiveBallExists(void)
{
    M424BallPhysicsState *ballState;
    M424CharacterBallState *characterState;
    s32 ballIndex;

    ballState = ballPhysicsStates;
    for (ballIndex = 0; ballIndex < ballCount; ballIndex++, ballState++) {
        characterState = ballState->object->data;
        if ((characterState->actionState == 3) || (characterState->actionState == 2)) {
            return 0;
        }
    }
    return 1;
}

void BallOrderAdjust(s32 collisionRetryCount, s32 updatePositions)
{
    Vec clampedPosition;
    M424BallPhysicsState *ballState;
    s32 collisionResolved;
    s32 remainingRetries;
    s32 ballIndex;

    if (updatePositions != 0) {
        for (ballIndex = 0; ballIndex < ballCount; ballIndex++) {
            ballState = &ballPhysicsStates[ballIndex];
            if (ballState->state != 6) {
                VECAdd(&ballState->currentPosition, &ballState->velocity, &ballState->position);
                if ((ballState->state < 3) && (BallPositionClamp(&ballState->position, &clampedPosition) != 0)) {
                    ballState->position = clampedPosition;
                }
                if ((ballState->velocity.y < 0.0f) && (ballState->state != 4) && (ballState->state != 6) && (ballState->position.y < 300.0f)) {
                    ballState->position.y = 300.0f;
                    if (ballState->state == 2) {
                        ballState->state = 1;
                    }
                }
            }
        }
    }

    for (ballIndex = 0, remainingRetries = collisionRetryCount; ballIndex < ballCount; ballIndex++) {
        ballState = &ballPhysicsStates[ballIndex];
        collisionResolved = 0;
        if (BallStateGet(ballIndex) < 4) {
            if (BallCollisionResolve(ballIndex, collisionRetryCount) < collisionRetryCount) {
                collisionResolved = 1;
            }
            if ((collisionResolved != 0) && (remainingRetries > 0)) {
                remainingRetries -= 1;
                ballIndex -= 1;
            }
            else {
                remainingRetries = collisionRetryCount;
            }
        }
    }
    ClawBallCollisionsUpdate(0);
}

s32 BallCollisionResolve(s32 ballIndex, s32 recursionDepth)
{
    Vec ballPosition;
    Vec otherBallPosition;
    Vec separationVector;
    Vec clampedPosition;
    Vec *unusedPointer;
    f32 separationScale;
    f32 separationDistance;
    s32 otherBallIndex;
    M424BallPhysicsState *ballPhysicsState;
    M424BallPhysicsState *otherBallPhysicsState;
    Vec *ballPositionPointer;
    Vec *otherBallPositionPointer;
    M424CharacterBallState *ballCharacterState;
    M424CharacterBallState *otherCharacterState;

    ballPhysicsState = &ballPhysicsStates[ballIndex];
    ballCharacterState = ballPhysicsState->object->data;
    ballPositionPointer = BallPositionPointerGet(ballIndex);
    ballPosition = *ballPositionPointer;

    for (otherBallIndex = 0; otherBallIndex < ballCount; otherBallIndex++) {
        if (otherBallIndex != ballIndex) {
            if (BallStateGet(otherBallIndex) < 3) {
                otherBallPhysicsState = &ballPhysicsStates[otherBallIndex];
                otherCharacterState = otherBallPhysicsState->object->data;
                otherBallPositionPointer = BallPositionPointerGet(otherBallIndex);
                otherBallPosition = *otherBallPositionPointer;
                VECSubtract(&otherBallPosition, &ballPosition, &separationVector);
                separationDistance = VECMag(&separationVector);
                if (separationDistance < 200.0f) {
                    if (separationDistance > 0.0f) {
                        separationScale = 1.0f / separationDistance;
                        separationVector.x *= separationScale;
                        separationVector.y *= separationScale;
                        separationVector.z *= separationScale;
                    }
                    else {
                        separationVector.x = 0.0f;
                        separationVector.y = 0.0f;
                        separationVector.z = 1.0f;
                    }
                    separationScale = 1.0f + (0.5f * (200.0f - separationDistance));
                    if (separationScale > 0.0f) {
                        VECScale(&separationVector, &separationVector, separationScale);
                        VECAdd(&otherBallPosition, &separationVector, &otherBallPosition);
                        *otherBallPositionPointer = otherBallPosition;
                        if ((otherBallPhysicsState->controllerState < 3) && (BallPositionClamp(&otherBallPosition, &clampedPosition) != 0)) {
                            otherBallPosition = clampedPosition;
                            *otherBallPositionPointer = clampedPosition;
                        }
                        if ((otherBallPhysicsState->controllerState == 2) && (otherCharacterState->motionState.y < 0.0f)) {
                            otherCharacterState->motionState.y += 0.1f * separationScale;
                        }
                    }
                    if (separationScale > 0.0f) {
                        VECScale(&separationVector, &separationVector, -1.0f);
                        VECAdd(&ballPosition, &separationVector, &ballPosition);
                        *ballPositionPointer = ballPosition;
                        if ((ballPhysicsState->controllerState < 3) && (BallPositionClamp(&ballPosition, &clampedPosition) != 0)) {
                            ballPosition = clampedPosition;
                            *ballPositionPointer = clampedPosition;
                        }
                        if ((ballPhysicsState->controllerState == 2) && (ballCharacterState->motionState.y < 0.0f)) {
                            ballCharacterState->motionState.y += 0.1f * separationScale;
                        }
                    }
                    if (recursionDepth > 0) {
                        recursionDepth--;
                        BallCollisionResolve(otherBallIndex, recursionDepth);
                    }
                }
            }
        }
    }
    return recursionDepth;
}

s32 BallPositionClamp(Vec *position, Vec *clampedPosition)
{
    Vec adjustedPosition;
    f32 radiusSquared;
    f32 adjustedRadiusSquared;
    f32 radius;
    f32 inverseRadius;

    adjustedPosition = *position;
    *clampedPosition = *position;
    radiusSquared = (adjustedPosition.x * adjustedPosition.x) + (adjustedPosition.z * adjustedPosition.z);
    radius = sqrtf(radiusSquared);
    if (radius < 350.0f) {
        if (radius > 0.0f) {
            inverseRadius = 1.0f / radius;
            adjustedPosition.x *= inverseRadius;
            adjustedPosition.z *= inverseRadius;
        }
        else {
            adjustedPosition.x = 1.0f;
            adjustedPosition.z = 0.0f;
        }
        clampedPosition->x = 350.0f * adjustedPosition.x;
        clampedPosition->z = 350.0f * adjustedPosition.z;
    }
    adjustedRadiusSquared = (adjustedPosition.x * adjustedPosition.x) + (adjustedPosition.z * adjustedPosition.z);
    radius = sqrtf(adjustedRadiusSquared);
    if (radius > 600.0f) {
        inverseRadius = 1.0f / radius;
        adjustedPosition.x *= inverseRadius;
        adjustedPosition.z *= inverseRadius;
        clampedPosition->x = 600.0f * adjustedPosition.x;
        clampedPosition->z = 600.0f * adjustedPosition.z;
    }
    return (s32)position;
}
