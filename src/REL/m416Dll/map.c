#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/sprite.h"

#include "ext_math.h"
#include <string.h>

#include "REL/m416Dll.h"

GXColor mapFadeColor = { 0, 0, 0, 255 };
Vec mapObjectPositions[8] = {
    { 70, 0, 70 },
    { 70, 0, -70 },
    { -70, 0, -70 },
    { -70, 0, 70 },
    { 70, 0, 70 },
    { 70, 0, -70 },
    { -70, 0, -70 },
    { -70, 0, 70 },
};
Vec mapLightPositions[2] = { { 370, 615, -245 }, { -370, 615, -245 } };
float mapRotationDelta[2] = { 0, 1 };

OMOBJ *mapObject;
OMOBJ *mapObjectsObject;
u32 mapTextureBufferSize;
void *mapTextureBuffer;
float mapRotation;
s16 mapLightIds[4];
float mapRotationAngles[2];
s16 trackedModelCount;
s16 trackedModelIds[256][4];

void InitializeMapModel(OMOBJ *object);
void UpdateMapModel(OMOBJ *object);
void InitializeMapObjects(OMOBJ *object);
void UpdateMapObjects(OMOBJ *object);

void DrawMapTexture(HU3DMODEL *model, Mtx mtx);

void InitializeModelTracking(void);
void EmptyMapCallback(void);

void InitializeMap(HUPROCESS *objman)
{
    mapObject = omAddObjEx(objman, 20, 2, 0, -1, InitializeMapModel);
    mapObjectsObject = omAddObjEx(objman, 20, 10, 0, -1, InitializeMapObjects);
    InitializeModelTracking();
}

void ShutdownMap(void)
{
    EmptyMapCallback();
}

void InitializeMapModel(OMOBJ *object)
{
    s16 i;
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M416, 0x00));
    Hu3DModelShadowMapSet(object->mdlId[0]);
    Hu3DModelLayerSet(object->mdlId[0], 0);
    object->trans.x = 0;
    object->trans.y = 0;
    object->trans.z = 0;
    TrackModelMaterials(object->mdlId[0]);
    for (i = 0; i < 2; i++) {
        mapLightIds[i + 2] = Hu3DLLightCreate(object->mdlId[0], 0, 0, 0, 0, 0, 0, 0, 0, 0);
        Hu3DLLightStaticSet(object->mdlId[0], mapLightIds[i + 2], 1);
        Hu3DLLightPointSet(object->mdlId[0], mapLightIds[i + 2], 45, 0.8, 3);
        Hu3DLLightColorSet(object->mdlId[0], mapLightIds[i + 2], 255, 160, 0, 255);
        Hu3DLLightPosSet(object->mdlId[0], mapLightIds[i + 2], mapLightPositions[i].x, 475, mapLightPositions[i].z, 0, 1, 0);
    }
    object->mdlId[1] = Hu3DHookFuncCreate(DrawMapTexture);
    Hu3DModelLayerSet(object->mdlId[1], 1);
    mapTextureBufferSize = GXGetTexBufferSize(640, 480, GX_TF_RGBA8, GX_FALSE, 0);
    mapTextureBuffer = HuMemDirectMallocNum(HEAP_MODEL, mapTextureBufferSize, HU_MEMNUM_OVL);
    memset(mapTextureBuffer, 0, mapTextureBufferSize);
    object->objFunc = UpdateMapModel;
}

void UpdateMapModel(OMOBJ *object)
{
    s32 i;
    for (i = 0; i < 2; i++) {
        Hu3DLLightPointSet(object->mdlId[0], mapLightIds[i + 2], (15 * sind(object->work[i])) + 45, 0.8, 3);
        object->work[i] += 8.0f;
        if (object->work[i] > 360.0f) {
            object->work[i] -= 360.0f;
        }
    }
}

