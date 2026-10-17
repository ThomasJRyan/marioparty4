#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXStruct.h"
#include "dolphin/gx/GXVert.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/flag.h"
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

#include "REL/m430Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/frand.h"

void GXSetTevIndWarp(GXTevStageID tevStage, GXIndTexStageID indStage, GXBool signedOffsets, GXBool replaceMode, GXIndTexMtxID matrixSel);
#endif

inline double Fabs2(double x)
{
    return (double)fabs(x);
}

typedef struct M430WaterParticleState {
    s32 ownerIndex;
    s16 modelId;
    char reservedParticleMetadata[2];
    s16 particleIndex;
    s16 effectIndex;
    Vec position;
    Vec targetPosition;
    float speed;
    float targetSpeed;
    float rotationY;
    float targetRotationY;
    float travelDistance;
    float maxTravelDistance;
} M430WaterParticleState; /* size 0x3C */

typedef struct M430WaterSystemWork {
    s32 cameraIndex;
    Vec cameraPosition;
    s32 waterSurfaceModelId;
    s32 underwaterModelId;
    s16 waterModelId;
    s16 waveParticleModelId;
    s16 waveTextureScrollId;
    s16 foamTextureScrollId;
    float waterDepth;
    s16 splashModelIds[0xC];
    s32 splashIndex;
    s16 leftWaveModelIds[0xA];
    s16 centerWaveModelIds[0xA];
    s16 rightWaveModelIds[0xA];
    s32 foamIndex;
    s16 foamModelIds[6];
    s16 foamOverlayModelIds[6];
    s16 foamEffectModelIds[6];
    s32 foamActiveFlags[6];
    s32 foamEffectIndex;
    M430WaterParticleState *particles;
} M430WaterSystemWork; /* size 0xC4 */

typedef struct M430WaterParticleWork {
    s16 particleType;
    char reservedParticleWorkMetadata[4];
    s16 particleIndex;
    Vec velocity;
    Vec position;
    float speed;
    float targetSpeed;
    float acceleration;
    float rotationY;
    float travelDistance;
    float maxTravelDistance;
    float rotationSpeed;
    float maxRotation;
} M430WaterParticleWork; /* size 0x40 */

typedef struct M430WaterRenderState {
    /* 0x00 */ u32 cameraIndex;
    /* 0x04 */ s32 modelId;
    /* 0x08 */ s32 textureCopied;
    /* 0x0C */ s32 waterRefractionReady;
    /* 0x10 */ s32 waterSurfaceReady;
    /* 0x14 */ s32 reflectionReady;
    /* 0x18 */ s32 reflectionTextureReady;
    /* 0x1C */ u32 scrollOffset;
    /* 0x20 */ s32 frameCounter;
    /* 0x24 */ float waterOffset;
    /* 0x28 */ float waterLevel;
    /* 0x2C */ Vec *waterVertices;
    /* 0x30 */ Vec *reflectionVertices;
    /* 0x34 */ void *displayListBuffer;
    /* 0x38 */ void *reflectionDisplayList;
    /* 0x3C */ char reservedRenderBytes3C[4];
    /* 0x40 */ s32 activeBufferIndex;
    /* 0x44 */ Vec *vertexBuffers[2];
    /* 0x4C */ char reservedRenderBytes4C[8];
    /* 0x54 */ GXColor *colorBuffers[2];
    /* 0x5C */ s32 displayListSize;
    /* 0x60 */ void *displayList;
    /* 0x64 */ u32 displayListLength;
    /* 0x68 */ u32 textureBufferSize;
    /* 0x6C */ void *textureBuffer;
} M430WaterRenderState; /* size = 0x70 */

void CreateWaterSystem(OMOBJ *object);
void UpdateWaterSystem(OMOBJ *object);
void UpdateWaterCameraState(M430WaterSystemWork *work);
void UpdateWaterSurfaceState(M430WaterSystemWork *work);
void UpdateWaterParticles(M430WaterSystemWork *work);
void UpdateWaterFoam(M430WaterSystemWork *work);
M430WaterParticleState *CreateWaterParticleStates(OMOBJ *ownerObject, s32 cameraIndex, s16 particleCount);
void UpdateWaterParticleStates(OMOBJ *object, M430WaterParticleState *particleStates);
void DrawWaterParticles(OMOBJ *object, M430WaterParticleState *particleStates);
void UpdateWaterParticleObject(OMOBJ *object);
void UpdateWaterEffectObject(OMOBJ *object);
void UpdateWaterParticleMotion(OMOBJ *object, M430WaterParticleWork *particle);
void UpdateWaterRenderObject(OMOBJ *object);
void InitializeWaterRenderStates(OMOBJ *object);
void CreateWaterRenderBuffers(OMOBJ *object, M430WaterRenderState *renderState);
void UpdateWaterSurfaceVertices(OMOBJ *object, M430WaterRenderState *renderState);
void BuildWaterDisplayList(OMOBJ *object, M430WaterRenderState *renderState);
void DrawWaterSurface(HU3DMODEL *model, Mtx matrix);
void DrawWaterReflection(HU3DMODEL *model, Mtx matrix, u32 cameraIndex);
void DrawWaterSurfaceReflection(HU3DMODEL *model, Mtx matrix);
void DrawWaterRefraction(HU3DMODEL *model, Mtx matrix);
void CaptureWaterTexture(HU3DMODEL *model, Mtx matrix);
void CaptureWaterReflection(HU3DMODEL *model, Mtx matrix);
void DrawWaterParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void DrawUnderwaterParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
float GetCameraHorizontalOffset(u32 cameraIndex);

ANIMDATA *waterWaveAnimation;
ANIMDATA *waterFoamTextureAnimation;
ANIMDATA *waterReflectionTextureAnimation;
ANIMDATA *waterSurfaceTextureAnimation;
ANIMDATA *waterRippleTextureAnimation;
M430WaterRenderState waterRenderStates[2];
s32 isWaterSurfaceMoving;
OMOBJ *waterSystemObject;
OMOBJ *waterParticleObject;
OMOBJ *waterRenderObject;
float leftCameraWaterBoundary;
float rightCameraWaterBoundary;
s32 leftCameraWaterBoundaryReady;
s32 rightCameraWaterBoundaryReady;

void InitializeWaterAssets(HUPROCESS *process)
{
    OMOBJ *object;

    waterWaveAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M430, 0x19), HU_MEMNUM_OVL));
    waterFoamTextureAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M430, 0x1A), HU_MEMNUM_OVL));
    waterReflectionTextureAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M430, 0x14), HU_MEMNUM_OVL));
    waterSurfaceTextureAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M430, 0x17), HU_MEMNUM_OVL));
    waterRippleTextureAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M430, 0x18), HU_MEMNUM_OVL));
    object = waterSystemObject = omAddObjEx(process, 0x1E, 0x68, 0, -1, CreateWaterSystem);
    object->work[0] = 0;
    object = waterParticleObject = omAddObjEx(process, 0x1E, 7, 0, -1, UpdateWaterRenderObject);
    object->work[0] = 0;
    object = waterRenderObject = omAddObjEx(process, 0x1E, 0, 0, -1, UpdateWaterParticleObject);
    object->work[0] = 0;
    isWaterSurfaceMoving = 0;
    leftCameraWaterBoundaryReady = rightCameraWaterBoundaryReady = 0;
    leftCameraWaterBoundary = rightCameraWaterBoundary = 0.0f;
}

void ReleaseWaterAssets(void)
{
    HuSprAnimKill(waterWaveAnimation);
    HuSprAnimKill(waterFoamTextureAnimation);
    HuSprAnimKill(waterReflectionTextureAnimation);
    HuSprAnimKill(waterSurfaceTextureAnimation);
    HuSprAnimKill(waterRippleTextureAnimation);
}

