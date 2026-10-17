#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/sprite.h"

#include "REL/m456Dll.h"

#include "ext_math.h"
#include "version.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/frand.h"
#endif

HUPROCESS *stageObjectManager;
OMOBJ *mainStageObject;
OMOBJ *fishObject;
OMOBJ *waterEffectObject;
OMOBJ *sprinkleObject;
OMOBJ *whaleObject;
OMOBJ *fishSchoolObject;
Vec stageCameraPosition;

typedef struct StageParticleWork {
    ANIMDATA *spriteAnimations[4];
    s32 particleModelIds[4];
    Vec fadeParameters;
} StageParticleWork;

StageParticleWork *stageParticleWork;

Vec fishStartPositions[4] = {
    { -460, 0, 150 },
    { 460, 0, 150 },
    { -460, 0, -150 },
    { 460, 0, -150 },
};

GXColor fishParticleColor = { 255, 255, 255, 255 };

typedef struct FramebufferConfig {
    s32 sourceWidth;
    s32 sourceHeight;
    s32 textureWidth;
    s32 textureHeight;
    BOOL useScreenCapture;
} FramebufferConfig;

FramebufferConfig framebufferConfigs[4] = {
    { 400, 400, 400, 400, FALSE },
    { 640, 480, 320, 240, TRUE },
    { 640, 480, 640, 480, FALSE },
    { 640, 480, 320, 240, TRUE },
};

void InitializeStageParticles(void);

void InitializeFishObject(OMOBJ *object);
void InitializeWaterEffect(OMOBJ *object);
void InitializeSprinkleObject(OMOBJ *object);
void InitializeWhaleObject(OMOBJ *object);
void InitializeFishSchool(OMOBJ *object);

OMOBJ *CreateStageObject(void);

void InitializeStageObjects(HUPROCESS *objman)
{
    stageObjectManager = objman;
    InitializeStageParticles();
    fishObject = omAddObjEx(stageObjectManager, 50, 0, 0, -1, InitializeFishObject);
    waterEffectObject = omAddObjEx(stageObjectManager, 50, 8, 0, -1, InitializeWaterEffect);
    sprinkleObject = omAddObjEx(stageObjectManager, 50, 1, 0, -1, InitializeSprinkleObject);
    whaleObject = omAddObjEx(stageObjectManager, 50, 0, 0, -1, InitializeWhaleObject);
    fishSchoolObject = omAddObjEx(stageObjectManager, 50, 0, 0, -1, InitializeFishSchool);
    mainStageObject = CreateStageObject();
}

void DestroyStageObject(OMOBJ *object);

void DestroyFishObject(OMOBJ *object);
void DestroyWaterEffect(OMOBJ *object);
void DestroySprinkleObject(OMOBJ *object);
void DestroyWhaleObject(OMOBJ *object);

void DestroyStageParticles(void);

void DestroyStageObjects(void)
{
    DestroyStageObject(mainStageObject);
    DestroyStageParticles();
    DestroyFishObject(fishObject);
    DestroyWaterEffect(waterEffectObject);
    DestroySprinkleObject(sprinkleObject);
    DestroyWhaleObject(whaleObject);
}

typedef struct FramebufferWork {
    void *textureBuffer;
    u32 textureBufferSize;
    BOOL useScreenCapture;
    s32 sourceWidth;
    s32 sourceHeight;
    s32 textureWidth;
    s32 textureHeight;
} FramebufferWork;

typedef struct SprinkleEffectWork {
    Vec position;
    Vec effectParameters;
    float frameDuration;
    u16 lifespan;
    u16 remainingFrames;
} SprinkleEffectWork;

typedef struct M456StageWork {
    s32 effectState;
    FramebufferWork framebuffers[4];
    SprinkleEffectWork sprinkles[64];
    float waterTextureCoordinates[26][2];
    float waterTextureDrift[26][2];
    void *displayList;
    u32 displayListSize;
    ANIMDATA *spriteAnimations[9];
    float verticalBobOffset;
    float stageHeight;
    u32 animationFrame;
    u32 rippleFrame;
    u32 rippleAngle;
    float waterTextureOffsetX;
    float waterTextureOffsetY;
} M456StageWork;

void UpdateStageObject(OMOBJ *object);

void DrawWaterSurface(HU3DMODEL *model, Mtx matrix);
void DrawStageBackdrop(HU3DMODEL *model, Mtx matrix);
void DrawStage(HU3DMODEL *model, Mtx matrix);

OMOBJ *CreateStageObject(void)
{
    OMOBJ *object = omAddObjEx(stageObjectManager, 127, 15, 0, -1, UpdateStageObject);
    M456StageWork *work = object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M456StageWork), HU_MEMNUM_OVL);
    M456StageWork *work2 = work;
    SprinkleEffectWork *sprinkleWork = &work->sprinkles[0];
    FramebufferWork *framebufferWork;
    s32 id;
    HU3DMODEL *modelP;
    s32 i;

    work->verticalBobOffset = 0;
    work->stageHeight = 0;
    work->animationFrame = 0;
    work->rippleFrame = 0;
    work->rippleAngle = 0;
    work->waterTextureOffsetX = 0;
    work->waterTextureOffsetY = 0;

    object->mdlId[0] = id = Hu3DHookFuncCreate(DrawStage);
    Hu3DModelCameraSet(id, HU3D_CAM0);
    Hu3DModelLayerSet(id, 5);
    modelP = &Hu3DData[id];
    modelP->hookData = work;
    work->spriteAnimations[4] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 21), HU_MEMNUM_OVL));
    work->spriteAnimations[5] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 22), HU_MEMNUM_OVL));
    work->spriteAnimations[6] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 23), HU_MEMNUM_OVL));
    work->spriteAnimations[7] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 24), HU_MEMNUM_OVL));
    work->spriteAnimations[0] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 17), HU_MEMNUM_OVL));
    work->spriteAnimations[1] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 18), HU_MEMNUM_OVL));
    work->spriteAnimations[2] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 19), HU_MEMNUM_OVL));
    work->spriteAnimations[3] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 20), HU_MEMNUM_OVL));
    work->spriteAnimations[8] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 25), HU_MEMNUM_OVL));
    object->mdlId[1] = id = Hu3DHookFuncCreate(DrawWaterSurface);
    Hu3DModelCameraSet(id, HU3D_CAM0);
    Hu3DModelLayerSet(id, 0);
    modelP = &Hu3DData[id];
    modelP->hookData = work;
    work2->displayListSize = 16384;
    work2->displayList = HuMemDirectMallocNum(HEAP_HEAP, work2->displayListSize, HU_MEMNUM_OVL);
    DCFlushRange(work2->displayList, 16384);
    GXBeginDisplayList(work2->displayList, work2->displayListSize);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition2f32(-0.5f, -0.5f);
    GXTexCoord2f32(0, 0);
    GXPosition2f32(0.5f, -0.5f);
    GXTexCoord2f32(1, 0);
    GXPosition2f32(0.5f, 0.5f);
    GXTexCoord2f32(1, 1);
    GXPosition2f32(-0.5f, 0.5f);
    GXTexCoord2f32(0, 1);
    work2->displayListSize = GXEndDisplayList();
    for (i = 0; i < 26; i++) {
        work2->waterTextureCoordinates[i][0] = 0;
        work2->waterTextureCoordinates[i][1] = 0;
        work2->waterTextureDrift[i][0] = 0;
        work2->waterTextureDrift[i][1] = 0;
    }

    object->mdlId[3] = id = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M456, 5));
    Hu3DModelPosSet(id, 0, 0, -5000);
    Hu3DModelScaleSet(id, 2, 2, 2);
    Hu3DModelCameraSet(id, HU3D_CAM0);
    Hu3DModelLayerSet(id, 1);
    id = Hu3DTexScrollCreate(object->mdlId[3], "S3TCsk");
    Hu3DTexScrollPosMoveSet(id, -0.0055f*(1.0f/REFRESH_RATE), 0, 0);
    id = Hu3DTexScrollCreate(object->mdlId[3], "pa_sk3");
    Hu3DTexScrollPosMoveSet(id, -0.0125f*(1.0f/REFRESH_RATE), 0, 0);

    object->mdlId[4] = id = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M456, 6));
    Hu3DModelPosSet(id, 0, 0, -3000);
    Hu3DModelScaleSet(id, 1.2f, 1.2f, 1.2f);
    Hu3DModelCameraSet(id, HU3D_CAM0);
    Hu3DModelLayerSet(id, 1);

    object->mdlId[8] = id = Hu3DHookFuncCreate(DrawStageBackdrop);
    Hu3DModelCameraSet(id, HU3D_CAM0);
    Hu3DModelLayerSet(id, 2);
    modelP = &Hu3DData[id];
    modelP->hookData = work;

    object->mdlId[10] = id = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M456, 0));
    Hu3DModelPosSet(id, 0, 0, 0);
    Hu3DModelCameraSet(id, HU3D_CAM0);
    Hu3DModelLayerSet(id, 3);
    Hu3DModelShadowMapSet(object->mdlId[10]);
    for (i = 0; i < 4; i++) {

        object->mdlId[i + 11] = id = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M456, 1));
        Hu3DModelCameraSet(object->mdlId[i + 11], HU3D_CAM0);
        Hu3DModelLayerSet(object->mdlId[i + 11], 3);
        Hu3DModelPosSet(object->mdlId[i + 11], fishStartPositions[i].x, fishStartPositions[i].y, fishStartPositions[i].z);
        Hu3DModelAttrSet(object->mdlId[i + 11], HU3D_MOTATTR_SHAPE_LOOP);
        Hu3DMotionShapeTimeSet(object->mdlId[i + 11], (0.25 * i) * Hu3DMotionShapeMaxTimeGet(object->mdlId[i + 11]));
    }
    modelP = &Hu3DData[object->mdlId[0]];
    framebufferWork = &work2->framebuffers[0];
    for (i = 0; i < 4; i++, framebufferWork++) {
        framebufferWork->sourceWidth = framebufferConfigs[i].sourceWidth;
        framebufferWork->sourceHeight = framebufferConfigs[i].sourceHeight;
        framebufferWork->textureWidth = framebufferConfigs[i].textureWidth;
        framebufferWork->textureHeight = framebufferConfigs[i].textureHeight;
        framebufferWork->useScreenCapture = framebufferConfigs[i].useScreenCapture;
        framebufferWork->textureBufferSize = GXGetTexBufferSize(framebufferWork->textureWidth, framebufferWork->textureHeight, GX_TF_RGB565, GX_FALSE, GX_FALSE);
        framebufferWork->textureBuffer = HuMemDirectMallocNum(HEAP_MODEL, framebufferWork->textureBufferSize, modelP->mallocNo);
        memset(framebufferWork->textureBuffer, 0, framebufferWork->textureBufferSize);
        DCFlushRange(framebufferWork->textureBuffer, framebufferWork->textureBufferSize);
    }
    for (i = 0; i < 64; i++, sprinkleWork++) {
        sprinkleWork->position.x = sprinkleWork->position.y = sprinkleWork->position.z = 0;
        sprinkleWork->effectParameters.x = 0;
        sprinkleWork->effectParameters.y = 0;
        sprinkleWork->effectParameters.z = 1;
        sprinkleWork->frameDuration = 1.0f / REFRESH_RATE;
        sprinkleWork->lifespan = REFRESH_RATE;
        sprinkleWork->remainingFrames = 0;
    }
    return object;
}

