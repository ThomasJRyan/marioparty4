#include "REL/m438Dll.h"

#include "game/memory.h"
#define HUSPR_USE_OLD_DEFS
#include "game/hu3d.h"
#include "game/sprite.h"
#include <string.h>

#include "ext_math.h"

extern u8 texMtxTbl[];

// PROTO
void RenderParticleSystem(M438ParticleSystem*);
void RenderParticleSystemsHook(HU3DMODEL*, Mtx);
void BuildFireMaterialDisplayList(M438FireMaterial*);
void UpdateParticleSystem(M438ParticleSystem*);
void ReleaseParticleAnimation(s16);
s32 AssignAnimationToMaterial(s16, u8, s16);

void InitializeFireManager(s16 particleSystemCapacity, s16 animationCapacity) {
    M438ParticleSystem* particleSystem;
    s32 particleSystemIndex;
    M438ParticleAnimation* particleAnimation;

    fireManager.hookModelId = Hu3DHookFuncCreate(RenderParticleSystemsHook);
    Hu3DModelLayerSet(fireManager.hookModelId, 6);
    fireManager.particleSystemCapacity = particleSystemCapacity;
    particleSystem = HuMemDirectMallocNum(HEAP_HEAP, particleSystemCapacity * sizeof(M438ParticleSystem), 0x10000000);
    fireManager.particleSystems = particleSystem;
    fireManager.animationCapacity = animationCapacity;
    particleAnimation = HuMemDirectMallocNum(HEAP_HEAP, animationCapacity * sizeof(M438ParticleAnimation), 0x10000000);
    fireManager.animations = particleAnimation;
    
    for (particleSystemIndex = 0; particleSystemIndex < particleSystemCapacity; particleSystemIndex++, particleSystem++) {
        particleSystem->stateFlags = 0;
    }
    
    for (particleSystemIndex = 0; particleSystemIndex < animationCapacity; particleSystemIndex++, particleAnimation++) {
        particleAnimation->referenceCount = 0;
        particleAnimation->resourceId = 0;
    }
    fireManager.activeParticleSystemId = fireManager.activeMaterialIndex = fireManager.activeTextureSlot = 0;
}

