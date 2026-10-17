#include "dolphin/mtx.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/flag.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "string.h"

#include "REL/m433Dll.h"

typedef struct M433FallingObjectState {
    /* 0x00 */ u8 modelId;
    /* 0x01 */ char reservedModelPadding01[2];
    /* 0x03 */ u8 motionState;
    /* 0x04 */ u8 objectState;
    /* 0x05 */ char reservedModelPadding05[3];
    /* 0x08 */ float movementSpeed;
    /* 0x0C */ Vec position;
    /* 0x18 */ Vec rotation;
    /* 0x24 */ Vec scale;
    /* 0x30 */ Vec spawnPosition;
    /* 0x3C */ char reservedMotionPadding3C[0xC];
    /* 0x48 */ Vec velocity;
    union {
        struct {
            /* 0x54 */ float angularVelocity;
            /* 0x58 */ float rotationAngle;
            char reservedRotationPadding5C[4];
            /* 0x60 */ float baseMovementSpeed;
        };
        struct {
            /* 0x54 */ s16 motionTimers[0x20];
        };
    };
} M433FallingObjectState; /* size = 0x94 */

typedef struct M433MapCameraTransitionState {
    char cameraTransitionState;
    u8 requestedCameraState;
    u8 previousCameraState;
    char reservedCameraFlag;
    u8 cameraTransitionFrame;
    char reservedCameraPadding08[4];
    Vec cameraPosition;
    Vec cameraRotation;
    char reservedCameraPadding24[0xC];
    Vec targetCameraPosition;
    Vec targetCameraRotation;
    char reservedCameraPadding48[0xC];
    s16 cameraTransitionDuration;
    char reservedCameraPadding56[0x3E];
} M433MapCameraTransitionState; /* size 0x94 */

typedef struct M433CameraManagerState {
    u8 cameraMode;
    Vec cameraPosition;
    Vec cameraRotation;
    Vec cameraStartPosition;
    Vec cameraStartRotation;
    Vec cameraEndPosition;
    Vec cameraEndRotation;
    char reservedCameraPadding4C[0xE4];
    float cameraZoom;
    float cameraStartZoom;
    float cameraEndZoom;
    char reservedCameraPadding13C[4];
    float cameraTransitionSpeed;
    float cameraTransitionDuration;
    float cameraTransitionRemaining;
    float cameraShakeAmplitude[3];
    float cameraShakeDamping[3];
    float cameraShakePhase[3];
    float cameraShakeSpeed[3];
    float cameraShakeOffset[3];
    u16 cameraShakeFrames[3];
} M433CameraManagerState; /* size 0x190 */

typedef struct M433MapParticleState {
    s16 particleModelId;
    s16 particleState;
    s16 particleLifetime;
    char reservedParticlePadding06[0x42];
} M433MapParticleState; /* size 0x48 */

void UpdateMapCameraTransition(OMOBJ *object);
void InitializeFallingMapObjects(OMOBJ *object);
void UpdateFallingMapObjects(OMOBJ *object);
void InitializeMapCamera(OMOBJ *object);
void UpdateMapCamera(OMOBJ *object);
s16 CreateMapParticleEmitter(s32 particleResourceId, s16 particleCount, HU3DPARTICLEHOOK particleUpdateCallback);
void UpdateMapParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void InitializeCameraManager(OMOBJ *object);
void UpdateCameraManager(OMOBJ *object);
void SetCameraTransition(float transitionDuration, Vec *targetPosition, Vec *targetRotation, float targetZoom);
void InitializeMapAllocationPool(void);
void *AllocateMapWork(s32 allocationSize);
void CleanupMapAllocationPool(void);
s32 GetRandomMapValue(void);
float ApproachFloat(float currentAngle, float targetAngle, float interpolationFactor);
void BuildRotationMatrix(Mtx rotationMatrix, float xRotation, float yRotation, float zRotation);
void RotateVector(Mtx rotationMatrix, Vec *rotation);
extern s32 useInitialMapMotion;

OMOBJ *cameraManagerObject;
OMOBJ *fallingObjectsObject;
OMOBJ *mapCameraObject;
void *mapWorkAllocations[0x20];
M433FallingObjectState fallingObjectStates[0xC];

void InitializeMapObjects(HUPROCESS *process)
{
    s32 randomEffectIndex;
    s32 initialEffectCount;

    useInitialMapMotion = 1;
    fallingObjectsObject = omAddObjEx(process, 0x1E, 8, 0, -1, InitializeFallingMapObjects);
    mapCameraObject = omAddObjEx(process, 0x1F, 3, 6, -1, InitializeMapCamera);
    cameraManagerObject = omAddObjEx(process, 0x1D, 1, 0, -1, InitializeCameraManager);
    initialEffectCount = frand() & 0x1F;
    for (randomEffectIndex = 0; randomEffectIndex < initialEffectCount; randomEffectIndex++) {
        GetRandomMapValue();
    }
    InitializeMapAllocationPool();
}