void SetStageParticleFade(float alpha);

void UpdateStageObject(OMOBJ *object)
{
    M456StageWork *work = object->data;
    M456StageWork *work2 = work;
    s32 i;
    if ((work->rippleAngle = work->rippleAngle + (0.007874016f * ((s32)frand() & 0x7F) * 3.0f)) > 360) {
        work->rippleAngle -= 360;
    }
    work->stageHeight = (25 + work->verticalBobOffset) + (5.0 * sind(work->rippleAngle));
    Hu3DModelPosSet(object->mdlId[10], 0, work->stageHeight, 0);
    for (i = 0; i < 4; i++) {
        Hu3DModelPosSet(object->mdlId[i + 11], fishStartPositions[i].x, work->stageHeight + fishStartPositions[i].y, fishStartPositions[i].z);
    }
    work->waterTextureOffsetX += 0.3f*(0.2f/REFRESH_RATE); // 0.06f / REFRESH_RATE
    work->waterTextureOffsetY += 0.3f*(0.2f/REFRESH_RATE);
    work->animationFrame++;
    work->rippleFrame++;
    if (work->rippleFrame >= (float)REFRESH_RATE && work->rippleFrame <= (REFRESH_RATE*1.35f)) {
        float time = 1 - ((work->rippleFrame - (float)REFRESH_RATE) / (REFRESH_RATE*0.35f));
        if (time < 0.0f) {
            time = 0.0f;
        }
        SetStageParticleFade(time);
    }
}

void DestroyStageObject(OMOBJ *object)
{
    M456StageWork *work = object->data;
    HuSprAnimKill(work->spriteAnimations[0]);
    HuSprAnimKill(work->spriteAnimations[1]);
    HuSprAnimKill(work->spriteAnimations[2]);
    HuSprAnimKill(work->spriteAnimations[3]);
    HuSprAnimKill(work->spriteAnimations[6]);
    HuSprAnimKill(work->spriteAnimations[7]);
    HuSprAnimKill(work->spriteAnimations[4]);
    HuSprAnimKill(work->spriteAnimations[5]);
    HuSprAnimKill(work->spriteAnimations[8]);
}

GXColor sprinkleColor = { 255, 255, 255, 32 };
GXColor faintSprinkleColor = { 255, 255, 255, 10 };
GXColor waterEffectColor = { 0, 126, 126, 255 };
Vec waterSurfaceCorners[4] = { -1000, 50, 1000, -1000, 50, -1000, 1000, 50, -1000, 1000, 50, 1000 };

Vec waterTextureCoordinates[26] = { -1500, 0, 1000, -1400, 0, 1000, 0, 0, 1000, 1400, 0, 1000, 1500, 0, 1000, -1500, 0, -500, -1400, 0, -500, 0, 0, -500, 1400,
    0, -500, 1500, 0, -500, -1500, 0, -2000, -1400, 0, -2000, 0, 0, -2000, 1400, 0, -2000, 1500, 0, -2000, -1500, 0, -5000, -1400, 0, -5000, 0, 0,
    -5000, 1400, 0, -5000, 1500, 0, -5000, -2000, 0, -500, -2500, 0, -2000, -3000, 0, -5000, 2000, 0, -500, 2500, 0, -2000, 3000, 0, -5000 };

GXColor waterVertexColors[26] = { { 96, 128, 255, 255 }, { 128, 160, 255, 32 }, { 128, 160, 255, 32 }, { 128, 160, 255, 32 }, { 96, 128, 255, 255 },
    { 24, 48, 255, 255 }, { 96, 128, 255, 32 }, { 128, 160, 255, 32 }, { 96, 128, 255, 32 }, { 24, 48, 255, 255 }, { 24, 48, 255, 255 },
    { 24, 48, 255, 255 }, { 96, 128, 255, 255 }, { 24, 48, 255, 255 }, { 24, 48, 255, 255 }, { 8, 24, 143, 255 }, { 8, 24, 143, 255 },
    { 8, 24, 143, 255 }, { 8, 24, 143, 255 }, { 8, 24, 143, 255 }, { 24, 48, 255, 255 }, { 24, 48, 255, 255 }, { 24, 48, 255, 255 },
    { 24, 48, 255, 255 }, { 24, 48, 255, 255 }, { 24, 48, 255, 255 } };

float waterTextureScale[26][2]
    = { 0, 1, 0.033333335, 1, 0.5, 1, 0.96666664, 1, 1, 1, 0, 0.5, 0.033333335, 0.5, 0.5, 0.5, 0.96666664, 0.5, 1, 0.5, 0, 0, 0.033333335, 0, 0.5, 0,
          0.96666664, 0, 1, 0, 0, 0, 0.033333335, 0, 0.5, 0, 0.96666664, 0, 1, 0, -0.15, 0.5, -0.33, 0, -0.49, 0, 1.15, 0.5, 1.33, 0, 1.49, 0 };

float waterTextureTileCoordinates[26][2] = { 0, 30, 0.5, 30, 7.5, 30, 14.5, 30, 15, 30, 0, 15, 0.5, 15, 7.5, 15, 14.5, 15, 15, 15, 0, 7.5, 0.5, 7.5, 7.5, 7.5,
    14.5, 7.5, 15, 7.5, 0, 0, 0.5, 0, 7.5, 0, 14.5, 0, 15, 0, -2.25, 15, -4.9500003, 7.5, -7.3500004, 0, 17.25, 15, 19.95, 7.5, 22.35, 0 };

float rippleAnimationData[2][3] = { 0, 0, 0.5, 0, 0.5, 0 };

GXColor fishShadowColor = { 80, 80, 80, 80 };

Vec fishSchoolPositions[9]
    = { -1500, 0, -5000, 0, 0, -5000, 1500, 0, -5000, -1500, 0, -3000, 0, 0, -3000, 1500, 0, -3000, -1500, 0, 1000, 0, 0, 1000, 1500, 0, 1000 };

