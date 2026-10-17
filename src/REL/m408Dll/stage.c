#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "REL/m408Dll.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

s32 vertexColorScratch[2];
ANIMDATA *stageSpriteAnimations[5];
s16 stageModelId;
s32 stageEffectTimer;
u32 stageParticleSeed;
OMOBJ *stageEffectObject;
s32 stageLightId;
s32 stageCameraModelId;
s32 stageParticleModelId;
HSFCONSTDATA *stageModelConstants[2];
void *stageDisplayLists[2];
HSFDRAWDATA *stageDrawData;
s16 stageLightHandle;

float textureScrollSpeed = 1000;
u32 stageRandomSeed = 0x41C64E6D;

void CreateStageEffectObject(HUPROCESS *objman);

void CreateStageEffects(HUPROCESS *objman)
{
    s32 i;
    stageEffectTimer = 0;
    stageParticleSeed = 0;
    stageLightId = stageCameraModelId = stageParticleModelId = -1;
    CreateStageEffectObject(objman);
    for (i = 0; i < (frand() & 0x7F); i++) {
        GetRandomStageValue();
    }
}

void InitializeStageEffects(OMOBJ *object);

void CreateStageEffectObject(HUPROCESS *objman)
{
    stageEffectObject = omAddObjEx(objman, 30, 35, 0, -1, InitializeStageEffects);
}

typedef struct StageModelHookWork {
    s16 animationPhase;
    s16 animationFrame;
    GXColor *originalVertexColors;
    HuVec2f *originalTextureCoordinates;
    u8 *processedVertexFlags;
    union {
        s16 animationParameters[32];
        float animationTimers[16];
    };
} StageModelHookWork;

void UpdateStageEffects(OMOBJ *object);

s16 CreateStageParticle(ANIMDATA *animationData, s16 particleCount, HU3DPARTICLEHOOK particleHook);
void UpdateStageParticleSparkles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void UpdateStageParticleDebris(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

StageModelHookWork *CreateStageModelHookWork(s16 modelId, s16 animationFrame);

void InitializeStageEffects(OMOBJ *object)
{
    s32 integerValue;
    float *floatPointer;
    s32 modelIndex;

    u16 *u16Pointer;
    StageModelHookWork *stagemodelhookworkPointer;
    object->data = u16Pointer = HuMemDirectMallocNum(HEAP_HEAP, 436 * sizeof(u16), HU_MEMNUM_OVL);
    stageSpriteAnimations[0] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M408, 0x24), HU_MEMNUM_OVL));
    stageSpriteAnimations[1] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M408, 0x25), HU_MEMNUM_OVL));
    stageSpriteAnimations[2] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M408, 0x26), HU_MEMNUM_OVL));
    stageSpriteAnimations[4] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M408, 0x28), HU_MEMNUM_OVL));
    stageSpriteAnimations[3] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M408, 0x29), HU_MEMNUM_OVL));
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x1A));
    omSetSca(object, 0.1f, 0.1f, 0.1f);
    integerValue = object->mdlId[0];
    Hu3DModelLayerSet(integerValue, 1);
    Hu3DModelAttrSet(integerValue, HU3D_ATTR_DISPOFF);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x1E));
    integerValue = object->mdlId[4];
    Hu3DModelLayerSet(integerValue, 1);
    Hu3DModelScaleSet(integerValue, 0.1f, 0.1f, 0.1f);
    Hu3DModelAttrSet(integerValue, HU3D_ATTR_DISPOFF);
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x20));
    integerValue = object->mdlId[5];
    Hu3DModelLayerSet(integerValue, 1);
    Hu3DModelScaleSet(integerValue, 0.1f, 0.1f, 0.1f);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x1B));
    integerValue = object->mdlId[2];
    Hu3DModelLayerSet(integerValue, 1);
    Hu3DModelPosSet(integerValue, -20, -10, -150);
    Hu3DModelScaleSet(integerValue, 0.14500001f, 0.14500001f, 0.14500001f);
    Hu3DModelAttrSet(integerValue, HU3D_ATTR_DISPOFF);
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x1D));
    integerValue = object->mdlId[3];
    Hu3DModelPosSet(integerValue, 0, -50, -500);
    Hu3DModelScaleSet(integerValue, 0.15f, 0.15f, 0.15f);
    Hu3DModelLayerSet(integerValue, 0);
    Hu3DModelAttrSet(integerValue, HU3D_ATTR_DISPOFF);
    stagemodelhookworkPointer = CreateStageModelHookWork(integerValue, integerValue);
    object->mdlId[9] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x1C));
    integerValue = object->mdlId[9];
    stageModelId = integerValue;
    Hu3DModelScaleSet(integerValue, 0.11000001f, 0.1f, 0.11000001f);
    Hu3DModelPosSet(integerValue, 0, 10000, 0);
    Hu3DModelLayerSet(integerValue, 1);
    Hu3DModelAttrSet(integerValue, HU3D_ATTR_DISPOFF);
    object->mdlId[11] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x1F));
    integerValue = object->mdlId[11];
    Hu3DModelPosSet(integerValue, 0, 10000, 0);
    Hu3DModelLayerSet(integerValue, 1);
    Hu3DModelScaleSet(integerValue, 0.105f, 0.1f, 0.105f);
    Hu3DModelAttrSet(integerValue, HU3D_ATTR_DISPOFF);
    object->mdlId[12] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x23));
    integerValue = object->mdlId[12];
    Hu3DModelLayerSet(integerValue, 3);
    Hu3DModelPosSet(integerValue, 0, 5000, 0);
    Hu3DModelScaleSet(integerValue, 0.1f, 0.1f, 0.1f);
    Hu3DModelAttrSet(integerValue, HU3D_ATTR_DISPOFF);
    Hu3DMotionSpeedSet(integerValue, 0);
    object->mdlId[19] = integerValue = CreateStageParticle(stageSpriteAnimations[0], 80, UpdateStageParticleSparkles);
    Hu3DModelLayerSet(integerValue, 1);
    Hu3DModelPosSet(integerValue, 0, 5100, 0);
    Hu3DParticleBlendModeSet(integerValue, 1);
    floatPointer = ((HU3DPARTICLE *)Hu3DData[integerValue].hookData)->work;
    floatPointer[2] = 140;
    object->mdlId[14] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x21));
    object->mdlId[15] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x22));
    object->mdlId[16] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x21));
    object->mdlId[17] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M408, 0x22));
    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
        integerValue = object->mdlId[modelIndex + 14];
        Hu3DModelLayerSet(integerValue, 3);
        Hu3DModelRotSet(integerValue, 0, frandmod(360), 0);
        Hu3DModelScaleSet(integerValue, 0.1f, 0.1f, 0.1f);
        Hu3DModelAttrSet(integerValue, HU3D_ATTR_DISPOFF);
        Hu3DMotionSpeedSet(integerValue, 0);
    }
    object->mdlId[6] = integerValue = CreateStageParticle(stageSpriteAnimations[4], 1, UpdateStageParticleDebris);
    Hu3DModelPosSet(integerValue, 0, 5000, 0);
    Hu3DModelLayerSet(integerValue, 3);
    Hu3DModelAttrReset(integerValue, HU3D_ATTR_DISPOFF);
    Hu3DParticleBlendModeSet(integerValue, 1);
    floatPointer = ((HU3DPARTICLE *)Hu3DData[integerValue].hookData)->work;
    floatPointer[1] = 800;
    object->objFunc = UpdateStageEffects;
}

void UpdateStageEffectAnimation(OMOBJ *object);