void CreateWaterSystem(OMOBJ *object)
{
    s32 setupIndex;
    M430WaterSystemWork *waterWork;
    s32 materialIndex;
    s32 modelId;
    HSFDATA *waterModelData;
    s32 foamTypeIndex;
    M430WaterSystemWork *work;

    u32 waterParticleAnimationIds[1] = { DATA_MAKE_NUM(DATADIR_M430, 0x01) };
    s32 splashModelIds[1];
    u32 splashAssetIds[4] = { DATA_MAKE_NUM(DATADIR_M430, 0x0B), DATA_MAKE_NUM(DATADIR_M430, 0x0C), DATA_MAKE_NUM(DATADIR_M430, 0x0D),
        DATA_MAKE_NUM(DATADIR_M430, 0x0E) };
    u32 foamModelAssetIds[4] = { DATA_MAKE_NUM(DATADIR_M430, 0x02), DATA_MAKE_NUM(DATADIR_M430, 0x02), DATA_MAKE_NUM(DATADIR_M430, 0x03),
        DATA_MAKE_NUM(DATADIR_M430, 0x03) };
    u32 foamOverlayAssetIds[4] = { DATA_MAKE_NUM(DATADIR_M430, 0x04), DATA_MAKE_NUM(DATADIR_M430, 0x04), 0, 0 };
    u32 foamEffectAssetIds[4] = { DATA_MAKE_NUM(DATADIR_M430, 0x06), DATA_MAKE_NUM(DATADIR_M430, 0x07), DATA_MAKE_NUM(DATADIR_M430, 0x08),
        DATA_MAKE_NUM(DATADIR_M430, 0x09) };
    s32 foamModels[4];
    s32 foamOverlayModels[4];
    s32 unusedFoamModels[4];
    s32 splashModels[4];
    s32 cameraIds[2] = { 1, 2 };
    s32 unreferencedModelOffsets[2];
    s32 foamTypeByIndex[6] = { 0, 3, 2, 1, 2, 3 };

    object->stat |= 0x100;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, 2 * sizeof(M430WaterSystemWork), HU_MEMNUM_OVL);
    work = object->data;
    memset(work, 0, 2 * sizeof(M430WaterSystemWork));
    for (waterWork = work, setupIndex = 0; setupIndex < 2; setupIndex++, waterWork++) {
        waterWork->cameraIndex = setupIndex;
        waterWork->cameraPosition.x = waterWork->cameraPosition.y = waterWork->cameraPosition.z = 0.0f;
        waterWork->splashIndex = 0;
        waterWork->foamIndex = 0;
        waterWork->foamEffectIndex = 0;
    }
    waterWork = work;
    for (setupIndex = 0; setupIndex < 2; setupIndex++) {
        object->mdlId[setupIndex] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M430, 0x00));
        waterWork[setupIndex].waterModelId = object->mdlId[setupIndex];
        Hu3DModelLayerSet(waterWork[setupIndex].waterModelId, 0);
        Hu3DModelScaleSet(waterWork[setupIndex].waterModelId, 1.15f, 1.15f, 1.15f);
        modelId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M430, 0x0A)), 1);
        waterWork[setupIndex].waveParticleModelId = modelId;
        Hu3DModelLayerSet(modelId, 1);
        *((s16 *)Hu3DData[modelId].hookData) = 1;
        waterWork[setupIndex].waveTextureScrollId = Hu3DTexScrollCreate(waterWork[setupIndex].waterModelId, "pa_sk2");
        waterWork[setupIndex].foamTextureScrollId = Hu3DTexScrollCreate(waterWork[setupIndex].waterModelId, "pa_sk3");
        Hu3DTexScrollPosMoveSet(waterWork[setupIndex].waveTextureScrollId, -0.0055f * REFRESH_FREQ, 0.0f, 0.0f);
        Hu3DTexScrollPosMoveSet(waterWork[setupIndex].foamTextureScrollId, -0.012f * REFRESH_FREQ, 0.0f, 0.0f);
        waterWork[setupIndex].waterDepth = 50000.0f;
        waterModelData = Hu3DData[waterWork[setupIndex].waterModelId].hsf;
        for (materialIndex = 0; materialIndex < waterModelData->materialNum; materialIndex++) {
            waterModelData->material[materialIndex].flags &= ~0x200;
            waterModelData->material[materialIndex].pass = 1;
            waterModelData->material[materialIndex].invAlpha = 0.001f;
        }
    }
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M430, 0x0F));
    Hu3DModelLayerSet(object->mdlId[2], 1);
    Hu3DModelScaleSet(object->mdlId[2], 1.5f, 1.5f, 1.5f);
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M430, 0x10));
    Hu3DModelLayerSet(object->mdlId[3], 2);
    Hu3DModelScaleSet(object->mdlId[3], 1.5f, 1.5f, 1.5f);
    for (setupIndex = 0; setupIndex < 2; setupIndex++) {
        modelId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M430, 0x15)), 0x64);
        object->mdlId[setupIndex + 4] = modelId;
        waterWork[setupIndex].waterSurfaceModelId = modelId;
        Hu3DModelLayerSet(modelId, 2);
        Hu3DParticleColSet(modelId, 0xFF, 0xFF, 0xFF);
        Hu3DParticleHookSet(modelId, DrawWaterParticleEffect);
        Hu3DModelPosSet(modelId, 0.0f, 0.0f, 0.0f);
        *((s16 *)Hu3DData[modelId].hookData) = 0;
    }
    for (setupIndex = 0; setupIndex < 2; setupIndex++) {
        modelId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M430, 0x15)), 0x64);
        object->mdlId[setupIndex + 6] = modelId;
        waterWork[setupIndex].underwaterModelId = modelId;
        Hu3DModelLayerSet(modelId, 1);
        Hu3DParticleColSet(modelId, 0xFF, 0xFF, 0xFF);
        Hu3DParticleHookSet(modelId, DrawUnderwaterParticles);
        Hu3DModelPosSet(modelId, 0.0f, 0.0f, 0.0f);
        *((s16 *)Hu3DData[modelId].hookData) = 0;
    }
    for (setupIndex = 0; setupIndex < 4; setupIndex++) {
        splashModels[setupIndex] = Hu3DModelCreateFile(splashAssetIds[setupIndex]);
    }
    for (setupIndex = 0; setupIndex < 0xC; setupIndex++) {
        modelId = splashModels[setupIndex % 4];
        object->mdlId[setupIndex + 8] = Hu3DModelLink(modelId);
        object->mdlId[setupIndex + 0x14] = Hu3DModelLink(modelId);
        waterWork[0].splashModelIds[setupIndex] = object->mdlId[setupIndex + 8];
        waterWork[1].splashModelIds[setupIndex] = object->mdlId[setupIndex + 0x14];
        Hu3DModelLayerSet(waterWork[0].splashModelIds[setupIndex], 1);
        Hu3DModelLayerSet(waterWork[1].splashModelIds[setupIndex], 1);
    }
    for (setupIndex = 0; setupIndex < 4; setupIndex++) {
        foamModels[setupIndex] = Hu3DModelCreateFile(foamModelAssetIds[setupIndex]);
        foamOverlayModels[setupIndex] = -1;
        unusedFoamModels[setupIndex] = -1;
        if (foamOverlayAssetIds[setupIndex] != 0) {
            foamOverlayModels[setupIndex] = Hu3DModelCreateFile(foamOverlayAssetIds[setupIndex]);
        }
    }
    for (setupIndex = 0; setupIndex < 6; setupIndex++) {
        s32 unreferencedModelOffsets[2] = { 0x5C, 0x62 };
        foamTypeIndex = foamTypeByIndex[setupIndex];
        modelId = foamModels[foamTypeIndex];
        for (materialIndex = 0; materialIndex < 2; materialIndex++) {
            object->mdlId[setupIndex + unreferencedModelOffsets[materialIndex]] = Hu3DModelLink(modelId);
            waterWork[materialIndex].foamModelIds[setupIndex] = object->mdlId[setupIndex + unreferencedModelOffsets[materialIndex]];
            Hu3DModelLayerSet(waterWork[materialIndex].foamModelIds[setupIndex], 1);
        }
        for (materialIndex = 0; materialIndex < 2; materialIndex++) {
            waterWork[materialIndex].foamOverlayModelIds[setupIndex] = -1;
            waterWork[materialIndex].foamEffectModelIds[setupIndex] = -1;
        }
        if (foamOverlayModels[foamTypeIndex] >= 0) {
            modelId = foamOverlayModels[foamTypeIndex];
            for (materialIndex = 0; materialIndex < 2; materialIndex++) {
                waterWork[materialIndex].foamOverlayModelIds[setupIndex] = Hu3DModelLink(modelId);
                Hu3DModelLayerSet(waterWork[materialIndex].foamOverlayModelIds[setupIndex], 4);
                Hu3DModelAttrSet(waterWork[materialIndex].foamOverlayModelIds[setupIndex], HU3D_ATTR_ZWRITE_OFF);
            }
        }
        if (foamEffectAssetIds[foamTypeIndex] != 0) {
            for (materialIndex = 0; materialIndex < 2; materialIndex++) {
                modelId = Hu3DParticleCreate(HuSprAnimReadFile(foamEffectAssetIds[foamTypeIndex]), 1);
                waterWork[materialIndex].foamEffectModelIds[setupIndex] = modelId;
                Hu3DModelLayerSet(modelId, 5);
                Hu3DParticleColSet(modelId, 0xFF, 0xFF, 0xFF);
                Hu3DModelPosSet(modelId, 0.0f, 0.0f, 0.0f);
                *((s16 *)Hu3DData[modelId].hookData) = 1;
            }
        }
    }
    for (setupIndex = 0; setupIndex < 1; setupIndex++) {
        splashModelIds[setupIndex] = Hu3DModelCreateFile(waterParticleAnimationIds[setupIndex]);
    }
    for (setupIndex = 0; setupIndex < 0x1E; setupIndex += 3) {
        modelId = splashModelIds[frandmod(1)];
        object->mdlId[setupIndex + 0x20] = Hu3DModelLink(modelId);
        object->mdlId[setupIndex + 0x21] = Hu3DModelLink(modelId);
        object->mdlId[setupIndex + 0x22] = Hu3DModelLink(modelId);
        object->mdlId[setupIndex + 0x3E] = Hu3DModelLink(modelId);
        object->mdlId[setupIndex + 0x3F] = Hu3DModelLink(modelId);
        object->mdlId[setupIndex + 0x40] = Hu3DModelLink(modelId);
        waterWork->leftWaveModelIds[setupIndex / 3] = object->mdlId[setupIndex + 0x20];
        waterWork->centerWaveModelIds[setupIndex / 3] = object->mdlId[setupIndex + 0x21];
        waterWork->rightWaveModelIds[setupIndex / 3] = object->mdlId[setupIndex + 0x22];
        waterWork[1].leftWaveModelIds[setupIndex / 3] = object->mdlId[setupIndex + 0x3E];
        waterWork[1].centerWaveModelIds[setupIndex / 3] = object->mdlId[setupIndex + 0x3F];
        waterWork[1].rightWaveModelIds[setupIndex / 3] = object->mdlId[setupIndex + 0x40];
        Hu3DModelLayerSet(waterWork->leftWaveModelIds[setupIndex / 3], 1);
        Hu3DModelLayerSet(waterWork->centerWaveModelIds[setupIndex / 3], 1);
        Hu3DModelLayerSet(waterWork->rightWaveModelIds[setupIndex / 3], 1);
        Hu3DModelLayerSet(waterWork[1].leftWaveModelIds[setupIndex / 3], 1);
        Hu3DModelLayerSet(waterWork[1].centerWaveModelIds[setupIndex / 3], 1);
        Hu3DModelLayerSet(waterWork[1].rightWaveModelIds[setupIndex / 3], 1);
    }
    for (setupIndex = 0; setupIndex < 2; setupIndex++) {
        waterWork[setupIndex].particles = CreateWaterParticleStates(object, setupIndex, 8);
    }
    for (setupIndex = 0; setupIndex < 4; setupIndex++) {
        Hu3DModelKill(splashModels[setupIndex]);
    }
    for (setupIndex = 0; setupIndex < 4; setupIndex++) {
        Hu3DModelKill(foamModels[setupIndex]);
        if (foamOverlayModels[setupIndex] >= 0) {
            Hu3DModelKill(foamOverlayModels[setupIndex]);
        }
        if (unusedFoamModels[setupIndex] >= 0) {
            Hu3DModelKill(unusedFoamModels[setupIndex]);
        }
    }
    for (setupIndex = 0; setupIndex < 1; setupIndex++) {
        Hu3DModelKill(splashModelIds[setupIndex]);
    }
    for (waterWork = work, setupIndex = 0; setupIndex < 2; setupIndex++, waterWork++) {
        Hu3DModelCameraSet(waterWork->waterModelId, cameraIds[setupIndex]);
        Hu3DModelCameraSet(waterWork->waveParticleModelId, cameraIds[setupIndex]);
        Hu3DModelCameraSet(waterWork->waterSurfaceModelId, cameraIds[setupIndex]);
        Hu3DModelCameraSet(waterWork->underwaterModelId, cameraIds[setupIndex]);
        for (materialIndex = 0; materialIndex < 0xC; materialIndex++) {
            Hu3DModelCameraSet(waterWork->splashModelIds[materialIndex], cameraIds[setupIndex]);
        }
        for (materialIndex = 0; materialIndex < 6; materialIndex++) {
            Hu3DModelCameraSet(waterWork->foamModelIds[materialIndex], cameraIds[setupIndex]);
            Hu3DModelAttrSet(waterWork->foamModelIds[materialIndex], HU3D_ATTR_DISPOFF);
            if (waterWork->foamOverlayModelIds[materialIndex] >= 0) {
                Hu3DModelCameraSet(waterWork->foamOverlayModelIds[materialIndex], cameraIds[setupIndex]);
                Hu3DModelAttrSet(waterWork->foamOverlayModelIds[materialIndex], HU3D_ATTR_DISPOFF);
            }
            if (waterWork->foamEffectModelIds[materialIndex] >= 0) {
                Hu3DModelCameraSet(waterWork->foamEffectModelIds[materialIndex], cameraIds[setupIndex]);
                Hu3DModelAttrSet(waterWork->foamEffectModelIds[materialIndex], HU3D_ATTR_DISPOFF);
            }
        }
        for (materialIndex = 0; materialIndex < 0xA; materialIndex++) {
            Hu3DModelCameraSet(waterWork->leftWaveModelIds[materialIndex], cameraIds[setupIndex]);
            Hu3DModelCameraSet(waterWork->centerWaveModelIds[materialIndex], cameraIds[setupIndex]);
            Hu3DModelCameraSet(waterWork->rightWaveModelIds[materialIndex], cameraIds[setupIndex]);
            Hu3DModelAttrSet(waterWork->leftWaveModelIds[materialIndex], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(waterWork->centerWaveModelIds[materialIndex], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(waterWork->rightWaveModelIds[materialIndex], HU3D_ATTR_DISPOFF);
        }
    }
    Hu3DModelCameraSet(object->mdlId[2], cameraIds[0]);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelCameraSet(object->mdlId[3], cameraIds[0]);
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    object->objFunc = UpdateWaterSystem;
}

void UpdateWaterSystem(OMOBJ *object)
{
    M430WaterSystemWork *waterWork;
    s32 cameraIndex;
    M430WaterSystemWork *work;

    work = object->data;
    if ((getRoundState() >= 2) && (isMinigameActive() != 0)) {
        waterWork = work;
        if ((leftCameraWaterBoundaryReady == 0) && (getCountdownTime() < 5.0f)) {
            leftCameraWaterBoundary = waterWork[0].cameraPosition.x;
            if (leftCameraWaterBoundary > waterWork[1].cameraPosition.x) {
                leftCameraWaterBoundary = waterWork[1].cameraPosition.x;
            }
            leftCameraWaterBoundaryReady = 1;
        }
        if ((rightCameraWaterBoundaryReady == 0) && (getCountdownTime() < 1.8f)) {
            rightCameraWaterBoundary = waterWork[0].cameraPosition.x;
            if (rightCameraWaterBoundary > waterWork[1].cameraPosition.x) {
                rightCameraWaterBoundary = waterWork[1].cameraPosition.x;
            }
            rightCameraWaterBoundaryReady = 1;
        }
    }
    for (waterWork = work, cameraIndex = 0; cameraIndex < 2; cameraIndex++, waterWork++) {
        waterWork->cameraPosition.z = waterWork->cameraPosition.x;
        waterWork->cameraPosition.x = waterWork->cameraPosition.y;
        UpdateWaterCameraState(waterWork);
        UpdateWaterSurfaceState(waterWork);
        UpdateWaterFoam(waterWork);
        UpdateWaterParticles(waterWork);
        UpdateWaterParticleStates(object, waterWork->particles);
        DrawWaterParticles(object, waterWork->particles);
        if (cameraIndex != 0) {
            if (isMinigameActive() != 0) {
                Hu3DModelCameraSet(waterWork->waterSurfaceModelId, 2);
                Hu3DModelCameraSet(waterWork->underwaterModelId, 2);
            }
            else {
                Hu3DModelCameraSet(waterWork->waterSurfaceModelId, 1);
                Hu3DModelCameraSet(waterWork->underwaterModelId, 1);
            }
        }
    }
    if (isWaterSurfaceMoving != 0) {
        Vec shadowCameraPosition = { 0.0f, 3000.0f, 0.0f };
        Vec shadowTarget = { 0.0f, 0.0f, 0.0f };
        Vec worldUp = { 0.0f, 1.0f, 0.0f };

        Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
        Hu3DModelShadowMapSet(object->mdlId[2]);
        Hu3DModelPosSet(object->mdlId[2], 0.0f, 0.0f, GetWaterCameraX(0) - 400.0f);
        Hu3DModelAttrReset(object->mdlId[3], HU3D_ATTR_DISPOFF);
        Hu3DModelPosSet(object->mdlId[3], 0.0f, 0.0f, GetWaterCameraX(0) - 400.0f);
        shadowCameraPosition.z = shadowTarget.z = GetWaterCameraX(0) - 400.0f;
        shadowCameraPosition.z += 100.0f;
        Hu3DShadowPosSet(&shadowCameraPosition, &worldUp, &shadowTarget);
        return;
    }
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
}

void UpdateWaterCameraState(M430WaterSystemWork *work)
{
    Vec waterModelPosition;
    Vec cameraPosition;
    Vec cameraDirection;
    HU3DPARTICLEDATA *waveParticle;
    s32 alpha;

    float waterBounds[2] = { -450.0f, 450.0f };
    cameraPosition.x = CenterM[work->cameraIndex].x;
    cameraPosition.y = CenterM[work->cameraIndex].y + (CZoomM[work->cameraIndex] * sind(-CRotM[work->cameraIndex].x));
    cameraPosition.z = CenterM[work->cameraIndex].z + (CZoomM[work->cameraIndex] * cosd(-CRotM[work->cameraIndex].x));
    waterModelPosition.x = cameraPosition.x;
    waterModelPosition.y = 0.0f;
    waterModelPosition.z = cameraPosition.z - 450000.0f;
    VECSubtract(&waterModelPosition, &cameraPosition, &cameraDirection);
    VECNormalize(&cameraDirection, &cameraDirection);
    VECScale(&cameraDirection, &cameraDirection, 6000.0f);
    VECAdd(&cameraPosition, &cameraDirection, &waterModelPosition);
    Hu3DModelPosSetV(work->waterModelId, &waterModelPosition);
    if ((getRoundState() >= 2) && (isMinigameActive() != 0)) {
        if ((rightCameraWaterBoundaryReady != 0) && (rightCameraWaterBoundary >= work->cameraPosition.x)) {
            if (work->waterDepth > 0.0f) {
                work->waterDepth = GetWaterCameraX(work->cameraIndex) - 23000.0f;
            }
            waterModelPosition.x = 0.0f;
            waterModelPosition.y = 0.0f;
            waterModelPosition.z = work->waterDepth - GetWaterCameraX(work->cameraIndex);
            VECSubtract(&waterModelPosition, &cameraPosition, &cameraDirection);
            VECNormalize(&cameraDirection, &cameraDirection);
            VECScale(&cameraDirection, &cameraDirection, 5000.0f);
            VECAdd(&cameraPosition, &cameraDirection, &waterModelPosition);
            waterModelPosition.y += 237.5f;
            Hu3DModelPosSet(work->waveParticleModelId, 0.0f, 593.75f, work->waterDepth);
            waveParticle = ((HU3DPARTICLE *)Hu3DData[work->waveParticleModelId].hookData)->data;
            waveParticle->pos.x = waveParticle->pos.y = waveParticle->pos.z = 0.0f;
            waveParticle->scale = 2000.0f;
            alpha = waveParticle->color.a;
            alpha += 2;
            if (alpha > 0xFF) {
                alpha = 0xFF;
            }
            waveParticle->color.a = alpha;
        }
    }
    else {
        waveParticle = ((HU3DPARTICLE *)Hu3DData[work->waveParticleModelId].hookData)->data;
        waveParticle->scale = 0.0f;
        waveParticle->color.a = 0;
    }
    DCStoreRange(waveParticle, sizeof(HU3DPARTICLEDATA));
}

void UpdateWaterSurfaceState(M430WaterSystemWork *work)
{
    float splashStartZ;
    float splashRange;
    float cameraProgress;
    s32 splashIndex;
    s32 splashModelCount;
    s32 showSplashes;

    showSplashes = 1;
    cameraProgress = work->cameraPosition.x;
    work->splashIndex = (s32)(Fabs2(cameraProgress) / 1000.0) % 12;
    splashIndex = work->splashIndex;
    splashStartZ = 1000.0f * (s32)(work->cameraPosition.x / 1000.0f);
    splashRange = 4000.0f;
    for (splashModelCount = 0; splashModelCount < 0xC; splashModelCount++) {
        if (showSplashes != 0) {
            Hu3DModelAttrReset(work->splashModelIds[splashIndex], HU3D_ATTR_DISPOFF);
            Hu3DModelPosSet(work->splashModelIds[splashIndex], 0.0f, 0.0f, splashStartZ);
            splashStartZ -= 1000.0f;
            splashRange -= 1000.0f;
            if (splashRange < 0.0f) {
                showSplashes = 0;
            }
        }
        else {
            Hu3DModelAttrSet(work->splashModelIds[splashIndex], HU3D_ATTR_DISPOFF);
        }
        splashIndex = (splashIndex + 1) % 12;
    }
}

void UpdateWaterParticles(M430WaterSystemWork *work)
{
    float foamZ;
    float fadeInDistance;
    float fadeOutDistance;
    float fadeOutStartZ;
    float fadeOutEndZ;
    float cameraProgress;
    s32 foamIndex;
    HU3DPARTICLEDATA *foamParticle;
    s32 foamModelIndex;
    s32 showFoam;
    s32 foamParticleModelId;
    s32 hideFoamModel;
    s32 hideOverlayModel;
    s32 hideEffectModel;

    showFoam = 1;
    {
        float foamPositionOffsets[6] = { -1800.0f, 2000.0f, -1800.0f, 2000.0f, -1800.0f, 2000.0f };
        Vec2f foamEffectOffsets[6] = {
            { -130.0f, 500.0f },
            { 180.0f, 70.0f },
            { 100.0f, 50.0f },
            { -30.000002f, 500.0f },
            { 100.0f, 50.0f },
            { 180.0f, 70.0f },
        };
        float foamScales[6] = { 2750.0f, 1550.0f, 1450.0f, 2750.0f, 1450.0f, 1550.0f };
        if (isWaterSurfaceMoving != 0) {
            showFoam = 0;
        }
        if ((getRoundState() == 0) || (isMinigameActive() == 0) || (leftCameraWaterBoundaryReady == 0) || !(leftCameraWaterBoundary > work->cameraPosition.x)) {
            for (foamModelIndex = 0; foamModelIndex < 6; foamModelIndex++) {
                work->foamActiveFlags[foamModelIndex] = 1;
            }
        }
        cameraProgress = work->cameraPosition.x;
        work->foamEffectIndex = (s32)(Fabs2(cameraProgress) / 5000.0) % 6;
        foamIndex = work->foamEffectIndex;
        foamZ = 1000.0f + (5000.0f * (s32)(work->cameraPosition.x / 5000.0f));
        fadeOutStartZ = (1000.0f + work->cameraPosition.x) - 9000.0f;
        fadeOutEndZ = (1000.0f + work->cameraPosition.x) - 20000.0f;
        for (foamModelIndex = 0; foamModelIndex < 6; foamModelIndex++) {
            hideFoamModel = hideOverlayModel = hideEffectModel = 1;
            if ((showFoam != 0) && (work->foamActiveFlags[foamIndex] != 0)) {
                if ((foamZ > fadeOutStartZ) || ((work->foamEffectModelIds[foamIndex] >= 0) && (foamZ > fadeOutEndZ))) {
                    if (foamZ > fadeOutStartZ) {
                        Hu3DModelAttrReset(work->foamModelIds[foamIndex], HU3D_ATTR_DISPOFF);
                        Hu3DModelPosSet(work->foamModelIds[foamIndex], foamPositionOffsets[foamIndex], 0.0f, foamZ);
                        if (work->foamOverlayModelIds[foamIndex] >= 0) {
                            Hu3DModelAttrReset(work->foamOverlayModelIds[foamIndex], HU3D_ATTR_DISPOFF);
                            Hu3DModelPosSet(work->foamOverlayModelIds[foamIndex], foamPositionOffsets[foamIndex], 0.0f, foamZ);
                        }
                        hideFoamModel = hideOverlayModel = 0;
                    }
                    if ((work->foamEffectModelIds[foamIndex] >= 0) && (foamZ > fadeOutEndZ) && (foamZ < (1000.0f + fadeOutStartZ))) {
                        foamParticleModelId = work->foamEffectModelIds[foamIndex];
                        Hu3DModelAttrReset(foamParticleModelId, HU3D_ATTR_DISPOFF);
                        foamParticle = ((HU3DPARTICLE *)Hu3DData[foamParticleModelId].hookData)->data;
                        Hu3DModelPosSet(work->foamEffectModelIds[foamIndex], foamPositionOffsets[foamIndex] + foamEffectOffsets[foamIndex].x, foamEffectOffsets[foamIndex].y, foamZ);
                        foamParticle->pos.x = foamParticle->pos.y = foamParticle->pos.z = 0.0f;
                        foamParticle->scale = foamScales[foamIndex];
                        foamParticle->color.a = -1;
                        fadeInDistance = fabs(fadeOutEndZ - foamZ);
                        fadeOutDistance = 1000.0f - (foamZ - fadeOutStartZ);
                        if (fadeInDistance < 2000.0f) {
                            fadeInDistance *= 0.0005f;
                            foamParticle->color.a = 255.0f * fadeInDistance;
                        }
                        else if (fadeOutDistance < 1000.0f) {
                            fadeOutDistance *= 0.001f;
                            foamParticle->color.a = 255.0f * fadeOutDistance;
                        }
                        DCStoreRange(foamParticle, sizeof(HU3DPARTICLEDATA));
                        hideEffectModel = 0;
                    }
                }
            }
            else {
                work->foamActiveFlags[foamIndex] = 0;
            }
            if (hideFoamModel != 0) {
                Hu3DModelAttrSet(work->foamModelIds[foamIndex], HU3D_ATTR_DISPOFF);
            }
            if ((hideOverlayModel != 0) && (work->foamOverlayModelIds[foamIndex] >= 0)) {
                Hu3DModelAttrSet(work->foamOverlayModelIds[foamIndex], HU3D_ATTR_DISPOFF);
            }
            if ((hideEffectModel != 0) && (work->foamEffectModelIds[foamIndex] >= 0)) {
                Hu3DModelAttrSet(work->foamEffectModelIds[foamIndex], HU3D_ATTR_DISPOFF);
            }
            foamZ -= 5000.0f;
            if (foamZ < fadeOutEndZ) {
                showFoam = 0;
            }
            foamIndex = (foamIndex + 1) % 6;
        }
    }
}

void UpdateWaterFoam(M430WaterSystemWork *work)
{
    float waveZ;
    float visibleWaveRange;
    float cameraProgress;
    s32 waveIndex;
    s32 waveModelIndex;
    s32 showWaves;

    showWaves = 1;
    if (isWaterSurfaceMoving != 0) {
        showWaves = 0;
    }
    cameraProgress = work->cameraPosition.x;
    work->foamIndex = (s32)(Fabs2(cameraProgress) / 1000.0) % 10;
    waveIndex = work->foamIndex;
    waveZ = 1000.0f + (1000.0f * (s32)(work->cameraPosition.x / 1000.0f));
    visibleWaveRange = 8000.0f;
    for (waveModelIndex = 0; waveModelIndex < 0xA; waveModelIndex++) {
        if (showWaves != 0) {
            Hu3DModelAttrReset(work->leftWaveModelIds[waveIndex], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(work->centerWaveModelIds[waveIndex], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(work->rightWaveModelIds[waveIndex], HU3D_ATTR_DISPOFF);
            Hu3DModelPosSet(work->leftWaveModelIds[waveIndex], -920.0f, 0.0f, waveZ);
            Hu3DModelPosSet(work->centerWaveModelIds[waveIndex], 0.0f, 0.0f, waveZ);
            Hu3DModelPosSet(work->rightWaveModelIds[waveIndex], 920.0f, 0.0f, waveZ);
            waveZ -= 1000.0f;
            visibleWaveRange -= 1000.0f;
            if (visibleWaveRange < 0.0f) {
                showWaves = 0;
            }
        }
        else {
            Hu3DModelAttrSet(work->leftWaveModelIds[waveIndex], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(work->centerWaveModelIds[waveIndex], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(work->rightWaveModelIds[waveIndex], HU3D_ATTR_DISPOFF);
        }
        waveIndex = (waveIndex + 1) % 10;
    }
}

M430WaterParticleState *CreateWaterParticleStates(OMOBJ *ownerObject, s32 cameraIndex, s16 particleCount)
{
    float startingHeight;
    float startingAngle;
    float radialDistance;
    M430WaterParticleState *particle;
    s32 particleIndex;
    M430WaterParticleState *particleArray;

    u32 particleModelAssetIds[2] = { DATA_MAKE_NUM(DATADIR_M430, 0x11), DATA_MAKE_NUM(DATADIR_M430, 0x12) };
    s32 cameraIds[2] = { 1, 2 };
    particleArray = HuMemDirectMallocNum(HEAP_HEAP, particleCount * sizeof(M430WaterParticleState), HU_MEMNUM_OVL);
    memset(particleArray, 0, particleCount * 0x3C);
    startingHeight = -90.0f;
    for (particle = particleArray, particleIndex = 0; particleIndex < particleCount; particleIndex++, particle++) {
        particle->ownerIndex = cameraIndex;
        particle->modelId = Hu3DModelCreateFile(particleModelAssetIds[particleIndex & 1]);
        Hu3DModelLayerSet(particle->modelId, 1);
        Hu3DModelCameraSet(particle->modelId, cameraIds[particle->ownerIndex]);
        startingAngle = frandmod(0x168);
        radialDistance = frandmod(0x118);
        particle->position.x = radialDistance * sind(startingAngle) + GetCameraHorizontalOffset(cameraIndex);
        particle->position.y = startingHeight;
        particle->position.z = radialDistance * cosd(startingAngle);
        particle->rotationY = frandmod(0x168);
        particle->travelDistance = 1.0f;
        particle->maxTravelDistance = 0.0f;
        startingHeight -= 10.0f;
        particle->effectIndex = particleCount;
    }
    return particleArray;
}

void UpdateWaterParticleStates(OMOBJ *object, M430WaterParticleState *particleStates)
{
    Vec targetDelta;
    float distanceToTarget;
    float targetAngle;
    float targetDistance;
    s32 particleIndex;
    s32 respawnParticle;
    s32 particleCount;

    particleCount = particleStates->effectIndex;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particleStates++) {
        respawnParticle = 0;
        if (particleStates->travelDistance > particleStates->maxTravelDistance) {
            respawnParticle = 1;
        }
        else {
            VECSubtract(&particleStates->targetPosition, &particleStates->position, &targetDelta);
            distanceToTarget = VECMag(&targetDelta);
            if (distanceToTarget < 50.0f) {
                respawnParticle = 1;
            }
        }
        if (respawnParticle != 0) {
            distanceToTarget = frandmod(0x168);
            targetAngle = 400.0f + frandmod(0x15E);
            particleStates->targetPosition.x = targetAngle * sind(distanceToTarget) + GetCameraHorizontalOffset(particleStates->ownerIndex);
            particleStates->targetPosition.z = 2.0 * (targetAngle * cosd(distanceToTarget));
            particleStates->targetPosition.y = particleStates->position.y;
            particleStates->travelDistance = 0.0f;
            particleStates->maxTravelDistance = 200.0f + frandmod(0x258);
            particleStates->targetSpeed = 400.0f;
        }
        VECSubtract(&particleStates->targetPosition, &particleStates->position, &targetDelta);
        particleStates->targetRotationY = atan2d(targetDelta.x, targetDelta.z);
        particleStates->rotationY = interpolateAngle(particleStates->rotationY, particleStates->targetRotationY, 0.08f);
        particleStates->targetSpeed += 0.05f * (300.0f - particleStates->targetSpeed);
        particleStates->speed += 0.2f * (particleStates->targetSpeed - particleStates->speed);
        particleStates->position.x = particleStates->position.x + ((REFRESH_FREQ * particleStates->speed) * sind(particleStates->rotationY));
        particleStates->position.z = particleStates->position.z + ((REFRESH_FREQ * particleStates->speed) * cosd(particleStates->rotationY));
        particleStates->travelDistance += REFRESH_FREQ * particleStates->speed;
    }
}

void DrawWaterParticles(OMOBJ *object, M430WaterParticleState *particleStates)
{
    float cameraDepthOffset;
    s32 particleIndex;
    s32 particleCount;

    particleCount = particleStates->effectIndex;
    if (isWaterSurfaceMoving != 0) {
        for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particleStates++) {
            Hu3DModelAttrSet(particleStates->modelId, HU3D_ATTR_DISPOFF);
        }
        return;
    }
    cameraDepthOffset = (s32)(0.00011111111f * (1000.0f + GetWaterCameraX(particleStates->ownerIndex)));
    cameraDepthOffset = (100.0f * (90.0f * cameraDepthOffset)) - 5000.0f;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particleStates++) {
        Hu3DModelPosSet(particleStates->modelId, particleStates->position.x, particleStates->position.y, particleStates->position.z + cameraDepthOffset);
        Hu3DModelRotSet(particleStates->modelId, 0.0f, particleStates->rotationY, 0.0f);
        Hu3DModelAttrReset(particleStates->modelId, HU3D_ATTR_DISPOFF);
    }
}

void UpdateWaterParticleObject(OMOBJ *object)
{
    float initialSpeed;
    float velocityAngle;
    float startingHeight;
    float rotationLimit;
    M430WaterParticleWork *particle;
    s16 particleCount;
    s32 particleIndex;
    M430WaterParticleWork *particleWork;
    s32 particleModelId;

    particleCount = 0x20;
    particleWork = HuMemDirectMallocNum(HEAP_HEAP, particleCount * sizeof(M430WaterParticleWork), HU_MEMNUM_OVL);
    object->data = particleWork;
    memset(particleWork, 0, particleCount * sizeof(M430WaterParticleWork));
    particleModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M430, 0x13));
    startingHeight = 1000.0f;
    for (particle = particleWork, particleIndex = 0; particleIndex < particleCount; particleIndex++, particle++) {
        particle->particleType = Hu3DModelLink(particleModelId);
        Hu3DModelLayerSet(particle->particleType, 1);
        Hu3DModelCameraSet(particle->particleType, 1);
        Hu3DModelAttrSet(particle->particleType, HU3D_ATTR_DISPOFF);
        particle->position.x = (2.8f * frandmod(0x3E8)) - 1400.0f;
        particle->position.z = 0.7f * frandmod(0x3E8) - 350.0f;
        particle->position.y = 300.0f + (0.8f * frandmod(0x3E8));
        velocityAngle = frandmod(0x168);
        initialSpeed = 200.0f + frandmod(0xAF);
        particle->velocity.x = initialSpeed * sind(velocityAngle);
        particle->velocity.y = 0.0f;
        particle->velocity.z = initialSpeed * cosd(velocityAngle);
        VECAdd(&particle->position, &particle->velocity, &particle->velocity);
        particle->rotationSpeed = 60.0f - (0.13483146f * initialSpeed);
        particle->acceleration = frandmod(0x168);
        if (frandmod(0x3E8) < 0x1F4) {
            rotationLimit = -70.0f + (0.1f * frandmod(0x32));
        }
        else {
            rotationLimit = 70.0f - (0.1f * frandmod(0x32));
        }
        particle->maxRotation = rotationLimit;
        if (particle->maxRotation < 0.0f) {
            particle->rotationSpeed = -particle->rotationSpeed;
        }
        particle->travelDistance = 1.0f;
        particle->maxTravelDistance = 0.0f;
        startingHeight -= 50.0f;
        particle->particleIndex = particleCount;
    }
    (void)particleIndex;
    (void)object;
    Hu3DModelKill(particleModelId);
    object->objFunc = UpdateWaterEffectObject;
}