void CleanupMapObjects(void)
{
    CleanupMapAllocationPool();
}

OMOBJFUNC CreateMapCameraCallback(HUPROCESS *process, OMOBJ *object)
{
    M433DllWork *work = object->data;
    CRot.x = 54.3f;
    CRot.y = 22.3f;
    CRot.z = 0.0f;
    Center.x = -130.1f;
    Center.y = 885.0f;
    Center.z = -900.3f;
    CZoom = 1100.0f;
    SetCameraTransition(0.0f, &Center, &CRot, CZoom);
    object->work[0] = 0xB4;
    work->roundResultState = 1;
    return UpdateMapCameraTransition;
}

void UpdateMapCameraTransition(OMOBJ *object)
{
    Vec cameraPosition;
    Vec cameraRotation;
    float cameraZoom;
    M433DllWork *work = object->data;

    UpdateMinigameFrameCounters(object);
    if (object->work[0] == 120.0f) {
        cameraPosition.x = 0.0f;
        cameraPosition.y = -10.0f;
        cameraPosition.z = -207.5f;
        cameraRotation.x = -26.5f;
        cameraRotation.y = 0.0f;
        cameraRotation.z = 0.0f;
        cameraZoom = 1400.0f;
        SetCameraTransition(90.0f, &cameraPosition, &cameraRotation, cameraZoom);
    }
    if (object->work[0] == 48.0f) {
        cameraPosition.x = 0.0f;
        cameraPosition.y = 200.0f;
        cameraPosition.z = 0.0f;
        cameraRotation.x = -30.0f;
        cameraRotation.y = 0.0f;
        cameraRotation.z = 0.0f;
        cameraZoom = 1900.0f;
        SetCameraTransition(object->work[0], &cameraPosition, &cameraRotation, cameraZoom);
    }
    if (--object->work[0] == 0) {
        work->roundResultState = 2;
        AdvanceMinigameState(object);
        cameraManagerObject->objFunc = NULL;
    }
}