void UpdateStageEffects(OMOBJ *object)
{
    u16 *u16Pointer;
    u32 state;
    state = GetStageEventState();
    u16Pointer = object->data;
    switch (state) {
        case 1:
            u16Pointer[0] = 2;
            u16Pointer[1] = 0;
            stageParticleSeed = 1;
            break;

        case 2:
        case 3:
            UpdateStageEffectAnimation(object);
            u16Pointer[1]++;
            break;

        case 4:
            Hu3DModelScaleSet(object->mdlId[2], 0.16000001f, 0.16000001f, 0.16000001f);
            Hu3DModelShadowMapSet(object->mdlId[0]);
            Hu3DModelShadowSet(object->mdlId[4]);
            break;

        default:
            break;
    }
    if (state > 3) {
        if (stageCameraModelId >= 0) {
            HuAudFXStop(stageCameraModelId);
            stageCameraModelId = -1;
        }
        if (stageParticleModelId >= 0) {
            HuAudFXStop(stageParticleModelId);
            stageParticleModelId = -1;
        }
    }
}

s32 GetStageEffectState(void)
{
    switch (stageParticleSeed) {
        case 0:
            return 0;

        case 1:
            if (stageEffectTimer) {
                return 2;
            }
            else {
                return 1;
            }
            break;

        case 2:
            if (stageEffectTimer) {
                return 3;
            }
            else {
                return 4;
            }
            break;
    }
}

void CleanupStageEffects(void)
{
    OMOBJ *object;
    s32 modelIndex;
    object = stageEffectObject;
    if (stageParticleSeed >= 2) {
        return;
    }
    stageParticleSeed = 2;
    Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[9], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[11], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[12], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[19], HU3D_ATTR_DISPOFF);
    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
        Hu3DModelAttrSet(object->mdlId[modelIndex + 14], HU3D_ATTR_DISPOFF);
    }
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[4], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[3], HU3D_ATTR_DISPOFF);
}

void UpdateStageEffectParticles(OMOBJ *object);

void UpdateStageEffectAnimation(OMOBJ *object)
{
    float scale;
    float interpolationValue;
    float interpolationValue2;
    float interpolationValue3;
    u16 *u16Pointer;
    float *floatPointer;
    s32 integerValue;
    s32 modelIndex;
    HU3DCAMERA *camera;
    u16Pointer = object->data;
    camera = &Hu3DCamera[0];
    interpolationValue = GetStageEventValue();
    interpolationValue3 = 7000;
    switch (u16Pointer[0]) {
        case 2:
            Hu3DModelAttrReset(object->mdlId[9], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(object->mdlId[11], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(object->mdlId[12], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(object->mdlId[19], HU3D_ATTR_DISPOFF);
            for (modelIndex = 0; modelIndex < 4; modelIndex++) {
                Hu3DModelAttrReset(object->mdlId[modelIndex + 14], HU3D_ATTR_DISPOFF);
            }
            Hu3DModelAttrReset(object->mdlId[6], HU3D_ATTR_DISPOFF);
            if (stageCameraModelId < 0) {
                stageCameraModelId = HuAudFXPlay(1363);
            }
            u16Pointer[2] = 0;
            u16Pointer[0]++;

        case 3:
            scale = u16Pointer[1] * 0.25f;
            Hu3DModelRotSet(object->mdlId[9], 0, 0.7f * -scale, 0);
            Hu3DModelRotSet(object->mdlId[11], 0, 1.3f * scale, 0);
            Hu3DModelRotSet(object->mdlId[12], 0, 3.7f * scale, 0);
            interpolationValue2 = 1.0f - ((interpolationValue - 5000) / 15000);
            Hu3DMotionTimeSet(object->mdlId[12], 95.0f * interpolationValue2);
            integerValue = object->mdlId[6];
            floatPointer = ((HU3DPARTICLE *)Hu3DData[integerValue].hookData)->work;
            floatPointer[2] = 1.0f - ((interpolationValue - 5000) / (interpolationValue3 - 5000));
            if (interpolationValue <= 5400 && u16Pointer[2] == 0) {
                u16Pointer[2] = 30;
                if (!minigameExitRequested) {
                    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, u16Pointer[2]);
                    minigameExitAllowed = 1;
                }
                stageEffectTimer = 1;
                u16Pointer[0]++;
            }
            if (interpolationValue <= 4800) {
                floatPointer[1] = 1200;
                u16Pointer[0]++;
            }
            UpdateStageEffectParticles(object);
            break;

        case 4:
            if (u16Pointer[2]-- == 0) {
                if (!minigameExitRequested) {
                    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
                    minigameExitAllowed = 0;
                }
                if (stageLightId < 0) {
                    stageLightId = HuAudFXPlay(1357);
                }
                CleanupStageEffects();
                if (stageCameraModelId >= 0) {
                    HuAudFXStop(stageCameraModelId);
                    stageCameraModelId = -1;
                }
                if (stageParticleModelId < 0) {
                    stageParticleModelId = HuAudFXPlay(1361);
                }
                u16Pointer[0]++;
            }

            break;

        case 5:
            if (!WipeStatGet()) {
                stageEffectTimer = 0;
            }
            integerValue = object->mdlId[6];
            if (integerValue != 0) {
                Hu3DModelPosSet(integerValue, 0, 200 + interpolationValue, 0);
                floatPointer = ((HU3DPARTICLE *)Hu3DData[integerValue].hookData)->work;
                floatPointer[2] -= 0.025f;
                if (floatPointer[2] <= 0) {
                    Hu3DModelAttrReset(integerValue, HU3D_ATTR_PARTICLE_KILL);
                    Hu3DModelKill(integerValue);
                    object->mdlId[6] = 0;
                }
            }
            scale = u16Pointer[1] % 360;
            scale = 0.1f * (1.4f + ((0.2f * interpolationValue) / 5000));
            Hu3DModelScaleSet(object->mdlId[2], scale, scale, scale);
            break;
    }
}

void UpdateStageEffectParticles(OMOBJ *object)
{
    float interpolationValue;
    float interpolationValue2;
    float interpolationValue3;
    float interpolationValue4;
    s32 modelIndex;
    s32 integerValue;
    s32 integerValue2;
    interpolationValue4 = GetStageEventValue() - 1500;
    interpolationValue3 = 500.0f * (int)(interpolationValue4 / 500.0f);
    integerValue = (int)(interpolationValue4 / 500.0f) % 4;
    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
        integerValue2 = object->mdlId[integerValue + 14];
        Hu3DModelPosSet(integerValue2, 0, interpolationValue3, 0);
        interpolationValue = interpolationValue3 - interpolationValue4;
        interpolationValue2 = 0;
        if (interpolationValue > -250.0f && interpolationValue < 1525.0f) {
            interpolationValue2 = 45;
            if (interpolationValue < 0.0f) {
                interpolationValue -= -250.0f;
                interpolationValue2 *= fabs(interpolationValue / 250.0f);
            }
            else {
                if (interpolationValue > 1425) {
                    interpolationValue -= 1425;
                    interpolationValue2 *= 1.0 - fabs(interpolationValue / 100.0f);
                }
            }
        }
        Hu3DMotionTimeSet(integerValue2, interpolationValue2);
        interpolationValue3 += 500.0f;
        integerValue = (integerValue + 1) % 4;
    }
}

s16 CreateStageParticle(ANIMDATA *animationData, s16 particleCount, HU3DPARTICLEHOOK particleHook)
{
    s32 modelIndex;
    HU3DPARTICLEDATA *integerValue;
    HU3DPARTICLE *particleWork;
    s32 createdObjectId;
    HU3DPARTICLEDATA *particleData;
    createdObjectId = Hu3DParticleCreate(animationData, particleCount);
    Hu3DParticleHookSet(createdObjectId, particleHook);
    Hu3DModelAttrSet(createdObjectId, HU3D_ATTR_DISPOFF);
    particleWork = Hu3DData[createdObjectId].hookData;
    particleData = particleWork->data;
    for (modelIndex = 0; modelIndex < particleWork->maxCnt; modelIndex++, particleData++) {
        particleData->scale = 0;
    }
    integerValue = HuMemDirectMallocNum(HEAP_HEAP, sizeof(HU3DPARTICLEDATA), HU_MEMNUM_OVL);
    particleWork->work = integerValue;
    integerValue->time = createdObjectId;
    integerValue->parManId = 0;
    for (modelIndex = 0; modelIndex < 32; modelIndex++) {
        // Ugly expression to match store without new type
        *((s16 *)(((u8 *)integerValue) + (modelIndex * 2) + 4)) = 0;
    }
    return createdObjectId;
}

void UpdateStageParticleFlames(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float interpolationValue;
    float interpolationValue2;

    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleWork;
    s32 particleIndex;
    s32 particleIndex2;
    s16 integerValue;
    particleWork = particle->work;
    switch (particleWork->emitCnt) {
        case 0:
            particleData = particle->data;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->time = 0;
            }
            particle->dataCnt = 0;
            particleWork->emitCnt++;
            particleWork->pos.y = 3;
            particleWork->pos.z = 25;
            break;

        case 1:
            for (particleIndex = 0; particleIndex < 100; particleIndex++) {
                particleData = particle->data;
                for (particleIndex2 = 0; particleIndex2 < particle->maxCnt; particleIndex2++, particleData++) {
                    if (!particleData->time) {
                        break;
                    }
                }
                if (particleIndex2 == particle->maxCnt) {
                    particleWork->emitCnt++;
                    break;
                }
                particleData->pos.x = particleData->pos.y = particleData->pos.z = 0;
                integerValue = GetRandomStageValue() % 360;
                particleData->vel.x = sind(integerValue);
                particleData->vel.y = -6.0 + (2.0 * particleWork->unk10.x);
                particleData->vel.z = cosd(integerValue);
                particleData->accel.x = ((GetRandomStageValue() & 0x7) + particleWork->pos.z) - 4.0f;
                particleData->accel.x *= 0.1f;
                interpolationValue = particleWork->pos.y + (0.01 * (float)(GetRandomStageValue() % 400));
                interpolationValue *= 0.1f;
                particleData->pos.x = interpolationValue * particleData->vel.x;
                particleData->pos.y = 0;
                particleData->pos.z = interpolationValue * particleData->vel.z;
                particleData->scale = (0.01 * (frand() % 2000)) + 30.0;
                particleData->scale *= 0.1f;
                particleData->color.a = 255;
                particleData->time = 30;
            }
            break;
    }
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->time == 0) {
            continue;
        }
        interpolationValue2 = particleData->accel.x;
        particleData->pos.x += particleData->vel.x * interpolationValue2;
        particleData->pos.y += particleData->vel.y;
        particleData->pos.z += particleData->vel.z * interpolationValue2;
        particleData->scale *= 1.025f;
        particleData->color.a *= 0.95f;
        particleData->color.a--;
        particleData->accel.x *= 0.8f;
        particleData->vel.y *= 0.85f;
        particleData->vel.y += 0.1f;
        if (particleData->color.a < 2) {
            particleData->color.a = 0;
            particleData->time = 0;
            particle->dataCnt++;
        }
    }
    DCFlushRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
    if (particle->dataCnt >= particle->maxCnt && particleWork->emitCnt == 2) {
        particleWork->emitCnt = 0;
        Hu3DModelAttrReset(particleWork->dataCnt, HU3D_ATTR_PARTICLE_KILL);
        Hu3DModelKill(particleWork->dataCnt);
    }
}

