#include "REL/m438Dll.h"

#include "ext_math.h"
#include "game/audio.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include "string.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

// STRUCT
typedef struct M438DebrisParticle {
    s32 particleActive;
    s32 particleLifetime;
    s32 particleRotation;
    Vec particlePosition;
    f32 particleScale;
    f32 particleGrowthRate;
    f32 particleMaximumScale;
} M438DebrisParticle;

typedef struct M438RockWork {
    s32 rockAvailable;
    s32 rockFalling;
    f32 rockBearing;
    f32 rotationX;
    f32 rotationVelocityX;
    f32 rotationY;
    f32 rotationVelocityY;
    f32 rotationZ;
    f32 rotationVelocityZ;
    Vec localMeshCenter;
    Vec worldPosition;
    Vec linearVelocity;
    HSFOBJECT *meshObject;
} M438RockWork;

typedef struct M438RockSceneWork {
    u32 updateFrame;
    M438RockWork rockStates[32];
    M438DebrisParticle debrisParticles[50];
} M438RockSceneWork;

// BSS
OMOBJ *mapObjects[6];
HU3DPARTICLEDATA *lavaParticles[0x96];
s32 activeRockCount;
s32 rockParticleCount;
s32 activeDebrisCount;

// DATA
char *rockMeshObjectNames[] = { "iwa_s1", "iwa_s2", "iwa_s3", "iwa_s4", "iwa_s5", "iwa_s6", "iwa_s7", "iwa_s8", "iwa_s9", "iwa_s10", "iwa_s11", "iwa_s12",
    "iwa_s13", "iwa_s14", "iwa_s15", "iwa_s16", "iwa_s17", "iwa_s18", "iwa_s19", "iwa_s20", "iwa_s21", "iwa_s22", "iwa_s23", "iwa_s24", "iwa_s25",
    "iwa_s26", "iwa_s27", "iwa_s28", "iwa_s29", "iwa_s30", "iwa_s31", "iwa_s32" };

// PROTO
void InitializeRockScene(OMOBJ *);
void UpdateRockScene(OMOBJ *);
void SpawnLavaParticleBurst(f32);
void UpdateLavaParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void UpdateLavafallParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void UpdateRockDebrisParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void ScaleModelToBounds(Vec *, f32);
void ExtractMatrixRotation(Mtx, Vec *);

void SetupMapEffects(HUPROCESS *process)
{
    OMOBJ *mapObject;

    activeRockCount = 0;
    mapObject = mapObjects[0] = omAddObjEx(process, 0x20, 8, 0, -1, InitializeRockScene);
    mapObject->work[0] = 0;
}

void EmptyMapCallback(void) { }

