#include "ext_math.h"
#include "game/audio.h"
#include "game/disp.h"
#include "game/hu3d.h"
#include "game/sprite.h"

#include "REL/m417Dll.h"
#include "version.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/frand.h"
#endif

typedef struct WaterParticleState {
    /* 0x00 */ s16 waterColor;
    /* 0x02 */ char padding[4];
    /* 0x06 */ s16 playerIndex;
    /* 0x08 */ Vec currentPosition;
    /* 0x14 */ Vec waveFrequency;
    /* 0x20 */ Float waveHeight;
    /* 0x24 */ Float waveWidth;
    /* 0x28 */ Float wavePhase;
    /* 0x2C */ Float waveState;
    /* 0x30 */ Float reservedWaveData;
    /* 0x34 */ Float travelLimit;
} WaterParticleState; /* size = 0x38 */

typedef struct WaterVertexData {
    /* 0x00 */ Vec waveCenter;
    /* 0x0C */ Float waveOffset;
    /* 0x10 */ Float waveAmplitude;
    /* 0x14 */ Float waveFrequency;
    /* 0x18 */ Float wavePhase;
    /* 0x1C */ s32 waveSampleCount;
    /* 0x20 */ Float *waveHeightSamples;
    /* 0x24 */ u16 *waveVertexGroups;
} WaterVertexData; /* size = 0x28 */

typedef struct WaterWaveState {
    /* 0x00 */ u32 activityState;
    /* 0x04 */ Vec centerPosition;
    /* 0x10 */ Float waveAmplitude;
    /* 0x14 */ Float waveFrequency;
    /* 0x18 */ Float waveRadius;
    /* 0x1C */ Float waveFade;
    /* 0x20 */ Float waveHeight;
    /* 0x24 */ Float waveWidth;
    /* 0x28 */ Float waveScale;
    /* 0x2C */ s32 waveState;
    /* 0x30 */ char reservedWaveData[0x30];
} WaterWaveState; /* size = 0x60 */

typedef struct WaterSurfaceState {
    /* 0x00 */ Float surfaceWaveOffset;
    /* 0x04 */ Float surfaceHeight;
    /* 0x08 */ Float surfaceHeightDelta;
    /* 0x0C */ Vec currentWaterNormal;
    /* 0x18 */ u32 waveVertexCount;
    /* 0x1C */ u16 *waveVertexOrder;
    /* 0x20 */ Float *waveHeightSamples;
    /* 0x24 */ Float *waveVertexWeights;
    /* 0x28 */ s32 *waveVertexFlags;
    /* 0x2C */ Vec *waveBasePositions;
    /* 0x30 */ Vec *waveDisplacements;
    /* 0x34 */ Vec *waveSurfaceNormals;
    /* 0x38 */ u16 (*waveGridIndices)[4];
    /* 0x3C */ WaterVertexData waveVertices[3];
    /* 0xB4 */ WaterWaveState activeWaveStates[16];
    /* 0x6B4 */ s32 waveBufferIndex;
    /* 0x6B8 */ Float waveTime;
    /* 0x6BC */ Vec *waveVertexPositions[2];
    /* 0x6C4 */ Vec *waveVertexNormals[2];
    /* 0x6CC */ GXColor *waveVertexColors[2];
    /* 0x6D4 */ Vec2f *waveTextureCoordinates[2];
    /* 0x6DC */ s32 displayListBufferSize;
    /* 0x6E0 */ void *displayListBuffers[2];
    /* 0x6E8 */ s32 displayListSizes[2];
    /* 0x6F0 */ u32 waterAttributes;
    /* 0x6F4 */ void *waterTextureData;
    /* 0x6F8 */ char reservedWaterState[8];
} WaterSurfaceState; /* size = 0x700 */

Float InterpolateAngle(Float startAngle, Float targetAngle, Float blendFactor);
void UpdateWaterSetup(OMOBJ *object);
void UpdateWaterParticles(OMOBJ *object);
WaterParticleState *CreateWaterParticle(OMOBJ *object, s16 firstModelIndex, s16 particleCount);
void InitializeWaterParticle(OMOBJ *object, WaterParticleState *particles);
void UpdateWaterParticle(OMOBJ *object, WaterParticleState *particles);
void UpdateWaterSurface(OMOBJ *object);
void WaterObjectNoop(OMOBJ *object);
void UpdateWaterSurfaceObject(OMOBJ *object);
void UpdateWaterSurfaceEffects(OMOBJ *object);
void DrawWaterSurface(OMOBJ *object);
void DrawWaterModel(HU3DMODEL *model, Mtx mtx);
void DrawWaterModelShadow(HU3DMODEL *model, Mtx mtx);
void DrawWaterModelParticles(HU3DMODEL *model, Mtx mtx);
void DrawWaterParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx mtx);
void EmitWaterEffect(s32 particleEffectId, u32 effectType, Float directionAngle, Float effectHeight, Float effectScale);

OMOBJ *waterObject;
WaterSurfaceState waterSurfaceState;
WaterContactInfo waterContactInfo[17];
ANIMDATA *waterSplashAnimation;
ANIMDATA *waterRippleAnimation;
ANIMDATA *waterFoamAnimation;
s16 waterEffectId;
char unusedWaterObjectData[0x30]; // unused, might be in main.c

void CreateWaterObjects(HUPROCESS *objectManager)
{
    OMOBJ *waterObject;
    s32 particleIndex;

    waterSplashAnimation = HuSprAnimRead(HuDataReadNum(3145742, HU_MEMNUM_OVL));
    waterRippleAnimation = HuSprAnimRead(HuDataReadNum(3145744, HU_MEMNUM_OVL));
    waterFoamAnimation = HuSprAnimRead(HuDataReadNum(3145743, HU_MEMNUM_OVL));
    waterObject = omAddObjEx(objectManager, 30, 9, 0, -1, UpdateWaterSetup);
    waterObject->work[0] = 0;
    waterObject = waterObject = omAddObjEx(objectManager, 31, 5, 0, -1, UpdateWaterSurface);
    waterObject->work[0] = 0;
    for (particleIndex = 0; particleIndex < 16; particleIndex++) {
        waterContactInfo[particleIndex].waterColor = -1;
    }
    waterContactInfo[particleIndex].waterColor = -2;
}

void DestroyWaterObjects(void)
{
    HuSprAnimKill(waterSplashAnimation);
    HuSprAnimKill(waterRippleAnimation);
    HuSprAnimKill(waterFoamAnimation);
}

void UpdateWaterSetup(OMOBJ *object)
{
    omSetTra(object, 0.0f, 0.0f, 0.0f);
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M417, 0));
    Hu3DModelLayerSet(object->mdlId[0], 0);
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M417, 4));
    Hu3DModelLayerSet(object->mdlId[3], 3);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M417, 2));
    Hu3DModelLayerSet(object->mdlId[2], 5);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_MOTATTR_LOOP);
    Hu3DMotionSpeedSet(object->mdlId[2], 0.7f);
WaterParticleState *CreateWaterParticle(OMOBJ *object, s16 firstModelIndex, s16 particleCount);
    HuAudFXPlay(1480);
    object->objFunc = UpdateWaterParticles;
}

void UpdateWaterParticles(OMOBJ *object)
{
void InitializeWaterParticle(OMOBJ *object, WaterParticleState *particles);
void UpdateWaterParticle(OMOBJ *object, WaterParticleState *particles);
}

