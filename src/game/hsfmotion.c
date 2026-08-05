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

static s32 SearchObjectIndex(HSFDATA *hsf, u32 encoded_name);
static s32 SearchAttributeIndex(HSFDATA *hsf, u32 encoded_name);

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
 * @param hsf_data Pointer to the serialized HSF data to load.
 * @return The allocated motion ID, or -1 when the motion table is full.
 */
s16 Hu3DMotionCreate(void *hsf_data) {
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
    motion->hsf = LoadHSF(hsf_data);
    motion->attr = 0;
    motion->modelId = -1;
    return i;
}

/**
 * @brief Create a motion entry that shares an existing model's HSF data.
 * @param model_id Model ID whose HSF motion data will back the new entry.
 * @return The allocated motion ID, or -1 when the motion table is full.
 */
s16 Hu3DMotionModelCreate(s16 model_id) {
    HU3DMODEL *model = &Hu3DData[model_id];
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
    motion->modelId = model_id;
    model->motIdSrc = i;
    return i;
}

/**
 * @brief Release a motion entry when no other model is actively using it.
 * @param motion_id Motion ID to release.
 * @return 1 if the motion was released; 0 if the slot was empty or another model still uses it.
 */
s32 Hu3DMotionKill(s16 motion_id) {
    HU3DMODEL *model;
    HU3DMOTION *motion;
    s16 i;

    motion = &Hu3DMotion[motion_id];
    if (motion->hsf == 0) {
        return 0;
    }
    model = Hu3DData;
    for (i = 0; i < 512; i++, model++) {
        if (model->hsf && model->motId == motion_id && motion->modelId != i) {
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
 * @param model_id Model ID to update.
 * @param motion_id Motion ID to use as the model's base motion.
 */
void Hu3DMotionSet(s16 model_id, s16 motion_id) {
    Hu3DData[model_id].motIdShift = -1;
    Hu3DData[model_id].motId = motion_id;
    Hu3DData[model_id].motWork.time = 0.0f;
    Hu3DData[model_id].motWork.start = 0.0f;
    Hu3DData[model_id].motWork.end = Hu3DMotionMaxTimeGet(model_id);
}

/**
 * @brief Assign and rewind a model's overlay motion.
 * @param model_id Model ID to update.
 * @param motion_id Motion ID to use as the overlay.
 */
void Hu3DMotionOverlaySet(s16 model_id, s16 motion_id) {
    Hu3DData[model_id].motIdOvl = motion_id;
    Hu3DData[model_id].motOvlWork.time = 0.0f;
    Hu3DData[model_id].motOvlWork.speed = 1.0f;
}

/**
 * @brief Remove a model's overlay motion.
 * @param model_id Model ID whose overlay motion is cleared.
 */
void Hu3DMotionOverlayReset(s16 model_id) {
    Hu3DData[model_id].motIdOvl = -1;
}

/**
 * @brief Get a model's current overlay-motion time.
 * @param model_id Model ID to query.
 * @return The overlay time in HSF motion timeline units.
 */
float Hu3DMotionOverlayTimeGet(s16 model_id) {
    return Hu3DData[model_id].motOvlWork.time;
}

/**
 * @brief Set a model's overlay-motion time without clamping it.
 * @param model_id Model ID to update.
 * @param time Overlay time in HSF motion timeline units.
 */
void Hu3DMotionOverlayTimeSet(s16 model_id, float time) {
    Hu3DData[model_id].motOvlWork.time = time;
}

/**
 * @brief Set a model's overlay-motion playback rate.
 * @param model_id Model ID to update.
 * @param playback_speed Timeline units advanced per frame multiplier.
 */
void Hu3DMotionOverlaySpeedSet(s16 model_id, float playback_speed) {
    Hu3DData[model_id].motOvlWork.speed = playback_speed;
}

/**
 * @brief Begin blending a model from its current motion to another motion.
 *
 * @details Any prior shift is promoted to the base motion before the new shift state is installed.
 * @param model_id Model ID to update.
 * @param motion_id Motion ID to blend toward.
 * @param start_time Initial time for the incoming motion, in HSF timeline units.
 * @param blend_duration Blend duration in frame-timing units.
 * @param requested_attrs Playback attributes requested for the incoming motion.
 */
void Hu3DMotionShiftSet(s16 model_id, s16 motion_id, float start_time, float blend_duration, u32 requested_attrs) {
    HU3DMODEL *model = &Hu3DData[model_id];
    HU3DMOTION *shift_motion = &Hu3DMotion[motion_id];
    s32 translated_attrs;

    requested_attrs &= ~HU3D_MOTATTR;
    translated_attrs = 0;
    if (model->motIdShift != -1) {
        model->motId = model->motIdShift;
        model->motWork.time = model->motShiftWork.time;
        model->motWork.speed = model->motShiftWork.speed;
        model->motWork.start = model->motShiftWork.start;
        model->motWork.end = model->motShiftWork.end;
        if (requested_attrs & HU3D_MOTATTR_SHIFT_LOOP) {
            translated_attrs |= HU3D_MOTATTR_LOOP;
        }
        if (requested_attrs & HU3D_MOTATTR_SHIFT_PAUSE) {
            translated_attrs |= HU3D_MOTATTR_PAUSE;
        }
        if (requested_attrs & HU3D_MOTATTR_SHIFT_REV) {
            translated_attrs |= HU3D_MOTATTR_REV;
        }
        model->motAttr &= ~HU3D_MOTATTR_ALL;
        model->motAttr |= translated_attrs;
        model->motAttr &= ~HU3D_MOTATTR;
    } else {
        model->motAttr &= ~HU3D_MOTATTR_SHIFT_ALL;
    }
    model->motIdShift = motion_id;
    model->motShiftWork.time = start_time;
    model->motShiftWork.speed = 1.0f;
    model->motOvlWork.start = 0.0f;
    model->motOvlWork.end = blend_duration;
    model->motShiftWork.start = 0.0f;
    model->motShiftWork.end = Hu3DMotionShiftMaxTimeGet(model_id);
    if (requested_attrs & HU3D_MOTATTR_LOOP) {
        translated_attrs |= HU3D_MOTATTR_SHIFT_LOOP;
    }
    if (requested_attrs & HU3D_MOTATTR_PAUSE) {
        translated_attrs |= HU3D_MOTATTR_SHIFT_PAUSE;
    }
    if (requested_attrs & HU3D_MOTATTR_REV) {
        translated_attrs |= HU3D_MOTATTR_SHIFT_REV;
    }
    requested_attrs &= ~HU3D_MOTATTR_NOSHIFT_ALL;
    model->motAttr |= translated_attrs | requested_attrs;
    model->motAttr &= ~HU3D_MOTATTR;
}

/**
 * @brief Assign and initialize a model's shape motion.
 * @param model_id Model ID to update.
 * @param motion_id Motion ID containing the shape tracks.
 */
void Hu3DMotionShapeSet(s16 model_id, s16 motion_id) {
    Hu3DData[model_id].motIdShape = motion_id;
    Hu3DData[model_id].motShapeWork.time = 0.0f;
    Hu3DData[model_id].motShapeWork.speed = 1.0f;
    Hu3DData[model_id].motShapeWork.start = 0.0f;
    Hu3DData[model_id].motShapeWork.end = Hu3DMotionShapeMaxTimeGet(model_id);
}

/**
 * @brief Get the shape-motion ID assigned to a model.
 * @param model_id Model ID to query.
 * @return The shape-motion ID, or -1 when none is assigned.
 */
s16 Hu3DMotionShapeIDGet(s16 model_id) {
    return Hu3DData[model_id].motIdShape;
}

/**
 * @brief Set a model's shape-motion playback rate.
 * @param model_id Model ID to update.
 * @param playback_speed Timeline units advanced per frame multiplier.
 */
void Hu3DMotionShapeSpeedSet(s16 model_id, float playback_speed) {
    HU3DMODEL *model = &Hu3DData[model_id];

    model->motShapeWork.speed = playback_speed;
}

/**
 * @brief Set a model's current shape-motion time.
 * @param model_id Model ID to update.
 * @param time Shape-motion time in HSF timeline units.
 */
void Hu3DMotionShapeTimeSet(s16 model_id, float time) {
    Hu3DData[model_id].motShapeWork.time = time;
}

/**
 * @brief Get the maximum time of a model's assigned shape motion.
 * @param model_id Model ID to query.
 * @return The shape motion's maximum HSF timeline time, or 0 if its motion entry is empty.
 */
float Hu3DMotionShapeMaxTimeGet(s16 model_id) {
    HU3DMODEL *model = &Hu3DData[model_id];

    return Hu3DMotionMotionMaxTimeGet(model->motIdShape);
}

/**
 * @brief Set the playback range for a model's shape motion.
 * @param model_id Model ID to update.
 * @param start_time Inclusive playback start time in HSF timeline units.
 * @param end_time Playback end time in HSF timeline units.
 */
void Hu3DMotionShapeStartEndSet(s16 model_id, float start_time, float end_time) {
    HU3DMODEL *model = &Hu3DData[model_id];

    model->motShapeWork.start = start_time;
    model->motShapeWork.end = end_time;
}

/**
 * @brief Attach a cluster motion to the first free cluster slot on a model.
 * @param model_id Model ID to update.
 * @param motion_id Motion ID containing cluster tracks.
 * @return The assigned cluster slot from 0 through 3, or -1 when all slots are occupied.
 */
s16 Hu3DMotionClusterSet(s16 model_id, s16 motion_id) {
    HU3DMODEL *model = &Hu3DData[model_id];
    s16 i;

    for (i = 0; i < 4; i++) {
        if (model->motIdCluster[i] == -1) {
            model->motIdCluster[i] = motion_id;
            model->clusterTime[i] = 0.0f;
            model->clusterSpeed[i] = 1.0f;
            model->clusterAttr[i] = HU3D_ATTR_NONE;
            model->attr |= HU3D_ATTR_CLUSTER_ON;
            ClusterAdjustObject(model->hsf, Hu3DMotion[motion_id].hsf);
            return i;
        }
    }
    OSReport("Error: Cluster Entry Over\n");
    return -1;
}

/**
 * @brief Replace a specified cluster-motion slot on a model.
 * @param model_id Model ID to update.
 * @param motion_id Motion ID containing cluster tracks.
 * @param cluster_slot Cluster slot from 0 through 3.
 * @return The supplied cluster slot.
 */
s16 Hu3DMotionClusterNoSet(s16 model_id, s16 motion_id, s16 cluster_slot) {
    HU3DMODEL *model = &Hu3DData[model_id];

    Hu3DMotionClusterReset(model_id, cluster_slot);
    model->motIdCluster[cluster_slot] = motion_id;
    model->clusterTime[cluster_slot] = 0.0f;
    model->clusterSpeed[cluster_slot] = 1.0f;
    model->attr |= HU3D_ATTR_CLUSTER_ON;
    ClusterAdjustObject(model->hsf, Hu3DMotion[motion_id].hsf);
    return cluster_slot;
}

/**
 * @brief Remove a model's shape motion.
 * @param model_id Model ID whose shape motion is cleared.
 */
void Hu3DMotionShapeReset(s16 model_id) {
    Hu3DData[model_id].motIdShape = -1;
}

/**
 * @brief Remove one or all cluster motions from a model.
 * @param model_id Model ID to update.
 * @param cluster_slot Cluster slot to clear, or -1 to clear all four slots.
 */
void Hu3DMotionClusterReset(s16 model_id, s16 cluster_slot) {
    HU3DMODEL *model = &Hu3DData[model_id];
    s16 i;

    if (cluster_slot == -1) {
        for (i = 0; i < 4; i++) {
            model->motIdCluster[i] = -1;
        }
        model->attr &= ~HU3D_ATTR_CLUSTER_ON;
    } else {
        model->motIdCluster[cluster_slot] = -1;
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
 * @param model_id Model ID to query.
 * @return The base-motion ID, or -1 when none is assigned.
 */
s16 Hu3DMotionIDGet(s16 model_id) {
    HU3DMODEL *model = &Hu3DData[model_id];

    return model->motId;
}

/**
 * @brief Get a model's incoming shift-motion ID.
 * @param model_id Model ID to query.
 * @return The shift-motion ID, or -1 when no blend is active.
 */
s16 Hu3DMotionShiftIDGet(s16 model_id) {
    HU3DMODEL *model = &Hu3DData[model_id];

    return model->motIdShift;
}

/**
 * @brief Set and clamp a model's base-motion time.
 *
 * @details Paused skinned models are evaluated immediately so their envelope vertices reflect the new time.
 * @param model_id Model ID to update.
 * @param time Requested time in HSF timeline units; clamped to zero through the motion maximum.
 */
void Hu3DMotionTimeSet(s16 model_id, float time) {
    HU3DMODEL *model = &Hu3DData[model_id];

    if (Hu3DMotionMaxTimeGet(model_id) <= time) {
        time = Hu3DMotionMaxTimeGet(model_id);
    }
    if (time < 0.0f) {
        time = 0.0f;
    }
    model->motWork.time = time;
    if (model->hsf != (HSFDATA*) -1 && model->hsf->cenvNum != 0 && (model->motAttr & HU3D_MOTATTR_PAUSE)) {
        Hu3DMotionExec(model_id, model->motId, time, 0);
        if (model->motIdShift != -1) {
            Hu3DSubMotionExec(model_id);
        }
        EnvelopeProc(model->hsf);
    }
}

/**
 * @brief Get a model's current base-motion time.
 * @param model_id Model ID to query.
 * @return The base-motion time in HSF timeline units.
 */
float Hu3DMotionTimeGet(s16 model_id) {
    HU3DMODEL *model = &Hu3DData[model_id];

    return model->motWork.time;
}

/**
 * @brief Get a model's current shift-motion time.
 * @param model_id Model ID to query.
 * @return The incoming motion time in HSF timeline units.
 */
float Hu3DMotionShiftTimeGet(s16 model_id) {
    HU3DMODEL *model = &Hu3DData[model_id];

    return model->motShiftWork.time;
}

/**
 * @brief Get the maximum time of a model's base motion.
 * @param model_id Model ID to query.
 * @return The base motion's maximum HSF time after conversion through the routine's signed 16-bit temporary, or 0 if no motion is assigned.
 */
float Hu3DMotionMaxTimeGet(s16 model_id) {
    HU3DMODEL *model = &Hu3DData[model_id];
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
 * @param model_id Model ID to query.
 * @return The shift motion's maximum HSF time after conversion through the routine's signed 16-bit temporary, or 0 if no shift is assigned.
 */
float Hu3DMotionShiftMaxTimeGet(s16 model_id) {
    HU3DMODEL *model = &Hu3DData[model_id];
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
 * @param model_id Model ID to update.
 * @param start_time Inclusive playback start time in HSF timeline units.
 * @param end_time Playback end time in HSF timeline units.
 */
void Hu3DMotionShiftStartEndSet(s16 model_id, float start_time, float end_time) {
    HU3DMODEL *model = &Hu3DData[model_id];

    model->motShiftWork.start = start_time;
    model->motShiftWork.end = end_time;
}

/**
 * @brief Get the maximum time stored in a motion-table entry.
 * @param motion_id Motion ID to query.
 * @return The HSF maximum time after conversion through the routine's signed 16-bit temporary, or 0 if the slot is empty.
 */
float Hu3DMotionMotionMaxTimeGet(s16 motion_id) {
    HU3DMOTION *motion = &Hu3DMotion[motion_id];
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
 * @param model_id Model ID to update.
 * @param start_time Inclusive playback start time in HSF timeline units.
 * @param end_time Playback end time in HSF timeline units.
 */
void Hu3DMotionStartEndSet(s16 model_id, float start_time, float end_time) {
    HU3DMODEL *model = &Hu3DData[model_id];

    model->motWork.start = start_time;
    model->motWork.end = end_time;
}

/**
 * @brief Check whether a model's base motion has reached its playback boundary.
 * @param model_id Model ID to query.
 * @return Nonzero at the maximum time during forward playback or at zero during reverse playback; otherwise zero.
 */
s32 Hu3DMotionEndCheck(s16 model_id) {
    if (!(Hu3DData[model_id].motAttr & HU3D_MOTATTR_REV)) {
        return (Hu3DMotionMaxTimeGet(model_id) <= Hu3DMotionTimeGet(model_id));
    } else {
        return (Hu3DMotionTimeGet(model_id) <= 0.0f);
    }
}

/**
 * @brief Set a model's base-motion playback rate.
 * @param model_id Model ID to update.
 * @param playback_speed Timeline units advanced per frame multiplier.
 */
void Hu3DMotionSpeedSet(s16 model_id, float playback_speed) {
    HU3DMODEL *model = &Hu3DData[model_id];

    model->motWork.speed = playback_speed;
}

/**
 * @brief Set a model's shift-motion playback rate.
 * @param model_id Model ID to update.
 * @param playback_speed Timeline units advanced per frame multiplier.
 */
void Hu3DMotionShiftSpeedSet(s16 model_id, float playback_speed) {
    HU3DMODEL *model = &Hu3DData[model_id];

    model->motShiftWork.speed = playback_speed;
}

/**
 * @brief Prevent selected position or rotation channels from being changed by motion.
 *
 * @details Selected channels are restored to their base transform when the mask is enabled.
 * @param model_id Model ID containing the object.
 * @param object_name Name of the HSF object to protect.
 * @param channel_mask Bit mask selecting position and rotation channels to preserve.
 */
void Hu3DMotionNoMotSet(s16 model_id, char *object_name, u32 channel_mask) {
    HSFCONSTDATA *constant_data;
    HSFOBJECT *object;

    object = Hu3DModelObjPtrGet(model_id, object_name);
    if (object->constData == 0) {
        constant_data = ObjConstantMake(object, (u32) Hu3DData[model_id].mallocNo);
    } else {
        constant_data = object->constData;
    }
    constant_data->attr |= channel_mask;
    if (channel_mask & 0x10) {
        object->mesh.curr.pos.x = object->mesh.base.pos.x;
    }
    if (channel_mask & 0x20) {
        object->mesh.curr.pos.y = object->mesh.base.pos.y;
    }
    if (channel_mask & 0x40) {
        object->mesh.curr.pos.z = object->mesh.base.pos.z;
    }
    if (channel_mask & 0x80) {
        object->mesh.curr.rot.x = object->mesh.base.rot.x;
    }
    if (channel_mask & 0x100) {
        object->mesh.curr.rot.y = object->mesh.base.rot.y;
    }
    if (channel_mask & 0x200) {
        object->mesh.curr.rot.z = object->mesh.base.rot.z;
    }
}

/**
 * @brief Allow motion to update selected object transform channels again.
 * @param model_id Model ID containing the object.
 * @param object_name Name of the HSF object to update.
 * @param channel_mask Bit mask of position and rotation channel locks to clear.
 */
void Hu3DMotionNoMotReset(s16 model_id, char *object_name, u32 channel_mask) {
    HSFOBJECT *object;
    HSFCONSTDATA *constant_data;

    object = Hu3DModelObjPtrGet(model_id, object_name);
    constant_data = object->constData;
    constant_data->attr &= ~channel_mask;
}

/**
 * @brief Lock selected object transform channels to a supplied value.
 * @param model_id Model ID containing the object.
 * @param object_name Name of the HSF object to update.
 * @param channel_mask Bit mask selecting position and rotation channels to lock.
 * @param forced_value Value written to every selected transform channel.
 */
void Hu3DMotionForceSet(s16 model_id, char *object_name, u32 channel_mask, float forced_value) {
    HSFCONSTDATA *constant_data;
    HSFOBJECT *object;

    object = Hu3DModelObjPtrGet(model_id, object_name);
    if (object->constData == 0) {
        constant_data = ObjConstantMake(object, (u32) Hu3DData[model_id].mallocNo);
    } else {
        constant_data = object->constData;
    }
    constant_data->attr |= channel_mask;
    if (channel_mask & 0x10) {
        object->mesh.curr.pos.x = forced_value;
    }
    if (channel_mask & 0x20) {
        object->mesh.curr.pos.y = forced_value;
    }
    if (channel_mask & 0x40) {
        object->mesh.curr.pos.z = forced_value;
    }
    if (channel_mask & 0x80) {
        object->mesh.curr.rot.x = forced_value;
    }
    if (channel_mask & 0x100) {
        object->mesh.curr.rot.y = forced_value;
    }
    if (channel_mask & 0x200) {
        object->mesh.curr.rot.z = forced_value;
    }
}

/**
 * @brief Advance all motion timelines attached to a model by one frame step.
 *
 * @details Playback attributes control pausing, reversal, looping, clamping, and completion of shift blends.
 * @param model_id Model ID whose base, overlay, shift, shape, and cluster timelines advance.
 */
void Hu3DMotionNext(s16 model_id) {
    HU3DMODEL *model = &Hu3DData[model_id];
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
 * @param model_id Destination model ID.
 * @param motion_id Motion ID whose tracks are evaluated.
 * @param sample_time Evaluation time in HSF timeline units.
 * @param overlay Zero to reset unprotected object transforms to their base values first; nonzero for overlay evaluation.
 */
void Hu3DMotionExec(s16 model_id, s16 motion_id, float sample_time, s32 overlay) {
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

    model = &Hu3DData[model_id];
    motion = &Hu3DMotion[motion_id];
    model_hsf = model->hsf;
    motion_hsf = motion->hsf;
    hsf_motion = motion_hsf->motion;
    track = hsf_motion->track;
    object_cursor = model_hsf->object;
    if (overlay == 0) {
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
                        object->mesh.mesh.baseMorph = GetCurve(track, sample_time);
                    } else if (object->type == 7) {
                        if (model->attr & HU3D_ATTR_CAMERA_MOTON) {
                            SetObjCameraMotion(model_id, track, GetCurve(track, sample_time));
                        }
                    } else if (object->type == 8) {
                        SetObjLightMotion(model_id, track, GetCurve(track, sample_time));
                    } else if (track->channel == 0x18) {
                        if (object->constData) {
                            constant_data = object->constData;
                            if (GetCurve(track, sample_time) == 1.0f) {
                                constant_data->attr &= ~0x1000;
                            } else {
                                constant_data->attr |= 0x1000;
                            }
                        }
                    } else if (track->channel == 0x1A) {
                        if (object->constData) {
                            constant_data = object->constData;
                            if (GetCurve(track, sample_time) == 1.0f) {
                                constant_data->attr &= ~0x2000;
                            } else {
                                constant_data->attr |= 0x2000;
                            }
                        }
                    } else {
                        transform_value = GetObjTRXPtr(object, track->channel);
                        if (transform_value != (float*) -1) {
                            *transform_value = GetCurve(track, sample_time);
                        }
                    }
                }
                break;
            case 3:
                morph_track = track;
                if (morph_track->target < model_hsf->objectNum) {
                    object = &model_hsf->object[morph_track->target];
                    object->mesh.mesh.morphWeight[morph_track->morphWeight] = GetCurve(morph_track, sample_time);
                }
                break;
            case 9:
                if (!(model->attr & HU3D_ATTR_CURVE_MOTOFF)) {
                    if (track->attrIdx < model_hsf->materialNum) {
                        SetObjMatMotion(model_id, track, GetCurve(track, sample_time));
                    }
                }
                break;
            case 5:
                if (!(model->attr & HU3D_ATTR_CURVE_MOTOFF)) {
                    cluster = &model_hsf->cluster[track->cluster];
                    cluster->index = GetClusterCurve(track, sample_time);
                }
                break;
            case 6:
                if (!(model->attr & HU3D_ATTR_CURVE_MOTOFF)) {
                    cluster_track = track;
                    cluster = &model_hsf->cluster[cluster_track->cluster];
                    cluster->weight[cluster_track->clusterWeight] = GetClusterWeightCurve(cluster_track, sample_time);
                }
                break;
            case 10:
                attribute_track = track;
                if (attribute_track->cluster != -1 || !(model->attr & HU3D_ATTR_CURVE_MOTOFF)) {
                    if (attribute_track->attrIdx != -1 && attribute_track->attrIdx < model_hsf->attributeNum) {
                        SetObjAttrMotion(model_id, attribute_track, GetCurve(attribute_track, sample_time));
                    }
                }
                break;
        }
    }
}

/**
 * @brief Evaluate camera tracks from a model's active base motion.
 * @param model_id Model ID providing the camera motion and camera-selection mask.
 */
void Hu3DCameraMotionExec(s16 model_id) {
    HU3DMODEL *model;
    HU3DMOTION *motion;
    HSFDATA *hsf;
    HSFMOTION *hsf_motion;
    HSFTRACK *track;
    HSFTRACK *track_end;

    model = &Hu3DData[model_id];
    motion = &Hu3DMotion[model->motId];
    hsf = motion->hsf;
    hsf_motion = hsf->motion;
    track = hsf_motion->track;
    if (model->attr & HU3D_ATTR_CAMERA_MOTON) {
        track_end = &track[hsf_motion->numTracks];
        for (; track < track_end; track++) {
            if (track->type == 2 && track->index == 7) {
                SetObjCameraMotion(model_id, track, GetCurve(track, model->motWork.time));
            }
        }
    }
}

/**
 * @brief Blend transform tracks from a model's incoming shift motion.
 *
 * @details Rotation channels take the shortest wrapped path across zero degrees before interpolation.
 * @param model_id Model ID with an active shift blend.
 */
void Hu3DSubMotionExec(s16 model_id) {
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

    model = &Hu3DData[model_id];
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
 * @param object Object whose current transform is queried.
 * @param channel HSF channel number selecting position, rotation, or scale.
 * @return A pointer to the selected component, or `(float *)-1` for an unsupported or motion-locked channel.
 */
__declspec(weak) float *GetObjTRXPtr(HSFOBJECT *object, u16 channel) {
    HSFCONSTDATA *constant_data = object->constData;

    switch (channel) {
        case 8:
            if (constant_data && (constant_data->attr & 0x10)) {
                return (float*) -1;
            }
            return &object->mesh.curr.pos.x;
        case 9:
            if (constant_data && (constant_data->attr & 0x20)) {
                return (float*) -1;
            }
            return &object->mesh.curr.pos.y;
        case 10:
            if (constant_data && (constant_data->attr & 0x40)) {
                return (float*) -1;
            }
            return &object->mesh.curr.pos.z;
        case 28:
            if (constant_data && (constant_data->attr & 0x80)) {
                return (float*) -1;
            }
            return &object->mesh.curr.rot.x;
        case 29:
            if (constant_data && (constant_data->attr & 0x100)) {
                return (float*) -1;
            }
            return &object->mesh.curr.rot.y;
        case 30:
            if (constant_data && (constant_data->attr & 0x200)) {
                return (float*) -1;
            }
            return &object->mesh.curr.rot.z;
        case 31:
            return &object->mesh.curr.scale.x;
        case 32:
            return &object->mesh.curr.scale.y;
        case 33:
            return &object->mesh.curr.scale.z;
        default:
            return (float*) -1;
    }
}

/**
 * @brief Apply an evaluated track value to one HSF material channel.
 * @param model_id Model ID containing the material.
 * @param track Track whose attribute index and channel select the destination.
 * @param value Evaluated value; color and alpha channels are clamped to 0 through 1.
 */
void SetObjMatMotion(s16 model_id, HSFTRACK *track, float value) {
    HSFMATERIAL *material;
    HSFDATA *hsf;
    HU3DMODEL *model;
    float clamped_value;

    model = &Hu3DData[model_id];
    hsf = model->hsf;
    material = &hsf->material[track->attrIdx];
    clamped_value = value;
    if (value > 1.0f) {
        clamped_value = 1.0f;
    } else if (value < 0.0f) {
        clamped_value = 0.0f;
    }
    switch (track->channel) {
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
 * @param model_id Model ID containing the texture attribute.
 * @param track Track whose attribute index and channel select the destination.
 * @param value Evaluated channel value.
 */
void SetObjAttrMotion(s16 model_id, HSFTRACK *track, float value) {
    HU3DMODEL *model;
    HSFDATA *hsf;
    HSFATTRIBUTE *attribute;
    HU3DATTRANIM *anim_work;
    float clamped_value;

    model = &Hu3DData[model_id];
    hsf = model->hsf;
    attribute = &hsf->attribute[track->attrIdx];
    clamped_value = value;
    if (value > 1.0f) {
        clamped_value = 1.0f;
    } else if (value < 0.0f) {
        clamped_value = 0.0f;
    }
    switch (track->channel) {
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
                anim_work = HuMemDirectMallocNum(HEAP_MODEL, sizeof(HU3DATTRANIM), (u32) Hu3DData[model_id].mallocNo);
                attribute->animWorkP = anim_work;
                anim_work->attr = 0;
                anim_work->trans3D.x = anim_work->trans3D.y = anim_work->trans3D.z = 0.0f;
                anim_work->rot.x = anim_work->rot.y = anim_work->rot.z = 0.0f;
                anim_work->scale3D.x = anim_work->scale3D.y = anim_work->scale3D.z = 1.0f;
            } else {
                anim_work = attribute->animWorkP;
            }
            if (track->channel != 0x43) {
                anim_work->attr |= 4;
            } else {
                anim_work->attr |= 8;
            }
            break;
    }
    switch (track->channel) {
        case 8:
            anim_work->trans3D.x = value;
            break;
        case 9:
            anim_work->trans3D.y = value;
            break;
        case 10:
            anim_work->trans3D.z = value;
            break;
        case 28:
            anim_work->rot.x = value;
            break;
        case 29:
            anim_work->rot.y = value;
            break;
        case 30:
            anim_work->rot.z = value;
            break;
        case 31:
            anim_work->scale3D.x = value;
            break;
        case 32:
            anim_work->scale3D.y = value;
            break;
        case 33:
            anim_work->scale3D.z = value;
            break;
        case 64:
            attribute->unk20 = clamped_value;
            break;
        case 62:
            attribute->kColor = clamped_value;
            break;
        case 63:
            attribute->nbtTpLvl = value;
            break;
        case 67:
            anim_work->bitMapPtr = bitMapPtr;
            break;
    }
}

/**
 * @brief Apply an evaluated camera track to every camera selected by a model.
 * @param model_id Model ID providing transform offsets and a camera bit mask.
 * @param track Track whose channel selects camera position, target, roll, or field of view.
 * @param value Evaluated channel value; roll and field of view are expressed in degrees.
 */
void SetObjCameraMotion(s16 model_id, HSFTRACK *track, float value) {
    HU3DMODEL *model;
    Vec rotated_up;
    Vec view_direction;
    float clamped_value;
    float transformed_value;
    s16 camera_mask;
    s16 camera_bits;
    s16 i;

    model = &Hu3DData[model_id];
    camera_bits = model->camInfoBit;
    if (camera_bits != 0) {
        clamped_value = value;
        if (value > 1.0f) {
            clamped_value = 1.0f;
        } else if (value < 0.0f) {
            clamped_value = 0.0f;
        }
        switch (track->channel) {
            case 8:
                transformed_value = model->scale.x * (value + model->pos.x);
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].pos.x = transformed_value;
                    }
                }
                break;
            case 9:
                transformed_value = model->scale.y * (value + model->pos.y);
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].pos.y = transformed_value;
                    }
                }
                break;
            case 10:
                transformed_value = model->scale.z * (value + model->pos.z);
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].pos.z = transformed_value;
                    }
                }
                break;
            case 11:
                transformed_value = model->scale.x * (value + model->pos.x);
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].target.x = transformed_value;
                    }
                }
                break;
            case 12:
                transformed_value = model->scale.y * (value + model->pos.y);
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].target.y = transformed_value;
                    }
                }
                break;
            case 13:
                transformed_value = model->scale.z * (value + model->pos.z);
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
                        rotated_up.x = view_direction.x * view_direction.y * (1.0 - cosd(value)) - view_direction.z * sind(value);
                        rotated_up.y = view_direction.y * view_direction.y + (1.0f - view_direction.y * view_direction.y) * cosd(value);
                        rotated_up.z = view_direction.y * view_direction.z * (1.0 - cosd(value)) + view_direction.x * sind(value);
                        VECNormalize(&rotated_up, &Hu3DCamera[i].up);
                        Hu3DCamera[i].upRot = value;
                    }
                }
                break;
            case 15:
                for (i = 0, camera_mask = 1; i < HU3D_CAM_MAX; i++, camera_mask <<= 1) {
                    if (camera_mask & camera_bits) {
                        Hu3DCamera[i].fov = value;
                    }
                }
                break;
        }
    }
}

