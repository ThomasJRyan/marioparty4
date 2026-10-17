#define HUSPR_USE_OLD_DEFS
#include "REL/m409Dll.h"

#include "game/hu3d.h"
#include "game/sprite.h"

void InitializeCursorEffectSystem(s16 effectCapacity, s16 modelCapacity)
{
    s32 resourceIndex;
    s32 modelIndex;
    CursorEffectWork *effectWork;
    CursorEffectModelWork *effectModel;

    cursorEffectSystem.drawHookId = Hu3DHookFuncCreate(DrawCursorEffectModel);
    Hu3DModelLayerSet(cursorEffectSystem.drawHookId, 6);
    cursorEffectSystem.effectCapacity = effectCapacity;
    effectWork = HuMemDirectMallocNum(HEAP_HEAP, effectCapacity * sizeof(CursorEffectWork), HU_MEMNUM_OVL);
    cursorEffectSystem.effects = effectWork;
    cursorEffectSystem.modelCapacity = modelCapacity;
    effectModel = HuMemDirectMallocNum(HEAP_HEAP, modelCapacity * sizeof(CursorEffectModelWork), HU_MEMNUM_OVL);
    cursorEffectSystem.effectModels = effectModel;

    for (resourceIndex = 0; resourceIndex < effectCapacity; resourceIndex++, effectWork++) {
        effectWork->effectAttributes = 0;
    }

    for (resourceIndex = 0; resourceIndex < modelCapacity; resourceIndex++, effectModel++) {
        effectModel->isActive = 0;
        effectModel->textureId = 0;
    }

    cursorEffectSystem.activeEffectId = cursorEffectSystem.activeModelId = cursorEffectSystem.activeLayerId = 0;
}

void CleanupCursorEffectSystem(void)
{
    s32 resourceIndex;

    for (resourceIndex = 1; resourceIndex < cursorEffectSystem.effectCapacity; resourceIndex++) {
        DestroyCursorEffect(resourceIndex);
    }

    for (resourceIndex = 1; resourceIndex < cursorEffectSystem.modelCapacity; resourceIndex++) {
        DestroyCursorEffectTexture(resourceIndex);
    }
    HuMemDirectFree(cursorEffectSystem.effects);
    HuMemDirectFree(cursorEffectSystem.effectModels);
    Hu3DModelKill(cursorEffectSystem.drawHookId);
}

extern u8 texMtxTbl[];

