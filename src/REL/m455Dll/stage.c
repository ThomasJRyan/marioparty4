#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"


#include "game/frand.h"
#include "game/sprite.h"

#include "REL/m455Dll.h"

#include "ext_math.h"
#include "math.h"
#include "version.h"
#include <string.h>

HUPROCESS *stageObjectManager;
OMOBJ *stageEnvironmentObject;
OMOBJ *snowmanParticleSystemObject;
OMOBJ *snowmanModelObject;
OMOBJ *platformEffectObject;
OMOBJ *characterObject;
OMOBJ *characterParticleObject;
Vec reservedPosition;

typedef struct FallingParticleWork {
    ANIMDATA *particleAnimations[4];
    s32 particleModelIds[4];
    Vec particleAlpha;
} FallingParticleWork;

FallingParticleWork *fallingParticleState;

Vec platformModelPositions[4] = {
    { -460, 0, 150 },
    { 460, 0, 150 },
    { -460, 0, -150 },
    { 460, 0, -150 },
};

GXColor whiteColor = { 255, 255, 255, 255 };

typedef struct FramebufferConfig {
    s32 sourceWidth;
    s32 sourceHeight;
    s32 textureWidth;
    s32 textureHeight;
    BOOL useScaledCopy;
} FramebufferConfig;

FramebufferConfig framebufferConfigs[4] = {
    { 400, 400, 400, 400, FALSE },
    { 640, 480, 320, 240, TRUE },
    { 640, 480, 640, 480, FALSE },
    { 640, 480, 320, 240, TRUE },
};

void ParticlesCreate(void);

void SnowmanParticleSystemCreate(OMOBJ *object);
void SnowmanModelCreate(OMOBJ *object);
void PlatformEffectCreate(OMOBJ *object);
void CharacterSwarmCreate(OMOBJ *object);
void CharacterMoteCreate(OMOBJ *object);

OMOBJ *StageEnvironmentCreate(void);

void StageObjectsCreate(HUPROCESS *objectManager)
{
    stageObjectManager = objectManager;
    ParticlesCreate();
    snowmanParticleSystemObject = omAddObjEx(stageObjectManager, 50, 0, 0, -1, SnowmanParticleSystemCreate);
    snowmanModelObject = omAddObjEx(stageObjectManager, 50, 8, 0, -1, SnowmanModelCreate);
    platformEffectObject = omAddObjEx(stageObjectManager, 50, 1, 0, -1, PlatformEffectCreate);
    characterObject = omAddObjEx(stageObjectManager, 50, 0, 0, -1, CharacterSwarmCreate);
    characterParticleObject = omAddObjEx(stageObjectManager, 50, 0, 0, -1, CharacterMoteCreate);
    stageEnvironmentObject = StageEnvironmentCreate();
}

void StageSpritesDestroy(OMOBJ *object);

void SnowmanParticleSystemDestroy(OMOBJ *object);
void SnowmanModelSystemDestroy(OMOBJ *object);
void PlatformEffectDestroy(OMOBJ *object);
void CharacterSwarmDestroy(OMOBJ *object);

void ParticlesDestroy(void);

void StageObjectsDestroy(void)
{
    StageSpritesDestroy(stageEnvironmentObject);
    ParticlesDestroy();
    SnowmanParticleSystemDestroy(snowmanParticleSystemObject);
    SnowmanModelSystemDestroy(snowmanModelObject);
    PlatformEffectDestroy(platformEffectObject);
    CharacterSwarmDestroy(characterObject);
}

typedef struct FramebufferWork {
    void *textureBuffer;
    u32 textureBufferSize;
    BOOL useScaledCopy;
    s32 sourceWidth;
    s32 sourceHeight;
    s32 textureWidth;
    s32 textureHeight;
} FramebufferWork;

typedef struct SnowParticle {
    Vec position;
    Vec renderParameters;
    float lifetimeReciprocal;
    u16 lifetime;
    u16 remainingLifetime;
} SnowParticle;

typedef struct StageEnvironmentWork {
    s32 reservedState;
    FramebufferWork framebufferWork[4];
    SnowParticle snowParticles[64];
    float textureCoordinates[26][2];
    float baseTextureCoordinates[26][2];
    void *displayList;
    u32 displayListSize;
    ANIMDATA *spriteAnimations[9];
    float verticalOffset;
    float platformHeight;
    u32 frameCounter;
    u32 animationTimer;
    u32 platformAngle;
    float textureScrollU;
    float textureScrollV;
} Work347C;

void StageEnvironmentUpdate(OMOBJ *object);

void SnowfallRender(HU3DMODEL *model, Mtx matrix);
void StageOverlayRender(HU3DMODEL *model, Mtx matrix);
void StageRender(HU3DMODEL *model, Mtx matrix);