/**
 * @brief Apply an evaluated object track to a model-derived global light.
 * @param model_id Model ID whose HSF light object is targeted.
 * @param track Track selecting the light object and animated channel.
 * @param value Evaluated position or target component.
 */
void SetObjLightMotion(s16 model_id, HSFTRACK *track, float value) {
    s16 light_index;
    HU3DMODEL *model;
    HSFDATA *hsf;
    HSFOBJECT *object_cursor;
    HSFOBJECT *object;
    HU3DLIGHT *light;
    float clamped_value;
    s16 i;

    model = &Hu3DData[model_id];
    hsf = model->hsf;
    object_cursor = hsf->object;
    for (i = light_index = 0; i < hsf->objectNum; i++, object_cursor++) {
        object = object_cursor;
        if (object->type == 8) {
            if (i != track->target) {
                light_index++;
            } else {
                break;
            }
        }
    }
    if (i != hsf->objectNum) {
        light = &Hu3DGlobalLight[model->lightId[light_index]];
        clamped_value = value;
        if (value > 1.0f) {
            clamped_value = 1.0f;
        } else if (value < 0.0f) {
            clamped_value = 0.0f;
        }
        switch (track->channel) {
            case 8:
                light->pos.x = value;
                break;
            case 9:
                light->pos.y = value;
                break;
            case 10:
                light->pos.z = value;
                break;
            case 11:
                light->offset.x = value;
                Hu3DGLightPosAimSetV(model->lightId[light_index], &light->pos, &light->offset);
                break;
            case 12:
                light->offset.y = value;
                Hu3DGLightPosAimSetV(model->lightId[light_index], &light->pos, &light->offset);
                break;
            case 13:
                light->offset.z = value;
                Hu3DGLightPosAimSetV(model->lightId[light_index], &light->pos, &light->offset);
                break;
        }
    }
}

