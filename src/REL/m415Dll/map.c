#include "game/memory.h"
#define HUSPR_USE_OLD_DEFS
#include "REL/m415Dll.h"

#include "game/hu3d.h"
#include "game/sprite.h"

#include <string.h>

void MapRendererInitialize(s16 objectCapacity, s16 textureCapacity)
{
    s32 objectIndex;
    MapObject *mapObject;
    MapTexture *mapTexture;

    mapRenderer.hookModelId = Hu3DHookFuncCreate(MapRendererDrawHook);
    Hu3DModelLayerSet(mapRenderer.hookModelId, 6);
    mapRenderer.objectCapacity = objectCapacity;
    mapRenderer.objects = mapObject = HuMemDirectMallocNum(HEAP_HEAP, objectCapacity * sizeof(MapObject), HU_MEMNUM_OVL);
    mapRenderer.textureCapacity = textureCapacity;
    mapRenderer.textures = mapTexture = HuMemDirectMallocNum(HEAP_HEAP, textureCapacity * sizeof(MapTexture), HU_MEMNUM_OVL);

    for (objectIndex = 0; objectIndex < objectCapacity; objectIndex++, mapObject++) {
        mapObject->flags = 0;
    }

    for (objectIndex = 0; objectIndex < textureCapacity; objectIndex++, mapTexture++) {
        mapTexture->referenceCount = 0;
        mapTexture->resourceId = 0;
    }
    mapRenderer.activeObjectId = mapRenderer.activeMeshId = mapRenderer.activeTextureSlot = 0;
}

