#include "game/hu3d.h"
#include "game/ClusterExec.h"
#include "game/data.h"
#include "game/EnvelopeExec.h"
#include "game/hsfload.h"
#include "game/init.h"
#include "game/memory.h"
#include "game/perf.h"
#include "game/ShapeExec.h"
#include "game/sprite.h"
#include "game/disp.h"

#include "dolphin/gx/GXVert.h"

#include "ext_math.h"

#ifndef __MWERKS__
#include <string.h>
#endif

#define SHADOW_HEAP_SIZE 0x9000

HU3DMODEL Hu3DData[HU3D_MODEL_MAX];
HU3DCAMERA Hu3DCamera[HU3D_CAM_MAX];
static s16 layerNum[8];
static void (*layerHook[8])(s16);
ANIMDATA *reflectAnim[5];
ANIMDATA *hiliteAnim[4];
HU3DPROJECTION Hu3DProjection[4];
HU3DSHADOW Hu3DShadowData;
HSFSCENE FogData;
Mtx Hu3DCameraMtx;
Mtx Hu3DCameraMtxXPose;
HU3DLIGHT Hu3DGlobalLight[0x8];
HU3DLIGHT Hu3DLocalLight[0x20];
Mtx lbl_8018D39C;

GXColor BGColor;
s16 reflectMapNo;
ANIMDATA *toonAnim;
s16 Hu3DShadowCamBit;
s32 Hu3DShadowF;
s32 shadowModelDrawF;
s16 Hu3DProjectionNum;
s16 Hu3DCameraNo;
s16 Hu3DCameraBit;
u32 Hu3DMallocNo;
s16 Hu3DPauseF;
u16 Hu3DCameraExistF;
static u16 NoSyncF;
s32 modelKillAllF;

#include "refMapData0.inc"
#include "refMapData1.inc"
#include "refMapData2.inc"
#include "refMapData3.inc"
#include "refMapData4.inc"
#include "toonMapData.inc"
#include "toonMapData2.inc"
#include "hiliteData.inc"
#include "hiliteData2.inc"
#include "hiliteData3.inc"
#include "hiliteData4.inc"

/**
 * @brief Initialize all Hu3D model, camera, light, motion, animation, particle, fog, and projection state.
 */
void Hu3DInit(void) {
    HU3DMODEL* data;
    HU3DCAMERA* camera;
    s16 i;

    data = Hu3DData;
    for (i = 0; i < HU3D_MODEL_MAX; i++, data++) {
        data->hsf = NULL;
    }
    camera = Hu3DCamera;
    for (i = 0; i < HU3D_CAM_MAX; i++, camera++) {
        camera->fov = -1.0f;
    }
    Hu3DMotionInit();
    Hu3DLighInit();
    BGColor.r = BGColor.g = BGColor.b = 0;
    BGColor.a = 0xFF;
    for (i = 0; i < 8; i++) {
        layerNum[i] = 0;
        layerHook[i] = 0;
    }
    reflectAnim[0] = HuSprAnimRead(refMapData0);
    reflectAnim[1] = HuSprAnimRead(refMapData1);
    reflectAnim[2] = HuSprAnimRead(refMapData2);
    reflectAnim[3] = HuSprAnimRead(refMapData3);
    reflectAnim[4] = HuSprAnimRead(refMapData4);
    reflectMapNo = 0;
    toonAnim = HuSprAnimRead(toonMapData);
    hiliteAnim[0] = HuSprAnimRead(hiliteData);
    hiliteAnim[1] = HuSprAnimRead(hiliteData2);
    hiliteAnim[2] = HuSprAnimRead(hiliteData3);
    hiliteAnim[3] = HuSprAnimRead(hiliteData4);
    Hu3DFogClear();
    Hu3DAnimInit();
    Hu3DParManInit();
    for (i = 0; i < 4; i++) {
        Hu3DProjection[i].anim = NULL;
    }
    Hu3DShadowCamBit = 0;
    Hu3DShadowData.buf = 0;
    Hu3DShadowF = 0;
    Hu3DProjectionNum = 0;
    Hu3DCameraExistF = 0;
    modelKillAllF = 0;
    Hu3DPauseF = 0;
    NoSyncF = 0;
}

/**
 * @brief Prepare Hu3D model flags and performance counters for a new rendered frame.
 */
void Hu3DPreProc(void) {
    HU3DMODEL *data;
    s16 i;

    GXSetCopyClear(BGColor, 0xFFFFFF);
    data = &Hu3DData[0];
    for (i = 0; i < HU3D_MODEL_MAX; i++, data++) {
        if (data->hsf != 0) {
            data->attr &= ~HU3D_ATTR_MOT_EXEC;
        }
    }
    totalPolyCnted = totalPolyCnt;
    totalMatCnted = totalMatCnt;
    totalTexCnted = totalTexCnt;
    totalTexCacheCnted = totalTexCacheCnt;
    totalPolyCnt = totalMatCnt = totalTexCnt = totalTexCacheCnt = 0;
}

#define HU3D_ATTR_CAMERA_UPDATE (HU3D_ATTR_CAMERA_MOTON|HU3D_ATTR_DISPOFF)

/**
 * @brief Render every active camera, layer hook, visible model, sprite layer, and projection.
 *
 * @details Motion and vertex deformation are evaluated before each eligible model is drawn.
 */
