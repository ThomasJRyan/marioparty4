#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"

#include "game/frand.h"
#include "game/sprite.h"

#include "REL/m434Dll.h"
#include "ext_math.h"
#include "version.h"
#include <string.h>


typedef struct WaterRippleEffect {
    Vec position;
    u16 remainingFrames;
} WaterRippleEffect;

typedef struct DisplayListBuffer {
    void *displayListBuffer;
    u32 displayListSize;
} DisplayListBuffer;

typedef struct EnvironmentTexture {
    void *textureBuffer;
    u16 sourceWidth;
    u16 sourceHeight;
    u16 textureWidth;
    u16 textureHeight;
    u32 textureBufferSize;
    s32 copyMode;
} EnvironmentTexture;

typedef struct EnvironmentTextureConfig {
    u16 sourceWidth;
    u16 sourceHeight;
    u16 textureWidth;
    u16 textureHeight;
    u16 copyMode;
} EnvironmentTextureConfig;

EnvironmentTextureConfig environmentTextureConfigs[5] = {
    { 200, 200, 200, 200, 0 },
    { 320, 240, 160, 120, 1 },
    { 640, 480, 320, 240, 1 },
    { 640, 480, 640, 480, 0 },
    { 0, 0, 0, 0, 0 },
};

OMOBJ *environmentObject;
Vec oceanGridVertices[1024];
WaterRippleEffect waterRippleEffects[64];
s32 waterRippleEffectCount;
HuVec2f oceanWaveParameters[1024];
ANIMDATA *waterTextureAnimation;
ANIMDATA *oceanTextureAnimation;
ANIMDATA *environmentTextureAnimation;
DisplayListBuffer oceanSurfaceDisplayList;
DisplayListBuffer environmentDisplayList;
DisplayListBuffer waterRippleDisplayList;
EnvironmentTexture environmentTextures[4];
Vec waterSurfacePositions[2];

void InitializeEnvironmentObject(OMOBJ *object);

void CreateEnvironmentObject(HUPROCESS *objman)
{
    "\0"; // Force lbl_1_data_132 to 0
    environmentObject = omAddObjEx(objman, 10, 16, 0, -1, InitializeEnvironmentObject);
    CenterM[1].y = -CenterM[2].y + -15.000001f;
    CRotM[1].x = -CRotM[2].x;
}

void FreeEnvironmentTextures(void);

void ShutdownEnvironment(void)
{
    FreeEnvironmentTextures();
}

void UpdateEnvironmentObject(OMOBJ *object);

void DrawOceanSurface(HU3DMODEL *model, Mtx mtx);
void CreateOceanSurfaceDisplayList(void);
void CreateWaveDisplayList(void);

void DrawEnvironmentTextureLayer(HU3DMODEL *model, Mtx mtx);
void DrawEnvironmentTextureOverlay(HU3DMODEL *model, Mtx mtx);
void DrawEnvironmentTextureDetail(HU3DMODEL *model, Mtx mtx);
void DrawEnvironmentSurface(HU3DMODEL *model, Mtx mtx);

void UpdateEnvironmentParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