GXColor fishSchoolColors[9] = {
    { 255, 255, 255, 0 },
    { 255, 255, 255, 0 },
    { 255, 255, 255, 0 },
    { 255, 255, 255, 0 },
    { 255, 255, 255, 255 },
    { 255, 255, 255, 0 },
    { 255, 255, 255, 0 },
    { 255, 255, 255, 255 },
    { 255, 255, 255, 0 },
};

GXColor stageWaterColor = { 255, 165, 89, 178 };
GXColor stageWaterShadowColor = { 153, 114, 89, 178 };
GXColor framebufferClearColor = { 0, 0, 0, 255 };

void CaptureFramebufferTexture(FramebufferWork *framebufferWork, s32 clearAfterCopy);

void DrawWaterSurface(HU3DMODEL *model, Mtx matrix)
{
    M456StageWork *work = model->hookData;
    M456StageWork *work2 = work;
    SprinkleEffectWork *sprinkleWork = &work2->sprinkles[0];
    FramebufferWork *framebufferWork = &work2->framebuffers[0];
    s32 i;
#ifdef NON_MATCHING
    Mtx44 proj;
#else
    Mtx proj;
#endif
    Mtx modelview;
    MTXOrtho(proj, 0, 480, 0, 640, 0, 100);
    GXSetProjection(proj, GX_ORTHOGRAPHIC);
    MTXIdentity(modelview);
    GXLoadPosMtxImm(modelview, GX_PNMTX0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanMatColor(GX_COLOR0A0, waterEffectColor);
    GXSetTevColor(GX_TEVREG0, faintSprinkleColor);
    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_TEXC, GX_CC_A0, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetScissor(0, 0, 400, 400);
    HuSprTexLoad(work->spriteAnimations[4], 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition2u16(0, 0);
    GXTexCoord2f32(work->waterTextureOffsetX, work->waterTextureOffsetY);
    GXPosition2u16(400, 0);
    GXTexCoord2f32(work->waterTextureOffsetX + 7, work->waterTextureOffsetY);
    GXPosition2u16(400, 400);
    GXTexCoord2f32(work->waterTextureOffsetX + 7, work->waterTextureOffsetY + 7);
    GXPosition2u16(0, 400);
    GXTexCoord2f32(work->waterTextureOffsetX, work->waterTextureOffsetY + 7);
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
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    HuSprTexLoad(work->spriteAnimations[8], 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    sprinkleWork = &work2->sprinkles[0];
    for (i = 0; i < 64; i++, sprinkleWork++) {
        float scale;
        float accel;
        if (sprinkleWork->remainingFrames == 0) {
            continue;
        }
        scale = 128 * (sprinkleWork->effectParameters.x * (0.2f + cosd(sprinkleWork->remainingFrames * sprinkleWork->frameDuration * 90.0f)));
        accel = sprinkleWork->remainingFrames * sprinkleWork->frameDuration;
        sprinkleColor.a = sprinkleWork->effectParameters.z * sind((accel * accel) * 90.0f) * 255.0;
        GXSetChanMatColor(GX_COLOR0A0, sprinkleColor);
        MTXScale(modelview, 0.5f * scale, 0.5f * scale, 1.0f);
        mtxRotCat(modelview, 0, 0, sprinkleWork->effectParameters.y);
        mtxTransCat(modelview, 400 * (sprinkleWork->position.x + 1500) / 3000, 400 * (sprinkleWork->position.z - -2000) / 3000, 0);
        GXLoadPosMtxImm(modelview, GX_PNMTX0);
        GXCallDisplayList(work2->displayList, work2->displayListSize);
        if (!omPauseChk()) {
            sprinkleWork->remainingFrames--;
        }
    }
    framebufferWork = &work2->framebuffers[0];
    CaptureFramebufferTexture(framebufferWork, 1);
}

void TransformStageModel(HU3DMODEL *model, Mtx matrix);

void DrawStageBackdrop(HU3DMODEL *model, Mtx matrix)
{
    M456StageWork *work;
    M456StageWork *work2;
    FramebufferWork *framebufferWork;
    TransformStageModel(model, matrix);
    work = model->hookData;
    work2 = work;
    framebufferWork = &work2->framebuffers[1];
    CaptureFramebufferTexture(framebufferWork, 0);
}

void DrawFramebufferTexture(FramebufferWork *framebufferWork, float xOffset, float yOffset, float scale);

void DrawStageModel(HU3DMODEL *model, Mtx matrix)
{
    M456StageWork *work = model->hookData;
    M456StageWork *work2 = work;
    FramebufferWork *framebufferWork = &work2->framebuffers[0];

    DrawFramebufferTexture(framebufferWork, 200, 0, 0.5f);
    framebufferWork = &work2->framebuffers[1];
    DrawFramebufferTexture(framebufferWork, 0, 0, 0.5f);
    framebufferWork = &work2->framebuffers[2];
    DrawFramebufferTexture(framebufferWork, 0, 150, 0.25f);
    framebufferWork = &work2->framebuffers[3];
    DrawFramebufferTexture(framebufferWork, 0, 300, 0.5f);
}

void LoadFramebufferTexture(s16 textureMapId, FramebufferWork *framebufferWork, GXTexWrapMode wrapMode, s32 unusedFlags, BOOL useLinearFiltering);

void DrawWaterEffect(HU3DMODEL *model, Mtx matrix)
{
    HU3DCAMERA *cameraP = &Hu3DCamera[Hu3DCameraNo];
    M456StageWork *work = model->hookData;
    M456StageWork *work2 = work;
    FramebufferWork *framebufferWork = &work2->framebuffers[2];
    float fov;
    Mtx proj;
    Mtx texMtx;
    Mtx projCamera;
    s16 lightMask;
    s16 i;

    CaptureFramebufferTexture(framebufferWork, 0);
    GXLoadPosMtxImm(matrix, GX_PNMTX0);
    fov = cameraP->fov;
    if (fov <= 0.0f) {
        fov = 45;
    }
    MTXLightPerspective(proj, fov, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, texMtx);
    MTXConcat(texMtx, Hu3DCameraMtx, texMtx);
    MTXConcat(proj, Hu3DCameraMtx, projCamera);
    MTXConcat(projCamera, texMtx, texMtx);
    GXLoadTexMtxImm(texMtx, GX_TEXMTX0, GX_MTX3x4);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX0);
    lightMask = Hu3DLightSet(model, &Hu3DCameraMtx, &Hu3DCameraMtxXPose, 4);
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetNumChans(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanMatColor(GX_COLOR0A0, fishParticleColor);
    GXSetZMode(GX_TRUE, GX_GREATER, GX_FALSE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    framebufferWork = &work2->framebuffers[1];
    LoadFramebufferTexture(0, framebufferWork, 0, 0, 0);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetArray(GX_VA_POS, waterSurfaceCorners, sizeof(Vec));
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    for (i = 0; i < 4; i++) {
        GXPosition1x16(i);
    }
    GXEnd();
    framebufferWork = &work2->framebuffers[3];
    CaptureFramebufferTexture(framebufferWork, 0);
    framebufferWork = &work2->framebuffers[2];
    LoadFramebufferTexture(0, framebufferWork, 0, 0, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    for (i = 0; i < 4; i++) {
        GXPosition1x16(i);
    }
    GXEnd();
}

void DrawStage(HU3DMODEL *model, Mtx matrix)
{
    HU3DCAMERA *cameraP = &Hu3DCamera[Hu3DCameraNo];
    M456StageWork *work = model->hookData;
    M456StageWork *work2 = work;
    FramebufferWork *framebufferWork = &work2->framebuffers[2];
    Mtx invXPose;
    Mtx proj;
    Mtx texMtx;
    Mtx projCamera;
    float lightProjectionScale;
    float lightProjectionOffset;
    float fov;
    s16 i;
    DrawWaterEffect(model, matrix);
    lightProjectionScale = 1.5f;
    lightProjectionOffset = 0;
    GXLoadPosMtxImm(matrix, GX_PNMTX0);
    MTXInvXpose(matrix, invXPose);
    GXLoadNrmMtxImm(invXPose, GX_PNMTX0);
    fov = cameraP->fov;
    if (fov <= 0.0f) {
        fov = 45;
    }
    MTXLightPerspective(proj, fov, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXInverse(Hu3DCameraMtx, texMtx);
    MTXConcat(texMtx, Hu3DCameraMtx, texMtx);
    MTXConcat(proj, Hu3DCameraMtx, projCamera);
    MTXConcat(projCamera, texMtx, texMtx);
    GXLoadTexMtxImm(texMtx, GX_TEXMTX0, GX_MTX3x4);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX0);
    GXSetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX1, GX_IDENTITY);
    GXSetNumTevStages(2);
    GXSetNumTexGens(3);
    GXSetNumChans(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_RASC, GX_CC_RASA, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevColor(GX_TEVREG0, fishShadowColor);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C0, GX_CC_TEXA, GX_CC_CPREV);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_VTX, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanMatColor(GX_COLOR0A0, fishParticleColor);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
    framebufferWork = &work2->framebuffers[3];
    LoadFramebufferTexture(0, framebufferWork, 0, 0, 0);
    HuSprTexLoad(work->spriteAnimations[3], 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    framebufferWork = &work2->framebuffers[0];
    LoadFramebufferTexture(2, framebufferWork, 0, 0, 1);
    GXSetTexCoordGen(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetNumIndStages(1);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD2, GX_TEXMAP2);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
    GXSetTevIndWarp(GX_TEVSTAGE0, GX_INDTEXSTAGE0, GX_TRUE, GX_FALSE, GX_ITM_1);
    GXSetIndTexMtx(GX_ITM_1, rippleAnimationData, -1);
    for (i = 0; i < 26; i++) {
        work2->waterTextureDrift[i][0] -= 0.007874016f * ((s32)frand() & 0x7F) * 0.005f;
        work2->waterTextureDrift[i][1] += 0.007874016f * ((s32)frand() & 0x7F) * 0.005f;
        work2->waterTextureCoordinates[i][0] = work2->waterTextureDrift[i][0] + waterTextureTileCoordinates[i][0];
        work2->waterTextureCoordinates[i][1] = work2->waterTextureDrift[i][1] + waterTextureTileCoordinates[i][1];
    }
    DCFlushRange(&work2->waterTextureCoordinates[0][0], sizeof(work2->waterTextureCoordinates));
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);

    GXSetArray(GX_VA_POS, waterTextureCoordinates, sizeof(Vec));
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, waterVertexColors, sizeof(GXColor));
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_TEX0, waterTextureScale, sizeof(float) * 2);
    GXSetVtxDesc(GX_VA_TEX1, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_TEX1, &work2->waterTextureCoordinates[0][0], sizeof(float) * 2);
    GXBegin(GX_QUADS, GX_VTXFMT0, 48);
    for (i = 0; i < 12; i++) {
        s16 index;
        if (i == 4) {
            index = 12;
        }
        else if (i == 9) {
            index = 13;
        }
        else {
            index = i;
        }
        GXPosition1x16(index + 0);
        GXColor1x16(index + 0);
        GXTexCoord1x16(index + 0);
        GXTexCoord1x16(index + 0);
        GXPosition1x16(index + 5);
        GXColor1x16(index + 5);
        GXTexCoord1x16(index + 5);
        GXTexCoord1x16(index + 5);
        GXPosition1x16(index + 6);
        GXColor1x16(index + 6);
        GXTexCoord1x16(index + 6);
        GXTexCoord1x16(index + 6);
        GXPosition1x16(index + 1);
        GXColor1x16(index + 1);
        GXTexCoord1x16(index + 1);
        GXTexCoord1x16(index + 1);
    }
    GXEnd();
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 8);
    GXPosition1x16(22);
    GXColor1x16(22);
    GXTexCoord1x16(22);
    GXTexCoord1x16(22);
    GXPosition1x16(15);
    GXColor1x16(15);
    GXTexCoord1x16(15);
    GXTexCoord1x16(15);
    GXPosition1x16(21);
    GXColor1x16(21);
    GXTexCoord1x16(21);
    GXTexCoord1x16(21);
    GXPosition1x16(10);
    GXColor1x16(10);
    GXTexCoord1x16(10);
    GXTexCoord1x16(10);
    GXPosition1x16(20);
    GXColor1x16(20);
    GXTexCoord1x16(20);
    GXTexCoord1x16(20);
    GXPosition1x16(5);
    GXColor1x16(5);
    GXTexCoord1x16(5);
    GXTexCoord1x16(5);
    GXPosition1x16(0);
    GXColor1x16(0);
    GXTexCoord1x16(0);
    GXTexCoord1x16(0);
    GXPosition1x16(0);
    GXColor1x16(0);
    GXTexCoord1x16(0);
    GXTexCoord1x16(0);
    GXEnd();
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 8);
    GXPosition1x16(19);
    GXColor1x16(19);
    GXTexCoord1x16(19);
    GXTexCoord1x16(19);
    GXPosition1x16(25);
    GXColor1x16(25);
    GXTexCoord1x16(25);
    GXTexCoord1x16(25);
    GXPosition1x16(14);
    GXColor1x16(14);
    GXTexCoord1x16(14);
    GXTexCoord1x16(14);
    GXPosition1x16(24);
    GXColor1x16(24);
    GXTexCoord1x16(24);
    GXTexCoord1x16(24);
    GXPosition1x16(9);
    GXColor1x16(9);
    GXTexCoord1x16(9);
    GXTexCoord1x16(9);
    GXPosition1x16(23);
    GXColor1x16(23);
    GXTexCoord1x16(23);
    GXTexCoord1x16(23);
    GXPosition1x16(4);
    GXColor1x16(4);
    GXTexCoord1x16(4);
    GXTexCoord1x16(4);
    GXPosition1x16(4);
    GXColor1x16(4);
    GXTexCoord1x16(4);
    GXTexCoord1x16(4);
    GXEnd();
    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevDirect(GX_TEVSTAGE1);
}