void MapObjectDraw(MapObject *mapObject)
{
    HU3DMODEL lightModel;
    Mtx meshModelViewMatrix;
    Mtx cameraModelMatrix;
    Mtx modelMatrix;
    Mtx rotationMatrix;
    Mtx textureMatrix;
    GXColor tevColor;
    s16 shadowTextureMap;
    s32 channelControlSource;
    s32 textureSlot;
    s16 lightId;
    s16 texCoordGeneratorCount;
    s16 tevStageCount;
    s32 meshIndex;
    s32 textureMatrixId0;
    s32 textureMatrixId1;
    s32 textureMatrixId2;
    s32 textureMatrixId3;
    MapTexture *mapTexture;
    MapMesh *mapMesh;

    mtxRot(rotationMatrix, mapObject->rotation.x, mapObject->rotation.y, mapObject->rotation.z);
    MTXScale(modelMatrix, mapObject->scale.x, mapObject->scale.y, mapObject->scale.z);
    MTXConcat(rotationMatrix, modelMatrix, modelMatrix);
    mtxTransCat(modelMatrix, mapObject->position.x, mapObject->position.y, mapObject->position.z);
    MTXConcat(Hu3DCameraMtx, modelMatrix, cameraModelMatrix);
    tevStageCount = 0;
    texCoordGeneratorCount = 0;
    shadowTextureMap = 0;

    for (textureSlot = GX_TEXMAP0; textureSlot < mapObject->textureSlotCount; textureSlot++) {
        if (mapObject->textureIds[textureSlot] != 0) {
            mapTexture = &mapRenderer.textures[mapObject->textureIds[textureSlot]];
            MTXCopy(mapTexture->textureMatrix, textureMatrix);
            mtxTransCat(textureMatrix, mapTexture->translation.x, mapTexture->translation.y, mapTexture->translation.z);
            switch (mapTexture->frame) {
                case 0:
                    GXLoadTexMtxImm(textureMatrix, texMtxTbl[texCoordGeneratorCount], GX_MTX2x4);
                    textureMatrixId0 = texMtxTbl[texCoordGeneratorCount];
                    GXSetTexCoordGen2(texCoordGeneratorCount, GX_TG_MTX2x4, textureSlot + 4, textureMatrixId0, 0, 0x7D);
                    GXSetTevOrder(tevStageCount, texCoordGeneratorCount, textureSlot, GX_COLOR0A0);
                    if (textureSlot == GX_TEXMAP0) {
                        if ((mapTexture->animation->bmp->dataFmt == 7) || (mapTexture->animation->bmp->dataFmt == 8)) {
                            GXSetTevColor(GX_TEVREG2, mapTexture->color);
                            GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC, GX_CC_ZERO);
                        }
                        else {
                            GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
                        }
                        GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
                    }
                    else {
                        tevColor.a = mapTexture->color.a;
                        GXSetTevColor(GX_TEVREG2, tevColor);
                        GXSetTevColorIn(tevStageCount, GX_CC_CPREV, GX_CC_TEXC, GX_CC_A2, GX_CC_ZERO);
                        GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
                    }
                    GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    break;
                case 1:
                    GXLoadTexMtxImm(textureMatrix, texMtxTbl[texCoordGeneratorCount], GX_MTX2x4);
                    textureMatrixId1 = texMtxTbl[texCoordGeneratorCount];
                    GXSetTexCoordGen2(texCoordGeneratorCount, GX_TG_MTX2x4, textureSlot + 4, textureMatrixId1, 0, 0x7D);
                    GXSetTevOrder(tevStageCount, texCoordGeneratorCount, textureSlot, GX_COLOR0A0);
                    GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_CPREV, GX_CC_TEXA, GX_CC_ZERO);
                    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
                    GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    break;
                case 2:
                    tevColor.a = 0xE0;
                    GXSetTevColor(GX_TEVREG1, tevColor);
                    GXSetTexCoordGen2(texCoordGeneratorCount + 1, GX_TG_BUMP0, texCoordGeneratorCount, 0x3C, 0, 0x7D);
                    GXSetTevOrder(tevStageCount, texCoordGeneratorCount, textureSlot, GX_COLOR0A0);
                    GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A1, GX_CC_RASC);
                    GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
                    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 0, GX_TEVPREV);
                    tevStageCount++;
                    GXSetTevOrder(tevStageCount, texCoordGeneratorCount + 1, GX_TEXMAP0, GX_COLOR0A0);
                    GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A1, GX_CC_CPREV);
                    GXSetTevColorOp(tevStageCount, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
                    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 0, GX_TEVPREV);
                    break;
                case 3:
                    texCoordGeneratorCount--;
                    GXLoadTexMtxImm(textureMatrix, texMtxTbl[texCoordGeneratorCount], GX_MTX2x4);
                    textureMatrixId2 = texMtxTbl[texCoordGeneratorCount];
                    GXSetTexCoordGen2(texCoordGeneratorCount, GX_TG_MTX2x4, textureSlot + 4, textureMatrixId2, 0, 0x7D);
                    GXSetTevOrder(tevStageCount, texCoordGeneratorCount, textureSlot, GX_COLOR0A0);
                    GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_CPREV, GX_CC_TEXC, GX_CC_ZERO);
                    GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
                    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    texCoordGeneratorCount++;
                    break;
                case 4:
                    mtxRot(rotationMatrix, mapObject->rotation.x, mapObject->rotation.y, mapObject->rotation.z);
                    MTXScale(modelMatrix, mapObject->scale.x, mapObject->scale.y, mapObject->scale.z);
                    MTXConcat(rotationMatrix, modelMatrix, modelMatrix);
                    mtxTransCat(modelMatrix, mapObject->position.x, mapObject->position.y, mapObject->position.z);
                    MTXConcat(Hu3DCameraMtx, modelMatrix, textureMatrix);
                    MTXInverse(Hu3DCameraMtx, rotationMatrix);
                    MTXConcat(rotationMatrix, textureMatrix, rotationMatrix);
                    MTXConcat(Hu3DShadowData.projMtx, Hu3DShadowData.lookAtMtx, modelMatrix);
                    MTXConcat(modelMatrix, rotationMatrix, textureMatrix);
                    GXLoadTexMtxImm(textureMatrix, texMtxTbl[texCoordGeneratorCount], GX_MTX3x4);
                    textureMatrixId3 = texMtxTbl[texCoordGeneratorCount];
                    GXSetTexCoordGen2(texCoordGeneratorCount, GX_TG_MTX3x4, GX_TG_POS, textureMatrixId3, 0, 0x7D);
                    GXSetTevOrder(tevStageCount, texCoordGeneratorCount, textureSlot, GX_COLOR0A0);
                    if (textureSlot == GX_TEXMAP0) {
                        GXSetTevColorIn(tevStageCount, GX_CC_ONE, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ZERO);
                        GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
                    }
                    else {
                        GXSetTevColorIn(tevStageCount, GX_CC_CPREV, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ZERO);
                        GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
                    }
                    GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
                    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 0, GX_TEVPREV);
                    break;
            }
            tevStageCount++;
            texCoordGeneratorCount++;
            if (mapTexture->scaledToFit != 0) {
                HuSprTexLoad(mapTexture->animation, 0, textureSlot, 1, 1, 1);
            }
            else {
                HuSprTexLoad(mapTexture->animation, 0, textureSlot, 0, 0, 1);
            }
        }
    }
    GXSetNumTevStages((s16)tevStageCount);
    GXSetNumTexGens((s16)texCoordGeneratorCount);
    mapMesh = mapObject->meshes;

    for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++, mapMesh++) {
        if ((mapMesh->flags != 0) && ((mapMesh->flags & 4) == 0)) {
            switch (mapMesh->renderMode) {
                case 0:
                    mtxRot(rotationMatrix, mapMesh->rotation.x, mapMesh->rotation.y, mapMesh->rotation.z);
                    if (mapMesh->billboardMode == 2) {
                        MTXConcat(mapRenderer.cameraMatrix, rotationMatrix, rotationMatrix);
                    }
                    else if (mapMesh->billboardMode == 1) {
                        MTXRotDeg(modelMatrix, 0x59, CRot.y);
                        MTXConcat(modelMatrix, rotationMatrix, rotationMatrix);
                    }
                    MTXScale(modelMatrix, mapMesh->scale.x, mapMesh->scale.y, mapMesh->scale.z);
                    MTXConcat(rotationMatrix, modelMatrix, modelMatrix);
                    mtxTransCat(modelMatrix, mapMesh->position.x, mapMesh->position.y, mapMesh->position.z);
                    MTXConcat(Hu3DCameraMtx, modelMatrix, meshModelViewMatrix);
                    break;
                case 1:
                    mtxRot(rotationMatrix, mapMesh->rotation.x, mapMesh->rotation.y, mapMesh->rotation.z);
                    if (mapMesh->billboardMode == 2) {
                        if (HmfInverseMtxF3X3(cameraModelMatrix, modelMatrix) == 0) {
                            MTXIdentity(modelMatrix);
                        }
                        MTXConcat(modelMatrix, rotationMatrix, rotationMatrix);
                    }
                    else if (mapMesh->billboardMode == 1) {
                        MTXRotDeg(modelMatrix, 0x59, CRot.y);
                        MTXConcat(modelMatrix, rotationMatrix, rotationMatrix);
                    }
                    MTXScale(modelMatrix, mapMesh->scale.x, mapMesh->scale.y, mapMesh->scale.z);
                    MTXConcat(rotationMatrix, modelMatrix, modelMatrix);
                    mtxTransCat(modelMatrix, mapMesh->position.x, mapMesh->position.y, mapMesh->position.z);
                    MTXConcat(cameraModelMatrix, modelMatrix, meshModelViewMatrix);
                    break;
            }
            GXLoadPosMtxImm(meshModelViewMatrix, 0);
            MTXInverse(meshModelViewMatrix, modelMatrix);
            MTXTranspose(modelMatrix, rotationMatrix);
            GXLoadNrmMtxImm(rotationMatrix, 0);
            GXSetChanAmbColor(GX_COLOR0A0, mapMesh->ambientColor);
            GXSetChanMatColor(GX_COLOR0A0, mapMesh->materialColor);
            GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
            GXSetZCompLoc(0);
            GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
            if ((mapMesh->flags & 0x10) != 0) {
                GXSetZMode(0, GX_LEQUAL, 1);
            }
            else {
                GXSetZMode(1, GX_LEQUAL, 0);
            }
            lightModel.attr = 0;
            lightId = Hu3DLightSet(&lightModel, &Hu3DCameraMtx, &Hu3DCameraMtxXPose, 0.0f);
            GXSetNumChans(1);
            if ((mapMesh->flags & 8) != 0) {
                channelControlSource = GX_SRC_VTX;
            }
            else {
                channelControlSource = GX_SRC_REG;
            }
            switch (mapMesh->materialFlags) {
                case 0:
                    GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, channelControlSource, 0, GX_DF_CLAMP, GX_AF_NONE);
                    break;
                case 1:
                    GXSetChanCtrl(GX_COLOR0A0, 1, GX_SRC_REG, channelControlSource, lightId, GX_DF_CLAMP, GX_AF_SPOT);
                    break;
                case 2:
                    GXSetChanCtrl(GX_COLOR0A0, 1, GX_SRC_REG, channelControlSource, lightId, GX_DF_CLAMP, GX_AF_SPEC);
                    break;
            }
            GXClearVtxDesc();
            GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
            GXSetArray(GX_VA_POS, mapMesh->vertices, 12);
            GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
            GXSetArray(GX_VA_CLR0, mapMesh->colors, 4);

            for (textureSlot = 0; textureSlot < mapMesh->textureCount; textureSlot++) {
                GXSetVtxDesc(textureSlot + GX_VA_TEX0, GX_INDEX16);
                GXSetVtxAttrFmt(GX_VTXFMT0, textureSlot + GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
                GXSetArray(textureSlot + GX_VA_TEX0, mapMesh->textureFrames[textureSlot].coordinates, 8);
            }
            GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
            GXSetArray(GX_VA_NRM, mapMesh->normals, 12);
            GXCallDisplayList(mapMesh->displayList, mapMesh->displayListSize);
        }
    }
}