void RenderParticleSystem(M438ParticleSystem* particleSystem) {
    HU3DMODEL lightModel;
    Mtx particlePositionMatrix;
    Mtx cameraPositionMatrix;
    Mtx scratchMatrix;
    Mtx rotationMatrix;
    Mtx textureMatrix;
    GXColor tevColor;
    s16 unreferencedTevScratch;
    M438FireMaterial* material;
    s16 tevStageCount;
    s16 texCoordCount;
    s32 textureMap;
    M438ParticleAnimation* particleAnimation;
    s32 colorSource;
    s32 materialIndex;
    s16 lightSetIndex;
    s32 textureMatrixId;
    s32 bumpTextureMatrixId;
    s32 shadowTextureMatrixId;
    s32 projectedTextureMatrixId;

    mtxRot(rotationMatrix, particleSystem->rotation.x, particleSystem->rotation.y, particleSystem->rotation.z);
    PSMTXScale(scratchMatrix, particleSystem->scale.x, particleSystem->scale.y, particleSystem->scale.z);
    PSMTXConcat(rotationMatrix, scratchMatrix, scratchMatrix);
    mtxTransCat(scratchMatrix, particleSystem->position.x, particleSystem->position.y, particleSystem->position.z);
    PSMTXConcat(Hu3DCameraMtx, scratchMatrix, cameraPositionMatrix);
    tevStageCount = 0;
    texCoordCount = 0;
    unreferencedTevScratch = 0;
    for (textureMap = GX_TEXMAP0; textureMap < particleSystem->animationSlotCount; textureMap++) {
        if (particleSystem->animationIds[textureMap] != 0) {
            particleAnimation = &fireManager.animations[particleSystem->animationIds[textureMap]];
            PSMTXCopy(particleAnimation->textureMatrix, textureMatrix);
            mtxTransCat(textureMatrix, particleAnimation->position.x, particleAnimation->position.y, particleAnimation->position.z);
            switch (particleAnimation->textureMappingMode) {
                case 0:
                    GXLoadTexMtxImm(textureMatrix, texMtxTbl[texCoordCount], GX_MTX2x4);
                    textureMatrixId = texMtxTbl[texCoordCount];
                    GXSetTexCoordGen2(texCoordCount, GX_TG_MTX2x4, textureMap + 4, textureMatrixId, 0, 0x7D);
                    GXSetTevOrder(tevStageCount, texCoordCount, textureMap, GX_COLOR0A0);
                    if (textureMap == GX_TEXMAP0) {
                        if ((particleAnimation->animationData->bmp->dataFmt == 7) || (particleAnimation->animationData->bmp->dataFmt == 8)) {
                            GXSetTevColor(GX_TEVREG2, particleAnimation->tintColor);
                            GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC, GX_CC_ZERO);
                        } else {
                            GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
                        }
                        GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
                    } else {
                        tevColor.a = particleAnimation->tintColor.a;
                        GXSetTevColor(GX_TEVREG2, tevColor);
                        GXSetTevColorIn(tevStageCount, GX_CC_CPREV, GX_CC_TEXC, GX_CC_A2, GX_CC_ZERO);
                        GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
                    }
                    GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    break;
                case 1:
                    GXLoadTexMtxImm(textureMatrix, texMtxTbl[texCoordCount], GX_MTX2x4);
                    bumpTextureMatrixId = texMtxTbl[texCoordCount];
                    GXSetTexCoordGen2(texCoordCount, GX_TG_MTX2x4, textureMap + 4, bumpTextureMatrixId, 0, 0x7D);
                    GXSetTevOrder(tevStageCount, texCoordCount, textureMap, GX_COLOR0A0);
                    GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_CPREV, GX_CC_TEXA, GX_CC_ZERO);
                    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
                    GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    break;
                case 2:
                    tevColor.a = 0xE0;
                    GXSetTevColor(GX_TEVREG1, tevColor);
                    GXSetTexCoordGen2(texCoordCount + 1, GX_TG_BUMP0, texCoordCount, 0x3C, 0, 0x7D);
                    GXSetTevOrder(tevStageCount, texCoordCount, textureMap, GX_COLOR0A0);
                    GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A1, GX_CC_RASC);
                    GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
                    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 0, GX_TEVPREV);
                    tevStageCount++;
                    GXSetTevOrder(tevStageCount, texCoordCount + 1, GX_TEXMAP0, GX_COLOR0A0);
                    GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A1, GX_CC_CPREV);
                    GXSetTevColorOp(tevStageCount, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
                    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 0, GX_TEVPREV);
                    break;
                case 3:
                    texCoordCount--;
                    GXLoadTexMtxImm(textureMatrix, texMtxTbl[texCoordCount], GX_MTX2x4);
                    shadowTextureMatrixId = texMtxTbl[texCoordCount];
                    GXSetTexCoordGen2(texCoordCount, GX_TG_MTX2x4, textureMap + 4, shadowTextureMatrixId, 0, 0x7D);
                    GXSetTevOrder(tevStageCount, texCoordCount, textureMap, GX_COLOR0A0);
                    GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_CPREV, GX_CC_TEXC, GX_CC_ZERO);
                    GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
                    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    texCoordCount++;
                    break;
                case 4:
                    mtxRot(rotationMatrix, particleSystem->rotation.x, particleSystem->rotation.y, particleSystem->rotation.z);
                    PSMTXScale(scratchMatrix, particleSystem->scale.x, particleSystem->scale.y, particleSystem->scale.z);
                    PSMTXConcat(rotationMatrix, scratchMatrix, scratchMatrix);
                    mtxTransCat(scratchMatrix, particleSystem->position.x, particleSystem->position.y, particleSystem->position.z);
                    PSMTXConcat(Hu3DCameraMtx, scratchMatrix, textureMatrix);
                    PSMTXInverse(Hu3DCameraMtx, rotationMatrix);
                    PSMTXConcat(rotationMatrix, textureMatrix, rotationMatrix);
                    PSMTXConcat(Hu3DShadowData.projMtx, Hu3DShadowData.lookAtMtx, scratchMatrix);
                    PSMTXConcat(scratchMatrix, rotationMatrix, textureMatrix);
                    GXLoadTexMtxImm(textureMatrix, texMtxTbl[texCoordCount], GX_MTX3x4);
                    projectedTextureMatrixId = texMtxTbl[texCoordCount];
                    GXSetTexCoordGen2(texCoordCount, GX_TG_MTX3x4, GX_TG_POS, projectedTextureMatrixId, 0, 0x7D);
                    GXSetTevOrder(tevStageCount, texCoordCount, textureMap, GX_COLOR0A0);
                    if (textureMap == GX_TEXMAP0) {
                        GXSetTevColorIn(tevStageCount, GX_CC_ONE, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ZERO);
                        GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
                    } else {
                        GXSetTevColorIn(tevStageCount, GX_CC_CPREV, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ZERO);
                        GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
                    }
                    GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 0, GX_TEVPREV);
                    break;
            }
            tevStageCount++;
            texCoordCount++;
            if (particleAnimation->isTiled != 0) {
                HuSprTexLoad(particleAnimation->animationData, 0, textureMap, 1, 1, 1);
            } else {
                HuSprTexLoad(particleAnimation->animationData, 0, textureMap, 0, 0, 1);
            }
        }
    }
    GXSetNumTevStages((s16)tevStageCount);
    GXSetNumTexGens((s16)texCoordCount);
    material = particleSystem->materials;
    for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
        if ((material->stateFlags != 0) && ((material->stateFlags & 4) == 0)) {
            switch (material->enabled) {
                case 0:
                    mtxRot(rotationMatrix, material->rotation.x, material->rotation.y, material->rotation.z);
                    if (material->textureCoordinateCount == 2) {
                        PSMTXConcat(fireManager.inverseCameraMatrix, rotationMatrix, rotationMatrix);
                    } else if (material->textureCoordinateCount == 1) {
                        PSMTXRotRad(scratchMatrix, 0x59, MTXDegToRad(CRot.y));
                        PSMTXConcat(scratchMatrix, rotationMatrix, rotationMatrix);
                    }
                    PSMTXScale(scratchMatrix, material->scale.x, material->scale.y, material->scale.z);
                    PSMTXTrans(textureMatrix, material->secondaryScale.x, material->secondaryScale.y, material->secondaryScale.z);
                    PSMTXConcat(scratchMatrix, textureMatrix, scratchMatrix);
                    PSMTXConcat(rotationMatrix, scratchMatrix, scratchMatrix);
                    mtxTransCat(scratchMatrix, material->position.x, material->position.y, material->position.z);
                    PSMTXConcat(Hu3DCameraMtx, scratchMatrix, particlePositionMatrix);
                    break;
                case 1:
                    mtxRot(rotationMatrix, material->rotation.x, material->rotation.y, material->rotation.z);
                    if (material->textureCoordinateCount == 2) {
                        if (HmfInverseMtxF3X3(cameraPositionMatrix, scratchMatrix) == 0) {
                            PSMTXIdentity(scratchMatrix);
                        }
                        PSMTXConcat(scratchMatrix, rotationMatrix, rotationMatrix);
                    } else if (material->textureCoordinateCount == 1) {
                        PSMTXRotRad(scratchMatrix, 0x59, MTXDegToRad(CRot.y));
                        PSMTXConcat(scratchMatrix, rotationMatrix, rotationMatrix);
                    }
                    PSMTXScale(scratchMatrix, material->scale.x, material->scale.y, material->scale.z);
                    PSMTXTrans(textureMatrix, material->secondaryScale.x, material->secondaryScale.y, material->secondaryScale.z);
                    PSMTXConcat(scratchMatrix, textureMatrix, scratchMatrix);
                    PSMTXConcat(rotationMatrix, scratchMatrix, scratchMatrix);
                    mtxTransCat(scratchMatrix, material->position.x, material->position.y, material->position.z);
                    PSMTXConcat(cameraPositionMatrix, scratchMatrix, particlePositionMatrix);
                    break;
            }
            GXLoadPosMtxImm(particlePositionMatrix, 0);
            PSMTXInverse(particlePositionMatrix, scratchMatrix);
            PSMTXTranspose(scratchMatrix, rotationMatrix);
            GXLoadNrmMtxImm(rotationMatrix, 0);
            GXSetChanAmbColor(GX_COLOR0A0, material->baseColor);
            GXSetChanMatColor(GX_COLOR0A0, material->modulationColor);
            GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
            GXSetZCompLoc(0);
            switch (material->blendMode) {
                case 0:
                    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
                    break;
                case 1:
                    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
                    break;
                case 2:
                    GXSetBlendMode(GX_BM_BLEND, GX_BL_ZERO, GX_BL_INVDSTCLR, GX_LO_NOOP);
                    break;
            }
            if ((material->stateFlags & 0x10) != 0) {
                GXSetZMode(0, GX_LEQUAL, 1);
            } else {
                GXSetZMode(1, GX_LEQUAL, 0);
            }
            lightModel.attr = 0;
            lightSetIndex = Hu3DLightSet(&lightModel, &Hu3DCameraMtx, &Hu3DCameraMtxXPose, 0.0f);
            GXSetNumChans(1);
            if ((material->stateFlags & 8) != 0) {
                colorSource = GX_SRC_VTX;
            } else {
                colorSource = GX_SRC_REG;
            }
            switch (material->animationMode) {
                case 0:
                    GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, colorSource, 0, GX_DF_CLAMP, GX_AF_NONE);
                    break;
                case 1:
                    GXSetChanCtrl(GX_COLOR0A0, 1, GX_SRC_REG, colorSource, lightSetIndex, GX_DF_CLAMP, GX_AF_SPOT);
                    break;
                case 2:
                    GXSetChanCtrl(GX_COLOR0A0, 1, GX_SRC_REG, colorSource, lightSetIndex, GX_DF_CLAMP, GX_AF_SPEC);
                    break;
            }
            GXClearVtxDesc();
            GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_RGBA6, 0);
            GXSetArray(GX_VA_POS, material->vertexCoordinates, 12);
            GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
            GXSetArray(GX_VA_CLR0, material->vertexColors, 4);
            for (textureMap = 0; textureMap < material->textureSlotCount; textureMap++) {
                GXSetVtxDesc(textureMap + GX_VA_TEX0, GX_INDEX16);
                GXSetVtxAttrFmt(GX_VTXFMT0, textureMap + GX_VA_TEX0, GX_TEX_ST, GX_RGBA6, 0);
                GXSetArray(textureMap + GX_VA_TEX0, &material->animationTracks[textureMap].topLeftU, 8);
            }
            GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_RGBA6, 0U);
            GXSetArray(GX_VA_NRM, material->vertexNormals, 12);
            GXCallDisplayList(material->displayList, material->displayListSize);
        }
    }
}