WaterParticleState *CreateWaterParticle(OMOBJ *object, s16 firstModelIndex, s16 particleCount)
{
    Float spawnHeight;
    Float spawnAngle;
    Float spawnRadius;

    WaterParticleState *particle; // size 0x38
    WaterParticleState *particles;
    s32 particleIndex;

    particles = HuMemDirectMallocNum(HEAP_HEAP, particleCount * sizeof(WaterParticleState), HU_MEMNUM_OVL);
    memset(particles, 0, particleCount * sizeof(WaterParticleState));
    spawnHeight = -200.0f;
    particle = particles;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particle++, firstModelIndex++) {
        particle->waterColor = firstModelIndex;
        object->mdlId[firstModelIndex] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M417, 3));
        Hu3DModelLayerSet(object->mdlId[particle->waterColor], 0);
        spawnAngle = frandmod(360);
        spawnRadius = frandmod(640);
        particle->currentPosition.x = spawnRadius * sind(spawnAngle);
        particle->currentPosition.y = spawnHeight;
        particle->currentPosition.z = spawnRadius * cosd(spawnAngle);
        particle->waveRadius = frandmod(360);
        particle->reservedWaveData = 1.0f;
        particle->travelLimit = 0.0f;
        spawnHeight -= 80.0f;
        particle->playerIndex = particleCount;
    }
    return particles;
}

void InitializeWaterParticle(OMOBJ *object, WaterParticleState *particles)
{
    Vec movementDelta;

    Float distanceToTarget;
    Float targetRadius;

    s32 particleIndex;
    s32 resetParticle;
    s32 particleCount;

    particleCount = particles->playerIndex;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particles++) {
        resetParticle = 0;
        if (particles->reservedWaveData > particles->travelLimit) {
            resetParticle = 1;
        }
        else {
            VECSubtract(&particles->waveFrequency, &particles->currentPosition, &movementDelta);
            distanceToTarget = VECMag(&movementDelta);
            if (distanceToTarget < 50.0f) {
                resetParticle = 1;
            }
        }
        if (resetParticle) {
            distanceToTarget = frandmod(360);
            targetRadius = 200.0f + frandmod(550);
            particles->waveFrequency.x = targetRadius * sind(distanceToTarget);
            particles->waveFrequency.z = targetRadius * cosd(distanceToTarget);
            particles->waveFrequency.y = particles->currentPosition.y;
            particles->reservedWaveData = 0.0f;
            particles->travelLimit = 200.0f + frandmod(600);
            particles->waveWidth = 0.0f;
        }
        VECSubtract(&particles->waveFrequency, &particles->currentPosition, &movementDelta);
        particles->waveState = atan2d(movementDelta.x, movementDelta.z);
Float InterpolateAngle(Float startAngle, Float targetAngle, Float blendFactor);
        particles->waveWidth += (0.05f * (300.0f - particles->waveWidth));
        particles->waveHeight += (0.2f * (particles->waveWidth - particles->waveHeight));
        particles->currentPosition.x += ((REFRESH_FREQ * particles->waveHeight) * sind(particles->wavePhase));
        particles->currentPosition.z += ((REFRESH_FREQ * particles->waveHeight) * cosd(particles->wavePhase));
        particles->reservedWaveData += REFRESH_FREQ * particles->waveHeight;
    }
}

void UpdateWaterParticle(OMOBJ *object, WaterParticleState *particles)
{
    s32 particleIndex;
    s32 particleCount;

    particleCount = particles->playerIndex;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particles++) {
        Hu3DModelPosSetV(object->mdlId[particles->waterColor], &particles->currentPosition);
        Hu3DModelRotSet(object->mdlId[particles->waterColor], 0.0f, particles->wavePhase, 0.0f);
    }
}

void UpdateWaterSurface(OMOBJ *object)
{
    s16 particleModelId;

    omSetTra(object, 0.0f, 0.0f, 0.0f);
    object->mdlId[0] = Hu3DHookFuncCreate(DrawWaterModel);
    Hu3DModelLayerSet(object->mdlId[0], 2);
    object->mdlId[1] = Hu3DHookFuncCreate(DrawWaterModelShadow);
    Hu3DModelLayerSet(object->mdlId[1], 1);
    object->mdlId[2] = Hu3DHookFuncCreate(DrawWaterModelParticles);
    Hu3DModelLayerSet(object->mdlId[2], 7);
    particleModelId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M417, 17)), 512);
    object->mdlId[3] = particleModelId;
    Hu3DModelLayerSet(particleModelId, 4);
    Hu3DParticleColSet(particleModelId, 255, 255, 255);
    Hu3DParticleHookSet(particleModelId, DrawWaterParticle);
    Hu3DModelPosSet(particleModelId, 0.0f, 0.0f, 0.0f);
    ((HU3DPARTICLE *)Hu3DData[particleModelId].hookData)->dataCnt = 0;
    waterEffectId = particleModelId;
    UpdateWaterSurfaceObject(object);
    object->objFunc = WaterObjectNoop;
}

void WaterObjectNoop(OMOBJ *object) { }