void MapRendererDrawHook(HU3DMODEL *model, Mtx modelMatrix)
{
    Mtx inverseModelMatrix;
    Mtx normalMatrix;
    s32 objectIndex;
    MapObject *mapObject;

    mapObject = mapRenderer.objects;
    GXLoadPosMtxImm(modelMatrix, 0);
    MTXInverse(modelMatrix, inverseModelMatrix);
    MTXTranspose(inverseModelMatrix, normalMatrix);
    GXLoadNrmMtxImm(normalMatrix, 0);
    HmfInverseMtxF3X3(Hu3DCameraMtx, mapRenderer.cameraMatrix);

    for (objectIndex = 0; objectIndex < mapRenderer.objectCapacity; objectIndex++, mapObject++) {
        if ((mapObject->flags != 0) && ((mapObject->flags & 2) != 0) && ((mapObject->flags & 4) == 0)) {
            if ((u8)omPauseChk() == 0) {
                if (mapObject->animationCallback) {
                    mapObject->animationCallback(mapObject);
                }
                if (mapObject->flags == 0)
                    continue;
                MapObjectAnimationsUpdate(mapObject);
                if (mapObject->flags == 0)
                    continue;
            }
            MapObjectDraw(mapObject);
        }
    }
}

MapObject *MapObjectGet(s16 objectId)
{
    return &mapRenderer.objects[objectId];
}

MapMesh *MapObjectMeshGet(s16 objectId, s16 meshIndex)
{
    return &mapRenderer.objects[objectId].meshes[meshIndex];
}

ANIMDATA **mapTextureAnimationGet(s16 textureId)
{
    return &mapRenderer.textures[textureId].animation;
}

void MapQuadVerticesAllocate(MapQuadVertices **verticesOut, s16 quadCount, f32 quadWidth, f32 quadHeight)
{
    s32 textureIndex;
    MapQuadVertices *quadVertices;

    quadVertices = *verticesOut = HuMemDirectMallocNum(HEAP_HEAP, quadCount * sizeof(MapQuadVertices), HU_MEMNUM_OVL);

    for (textureIndex = 0; textureIndex < quadCount; quadVertices++, textureIndex++) {
        quadVertices->topLeft.x = (-0.5f * quadWidth);
        quadVertices->topLeft.y = (0.5f * quadHeight);
        quadVertices->topLeft.z = 0.0f;
        quadVertices->topRight.x = (0.5f * quadWidth);
        quadVertices->topRight.y = (0.5f * quadHeight);
        quadVertices->topRight.z = 0.0f;
        quadVertices->bottomRight.x = (0.5f * quadWidth);
        quadVertices->bottomRight.y = (-0.5f * quadHeight);
        quadVertices->bottomRight.z = 0.0f;
        quadVertices->bottomLeft.x = (-0.5f * quadWidth);
        quadVertices->bottomLeft.y = (-0.5f * quadHeight);
        quadVertices->bottomLeft.z = 0.0f;
    }
}

void MapVertexArrayCopy(Vec **verticesOut, s16 vertexCount, Vec *sourceVertices)
{
    s32 textureIndex;
    s32 itemIndex;
    Vec *vector;

    vector = *verticesOut = HuMemDirectMallocNum(HEAP_HEAP, vertexCount * sizeof(Vec[4]), HU_MEMNUM_OVL);

    for (textureIndex = 0; textureIndex < vertexCount; textureIndex++) {
        for (itemIndex = 0; itemIndex < 4; vector++, sourceVertices++, itemIndex++) {
            vector->x = sourceVertices->x;
            vector->y = sourceVertices->y;
            vector->z = sourceVertices->z;
        }
    }
}

void MapColorArrayCopy(GXColor **colorsOut, s16 colorCount, GXColor *sourceColors)
{
    s32 textureIndex;
    s32 itemIndex;
    GXColor *color;

    color = *colorsOut = HuMemDirectMallocNum(HEAP_HEAP, colorCount * sizeof(GXColor[4]), HU_MEMNUM_OVL);

    for (textureIndex = 0; textureIndex < colorCount; textureIndex++) {
        for (itemIndex = 0; itemIndex < 4; color++, sourceColors++, itemIndex++) {
            color->r = sourceColors->r;
            color->g = sourceColors->g;
            color->b = sourceColors->b;
            color->a = sourceColors->a;
        }
    }
}

void MapVectorArrayCopy(Vec **vectorsOut, s16 vectorCount, Vec *sourceVectors)
{
    s32 textureIndex;
    Vec *vector;

    vector = *vectorsOut = HuMemDirectMallocNum(HEAP_HEAP, vectorCount * sizeof(Vec), HU_MEMNUM_OVL);

    for (textureIndex = 0; textureIndex < vectorCount; vector++, sourceVectors++, textureIndex++) {
        vector->x = sourceVectors->x;
        vector->y = sourceVectors->y;
        vector->z = sourceVectors->z;
    }
}

void MapTexcoordArrayCopy(HuVec2f **texcoordsOut, s16 quadCount, HuVec2f *sourceTexcoords)
{
    s32 textureIndex;
    s32 itemIndex;
    HuVec2f *texcoord;

    texcoord = *texcoordsOut = HuMemDirectMallocNum(HEAP_HEAP, quadCount * sizeof(HuVec2f[4]), HU_MEMNUM_OVL);

    for (textureIndex = 0; textureIndex < quadCount; textureIndex++) {
        for (itemIndex = 0; itemIndex < 4; texcoord++, sourceTexcoords++, itemIndex++) {
            texcoord->x = sourceTexcoords->x;
            texcoord->y = sourceTexcoords->y;
        }
    }
}

void MapMeshDisplayListBuild(MapMesh *mesh)
{
    void *displayListBuffer;
    s32 objectIndex;
    s32 itemIndex;
    void *dataPointer;

    Vec defaultVertexPositions[4] = { { -50.0f, 50.0f, 0.0f }, { 50.0f, 50.0f, 0.0f }, { 50.0f, -50.0f, 0.0f }, { -50.0f, -50.0f, 0.0f } };
    GXColor defaultVertexColors[4] = { { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF } };

    for (itemIndex = 0; itemIndex < 4; itemIndex++) {
        mesh->vertices[itemIndex] = defaultVertexPositions[itemIndex];
    }

    for (itemIndex = 0; itemIndex < 4; itemIndex++) {
        mesh->colors[itemIndex] = defaultVertexColors[itemIndex];
    }
    mesh->normals->x = 0.0f;
    mesh->normals->y = 0.0f;
    mesh->normals->z = 1.0;
    dataPointer = mesh->displayList;
    displayListBuffer = dataPointer;
    GXBeginDisplayList(dataPointer, 0x100U);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    for (itemIndex = 0; itemIndex < 4; itemIndex++) {
        GXPosition1x16(itemIndex);
        GXNormal1x16(0);
        GXColor1x16(itemIndex);
        for (objectIndex = 0; objectIndex < mesh->textureCount; objectIndex++) {
            GXTexCoord1x16(itemIndex);
        }
    }
    mesh->displayListSize = GXEndDisplayList();
}

