#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/disp.h"
#include "game/process.h"

#include "ext_math.h"

#define DISP_HALF_W (HU_DISP_WIDTH/2.0f)
#define DISP_HALF_H (HU_DISP_HEIGHT/2.0f)

typedef struct {
    /* 0x00 */ float time;
    /* 0x04 */ float distance;
    /* 0x08 */ Vec position;
    /* 0x14 */ Vec up;
    /* 0x20 */ Vec target;
} HsfexStruct02; // Size 0x2C

static void SetObjCamMotion(s16 model_id, HSFTRACK *track, float value, HsfexStruct02 *sample);

/**
 * @brief Plays a model camera motion from the selected camera's current pose.
 *
 * @param model_id Model whose active motion contains the camera tracks.
 * @param camera_mask Bit mask selecting the camera to update.
 * @param duration Number of frames over which to play the camera motion.
 * @param ease_mode Time remapping mode: 0 linear, 1 ease-out, or 2 ease-in.
 */
void CamMotionEx2(s16 model_id, s16 camera_mask, float duration, s16 ease_mode) {
    HU3DCAMERA *camera;
    s16 i;

    for (i = 0; i < HU3D_CAM_MAX; i++) {
        if (camera_mask & (1 << i)) {
            break;
        }
    }
    camera = &Hu3DCamera[i];
    CamMotionEx(model_id, camera_mask, &camera->pos, &camera->up, &camera->target, duration, ease_mode);
}

/**
 * @brief Samples and plays the camera tracks in a model motion.
 *
 * @details Samples the source motion at six-frame intervals, redistributes the
 * samples by travelled distance, and interpolates the camera pose each frame.
 *
 * @param model_id Model whose active motion contains the camera tracks.
 * @param camera_mask Bit mask selecting the camera to update.
 * @param start_position Camera position used before the first sampled key.
 * @param start_up Camera up vector used before the first sampled key.
 * @param start_target Camera target used before the first sampled key.
 * @param duration Number of frames over which to play the camera motion.
 * @param ease_mode Time remapping mode: 0 linear, 1 ease-out, or 2 ease-in;
 * other values are not handled.
 */