void UpdateWaterSurfaceObject(OMOBJ *object)
{
    Vec vertexDelta;
    Float radialDistance;

    Float vertexDistance;
    Float rowZ;
    Float gridSpacing;
    Float waveDistance;

    s32 rowIndex;
    s32 columnIndex;
    Vec *vertex;
    s32 vertexIndex;
    u16(*gridIndices)[4];
    s32 vertexOrder;

    Vec waveCenters[3] = { { 0.0f, 0.0f, 700.0f }, { -609.0f, 0.0f, -350.0f }, { 609.0f, 0.0f, -350.0f } };
    Vec waveParameters[3] = { { 0.006f, -0.8f * REFRESH_FREQ, 10.8f }, { 0.0045f, -0.6f * REFRESH_FREQ, 9.6f }, { 0.0033f, -0.4f * REFRESH_FREQ, 12.0f } };

    waterSurfaceState.waterColor = 0.0f;
    waterSurfaceState.surfaceHeight = 0.0f;
    waterSurfaceState.surfaceHeightDelta = 0.0f;
    waterSurfaceState.currentWaterNormal.x = 0.0f;
    waterSurfaceState.currentWaterNormal.y = 1.0f;
    waterSurfaceState.currentWaterNormal.z = 0.0f;
    waterSurfaceState.waveHeightSamples = HuMemDirectMallocNum(HEAP_HEAP, 30 * 36 * sizeof(Float), HU_MEMNUM_OVL);
    waterSurfaceState.waveVertexFlags = HuMemDirectMallocNum(HEAP_HEAP, 1080 * sizeof(s32), HU_MEMNUM_OVL);
    waterSurfaceState.waveVertexWeights = HuMemDirectMallocNum(HEAP_HEAP, 1080 * sizeof(Float), HU_MEMNUM_OVL);
    waterSurfaceState.waveVertexOrder = HuMemDirectMallocNum(HEAP_HEAP, 1080 * sizeof(s16), HU_MEMNUM_OVL);
    waterSurfaceState.waveBasePositions = HuMemDirectMallocNum(HEAP_HEAP, 30 * 36 * sizeof(Vec), HU_MEMNUM_OVL);
    waterSurfaceState.waveDisplacements = HuMemDirectMallocNum(HEAP_HEAP, 30 * 36 * sizeof(Vec), HU_MEMNUM_OVL);
    waterSurfaceState.waveSurfaceNormals = HuMemDirectMallocNum(HEAP_HEAP, 30 * 36 * sizeof(Vec), HU_MEMNUM_OVL);
    waterSurfaceState.waveGridIndices = HuMemDirectMallocNum(HEAP_HEAP, 8120, HU_MEMNUM_OVL);
    for (rowIndex = 0; rowIndex < 3; rowIndex++) {
        waterSurfaceState.waveVertices[rowIndex].waveHeight = HuMemDirectMallocNum(HEAP_HEAP, 4320, HU_MEMNUM_OVL);
        waterSurfaceState.waveVertices[rowIndex].waveWidth = HuMemDirectMallocNum(HEAP_HEAP, 2160, HU_MEMNUM_OVL);
    }
    waterSurfaceState.waveBufferIndex = 0;
    waterSurfaceState.displayListBufferSize = 19296;
    for (rowIndex = 0; rowIndex < 2; rowIndex++) {
        waterSurfaceState.waveVertexPositions[rowIndex] = HuMemDirectMallocNum(HEAP_HEAP, 30 * 36 * sizeof(Vec), HU_MEMNUM_OVL);
        waterSurfaceState.waveVertexNormals[rowIndex] = HuMemDirectMallocNum(HEAP_HEAP, 30 * 36 * sizeof(Vec), HU_MEMNUM_OVL);
        waterSurfaceState.waveVertexColors[rowIndex] = HuMemDirectMallocNum(HEAP_HEAP, 1080 * sizeof(GXColor), HU_MEMNUM_OVL);
        waterSurfaceState.waveTextureCoordinates[rowIndex] = HuMemDirectMallocNum(HEAP_HEAP, 30 * 36 * sizeof(Vec2f), HU_MEMNUM_OVL);
        waterSurfaceState.displayListBuffers[rowIndex] = HuMemDirectMallocNum(HEAP_HEAP, waterSurfaceState.displayListBufferSize, HU_MEMNUM_OVL);
        DCFlushRange(waterSurfaceState.displayListBuffers[rowIndex], waterSurfaceState.displayListBufferSize);
        waterSurfaceState.displayListSizes[rowIndex] = 0;
    }
    waterSurfaceState.waterAttributes = GXGetTexBufferSize(640, 480, 5, GX_FALSE, 0);
    waterSurfaceState.waterTextureData = HuMemDirectMallocNum(HEAP_HEAP, waterSurfaceState.waterAttributes, HU_MEMNUM_OVL);
    DCFlushRange(waterSurfaceState.waterTextureData, waterSurfaceState.waterAttributes);
    waterSurfaceState.waveTime = 30.0f;
    vertex = waterSurfaceState.waveBasePositions;
    vertexDistance = 0.0f;
    rowZ = -900.0f;
    gridSpacing = 60.0f;
    for (rowIndex = 0; rowIndex < 36; rowIndex++) {
        vertexDistance = 26.666666f - vertexDistance;
        for (columnIndex = 0; columnIndex < 30; vertex++, columnIndex++) {
            vertex->x = vertexDistance + ((columnIndex * gridSpacing) - 900.0f);
            vertex->y = 0.0f;
            vertex->z = rowZ;
        }
        rowZ += gridSpacing * sind(60);
    }
    vertex = waterSurfaceState.waveBasePositions;
    columnIndex = 0;
    vertexIndex = 1080;
    for (rowIndex = 0; rowIndex < 1080; rowIndex++, vertex++) {
        vertexDistance = VECMag(vertex);
        if (vertexDistance > 850.0f) {
            vertexIndex--;
            waterSurfaceState.waveVertexFlags[rowIndex] = 0;
            waterSurfaceState.waveVertexOrder[rowIndex] = vertexIndex;
            waterSurfaceState.waveVertexWeights[vertexIndex] = 0.0f;
            vertexDistance = 850.0f / vertexDistance;
            VECScale(vertex, vertex, vertexDistance);
        }
        else {
            waterSurfaceState.waveVertexFlags[rowIndex] = 1;
            waterSurfaceState.waveVertexOrder[rowIndex] = columnIndex;
            waterSurfaceState.waveVertexWeights[columnIndex] = 1.0f;
            vertexDistance = 750.0f - vertexDistance;
            if (vertexDistance < 100.0f) {
                vertexDistance *= 0.01f;
                waterSurfaceState.waveVertexWeights[columnIndex] = sqrtf(vertexDistance);
            }
            columnIndex++;
        }
    }
    waterSurfaceState.waveVertexCount = columnIndex;
    vertex = HuMemDirectMallocNum(HEAP_HEAP, 30 * 36 * sizeof(Vec), HU_MEMNUM_OVL);
    for (rowIndex = 0; rowIndex < 30 * 36; rowIndex++) {
        vertex[waterSurfaceState.waveVertexOrder[rowIndex]] = waterSurfaceState.waveBasePositions[rowIndex];
    }
    HuMemDirectFree(waterSurfaceState.waveBasePositions);
    waterSurfaceState.waveBasePositions = vertex;
    memset(waterSurfaceState.waveDisplacements, 0, 30 * 36 * sizeof(Vec));
    gridIndices = waterSurfaceState.waveGridIndices;
    for (rowIndex = 0; rowIndex < 35; rowIndex++) {
        for (columnIndex = 0; columnIndex < 29; gridIndices++, columnIndex++) {
            if ((rowIndex & 1) == 0) {
                vertexIndex = columnIndex + (rowIndex * 30);
                (*gridIndices)[0] = waterSurfaceState.waveVertexOrder[vertexIndex];
                (*gridIndices)[1] = waterSurfaceState.waveVertexOrder[vertexIndex + 30];
                (*gridIndices)[2] = waterSurfaceState.waveVertexOrder[vertexIndex + 1];
                (*gridIndices)[3] = waterSurfaceState.waveVertexOrder[vertexIndex + 31];
            }
            else {
                vertexIndex = columnIndex + (rowIndex * 30);
                (*gridIndices)[1] = waterSurfaceState.waveVertexOrder[vertexIndex];
                (*gridIndices)[3] = waterSurfaceState.waveVertexOrder[vertexIndex + 30];
                (*gridIndices)[0] = waterSurfaceState.waveVertexOrder[vertexIndex + 1];
                (*gridIndices)[2] = waterSurfaceState.waveVertexOrder[vertexIndex + 31];
            }
            rowIndex == 1;
        }
    }
    for (rowIndex = 0; rowIndex < 3; rowIndex++) {
        waterSurfaceState.waveVertices[rowIndex].waterColor = waveCenters[rowIndex];
        waterSurfaceState.waveVertices[rowIndex].waveOffset = 0.0f;
        waterSurfaceState.waveVertices[rowIndex].waveAmplitude = 360.0f * waveParameters[rowIndex].x;
        waterSurfaceState.waveVertices[rowIndex].wavePhase = 360.0f * waveParameters[rowIndex].y;
        waterSurfaceState.waveVertices[rowIndex].waveFrequency = waveParameters[rowIndex].z;
        vertexIndex = -1;
        gridSpacing = 500000.0f;
        vertex = waterSurfaceState.waveBasePositions;
        for (columnIndex = 0; columnIndex < waterSurfaceState.waveVertexCount; columnIndex++) {
            VECSubtract(vertex++, &waterSurfaceState.waveVertices[rowIndex].waterColor, &vertexDelta);
            vertexDistance = VECMag(&vertexDelta);
            if (vertexDistance < gridSpacing) {
                vertexIndex = columnIndex;
                gridSpacing = vertexDistance;
            }
        }
        waterSurfaceState.waveVertices[rowIndex].waterColor = waterSurfaceState.waveBasePositions[vertexIndex];
        vertex = waterSurfaceState.waveBasePositions;
        vertexOrder = 0;
        for (columnIndex = 0; columnIndex < waterSurfaceState.waveVertexCount; columnIndex++) {
            VECSubtract(vertex++, &waterSurfaceState.waveVertices[rowIndex].waterColor, &vertexDelta);
            vertexDistance = waterSurfaceState.waveVertices[rowIndex].waveAmplitude * VECMag(&vertexDelta);
            waterSurfaceState.waveVertices[rowIndex].waveHeight[vertexOrder] = vertexDistance;
            for (vertexIndex = 0; vertexIndex <= vertexOrder; vertexIndex++) {
                rowZ = waterSurfaceState.waveVertices[rowIndex].waveHeight[vertexIndex] - vertexDistance;
                if (fabs(rowZ) < 5.0) {
                    if (vertexIndex == vertexOrder) {
                        vertexOrder++;
                    }
                    break;
                }
            }
            waterSurfaceState.waveVertices[rowIndex].waveWidth[columnIndex] = vertexIndex;
        }
        waterSurfaceState.waveVertices[rowIndex].waveSpeed = vertexOrder;
    }
    for (rowIndex = 0; rowIndex < 2; rowIndex++) {
        memcpy(waterSurfaceState.waveVertexPositions[rowIndex], waterSurfaceState.waveBasePositions, 12960);
        for (columnIndex = 0; columnIndex < 1080; columnIndex++) {
            waterSurfaceState.waveVertexColors[rowIndex][columnIndex].r = 240;
            waterSurfaceState.waveVertexColors[rowIndex][columnIndex].g = 250;
            waterSurfaceState.waveVertexColors[rowIndex][columnIndex].b = 255;
            waterSurfaceState.waveVertexColors[rowIndex][columnIndex].a = 192;
        }
        for (vertexOrder = 0, columnIndex = 0; columnIndex < 36; columnIndex++) {
            for (vertexIndex = 0; vertexIndex < 30; vertexIndex++) {
                vertexOrder = waterSurfaceState.waveVertexOrder[vertexIndex + (columnIndex * 30)];
                waterSurfaceState.waveTextureCoordinates[rowIndex][vertexOrder].x = 0.000625f * waterSurfaceState.waveBasePositions[vertexOrder].x;
                waterSurfaceState.waveTextureCoordinates[rowIndex][vertexOrder].y = 0.000625f * waterSurfaceState.waveBasePositions[vertexOrder].z;
                waterSurfaceState.waveTextureCoordinates[rowIndex][vertexOrder].x *= 3.0f;
                waterSurfaceState.waveTextureCoordinates[rowIndex][vertexOrder].y *= 3.0f;
            }
        }
    }
    UpdateWaterSurfaceEffects(object);
    DrawWaterSurface(object);
}