OMOBJ *StageEnvironmentCreate(void)
{
    OMOBJ *object = omAddObjEx(stageObjectManager, 127, 15, 0, -1, StageEnvironmentUpdate);
    Work347C *work = object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(Work347C), HU_MEMNUM_OVL);
    Work347C *environmentWork = work;
    SnowParticle *snowParticle = &work->snowParticles[0];
    FramebufferWork *framebufferWork;
    s32 resourceId;
    HU3DMODEL *modelData;
    s32 i;

    work->verticalOffset = 0;
    work->platformHeight = 0;
    work->frameCounter = 0;
    work->animationTimer = 0;
    work->platformAngle = 0;
    work->textureScrollU = 0;
    work->textureScrollV = 0;

    object->mdlId[0] = resourceId = Hu3DHookFuncCreate(StageRender);
    Hu3DModelCameraSet(resourceId, HU3D_CAM0);
    Hu3DModelLayerSet(resourceId, 5);
    modelData = &Hu3DData[resourceId];
    modelData->hookData = work;
    work->spriteAnimations[4] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 21), HU_MEMNUM_OVL));
    work->spriteAnimations[5] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 22), HU_MEMNUM_OVL));
    work->spriteAnimations[6] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 23), HU_MEMNUM_OVL));
    work->spriteAnimations[7] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 24), HU_MEMNUM_OVL));
    work->spriteAnimations[0] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 17), HU_MEMNUM_OVL));
    work->spriteAnimations[1] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 18), HU_MEMNUM_OVL));
    work->spriteAnimations[2] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 19), HU_MEMNUM_OVL));
    work->spriteAnimations[3] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 20), HU_MEMNUM_OVL));
    work->spriteAnimations[8] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 25), HU_MEMNUM_OVL));
    object->mdlId[1] = resourceId = Hu3DHookFuncCreate(SnowfallRender);
    Hu3DModelCameraSet(resourceId, HU3D_CAM0);
    Hu3DModelLayerSet(resourceId, 0);
    modelData = &Hu3DData[resourceId];
    modelData->hookData = work;
    environmentWork->displayListSize = 16384;
    environmentWork->displayList = HuMemDirectMallocNum(HEAP_HEAP, environmentWork->displayListSize, HU_MEMNUM_OVL);
    DCFlushRange(environmentWork->displayList, 16384);
    GXBeginDisplayList(environmentWork->displayList, environmentWork->displayListSize);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition2f32(-0.5f, -0.5f);
    GXTexCoord2f32(0, 0);
    GXPosition2f32(0.5f, -0.5f);
    GXTexCoord2f32(1, 0);
    GXPosition2f32(0.5f, 0.5f);
    GXTexCoord2f32(1, 1);
    GXPosition2f32(-0.5f, 0.5f);
    GXTexCoord2f32(0, 1);
    environmentWork->displayListSize = GXEndDisplayList();
    for(i=0; i<26; i++) {
        environmentWork->textureCoordinates[i][0] = 0;
        environmentWork->textureCoordinates[i][1] = 0;
        environmentWork->baseTextureCoordinates[i][0] = 0;
        environmentWork->baseTextureCoordinates[i][1] = 0;
    }

    object->mdlId[3] = resourceId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M455, 5));
    Hu3DModelPosSet(resourceId, 0, 0, -5000);
    Hu3DModelScaleSet(resourceId, 2, 2, 2);
    Hu3DModelCameraSet(resourceId, HU3D_CAM0);
    Hu3DModelLayerSet(resourceId, 1);
    resourceId = Hu3DTexScrollCreate(object->mdlId[3], "S3TCsk");
    Hu3DTexScrollPosMoveSet(resourceId, -0.0055f * REFRESH_FREQ, 0, 0);
    resourceId = Hu3DTexScrollCreate(object->mdlId[3], "pa_sk3");
    Hu3DTexScrollPosMoveSet(resourceId, -0.0125f * REFRESH_FREQ, 0, 0);

    object->mdlId[4] = resourceId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M455, 6));
    Hu3DModelPosSet(resourceId, 0, 0, -3000);
    Hu3DModelScaleSet(resourceId, 1.2f, 1.2f, 1.2f);
    Hu3DModelCameraSet(resourceId, HU3D_CAM0);
    Hu3DModelLayerSet(resourceId, 1);

    object->mdlId[8] = resourceId = Hu3DHookFuncCreate(StageOverlayRender);
    Hu3DModelCameraSet(resourceId, HU3D_CAM0);
    Hu3DModelLayerSet(resourceId, 2);
    modelData = &Hu3DData[resourceId];
    modelData->hookData = work;

    object->mdlId[10] = resourceId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M455, 0));
    Hu3DModelPosSet(resourceId, 0, 0, 0);
    Hu3DModelCameraSet(resourceId, HU3D_CAM0);
    Hu3DModelLayerSet(resourceId, 3);
    Hu3DModelShadowMapSet(object->mdlId[10]);
    for(i=0; i<4; i++) {

        object->mdlId[i+11] = resourceId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M455, 1));
        Hu3DModelCameraSet(object->mdlId[i+11], HU3D_CAM0);
        Hu3DModelLayerSet(object->mdlId[i+11], 3);
        Hu3DModelPosSet(object->mdlId[i+11], platformModelPositions[i].x, platformModelPositions[i].y, platformModelPositions[i].z);
        Hu3DModelAttrSet(object->mdlId[i+11], HU3D_MOTATTR_SHAPE_LOOP);
        Hu3DMotionShapeTimeSet(object->mdlId[i+11], (0.25*i)*Hu3DMotionShapeMaxTimeGet(object->mdlId[i+11]));
    }
    modelData = &Hu3DData[object->mdlId[0]];
    framebufferWork = &environmentWork->framebufferWork[0];
    for(i=0; i<4; i++, framebufferWork++) {
        framebufferWork->sourceWidth = framebufferConfigs[i].sourceWidth;
        framebufferWork->sourceHeight = framebufferConfigs[i].sourceHeight;
        framebufferWork->textureWidth = framebufferConfigs[i].textureWidth;
        framebufferWork->textureHeight = framebufferConfigs[i].textureHeight;
        framebufferWork->useScaledCopy = framebufferConfigs[i].useScaledCopy;
        framebufferWork->textureBufferSize = GXGetTexBufferSize(framebufferWork->textureWidth, framebufferWork->textureHeight, GX_TF_RGB565, GX_FALSE, GX_FALSE);
        framebufferWork->textureBuffer = HuMemDirectMallocNum(HEAP_MODEL, framebufferWork->textureBufferSize, modelData->mallocNo);
        memset(framebufferWork->textureBuffer, 0, framebufferWork->textureBufferSize);
        DCFlushRange(framebufferWork->textureBuffer, framebufferWork->textureBufferSize);
    }
    for(i=0; i<64; i++, snowParticle++) {
        snowParticle->position.x = snowParticle->position.y = snowParticle->position.z = 0;
        snowParticle->renderParameters.x = 0;
        snowParticle->renderParameters.y = 0;
        snowParticle->renderParameters.z = 1;
        snowParticle->lifetimeScale = REFRESH_FREQ;
        snowParticle->lifetime = REFRESH_RATE;
        snowParticle->remainingLifetime = 0;
    }
    return object;
}

void FallingParticleAlphaSet(float alpha);

void StageEnvironmentUpdate(OMOBJ *object)
{
    Work347C *work = object->data;
    Work347C *environmentWork = work;
    s32 i;
    if((work->platformAngle = work->platformAngle+(0.007874016f*((s32)frand() & 0x7F)*3.0f)) > 360) {
        work->platformAngle -= 360;
    }
    work->platformHeight = (25+work->verticalOffset)+(5.0*sind(work->platformAngle));
    Hu3DModelPosSet(object->mdlId[10], 0, work->platformHeight, 0);
    for(i=0; i<4; i++) {
        Hu3DModelPosSet(object->mdlId[i+11], platformModelPositions[i].x, work->platformHeight+platformModelPositions[i].y, platformModelPositions[i].z);
    }
    work->textureScrollU += 6.0f / (REFRESH_RATE_F * 100.0f);
    work->textureScrollV += 6.0f / (REFRESH_RATE_F * 100.0f);
    work->frameCounter++;
    work->animationTimer++;
    if(work->animationTimer >= REFRESH_RATE_F && work->animationTimer <= REFRESH_RATE_F * 1.35f) {
        float time = 1 - ((work->animationTimer - REFRESH_RATE_F) / (REFRESH_RATE_F * 0.35f));
        if(time < 0.0f) {
            time = 0.0f;
        }
        FallingParticleAlphaSet(time);
    }
}