void MapObjectAnimationsUpdate(MapObject *mapObject)
{
    s32 animationState;
    s32 meshIndex2;
    s32 textureSlotIndex;
    s32 completedMeshCount;
    MapMesh *mapMesh;
    MapTexture *mapTexture;
    MapTextureFrame *textureFrame;

    completedMeshCount = 0;
    mapMesh = mapObject->meshes;

    for (meshIndex2 = 0; meshIndex2 < mapObject->meshCount; meshIndex2++, mapMesh++) {
        if ((mapMesh->flags & 4) != 0) {
            completedMeshCount++;
        }
        else {
#ifdef NON_MATCHING
            u8 count = mapMesh->textureCount;
#endif
            textureFrame = mapMesh->textureFrames;
#ifdef NON_MATCHING
            // They free mapMesh and the for loop tries to access textureCount afterwards
            for (textureSlotIndex = 0; textureSlotIndex < count; textureSlotIndex++, textureFrame++) {
#else
            for (textureSlotIndex = 0; textureSlotIndex < mapMesh->textureCount; textureSlotIndex++, textureFrame++) {
#endif
                mapTexture = &mapRenderer.textures[mapObject->textureIds[textureSlotIndex]];
                textureFrame->flags &= ~4;
                switch (textureFrame->flags & 0xF) {
                    case 1:
                        textureFrame->frameFraction += textureFrame->frameDuration;
                        if (textureFrame->frameFraction >= 1.0f) {
                            textureFrame->currentFrame++;
                            textureFrame->frameFraction--;
                        }
                        break;
                    case 2:
                        textureFrame->frameFraction += textureFrame->frameDuration;
                        if (textureFrame->frameFraction >= 1.0f) {
                            textureFrame->currentFrame--;
                            textureFrame->frameFraction--;
                        }
                        break;
                }
                if (textureFrame->currentFrame >= mapTexture->atlasRowCount) {
                    if ((textureFrame->flags & 0x10) != 0) {
                        textureFrame->flags |= 8;
                        textureFrame->currentFrame--;
                    }
                    else if ((textureFrame->flags & 0x40) != 0) {
                        textureFrame->flags = (textureFrame->flags & 0xF0) | 2;
                        textureFrame->currentFrame -= 2;
                    }
                    else if ((textureFrame->flags & 0x20) != 0) {
                        textureFrame->currentFrame = 0;
                    }
                    textureFrame->flags |= 4;
                }
                else if (textureFrame->currentFrame < 0) {
                    if ((textureFrame->flags & 0x10) != 0) {
                        textureFrame->flags |= 8;
                        textureFrame->currentFrame = 0;
                    }
                    else if ((textureFrame->flags & 0x20) != 0) {
                        if ((textureFrame->flags & 0x40) != 0) {
                            textureFrame->flags = (textureFrame->flags & 0xF0) | 1;
                            textureFrame->currentFrame = 1;
                        }
                        else {
                            textureFrame->currentFrame = mapTexture->atlasRowCount - 1;
                        }
                    }
                    else if ((textureFrame->flags & 0x40) != 0) {
                        textureFrame->flags |= 8;
                        textureFrame->currentFrame = 0;
                    }
                    textureFrame->flags |= 4;
                }
                else {
                    textureFrame->flags &= ~8;
                }
                if (textureFrame->currentFrame != textureFrame->previousFrame) {
                    textureFrame->coordinates[0].y = textureFrame->currentFrame * mapTexture->scaleY;
                    textureFrame->coordinates[1].y = textureFrame->currentFrame * mapTexture->scaleY;
                    textureFrame->coordinates[2].y = ((textureFrame->currentFrame + 1) * mapTexture->scaleY);
                    textureFrame->coordinates[3].y = ((textureFrame->currentFrame + 1) * mapTexture->scaleY);
                    if ((textureFrame->currentFrame + 1) < mapTexture->atlasRowCount) {
                        textureFrame->coordinates[2].y -= mapTexture->texelHeight;
                        textureFrame->coordinates[3].y -= mapTexture->texelHeight;
                    }
                    textureFrame->previousFrame = textureFrame->currentFrame;
                }
                if ((textureFrame->flags & 8) != 0) {
                    if ((mapMesh->flags & 0x40000000) != 0) {
                        mapMesh->flags |= 4;
                        if ((textureFrame->flags & 0x40) == 0) {
                            if (textureFrame->currentFrame <= 0) {
                                textureFrame->currentFrame = mapTexture->atlasRowCount;
                            }
                            else {
                                textureFrame->currentFrame = 0;
                            }
                        }
                    }
                    else if ((mapMesh->flags & 0x80000000) != 0) {
                        MapObjectDestroy(mapObject->objectIndex);
                    }
                    completedMeshCount++;
                }
            }
        }
    }
    if (completedMeshCount >= mapObject->meshCount) {
        if ((mapObject->flags & 0x40000000) != 0) {
            mapObject->flags |= 4;
            return;
        }
        if ((mapObject->flags & 0x80000000) != 0) {
            MapObjectDestroy(mapObject->objectIndex);
        }
    }
}

s16 MapObjectCreate(u8 meshCount, u8 textureSlotCount)
{
    s32 objectIndex2;
    s32 textureIndex;
    MapMesh *mapMesh;
    MapObject *mapObject;
    void *meshIndex2;

    mapObject = mapRenderer.objects + 1;

    for (objectIndex2 = 1; objectIndex2 < mapRenderer.objectCapacity; objectIndex2++, mapObject++) {
        if (mapObject->flags == 0) {
            break;
        }
    }
    if (objectIndex2 == mapRenderer.objectCapacity) {
        return 0;
    }
    mapObject->objectIndex = objectIndex2;
    mapObject->flags = 1;
    mapObject->animationCallback = NULL;
    mapObject->meshCount = meshCount;
    mapObject->textureSlotCount = textureSlotCount;
    mapMesh = HuMemDirectMallocNum(HEAP_HEAP, meshCount * sizeof(MapMesh), HU_MEMNUM_OVL);
    mapObject->meshes = mapMesh;
    mapObject->textureIds = HuMemDirectMallocNum(HEAP_HEAP, textureSlotCount * 2, HU_MEMNUM_OVL);
    mapObject->displayListUsed = 0;
    mapObject->displayList = HuMemDirectMallocNum(HEAP_HEAP, meshCount * sizeof(MapVertexAttributeQuads), HU_MEMNUM_OVL);
    mapObject->vertexPositions = HuMemDirectMallocNum(HEAP_HEAP, meshCount * sizeof(Vec[4]), HU_MEMNUM_OVL);
    mapObject->vertexColors = HuMemDirectMallocNum(HEAP_HEAP, meshCount * sizeof(MapVectorQuad), HU_MEMNUM_OVL);
    mapObject->vertexNormals = HuMemDirectMallocNum(HEAP_HEAP, meshCount * sizeof(Vec), HU_MEMNUM_OVL);
    mapObject->textureFrames = HuMemDirectMallocNum(HEAP_HEAP, textureSlotCount * (meshCount * sizeof(MapTextureFrame)), HU_MEMNUM_OVL);
    DCFlushRangeNoSync(mapObject->displayList, meshCount << 8);
    mapObject->position.x = mapObject->position.y = mapObject->position.z = 0.0f;
    mapObject->rotation.x = mapObject->rotation.y = mapObject->rotation.z = 0.0f;
    mapObject->scale.x = mapObject->scale.y = mapObject->scale.z = 1.0f;
    mapObject->sortRotation.x = mapObject->sortRotation.y = mapObject->sortRotation.z = 0.0f;
    mapObject->sortPosition.x = mapObject->sortPosition.y = mapObject->sortPosition.z = 0.0f;
    mapObject->renderMode = 0;

    for (objectIndex2 = 0; objectIndex2 < 0x20; objectIndex2++) {
        mapObject->renderParameters[objectIndex2] = 0;
    }
    for (objectIndex2 = 0; objectIndex2 < textureSlotCount; objectIndex2++) {
        mapObject->textureIds[objectIndex2] = 0;
    }
    for (objectIndex2 = 0; objectIndex2 < meshCount; objectIndex2++, mapMesh++) {
        mapMesh->flags = 1;
        mapMesh->textureCount = textureSlotCount;
        mapMesh->renderMode = 0;
        mapMesh->billboardMode = 2;
        mapMesh->materialFlags = 0;

        mapMesh->ambientColor.r = mapMesh->ambientColor.g = mapMesh->ambientColor.b = 0;
        mapMesh->ambientColor.a = 0xFF;
        mapMesh->materialColor.r = mapMesh->materialColor.g = mapMesh->materialColor.b = 0xFF;
        mapMesh->materialColor.a = 0xFF;
        mapMesh->textureFrames = &mapObject->textureFrames[objectIndex2 * textureSlotCount];
        for (textureIndex = 0; textureIndex < textureSlotCount; textureIndex++) {
            mapMesh->textureFrames[textureIndex].currentFrame = mapMesh->textureFrames[textureIndex].previousFrame = 0;
            mapMesh->textureFrames[textureIndex].frameDuration = 1.0f;
            mapMesh->textureFrames[textureIndex].frameFraction = 0.0f;
        }

        mapMesh->position.x = mapMesh->position.y = mapMesh->position.z = 0.0f;
        mapMesh->rotation.x = mapMesh->rotation.y = mapMesh->rotation.z = 0.0f;
        mapMesh->scale.x = mapMesh->scale.y = mapMesh->scale.z = 1.0f;
        mapMesh->rotationOffset.x = mapMesh->rotationOffset.y = mapMesh->rotationOffset.z = 0.0f;
        mapMesh->translationOffset.x = mapMesh->translationOffset.y = mapMesh->translationOffset.z = 0.0f;
        mapMesh->normal = 0;

        for (textureIndex = 0; textureIndex < 0x10; textureIndex++) {
            mapMesh->textureIds[textureIndex] = 0;
        }
        mapMesh->vertices = &mapObject->vertexPositions[objectIndex2 * 4];
        mapMesh->colors = &mapObject->vertexColors[objectIndex2 * 4];
        mapMesh->normals = &mapObject->vertexNormals[objectIndex2];
        mapMesh->displayList = mapObject->displayList + mapObject->displayListUsed;
        MapMeshDisplayListBuild(mapMesh);
        mapObject->displayListUsed += mapMesh->displayListSize;
    }
    meshIndex2 = mapObject->displayList;
    mapObject->displayList = HuMemDirectMallocNum(HEAP_HEAP, mapObject->displayListUsed, HU_MEMNUM_OVL);
    memcpy(mapObject->displayList, meshIndex2, mapObject->displayListUsed);
    HuMemDirectFree(meshIndex2);
    DCFlushRangeNoSync(mapObject->vertexPositions, meshCount * 0x30);
    DCFlushRangeNoSync(mapObject->vertexColors, meshCount * 0x10);
    DCFlushRangeNoSync(mapObject->vertexNormals, meshCount * 0xC);
    DCFlushRangeNoSync(mapObject->displayList, mapObject->displayListUsed);
    mapMesh = mapObject->meshes;
    mapObject->displayListUsed = 0;

    for (objectIndex2 = 0; objectIndex2 < meshCount; objectIndex2++, mapMesh++) {
        mapMesh->displayList = mapObject->displayList + mapObject->displayListUsed;
        mapObject->displayListUsed += mapMesh->displayListSize;
    }
    mapRenderer.activeObjectId = mapObject->objectIndex;
    mapRenderer.activeMeshId = 0;
    mapRenderer.activeTextureSlot = 0;
    return mapObject->objectIndex;
}

void MapObjectDestroy(s16 objectId)
{
    s32 textureSlot;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->flags != 0) {
            if (mapObject->vertexPositions != NULL) {
                HuMemDirectFree(mapObject->vertexPositions);
            }
            if (mapObject->vertexColors != NULL) {
                HuMemDirectFree(mapObject->vertexColors);
            }
            if (mapObject->vertexNormals != NULL) {
                HuMemDirectFree(mapObject->vertexNormals);
            }
            if (mapObject->displayList != NULL) {
                HuMemDirectFree(mapObject->displayList);
            }
            if (mapObject->textureFrames != NULL) {
                HuMemDirectFree(mapObject->textureFrames);
            }

            for (textureSlot = 0; textureSlot < mapObject->textureSlotCount; textureSlot++) {
                MapTextureAnimationRelease(mapObject->textureIds[textureSlot]);
            }
            HuMemDirectFree(mapObject->meshes);
            HuMemDirectFree(mapObject->textureIds);
            mapObject->flags = 0;
        }
    }
}