void InitializeRockScene(OMOBJ *mapObject)
{
    Vec rockCenter;
    Vec maximumVertex;
    Vec minimumVertex;
    s32 particleIndex;
    s32 particleSlot;
    HSFOBJECT *rockMesh;
    s32 particleObjectId;
    M438RockWork *rockState;
    HU3DPARTICLEDATA *particleData;
    M438DebrisParticle *debrisState;
    M438RockSceneWork *mapState;

    mapObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M438RockSceneWork), HU_MEMNUM_OVL);
    mapState = mapObject->data;
    memset(mapState, 0, 0x108C);
    mapObject->mdlId[0] = Hu3DModelCreateFile(0x450000);
    Hu3DModelLayerSet(mapObject->mdlId[0], 0);
    Hu3DModelAttrSet(mapObject->mdlId[0], 0x40000001);
    Hu3DModelShadowMapSet(mapObject->mdlId[0]);
    mapObject->mdlId[1] = Hu3DModelCreateFile(0x450001);
    Hu3DModelLayerSet(mapObject->mdlId[1], 1);
    Hu3DModelPosSet(mapObject->mdlId[1], 0.0f, -80.0f, 0.0f);
    mapObject->mdlId[2] = Hu3DModelCreateFile(0x450002);
    Hu3DModelLayerSet(mapObject->mdlId[2], 0);
    Hu3DModelPosSet(mapObject->mdlId[2], 0.0f, -50.0f, 0.0f);
    Hu3DModelAttrSet(mapObject->mdlId[2], 0x40000001);
    Hu3DMotionSpeedSet(mapObject->mdlId[2], 0.1f);
    particleSlot = Hu3DTexScrollCreate(mapObject->mdlId[1], "lavafall");
    Hu3DTexScrollPosMoveSet(particleSlot, 0.0f, -1.5f * REFRESH_FREQ, 0.0f);
    particleSlot = Hu3DTexScrollCreate(mapObject->mdlId[1], "lafall");
    Hu3DTexScrollPosMoveSet(particleSlot, 0.0f, -REFRESH_FREQ, 0.0f);
    particleSlot = 0;
  
    particleObjectId = Hu3DParticleCreate(HuSprAnimReadFile(0x45000C), 0x32);
    mapObject->mdlId[3] = particleObjectId;
    Hu3DModelLayerSet(particleObjectId, 3);
    Hu3DParticleHookSet(particleObjectId, UpdateLavaParticle);
    for (particleIndex = 0; particleIndex < 0x32; particleIndex++, particleSlot++) {
        lavaParticles[particleSlot] = &((HU3DPARTICLE *)(Hu3DData[particleObjectId].hookData))->data[particleIndex];
    }

    particleObjectId = Hu3DParticleCreate(HuSprAnimReadFile(0x45000D), 0x32);
    mapObject->mdlId[4] = particleObjectId;
    Hu3DModelLayerSet(particleObjectId, 3);
    Hu3DParticleHookSet(particleObjectId, UpdateLavaParticle);
    for (particleIndex = 0; particleIndex < 0x32; particleIndex++, particleSlot++) {
        lavaParticles[particleSlot] = &((HU3DPARTICLE *)(Hu3DData[particleObjectId].hookData))->data[particleIndex];
    }

    particleObjectId = Hu3DParticleCreate(HuSprAnimReadFile(0x45000E), 0x32);
    mapObject->mdlId[5] = particleObjectId;
    Hu3DModelLayerSet(particleObjectId, 3);
    Hu3DParticleHookSet(particleObjectId, UpdateLavaParticle);
    for (particleIndex = 0; particleIndex < 0x32; particleIndex++, particleSlot++) {
        lavaParticles[particleSlot] = &((HU3DPARTICLE *)(Hu3DData[particleObjectId].hookData))->data[particleIndex];
    }
    for (particleIndex = 0; particleIndex < 0x95; particleIndex++) {
        for (particleSlot = 1; particleSlot < 0x96; particleSlot++) {
            if (frandmod(0x3E8) < 0x1C2) {
                particleData = lavaParticles[particleIndex];
                lavaParticles[particleIndex] = lavaParticles[particleSlot];
                lavaParticles[particleSlot] = particleData;
            }
        }
    }

    for (particleIndex = 0; particleIndex < 0x96; particleIndex++) {
        lavaParticles[particleIndex]->scale = 0.0f;
        lavaParticles[particleIndex]->color.a = 0;
        lavaParticles[particleIndex]->time = 0;
    }
  
    particleObjectId = Hu3DParticleCreate(HuSprAnimReadFile(0x45000F), 0x32);
    mapObject->mdlId[6] = particleObjectId;
    Hu3DModelLayerSet(particleObjectId, 3);
    Hu3DParticleHookSet(particleObjectId, UpdateLavafallParticle);
    particleData = &((HU3DPARTICLE *)(Hu3DData[particleObjectId].hookData))->data[0];
    for (particleIndex = 0; particleIndex < 0x32; particleIndex++, particleData++) {
        particleData->scale = 0.0f;
        particleData->color.a = 0;
        particleData->time = 0;
    }
  
    particleObjectId = Hu3DParticleCreate(HuSprAnimReadFile(0x450010), 0x1F4);
    mapObject->mdlId[7] = particleObjectId;
    Hu3DModelLayerSet(particleObjectId, 2);
    Hu3DParticleBlendModeSet(particleObjectId, 1);
    Hu3DParticleHookSet(particleObjectId, UpdateRockDebrisParticle);
    particleData = &((HU3DPARTICLE *)(Hu3DData[particleObjectId].hookData))->data[0];
    for (particleIndex = 0; particleIndex < 0x1F4; particleIndex++, particleData++) {
        particleData->scale = 0.0f;
        particleData->color.a = 0;
        particleData->time = 0;
    }
    rockState = mapState->rockStates;
    for (particleIndex = 0; particleIndex < 0x20; particleIndex++, rockState++) {
        rockState->rockAvailable = 1;
        rockState->rockFalling = 0;
        rockState->rotationX = rockState->rotationVelocityX = 0.0f;
        rockState->rotationY = rockState->rotationVelocityY = 0.0f;
        rockState->rotationZ = rockState->rotationVelocityZ = 0.0f;
        rockMesh = Hu3DModelObjPtrGet(mapObject->mdlId[0], rockMeshObjectNames[particleIndex]);
        rockState->meshObject = rockMesh;
        maximumVertex.x = maximumVertex.y = maximumVertex.z = -100000.0f;
        minimumVertex.x = minimumVertex.y = minimumVertex.z = 100000.0f;

        for (particleSlot = 0; particleSlot < rockMesh->mesh.vertex->count; particleSlot++) {
            if (((Vec *)rockMesh->mesh.vertex->data)[particleSlot].x > maximumVertex.x) {
                maximumVertex.x = ((Vec *)rockMesh->mesh.vertex->data)[particleSlot].x;
            }
            if (((Vec *)rockMesh->mesh.vertex->data)[particleSlot].y > maximumVertex.y) {
                maximumVertex.y = ((Vec *)rockMesh->mesh.vertex->data)[particleSlot].y;
            }
            if (((Vec *)rockMesh->mesh.vertex->data)[particleSlot].z > maximumVertex.z) {
                maximumVertex.z = ((Vec *)rockMesh->mesh.vertex->data)[particleSlot].z;
            }
            if (((Vec *)rockMesh->mesh.vertex->data)[particleSlot].x < minimumVertex.x) {
                minimumVertex.x = ((Vec *)rockMesh->mesh.vertex->data)[particleSlot].x;
            }
            if (((Vec *)rockMesh->mesh.vertex->data)[particleSlot].y < minimumVertex.y) {
                minimumVertex.y = ((Vec *)rockMesh->mesh.vertex->data)[particleSlot].y;
            }
            if (((Vec *)rockMesh->mesh.vertex->data)[particleSlot].z < minimumVertex.z) {
                minimumVertex.z = ((Vec *)rockMesh->mesh.vertex->data)[particleSlot].z;
            }
        }
        VECAdd(&maximumVertex, &minimumVertex, &rockCenter);
        VECScale(&rockCenter, &rockCenter, 0.5f);
        rockState->localMeshCenter = rockCenter;
        rockState->worldPosition = rockState->localMeshCenter;
        rockState->rockBearing = (360.0 + atan2d(rockCenter.x, rockCenter.z));
        if (rockState->rockBearing >= 360.0f) {
            rockState->rockBearing -= 360.0f;
        }
    }
    particleObjectId = Hu3DModelCreateFile(0x45000B);
    Hu3DModelLayerSet(particleObjectId, 2);
    debrisState = mapState->debrisParticles;
    for (particleIndex = 0; particleIndex < 0x32; particleIndex++, debrisState++) {
        debrisState->particleActive = 0;
        debrisState->particleRotation = 0;
        debrisState->particleScale = 0.0f;
        debrisState->particleGrowthRate = debrisState->particleMaximumScale = 0.0f;
        debrisState->particleLifetime = Hu3DModelLink(particleObjectId);
        Hu3DModelLayerSet(debrisState->particleLifetime, 2);
        Hu3DModelAttrSet(debrisState->particleLifetime, 1);
    }
    Hu3DModelKill(particleObjectId);
    mapObject->objFunc = UpdateRockScene;
}

