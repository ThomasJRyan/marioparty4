#include "game/ClusterExec.h"
#include "game/EnvelopeExec.h"
#include "game/ShapeExec.h"
#include "game/hsfload.h"
#include "game/hu3d.h"
#include "game/init.h"

#include "ext_math.h"
#include <string.h>

#define HU3D_MOTATTR_SHIFT_ALL (HU3D_MOTATTR_SHIFT_LOOP|HU3D_MOTATTR_SHIFT_PAUSE|HU3D_MOTATTR_SHIFT_REV)
#define HU3D_MOTATTR_NOSHIFT_ALL (HU3D_MOTATTR_LOOP|HU3D_MOTATTR_PAUSE|HU3D_MOTATTR_REV)
#define HU3D_MOTATTR_ALL (HU3D_MOTATTR_SHIFT_ALL|HU3D_MOTATTR_NOSHIFT_ALL)

static s32 SearchObjectIndex(HSFDATA *arg0, u32 arg1);
static s32 SearchAttributeIndex(HSFDATA *arg0, u32 arg1);

HU3DMOTION Hu3DMotion[HU3D_MOTION_MAX];

static HSFBITMAP *bitMapPtr;

/**
 * @brief Clear HSF pointers through the motion-sized view used during Hu3D initialization.
 *
 * @details The original routine casts `Hu3DData` to motion entries and clears
 * each viewed HSF pointer; this documents the observable target without
 * assuming the cast was intended to reference `Hu3DMotion`.
 */
void Hu3DMotionInit(void) {
    HU3DMOTION *motion;
    s16 i;

    motion = (HU3DMOTION*) Hu3DData;
    for (i = 0; i < HU3D_MOTION_MAX; i++, motion++) {
        motion->hsf = 0;
    }
}

/**
 * @brief Load HSF data into the first free motion-table slot.
 * @param arg0 Pointer to the serialized HSF data to load.
 * @return The allocated motion ID, or -1 when the motion table is full.
 */
s16 Hu3DMotionCreate(void *arg0) {
    HU3DMOTION *motion;
    s16 i;

    motion = Hu3DMotion;
    for (i = 0; i < HU3D_MOTION_MAX; i++, motion++) {
        if (motion->hsf == 0) {
            break;
        }
    }
    if (i == HU3D_MOTION_MAX) {
        OSReport("Error: Create Motion Over!\n");
        return -1;
    }
    motion->hsf = LoadHSF(arg0);
    motion->attr = 0;
    motion->modelId = -1;
    return i;
}

/**
 * @brief Create a motion entry that shares an existing model's HSF data.
 * @param arg0 Model ID whose HSF motion data will back the new entry.
 * @return The allocated motion ID, or -1 when the motion table is full.
 */
s16 Hu3DMotionModelCreate(s16 arg0) {
    HU3DMODEL *model = &Hu3DData[arg0];
    HU3DMOTION *motion;
    s16 i;

    motion = Hu3DMotion;
    for (i = 0; i < HU3D_MOTION_MAX; i++, motion++) {
        if (motion->hsf == 0) {
            break;
        }
    }
    if (i == HU3D_MOTION_MAX) {
        OSReport("Error: Create Motion Over!\n");
        return -1;
    }
    motion->hsf = model->hsf;
    motion->attr = 0;
    motion->modelId = arg0;
    model->motIdSrc = i;
    return i;
}

/**
 * @brief Release a motion entry when no other model is actively using it.
 * @param arg0 Motion ID to release.
 * @return 1 if the motion was released; 0 if the slot was empty or another model still uses it.
 */
s32 Hu3DMotionKill(s16 arg0) {
    HU3DMODEL *model;
    HU3DMOTION *motion;
    s16 i;

    motion = &Hu3DMotion[arg0];
    if (motion->hsf == 0) {
        return 0;
    }
    model = Hu3DData;
    for (i = 0; i < 512; i++, model++) {
        if (model->hsf && model->motId == arg0 && motion->modelId != i) {
            break;
        }
    }
    if (i != 512) {
        return 0;
    }
    if (motion->modelId == -1) {
        HuMemDirectFree(motion->hsf);
    } else {
        Hu3DData[motion->modelId].motIdSrc = -1;
    }
    motion->hsf = NULL;
    return 1;
}

/**
 * @brief Attempt to release every populated motion-table entry.
 */
void Hu3DMotionAllKill(void) {
    HU3DMOTION *motion;
    s16 i;

    motion = Hu3DMotion;
    for (i = 0; i < HU3D_MOTION_MAX; i++, motion++) {
        if (motion->hsf) {
            Hu3DMotionKill(i);
        }
    }
}

/**
 * @brief Assign a base motion to a model and rewind it to the motion start.
 * @param arg0 Model ID to update.
 * @param arg1 Motion ID to use as the model's base motion.
 */
void Hu3DMotionSet(s16 arg0, s16 arg1) {
    Hu3DData[arg0].motIdShift = -1;
    Hu3DData[arg0].motId = arg1;
    Hu3DData[arg0].motWork.time = 0.0f;
    Hu3DData[arg0].motWork.start = 0.0f;
    Hu3DData[arg0].motWork.end = Hu3DMotionMaxTimeGet(arg0);
}

/**
 * @brief Assign and rewind a model's overlay motion.
 * @param arg0 Model ID to update.
 * @param arg1 Motion ID to use as the overlay.
 */
void Hu3DMotionOverlaySet(s16 arg0, s16 arg1) {
    Hu3DData[arg0].motIdOvl = arg1;
    Hu3DData[arg0].motOvlWork.time = 0.0f;
    Hu3DData[arg0].motOvlWork.speed = 1.0f;
}

/**
 * @brief Remove a model's overlay motion.
 * @param arg0 Model ID whose overlay motion is cleared.
 */
void Hu3DMotionOverlayReset(s16 arg0) {
    Hu3DData[arg0].motIdOvl = -1;
}

/**
 * @brief Get a model's current overlay-motion time.
 * @param arg0 Model ID to query.
 * @return The overlay time in HSF motion timeline units.
 */
float Hu3DMotionOverlayTimeGet(s16 arg0) {
    return Hu3DData[arg0].motOvlWork.time;
}

/**
 * @brief Set a model's overlay-motion time without clamping it.
 * @param arg0 Model ID to update.
 * @param arg1 Overlay time in HSF motion timeline units.
 */
void Hu3DMotionOverlayTimeSet(s16 arg0, float arg1) {
    Hu3DData[arg0].motOvlWork.time = arg1;
}

/**
 * @brief Set a model's overlay-motion playback rate.
 * @param arg0 Model ID to update.
 * @param arg1 Timeline units advanced per frame multiplier.
 */
void Hu3DMotionOverlaySpeedSet(s16 arg0, float arg1) {
    Hu3DData[arg0].motOvlWork.speed = arg1;
}

/**
 * @brief Begin blending a model from its current motion to another motion.
 *
 * @details Any prior shift is promoted to the base motion before the new shift state is installed.
 * @param arg0 Model ID to update.
 * @param arg1 Motion ID to blend toward.
 * @param arg2 Initial time for the incoming motion, in HSF timeline units.
 * @param arg3 Blend duration in frame-timing units.
 * @param arg4 Playback attributes for the current and incoming motions.
 */