void InitializeFallingMapObjects(OMOBJ *object)
{
    Vec objectSpawnPositions[0x10] = {
        { 475.0f, 800.0f, -1400.0f },
        { -300.0f, 700.0f, -1400.0f },
        { 300.0f, 875.0f, -1450.0f },
        { -575.0f, 850.0f, -1550.0f },
        { 350.0f, 750.0f, -1500.0f },
        { -450.0f, 775.0f, -1550.0f },
        { 325.0f, 725.0f, -1450.0f },
        { -525.0f, 725.0f, -1500.0f },
        { 575.0f, 850.0f, -1550.0f },
        { -350.0f, 750.0f, -1500.0f },
        { 450.0f, 775.0f, -1550.0f },
        { -325.0f, 725.0f, -1450.0f },
        { 525.0f, 725.0f, -1500.0f },
        { -475.0f, 800.0f, -1400.0f },
        { 300.0f, 700.0f, -1400.0f },
        { -300.0f, 875.0f, -1450.0f },
    };
    M433FallingObjectState *fallingObjectState;
    s32 modelId;
    M433MapParticleState *particleState;
    s32 i;
    s32 spawnIndex;

    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 0xCD));
    object->mdlId[0] = modelId;
    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 0xCE));
    object->mdlId[1] = modelId;
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DMotionSpeedSet(modelId, 0.5f);
    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 0xCF));
    object->mdlId[2] = modelId;
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    modelId = CreateMapParticleEmitter(DATA_MAKE_NUM(DATADIR_M425, 0x05), 1, UpdateMapParticles);
    object->mdlId[3] = modelId;
    Hu3DParticleBlendModeSet(modelId, 1);
    Hu3DModelLayerSet(modelId, 6);
    particleState = ((HU3DPARTICLE *)Hu3DData[modelId].hookData)->work;
    particleState->particleState = 0;
    HuDataDirClose(DATA_MAKE_NUM(DATADIR_M425, 0x05));
    modelId = CreateMapParticleEmitter(DATA_MAKE_NUM(DATADIR_M433, 0xD3), 0xA, UpdateMapParticles);
    object->mdlId[4] = modelId;
    Hu3DParticleBlendModeSet(modelId, 1);
    Hu3DModelLayerSet(modelId, 6);
    particleState = ((HU3DPARTICLE *)Hu3DData[modelId].hookData)->work;
    particleState->particleState = 1;
    modelId = CreateMapParticleEmitter(DATA_MAKE_NUM(DATADIR_M433, 0xD2), 4, UpdateMapParticles);
    object->mdlId[5] = modelId;
    Hu3DParticleBlendModeSet(modelId, 1);
    Hu3DModelLayerSet(modelId, 6);
    particleState = ((HU3DPARTICLE *)Hu3DData[modelId].hookData)->work;
    particleState->particleState = 2;
    modelId = CreateMapParticleEmitter(DATA_MAKE_NUM(DATADIR_M433, 0xD1), 6, UpdateMapParticles);
    object->mdlId[6] = modelId;
    Hu3DParticleBlendModeSet(modelId, 1);
    Hu3DModelLayerSet(modelId, 6);
    particleState = ((HU3DPARTICLE *)Hu3DData[modelId].hookData)->work;
    particleState->particleState = 3;
    for (i = 0; i < 0xC; i++) {
        fallingObjectState = &fallingObjectStates[i];
        modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M425, 0x02));
        fallingObjectState->modelId = modelId;
        fallingObjectState->position.x = objectSpawnPositions[i].x;
        fallingObjectState->position.y = objectSpawnPositions[i].y - 300.0f;
        fallingObjectState->position.z = 800.0f + objectSpawnPositions[i].z;
        Hu3DModelPosSet(modelId, fallingObjectState->position.x, fallingObjectState->position.y, fallingObjectState->position.z);
        Hu3DMotionSpeedSet(modelId, 6.0f);
        fallingObjectState->motionState = fallingObjectState->objectState = 0;
        fallingObjectState->rotation.x = fallingObjectState->rotation.y = fallingObjectState->rotation.z = 0.0f;
        fallingObjectState->scale.x = fallingObjectState->scale.y = fallingObjectState->scale.z = 1.0f;
        fallingObjectState->velocity.x = fallingObjectState->velocity.y = fallingObjectState->velocity.z = 0.0f;
        fallingObjectState->spawnPosition.x = fallingObjectState->position.x;
        fallingObjectState->spawnPosition.y = fallingObjectState->position.y;
        fallingObjectState->spawnPosition.z = fallingObjectState->position.z;
        if ((i & 1) != 0) {
            fallingObjectState->rotation.y = 90.0f;
            fallingObjectState->position.y += 650.0;
            fallingObjectState->position.x -= 1000.0 + (200.0f * ((GetRandomMapValue() - 0x8000) / 32768.0f));
        }
        else {
            fallingObjectState->rotation.y = 90.0f;
            fallingObjectState->position.x += 200.0f * ((GetRandomMapValue() - 0x8000) / 32768.0f);
        }
        fallingObjectState->rotation.y += 10.0f * ((GetRandomMapValue() - 0x8000) / 32768.0f);
        fallingObjectState->position.z = fallingObjectState->position.z - 400.0;
        fallingObjectState->movementSpeed = 100.0f * (0.1f + (0.02f * (GetRandomMapValue() / 65536.0f)));
        for (spawnIndex = 0; spawnIndex < 0x20; spawnIndex++) {
            fallingObjectState->motionTimers[spawnIndex] = 0;
        }
        fallingObjectState->baseMovementSpeed = fallingObjectState->movementSpeed;
    }
    HuDataDirClose(DATA_MAKE_NUM(DATADIR_M425, 0x02));
    HuAudFXPlayVol(0x6A3, 0x41);
    object->objFunc = UpdateFallingMapObjects;
}

