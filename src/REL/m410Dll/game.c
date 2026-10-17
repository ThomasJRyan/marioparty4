#include "ext_math.h"
#include "game/audio.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/sprite.h"

#include "REL/m410Dll.h"
#include "version.h"
#include <string.h>

typedef void (*GoalCallback)(u16, u16, u16);

OMOBJ *gameObject;
OMOBJ *playerSphereObject;
OMOBJ *goalObject;
float currentGoalMotion;
float targetGoalMotion;
float goalMotionStep;
GoalCallback goalCallback;
HUPROCESS *gameObjectManager;
s32 goalActivationTimer;
s32 goalParticleModelId;
s32 goalSoundEffectId;
s32 jointSearchSucceeded;

s16 goalAnimationTimer = 15 * REFRESH_RATE;
s32 characterModelDataIds[]
    = { DATA_MAKE_NUM(DATADIR_M410, 0), DATA_MAKE_NUM(DATADIR_M410, 1), DATA_MAKE_NUM(DATADIR_M410, 2), DATA_MAKE_NUM(DATADIR_M410, 3),
          DATA_MAKE_NUM(DATADIR_M410, 4), DATA_MAKE_NUM(DATADIR_M410, 5), DATA_MAKE_NUM(DATADIR_M410, 6), DATA_MAKE_NUM(DATADIR_M410, 7) };

void CreatePlayerModels(OMOBJ *object);
void UpdatePlayerModels(OMOBJ *object);
void UpdatePlayerModelLayer(s16 layer);
float CalculateBoundaryCollisionTime(PlayerPhysicsWork *physicsWork, u8 *collisionAxisOut);
float CalculateSphereCollisionTime(Vec startPosition, Vec velocity, float movingRadius, Vec targetPosition, Vec targetVelocity, float targetRadius);
float CalculateVerticalSphereCollisionTime(Vec startPosition, Vec velocity, float movingRadius, Vec targetPosition, Vec targetVelocity, float targetRadius, float verticalRadius);
float CalculateHorizontalSphereCollisionTime(Vec startPosition, Vec velocity, float movingRadius, Vec targetPosition, float targetRadius);
s32 TestSweptSphereCollision(Vec *startPosition, Vec *startVelocity, float movingRadius, Vec *endPosition, Vec *endVelocity, float targetRadius, float targetVerticalRadius);
void ReflectVelocityAtBoundary(PlayerPhysicsWork *physicsWork, u8 collisionAxes);
void CalculateSphereCollisionResponse(Vec position, Vec velocity, Vec targetPosition, Vec *responseVelocityOut, Vec *responsePositionOut);
void ResolvePlayerCollisions(PlayerPhysicsWork *firstPlayer, PlayerPhysicsWork *secondPlayer);
void ResolvePlayerCollisionVariant(PlayerPhysicsWork *firstPlayer, PlayerPhysicsWork *secondPlayer);
void ResolveGoalCollision(PlayerPhysicsWork *playerWork, PlayerPhysicsWork *goalWork);
void CreatePlayerModelManager(OMOBJ *object);
void UpdatePlayerModelManager(OMOBJ *object);
void HidePlayerModels(OMOBJ *object);
void CreateGoalObject(OMOBJ *object);
void UpdateGoalObject(OMOBJ *object);
void HideGoalObject(OMOBJ *object);
void CreateGoalParticleEffect(void);
void SetGoalParticlePosition(Vec *position);
void UpdateGoalParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

void InitializeGameObjects(HUPROCESS *objman)
{
    gameObject = omAddObjEx(objman, 3, 3, 0, -1, CreatePlayerModels);
    goalObject = omAddObjEx(objman, 5, 54, 3, -1, CreateGoalObject);
    playerSphereObject = omAddObjEx(objman, 7, 60, 0, -1, CreatePlayerModelManager);
    goalCallback = NULL;
    gameObjectManager = objman;
    goalActivationTimer = 5 * REFRESH_RATE;
    CreateGoalParticleEffect();
}

void SetGameObjectsHidden(void)
{
    HidePlayerModels(playerSphereObject);
    HideGoalObject(goalObject);
}

void CreatePlayerModels(OMOBJ *object)
{
    s32 loopIndex;

    object->stat |= 0x100;
    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        if (!loopIndex) {
            object->mdlId[loopIndex] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M410, 12));
        }
        else {
            object->mdlId[loopIndex] = Hu3DModelLink(*object->mdlId);
        }
        Hu3DModelPosSet(object->mdlId[loopIndex], 0.0f, 140.0f + (140.0f * loopIndex), -790.0f);
    }
    object->objFunc = UpdatePlayerModels;
    Hu3DLayerHookSet(7, UpdatePlayerModelLayer);
    currentGoalMotion = targetGoalMotion = goalMotionStep = 0.0f;
}

void UpdatePlayerModels(OMOBJ *object)
{
    float motionValue;
    float motionValue2;
    float motionValue3;
    HU3DMODEL *model;
    s32 loopIndex;
    s16 stateValue;

    switch (object->work[0]) {
        case 0:
            if (getMinigamePhase() == 1) {
                object->work[0]++;
#if VERSION_PAL
                object->work[1] = 213;
#else
                object->work[1] = 256;
#endif
            }
            break;
        case 1:
            if (!--object->work[1]) {
                object->work[0]++;
                object->work[1] = REFRESH_RATE * 3 / 2;
                currentGoalMotion = 0.0f;
#if VERSION_PAL
                targetGoalMotion = -12.0f;
                goalMotionStep = 2.4f;
#else
                targetGoalMotion = -10.000001f;
                goalMotionStep = 2.0000002f;
#endif
                goalSoundEffectId = HuAudFXPlay(1381);
            }
            break;
        case 2:
            if (!--object->work[1]) {
                object->work[0]++;
                targetGoalMotion = 0.0f;
#if VERSION_PAL
                goalMotionStep = 0.576f;
#else
                goalMotionStep = 0.40000007f;
#endif
            }
            break;
        case 3:
            if (getMinigameSubstate() == 2) {
                object->work[0]++;
#if VERSION_PAL
                targetGoalMotion = -2.4f;
                goalMotionStep = 0.072f;
#else
                targetGoalMotion = -2.0000002f;
                goalMotionStep = 0.05000001f;
#endif
            }
            break;
    }
    if (getMinigameSubstate() == 2) {
        if (!--goalAnimationTimer) {
            targetGoalMotion = -targetGoalMotion;
            goalAnimationTimer = 15 * REFRESH_RATE;
        }
    }
    else if (getMinigameSubstate() > 2) {
        targetGoalMotion = 0.0f;
    }
    if (currentGoalMotion != targetGoalMotion) {
        if (currentGoalMotion < targetGoalMotion) {
            currentGoalMotion += goalMotionStep;
            if (currentGoalMotion > targetGoalMotion) {
                currentGoalMotion = targetGoalMotion;
            }
        }
        else {
            currentGoalMotion -= goalMotionStep;
            if (currentGoalMotion < targetGoalMotion) {
                currentGoalMotion = targetGoalMotion;
            }
        }
    }
    if (goalSoundEffectId >= 0) {
        stateValue = 512.0 * fabs(currentGoalMotion);
        if (stateValue > 127) {
            stateValue = 127;
        }
        HuAudFXVolSet(goalSoundEffectId, stateValue);
        motionValue = (0.5 * (16384.0 * fabs(currentGoalMotion))) - 8191.0;
        if (motionValue < -8191.0f) {
            motionValue = -8191.0f;
        }
        else if (motionValue > 8191.0f) {
            motionValue = 8191.0f;
        }
        HuAudFXPitchSet(goalSoundEffectId, motionValue);
    }

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        model = &Hu3DData[object->mdlId[loopIndex]];
        if (loopIndex & 1) {
            motionValue3 = currentGoalMotion;
        }
        else {
            motionValue3 = -currentGoalMotion;
        }
        motionValue2 = motionValue3;
        model->pos.x += motionValue2;
        if (motionValue2 < 0.0) {
            if (model->pos.x <= -40.0f) {
                model->pos.x += 40.0f;
            }
        }
        else {
            if (model->pos.x >= 40.0f) {
                model->pos.x -= 40.0f;
            }
        }
    }
}