void DrawCursorEffects(CursorEffectWork *effect)
{
    HU3DMODEL renderModel;
    Mtx layerModelMatrix;
    Mtx cameraEffectMatrix;
    Mtx workingMatrix;
    Mtx rotationMatrix;
    GXColor tevColor;
    s16 textureCoordinate;
    GXColorSrc lightColorSource;
    s32 textureLayerIndex;
    s16 lightId;
    CursorEffectModelWork *effectModel;
    s16 texcoordIndex;
    s16 tevStageCount;
    s32 effectLayerIndex;
    s32 textureMatrixId;
    s32 textureMatrixId2;
    s32 textureMatrixId3;
    CursorEffectLayerWork *effectLayer;

    mtxRot(rotationMatrix, effect->rotation.x, effect->rotation.y, effect->rotation.z);
    MTXScale(workingMatrix, effect->scale.x, effect->scale.y, effect->scale.z);
    MTXConcat(rotationMatrix, workingMatrix, workingMatrix);
    mtxTransCat(workingMatrix, effect->position.x, effect->position.y, effect->position.z);
    MTXConcat(Hu3DCameraMtx, workingMatrix, cameraEffectMatrix);
    tevStageCount = 0;
    texcoordIndex = 0;
    textureCoordinate = 0;

    for (textureLayerIndex = GX_TEXMAP0; textureLayerIndex < effect->textureLayerCount; textureLayerIndex++) {
        effectModel = &cursorEffectSystem.effectModels[effect->layerModelIds[textureLayerIndex]];
        MTXRotRad(effectModel->transform, 0x5A, 0.0f);
        mtxTransCat(effectModel->transform, effectModel->position.x, effectModel->position.y, effectModel->position.z);
        switch (effectModel->modelIndex) {
            case 0:
                GXLoadTexMtxImm(effectModel->transform, texMtxTbl[texcoordIndex], GX_MTX2x4);
                textureMatrixId = texMtxTbl[texcoordIndex];
                GXSetTexCoordGen2(texcoordIndex, GX_TG_MTX2x4, textureLayerIndex + GX_TG_TEX0, textureMatrixId, GX_FALSE, 0x7D);
                GXSetTevOrder(tevStageCount, texcoordIndex, textureLayerIndex, GX_COLOR0A0);
                if (textureLayerIndex == GX_TEXMAP0) {
                    if ((effectModel->spriteData->bmp->dataFmt == 7) || (effectModel->spriteData->bmp->dataFmt == 8)) {
                        GXSetTevColor(GX_TEVREG2, effectModel->color);
                        GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_C2, GX_CC_RASC, GX_CC_ZERO);
                    }
                    else {
                        GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
                    }
                    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
                }
                else {
                    tevColor.a = effectModel->color.a;
                    GXSetTevColor(GX_TEVREG2, tevColor);
                    GXSetTevColorIn(tevStageCount, GX_CC_CPREV, GX_CC_TEXC, GX_CC_A2, GX_CC_ZERO);
                    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
                }
                GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                break;
            case 1:
                GXLoadTexMtxImm(effectModel->transform, texMtxTbl[texcoordIndex], GX_MTX2x4);
                textureMatrixId2 = texMtxTbl[texcoordIndex];
                GXSetTexCoordGen2(texcoordIndex, GX_TG_MTX2x4, textureLayerIndex + GX_TG_TEX0, textureMatrixId2, GX_FALSE, 0x7D);
                GXSetTevOrder(tevStageCount, texcoordIndex, textureLayerIndex, GX_COLOR0A0);
                GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_CPREV, GX_CC_TEXA, GX_CC_ZERO);
                GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
                GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                break;
            case 2:
                tevColor.a = 0xE0;
                GXSetTevColor(GX_TEVREG1, tevColor);
                GXSetTexCoordGen2(texcoordIndex + GX_TEXCOORD1, GX_TG_BUMP0, texcoordIndex, 0x3C, GX_FALSE, 0x7D);
                GXSetTevOrder(tevStageCount, texcoordIndex, textureLayerIndex, GX_COLOR0A0);
                GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A1, GX_CC_RASC);
                GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
                GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
                tevStageCount++;
                GXSetTevOrder(tevStageCount, texcoordIndex + GX_TEXCOORD1, GX_TEXMAP0, GX_COLOR0A0);
                GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A1, GX_CC_CPREV);
                GXSetTevColorOp(tevStageCount, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
                GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
                break;
            case 3:
                texcoordIndex--;
                GXLoadTexMtxImm(effectModel->transform, texMtxTbl[texcoordIndex], GX_MTX2x4);
                textureMatrixId3 = texMtxTbl[texcoordIndex];
                GXSetTexCoordGen2(texcoordIndex, GX_TG_MTX2x4, textureLayerIndex + GX_TG_TEX0, textureMatrixId3, GX_FALSE, 0x7D);
                GXSetTevOrder(tevStageCount, texcoordIndex, textureLayerIndex, GX_COLOR0A0);
                GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_CPREV, GX_CC_TEXC, GX_CC_ZERO);
                GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
                GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                texcoordIndex++;
                break;
        }
        tevStageCount++;
        texcoordIndex++;
        HuSprTexLoad(effectModel->spriteData, 0, textureLayerIndex, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    }
    GXSetNumTevStages((s32)tevStageCount);
    GXSetNumTexGens((s32)texcoordIndex);
    effectLayer = effect->layers;

    for (effectLayerIndex = 0; effectLayerIndex < effect->layerCount; effectLayerIndex++, effectLayer++) {
        if ((effectLayer->layerAttributes != 0) && ((effectLayer->layerAttributes & 4) == 0)) {
            switch (effectLayer->layerFlags) {
                case 0:
                    mtxRot(rotationMatrix, effectLayer->position.x, effectLayer->position.y, effectLayer->position.z);
                    MTXIdentity(workingMatrix);
                    HmfInverseMtxF3X3(Hu3DCameraMtx, workingMatrix);
                    MTXConcat(workingMatrix, rotationMatrix, rotationMatrix);
                    MTXScale(workingMatrix, effectLayer->scale.x, effectLayer->scale.y, effectLayer->scale.z);
                    MTXConcat(rotationMatrix, workingMatrix, workingMatrix);
                    mtxTransCat(workingMatrix, effectLayer->startPosition.x, effectLayer->startPosition.y, effectLayer->startPosition.z);
                    MTXConcat(Hu3DCameraMtx, workingMatrix, layerModelMatrix);
                    break;
                case 1:
                    MTXCopy(cameraEffectMatrix, rotationMatrix);
                    mtxRotCat(rotationMatrix, effectLayer->position.x, effectLayer->position.y, effectLayer->position.z);
                    MTXScale(workingMatrix, effectLayer->scale.x, effectLayer->scale.y, effectLayer->scale.z);
                    MTXConcat(rotationMatrix, workingMatrix, workingMatrix);
                    mtxTransCat(workingMatrix, effectLayer->startPosition.x, effectLayer->startPosition.y, effectLayer->startPosition.z);
                    MTXCopy(workingMatrix, layerModelMatrix);
                    break;
            }
            GXLoadPosMtxImm(layerModelMatrix, 0);
            MTXInverse(layerModelMatrix, workingMatrix);
            MTXTranspose(workingMatrix, rotationMatrix);
            GXLoadNrmMtxImm(rotationMatrix, 0);
            GXSetChanAmbColor(GX_COLOR0A0, effectLayer->primaryColor);
            GXSetChanMatColor(GX_COLOR0A0, effectLayer->secondaryColor);
            GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
            if ((effectLayer->layerAttributes & 0x10) != 0) {
                GXSetZMode(GX_FALSE, GX_LEQUAL, GX_TRUE);
            }
            else {
                GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
            }
            renderModel.attr = 0;
            lightId = Hu3DLightSet(&renderModel, &Hu3DCameraMtx, &Hu3DCameraMtxXPose, 0.0f);
            GXSetNumChans(1);
            if ((effectLayer->layerAttributes & 8) != 0) {
                lightColorSource = GX_SRC_VTX;
            }
            else {
                lightColorSource = GX_SRC_REG;
            }
            switch (effectLayer->layerState) {
                case 0:
                    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, lightColorSource, 0, GX_DF_CLAMP, GX_AF_NONE);
                    break;
                case 1:
                    GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, lightColorSource, lightId, GX_DF_CLAMP, GX_AF_SPOT);
                    break;
                case 2:
                    GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, lightColorSource, lightId, GX_DF_CLAMP, GX_AF_SPEC);
                    break;
            }
            GXClearVtxDesc();
            GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
            GXSetArray(GX_VA_POS, effectLayer->texcoordArray, 12);
            GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
            GXSetArray(GX_VA_CLR0, effectLayer->colorArray, 4);
            for (textureLayerIndex = 0; textureLayerIndex < effectLayer->quadCount; textureLayerIndex++) {
                GXSetVtxDesc(textureLayerIndex + GX_VA_TEX0, GX_INDEX16);
                GXSetVtxAttrFmt(GX_VTXFMT0, textureLayerIndex + GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
                GXSetArray(textureLayerIndex + GX_VA_TEX0, effectLayer->colorKeys[textureLayerIndex].colorValues, 8);
            }
            GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
            GXSetArray(GX_VA_NRM, effectLayer->normalVector, 12);
            GXCallDisplayList(effectLayer->vertexArray, effectLayer->displayListSize);
        }
    }
}