void UpdateFallingMapObjects(OMOBJ *object)
{
    Mtx rotationMatrix;
    Vec rotation;
    M433FallingObjectState *fallingObjectState;
    s32 i;

    for (i = 0; i < 0xC; i++) {
        fallingObjectState = &fallingObjectStates[i];
        switch (fallingObjectState->objectState) {
            case 0:
                if (fallingObjectState->movementSpeed < fallingObjectState->baseMovementSpeed) {
                    fallingObjectState->movementSpeed += 0.4f;
                }
                if ((s32)(64.0f * (GetRandomMapValue() / 65536.0f)) == 0) {
                    Hu3DMotionTimeSet(fallingObjectState->modelId, 0.0f);
                }
                if (Hu3DMotionEndCheck(fallingObjectState->modelId) != 0) {
                    fallingObjectState->velocity.y = -3.8933334f;
                }
                else {
                    fallingObjectState->velocity.y = -1.2166667f;
                }
                fallingObjectState->angularVelocity += 0.1f * ((GetRandomMapValue() - 0x8000) / 32768.0f);
                fallingObjectState->rotationAngle += fallingObjectState->angularVelocity;
                if (fallingObjectState->rotationAngle > 180.0f) {
                    fallingObjectState->rotationAngle -= 180.0f;
                }
                if (fallingObjectState->rotationAngle < -180.0f) {
                    fallingObjectState->rotationAngle += 180.0f;
                }
                fallingObjectState->rotation.x = -30.0 * sind(fallingObjectState->rotationAngle) * sind(fallingObjectState->rotation.y);
                fallingObjectState->rotation.z = -30.0 * sind(fallingObjectState->rotationAngle) * cosd(fallingObjectState->rotation.y);
                if (fallingObjectState->position.y <= 500.0f || fabs(fallingObjectState->position.x) > 1600.0) {
                    fallingObjectState->velocity.y = 0.0f;
                    fallingObjectState->motionTimers[6] = 0xB4;
                    fallingObjectState->movementSpeed *= 0.75f;
                    fallingObjectState->objectState++;
                }
                break;
            case 1:
                fallingObjectState->rotation.y += 1.0f;
                fallingObjectState->rotation.x = 0.0f;
                fallingObjectState->rotation.z = 60.0 * sind(fallingObjectState->motionTimers[6]) * cosd(fallingObjectState->rotation.y);
                if (--fallingObjectState->motionTimers[6] == 0) {
                    Hu3DModelAttrSet(fallingObjectState->modelId, HU3D_MOTATTR_LOOP);
                    Hu3DMotionSpeedSet(fallingObjectState->modelId, 6.0f);
                    fallingObjectState->velocity.y = 4.866667f;
                    fallingObjectState->objectState++;
                }
                break;
            case 2:
                if (fallingObjectState->movementSpeed > 4.0f) {
                    fallingObjectState->movementSpeed -= 0.1f;
                    fallingObjectState->velocity.y += 0.73f;
                }
                fallingObjectState->velocity.y += -0.5475f;
                if (fallingObjectState->velocity.y < 0.0f) {
                    Hu3DModelAttrReset(fallingObjectState->modelId, HU3D_MOTATTR_LOOP);
                    Hu3DMotionSpeedSet(fallingObjectState->modelId, 2.0f);
                    fallingObjectState->baseMovementSpeed = 100.0f * (0.1f + (0.02f * (GetRandomMapValue() / 65536.0f)));
                    fallingObjectState->objectState = 0;
                }
                break;
        }
        fallingObjectState->velocity.x = fallingObjectState->movementSpeed * sind(fallingObjectState->rotation.y);
        fallingObjectState->velocity.z = fallingObjectState->movementSpeed * cosd(fallingObjectState->rotation.y);
        fallingObjectState->position.x += fallingObjectState->velocity.x;
        fallingObjectState->position.y += fallingObjectState->velocity.y;
        fallingObjectState->position.z += fallingObjectState->velocity.z;
        if (fallingObjectState->position.z >= -500.0f) {
            fallingObjectState->position.z = -500.0f;
        }
        Hu3DModelPosSet(fallingObjectState->modelId, fallingObjectState->position.x, fallingObjectState->position.y, fallingObjectState->position.z);
        BuildRotationMatrix(rotationMatrix, fallingObjectState->rotation.x, -fallingObjectState->rotation.y, fallingObjectState->rotation.z);
        RotateVector(rotationMatrix, &rotation);
        Hu3DModelRotSetV(fallingObjectState->modelId, &rotation);
    }
}

void InitializeMapCamera(OMOBJ *object)
{
    s32 mapState;
    s32 currentMotion;

    s32 mapMotionIds[4] = { 0x004000D4, 0x004000D5, 0x004000D6, 0x004000D7 };
    s32 mapModelIds[4] = { 0x0040000C, 0x00400010, 0x0040000F, 0x0040000E };
    M433MapCameraTransitionState *work = AllocateMapWork(sizeof(M433MapCameraTransitionState));
    object->data = work;
    if (useInitialMapMotion != 0) {
        currentMotion = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 0x0B));
    }
    else {
        currentMotion = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 0x62));
    }
    object->mdlId[0] = currentMotion;
    for (mapState = 0; mapState < 4; mapState++) {
        if (useInitialMapMotion != 0) {
            object->mtnId[mapState] = Hu3DJointMotionFile(currentMotion, mapModelIds[mapState]);
        }
        else {
            object->mtnId[mapState] = Hu3DJointMotionFile(currentMotion, mapMotionIds[mapState]);
        }
    }
    Hu3DMotionSet(currentMotion, object->mtnId[0]);
    Hu3DModelAttrSet(currentMotion, HU3D_MOTATTR_LOOP);
    work->requestedCameraState = work->previousCameraState = 0;
    work->cameraTransitionFrame = 0;
    work->cameraTransitionDuration = 0;
    work->cameraPosition.x = 0.0f;
    work->cameraPosition.y = 180.0f;
    work->cameraPosition.z = -490.0f;
    work->targetCameraPosition = work->cameraPosition;
    work->cameraRotation.x = work->cameraRotation.y = work->cameraRotation.z = 0.0f;
    work->targetCameraRotation = work->cameraRotation;
    omSetTra(object, work->cameraPosition.x, work->cameraPosition.y, work->cameraPosition.z);
    omSetRot(object, work->cameraRotation.x, work->cameraRotation.y, work->cameraRotation.z);
    object->objFunc = UpdateMapCamera;
}