void UpdateStageParticleSmoke(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float interpolationValue;
    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleWork;
    s32 particleIndex;
    s16 integerValue;
    s32 integerValue2;
    particleWork = particle->work;
    switch (particleWork->emitCnt) {
        case 0:
            particleData = particle->data;
            particleWork->pos.x = 10;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->pos.x = particleData->pos.y = particleData->pos.z = 0;
                integerValue = GetRandomStageValue() % 360;
                particleData->vel.x = sind(integerValue);
                particleData->vel.y = 0;
                particleData->vel.z = cosd(integerValue);
                interpolationValue = particleWork->pos.x + (particleWork->pos.x * ((1.0 / 65536.0) * (float)GetRandomStageValue()));
                particleData->pos.x = interpolationValue * particleData->vel.x;
                particleData->pos.y = -(100.0 * ((1.0 / 65536.0) * (float)GetRandomStageValue()));
                particleData->pos.z = interpolationValue * particleData->vel.z;
                particleData->scale = (0.01 * (frand() % 2000)) + 40.0;
                integerValue2 = frand() & 0x2F;
                particleData->color.r = integerValue2 + 208;
                particleData->color.g = integerValue2 + 208;
                particleData->color.a = (frand() & 0x5F) + 32;
            }
            particleWork->emitCnt++;
            particle->dataCnt = 60;
            break;

        case 1:
            if (--particle->dataCnt == 0) {
                Hu3DModelAttrReset(particleWork->dataCnt, HU3D_ATTR_PARTICLE_KILL);
                Hu3DModelKill(particleWork->dataCnt);
                return;
            }
            break;
    }
    DCFlushRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void UpdateStageParticleSparkles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float interpolationValue;
    float interpolationValue2;

    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleWork;
    s32 particleIndex;
    s32 integerValue;
    s16 integerValue2;
    particleWork = particle->work;
    switch (particleWork->emitCnt) {
        case 0:
            particleData = particle->data;
            interpolationValue2 = 360.0f / particle->maxCnt;

            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->time = (s32)(360.0 * ((1.0 / 65536.0) * (float)GetRandomStageValue()));
                particleData->pos.x = particleData->pos.y = particleData->pos.z = 0;
                integerValue2 = (interpolationValue2 * particleIndex) + (int)((interpolationValue2 * 0.5f) * ((1.0 / 32768.0) * (float)(GetRandomStageValue() - 32768)));
                particleData->speedDecay = integerValue2;
                particleData->zRot = (M_PI * (integerValue2 + 90.0f)) / 180.0;
                particleData->vel.x = sind(integerValue2);
                particleData->vel.y = 0;
                particleData->vel.z = cosd(integerValue2);
                interpolationValue = particleWork->pos.y + ((0.25f * particleWork->pos.y) * ((1.0 / 65536.0) * (float)GetRandomStageValue()));
                particleData->accel.x = interpolationValue * particleData->vel.x;
                particleData->accel.y = 120.0 * ((1.0 / 32768.0) * (float)(GetRandomStageValue() - 32768));
                particleData->accel.z = interpolationValue * particleData->vel.z;
                particleData->colorIdx = particleData->scale = (0.01 * (frand() % 2000)) + 160;
                if (particle->blendMode == 1) {
                    integerValue = frand() & 0x2F;
                    particleData->color.r = integerValue + 32;
                    particleData->color.g = integerValue + 32;
                    particleData->color.b = 64 + integerValue + ((int)(((46 - integerValue) * ((1.0 / 65536.0) * (float)GetRandomStageValue()))));
                    particleData->color.a = (frand() & 0x2F) + 32;
                }
                else {
                    integerValue = frand() & 0x2F;
                    particleData->color.r = integerValue + 208;
                    particleData->color.g = integerValue + 208;
                    particleData->color.a = (frand() & 0x4F) + 176;
                }
            }
            particleWork->emitCnt++;

        case 1:
            particleData = particle->data;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->time += 4;
                if (particleData->time >= 360) {
                    particleData->time -= 360;
                }
                particleData->pos.x = particleData->accel.x;
                particleData->pos.y = particleData->accel.y;
                particleData->pos.z = particleData->accel.z;
                particleData->zRot = M_PI * (90 + (particleData->speedDecay + particleWork->pos.z)) / 180;
                particleData->scale = particleData->colorIdx * ((0.1f * sind(particleData->time)) + 1.0);
            }
            break;
    }
    DCFlushRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void UpdateStageParticleGlow(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float interpolationValue;
    float interpolationValue2;
    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleWork;
    s32 particleIndex;
    s16 integerValue;
    s32 integerValue2;
    particleWork = particle->work;
    switch (particleWork->emitCnt) {
        case 0:
            particleData = particle->data;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->time = GetRandomStageValue() % 360;
                particleData->pos.x = particleData->pos.y = particleData->pos.z = 0;
                integerValue = GetRandomStageValue() % 360;
                particleData->speedDecay = integerValue;
                particleData->vel.x = sind(integerValue);
                particleData->vel.y = 0;
                particleData->vel.z = cosd(integerValue);
                interpolationValue2 = particleWork->pos.x + (0.01 * ((30.0f * particleWork->pos.x) * ((1.0 / 65536.0) * (float)GetRandomStageValue())));
                interpolationValue = (0.15f + (0.3f * ((1.0 / 65536.0) * (float)GetRandomStageValue())));
                particleData->accel.x = interpolationValue2 * particleData->vel.x;
                particleData->accel.y = 50 * ((1.0 / 32768.0) * (float)(GetRandomStageValue() - 32768));
                particleData->accel.z = interpolationValue2 * particleData->vel.z * interpolationValue;
                particleData->colorIdx = particleData->scale
                    = particleWork->pos.x + (0.01 * ((50.0f * particleWork->pos.x) * ((1.0 / 65536.0) * (float)GetRandomStageValue())));
                integerValue2 = frand() & 0x7F;
                particleData->color.r = integerValue2 + 64;
                particleData->color.g = integerValue2 + 64;
                interpolationValue2 -= particleWork->pos.x;
                particleData->color.a
                    = 32 + (int)((48.0f * interpolationValue2) / (0.2f * particleWork->pos.x)) + (int)(32 * ((1.0 / 65536.0) * (float)GetRandomStageValue()));
            }
            particleWork->emitCnt++;

        case 1:
            particleData = particle->data;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->time += 2;
                if (particleData->time >= 360) {
                    particleData->time -= 360;
                }
                particleData->pos.x = particleData->accel.x;
                particleData->pos.y = particleData->accel.y;
                particleData->pos.z = particleData->accel.z;
                particleData->scale = particleData->colorIdx * ((0.1f * sind(particleData->time)) + 1.0);
            }
            break;
    }
    DCFlushRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void UpdateStageParticleDebris(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float interpolationValue;
    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleWork;
    s32 particleIndex;
    u8 integerValue;
    particleWork = particle->work;
    switch (particleWork->emitCnt) {
        case 0:
            particleData = particle->data;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->pos.x = particleData->pos.y = particleData->pos.z = 0;
                particleData->color.r = particleData->color.g = particleData->color.b = particleData->color.a = 255;
                particleData->scale = particleWork->pos.x;
            }
            particleWork->emitCnt++;
            break;

        case 1:
            if (particleWork->pos.y < 0.0f) {
                break;
            }
            interpolationValue = 0.1f + particleWork->pos.y;
            interpolationValue = 1.2f * (interpolationValue * interpolationValue);
            if (interpolationValue > 1.0f) {
                interpolationValue = 1.0f;
            }
            integerValue = 255 * interpolationValue;
            particleData = particle->data;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->color.a = integerValue;
            }
            break;
    }
    DCFlushRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void UpdateStageParticleTrail(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float angle;
    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleWork;
    s32 particleIndex;
    s16 integerValue;
    integerValue = 0;
    particleWork = particle->work;
    switch (particleWork->emitCnt) {
        case 0:
            particleData = particle->data;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->time = particleData->parManId = 0;
            }
            particleWork->emitCnt++;
            break;

        case 1:
            particleData = particle->data;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                if (particleData->time) {
                    particleData->time--;
                    continue;
                }
                if (particleData->parManId == 0) {
                    angle = 360.0f * ((1.0 / 65536.0) * (float)GetRandomStageValue());
                    particleData->pos.x = 10.0 * (50.0 + (250.0 * ((1.0 / 65536.0) * (float)GetRandomStageValue()))) * sind(angle);
                    particleData->pos.y = 0;
                    particleData->pos.z = 10.0 * (50.0 + (250.0 * ((1.0 / 65536.0) * (float)GetRandomStageValue()))) * cosd(angle);
                    particleData->zRot = (M_PI * (angle - 90.0f)) / 180;
                    particleData->color.r = particleData->color.g = particleData->color.b = particleData->color.a = 192;
                    particleData->color.b = 160;
                    particleData->scale = 0;
                    particleData->time = (s32)(90.0 * ((1.0 / 65536.0) * (float)GetRandomStageValue()));
                    particleData->parManId = 60;
                }
                particleData->color.a = 248 * sind(particleData->parManId * 3);
                particleData->scale = particleWork->pos.x * (60 - particleData->parManId);
                particleData->parManId--;
            }
            if (particle->maxCnt == integerValue) {
                Hu3DModelAttrReset(particleWork->dataCnt, HU3D_ATTR_PARTICLE_KILL);
                Hu3DModelKill(particleWork->dataCnt);
                return;
            }
            break;
    }
    DCFlushRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