void UpdateWaterEffectObject(OMOBJ *object)
{
    Vec velocity;
    s32 unusedParticleFlag;
    s32 particleIndex;

    M430WaterParticleWork *particle = object->data;
    s32 particleCount = particle->particleIndex;
    if (getRoundState() >= 4) {
        for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particle++) {
            unusedParticleFlag = 0;
            VECSubtract(&particle->position, &particle->velocity, &velocity);
            particle->rotationY = atan2d(velocity.x, velocity.z);
            particle->acceleration = interpolateAngle(particle->acceleration, particle->rotationY + particle->maxRotation, 0.08f);
            particle->targetSpeed += (0.05f * (600.0f - particle->targetSpeed));
            particle->speed += (0.2f * (particle->targetSpeed - particle->speed));
            particle->velocity.x += ((REFRESH_FREQ * particle->speed) * sind(particle->acceleration));
            particle->velocity.z += ((REFRESH_FREQ * particle->speed) * cosd(particle->acceleration));
            particle->travelDistance += (REFRESH_FREQ * particle->speed);
        }
        UpdateWaterParticleMotion(object, object->data);
    }
}

void UpdateWaterParticleMotion(OMOBJ *object, M430WaterParticleWork *particle)
{
    Mtx rotationMatrix;
    float cameraDepthOffset;
    s32 particleIndex;
    Mtx *modelMatrix;
    s32 particleCount;

    particleCount = particle->particleIndex;
    cameraDepthOffset = GetWaterCameraX(0) - 2000.0f;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particle++) {
        Hu3DModelPosSet(particle->particleType, particle->velocity.x, particle->velocity.y, particle->velocity.z + cameraDepthOffset);
        modelMatrix = &Hu3DData[particle->particleType].mtx;
        MTXRotRad(*modelMatrix, 0x59, 0.017453292f * particle->acceleration);
        MTXRotRad(rotationMatrix, 0x5A, 0.017453292f * particle->rotationSpeed);
        MTXConcat(*modelMatrix, rotationMatrix, *modelMatrix);
        Hu3DModelAttrReset(particle->particleType, HU3D_ATTR_DISPOFF);
    }
}

