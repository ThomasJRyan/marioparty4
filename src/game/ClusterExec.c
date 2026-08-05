#include "game/ClusterExec.h"
#include "game/EnvelopeExec.h"
#include "game/hu3d.h"

#include "game/sprite.h"

/**
 * @brief Evaluates an HSF cluster index animation track.
 *
 * @param track Track containing linear, Bezier, or constant curve data.
 * @param sample_time Motion time at which to sample the curve.
 * @return Sampled cluster index value, or 0.0 for an unsupported curve type.
 */
float GetClusterCurve(HSFTRACK *track, float sample_time) {
    float *constant_value;

    switch (track->curveType) {
        case 1:
            return GetLinear(track->numKeyframes, track->data, sample_time);
        case 2:
            return GetBezier(track->numKeyframes, track, sample_time);
        case 4:
            constant_value = &track->value;
            return *constant_value;
    }
    return 0.0f;
}

/**
 * @brief Evaluates an HSF cluster-weight animation track.
 *
 * @param track Track containing linear, Bezier, or constant curve data.
 * @param sample_time Motion time at which to sample the curve.
 * @return Sampled morph weight, or 0.0 for an unsupported curve type.
 */
float GetClusterWeightCurve(HSFTRACK *track, float sample_time) {
    float *constant_value;

    switch (track->curveType) {
        case 1:
            return GetLinear(track->numKeyframes, track->data, sample_time);
        case 2:
            return GetBezier(track->numKeyframes, track, sample_time);
        case 4:
            constant_value = &track->value;
            return *constant_value;
    }
    return 0.0f;
}

/**
 * @brief Applies one cluster's current blend state to its target vertices.
 *
 * @details Type 2 clusters blend all supplied buffers by weight. Other types
 * interpolate between buffers surrounding the cluster's fractional index.
 *
 * @param cluster Cluster whose part indices and vertex sources drive the update.
 */
void SetClusterMain(HSFCLUSTER *cluster) {
    float total_weight;
    float blend_weight;
    s32 vertex_index;
    s32 target_vertex_index;
    u16 *part_vertex;
    s32 next_vertex_index;
    s32 i;
    s32 j;
    HSFBUFFER *next_vertex_buffer;
    HSFPART *part;
    HSFBUFFER *vertex_buffer;

    part = cluster->part;
    if (cluster->vertexNum != 0) {
        if (cluster->type == 2) {
            part_vertex = part->vertex;
            vertex_buffer = *cluster->vertex;
            total_weight = 0.0f;
            for (i = 0; i < cluster->vertexNum; i++) {
                total_weight += cluster->weight[i];
            }
            for (i = 0; i < part->num; i++, part_vertex++) {
                target_vertex_index = *part_vertex;
                Vertextop[target_vertex_index].x = ((Vec*) vertex_buffer->data)[i].x;
                Vertextop[target_vertex_index].y = ((Vec*) vertex_buffer->data)[i].y;
                Vertextop[target_vertex_index].z = ((Vec*) vertex_buffer->data)[i].z;
            }
            for (i = 1; i < cluster->vertexNum; i++) {
                vertex_buffer = cluster->vertex[i];
                part_vertex = part->vertex;
                blend_weight = cluster->weight[i];
                if (blend_weight < 0.0f) {
                    blend_weight = 0.0f;
                } else if (total_weight > 1.0f) {
                    blend_weight /= total_weight;
                }
                for (j = 0; j < part->num; j++, part_vertex++) {
                    target_vertex_index = *part_vertex;
                    Vertextop[target_vertex_index].x += blend_weight * (((Vec*) vertex_buffer->data)[j].x - Vertextop[target_vertex_index].x);
                    Vertextop[target_vertex_index].y += blend_weight * (((Vec*) vertex_buffer->data)[j].y - Vertextop[target_vertex_index].y);
                    Vertextop[target_vertex_index].z += blend_weight * (((Vec*) vertex_buffer->data)[j].z - Vertextop[target_vertex_index].z);
                }
            }
            return;
        }
        vertex_index = cluster->index;
        next_vertex_index = vertex_index + 1;
        if (next_vertex_index >= cluster->vertexNum) {
            next_vertex_index = vertex_index;
        }
        blend_weight = cluster->index - vertex_index;
        vertex_buffer = cluster->vertex[vertex_index];
        next_vertex_buffer = cluster->vertex[next_vertex_index];
        part_vertex = part->vertex;
        for (i = 0; i < part->num; i++, part_vertex++) {
            target_vertex_index = *part_vertex;
            Vertextop[target_vertex_index].x = ((Vec*) vertex_buffer->data)[i].x + blend_weight * (((Vec*) next_vertex_buffer->data)[i].x - ((Vec*) vertex_buffer->data)[i].x);
            Vertextop[target_vertex_index].y = ((Vec*) vertex_buffer->data)[i].y + blend_weight * (((Vec*) next_vertex_buffer->data)[i].y - ((Vec*) vertex_buffer->data)[i].y);
            Vertextop[target_vertex_index].z = ((Vec*) vertex_buffer->data)[i].z + blend_weight * (((Vec*) next_vertex_buffer->data)[i].z - ((Vec*) vertex_buffer->data)[i].z);
        }
    }
}