Vec rockMeshCorners[4][2] = {
    {
        { -967.0f, -80.0f, -312.0f },
        { -867.0f, -80.0f, -485.0f },
    },
    {
        { -917.0f, -80.0f, -857.0f },
        { -817.0f, -80.0f, -1030.0f },
    },
    {
        { 967.0f, -80.0f, -312.0f },
        { 867.0f, -80.0f, -485.0f },
    },
    {
        { 917.0f, -80.0f, -857.0f },
        { 817.0f, -80.0f, -1030.0f },
    },
};

void UpdateRockScene(OMOBJ *mapObject)
{
    Mtx rotationMatrix;
    Mtx axisRotationMatrix;
    Vec meshRotation;
    Vec rotationAxis;
    Vec localCenter;
    f32 previousHeight;
    f32 debrisColorScale;
    f32 debrisAngle;
    s32 particleIndex;
    M438RockWork *rockState;
    M438DebrisParticle *debrisParticle;
    M438RockSceneWork *mapState;

    mapState = mapObject->data;
    mapState->updateFrame++;
    rockState = mapState->rockStates;

    for (particleIndex = 0; particleIndex < 0x20; particleIndex++, rockState++) {
        if ((rockState->rockAvailable == 0) && (rockState->rockFalling != 0)) {
            previousHeight = rockState->worldPosition.y;
            VECAdd(&rockState->worldPosition, &rockState->linearVelocity, &rockState->worldPosition);
            rockState->linearVelocity.x *= 0.92f;
            rockState->linearVelocity.z *= 0.92f;
            rockState->linearVelocity.y += -980.0f * REFRESH_FREQ * REFRESH_FREQ;
            if (previousHeight > 0.0) {
                if (rockState->worldPosition.y <= 0.0) {
                    HuAudFXEmiterPlay(0x6FF, &rockState->worldPosition);
                }
            }
            rockState->rotationX += rockState->rotationVelocityX;
            rockState->rotationY += rockState->rotationVelocityY;
            rockState->rotationZ += rockState->rotationVelocityZ;
            rotationAxis.x = sind(90.0f + rockState->rockBearing);
            rotationAxis.z = cosd(90.0f + rockState->rockBearing);
            rotationAxis.y = 0.0f;
            MTXRotAxisRad(rotationMatrix, &rotationAxis, MTXDegToRad(rockState->rotationX));
            rotationAxis.x = rotationAxis.z = 0.0f;
            rotationAxis.y = 1.0f;
            MTXRotAxisRad(axisRotationMatrix, &rotationAxis, MTXDegToRad(rockState->rotationY));
            MTXConcat(axisRotationMatrix, rotationMatrix, rotationMatrix);
            rotationAxis.x = sind(rockState->rockBearing);
            rotationAxis.z = cosd(rockState->rockBearing);
            rotationAxis.y = 0.0f;
            MTXRotAxisRad(axisRotationMatrix, &rotationAxis, MTXDegToRad(rockState->rotationZ));
            MTXConcat(axisRotationMatrix, rotationMatrix, rotationMatrix);
            ExtractMatrixRotation(rotationMatrix, &meshRotation);
            rockState->meshObject->mesh.base.rot.x = meshRotation.x;
            rockState->meshObject->mesh.base.rot.y = meshRotation.y;
            rockState->meshObject->mesh.base.rot.z = meshRotation.z;
            MTXInverse(rotationMatrix, rotationMatrix);
            MTXMultVec(rotationMatrix, &rockState->localMeshCenter, &localCenter);
            rockState->meshObject->mesh.base.pos.x = (rockState->worldPosition.x - localCenter.x);
            rockState->meshObject->mesh.base.pos.y = (rockState->worldPosition.y - localCenter.y);
            rockState->meshObject->mesh.base.pos.z = (rockState->worldPosition.z - localCenter.z);
            if (rockState->worldPosition.y < -500.0f) {
                rockState->rockFalling = 0;
            }
        }
    }
    if ((mapState->updateFrame & 1) == 0) {
        debrisParticle = mapState->debrisParticles;

        for (particleIndex = 0; particleIndex < 0x32; particleIndex++, debrisParticle++) {
            if (debrisParticle->particleActive != 0)
                continue;
            debrisParticle->particleActive = 1;
            debrisParticle->particleScale = frandmod(0x168);
            if (frandmod(0x64) < 0x14) {
                debrisParticle->particlePosition.x = (0.8f * (frandmod(0x7D0) - 0x3E8));
                debrisParticle->particlePosition.y = -80.0f;
                debrisParticle->particlePosition.z = ((-1.2f * frandmod(0x3E8)) - 1000.0f);
            }
            else {
                debrisAngle = frandmod(0x168);
                debrisColorScale = 800.0f + (0.25f * frandmod(0x3E8));
                debrisParticle->particlePosition.x = (debrisColorScale * sind(debrisAngle));
                debrisParticle->particlePosition.y = -80.0f;
                debrisParticle->particlePosition.z = (debrisColorScale * cosd(debrisAngle));
            }
            debrisParticle->particleMaximumScale = (0.1f + (0.0005f * frandmod(0x3E8)));
            debrisParticle->particleGrowthRate = 0.0f;
            Hu3DModelAttrReset(debrisParticle->particleLifetime, 1);
            Hu3DModelPosSetV(debrisParticle->particleLifetime, &debrisParticle->particlePosition);
            Hu3DModelRotSet(debrisParticle->particleLifetime, 0.0f, debrisParticle->particleScale, 0.0f);
            Hu3DModelScaleSet(debrisParticle->particleLifetime, debrisParticle->particleGrowthRate, debrisParticle->particleGrowthRate, debrisParticle->particleGrowthRate);
            break;
        }
    }
    debrisParticle = mapState->debrisParticles;
    for (particleIndex = 0; particleIndex < 0x32; particleIndex++, debrisParticle++) {
        if (debrisParticle->particleActive != 0) {
            debrisParticle->particleGrowthRate += 0.005f;
            if (debrisParticle->particleGrowthRate > debrisParticle->particleMaximumScale) {
                debrisParticle->particleActive = 0;
                Hu3DModelAttrSet(debrisParticle->particleLifetime, 1);
                ScaleModelToBounds(&Hu3DData[debrisParticle->particleLifetime].pos, debrisParticle->particleMaximumScale);
            }
            Hu3DModelScaleSet(debrisParticle->particleLifetime, debrisParticle->particleGrowthRate, debrisParticle->particleGrowthRate, debrisParticle->particleGrowthRate);
        }
    }

    for (particleIndex = 0; particleIndex < 4; particleIndex++) {
        VECSubtract(&rockMeshCorners[particleIndex][1], &rockMeshCorners[particleIndex][0], &rotationAxis);
        debrisAngle = 0.001f * frandmod(0x3E8);
        VECScale(&rotationAxis, &rotationAxis, debrisAngle);
        VECAdd(&rockMeshCorners[particleIndex][0], &rotationAxis, &localCenter);
        VECScale(&localCenter, &localCenter, 0.98f);
        ScaleModelToBounds(&localCenter, 0.03f);
    }
    if ((activeRockCount == 0) && (WipeStatGet() == 0)) {
        HuAudFXPlay(0x6F9);
        HuAudFXPlay(0x6FD);
        activeRockCount = 1;
    }
}

void UpdateLavaParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    u16 unreferencedParticleValue;

    particleData = particle->data;

    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->time != 0) {
            VECAdd(&particleData->pos, &particleData->vel, &particleData->pos);
            particleData->vel.x *= 0.95f;
            particleData->vel.z *= 0.95f;
            particleData->vel.y += -980.0f * REFRESH_FREQ * REFRESH_FREQ;
            if (particleData->time < REFRESH_RATE_F / 2.5f) {
                particleData->color.a = 0.9f * particleData->color.a;
            }
            if (--particleData->time == 0) {
                particleData->time = 0;
                particleData->scale = 0.0f;
                particleData->color.a = 0;
            }
        }
    }
    DCStoreRange(particle->data, particle->maxCnt * 0x44);
}

void UpdateLavafallParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    u16 unreferencedParticleValue;

    particleData = particle->data;

    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->time != 0) {
            VECAdd(&particleData->pos, &particleData->vel, &particleData->pos);
            particleData->vel.x *= 0.92f;
            particleData->vel.z *= 0.92f;
            particleData->scale += 5.0f;
            particleData->color.a *= 0.99f;
            if (particleData->time < REFRESH_RATE_F / 2.5f) {
                particleData->color.a = 0.9f * particleData->color.a;
            }
            if (--particleData->time == 0) {
                particleData->time = 0;
                particleData->scale = 0.0f;
                particleData->color.a = 0;
            }
        }
    }
    DCStoreRange(particle->data, particle->maxCnt * 0x44);
}

