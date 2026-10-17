#include "game/audio.h"
#include "game/chrman.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/wipe.h"
#include "game/hu3d.h"
#include "game/sprite.h"

#include "REL/m408Dll.h"

#include "ext_math.h"
#include "math.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

OMOBJ *cameraTargetObject;
u32 cameraParticleEffectState;
Vec cameraParticlePosition;
Vec cameraParticleVelocity;
HU3DPARTICLEDATA *cameraParticleData[20];
u32 cameraParticleCount;
HU3DPARTICLEDATA *cameraParticleGroups[3];
s32 cameraParticleGroupCounts[3];
s32 cameraPrimaryParticleId;
s32 cameraSecondaryParticleId;

void InitializeCameraEffects(OMOBJ *object);
void InitializeCameraTarget(OMOBJ *object);

void CreateCameraObjects(HUPROCESS *objman)
{
    OMOBJ *object;
    cameraParticleEffectState = 0;
    object = omAddObjEx(objman, 20, 9, 0, -1, InitializeCameraEffects);
    object->work[0] = 0;
    cameraTargetObject = omAddObjEx(objman, 21, 2, 0, -1, InitializeCameraTarget);
    cameraTargetObject->work[0] = 0;
    cameraPrimaryParticleId = cameraSecondaryParticleId = -1;
}

void UpdateCameraEffects(OMOBJ *object);

void UpdateCameraParticleEmitter(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void SetCameraParticleGroup(s16 particleModelId, u32 particleGroupId);
void UpdateCameraFade(void);
void CleanupCameraEffects(void);

void InitializeCameraEffects(OMOBJ *object)
{
    s16 createdObjectId;
    s32 modelIndex;
    for (modelIndex = 0; modelIndex < 20; modelIndex++) {
        cameraParticleData[modelIndex] = NULL;
    }
    cameraParticleCount = 0;
    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
        cameraParticleGroups[modelIndex] = NULL;
        cameraParticleGroupCounts[modelIndex] = 0;
    }
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x00));
    omSetSca(object, 0.1f, 0.1f, 0.1f);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x01));
    Hu3DModelScaleSet(object->mdlId[1], 0.1f, 0.1f, 0.1f);
    Hu3DModelPosSet(object->mdlId[1], 0, 11990, 0);
    Hu3DModelShadowMapSet(object->mdlId[1]);
    createdObjectId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M408, 0x0B)), 1);
    object->mdlId[2] = createdObjectId;
    Hu3DModelLayerSet(createdObjectId, 6);
    Hu3DParticleColSet(createdObjectId, 255, 255, 255);
    Hu3DParticleHookSet(createdObjectId, UpdateCameraParticleEmitter);
    Hu3DModelPosSet(createdObjectId, 0, 0, 0);
    ((HU3DPARTICLE *)Hu3DData[createdObjectId].hookData)->dataCnt = 0;
    ((HU3DPARTICLE *)Hu3DData[createdObjectId].hookData)->emitCnt = 0;
    createdObjectId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M408, 0x0D)), 1);
    object->mdlId[3] = createdObjectId;
    Hu3DParticleBlendModeSet(createdObjectId, 1);
    Hu3DModelLayerSet(createdObjectId, 6);
    Hu3DParticleColSet(createdObjectId, 255, 255, 255);
    Hu3DParticleHookSet(createdObjectId, UpdateCameraParticleEmitter);
    Hu3DModelPosSet(createdObjectId, 0, 0, 0);
    ((HU3DPARTICLE *)Hu3DData[createdObjectId].hookData)->dataCnt = 0;
    ((HU3DPARTICLE *)Hu3DData[createdObjectId].hookData)->emitCnt = 1;
    createdObjectId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M408, 0x11)), 1);
    object->mdlId[5] = createdObjectId;
    Hu3DModelLayerSet(createdObjectId, 7);
    Hu3DParticleColSet(createdObjectId, 255, 255, 255);
    Hu3DParticleHookSet(createdObjectId, UpdateCameraParticleEmitter);
    Hu3DModelPosSet(createdObjectId, 0, 0, 0);
    ((HU3DPARTICLE *)Hu3DData[createdObjectId].hookData)->dataCnt = 0;
    ((HU3DPARTICLE *)Hu3DData[createdObjectId].hookData)->emitCnt = 2;
    createdObjectId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M408, 0x10)), 8);
    object->mdlId[6] = createdObjectId;
    Hu3DParticleBlendModeSet(createdObjectId, 1);
    Hu3DModelLayerSet(createdObjectId, 6);
    Hu3DModelPosSet(createdObjectId, 0, 0, 0);
    SetCameraParticleGroup(createdObjectId, 0);
    createdObjectId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M408, 0x0F)), 6);
    object->mdlId[6] = createdObjectId;
    Hu3DParticleBlendModeSet(createdObjectId, 1);
    Hu3DModelLayerSet(createdObjectId, 6);
    Hu3DModelPosSet(createdObjectId, 0, 0, 0);
    SetCameraParticleGroup(createdObjectId, 1);
    createdObjectId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M408, 0x0D)), 5);
    object->mdlId[6] = createdObjectId;
    Hu3DParticleBlendModeSet(createdObjectId, 1);
    Hu3DModelLayerSet(createdObjectId, 6);
    Hu3DModelPosSet(createdObjectId, 0, 0, 0);
    SetCameraParticleGroup(createdObjectId, 2);
    UpdateCameraFade();
    cameraParticleEffectState = 1;
    object->objFunc = UpdateCameraEffects;
    object->work[1] = 0;
}