float GetWaterCameraX(s32 cameraIndex)
{
    M430WaterSystemWork *waterWork;
    M430WaterSystemWork *waterWorkArray;

    waterWorkArray = waterSystemObject->data;
    cameraIndex &= 1;
    waterWork = waterWorkArray;
    return waterWork[cameraIndex].cameraPosition.x;
}

void SetWaterCameraY(s32 cameraIndex, float cameraPositionY)
{
    M430WaterSystemWork *waterWork;
    M430WaterSystemWork *waterWorkArray;

    waterWorkArray = waterSystemObject->data;
    cameraIndex &= 1;
    waterWork = waterWorkArray;
    waterWork[cameraIndex].cameraPosition.y = cameraPositionY;
}

void UpdateWaterRenderObject(OMOBJ *object)
{
    object->stat |= 0x100;
    omSetStatBit(object, 0xA0);
    object->mdlId[0] = Hu3DHookFuncCreate(DrawWaterSurface);
    Hu3DModelLayerSet(object->mdlId[0], 4);
    Hu3DModelCameraSet(object->mdlId[0], 1);
    object->mdlId[1] = Hu3DHookFuncCreate(DrawWaterSurface);
    Hu3DModelLayerSet(object->mdlId[1], 4);
    Hu3DModelCameraSet(object->mdlId[1], 2);
    object->mdlId[2] = Hu3DHookFuncCreate(DrawWaterSurfaceReflection);
    Hu3DModelLayerSet(object->mdlId[2], 0);
    Hu3DModelCameraSet(object->mdlId[2], 1);
    object->mdlId[3] = Hu3DHookFuncCreate(DrawWaterSurfaceReflection);
    Hu3DModelLayerSet(object->mdlId[3], 0);
    Hu3DModelCameraSet(object->mdlId[3], 2);
    object->mdlId[4] = Hu3DHookFuncCreate(DrawWaterRefraction);
    Hu3DModelLayerSet(object->mdlId[4], 1);
    Hu3DModelCameraSet(object->mdlId[4], 3);
    object->mdlId[5] = Hu3DHookFuncCreate(CaptureWaterTexture);
    Hu3DModelLayerSet(object->mdlId[5], 3);
    Hu3DModelCameraSet(object->mdlId[5], 3);
    object->mdlId[6] = Hu3DHookFuncCreate(CaptureWaterReflection);
    Hu3DModelLayerSet(object->mdlId[6], 7);
    Hu3DModelCameraSet(object->mdlId[6], 3);
    waterRenderStates->cameraIndex = 0;
    waterRenderStates->modelId = object->mdlId[0];
    CreateWaterRenderBuffers(object, waterRenderStates);
    waterRenderStates[1].cameraIndex = 1;
    waterRenderStates[1].modelId = object->mdlId[1];
    CreateWaterRenderBuffers(object, &waterRenderStates[1]);
    object->objFunc = InitializeWaterRenderStates;
}