/**
 * @brief Evaluate an HSF track using its encoded interpolation type.
 * @param track Track containing curve type, keyframes, and cached search state.
 * @param sample_time Evaluation time in HSF timeline units.
 * @return The evaluated scalar value; bitmap tracks update the shared bitmap pointer and return 0.
 */
float GetCurve(HSFTRACK *track, float sample_time) {
    float *value;

    switch (track->curveType) {
        case 1:
            return GetLinear(track->numKeyframes, track->data, sample_time);
        case 2:
            return GetBezier(track->numKeyframes, track, sample_time);
        case 3:
            bitMapPtr = GetBitMap(track->numKeyframes, track->data, sample_time);
            break;
        case 4:
            value = &track->value;
            return *value;
        case 0:
            return GetConstant(track->numKeyframes, track->data, sample_time);
    }
    return 0.0f;
}

/**
 * @brief Evaluate a stepped scalar keyframe sequence.
 * @param keyframe_count Number of keyframes.
 * @param keyframes Alternating time and value pairs.
 * @param sample_time Evaluation time in HSF timeline units.
 * @return The value from the latest keyframe not after the requested time.
 */
float GetConstant(s32 keyframe_count, float *keyframes, float sample_time) {
    float *keyframe;
    s16 i;

    keyframe = keyframes;
    if (sample_time == 0.0f || keyframe_count == 1) {
        return keyframes[1];
    }
    for (i = 0; i < keyframe_count; i++, keyframe += 2) {
        if (sample_time < keyframe[0]) {
            return keyframe[-1];
        }
    }
    return keyframe[-1];
}