void MapRendererStateSet(s16 objectId, s16 meshIndex, u8 textureSlot)
{
    mapRenderer.activeObjectId = objectId;
    mapRenderer.activeMeshId = meshIndex;
    mapRenderer.activeTextureSlot = textureSlot;
}

s16 MapTextureAnimationLoad(u32 assetId, s16 width, s16 height)
{
    ANIMDATA *textureAnimation;
    f32 horizontalScale;
    f32 verticalScale;
    s32 textureIndex;
    MapTexture *mapTexture;

    mapTexture = mapRenderer.textures + 1;
    for (textureIndex = 1; textureIndex < mapRenderer.textureCapacity; textureIndex++, mapTexture++) {
        if ((mapTexture->referenceCount != 0) && (mapTexture->resourceId == assetId)) {
            break;
        }
    }
    if (textureIndex < mapRenderer.textureCapacity) {
        mapTexture->referenceCount++;
        return textureIndex;
    }
    mapTexture = mapRenderer.textures + 1;
    for (textureIndex = 1; textureIndex < mapRenderer.textureCapacity; textureIndex++, mapTexture++) {
        if (mapTexture->referenceCount == 0) {
            break;
        }
    }
    mapTexture->resourceId = assetId;
    mapTexture->referenceCount = 1;
    if (((assetId & 0xFFFF0000) + 0x10000) == 0U) {
        textureAnimation = HuSprAnimMake(width, height, 2);
        textureAnimation->bmp->data = HuMemDirectMallocNum(HEAP_HEAP, height * (width * 2), HU_MEMNUM_OVL);
        width = height = 0;
    }
    else {
        textureAnimation = HuSprAnimRead(HuDataReadNum(assetId, 0x10000000));
    }
    if ((width == 0) || (height == 0)) {
        width = textureAnimation->bmp->sizeX;
        height = textureAnimation->bmp->sizeY;
        horizontalScale = verticalScale = 1.0f;
    }
    else {
        horizontalScale = width / (f32)textureAnimation->bmp->sizeX;
        verticalScale = height / (f32)textureAnimation->bmp->sizeY;
    }
    mapTexture->animation = textureAnimation;
    mapTexture->width = width;
    mapTexture->height = height;
    mapTexture->scaleX = horizontalScale;
    mapTexture->scaleY = verticalScale;
    mapTexture->texelWidth = 1.0f / textureAnimation->bmp->sizeX;
    mapTexture->texelHeight = 1.0f / textureAnimation->bmp->sizeY;
    mapTexture->frame = 0;
    mapTexture->atlasRowCount = (textureAnimation->bmp->sizeY / height);
    if (mapTexture->atlasRowCount < 1) {
        mapTexture->atlasRowCount = 1;
    }
    mapTexture->scaledToFit = 0;
    if ((width > textureAnimation->bmp->sizeX) || (height > textureAnimation->bmp->sizeY)) {
        mapTexture->scaledToFit = 1;
    }
    mapTexture->color.r = mapTexture->color.g = mapTexture->color.b = 0xFF;
    mapTexture->color.a = 0xFF;
    MTXIdentity(mapTexture->textureMatrix);
    mapTexture->translation.x = mapTexture->translation.y = mapTexture->translation.z = 0.0f;
    return textureIndex;
}