StageModelHookWork *CreateStageModelHookWork(s16 modelId, s16 animationFrame)
{
    StageModelHookWork *returnPointer;
    s32 modelIndex;
    HU3DMODEL *model;
    model = &Hu3DData[modelId];
    returnPointer = HuMemDirectMallocNum(HEAP_HEAP, sizeof(StageModelHookWork), HU_MEMNUM_OVL);
    model->hookData = returnPointer;
    returnPointer->animationPhase = modelId;
    returnPointer->animationFrame = animationFrame;
    returnPointer->originalVertexColors = NULL;
    returnPointer->originalTextureCoordinates = NULL;
    for (modelIndex = 0; modelIndex < 32; modelIndex++) {
        returnPointer->animationParameters[modelIndex] = 0;
    }
    return returnPointer;
}

void UpdateStageModelHook(s16 modelId)
{
    float angle;
    StageModelHookWork *stagemodelhookworkPointer;
    s32 modelIndex;
    HSFOBJECT *sceneObject;
    s32 modelIndex2;
    HU3DMODEL *model;
    model = &Hu3DData[modelId];
    sceneObject = model->hsf->root;
    stagemodelhookworkPointer = model->hookData;
    if (stagemodelhookworkPointer->originalTextureCoordinates == NULL) {
        stagemodelhookworkPointer->originalTextureCoordinates = HuMemDirectMallocNum(HEAP_HEAP, sceneObject->mesh.st->count * sizeof(HuVec2f), HU_MEMNUM_OVL);
        stagemodelhookworkPointer->processedVertexFlags = HuMemDirectMallocNum(HEAP_HEAP, sceneObject->mesh.st->count, HU_MEMNUM_OVL);
        for (modelIndex = 0; modelIndex < sceneObject->mesh.st->count; modelIndex++) {
            stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].x = ((HuVec2f *)(sceneObject->mesh.st->data))[modelIndex].x;
            stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].y = ((HuVec2f *)(sceneObject->mesh.st->data))[modelIndex].y;
        }
    }
    stagemodelhookworkPointer->animationTimers[0] += 2.0f;
    for (modelIndex = 0; modelIndex < sceneObject->mesh.st->count; modelIndex++) {
        stagemodelhookworkPointer->processedVertexFlags[modelIndex] = 0;
    }
    for (modelIndex = 0; modelIndex < sceneObject->mesh.st->count; modelIndex++) {
        if (stagemodelhookworkPointer->processedVertexFlags[modelIndex]) {
            continue;
        }
        stagemodelhookworkPointer->processedVertexFlags[modelIndex]++;
        angle = stagemodelhookworkPointer->animationTimers[0] + (60.0f * modelIndex);
        ((HuVec2f *)(sceneObject->mesh.st->data))[modelIndex].x = stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].x + (0.005f * sind(angle));
        ((HuVec2f *)(sceneObject->mesh.st->data))[modelIndex].y = stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].y + (0.005f * cosd(angle));
        for (modelIndex2 = modelIndex; modelIndex2 < sceneObject->mesh.st->count; modelIndex2++) {
            if (stagemodelhookworkPointer->processedVertexFlags[modelIndex2]) {
                continue;
            }
            if (stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].x != stagemodelhookworkPointer->originalTextureCoordinates[modelIndex2].x || stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].y != stagemodelhookworkPointer->originalTextureCoordinates[modelIndex2].y) {
                continue;
            }
            stagemodelhookworkPointer->processedVertexFlags[modelIndex2]++;
            ((HuVec2f *)(sceneObject->mesh.st->data))[modelIndex2].x = ((HuVec2f *)(sceneObject->mesh.st->data))[modelIndex].x;
            ((HuVec2f *)(sceneObject->mesh.st->data))[modelIndex2].y = ((HuVec2f *)(sceneObject->mesh.st->data))[modelIndex].y;
        }
    }
}