void DrawCursorEffectModel(HU3DMODEL *model, Mtx matrix)
{
    Mtx inverseMatrix;
    Mtx normalMatrix;
    s32 effectIndex;
    CursorEffectWork *effect;

    effect = cursorEffectSystem.effects;
    GXLoadPosMtxImm(matrix, 0);
    MTXInverse(matrix, inverseMatrix);
    MTXTranspose(inverseMatrix, normalMatrix);
    GXLoadNrmMtxImm(normalMatrix, 0);

    for (effectIndex = 0; effectIndex < cursorEffectSystem.effectCapacity; effectIndex++, effect++) {
        if ((effect->effectAttributes == 0) || ((effect->effectAttributes & 2) == 0) || ((effect->effectAttributes & 4) != 0)) {
            continue;
        }
        if (omPauseChk() == 0) {
            if (effect->drawCallback) {
                effect->drawCallback(effect);
            }
            if (effect->effectAttributes == 0)
                continue;
            UpdateCursorEffect(effect);
            if (effect->effectAttributes == 0)
                continue;
        }
        DrawCursorEffects(effect);
    }
}

CursorEffectWork *GetCursorEffect(s16 idx)
{
    return &cursorEffectSystem.effects[idx];
}

CursorEffectLayerWork *GetCursorEffectLayer(s16 effectId, s16 layerIndex)
{
    return &cursorEffectSystem.effects[effectId].layers[layerIndex];
}

void AllocateQuadVertices(CursorEffectQuad **quadArray, s16 quadCount, F32 quadWidth, F32 quadHeight)
{
    s32 quadIndex;
    CursorEffectQuad *quad;

    quad = *quadArray = HuMemDirectMallocNum(HEAP_HEAP, quadCount * sizeof(CursorEffectQuad), HU_MEMNUM_OVL);

    for (quadIndex = 0; quadIndex < quadCount; quad++, quadIndex++) {
        quad->topLeft.x = (-0.5f * quadWidth);
        quad->topLeft.y = (0.5f * quadHeight);
        quad->topLeft.z = 0.0f;
        quad->topRight.x = (0.5f * quadWidth);
        quad->topRight.y = (0.5f * quadHeight);
        quad->topRight.z = 0.0f;
        quad->bottomRight.x = (0.5f * quadWidth);
        quad->bottomRight.y = (-0.5f * quadHeight);
        quad->bottomRight.z = 0.0f;
        quad->bottomLeft.x = (-0.5f * quadWidth);
        quad->bottomLeft.y = (-0.5f * quadHeight);
        quad->bottomLeft.z = 0.0f;
    }
}

void AllocateVectorArray(Vec **vectorArray, s16 vectorCount, Vec *sourceVectors)
{
    s32 arrayIndex;
    s32 vectorIndex;
    Vec *destinationVector;

    destinationVector = *vectorArray = HuMemDirectMallocNum(HEAP_HEAP, vectorCount * sizeof(Vec[4]), HU_MEMNUM_OVL);

    for (arrayIndex = 0; arrayIndex < vectorCount; arrayIndex++) {
        for (vectorIndex = 0; vectorIndex < 4; destinationVector++, sourceVectors++, vectorIndex++) {
            destinationVector->x = sourceVectors->x;
            destinationVector->y = sourceVectors->y;
            destinationVector->z = sourceVectors->z;
        }
    }
}

void AllocateColorArray(GXColor **colorArray, s16 colorCount, GXColor *sourceColors)
{
    s32 arrayIndex;
    s32 colorIndex;
    GXColor *destinationColor;

    destinationColor = *colorArray = HuMemDirectMallocNum(HEAP_HEAP, colorCount * sizeof(GXColor[4]), HU_MEMNUM_OVL);

    for (arrayIndex = 0; arrayIndex < colorCount; arrayIndex++) {
        for (colorIndex = 0; colorIndex < 4; destinationColor++, sourceColors++, colorIndex++) {
            destinationColor->r = sourceColors->r;
            destinationColor->g = sourceColors->g;
            destinationColor->b = sourceColors->b;
            destinationColor->a = sourceColors->a;
        }
    }
}

void AllocatePositionArray(Vec **positionArray, s16 positionCount, Vec *sourcePositions)
{
    s32 positionIndex;
    Vec *destinationPosition;

    destinationPosition = *positionArray = HuMemDirectMallocNum(HEAP_HEAP, positionCount * sizeof(Vec), HU_MEMNUM_OVL);

    for (positionIndex = 0; positionIndex < positionCount; destinationPosition++, sourcePositions++, positionIndex++) {
        destinationPosition->x = sourcePositions->x;
        destinationPosition->y = sourcePositions->y;
        destinationPosition->z = sourcePositions->z;
    }
}

void AllocateScreenVectorArray(HuVec2f **screenVectorArray, s16 vectorCount, HuVec2f *sourceVectors)
{
    s32 arrayIndex;
    s32 vectorIndex;
    HuVec2f *destinationVector;

    destinationVector = *screenVectorArray = HuMemDirectMallocNum(HEAP_HEAP, vectorCount * sizeof(HuVec2f[4]), HU_MEMNUM_OVL);

    for (arrayIndex = 0; arrayIndex < vectorCount; arrayIndex++) {
        for (vectorIndex = 0; vectorIndex < 4; destinationVector++, sourceVectors++, vectorIndex++) {
            destinationVector->x = sourceVectors->x;
            destinationVector->y = sourceVectors->y;
        }
    }
}