void StageSpritesDestroy(OMOBJ *object)
{
    Work347C *work = object->data;
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

GXColor snowParticleColor = { 255, 255, 255, 32 };
GXColor snowHighlightColor = { 255, 255, 255, 10 };
GXColor platformMaterialColor = { 0, 126, 126, 255 };
Vec framebufferQuadVertices[4] = {
    -1000, 50, 1000,
    -1000, 50, -1000,
    1000, 50, -1000,
    1000, 50, 1000
};

Vec platformVertices[26] = {
    -1500, 0, 1000,
    -1400, 0, 1000,
    0, 0, 1000,
    1400, 0, 1000,
    1500, 0, 1000,
    -1500, 0, -500,
    -1400, 0, -500,
    0, 0, -500,
    1400, 0, -500,
    1500, 0, -500,
    -1500, 0, -2000,
    -1400, 0, -2000,
    0, 0, -2000,
    1400, 0, -2000,
    1500, 0, -2000,
    -1500, 0, -5000,
    -1400, 0, -5000,
    0, 0, -5000,
    1400, 0, -5000,
    1500, 0, -5000,
    -2000, 0, -500,
    -2500, 0, -2000,
    -3000, 0, -5000,
    2000, 0, -500,
    2500, 0, -2000,
    3000, 0, -5000
};

GXColor platformVertexColors[26] = {
    { 96, 128, 255, 255 },
    { 128, 160, 255, 32 },
    { 128, 160, 255, 32 },
    { 128, 160, 255, 32 },
    { 96, 128, 255, 255 },
    { 24, 48, 255, 255 },
    { 96, 128, 255, 32 },
    { 128, 160, 255, 32 },
    { 96, 128, 255, 32 },
    { 24, 48, 255, 255 },
    { 24, 48, 255, 255 },
    { 24, 48, 255, 255 },
    { 96, 128, 255, 255 },
    { 24, 48, 255, 255 },
    { 24, 48, 255, 255 },
    { 8, 24, 143, 255 },
    { 8, 24, 143, 255 },
    { 8, 24, 143, 255 },
    { 8, 24, 143, 255 },
    { 8, 24, 143, 255 },
    { 24, 48, 255, 255 },
    { 24, 48, 255, 255 },
    { 24, 48, 255, 255 },
    { 24, 48, 255, 255 },
    { 24, 48, 255, 255 },
    { 24, 48, 255, 255 }
};

float platformTextureCoordinates[26][2] = {
    0, 1,
    0.033333335, 1,
    0.5, 1,
    0.96666664, 1,
    1, 1,
    0, 0.5,
    0.033333335, 0.5,
    0.5, 0.5,
    0.96666664, 0.5,
    1, 0.5,
    0, 0,
    0.033333335, 0,
    0.5, 0,
    0.96666664, 0,
    1, 0,
    0, 0,
    0.033333335, 0,
    0.5, 0,
    0.96666664, 0,
    1, 0,
    -0.15, 0.5,
    -0.33, 0,
    -0.49, 0,
    1.15, 0.5,
    1.33, 0,
    1.49, 0
};

float textureScrollOffsets[26][2] = {
    0, 30,
    0.5, 30,
    7.5, 30,
    14.5, 30,
    15, 30,
    0, 15,
    0.5, 15,
    7.5, 15,
    14.5, 15,
    15, 15,
    0, 7.5,
    0.5, 7.5,
    7.5, 7.5,
    14.5, 7.5,
    15, 7.5,
    0, 0,
    0.5, 0,
    7.5, 0,
    14.5, 0,
    15, 0,
    -2.25, 15,
    -4.9500003, 7.5,
    -7.3500004, 0,
    17.25, 15,
    19.95, 7.5,
    22.35, 0
};

float indirectTextureMatrix[2][3] = {
    0, 0, 0.5,
    0, 0.5, 0
};

GXColor shadowDirection0 = { 80, 80, 80, 80 };

Vec groundVertices[9] = {
    -1500,  0, -5000,
    0,  0,  -5000,
    1500,  0,  -5000,
    -1500,  0,  -3000,
    0,  0,  -3000,
    1500,  0,  -3000,
    -1500,  0,  1000,
    0,  0,  1000,
    1500,  0,  1000
};

GXColor groundVertexColors[9] = {
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

GXColor groundHighlightColor = { 255, 165, 89, 178 };
GXColor groundShadowColor = { 153, 114, 89, 178 };
GXColor framebufferClearColor = { 0, 0, 0, 255 };

void FramebufferCapture(FramebufferWork *framebuffer, s32 clearAfterCopy);

void SnowfallRender(HU3DMODEL *model, Mtx matrix)
{
    Work347C *work = model->hookData;
    Work347C *environmentWork = work;
    SnowParticle *snowParticle = &environmentWork->snowParticles[0];
    FramebufferWork *framebufferWork = &environmentWork->framebufferWork[0];
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
    GXSetChanMatColor(GX_COLOR0A0, platformMaterialColor);
    GXSetTevColor(GX_TEVREG0, snowHighlightColor);
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
    GXTexCoord2f32(work->textureScrollU, work->textureScrollV);
    GXPosition2u16(400, 0);
    GXTexCoord2f32(work->textureScrollU+7, work->textureScrollV);
    GXPosition2u16(400, 400);
    GXTexCoord2f32(work->textureScrollU+7, work->textureScrollV+7);
    GXPosition2u16(0, 400);
    GXTexCoord2f32(work->textureScrollU, work->textureScrollV+7);
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
    snowParticle = &environmentWork->snowParticles[0];
    for(i=0; i<64; i++, snowParticle++) {
        float scale;
        float accel;
        if(snowParticle->remainingLifetime == 0) {
            continue;
        }
        scale = 128*(snowParticle->renderParameters.x*(0.2f+cosd(snowParticle->remainingLifetime*snowParticle->lifetimeScale*90.0f)));
        accel = snowParticle->remainingLifetime*snowParticle->lifetimeScale;
        snowParticleColor.a = snowParticle->renderParameters.z*sind((accel*accel)*90.0f)*255.0;
        GXSetChanMatColor(GX_COLOR0A0, snowParticleColor);
        MTXScale(modelview, 0.5f*scale, 0.5f*scale, 1.0f);
        mtxRotCat(modelview, 0, 0, snowParticle->renderParameters.y);
        mtxTransCat(modelview, 400*(snowParticle->position.x+1500)/3000, 400*(snowParticle->position.z - -2000)/3000, 0);
        GXLoadPosMtxImm(modelview, GX_PNMTX0);
        GXCallDisplayList(environmentWork->displayList, environmentWork->displayListSize);
        if(!omPauseChk()) {
            snowParticle->remainingLifetime--;
        }
    }
    framebufferWork = &environmentWork->framebufferWork[0];
    FramebufferCapture(framebufferWork, 1);
}

void GroundRender(HU3DMODEL *model, Mtx matrix);

void StageOverlayRender(HU3DMODEL *model, Mtx matrix)
{
    Work347C *work;
    Work347C *environmentWork;
    FramebufferWork *framebufferWork;
    GroundRender(model, matrix);
    work = model->hookData;
    environmentWork = work;
    framebufferWork = &environmentWork->framebufferWork[1];
    FramebufferCapture(framebufferWork, 0);
}

void FramebufferComposite(FramebufferWork *framebuffer, float x, float y, float scale);

void BackdropRender(HU3DMODEL *model, Mtx matrix)
{
    Work347C *work = model->hookData;
    Work347C *environmentWork = work;
    FramebufferWork *framebufferWork = &environmentWork->framebufferWork[0];

    FramebufferComposite(framebufferWork, 200, 0, 0.5f);
    framebufferWork = &environmentWork->framebufferWork[1];
    FramebufferComposite(framebufferWork, 0, 0, 0.5f);
    framebufferWork = &environmentWork->framebufferWork[2];
    FramebufferComposite(framebufferWork, 0, 150, 0.25f);
    framebufferWork = &environmentWork->framebufferWork[3];
    FramebufferComposite(framebufferWork, 0, 300, 0.5f);
}

void FramebufferTextureBind(s16 textureMapId, FramebufferWork *framebuffer, GXTexWrapMode wrapMode, s32 filterMode, BOOL useMipmaps);

void PlatformRender(HU3DMODEL *model, Mtx matrix)
{
    HU3DCAMERA *cameraP = &Hu3DCamera[Hu3DCameraNo];
    Work347C *work = model->hookData;
    Work347C *environmentWork = work;
    FramebufferWork *framebufferWork = &environmentWork->framebufferWork[2];
    float fov;
    Mtx proj;
    Mtx texMtx;
    Mtx projCamera;
    s16 lightMask;
    s16 i;

    FramebufferCapture(framebufferWork, 0);
    GXLoadPosMtxImm(matrix, GX_PNMTX0);
    fov = cameraP->fov;
    if(fov <= 0.0f) {
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
    GXSetChanMatColor(GX_COLOR0A0, whiteColor);
    GXSetZMode(GX_TRUE, GX_GREATER, GX_FALSE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    framebufferWork = &environmentWork->framebufferWork[1];
    FramebufferTextureBind(0, framebufferWork, 0, 0, 0);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetArray(GX_VA_POS, framebufferQuadVertices, sizeof(Vec));
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    for(i=0; i<4; i++) {
        GXPosition1x16(i);
    }
    GXEnd();
    framebufferWork = &environmentWork->framebufferWork[3];
    FramebufferCapture(framebufferWork, 0);
    framebufferWork = &environmentWork->framebufferWork[2];
    FramebufferTextureBind(0, framebufferWork, 0, 0, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    for(i=0; i<4; i++) {
        GXPosition1x16(i);
    }
    GXEnd();
}

void StageRender(HU3DMODEL *model, Mtx matrix)
{
    HU3DCAMERA *cameraP = &Hu3DCamera[Hu3DCameraNo];
    Work347C *work = model->hookData;
    Work347C *environmentWork = work;
    FramebufferWork *framebufferWork = &environmentWork->framebufferWork[2];
    Mtx invXPose;
    Mtx proj;
    Mtx texMtx;
    Mtx projCamera;
    float textureScrollScale;
    float textureScrollOffset;
    float fov;
    s16 i;
    PlatformRender(model, matrix);
    textureScrollScale = 1.5f;
    textureScrollOffset = 0;
    GXLoadPosMtxImm(matrix, GX_PNMTX0);
    MTXInvXpose(matrix, invXPose);
    GXLoadNrmMtxImm(invXPose, GX_PNMTX0);
    fov = cameraP->fov;
    if(fov <= 0.0f) {
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
    GXSetTevColor(GX_TEVREG0, shadowDirection0);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C0, GX_CC_TEXA, GX_CC_CPREV);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_VTX, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanMatColor(GX_COLOR0A0, whiteColor);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
    framebufferWork = &environmentWork->framebufferWork[3];
    FramebufferTextureBind(0, framebufferWork, 0, 0, 0);
    HuSprTexLoad(work->spriteAnimations[3], 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    framebufferWork = &environmentWork->framebufferWork[0];
    FramebufferTextureBind(2, framebufferWork, 0, 0, 1);
    GXSetTexCoordGen(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetNumIndStages(1);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD2, GX_TEXMAP2);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
    GXSetTevIndWarp(GX_TEVSTAGE0, GX_INDTEXSTAGE0, GX_TRUE, GX_FALSE, GX_ITM_1);
    GXSetIndTexMtx(GX_ITM_1, indirectTextureMatrix, -1);
    for(i=0; i<26; i++) {
        environmentWork->baseTextureCoordinates[i][0] -= 0.007874016f*((s32)frand() & 0x7F)*0.005f;
        environmentWork->baseTextureCoordinates[i][1] += 0.007874016f*((s32)frand() & 0x7F)*0.005f;
        environmentWork->textureCoordinates[i][0] = environmentWork->baseTextureCoordinates[i][0]+textureScrollOffsets[i][0];
        environmentWork->textureCoordinates[i][1] = environmentWork->baseTextureCoordinates[i][1]+textureScrollOffsets[i][1];
    }
    DCFlushRange(&environmentWork->textureCoordinates[0][0], sizeof(environmentWork->textureCoordinates));
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);

    GXSetArray(GX_VA_POS, platformVertices, sizeof(Vec));
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, platformVertexColors, sizeof(GXColor));
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_TEX0, platformTextureCoordinates, sizeof(float)*2);
    GXSetVtxDesc(GX_VA_TEX1, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_TEX1, &environmentWork->textureCoordinates[0][0], sizeof(float)*2);
    GXBegin(GX_QUADS, GX_VTXFMT0, 48);
    for(i=0; i<12; i++) {
        s16 index;
        if(i == 4) {
            index = 12;
        } else if(i == 9) {
            index = 13;
        } else {
            index = i;
        }
        GXPosition1x16(index+0);
        GXColor1x16(index+0);
        GXTexCoord1x16(index+0);
        GXTexCoord1x16(index+0);
        GXPosition1x16(index+5);
        GXColor1x16(index+5);
        GXTexCoord1x16(index+5);
        GXTexCoord1x16(index+5);
        GXPosition1x16(index+6);
        GXColor1x16(index+6);
        GXTexCoord1x16(index+6);
        GXTexCoord1x16(index+6);
        GXPosition1x16(index+1);
        GXColor1x16(index+1);
        GXTexCoord1x16(index+1);
        GXTexCoord1x16(index+1);
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

void GroundRender(HU3DMODEL *model, Mtx matrix)
{
    s32 groundVertexCount = 0;
    Work347C *work = model->hookData;
    float scale = 1.5f;
    float x = 0;
    Mtx textureMatrix;
    Mtx final;
    MTXTrans(final, x, -300, 0);
    GXSetTevColor(GX_TEVREG0, groundHighlightColor);
    MTXScale(textureMatrix, scale, 1, 1);
    MTXConcat(final, textureMatrix, final);
    MTXConcat(Hu3DCameraMtx, final, final);
    GXLoadPosMtxImm(final, GX_PNMTX0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanMatColor(GX_COLOR0A0, whiteColor);
    HuSprTexLoad(work->spriteAnimations[0], 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    HuSprTexLoad(work->spriteAnimations[1], 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetNumTexGens(2);
    MTXRotDeg(final, 'X', 90.0f);
    x = 0.004f*scale;
    MTXScale(textureMatrix, x, -0.004f, 0.004f);
    MTXConcat(final, textureMatrix, final);
    MTXTrans(textureMatrix, (work->frameCounter%1024)/(512.0f*x), 0, 1.2499999f*(work->frameCounter%400));
    MTXConcat(final, textureMatrix, final);
    GXLoadTexMtxImm(final, GX_TEXMTX0, GX_MTX2x4);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_POS, GX_TEXMTX0);
    MTXRotDeg(final, 'X', 90.0f);
    x = 0.004f*scale;
    MTXScale(textureMatrix, x, -0.004f, 0.004f);
    MTXConcat(final, textureMatrix, final);
    MTXTrans(textureMatrix, (work->frameCounter%300)/(150.0f*x), 0, 0);
    MTXConcat(final, textureMatrix, final);
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
    GXSetArray(GX_VA_POS, groundVertices, sizeof(Vec));
    GXSetArray(GX_VA_CLR0, groundVertexColors, sizeof(GXColor));
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

void FramebufferCapture(FramebufferWork *framebuffer, s32 clearAfterCopy)
{
    GXSetTexCopySrc(0, 0, framebuffer->sourceWidth, framebuffer->sourceHeight);
    GXSetTexCopyDst(framebuffer->textureWidth, framebuffer->textureHeight, GX_TF_RGB565, framebuffer->useScaledCopy);
    GXSetCopyClear(framebufferClearColor, GX_MAX_Z24);
    GXCopyTex(framebuffer->textureBuffer, clearAfterCopy);

}

void FramebufferComposite(FramebufferWork *fbWork, float left, float top, float scale)
{
    float x2 = left+(fbWork->textureWidth*scale);
    float y2 = top+(fbWork->textureHeight*scale);
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
    GXPosition3f32(left, top, 0);
    GXTexCoord2f32(0, 0);
    GXPosition3f32(x2, top, 0);
    GXTexCoord2f32(1, 0);
    GXPosition3f32(x2, y2, 0);
    GXTexCoord2f32(1, 1);
    GXPosition3f32(left, y2, 0);
    GXTexCoord2f32(0, 1);
    GXEnd();
}

void FramebufferTextureBind(s16 textureMapId, FramebufferWork *framebuffer, GXTexWrapMode wrapMode, s32 filterMode, BOOL useMipmaps)
{
    GXTexObj texObj;
    GXInitTexObj(&texObj, framebuffer->textureBuffer, framebuffer->textureWidth, framebuffer->textureHeight, GX_TF_RGB565, wrapMode, wrapMode, FALSE);
    if(useMipmaps) {
        GXInitTexObjLOD(&texObj, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    } else {
        GXInitTexObjLOD(&texObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    }
    GXLoadTexObj(&texObj, textureMapId);
}

s32 SnowParticleSpawn(Vec *position, float horizontalSpeed, float depthSpeed, s32 lifetime)
{
    Work347C *work;
    Work347C *environmentWork;
    SnowParticle *snowParticle;
    s32 i;
    if(!stageEnvironmentObject) {
        return -1;
    }
    work = stageEnvironmentObject->data;
    environmentWork = work;
    snowParticle = &work->snowParticles[0];
    for(i = 0; i < 64; i++, snowParticle++) {
        if (snowParticle->remainingLifetime == 0) {
            break;
        }
    }
    if(i >= 64) {
        return -1;
    }
    snowParticle->position = *position;
    if(horizontalSpeed < 0.0f) {
        snowParticle->renderParameters.x = 1;
    } else {
        snowParticle->renderParameters.x = horizontalSpeed;
    }
    if(depthSpeed < 0.0f) {
        snowParticle->renderParameters.z = 1;
    } else {
        snowParticle->renderParameters.z = depthSpeed;
    }
    if(lifetime < 0) {
        snowParticle->lifetime = REFRESH_RATE;
    } else {
        snowParticle->lifetime = lifetime;
    }
    snowParticle->renderParameters.y = (s32)frand() % 360;
    snowParticle->remainingLifetime = snowParticle->lifetime;
    snowParticle->lifetimeReciprocal = 1.0f / snowParticle->lifetime;
    return i;
}

float PlatformHeightGet(void)
{
    if(!stageEnvironmentObject) {
        return 0;
    } else {
        Work347C *work = stageEnvironmentObject->data;
        return work->platformHeight;
    }
}

float VerticalOffsetGet(void)
{
    if(!stageEnvironmentObject) {
        return 0;
    } else {
        Work347C *work = stageEnvironmentObject->data;
        return work->verticalOffset;
    }
}

typedef struct SnowmanWork {
    s32 particleIndex;
    s32 particleModelId;
    s32 isActive;
    s32 age;
    ANIMDATA *animation;
    Vec position;
    Vec rotation;
    Vec scale;
} SnowmanParticleWork;

void SnowmanParticleSystemUpdate(OMOBJ *object);

void SnowmanParticleSystemCreate(OMOBJ *object)
{
    SnowmanParticleWork *work;
    s32 i;
    ANIMDATA *anim;
    work = object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(SnowmanParticleWork)*8, HU_MEMNUM_OVL);
    memset(work, 0, sizeof(SnowmanParticleWork)*8);
    anim = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 26), HU_MEMNUM_OVL));
    for(i=0; i<8; i++, work++) {
        work->animation = anim;
        work->particleIndex = i;
        work->particleModelId = Hu3DParticleCreate(work->animation, 32);
        Hu3DModelCameraSet(work->particleModelId, HU3D_CAM0);
        Hu3DModelLayerSet(work->particleModelId, 6);
        Hu3DModelAttrSet(work->particleModelId, HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(work->particleModelId, HU3D_ATTR_PARTICLE_KILL);
        work->isActive = 0;
        work->age = 0;
        work->position.x = work->position.y = work->position.z = 0;
        work->rotation.x = work->rotation.y = work->rotation.z = 0;
        work->scale.x = work->scale.y = work->scale.z = 1;
    }
    object->objFunc = SnowmanParticleSystemUpdate;
}

void SnowmanParticleSystemUpdate(OMOBJ *object)
{
    SnowmanParticleWork *work = object->data;
    s32 i;
    for(i=0; i<8; i++, work++) {
        if(work->isActive == 0) {
            continue;
        }
        if(work->age > 1) {
            HU3DMODEL *modelData = &Hu3DData[work->particleModelId];
            HU3DPARTICLE *particleP = modelData->hookData;
            HU3DPARTICLEDATA *data = particleP->data;
            s32 j;
            for(j=0; j<particleP->maxCnt; j++, data++) {
                if(++data->time > data->parManId) {
                    s32 alpha = data->color.a-1;
                    if(alpha < 0) {
                        data->scale = 0;
                    } else {
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
        if(++work->age > REFRESH_RATE_F * 1.5f) {
            Hu3DModelAttrSet(work->particleModelId, HU3D_ATTR_DISPOFF);
            work->isActive = 0;
        } else {
            Hu3DModelPosSet(work->particleModelId, work->position.x, work->position.y, work->position.z);
            Hu3DModelRotSet(work->particleModelId, work->rotation.x, work->rotation.y, work->rotation.z);
            Hu3DModelScaleSet(work->particleModelId, work->scale.x, work->scale.y, work->scale.z);
        }
    }
}

void SnowmanParticleSystemDestroy(OMOBJ *object)
{
    SnowmanParticleWork *work = object->data;
    ANIMDATA *anim = work->animation;
    anim->useNum = 0;
    HuSprAnimKill(anim);
}

s32 SnowmanParticleSpawn(Vec *position, float scale)
{
    SnowmanParticleWork *work = snowmanParticleSystemObject->data;
    s32 i;
    HU3DMODEL *snowmanModel;
    HU3DPARTICLE *snowmanParticles;
    HU3DPARTICLEDATA *snowmanParticle;

    for(i=0; i<8; i++, work++) {
        if(work->isActive == 0) {
            break;
        }
    }
    if(i >= 8) {
        return -1;
    }
    Hu3DModelAttrReset(work->particleModelId, HU3D_ATTR_DISPOFF);
    work->isActive = 1;
    work->age = 0;
    snowmanModel = &Hu3DData[work->particleModelId];
    snowmanParticles = snowmanModel->hookData;
    snowmanParticles->blendMode = 1;
    snowmanParticle = snowmanParticles->data;
    for(i=0; i<snowmanParticles->maxCnt; i++, snowmanParticle++) {
        float elevationAngle = ((s32)frand()%20)+90;
        float directionAngle = (s32)frand()%360;
        float elevationSine = sind(elevationAngle);
        float elevationCosine = cosd(elevationAngle);
        float speedVariation = 0.75f+0.15f*(0.007874016f*((s32)frand() & 0x7F));
        float particleSpeed = 0.15f;
        snowmanParticle->vel.x = particleSpeed*(0.65f*(50.0*(elevationCosine*sind(directionAngle))));
        snowmanParticle->vel.z = particleSpeed*(0.65f*(50.0*(elevationCosine*cosd(directionAngle))));
        snowmanParticle->vel.y = particleSpeed*(50*elevationSine*speedVariation);
        snowmanParticle->time = 0;
        snowmanParticle->parManId = ((s32)frand()%16)+45;
        snowmanParticle->zRot = (s32)frand()%360;
        snowmanParticle->scale = 145;
        snowmanParticle->pos.x = 0.05f*(((s32)frand() & 0x7F)-64);
        snowmanParticle->pos.y = 0.65f*(((s32)frand() & 0x7F)-64);
        snowmanParticle->pos.z = 0.05f*(((s32)frand() & 0x7F)-64);
        snowmanParticle->color.r = ((s32)frand()%64)+192;
        snowmanParticle->color.g = ((s32)frand()%64)+192;
        snowmanParticle->color.b = ((s32)frand()%64)+192;
        snowmanParticle->color.a = 64;
    }
    if(scale < 0.0f) {
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

void SnowmanModelUpdate(OMOBJ *object);

void SnowmanModelCreate(OMOBJ *object)
{
    s32 i;
    for(i=0; i<object->mdlcnt; i++) {
        if(i == 0) {
            object->mdlId[i] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M455, 9));
        } else {
            object->mdlId[i] = Hu3DModelLink(object->mdlId[0]);
        }
        Hu3DModelCameraSet(object->mdlId[i], HU3D_CAM0);
        Hu3DModelLayerSet(object->mdlId[i], 6);
        Hu3DModelAttrSet(object->mdlId[i], HU3D_ATTR_DISPOFF);

    }
    object->stat |= 0x100;
    object->objFunc = SnowmanModelUpdate;
}

void SnowmanModelUpdate(OMOBJ *object)
{
    s32 i;
    for(i=0; i<object->mdlcnt; i++) {
        HU3DMODEL *modelData = &Hu3DData[object->mdlId[i]];
        if(!(modelData->attr & HU3D_ATTR_DISPOFF)) {
           if(Hu3DMotionTimeGet(object->mdlId[i]) >= Hu3DMotionMaxTimeGet(object->mdlId[i])) {
               Hu3DModelAttrSet(object->mdlId[i], HU3D_ATTR_DISPOFF);
           }
        }
    }
}

void SnowmanModelSystemDestroy(OMOBJ *object)
{

}

s32 SnowmanModelSpawn(Vec *position, float scale)
{
    s32 i;
    for(i=0; i<snowmanModelObject->mdlcnt; i++) {
        HU3DMODEL *modelData = &Hu3DData[snowmanModelObject->mdlId[i]];
        if(modelData->attr & HU3D_ATTR_DISPOFF) {
            break;
        }
    }
    if(i >= snowmanModelObject->mdlcnt) {
        return -1;
    }
    Hu3DMotionTimeSet(snowmanModelObject->mdlId[i], 0);
    Hu3DMotionSpeedSet(snowmanModelObject->mdlId[i], 1);
    Hu3DModelPosSet(snowmanModelObject->mdlId[i], position->x, position->y, position->z);
    Hu3DModelScaleSet(snowmanModelObject->mdlId[i], scale, scale, scale);
    Hu3DModelAttrReset(snowmanModelObject->mdlId[i], HU3D_ATTR_DISPOFF);
    return i;
}

void PlatformEffectUpdate(OMOBJ *object);

void PlatformEffectCreate(OMOBJ *object)
{
    HU3DPARTICLE *platformParticles;
    HU3DPARTICLEDATA *platformParticle;
    s32 i;
    HU3DMODEL *modelData;
    ANIMDATA *anim = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 26), HU_MEMNUM_OVL));

    object->data = anim;
    object->mdlId[0] = Hu3DParticleCreate(anim, 64);
    Hu3DModelCameraSet(object->mdlId[0], HU3D_CAM0);
    Hu3DModelLayerSet(object->mdlId[0], 3);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_PARTICLE_KILL);
    modelData = &Hu3DData[object->mdlId[0]];
    platformParticles = modelData->hookData;
    platformParticle = platformParticles->data;
    platformParticles->blendMode = 1;
    for(i=0; i<platformParticles->maxCnt; i++, platformParticle++) {
        platformParticle->scale = 0;
    }
    object->objFunc = PlatformEffectUpdate;
}

void PlatformEffectUpdate(OMOBJ *object)
{
    HU3DMODEL *modelData = &Hu3DData[object->mdlId[0]];
    HU3DPARTICLE *platformParticles = modelData->hookData;
    HU3DPARTICLEDATA *platformParticle = platformParticles->data;
    s32 i;
    for(i=0; i<platformParticles->maxCnt; i++, platformParticle++) {
        if(platformParticle->scale <= 0.0f) {
            continue;
        }
        if(++platformParticle->time > 360) {
            platformParticle->time -= 360;
        }
        platformParticle->pos.x += platformParticle->vel.x*sind(2.0f*platformParticle->time);
        platformParticle->pos.y += platformParticle->vel.y;
        if(platformParticle->pos.y >= 0.0f) {
            SnowParticleSpawn(&platformParticle->pos, 0.00625f*platformParticle->scale, 1, REFRESH_RATE / 4);
            platformParticle->scale = 0;
        }

    }
}

void PlatformEffectDestroy(OMOBJ *object)
{
    ANIMDATA *anim = object->data;
    HuSprAnimKill(anim);
    Hu3DModelKill(object->mdlId[0]);
    object->mdlId[0] = -1;
}

s32 PlatformParticleSpawn(Vec *pos, float scale)
{
    s32 i;
    HU3DMODEL *modelData = &Hu3DData[platformEffectObject->mdlId[0]];
    HU3DPARTICLE *platformParticles = modelData->hookData;
    HU3DPARTICLEDATA *platformParticle = platformParticles->data;
    for(i=0; i<platformParticles->maxCnt; i++, platformParticle++) {
        if(platformParticle->scale <= 0.0f) {
            break;
        }
    }
    if(i >= platformParticles->maxCnt) {
        return -1;
    }
    if(scale <= 0.0f) {
        scale = 1.0f;
    }
    platformParticle->pos = *pos;
#if VERSION_PAL
    platformParticle->vel.x = 1.2f * (1.5f+(0.2f*(0.007874016f*((s32)frand() & 0x7F)))) * 0.5f;
    platformParticle->vel.z = 1.2f * (1.5f+(0.2f*(0.007874016f*((s32)frand() & 0x7F))));
    platformParticle->vel.y = 1.2f * (1.5f+(0.2f*(0.007874016f*((s32)frand() & 0x7F)))) * 2.0f;
#else
    platformParticle->vel.x = (1.5f+(0.2f*(0.007874016f*((s32)frand() & 0x7F))))*0.5f;
    platformParticle->vel.z = (1.5f+(0.2f*(0.007874016f*((s32)frand() & 0x7F))));
    platformParticle->vel.y = (1.5f+(0.2f*(0.007874016f*((s32)frand() & 0x7F))))*2.0f;
#endif
    platformParticle->time = (s32)frand()%360;
    platformParticle->scale = 20*scale;
    return i;
}


typedef struct CharacterUpdateWork {
    s32 modelId;
    s32 isBoundaryDrifter;
    s32 phase;
    float speed;
    float heading;
    Vec position;
    Vec targetPosition;
} CharacterSwarmWork;

void CharacterSwarmUpdate(OMOBJ *object);

void CharacterSwarmCreate(OMOBJ *object)
{
    CharacterSwarmWork *baseWork = object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(CharacterSwarmWork)*8, HU_MEMNUM_OVL);
    CharacterSwarmWork *characterWork = baseWork;
    s32 i;
    memset(characterWork, 0, 8*sizeof(CharacterSwarmWork));
    for(i=0; i<8; i++, characterWork++) {
        switch(i) {
            case 0:
                characterWork->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M455, 10));
                break;

            case 1:
                characterWork->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M455, 10));
                break;

            default:
                if(i & 0x1) {
                    characterWork->modelId = Hu3DModelLink(baseWork[0].modelId);
                } else {
                    characterWork->modelId = Hu3DModelLink(baseWork[1].modelId);
                }
                break;
        }
        Hu3DModelScaleSet(characterWork->modelId, 0.5f, 0.5f, 0.5f);
        Hu3DModelCameraSet(characterWork->modelId, HU3D_CAM0);
        Hu3DModelLayerSet(characterWork->modelId, 1);
        characterWork->isBoundaryDrifter = 0;
        characterWork->phase = 0;
        characterWork->speed = 2;
        characterWork->heading = (s32)frand() % 360;
        characterWork->position.x = 1500*((0.007874016f*((s32)frand() & 0x7F))-0.5f);
        characterWork->position.z = 500*((0.007874016f*((s32)frand() & 0x7F))-0.5f);
        characterWork->position.y = -250+(100*(0.3f*i));
        characterWork->targetPosition.x = 1500*((0.007874016f*((s32)frand() & 0x7F))-0.5f);
        characterWork->targetPosition.z = 500*((0.007874016f*((s32)frand() & 0x7F))-0.5f);
        characterWork->targetPosition.y = characterWork->position.y;
        Hu3DModelPosSet(characterWork->modelId, characterWork->position.x, characterWork->position.y, characterWork->position.z);
        Hu3DModelRotSet(characterWork->modelId, 0, characterWork->heading, 0);
    }
    object->objFunc = CharacterSwarmUpdate;
}

float AngleWeightedInterpolate(float start, float end, float weight);

void CharacterSwarmUpdate(OMOBJ *object)
{
    CharacterSwarmWork *characterWork = object->data;
    s32 i;
    for(i=0; i<8; i++, characterWork++) {
        float dx;
        float dz;
        float vel;
        float angle;
        if(characterWork->isBoundaryDrifter == 0) {
            vel = 1+(0.5*sind((characterWork->phase*5)%180));
        } else {
            vel = 3;
        }
#if VERSION_PAL
        characterWork->position.x += vel * (characterWork->speed * sind(characterWork->heading)) * 1.2f;
        characterWork->position.z += vel * (characterWork->speed * cosd(characterWork->heading)) * 1.2f;
#else
        characterWork->position.x += vel*(characterWork->speed*sind(characterWork->heading));
        characterWork->position.z += vel*(characterWork->speed*cosd(characterWork->heading));
#endif
        dx = characterWork->targetPosition.x-characterWork->position.x;
        dz = characterWork->targetPosition.z-characterWork->position.z;
        angle = atan2d(dx, dz);
        characterWork->heading = AngleWeightedInterpolate(angle, characterWork->heading, 3.5f);
        if((dx*dx)+(dz*dz) < 10000.0f) {
            if(characterWork->isBoundaryDrifter == 0) {
                characterWork->targetPosition.x = 2000*((0.007874016f*((s32)frand() & 0x7F))-0.5f);
                characterWork->targetPosition.z = 700*((0.007874016f*((s32)frand() & 0x7F))-0.5f);
                characterWork->targetPosition.y = characterWork->position.y;
            } else {
                if(characterWork->position.x < 0.0f) {
                    characterWork->targetPosition.x = -2500;
                } else {
                    characterWork->targetPosition.x = 2500;
                }
                characterWork->targetPosition.z = 500*((0.007874016f*((s32)frand() & 0x7F))-0.5f);
                characterWork->targetPosition.y = characterWork->position.y;
            }
        }
        Hu3DModelPosSet(characterWork->modelId, characterWork->position.x, characterWork->position.y, characterWork->position.z);
        Hu3DModelRotSet(characterWork->modelId, 0, characterWork->heading, 0);
        characterWork->phase += frand()&0x3;
    }
}

void CharacterSwarmDestroy(OMOBJ *object)
{

}

void CharacterSwarmBoundaryDriftEnable(void)
{
    CharacterSwarmWork *characterWork = characterObject->data;
    s32 i;
    for(i=0; i<8; i++, characterWork++) {
        characterWork->isBoundaryDrifter = 1;
        if(characterWork->position.x < 0.0f) {
            characterWork->targetPosition.x = -2500;
        } else {
            characterWork->targetPosition.x = 2500;
        }
        characterWork->targetPosition.z = 500*((0.007874016f*((s32)frand() & 0x7F))-0.5f);
        characterWork->targetPosition.y = characterWork->position.y;
    }
}

typedef struct CharacterMotionWork {
    s16 modelId;
    s16 reservedModelFlags[2];
    s16 particleCount;
    Vec position;
    Vec targetPosition;
    float currentSpeed;
    float targetSpeed;
    float heading;
    float targetHeading;
    float travelDistance;
    float reservedEffectScale;
    float rotationSpeed;
    float steeringDirection;
} CharacterMotionWork;

void CharacterMoteModelUpdate(OMOBJ *object, CharacterMotionWork *characterWork)
{
    s32 num = characterWork[0].particleCount;
    float ofsZ = -2000;
    s32 i;
    for(i=0; i<num; i++, characterWork++) {
        Mtx *modelMtx;
        Mtx rotZ;
        Hu3DModelPosSet(characterWork->modelId, characterWork->position.x, characterWork->position.y, characterWork->position.z+ofsZ);
        modelMtx = &Hu3DData[characterWork->modelId].mtx;
        MTXRotDeg(*modelMtx, 'Y', characterWork->heading);
        MTXRotDeg(rotZ, 'Z', characterWork->rotationSpeed);
        MTXConcat(*modelMtx, rotZ, *modelMtx);
        Hu3DModelAttrReset(characterWork->modelId, HU3D_ATTR_DISPOFF);
    }
}

float AngleApproach(float start, float end, float weight);

void CharacterMoteUpdate(OMOBJ *object)
{
    CharacterMotionWork *characterWork = object->data;
    s32 num = characterWork[0].particleCount;
    s32 i;
    for(i=0; i<num; i++, characterWork++) {
        Vec diff;
        s32 nearbyParticleCount = 0;
        VECSubtract(&characterWork->targetPosition, &characterWork->position, &diff);
        characterWork->targetHeading = atan2d(diff.x, diff.z);
        characterWork->heading = AngleApproach(characterWork->heading, characterWork->targetHeading+characterWork->steeringDirection, 0.08f);
        characterWork->targetSpeed += 0.05f*(600.0f-characterWork->targetSpeed);
        characterWork->currentSpeed += 0.2f*(characterWork->targetSpeed-characterWork->currentSpeed);
        characterWork->position.x += (characterWork->currentSpeed*REFRESH_FREQ)*sind(characterWork->heading);
        characterWork->position.z += (characterWork->currentSpeed*REFRESH_FREQ)*cosd(characterWork->heading);
        characterWork->travelDistance += characterWork->currentSpeed*REFRESH_FREQ;
    }
    CharacterMoteModelUpdate(object, object->data);

}

void CharacterMoteCreate(OMOBJ *object)
{
    s16 num = 12;
    CharacterMotionWork *baseWork;
    s32 modelId;
    CharacterMotionWork *work;
    float radius;
    float angle;
    float ofsZ;
    s32 i;
    object->data = baseWork = HuMemDirectMallocNum(HEAP_HEAP, num*sizeof(CharacterMotionWork), HU_MEMNUM_OVL);
    memset(baseWork, 0, num*sizeof(CharacterMotionWork));
    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M455, 8));
    ofsZ = 1500;
    work = baseWork;
    for(i=0; i<num; i++, work++) {

        work->modelId = Hu3DModelLink(modelId);
        Hu3DModelLayerSet(work->modelId, 6);
        Hu3DModelCameraSet(work->modelId, 1);
        Hu3DModelScaleSet(work->modelId, 7, 7, 7);
        work->targetPosition.x = ((s32)frandmod(1000)*4.0f)-2000;
        work->targetPosition.z = ((s32)frandmod(1000))-500.0f;
        work->targetPosition.y = ((s32)frandmod(1000)*0.8f)+300;
        angle = (s32)frandmod(360);
        radius = ((s32)frandmod(250))+200.0f;
        work->position.x = radius*sind(angle);
        work->position.y = 0;
        work->position.z = radius*cosd(angle);
        VECAdd(&work->targetPosition, &work->position, &work->position);
        work->rotationSpeed = 60-((6.0f/55.0f)*radius);
        work->heading = (s32)frandmod(360);

        work->steeringDirection = ((s32)frandmod(1000) < 500) ? (-70.0f+((s32)frandmod(50)*0.1f)) : (70.0f-((s32)frandmod(50)*0.1f));
        if(work->steeringDirection < 0.0f) {
            work->rotationSpeed = -work->rotationSpeed;
        }
        work->travelDistance = 1;
        work->reservedEffectScale = 0;
        ofsZ -= 50.0f;
        work->particleCount = num;
    }
    Hu3DModelKill(modelId);
    object->objFunc = CharacterMoteUpdate;
}