void InitializeWaterRenderStates(OMOBJ *object)
{
    waterRenderStates[0].textureCopied = waterRenderStates[1].textureCopied = 0;
    waterRenderStates[0].waterRefractionReady = waterRenderStates[1].waterRefractionReady = 0;
    waterRenderStates[0].waterSurfaceReady = waterRenderStates[1].waterSurfaceReady = 0;
    waterRenderStates[0].reflectionReady = waterRenderStates[1].reflectionReady = 0;
    waterRenderStates[0].reflectionTextureReady = waterRenderStates[1].reflectionTextureReady = 0;
}

void CreateWaterRenderBuffers(OMOBJ *object, M430WaterRenderState *renderState)
{
    float vertexZ;
    float vertexXOffset;
    s32 vertexIndex;
    Vec *vertex;
    GXColor *color;
    s32 colorIndex;

    s32 waterGridRowOffsets[8] = { 0, 0x16, 1, 0x17, 1, 0, 0x17, 0x16 };

    renderState->waterVertices = HuMemDirectMallocNum(HEAP_HEAP, 0x1B8 * sizeof(Vec), HU_MEMNUM_OVL);
    renderState->reflectionVertices = HuMemDirectMallocNum(HEAP_HEAP, 0x1B8 * sizeof(Vec), HU_MEMNUM_OVL);
    renderState->displayListBuffer = HuMemDirectMallocNum(HEAP_HEAP, 0x14A0, HU_MEMNUM_OVL);
    renderState->reflectionDisplayList = HuMemDirectMallocNum(HEAP_HEAP, 0x6E0, HU_MEMNUM_OVL);
    renderState->activeBufferIndex = 0;
    for (vertexIndex = 0; vertexIndex < 2; vertexIndex++) {
        renderState->vertexBuffers[vertexIndex] = HuMemDirectMallocNum(HEAP_HEAP, 0x1B8 * sizeof(Vec), HU_MEMNUM_OVL);
        renderState->colorBuffers[vertexIndex] = HuMemDirectMallocNum(HEAP_HEAP, 0x1B8 * sizeof(GXColor), HU_MEMNUM_OVL);
    }
    renderState->displayListSize = 0x12D0;
    renderState->displayList = HuMemDirectMallocNum(HEAP_HEAP, renderState->displayListSize, HU_MEMNUM_OVL);
    DCFlushRange(renderState->displayList, renderState->displayListSize);
    renderState->displayListLength = 0;
    renderState->textureBufferSize = GXGetTexBufferSize(0x140, 0x1E0, 5, 0, 0);
    if (renderState->cameraIndex == 0) {
        renderState->textureBufferSize *= 2;
    }
    renderState->textureBuffer = HuMemDirectMallocNum(HEAP_HEAP, renderState->textureBufferSize, HU_MEMNUM_OVL);
    DCFlushRange(renderState->textureBuffer, renderState->textureBufferSize);
    renderState->scrollOffset = 0;
    renderState->frameCounter = 0;
    renderState->waterOffset = 0.0f;
    renderState->waterLevel = 0.0f;
    vertex = renderState->waterVertices;
    vertexZ = 0.0f;
    vertexXOffset = -2000.0f;
    for (vertexIndex = 0; vertexIndex < 0x14; vertexIndex++) {
        vertexZ = 65.47619f - vertexZ;
        for (colorIndex = 0; colorIndex < 0x16; colorIndex++) {
            vertex->x = ((130.95238f * colorIndex) - 1375.0f) + vertexZ;
            vertex->y = 0.0f;
            vertex->z = vertexXOffset;
            vertex++;
        }
        vertexXOffset += 210.52632f;
    }
    memset(renderState->reflectionVertices, 0, 0x1B8 * sizeof(Vec));
    for (vertex = renderState->reflectionVertices, vertexIndex = 0; vertexIndex < 0x1B8; vertexIndex++, vertex++) {
        vertex->y = (0.05f * frandmod(0x3E8)) - 20.0f;
    }
    for (vertexIndex = 0; vertexIndex < 2; vertexIndex++) {
        memcpy(renderState->vertexBuffers[vertexIndex], renderState->waterVertices, 0x1B8 * sizeof(Vec));
        for (color = renderState->colorBuffers[vertexIndex], colorIndex = 0; colorIndex < 0x1B8; colorIndex++, color++) {
            color->r = 0x32;
            color->g = 0x8C;
            color->b = 0xB4;
            color->a = 0;
        }
    }
    UpdateWaterSurfaceVertices(object, renderState);
    BuildWaterDisplayList(object, renderState);
}

void UpdateWaterSurfaceVertices(OMOBJ *object, M430WaterRenderState *renderState)
{
    Vec waveReferencePosition;
    Vec vertexToReference;
    float alpha;
    float distanceRatio;
    float cameraPositionX;
    float waterPositionX;
    GXColor *vertexColor;
    s32 vertexIndex;
    Vec *waterVertex;
    Vec *reflectionVertex;
    Vec *outputVertex;
    u32 cameraIndex;
    s32 firstVertexCount;
    s32 secondVertexCount;
    M430WaterSystemWork *waterWorkArray;
    M430WaterSystemWork *waterWork;

    if (omPauseChk() == 0) {
        renderState->scrollOffset++;
    }
    renderState->activeBufferIndex = 1 - renderState->activeBufferIndex;
    cameraIndex = renderState->cameraIndex;
    waterWork = waterSystemObject->data;
    cameraIndex &= 1;
    waterWorkArray = waterWork;
    cameraPositionX = waterWorkArray[cameraIndex].cameraPosition.x;
    waterPositionX = cameraPositionX;
    distanceRatio = waterPositionX;
    firstVertexCount = 0.002375f * distanceRatio;
    renderState->waterLevel = 421.05264f * firstVertexCount;
    renderState->waterOffset = distanceRatio - renderState->waterLevel;
    renderState->frameCounter = (firstVertexCount * 2) % 20;
    secondVertexCount = (renderState->frameCounter + 0x14) % 20;
    Hu3DModelPosSet(renderState->modelId, GetCameraHorizontalOffset(renderState->cameraIndex), 0.0f, renderState->waterLevel - 500.0f);
    waterVertex = renderState->waterVertices;
    vertexColor = renderState->colorBuffers[renderState->activeBufferIndex];
    waveReferencePosition.x = 0.0f;
    waveReferencePosition.y = 0.0f;
    waveReferencePosition.z = 2000.0f + renderState->waterOffset;
    if (isWaterSurfaceMoving != 0) {
        for (vertexIndex = 0; vertexIndex < 0x1B8; vertexIndex++) {
            VECSubtract(waterVertex, &waveReferencePosition, &vertexToReference);
            alpha = 0.00025f * VECMag(&vertexToReference);
            if (alpha < 0.3f) {
                vertexColor->a = -1;
            }
            else if (alpha > 0.75f) {
                vertexColor->a = 0;
            }
            else {
                alpha = 255.0f - (566.6667f * (alpha - 0.3f));
                vertexColor->a = alpha;
            }
            waterVertex++;
            vertexColor++;
        }
    }
    else {
        for (vertexIndex = 0; vertexIndex < 0x1B8; vertexIndex++) {
            VECSubtract(waterVertex, &waveReferencePosition, &vertexToReference);
            alpha = 0.00025f * VECMag(&vertexToReference);
            if (alpha < 0.25f) {
                vertexColor->a = -0x1A;
            }
            else if (alpha > 0.85f) {
                vertexColor->a = 0;
            }
            else {
                alpha = 230.0f - (383.3333f * (alpha - 0.25f));
                vertexColor->a = alpha;
            }
            waterVertex++;
            vertexColor++;
        }
    }
    if (isWaterSurfaceMoving != 0) {
        memcpy(renderState->vertexBuffers[renderState->activeBufferIndex], renderState->waterVertices, 0x1B8 * sizeof(Vec));
    }
    else {
        waterVertex = renderState->waterVertices;
        outputVertex = renderState->vertexBuffers[renderState->activeBufferIndex];
        vertexIndex = (0x14 - secondVertexCount) * 0x16;
        reflectionVertex = &renderState->reflectionVertices[0x1B8 - vertexIndex];
        while (vertexIndex-- != 0) {
            VECAdd(waterVertex, reflectionVertex, outputVertex);
            waterVertex++;
            reflectionVertex++;
            outputVertex++;
        }
        vertexIndex = secondVertexCount * 0x16;
        reflectionVertex = renderState->reflectionVertices;
        while (vertexIndex-- != 0) {
            VECAdd(waterVertex, reflectionVertex, outputVertex);
            waterVertex++;
            reflectionVertex++;
            outputVertex++;
        }
        memcpy(renderState->vertexBuffers[renderState->activeBufferIndex], renderState->waterVertices, 0x16 * sizeof(Vec));
    }
    DCStoreRangeNoSync(renderState->vertexBuffers[renderState->activeBufferIndex], 0x1B8 * sizeof(Vec));
    DCStoreRangeNoSync(renderState->colorBuffers[renderState->activeBufferIndex], 0x1B8 * sizeof(GXColor));
    PPCSync();
}