void CamMotionEx(s16 model_id, s16 camera_mask, Vec *start_position, Vec *start_up, Vec *start_target, float duration, s16 ease_mode) {
    Vec segment_delta;
    float control_values[4];
    float control_times[4];
    float motion_max_time;
    float accumulated_distance;
    float total_distance;
    float playback_time;
    float frame;
    s16 sample_capacity;
    s16 camera_index;
    s16 sample_count;
    s16 sample_index;
    HsfexStruct02 *sample;
    HU3DCAMERA *camera;
    HU3DMODEL *model;
    HSFDATA *model_hsf;
    HU3DMOTION *motion_data;
    HSFDATA *motion_hsf;
    HSFOBJECT *target_object;
    HSFTRACK *track_end;
    HSFMOTION *motion;
    HsfexStruct02 *samples;
    HSFTRACK *track;

    model = &Hu3DData[model_id];
    motion_data = &Hu3DMotion[model->motId];
    model_hsf = model->hsf;
    motion_hsf = motion_data->hsf;
    motion = motion_hsf->motion;
    for (camera_index = 0; camera_index < HU3D_CAM_MAX; camera_index++) {
        if (camera_mask & (1 << camera_index)) {
            break;
        }
    }
    camera = &Hu3DCamera[camera_index];
    motion_max_time = motion->maxTime;
    sample_capacity = 0.5f + (motion_max_time / 6.0f) + 1.0f;
    sample = samples = HuMemDirectMallocNum(HEAP_HEAP, (sample_capacity + 1) * sizeof(HsfexStruct02), HU_MEMNUM_OVL);
    sample->time = 0.0f;
    sample->position = *start_position;
    sample->target = *start_target;
    sample->up = *start_up;
    sample++;
    for (sample_count = 1, frame = 0.0f; frame <= motion_max_time; sample++, sample_count++) {
        sample->time = frame;
        track = motion->track;
        track_end = &track[motion->numTracks];
        while (track < track_end) {
            if (track->type == 2) {
                target_object = &model_hsf->object[track->target];
                if (target_object->type == 7) {
                    SetObjCamMotion(model_id, track, GetCurve(track, frame), sample);
                }
            }
            track++;
        }
        frame += 6.0f;
    }
    if (frame != motion_max_time) {
        sample->time = motion_max_time;
        track = motion->track;
        track_end = &track[motion->numTracks];
        while (track < track_end) {
            if (track->type == 2) {
                target_object = &model_hsf->object[track->target];
                if (target_object->type == 7) {
                    SetObjCamMotion(model_id, track, GetCurve(track, motion_max_time), sample);
                }
            }
            track++;
        }
        sample_count++;
    }
    sample = samples;
    sample->distance = 0.0f;
    total_distance = 0.0f;
    for (frame = total_distance; frame < sample_count - 1; frame += 1.0f, sample++) {
        VECSubtract(&sample[1].position, &sample[0].position, &segment_delta);
        sample[1].distance = VECMag(&segment_delta);
        total_distance += sample[1].distance;
    }
    sample = samples;
    accumulated_distance = 0.0f;
    frame = accumulated_distance;
    while (frame < sample_count) {
        accumulated_distance += sample->distance;
        sample->time = duration * (accumulated_distance / total_distance);
        frame += 1.0f;
        sample++;
    }
    frame = 0.0f;
    while (frame <= duration) {
        switch (ease_mode) {
            case 0:
                playback_time = frame;
                break;
            case 1:
                playback_time = duration * sind(90.0f * (frame / duration));
                break;
            case 2:
                playback_time = duration * (1.0 - cosd(90.0f * (frame / duration)));
                break;
            }
        sample = samples;
        for (sample_index = 0; sample_index < sample_count; sample_index++, sample++) {
            if (sample->time <= playback_time && sample[1].time > playback_time) {
                break;
            }
        }
        if (sample_index != sample_count) {
            if (sample_index == 0) {
                control_times[0] = -1.0f;
            } else {
                control_times[0] = sample[-1].time;
            }
            control_times[1] = sample->time;
            if (sample_index >= sample_count - 1) {
                control_times[2] = 1.0f + sample->time;
            }
            control_times[2] = sample[1].time;
            if (sample_index >= sample_count - 2) {
                control_times[3] = 1.0f + control_times[2];
            } else {
                control_times[3] = sample[2].time;
            }
            if (sample_index == 0) {
                control_values[0] = sample[0].position.x;
            } else {
                control_values[0] = sample[-1].position.x;
            }
            control_values[1] = sample[0].position.x;
            if (sample_index >= sample_count - 1) {
                control_values[2] = control_values[1];
            } else {
                control_values[2] = sample[1].position.x;
            }
            if (sample_index >= sample_count - 2) {
                control_values[3] = control_values[2];
            } else {
                control_values[3] = sample[2].position.x;
            }
            camera->pos.x = InterpolateBMLine(control_values, control_times, playback_time);
            if (sample_index == 0) {
                control_values[0] = sample[0].position.y;
            } else {
                control_values[0] = sample[-1].position.y;
            }
            control_values[1] = sample[0].position.y;
            if (sample_index >= sample_count - 1) {
                control_values[2] = control_values[1];
            } else {
                control_values[2] = sample[1].position.y;
            }
            if (sample_index >= sample_count - 2) {
                control_values[3] = control_values[2];
            } else {
                control_values[3] = sample[2].position.y;
            }
            camera->pos.y = InterpolateBMLine(control_values, control_times, playback_time);
            if (sample_index == 0) {
                control_values[0] = sample[0].position.z;
            } else {
                control_values[0] = sample[-1].position.z;
            }
            control_values[1] = sample[0].position.z;
            if (sample_index >= sample_count - 1) {
                control_values[2] = control_values[1];
            } else {
                control_values[2] = sample[1].position.z;
            }
            if (sample_index >= sample_count - 2) {
                control_values[3] = control_values[2];
            } else {
                control_values[3] = sample[2].position.z;
            }
            camera->pos.z = InterpolateBMLine(control_values, control_times, playback_time);
            if (sample_index == 0) {
                control_values[0] = sample[0].target.x;
            } else {
                control_values[0] = sample[-1].target.x;
            }
            control_values[1] = sample[0].target.x;
            if (sample_index >= sample_count - 1) {
                control_values[2] = control_values[1];
            } else {
                control_values[2] = sample[1].target.x;
            }
            if (sample_index >= sample_count - 2) {
                control_values[3] = control_values[2];
            } else {
                control_values[3] = sample[2].target.x;
            }
            camera->target.x = InterpolateBMLine(control_values, control_times, playback_time);
            if (sample_index == 0) {
                control_values[0] = sample->target.y;
            } else {
                control_values[0] = sample[-1].target.y;
            }
            control_values[1] = sample->target.y;
            if (sample_index >= sample_count - 1) {
                control_values[2] = control_values[1];
            } else {
                control_values[2] = sample[1].target.y;
            }
            if (sample_index >= sample_count - 2) {
                control_values[3] = control_values[2];
            } else {
                control_values[3] = sample[2].target.y;
            }
            camera->target.y = InterpolateBMLine(control_values, control_times, playback_time);
            if (sample_index == 0) {
                control_values[0] = sample->target.z;
            } else {
                control_values[0] = sample[-1].target.z;
            }
            control_values[1] = sample->target.z;
            if (sample_index >= sample_count - 1) {
                control_values[2] = control_values[1];
            } else {
                control_values[2] = sample[1].target.z;
            }
            if (sample_index >= sample_count - 2) {
                control_values[3] = control_values[2];
            } else {
                control_values[3] = sample[2].target.z;
            }
            camera->target.z = InterpolateBMLine(control_values, control_times, playback_time);
            if (sample_index == 0) {
                control_values[0] = sample->up.x;
            } else {
                control_values[0] = sample[-1].up.x;
            }
            control_values[1] = sample->up.x;
            if (sample_index >= sample_count - 1) {
                control_values[2] = control_values[1];
            } else {
                control_values[2] = sample[1].up.x;
            }
            if (sample_index >= sample_count - 2) {
                control_values[3] = control_values[2];
            } else {
                control_values[3] = sample[2].up.x;
            }
            camera->up.x = InterpolateBMLine(control_values, control_times, playback_time);
            if (sample_index == 0) {
                control_values[0] = sample->up.y;
            } else {
                control_values[0] = sample[-1].up.y;
            }
            control_values[1] = sample->up.y;
            if (sample_index >= sample_count - 1) {
                control_values[2] = control_values[1];
            } else {
                control_values[2] = sample[1].up.y;
            }
            if (sample_index >= sample_count - 2) {
                control_values[3] = control_values[2];
            } else {
                control_values[3] = sample[2].up.y;
            }
            camera->up.y = InterpolateBMLine(control_values, control_times, playback_time);
            if (sample_index == 0) {
                control_values[0] = sample->up.z;
            } else {
                control_values[0] = sample[-1].up.z;
            }
            control_values[1] = sample->up.z;
            if (sample_index >= sample_count - 1) {
                control_values[2] = control_values[1];
            } else {
                control_values[2] = sample[1].up.z;
            }
            if (sample_index >= sample_count - 2) {
                control_values[3] = control_values[2];
            } else {
                control_values[3] = sample[2].up.z;
            }
            camera->up.z = InterpolateBMLine(control_values, control_times, playback_time);
            HuPrcVSleep();
            frame += 1.0f;
        } else {
            break;
        }
    }
    HuMemDirectFree(samples);
}