void Hu3DExec(void) {
    GXColor unusedColor = {0, 0, 0, 0};
    HU3DCAMERA* camera;
    HU3DMODEL* data;
    s16 camera_bit;
    s16 model_index;
    s16 vertices_changed;
    s16 draw_pending;
    s16 j;
    s16 i;
    void (* layer_hook)(s16);
    Mtx model_mtx;
    Mtx draw_mtx;
    HU3DPROJECTION* projection;

    HuPerfBegin(3);
    GXSetCurrentMtx(0U);
    camera = Hu3DCamera;
    shadowModelDrawF = 0;
    HuSprBegin();
    draw_pending = 0;
    for (Hu3DCameraNo = 0; Hu3DCameraNo < HU3D_CAM_MAX; Hu3DCameraNo++, camera++) {
        if (-1.0f != camera->fov) {
            GXInvalidateVtxCache();
            camera_bit = (s16) (1 << Hu3DCameraNo);
            Hu3DCameraBit = camera_bit;
            if (NoSyncF == 0) {
                if (Hu3DCameraNo == 0 && Hu3DShadowF != 0 && Hu3DShadowCamBit != 0) {
                    Hu3DShadowExec();
                    draw_pending = 1;
                    GXSetDrawDone();
                } else if (Hu3DCameraNo != 0) {
                    draw_pending = 1;
                    GXSetDrawDone();
                }
            } else if (Hu3DCameraNo == 0 && Hu3DShadowF != 0 && Hu3DShadowCamBit != 0) {
                Hu3DShadowExec();
            }
            projection = &Hu3DProjection[0];
            for (i = 0; i < 4; i++, projection++) {
                if (projection->anim != 0) {
                    C_MTXLookAt(projection->lookAtMtx, &projection->camPos, &projection->camUp, &projection->camTarget);
                }
            }
            if (Hu3DCameraNo == 0) {
                HuSprDispInit();
                HuSprExec(0x7F);
            }
            if (FogData.fogType != GX_FOG_NONE) {
                GXSetFog(FogData.fogType, FogData.fogStart, FogData.fogEnd, camera->near, camera->far, FogData.color);
            }
            for (j = 0; j < 8; j++) {
                if (layerHook[j] != 0) {
                    Hu3DCameraSet(Hu3DCameraNo, Hu3DCameraMtx);
                    PSMTXInvXpose(Hu3DCameraMtx, Hu3DCameraMtxXPose);
                    layer_hook = layerHook[j];
                    layer_hook(j);
                }
                if (layerNum[j] != 0) {
                    Hu3DDrawPreInit();
                    Hu3DCameraSet(Hu3DCameraNo, Hu3DCameraMtx);
                    PSMTXInvXpose(Hu3DCameraMtx, Hu3DCameraMtxXPose);
                    data = Hu3DData;
                    for (i = 0, model_index = i; i < HU3D_MODEL_MAX; i++, data++) {
                        if (data->hsf != 0) {
                            if ((data->attr & HU3D_ATTR_CAMERA) != 0) {
                                Hu3DCameraMotionExec(i);
                            } else {
                                if ((data->attr & HU3D_ATTR_CAMERA_UPDATE) == HU3D_ATTR_CAMERA_UPDATE && data->motId != -1) {
                                    Hu3DMotionExec(i, data->motId, data->motWork.time, 0);
                                }
                                if ((data->attr & (HU3D_ATTR_DISPOFF|HU3D_ATTR_MOTION_OFF)) == 0 && (data->cameraBit & camera_bit) != 0 && data->layerNo == j) {
                                    if (((data->attr & HU3D_ATTR_MOT_EXEC) == 0 && (data->attr & HU3D_ATTR_MOT_SLOW) == 0) || ((data->attr & HU3D_ATTR_MOT_SLOW) != 0 && (data->tick & 1) != 0)) {
                                        vertices_changed = 0;
                                        data->motAttr &= ~HU3D_MOTATTR;
                                        if (data->motId != -1) {
                                            Hu3DMotionExec(i, data->motId, data->motWork.time, 0);
                                        }
                                        if (data->motIdShift != -1) {
                                            Hu3DSubMotionExec(i);
                                        }
                                        if (data->motIdOvl != -1) {
                                            Hu3DMotionExec(i, data->motIdOvl, data->motOvlWork.time, 1);
                                        }
                                        if ((data->attr & HU3D_ATTR_CLUSTER_ON) != 0) {
                                            ClusterMotionExec(data);
                                            vertices_changed = 1;
                                        }
                                        if (data->motIdShape != -1) {
                                            if (data->motId == -1) {
                                                Hu3DMotionExec(i, data->motIdShape, data->motShapeWork.time, 0);
                                            } else {
                                                Hu3DMotionExec(i, data->motIdShape, data->motShapeWork.time, 1);
                                            }
                                            vertices_changed = 1;
                                        }
                                        if ((data->attr & (HU3D_ATTR_ENVELOPE_OFF|HU3D_ATTR_HOOKFUNC)) == 0 && (data->motAttr & HU3D_MOTATTR_PAUSE) == 0) {
                                            vertices_changed = 1;
                                            InitVtxParm(data->hsf);
                                            if (data->motIdShape != -1) {
                                                ShapeProc(data->hsf);
                                            }
                                            if ((data->attr & 0x400) != 0) {
                                                ClusterProc(data);
                                            }
                                            if (data->hsf->cenvNum != 0) {
                                                EnvelopeProc(data->hsf);
                                            }
                                            PPCSync();
                                        }
                                        if (vertices_changed != 0) {
                                            GXInvalidateVtxCache();
                                        }
                                        data->attr |= 0x800;
                                    }
                                    if (draw_pending != 0 && (data->attr & HU3D_ATTR_HOOKFUNC) != 0) {
                                        GXWaitDrawDone();
                                        draw_pending = 0;
                                    }
                                    if ((data->attr & HU3D_ATTR_HOOK) == 0 && (0.0f != data->scale.x || 0.0f != data->scale.y || 0.0f != data->scale.z)) {
                                        mtxRot(model_mtx, data->rot.x, data->rot.y, data->rot.z);
                                        mtxScaleCat(model_mtx, data->scale.x, data->scale.y, data->scale.z);
                                        mtxTransCat(model_mtx, data->pos.x, data->pos.y, data->pos.z);
                                        PSMTXConcat(Hu3DCameraMtx, model_mtx, draw_mtx);
                                        PSMTXConcat(draw_mtx, data->mtx, draw_mtx);
                                        Hu3DDraw(data, draw_mtx, &data->scale);
                                    }
                                    data->tick++;
                                    model_index++;
                                    if (model_index >= layerNum[j]) {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    Hu3DDrawPost();
                }
            }
        }
    }
    HuSprDispInit();
    HuSprExec(0);
    data = Hu3DData;
    for (i = 0; i < HU3D_MODEL_MAX; i++, data++) {
        if (data->hsf != 0 && (data->motId != -1 || (data->attr & HU3D_ATTR_CLUSTER_ON) != 0 || data->motIdShape != -1) && (Hu3DPauseF == 0 || (data->attr & HU3D_ATTR_NOPAUSE) != 0)) {
            Hu3DMotionNext(i);
        }
    }
    HuSprFinish();
    Hu3DAnimExec();
    HuPerfEnd(3);
}

/**
 * @brief Release all models, motions, animations, particles, and heap allocations owned by Hu3D.
 */
void Hu3DAllKill(void) {
    s16 i;
    Hu3DModelAllKill();
    Hu3DMotionAllKill();
    Hu3DCameraAllKill();
    Hu3DLightAllKill();
    Hu3DAnimAllKill();
    if(reflectAnim[0] != (ANIMDATA *)refMapData0) {
        HuMemDirectFree(reflectAnim[0]);
    }
    reflectAnim[0] = HuSprAnimRead(refMapData0);
    if(Hu3DShadowData.buf) {
        HuMemDirectFree(Hu3DShadowData.buf);
        Hu3DShadowCamBit = 0;
        Hu3DShadowData.buf = NULL;
        Hu3DShadowF = 0;
    }
    Hu3DFogClear();
    for(i=0; i<8; i++) {
        layerNum[i] = 0;
        layerHook[i] = NULL;
    }
    for(i=0; i<4; i++) {
        if(Hu3DProjection[i].anim) {
            Hu3DProjectionKill(i);
        }
        Hu3DProjection[i].anim = NULL;
    }
    NoSyncF = 0;
}

/**
 * @brief Set the RGB color used when clearing the render target.
 * @param red Red component from 0 through 255.
 * @param green Green component from 0 through 255.
 * @param blue Blue component from 0 through 255.
 */
void Hu3DBGColorSet(u8 red, u8 green, u8 blue) {
    BGColor.r = red;
    BGColor.g = green;
    BGColor.b = blue;
}

/**
 * @brief Assign a callback to a Hu3D render layer.
 * @param layer Layer index from 0 through 7.
 * @param func Callback invoked with the layer index before models on that layer are drawn; NULL disables it.
 */
void Hu3DLayerHookSet(s16 layer, void (*func)(s16)) {
    layerHook[layer] = func;
}

/**
 * @brief Set the global Hu3D pause flag.
 * @param paused Nonzero to mark Hu3D paused; zero to clear the flag.
 */
void Hu3DPauseSet(BOOL paused) {
    Hu3DPauseF = paused;
}

/**
 * @brief Control whether Hu3D issues inter-camera draw synchronization.
 * @param no_sync Nonzero to suppress normal draw-done synchronization; zero to enable it.
 */
void Hu3DNoSyncSet(BOOL no_sync) {
    NoSyncF = no_sync;
}

/**
 * @brief Load HSF data into a newly initialized model slot.
 *
 * @details Embedded motion, cluster, shape, fog, transform, and render state are initialized from the loaded HSF.
 * @param hsf_data Pointer to serialized HSF model data.
 * @return The allocated model ID, or -1 when all model slots are occupied.
 */
s16 Hu3DModelCreate(void *hsf_data) {
    HSFDATA* unused_hsf;
    HU3DMODEL* model;
    s16 i;
    s16 model_id;

    model = Hu3DData;

    for (model_id = 0; model_id < HU3D_MODEL_MAX; model_id++, model++) {
        if (model->hsf == 0x0) {
            break;
        }
    }
    if (model_id == HU3D_MODEL_MAX) {
        OSReport("Error: Create Model Over!\n");
        return -1;
    }
    model->hsf = LoadHSF(hsf_data);
    model->mallocNo = Hu3DMallocNo = (u32)model->hsf;
    model->attr = HU3D_ATTR_NONE;
    model->motAttr = HU3D_ATTR_NONE;
    model->projBit = 0;
    MakeDisplayList(model_id, model->mallocNo);
    model->motWork.speed = 1.0f;
    for (i = 0; i < 4; i++) {
        model->motIdCluster[i] = -1;
    }
    model->motIdOvl = -1;
    model->motIdShift = -1;
    model->motIdShape = -1;
    model->motWork.time = 0.0f;
    if (model->hsf->motionNum != 0) {
        model->motId = model->motIdSrc = Hu3DMotionModelCreate(model_id);
        if (model->hsf->cenvNum != 0) {
            Hu3DMotionExec(model_id, model->motId, 0.0f, 0);
            EnvelopeProc(model->hsf);
            PPCSync();
        }
        if (model->hsf->clusterNum != 0) {
            Hu3DMotionClusterSet(model_id, model->motId);
        }
        if (model->hsf->shapeNum != 0) {
            Hu3DMotionShapeSet(model_id, model->motId);
        }
        model->motWork.start = 0.0f;
        model->motWork.end = Hu3DMotionMaxTimeGet(model_id);
    } else {
        model->motIdSrc = model->motId = -1;
    }
    model->pos.x = model->pos.y = model->pos.z = 0.0f;
    model->rot.x = model->rot.y = model->rot.z = 0.0f;
    model->scale.x = model->scale.y = model->scale.z = 1.0f;
    model->cameraBit = -1;
    model->layerNo = 0;
    model->hookData = 0;
    model->lightNum = 0;
    model->hiliteIdx = 0;
    model->ambR = model->ambG = model->ambB = 1.0f;
    model->reflectType = -1;
    model->linkMdlId = -1;

    for (i = 0; i < 8; i++) {
        model->lLightId[i] = -1;
    }
    model->camInfoBit = 0;
    model->tick = (u8) model_id;
    PSMTXIdentity(model->mtx);
    layerNum[0] += 1;
    HuMemDCFlush(HEAP_MODEL);
    if ((model->hsf->sceneNum != 0) && ((model->hsf->scene->fogStart) || (model->hsf->scene->fogEnd))) {
        Hu3DFogSet(model->hsf->scene->fogStart, model->hsf->scene->fogEnd, model->hsf->scene->color.r, model->hsf->scene->color.g, model->hsf->scene->color.b);
    }
    return model_id;
}

/**
 * @brief Create an independently transformed model instance from an existing model.
 *
 * @details The new instance copies the source HSF header, duplicates the HSF
 * object array, and retargets the copied root pointer. It retains `hsfLink` to
 * the source HSF and reuses the source allocation and motion IDs, while
 * initializing its own transform, camera, layer, hook, light, projection,
 * ambient, reflection, and matrix state.
 * @param source_model_id Source model ID whose HSF header, object hierarchy,
 * allocation identity, attributes, and motion IDs seed the new instance.
 * @return The linked model ID, or -1 when all model slots are occupied.
 */
s16 Hu3DModelLink(s16 source_model_id) {
    HSFOBJECT* object_copy;
    HU3DMODEL* source_model;
    HU3DMODEL* linked_model;
    s16 linked_model_id;
    s16 i;
    s32 unused_object_offset;
    s32 unused_root_offset;
    s32 unused_data_offset;
    s32 unused_copy_index;
    HSFDATA* unused_source_hsf;
    HSFDATA* unused_linked_hsf;

    source_model = &Hu3DData[source_model_id];
    linked_model = Hu3DData;
    for (linked_model_id = 0; linked_model_id < HU3D_MODEL_MAX; linked_model_id++, linked_model++) {
        if (linked_model->hsf == 0x0) {
            break;
        }
    }
    if (linked_model_id == HU3D_MODEL_MAX) {
        return -1;
    }
    linked_model->hsfLink = source_model->hsf;
    linked_model->hsf = HuMemDirectMallocNum(HEAP_MODEL, 0x80, linked_model->mallocNoLink);
    linked_model->mallocNoLink = (u32)linked_model->hsf;
    *linked_model->hsf = *source_model->hsf;
    object_copy = Hu3DObjDuplicate(linked_model->hsf, linked_model->mallocNoLink);
    linked_model->hsf->root = (HSFOBJECT*)((u32)object_copy + ((u32)linked_model->hsf->root - (u32)linked_model->hsf->object));
    linked_model->hsf->object = object_copy;
    linked_model->mallocNo = source_model->mallocNo;
    linked_model->attr = source_model->attr;
    source_model->attr |= HU3D_ATTR_LINK;
    linked_model->motAttr = source_model->motAttr;
    linked_model->pos.x = linked_model->pos.y = linked_model->pos.z = 0.0f;
    linked_model->rot.x = linked_model->rot.y = linked_model->rot.z = 0.0f;
    linked_model->scale.x = linked_model->scale.y = linked_model->scale.z = 1.0f;
    linked_model->motId = source_model->motId;
    if (linked_model->motId != -1) {
        linked_model->motWork.start = 0.0f;
        linked_model->motWork.end = Hu3DMotionMaxTimeGet(linked_model_id);
    }
    linked_model->motIdShift = linked_model->motIdOvl = linked_model->motIdShape = -1;
    for (i = 0; i < 4; i++) {
        linked_model->motIdCluster[i] = source_model->motIdCluster[i];
        if (linked_model->motIdCluster[i] != -1) {
            ClusterAdjustObject(linked_model->hsf, Hu3DMotion[linked_model->motIdCluster[i]].hsf);
            linked_model->attr |= HU3D_ATTR_CLUSTER_ON;
        }
    }
    linked_model->motWork.time = source_model->motWork.time;
    linked_model->motWork.speed = source_model->motWork.speed;
    linked_model->motIdSrc = source_model->motIdSrc;
    linked_model->cameraBit = -1;
    linked_model->layerNo = 0;
    linked_model->hookData = 0;
    linked_model->lightNum = 0;
    linked_model->hiliteIdx = 0;
    linked_model->projBit = 0;
    linked_model->ambR = linked_model->ambG = linked_model->ambB = 1.0f;
    linked_model->reflectType = -1;
    linked_model->linkMdlId = source_model_id;
    for (i = 0; i < 8; i++) {
        linked_model->lLightId[i] = -1;
    }
    linked_model->camInfoBit = 0;
    PSMTXIdentity(linked_model->mtx);
    layerNum[0] += 1;
    return linked_model_id;
}

/**
 * @brief Create a model slot rendered by a callback instead of HSF geometry.
 * @param hook Render callback stored in the new model entry.
 * @return The allocated model ID, or -1 when all model slots are occupied.
 */
s16 Hu3DHookFuncCreate(HU3DMODELHOOK hook) {
    HSFDATA* unused_hsf;
    HU3DMODEL* model;
    s16 model_id;
    s16 i;

    model = Hu3DData;
    for (model_id = 0; model_id < HU3D_MODEL_MAX; model_id++, model++) {
        if (model->hsf == 0) {
            break;
        }
    }
    if (model_id == HU3D_MODEL_MAX) {
        return -1;
    }
    model->hookFunc = hook;
    model->mallocNo = model_id+10000;
    model->attr = HU3D_ATTR_HOOKFUNC;
    model->motAttr = HU3D_ATTR_NONE;
    model->pos.x = model->pos.y = model->pos.z = 0.0f;
    model->rot.x = model->rot.y = model->rot.z = 0.0f;
    model->scale.x = model->scale.y = model->scale.z = 1.0f;
    model->motId = model->motIdShift = model->motIdOvl = model->motIdShape = -1;

    for (i = 0; i < 4; i++) {
        model->motIdCluster[i] = -1;
    }
    model->motWork.time = 0.0f;
    model->motWork.speed = 1.0f;
    model->motIdSrc = -1;
    model->cameraBit = -1;
    model->layerNo = 0;
    model->hookData = 0;
    model->lightNum = 0;
    model->hiliteIdx = 0;
    model->linkMdlId = -1;
    model->projBit = 0;
    model->reflectType = -1;

    for (i = 0; i < 8; i++) {
        model->lLightId[i] = -1;
    }
    model->camInfoBit = 0;
    PSMTXIdentity(model->mtx);
    layerNum[0] += 1;
    return model_id;
}

/**
 * @brief Release a model and any motion, light, animation, and HSF storage no longer shared.
 * @param model_id Model ID to release.
 */
void Hu3DModelKill(s16 model_id) {
    HSFDATA* hsf;
    HU3DMODEL* model;
    HU3DMODEL* model_cursor;
    HU3DPARTICLE *particle;
    s16 reference_count;
    s16 i;

    model = &Hu3DData[model_id];
    hsf = model->hsf;
    if (hsf != 0) {
        if ((model->attr & HU3D_ATTR_SHADOW) != 0) {
            Hu3DShadowCamBit -= 1;
        }
        layerNum[model->layerNo] -= 1;

        if ((model->attr & HU3D_ATTR_HOOKFUNC) != 0) {
            HuMemDirectFreeNum(HEAP_MODEL, model->mallocNo);
            if ((model->attr & HU3D_ATTR_PARTICLE_KILL) != 0) {
                particle = model->hookData;
                HuSprAnimKill(particle->anim);
            }
            model->hsf = NULL;
            if (modelKillAllF == 0) {
                HuMemDCFlush(HEAP_MODEL);
            }
            return;
        }
        if ((model->attr & HU3D_ATTR_CAMERA) != 0) {
            if (model->motId != -1) {
                Hu3DMotionKill(model->motId);
            }
            HuMemDirectFreeNum(HEAP_MODEL, model->mallocNo);
            model->hsf = NULL;
            return;
        }
        Hu3DAnimModelKill(model_id);
        if (model->linkMdlId != -1) {
            HuMemDirectFree(model->hsf);
            HuMemDirectFreeNum(HEAP_MODEL, model->mallocNoLink);
            hsf = model->hsfLink;
            model->hsf = hsf;
        }
        model_cursor = Hu3DData;
        for (reference_count = i = 0; i < HU3D_MODEL_MAX; i++, model_cursor++) {
            if ((model_cursor->hsf != 0) && (model_cursor->hsf == hsf || (model_cursor->linkMdlId != -1 && model_cursor->hsfLink == hsf))) {
                reference_count++;
            }
        }
        if (reference_count > 1) {
            model->hsf = NULL;
            model_cursor = Hu3DData;
            if (model->motIdSrc != -1) {
                for (i = 0; i < HU3D_MODEL_MAX; i++, model_cursor++) {
                    if (model_cursor->hsf != 0 && model_cursor->linkMdlId != -1 && model_cursor->hsfLink == hsf) {
                        Hu3DMotion[model->motIdSrc].modelId = i;
                        break;
                    }
                }
            }
            if (modelKillAllF == 0) {
                HuMemDCFlush(HEAP_MODEL);
            }
            return;
        }
        if (model->motIdSrc != -1 && Hu3DMotionKill(model->motIdSrc) == 0) {
            Hu3DMotion[model->motIdSrc].modelId = -1;
            HuMemDirectFreeNum(HEAP_MODEL, model->mallocNo);
            model->hsf = NULL;
            if (modelKillAllF == 0) {
                HuMemDCFlush(HEAP_MODEL);
            }
            return;
        }
        HuMemDirectFree(model->hsf);
        HuMemDirectFreeNum(HEAP_MODEL, model->mallocNo);
        for (i = 0; i < model->lightNum; i++) {
            Hu3DGLightKill(model->lightId[i]);
        }
        for (i = 0; i < 8; i++) {
            if (model->lLightId[i] != -1) {
                Hu3DLLightKill(model_id, i);
            }
        }
        model->hsf = NULL;
        if (modelKillAllF == 0) {
            HuMemDCFlush(HEAP_MODEL);
        }
    }
}

/**
 * @brief Release every populated Hu3D model slot.
 */
void Hu3DModelAllKill(void) {
    HU3DMODEL* model;
    s16 i;

    modelKillAllF = 1;
    model = Hu3DData;

    for (i = 0; i < HU3D_MODEL_MAX; i++, model++) {
        if (model->hsf != 0) {
            Hu3DModelKill(i);
        }
    }
    modelKillAllF = 0;

    for (i = 0; i < 8; i++) {
        layerNum[i] = 0;
        layerHook[i] = NULL;
    }
    Hu3DParManAllKill();
    HuMemDCFlush(HEAP_MODEL);
}

/**
 * @brief Set a model's position components.
 * @param index Model ID to update.
 * @param x X position component.
 * @param y Y position component.
 * @param z Z position component.
 */
void Hu3DModelPosSet(s16 index, f32 x, f32 y, f32 z) {
    HU3DMODEL* model;

    model = &Hu3DData[index];
    model->pos.x = x;
    model->pos.y = y;
    model->pos.z = z;
}

/**
 * @brief Set a model's position from a vector.
 * @param model_id Model ID to update.
 * @param position Vector containing the new position components.
 */
void Hu3DModelPosSetV(s16 model_id, Vec *position) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    model->pos = *position;
}

/**
 * @brief Set a model's Euler rotation in degrees components.
 * @param index Model ID to update.
 * @param x X Euler rotation in degrees component.
 * @param y Y Euler rotation in degrees component.
 * @param z Z Euler rotation in degrees component.
 */
void Hu3DModelRotSet(s16 index, f32 x, f32 y, f32 z) {
    HU3DMODEL* model;

    model = &Hu3DData[index];
    model->rot.x = x;
    model->rot.y = y;
    model->rot.z = z;
}

/**
 * @brief Set a model's Euler rotation in degrees from a vector.
 * @param model_id Model ID to update.
 * @param rotation Vector containing the new Euler rotation in degrees components.
 */
void Hu3DModelRotSetV(s16 model_id, Vec *rotation) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    model->rot = *rotation;
}

/**
 * @brief Set a model's scale components.
 * @param index Model ID to update.
 * @param x X scale component.
 * @param y Y scale component.
 * @param z Z scale component.
 */
void Hu3DModelScaleSet(s16 index, f32 x, f32 y, f32 z) {
    HU3DMODEL* model;

    model = &Hu3DData[index];
    model->scale.x = x;
    model->scale.y = y;
    model->scale.z = z;
}

/**
 * @brief Set a model's scale from a vector.
 * @param model_id Model ID to update.
 * @param scale Vector containing the new scale components.
 */
void Hu3DModelScaleSetV(s16 model_id, Vec *scale) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    model->scale = *scale;
}

/**
 * @brief Set model render-attribute bits and propagate supported bits to linked models.
 * @param model_id Model ID to update.
 * @param attrs Bit mask of HU3D model attributes to enable.
 */
void Hu3DModelAttrSet(s16 model_id, u32 attrs) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    if ((attrs & HU3D_MOTATTR) != 0) {
        model->motAttr |= attrs & ~HU3D_MOTATTR;
    } else {
        model->attr |= attrs;
    }
}

/**
 * @brief Clear model render-attribute bits and propagate supported bits to linked models.
 * @param model_id Model ID to update.
 * @param attrs Bit mask of HU3D model attributes to disable.
 */
void Hu3DModelAttrReset(s16 model_id, u32 attrs) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    if ((attrs & HU3D_MOTATTR) != 0) {
        model->motAttr &= ~attrs;
    } else {
        model->attr &= ~attrs;
    }
}