void Hu3DMotionShiftSet(s16 arg0, s16 arg1, float arg2, float arg3, u32 arg4) {
    HU3DMODEL *model = &Hu3DData[arg0];
    HU3DMOTION *shift_motion = &Hu3DMotion[arg1];
    s32 motion_attrs;

    arg4 &= ~HU3D_MOTATTR;
    motion_attrs = 0;
    if (model->motIdShift != -1) {
        model->motId = model->motIdShift;
        model->motWork.time = model->motShiftWork.time;
        model->motWork.speed = model->motShiftWork.speed;
        model->motWork.start = model->motShiftWork.start;
        model->motWork.end = model->motShiftWork.end;
        if (arg4 & HU3D_MOTATTR_SHIFT_LOOP) {
            motion_attrs |= HU3D_MOTATTR_LOOP;
        }
        if (arg4 & HU3D_MOTATTR_SHIFT_PAUSE) {
            motion_attrs |= HU3D_MOTATTR_PAUSE;
        }
        if (arg4 & HU3D_MOTATTR_SHIFT_REV) {
            motion_attrs |= HU3D_MOTATTR_REV;
        }
        model->motAttr &= ~HU3D_MOTATTR_ALL;
        model->motAttr |= motion_attrs;
        model->motAttr &= ~HU3D_MOTATTR;
    } else {
        model->motAttr &= ~HU3D_MOTATTR_SHIFT_ALL;
    }
    model->motIdShift = arg1;
    model->motShiftWork.time = arg2;
    model->motShiftWork.speed = 1.0f;
    model->motOvlWork.start = 0.0f;
    model->motOvlWork.end = arg3;
    model->motShiftWork.start = 0.0f;
    model->motShiftWork.end = Hu3DMotionShiftMaxTimeGet(arg0);
    if (arg4 & HU3D_MOTATTR_LOOP) {
        motion_attrs |= HU3D_MOTATTR_SHIFT_LOOP;
    }
    if (arg4 & HU3D_MOTATTR_PAUSE) {
        motion_attrs |= HU3D_MOTATTR_SHIFT_PAUSE;
    }
    if (arg4 & HU3D_MOTATTR_REV) {
        motion_attrs |= HU3D_MOTATTR_SHIFT_REV;
    }
    arg4 &= ~HU3D_MOTATTR_NOSHIFT_ALL;
    model->motAttr |= motion_attrs | arg4;
    model->motAttr &= ~HU3D_MOTATTR;
}

/**
 * @brief Assign and initialize a model's shape motion.
 * @param arg0 Model ID to update.
 * @param arg1 Motion ID containing the shape tracks.
 */
void Hu3DMotionShapeSet(s16 arg0, s16 arg1) {
    Hu3DData[arg0].motIdShape = arg1;
    Hu3DData[arg0].motShapeWork.time = 0.0f;
    Hu3DData[arg0].motShapeWork.speed = 1.0f;
    Hu3DData[arg0].motShapeWork.start = 0.0f;
    Hu3DData[arg0].motShapeWork.end = Hu3DMotionShapeMaxTimeGet(arg0);
}

/**
 * @brief Get the shape-motion ID assigned to a model.
 * @param arg0 Model ID to query.
 * @return The shape-motion ID, or -1 when none is assigned.
 */
s16 Hu3DMotionShapeIDGet(s16 arg0) {
    return Hu3DData[arg0].motIdShape;
}

/**
 * @brief Set a model's shape-motion playback rate.
 * @param arg0 Model ID to update.
 * @param arg1 Timeline units advanced per frame multiplier.
 */
void Hu3DMotionShapeSpeedSet(s16 arg0, float arg1) {
    HU3DMODEL *model = &Hu3DData[arg0];

    model->motShapeWork.speed = arg1;
}

/**
 * @brief Set a model's current shape-motion time.
 * @param arg0 Model ID to update.
 * @param arg1 Shape-motion time in HSF timeline units.
 */
void Hu3DMotionShapeTimeSet(s16 arg0, float arg1) {
    Hu3DData[arg0].motShapeWork.time = arg1;
}

/**
 * @brief Get the maximum time of a model's assigned shape motion.
 * @param arg0 Model ID to query.
 * @return The shape motion's maximum HSF timeline time, or 0 if its motion entry is empty.
 */
float Hu3DMotionShapeMaxTimeGet(s16 arg0) {
    HU3DMODEL *model = &Hu3DData[arg0];

    return Hu3DMotionMotionMaxTimeGet(model->motIdShape);
}

/**
 * @brief Set the playback range for a model's shape motion.
 * @param arg0 Model ID to update.
 * @param arg1 Inclusive playback start time in HSF timeline units.
 * @param arg2 Playback end time in HSF timeline units.
 */
void Hu3DMotionShapeStartEndSet(s16 arg0, float arg1, float arg2) {
    HU3DMODEL *model = &Hu3DData[arg0];

    model->motShapeWork.start = arg1;
    model->motShapeWork.end = arg2;
}

/**
 * @brief Attach a cluster motion to the first free cluster slot on a model.
 * @param arg0 Model ID to update.
 * @param arg1 Motion ID containing cluster tracks.
 * @return The assigned cluster slot from 0 through 3, or -1 when all slots are occupied.
 */
s16 Hu3DMotionClusterSet(s16 arg0, s16 arg1) {
    HU3DMODEL *model = &Hu3DData[arg0];
    s16 i;

    for (i = 0; i < 4; i++) {
        if (model->motIdCluster[i] == -1) {
            model->motIdCluster[i] = arg1;
            model->clusterTime[i] = 0.0f;
            model->clusterSpeed[i] = 1.0f;
            model->clusterAttr[i] = HU3D_ATTR_NONE;
            model->attr |= HU3D_ATTR_CLUSTER_ON;
            ClusterAdjustObject(model->hsf, Hu3DMotion[arg1].hsf);
            return i;
        }
    }
    OSReport("Error: Cluster Entry Over\n");
    return -1;
}

/**
 * @brief Replace a specified cluster-motion slot on a model.
 * @param arg0 Model ID to update.
 * @param arg1 Motion ID containing cluster tracks.
 * @param arg2 Cluster slot from 0 through 3.
 * @return The supplied cluster slot.
 */
s16 Hu3DMotionClusterNoSet(s16 arg0, s16 arg1, s16 arg2) {
    HU3DMODEL *model = &Hu3DData[arg0];

    Hu3DMotionClusterReset(arg0, arg2);
    model->motIdCluster[arg2] = arg1;
    model->clusterTime[arg2] = 0.0f;
    model->clusterSpeed[arg2] = 1.0f;
    model->attr |= HU3D_ATTR_CLUSTER_ON;
    ClusterAdjustObject(model->hsf, Hu3DMotion[arg1].hsf);
    return arg2;
}

/**
 * @brief Remove a model's shape motion.
 * @param arg0 Model ID whose shape motion is cleared.
 */
void Hu3DMotionShapeReset(s16 arg0) {
    Hu3DData[arg0].motIdShape = -1;
}

/**
 * @brief Remove one or all cluster motions from a model.
 * @param arg0 Model ID to update.
 * @param arg1 Cluster slot to clear, or -1 to clear all four slots.
 */
void Hu3DMotionClusterReset(s16 arg0, s16 arg1) {
    HU3DMODEL *model = &Hu3DData[arg0];
    s16 i;

    if (arg1 == -1) {
        for (i = 0; i < 4; i++) {
            model->motIdCluster[i] = -1;
        }
        model->attr &= ~HU3D_ATTR_CLUSTER_ON;
    } else {
        model->motIdCluster[arg1] = -1;
        for (i = 0; i < 4; i++) {
            if (model->motIdCluster[i] != -1) {
                return;
            }
        }
        model->attr &= ~HU3D_ATTR_CLUSTER_ON;
    }
}

/**
 * @brief Get a model's base-motion ID.
 * @param arg0 Model ID to query.
 * @return The base-motion ID, or -1 when none is assigned.
 */
s16 Hu3DMotionIDGet(s16 arg0) {
    HU3DMODEL *model = &Hu3DData[arg0];

    return model->motId;
}

/**
 * @brief Get a model's incoming shift-motion ID.
 * @param arg0 Model ID to query.
 * @return The shift-motion ID, or -1 when no blend is active.
 */
s16 Hu3DMotionShiftIDGet(s16 arg0) {
    HU3DMODEL *model = &Hu3DData[arg0];

    return model->motIdShift;
}

/**
 * @brief Set and clamp a model's base-motion time.
 *
 * @details Paused skinned models are evaluated immediately so their envelope vertices reflect the new time.
 * @param arg0 Model ID to update.
 * @param arg1 Requested time in HSF timeline units; clamped to zero through the motion maximum.
 */