void SpawnWaterSplash(Float directionAngle, Float splashStrength)
{
    Float waveStrength;
    Float splashScale;

    WaterWaveState *waveState;
    s32 waveIndex;

    if (splashStrength < 0.0f) {
        splashStrength = 0.0f;
    }
    else if (splashStrength > 1.0f) {
        splashStrength = 1.0f;
    }
    waveStrength = 1.0f - splashStrength;
    splashScale = waveStrength;
    splashStrength = 0.5f + (0.5f * splashStrength);
    waveStrength = 0.5f + (0.5f * waveStrength);
    waveState = &waterSurfaceState.activeWaveStates[0];

    for (waveIndex = 0; waveIndex < 16; waveIndex++, waveState++) {
        if (!waveState->activityState) {
            waveState->activityState = 1;
            waveState->centerPosition.x = 700.0 * sind(180.0f + directionAngle);
            waveState->centerPosition.y = 0.0f;
            waveState->centerPosition.z = 700.0 * cosd(180.0f + directionAngle);
            waveState->waveAmplitude = directionAngle;
            waveState->waveFrequency = 520.0f + (80.0f * splashScale);
            waveState->waveFrequency *= REFRESH_FREQ;
            waveState->waveRadius = 800.0f * splashStrength;
            waveState->waveFade = 0.05f;
            waveState->waveHeight = splashStrength;
            waveState->waveWidth = 0.0f;
            waveState->waveScale = 0.2f + (0.2f * waveStrength);
            waveState->waveState = HuAudFXEmiterPlay(1484, &waveState->centerPosition);
            return;
        }
    }
    (void)waveState;
}