typedef struct AnimatedMeshDrawWork {
    HU3DMODEL *model;
    HSFOBJECT *sceneObject;
    s32 textureCoordinateCount;
    Mtx transformMatrix;
    float textureAnimationOffset[2];

} AnimatedMeshDrawWork;

void DrawAnimatedStageMesh(AnimatedMeshDrawWork *drawWork, HSFFACE *face, HSFDRAWDATA *drawData, GXColor *vertexColors);

void TransformStageModelVertices(HU3DMODEL *model, Mtx transformMatrix)
{
    StageModelHookWork *stagemodelhookworkPointer;
    GXColor *gxcolorPointer;
    s16 modelIndex;
    s32 frameCounter;
    HSFBUFFER *hsfbufferPointer;
    HSFFACE *hsffacePointer;

    AnimatedMeshDrawWork scratchValue;
    Mtx transformMatrix2;
    Mtx transformMatrix3;
    Mtx transformMatrix4;
    s16 scratchValue2 = 0;
    stagemodelhookworkPointer = model->hookData;
    scratchValue.model = &Hu3DData[stagemodelhookworkPointer->animationFrame];
    scratchValue.sceneObject = scratchValue.model->hsf->root;
    MTXCopy(transformMatrix, scratchValue.transformMatrix);
    stageModelConstants[0] = scratchValue.sceneObject->constData;
    stageDisplayLists[0] = stageModelConstants[0]->dlBuf;
    stageDrawData = stageModelConstants[0]->drawData;
    GXLoadPosMtxImm(scratchValue.transformMatrix, GX_PNMTX0);
    MTXInverse(scratchValue.transformMatrix, transformMatrix2);
    MTXTranspose(transformMatrix2, transformMatrix3);
    GXLoadNrmMtxImm(transformMatrix3, GX_PNMTX0);
    if (Hu3DShadowF && (stageModelConstants[0]->attr & 0x8)) {
        MTXConcat(transformMatrix4, scratchValue.transformMatrix, transformMatrix3);
        MTXConcat(Hu3DShadowData.projMtx, Hu3DShadowData.lookAtMtx, transformMatrix2);
        MTXConcat(transformMatrix2, transformMatrix3, transformMatrix3);
        GXLoadTexMtxImm(transformMatrix3, GX_TEXMTX9, GX_MTX3x4);
    }
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
    if (stagemodelhookworkPointer->originalVertexColors == NULL) {
        stagemodelhookworkPointer->originalVertexColors = HuMemDirectMallocNum(HEAP_HEAP, scratchValue.sceneObject->mesh.color->count * sizeof(GXColor), HU_MEMNUM_OVL);
        for (modelIndex = 0; modelIndex < scratchValue.sceneObject->mesh.color->count; modelIndex++) {
            gxcolorPointer = &stagemodelhookworkPointer->originalVertexColors[modelIndex];
            gxcolorPointer->r = stagemodelhookworkPointer->animationParameters[0];
            gxcolorPointer->g = stagemodelhookworkPointer->animationParameters[1];
            gxcolorPointer->b = stagemodelhookworkPointer->animationParameters[2];
            if (((GXColor *)(scratchValue.sceneObject->mesh.color->data))[modelIndex].a == 255) {
                gxcolorPointer->a = stagemodelhookworkPointer->animationParameters[3];
            }
            else {
                gxcolorPointer->a = ((GXColor *)(scratchValue.sceneObject->mesh.color->data))[modelIndex].a;
            }
        }
    }
    hsfbufferPointer = scratchValue.sceneObject->mesh.face;
    hsffacePointer = hsfbufferPointer->data;
    frameCounter = 0;
    for (modelIndex = 0; modelIndex < hsfbufferPointer->count;) {
        DrawAnimatedStageMesh(&scratchValue, hsffacePointer, &stageDrawData[frameCounter], stagemodelhookworkPointer->originalVertexColors);
        frameCounter++;
        modelIndex += stageDrawData[frameCounter - 1].polyCnt;
        hsffacePointer += stageDrawData[frameCounter - 1].polyCnt;
    }
}

