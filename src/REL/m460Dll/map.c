#include "datadir_enum.h"
#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXVert.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "string.h"

#include "REL/m460Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

typedef struct M460MapWork {
    s32 isActive;
    u32 mapState;
    s32 mapMode;
    s32 leftDoorIndex;
    s32 rightDoorIndex;
    s32 centerDoorIndex;
    s32 doorTransitionState;
    s32 doorDelayState;
    char unreferencedMapData[4];
    u32 doorDelayTimer;
    s32 isDoorOpening;
    s32 movingDoorModelId;
    u32 doorAnimationFrame;
    float doorRiseHeight;
    float doorRiseVelocity;
    Vec doorOffset;
    Vec doorVelocity;
    HSFOBJECT *doorModelObject;
    u32 textureBufferSize;
    void *textureBuffer;
} M460MapWork; /* size = 0x60 */

void M460InitializeMap(OMOBJ *object);
void M460UpdateMap(OMOBJ *object);
void M460UpdateDoorSelection(OMOBJ *object);
u32 M460WrapDoorIndex(u32 doorCount, u32 previousDoorIndex);
void M460DrawDoorOverlay(HU3DMODEL *model, Mtx mtx);
void M460DrawMapOverlay(HU3DMODEL *model, Mtx mtx);
void M460UpdateMapParticles(s32 modelId);
void M460UpdateDoorAnimation(OMOBJ *object, Vec *targetPosition, u32 animationType);

float doorSelectionCameraSettings[8][8] = {
    { 0.0f, 325.0f, -387.0f, -8.25f, 15.0f, 0.0f, 1208.0f, 38.65f },
    { 0.0f, 325.0f, -387.0f, -8.25f, -15.0f, 0.0f, 1208.0f, 38.65f },
    { 0.0f, 325.0f, -700.0f, -45.0f, 18.0f, 0.0f, 1500.0f, 41.5f },
    { 0.0f, 325.0f, -700.0f, -45.0f, 0.0f, 0.0f, 1500.0f, 41.5f },
    { 0.0f, 325.0f, -700.0f, -45.0f, -18.0f, 0.0f, 925.0f, 68.0f },
    { 0.0f, 302.0f, -783.0f, 14.25f, 28.5f, 0.0f, 833.0f, 61.0f },
    { 0.0f, 302.0f, -783.0f, 14.25f, 0.0f, 0.0f, 833.0f, 61.0f },
    { 0.0f, 302.0f, -783.0f, 14.25f, -28.5f, 0.0f, 833.0f, 61.0f },
};
float playerDoorCameraSettings[2][8] = {
    { -250.0f, 200.0f, -1000.0f, -10.0f, 0.0f, 0.0f, 1200.0f, 41.5f },
    { 250.0f, 200.0f, -1000.0f, -10.0f, 0.0f, 0.0f, 1200.0f, 41.5f },
};
s32 doorModelFileIds[3] = { DATA_MAKE_NUM(DATADIR_M460, 0x00), DATA_MAKE_NUM(DATADIR_M460, 0x01), DATA_MAKE_NUM(DATADIR_M460, 0x02) };
s32 doorOverlayFileIds[3] = { DATA_MAKE_NUM(DATADIR_M460, 0x03), DATA_MAKE_NUM(DATADIR_M460, 0x04), DATA_MAKE_NUM(DATADIR_M460, 0x05) };
s32 doorEffectFileIds[3] = { DATA_MAKE_NUM(DATADIR_M460, 0x06), DATA_MAKE_NUM(DATADIR_M460, 0x07), DATA_MAKE_NUM(DATADIR_M460, 0x08) };
s32 mapEffectFileIds[2] = { DATA_MAKE_NUM(DATADIR_M460, 0x09), DATA_MAKE_NUM(DATADIR_M460, 0x0A) };
s32 mapParticleFileIds[2] = { DATA_MAKE_NUM(DATADIR_M460, 0x0B), DATA_MAKE_NUM(DATADIR_M460, 0x0C) };

// M460MapWork
OMOBJ *mapObject;