void UpdatePlayerModelLayer(s16 playerIndex) // what happens with the argument?
{
    PlayerCollisionWork collisionWork[128]; // TODO get size right
    PlayerPhysicsWork *physicsWork[60]; // TODO get size right
    Vec temporaryPosition;
    Vec temporaryPosition2;

    u8 temporaryValue;

    f32 motionValue;
    f32 motionValue2;
    f32 motionValue3;

    PlayerPhysicsWork *physicsWork2;
    PlayerPhysicsWork *physicsWork3;
    s32 loopIndex;
    PlayerPhysicsWork *physicsWork4;
    u8 loopIndex2;
    s32 loopIndex3;
    s32 loopIndex4;
    s32 stateValue;
    OMOBJ *object;
    OMOBJ **groupMembers;

    stateValue = 0;
    if (omPauseChk()) {

        return;
    }

    for (loopIndex4 = 0, physicsWork2 = playerSphereObject->data, loopIndex = 0; loopIndex < 60; loopIndex++, physicsWork2++) {
        if (physicsWork2->isActive) {
            physicsWork[loopIndex4++] = physicsWork2;
            physicsWork2->previousPosition = physicsWork2->position;
        }
    }
    for (loopIndex = 0; loopIndex < loopIndex4; loopIndex++) {
        physicsWork2 = physicsWork[loopIndex];
        physicsWork2->collisionUpdatePending = 0;
        physicsWork2->collisionPartner = 0;
        physicsWork2->linkTimer = 0;
    }
    for (physicsWork2 = goalObject->data, loopIndex = 0; loopIndex < 18; loopIndex++, physicsWork2++) {
        physicsWork2->collisionPartner = 0;
        physicsWork2->previousPosition = physicsWork2->position;
    }
    motionValue2 = 1.0f;
    do {
        for (loopIndex2 = 0, motionValue3 = motionValue2, loopIndex = 0; loopIndex < loopIndex4; loopIndex++) {
            physicsWork3 = physicsWork[loopIndex];
            if (physicsWork3->collisionUpdatePending) {
                continue;
            }
            if (physicsWork3->linkTimer >= 8) {
                physicsWork3->velocity.x = physicsWork3->velocity.y = physicsWork3->velocity.z = 0.0f;
                continue;
            }
            physicsWork3->collisionUpdatePending = 1;
            if (!physicsWork3->isConnectedToGoal) {
                motionValue = CalculateBoundaryCollisionTime(physicsWork3, &temporaryValue);
                if (motionValue >= 0.0f && motionValue < motionValue2) {
                    physicsWork3->collisionUpdatePending = 0;
                    if (motionValue <= motionValue3) {
                        if (motionValue < motionValue3) {
                            loopIndex2 = 0;
                        }
                        collisionWork[loopIndex2].collisionType = 0;
                        collisionWork[loopIndex2].firstPhysicsWork = physicsWork3;
                        collisionWork[loopIndex2].collisionAxes = temporaryValue;
                        loopIndex2++;
                        motionValue3 = motionValue;
                        if (!physicsWork3->isVerticalCollisionState && (temporaryValue & 2) && physicsWork3->velocity.y < 0.0f) {
                            physicsWork3->isVerticalCollisionState = 1;
                            physicsWork3->stateTimer = 4 * REFRESH_RATE;
                        }
                    }
                }
            }

            for (loopIndex3 = 0; loopIndex3 < loopIndex4; loopIndex3++) {
                physicsWork4 = physicsWork[loopIndex3];
                if (physicsWork3 != physicsWork4 && (physicsWork3->collisionPartner != physicsWork4 || physicsWork4->collisionPartner != physicsWork3)
                    && !(physicsWork3->isVerticalCollisionState ^ physicsWork4->isVerticalCollisionState)) {
                    motionValue = CalculateSphereCollisionTime(physicsWork3->position, physicsWork3->velocity, physicsWork3->collisionRadius, physicsWork4->position, physicsWork4->velocity, physicsWork4->collisionRadius);
                    if (motionValue >= 0.0f && motionValue < motionValue2) {
                        physicsWork3->collisionUpdatePending = 0;
                        if (motionValue <= motionValue3) {
                            if (motionValue < motionValue3) {
                                loopIndex2 = 0;
                            }
                            collisionWork[loopIndex2].collisionType = 1;
                            collisionWork[loopIndex2].firstPhysicsWork = physicsWork3;
                            collisionWork[loopIndex2].secondPhysicsWork = physicsWork4;
                            loopIndex2++;
                            motionValue3 = motionValue;
                        }
                    }
                }
            }
            groupMembers = omGetGroupMemberListEx(gameObjectManager, 0);
            for (loopIndex3 = 0; loopIndex3 < 4; loopIndex3++) {
                object = groupMembers[loopIndex3];
                if (physicsWork3->collisionPartner != object && physicsWork3->isVerticalCollisionState) {
                    motionValue = CalculateHorizontalSphereCollisionTime(physicsWork3->position, physicsWork3->velocity, physicsWork3->collisionRadius, object->trans, 30.000002f);
                    if (motionValue >= 0.0f && motionValue < motionValue2) {
                        physicsWork3->collisionUpdatePending = 0;
                        if (motionValue <= motionValue3) {
                            if (motionValue < motionValue3) {
                                loopIndex2 = 0;
                            }
                            collisionWork[loopIndex2].collisionType = 2;
                            collisionWork[loopIndex2].firstPhysicsWork = physicsWork3;
                            collisionWork[loopIndex2].collidingObject = object;
                            loopIndex2++;
                            motionValue3 = motionValue;
                        }
                    }
                }
            }
            if (!physicsWork3->isVerticalCollisionState) {
                physicsWork4 = goalObject->data;

                for (loopIndex3 = 0; loopIndex3 < 18; loopIndex3++, physicsWork4++) {
                    if (physicsWork3->collisionPartner != physicsWork4 || physicsWork4->collisionPartner != physicsWork3) {
                        motionValue = CalculateVerticalSphereCollisionTime(
                            physicsWork3->position, physicsWork3->velocity, physicsWork3->collisionRadius, physicsWork4->position, physicsWork4->velocity, physicsWork4->collisionRadius, physicsWork4->collisionHeight);
                        if (motionValue >= 0.0f && motionValue < motionValue2) {
                            physicsWork3->collisionUpdatePending = 0;
                            if (motionValue <= motionValue3) {
                                if (motionValue < motionValue3) {
                                    loopIndex2 = 0;
                                }
                                collisionWork[loopIndex2].collisionType = 3;
                                collisionWork[loopIndex2].firstPhysicsWork = physicsWork3;
                                collisionWork[loopIndex2].secondPhysicsWork = physicsWork4;
                                loopIndex2++;
                                motionValue3 = motionValue;
                            }
                        }
                    }
                }
            }
        }
        if (!loopIndex2) {
            break;
        }
        for (loopIndex = 0; loopIndex < loopIndex4; loopIndex++) {
            physicsWork2 = physicsWork[loopIndex];
            physicsWork2->position.x += physicsWork2->velocity.x * motionValue3 * motionValue2;
            physicsWork2->position.y += physicsWork2->velocity.y * motionValue3 * motionValue2;
            physicsWork2->position.z += physicsWork2->velocity.z * motionValue3 * motionValue2;
        }
        physicsWork2 = goalObject->data;

        for (loopIndex = 0; loopIndex < 18; loopIndex++, physicsWork2++) {
            physicsWork2->position.x += physicsWork2->velocity.x * motionValue3 * motionValue2;
            physicsWork2->position.y += physicsWork2->velocity.y * motionValue3 * motionValue2;
            physicsWork2->position.z += physicsWork2->velocity.z * motionValue3 * motionValue2;
        }
        for (loopIndex = 0; loopIndex < loopIndex2; loopIndex++) {
            switch (collisionWork[loopIndex].collisionType) {
                case 0:
                    ReflectVelocityAtBoundary(collisionWork[loopIndex].firstPhysicsWork, collisionWork[loopIndex].collisionAxes);
                    collisionWork[loopIndex].firstPhysicsWork->collisionPartner = NULL;
                    collisionWork[loopIndex].firstPhysicsWork->linkTimer++;
                    break;
                case 1:
                    ResolvePlayerCollisions(collisionWork[loopIndex].firstPhysicsWork, collisionWork[loopIndex].secondPhysicsWork);
                    collisionWork[loopIndex].firstPhysicsWork->collisionPartner = collisionWork[loopIndex].secondPhysicsWork;
                    collisionWork[loopIndex].secondPhysicsWork->collisionPartner = collisionWork[loopIndex].firstPhysicsWork;
                    collisionWork[loopIndex].firstPhysicsWork->linkTimer++;
                    collisionWork[loopIndex].secondPhysicsWork->linkTimer++;
                    break;
                case 2:
                    ResolveGoalCollision(collisionWork[loopIndex].firstPhysicsWork, collisionWork[loopIndex].secondPhysicsWork);
                    collisionWork[loopIndex].firstPhysicsWork->collisionPartner = collisionWork[loopIndex].secondPhysicsWork;
                    collisionWork[loopIndex].firstPhysicsWork->linkTimer++;
                    break;
                case 3:
                    ResolvePlayerCollisionVariant(collisionWork[loopIndex].firstPhysicsWork, collisionWork[loopIndex].secondPhysicsWork);
                    collisionWork[loopIndex].firstPhysicsWork->collisionPartner = collisionWork[loopIndex].secondPhysicsWork;
                    collisionWork[loopIndex].secondPhysicsWork->collisionPartner = collisionWork[loopIndex].firstPhysicsWork;
                    collisionWork[loopIndex].firstPhysicsWork->linkTimer++;
                    collisionWork[loopIndex].firstPhysicsWork->collisionCooldown = 2;
                    if (VECMag(&collisionWork[loopIndex].firstPhysicsWork->velocity) > 10.0f) {
                        HuAudFXPlay(1377);
                    }
                    break;
            }
        }
        motionValue2 *= 1.0f - motionValue3;
        stateValue++;
    } while (stateValue < 100);
    (void)stateValue;

    for (loopIndex = 0; loopIndex < loopIndex4; loopIndex++) {
        physicsWork2 = physicsWork[loopIndex];
        physicsWork2->position.x += physicsWork2->velocity.x * motionValue2;
        physicsWork2->position.y += physicsWork2->velocity.y * motionValue2;
        physicsWork2->position.z += physicsWork2->velocity.z * motionValue2;
    }
    physicsWork2 = goalObject->data;

    for (loopIndex = 0; loopIndex < 18; loopIndex++, physicsWork2++) {
        physicsWork2->position.x += physicsWork2->velocity.x * motionValue2;
        physicsWork2->position.y += physicsWork2->velocity.y * motionValue2;
        physicsWork2->position.z += physicsWork2->velocity.z * motionValue2;
    }
    for (loopIndex = 0; loopIndex < loopIndex4; loopIndex++) {
        physicsWork3 = physicsWork[loopIndex];
        VECSubtract(&physicsWork3->position, &physicsWork3->previousPosition, &temporaryPosition);
        physicsWork4 = goalObject->data;

        for (loopIndex3 = 0; loopIndex3 < 18; loopIndex3++, physicsWork4++) {
            VECSubtract(&physicsWork4->position, &physicsWork4->previousPosition, &temporaryPosition2);
            if (TestSweptSphereCollision(&physicsWork3->previousPosition, &temporaryPosition, physicsWork3->collisionRadius, &physicsWork4->previousPosition, &temporaryPosition2, physicsWork4->collisionRadius, physicsWork4->collisionHeight)
                && !physicsWork3->isConnectedToGoal && physicsWork4->linkedPhysicsWork == NULL) {
                physicsWork3->isConnectedToGoal = 1;
                physicsWork3->linkedPhysicsWork = physicsWork4;
                physicsWork3->stateTimer = REFRESH_RATE / 6;
                physicsWork4->goalCollisionPending = 1;
                physicsWork4->linkedPhysicsWork = physicsWork3;
                if (goalCallback != NULL) {
                    goalCallback(physicsWork3->playerIndex, (physicsWork4->playerIndex & 1) + 1, physicsWork4->sphereIndex);
                    if (getMinigameSubstate() == 2) {
                        omVibrate(physicsWork3->playerIndex, 12, 6, 6);
                        HuAudFXPlay(9);
                        HuAudFXPlay(1378);
                    }
                }
            }
        }
    }
}

