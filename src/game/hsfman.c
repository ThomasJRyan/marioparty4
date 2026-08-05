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
    void (* temp)(s16);
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
                    temp = layerHook[j];
                    temp(j);
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
 * @param arg0 Red component from 0 through 255.
 * @param arg1 Green component from 0 through 255.
 * @param arg2 Blue component from 0 through 255.
 */
void Hu3DBGColorSet(u8 arg0, u8 arg1, u8 arg2) {
    BGColor.r = arg0;
    BGColor.g = arg1;
    BGColor.b = arg2;
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
 * @param arg0 Nonzero to mark Hu3D paused; zero to clear the flag.
 */
void Hu3DPauseSet(BOOL arg0) {
    Hu3DPauseF = arg0;
}

/**
 * @brief Control whether Hu3D issues inter-camera draw synchronization.
 * @param noSync Nonzero to suppress normal draw-done synchronization; zero to enable it.
 */
void Hu3DNoSyncSet(BOOL noSync) {
    NoSyncF = noSync;
}

/**
 * @brief Load HSF data into a newly initialized model slot.
 *
 * @details Embedded motion, cluster, shape, fog, transform, and render state are initialized from the loaded HSF.
 * @param arg0 Pointer to serialized HSF model data.
 * @return The allocated model ID, or -1 when all model slots are occupied.
 */
s16 Hu3DModelCreate(void *arg0) {
    HSFDATA* temp_r0;
    HU3DMODEL* var_r31;
    s16 i;
    s16 model_id;

    var_r31 = Hu3DData;

    for (model_id = 0; model_id < HU3D_MODEL_MAX; model_id++, var_r31++) {
        if (var_r31->hsf == 0x0) {
            break;
        }
    }
    if (model_id == HU3D_MODEL_MAX) {
        OSReport("Error: Create Model Over!\n");
        return -1;
    }
    var_r31->hsf = LoadHSF(arg0);
    var_r31->mallocNo = Hu3DMallocNo = (u32)var_r31->hsf;
    var_r31->attr = HU3D_ATTR_NONE;
    var_r31->motAttr = HU3D_ATTR_NONE;
    var_r31->projBit = 0;
    MakeDisplayList(model_id, var_r31->mallocNo);
    var_r31->motWork.speed = 1.0f;
    for (i = 0; i < 4; i++) {
        var_r31->motIdCluster[i] = -1;
    }
    var_r31->motIdOvl = -1;
    var_r31->motIdShift = -1;
    var_r31->motIdShape = -1;
    var_r31->motWork.time = 0.0f;
    if (var_r31->hsf->motionNum != 0) {
        var_r31->motId = var_r31->motIdSrc = Hu3DMotionModelCreate(model_id);
        if (var_r31->hsf->cenvNum != 0) {
            Hu3DMotionExec(model_id, var_r31->motId, 0.0f, 0);
            EnvelopeProc(var_r31->hsf);
            PPCSync();
        }
        if (var_r31->hsf->clusterNum != 0) {
            Hu3DMotionClusterSet(model_id, var_r31->motId);
        }
        if (var_r31->hsf->shapeNum != 0) {
            Hu3DMotionShapeSet(model_id, var_r31->motId);
        }
        var_r31->motWork.start = 0.0f;
        var_r31->motWork.end = Hu3DMotionMaxTimeGet(model_id);
    } else {
        var_r31->motIdSrc = var_r31->motId = -1;
    }
    var_r31->pos.x = var_r31->pos.y = var_r31->pos.z = 0.0f;
    var_r31->rot.x = var_r31->rot.y = var_r31->rot.z = 0.0f;
    var_r31->scale.x = var_r31->scale.y = var_r31->scale.z = 1.0f;
    var_r31->cameraBit = -1;
    var_r31->layerNo = 0;
    var_r31->hookData = 0;
    var_r31->lightNum = 0;
    var_r31->hiliteIdx = 0;
    var_r31->ambR = var_r31->ambG = var_r31->ambB = 1.0f;
    var_r31->reflectType = -1;
    var_r31->linkMdlId = -1;

    for (i = 0; i < 8; i++) {
        var_r31->lLightId[i] = -1;
    }
    var_r31->camInfoBit = 0;
    var_r31->tick = (u8) model_id;
    PSMTXIdentity(var_r31->mtx);
    layerNum[0] += 1;
    HuMemDCFlush(HEAP_MODEL);
    if ((var_r31->hsf->sceneNum != 0) && ((var_r31->hsf->scene->fogStart) || (var_r31->hsf->scene->fogEnd))) {
        Hu3DFogSet(var_r31->hsf->scene->fogStart, var_r31->hsf->scene->fogEnd, var_r31->hsf->scene->color.r, var_r31->hsf->scene->color.g, var_r31->hsf->scene->color.b);
    }
    return model_id;
}

/**
 * @brief Create a model instance that shares resources with an existing model.
 * @param arg0 Source model ID whose HSF data and motion state are linked.
 * @return The linked model ID, or -1 when all model slots are occupied.
 */
s16 Hu3DModelLink(s16 arg0) {
    HSFOBJECT* temp_r3_2;
    HU3DMODEL* temp_r30;
    HU3DMODEL* var_r31;
    s16 linked_model_id;
    s16 i;
    s32 object_offset;
    s32 root_offset;
    s32 data_offset;
    s32 copy_index;
    HSFDATA* var_r4;
    HSFDATA* var_r5;

    temp_r30 = &Hu3DData[arg0];
    var_r31 = Hu3DData;
    for (linked_model_id = 0; linked_model_id < HU3D_MODEL_MAX; linked_model_id++, var_r31++) {
        if (var_r31->hsf == 0x0) {
            break;
        }
    }
    if (linked_model_id == HU3D_MODEL_MAX) {
        return -1;
    }
    var_r31->hsfLink = temp_r30->hsf;
    var_r31->hsf = HuMemDirectMallocNum(HEAP_MODEL, 0x80, var_r31->mallocNoLink);
    var_r31->mallocNoLink = (u32)var_r31->hsf;
    *var_r31->hsf = *temp_r30->hsf;
    temp_r3_2 = Hu3DObjDuplicate(var_r31->hsf, var_r31->mallocNoLink);
    var_r31->hsf->root = (HSFOBJECT*)((u32)temp_r3_2 + ((u32)var_r31->hsf->root - (u32)var_r31->hsf->object));
    var_r31->hsf->object = temp_r3_2;
    var_r31->mallocNo = temp_r30->mallocNo;
    var_r31->attr = temp_r30->attr;
    temp_r30->attr |= HU3D_ATTR_LINK;
    var_r31->motAttr = temp_r30->motAttr;
    var_r31->pos.x = var_r31->pos.y = var_r31->pos.z = 0.0f;
    var_r31->rot.x = var_r31->rot.y = var_r31->rot.z = 0.0f;
    var_r31->scale.x = var_r31->scale.y = var_r31->scale.z = 1.0f;
    var_r31->motId = temp_r30->motId;
    if (var_r31->motId != -1) {
        var_r31->motWork.start = 0.0f;
        var_r31->motWork.end = Hu3DMotionMaxTimeGet(linked_model_id);
    }
    var_r31->motIdShift = var_r31->motIdOvl = var_r31->motIdShape = -1;
    for (i = 0; i < 4; i++) {
        var_r31->motIdCluster[i] = temp_r30->motIdCluster[i];
        if (var_r31->motIdCluster[i] != -1) {
            ClusterAdjustObject(var_r31->hsf, Hu3DMotion[var_r31->motIdCluster[i]].hsf);
            var_r31->attr |= HU3D_ATTR_CLUSTER_ON;
        }
    }
    var_r31->motWork.time = temp_r30->motWork.time;
    var_r31->motWork.speed = temp_r30->motWork.speed;
    var_r31->motIdSrc = temp_r30->motIdSrc;
    var_r31->cameraBit = -1;
    var_r31->layerNo = 0;
    var_r31->hookData = 0;
    var_r31->lightNum = 0;
    var_r31->hiliteIdx = 0;
    var_r31->projBit = 0;
    var_r31->ambR = var_r31->ambG = var_r31->ambB = 1.0f;
    var_r31->reflectType = -1;
    var_r31->linkMdlId = arg0;
    for (i = 0; i < 8; i++) {
        var_r31->lLightId[i] = -1;
    }
    var_r31->camInfoBit = 0;
    PSMTXIdentity(var_r31->mtx);
    layerNum[0] += 1;
    return linked_model_id;
}

/**
 * @brief Create a model slot rendered by a callback instead of HSF geometry.
 * @param hook Render callback stored in the new model entry.
 * @return The allocated model ID, or -1 when all model slots are occupied.
 */
s16 Hu3DHookFuncCreate(HU3DMODELHOOK hook) {
    HSFDATA* sp8;
    HU3DMODEL* var_r31;
    s16 model_id;
    s16 i;

    var_r31 = Hu3DData;
    for (model_id = 0; model_id < HU3D_MODEL_MAX; model_id++, var_r31++) {
        if (var_r31->hsf == 0) {
            break;
        }
    }
    if (model_id == HU3D_MODEL_MAX) {
        return -1;
    }
    var_r31->hookFunc = hook;
    var_r31->mallocNo = model_id+10000;
    var_r31->attr = HU3D_ATTR_HOOKFUNC;
    var_r31->motAttr = HU3D_ATTR_NONE;
    var_r31->pos.x = var_r31->pos.y = var_r31->pos.z = 0.0f;
    var_r31->rot.x = var_r31->rot.y = var_r31->rot.z = 0.0f;
    var_r31->scale.x = var_r31->scale.y = var_r31->scale.z = 1.0f;
    var_r31->motId = var_r31->motIdShift = var_r31->motIdOvl = var_r31->motIdShape = -1;

    for (i = 0; i < 4; i++) {
        var_r31->motIdCluster[i] = -1;
    }
    var_r31->motWork.time = 0.0f;
    var_r31->motWork.speed = 1.0f;
    var_r31->motIdSrc = -1;
    var_r31->cameraBit = -1;
    var_r31->layerNo = 0;
    var_r31->hookData = 0;
    var_r31->lightNum = 0;
    var_r31->hiliteIdx = 0;
    var_r31->linkMdlId = -1;
    var_r31->projBit = 0;
    var_r31->reflectType = -1;

    for (i = 0; i < 8; i++) {
        var_r31->lLightId[i] = -1;
    }
    var_r31->camInfoBit = 0;
    PSMTXIdentity(var_r31->mtx);
    layerNum[0] += 1;
    return model_id;
}

/**
 * @brief Release a model and any motion, light, animation, and HSF storage no longer shared.
 * @param arg0 Model ID to release.
 */
void Hu3DModelKill(s16 arg0) {
    HSFDATA* var_r28;
    HU3DMODEL* temp_r31;
    HU3DMODEL* var_r30;
    HU3DPARTICLE *copy;
    s16 reference_count;
    s16 i;

    temp_r31 = &Hu3DData[arg0];
    var_r28 = temp_r31->hsf;
    if (var_r28 != 0) {
        if ((temp_r31->attr & HU3D_ATTR_SHADOW) != 0) {
            Hu3DShadowCamBit -= 1;
        }
        layerNum[temp_r31->layerNo] -= 1;

        if ((temp_r31->attr & HU3D_ATTR_HOOKFUNC) != 0) {
            HuMemDirectFreeNum(HEAP_MODEL, temp_r31->mallocNo);
            if ((temp_r31->attr & HU3D_ATTR_PARTICLE_KILL) != 0) {
                copy = temp_r31->hookData;
                HuSprAnimKill(copy->anim);
            }
            temp_r31->hsf = NULL;
            if (modelKillAllF == 0) {
                HuMemDCFlush(HEAP_MODEL);
            }
            return;
        }
        if ((temp_r31->attr & HU3D_ATTR_CAMERA) != 0) {
            if (temp_r31->motId != -1) {
                Hu3DMotionKill(temp_r31->motId);
            }
            HuMemDirectFreeNum(HEAP_MODEL, temp_r31->mallocNo);
            temp_r31->hsf = NULL;
            return;
        }
        Hu3DAnimModelKill(arg0);
        if (temp_r31->linkMdlId != -1) {
            HuMemDirectFree(temp_r31->hsf);
            HuMemDirectFreeNum(HEAP_MODEL, temp_r31->mallocNoLink);
            var_r28 = temp_r31->hsfLink;
            temp_r31->hsf = var_r28;
        }
        var_r30 = Hu3DData;
        for (reference_count = i = 0; i < HU3D_MODEL_MAX; i++, var_r30++) {
            if ((var_r30->hsf != 0) && (var_r30->hsf == var_r28 || (var_r30->linkMdlId != -1 && var_r30->hsfLink == var_r28))) {
                reference_count++;
            }
        }
        if (reference_count > 1) {
            temp_r31->hsf = NULL;
            var_r30 = Hu3DData;
            if (temp_r31->motIdSrc != -1) {
                for (i = 0; i < HU3D_MODEL_MAX; i++, var_r30++) {
                    if (var_r30->hsf != 0 && var_r30->linkMdlId != -1 && var_r30->hsfLink == var_r28) {
                        Hu3DMotion[temp_r31->motIdSrc].modelId = i;
                        break;
                    }
                }
            }
            if (modelKillAllF == 0) {
                HuMemDCFlush(HEAP_MODEL);
            }
            return;
        }
        if (temp_r31->motIdSrc != -1 && Hu3DMotionKill(temp_r31->motIdSrc) == 0) {
            Hu3DMotion[temp_r31->motIdSrc].modelId = -1;
            HuMemDirectFreeNum(HEAP_MODEL, temp_r31->mallocNo);
            temp_r31->hsf = NULL;
            if (modelKillAllF == 0) {
                HuMemDCFlush(HEAP_MODEL);
            }
            return;
        }
        HuMemDirectFree(temp_r31->hsf);
        HuMemDirectFreeNum(HEAP_MODEL, temp_r31->mallocNo);
        for (i = 0; i < temp_r31->lightNum; i++) {
            Hu3DGLightKill(temp_r31->lightId[i]);
        }
        for (i = 0; i < 8; i++) {
            if (temp_r31->lLightId[i] != -1) {
                Hu3DLLightKill(arg0, i);
            }
        }
        temp_r31->hsf = NULL;
        if (modelKillAllF == 0) {
            HuMemDCFlush(HEAP_MODEL);
        }
    }
}

/**
 * @brief Release every populated Hu3D model slot.
 */
void Hu3DModelAllKill(void) {
    HU3DMODEL* var_r30;
    s16 i;

    modelKillAllF = 1;
    var_r30 = Hu3DData;

    for (i = 0; i < HU3D_MODEL_MAX; i++, var_r30++) {
        if (var_r30->hsf != 0) {
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
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[index];
    temp_r31->pos.x = x;
    temp_r31->pos.y = y;
    temp_r31->pos.z = z;
}

/**
 * @brief Set a model's position from a vector.
 * @param arg0 Model ID to update.
 * @param arg1 Vector containing the new position components.
 */
void Hu3DModelPosSetV(s16 arg0, Vec *arg1) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    temp_r31->pos = *arg1;
}

/**
 * @brief Set a model's Euler rotation in degrees components.
 * @param index Model ID to update.
 * @param x X Euler rotation in degrees component.
 * @param y Y Euler rotation in degrees component.
 * @param z Z Euler rotation in degrees component.
 */
void Hu3DModelRotSet(s16 index, f32 x, f32 y, f32 z) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[index];
    temp_r31->rot.x = x;
    temp_r31->rot.y = y;
    temp_r31->rot.z = z;
}

/**
 * @brief Set a model's Euler rotation in degrees from a vector.
 * @param arg0 Model ID to update.
 * @param arg1 Vector containing the new Euler rotation in degrees components.
 */
void Hu3DModelRotSetV(s16 arg0, Vec *arg1) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    temp_r31->rot = *arg1;
}