void RenderParticleSystemsHook(HU3DMODEL* model, Mtx matrix) {
    Mtx inversePositionMatrix;
    Mtx normalMatrix;
    M438ParticleSystem* particleSystem;
    s32 particleSystemIndex;

    particleSystem = fireManager.particleSystems;
    GXLoadPosMtxImm(matrix, 0);
    PSMTXInverse(matrix, inversePositionMatrix);
    PSMTXTranspose(inversePositionMatrix, normalMatrix);
    GXLoadNrmMtxImm(normalMatrix, 0);
    HmfInverseMtxF3X3(Hu3DCameraMtx, fireManager.inverseCameraMatrix);
    
    for (particleSystemIndex = 0; particleSystemIndex < fireManager.particleSystemCapacity; particleSystemIndex++, particleSystem++) {
        if ((particleSystem->stateFlags != 0) && ((particleSystem->stateFlags & 2) != 0) && ((particleSystem->stateFlags & 4) == 0)) {
            if ((u8)omPauseChk() == 0) {
                if (particleSystem->updateCallback) {
                    particleSystem->updateCallback(particleSystem);
                }
                if (particleSystem->stateFlags == 0) continue;
                UpdateParticleSystem(particleSystem);
                if (particleSystem->stateFlags == 0) continue;
            }
            RenderParticleSystem(particleSystem);
        }
    }
}

M438ParticleSystem* GetParticleSystem(s16 particleSystemId) {
    return &fireManager.particleSystems[particleSystemId];
}

M438FireMaterial* GetFireMaterial(s16 particleSystemId, s16 materialIndex) {
    return &fireManager.particleSystems[particleSystemId].materials[materialIndex];
}

M438ParticleAnimation* GetParticleAnimation(s16 animationId) {
    return &fireManager.animations[animationId];
}

void CreateFireQuadVertices(M438FireQuadVertices** vertices, s16 vertexCount, f32 width, f32 height) {
    s32 vertexIndex;
    M438FireQuadVertices* value2;

    value2 = *vertices = HuMemDirectMallocNum(HEAP_HEAP, vertexCount * sizeof(M438FireQuadVertices), 0x10000000);
    
    for (vertexIndex = 0; vertexIndex < vertexCount; value2++, vertexIndex++) {
        value2->topLeft.x  = (-0.5f * width);
        value2->topLeft.y  = (0.5f * height);
        value2->topLeft.z  = 0.0f;
        value2->topRight.x  = (0.5f * width);
        value2->topRight.y  = (0.5f * height);
        value2->topRight.z  = 0.0f;
        value2->bottomRight.x = (0.5f * width);
        value2->bottomRight.y = (-0.5f * height);
        value2->bottomRight.z = 0.0f;
        value2->bottomLeft.x = (-0.5f * width);
        value2->bottomLeft.y = (-0.5f * height);
        value2->bottomLeft.z = 0.0f;
    }
}

void CopyFireQuadVertices(M438FireQuadVertices** outputArray, s16 vertexArray, Vec* sourceValues) {
    Vec* vector2;
    s32 quadIndex;
    s32 quadVertexIndex;

    vector2 = (Vec*)*outputArray = HuMemDirectMallocNum(HEAP_HEAP, vertexArray * sizeof(M438FireQuadVertices), 0x10000000);
    
    for (quadIndex = 0; quadIndex < vertexArray; quadIndex++) {
        for (quadVertexIndex = 0; quadVertexIndex < 4; vector2++, sourceValues++, quadVertexIndex++) {
            vector2->x = sourceValues->x;
            vector2->y = sourceValues->y;
            vector2->z = sourceValues->z;
        }
    }
}

void CopyFireQuadColors(M438FireQuadColors** outputArray, s16 colorArray, GXColor* sourceColors) {
    GXColor* color2;
    s32 quadIndex;
    s32 quadVertexIndex;

    color2 = (GXColor*)*outputArray = HuMemDirectMallocNum(HEAP_HEAP, colorArray * sizeof(M438FireQuadColors), 0x10000000U);
    
    for (quadIndex = 0; quadIndex < colorArray; quadIndex++) {
        for (quadVertexIndex = 0; quadVertexIndex < 4; color2++, sourceColors++, quadVertexIndex++) {
            color2->r = sourceColors->r;
            color2->g = sourceColors->g;
            color2->b = sourceColors->b;
            color2->a = sourceColors->a;
        }
    }
}

void CopyFireQuadNormals(Vec** normalArrayOut, s16 normalCount, Vec* sourceNormals) {
    s32 vectorIndex;
    Vec* destinationVector;
    
    destinationVector = *normalArrayOut = HuMemDirectMallocNum(HEAP_HEAP, normalCount * sizeof(Vec), 0x10000000);
    
    for (vectorIndex = 0; vectorIndex < normalCount; destinationVector++, sourceNormals++, vectorIndex++) {
        destinationVector->x = sourceNormals->x;
        destinationVector->y = sourceNormals->y;
        destinationVector->z = sourceNormals->z;
    }
}

void CopyFireQuadUvs(M438FireQuadTextureCoordinates** outputArray, s16 textureCoordinateArray, HuVec2f* sourceValues) {
    HuVec2f* vector2;
    s32 quadIndex;
    s32 quadVertexIndex;
    
    vector2 = (HuVec2f*)*outputArray = HuMemDirectMallocNum(HEAP_HEAP, textureCoordinateArray * sizeof(M438FireQuadTextureCoordinates), 0x10000000);
    
    for (quadIndex = 0; quadIndex < textureCoordinateArray; quadIndex++) {
        for (quadVertexIndex = 0; quadVertexIndex < 4; vector2++, sourceValues++, quadVertexIndex++) {
            vector2->x = sourceValues->x;
            vector2->y = sourceValues->y;
        }
    }
}

void BuildFireMaterialDisplayList(M438FireMaterial* material) {
    Vec vector[4] = {
        { -50.0f,  50.0f, 0.0f },
        {  50.0f,  50.0f, 0.0f },
        {  50.0f, -50.0f, 0.0f },
        { -50.0f, -50.0f, 0.0f }
    };
    GXColor color[4] = {
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF }
    };
    
    void* buffer;
    s32 vertexIndex;
    s32 textureCoordinateIndex;
    void* buffer2;
    
    
    for (vertexIndex = 0; vertexIndex < 4; vertexIndex++) {
        material->vertexCoordinates[vertexIndex] = vector[vertexIndex];
    }
    
    for (vertexIndex = 0; vertexIndex < 4; vertexIndex++) {
        material->vertexColors[vertexIndex] = color[vertexIndex];
    }
    material->vertexNormals->x = 0.0f;
    material->vertexNormals->y = 0.0f;
    material->vertexNormals->z = 1.0f;
    buffer2 = material->displayList;
    buffer = buffer2;
    GXBeginDisplayList(buffer2, 0x100);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    
    for (vertexIndex = 0; vertexIndex < 4; vertexIndex++) {
        GXPosition1x16(vertexIndex);
        GXColor1x16(0);
        GXNormal1x16(vertexIndex);
        for (textureCoordinateIndex = 0; textureCoordinateIndex < material->textureSlotCount; textureCoordinateIndex++) {
            GXTexCoord1x16(vertexIndex);
        }
    }
    material->displayListSize = GXEndDisplayList();
}