void InitializeEnvironmentObject(OMOBJ *object)
{
    s32 gridRow;
    Vec *gridPosition;
    HuVec2f *waveParameters;
    s32 gridColumn;
    ANIMDATA *particleAnimation;

    object->mdlId[9] = Hu3DHookFuncCreate(DrawEnvironmentSurface);
    Hu3DModelLayerSet(object->mdlId[9], 1);
    Hu3DModelCameraSet(object->mdlId[9], 1);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M434, 0x02));
    Hu3DModelLayerSet(object->mdlId[1], 2);
    Hu3DModelCameraSet(object->mdlId[1], 2);
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M434, 0x00));
    Hu3DModelLayerSet(object->mdlId[0], 2);
    Hu3DModelCameraSet(object->mdlId[0], 8);
    object->mdlId[8] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M434, 0x01));
    Hu3DModelLayerSet(object->mdlId[8], 2);
    Hu3DModelCameraSet(object->mdlId[8], 4);
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M434, 0x08));
    Hu3DModelClusterAttrSet(object->mdlId[3], 0, 0xC0000001);
    Hu3DModelLayerSet(object->mdlId[3], 2);
    Hu3DModelCameraSet(object->mdlId[3], 8);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M434, 0x09));
    Hu3DModelClusterAttrSet(object->mdlId[4], 0, 0xC0000001);
    Hu3DModelLayerSet(object->mdlId[4], 2);
    Hu3DModelCameraSet(object->mdlId[4], 8);
    object->mdlId[2] = Hu3DHookFuncCreate(DrawOceanSurface);
    Hu3DModelLayerSet(object->mdlId[2], 3);
    Hu3DModelCameraSet(object->mdlId[2], 8);
    gridPosition = oceanGridVertices;
    for (gridRow = 0; gridRow < 32; gridRow++) {
        for (gridColumn = 0; gridColumn < 32; gridPosition++, gridColumn++) {
            gridPosition->x = -700.0f + ((1400.0f * gridColumn) / 31.0f);
            gridPosition->y = -15.000001f;
            gridPosition->z = -600.0f + ((1200.0f * gridRow) / 31.0f);
            if (gridRow & 0x1) {
                gridPosition->x += 700.0f / 31.0f;
            }
        }
    }
    DCFlushRangeNoSync(oceanGridVertices, sizeof(oceanGridVertices));
    waveParameters = oceanWaveParameters;
    for (gridRow = 0; gridRow < 1024; gridRow++, waveParameters++) {
        waveParameters->x = 360.0f * ((1.0f / 256.0f) * frand8());
        waveParameters->y = 2.0f * ((1.0f / 256.0f) * frand8()) + 0.5f;
    }
    memset(waterRippleEffects, 0, sizeof(waterRippleEffects));
    waterRippleEffectCount = 0;
    oceanSurfaceDisplayList.displayListSize = 16384;
    oceanSurfaceDisplayList.displayListBuffer = HuMemDirectMallocNum(HEAP_HEAP, oceanSurfaceDisplayList.displayListSize, HU_MEMNUM_OVL);
    DCInvalidateRange(oceanSurfaceDisplayList.displayListBuffer, oceanSurfaceDisplayList.displayListSize);
    CreateOceanSurfaceDisplayList();
    waterRippleDisplayList.displayListSize = 16384;
    waterRippleDisplayList.displayListBuffer = HuMemDirectMallocNum(HEAP_HEAP, waterRippleDisplayList.displayListSize, HU_MEMNUM_OVL);
    CreateWaveDisplayList();
    oceanTextureAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M430, 0x19));
    environmentTextureAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M434, 0x0A));
    HuDataDirClose(DATADIR_M430);
    for (gridRow = 0; gridRow < 4; gridRow++) {
        environmentTextures[gridRow].sourceWidth = environmentTextureConfigs[gridRow].sourceWidth;
        environmentTextures[gridRow].sourceHeight = environmentTextureConfigs[gridRow].sourceHeight;
        environmentTextures[gridRow].textureWidth = environmentTextureConfigs[gridRow].textureWidth;
        environmentTextures[gridRow].textureHeight = environmentTextureConfigs[gridRow].textureHeight;
        environmentTextures[gridRow].copyMode = environmentTextureConfigs[gridRow].copyMode;
        environmentTextures[gridRow].textureBufferSize = GXGetTexBufferSize(environmentTextures[gridRow].textureWidth, environmentTextures[gridRow].textureHeight, GX_TF_RGB565, GX_FALSE, 0);
        environmentTextures[gridRow].textureBuffer = HuMemDirectMallocNum(HEAP_HEAP, environmentTextures[gridRow].textureBufferSize, HU_MEMNUM_OVL);
        memset(environmentTextures[gridRow].textureBuffer, 0, environmentTextures[gridRow].textureBufferSize);
    }
    object->mdlId[5] = Hu3DHookFuncCreate(DrawEnvironmentTextureLayer);
    Hu3DModelLayerSet(object->mdlId[5], 3);
    Hu3DModelCameraSet(object->mdlId[5], 2);
    object->mdlId[6] = Hu3DHookFuncCreate(DrawEnvironmentTextureOverlay);
    Hu3DModelLayerSet(object->mdlId[6], 3);
    Hu3DModelCameraSet(object->mdlId[6], 4);
    object->mdlId[7] = Hu3DHookFuncCreate(DrawEnvironmentTextureDetail);
    Hu3DModelLayerSet(object->mdlId[7], 5);
    Hu3DModelCameraSet(object->mdlId[7], 4);
    particleAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M434, 0x0B));
    object->mdlId[10] = Hu3DParticleCreate(particleAnimation, 100);
    Hu3DParticleHookSet(object->mdlId[10], UpdateEnvironmentParticle);
    Hu3DModelAttrSet(object->mdlId[10], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(object->mdlId[10], 4);
    Hu3DModelCameraSet(object->mdlId[10], 8);
    object->objFunc = UpdateEnvironmentObject;
}

