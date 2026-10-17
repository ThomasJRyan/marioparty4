#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/process.h"

#include "REL/m407dll.h"

// function signatures
void CreateMapSceneModels(OMOBJ *);
void UpdateMapSceneModels(OMOBJ *);

// bss
HUPROCESS *mapProcess;
OMOBJ *mapSceneObject;
s16 mapSceneState;
s32 mapSectionIndex;

// data
s32 mapModelAssetIds[] = {
    DATA_MAKE_NUM(DATADIR_M407, 0x00),
    DATA_MAKE_NUM(DATADIR_M407, 0x02),
    DATA_MAKE_NUM(DATADIR_M407, 0x03),
    DATA_MAKE_NUM(DATADIR_M407, 0x01),
    DATA_MAKE_NUM(DATADIR_M407, 0x05),
    DATA_MAKE_NUM(DATADIR_M407, 0x06),
    DATA_MAKE_NUM(DATADIR_M407, 0x07),
    DATA_MAKE_NUM(DATADIR_M407, 0x08),
    DATA_MAKE_NUM(DATADIR_M407, 0x09),
    DATA_MAKE_NUM(DATADIR_M407, 0x0A),
    DATA_MAKE_NUM(DATADIR_M407, 0x0B),
    DATA_MAKE_NUM(DATADIR_M407, 0x0C),
    DATA_MAKE_NUM(DATADIR_M407, 0x04),
};

Vec playerMapPositions[] = { { 600.0f, 3.0f, -300.0f }, { 200.0f, 3.0f, -300.0f }, { -200.0f, 3.0f, -300.0f }, { -600.0f, 3.0f, -300.0f } };

Vec mapLightPosition = { 2000.0f, 3000.0f, 2000.0f };
Vec mapLightDirection = { -0.5f, -0.5f, -0.5f };
GXColor mapLightColor = { 255, 255, 255, 255 };
Vec mapShadowUp = { 0.0f, 1.0f, 0.0f };

void InitializeMapScene(HUPROCESS *process, s32 mapSectionIndex)
{
    HU3DLIGHT *activeLight;

    mapProcess = process;
    mapSectionIndex = mapSectionIndex;
    mapSceneObject = omAddObjEx(mapProcess, 0x100, 0xD, 0, 1, CreateMapSceneModels);
    omSetStatBit(mapSceneObject, 0x100);
    Hu3DLighInit();
    mapSceneState = Hu3DGLightCreateV(&mapLightPosition, &mapLightDirection, &mapLightColor);
    Hu3DGLightInfinitytSet(mapSceneState);
    activeLight = &Hu3DGlobalLight[mapSceneState];
}

void GetMapSceneState(void)
{
    s32 i;

    for (i = 0; i < 13; i++) {
        Hu3DModelKill(mapSceneObject->mdlId[i]);
    }
}

void CreateMapSceneModels(OMOBJ *object)
{
    s32 unusedModelIndex;
    s32 secondIndex;
    s32 thirdIndex;
    s32 fourthIndex;
    s32 i;

    object->objFunc = UpdateMapSceneModels;
    object->mdlId[0] = Hu3DModelCreateFile(mapModelAssetIds[2]);
    Hu3DModelShadowMapSet(object->mdlId[0]);
    Hu3DModelAmbSet(object->mdlId[0], 1.0f, 1.0f, 1.0f);
    object->mdlId[1] = Hu3DModelCreateFile(mapModelAssetIds[3]);
    Hu3DModelAmbSet(object->mdlId[1], 1.0f, 1.0f, 1.0f);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    for (i = 0; i < ARRAY_COUNT(GWPlayerCfg); i++) {
        object->mdlId[i + 2] = Hu3DModelCreateFile(mapModelAssetIds[GWPlayerCfg[i].character + 4]);
        Hu3DModelAmbSet(object->mdlId[i + 2], 1.0f, 1.0f, 1.0f);
        Hu3DModelShadowMapSet(object->mdlId[i + 2]);
    }

    for (i = 0; i < 5; i++) {
        if (i == 0) {
            object->mdlId[i + 6] = Hu3DModelCreateFile(mapModelAssetIds[0]);
        }
        else {
            object->mdlId[i + 6] = Hu3DModelLink(object->mdlId[6]);
        }
        Hu3DModelAmbSet(object->mdlId[i + 6], 1.0f, 1.0f, 1.0f);
        Hu3DModelShadowMapSet(object->mdlId[i + 6]);
    }

    object->mdlId[11] = Hu3DModelCreateFile(mapModelAssetIds[12]);
    object->mdlId[12] = Hu3DModelCreateFile(mapModelAssetIds[1]);

    for (i = 0; i < 4; i++) {
        Hu3DModelPosSetV(object->mdlId[i + 2], &playerMapPositions[i]);
    }

    for (i = 0; i < 5; i++) {
        Hu3DModelPosSet(object->mdlId[i + 6], 0.0f, 0.0f, -2000.0f + (8000.0f * (f32)i));
    }

    Hu3DModelPosSet(object->mdlId[11], 0.0f, 0.0f, -3900.0f);
    Hu3DModelPosSet(object->mdlId[12], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(object->mdlId[12], HU3D_MOTATTR_LOOP);

    if (mapSectionIndex > 0) {
        Hu3DModelPosSet(object->mdlId[1], 0.0f, 0.0f, 200.0f * (f32)mapSectionIndex);
        Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
    }
}

void UpdateMapSceneModels(OMOBJ *object)
{
    Vec adjustedLightPosition;

    adjustedLightPosition.x = mapLightPosition.x;
    adjustedLightPosition.y = mapLightPosition.y;
    adjustedLightPosition.z = mapLightPosition.z + Center.z;
    Hu3DGLightPosSetV(mapSceneState, &adjustedLightPosition, &mapLightDirection);
    Hu3DShadowPosSet(&adjustedLightPosition, &mapShadowUp, &Center);
}