/**
 * @brief Applies all active cluster motions to a rendered model's meshes.
 *
 * @param model Runtime model whose four cluster-motion slots are processed.
 */
void ClusterProc(HU3DMODEL *model) {
    s32 motion_id;
    s32 i;
    s32 j;
    s32 k;
    HSFDATA *motion_hsf;
    HSFDATA *model_hsf;
    HU3DMOTION *motion;
    HSFCLUSTER *cluster;
    HSFOBJECT *target_object;

    for (i = 0; i < 4; i++) {
        motion_id = model->motIdCluster[i];
        if (motion_id != -1) {
            motion = &Hu3DMotion[motion_id];
            motion_hsf = motion->hsf;
            model_hsf = model->hsf;
            cluster = motion_hsf->cluster;
            for (j = 0; j < motion_hsf->clusterNum; j++, cluster++) {
                if (cluster->target != -1) {
                    target_object = model_hsf->object;
                    target_object += cluster->target;
                    Vertextop = target_object->mesh.vertex->data;
                    if (target_object->mesh.cenvNum) {
                        for (k = 0; k < target_object->mesh.vertex->count; k++) {
                            Vertextop[k].x = ((Vec*) target_object->mesh.file[0])[k].x;
                            Vertextop[k].y = ((Vec*) target_object->mesh.file[0])[k].y;
                            Vertextop[k].z = ((Vec*) target_object->mesh.file[0])[k].z;
                        }
                    }
                    SetClusterMain(cluster);
                    DCStoreRangeNoSync(Vertextop, target_object->mesh.vertex->count * sizeof(Vec));
                    target_object->mesh.writeNum++;
                }
            }
        }
    }
}

/**
 * @brief Samples active cluster motion tracks into cluster indices and weights.
 *
 * @param model Runtime model providing motion IDs and per-slot sample times.
 */
void ClusterMotionExec(HU3DMODEL *model) {
    float motion_time;
    s32 i;
    s32 j;
    s16 motion_id;
    HSFCLUSTER *cluster;
    HSFDATA *motion_hsf;
    HSFMOTION *motion_data; // ! - uninitialized
    HSFTRACK *track;
    HSFTRACK *weight_track;
    HU3DMOTION *motion;

    track = motion_data->track;
    for (i = 0; i < 4; i++) {
        if (model->motIdCluster[i] != -1) {
            motion_id = model->motIdCluster[i];
            motion = &Hu3DMotion[motion_id];
            motion_hsf = motion->hsf;
            motion_data = motion_hsf->motion;
            track = motion_data->track;
            motion_time = model->clusterTime[i];
            for (j = 0; j < motion_data->numTracks; j++, track++) {
                switch (track->type) {
                    case 5:
                        cluster = &motion_hsf->cluster[track->cluster];
                        cluster->index = GetClusterCurve(track, motion_time);
                        break;
                    case 6:
                        weight_track = track;
                        cluster = &motion_hsf->cluster[weight_track->cluster];
                        cluster->weight[weight_track->clusterWeight] = GetClusterCurve(weight_track, motion_time);
                        break;
                }
            }
        }
    }
}