void InitializeCursorEffectLayer(CursorEffectLayerWork *layerWork)
{
    Vec defaultQuadPositions[4] = { { -50.0f, 50.0f, 0.0f }, { 50.0f, 50.0f, 0.0f }, { 50.0f, -50.0f, 0.0f }, { -50.0f, -50.0f, 0.0f } };
    GXColor defaultQuadColors[4] = { { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF } };
    s32 vertexIndex;
    s32 quadIndex;
    void *displayListBuffer;

    if (layerWork->texcoordArray != NULL) {
        HuMemDirectFree(layerWork->texcoordArray);
    }
    AllocateVectorArray((Vec **)&layerWork->texcoordArray, 1, &defaultQuadPositions[0]);
    if (layerWork->colorArray != NULL) {
        HuMemDirectFree(layerWork->colorArray);
    }
    AllocateColorArray((GXColor **)&layerWork->colorArray, 1, &defaultQuadColors[0]);
    if (layerWork->normalVector != NULL) {
        HuMemDirectFree(layerWork->normalVector);
    }
    layerWork->normalVector = HuMemDirectMallocNum(HEAP_HEAP, sizeof(Vec), HU_MEMNUM_OVL);
    layerWork->normalVector->x = 0.0f;
    layerWork->normalVector->y = 0.0f;
    layerWork->normalVector->z = 1.0f;
    if (layerWork->vertexArray != NULL) {
        HuMemDirectFree(layerWork->vertexArray);
    }
    displayListBuffer = HuMemDirectMallocNum(HEAP_HEAP, 0x1000, HU_MEMNUM_OVL);
    layerWork->vertexArray = displayListBuffer;
    DCFlushRange(layerWork->vertexArray, 0x1000);
    GXBeginDisplayList(displayListBuffer, 0x1000);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    for (vertexIndex = 0; vertexIndex < 4; vertexIndex++) {
        GXPosition1x16(vertexIndex);
        GXColor1x16(0);
        GXNormal1x16(vertexIndex);

        for (quadIndex = 0; quadIndex < layerWork->quadCount; quadIndex++) {
            GXTexCoord1x16(vertexIndex);
        }
    }
    layerWork->displayListSize = GXEndDisplayList();
}

void UpdateCursorEffect(CursorEffectWork *effect)
{
    s32 layerIndex;
    s32 quadIndex;
    s32 activeLayerCount;
    CursorEffectLayerWork *layerWork;
    CursorEffectColorKey *colorKey;
    CursorEffectModelWork *effectModel;

    activeLayerCount = 0;
    layerWork = effect->layers;

    for (layerIndex = 0; layerIndex < effect->layerCount; layerIndex++, layerWork++) {
        if ((layerWork->layerAttributes & 4) != 0) {
            activeLayerCount++;
        }
        else {
            colorKey = layerWork->colorKeys;

            for (quadIndex = 0; quadIndex < layerWork->quadCount; quadIndex++, colorKey++) {
                effectModel = &cursorEffectSystem.effectModels[effect->layerModelIds[quadIndex]];
                colorKey->keyCount &= ~4;
                switch (colorKey->keyCount & 0xF) {
                    case 1:
                        colorKey->keyEndTime += colorKey->keyStartTime;
                        if (colorKey->keyEndTime >= 1.0f) {
                            colorKey->keyType++;
                            colorKey->keyEndTime--;
                        }
                        break;
                    case 2:
                        colorKey->keyEndTime += colorKey->keyStartTime;
                        if (colorKey->keyEndTime >= 1.0f) {
                            colorKey->keyType--;
                            colorKey->keyEndTime--;
                        }
                        break;
                }
                if (colorKey->keyType >= effectModel->alpha) {
                    if ((colorKey->keyCount & 0x10) != 0) {
                        colorKey->keyCount |= 8;
                        colorKey->keyType--;
                    }
                    else if ((colorKey->keyCount & 0x40) != 0) {
                        colorKey->keyCount = (colorKey->keyCount & 0xF0) | 2;
                        colorKey->keyType -= 2;
                    }
                    else if ((colorKey->keyCount & 0x20) != 0) {
                        colorKey->keyType = 0;
                    }
                    colorKey->keyCount |= 4;
                }
                else if (colorKey->keyType < 0) {
                    if ((colorKey->keyCount & 0x10) != 0) {
                        colorKey->keyCount |= 8;
                        colorKey->keyType = 0;
                    }
                    else if ((colorKey->keyCount & 0x20) != 0) {
                        if ((colorKey->keyCount & 0x40) != 0) {
                            colorKey->keyCount = ((colorKey->keyCount & 0xF0) | 1);
                            colorKey->keyType = 1U;
                        }
                        else {
                            colorKey->keyType = (effectModel->alpha - 1);
                        }
                    }
                    else if ((colorKey->keyCount & 0x40) != 0) {
                        colorKey->keyCount |= 8;
                        colorKey->keyType = 0;
                    }
                    colorKey->keyCount |= 4;
                }
                else {
                    colorKey->keyCount &= ~8;
                }
                if (colorKey->keyType != colorKey->keyFlags) {
                    colorKey->colorValues[0].x = (colorKey->keyType * effectModel->textureScaleX);
                    colorKey->colorValues[0].y = (colorKey->keyType * effectModel->textureScaleY);
                    colorKey->colorValues[1].x = ((colorKey->keyType + 1) * effectModel->textureScaleX);
                    colorKey->colorValues[1].y = (colorKey->keyType * effectModel->textureScaleY);
                    colorKey->colorValues[2].x = ((colorKey->keyType + 1) * effectModel->textureScaleX);
                    colorKey->colorValues[2].y = ((colorKey->keyType + 1) * effectModel->textureScaleY);
                    colorKey->colorValues[3].x = (colorKey->keyType * effectModel->textureScaleX);
                    colorKey->colorValues[3].y = ((colorKey->keyType + 1) * effectModel->textureScaleY);
                    if ((colorKey->keyType + 1) < effectModel->alpha) {
                        colorKey->colorValues[2].y -= effectModel->textureOffsetY;
                        colorKey->colorValues[3].y -= effectModel->textureOffsetY;
                    }
                    colorKey->keyFlags = colorKey->keyType;
                }
                if ((colorKey->keyCount & 8) != 0) {
                    if ((layerWork->layerAttributes & 0x40000000) != 0) {
                        layerWork->layerAttributes |= 4;
                        if ((colorKey->keyCount & 0x40) == 0) {
                            if (colorKey->keyType <= 0) {
                                colorKey->keyType = effectModel->alpha;
                            }
                            else {
                                colorKey->keyType = 0U;
                            }
                        }
                    }
                    else if ((layerWork->layerAttributes & 0x80000000) != 0) {
                        DestroyCursorEffect(effect->effectIndex);
                    }
                    activeLayerCount++;
                }
            }
        }
    }
    if (activeLayerCount >= effect->layerCount) {
        if ((effect->effectAttributes & 0x40000000) != 0) {
            effect->effectAttributes |= 4;
            return;
        }
        if ((effect->effectAttributes & 0x80000000) != 0) {
            DestroyCursorEffect(effect->effectIndex);
        }
    }
}