/**
 * @brief Get a model's render-attribute mask.
 * @param model_id Model ID to query.
 * @return The current HU3D attribute bit mask.
 */
u32 Hu3DModelAttrGet(s16 model_id) {
    HU3DMODEL *model = &Hu3DData[model_id];
    return model->attr;
}

/**
 * @brief Get a model's motion-playback attribute mask.
 * @param model_id Model ID to query.
 * @return The current HU3D motion attribute bit mask.
 */
u32 Hu3DModelMotionAttrGet(s16 model_id) {
    HU3DMODEL *model = &Hu3DData[model_id];
    return model->motAttr;
}

/**
 * @brief Enable playback attributes on one model cluster-motion slot.
 * @param model_id Model ID to update.
 * @param cluster_slot Cluster slot from 0 through 3.
 * @param attrs Cluster attribute bits to enable.
 */
void Hu3DModelClusterAttrSet(s16 model_id, s16 cluster_slot, s32 attrs) {
    HU3DMODEL* model;
    s32 unused_attr;

    model = &Hu3DData[model_id];
    model->clusterAttr[cluster_slot] |= attrs;
}

/**
 * @brief Disable playback attributes on one model cluster-motion slot.
 * @param model_id Model ID to update.
 * @param cluster_slot Cluster slot from 0 through 3.
 * @param attrs Cluster attribute bits to clear.
 */
void Hu3DModelClusterAttrReset(s16 model_id, s16 cluster_slot, s32 attrs) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    model->clusterAttr[cluster_slot] &= ~attrs;
}

/**
 * @brief Set the camera visibility mask for a model.
 * @param model_id Model ID to update.
 * @param camera_mask Bit mask of cameras allowed to draw the model.
 */
void Hu3DModelCameraSet(s16 model_id, u16 camera_mask) {
    HU3DMODEL* model = &Hu3DData[model_id];
    model->cameraBit = camera_mask;
}

/**
 * @brief Move a model to a render layer.
 * @param model_id Model ID to update.
 * @param layer Destination layer index from 0 through 7.
 */
void Hu3DModelLayerSet(s16 model_id, s16 layer) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    layerNum[model->layerNo] -= 1;
    model->layerNo = layer;
    layerNum[layer] += 1;
}

/**
 * @brief Find a named HSF object within a model.
 * @param model_id Model ID whose object table is searched.
 * @param object_name Object name in the external naming format accepted by MakeObjectName.
 * @return The matching object pointer, or NULL after reporting an error if no object matches.
 */
HSFOBJECT* Hu3DModelObjPtrGet(s16 model_id, char *object_name) {
    char name[0x100];
    s32 unused_name_length;
    s16 object_index;
    HSFDATA* hsf;
    s16 bucket_index;
    HSFOBJECT* object;
    HSFOBJECT* object_cursor;
    u32 unused_name_word;

    hsf = Hu3DData[model_id].hsf;
    object = hsf->object;
    strcpy(&name, MakeObjectName(object_name));

    for (bucket_index = 0; bucket_index < hsf->objectNum; object++, bucket_index++) {
        object_cursor = object;
        if (strcmp(&name, object_cursor->name) == 0) {
            return object;
        }
    }
    if (bucket_index == hsf->objectNum) {
        OSReport("Error: OBJPtr Error!\n");
    }
    return NULL;
}

/**
 * @brief Set constant-data attribute bits on attrs mesh object.
 * @param object Object to update; non-mesh objects are ignored.
 * @param attrs Constant-data attribute bits to enable.
 */
static inline void inlineFunc(HSFOBJECT* object, u32 attrs) {
    HSFCONSTDATA* constant_data;
    HSFOBJECT* mesh_object = object;
    if (mesh_object->type == HSF_OBJ_MESH) {
        constant_data = mesh_object->constData;
        constant_data->attr |= attrs;
    }
}

/**
 * @brief Apply a uniform transparency level to every material in a model.
 *
 * @details The inverse alpha, material pass, mesh constant flags, and model override flag are updated.
 * @param model_id Model ID to update.
 * @param opacity Opacity level, normally 0 for transparent through 1 for opaque.
 */
void Hu3DModelTPLvlSet(s16 model_id, f32 opacity) {
    HSFMATERIAL* material;
    HSFDATA* hsf;
    s16 i;
    HU3DMODEL* model;
    HSFOBJECT* mesh_object;
    HSFOBJECT* object;
    HSFCONSTDATA* constant_data;

    model = &Hu3DData[model_id];
    hsf = model->hsf;
    material = hsf->material;
    for (i = 0; i < hsf->materialNum; i++, material++) {
        material->invAlpha = 1.0f - opacity;
        if (1.0f != opacity) {
            material->pass = (material->pass & 0xF0) | 1;
        } else {
            material->pass &= 0xF0;
        }
    }
    object = hsf->object;
    for (i = 0; i < hsf->objectNum; object++, i++) {
        mesh_object = object;
        if (mesh_object->type == 2) {
            constant_data = mesh_object->constData;
            constant_data->attr |= 1;
        }
    }
    model->attr |= HU3D_ATTR_TPLVL_SET;
}

/**
 * @brief Assign a highlight texture to every mesh object in a model.
 * @param model_id Model ID to update.
 * @param highlight_anim Highlight animation data to reference; ownership remains with the caller.
 */
void Hu3DModelHiliteMapSet(s16 model_id, ANIMDATA *highlight_anim) {
    HSFOBJECT* mesh_object;
    HU3DMODEL* model;
    HSFDATA* hsf;
    s16 i;
    HSFOBJECT* object;
    HSFCONSTDATA* constant_data;

    model = &Hu3DData[model_id];
    hsf = model->hsf;
    object = hsf->object;
    for (i = 0; i < hsf->objectNum; object++, i++) {
        mesh_object = object;
        if (mesh_object->type == HSF_OBJ_MESH) {
            mesh_object->flags |= 0x100;
            constant_data = mesh_object->constData;
            constant_data->attr |= 0x8000;
            constant_data->hiliteMap = highlight_anim;
        }
    }
}

/**
 * @brief Set constant-data attribute bits on an object when constant data exists.
 * @param object HSF object to update.
 * @param attrs Constant-data attribute bits to enable.
 */
static inline void constDataFlagSet(HSFOBJECT* object, u32 attrs) {
    HSFCONSTDATA* constant_data;
    HSFOBJECT* object_with_data = object;
    if (object_with_data->constData != 0) {
        constant_data = object_with_data->constData;
        constant_data->attr |= attrs;
    }
}

/**
 * @brief Clear constant-data attribute bits on an object when constant data exists.
 * @param object HSF object to update.
 * @param attrs Constant-data attribute bits to disable.
 */
static inline void constDataFlagReset(HSFOBJECT* object, u32 attrs) {
    HSFCONSTDATA* constant_data;
    HSFOBJECT* object_with_data = object;
    if (object_with_data->constData != 0) {
        constant_data = object_with_data->constData;
        constant_data->attr &= ~attrs;
    }
}

/**
 * @brief Enable shadow casting for a model and its eligible mesh objects.
 * @param model_id Model ID to mark as a shadow caster.
 */
void Hu3DModelShadowSet(s16 model_id) {
    HSFCONSTDATA* child_constant_data;
    HSFDATA* hsf;
    HSFCONSTDATA* constant_data;
    HSFOBJECT* object_entry;
    s16 object_index;
    HSFOBJECT* object;
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    hsf = model->hsf;
    if ((model->attr & HU3D_ATTR_SHADOW) == 0) {
        Hu3DShadowCamBit++;
    }
    model->attr |= HU3D_ATTR_SHADOW;
    object = hsf->object;

    for (object_index = 0; object_index < hsf->objectNum; object_index++, object++) {
        constDataFlagSet(object, 0x400);
    }
}

/**
 * @brief Disable shadow casting for a model and its mesh objects.
 * @param model_id Model ID to remove from shadow rendering.
 */