void Hu3DMotionTimeSet(s16 arg0, float arg1) {
    HU3DMODEL *model = &Hu3DData[arg0];

    if (Hu3DMotionMaxTimeGet(arg0) <= arg1) {
        arg1 = Hu3DMotionMaxTimeGet(arg0);
    }
    if (arg1 < 0.0f) {
        arg1 = 0.0f;
    }
    model->motWork.time = arg1;
    if (model->hsf != (HSFDATA*) -1 && model->hsf->cenvNum != 0 && (model->motAttr & HU3D_MOTATTR_PAUSE)) {
        Hu3DMotionExec(arg0, model->motId, arg1, 0);
        if (model->motIdShift != -1) {
            Hu3DSubMotionExec(arg0);
        }
        EnvelopeProc(model->hsf);
    }
}

/**
 * @brief Get a model's current base-motion time.
 * @param arg0 Model ID to query.
 * @return The base-motion time in HSF timeline units.
 */
float Hu3DMotionTimeGet(s16 arg0) {
    HU3DMODEL *model = &Hu3DData[arg0];

    return model->motWork.time;
}

/**
 * @brief Get a model's current shift-motion time.
 * @param arg0 Model ID to query.
 * @return The incoming motion time in HSF timeline units.
 */
float Hu3DMotionShiftTimeGet(s16 arg0) {
    HU3DMODEL *model = &Hu3DData[arg0];

    return model->motShiftWork.time;
}

/**
 * @brief Get the maximum time of a model's base motion.
 * @param arg0 Model ID to query.
 * @return The base motion's maximum HSF time after conversion through the routine's signed 16-bit temporary, or 0 if no motion is assigned.
 */
float Hu3DMotionMaxTimeGet(s16 arg0) {
    HU3DMODEL *model = &Hu3DData[arg0];
    HU3DMOTION *motion;
    HSFMOTION *hsf_motion;
    s16 max_time;

    if (model->motId == -1) {
        return 0.0f;
    }
    motion = &Hu3DMotion[model->motId];
    hsf_motion = motion->hsf->motion;
    max_time = 0.0001 + hsf_motion->maxTime;
    return max_time;
}

/**
 * @brief Get the maximum time of a model's shift motion.
 * @param arg0 Model ID to query.
 * @return The shift motion's maximum HSF time after conversion through the routine's signed 16-bit temporary, or 0 if no shift is assigned.
 */
float Hu3DMotionShiftMaxTimeGet(s16 arg0) {
    HU3DMODEL *model = &Hu3DData[arg0];
    HU3DMOTION *motion;
    HSFMOTION *hsf_motion;
    s16 max_time;

    if (model->motIdShift == -1) {
        return 0.0f;
    }
    motion = &Hu3DMotion[model->motIdShift];
    hsf_motion = motion->hsf->motion;
    max_time = 0.0001 + hsf_motion->maxTime;
    return max_time;
}

/**
 * @brief Set the playback range for a model's incoming shift motion.
 * @param arg0 Model ID to update.
 * @param arg1 Inclusive playback start time in HSF timeline units.
 * @param arg2 Playback end time in HSF timeline units.
 */
void Hu3DMotionShiftStartEndSet(s16 arg0, float arg1, float arg2) {
    HU3DMODEL *model = &Hu3DData[arg0];

    model->motShiftWork.start = arg1;
    model->motShiftWork.end = arg2;
}

/**
 * @brief Get the maximum time stored in a motion-table entry.
 * @param arg0 Motion ID to query.
 * @return The HSF maximum time after conversion through the routine's signed 16-bit temporary, or 0 if the slot is empty.
 */
float Hu3DMotionMotionMaxTimeGet(s16 arg0) {
    HU3DMOTION *motion = &Hu3DMotion[arg0];
    HSFMOTION *hsf_motion;
    s16 max_time;

    if (motion->hsf == 0) {
        return 0.0f;
    }
    hsf_motion = motion->hsf->motion;
    max_time = 0.0001 + hsf_motion->maxTime;
    return max_time;
}

/**
 * @brief Set the playback range for a model's base motion.
 * @param arg0 Model ID to update.
 * @param arg1 Inclusive playback start time in HSF timeline units.
 * @param arg2 Playback end time in HSF timeline units.
 */
void Hu3DMotionStartEndSet(s16 arg0, float arg1, float arg2) {
    HU3DMODEL *model = &Hu3DData[arg0];

    model->motWork.start = arg1;
    model->motWork.end = arg2;
}

/**
 * @brief Check whether a model's base motion has reached its playback boundary.
 * @param arg0 Model ID to query.
 * @return Nonzero at the maximum time during forward playback or at zero during reverse playback; otherwise zero.
 */
s32 Hu3DMotionEndCheck(s16 arg0) {
    if (!(Hu3DData[arg0].motAttr & HU3D_MOTATTR_REV)) {
        return (Hu3DMotionMaxTimeGet(arg0) <= Hu3DMotionTimeGet(arg0));
    } else {
        return (Hu3DMotionTimeGet(arg0) <= 0.0f);
    }
}

/**
 * @brief Set a model's base-motion playback rate.
 * @param arg0 Model ID to update.
 * @param arg1 Timeline units advanced per frame multiplier.
 */
void Hu3DMotionSpeedSet(s16 arg0, float arg1) {
    HU3DMODEL *model = &Hu3DData[arg0];

    model->motWork.speed = arg1;
}

/**
 * @brief Set a model's shift-motion playback rate.
 * @param arg0 Model ID to update.
 * @param arg1 Timeline units advanced per frame multiplier.
 */
void Hu3DMotionShiftSpeedSet(s16 arg0, float arg1) {
    HU3DMODEL *model = &Hu3DData[arg0];

    model->motShiftWork.speed = arg1;
}

/**
 * @brief Prevent selected position or rotation channels from being changed by motion.
 *
 * @details Selected channels are restored to their base transform when the mask is enabled.
 * @param arg0 Model ID containing the object.
 * @param arg1 Name of the HSF object to protect.
 * @param arg2 Bit mask selecting position and rotation channels to preserve.
 */
void Hu3DMotionNoMotSet(s16 arg0, char *arg1, u32 arg2) {
    HSFCONSTDATA *constant_data;
    HSFOBJECT *object;

    object = Hu3DModelObjPtrGet(arg0, arg1);
    if (object->constData == 0) {
        constant_data = ObjConstantMake(object, (u32) Hu3DData[arg0].mallocNo);
    } else {
        constant_data = object->constData;
    }
    constant_data->attr |= arg2;
    if (arg2 & 0x10) {
        object->mesh.curr.pos.x = object->mesh.base.pos.x;
    }
    if (arg2 & 0x20) {
        object->mesh.curr.pos.y = object->mesh.base.pos.y;
    }
    if (arg2 & 0x40) {
        object->mesh.curr.pos.z = object->mesh.base.pos.z;
    }
    if (arg2 & 0x80) {
        object->mesh.curr.rot.x = object->mesh.base.rot.x;
    }
    if (arg2 & 0x100) {
        object->mesh.curr.rot.y = object->mesh.base.rot.y;
    }
    if (arg2 & 0x200) {
        object->mesh.curr.rot.z = object->mesh.base.rot.z;
    }
}

/**
 * @brief Allow motion to update selected object transform channels again.
 * @param arg0 Model ID containing the object.
 * @param arg1 Name of the HSF object to update.
 * @param arg2 Bit mask of position and rotation channel locks to clear.
 */
void Hu3DMotionNoMotReset(s16 arg0, char *arg1, u32 arg2) {
    HSFOBJECT *object;
    HSFCONSTDATA *constant_data;

    object = Hu3DModelObjPtrGet(arg0, arg1);
    constant_data = object->constData;
    constant_data->attr &= ~arg2;
}

/**
 * @brief Lock selected object transform channels to a supplied value.
 * @param arg0 Model ID containing the object.
 * @param arg1 Name of the HSF object to update.
 * @param arg2 Bit mask selecting position and rotation channels to lock.
 * @param arg3 Value written to every selected transform channel.
 */