HSFATTRIBUTE *GetModelAttributeByName(s16 model, char *name)
{
    HSFDATA *modelData;
    HSFATTRIBUTE *attribute;
    s32 attributeIndex;
    HU3DMODEL *model;
    HSFBITMAP *bitmap;
    model = &Hu3DData[model];
    modelData = model->hsf;
    attribute = modelData->attribute;
    for (attributeIndex = 0; attributeIndex < modelData->attributeNum; attributeIndex++, attribute++) {
        bitmap = attribute->bitmap;
        if (strcmp(name, bitmap->name) == 0) {
            break;
        }
    }
    if (attributeIndex >= modelData->attributeNum) {
        return NULL;
    }
    return attribute;
}

void UpdateEnvironmentObject(OMOBJ *object)
{
    s32 effectIndex;
    WaterRippleEffect *effect;
    HuVec2f *waveParameters;
    HSFATTRIBUTE *skyAttribute;
    Vec *gridPosition;
    s32 gridRow;
    s32 activeEffectCount;
    WaterRippleEffect *activeEffects[64];

    skyAttribute = GetModelAttributeByName(object->mdlId[1], "sora");
    if (skyAttribute != NULL) {
        skyAttribute->trans.x += 0.0001f;
        skyAttribute->trans.y -= 0.00005f;
    }
    effect = waterRippleEffects;
    activeEffectCount = 0;
    for (effectIndex = 0; effectIndex < 64; effectIndex++, effect++) {
        if (effect->remainingFrames) {
            activeEffects[activeEffectCount++] = effect;
        }
    }
    gridPosition = oceanGridVertices;
    waveParameters = oceanWaveParameters;
    for (effectIndex = 0; effectIndex < 32; effectIndex++) {
        for (gridRow = 0; gridRow < 32; gridRow++, gridPosition++, waveParameters++) {
            gridPosition->y = -15.000001f + (2.0f * sind(waveParameters->x));
            waveParameters->x += waveParameters->y;
        }
    }
    DCFlushRangeNoSync(oceanGridVertices, sizeof(oceanGridVertices));
    for (effectIndex = 0; effectIndex < activeEffectCount; effectIndex++) {
        effect = activeEffects[effectIndex];
        effect->remainingFrames--;
    }
    CenterM[1].y = -15.000001f + (-CenterM[2].y);
    CRotM[1].x = -CRotM[2].x;
}

void FreeEnvironmentTextures(void)
{
    s32 textureIndex;
    for (textureIndex = 0; textureIndex < 4; textureIndex++) {
        if (environmentTextures[textureIndex].textureBuffer != NULL) {
            HuMemDirectFree(environmentTextures[textureIndex].textureBuffer);
            environmentTextures[textureIndex].textureBuffer = NULL;
        }
    }
}