void BuildWaterDisplayList(OMOBJ *object, M430WaterRenderState *renderState)
{
    s32 stripIndex;
    s32 stripRow;
    s32 vertexIndex;
    s32 vertexParity;
    s32 rowIndex;
    s32 vertexArrayIndex;
    s32 stripVertexCount;

    s32 rowStartOffsets[5] = { 0, 0, 0, 1, 1 };
    s32 rowDirections[4] = { 1, 0, 0, 0 };
    s32 rowParityOffsets[4] = { 0, 0, 1, 2 };
    GXBeginDisplayList(renderState->displayList, renderState->displayListSize);
    rowIndex = 0;
    stripIndex = 0;
    for (stripRow = 0; stripRow < 0x13; stripRow++) {
        stripVertexCount = 0x2B - (((rowIndex * 2) + rowParityOffsets[stripIndex]) * 2);
        vertexParity = rowDirections[stripIndex];
        GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, stripVertexCount);
        for (vertexIndex = 0; vertexIndex < stripVertexCount; vertexIndex++) {
            vertexArrayIndex = (vertexIndex >> 1) + ((stripRow + vertexParity) * 0x16) + (rowIndex + rowStartOffsets[stripIndex + vertexParity]);
            GXUnknownu16(vertexArrayIndex);
            GXUnknownu16(vertexArrayIndex);
            vertexParity = 1 - vertexParity;
        }
        stripIndex++;
        if (stripIndex >= 4) {
            stripIndex = 0;
            rowIndex++;
        }
    }
    renderState->displayListLength = GXEndDisplayList();
}

void DrawWaterSurface(HU3DMODEL *model, Mtx matrix)
{
    Mtx normalMatrix;
    Mtx waterTextureMatrix;
    Mtx waterModelMatrix;
    Mtx lightPerspectiveMatrix;
    Mtx cameraInverseMatrix;
    Mtx lightTextureMatrix;
    Mtx reflectionPerspectiveMatrix;
    Mtx reflectionTextureMatrix;
    GXTexObj waterTexture;
    float textureScale;
    float sceneScale;
    M430WaterRenderState *renderState;

    GXColor whiteColor = { 0xFF, 0xFF, 0xFF, 0xFF };
    GXColor darkColor = { 0x08, 0x08, 0x08, 0xFF };
    GXColor reflectionColor = { 0x00, 0x00, 0x00, 0x80 };
    GXColor waterTint = { 0xCC, 0xBF, 0xB2, 0x33 };
    GXColor foamTint = { 0xE0, 0xD8, 0xF4, 0x08 };
    renderState = waterRenderStates;
    if (renderState->waterSurfaceReady != 0) {
        renderState = &renderState[1];
    }
    renderState->waterSurfaceReady = 1;
    DrawWaterReflection(model, matrix, renderState->cameraIndex);
    MTXCopy(matrix, waterModelMatrix);
    if (isMinigameActive() != 0) {
        sceneScale = 1.0f;
        textureScale = GetCameraHorizontalOffset(renderState->cameraIndex);
    }
    else {
        sceneScale = 1.5f;
        textureScale = 0.0f;
    }
    MTXTrans(waterTextureMatrix, textureScale - GetCameraHorizontalOffset(renderState->cameraIndex), 0.0f, 0.0f);
    MTXConcat(waterModelMatrix, waterTextureMatrix, waterModelMatrix);
    MTXScale(normalMatrix, sceneScale, 1.0f, 1.0f);
    MTXConcat(waterModelMatrix, normalMatrix, waterModelMatrix);
    GXLoadPosMtxImm(waterModelMatrix, 0);
    MTXInvXpose(waterModelMatrix, normalMatrix);
    GXLoadNrmMtxImm(normalMatrix, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    if ((isMinigameActive() == 0) && (renderState->cameraIndex == 0)) {
        GXInitTexObj(&waterTexture, renderState->textureBuffer, 0x280, 0x1E0, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, 0);
    }
    else {
        GXInitTexObj(&waterTexture, renderState->textureBuffer, 0x140, 0x1E0, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, 0);
    }
    GXInitTexObjLOD(&waterTexture, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXLoadTexObj(&waterTexture, GX_TEXMAP0);
    HuSprTexLoad(waterWaveAnimation, 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    HuSprTexLoad(waterReflectionTextureAnimation, 0, 2, GX_CLAMP, GX_CLAMP, GX_LINEAR);
    GXSetNumTexGens(3);
    if (isMinigameActive() != 0) {
        C_MTXLightPerspective(lightPerspectiveMatrix, 41.5f, 0.6f, 0.5f, -0.5f, 0.5f, 0.5f);
    }
    else {
        C_MTXLightPerspective(lightPerspectiveMatrix, 41.5f, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
    }
    MTXInverse(Hu3DCameraMtx, cameraInverseMatrix);
    MTXConcat(cameraInverseMatrix, waterModelMatrix, cameraInverseMatrix);
    MTXConcat(lightPerspectiveMatrix, Hu3DCameraMtx, lightTextureMatrix);
    MTXConcat(lightTextureMatrix, cameraInverseMatrix, cameraInverseMatrix);
    GXLoadTexMtxImm(cameraInverseMatrix, 0x1E, GX_MTX3x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, 0x1E, GX_FALSE, 0x7D);
    C_MTXLightPerspective(lightPerspectiveMatrix, 70.0f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, reflectionPerspectiveMatrix);
    MTXConcat(reflectionPerspectiveMatrix, waterModelMatrix, reflectionPerspectiveMatrix);
    MTXConcat(lightPerspectiveMatrix, Hu3DCameraMtx, reflectionTextureMatrix);
    MTXConcat(reflectionTextureMatrix, reflectionPerspectiveMatrix, reflectionPerspectiveMatrix);
    GXLoadTexMtxImm(reflectionPerspectiveMatrix, 0x24, GX_MTX3x4);
    GXSetTexCoordGen2(GX_TEXCOORD2, GX_TG_MTX3x4, GX_TG_POS, 0x24, GX_FALSE, 0x7D);
    MTXRotRad(waterTextureMatrix, 0x58, 1.5707964f);
    textureScale = 0.00075f * sceneScale;
    MTXScale(normalMatrix, textureScale, -0.003f, 0.00075f);
    MTXConcat(waterTextureMatrix, normalMatrix, waterTextureMatrix);
    MTXTrans(normalMatrix, (renderState->scrollOffset & 0x3FF) / (1024.0f * textureScale), 0.0f, (0.7f * renderState->waterLevel) - (0.3f * renderState->waterOffset));
    MTXConcat(waterTextureMatrix, normalMatrix, waterTextureMatrix);
    waterTextureMatrix[0][1] = -0.001f;
    GXLoadTexMtxImm(waterTextureMatrix, 0x21, GX_MTX2x4);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_POS, 0x21, GX_FALSE, 0x7D);
    GXSetNumIndStages(2);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD1, GX_TEXMAP1);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
    GXSetTevIndWarp(0, 0, GX_TRUE, GX_FALSE, 1);
    MTXScale(waterTextureMatrix, -0.25f, -0.5f, 0.5f);
    GXSetIndTexMtx(GX_ITM_0, (float(*)[3])waterTextureMatrix, -4);
    GXSetIndTexOrder(GX_INDTEXSTAGE1, GX_TEXCOORD1, GX_TEXMAP1);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE1, GX_ITS_1, GX_ITS_1);
    GXSetTevIndWarp(1, 1, GX_TRUE, GX_FALSE, 2);
    MTXScale(waterTextureMatrix, 0.4f, 0.5f, 0.5f);
    GXSetIndTexMtx(GX_ITM_1, (float(*)[3])waterTextureMatrix, -1);
    GXSetNumTevStages(2);
    GXSetTevColor(GX_TEVREG0, waterTint);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_TEXC, GX_CC_RASA, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_ZERO, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD2, GX_TEXMAP2, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A0, GX_CC_CPREV);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);
    GXSetZMode(1, GX_LEQUAL, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_POS, renderState->vertexBuffers[renderState->activeBufferIndex], sizeof(Vec));
    GXSetArray(GX_VA_CLR0, renderState->colorBuffers[renderState->activeBufferIndex], sizeof(GXColor));
    GXSetCullMode(GX_CULL_NONE);
    GXCallDisplayList(renderState->displayList, renderState->displayListLength);
    GXSetCullMode(GX_CULL_BACK);
    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevDirect(GX_TEVSTAGE1);
}

Vec waterSurfaceVertices[6] = {
    { -2750.0f, 0.0f, -2500.0f },
    { 2750.0f, 0.0f, -2500.0f },
    { -2750.0f, 0.0f, 0.0f },
    { 2750.0f, 0.0f, 0.0f },
    { -1375.0f, 0.0f, 2500.0f },
    { 1375.0f, 0.0f, 2500.0f },
};
GXColor waterSurfaceColors[6] = {
    { 5, 0x38, 0xA2, 0 },
    { 5, 0x38, 0xA2, 0xFF },
    { 0x14, 0x54, 0xAB, 0xFF },
    { 0x1E, 0x6D, 0xB0, 0xFF },
    { 0x2A, 0x7E, 0xB4, 0xFF },
    { 0x32, 0x8C, 0xB4, 0xFF },
};