void InitializeMapObjects(OMOBJ *object)
{
    Mtx transformMatrix68;
    Mtx transformMatrix38;
    Mtx transformMatrix8;
    s32 index30;
    Vec *position29;
    s32 index28;
    s32 index27;
    s16 index26;
    s16 index25;
    s16 index24;
    ANIMDATA *animationData23;
    object->stat |= 0x100;
    for (index30 = 0; index30 < 2; index30++) {
        if (index30 == 0) {
            index25 = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M416, 0x01));
            index26 = index25;
        }
        else {
            index24 = Hu3DModelLink(index25);
            index26 = index24;
        }
        object->mdlId[index30] = index26;
        Hu3DModelLayerSet(index26, 2);
        TrackModelMaterials(object->mdlId[index30]);
        Hu3DModelPosSet(object->mdlId[index30], mapLightPositions[index30].x, mapLightPositions[index30].y, mapLightPositions[index30].z);
    }
    animationData23 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M416, 0x06));
    position29 = &mapObjectPositions[0];
    for (index30 = 0; index30 < 2; index30++) {
        for (index27 = 0; index27 < 4; index27++, position29++) {
            index28 = (index30 * 4) + 2;
            index28 = index27 + index28;
            object->mdlId[index28] = Hu3DParticleCreate(animationData23, 50);
            Hu3DParticleHookSet(object->mdlId[index28], updatePlayerParticle);
            Hu3DParticleBlendModeSet(object->mdlId[index28], 1);
            Hu3DModelLayerSet(object->mdlId[index28], 6);
            Hu3DModelScaleSet(object->mdlId[index28], 0.5f, 0.7f, 0.5f);
            MTXTrans(transformMatrix68, mapLightPositions[index30].x, mapLightPositions[index30].y, mapLightPositions[index30].z);
            MTXTrans(transformMatrix38, 0, -150, 0);
            MTXConcat(transformMatrix68, transformMatrix38, transformMatrix68);
            MTXTrans(transformMatrix8, position29->x, position29->y, position29->z);
            MTXConcat(transformMatrix68, transformMatrix8, transformMatrix68);
            Hu3DModelPosSet(object->mdlId[index28], transformMatrix68[0][3], transformMatrix68[1][3], transformMatrix68[2][3]);
        }
    }
    for (index30 = 0; index30 < 2; index30++) {
        mapLightIds[index30] = Hu3DLLightCreate(object->mdlId[index30], 0, 0, 0, 0, 0, 0, 0, 0, 0);
        Hu3DLLightStaticSet(object->mdlId[index30], mapLightIds[index30], 1);
        Hu3DLLightPointSet(object->mdlId[index30], mapLightIds[index30], 1000, 0.8, 3);
        Hu3DLLightColorSet(object->mdlId[index30], mapLightIds[index30], 255, 160, 0, 255);
        Hu3DLLightPosSet(object->mdlId[index30], mapLightIds[index30], mapLightPositions[index30].x, 475, mapLightPositions[index30].z, 0, 1, 0);
    }
    object->work[0] = 0;
    object->work[1] = 45;
    mapRotation = 0;
    object->objFunc = UpdateMapObjects;
}

void UpdateMapObjects(OMOBJ *object)
{
    float scalarValue31;

    s32 index31;
    Vec *position29;
    s32 index28;
    s32 index27;
    HU3DMODEL *model26;

    Mtx transformMatrix130[2];
    Mtx transformMatrix100;
    Mtx transformMatrixD0;
    Mtx transformMatrixA0;
    Mtx transformMatrix70;
    Mtx transformMatrix40;
    Mtx transformMatrix10;
    float scalarValues8[2];
    MTXIdentity(transformMatrix100);
    MTXIdentity(transformMatrix130[0]);
    MTXIdentity(transformMatrix130[1]);
    MTXIdentity(transformMatrixD0);
    MTXIdentity(transformMatrixA0);
    MTXIdentity(transformMatrix70);
    MTXIdentity(transformMatrix40);
    MTXIdentity(transformMatrix10);
    mapRotation = fmod(60.0f / REFRESH_RATE + mapRotation, 360);
    scalarValues8[0] = 20 * sind(mapRotation);
    scalarValues8[1] = 20 * cosd(mapRotation);
    if (getGameState() == 6) {
        for (index31 = 0; index31 < 2; index31++) {
            scalarValue31 = scalarValues8[index31] - mapRotationAngles[index31];
            if (scalarValue31 * mapRotationDelta[index31] <= 0.0f) {
                model26 = &Hu3DData[object->mdlId[index31]];
                HuAudFXEmiterPlay(1467, &model26->pos);
            }
            mapRotationDelta[index31] = scalarValue31;
        }
    }
    mapRotationAngles[0] = scalarValues8[0];
    mapRotationAngles[1] = scalarValues8[1];
    position29 = &mapObjectPositions[0];
    for (index31 = 0; index31 < 2; index31++) {
        MTXRotDeg(transformMatrix130[index31], 'y', mapRotationAngles[index31]);
        for (index28 = 0; index28 < 4; index28++, position29++) {
            index27 = (index31 * 4) + 2;
            index27 = index28 + index27;
            MTXTrans(transformMatrixA0, mapLightPositions[index31].x, mapLightPositions[index31].y, mapLightPositions[index31].z);
            MTXTrans(transformMatrix70, 0, -150, 0);
            MTXConcat(transformMatrixA0, transformMatrix70, transformMatrixA0);
            MTXTrans(transformMatrix40, position29->x, position29->y, position29->z);
            MTXConcat(transformMatrix130[index31], transformMatrix40, transformMatrix10);
            MTXConcat(transformMatrixA0, transformMatrix10, transformMatrixA0);
            Hu3DModelPosSet(object->mdlId[index27], transformMatrixA0[0][3], transformMatrixA0[1][3], transformMatrixA0[2][3]);
        }
    }
    Hu3DModelRotSet(object->mdlId[0], 0, mapRotationAngles[0], 0);
    Hu3DModelRotSet(object->mdlId[1], 0, mapRotationAngles[1], 0);
    for (index31 = 0; index31 < 2; index31++) {
        Hu3DLLightPointSet(object->mdlId[index31], mapLightIds[index31], (15 * sind(object->work[index31])) + 45, 0.9, 3);
        object->work[index31] += 8.0f;
        if (object->work[index31] > 360.0f) {
            object->work[index31] -= 360.0f;
        }
    }
}