void UpdateCameraEffectCleanup(OMOBJ *object);

void UpdateCameraEffects(OMOBJ *object)
{
    object->work[1]++;
    switch (cameraParticleEffectState) {
        case 1:
            if (cameraTargetObject->work[0] == 4) {
                cameraParticleEffectState = 2;
                GetCameraPositionRotation(&cameraParticlePosition, &cameraParticleVelocity);
                object->work[1] = 0;
            }
            break;

        case 2:
            GetCameraPositionRotation(&cameraParticlePosition, &cameraParticleVelocity);
            if (cameraSecondaryParticleId < 0) {
                cameraSecondaryParticleId = HuAudFXPlay(1361);
            }
            cameraParticleEffectState = 3;
            object->work[1] = 0;
            break;

        case 3:
#if VERSION_PAL
            cameraParticleVelocity.y += -400.0f / REFRESH_RATE_F;
            cameraParticleVelocity.z += -35.0f / REFRESH_RATE_F;
            cameraParticlePosition.y += 20.0f / 50.000005f;
#else
            cameraParticleVelocity.y += -400.0f / REFRESH_RATE_OFF_BY_1;
            cameraParticleVelocity.z += -35.0f / REFRESH_RATE_OFF_BY_1;
            cameraParticlePosition.y += 20.0f / 60.0f;
#endif
            GetCameraPositionRotation(&cameraParticlePosition, &cameraParticleVelocity);
            if (cameraParticleVelocity.y > 12010) {
                break;
            }
            if (cameraPrimaryParticleId < 0) {
                cameraPrimaryParticleId = HuAudFXPlay(1357);
            }
            if (cameraSecondaryParticleId >= 0) {
                HuAudFXStop(cameraSecondaryParticleId);
                cameraSecondaryParticleId = -1;
            }
            cameraParticleEffectState = 4;
            object->work[1] = 0;
            object->work[2] = 0;
            break;

        case 4:
#if VERSION_PAL
            if (object->work[1] < 20.0f) {
                cameraParticlePosition.y += 0.39999998f;
#else
            if (object->work[1] < 24.0f) {
                cameraParticlePosition.y += 4.0f / 12.0f;
#endif
                GetCameraPositionRotation(&cameraParticlePosition, &cameraParticleVelocity);
            }
            else {
                if (object->work[2] == 0) {
                    if (!minigameExitRequested) {
                        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
                        minigameExitAllowed = 1;
                    }
                    object->work[2]++;
                }
#if VERSION_PAL
                CZoom -= 3.0f;
#else
                CZoom -= 2.5000002f;
#endif
#if VERSION_PAL
                if (object->work[1] < 75.0f) {
#else
                if (object->work[1] < 90.0f) {
#endif
                    break;
                }
                cameraParticleEffectState = 5;
                object->work[1] = 0;
            }
            break;

        default:
            cameraParticleEffectState = 5;
            break;
    }
    omSetTra(object, Center.x + (sind(CRot.y) * cosd(CRot.x) * CZoom), Center.y + (-sind(CRot.x) * CZoom),
        Center.z + (cosd(CRot.y) * cosd(CRot.x) * CZoom));
    CleanupCameraEffects();
    if (cameraParticleEffectState == 5) {
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
        cameraParticleEffectState = 5;
        object->objFunc = UpdateCameraEffectCleanup;
    }
}

void UpdateCameraEffectCleanup(OMOBJ *object)
{
    if (!WipeStatGet() && object->work[2] != 0) {
        if (!minigameExitRequested) {
            WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
            minigameExitAllowed = 0;
        }
        object->work[2] = 0;
    }
    if (GetStageEventState() >= 4) {
        Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    }
    omSetTra(object, Center.x + (sind(CRot.y) * cosd(CRot.x) * CZoom), Center.y + (-sind(CRot.x) * CZoom) - 1000,
        Center.z + (cosd(CRot.y) * cosd(CRot.x) * CZoom));
}

typedef struct CameraTargetWork {
    Vec velocity;
    Vec cameraPosition;
    Vec cameraTarget;
    Vec cameraVelocity;
    Vec cameraOrbit;
    Vec cameraRotation;
    s32 soundEmitterIds[2];
} CameraTargetWork;

void UpdateCameraTarget(OMOBJ *object);
void UpdateCameraParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

void InitializeCameraTarget(OMOBJ *object)
{
    CameraTargetWork *cameratargetworkPointer;
    s32 createdObjectId;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(CameraTargetWork), HU_MEMNUM_OVL);
    cameratargetworkPointer = object->data;
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x0A));
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DMotionSpeedSet(object->mdlId[0], 3.0f);
    Hu3DModelShadowSet(object->mdlId[0]);
    object->objFunc = UpdateCameraTarget;
    object->work[0] = 0;
    object->work[1] = 0;
    cameratargetworkPointer->cameraRotation.x = cameratargetworkPointer->cameraRotation.y = cameratargetworkPointer->cameraRotation.z = 0;
    cameratargetworkPointer->velocity.x = cameratargetworkPointer->velocity.y = cameratargetworkPointer->velocity.z = 0;
    cameratargetworkPointer->velocity.z = 1000;
    omSetTra(object, 0, 12080, -2 * cameratargetworkPointer->velocity.z);
    omSetSca(object, 0.5f, 0.5f, 0.5f);
    createdObjectId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M408, 0x24)), 1024);
    object->mdlId[1] = createdObjectId;
    Hu3DModelLayerSet(createdObjectId, 6);
    Hu3DParticleColSet(createdObjectId, 255, 255, 255);
    Hu3DParticleHookSet(createdObjectId, UpdateCameraParticle);
    Hu3DModelPosSet(createdObjectId, 0, 0, 0);
    ((HU3DPARTICLE *)Hu3DData[createdObjectId].hookData)->dataCnt = 0;
    cameratargetworkPointer->cameraOrbit.y = -35;
    cameratargetworkPointer->cameraOrbit.z = 350;
    cameratargetworkPointer->cameraTarget.y = 800 + object->trans.y;
    cameratargetworkPointer->soundEmitterIds[0] = cameratargetworkPointer->soundEmitterIds[1] = -1;
    HuDataDirClose(DATA_MAKE_NUM(DATADIR_M408, 10));
}