void M460CreateMapObject(HUPROCESS *process)
{
    OMOBJ *createdMapObject = omAddObjEx(process, 0x1E, 0x10, 0, -1, M460InitializeMap);
    mapObject = createdMapObject;
    createdMapObject->work[0] = 0;
}

void M460DestroyMapObject(void) { }

void M460InitializeMap(OMOBJ *object)
{
    s32 modelIndex;
    s32 modelId;
    M460MapWork *mapWork;
    HU3DPARTICLEDATA *particleData;

    mapWork = object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M460MapWork), HU_MEMNUM_OVL);
    memset(mapWork, 0, sizeof(M460MapWork));
    omSetTra(object, 0.0f, 0.0f, 0.0f);
    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
        modelId = Hu3DModelCreateFile(doorModelFileIds[modelIndex]);
        object->mdlId[modelIndex] = modelId;
        Hu3DModelLayerSet(modelId, 1);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    }
    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
        modelId = Hu3DModelCreateFile(doorOverlayFileIds[modelIndex]);
        object->mdlId[modelIndex + 3] = modelId;
        Hu3DModelLayerSet(modelId, 1);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    }
    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
        modelId = Hu3DModelCreateFile(doorEffectFileIds[modelIndex]);
        object->mdlId[modelIndex + 6] = modelId;
        Hu3DModelLayerSet(modelId, 1);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    }
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        modelId = Hu3DModelCreateFile(mapEffectFileIds[modelIndex]);
        object->mdlId[modelIndex + 9] = modelId;
        Hu3DModelLayerSet(modelId, 1);
        Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    }
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        modelId = Hu3DModelCreateFile(mapParticleFileIds[modelIndex]);
        object->mdlId[modelIndex + 0xB] = modelId;
        Hu3DModelLayerSet(modelId, 3);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    }
    mapWork->mapMode = 8;
    M460UpdateDoorSelection(object);
    modelId = Hu3DHookFuncCreate(M460DrawDoorOverlay);
    object->mdlId[13] = modelId;
    Hu3DModelCameraSet(modelId, 1);
    Hu3DModelLayerSet(modelId, 7);
    modelId = Hu3DHookFuncCreate(M460DrawMapOverlay);
    object->mdlId[14] = modelId;
    Hu3DModelCameraSet(modelId, 2);
    Hu3DModelLayerSet(modelId, 7);
    mapWork->textureBufferSize = GXGetTexBufferSize(0x280, 0x1E0, 5, 0, 0);
    mapWork->textureBuffer = HuMemDirectMallocNum(HEAP_HEAP, mapWork->textureBufferSize, HU_MEMNUM_OVL);
    DCFlushRange(mapWork->textureBuffer, mapWork->textureBufferSize);
    modelId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M460, 0x23)), 0x100);
    object->mdlId[15] = modelId;
    Hu3DModelLayerSet(modelId, 4);
    for (particleData = ((HU3DPARTICLE *)Hu3DData[modelId].hookData)->data, modelIndex = 0; modelIndex < 0x100; modelIndex++, particleData++) {
        particleData->scale = 0.0f;
        particleData->color.a = 0;
        particleData->time = 0;
    }
    DCStoreRange(((HU3DPARTICLE *)Hu3DData[modelId].hookData)->data, 0x4400);
    mapWork->isDoorOpening = 1;
    mapWork->movingDoorModelId = -1;
    object->objFunc = M460UpdateMap;
}