void UpdateWaterWaves(OMOBJ *object)
{
    Mtx waveTranslation;
    Mtx waveRotation;
    Mtx waveTransform;
    ROMtx waveTransformRotation;
    Vec waveDirection;
    Vec surfaceToWave;
    Vec waveVelocity;

    Float waveValue;
    Float waveDisplacement;
    Float colorAlpha;
    Float waveFalloff;
    Float waveCurve;

    WaterWaveState *waveState;
    u32 vertexIndex;
    WaterContactInfo *contactInfo;
    GXColor *vertexColors;
    Vec *displacement;
    Vec *surfaceNormal;
    s32 waveIndex;

    Vec origin = { 0.0f, 0.0f, 0.0f };
    vertexColors = waterSurfaceState.waveVertexColors[waterSurfaceState.waveBufferIndex];
    for (vertexIndex = 0; vertexIndex < waterSurfaceState.waveVertexCount; vertexIndex++) {
        vertexColors[vertexIndex].a += -(Float)(vertexColors[vertexIndex].a) * 0.15f;
    }
    contactInfo = &waterContactInfo[0]; // looks like get_water_contact_info but it doesn't get inlined
    waveState = &waterSurfaceState.activeWaveStates[0];

    for (waveIndex = 0; waveIndex < 16; waveIndex++, waveState++, contactInfo++) {
        contactInfo->waterContactIndex = -1;
        if (waveState->activityState != 0) {
            waveVelocity.x = waveState->waveFrequency * sind(waveState->waveAmplitude);
            waveVelocity.y = 0.0f;
            waveVelocity.z = waveState->waveFrequency * cosd(waveState->waveAmplitude);
            VECAdd(&waveState->centerPosition, &waveVelocity, &waveState->centerPosition);
            if (waveState->waveState >= 0) {
                HuAudFXEmiterUpDate(waveState->waveState, &waveState->centerPosition);
            }
            if (VECMag(&waveState->centerPosition) > 800.0f && waveState->activityState >= 2 && (waveState->waveFade -= REFRESH_FREQ * 2, waveState->waveFade < 0.0f)) {
                waveState->activityState = 0;
            }
            else {
                if (waveState->activityState < 2) {
                    VECSubtract(&origin, &waveState->centerPosition, &surfaceToWave);
                    if (VECDotProduct(&waveVelocity, &surfaceToWave) <= 0.0f) {
                        waveState->activityState = 2;
                    }
                    else {
                        waveValue = 1.2f * (VECMag(&surfaceToWave) / (waveState->waveRadius * waveState->waveScale));
                        if (waveValue < 1.2f) {
                            if (waveValue > 1.0f) {
                                waveValue = 5.0f * (1.2f - waveValue);
                            }
                            contactInfo->waterContactIndex = waveIndex;
                            contactInfo->waterHeight = waveState->waveHeight;
                            contactInfo->waveAngle = waveState->waveAmplitude;
                            contactInfo->waveStrength = waveValue;
                        }
                    }
                }
                else if (waveState->activityState == 2 && VECMag(&waveState->centerPosition) > ((800.0f - (100.0f * (2.0f * waveState->waveHeight))) - 30.000002f)) {
                    waveState->activityState = 3;
void EmitWaterEffect(s32 particleEffectId, u32 effectType, Float directionAngle, Float effectHeight, Float effectScale);
                    waveDirection.x = sind(waveState->waveAmplitude);
                    waveDirection.y = 0.0f;
                    waveDirection.z = cosd(waveState->waveAmplitude);
                    VECScale(&waveDirection, &waveDirection, 800.0f);
                    HuAudFXEmiterPlay(1481, &waveDirection);
                    if (waveState->waveState >= 0) {
                        waveState->waveState = -1;
                    }
                }
                waveState->waveFade += REFRESH_FREQ;
                if (waveState->waveFade > 1.0f) {
                    waveState->waveFade = 1.0f;
                }
                if (waveState->activityState >= 2) {
                    waveState->waveWidth += REFRESH_FREQ;
                    if (waveState->waveWidth > 1.0f) {
                        waveState->waveWidth = 1.0f;
                    }
                }
                MTXTrans(waveTranslation, waveState->centerPosition.x, waveState->centerPosition.y, waveState->centerPosition.z);
                MTXRotRad(waveRotation, 89, 0.017453292f * waveState->waveAmplitude);
                MTXConcat(waveTranslation, waveRotation, waveTransform);
                waveValue = waveState->waveRadius * waveState->waveFade;
                MTXScale(waveTranslation, waveValue, waveValue, waveValue * waveState->waveScale);
                MTXConcat(waveTransform, waveTranslation, waveRotation);
                MTXInverse(waveRotation, waveTranslation);
                PSMTXReorder(waveTranslation, waveTransformRotation);
                PSMTXROMultVecArray(waveTransformRotation, waterSurfaceState.waveBasePositions, waterSurfaceState.waveSurfaceNormals, waterSurfaceState.waveVertexCount);
                waveDirection.x = (0.15f * waveState->waveRadius) * (waveState->waveWidth * sind(waveState->waveAmplitude));
                waveDirection.y = 0.0f;
                waveDirection.z = (0.15f * waveState->waveRadius) * (waveState->waveWidth * cosd(waveState->waveAmplitude));
                colorAlpha = 255.0f;
                surfaceNormal = waterSurfaceState.waveSurfaceNormals;
                displacement = waterSurfaceState.waveDisplacements;
                vertexColors = waterSurfaceState.waveVertexColors[waterSurfaceState.waveBufferIndex];
                for (vertexIndex = 0; vertexIndex < waterSurfaceState.waveVertexCount; vertexIndex++, surfaceNormal++, displacement++) {
                    waveValue = VECMag(surfaceNormal);
                    if (!(waveValue >= 1.0f)) {
                        waveDisplacement = (1.0f - waveValue) * waveState->waveFade;
                        waveFalloff = waveState->waveWidth * ((1.0f - waveValue) * (1.0f - waveValue));
                        waveCurve = ((1.0f - waveValue) * (1.0f - (waveValue * waveValue))) + (waveValue * ((1.0f - waveValue) * (1.0f - waveValue)));
                        displacement->x += waterSurfaceState.waveVertexWeights[vertexIndex] * (waveDirection.x * waveDisplacement);
                        displacement->y += waterSurfaceState.waveVertexWeights[vertexIndex]
                            * (((waveDirection.y * waveDisplacement) + (waveState->waveFade * (0.3f * waveState->waveRadius * waveCurve))) - (0.3f * waveState->waveRadius * waveFalloff));
                        displacement->z += waterSurfaceState.waveVertexWeights[vertexIndex] * (waveDirection.z * waveDisplacement);
                        if (surfaceNormal->z >= -0.2f) {
                            waveValue = vertexColors[vertexIndex].a + (colorAlpha * (1.0f - (waveValue * (waveValue * waveValue))));
                            if (waveValue > colorAlpha) {
                                waveValue = colorAlpha;
                            }
                            vertexColors[vertexIndex].a = waveValue;
                        }
                    }
                }
            }
        }
    }
    contactInfo->waterContactIndex = -2;
}