void Hu3DMotionForceSet(s16 arg0, char *arg1, u32 arg2, float arg3) {
    HSFCONSTDATA *constant_data;
    HSFOBJECT *object;

    object = Hu3DModelObjPtrGet(arg0, arg1);
    if (object->constData == 0) {
        constant_data = ObjConstantMake(object, (u32) Hu3DData[arg0].mallocNo);
    } else {
        constant_data = object->constData;
    }
    constant_data->attr |= arg2;
    if (arg2 & 0x10) {
        object->mesh.curr.pos.x = arg3;
    }
    if (arg2 & 0x20) {
        object->mesh.curr.pos.y = arg3;
    }
    if (arg2 & 0x40) {
        object->mesh.curr.pos.z = arg3;
    }
    if (arg2 & 0x80) {
        object->mesh.curr.rot.x = arg3;
    }
    if (arg2 & 0x100) {
        object->mesh.curr.rot.y = arg3;
    }
    if (arg2 & 0x200) {
        object->mesh.curr.rot.z = arg3;
    }
}

/**
 * @brief Advance all motion timelines attached to a model by one frame step.
 *
 * @details Playback attributes control pausing, reversal, looping, clamping, and completion of shift blends.
 * @param arg0 Model ID whose base, overlay, shift, shape, and cluster timelines advance.
 */
void Hu3DMotionNext(s16 arg0) {
    HU3DMODEL *model = &Hu3DData[arg0];
    HSFMOTION *hsf_motion;
    HU3DMOTION *motion;
    u32 motion_attrs;
    s16 i;

    motion = &Hu3DMotion[model->motId];
    hsf_motion = motion->hsf->motion;
    motion_attrs = model->motAttr;
    if (model->motId != -1) {
        motion = &Hu3DMotion[model->motId];
        if (!(motion_attrs & HU3D_MOTATTR_PAUSE)) {
            if (!(motion_attrs & HU3D_MOTATTR_REV)) {
                model->motWork.time += model->motWork.speed * minimumVcountf;
            } else {
                model->motWork.time -= model->motWork.speed * minimumVcountf;
            }
            if (motion_attrs & HU3D_MOTATTR_LOOP) {
                if (model->motWork.time < model->motWork.start) {
                    model->motWork.time = model->motWork.end - (model->motWork.start - model->motWork.time);
                } else if (model->motWork.time >= model->motWork.end) {
                    model->motWork.time = model->motWork.start + (model->motWork.time - model->motWork.end);
                }
            } else if (model->motWork.time < 0.0f) {
                model->motWork.time = 0.0f;
            } else if (model->motWork.time >= model->motWork.end) {
                model->motWork.time = model->motWork.end;
            }
        }
    }
    if (model->motIdOvl != -1) {
        motion = &Hu3DMotion[model->motIdOvl];
        hsf_motion = motion->hsf->motion;
        if (!(motion_attrs & HU3D_MOTATTR_OVL_PAUSE)) {
            if (!(motion_attrs & HU3D_MOTATTR_OVL_REV)) {
                model->motOvlWork.time += model->motOvlWork.speed * minimumVcountf;
            } else {
                model->motOvlWork.time -= model->motOvlWork.speed * minimumVcountf;
            }
            if (motion_attrs & HU3D_MOTATTR_OVL_LOOP) {
                if (model->motOvlWork.time < 0.0f) {
                    model->motOvlWork.time = hsf_motion->maxTime;
                } else if (model->motOvlWork.time >= hsf_motion->maxTime) {
                    model->motOvlWork.time = 0.0f;
                }
            } else if (model->motOvlWork.time < 0.0f) {
                model->motOvlWork.time = 0.0f;
            } else if (model->motOvlWork.time >= hsf_motion->maxTime) {
                model->motOvlWork.time = hsf_motion->maxTime;
            }
        }
    }
    if (model->motIdShift != -1) {
        model->motOvlWork.start += minimumVcountf;
        if (model->motOvlWork.start >= model->motOvlWork.end) {
            model->motId = model->motIdShift;
            model->motWork.time = model->motShiftWork.time;
            model->motWork.speed = model->motShiftWork.speed;
            model->motWork.start = model->motShiftWork.start;
            model->motWork.end = model->motShiftWork.end;
            model->motIdShift = -1;
            motion_attrs = 0;
            if (model->motAttr & HU3D_MOTATTR_SHIFT_LOOP) {
                motion_attrs |= HU3D_MOTATTR_LOOP;
            }
            if (model->motAttr & HU3D_MOTATTR_SHIFT_PAUSE) {
                motion_attrs |= HU3D_MOTATTR_PAUSE;
            }
            if (model->motAttr & HU3D_MOTATTR_SHIFT_REV) {
                motion_attrs |= HU3D_MOTATTR_REV;
            }
            model->motAttr &= ~HU3D_MOTATTR_ALL;
            model->motAttr |= motion_attrs;
            model->motAttr &= ~HU3D_MOTATTR;
            return;
        }
        if (!(model->motAttr & HU3D_MOTATTR_SHIFT_PAUSE)) {
            motion = &Hu3DMotion[model->motIdShift];
            if (!(model->motAttr & HU3D_MOTATTR_SHIFT_REV)) {
                model->motShiftWork.time += model->motShiftWork.speed * minimumVcountf;
            } else {
                model->motShiftWork.time -= model->motShiftWork.speed * minimumVcountf;
            }
            if (model->motAttr & HU3D_MOTATTR_SHIFT_LOOP) {
                if (model->motShiftWork.time < model->motShiftWork.start) {
                    model->motShiftWork.time = model->motShiftWork.end;
                } else if (model->motShiftWork.time >= model->motShiftWork.end) {
                    model->motShiftWork.time = model->motShiftWork.start;
                }
            } else if (model->motShiftWork.time < model->motShiftWork.start) {
                model->motShiftWork.time = model->motShiftWork.start;
            } else if (model->motShiftWork.time >= model->motShiftWork.end) {
                model->motShiftWork.time = model->motShiftWork.end;
            }
        }
    }
    if (model->motIdShape != -1 && !(motion_attrs & HU3D_MOTATTR_SHAPE_PAUSE)) {
        motion = &Hu3DMotion[model->motIdShape];
        hsf_motion = motion->hsf->motion;
        if (!(motion_attrs & HU3D_MOTATTR_SHAPE_REV)) {
            model->motShapeWork.time += model->motShapeWork.speed * minimumVcountf;
        } else {
            model->motShapeWork.time -= model->motShapeWork.speed * minimumVcountf;
        }
        if (motion_attrs & HU3D_MOTATTR_SHAPE_LOOP) {
            if (model->motShapeWork.time < model->motShapeWork.start) {
                model->motShapeWork.time = model->motShapeWork.end;
            } else if (model->motShapeWork.time >= model->motShapeWork.end) {
                model->motShapeWork.time = model->motShapeWork.start;
            }
        } else if (model->motShapeWork.time < model->motShapeWork.start) {
            model->motShapeWork.time = model->motShapeWork.start;
        } else if (model->motShapeWork.time >= model->motShapeWork.end) {
            model->motShapeWork.time = model->motShapeWork.end;
        }
    }
    if (model->attr & HU3D_ATTR_CLUSTER_ON) {
        for (i = 0; i < 4; i++) {
            if (model->motIdCluster[i] != -1 && !(model->clusterAttr[i] & HU3D_CLUSTER_ATTR_PAUSE)) {
                motion = &Hu3DMotion[model->motIdCluster[i]];
                hsf_motion = motion->hsf->motion;
                if (!(model->clusterAttr[i] & HU3D_CLUSTER_ATTR_REV)) {
                    model->clusterTime[i] += model->clusterSpeed[i] * minimumVcountf;
                } else {
                    model->clusterTime[i] -= model->clusterSpeed[i] * minimumVcountf;
                }
                if (model->clusterAttr[i] & HU3D_CLUSTER_ATTR_LOOP) {
                    if (model->clusterTime[i] < 0.0f) {
                        model->clusterTime[i] = hsf_motion->maxTime;
                    } else if (model->clusterTime[i] >= hsf_motion->maxTime) {
                        model->clusterTime[i] = 0.0f;
                    }
                } else if (model->clusterTime[i] < 0.0f) {
                    model->clusterTime[i] = 0.0f;
                } else if (model->clusterTime[i] >= hsf_motion->maxTime) {
                    model->clusterTime[i] = hsf_motion->maxTime;
                }
            }
        }
    }
}