GXColor oceanSurfaceTint = { 255, 255, 255, 64 };

float oceanWaveIndirectionMatrix[2][3] = { 0.012f, 0, 0, 0, 0.012f, 0 };

float oceanDetailIndirectionMatrix[2][3] = { 0, 0, 0.5f, 0, 0.5f, 0 };

void DrawOceanSurface(HU3DMODEL *model, Mtx mtx)
{
    float textureScale;

    Mtx projectionMatrix;
    Mtx textureMatrix;
    Mtx scaleMatrix;
    Mtx rotationMatrix;
    Mtx textureTranslationMatrix;
    Mtx viewModelMatrix;
    Mtx projectedViewModelMatrix;
    Mtx secondViewModelMatrix;
    Mtx secondProjectedViewModelMatrix;
    GXTexObj textureObject;
    GXLoadPosMtxImm(mtx, GX_PNMTX0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetNumTexGens(4);
    GXInitTexObj(&textureObject, environmentTextures[2].textureBuffer, environmentTextures[2].textureWidth, environmentTextures[2].textureHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&textureObject, GX_TEXMAP0);
    GXInitTexObj(&textureObject, environmentTextures[1].textureBuffer, environmentTextures[1].textureWidth, environmentTextures[1].textureHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&textureObject, GX_TEXMAP1);
    MTXLightPerspective(projectionMatrix, 30.0f, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, viewModelMatrix);
    MTXConcat(viewModelMatrix, mtx, viewModelMatrix);
    MTXConcat(projectionMatrix, Hu3DCameraMtx, projectedViewModelMatrix);
    MTXConcat(projectedViewModelMatrix, viewModelMatrix, viewModelMatrix);
    GXLoadTexMtxImm(viewModelMatrix, GX_TEXMTX0, GX_MTX3x4);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX0);
    MTXLightPerspective(projectionMatrix, 30.0f, 1.2f, 0.5f, 0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, secondViewModelMatrix);
    MTXConcat(secondViewModelMatrix, mtx, secondViewModelMatrix);
    MTXConcat(projectionMatrix, Hu3DCameraMtx, secondProjectedViewModelMatrix);
    MTXConcat(secondProjectedViewModelMatrix, secondViewModelMatrix, secondViewModelMatrix);
    GXLoadTexMtxImm(secondViewModelMatrix, GX_TEXMTX1, GX_MTX3x4);
    GXSetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX1);
    GXSetNumTevStages(2);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_CPREV, GX_CC_TEXC, GX_CC_A0, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevColor(GX_TEVSTAGE1, oceanSurfaceTint);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    HuSprTexLoad(oceanTextureAnimation, 0, GX_TEXMAP2, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXInitTexObj(&textureObject, environmentTextures[0].textureBuffer, environmentTextures[0].textureWidth, environmentTextures[0].textureHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&textureObject, GX_TEXMAP3);
    GXSetNumIndStages(2);
    MTXRotDeg(rotationMatrix, 'X', -90.0f);
    textureScale = 1.0f / 600.0f;
    MTXScale(scaleMatrix, textureScale, textureScale, textureScale);
    MTXConcat(scaleMatrix, rotationMatrix, textureMatrix);
    textureMatrix[0][1] = -0.03f;
    GXLoadTexMtxImm(textureMatrix, GX_TEXMTX2, GX_MTX2x4);
    GXSetTexCoordGen(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_POS, GX_TEXMTX2);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD2, GX_TEXMAP2);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
    GXSetIndTexMtx(GX_ITM_0, oceanWaveIndirectionMatrix, 0);
    GXSetTevIndWarp(GX_TEVSTAGE0, GX_INDTEXSTAGE0, GX_TRUE, GX_FALSE, GX_ITM_0);
    MTXRotDeg(rotationMatrix, 'X', -90.0f);
    textureScale = 1.0f / 1200.0f;
    MTXScale(scaleMatrix, textureScale, textureScale, textureScale);
    MTXConcat(scaleMatrix, rotationMatrix, textureMatrix);
    MTXTrans(textureTranslationMatrix, 0.5f, 0.5f, 0.0f);
    MTXConcat(textureTranslationMatrix, textureMatrix, textureMatrix);
    GXLoadTexMtxImm(textureMatrix, GX_TEXMTX3, GX_MTX2x4);
    GXSetTexCoordGen(GX_TEXCOORD3, GX_TG_MTX2x4, GX_TG_POS, GX_TEXMTX3);
    GXSetIndTexOrder(GX_INDTEXSTAGE1, GX_TEXCOORD3, GX_TEXMAP3);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE1, GX_ITS_1, GX_ITS_1);
    GXSetIndTexMtx(GX_ITM_1, oceanDetailIndirectionMatrix, 0);
    GXSetTevIndWarp(GX_TEVSTAGE1, GX_INDTEXSTAGE1, GX_TRUE, GX_FALSE, GX_ITM_1);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, oceanGridVertices, sizeof(Vec));
    GXSetCullMode(GX_CULL_BACK);
    GXCallDisplayList(oceanSurfaceDisplayList.displayListBuffer, oceanSurfaceDisplayList.displayListSize);
    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevDirect(GX_TEVSTAGE1);
}