void TransformStageModel(HU3DMODEL *model, Mtx matrix)
{
    s32 vertexIndex = 0;
    M456StageWork *work = model->hookData;
    float scale = 1.5f;
    float x = 0;
    Mtx transformMatrix;
    Mtx final;
    MTXTrans(final, x, -300, 0);
    GXSetTevColor(GX_TEVREG0, stageWaterColor);
    MTXScale(transformMatrix, scale, 1, 1);
    MTXConcat(final, transformMatrix, final);
    MTXConcat(Hu3DCameraMtx, final, final);
    GXLoadPosMtxImm(final, GX_PNMTX0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanMatColor(GX_COLOR0A0, fishParticleColor);
    HuSprTexLoad(work->spriteAnimations[0], 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    HuSprTexLoad(work->spriteAnimations[1], 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetNumTexGens(2);
    MTXRotDeg(final, 'X', 90.0f);
    x = 0.004f * scale;
    MTXScale(transformMatrix, x, -0.004f, 0.004f);
    MTXConcat(final, transformMatrix, final);
    MTXTrans(transformMatrix, (work->animationFrame % 1024) / (512.0f * x), 0, 1.2499999f * (work->animationFrame % 400));
    MTXConcat(final, transformMatrix, final);
    GXLoadTexMtxImm(final, GX_TEXMTX0, GX_MTX2x4);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_POS, GX_TEXMTX0);
    MTXRotDeg(final, 'X', 90.0f);
    x = 0.004f * scale;
    MTXScale(transformMatrix, x, -0.004f, 0.004f);
    MTXConcat(final, transformMatrix, final);
    MTXTrans(transformMatrix, (work->animationFrame % 300) / (150.0f * x), 0, 0);
    MTXConcat(final, transformMatrix, final);
    GXLoadTexMtxImm(final, GX_TEXMTX1, GX_MTX2x4);
    GXSetTexCoordGen(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_POS, GX_TEXMTX1);
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
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_POS, fishSchoolPositions, sizeof(Vec));
    GXSetArray(GX_VA_CLR0, fishSchoolColors, sizeof(GXColor));
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 6);
    GXPosition1x8(0);
    GXColor1x8(0);
    GXPosition1x8(1);
    GXColor1x8(1);
    GXPosition1x8(3);
    GXColor1x8(3);
    GXPosition1x8(4);
    GXColor1x8(4);
    GXPosition1x8(6);
    GXColor1x8(6);
    GXPosition1x8(7);
    GXColor1x8(7);
    GXEnd();
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 6);
    GXPosition1x8(1);
    GXColor1x8(1);
    GXPosition1x8(2);
    GXColor1x8(2);
    GXPosition1x8(4);
    GXColor1x8(4);
    GXPosition1x8(5);
    GXColor1x8(5);
    GXPosition1x8(7);
    GXColor1x8(7);
    GXPosition1x8(8);
    GXColor1x8(8);
    GXEnd();
}

void CaptureFramebufferTexture(FramebufferWork *framebufferWork, s32 clearAfterCopy)
{
    GXSetTexCopySrc(0, 0, framebufferWork->sourceWidth, framebufferWork->sourceHeight);
    GXSetTexCopyDst(framebufferWork->textureWidth, framebufferWork->textureHeight, GX_TF_RGB565, framebufferWork->useScreenCapture);
    GXSetCopyClear(framebufferClearColor, GX_MAX_Z24);
    GXCopyTex(framebufferWork->textureBuffer, clearAfterCopy);
}