/**
 * @brief Evaluate one HSF motion and apply its tracks to a model.
 * @param arg0 Destination model ID.
 * @param arg1 Motion ID whose tracks are evaluated.
 * @param arg2 Evaluation time in HSF timeline units.
 * @param arg3 Zero to reset unprotected object transforms to their base values first; nonzero for overlay evaluation.
 */
void Hu3DMotionExec(s16 arg0, s16 arg1, float arg2, s32 arg3) {
    HU3DMOTION *motion;
    HSFDATA *motion_hsf;
    HSFTRACK *track_end;
    HSFCONSTDATA *constant_data;
    HSFDATA *model_hsf;
    HSFMOTION *hsf_motion;
    HSFOBJECT *object;
    HSFOBJECT *object_cursor;
    HSFCLUSTER *cluster;
    HSFTRACK *track;
    HSFTRACK *morph_track;
    HSFTRACK *cluster_track;
    HSFTRACK *attribute_track;
    HU3DMODEL *model;
    s16 constant_attrs;
    s16 object_index;
    float *transform_value;

    model = &Hu3DData[arg0];
    motion = &Hu3DMotion[arg1];
    model_hsf = model->hsf;
    motion_hsf = motion->hsf;
    hsf_motion = motion_hsf->motion;
    track = hsf_motion->track;
    object_cursor = model_hsf->object;
    if (arg3 == 0) {
        for (object_index = 0; object_index < model_hsf->objectNum; object_cursor++, object_index++) {
            object = object_cursor;
            if (object->constData) {
                constant_data = object->constData;
                if (constant_data->attr & 0x3F0) {
                    constant_attrs = constant_data->attr;
                    if (!(constant_attrs & 0x10)) {
                        object->mesh.curr.pos.x = object->mesh.base.pos.x;
                    }
                    if (!(constant_attrs & 0x20)) {
                        object->mesh.curr.pos.y = object->mesh.base.pos.y;
                    }
                    if (!(constant_attrs & 0x40)) {
                        object->mesh.curr.pos.z = object->mesh.base.pos.z;
                    }
                    if (!(constant_attrs & 0x80)) {
                        object->mesh.curr.rot.x = object->mesh.base.rot.x;
                    }
                    if (!(constant_attrs & 0x100)) {
                        object->mesh.curr.rot.y = object->mesh.base.rot.y;
                    }
                    if (!(constant_attrs & 0x200)) {
                        object->mesh.curr.rot.z = object->mesh.base.rot.z;
                    }
                } else {
                    object->mesh.curr = object->mesh.base;
                }
            } else {
                object->mesh.curr = object->mesh.base;
            }
        }
    }
    track_end = &track[hsf_motion->numTracks];
    for (; track < track_end; track++) {
        switch (track->type) {
            case 2:
                if (track->target < model_hsf->objectNum && track->target != -1) {
                    object = &model_hsf->object[track->target];
                    if (track->channel == 0x28) {
                        object->mesh.mesh.baseMorph = GetCurve(track, arg2);
                    } else if (object->type == 7) {
                        if (model->attr & HU3D_ATTR_CAMERA_MOTON) {
                            SetObjCameraMotion(arg0, track, GetCurve(track, arg2));
                        }
                    } else if (object->type == 8) {
                        SetObjLightMotion(arg0, track, GetCurve(track, arg2));
                    } else if (track->channel == 0x18) {
                        if (object->constData) {
                            constant_data = object->constData;
                            if (GetCurve(track, arg2) == 1.0f) {
                                constant_data->attr &= ~0x1000;
                            } else {
                                constant_data->attr |= 0x1000;
                            }
                        }
                    } else if (track->channel == 0x1A) {
                        if (object->constData) {
                            constant_data = object->constData;
                            if (GetCurve(track, arg2) == 1.0f) {
                                constant_data->attr &= ~0x2000;
                            } else {
                                constant_data->attr |= 0x2000;
                            }
                        }
                    } else {
                        transform_value = GetObjTRXPtr(object, track->channel);
                        if (transform_value != (float*) -1) {
                            *transform_value = GetCurve(track, arg2);
                        }
                    }
                }
                break;
            case 3:
                morph_track = track;
                if (morph_track->target < model_hsf->objectNum) {
                    object = &model_hsf->object[morph_track->target];
                    object->mesh.mesh.morphWeight[morph_track->morphWeight] = GetCurve(morph_track, arg2);
                }
                break;
            case 9:
                if (!(model->attr & HU3D_ATTR_CURVE_MOTOFF)) {
                    if (track->attrIdx < model_hsf->materialNum) {
                        SetObjMatMotion(arg0, track, GetCurve(track, arg2));
                    }
                }
                break;
            case 5:
                if (!(model->attr & HU3D_ATTR_CURVE_MOTOFF)) {
                    cluster = &model_hsf->cluster[track->cluster];
                    cluster->index = GetClusterCurve(track, arg2);
                }
                break;
            case 6:
                if (!(model->attr & HU3D_ATTR_CURVE_MOTOFF)) {
                    cluster_track = track;
                    cluster = &model_hsf->cluster[cluster_track->cluster];
                    cluster->weight[cluster_track->clusterWeight] = GetClusterWeightCurve(cluster_track, arg2);
                }
                break;
            case 10:
                attribute_track = track;
                if (attribute_track->cluster != -1 || !(model->attr & HU3D_ATTR_CURVE_MOTOFF)) {
                    if (attribute_track->attrIdx != -1 && attribute_track->attrIdx < model_hsf->attributeNum) {
                        SetObjAttrMotion(arg0, attribute_track, GetCurve(attribute_track, arg2));
                    }
                }
                break;
        }
    }
}

/**
 * @brief Evaluate camera tracks from a model's active base motion.
 * @param arg0 Model ID providing the camera motion and camera-selection mask.
 */
void Hu3DCameraMotionExec(s16 arg0) {
    HU3DMODEL *model;
    HU3DMOTION *motion;
    HSFDATA *hsf;
    HSFMOTION *hsf_motion;
    HSFTRACK *track;
    HSFTRACK *track_end;

    model = &Hu3DData[arg0];
    motion = &Hu3DMotion[model->motId];
    hsf = motion->hsf;
    hsf_motion = hsf->motion;
    track = hsf_motion->track;
    if (model->attr & HU3D_ATTR_CAMERA_MOTON) {
        track_end = &track[hsf_motion->numTracks];
        for (; track < track_end; track++) {
            if (track->type == 2 && track->index == 7) {
                SetObjCameraMotion(arg0, track, GetCurve(track, model->motWork.time));
            }
        }
    }
}

/**
 * @brief Blend transform tracks from a model's incoming shift motion.
 *
 * @details Rotation channels take the shortest wrapped path across zero degrees before interpolation.
 * @param arg0 Model ID with an active shift blend.
 */