void M460UpdateMap(OMOBJ *object)
{
    Vec doorPosition;
    Vec doorVelocityDelta;
    float doorInterpolation;
    M460MapWork *mapWork;
    M460CameraWork *selectionCamera;
    M460CameraWork *doorCamera;
    u32 selectionState;
    s32 previousDoorHeightBand;
    s32 currentDoorHeightBand;

    mapWork = object->data;
    selectionCamera = M460FindPrimaryCamera(1);
    doorCamera = M460FindPrimaryCamera(2);
    switch (M460GetRoundState()) {
        case 1:
            mapWork->doorTransitionState = 0;
            mapWork->doorDelayState = 0;
            mapWork->doorDelayTimer = 0;
            if (mapWork->isActive != 0) {
                M460UpdateDoorSelection(object);
                mapWork->isActive = 0;
            }
            break;
        case 3:
            selectionState = M460GetSelectionState();
            Hu3DMotionSpeedSet(object->mdlId[selectionState + 9], 1.5f);
            if (mapWork->doorTransitionState == 0) {
                if (M460IsSelectionConfirmed() != 0) {
                    mapWork->doorTransitionState = 1;
                    if (selectionState == 0) {
                        mapWork->movingDoorModelId = object->mdlId[mapWork->centerDoorIndex + 3];
                        mapWork->doorModelObject = Hu3DModelObjPtrGet(mapWork->movingDoorModelId, "door_l");
                    }
                    else if (selectionState == 1) {
                        mapWork->movingDoorModelId = object->mdlId[mapWork->rightDoorIndex + 6];
                        mapWork->doorModelObject = Hu3DModelObjPtrGet(mapWork->movingDoorModelId, "door_r");
                    }
                    mapWork->isDoorOpening = 0;
                    mapWork->doorAnimationFrame = 0;
                    Hu3DModelAttrReset(object->mdlId[selectionState + 0xB], HU3D_ATTR_DISPOFF);
                }
            }
            else if (mapWork->isDoorOpening != 0) {
                M460MarkSelectionReady();
                selectionCamera->fieldOfView -= 7.0f * REFRESH_FREQ;
            }
            if (mapWork->doorDelayState == 0) {
                mapWork->doorDelayTimer++;
                if (mapWork->doorDelayTimer > 1.7f * REFRESH_RATE_F) {
                    mapWork->doorDelayState = 1;
                    M460StartMapCameraFade(2.5f * REFRESH_FREQ);
                    selectionCamera->cameraCenter.x = playerDoorCameraSettings[selectionState][0];
                    selectionCamera->cameraCenter.y = playerDoorCameraSettings[selectionState][1];
                    selectionCamera->cameraCenter.z = playerDoorCameraSettings[selectionState][2];
                    selectionCamera->cameraRotation.x = playerDoorCameraSettings[selectionState][3];
                    selectionCamera->cameraRotation.y = playerDoorCameraSettings[selectionState][4];
                    selectionCamera->cameraRotation.z = playerDoorCameraSettings[selectionState][5];
                    selectionCamera->cameraZoom = playerDoorCameraSettings[selectionState][6];
                    selectionCamera->fieldOfView = playerDoorCameraSettings[selectionState][7];
                }
            }
            break;
        case 4:
            selectionCamera->fieldOfView -= 7.0f * REFRESH_FREQ;
            mapWork->isActive = 1;
            break;
    }
    if ((mapWork->isDoorOpening == 0) && (mapWork->movingDoorModelId >= 0)) {
        if (mapWork->doorAnimationFrame == 0) {
            mapWork->doorRiseHeight = 0.0f;
            mapWork->doorRiseVelocity = 0.0f;
            mapWork->doorOffset.x = mapWork->doorOffset.y = mapWork->doorOffset.z = 0.0f;
            mapWork->doorVelocity = mapWork->doorOffset;
            mapWork->doorRiseVelocity = 175.0f * REFRESH_FREQ;
            if (mapWork->doorModelObject) {
                doorPosition.x = mapWork->doorModelObject->mesh.base.pos.x;
                doorPosition.y = 10.0f + mapWork->doorModelObject->mesh.base.pos.y;
                doorPosition.z = 20.0f + mapWork->doorModelObject->mesh.base.pos.z;
                M460UpdateDoorAnimation(object, &doorPosition, 0);
            }
            HuAudFXPlay(0x818);
        }
        if (mapWork->doorAnimationFrame < REFRESH_RATE_F / 2.5f) {
            mapWork->doorRiseVelocity *= 0.93f;
        }
        else {
            doorInterpolation = 233.33333f * REFRESH_FREQ;
            mapWork->doorRiseVelocity = mapWork->doorRiseVelocity + (2.0f * REFRESH_FREQ * doorInterpolation);
            if (mapWork->doorRiseVelocity > doorInterpolation) {
                mapWork->doorRiseVelocity = doorInterpolation;
            }
        }
        previousDoorHeightBand = 0.5f + (0.2f * mapWork->doorRiseHeight);
        mapWork->doorRiseHeight = mapWork->doorRiseHeight + mapWork->doorRiseVelocity;
        if (mapWork->doorRiseHeight >= 350.0f) {
            mapWork->doorRiseHeight = 350.0f;
            mapWork->isDoorOpening = 1;
            HuAudFXPlay(0x819);
            if (mapWork->doorModelObject) {
                doorPosition.x = mapWork->doorModelObject->mesh.base.pos.x;
                doorPosition.y = (mapWork->doorModelObject->mesh.base.pos.y + mapWork->doorRiseHeight) - 15.000001f;
                doorPosition.z = 10.0f + mapWork->doorModelObject->mesh.base.pos.z;
                M460UpdateDoorAnimation(object, &doorPosition, 2);
            }
        }
        currentDoorHeightBand = 0.5f + (0.2f * mapWork->doorRiseHeight);
        if (previousDoorHeightBand != currentDoorHeightBand) {
            mapWork->doorVelocity.x = 100.0f * ((0.00003f * frandmod(0x3E8)) - 0.015f);
            mapWork->doorVelocity.y = 100.0f * ((0.00006f * frandmod(0x3E8)) - 0.03f);
            mapWork->doorVelocity.z = 100.0f * ((0.00006f * frandmod(0x3E8)) - 0.03f);
            if (mapWork->doorModelObject) {
                doorPosition.x = mapWork->doorModelObject->mesh.base.pos.x;
                doorPosition.y = (mapWork->doorModelObject->mesh.base.pos.y + mapWork->doorRiseHeight) - 15.000001f;
                doorPosition.z = 15.000001f + mapWork->doorModelObject->mesh.base.pos.z;
                M460UpdateDoorAnimation(object, &doorPosition, 1);
            }
        }
        VECSubtract(&mapWork->doorVelocity, &mapWork->doorOffset, &doorVelocityDelta);
        VECScale(&doorVelocityDelta, &doorVelocityDelta, 0.4f);
        VECAdd(&mapWork->doorOffset, &doorVelocityDelta, &doorPosition);
        doorPosition.y += mapWork->doorRiseHeight;
        Hu3DModelPosSetV(mapWork->movingDoorModelId, &doorPosition);
        mapWork->doorAnimationFrame++;
    }
    M460UpdateMapParticles(object->mdlId[15]);
    if (doorCamera->isCameraActive != 0) {
        doorCamera->cameraOpacity = doorCamera->cameraOpacity + doorCamera->opacityChangeRate;
        if (doorCamera->cameraOpacity > 1.0f) {
            doorCamera->isCameraActive = 0;
            doorCamera->cameraOpacity = 1.0f;
            M460SetCameraMode(1);
        }
    }
}