void SetCameraTargetPosition(OMOBJ *object);
void SetCameraParticleEmission(s32 particleModelId, Vec *emissionPosition, u32 particleCount);

void UpdateCameraTarget(OMOBJ *object)
{
    float frameTimer;
    float frameTimer2;

    Vec positionVector;
    Vec positionVector2;
    Vec positionVector3;
    Vec positionVector4;

    CameraTargetWork *cameratargetworkPointer;
    cameratargetworkPointer = object->data;
    object->trans.x += cameratargetworkPointer->velocity.x * REFRESH_FREQ;
    object->trans.y += cameratargetworkPointer->velocity.y * REFRESH_FREQ;
    object->trans.z += cameratargetworkPointer->velocity.z * REFRESH_FREQ;
    object->work[1]++;
    switch (object->work[0]) {
        case 0:
            if (object->work[1] < REFRESH_RATE_F * 2) {
                frameTimer2 = REFRESH_FREQ * (1.5f * (REFRESH_RATE_F * 2 - object->work[1]));
                if (frameTimer2 > 1.0f) {
                    frameTimer2 = 1.0f;
                }
                frameTimer = cameratargetworkPointer->cameraTarget.z;
                cameratargetworkPointer->cameraOrbit.z -= (175.0f / REFRESH_RATE) * frameTimer2;
#if VERSION_PAL
                cameratargetworkPointer->cameraOrbit.y += (70.0f / REFRESH_RATE) * frameTimer2;
#else
                cameratargetworkPointer->cameraOrbit.y += (70.0f / REFRESH_RATE_OFF_BY_1) * frameTimer2;
#endif
                cameratargetworkPointer->cameraTarget.x = object->trans.x + (cameratargetworkPointer->cameraOrbit.z * sind(cameratargetworkPointer->cameraOrbit.y));
                cameratargetworkPointer->cameraTarget.z = object->trans.z + (cameratargetworkPointer->cameraOrbit.z * cosd(cameratargetworkPointer->cameraOrbit.y));
#if VERSION_PAL
                cameratargetworkPointer->cameraTarget.y -= (500.0f / REFRESH_RATE) * frameTimer2;
#else
                cameratargetworkPointer->cameraTarget.y -= (500.0f / REFRESH_RATE_OFF_BY_1) * frameTimer2;
#endif
                cameratargetworkPointer->cameraVelocity.x = 0;
                cameratargetworkPointer->cameraVelocity.y = 0;
                cameratargetworkPointer->cameraVelocity.z = 0.96f * cameratargetworkPointer->velocity.z;
            }
            else {
                cameratargetworkPointer->cameraTarget.z += cameratargetworkPointer->cameraVelocity.z * REFRESH_FREQ;
            }
            cameratargetworkPointer->cameraPosition.x = object->trans.x;
            cameratargetworkPointer->cameraPosition.y = object->trans.y;
            cameratargetworkPointer->cameraPosition.z = object->trans.z;
            positionVector.x = object->trans.x;
            positionVector.y = object->trans.y;
            positionVector.z = object->trans.z;
            if (cameratargetworkPointer->soundEmitterIds[0] < 0) {
                cameratargetworkPointer->soundEmitterIds[0] = HuAudFXEmiterPlay(1362, &positionVector);
            }
            if (cameratargetworkPointer->soundEmitterIds[1] < 0) {
                cameratargetworkPointer->soundEmitterIds[1] = HuAudFXEmiterPlay(1356, &positionVector);
            }
            if (object->work[1] < REFRESH_RATE_F * 3) {
                break;
            }
            object->work[0] = 1;
            object->work[1] = 0;
            break;

        case 1:
            cameratargetworkPointer->cameraVelocity.z *= 0.6f;
            cameratargetworkPointer->cameraTarget.z += (cameratargetworkPointer->cameraVelocity.z * REFRESH_FREQ);
            cameratargetworkPointer->cameraTarget.x += (cameratargetworkPointer->cameraVelocity.z * REFRESH_FREQ);
            cameratargetworkPointer->cameraPosition.x = object->trans.x;
            cameratargetworkPointer->cameraPosition.y = object->trans.y;
            cameratargetworkPointer->cameraPosition.z = object->trans.z;
            if (object->work[1] < REFRESH_RATE_F) {
                break;
            }
            object->work[0] = 2;
            object->work[1] = 0;
            break;

        case 2:
#if VERSION_PAL
            cameratargetworkPointer->cameraRotation.x += (90.0f / REFRESH_RATE_F);
#else
            cameratargetworkPointer->cameraRotation.x += (90.0f / REFRESH_RATE_OFF_BY_1);
#endif
            cameratargetworkPointer->velocity.z = 1000.0 * cosd(cameratargetworkPointer->cameraRotation.x);
            cameratargetworkPointer->velocity.y = 1000.0 * sind(cameratargetworkPointer->cameraRotation.x);
            cameratargetworkPointer->cameraPosition.x = object->trans.x;
            cameratargetworkPointer->cameraPosition.y = object->trans.y;
            cameratargetworkPointer->cameraPosition.z = object->trans.z;
            if (object->work[1] < REFRESH_RATE_F * 2) {
                break;
            }
            object->work[0] = 3;
            object->work[1] = 0;
            cameratargetworkPointer->cameraRotation.x = 180;
            break;

        case 3:
#if VERSION_PAL
            cameratargetworkPointer->cameraRotation.z += (180.0f / REFRESH_RATE_F);
#else
            cameratargetworkPointer->cameraRotation.z += (180.0f / REFRESH_RATE_OFF_BY_1);
#endif
            frameTimer = REFRESH_FREQ * object->work[1];
            frameTimer = 1.0f - (frameTimer * frameTimer);
            cameratargetworkPointer->cameraPosition.x += frameTimer * (object->trans.x - cameratargetworkPointer->cameraPosition.x);
            cameratargetworkPointer->cameraPosition.y += frameTimer * (object->trans.y - cameratargetworkPointer->cameraPosition.y);
            cameratargetworkPointer->cameraPosition.z += frameTimer * (object->trans.z - cameratargetworkPointer->cameraPosition.z);
            if (object->work[1] < REFRESH_RATE_F) {
                break;
            }
            cameraParticleVelocity.x = cameratargetworkPointer->cameraPosition.x;
            cameraParticleVelocity.y = cameratargetworkPointer->cameraPosition.y;
            cameraParticleVelocity.z = cameratargetworkPointer->cameraPosition.z;
            cameraParticlePosition.x = cameratargetworkPointer->cameraTarget.x;
            cameraParticlePosition.y = cameratargetworkPointer->cameraTarget.y;
            cameraParticlePosition.z = cameratargetworkPointer->cameraTarget.z;
            object->work[0] = 4;
            object->work[1] = 0;
            break;

        case 4:
#if VERSION_PAL
            cameratargetworkPointer->cameraRotation.z += (180.0f / REFRESH_RATE_F);
#else
            cameratargetworkPointer->cameraRotation.z += (180.0f / REFRESH_RATE_OFF_BY_1);
#endif
            if (object->work[1] > REFRESH_RATE_F * 0.7f) {
                if (cameratargetworkPointer->soundEmitterIds[0] >= 0) {
                    HuAudFXStop(cameratargetworkPointer->soundEmitterIds[0]);
                    cameratargetworkPointer->soundEmitterIds[0] = -1;
                }
                if (cameratargetworkPointer->soundEmitterIds[1] >= 0) {
                    HuAudFXStop(cameratargetworkPointer->soundEmitterIds[1]);
                    cameratargetworkPointer->soundEmitterIds[1] = -1;
                }
            }
            if (object->work[1] < REFRESH_RATE_F * 4) {
                break;
            }
            object->work[0] = 5;
            object->work[1] = 0;
            break;

        default:
            object->work[0] = 5;
            object->objFunc = SetCameraTargetPosition;
            break;
    }
    omSetRot(object, -cameratargetworkPointer->cameraRotation.x, cameratargetworkPointer->cameraRotation.y, cameratargetworkPointer->cameraRotation.z);
    if (object->trans.y < 12150.0f) {
        positionVector2.x = object->trans.x;
        positionVector2.y = object->trans.y;
        positionVector2.z = object->trans.z + 10;
        SetCameraParticleEmission(object->mdlId[1], &positionVector2, 4);
        positionVector2.x = positionVector4.x = object->trans.x;
        positionVector2.z = object->trans.z;
        positionVector4.z = object->trans.z - 10;
        positionVector2.y = object->trans.y + 250.0f;
        positionVector4.y = object->trans.y;
        positionVector3.x = 0;
        positionVector3.y = 0;
        positionVector3.z = 1;
        Hu3DShadowPosSet(&positionVector2, &positionVector3, &positionVector4);
    }
    if (cameratargetworkPointer->soundEmitterIds[0] >= 0) {
        positionVector.x = object->trans.x - 30;
        positionVector.y = object->trans.y;
        positionVector.z = object->trans.z + 25;
        HuAudFXEmiterUpDate(cameratargetworkPointer->soundEmitterIds[0], &positionVector);
    }
    if (cameratargetworkPointer->soundEmitterIds[1] < 0) {
        positionVector.x = object->trans.x + 30;
        positionVector.y = object->trans.y;
        positionVector.z = object->trans.z + 25;
        HuAudFXEmiterUpDate(cameratargetworkPointer->soundEmitterIds[1], &positionVector);
    }
    if (object->work[0] < 4) {
        SetCameraParameters(cameratargetworkPointer->cameraTarget.x, cameratargetworkPointer->cameraTarget.y, cameratargetworkPointer->cameraTarget.z, cameratargetworkPointer->cameraPosition.x, cameratargetworkPointer->cameraPosition.y, cameratargetworkPointer->cameraPosition.z);
    }
    if (GetCameraSequenceState() == 5) {
        object->objFunc = SetCameraTargetPosition;
    }
}