float CalculateBoundaryCollisionTime(PlayerPhysicsWork *physicsWork, u8 *collisionAxisOut)
{
    float motionValue;
    float motionValue2;
    float motionValue3;

    motionValue2 = 1.0f;
    if (0.0f == physicsWork->velocity.x && 0.0f == physicsWork->velocity.y && 0.0f == physicsWork->velocity.z) {
        return -1.0f;
    }
    *collisionAxisOut = 0;
    if (0.0f != physicsWork->velocity.x) {
        if (physicsWork->velocity.x < 0.0f) {
            motionValue3 = (-500.0f - physicsWork->position.x + physicsWork->collisionRadius) / physicsWork->velocity.x;
        }
        else {
            motionValue3 = (500.0f - physicsWork->position.x - physicsWork->collisionRadius) / physicsWork->velocity.x;
        }
        motionValue = motionValue3;
        if (motionValue < 0.0f) {
            motionValue = 0.0f;
        }
        if (motionValue >= 0.0f && motionValue < 1.0f && motionValue < motionValue2) {
            *collisionAxisOut = 1;
            motionValue2 = motionValue;
        }
    }
    if (physicsWork->velocity.y < 0.0f) {
        motionValue = (-physicsWork->position.y + physicsWork->collisionRadius) / physicsWork->velocity.y;
        if (motionValue < 0.0f) {
            motionValue = 0.0f;
        }
        if (motionValue >= 0.0f && motionValue < 1.0f) {
            if (motionValue == motionValue2) {
                *collisionAxisOut |= 2;
            }
            else if (motionValue < motionValue2) {
                motionValue2 = motionValue;
                *collisionAxisOut = 2;
            }
        }
    }
    if (physicsWork->velocity.z < 0.0f) {
        motionValue = (-800.0f - physicsWork->position.z + physicsWork->collisionRadius) / physicsWork->velocity.z;
        if (motionValue < 0.0f) {
            motionValue = 0.0f;
        }
        if (motionValue >= 0.0f && motionValue < 1.0f) {
            if (motionValue == motionValue2) {
                *collisionAxisOut |= 4;
            }
            else if (motionValue < motionValue2) {
                *collisionAxisOut = 4;
                motionValue2 = motionValue;
            }
        }
    }
    if (motionValue >= 0.0f && motionValue < 1.0f) {
        return motionValue;
    }
    return -1.0f;
}