/**
 * @brief Set a model's scale components.
 * @param index Model ID to update.
 * @param x X scale component.
 * @param y Y scale component.
 * @param z Z scale component.
 */
void Hu3DModelScaleSet(s16 index, f32 x, f32 y, f32 z) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[index];
    temp_r31->scale.x = x;
    temp_r31->scale.y = y;
    temp_r31->scale.z = z;
}

/**
 * @brief Set a model's scale from a vector.
 * @param arg0 Model ID to update.
 * @param arg1 Vector containing the new scale components.
 */
void Hu3DModelScaleSetV(s16 arg0, Vec *arg1) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    temp_r31->scale = *arg1;
}

/**
 * @brief Set model render-attribute bits and propagate supported bits to linked models.
 * @param arg0 Model ID to update.
 * @param arg1 Bit mask of HU3D model attributes to enable.
 */
void Hu3DModelAttrSet(s16 arg0, u32 arg1) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    if ((arg1 & HU3D_MOTATTR) != 0) {
        temp_r31->motAttr |= arg1 & ~HU3D_MOTATTR;
    } else {
        temp_r31->attr |= arg1;
    }
}

/**
 * @brief Clear model render-attribute bits and propagate supported bits to linked models.
 * @param arg0 Model ID to update.
 * @param arg1 Bit mask of HU3D model attributes to disable.
 */
void Hu3DModelAttrReset(s16 arg0, u32 arg1) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    if ((arg1 & HU3D_MOTATTR) != 0) {
        temp_r31->motAttr &= ~arg1;
    } else {
        temp_r31->attr &= ~arg1;
    }
}

/**
 * @brief Get a model's render-attribute mask.
 * @param arg0 Model ID to query.
 * @return The current HU3D attribute bit mask.
 */
u32 Hu3DModelAttrGet(s16 arg0) {
    HU3DMODEL *copy = &Hu3DData[arg0];
    return copy->attr;
}

/**
 * @brief Get a model's motion-playback attribute mask.
 * @param arg0 Model ID to query.
 * @return The current HU3D motion attribute bit mask.
 */
u32 Hu3DModelMotionAttrGet(s16 arg0) {
    HU3DMODEL *copy = &Hu3DData[arg0];
    return copy->motAttr;
}

/**
 * @brief Enable playback attributes on one model cluster-motion slot.
 * @param arg0 Model ID to update.
 * @param arg1 Cluster slot from 0 through 3.
 * @param arg2 Cluster attribute bits to enable.
 */
void Hu3DModelClusterAttrSet(s16 arg0, s16 arg1, s32 arg2) {
    HU3DMODEL* temp_r31;
    s32 unused_attr;

    temp_r31 = &Hu3DData[arg0];
    temp_r31->clusterAttr[arg1] |= arg2;
}

/**
 * @brief Disable playback attributes on one model cluster-motion slot.
 * @param arg0 Model ID to update.
 * @param arg1 Cluster slot from 0 through 3.
 * @param arg2 Cluster attribute bits to clear.
 */
void Hu3DModelClusterAttrReset(s16 arg0, s16 arg1, s32 arg2) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    temp_r31->clusterAttr[arg1] &= ~arg2;
}

/**
 * @brief Set the camera visibility mask for a model.
 * @param arg0 Model ID to update.
 * @param arg1 Bit mask of cameras allowed to draw the model.
 */
void Hu3DModelCameraSet(s16 arg0, u16 arg1) {
    HU3DMODEL* copy = &Hu3DData[arg0];
    copy->cameraBit = arg1;
}

/**
 * @brief Move a model to a render layer.
 * @param arg0 Model ID to update.
 * @param arg1 Destination layer index from 0 through 7.
 */
void Hu3DModelLayerSet(s16 arg0, s16 arg1) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    layerNum[temp_r31->layerNo] -= 1;
    temp_r31->layerNo = arg1;
    layerNum[arg1] += 1;
}