void UpdateParticleSystem(M438ParticleSystem* particleSystem) {
    M438FireMaterial* material;
    M438ParticleAnimation* particleAnimation;
    M438ParticleAnimationTrack* animationTrack;
    s32 completedLayerCount;
    s32 materialIndex;
    s32 textureLayerIndex;

    completedLayerCount = 0;
    
    material = particleSystem->materials;
    for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
        if ((material->stateFlags & 4) != 0) {
            completedLayerCount++;
        } else {
            animationTrack = material->animationTracks;
            for (textureLayerIndex = 0; textureLayerIndex < material->textureSlotCount; textureLayerIndex++, animationTrack++) {
                particleAnimation = &fireManager.animations[particleSystem->animationIds[textureLayerIndex]];
                animationTrack->playbackFlags &= ~4;
                switch (animationTrack->playbackFlags & 0xF) {
                    case 1:
                        animationTrack->frameAccumulator += animationTrack->frameStep;
                        if (animationTrack->frameAccumulator >= 1.0f) {
                            animationTrack->frameIndex++;
                            animationTrack->frameAccumulator -= 1.0f;
                        }
                        break;
                    case 2:
                        animationTrack->frameAccumulator += animationTrack->frameStep;
                        if (animationTrack->frameAccumulator >= 1.0f) {
                            animationTrack->frameIndex -= 1;
                            animationTrack->frameAccumulator -= 1.0f;
                        }
                        break;
                }
                if (animationTrack->frameIndex >= particleAnimation->rowsPerFrame) {
                    if ((animationTrack->playbackFlags & 0x10) != 0) {
                        animationTrack->playbackFlags |= 8;
                        animationTrack->frameIndex -= 1;
                    } else if ((animationTrack->playbackFlags & 0x40) != 0) {
                        animationTrack->playbackFlags = (animationTrack->playbackFlags & 0xF0) | 2;
                        animationTrack->frameIndex -= 2;
                    } else if ((animationTrack->playbackFlags & 0x20) != 0) {
                        animationTrack->frameIndex = 0;
                    }
                    animationTrack->playbackFlags |= 4;
                } else if (animationTrack->frameIndex < 0) {
                    if ((animationTrack->playbackFlags & 0x10) != 0) {
                        animationTrack->playbackFlags |= 8;
                        animationTrack->frameIndex = 0;
                    } else if ((animationTrack->playbackFlags & 0x20) != 0) {
                        if ((animationTrack->playbackFlags & 0x40) != 0) {
                            animationTrack->playbackFlags = (animationTrack->playbackFlags & 0xF0) | 1;
                            animationTrack->frameIndex = 1;
                        } else {
                            animationTrack->frameIndex = particleAnimation->rowsPerFrame - 1;
                        }
                    } else if ((animationTrack->playbackFlags & 0x40) != 0) {
                        animationTrack->playbackFlags |= 8;
                        animationTrack->frameIndex = 0;
                    }
                    animationTrack->playbackFlags |= 4;
                } else {
                    animationTrack->playbackFlags &= ~8;
                }
                if (animationTrack->frameIndex != animationTrack->previousFrameIndex) {
                    animationTrack->topLeftV = (animationTrack->frameIndex * particleAnimation->textureScaleY);
                    animationTrack->topRightV = (animationTrack->frameIndex * particleAnimation->textureScaleY);
                    animationTrack->bottomRightV = ((animationTrack->frameIndex + 1) * particleAnimation->textureScaleY);
                    animationTrack->bottomLeftV = ((animationTrack->frameIndex + 1) * particleAnimation->textureScaleY);
                    if ((animationTrack->frameIndex + 1) < particleAnimation->rowsPerFrame) {
                        animationTrack->bottomRightV -= particleAnimation->inverseTextureHeight;
                        animationTrack->bottomLeftV -= particleAnimation->inverseTextureHeight;
                    }
                    animationTrack->previousFrameIndex = animationTrack->frameIndex;
                    DCFlushRangeNoSync(&animationTrack->topLeftU, 0x20);
                }
                if ((animationTrack->playbackFlags & 8) != 0) {
                    if ((material->stateFlags & 0x40000000) != 0) {
                        material->stateFlags |= 4;
                        if ((animationTrack->playbackFlags & 0x40) == 0) {
                            if (animationTrack->frameIndex <= 0) {
                                animationTrack->frameIndex = particleAnimation->rowsPerFrame;
                            } else {
                                animationTrack->frameIndex = 0;
                            }
                        }
                    } else if ((material->stateFlags & 0x80000000) != 0) {
                        DestroyParticleSystem(particleSystem->systemId);
                    }
                    completedLayerCount++;
                }
            }
        }
    }
    if (completedLayerCount >= particleSystem->materialCount) {
        if ((particleSystem->stateFlags & 0x40000000) != 0) {
            particleSystem->stateFlags |= 4;
        } else if ((particleSystem->stateFlags & 0x80000000) != 0) {
            DestroyParticleSystem(particleSystem->systemId);
        }
    }
    PPCSync();
}