void Hu3DModelShadowReset(s16 model_id) {
    s16 object_index;
    HSFOBJECT* object;
    HSFCONSTDATA* constant_data;
    HU3DMODEL* model;
    HSFDATA* hsf;
    HSFOBJECT* object_entry;

    model = &Hu3DData[(s16) model_id];
    hsf = model->hsf;
    model->attr &= ~HU3D_ATTR_SHADOW;
    Hu3DShadowCamBit -= 1;
    object = hsf->object;
    for (object_index = 0; object_index < hsf->objectNum; object_index++, object++) {
        constDataFlagReset(object, 0x400);
    }
}

/**
 * @brief Make a model visible during the shadow-rendering pass.
 * @param model_id Model ID to update.
 */
void Hu3DModelShadowDispOn(s16 model_id) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    model->attr |= HU3D_ATTR_SHADOW;
}

/**
 * @brief Hide a model during the shadow-rendering pass.
 * @param model_id Model ID to update.
 */
void Hu3DModelShadowDispOff(s16 model_id) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    model->attr &= ~HU3D_ATTR_SHADOW;
}

/**
 * @brief Enable shadow-map sampling on every mesh object in a model.
 * @param model_id Model ID to receive the shadow map.
 */
void Hu3DModelShadowMapSet(s16 model_id) {
    HSFCONSTDATA* hsf;
    HSFDATA* model;
    s16 i;
    HSFOBJECT* object;
    HSFOBJECT* object_entry;

    model = Hu3DData[model_id].hsf;
    object = model->object;
    for (i = 0; i < model->objectNum; i++, object++) {
        constDataFlagSet(object, 8);
    }
}

/**
 * @brief Enable shadow-map sampling on one named object and its descendants.
 * @param model_id Model ID containing the object hierarchy.
 * @param object_name Name of the root object to update.
 */
void Hu3DModelShadowMapObjSet(s16 model_id, char *object_name) {
    char name[0x100];
    HSFDATA* hsf;
    s16 i;
    HSFOBJECT* object;
    HSFOBJECT* object_entry;
    HSFCONSTDATA* constant_data;

    hsf = Hu3DData[model_id].hsf;
    object = hsf->object;
    strcpy(&name, MakeObjectName(object_name));

    for (i = 0; i < hsf->objectNum; i++, object++) {
        object_entry = object;
        if (object_entry->constData != 0x0 && strcmp(&name, object_entry->name) == 0) {
            constant_data = object_entry->constData;
            constant_data->attr |= 8;
            break;
        }
    }
}

/**
 * @brief Set a model's ambient RGB multipliers.
 * @param model_id Model ID to update.
 * @param ambient_red Red ambient multiplier.
 * @param ambient_green Green ambient multiplier.
 * @param ambient_blue Blue ambient multiplier.
 */
void Hu3DModelAmbSet(s16 model_id, f32 ambient_red, f32 ambient_green, f32 ambient_blue) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    model->ambR = ambient_red;
    model->ambG = ambient_green;
    model->ambB = ambient_blue;
}

/**
 * @brief Attach another model to a named HSF object's hook slot.
 * @param parent_model_id Parent model ID containing the named object.
 * @param object_name Name of the parent object.
 * @param child_model_id Child model ID to mark as hooked.
 */
void Hu3DModelHookSet(s16 parent_model_id, char *object_name, s16 child_model_id) {
    char name[0x100];
    HU3DMODEL* child_model;
    HSFDATA* hsf;
    s16 i;
    HSFCONSTDATA *constant_data;
    HSFOBJECT* object_entry;
    HSFOBJECT* object;

    hsf = Hu3DData[parent_model_id].hsf;
    object = hsf->object;
    strcpy(&name, MakeObjectName(object_name));

    for (i = 0; i < hsf->objectNum; i++, object++) {
        object_entry = object;
        if (object_entry->constant_data != 0) {
            if (strcmp(&name, object_entry->name) == 0) {
                constant_data = object_entry->constant_data;
                constant_data->hookMdlId = child_model_id;
                child_model = &Hu3DData[child_model_id];
                child_model->attr |= HU3D_ATTR_HOOK;
                (void)child_model;
                return;
            }
        }
    }
    OSReport("Error: Not Found %s for HookSet\n", object_name);
}

/**
 * @brief Detach every child model hooked beneath a parent model.
 * @param parent_model_id Parent model ID whose object hooks are cleared.
 */
void Hu3DModelHookReset(s16 parent_model_id) {
    HSFCONSTDATA* constant_data;
    HSFDATA* hsf;
    HSFOBJECT* object_entry;
    HU3DMODEL* child_model;
    s16 object_index;
    HSFOBJECT* object;
    s16 hook_model_id;

    hsf = Hu3DData[parent_model_id].hsf;
    object = hsf->object;
    for (object_index = 0; object_index < hsf->objectNum; object_index++, object++) {
        object_entry = object;
        if (object_entry->constData != 0) {
            constant_data = object_entry->constData;
            if (constant_data->hookMdlId != -1) {
                hook_model_id = constant_data->hookMdlId;
                child_model = &Hu3DData[hook_model_id];
                child_model->attr &= ~HU3D_ATTR_HOOK;
                constant_data->hookMdlId = -1;
                (void)child_model;
            }
        }
    }
}

/**
 * @brief Detach the child model hooked to one named parent object.
 * @param parent_model_id Parent model ID containing the object.
 * @param object_name Name of the object whose hook is cleared.
 */
void Hu3DModelHookObjReset(s16 parent_model_id, char *object_name) {
    char name[0x100];
    HU3DMODEL* child_model;
    HSFDATA* hsf;
    HSFOBJECT* object_entry;
    HSFCONSTDATA* constant_data;
    s16 i;
    HSFOBJECT* object;
    s16 hook_model_id;

    hsf = Hu3DData[parent_model_id].hsf;
    object = hsf->object;
    strcpy(&name, MakeObjectName(object_name));

    for (i = 0; i < hsf->objectNum; i++, object++) {
        object_entry = object;
        if (object_entry->constData != 0) {
            if (strcmp(&name, object_entry->name) == 0) {
                constant_data = object_entry->constData;
                hook_model_id = constant_data->hookMdlId;
                child_model = &Hu3DData[hook_model_id];
                child_model->attr &= ~HU3D_ATTR_HOOK;
                constant_data->hookMdlId = -1;
                (void)child_model;
                return;
            }
        }
    }

    OSReport("Error: Not Found %s for HookReset\n", object_name);
}

/**
 * @brief Enable a projection texture on a model.
 * @param model_id Model ID to update.
 * @param projection_slot Projection slot from 0 through 3.
 */
void Hu3DModelProjectionSet(s16 model_id, s16 projection_slot) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    model->projBit = model->projBit | (1 << projection_slot);
}

/**
 * @brief Disable a projection texture on a model.
 * @param model_id Model ID to update.
 * @param projection_slot Projection slot from 0 through 3.
 */
void Hu3DModelProjectionReset(s16 model_id, s16 projection_slot) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    model->projBit &= ~(1 << projection_slot);
}

/**
 * @brief Set the highlight mode encoded in every material pass.
 * @param model_id Model ID to update.
 * @param highlight_type Highlight type encoded into the upper material-pass nibble.
 */
void Hu3DModelHiliteTypeSet(s16 model_id, s16 highlight_type) {
    HSFDATA* hsf;
    HSFMATERIAL* material;
    HU3DMODEL* model;
    HU3DMODEL* hsf_model;
    s16 i;
    s32 unused_value;

    hsf_model = &Hu3DData[model_id];
    hsf = hsf_model->hsf;
    material = hsf->material;

    highlight_type = ((s16) highlight_type) * 0x10;
    highlight_type &= 0xF0;
    for (i = 0; i < hsf->materialNum; i++, material++) {
        material->pass = material->pass & 0xF | highlight_type;
        material->flags |= 0x100;
    }
    model = &Hu3DData[model_id];
    model->attr |= HU3D_ATTR_HILITE;
    (void)model;
}

/**
 * @brief Select the reflection-map mode used by a model.
 * @param model_id Model ID to update.
 * @param reflection_type Reflection type value, or -1 for no model-specific type.
 */
void Hu3DModelReflectTypeSet(s16 model_id, s16 reflection_type) {
    HU3DMODEL *model = &Hu3DData[model_id];
    model->reflectType = reflection_type;
}