float AngleApproach(float start, float end, float weight)
{
    float angle = fmodf(end-start, 360);
    if(angle < 0.0f) {
        angle += 360.0f;
    }
    if(angle > 180.0f) {
        angle -= 360.0f;
    }
    angle = fmodf(start+(weight*angle), 360);
    if(angle < 0.0f) {
        angle += 360.0f;
    }
    return angle;
}

void FallingParticleRender(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

void ParticlesCreate(void)
{
    FallingParticleWork *particleWork = fallingParticleState = HuMemDirectMallocNum(HEAP_HEAP, sizeof(FallingParticleWork), HU_MEMNUM_OVL);
    HU3DPARTICLE *particle;
    HU3DMODEL *modelData;
    s16 *newWork;
    s32 modelId;
    memset(particleWork, 0, sizeof(FallingParticleWork));
    particleWork->particleAnimations[0] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 13), HU_MEMNUM_OVL));
    particleWork->particleAnimations[1] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 14), HU_MEMNUM_OVL));
    particleWork->particleAnimations[2] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 15), HU_MEMNUM_OVL));
    particleWork->particleAnimations[3] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M455, 16), HU_MEMNUM_OVL));
    particleWork->particleAlpha.y = 1;
    particleWork->particleAlpha.z = 0;
    particleWork->particleModelIds[0] = modelId = Hu3DParticleCreate(particleWork->particleAnimations[0], 1);
    Hu3DModelLayerSet(modelId, 6);
    modelData = &Hu3DData[modelId];
    particle = modelData->hookData;
    particle->blendMode = 1;
    particle->hook = FallingParticleRender;
    newWork = particle->work = HuMemDirectMallocNum(HEAP_MODEL, sizeof(s16)*4, modelData->mallocNo);
    memset(newWork, 0, 4*sizeof(s16));
    newWork[1] = 0;
    particleWork->particleModelIds[1] = modelId = Hu3DParticleCreate(particleWork->particleAnimations[3], 10);
    Hu3DModelLayerSet(modelId, 6);
    modelData = &Hu3DData[modelId];
    particle = modelData->hookData;
    particle->blendMode = 1;
    particle->hook = FallingParticleRender;
    newWork = particle->work = HuMemDirectMallocNum(HEAP_MODEL, sizeof(s16)*4, modelData->mallocNo);
    memset(newWork, 0, 4*sizeof(s16));
    newWork[1] = 1;
    particleWork->particleModelIds[2] = modelId = Hu3DParticleCreate(particleWork->particleAnimations[2], 4);
    Hu3DModelLayerSet(modelId, 6);
    modelData = &Hu3DData[modelId];
    particle = modelData->hookData;
    particle->blendMode = 1;
    particle->hook = FallingParticleRender;
    newWork = particle->work = HuMemDirectMallocNum(HEAP_MODEL, sizeof(s16)*4, modelData->mallocNo);
    memset(newWork, 0, 4*sizeof(s16));
    newWork[1] = 2;
    particleWork->particleModelIds[3] = modelId = Hu3DParticleCreate(particleWork->particleAnimations[1], 6);
    Hu3DModelLayerSet(modelId, 6);
    modelData = &Hu3DData[modelId];
    particle = modelData->hookData;
    particle->blendMode = 1;
    particle->hook = FallingParticleRender;
    newWork = particle->work = HuMemDirectMallocNum(HEAP_MODEL, sizeof(s16)*4, modelData->mallocNo);
    memset(newWork, 0, 4*sizeof(s16));
    newWork[1] = 3;
}