void UpdateMapCamera(OMOBJ *object)
{
    M433MapCameraTransitionState *work;
    s32 particleState;
    s32 particleIndex;

    work = object->data;
    particleIndex = object->mdlId[0];
    switch (work->cameraTransitionFrame) {
        case 0:
            work->requestedCameraState = 0;
            particleState = 0x40000001;
            break;
        case 1:
            work->targetCameraRotation.y = 0.0f;
            work->requestedCameraState = 1;
            particleState = 0x40000001;
            if (work->cameraTransitionDuration++ >= 0x5A) {
                work->cameraTransitionDuration = 0;
                work->cameraTransitionFrame = 0;
            }
            break;
        case 2:
            work->targetCameraRotation.y = 0.0f;
            work->requestedCameraState = 2;
            particleState = 0;
            if (Hu3DMotionEndCheck(particleIndex) != 0) {
                work->cameraTransitionFrame = 0;
            }
            break;
        case 3:
            work->targetCameraRotation.y = 0.0f;
            work->requestedCameraState = 3;
            particleState = 0x40000001;
            break;
        case 4:
            work->targetCameraRotation.y = 0.0f;
            work->cameraTransitionFrame = 0;
            break;
        case 5:
            work->targetCameraRotation.y = 50.0f;
            work->cameraTransitionFrame = 0;
            break;
        case 6:
            work->targetCameraRotation.y = -50.0f;
            work->cameraTransitionFrame = 0;
            break;
    }
    if (work->requestedCameraState != work->previousCameraState) {
        work->previousCameraState = work->requestedCameraState;
        Hu3DMotionShiftSet(particleIndex, object->mtnId[work->requestedCameraState], 0.0f, 8.0f, particleState);
        switch (work->requestedCameraState) {
            case 1:
                HuAudFXPlay(0x43);
                break;
            case 2:
                HuAudFXPlay(0x45);
                break;
        }
    }
    work->cameraRotation.y = ApproachFloat(work->cameraRotation.y, work->targetCameraRotation.y, 0.9f);
    omSetTra(object, work->cameraPosition.x, work->cameraPosition.y, work->cameraPosition.z);
    omSetRot(object, work->cameraRotation.x, work->cameraRotation.y, work->cameraRotation.z);
}

s16 CreateMapParticleEmitter(s32 resourceId, s16 particleCount, HU3DPARTICLEHOOK particleUpdateCallback)
{
    M433MapParticleState *particleState;
    s32 particleModelId;
    HU3DPARTICLE *particleData;
    ANIMDATA *particleAnimation;

    particleAnimation = HuSprAnimRead(HuDataReadNum(resourceId, HU_MEMNUM_OVL));
    particleModelId = Hu3DParticleCreate(particleAnimation, particleCount);
    Hu3DParticleHookSet(particleModelId, particleUpdateCallback);
    particleData = Hu3DData[particleModelId].hookData;
    particleState = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M433MapParticleState), HU_MEMNUM_OVL);
    particleData->work = particleState;
    particleState->particleModelId = particleModelId;
    particleState->particleLifetime = 0;
    return particleModelId;
}

void UpdateMapParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float directionLength;
    float particleDistance;
    float normalizedParticleDistance;
    float distancePerParticle;
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    float particleScaleByType[4] = { 1000.0f, 200.0f, 600.0f, 1200.0f };
    float particleOpacityByType[4] = { 1.0f, 0.6f, 0.3f, 0.425f };
    Vec particlePosition;
    Vec particleVelocity;
    Vec particleAcceleration;
    Vec normalizedDirection;
    s16 unusedParticleValue = 0;
    M433MapParticleState *particleState = particle->work;
    switch (particleState->particleLifetime) {
        case 0:
            particle->dataCnt = 0x54;
            particleData = particle->data;
            if (particleState->particleState != 0) {
                normalizedDirection.x = 0.0f;
                normalizedDirection.y = -4700.0f;
                normalizedDirection.z = 4450.0f;
                directionLength = 1.0f / sqrtf((normalizedDirection.z * normalizedDirection.z) + ((normalizedDirection.x * normalizedDirection.x) + (normalizedDirection.y * normalizedDirection.y)));
                normalizedDirection.x *= directionLength;
                normalizedDirection.y *= directionLength;
                normalizedDirection.z *= directionLength;
                distancePerParticle = 4000.0f / particle->maxCnt;
                for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                    particleDistance = 500.0f + ((distancePerParticle * particleIndex) + (0.25f * distancePerParticle * (GetRandomMapValue() / 65536.0f)));
                    normalizedParticleDistance = particleDistance / 4000.0f;
                    particleData->accel.x = particleDistance;
                    particleData->pos.x = particleDistance * normalizedDirection.x;
                    particleData->pos.y = 5500.0f + (particleDistance * normalizedDirection.y);
                    particleData->pos.z = -3950.0f + (particleDistance * normalizedDirection.z);
                    particleData->scale = (0.3f + (0.7f * (normalizedParticleDistance * normalizedParticleDistance))) * (particleScaleByType[particleState->particleState] * (0.7f + (0.3f * (GetRandomMapValue() / 65536.0f))));
                    particleData->color.r = (s32)(160.0f * (GetRandomMapValue() / 65536.0f)) + 0x20;
                    particleData->color.g = (s32)(96.0f * (GetRandomMapValue() / 65536.0f)) + 0x20;
                    particleData->color.b = (s32)(128.0f * (GetRandomMapValue() / 65536.0f)) + 0x20;
                    particleData->color.a = ((1.0 - (0.4 * normalizedParticleDistance)) * (255.0f * particleOpacityByType[particleState->particleState])) - (s32)(16.0f * (GetRandomMapValue() / 65536.0f));
                }
                particleState->particleLifetime++;
            }
            else {
                particleData->pos.x = 0.0f;
                particleData->pos.y = 5500.0f;
                particleData->pos.z = -3950.0f;
                particleData->scale = 1000.0f;
            }
            break;
        case 1:
            particlePosition.x = sind(CRot.y) * cosd(CRot.x);
            particlePosition.y = -sind(CRot.x);
            particlePosition.z = cosd(CRot.y) * cosd(CRot.x);
            particleVelocity.x = -(Center.x + (1.25f * (particlePosition.x * CZoom)));
            particleVelocity.y = 5500.0f - (Center.y + (particlePosition.y * CZoom));
            particleVelocity.z = -3950.0f - (Center.z + (1.25f * (particlePosition.z * CZoom)));
            directionLength = 1.0f / sqrtf((particleVelocity.z * particleVelocity.z) + ((particleVelocity.x * particleVelocity.x) + (particleVelocity.y * particleVelocity.y)));
            particleVelocity.x *= directionLength;
            particleVelocity.y *= directionLength;
            particleVelocity.z *= directionLength;
            directionLength = -((-particleVelocity.z * -particlePosition.z) + ((-particleVelocity.x * -particlePosition.x) + (-particleVelocity.y * -particlePosition.y)));
            particleAcceleration.x = -particlePosition.x + (-2.0f * particleVelocity.x * directionLength);
            particleAcceleration.y = -particlePosition.y + (-2.0f * particleVelocity.y * directionLength);
            particleAcceleration.z = -particlePosition.z + (-2.0f * particleVelocity.z * directionLength);
            for (particleData = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->pos.x = particleAcceleration.x * particleData->accel.x;
                particleData->pos.y = 5500.0f + (particleAcceleration.y * particleData->accel.x);
                particleData->pos.z = -3950.0f + (particleAcceleration.z * particleData->accel.x);
                if (particle->dataCnt < 0xA) {
                    particleData->color.a *= 0.8f;
                }
            }
            particle->dataCnt--;
            if (particle->dataCnt == 0) {
                Hu3DModelAttrReset(particleState->particleModelId, HU3D_ATTR_PARTICLE_KILL);
                Hu3DModelKill(particleState->particleModelId);
                return;
            }
            break;
    }
    DCFlushRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void InitializeCameraManager(OMOBJ *object)
{
    M433CameraManagerState *work = AllocateMapWork(sizeof(M433CameraManagerState));
    object->data = work;
    work->cameraTransitionRemaining = work->cameraTransitionDuration = 0.0f;
    work->cameraTransitionSpeed = 1.0f;
    work->cameraRotation.x = work->cameraStartRotation.x = -30.0f;
    work->cameraRotation.y = work->cameraStartRotation.y = 0.0f;
    work->cameraRotation.z = work->cameraStartRotation.z = 0.0f;
    work->cameraPosition.x = work->cameraStartPosition.x = 0.0f;
    work->cameraPosition.y = work->cameraStartPosition.y = 200.0f;
    work->cameraPosition.z = work->cameraStartPosition.z = 0.0f;
    work->cameraZoom = work->cameraStartZoom = 1900.0f;
    work->cameraShakeFrames[0] = work->cameraShakeFrames[1] = work->cameraShakeFrames[2] = 0;
    object->objFunc = UpdateCameraManager;
}