s16 CreateParticleSystem(u8 materialCount, u8 animationSlotCount) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;
    s32 systemIndex;
    s32 animationSlotIndex;
    void* buffer;

    particleSystem = &fireManager.particleSystems[1];
    
    for (systemIndex = 1; systemIndex < fireManager.particleSystemCapacity; systemIndex++, particleSystem++) {
        if (particleSystem->stateFlags == 0) break;
    }
    if (systemIndex == fireManager.particleSystemCapacity) {
        OSReport("OVER BILL\n");
        return 0;
    }
    particleSystem->systemId = systemIndex;
    particleSystem->stateFlags = 1;
    particleSystem->updateCallback = NULL;
    particleSystem->materialCount = materialCount;
    particleSystem->animationSlotCount = animationSlotCount;
    material = HuMemDirectMallocNum(HEAP_HEAP, materialCount * 0x98, 0x10000000);
    particleSystem->materials = material;
    particleSystem->animationIds = HuMemDirectMallocNum(HEAP_HEAP, animationSlotCount * 2, 0x10000000);
    particleSystem->displayListSize = 0;
    particleSystem->displayList = HuMemDirectMallocNum(HEAP_HEAP, materialCount << 8, 0x10000000);
    particleSystem->vertexBuffer = HuMemDirectMallocNum(HEAP_HEAP, materialCount * 0x30, 0x10000000);
    particleSystem->colorBuffer = HuMemDirectMallocNum(HEAP_HEAP, materialCount * 0x10, 0x10000000);
    particleSystem->normalBuffer = HuMemDirectMallocNum(HEAP_HEAP, materialCount * 0xC, 0x10000000);
    particleSystem->animationTracks = HuMemDirectMallocNum(HEAP_HEAP, animationSlotCount * (materialCount * 0x30), 0x10000000);
    DCFlushRangeNoSync(particleSystem->displayList, materialCount << 8);
    particleSystem->position.x = particleSystem->position.y = particleSystem->position.z = 0.0f;
    particleSystem->rotation.x = particleSystem->rotation.y = particleSystem->rotation.z = 0.0f;
    particleSystem->scale.x = particleSystem->scale.y = particleSystem->scale.z = 1.0f;
    particleSystem->textureRotation.x = particleSystem->textureRotation.y = particleSystem->textureRotation.z = 0.0f;
    particleSystem->textureOffset.x = particleSystem->textureOffset.y = particleSystem->textureOffset.z = 0.0f;
    particleSystem->coordinateFlags = 0;
    
    for (systemIndex = 0; systemIndex < 0x20; systemIndex++) {
        particleSystem->animationFrameData[systemIndex] = 0;
    }
    
    for (systemIndex = 0; systemIndex < animationSlotCount; systemIndex++) {
        particleSystem->animationIds[systemIndex] = 0;
    }
    
    for (systemIndex = 0; systemIndex < materialCount; systemIndex++, material++) {
        material->stateFlags = 1;
        material->textureSlotCount = animationSlotCount;
        material->enabled = 0;
        material->textureCoordinateCount = 2;
        material->animationMode = 0;
        material->blendMode = 0;
        material->baseColor.r = material->baseColor.g = material->baseColor.b = 0;
        material->baseColor.a = 0xFF;
        material->modulationColor.r = material->modulationColor.g = material->modulationColor.b = 0xFF;
        material->modulationColor.a = 0xFF;
        material->animationTracks = &particleSystem->animationTracks[systemIndex * animationSlotCount];
        
        for (animationSlotIndex = 0; animationSlotIndex < animationSlotCount; animationSlotIndex++) {
            material->animationTracks[animationSlotIndex].frameIndex = material->animationTracks[animationSlotIndex].previousFrameIndex = 0;
            material->animationTracks[animationSlotIndex].frameStep = 1.0f;
            material->animationTracks[animationSlotIndex].frameAccumulator = 0.0f;
        }
        material->position.x = material->position.y = material->position.z = 0.0f;
        material->rotation.x = material->rotation.y = material->rotation.z = 0.0f;
        material->scale.x = material->scale.y = material->scale.z = 1.0f;
        material->particleOffset.x = material->particleOffset.y = material->particleOffset.z = 0.0f;
        material->secondaryScale.x = material->secondaryScale.y = material->secondaryScale.z = 0.0f;
        material->textureOffset.x = material->textureOffset.y = material->textureOffset.z = 0.0f;
        material->renderFlags = 0;
        
        for (animationSlotIndex = 0; animationSlotIndex < 0x10; animationSlotIndex++) {
            material->animationFrameData[animationSlotIndex] = 0;
        }
        material->vertexCoordinates = &particleSystem->vertexBuffer[systemIndex * 4];
        material->vertexColors = &particleSystem->colorBuffer[systemIndex * 4];
        material->vertexNormals = &particleSystem->normalBuffer[systemIndex];
        material->displayList = ((char*)particleSystem->displayList) + particleSystem->displayListSize;
        BuildFireMaterialDisplayList(material);
        particleSystem->displayListSize += material->displayListSize;
    }
    buffer = particleSystem->displayList;
    particleSystem->displayList = HuMemDirectMallocNum(HEAP_HEAP, particleSystem->displayListSize, 0x10000000);
    memcpy(particleSystem->displayList, buffer, particleSystem->displayListSize);
    HuMemDirectFree(buffer);
    DCFlushRangeNoSync(particleSystem->vertexBuffer, materialCount * 0x30);
    DCFlushRangeNoSync(particleSystem->colorBuffer, materialCount * 0x10);
    DCFlushRangeNoSync(particleSystem->normalBuffer, materialCount * 0xC);
    DCFlushRangeNoSync(particleSystem->displayList, particleSystem->displayListSize);
    material = particleSystem->materials;
    particleSystem->displayListSize = 0;
    
    for (systemIndex = 0; systemIndex < materialCount; systemIndex++, material++) {
        material->displayList = ((char*)particleSystem->displayList) + particleSystem->displayListSize;
        particleSystem->displayListSize += material->displayListSize;
    }
    fireManager.activeParticleSystemId = particleSystem->systemId;
    fireManager.activeMaterialIndex = 0;
    fireManager.activeTextureSlot = 0;
    PPCSync();
    return particleSystem->systemId;
}

void DestroyParticleSystem(s16 particleSystemId) {
    M438ParticleSystem* particleSystem;
    s32 textureSlotIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if ((u32) particleSystem->stateFlags != 0) {
            if (particleSystem->vertexBuffer != NULL) {
                HuMemDirectFree(particleSystem->vertexBuffer);
            }
            if (particleSystem->colorBuffer != NULL) {
                HuMemDirectFree(particleSystem->colorBuffer);
            }
            if (particleSystem->normalBuffer != NULL) {
                HuMemDirectFree(particleSystem->normalBuffer);
            }
            if (particleSystem->displayList != NULL) {
                HuMemDirectFree(particleSystem->displayList);
            }
            if (particleSystem->animationTracks != NULL) {
                HuMemDirectFree(particleSystem->animationTracks);
            }
            
            for (textureSlotIndex = 0; textureSlotIndex < particleSystem->animationSlotCount; textureSlotIndex++) {
                ReleaseParticleAnimation(particleSystem->animationIds[textureSlotIndex]);
            }
            HuMemDirectFree(particleSystem->materials);
            HuMemDirectFree(particleSystem->animationIds);
            particleSystem->stateFlags = 0;
            particleSystem->systemId = 0;
        }
    }
}

void SelectParticleMaterial(s16 particleSystemId, s16 materialIndex, u8 textureSlotIndex) {
    fireManager.activeParticleSystemId = particleSystemId;
    fireManager.activeMaterialIndex = materialIndex;
    fireManager.activeTextureSlot = textureSlotIndex;
}

#include "game/sprite.h"