void Hu3DSubMotionExec(s16 arg0) {
    HU3DMODEL *model;
    HU3DMOTION *shift_motion;
    HSFDATA *model_hsf;
    HSFDATA *motion_hsf;
    HSFMOTION *hsf_motion;
    HSFTRACK *track;
    HSFOBJECT *object_cursor;
    HSFOBJECT *object;
    float *transform_value;
    float blend_weight;
    float target_value;
    s16 channel;
    s16 track_index;

    model = &Hu3DData[arg0];
    shift_motion = &Hu3DMotion[model->motIdShift];
    model_hsf = model->hsf;
    motion_hsf = shift_motion->hsf;
    hsf_motion = motion_hsf->motion;
    track = hsf_motion->track;
    object_cursor = model_hsf->object;
    if (model->motId == -1) {
        for (track_index = 0; track_index < model_hsf->objectNum; object_cursor++, track_index++) {
            object = object_cursor;
            object->mesh.curr = object->mesh.base;
        }
    }
    if (model->motOvlWork.end) {
        blend_weight = model->motOvlWork.start / model->motOvlWork.end;
    } else {
        blend_weight = 1.0f;
    }
    for (track_index = 0; track_index < hsf_motion->numTracks; track_index++, track++) {
        switch (track->type) {
            case 2:
                if (track->target < model_hsf->objectNum && track->target != -1) {
                    object = &model_hsf->object[track->target];
                    channel = track->channel;
                    transform_value = GetObjTRXPtr(object, channel);
                    if (transform_value != (float*) -1) {
                        if (channel == 0x1C || channel == 0x1D || channel == 0x1E) {
                            target_value = GetCurve(track, model->motShiftWork.time);
                            if (target_value < 0.0f) {
                                target_value += 360.0f;
                            }
                            if (*transform_value < 0.0f) {
                                *transform_value += 360.0f;
                            }
                            if (target_value < 180.0f) {
                                if (*transform_value > (180.0f + target_value)) {
                                    *transform_value -= 360.0f;
                                }
                            } else if (*transform_value < (target_value - 180.0f)) {
                                target_value -= 360.0f;
                            }
                            *transform_value = (1.0f - blend_weight) * *transform_value + blend_weight * target_value;
                        } else {
                            *transform_value = (1.0f - blend_weight) * *transform_value + blend_weight * GetCurve(track, model->motShiftWork.time);
                        }
                    }
                }
                break;
        }
    }
}

/**
 * @brief Resolve an HSF object transform channel to writable storage.
 * @param arg0 Object whose current transform is queried.
 * @param arg1 HSF channel number selecting position, rotation, or scale.
 * @return A pointer to the selected component, or `(float *)-1` for an unsupported or motion-locked channel.
 */
__declspec(weak) float *GetObjTRXPtr(HSFOBJECT *arg0, u16 arg1) {
    HSFCONSTDATA *constant_data = arg0->constData;

    switch (arg1) {
        case 8:
            if (constant_data && (constant_data->attr & 0x10)) {
                return (float*) -1;
            }
            return &arg0->mesh.curr.pos.x;
        case 9:
            if (constant_data && (constant_data->attr & 0x20)) {
                return (float*) -1;
            }
            return &arg0->mesh.curr.pos.y;
        case 10:
            if (constant_data && (constant_data->attr & 0x40)) {
                return (float*) -1;
            }
            return &arg0->mesh.curr.pos.z;
        case 28:
            if (constant_data && (constant_data->attr & 0x80)) {
                return (float*) -1;
            }
            return &arg0->mesh.curr.rot.x;
        case 29:
            if (constant_data && (constant_data->attr & 0x100)) {
                return (float*) -1;
            }
            return &arg0->mesh.curr.rot.y;
        case 30:
            if (constant_data && (constant_data->attr & 0x200)) {
                return (float*) -1;
            }
            return &arg0->mesh.curr.rot.z;
        case 31:
            return &arg0->mesh.curr.scale.x;
        case 32:
            return &arg0->mesh.curr.scale.y;
        case 33:
            return &arg0->mesh.curr.scale.z;
        default:
            return (float*) -1;
    }
}

/**
 * @brief Apply an evaluated track value to one HSF material channel.
 * @param arg0 Model ID containing the material.
 * @param arg1 Track whose attribute index and channel select the destination.
 * @param arg2 Evaluated value; color and alpha channels are clamped to 0 through 1.
 */
void SetObjMatMotion(s16 arg0, HSFTRACK *arg1, float arg2) {
    HSFMATERIAL *material;
    HSFDATA *hsf;
    HU3DMODEL *model;
    float clamped_value;

    model = &Hu3DData[arg0];
    hsf = model->hsf;
    material = &hsf->material[arg1->attrIdx];
    clamped_value = arg2;
    if (arg2 > 1.0f) {
        clamped_value = 1.0f;
    } else if (arg2 < 0.0f) {
        clamped_value = 0.0f;
    }
    switch (arg1->channel) {
        case 0:
            material->litColor[0] = clamped_value * 255.0f;
            break;
        case 1:
            material->litColor[1] = clamped_value * 255.0f;
            break;
        case 2:
            material->litColor[2] = clamped_value * 255.0f;
            break;
        case 0x31:
            material->color[0] = clamped_value * 255.0f;
            break;
        case 0x32:
            material->color[1] = clamped_value * 255.0f;
            break;
        case 0x33:
            material->color[2] = clamped_value * 255.0f;
            break;
        case 0x34:
            material->shadowColor[0] = clamped_value * 255.0f;
            break;
        case 0x35:
            material->shadowColor[1] = clamped_value * 255.0f;
            break;
        case 0x36:
            material->shadowColor[2] = clamped_value * 255.0f;
            break;
        case 0x39:
            if (!(model->attr & HU3D_ATTR_TPLVL_SET)) {
                material->invAlpha = clamped_value;
            }
            break;
        case 0x3C:
            material->refAlpha = clamped_value;
            break;
    }
}

/**
 * @brief Apply an evaluated track value to one HSF texture-attribute channel.
 *
 * @details Animated transform work is allocated lazily from the model heap.
 * @param arg0 Model ID containing the texture attribute.
 * @param arg1 Track whose attribute index and channel select the destination.
 * @param arg2 Evaluated channel value.
 */
void SetObjAttrMotion(s16 arg0, HSFTRACK *arg1, float arg2) {
    HU3DMODEL *model;
    HSFDATA *hsf;
    HSFATTRIBUTE *attribute;
    HU3DATTRANIM *anim_work;
    float clamped_value;

    model = &Hu3DData[arg0];
    hsf = model->hsf;
    attribute = &hsf->attribute[arg1->attrIdx];
    clamped_value = arg2;
    if (arg2 > 1.0f) {
        clamped_value = 1.0f;
    } else if (arg2 < 0.0f) {
        clamped_value = 0.0f;
    }
    switch (arg1->channel) {
        case 8:
        case 9:
        case 0xA:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
        case 0x20:
        case 0x21:
        case 0x43:
            if (attribute->animWorkP == 0) {
                anim_work = HuMemDirectMallocNum(HEAP_MODEL, sizeof(HU3DATTRANIM), (u32) Hu3DData[arg0].mallocNo);
                attribute->animWorkP = anim_work;
                anim_work->attr = 0;
                anim_work->trans3D.x = anim_work->trans3D.y = anim_work->trans3D.z = 0.0f;
                anim_work->rot.x = anim_work->rot.y = anim_work->rot.z = 0.0f;
                anim_work->scale3D.x = anim_work->scale3D.y = anim_work->scale3D.z = 1.0f;
            } else {
                anim_work = attribute->animWorkP;
            }
            if (arg1->channel != 0x43) {
                anim_work->attr |= 4;
            } else {
                anim_work->attr |= 8;
            }
            break;
    }
    switch (arg1->channel) {
        case 8:
            anim_work->trans3D.x = arg2;
            break;
        case 9:
            anim_work->trans3D.y = arg2;
            break;
        case 10:
            anim_work->trans3D.z = arg2;
            break;
        case 28:
            anim_work->rot.x = arg2;
            break;
        case 29:
            anim_work->rot.y = arg2;
            break;
        case 30:
            anim_work->rot.z = arg2;
            break;
        case 31:
            anim_work->scale3D.x = arg2;
            break;
        case 32:
            anim_work->scale3D.y = arg2;
            break;
        case 33:
            anim_work->scale3D.z = arg2;
            break;
        case 64:
            attribute->unk20 = clamped_value;
            break;
        case 62:
            attribute->kColor = clamped_value;
            break;
        case 63:
            attribute->nbtTpLvl = arg2;
            break;
        case 67:
            anim_work->bitMapPtr = bitMapPtr;
            break;
    }
}