void UpdateCameraManager(OMOBJ *object)
{
    float transitionProgress;
    s32 cameraAxis;
    M433CameraManagerState *work = object->data;
    if (work->cameraTransitionRemaining > 0.0f) {
        work->cameraTransitionRemaining = work->cameraTransitionRemaining - work->cameraTransitionSpeed;
        transitionProgress = (work->cameraTransitionDuration - work->cameraTransitionRemaining) / work->cameraTransitionDuration;
        transitionProgress = sind((90.0f * transitionProgress));
        if (work->cameraMode == 0) {
            work->cameraPosition.x = work->cameraStartPosition.x + (transitionProgress * (work->cameraEndPosition.x - work->cameraStartPosition.x));
            work->cameraPosition.y = work->cameraStartPosition.y + (transitionProgress * (work->cameraEndPosition.y - work->cameraStartPosition.y));
            work->cameraPosition.z = work->cameraStartPosition.z + (transitionProgress * (work->cameraEndPosition.z - work->cameraStartPosition.z));
            work->cameraRotation.x = work->cameraStartRotation.x + (transitionProgress * (work->cameraEndRotation.x - work->cameraStartRotation.x));
            work->cameraRotation.y = work->cameraStartRotation.y + (transitionProgress * (work->cameraEndRotation.y - work->cameraStartRotation.y));
            work->cameraRotation.z = work->cameraStartRotation.z + (transitionProgress * (work->cameraEndRotation.z - work->cameraStartRotation.z));
            work->cameraZoom = work->cameraStartZoom + (transitionProgress * (work->cameraEndZoom - work->cameraStartZoom));
        }
    }
    CRot.x = work->cameraRotation.x;
    CRot.y = work->cameraRotation.y;
    CRot.z = work->cameraRotation.z;
    Center.x = work->cameraPosition.x;
    Center.y = work->cameraPosition.y;
    Center.z = work->cameraPosition.z;
    CZoom = work->cameraZoom;
    for (cameraAxis = 0; cameraAxis < 3; cameraAxis++) {
        work->cameraShakeOffset[cameraAxis] = 0.0f;
        if (work->cameraShakeFrames[cameraAxis] != 0) {
            work->cameraShakeOffset[cameraAxis] = work->cameraShakeAmplitude[cameraAxis] * sind(work->cameraShakePhase[cameraAxis]);
            work->cameraShakePhase[cameraAxis] += work->cameraShakeSpeed[cameraAxis];
            work->cameraShakeAmplitude[cameraAxis] *= work->cameraShakeDamping[cameraAxis];
            work->cameraShakeFrames[cameraAxis] -= 1;
        }
    }
    Center.x += work->cameraShakeOffset[0];
    Center.y += work->cameraShakeOffset[1];
    Center.z += work->cameraShakeOffset[2];
}

void SetCameraTransition(float transitionDuration, Vec *targetPosition, Vec *targetRotation, float targetZoom)
{
    M433CameraManagerState *work = cameraManagerObject->data;
    work->cameraMode = 0;
    work->cameraTransitionDuration = transitionDuration;
    work->cameraTransitionRemaining = transitionDuration;
    if (transitionDuration == 0.0f) {
        work->cameraPosition.x = targetPosition->x;
        work->cameraPosition.y = targetPosition->y;
        work->cameraPosition.z = targetPosition->z;
        work->cameraRotation.x = targetRotation->x;
        work->cameraRotation.y = targetRotation->y;
        work->cameraRotation.z = targetRotation->z;
        work->cameraZoom = targetZoom;
        return;
    }
    work->cameraStartPosition = work->cameraPosition;
    work->cameraEndPosition.x = targetPosition->x;
    work->cameraEndPosition.y = targetPosition->y;
    work->cameraEndPosition.z = targetPosition->z;
    work->cameraStartRotation = work->cameraRotation;
    work->cameraEndRotation.x = targetRotation->x;
    work->cameraEndRotation.y = targetRotation->y;
    work->cameraEndRotation.z = targetRotation->z;
    work->cameraStartZoom = work->cameraZoom;
    work->cameraEndZoom = targetZoom;
}

void SetCameraTransitionSpeed(float transitionSpeed)
{
    M433CameraManagerState *cameraState = cameraManagerObject->data;
    cameraState->cameraTransitionSpeed = transitionSpeed;
}

void SetCameraShake(u8 shakeIndex, u16 shakeDuration, float shakeAmplitude, float shakeDamping, float shakeSpeed)
{
    M433CameraManagerState *cameraState = cameraManagerObject->data;
    cameraState->cameraShakeFrames[shakeIndex] = shakeDuration;
    cameraState->cameraShakeAmplitude[shakeIndex] = shakeAmplitude;
    cameraState->cameraShakeDamping[shakeIndex] = shakeDamping;
    cameraState->cameraShakeSpeed[shakeIndex] = shakeSpeed;
    cameraState->cameraShakePhase[shakeIndex] = 90.0f;
}