void SetCameraTargetPosition(OMOBJ *object)
{
    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
}

void UpdateCameraParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    if (particle->dataCnt == 0) {
        particle->dataCnt++;
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->time = 0;
            particleData->scale = 0;
            particleData->color.a = 0;
        }
    }
    else {
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            if (particleData->time) {
                particleData->pos.x += particleData->vel.x;
                particleData->pos.y += particleData->vel.y;
                particleData->pos.z += particleData->vel.z;
                particleData->scale += particleData->accel.x;
                particleData->accel.y -= particleData->accel.z;
                particleData->color.a = particleData->accel.y;
                if (--particleData->time == 0) {
                    particleData->time = 0;
                    particleData->scale = 0;
                    particleData->color.a = 0;
                }
            }
        }
        DCStoreRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
    }
}

void SetCameraParticleEmission(s32 particleModelId, Vec *emissionPosition, u32 particleCount)
{
    float interpolationValue;
    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleWork;
    s32 modelIndex;
    particleWork = Hu3DData[particleModelId].hookData;
    if (particleWork->dataCnt) {
        particleData = particleWork->data;
        for (modelIndex = 0; modelIndex < particleWork->maxCnt; modelIndex++, particleData++) {
            if (particleData->time != 0) {
                continue;
            }
            particleData->time = REFRESH_RATE_F * (((1.5f / 1000.0f) * frandmod(1000)) + 1.0f);
            particleData->pos.x = emissionPosition->x;
            particleData->pos.y = 12000;
            particleData->pos.z = emissionPosition->z;
            particleData->vel.x = REFRESH_FREQ * ((0.6f * frandmod(1000)) - 300.0f);
            particleData->vel.y = REFRESH_FREQ * ((0.04f * frandmod(1000)) + 10.0f);
            particleData->vel.z = 0;
            particleData->accel.x = 0.3f + ((3.0f / 1000.0f) * frandmod(1000));
            particleData->scale = 10;
            particleData->color.a = frandmod(127) + 64;
            particleData->accel.y = particleData->color.a;
            particleData->accel.z = particleData->accel.y * (1.0f / particleData->time);
            interpolationValue = frandmod(1000) * (1.0f / 1000.0f);
            particleData->color.r = (interpolationValue * 55) + 200;
            particleData->color.g = (interpolationValue * 50) + 205;
            particleData->color.b = (interpolationValue * 5) + 250;
            if (--particleCount == 0) {
                break;
            }
        }
    }
}