void CreateOceanSurfaceDisplayList(void)
{
    s32 vertexIndex;
    s32 gridRow;
    s32 firstVertexIndex;
    GXBeginDisplayList(oceanSurfaceDisplayList.displayListBuffer, oceanSurfaceDisplayList.displayListSize);
    for (gridRow = 0; gridRow < 31; gridRow++) {
        firstVertexIndex = gridRow * 32;
        GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 64);
        if (gridRow & 0x1) {
            for (vertexIndex = 0; vertexIndex < 32; vertexIndex++) {
                GXPosition1x16(firstVertexIndex + vertexIndex + 32);
                GXPosition1x16(firstVertexIndex + vertexIndex);
            }
        }
        else {
            for (vertexIndex = 31; vertexIndex >= 0; vertexIndex--) {
                GXPosition1x16(firstVertexIndex + vertexIndex);
                GXPosition1x16(firstVertexIndex + vertexIndex + 32);
            }
        }
    }
    oceanSurfaceDisplayList.displayListSize = GXEndDisplayList();
    DCFlushRangeNoSync(oceanSurfaceDisplayList.displayListBuffer, oceanSurfaceDisplayList.displayListSize);
}

void CreateWaveDisplayList(void)
{
    GXBeginDisplayList(waterRippleDisplayList.displayListBuffer, waterRippleDisplayList.displayListSize);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition2f32(-0.5f, -0.5f);
    GXTexCoord2f32(0, 0);
    GXPosition2f32(0.5f, -0.5f);
    GXTexCoord2f32(1, 0);
    GXPosition2f32(0.5f, 0.5f);
    GXTexCoord2f32(1, 1);
    GXPosition2f32(-0.5f, 0.5f);
    GXTexCoord2f32(0, 1);
    GXEnd();
    waterRippleDisplayList.displayListSize = GXEndDisplayList();
    DCFlushRangeNoSync(waterRippleDisplayList.displayListBuffer, waterRippleDisplayList.displayListSize);
}

void DrawEnvironmentTexture(EnvironmentTexture *texture, GXBool copyToTexture)
{
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_TRUE);
    GXSetTexCopySrc(0, 0, texture->sourceWidth, texture->sourceHeight);
    GXSetTexCopyDst(texture->textureWidth, texture->textureHeight, GX_TF_RGB565, texture->copyMode);
    GXSetCopyClear(BGColor, 0xFFFFFF);
    GXCopyTex(texture->textureBuffer, copyToTexture);
    GXPixModeSync();
    DCFlushRange(texture->textureBuffer, texture->textureBufferSize);
}

void DrawEnvironmentTextureLayer(HU3DMODEL *model, Mtx mtx)
{
    DrawEnvironmentTexture(&environmentTextures[1], GX_TRUE);
}