/**
 * @brief Find a named HSF object within a model.
 * @param arg0 Model ID whose object table is searched.
 * @param arg1 Object name in the external naming format accepted by MakeObjectName.
 * @return The matching object pointer, or NULL after reporting an error if no object matches.
 */
HSFOBJECT* Hu3DModelObjPtrGet(s16 arg0, char *arg1) {
    char name[0x100];
    s32 name_length;
    s16 object_index;
    HSFDATA* temp_r31;
    s16 bucket_index;
    HSFOBJECT* var_r29;
    HSFOBJECT* var_r28;
    u32 name_word;

    temp_r31 = Hu3DData[arg0].hsf;
    var_r29 = temp_r31->object;
    strcpy(&name, MakeObjectName(arg1));

    for (bucket_index = 0; bucket_index < temp_r31->objectNum; var_r29++, bucket_index++) {
        var_r28 = var_r29;
        if (strcmp(&name, var_r28->name) == 0) {
            return var_r29;
        }
    }
    if (bucket_index == temp_r31->objectNum) {
        OSReport("Error: OBJPtr Error!\n");
    }
    return NULL;
}

/**
 * @brief Set constant-data attribute bits on a mesh object.
 * @param var_r26 Object to update; non-mesh objects are ignored.
 * @param a Constant-data attribute bits to enable.
 */
static inline void inlineFunc(HSFOBJECT* var_r26, u32 a) {
    HSFCONSTDATA* temp_r25;
    HSFOBJECT* copy = var_r26;
    if (copy->type == HSF_OBJ_MESH) {
        temp_r25 = copy->constData;
        temp_r25->attr |= a;
    }
}

/**
 * @brief Apply a uniform transparency level to every material in a model.
 *
 * @details The inverse alpha, material pass, mesh constant flags, and model override flag are updated.
 * @param arg0 Model ID to update.
 * @param arg8 Opacity level, normally 0 for transparent through 1 for opaque.
 */
void Hu3DModelTPLvlSet(s16 arg0, f32 arg8) {
    HSFMATERIAL* var_r31;
    HSFDATA* temp_r30;
    s16 i;
    HU3DMODEL* temp_r28;
    HSFOBJECT* var_r27;
    HSFOBJECT* var_r26;
    HSFCONSTDATA* temp_r25;

    temp_r28 = &Hu3DData[arg0];
    temp_r30 = temp_r28->hsf;
    var_r31 = temp_r30->material;
    for (i = 0; i < temp_r30->materialNum; i++, var_r31++) {
        var_r31->invAlpha = 1.0f - arg8;
        if (1.0f != arg8) {
            var_r31->pass = (var_r31->pass & 0xF0) | 1;
        } else {
            var_r31->pass &= 0xF0;
        }
    }
    var_r26 = temp_r30->object;
    for (i = 0; i < temp_r30->objectNum; var_r26++, i++) {
        var_r27 = var_r26;
        if (var_r27->type == 2) {
            temp_r25 = var_r27->constData;
            temp_r25->attr |= 1;
        }
    }
    temp_r28->attr |= HU3D_ATTR_TPLVL_SET;
}

/**
 * @brief Assign a highlight texture to every mesh object in a model.
 * @param arg0 Model ID to update.
 * @param arg1 Highlight animation data to reference; ownership remains with the caller.
 */
void Hu3DModelHiliteMapSet(s16 arg0, ANIMDATA *arg1) {
    HSFOBJECT* copy;
    HU3DMODEL* temp_r30;
    HSFDATA* temp_r29;
    s16 i;
    HSFOBJECT* var_r27;
    HSFCONSTDATA* temp_r25;

    temp_r30 = &Hu3DData[arg0];
    temp_r29 = temp_r30->hsf;
    var_r27 = temp_r29->object;
    for (i = 0; i < temp_r29->objectNum; var_r27++, i++) {
        copy = var_r27;
        if (copy->type == HSF_OBJ_MESH) {
            copy->flags |= 0x100;
            temp_r25 = copy->constData;
            temp_r25->attr |= 0x8000;
            temp_r25->hiliteMap = arg1;
        }
    }
}

/**
 * @brief Set constant-data attribute bits on an object when constant data exists.
 * @param var_r26 HSF object to update.
 * @param a Constant-data attribute bits to enable.
 */
static inline void constDataFlagSet(HSFOBJECT* var_r26, u32 a) {
    HSFCONSTDATA* temp_r25;
    HSFOBJECT* copy = var_r26;
    if (copy->constData != 0) {
        temp_r25 = copy->constData;
        temp_r25->attr |= a;
    }
}

/**
 * @brief Clear constant-data attribute bits on an object when constant data exists.
 * @param var_r26 HSF object to update.
 * @param a Constant-data attribute bits to disable.
 */
static inline void constDataFlagReset(HSFOBJECT* var_r26, u32 a) {
    HSFCONSTDATA* temp_r25;
    HSFOBJECT* copy = var_r26;
    if (copy->constData != 0) {
        temp_r25 = copy->constData;
        temp_r25->attr &= ~a;
    }
}

/**
 * @brief Enable shadow casting for a model and its eligible mesh objects.
 * @param arg0 Model ID to mark as a shadow caster.
 */
void Hu3DModelShadowSet(s16 arg0) {
    HSFCONSTDATA* temp_r26;
    HSFDATA* temp_r30;
    HSFCONSTDATA* temp_r25;
    HSFOBJECT* copy;
    s16 object_index;
    HSFOBJECT* var_r27;
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    temp_r30 = temp_r31->hsf;
    if ((temp_r31->attr & HU3D_ATTR_SHADOW) == 0) {
        Hu3DShadowCamBit++;
    }
    temp_r31->attr |= HU3D_ATTR_SHADOW;
    var_r27 = temp_r30->object;

    for (object_index = 0; object_index < temp_r30->objectNum; object_index++, var_r27++) {
        constDataFlagSet(var_r27, 0x400);
    }
}

/**
 * @brief Disable shadow casting for a model and its mesh objects.
 * @param arg0 Model ID to remove from shadow rendering.
 */
void Hu3DModelShadowReset(s16 arg0) {
    s16 object_index;
    HSFOBJECT* var_r27;
    HSFCONSTDATA* temp_r26;
    HU3DMODEL* temp_r31;
    HSFDATA* temp_r30;
    HSFOBJECT* copy;

    temp_r31 = &Hu3DData[(s16) arg0];
    temp_r30 = temp_r31->hsf;
    temp_r31->attr &= ~HU3D_ATTR_SHADOW;
    Hu3DShadowCamBit -= 1;
    var_r27 = temp_r30->object;
    for (object_index = 0; object_index < temp_r30->objectNum; object_index++, var_r27++) {
        constDataFlagReset(var_r27, 0x400);
    }
}

/**
 * @brief Make a model visible during the shadow-rendering pass.
 * @param arg0 Model ID to update.
 */
void Hu3DModelShadowDispOn(s16 arg0) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    temp_r31->attr |= HU3D_ATTR_SHADOW;
}

/**
 * @brief Hide a model during the shadow-rendering pass.
 * @param arg0 Model ID to update.
 */
void Hu3DModelShadowDispOff(s16 arg0) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    temp_r31->attr &= ~HU3D_ATTR_SHADOW;
}

/**
 * @brief Enable shadow-map sampling on every mesh object in a model.
 * @param arg0 Model ID to receive the shadow map.
 */
void Hu3DModelShadowMapSet(s16 arg0) {
    HSFCONSTDATA* temp_r27;
    HSFDATA* temp_r31;
    s16 i;
    HSFOBJECT* var_r28;
    HSFOBJECT* copy;

    temp_r31 = Hu3DData[arg0].hsf;
    var_r28 = temp_r31->object;
    for (i = 0; i < temp_r31->objectNum; i++, var_r28++) {
        constDataFlagSet(var_r28, 8);
    }
}

/**
 * @brief Enable shadow-map sampling on one named object and its descendants.
 * @param arg0 Model ID containing the object hierarchy.
 * @param arg1 Name of the root object to update.
 */
void Hu3DModelShadowMapObjSet(s16 arg0, char *arg1) {
    char name[0x100];
    HSFDATA* temp_r30;
    s16 i;
    HSFOBJECT* var_r28;
    HSFOBJECT* copy;
    HSFCONSTDATA* temp_r27;

    temp_r30 = Hu3DData[arg0].hsf;
    var_r28 = temp_r30->object;
    strcpy(&name, MakeObjectName(arg1));

    for (i = 0; i < temp_r30->objectNum; i++, var_r28++) {
        copy = var_r28;
        if (copy->constData != 0x0 && strcmp(&name, copy->name) == 0) {
            temp_r27 = copy->constData;
            temp_r27->attr |= 8;
            break;
        }
    }
}

/**
 * @brief Set a model's ambient RGB multipliers.
 * @param arg0 Model ID to update.
 * @param arg8 Red ambient multiplier.
 * @param arg9 Green ambient multiplier.
 * @param argA Blue ambient multiplier.
 */
void Hu3DModelAmbSet(s16 arg0, f32 arg8, f32 arg9, f32 argA) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    temp_r31->ambR = arg8;
    temp_r31->ambG = arg9;
    temp_r31->ambB = argA;
}

/**
 * @brief Attach another model to a named HSF object's hook slot.
 * @param arg0 Parent model ID containing the named object.
 * @param arg1 Name of the parent object.
 * @param arg2 Child model ID to mark as hooked.
 */