void TransformStageModelNormals(HU3DMODEL *model, Mtx normalMatrix)
{
    StageModelHookWork *stagemodelhookworkPointer;
    s16 modelIndex;
    s16 modelIndex2;
    s16 modelIndex3;
    GXColor *gxcolorPointer;
    s32 frameCounter;
    HSFBUFFER *hsfbufferPointer;
    HSFFACE *hsffacePointer;
    s32 integerValue;
    void *integerValue2;

    AnimatedMeshDrawWork scratchValue;
    Mtx transformMatrix;
    Mtx transformMatrix2;
    Mtx transformMatrix3;
    float scratchValue2[2][8];
    float scratchValue3[2][8];
    s16 scratchValue4 = 0;
    stagemodelhookworkPointer = model->hookData;
    scratchValue.model = &Hu3DData[stagemodelhookworkPointer->animationFrame];
    scratchValue.sceneObject = scratchValue.model->hsf->root;
    MTXCopy(normalMatrix, scratchValue.transformMatrix);
    stageModelConstants[0] = scratchValue.sceneObject->constData;
    stageDisplayLists[0] = stageModelConstants[0]->dlBuf;
    stageDrawData = stageModelConstants[0]->drawData;
    GXLoadPosMtxImm(scratchValue.transformMatrix, GX_PNMTX0);
    MTXInverse(scratchValue.transformMatrix, transformMatrix);
    MTXTranspose(transformMatrix, transformMatrix2);
    GXLoadNrmMtxImm(transformMatrix2, GX_PNMTX0);
    if (Hu3DShadowF && (stageModelConstants[0]->attr & 0x8)) {
        MTXConcat(transformMatrix3, scratchValue.transformMatrix, transformMatrix2);
        MTXConcat(Hu3DShadowData.projMtx, Hu3DShadowData.lookAtMtx, transformMatrix);
        MTXConcat(transformMatrix, transformMatrix2, transformMatrix2);
        GXLoadTexMtxImm(transformMatrix2, GX_TEXMTX9, GX_MTX3x4);
    }
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
    if (stagemodelhookworkPointer->originalVertexColors == NULL) {
        stagemodelhookworkPointer->originalVertexColors = HuMemDirectMallocNum(HEAP_HEAP, scratchValue.sceneObject->mesh.color->count * sizeof(GXColor), HU_MEMNUM_OVL);
        for (modelIndex = 0; modelIndex < scratchValue.sceneObject->mesh.color->count; modelIndex++) {
            gxcolorPointer = &stagemodelhookworkPointer->originalVertexColors[modelIndex];
            gxcolorPointer->r = stagemodelhookworkPointer->animationParameters[0];
            gxcolorPointer->g = stagemodelhookworkPointer->animationParameters[1];
            gxcolorPointer->b = stagemodelhookworkPointer->animationParameters[2];
            gxcolorPointer->a = stagemodelhookworkPointer->animationParameters[3];
        }
    }
    if (stagemodelhookworkPointer->originalTextureCoordinates == NULL) {
        stagemodelhookworkPointer->originalTextureCoordinates = HuMemDirectMallocNum(HEAP_HEAP, scratchValue.sceneObject->mesh.st->count * sizeof(HuVec2f), HU_MEMNUM_OVL);
        stagemodelhookworkPointer->processedVertexFlags = HuMemDirectMallocNum(HEAP_HEAP, scratchValue.sceneObject->mesh.st->count, HU_MEMNUM_OVL);
        for (modelIndex = 0; modelIndex < scratchValue.sceneObject->mesh.st->count; modelIndex++) {
            stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].x = ((HuVec2f *)(scratchValue.sceneObject->mesh.st->data))[modelIndex].x;
            stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].y = ((HuVec2f *)(scratchValue.sceneObject->mesh.st->data))[modelIndex].y;
        }
    }
    stagemodelhookworkPointer->animationTimers[15] += 0.5f;
    for (modelIndex = 0; modelIndex < 8; modelIndex++) {
        float angle = stagemodelhookworkPointer->animationTimers[15] + (40.0f * modelIndex);
        scratchValue2[0][modelIndex] = sind(angle);
        scratchValue3[0][modelIndex] = cosd(angle);
    }
    for (modelIndex = 0; modelIndex < scratchValue.sceneObject->mesh.st->count; modelIndex++) {
        stagemodelhookworkPointer->processedVertexFlags[modelIndex] = 0;
    }
    for (modelIndex = 0, modelIndex2 = 0; modelIndex < scratchValue.sceneObject->mesh.st->count; modelIndex++, modelIndex2++) {
        if (stagemodelhookworkPointer->processedVertexFlags[modelIndex]) {
            continue;
        }
        stagemodelhookworkPointer->processedVertexFlags[modelIndex]++;
        if (stagemodelhookworkPointer->animationParameters[4]) {
            stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].x = (1.5f * ((HuVec2f *)(scratchValue.sceneObject->mesh.st->data))[modelIndex].x) + (0.04f * scratchValue2[0][modelIndex2 & 0x7]);
            stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].y = (2.0f * ((HuVec2f *)(scratchValue.sceneObject->mesh.st->data))[modelIndex].y) + (0.04f * scratchValue3[0][modelIndex2 & 0x7]);
        }
        else {
            stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].x = (1.25f * ((HuVec2f *)(scratchValue.sceneObject->mesh.st->data))[modelIndex].x) + (0.04f * scratchValue2[0][modelIndex2 & 0x7]);
            stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].y = (1.75f * ((HuVec2f *)(scratchValue.sceneObject->mesh.st->data))[modelIndex].y) + (0.04f * scratchValue3[0][modelIndex2 & 0x7]);
        }
        integerValue = stagemodelhookworkPointer->animationParameters[3] + ((0.8f * stagemodelhookworkPointer->animationParameters[3]) * sind((3.0f * stagemodelhookworkPointer->animationTimers[15]) + (120.0f * modelIndex2)));
        stagemodelhookworkPointer->originalVertexColors[modelIndex].a = integerValue;
        for (modelIndex3 = modelIndex; modelIndex3 < scratchValue.sceneObject->mesh.st->count; modelIndex3++) {
            modelIndex2++;
            if (stagemodelhookworkPointer->processedVertexFlags[modelIndex3]) {
                continue;
            }
            if (((HuVec2f *)(scratchValue.sceneObject->mesh.st->data))[modelIndex].x != ((HuVec2f *)(scratchValue.sceneObject->mesh.st->data))[modelIndex3].x
                || ((HuVec2f *)(scratchValue.sceneObject->mesh.st->data))[modelIndex].y != ((HuVec2f *)(scratchValue.sceneObject->mesh.st->data))[modelIndex3].y) {
                continue;
            }
            stagemodelhookworkPointer->processedVertexFlags[modelIndex3]++;
            stagemodelhookworkPointer->originalTextureCoordinates[modelIndex3].x = stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].x;
            stagemodelhookworkPointer->originalTextureCoordinates[modelIndex3].y = stagemodelhookworkPointer->originalTextureCoordinates[modelIndex].y;
            stagemodelhookworkPointer->originalVertexColors[modelIndex3].a = integerValue;
            modelIndex2--;
        }
    }
    integerValue2 = scratchValue.sceneObject->mesh.st->data;
    scratchValue.sceneObject->mesh.st->data = stagemodelhookworkPointer->originalTextureCoordinates;
    hsfbufferPointer = scratchValue.sceneObject->mesh.face;
    hsffacePointer = hsfbufferPointer->data;
    frameCounter = 0;
    for (modelIndex = 0; modelIndex < hsfbufferPointer->count;) {
        DrawAnimatedStageMesh(&scratchValue, hsffacePointer, &stageDrawData[frameCounter], stagemodelhookworkPointer->originalVertexColors);
        frameCounter++;
        modelIndex += stageDrawData[frameCounter - 1].polyCnt;
        hsffacePointer += stageDrawData[frameCounter - 1].polyCnt;
    }
    scratchValue.sceneObject->mesh.st->data = integerValue2;
}

void LoadStageTexture(HU3DMODEL *model, HSFBITMAP *bitmap, HSFATTRIBUTE *attribute, s16 textureMapId);
void ConfigureStageMaterial(AnimatedMeshDrawWork *drawWork, HSFMATERIAL *material);