s16 LoadParticleAnimation(u32 resourceId, s16 columnCount, s16 rowCount) {
    f32 textureScaleX;
    f32 textureScaleY;
    ANIMDATA* animationData;
    M438ParticleAnimation* particleAnimation;
    s32 animationIndex;

    particleAnimation = &fireManager.animations[1];
    for (animationIndex = 1; animationIndex < fireManager.animationCapacity; animationIndex++, particleAnimation++) {
        if ((particleAnimation->referenceCount != 0) && (particleAnimation->resourceId == resourceId)) break;
    }
    if (animationIndex < fireManager.animationCapacity) {
        particleAnimation->referenceCount++;
        return animationIndex;
    }
    
    particleAnimation = &fireManager.animations[1];
    for (animationIndex = 1; animationIndex < fireManager.animationCapacity; animationIndex++, particleAnimation++) {
        if (particleAnimation->referenceCount == 0) break;
    }
    particleAnimation->resourceId = resourceId;
    particleAnimation->referenceCount = 1;
    if (((resourceId & 0xFFFF0000) + 0x10000) == 0) {
        animationData = HuSprAnimMake(columnCount, rowCount, 2);
        animationData->bmp->data = HuMemDirectMallocNum(HEAP_HEAP, rowCount * (columnCount * 2), 0x10000000);
        columnCount = rowCount = 0;
    } else {
        animationData = HuSprAnimRead(HuDataReadNum(resourceId, 0x10000000));
    }
    if ((columnCount == 0) || (rowCount == 0)) {
        columnCount = animationData->bmp->sizeX;
        rowCount = animationData->bmp->sizeY;
        textureScaleX = textureScaleY = 1.0f;
    } else {
        textureScaleX = columnCount / (f32)animationData->bmp->sizeX;
        textureScaleY = rowCount / (f32)animationData->bmp->sizeY;
    }
    particleAnimation->animationData = animationData;
    particleAnimation->frameWidth = columnCount;
    particleAnimation->frameHeight = rowCount;
    particleAnimation->textureScaleX = textureScaleX;
    particleAnimation->textureScaleY = textureScaleY;
    particleAnimation->inverseTextureWidth = 1.0f / animationData->bmp->sizeX;
    particleAnimation->inverseTextureHeight = 1.0f / animationData->bmp->sizeY;
    particleAnimation->textureMappingMode = 0;
    particleAnimation->rowsPerFrame = animationData->bmp->sizeY / rowCount;
    if (particleAnimation->rowsPerFrame < 1) {
        particleAnimation->rowsPerFrame = 1;
    }
    particleAnimation->isTiled = 0;
    if ((columnCount > animationData->bmp->sizeX) || (rowCount > animationData->bmp->sizeY)) {
        particleAnimation->isTiled = 1;
    }
    particleAnimation->tintColor.r = particleAnimation->tintColor.g = particleAnimation->tintColor.b = 0xFF;
    particleAnimation->tintColor.a = 0xFF;
    PSMTXIdentity(particleAnimation->textureMatrix);
    particleAnimation->position.x = particleAnimation->position.y = particleAnimation->position.z = 0.0f;
    return animationIndex;
}

void CreateParticleAnimation(s16 animationId, s16 resourceId, s16 columnCount, s16 rowCount, s16 textureWidth, s16 textureHeight) {
    f32 textureScaleX;
    f32 textureScaleY;
    M438ParticleAnimation* particleAnimation;
    ANIMDATA* animationData;

    particleAnimation = &fireManager.animations[animationId];
    HuSprAnimKill(particleAnimation->animationData);
    particleAnimation->resourceId |= 0xFFFF0000;
    animationData = HuSprAnimMake(columnCount, rowCount, resourceId);
    animationData->bmp->data = HuMemDirectMallocNum(HEAP_HEAP, rowCount * (animationData->bmp->pixSize * columnCount), 0x10000000U);
    textureScaleX = textureWidth / (f32) animationData->bmp->sizeX;
    textureScaleY = textureHeight / (f32) animationData->bmp->sizeY;
    particleAnimation->animationData = animationData;
    particleAnimation->frameWidth = columnCount;
    particleAnimation->frameHeight = rowCount;
    particleAnimation->textureScaleX = textureScaleX;
    particleAnimation->textureScaleY = textureScaleY;
    particleAnimation->inverseTextureWidth = 1.0f / (f32) animationData->bmp->sizeX;
    particleAnimation->inverseTextureHeight = 1.0f / (f32) animationData->bmp->sizeY;
    particleAnimation->textureMappingMode = 0;
    particleAnimation->rowsPerFrame = animationData->bmp->sizeY / textureHeight;
    particleAnimation->isTiled = 0;
    particleAnimation->tintColor.r = particleAnimation->tintColor.g = particleAnimation->tintColor.b = 0xFF;
    particleAnimation->tintColor.a = 0xFF;
}

void ReleaseParticleAnimation(s16 animationId) {
    M438ParticleAnimation* particleAnimation;

    particleAnimation = &fireManager.animations[animationId];
    if (particleAnimation->resourceId != 0) {
        if (particleAnimation->referenceCount != 0) {
            particleAnimation->referenceCount -= 1;
        }
        if (particleAnimation->referenceCount == 0) {
            HuSprAnimKill(particleAnimation->animationData);
            particleAnimation->resourceId = 0;
        }
    }
}

u8 LoadAnimationToTextureSlot(s16 particleSystemId, u8 textureSlotIndex, u32 resourceId, s16 columnCount, s16 rowCount) {
    s32 animationIndex;

    if (particleSystemId == 0) return 0;
    
    animationIndex = LoadParticleAnimation(resourceId, columnCount, rowCount);
    if (animationIndex == 0) return 0;
    
    AssignAnimationToMaterial(particleSystemId, textureSlotIndex, animationIndex);
    return animationIndex;
}

u8 LoadAnimationToTextureSlotDefaultSize(s16 particleSystemId, u8 textureSlotIndex, u32 resourceId) {
    u8 animationIndex;

    animationIndex = LoadAnimationToTextureSlot(particleSystemId, textureSlotIndex, resourceId, 0, 0);
    
    return animationIndex;
}

s32 AssignAnimationToMaterial(s16 particleSystemId, u8 textureSlotIndex, s16 animationId) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;
    M438ParticleAnimation* particleAnimation;
    M438ParticleAnimationTrack* animationTrack;
    s32 materialIndex;

    if (animationId == 0) {
        return 0;
    }
    if (particleSystemId == 0) {
        return 0;
    }
    particleSystem = &fireManager.particleSystems[particleSystemId];
    particleSystem->stateFlags |= 2;
    if (animationId == particleSystem->animationIds[textureSlotIndex]) {
        return 1;
    }
    particleSystem->animationIds[textureSlotIndex] = animationId;
    particleAnimation = &fireManager.animations[animationId];
    
    material = particleSystem->materials;
    for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
        if (textureSlotIndex == 0) {
            material->vertexCoordinates[0].x = -0.5f * particleAnimation->frameWidth;
            material->vertexCoordinates[0].y = 0.5f * particleAnimation->frameHeight;
            material->vertexCoordinates[1].x = (0.5f * particleAnimation->frameWidth);
            material->vertexCoordinates[1].y = (0.5f * particleAnimation->frameHeight);
            material->vertexCoordinates[2].x = (0.5f * particleAnimation->frameWidth);
            material->vertexCoordinates[2].y = (-0.5f * particleAnimation->frameHeight);
            material->vertexCoordinates[3].x = (-0.5f * particleAnimation->frameWidth);
            material->vertexCoordinates[3].y = (-0.5f * particleAnimation->frameHeight);
            material->vertexCoordinates[0].z = material->vertexCoordinates[1].z = material->vertexCoordinates[2].z = material->vertexCoordinates[3].z = 0.0f;
        }
        animationTrack = &material->animationTracks[textureSlotIndex];
        animationTrack->playbackFlags = 0;
        animationTrack->frameIndex = animationTrack->previousFrameIndex = 0;
        animationTrack->frameStep = 1.0f;
        animationTrack->frameAccumulator = 0.0f;
        animationTrack->topLeftU = animationTrack->frameIndex * particleAnimation->textureScaleX;
        animationTrack->topLeftV = animationTrack->frameIndex * particleAnimation->textureScaleY;
        animationTrack->topRightU = (animationTrack->frameIndex + 1) * particleAnimation->textureScaleX;
        animationTrack->topRightV = animationTrack->frameIndex * particleAnimation->textureScaleY;
        animationTrack->bottomRightU = (animationTrack->frameIndex + 1) * particleAnimation->textureScaleX;
        animationTrack->bottomRightV = (animationTrack->frameIndex + 1) * particleAnimation->textureScaleY;
        animationTrack->bottomLeftU = (animationTrack->frameIndex * particleAnimation->textureScaleX);
        animationTrack->bottomLeftV = (animationTrack->frameIndex + 1) * particleAnimation->textureScaleY;
        if ((animationTrack->frameIndex + 1) < particleAnimation->rowsPerFrame) {
            animationTrack->bottomRightV -= particleAnimation->inverseTextureHeight;
            animationTrack->bottomLeftV -= particleAnimation->inverseTextureHeight;
        }
        DCFlushRangeNoSync(&animationTrack->topLeftU, 0x20);
    }
    fireManager.activeParticleSystemId = particleSystemId;
    fireManager.activeTextureSlot = textureSlotIndex;
    PPCSync();
    return 1;
}