void Hu3DModelHookSet(s16 arg0, char *arg1, s16 arg2) {
    char name[0x100];
    HU3DMODEL* data;
    HSFDATA* temp_r30;
    s16 i;
    HSFCONSTDATA *constData;
    HSFOBJECT* copy;
    HSFOBJECT* var_r27;

    temp_r30 = Hu3DData[arg0].hsf;
    var_r27 = temp_r30->object;
    strcpy(&name, MakeObjectName(arg1));

    for (i = 0; i < temp_r30->objectNum; i++, var_r27++) {
        copy = var_r27;
        if (copy->constData != 0) {
            if (strcmp(&name, copy->name) == 0) {
                constData = copy->constData;
                constData->hookMdlId = arg2;
                data = &Hu3DData[arg2];
                data->attr |= HU3D_ATTR_HOOK;
                (void)data;
                return;
            }
        }
    }
    OSReport("Error: Not Found %s for HookSet\n", arg1);
}

/**
 * @brief Detach every child model hooked beneath a parent model.
 * @param arg0 Parent model ID whose object hooks are cleared.
 */
void Hu3DModelHookReset(s16 arg0) {
    HSFCONSTDATA* temp_r31;
    HSFDATA* temp_r30;
    HSFOBJECT* copy;
    HU3DMODEL* temp_r28;
    s16 object_index;
    HSFOBJECT* var_r26;
    s16 hook_model_id;

    temp_r30 = Hu3DData[arg0].hsf;
    var_r26 = temp_r30->object;
    for (object_index = 0; object_index < temp_r30->objectNum; object_index++, var_r26++) {
        copy = var_r26;
        if (copy->constData != 0) {
            temp_r31 = copy->constData;
            if (temp_r31->hookMdlId != -1) {
                hook_model_id = temp_r31->hookMdlId;
                temp_r28 = &Hu3DData[hook_model_id];
                temp_r28->attr &= ~HU3D_ATTR_HOOK;
                temp_r31->hookMdlId = -1;
                (void)temp_r28;
            }
        }
    }
}

/**
 * @brief Detach the child model hooked to one named parent object.
 * @param arg0 Parent model ID containing the object.
 * @param arg1 Name of the object whose hook is cleared.
 */
void Hu3DModelHookObjReset(s16 arg0, char *arg1) {
    char name[0x100];
    HU3DMODEL* temp_r28;
    HSFDATA* temp_r30;
    HSFOBJECT* copy;
    HSFCONSTDATA* temp_r29;
    s16 i;
    HSFOBJECT* var_r26;
    s16 hook_model_id;

    temp_r30 = Hu3DData[arg0].hsf;
    var_r26 = temp_r30->object;
    strcpy(&name, MakeObjectName(arg1));

    for (i = 0; i < temp_r30->objectNum; i++, var_r26++) {
        copy = var_r26;
        if (copy->constData != 0) {
            if (strcmp(&name, copy->name) == 0) {
                temp_r29 = copy->constData;
                hook_model_id = temp_r29->hookMdlId;
                temp_r28 = &Hu3DData[hook_model_id];
                temp_r28->attr &= ~HU3D_ATTR_HOOK;
                temp_r29->hookMdlId = -1;
                (void)temp_r28;
                return;
            }
        }
    }

    OSReport("Error: Not Found %s for HookReset\n", arg1);
}

/**
 * @brief Enable a projection texture on a model.
 * @param arg0 Model ID to update.
 * @param arg1 Projection slot from 0 through 3.
 */
void Hu3DModelProjectionSet(s16 arg0, s16 arg1) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    temp_r31->projBit = temp_r31->projBit | (1 << arg1);
}

/**
 * @brief Disable a projection texture on a model.
 * @param arg0 Model ID to update.
 * @param arg1 Projection slot from 0 through 3.
 */
void Hu3DModelProjectionReset(s16 arg0, s16 arg1) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    temp_r31->projBit &= ~(1 << arg1);
}

/**
 * @brief Set the highlight mode encoded in every material pass.
 * @param arg0 Model ID to update.
 * @param arg1 Highlight type encoded into the upper material-pass nibble.
 */
void Hu3DModelHiliteTypeSet(s16 arg0, s16 arg1) {
    HSFDATA* temp_r30;
    HSFMATERIAL* var_r31;
    HU3DMODEL* temp_r29;
    HU3DMODEL* temp_r5;
    s16 i;
    s32 temp;

    temp_r5 = &Hu3DData[arg0];
    temp_r30 = temp_r5->hsf;
    var_r31 = temp_r30->material;

    arg1 = ((s16) arg1) * 0x10;
    arg1 &= 0xF0;
    for (i = 0; i < temp_r30->materialNum; i++, var_r31++) {
        var_r31->pass = var_r31->pass & 0xF | arg1;
        var_r31->flags |= 0x100;
    }
    temp_r29 = &Hu3DData[arg0];
    temp_r29->attr |= HU3D_ATTR_HILITE;
    (void)temp_r29;
}

/**
 * @brief Select the reflection-map mode used by a model.
 * @param arg0 Model ID to update.
 * @param arg1 Reflection type value, or -1 for no model-specific type.
 */