void InitializeMapAllocationPool(void)
{
    s32 i;
    for (i = 0; i < 0x20; i++) {
        mapWorkAllocations[i] = NULL;
    }
}

void *AllocateMapWork(s32 allocationSize)
{
    s32 i;

    for (i = 0; i < 0x20; i++) {
        if (!mapWorkAllocations[i]) {
            break;
        }
    }
    if (i == 0x20) {
        return NULL;
    }
    mapWorkAllocations[i] = HuMemDirectMallocNum(HEAP_HEAP, allocationSize, HU_MEMNUM_OVL);
    return mapWorkAllocations[i];
}

void FreeMapWork(u32 allocationAddress)
{
    s32 i;

    for (i = 0; i < 0x20; i++) {
        if (mapWorkAllocations[i] == (void *)allocationAddress) {
            break;
        }
    }
    if (i != 0x20) {
        HuMemDirectFree(mapWorkAllocations[i]);
        mapWorkAllocations[i] = NULL;
    }
}

void CleanupMapAllocationPool(void)
{
    s32 i;

    for (i = 0; i < 0x20; i++) {
        if (mapWorkAllocations[i]) {
            HuMemDirectFree(mapWorkAllocations[i]);
            mapWorkAllocations[i] = NULL;
        }
    }
}

u32 randomNumberSeed = 0x41C64E6D;

s32 GetRandomMapValue(void)
{
    randomNumberSeed *= 0x41C64E6D;
    randomNumberSeed += 0x3039;
    return randomNumberSeed >> 0x10;
}

float ApproachFloat(float currentAngle, float targetAngle, float interpolationFactor)
{
    float angleDifference;

    if (currentAngle > 180.0f) {
        currentAngle -= 360.0f;
    }
    else if (currentAngle <= -180.0f) {
        currentAngle += 360.0f;
    }
    if (targetAngle > 180.0f) {
        targetAngle -= 360.0f;
    }
    else if (targetAngle <= -180.0f) {
        targetAngle += 360.0f;
    }
    angleDifference = currentAngle - targetAngle;
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    else if (angleDifference <= -180.0f) {
        angleDifference += 360.0f;
    }
    currentAngle = targetAngle + (angleDifference * interpolationFactor);
    if (currentAngle > 180.0f) {
        return currentAngle - 360.0f;
    }
    if (currentAngle <= -180.0f) {
        currentAngle += 360.0f;
    }
    return currentAngle;
}

void BuildRotationMatrix(Mtx rotationMatrix, float xRotation, float yRotation, float zRotation)
{
    Mtx xRotationMatrix;
    Mtx yRotationMatrix;

    if (zRotation != 0.0f) {
        MTXRotRad(rotationMatrix, 0x5A, MTXDegToRad(zRotation));
    }
    else {
        MTXIdentity(rotationMatrix);
    }
    if (xRotation != 0.0f) {
        MTXRotRad(xRotationMatrix, 0x58, MTXDegToRad(xRotation));
        MTXConcat(xRotationMatrix, rotationMatrix, rotationMatrix);
    }
    if (yRotation != 0.0f) {
        MTXRotRad(yRotationMatrix, 0x59, MTXDegToRad(yRotation));
        MTXConcat(yRotationMatrix, rotationMatrix, rotationMatrix);
    }
}

void RotateVector(Mtx rotationMatrix, Vec *rotation)
{
    float sinePitch;
    float cosinePitch;

    rotation->x = atan2d(rotationMatrix[1][2], rotationMatrix[2][2]);
    if (rotation->x < 0.0f) {
        rotation->x += 360.0f;
    }
    rotation->z = atan2d(rotationMatrix[0][1], rotationMatrix[0][0]);
    if (rotation->z < 0.0f) {
        rotation->z += 360.0f;
    }
    sinePitch = -rotationMatrix[0][2];
    cosinePitch = sqrtf((1.0 - (sinePitch * sinePitch)) < 0.0 ? (-(1.0 - (sinePitch * sinePitch))) : (1.0 - (sinePitch * sinePitch)));
    if ((rotation->x > 90.0f) && (rotation->x < 270.0f) && (rotation->z > 90.0f) && (rotation->z < 270.0f)) {
        rotation->x = fmod(180.0f + rotation->x, 360.0);
        rotation->z = fmod(180.0f + rotation->z, 360.0);
        cosinePitch = -cosinePitch;
    }
    rotation->y = atan2d(sinePitch, cosinePitch);
    if (rotation->y < 0.0f) {
        rotation->y += 360.0f;
    }
}

void SetMapMotion(s32 transitionFrame)
{
    M433MapCameraTransitionState *work = mapCameraObject->data;
    work->cameraTransitionFrame = transitionFrame;
}