HU3DCAMERA defCamera = {
    45.0f,
    20.0f,
    5000.0f,
    0.0f,
    HU_DISP_ASPECT,
    {0.0f, 0.0f, 100.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    0, 0, HU_FB_WIDTH, HU_FB_HEIGHT,
    0.0f, 0.0f, HU_FB_WIDTH, HU_FB_HEIGHT,
    0.0f, 1.0f
};

/**
 * @brief Initialize every camera selected by a bit mask with default perspective and viewport state.
 * @param cam Camera-selection bit mask.
 */
void Hu3DCameraCreate(s32 cam) {
    s16 mask;
    s16 i;
    HU3DCAMERA* cam_ptr;

    defCamera.viewportW = RenderMode->fbWidth;
    defCamera.viewportH = RenderMode->efbHeight;
    defCamera.scissorW = RenderMode->fbWidth;
    defCamera.scissorH = RenderMode->efbHeight;
    Hu3DCameraExistF |= cam;

    for (i = 0, mask = 1; i < HU3D_CAM_MAX; i++, mask <<= 1) {
        if ((cam & mask) != 0) {
            cam_ptr = &Hu3DCamera[i];
            *cam_ptr = defCamera;
        }
    }
}

/**
 * @brief Set perspective parameters on every selected camera.
 * @param cam Camera-selection bit mask.
 * @param fov Vertical field of view in degrees.
 * @param near Near clipping-plane distance.
 * @param far Far clipping-plane distance.
 * @param aspect Viewport aspect ratio.
 */
void Hu3DCameraPerspectiveSet(s32 cam, f32 fov, f32 near, f32 far, f32 aspect) {
    s16 mask;
    s16 i;
    HU3DCAMERA* cam_ptr;

    for (i = 0, mask = 1; i < HU3D_CAM_MAX; i++, mask <<= 1) {
        if ((cam & mask) != 0) {
            cam_ptr = &Hu3DCamera[i];
            cam_ptr->fov = fov;
            cam_ptr->near = near;
            cam_ptr->far = far;
            cam_ptr->aspect = aspect;
        }
    }
}

/**
 * @brief Set viewport bounds and depth range on every selected camera.
 * @param cam Camera-selection bit mask.
 * @param vx Viewport X origin in pixels.
 * @param vy Viewport Y origin in pixels.
 * @param vw Viewport width in pixels.
 * @param vh Viewport height in pixels.
 * @param nz Near depth value.
 * @param fz Far depth value.
 */
void Hu3DCameraViewportSet(s32 cam, f32 vx, f32 vy, f32 vw, f32 vh, f32 nz, f32 fz) {
    s16 mask;
    s16 i;
    HU3DCAMERA* cam_ptr;

    for (i = 0, mask = 1; i < HU3D_CAM_MAX; i++, mask <<= 1) {
        if ((cam & mask) != 0) {
            cam_ptr = &Hu3DCamera[i];
            cam_ptr->viewportX = vx;
            cam_ptr->viewportY = vy;
            cam_ptr->viewportW = vw;
            cam_ptr->viewportH = vh;
            cam_ptr->viewportNear = nz;
            cam_ptr->viewportFar = fz;
        }
    }
}

/**
 * @brief Set scissor bounds on every selected camera.
 * @param cam Camera-selection bit mask.
 * @param x Scissor X origin in pixels.
 * @param y Scissor Y origin in pixels.
 * @param w Scissor width in pixels.
 * @param h Scissor height in pixels.
 */
void Hu3DCameraScissorSet(s32 cam, u32 x, u32 y, u32 w, u32 h) {
    s16 mask;
    s16 i;
    HU3DCAMERA* cam_ptr;

    for (i = 0, mask = 1; i < HU3D_CAM_MAX; i++, mask <<= 1) {
        if ((cam & mask) != 0) {
            cam_ptr = &Hu3DCamera[i];
            cam_ptr->scissorX = x;
            cam_ptr->scissorY = y;
            cam_ptr->scissorW = w;
            cam_ptr->scissorH = h;
        }
    }
}

/**
 * @brief Set position, up vector, and target on every selected camera.
 * @param cam Camera-selection bit mask.
 * @param x Camera-position X coordinate.
 * @param y Camera-position Y coordinate.
 * @param z Camera-position Z coordinate.
 * @param ux Up-vector X component.
 * @param uy Up-vector Y component.
 * @param uz Up-vector Z component.
 * @param tx Target X coordinate.
 * @param ty Target Y coordinate.
 * @param tz Target Z coordinate.
 */
void Hu3DCameraPosSet(s32 cam, f32 x, f32 y, f32 z, f32 ux, f32 uy, f32 uz, f32 tx, f32 ty, f32 tz) {
    s16 mask;
    s16 i;
    HU3DCAMERA* cam_ptr;

    for (i = 0, mask = 1; i < HU3D_CAM_MAX; i++, mask <<= 1) {
        if ((cam & mask) != 0) {
            cam_ptr = &Hu3DCamera[i];
            cam_ptr->pos.x = x;
            cam_ptr->pos.y = y;
            cam_ptr->pos.z = z;
            cam_ptr->up.x = ux;
            cam_ptr->up.y = uy;
            cam_ptr->up.z = uz;
            cam_ptr->target.x = tx;
            cam_ptr->target.y = ty;
            cam_ptr->target.z = tz;
        }
    }
}

/**
 * @brief Set position, up vector, and target on every selected camera from vectors.
 * @param cam Camera-selection bit mask.
 * @param pos Camera position.
 * @param up Camera up vector.
 * @param target Camera target position.
 */
void Hu3DCameraPosSetV(s32 cam, Vec *pos, Vec *up, Vec *target) {
    s16 mask;
    s16 i;
    HU3DCAMERA* cam_ptr;

    for (i = 0, mask = 1; i < HU3D_CAM_MAX; i++, mask <<= 1) {
        if ((cam & mask) != 0) {
            cam_ptr = &Hu3DCamera[i];
            cam_ptr->pos = *pos;
            cam_ptr->up = *up;
            cam_ptr->target = *target;
        }
    }
}

/**
 * @brief Deactivate every camera selected by a bit mask.
 *
 * @details A deactivated camera is marked by setting its field of view to -1.
 * @param cam Camera-selection bit mask.
 */
void Hu3DCameraKill(s32 cam) {
    s16 mask;
    s16 i;
    HU3DCAMERA* cam_ptr;

    for (i = 0, mask = 1; i < HU3D_CAM_MAX; i++, mask <<= 1) {
        if ((cam & mask) != 0) {
            cam_ptr = &Hu3DCamera[i];
            cam_ptr->fov = -1.0f;
        }
    }
}

/**
 * @brief Deactivate all Hu3D cameras.
 */
void Hu3DCameraAllKill(void) {
    HU3DCAMERA* cam_ptr;
    HU3DCAMERA* cam_ptr2;
    s16 i;
    s16 mask;
    s16 j;
    s16 mask2;

    cam_ptr = &Hu3DCamera[0];
    for (i = 0, mask = 1; i < HU3D_CAM_MAX; i++, mask <<= 1, cam_ptr++) {
        if (-1.0f != cam_ptr->fov) {
           for (j = 0, mask2 = 1; j < HU3D_CAM_MAX; j++, mask2 <<= 1) {
                if ((mask & mask2) != 0) {
                    cam_ptr2 = &Hu3DCamera[j];
                    cam_ptr2->fov = -1.0f;
                }
            }
        }
    }
    Hu3DCameraExistF = 0;
}

/**
 * @brief Load one camera's GX perspective, viewport, scissor, and view matrix.
 * @param camera_index Camera index to activate for drawing.
 * @param view_mtx Output view matrix populated from camera position, up vector, and target.
 */
void Hu3DCameraSet(s32 camera_index, Mtx view_mtx) {
    Mtx44 projection_mtx;
    Mtx44 unused_mtx;
    HU3DCAMERA* camera;

    camera = &Hu3DCamera[camera_index];
    C_MTXPerspective(projection_mtx, camera->fov, camera->aspect, camera->near, camera->far);
    GXSetProjection(projection_mtx, GX_PERSPECTIVE);
    if (RenderMode->field_rendering != 0) {
        GXSetViewportJitter(camera->viewportX, camera->viewportY, camera->viewportW, camera->viewportH, camera->viewportNear, camera->viewportFar, VIGetNextField());
    } else {
        GXSetViewport(camera->viewportX, camera->viewportY, camera->viewportW, camera->viewportH, camera->viewportNear, camera->viewportFar);
    }
    GXSetScissor(camera->scissorX, camera->scissorY, camera->scissorW, camera->scissorH);
    C_MTXLookAt(view_mtx, &camera->pos, &camera->up, &camera->target);
}

/**
 * @brief Initialize a camera from the first HSF camera object in a model.
 * @param model_id Model ID whose HSF camera object is read.
 * @param camera_index Camera index to initialize and store on the model.
 * @return TRUE if a camera object was found and applied; FALSE otherwise.
 */
BOOL Hu3DModelCameraInfoSet(s16 model_id, u16 camera_index) {
    HU3DCAMERA* cam;
    HU3DCAMERA* unused_camera_a;
    HU3DCAMERA* unused_camera_b;
    HSFOBJECT* obj_copy;
    HU3DMODEL* camera_model;
    HU3DMODEL* model;
    HSFOBJECT* object;
    HSFDATA* hsf;
    Point3d rotated_up;
    Point3d view_direction;
    f32 up_rotation;
    s16 i;
    s16 unused_index_a;
    s16 unused_index_b;
    s16 unused_mask_a;
    s16 unused_mask_b;

    model = &Hu3DData[model_id];
    hsf = model->hsf;
    cam = &Hu3DCamera[camera_index];
    object = hsf->object;

    for (i = 0; i < hsf->objectNum; i++, object++) {
        obj_copy = object;
        if (obj_copy->type == 7) {
            up_rotation = obj_copy->mesh.base.rot.x;
            cam->upRot = up_rotation;

            VECSubtract((Point3d* ) &obj_copy->camera.pos, (Point3d* ) &obj_copy->camera.target, &view_direction);

            rotated_up.x = ((view_direction.x * view_direction.y * (1.0 - cosd(up_rotation))) - (view_direction.z * sind(up_rotation)));
            rotated_up.y = ((view_direction.y * view_direction.y) + (1.0f - (view_direction.y * view_direction.y)) * cosd(up_rotation));
            rotated_up.z = (((view_direction.y * view_direction.z) * (1.0 - cosd(up_rotation))) + (view_direction.x * sind(up_rotation)));
            VECNormalize(&rotated_up, &view_direction);

            Hu3DCameraPosSet(camera_index, obj_copy->camera.pos.x, obj_copy->camera.pos.y, obj_copy->camera.pos.z,
                             view_direction.x, view_direction.y, view_direction.z,
                             obj_copy->camera.target.x, obj_copy->camera.target.y, obj_copy->camera.target.z);

            Hu3DCameraPerspectiveSet(camera_index, obj_copy->camera.fov, obj_copy->camera.near, obj_copy->camera.far, HU_DISP_ASPECT);

            model->camInfoBit = camera_index;
            camera_model = &Hu3DData[model_id];
            camera_model->attr |= HU3D_ATTR_CAMERA_MOTON;
            (void)camera_model;
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Create a camera-motion model backed by an existing motion.
 * @param motion_id Motion ID containing camera tracks.
 * @param camera_mask Camera-selection bit mask driven by the new model.
 * @return The allocated model ID, or -1 when no model slot is available.
 */
s16 Hu3DModelCameraCreate(s16 motion_id, u16 camera_mask) {
    HU3DMODEL* model;
    s16 model_id;

    model_id = Hu3DHookFuncCreate((HU3DMODELHOOK)-1);
    model = &Hu3DData[(s16) model_id];
    model->attr &= ~HU3D_ATTR_HOOKFUNC;
    model->attr |= HU3D_ATTR_CAMERA | HU3D_ATTR_CAMERA_MOTON;
    model->motId = motion_id;
    model->camInfoBit = camera_mask;
    return model_id;
}

/**
 * @brief Enable camera-track evaluation for a model.
 * @param model_id Model ID containing camera tracks.
 * @param camera_mask Camera-selection bit mask driven by the model.
 */
void Hu3DCameraMotionOn(s16 model_id, s8 camera_mask) {
    HU3DMODEL* attribute_model;
    HU3DMODEL* camera_model;

    camera_model = &Hu3DData[model_id];
    camera_model->camInfoBit = camera_mask;
    attribute_model = &Hu3DData[model_id];
    attribute_model->attr |= HU3D_ATTR_CAMERA_MOTON;
}

/**
 * @brief Enable and rewind camera motion on a model.
 * @param model_id Model ID containing camera tracks.
 * @param camera_mask Camera-selection bit mask driven by the model.
 */
void Hu3DCameraMotionStart(s16 model_id, u16 camera_mask) {
    HU3DMODEL* motion_model;
    HU3DMODEL* attribute_model;
    HU3DMODEL* model;
    HU3DMODEL* camera_model;

    model = &Hu3DData[model_id];
    camera_model = &Hu3DData[model_id];
    camera_model->camInfoBit = camera_mask;
    attribute_model = &Hu3DData[model_id];
    attribute_model->attr |= HU3D_ATTR_CAMERA_MOTON;
    motion_model = &Hu3DData[model_id];
    motion_model->motAttr &= ~HU3D_MOTATTR_PAUSE;
    Hu3DMotionStartEndSet(model_id, 0.0f, Hu3DMotionMotionMaxTimeGet(model->motId));
    Hu3DMotionTimeSet(model_id, 0.0f);
}

/**
 * @brief Disable camera-track evaluation for a model.
 * @param model_id Model ID whose camera-motion attribute is cleared.
 */
void Hu3DCameraMotionOff(s16 model_id) {
    HU3DMODEL* model;

    model = &Hu3DData[model_id];
    model->attr &= ~HU3D_ATTR_CAMERA_MOTON;
}

/**
 * @brief Mark every global and local Hu3D light slot as unused.
 */
void Hu3DLighInit(void) {
    HU3DLIGHT* light;
    s16 light_index;

    light = Hu3DGlobalLight;
    for (light_index = 0; light_index < 8; light_index++, light++) {
        light->type = -1;
    }
    light = Hu3DLocalLight;
    for (light_index = 0; light_index < 0x20; light_index++, light++) {
        light->type = -1;
    }
}


/**
 * @brief Create a global light from scalar position, direction, and color values.
 * @param position_x Light-position X coordinate.
 * @param position_y Light-position Y coordinate.
 * @param position_z Light-position Z coordinate.
 * @param direction_x Direction or aim X component.
 * @param direction_y Direction or aim Y component.
 * @param direction_z Direction or aim Z component.
 * @param red Red component from 0 through 255.
 * @param green Green component from 0 through 255.
 * @param blue Blue component from 0 through 255.
 * @return The global-light index from 0 through 7, or -1 if all slots are occupied.
 */
s16 Hu3DGLightCreate(f32 position_x, f32 position_y, f32 position_z, f32 direction_x, f32 direction_y, f32 direction_z, u8 red, u8 green, u8 blue) {
    Vec vec1;
    Vec vec2;
    GXColor color;

    vec1.x = position_x;
    vec1.y = position_y;
    vec1.z = position_z;
    vec2.x = direction_x;
    vec2.y = direction_y;
    vec2.z = direction_z;
    color.r = red;
    color.g = green;
    color.b = blue;
    color.a = 0xFF;
    return Hu3DGLightCreateV(&vec1, &vec2, &color);
}

/**
 * @brief Initialize a Hu3D light as a normalized spot light.
 * @param light Light structure to initialize.
 * @param position Light position.
 * @param direction Direction vector, normalized before storage.
 * @param color RGBA light color.
 * @return Indeterminate; the original non-void routine reaches the end without returning a value.
 */
inline s16 Hu3DLightCreateV(HU3DLIGHT *light, Vec *position, Vec *direction, GXColor *color) {
    light->type = 0;
    light->pos = *position;
    light->dir = *direction;
    light->offset.x = light->offset.y = light->offset.z = 0.0f;
    light->cutoff = 30.0f;
    light->func = 2;
    VECNormalize(&light->dir, &light->dir);
    light->color = *color;
}

/**
 * @brief Create a global light from vectors.
 * @param pos Light position.
 * @param dir Direction vector, normalized before storage.
 * @param color RGBA light color.
 * @return The global-light index from 0 through 7, or -1 if all slots are occupied.
 */
s16 Hu3DGLightCreateV(Vec* pos, Vec* dir, GXColor* color) {
    s16 light_id;
    HU3DLIGHT* light;

    light = Hu3DGlobalLight;

    for (light_id = 0; light_id < 8; light_id++, light++) {
        if (light->type == -1) {
            break;
        }
    }
    if (light_id == 8) {
        return -1;
    }

    Hu3DLightCreateV(light, pos, dir, color);

    return light_id;
}

/**
 * @brief Create a model-local light from scalar position, direction, and color values.
 * @param model_id Model ID that will own the light.
 * @param position_x Light-position X coordinate.
 * @param position_y Light-position Y coordinate.
 * @param position_z Light-position Z coordinate.
 * @param direction_x Direction or aim X component.
 * @param direction_y Direction or aim Y component.
 * @param direction_z Direction or aim Z component.
 * @param red Red component from 0 through 255.
 * @param green Green component from 0 through 255.
 * @param blue Blue component from 0 through 255.
 * @return The model-local light slot from 0 through 7, or -1 if no global local-light storage or model slot is free.
 */
s16 Hu3DLLightCreate(s16 model_id, f32 position_x, f32 position_y, f32 position_z, f32 direction_x, f32 direction_y, f32 direction_z, u8 red, u8 green, u8 blue) {
    Vec vec1;
    Vec vec2;
    GXColor color;

    vec1.x = position_x;
    vec1.y = position_y;
    vec1.z = position_z;
    vec2.x = direction_x;
    vec2.y = direction_y;
    vec2.z = direction_z;
    color.r = red;
    color.g = green;
    color.b = blue;
    color.a = 0xFF;
    return Hu3DLLightCreateV(model_id, &vec1, &vec2, &color);
}

/**
 * @brief Create a model-local light from vectors.
 * @param model_id Model ID that will own the light.
 * @param position Light position.
 * @param direction Direction vector, normalized before storage.
 * @param color RGBA light color.
 * @return The model-local light slot from 0 through 7, or -1 if no storage or model slot is free.
 */
s16 Hu3DLLightCreateV(s16 model_id, Vec* position, Vec* direction, GXColor* color) {
    HU3DMODEL* model;
    s16 slot_index;
    s16 light_id;
    HU3DLIGHT* light;

    model = &Hu3DData[model_id];
    light = Hu3DLocalLight;
    for (slot_index = 0; slot_index < 0x20; slot_index++, light++) {
        if (light->type == -1) {
            break;
        }
    }
    if (slot_index == 0x20) {
        return -1;
    }

    Hu3DLightCreateV(light, position, direction, color);

    for (light_id = 0; light_id < 8; light_id++) {
        if (model->lLightId[light_id] == -1) {
            break;
        }
    }
    if (light_id == 8) {
        return -1;
    }
    model->lLightId[light_id] = slot_index;
    model->attr |= HU3D_ATTR_LLIGHT;
    return light_id;
}

/**
 * @brief Configure a Hu3D light for spot attenuation.
 * @param light Light structure to update.
 * @param cutoff Spot cutoff angle in degrees.
 * @param spot_function GX spot-function selector.
 */
inline void Hu3DLightSpotSet(HU3DLIGHT *light, u16 spot_function, f32 cutoff) {
    light->type &= 0xFF00;
    light->cutoff = cutoff;
    light->func = spot_function;
}

/**
 * @brief Configure a Hu3D light for spot attenuation.
 * @param light_index Global-light index.
 * @param cutoff Spot cutoff angle in degrees.
 * @param spot_function GX spot-function selector.
 */
void Hu3DGLightSpotSet(s16 light_index, f32 cutoff, u16 spot_function) {
    HU3DLIGHT *light = &Hu3DGlobalLight[light_index];

    Hu3DLightSpotSet(light, spot_function, cutoff);
}

/**
 * @brief Configure a Hu3D light for spot attenuation.
 * @param model_id Model ID that owns the light.
 * @param light_slot Model-local light slot.
 * @param cutoff Spot cutoff angle in degrees.
 * @param spot_function GX spot-function selector.
 */
void Hu3DLLightSpotSet(s16 model_id, s16 light_slot, f32 cutoff, u16 spot_function) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[model_id];
    light = &Hu3DLocalLight[data->lLightId[light_slot]];
    Hu3DLightSpotSet(light, spot_function, cutoff);
}

/**
 * @brief Configure a Hu3D light as an infinite directional light.
 * @param light Light structure to update.
 */
inline void Hu3DLightInfinitytSet(HU3DLIGHT *light) {
    light->type &= 0xFF00;
    light->type |= 1;
}

/**
 * @brief Configure a Hu3D light as an infinite directional light.
 * @param light_index Global-light index.
 */
void Hu3DGLightInfinitytSet(s16 light_index) {
    HU3DLIGHT* light = &Hu3DGlobalLight[light_index];

    Hu3DLightInfinitytSet(light);
}

/**
 * @brief Configure a Hu3D light as an infinite directional light.
 * @param model_id Model ID that owns the light.
 * @param light_slot Model-local light slot.
 */
void Hu3DLLightInfinitytSet(s16 model_id, s16 light_slot) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[model_id];
    light = &Hu3DLocalLight[data->lLightId[light_slot]];
    Hu3DLightInfinitytSet(light);
}

/**
 * @brief Configure a Hu3D light for point-distance attenuation.
 * @param light Light structure to update.
 * @param reference_distance Reference distance used by GX attenuation.
 * @param brightness Brightness at the reference distance.
 * @param distance_function GX distance-attenuation function selector.
 */
inline void Hu3DLightPointSet(HU3DLIGHT *light, f32 reference_distance, f32 brightness, u16 distance_function) {
    light->type &= 0xFF00;
    light->type |= 2;
    light->cutoff = reference_distance;
    light->brightness = brightness;
    light->func = distance_function;
}

/**
 * @brief Configure a Hu3D light for point-distance attenuation.
 * @param light_index Global-light index.
 * @param reference_distance Reference distance used by GX attenuation.
 * @param brightness Brightness at the reference distance.
 * @param distance_function GX distance-attenuation function selector.
 */
void Hu3DGLightPointSet(s16 light_index, f32 reference_distance, f32 brightness, u16 distance_function) {
    HU3DLIGHT* light = &Hu3DGlobalLight[light_index];

    Hu3DLightPointSet(light, reference_distance, brightness, distance_function);
}

/**
 * @brief Configure a Hu3D light for point-distance attenuation.
 * @param model_id Model ID that owns the light.
 * @param light_slot Model-local light slot.
 * @param reference_distance Reference distance used by GX attenuation.
 * @param brightness Brightness at the reference distance.
 * @param distance_function GX distance-attenuation function selector.
 */
void Hu3DLLightPointSet(s16 model_id, s16 light_slot, f32 reference_distance, f32 brightness, u16 distance_function) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[model_id];
    light = &Hu3DLocalLight[data->lLightId[light_slot]];
    Hu3DLightPointSet(light, reference_distance, brightness, distance_function);
}

/**
 * @brief Mark a global-light slot as unused.
 * @param index Global-light index to release.
 */
void Hu3DGLightKill(s16 index) {
    Hu3DGlobalLight[index].type = -1;
}

/**
 * @brief Release a model-local light and clear its owning slot.
 * @param model_id Model ID that owns the light.
 * @param light_slot Model-local light slot to release.
 */
void Hu3DLLightKill(s16 model_id, s16 light_slot) {
    HU3DMODEL* model;
    HU3DLIGHT* light;
    s16 global_light_id;

    model = &Hu3DData[model_id];
    light = &Hu3DLocalLight[model->lLightId[light_slot]];
    light->type = -1;
    model->lLightId[light_slot] = -1;

    for (global_light_id = 0; global_light_id < 8; global_light_id++) {
        if (model->lLightId[global_light_id] == -1) {
            break;
        }
    }
    if (global_light_id == 8) {
        model->attr &= ~HU3D_ATTR_LLIGHT;
    }
}

/**
 * @brief Mark every populated global-light slot as unused.
 */
void Hu3DLightAllKill(void) {
    s16 i;
    HU3DLIGHT* light;

    light = Hu3DGlobalLight;
    for (i = 0; i < 8; i++, light++) {
        if (light->type != -1) {
            Hu3DGlobalLight[i].type = -1;
        }
    }
}

/**
 * @brief Set a Hu3D light's RGBA color.
 * @param light Light structure to update.
 * @param r Red component from 0 through 255.
 * @param g Green component from 0 through 255.
 * @param b Blue component from 0 through 255.
 * @param a Alpha component from 0 through 255.
 */
inline void Hu3DLightColorSet(HU3DLIGHT *light, u8 r, u8 g, u8 b, u8 a) {
    light->color.r = r;
    light->color.g = g;
    light->color.b = b;
    light->color.a = a;
}

/**
 * @brief Set a Hu3D light's RGBA color.
 * @param index Global-light index.
 * @param r Red component from 0 through 255.
 * @param g Green component from 0 through 255.
 * @param b Blue component from 0 through 255.
 * @param a Alpha component from 0 through 255.
 */
void Hu3DGLightColorSet(s16 index, u8 r, u8 g, u8 b, u8 a) {
    HU3DLIGHT* light = &Hu3DGlobalLight[index];

    Hu3DLightColorSet(light, r, g, b, a);
}

/**
 * @brief Set a Hu3D light's RGBA color.
 * @param model_id Model ID that owns the light.
 * @param light_slot Model-local light slot.
 * @param r Red component from 0 through 255.
 * @param g Green component from 0 through 255.
 * @param b Blue component from 0 through 255.
 * @param a Alpha component from 0 through 255.
 */
void Hu3DLLightColorSet(s16 model_id, s16 light_slot, u8 r, u8 g, u8 b, u8 a) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[model_id];
    light = &Hu3DLocalLight[data->lLightId[light_slot]];
    Hu3DLightColorSet(light, r, g, b, a);
}