void ParticlesDestroy(void)
{
    FallingParticleWork *particleWork = fallingParticleState;
#ifndef NON_MATCHING
    // these already get freed in HuSprAnimKill by hsfman.c
    HuSprAnimKill(particleWork->particleAnimations[0]);
    HuSprAnimKill(particleWork->particleAnimations[1]);
    HuSprAnimKill(particleWork->particleAnimations[2]);
    HuSprAnimKill(particleWork->particleAnimations[3]);
#endif
}

float fallingParticleScales[4] = {
    1000,
    200,
    600,
    1200
};

float fallingParticleAlphaScales[4] = {
    1,
    0.6,
    0.3,
    0.425
};

void FallingParticleRender(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    s16 activeParticleCount = 0;
    FallingParticleWork *particleWork = fallingParticleState;
    s16 *newWorkP = particle->work;
    s32 spawnedParticleCount = 0;
    Vec pos = Center;
    Vec rot = CRot;
    float particleDistance;
    float normalizedDistance;
    float zoom = CZoom;
    HU3DPARTICLEDATA *fallingParticle;
    Vec cameraDirection;
    Vec reflectionDirection;
    Vec reflectedDirection;
    Vec spawnDirection;
    float normalizationFactor;
    float particleSpacing;
    s32 i;
    switch(newWorkP[2]) {
        case 0:
            fallingParticle = particle->data;
            if(newWorkP[1]) {
                spawnDirection.x = 2000;
                spawnDirection.y = -300;
                spawnDirection.z = 500;
                normalizationFactor = 1/VECMagPoint(spawnDirection.x, spawnDirection.y, spawnDirection.z);
                spawnDirection.x *= normalizationFactor;
                spawnDirection.y *= normalizationFactor;
                spawnDirection.z *= normalizationFactor;
                particleSpacing = 3200.0f/particle->maxCnt;
                for(i=0; i<particle->maxCnt; i++, fallingParticle++) {
                    particleDistance = 500.0+((particleSpacing*i)+((0.25f*particleSpacing)*((float)((s32)frand() & 0xFFFF)*0.0000152587890625)));
                    normalizedDistance = particleDistance/3200;
                    fallingParticle->accel.x = particleDistance;
                    fallingParticle->pos.x = 1000+(particleDistance*spawnDirection.x);
                    fallingParticle->pos.y = 4800+(particleDistance*spawnDirection.y);
                    fallingParticle->pos.z = -3500+(particleDistance*spawnDirection.z);
                    fallingParticle->scale = ((0.7f*(normalizedDistance*normalizedDistance))+0.3f)*(fallingParticleScales[newWorkP[1]]*(0.7f+(0.3f*((float)((s32)frand() & 0xFFFF)*0.0000152587890625))));
                    fallingParticle->color.r = (s32)(((float)((s32)frand() & 0xFFFF)*0.0000152587890625)*160.0)+32;
                    fallingParticle->color.g = (s32)(((float)((s32)frand() & 0xFFFF)*0.0000152587890625)*96.0)+32;
                    fallingParticle->color.b = (s32)(((float)((s32)frand() & 0xFFFF)*0.0000152587890625)*128.0)+32;
                    fallingParticle->color.a = (255*fallingParticleAlphaScales[newWorkP[1]]*(1.0-(0.4*normalizedDistance)))-(s32)(((float)((s32)frand() & 0xFFFF)*0.0000152587890625)*16.0);
                    fallingParticle->accel.z = fallingParticle->color.a;
                }
                newWorkP[2]++;
            } else {
                fallingParticle->pos.x = 1000;
                fallingParticle->pos.y = 4800;
                fallingParticle->pos.z = -3500;
                fallingParticle->scale = 1000;
            }
            break;

        case 1:
            cameraDirection.x = sind(rot.y)*cosd(rot.x);
            cameraDirection.y = -sind(rot.x);
            cameraDirection.z = cosd(rot.y)*cosd(rot.x);
            reflectionDirection.x = 1000-(pos.x+(1.25f*(cameraDirection.x*zoom)));
            reflectionDirection.y = 4800-(pos.y+(cameraDirection.y*zoom));
            reflectionDirection.z = -3500-(pos.z+(1.25f*(cameraDirection.z*zoom)));
            normalizationFactor = 1/VECMagPoint(reflectionDirection.x, reflectionDirection.y, reflectionDirection.z);
            reflectionDirection.x *= normalizationFactor;
            reflectionDirection.y *= normalizationFactor;
            reflectionDirection.z *= normalizationFactor;
            normalizationFactor = -((-reflectionDirection.x*-cameraDirection.x)+(-reflectionDirection.y*-cameraDirection.y)+(-reflectionDirection.z*-cameraDirection.z));
            reflectedDirection.x = -cameraDirection.x+((-2.0f*reflectionDirection.x)*normalizationFactor);
            reflectedDirection.y = -cameraDirection.y+((-2.0f*reflectionDirection.y)*normalizationFactor);
            reflectedDirection.z = -cameraDirection.z+((-2.0f*reflectionDirection.z)*normalizationFactor);
            fallingParticle = particle->data;
            for(i=0; i<particle->maxCnt; i++, fallingParticle++) {
                fallingParticle->pos.x = 1000+(reflectedDirection.x*fallingParticle->accel.x);
                fallingParticle->pos.y = 4800+(reflectedDirection.y*fallingParticle->accel.x);
                fallingParticle->pos.z = -3500+(reflectedDirection.z*fallingParticle->accel.x);
                fallingParticle->color.a = fallingParticle->accel.z*particleWork->particleAlpha.y;
            }
            break;
    }
    DCFlushRange(particle->data, particle->maxCnt*sizeof(HU3DPARTICLEDATA));
    
}