void UpdateWaterSurfaceEffects(OMOBJ *object)
{
    Vec waveDirection;
    Vec surfaceNormal;
    Vec edgeVector;
    Vec normalCrossProduct;

    Float waveHeight;

    u16(*waveIndices)[4];
    s32 triangleIndex;
    WaterVertexData *vertexData;
    Vec *vertexPositions;
    Vec *vertexDisplacements;
    u32 vertexIndex;
    Vec *vertexNormals;
    Vec *waterPositions;
    Vec *waterDisplacements;

    waterSurfaceState.waveBufferIndex = 1 - waterSurfaceState.waveBufferIndex;
    UpdateWaterWaves(object);
    waterSurfaceState.waveTime += 5 * REFRESH_FREQ;
    if (waterSurfaceState.waveTime >= 360.0f) {
        waterSurfaceState.waveTime -= 360.0f;
    }
    vertexData = waterSurfaceState.waveVertices;
    for (triangleIndex = 0; triangleIndex < 3; triangleIndex++, vertexData++) {
        vertexData->waveOffset += vertexData->wavePhase;
        if (vertexData->waveOffset >= 360.0f) {
            vertexData->waveOffset -= 360.0f;
        }
        else if (vertexData->waveOffset < 0.0f) {
            vertexData->waveOffset += 360.0f;
        }
        waveHeight = vertexData->waveOffset;
        for (vertexIndex = 0; vertexIndex < vertexData->waveSampleCount; vertexIndex++) {
            waterSurfaceState.waveHeightSamples[vertexIndex] = vertexData->waveFrequency * sind((waveHeight + vertexData->waveHeight[vertexIndex]));
        }
        vertexDisplacements = waterSurfaceState.waveDisplacements;
        for (vertexIndex = 0; vertexIndex < waterSurfaceState.waveVertexCount; vertexIndex++, vertexDisplacements++) {
            vertexDisplacements->y = vertexDisplacements->y + waterSurfaceState.waveHeightSamples[vertexData->waveVertexGroups[vertexIndex]];
        }
    }
    waterDisplacements = waterSurfaceState.waveBasePositions;
    waterPositions = waterSurfaceState.waveDisplacements;
    vertexNormals = waterSurfaceState.waveVertexPositions[waterSurfaceState.waveBufferIndex];
    for (triangleIndex = 0; triangleIndex < waterSurfaceState.waveVertexCount; waterDisplacements++, waterPositions++, vertexNormals++, triangleIndex++) {
        VECAdd(waterDisplacements, waterPositions, vertexNormals);
    }
    memset(waterSurfaceState.waveDisplacements, 0, 30 * 36 * sizeof(Vec));
    vertexPositions = waterSurfaceState.waveVertexNormals[waterSurfaceState.waveBufferIndex];
    memset(vertexPositions, 0, 30 * 36 * sizeof(Vec));
    vertexDisplacements = waterSurfaceState.waveVertexPositions[waterSurfaceState.waveBufferIndex];
    waveIndices = waterSurfaceState.waveGridIndices;
    for (triangleIndex = 0; triangleIndex < 1015; waveIndices++, triangleIndex++) {
        VECSubtract(&vertexDisplacements[(*waveIndices)[1]], &vertexDisplacements[(*waveIndices)[0]], &waveDirection);
        VECSubtract(&vertexDisplacements[(*waveIndices)[3]], &vertexDisplacements[(*waveIndices)[0]], &surfaceNormal);
        VECSubtract(&vertexDisplacements[(*waveIndices)[2]], &vertexDisplacements[(*waveIndices)[0]], &edgeVector);
        VECCrossProduct(&waveDirection, &surfaceNormal, &normalCrossProduct);
        VECNormalize(&normalCrossProduct, &normalCrossProduct);
        VECAdd(&normalCrossProduct, &vertexPositions[(*waveIndices)[0]], &vertexPositions[(*waveIndices)[0]]);
        VECAdd(&normalCrossProduct, &vertexPositions[(*waveIndices)[1]], &vertexPositions[(*waveIndices)[1]]);
        VECAdd(&normalCrossProduct, &vertexPositions[(*waveIndices)[3]], &vertexPositions[(*waveIndices)[3]]);
        VECCrossProduct(&surfaceNormal, &edgeVector, &normalCrossProduct);
        VECNormalize(&normalCrossProduct, &normalCrossProduct);
        VECAdd(&normalCrossProduct, &vertexPositions[(*waveIndices)[0]], &vertexPositions[(*waveIndices)[0]]);
        VECAdd(&normalCrossProduct, &vertexPositions[(*waveIndices)[3]], &vertexPositions[(*waveIndices)[3]]);
        VECAdd(&normalCrossProduct, &vertexPositions[(*waveIndices)[2]], &vertexPositions[(*waveIndices)[2]]);
    }
    for (triangleIndex = 0; triangleIndex < 1080; triangleIndex++, vertexPositions++) {
        VECNormalize(vertexPositions, vertexPositions);
    }
    vertexNormals = waterSurfaceState.waveVertexPositions[waterSurfaceState.waveBufferIndex];
    for (triangleIndex = 0; triangleIndex < waterSurfaceState.waveVertexCount; vertexNormals++, triangleIndex++) {
        waveHeight = 0.5f - (0.00041666668f * vertexNormals->y);
        vertexNormals->y = 1600.0f * (0.25f - (waveHeight * waveHeight));
    }
    triangleIndex = waterSurfaceState.waveVertexOrder[555];
    waveHeight = waterSurfaceState.waveVertexPositions[waterSurfaceState.waveBufferIndex][triangleIndex].y;
    waterSurfaceState.surfaceHeightDelta = waveHeight - waterSurfaceState.surfaceHeight;
    waterSurfaceState.surfaceHeight = waveHeight;
    waterSurfaceState.currentWaterNormal = waterSurfaceState.waveVertexNormals[waterSurfaceState.waveBufferIndex][triangleIndex];

    DCStoreRangeNoSync(waterSurfaceState.waveVertexPositions[waterSurfaceState.waveBufferIndex], 30 * 36 * sizeof(Vec));
    DCStoreRangeNoSync(waterSurfaceState.waveVertexNormals[waterSurfaceState.waveBufferIndex], 30 * 36 * sizeof(Vec));
    DCStoreRangeNoSync(waterSurfaceState.waveVertexColors[waterSurfaceState.waveBufferIndex], 1080 * sizeof(GXColor));
    DCStoreRangeNoSync(waterSurfaceState.waveTextureCoordinates[waterSurfaceState.waveBufferIndex], 30 * 36 * sizeof(Vec2f));
    PPCSync();
    (void)triangleIndex;
    (void)triangleIndex;
}

void DrawWaterSurface(OMOBJ *object)
{
    s32 rowIndex;
    s32 columnIndex;
    s32 vertexIndex;

    GXBeginDisplayList(waterSurfaceState.displayListBuffers[0], waterSurfaceState.displayListBufferSize);
    for (vertexIndex = 0; vertexIndex < 35; vertexIndex++) {
        if ((vertexIndex & 1) == 0) {
            GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 60);
            for (columnIndex = 0; columnIndex < 30; columnIndex++) {
                rowIndex = waterSurfaceState.waveVertexOrder[columnIndex + ((vertexIndex + 1) * 30)];
                GXPosition1x16(rowIndex);
                GXNormal1x16(rowIndex);
                GXColor1x16(rowIndex);
                GXTexCoord1x16(rowIndex);
                rowIndex = waterSurfaceState.waveVertexOrder[columnIndex + (vertexIndex * 30)];
                GXPosition1x16(rowIndex);
                GXNormal1x16(rowIndex);
                GXColor1x16(rowIndex);
                GXTexCoord1x16(rowIndex);
            }
        }
        else {
            GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 59);
            for (columnIndex = 29; columnIndex >= 0; columnIndex--) {
                rowIndex = waterSurfaceState.waveVertexOrder[columnIndex + (vertexIndex * 30)];
                GXPosition1x16(rowIndex);
                GXNormal1x16(rowIndex);
                GXColor1x16(rowIndex);
                GXTexCoord1x16(rowIndex);
                if (!columnIndex) {
                    break;
                }
                rowIndex = waterSurfaceState.waveVertexOrder[((vertexIndex + 1) * 30) + columnIndex - 1];
                GXPosition1x16(rowIndex);
                GXNormal1x16(rowIndex);
                GXColor1x16(rowIndex);
                GXTexCoord1x16(rowIndex);
            }
        }
    }
    waterSurfaceState.displayListSizes[0] = GXEndDisplayList();
}

