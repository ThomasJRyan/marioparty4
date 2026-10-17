#include "game/hu3d.h"

#include "REL/m421Dll.h"

void InitializeHostModels(OMOBJ *object);
void HostModelCallback(OMOBJ *object);

OMOBJ *hostObject;

void CreateHost(HUPROCESS *work)
{
    OMOBJ *object = omAddObjEx(work, 0x1E, 6, 0, -1, InitializeHostModels);
    hostObject = object;
    object->work[0] = 0;
}

void EmptyHostCallback(void) { }

void InitializeHostModels(OMOBJ *playerObject)
{
    omSetTra(playerObject, 0.0f, 0.0f, 0.0f);
    playerObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M421, 0x02));
    Hu3DModelLayerSet(playerObject->mdlId[0], 1);
    Hu3DModelShadowMapSet(playerObject->mdlId[0]);

    playerObject->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M421, 0x00));
    Hu3DModelLayerSet(playerObject->mdlId[1], 1);

    playerObject->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M421, 0x01));
    Hu3DModelLayerSet(playerObject->mdlId[2], 5);

    playerObject->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M421, 0x05));
    Hu3DModelLayerSet(playerObject->mdlId[3], 1);
    Hu3DModelAttrSet(playerObject->mdlId[3], HU3D_MOTATTR_LOOP);

    playerObject->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M421, 0x03));
    Hu3DModelLayerSet(playerObject->mdlId[4], 5);
    Hu3DModelAttrSet(playerObject->mdlId[4], HU3D_MOTATTR_LOOP);

    playerObject->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M421, 0x04));
    Hu3DModelLayerSet(playerObject->mdlId[5], 5);
    Hu3DMotionTimeSet(playerObject->mdlId[5], 0.0f);
    playerObject->work[0] = 0;
    playerObject->work[1] = 0;
    playerObject->objFunc = HostModelCallback;
}

void HostModelCallback(OMOBJ *object) { }

void ResetHostMotion(void)
{
    Hu3DMotionTimeSet(hostObject->mdlId[5], 0.0f);
}