void DrawFramebufferTexture(FramebufferWork *fbWork, float x1, float y1, float scale)
{
    float x2 = x1 + (fbWork->textureWidth * scale);
    float y2 = y1 + (fbWork->textureHeight * scale);
    Mtx44 proj;
    Mtx modelview;
    GXTexObj texObj;
    GXColor color;
    GXInitTexObj(&texObj, fbWork->textureBuffer, fbWork->textureWidth, fbWork->textureHeight, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObjLOD(&texObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXLoadTexObj(&texObj, GX_TEXMAP0);
    MTXOrtho(proj, 0, 480, 0, 640, 0, 10);
    GXSetProjection(proj, GX_ORTHOGRAPHIC);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    color.a = 255;
    GXSetTevColor(GX_TEVREG0, color);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumTexGens(1);
    GXSetNumTevStages(1);
    MTXIdentity(modelview);
    GXLoadPosMtxImm(modelview, GX_PNMTX0);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(x1, y1, 0);
    GXTexCoord2f32(0, 0);
    GXPosition3f32(x2, y1, 0);
    GXTexCoord2f32(1, 0);
    GXPosition3f32(x2, y2, 0);
    GXTexCoord2f32(1, 1);
    GXPosition3f32(x1, y2, 0);
    GXTexCoord2f32(0, 1);
    GXEnd();
}

void LoadFramebufferTexture(s16 textureMapId, FramebufferWork *framebufferWork, GXTexWrapMode wrapMode, s32 unusedFlags, BOOL useLinearFiltering)
{
    GXTexObj texObj;
    GXInitTexObj(&texObj, framebufferWork->textureBuffer, framebufferWork->textureWidth, framebufferWork->textureHeight, GX_TF_RGB565, wrapMode, wrapMode, FALSE);
    if (useLinearFiltering) {
        GXInitTexObjLOD(&texObj, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    }
    else {
        GXInitTexObjLOD(&texObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    }
    GXLoadTexObj(&texObj, textureMapId);
}

s32 CreateSprinkleEffect(Vec *position, float scale, float opacity, s32 lifespan)
{
    M456StageWork *work;
    M456StageWork *work2;
    SprinkleEffectWork *sprinkleWork;
    s32 i;
    if (!mainStageObject) {
        return -1;
    }
    work = mainStageObject->data;
    work2 = work;
    sprinkleWork = &work->sprinkles[0];
    for (i = 0; i < 64; i++, sprinkleWork++) {
        if (sprinkleWork->remainingFrames == 0) {
            break;
        }
    }
    if (i >= 64) {
        return -1;
    }
    sprinkleWork->position = *position;
    if (scale < 0.0f) {
        sprinkleWork->effectParameters.x = 1;
    }
    else {
        sprinkleWork->effectParameters.x = scale;
    }
    if (opacity < 0.0f) {
        sprinkleWork->effectParameters.z = 1;
    }
    else {
        sprinkleWork->effectParameters.z = opacity;
    }
    if (lifespan < 0) {
        sprinkleWork->lifespan = REFRESH_RATE;
    }
    else {
        sprinkleWork->lifespan = lifespan;
    }
    sprinkleWork->effectParameters.y = (s32)frand() % 360;
    sprinkleWork->remainingFrames = sprinkleWork->lifespan;
    sprinkleWork->frameDuration = 1.0f / sprinkleWork->lifespan;
    return i;
}

float GetStageHeight(void)
{
    if (!mainStageObject) {
        return 0;
    }
    else {
        M456StageWork *work = mainStageObject->data;
        return work->stageHeight;
    }
}

float GetStageWaveOffset(void)
{
    if (!mainStageObject) {
        return 0;
    }
    else {
        M456StageWork *work = mainStageObject->data;
        return work->verticalBobOffset;
    }
}

typedef struct FishParticleWork {
    s32 particleIndex;
    s32 particleModelId;
    s32 isActive;
    s32 elapsedFrames;
    ANIMDATA *particleAnimation;
    Vec position;
    Vec rotation;
    Vec scale;
} FishParticleWork;

void UpdateFishObject(OMOBJ *object);

void InitializeFishObject(OMOBJ *object)
{
    FishParticleWork *work;
    s32 i;
    ANIMDATA *anim;
    work = object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(FishParticleWork) * 8, HU_MEMNUM_OVL);
    memset(work, 0, sizeof(FishParticleWork) * 8);
    anim = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 26), HU_MEMNUM_OVL));
    for (i = 0; i < 8; i++, work++) {
        work->particleAnimation = anim;
        work->particleIndex = i;
        work->particleModelId = Hu3DParticleCreate(work->particleAnimation, 32);
        Hu3DModelCameraSet(work->particleModelId, HU3D_CAM0);
        Hu3DModelLayerSet(work->particleModelId, 6);
        Hu3DModelAttrSet(work->particleModelId, HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(work->particleModelId, HU3D_ATTR_PARTICLE_KILL);
        work->isActive = 0;
        work->elapsedFrames = 0;
        work->position.x = work->position.y = work->position.z = 0;
        work->rotation.x = work->rotation.y = work->rotation.z = 0;
        work->scale.x = work->scale.y = work->scale.z = 1;
    }
    object->objFunc = UpdateFishObject;
}

void UpdateFishObject(OMOBJ *object)
{
    FishParticleWork *work = object->data;
    s32 i;
    for (i = 0; i < 8; i++, work++) {
        if (work->isActive == 0) {
            continue;
        }
        if (work->elapsedFrames > 1) {
            HU3DMODEL *modelP = &Hu3DData[work->particleModelId];
            HU3DPARTICLE *particleP = modelP->hookData;
            HU3DPARTICLEDATA *data = particleP->data;
            s32 j;
            for (j = 0; j < particleP->maxCnt; j++, data++) {
                if (++data->time > data->parManId) {
                    s32 alpha = data->color.a - 1;
                    if (alpha < 0) {
                        data->scale = 0;
                    }
                    else {
                        data->color.a = alpha;
                    }
                }
                data->scale *= 0.975;
                data->pos.x += data->vel.x;
                data->pos.y += data->vel.y;
                data->pos.z += data->vel.z;
                data->vel.y -= 0.3103333379576603;
            }
        }
        if (++work->elapsedFrames > (1.5f * REFRESH_RATE)) {
            Hu3DModelAttrSet(work->particleModelId, HU3D_ATTR_DISPOFF);
            work->isActive = 0;
        }
        else {
            Hu3DModelPosSet(work->particleModelId, work->position.x, work->position.y, work->position.z);
            Hu3DModelRotSet(work->particleModelId, work->rotation.x, work->rotation.y, work->rotation.z);
            Hu3DModelScaleSet(work->particleModelId, work->scale.x, work->scale.y, work->scale.z);
        }
    }
}

void DestroyFishObject(OMOBJ *object)
{
    FishParticleWork *work = object->data;
    ANIMDATA *anim = work->particleAnimation;
    anim->useNum = 0;
    HuSprAnimKill(anim);
}

s32 CreateWaterEffect(Vec *position, float scale)
{
    FishParticleWork *work = fishObject->data;
    s32 i;
    HU3DMODEL *particleModel;
    HU3DPARTICLE *particleSystem;
    HU3DPARTICLEDATA *particleData;

    for (i = 0; i < 8; i++, work++) {
        if (work->isActive == 0) {
            break;
        }
    }
    if (i >= 8) {
        return -1;
    }
    Hu3DModelAttrReset(work->particleModelId, HU3D_ATTR_DISPOFF);
    work->isActive = 1;
    work->elapsedFrames = 0;
    particleModel = &Hu3DData[work->particleModelId];
    particleSystem = particleModel->hookData;
    particleSystem->blendMode = 1;
    particleData = particleSystem->data;
    for (i = 0; i < particleSystem->maxCnt; i++, particleData++) {
        float elevationAngle = ((s32)frand() % 20) + 90;
        float radialAngle = (s32)frand() % 360;
        float elevationSine = sind(elevationAngle);
        float elevationCosine = cosd(elevationAngle);
        float randomScale = 0.75f + 0.15f * (0.007874016f * ((s32)frand() & 0x7F));
        float velocityScale = 0.15f;
        particleData->vel.x = velocityScale * (0.65f * (50.0 * (elevationCosine * sind(radialAngle))));
        particleData->vel.z = velocityScale * (0.65f * (50.0 * (elevationCosine * cosd(radialAngle))));
        particleData->vel.y = velocityScale * (50 * elevationSine * randomScale);
        particleData->time = 0;
        particleData->parManId = ((s32)frand() % 16) + 45;
        particleData->zRot = (s32)frand() % 360;
        particleData->scale = 145.0f;
        particleData->pos.x = 0.05f * (((s32)frand() & 0x7F) - 64);
        particleData->pos.y = 0.65f * (((s32)frand() & 0x7F) - 64);
        particleData->pos.z = 0.05f * (((s32)frand() & 0x7F) - 64);
        particleData->color.r = ((s32)frand() % 64) + 192;
        particleData->color.g = ((s32)frand() % 64) + 192;
        particleData->color.b = ((s32)frand() % 64) + 192;
        particleData->color.a = 64;
    }
    if (scale < 0.0f) {
        scale = 1.0f;
    }
    work->position = *position;
    work->rotation.x = work->rotation.y = work->rotation.z = 0;
    work->scale.x = work->scale.y = work->scale.z = scale;
    Hu3DModelPosSet(work->particleModelId, work->position.x, work->position.y, work->position.z);
    Hu3DModelRotSet(work->particleModelId, work->rotation.x, work->rotation.y, work->rotation.z);
    Hu3DModelScaleSet(work->particleModelId, work->scale.x, work->scale.y, work->scale.z);
    return i;
}

void UpdateWaterEffect(OMOBJ *object);

void InitializeWaterEffect(OMOBJ *object)
{
    s32 i;
    for (i = 0; i < object->mdlcnt; i++) {
        if (i == 0) {
            object->mdlId[i] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M456, 9));
        }
        else {
            object->mdlId[i] = Hu3DModelLink(object->mdlId[0]);
        }
        Hu3DModelCameraSet(object->mdlId[i], HU3D_CAM0);
        Hu3DModelLayerSet(object->mdlId[i], 6);
        Hu3DModelAttrSet(object->mdlId[i], HU3D_ATTR_DISPOFF);
    }
    object->stat |= 0x100;
    object->objFunc = UpdateWaterEffect;
}