void DrawAnimatedStageMesh(AnimatedMeshDrawWork *drawWork, HSFFACE *face, HSFDRAWDATA *drawData, GXColor *vertexColors)
{
    HSFMATERIAL *material2;
    HSFOBJECT *sceneObject;
    HSFATTRIBUTE *attribute2;
    s16 animationIndex;
    s16 integerValue;
    HU3DATTRANIM *hu3dattranimPointer;
    HU3DMODEL *model;
    HU3DTEXANIM *hu3dtexanimPointer;
    void *voidPointer;
    GXColor drawColor;
    sceneObject = drawWork->sceneObject;
    model = drawWork->model;
    material2 = &sceneObject->mesh.material[face->mat & 0xFFF];
    drawColor.r = material2->litColor[0];
    drawColor.g = material2->litColor[1];
    drawColor.b = material2->litColor[2];
    drawColor.a = 255;
    GXSetChanAmbColor(GX_COLOR0A0, drawColor);
    drawColor.r = material2->color[0];
    drawColor.g = material2->color[1];
    drawColor.b = material2->color[2];
    drawColor.a = 255;
    GXSetChanMatColor(GX_COLOR0A0, drawColor);
    if ((material2->invAlpha != 0 || material2->pass == 1) && !(model->attr & 0x2)) {
        GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    }
    else {
        GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    }
    if (material2->attrNum == 0) {
        return;
    }
    integerValue = (sceneObject->mesh.color) ? 5 : 1;
    if (drawData->flags & 0x2) {
        integerValue |= 0x2;
    }
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, sceneObject->mesh.vertex->data, sizeof(HuVecF));
    if (integerValue & 0x2) {
        GXSetVtxDesc(GX_VA_NBT, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NBT, GX_NRM_NBT, GX_S16, 8);
    }
    else {
        GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
        GXSetArray(GX_VA_NRM, sceneObject->mesh.normal->data, sizeof(HuVecF));
    }
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_TEX0, sceneObject->mesh.st->data, sizeof(HuVec2f));
    if (integerValue & 0x4) {
        GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GXSetArray(GX_VA_CLR0, vertexColors, sizeof(GXColor));
    }
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZCompLoc(GX_FALSE);
    for (animationIndex = 0; animationIndex < material2->attrNum; animationIndex++) {
        attribute2 = &sceneObject->mesh.attribute[material2->attr[animationIndex]];
        if (attribute2->animWorkP) {
            hu3dattranimPointer = attribute2->animWorkP;
            hu3dtexanimPointer = &Hu3DTexAnimData[hu3dattranimPointer->animId];
            if ((hu3dattranimPointer->attr & 0x1) && !(hu3dtexanimPointer->attr & 0x4)) {
                if (Hu3DAnimSet(drawWork->model, attribute2, (s16)animationIndex)) {
                    continue;
                }
            }
        }
        LoadStageTexture(drawWork->model, attribute2->bitmap, attribute2, (s16)animationIndex);
    }
    ConfigureStageMaterial(drawWork, material2);
    voidPointer = (void *)(drawData->dlOfs + ((u32)stageDisplayLists[0]));
    GXCallDisplayList(voidPointer, drawData->dlSize);
}

void LoadStageTexture(HU3DMODEL *model, HSFBITMAP *bitmap, HSFATTRIBUTE *attribute, s16 textureMapId)
{
    GXTexObj textureObject;
    GXTlutObj textureObject2;
    s16 integerValue;
    s16 integerValue2;
    s16 integerValue3;
    s16 integerValue4;
    integerValue = bitmap->sizeX;
    integerValue2 = bitmap->sizeY;
    integerValue3 = (attribute->wrapS == 1) ? 1 : 0;
    integerValue4 = (attribute->wrapT == 1) ? 1 : 0;
    switch (bitmap->dataFmt) {
        case 6:
            GXInitTexObj(&textureObject, bitmap->data, integerValue, integerValue2, GX_TF_RGBA8, integerValue3, integerValue4, GX_FALSE);
            break;

        case 4:
            GXInitTexObj(&textureObject, bitmap->data, integerValue, integerValue2, GX_TF_RGB565, integerValue3, integerValue4, GX_FALSE);
            break;

        case 5:
            GXInitTexObj(&textureObject, bitmap->data, integerValue, integerValue2, GX_TF_RGB5A3, integerValue3, integerValue4, GX_FALSE);
            break;

        case 9:
            if (bitmap->pixSize < 8) {
                GXInitTlutObj(&textureObject2, bitmap->palData, GX_TL_RGB565, bitmap->palSize);
                GXLoadTlut(&textureObject2, textureMapId);
                GXInitTexObjCI(&textureObject, bitmap->data, integerValue, integerValue2, GX_TF_C4, integerValue3, integerValue4, GX_FALSE, textureMapId);
            }
            else {
                GXInitTlutObj(&textureObject2, bitmap->palData, GX_TL_RGB565, bitmap->palSize);
                GXLoadTlut(&textureObject2, textureMapId);
                GXInitTexObjCI(&textureObject, bitmap->data, integerValue, integerValue2, GX_TF_C8, integerValue3, integerValue4, GX_FALSE, textureMapId);
            }
            break;

        case 10:
            if (bitmap->pixSize < 8) {
                GXInitTlutObj(&textureObject2, bitmap->palData, GX_TL_RGB5A3, bitmap->palSize);
                GXLoadTlut(&textureObject2, textureMapId);
                GXInitTexObjCI(&textureObject, bitmap->data, integerValue, integerValue2, GX_TF_C4, integerValue3, integerValue4, GX_FALSE, textureMapId);
            }
            else {
                GXInitTlutObj(&textureObject2, bitmap->palData, GX_TL_RGB5A3, bitmap->palSize);
                GXLoadTlut(&textureObject2, textureMapId);
                GXInitTexObjCI(&textureObject, bitmap->data, integerValue, integerValue2, GX_TF_C8, integerValue3, integerValue4, GX_FALSE, textureMapId);
            }
            break;
    }
    if (model->attr & 0x40) {
        GXInitTexObjLOD(&textureObject, GX_NEAR, GX_NEAR, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);
    }
    else {
        GXInitTexObjLOD(&textureObject, GX_LINEAR, GX_LINEAR, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);
    }
    GXLoadTexObj(&textureObject, textureMapId);
}

#define SET_TEVCOLOR_ALPHA(reg, colorVar, value)                                                                                                    \
    {                                                                                                                                                \
        colorVar.a = (value);                                                                                                                       \
        GXSetTevColor(reg, colorVar);                                                                                                               \
    }

extern u32 texMtxTbl[];