void ShutdownFireManager(void) {
    M438ParticleSystem* particleSystem;
    M438ParticleAnimation* particleAnimation2;
    M438ParticleAnimation* particleAnimation;
    s32 particleSystemIndex;
    s32 textureLayerIndex;
    s16 animationIndex;

    for (particleSystemIndex = 1; particleSystemIndex < fireManager.particleSystemCapacity; particleSystemIndex++) {
        if ((s16)particleSystemIndex != 0) {
            particleSystem = &fireManager.particleSystems[(s16)particleSystemIndex];
            if (particleSystem->stateFlags != 0) {
                if (particleSystem->vertexBuffer != NULL) {
                    HuMemDirectFree(particleSystem->vertexBuffer);
                }
                if (particleSystem->colorBuffer != NULL) {
                    HuMemDirectFree(particleSystem->colorBuffer);
                }
                if (particleSystem->normalBuffer != NULL) {
                    HuMemDirectFree(particleSystem->normalBuffer);
                }
                if (particleSystem->displayList != NULL) {
                    HuMemDirectFree(particleSystem->displayList);
                }
                if (particleSystem->animationTracks != NULL) {
                    HuMemDirectFree(particleSystem->animationTracks);
                }
                
                for (textureLayerIndex = 0; textureLayerIndex < particleSystem->animationSlotCount; textureLayerIndex++) {
                    animationIndex = particleSystem->animationIds[textureLayerIndex];
                    particleAnimation = &fireManager.animations[animationIndex];
                    if (particleAnimation->resourceId != 0) {
                        if (particleAnimation->referenceCount != 0) {
                            particleAnimation->referenceCount -= 1;
                        }
                        if (particleAnimation->referenceCount == 0) {
                            HuSprAnimKill(particleAnimation->animationData);
                            particleAnimation->resourceId = 0;
                        }
                    }
                }
                HuMemDirectFree(particleSystem->materials);
                HuMemDirectFree(particleSystem->animationIds);
                particleSystem->stateFlags = 0;
                particleSystem->systemId = 0;
            }
        }
    }
    
    for (particleSystemIndex = 1; particleSystemIndex < fireManager.animationCapacity; particleSystemIndex++) {
        particleAnimation2 = &fireManager.animations[(s16)particleSystemIndex];
        if (particleAnimation2->resourceId != 0U) {
            if (particleAnimation2->referenceCount != 0) {
                particleAnimation2->referenceCount -= 1;
            }
            if (particleAnimation2->referenceCount == 0) {
                HuSprAnimKill(particleAnimation2->animationData);
                particleAnimation2->resourceId = 0;
            }
        }
    }
    HuMemDirectFree(fireManager.particleSystems);
    HuMemDirectFree(fireManager.animations);
    Hu3DModelKill(fireManager.hookModelId);
}

void SetParticleSystemPosition(s16 particleSystemId, f32 x, f32 y, f32 z) {
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        particleSystem->position.x = x;
        particleSystem->position.y = y;
        particleSystem->position.z = z;
        if (particleSystem->materialCount == 1) {
            particleSystem->materials->position = particleSystem->position;
        }
    }
}

void SetMaterialPosition(s16 particleSystemId, s16 materialIndex, f32 x, f32 y, f32 z) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->position.x = x;
            material->position.y = y;
            material->position.z = z;
        }
    }
}

void SetAllMaterialPositions(s16 particleSystemId, f32 x, f32 y, f32 z) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;
    s32 materialIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            material->position.x = x;
            material->position.y = y;
            material->position.z = z;
        }
    }
}

void TranslateParticleSystem(s16 particleSystemId, f32 x, f32 y, f32 z) {
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        particleSystem->position.x += x;
        particleSystem->position.y += y;
        particleSystem->position.z += z;
        if (particleSystem->materialCount == 1) {
            particleSystem->materials->position = particleSystem->position;
        }
    }
}

void TranslateMaterial(s16 particleSystemId, s16 materialIndex, f32 x, f32 y, f32 z) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->position.x += x;
            material->position.y += y;
            material->position.z += z;
        }
    }
}

void TranslateAllMaterials(s16 particleSystemId, f32 x, f32 y, f32 z) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;
    s32 materialIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            material->position.x += x;
            material->position.y += y;
            material->position.z += z;
        }
    }
}

void SetMaterialSecondaryScale(s16 particleSystemId, s16 materialIndex, f32 x, f32 y, f32 z) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->secondaryScale.x = x;
            material->secondaryScale.y = y;
            material->secondaryScale.z = z;
        }
    }
}

void SetAllMaterialSecondaryScales(s16 particleSystemId, f32 x, f32 y, f32 z) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;
    s32 materialIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            material->secondaryScale.x = x;
            material->secondaryScale.y = y;
            material->secondaryScale.z = z;
        }
    }
}

void SetParticleSystemTextureRotation(s16 particleSystemId, f32 x, f32 y, f32 z) {
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        particleSystem->textureRotation.x = x;
        particleSystem->textureRotation.y = y;
        particleSystem->textureRotation.z = z;
        if (particleSystem->materialCount == 1) {
            particleSystem->materials->particleOffset = particleSystem->textureRotation;
        }
    }
}

void SetMaterialTextureRotation(s16 particleSystemId, s16 materialIndex, f32 x, f32 y, f32 z) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->particleOffset.x = x;
            material->particleOffset.y = y;
            material->particleOffset.z = z;
        }
    }
}

void SetAllMaterialTextureRotations(s16 particleSystemId, f32 x, f32 y, f32 z) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;
    s32 materialIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            material->particleOffset.x = x;
            material->particleOffset.y = y;
            material->particleOffset.z = z;
        }
    }
}

void SetParticleSystemRotation(s16 particleSystemId, f32 x, f32 y, f32 z) {
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        particleSystem->rotation.x = x;
        particleSystem->rotation.y = y;
        particleSystem->rotation.z = z;
        if (particleSystem->materialCount == 1) {
            particleSystem->materials->rotation = particleSystem->rotation;
        }
    }
}

void SetMaterialRotation(s16 particleSystemId, s16 materialIndex, f32 x, f32 y, f32 z) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->rotation.x = x;
            material->rotation.y = y;
            material->rotation.z = z;
        }
    }
}