void SpawnLavaParticleBurst(f32 bearingAngle)
{
    Vec particleDirection;
    f32 particleAngle;
    f32 particleSpeed;
    f32 particleAngleSpread;
    f32 particleDirectionSpread;
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    u32 remainingParticleCount;

    particleAngleSpread = 20.0f;
    particleDirectionSpread = 7.0f;
    remainingParticleCount = 0x19;
  
    for (particleIndex = 0; particleIndex < 0x96; particleIndex++) {
        particleData = lavaParticles[particleIndex];
        if (particleData->time == 0) {
            particleData->time = REFRESH_RATE_F * (0.6f + (0.0006f * frandmod(0x3E8)));
            particleAngle = ((0.002f * frandmod(0x3E8)) - 1.0f);
            particleAngle = bearingAngle + (particleAngle * particleAngleSpread);
            particleDirection.x = sind(particleAngle);
            particleDirection.y = 0.0f;
            particleDirection.z = cosd(particleAngle);
          
            particleAngle = bearingAngle + (((0.002f * frandmod(0x3E8)) - 1.0f)) * particleDirectionSpread;
            particleData->pos.x = 7.5 * sind(particleAngle) * 100.0;
            particleData->pos.y = 0.2f * frandmod(0x3E8);
            particleData->pos.z = 7.5 * cosd(particleAngle) * 100.0;
          
            particleSpeed = REFRESH_FREQ * 100.0f * (4.0f + (0.014f * frandmod(0x3E8)));
            particleData->vel.x = particleDirection.x * particleSpeed;
            particleData->vel.z = particleDirection.z * particleSpeed;
            particleData->vel.y = particleSpeed * ((0.0006f * frandmod(0x3E8)) - 0.2f);

            particleData->zRot = frandmod(0x168);
            particleData->scale = 10.0f + frandmod(0x32);
            particleData->color.a = 0xFA;
            particleData->color.r = particleData->color.g = particleData->color.b = 0xFF;

            remainingParticleCount--;
            if (remainingParticleCount < 1)
                break;
        }
    }
    particleData = ((HU3DPARTICLE *)(Hu3DData[mapObjects[0]->mdlId[6]].hookData))->data;
    remainingParticleCount = 0xF;
    for (particleIndex = 0; particleIndex < 0x32; particleIndex++, particleData++) {
        if (particleData->time == 0) {
            particleData->time = REFRESH_RATE_F * (0.6f + (0.0006f * frandmod(0x3E8)));
            particleAngle = ((0.002f * frandmod(0x3E8)) - 1.0f);
            particleAngle = bearingAngle + (particleAngle * particleAngleSpread);
            particleDirection.x = sind(particleAngle);
            particleDirection.y = 0.0f;
            particleDirection.z = cosd(particleAngle);
          
            particleAngle = bearingAngle + (((0.002f * frandmod(0x3E8)) - 1.0f)) * particleDirectionSpread;
            particleData->pos.x = 7.5 * sind(particleAngle) * 100.0;
            particleData->pos.y = 0.2f * frandmod(0x3E8);
            particleData->pos.z = 7.5 * cosd(particleAngle) * 100.0;
          
            particleSpeed = REFRESH_FREQ * 100.0f * -(0.01f * frandmod(0x3E8));
            particleData->vel.x = particleDirection.x * particleSpeed;
            particleData->vel.z = particleDirection.z * particleSpeed;
            particleData->vel.y = particleSpeed * -((0.00020000001f * frandmod(0x3E8)) + 0.05f);

            particleData->zRot = frandmod(0x168);
            particleData->scale = 100.0f + frandmod(0x64);
            particleData->color.a = 0x50 + frandmod(0x50);
            particleAngle = frandmod(0x3E8) * 0.001f;
            particleData->color.r = (60.0f * particleAngle) + 80.0f;
            particleData->color.g = (60.0f * particleAngle) + 61.0f;
            particleData->color.b = (60.0f * particleAngle) + 54.0f;

            remainingParticleCount--;
            if (remainingParticleCount < 1)
                break;
        }
    }
}

void UpdateRockDebrisParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    u16 unreferencedParticleValue;

    rockParticleCount = 0;
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->time != 0) {
            VECAdd(&particleData->pos, &particleData->vel, &particleData->pos);
            particleData->vel.y += -980.0f * REFRESH_FREQ * REFRESH_FREQ;
            if (particleData->time < REFRESH_RATE_F / 5.0f) {
                particleData->color.a = (0.8f * particleData->color.a);
            }
            if (--particleData->time == 0) {
                particleData->time = 0;
                particleData->scale = 0.0f;
                particleData->color.a = 0;
            }
            rockParticleCount++;
        }
    }
    DCStoreRange(particle->data, particle->maxCnt * 0x44);
}

void ScaleModelToBounds(Vec *particleOrigin, f32 particleScale)
{
    Vec particleDirection;
    f32 particleAngle;
    f32 particleSpeed;
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    u32 remainingParticleCount;

    particleData = ((HU3DPARTICLE *)(Hu3DData[mapObjects[0]->mdlId[7]].hookData))->data;
    remainingParticleCount = (u32)(3.0f + (15.0f * particleScale));

    for (particleIndex = 0; particleIndex < 0x1F4; particleIndex++, particleData++) {
        if (particleData->time == 0) {
            particleData->time = REFRESH_RATE_F * (0.3f + (0.0003f * frandmod(0x3E8)));
            particleData->pos = *particleOrigin;
            particleData->pos.x += particleScale * (0.07f * (frandmod(0x3E8) - 0x1F4));
            particleData->pos.z += particleScale * (0.07f * (frandmod(0x3E8) - 0x1F4));

            particleAngle = frandmod(0x168);
            particleDirection.x = sind(particleAngle);
            particleDirection.y = 0.0f;
            particleDirection.z = cosd(particleAngle);

            particleSpeed = REFRESH_FREQ * 100.0f * (0.5f + (0.001f * frandmod(0x3E8)));
            particleData->vel.x = particleDirection.x * particleSpeed;
            particleData->vel.z = particleDirection.z * particleSpeed;
            particleData->vel.y = REFRESH_FREQ * 100.0f * ((2.0f * particleScale) + (1.0f + (0.002f * frandmod(0x3E8))));
            particleData->zRot = frandmod(0x168);
            particleData->scale = (20.0f + frandmod(0x14));
            particleData->color.a = (frandmod(0x46) + 0x64);

            particleAngle = 0.001f * frandmod(0x3E8);
            particleData->color.r = 177.0f + (78.0f * particleAngle);
            particleData->color.g = 60.0f + (129.0f * particleAngle);
            particleData->color.b = 30.0f + (122.0f * particleAngle);

            remainingParticleCount--;
            if (remainingParticleCount < 1)
                break;
        }
    }
}