float CalculateSphereCollisionTime(Vec firstPosition, Vec firstVelocity, float angleStep, Vec secondPosition, Vec secondVelocity, float height)
{
    float motionValue;
    float motionValue2;
    float motionValue3;
    float motionValue4;
    float motionValue5;
    float motionValue6;
    float motionValue7;

    VECSubtract(&secondPosition, &firstPosition, &secondPosition);
    VECSubtract(&secondVelocity, &firstVelocity, &secondVelocity);
    if (VECMag(&secondPosition) > height + (angleStep + VECMag(&secondVelocity))) {
        return -1.0f;
    }
    motionValue6 = (angleStep + height) * (angleStep + height);
    motionValue3 = secondVelocity.z * secondVelocity.z + (secondVelocity.x * secondVelocity.x + (secondVelocity.y * secondVelocity.y));
    motionValue4 = 2.0f * (secondVelocity.z * secondPosition.z + (secondVelocity.x * secondPosition.x + secondVelocity.y * secondPosition.y));
    motionValue7 = (secondPosition.z * secondPosition.z + (secondPosition.x * secondPosition.x + secondPosition.y * secondPosition.y)) - motionValue6;
    if (0.0f == motionValue3) {
        return -1.0f;
    }
    motionValue = motionValue4 * motionValue4 - 4.0f * motionValue3 * motionValue7;
    if (motionValue <= 0.0f) {
        return -1.0f;
    }
    motionValue = sqrtf(motionValue);
    motionValue5 = (-motionValue4 + motionValue) / (2.0f * motionValue3);
    motionValue2 = (-motionValue4 - motionValue) / (2.0f * motionValue3);
    if (motionValue5 * motionValue2 < 0.0f && motionValue2 < 0.0f) {
        if (motionValue5 > fabs(motionValue2)) {
            return 0.0f;
        }
    }
    if (motionValue >= 0.0f && motionValue < 1.0f) {
        return motionValue;
    }
    return -1.0f;
}

float CalculateVerticalSphereCollisionTime(Vec firstPosition, Vec firstVelocity, float movingRadius, Vec secondPosition, Vec secondVelocity, float targetRadius, float verticalRadius)
{
    float temporaryValue;

    float motionValue;
    float motionValue2;
    float motionValue3;
    float motionValue4;
    float motionValue5;
    float motionValue6;
    float motionValue7;
    float motionValue8;

    VECSubtract(&firstPosition, &secondPosition, &firstPosition);
    VECSubtract(&firstVelocity, &secondVelocity, &firstVelocity);

    if (VECMag(&firstPosition) > (movingRadius) + VECMag(&firstVelocity) + targetRadius + verticalRadius) {
        return -1.0f;
    }
    movingRadius += targetRadius;
    motionValue4 = VECMagXZ(&firstPosition);

    motionValue5 = VECSumXZ(&firstPosition, &firstVelocity) - motionValue4;

    motionValue6 = (motionValue5 * motionValue5) + (firstVelocity.y * firstVelocity.y);
    motionValue7 = 2.0f * ((firstPosition.y * firstVelocity.y) + ((-verticalRadius * motionValue5) + (motionValue4 * motionValue5)));
    temporaryValue = (firstPosition.y * firstPosition.y + (((verticalRadius * verticalRadius) + (motionValue4 * motionValue4)) - (2.0f * verticalRadius * motionValue4))) - (movingRadius * movingRadius);
    if (0.0f == motionValue6) {
        return -1.0f;
    }
    motionValue = (motionValue7 * motionValue7) - (4.0f * motionValue6 * temporaryValue);
    if (motionValue < 0.0f) {
        return -1.0f;
    }

    motionValue = sqrtf(motionValue);
    motionValue8 = (-motionValue7 + motionValue) / (2.0f * motionValue6);
    motionValue3 = (-motionValue7 - motionValue) / (2.0f * motionValue6);

    if (motionValue8 * motionValue3 < 0.0f && motionValue3 < 0.0f) {
        if (motionValue8 > fabs(motionValue3)) {
            return 0.0f;
        }
    }
    if (motionValue >= 0.0f && motionValue < 1.0f) {
        return motionValue;
    }
    return -1.0f;
}

float CalculateHorizontalSphereCollisionTime(Vec firstPosition, Vec firstVelocity, float movingRadius, Vec secondPosition, float collisionRadius)
{
    float motionValue;
    float motionValue2;
    float motionValue3;
    float motionValue4;
    float motionValue5;
    float motionValue6;
    float motionValue7;

    VECSubtract(&firstPosition, &secondPosition, &firstPosition);
    firstPosition.y = 0.0f;
    firstVelocity.y = 0.0f;
    if (VECMag(&firstPosition) > movingRadius + VECMag(&firstVelocity) + collisionRadius) {
        return -1.0f;
    }
    motionValue6 = (movingRadius + collisionRadius) * (movingRadius + collisionRadius);
    motionValue3 = (firstVelocity.x * firstVelocity.x) + (firstVelocity.z * firstVelocity.z);
    motionValue4 = 2.0f * ((firstVelocity.x * firstPosition.x) + (firstVelocity.z * firstPosition.z));
    motionValue7 = ((firstPosition.x * firstPosition.x) + (firstPosition.z * firstPosition.z)) - motionValue6;
    if (0.0f == motionValue3) {
        return -1.0f;
    }
    motionValue = (motionValue4 * motionValue4) - (4.0f * motionValue3 * motionValue7);
    if (motionValue <= 0.0f) {
        return -1.0f;
    }
    motionValue = sqrtf(motionValue);
    motionValue5 = (-motionValue4 + motionValue) / (2.0f * motionValue3);
    motionValue2 = (-motionValue4 - motionValue) / (2.0f * motionValue3);
    if (motionValue5 * motionValue2 < 0.0f && motionValue2 < 0.0f) {
        if (motionValue5 > fabs(motionValue2)) {
            return 0.0f;
        }
    }
    if (motionValue >= 0.0f && motionValue < 1.0f) {
        return motionValue;
    }
    return -1.0f;
}

s32 TestSweptSphereCollision(Vec *firstPosition, Vec *firstVelocity, float movingRadius, Vec *secondPosition, Vec *secondVelocity, float targetRadius, float targetVerticalRadius)
{
    Vec temporaryPosition;
    Vec temporaryPosition2;
    Vec temporaryPosition3;
    float motionValue;

    VECSubtract(firstPosition, secondPosition, &temporaryPosition);
    VECSubtract(firstVelocity, secondVelocity, &temporaryPosition2);
    if (VECMag(&temporaryPosition) > movingRadius + VECMag(&temporaryPosition2) + targetRadius + targetVerticalRadius) {
        return 0;
    }
    if (temporaryPosition2.y < 0.0f) {
        motionValue = -temporaryPosition.y / temporaryPosition2.y;
        if (motionValue >= 0.0f && motionValue < 1.0f) {
            temporaryPosition3.x = motionValue * temporaryPosition2.x + temporaryPosition.x;
            temporaryPosition3.y = motionValue * temporaryPosition2.y + temporaryPosition.y;
            temporaryPosition3.z = motionValue * temporaryPosition2.z + temporaryPosition.z;
            if (VECMag(&temporaryPosition3) < targetVerticalRadius) {
                return 1;
            }
        }
    }
    return 0;
}

void ReflectVelocityAtBoundary(PlayerPhysicsWork *physicsWork, u8 collisionAxes)
{
    if (collisionAxes & 1) {
        physicsWork->velocity.x = -physicsWork->velocity.x;
    }
    if (collisionAxes & 2) {
        physicsWork->velocity.y = -physicsWork->velocity.y;
        if (fabs(physicsWork->velocity.y) < 4.0) {
            physicsWork->velocity.y = 0.0f;
        }
        if (fabs(physicsWork->velocity.y) > 10.0) {
            if (physicsWork->position.z < -400.0f) {
                HuAudFXPlay(1380);
            }
            else {
                HuAudFXPlay(1379);
            }
        }
    }
    if (collisionAxes & 4) {
        physicsWork->velocity.z = -physicsWork->velocity.z;
    }
    physicsWork->velocity.x *= 0.7f;
    physicsWork->velocity.y *= 0.7f;
    physicsWork->velocity.z *= 0.7f;
}