void CopyMapTexture(void *textureBuffer, u8 clearAfterCopy)
{
    GXSetTexCopySrc(0, 0, 640, 480);
    GXSetTexCopyDst(640, 480, GX_TF_RGBA8, GX_FALSE);
    GXSetCopyClear(BGColor, 0xFFFFFF);
    GXCopyTex(textureBuffer, clearAfterCopy);
    DCFlushRange(textureBuffer, mapTextureBufferSize);
}

void DrawMapTexture(HU3DMODEL *model, Mtx mtx)
{
    Mtx44 transformMatrix34;
    GXTexObj textureObject14;
    MTXOrtho(transformMatrix34, 0, 480, 0, 576, 0, 1000);
    GXSetProjection(transformMatrix34, GX_ORTHOGRAPHIC);
    MTXIdentity(mtx);
    GXLoadPosMtxImm(mtx, GX_PNMTX0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanAmbColor(GX_COLOR0A0, mapFadeColor);
    GXSetChanMatColor(GX_COLOR0A0, mapFadeColor);
    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP1, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXInitTexObj(&textureObject14, mapTextureBuffer, 640, 480, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&textureObject14, GX_TEXMAP0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_INVSRCALPHA, GX_BL_SRCALPHA, GX_LO_NOOP);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(0, 0, -10);
    GXTexCoord2f32(0, 0);
    GXPosition3f32(576, 0, -10);
    GXTexCoord2f32(1, 0);
    GXPosition3f32(576, 480, -10);
    GXTexCoord2f32(1, 1);
    GXPosition3f32(0, 480, -10);
    GXTexCoord2f32(0, 1);
    GXEnd();
    CopyMapTexture(mapTextureBuffer, GX_FALSE);
}

void InitializeModelTracking(void)
{
    trackedModelCount = 0;
}

void TrackModelMaterials(s16 model)
{
    HSFMATERIAL *material31;
    HSFDATA *modelData30;
    s32 index29;
    HU3DMODEL *model28;
    s16 *modelIds27;
    s16 index26;
    model28 = &Hu3DData[model];
    modelData30 = model28->hsf;
    index26 = modelData30->materialNum;
    modelIds27 = &trackedModelIds[trackedModelCount][0];
    *modelIds27 = model;
    material31 = modelData30->material;
    for (index29 = 0; index29 < index26; material31++, index29++) {
        material31->vtxMode = 1;
    }
    trackedModelCount++;
}

void SetTrackedModelAmbient(float ambientLevel)
{
    s16 *modelIds31;
    s32 index30;
    modelIds31 = &trackedModelIds[0][0];
    for (index30 = 0; index30 < trackedModelCount; modelIds31 += 4, index30++) {
        HU3DMODEL *model = &Hu3DData[*modelIds31];
        Hu3DModelAmbSet(*modelIds31, ambientLevel, ambientLevel, ambientLevel);
    }
}

void EmptyMapCallback(void) { }

void SetMapFadeAlpha(float alpha)
{
    mapFadeColor.a = alpha * 255;
}