void DrawWaterModel(HU3DMODEL *model, Mtx modelMatrix)
{
    Mtx textureMatrix;
    Mtx waterMatrix;
    Mtx modelMatrix;
    GXTexObj textureObject;

    GXColor *waterColor;
    GXColor *highlightColor;

    GXColor waterColor = { 0xD8, 0xE2, 0xE5, 0x4C };
    GXColor highlightColor = { 0xE0, 0xD8, 0xF4, 0x40 };
    DCStoreRangeNoSync(&waterColor, 4);
    DCStoreRangeNoSync(&highlightColor, 4);
    PPCSync();
    GXLoadPosMtxImm(modelMatrix, 0);
    MTXInvXpose(modelMatrix, textureMatrix);
    GXLoadNrmMtxImm(textureMatrix, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXInitTexObj(&textureObject, waterSurfaceState.waterTextureData, 640, 480, 5, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObjLOD(&textureObject, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXLoadTexObj(&textureObject, GX_TEXMAP0);
    HuSprTexLoad(waterSplashAnimation, 0, 1, GX_CLAMP, GX_CLAMP, GX_LINEAR);
    HuSprTexLoad(waterFoamAnimation, 0, 2, GX_CLAMP, GX_CLAMP, GX_LINEAR);
    HuSprTexLoad(waterRippleAnimation, 0, 3, GX_MIRROR, GX_MIRROR, GX_LINEAR);
    GXSetNumTexGens(5);
    C_MTXLightPerspective(modelMatrix, 35.0f, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, textureMatrix);
    MTXConcat(textureMatrix, modelMatrix, textureMatrix);
    MTXConcat(modelMatrix, Hu3DCameraMtx, waterMatrix);
    MTXConcat(waterMatrix, textureMatrix, waterMatrix);
    waterMatrix[1][3] -= 44.0f;
    GXLoadTexMtxImm(waterMatrix, 30, GX_MTX3x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, 30, GX_FALSE, 125);
    C_MTXLightPerspective(modelMatrix, 70.0f, 1.0f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, textureMatrix);
    MTXConcat(textureMatrix, modelMatrix, textureMatrix);
    MTXConcat(modelMatrix, Hu3DCameraMtx, waterMatrix);
    MTXConcat(waterMatrix, textureMatrix, waterMatrix);
    GXLoadTexMtxImm(waterMatrix, 36, GX_MTX3x4);
    GXSetTexCoordGen2(GX_TEXCOORD2, GX_TG_MTX3x4, GX_TG_POS, 36, GX_FALSE, 125);
    MTXRotRad(waterMatrix, 90, 0.017453292f * waterSurfaceState.waveTime);
    GXLoadTexMtxImm(waterMatrix, 39, GX_MTX2x4);
    GXSetTexCoordGen2(GX_TEXCOORD3, GX_TG_MTX2x4, GX_TG_TEXCOORD0, 39, GX_FALSE, 125);
    MTXScale(textureMatrix, 1.2f, -1.2f, 1.2f);
    MTXConcat(waterMatrix, textureMatrix, waterMatrix);
    GXLoadTexMtxImm(waterMatrix, 42, GX_MTX2x4);
    GXSetTexCoordGen2(GX_TEXCOORD4, GX_TG_MTX2x4, GX_TG_TEXCOORD0, 42, GX_FALSE, 125);
    MTXScale(textureMatrix, 0.49f, -0.49f, 0.49f);
    MTXConcat(textureMatrix, Hu3DCameraMtx, waterMatrix);
    waterMatrix[0][3] = 0.5f;
    waterMatrix[1][3] = 0.5f;
    GXLoadTexMtxImm(waterMatrix, 33, GX_MTX2x4);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_NRM, 33, GX_FALSE, 125);
    GXSetNumIndStages(3);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD1, GX_TEXMAP1);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
    GXSetTevIndWarp(GX_TEVSTAGE0, GX_INDTEXSTAGE0, GX_TRUE, GX_FALSE, GX_ITM_0);
    MTXScale(waterMatrix, -0.5f, -0.5f, 0.5f);
    GXSetIndTexMtx(GX_ITM_0, (Float(*)[3])waterMatrix, -2);
    GXSetIndTexOrder(GX_INDTEXSTAGE1, GX_TEXCOORD1, GX_TEXMAP1);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE1, GX_ITS_1, GX_ITS_1);
    GXSetTevIndWarp(GX_TEVSTAGE1, GX_INDTEXSTAGE1, GX_TRUE, GX_FALSE, GX_ITM_1);
    MTXScale(waterMatrix, 0.5f, 0.5f, 0.5f);
    GXSetIndTexMtx(GX_ITM_1, (Float(*)[3])waterMatrix, 0);
    GXSetIndTexOrder(GX_INDTEXSTAGE2, GX_TEXCOORD1, GX_TEXMAP1);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE2, GX_ITS_1, GX_ITS_1);
    GXSetTevIndWarp(GX_TEVSTAGE2, GX_INDTEXSTAGE2, GX_TRUE, GX_FALSE, GX_ITM_2);
    GXSetTevIndWarp(GX_TEVSTAGE3, GX_INDTEXSTAGE2, GX_TRUE, GX_FALSE, GX_ITM_2);
    MTXScale(waterMatrix, -0.65f, -0.65f, 0.65f);
    GXSetIndTexMtx(GX_ITM_2, (Float(*)[3])waterMatrix, -3);
    GXSetNumTevStages(5);
    GXSetTevColor(GX_TEVREG0, waterColor);
    GXSetTevColor(GX_TEVREG1, highlightColor);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD2, GX_TEXMAP2, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A0, GX_CC_CPREV);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD3, GX_TEXMAP3, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A1, GX_CC_CPREV);
    GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD4, GX_TEXMAP3, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE3, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASA, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG2);
    GXSetTevAlphaIn(GX_TEVSTAGE3, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVREG2);
    GXSetTevOrder(GX_TEVSTAGE4, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE4, GX_CC_CPREV, GX_CC_C1, GX_CC_C2, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE4, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE4, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE4, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_POS, waterSurfaceState.waveVertexPositions[waterSurfaceState.waveBufferIndex], 12);
    GXSetArray(GX_VA_NRM, waterSurfaceState.waveVertexNormals[waterSurfaceState.waveBufferIndex], 12);
    GXSetArray(GX_VA_CLR0, waterSurfaceState.waveVertexColors[waterSurfaceState.waveBufferIndex], 4);
    GXSetArray(GX_VA_TEX0, waterSurfaceState.waveTextureCoordinates[waterSurfaceState.waveBufferIndex], 8);
    GXCallDisplayList(waterSurfaceState.displayListBuffers[0], waterSurfaceState.displayListSizes[0]);
    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevDirect(GX_TEVSTAGE1);
    GXSetTevDirect(GX_TEVSTAGE2);
    GXSetTevDirect(GX_TEVSTAGE3);
}

void DrawWaterModelShadow(HU3DMODEL *model, Mtx modelMatrix)
{
    GXSetTexCopySrc(0, 0, 640, 480);
    GXSetTexCopyDst(640, 480, GX_TF_RGB5A3, GX_FALSE);
    GXCopyTex(waterSurfaceState.waterTextureData, GX_FALSE);
    GXPixModeSync();
}

void DrawWaterModelParticles(HU3DMODEL *model, Mtx modelMatrix)
{
    if (!omPauseChk()) {
        UpdateWaterSurfaceEffects(NULL);
    }
}

void DrawWaterParticle(HU3DMODEL *waterModel, HU3DPARTICLE *particleEffect, Float (*particleMatrix)[4])
{
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;

    if (!particleEffect->dataCnt) {
        particleEffect->dataCnt++;
        particleData = particleEffect->data;
        for (particleIndex = 0; particleIndex < particleEffect->maxCnt; particleIndex++, particleData++) {
            particleData->time = 0;
            particleData->scale = 0.0f;
            particleData->color.a = 0;
        }
        return;
    }
    particleData = particleEffect->data;
    for (particleIndex = 0; particleIndex < particleEffect->maxCnt; particleIndex++, particleData++) {
        if (particleData->time) {
            particleData->pos.x += particleData->vel.x;
            particleData->pos.y += particleData->vel.y;
            particleData->pos.z += particleData->vel.z;
            particleData->vel.y += -980.0f * REFRESH_FREQ * REFRESH_FREQ;
            if (particleData->time < REFRESH_RATE_F / 5) {
                particleData->color.a = 0.88f * particleData->color.a;
            }
            if (!(--particleData->time)) {
                particleData->time = 0;
                particleData->scale = 0.0f;
                particleData->color.a = 0;
            }
        }
    }
    DCStoreRange(particleEffect->data, particleEffect->maxCnt * 68);
}