Vec mapDoorOrigin = { 0.0f, 0.0f, 0.0f };
GXColor doorOverlayColor = { 0, 0, 0, 0x80 };

void M460UpdateDoorSelection(OMOBJ *object)
{
    M460MapWork *mapWork;
    M460CameraWork *selectionCamera;
    u32 cameraSettingsIndex;

    mapWork = object->data;
    Hu3DModelAttrSet(object->mdlId[mapWork->leftDoorIndex], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[mapWork->rightDoorIndex + 6], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[mapWork->centerDoorIndex + 3], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[11], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[12], HU3D_ATTR_DISPOFF);
    if (mapWork->mapState != 0) {
        mapWork->leftDoorIndex = M460WrapDoorIndex(3, mapWork->leftDoorIndex);
        mapWork->centerDoorIndex = M460WrapDoorIndex(3, mapWork->centerDoorIndex);
        mapWork->rightDoorIndex = M460WrapDoorIndex(3, mapWork->rightDoorIndex);
        selectionCamera = M460FindPrimaryCamera(1);
        cameraSettingsIndex = mapWork->mapMode = M460WrapDoorIndex(8, mapWork->mapMode);
        selectionCamera->cameraCenter.x = doorSelectionCameraSettings[cameraSettingsIndex][0];
        selectionCamera->cameraCenter.y = doorSelectionCameraSettings[cameraSettingsIndex][1];
        selectionCamera->cameraCenter.z = doorSelectionCameraSettings[cameraSettingsIndex][2];
        selectionCamera->cameraRotation.x = doorSelectionCameraSettings[cameraSettingsIndex][3];
        selectionCamera->cameraRotation.y = doorSelectionCameraSettings[cameraSettingsIndex][4];
        selectionCamera->cameraRotation.z = doorSelectionCameraSettings[cameraSettingsIndex][5];
        selectionCamera->cameraZoom = doorSelectionCameraSettings[cameraSettingsIndex][6];
        selectionCamera->fieldOfView = doorSelectionCameraSettings[cameraSettingsIndex][7];
    }
    Hu3DMotionTimeSet(object->mdlId[9], 0.0f);
    Hu3DMotionSpeedSet(object->mdlId[9], 0.0f);
    Hu3DMotionTimeSet(object->mdlId[10], 0.0f);
    Hu3DMotionSpeedSet(object->mdlId[10], 0.0f);
    Hu3DModelAttrReset(object->mdlId[mapWork->leftDoorIndex], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[mapWork->centerDoorIndex + 3], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(object->mdlId[mapWork->rightDoorIndex + 6], HU3D_ATTR_DISPOFF);
    Hu3DModelPosSetV(object->mdlId[mapWork->centerDoorIndex + 3], &mapDoorOrigin);
    Hu3DModelPosSetV(object->mdlId[mapWork->rightDoorIndex + 6], &mapDoorOrigin);
    mapWork->mapState++;
}

u32 M460WrapDoorIndex(u32 doorCount, u32 previousDoorIndex)
{
    u32 doorIndex = frandmod(doorCount);
    if (doorIndex == previousDoorIndex) {
        doorIndex++;
    }
    doorIndex %= doorCount;
    return doorIndex;
}

void M460DrawDoorOverlay(HU3DMODEL *model, Mtx mtx)
{
    M460MapWork *mapWork = mapObject->data;
    GXSetTexCopySrc(0, 0, 0x280, 0x1E0);
    GXSetTexCopyDst(0x280, 0x1E0, GX_TF_RGB5A3, 0);
    GXCopyTex(mapWork->textureBuffer, 0);
    GXPixModeSync();
}

void M460DrawMapOverlay(HU3DMODEL *model, Mtx mtx)
{
    Mtx projectionMatrix;
#ifdef NON_MATCHING
    Mtx44 projectionMatrix2;
#else
    Mtx projectionMatrix2;
#endif
    GXTexObj textureObject;
    float interpolation;
    float movementSpeed;
    float blendFactor;
    M460MapWork *mapWork;
    M460CameraWork *mapCamera;

    mapWork = mapObject->data;
    mapCamera = M460FindSecondaryCamera(2);
    doorOverlayColor.a = 255.0f * mapCamera->cameraOpacity;
    DCStoreRange(&doorOverlayColor, 4);
    C_MTXPerspective(projectionMatrix2, 41.5f, 1.2f, 100.0f, 5500.0f);
    GXSetProjection(projectionMatrix2, GX_PERSPECTIVE);
    MTXIdentity(projectionMatrix);
    GXLoadPosMtxImm(projectionMatrix, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXInitTexObj(&textureObject, mapWork->textureBuffer, 0x280, 0x1E0, 5, GX_CLAMP, GX_CLAMP, 0);
    GXInitTexObjLOD(&textureObject, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
    GXLoadTexObj(&textureObject, GX_TEXMAP0);
    C_MTXLightPerspective(projectionMatrix2, 41.5f, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
    GXLoadTexMtxImm(projectionMatrix2, 0x1E, GX_MTX3x4);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, 0x1E, 0, 0x7D);
    GXSetNumTevStages(1);
    GXSetTevColor(GX_TEVREG0, doorOverlayColor);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 0, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 0, GX_TEVPREV);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
    GXSetZMode(0, GX_LEQUAL, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    interpolation = 4125.0f;
    movementSpeed = 2750.0f;
    blendFactor = 2530.0f;
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-interpolation, movementSpeed, -blendFactor);
    GXPosition3f32(interpolation, movementSpeed, -blendFactor);
    GXPosition3f32(interpolation, -movementSpeed, -blendFactor);
    GXPosition3f32(-interpolation, -movementSpeed, -blendFactor);
}

void M460UpdateMapParticles(s32 modelId)
{
    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleSystem;
    s32 particleIndex;

    particleSystem = Hu3DData[modelId].hookData;
    for (particleData = particleSystem->data, particleIndex = 0; particleIndex < particleSystem->maxCnt; particleIndex++, particleData++) {
        if (particleData->time != 0) {
            VECAdd(&particleData->pos, &particleData->vel, &particleData->pos);
            switch (particleData->parManId) {
                case 0:
                    particleData->scale += 1.0f;
                    particleData->speedDecay *= 0.99f;
                    particleData->vel.y += 70.0f * REFRESH_FREQ * REFRESH_FREQ;
                    break;
                case 1:
                    particleData->scale += 2.0f;
                    particleData->speedDecay *= 0.98f;
                    break;
                case 2:
                    particleData->scale += 1.0f;
                    particleData->speedDecay *= 0.995f;
                    break;
            }
            if (particleData->time < REFRESH_RATE_F / 5) {
                particleData->speedDecay = 0.9f * particleData->speedDecay;
            }
            particleData->color.a = particleData->speedDecay;
            if (--particleData->time == 0) {
                particleData->time = 0;
                particleData->scale = 0.0f;
                particleData->color.a = 0;
            }
        }
    }
    DCStoreRange(particleSystem->data, particleSystem->maxCnt * 0x44);
}

void M460UpdateDoorAnimation(OMOBJ *object, Vec *targetPosition, u32 animationType)
{
    Vec particleOffset;
    float particleVariation;

    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleSystem;
    s32 particleIndex;
    u32 particleCount;

    GXColor particleStartColor = { 0xA2, 0x97, 0x7B, 0x80 };
    GXColor particleEndColor = { 0xE7, 0xDD, 0xC0, 0xFF };
    s32 particleCountByEffect[3] = { 0x00000032, 4, 0x00000046 };

    particleSystem = Hu3DData[object->mdlId[15]].hookData;
    if (animationType > 2) {
        animationType = 1;
    }
    particleCount = particleCountByEffect[animationType];
    for (particleData = particleSystem->data, particleIndex = 0; particleIndex < particleSystem->maxCnt; particleIndex++, particleData++) {
        if (particleData->time == 0) {
            particleData->parManId = animationType;
            particleOffset.x = 100.0f * ((0.0026f * frandmod(0x3E8)) - 1.3f);
            particleOffset.y = 0.0f;
            particleOffset.z = 100.0f * ((0.000100000005f * frandmod(0x3E8)) - 0.05f);
            VECAdd(&particleOffset, targetPosition, &particleData->pos);
            particleData->zRot = 0.0031415902f * frandmod(0x3E8);
            switch (animationType) {
                case 0:
                    particleData->time = REFRESH_RATE_F * (0.8f + (0.0007f * frandmod(0x3E8)));
                    particleData->vel.x = 100.0f * REFRESH_FREQ * (0.000100000005f * frandmod(0x3E8) - 0.05f);
                    particleData->vel.y = 100.0f * REFRESH_FREQ * (0.05f + (0.00020000001f * frandmod(0x3E8)));
                    particleData->vel.z = 100.0f * REFRESH_FREQ * (0.1f + (0.0006f * frandmod(0x3E8)));
                    particleVariation = 0.001f * frandmod(0x3E8);
                    particleData->scale = 30.0f + (30.0f * particleVariation);
                    particleData->color.a = 40.0f + (40.0f * (1.0f - particleVariation));
                    particleData->color.r = particleStartColor.r + (particleVariation * (particleEndColor.r - particleStartColor.r));
                    particleData->color.g = particleStartColor.g + (particleVariation * (particleEndColor.g - particleStartColor.g));
                    particleData->color.b = particleStartColor.b + (particleVariation * (particleEndColor.b - particleStartColor.b));
                    break;
                case 1:
                    particleOffset.x = 100.0f * (1.1f + (0.00020000001f * frandmod(0x3E8)));
                    particleOffset.y = 100.0f * (0.2f + (0.0005f * frandmod(0x3E8)));
                    if (frandmod(0x3E8) < 0x1F4) {
                        particleOffset.x *= -1.0f;
                    }
                    VECAdd(&particleOffset, targetPosition, &particleData->pos);
                    particleData->time = REFRESH_RATE_F * (0.2f + (0.0005f * frandmod(0x3E8)));
                    particleData->vel.x = 100.0f * REFRESH_FREQ * ((0.000100000005f * frandmod(0x3E8)) - 0.05f);
                    particleData->vel.y = -100.0f * REFRESH_FREQ * (0.2f + (0.0003f * frandmod(0x3E8)));
                    particleData->vel.z = 100.0f * REFRESH_FREQ * (0.05f + (0.000100000005f * frandmod(0x3E8)));
                    particleVariation = 0.001f * frandmod(0x3E8);
                    particleData->scale = 20.0f + (30.0f * particleVariation);
                    particleData->color.a = 8.0f + (13.0f * (1.0f - particleVariation));
                    particleData->color.r = particleStartColor.r + (particleVariation * (particleEndColor.r - particleStartColor.r));
                    particleData->color.g = particleStartColor.g + (particleVariation * (particleEndColor.g - particleStartColor.g));
                    particleData->color.b = particleStartColor.b + (particleVariation * (particleEndColor.b - particleStartColor.b));
                    break;
                case 2:
                    particleData->time = REFRESH_RATE_F * (0.9f + (0.00080000004f * frandmod(0x3E8)));
                    particleData->vel.x = 100.0f * REFRESH_FREQ * ((0.00020000001f * frandmod(0x3E8)) - 0.1f);
                    particleData->vel.y = -100.0f * REFRESH_FREQ * (0.1f + (0.0007f * frandmod(0x3E8)));
                    particleData->vel.z = 100.0f * REFRESH_FREQ * ((0.00020000001f * frandmod(0x3E8)) - 0.1f);
                    particleVariation = 0.001f * frandmod(0x3E8);
                    particleData->scale = 40.0f + (60.0f * particleVariation);
                    particleData->color.a = 8.0f + (10.0f * (1.0f - particleVariation));
                    particleData->color.r = particleStartColor.r + (particleVariation * (particleEndColor.r - particleStartColor.r));
                    particleData->color.g = particleStartColor.g + (particleVariation * (particleEndColor.g - particleStartColor.g));
                    particleData->color.b = particleStartColor.b + (particleVariation * (particleEndColor.b - particleStartColor.b));
                    break;
            }
            particleData->speedDecay = particleData->color.a;
            particleCount--;
            if (particleCount < 1) {
                break;
            }
        }
    }

    DCStoreRange(particleSystem->data, particleSystem->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void M460StartMapCameraFade(float volumeChangeRate)
{
    M460CameraWork *mapCamera;
    M460CameraWork *selectionCamera;

    selectionCamera = M460FindPrimaryCamera(1);
    mapCamera = M460FindPrimaryCamera(2);
    mapCamera->isCameraActive = 1;
    mapCamera->cameraCenter = selectionCamera->cameraCenter;
    mapCamera->cameraRotation = selectionCamera->cameraRotation;
    mapCamera->cameraZoom = selectionCamera->cameraZoom;
    mapCamera->fieldOfView = selectionCamera->fieldOfView;
    mapCamera->viewportRect = selectionCamera->viewportRect;
    mapCamera->viewportScale = selectionCamera->viewportScale;
    mapCamera->opacityChangeRate = volumeChangeRate;
    mapCamera->cameraOpacity = 0.0f;
    M460SetCameraMode(2);
}

s32 M460IsMapCameraActive(void)
{
    M460CameraWork *mapCamera = M460FindPrimaryCamera(2);
    return mapCamera->isCameraActive;
}