#if VERSION_PAL
#define MIN_PARTICLE_00_VALUE 282.5f
#define MAX_PARTICLE_00_VALUE 500.0f
#define PARTICLE_00_CAP 501
#else
#define MIN_PARTICLE_00_VALUE 339.0f
#define MAX_PARTICLE_00_VALUE 600.0f
#define PARTICLE_00_CAP 601
#endif

void UpdateCameraParticleEmitter(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float angle;
    float angle2;
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    float scratchValue[][6] = {
        { 600, 1, 0.95, 0.8, 1, 0 },
        { 600, 1, 0.6, 0.9, 0.3, 0 },
        { 13, 1, 1, 1, 0.5, 0 },
    };
    Vec positionVector;
    if (particle->dataCnt == 0) {
        particle->dataCnt++;
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->time = 1;
            particleData->scale = scratchValue[particle->emitCnt][0];
            particleData->color.r = 255 * scratchValue[particle->emitCnt][1];
            particleData->color.g = 255 * scratchValue[particle->emitCnt][2];
            particleData->color.b = 255 * scratchValue[particle->emitCnt][3];
            particleData->color.a = 255 * scratchValue[particle->emitCnt][4];
            angle = -70;
            angle2 = -10;
            particleData->vel.x = sind(angle2) * cosd(angle);
            particleData->vel.y = -sind(angle);
            particleData->vel.z = cosd(angle2) * cosd(angle);
        }
    }
    else {
        particle->dataCnt++;
        if (particle->dataCnt < MIN_PARTICLE_00_VALUE) {
            return;
        }
        if (particle->dataCnt >= MAX_PARTICLE_00_VALUE) {
            particle->dataCnt = PARTICLE_00_CAP;
            particleData = particle->data;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->time = 0;
                particleData->scale = 0;
                particleData->color.a = 0;
            }
        }
        else {
            particleData = particle->data;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                if (particleData->time == 0) {
                    continue;
                }
                positionVector.x = sind(CRot.y) * cosd(CRot.x);
                positionVector.y = -sind(CRot.x);
                positionVector.z = cosd(CRot.y) * cosd(CRot.x);
                if (particle->emitCnt < 2) {
                    angle = 4500;
                    particleData->pos.x = (Center.x + (particleData->vel.x * angle)) + (positionVector.x * CZoom);
                    particleData->pos.y = (Center.y + (particleData->vel.y * angle)) + (positionVector.y * CZoom);
                    particleData->pos.z = (Center.z + (particleData->vel.z * angle)) + (positionVector.z * CZoom);
                    if (particle->emitCnt != 0) {
                        angle = -((positionVector.x * particleData->vel.x) + (positionVector.y * particleData->vel.y) + (positionVector.z * particleData->vel.z));
                        if (angle > 0.77f) {
                            angle2 = (1.0f / 0.23f) * (angle - 0.77f);
                            particleData->scale = 1400 * angle2;
                            particleData->color.a = (32.0f * angle2) + 16.0f;
                        }
                        else {
                            particleData->scale = 0;
                        }
                    }
                }
                else {
                    angle = CZoom - 10;
                    particleData->pos.x = (positionVector.x * angle) + Center.x;
                    particleData->pos.y = (positionVector.y * angle) + Center.y;
                    particleData->pos.z = (positionVector.z * angle) + Center.z;
                    angle = -((positionVector.x * particleData->vel.x) + (positionVector.y * particleData->vel.y) + (positionVector.z * particleData->vel.z));
                    if (angle > 0.9f) {
                        angle2 = 10.0f * (angle - 0.9f);
                        particleData->color.a = angle2 * 210.0f;
                    }
                    else {
                        particleData->color.a = 0;
                    }
                }
            }
            DCStoreRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
        }
    }
}