s16 CreateCursorEffect(u8 layerCount, u8 textureLayerCount)
{
    s32 modelIndex;
    s32 layerIndex;
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    effectWork = &cursorEffectSystem.effects[1];

    for (modelIndex = 1; modelIndex < cursorEffectSystem.effectCapacity; modelIndex++, effectWork++) {
        if (effectWork->effectAttributes == 0)
            break;
    }

    if (modelIndex == cursorEffectSystem.effectCapacity)
        return 0;

    effectWork->effectIndex = modelIndex;
    effectWork->effectAttributes = 1;
    effectWork->drawCallback = NULL;
    effectWork->layerCount = layerCount;
    effectWork->textureLayerCount = textureLayerCount;
    layerWork = HuMemDirectMallocNum(HEAP_HEAP, layerCount * sizeof(CursorEffectLayerWork), HU_MEMNUM_OVL);
    effectWork->layers = layerWork;
    effectWork->layerModelIds = HuMemDirectMallocNum(HEAP_HEAP, textureLayerCount * sizeof(s16), HU_MEMNUM_OVL);
    effectWork->position.x = effectWork->position.y = effectWork->position.z = 0.0f;
    effectWork->rotation.x = effectWork->rotation.y = effectWork->rotation.z = 0.0f;
    effectWork->scale.x = effectWork->scale.y = effectWork->scale.z = 1.0f;
    effectWork->targetPosition.x = effectWork->targetPosition.y = effectWork->targetPosition.z = 0.0f;
    effectWork->originPosition.x = effectWork->originPosition.y = effectWork->originPosition.z = 0.0f;

    for (modelIndex = 0; modelIndex < 0x20; modelIndex++) {
        effectWork->layerState[modelIndex] = 0;
    }
    for (modelIndex = 0; modelIndex < textureLayerCount; modelIndex++) {
        effectWork->layerModelIds[modelIndex] = 0;
    }

    for (modelIndex = 0; modelIndex < layerCount; modelIndex++, layerWork++) {
        layerWork->layerAttributes = 1;
        layerWork->quadCount = textureLayerCount;
        layerWork->layerFlags = 0;
        layerWork->layerState = 0;
        layerWork->primaryColor.r = layerWork->primaryColor.g = layerWork->primaryColor.b = 0;
        layerWork->primaryColor.a = 0xFF;
        layerWork->secondaryColor.r = layerWork->secondaryColor.g = layerWork->secondaryColor.b = 0xFF;
        layerWork->secondaryColor.a = 0xFF;
        layerWork->colorKeys = HuMemDirectMallocNum(HEAP_HEAP, textureLayerCount * sizeof(CursorEffectColorKey), HU_MEMNUM_OVL);

        for (layerIndex = 0; layerIndex < textureLayerCount; layerIndex++) {
            layerWork->colorKeys[layerIndex].keyType = layerWork->colorKeys[layerIndex].keyFlags = 0;
            layerWork->colorKeys[layerIndex].colorValues = NULL;
            layerWork->colorKeys[layerIndex].keyStartTime = 1.0f;
            layerWork->colorKeys[layerIndex].keyEndTime = 0.0f;
        }
        layerWork->startPosition.x = layerWork->startPosition.y = layerWork->startPosition.z = 0.0f;
        layerWork->position.x = layerWork->position.y = layerWork->position.z = 0.0f;
        layerWork->scale.x = layerWork->scale.y = layerWork->scale.z = 1.0f;
        layerWork->scaleDelta.x = layerWork->scaleDelta.y = layerWork->scaleDelta.z = 0.0f;
        layerWork->positionDelta.x = layerWork->positionDelta.y = layerWork->positionDelta.z = 0.0f;

        for (layerIndex = 0; layerIndex < 0x10; layerIndex++) {
            layerWork->animationState[layerIndex] = 0;
        }
        layerWork->texcoordArray = layerWork->colorArray = layerWork->normalVector = NULL;
        layerWork->vertexArray = NULL;
        InitializeCursorEffectLayer(layerWork);
    }
    return effectWork->effectIndex;
}

void DestroyCursorEffect(s16 effectId)
{
    CursorEffectColorKey *colorKey;
    s32 modelIndex;
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->effectAttributes != 0) {
            layerWork = effectWork->layers;

            for (modelIndex = 0; modelIndex < effectWork->layerCount; modelIndex++, layerWork++) {
                if (layerWork->texcoordArray != NULL) {
                    HuMemDirectFree(layerWork->texcoordArray);
                }
                if (layerWork->colorArray != NULL) {
                    HuMemDirectFree(layerWork->colorArray);
                }
                if (layerWork->normalVector != NULL) {
                    HuMemDirectFree(layerWork->normalVector);
                }
                if (layerWork->vertexArray != NULL) {
                    HuMemDirectFree(layerWork->vertexArray);
                }
                colorKey = layerWork->colorKeys;
                HuMemDirectFree(colorKey->colorValues);
                HuMemDirectFree(layerWork->colorKeys);
            }

            for (modelIndex = 0; modelIndex < effectWork->textureLayerCount; modelIndex++) {
                DestroyCursorEffectTexture(effectWork->layerModelIds[modelIndex]);
            }
            HuMemDirectFree(effectWork->layers);
            HuMemDirectFree(effectWork->layerModelIds);
            effectWork->effectAttributes = 0;
            effectWork->effectIndex = 0;
        }
    }
}