void Hu3DModelReflectTypeSet(s16 arg0, s16 arg1) {
    HU3DMODEL *copy = &Hu3DData[arg0];
    copy->reflectType = arg1;
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
 * @param arg0 Camera index to activate for drawing.
 * @param arg1 Output view matrix populated from camera position, up vector, and target.
 */
void Hu3DCameraSet(s32 arg0, Mtx arg1) {
    Mtx44 projection_mtx;
    Mtx44 unused_mtx;
    HU3DCAMERA* temp_r31;

    temp_r31 = &Hu3DCamera[arg0];
    C_MTXPerspective(projection_mtx, temp_r31->fov, temp_r31->aspect, temp_r31->near, temp_r31->far);
    GXSetProjection(projection_mtx, GX_PERSPECTIVE);
    if (RenderMode->field_rendering != 0) {
        GXSetViewportJitter(temp_r31->viewportX, temp_r31->viewportY, temp_r31->viewportW, temp_r31->viewportH, temp_r31->viewportNear, temp_r31->viewportFar, VIGetNextField());
    } else {
        GXSetViewport(temp_r31->viewportX, temp_r31->viewportY, temp_r31->viewportW, temp_r31->viewportH, temp_r31->viewportNear, temp_r31->viewportFar);
    }
    GXSetScissor(temp_r31->scissorX, temp_r31->scissorY, temp_r31->scissorW, temp_r31->scissorH);
    C_MTXLookAt(arg1, &temp_r31->pos, &temp_r31->up, &temp_r31->target);
}

/**
 * @brief Initialize a camera from the first HSF camera object in a model.
 * @param arg0 Model ID whose HSF camera object is read.
 * @param arg1 Camera index to initialize and store on the model.
 * @return TRUE if a camera object was found and applied; FALSE otherwise.
 */
BOOL Hu3DModelCameraInfoSet(s16 arg0, u16 arg1) {
    HU3DCAMERA* cam;
    HU3DCAMERA* temp_r30;
    HU3DCAMERA* temp_r29;
    HSFOBJECT* obj_copy;
    HU3DMODEL* temp_r24;
    HU3DMODEL* temp_r28;
    HSFOBJECT* var_r23;
    HSFDATA* temp_r27;
    Point3d rotated_up;
    Point3d view_direction;
    f32 up_rotation;
    s16 i;
    s16 unused_index_a;
    s16 unused_index_b;
    s16 unused_mask_a;
    s16 unused_mask_b;

    temp_r28 = &Hu3DData[arg0];
    temp_r27 = temp_r28->hsf;
    cam = &Hu3DCamera[arg1];
    var_r23 = temp_r27->object;

    for (i = 0; i < temp_r27->objectNum; i++, var_r23++) {
        obj_copy = var_r23;
        if (obj_copy->type == 7) {
            up_rotation = obj_copy->mesh.base.rot.x;
            cam->upRot = up_rotation;

            VECSubtract((Point3d* ) &obj_copy->camera.pos, (Point3d* ) &obj_copy->camera.target, &view_direction);

            rotated_up.x = ((view_direction.x * view_direction.y * (1.0 - cosd(up_rotation))) - (view_direction.z * sind(up_rotation)));
            rotated_up.y = ((view_direction.y * view_direction.y) + (1.0f - (view_direction.y * view_direction.y)) * cosd(up_rotation));
            rotated_up.z = (((view_direction.y * view_direction.z) * (1.0 - cosd(up_rotation))) + (view_direction.x * sind(up_rotation)));
            VECNormalize(&rotated_up, &view_direction);

            Hu3DCameraPosSet(arg1, obj_copy->camera.pos.x, obj_copy->camera.pos.y, obj_copy->camera.pos.z,
                             view_direction.x, view_direction.y, view_direction.z,
                             obj_copy->camera.target.x, obj_copy->camera.target.y, obj_copy->camera.target.z);

            Hu3DCameraPerspectiveSet(arg1, obj_copy->camera.fov, obj_copy->camera.near, obj_copy->camera.far, HU_DISP_ASPECT);

            temp_r28->camInfoBit = arg1;
            temp_r24 = &Hu3DData[arg0];
            temp_r24->attr |= HU3D_ATTR_CAMERA_MOTON;
            (void)temp_r24;
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Create a camera-motion model backed by an existing motion.
 * @param arg0 Motion ID containing camera tracks.
 * @param arg1 Camera-selection bit mask driven by the new model.
 * @return The allocated model ID, or -1 when no model slot is available.
 */
s16 Hu3DModelCameraCreate(s16 arg0, u16 arg1) {
    HU3DMODEL* temp_r31;
    s16 model_id;

    model_id = Hu3DHookFuncCreate((HU3DMODELHOOK)-1);
    temp_r31 = &Hu3DData[(s16) model_id];
    temp_r31->attr &= ~HU3D_ATTR_HOOKFUNC;
    temp_r31->attr |= HU3D_ATTR_CAMERA | HU3D_ATTR_CAMERA_MOTON;
    temp_r31->motId = arg0;
    temp_r31->camInfoBit = arg1;
    return model_id;
}

/**
 * @brief Enable camera-track evaluation for a model.
 * @param arg0 Model ID containing camera tracks.
 * @param arg1 Camera-selection bit mask driven by the model.
 */
void Hu3DCameraMotionOn(s16 arg0, s8 arg1) {
    HU3DMODEL* copy;
    HU3DMODEL* copy2;

    copy2 = &Hu3DData[arg0];
    copy2->camInfoBit = arg1;
    copy = &Hu3DData[arg0];
    copy->attr |= HU3D_ATTR_CAMERA_MOTON;
}

/**
 * @brief Enable and rewind camera motion on a model.
 * @param arg0 Model ID containing camera tracks.
 * @param arg1 Camera-selection bit mask driven by the model.
 */
void Hu3DCameraMotionStart(s16 arg0, u16 arg1) {
    HU3DMODEL* temp_r30;
    HU3DMODEL* temp_r29;
    HU3DMODEL* temp_r28;
    HU3DMODEL* temp_r27;

    temp_r28 = &Hu3DData[arg0];
    temp_r27 = &Hu3DData[arg0];
    temp_r27->camInfoBit = arg1;
    temp_r29 = &Hu3DData[arg0];
    temp_r29->attr |= HU3D_ATTR_CAMERA_MOTON;
    temp_r30 = &Hu3DData[arg0];
    temp_r30->motAttr &= ~HU3D_MOTATTR_PAUSE;
    Hu3DMotionStartEndSet(arg0, 0.0f, Hu3DMotionMotionMaxTimeGet(temp_r28->motId));
    Hu3DMotionTimeSet(arg0, 0.0f);
}

/**
 * @brief Disable camera-track evaluation for a model.
 * @param arg0 Model ID whose camera-motion attribute is cleared.
 */
void Hu3DCameraMotionOff(s16 arg0) {
    HU3DMODEL* temp_r31;

    temp_r31 = &Hu3DData[arg0];
    temp_r31->attr &= ~HU3D_ATTR_CAMERA_MOTON;
}

/**
 * @brief Mark every global and local Hu3D light slot as unused.
 */
void Hu3DLighInit(void) {
    HU3DLIGHT* var_r31;
    s16 light_index;

    var_r31 = Hu3DGlobalLight;
    for (light_index = 0; light_index < 8; light_index++, var_r31++) {
        var_r31->type = -1;
    }
    var_r31 = Hu3DLocalLight;
    for (light_index = 0; light_index < 0x20; light_index++, var_r31++) {
        var_r31->type = -1;
    }
}


/**
 * @brief Create a global light from scalar position, direction, and color values.
 * @param arg8 Light-position X coordinate.
 * @param arg9 Light-position Y coordinate.
 * @param argA Light-position Z coordinate.
 * @param argB Direction or aim X component.
 * @param argC Direction or aim Y component.
 * @param argD Direction or aim Z component.
 * @param arg0 Red component from 0 through 255.
 * @param arg1 Green component from 0 through 255.
 * @param arg2 Blue component from 0 through 255.
 * @return The global-light index from 0 through 7, or -1 if all slots are occupied.
 */
s16 Hu3DGLightCreate(f32 arg8, f32 arg9, f32 argA, f32 argB, f32 argC, f32 argD, u8 arg0, u8 arg1, u8 arg2) {
    Vec vec1;
    Vec vec2;
    GXColor color;

    vec1.x = arg8;
    vec1.y = arg9;
    vec1.z = argA;
    vec2.x = argB;
    vec2.y = argC;
    vec2.z = argD;
    color.r = arg0;
    color.g = arg1;
    color.b = arg2;
    color.a = 0xFF;
    return Hu3DGLightCreateV(&vec1, &vec2, &color);
}

/**
 * @brief Initialize a Hu3D light as a normalized spot light.
 * @param light Light structure to initialize.
 * @param arg0 Light position.
 * @param arg1 Direction vector, normalized before storage.
 * @param arg2 RGBA light color.
 * @return Indeterminate; the original non-void routine reaches the end without returning a value.
 */
inline s16 Hu3DLightCreateV(HU3DLIGHT *light, Vec *arg0, Vec *arg1, GXColor *arg2) {
    light->type = 0;
    light->pos = *arg0;
    light->dir = *arg1;
    light->offset.x = light->offset.y = light->offset.z = 0.0f;
    light->cutoff = 30.0f;
    light->func = 2;
    VECNormalize(&light->dir, &light->dir);
    light->color = *arg2;
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
    HU3DLIGHT* var_r31;

    var_r31 = Hu3DGlobalLight;

    for (light_id = 0; light_id < 8; light_id++, var_r31++) {
        if (var_r31->type == -1) {
            break;
        }
    }
    if (light_id == 8) {
        return -1;
    }

    Hu3DLightCreateV(var_r31, pos, dir, color);

    return light_id;
}

/**
 * @brief Create a model-local light from scalar position, direction, and color values.
 * @param arg0 Model ID that will own the light.
 * @param arg8 Light-position X coordinate.
 * @param arg9 Light-position Y coordinate.
 * @param argA Light-position Z coordinate.
 * @param argB Direction or aim X component.
 * @param argC Direction or aim Y component.
 * @param argD Direction or aim Z component.
 * @param arg1 Red component from 0 through 255.
 * @param arg2 Green component from 0 through 255.
 * @param arg3 Blue component from 0 through 255.
 * @return The model-local light slot from 0 through 7, or -1 if no global local-light storage or model slot is free.
 */
s16 Hu3DLLightCreate(s16 arg0, f32 arg8, f32 arg9, f32 argA, f32 argB, f32 argC, f32 argD, u8 arg1, u8 arg2, u8 arg3) {
    Vec vec1;
    Vec vec2;
    GXColor color;

    vec1.x = arg8;
    vec1.y = arg9;
    vec1.z = argA;
    vec2.x = argB;
    vec2.y = argC;
    vec2.z = argD;
    color.r = arg1;
    color.g = arg2;
    color.b = arg3;
    color.a = 0xFF;
    return Hu3DLLightCreateV(arg0, &vec1, &vec2, &color);
}

/**
 * @brief Create a model-local light from vectors.
 * @param arg0 Model ID that will own the light.
 * @param arg1 Light position.
 * @param arg2 Direction vector, normalized before storage.
 * @param arg3 RGBA light color.
 * @return The model-local light slot from 0 through 7, or -1 if no storage or model slot is free.
 */
s16 Hu3DLLightCreateV(s16 arg0, Vec* arg1, Vec* arg2, GXColor* arg3) {
    HU3DMODEL* temp_r29;
    s16 slot_index;
    s16 light_id;
    HU3DLIGHT* var_r31;

    temp_r29 = &Hu3DData[arg0];
    var_r31 = Hu3DLocalLight;
    for (slot_index = 0; slot_index < 0x20; slot_index++, var_r31++) {
        if (var_r31->type == -1) {
            break;
        }
    }
    if (slot_index == 0x20) {
        return -1;
    }

    Hu3DLightCreateV(var_r31, arg1, arg2, arg3);

    for (light_id = 0; light_id < 8; light_id++) {
        if (temp_r29->lLightId[light_id] == -1) {
            break;
        }
    }
    if (light_id == 8) {
        return -1;
    }
    temp_r29->lLightId[light_id] = slot_index;
    temp_r29->attr |= HU3D_ATTR_LLIGHT;
    return light_id;
}

/**
 * @brief Configure a Hu3D light for spot attenuation.
 * @param light Light structure to update.
 * @param arg8 Spot cutoff angle in degrees.
 * @param arg1 GX spot-function selector.
 */
inline void Hu3DLightSpotSet(HU3DLIGHT *light, u16 arg1, f32 arg8) {
    light->type &= 0xFF00;
    light->cutoff = arg8;
    light->func = arg1;
}

/**
 * @brief Configure a Hu3D light for spot attenuation.
 * @param arg0 Global-light index.
 * @param arg8 Spot cutoff angle in degrees.
 * @param arg1 GX spot-function selector.
 */
void Hu3DGLightSpotSet(s16 arg0, f32 arg8, u16 arg1) {
    HU3DLIGHT *light = &Hu3DGlobalLight[arg0];

    Hu3DLightSpotSet(light, arg1, arg8);
}

/**
 * @brief Configure a Hu3D light for spot attenuation.
 * @param arg0 Model ID that owns the light.
 * @param arg1 Model-local light slot.
 * @param arg8 Spot cutoff angle in degrees.
 * @param arg2 GX spot-function selector.
 */
void Hu3DLLightSpotSet(s16 arg0, s16 arg1, f32 arg8, u16 arg2) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[arg0];
    light = &Hu3DLocalLight[data->lLightId[arg1]];
    Hu3DLightSpotSet(light, arg2, arg8);
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
 * @param lightIndex Global-light index.
 */
void Hu3DGLightInfinitytSet(s16 lightIndex) {
    HU3DLIGHT* light = &Hu3DGlobalLight[lightIndex];

    Hu3DLightInfinitytSet(light);
}

/**
 * @brief Configure a Hu3D light as an infinite directional light.
 * @param dataIndex Model ID that owns the light.
 * @param lightIndex Model-local light slot.
 */
void Hu3DLLightInfinitytSet(s16 dataIndex, s16 lightIndex) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[dataIndex];
    light = &Hu3DLocalLight[data->lLightId[lightIndex]];
    Hu3DLightInfinitytSet(light);
}

/**
 * @brief Configure a Hu3D light for point-distance attenuation.
 * @param light Light structure to update.
 * @param arg8 Reference distance used by GX attenuation.
 * @param arg9 Brightness at the reference distance.
 * @param arg1 GX distance-attenuation function selector.
 */
inline void Hu3DLightPointSet(HU3DLIGHT *light, f32 arg8, f32 arg9, u16 arg1) {
    light->type &= 0xFF00;
    light->type |= 2;
    light->cutoff = arg8;
    light->brightness = arg9;
    light->func = arg1;
}

/**
 * @brief Configure a Hu3D light for point-distance attenuation.
 * @param arg0 Global-light index.
 * @param arg8 Reference distance used by GX attenuation.
 * @param arg9 Brightness at the reference distance.
 * @param arg1 GX distance-attenuation function selector.
 */
void Hu3DGLightPointSet(s16 arg0, f32 arg8, f32 arg9, u16 arg1) {
    HU3DLIGHT* light = &Hu3DGlobalLight[arg0];

    Hu3DLightPointSet(light, arg8, arg9, arg1);
}

/**
 * @brief Configure a Hu3D light for point-distance attenuation.
 * @param arg0 Model ID that owns the light.
 * @param arg1 Model-local light slot.
 * @param arg8 Reference distance used by GX attenuation.
 * @param arg9 Brightness at the reference distance.
 * @param arg2 GX distance-attenuation function selector.
 */
void Hu3DLLightPointSet(s16 arg0, s16 arg1, f32 arg8, f32 arg9, u16 arg2) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[arg0];
    light = &Hu3DLocalLight[data->lLightId[arg1]];
    Hu3DLightPointSet(light, arg8, arg9, arg2);
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
 * @param dataIndex Model ID that owns the light.
 * @param lightIndex Model-local light slot to release.
 */
void Hu3DLLightKill(s16 dataIndex, s16 lightIndex) {
    HU3DMODEL* temp_r31;
    HU3DLIGHT* light;
    s16 global_light_id;

    temp_r31 = &Hu3DData[dataIndex];
    light = &Hu3DLocalLight[temp_r31->lLightId[lightIndex]];
    light->type = -1;
    temp_r31->lLightId[lightIndex] = -1;

    for (global_light_id = 0; global_light_id < 8; global_light_id++) {
        if (temp_r31->lLightId[global_light_id] == -1) {
            break;
        }
    }
    if (global_light_id == 8) {
        temp_r31->attr &= ~HU3D_ATTR_LLIGHT;
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
 * @param dataIndex Model ID that owns the light.
 * @param lightIndex Model-local light slot.
 * @param r Red component from 0 through 255.
 * @param g Green component from 0 through 255.
 * @param b Blue component from 0 through 255.
 * @param a Alpha component from 0 through 255.
 */
void Hu3DLLightColorSet(s16 dataIndex, s16 lightIndex, u8 r, u8 g, u8 b, u8 a) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[dataIndex];
    light = &Hu3DLocalLight[data->lLightId[lightIndex]];
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
 * @param dataIndex Model ID that owns the light.
 * @param lightIndex Model-local light slot.
 * @param pos Light position.
 * @param aim Direction vector; despite the name, it is normalized directly rather than treated as a target point.
 */
void Hu3DLLightPosSetV(s16 dataIndex, s16 lightIndex, Vec* pos, Vec* aim) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[dataIndex];
    light = &Hu3DLocalLight[data->lLightId[lightIndex]];
    Hu3DLightPosSetV(light, pos, aim);
}

/**
 * @brief Set a light position and normalized direction from scalar components.
 * @param light Light structure to update.
 * @param arg8 Light-position X coordinate.
 * @param arg9 Light-position Y coordinate.
 * @param argA Light-position Z coordinate.
 * @param argB Direction-vector X component.
 * @param argC Direction-vector Y component.
 * @param argD Direction-vector Z component.
 */
inline void Hu3DLightPosSet(HU3DLIGHT *light, f32 arg8, f32 arg9, f32 argA, f32 argB, f32 argC, f32 argD) {
    light->pos.x = arg8;
    light->pos.y = arg9;
    light->pos.z = argA;
    light->dir.x = argB;
    light->dir.y = argC;
    light->dir.z = argD;
    VECNormalize(&light->dir, &light->dir);
}

/**
 * @brief Set a global light position and normalized direction from scalar components.
 * @param arg0 Global-light index.
 * @param arg8 Light-position X coordinate.
 * @param arg9 Light-position Y coordinate.
 * @param argA Light-position Z coordinate.
 * @param argB Direction-vector X component.
 * @param argC Direction-vector Y component.
 * @param argD Direction-vector Z component.
 */
void Hu3DGLightPosSet(s16 arg0, f32 arg8, f32 arg9, f32 argA, f32 argB, f32 argC, f32 argD) {
    HU3DLIGHT* light;

    light = &Hu3DGlobalLight[arg0];
    Hu3DLightPosSet(light, arg8, arg9, argA, argB, argC, argD);
}

/**
 * @brief Set a model-local light position and normalized direction from scalar components.
 * @param arg0 Model ID that owns the light.
 * @param arg1 Model-local light slot.
 * @param arg8 Light-position X coordinate.
 * @param arg9 Light-position Y coordinate.
 * @param argA Light-position Z coordinate.
 * @param argB Direction-vector X component.
 * @param argC Direction-vector Y component.
 * @param argD Direction-vector Z component.
 */
void Hu3DLLightPosSet(s16 arg0, s16 arg1, f32 arg8, f32 arg9, f32 argA, f32 argB, f32 argC, f32 argD) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[arg0];
    light = &Hu3DLocalLight[data->lLightId[arg1]];
    Hu3DLightPosSet(light, arg8, arg9, argA, argB, argC, argD);
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
 * @param dataIndex Model ID that owns the light.
 * @param lightIndex Model-local light slot.
 * @param pos Light position.
 * @param aim Target point used to derive the normalized direction.
 */
void Hu3DLLightPosAimSetV(s16 dataIndex, s16 lightIndex, Vec* pos, Vec* aim) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[dataIndex];
    light = &Hu3DLocalLight[data->lLightId[lightIndex]];
    Hu3DLightPosAimSetV(light, pos, aim);
}

/**
 * @brief Set a global light position and aim point from scalar components.
 * @param arg0 Global-light index.
 * @param arg8 Light-position X coordinate.
 * @param arg9 Light-position Y coordinate.
 * @param argA Light-position Z coordinate.
 * @param argB Target X coordinate.
 * @param argC Target Y coordinate.
 * @param argD Target Z coordinate.
 */
void Hu3DGLightPosAimSet(s16 arg0, f32 arg8, f32 arg9, f32 argA, f32 argB, f32 argC, f32 argD) {
    Vec pos;
    Vec aim;
    HU3DLIGHT *light;
    HU3DLIGHT *light2;

    pos.x = arg8;
    pos.y = arg9;
    pos.z = argA;
    aim.x = argB;
    aim.y = argC;
    aim.z = argD;

    light = &Hu3DGlobalLight[arg0];
    light2 = light;
    Hu3DLightPosAimSetV(light2, &pos, &aim);
}

/**
 * @brief Set a model-local light position and aim point from scalar components.
 * @param arg0 Model ID that owns the light.
 * @param arg1 Model-local light slot.
 * @param arg8 Light-position X coordinate.
 * @param arg9 Light-position Y coordinate.
 * @param argA Light-position Z coordinate.
 * @param argB Target X coordinate.
 * @param argC Target Y coordinate.
 * @param argD Target Z coordinate.
 */
void Hu3DLLightPosAimSet(s16 arg0, s16 arg1, f32 arg8, f32 arg9, f32 argA, f32 argB, f32 argC, f32 argD) {
    Vec pos;
    Vec aim;
    HU3DMODEL* data;
    HU3DLIGHT* light;
    HU3DLIGHT* light2;

    pos.x = arg8;
    pos.y = arg9;
    pos.z = argA;
    aim.x = argB;
    aim.y = argC;
    aim.z = argD;

    data = &Hu3DData[arg0];
    light = &Hu3DLocalLight[data->lLightId[arg1]];
    light2 = light;
    Hu3DLightPosAimSetV(light2, &pos, &aim);
}

/**
 * @brief Control whether a light is transformed by the supplied model matrices.
 * @param light Light structure to update.
 * @param arg1 Nonzero to mark the light for matrix transformation; zero to keep coordinates unchanged.
 */
inline void Hu3DLightStaticSet(HU3DLIGHT *light, s32 arg1) {
    if (arg1 != 0) {
        light->type |= 0x8000;
    } else {
        light->type &= ~0x8000;
    }
}

/**
 * @brief Control matrix transformation for a global light.
 * @param index Global-light index.
 * @param arg1 Nonzero to transform the light by draw matrices; zero to use stored coordinates.
 */
void Hu3DGLightStaticSet(s16 index, s32 arg1) {
    HU3DLIGHT* light = &Hu3DGlobalLight[index];

    Hu3DLightStaticSet(light, arg1);
}

/**
 * @brief Control matrix transformation for a model-local light.
 * @param dataIndex Model ID that owns the light.
 * @param lightIndex Model-local light slot.
 * @param arg2 Nonzero to transform the light by draw matrices; zero to use stored coordinates.
 */
void Hu3DLLightStaticSet(s16 dataIndex, s16 lightIndex, s32 arg2) {
    HU3DMODEL* data;
    HU3DLIGHT* light;

    data = &Hu3DData[dataIndex];
    light = &Hu3DLocalLight[data->lLightId[lightIndex]];
    Hu3DLightStaticSet(light, arg2);
}

/**
 * @brief Create global lights from the HSF light objects in a model.
 * @param arg0 Model ID whose HSF object table is scanned.
 * @param arg1 Nonzero to mark created lights for matrix transformation.
 * @return The number of created lights, capped at eight; an existing nonzero count is returned unchanged.
 */
s32 Hu3DModelLightInfoSet(s16 arg0, s16 arg1) {
    Vec direction;
    Vec unused_pos;
    Vec unused_target;
    HU3DLIGHT* sp1C;
    s16 light_id;
    u8 red;
    u8 green;
    u8 blue;
    HSFDATA* temp_r21;
    HSFOBJECT* var_r18;
    HSFOBJECT* var_r31;
    HU3DMODEL* temp_r28;
    s16 object_index;
    s16 light_count;

    temp_r28 = &Hu3DData[arg0];
    temp_r21 = temp_r28->hsf;
    if (temp_r28->lightNum != 0) {
        return temp_r28->lightNum;
    }
    var_r31 = temp_r21->object;

    for (object_index = light_count = 0; object_index < temp_r21->objectNum; object_index++, var_r31++) {
        var_r18 = var_r31;
        if (var_r18->type != 8) {
            continue;
        }
        direction.x = var_r18->light.target.x - var_r18->light.pos.x;
        direction.y = var_r18->light.target.y - var_r18->light.pos.y;
        direction.z = var_r18->light.target.z - var_r18->light.pos.z;
        blue = var_r18->light.b;
        green = var_r18->light.g;
        red = var_r18->light.r;
        light_id = Hu3DGLightCreate(var_r18->light.pos.x, var_r18->light.pos.y, var_r18->light.pos.z,
                                direction.x, direction.y, direction.z, red, green, blue);


        temp_r28->lightId[light_count] = light_id;
        sp1C = &Hu3DGlobalLight[light_id];
        Hu3DGLightStaticSet(light_id, arg1);
        switch (var_r18->light.type) {
            case 0:
                Hu3DGLightSpotSet(light_id, var_r18->light.cutoff, 2);
                break;
            case 1:
                Hu3DGLightPointSet(light_id, var_r18->mesh.base.scale.x - var_r18->mesh.base.rot.z, 1.0f, 2);
                Hu3DGLightPosSet(light_id, var_r18->light.pos.x, var_r18->light.pos.y, var_r18->light.pos.z, var_r18->light.target.x, var_r18->light.target.y, var_r18->light.target.z);
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
    temp_r28->lightNum = light_count;
    return light_count;
}

/**
 * @brief Configure and load all active global and model-local GX lights for a draw.
 * @param arg0 Model whose local lights are included.
 * @param arg1 Position transformation matrix.
 * @param arg2 Direction transformation matrix.
 * @param arg8 Specular exponent control; zero selects ordinary light direction setup.
 * @return Bit mask of GX light slots loaded by the routine.
 */
s16 Hu3DLightSet(HU3DMODEL* arg0, Mtx *arg1, Mtx *arg2, f32 arg8) {
    s16 light_bit;
    HU3DLIGHT* var_r29;
    s16 light_mask;
    s16 i;

    light_mask = 0;
    light_bit = 1;
    var_r29 = Hu3DGlobalLight;

    for (i = 0; i < 8; i++, var_r29++) {
        if (var_r29->type != -1) {
            lightSet(var_r29, light_bit, arg2, arg1, arg8);
            light_mask |= light_bit;
            light_bit <<= 1;
        }
    }
    if ((arg0->attr & HU3D_ATTR_LLIGHT) != 0) {
        for (i = 0; i < 8; i++) {
            if (arg0->lLightId[i] != -1) {
                var_r29 = &Hu3DLocalLight[arg0->lLightId[i]];
                lightSet(var_r29, light_bit, arg2, arg1, arg8);
                light_mask |= light_bit;
                light_bit <<= 1;
            }
        }
    }
    return light_mask;
}

/**
 * @brief Configure and immediately load one GX light object.
 * @param arg0 Hu3D light state to convert.
 * @param arg1 GX light-slot bit.
 * @param arg2 Matrix used to transform directions for flagged lights.
 * @param arg3 Matrix used to transform positions for flagged lights.
 * @param arg8 Specular exponent control; zero uses diffuse direction setup.
 */
void lightSet(HU3DLIGHT* arg0, s16 arg1, Mtx *arg2, Mtx *arg3, f32 arg8) {
    GXLightObj gx_light;
    Point3d direction;
    Point3d position;

    switch ((u8)arg0->type) {
    case 0:
        GXInitLightAttn(&gx_light, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        GXInitLightSpot(&gx_light, arg0->cutoff, arg0->func);
        break;
    case 1:
        GXInitLightAttn(&gx_light, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        GXInitLightSpot(&gx_light, 20.0f, GX_SP_COS);
        GXInitLightAttnK(&gx_light, 1.0f, 0.0f, 0.0f);
        VECScale(&arg0->dir, &arg0->pos, -1000000.0f);
        break;
    case 2:
        GXInitLightAttn(&gx_light, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        GXInitLightDistAttn(&gx_light, arg0->cutoff, arg0->brightness, arg0->func);
        break;
    }
    if ((arg0->type & 0x8000) != 0) {
        PSMTXMultVec(*arg2, &arg0->dir, &direction);
        PSMTXMultVec(*arg3, &arg0->pos, &position);
        GXInitLightPos(&gx_light, position.x, position.y, position.z);
    } else {
        GXInitLightPos(&gx_light, arg0->pos.x, arg0->pos.y, arg0->pos.z);
        direction = arg0->dir;
    }
    if (0.0f == arg8) {
        GXInitLightDir(&gx_light, direction.x, direction.y, direction.z);
    } else {
        GXInitSpecularDir(&gx_light, direction.x, direction.y, direction.z);
        GXInitLightAttn(&gx_light, 0.0f, 0.0f, 1.0f, arg8 / 2, 0.0f, 1.0f - (arg8 / 2));
    }
    GXInitLightColor(&gx_light, arg0->color);
    GXLoadLightObjImm(&gx_light, arg1);
}

/**
 * @brief Replace the first reflection-map animation.
 * @param arg0 Animation data to use as reflection map zero; the previous non-static animation is freed.
 */
void Hu3DReflectMapSet(ANIMDATA* arg0) {

    if (reflectAnim[0] != (ANIMDATA*) refMapData0) {
        HuMemDirectFree(reflectAnim[0]);
    }
    reflectAnim[0] = HuSprAnimRead(arg0);
    reflectMapNo = 0;
}

/**
 * @brief Select the active built-in reflection map.
 * @param arg0 Reflection-map index.
 */
void Hu3DReflectNoSet(s16 arg0) {
    reflectMapNo = arg0;
}

/**
 * @brief Enable linear fog with supplied distances and color.
 * @param arg0 Fog start distance.
 * @param arg1 Fog end distance.
 * @param arg2 Red fog component from 0 through 255.
 * @param arg3 Green fog component from 0 through 255.
 * @param arg4 Blue fog component from 0 through 255.
 */
void Hu3DFogSet(f32 arg0, f32 arg1, u8 arg2, u8 arg3, u8 arg4) {
    FogData.fogType = 4;
    FogData.fogStart = arg0;
    FogData.fogEnd = arg1;
    FogData.color.r = arg2;
    FogData.color.g = arg3;
    FogData.color.b = arg4;
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
 * @param arg8 Shadow-camera field of view in degrees.
 * @param arg9 Near clipping-plane distance.
 * @param argA Far clipping-plane distance.
 */
void Hu3DShadowCreate(f32 arg8, f32 arg9, f32 argA) {
    Hu3DShadowData.size = 0xC0;
    if (Hu3DShadowData.buf == 0) {
        Hu3DShadowData.buf = HuMemDirectMalloc(HEAP_MODEL, SHADOW_HEAP_SIZE);
    }
    Hu3DShadowData.fov = arg8;
    Hu3DShadowData.nnear = arg9;
    Hu3DShadowData.ffar = argA;
    Hu3DShadowData.camPos.x = 300.0f;
    Hu3DShadowData.camPos.y = 300.0f;
    Hu3DShadowData.camPos.z = 0.0f;
    Hu3DShadowData.camTarget.x = Hu3DShadowData.camTarget.y = Hu3DShadowData.camTarget.z = 0.0f;
    Hu3DShadowData.camUp.x = -1.0f;
    Hu3DShadowData.camUp.y = 1.0f;
    Hu3DShadowData.camUp.z = 0.0f;
    C_MTXLightPerspective(Hu3DShadowData.projMtx, arg8, HU_DISP_ASPECT, 0.5f, -0.5f, 0.5f, 0.5f);
    VECNormalize(&Hu3DShadowData.camUp, &Hu3DShadowData.camUp);
    Hu3DShadowData.alpha = 0x80;
    Hu3DShadowF = 1;
}

/**
 * @brief Set the shadow camera's position, up vector, and target.
 * @param camPos Shadow-camera position.
 * @param camUp Shadow-camera up vector.
 * @param camTarget Shadow-camera target point.
 */
void Hu3DShadowPosSet(Vec* camPos, Vec* camUp, Vec* camTarget) {
    Hu3DShadowData.camPos = *camPos;
    Hu3DShadowData.camTarget = *camTarget;
    Hu3DShadowData.camUp = *camUp;
}

/**
 * @brief Set shadow opacity.
 * @param arg8 Shadow opacity multiplier converted to an 8-bit alpha value.
 */
void Hu3DShadowTPLvlSet(f32 arg8) {
    Hu3DShadowData.alpha = 255.0f * arg8;
}

/**
 * @brief Resize and reallocate the square shadow-map buffer.
 * @param arg0 Shadow-map width and height in pixels.
 */
void Hu3DShadowSizeSet(u16 arg0) {
    Hu3DShadowData.size = arg0;
    if (Hu3DShadowData.buf != 0) {
        HuMemDirectFree(Hu3DShadowData.buf);
    }
    Hu3DShadowData.buf = HuMemDirectMalloc(HEAP_MODEL, arg0 * arg0);
}

/**
 * @brief Render all eligible shadow-casting models into the shadow-map texture.
 *
 * @details The routine installs a shadow camera, draws casters, copies the embedded framebuffer into the R8 texture, then restores the main viewport and basic GX state.
 */
void Hu3DShadowExec(void) {
    HU3DMODEL* var_r31;
    s16 model_id;
    Mtx model_mtx;
    Mtx draw_mtx;
    Mtx rotation_mtx;
    Mtx44 projection_mtx;
    GXColor clear_color = {0, 0, 0, 0};
    s32 test;
    s32 test2;

    Hu3DDrawPreInit();
    GXSetCopyClear(clear_color, 0xFFFFFF);
    C_MTXPerspective(projection_mtx, Hu3DShadowData.fov, HU_DISP_ASPECT, Hu3DShadowData.nnear, Hu3DShadowData.ffar);
    GXSetProjection(projection_mtx, GX_PERSPECTIVE);
    if (Hu3DShadowData.size <= 0xF0) {
        GXSetScissor(2, 2, Hu3DShadowData.size * 2 - 4, Hu3DShadowData.size * 2 - 4);
        GXSetViewport(0.0f, 0.0f, Hu3DShadowData.size * 2, Hu3DShadowData.size * 2, 0.0f, 1.0f);
        test = (Hu3DShadowData.size / 2) * (Hu3DShadowData.size / 2);
    } else {
        GXSetScissor(1, 1, Hu3DShadowData.size - 2, Hu3DShadowData.size - 2);
        GXSetViewport(0.0f, 0.0f, Hu3DShadowData.size, Hu3DShadowData.size, 0.0f, 1.0f);
        test = Hu3DShadowData.size * Hu3DShadowData.size;
    }
    C_MTXLookAt(Hu3DCameraMtx, &Hu3DShadowData.camPos, &Hu3DShadowData.camUp, &Hu3DShadowData.camTarget);
    PSMTXCopy(Hu3DCameraMtx, Hu3DShadowData.lookAtMtx);
    var_r31 = Hu3DData;
    shadowModelDrawF = 1;
    GXInvalidateTexAll();
    GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, BGColor);

    for (model_id = 0; model_id < HU3D_MODEL_MAX; model_id++, var_r31++) {
        if (var_r31->hsf != 0 && (var_r31->attr & HU3D_ATTR_SHADOW) != 0 && (var_r31->attr & HU3D_ATTR_DISPOFF) == 0 && (var_r31->attr & HU3D_ATTR_HOOK) == 0) {
            if ((var_r31->attr & HU3D_ATTR_MOTION_OFF) != 0) {
                test2 = 0;
                if (var_r31->motId != -1) {
                    Hu3DMotionExec(model_id, var_r31->motId, var_r31->motWork.time, 0);
                }
                if (var_r31->motIdShift != -1) {
                    Hu3DSubMotionExec(model_id);
                }
                if (var_r31->motIdOvl != -1) {
                    Hu3DMotionExec(model_id, var_r31->motIdOvl, var_r31->motOvlWork.time, 1);
                }
                if ((var_r31->attr & HU3D_ATTR_CLUSTER_ON) != 0) {
                    ClusterMotionExec(var_r31);
                    test2 = 1;
                }
                if (var_r31->motIdShape != -1) {
                    if (var_r31->motId == -1) {
                        Hu3DMotionExec(model_id, var_r31->motIdShape, var_r31->motShapeWork.time, 0);
                    } else {
                        Hu3DMotionExec(model_id, var_r31->motIdShape, var_r31->motShapeWork.time, 1);
                    }
                }
                if ((var_r31->attr & (HU3D_ATTR_HOOKFUNC|HU3D_ATTR_ENVELOPE_OFF)) == 0 || (var_r31->motAttr & HU3D_MOTATTR_PAUSE) == 0) {
                    test2 = 1;
                    InitVtxParm(var_r31->hsf);
                    if (var_r31->motIdShape != -1) {
                        ShapeProc(var_r31->hsf);
                    }
                    if ((var_r31->attr & HU3D_ATTR_CLUSTER_ON) != 0) {
                        ClusterProc(var_r31);
                    }
                    if (var_r31->hsf->cenvNum != 0) {
                        EnvelopeProc(var_r31->hsf);
                    }
                    PPCSync();
                }
                var_r31->attr |= HU3D_ATTR_MOT_EXEC;
            }
            mtxRot(rotation_mtx, var_r31->rot.x, var_r31->rot.y, var_r31->rot.z);
            PSMTXScale(model_mtx, var_r31->scale.x, var_r31->scale.y, var_r31->scale.z);
            PSMTXConcat(rotation_mtx, model_mtx, model_mtx);
            mtxTransCat(model_mtx, var_r31->pos.x, var_r31->pos.y, var_r31->pos.z);
            PSMTXConcat(Hu3DCameraMtx, model_mtx, draw_mtx);
            PSMTXConcat(draw_mtx, var_r31->mtx, draw_mtx);
            Hu3DDraw(var_r31, draw_mtx, &var_r31->scale);
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
 * @param arg0 Animation data used as the projected texture; retained by the slot.
 * @param arg8 Projection field of view in degrees.
 * @param arg9 Near clipping-plane distance.
 * @param argA Far clipping-plane distance.
 * @return The projection slot from 0 through 3, or -1 when all slots are occupied.
 */
s16 Hu3DProjectionCreate(void *arg0, f32 arg8, f32 arg9, f32 argA) {
    s16 projection_id;
    HU3DPROJECTION* var_r31;

    var_r31 = Hu3DProjection;

    for (projection_id = 0; projection_id < 4; projection_id++, var_r31++) {
        if (var_r31->anim == 0U) {
            break;
        }
    }
    if ((s16) projection_id == 4) {
        return -1;
    }
    var_r31->anim = arg0;
    var_r31->fov = arg8;
    var_r31->nnear = arg9;
    var_r31->ffar = argA;
    var_r31->camPos.x = 1000.0f;
    var_r31->camPos.y = 1000.0f;
    var_r31->camPos.z = 0.0f;
    var_r31->camTarget.x = var_r31->camTarget.y = var_r31->camTarget.z = 0.0f;
    var_r31->camUp.x = -1.0f;
    var_r31->camUp.y = 1.0f;
    var_r31->camUp.z = 0.0f;
    C_MTXLightPerspective(var_r31->projMtx, arg8, HU_DISP_ASPECT, 0.5f, -0.5f, 0.5f, 0.5f);
    VECNormalize(&var_r31->camUp, &var_r31->camUp);
    var_r31->alpha = 0x80;
    Hu3DProjectionNum++;
    return projection_id;
}

/**
 * @brief Release a projection slot's animation and mark the slot unused.
 * @param arg0 Projection slot from 0 through 3.
 */
void Hu3DProjectionKill(s16 arg0) {
    HuSprAnimKill(Hu3DProjection[arg0].anim);
    Hu3DProjection[arg0].anim = NULL;
}

/**
 * @brief Set a projection camera's position, up vector, and target.
 * @param arg0 Projection slot from 0 through 3.
 * @param arg1 Projection-camera position.
 * @param arg2 Projection-camera up vector.
 * @param arg3 Projection-camera target point.
 */
void Hu3DProjectionPosSet(s16 arg0, Vec* arg1, Vec* arg2, Vec* arg3) {
    Hu3DProjection[arg0].camPos = *arg1;
    Hu3DProjection[arg0].camTarget = *arg3;
    Hu3DProjection[arg0].camUp = *arg2;
}

/**
 * @brief Set a projection texture's opacity.
 * @param arg0 Projection slot from 0 through 3.
 * @param arg8 Opacity multiplier converted to an 8-bit alpha value.
 */
void Hu3DProjectionTPLvlSet(s16 arg0, f32 arg8) {
    Hu3DProjection[arg0].alpha = 255.0f * arg8;
}

/**
 * @brief Replace an HSF bitmap with mip levels from sprite animation data.
 *
 * @details Mip image data is allocated from the model heap and copied from successive animation bitmaps.
 * @param arg0 Serialized sprite animation containing the replacement mip images.
 * @param arg1 Model ID containing the target bitmap.
 * @param arg2 Target bitmap name.
 * @param arg8 LOD bias stored on the bitmap.
 */
void Hu3DMipMapSet(char* arg0, s16 arg1, char *arg2, f32 arg8) {
    HSFBITMAP* temp_r31;
    ANIMBMP* var_r30;
    s16 i;
    HSFDATA* temp_r27;
    HSFATTRIBUTE* var_r26;
    HU3DMODEL* temp_r25;
    s32 level_size;
    void *level_data;
    void* temp_r22;
    ANIMDATA* temp_r3;

    temp_r25 = &Hu3DData[arg1];
    temp_r27 = temp_r25->hsf;
    var_r26 = temp_r27->attribute;
    for (i = 0; i < temp_r27->attributeNum; i++, var_r26++) {
        if (strcmp(arg2, var_r26->bitmap->name) == 0) {
            break;
        }
    }
    if (i == temp_r27->attributeNum) {
        OSReport("Error: Not Found %s for MipMapSet\n", arg2);
        return;
    }
    temp_r31 = var_r26->bitmap;
    temp_r3 = HuSprAnimRead(arg0);
    var_r30 = temp_r3->bmp;
    for ( i = 0, level_size = i; i < temp_r3->bmpNum; i++, var_r30++) {
        level_size += var_r30->dataSize;
    }
    level_data = HuMemDirectMallocNum(HEAP_MODEL, level_size, temp_r25->mallocNo);
    temp_r22 = level_data;
    var_r30 = temp_r3->bmp;
    temp_r31->data = temp_r22;
    temp_r31->sizeX = var_r30->sizeX;
    temp_r31->sizeY = var_r30->sizeY;
    temp_r31->palSize = var_r30->palNum;
    temp_r31->palData = var_r30->palData;
    temp_r31->maxLod = temp_r3->bmpNum;
    switch (var_r30->dataFmt) {
        case 0:
            temp_r31->dataFmt = 6;
            break;
        case 2:
            temp_r31->dataFmt = 5;
            break;
        case 3:
            temp_r31->dataFmt = 0xA;
            temp_r31->pixSize = 8;
            break;
        case 4:
            temp_r31->dataFmt = 0xA;
            temp_r31->pixSize = 4;
            break;
    }

    for (i = 0; i < temp_r3->bmpNum; i++, var_r30++) {
        memcpy(level_data, var_r30->data, var_r30->dataSize);
        level_data = (void*) ((int) level_data + var_r30->dataSize);
    }
    DCFlushRange(temp_r22, level_size);
}