void CalculateSphereCollisionResponse(Vec firstPosition, Vec firstVelocity, Vec firstRadius, Vec *secondPosition, Vec *secondVelocity)
{
    Vec temporaryPosition;
    Vec temporaryPosition2;
    Vec temporaryPosition3;
    Vec temporaryPosition4;
    float motionValue;
    float motionValue2;
    float motionValue3;
    float motionValue4;

    temporaryPosition = firstVelocity;
    motionValue = VECMag(&temporaryPosition);
    if (motionValue < 0.001f) {
        secondPosition->x = secondPosition->y = secondPosition->z = 0.0f;
        secondVelocity->x = secondVelocity->y = secondVelocity->z = 0.0f;
        return;
    }
    VECNormalize(&temporaryPosition, &temporaryPosition);
    motionValue *= 0.9f;
    VECSubtract(&firstRadius, &firstPosition, &temporaryPosition2);
    VECNormalize(&temporaryPosition2, &temporaryPosition2);
    motionValue4 = VECDotProduct(&temporaryPosition, &temporaryPosition2);
    if (motionValue4 < 0.0f) {
        secondPosition->x = firstVelocity.x;
        secondPosition->y = firstVelocity.y;
        secondPosition->z = firstVelocity.z;
        secondVelocity->x = secondVelocity->y = secondVelocity->z = 0.0f;
        return;
    }
    motionValue3 = motionValue * motionValue4;
    motionValue2 = motionValue - motionValue3;
    if (motionValue2 < 0.001f) {
        secondPosition->x = secondPosition->y = secondPosition->z = 0.0f;
    }
    else {
        VECCrossProduct(&temporaryPosition, &temporaryPosition2, &temporaryPosition4);
        VECNormalize(&temporaryPosition4, &temporaryPosition4);
        VECCrossProduct(&temporaryPosition2, &temporaryPosition4, &temporaryPosition3);
        VECNormalize(&temporaryPosition3, &temporaryPosition3);
        secondPosition->x = temporaryPosition3.x * motionValue2;
        secondPosition->y = temporaryPosition3.y * motionValue2;
        secondPosition->z = temporaryPosition3.z * motionValue2;
    }
    secondVelocity->x = temporaryPosition2.x * motionValue3;
    secondVelocity->y = temporaryPosition2.y * motionValue3;
    secondVelocity->z = temporaryPosition2.z * motionValue3;
}

void ResolveSphereCollision(PlayerPhysicsWork *playerWork, PlayerPhysicsWork *otherPlayerWork)
{
    Vec temporaryPosition;
    Vec temporaryPosition2;
    float motionValue;
    float motionValue2;
    float motionValue3;

    playerWork->velocity.x *= 0.9f;
    playerWork->velocity.y *= 0.9f;
    playerWork->velocity.z *= 0.9f;
    VECSubtract(&playerWork->position, &otherPlayerWork->position, &temporaryPosition);
    VECNormalize(&temporaryPosition, &temporaryPosition2);
    motionValue2 = (playerWork->collisionRadius + otherPlayerWork->collisionRadius) - VECMag(&temporaryPosition);
    if (motionValue2 > 0.001f) {
        VECSubtract(&playerWork->velocity, &otherPlayerWork->velocity, &temporaryPosition);
        if (VECMag(&temporaryPosition) < 0.001f) {
            playerWork->position.x += temporaryPosition2.x * motionValue2;
            playerWork->position.y += temporaryPosition2.y * motionValue2;
            playerWork->position.z += temporaryPosition2.z * motionValue2;
        }
        else {
            VECNormalize(&temporaryPosition, &temporaryPosition);
            playerWork->position.x -= temporaryPosition.x * motionValue2;
            playerWork->position.y -= temporaryPosition.y * motionValue2;
            playerWork->position.z -= temporaryPosition.z * motionValue2;
        }
    }
    motionValue = ((-playerWork->velocity.x * temporaryPosition2.x) - (playerWork->velocity.y * temporaryPosition2.y)) - (playerWork->velocity.z * temporaryPosition2.z);
    if (motionValue > 0.0f) {
        playerWork->velocity.x += 2.0f * temporaryPosition2.x * motionValue;
        playerWork->velocity.y += 2.0f * temporaryPosition2.y * motionValue;
        playerWork->velocity.z += 2.0f * temporaryPosition2.z * motionValue;
    }
    VECSubtract(&otherPlayerWork->velocity, &playerWork->velocity, &temporaryPosition);
    motionValue3 = VECMag(&temporaryPosition);
    if (motionValue3 < 0.001f) {
        return;
    }
    VECNormalize(&temporaryPosition, &temporaryPosition);
    motionValue = VECDotProduct(&temporaryPosition2, &temporaryPosition);
    if (motionValue > 0.0f) {
        playerWork->velocity.x += temporaryPosition2.x * motionValue * motionValue3;
        playerWork->velocity.y += temporaryPosition2.y * motionValue * motionValue3;
        playerWork->velocity.z += temporaryPosition2.z * motionValue * motionValue3;
    }
}

void ResolvePlayerCollisions(PlayerPhysicsWork *firstPlayer, PlayerPhysicsWork *secondPlayer)
{
    Vec temporaryPosition;
    Vec temporaryPosition2;
    Vec temporaryPosition3;
    Vec temporaryPosition4;
    Vec temporaryPosition5;
    Vec temporaryPosition6;
    Vec temporaryPosition7;
    Vec temporaryPosition8;
    Vec temporaryPosition9;
    Vec temporaryPosition10;

    Vec *temporaryPosition;
    Vec *temporaryPosition2;
    Vec *temporaryPosition3;
    Vec *temporaryPosition4;
    Vec *temporaryPosition5;
    Vec *temporaryPosition6;

    if (firstPlayer->isConnectedToGoal || secondPlayer->isConnectedToGoal) {
        if (firstPlayer->isConnectedToGoal) {
            ResolveSphereCollision(secondPlayer, firstPlayer);
        }
        else {
            ResolveSphereCollision(firstPlayer, secondPlayer);
        }
        return;
    }
    CalculateSphereCollisionResponse(firstPlayer->position, firstPlayer->velocity, secondPlayer->position, &temporaryPosition2, &temporaryPosition);
    CalculateSphereCollisionResponse(secondPlayer->position, secondPlayer->velocity, firstPlayer->position, &temporaryPosition4, &temporaryPosition3);
    VECAdd(&temporaryPosition2, &temporaryPosition3, &firstPlayer->velocity);
    VECAdd(&temporaryPosition, &temporaryPosition4, &secondPlayer->velocity);
}