void SetCursorEffectLayer(s16 effectId, s16 modelIndex, u8 layerIndex)
{
    cursorEffectSystem.activeEffectId = effectId;
    cursorEffectSystem.activeModelId = modelIndex;
    cursorEffectSystem.activeLayerId = layerIndex;
}

s16 CreateCursorEffectTexture(u32 textureAssetId, s16 textureWidth, s16 textureHeight)
{
    ANIMDATA *textureAnimation;
    F32 textureScaleY;
    F32 textureScaleX;
    s32 textureIndex;
    CursorEffectModelWork *effectModel;

    effectModel = &cursorEffectSystem.effectModels[1];

    for (textureIndex = 1; textureIndex < cursorEffectSystem.modelCapacity; textureIndex++, effectModel++) {
        if ((effectModel->isActive != 0) && (effectModel->textureId == textureAssetId))
            break;
    }
    if (textureIndex < cursorEffectSystem.modelCapacity) {
        effectModel->isActive++;
        return textureIndex;
    }
    effectModel = &cursorEffectSystem.effectModels[1];

    for (textureIndex = 1; textureIndex < cursorEffectSystem.modelCapacity; textureIndex++, effectModel++) {
        if (effectModel->isActive == 0)
            break;
    }
    effectModel->textureId = textureAssetId;
    effectModel->isActive = 1;
    textureAnimation = HuSprAnimRead(HuDataReadNum(textureAssetId, HU_MEMNUM_OVL));
    if ((textureWidth == 0) || (textureHeight == 0)) {
        textureWidth = textureAnimation->bmp->sizeX;
        textureHeight = textureAnimation->bmp->sizeY;
        textureScaleY = textureScaleX = 1.0f;
    }
    else {
        textureScaleY = textureWidth / (F32)textureAnimation->bmp->sizeX;
        textureScaleX = textureHeight / (F32)textureAnimation->bmp->sizeY;
    }
    effectModel->spriteData = textureAnimation;
    effectModel->tileWidth = textureWidth;
    effectModel->tileHeight = textureHeight;
    effectModel->textureScaleX = textureScaleY;
    effectModel->textureScaleY = textureScaleX;
    effectModel->textureOffsetX = (1.0f / textureAnimation->bmp->sizeX);
    effectModel->textureOffsetY = 1.0f / textureAnimation->bmp->sizeY;
    effectModel->modelIndex = 0;
    effectModel->alpha = (textureAnimation->bmp->sizeY / textureHeight);
    effectModel->color.r = effectModel->color.g = effectModel->color.b = 0xFF;
    effectModel->color.a = 0xFF;
    MTXIdentity(effectModel->transform);
    effectModel->position.x = effectModel->position.y = effectModel->position.z = 0.0f;
    return textureIndex;
}

void DestroyCursorEffectTexture(s16 textureId)
{
    CursorEffectModelWork *effectModel;

    effectModel = &cursorEffectSystem.effectModels[textureId];
    if (effectModel->textureId != 0) {
        if (effectModel->isActive != 0) {
            effectModel->isActive -= 1;
        }
        if (effectModel->isActive == 0) {
            HuSprAnimKill(effectModel->spriteData);
            effectModel->textureId = 0;
        }
    }
}

s32 CreateCursorEffectSprite(s16 effectId, u8 textureLayerIndex, u32 spriteAssetId, s16 spriteWidth, s16 spriteHeight)
{
    F32 textureScaleY;
    F32 textureScaleX;
    s32 layerIndex;
    s32 textureIndex;
    s32 textureModelId;
    CursorEffectColorKey *colorKey;
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;
    CursorEffectModelWork *textureWork;
    CursorEffectQuad *quad;

    if (effectId == 0) {
        return 0;
    }
    effectWork = &cursorEffectSystem.effects[effectId];
    textureModelId = CreateCursorEffectTexture(spriteAssetId, spriteWidth, spriteHeight);
    if (textureModelId == 0) {
        return 0;
    }
    effectWork->effectAttributes |= 2;
    effectWork->layerModelIds[textureLayerIndex] = textureModelId;
    textureWork = &cursorEffectSystem.effectModels[textureModelId];
    layerWork = effectWork->layers;

    for (layerIndex = 0; layerIndex < effectWork->layerCount; layerIndex++, layerWork++) {
        if (textureLayerIndex == 0) {
            if (layerWork->texcoordArray) {
                HuMemDirectFree(layerWork->texcoordArray);
            }
            textureScaleX = textureWork->tileHeight;
            textureScaleY = textureWork->tileWidth;
            quad = layerWork->texcoordArray = HuMemDirectMallocNum(HEAP_HEAP, 0x30, HU_MEMNUM_OVL);

            for (textureIndex = 0; textureIndex < 1; quad++, textureIndex++) {
                quad->topLeft.x = (-0.5f * textureScaleY);
                quad->topLeft.y = (0.5f * textureScaleX);
                quad->topLeft.z = 0.0f;
                quad->topRight.x = (0.5f * textureScaleY);
                quad->topRight.y = (0.5f * textureScaleX);
                quad->topRight.z = 0.0f;
                quad->bottomRight.x = (0.5f * textureScaleY);
                quad->bottomRight.y = (-0.5f * textureScaleX);
                quad->bottomRight.z = 0.0f;
                quad->bottomLeft.x = (-0.5f * textureScaleY);
                quad->bottomLeft.y = (-0.5f * textureScaleX);
                quad->bottomLeft.z = 0.0f;
            }
        }
        colorKey = &layerWork->colorKeys[textureLayerIndex];
        colorKey->keyCount = 0;
        colorKey->keyType = colorKey->keyFlags = 0;
        colorKey->keyStartTime = 1.0f;
        colorKey->keyEndTime = 0.0f;
        if (colorKey->colorValues == NULL) {
            colorKey->colorValues = HuMemDirectMallocNum(HEAP_HEAP, 0x20, HU_MEMNUM_OVL);
        }
        colorKey->colorValues[0].x = colorKey->keyType * textureWork->textureScaleX;
        colorKey->colorValues[0].y = colorKey->keyType * textureWork->textureScaleY;
        colorKey->colorValues[1].x = ((colorKey->keyType + 1) * textureWork->textureScaleX);
        colorKey->colorValues[1].y = (colorKey->keyType * textureWork->textureScaleY);
        colorKey->colorValues[2].x = ((colorKey->keyType + 1) * textureWork->textureScaleX);
        colorKey->colorValues[2].y = ((colorKey->keyType + 1) * textureWork->textureScaleY);
        colorKey->colorValues[3].x = (colorKey->keyType * textureWork->textureScaleX);
        colorKey->colorValues[3].y = ((colorKey->keyType + 1) * textureWork->textureScaleY);
        if ((colorKey->keyType + 1) < textureWork->alpha) {
            colorKey->colorValues[2].y -= textureWork->textureOffsetY;
            colorKey->colorValues[3].y -= textureWork->textureOffsetY;
        }
    }
    return 1;
}