/**
 * @brief Linearly interpolate a scalar keyframe sequence.
 * @param keyframe_count Number of keyframes.
 * @param keyframes Array of time/value keyframe pairs.
 * @param sample_time Evaluation time in HSF timeline units.
 * @return The interpolated value, or the final keyframe value after the sequence ends.
 */
float GetLinear(s32 keyframe_count, float keyframes[][2], float sample_time) {
    float value;
    float time_span;
    s16 previous_index;
    s16 keyframe_index;

    if (sample_time == 0.0f || keyframe_count == 1) {
        return keyframes[0][1];
    }
    for (keyframe_index = 0; keyframe_index < keyframe_count; keyframe_index++) {
        if (sample_time < keyframes[keyframe_index][0]) {
            previous_index = keyframe_index - 1;
            time_span = keyframes[keyframe_index][0] - keyframes[previous_index][0];
            value = keyframes[previous_index][1] + (sample_time - keyframes[previous_index][0]) * ((keyframes[keyframe_index][1] - keyframes[previous_index][1]) / time_span);
            return value;
        }
    }
    return keyframes[keyframe_count - 1][1];
}

/**
 * @brief Evaluate the cubic curve segment surrounding a requested time.
 * @param keyframe_count Number of curve keyframes.
 * @param track Track containing time, value, and tangent data plus a cached starting index.
 * @param sample_time Evaluation time in HSF timeline units.
 * @return The cubic-interpolated value, or the boundary keyframe value outside the sequence.
 */