void ResolvePlayerCollisionVariant(PlayerPhysicsWork *firstPlayer, PlayerPhysicsWork *secondPlayer)
{
    Vec temporaryPosition;
    Vec temporaryPosition2;
    Vec temporaryPosition3;

    float motionValue;
    float motionValue2;

    firstPlayer->velocity.x *= 0.7f;
    firstPlayer->velocity.y *= 0.7f;
    firstPlayer->velocity.z *= 0.7f;
    VECSubtract(&firstPlayer->position, &secondPlayer->position, &temporaryPosition2);

    temporaryPosition2.y = 0.0f;
    VECNormalize(&temporaryPosition2, &temporaryPosition2);

    temporaryPosition.x = temporaryPosition2.x * secondPlayer->collisionHeight + secondPlayer->position.x;
    temporaryPosition.y = secondPlayer->position.y;
    temporaryPosition.z = temporaryPosition2.z * secondPlayer->collisionHeight + secondPlayer->position.z;

    VECSubtract(&firstPlayer->position, &temporaryPosition, &temporaryPosition2);
    VECNormalize(&temporaryPosition2, &temporaryPosition3);
    motionValue = (firstPlayer->collisionRadius + secondPlayer->collisionRadius) - VECMag(&temporaryPosition2);

    if (motionValue > 0.001f) {
        VECSubtract(&firstPlayer->velocity, &secondPlayer->velocity, &temporaryPosition2);
        if (VECMag(&temporaryPosition2) < 0.001f) {
            firstPlayer->position.x += temporaryPosition3.x * motionValue;
            firstPlayer->position.y += temporaryPosition3.y * motionValue;
            firstPlayer->position.z += temporaryPosition3.z * motionValue;
        }
        else {
            VECNormalize(&temporaryPosition2, &temporaryPosition2);
            firstPlayer->position.x -= temporaryPosition2.x * motionValue;
            firstPlayer->position.y -= temporaryPosition2.y * motionValue;
            firstPlayer->position.z -= temporaryPosition2.z * motionValue;
        }
    }
    motionValue2 = -firstPlayer->velocity.x * temporaryPosition3.x - firstPlayer->velocity.y * temporaryPosition3.y - firstPlayer->velocity.z * temporaryPosition3.z;
    if (motionValue2 > 0.0f) {
        firstPlayer->velocity.x += 2.0f * temporaryPosition3.x * motionValue2;
        firstPlayer->velocity.y += 2.0f * temporaryPosition3.y * motionValue2;
        firstPlayer->velocity.z += 2.0f * temporaryPosition3.z * motionValue2;
    }
    VECSubtract(&secondPlayer->velocity, &firstPlayer->velocity, &temporaryPosition2);
    motionValue = VECMag(&temporaryPosition2);
    if (!(motionValue < 0.001f)) {
        VECNormalize(&temporaryPosition2, &temporaryPosition2);
        motionValue2 = VECDotProduct(&temporaryPosition3, &temporaryPosition2);
        if (motionValue2 > 0.0f) {
            firstPlayer->velocity.x += temporaryPosition3.x * motionValue2 * motionValue;
            firstPlayer->velocity.y += temporaryPosition3.y * motionValue2 * motionValue;
            firstPlayer->velocity.z += temporaryPosition3.z * motionValue2 * motionValue;
        }
    }
}

void ResolveGoalCollision(PlayerPhysicsWork *playerWork, PlayerPhysicsWork *goalWork)
{
    Vec temporaryPosition;
    float motionValue;

    VECSubtract(&playerWork->position, (Vec *)&goalWork->previousPosition.z, &temporaryPosition);
    temporaryPosition.y = 0.0f;
    VECNormalize(&temporaryPosition, &temporaryPosition);
    motionValue = -playerWork->velocity.x * temporaryPosition.x - playerWork->velocity.z * temporaryPosition.z;
    playerWork->velocity.x += 2.0f * temporaryPosition.x * motionValue;
    playerWork->velocity.z += 2.0f * temporaryPosition.z * motionValue;
    playerWork->velocity.x *= 0.5f;
    playerWork->velocity.y *= 0.5f;
    playerWork->velocity.z *= 0.5f;
}

void CreatePlayerModelManager(OMOBJ *object)
{
    s32 loopIndex;
    PlayerPhysicsWork *physicsWork;
    s16 stateValue;

    object->stat |= 0x100;
    physicsWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerPhysicsWork) * 60, HU_MEMNUM_OVL);
    object->data = physicsWork;
    memset(physicsWork, 0, 4320);

    for (loopIndex = 0; loopIndex < 60; loopIndex++, physicsWork++) {
        physicsWork->sphereIndex = loopIndex;
        physicsWork->playerIndex = loopIndex / 15;
        if ((loopIndex % 15) == 0) {
            object->mdlId[loopIndex] = Hu3DModelCreateFile(characterModelDataIds[GWPlayerCfg[physicsWork->playerIndex].character]);
            stateValue = object->mdlId[loopIndex];
        }
        else {
            object->mdlId[loopIndex] = Hu3DModelLink(stateValue);
        }
        physicsWork->modelId = object->mdlId[loopIndex];
        Hu3DModelAttrSet(object->mdlId[loopIndex], HU3D_ATTR_DISPOFF);
        Hu3DModelShadowSet(object->mdlId[loopIndex]);
    }
    object->objFunc = UpdatePlayerModelManager;
}

void UpdatePlayerModelManager(OMOBJ *object)
{
    Mtx temporaryTransform;
    Vec temporaryPosition;
    float motionValue;
    float motionValue2;
    PlayerPhysicsWork *physicsWork;
    s32 loopIndex;
    HU3DMODEL *model;

    physicsWork = object->data;

    for (loopIndex = 0; loopIndex < 60; loopIndex++, physicsWork++) {
        if (!physicsWork->isActive) {
            Hu3DModelAttrSet(physicsWork->modelId, HU3D_ATTR_DISPOFF);
            continue;
        }
        if (physicsWork->isConnectedToGoal) {
            Hu3DModelObjPosGet(physicsWork->linkedPhysicsWork->modelId, "i_hook", &temporaryPosition);
            VECSubtract(&temporaryPosition, &physicsWork->position, &temporaryPosition);
            if (physicsWork->stateTimer != 0) {
                motionValue = cosd(90.0f * physicsWork->stateTimer / (REFRESH_RATE / 6));
                physicsWork->velocity.x = motionValue * temporaryPosition.x;
                physicsWork->velocity.y = motionValue * temporaryPosition.y;
                physicsWork->velocity.z = motionValue * temporaryPosition.z;
                physicsWork->stateTimer--;
            }
            else {
                physicsWork->velocity = temporaryPosition;
            }
        }
        else {
#if VERSION_PAL
            physicsWork->velocity.y += -1.372f;
#else
            physicsWork->velocity.y += -0.95277786f;
#endif
            if (physicsWork->isVerticalCollisionState && (getMinigameSubstate() != 4 || (getMinigameSubstate() == 4 && physicsWork->stateTimer < REFRESH_RATE / 4))) {
                if (--physicsWork->stateTimer == 0) {
                    physicsWork->isActive = 0;
                }
                if (physicsWork->stateTimer < REFRESH_RATE / 4) {
                    physicsWork->isVisible ^= 1;
                }
            }
            if (!physicsWork->collisionCooldown) {
                if (0.0f != physicsWork->velocity.x || 0.0f != physicsWork->velocity.z) {
                    temporaryPosition.x = physicsWork->velocity.z;
                    temporaryPosition.y = 0.0f;
                    temporaryPosition.z = -physicsWork->velocity.x;
                    motionValue2 = VECMag(&temporaryPosition) / 30.000002f;
                    if (motionValue2 >= 0.001f) {
                        MTXRotAxisRad(temporaryTransform, &temporaryPosition, motionValue2);
                        model = &Hu3DData[object->mdlId[loopIndex]];
                        MTXConcat(temporaryTransform, model->mtx, model->mtx);
                    }
                }
            }
            else {
                physicsWork->collisionCooldown--;
            }
        }
        if (physicsWork->isVisible) {
            Hu3DModelAttrReset(physicsWork->modelId, HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrSet(physicsWork->modelId, HU3D_ATTR_DISPOFF);
        }
        Hu3DModelPosSet(object->mdlId[loopIndex], physicsWork->position.x, physicsWork->position.y, physicsWork->position.z);
    }
    if (getMinigameSubstate() > 2 && isMinigameSequenceDone() == 1) {
        physicsWork = playerSphereObject->data;

        for (loopIndex = 0; loopIndex < 60; loopIndex++, physicsWork++) {
            if (physicsWork->isActive) {
                SetGoalParticlePosition(&physicsWork->position);
                Hu3DModelAttrSet(physicsWork->modelId, HU3D_ATTR_DISPOFF);
                physicsWork->isActive = 0;
            }
        }
        playerSphereObject->objFunc = NULL;
        Hu3DModelAttrReset(goalParticleModelId, HU3D_ATTR_DISPOFF);
        SetMinigameControlFlag(4);
    }
}

void HidePlayerModels(OMOBJ *object)
{
    HuMemDirectFree(object->data);
}

s32 goalModelDataIds[] = { DATA_MAKE_NUM(DATADIR_M410, 8), DATA_MAKE_NUM(DATADIR_M410, 9), DATA_MAKE_NUM(DATADIR_M410, 8) };

void CreateGoalObject(OMOBJ *object)
{
    PlayerPhysicsWork *physicsWork;
    s32 loopIndex;
    s16 stateValue;

    object->stat |= 0x100;
    physicsWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerPhysicsWork) * 18, HU_MEMNUM_OVL);
    object->data = physicsWork;
    memset(physicsWork, 0, sizeof(PlayerPhysicsWork) * 18);

    for (loopIndex = 0; loopIndex < 18; loopIndex++, physicsWork++) {
        physicsWork->playerIndex = loopIndex / 6;
        object->mdlId[loopIndex] = Hu3DModelCreateFile(goalModelDataIds[physicsWork->playerIndex]);
        stateValue = object->mdlId[loopIndex];
        (void)stateValue;
        Hu3DModelAttrSet(object->mdlId[loopIndex], HU3D_MOTATTR_PAUSE);
        Hu3DData[object->mdlId[loopIndex]].clusterAttr[0] = 0;
        physicsWork->modelId = object->mdlId[loopIndex];
        physicsWork->sphereIndex = loopIndex;
        physicsWork->position.x = (250.0f * (loopIndex % 6)) - 750.0f;

        if ((physicsWork->playerIndex & 1) == 0) {
            physicsWork->position.x += 125.0f;
        }
        physicsWork->position.y = 140.0f + (140.0f * physicsWork->playerIndex);
        physicsWork->position.z = -700.0f;
        physicsWork->collisionRadius = 1.0f;
        physicsWork->collisionHeight = 50.0f;
        Hu3DModelPosSet(object->mdlId[loopIndex], physicsWork->position.x, physicsWork->position.y, physicsWork->position.z);

        if (!loopIndex) {
            object->mdlId[loopIndex + 0x12] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M410, 10));
        }
        else {
            object->mdlId[loopIndex + 0x12] = Hu3DModelLink(object->mdlId[18]);
        }
        Hu3DModelShadowMapSet(object->mdlId[loopIndex + 18]);
        Hu3DModelPosSet(object->mdlId[loopIndex + 18], physicsWork->position.x, physicsWork->position.y, physicsWork->position.z);
        if (!loopIndex) {
            object->mdlId[loopIndex + 36] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M410, 11));
        }
        else {
            object->mdlId[loopIndex + 36] = Hu3DModelLink(object->mdlId[36]);
        }
        Hu3DModelPosSet(object->mdlId[loopIndex + 0x24], 20.0f + physicsWork->position.x, -10.0f + physicsWork->position.y, physicsWork->position.z);
    }
    object->objFunc = UpdateGoalObject;
}