s32 CreateCursorEffectSpriteSimple(s16 effectId, u8 layerIndex, u32 spriteAssetId)
{
    s32 spriteId;

    spriteId = CreateCursorEffectSprite(effectId, layerIndex, spriteAssetId, 0, 0);
    return spriteId;
}

void SetCursorEffectPosition(s16 effectId, F32 positionX, F32 positionY, F32 positionZ)
{
    CursorEffectWork *effectWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        effectWork->position.x = positionX;
        effectWork->position.y = positionY;
        effectWork->position.z = positionZ;
    }
}

void SetCursorEffectPositionAtLayer(s16 effectId, s16 layerIndex, F32 positionX, F32 positionY, F32 positionZ)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->startPosition.x = positionX;
            layerWork->startPosition.y = positionY;
            layerWork->startPosition.z = positionZ;
        }
    }
}

void SetCursorEffectPositionImmediate(s16 effectId, F32 positionX, F32 positionY, F32 positionZ)
{
    s32 layerIndex;
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        layerWork = effectWork->layers;

        for (layerIndex = 0; layerIndex < effectWork->layerCount; layerIndex++, layerWork++) {
            layerWork->startPosition.x = positionX;
            layerWork->startPosition.y = positionY;
            layerWork->startPosition.z = positionZ;
        }
    }
}

void MoveCursorEffectPosition(s16 effectId, F32 positionX, F32 positionY, F32 positionZ)
{
    CursorEffectWork *effectWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        effectWork->position.x += positionX;
        effectWork->position.y += positionY;
        effectWork->position.z += positionZ;
    }
}

void MoveCursorEffectPositionAtLayer(s16 effectId, s16 layerIndex, F32 positionX, F32 positionY, F32 positionZ)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->startPosition.x += positionX;
            layerWork->startPosition.y += positionY;
            layerWork->startPosition.z += positionZ;
        }
    }
}

void MoveCursorEffectPositionImmediate(s16 effectId, F32 positionX, F32 positionY, F32 positionZ)
{
    s32 layerIndex;
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        layerWork = effectWork->layers;

        for (layerIndex = 0; layerIndex < effectWork->layerCount; layerIndex++, layerWork++) {
            layerWork->startPosition.x += positionX;
            layerWork->startPosition.y += positionY;
            layerWork->startPosition.z += positionZ;
        }
    }
}

void SetCursorEffectScale(s16 effectId, F32 scaleX, F32 scaleY, F32 scaleZ)
{
    CursorEffectWork *effectWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        effectWork->rotation.x = scaleX;
        effectWork->rotation.y = scaleY;
        effectWork->rotation.z = scaleZ;
    }
}

void SetCursorEffectScaleAtLayer(s16 effectId, s16 layerIndex, F32 scaleX, F32 scaleY, F32 scaleZ)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->position.x = scaleX;
            layerWork->position.y = scaleY;
            layerWork->position.z = scaleZ;
        }
    }
}

void SetCursorEffectScaleImmediate(s16 effectId, F32 scaleX, F32 scaleY, F32 scaleZ)
{
    s32 layerIndex;
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        layerWork = effectWork->layers;

        for (layerIndex = 0; layerIndex < effectWork->layerCount; layerIndex++, layerWork++) {
            layerWork->position.x = scaleX;
            layerWork->position.y = scaleY;
            layerWork->position.z = scaleZ;
        }
    }
}

void SetCursorEffectDepth(s16 effectId, F32 depthX, F32 depthY, F32 depthZ)
{
    CursorEffectWork *effectWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        effectWork->scale.x = depthX;
        effectWork->scale.y = depthY;
        effectWork->scale.z = depthZ;
    }
}

void SetCursorEffectDepthAtLayer(s16 effectId, s16 layerIndex, F32 depthX, F32 depthY, F32 depthZ)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->scale.x = depthX;
            layerWork->scale.y = depthY;
            layerWork->scale.z = depthZ;
        }
    }
}

void SetCursorEffectDepthImmediate(s16 effectId, F32 depthX, F32 depthY, F32 depthZ)
{
    s32 layerIndex;
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        layerWork = effectWork->layers;

        for (layerIndex = 0; layerIndex < effectWork->layerCount; layerIndex++, layerWork++) {
            layerWork->scale.x = depthX;
            layerWork->scale.y = depthY;
            layerWork->scale.z = depthZ;
        }
    }
}

void SetCursorEffectFlags(s16 effectId, s32 attributeFlags)
{
    CursorEffectWork *effectWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        effectWork->effectAttributes |= attributeFlags;
    }
}

void SetCursorEffectLayerFlags(s16 effectId, s16 layerIndex, s32 layerFlags)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->layerAttributes |= layerFlags;
        }
    }
}