void DrawWaterReflection(HU3DMODEL *model, Mtx matrix, u32 cameraIndex)
{
    Mtx scaleMatrix;
    Mtx waterTransform;
    Mtx modelViewMatrix;
    Mtx lightPerspectiveMatrix;
    Mtx cameraInverseMatrix;
    Mtx lightTextureMatrix;
    float textureScale;
    float sceneScale;

    GXColor waterTint = { 0x32, 0x8C, 0xB4, 0x33 };
    if (isMinigameActive() != 0) {
        sceneScale = 1.0f;
        textureScale = GetCameraHorizontalOffset(cameraIndex);
    }
    else {
        sceneScale = 1.5f;
        textureScale = 0.0f;
    }
    MTXTrans(waterTransform, textureScale, 0.0f, ((waterRenderStates[cameraIndex].waterLevel + waterRenderStates[cameraIndex].waterOffset) - 2500.0f) - 2000.0f);
    MTXScale(scaleMatrix, sceneScale, 1.0f, 1.0f);
    MTXConcat(waterTransform, scaleMatrix, waterTransform);
    MTXConcat(Hu3DCameraMtx, waterTransform, modelViewMatrix);
    GXLoadPosMtxImm(modelViewMatrix, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    HuSprTexLoad(waterFoamTextureAnimation, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    HuSprTexLoad(waterReflectionTextureAnimation, 0, 1, GX_CLAMP, GX_CLAMP, GX_LINEAR);
    GXSetNumTexGens(2);
    C_MTXLightPerspective(lightPerspectiveMatrix, 70.0f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, cameraInverseMatrix);
    MTXConcat(cameraInverseMatrix, modelViewMatrix, cameraInverseMatrix);
    MTXConcat(lightPerspectiveMatrix, Hu3DCameraMtx, lightTextureMatrix);
    MTXConcat(lightTextureMatrix, cameraInverseMatrix, cameraInverseMatrix);
    GXLoadTexMtxImm(cameraInverseMatrix, 0x21, GX_MTX3x4);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX3x4, GX_TG_POS, 0x21, GX_FALSE, 0x7D);
    MTXRotRad(waterTransform, 0x58, 1.5707964f);
    textureScale = 0.00075f * sceneScale;
    MTXScale(scaleMatrix, textureScale, -0.003f, 0.00075f);
    MTXConcat(waterTransform, scaleMatrix, waterTransform);
    MTXTrans(scaleMatrix, (waterRenderStates[cameraIndex].scrollOffset & 0x3FF) / (1024.0f * textureScale), 0.0f,
        0.3f * (waterRenderStates[cameraIndex].waterLevel + waterRenderStates[cameraIndex].waterOffset));
    MTXConcat(waterTransform, scaleMatrix, waterTransform);
    GXLoadTexMtxImm(waterTransform, 0x1E, GX_MTX2x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_POS, 0x1E, GX_FALSE, 0x7D);
    GXSetNumIndStages(1);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD0, GX_TEXMAP0);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
    GXSetTevIndWarp(0, 0, GX_TRUE, GX_FALSE, 1);
    MTXScale(waterTransform, 0.5f, 0.6f, 0.6f);
    GXSetIndTexMtx(GX_ITM_0, (float(*)[3])waterTransform, -1);
    GXSetNumTevStages(1);
    GXSetTevColor(GX_TEVREG0, waterTint);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A0, GX_CC_RASC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_SET);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_POS, waterSurfaceVertices, sizeof(Vec));
    GXSetArray(GX_VA_CLR0, waterSurfaceColors, sizeof(GXColor));
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 6);
    GXPosition1x8(0);
    GXColor1x8(3);
    GXPosition1x8(1);
    GXColor1x8(3);
    GXPosition1x8(2);
    GXColor1x8(4);
    GXPosition1x8(3);
    GXColor1x8(4);
    GXPosition1x8(4);
    GXColor1x8(5);
    GXPosition1x8(5);
    GXColor1x8(5);
    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
}

Vec waterReflectionVertices[8] = {
    { -2750.0f, 20.0f, 0.0f },
    { 2750.0f, 20.0f, 0.0f },
    { -2750.0f, -20.0f, 0.0f },
    { 2750.0f, -20.0f, 0.0f },
    { -2750.0f, -160.0f, 0.0f },
    { 2750.0f, -160.0f, 0.0f },
    { -2750.0f, -340.0f, 0.0f },
    { 2750.0f, -340.0f, 0.0f },
};
Vec waterRefractionVertices[6] = {
    { -700.0f, 0.0f, -1750.0f },
    { 700.0f, 0.0f, -1750.0f },
    { -700.0f, 0.0f, -1400.0f },
    { 700.0f, 0.0f, -1400.0f },
    { -700.0f, 0.0f, 700.0f },
    { 700.0f, 0.0f, 700.0f },
};

void DrawWaterSurfaceReflection(HU3DMODEL *model, Mtx matrix)
{
    Mtx textureTransform;
    Mtx reflectionTransform;
    Mtx modelViewMatrix;
    Mtx lightPerspectiveMatrix;
    Mtx cameraInverseMatrix;
    Mtx lightTextureMatrix;
    Vec cameraTarget;
    Vec cameraPosition;
    Vec cameraDirection;
    float sceneScale;
    float textureScale;
    s32 cameraIndex;
    s32 vertexIndex;

    GXColor waterTint = { 0x32, 0x8C, 0xB4, 0x33 };
    cameraIndex = 0;
    if (waterRenderStates[cameraIndex].reflectionReady != 0) {
        cameraIndex++;
    }
    waterRenderStates[cameraIndex].reflectionReady = 1;
    cameraPosition.x = CenterM[cameraIndex].x;
    cameraPosition.y = CenterM[cameraIndex].y + (CZoomM[cameraIndex] * sind(-CRotM[cameraIndex].x));
    cameraPosition.z = CenterM[cameraIndex].z + (CZoomM[cameraIndex] * cosd(-CRotM[cameraIndex].x));
    cameraTarget.x = cameraPosition.x;
    cameraTarget.y = 0.0f;
    cameraTarget.z = cameraPosition.z - 450000.0f;
    VECSubtract(&cameraTarget, &cameraPosition, &cameraDirection);
    VECNormalize(&cameraDirection, &cameraDirection);
    VECScale(&cameraDirection, &cameraDirection, 8000.0f);
    VECAdd(&cameraPosition, &cameraDirection, &cameraTarget);
    MTXTrans(reflectionTransform, cameraTarget.x, cameraTarget.y, cameraTarget.z);
    sceneScale = 1.0f;
    if (isMinigameActive() == 0) {
        sceneScale = 1.5f;
        MTXScale(textureTransform, sceneScale, 1.0f, 1.0f);
        MTXConcat(reflectionTransform, textureTransform, reflectionTransform);
    }
    MTXConcat(Hu3DCameraMtx, reflectionTransform, modelViewMatrix);
    GXLoadPosMtxImm(modelViewMatrix, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    HuSprTexLoad(waterFoamTextureAnimation, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    HuSprTexLoad(waterReflectionTextureAnimation, 0, 1, GX_CLAMP, GX_CLAMP, GX_LINEAR);
    GXSetNumTexGens(2);
    C_MTXLightPerspective(lightPerspectiveMatrix, 70.0f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, cameraInverseMatrix);
    MTXConcat(cameraInverseMatrix, modelViewMatrix, cameraInverseMatrix);
    MTXConcat(lightPerspectiveMatrix, Hu3DCameraMtx, lightTextureMatrix);
    MTXConcat(lightTextureMatrix, cameraInverseMatrix, cameraInverseMatrix);
    GXLoadTexMtxImm(cameraInverseMatrix, 0x21, GX_MTX3x4);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX3x4, GX_TG_POS, 0x21, GX_FALSE, 0x7D);
    textureScale = 0.00075f * sceneScale;
    MTXScale(reflectionTransform, textureScale, -0.005f, 0.00075f);
    MTXTrans(textureTransform, (waterRenderStates[cameraIndex].scrollOffset & 0x3FF) / (1024.0f * textureScale),
        0.03f * -(waterRenderStates[cameraIndex].waterLevel + waterRenderStates[cameraIndex].waterOffset), 0.0f);
    MTXConcat(reflectionTransform, textureTransform, reflectionTransform);
    GXLoadTexMtxImm(reflectionTransform, 0x1E, GX_MTX2x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_POS, 0x1E, GX_FALSE, 0x7D);
    GXSetNumIndStages(1);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD0, GX_TEXMAP0);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
    GXSetTevIndWarp(0, 0, GX_TRUE, GX_FALSE, 1);
    MTXScale(reflectionTransform, 0.5f, 0.6f, 0.6f);
    GXSetIndTexMtx(GX_ITM_0, (float(*)[3])reflectionTransform, -1);
    GXSetNumTevStages(1);
    GXSetTevColor(GX_TEVREG0, waterTint);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A0, GX_CC_RASC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_POS, waterReflectionVertices, 0xC);
    GXSetArray(GX_VA_CLR0, waterSurfaceColors, 4);
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 8);
    for (vertexIndex = 0; vertexIndex < 8; vertexIndex++) {
        GXPosition1x8(vertexIndex);
        GXColor1x8(vertexIndex >> 1);
    }
    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
}

GXColor waterRefractionColors[2] = { { 0xFF, 0xFF, 0xFF, 0 }, { 0xFF, 0xFF, 0xFF, 0xFF } };

void DrawWaterRefraction(HU3DMODEL *model, Mtx matrix)
{
    Mtx textureTransform;
    Mtx waterModelMatrix;
    float textureScale;
    float sceneScale;
    u8 *textureData;
    u8 *textureAlpha;

    GXColor shallowWaterTint = { 0xFF, 0xA5, 0x59, 0xB2 };
    GXColor deepWaterTint = { 0x99, 0x72, 0x59, 0xB2 };
    s32 cameraIndex = 0;
    if (waterRenderStates[cameraIndex].waterRefractionReady != 0) {
        cameraIndex++;
    }
    waterRenderStates[cameraIndex].waterRefractionReady = 1;
    if (isMinigameActive() != 0) {
        sceneScale = 1.0f;
        textureScale = GetCameraHorizontalOffset(cameraIndex);
    }
    else {
        sceneScale = 1.5f;
        textureScale = 0.0f;
    }
    if (isWaterSurfaceMoving != 0) {
        MTXTrans(waterModelMatrix, textureScale, 0.0f, 300.0f + (waterRenderStates[cameraIndex].waterLevel + waterRenderStates[cameraIndex].waterOffset));
        GXSetTevColor(GX_TEVREG0, deepWaterTint);
    }
    else {
        MTXTrans(waterModelMatrix, textureScale, -150.0f, waterRenderStates[cameraIndex].waterLevel + waterRenderStates[cameraIndex].waterOffset);
        GXSetTevColor(GX_TEVREG0, shallowWaterTint);
    }
    MTXScale(textureTransform, sceneScale, 1.0f, 1.0f);
    MTXConcat(waterModelMatrix, textureTransform, waterModelMatrix);
    MTXConcat(Hu3DCameraMtx, waterModelMatrix, waterModelMatrix);
    GXLoadPosMtxImm(waterModelMatrix, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    HuSprTexLoad(waterSurfaceTextureAnimation, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    HuSprTexLoad(waterRippleTextureAnimation, 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetNumTexGens(2);
    MTXRotRad(waterModelMatrix, 0x58, 1.5707964f);
    textureScale = 0.004f * sceneScale;
    MTXScale(textureTransform, textureScale, -0.004f, 0.004f);
    MTXConcat(waterModelMatrix, textureTransform, waterModelMatrix);
    MTXTrans(textureTransform, (waterRenderStates[cameraIndex].scrollOffset & 0x3FF) / (512.0f * textureScale), 0.0f,
        (1.2499999f * (waterRenderStates[cameraIndex].scrollOffset % 400)) + (waterRenderStates[cameraIndex].waterLevel + waterRenderStates[cameraIndex].waterOffset));
    MTXConcat(waterModelMatrix, textureTransform, waterModelMatrix);
    GXLoadTexMtxImm(waterModelMatrix, 0x1E, GX_MTX2x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_POS, 0x1E, GX_FALSE, 0x7D);
    MTXRotRad(waterModelMatrix, 0x58, 1.5707964f);
    textureScale = 0.004f * sceneScale;
    MTXScale(textureTransform, textureScale, -0.004f, 0.004f);
    MTXConcat(waterModelMatrix, textureTransform, waterModelMatrix);
    MTXTrans(textureTransform, (waterRenderStates[cameraIndex].scrollOffset % 300) / (150.0f * textureScale), 0.0f, waterRenderStates[cameraIndex].waterLevel + waterRenderStates[cameraIndex].waterOffset);
    MTXConcat(waterModelMatrix, textureTransform, waterModelMatrix);
    GXLoadTexMtxImm(waterModelMatrix, 0x21, GX_MTX2x4);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_POS, 0x21, GX_FALSE, 0x7D);
    GXSetNumTevStages(2);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_CPREV);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_SET);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_POS, waterRefractionVertices, 0xC);
    GXSetArray(GX_VA_CLR0, waterRefractionColors, 4);
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 6);
    GXPosition1x8(0);
    GXColor1x8(0);
    GXPosition1x8(1);
    GXColor1x8(0);
    GXPosition1x8(2);
    GXColor1x8(1);
    GXPosition1x8(3);
    GXColor1x8(1);
    GXPosition1x8(4);
    GXColor1x8(1);
    GXPosition1x8(5);
    GXColor1x8(1);
}