void UpdateGoalObject(OMOBJ *object)
{
    float motionValue;
    PlayerPhysicsWork *physicsWork;
    s32 loopIndex;

    physicsWork = object->data;

    for (loopIndex = 0; loopIndex < 18; loopIndex++, physicsWork++) {
        if (physicsWork->playerIndex & 1) {
            motionValue = currentGoalMotion;
        }
        else {
            motionValue = -currentGoalMotion;
        }
        physicsWork->velocity.x = motionValue;
        if (physicsWork->goalCollisionPending) {
            Hu3DModelAttrReset(object->mdlId[loopIndex], HU3D_MOTATTR_PAUSE);
            Hu3DMotionSpeedSet(object->mdlId[loopIndex], 4.0f);
            Hu3DMotionSet(object->mdlId[loopIndex], 0);
            Hu3DMotionClusterNoSet(object->mdlId[loopIndex], Hu3DData[object->mdlId[loopIndex]].motIdCluster[0], 0);
            Hu3DData[object->mdlId[loopIndex]].clusterSpeed[0] = 4.0f;
            physicsWork->goalCollisionPending = 0;
        }
        if (physicsWork->velocity.x < 0.0f) {
            if (physicsWork->position.x < -750.0f) {
                physicsWork->position.x += 1500.0f;
                physicsWork->wrappedAroundStage = 1;
            }
        }
        else if (physicsWork->position.x > 750.0f) {
            physicsWork->position.x -= 1500.0f;
            physicsWork->wrappedAroundStage = 1;
        }
        if (physicsWork->wrappedAroundStage) {
            if (physicsWork->linkedPhysicsWork != NULL) {
                physicsWork->linkedPhysicsWork->isActive = 0;
                physicsWork->linkedPhysicsWork = NULL;
                Hu3DMotionClusterNoSet(object->mdlId[loopIndex], Hu3DData[object->mdlId[loopIndex]].motIdCluster[0], 0);
                Hu3DData[object->mdlId[loopIndex]].clusterSpeed[0] = 0.0f;
            }
            physicsWork->wrappedAroundStage = 0;
        }
        Hu3DModelPosSet(object->mdlId[loopIndex], physicsWork->position.x, physicsWork->position.y, physicsWork->position.z);
        Hu3DModelPosSet(object->mdlId[loopIndex + 18], physicsWork->position.x, physicsWork->position.y, physicsWork->position.z);
        Hu3DModelPosSet(object->mdlId[loopIndex + 36], 20.0f + physicsWork->position.x, -10.0f + physicsWork->position.y, physicsWork->position.z);
    }
}

void HideGoalObject(OMOBJ *object)
{
    HuMemDirectFree(object->data);
}

void CreateGoalParticleEffect(void)
{
    ANIMDATA *stateValue;

    stateValue = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 2), HU_MEMNUM_OVL));
    goalParticleModelId = Hu3DParticleCreate(stateValue, 60);
    Hu3DParticleHookSet(goalParticleModelId, UpdateGoalParticle);
    Hu3DModelAttrSet(goalParticleModelId, HU3D_ATTR_DISPOFF);
}

void SetGoalParticlePosition(Vec *position)
{
    HU3DPARTICLE *stateValue;
    HU3DPARTICLEDATA *stateValue2;

    stateValue = Hu3DData[goalParticleModelId].hookData;
    stateValue2 = &stateValue->data[stateValue->emitCnt++];
    stateValue2->scale = 100.0f;
    stateValue2->pos.x = position->x;
    stateValue2->pos.y = position->y;
    stateValue2->pos.z = position->z;
}

s32 endTransitionTimer = REFRESH_RATE;

void UpdateGoalParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *stateValue;
    s32 loopIndex;
    s32 stateValue;

    if (!particle->dataCnt) {
        particle->dataCnt = 1;
        stateValue = &particle->data[particle->emitCnt];
        for (loopIndex = particle->emitCnt; loopIndex < particle->maxCnt; loopIndex++, stateValue++) {
            stateValue->scale = 0.0f;
        }
    }
    stateValue = (endTransitionTimer * 0xFF) / REFRESH_RATE;
    stateValue = particle->data;

    for (loopIndex = 0; loopIndex < particle->emitCnt; loopIndex++, stateValue++) {
        stateValue->color.a = stateValue; // 43?
    }
    if (--endTransitionTimer == 0) {
        model->attr |= 1;
    }
    DCStoreRange(particle->data, particle->maxCnt * 0x44);
}

s32 SetPlayerSphereState(Vec position, Vec velocity, u16 playerIndex)
{
    PlayerPhysicsWork *physicsWork = playerSphereObject->data;
    s32 loopIndex;

    physicsWork = &physicsWork[playerIndex * 15];
    for (loopIndex = 0; loopIndex < 15; loopIndex++, physicsWork++) {
        if (!physicsWork->isActive) {
            *(s16 *)physicsWork = 0;
            physicsWork->isActive = 1;
            physicsWork->isVisible = 1;
            physicsWork->position = position;
            physicsWork->velocity = velocity;
            physicsWork->collisionRadius = 30.000002f;
            physicsWork->linkedPhysicsWork = NULL;
            physicsWork->stateTimer = 0;
            return 1;
        }
    }
    return 0;
}