void UpdateWaterEffect(OMOBJ *object)
{
    s32 i;
    for (i = 0; i < object->mdlcnt; i++) {
        HU3DMODEL *modelP = &Hu3DData[object->mdlId[i]];
        if (!(modelP->attr & HU3D_ATTR_DISPOFF)) {
            if (Hu3DMotionTimeGet(object->mdlId[i]) >= Hu3DMotionMaxTimeGet(object->mdlId[i])) {
                Hu3DModelAttrSet(object->mdlId[i], HU3D_ATTR_DISPOFF);
            }
        }
    }
}

void DestroyWaterEffect(OMOBJ *object) { }

s32 CreateWaterParticle(Vec *pos, float scale)
{
    s32 i;
    for (i = 0; i < waterEffectObject->mdlcnt; i++) {
        HU3DMODEL *modelP = &Hu3DData[waterEffectObject->mdlId[i]];
        if (modelP->attr & HU3D_ATTR_DISPOFF) {
            break;
        }
    }
    if (i >= waterEffectObject->mdlcnt) {
        return -1;
    }
    Hu3DMotionTimeSet(waterEffectObject->mdlId[i], 0);
    Hu3DMotionSpeedSet(waterEffectObject->mdlId[i], 1);
    Hu3DModelPosSet(waterEffectObject->mdlId[i], pos->x, pos->y, pos->z);
    Hu3DModelScaleSet(waterEffectObject->mdlId[i], scale, scale, scale);
    Hu3DModelAttrReset(waterEffectObject->mdlId[i], HU3D_ATTR_DISPOFF);
    return i;
}

void UpdateSprinkleObject(OMOBJ *object);

void InitializeSprinkleObject(OMOBJ *object)
{
    HU3DPARTICLE *particleSystem;
    HU3DPARTICLEDATA *particleData;
    s32 i;
    HU3DMODEL *modelP;
    ANIMDATA *anim = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 26), HU_MEMNUM_OVL));

    object->data = anim;
    object->mdlId[0] = Hu3DParticleCreate(anim, 64);
    Hu3DModelCameraSet(object->mdlId[0], HU3D_CAM0);
    Hu3DModelLayerSet(object->mdlId[0], 3);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_PARTICLE_KILL);
    modelP = &Hu3DData[object->mdlId[0]];
    particleSystem = modelP->hookData;
    particleData = particleSystem->data;
    particleSystem->blendMode = 1;
    for (i = 0; i < particleSystem->maxCnt; i++, particleData++) {
        particleData->scale = 0;
    }
    object->objFunc = UpdateSprinkleObject;
}

void UpdateSprinkleObject(OMOBJ *object)
{
    HU3DMODEL *modelP = &Hu3DData[object->mdlId[0]];
    HU3DPARTICLE *particleSystem = modelP->hookData;
    HU3DPARTICLEDATA *particleData = particleSystem->data;
    s32 i;
    for (i = 0; i < particleSystem->maxCnt; i++, particleData++) {
        if (particleData->scale <= 0.0f) {
            continue;
        }
        if (++particleData->time > 360) {
            particleData->time -= 360;
        }
        particleData->pos.x += particleData->vel.x * sind(2.0f * particleData->time);
        particleData->pos.y += particleData->vel.y;
        if (particleData->pos.y >= 0.0f) {
            CreateSprinkleEffect(&particleData->pos, 0.00625f * particleData->scale, 1, REFRESH_RATE / 4);
            particleData->scale = 0;
        }
    }
}

void DestroySprinkleObject(OMOBJ *object)
{
    ANIMDATA *anim = object->data;
    HuSprAnimKill(anim);
    Hu3DModelKill(object->mdlId[0]);
    object->mdlId[0] = -1;
}

s32 CreateSprinkleObject(Vec *pos, float scale)
{
    s32 i;
    HU3DMODEL *modelP = &Hu3DData[sprinkleObject->mdlId[0]];
    HU3DPARTICLE *particleSystem = modelP->hookData;
    HU3DPARTICLEDATA *particleData = particleSystem->data;
    for (i = 0; i < particleSystem->maxCnt; i++, particleData++) {
        if (particleData->scale <= 0.0f) {
            break;
        }
    }
    if (i >= particleSystem->maxCnt) {
        return -1;
    }
    if (scale <= 0.0f) {
        scale = 1.0f;
    }
    particleData->pos = *pos;
    particleData->vel.x = ((60.0f/REFRESH_RATE) * (1.5f + (0.2f * (0.007874016f * (frand() & 0x7F))))) * 0.5f;
    particleData->vel.z = (1.5f + (0.2f * (0.007874016f * (frand() & 0x7F)))) * (60.0f/REFRESH_RATE);
    particleData->vel.y = ((60.0f/REFRESH_RATE) * (1.5f + (0.2f * (0.007874016f * (frand() & 0x7F))))) * 2.0f;
    particleData->time = (s32)frand() % 360;
    particleData->scale = 20 * scale;
    return i;
}

typedef struct WhaleWork {
    s32 modelId;
    s32 isReturning;
    s32 phase;
    float speed;
    float heading;
    Vec targetPosition;
    Vec position;
} WhaleWork;

void UpdateWhaleObject(OMOBJ *object);

void InitializeWhaleObject(OMOBJ *object)
{
    WhaleWork *workBase = object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(WhaleWork) * 8, HU_MEMNUM_OVL);
    WhaleWork *workP = workBase;
    s32 i;
    memset(workP, 0, 8 * sizeof(WhaleWork));
    for (i = 0; i < 8; i++, workP++) {
        switch (i) {
            case 0:
                workP->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M456, 3));
                break;

            case 1:
                workP->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M456, 4));
                break;

            default:
                if (i & 0x1) {
                    workP->modelId = Hu3DModelLink(workBase[0].modelId);
                }
                else {
                    workP->modelId = Hu3DModelLink(workBase[1].modelId);
                }
                break;
        }
        Hu3DModelCameraSet(workP->modelId, HU3D_CAM0);
        Hu3DModelLayerSet(workP->modelId, 1);
        workP->isReturning = 0;
        workP->phase = 0;
        workP->speed = 2;
        workP->heading = (s32)frand() % 360;
        workP->targetPosition.x = 1500 * ((0.007874016f * ((s32)frand() & 0x7F)) - 0.5f);
        workP->targetPosition.z = 500 * ((0.007874016f * ((s32)frand() & 0x7F)) - 0.5f);
        workP->targetPosition.y = -250 + (100 * (0.3f * i));
        workP->position.x = 1500 * ((0.007874016f * ((s32)frand() & 0x7F)) - 0.5f);
        workP->position.z = 500 * ((0.007874016f * ((s32)frand() & 0x7F)) - 0.5f);
        workP->position.y = workP->targetPosition.y;
        Hu3DModelPosSet(workP->modelId, workP->targetPosition.x, workP->targetPosition.y, workP->targetPosition.z);
        Hu3DModelRotSet(workP->modelId, 0, workP->heading, 0);
    }
    object->objFunc = UpdateWhaleObject;
}

float ApproachValue(float start, float end, float weight);

