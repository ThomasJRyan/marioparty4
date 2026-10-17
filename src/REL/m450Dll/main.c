#include "REL/executor.h"
#include "datadir_enum.h"
#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXStruct.h"
#include "dolphin/gx/GXVert.h"
#include "dolphin/mtx/GeoTypes.h"
#include "dolphin/pad.h"
#include "dolphin/types.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/wipe.h"

#include "game/gamework.h"
#include "game/gamework_data.h"

#include "game/mapspace.h"
#include "game/pad.h"
#include "game/printfunc.h"

#include "game/hu3d.h"

#include "game/esprite.h"
#include "game/sprite.h"

#include "ext_math.h"

#include "game/window.h"
#include "string.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/hsfex.h"

void HuSysVWaitSet(s16 vcount);
#endif

#undef ABS
#define ABS(x) ((0 > (x)) ? -(x) : (x))

typedef struct RegisteredModelNode {
    /* 0x00 */ u16 resourceKey;
    /* 0x02 */ s16 modelId;
    /* 0x04 */ struct RegisteredModelNode *previous;
    /* 0x08 */ struct RegisteredModelNode *next;
} RegisteredModelNode; // Size 0xC

typedef struct ParticleGroupNode {
    /* 0x00 */ HU3DPARMANPARAM *particleParameters;
    /* 0x04 */ s32 *particleIds;
    /* 0x08 */ s32 lifetime;
    /* 0x0C */ s32 particleCount;
    /* 0x10 */ struct ParticleGroupNode *previous;
    /* 0x14 */ struct ParticleGroupNode *next;
} ParticleGroupNode; // Size 0x18

typedef struct {
    /* 0x00 */ u32 resourceId;
    /* 0x04 */ s16 modelId;
} CachedModelResource; // Size 8

RegisteredModelNode registeredModelList;
ParticleGroupNode lbl1Bss340;
HUPROCESS *lbl1Bss33C;
Vec lbl1Bss330;
Vec lbl1Bss324;
Vec lbl1Bss318;
Vec lbl1Bss30C;
u32 lbl1Bss308;
s32 lbl1Bss304;
u32 lbl1Bss300;
s16 lbl1Bss2FE;
s16 lbl1Bss2FC;
s32 lbl1Bss2F8;
s32 lbl1Bss2F4;
float lbl1Bss2F0;
s32 lbl1Bss2B0[16];
CachedModelResource lbl1Bss1B0[32];

void RegisteredModelListInit(void)
{
    registeredModelList.previous = NULL;
    registeredModelList.next = NULL;
}

void RegisteredModelAdd(u16 resourceKey, s16 modelId)
{
    RegisteredModelNode *newModelNode;
    RegisteredModelNode *listHead;
    RegisteredModelNode *previousFirstNode;

    newModelNode = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*newModelNode), HU_MEMNUM_OVL);
    newModelNode->resourceKey = resourceKey;
    newModelNode->modelId = modelId;
    listHead = &registeredModelList;
    previousFirstNode = listHead->next;
    listHead->next = newModelNode;
    if (NULL != previousFirstNode) {
        previousFirstNode->previous = newModelNode;
    }
    newModelNode->previous = listHead;
    newModelNode->next = previousFirstNode;
}

s16 RegisteredModelRemoveByResourceKey(s16 resourceKey)
{
    RegisteredModelNode *modelNode;
    RegisteredModelNode *previousNode;
    RegisteredModelNode *nextNode;
    s16 removedModelId;

    modelNode = registeredModelList.next;
    while (NULL != modelNode) {
        if (resourceKey == modelNode->resourceKey) {
            removedModelId = modelNode->modelId;
            previousNode = modelNode->previous;
            nextNode = modelNode->next;
            previousNode->next = nextNode;
            if (NULL != nextNode) {
                nextNode->previous = previousNode;
            }
            HuMemDirectFree(modelNode);
            return removedModelId;
        }
        modelNode = modelNode->next;
    }
    return -1;
}

void ParticleGroupListInit(void)
{
    lbl1Bss340.previous = NULL;
    lbl1Bss340.next = NULL;
}

void ParticleGroupCreate(s16 particleManagerId, HU3DPARMANPARAM *particleParameters, s32 renderLayer, s32 lifetime, s32 particleCount, Vec *particlePositions, u8 blendMode)
{
    ParticleGroupNode *firstParticleGroup;
    ParticleGroupNode *newParticleGroup;
    ParticleGroupNode *listHead;
    s32 particleIndex;

    newParticleGroup = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*newParticleGroup), HU_MEMNUM_OVL);
    newParticleGroup->particleParameters = particleParameters;
    newParticleGroup->lifetime = lifetime;
    newParticleGroup->particleCount = particleCount;
    newParticleGroup->particleIds = HuMemDirectMallocNum(HEAP_HEAP, particleCount * sizeof(*newParticleGroup->particleIds), HU_MEMNUM_OVL);
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particlePositions++) {
        newParticleGroup->particleIds[particleIndex] = Hu3DParManLink(particleManagerId, particleParameters);
        if (newParticleGroup->particleIds[particleIndex] >= 0) {
            Hu3DParManAttrReset(newParticleGroup->particleIds[particleIndex], 1);
            Hu3DParManPosSet(newParticleGroup->particleIds[particleIndex], particlePositions->x, particlePositions->y, particlePositions->z);
            Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(newParticleGroup->particleIds[particleIndex]), blendMode);
            Hu3DModelLayerSet(Hu3DParManModelIDGet(newParticleGroup->particleIds[particleIndex]), renderLayer);
        }
    }
    listHead = &lbl1Bss340;
    firstParticleGroup = listHead->next;
    listHead->next = newParticleGroup;
    if (NULL != firstParticleGroup) {
        firstParticleGroup->previous = newParticleGroup;
    }
    newParticleGroup->previous = listHead;
    newParticleGroup->next = firstParticleGroup;
}

void ParticleGroupUpdate(void)
{
    ParticleGroupNode *particleGroup;
    ParticleGroupNode *previousGroup;
    ParticleGroupNode *nextGroup;
    s32 particleIndex;

    particleGroup = lbl1Bss340.next;
    while (NULL != particleGroup) {
        particleGroup->lifetime--;
        if (particleGroup->lifetime == 0) {
            for (particleIndex = 0; particleIndex < particleGroup->particleCount; particleIndex++) {
                if (particleGroup->particleIds[particleIndex] >= 0) {
                    Hu3DParManAttrSet(particleGroup->particleIds[particleIndex], 1);
                }
            }
        }
        previousGroup = particleGroup->previous;
        nextGroup = particleGroup->next;
        if (-particleGroup->particleParameters->maxTime > particleGroup->lifetime) {
            for (particleIndex = 0; particleIndex < particleGroup->particleCount; particleIndex++) {
                if (particleGroup->particleIds[particleIndex] >= 0) {
                    Hu3DParManKill(particleGroup->particleIds[particleIndex]);
                }
            }
            previousGroup->next = nextGroup;
            if (NULL != nextGroup) {
                nextGroup->previous = previousGroup;
            }
            HuMemDirectFree(particleGroup->particleIds);
            HuMemDirectFree(particleGroup);
        }
        particleGroup = nextGroup;
    }
}

void ModelResourceCacheInit(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        lbl1Bss1B0[i].resourceId = 0;
        lbl1Bss1B0[i].modelId = -1;
    }
}

s16 ModelResourceGetOrCreate(s32 resourceId)
{
    s32 cacheIndex;

    for (cacheIndex = 0; cacheIndex < 32; cacheIndex++) {
        if (resourceId == lbl1Bss1B0[cacheIndex].resourceId) {
            return Hu3DModelLink(lbl1Bss1B0[cacheIndex].modelId);
        }
    }
    for (cacheIndex = 0; cacheIndex < 32; cacheIndex++) {
        if (lbl1Bss1B0[cacheIndex].resourceId == 0) {
            lbl1Bss1B0[cacheIndex].resourceId = resourceId;
            lbl1Bss1B0[cacheIndex].modelId = Hu3DModelCreateFile(resourceId);
            return lbl1Bss1B0[cacheIndex].modelId;
        }
    }
    return Hu3DModelCreateFile(resourceId);
}

void TimerDigitBanksSet(s32 *spriteBankIds, s32 firstDigitIndex, s32 elapsedFrames)
{
    s32 digitValue;

    espBankSet(spriteBankIds[firstDigitIndex + 1], 10);
    espBankSet(spriteBankIds[firstDigitIndex + 4], 11);
    if (600 * REFRESH_RATE <= elapsedFrames) {
        elapsedFrames = 600 * REFRESH_RATE - 1;
    }
    digitValue = elapsedFrames / (60 * REFRESH_RATE);
    elapsedFrames -= digitValue * (60 * REFRESH_RATE);
    if (digitValue > 9) {
        digitValue = 9;
    }
    espBankSet(spriteBankIds[firstDigitIndex], digitValue);
    digitValue = elapsedFrames / REFRESH_RATE;
    elapsedFrames -= digitValue * REFRESH_RATE;
    espBankSet(spriteBankIds[firstDigitIndex + 2], digitValue / 10);
    espBankSet(spriteBankIds[firstDigitIndex + 3], digitValue % 10);
    digitValue = 101.5f * elapsedFrames / REFRESH_RATE;
    espBankSet(spriteBankIds[firstDigitIndex + 5], digitValue / 10);
    espBankSet(spriteBankIds[firstDigitIndex + 6], digitValue % 10);
}

void TimerRecordSet(s32 recordIndex)
{
    lbl1Bss2F0 = 0.0f;
    lbl1Bss300 = lbl1Bss304;
    GWMGRecordSet(recordIndex, lbl1Bss300);
    TimerDigitBanksSet(lbl1Bss2B0, 7, lbl1Bss304);
}

void TimerSpritesShow(void)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        espDispOn(lbl1Bss2B0[i]);
    }
}

void TimerSpritesPulse(void)
{
    float pulseScale;
    s32 spriteIndex;

    if (0.0f <= lbl1Bss2F0) {
        lbl1Bss2F0 += 16.0f;
        pulseScale = 1.0 + 0.2f * sind(lbl1Bss2F0);
        for (spriteIndex = 7; spriteIndex <= 14; spriteIndex++) {
            espScaleSet(lbl1Bss2B0[spriteIndex], pulseScale, pulseScale);
        }
    }
}

void TimerDisplayCreate(float verticalPosition, s32 timerSpriteId)
{
    float spriteX;
    float spriteY;
    s32 spriteIndex;

    spriteX = 288.0f;
    spriteY = verticalPosition;
    lbl1Bss2B0[15] = espEntry(timerSpriteId, 0, 0);
    espTPLvlSet(lbl1Bss2B0[15], 0.5f);
    espColorSet(lbl1Bss2B0[15], 0, 0, 0);
    espPosSet(lbl1Bss2B0[15], spriteX, spriteY);
    spriteX = 248.0f;
    spriteY = verticalPosition - 8.0f;
    lbl1Bss2B0[14] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 55), 0, 0);
    espPosSet(lbl1Bss2B0[14], spriteX - 18.0f, spriteY);
    for (spriteIndex = 7; spriteIndex <= 13; spriteIndex++) {
        lbl1Bss2B0[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 48), 0, 0);
        espPosSet(lbl1Bss2B0[spriteIndex], spriteX, spriteY);
        spriteX += 16.0f;
    }
    TimerDigitBanksSet(lbl1Bss2B0, 7, lbl1Bss300);
    spriteX = 248.0f;
    spriteY = verticalPosition + 8.0f;
    for (spriteIndex = 0; spriteIndex <= 6; spriteIndex++) {
        lbl1Bss2B0[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 49), 0, 0);
        espPosSet(lbl1Bss2B0[spriteIndex], spriteX, spriteY);
        spriteX += 16.0f;
    }
    TimerDigitBanksSet(lbl1Bss2B0, 0, 0);
    for (spriteIndex = 0; spriteIndex < 16; spriteIndex++) {
        espDrawNoSet(lbl1Bss2B0[spriteIndex], 0);
        espDispOff(lbl1Bss2B0[spriteIndex]);
        espAttrSet(lbl1Bss2B0[spriteIndex], HUSPR_ATTR_NOANIM);
        espPriSet(lbl1Bss2B0[spriteIndex], 0x80 + spriteIndex);
    }
}

void VectorEaseInterpolate(float progress, Vec *startPosition, Vec *endPosition, Vec *interpolatedPosition)
{
    float easedProgress;

    if (1.0f < progress) {
        progress = 1.0f;
    }
    easedProgress = sind(90.0f * progress) * sind(90.0f * progress);
    interpolatedPosition->x = startPosition->x + easedProgress * (endPosition->x - startPosition->x);
    interpolatedPosition->y = startPosition->y + easedProgress * (endPosition->y - startPosition->y);
    interpolatedPosition->z = startPosition->z + easedProgress * (endPosition->z - startPosition->z);
}

void CameraPoseSet(Vec *cameraPosition, Vec *cameraTarget, Vec *cameraUp)
{
    Hu3DCameraPosSetV(1, cameraPosition, cameraTarget, cameraUp);
    lbl1Bss330 = *cameraPosition;
    lbl1Bss318 = *cameraUp;
}

s32 CameraPoseInterpolate(float progress, Vec *cameraTarget)
{
    Vec interpolatedCameraPosition;
    Vec interpolatedCameraUp;

    VectorEaseInterpolate(progress, &lbl1Bss330, &lbl1Bss324, &interpolatedCameraPosition);
    VectorEaseInterpolate(progress, &lbl1Bss318, &lbl1Bss30C, &interpolatedCameraUp);
    Hu3DCameraPosSetV(1, &interpolatedCameraPosition, cameraTarget, &interpolatedCameraUp);
    if (1.0f <= progress) {
        return 1;
    }
    else {
        lbl1Bss2F4 = 1;
        return 0;
    }
}

void CameraLookAtTransition(float progress, Vec *cameraTarget)
{
    Vec interpolatedCameraPosition;
    Vec interpolatedCameraUp;

    VectorEaseInterpolate(progress, &lbl1Bss330, &lbl1Bss324, &interpolatedCameraPosition);
    VectorEaseInterpolate(progress, &lbl1Bss318, &lbl1Bss30C, &interpolatedCameraUp);
    CameraPoseSet(&interpolatedCameraPosition, cameraTarget, &interpolatedCameraUp);
    lbl1Bss2F4 = 1;
}

void WorldPositionToScreenPosition(Vec *worldPosition, s16 cameraIndex, Vec *screenPosition)
{
    Mtx cameraViewMatrix;
    Vec cameraSpacePosition;
    float perspectiveWidth;
    float perspectiveHeight;
    float viewportLeft;
    float viewportWidth;
    HU3DCAMERA *camera;
    s32 cameraIndexBit;

    for (cameraIndexBit = 0; cameraIndexBit < 16; cameraIndexBit++) {
        if (cameraIndex & (1 << cameraIndexBit)) {
            break;
        }
    }
    camera = &Hu3DCamera[cameraIndexBit];
    MTXLookAt(cameraViewMatrix, &camera->pos, &camera->up, &camera->target);
    MTXMultVec(cameraViewMatrix, worldPosition, &cameraSpacePosition);
    perspectiveWidth = cameraSpacePosition.z * (sind(camera->fov / 2) / cosd(camera->fov / 2)) * camera->aspect;
    perspectiveHeight = cameraSpacePosition.z * (sind(camera->fov / 2) / cosd(camera->fov / 2));
    viewportLeft = 0.9f * camera->viewportX;
    viewportWidth = 0.9f * camera->viewportW;
    screenPosition->x = viewportWidth / 2 + cameraSpacePosition.x * ((viewportWidth / 2) / -perspectiveWidth) + viewportLeft;
    screenPosition->y = camera->viewportH / 2 + cameraSpacePosition.y * ((camera->viewportH / 2) / perspectiveHeight) + camera->viewportY;
    screenPosition->z = 0.0f;
}

void ModelObjectPositionGet(s16 modelId, char *objectName, Vec *position)
{
    Mtx objectMatrix;

    Hu3DModelObjMtxGet(modelId, objectName, objectMatrix);
    position->x = objectMatrix[0][3];
    position->y = objectMatrix[1][3];
    position->z = objectMatrix[2][3];
}

void ModelObjectWorldPositionGet(s16 modelId, char *objectName, Vec *worldPosition, Mtx worldMatrix)
{
    Mtx worldObjectMatrix;
    HU3DMODEL *model;

    model = &Hu3DData[modelId];
    Hu3DModelObjMtxGet(modelId, objectName, worldObjectMatrix);
    MTXConcat(model->mtx, worldObjectMatrix, worldObjectMatrix);
    if (NULL != worldMatrix) {
        MTXCopy(worldObjectMatrix, worldMatrix);
    }
    worldPosition->x = worldObjectMatrix[0][3];
    worldPosition->y = worldObjectMatrix[1][3];
    worldPosition->z = worldObjectMatrix[2][3];
}

float AngleInterpolate(float startAngle, float targetAngle, float progress)
{
    float shortestAngleDelta;
    float interpolatedAngle;

    shortestAngleDelta = fmod(targetAngle - startAngle, 360.0);
    if (0.0f > shortestAngleDelta) {
        shortestAngleDelta += 360.0f;
    }
    if (180.0f < shortestAngleDelta) {
        shortestAngleDelta -= 360.0f;
    }
    interpolatedAngle = fmod(startAngle + shortestAngleDelta * progress, 360.0);
    if (0.0f > interpolatedAngle) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

void AudioSequencePlay(s32 sequenceId)
{
    lbl1Bss2F8 = HuAudSeqPlay((s32)sequenceId);
}

void AudioSequenceFadeOut(s32 fadeDuration)
{
    if (lbl1Bss2F8 >= 0) {
        if (fadeDuration < 0) {
            fadeDuration = 1000;
        }
        HuAudSeqFadeOut(lbl1Bss2F8, fadeDuration);
    }
    lbl1Bss2F8 = -1;
}

void VectorTransform(Vec *movementVector, float *headingAngle, s32 stickX, s32 stickY)
{
    Vec stickMovement;
    float stickMagnitude;
    s32 movementIntensity;

    stickMovement.x = 0.0f;
    stickMovement.y = 0.0f;
    stickMovement.z = 0.0f;
    stickMovement.x = stickX / 4.0f;
    stickMovement.z = -(float)stickY / 4.0f;
    movementIntensity = 0;
    stickMagnitude = VECMagXZ(&stickMovement);
    if (0.5f <= stickMagnitude) {
        if (10.0f <= stickMagnitude) {
            stickMovement.x /= stickMagnitude;
            stickMovement.z /= stickMagnitude;
            stickMagnitude = 10.0f;
            stickMovement.x = 10.0f * stickMovement.x;
            stickMovement.z = 10.0f * stickMovement.z;
        }
        *headingAngle = AngleInterpolate(*headingAngle, atan2d(stickMovement.x, stickMovement.z), 0.4f);
        if (8.0f <= stickMagnitude) {
            movementIntensity = 2;
        }
        else {
            movementIntensity = 1;
        }
    }
    VECAdd((Vec *)&movementVector, &stickMovement, (Vec *)&movementVector);
}

typedef struct ArenaModelLayout {
    /* 0x00 */ char **modelNames;
    /* 0x04 */ float modelRadius;
    /* 0x08 */ s32 modelCount;
    /* 0x0C */ s32 activeModelCount;
    /* 0x10 */ s32 selectedModelIndex;
    /* 0x14 */ s32 resetModels;
    /* 0x18 */ s32 modelsVisible;
    /* 0x1C */ s32 layoutComplete;
    /* 0x20 */ s32 firstModelIndex;
    /* 0x24 */ s32 *modelIds;
    /* 0x28 */ s32 *objectIds;
    /* 0x2C */ s32 *modelLinkIds;
    /* 0x30 */ Vec *modelPositions;
    /* 0x34 */ Vec *modelVelocities;
} ArenaModelLayout; /* size = 0x38 */

typedef struct ArenaObjectAnimationState {
    /* 0x00 */ char *objectName;
    /* 0x04 */ Vec position;
    /* 0x10 */ float previousHeight;
    /* 0x14 */ float animationTimer;
    /* 0x18 */ s32 isVisible;
    /* 0x1C */ s32 animationActive;
    /* 0x20 */ s32 soundPlayed;
} ArenaObjectAnimationState; /* size = 0x24 */

typedef struct MotionResourceInfo {
    /* 0x00 */ char *motionName;
    /* 0x04 */ u32 motionId;
    /* 0x08 */ s32 motionAttributes;
    /* 0x0C */ float motionThreshold;
} MotionResourceInfo; /* size = 0x10 */

typedef struct ScoreDisplayState {
    /* 0x00 */ s32 hundredsSprite;
    /* 0x04 */ s32 tensSprite;
    /* 0x08 */ s32 onesSprite;
    /* 0x0C */ s32 extraSprite;
    /* 0x10 */ s32 digitCount;
    /* 0x14 */ s32 score;
    /* 0x18 */ s32 *digitSprites;
} ScoreDisplayState; /* size = 0x1C */

typedef struct ArenaWaypoint {
    /* 0x00 */ s32 targetIndex;
    /* 0x04 */ float targetX;
    /* 0x08 */ float targetY;
    /* 0x0C */ float targetZ;
    /* 0x10 */ u32 targetFlags;
} ArenaWaypoint; /* size = 0x14 */

typedef struct ArenaObjectState {
    char *objectName;
    Vec position;
    s32 modelId;
    float animationTime;
    Vec targetPosition;
    s32 state;
    s32 flags;
} ArenaObjectState; /* size = 0x2C */

typedef struct CollisionProfile {
    float movementSpeed;
    s32 canAttack;
} CollisionProfile; /* size = 0x08 */

typedef void (*PlayerOldHook)(OMOBJ *obj);
typedef Vec *(*PlayerOldPosHook)(OMOBJ *obj);

typedef struct PlayerMovementWork {
    OMOBJ *object;
    PlayerOldHook updateHook;
    PlayerOldPosHook positionHook;
    PlayerOldHook collisionHook;
    PlayerOldHook stateHook;
    s32 *modelIds;
    s32 playerIndex;
    s32 playerType;
    s32 inputX;
    s32 inputZ;
    s32 movementX;
    s32 movementZ;
    s32 targetPlayer;
    s32 collisionCount;
    s32 actionState;
    s32 stateFlags;
    s32 selectionIndex;
    s32 actionId;
    s32 actionTimer;
    s32 collisionTimer;
    Vec *positionHistory;
    Vec position;
    float heightOffset;
    s32 obstacleCount;
    Vec *obstacles;
    Vec worldPosition;
    Vec previousPosition;
    Vec acceleration;
    Vec velocity;
    float rotationY;
    float rotationSpeed;
    float verticalVelocity;
    float jumpHeight;
    Vec collisionPosition;
    float movementScale;
    float collisionRadius;
    float collisionHeight;
} PlayerMovementWork;

typedef PlayerMovementWork *(*PlayerOldFind)(int no);

typedef struct ArenaPlayerState {
    PlayerMovementWork playerWork;
    ArenaWaypoint *targetWaypoints;
    float collisionRadius;
    float movementProgress;
    Vec targetPosition;
    Vec movementVelocity;
    float animationTimer;
    float targetRotation;
    s32 unreferencedF0;
    s32 unreferencedF4;
    float transitionProgress;
    u32 stateFlags;
    s32 actionIndex;
    s32 selectedArenaIndex;
    s32 attackTimer;
    s32 targetCount;
    s32 currentTarget;
    s32 effectTimer;
    s32 playerState;
    s32 characterId;
    s32 controllerIndex;
} ArenaPlayerState; /* size = 0x124 */

typedef struct MovingEffectState {
    Vec position;
    Vec startingPosition;
    Vec targetPosition;
    Vec movementVelocity;
    Vec movementOffset;
    float rotation;
    float bounceScale;
    float bounceVelocity;
    float verticalVelocity;
    float horizontalVelocity;
    float lifetime;
    s32 hookModelId;
    s32 effectModelId;
    float targetHeight;
    float targetRadius;
    Vec interactionPosition;
    s32 targetPlayerIndex;
    s32 isFirstImpact;
    s32 removeRequested;
    s32 chasingPlayer;
} MovingEffectState; /* size = 0x80 */

typedef struct ArenaOpponentState {
    PlayerMovementWork playerWork;
    CollisionProfile *collisionData;
    Vec2f *cornerPositions;
    char *animationName;
    float *targetHeight;
    Vec targetPosition;
    float actionTimer;
    float rotationProgress;
    float motionTimer;
    float targetRotation;
    float jumpTimer;
    float turnTimer;
    s32 targetIndex;
    s32 stateFlags;
    float movementSpeed;
    float turnSpeed;
    float jumpSpeed;
    s32 actionIndex;
    s32 targetPlayer;
    Vec movementOrigin;
    Vec movementTarget;
    s32 hitCount;
    s32 targetCount;
    s32 actionTimer2;
    s32 isOutsideArena;
    s32 particleId;
    s32 collisionState;
    float verticalOffset;
    s32 impactState;
    s32 targetState;
    s32 actionState;
    u32 stateFlags2;
} ArenaOpponentState; /* size = 0x158 */

typedef struct ParticleState {
    s32 animationState;
    Vec originPosition;
    float age;
    float startAngle;
    float angleOffset;
    float ageStep;
    float lifetime;
    float targetHeight;
    float rotationAngle;
    float scale;
    Vec position;
    s16 textureFrame;
    s16 particleIndex;
} ParticleState; /* size = 0x40 */

typedef void (*ParticleDrawHook)(HU3DMODEL *, struct ParticleSystemState *, Mtx);

typedef struct ParticleSystemState {
    ParticleDrawHook drawHook;
    ANIMDATA *spriteAnimation;
    Vec *positionOffset;
    float *scaleOffset;
    ParticleState *particles;
    Vec *vertexPositions;
    Vec2f *textureCoordinates;
    Vec2f *textureFrameCoordinates;
    GXColor *vertexColors;
    void *displayList;
    Vec quadVertices;
    float topEdge;
    float rightEdge;
    float bottomEdge;
    float leftEdge;
    float upperLeftX;
    float upperLeftY;
    float lowerLeftX;
    float lowerLeftY;
    float edgeOffset;
    u32 displayListSize;
    float textureFrameWidth;
    float textureFrameHeight;
    s16 particleCapacity;
    s16 activeParticleCount;
    s16 textureFrameCount;
    u8 blendMode;
    u8 destroyRequested;
} ParticleSystemState; /* size = 0x6C */

s32 ScoreDisplaySubtractPoints(s32 scoreDisplayIndex, s32 pointsToSubtract);
s32 MinigameTypeIsAvailable(s32 minigameType);
Vec *ArenaWaypointPositionGet(s32 waypointIndex);
void ArenaModelTargetMove(Vec *startPosition, Vec *targetPosition, s32 layoutIndex);
void ArenaFireEffectCreate(Vec *position, float rotation);
void HookEffectCreate(s32 hookModelId, s32 effectModelId, float targetHeight);
void HookEffectArcCreate(s32 hookModelId, Vec *movementVelocity);
void ThunderAttackUpdate();
s32 PointInsideTriangleCheck(Vec *point, Vec *triangleVertices);
s32 ArenaPlayerOpponentCollisionResolve();
s32 PlayerMoveTowardPosition(PlayerMovementWork *player, Vec *targetPosition);
s32 ArenaPlayerTargetListReset(ArenaPlayerState *playerState);
void ArenaPlayerTargetRemove(s32 targetIndex);
s32 ArenaPlayerCanMoveCheck(ArenaPlayerState *playerState);
void ArenaPlayerRoundResultUpdate(OMOBJ *object);
void ArenaUnusedRoutinePrototype();
void ArenaSceneExit(OMOBJ *object);
void ArenaOpponentMotionSet(s32 motionIndex, float transitionFrames, u32 motionAttributes, s32 collisionState, float verticalOffset);
s32 ArenaOpponentImpactSoundPlay(s32 opponentIndex, s32 soundId);
s32 ArenaOpponentTargetStateSet(s32 opponentIndex, s32 targetState);
void ArenaOpponentAnimationSet(s32 opponentIndex, s32 motionIndex, float transitionFrames, u32 motionAttributes, s32 collisionState, float verticalOffset);

s16 lbl1Data0[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

s32 lbl1Data10[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

float lbl1Data30[0xA] = { 70.0f, 70.0f, 70.0f, 70.0f, 70.0f, 70.0f, 70.0f, 70.0f, 70.0f, 100.0f };

float lbl1Data58[0xA] = {
    150.0f,
    170.0f,
    190.0f,
    170.0f,
    160.0f,
    150.0f,
    180.0f,
    200.0f,
    150.0f,
    200.0f,
};

PlayerOldFind playerByIndex;
float lbl1Bss1A8;
OMOBJ *lbl1Bss1A4;
OMOBJ *lbl1Bss1A0;
OMOBJ *lbl1Bss144[0x17];
OMOBJ **lbl1Bss140;
OMOBJ **lbl1Bss13C;
// data: ArenaPlayerState
OMOBJ *lbl1Bss138;
// data: ArenaOpponentState
OMOBJ *lbl1Bss134;
// data: ArenaOpponentState
OMOBJ **lbl1Bss130;
float lbl1Bss12C;
float lbl1Bss128;
float lbl1Bss124;
s32 lbl1Bss120;
s32 lbl1Bss11C;
s32 lbl1Bss118;
s32 lbl1Bss114;
s32 lbl1Bss110;
s32 lbl1Bss10C;
s32 lbl1Bss108;
ArenaObjectState *lbl1Bss104;
s32 lbl1Bss100;
ArenaModelLayout *lbl1BssFC;
s32 lbl1BssF8;
Vec lbl1BssEC;
void *lbl1BssE8[1];
MovingEffectState *lbl1BssE4[1];
MovingEffectState *lbl1BssDC[2];
s32 lbl1BssD8;
Vec lbl1BssCC;
float lbl1BssC8;
s32 lbl1BssC4;
s32 lbl1BssB8[3];
Mtx lbl1Bss88;
Vec lbl1Bss4C[5];
Vec lbl1Bss34[2];
s16 lbl1Bss2C[4];
ANIMDATA *lbl1Bss28;
s16 lbl1Bss24;
s32 lbl1Bss20;
s32 lbl1Bss1C;
s32 lbl1Bss18;
ANIMDATA *lbl1Bss14;
s32 lbl1Bss10;
s32 lbl1BssC;
s32 lbl1Bss8;
s32 lbl1Bss4;
s32 lbl1Bss0;

void PlayerAnimationIndexUpdate(PlayerMovementWork *player)
{
    s32 i;
    for (i = 0; i < 8; i++) {
        if (player->actionId == player->modelIds[i]) {
            player->selectionIndex = lbl1Data10[i];
            return;
        }
    }
    player->selectionIndex = -1;
}

void PlayerCollisionPointDistanceUpdate(float *minDist, s32 *state, s32 stateNew, Vec *posA, float aYOfs, Vec *posB, float bYOfs)
{
    Vec pointA = *posA;
    Vec pointB = *posB;
    float dist;
    pointA.y += aYOfs;
    pointB.y += bYOfs;
    dist = VECDistance(&pointA, &pointB);
    if (*minDist > dist) {
        *minDist = dist;
        *state = stateNew;
    }
}

void PlayerCollisionDistanceGet(PlayerMovementWork *player1, PlayerMovementWork *player2, float *minDist, s32 *state)
{
    *minDist = 10 * (player1->collisionRadius + player2->collisionRadius);
    PlayerCollisionPointDistanceUpdate(minDist, state, 0, &player1->worldPosition, player1->collisionRadius, &player2->worldPosition, player2->collisionRadius);
    PlayerCollisionPointDistanceUpdate(minDist, state, 1, &player1->worldPosition, player1->collisionHeight - player1->collisionRadius, &player2->worldPosition, player2->collisionHeight - player2->collisionRadius);
    PlayerCollisionPointDistanceUpdate(minDist, state, 2, &player1->worldPosition, player1->collisionRadius, &player2->worldPosition, player2->collisionHeight - player2->collisionRadius);
    PlayerCollisionPointDistanceUpdate(minDist, state, 3, &player1->worldPosition, player1->collisionHeight - player1->collisionRadius, &player2->worldPosition, player2->collisionRadius);
}

s32 PlayerCollisionVelocityResolve(PlayerMovementWork *player, PlayerMovementWork *player2)
{
    Vec dir;
    Vec dir2;
    float dist;

    player->stateFlags |= 0x1;
    player->velocity.x = player->inputX / 4.0f;
    player->velocity.z = -(float)player->inputZ / 4.0f;
    if (player->velocity.x != 0.0f || player->velocity.z != 0.0f) {
        return;
    }
    VECSubtract(&player->worldPosition, &player2->worldPosition, &dir);
    dist = VECMagXZ(&dir);
    if (0.0f == dist) {
        dir.x = 0;
        dir.z = -1;
    }
    else {
        dir.x /= dist;
        dir.z /= dist;
    }
    VECSubtract(&player->worldPosition, &player->previousPosition, &dir2);
    dist = VECMagXZ(&dir2);
    if (5 > dist) {
        dist = 5;
    }
    player->velocity.x = dir.x * dist;
    player->velocity.z = dir.z * dist;
}

void PlayerJumpStart(PlayerMovementWork *player, float jumpAcceleration, float initialVerticalVelocity)
{
    player->actionState = 1;
    player->acceleration.y = jumpAcceleration;
    player->verticalVelocity = initialVerticalVelocity;
    if (player->stateFlags & 0x2) {
        player->stateFlags &= ~0x2;
    }
}

void PlayerLookupConfigure(PlayerOldFind find, float verticalSeparation)
{
    playerByIndex = find;
    lbl1Bss1A8 = verticalSeparation;
}

void PlayerMovementWorkInit(PlayerMovementWork *player, OMOBJ *obj, s32 playerIndex, s32 playerType, s32 *modelIds, s32 initialActionId, Vec *initialPosition, float initialRotation)
{
    player->object = obj;
    player->playerIndex = playerIndex;
    player->playerType = playerType;
    player->modelIds = modelIds;
    player->updateHook = NULL;
    player->positionHook = NULL;
    player->collisionHook = NULL;
    player->stateHook = NULL;
    player->positionHistory = NULL;
    player->obstacleCount = 0;
    player->obstacles = NULL;
    player->actionState = 0;
    player->stateFlags = 0;
    player->actionId = initialActionId;
    player->actionTimer = 0;
    player->collisionTimer = 0;
    PlayerAnimationIndexUpdate(player);
    player->rotationY = initialRotation;
    player->worldPosition = *initialPosition;
    player->previousPosition = *initialPosition;
    player->acceleration.x = 0;
    player->acceleration.y = 0;
    player->acceleration.z = 0;
    player->movementScale = 1;
    player->jumpHeight = 0;
    player->collisionRadius = lbl1Data30[player->playerType];
    player->collisionHeight = lbl1Data58[player->playerType];
}

void PlayerUpdateHookSet(PlayerMovementWork *player, PlayerOldHook updateHook)
{
    player->updateHook = updateHook;
}

void PlayerPositionHookSet(PlayerMovementWork *player, PlayerOldPosHook positionHook)
{
    player->positionHook = positionHook;
}

void PlayerCollisionHookSet(PlayerMovementWork *player, PlayerOldHook collisionHook)
{
    player->collisionHook = collisionHook;
}

void PlayerStateHookSet(PlayerMovementWork *player, PlayerOldHook stateHook)
{
    player->stateHook = stateHook;
}

void PlayerObstaclesSet(PlayerMovementWork *player, s32 obstacleCount, Vec *obstacleBounds)
{
    player->obstacleCount = obstacleCount;
    player->obstacles = obstacleBounds;
}

void PlayerObjectTransformSync(PlayerMovementWork *player)
{
    omSetTra(player->object, player->worldPosition.x, player->worldPosition.y, player->worldPosition.z);
    omSetRot(player->object, 0.0f, player->rotationY, 0.0f);
}

void PlayerCollisionResolve(PlayerMovementWork *player)
{
    BOOL addTime;
    s32 i;
    player->stateFlags &= ~0x100;
    if (player->stateFlags & 0x8) {
        return;
    }
    if (NULL == playerByIndex) {
        return;
    }
    addTime = FALSE;
    for (i = 0; i < 4; i++) {
        PlayerMovementWork *player2 = playerByIndex(i);
        BOOL fixPos;
        float minDist;
        s32 state;
        if (player == player2) {
            continue;
        }
        if (player2->stateFlags & 0x8) {
            continue;
        }
        fixPos = FALSE;
        PlayerCollisionDistanceGet(player, player2, &minDist, &state);
        if (minDist < player->collisionRadius + player2->collisionRadius) {
            if (state < 2) {
                fixPos = TRUE;
                if (player->actionState == 1) {
                    PlayerCollisionVelocityResolve(player, player2);
                }
            }
            else {
                if (state == 2 && player->actionState == 1) {
                    PlayerCollisionVelocityResolve(player, player2);
                    player->stateFlags |= 0x100;
                    player->verticalVelocity = 0;
                    if (!(player2->stateFlags & 0x100)) {
                        player->acceleration.y = (20.0f*(80.3f/REFRESH_RATE));
                    }
                }
                if (state == 3) {
                    fixPos = TRUE;
                    if (player->actionState == 1) {
                        if (0.0f < player->acceleration.y) {
                            player->acceleration.y = 0.0f;
                        }
                        player->verticalVelocity = 0;
                    }
                }
            }
        }
        if (fixPos) {
            Vec dir;
            addTime = TRUE;
            VECSubtract(&player->worldPosition, &player2->worldPosition, &dir);
            if (0.0f == minDist) {
                dir.x = 0;
                dir.z = -1;
            }
            else {
                dir.x /= minDist;
                dir.z /= minDist;
            }
            player->worldPosition.x = player2->worldPosition.x + (dir.x * (player->collisionRadius + player2->collisionRadius));
            player->worldPosition.z = player2->worldPosition.z + (dir.z * (player->collisionRadius + player2->collisionRadius));
        }
    }
    if (addTime) {
        player->collisionTimer++;
    }
    else {
        player->collisionTimer = 0;
    }
}

float PlayerArenaBoundaryResolve(PlayerMovementWork *player, float heightOffset)
{
    Vec *pos;
    s32 i;
    AddX = 0.0f;
    AddZ = 0.0f;
    MapWall(player->collisionRadius, player->worldPosition.x, heightOffset + player->collisionRadius, player->worldPosition.z);
    player->worldPosition.x += AddX;
    player->worldPosition.z += AddZ;
    if (NULL != player->obstacles) {
        for (pos = player->obstacles, i = 0; i < player->obstacleCount; i++, pos += 2) {
            int num = 0;
            if (pos[0].x == pos[1].x || pos[0].x > player->worldPosition.x || pos[1].x < player->worldPosition.x) {
                num++;
            }
            if (pos[0].y == pos[1].y || pos[0].y > player->worldPosition.y || pos[1].y < player->worldPosition.y) {
                num++;
            }
            if (pos[0].z == pos[1].z || pos[0].z > player->worldPosition.z || pos[1].z < player->worldPosition.z) {
                num++;
            }
            if (num == 3) {
                if (pos[0].x != pos[1].x) {
                    if (pos[0].x > player->worldPosition.x) {
                        player->worldPosition.x = pos[0].x + player->collisionRadius;
                    }
                    if (pos[1].x < player->worldPosition.x) {
                        player->worldPosition.x = pos[1].x - player->collisionRadius;
                    }
                }
                if (pos[0].y != pos[1].y) {
                    if (pos[0].y > player->worldPosition.y) {
                        player->worldPosition.y = pos[0].y;
                    }
                    if (pos[1].y < player->worldPosition.y) {
                        player->worldPosition.y = pos[1].y;
                    }
                }
                if (pos[0].z != pos[1].z) {
                    if (pos[0].z > player->worldPosition.z) {
                        player->worldPosition.z = pos[0].z + player->collisionRadius;
                    }
                    if (pos[1].z < player->worldPosition.z) {
                        player->worldPosition.z = pos[1].z - player->collisionRadius;
                    }
                }
            }
        }
    }
    return ABS(AddX) + ABS(AddZ);
}

void PlayerMotionStateWalk(PlayerMovementWork *player, s32 *motionState, u32 *motionAttributes)
{
    *motionAttributes = HU3D_MOTATTR_LOOP;
    *motionState = 0;
    if (0.5f <= player->rotationSpeed) {
        if (7.0f <= player->rotationSpeed) {
            *motionState = 2;
        }
        else {
            *motionState = 1;
        }
    }

    if (player->modelIds[3] >= 0 && (player->movementX & 0x100)) {
        PlayerJumpStart(player, (20.0f*(80.3f/REFRESH_RATE)), 1.0f);
        *motionAttributes = HU3D_MOTATTR_NONE;
        *motionState = 3;
    }
}

void PlayerMotionStateJump(PlayerMovementWork *player, s32 *motionState, u32 *motionAttributes)
{
    if (0.0f < player->verticalVelocity) {
        if (player->movementZ & 0x100) {
            player->verticalVelocity++;
            if (4.0f < player->verticalVelocity) {
                player->verticalVelocity = 0;
            }
            else {
                player->acceleration.y += 219.0f / REFRESH_RATE;
            }
        }
        else {
            player->verticalVelocity = 0;
        }
    }
    player->acceleration.y += -146.0f / REFRESH_RATE;
    if (player->modelIds[5] >= 0 && (player->movementX & 0x140)) {
        player->actionState = 3;
        player->acceleration.y = 0;
        player->stateFlags &= ~0x3;
        player->stateFlags |= 0x4;
        *motionAttributes = HU3D_MOTATTR_NONE;
        *motionState = 5;
    }
}

void PlayerMotionStateLand(PlayerMovementWork *player, s32 *motionState, u32 *motionAttributes)
{
    if ((player->stateFlags & 0x1000) || Hu3DMotionEndCheck(player->object->mdlId[0])) {
        player->stateFlags &= ~0x3;
        if (player->selectionIndex == 6) {
            player->actionState = 4;
            *motionAttributes = HU3D_MOTATTR_NONE;
            *motionState = 7;
        }
        else {
            player->actionState = 0;
            *motionAttributes = HU3D_MOTATTR_LOOP;
            *motionState = 0;
        }
    }
}

void PlayerMotionStateStop(PlayerMovementWork *player, s32 *motionState, u32 *motionAttributes)
{
    if (Hu3DMotionShiftIDGet(player->object->mdlId[0]) < 0 && Hu3DMotionEndCheck(player->object->mdlId[0])) {
        PlayerJumpStart(player, 0.0f, 0.0f);
    }
}

void PlayerMotionStateFall(PlayerMovementWork *player, s32 *motionState, u32 *motionAttributes)
{
    if (Hu3DMotionShiftIDGet(player->object->mdlId[0]) < 0 && Hu3DMotionEndCheck(player->object->mdlId[0])) {
        player->stateFlags &= ~0x4;
        player->actionState = 0;
        *motionAttributes = HU3D_MOTATTR_LOOP;
        *motionState = 0;
    }
}

void PlayerMotionUpdate(PlayerMovementWork *player)
{
    u32 motAttr = HU3D_MOTATTR_NONE;
    s32 state = -1;
    float boundaryCollisionAmount;
    float heightAboveGround;
    float previousWorldHeight;
    Vec *surfacePositions;
    BOOL hasSupportingSurface;
    if (player->stateFlags & 0x1) {
        player->acceleration.x = player->velocity.x;
        player->acceleration.z = player->velocity.z;
    }
    else {
        player->acceleration.x = player->inputX / 4.0f;
        player->acceleration.z = -(float)player->inputZ / 4.0f;
        if (player->actionState == 1 && 0.0f == player->acceleration.x && 0.0f == player->acceleration.z) {
            player->acceleration.x = 0.98f * (player->worldPosition.x - player->previousPosition.x);
            player->acceleration.z = 0.98f * (player->worldPosition.z - player->previousPosition.z);
        }
        if (player->stateFlags & 0x14) {
            player->acceleration.x = 0;
            player->acceleration.z = 0;
            player->movementX = 0;
        }
    }
    player->previousPosition = player->worldPosition;
    player->rotationSpeed = VECMagXZ(&player->acceleration);
    if (0.5f <= player->rotationSpeed) {
        if (10.0f <= player->rotationSpeed) {
            player->acceleration.x /= player->rotationSpeed;
            player->acceleration.z /= player->rotationSpeed;
            player->rotationSpeed = 10;
            player->acceleration.x = 10 * player->acceleration.x;
            player->acceleration.z = 10 * player->acceleration.z;
        }
    }
    else {
        player->acceleration.x = 0;
        player->acceleration.z = 0;
        player->rotationSpeed = 0;
    }
    player->acceleration.x *= player->movementScale;
    player->acceleration.z *= player->movementScale;
    player->rotationSpeed *= player->movementScale;
    if (player->stateFlags & 0x800) {
        player->movementX |= 0x100;
        player->movementZ |= 0x100;
        if (player->actionState == 1) {
            if (0.0f == player->verticalVelocity) {
                player->stateFlags &= ~0x800;
            }
        }
    }
    switch (player->actionState) {
        case 0:
            PlayerMotionStateWalk(player, &state, &motAttr);
            break;

        case 1:
            PlayerMotionStateJump(player, &state, &motAttr);
            break;

        case 2:
            PlayerMotionStateLand(player, &state, &motAttr);
            break;

        case 3:
            PlayerMotionStateStop(player, &state, &motAttr);
            break;

        case 4:
            PlayerMotionStateFall(player, &state, &motAttr);
            break;

        default:
            OSReport("*** player action error(%d)!!\n", player->actionState);
            break;
    }
    if (player->stateFlags & 0x2000) {
        player->acceleration.y = 0;
    }
    boundaryCollisionAmount = 0.0f;
    if (50.0f < ABS(player->acceleration.y)) {
        player->acceleration.y = 50.0f * ((0.0f > player->acceleration.y) ? -1 : 1);
    }
    if (player->stateFlags & 0x2) {
        Mtx rotMtx;
        MTXRotDeg(rotMtx, 'y', player->positionHistory[2].x - player->heightOffset);
        MTXMultVec(rotMtx, &player->position, &player->position);
        VECAdd(&player->position, &player->positionHistory[0], &player->worldPosition);
        player->rotationY += player->positionHistory[2].x - player->heightOffset;
    }
    previousWorldHeight = player->worldPosition.y;
    VECAdd(&player->worldPosition, &player->acceleration, &player->worldPosition);
    if ((player->stateFlags & 0x1) == 0 && (0.0f != player->acceleration.x || 0.0f != player->acceleration.z)) {
        player->rotationY = AngleInterpolate(player->rotationY, atan2d(player->acceleration.x, player->acceleration.z), 0.4f * player->movementScale);
    }
    boundaryCollisionAmount += PlayerArenaBoundaryResolve(player, previousWorldHeight);
    if (nMap == 0) {
        player->jumpHeight = lbl1Bss1A8;
    }
    else {
        player->jumpHeight = MapPos(player->worldPosition.x, player->worldPosition.y, player->worldPosition.z, player->collisionHeight, &player->collisionPosition);
    }
    hasSupportingSurface = FALSE;
    if (NULL != player->positionHook) {
        surfacePositions = player->positionHook(player->object);
        if (NULL != surfacePositions) {
            if (player->jumpHeight < surfacePositions[1].y) {
                hasSupportingSurface = TRUE;
                player->positionHistory = surfacePositions;
                player->jumpHeight = surfacePositions[1].y;
            }
        }
    }
    if (!hasSupportingSurface && (player->stateFlags & 0x2)) {
        player->stateFlags &= ~0x2;
    }
    if (player->actionState == 1 && player->jumpHeight >= player->worldPosition.y) {
        player->actionState = 2;
        player->stateFlags &= ~0x3;
        player->worldPosition.y = player->jumpHeight;
        player->acceleration.y = 0;
        motAttr = HU3D_MOTATTR_NONE;
        state = 4;
        if (player->selectionIndex == 5) {
            state = 6;
            if (NULL != player->stateHook) {
                player->stateHook(player->object);
            }
            if (player->playerIndex < 4) {
                omVibrate(player->playerIndex, 12, 4, 2);
            }
        }
        if (player->stateFlags & 0x200) {
            state = -1;
            if (NULL != player->collisionHook) {
                player->collisionHook(player->object);
            }
        }
        if (hasSupportingSurface) {
            player->stateFlags |= 0x2;
        }
    }
    heightAboveGround = player->worldPosition.y - player->jumpHeight;
    if (player->actionState == 0 && 10.0f < heightAboveGround) {
        if ((player->stateFlags & 0x2000) == 0) {
            if ((player->stateFlags & 0x400) != 0) {
                player->stateFlags |= 0x800;
            }
            else {
                PlayerJumpStart(player, 0.0f, 0.0f);

                motAttr = HU3D_MOTATTR_NONE;
                state = 3;
            }
        }
    }
    else if (hasSupportingSurface) {
        if (!(player->stateFlags & 0x2)) {
            player->worldPosition.y = player->jumpHeight;
            player->stateFlags |= 0x2;
        }
    }
    PlayerCollisionResolve(player);
    if (NULL != player->updateHook) {
        player->updateHook(player->object);
    }
    boundaryCollisionAmount += PlayerArenaBoundaryResolve(player, player->worldPosition.y);
    if (player->stateFlags & 0x2) {
        player->heightOffset = player->positionHistory[2].x;
        VECSubtract(&player->worldPosition, &player->positionHistory[0], &player->position);
    }
    if (0.0f != boundaryCollisionAmount) {
        player->actionTimer++;
    }
    else {
        player->actionTimer = 0;
    }
    if (state >= 0 && player->selectionIndex != state) {
        player->selectionIndex = state;
        player->actionId = player->modelIds[player->selectionIndex];
        if (player->actionId >= 0 && !(player->stateFlags & 0x20)) {
            Hu3DMotionShiftSet(player->object->mdlId[0], player->object->mtnId[player->actionId], 0, 8, motAttr);
        }
    }
}

void PlayerMotionSet(PlayerMovementWork *player, s32 motionId)
{
    s32 i;
    player->actionId = motionId;
    player->actionState = 0;
    player->stateFlags &= 0x1400;
    player->previousPosition = player->worldPosition;
    player->acceleration.x = 0;
    player->acceleration.y = 0;
    player->acceleration.z = 0;
    PlayerAnimationIndexUpdate(player);
    if (player->selectionIndex == 3) {
        Hu3DModelAttrReset(player->object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
        Hu3DMotionSet(player->object->mdlId[0], player->object->mtnId[player->actionId]);
        Hu3DMotionTimeSet(player->object->mdlId[0], Hu3DMotionMaxTimeGet(player->object->mdlId[0]) - 1);
    }
    else if (player->selectionIndex == 5) {
        player->actionState = 3;
        player->stateFlags |= 0x4;
        Hu3DModelAttrReset(player->object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
        Hu3DMotionSet(player->object->mdlId[0], player->object->mtnId[player->actionId]);
        Hu3DMotionTimeSet(player->object->mdlId[0], Hu3DMotionMaxTimeGet(player->object->mdlId[0]) - 1);
    }
    else {
        Hu3DModelAttrReset(player->object->mdlId[0], HU3D_MOTATTR_PAUSE);
        Hu3DMotionShiftSet(player->object->mdlId[0], player->object->mtnId[player->actionId], 0, 8, HU3D_MOTATTR_LOOP);
    }
}

void PlayerStateFlagsSet(PlayerMovementWork *player, s32 stateFlags)
{
    player->stateFlags |= stateFlags;
}

void PlayerStateFlagsReset(PlayerMovementWork *player, s32 stateFlags)
{
    player->stateFlags &= ~stateFlags;
}

void PlayerFireParticleUpdate(HU3DMODEL *particleModel, ParticleSystemState *particleSystem, Mtx modelMatrix)
{
    Vec playerUpperCollisionPoint;
    Vec playerLowerCollisionPoint;
    float normalizedParticleAge;
    float collisionDistance;
    float particleAlpha;

    ParticleState *particle;
    PlayerMovementWork *player;
    GXColor *particleColor;
    s16 particleIndex;
    ArenaPlayerState *playerState;

    playerState = lbl1Bss138->data;
    player = &playerState->playerWork;
    particle = particleSystem->particles;
    particleColor = particleSystem->vertexColors;
    particleSystem->activeParticleCount = 0;
    for (particleIndex = 0; particleIndex < particleSystem->particleCapacity; particleIndex++, particle++, particleColor++) {
        if (particle->particleIndex >= 0) {
            particle->age += particle->ageStep;
            if (particle->lifetime < particle->age) {
                particle->particleIndex = -1;
            }
            else {
                particleSystem->activeParticleCount++;
                if (NULL != particleSystem->positionOffset) {
                    particle->originPosition = *particleSystem->positionOffset;
                }
                if (NULL != particleSystem->scaleOffset) {
                    particle->startAngle = *particleSystem->scaleOffset;
                }
                particle->position.x = particle->originPosition.x + (particle->age * sind((particle->startAngle + particle->angleOffset)));
                particle->position.y = particle->position.y + (0.03f * (particle->targetHeight - particle->position.y));
                particle->position.z = particle->originPosition.z + (particle->age * cosd((particle->startAngle + particle->angleOffset)));
                normalizedParticleAge = particle->age / particle->lifetime;
                particleAlpha = 512.0 * cosd((90.0f * normalizedParticleAge));
                particleColor->a = 255.0f < particleAlpha ? 255.0f : particleAlpha;
                particle->textureFrame = particleSystem->textureFrameCount * normalizedParticleAge;
                if (particleSystem->textureFrameCount <= particle->textureFrame) {
                    particle->textureFrame = particleSystem->textureFrameCount - 1;
                }
                normalizedParticleAge *= 2.0f;
                if (1.0f < normalizedParticleAge) {
                    normalizedParticleAge = 1.0f;
                }
                particle->scale = sind((100.0f * normalizedParticleAge));
                collisionDistance = (26.666666f * particle->scale) + player->collisionRadius;
                playerUpperCollisionPoint = player->worldPosition;
                playerUpperCollisionPoint.y += collisionDistance;
                playerLowerCollisionPoint = player->worldPosition;
                playerLowerCollisionPoint.y += player->collisionHeight - collisionDistance;
                if ((particle->textureFrame < 0xC)
                    && ((collisionDistance > VECDistance(&playerUpperCollisionPoint, &particle->position)) || (collisionDistance > VECDistance(&playerLowerCollisionPoint, &particle->position)))) {
                    playerState->stateFlags |= 0x20;
                }
            }
        }
    }
}
void ParticleVertexDataPack(Vec *vertices, float *packedCoordinates)
{
    packedCoordinates[0] = vertices[0].x;
    packedCoordinates[4] = vertices[0].y;
    packedCoordinates[8] = vertices[0].z;
    packedCoordinates[1] = vertices[1].y;
    packedCoordinates[5] = vertices[1].z;
    packedCoordinates[9] = vertices[2].x;
    packedCoordinates[2] = vertices[2].z;
    packedCoordinates[6] = vertices[3].x;
    packedCoordinates[10] = vertices[3].y;
    packedCoordinates[3] = 0.0f;
    packedCoordinates[7] = 0.0f;
    packedCoordinates[11] = 0.0f;
}

void VectorArrayScale(Vec *sourceVectors, Vec *scaledVectors, s16 vectorCount, float scale)
{
    s16 vectorIndex;
    for (vectorIndex = 0; vectorIndex < vectorCount; vectorIndex++, sourceVectors++, scaledVectors++) {
        scaledVectors->x = sourceVectors->x * scale;
        scaledVectors->y = sourceVectors->y * scale;
        scaledVectors->z = sourceVectors->z * scale;
    }
}

void TextureFrameCoordinatesSet(Vec2f *textureCoordinates, Vec2f *frameCoordinates, s16 frameIndex, float frameWidth, float frameHeight)
{
    textureCoordinates[0].x = frameCoordinates[frameIndex].x;
    textureCoordinates[0].y = frameCoordinates[frameIndex].y;
    textureCoordinates[1].x = frameWidth + frameCoordinates[frameIndex].x;
    textureCoordinates[1].y = frameCoordinates[frameIndex].y;
    textureCoordinates[2].x = frameWidth + frameCoordinates[frameIndex].x;
    textureCoordinates[2].y = frameHeight + frameCoordinates[frameIndex].y;
    textureCoordinates[3].x = frameCoordinates[frameIndex].x;
    textureCoordinates[3].y = frameHeight + frameCoordinates[frameIndex].y;
}

void ParticleSystemDraw(HU3DMODEL *particleModel, Mtx modelMatrix)
{
    Mtx modelRotationMatrix;
    ROMtx reorderedRotationMatrix;
    Mtx particleRotationMatrix;
    Mtx combinedRotationMatrix;
    Vec quadVerticesInCameraSpace[4];
    Vec scaledQuadVertices[4];
    Vec rotatedQuadVertex3;
    Vec rotatedQuadVertex2;
    Vec rotatedQuadVertex1;
    Vec rotatedQuadVertex0;
    ParticleDrawHook particleDrawHook;
    s16 textureFormat;

    ParticleSystemState *particleSystem;
    Vec *vertexPosition;
    ParticleState *particle;
    Vec2f *textureCoordinates;
    s16 particleIndex;

    particleSystem = particleModel->hookData;
    if ((particleSystem->destroyRequested == 0) || (particleSystem->activeParticleCount != 0)) {
        GXLoadPosMtxImm(modelMatrix, 0);
        GXSetNumTevStages(1);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, GX_FALSE, 0x7D);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        textureFormat = particleSystem->spriteAnimation->bmp->dataFmt & 0xF;
        if ((textureFormat == 8) || (textureFormat == 7)) {
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXA, GX_CC_RASC, GX_CC_ZERO);
        }
        else {
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
        }
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
        if ((particleModel->attr & 2) != 0) {
            GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        }
        else {
            GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
        }
        HuSprTexLoad(particleSystem->spriteAnimation, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
        GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
        GXSetZCompLoc(GX_FALSE);
        switch (particleSystem->blendMode) {
            case 0:
                GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
                break;
            case 1:
                GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
                break;
            case 2:
                GXSetBlendMode(GX_BM_BLEND, GX_BL_ZERO, GX_BL_INVDSTCLR, GX_LO_NOOP);
                break;
        }
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetArray(GX_VA_POS, particleSystem->vertexPositions, 0xC);
        GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GXSetArray(GX_VA_CLR0, particleSystem->vertexColors, 4);
        GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
        GXSetArray(GX_VA_TEX0, particleSystem->textureCoordinates, 8);
        modelRotationMatrix[0][0] = modelMatrix[0][0];
        modelRotationMatrix[1][0] = modelMatrix[0][1];
        modelRotationMatrix[2][0] = modelMatrix[0][2];
        modelRotationMatrix[0][1] = modelMatrix[1][0];
        modelRotationMatrix[1][1] = modelMatrix[1][1];
        modelRotationMatrix[2][1] = modelMatrix[1][2];
        modelRotationMatrix[0][2] = modelMatrix[2][0];
        modelRotationMatrix[1][2] = modelMatrix[2][1];
        modelRotationMatrix[2][2] = modelMatrix[2][2];
        modelRotationMatrix[0][3] = 0.0f;
        modelRotationMatrix[1][3] = 0.0f;
        modelRotationMatrix[2][3] = 0.0f;
        PSMTXReorder(modelRotationMatrix, reorderedRotationMatrix);
        if ((omPauseChk() == 0) && (particleSystem->drawHook)) {
            particleDrawHook = particleSystem->drawHook;
            particleDrawHook(particleModel, particleSystem, modelMatrix);
        }
        particle = particleSystem->particles;
        vertexPosition = particleSystem->vertexPositions;
        textureCoordinates = particleSystem->textureCoordinates;
        PSMTXROMultVecArray(reorderedRotationMatrix, &particleSystem->quadVertices, quadVerticesInCameraSpace, 4);
        for (particleIndex = 0; particleIndex < particleSystem->particleCapacity; particleIndex++, particle++, textureCoordinates += 4) {
            if (particle->particleIndex < 0) {
                vertexPosition->x = vertexPosition->y = vertexPosition->z = 0.0f;
                vertexPosition++;
                vertexPosition->x = vertexPosition->y = vertexPosition->z = 0.0f;
                vertexPosition++;
                vertexPosition->x = vertexPosition->y = vertexPosition->z = 0.0f;
                vertexPosition++;
                vertexPosition->x = vertexPosition->y = vertexPosition->z = 0.0f;
                vertexPosition++;
            }
            else {
                if (particle->rotationAngle == 0.0f) {
                    VectorArrayScale(quadVerticesInCameraSpace, scaledQuadVertices, 4, particle->scale);
                    VECAdd(&scaledQuadVertices[0], &particle->position, vertexPosition++);
                    VECAdd(&scaledQuadVertices[1], &particle->position, vertexPosition++);
                    VECAdd(&scaledQuadVertices[2], &particle->position, vertexPosition++);
                    VECAdd(&scaledQuadVertices[3], &particle->position, vertexPosition++);
                }
                else {
                    VectorArrayScale(&particleSystem->quadVertices, scaledQuadVertices, 4, particle->scale);
                    MTXRotRad(particleRotationMatrix, 0x5A, 0.017453292f * particle->rotationAngle);
                    MTXConcat(modelRotationMatrix, particleRotationMatrix, combinedRotationMatrix);
                    MTXMultVecArray(combinedRotationMatrix, scaledQuadVertices, &rotatedQuadVertex0, 4);
                    VECAdd(&rotatedQuadVertex0, &particle->position, vertexPosition++);
                    VECAdd(&rotatedQuadVertex1, &particle->position, vertexPosition++);
                    VECAdd(&rotatedQuadVertex2, &particle->position, vertexPosition++);
                    VECAdd(&rotatedQuadVertex3, &particle->position, vertexPosition++);
                }
                if (particle->textureFrame != -1) {
                    TextureFrameCoordinatesSet(textureCoordinates, particleSystem->textureFrameCoordinates, particle->textureFrame, particleSystem->textureFrameWidth, particleSystem->textureFrameHeight);
                }
            }
        }
        DCFlushRangeNoSync(particleSystem->vertexPositions, particleSystem->particleCapacity * 0xC * 4);
        DCFlushRangeNoSync(particleSystem->textureCoordinates, particleSystem->particleCapacity * 8 * 4);
        GXCallDisplayList(particleSystem->displayList, particleSystem->displayListSize);
    }
}

void TextureFrameGridGenerate(Vec2f *frameCoordinates, s16 frameCount, s16 framesPerRow, float frameWidth, float frameHeight)
{
    s16 frameIndex;
    s16 columnIndex;
    s16 rowIndex;

    (void)frameHeight;
    (void)frameHeight;
    (void)frameHeight;
    (void)frameWidth;
    (void)frameWidth;
    (void)frameWidth;

    for (frameIndex = 0; frameIndex < frameCount; frameIndex++, frameCoordinates++) {
        columnIndex = frameIndex % framesPerRow;
        rowIndex = frameIndex / framesPerRow;
        frameCoordinates->x = columnIndex * frameWidth;
        frameCoordinates->y = rowIndex * frameHeight;
    }
}

s16 ParticleSystemCreate(ANIMDATA *spriteAnimation, Vec *positionOffset, float *scaleOffset, s16 particleCapacity, float quadHalfSize, s16 framesPerRow, s16 frameRowCount)
{
    void *displayListBuildBuffer;

    ParticleSystemState *particleSystem;
    s16 particleIndex;
    Vec2f *textureCoordinate;
    HU3DMODEL *particleModel;
    GXColor *particleColor;
    Vec *vertexPosition;
    ParticleState *particle;
    s16 textureColumnCount;
    void *displayListBuffer;
    s16 modelId;
    s16 textureRowCount;

    modelId = Hu3DHookFuncCreate(ParticleSystemDraw);
    particleModel = &Hu3DData[modelId];
    particleModel->hookData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(ParticleSystemState), particleModel->mallocNo);
    particleSystem = particleModel->hookData;
    spriteAnimation->useNum++;
    particleSystem->spriteAnimation = spriteAnimation;
    particleSystem->positionOffset = positionOffset;
    particleSystem->scaleOffset = scaleOffset;
    particleSystem->particleCapacity = particleCapacity;
    particleSystem->blendMode = 0;
    particleSystem->drawHook = 0;
    particleSystem->activeParticleCount = particleCapacity;
    particleSystem->destroyRequested = 0;
    particleSystem->particles = HuMemDirectMallocNum(HEAP_MODEL, particleCapacity * 0x40, particleModel->mallocNo);
    for (particle = particleSystem->particles, particleIndex = 0; particleIndex < particleCapacity; particleIndex++, particle++) {
        particle->particleIndex = -1;
        particle->textureFrame = -1;
    }
    particleSystem->vertexPositions = HuMemDirectMallocNum(HEAP_MODEL, particleCapacity * sizeof(Vec) * 4, particleModel->mallocNo);
    for (vertexPosition = particleSystem->vertexPositions, particleIndex = 0; particleIndex < (particleCapacity * 4); particleIndex++, vertexPosition++) {
        vertexPosition->x = 0.0f;
        vertexPosition->y = 0.0f;
        vertexPosition->z = 0.0f;
    }
    particleSystem->vertexColors = HuMemDirectMallocNum(HEAP_MODEL, particleCapacity * sizeof(GXColor), particleModel->mallocNo);
    for (particleColor = particleSystem->vertexColors, particleIndex = 0; particleIndex < particleCapacity; particleIndex++, particleColor++) {
        particleColor->r = 0xFF;
        particleColor->g = 0xFF;
        particleColor->b = 0xFF;
        particleColor->a = 0xFF;
    }
    particleSystem->textureCoordinates = HuMemDirectMallocNum(HEAP_MODEL, particleCapacity * sizeof(Vec2f) * 4, particleModel->mallocNo);
    textureCoordinate = particleSystem->textureCoordinates;
    for (particleIndex = 0; particleIndex < particleCapacity; particleIndex++) {
        textureCoordinate->x = 0.0f;
        textureCoordinate->y = 0.0f;
        textureCoordinate++;
        textureCoordinate->x = 1.0f;
        textureCoordinate->y = 0.0f;
        textureCoordinate++;
        textureCoordinate->x = 1.0f;
        textureCoordinate->y = 1.0f;
        textureCoordinate++;
        textureCoordinate->x = 0.0f;
        textureCoordinate->y = 1.0f;
        textureCoordinate++;
    }
    if ((framesPerRow != 0) || (frameRowCount != 0)) {
        textureColumnCount = spriteAnimation->bmp->sizeX / framesPerRow;
        textureRowCount = spriteAnimation->bmp->sizeY / frameRowCount;
        particleSystem->textureFrameCount = textureColumnCount * textureRowCount;
        particleSystem->textureFrameWidth = 1.0f / textureColumnCount;
        particleSystem->textureFrameHeight = 1.0f / textureRowCount;
    }
    else {
        textureColumnCount = 1;
        particleSystem->textureFrameCount = 1;
        particleSystem->textureFrameWidth = 1.0f;
        particleSystem->textureFrameHeight = 1.0f;
    }
    particleSystem->textureFrameCoordinates = HuMemDirectMallocNum(HEAP_MODEL, particleSystem->textureFrameCount * sizeof(Vec2f), particleModel->mallocNo);
    TextureFrameGridGenerate(particleSystem->textureFrameCoordinates, particleSystem->textureFrameCount, textureColumnCount, particleSystem->textureFrameWidth, particleSystem->textureFrameHeight);
    particleSystem->quadVertices.x = particleSystem->lowerLeftX = particleSystem->upperLeftX = particleSystem->lowerLeftY = -quadHalfSize;
    particleSystem->rightEdge = quadHalfSize;
    particleSystem->quadVertices.y = quadHalfSize;
    particleSystem->leftEdge = quadHalfSize;
    particleSystem->topEdge = quadHalfSize;
    particleSystem->quadVertices.z = particleSystem->bottomEdge = particleSystem->upperLeftY = particleSystem->edgeOffset = 0.0f;
    displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, 0x20000, particleModel->mallocNo);
    displayListBuildBuffer = displayListBuffer;
    GXBeginDisplayList(displayListBuildBuffer, 0x20000);
    GXBegin(GX_QUADS, GX_VTXFMT0, particleCapacity * 4);
    for (particleIndex = 0; particleIndex < particleCapacity; particleIndex++) {
        // TODO is this right?
        GXPosition1x16(particleIndex * 4);
        GXPosition1x16(particleIndex);
        GXPosition1x16(particleIndex * 4);

        GXPosition1x16(particleIndex * 4 + 1);
        GXPosition1x16(particleIndex);
        GXPosition1x16(particleIndex * 4 + 1);

        GXPosition1x16(particleIndex * 4 + 2);
        GXPosition1x16(particleIndex);
        GXPosition1x16(particleIndex * 4 + 2);

        GXPosition1x16(particleIndex * 4 + 3);
        GXPosition1x16(particleIndex);
        GXPosition1x16(particleIndex * 4 + 3);
    }
    particleSystem->displayListSize = GXEndDisplayList();
    DCFlushRangeNoSync(displayListBuffer, particleSystem->displayListSize);
    particleSystem->displayList = HuMemDirectMallocNum(HEAP_MODEL, particleSystem->displayListSize, particleModel->mallocNo);
    memcpy(particleSystem->displayList, displayListBuffer, particleSystem->displayListSize);
    DCFlushRangeNoSync(particleSystem->displayList, particleSystem->displayListSize);
    HuMemDirectFree(displayListBuffer);
    return modelId;
}

ParticleState *ParticleSpawn(s16 modelId, Vec *position, GXColor *color, float scale, float rotation)
{
    ParticleState *particle;
    ParticleSystemState *particleSystem;
    s16 particleIndex;
    HU3DMODEL *particleModel;

    particleModel = &Hu3DData[modelId];
    particleSystem = particleModel->hookData;
    if (particleSystem->destroyRequested != 0) {
        return NULL;
    }

    for (particleIndex = 0, particle = particleSystem->particles; particleIndex < particleSystem->particleCapacity; particleIndex++, particle++) {
        if (particle->particleIndex < 0) {
            break;
        }
    }
    if (particleIndex == particleSystem->particleCapacity) {
        return NULL;
    }
    particleSystem->vertexColors[particleIndex] = *color;
    particle->position = *position;
    particle->scale = scale;
    particle->rotationAngle = rotation;
    particle->textureFrame = 0;
    particle->animationState = 0;
    particle->particleIndex = particleIndex;
    return particle;
}

void ParticleDrawHookSet(s16 modelId, ParticleDrawHook drawHook)
{
    HU3DMODEL *particleModel = &Hu3DData[modelId];
    ParticleSystemState *particleSystem = particleModel->hookData;
    particleSystem->drawHook = drawHook;
}

void ParticleBlendModeSet(s16 modelId, u8 blendMode)
{
    HU3DMODEL *particleModel = &Hu3DData[modelId];
    ((ParticleSystemState *)particleModel->hookData)->blendMode = blendMode;
}

void ParticleSystemDestroyRequest(s16 modelId)
{
    HU3DMODEL *particleModel = &Hu3DData[modelId];
    ParticleSystemState *particleSystem = particleModel->hookData;
    particleSystem->destroyRequested = 1;
}

s32 ParticleSystemDestroyIfFinished(s16 modelId)
{
    HU3DMODEL *particleModel = &Hu3DData[modelId];
    ParticleSystemState *particleSystem = particleModel->hookData;

    if ((particleSystem->destroyRequested != 0) && (particleSystem->activeParticleCount == 0)) {
        HuMemDirectFree(particleSystem->particles);
        HuMemDirectFree(particleSystem->vertexPositions);
        HuMemDirectFree(particleSystem->vertexColors);
        HuMemDirectFree(particleSystem->textureCoordinates);
        HuMemDirectFree(particleSystem->textureFrameCoordinates);
        HuMemDirectFree(particleSystem->displayList);
        HuSprAnimKill(particleSystem->spriteAnimation);
        HuMemDirectFree(particleModel->hookData);
        Hu3DModelKill(modelId);
        return 1;
    }
    return 0;
}

void FireParticleBurstCreate(s16 modelId, s16 particleCount, Vec *origin, float startAngle, float lifetime, float targetHeight)
{
    ParticleState *particle;
    s16 particleIndex;

    GXColor particleColor = { 0xFF, 0xFF, 0xFF, 0xFF };
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++) {
        particle = ParticleSpawn(modelId, origin, &particleColor, 0.0f, 0.0f);
        if (!particle) {
            break;
        }
        particle->originPosition = *origin;
        particle->age = 0.0f;
        particle->startAngle = startAngle;
        particle->ageStep = 5.0f;
        particle->lifetime = lifetime;
        particle->targetHeight = targetHeight;
        particle->angleOffset = (20.0f * frandf()) - 10.0f;
        particle->ageStep = particle->ageStep * (0.8f + (0.4f * frandf()));
        particle->lifetime = particle->lifetime * (0.9f + (0.3f * frandf()));
    }
}

s16 FireParticleSystemCreate(Vec *positionOffset, float *scaleOffset, s16 particleCapacity)
{
    s16 particleModelId;

    particleModelId = ParticleSystemCreate(lbl1Bss14, positionOffset, scaleOffset, particleCapacity, 80.0f, 0x40, 0x40);
    ParticleDrawHookSet(particleModelId, PlayerFireParticleUpdate);
    ParticleBlendModeSet(particleModelId, 1);
    Hu3DModelLayerSet(particleModelId, 5);
    return particleModelId;
}

float SineInterpolate(float startValue, float endValue, float elapsedTime, float duration)
{
    if (elapsedTime >= duration) {
        return endValue;
    }
    return startValue + ((endValue - startValue) * sind(((90.0f / duration) * elapsedTime)));
}

float CosineInterpolate(float startValue, float endValue, float elapsedTime, float duration)
{
    if (elapsedTime >= duration) {
        return endValue;
    }
    return startValue + ((endValue - startValue) * (1.0 - cosd(((90.0f / duration) * elapsedTime))));
}

void MessageWindowInit(s32 messageCount)
{
    lbl1Bss10 = 0;
    lbl1BssC = messageCount;
    HuWinInit(1);
}

void MessageProcessStart(void (*messageProcess)(), s32 messageId)
{
    if ((messageId < 0) || (MinigameTypeIsAvailable(messageId) == 0)) {
        lbl1Bss10++;
        HuPrcChildCreate(messageProcess, 0x64, 0x2000, 0, HuPrcCurrentGet());
    }
}

void MessageProcessEnd(void)
{
    lbl1Bss10--;
    HuPrcEnd();
}

void MessageWindowSizeAccumulate(float *maximumSize, s32 messageId)
{
    float measuredMessageSize[2];

    HuWinMesMaxSizeGet(1, measuredMessageSize, messageId);
    if (measuredMessageSize[0] > maximumSize[0]) {
        maximumSize[0] = measuredMessageSize[0];
    }
    if (measuredMessageSize[1] > maximumSize[1]) {
        maximumSize[1] = measuredMessageSize[1];
    }
}

GXColor lbl1Data9F[3] = {
    { 0xFF, 0xFF, 0xCC, 0xE6 },
    { 0xCC, 0xFF, 0xFF, 0xE6 },
    { 0xFF, 0xCC, 0xFF, 0xE6 },
};

s32 lbl1DataAC[2] = { DATA_MAKE_NUM(DATADIR_M450, 0x2C), DATA_MAKE_NUM(DATADIR_M450, 0x2D) };

s32 lbl1DataB4[0x13] = {
    DATA_MAKE_NUM(DATADIR_M450, 0x2E),
    DATA_MAKE_NUM(DATADIR_M450, 0x2F),
    DATA_MAKE_NUM(DATADIR_M450, 0x30),
    DATA_MAKE_NUM(DATADIR_M450, 0x31),
    DATA_MAKE_NUM(DATADIR_M450, 0x32),
    DATA_MAKE_NUM(DATADIR_M450, 0x33),
    DATA_MAKE_NUM(DATADIR_M450, 0x34),
    DATA_MAKE_NUM(DATADIR_M450, 0x35),
    DATA_MAKE_NUM(DATADIR_M450, 0x36),
    DATA_MAKE_NUM(DATADIR_M450, 0x37),
    DATA_MAKE_NUM(DATADIR_M450, 0x38),
    DATA_MAKE_NUM(DATADIR_M450, 0x39),
    DATA_MAKE_NUM(DATADIR_M450, 0x3A),
    DATA_MAKE_NUM(DATADIR_M450, 0x3B),
    DATA_MAKE_NUM(DATADIR_M450, 0x3C),
    DATA_MAKE_NUM(DATADIR_M450, 0x3D),
    DATA_MAKE_NUM(DATADIR_M450, 0x3E),
    DATA_MAKE_NUM(DATADIR_M450, 0x3F),
    DATA_MAKE_NUM(DATADIR_M450, 0x40),
};

s32 lbl1Data100[5] = {
    DATA_MAKE_NUM(DATADIR_M450, 0x41),
    DATA_MAKE_NUM(DATADIR_M450, 0x42),
    DATA_MAKE_NUM(DATADIR_M450, 0x43),
    DATA_MAKE_NUM(DATADIR_M450, 0x44),
    DATA_MAKE_NUM(DATADIR_M450, 0x45),
};
s32 lbl1Data114[0xF] = {
    DATA_MAKE_NUM(DATADIR_M450, 0x46),
    DATA_MAKE_NUM(DATADIR_M450, 0x47),
    DATA_MAKE_NUM(DATADIR_M450, 0x47),
    DATA_MAKE_NUM(DATADIR_M450, 0x49),
    DATA_MAKE_NUM(DATADIR_M450, 0x4A),
    DATA_MAKE_NUM(DATADIR_M450, 0x4B),
    DATA_MAKE_NUM(DATADIR_M450, 0x4C),
    DATA_MAKE_NUM(DATADIR_M450, 0x4D),
    DATA_MAKE_NUM(DATADIR_M450, 0x4E),
    DATA_MAKE_NUM(DATADIR_M450, 0x4F),
    DATA_MAKE_NUM(DATADIR_M450, 0x50),
    DATA_MAKE_NUM(DATADIR_M450, 0x51),
    DATA_MAKE_NUM(DATADIR_M450, 0x52),
    DATA_MAKE_NUM(DATADIR_M450, 0x53),
    DATA_MAKE_NUM(DATADIR_M450, 0x54),
};
s32 lbl1Data150[0xF] = {
    DATA_MAKE_NUM(DATADIR_M450, 0x55),
    DATA_MAKE_NUM(DATADIR_M450, 0x56),
    DATA_MAKE_NUM(DATADIR_M450, 0x56),
    DATA_MAKE_NUM(DATADIR_M450, 0x58),
    DATA_MAKE_NUM(DATADIR_M450, 0x59),
    DATA_MAKE_NUM(DATADIR_M450, 0x5A),
    DATA_MAKE_NUM(DATADIR_M450, 0x5B),
    DATA_MAKE_NUM(DATADIR_M450, 0x55),
    DATA_MAKE_NUM(DATADIR_M450, 0x5C),
    DATA_MAKE_NUM(DATADIR_M450, 0x5D),
    DATA_MAKE_NUM(DATADIR_M450, 0x5E),
    DATA_MAKE_NUM(DATADIR_M450, 0x5F),
    DATA_MAKE_NUM(DATADIR_M450, 0x60),
    DATA_MAKE_NUM(DATADIR_M450, 0x61),
    DATA_MAKE_NUM(DATADIR_M450, 0x62),
};
s32 lbl1Data18C[8] = { 0, 1, 2, 1, 1, -1, -1, -1 };
s32 lbl1Data1AC[8] = { 0, 1, 2, 1, 1, -1, -1, -1 };
s32 lbl1Data1CC[4] = { 0x3E, 0x65, 0x3F, 0x66 };
CollisionProfile lbl1Data1DC[5] = {
    { 8.5f * REFRESH_RATE, 0 },
    { 5.0f * REFRESH_RATE, 1 },
    { 5.0f * REFRESH_RATE, 0 },
    { 5.0f * REFRESH_RATE, 1 },
    { 0.0f, 0 },
};
CollisionProfile lbl1Data204[6] = {
    { 3.5f * REFRESH_RATE, 0 },
    { 5.0f * REFRESH_RATE, 1 },
    { 5.0f * REFRESH_RATE, 0 },
    { 5.0f * REFRESH_RATE, 1 },
    { 5.0f * REFRESH_RATE, 0 },
    { 0.0f, 0 },
};
Vec2f lbl1Data234[4] = {
    { 5.0f * REFRESH_RATE, 5.0f * REFRESH_RATE },
    { 5.0f * REFRESH_RATE, 4.0f * REFRESH_RATE },
    { 5.0f * REFRESH_RATE, 3.5f * REFRESH_RATE },
    { 5.0f * REFRESH_RATE, 3.0f * REFRESH_RATE },
};
float lbl1Data254[5] = { 0.4f, 0.35f, 0.3f, 0.25f, 0.2f };
float lbl1Data268[5] = { 0.45f, 0.4f, 0.35f, 0.3f, 0.25f };
float lbl1Data27C[5] = { 0.5f, 0.45f, 0.4f, 0.35f, 0.3f };
float lbl1Data290[5] = { 0.55f, 0.5f, 0.45f, 0.4f, 0.35f };

s32 MessageWindowCreateAnimated(float left, float top, float width, float height, s32 colorIndex)
{
    float animatedScale[2];

    float centerX;
    float centerY;
    float initialScaleX;
    float initialScaleY;
    float animatedX;
    float animatedY;

    s32 animationStep;
    WindowData *windowData;
    GXColor *windowColor;
    s32 windowId;

    if (555.0f < (left + width)) {
        left = 555.0f - width;
    }
    if (21.0f > left) {
        left = 21.0f;
    }
    lbl1Bss10++;
    windowId = HuWinCreate(left, top, width, height, 1);
    windowData = &winData[windowId];
    windowColor = &lbl1Data9F[colorIndex];
    HuWinBGTPLvlSet(windowId, windowColor->a / 255.0f);
    HuSprColorSet(windowData->group, 0, windowColor->r, windowColor->g, windowColor->b);
    windowData->mess_color = 0;
    windowData->mess_shadow_color = 9;
    windowData->attr |= 0x80;
    windowData->active_pad = 0;
    centerX = left + (0.5f * width);
    initialScaleX = 4.0f / width;
    if (((4.0f + centerX) - (left + width)) >= 0.0f) {
        centerX = (left + width) - 4.0f;
    }
    else if ((left - (centerX - 2.0f)) >= 0.0f) {
        centerX = left;
    }
    else {
        centerX -= 2.0f;
    }
    centerY = top + (0.5f * height);
    initialScaleY = 4.0f / height;
    if (((4.0f + centerY) - (top + height)) >= 0.0f) {
        centerY = (top + height) - 4.0f;
    }
    else if ((top - (centerY - 2.0f)) >= 0.0f) {
        centerY = top;
    }
    else {
        centerY -= 2.0f;
    }
    for (animationStep = 0; animationStep < 0xA; animationStep++) {
        animatedX = SineInterpolate(centerX, left, animationStep, 10.0f);
        animatedScale[1] = SineInterpolate(initialScaleX, 1.0f, animationStep, 10.0f);
        animatedY = SineInterpolate(centerY, top, animationStep, 10.0f);
        animatedScale[0] = SineInterpolate(initialScaleY, 1.0f, animationStep, 10.0f);
        HuWinPosSet(windowId, animatedX, animatedY);
        HuWinScaleSet(windowId, animatedScale[1], animatedScale[0]);
        HuPrcVSleep();
    }
    HuWinPosSet(windowId, left, top);
    HuWinScaleSet(windowId, 1.0f, 1.0f);
    HuPrcVSleep();
    return windowId;
}

void MessageWindowCloseAnimated(s32 windowId)
{
    float collapsedScale[2];
    float collapseTargetX;
    float collapseTargetY;
    float windowLeft;
    float windowTop;
    float windowWidth;
    float windowHeight;
    float collapsedScaleX;
    float collapsedScaleY;
    float animatedX;
    float animatedY;

    s32 animationStep;
    WindowData *windowData;

    HuWinMesSet(windowId, 0x1D0016);
    HuPrcVSleep();
    windowData = &winData[windowId];
    windowLeft = windowData->pos_x;
    windowTop = windowData->pos_y;
    windowWidth = windowData->w;
    windowHeight = windowData->h;
    collapseTargetX = windowLeft + (0.5f * windowWidth);
    collapsedScaleX = 4.0f / windowWidth;
    if (((4.0f + collapseTargetX) - (windowLeft + windowWidth)) >= 0.0f) {
        collapseTargetX = (windowLeft + windowWidth) - 4.0f;
    }
    else if ((windowLeft - (collapseTargetX - 2.0f)) >= 0.0f) {
        collapseTargetX = windowLeft;
    }
    else {
        collapseTargetX -= 2.0f;
    }
    collapseTargetY = windowTop + (0.5f * windowHeight);
    collapsedScaleY = 4.0f / windowHeight;
    if (((4.0f + collapseTargetY) - (windowTop + windowHeight)) >= 0.0f) {
        collapseTargetY = (windowTop + windowHeight) - 4.0f;
    }
    else if ((windowTop - (collapseTargetY - 2.0f)) >= 0.0f) {
        collapseTargetY = windowTop;
    }
    else {
        collapseTargetY -= 2.0f;
    }
    for (animationStep = 0; animationStep < 0xA; animationStep++) {
        animatedX = CosineInterpolate(windowLeft, collapseTargetX, animationStep, 10.0f);
        collapsedScale[1] = CosineInterpolate(1.0f, collapsedScaleX, animationStep, 10.0f);
        animatedY = CosineInterpolate(windowTop, collapseTargetY, animationStep, 10.0f);
        collapsedScale[0] = CosineInterpolate(1.0f, collapsedScaleY, animationStep, 10.0f);
        HuWinPosSet(windowId, animatedX, animatedY);
        HuWinScaleSet(windowId, collapsedScale[1], collapsedScale[0]);
        HuPrcVSleep();
    }
    HuWinPosSet(windowId, collapseTargetX, collapseTargetY);
    HuWinScaleSet(windowId, collapsedScaleX, collapsedScaleY);
    HuPrcVSleep();
    HuWinKill(windowId);
    lbl1Bss10--;
}

void MessageWindowTextSet(s32 windowId, u32 messageId)
{
    winData[windowId].mess_color = 0;
    winData[windowId].mess_shadow_color = 9;
    winData[windowId].attr |= 0x80;
    HuWinMesSet(windowId, messageId);
}

void MessageWindowWait(s32 windowId, s32 waitFrames)
{
    HuWinComKeyReset();
    if (waitFrames > 0) {
        HuPrcSleep(waitFrames);
    }
    else {
        winData[windowId].active_pad = 1 << lbl1BssC;
        HuWinMesWait(windowId);
    }
    HuWinComKeyReset();
}

void MessageWindowChoiceWait(s32 windowId)
{
    s32 choiceIndex;

    HuWinComKeyReset();
    winData[windowId].active_pad = 1 << lbl1BssC;
    choiceIndex = HuWinChoiceGet(windowId, 1);
    if ((choiceIndex == -1) || (choiceIndex == 1)) {
        lbl1Bss8 = 0;
    }
    else {
        lbl1Bss8 = 1;
    }
    HuWinComKeyReset();
}

void ArenaOpponentMessageProcess(void)
{
    Vec messageAnchorPosition;
    float messageWindowLeft;
    float soundPan;
    float minimumWindowLeft;

    s32 messageWindowId;

    float messageWindowSize[2] = { 0.0f, 0.0f };
    s32 messageAndSoundIds[22] = {
        0x002D0003,
        0x0000003E,
        0x002D0009,
        0x0000003E,
        0x002D000B,
        0x0000003E,
        0x002D000D,
        0x0000003E,
        0x002D000F,
        0x0000003E,
        0x002D0011,
        0x0000003E,
        0x002D0013,
        0x0000003E,
        0x002D0015,
        0x0000003E,
        0x002D0017,
        0x0000003E,
        0x002D0019,
        0x0000003E,
        0x002D001B,
        0x0000003F,
    };
    MessageWindowSizeAccumulate(messageWindowSize, messageAndSoundIds[lbl1Bss4 * 2]);
    messageAnchorPosition = lbl1Bss34[0];
    messageAnchorPosition.y += 200.0f;
    Hu3D3Dto2D(&messageAnchorPosition, 1, &messageAnchorPosition);
    if (messageAndSoundIds[(lbl1Bss4 * 2) + 1] >= 0) {
        soundPan = 32.0f + (64.0f * (messageAnchorPosition.x / 576.0f));
        if (32.0f > soundPan) {
            soundPan = 32.0f;
        }
        if (96.0f < soundPan) {
            soundPan = 96.0f;
        }
        HuAudFXPanning(ArenaOpponentImpactSoundPlay(0, messageAndSoundIds[(lbl1Bss4 * 2) + 1]), (s32)soundPan);
    }
    messageWindowLeft = messageAnchorPosition.x - (messageWindowSize[0] / 2);
    minimumWindowLeft = 72.0f;
    if (lbl1Bss4 >= 5) {
        if (minimumWindowLeft > messageWindowLeft) {
            messageWindowLeft = minimumWindowLeft;
        }
        if ((576.0f - minimumWindowLeft) < (messageWindowLeft + messageWindowSize[0])) {
            messageWindowLeft = (576.0f - minimumWindowLeft) - messageWindowSize[0];
        }
    }
    messageWindowId = MessageWindowCreateAnimated(messageWindowLeft, messageAnchorPosition.y - messageWindowSize[1], messageWindowSize[0], messageWindowSize[1], 0);
    MessageWindowTextSet(messageWindowId, messageAndSoundIds[lbl1Bss4 * 2]);
    MessageWindowWait(messageWindowId, lbl1Bss0);
    MessageWindowCloseAnimated(messageWindowId);
    MessageProcessEnd();
}

void ArenaPlayerMessageProcess(void)
{
    Vec messageAnchorPosition;

    float messageWindowLeft;
    float soundPan;
    float minimumWindowLeft;

    s32 messageWindowId;

    float messageWindowSize[2] = { 0.0f, 0.0f };
    s32 messageAndSoundIds[22] = {
        0x002D0002,
        0x0000003E,
        0x002D0008,
        0x0000003E,
        0x002D000A,
        0x0000003E,
        0x002D000C,
        0x0000003E,
        0x002D000E,
        0x0000003E,
        0x002D0010,
        0x0000003E,
        0x002D0012,
        0x0000003E,
        0x002D0014,
        0x0000003E,
        0x002D0016,
        0x0000003E,
        0x002D0018,
        0x0000003F,
        0x002D001A,
        0x0000003F,
    };
    MessageWindowSizeAccumulate(messageWindowSize, messageAndSoundIds[lbl1Bss4 * 2]);
    messageAnchorPosition = lbl1Bss34[1];
    messageAnchorPosition.y += 200.0f;
    Hu3D3Dto2D(&messageAnchorPosition, 1, &messageAnchorPosition);
    if (messageAndSoundIds[(lbl1Bss4 * 2) + 1] >= 0) {
        soundPan = 32.0f + (64.0f * (messageAnchorPosition.x / 576.0f));
        if (32.0f > soundPan) {
            soundPan = 32.0f;
        }
        if (96.0f < soundPan) {
            soundPan = 96.0f;
        }
        HuAudFXPanning(ArenaOpponentImpactSoundPlay(1, messageAndSoundIds[(lbl1Bss4 * 2) + 1]), (s32)soundPan);
    }
    messageWindowLeft = messageAnchorPosition.x - (messageWindowSize[0] / 2);
    minimumWindowLeft = 72.0f;
    if (lbl1Bss4 >= 5) {
        if (minimumWindowLeft > messageWindowLeft) {
            messageWindowLeft = minimumWindowLeft;
        }
        if ((576.0f - minimumWindowLeft) < (messageWindowLeft + messageWindowSize[0])) {
            messageWindowLeft = (576.0f - minimumWindowLeft) - messageWindowSize[0];
        }
    }
    messageWindowId = MessageWindowCreateAnimated(messageWindowLeft, messageAnchorPosition.y - messageWindowSize[1], messageWindowSize[0], messageWindowSize[1], 1);
    MessageWindowTextSet(messageWindowId, messageAndSoundIds[lbl1Bss4 * 2]);
    MessageWindowWait(messageWindowId, lbl1Bss0);
    MessageWindowCloseAnimated(messageWindowId);
    MessageProcessEnd();
}

void ArenaIntroSequence(void)
{
    s32 messageWindowId;

    float messageWindowSize[2] = { 0.0f, 0.0f };
    MessageWindowSizeAccumulate(messageWindowSize, 0x2D0001);
    MessageWindowSizeAccumulate(messageWindowSize, 0x2D0004);
    MessageWindowSizeAccumulate(messageWindowSize, 0x2D0005);
    MessageWindowSizeAccumulate(messageWindowSize, 0x2D0006);
    MessageWindowSizeAccumulate(messageWindowSize, 0x2D0007);
    messageWindowId = MessageWindowCreateAnimated((576.0f - messageWindowSize[0]) / 2, 48.0f, messageWindowSize[0], messageWindowSize[1], 2);
    ArenaOpponentMotionSet(3, 8.0f, 0, 0, 0.0f);
    MessageWindowTextSet(messageWindowId, 0x2D0001);
    MessageWindowWait(messageWindowId, -1);
    ArenaOpponentAnimationSet(0, 8, 8.0f, 0, 7, 0.0f);
    ArenaOpponentAnimationSet(1, 8, 8.0f, 0, 7, 0.0f);
    MessageProcessStart(ArenaOpponentMessageProcess, -1);
    MessageProcessStart(ArenaPlayerMessageProcess, -1);

    while (lbl1Bss10 > 2) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x3B);
    ArenaOpponentMotionSet(4, 8.0f, 0, 0, REFRESH_RATE);
    ArenaOpponentAnimationSet(0, 9, 8.0f, 0, 7, REFRESH_RATE);
    ArenaOpponentAnimationSet(1, 9, 8.0f, 0, 7, REFRESH_RATE);
    MessageWindowTextSet(messageWindowId, 0x2D0004);
    MessageWindowWait(messageWindowId, -1);
    HuAudFXPlay(0x3C);
    MessageWindowTextSet(messageWindowId, 0x2D0005);
    MessageWindowWait(messageWindowId, -1);
    MessageWindowTextSet(messageWindowId, 0x2D0006);
    MessageWindowWait(messageWindowId, -1);
    HuAudFXPlay(0x3A);
    MessageWindowTextSet(messageWindowId, 0x2D0007);
    MessageWindowWait(messageWindowId, -1);
    MessageWindowCloseAnimated(messageWindowId);
    MessageProcessEnd();
}

void ArenaTournamentSequenceA(void)
{
    lbl1Bss4 = 2;
    lbl1Bss0 = -1;
    ArenaOpponentAnimationSet(1, 0xD, 8.0f, 0x40000001, -1, 0.0f);
    MessageProcessStart(ArenaPlayerMessageProcess, -1);
    while (lbl1Bss10 > 1) HuPrcVSleep();
    ArenaOpponentAnimationSet(1, 0, 8.0f, 0x40000001, -1, 0.0f);
    ArenaOpponentAnimationSet(0, 0xD, 8.0f, 0x40000001, -1, 0.0f);
    MessageProcessStart(ArenaOpponentMessageProcess, -1);
    while (lbl1Bss10 > 1) HuPrcVSleep();
    ArenaOpponentAnimationSet(0, 0, 8.0f, 0x40000001, -1, 0.0f);
    lbl1Bss4 = 3;
    lbl1Bss0 = -1;
    ArenaOpponentAnimationSet(1, 0xD, 8.0f, 0x40000001, -1, 0.0f);
    MessageProcessStart(ArenaPlayerMessageProcess, -1);
    while (lbl1Bss10 > 1) HuPrcVSleep();
    ArenaOpponentAnimationSet(1, 0, 8.0f, 0x40000001, -1, 0.0f);
    ArenaOpponentAnimationSet(0, 0xD, 8.0f, 0x40000001, -1, 0.0f);
    MessageProcessStart(ArenaOpponentMessageProcess, -1);
    while (lbl1Bss10 > 1) HuPrcVSleep();
    HuPrcSleep(0x1E);
    AudioSequenceFadeOut(0x3E8);
    lbl1Bss4 = 4;
    lbl1Bss0 = 0x1E;
    ArenaOpponentAnimationSet(0, 0xC, 8.0f, 0x40000001, -1, 0.0f);
    ArenaOpponentAnimationSet(1, 0xC, 8.0f, 0x40000001, -1, 0.0f);
    MessageProcessStart(ArenaOpponentMessageProcess, -1);
    MessageProcessStart(ArenaPlayerMessageProcess, -1);
    while (lbl1Bss10 > 1) HuPrcVSleep();
    MessageProcessEnd();
}

void ArenaTournamentSequenceB(void)
{
    lbl1Bss4 = 5;
    lbl1Bss0 = 0x1E;
    MessageProcessStart(ArenaOpponentMessageProcess, -1);
    MessageProcessStart(ArenaPlayerMessageProcess, -1);
    while (lbl1Bss10 > 1) HuPrcVSleep();
    lbl1Bss4 = 6;
    lbl1Bss0 = -1;
    ArenaOpponentAnimationSet(1, 0xD, 8.0f, 0x40000001, -1, 0.0f);
    MessageProcessStart(ArenaPlayerMessageProcess, -1);
    while (lbl1Bss10 > 1) HuPrcVSleep();
    ArenaOpponentAnimationSet(1, 0, 8.0f, 0x40000001, -1, 0.0f);
    ArenaOpponentAnimationSet(0, 0xD, 8.0f, 0x40000001, -1, 0.0f);
    MessageProcessStart(ArenaOpponentMessageProcess, -1);
    while (lbl1Bss10 > 1) HuPrcVSleep();
    ArenaOpponentAnimationSet(0, 0, 8.0f, 0x40000001, -1, 0.0f);
    lbl1Bss4 = 7;
    lbl1Bss0 = -1;
    ArenaOpponentAnimationSet(1, 0xD, 8.0f, 0x40000001, -1, 0.0f);
    MessageProcessStart(ArenaPlayerMessageProcess, -1);
    while (lbl1Bss10 > 1) HuPrcVSleep();
    ArenaOpponentAnimationSet(1, 0, 8.0f, 0x40000001, -1, 0.0f);
    ArenaOpponentAnimationSet(0, 0xD, 8.0f, 0x40000001, -1, 0.0f);
    MessageProcessStart(ArenaOpponentMessageProcess, -1);
    while (lbl1Bss10 > 1) HuPrcVSleep();
    ArenaOpponentAnimationSet(0, 0, 8.0f, 0x40000001, -1, 0.0f);
    lbl1Bss4 = 8;
    lbl1Bss0 = -1;
    ArenaOpponentAnimationSet(1, 0xE, 8.0f, 0, -1, 0.0f);
    ArenaOpponentAnimationSet(1, 0xE, 8.0f, 0, -1, 0.0f);
    MessageProcessStart(ArenaPlayerMessageProcess, -1);
    while (lbl1Bss10 > 1) HuPrcVSleep();
    ArenaOpponentAnimationSet(1, 0, 8.0f, 0x40000001, -1, 0.0f);
    ArenaOpponentAnimationSet(0, 0xE, 8.0f, 0, -1, 0.0f);
    MessageProcessStart(ArenaOpponentMessageProcess, -1);
    while (lbl1Bss10 > 1) HuPrcVSleep();
    ArenaOpponentAnimationSet(0, 0, 8.0f, 0x40000001, -1, 0.0f);
    lbl1Bss4 = 9;
    lbl1Bss0 = 0x1E;
    MessageProcessStart(ArenaOpponentMessageProcess, -1);
    MessageProcessStart(ArenaPlayerMessageProcess, -1);
    while (lbl1Bss10 > 1) {
        HuPrcVSleep();
    }
    MessageProcessEnd();
}

void ArenaFinalRoundSequence(void)
{
    ArenaOpponentAnimationSet(1, 4, 8.0f, 0, -1, 0.0f);
    MessageProcessStart(ArenaPlayerMessageProcess, -1);
    ArenaOpponentAnimationSet(0, 4, 8.0f, 0, -1, 0.0f);
    MessageProcessStart(ArenaOpponentMessageProcess, -1);
    while (lbl1Bss10 > 1) {
        HuPrcVSleep();
    }
    MessageProcessEnd();
}

void ArenaPlayerScoreDialogue(void)
{
    s32 messageWindowId;

    float messageWindowSize[2] = { 0.0f, 0.0f };
    MessageWindowSizeAccumulate(messageWindowSize, 0x2D001C);
    MessageWindowSizeAccumulate(messageWindowSize, 0x2D001D);
    MessageWindowSizeAccumulate(messageWindowSize, 0x2D001E);
    messageWindowId = MessageWindowCreateAnimated((576.0f - messageWindowSize[0]) / 2, 132.0f, messageWindowSize[0], messageWindowSize[1], 2);
    ArenaOpponentMotionSet(8, 8.0f, 0, 7, 0.0f);
    HuAudFXPlay(0x3C);
    MessageWindowTextSet(messageWindowId, 0x2D001C);
    MessageWindowWait(messageWindowId, -1);
    MessageWindowTextSet(messageWindowId, 0x2D001D);
    MessageWindowWait(messageWindowId, -1);
    HuAudFXPlay(0x3C);
    MessageWindowTextSet(messageWindowId, 0x2D001E);
    MessageWindowWait(messageWindowId, -1);
    MessageWindowCloseAnimated(messageWindowId);
    MessageProcessEnd();
}

void ArenaFinalScoreDialogue(void)
{
    s32 messageWindowId;
    float messageWindowSize[2] = { 0.0f, 0.0f };

    MessageWindowSizeAccumulate(messageWindowSize, 0x2D001F);
    MessageWindowSizeAccumulate(messageWindowSize, 0x2D0020);
    MessageWindowSizeAccumulate(messageWindowSize, 0x2D0021);
    messageWindowId = MessageWindowCreateAnimated((576.0f - messageWindowSize[0]) / 2, 48.0f, messageWindowSize[0], messageWindowSize[1], 2);
    HuAudFXPlay(0x3C);
    MessageWindowTextSet(messageWindowId, 0x2D0020);
    ArenaOpponentMotionSet(5, 8.0f, 0, 6, 0.0f);
    MessageWindowTextSet(messageWindowId, 0x2D0020);
    MessageWindowWait(messageWindowId, -1);
    HuAudFXPlay(0x3C);
    MessageWindowTextSet(messageWindowId, 0x2D0021);
    ArenaOpponentMotionSet(0, 8.0f, 0, -1, 0.0f);
    MessageWindowCloseAnimated(messageWindowId);
    MessageProcessEnd();
}

void ArenaOpponentArcMove(ArenaOpponentState *opponent, s32 modelId, Vec *arcCenter, s8 arcAxis)
{
    float arcRadius;
    float absoluteCenterX;
    float absoluteCenterY;
    float selectedCenterY;
    float absoluteCenterZ;
    float selectedCenterZ;
    float jumpTargetDistance;
    float absoluteJumpTargetDistance;

    s32 crossedArcStart;
    s32 xDirection;
    s32 zDirection;

    if (90.0f <= opponent->rotationProgress) {
        opponent->playerWork.worldPosition.y -= 10.0f;
        return;
    }
    crossedArcStart = 1;
    if (arcAxis == 0x78) {
        if (arcCenter->x > opponent->playerWork.worldPosition.x) {
            crossedArcStart = 0;
        }
    }
    else if (arcCenter->z < opponent->playerWork.worldPosition.z) {
        crossedArcStart = 0;
    }
    if (crossedArcStart != 0) {
        opponent->rotationProgress += 5.0f;
        if (0.0f > arcCenter->x) {
            absoluteCenterX = -arcCenter->x;
        }
        else {
            absoluteCenterX = arcCenter->x;
        }
        arcRadius = absoluteCenterX;
        if (0.0f > arcCenter->y) {
            absoluteCenterY = -arcCenter->y;
        }
        else {
            absoluteCenterY = arcCenter->y;
        }
        if (arcRadius < absoluteCenterY) {
            if (0.0f > arcCenter->y) {
                selectedCenterY = -arcCenter->y;
            }
            else {
                selectedCenterY = arcCenter->y;
            }
            arcRadius = selectedCenterY;
        }
        if (0.0f > arcCenter->z) {
            absoluteCenterZ = -arcCenter->z;
        }
        else {
            absoluteCenterZ = arcCenter->z;
        }
        if (arcRadius < absoluteCenterZ) {
            if (0.0f > arcCenter->z) {
                selectedCenterZ = -arcCenter->z;
            }
            else {
                selectedCenterZ = arcCenter->z;
            }
            arcRadius = selectedCenterZ;
        }
        arcRadius = 1500.0f - arcRadius;
        if (arcAxis == 0x78) {
            if (0.0f > arcCenter->x) {
                xDirection = -1;
            }
            else {
                xDirection = 1;
            }
            opponent->playerWork.worldPosition.x = arcCenter->x + ((arcRadius * xDirection) * sind(opponent->rotationProgress));
        }
        else {
            if (0.0f > arcCenter->z) {
                zDirection = -1;
            }
            else {
                zDirection = 1;
            }
            opponent->playerWork.worldPosition.z = arcCenter->z + ((arcRadius * zDirection) * sind(opponent->rotationProgress));
        }
        MTXRotRad(Hu3DData[modelId].mtx, 0x58, 0.017453292f * opponent->rotationProgress);
        opponent->playerWork.worldPosition.y = arcCenter->y + (arcRadius * cosd(opponent->rotationProgress));
        return;
    }
    if (arcAxis == 0x78) {
        opponent->playerWork.inputX = 0x48;
    }
    else {
        opponent->playerWork.inputZ = 0x48;
    }
    if (opponent->stateFlags2 & 0x200) {
        jumpTargetDistance = opponent->jumpTimer - opponent->playerWork.worldPosition.x;
        if (0.0f > jumpTargetDistance) {
            absoluteJumpTargetDistance = -jumpTargetDistance;
        }
        else {
            absoluteJumpTargetDistance = jumpTargetDistance;
        }
        if (10.0f > absoluteJumpTargetDistance) {
            opponent->stateFlags2 &= ~0x200;
        }
        if (0.0f <= jumpTargetDistance) {
            opponent->playerWork.inputX = 0x48;
        }
        else {
            opponent->playerWork.inputX = -0x48;
        }
    }
    PlayerMotionUpdate(&opponent->playerWork);
}

void ArenaModelShadowsSet(OMOBJ *object, Vec *position, s32 modelCount)
{
    s32 modelIndex;

    if (1495.0f > position->y) {
        for (modelIndex = 0; modelIndex < modelCount; modelIndex++) {
            Hu3DModelShadowDispOff(object->mdlId[modelIndex]);
        }
        return;
    }
    for (modelIndex = 0; modelIndex < modelCount; modelIndex++) {
        Hu3DModelShadowDispOn(object->mdlId[modelIndex]);
    }
}

void ArenaOpponentModelReset(ArenaOpponentState *opponent, OMOBJ *object, char *animationName, s32 modelCount)
{
    s32 modelIndex;

    opponent->animationName = animationName;
    opponent->stateFlags2 |= 1;
    omSetTra(object, 0.0f, 0.0f, 0.0f);
    omSetRot(object, 0.0f, 0.0f, 0.0f);
    omSetSca(object, 1.0f, 1.0f, 1.0f);
    for (modelIndex = 0; modelIndex < modelCount; modelIndex++) {
        Hu3DModelPosSet(object->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(object->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(object->mdlId[modelIndex], 1.0f, 1.0f, 1.0f);
    }
}

void ArenaOpponentModelPositionSet(ArenaOpponentState *opponent, OMOBJ *object, char *animationName, s32 modelCount)
{
    s32 modelIndex;

    opponent->animationName = animationName;
    opponent->stateFlags2 &= ~1;
    ModelObjectWorldPositionGet(lbl1Bss1A0->mdlId[0], opponent->animationName, &opponent->playerWork.worldPosition, NULL);
    omSetTra(object, 0.0f, 0.0f, 0.0f);
    omSetRot(object, 0.0f, 0.0f, 0.0f);
    for (modelIndex = 0; modelIndex < modelCount; modelIndex++) {
        MTXIdentity(Hu3DData[object->mdlId[modelIndex]].mtx);
        Hu3DModelPosSet(object->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(object->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
    }
}

s32 ArenaOpponentMoveToTarget(ArenaOpponentState *opponent)
{
    if (PlayerMoveTowardPosition(&opponent->playerWork, &opponent->targetPosition) != 0) {
        opponent->actionState += 1;
        return 1;
    }
    PlayerMotionUpdate(&opponent->playerWork);
    return 0;
}

void ArenaOpponentHeightInterpolate(ArenaOpponentState *opponent, float duration, float startHeight, float targetHeight)
{
    float normalizedProgress;
    float easedProgress;

    opponent->actionTimer += 1.0f;
    normalizedProgress = opponent->actionTimer / duration;
    if (1.0f < normalizedProgress) {
        normalizedProgress = 1.0f;
    }
    easedProgress = sind((90.0f * normalizedProgress)) * sind((90.0f * normalizedProgress));
    opponent->playerWork.worldPosition.y = startHeight + (easedProgress * (targetHeight - startHeight));
}

void ArenaOpponentMotionSet(s32 motionIndex, float transitionFrames, u32 motionAttributes, s32 collisionState, float verticalOffset)
{
    ArenaOpponentState *opponent;

    if (transitionFrames == 0.0f) {
        Hu3DMotionSet(lbl1Bss134->mdlId[0], lbl1Bss134->mtnId[motionIndex]);
    }
    else {
        Hu3DMotionShiftSet(lbl1Bss134->mdlId[0], lbl1Bss134->mtnId[motionIndex], 0.0f, transitionFrames, motionAttributes);
    }
    opponent = lbl1Bss134->data;
    opponent->collisionState = -1;
    opponent->stateFlags2 &= ~0x2100;
    if (collisionState >= 0) {
        opponent->collisionState = collisionState;
        opponent->verticalOffset = verticalOffset;
        opponent->stateFlags2 |= 0x100;
        if (motionIndex == 4 || motionIndex == 5) {
            opponent->stateFlags2 |= 0x2000;
        }
    }
}

s32 lbl1DataF6C[6];
ScoreDisplayState lbl1DataA0C[2];

void ArenaOpponentMovementSpeedSet(ArenaOpponentState *opponent, Vec2f *speedRange, s32 scaleByTimer)
{
    float speedDifference;
    float timerSpeedScale = 1.0f;
    if (scaleByTimer != 0) {
        timerSpeedScale = opponent->turnTimer;
        if ((lbl1DataF6C[lbl1Bss118] - lbl1Bss304) > 60 * REFRESH_RATE) {
            opponent->turnTimer = opponent->turnTimer - 0.1f;
            if (0.0f > opponent->turnTimer) {
                opponent->turnTimer = 0.0f;
            }
        }
    }
    speedDifference = speedRange->x - speedRange->y;
    opponent->movementSpeed = speedRange->y + (speedDifference * timerSpeedScale);
}

float ArenaOpponentTargetHeightGet(ArenaOpponentState *opponent)
{
    s32 scoreIndex = lbl1DataA0C[1].score - 1;
    if (scoreIndex > 4) {
        scoreIndex = 4;
    }
    if (scoreIndex < 0) {
        scoreIndex = 0;
    }
    return opponent->targetHeight[scoreIndex];
}

void ArenaOpponentArenaBoundsClamp(OMOBJ *object)
{
    float distanceFromArenaCenter;
    ArenaOpponentState *opponent = object->data;

    opponent->isOutsideArena = 0;
    distanceFromArenaCenter = sqrtf((opponent->playerWork.worldPosition.x * opponent->playerWork.worldPosition.x) + (opponent->playerWork.worldPosition.z * opponent->playerWork.worldPosition.z));
    if (680.0f < distanceFromArenaCenter) {
        opponent->isOutsideArena = 1;
        opponent->playerWork.worldPosition.x = 680.0f * (opponent->playerWork.worldPosition.x / distanceFromArenaCenter);
        opponent->playerWork.worldPosition.z = 680.0f * (opponent->playerWork.worldPosition.z / distanceFromArenaCenter);
    }
}

void ArenaOpponentUpdate(OMOBJ *object)
{
    Mtx opponentScaleMatrix;
    Vec opponentWorldPosition;
    Vec directionVector;
    Vec playerWorldPosition;
    Vec opponentHeadPosition;
    float bobbingHeight;
    float actionWaitFrames;

    ArenaOpponentState *opponent;
    s32 shouldSpawnFireParticle;

    shouldSpawnFireParticle = 0;
    playerWorldPosition = ((ArenaPlayerState *)lbl1Bss138->data)->playerWork.worldPosition;
    opponent = object->data;
    opponent->playerWork.inputX = 0;
    opponent->playerWork.inputZ = 0;
    opponent->playerWork.movementX = 0;
    opponent->playerWork.movementZ = 0;
    opponent->playerWork.targetPlayer = 0;
    opponent->playerWork.collisionCount = 0;
    switch (opponent->actionState) {
        case 0xBB8:
            if (lbl1Bss1A4->work[0] >= 0x3EF) {
                opponent->actionState++;
                opponent->rotationProgress = 0.0f;
                opponent->stateFlags2 = opponent->stateFlags2 & 0xFFFFFEFF;
            }
            break;
        case 0xBB9:
            ArenaOpponentArcMove(opponent, object->mdlId[0], lbl1Bss4C, 0x7A);
            if (1000.0f > opponent->playerWork.worldPosition.y) {
                opponent->actionState++;
                Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            break;
        case 0xBBA:
            if (lbl1Bss1A4->work[0] >= 0x402) {
                opponent->actionState++;
                opponent->actionTimer = 0.0f;
                opponent->playerWork.rotationY = 270.0f;
                ArenaOpponentModelReset(opponent, object, "m450mex06-s4kuppa", 2);
                PlayerMotionSet(&opponent->playerWork, 0);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            break;
        case 0xBBB:
            if (lbl1Bss1A4->work[0] >= 0x403) {
                opponent->actionTimer += 1.0f;
                if (30.0f < opponent->actionTimer) {
                    opponent->actionState++;
                    opponent->rotationProgress = 0.0f;
                    ArenaOpponentModelPositionSet(opponent, object, "m450mex06-s4kuppa", 2);
                    omSetSca(object, 1.5f, 1.5f, 1.5f);
                    opponent->playerWork.rotationY = 270.0f;
                    opponent->playerWork.worldPosition.y = 1505.0f;
                }
            }
            break;
        case 0xBBC:
            ArenaOpponentArcMove(opponent, object->mdlId[0], &lbl1Bss4C[3], 0x78);
            if (1000.0f > opponent->playerWork.worldPosition.y) {
                opponent->actionState++;
                Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
                lbl1Bss1A4->work[0] = 0x404;
                lbl1Bss128 = 0.0f;
            }
            break;
        case 0xBBD:
            if (lbl1Bss1A4->work[0] >= 0x409) {
                opponent->actionState++;
                opponent->actionTimer = 0.0f;
                opponent->motionTimer = 360.0f * frandf();
                opponent->stateFlags2 |= 2;
                MTXIdentity(Hu3DData[object->mdlId[0]].mtx);
                omSetSca(object, 1.0f, 1.0f, 1.0f);
                Hu3DModelScaleSet(object->mdlId[1], 1.5f, 1.5f, 1.5f);
                Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(object->mdlId[1], HU3D_MOTATTR_PAUSE);
                Hu3DModelHookSet(object->mdlId[1], "g000i00-itemhook_A", object->mdlId[0]);
                opponent->playerWork.worldPosition.x = 0.0f;
                opponent->playerWork.worldPosition.y = 3000.0f;
                opponent->playerWork.worldPosition.z = 0.0f;
                opponent->playerWork.rotationY = 270.0f;
                PlayerMotionSet(&opponent->playerWork, 0);
                opponent->playerWork.stateFlags |= 0x2020;
                Hu3DMotionSet(object->mdlId[0], object->mtnId[7]);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            break;
        case 0xBBE:
            ArenaOpponentHeightInterpolate(opponent, 2 * REFRESH_RATE, 3000.0f, 1605.0f);
            if (lbl1Bss1A4->work[0] >= 0x40B) {
                opponent->actionState++;
                opponent->actionTimer = 0.0f;
                opponent->targetPosition.x = 1200.0f;
                opponent->targetPosition.y = opponent->playerWork.worldPosition.y;
                opponent->targetPosition.z = 0.0f;
            }
            break;
        case 0xBBF:
            ArenaOpponentHeightInterpolate(opponent, 60.0f, opponent->targetPosition.y, 1605.0f);
            if ((ArenaOpponentMoveToTarget(opponent) != 0) && (lbl1Bss1A4->work[0] < 0x40C)) {
                lbl1Bss1A4->work[0] = 0x40C;
                lbl1Bss128 = 0.0f;
            }
            break;
        case 0xBC0:
            opponent->playerWork.rotationY = AngleInterpolate(opponent->playerWork.rotationY, 270.0f, 0.05f);
            if (lbl1Bss1A4->work[0] >= 0x40D) {
                opponent->actionState++;
                opponent->targetRotation = 0.0f;
                opponent->turnSpeed = 0.0f;
                ArenaOpponentMovementSpeedSet(opponent, opponent->cornerPositions, 0);
                opponent->targetPosition = opponent->playerWork.worldPosition;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[9], 0.0f, 8.0f, 0x40000001);
            }
            break;
        case 0xBC1:
            if ((opponent->stateFlags2 & 0x80) != 0) {
                opponent->movementSpeed += 1.0f;
                if ((Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) && (Hu3DMotionEndCheck(object->mdlId[0]) != 0)) {
                    opponent->stateFlags2 = opponent->stateFlags2 & 0xFFFFFF7F;
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[9], 0.0f, 8.0f, 0x40000001);
                }
            }
            else if ((opponent->actionTimer2 > lbl1DataA0C[0].score) || (lbl1DataA0C[0].score <= 0)) {
                opponent->movementSpeed += 1.0f;
                opponent->stateFlags2 |= 0x80;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[8], 0.0f, 8.0f, 0);
            }
            VECSubtract(&((ArenaPlayerState *)lbl1Bss138->data)->playerWork.worldPosition, &opponent->playerWork.worldPosition, &directionVector);
            opponent->playerWork.rotationY = AngleInterpolate(opponent->playerWork.rotationY, atan2d(directionVector.x, directionVector.z), 0.4f);
            opponent->movementSpeed -= 1.0f;
            if (0.0f > opponent->movementSpeed) {
                opponent->actionState = 0xBC2;
                opponent->stateFlags2 |= 0x10;
                ArenaOpponentMovementSpeedSet(opponent, opponent->cornerPositions, 0);
                HookEffectCreate(object->mdlId[0], object->mdlId[1], opponent->playerWork.rotationY - 45.0f);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[10], 0.0f, 8.0f, 0);
            }
            opponent->targetRotation += 0.5f;
            if (360.0f <= opponent->targetRotation) {
                opponent->targetRotation -= 360.0f;
            }
            opponent->playerWork.worldPosition.x
                = opponent->targetPosition.x - (400.0 * (1.0 - (0.0 > cosd(opponent->targetRotation) ? -cosd(opponent->targetRotation) : cosd(opponent->targetRotation))));
            opponent->playerWork.worldPosition.z = opponent->targetPosition.z + (800.0 * sind(opponent->targetRotation));
            if ((opponent->actionState == 0xBC1) && (lbl1Bss1A4->work[0] >= 0x40E)) {
                opponent->actionState = 0xBC3;
                opponent->targetPosition = opponent->playerWork.worldPosition;
                opponent->targetPosition.z = -1800.0f;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[7], 0.0f, 8.0f, 0x40000001);
            }
            break;
        case 0xBC2:
            opponent->stateFlags2 &= ~0x80;
            if (((opponent->stateFlags2 & 0x20) == 0) && (Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) && (Hu3DMotionEndCheck(object->mdlId[0]) != 0)) {
                opponent->stateFlags2 |= 0x20;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[7], 0.0f, 8.0f, 0x40000001);
            }
            opponent->movementSpeed -= 1.0f;
            if (0.0f > opponent->movementSpeed) {
                opponent->actionState = 0xBC1;
                opponent->stateFlags2 &= ~0x30;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[9], 0.0f, 8.0f, 0x40000001);
                ArenaOpponentMovementSpeedSet(opponent, opponent->cornerPositions, 1);
            }
            break;
        case 0xBC3:
            if (lbl1Bss1A4->work[0] >= 0x40F) {
                if ((opponent->stateFlags2 & 0x1000) == 0) {
                    opponent->stateFlags2 |= 0x1000;
                    HuAudFXPlay(0x3B);
                }
                if (PlayerMoveTowardPosition(&opponent->playerWork, &opponent->targetPosition) != 0) {
                    opponent->actionState++;
                    opponent->stateFlags2 &= ~2;
                    Hu3DModelHookObjReset(object->mdlId[1], "g000i00-itemhook_A");
                    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
                    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
                    lbl1Bss1A4->work[0] = 0x410;
                    lbl1Bss128 = 0.0f;
                }
                PlayerMotionUpdate(&opponent->playerWork);
            }
            break;
        case 0xBC4:
            if (lbl1Bss1A4->work[0] >= 0x411) {
                opponent->actionState++;
                opponent->playerWork.rotationY = 0.0f;
                ArenaOpponentModelReset(opponent, object, "m450mex06-s6kuppa01", 2);
                PlayerMotionSet(&opponent->playerWork, 0);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            break;
        case 0xBC5:
            if (lbl1Bss1A4->work[0] >= 0x415) {
                opponent->actionState = 0xBC7;
                opponent->actionTimer = 0.0f;
                ArenaOpponentModelPositionSet(opponent, object, "m450mex06-s6kuppa01", 2);
                omSetSca(object, 1.5f, 1.5f, 1.5f);
                opponent->playerWork.rotationY = 0.0f;
                opponent->playerWork.worldPosition.y = 1505.0f;
                PlayerMotionSet(&opponent->playerWork, 0);
                opponent->playerWork.updateHook = ArenaOpponentArenaBoundsClamp;
            }
            break;
        case 0xBC6:
            opponent->actionTimer += 1.0f;
            if (120.0f < opponent->actionTimer) {
                opponent->actionState++;
                opponent->actionTimer = 0.0f;
                opponent->playerWork.stateFlags &= ~0x20;
                PlayerMotionSet(&opponent->playerWork, 0);
            }
            break;
        case 0xBC7:
            opponent->playerWork.movementScale = ArenaOpponentTargetHeightGet(opponent);
            PlayerMoveTowardPosition(&opponent->playerWork, &playerWorldPosition);
            PlayerMotionUpdate(&opponent->playerWork);
            opponent->actionTimer += 1.0f;
            actionWaitFrames = (REFRESH_RATE * (3.0f + lbl1DataA0C[1].score));
            if (actionWaitFrames < opponent->actionTimer) {
                opponent->actionState++;
                opponent->stateFlags2 &= ~0x70;
                opponent->actionTimer = 0.0f;
                opponent->movementOrigin = playerWorldPosition;
                if (frandmod(0x64) < 0x1E) {
                    opponent->targetPlayer = 0xBC9;
                }
                else if (500.0f > VECDistance(&opponent->playerWork.worldPosition, &playerWorldPosition)) {
                    opponent->targetPlayer = 0xBCA;
                    if (lbl1DataA0C[1].score <= 2) {
                        opponent->targetPlayer = 0xBCB;
                    }
                }
                else {
                    opponent->targetPlayer = 0xBCC;
                }
                PlayerMotionSet(&opponent->playerWork, 0);
            }
            break;
        case 0xBC8:
            VECSubtract(&opponent->movementOrigin, &opponent->playerWork.worldPosition, &directionVector);
            opponent->playerWork.rotationY = AngleInterpolate(opponent->playerWork.rotationY, atan2d(directionVector.x, directionVector.z), 0.4f);
            opponent->actionTimer += 1.0f;
            opponent->movementOrigin = playerWorldPosition;
            if (60.0f < opponent->actionTimer) {
                opponent->actionState = opponent->targetPlayer;
                opponent->stateFlags2 &= ~0x30;
                opponent->actionTimer = 0.0f;
                opponent->playerWork.stateFlags |= 0x20;
            }
            break;
        case 0xBC9:
            if ((opponent->stateFlags2 & 0x10) == 0) {
                opponent->stateFlags2 |= 0x10;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[15], 0.0f, 8.0f, 0x40000001);
            }
            else {
                opponent->playerWork.movementScale = 1.5f;
                if ((PlayerMoveTowardPosition(&opponent->playerWork, &opponent->movementOrigin) != 0) || (opponent->playerWork.actionTimer > 0)) {
                    opponent->actionState = 0xBC6;
                    opponent->actionTimer = 0.0f;
                    opponent->stateFlags2 |= 0x20;
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 8.0f, 0x40000001);
                }
                else {
                    PlayerMotionUpdate(&opponent->playerWork);
                }
            }
            break;
        case 0xBCA:
            if ((opponent->stateFlags2 & 0x10) == 0) {
                opponent->stateFlags2 |= 0x10;
                opponent->hitCount = FireParticleSystemCreate(NULL, NULL, 0x200);
                opponent->targetCount = HuAudFXPlay(0x7BC);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[12], 0.0f, 8.0f, 0);
            }
            else if (Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) {
                if (45.0f < Hu3DMotionTimeGet(object->mdlId[0])) {
                    shouldSpawnFireParticle = 1;
                }
                if (shouldSpawnFireParticle != 0) {
                    if ((Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) && (Hu3DMotionEndCheck(object->mdlId[0]) != 0)) {
                        Hu3DMotionTimeSet(object->mdlId[0], 50.0f);
                    }
                    opponent->actionTimer += 1.0f;
                    if (3 * REFRESH_RATE < opponent->actionTimer) {
                        opponent->actionState = 0xBC6;
                        opponent->actionTimer = 0.0f;
                        opponent->stateFlags2 |= 0x20;
                        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 30.0f, 0x40000001);
                        ParticleSystemDestroyRequest(opponent->hitCount);
                        if (opponent->targetCount >= 0) {
                            HuAudFXStop(opponent->targetCount);
                            opponent->targetCount = -1;
                        }
                    }
                }
            }
            break;
        case 0xBCB:
            if ((opponent->stateFlags2 & 0x10) == 0) {
                opponent->stateFlags2 |= 0x10;
                opponent->hitCount = FireParticleSystemCreate(NULL, NULL, 0x200);
                opponent->targetCount = HuAudFXPlay(0x7BC);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[13], 0.0f, 8.0f, 0);
            }
            else if ((opponent->stateFlags2 & 0x40) == 0) {
                if (Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) {
                    if (52.0f < Hu3DMotionTimeGet(object->mdlId[0])) {
                        shouldSpawnFireParticle = 1;
                    }
                    if (Hu3DMotionEndCheck(object->mdlId[0]) != 0) {
                        opponent->stateFlags2 |= 0x40;
                        Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
                        Hu3DMotionSet(object->mdlId[0], object->mtnId[14]);
                    }
                }
            }
            else {
                shouldSpawnFireParticle = 1;
                opponent->playerWork.movementScale = 0.25f;
                PlayerMoveTowardPosition(&opponent->playerWork, &playerWorldPosition);
                PlayerMotionUpdate(&opponent->playerWork);
                opponent->actionTimer += 1.0f;
                if (3 * REFRESH_RATE < opponent->actionTimer) {
                    opponent->actionState = 0xBC6;
                    opponent->actionTimer = 0.0f;
                    opponent->stateFlags2 |= 0x20;
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 8.0f, 0x40000001);
                    ParticleSystemDestroyRequest(opponent->hitCount);
                    if (opponent->targetCount >= 0) {
                        HuAudFXStop(opponent->targetCount);
                        opponent->targetCount = -1;
                    }
                }
            }
            break;
        case 0xBCC:
            if ((opponent->stateFlags2 & 0x10) == 0) {
                HookEffectArcCreate(object->mdlId[0], &opponent->movementOrigin);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DMotionSet(object->mdlId[0], object->mtnId[11]);
            }
            else if (((opponent->stateFlags2 & 0x20) == 0) && (Hu3DMotionShiftIDGet(object->mdlId[0]) < 0)
                && (Hu3DMotionEndCheck(object->mdlId[0]) != 0)) {
                opponent->actionState = 0xBC6;
                opponent->actionTimer = 0.0f;
                opponent->stateFlags2 |= 0x20;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 8.0f, 0x40000001);
                if (((opponent->stateFlags2 & 0x40) == 0) && (lbl1DataA0C[1].score <= 2)) {
                    opponent->actionState = 0xBCD;
                    opponent->stateFlags2 |= 0x40;
                }
            }
            break;
        case 0xBCD:
            opponent->actionTimer += 1.0f;
            if (10.0f < opponent->actionTimer) {
                opponent->actionState = 0xBC8;
                opponent->targetPlayer = 0xBCC;
                opponent->movementOrigin = playerWorldPosition;
                opponent->stateFlags2 &= ~0x30;
                opponent->actionTimer = 0.0f;
            }
            break;
        case 0xBCE:
            if ((Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) && (Hu3DMotionEndCheck(object->mdlId[0]) != 0)) {
                opponent->actionState = 0xBC6;
                opponent->actionTimer = 0.0f;
                opponent->stateFlags2 &= ~0x8000;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 8.0f, 0x40000001);
                Hu3DModelAttrSet(lbl1Bss1A0->mdlId[opponent->actionIndex + 2], HU3D_ATTR_DISPOFF);
            }
            break;
        case 0xBCF:
            if ((opponent->stateFlags2 & 0x2000) && (Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) && (128.0f < Hu3DMotionTimeGet(object->mdlId[0]))) {
                opponent->stateFlags2 &= ~0x2000;
                HuAudFXPlay(0x7BD);
            }
            break;
        default:
            OSReport("*** kuppa mode error(%d)!!\n", opponent->actionState);
            break;
    }

    if ((opponent->actionState >= 0xBC6) && (opponent->actionState <= 0xBCD)) {
        if (opponent->stateFlags2 & 0x8000) {
            opponent->actionTimer = 0.0f;
            if (opponent->hitCount >= 0) {
                ParticleSystemDestroyRequest(opponent->hitCount);
            }
            if (opponent->targetCount >= 0) {
                HuAudFXStop(opponent->targetCount);
                opponent->targetCount = -1;
            }
            if (ScoreDisplaySubtractPoints(1, 1) <= 0) {
                opponent->actionState = 0xBCF;
                opponent->stateFlags2 |= 0x2000;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[17], 0.0f, 8.0f, 0);
                HuAudFXPlay(0x3B);
            }
            else {
                opponent->actionState = 0xBCE;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[16], 0.0f, 8.0f, 0);
                HuAudFXPlay(0x3B);
                Hu3DModelPosSet(lbl1Bss1A0->mdlId[opponent->actionIndex + 2], opponent->playerWork.worldPosition.x, opponent->playerWork.worldPosition.y,
                    150.0f + opponent->playerWork.worldPosition.z);
                Hu3DModelScaleSet(lbl1Bss1A0->mdlId[opponent->actionIndex + 2], 2.0f, 2.0f, 2.0f);
                Hu3DMotionTimeSet(lbl1Bss1A0->mdlId[opponent->actionIndex + 2], 0.0f);
                Hu3DModelAttrSet(lbl1Bss1A0->mdlId[opponent->actionIndex + 2], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(lbl1Bss1A0->mdlId[opponent->actionIndex + 2], HU3D_MOTATTR_PAUSE);
                Hu3DModelAttrReset(lbl1Bss1A0->mdlId[opponent->actionIndex + 2], HU3D_ATTR_DISPOFF);
            }
        }
        if (opponent->actionState != 0xBCF) {
            if ((opponent->actionState < 0xBCE) && ((lbl1DataA0C[0].score <= 0) || (((ArenaPlayerState *)lbl1Bss138->data)->playerState >= 0x7EB))) {
                opponent->actionState = 0xBCF;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[3], 0.0f, 8.0f, 0x40000001);
                HuAudFXPlay(0x3A);
                if (opponent->hitCount >= 0) {
                    ParticleSystemDestroyRequest(opponent->hitCount);
                }
                if (opponent->targetCount >= 0) {
                    HuAudFXStop(opponent->targetCount);
                    opponent->targetCount = -1;
                }
            }
            else if (((opponent->isOutsideArena != 0) || (ArenaPlayerOpponentCollisionResolve() != 0)) && (opponent->actionState == 0xBC9)) {
                opponent->actionState = 0xBC6;
                opponent->actionTimer = 0.0f;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 8.0f, 0x40000001);
            }
        }
    }
    if ((opponent->stateFlags2 & 0x100) && (Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) && (Hu3DMotionEndCheck(object->mdlId[0]) != 0)) {
        if (opponent->stateFlags2 & 0x2000) {
            opponent->stateFlags2 &= ~0x2000;
            HuAudFXPlay(0x7BE);
        }
        opponent->verticalOffset -= 1.0f;
        if (0.0f > opponent->verticalOffset) {
            opponent->stateFlags2 &= ~0x100;
            Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[opponent->collisionState], 0.0f, 8.0f, 0x40000001);
        }
    }
    if (opponent->hitCount >= 0) {
        if (shouldSpawnFireParticle != 0) {
            ModelObjectPositionGet(object->mdlId[0], "g000m0-itemhook_M", &opponentWorldPosition);
            ModelObjectPositionGet(object->mdlId[0], "g000m0-ns_hed", &opponentHeadPosition);
            VECSubtract(&opponentWorldPosition, &opponentHeadPosition, &directionVector);
            FireParticleBurstCreate(opponent->hitCount, 1, &opponentWorldPosition, atan2d(directionVector.x, directionVector.z), 700.0f, 1550.0f);
        }
        if (ParticleSystemDestroyIfFinished(opponent->hitCount) != 0) {
            opponent->hitCount = -1;
        }
    }
    opponent->actionTimer2 = lbl1DataA0C[0].score;
    opponentWorldPosition = opponent->playerWork.worldPosition;
    if (opponent->stateFlags2 & 2) {
        opponent->motionTimer += 4.0f;
        if (360.0f <= opponent->motionTimer) {
            opponent->motionTimer -= 360.0f;
        }
        bobbingHeight = opponent->playerWork.worldPosition.y + (8.0 * sind(opponent->motionTimer));
        omSetTra(object, 0.0f, 0.0f, 0.0f);
        omSetRot(object, 0.0f, 0.0f, 0.0f);
        Hu3DModelPosSet(object->mdlId[1], opponent->playerWork.worldPosition.x, bobbingHeight, opponent->playerWork.worldPosition.z);
        Hu3DModelRotSet(object->mdlId[1], 0.0f, opponent->playerWork.rotationY, 0.0f);
    }
    else if (opponent->stateFlags2 & 1) {
        ModelObjectWorldPositionGet(lbl1Bss1A0->mdlId[0], opponent->animationName, &opponentWorldPosition, Hu3DData[object->mdlId[0]].mtx);
        MTXScale(opponentScaleMatrix, 1.5f, 1.5f, 1.5f);
        MTXConcat(Hu3DData[object->mdlId[0]].mtx, opponentScaleMatrix, Hu3DData[object->mdlId[0]].mtx);
    }
    else {
        omSetTra(opponent->playerWork.object, opponent->playerWork.worldPosition.x, opponent->playerWork.worldPosition.y, opponent->playerWork.worldPosition.z);
        omSetRot(opponent->playerWork.object, 0.0f, opponent->playerWork.rotationY, 0.0f);
    }
    ArenaModelShadowsSet(object, &opponentWorldPosition, 2);
}

void ArenaOpponentCharacterInit(OMOBJ *object)
{
    Vec startingPosition;

    ArenaOpponentState *opponent;
    s32 modelIndex;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(ArenaOpponentState), HU_MEMNUM_OVL);
    opponent = object->data;
    opponent->actionState = 0xBB8;
    opponent->stateFlags2 = 0;
    opponent->targetState = 0;
    opponent->hitCount = -1;
    opponent->targetCount = -1;
    startingPosition.x = 0.0f;
    startingPosition.y = 1500.0f;
    startingPosition.z = -800.0f;
    PlayerMovementWorkInit(&opponent->playerWork, object, 4, 9, lbl1Data18C, 0, &startingPosition, 0.0f);
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        object->mdlId[modelIndex] = Hu3DModelCreateFile(lbl1DataAC[modelIndex]);
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_MOTATTR_LOOP);
        Hu3DModelAttrReset(object->mdlId[modelIndex], HU3D_MOTATTR_PAUSE);
        Hu3DModelLayerSet(object->mdlId[modelIndex], 3);
        Hu3DModelShadowSet(object->mdlId[modelIndex]);
    }
    for (modelIndex = 0; modelIndex < 0x13; modelIndex++) {
        object->mtnId[modelIndex] = Hu3DJointMotion(object->mdlId[0], HuDataSelHeapReadNum(lbl1DataB4[modelIndex], HU_MEMNUM_OVL, HEAP_MODEL));
    }
    Hu3DMotionSet(object->mdlId[0], object->mtnId[0]);
    CharNpcDustSet(object->mdlId[0], object->mtnId[1], 2, 8);
    CharNpcDustSet(object->mdlId[0], object->mtnId[2], 1, 8);
    CharNpcDustSet(object->mdlId[0], object->mtnId[15], 3, 8);
    opponent->turnTimer = 1.0f;
    switch (GWPlayerCfg[lbl1Bss120].diff) {
        case 0:
            opponent->cornerPositions = &lbl1Data234[0];
            opponent->targetHeight = lbl1Data254;
            break;
        case 1:
            opponent->cornerPositions = &lbl1Data234[1];
            opponent->targetHeight = lbl1Data268;
            break;
        case 2:
            opponent->cornerPositions = &lbl1Data234[2];
            opponent->targetHeight = lbl1Data27C;
            break;
        case 3:
        default:
            opponent->cornerPositions = &lbl1Data234[3];
            opponent->targetHeight = lbl1Data290;
            break;
    }
    omSetTra(opponent->playerWork.object, opponent->playerWork.worldPosition.x, opponent->playerWork.worldPosition.y, opponent->playerWork.worldPosition.z);
    omSetRot(opponent->playerWork.object, 0.0f, opponent->playerWork.rotationY, 0.0f);
    omSetSca(object, 1.5f, 1.5f, 1.5f);
    Hu3DModelAttrReset(*object->mdlId, HU3D_ATTR_DISPOFF);
    object->objFunc = (OMOBJFUNC)ArenaOpponentUpdate;
}

s32 ArenaOpponentImpactSoundPlay(s32 opponentIndex, s32 impactSoundIndex)
{
    s32 soundMappingIndex;

    for (soundMappingIndex = 0; soundMappingIndex < 2; soundMappingIndex++) {
        if (impactSoundIndex == lbl1Data1CC[soundMappingIndex * 2]) {
            return HuAudFXPlay(lbl1Data1CC[opponentIndex + (soundMappingIndex * 2)]);
        }
    }

    return -1;
}

s32 ArenaOpponentTargetStateSet(s32 opponentIndex, s32 targetState)
{
    ArenaOpponentState *opponent = lbl1Bss130[opponentIndex]->data;
    opponent->targetState = targetState;
    return opponentIndex;
}

void ArenaOpponentAnimationSet(s32 opponentIndex, s32 animationIndex, float transitionFrames, u32 motionAttributes, s32 collisionState, float verticalOffset)
{
    ArenaOpponentState *opponent;
    OMOBJ *opponentObject;

    opponentObject = lbl1Bss130[opponentIndex];
    if (transitionFrames == 0.0f) {
        Hu3DMotionSet(opponentObject->mdlId[0], opponentObject->mtnId[animationIndex]);
    }
    else {
        Hu3DMotionShiftSet(opponentObject->mdlId[0], opponentObject->mtnId[animationIndex], 0.0f, transitionFrames, motionAttributes);
    }
    opponent = opponentObject->data;
    opponent->collisionState = -1;
    opponent->stateFlags2 &= ~0x100;
    if (collisionState >= 0) {
        opponent->collisionState = collisionState;
        opponent->verticalOffset = verticalOffset;
        opponent->stateFlags2 |= 0x100;
    }
    opponent->impactState = animationIndex;
}

void ArenaMessageSequenceStart(s32 messageGroup, s32 messageIndex, s32 alternateSequence, s32 messageParameter)
{
    lbl1Bss4 = messageGroup;
    lbl1Bss0 = messageIndex;
    if (alternateSequence == 0) {
        MessageProcessStart(ArenaOpponentMessageProcess, messageParameter);
    }
    else {
        MessageProcessStart(ArenaPlayerMessageProcess, messageParameter);
    }
}

void ArenaOpponentModelVisibilitySet(ArenaOpponentState *opponent, OMOBJ *object, s32 visible)
{
    s32 modelIndex;
    if (visible != 0) {
        if (object->work[0] == 0) {
            Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
        }
        Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    }
    else {
        for (modelIndex = 0; modelIndex < 5; modelIndex++) {
            Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
        }
    }
    opponent->playerWork.stateFlags &= ~0xC00;
}

ArenaModelLayout arenaLayout;
ArenaModelLayout lbl1Data13B8;

void ArenaOpponentRoamingTargetSelect(ArenaOpponentState *opponent, OMOBJ *object)
{
    s32 modelIndex;
    float randomDistance;
    ArenaModelLayout *modelLayout = &arenaLayout;
    float randomAngle;

    opponent->targetIndex = -1;
    opponent->stateFlags2 &= ~0x30;
    opponent->movementTarget.x = 0.0f;
    opponent->movementTarget.y = 0.0f;
    opponent->movementTarget.z = 0.0f;
    if (object->work[0] != 0) {
        modelIndex = frandmod(modelLayout->modelCount);
        while (modelLayout->modelIds[modelIndex] < 0) {
            modelIndex = (modelIndex + 1) % modelLayout->modelCount;
        }
        opponent->targetIndex = modelLayout->modelIds[modelIndex];
        randomDistance = 200.0f * frandf();
        randomAngle = 360.0f * frandf();
        opponent->movementTarget.x = randomDistance * sind(randomAngle);
        opponent->movementTarget.z = randomDistance * cosd(randomAngle);
    }
}

void ArenaOpponentPlayerChaseUpdate(ArenaOpponentState *opponent, OMOBJ *object)
{
    Vec fireEffectPosition;
    Vec targetOffset;

    opponent->targetPosition = ((ArenaPlayerState *)lbl1Bss138->data)->playerWork.worldPosition;
    if (((opponent->stateFlags2 & 0x10) != 0) && ((opponent->stateFlags2 & 0x20) == 0)) {
        if ((Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) && (20.0f < Hu3DMotionTimeGet(object->mdlId[0]))) {
            opponent->stateFlags2 |= 0x20;
            fireEffectPosition = opponent->playerWork.worldPosition;
            fireEffectPosition.x += (200.0 * sind(opponent->playerWork.rotationY));
            fireEffectPosition.y += 100.0f;
            fireEffectPosition.z += (200.0 * cosd(opponent->playerWork.rotationY));
            ArenaFireEffectCreate(&fireEffectPosition, opponent->playerWork.rotationY);
        }
    }
    else {
    PlayerMoveTowardPosition(&opponent->playerWork, &opponent->targetPosition);
        PlayerMotionUpdate(&opponent->playerWork);
        VECSubtract(&opponent->playerWork.worldPosition, &opponent->targetPosition, &targetOffset);
        if ((300.0f > sqrtf((targetOffset.x * targetOffset.x) + (targetOffset.z * targetOffset.z))) && ((opponent->stateFlags2 & 0x10) == 0)) {
            opponent->stateFlags2 |= 0x10;
            Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[5], 0.0f, 8.0f, 0);
        }
    }
}

void ArenaOpponentWaypointMoveUpdate(ArenaOpponentState *opponent, OMOBJ *object)
{
    opponent->targetPosition = *ArenaWaypointPositionGet(opponent->targetIndex);
    VECAdd(&opponent->movementTarget, &opponent->targetPosition, &opponent->targetPosition);
    if ((opponent->stateFlags2 & 0x10) && ((opponent->stateFlags2 & 0x20) == 0) && (Hu3DMotionShiftIDGet(object->mdlId[0]) < 0)
        && (Hu3DMotionEndCheck(*object->mdlId) != 0)) {
        ArenaModelTargetMove(&opponent->playerWork.worldPosition, &opponent->movementTarget, opponent->targetIndex);
    }
    if (PlayerMoveTowardPosition(&opponent->playerWork, &opponent->targetPosition) != 0) {
        if ((opponent->stateFlags2 & 0x10) == 0) {
            opponent->stateFlags2 |= 0x10;
            Hu3DMotionShiftSet(*object->mdlId, object->mtnId[5], 0.0f, 8.0f, 0);
        }
    }
    else {
        PlayerMotionUpdate(&opponent->playerWork);
    }
}

void ArenaOpponentJumpStart(ArenaOpponentState *opponent, OMOBJ *object)
{
    opponent->actionState = 0xFAD;
    opponent->actionTimer = 0.0f;
    Hu3DMotionShiftSet(*object->mdlId, *object->mtnId, 0.0f, 8.0f, 0);
    ModelObjectWorldPositionGet(lbl1Bss1A0->mdlId[0], object->work[0] == 0 ? "m450mex06-s3mini2" : "m450mex06-s3mini1", &opponent->targetPosition, NULL);
}

HU3DPARMANPARAM particleParameters;

void ArenaOpponentHookDropStart(ArenaOpponentState *opponent, OMOBJ *object)
{
    ModelObjectPositionGet(object->mdlId[4], "g007i03-itemhook_M", &opponent->playerWork.worldPosition);
    Hu3DModelHookObjReset(object->mdlId[4], "g007i03-itemhook_M");
    Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
    opponent->actionState = 0xFAE;
    opponent->stateFlags2 &= 0xFFFFFFFD;
    opponent->jumpSpeed = 19.6f;
    opponent->actionTimer = 0.0f;
    Hu3DMotionSet(object->mdlId[0], object->mtnId[6]);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DMotionTimeSet(object->mdlId[0], 1.0f);
    opponent->particleId = Hu3DParManLink(lbl1Bss24, &particleParameters);
    Hu3DParManPosSet(opponent->particleId, opponent->playerWork.worldPosition.x, opponent->playerWork.worldPosition.y, opponent->playerWork.worldPosition.z);
    Hu3DParManAttrReset(opponent->particleId, 1);
    Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(opponent->particleId), 1);
    Hu3DModelLayerSet(Hu3DParManModelIDGet(opponent->particleId), 5);
    ArenaOpponentImpactSoundPlay(object->work[0], 0x3F);
}

void ArenaOpponentHookDropUpdate(OMOBJ *object)
{
    Vec opponentToPlayerDirection;
    float bobbingHeight;

    ArenaOpponentState *opponent;
    Vec *opponentDisplayPosition;
    ArenaOpponentState *otherOpponent;

    otherOpponent = lbl1Bss130[object->work[0] ^ 1]->data;
    opponent = object->data;
    opponent->playerWork.inputX = 0;
    opponent->playerWork.inputZ = 0;
    opponent->playerWork.movementX = 0;
    opponent->playerWork.movementZ = 0;
    opponent->playerWork.targetPlayer = 0;
    opponent->playerWork.collisionCount = 0;

    switch (opponent->actionState) {
        case 0xFA0:
            if ((lbl1Bss1A4->work[0] >= 0x3EF) && (30.0f < lbl1Bss128)) {
                opponent->actionState++;
                opponent->rotationProgress = 0.0f;
                opponent->stateFlags2 &= ~0x100;
                opponent->stateFlags2 |= 0x200;
                opponent->jumpTimer = opponent->playerWork.worldPosition.x - (100.0f * (0.0f > opponent->playerWork.worldPosition.x ? -1 : 1));
            }
            if ((object->work[0] == 0) && (opponent->impactState == 7)) {
                opponent->playerWork.rotationY = 0.0f;
            }
            else {
                VECSubtract(&((ArenaPlayerState *)lbl1Bss138->data)->playerWork.worldPosition, &opponent->playerWork.worldPosition, &opponentToPlayerDirection);
                opponent->playerWork.rotationY = AngleInterpolate(opponent->playerWork.rotationY, atan2d(opponentToPlayerDirection.x, opponentToPlayerDirection.z), 0.4f);
            }
            break;
        case 0xFA1:
            ArenaOpponentArcMove(opponent, object->mdlId[0], lbl1Bss4C, 0x7A);
            if (-1100.0f > opponent->playerWork.worldPosition.z) {
                opponent->actionState++;
                ArenaOpponentAnimationSet(object->work[0], 12, 8.0f, 0, 1, 0.0f);
            }
            break;
        case 0xFA2:
            if (lbl1Bss10 == 0) {
                ArenaOpponentArcMove(opponent, object->mdlId[0], lbl1Bss4C, 0x7A);
                if (1000.0f > opponent->playerWork.worldPosition.y) {
                    opponent->actionState++;
                    opponent->stateFlags2 &= ~0x200;
                    ArenaOpponentModelVisibilitySet(opponent, object, 0);
                }
            }
            else {
                opponent->playerWork.rotationY = AngleInterpolate(opponent->playerWork.rotationY, 0.0f, 0.4f);
            }
            break;
        case 0xFA3:
            if (((ArenaPlayerState *)lbl1Bss138->data)->playerState >= 0x7D5) {
                opponent->actionState++;
                ArenaOpponentModelReset(opponent, object, object->work[0] == 0 ? "m450mex06-s2kuppa01" : "m450mex06-s2kuppa02", 5);
                PlayerMotionSet(&opponent->playerWork, 0);
                ArenaOpponentModelVisibilitySet(opponent, object, 1);
            }
            break;
        case 0xFA4:
            if (lbl1Bss1A4->work[0] >= 0x3F2) {
                opponent->actionState = opponent->actionState + 1;
                opponent->playerWork.rotationY = 0.0f;
                ArenaOpponentModelPositionSet(opponent, object, object->work[0] == 0 ? "m450mex06-s2kuppa01" : "m450mex06-s2kuppa02", 5);
                opponent->playerWork.worldPosition.y = 1505.0f;
                opponent->playerWork.stateFlags |= 0x400;
                ModelObjectWorldPositionGet(lbl1Bss1A0->mdlId[0], "m450mex06-s2hook04", &opponent->targetPosition, NULL);
                opponent->targetPosition.x += (-100.0f + (300.0f * (object->work[0] ^ 1)));
                opponent->targetPosition.z += (100.0f - (200.0f * (object->work[0] ^ 1)));
            }
            break;
        case 0xFA5:
            if (ArenaOpponentMoveToTarget(opponent) != 0) {
                opponent->targetPosition.x = 1200.0f;
                opponent->targetPosition.z = 150.0f - (300.0f * (object->work[0] ^ 1));
            }
            break;
        case 0xFA6:
            if (ArenaOpponentMoveToTarget(opponent)) {
                ArenaOpponentModelVisibilitySet(opponent, object, 0);
            }
            break;
        case 0xFA7:
            if (lbl1Bss1A4->work[0] >= 0x3F7) {
                opponent->actionState++;
                opponent->actionTimer = 0.0f;
                opponent->motionTimer = 360.0f * frandf();
                opponent->stateFlags2 |= 2;
                Hu3DModelAttrReset(object->mdlId[4], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(object->mdlId[4], HU3D_MOTATTR_PAUSE);
                Hu3DModelHookSet(object->mdlId[4], "g007i03-itemhook_M", object->mdlId[0]);
                opponent->playerWork.worldPosition.x = 0.0f;
                opponent->playerWork.worldPosition.y = 3000.0f;
                opponent->playerWork.worldPosition.z = -300.0f + (600.0f * object->work[0]);
                opponent->playerWork.rotationY = 270.0f;
                PlayerMotionSet(&opponent->playerWork, 0);
                opponent->playerWork.stateFlags |= 0x2020;
                ArenaOpponentModelVisibilitySet(opponent, object, 1);
            }
            break;
        case 0xFA8:
            ArenaOpponentHeightInterpolate(opponent, 2 * REFRESH_RATE, 3000.0f, 1605.0f);
            if (lbl1Bss1A4->work[0] >= 0x3F9) {
                opponent->actionState = opponent->actionState + 1;
                opponent->actionTimer = 0.0f;
                opponent->targetPosition.x = 800.0f;
                opponent->targetPosition.y = opponent->playerWork.worldPosition.y;
                opponent->targetPosition.z = -400.0f + (800.0f * object->work[0]);
            }
            break;
        case 0xFA9:
            ArenaOpponentHeightInterpolate(opponent, 60.0f, opponent->targetPosition.y, 2055.0f);
            if ((ArenaOpponentMoveToTarget(opponent) != 0) && (lbl1Bss1A4->work[0] < 0x3FA)) {
                lbl1Bss1A4->work[0] = 0x3FA;
                lbl1Bss128 = 0.0f;
            }
            break;
        case 0xFAA:
            opponent->playerWork.rotationY = AngleInterpolate(opponent->playerWork.rotationY, 270.0f, 0.05f);
            if (lbl1Bss1A4->work[0] >= 0x3FB) {
                opponent->actionState++;
                opponent->stateFlags = 0;
                opponent->movementSpeed = opponent->collisionData[opponent->stateFlags].movementSpeed;
                opponent->targetPosition = opponent->playerWork.worldPosition;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[10], 0.0f, 8.0f, 0x40000001);
            }
            break;
        case 0xFAB:
            if (opponent->stateFlags2 & 0x80) {
                opponent->movementSpeed += 1.0f;
                if ((Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) && (Hu3DMotionEndCheck(object->mdlId[0]) != 0)) {
                    opponent->stateFlags2 = opponent->stateFlags2 & 0xFFFFFF7F;
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[10], 0.0f, 8.0f, 0x40000001);
                }
            }
            else if ((opponent->actionTimer2 > lbl1DataA0C[0].score) || (lbl1DataA0C[0].score <= 0)) {
                opponent->movementSpeed += 1.0f;
                if (lbl1DataA0C[0].score > 0) {
                    opponent->stateFlags2 |= 0x80;
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[11], 0.0f, 8.0f, 0);
                }
                else {
                    opponent->actionState = 0xFB0;
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[3], 0.0f, 8.0f, 0);
                }
            }
            opponent->stateFlags2 &= ~4;
            opponent->movementSpeed -= 1.0f;
            if ((otherOpponent->stateFlags2 & 4) && (0.0f > opponent->movementSpeed)) {
                opponent->stateFlags++;
                if (opponent->collisionData[opponent->stateFlags].movementSpeed == 0.0f) {
                    opponent->stateFlags -= 2;
                }
                opponent->movementSpeed = opponent->collisionData[opponent->stateFlags].movementSpeed;
                if (opponent->collisionData[opponent->stateFlags].canAttack) {
                    opponent->actionState = 0xFAC;
                    ArenaOpponentRoamingTargetSelect(opponent, object);
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[10], 0.0f, 8.0f, 0);
                }
            }
            else if (PlayerMoveTowardPosition(&opponent->playerWork, &opponent->targetPosition) != 0) {
                opponent->stateFlags2 |= 4;
                VECSubtract(&((ArenaPlayerState *)lbl1Bss138->data)->playerWork.worldPosition, &opponent->playerWork.worldPosition, &opponentToPlayerDirection);
                opponent->playerWork.rotationY = AngleInterpolate(opponent->playerWork.rotationY, atan2d(opponentToPlayerDirection.x, opponentToPlayerDirection.z), 0.4f);
            }
            else {
                PlayerMotionUpdate(&opponent->playerWork);
            }
            if (lbl1Bss1A4->work[0] >= 0x3FC) {
                ArenaOpponentJumpStart(opponent, object);
            }
            break;
        case 0xFAC:
            opponent->stateFlags2 &= ~0x84;
            opponent->movementSpeed -= 1.0f;
            if (0.0f > opponent->movementSpeed) {
                opponent->stateFlags++;
                if (opponent->collisionData[opponent->stateFlags].movementSpeed == 0.0f) {
                    opponent->stateFlags -= 2;
                }
                opponent->movementSpeed = opponent->collisionData[opponent->stateFlags].movementSpeed;
                if (opponent->collisionData[opponent->stateFlags].canAttack == 0) {
                    opponent->actionState = 0xFAB;
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[10], 0.0f, 8.0f, 0x40000001);
                    opponent->targetPosition.x = 800.0f;
                    if (0.0f > otherOpponent->playerWork.worldPosition.z) {
                        opponent->targetPosition.z = 200.0f;
                    }
                    else {
                        opponent->targetPosition.z = -200.0f;
                    }
                    opponent->targetPosition.z += opponent->targetPosition.z * frandf();
                }
            }
            else {
                if (object->work[0] == 0) {
                    ArenaOpponentPlayerChaseUpdate(opponent, object);
                }
                else {
                    ArenaOpponentWaypointMoveUpdate(opponent, object);
                }
                if ((((opponent->stateFlags2 & 0x10) == 0) || (opponent->stateFlags2 & 0x20)) && (lbl1Bss1A4->work[0] == 0x3E8)) {
                    opponent->movementSpeed = 0.0f;
                }
            }
            if (lbl1Bss1A4->work[0] >= 0x3FC) {
                ArenaOpponentJumpStart(opponent, object);
            }
            break;
        case 0xFAD:
            if (lbl1Bss1A4->work[0] >= 0x3FD) {
                if (PlayerMoveTowardPosition(&opponent->playerWork, &opponent->targetPosition) != 0) {
                    opponent->playerWork.rotationY = AngleInterpolate(opponent->playerWork.rotationY, 270.0f, 0.4f);
                    if ((object->work[0] + 0x3FE) == lbl1Bss1A4->work[0]) {
                        lbl1Bss1A4->work[0] += 1;
                        lbl1Bss128 = 0;
                    }
                    if (lbl1Bss1A4->work[0] > 0x3FF) {
                        ArenaOpponentHookDropStart(opponent, object);
                    }
                }
                PlayerMotionUpdate(&opponent->playerWork);
            }
            break;
        case 0xFAE:
            opponent->playerWork.rotationY = AngleInterpolate(opponent->playerWork.rotationY, 270.0f, 0.4f);
            opponent->playerWork.worldPosition.x += 0.1f * (opponent->targetPosition.x - opponent->playerWork.worldPosition.x);
            opponent->playerWork.worldPosition.z += 0.1f * (opponent->targetPosition.z - opponent->playerWork.worldPosition.z);
            if (1505.0f < opponent->playerWork.worldPosition.y) {
                opponent->playerWork.worldPosition.y += opponent->jumpSpeed;
                opponent->jumpSpeed = opponent->jumpSpeed - 0.98f;
                if (1505.0f >= opponent->playerWork.worldPosition.y) {
                    opponent->playerWork.worldPosition.y = 1505.0f;
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
                }
            }
            else if (((opponent->stateFlags2 & 0x400) == 0) && (Hu3DMotionEndCheck(object->mdlId[0]) != 0)) {
                opponent->stateFlags2 |= 0x400;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 8.0f, 0x40000001);
            }
            if (lbl1Bss1A4->work[0] >= 0x402) {
                opponent->actionState = opponent->actionState + 1;
                opponent->actionTimer = 0.0f;
                opponent->playerWork.stateFlags &= ~0x2020;
                ArenaOpponentModelReset(opponent, object, object->work[0] == 0 ? "m450mex06-s3mini2" : "m450mex06-s3mini1", 5);
            }
            opponent->actionTimer += 1.0f;
            if (opponent->actionTimer == 10.0f) {
                Hu3DParManAttrSet(opponent->particleId, 1);
            }
            break;
        case 0xFAF:
            if (lbl1Bss1A4->work[0] >= 0x403) {
                opponent->actionState = opponent->actionState + 1;
                Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            break;
        case 0xFB0:
            break;
        default:
            OSReport("*** minik mode error(%d)!!\n", opponent->actionState);
            break;
    }

    if ((opponent->stateFlags2 & 0x100) && (Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) && (Hu3DMotionEndCheck(object->mdlId[0]) != 0)) {
        opponent->verticalOffset -= 1.0f;
        if (0.0f > opponent->verticalOffset) {
            opponent->stateFlags2 &= ~0x100;
            opponent->impactState = opponent->collisionState;
            Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[opponent->collisionState], 0.0f, 8.0f, 0x40000001);
        }
    }
    opponent->actionTimer2 = lbl1DataA0C[0].score;
    opponentDisplayPosition = &lbl1Bss34[object->work[0]];
    if (opponent->stateFlags2 & 2) {
        *opponentDisplayPosition = opponent->playerWork.worldPosition;
        opponentDisplayPosition->y += 100.0f;
        opponent->motionTimer += 4.0f;
        if (360.0f <= opponent->motionTimer) {
            opponent->motionTimer -= 360.0f;
        }
        bobbingHeight = opponent->playerWork.worldPosition.y + (8.0 * sind(opponent->motionTimer));
        omSetTra(object, 0.0f, 0.0f, 0.0f);
        omSetRot(object, 0.0f, 0.0f, 0.0f);
        Hu3DModelPosSet(object->mdlId[4], opponent->playerWork.worldPosition.x, bobbingHeight, opponent->playerWork.worldPosition.z);
        Hu3DModelRotSet(object->mdlId[4], 0.0f, opponent->playerWork.rotationY, 0.0f);
    }
    else if ((opponent->stateFlags2 & 1) != 0) {
        ModelObjectWorldPositionGet(lbl1Bss1A0->mdlId[0], opponent->animationName, opponentDisplayPosition, Hu3DData[object->mdlId[0]].mtx);
    }
    else {
        *opponentDisplayPosition = opponent->playerWork.worldPosition;
        omSetTra(opponent->playerWork.object, opponent->playerWork.worldPosition.x, opponent->playerWork.worldPosition.y, opponent->playerWork.worldPosition.z);
        omSetRot(opponent->playerWork.object, 0.0f, opponent->playerWork.rotationY, 0.0f);
    }
    ArenaModelShadowsSet(object, opponentDisplayPosition, 5);
    if (opponent->targetState != 0) {
        lbl1BssD8++;
        VECAdd(&lbl1BssCC, &opponent->playerWork.worldPosition, &lbl1BssCC);
    }
}

void ArenaOpponentObjectInit(OMOBJ *object)
{
    Vec startingPosition;

    ArenaOpponentState *opponent;
    s32 modelIndex;
    s32 *motionResourceIds;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(ArenaOpponentState), HU_MEMNUM_OVL);
    opponent = object->data;
    opponent->actionState = 0xFA0;
    opponent->stateFlags2 = 0;
    opponent->targetState = 0;
    opponent->particleId = -1;
    opponent->impactState = 7;
    startingPosition.x = -300.0f + (600.0f * object->work[0]);
    startingPosition.y = 1500.0f;
    startingPosition.z = -600.0f;
    PlayerMovementWorkInit(&opponent->playerWork, object, 4, 8, lbl1Data1AC, 0, &startingPosition, 0.0f);
    opponent->playerWork.stateFlags |= 0x1000;
    for (modelIndex = 0; modelIndex < 5; modelIndex++) {
        object->mdlId[modelIndex] = Hu3DModelCreateFile(lbl1Data100[modelIndex]);
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
        Hu3DModelLayerSet(object->mdlId[modelIndex], 3);
        Hu3DModelShadowSet(object->mdlId[modelIndex]);
    }
    opponent->turnTimer = 1.0f;
    opponent->collisionData = lbl1Data204;
    motionResourceIds = lbl1Data150;
    if (object->work[0] == 0) {
        opponent->collisionData = lbl1Data1DC;
        motionResourceIds = lbl1Data114;
        Hu3DModelHookSet(object->mdlId[0], "g007m0-itemhook_r", object->mdlId[1]);
    }
    for (modelIndex = 0; modelIndex < 0xF; modelIndex++) {
        object->mtnId[modelIndex] = Hu3DJointMotionFile(object->mdlId[0], motionResourceIds[modelIndex]);
    }
    Hu3DMotionSet(object->mdlId[0], object->mtnId[7]);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    if (object->work[0] == 0) {
        CharNpcDustSet(object->mdlId[0], object->mtnId[1], 0, 0xD);
        CharNpcDustSet(object->mdlId[0], object->mtnId[2], 1, 0xD);
    }
    else {
        CharNpcDustSet(object->mdlId[0], object->mtnId[1], 2, 0xD);
        CharNpcDustSet(object->mdlId[0], object->mtnId[2], 3, 0xD);
    }
    omSetTra(opponent->playerWork.object, opponent->playerWork.worldPosition.x, opponent->playerWork.worldPosition.y, opponent->playerWork.worldPosition.z);
    omSetRot(opponent->playerWork.object, 0.0f, opponent->playerWork.rotationY, 0.0f);
    if (object->work[0] == 0) {
        Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
    }
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    opponent->playerWork.stateFlags &= ~0xC00;
    object->objFunc = (OMOBJFUNC)ArenaOpponentHookDropUpdate;
}

Vec lbl1Data464 = { 7000.0f, 7000.0f, 7000.0f };
Vec lbl1Data470 = { 0.0f, 1.0f, 0.0f };
Vec lbl1Data47C = { 0.0f, 0.0f, 0.0f };
Vec lbl1Data488 = { 0.0f, 2200.0f, 1500.0f };
Vec lbl1Data494 = { 0.0f, 1800.0f, 0.0f };
Vec lbl1Data4A0 = { 0.0f, 1800.f, 650.0f };
Vec lbl1Data4AC = { 0.0f, 1700.0f, -650.0f };
Vec lbl1Data4B8 = { 0.0f, 1850.0f, 450.0f };
Vec lbl1Data4C4 = { 0.0f, 1775.0f, -50.0f };
Vec lbl1Data4D0 = { 0.0f, 1850.0f, 550.0f };
Vec lbl1Data4DC = { 0.0f, 1775.0f, -50.0f };
Vec lbl1Data4E8 = { 0.0f, 3000.0f, 1000.0f };
Vec lbl1Data4F4 = { 0.0f, -3.0f, -1.0f };
GXColor lbl1Data500 = { 0xFF, 0xFF, 0xFF, 0xFF };
Vec lbl1Data504 = { 0.0f, 8000.0f, 1000.0f };
Vec lbl1Data510 = { 0.0f, 0.0f, 0.0f };
Vec lbl1Data51C[4] = {
    { 1500.0f, 700.0f, 300.0f },
    { 1500.0f, 1200.0f, 200.0f },
    { 2400.0f, 1400.0f, 400.0f },
    { 2500.0f, 800.0f, -200.0f },
};

s32 characterDifficultyTable[8];

void ScoreDisplayInit(void)
{
    ScoreDisplayState *scoreDisplay;
    s32 digitIndex;
    s32 playerIndex;
    s32 spriteX;
    s32 spriteCount;

    lbl1DataA0C[0].extraSprite = characterDifficultyTable[GWPlayerCfg[lbl1Bss120].character];
    for (scoreDisplay = lbl1DataA0C, playerIndex = 0; playerIndex < 2; playerIndex++, scoreDisplay++) {
        spriteCount = scoreDisplay->digitCount + 3;
        scoreDisplay->digitSprites = HuMemDirectMallocNum(HEAP_HEAP, spriteCount * 4, HU_MEMNUM_OVL);
        scoreDisplay->digitSprites[0] = espEntry(scoreDisplay->hundredsSprite, 0, 0);
        scoreDisplay->digitSprites[1] = espEntry(scoreDisplay->tensSprite, 0, 0);
        scoreDisplay->digitSprites[2] = espEntry(scoreDisplay->extraSprite, 0, 0);
        for (digitIndex = 3; digitIndex < spriteCount; digitIndex++) {
            scoreDisplay->digitSprites[digitIndex] = espEntry(scoreDisplay->onesSprite, 0, 0);
        }
        spriteX = 0x30;
        if (playerIndex != 0) {
            spriteX = 0x210;
        }
        for (digitIndex = 0; digitIndex < spriteCount; digitIndex++) {
            espDrawNoSet(scoreDisplay->digitSprites[digitIndex], 0);
            espDispOff(scoreDisplay->digitSprites[digitIndex]);
            espAttrSet(scoreDisplay->digitSprites[digitIndex], HUSPR_ATTR_NOANIM);
            espBankSet(scoreDisplay->digitSprites[digitIndex], 0);
            espPriSet(scoreDisplay->digitSprites[digitIndex], 0x80 - playerIndex);
            espPosSet(scoreDisplay->digitSprites[digitIndex], spriteX, 225.0f);
        }
        espTPLvlSet(scoreDisplay->digitSprites[0], 0.5f);
        espColorSet(scoreDisplay->digitSprites[0], 0, 0, 0);
        espPosSet(scoreDisplay->digitSprites[2], spriteX - 2, 83.0f);
        for (digitIndex = 3; digitIndex < spriteCount; digitIndex++) {
            espPosSet(scoreDisplay->digitSprites[digitIndex], spriteX - 1, ((digitIndex - 3) * 0x1A) + 0x7B);
        }
    }
}

void ScoreDisplayShow(s32 scoreDisplayIndex)
{
    s32 digitIndex;

    for (digitIndex = 0; digitIndex < (lbl1DataA0C[scoreDisplayIndex].digitCount + 3); digitIndex++) {
        espDispOn(lbl1DataA0C[scoreDisplayIndex].digitSprites[digitIndex]);
    }
}

s32 ScoreDisplaySubtractPoints(s32 scoreDisplayIndex, s32 pointsToSubtract)
{
    ScoreDisplayState *scoreDisplay;
    s32 digitIndex;
    s32 scoreAfterSubtraction;

    scoreDisplay = &lbl1DataA0C[scoreDisplayIndex];
    if (pointsToSubtract == 0) {
        pointsToSubtract = scoreDisplay->digitCount + 1;
    }
    scoreDisplay->score -= pointsToSubtract;
    scoreAfterSubtraction = scoreDisplay->score;
    if (scoreDisplay->score < 0) {
        scoreDisplay->score = 0;
    }
    for (digitIndex = 0; digitIndex < scoreDisplay->digitCount; digitIndex++) {
        if (scoreDisplay->score <= digitIndex) {
            espBankSet(scoreDisplay->digitSprites[digitIndex + 3], 1);
        }
    }
    return scoreAfterSubtraction;
}

void ArenaSparkleObjectUpdate(OMOBJ *object)
{
    Vec cameraFacingDirection;
    float sparkleDepthScale;
    float sparkleHorizontalScale;
    float sparkleRotation;

    HU3DCAMERA *camera;

    sparkleDepthScale = 1.0f;
    sparkleHorizontalScale = 1.0f;
    if (lbl1Bss1A4->work[0] > 0x3EC) {
        sparkleDepthScale = 0.8f;
        sparkleHorizontalScale = 0.4f;
    }
    if (object->work[0] != 0) {
        if (lbl1Bss1A4->work[0] <= 0x3EB) {
            object->work[0] = object->work[0] - (3.0f - (lbl1Bss128 / 120.0f));
            if (object->work[0] > 10 * REFRESH_RATE) {
                object->work[0] = 1;
            }
        }
        if (--object->work[0] == 0) {
            camera = Hu3DCamera;
            VECSubtract(&camera->pos, &camera->target, &cameraFacingDirection);
            sparkleRotation = (atan2d(cameraFacingDirection.x, cameraFacingDirection.z)) + ((90.0f * frandf()) - 45.0f);
            omSetRot(object, 0.0f, sparkleRotation, 0.0f);
            omSetSca(object, sparkleHorizontalScale, sparkleDepthScale, sparkleHorizontalScale);
            Hu3DMotionTimeSet(object->mdlId[0], 0.0f);
            Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
            HuAudFXPlay(0x7C0);
        }
    }
    else if (Hu3DMotionEndCheck(object->mdlId[0]) != 0) {
        object->work[0] = frandmod(10 * REFRESH_RATE);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    }
}

void ArenaSparkleObjectInit(OMOBJ *object)
{
    object->work[0] = frandmod(10 * REFRESH_RATE);
    omSetTra(object, 0.0f, 0.0f, 0.0f);
    omSetRot(object, 0.0f, 0.0f, 0.0f);
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M450, 0x22));
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[0], 1);
    object->objFunc = ArenaSparkleObjectUpdate;
}

MotionResourceInfo lbl1Data5CC[8] = {
    { "c000m1-ske_head", 0x100, 1, 183.0f },
    { "c001m1-ske_head", 0x100, 1, 183.0f },
    { "c002m1-ske_head", 0x80, 0, 100.0f },
    { "c003m1-ske_head", 0x80, 0, 90.0f },
    { "c004m1-ske_head", 0x80, 0, 83.0f },
    { "c005m1-ske_head", 0x80, 0, 112.0f },
    { "c006m1-ske_head", 0x80, 0, 100.0f },
    { "c007m1-ske_head", 0x80, 0, 70.0f },
};
s32 lbl1Data64C[8][0x17] = {
    {
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x08),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x0A),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x09),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x13),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x16),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x19),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x1A),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x36),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x6D),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x1F),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x1C),
        DATA_MAKE_NUM(DATADIR_M450, 0x00),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x0D),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x0E),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x10),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x11),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x7F),
    },
    {
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x02),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x03),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x08),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x0A),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x09),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x13),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x16),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x19),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x1A),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x36),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x6D),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x1F),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x1C),
        DATA_MAKE_NUM(DATADIR_M450, 0x01),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x0D),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x0E),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x10),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x11),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x7F),
    },
    {
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x02),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x03),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x08),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x0A),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x09),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x13),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x16),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x19),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x1A),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x36),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x6D),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x1F),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x1C),
        DATA_MAKE_NUM(DATADIR_M450, 0x02),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x0D),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x0E),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x10),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x11),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x7F),
    },
    {
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x02),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x03),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x08),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x0A),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x09),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x13),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x16),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x19),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x1A),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x36),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x6D),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x1F),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x1C),
        DATA_MAKE_NUM(DATADIR_M450, 0x03),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x0D),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x0E),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x10),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x11),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x7F),
    },
    {
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x02),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x03),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x08),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x0A),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x09),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x13),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x16),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x19),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x1A),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x36),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x6D),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x1F),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x1C),
        DATA_MAKE_NUM(DATADIR_M450, 0x04),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x0D),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x0E),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x10),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x11),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x7F),
    },
    {
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x02),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x03),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x08),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x0A),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x09),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x13),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x16),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x19),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x1A),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x36),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x6D),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x1F),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x1C),
        DATA_MAKE_NUM(DATADIR_M450, 0x05),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x0D),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x0E),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x10),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x11),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x7F),
    },
    {
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x02),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x03),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x08),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x0A),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x09),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x13),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x16),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x19),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x1A),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x36),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x6D),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x1F),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x1C),
        DATA_MAKE_NUM(DATADIR_M450, 0x06),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x0D),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x0E),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x10),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x11),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x7F),
    },
    {
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x02),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x03),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x08),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x0A),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x09),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x13),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x16),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x19),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x1A),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x36),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x6D),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x1F),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x1C),
        DATA_MAKE_NUM(DATADIR_M450, 0x07),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x0D),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x0E),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x10),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x11),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x7F),
    },
};
s32 lbl1Data92C[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
ArenaWaypoint lbl1Data94C[8] = {
    { 9, 1.0f, 8.0f, 0.0f, 0 },
    { 0xA, 1.0f, 0.0f, 0.0f, 0 },
    { 0xB, 2.5f, 8.0f, 0.0f, 0 },
    { 0xC, 1.0f, 8.0f, 61.0f, 0x40000002 },
    { -1, 1.0f, 0.0f, 0.0f, 0 },
    { 0xD, 1.0f, 8.0f, 0.0f, 0 },
    { 0, 1.0f, 16.0f, 0.0f, 0x40000001 },
    { -1, 1.0f, 0.0f, 0.0f, 0 },
};
s32 characterDifficultyTable[8] = {
    0x610000,
    0x610001,
    0x610002,
    0x610003,
    0x610004,
    0x610005,
    0x610006,
    0x610007,
};
ScoreDisplayState lbl1DataA0C[2] = {
    { 0x00510065, DATA_MAKE_NUM(DATADIR_M450, 0x64), DATA_MAKE_NUM(DATADIR_M450, 0x68), DATA_MAKE_NUM(DATADIR_M450, 0x6B), 0xA, 0xA, NULL },
    { 0x00510067, DATA_MAKE_NUM(DATADIR_M450, 0x66), DATA_MAKE_NUM(DATADIR_M450, 0x69), DATA_MAKE_NUM(DATADIR_M450, 0x6A), 5, 5, NULL },
};
ArenaObjectState lbl1DataA80[9] = {
    { "m450mex06-s2hook01", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 135.0f, 36.0f / REFRESH_RATE, 350.0f }, -1, 0 },
    { "m450mex06-s2hook01", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 315.0f, 36.0f / REFRESH_RATE, 350.0f }, -2, 0 },
    { "m450mex06-s2hook02", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 45.0f, -36.0f / REFRESH_RATE, 350.0f }, -1, 0 },
    { "m450mex06-s2hook02", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 225.0f, -36.0f / REFRESH_RATE, 350.0f }, -2, 0 },
    { "m450mex06-s2hook03", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 0.0f, 36.0f / REFRESH_RATE, 350.0f }, -1, 0 },
    { "m450mex06-s2hook03", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 120.0f, 36.0f / REFRESH_RATE, 350.0f }, -2, 0 },
    { "m450mex06-s2hook03", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 240.0f, 36.0f / REFRESH_RATE, 350.0f }, -2, 0 },
    { "m450mex06-s2hook04", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 0.0f, 36.0f / REFRESH_RATE, 300.0f }, -1, 0 },
    { "m450mex06-s2hook04", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 180.0f, 36.0f / REFRESH_RATE, 300.0f }, -2, 0 },
};
ArenaObjectState lbl1DataC58[0xA] = {
    { "m450mex06-s4hook01", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 60.0f, -36.0f / REFRESH_RATE, 300.0f }, -1, 0 },
    { "m450mex06-s4hook01", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 180.0f, -36.0f / REFRESH_RATE, 300.0f }, -2, 0 },
    { "m450mex06-s4hook01", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 300.0f, -36.0f / REFRESH_RATE, 300.0f }, -2, 0 },
    { "m450mex06-s4hook03", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 135.0f, -72.0f / REFRESH_RATE, 300.0f }, -1, 0 },
    { "m450mex06-s4hook03", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 315.0f, -72.0f / REFRESH_RATE, 300.0f }, -2, 0 },
    { "m450mex06-s4hook02", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 225.0f, -36.0f / REFRESH_RATE, 300.0f }, -1, 0 },
    { "m450mex06-s4hook02", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 45.0f, -36.0f / REFRESH_RATE, 300.0f }, -2, 0 },
    { "m450mex06-s4hook04", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 0.0f, 72.0f / REFRESH_RATE, 300.0f }, -1, 0 },
    { "m450mex06-s4hook04", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 120.0f, 72.0f / REFRESH_RATE, 300.0f }, -2, 0 },
    { "m450mex06-s4hook04", { 0.0f, 0.0f, 0.0f }, -1, 0.0f, { 240.0f, 72.0f / REFRESH_RATE, 300.0f }, -2, 0 },
};
char *lbl1DataE68[5] = {
    "m450m01-from1to2",
    "m450m01-from2to3",
    "m450m01-from3to4",
    "m450m01-from4to5",
    "m450m01-from5to6",
};
s32 lbl1DataE7C[0x21] = {
    DATA_MAKE_NUM(DATADIR_M450, 0x10),
    DATA_MAKE_NUM(DATADIR_M450, 0x11),
    DATA_MAKE_NUM(DATADIR_M450, 0x12),
    DATA_MAKE_NUM(DATADIR_M450, 0x13),
    DATA_MAKE_NUM(DATADIR_M450, 0x14),
    DATA_MAKE_NUM(DATADIR_M450, 0x15),
    DATA_MAKE_NUM(DATADIR_M450, 0x16),
    DATA_MAKE_NUM(DATADIR_M450, 0x16),
    DATA_MAKE_NUM(DATADIR_M450, 0x17),
    DATA_MAKE_NUM(DATADIR_M450, 0x18),
    DATA_MAKE_NUM(DATADIR_M450, 0x19),
    DATA_MAKE_NUM(DATADIR_M450, 0x1A),
    DATA_MAKE_NUM(DATADIR_M450, 0x1B),
    DATA_MAKE_NUM(DATADIR_M450, 0x1C),
    DATA_MAKE_NUM(DATADIR_M450, 0x1D),
    DATA_MAKE_NUM(DATADIR_M450, 0x1E),
    DATA_MAKE_NUM(DATADIR_M450, 0x1F),
    DATA_MAKE_NUM(DATADIR_M450, 0x16),
    DATA_MAKE_NUM(DATADIR_M450, 0x16),
    DATA_MAKE_NUM(DATADIR_M450, 0x20),
    DATA_MAKE_NUM(DATADIR_M450, 0x21),
    DATA_MAKE_NUM(DATADIR_M450, 0x21),
    DATA_MAKE_NUM(DATADIR_M450, 0x21),
    DATA_MAKE_NUM(DATADIR_M450, 0x21),
    DATA_MAKE_NUM(DATADIR_M450, 0x21),
    DATA_MAKE_NUM(DATADIR_M450, 0x28),
    DATA_MAKE_NUM(DATADIR_M450, 0x28),
    DATA_MAKE_NUM(DATADIR_M450, 0x28),
    DATA_MAKE_NUM(DATADIR_M450, 0x29),
    DATA_MAKE_NUM(DATADIR_M450, 0x2A),
    DATA_MAKE_NUM(DATADIR_M450, 0x2A),
    DATA_MAKE_NUM(DATADIR_M450, 0x2B),
    DATA_MAKE_NUM(DATADIR_M450, 0x2B),
};
s32 lbl1DataF00[0x17] = {
    DATA_MAKE_NUM(DATADIR_M450, 0x08),
    DATA_MAKE_NUM(DATADIR_M450, 0x09),
    DATA_MAKE_NUM(DATADIR_M450, 0x0B),
    DATA_MAKE_NUM(DATADIR_M450, 0x0C),
    DATA_MAKE_NUM(DATADIR_M450, 0x0E),
    DATA_MAKE_NUM(DATADIR_M450, 0x0A),
    DATA_MAKE_NUM(DATADIR_M450, 0x0A),
    DATA_MAKE_NUM(DATADIR_M450, 0x0A),
    DATA_MAKE_NUM(DATADIR_M450, 0x0A),
    DATA_MAKE_NUM(DATADIR_M450, 0x0D),
    DATA_MAKE_NUM(DATADIR_M450, 0x0D),
    DATA_MAKE_NUM(DATADIR_M450, 0x0D),
    DATA_MAKE_NUM(DATADIR_M450, 0x0D),
    DATA_MAKE_NUM(DATADIR_M450, 0x0D),
    DATA_MAKE_NUM(DATADIR_M450, 0x0D),
    DATA_MAKE_NUM(DATADIR_M450, 0x0D),
    DATA_MAKE_NUM(DATADIR_M450, 0x0D),
    DATA_MAKE_NUM(DATADIR_M450, 0x0D),
    DATA_MAKE_NUM(DATADIR_M450, 0x0F),
    DATA_MAKE_NUM(DATADIR_M450, 0x0F),
    DATA_MAKE_NUM(DATADIR_M450, 0x0F),
    DATA_MAKE_NUM(DATADIR_M450, 0x0F),
    DATA_MAKE_NUM(DATADIR_M450, 0x0F),
};
s32 lbl1DataF5C[4]
    = { DATA_MAKE_NUM(DATADIR_M450, 0x24), DATA_MAKE_NUM(DATADIR_M450, 0x25), DATA_MAKE_NUM(DATADIR_M450, 0x26), DATA_MAKE_NUM(DATADIR_M450, 0x27) };
s32 lbl1DataF6C[6] = { 0, 60 * REFRESH_RATE, 300 * REFRESH_RATE, 60 * REFRESH_RATE, 300 * REFRESH_RATE, 300 * REFRESH_RATE };
float lbl1DataF84[5] = { 1200.0f, 1500.0f, 0.0f, 200.0f, 300.0f };
float lbl1DataF98[5] = { 1200.0f, 1500.0f, 0.0f, 200.0f, 300.0f };
float *lbl1DataFAC[6] = { NULL, lbl1DataF84, NULL, lbl1DataF98, NULL, NULL };
s32 lbl1DataFC4[5] = { 5, 4, 3, 2, 0 };
s32 lbl1DataFD8[5] = { 3, 4, 5, 2, 2 };
s32 lbl1DataFEC[5] = { 3, 4, 2, 5, 3 };
s32 lbl1Data1000[5] = { 4, 2, 3, 5, 3 };
s32 lbl1Data1014[0xA] = { 9, 0x10, 0xA, 8, 0xC, 0xD, 0xB, 0xE, 0xF, 1 };
s32 lbl1Data103C[0xA] = { 9, 0x10, 0xA, 8, 0xB, 0xC, 0xE, 0xF, 0xD, 1 };
s32 lbl1Data1064[0xA] = { 8, 0xC, 9, 0xB, 0xA, 0x10, 0xE, 0xF, 0xD, 5 };
s32 lbl1Data108C[0xA] = { 8, 0xC, 9, 0xB, 0xF, 0xA, 0x10, 0xE, 0xD, 6 };
s32 lbl1Data10B4[0xA] = { 8, 0xA, 0x10, 0xC, 9, 0xD, 0xB, 0xE, 0xF, 2 };
s32 lbl1Data10DC[0xA] = { 8, 0xA, 0xC, 0xB, 9, 0xD, 0xE, 0xF, 0x10, 8 };
s32 lbl1Data1104[0xA] = { 8, 0xC, 9, 0x10, 0xF, 0xA, 0xB, 0xE, 0xD, 3 };
s32 lbl1Data112C[0xA] = { 0xB, 8, 0xA, 9, 0xD, 0x10, 0xE, 0xC, 0xF, 5 };
s32 lbl1Data1154[0xA] = { 9, 0xC, 0xA, 8, 0xD, 0x10, 0xB, 0xE, 0xF, 5 };
s32 lbl1Data117C[0xA] = { 9, 0xB, 0xA, 8, 0xF, 0xC, 0x10, 0xE, 0xD, 6 };
s32 lbl1Data11A4[0xA] = { 8, 9, 0x10, 0xE, 0xD, 0xA, 0xC, 0xB, 0xF, 2 };
s32 lbl1Data11CC[0xA] = { 0xB, 8, 0xA, 0xE, 9, 0xD, 0xC, 0xF, 0x10, 8 };
s32 *lbl1Data11F4[4] = { lbl1DataFC4, lbl1DataFD8, lbl1DataFEC, lbl1Data1000 };
s32 *lbl1Data1204[3] = { lbl1Data1014, lbl1Data103C, lbl1Data1064 };
s32 *lbl1Data1210[3] = { lbl1Data108C, lbl1Data10B4, lbl1Data10DC };
s32 *lbl1Data121C[3] = { lbl1Data1104, lbl1Data112C, lbl1Data1154 };
s32 *lbl1Data1228[3] = { lbl1Data117C, lbl1Data11A4, lbl1Data11CC };
s32 lbl1Data1234[8] = { 0xFFFFFFFF, 0, 1, 0, 0, 0xFFFFFFFF, 0, 1 };

char *lbl1Data12A0[4] = {
    "m450mex06-s3hook01",
    "m450mex06-s3hook02",
    "m450mex06-s3hook03",
    "m450mex06-s3hook04",
};

char *lbl1Data135C[9] = {
    "m450mex06-s5hook03",
    "m450mex06-s5hook02",
    "m450mex06-s5hook01",
    "m450mex06-s5hook06",
    "m450mex06-s5hook05",
    "m450mex06-s5hook04",
    "m450mex06-s5hook09",
    "m450mex06-s5hook08",
    "m450mex06-s5hook07",
};
ArenaModelLayout arenaLayout = { lbl1Data12A0, 300.0f, 4, 2, -1, 0, 1, 0, 2, NULL, NULL, NULL, NULL, NULL };
ArenaModelLayout lbl1Data13B8 = { lbl1Data135C, 200.0f, 9, 3, -1, 0, 1, 0, 8, NULL, NULL, NULL, NULL, NULL };
ArenaObjectAnimationState lbl1Data1450[5] = {
    { "m450mex06-s6hook01", { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0, 0, 0 },
    { "m450mex06-s6hook02", { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0, 0, 0 },
    { "m450mex06-s6hook03", { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0, 0, 0 },
    { "m450mex06-s6hook04", { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0, 0, 0 },
    { "m450mex06-s6hook05", { 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, 0, 0, 0 },
};

HU3DPARMANPARAM particleParameters = {
    0x1E,
    { 0, 0 },
    1.0f,
    30.0f,
    360.0f,
    { 0.0f, 0.1f, 0.0f },
    1.0f,
    1.0f,
    50.0f,
    1.08f,
    1,
    {
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF },
    },
    {
        { 0xFF, 0xFF, 0xFF, 0 },
        { 0xFF, 0xFF, 0xFF, 0 },
        { 0xFF, 0xFF, 0xFF, 0 },
        { 0xFF, 0xFF, 0xFF, 0 },
    },
};

void ArenaFloatingObjectUpdate(OMOBJ *object)
{
    float spawnX;
    float spawnZ;

    if (object->work[0] != 0) {
        if (lbl1Bss108 != 0) {
            if (--object->work[0] == 0) {
                spawnX = (1500.0f * frandf()) - 750.0f;
                spawnZ = (1500.0f * frandf()) - 750.0f;
                omSetTra(object, spawnX, 1380.0f, spawnZ);
                Hu3DMotionTimeSet(object->mdlId[0], 0.0f);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
            }
        }
    }
    else if (Hu3DMotionEndCheck(object->mdlId[0]) != 0) {
        object->work[0] = frandmod(3 * REFRESH_RATE);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    }
}

void ArenaFloatingObjectInit(OMOBJ *object)
{
    object->work[0] = frandmod(3 * REFRESH_RATE);
    omSetTra(object, 0.0f, 0.0f, 0.0f);
    omSetRot(object, 0.0f, 0.0f, 0.0f);

    object->mdlId[0] = ModelResourceGetOrCreate(DATA_MAKE_NUM(DATADIR_M450, 0x23));
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[0], 1);
    object->objFunc = ArenaFloatingObjectUpdate;
}

void NoopObjectUpdate(OMOBJ *object) { }

void ArenaLineObjectInit(OMOBJ *object)
{
    omSetTra(object, 0.0f, 0.0f, 0.0f);
    omSetRot(object, 0.0f, 0.0f, 0.0f);
    object->mdlId[0] = Hu3DModelCreateFile(lbl1DataF00[object->work[0]]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    object->objFunc = NoopObjectUpdate;
}

void ArenaLineBetweenWaypointsSet(s32 firstWaypointIndex, s32 secondWaypointIndex, s32 modelIndex)
{
    Vec waypointMidpoint = lbl1Data1450[firstWaypointIndex].position;

    VECAdd(&lbl1Data1450[secondWaypointIndex].position, &waypointMidpoint, &waypointMidpoint);
    VECScale(&waypointMidpoint, &waypointMidpoint, 0.5f);
    waypointMidpoint.y = 1501.0f;
    Hu3DModelPosSet(lbl1Bss1A0->mdlId[modelIndex], waypointMidpoint.x, waypointMidpoint.y, waypointMidpoint.z);
    Hu3DModelRotSet(
        lbl1Bss1A0->mdlId[modelIndex], 0.0f, atan2d(lbl1Data1450[firstWaypointIndex].position.x - waypointMidpoint.x, lbl1Data1450[firstWaypointIndex].position.z - waypointMidpoint.z), 0.0f);
    Hu3DModelScaleSet(
        lbl1Bss1A0->mdlId[modelIndex], 1.0f, 1.0f, VECDistance(&lbl1Data1450[firstWaypointIndex].position, &lbl1Data1450[secondWaypointIndex].position) / 1000.0f);
    Hu3DMotionTimeSet(lbl1Bss1A0->mdlId[modelIndex], 0.0f);
    Hu3DModelAttrReset(lbl1Bss1A0->mdlId[modelIndex], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(lbl1Bss1A0->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
}

void ArenaModelAnimationTrigger(Vec *position, s32 modelIndex)
{
    Hu3DModelPosSet(lbl1Bss1A0->mdlId[modelIndex], position->x, position->y, position->z);
    Hu3DMotionTimeSet(lbl1Bss1A0->mdlId[modelIndex], 0.0f);
    Hu3DModelAttrReset(lbl1Bss1A0->mdlId[modelIndex], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(lbl1Bss1A0->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
    HuAudFXPlay(0x7B7);
}

void ArenaModelLayoutUpdate(ArenaModelLayout *layout)
{
    s32 modelIndex;
    s32 modelId;

    if (layout->resetModels != 0) {
        layout->resetModels = 0;
        layout->modelsVisible = 0;
        for (modelIndex = 0; modelIndex < layout->modelCount; modelIndex++) {
            if (layout->modelIds[modelIndex] >= 0) {
                modelId = lbl1Bss1A0->mdlId[layout->modelIds[modelIndex]];
                MTXIdentity(Hu3DData[modelId].mtx);
                if (layout->modelCount == 4) {
                    Hu3DModelRotSet(lbl1Bss1A0->mdlId[layout->modelIds[modelIndex]], 0.0f, 90.0f, 0.0f);
                }
                else {
                    Hu3DModelRotSet(lbl1Bss1A0->mdlId[layout->modelIds[modelIndex]], 0.0f, -90.0f, 0.0f);
                }
            }
        }
    }
    for (modelIndex = 0; modelIndex < layout->modelCount; modelIndex++) {
        if (layout->modelsVisible != 0) {
            if (layout->modelIds[modelIndex] < 0) {
                ModelObjectWorldPositionGet(lbl1Bss1A0->mdlId[0], layout->modelNames[modelIndex], &layout->modelPositions[modelIndex], NULL);
            }
            else {
                modelId = lbl1Bss1A0->mdlId[layout->modelIds[modelIndex]];
                ModelObjectWorldPositionGet(lbl1Bss1A0->mdlId[0], layout->modelNames[modelIndex], &layout->modelPositions[modelIndex], Hu3DData[modelId].mtx);
            }
        }
        else if (layout->modelIds[modelIndex] >= 0) {
            Hu3DModelPosSet(lbl1Bss1A0->mdlId[layout->modelIds[modelIndex]], layout->modelPositions[modelIndex].x, layout->modelPositions[modelIndex].y, layout->modelPositions[modelIndex].z);
        }
    }
    for (modelIndex = 0; modelIndex < layout->modelCount; modelIndex++) {
        if (layout->objectIds[modelIndex] >= 0) {
            omSetTra(lbl1Bss144[layout->objectIds[modelIndex]], layout->modelPositions[modelIndex].x, layout->modelPositions[modelIndex].y, layout->modelPositions[modelIndex].z);
        }
    }
}

void ArenaModelLayoutInit(ArenaModelLayout *layout, s32 *modelLinks, s32 firstObjectIndex)
{
    s32 modelIndex;
    Vec *modelVelocity;

    layout->modelPositions = HuMemDirectMallocNum(HEAP_HEAP, layout->modelCount * sizeof(Vec), HU_MEMNUM_OVL);
    layout->modelVelocities = HuMemDirectMallocNum(HEAP_HEAP, layout->modelCount * sizeof(Vec), HU_MEMNUM_OVL);
    layout->modelIds = HuMemDirectMallocNum(HEAP_HEAP, layout->modelCount * sizeof(s32), HU_MEMNUM_OVL);
    layout->objectIds = HuMemDirectMallocNum(HEAP_HEAP, layout->modelCount * sizeof(s32), HU_MEMNUM_OVL);
    layout->modelLinkIds = modelLinks;
    for (modelIndex = 0; modelIndex < layout->modelCount; modelIndex++) {
        layout->modelIds[modelIndex] = *modelLinks++;
        layout->objectIds[modelIndex] = firstObjectIndex++;
        modelVelocity = &layout->modelVelocities[modelIndex];
        modelVelocity->x = 0.0f;
        modelVelocity->y = 0.0f;
        modelVelocity->z = 0.0f;
    }
    layout->selectedModelIndex = layout->modelIds[*modelLinks];
    layout->modelIds[*modelLinks] = -1;
    layout->objectIds[*modelLinks] = -1;
    ArenaModelLayoutUpdate(layout);
    for (modelIndex = 0; modelIndex < layout->modelCount; modelIndex++) {
        if (layout->modelIds[modelIndex] >= 0) {
            Hu3DModelAttrReset(lbl1Bss1A0->mdlId[layout->modelIds[modelIndex]], HU3D_ATTR_DISPOFF);
        }
    }
}

void ArenaSelectedModelMove(void)
{
    Vec modelMovement;
    s32 modelIndex;
    s32 modelId;

    if ((0 > lbl1BssF8) || (NULL == lbl1BssFC)) {
        return;
    }
    VECSubtract(&lbl1BssEC, &lbl1BssFC->modelPositions[lbl1BssF8], &modelMovement);
    if (10.0f < VECMag(&modelMovement)) {
        VECNormalize(&modelMovement, &modelMovement);
        VECScale(&modelMovement, &modelMovement, 10.0f);
    }
    lbl1BssFC->modelVelocities[lbl1BssF8] = modelMovement;
    VECAdd(&modelMovement, &lbl1BssFC->modelPositions[lbl1BssF8], &lbl1BssFC->modelPositions[lbl1BssF8]);
    if ((modelMovement.x == 0.0f) && (modelMovement.z == 0.0f)) {
        for (modelId = lbl1BssFC->firstModelIndex, modelIndex = 0; modelIndex < lbl1BssFC->modelCount; modelIndex++, modelId++) {
            if ((lbl1BssFC->modelIds[modelIndex] >= 0) && (modelId != lbl1BssFC->modelIds[modelIndex])) {
                break;
            }
        }
        if (lbl1BssFC->modelCount == modelIndex) {
            lbl1BssFC->layoutComplete = 1;
        }
        lbl1BssFC->modelPositions[lbl1BssF8] = lbl1BssEC;
        lbl1BssF8 = -1;
        lbl1BssFC = NULL;
        HuAudFXPlay(0x7B9);
    }
}

void ArenaModelLayoutShow(ArenaModelLayout *layout, s32 selectedModelId)
{
    s32 modelIndex;
    s32 modelId;

    layout->modelsVisible = 1;
    layout->modelIds[layout->modelCount - 1] = selectedModelId;
    for (modelIndex = 0; modelIndex < layout->modelCount; modelIndex++) {
        modelId = lbl1Bss1A0->mdlId[layout->modelIds[modelIndex]];
        ModelObjectWorldPositionGet(lbl1Bss1A0->mdlId[0], layout->modelNames[modelIndex], &layout->modelPositions[modelIndex], Hu3DData[modelId].mtx);
        Hu3DModelPosSet(modelId, 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(modelId, 0.0f, 0.0f, 0.0f);
    }
}

void ArenaModelLayoutResetRequest(ArenaModelLayout *layout)
{
    layout->resetModels = 1;
}

void MinigameTypeUpdateIfHigher(s32 minigameType)
{
    if (minigameType > GWMGTypeGet()) {
        GWMGTypeSet(minigameType);
    }
}

s32 MinigameTypeIsAvailable(s32 minigameType)
{
    if (minigameType > GWMGTypeGet()) {
        return 0;
    }
    return 1;
}

void ArenaCourseSetup(s32 courseIndex)
{
    s32 objectIndex;
    ArenaModelLayout *courseLayout;
    s32 modelIndex;

    nMap = 0;
    lbl1Bss118 = courseIndex;
    lbl1BssF8 = -1;
    lbl1BssFC = NULL;
    lbl1Bss110 = 1;
    lbl1Bss304 = lbl1DataF6C[courseIndex];
    courseLayout = NULL;
    switch (courseIndex) {
        case 1:
            MapObject[nMap++] = lbl1Bss144[0];
            break;
        case 2:
            lbl1Bss110 = 2;
            courseLayout = &arenaLayout;
            arenaLayout.resetModels = 1;
            MapObject[nMap++] = lbl1Bss144[1];
            break;
        case 3:
            MapObject[nMap++] = lbl1Bss144[2];
            break;
        case 4:
            lbl1Bss110 = 2;
            courseLayout = &lbl1Data13B8;
            lbl1Data13B8.resetModels = 1;
            MapObject[nMap++] = lbl1Bss144[3];
            break;
        case 5:
            lbl1Bss110 = 3;
            MapObject[nMap++] = lbl1Bss144[4];
            for (objectIndex = 0; objectIndex < 5; objectIndex++) {
                MapObject[nMap++] = lbl1Bss144[objectIndex + 0x12];
            }
            break;
    }
    MinigameTypeUpdateIfHigher(courseIndex);
    if (NULL != courseLayout) {
        for (modelIndex = 0; modelIndex < courseLayout->modelCount; modelIndex++) {
            if (courseLayout->objectIds[modelIndex] >= 0) {
                MapObject[nMap++] = lbl1Bss144[courseLayout->objectIds[modelIndex]];
            }
        }
    }
}

void ArenaModelMotionUpdate(OMOBJ *object)
{
    Vec hookOffset;
    float animationTime;

    ArenaObjectAnimationState *animationState;
    s32 waypointIndex;
    s32 modelId;
    Vec *arenaModelVelocity;
    Vec *courseModelVelocity;

    for (waypointIndex = 0; waypointIndex < arenaLayout.modelCount; waypointIndex++) {
        arenaModelVelocity = &arenaLayout.modelVelocities[waypointIndex];
        arenaModelVelocity->x = 0.0f;
        arenaModelVelocity->y = 0.0f;
        arenaModelVelocity->z = 0.0f;
    }
    for (waypointIndex = 0; waypointIndex < lbl1Data13B8.modelCount; waypointIndex++) {
        courseModelVelocity = &lbl1Data13B8.modelVelocities[waypointIndex];
        courseModelVelocity->x = 0.0f;
        courseModelVelocity->y = 0.0f;
        courseModelVelocity->z = 0.0f;
    }
    ArenaSelectedModelMove();
    ArenaModelLayoutUpdate(&arenaLayout);
    ArenaModelLayoutUpdate(&lbl1Data13B8);
    modelId = object->mdlId[6];
    ModelObjectWorldPositionGet(object->mdlId[0], "m450mex06-s3hook06", &hookOffset, Hu3DData[modelId].mtx);
    modelId = object->mdlId[7];
    ModelObjectWorldPositionGet(object->mdlId[0], "m450mex06-s3hook05", &hookOffset, Hu3DData[modelId].mtx);
    modelId = object->mdlId[17];
    ModelObjectWorldPositionGet(object->mdlId[0], "m450mex06-s5hook11", &hookOffset, Hu3DData[modelId].mtx);
    modelId = object->mdlId[18];
    ModelObjectWorldPositionGet(object->mdlId[0], "m450mex06-s5hook10", &hookOffset, Hu3DData[modelId].mtx);
    modelId = object->mdlId[19];
    ModelObjectWorldPositionGet(object->mdlId[0], "m450m01-from5to6", &hookOffset, Hu3DData[modelId].mtx);
    for (waypointIndex = 0; waypointIndex < 5; waypointIndex++) {
        animationState = &lbl1Data1450[waypointIndex];
        animationState->previousHeight = animationState->position.y;
        modelId = object->mdlId[waypointIndex + 0x14];
        ModelObjectWorldPositionGet(object->mdlId[0], animationState->objectName, &animationState->position, Hu3DData[modelId].mtx);
        ModelObjectPositionGet(object->mdlId[waypointIndex + 0x14], "m450_s6a1-s6a1hook", &hookOffset);
        VECAdd(&hookOffset, &animationState->position, &animationState->position);
        if (1560.0f > animationState->position.y) {
            animationState->position.y = 1560.0f;
        }
        omSetTra(lbl1Bss144[waypointIndex + 0x12], animationState->position.x, animationState->position.y, animationState->position.z);
        animationState->previousHeight = animationState->position.y - animationState->previousHeight;
        if (animationState->isVisible != 0) {
            if (animationState->animationActive != 0) {
                animationTime = Hu3DMotionTimeGet(object->mdlId[waypointIndex + 0x14]);
                if ((10.0f <= animationTime) && (40.0f >= animationTime)) {
                    animationState->previousHeight = 0.0f;
                }
                if (40.0f <= animationTime) {
                    Hu3DMotionTimeSet(object->mdlId[waypointIndex + 0x14], 10.0f);
                    if (animationState->soundPlayed != 0) {
                        animationState->soundPlayed = 0;
                    }
                }
                if (((ArenaPlayerState *)lbl1Bss138->data)->targetCount < 3) {
                    animationState->animationTimer += 1.0f;
                    if (20 * REFRESH_RATE < animationState->animationTimer) {
                        animationState->animationActive = 0;
                        ArenaPlayerTargetRemove(waypointIndex);
                    }
                }
            }
            else if (Hu3DMotionEndCheck(object->mdlId[waypointIndex + 0x14]) != 0) {
                animationState->isVisible = 0;
            }
        }
    }
}

void ArenaRotationSet(float rotationDegrees, s8 rotationAxis)
{
    Mtx rotationMatrix;
    HU3DMODEL *model;
    s32 modelIndex;

    if (lbl1Bss114 == 0) {
        if (rotationAxis == 0x78) {
            MTXRotRad(rotationMatrix, 0x5A, 0.017453292f * rotationDegrees);
        }
        else {
            MTXRotRad(rotationMatrix, 0x58, 0.017453292f * rotationDegrees);
        }
        MTXConcat(rotationMatrix, lbl1Bss88, rotationMatrix);
        for (modelIndex = 0; modelIndex <= 1; modelIndex++) {
            model = &Hu3DData[lbl1Bss1A0->mdlId[modelIndex]];
            MTXCopy(rotationMatrix, model->mtx);
        }
        ArenaModelMotionUpdate(lbl1Bss1A0);
    }
}

void ArenaRotationQuarterTurn(s8 rotationAxis)
{
    Mtx rotationMatrix;
    s32 modelIndex;
    HU3DMODEL *model;

    lbl1Bss114 = 1;
    if (rotationAxis == 0x78) {
        MTXRotRad(rotationMatrix, 0x5A, 1.5707964f);
    }
    else {
        MTXRotRad(rotationMatrix, 0x58, 1.5707964f);
    }
    MTXConcat(rotationMatrix, lbl1Bss88, lbl1Bss88);
    for (modelIndex = 0; modelIndex <= 1; modelIndex++) {
        model = &Hu3DData[lbl1Bss1A0->mdlId[modelIndex]];
        MTXCopy(lbl1Bss88, model->mtx);
    }
    ArenaModelMotionUpdate(lbl1Bss1A0);
}

void ArenaHookPositionsUpdate(void)
{
    s32 hookIndex;

    for (hookIndex = 0; hookIndex < 5; hookIndex++) {
        ModelObjectWorldPositionGet(lbl1Bss1A0->mdlId[0], lbl1DataE68[hookIndex], &lbl1Bss4C[hookIndex], NULL);
    }
}

void FireEffectArrayInit(ArenaObjectState *fireEffects, s32 fireEffectCount)
{
    s32 fireEffectIndex;

    lbl1Bss104 = fireEffects;
    lbl1Bss100 = fireEffectCount;
    for (fireEffectIndex = 0; fireEffectIndex < lbl1Bss100; fireEffectIndex++, fireEffects++) {
        fireEffects->modelId = -1;
        fireEffects->flags = 1;
    }
}

void FireEffectsStop(void)
{
    ArenaObjectState *fireEffect;
    s32 effectIndex;

    if (NULL != lbl1Bss104) {
        for (fireEffect = lbl1Bss104, effectIndex = 0; effectIndex < lbl1Bss100; effectIndex++, fireEffect++) {
            if (fireEffect->modelId >= 0) {
                ParticleSystemDestroyRequest(fireEffect->modelId);
            }
            if (fireEffect->state >= 0) {
                HuAudFXStop(fireEffect->state);
                fireEffect->state = -2;
            }
        }
    }
}

void FireEffectsUpdate(OMOBJ *object)
{
    ArenaObjectState *fireEffect;
    s32 activeEffectCount;
    s32 effectIndex;
    s32 soundAvailable;
    s32 firstEffectPending;

    if (NULL != lbl1Bss104) {
        soundAvailable = 1;
        firstEffectPending = 1;
        activeEffectCount = 0;
        for (fireEffect = lbl1Bss104, effectIndex = 0; effectIndex < lbl1Bss100; effectIndex++, fireEffect++) {
            if ((firstEffectPending != 0) && (fireEffect->flags != 0)) {
                activeEffectCount++;
                firstEffectPending = 0;
                fireEffect->flags = 0;
                fireEffect->animationTime = fireEffect->targetPosition.x;
                ModelObjectWorldPositionGet(lbl1Bss1A0->mdlId[0], fireEffect->objectName, &fireEffect->position, NULL);
                fireEffect->modelId = FireParticleSystemCreate(NULL, &fireEffect->animationTime, 0x40);
            }
            else if (fireEffect->modelId >= 0) {
                activeEffectCount++;
                fireEffect->animationTime += fireEffect->targetPosition.y;
                if (0.0f > fireEffect->animationTime) {
                    fireEffect->animationTime += 360.0f;
                }
                if (360.0f <= fireEffect->animationTime) {
                    fireEffect->animationTime -= 360.0f;
                }
                if ((lbl1Bss308 & 1) == 0) {
                    FireParticleBurstCreate(fireEffect->modelId, 1, &fireEffect->position, 0.0f, fireEffect->targetPosition.z, fireEffect->position.y);
                    if ((soundAvailable != 0) && (fireEffect->state < 0) && (fireEffect->state > -2)) {
                        soundAvailable = 0;
                        fireEffect->state = HuAudFXEmiterPlay(fireEffect->targetPosition.z == 350.0f ? 0x7BF : 0x7AD, &fireEffect->position);
                    }
                }
                if (ParticleSystemDestroyIfFinished(fireEffect->modelId) != 0) {
                    fireEffect->modelId = -1;
                }
            }
        }
        if (activeEffectCount == 0) {
            lbl1Bss104 = NULL;
        }
    }
    ArenaModelMotionUpdate(object);
    ThunderAttackUpdate();
}

void ArenaObjectManagerInit(OMOBJ *object)
{
    s32 modelIndex;
    s32 **difficultyMotionTable;

    lbl1Bss104 = NULL;
    MTXIdentity(lbl1Bss88);
    omSetTra(object, 0.0f, 0.0f, 0.0f);
    omSetRot(object, 0.0f, 0.0f, 0.0f);
    for (modelIndex = 0; modelIndex < 0x21; modelIndex++) {
        object->mdlId[modelIndex] = Hu3DModelCreateFile(lbl1DataE7C[modelIndex]);
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF | HU3D_ATTR_NOCULL);
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_MOTATTR_LOOP);
        Hu3DModelAttrReset(object->mdlId[modelIndex], HU3D_MOTATTR_PAUSE);
        Hu3DModelLayerSet(object->mdlId[modelIndex], 0);
        Hu3DModelPosSet(object->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(object->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
        if (modelIndex != 1) {
            Hu3DModelShadowMapSet(object->mdlId[modelIndex]);
        }
    }
    Hu3DMotionSpeedSet(object->mdlId[1], 0.1f);
    switch (GWPlayerCfg[lbl1Bss120].diff) {
        case 0:
            difficultyMotionTable = lbl1Data1204;
            break;
        case 1:
            difficultyMotionTable = lbl1Data1210;
            break;
        case 2:
            difficultyMotionTable = lbl1Data121C;
            break;
        case 3:
        default:
            difficultyMotionTable = lbl1Data1228;
            break;
    }
    ArenaModelLayoutInit(&arenaLayout, lbl1Data11F4[frandmod(4)], 5);
    ArenaModelLayoutInit(&lbl1Data13B8, difficultyMotionTable[frandmod(3)], 9);
    ArenaHookPositionsUpdate();

    for (modelIndex = 0; modelIndex <= 1; modelIndex++) {
        Hu3DModelAttrReset(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
    }
    Hu3DModelAttrReset(object->mdlId[6], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[17], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[19], HU3D_ATTR_DISPOFF);
    for (modelIndex = 0; modelIndex < 5; modelIndex++) {
        Hu3DModelAttrSet(object->mdlId[modelIndex + 0x14], HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(object->mdlId[modelIndex + 0x14], HU3D_MOTATTR_LOOP);
        Hu3DModelAttrReset(object->mdlId[modelIndex + 0x14], HU3D_ATTR_DISPOFF);
    }
    object->objFunc = FireEffectsUpdate;
}

float ValueSpacingCalculate(s32 stepCount, float startValue, float endValue)
{
    return (endValue + (startValue * (stepCount - 1))) / stepCount;
}

void ArenaMovementArcCalculate(Vec *startPosition, Vec *targetPosition, float arcHeight, s32 segmentCount, float *xOffsets, float *zOffsets)
{
    Vec movementVector;

    VECSubtract(targetPosition, startPosition, &movementVector);
    if (360.0f <= arcHeight) {
        arcHeight -= 360.0f;
    }
    else if (0.0f > arcHeight) {
        arcHeight += 360.0f;
    }
    if (360.0f <= arcHeight) {
        arcHeight -= 360.0f;
    }
    else if (0.0f > arcHeight) {
        arcHeight += 360.0f;
    }
    if (360.0f <= arcHeight) {
        arcHeight -= 360.0f;
    }
    else if (0.0f > arcHeight) {
        arcHeight += 360.0f;
    }
    *xOffsets = (atan2d(movementVector.z, movementVector.x)) - 90.0;
    *zOffsets = atan2d(movementVector.y, sqrtf((movementVector.x * movementVector.x) + (movementVector.z * movementVector.z)));
    if (segmentCount == 0) {
        *xOffsets *= -1.0f;
    }
    if (segmentCount != 0) {
        *xOffsets += arcHeight;
    }
    else {
        *xOffsets -= arcHeight;
    }
    if (180.0f <= *xOffsets) {
        *xOffsets -= 360.0f;
    }
    else if (-180.0f > *xOffsets) {
        *xOffsets += 360.0f;
    }
    if (180.0f <= *xOffsets) {
        *xOffsets -= 360.0f;
    }
    else if (-180.0f > *xOffsets) {
        *xOffsets += 360.0f;
    }
    if (180.0f <= *xOffsets) {
        *xOffsets -= 360.0f;
    }
    else if (-180.0f > *xOffsets) {
        *xOffsets += 360.0f;
    }
    if (60.0f <= *xOffsets) {
        *xOffsets = 60.0f;
    }
    if (-60.0f >= *xOffsets) {
        *xOffsets = -60.0f;
    }
    if (45.0f <= *zOffsets) {
        *zOffsets = 45.0f;
    }
    if (-45.0f >= *zOffsets) {
        *zOffsets = -45.0f;
    }
}

void ArenaPlayerSpecialMotionSet(ArenaPlayerState *playerState, OMOBJ *playerObject, Vec *targetPosition)
{
    float arcHorizontalAngle;
    float arcVerticalAngle;
    MotionResourceInfo *motionInfo;

    playerState->stateFlags |= 0x8000;
    motionInfo = &lbl1Data5CC[playerState->characterId];
    if (NULL == targetPosition) {
        arcHorizontalAngle = 0.0f;
        arcVerticalAngle = 0.0f;
    }
    else {
        ArenaMovementArcCalculate(&playerState->playerWork.worldPosition, targetPosition, playerState->playerWork.rotationY, motionInfo->motionAttributes, &arcHorizontalAngle, &arcVerticalAngle);
    }
    playerState->collisionRadius = ValueSpacingCalculate(10, playerState->collisionRadius, arcHorizontalAngle);
    playerState->movementProgress = ValueSpacingCalculate(10, playerState->movementProgress, arcVerticalAngle);
    Hu3DMotionForceSet(playerObject->mdlId[0], motionInfo->motionName, motionInfo->motionId, playerState->collisionRadius);
    Hu3DMotionForceSet(playerObject->mdlId[0], motionInfo->motionName, 0x200, motionInfo->motionThreshold - playerState->movementProgress);
}

void ArenaPlayerSpecialMotionReset(ArenaPlayerState *playerState, OMOBJ *playerObject)
{
    MotionResourceInfo *motionInfo;

    if (playerState->stateFlags & 0x8000) {
        playerState->stateFlags &= ~0x8000;
        motionInfo = &lbl1Data5CC[playerState->characterId];
        Hu3DMotionNoMotReset(playerObject->mdlId[0], motionInfo->motionName, motionInfo->motionId);
        Hu3DMotionNoMotReset(playerObject->mdlId[0], motionInfo->motionName, 0x200);
    }
}

Vec *ArenaWaypointPositionGet(s32 waypointIndex)
{
    ArenaModelLayout *modelLayout;
    s32 modelIndex;

    modelLayout = &arenaLayout;
    for (modelIndex = 0; modelIndex < modelLayout->modelCount; modelIndex++) {
        if (waypointIndex == modelLayout->modelIds[modelIndex]) {
            return &modelLayout->modelPositions[modelIndex];
        }
    }
    return modelLayout->modelPositions;
}

void FallingObjectUpdate(OMOBJ *object)
{
    Vec targetPlayerPosition;
    float fadeAmount;
    float reboundSpeed;
    float impactSpeedMagnitude;

    MovingEffectState *fallingObject;
    s32 modelIndex;

    fallingObject = object->data;
    fallingObject->verticalVelocity += 0.98f;
    if (1555.0f < fallingObject->position.y) {
        fallingObject->position.y -= fallingObject->verticalVelocity;
    }
    if (1555.0f >= fallingObject->position.y) {
        if (10.0f < fallingObject->verticalVelocity) {
            fallingObject->verticalVelocity = 10.0f;
        }
        fallingObject->position.y = 1555.0f;
        if (0.0f > (fallingObject->verticalVelocity / 1.5f)) {
            reboundSpeed = -(fallingObject->verticalVelocity / 1.5f);
        }
        else {
            reboundSpeed = fallingObject->verticalVelocity / 1.5f;
        }
        fallingObject->verticalVelocity = -reboundSpeed;
        if (0.0f > fallingObject->verticalVelocity) {
            impactSpeedMagnitude = -fallingObject->verticalVelocity;
        }
        else {
            impactSpeedMagnitude = fallingObject->verticalVelocity;
        }
        if (1.0f < impactSpeedMagnitude) {
            fallingObject->position.y += 1.0f;
        }
        if (fallingObject->isFirstImpact != 0) {
            fallingObject->isFirstImpact = 0;
            omVibrate(lbl1Bss120, 0xC, 4, 2);
            HuAudFXPlay(0x7AE);
        }
    }
    if (Hu3DMotionEndCheck(object->mdlId[0]) != 0) {
        fallingObject->removeRequested = 1;
    }
    if ((fallingObject->isFirstImpact == 0) && (lbl1Bss1A4->work[0] > 0x3FB)) {
        fallingObject->removeRequested = 1;
    }
    if ((fallingObject->isFirstImpact == 0) && (lbl1Bss1A4->work[0] == 0x3E8)) {
        fallingObject->removeRequested = 1;
    }
    if (fallingObject->removeRequested != 0) {
        fallingObject->lifetime = fallingObject->lifetime - 1.0f;
        if ((lbl1Bss308 & 1) != 0) {
            for (modelIndex = 0; modelIndex < 2; modelIndex++) {
                Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
            }
        }
        else {
            for (modelIndex = 0; modelIndex < 2; modelIndex++) {
                Hu3DModelAttrReset(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
            }
        }
        if (0.0f > fallingObject->lifetime) {
            Hu3DModelShadowReset(object->mdlId[0]);
            for (modelIndex = 0; modelIndex < 2; modelIndex++) {
                Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
            }
            RegisteredModelAdd(0x1000, object->mdlId[0]);
            RegisteredModelAdd(0x2000, object->mdlId[1]);
            omDelObjEx(lbl1Bss33C, object);
            return;
        }
    }
    if (fallingObject->removeRequested == 0) {
        for (modelIndex = 0; modelIndex < 1; modelIndex++) {
            if (NULL == lbl1BssE8[modelIndex]) {
                lbl1BssE8[modelIndex] = fallingObject;
                break;
            }
        }
    }
    fadeAmount = (fallingObject->position.y - 1555.0f) / 300.0f;
    if (1.0f < fadeAmount) {
        fadeAmount = 1.0f;
    }
    fadeAmount = 1.0f - fadeAmount;
    targetPlayerPosition = *ArenaWaypointPositionGet(fallingObject->targetPlayerIndex);
    VECAdd(&fallingObject->interactionPosition, &targetPlayerPosition, &targetPlayerPosition);
    fallingObject->position.x += fadeAmount * (targetPlayerPosition.x - fallingObject->position.x);
    fallingObject->position.z += fadeAmount * (targetPlayerPosition.z - fallingObject->position.z);
    omSetTra(object, fallingObject->position.x, fallingObject->position.y, fallingObject->position.z);
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        Hu3DModelPosSet(object->mdlId[modelIndex], fallingObject->position.x, fallingObject->position.y, fallingObject->position.z);
    }
}

void FallingObjectInit(OMOBJ *object)
{
    MovingEffectState *fallingObject;
    s32 modelIndex;
    s32 modelId;

    fallingObject = object->data;
    fallingObject->verticalVelocity = 0.0f;
    fallingObject->lifetime = 30.0f;
    modelId = RegisteredModelRemoveByResourceKey(0x1000);
    if (modelId < 0) {
        object->mdlId[0] = Hu3DModelLink(lbl1Bss2C[0]);
    }
    else {
        object->mdlId[0] = modelId;
    }
    modelId = RegisteredModelRemoveByResourceKey(0x2000);
    if (modelId < 0) {
        object->mdlId[1] = Hu3DModelLink(lbl1Bss2C[1]);
    }
    else {
        object->mdlId[1] = modelId;
    }
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(object->mdlId[modelIndex], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
        Hu3DModelLayerSet(object->mdlId[modelIndex], 4);
        Hu3DMotionTimeSet(object->mdlId[modelIndex], 0.0f);
        Hu3DModelPosSet(object->mdlId[modelIndex], fallingObject->position.x, fallingObject->position.y, fallingObject->position.z);
        Hu3DModelRotSet(object->mdlId[modelIndex], 0.0f, 0.0f, 0.0f);
    }
    omSetTra(object, fallingObject->position.x, fallingObject->position.y, fallingObject->position.z);
    omSetRot(object, 0.0f, 0.0f, 0.0f);
    Hu3DModelShadowSet(object->mdlId[0]);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
    object->objFunc = FallingObjectUpdate;
}

void ArenaModelTargetMove(Vec *startPosition, Vec *targetPosition, s32 layoutIndex)
{
    MovingEffectState *fallingObject;
    s32 soundIndex;
    OMOBJ *fallingObjectObj;

    for (soundIndex = 0; soundIndex < 2; soundIndex++) {
        if (lbl1Data1CC[soundIndex * 2] == 0x3E) {
            HuAudFXPlay(lbl1Data1CC[(soundIndex * 2) + 1]);
            break;
        }
    }
    fallingObjectObj = omAddObjEx(lbl1Bss33C, 0x6A, 2, 0, -1, FallingObjectInit);
    fallingObjectObj->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(MovingEffectState), HU_MEMNUM_OVL);
    fallingObject = fallingObjectObj->data;
    fallingObject->isFirstImpact = 1;
    fallingObject->removeRequested = 0;
    fallingObject->chasingPlayer = 0;
    fallingObject->targetPlayerIndex = layoutIndex;
    fallingObject->position = *startPosition;
    fallingObject->interactionPosition = *targetPosition;
}

void ChasingEffectUpdate(OMOBJ *object)
{
    MovingEffectState *effectState = object->data;
    ArenaPlayerState *player = lbl1Bss138->data;
    effectState->position.y += (0.1f * (1505.0f - effectState->position.y));
    if ((lbl1Bss1A4->work[0] > 0x3FB) || (lbl1Bss1A4->work[0] == 0x3E8)) {
        effectState->removeRequested = 1;
    }
    if (effectState->removeRequested != 0) {
        effectState->bounceScale = effectState->bounceScale - 0.1f;
        if (0.0f > effectState->bounceScale) {
            Hu3DModelAttrSet(*object->mdlId, HU3D_ATTR_DISPOFF);
            RegisteredModelAdd(0x3000, object->mdlId[0]);
            omDelObjEx(lbl1Bss33C, object);
            return;
        }
    }
    else {
        effectState->lifetime += 1.0f;
        if (effectState->chasingPlayer != 0) {
            if (60.0f < effectState->lifetime) {
                effectState->removeRequested = 1;
            }
            effectState->position.x += (0.1f * (player->playerWork.worldPosition.x - effectState->position.x));
            effectState->position.z += (0.1f * (player->playerWork.worldPosition.z - effectState->position.z));
            effectState->bounceVelocity *= 1.01f;
        }
        else {
            if (240.0f < effectState->lifetime) {
                effectState->removeRequested = 1;
            }
            effectState->position.x += (11.0 * sind(effectState->rotation));
            effectState->position.z += (11.0 * cosd(effectState->rotation));
            effectState->rotation = AngleInterpolate(
                effectState->rotation, atan2d(player->playerWork.worldPosition.x - effectState->position.x, player->playerWork.worldPosition.z - effectState->position.z), 0.05f);
        }
    }
    if ((effectState->removeRequested == 0) && (1605.0f > effectState->position.y)) {
        s32 effectSlot;
        for (effectSlot = 0; effectSlot < 1; effectSlot++) {
            if (NULL == lbl1BssE4[effectSlot]) {
                lbl1BssE4[effectSlot] = effectState;
                break;
            }
        }
    }
    Hu3DModelTPLvlSet(object->mdlId[0], effectState->bounceScale);
    omSetTra(object, effectState->position.x, effectState->position.y, effectState->position.z);
    omSetSca(object, effectState->bounceVelocity, effectState->bounceVelocity, effectState->bounceVelocity);
}

void ChasingEffectInit(OMOBJ *object)
{
    MovingEffectState *effectState;
    s16 modelId;

    effectState = object->data;
    effectState->bounceScale = 1.0f;
    effectState->bounceVelocity = 1.0f;
    effectState->lifetime = 0.0f;
    modelId = RegisteredModelRemoveByResourceKey(0x3000);
    if (modelId < 0) {
        object->mdlId[0] = Hu3DModelLink(lbl1Bss2C[2]);
    }
    else {
        object->mdlId[0] = modelId;
    }
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(object->mdlId[0], 4);
    omSetTra(object, effectState->position.x, effectState->position.y, effectState->position.z);
    omSetRot(object, 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    object->objFunc = ChasingEffectUpdate;
}

void ArenaFireEffectCreate(Vec *position, float rotation)
{
    OMOBJ *effectObject;
    s32 soundIndex;
    MovingEffectState *effectState;

    for (soundIndex = 0; soundIndex < 2; soundIndex++) {
        if (lbl1Data1CC[soundIndex * 2] == 0x3E) {
            HuAudFXPlay(lbl1Data1CC[soundIndex * 2]);
            break;
        }
    }
    effectObject = omAddObjEx(lbl1Bss33C, 0x6B, 1, 0, -1, ChasingEffectInit);
    effectObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(MovingEffectState), HU_MEMNUM_OVL);
    effectState = effectObject->data;
    effectState->removeRequested = 0;
    effectState->chasingPlayer = 0;
    effectState->position = *position;
    effectState->rotation = rotation;
    HuAudFXPlay(0x7AF);
}

void HookEffectUpdate(OMOBJ *object)
{
    Mtx rotationMatrix;
    Mtx effectModelMatrix;
    Mtx hookModelMatrix;
    Vec motionOffset;
    Vec hookPosition;
    float horizontalVelocity;

    MovingEffectState *effectState = object->data;
    if (effectState->hookModelId != 0) {
        ModelObjectPositionGet(effectState->hookModelId, "g000m0-itemhook_R", &hookPosition);
        if (Hu3DMotionShiftIDGet(effectState->hookModelId) < 0) {
            Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
            if (165.0f <= Hu3DMotionTimeGet(effectState->hookModelId)) {
                HuAudFXPlay(0x7B0);
                effectState->isFirstImpact = 0;
                effectState->rotation = 0.0f;
                effectState->horizontalVelocity = VECDistance(&hookPosition, &effectState->startingPosition);
                Hu3DModelObjMtxGet(effectState->effectModelId, "g000i00-itemhook_A", effectModelMatrix);
                Hu3DModelObjMtxGet(effectState->hookModelId, "g000m0-itemhook_R", hookModelMatrix);
                MTXConcat(effectModelMatrix, hookModelMatrix, effectModelMatrix);
                effectState->position.x = effectModelMatrix[0][3];
                effectState->position.y = effectModelMatrix[1][3];
                effectState->position.z = effectModelMatrix[2][3];
                Hu3DModelHookObjReset(effectState->hookModelId, "g000m0-itemhook_R");
                MTXIdentity(Hu3DData[object->mdlId[0]].mtx);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_HOOK);
            }
        }
        effectState->startingPosition = hookPosition;
    }
    if (effectState->isFirstImpact == 0) {
        if (lbl1Bss1A4->work[0] > 0x40D) {
            effectState->bounceScale = effectState->bounceScale - 0.05f;
        }
        horizontalVelocity = effectState->horizontalVelocity;
        motionOffset.x = 0.0f;
        motionOffset.y = 0.0f;
        motionOffset.z = horizontalVelocity;
        MTXRotRad(rotationMatrix, 0x59, 0.017453292f * (effectState->targetHeight + effectState->targetRadius));
        MTXMultVec(rotationMatrix, &motionOffset, &motionOffset);
        VECAdd(&effectState->position, &motionOffset, &effectState->position);
        effectState->position.y += 0.1f * (1575.0f - effectState->position.y);
        effectState->horizontalVelocity += 0.05f * (30.0f - effectState->horizontalVelocity);
        effectState->rotation += effectState->horizontalVelocity;
        effectState->targetRadius += 0.08f * effectState->horizontalVelocity;
        if (360.0f <= effectState->targetRadius) {
            effectState->targetRadius = 360.0f;
        }
        motionOffset.x = 0.0f;
        motionOffset.y = 1575.0f;
        motionOffset.z = 0.0f;
        if ((360.0f <= effectState->targetRadius) && (1500.0f < VECDistance(&motionOffset, &effectState->position))) {
            effectState->bounceScale = effectState->bounceScale - 0.05f;
            if (0.0f > effectState->bounceScale) {
                Hu3DModelShadowReset(object->mdlId[0]);
                Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
                RegisteredModelAdd(0x4000, object->mdlId[0]);
                omDelObjEx(lbl1Bss33C, object);
                return;
            }
        }
        if (1.0f <= effectState->bounceScale) {
            s32 effectSlot;
            for (effectSlot = 0; effectSlot < 2; effectSlot++) {
                if (NULL == lbl1BssDC[effectSlot]) {
                    lbl1BssDC[effectSlot] = effectState;
                    break;
                }
            }
        }
        omSetTra(object, effectState->position.x, effectState->position.y, effectState->position.z);
        omSetRot(object, 0.0f, effectState->rotation, 0.0f);
    }
    Hu3DModelShadowDispOn(object->mdlId[0]);
    Hu3DModelTPLvlSet(object->mdlId[0], effectState->bounceScale);
}

void HookEffectCreate(s32 hookModelId, s32 effectModelId, float targetHeight)
{
    OMOBJ *effectObject;
    MovingEffectState *effectState;
    s16 modelId;

    HuAudFXPlay(0x3A);
    effectObject = omAddObjEx(lbl1Bss33C, 0x6C, 1, 0, -1, HookEffectUpdate);
    effectObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(MovingEffectState), HU_MEMNUM_OVL);
    effectState = effectObject->data;
    effectState->bounceScale = 1.0f;
    effectState->lifetime = 0.0f;
    effectState->isFirstImpact = 1;
    effectState->removeRequested = 0;
    effectState->chasingPlayer = 0;
    effectState->hookModelId = hookModelId;
    effectState->effectModelId = effectModelId;
    effectState->targetHeight = targetHeight;
    effectState->targetRadius = 0.0f;

    modelId = RegisteredModelRemoveByResourceKey(0x4000);
    if (modelId < 0) {
        effectObject->mdlId[0] = Hu3DModelLink(lbl1Bss2C[3]);
    }
    else {
        effectObject->mdlId[0] = modelId;
    }
    Hu3DModelAttrSet(effectObject->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(effectObject->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(effectObject->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(effectObject->mdlId[0], 4);
    omSetTra(effectObject, 0.0f, 0.0f, 0.0f);
    omSetRot(effectObject, 0.0f, 0.0f, 0.0f);
    Hu3DModelHookSet(effectState->hookModelId, "g000m0-itemhook_R", effectObject->mdlId[0]);
    Hu3DModelShadowSet(effectObject->mdlId[0]);
    Hu3DModelShadowDispOff(effectObject->mdlId[0]);
}

void HookEffectArcUpdate(OMOBJ *object)
{
    Mtx arcRotationMatrix;
    Vec arcOffset;
    Vec hookPosition;
    float horizontalDelta;
    float depthDelta;
    float targetHeight;

    MovingEffectState *effectState;
    s32 effectSlot;
    ArenaOpponentState *opponent;

    effectState = object->data;
    opponent = lbl1Bss134->data;
    if (effectState->hookModelId != 0) {
        ModelObjectPositionGet(effectState->hookModelId, "g000m0-itemhook_R", &hookPosition);
        if ((Hu3DMotionShiftIDGet(effectState->hookModelId) < 0) && (165.0f <= Hu3DMotionTimeGet(effectState->hookModelId))) {
            HuAudFXPlay(0x7B0);
            effectState->isFirstImpact = 0;
            effectState->rotation = 0.0f;
            effectState->horizontalVelocity = VECDistance(&hookPosition, &effectState->startingPosition);
            ModelObjectPositionGet(effectState->hookModelId, "g000m0-itemhook_R", &effectState->position);
            Hu3DModelHookObjReset(effectState->hookModelId, "g000m0-itemhook_R");
            MTXIdentity(Hu3DData[*object->mdlId].mtx);
            Hu3DModelAttrReset(*object->mdlId, HU3D_ATTR_HOOK);
            horizontalDelta = effectState->position.x - effectState->movementVelocity.x;
            depthDelta = effectState->position.z - effectState->movementVelocity.z;
            effectState->movementOffset.x = sqrtf((horizontalDelta * horizontalDelta) + (depthDelta * depthDelta)) / 2;
            effectState->movementOffset.z = 150.0f + effectState->position.y;
            VECAdd(&effectState->position, &effectState->movementVelocity, &effectState->movementVelocity);
            VECScale(&effectState->movementVelocity, &effectState->movementVelocity, 0.5f);
            effectState->movementVelocity.y = effectState->position.y;
            effectState->movementOffset.y = atan2d(effectState->position.x - effectState->movementVelocity.x, effectState->position.z - effectState->movementVelocity.z);
        }
        effectState->startingPosition = hookPosition;
        if ((effectState->isFirstImpact != 0) && (opponent->actionState >= 0xBCE)) {
            Hu3DModelShadowReset(object->mdlId[0]);
            Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
            Hu3DModelHookObjReset(effectState->hookModelId, "g000m0-itemhook_R");
            RegisteredModelAdd(0x4000, object->mdlId[0]);
            omDelObjEx(lbl1Bss33C, object);
            return;
        }
    }
    if (effectState->isFirstImpact == 0) {
        if (lbl1Bss1A4->work[0] > 0x415) {
            effectState->bounceScale = effectState->bounceScale - 0.05f;
        }
        targetHeight = 1575.0f;
        if (180.0f < effectState->targetRadius) {
            targetHeight = effectState->movementOffset.z;
        }
        effectState->position.y += (0.05f * (targetHeight - effectState->position.y));
        effectState->horizontalVelocity += 0.05f * (30.0f - effectState->horizontalVelocity);
        effectState->rotation += effectState->horizontalVelocity;
        effectState->targetRadius += 0.08f * effectState->horizontalVelocity;
        if (360.0f <= effectState->targetRadius) {
            effectState->targetRadius = 360.0f;
            VECAdd(&effectState->targetPosition, &effectState->position, &effectState->position);
        }
        else {
            hookPosition = effectState->position;
            arcOffset.x = (0.6666667f * effectState->movementOffset.x) * sind(effectState->targetRadius);
            arcOffset.z = effectState->movementOffset.x * cosd(effectState->targetRadius);
            MTXRotRad(arcRotationMatrix, 0x59, 0.017453292f * effectState->movementOffset.y);
            MTXMultVec(arcRotationMatrix, &arcOffset, &arcOffset);
            effectState->position.x = effectState->movementVelocity.x + arcOffset.x;
            effectState->position.z = effectState->movementVelocity.z + arcOffset.z;
            VECSubtract(&effectState->position, &hookPosition, &effectState->targetPosition);
        }
        arcOffset.x = 0.0f;
        arcOffset.y = 1575.0f;
        arcOffset.z = 0.0f;
        if ((360.0f <= effectState->targetRadius) && (1500.0f < VECDistance(&arcOffset, &effectState->position))) {
            effectState->bounceScale = effectState->bounceScale - 0.05f;
            if (0.0f > effectState->bounceScale) {
                Hu3DModelShadowReset(*object->mdlId);
                Hu3DModelAttrSet(*object->mdlId, HU3D_ATTR_DISPOFF);
                RegisteredModelAdd(0x4000, object->mdlId[0]);
                omDelObjEx(lbl1Bss33C, object);
                return;
            }
        }
        if (1.0f <= effectState->bounceScale) {
            for (effectSlot = 0; effectSlot < 2; effectSlot++) {
                if (NULL == lbl1BssDC[effectSlot]) {
                    lbl1BssDC[effectSlot] = effectState;
                    break;
                }
            }
        }
        omSetTra(object, effectState->position.x, effectState->position.y, effectState->position.z);
        omSetRot(object, 0.0f, effectState->rotation, 0.0f);
        omSetSca(object, 1.0f, 1.0f, 1.0f);
    }
    Hu3DModelShadowDispOn(object->mdlId[0]);
    Hu3DModelTPLvlSet(object->mdlId[0], effectState->bounceScale);
}

void HookEffectArcCreate(s32 hookModelId, Vec *movementVelocity)
{
    OMOBJ *effectObject;
    MovingEffectState *effectState;
    s16 modelId;

    effectObject = omAddObjEx(lbl1Bss33C, 0x6C, 1, 0, -1, HookEffectArcUpdate);
    effectObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(MovingEffectState), HU_MEMNUM_OVL);
    effectState = effectObject->data;
    effectState->hookModelId = hookModelId;
    effectState->bounceScale = 1.0f;
    effectState->lifetime = 0.0f;
    effectState->isFirstImpact = 1;
    effectState->removeRequested = 0;
    effectState->chasingPlayer = 0;
    effectState->targetRadius = 0.0f;
    effectState->movementVelocity = *movementVelocity;

    modelId = RegisteredModelRemoveByResourceKey(0x4000);
    if (modelId < 0) {
        effectObject->mdlId[0] = Hu3DModelLink(lbl1Bss2C[3]);
    }
    else {
        effectObject->mdlId[0] = modelId;
    }
    Hu3DModelAttrSet(effectObject->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(effectObject->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(effectObject->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelLayerSet(effectObject->mdlId[0], 4);
    omSetTra(effectObject, 0.0f, 0.0f, 0.0f);
    omSetRot(effectObject, 0.0f, 0.0f, 0.0f);
    omSetSca(effectObject, 1.0f, 1.0f, 1.0f);
    Hu3DModelHookSet(effectState->hookModelId, "g000m0-itemhook_R", effectObject->mdlId[0]);
    Hu3DModelAttrReset(effectObject->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowSet(effectObject->mdlId[0]);
    Hu3DModelShadowDispOff(effectObject->mdlId[0]);
}

void ThunderAttackUpdate(void)
{
    Vec triangleVertices[3];
    Vec triangleCenter;
    float fadeAmount;

    s32 modelIndex;
    ArenaPlayerState *player;
    ArenaOpponentState *opponent;
    HSFBUFFER *vertexBuffer;
    s32 hitCount;
    HU3DMODEL *modelData;
    HSFOBJECT *modelObject;

    if (lbl1BssC4 >= 0) {
        player = lbl1Bss138->data;
        opponent = lbl1Bss134->data;
        switch (lbl1BssC4) {
            case 0x1388:
                for (modelIndex = 0x19; modelIndex <= 0x1B; modelIndex++) {
                    if (Hu3DMotionEndCheck(lbl1Bss1A0->mdlId[modelIndex]) == 0) {
                        return;
                    }
                }
                lbl1BssC4++;
                lbl1BssC8 = 0.0f;
                Hu3DModelPosSet(lbl1Bss1A0->mdlId[28], 0.0f, 1501.0f, 0.0f);
                Hu3DModelAttrReset(lbl1Bss1A0->mdlId[28], HU3D_ATTR_DISPOFF);
                Hu3DModelTPLvlSet(lbl1Bss1A0->mdlId[28], 0.0f);
                modelData = &Hu3DData[lbl1Bss1A0->mdlId[28]];
                modelObject = modelData->hsf->object;
                vertexBuffer = modelObject->mesh.vertex;
                for (modelIndex = 0; modelIndex < 3; modelIndex++) {
                    ((Vec *)vertexBuffer->data)[modelIndex].x = lbl1Data1450[lbl1BssB8[modelIndex]].position.x;
                    ((Vec *)vertexBuffer->data)[modelIndex].z = lbl1Data1450[lbl1BssB8[modelIndex]].position.z;
                    DCFlushRange(&((Vec *)vertexBuffer->data)[modelIndex], sizeof(Vec));
                }

                break;
            case 0x1389:
                fadeAmount = lbl1BssC8 / 30.0f;
                if (1.0f < fadeAmount) {
                    fadeAmount = 1.0f;
                }
                Hu3DModelTPLvlSet(lbl1Bss1A0->mdlId[28], fadeAmount);
                lbl1BssC8 += 1.0f;
                if ((120.0f < lbl1BssC8) && (player->playerWork.actionState == 0)) {
                    lbl1BssC4++;
                    lbl1BssC8 = 0.0f;
                    hitCount = 0;
                    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
                        triangleVertices[modelIndex] = lbl1Data1450[lbl1BssB8[modelIndex]].position;
                    }
                    if ((ArenaPlayerCanMoveCheck(player) != 0) && (PointInsideTriangleCheck(&player->playerWork.worldPosition, triangleVertices) != 0)) {
                        hitCount++;
                        player->currentTarget = 0x1D;
                        player->stateFlags |= 0x20000;
                        ArenaModelAnimationTrigger(&player->playerWork.worldPosition, 29);
                    }
                    if (PointInsideTriangleCheck(&opponent->playerWork.worldPosition, triangleVertices) != 0) {
                        hitCount++;
                        opponent->actionIndex = 0x1E;
                        opponent->stateFlags2 |= 0x8000;
                        ArenaModelAnimationTrigger(&opponent->playerWork.worldPosition, 30);
                    }
                    if (hitCount == 0) {
                        triangleCenter.x = 0.0f;
                        triangleCenter.y = 0.0f;
                        triangleCenter.z = 0.0f;
                        for (modelIndex = 0; modelIndex < 3; modelIndex++) {
                            VECAdd(&triangleVertices[modelIndex], &triangleCenter, &triangleCenter);
                        }
                        VECScale(&triangleCenter, &triangleCenter, 0.33333334f);
                        ArenaModelAnimationTrigger(&triangleCenter, 29);
                        return;
                    }
                }
                break;
            case 0x138A:
                if (45.0f < lbl1BssC8) {
                    fadeAmount = (lbl1BssC8 - 45.0f) / 15.0f;
                    if (1.0f < fadeAmount) {
                        fadeAmount = 1.0f;
                    }
                    Hu3DModelTPLvlSet(lbl1Bss1A0->mdlId[28], 1.0f - fadeAmount);
                    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
                        Hu3DModelTPLvlSet(lbl1Bss1A0->mdlId[modelIndex + 0x19], 1.0f - fadeAmount);
                    }
                }
                lbl1BssC8 += 1.0f;
                if (60.0f <= lbl1BssC8) {
                    lbl1BssC4 = -1;
                    Hu3DModelAttrSet(lbl1Bss1A0->mdlId[28], HU3D_ATTR_DISPOFF);
                    Hu3DModelTPLvlSet(lbl1Bss1A0->mdlId[28], 1.0f);
                    Hu3DModelAttrSet(lbl1Bss1A0->mdlId[29], HU3D_ATTR_DISPOFF);
                    Hu3DModelAttrSet(lbl1Bss1A0->mdlId[30], HU3D_ATTR_DISPOFF);
                    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
                        lbl1Data1450[lbl1BssB8[0]].animationActive = 0;
                        ArenaPlayerTargetRemove(lbl1BssB8[0]);
                        Hu3DModelAttrSet(lbl1Bss1A0->mdlId[modelIndex + 0x19], HU3D_ATTR_DISPOFF);
                        Hu3DModelTPLvlSet(lbl1Bss1A0->mdlId[modelIndex + 0x19], 1.0f);
                ArenaPlayerTargetListReset(player);
                    return;
                }
                break;
            default:
                OSReport("*** thunder attack mode error(%d)!!\n", lbl1BssC4);
                break;
        }
    }
}

void ArenaPlayerObstacleCollisionCheck(ArenaPlayerState *playerState, Vec *obstaclePosition)
{
    Vec obstacleCenter;
    Vec playerCollisionPoint;
    float horizontalDelta;
    float depthDelta;
    float distance;
    float collisionHeight;

    if (NULL != obstaclePosition) {
        obstacleCenter = *obstaclePosition;
        collisionHeight = playerState->playerWork.collisionHeight;
        if ((playerState->playerWork.actionState == 3) || (playerState->playerWork.actionState == 4)) {
            collisionHeight *= 0.5f;
        }
        horizontalDelta = playerState->playerWork.worldPosition.x - obstacleCenter.x;
        depthDelta = playerState->playerWork.worldPosition.z - obstacleCenter.z;
        distance = sqrtf((horizontalDelta * horizontalDelta) + (depthDelta * depthDelta));
        if (((obstacleCenter.y - 50.0f) < (playerState->playerWork.worldPosition.y + collisionHeight)) && (playerState->playerWork.collisionRadius > distance)) {
            if (distance == 0.0f) {
                horizontalDelta = 0.0f;
                depthDelta = -1.0f;
            }
            else {
                horizontalDelta /= distance;
                depthDelta /= distance;
            }
            playerState->playerWork.worldPosition.x = obstacleCenter.x + (horizontalDelta * playerState->playerWork.collisionRadius);
            playerState->playerWork.worldPosition.z = obstacleCenter.z + (depthDelta * playerState->playerWork.collisionRadius);
        }
        if ((playerState->stateFlags & 0x220) == 0) {
            playerCollisionPoint = playerState->playerWork.worldPosition;
            playerCollisionPoint.y += collisionHeight - playerState->playerWork.collisionRadius;
            distance = VECDistance(&playerCollisionPoint, &obstacleCenter);
            if ((playerCollisionPoint.y < obstacleCenter.y) && ((50.0f + playerState->playerWork.collisionRadius) > distance)) {
                playerState->stateFlags |= 0x200;
                return;
            }
            playerCollisionPoint = playerState->playerWork.worldPosition;
            playerCollisionPoint.y += playerState->playerWork.collisionRadius;
            distance = VECDistance(&playerCollisionPoint, &obstacleCenter);
            if ((50.0f + playerState->playerWork.collisionRadius) > distance) {
                playerState->stateFlags |= 0x20;
            }
        }
    }
}

void ArenaPlayerChasingEffectCollisionCheck(ArenaPlayerState *playerState, MovingEffectState *chasingEffect)
{
    Vec playerCollisionPoint;
    float distance;

    if ((NULL != chasingEffect) && (playerState->playerWork.actionState == 0) && ((playerState->stateFlags & 0x2000) == 0)) {
        playerCollisionPoint = playerState->playerWork.worldPosition;
        playerCollisionPoint.y += playerState->playerWork.collisionRadius;
        distance = VECDistance(&playerCollisionPoint, &chasingEffect->position);
        if ((50.0f + playerState->playerWork.collisionRadius) > distance) {
            playerState->stateFlags |= 0x2000;
            chasingEffect->chasingPlayer = 1;
            chasingEffect->lifetime = 0.0f;
        }
    }
}

void ArenaPlayerHookEffectCollisionCheck(ArenaPlayerState *playerState, MovingEffectState *hookEffect)
{
    Vec playerCollisionPoint;
    float distance;

    if ((NULL != hookEffect) && ((playerState->stateFlags & 0x20) == 0)) {
        playerCollisionPoint = playerState->playerWork.worldPosition;
        playerCollisionPoint.y += playerState->playerWork.collisionRadius;
        distance = VECDistance(&playerCollisionPoint, &hookEffect->position);
        if ((70.0f + playerState->playerWork.collisionRadius) > distance) {
            playerState->stateFlags |= 0x20;
        }
    }
}

void ArenaPlayerEffectCollisionsUpdate(ArenaPlayerState *playerState)
{
    s32 effectIndex;

    for (effectIndex = 0; effectIndex < 1; effectIndex++) {
        ArenaPlayerObstacleCollisionCheck(playerState, lbl1BssE8[effectIndex]);
    }
    for (effectIndex = 0; effectIndex < 1; effectIndex++) {
        ArenaPlayerChasingEffectCollisionCheck(playerState, lbl1BssE4[effectIndex]);
    }
    for (effectIndex = 0; effectIndex < 2; effectIndex++) {
        ArenaPlayerHookEffectCollisionCheck(playerState, lbl1BssDC[effectIndex]);
    }
}

s32 PointInsideTriangleCheck(Vec *point, Vec *triangleVertices)
{
    Vec firstEdge;
    Vec secondEdge;
    Vec thirdEdge;
    Vec triangleNormal;
    Vec pointFromFirstVertex;
    Vec pointFromSecondVertex;
    Vec pointFromThirdVertex;
    Vec firstEdgeCrossNormal;
    Vec secondEdgeCrossNormal;
    Vec thirdEdgeCrossNormal;

    VECSubtract(&triangleVertices[1], &triangleVertices[0], &firstEdge);
    VECSubtract(&triangleVertices[2], &triangleVertices[1], &secondEdge);
    VECSubtract(&triangleVertices[0], &triangleVertices[2], &thirdEdge);
    VECCrossProduct(&firstEdge, &secondEdge, &triangleNormal);
    VECSubtract(point, &triangleVertices[0], &pointFromFirstVertex);
    VECSubtract(point, &triangleVertices[1], &pointFromSecondVertex);
    VECSubtract(point, &triangleVertices[2], &pointFromThirdVertex);
    VECCrossProduct(&firstEdge, &triangleNormal, &firstEdgeCrossNormal);
    VECCrossProduct(&secondEdge, &triangleNormal, &secondEdgeCrossNormal);
    VECCrossProduct(&thirdEdge, &triangleNormal, &thirdEdgeCrossNormal);
    if ((0.0f >= VECDotProduct(&firstEdgeCrossNormal, &pointFromFirstVertex)) && (0.0f >= VECDotProduct(&secondEdgeCrossNormal, &pointFromSecondVertex)) && (0.0f >= VECDotProduct(&thirdEdgeCrossNormal, &pointFromThirdVertex))) {
        return 1;
    }
    return 0;
}

s32 ArenaPlayerOpponentCollisionResolve(void)
{
    float horizontalDelta;
    float depthDelta;
    float distance;

    ArenaPlayerState *player = lbl1Bss138->data;
    ArenaOpponentState *opponent = lbl1Bss134->data;

    horizontalDelta = player->playerWork.worldPosition.x - opponent->playerWork.worldPosition.x;
    depthDelta = player->playerWork.worldPosition.z - opponent->playerWork.worldPosition.z;
    distance = sqrtf((horizontalDelta * horizontalDelta) + (depthDelta * depthDelta));
    if ((150.0f + player->playerWork.collisionRadius) > distance) {
        if (distance == 0.0f) {
            horizontalDelta = 0.0f;
            depthDelta = -1.0f;
        }
        else {
            horizontalDelta /= distance;
            depthDelta /= distance;
        }
        player->stateFlags |= 0x200000;
        if (opponent->actionState != 0xBC9) {
            player->stateFlags |= 4;
        }
        player->playerWork.worldPosition.x = opponent->playerWork.worldPosition.x + (horizontalDelta * (150.0f + player->playerWork.collisionRadius));
        player->playerWork.worldPosition.z = opponent->playerWork.worldPosition.z + (depthDelta * (150.0f + player->playerWork.collisionRadius));
        return 1;
    }
    return 0;
}

s32 PlayerMoveTowardPosition(PlayerMovementWork *player, Vec *targetPosition)
{
    float horizontalDelta;
    float depthDelta;
    float distance;

    horizontalDelta = targetPosition->x - player->worldPosition.x;
    depthDelta = targetPosition->z - player->worldPosition.z;
    distance = sqrtf((horizontalDelta * horizontalDelta) + (depthDelta * depthDelta));
    if (10.0f <= distance) {
        horizontalDelta /= distance;
        depthDelta /= distance;
    }
    else {
        return 1;
    }
    player->inputX = 72.0f * horizontalDelta;
    player->inputZ = 72.0f * -depthDelta;
    return 0;
}

void ArenaPlayerBoundsClamp(OMOBJ *object)
{
    ArenaPlayerState *player = object->data;

    if ((player->stateFlags & 1) != 0) {
        if (1250.0f < player->playerWork.worldPosition.x) {
            player->playerWork.worldPosition.x = 1250.0f;
        }
        if (-1250.0f > player->playerWork.worldPosition.x) {
            player->playerWork.worldPosition.x = -1250.0f;
        }
        if (1250.0f < player->playerWork.worldPosition.z) {
            player->playerWork.worldPosition.z = 1250.0f;
        }
        if (-1250.0f > player->playerWork.worldPosition.z) {
            player->playerWork.worldPosition.z = -1250.0f;
        }
    }
}

void ArenaPlayerStateTransitionApply(OMOBJ *object)
{
    ArenaPlayerState *player = object->data;

    if (player->stateFlags & 0x10) {
        player->stateFlags |= 0x42;
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[10], 0.0f, 8.0f, 0x40000002);
    }
    if (player->stateFlags & 0x100) {
        player->stateFlags |= 0x402;
        Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
        Hu3DMotionSet(object->mdlId[0], object->mtnId[14]);
        Hu3DMotionTimeSet(object->mdlId[0], 48.0f);
    }
    if (player->stateFlags & 0x1000) {
        player->stateFlags |= 0x4002;
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[9], 0.0f, 8.0f, 0);
    }
    if (player->stateFlags & 0x10000) {
        player->stateFlags |= 0x40002;
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[16], 0.0f, 8.0f, 0);
        Hu3DModelPosSet(
            lbl1Bss1A0->mdlId[player->currentTarget + 2], player->playerWork.worldPosition.x, player->playerWork.worldPosition.y, 50.0f + player->playerWork.worldPosition.z);
        Hu3DMotionTimeSet(lbl1Bss1A0->mdlId[player->currentTarget + 2], 0.0f);
        Hu3DModelAttrSet(lbl1Bss1A0->mdlId[player->currentTarget + 2], HU3D_MOTATTR_LOOP);
        Hu3DModelAttrReset(lbl1Bss1A0->mdlId[player->currentTarget + 2], HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(lbl1Bss1A0->mdlId[player->currentTarget + 2], HU3D_ATTR_DISPOFF);
    }
    if (player->stateFlags & 0x100000) {
        player->stateFlags |= 0x400002;
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[9], 0.0f, 8.0f, 0);
    }
    player->stateFlags &= ~0x30;
    player->stateFlags &= ~0x300;
    player->stateFlags &= ~0x3000;
    player->stateFlags &= ~0x30000;
    player->stateFlags &= ~0x300000;
    player->actionIndex = 2 * REFRESH_RATE;
    player->transitionProgress = 0.0f;
    player->playerWork.stateFlags &= ~0x210;
}

void ArenaPlayerHitStateSet(OMOBJ *playerObject)
{
    ArenaPlayerState *player = playerObject->data;

    if (player->stateFlags & 0x555550) {
        player->stateFlags &= ~0x800;
        return;
    }
    player->stateFlags |= 0x800;
}

void ArenaPlayerCameraTargetSet(void)
{
    Vec targetPosition;

    Vec *cameraOffset;
    ArenaPlayerState *player = lbl1Bss138->data;

    targetPosition = player->playerWork.worldPosition;
    if (lbl1Bss10C == 0) {
        targetPosition.y = 1500.0f;
    }
    lbl1Bss324 = targetPosition;
    lbl1Bss30C = targetPosition;
    cameraOffset = &lbl1Data51C[lbl1Bss110];
    lbl1Bss324.x += (cameraOffset->x * sind((180.0f + player->playerWork.rotationY)));
    lbl1Bss324.z += (cameraOffset->x * cosd((180.0f + player->playerWork.rotationY)));
    lbl1Bss324.y += cameraOffset->y;
    lbl1Bss30C.y += cameraOffset->z;
}

void ArenaCameraTargetSet(float cameraYaw, float cameraHeight)
{
    Vec targetPosition;
    Vec *cameraOffset;
    ArenaPlayerState *player = lbl1Bss138->data;

    targetPosition = player->playerWork.worldPosition;
    if (lbl1Bss10C == 0) {
        targetPosition.y = 1500.0f;
    }
    if (cameraHeight != 0.0f) {
        targetPosition.y = cameraHeight;
    }
    lbl1Bss324 = targetPosition;
    lbl1Bss30C = targetPosition;
    cameraOffset = &lbl1Data51C[lbl1Bss110];
    lbl1Bss324.x += cameraOffset->x * sind(cameraYaw);
    lbl1Bss324.z += cameraOffset->x * cosd(cameraYaw);
    lbl1Bss324.y += cameraOffset->y;
    lbl1Bss30C.y += cameraOffset->z;
}

void ArenaPlayerOpponentCameraSet(void)
{
    float horizontalDelta;
    float depthDelta;
    float distance;
    ArenaPlayerState *player = lbl1Bss138->data;
    ArenaOpponentState *opponent = lbl1Bss134->data;

    horizontalDelta = player->playerWork.worldPosition.x - opponent->playerWork.worldPosition.x;
    depthDelta = player->playerWork.worldPosition.z - opponent->playerWork.worldPosition.z;
    distance = sqrtf((horizontalDelta * horizontalDelta) + (depthDelta * depthDelta));
    horizontalDelta = 800.0f * (horizontalDelta / distance);
    depthDelta = 800.0f * (depthDelta / distance);
    lbl1Bss324 = player->playerWork.worldPosition;
    lbl1Bss30C = player->playerWork.worldPosition;
    lbl1Bss324.x += horizontalDelta;
    lbl1Bss324.y += 350.0f;
    lbl1Bss324.z += depthDelta;
    lbl1Bss30C.y += 150.0f;
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

void DebugOverlayUpdate(void)
{
    ArenaPlayerState *player;

    HuSysVWaitSet(1);
    if (HuPadBtn[0] & PAD_BUTTON_X) {
        lbl1Bss304 = -1;
    }
    print8(0x18, 0x20, 1.5f, "%d COUNT:%d MODE:%d MES:%d CAM:%d", GWMGTypeGet(), lbl1Bss308, lbl1Bss1A4->work[0], lbl1Bss2FE, lbl1Bss110);
    print8(0x18, 0x30, 1.5f, "%f %f %f", lbl1Bss330.x, lbl1Bss330.y, lbl1Bss330.z);
    print8(0x18, 0x40, 1.5f, "%f %f %f", lbl1Bss318.x, lbl1Bss318.y, lbl1Bss318.z);
    print8(0x18, 0x50, 1.5f, "H:%x M:%x", HuMemHeapSizeGet(HEAP_HEAP) - HuMemUsedMallocSizeGet(HEAP_HEAP),
        HuMemHeapSizeGet(HEAP_MODEL) - HuMemUsedMallocSizeGet(HEAP_MODEL));
    player = lbl1Bss138->data;
    print8(0x18, 0x60, 1.5f, "%f (%f %f %f)", player->playerWork.jumpHeight, player->playerWork.worldPosition.x, player->playerWork.worldPosition.y, player->playerWork.worldPosition.z);
    print8(0x18, 0x70, 1.5f, "%d %d", player->selectedArenaIndex, player->attackTimer);
}

void ArenaPlayerArcMove(ArenaPlayerState *playerState, Vec *targetPosition, s8 arcAxis)
{
    Mtx rotationMatrix;
    float arcRadius;

    s32 shouldStartArc;

    nMap = 0;
    shouldStartArc = 1;
    lbl1Bss10C = 1;
    if (playerState->animationTimer == 0.0f) {
        lbl1Bss114 = 0;
        if (arcAxis == 0x78) {
            if (targetPosition->x > playerState->playerWork.worldPosition.x) {
                shouldStartArc = 0;
            }
        }
        else if (targetPosition->z < playerState->playerWork.worldPosition.z) {
            shouldStartArc = 0;
        }
    }
    if (shouldStartArc != 0) {
        CharModelFxFlagSet(playerState->characterId, 0);
        playerState->animationTimer += 1.0f;
        arcRadius = 0.0f > targetPosition->x ? -targetPosition->x : targetPosition->x;
        if (arcRadius < (0.0f > targetPosition->y ? -targetPosition->y : targetPosition->y)) {
            arcRadius = 0.0f > targetPosition->y ? -targetPosition->y : targetPosition->y;
        }
        if (arcRadius < (0.0f > targetPosition->z ? -targetPosition->z : targetPosition->z)) {
            arcRadius = 0.0f > targetPosition->z ? -targetPosition->z : targetPosition->z;
        }
        arcRadius = 1505.0f - arcRadius;
        if (arcAxis == 0x78) {
            ArenaRotationSet(playerState->animationTimer, arcAxis);
            MTXRotRad(rotationMatrix, 0x5A, 0.017453292f * playerState->animationTimer);
            playerState->playerWork.worldPosition.x = targetPosition->x + ((arcRadius * (0.0f > targetPosition->x ? -1 : 1)) * sind(playerState->animationTimer));
        }
        else {
            ArenaRotationSet(playerState->animationTimer, arcAxis);
            MTXRotRad(rotationMatrix, 0x58, 0.017453292f * playerState->animationTimer);
            playerState->playerWork.worldPosition.z = targetPosition->z + ((arcRadius * (0.0f > targetPosition->z ? -1 : 1)) * sind(playerState->animationTimer));
        }
        playerState->playerWork.worldPosition.y = targetPosition->y + (arcRadius * cosd(playerState->animationTimer));
        MTXMultVec(&rotationMatrix[0], &playerState->playerWork.worldPosition, &playerState->playerWork.worldPosition);
    }
    else {
        if (arcAxis == 0x78) {
            playerState->playerWork.inputX = 0x48;
        }
        else {
            playerState->playerWork.inputZ = 0x48;
        }
        PlayerMotionUpdate(&playerState->playerWork);
    }
    if (90.0f <= playerState->animationTimer) {
        if (lbl1Bss114 == 0) {
            ArenaRotationQuarterTurn(arcAxis);
        }
        ArenaHookPositionsUpdate();
    }
}

s32 ArenaPlayerInsideBoundsCheck(float *bounds, ArenaPlayerState *playerState)
{
    if (playerState->playerWork.actionState != 0) {
        return 0;
    }
    if (((bounds[0] - bounds[3]) <= playerState->playerWork.worldPosition.x) && ((bounds[0] + bounds[3]) >= playerState->playerWork.worldPosition.x)
        && ((bounds[2] - bounds[4]) <= playerState->playerWork.worldPosition.z) && ((bounds[2] + bounds[4]) >= playerState->playerWork.worldPosition.z)) {
        return 1;
    }
    return 0;
}

void ArenaPlayerArenaSelectionUpdate(ArenaPlayerState *playerState, ArenaModelLayout *arenaLayout)
{
    s32 modelIndex;

    playerState->selectedArenaIndex = -1;
    if (NULL != arenaLayout) {
        if ((playerState->playerWork.actionState == 1) || (playerState->playerWork.actionState == 3)) {
            return;
        }
        if ((!(1505.0f < playerState->playerWork.worldPosition.y) && !(1495.0f > playerState->playerWork.worldPosition.y)) || (playerState->playerWork.actionState == 1) || (playerState->playerWork.actionState == 3)) {
            for (modelIndex = 0; modelIndex < arenaLayout->modelCount; modelIndex++) {
                if ((0 <= arenaLayout->modelIds[modelIndex]) && ((arenaLayout->modelPositions[modelIndex].x - arenaLayout->modelRadius) <= playerState->playerWork.worldPosition.x)
                    && ((arenaLayout->modelRadius + arenaLayout->modelPositions[modelIndex].x) >= playerState->playerWork.worldPosition.x)
                    && ((arenaLayout->modelPositions[modelIndex].z - arenaLayout->modelRadius) <= playerState->playerWork.worldPosition.z)
                    && ((arenaLayout->modelRadius + arenaLayout->modelPositions[modelIndex].z) >= playerState->playerWork.worldPosition.z)) {
                    playerState->selectedArenaIndex = modelIndex;
                }
            }
        }
    }
}

void ArenaNeighborModelMoveStart(ArenaPlayerState *playerState, ArenaModelLayout *arenaLayout)
{
    s32 neighborModelIndex;
    s32 neighborOffsetIndex;
    s32 neighborColumn;
    s32 neighborRow;
    s32 selectedColumn;
    s32 selectedRow;

    if ((playerState->stateFlags & 0x800) != 0) {
        if ((NULL == arenaLayout) || (playerState->selectedArenaIndex < 0) || (lbl1BssF8 >= 0) || (arenaLayout->layoutComplete != 0)) {
            return;
        }
        selectedColumn = playerState->selectedArenaIndex % arenaLayout->activeModelCount;
        selectedRow = playerState->selectedArenaIndex / arenaLayout->activeModelCount;
        for (neighborOffsetIndex = 0; neighborOffsetIndex < 4; neighborOffsetIndex++) {
            neighborColumn = selectedColumn + lbl1Data1234[neighborOffsetIndex * 2];
            neighborRow = selectedRow + lbl1Data1234[(neighborOffsetIndex * 2) + 1];
            if ((neighborColumn >= 0) && (arenaLayout->activeModelCount > neighborColumn) && (neighborRow >= 0) && (arenaLayout->activeModelCount > neighborRow)) {
                neighborModelIndex = neighborColumn + (neighborRow * arenaLayout->activeModelCount);
                if (arenaLayout->modelIds[neighborModelIndex] < 0) {
                    lbl1BssFC = arenaLayout;
                    lbl1BssF8 = neighborModelIndex;
                    arenaLayout->modelIds[neighborModelIndex] = arenaLayout->modelIds[playerState->selectedArenaIndex];
                    arenaLayout->modelIds[playerState->selectedArenaIndex] = -1;
                    arenaLayout->objectIds[neighborModelIndex] = arenaLayout->objectIds[playerState->selectedArenaIndex];
                    arenaLayout->objectIds[playerState->selectedArenaIndex] = -1;
                    lbl1BssEC = arenaLayout->modelPositions[neighborModelIndex];
                    arenaLayout->modelPositions[neighborModelIndex] = arenaLayout->modelPositions[playerState->selectedArenaIndex];
                    HuAudFXPlay(0x7B8);
                    return;
                }
            }
        }
    }
}

s32 ArenaPlayerTargetListReset(ArenaPlayerState *playerState)
{
    s32 targetSlot;

    playerState->targetCount = 0;
    for (targetSlot = 0; targetSlot < 3; targetSlot++) {
        lbl1BssB8[targetSlot] = -1;
    }
}

void ArenaPlayerTargetRemove(s32 targetIndex)
{
    s32 modelIndex;
    s32 targetSlot;
    ArenaPlayerState *player = lbl1Bss138->data;

    for (targetSlot = 0; targetSlot < 3; targetSlot++) {
        if (targetIndex == lbl1BssB8[targetSlot]) {
            player->targetCount--;
            lbl1BssB8[targetSlot] = -1;
            for (modelIndex = 0; modelIndex < 3; modelIndex++) {
                Hu3DModelAttrSet(lbl1Bss1A0->mdlId[modelIndex + 0x19], HU3D_ATTR_DISPOFF);
            }
            for (modelIndex = targetSlot; modelIndex < 2; modelIndex++) {
                lbl1BssB8[modelIndex] = lbl1BssB8[modelIndex + 1];
                lbl1BssB8[modelIndex + 1] = -1;
            }
        }
    }
}

void ArenaPlayerNearbyTargetFind(ArenaPlayerState *playerState)
{
    s32 targetIndex;

    playerState->attackTimer = -1;
    if (lbl1Bss118 == 5) {
        if ((playerState->playerWork.actionState == 1) || (playerState->playerWork.actionState == 3)) {
            return;
        }
        for (targetIndex = 0; targetIndex < 5; targetIndex++) {
            float horizontalDelta;
            float depthDelta;
            ArenaObjectAnimationState *target = &lbl1Data1450[targetIndex];
            horizontalDelta = playerState->playerWork.worldPosition.x - target->position.x;
            depthDelta = playerState->playerWork.worldPosition.z - target->position.z;
            if (100.0f > sqrtf((horizontalDelta * horizontalDelta) + (depthDelta * depthDelta))) {
                playerState->attackTimer = targetIndex;
            }
        }
    }
}

void ArenaPlayerTargetSelect(ArenaPlayerState *playerState)
{
    s32 targetSlot;

    if (playerState->stateFlags & 0x800) {
        if ((lbl1Bss118 != 5) || (playerState->attackTimer < 0)) {
            return;
        }
        if ((playerState->targetCount < 3) && (lbl1Data1450[playerState->attackTimer].isVisible == 0)) {
            for (targetSlot = 0; targetSlot < playerState->targetCount; targetSlot++) {
                if (playerState->attackTimer == lbl1BssB8[targetSlot]) {
                    return;
                }
            }
            lbl1Data1450[playerState->attackTimer].animationTimer = 0.0f;
            lbl1Data1450[playerState->attackTimer].isVisible = 1;
            lbl1Data1450[playerState->attackTimer].animationActive = 1;
            lbl1Data1450[playerState->attackTimer].soundPlayed = 1;
            lbl1BssB8[playerState->targetCount++] = playerState->attackTimer;
            Hu3DMotionTimeSet(lbl1Bss1A0->mdlId[playerState->attackTimer + 0x14], 0.0f);
            Hu3DModelAttrReset(lbl1Bss1A0->mdlId[playerState->attackTimer + 0x14], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
            if (playerState->targetCount == 2) {
                ArenaLineBetweenWaypointsSet(lbl1BssB8[0], lbl1BssB8[1], 25);
                HuAudFXPlay(0x7B4);
            }
            if (playerState->targetCount == 3) {
                lbl1BssC8 = 0.0f;
                lbl1BssC4 = 0x1388;
                ArenaLineBetweenWaypointsSet(lbl1BssB8[0], lbl1BssB8[2], 26);
                ArenaLineBetweenWaypointsSet(lbl1BssB8[1], lbl1BssB8[2], 27);
                HuAudFXPlay(0x7B5);
                HuAudFXPlay(0x7B6);
            }
        }
    }
}

void ArenaPlayerTargetHeightResolve(ArenaPlayerState *playerState)
{
    ArenaObjectAnimationState *target;

    ArenaPlayerNearbyTargetFind(playerState);
    if (playerState->attackTimer >= 0) {
        target = &lbl1Data1450[playerState->attackTimer];
        if ((playerState->playerWork.actionState != 1) && (playerState->playerWork.actionState != 3) && (1560.0f > playerState->playerWork.worldPosition.y)) {
            playerState->playerWork.worldPosition.y = 1565.0f;
            playerState->playerWork.previousPosition.y = 1565.0f;
        }
        if (0.0f != target->previousHeight) {
            playerState->playerWork.worldPosition.y = 5.0f + target->position.y;
            playerState->playerWork.previousPosition.y = 5.0f + target->position.y;
        }
    }
}

s32 ArenaPlayerCanActCheck(ArenaPlayerState *playerState)
{
    if ((playerState->stateFlags & 0x50) != 0) {
        return 0;
    }
    if ((playerState->stateFlags & 0x500) != 0) {
        return 0;
    }
    if ((playerState->stateFlags & 0x5000) != 0) {
        return 0;
    }
    if ((playerState->stateFlags & 0x50000) != 0) {
        return 0;
    }
    if ((playerState->stateFlags & 0x500000) != 0) {
        return 0;
    }
    if (lbl1DataA0C[0].score <= 0) {
        return 0;
    }
    if ((playerState->stateFlags & 0x80000) != 0) {
        return 0;
    }
    if (playerState->playerWork.actionState != 0) {
        return 0;
    }
    return 1;
}

s32 ArenaPlayerCanMoveCheck(ArenaPlayerState *playerState)
{
    if ((playerState->stateFlags & 0x50) != 0) {
        return 0;
    }
    if ((playerState->stateFlags & 0x500) != 0) {
        return 0;
    }
    if ((playerState->stateFlags & 0x5000) != 0) {
        return 0;
    }
    if ((playerState->stateFlags & 0x50000) != 0) {
        return 0;
    }
    if ((playerState->stateFlags & 0x500000) != 0) {
        return 0;
    }
    if ((playerState->stateFlags & 2) != 0) {
        return 0;
    }
    return 1;
}

void ArenaPlayerRoundStart(ArenaPlayerState *playerState, OMOBJ *playerObject)
{
    playerState->playerState++;
    lbl1Bss1A4->work[0] += 1;
    lbl1Bss128 = 0.0f;
    PlayerMotionSet(&playerState->playerWork, 0);
    Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_ATTR_DISPOFF);
    playerObject->objFunc = ArenaPlayerRoundResultUpdate;
}

void ArenaPlayerSetupStateUpdate(void)
{
    s32 effectSlot;

    for (effectSlot = 0; effectSlot < 1; effectSlot++) {
        lbl1BssE8[effectSlot] = NULL;
    }
    for (effectSlot = 0; effectSlot < 1; effectSlot++) {
        lbl1BssE4[effectSlot] = NULL;
    }
    for (effectSlot = 0; effectSlot < 2; effectSlot++) {
        lbl1BssDC[effectSlot] = NULL;
    }
}

void ArenaPlayerFacingOpponentUpdate(ArenaPlayerState *playerState)
{
    Mtx rotationMatrix;
    Vec playerFacingDirection;
    Vec opponentDirection;
    ArenaOpponentState *opponent = lbl1Bss134->data;

    playerFacingDirection.x = 0.0f;
    playerFacingDirection.y = 0.0f;
    playerFacingDirection.z = 1.0f;
    MTXRotRad(rotationMatrix, 0x59, 0.017453292f * playerState->playerWork.rotationY);
    MTXMultVec(rotationMatrix, &playerFacingDirection, &playerFacingDirection);
    VECNormalize(&playerFacingDirection, &playerFacingDirection);
    VECSubtract(&playerState->playerWork.worldPosition, &opponent->playerWork.worldPosition, &playerState->movementVelocity);
    VECNormalize(&playerState->movementVelocity, &playerState->movementVelocity);
    VECSubtract(&opponent->playerWork.worldPosition, &playerState->playerWork.worldPosition, &opponentDirection);
    VECNormalize(&opponentDirection, &opponentDirection);
    if (0.0f <= VECDotProduct(&playerFacingDirection, &opponentDirection)) {
        playerState->effectTimer = 0x11;
        return;
    }
    playerState->effectTimer = 0x13;
}

s32 ArenaPlayerActionStateUpdate(ArenaPlayerState *playerState, OMOBJ *playerObject)
{
    s32 pointsToSubtract;
    float jumpVelocity;

    if (playerState->stateFlags & 2) {
        if (playerState->stateFlags & 0x40) {
            playerState->transitionProgress += 1.0f;
            if (playerState->transitionProgress == 15.0f) {
                Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_PAUSE);
            }
            if ((15.0f < playerState->transitionProgress) && (Hu3DMotionEndCheck(playerObject->mdlId[0]) != 0)) {
                playerState->stateFlags &= ~0x40;
                PlayerMotionSet(&playerState->playerWork, 0);
            }
            return 0;
        }
        if (playerState->stateFlags & 0x400) {
            playerState->transitionProgress += 1.0f;
            if (playerState->transitionProgress == 15.0f) {
                Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
            }
            if ((15.0f < playerState->transitionProgress) && (Hu3DMotionEndCheck(playerObject->mdlId[0]) != 0)) {
                playerState->stateFlags &= ~0x400;
                PlayerMotionSet(&playerState->playerWork, 0);
            }
            return 0;
        }
        if (playerState->stateFlags & 0x4000) {
            playerState->transitionProgress += 1.0f;
            if (120.0f < playerState->transitionProgress) {
                playerState->stateFlags &= ~0x4000;
                PlayerMotionSet(&playerState->playerWork, 0);
            }
            return 0;
        }
        if (playerState->stateFlags & 0x40000) {
            playerState->transitionProgress += 1.0f;
            if (120.0f < playerState->transitionProgress) {
                playerState->stateFlags &= ~0x40000;
                PlayerMotionSet(&playerState->playerWork, 0);
                Hu3DModelAttrSet(lbl1Bss1A0->mdlId[playerState->currentTarget + 2], HU3D_ATTR_DISPOFF);
            }
            return 0;
        }
        if (playerState->stateFlags & 0x400000) {
            if ((Hu3DMotionShiftIDGet(playerObject->mdlId[0]) < 0) && (Hu3DMotionEndCheck(playerObject->mdlId[0]) != 0)) {
                if (playerState->transitionProgress == 0.0f) {
                    playerState->transitionProgress = 1.0f;
                    Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[playerState->effectTimer + 1], 0.0f, 8.0f, 0);
                }
                else {
                    playerState->stateFlags &= ~0x400000;
                    PlayerMotionSet(&playerState->playerWork, 0);
                }
            }
            return 0;
        }
        if ((playerState->actionIndex & 1) == 0) {
            Hu3DModelAttrSet(playerObject->mdlId[0], HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_ATTR_DISPOFF);
        }
        if (--playerState->actionIndex < 0) {
            playerState->stateFlags &= ~0x70;
            playerState->stateFlags &= ~0x700;
            playerState->stateFlags &= ~0x7000;
            playerState->stateFlags &= ~0x70000;
            playerState->stateFlags &= ~0x700004;
            playerState->stateFlags &= ~0x2;
            Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_ATTR_DISPOFF);
        }
        return 1;
    }
    if (playerState->stateFlags & 0x110) {
        PlayerMotionUpdate(&playerState->playerWork);
        return 0;
    }
    if ((playerState->stateFlags & 0x1000) != 0) {
        if ((Hu3DMotionShiftIDGet(playerObject->mdlId[0]) < 0) && (Hu3DMotionEndCheck(playerObject->mdlId[0]) != 0)) {
            playerState->playerWork.rotationY += (10.0 + (8.0 * sind((180.0f * (playerState->transitionProgress / 60.0f)))));
            if (360.0f <= playerState->playerWork.rotationY) {
                playerState->playerWork.rotationY -= 360.0f;
            }
            playerState->transitionProgress += 1.0f;
            if (60.0f < playerState->transitionProgress) {
                ArenaPlayerStateTransitionApply(playerObject);
            }
        }
        return 0;
    }
    if (playerState->stateFlags & 0x10000) {
        ArenaPlayerStateTransitionApply(playerObject);
        return 0;
    }
    if (playerState->stateFlags & 0x100000) {
        playerState->playerWork.inputX = 72.0f * playerState->movementVelocity.x;
        playerState->playerWork.inputZ = -(72.0f * playerState->movementVelocity.z);
        PlayerMotionUpdate(&playerState->playerWork);
        return 0;
    }
    if ((playerState->stateFlags & 0x20) != 0) {
        if (lbl1Bss118 < 4) {
            omVibrate(lbl1Bss120, 0xC, 4, 2);
        }
        else {
            omVibrate(lbl1Bss120, 0xC, 0xC, 0);
        }
        CharFXPlay(playerState->characterId, 0x123);
        playerState->stateFlags |= 0x10;
        playerState->playerWork.stateFlags |= 0x210;
        PlayerJumpStart(&playerState->playerWork, (10.0f*(160.6f/REFRESH_RATE))*1.8f, 0.0f);
        Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[8], 0.0f, 8.0f, 0);
        if (ScoreDisplaySubtractPoints(0, 1) <= 0) {
            playerState->stateFlags |= 0x80000;
        }
        return 0;
    }
    if (playerState->stateFlags & 0x200) {
        omVibrate(lbl1Bss120, 0xC, 4, 2);
        CharFXPlay(playerState->characterId, 0x123);
        playerState->stateFlags |= 0x100;
        playerState->playerWork.stateFlags |= 0x210;
        PlayerJumpStart(&playerState->playerWork, 0.0f, 0.0f);
        if (ScoreDisplaySubtractPoints(0, 1) <= 0) {
            playerState->stateFlags |= 0x80000;
        }
        return 0;
    }
    if (playerState->stateFlags & 0x2000) {
        omVibrate(lbl1Bss120, 0xC, 4, 2);
        CharFXPlay(playerState->characterId, 0x123);
        playerState->transitionProgress = 0.0f;
        playerState->stateFlags |= 0x1000;
        playerState->playerWork.stateFlags |= 0x10;
        Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[15], 0.0f, 8.0f, 0);
        if (ScoreDisplaySubtractPoints(0, 1) <= 0) {
            playerState->stateFlags |= 0x80000;
        }
        return 0;
    }
    if (playerState->stateFlags & 0x20000) {
        omVibrate(lbl1Bss120, 0xC, 4, 2);
        CharFXPlay(playerState->characterId, 0x123);
        playerState->transitionProgress = 0.0f;
        playerState->stateFlags |= 0x10000;
        playerState->playerWork.stateFlags |= 0x10;
        if (ScoreDisplaySubtractPoints(0, 1) <= 0) {
            playerState->stateFlags |= 0x80000;
        }
        return 0;
    }
    if ((playerState->stateFlags & 0x200000) != 0) {
        omVibrate(lbl1Bss120, 0xC, 0xC, 0);
        CharFXPlay(playerState->characterId, 0x123);
        playerState->transitionProgress = 0.0f;
        playerState->stateFlags |= 0x100000;
        playerState->playerWork.stateFlags |= 0x200;
        playerState->playerWork.stateFlags &= ~0x10;
        jumpVelocity = (10.0f*(160.6f/REFRESH_RATE))*1.5f;
        if (playerState->stateFlags & 4) {
            jumpVelocity = jumpVelocity / 2;
        }
        ArenaPlayerFacingOpponentUpdate(playerState);
        PlayerJumpStart(&playerState->playerWork, jumpVelocity, 0.0f);
        Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[playerState->effectTimer], 0.0f, 8.0f, 0);
        pointsToSubtract = 1;
        if ((playerState->stateFlags & 4) != 0) {
            pointsToSubtract = 1;
        }
        if (ScoreDisplaySubtractPoints(0, pointsToSubtract) <= 0) {
            playerState->stateFlags |= 0x80000;
        }
        return 0;
    }
    return 1;
}

void ArenaPlayerInputUpdate(OMOBJ *playerObject)
{
    s32 canProcessInput;
    float cameraYaw;

    ArenaPlayerState *player = playerObject->data;
    ArenaModelLayout *selectedArenaLayout;

    player->stateFlags |= 1;
    player->stateFlags &= 0xFFFFF7FF;
    ArenaPlayerTargetHeightResolve(player);
    canProcessInput = 1;
    player->playerWork.inputX = 0;
    player->playerWork.inputZ = 0;
    player->playerWork.movementX = 0;
    player->playerWork.movementZ = 0;
    player->playerWork.targetPlayer = 0;
    player->playerWork.collisionCount = 0;
    if (lbl1Bss118 == 5) {
        ArenaPlayerOpponentCollisionResolve();
    }
    canProcessInput = ArenaPlayerActionStateUpdate(player, playerObject);
    if (canProcessInput != 0) {
        if (((player->stateFlags & 0x80000) || (lbl1Bss304 < 0)) && (player->playerWork.actionState == 0)) {
            player->playerState = 0x7EB;
            Hu3DModelAttrReset(*playerObject->mdlId, HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
            Hu3DMotionShiftSet(*playerObject->mdlId, playerObject->mtnId[22], 0.0f, 8.0f, 0);
            Hu3DModelAttrReset(*playerObject->mdlId, HU3D_ATTR_DISPOFF);
            playerObject->objFunc = ArenaPlayerRoundResultUpdate;
            return;
        }
        if (NULL != lbl1DataFAC[lbl1Bss118]) {
            if (ArenaPlayerInsideBoundsCheck(lbl1DataFAC[lbl1Bss118], player) != 0) {
                if (ArenaPlayerCanActCheck(player) != 0) {
                    ArenaPlayerRoundStart(player, playerObject);
                    return;
                }
            }
        }
        player->playerWork.inputX = HuPadStkX[player->controllerIndex];
        player->playerWork.inputZ = HuPadStkY[player->controllerIndex];
        player->playerWork.movementX = HuPadBtnDown[player->controllerIndex];
        player->playerWork.movementZ = HuPadBtn[player->controllerIndex];
        player->playerWork.targetPlayer = HuPadTrigL[player->controllerIndex];
        player->playerWork.collisionCount = HuPadTrigR[player->controllerIndex];
        if ((lbl1Bss118 >= 2) && (lbl1Bss118 <= 4)) {
            player->playerWork.inputX = HuPadStkY[player->controllerIndex];
            player->playerWork.inputZ = -HuPadStkX[player->controllerIndex];
        }
        PlayerMotionUpdate(&player->playerWork);
    }
    if ((1490.0f > player->playerWork.worldPosition.y) && (player->playerWork.actionState != 1) && (player->playerWork.actionState != 3)) {
        omVibrate(lbl1Bss120, 0xC, 4, 2);
        CharFXPlay(player->characterId, 0x123);
        player->stateFlags |= 0x800000;
        player->playerState = 0x7EB;
        PlayerJumpStart(&player->playerWork, (10.0f*(160.6f/REFRESH_RATE))*2.0f, 0.0f);
        Hu3DMotionSet(playerObject->mdlId[0], playerObject->mtnId[8]);
        Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_ATTR_DISPOFF);
        ScoreDisplaySubtractPoints(0, 0);
        playerObject->objFunc = ArenaPlayerRoundResultUpdate;
        return;
    }
    ArenaPlayerEffectCollisionsUpdate(player);

    selectedArenaLayout = NULL;
    if (lbl1Bss118 == 2) {
        selectedArenaLayout = &arenaLayout;
    }
    if (lbl1Bss118 == 4) {
        selectedArenaLayout = &lbl1Data13B8;
    }
    ArenaPlayerArenaSelectionUpdate(player, selectedArenaLayout);
    ArenaNeighborModelMoveStart(player, selectedArenaLayout);
    if (NULL != selectedArenaLayout) {
        if ((player->selectedArenaIndex >= 0) && (((selectedArenaLayout->modelVelocities[player->selectedArenaIndex].x) != 0.0f) || (selectedArenaLayout->modelVelocities[player->selectedArenaIndex].z != 0.0f))) {
            VECAdd(&selectedArenaLayout->modelVelocities[player->selectedArenaIndex], &player->playerWork.worldPosition, &player->playerWork.worldPosition);
            player->playerWork.previousPosition = player->playerWork.worldPosition;
        }
        if (selectedArenaLayout->layoutComplete != 0) {
            if (ArenaPlayerCanActCheck(player) != 0) {
                ArenaPlayerRoundStart(player, playerObject);
                return;
            }
        }
    }
    if ((lbl1Bss118 == 5) && (lbl1DataA0C[1].score <= 0)) {
        if (ArenaPlayerCanActCheck(player) != 0) {
            lbl1Bss11C = lbl1Bss120;
            ArenaPlayerRoundStart(player, playerObject);
            AudioSequenceFadeOut(0x3E8);
            return;
        }
    }
    ArenaPlayerNearbyTargetFind(player);
    ArenaPlayerTargetSelect(player);
    if (lbl1Bss118 < 5) {
        cameraYaw = 0.0;
        if ((lbl1Bss118 >= 2) && (lbl1Bss118 <= 4)) {
            cameraYaw = 270.0f;
        }
        ArenaCameraTargetSet(cameraYaw, 1500.0f);
        CameraLookAtTransition(0.5f, &lbl1Data470);
    }
    omSetTra(player->playerWork.object, player->playerWork.worldPosition.x, player->playerWork.worldPosition.y, player->playerWork.worldPosition.z);
    omSetRot(player->playerWork.object, 0.0f, player->playerWork.rotationY, 0.0f);
    ArenaPlayerSetupStateUpdate();
}

void ArenaPlayerActionComplete(ArenaPlayerState *playerState, OMOBJ *playerObject)
{
    playerState->playerState++;
    playerState->stateFlags &= ~0x802;
    playerState->stateFlags &= ~0x70;
    playerState->stateFlags &= ~0x700;
    playerState->stateFlags &= ~0x7000;
    playerState->stateFlags &= ~0x70000;
    playerState->stateFlags &= ~0x7;
    ArenaPlayerTargetListReset(playerState);
    playerObject->objFunc = ArenaPlayerInputUpdate;
}

void ArenaPlayerRoundResultUpdate(OMOBJ *playerObject)
{
    Vec opponentDirection;
    float horizontalDistance;
    float motionProgress;
    float horizontalComponent;
    float depthComponent;

    ArenaPlayerState *player;
    ArenaPlayerState *playerWorkState;
    ArenaWaypoint *waypoint;
    s32 modelIndex;

    player = playerObject->data;
    player->stateFlags &= 0xFFFFFFFE;
    player->playerWork.inputX = 0;
    player->playerWork.inputZ = 0;
    player->playerWork.movementX = 0;
    player->playerWork.movementZ = 0;
    player->playerWork.targetPlayer = 0;
    player->playerWork.collisionCount = 0;
    CharModelFxFlagSet(player->characterId, 1);
    lbl1Bss10C = 0;
    switch (player->playerState) {
        case 0x7D0:
            if (lbl1Bss1A4->work[0] > 0x3EC) {
                player->playerState++;
                Hu3DModelShadowSet(playerObject->mdlId[0]);
                if (MinigameTypeIsAvailable(0) != 0) {
                    player->playerWork.rotationY = 180.0f;
                    PlayerMotionSet(&player->playerWork, 0);
                }
                else {
                    player->playerWork.rotationY = 0.0f;
                    PlayerMotionSet(&player->playerWork, 5);
                }
                player->playerWork.stateFlags |= 0x20;
            }
            break;
        case 0x7D1:
            PlayerMotionUpdate(&player->playerWork);
            if (player->playerWork.actionState == 2) {
                player->playerState++;
                player->selectedArenaIndex = 0;
                player->currentWaypoint = -1;
                player->targetWaypoints = lbl1Data94C;
                player->targetRotation = 360.0f;
                omVibrate(lbl1Bss120, 0xC, 0xC, 0);
                if (MinigameTypeIsAvailable(0) != 0) {
                    player->playerState = 0x7D4;
                    player->animationTimer = 0.0f;
                    PlayerMotionSet(&player->playerWork, 0);
                }
                player->playerWork.stateFlags &= ~0x20;
            }
            break;
        case 0x7D2:
        case 0x7DA:
            if (player->targetRotation != 360.0f) {
                player->playerWork.rotationY = AngleInterpolate(player->playerWork.rotationY, player->targetRotation, 0.05f);
            }
            if (player->currentWaypoint < 0) {
                player->currentWaypoint = player->selectedArenaIndex;
                player->selectedArenaIndex++;
                waypoint = &player->targetWaypoints[player->currentWaypoint];
                if (waypoint->targetY == 0.0f) {
                    Hu3DMotionSet(playerObject->mdlId[0], playerObject->mtnId[waypoint->targetIndex]);
                    Hu3DMotionTimeSet(playerObject->mdlId[0], waypoint->targetZ);
                }
                else {
                    Hu3DMotionShiftSet(
                        playerObject->mdlId[0], playerObject->mtnId[waypoint->targetIndex], waypoint->targetZ, waypoint->targetY, waypoint->targetFlags & 0xBFFFFFFD);
                }
                Hu3DMotionSpeedSet(playerObject->mdlId[0], waypoint->targetX);
                Hu3DMotionShiftSpeedSet(playerObject->mdlId[0], waypoint->targetX);
                if (waypoint->targetFlags & 0x40000003) {
                    player->playerState++;
                    Hu3DMotionSpeedSet(playerObject->mdlId[0], 1.0f);
                    Hu3DMotionShiftSpeedSet(playerObject->mdlId[0], 1.0f);
                }
            }
            else if ((Hu3DMotionShiftIDGet(playerObject->mdlId[0]) < 0) && (Hu3DMotionEndCheck(playerObject->mdlId[0]) != 0)) {
                player->currentWaypoint = -1;
                if (player->targetWaypoints[player->selectedArenaIndex].targetIndex < 0) {
                    player->playerState++;
                    Hu3DMotionSpeedSet(playerObject->mdlId[0], 1.0f);
                    Hu3DMotionShiftSpeedSet(playerObject->mdlId[0], 1.0f);
                }
            }
            break;
        case 0x7D3:
            if (Hu3DMotionShiftIDGet(playerObject->mdlId[0]) < 0) {
                motionProgress = Hu3DMotionTimeGet(playerObject->mdlId[0]) / Hu3DMotionMaxTimeGet(playerObject->mdlId[0]);
                if (0.5f > motionProgress) {
                    motionProgress = 0.5f;
                }
                player->playerWork.rotationY = 180.0f * (2.0f * (motionProgress - 0.5f));
                if (Hu3DMotionEndCheck(playerObject->mdlId[0]) != 0) {
                    player->playerState++;
                    player->animationTimer = 0.0f;
                    PlayerMotionSet(&player->playerWork, 0);
                    Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[0], 0.0f, 8.0f, 0x40000001);
                }
            }
            break;
        case 0x7D4:
            if ((lbl1Bss1A4->work[0] >= 0x3EF) && (120.0f < lbl1Bss128)) {
                player->playerState++;
            }
            break;
        case 0x7D5:
            if (90.0f > player->animationTimer) {
                ArenaPlayerArcMove(player, lbl1Bss4C, 0x7A);
            }
            else {
                if (lbl1Bss1A4->work[0] < 0x3F0) {
                    lbl1Bss1A4->work[0] = 0x3F0;
                    lbl1Bss128 = 0.0f;
                    lbl1Bss324 = lbl1Data4B8;
                    lbl1Bss30C = lbl1Data4C4;
                }
                if ((lbl1Bss4C->z - 100.0f) < player->playerWork.worldPosition.z) {
                    player->playerWork.inputZ = 0x48;
                    PlayerMotionUpdate(&player->playerWork);
                }
                else {
                    player->playerState++;
                    PlayerMotionSet(&player->playerWork, 0);
                }
            }
            break;
        case 0x7D6:
            if (lbl1Bss1A4->work[0] >= 0x3F3) {
                ArenaPlayerActionComplete(player, playerObject);
            }
            break;
        case 0x7D8:
            player->targetPosition.x = 1200.0f;
            player->targetPosition.z = 0.0f;
            if (PlayerMoveTowardPosition(&player->playerWork, &player->targetPosition) != 0) {
                if (lbl1Bss1A4->work[0] >= 0x3F5) {
                    player->playerState++;
                    player->animationTimer = 0.0f;
                    player->playerWork.worldPosition.z = 0.0f;
                }
                else {
                    player->playerWork.rotationY = AngleInterpolate(player->playerWork.rotationY, 90.0f, 0.4f);
                    player->playerWork.worldPosition.z = 0.0f;
                }
            }
            PlayerMotionUpdate(&player->playerWork);
            break;
        case 0x7D9:
            if (90.0f > player->animationTimer) {
                ArenaPlayerArcMove(player, &lbl1Bss4C[1], 0x78);
            }
            else {
                if ((-1000.0f < player->playerWork.worldPosition.x) && (lbl1Bss1A4->work[0] < 0x3F6)) {
                    lbl1Bss1A4->work[0] = 0x3F6;
                    lbl1Bss128 = 0.0f;
                }
                if (lbl1Bss1A4->work[0] >= 0x3F7) {
                    player->playerState = 0x7DB;
                    PlayerMotionSet(&player->playerWork, 0);
                }
                if (arenaLayout.modelIds[2] >= 0) {
                    player->targetPosition = arenaLayout.modelPositions[2];
                }
                else {
                    player->targetPosition = arenaLayout.modelPositions[3];
                }
                PlayerMoveTowardPosition(&player->playerWork, &player->targetPosition);
                PlayerMotionUpdate(&player->playerWork);
            }
            break;
        case 0x7DB:
            if ((lbl1Bss1A4->work[0] <= 0x3F9) && (Hu3DMotionShiftIDGet(playerObject->mdlId[0]) < 0) && (lbl1BssD8 > 0)) {
                VECScale(&lbl1BssCC, &lbl1BssCC, 1.0f / lbl1BssD8);
                ArenaPlayerSpecialMotionSet(player, playerObject, &lbl1BssCC);
            }
            if (lbl1Bss1A4->work[0] >= 0x3FA) {
                ArenaPlayerSpecialMotionSet(player, playerObject, NULL);
            }
            if (lbl1Bss1A4->work[0] >= 0x3FB) {
                ArenaPlayerSpecialMotionReset(player, playerObject);
                ArenaPlayerActionComplete(player, playerObject);
            }
            break;
        case 0x7DD:
            player->targetPosition.x = 300.0f;
            player->targetPosition.z = 0.0f;
            if (lbl1Bss1A4->work[0] >= 0x3FD) {
                if (PlayerMoveTowardPosition(&player->playerWork, &player->targetPosition) != 0) {
                    player->playerWork.rotationY = AngleInterpolate(player->playerWork.rotationY, 90.0f, 0.4f);
                    if (lbl1Bss1A4->work[0] <= 0x3FD) {
                        lbl1Bss1A4->work[0] += 1;
                        lbl1Bss128 = 0.0f;
                    }
                    if (lbl1Bss1A4->work[0] >= 0x402) {
                        player->playerState++;
                        player->animationTimer = 0.0f;
                    }
                }
                PlayerMotionUpdate(&player->playerWork);
            }
            break;
        case 0x7DE:
            if (90.0f > player->animationTimer) {
                ArenaPlayerArcMove(player, &lbl1Bss4C[2], 0x78);
            }
            else if ((100.0f + lbl1Bss4C[2].x) > player->playerWork.worldPosition.x) {
                player->playerWork.inputX = 0x48;
                PlayerMotionUpdate(&player->playerWork);
            }
            else {
                player->playerState++;
                PlayerMotionSet(&player->playerWork, 0);
                lbl1Bss1A4->work[0] = 0x403;
                lbl1Bss128 = 0.0f;
            }
            break;
        case 0x7DF:
            if (lbl1Bss1A4->work[0] >= 0x405) {
                ArenaPlayerActionComplete(player, playerObject);
            }
            break;
        case 0x7E1:
            player->targetPosition.x = 1200.0f;
            player->targetPosition.z = 0.0f;
            if (PlayerMoveTowardPosition(&player->playerWork, &player->targetPosition) != 0) {
                if (lbl1Bss1A4->work[0] >= 0x407) {
                    player->playerState++;
                    player->animationTimer = 0.0f;
                    player->playerWork.worldPosition.z = 0.0f;
                }
                else {
                    player->playerWork.rotationY = AngleInterpolate(player->playerWork.rotationY, 90.0f, 0.4f);
                    player->playerWork.worldPosition.z = 0.0f;
                }
            }
            PlayerMotionUpdate(&player->playerWork);
            break;
        case 0x7E2:
            if (90.0f > player->animationTimer) {
                ArenaPlayerArcMove(player, &lbl1Bss4C[3], 0x78);
            }
            else {
                if ((-1000.0f < player->playerWork.worldPosition.x) && (lbl1Bss1A4->work[0] < 0x408)) {
                    lbl1Bss1A4->work[0] = 0x408;
                    lbl1Bss128 = 0.0f;
                }
                if (lbl1Bss1A4->work[0] >= 0x409) {
                    player->playerState++;
                }
                player->targetPosition = lbl1Data13B8.modelPositions[4];
                PlayerMoveTowardPosition(&player->playerWork, &player->targetPosition);
                PlayerMotionUpdate(&player->playerWork);
            }
            break;
        case 0x7E3:
            if (lbl1Bss1A4->work[0] >= 0x40D) {
                ArenaPlayerActionComplete(player, playerObject);
            }
            break;
        case 0x7E5:
            player->targetPosition.x = 0.0f;
            player->targetPosition.z = -400.0f;
            if (lbl1Bss1A4->work[0] >= 0x410) {
                if (PlayerMoveTowardPosition(&player->playerWork, &player->targetPosition) != 0) {
                    player->playerWork.rotationY = AngleInterpolate(player->playerWork.rotationY, 180.0f, 0.4f);
                    if ((lbl1Bss1A4->work[0] <= 0x410) && (120.0f <= lbl1Bss128)) {
                        lbl1Bss1A4->work[0] += 1;
                        lbl1Bss128 = 0.0f;
                        player->playerState++;
                        player->animationTimer = 0.0f;

                        ArenaModelLayoutShow(&lbl1Data13B8, 0x10);
                        AudioSequenceFadeOut(0x3E8);
                    }
                }
                PlayerMotionUpdate(&player->playerWork);
            }
            break;
        case 0x7E6:
            if (90.0f > player->animationTimer) {
                ArenaPlayerArcMove(player, &lbl1Bss4C[4], 0x7A);
            }
            else {
                if (lbl1Bss1A4->work[0] <= 0x411) {
                    lbl1Bss1A4->work[0] = 0x412;
                    lbl1Bss128 = 0.0f;
                    CameraPoseSet(&lbl1Bss324, &lbl1Data470, &lbl1Bss30C);
                    lbl1Bss324 = lbl1Data4D0;
                    lbl1Bss30C = lbl1Data4DC;
                }
                player->targetPosition.x = 0.0f;
                player->targetPosition.z = 600.0f;
                if (PlayerMoveTowardPosition(&player->playerWork, &player->targetPosition) != 0) {
                    ++player->playerState;
                    PlayerMotionSet(&player->playerWork, 0);
                }
                else {
                    PlayerMotionUpdate(&player->playerWork);
                }
            }
            break;
        case 0x7E7:
            player->playerWork.rotationY = AngleInterpolate(player->playerWork.rotationY, 180.0f, 0.4f);
            if (lbl1Bss1A4->work[0] >= 0x415) {
                ArenaPlayerActionComplete(player, playerObject);
            }
            break;
        case 0x7E9:
            if (lbl1Bss1A4->work[0] >= 0x417) {
                if (((REFRESH_RATE/6)*10.0f) < lbl1Bss128) {
                    player->playerState++;
                    GWPlayerCoinWinSet(lbl1Bss120, 10);
                    Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[21], 0.0f, 8.0f, 0);
                    lbl1Bss18 = HuAudSStreamPlay(0xD);
                }
            }
            if ((lbl1Bss1A4->work[0] == 0x416) && (240.0f < lbl1Bss128)) {
                lbl1Bss1A4->work[0] += 1;
                lbl1Bss128 = 0.0f;
            }
            break;
        case 0x7EA:
            VECSubtract(&((ArenaOpponentState *)lbl1Bss134->data)->playerWork.worldPosition, &player->playerWork.worldPosition, &opponentDirection);
            player->playerWork.rotationY = AngleInterpolate(player->playerWork.rotationY, 180.0 + (atan2d(opponentDirection.x, opponentDirection.z)), 0.4f);
            break;
        case 0x7EB:
            if (player->stateFlags & 0x800000) {
                playerWorkState = player;
                playerWorkState->playerWork.acceleration.y += -146.0f / REFRESH_RATE;
                playerWorkState->playerWork.worldPosition.y += playerWorkState->playerWork.acceleration.y;
                playerWorkState->playerWork.jumpHeight = MapPos(
                    playerWorkState->playerWork.worldPosition.x, playerWorkState->playerWork.worldPosition.y, playerWorkState->playerWork.worldPosition.z, playerWorkState->playerWork.collisionHeight, &playerWorkState->playerWork.collisionPosition);
                if (playerWorkState->playerWork.jumpHeight > playerWorkState->playerWork.worldPosition.y) {
                    PlayerJumpStart(&player->playerWork, (10.0f*(160.6f/REFRESH_RATE))*2.0f, 0.0f);
                }
                horizontalComponent = playerWorkState->playerWork.worldPosition.x;
                depthComponent = playerWorkState->playerWork.worldPosition.z;
                horizontalDistance = sqrtf((horizontalComponent * horizontalComponent) + (depthComponent * depthComponent));
                if (horizontalDistance == 0.0f) {
                    horizontalComponent = 0.0f;
                    depthComponent = -1.0f;
                }
                else {
                    horizontalComponent /= horizontalDistance;
                    depthComponent /= horizontalDistance;
                }
                playerWorkState->playerWork.acceleration.x = 72.0f * horizontalComponent / 4;
                playerWorkState->playerWork.acceleration.z = 72.0f * depthComponent / 4;
                playerWorkState->playerWork.worldPosition.x += playerWorkState->playerWork.acceleration.x;
                playerWorkState->playerWork.worldPosition.z += playerWorkState->playerWork.acceleration.z;
                playerWorkState->playerWork.rotationY = AngleInterpolate(playerWorkState->playerWork.rotationY, atan2d(playerWorkState->playerWork.acceleration.x, playerWorkState->playerWork.acceleration.z), 0.4f);
            }
            break;
        default:
            OSReport("*** player mode error(%d)!!\n", player->playerState);
            break;
    }
    if ((player->playerState >= 0x7E9) && (player->playerState <= 0x7EA)) {
        ArenaPlayerTargetHeightResolve(player);
    }
    if (1495.0f > player->playerWork.worldPosition.y) {
        for (modelIndex = 0; modelIndex < 1; modelIndex++) {
            Hu3DModelShadowDispOff(playerObject->mdlId[modelIndex]);
        }
    }
    else {
        for (modelIndex = 0; modelIndex < 1; modelIndex++) {
            Hu3DModelShadowDispOn(playerObject->mdlId[modelIndex]);
        }
    }
    omSetTra(player->playerWork.object, player->playerWork.worldPosition.x, player->playerWork.worldPosition.y, player->playerWork.worldPosition.z);
    omSetRot(player->playerWork.object, 0.0f, player->playerWork.rotationY, 0.0f);
}

void ArenaPlayerCharacterInit(OMOBJ *object)
{
    Vec spawnPosition;
    ArenaPlayerState *player;
    s32 motionIndex;
    s32 controllerIndex;
    s32 characterId;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(ArenaPlayerState), HU_MEMNUM_OVL);
    player = object->data;
    player->stateFlags = 0;
    player->characterId = GWPlayerCfg[lbl1Bss120].character;
    player->controllerIndex = GWPlayerCfg[lbl1Bss120].pad_idx;
    player->playerState = 0x7D0;
    player->collisionRadius = 0.0f;
    player->movementProgress = 0.0f;
    spawnPosition.x = 0.0f;
    spawnPosition.y = 4000.0f;
    spawnPosition.z = 0.0f;
    characterId = player->characterId;
    controllerIndex = lbl1Bss120;
    PlayerMovementWorkInit(&player->playerWork, object, controllerIndex, characterId, lbl1Data92C, 0, &spawnPosition, 0.0f);
    player->playerWork.updateHook = ArenaPlayerBoundsClamp;
    player->playerWork.collisionHook = ArenaPlayerStateTransitionApply;
    player->playerWork.stateHook = ArenaPlayerHitStateSet;
    object->mdlId[0] = CharModelCreate(lbl1Data0[player->characterId], 2);
    CharModelStepFxSet(lbl1Data0[player->characterId], 0);
    for (motionIndex = 0; motionIndex < 0x17; motionIndex++) {
        object->mtnId[motionIndex] = CharMotionCreate(lbl1Data0[player->characterId], lbl1Data64C[player->characterId][motionIndex]);
        CharMotionSet(lbl1Data0[player->characterId], object->mtnId[motionIndex]);
    }
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[0], 2);
    Hu3DMotionSet(object->mdlId[0], *object->mtnId);
    omSetTra(player->playerWork.object, player->playerWork.worldPosition.x, player->playerWork.worldPosition.y, player->playerWork.worldPosition.z);
    omSetRot(player->playerWork.object, 0.0f, player->playerWork.rotationY, 0.0f);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    CharModelLayerSetAll2(6);
    CharMotionDataClose(lbl1Data0[player->characterId]);
    object->objFunc = ArenaPlayerRoundResultUpdate;
}

void ArenaCameraMotionSequenceUpdate(OMOBJ *mainObject, s32 sequenceId)
{
    lbl1Bss108 = 1;
    lbl1Bss128 += 1.0f;
    if ((CameraPoseInterpolate(lbl1Bss128 / 120.0f, &lbl1Data470) != 0) && (MGSeqStatGet(lbl1Bss2FE) == 0)) {
        lbl1Bss2FE = -1;
        mainObject->work[0] += 1;
        CameraPoseSet(&lbl1Bss324, &lbl1Data470, &lbl1Bss30C);
        ScoreDisplayShow(0);
        if (lbl1Bss118 == 5) {
            ScoreDisplayShow(1);
        }
    }
    if ((sequenceId >= 0) && (lbl1Bss2F8 < 0) && ((MGSeqStatGet(lbl1Bss2FE) & 0x10) != 0)) {
        AudioSequencePlay(sequenceId);
    }
}

void ArenaCameraAndMinigameSequenceUpdate(OMOBJ *mainObject, float cameraYaw, float transitionDuration)
{
    lbl1Bss108 = 0;
    lbl1Bss110 = 0;
    if (lbl1Bss2FE < 0) {
        nMap = 0;
        FireEffectsStop();
        MGSeqParamSet(lbl1Bss2FC, 2, -1);
        lbl1Bss128 = 0.0f;
        lbl1Bss2FC = -1;
        lbl1Bss2FE = MGSeqCreate(3, 1);
        return;
    }
    lbl1Bss128 += 1.0f;
    if (lbl1Bss118 < 5) {
        ArenaCameraTargetSet(cameraYaw, 1500.0f);
    }
    else {
        ArenaPlayerOpponentCameraSet();
    }
    CameraPoseInterpolate(lbl1Bss128 / transitionDuration, &lbl1Data470);

    if ((transitionDuration < lbl1Bss128) && (MGSeqStatGet(lbl1Bss2FE) == 0)) {
        mainObject->work[0] += 1;
        lbl1Bss128 = 0.0f;
        lbl1Bss124 = 0.0f;
        lbl1Bss2FE = -1;
        CameraPoseSet(&lbl1Bss324, &lbl1Data470, &lbl1Bss30C);
    }
}

void ArenaMainStateUpdate(OMOBJ *mainObject)
{
    Vec cameraDirection;
    s32 shouldAdvanceSequence;
    s32 sequenceReady;
    HU3DCAMERA *camera;
    float fadeAmount;
    ArenaModelLayout *arenaLayout;

    if (omSysExitReq != 0) {
        AudioSequenceFadeOut(0x64);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        mainObject->objFunc = ArenaSceneExit;
    }
    lbl1Bss308++;
    switch (mainObject->work[0]) {
        case 0x3E8:
            if (lbl1Bss2FE < 0) {
                AudioSequenceFadeOut(0x64);
                MGSeqParamSet(lbl1Bss2FC, 2, -1);
                lbl1Bss2FC = -1;
                lbl1Bss2FE = MGSeqCreate(3, 1);
                lbl1Bss1C = -1;
            }
            else {
                lbl1Bss128 += 1.0f;
                sequenceReady = 0;
                if (lbl1Bss1C < 0) {
                    if (120.0f < lbl1Bss128) {
                        lbl1Bss1C = HuAudSStreamPlay(3);
                    }
                }
                else if (HuAudSStreamStatGet(lbl1Bss1C) == 0) {
                    sequenceReady = 1;
                }
                if ((sequenceReady != 0) || (1200.0f < lbl1Bss128)) {
                    AudioSequenceFadeOut(0x64);
                    lbl1Bss2FE = -1;
                    WipeColorSet(0, 0, 0);
                    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                    mainObject->objFunc = ArenaSceneExit;
                }
            }
            break;
        case 0x3E9:
            lbl1Bss128 += 1.0f;
            if (60.0f < lbl1Bss128) {
                mainObject->work[0] += 1;
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
                HuAudFXPlay(0x7B2);
            }
            break;
        case 0x3EA:
            if (WipeStatGet() == 0) {
                mainObject->work[0] += 1;
                lbl1Bss128 = 0.0f;
            }
            break;
        case 0x3EB:
            lbl1Bss128 += 1.0f;
            if (120.0f < lbl1Bss128) {
                mainObject->work[0] += 1;
                lbl1Bss128 = 0.0f;
                lbl1Bss324 = lbl1Data488;
                lbl1Bss30C = lbl1Data494;
                AudioSequencePlay(0x41);
            }
            break;
        case 0x3EC:
            lbl1Bss128 += 1.0f;
            if (CameraPoseInterpolate(lbl1Bss128 / 120.0f, &lbl1Data470) != 0) {
                mainObject->work[0] += 1;
                lbl1Bss128 = 0;
                CameraPoseSet(&lbl1Bss324, &lbl1Data470, &lbl1Bss30C);
                lbl1Bss324 = lbl1Data4A0;
                lbl1Bss30C = lbl1Data4AC;
            }
            break;
        case 0x3ED:
            if (((ArenaPlayerState *)lbl1Bss138->data)->playerState >= 0x7D3) {
                lbl1Bss128 += 1.0f;
                if (CameraPoseInterpolate(lbl1Bss128 / 120.0f, &lbl1Data470) != 0) {
                    mainObject->work[0] += 1;
                    CameraPoseSet(&lbl1Bss324, &lbl1Data470, &lbl1Bss30C);
                    MessageProcessStart(ArenaIntroSequence, 0);
                }
            }
            break;
        case 0x3EE:
        case 0x3F1:
        case 0x3F8:
        case 0x401:
        case 0x40A:
        case 0x413:
            if (lbl1Bss10 == 0) {
                mainObject->work[0] += 1;
                lbl1Bss128 = 0.0f;
            }
            break;
        case 0x3EF:
            lbl1Bss128 += 1.0f;
            if (120.0f < lbl1Bss128) {
                ArenaPlayerCameraTargetSet();
                CameraLookAtTransition((lbl1Bss128 - 120.0f) / 240.0f, &lbl1Data470);
            }
            break;
        case 0x3F0:
            lbl1Bss128 += 1.0f;
            if (CameraPoseInterpolate(lbl1Bss128 / 120.0f, &lbl1Data470) != 0) {
                mainObject->work[0] += 1;
                CameraPoseSet(&lbl1Bss324, &lbl1Data470, &lbl1Bss30C);
                MessageProcessStart(ArenaTournamentSequenceA, 1);
            }
            if (MinigameTypeIsAvailable(1) != 0) {
                AudioSequenceFadeOut(0x3E8);
            }
            break;
        case 0x3F2:
            if (lbl1Bss2FE < 0) {
                lbl1Bss128 = 0.0f;
                lbl1Bss2FE = MGSeqCreate(3, 0);
                FireEffectArrayInit(&lbl1DataA80[0], 9);
                ArenaCourseSetup(1);
                ArenaPlayerCameraTargetSet();
            }
            else {
                ArenaCameraMotionSequenceUpdate(mainObject, 0x40);
            }
            break;
        case 0x3F3:
        case 0x3FB:
        case 0x405:
        case 0x40D:
        case 0x415:
            if (lbl1Bss2FC >= 0) {
                MGSeqParamSet(lbl1Bss2FC, 1, (lbl1Bss304 + REFRESH_RATE - 1) / REFRESH_RATE);
            }
            else {
                shouldAdvanceSequence = 0;
                if ((lbl1Bss118 == 2) || (lbl1Bss118 == 4) || (lbl1Bss118 == 5)) {
                    if (lbl1Bss304 < 30 * REFRESH_RATE) {
                        shouldAdvanceSequence = 1;
                    }
                }
                else {
                    shouldAdvanceSequence = 1;
                }
                if (shouldAdvanceSequence != 0) {
                    lbl1Bss2FC = MGSeqCreate(1, lbl1Bss304 / REFRESH_RATE, -1, -1);
                }
            }
            lbl1Bss304--;
            if (((ArenaPlayerState *)lbl1Bss138->data)->playerState >= 0x7EB) {
                mainObject->work[0] = 0x3E8;
                lbl1Bss128 = 0.0f;
            }
            break;
        case 0x3F4:
            ArenaCameraAndMinigameSequenceUpdate(mainObject, 270.0f, 2 * REFRESH_RATE);
            break;
        case 0x3F5:
            lbl1Bss128 += 1.0f;
            ArenaCameraTargetSet(270.0f, 0.0f);
            CameraPoseInterpolate(1.0f, &lbl1Data470);
            break;
        case 0x3F6:
            lbl1Bss128 += 1.0f;
            if (90.0f < lbl1Bss128) {
                mainObject->work[0] += 1;
                lbl1Bss128 = 0.0f;
            }
            CameraPoseSet(&lbl1Bss324, &lbl1Data470, &lbl1Bss30C);
            lbl1Bss20 = -1;
            break;
        case 0x3F7:
            if (lbl1Bss20 < 0) {
                lbl1Bss20 = HuAudFXPlay(0x7B1);
            }
            omVibrate(lbl1Bss120, 0xC, 6, 6);
            lbl1Bss128 += 1.0f;
            Hu3DModelPosSet(lbl1Bss1A0->mdlId[6], 0.0f, -(2.0f * lbl1Bss128), 0.0f);
            if (2 * REFRESH_RATE < lbl1Bss128) {
                mainObject->work[0] += 1;
                MessageProcessStart(ArenaTournamentSequenceB, 2);
                HuAudFXPlay(0x7BB);
            }
            break;
        case 0x3F9:
            break;
        case 0x3FA:
            if (lbl1Bss2FE < 0) {
                lbl1Bss128 = 0.0f;
                lbl1Bss2FE = MGSeqCreate(3, 0);
                ArenaCourseSetup(2);
                ArenaCameraTargetSet(270.0f, 0.0f);
            }
            else {
                ArenaCameraMotionSequenceUpdate(mainObject, -1);
            }
            break;
        case 0x3FC:
            fadeAmount = lbl1Bss128 / (2 * REFRESH_RATE);
            if (1.0f < fadeAmount) {
                fadeAmount = 1.0f;
            }
            arenaLayout = &arenaLayout;
            Hu3DModelAttrReset(lbl1Bss1A0->mdlId[5], HU3D_ATTR_DISPOFF);
            Hu3DModelRotSet(lbl1Bss1A0->mdlId[5], 0.0f, 90.0f, 0.0f);
            Hu3DModelPosSet(lbl1Bss1A0->mdlId[5], arenaLayout->modelPositions[3].x, arenaLayout->modelPositions[3].y, arenaLayout->modelPositions[3].z);
            Hu3DModelTPLvlSet(lbl1Bss1A0->mdlId[5], fadeAmount);
            ArenaCameraAndMinigameSequenceUpdate(mainObject, 270.0f, 2 * REFRESH_RATE);
            break;
        case 0x3FD:
        case 0x3FE:
        case 0x3FF:
            lbl1Bss124 += 1.0f;
            ArenaCameraTargetSet(270.0f, 0.0f);
            CameraLookAtTransition(lbl1Bss124 / 120.0f, &lbl1Data470);
            lbl1Bss20 = -1;
            break;
        case 0x400:
            lbl1Bss124 += 1.0f;
            ArenaCameraTargetSet(270.0f, 0.0f);
            CameraLookAtTransition(lbl1Bss124 / 120.0f, &lbl1Data470);
            if (lbl1Bss20 < 0) {
                lbl1Bss20 = HuAudFXPlay(0x7B1);
            }
            omVibrate(lbl1Bss120, 0xC, 6, 6);
            lbl1Bss128 += 1.0f;
            Hu3DModelAttrReset(lbl1Bss1A0->mdlId[7], HU3D_ATTR_DISPOFF);
            Hu3DModelPosSet(lbl1Bss1A0->mdlId[7], 0.0f, -(60.0f - (lbl1Bss128 / 2)), 0.0f);
            if (120.0f < lbl1Bss128) {
                mainObject->work[0] += 1;
                lbl1Bss128 = 0;
                MessageProcessStart(ArenaFinalRoundSequence, -1);
                ArenaModelLayoutShow(&arenaLayout, 5);
                CameraPoseSet(&lbl1Bss324, &lbl1Data470, &lbl1Bss30C);
                HuAudFXPlay(0x7BB);
            }
            break;
        case 0x402:
            lbl1Bss128 += 1.0f;
            ArenaCameraTargetSet(270.0f, 0.0f);
            CameraPoseInterpolate(1.0f, &lbl1Data470);
            break;
        case 0x403:
            CameraPoseSet(&lbl1Bss324, &lbl1Data470, &lbl1Bss30C);
            break;
        case 0x404:
            if (lbl1Bss2FE < 0) {
                lbl1Bss128 = 0.0f;
                lbl1Bss2FE = MGSeqCreate(3, 0);
                FireEffectArrayInit(&lbl1DataC58[0], 10);
                ArenaCourseSetup(3);
                ArenaCameraTargetSet(270.0f, 0.0f);
            }
            else {
                ArenaCameraMotionSequenceUpdate(mainObject, -1);
            }
            break;
        case 0x406:
            ArenaCameraAndMinigameSequenceUpdate(mainObject, 270.0f, 2 * REFRESH_RATE);
            break;
        case 0x407:
            lbl1Bss128 += 1.0f;
            ArenaCameraTargetSet(270.0f, 0.0f);
            CameraPoseInterpolate(1.0f, &lbl1Data470);
            break;
        case 0x408:
            lbl1Bss128 += 1.0f;
            if (90.0f < lbl1Bss128) {
                mainObject->work[0] += 1;
                lbl1Bss128 = 0.0f;
            }
            CameraPoseSet(&lbl1Bss324, &lbl1Data470, &lbl1Bss30C);
            lbl1Bss20 = -1;
            break;
        case 0x409:
            if (lbl1Bss20 < 0) {
                lbl1Bss20 = HuAudFXPlay(0x7B1);
            }
            omVibrate(lbl1Bss120, 0xC, 6, 6);
            lbl1Bss128 += 1.0f;
            Hu3DModelPosSet(lbl1Bss1A0->mdlId[17], 0.0f, -(2.0f * lbl1Bss128), 0.0f);
            if (2 * REFRESH_RATE < lbl1Bss128) {
                mainObject->work[0] += 1;
                MessageProcessStart(ArenaPlayerScoreDialogue, 4);
                HuAudFXPlay(0x7BB);
            }
            break;
        case 0x40B:
            break;
        case 0x40C:
            if (lbl1Bss2FE < 0) {
                lbl1Bss128 = 0.0f;
                lbl1Bss2FE = MGSeqCreate(3, 0);
                ArenaCourseSetup(4);
                ArenaCameraTargetSet(270.0f, 0.0f);
            }
            else {
                ArenaCameraMotionSequenceUpdate(mainObject, -1);
            }
            break;
        case 0x40E:
            fadeAmount = lbl1Bss128 / (2 * REFRESH_RATE);
            if (1.0f < fadeAmount) {
                fadeAmount = 1.0f;
            }
            arenaLayout = &lbl1Data13B8;
            Hu3DModelAttrReset(lbl1Bss1A0->mdlId[16], HU3D_ATTR_DISPOFF);
            Hu3DModelRotSet(lbl1Bss1A0->mdlId[16], 0.0f, -90.0f, 0.0f);
            Hu3DModelPosSet(lbl1Bss1A0->mdlId[16], arenaLayout->modelPositions[8].x, arenaLayout->modelPositions[8].y, arenaLayout->modelPositions[8].z);
            Hu3DModelTPLvlSet(lbl1Bss1A0->mdlId[16], fadeAmount);
            ArenaCameraAndMinigameSequenceUpdate(mainObject, 270.0f, 2 * REFRESH_RATE);
            break;
        case 0x40F:
            lbl1Bss20 = -1;
            break;
        case 0x410:
            if (lbl1Bss20 < 0) {
                lbl1Bss20 = HuAudFXPlay(0x7B1);
            }
            omVibrate(lbl1Bss120, 0xC, 6, 6);
            lbl1Bss128 += 1.0f;
            if (120.0f <= lbl1Bss128) {
                lbl1Bss128 = 120.0f;
            }
            Hu3DModelAttrReset(lbl1Bss1A0->mdlId[18], HU3D_ATTR_DISPOFF);
            Hu3DModelPosSet(lbl1Bss1A0->mdlId[18], 0.0f, -(60.0f - (lbl1Bss128 / 2)), 0.0f);
            ArenaCameraTargetSet(0.0f, 0.0f);
            CameraLookAtTransition(lbl1Bss128 / 120.0f, &lbl1Data470);
            break;
        case 0x411:
            if (lbl1Bss20 >= 0) {
                lbl1Bss20 = -1;
                HuAudFXPlay(0x7BB);
            }
            lbl1Bss128 += 1.0f;
            ArenaPlayerCameraTargetSet();
            CameraPoseInterpolate(1.0f, &lbl1Data470);
            break;
        case 0x412:
            lbl1Bss128 += 1.0f;
            if (CameraPoseInterpolate(lbl1Bss128 / 120.0f, &lbl1Data470) != 0) {
                mainObject->work[0] += 1;
                CameraPoseSet(&lbl1Bss324, &lbl1Data470, &lbl1Bss30C);
                MessageProcessStart(ArenaFinalScoreDialogue, 5);
                Hu3DModelAttrSet(lbl1Bss1A0->mdlId[19], HU3D_ATTR_DISPOFF);
            }
            break;
        case 0x414:
            if (lbl1Bss2FE < 0) {
                lbl1Bss128 = 0.0f;
                lbl1Bss2FE = MGSeqCreate(3, 0);
                ArenaCourseSetup(5);
                ArenaCameraTargetSet(0.0f, 2000.0f);
                AudioSequencePlay(0x42);
            }
            else {
                ArenaCameraMotionSequenceUpdate(mainObject, -1);
            }
            break;
        case 0x416:
            lbl1Bss128 += 1.0f;
            break;
        case 0x417:
            ArenaCameraAndMinigameSequenceUpdate(mainObject, 0.0f, 2 * REFRESH_RATE);
            break;
        case 0x418:
            lbl1Bss128 += 1.0f;
            sequenceReady = 0;
            if (HuAudSStreamStatGet(lbl1Bss18) == 0) {
                sequenceReady = 1;
            }
            if ((sequenceReady != 0) || (1200.0f < lbl1Bss128)) {
                AudioSequenceFadeOut(0x64);
                lbl1Bss2FE = -1;
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                mainObject->objFunc = ArenaSceneExit;
            }
            break;
        default:
            OSReport("*** main mode error(%d)!!\n", mainObject->work[0]);
            break;
    }

    camera = Hu3DCamera;
    VECSubtract(&camera->target, &camera->pos, &cameraDirection);
    VECNormalize(&cameraDirection, &cameraDirection);
    HuAudFXListnerUpdate(&camera->pos, &cameraDirection);
    lbl1BssD8 = 0;
    lbl1BssCC.x = 0.0f;
    lbl1BssCC.y = 0.0f;
    lbl1BssCC.z = 0.0f;
}

void ObjectSetup(void)
{
    Vec shadowDirection;
    Vec cameraDirection;
    s32 objectIndex;
    s32 resourceSlot;
    OMOBJ *createdObject;
    s32 lightId;

    OSReport("******* M450 ObjectSetup *********\n");
    lbl1Bss33C = omInitObjMan(0x3E8, 0x2000);
    omGameSysInit(lbl1Bss33C);
    nMap = 0;
    nChar = 0;
    lbl1Bss120 = 0;
    lbl1Bss11C = -1;
    lbl1Bss118 = 0;
    lbl1Bss110 = 0;
    lbl1Bss10C = 0;
    lbl1Bss108 = 0;
    lbl1Bss308 = 0;
    lbl1Bss2FE = -1;
    lbl1Bss2FC = -1;
    lbl1Bss2F8 = -1;
    lbl1Bss1C = -1;
    lbl1Bss18 = -1;
    lbl1BssD8 = 0;
    lbl1BssF8 = -1;
    lbl1BssFC = NULL;
    lbl1BssC4 = -1;
    ArenaPlayerSetupStateUpdate();
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 30.0f, 20.0f, 30000.0f, 1.2f);
    CameraPoseSet(&lbl1Data464, &lbl1Data470, &lbl1Data47C);
    VECSubtract(&lbl1Data47C, &lbl1Data464, &cameraDirection);
    VECNormalize(&cameraDirection, &cameraDirection);
    HuAudFXListnerSetEX(&lbl1Data464, &cameraDirection, 5800.0, 1000.0, 100.0, 300.0, 300.0);
    lightId = Hu3DGLightCreateV(&lbl1Data4E8, &lbl1Data4F4, &lbl1Data500);
    Hu3DGLightStaticSet(lightId, 1);
    Hu3DGLightInfinitytSet(lightId);
    Hu3DShadowCreate(30.0f, 20.0f, 30000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    VECNormalize(&lbl1Data504, &shadowDirection);
    Hu3DShadowPosSet(&lbl1Data504, &shadowDirection, &lbl1Data510);
    MessageWindowInit(lbl1Bss120);
    lbl1Bss14 = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M450, 0x63), 0x10000000));
    HuSprAnimLock(lbl1Bss14);
    ScoreDisplayInit();
    for (resourceSlot = 0; resourceSlot < 0x20; resourceSlot++) {
        lbl1Bss1B0[resourceSlot].resourceId = 0;
        lbl1Bss1B0[resourceSlot].modelId = -1;
    }
    lbl1Bss1A0 = omAddObjEx(lbl1Bss33C, 0x64, 0x21, 0, -1, ArenaObjectManagerInit);
    for (objectIndex = 0; objectIndex < 0x17; objectIndex++) {
        lbl1Bss144[objectIndex] = omAddObjEx(lbl1Bss33C, 0x65, 1, 0, -1, ArenaLineObjectInit);
        lbl1Bss144[objectIndex]->work[0] = objectIndex;
    }
    omMakeGroupEx(lbl1Bss33C, 0, 1);
    lbl1Bss140 = omGetGroupMemberListEx(lbl1Bss33C, 0);
    for (objectIndex = 0; objectIndex < 1; objectIndex++) {
        createdObject = omAddObjEx(lbl1Bss33C, 0x66, 1, 0, 0, ArenaSparkleObjectInit);
    }
    omMakeGroupEx(lbl1Bss33C, 1, 0xA);
    lbl1Bss13C = omGetGroupMemberListEx(lbl1Bss33C, 1);
    for (objectIndex = 0; objectIndex < 0xA; objectIndex++) {
        createdObject = omAddObjEx(lbl1Bss33C, 0x67, 1, 0, 1, ArenaFloatingObjectInit);
    }
    PlayerLookupConfigure(NULL, 1505.0f);
    lbl1Bss138 = omAddObjEx(lbl1Bss33C, 0x6D, 1, 0x17, -1, ArenaPlayerCharacterInit);
    lbl1Bss134 = omAddObjEx(lbl1Bss33C, 0x68, 2, 0x13, -1, ArenaOpponentCharacterInit);
    omMakeGroupEx(lbl1Bss33C, 2, 2);
    lbl1Bss130 = omGetGroupMemberListEx(lbl1Bss33C, 2);
    for (objectIndex = 0; objectIndex < 2; objectIndex++) {
        createdObject = omAddObjEx(lbl1Bss33C, 0x69, 5, 0xF, 2, ArenaOpponentObjectInit);
        createdObject->work[0] = objectIndex;
    }
    registeredModelList.previous = NULL;
    registeredModelList.next = NULL;
    for (objectIndex = 0; objectIndex < 4; objectIndex++) {
        lbl1Bss2C[objectIndex] = Hu3DModelCreateFile(lbl1DataF5C[objectIndex]);
        Hu3DModelAttrSet(lbl1Bss2C[objectIndex], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(lbl1Bss2C[objectIndex], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    }
    lbl1Bss28 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M450, 0x6C));
    HuSprAnimLock(lbl1Bss28);
    lbl1Bss24 = Hu3DParManCreate(lbl1Bss28, 0x14, &particleParameters);
    Hu3DParManAttrSet(lbl1Bss24, 1);
    lbl1Bss1A4 = omAddObjEx(lbl1Bss33C, 0x6E, 0, 0, -1, ArenaMainStateUpdate);
    lbl1Bss1A4->work[0] = 0x3E9;
    lbl1Bss128 = 0.0f;
}

void ArenaSceneExit(OMOBJ *object)
{
    s32 playerIndex;

    if (WipeStatGet() == 0) {
        if (lbl1Bss2FE >= 0) {
            MGSeqKill(lbl1Bss2FE);
        }
        if (lbl1Bss2FC >= 0) {
            MGSeqKill(lbl1Bss2FC);
        }
        MGSeqKillAll();
        HuAudAllStop();
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            CharModelKill(lbl1Data0[GWPlayerCfg[playerIndex].character]);
        }
        omOvlReturnEx(1, 1);
    }
}