void SetAllMaterialRotations(s16 particleSystemId, f32 x, f32 y, f32 z) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;
    s32 materialIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            material->rotation.x = x;
            material->rotation.y = y;
            material->rotation.z = z;
        }
    }
}

void SetParticleSystemScale(s16 particleSystemId, f32 x, f32 y, f32 z) {
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        particleSystem->scale.x = x;
        particleSystem->scale.y = y;
        particleSystem->scale.z = z;
        if (particleSystem->materialCount == 1) {
            particleSystem->materials->scale = particleSystem->scale;
        }
    }
}

void SetMaterialScale(s16 particleSystemId, s16 materialIndex, f32 x, f32 y, f32 z) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->scale.x = x;
            material->scale.y = y;
            material->scale.z = z;
        }
    }
}

void SetAllMaterialScales(s16 particleSystemId, f32 x, f32 y, f32 z) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;
    s32 materialIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            material->scale.x = x;
            material->scale.y = y;
            material->scale.z = z;
        }
    }
}

void EnableParticleSystemFlags(s16 particleSystemId, s32 flags) {
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        particleSystem->stateFlags |= flags;
    }
}

void EnableMaterialFlags(s16 particleSystemId, s16 materialIndex, s32 flags) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->stateFlags |= flags;
        }
    }
}

void EnableAllMaterialFlags(s16 particleSystemId, s32 flags) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;
    s32 materialIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            material->stateFlags |= flags;
        }
    }
}

void ClearParticleSystemFlags(s16 particleSystemId, u32 flags) {
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        particleSystem->stateFlags &= ~flags;
    }
}

void ClearMaterialFlags(s16 particleSystemId, s16 materialIndex, u32 flags) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if ((s16) particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->stateFlags &= ~flags;
        }
    }
}

void ClearAllMaterialFlags(s16 particleSystemId, u32 flags) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;
    s32 materialIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            material->stateFlags &= ~flags;
        }
    }
}

s32 ParticleSystemHasFlags(s16 particleSystemId, s32 flags) {
    M438ParticleSystem* particleSystem;
    
    if (particleSystemId == 0) {
        return 0;
    }

    particleSystem = &fireManager.particleSystems[particleSystemId];
    if ((particleSystem->stateFlags & flags) != 0) {
        return 1;
    }
    return 0;
}

s16 MaterialHasFlags(s16 particleSystemId, s16 materialIndex, s32 flags) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            if ((material->stateFlags & flags) != 0) {
                return 1;
            }
            particleSystemId = 0;
        }
    }
    return particleSystemId;
}

void SetMaterialColor(s16 particleSystemId, s16 materialIndex, u8 red, u8 green, u8 blue, u8 alpha) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->modulationColor.r = red;
            material->modulationColor.g = green;
            material->modulationColor.b = blue;
            material->modulationColor.a = alpha;
        }
    }
}

void SetAllMaterialColors(s16 particleSystemId, u8 red, u8 green, u8 blue, u8 alpha) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;
    s32 materialIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            material->modulationColor.r = red;
            material->modulationColor.g = green;
            material->modulationColor.b = blue;
            material->modulationColor.a = alpha;
        }
    }
}

void SetMaterialEnabled(s16 particleSystemId, s16 materialIndex, u8 enabled) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->enabled = enabled;
        }
    }
}

void SetAllMaterialsEnabled(s16 particleSystemId, u8 enabled) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;
    s32 materialIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            material->enabled = enabled;
        }
    }
}

void SetMaterialTextureCoordinateCount(s16 particleSystemId, s16 materialIndex, u8 coordinateCount) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->textureCoordinateCount = coordinateCount;
        }
    }
}

void SetAllMaterialTextureCoordinateCounts(s16 particleSystemId, u8 coordinateCount) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;
    s32 materialIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            material->textureCoordinateCount = coordinateCount;
        }
    }
}

void SetMaterialTextureMode(s16 particleSystemId, s16 materialIndex, u8 textureMode) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->blendMode = textureMode;
        }
    }
}

void SetAllMaterialTextureModes(s16 particleSystemId, u8 textureMode) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;
    s32 materialIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            material->blendMode = textureMode;
        }
    }
}

void SetMaterialDefaultAnimationFlags(s16 particleSystemId, s16 materialIndex, u8 animationFlags) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->animationTracks[0].playbackFlags = animationFlags;
        }
    }
}

void SetMaterialAnimationFlags(s16 particleSystemId, s16 materialIndex, u8 textureSlotIndex, u8 animationFlags) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->animationTracks[textureSlotIndex].playbackFlags = animationFlags;
        }
    }
}

void SetAllMaterialAnimationFlags(s16 particleSystemId, u8 animationFlags) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;
    s32 materialIndex;
    s32 textureLayerIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = (M438FireMaterial*)&particleSystem->materials; //! bug
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            for (textureLayerIndex = 0; textureLayerIndex < material->textureSlotCount; textureLayerIndex++) {
                material->animationTracks[textureLayerIndex].playbackFlags = animationFlags;
            }
        }
    }
}

void SetMaterialDefaultAnimationSpeed(s16 particleSystemId, s16 materialIndex, f32 x) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->animationTracks[0].frameStep = x;
        }
    }
}

void SetMaterialAnimationSpeed(s16 particleSystemId, s16 materialIndex, u8 textureSlotIndex, f32 x) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->animationTracks[textureSlotIndex].frameStep = x;
        }
    }
}

void SetAllMaterialAnimationSpeeds(s16 particleSystemId, f32 x) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;
    s32 materialIndex;
    s32 textureLayerIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            for (textureLayerIndex = 0; textureLayerIndex < material->textureSlotCount; textureLayerIndex++) {
                material->animationTracks[textureLayerIndex].frameStep = x;
            }
        }
    }
}

void SetMaterialDefaultAnimationFrame(s16 particleSystemId, s16 materialIndex, u8 animationFrame) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->animationTracks[0].frameIndex = animationFrame;
        }
    }
}

void SetMaterialAnimationFrame(s16 particleSystemId, s16 materialIndex, u8 textureSlotIndex, u8 animationFrame) {
    M438ParticleSystem* particleSystem;
    M438FireMaterial* material;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        if (particleSystem->materialCount > materialIndex) {
            material = &particleSystem->materials[materialIndex];
            material->animationTracks[textureSlotIndex].frameIndex = animationFrame;
        }
    }
}

void SetAllMaterialAnimationFrames(s16 particleSystemId, u8 animationFrame) {
    M438FireMaterial* material;
    M438ParticleSystem* particleSystem;
    s32 materialIndex;
    s32 textureLayerIndex;

    if (particleSystemId != 0) {
        particleSystem = &fireManager.particleSystems[particleSystemId];
        material = particleSystem->materials;
        
        for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
            for (textureLayerIndex = 0; textureLayerIndex < material->textureSlotCount; textureLayerIndex++) {
                material->animationTracks[textureLayerIndex].frameIndex = animationFrame;
            }
        }
    }
}

void SetParticleAnimationTextureMode(s16 animationId, u8 textureMode) {
    M438ParticleAnimation* particleAnimation;
    
    if (animationId != 0) {
        particleAnimation = &fireManager.animations[animationId];
        particleAnimation->textureMappingMode = textureMode;
    }
}