void UpdateWhaleObject(OMOBJ *object)
{
    WhaleWork *workP = object->data;
    s32 i;
    for (i = 0; i < 8; i++, workP++) {
        float dx;
        float dz;
        float vel;
        float angle;
        if (workP->isReturning == 0) {
            vel = 1 + (0.5 * sind((workP->phase * 5) % 180));
        }
        else {
            vel = 3;
        }
        workP->targetPosition.x += ((60.0f/REFRESH_RATE) * (vel * (workP->speed * sind(workP->heading))));
        workP->targetPosition.z += ((60.0f/REFRESH_RATE) * (vel * (workP->speed * cosd(workP->heading))));
        dx = workP->position.x - workP->targetPosition.x;
        dz = workP->position.z - workP->targetPosition.z;
        angle = atan2d(dx, dz);
        workP->heading = ApproachValue(angle, workP->heading, 3.5f);
        if ((dx * dx) + (dz * dz) < 10000.0f) {
            if (workP->isReturning == 0) {
                workP->position.x = 2000 * ((0.007874016f * ((s32)frand() & 0x7F)) - 0.5f);
                workP->position.z = 700 * ((0.007874016f * ((s32)frand() & 0x7F)) - 0.5f);
                workP->position.y = workP->targetPosition.y;
            }
            else {
                if (workP->targetPosition.x < 0.0f) {
                    workP->position.x = -2500;
                }
                else {
                    workP->position.x = 2500;
                }
                workP->position.z = 500 * ((0.007874016f * ((s32)frand() & 0x7F)) - 0.5f);
                workP->position.y = workP->targetPosition.y;
            }
        }
        Hu3DModelPosSet(workP->modelId, workP->targetPosition.x, workP->targetPosition.y, workP->targetPosition.z);
        Hu3DModelRotSet(workP->modelId, 0, workP->heading, 0);
        workP->phase += frand() & 0x3;
    }
}

void DestroyWhaleObject(OMOBJ *object) { }

void SendWhalesOffscreen(void)
{
    WhaleWork *workP = whaleObject->data;
    s32 i;
    for (i = 0; i < 8; i++, workP++) {
        workP->isReturning = 1;
        if (workP->targetPosition.x < 0.0f) {
            workP->position.x = -2500;
        }
        else {
            workP->position.x = 2500;
        }
        workP->position.z = 500 * ((0.007874016f * ((s32)frand() & 0x7F)) - 0.5f);
        workP->position.y = workP->targetPosition.y;
    }
}

typedef struct FishSchoolWork {
    s16 modelId;
    s16 reserved2[2];
    s16 modelCount;
    Vec position;
    Vec targetPosition;
    float speed;
    float targetSpeed;
    float heading;
    float targetHeading;
    float distanceTraveled;
    float animationPhase;
    float turnSpeed;
    float steeringOffset;
} FishSchoolWork;

void UpdateFishModels(OMOBJ *object, FishSchoolWork *workP)
{
    s32 num = workP[0].modelCount;
    float ofsZ = -2000;
    s32 i;
    for (i = 0; i < num; i++, workP++) {
        Mtx *modelMtx;
        Mtx rotZ;
        Hu3DModelPosSet(workP->modelId, workP->position.x, workP->position.y, workP->position.z + ofsZ);
        modelMtx = &Hu3DData[workP->modelId].mtx;
        MTXRotDeg(*modelMtx, 'Y', workP->heading);
        MTXRotDeg(rotZ, 'Z', workP->turnSpeed);
        MTXConcat(*modelMtx, rotZ, *modelMtx);
        Hu3DModelAttrReset(workP->modelId, HU3D_ATTR_DISPOFF);
    }
}

float ApproachHeading(float start, float end, float weight);

void UpdateFishSchool(OMOBJ *object)
{
    FishSchoolWork *workP = object->data;
    s32 num = workP[0].modelCount;
    s32 i;
    for (i = 0; i < num; i++, workP++) {
        Vec diff;
        s32 unusedCounter = 0;
        VECSubtract(&workP->targetPosition, &workP->position, &diff);
        workP->targetHeading = atan2d(diff.x, diff.z);
        workP->heading = ApproachHeading(workP->heading, workP->targetHeading + workP->steeringOffset, 0.08f);
        workP->targetSpeed += 0.05f * (600.0f - workP->targetSpeed);
        workP->speed += 0.2f * (workP->targetSpeed - workP->speed);
        workP->position.x += (workP->speed * (1.0f / REFRESH_RATE)) * sind(workP->heading);
        workP->position.z += (workP->speed * (1.0f / REFRESH_RATE)) * cosd(workP->heading);
        workP->distanceTraveled += workP->speed * (1.0f / REFRESH_RATE);
    }
    UpdateFishModels(object, object->data);
}

void InitializeFishSchool(OMOBJ *object)
{
    s16 num = 12;
    FishSchoolWork *workBase;
    s32 modelId;
    FishSchoolWork *work;
    float radius;
    float angle;
    float ofsZ;
    s32 i;
    object->data = workBase = HuMemDirectMallocNum(HEAP_HEAP, num * sizeof(FishSchoolWork), HU_MEMNUM_OVL);
    memset(workBase, 0, num * sizeof(FishSchoolWork));
    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M456, 8));
    ofsZ = 1500;
    work = workBase;
    for (i = 0; i < num; i++, work++) {

        work->modelId = Hu3DModelLink(modelId);
        Hu3DModelLayerSet(work->modelId, 6);
        Hu3DModelCameraSet(work->modelId, 1);
        Hu3DModelScaleSet(work->modelId, 7, 7, 7);
        work->targetPosition.x = ((s32)frandmod(1000) * 4.0f) - 2000;
        work->targetPosition.z = ((s32)frandmod(1000)) - 500.0f;
        work->targetPosition.y = ((s32)frandmod(1000) * 0.8f) + 300;
        angle = (s32)frandmod(360);
        radius = ((s32)frandmod(250)) + 200.0f;
        work->position.x = radius * sind(angle);
        work->position.y = 0;
        work->position.z = radius * cosd(angle);
        VECAdd(&work->targetPosition, &work->position, &work->position);
        work->turnSpeed = 60 - ((6.0f / 55.0f) * radius);
        work->heading = (s32)frandmod(360);

        work->steeringOffset = ((s32)frandmod(1000) < 500) ? (-70.0f + ((s32)frandmod(50) * 0.1f)) : (70.0f - ((s32)frandmod(50) * 0.1f));
        if (work->steeringOffset < 0.0f) {
            work->turnSpeed = -work->turnSpeed;
        }
        work->speed = 1;
        work->targetSpeed = 0;
        ofsZ -= 50.0f;
        work->modelCount = num;
    }
    Hu3DModelKill(modelId);
    object->objFunc = UpdateFishSchool;
}

float ApproachHeading(float start, float end, float weight)
{
    float angle = fmodf(end - start, 360);
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    if (angle > 180.0f) {
        angle -= 360.0f;
    }
    angle = fmodf(start + (weight * angle), 360);
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    return angle;
}

void UpdateStageParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

void InitializeStageParticles(void)
{
    StageParticleWork *workP = stageParticleWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(StageParticleWork), HU_MEMNUM_OVL);
    HU3DPARTICLE *particle;
    HU3DMODEL *modelP;
    s16 *newWork;
    s32 modelId;
    memset(workP, 0, sizeof(StageParticleWork));
    workP->spriteAnimations[0] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 13), HU_MEMNUM_OVL));
    workP->spriteAnimations[1] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 14), HU_MEMNUM_OVL));
    workP->spriteAnimations[2] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 15), HU_MEMNUM_OVL));
    workP->spriteAnimations[3] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M456, 16), HU_MEMNUM_OVL));
    workP->fadeParameters.y = 1;
    workP->fadeParameters.z = 0;
    workP->particleModelIds[0] = modelId = Hu3DParticleCreate(workP->spriteAnimations[0], 1);
    Hu3DModelLayerSet(modelId, 6);
    modelP = &Hu3DData[modelId];
    particle = modelP->hookData;
    particle->blendMode = 1;
    particle->hook = UpdateStageParticle;
    newWork = particle->work = HuMemDirectMallocNum(HEAP_MODEL, sizeof(s16) * 4, modelP->mallocNo);
    memset(newWork, 0, 4 * sizeof(s16));
    newWork[1] = 0;
    workP->particleModelIds[1] = modelId = Hu3DParticleCreate(workP->spriteAnimations[3], 10);
    Hu3DModelLayerSet(modelId, 6);
    modelP = &Hu3DData[modelId];
    particle = modelP->hookData;
    particle->blendMode = 1;
    particle->hook = UpdateStageParticle;
    newWork = particle->work = HuMemDirectMallocNum(HEAP_MODEL, sizeof(s16) * 4, modelP->mallocNo);
    memset(newWork, 0, 4 * sizeof(s16));
    newWork[1] = 1;
    workP->particleModelIds[2] = modelId = Hu3DParticleCreate(workP->spriteAnimations[2], 4);
    Hu3DModelLayerSet(modelId, 6);
    modelP = &Hu3DData[modelId];
    particle = modelP->hookData;
    particle->blendMode = 1;
    particle->hook = UpdateStageParticle;
    newWork = particle->work = HuMemDirectMallocNum(HEAP_MODEL, sizeof(s16) * 4, modelP->mallocNo);
    memset(newWork, 0, 4 * sizeof(s16));
    newWork[1] = 2;
    workP->particleModelIds[3] = modelId = Hu3DParticleCreate(workP->spriteAnimations[1], 6);
    Hu3DModelLayerSet(modelId, 6);
    modelP = &Hu3DData[modelId];
    particle = modelP->hookData;
    particle->blendMode = 1;
    particle->hook = UpdateStageParticle;
    newWork = particle->work = HuMemDirectMallocNum(HEAP_MODEL, sizeof(s16) * 4, modelP->mallocNo);
    memset(newWork, 0, 4 * sizeof(s16));
    newWork[1] = 3;
}