/**
 * @brief Set a light position and normalized direction from vectors.
 * @param light Light structure to update.
 * @param pos Light position.
 * @param aim Direction vector; despite the name, it is normalized directly rather than treated as a target point.
 */
inline void Hu3DLightPosSetV(HU3DLIGHT *light, Vec* pos, Vec* aim) {
    light->pos = *pos;
    VECNormalize(aim, &light->dir);
}

/**
 * @brief Set a light position and normalized direction from vectors.
 * @param index Global-light index.
 * @param pos Light position.
 * @param aim Direction vector; despite the name, it is normalized directly rather than treated as a target point.
 */
void Hu3DGLightPosSetV(s16 index, Vec* pos, Vec* aim) {
    HU3DLIGHT* light = &Hu3DGlobalLight[index];

    Hu3DLightPosSetV(light, pos, aim);
}

/**
 * @brief Set a light position and normalized direction from vectors.
 * @param model_id Model ID that owns the light.
 * @param light_slot Model-local light slot.
 * @param pos Light position.
 * @param aim Direction vector; despite the name, it is normalized directly rather than treated as a target point.
 */
void Hu3DLLightPosSetV(s16 model_id, s16 light_slot, Vec* pos, Vec* aim) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[model_id];
    light = &Hu3DLocalLight[data->lLightId[light_slot]];
    Hu3DLightPosSetV(light, pos, aim);
}

/**
 * @brief Set a light position and normalized direction from scalar components.
 * @param light Light structure to update.
 * @param position_x Light-position X coordinate.
 * @param position_y Light-position Y coordinate.
 * @param position_z Light-position Z coordinate.
 * @param direction_x Direction-vector X component.
 * @param direction_y Direction-vector Y component.
 * @param direction_z Direction-vector Z component.
 */
inline void Hu3DLightPosSet(HU3DLIGHT *light, f32 position_x, f32 position_y, f32 position_z, f32 direction_x, f32 direction_y, f32 direction_z) {
    light->pos.x = position_x;
    light->pos.y = position_y;
    light->pos.z = position_z;
    light->dir.x = direction_x;
    light->dir.y = direction_y;
    light->dir.z = direction_z;
    VECNormalize(&light->dir, &light->dir);
}

/**
 * @brief Set a global light position and normalized direction from scalar components.
 * @param light_index Global-light index.
 * @param position_x Light-position X coordinate.
 * @param position_y Light-position Y coordinate.
 * @param position_z Light-position Z coordinate.
 * @param direction_x Direction-vector X component.
 * @param direction_y Direction-vector Y component.
 * @param direction_z Direction-vector Z component.
 */
void Hu3DGLightPosSet(s16 light_index, f32 position_x, f32 position_y, f32 position_z, f32 direction_x, f32 direction_y, f32 direction_z) {
    HU3DLIGHT* light;

    light = &Hu3DGlobalLight[light_index];
    Hu3DLightPosSet(light, position_x, position_y, position_z, direction_x, direction_y, direction_z);
}

/**
 * @brief Set a model-local light position and normalized direction from scalar components.
 * @param model_id Model ID that owns the light.
 * @param light_slot Model-local light slot.
 * @param position_x Light-position X coordinate.
 * @param position_y Light-position Y coordinate.
 * @param position_z Light-position Z coordinate.
 * @param direction_x Direction-vector X component.
 * @param direction_y Direction-vector Y component.
 * @param direction_z Direction-vector Z component.
 */
void Hu3DLLightPosSet(s16 model_id, s16 light_slot, f32 position_x, f32 position_y, f32 position_z, f32 direction_x, f32 direction_y, f32 direction_z) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[model_id];
    light = &Hu3DLocalLight[data->lLightId[light_slot]];
    Hu3DLightPosSet(light, position_x, position_y, position_z, direction_x, direction_y, direction_z);
}

/**
 * @brief Set a light position and aim it at a target point.
 * @param light Light structure to update.
 * @param pos Light position.
 * @param aim Target point used to derive the normalized direction.
 */
inline void Hu3DLightPosAimSetV(HU3DLIGHT *light, Vec* pos, Vec* aim) {
    light->pos = *pos;
    VECSubtract(aim, pos, &light->dir);
    VECNormalize(&light->dir, &light->dir);
}

/**
 * @brief Set a light position and aim it at a target point.
 * @param index Global-light index.
 * @param pos Light position.
 * @param aim Target point used to derive the normalized direction.
 */
void Hu3DGLightPosAimSetV(s16 index, Vec* pos, Vec* aim) {
    HU3DLIGHT* light = &Hu3DGlobalLight[index];

    Hu3DLightPosAimSetV(light, pos, aim);
}