void UpdateCameraParticleAnimation(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float angle;
    float angle2;
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    Vec positionVector;
    Vec positionVector2;
    Vec positionVector3;
    if (particle->dataCnt == 0) {
        particle->dataCnt++;
        angle = -70;
        angle2 = -10;
        particle->pos.x = sind(angle2) * cosd(angle);
        particle->pos.y = -sind(angle);
        particle->pos.z = cosd(angle2) * cosd(angle);
        angle = 0;
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->time = 1;
            angle += 1.0f / particle->maxCnt;
            particleData->scale = 40.0f + ((1.0f - angle) * (100.0f + frandmod(1000)));
            particleData->color.r = frandmod(127) + 128;
            particleData->color.g = frandmod(127) + 128;
            particleData->color.b = frandmod(127) + 128;
            particleData->color.a = frandmod(16) + 16;
        }
    }
    else {
        particle->dataCnt++;
        if (particle->dataCnt < MIN_PARTICLE_00_VALUE) {
            return;
        }
        if (particle->dataCnt >= MAX_PARTICLE_00_VALUE) {
            particle->dataCnt = PARTICLE_00_CAP;
            particleData = particle->data;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->time = 0;
                particleData->scale = 0;
                particleData->color.a = 0;
            }
        }
        else {
            positionVector.x = sind(CRot.y) * cosd(CRot.x);
            positionVector.y = -sind(CRot.x);
            positionVector.z = cosd(CRot.y) * cosd(CRot.x);
            angle = -((-particle->pos.x * -positionVector.x) + (-particle->pos.y * -positionVector.y) + (-particle->pos.z * -positionVector.z));
            positionVector2.x = -positionVector.x + (-2.0f * particle->pos.x * angle);
            positionVector2.y = -positionVector.y + (-2.0f * particle->pos.y * angle);
            positionVector2.z = -positionVector.z + (-2.0f * particle->pos.z * angle);
            angle = 4500;
            positionVector3.x = (Center.x + (particle->pos.x * angle)) + (positionVector.x * CZoom);
            positionVector3.y = (Center.y + (particle->pos.y * angle)) + (positionVector.y * CZoom);
            positionVector3.z = (Center.z + (particle->pos.z * angle)) + (positionVector.z * CZoom);
            particleData = particle->data;
            angle2 = 0;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                if (particleData->time == 0) {
                    continue;
                }
                angle2 += 1.0f / particle->maxCnt;
                angle = 10.0f * (430.0f * (1.0f - ((1.0f - angle2) * (1.0f - angle2))));
                particleData->pos.x = positionVector3.x + (positionVector2.x * angle);
                particleData->pos.y = positionVector3.y + (positionVector2.y * angle);
                particleData->pos.z = positionVector3.z + (positionVector2.z * angle);
            }
            DCStoreRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
        }
    }
}