__declspec(weak) float GetBezier(s32 keyframe_count, HSFTRACK *track, float sample_time) {
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

    next_keyframe = track->data;
    if (sample_time == 0.0f || keyframe_count == 1) {
        return next_keyframe[0][1];
    }
    i = -1;
    if (track->start == 0 && sample_time < next_keyframe[0][0]) {
        i = 0;
    }
    if (i == -1) {
        next_keyframe = (float(*)[4]) track->data + track->start;
        previous_keyframe = next_keyframe - 1;
        for (i = track->start; i < keyframe_count; i++, next_keyframe++) {
            if (sample_time >= previous_keyframe[0][0] && sample_time < next_keyframe[0][0]) {
                break;
            }
            previous_keyframe = next_keyframe;
        }
        if (i >= keyframe_count) {
            next_keyframe = track->data;
            for (i = 0; i < keyframe_count; i++, next_keyframe++) {
                if (sample_time < next_keyframe[0][0]) {
                    break;
                }
            }
        }
    }
    track->start = i;
    if (i == keyframe_count) {
        return next_keyframe[-1][1];
    }
    end_time = next_keyframe[0][0];
    next_keyframe--;
    start_time = next_keyframe[0][0];
    t = (sample_time - start_time) / (end_time - start_time);
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
 * @param keyframe_count Number of bitmap keyframes.
 * @param keyframe First bitmap keyframe in chronological order.
 * @param sample_time Evaluation time in frames; keyframe times are converted from seconds at 60 Hz.
 * @return The bitmap from the latest keyframe not after the requested time.
 */
HSFBITMAP *GetBitMap(s32 keyframe_count, HSFBITMAPKEY *keyframe, float sample_time) {
    s16 keyframe_index;

    if (sample_time == 0.0f || keyframe_count == 1) {
        return keyframe->data;
    }
    for (keyframe_index = 0; keyframe_index < keyframe_count; keyframe_index++, keyframe++) {
        if (sample_time < keyframe->time * 60.0f) {
            break;
        }
    }
    return keyframe[-1].data;
}

/**
 * @brief Load a motion and retarget its object and attribute tracks to a model.
 * @param model_id Destination model ID.
 * @param hsf_data Pointer to serialized HSF motion data.
 * @return The created motion ID, or -1 if the motion table is full.
 */
s16 Hu3DJointMotion(s16 model_id, void *hsf_data) {
    s16 motion_id;

    motion_id = Hu3DMotionCreate(hsf_data);
    JointModel_Motion(model_id, motion_id);
    return motion_id;
}

/**
 * @brief Retarget motion tracks to matching objects and attributes in a model.
 * @param model_id Destination model ID.
 * @param motion_id Motion ID whose encoded target names are resolved.
 */
void JointModel_Motion(s16 model_id, s16 motion_id) {
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

    model = &Hu3DData[model_id];
    motion = &Hu3DMotion[motion_id];
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
 * @param model_id Model ID to evaluate.
 */
void Hu3DMotionCalc(s16 model_id) {
    HU3DMODEL *model = &Hu3DData[model_id];

    if ((model->attr & HU3D_ATTR_DISPOFF) || (model->attr & HU3D_ATTR_HOOK)) {
        return;
    }
    if (model->motId != -1) {
        Hu3DMotionExec(model_id, model->motId, model->motWork.time, 0);
    }
    if (model->motIdShift != -1) {
        Hu3DSubMotionExec(model_id);
    }
    if (model->motIdOvl != -1) {
        Hu3DMotionExec(model_id, model->motIdOvl, model->motOvlWork.time, 1);
    }
    if (model->attr & HU3D_ATTR_CLUSTER_ON) {
        ClusterMotionExec(model);
    }
    if (model->motIdShape != -1) {
        if (model->motId == -1) {
            Hu3DMotionExec(model_id, model->motIdShape, model->motShapeWork.time, 0);
        } else {
            Hu3DMotionExec(model_id, model->motIdShape, model->motShapeWork.time, 1);
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
 * @param hsf HSF data whose object table is searched.
 * @param encoded_name Packed HSF object name.
 * @return The matching object index, or -1 when no object name matches.
 */
static s32 SearchObjectIndex(HSFDATA *hsf, u32 encoded_name) {
    s32 i;
    char *object_name;
    HSFOBJECT *object;

    object = hsf->object;
    object_name = SetName(&encoded_name);
    for (i = 0; i < hsf->objectNum; i++, object++) {
        if (CmpObjectName(object->name, object_name) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Resolve an encoded HSF name to a texture-attribute index.
 * @param hsf HSF data whose attribute table is searched.
 * @param encoded_name Packed HSF attribute name.
 * @return The matching attribute index, or -1 when no attribute name matches.
 */
static s32 SearchAttributeIndex(HSFDATA *hsf, u32 encoded_name) {
    HSFATTRIBUTE *attribute;
    size_t name_length;
    char *attribute_name;
    s32 i;

    attribute = hsf->attribute;
    attribute_name = SetName(&encoded_name);
    for (i = 0; i < hsf->attributeNum; i++, attribute++) {
        if (attribute->name) {
            name_length = strlen(attribute->name);
            if (strncmp(attribute->name, attribute_name, name_length) == 0) {
                return i;
            }
        }
    }
    return -1;
}