void CaptureWaterTexture(HU3DMODEL *model, Mtx matrix)
{
    s32 cameraIndex = 0;
    if (waterRenderStates[cameraIndex].textureCopied != 0) {
        cameraIndex++;
    }
    if ((isMinigameActive() == 0) && (cameraIndex == 0)) {
        GXSetTexCopySrc(0, 0, 0x280, 0x1E0);
        GXSetTexCopyDst(0x280, 0x1E0, GX_TF_RGB5A3, 0);
    }
    else {
        GXSetTexCopySrc(cameraIndex * 0x140, 0, 0x140, 0x1E0);
        GXSetTexCopyDst(0x140, 0x1E0, GX_TF_RGB5A3, 0);
    }
    GXCopyTex(waterRenderStates[cameraIndex].textureBuffer, 0);
    GXPixModeSync();
    waterRenderStates[cameraIndex].textureCopied = 1;
}

void CaptureWaterReflection(HU3DMODEL *model, Mtx matrix)
{
    s32 cameraIndex = 0;
    if (waterRenderStates[cameraIndex].reflectionTextureReady != 0) {
        cameraIndex++;
    }
    waterRenderStates[cameraIndex].reflectionTextureReady = 1;
    UpdateWaterSurfaceVertices(waterParticleObject, &waterRenderStates[cameraIndex]);
}

void DrawWaterParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;

    if ((particle->dataCnt == 0) || (isWaterSurfaceMoving != 0)) {
        particle->dataCnt++;
        for (particleData = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->time = 0;
            particleData->scale = 0.0f;
            particleData->color.a = 0;
        }
        return;
    }
    for (particleData = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->time != 0) {
            VECAdd(&particleData->pos, &particleData->vel, &particleData->pos);
            VECScale(&particleData->vel, &particleData->vel, 0.97f);
            if (particleData->time < REFRESH_RATE_F / 2.5f) {
                particleData->color.a = 0.9f * particleData->color.a;
            }
            particleData->scale += 1.5f;
            if (--particleData->time == 0) {
                particleData->time = 0;
                particleData->scale = 0.0f;
                particleData->color.a = 0;
            }
        }
    }
    DCStoreRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void EmitSurfaceSplashParticles(s32 cameraIndex, Vec *splashPosition, float sprayAngle, float effectRate)
{
    Vec particleDirection;
    float particleSpeed;
    float particleAngle;
    float angleSpread;
    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleSystem;
    s32 particleIndex;
    M430WaterSystemWork *waterWorkArray;
    u32 particlesToSpawn;
    void *waterWork;

    waterWork = waterSystemObject->data;
    waterWorkArray = waterWork;
    particleSystem = Hu3DData[waterWorkArray[cameraIndex].waterSurfaceModelId].hookData;
    if (particleSystem->dataCnt != 0) {
        particleData = particleSystem->data;
        if (!(frandmod(0x3E8) > (1200.0f * effectRate))) {
            particlesToSpawn = 1;
            angleSpread = 30.0f;
            for (particleIndex = 0; particleIndex < particleSystem->maxCnt; particleIndex++, particleData++) {
                if (particleData->time == 0) {
                    particleData->time = REFRESH_RATE_F * (0.5f + (0.0005f * frandmod(0x3E8)));
                    particleAngle = (0.002f * frandmod(0x3E8)) - 1.0f;
                    particleAngle = sprayAngle + (particleAngle * angleSpread);
                    particleDirection.x = sind(particleAngle);
                    particleDirection.y = 0.0f;
                    particleDirection.z = cosd(particleAngle);
                    particleData->pos = *splashPosition;
                    particleData->pos.y = -20.0f;
                    particleSpeed = (300.0f / REFRESH_RATE_F) + effectRate * (REFRESH_FREQ * 100.0f * (2.0f * (0.001f * frandmod(0x3E8))));
                    particleData->vel.x = particleDirection.x * particleSpeed;
                    particleData->vel.y = -0.2f * particleSpeed;
                    particleData->vel.z = particleDirection.z * particleSpeed;
                    particleSpeed = 0.001f * frandmod(0x3E8);
                    particleSpeed += (1.0f - particleSpeed) * (0.5f * effectRate);
                    particleData->zRot = frandmod(0x168);
                    particleData->scale = 80.0f + frandmod(0x46);
                    particleData->color.a = frandmod(0x64) + 0x64;
                    particleData->color.r = 150.0f + (95.0f * particleSpeed);
                    particleData->color.g = 160.0f + (80.0f * particleSpeed);
                    particleData->color.b = 166.0f + (60.0f * particleSpeed);
                    if (--particlesToSpawn == 0) {
                        break;
                    }
                }
            }
        }
    }
}

void DrawUnderwaterParticles(HU3DMODEL *model, HU3DPARTICLE *particleSystem, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;

    if ((particleSystem->dataCnt == 0) || (isWaterSurfaceMoving != 0)) {
        particleSystem->dataCnt++;
        for (particleData = particleSystem->data, particleIndex = 0; particleIndex < particleSystem->maxCnt; particleIndex++, particleData++) {
            particleData->time = 0;
            particleData->scale = 0.0f;
            particleData->color.a = 0;
        }
        return;
    }
    for (particleData = particleSystem->data, particleIndex = 0; particleIndex < particleSystem->maxCnt; particleIndex++, particleData++) {
        if (particleData->time != 0) {
            particleData->time--;
            VECAdd(&particleData->pos, &particleData->vel, &particleData->pos);
            particleData->scale += 0.5f;
            if (particleData->time < REFRESH_RATE_F / 2.0) {
                particleData->color.a = 0.98f * particleData->color.a;
            }
            if ((particleData->pos.y < -40.0f) || (particleData->time == 0)) {
                particleData->time = 0;
                particleData->scale = 0.0f;
                particleData->color.a = 0;
            }
        }
    }
    DCStoreRange(particleSystem->data, particleSystem->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void EmitUnderwaterSplashParticles(s32 cameraIndex, Vec *splashPosition, float sprayAngle, float effectRate, Vec *waterFlow)
{
    Vec particleDirection;
    Vec currentFlow;
    s32 unusedParticleFlag;
    float particleSpeed;
    float particleAngle;
    float angleSpread;
    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleSystem;
    s32 particleIndex;
    M430WaterSystemWork *waterWorkArray;
    u32 particlesToSpawn;
    M430WaterSystemWork *waterWork;

    waterWork = waterSystemObject->data;
    {
        float sprayAngleOffsets[2] = { -100.0f, 100.0f };
        waterWorkArray = waterWork;
        particleSystem = Hu3DData[waterWorkArray[cameraIndex].underwaterModelId].hookData;
        if (particleSystem->dataCnt != 0) {
            particleData = particleSystem->data;
            if (!(frandmod(0x3E8) > (1000.0f * effectRate))) {
                particlesToSpawn = 2;
                angleSpread = 40.0f;
                VECScale(waterFlow, &currentFlow, 0.2f);
                currentFlow.x = 0.0f;
                for (particleIndex = 0; particleIndex < particleSystem->maxCnt; particleIndex++, particleData++) {
                    if (particleData->time == 0) {
                        particleData->time = REFRESH_RATE_F * (0.5f + (0.0005f * frandmod(0x3E8)));
                        particleAngle = (0.002f * frandmod(0x3E8)) - 1.0f;
                        particleAngle = sprayAngle + sprayAngleOffsets[particleIndex & 1] + (particleAngle * angleSpread);
                        particleDirection.x = sind(particleAngle);
                        particleDirection.y = 0.0f;
                        particleDirection.z = cosd(particleAngle);
                        particleData->pos = *splashPosition;
                        particleData->pos.x = particleData->pos.x + ((0.05f * frandmod(0x3E8)) - 25.0f);
                        particleData->pos.z = particleData->pos.z + ((0.05f * frandmod(0x3E8)) - 25.0f);
                        particleData->pos.y = 40.0f;
                        particleSpeed = (REFRESH_FREQ * 100.0f) + (effectRate * (0.1f / REFRESH_RATE_F * frandmod(0x3E8)));
                        particleData->vel.x = particleDirection.x * particleSpeed;
                        particleData->vel.z = particleDirection.z * particleSpeed;
                        particleData->vel.y = 0.0f;
                        VECAdd(&particleData->vel, &currentFlow, &particleData->vel);
                        particleSpeed = 0.001f * frandmod(0x3E8);
                        particleSpeed += (1.0f - particleSpeed) * (0.3f * effectRate);
                        particleData->zRot = frandmod(0x168);
                        particleData->scale = 60.0f + frandmod(0x50);
                        particleData->color.a = (60.0f * effectRate) + frandmod(0x50);
                        particleData->color.r = 150.0f + (95.0f * particleSpeed);
                        particleData->color.g = 160.0f + (80.0f * particleSpeed);
                        particleData->color.b = 166.0f + (60.0f * particleSpeed);
                        if (--particlesToSpawn == 0) {
                            break;
                        }
                    }
                }
            }
        }
    }
}

float GetCameraHorizontalOffset(u32 cameraIndex)
{
    float cameraHorizontalOffsets[2] = { -450.0f, 450.0f };

    return cameraHorizontalOffsets[cameraIndex & 1];
}

float GetWaterLevel(void)
{
    return 0.0f;
}

void SetWaterSurfaceMoving(u32 isMoving)
{
    isWaterSurfaceMoving = 0;
    if (isMoving != 0) {
        isWaterSurfaceMoving = 1;
    }
}