/**
 * @brief Apply an evaluated camera track to every camera selected by a model.
 * @param arg0 Model ID providing transform offsets and a camera bit mask.
 * @param arg1 Track whose channel selects camera position, target, roll, or field of view.
 * @param arg2 Evaluated channel value; roll and field of view are expressed in degrees.
 */
void SetObjCameraMotion(s16 arg0, HSFTRACK *arg1, float arg2) {
    HU3DMODEL *model;
    Vec rotated_up;
    Vec view_direction;
    float clamped_value;
    float transformed_value;
    s16 camera_mask;
    s16 camera_bits;
    s16 i;

    model = &Hu3DData[arg0];
    camera_bits = model->camInfoBit;
    if (camera_bits != 0) {
        clamped_value = arg2;
        if (arg2 > 1.0f) {
            clamped_value = 1.0f;
        } else if (arg2 < 0.0f) {
            clamped_value = 0.0f;
        }
        switch (arg1->channel) {
            case 8:
                transformed_value = model->scale.x * (arg2 + model->pos.x);
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].pos.x = transformed_value;
                    }
                }
                break;
            case 9:
                transformed_value = model->scale.y * (arg2 + model->pos.y);
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].pos.y = transformed_value;
                    }
                }
                break;
            case 10:
                transformed_value = model->scale.z * (arg2 + model->pos.z);
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].pos.z = transformed_value;
                    }
                }
                break;
            case 11:
                transformed_value = model->scale.x * (arg2 + model->pos.x);
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].target.x = transformed_value;
                    }
                }
                break;
            case 12:
                transformed_value = model->scale.y * (arg2 + model->pos.y);
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].target.y = transformed_value;
                    }
                }
                break;
            case 13:
                transformed_value = model->scale.z * (arg2 + model->pos.z);
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].target.z = transformed_value;
                    }
                }
                break;
            case 14:
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        VECSubtract(&Hu3DCamera[i].pos, &Hu3DCamera[i].target, &view_direction);
                        VECNormalize(&view_direction, &view_direction);
                        rotated_up.x = view_direction.x * view_direction.y * (1.0 - cosd(arg2)) - view_direction.z * sind(arg2);
                        rotated_up.y = view_direction.y * view_direction.y + (1.0f - view_direction.y * view_direction.y) * cosd(arg2);
                        rotated_up.z = view_direction.y * view_direction.z * (1.0 - cosd(arg2)) + view_direction.x * sind(arg2);
                        VECNormalize(&rotated_up, &Hu3DCamera[i].up);
                        Hu3DCamera[i].upRot = arg2;
                    }
                }
                break;
            case 15:
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].fov = arg2;
                    }
                }
                break;
        }
    }
}

/**
 * @brief Apply an evaluated object track to a model-derived global light.
 * @param arg0 Model ID whose HSF light object is targeted.
 * @param arg1 Track selecting the light object and animated channel.
 * @param arg2 Evaluated position or target component.
 */
void SetObjLightMotion(s16 arg0, HSFTRACK *arg1, float arg2) {
    s16 light_index;
    HU3DMODEL *model;
    HSFDATA *hsf;
    HSFOBJECT *object_cursor;
    HSFOBJECT *object;
    HU3DLIGHT *light;
    float clamped_value;
    s16 i;

    model = &Hu3DData[arg0];
    hsf = model->hsf;
    object_cursor = hsf->object;
    for (i = light_index = 0; i < hsf->objectNum; i++, object_cursor++) {
        object = object_cursor;
        if (object->type == 8) {
            if (i != arg1->target) {
                light_index++;
            } else {
                break;
            }
        }
    }
    if (i != hsf->objectNum) {
        light = &Hu3DGlobalLight[model->lightId[light_index]];
        clamped_value = arg2;
        if (arg2 > 1.0f) {
            clamped_value = 1.0f;
        } else if (arg2 < 0.0f) {
            clamped_value = 0.0f;
        }
        switch (arg1->channel) {
            case 8:
                light->pos.x = arg2;
                break;
            case 9:
                light->pos.y = arg2;
                break;
            case 10:
                light->pos.z = arg2;
                break;
            case 11:
                light->offset.x = arg2;
                Hu3DGLightPosAimSetV(model->lightId[light_index], &light->pos, &light->offset);
                break;
            case 12:
                light->offset.y = arg2;
                Hu3DGLightPosAimSetV(model->lightId[light_index], &light->pos, &light->offset);
                break;
            case 13:
                light->offset.z = arg2;
                Hu3DGLightPosAimSetV(model->lightId[light_index], &light->pos, &light->offset);
                break;
        }
    }
}

/**
 * @brief Evaluate an HSF track using its encoded interpolation type.
 * @param arg0 Track containing curve type, keyframes, and cached search state.
 * @param arg1 Evaluation time in HSF timeline units.
 * @return The evaluated scalar value; bitmap tracks update the shared bitmap pointer and return 0.
 */
float GetCurve(HSFTRACK *arg0, float arg1) {
    float *value;

    switch (arg0->curveType) {
        case 1:
            return GetLinear(arg0->numKeyframes, arg0->data, arg1);
        case 2:
            return GetBezier(arg0->numKeyframes, arg0, arg1);
        case 3:
            bitMapPtr = GetBitMap(arg0->numKeyframes, arg0->data, arg1);
            break;
        case 4:
            value = &arg0->value;
            return *value;
        case 0:
            return GetConstant(arg0->numKeyframes, arg0->data, arg1);
    }
    return 0.0f;
}

/**
 * @brief Evaluate a stepped scalar keyframe sequence.
 * @param arg0 Number of keyframes.
 * @param arg1 Alternating time and value pairs.
 * @param arg2 Evaluation time in HSF timeline units.
 * @return The value from the latest keyframe not after the requested time.
 */
float GetConstant(s32 arg0, float *arg1, float arg2) {
    float *keyframe;
    s16 i;

    keyframe = arg1;
    if (arg2 == 0.0f || arg0 == 1) {
        return arg1[1];
    }
    for (i = 0; i < arg0; i++, keyframe += 2) {
        if (arg2 < keyframe[0]) {
            return keyframe[-1];
        }
    }
    return keyframe[-1];
}

/**
 * @brief Linearly interpolate a scalar keyframe sequence.
 * @param arg0 Number of keyframes.
 * @param arg1 Array of time/value keyframe pairs.
 * @param arg2 Evaluation time in HSF timeline units.
 * @return The interpolated value, or the final keyframe value after the sequence ends.
 */
float GetLinear(s32 arg0, float arg1[][2], float arg2) {
    float value;
    float time_span;
    s16 previous_index;
    s16 keyframe_index;

    if (arg2 == 0.0f || arg0 == 1) {
        return arg1[0][1];
    }
    for (keyframe_index = 0; keyframe_index < arg0; keyframe_index++) {
        if (arg2 < arg1[keyframe_index][0]) {
            previous_index = keyframe_index - 1;
            time_span = arg1[keyframe_index][0] - arg1[previous_index][0];
            value = arg1[previous_index][1] + (arg2 - arg1[previous_index][0]) * ((arg1[keyframe_index][1] - arg1[previous_index][1]) / time_span);
            return value;
        }
    }
    return arg1[arg0 - 1][1];
}

/**
 * @brief Evaluate the cubic curve segment surrounding a requested time.
 * @param arg0 Number of curve keyframes.
 * @param arg1 Track containing time, value, and tangent data plus a cached starting index.
 * @param arg2 Evaluation time in HSF timeline units.
 * @return The cubic-interpolated value, or the boundary keyframe value outside the sequence.
 */