void ReportGoalScore(u16 index1, u16 index2, u16 index3)
{
    OSReport("GOAL!!(%d , %d , %d)\n", index1, index2, index3);
}

void SetGoalCallback(void *callback)
{
    goalCallback = callback;
}

void GetPlayerPosition(u16 playerIndex, Vec *positionOut)
{
    PlayerPhysicsWork *physicsWork;

    physicsWork = &((PlayerPhysicsWork *)goalObject->data)[playerIndex];
    positionOut->x = physicsWork->position.x;
    positionOut->y = physicsWork->position.y;
    positionOut->z = physicsWork->position.z;
}

void GetPlayerVelocity(u16 playerIndex, Vec *velocityOut)
{
    PlayerPhysicsWork *physicsWork;

    physicsWork = &((PlayerPhysicsWork *)goalObject->data)[playerIndex];
    velocityOut->x = physicsWork->velocity.x;
    velocityOut->y = physicsWork->velocity.y;
    velocityOut->z = physicsWork->velocity.z;
}

void GetPlayerFuturePosition(u16 playerIndex, u16 framesAhead, Vec *positionOut)
{
    Vec temporaryPosition;
    PlayerPhysicsWork *physicsWork;

    physicsWork = &((PlayerPhysicsWork *)goalObject->data)[playerIndex];
    temporaryPosition.x = physicsWork->position.x + physicsWork->velocity.x * framesAhead;
    temporaryPosition.y = physicsWork->position.y + physicsWork->velocity.y * framesAhead;
    temporaryPosition.z = physicsWork->position.z + physicsWork->velocity.z * framesAhead;
    if (physicsWork->velocity.x < 0.0f) {
        while (temporaryPosition.x < -750.0f) {
            temporaryPosition.x += 1500.0f;
        }
    }
    else {
        while (temporaryPosition.x > 750.0f) {
            temporaryPosition.x -= 1500.0f;
        }
    }
    positionOut->x = temporaryPosition.x;
    positionOut->y = temporaryPosition.y;
    positionOut->z = temporaryPosition.z;
}

s32 FindModelJointMatrix(s16 modelId, char *jointName, Mtx transformOut)
{
    Mtx temporaryTransform;
    Mtx temporaryTransform2;
    HSFDATA *stateValue;
    HU3DMODEL *model;

    model = &Hu3DData[modelId];
    stateValue = model->hsf;
    jointSearchSucceeded = 0;
    mtxRot(temporaryTransform2, model->rot.x, model->rot.y, model->rot.z);
    MTXScale(temporaryTransform, model->scale.x, model->scale.y, model->scale.z);
    MTXConcat(temporaryTransform2, temporaryTransform, temporaryTransform);
    mtxTransCat(temporaryTransform, model->pos.x, model->pos.y, model->pos.z);
    SearchJointHierarchy(stateValue->root, temporaryTransform, jointName, transformOut);
    return jointSearchSucceeded;
}

void SearchJointHierarchy(HSFOBJECT *joint, Mtx parentTransform, char *jointName, Mtx transformOut)
{
    Mtx temporaryTransform;
    Mtx temporaryTransform2;
    Mtx temporaryTransform3;
    Mtx temporaryTransform4;
    Mtx temporaryTransform5;
    Mtx temporaryTransform6;
    Mtx temporaryTransform7;
    Mtx temporaryTransform8;

    HSFTRANSFORM *stateValue;
    HSFTRANSFORM *stateValue2;
    HSFTRANSFORM *stateValue3;
    HSFTRANSFORM *stateValue4;
    HSFOBJECT *joint2;
    HSFOBJECT *joint3;
    u32 loopIndex;
    u32 loopIndex2;
    u32 loopIndex3;
    u32 loopIndex4;

    if (jointSearchSucceeded) {
        return;
    }
    stateValue = &joint->mesh.curr;

    mtxRot(temporaryTransform2, stateValue->rot.x, stateValue->rot.y, stateValue->rot.z);
    MTXScale(temporaryTransform, stateValue->scale.x, stateValue->scale.y, stateValue->scale.z);
    MTXConcat(temporaryTransform2, temporaryTransform, temporaryTransform);
    mtxTransCat(temporaryTransform, stateValue->pos.x, stateValue->pos.y, stateValue->pos.z);
    MTXConcat(parentTransform, temporaryTransform, temporaryTransform);
    if (!strcmp(jointName, joint->name)) {
        MTXCopy(temporaryTransform, transformOut);
        jointSearchSucceeded = 1;
    }

    for (loopIndex = 0; loopIndex < joint->mesh.childrenCount; loopIndex++) {
        joint = joint->mesh.children[loopIndex];
        if (!jointSearchSucceeded) {
            stateValue2 = &joint->mesh.curr;
            mtxRot(temporaryTransform3, stateValue2->rot.x, stateValue2->rot.y, stateValue2->rot.z);
            MTXScale(temporaryTransform4, stateValue2->scale.x, stateValue2->scale.y, stateValue2->scale.z);
            MTXConcat(temporaryTransform3, temporaryTransform4, temporaryTransform4);
            mtxTransCat(temporaryTransform4, stateValue2->pos.x, stateValue2->pos.y, stateValue2->pos.z);
            MTXConcat(temporaryTransform, temporaryTransform4, temporaryTransform4);
            if (!strcmp(jointName, joint->name)) {
                MTXCopy(temporaryTransform4, transformOut);
                jointSearchSucceeded = 1;
            }
            for (loopIndex2 = 0; loopIndex2 < joint->mesh.childrenCount; loopIndex2++) {
                joint2 = joint->mesh.children[loopIndex2];
                if (!jointSearchSucceeded) {
                    stateValue3 = &joint2->mesh.curr;
                    mtxRot(temporaryTransform5, stateValue3->rot.x, stateValue3->rot.y, stateValue3->rot.z);
                    MTXScale(temporaryTransform6, stateValue3->scale.x, stateValue3->scale.y, stateValue3->scale.z);
                    MTXConcat(temporaryTransform5, temporaryTransform6, temporaryTransform6);
                    mtxTransCat(temporaryTransform6, stateValue3->pos.x, stateValue3->pos.y, stateValue3->pos.z);
                    MTXConcat(temporaryTransform4, temporaryTransform6, temporaryTransform6);
                    if (!strcmp(jointName, joint2->name)) {
                        MTXCopy(temporaryTransform6, transformOut);
                        jointSearchSucceeded = 1;
                    }

                    for (loopIndex3 = 0; loopIndex3 < joint2->mesh.childrenCount; loopIndex3++) {
                        joint3 = joint2->mesh.children[loopIndex3];
                        if (!jointSearchSucceeded) {
                            stateValue4 = &joint3->mesh.curr;
                            mtxRot(temporaryTransform7, stateValue4->rot.x, stateValue4->rot.y, stateValue4->rot.z);
                            MTXScale(temporaryTransform8, stateValue4->scale.x, stateValue4->scale.y, stateValue4->scale.z);
                            MTXConcat(temporaryTransform7, temporaryTransform8, temporaryTransform8);
                            mtxTransCat(temporaryTransform8, stateValue4->pos.x, stateValue4->pos.y, stateValue4->pos.z);
                            MTXConcat(temporaryTransform6, temporaryTransform8, temporaryTransform8);
                            if (!strcmp(jointName, joint3->name)) {
                                MTXCopy(temporaryTransform8, transformOut);
                                jointSearchSucceeded = 1;
                            }

                            for (loopIndex4 = 0; loopIndex4 < joint3->mesh.childrenCount; loopIndex4++) {
                                SearchJointHierarchy(joint3->mesh.children[loopIndex4], temporaryTransform8, jointName, transformOut);
                            }
                        }
                    }
                }
            }
        }
    }
}