/**
 * @brief Set a light position and aim it at a target point.
 * @param model_id Model ID that owns the light.
 * @param light_slot Model-local light slot.
 * @param pos Light position.
 * @param aim Target point used to derive the normalized direction.
 */
void Hu3DLLightPosAimSetV(s16 model_id, s16 light_slot, Vec* pos, Vec* aim) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[model_id];
    light = &Hu3DLocalLight[data->lLightId[light_slot]];
    Hu3DLightPosAimSetV(light, pos, aim);
}

/**
 * @brief Set a global light position and aim point from scalar components.
 * @param light_index Global-light index.
 * @param position_x Light-position X coordinate.
 * @param position_y Light-position Y coordinate.
 * @param position_z Light-position Z coordinate.
 * @param target_x Target X coordinate.
 * @param target_y Target Y coordinate.
 * @param target_z Target Z coordinate.
 */
void Hu3DGLightPosAimSet(s16 light_index, f32 position_x, f32 position_y, f32 position_z, f32 target_x, f32 target_y, f32 target_z) {
    Vec pos;
    Vec aim;
    HU3DLIGHT *light;
    HU3DLIGHT *light2;

    pos.x = position_x;
    pos.y = position_y;
    pos.z = position_z;
    aim.x = target_x;
    aim.y = target_y;
    aim.z = target_z;

    light = &Hu3DGlobalLight[light_index];
    light2 = light;
    Hu3DLightPosAimSetV(light2, &pos, &aim);
}

/**
 * @brief Set a model-local light position and aim point from scalar components.
 * @param model_id Model ID that owns the light.
 * @param light_slot Model-local light slot.
 * @param position_x Light-position X coordinate.
 * @param position_y Light-position Y coordinate.
 * @param position_z Light-position Z coordinate.
 * @param target_x Target X coordinate.
 * @param target_y Target Y coordinate.
 * @param target_z Target Z coordinate.
 */
void Hu3DLLightPosAimSet(s16 model_id, s16 light_slot, f32 position_x, f32 position_y, f32 position_z, f32 target_x, f32 target_y, f32 target_z) {
    Vec pos;
    Vec aim;
    HU3DMODEL* data;
    HU3DLIGHT* light;
    HU3DLIGHT* light2;

    pos.x = position_x;
    pos.y = position_y;
    pos.z = position_z;
    aim.x = target_x;
    aim.y = target_y;
    aim.z = target_z;

    data = &Hu3DData[model_id];
    light = &Hu3DLocalLight[data->lLightId[light_slot]];
    light2 = light;
    Hu3DLightPosAimSetV(light2, &pos, &aim);
}

/**
 * @brief Control whether a light is transformed by the supplied model matrices.
 * @param light Light structure to update.
 * @param transform_with_matrices Nonzero to mark the light for matrix transformation; zero to keep coordinates unchanged.
 */
inline void Hu3DLightStaticSet(HU3DLIGHT *light, s32 transform_with_matrices) {
    if (transform_with_matrices != 0) {
        light->type |= 0x8000;
    } else {
        light->type &= ~0x8000;
    }
}

/**
 * @brief Control matrix transformation for a global light.
 * @param index Global-light index.
 * @param transform_with_matrices Nonzero to transform the light by draw matrices; zero to use stored coordinates.
 */
void Hu3DGLightStaticSet(s16 index, s32 transform_with_matrices) {
    HU3DLIGHT* light = &Hu3DGlobalLight[index];

    Hu3DLightStaticSet(light, transform_with_matrices);
}

/**
 * @brief Control matrix transformation for a model-local light.
 * @param model_id Model ID that owns the light.
 * @param light_slot Model-local light slot.
 * @param transform_with_matrices Nonzero to transform the light by draw matrices; zero to use stored coordinates.
 */
void Hu3DLLightStaticSet(s16 model_id, s16 light_slot, s32 transform_with_matrices) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[model_id];
    light = &Hu3DLocalLight[data->lLightId[light_slot]];
    Hu3DLightStaticSet(light, transform_with_matrices);
}

/**
 * @brief Create global lights from the HSF light objects in a model.
 * @param model_id Model ID whose HSF object table is scanned.
 * @param transform_with_matrices Nonzero to mark created lights for matrix transformation.
 * @return The number of created lights, capped at eight; an existing nonzero count is returned unchanged.
 */
s32 Hu3DModelLightInfoSet(s16 model_id, s16 transform_with_matrices) {
    Vec direction;
    Vec unused_pos;
    Vec unused_target;
    HU3DLIGHT* light;
    s16 light_id;
    u8 red;
    u8 green;
    u8 blue;
    HSFDATA* hsf;
    HSFOBJECT* light_object;
    HSFOBJECT* object;
    HU3DMODEL* model;
    s16 object_index;
    s16 light_count;

    model = &Hu3DData[model_id];
    hsf = model->hsf;
    if (model->lightNum != 0) {
        return model->lightNum;
    }
    object = hsf->object;

    for (object_index = light_count = 0; object_index < hsf->objectNum; object_index++, object++) {
        light_object = object;
        if (light_object->type != 8) {
            continue;
        }
        direction.x = light_object->light.target.x - light_object->light.pos.x;
        direction.y = light_object->light.target.y - light_object->light.pos.y;
        direction.z = light_object->light.target.z - light_object->light.pos.z;
        blue = light_object->light.b;
        green = light_object->light.g;
        red = light_object->light.r;
        light_id = Hu3DGLightCreate(light_object->light.pos.x, light_object->light.pos.y, light_object->light.pos.z,
                                direction.x, direction.y, direction.z, red, green, blue);


        model->lightId[light_count] = light_id;
        light = &Hu3DGlobalLight[light_id];
        Hu3DGLightStaticSet(light_id, transform_with_matrices);
        switch (light_object->light.type) {
            case 0:
                Hu3DGLightSpotSet(light_id, light_object->light.cutoff, 2);
                break;
            case 1:
                Hu3DGLightPointSet(light_id, light_object->mesh.base.scale.x - light_object->mesh.base.rot.z, 1.0f, 2);
                Hu3DGLightPosSet(light_id, light_object->light.pos.x, light_object->light.pos.y, light_object->light.pos.z, light_object->light.target.x, light_object->light.target.y, light_object->light.target.z);
                break;
            case 2:
                Hu3DGLightInfinitytSet(light_id);
                break;
        }
        light_count++;
        if (light_count >= 8) {
            break;
        }
    }
    model->lightNum = light_count;
    return light_count;
}

/**
 * @brief Configure and load all active global and model-local GX lights for a draw.
 * @param model Model whose local lights are included.
 * @param position_mtx Position transformation matrix.
 * @param direction_mtx Direction transformation matrix.
 * @param specular_control Specular exponent control; zero selects ordinary light direction setup.
 * @return Bit mask of GX light slots loaded by the routine.
 */
s16 Hu3DLightSet(HU3DMODEL* model, Mtx *position_mtx, Mtx *direction_mtx, f32 specular_control) {
    s16 light_bit;
    HU3DLIGHT* light;
    s16 light_mask;
    s16 i;

    light_mask = 0;
    light_bit = 1;
    light = Hu3DGlobalLight;

    for (i = 0; i < 8; i++, light++) {
        if (light->type != -1) {
            lightSet(light, light_bit, direction_mtx, position_mtx, specular_control);
            light_mask |= light_bit;
            light_bit <<= 1;
        }
    }
    if ((model->attr & HU3D_ATTR_LLIGHT) != 0) {
        for (i = 0; i < 8; i++) {
            if (model->lLightId[i] != -1) {
                light = &Hu3DLocalLight[model->lLightId[i]];
                lightSet(light, light_bit, direction_mtx, position_mtx, specular_control);
                light_mask |= light_bit;
                light_bit <<= 1;
            }
        }
    }
    return light_mask;
}

/**
 * @brief Configure and immediately load one GX light object.
 * @param light Hu3D light state to convert.
 * @param light_bit GX light-slot bit.
 * @param direction_mtx Matrix used to transform directions for flagged lights.
 * @param position_mtx Matrix used to transform positions for flagged lights.
 * @param specular_control Specular exponent control; zero uses diffuse direction setup.
 */