s32 FindAvailableRockForAngle(f32 targetBearing)
{
    M438RockSceneWork *mapState;
    s32 hitDetected;
    s32 rockIndex;
    M438RockWork *rockState;
    float angleDifference;
    float unusedAngle;

    mapState = mapObjects[0]->data;
    hitDetected = 0;
    rockState = mapState->rockStates;
  
    for (rockIndex = 0; rockIndex < 0x20; rockIndex++, rockState++) {
        angleDifference = AngleDifference(rockState->rockBearing, targetBearing);
        if ((fabs(angleDifference) < 12.0) && (rockState->rockAvailable != 0)) {
            rockState->rockAvailable = 0;
            rockState->rockFalling = 1;
            rockState->worldPosition = rockState->localMeshCenter;
            rockState->linearVelocity.y = (REFRESH_FREQ * 100.0f * (2.0f + (0.002f * frandmod(0x3E8))));
            rockState->linearVelocity.x = (REFRESH_FREQ * 100.0f * (sind(rockState->rockBearing) * (10.0f + (0.006f * frandmod(0x3E8)))));
            rockState->linearVelocity.z = (REFRESH_FREQ * 100.0f * (cosd(rockState->rockBearing) * (10.0f + (0.006f * frandmod(0x3E8)))));
            rockState->rotationX = rockState->rotationY = rockState->rotationZ = 0.0f;
            rockState->rotationVelocityX = (REFRESH_FREQ * (10.0f + (0.05f * frandmod(0x3E8))));
            rockState->rotationVelocityY = (REFRESH_FREQ * (30.0f + (0.05f * frandmod(0x3E8))));
            rockState->rotationVelocityZ = (REFRESH_FREQ * (10.0f + (0.02f * frandmod(0x3E8))));
            if (frandmod(0x64) < 0x50) {
                rockState->rotationVelocityX *= -1.0f;
            }
            if (frandmod(0x64) < 0x32) {
                rockState->rotationVelocityY *= -1.0f;
            }
            if (frandmod(0x64) < 0x32) {
                rockState->rotationVelocityZ *= -1.0f;
            }
            SpawnLavaParticleBurst(rockState->rockBearing);
            hitDetected = 1;
        }
    }
    return hitDetected;
}