void SetCameraParticleGroup(s16 particleModelId, u32 particleGroupId)
{
    float angle;
    float angle2;
    HU3DPARTICLEDATA *updateCounter;
    HU3DPARTICLE *particleWork;
    s32 modelIndex;
    s32 modelIndex2;
    float scratchValue[] = { 0.4f, 0.21f, 0.16f, 1.0f };
    float scratchValue2[] = { 0.15f, 0.6f, 1.3f, 1.0f };
    s16 scratchValue3[] = { 1, 0, 2, 0, 0, 1, 2, 0, 0, 1, 2, 1, 0, 0, 1, 2, 0, 1, 2, -1 };
    particleGroupId &= 0x3;
    particleWork = Hu3DData[particleModelId].hookData;
    particleWork->dataCnt = 1;
    angle = -70;
    angle2 = -10;
    particleWork->pos.x = sind(angle2) * cosd(angle);
    particleWork->pos.y = -sind(angle);
    particleWork->pos.z = cosd(angle2) * cosd(angle);
    updateCounter = particleWork->data;
    for (modelIndex2 = 0; modelIndex2 < particleWork->maxCnt; modelIndex2++, updateCounter++) {
        updateCounter->time = 1;
        updateCounter->parManId = particleGroupId;
        for (modelIndex = 0; scratchValue3[modelIndex] >= 0; modelIndex++) {
            if (particleGroupId != scratchValue3[modelIndex]) {
                continue;
            }
            if (cameraParticleData[modelIndex] == NULL) {
                break;
            }
        }
        if (scratchValue3[modelIndex] < 0) {
            updateCounter->scale = 0;
            updateCounter->color.a = 0;
            continue;
        }
        cameraParticleData[modelIndex] = updateCounter;
        cameraParticleCount++;
        updateCounter->scale = 800.0f * (scratchValue2[particleGroupId] * (1.0f + ((0.7f / 1000.0f) * frandmod(1000))));
        updateCounter->color.r = frandmod(160) + 32;
        updateCounter->color.g = frandmod(160) + 32;
        updateCounter->color.b = frandmod(160) + 32;
        updateCounter->color.a = (255.0f * scratchValue[particleGroupId]) - frandmod(16);
    }
    cameraParticleGroups[particleGroupId] = particleWork->data;
    cameraParticleGroupCounts[particleGroupId] = particleWork->maxCnt;
}