void SetCursorEffectAllFlags(s16 effectId, s32 layerFlags)
{
    s32 layerIndex;
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        layerWork = effectWork->layers;

        for (layerIndex = 0; layerIndex < effectWork->layerCount; layerIndex++) {
            layerWork->layerAttributes |= layerFlags;
        }
    }
}

void SetCursorEffectAttribute(s16 effectId, u32 attributeMask)
{
    CursorEffectWork *effectWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        effectWork->effectAttributes &= ~attributeMask;
    }
}

void SetCursorEffectLayerAttribute(s16 effectId, s16 layerIndex, u32 layerFlags)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->layerAttributes &= ~layerFlags;
        }
    }
}

void SetCursorEffectAllAttributes(s16 effectId, u32 layerAttributes)
{
    s32 layerIndex;
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        layerWork = effectWork->layers;

        for (layerIndex = 0; layerIndex < effectWork->layerCount; layerIndex++) {
            layerWork->layerAttributes &= ~layerAttributes;
        }
    }
}

s32 CheckCursorEffectFlags(s16 effectId, s32 attributeFlags)
{
    CursorEffectWork *effectWork;

    if (effectId == 0) {
        return 0;
    }

    effectWork = &cursorEffectSystem.effects[effectId];
    if ((effectWork->effectAttributes & attributeFlags) != 0) {
        return 1;
    }
    return 0;
}

s32 CheckCursorEffectLayerFlags(s16 effectId, s16 layerIndex, s32 layerFlags)
{
    CursorEffectLayerWork *layerWork;
    CursorEffectWork *effectWork;

    if (effectId == 0)
        return;

    effectWork = &cursorEffectSystem.effects[effectId];
    if (effectWork->layerCount <= layerIndex)
        return;

    layerWork = &effectWork->layers[layerIndex];
    if ((layerWork->layerAttributes & layerFlags) != 0) {
        return 1;
    }
    else {
        return 0;
    }
}

void SetCursorEffectColorAtLayer(s16 effectId, s16 layerIndex, u8 red, u8 green, u8 blue, u8 alpha)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->secondaryColor.r = red;
            layerWork->secondaryColor.g = green;
            layerWork->secondaryColor.b = blue;
            layerWork->secondaryColor.a = alpha;
        }
    }
}

void SetCursorEffectColor(s16 effectId, u8 red, u8 green, u8 blue, u8 alpha)
{
    s32 effectIndex;
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        layerWork = effectWork->layers;

        for (effectIndex = 0; effectIndex < effectWork->layerCount; effectIndex++, layerWork++) {
            layerWork->secondaryColor.r = red;
            layerWork->secondaryColor.g = green;
            layerWork->secondaryColor.b = blue;
            layerWork->secondaryColor.a = alpha;
        }
    }
}

void SetCursorEffectLayerAlpha(s16 effectId, s16 layerIndex, u8 alpha)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->colorKeys->keyCount = alpha;
        }
    }
}

void SetCursorEffectSubLayerAlpha(s16 effectId, s16 layerIndex, u8 keyIndex, u8 alpha)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->colorKeys[keyIndex].keyCount = alpha;
        }
    }
}

void SetCursorEffectAllAlpha(s16 effectId, u8 alpha)
{
    s32 layerIndex;
    s32 keyIndex;
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        layerWork = (CursorEffectLayerWork *)(&effectWork->layers); //! BAD WHY
        (void)effectWork;

        for (layerIndex = 0; layerIndex < effectWork->layerCount; layerIndex++) {
            for (keyIndex = 0; keyIndex < layerWork->quadCount; keyIndex++) {
                layerWork->colorKeys[keyIndex].keyCount = alpha;
            }
        }
    }
}

void SetCursorEffectLayerOpacity(s16 effectId, s16 layerIndex, F32 opacity)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->colorKeys->keyStartTime = opacity;
        }
    }
}

void SetCursorEffectSubLayerOpacity(s16 effectId, s16 layerIndex, u8 subLayerIndex, F32 opacity)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->colorKeys[subLayerIndex].keyStartTime = opacity;
        }
    }
}

void SetCursorEffectAllOpacity(s16 effectId, F32 opacity)
{
    CursorEffectWork *effectWork;
    s32 layerIndex;
    s32 keyIndex;
    CursorEffectLayerWork *volatile layerCursor;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        layerCursor = &effectWork->layers[0];

        for (layerIndex = 0; layerIndex < effectWork->layerCount; layerIndex++) {
            for (keyIndex = 0; keyIndex < layerCursor->quadCount; keyIndex++) {
                layerCursor->colorKeys[keyIndex].keyStartTime = opacity;
            }
        }
    }
}

void SetCursorEffectLayerPriority(s16 effectId, s16 layerIndex, u8 priority)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->colorKeys->keyType = priority;
        }
    }
}

void SetCursorEffectSubLayerPriority(s16 effectId, s16 layerIndex, u8 keyIndex, u8 priority)
{
    CursorEffectWork *effectWork;
    CursorEffectLayerWork *layerWork;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        if (effectWork->layerCount > layerIndex) {
            layerWork = &effectWork->layers[layerIndex];
            layerWork->colorKeys[keyIndex].keyType = priority;
        }
    }
}

void SetCursorEffectAllPriority(s16 effectId, u8 priority)
{
    CursorEffectLayerWork *volatile layerCursor;
    s32 layerIndex;
    CursorEffectWork *effectWork;
    s32 keyIndex;

    if (effectId != 0) {
        effectWork = &cursorEffectSystem.effects[effectId];
        layerCursor = effectWork->layers;
        for (layerIndex = 0; layerIndex < effectWork->layerCount; layerIndex++) {
            for (keyIndex = 0; keyIndex < layerCursor->quadCount; keyIndex++) {
                layerCursor->colorKeys[keyIndex].keyType = priority;
            }
        }
    }
}