__declspec(weak) float GetBezier(s32 arg0, HSFTRACK *arg1, float arg2) {
    float end_time;
    float start_time;
    float twice_t_squared;
    float three_t_squared;
    float t_cubed;
    float twice_t_cubed;
    float t_squared;
    float t;
    float (*next_keyframe)[4];
    float (*previous_keyframe)[4];
    s32 i;

    next_keyframe = arg1->data;
    if (arg2 == 0.0f || arg0 == 1) {
        return next_keyframe[0][1];
    }
    i = -1;
    if (arg1->start == 0 && arg2 < next_keyframe[0][0]) {
        i = 0;
    }
    if (i == -1) {
        next_keyframe = (float(*)[4]) arg1->data + arg1->start;
        previous_keyframe = next_keyframe - 1;
        for (i = arg1->start; i < arg0; i++, next_keyframe++) {
            if (arg2 >= previous_keyframe[0][0] && arg2 < next_keyframe[0][0]) {
                break;
            }
            previous_keyframe = next_keyframe;
        }
        if (i >= arg0) {
            next_keyframe = arg1->data;
            for (i = 0; i < arg0; i++, next_keyframe++) {
                if (arg2 < next_keyframe[0][0]) {
                    break;
                }
            }
        }
    }
    arg1->start = i;
    if (i == arg0) {
        return next_keyframe[-1][1];
    }
    end_time = next_keyframe[0][0];
    next_keyframe--;
    start_time = next_keyframe[0][0];
    t = (arg2 - start_time) / (end_time - start_time);
    t_squared = t * t;
    twice_t_squared = 2.0f * t_squared;
    three_t_squared = 3.0f * t_squared;
    t_cubed = t * t_squared;
    twice_t_cubed = t * twice_t_squared;
    return next_keyframe[0][1] * (twice_t_cubed - three_t_squared + 1.0f)
        + next_keyframe[1][1] * (-twice_t_cubed + three_t_squared)
        + next_keyframe[0][2] * (t_cubed - twice_t_squared + t)
        + next_keyframe[1][3] * (t_cubed - t_squared);
}

/**
 * @brief Select the bitmap keyframe active at a requested time.
 * @param arg0 Number of bitmap keyframes.
 * @param arg1 First bitmap keyframe in chronological order.
 * @param arg2 Evaluation time in frames; keyframe times are converted from seconds at 60 Hz.
 * @return The bitmap from the latest keyframe not after the requested time.
 */
HSFBITMAP *GetBitMap(s32 arg0, HSFBITMAPKEY *arg1, float arg2) {
    s16 keyframe_index;

    if (arg2 == 0.0f || arg0 == 1) {
        return arg1->data;
    }
    for (keyframe_index = 0; keyframe_index < arg0; keyframe_index++, arg1++) {
        if (arg2 < arg1->time * 60.0f) {
            break;
        }
    }
    return arg1[-1].data;
}

/**
 * @brief Load a motion and retarget its object and attribute tracks to a model.
 * @param arg0 Destination model ID.
 * @param arg1 Pointer to serialized HSF motion data.
 * @return The created motion ID, or -1 if the motion table is full.
 */
s16 Hu3DJointMotion(s16 arg0, void *arg1) {
    s16 motion_id;

    motion_id = Hu3DMotionCreate(arg1);
    JointModel_Motion(arg0, motion_id);
    return motion_id;
}

/**
 * @brief Retarget motion tracks to matching objects and attributes in a model.
 * @param arg0 Destination model ID.
 * @param arg1 Motion ID whose encoded target names are resolved.
 */
void JointModel_Motion(s16 arg0, s16 arg1) {
    HU3DMODEL *model;
    HU3DMOTION *motion;
    HSFDATA *model_hsf;
    HSFDATA *motion_hsf;
    HSFMOTION *hsf_motion;
    HSFTRACK *track;
    HSFTRACK *object_track;
    HSFTRACK *morph_track;
    HSFTRACK *attribute_track;
    s32 attribute_index;
    s32 i;

    model = &Hu3DData[arg0];
    motion = &Hu3DMotion[arg1];
    model_hsf = model->hsf;
    motion_hsf = motion->hsf;
    hsf_motion = motion_hsf->motion;
    track = hsf_motion->track;
    for (i = 0; i < hsf_motion->numTracks; i++, track++) {
        switch (track->type) {
            case 2:
                object_track = track;
                object_track->target = SearchObjectIndex(model_hsf, object_track->target);
                break;
            case 3:
                morph_track = track;
                morph_track->target = SearchObjectIndex(model_hsf, morph_track->target);
                break;
            case 10:
                attribute_track = track;
                if (attribute_track->attrIdx == -1) {
                    attribute_index = SearchAttributeIndex(model_hsf, attribute_track->cluster);
                    if (attribute_index != -1) {
                        attribute_track->attrIdx = attribute_index;
                    } else {
                        attribute_track->attrIdx = -1;
                    }
                }
                break;
        }
    }
}

/**
 * @brief Evaluate all enabled motion and deformation stages for a visible model.
 *
 * @details The routine applies base, shift, overlay, cluster, and shape motion before cluster, shape, and envelope deformation.
 * @param arg0 Model ID to evaluate.
 */
void Hu3DMotionCalc(s16 arg0) {
    HU3DMODEL *model = &Hu3DData[arg0];

    if ((model->attr & HU3D_ATTR_DISPOFF) || (model->attr & HU3D_ATTR_HOOK)) {
        return;
    }
    if (model->motId != -1) {
        Hu3DMotionExec(arg0, model->motId, model->motWork.time, 0);
    }
    if (model->motIdShift != -1) {
        Hu3DSubMotionExec(arg0);
    }
    if (model->motIdOvl != -1) {
        Hu3DMotionExec(arg0, model->motIdOvl, model->motOvlWork.time, 1);
    }
    if (model->attr & HU3D_ATTR_CLUSTER_ON) {
        ClusterMotionExec(model);
    }
    if (model->motIdShape != -1) {
        if (model->motId == -1) {
            Hu3DMotionExec(arg0, model->motIdShape, model->motShapeWork.time, 0);
        } else {
            Hu3DMotionExec(arg0, model->motIdShape, model->motShapeWork.time, 1);
        }
    }
    if (!(model->attr & (HU3D_ATTR_ENVELOPE_OFF|HU3D_ATTR_HOOKFUNC)) || !(model->motAttr & HU3D_MOTATTR_PAUSE)) {
        InitVtxParm(model->hsf);
        if (model->motIdShape != -1) {
            ShapeProc(model->hsf);
        }
        if (model->attr & HU3D_ATTR_CLUSTER_ON) {
            ClusterProc(model);
        }
        if (model->hsf->cenvNum != 0) {
            EnvelopeProc(model->hsf);
        }
        PPCSync();
    }
    model->attr |= HU3D_ATTR_MOT_EXEC;
}

/**
 * @brief Resolve an encoded HSF name to an object index.
 * @param arg0 HSF data whose object table is searched.
 * @param arg1 Packed HSF object name.
 * @return The matching object index, or -1 when no object name matches.
 */
static s32 SearchObjectIndex(HSFDATA *arg0, u32 arg1) {
    s32 i;
    char *object_name;
    HSFOBJECT *object;

    object = arg0->object;
    object_name = SetName(&arg1);
    for (i = 0; i < arg0->objectNum; i++, object++) {
        if (CmpObjectName(object->name, object_name) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Resolve an encoded HSF name to a texture-attribute index.
 * @param arg0 HSF data whose attribute table is searched.
 * @param arg1 Packed HSF attribute name.
 * @return The matching attribute index, or -1 when no attribute name matches.
 */
static s32 SearchAttributeIndex(HSFDATA *arg0, u32 arg1) {
    HSFATTRIBUTE *attribute;
    size_t name_length;
    char *attribute_name;
    s32 i;

    attribute = arg0->attribute;
    attribute_name = SetName(&arg1);
    for (i = 0; i < arg0->attributeNum; i++, attribute++) {
        if (attribute->name) {
            name_length = strlen(attribute->name);
            if (strncmp(attribute->name, attribute_name, name_length) == 0) {
                return i;
            }
        }
    }
    return -1;
}