void UpdateCameraFade(void)
{
    float interpolationValue;
    float interpolationValue2;
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    float scratchValue[] = { 0.02f, 0.08f, 0.07f, 0.5f };
    float scratchValue2[] = { 0.0009f, 0.00020000001f, 0.00001f, 0.0005f };
    interpolationValue = 0.1f;
    for (particleIndex = 0; particleIndex < cameraParticleCount; particleIndex++) {
        particleData = cameraParticleData[particleIndex];
        interpolationValue2 = 0.5f * (scratchValue[particleData->parManId] * (1.0f + (frandmod(1000) * scratchValue2[particleData->parManId])));
        interpolationValue += interpolationValue2;
        particleData->accel.x = interpolationValue;
        interpolationValue += interpolationValue2;
    }
    interpolationValue = 1.0f / interpolationValue;
    for (particleIndex = 0; particleIndex < cameraParticleCount; particleIndex++) {
        particleData = cameraParticleData[particleIndex];
        particleData->accel.x *= interpolationValue;
        particleData->scale *= (0.8f - (0.4f * particleData->accel.x));
        particleData->accel.x = 10.0f * (400.0f * particleData->accel.x);
    }
    for (particleIndex = 0; particleIndex < 3; particleIndex++) {
        DCStoreRangeNoSync(cameraParticleGroups[particleIndex], cameraParticleGroupCounts[particleIndex] * sizeof(HU3DPARTICLEDATA));
    }
    PPCSync();
}

void CleanupCameraEffects(void)
{
    float angle;
    float angle2;
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    Vec positionVector;
    Vec positionVector2;
    Vec positionVector3;
    Vec positionVector4;
    particleData = cameraParticleData[0];
    particleData->time++;
    if (particleData->time < MIN_PARTICLE_00_VALUE) {
        return;
    }
    if (particleData->time >= MAX_PARTICLE_00_VALUE) {
        particleData->time = PARTICLE_00_CAP;
        for (particleIndex = 0; particleIndex < cameraParticleCount; particleIndex++) {
            particleData = cameraParticleData[particleIndex];
            particleData->time = 0;
            particleData->scale = 0;
            particleData->color.a = 0;
        }
        return;
    }
    positionVector.x = sind(CRot.y) * cosd(CRot.x);
    positionVector.y = -sind(CRot.x);
    positionVector.z = cosd(CRot.y) * cosd(CRot.x);
    angle = -70;
    angle2 = -10;
    positionVector2.x = sind(angle2) * cosd(angle);
    positionVector2.y = -sind(angle);
    positionVector2.z = cosd(angle2) * cosd(angle);
    angle = 4500;
    positionVector4.x = (Center.x + (positionVector2.x * angle)) + (positionVector.x * CZoom);
    positionVector4.y = (Center.y + (positionVector2.y * angle)) + (positionVector.y * CZoom);
    positionVector4.z = (Center.z + (positionVector2.z * angle)) + (positionVector.z * CZoom);
    angle = -((-positionVector2.x * -positionVector.x) + (-positionVector2.y * -positionVector.y) + (-positionVector2.z * -positionVector.z));
    positionVector3.x = -positionVector.x + (-2.0f * positionVector2.x * angle);
    positionVector3.y = -positionVector.y + (-2.0f * positionVector2.y * angle);
    positionVector3.z = -positionVector.z + (-2.0f * positionVector2.z * angle);
    if (angle > 0.5f) {
        angle2 = 3.0f - (2.0f * angle);
    }
    else {
        angle2 = 1.0f;
    }
    for (particleIndex = 0; particleIndex < cameraParticleCount; particleIndex++) {
        particleData = cameraParticleData[particleIndex];
        particleData->pos.x = positionVector4.x + (angle2 * (positionVector3.x * particleData->accel.x));
        particleData->pos.y = positionVector4.y + (angle2 * (positionVector3.y * particleData->accel.x));
        particleData->pos.z = positionVector4.z + (angle2 * (positionVector3.z * particleData->accel.x));
    }
    for (particleIndex = 0; particleIndex < 3; particleIndex++) {
        DCStoreRangeNoSync(cameraParticleGroups[particleIndex], cameraParticleGroupCounts[particleIndex] * sizeof(HU3DPARTICLEDATA));
    }
    PPCSync();
}

void GetCameraTargetTransform(Vec *cameraPosition, Vec *cameraRotation)
{
    cameraPosition->x = cameraTargetObject->trans.x;
    cameraPosition->y = cameraTargetObject->trans.y;
    cameraPosition->z = cameraTargetObject->trans.z;
    cameraRotation->x = cameraTargetObject->rot.x;
    cameraRotation->y = cameraTargetObject->rot.y;
    cameraRotation->z = cameraTargetObject->rot.z;
}

u32 GetCameraSequenceState(void)
{
    return cameraParticleEffectState;
}