/**
 * @brief Applies one evaluated camera track channel to a sampled camera pose.
 *
 * @param model_id Model that supplies the camera transform.
 * @param track Camera motion track being evaluated.
 * @param value Evaluated value of the track.
 * @param sample Camera pose sample to update.
 */
static void SetObjCamMotion(s16 model_id, HSFTRACK *track, float value, HsfexStruct02 *sample) {
    HU3DMODEL *model = &Hu3DData[model_id];
    Vec rotated_up;
    Vec view_direction;

    switch (track->channel) {
        case 8:
            sample->position.x = model->scale.x * (value + model->pos.x);
            break;
        case 9:
            sample->position.y = model->scale.y * (value + model->pos.y);
            break;
        case 10:
            sample->position.z = model->scale.z * (value + model->pos.z);
            break;
        case 11:
            sample->target.x = model->scale.x * (value + model->pos.x);
            break;
        case 12:
            sample->target.y = model->scale.y * (value + model->pos.y);
            break;
        case 13:
            sample->target.z = model->scale.z * (value + model->pos.z);
            break;
        case 14:
            VECSubtract(&sample->position, &sample->target, &view_direction);
            VECNormalize(&view_direction, &view_direction);
            rotated_up.x = view_direction.x * view_direction.y * (1.0 - cosd(value)) - view_direction.z * sind(value);
            rotated_up.y = view_direction.y * view_direction.y + (1.0f - view_direction.y * view_direction.y) * cosd(value);
            rotated_up.z = view_direction.y * view_direction.z * (1.0 - cosd(value)) + view_direction.x * sind(value);
            VECNormalize(&rotated_up, &sample->up);
            break;
    }
}