void FallingParticleAlphaSet(float alpha)
{
    FallingParticleWork *particleWork = fallingParticleState;
    particleWork->particleAlpha.y = alpha;
    if(particleWork->particleAlpha.y > 1.0f) {
        particleWork->particleAlpha.y = 1.0f;
    }
    if(particleWork->particleAlpha.y <= 0.0f) {
        Hu3DModelAttrSet(particleWork->particleModelIds[0], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(particleWork->particleModelIds[1], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(particleWork->particleModelIds[2], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(particleWork->particleModelIds[3], HU3D_ATTR_DISPOFF);
    } else {
        Hu3DModelAttrReset(particleWork->particleModelIds[0], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(particleWork->particleModelIds[1], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(particleWork->particleModelIds[2], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(particleWork->particleModelIds[3], HU3D_ATTR_DISPOFF);
    }
}

float AngleShortestDifference(float startAngle, float endAngle)
{
    float angleDiff;
    if(startAngle >= 360.0f) {
        startAngle -= 360.0f;
    } else if(startAngle < 0.0f) {
        startAngle += 360.0f;
    }
    if(endAngle >= 360.0f) {
        endAngle -= 360.0f;
    } else if(endAngle < 0.0f) {
        endAngle += 360.0f;
    }
    angleDiff = startAngle-endAngle;
    if(angleDiff <= -180.0f) {
        angleDiff += 360.0f;
    } else if(angleDiff >= 180.0f) {
        angleDiff -= 360.0f;
    }
    return angleDiff;
}

float AngleWeightedInterpolate(float startAngle, float endAngle, float maxStep)
{
    float angle;
    float diff;
    
    if(startAngle >= 360.0) {
        startAngle -= 360.0;
    } else if(startAngle < 0.0) {
        startAngle += 360.0;
    }
    if(endAngle >= 360.0) {
        endAngle -= 360.0;
    } else if(endAngle < 0.0) {
        endAngle += 360.0;
    }
    diff = 360.0+(startAngle-endAngle);
    if(fabs(diff) >= 360.0) {
        diff = fmod(diff, 360.0);
    }
    if(diff < 180.0) {
        if(diff <= maxStep) {
            angle = diff;
        } else {
            angle = maxStep;
        }
    } else if(360.0-diff <= maxStep) {
        angle = -(360.0-diff);
    } else {
        angle = -maxStep;
    }
    angle += endAngle;
    if(angle >= 360.0) {
        angle -= 360.0;
    } else if(angle < 0.0) {
        angle += 360.0;
    }
    
    return angle;
}