void ConfigureStageMaterial(AnimatedMeshDrawWork *drawWork, HSFMATERIAL *material)
{
    GXColor drawColor;
    GXColor drawColor2;
    GXColor drawColor3;
    u16 animationIndex;
    HSFATTRIBUTE *attribute2;
    u16 updateCounter;
    HSFOBJECT *sceneObject;
    HU3DATTRANIM *hu3dattranimPointer;
    u16 animationIndex2;
    u16 integerValue;
    s32 integerValue2;
    s32 integerValue3;
    u16 integerValue4;
    sceneObject = drawWork->sceneObject;
    if (material->vtxMode == 2 || material->vtxMode == 3) {
        integerValue = 1;
    }
    else {
        integerValue = 0;
        if (material->vtxMode == 0 && !sceneObject->mesh.color) {
            integerValue2 = 0;
        }
        else {
            integerValue2 = 1;
        }
    }

    if (material->attrNum == 1) {
        updateCounter = animationIndex = 1;
        attribute2 = &sceneObject->mesh.attribute[material->attr[0]];
        GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
        if (attribute2->animWorkP) {
            hu3dattranimPointer = attribute2->animWorkP;
            if (hu3dattranimPointer->attr & 0x2) {
                GXLoadTexMtxImm(Hu3DTexScrData[hu3dattranimPointer->texScrId].texMtx, GX_TEXMTX0, GX_MTX2x4);
                GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);
            }
        }
        if (attribute2->originalTextureCoordinates[2] == 0) {
            GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
            GXSetTevOrder(animationIndex, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
            GXSetTevColorIn(animationIndex, GX_CC_CPREV, GX_CC_TEXC, GX_CC_TEXA, GX_CC_ZERO);
            GXSetTevColorOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(animationIndex, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
            GXSetTevAlphaOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            animationIndex++;
        }
        else {
            GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
            GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
            GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        }
        SET_TEVCOLOR_ALPHA(GX_TEVREG0, drawColor, 255 * (1.0f - material->invAlpha));
        if (integerValue) {
            GXSetTevOrder(animationIndex, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR1A1);
            GXSetTevColorIn(animationIndex, GX_CC_CPREV, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
            GXSetTevColorOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(animationIndex, GX_CA_ZERO, GX_CA_APREV, GX_CA_A0, GX_CA_ZERO);
            GXSetTevAlphaOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            animationIndex++;
        }
        else if (0.0f != material->invAlpha) {
            GXSetTevOrder(animationIndex, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
            GXSetTevColorIn(animationIndex, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
            GXSetTevColorOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(animationIndex, GX_CA_ZERO, GX_CA_APREV, GX_CA_RASA, GX_CA_ZERO);
            GXSetTevAlphaOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            animationIndex++;
        }
        GXSetNumTexGens(updateCounter);
        GXSetNumTevStages(animationIndex);
    }
    else {
        integerValue3 = 0;
        updateCounter = 1;
        GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
        for (animationIndex2 = animationIndex = 0; animationIndex2 < material->attrNum; animationIndex++, animationIndex2++) {
            attribute2 = &sceneObject->mesh.attribute[material->attr[animationIndex2]];
            if (0.0f != attribute2->nbtTpLvl) {
                GXSetTexCoordGen(GX_TEXCOORD1, GX_TG_BUMP0, GX_TG_TEXCOORD0, GX_IDENTITY);
                SET_TEVCOLOR_ALPHA(GX_TEVREG1, drawColor2, 10 * attribute2->nbtTpLvl);
                GXSetTevOrder(animationIndex, GX_TEXCOORD0, animationIndex2, GX_COLOR0A0);
                GXSetTevColorIn(animationIndex, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A1, GX_CC_RASC);
                GXSetTevColorOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaIn(animationIndex, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
                GXSetTevAlphaOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
                animationIndex++;
                GXSetTevOrder(animationIndex, GX_TEXCOORD1, animationIndex2, GX_COLOR0A0);
                GXSetTevColorIn(animationIndex, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A1, GX_CC_CPREV);
                GXSetTevColorOp(animationIndex, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaIn(animationIndex, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
                GXSetTevAlphaOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
                updateCounter++;
                integerValue3 = 1;
            }
            else {
                if (attribute2->animWorkP) {
                    hu3dattranimPointer = attribute2->animWorkP;
                    if (hu3dattranimPointer->attr & 0x2) {
                        GXLoadTexMtxImm(Hu3DTexScrData[hu3dattranimPointer->texScrId].texMtx, texMtxTbl[updateCounter], GX_MTX2x4);
                        GXSetTexCoordGen(updateCounter, GX_TG_MTX2x4, GX_TG_TEX0, texMtxTbl[updateCounter]);
                        integerValue4 = (u16)updateCounter;
                        updateCounter++;
                    }
                }
                else {
                    integerValue4 = 0;
                }
                GXSetTevOrder(animationIndex, integerValue4, animationIndex2, GX_COLOR0A0);
                if (animationIndex2 == 0) {
                    GXSetTevColorIn(animationIndex, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
                    GXSetTevAlphaIn(animationIndex, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
                }
                else if (integerValue3 != 0) {
                    GXSetTevColorIn(animationIndex, GX_CC_ZERO, GX_CC_CPREV, GX_CC_TEXC, GX_CC_ZERO);
                    GXSetTevAlphaIn(animationIndex, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
                    integerValue3 = 0;
                }
                else if (attribute2->originalTextureCoordinates[2] == 0) {
                    GXSetTevColorIn(animationIndex, GX_CC_CPREV, GX_CC_TEXC, GX_CC_TEXA, GX_CC_ZERO);
                    GXSetTevAlphaIn(animationIndex, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
                }
                else {
                    SET_TEVCOLOR_ALPHA(GX_TEVREG2, drawColor3, 255 * attribute2->kColor);
                    GXSetTevColorIn(animationIndex, GX_CC_CPREV, GX_CC_TEXC, GX_CC_A2, GX_CC_ZERO);
                    GXSetTevAlphaIn(animationIndex, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
                }
                GXSetTevColorOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            }
        }
        SET_TEVCOLOR_ALPHA(GX_TEVREG0, drawColor, 255 * (1.0f - material->invAlpha));
        if (integerValue) {
            GXSetTevOrder(animationIndex, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR1A1);
            GXSetTevColorIn(animationIndex, GX_CC_CPREV, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
            GXSetTevColorOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(animationIndex, GX_CA_ZERO, GX_CA_APREV, GX_CA_A0, GX_CA_ZERO);
            GXSetTevAlphaOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            animationIndex++;
        }
        else {
            GXSetTevOrder(animationIndex, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
            GXSetTevColorIn(animationIndex, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
            GXSetTevColorOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            GXSetTevAlphaIn(animationIndex, GX_CA_ZERO, GX_CA_APREV, GX_CA_A0, GX_CA_ZERO);
            GXSetTevAlphaOp(animationIndex, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
            animationIndex++;
        }
        GXSetNumTexGens(updateCounter);
        GXSetNumTevStages(animationIndex);
    }
    stageLightHandle = Hu3DLightSet(drawWork->model, &Hu3DCameraMtx, &Hu3DCameraMtxXPose, (integerValue) ? material->hiliteScale : 0);
    if (integerValue) {
        GXSetNumChans(2);
        if (sceneObject->mesh.color) {
            GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, GX_SRC_VTX, stageLightHandle, GX_DF_CLAMP, GX_AF_NONE);
            GXSetChanCtrl(GX_COLOR1A1, GX_TRUE, GX_SRC_REG, GX_SRC_VTX, stageLightHandle, GX_DF_NONE, GX_AF_SPEC);
        }
        else {
            GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, GX_SRC_REG, stageLightHandle, GX_DF_CLAMP, GX_AF_NONE);
            GXSetChanCtrl(GX_COLOR1A1, GX_TRUE, GX_SRC_REG, GX_SRC_REG, stageLightHandle, GX_DF_NONE, GX_AF_SPEC);
        }
    }
    else {
        GXSetNumChans(1);
        if (sceneObject->mesh.color) {
            GXSetChanCtrl(GX_COLOR0A0, integerValue2, GX_SRC_REG, GX_SRC_VTX, stageLightHandle, GX_DF_CLAMP, GX_AF_SPOT);
        }
        else {
            GXSetChanCtrl(GX_COLOR0A0, integerValue2, GX_SRC_REG, GX_SRC_REG, stageLightHandle, GX_DF_CLAMP, GX_AF_SPOT);
        }
    }
}
#undef SET_TEVCOLOR_ALPHA

s32 GetRandomStageValue(void)
{
    stageRandomSeed *= 0x41C64E6D;
    stageRandomSeed += 0x3039;
    return stageRandomSeed >> 16;
}

float ApproachFloat(float currentValue, float targetValue)
{
    float angle;
    float interpolationValue;
    interpolationValue = 360;
    angle = atan2d(currentValue, targetValue);
    if (angle < -180) {
        angle += interpolationValue;
    }
    else if (angle >= 180.0) {
        angle -= interpolationValue;
    }
    return angle;
}

float MeasureVectorDistance(Vec *firstPosition, Vec *secondPosition)
{
    return VECDistanceXYZ(firstPosition, secondPosition);
}
