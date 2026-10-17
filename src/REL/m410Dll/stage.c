#include "game/hu3d.h"
#include "game/object.h"

void CreateStageBackground(OMOBJ *object);
void UpdateStageBackground(OMOBJ *object);

void InitializeStageObjects(HUPROCESS *process)
{
    OMOBJ *object;

    object = omAddObjEx(process, 30, 6, 0, -1, CreateStageBackground);
    object->work[0] = 0;
}

void InitializeStageState(void) { }

void CreateStageBackground(OMOBJ *object)
{
    omSetTra(object, 0.0f, 0.0f, 0.0f);
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M410, 13));
    Hu3DModelShadowMapSet(object->mdlId[0]);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M410, 14));
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M410, 15));
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M410, 16));
    Hu3DModelAttrSet(object->mdlId[3], HU3D_MOTATTR_LOOP);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M410, 17));
    Hu3DModelAttrSet(object->mdlId[4], HU3D_MOTATTR_LOOP);
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M410, 18));
    object->objFunc = UpdateStageBackground;
}

void UpdateStageBackground(OMOBJ *object)
{
    Hu3DModelPosSet(object->mdlId[5], Center.x, Center.y, Center.z);
}