void DestroyStageParticles(void)
{
    StageParticleWork *workP = stageParticleWork;
    HuSprAnimKill(workP->spriteAnimations[0]);
    HuSprAnimKill(workP->spriteAnimations[1]);
    HuSprAnimKill(workP->spriteAnimations[2]);
    HuSprAnimKill(workP->spriteAnimations[3]);
}

float stageParticleSizes[4] = { 1000, 200, 600, 1200 };

float stageParticleAlpha[4] = { 1, 0.6, 0.3, 0.425 };

void UpdateStageParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    s16 particleState = 0;
    StageParticleWork *workP = stageParticleWork;
    s16 *newWorkP = particle->work;
    s32 particleSlot = 0;
    Vec pos = Center;
    Vec rot = CRot;
    float particleDistance;
    float distanceRatio;
    float zoom = CZoom;
    HU3DPARTICLEDATA *particleData;
    Vec surfaceNormal;
    Vec viewDirection;
    Vec reflectedDirection;
    Vec emitterDirection;
    float inverseMagnitude;
    float particleSpacing;
    s32 i;
    switch (newWorkP[2]) {
        case 0:
            particleData = particle->data;
            if (newWorkP[1]) {
                emitterDirection.x = 2000;
                emitterDirection.y = -300;
                emitterDirection.z = 500;
                inverseMagnitude = 1 / VECMagPoint(emitterDirection.x, emitterDirection.y, emitterDirection.z);
                emitterDirection.x *= inverseMagnitude;
                emitterDirection.y *= inverseMagnitude;
                emitterDirection.z *= inverseMagnitude;
                particleSpacing = 3200.0f / particle->maxCnt;
                for (i = 0; i < particle->maxCnt; i++, particleData++) {
                    particleDistance = 500.0 + ((particleSpacing * i) + ((0.25f * particleSpacing) * ((float)((s32)frand() & 0xFFFF) * 0.0000152587890625)));
                    distanceRatio = particleDistance / 3200;
                    particleData->accel.x = particleDistance;
                    particleData->pos.x = 1000 + (particleDistance * emitterDirection.x);
                    particleData->pos.y = 4800 + (particleDistance * emitterDirection.y);
                    particleData->pos.z = -3500 + (particleDistance * emitterDirection.z);
                    particleData->scale = ((0.7f * (distanceRatio * distanceRatio)) + 0.3f)
                        * (stageParticleSizes[newWorkP[1]] * (0.7f + (0.3f * ((float)((s32)frand() & 0xFFFF) * 0.0000152587890625))));
                    particleData->color.r = (s32)(((float)((s32)frand() & 0xFFFF) * 0.0000152587890625) * 160.0) + 32;
                    particleData->color.g = (s32)(((float)((s32)frand() & 0xFFFF) * 0.0000152587890625) * 96.0) + 32;
                    particleData->color.b = (s32)(((float)((s32)frand() & 0xFFFF) * 0.0000152587890625) * 128.0) + 32;
                    particleData->color.a = (255 * stageParticleAlpha[newWorkP[1]] * (1.0 - (0.4 * distanceRatio)))
                        - (s32)(((float)((s32)frand() & 0xFFFF) * 0.0000152587890625) * 16.0);
                    particleData->accel.z = particleData->color.a;
                }
                newWorkP[2]++;
            }
            else {
                particleData->pos.x = 1000;
                particleData->pos.y = 4800;
                particleData->pos.z = -3500;
                particleData->scale = 1000;
            }
            break;

        case 1:
            surfaceNormal.x = sind(rot.y) * cosd(rot.x);
            surfaceNormal.y = -sind(rot.x);
            surfaceNormal.z = cosd(rot.y) * cosd(rot.x);
            viewDirection.x = 1000 - (pos.x + (1.25f * (surfaceNormal.x * zoom)));
            viewDirection.y = 4800 - (pos.y + (surfaceNormal.y * zoom));
            viewDirection.z = -3500 - (pos.z + (1.25f * (surfaceNormal.z * zoom)));
            inverseMagnitude = 1 / VECMagPoint(viewDirection.x, viewDirection.y, viewDirection.z);
            viewDirection.x *= inverseMagnitude;
            viewDirection.y *= inverseMagnitude;
            viewDirection.z *= inverseMagnitude;
            inverseMagnitude = -((-viewDirection.x * -surfaceNormal.x) + (-viewDirection.y * -surfaceNormal.y) + (-viewDirection.z * -surfaceNormal.z));
            reflectedDirection.x = -surfaceNormal.x + ((-2.0f * viewDirection.x) * inverseMagnitude);
            reflectedDirection.y = -surfaceNormal.y + ((-2.0f * viewDirection.y) * inverseMagnitude);
            reflectedDirection.z = -surfaceNormal.z + ((-2.0f * viewDirection.z) * inverseMagnitude);
            particleData = particle->data;
            for (i = 0; i < particle->maxCnt; i++, particleData++) {
                particleData->pos.x = 1000 + (reflectedDirection.x * particleData->accel.x);
                particleData->pos.y = 4800 + (reflectedDirection.y * particleData->accel.x);
                particleData->pos.z = -3500 + (reflectedDirection.z * particleData->accel.x);
                particleData->color.a = particleData->accel.z * workP->fadeParameters.y;
            }
            break;
    }
    DCFlushRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void SetStageParticleFade(float alpha)
{
    StageParticleWork *workP = stageParticleWork;
    workP->fadeParameters.y = alpha;
    if (workP->fadeParameters.y > 1.0f) {
        workP->fadeParameters.y = 1.0f;
    }
    if (workP->fadeParameters.y <= 0.0f) {
        Hu3DModelAttrSet(workP->particleModelIds[0], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(workP->particleModelIds[1], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(workP->particleModelIds[2], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(workP->particleModelIds[3], HU3D_ATTR_DISPOFF);
    }
    else {
        Hu3DModelAttrReset(workP->particleModelIds[0], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(workP->particleModelIds[1], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(workP->particleModelIds[2], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(workP->particleModelIds[3], HU3D_ATTR_DISPOFF);
    }
}

float InterpolateValue(float start, float end)
{
    float angleDiff;
    if (start >= 360.0f) {
        start -= 360.0f;
    }
    else if (start < 0.0f) {
        start += 360.0f;
    }
    if (end >= 360.0f) {
        end -= 360.0f;
    }
    else if (end < 0.0f) {
        end += 360.0f;
    }
    angleDiff = start - end;
    if (angleDiff <= -180.0f) {
        angleDiff += 360.0f;
    }
    else if (angleDiff >= 180.0f) {
        angleDiff -= 360.0f;
    }
    return angleDiff;
}

float ApproachValue(float start, float end, float max)
{
    float angle;
    float diff;

    if (start >= 360.0) {
        start -= 360.0;
    }
    else if (start < 0.0) {
        start += 360.0;
    }
    if (end >= 360.0) {
        end -= 360.0;
    }
    else if (end < 0.0) {
        end += 360.0;
    }
    diff = 360.0 + (start - end);
    if (fabs(diff) >= 360.0) {
        diff = fmod(diff, 360.0);
    }
    if (diff < 180.0) {
        if (diff <= max) {
            angle = diff;
        }
        else {
            angle = max;
        }
    }
    else if (360.0 - diff <= max) {
        angle = -(360.0 - diff);
    }
    else {
        angle = -max;
    }
    angle += end;
    if (angle >= 360.0) {
        angle -= 360.0;
    }
    else if (angle < 0.0) {
        angle += 360.0;
    }

    return angle;
}