void EmitWaterEffect(s32 particleEffectId, u32 effectType, Float directionAngle, Float effectHeight, Float effectScale)
{
    Vec effectPosition;

    Float effectSize;
    Float effectAngle;
    Float effectRadius;
    Float effectHeightOffset;

    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleSystem;
    s32 particleIndex;
    u32 particleIndex;

    particleSystem = Hu3DData[particleEffectId].hookData;
    {
        Float particleScale[2] = { 800.0f, 150.0f };
        if (!particleSystem->dataCnt) {
            return;
        }
        particleData = particleSystem->data;
        if (effectType) {
            particleIndex = 10.0f + (20.0f * effectScale);
            effectScale *= 0.7f;
            directionAngle += 180.0f;
            effectHeight = 60.0f;
            effectHeightOffset = GetWaterHeight();
        }
        else {
            particleIndex = 20.0f + (80.0f * effectScale);
            effectHeightOffset = 0.0f;
        }
        for (particleIndex = 0; particleIndex < particleSystem->maxCnt; particleIndex++, particleData++) {
            if (particleData->time) {
                continue;
            }
            particleData->time = REFRESH_RATE_F * (1.0f + (0.0005f * frandmod(1000)));
            effectAngle = 0.001f * frandmod(1000);
            effectRadius = effectAngle;
            if (frandmod(1000) < 500) {
                effectRadius *= -1.0f;
            }
            effectAngle = effectScale * (0.4f + (0.6f * (1.0f - (effectAngle * effectAngle))));
            effectSize = directionAngle + (effectRadius * effectHeight);
            effectPosition.x = sind(effectSize);
            effectPosition.y = 0.0f;
            effectPosition.z = cosd(effectSize);
            particleData->pos.x = effectPosition.x * particleScale[effectType];
            particleData->pos.y = (effectHeightOffset + (0.15f * frandmod(1000))) - 100.0f;
            particleData->pos.z = effectPosition.z * particleScale[effectType];
            particleData->vel.x = REFRESH_FREQ * (((100.0f * (1.5f * effectPosition.x)) + (0.1f * frandmod(1000))) - 50.0f);
            particleData->vel.y = REFRESH_FREQ * (450.0f + (0.5f * frandmod(1000)));
            particleData->vel.z = REFRESH_FREQ * (((100.0f * (1.5f * effectPosition.z)) + (0.1f * frandmod(1000))) - 50.0f);
            particleData->vel.x *= effectAngle;
            particleData->vel.y *= 0.2f + (0.8f * effectAngle);
            particleData->vel.z *= effectAngle;
            particleData->zRot = frandmod(360);
            effectSize = 0.001f * frandmod(1000);
            if (effectType) {
                particleData->scale = 50.0f + frandmod(50);
                particleData->color.a = frandmod(80) + 60;
                particleData->color.r = 200.0f + (55.0f * effectSize);
                particleData->color.g = 220.0f + (20.0f * effectSize);
                particleData->color.b = 245.0f + (10.0f * effectSize);
            }
            else {
                particleData->scale = 100.0f + frandmod(100);
                particleData->color.a = frandmod(100) + 80;
                particleData->color.r = 200.0f + (55.0f * effectSize);
                particleData->color.g = 220.0f + (20.0f * effectSize);
                particleData->color.b = 245.0f + (10.0f * effectSize);
            }
            particleIndex--;
            if (!particleIndex) {
                break;
            }
        }
    }
    (void)particleIndex;
}

void SetWaterWave(Float worldX, Float worldY, Float worldZ, Float waveStrength)
{
    Float waveAngle;
    Float waveMagnitude;
    Float waveDistance;

    s32 waveIndex;
    s32 activeWaveIndex;

    waveAngle = 180.0 + (atan2d(worldX, worldZ));
    if (waveAngle >= 360.0f) {
        waveAngle -= 360.0f;
    }
    SpawnWaterSplash(waveAngle, waveStrength);
}

Float GetWaterHeight(void)
{
    return 10.0f + (waterSurfaceState.surfaceHeight + waterObject->trans.y);
}

Float GetWaterCurrentPosition(void)
{
    return waterSurfaceState.surfaceHeightDelta;
}

void GetWaterWaveAmplitude(Vec *waveAmplitude)
{
    *waveAmplitude = waterSurfaceState.currentWaterNormal;
}

s32 FindNearestWaterVertex(Float worldX, Float worldY, Float worldZ)
{
    Float candidateDistance;
    Float offsetZ;
    Float nearestDistance;

    s32 firstRow;
    s32 firstColumn;
    s32 rowIndex;
    s32 columnEnd;
    s32 rowEnd;
    s32 vertexIndex;
    s32 nearestVertexIndex;

    candidateDistance = waterSurfaceState.waveBasePositions[waterSurfaceState.waveVertexOrder[555]].x - waterSurfaceState.waveBasePositions[waterSurfaceState.waveVertexOrder[554]].x;
    firstColumn = (15.0f + (s32)(worldX / candidateDistance)) - 1.0f;
    columnEnd = firstColumn + 2;
    if (firstColumn < 0) {
        firstColumn = 0;
    }
    if (columnEnd > 30) {
        columnEnd = 30;
    }
    candidateDistance = waterSurfaceState.waveBasePositions[waterSurfaceState.waveVertexOrder[555]].z - waterSurfaceState.waveBasePositions[waterSurfaceState.waveVertexOrder[525]].z;
    firstRow = (18.0f + (s32)(worldZ / candidateDistance)) - 1.0f;
    rowEnd = firstRow + 2;
    if (firstRow < 0) {
        firstRow = 0;
    }
    if (rowEnd > 36) {
        rowEnd = 36;
    }
    nearestDistance = 1e10f;
    nearestVertexIndex = -1;
    for (; firstRow < rowEnd; firstRow++) {
        for (rowIndex = firstColumn; rowIndex < columnEnd; rowIndex++) {
            vertexIndex = waterSurfaceState.waveVertexOrder[rowIndex + (firstRow * 30)];
            candidateDistance = worldX - waterSurfaceState.waveBasePositions[vertexIndex].x;
            offsetZ = worldZ - waterSurfaceState.waveBasePositions[vertexIndex].z;
            candidateDistance = (candidateDistance * candidateDistance) + (offsetZ * offsetZ);
            if (candidateDistance < nearestDistance) {
                nearestDistance = candidateDistance;
                nearestVertexIndex = vertexIndex;
            }
        }
    }
    return nearestVertexIndex;
}

Float GetWaterHeightAtPosition(Float worldX, Float worldY, Float worldZ)
{
    s32 vertexIndex;

    vertexIndex = FindNearestWaterVertex(worldX, worldY, worldZ);
    if (vertexIndex < 0) {
        return 0.0f;
    }
    return 25.0f + (waterObject->trans.y + waterSurfaceState.waveVertexPositions[waterSurfaceState.waveBufferIndex][vertexIndex].y);
}

void GetWaterNormalAtPosition(Float worldX, Float worldY, Float worldZ, Vec *surfaceNormal)
{
    s32 vertexIndex;

    vertexIndex = FindNearestWaterVertex(worldX, worldY, worldZ);
    if (vertexIndex < 0) {
        surfaceNormal->x = surfaceNormal->z = 0.0f;
        surfaceNormal->y = 1.0f;
    }
    else {
        *surfaceNormal = waterSurfaceState.waveVertexNormals[waterSurfaceState.waveBufferIndex][vertexIndex];
    }
}

WaterContactInfo *GetWaterContactInfo(void)
{
    return &waterContactInfo[0];
}
