#include "datadir_enum.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/hu3d.h"

#include "REL/m443Dll.h"
#include "version.h"

void CreateMapModels(OMOBJ *object);
void UpdateMapModels(OMOBJ *object);

s32 playerCameraMasks[5] = { 1, 2, 4, 8, 0x10 };

void InitializeMapObject(HUPROCESS *process)
{
    OMOBJ *mapObject = omAddObjEx(process, 0x1E, 0xD, 0, -1, CreateMapModels);
    mapObject->work[0] = 0;
}

void EmptyMapCallback(void) { }

void CreateMapModels(OMOBJ *object)
{
    s32 modelIndex;

    omSetTra(object, 0.0f, 0.0f, 0.0f);
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M443, 0));
    Hu3DModelLayerSet(object->mdlId[0], 2);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M443, 2));
    Hu3DModelLayerSet(object->mdlId[2], 2);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M443, 4));
    Hu3DModelLayerSet(object->mdlId[3], 2);
    Hu3DMotionSpeedSet(object->mdlId[3], 0.0f);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M443, 3));
    Hu3DModelLayerSet(object->mdlId[4], 2);
    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
        object->mdlId[modelIndex + 5] = Hu3DModelLink(object->mdlId[4]);
        Hu3DModelLayerSet(object->mdlId[modelIndex + 5], 2);
        Hu3DModelScaleSet(object->mdlId[modelIndex + 5], 0.93f, 0.93f, 0.93f);
    }
    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
        object->mdlId[modelIndex + 9] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M443, 1));
        Hu3DModelLayerSet(object->mdlId[modelIndex + 9], 2);
        Hu3DMotionSpeedSet(object->mdlId[modelIndex + 9], 0.0f);
    }
    object->work[0] = 0;
    object->work[1] = 0;
    object->work[2] = 0;
    object->work[3] = 0;
    object->objFunc = UpdateMapModels;
}

typedef struct M443DllMapWork {
    Vec playerCameraOffsets[4];
    float cameraZoomAdjustment;
} M443DllMapWork; /* size = 0x34 */

void UpdateMapModels(OMOBJ *object)
{
    float modelVerticalOffset;
    float animationProgress;

    M443DllCameraState *cameraState;
    s32 playerIndex;
    s32 showingModels;

    showingModels = 1;
    {
        Mtx cameraRotationMatrix;
        Vec modelPosition;
        Vec transformedOffset;
        Vec cameraDirection = { 0.0f, 0.0f, 0.0f };
        Vec playerOffsets[4] = {
            { -50.0f, 0.0f, -150.0f },
            { 50.0f, 0.0f, -150.0f },
            { -50.0f, 0.0f, -150.0f },
            { 50.0f, 0.0f, -150.0f },
        };

        for (playerIndex = 0; playerIndex < 5; playerIndex++) {
            cameraState = FindResultCameraByMask(playerCameraMasks[playerIndex]);
            modelPosition.x = cameraState->targetPosition.x + (cameraState->zoomDistance * (sind(cameraState->rotation.y) * cosd(cameraState->rotation.x)));
            modelPosition.y = cameraState->targetPosition.y + (cameraState->zoomDistance * -sind(cameraState->rotation.x));
            modelPosition.z = cameraState->targetPosition.z + (cameraState->zoomDistance * (cosd(cameraState->rotation.y) * cosd(cameraState->rotation.x)));
            Hu3DModelPosSetV(object->mdlId[playerIndex + 4], &modelPosition);
            SetModelCameraMask(object->mdlId[playerIndex + 4], playerCameraMasks[playerIndex]);
        }
        object->work[1] += 1;
        modelVerticalOffset = 150.0f;
        switch (object->work[0]) {
            case 0:
                if (getCameraTransitionState() == 4) {
                    object->work[0] += 1;
                    object->work[1] = 0;
                }
                break;
            case 1:
                animationProgress = 1.0f - (REFRESH_FREQ * object->work[1]);
                if (animationProgress <= 0.0f) {
                    object->work[0] += 1;
                    object->work[1] = 0;
                    animationProgress = 0.0f;
                }
                modelVerticalOffset = modelVerticalOffset * animationProgress;
                break;
            case 2:
                modelVerticalOffset = 0.0f;
                if (getResultStage() < 2) {
                    object->work[1] = 0;
                }
                if (object->work[1] >= REFRESH_RATE_F) {
                    object->work[0] += 1;
                    object->work[1] = 0;
                }
                break;
            case 3:
                animationProgress = REFRESH_FREQ * object->work[1];
                if (animationProgress > 1.0f) {
                    object->work[0] += 1;
                    object->work[1] = 0;
                    showingModels = 0;
                }
                modelVerticalOffset = modelVerticalOffset * animationProgress;
                break;
            default:
                showingModels = 0;
                break;
        }
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            cameraState = FindResultCameraByMask(playerCameraMasks[playerIndex + 1]);
            mtxRot(cameraRotationMatrix, cameraState->rotation.x, cameraState->rotation.y, 0.0f);
            cameraDirection.z = cameraState->zoomDistance;
            MTXMultVec(cameraRotationMatrix, &cameraDirection, &transformedOffset);
            VECAdd(&cameraState->targetPosition, &transformedOffset, &modelPosition);
            playerOffsets[playerIndex].y += modelVerticalOffset;
            MTXMultVec(cameraRotationMatrix, &playerOffsets[playerIndex], &transformedOffset);
            VECAdd(&modelPosition, &transformedOffset, &modelPosition);
            Hu3DModelPosSetV(object->mdlId[playerIndex + 9], &modelPosition);
            Hu3DModelRotSet(object->mdlId[playerIndex + 9], cameraState->rotation.x, cameraState->rotation.y, 0.0f);
            Hu3DModelScaleSet(object->mdlId[playerIndex + 9], 0.15f, 0.15f, 0.15f);
            SetModelCameraMask(object->mdlId[playerIndex + 9], playerCameraMasks[playerIndex + 1]);
            if (getResultStage() >= 1) {
                if (object->work[2] == 0) {
                    object->work[2] = 1;
                    object->work[3] = 0;
                }
                Hu3DMotionSpeedSet(object->mdlId[playerIndex + 9], 0.95f);
                if (showingModels == 0) {
                    Hu3DModelAttrSet(object->mdlId[playerIndex + 9], HU3D_ATTR_DISPOFF);
                }
            }
        }
        if (object->work[2] != 0) {
            object->work[3] += 1;
            if (object->work[3] >= 60.0f) {
                switch (object->work[2]) {
                    case 1:
                    case 2:
                        HuAudFXPlay(0x725);
                        break;
                    case 3:
                        HuAudFXPlay(0x726);
                        break;
                }
                object->work[2] += 1;
                object->work[3] = 0;
            }
        }
        if (getResultStage() == 2) {
            Hu3DMotionSpeedSet(object->mdlId[3], 1.0f);
        }
        else if (getResultStage() == 5) {
            Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
        }
        if (getResultStage() == 5) {
            Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
            Hu3DModelShadowMapSet(object->mdlId[0]);
            Hu3DModelShadowMapSet(object->mdlId[2]);
        }
    }
}