/**
 * @brief Interpolates between the middle two of four time-value samples.
 *
 * @details Uses the neighboring secant slopes to construct two quadratic
 * segments joined at the midpoint of the second and third sample times.
 *
 * @param values Four sample values surrounding the interval to evaluate.
 * @param times Four corresponding sample times in ascending order.
 * @param time Sample time between @p times[1] and @p times[2].
 * @return Interpolated value at @p time.
 */
float InterpolateBMLine(float *values, float *times, float time) {
    float endpoint_slopes[2];
    float slope_intersection_time;
    float segment_end_value;
    float base_slope;
    float midpoint_time;
    float half_interval;
    float midpoint_value;
    float midpoint_slope;
    float segment_start_value;
    float segment_start_slope;
    float segment_end_time;
    float segment_end_slope;
    float segment_start_time;
    s32 has_slope_intersection;
    s32 intersection_in_range;
    s32 i;

    if (values[0] == values[1] && values[0] == values[2] && values[0] == values[3]) {
        return values[0];
    }
    for (i = 1; i <= 2; i++) {
        endpoint_slopes[i - 1] = 0.5f * ((values[i] - values[i - 1]) / (times[i] - times[i - 1]) + (values[i + 1] - values[i]) / (times[i + 1] - times[i]));
    }
    midpoint_time = 0.5f * (times[2] + times[1]);
    has_slope_intersection = 0;
    if (endpoint_slopes[1] - endpoint_slopes[0] != 0.0f) {
        slope_intersection_time = (endpoint_slopes[1] * times[2] - endpoint_slopes[0] * times[1] - (values[2] - values[1])) / (endpoint_slopes[1] - endpoint_slopes[0]);
        intersection_in_range = 0;
        if (times[1] <= slope_intersection_time && slope_intersection_time <= times[2]) {
            intersection_in_range = 1;
        }
        has_slope_intersection = (intersection_in_range != 0) ? 1 : 0;
    }
    if (has_slope_intersection == 1) {
        half_interval = midpoint_time - times[1];
        base_slope = (values[2] - values[1]) / (times[2] - times[1]) - (endpoint_slopes[1] - endpoint_slopes[0]) / 2;
        midpoint_value = base_slope * half_interval + ((endpoint_slopes[1] - endpoint_slopes[0]) / (2.0f * (times[2] - times[1]))) * half_interval * half_interval + values[1];
        midpoint_slope = base_slope + half_interval * ((endpoint_slopes[1] - endpoint_slopes[0]) / (times[2] - times[1]));
    } else {
        half_interval = midpoint_time - times[1];
        midpoint_value = (values[2] + values[1]) * (half_interval / (times[2] - times[1]));
        midpoint_slope = 2.0f * (values[2] - values[1]) / (times[2] - times[1]) - (endpoint_slopes[1] + endpoint_slopes[0]) * (half_interval / (times[2] - times[1]));
    }
    if (time < midpoint_time) {
        segment_start_time = times[1];
        segment_start_value = values[1];
        segment_start_slope = endpoint_slopes[0];
        segment_end_time = midpoint_time;
        segment_end_value = midpoint_value;
        segment_end_slope = midpoint_slope;
    } else {
        segment_start_time = midpoint_time;
        segment_start_value = midpoint_value;
        segment_start_slope = midpoint_slope;
        segment_end_time = times[2];
        segment_end_value = values[2];
        segment_end_slope = endpoint_slopes[1];
    }
    return ((segment_end_slope - segment_start_slope) / (2.0f * (segment_end_time - segment_start_time))) * (time - segment_start_time) * (time - segment_start_time) + (time - segment_start_time) * ((segment_end_value - segment_start_value) / (segment_end_time - segment_start_time) - (segment_end_slope - segment_start_slope) / 2) + segment_start_value;
}