void MapTextureAnimationCreate(s16 textureId, s16 textureFormat, s16 width, s16 height, s16 frameWidth, s16 frameHeight)
{
    f32 horizontalScale;
    f32 verticalScale;
    ANIMDATA *textureAnimation;
    MapTexture *mapTexture;

    mapTexture = &mapRenderer.textures[textureId];
    HuSprAnimKill(mapTexture->animation);
    mapTexture->resourceId |= 0xFFFF0000;
    textureAnimation = HuSprAnimMake(width, height, textureFormat);
    textureAnimation->bmp->data = HuMemDirectMallocNum(HEAP_HEAP, height * (textureAnimation->bmp->pixSize * width), 0x10000000U);
    horizontalScale = frameWidth / (f32)textureAnimation->bmp->sizeX;
    verticalScale = frameHeight / (f32)textureAnimation->bmp->sizeY;
    mapTexture->animation = textureAnimation;
    mapTexture->width = width;
    mapTexture->height = height;
    mapTexture->scaleX = horizontalScale;
    mapTexture->scaleY = verticalScale;
    mapTexture->texelWidth = 1.0f / textureAnimation->bmp->sizeX;
    mapTexture->texelHeight = 1.0f / textureAnimation->bmp->sizeY;
    mapTexture->frame = 0;
    mapTexture->atlasRowCount = textureAnimation->bmp->sizeY / frameHeight;
    mapTexture->scaledToFit = 0;
    mapTexture->color.r = mapTexture->color.g = mapTexture->color.b = 0xFF;
    mapTexture->color.a = 0xFF;
}

void MapTextureAnimationRelease(s16 textureId)
{
    MapTexture *mapTexture;

    mapTexture = &mapRenderer.textures[textureId];
    if (mapTexture->resourceId != 0) {
        if (mapTexture->referenceCount != 0) {
            mapTexture->referenceCount -= 1;
        }
        if (mapTexture->referenceCount == 0) {
            HuSprAnimKill(mapTexture->animation);
            mapTexture->resourceId = 0;
        }
    }
}

u8 MapObjectTextureLoad(s16 objectId, u8 textureSlot, s32 assetId, s16 width, s16 height)
{
    s32 textureId;

    textureId = MapTextureAnimationLoad(assetId, width, height);
    if (textureId == 0) {
        return 0;
    }
    MapObjectTextureSet(objectId, textureSlot, textureId);
    return textureId;
}

u8 MapObjectTextureRead(s16 objectId, u8 textureSlot, s32 assetId)
{
    u8 textureFrame2;
    s32 mapTexture3;
    u8 textureId;

    if ((mapTexture3 = MapTextureAnimationLoad(assetId, 0, 0)) == 0) {
        textureFrame2 = 0;
    }
    else {
        MapObjectTextureSet(objectId, textureSlot, mapTexture3);
        textureFrame2 = mapTexture3;
    }
    textureId = textureFrame2;
    return textureId;
}

s32 MapObjectTextureSet(s16 objectId, u8 textureSlot, s16 textureId)
{
    s32 meshIndex;
    MapMesh *mapMesh;
    MapObject *mapObject;
    MapTexture *mapTexture;
    MapTextureFrame *textureFrame;

    if (textureId == 0) {
        return 0;
    }
    if (objectId == 0) {
        return 0;
    }
    mapObject = &mapRenderer.objects[objectId];
    mapObject->flags |= 2;
    mapObject->textureIds[textureSlot] = textureId;
    mapTexture = &mapRenderer.textures[textureId];
    mapMesh = mapObject->meshes;

    for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++, mapMesh++) {
        if (textureSlot == 0) {
            mapMesh->vertices[0].x = -0.5f * mapTexture->width;
            mapMesh->vertices[0].y = 0.5f * mapTexture->height;
            mapMesh->vertices[1].x = (0.5f * mapTexture->width);
            mapMesh->vertices[1].y = (0.5f * mapTexture->height);
            mapMesh->vertices[2].x = (0.5f * mapTexture->width);
            mapMesh->vertices[2].y = (-0.5f * mapTexture->height);
            mapMesh->vertices[3].x = (-0.5f * mapTexture->width);
            mapMesh->vertices[3].y = (-0.5f * mapTexture->height);
            mapMesh->vertices[0].z = mapMesh->vertices[1].z = mapMesh->vertices[2].z = mapMesh->vertices[3].z = 0.0f;
        }
        textureFrame = &mapMesh->textureFrames[textureSlot];
        textureFrame->flags = 0;
        textureFrame->currentFrame = textureFrame->previousFrame = 0;
        textureFrame->frameDuration = 1.0f;
        textureFrame->frameFraction = 0.0f;
        textureFrame->coordinates[0].x = textureFrame->currentFrame * mapTexture->scaleX;
        textureFrame->coordinates[0].y = textureFrame->currentFrame * mapTexture->scaleY;
        textureFrame->coordinates[1].x = (textureFrame->currentFrame + 1) * mapTexture->scaleX;
        textureFrame->coordinates[1].y = textureFrame->currentFrame * mapTexture->scaleY;
        textureFrame->coordinates[2].x = (textureFrame->currentFrame + 1) * mapTexture->scaleX;
        textureFrame->coordinates[2].y = (textureFrame->currentFrame + 1) * mapTexture->scaleY;
        textureFrame->coordinates[3].x = textureFrame->currentFrame * mapTexture->scaleX;
        textureFrame->coordinates[3].y = (textureFrame->currentFrame + 1) * mapTexture->scaleY;
        if ((textureFrame->currentFrame + 1) < mapTexture->atlasRowCount) {
            textureFrame->coordinates[2].y -= mapTexture->texelHeight;
            textureFrame->coordinates[3].y -= mapTexture->texelHeight;
        }
        DCFlushRangeNoSync(textureFrame->coordinates, 0x20);
    }
    mapRenderer.activeObjectId = objectId;
    mapRenderer.activeTextureSlot = textureSlot;
    return 1;
}