void DrawEnvironmentTextureOverlay(HU3DMODEL *model, Mtx mtx)
{
    DrawEnvironmentTexture(&environmentTextures[2], GX_TRUE);
}

void DrawEnvironmentTextureDetail(HU3DMODEL *model, Mtx mtx)
{
    Mtx projectionMatrix;
    Mtx viewModelMatrix;
    Mtx projectedViewModelMatrix;
    GXTexObj textureObject;

    DrawEnvironmentTexture(&environmentTextures[3], GX_FALSE);
    GXLoadPosMtxImm(mtx, GX_PNMTX0);
    MTXLightPerspective(projectionMatrix, 30.0f, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, viewModelMatrix);
    MTXConcat(viewModelMatrix, mtx, viewModelMatrix);
    MTXConcat(projectionMatrix, Hu3DCameraMtx, projectedViewModelMatrix);
    MTXConcat(projectedViewModelMatrix, viewModelMatrix, viewModelMatrix);
    GXLoadTexMtxImm(viewModelMatrix, GX_TEXMTX0, GX_MTX3x4);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetZMode(GX_TRUE, GX_GREATER, GX_FALSE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXInitTexObj(&textureObject, environmentTextures[2].textureBuffer, environmentTextures[2].textureWidth, environmentTextures[2].textureHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&textureObject, GX_TEXMAP0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-1000.0f, -5.000001f, -1000.0f);
    GXPosition3f32(1000.0f, -5.000001f, -1000.0f);
    GXPosition3f32(1000.0f, -5.000001f, 1000.0f);
    GXPosition3f32(-1000.0f, -5.000001f, 1000.0f);
    GXEnd();
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-1000.0f, -70.0f, -1000.0f);
    GXPosition3f32(1000.0f, -70.0f, -1000.0f);
    GXPosition3f32(1000.0f, -70.0f, 1000.0f);
    GXPosition3f32(-1000.0f, -70.0f, 1000.0f);
    GXEnd();
    DrawEnvironmentTexture(&environmentTextures[2], GX_FALSE);
    GXInitTexObj(&textureObject, environmentTextures[3].textureBuffer, environmentTextures[3].textureWidth, environmentTextures[3].textureHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&textureObject, GX_TEXMAP0);
    GXSetZMode(GX_TRUE, GX_GREATER, GX_FALSE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-1000.0f, -5.000001f, -1000.0f);
    GXPosition3f32(1000.0f, -5.000001f, -1000.0f);
    GXPosition3f32(1000.0f, -5.000001f, 1000.0f);
    GXPosition3f32(-1000.0f, -5.000001f, 1000.0f);
    GXEnd();
}

GXColor splashParticleColor = { 255, 0, 0, 255 };
GXColor splashParticleAlpha = { 255, 255, 255, 32 };
GXColor waterRippleColor = { 0, 126, 126, 255 };