s32 FindAvailableRockAtAngle(f32 targetBearing)
{
    M438RockSceneWork *mapState;
    s32 rockHit;
    s32 rockIndex;
    M438RockWork *rockState;
    f32 angleDifference;

    mapState = mapObjects[0]->data;
    rockHit = 0;
    rockState = mapState->rockStates;
  
    for (rockIndex = 0; rockIndex < 0x20; rockIndex++, rockState++) {
        angleDifference = AngleDifference(rockState->rockBearing, targetBearing);
        if ((fabs(angleDifference) < 8.0) && (rockState->rockAvailable != 0)) {
            rockHit = 1;
            break;
        }
    }
    return rockHit;
}

f32 Atan2dFromComponents(f32 cosineComponent, f32 sineComponent)
{
    f32 fullRotationDegrees;
    f32 rotationDegrees;

    fullRotationDegrees = 360.0f;
    rotationDegrees = 180.0 * (atan2f(sineComponent, cosineComponent) / M_PI);
    if (rotationDegrees < 0.0f) {
        rotationDegrees += fullRotationDegrees;
      
    }
    else if (rotationDegrees >= fullRotationDegrees) {
        rotationDegrees -= fullRotationDegrees;
    }
    return rotationDegrees;
}

void ExtractMatrixRotation(Mtx rotationMatrix, Vec *rotation)
{
    f32 cosinePitch;
    f32 negativeRotationSine;

    rotation->x = Atan2dFromComponents(rotationMatrix[2][2], rotationMatrix[1][2]);
    rotation->z = Atan2dFromComponents(rotationMatrix[0][0], rotationMatrix[0][1]);
    negativeRotationSine = -rotationMatrix[0][2];
    cosinePitch = sqrtf(fabs(1.0 - (negativeRotationSine * negativeRotationSine)));
    if ((rotation->x > 90.0f) && (rotation->x < 270.0f) && (rotation->z > 90.0f) && (rotation->z < 270.0f)) {
        rotation->x = fmod((180.0f + rotation->x), 360.0);
        rotation->z = fmod((180.0f + rotation->z), 360.0);
        cosinePitch = -cosinePitch;
    }
    rotation->y = Atan2dFromComponents(cosinePitch, negativeRotationSine);
}

f32 ApproachAngle(f32 currentAngle, f32 targetAngle, f32 interpolationFactor)
{
    f32 angleDifference;

    angleDifference = fmodf((targetAngle - currentAngle), 360.0);
    if (angleDifference < 0.0f) {
        angleDifference += 360.0f;
    }
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    angleDifference = fmodf((currentAngle + (interpolationFactor * angleDifference)), 360.0);
    if (angleDifference < 0.0f) {
        angleDifference += 360.0f;
    }
    return angleDifference;
}

f32 AngleDifference(f32 startAngle, f32 endAngle)
{
    f32 angleDifference;

    angleDifference = fmodf((endAngle - startAngle), 360.0);
    if (angleDifference < 0.0f) {
        angleDifference += 360.0f;
    }
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    return angleDifference;
}