void MapRendererShutdown(void)
{
    s32 objectIndex;
    s32 textureSlot;
    s16 index2;
    MapObject *mapObject;
    MapTexture *mapTexture;
    MapTexture *mapTexture2;

    for (objectIndex = 1; objectIndex < mapRenderer.objectCapacity; objectIndex++) {
        if ((s16)objectIndex != 0) {
            mapObject = &mapRenderer.objects[(s16)objectIndex];
            if (mapObject->flags != 0) {
                if (mapObject->vertexPositions != NULL) {
                    HuMemDirectFree(mapObject->vertexPositions);
                }
                if (mapObject->vertexColors != NULL) {
                    HuMemDirectFree(mapObject->vertexColors);
                }
                if (mapObject->vertexNormals != NULL) {
                    HuMemDirectFree(mapObject->vertexNormals);
                }
                if (mapObject->displayList != NULL) {
                    HuMemDirectFree(mapObject->displayList);
                }
                if (mapObject->textureFrames != NULL) {
                    HuMemDirectFree(mapObject->textureFrames);
                }

                for (textureSlot = 0; textureSlot < mapObject->textureSlotCount; textureSlot++) {
                    index2 = mapObject->textureIds[textureSlot];
                    mapTexture2 = &mapRenderer.textures[index2];
                    if (mapTexture2->resourceId != 0) {
                        if (mapTexture2->referenceCount != 0) {
                            mapTexture2->referenceCount--;
                        }
                        if (mapTexture2->referenceCount == 0) {
                            HuSprAnimKill(mapTexture2->animation);
                            mapTexture2->resourceId = 0;
                        }
                    }
                }
                HuMemDirectFree(mapObject->meshes);
                HuMemDirectFree(mapObject->textureIds);
                mapObject->flags = 0;
            }
        }
    }

    for (objectIndex = 1; objectIndex < mapRenderer.textureCapacity; objectIndex++) {
        mapTexture = &mapRenderer.textures[(s16)objectIndex];
        if (mapTexture->resourceId != 0) {
            if (mapTexture->referenceCount != 0) {
                mapTexture->referenceCount -= 1;
            }
            if (mapTexture->referenceCount == 0) {
                HuSprAnimKill(mapTexture->animation);
                mapTexture->resourceId = 0;
            }
        }
    }
    HuMemDirectFree(mapRenderer.objects);
    HuMemDirectFree(mapRenderer.textures);
    Hu3DModelKill(mapRenderer.hookModelId);
}

void MapObjectPositionSet(s16 objectId, f32 x, f32 y, f32 z)
{
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapObject->position.x = x;
        mapObject->position.y = y;
        mapObject->position.z = z;
        if (mapObject->meshCount == 1) {
            mapObject->meshes->position = mapObject->position;
        }
    }
}

void MapMeshPositionSet(s16 objectId, s16 meshIndex, f32 x, f32 y, f32 z)
{
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->position.x = x;
            mapMesh->position.y = y;
            mapMesh->position.z = z;
        }
    }
}

void MapMeshesPositionSet(s16 objectId, f32 x, f32 y, f32 z)
{
    s32 meshIndex;
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapMesh = mapObject->meshes;
        for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++, mapMesh++) {
            mapMesh->position.x = x;
            mapMesh->position.y = y;
            mapMesh->position.z = z;
        }
    }
}

void MapObjectPositionAdd(s16 objectId, f32 x, f32 y, f32 z)
{
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapObject->position.x += x;
        mapObject->position.y += y;
        mapObject->position.z += z;
        if (mapObject->meshCount == 1) {
            mapObject->meshes->position = mapObject->position;
        }
    }
}

void MapMeshPositionAdd(s16 objectId, s16 meshIndex, f32 x, f32 y, f32 z)
{
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->position.x += x;
            mapMesh->position.y += y;
            mapMesh->position.z += z;
        }
    }
}

void MapMeshesPositionAdd(s16 objectId, f32 x, f32 y, f32 z)
{
    s32 meshIndex;
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapMesh = mapObject->meshes;

        for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++, mapMesh++) {
            mapMesh->position.x += x;
            mapMesh->position.y += y;
            mapMesh->position.z += z;
        }
    }
}

void MapObjectRotationSet(s16 objectId, f32 xRotation, f32 yRotation, f32 zRotation)
{
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapObject->rotation.x = xRotation;
        mapObject->rotation.y = yRotation;
        mapObject->rotation.z = zRotation;
        if (mapObject->meshCount == 1) {
            mapObject->meshes->rotation = mapObject->rotation;
        }
    }
}

void MapMeshRotationSet(s16 objectId, s16 meshIndex, f32 xRotation, f32 yRotation, f32 zRotation)
{
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->rotation.x = xRotation;
            mapMesh->rotation.y = yRotation;
            mapMesh->rotation.z = zRotation;
        }
    }
}

void MapMeshesRotationSet(s16 objectId, f32 xRotation, f32 yRotation, f32 zRotation)
{
    s32 meshIndex;
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapMesh = mapObject->meshes;
        for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++, mapMesh++) {
            mapMesh->rotation.x = xRotation;
            mapMesh->rotation.y = yRotation;
            mapMesh->rotation.z = zRotation;
        }
    }
}

void MapObjectScaleSet(s16 objectId, f32 scaleX, f32 scaleY, f32 scaleZ)
{
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapObject->scale.x = scaleX;
        mapObject->scale.y = scaleY;
        mapObject->scale.z = scaleZ;
        if (mapObject->meshCount == 1) {
            mapObject->meshes->scale = mapObject->scale;
        }
    }
}

void MapMeshScaleSet(s16 objectId, s16 meshIndex, f32 scaleX, f32 scaleY, f32 scaleZ)
{
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->scale.x = scaleX;
            mapMesh->scale.y = scaleY;
            mapMesh->scale.z = scaleZ;
        }
    }
}