/**
 * @brief Unprojects a screen coordinate and view depth into world space.
 *
 * @param screen_position Screen x/y in pixels and positive view depth in z.
 * @param camera_mask Bit mask selecting the camera whose view to invert.
 * @param world_position Receives the corresponding world-space position.
 */
void Hu3D2Dto3D(Vec *screen_position, s16 camera_mask, Vec *world_position) {
    HU3DCAMERA *camera;
    float view_height;
    float tan_half_fov;
    float view_width;
    float normalized_x;
    float normalized_y;
    s16 i;
    Mtx view_matrix;

    for (i = 0; i < HU3D_CAM_MAX; i++) {
        if (camera_mask & (1 << i)) {
            break;
        }
    }
    camera = &Hu3DCamera[i];
    tan_half_fov = sind(camera->fov / 2) / cosd(camera->fov / 2);
    view_height = tan_half_fov * screen_position->z * 2.0f;
    view_width = view_height * HU_DISP_ASPECT;
    normalized_x = screen_position->x / HU_DISP_WIDTH;
    normalized_y = screen_position->y / HU_DISP_HEIGHT;
    world_position->x = (normalized_x - 0.5) * view_width;
    world_position->y = -(normalized_y - 0.5) * view_height;
    world_position->z = -screen_position->z;
    C_MTXLookAt(view_matrix, &camera->pos, &camera->up, &camera->target);
    PSMTXInverse(view_matrix, view_matrix);
    PSMTXMultVec(view_matrix, world_position, world_position);
}

/**
 * @brief Projects a world-space point onto the display.
 *
 * @param world_position World-space position to project.
 * @param camera_mask Bit mask selecting the camera used for projection.
 * @param screen_position Receives x/y display coordinates in pixels; z is set
 * to zero.
 */
void Hu3D3Dto2D(Vec *world_position, s16 camera_mask, Vec *screen_position) {
    Vec view_position;
    HU3DCAMERA *camera;
    float projected_half_width;
    float projected_half_height;
    s16 i;
    Mtx view_matrix;

    for (i = 0; i < HU3D_CAM_MAX; i++) {
        if (camera_mask & (1 << i)) {
            break;
        }
    }
    camera = &Hu3DCamera[i];
    C_MTXLookAt(view_matrix, &camera->pos, &camera->up, &camera->target);
    PSMTXMultVec(view_matrix, world_position, &view_position);
    projected_half_width = (sind(camera->fov / 2) / cosd(camera->fov / 2)) * view_position.z * HU_DISP_ASPECT;
    projected_half_height = (sind(camera->fov / 2) / cosd(camera->fov / 2)) * view_position.z;
    screen_position->x = DISP_HALF_W + view_position.x * (DISP_HALF_W / -projected_half_width);
    screen_position->y = DISP_HALF_H + view_position.y * (DISP_HALF_H / projected_half_height);
    screen_position->z = 0.0f;
}

/**
 * @brief Extracts the translation column from a 3x4 transform matrix.
 *
 * @param matrix Transform matrix to inspect.
 * @param translation Receives the matrix translation in x, y, and z.
 */
void Hu3DMtxTransGet(Mtx matrix, Vec *translation) {
    translation->x = matrix[0][3];
    translation->y = matrix[1][3];
    translation->z = matrix[2][3];
}

/**
 * @brief Computes an angle while defining the zero-denominator limit.
 *
 * @param numerator Value corresponding to the sine-like axis component.
 * @param denominator Value corresponding to the cosine-like axis component.
 * @return Angle in radians in the range produced by atan2f.
 */
static inline float GetAngleXY(float numerator, float denominator) {
    if (denominator == 0.0f) {
        if (numerator >= 0.0f) {
            return M_PI / 2;
        } else {
            return -(M_PI / 2);
        }
    } else {
        return atan2f(numerator, denominator);
    }
}

/**
 * @brief Extracts Euler rotation from a scaled 3x4 transform matrix.
 *
 * @details Removes the column magnitudes before deriving angles. Degenerate
 * matrices produce a zero rotation vector.
 *
 * @param matrix Transform matrix to inspect.
 * @param rotation Receives x/y/z Euler angles in degrees.
 */