void DrawEnvironmentSurface(HU3DMODEL *model, Mtx mtx)
{

    float effectScale;
#ifdef NON_MATCHING
    Mtx44 projectionMatrix;
#else
    Mtx projectionMatrix;
#endif
    Mtx modelMatrix;
    WaterRippleEffect *effect;
    s32 effectIndex;
    s32 effectSlot;

    MTXOrtho(projectionMatrix, 0, 480, 0, 640, 0, 100);
    GXSetProjection(projectionMatrix, GX_ORTHOGRAPHIC);
    MTXIdentity(modelMatrix);
    GXLoadPosMtxImm(modelMatrix, GX_PNMTX0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanMatColor(GX_COLOR0A0, waterRippleColor);
    GXSetNumTexGens(0);
    HuSprTexLoad(environmentTextureAnimation, 0, GX_TEXMAP0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);

    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetScissor(0, 0, environmentTextures[0].textureWidth, environmentTextures[0].textureHeight);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition2u16(0, 0);
    GXPosition2u16(environmentTextures[0].textureWidth, 0);
    GXPosition2u16(environmentTextures[0].textureWidth, environmentTextures[0].textureHeight);
    GXPosition2u16(0, environmentTextures[0].textureHeight);
    GXEnd();
    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetNumTevStages(1);

    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXClearVtxDesc();

    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    effectSlot = waterRippleEffectCount;
    for (effectIndex = 0; effectIndex < 64; effectSlot = (effectSlot + 1) % 64, effectIndex++) {
        effect = &waterRippleEffects[effectSlot];
        if (effect->remainingFrames == 0) {
            continue;
        }
        effectScale = (0.2f + cosd((90.0f * effect->remainingFrames) / REFRESH_RATE)) * 128.0f;
        splashParticleAlpha.a = 255.0f * sind((90.0f * effect->remainingFrames) / REFRESH_RATE);
        GXSetChanMatColor(GX_COLOR0A0, splashParticleAlpha);
        MTXScale(modelMatrix, 0.5f * effectScale, 0.5f * effectScale, 1.0f);
        mtxTransCat(modelMatrix, (200.0f * (600.0f + effect->position.x)) / 1200.0f, (200.0f * (600.0f + effect->position.z)) / 1200.0f, 0);
        GXLoadPosMtxImm(modelMatrix, GX_PNMTX0);
        GXCallDisplayList(waterRippleDisplayList.displayListBuffer, waterRippleDisplayList.displayListSize);
    }
    DrawEnvironmentTexture(&environmentTextures[0], GX_TRUE);
}

void CreateWaterImpactRipple(Vec *position)
{
    WaterRippleEffect *effectSlot;
    effectSlot = &waterRippleEffects[waterRippleEffectCount];
    if (effectSlot->remainingFrames == 0) {
        effectSlot->position.x = position->x;
        effectSlot->position.y = position->y;
        effectSlot->position.z = position->z;
        effectSlot->remainingFrames = REFRESH_RATE;
        waterRippleEffectCount = (waterRippleEffectCount + 1) % 64;
        return;
    }
}

void UpdateEnvironmentParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float emissionAngle;
    float particleSpeed;
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    s32 expiredParticleCount;
    if (particle->dataCnt == 0) {
        particle->dataCnt = 1;
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleData++, particleIndex++) {
            particleData->scale = 10;
            emissionAngle = ((1.0f / 256.0f) * frand8()) * 360.0f;
            particleData->pos.x = (0.2f * sind(emissionAngle)) * 100.0f;
            particleData->pos.y = 0;
            particleData->pos.z = (0.2f * cosd(emissionAngle)) * 100.0f;
            particleSpeed = (100.0f * (0.025f * ((1.0f / 256.0f) * frand8()))) + 2.5f;
            particleData->vel.x = particleSpeed * sind(emissionAngle);
            particleData->vel.y = (100.0f * (0.025f * (((1.0f / 256.0f) * (s32)(frand() & 0x1FF)) - 1.0f))) + 7.5000005f;
            particleData->vel.z = particleSpeed * cosd(emissionAngle);
            particleData->color.r = particleData->color.g = particleData->color.b = 255;
            particleData->color.a = (frand() & 0x1F) + 16;
        }
    }
    expiredParticleCount = 0;
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->color.a == 0) {
            expiredParticleCount++;
            continue;
        }
        particleData->pos.x += particleData->vel.x;
        particleData->pos.y += particleData->vel.y;
        particleData->pos.z += particleData->vel.z;
        particleData->vel.y += -0.5f;
        particleData->color.a--;
    }
    if (expiredParticleCount == particle->maxCnt) {
        model->attr |= 0x1;
    }
    DCStoreRangeNoSync(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void CreateWaterSplash(Vec *position)
{
    Hu3DModelPosSet(environmentObject->mdlId[10], position->x, position->y, position->z);
    Hu3DModelAttrReset(environmentObject->mdlId[10], HU3D_ATTR_DISPOFF);
}