void MapMeshesScaleSet(s16 objectId, f32 scaleX, f32 scaleY, f32 scaleZ)
{
    s32 meshIndex;
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapMesh = mapObject->meshes;
        for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++, mapMesh++) {
            mapMesh->scale.x = scaleX;
            mapMesh->scale.y = scaleY;
            mapMesh->scale.z = scaleZ;
        }
    }
}

void MapObjectFlagsEnable(s16 objectId, s32 flags)
{
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapObject->flags |= flags;
    }
}

void MapMeshFlagsEnable(s16 objectId, s16 meshIndex, s32 flags)
{
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->flags |= flags;
        }
    }
}

void MapMeshesFlagsEnable(s16 objectId, s32 flags)
{
    s32 meshIndex;
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapMesh = mapObject->meshes;

        for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++) {
            mapMesh->flags |= flags;
        }
    }
}

void MapObjectFlagsDisable(s16 objectId, u32 flags)
{
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapObject->flags &= ~flags;
    }
}

void MapMeshFlagsDisable(s16 objectId, s16 meshIndex, u32 flags)
{
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->flags &= ~flags;
        }
    }
}

void MapMeshesFlagsDisable(s16 objectId, u32 flags)
{
    s32 meshIndex;
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapMesh = mapObject->meshes;

        for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++) {
            mapMesh->flags &= ~flags;
        }
    }
}

s32 MapObjectFlagCheck(s16 objectId, s32 flags)
{
    MapObject *mapObject;

    if (objectId == 0) {
        return 0;
    }

    mapObject = &mapRenderer.objects[objectId];
    if ((mapObject->flags & flags) != 0) {
        return 1;
    }
    return 0;
}

s16 MapMeshFlagCheck(s16 objectId, s16 meshIndex, s32 flags)
{
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            if ((mapMesh->flags & flags) != 0) {
                objectId = 1;
            }
            else {
                objectId = 0;
            }
        }
    }
    return objectId;
}

void MapMeshColorSet(s16 objectId, s16 meshIndex, u8 red, u8 green, u8 blue, u8 alpha)
{
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->materialColor.r = red;
            mapMesh->materialColor.g = green;
            mapMesh->materialColor.b = blue;
            mapMesh->materialColor.a = alpha;
        }
    }
}

void MapMeshesColorSet(s16 objectId, u8 red, u8 green, u8 blue, u8 alpha)
{
    s32 meshIndex;
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapMesh = mapObject->meshes;

        for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++, mapMesh++) {
            mapMesh->materialColor.r = red;
            mapMesh->materialColor.g = green;
            mapMesh->materialColor.b = blue;
            mapMesh->materialColor.a = alpha;
        }
    }
}

void MapMeshRenderModeSet(s16 objectId, s16 meshIndex, u8 renderMode)
{
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->renderMode = renderMode;
        }
    }
}

void MapMeshesRenderModeSet(s16 objectId, u8 renderMode)
{
    s32 meshIndex;
    MapObject *mapObject;
    MapMesh *mapMesh;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapMesh = mapObject->meshes;

        for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++, mapMesh++) {
            mapMesh->renderMode = renderMode;
        }
    }
}

void MapMeshBillboardModeSet(s16 objectId, s16 meshIndex, u8 billboardMode)
{
    MapObject *mapObject;
    MapMesh *mapMesh;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->billboardMode = billboardMode;
        }
    }
}

void MapMeshesBillboardModeSet(s16 objectId, u8 billboardMode)
{
    s32 meshIndex;
    MapObject *mapObject;
    MapMesh *mapMesh;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapMesh = mapObject->meshes;

        for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++, mapMesh++) {
            mapMesh->billboardMode = billboardMode;
        }
    }
}

void MapMeshTextureFlagsSet(s16 objectId, s16 meshIndex, u8 textureFlags)
{
    MapObject *mapObject;
    MapMesh *mapMesh;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->textureFrames->flags = textureFlags;
        }
    }
}

void MapMeshTextureFlagsIndexedSet(s16 objectId, s16 meshIndex, u8 textureIndex, u8 textureFlags)
{
    MapObject *mapObject;
    MapMesh *mapMesh;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->textureFrames[textureIndex].flags = textureFlags;
        }
    }
}

void MapMeshesTextureFlagsSet(s16 objectId, u8 textureFlags)
{
    s32 meshIndex;
    s32 index;
    MapMesh *mapMesh;
    MapObject *mapObject;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapMesh = (MapMesh *)&mapObject->meshes; //! BAD WHY
        (void)mapObject;

        for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++, mapMesh++) {
            for (index = 0; index < mapMesh->textureCount; index++) {
                mapMesh->textureFrames[index].flags = textureFlags;
            }
        }
    }
}

void MapMeshTextureAnimationSpeedSet(s16 objectId, s16 meshIndex, f32 frameDuration)
{
    MapObject *mapObject;
    MapMesh *mapMesh;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->textureFrames->frameDuration = frameDuration;
        }
    }
}

void MapMeshTextureAnimationSpeedIndexedSet(s16 objectId, s16 meshIndex, u8 textureIndex, f32 frameDuration)
{
    MapObject *mapObject;
    MapMesh *mapMesh;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->textureFrames[textureIndex].frameDuration = frameDuration;
        }
    }
}

void MapMeshesTextureAnimationSpeedSet(s16 objectId, f32 frameDuration)
{
    MapMesh *volatile mapMesh;
    s32 meshIndex;
    MapObject *mapObject;
    s32 index;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapMesh = mapObject->meshes;

        for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++) {
            for (index = 0; index < mapMesh->textureCount; index++) {
                mapMesh->textureFrames[index].frameDuration = frameDuration;
            }
        }
    }
}

void MapMeshTextureFrameSet(s16 objectId, s16 meshIndex, u8 frameIndex)
{
    MapObject *mapObject;
    MapMesh *mapMesh;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->textureFrames->currentFrame = frameIndex;
        }
    }
}

void MapMeshTextureFrameIndexedSet(s16 objectId, s16 meshIndex, u8 textureIndex, u8 frameIndex)
{
    MapObject *mapObject;
    MapMesh *mapMesh;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        if (mapObject->meshCount > meshIndex) {
            mapMesh = &mapObject->meshes[meshIndex];
            mapMesh->textureFrames[textureIndex].currentFrame = frameIndex;
        }
    }
}

// ...

void MapMeshesTextureFrameSet(s16 objectId, u8 frameIndex)
{
    MapMesh *volatile mapMesh;
    s32 meshIndex;
    MapObject *mapObject;
    s32 index;

    if (objectId != 0) {
        mapObject = &mapRenderer.objects[objectId];
        mapMesh = mapObject->meshes;
        for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++) {
            for (index = 0; index < mapMesh->textureCount; index++) {
                mapMesh->textureFrames[index].currentFrame = frameIndex;
            }
        }
    }
}

void MapTextureAnimationFrameSet(s16 textureId, u8 frameIndex)
{
    MapTexture *mapTexture;
    if (textureId != 0) {
        mapTexture = &mapRenderer.textures[textureId];
        mapTexture->frame = frameIndex;
    }
}