void Hu3DMtxRotGet(Mtx matrix, Vec *rotation) {
    float cos_y;
    float x_axis_scale;
    float y_axis_scale;
    float z_axis_scale;
    float x_axis_length_squared;
    float y_axis_length_squared;
    float z_axis_length_squared;
    float y_angle;
    float sin_y;

    x_axis_length_squared = matrix[0][0] * matrix[0][0] + matrix[1][0] * matrix[1][0] + matrix[2][0] * matrix[2][0];
    x_axis_scale = sqrtf(x_axis_length_squared);
    if (!(x_axis_scale < 0.00000001f)) {
        y_axis_length_squared = matrix[0][1] * matrix[0][1] + matrix[1][1] * matrix[1][1] + matrix[2][1] * matrix[2][1];
        y_axis_scale = sqrtf(y_axis_length_squared);
        if (!(y_axis_scale < 0.00000001f)) {
            z_axis_length_squared = matrix[0][2] * matrix[0][2] + matrix[1][2] * matrix[1][2] + matrix[2][2] * matrix[2][2];
            z_axis_scale = sqrtf(z_axis_length_squared);
            if (!(z_axis_scale < 0.00000001f)) {
                sin_y = -matrix[2][0] / x_axis_scale;
                if (sin_y >= 1.0f) {
                    y_angle = M_PI / 2;
                } else if (sin_y <= -1.0f) {
                    y_angle = -(M_PI / 2);
                } else {
                    y_angle = asinf(sin_y);
                }
                rotation->y = y_angle;
                cos_y = cos(rotation->y);
                if (cos_y >= 0.00000001f) {
                    rotation->x = GetAngleXY(matrix[2][1] / y_axis_scale, matrix[2][2] / z_axis_scale);
                    rotation->z = GetAngleXY(matrix[1][0], matrix[0][0]);
                } else {
                    rotation->x = GetAngleXY(matrix[0][1], matrix[1][1]);
                    rotation->z = 0.0f;
                }
                rotation->x = MTXRadToDeg(rotation->x);
                rotation->y = MTXRadToDeg(rotation->y);
                rotation->z = MTXRadToDeg(rotation->z);
                return;
            }
        }
    }
    rotation->x = 0.0f;
    rotation->y = 0.0f;
    rotation->z = 0.0f;
}

/**
 * @brief Extracts signed axis scales from a 3x4 transform matrix.
 *
 * @details Orthogonalizes the matrix columns before measuring them and negates
 * all three scales when the resulting basis has negative handedness.
 *
 * @param matrix Transform matrix to inspect.
 * @param scale Receives the signed x, y, and z scale factors.
 */
void Hu3DMtxScaleGet(Mtx matrix, Vec *scale) {
    Vec shear;
    Vec x_axis;
    Vec y_axis;
    Vec z_axis;
    Vec projection;

    x_axis.x = matrix[0][0];
    x_axis.y = matrix[1][0];
    x_axis.z = matrix[2][0];
    scale->x = VECMag(&x_axis);
    VECNormalize(&x_axis, &x_axis);
    y_axis.x = matrix[0][1];
    y_axis.y = matrix[1][1];
    y_axis.z = matrix[2][1];
    shear.x = VECDotProduct(&x_axis, &y_axis);
    VECScale(&x_axis, &projection, shear.x);
    VECSubtract(&y_axis, &projection, &y_axis);
    scale->y = VECMag(&y_axis);
    VECNormalize(&y_axis, &y_axis);
    shear.x /= scale->y;
    z_axis.x = matrix[0][2];
    z_axis.y = matrix[1][2];
    z_axis.z = matrix[2][2];
    shear.z = VECDotProduct(&y_axis, &z_axis);
    VECScale(&y_axis, &projection, shear.z);
    VECSubtract(&z_axis, &projection, &z_axis);
    shear.y = VECDotProduct(&x_axis, &z_axis);
    VECScale(&x_axis, &projection, shear.y);
    VECSubtract(&z_axis, &projection, &z_axis);
    scale->z = VECMag(&z_axis);
    VECNormalize(&z_axis, &z_axis);
    VECCrossProduct(&y_axis, &z_axis, &projection);
    if (VECDotProduct(&x_axis, &projection) < 0.0) {
        scale->x *= -1.0;
        scale->y *= -1.0;
        scale->z *= -1.0;
    }
}