void lightSet(HU3DLIGHT* light, s16 light_bit, Mtx *direction_mtx, Mtx *position_mtx, f32 specular_control) {
    GXLightObj gx_light;
    Point3d direction;
    Point3d position;

    switch ((u8)light->type) {
    case 0:
        GXInitLightAttn(&gx_light, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        GXInitLightSpot(&gx_light, light->cutoff, light->func);
        break;
    case 1:
        GXInitLightAttn(&gx_light, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        GXInitLightSpot(&gx_light, 20.0f, GX_SP_COS);
        GXInitLightAttnK(&gx_light, 1.0f, 0.0f, 0.0f);
        VECScale(&light->dir, &light->pos, -1000000.0f);
        break;
    case 2:
        GXInitLightAttn(&gx_light, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        GXInitLightDistAttn(&gx_light, light->cutoff, light->brightness, light->func);
        break;
    }
    if ((light->type & 0x8000) != 0) {
        PSMTXMultVec(*direction_mtx, &light->dir, &direction);
        PSMTXMultVec(*position_mtx, &light->pos, &position);
        GXInitLightPos(&gx_light, position.x, position.y, position.z);
    } else {
        GXInitLightPos(&gx_light, light->pos.x, light->pos.y, light->pos.z);
        direction = light->dir;
    }
    if (0.0f == specular_control) {
        GXInitLightDir(&gx_light, direction.x, direction.y, direction.z);
    } else {
        GXInitSpecularDir(&gx_light, direction.x, direction.y, direction.z);
        GXInitLightAttn(&gx_light, 0.0f, 0.0f, 1.0f, specular_control / 2, 0.0f, 1.0f - (specular_control / 2));
    }
    GXInitLightColor(&gx_light, light->color);
    GXLoadLightObjImm(&gx_light, light_bit);
}

/**
 * @brief Replace the first reflection-map animation.
 * @param reflection_anim Animation data to use as reflection map zero; the previous non-static animation is freed.
 */
void Hu3DReflectMapSet(ANIMDATA* reflection_anim) {

    if (reflectAnim[0] != (ANIMDATA*) refMapData0) {
        HuMemDirectFree(reflectAnim[0]);
    }
    reflectAnim[0] = HuSprAnimRead(reflection_anim);
    reflectMapNo = 0;
}

/**
 * @brief Select the active built-in reflection map.
 * @param reflection_index Reflection-map index.
 */
void Hu3DReflectNoSet(s16 reflection_index) {
    reflectMapNo = reflection_index;
}

/**
 * @brief Enable linear fog with supplied distances and color.
 * @param fog_start Fog start distance.
 * @param fog_end Fog end distance.
 * @param red Red fog component from 0 through 255.
 * @param green Green fog component from 0 through 255.
 * @param blue Blue fog component from 0 through 255.
 */
void Hu3DFogSet(f32 fog_start, f32 fog_end, u8 red, u8 green, u8 blue) {
    FogData.fogType = 4;
    FogData.fogStart = fog_start;
    FogData.fogEnd = fog_end;
    FogData.color.r = red;
    FogData.color.g = green;
    FogData.color.b = blue;
    FogData.color.a = 0xFF;
}

/**
 * @brief Disable Hu3D fog and reset its distances and color to zero.
 */
void Hu3DFogClear(void) {
    FogData.fogType = 0;
    GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, BGColor);
}

/**
 * @brief Create or recreate the shadow texture and projection settings.
 *
 * @details The shadow buffer is allocated at the current shadow-map width and height.
 * @param fov Shadow-camera field of view in degrees.
 * @param near Near clipping-plane distance.
 * @param far Far clipping-plane distance.
 */
void Hu3DShadowCreate(f32 fov, f32 near, f32 far) {
    Hu3DShadowData.size = 0xC0;
    if (Hu3DShadowData.buf == 0) {
        Hu3DShadowData.buf = HuMemDirectMalloc(HEAP_MODEL, SHADOW_HEAP_SIZE);
    }
    Hu3DShadowData.fov = fov;
    Hu3DShadowData.nnear = near;
    Hu3DShadowData.ffar = far;
    Hu3DShadowData.camPos.x = 300.0f;
    Hu3DShadowData.camPos.y = 300.0f;
    Hu3DShadowData.camPos.z = 0.0f;
    Hu3DShadowData.camTarget.x = Hu3DShadowData.camTarget.y = Hu3DShadowData.camTarget.z = 0.0f;
    Hu3DShadowData.camUp.x = -1.0f;
    Hu3DShadowData.camUp.y = 1.0f;
    Hu3DShadowData.camUp.z = 0.0f;
    C_MTXLightPerspective(Hu3DShadowData.projMtx, fov, HU_DISP_ASPECT, 0.5f, -0.5f, 0.5f, 0.5f);
    VECNormalize(&Hu3DShadowData.camUp, &Hu3DShadowData.camUp);
    Hu3DShadowData.alpha = 0x80;
    Hu3DShadowF = 1;
}

/**
 * @brief Set the shadow camera's position, up vector, and target.
 * @param camera_position Shadow-camera position.
 * @param camera_up Shadow-camera up vector.
 * @param camera_target Shadow-camera target point.
 */
void Hu3DShadowPosSet(Vec* camera_position, Vec* camera_up, Vec* camera_target) {
    Hu3DShadowData.camera_position = *camera_position;
    Hu3DShadowData.camera_target = *camera_target;
    Hu3DShadowData.camera_up = *camera_up;
}

/**
 * @brief Set shadow opacity.
 * @param opacity Shadow opacity multiplier converted to an 8-bit alpha value.
 */
void Hu3DShadowTPLvlSet(f32 opacity) {
    Hu3DShadowData.alpha = 255.0f * opacity;
}

/**
 * @brief Resize and reallocate the square shadow-map buffer.
 * @param size Shadow-map width and height in pixels.
 */
void Hu3DShadowSizeSet(u16 size) {
    Hu3DShadowData.size = size;
    if (Hu3DShadowData.buf != 0) {
        HuMemDirectFree(Hu3DShadowData.buf);
    }
    Hu3DShadowData.buf = HuMemDirectMalloc(HEAP_MODEL, size * size);
}

/**
 * @brief Render all eligible shadow-casting models into the shadow-map texture.
 *
 * @details The routine installs a shadow camera, draws casters, copies the embedded framebuffer into the R8 texture, then restores the main viewport and basic GX state.
 */
void Hu3DShadowExec(void) {
    HU3DMODEL* model;
    s16 model_id;
    Mtx model_mtx;
    Mtx draw_mtx;
    Mtx rotation_mtx;
    Mtx44 projection_mtx;
    GXColor clear_color = {0, 0, 0, 0};
    s32 unused_buffer_area;
    s32 unused_vertices_changed;

    Hu3DDrawPreInit();
    GXSetCopyClear(clear_color, 0xFFFFFF);
    C_MTXPerspective(projection_mtx, Hu3DShadowData.fov, HU_DISP_ASPECT, Hu3DShadowData.nnear, Hu3DShadowData.ffar);
    GXSetProjection(projection_mtx, GX_PERSPECTIVE);
    if (Hu3DShadowData.size <= 0xF0) {
        GXSetScissor(2, 2, Hu3DShadowData.size * 2 - 4, Hu3DShadowData.size * 2 - 4);
        GXSetViewport(0.0f, 0.0f, Hu3DShadowData.size * 2, Hu3DShadowData.size * 2, 0.0f, 1.0f);
        unused_buffer_area = (Hu3DShadowData.size / 2) * (Hu3DShadowData.size / 2);
    } else {
        GXSetScissor(1, 1, Hu3DShadowData.size - 2, Hu3DShadowData.size - 2);
        GXSetViewport(0.0f, 0.0f, Hu3DShadowData.size, Hu3DShadowData.size, 0.0f, 1.0f);
        unused_buffer_area = Hu3DShadowData.size * Hu3DShadowData.size;
    }
    C_MTXLookAt(Hu3DCameraMtx, &Hu3DShadowData.camPos, &Hu3DShadowData.camUp, &Hu3DShadowData.camTarget);
    PSMTXCopy(Hu3DCameraMtx, Hu3DShadowData.lookAtMtx);
    model = Hu3DData;
    shadowModelDrawF = 1;
    GXInvalidateTexAll();
    GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, BGColor);

    for (model_id = 0; model_id < HU3D_MODEL_MAX; model_id++, model++) {
        if (model->hsf != 0 && (model->attr & HU3D_ATTR_SHADOW) != 0 && (model->attr & HU3D_ATTR_DISPOFF) == 0 && (model->attr & HU3D_ATTR_HOOK) == 0) {
            if ((model->attr & HU3D_ATTR_MOTION_OFF) != 0) {
                unused_vertices_changed = 0;
                if (model->motId != -1) {
                    Hu3DMotionExec(model_id, model->motId, model->motWork.time, 0);
                }
                if (model->motIdShift != -1) {
                    Hu3DSubMotionExec(model_id);
                }
                if (model->motIdOvl != -1) {
                    Hu3DMotionExec(model_id, model->motIdOvl, model->motOvlWork.time, 1);
                }
                if ((model->attr & HU3D_ATTR_CLUSTER_ON) != 0) {
                    ClusterMotionExec(model);
                    unused_vertices_changed = 1;
                }
                if (model->motIdShape != -1) {
                    if (model->motId == -1) {
                        Hu3DMotionExec(model_id, model->motIdShape, model->motShapeWork.time, 0);
                    } else {
                        Hu3DMotionExec(model_id, model->motIdShape, model->motShapeWork.time, 1);
                    }
                }
                if ((model->attr & (HU3D_ATTR_HOOKFUNC|HU3D_ATTR_ENVELOPE_OFF)) == 0 || (model->motAttr & HU3D_MOTATTR_PAUSE) == 0) {
                    unused_vertices_changed = 1;
                    InitVtxParm(model->hsf);
                    if (model->motIdShape != -1) {
                        ShapeProc(model->hsf);
                    }
                    if ((model->attr & HU3D_ATTR_CLUSTER_ON) != 0) {
                        ClusterProc(model);
                    }
                    if (model->hsf->cenvNum != 0) {
                        EnvelopeProc(model->hsf);
                    }
                    PPCSync();
                }
                model->attr |= HU3D_ATTR_MOT_EXEC;
            }
            mtxRot(rotation_mtx, model->rot.x, model->rot.y, model->rot.z);
            PSMTXScale(model_mtx, model->scale.x, model->scale.y, model->scale.z);
            PSMTXConcat(rotation_mtx, model_mtx, model_mtx);
            mtxTransCat(model_mtx, model->pos.x, model->pos.y, model->pos.z);
            PSMTXConcat(Hu3DCameraMtx, model_mtx, draw_mtx);
            PSMTXConcat(draw_mtx, model->mtx, draw_mtx);
            Hu3DDraw(model, draw_mtx, &model->scale);
        }
    }
    Hu3DDrawPost();
    shadowModelDrawF = 0;
    if (Hu3DShadowData.size <= 0xF0) {
        GXSetTexCopySrc(0, 0, Hu3DShadowData.size * 2, Hu3DShadowData.size * 2);
        GXSetTexCopyDst(Hu3DShadowData.size, Hu3DShadowData.size, GX_CTF_R8, 1);
    } else {
        GXSetTexCopySrc(0, 0, Hu3DShadowData.size, Hu3DShadowData.size);
        GXSetTexCopyDst(Hu3DShadowData.size, Hu3DShadowData.size, GX_CTF_R8, 0);
    }
    GXCopyTex(Hu3DShadowData.buf, 1);
    GXSetViewport(0.0f, 0.0f, RenderMode->fbWidth, RenderMode->xfbHeight, 0.0f, 1.0f);
    GXSetScissor(0, 0, RenderMode->fbWidth, RenderMode->efbHeight);
    C_MTXOrtho(projection_mtx, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    GXSetProjection(projection_mtx, GX_ORTHOGRAPHIC);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_U8, 0);
    GXSetTevColor(GX_TEVREG0, BGColor);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1U, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1U, GX_TEVPREV);
    GXSetNumChans(0);
    PSMTXIdentity(draw_mtx);
    GXLoadPosMtxImm(draw_mtx, 0);
    GXSetZMode(0, GX_ALWAYS, 1);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    GXColor3u8(0, 0, 0);
    GXColor3u8(1, 0, 0);
    GXColor3u8(1, 1, 0);
    GXColor3u8(0, 1, 0);
}

/**
 * @brief Create a projected-texture slot.
 * @param anim Animation data used as the projected texture; retained by the slot.
 * @param fov Projection field of view in degrees.
 * @param near Near clipping-plane distance.
 * @param far Far clipping-plane distance.
 * @return The projection slot from 0 through 3, or -1 when all slots are occupied.
 */
s16 Hu3DProjectionCreate(void *anim, f32 fov, f32 near, f32 far) {
    s16 projection_id;
    HU3DPROJECTION* projection;

    projection = Hu3DProjection;

    for (projection_id = 0; projection_id < 4; projection_id++, projection++) {
        if (projection->anim == 0U) {
            break;
        }
    }
    if ((s16) projection_id == 4) {
        return -1;
    }
    projection->anim = anim;
    projection->fov = fov;
    projection->nnear = near;
    projection->ffar = far;
    projection->camPos.x = 1000.0f;
    projection->camPos.y = 1000.0f;
    projection->camPos.z = 0.0f;
    projection->camTarget.x = projection->camTarget.y = projection->camTarget.z = 0.0f;
    projection->camUp.x = -1.0f;
    projection->camUp.y = 1.0f;
    projection->camUp.z = 0.0f;
    C_MTXLightPerspective(projection->projMtx, fov, HU_DISP_ASPECT, 0.5f, -0.5f, 0.5f, 0.5f);
    VECNormalize(&projection->camUp, &projection->camUp);
    projection->alpha = 0x80;
    Hu3DProjectionNum++;
    return projection_id;
}

/**
 * @brief Release a projection slot's animation and mark the slot unused.
 * @param projection_slot Projection slot from 0 through 3.
 */
void Hu3DProjectionKill(s16 projection_slot) {
    HuSprAnimKill(Hu3DProjection[projection_slot].anim);
    Hu3DProjection[projection_slot].anim = NULL;
}

/**
 * @brief Set a projection camera's position, up vector, and target.
 * @param projection_slot Projection slot from 0 through 3.
 * @param position Projection-camera position.
 * @param up Projection-camera up vector.
 * @param target Projection-camera target point.
 */
void Hu3DProjectionPosSet(s16 projection_slot, Vec* position, Vec* up, Vec* target) {
    Hu3DProjection[projection_slot].camPos = *position;
    Hu3DProjection[projection_slot].camTarget = *target;
    Hu3DProjection[projection_slot].camUp = *up;
}

/**
 * @brief Set a projection texture's opacity.
 * @param projection_slot Projection slot from 0 through 3.
 * @param opacity Opacity multiplier converted to an 8-bit alpha value.
 */
void Hu3DProjectionTPLvlSet(s16 projection_slot, f32 opacity) {
    Hu3DProjection[projection_slot].alpha = 255.0f * opacity;
}

/**
 * @brief Replace an HSF bitmap with mip levels from sprite animation data.
 *
 * @details Mip image data is allocated from the model heap and copied from successive animation bitmaps.
 * @param anim_data Serialized sprite animation containing the replacement mip images.
 * @param model_id Model ID containing the target bitmap.
 * @param bitmap_name Target bitmap name.
 * @param lod_bias LOD bias stored on the bitmap.
 */
void Hu3DMipMapSet(char* anim_data, s16 model_id, char *bitmap_name, f32 lod_bias) {
    HSFBITMAP* bitmap;
    ANIMBMP* anim_bitmap;
    s16 i;
    HSFDATA* hsf;
    HSFATTRIBUTE* attribute;
    HU3DMODEL* model;
    s32 level_size;
    void *level_data;
    void* mip_data;
    ANIMDATA* anim;

    model = &Hu3DData[model_id];
    hsf = model->hsf;
    attribute = hsf->attribute;
    for (i = 0; i < hsf->attributeNum; i++, attribute++) {
        if (strcmp(bitmap_name, attribute->bitmap->name) == 0) {
            break;
        }
    }
    if (i == hsf->attributeNum) {
        OSReport("Error: Not Found %s for MipMapSet\n", bitmap_name);
        return;
    }
    bitmap = attribute->bitmap;
    anim = HuSprAnimRead(anim_data);
    anim_bitmap = anim->bmp;
    for ( i = 0, level_size = i; i < anim->bmpNum; i++, anim_bitmap++) {
        level_size += anim_bitmap->dataSize;
    }
    level_data = HuMemDirectMallocNum(HEAP_MODEL, level_size, model->mallocNo);
    mip_data = level_data;
    anim_bitmap = anim->bmp;
    bitmap->data = mip_data;
    bitmap->sizeX = anim_bitmap->sizeX;
    bitmap->sizeY = anim_bitmap->sizeY;
    bitmap->palSize = anim_bitmap->palNum;
    bitmap->palData = anim_bitmap->palData;
    bitmap->maxLod = anim->bmpNum;
    switch (anim_bitmap->dataFmt) {
        case 0:
            bitmap->dataFmt = 6;
            break;
        case 2:
            bitmap->dataFmt = 5;
            break;
        case 3:
            bitmap->dataFmt = 0xA;
            bitmap->pixSize = 8;
            break;
        case 4:
            bitmap->dataFmt = 0xA;
            bitmap->pixSize = 4;
            break;
    }

    for (i = 0; i < anim->bmpNum; i++, anim_bitmap++) {
        memcpy(level_data, anim_bitmap->data, anim_bitmap->dataSize);
        level_data = (void*) ((int) level_data + anim_bitmap->dataSize);
    }
    DCFlushRange(mip_data, level_size);
}
