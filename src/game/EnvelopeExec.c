#include "game/EnvelopeExec.h"
#include "game/hsfex.h"

#include "string.h"

static void SetEnvelopMtx(HSFOBJECT *object_base, HSFOBJECT *object, Mtx parent_matrix);
static void SetEnvelopMain(HSFDATA *model_hsf);
static void SetEnvelop(HSFCENV *envelope);
static void SetMtx(HSFOBJECT *object, Mtx parent_matrix);
static void SetRevMtx(void);
static HSFSKELETON *SearchSklenton(char *object_name);

Vec *Vertextop;
Mtx *MtxTop;
static u32 nObj;
static u32 nMesh;
static HSFOBJECT *objtop;
static HSFDATA *CurHsf;
static Vec *vtxenv;
static Vec *normenv;
static Vec *normtop;
static s32 Meshcnt;
static s32 Meshno;

/**
 * @brief Initializes bind transforms and envelope matrices for an HSF model.
 *
 * @details Mesh transforms are synchronized with same-named skeleton entries,
 * then world and inverse bind matrices are built for later skinning passes.
 * Models without cluster-envelope records are left unchanged.
 *
 * @param model_hsf HSF model whose mesh and matrix data are initialized.
 */
void InitEnvelope(HSFDATA *model_hsf) {
    HSFBUFFER *vertex_buffer;
    HSFBUFFER *normal_buffer;
    HSFMATRIX *matrix_buffer;
    HSFOBJECT *object;
    HSFSKELETON *skeleton;
    Mtx identity_matrix;
    s32 i;
    s32 j;

    if (model_hsf->cenvNum != 0) {
        object = model_hsf->object;
        for (Meshcnt = i = 0; i < model_hsf->objectNum; i++, object++) {
            if (object->type == 2) {
                if (object->mesh.file[0]) {
                    vertex_buffer = object->mesh.vertex;
                    normal_buffer = object->mesh.normal;
                    Meshcnt++;
                } else {
                    continue;
                }
            }
            skeleton = model_hsf->skeleton;
            for (j = 0; j < model_hsf->skeletonNum; j++, skeleton++) {
                if (strcmp(object->name, skeleton->name) == 0) {
                    object->mesh.base.pos.x = skeleton->transform.pos.x;
                    object->mesh.base.pos.y = skeleton->transform.pos.y;
                    object->mesh.base.pos.z = skeleton->transform.pos.z;
                    object->mesh.base.rot.x = skeleton->transform.rot.x;
                    object->mesh.base.rot.y = skeleton->transform.rot.y;
                    object->mesh.base.rot.z = skeleton->transform.rot.z;
                    object->mesh.base.scale.x = skeleton->transform.scale.x;
                    object->mesh.base.scale.y = skeleton->transform.scale.y;
                    object->mesh.base.scale.z = skeleton->transform.scale.z;
                }
            }
            object->mesh.curr.pos.x = object->mesh.base.pos.x;
            object->mesh.curr.pos.y = object->mesh.base.pos.y;
            object->mesh.curr.pos.z = object->mesh.base.pos.z;
            object->mesh.curr.rot.x = object->mesh.base.rot.x;
            object->mesh.curr.rot.y = object->mesh.base.rot.y;
            object->mesh.curr.rot.z = object->mesh.base.rot.z;
            object->mesh.curr.scale.x = object->mesh.base.scale.x;
            object->mesh.curr.scale.y = object->mesh.base.scale.y;
            object->mesh.curr.scale.z = object->mesh.base.scale.z;
        }
        CurHsf = model_hsf;
        objtop = model_hsf->object;
        matrix_buffer = CurHsf->matrix;
        if (matrix_buffer) {
            MtxTop = matrix_buffer->data;
            nObj = matrix_buffer->count;
            nMesh = matrix_buffer->base_idx;
        }
        PSMTXIdentity(identity_matrix);
        SetMtx(model_hsf->root, identity_matrix);
        SetRevMtx();
    }
}

/**
 * @brief Recursively builds current world matrices for an object hierarchy.
 *
 * @param object_base Base of the contiguous object array, used to derive matrix slots.
 * @param object Current hierarchy object whose transform is evaluated.
 * @param parent_matrix Parent object's world matrix.
 */
static void SetEnvelopMtx(HSFOBJECT *object_base, HSFOBJECT *object, Mtx parent_matrix) {
    Mtx rotation_matrix;
    Mtx world_matrix;
    Mtx translation_matrix;
    s32 object_index;
    s32 i;

    PSMTXTrans(translation_matrix, object->mesh.curr.pos.x, object->mesh.curr.pos.y, object->mesh.curr.pos.z);
    PSMTXConcat(parent_matrix, translation_matrix, world_matrix);
    if (object->mesh.curr.rot.z) {
        PSMTXRotRad(rotation_matrix, 'z', MTXDegToRad(object->mesh.curr.rot.z));
        PSMTXConcat(world_matrix, rotation_matrix, world_matrix);
    }
    if (object->mesh.curr.rot.y) {
        PSMTXRotRad(rotation_matrix, 'y', MTXDegToRad(object->mesh.curr.rot.y));
        PSMTXConcat(world_matrix, rotation_matrix, world_matrix);
    }
    if (object->mesh.curr.rot.x) {
        PSMTXRotRad(rotation_matrix, 'x', MTXDegToRad(object->mesh.curr.rot.x));
        PSMTXConcat(world_matrix, rotation_matrix, world_matrix);
    }
    if (object->mesh.curr.scale.x != 1.0f) {
        world_matrix[0][0] *= object->mesh.curr.scale.x;
        world_matrix[1][0] *= object->mesh.curr.scale.x;
        world_matrix[2][0] *= object->mesh.curr.scale.x;
    }
    if (object->mesh.curr.scale.y != 1.0f) {
        world_matrix[0][1] *= object->mesh.curr.scale.y;
        world_matrix[1][1] *= object->mesh.curr.scale.y;
        world_matrix[2][1] *= object->mesh.curr.scale.y;
    }
    if (object->mesh.curr.scale.z != 1.0f) {
        world_matrix[0][2] *= object->mesh.curr.scale.z;
        world_matrix[1][2] *= object->mesh.curr.scale.z;
        world_matrix[2][2] *= object->mesh.curr.scale.z;
    }
    object_index = object - object_base;
    PSMTXCopy(world_matrix, MtxTop[nMesh + object_index]);
    for (i = 0; i < object->mesh.childrenCount; i++) {
        SetEnvelopMtx(object_base, object->mesh.children[i], world_matrix);
    }
}

/**
 * @brief Rebuilds current hierarchy matrices and skins all enveloped meshes.
 *
 * @param model_hsf HSF model containing the hierarchy, matrices, and envelope data.
 */
void EnvelopeProc(HSFDATA *model_hsf) {
    HSFMATRIX *matrix_buffer;
    HSFOBJECT *root_object;
    Mtx identity_matrix;

    CurHsf = model_hsf;
    matrix_buffer = CurHsf->matrix;
    MtxTop = matrix_buffer->data;
    nObj = matrix_buffer->count;
    nMesh = matrix_buffer->base_idx;
    root_object = model_hsf->root;
    PSMTXIdentity(identity_matrix);
    SetEnvelopMtx(model_hsf->object, root_object, identity_matrix);
    SetEnvelopMain(model_hsf);
}

/**
 * @brief Clears the per-frame vertex-write counters for all HSF mesh objects.
 *
 * @param model_hsf HSF model whose mesh objects are reset.
 */
void InitVtxParm(HSFDATA *model_hsf) {
    HSFOBJECT *object;
    s32 i;

    object = model_hsf->object;
    for (i = 0; i < model_hsf->objectNum; i++, object++) {
        if (object->type == 2) {
            object->mesh.writeNum = 0;
        }
    }
}

/**
 * @brief Skins each mesh with its cluster-envelope records and flushes results.
 *
 * @param model_hsf HSF model whose mesh vertex and normal buffers are updated.
 */
static void SetEnvelopMain(HSFDATA *model_hsf) {
    void *vertex_data;
    void *source_vertex_data;
    void *output_vertex_data;
    HSFBUFFER *normal_buffer;
    HSFBUFFER *vertex_buffer;
    HSFOBJECT *object;
    s32 i;
    s32 j;
    HSFCENV *envelope;

    object = model_hsf->object;
    for (Meshno = i = 0; i < model_hsf->objectNum; i++, object++) {
        if (object->type == 2) {
            PSMTXInverse(MtxTop[&object[nMesh] - model_hsf->object], MtxTop[Meshno]);
            vertex_buffer = object->mesh.vertex;
            normal_buffer = object->mesh.normal;
            if (object->mesh.writeNum != 0) {
                Vertextop = vertex_buffer->data;
            } else {
                Vertextop = object->mesh.file[0];
            }
            vtxenv = vertex_buffer->data;
            normtop = object->mesh.file[1];
            normenv = normal_buffer->data;
            envelope = object->mesh.cenv;
            for (j = 0; j < object->mesh.cenvNum; j++, envelope++) {
                SetEnvelop(envelope);
            }
            vertex_data = vertex_buffer->data;
            source_vertex_data = object->mesh.file[0];
            output_vertex_data = vertex_buffer->data;
            DCStoreRangeNoSync(normenv, normal_buffer->count * sizeof(Vec));
            DCStoreRangeNoSync(vtxenv, vertex_buffer->count * sizeof(Vec));
            Meshno++;
        }
    }
}

/**
 * @brief Applies single-, dual-, and multi-bone skinning records to a mesh.
 *
 * @details Positions and normals are transformed from bind space with matrices
 * precomputed for the current mesh. Unweighted trailing vertices are copied.
 *
 * @param envelope Cluster-envelope record defining affected ranges and weights.
 */
static void SetEnvelop(HSFCENV *envelope) {
    Vec weighted_position_delta;
    Vec position_delta_sum;
    Vec weighted_normal_delta;
    Vec normal_delta_sum;
    Vec matrix_scale;
    s32 processed_weight_count;
    u32 first_target;
    u32 second_target;
    HSFCENVDUAL *dual_envelope;
    HSFCENVDUALWEIGHT *dual_weight;
    HSFCENVMULTI *multi_envelope;
    HSFCENVMULTIWEIGHT *multi_weight;
    HSFCENVSINGLE *single_envelope;
    Vec *output_normal;
    Vec *source_normal;
    Vec *output_position;
    Vec *source_position;
    float weight;
    MtxPtr blended_matrix;
    s32 normal_index;
    s32 position_index;
    s32 i;
    s32 j;
    Mtx position_matrix;
    Mtx normal_matrix;
    Mtx work_matrix_a;
    Mtx work_matrix_b;
    Mtx inverse_scale_matrix;
    Mtx second_position_matrix;
    Mtx combined_matrix;
    Mtx alias_safe_matrix;

    single_envelope = envelope->singleData;
    for (i = 0; i < envelope->singleCount; i++, single_envelope++) {
        normal_index = single_envelope->normal;
        position_index = single_envelope->pos;
        output_position = &vtxenv[position_index];
        source_position = &Vertextop[position_index];
        output_normal = &normenv[normal_index];
        source_normal = &normtop[normal_index];
        PSMTXConcat(MtxTop[nMesh + single_envelope->target], MtxTop[nMesh + nObj + nObj * Meshno + single_envelope->target], work_matrix_a);
        PSMTXConcat(MtxTop[Meshno], work_matrix_a, position_matrix);
        Hu3DMtxScaleGet(&position_matrix[0], &matrix_scale);
        if (matrix_scale.x != 1.0f || matrix_scale.y != 1.0f || matrix_scale.z != 1.0f) {
            PSMTXScale(inverse_scale_matrix, 1.0 / matrix_scale.x, 1.0 / matrix_scale.y, 1.0 / matrix_scale.z);
            PSMTXConcat(inverse_scale_matrix, position_matrix, normal_matrix);
            PSMTXInvXpose(normal_matrix, normal_matrix);
        } else {
            PSMTXInvXpose(position_matrix, normal_matrix);
        }
        if (single_envelope->posNum == 1) {
            PSMTXMultVec(position_matrix, source_position, output_position);
            PSMTXMultVec(normal_matrix, source_normal, output_normal);
        } else if (single_envelope->posNum <= 6) {
            PSMTXMultVecArray(position_matrix, source_position, output_position, single_envelope->posNum);
            PSMTXMultVecArray(normal_matrix, source_normal, output_normal, single_envelope->normalNum);
        } else {
            PSMTXReorder(position_matrix, (ROMtxPtr) work_matrix_a);
            PSMTXReorder(normal_matrix, (ROMtxPtr) work_matrix_b);
            PSMTXROMultVecArray((ROMtxPtr) work_matrix_a, source_position, output_position, single_envelope->posNum);
            PSMTXROMultVecArray((ROMtxPtr) work_matrix_b, source_normal, output_normal, single_envelope->normalNum);
        }
    }
    dual_envelope = envelope->dualData;
    for (i = 0; i < envelope->dualCount; i++, dual_envelope++) {
        first_target = dual_envelope->target1;
        second_target = dual_envelope->target2;
        PSMTXConcat(MtxTop[nMesh + first_target], MtxTop[nMesh + nObj + nObj * Meshno + first_target], work_matrix_a);
        PSMTXConcat(MtxTop[Meshno], work_matrix_a, position_matrix);
        PSMTXConcat(MtxTop[nMesh + second_target], MtxTop[nMesh + nObj + nObj * Meshno + second_target], work_matrix_a);
        PSMTXConcat(MtxTop[Meshno], work_matrix_a, (float (*)[4]) &second_position_matrix[0]);
        dual_weight = dual_envelope->weight;
        for (j = 0; j < dual_envelope->weightNum; j++, dual_weight++) {
            normal_index = dual_weight->normal;
            position_index = dual_weight->pos;
            output_position = &vtxenv[position_index];
            source_position = &Vertextop[position_index];
            output_normal = &normenv[normal_index];
            source_normal = &normtop[normal_index];
            weight = dual_weight->weight;
            work_matrix_a[0][0] = position_matrix[0][0] * weight;
            work_matrix_a[1][0] = position_matrix[1][0] * weight;
            work_matrix_a[2][0] = position_matrix[2][0] * weight;
            work_matrix_a[0][1] = position_matrix[0][1] * weight;
            work_matrix_a[1][1] = position_matrix[1][1] * weight;
            work_matrix_a[2][1] = position_matrix[2][1] * weight;
            work_matrix_a[0][2] = position_matrix[0][2] * weight;
            work_matrix_a[1][2] = position_matrix[1][2] * weight;
            work_matrix_a[2][2] = position_matrix[2][2] * weight;
            work_matrix_a[0][3] = position_matrix[0][3] * weight;
            work_matrix_a[1][3] = position_matrix[1][3] * weight;
            work_matrix_a[2][3] = position_matrix[2][3] * weight;
            weight = 1.0f - dual_weight->weight;
            work_matrix_b[0][0] = second_position_matrix[0][0] * weight;
            work_matrix_b[1][0] = second_position_matrix[1][0] * weight;
            work_matrix_b[2][0] = second_position_matrix[2][0] * weight;
            work_matrix_b[0][1] = second_position_matrix[0][1] * weight;
            work_matrix_b[1][1] = second_position_matrix[1][1] * weight;
            work_matrix_b[2][1] = second_position_matrix[2][1] * weight;
            work_matrix_b[0][2] = second_position_matrix[0][2] * weight;
            work_matrix_b[1][2] = second_position_matrix[1][2] * weight;
            work_matrix_b[2][2] = second_position_matrix[2][2] * weight;
            work_matrix_b[0][3] = second_position_matrix[0][3] * weight;
            work_matrix_b[1][3] = second_position_matrix[1][3] * weight;
            work_matrix_b[2][3] = second_position_matrix[2][3] * weight;
            if (combined_matrix == work_matrix_b || combined_matrix == work_matrix_a) {
                blended_matrix = alias_safe_matrix;
            } else {
                blended_matrix = combined_matrix;
            }
            blended_matrix[0][0] = work_matrix_b[0][0] + work_matrix_a[0][0];
            blended_matrix[0][1] = work_matrix_b[0][1] + work_matrix_a[0][1];
            blended_matrix[0][2] = work_matrix_b[0][2] + work_matrix_a[0][2];
            blended_matrix[0][3] = work_matrix_b[0][3] + work_matrix_a[0][3];
            blended_matrix[1][0] = work_matrix_b[1][0] + work_matrix_a[1][0];
            blended_matrix[1][1] = work_matrix_b[1][1] + work_matrix_a[1][1];
            blended_matrix[1][2] = work_matrix_b[1][2] + work_matrix_a[1][2];
            blended_matrix[1][3] = work_matrix_b[1][3] + work_matrix_a[1][3];
            blended_matrix[2][0] = work_matrix_b[2][0] + work_matrix_a[2][0];
            blended_matrix[2][1] = work_matrix_b[2][1] + work_matrix_a[2][1];
            blended_matrix[2][2] = work_matrix_b[2][2] + work_matrix_a[2][2];
            blended_matrix[2][3] = work_matrix_b[2][3] + work_matrix_a[2][3];
            if (blended_matrix == alias_safe_matrix) {
                PSMTXCopy(alias_safe_matrix, combined_matrix);
            }
            Hu3DMtxScaleGet(&combined_matrix[0], &matrix_scale);
            if (matrix_scale.x != 1.0f || matrix_scale.y != 1.0f || matrix_scale.z != 1.0f) {
                PSMTXScale(inverse_scale_matrix, 1.0 / matrix_scale.x, 1.0 / matrix_scale.y, 1.0 / matrix_scale.z);
                PSMTXConcat(inverse_scale_matrix, combined_matrix, work_matrix_b);
                PSMTXInvXpose(work_matrix_b, work_matrix_b);
            } else {
                PSMTXInvXpose(combined_matrix, work_matrix_b);
            }
            if (dual_weight->posNum == 1) {
                PSMTXMultVec(combined_matrix, source_position, output_position);
            } else if (dual_weight->posNum <= 6) {
                PSMTXMultVecArray(combined_matrix, source_position, output_position, dual_weight->posNum);
            } else {
                PSMTXReorder(combined_matrix, (ROMtxPtr) work_matrix_a);
                PSMTXROMultVecArray((ROMtxPtr) work_matrix_a, source_position, output_position, dual_weight->posNum);
            }
            if (dual_weight->normalNum != 0) {
                if (dual_weight->normalNum == 1) {
                    PSMTXMultVec(work_matrix_b, source_normal, output_normal);
                } else if (dual_weight->normalNum <= 6) {
                    PSMTXMultVecArray(work_matrix_b, source_normal, output_normal, dual_weight->normalNum);
                } else {
                    PSMTXReorder(work_matrix_b, (ROMtxPtr) work_matrix_a);
                    PSMTXROMultVecArray((ROMtxPtr) work_matrix_a, source_normal, output_normal, dual_weight->normalNum);
                }
            }
        }
    }
    multi_envelope = envelope->multiData;
    for (i = 0; i < envelope->multiCount; i++, multi_envelope++) {
        multi_weight = multi_envelope->weight;
        normal_index = multi_envelope->normal;
        position_index = multi_envelope->pos;
        output_position = &vtxenv[position_index];
        source_position = &Vertextop[position_index];
        output_normal = &normenv[normal_index];
        source_normal = &normtop[normal_index];
        position_delta_sum.x = position_delta_sum.y = position_delta_sum.z = 0.0f;
        normal_delta_sum.x = normal_delta_sum.y = normal_delta_sum.z = 0.0f;
        processed_weight_count = 0;
        for (j = 0; j < multi_envelope->weightNum; j++, multi_weight++) {
            PSMTXConcat(MtxTop[nMesh + multi_weight->target], MtxTop[nMesh + nObj + nObj * Meshno + multi_weight->target], position_matrix);
            PSMTXConcat(MtxTop[Meshno], position_matrix, position_matrix);
            PSMTXInvXpose(position_matrix, normal_matrix);
            PSMTXMultVec(position_matrix, source_position, &weighted_position_delta);
            PSMTXMultVec(normal_matrix, source_normal, &weighted_normal_delta);
            weighted_position_delta.x = multi_weight->value * (weighted_position_delta.x - source_position->x);
            weighted_position_delta.y = multi_weight->value * (weighted_position_delta.y - source_position->y);
            weighted_position_delta.z = multi_weight->value * (weighted_position_delta.z - source_position->z);
            VECAdd(&position_delta_sum, &weighted_position_delta, &position_delta_sum);
            weighted_normal_delta.x = multi_weight->value * (weighted_normal_delta.x - source_normal->x);
            weighted_normal_delta.y = multi_weight->value * (weighted_normal_delta.y - source_normal->y);
            weighted_normal_delta.z = multi_weight->value * (weighted_normal_delta.z - source_normal->z);
            VECAdd(&normal_delta_sum, &weighted_normal_delta, &normal_delta_sum);
        }
        output_position->x = source_position->x + position_delta_sum.x;
        output_position->y = source_position->y + position_delta_sum.y;
        output_position->z = source_position->z + position_delta_sum.z;
        output_normal->x = source_normal->x + normal_delta_sum.x;
        output_normal->y = source_normal->y + normal_delta_sum.y;
        output_normal->z = source_normal->z + normal_delta_sum.z;
    }
    position_index = envelope->vtxCount;
    output_position = &vtxenv[position_index];
    source_position = &Vertextop[position_index];
    for (i = 0; i < envelope->copyCount; i++, output_position++, source_position++) {
        output_position->x = source_position->x;
        output_position->y = source_position->y;
        output_position->z = source_position->z;
    }
}

/**
 * @brief Recursively constructs bind-pose world matrices for an object tree.
 *
 * @param object Current object; a same-named skeleton record overrides its base transform.
 * @param parent_matrix Parent bind-pose world matrix.
 */
static void SetMtx(HSFOBJECT *object, Mtx parent_matrix) {
    HSFSKELETON *skeleton;
    Mtx world_matrix;
    Mtx scale_matrix;
    Mtx rotation_matrix;
    s32 object_index;
    s32 i;

    skeleton = SearchSklenton(object->name);
    if (skeleton) {
        object->mesh.base.pos.x = skeleton->transform.pos.x;
        object->mesh.base.pos.y = skeleton->transform.pos.y;
        object->mesh.base.pos.z = skeleton->transform.pos.z;
        object->mesh.base.rot.x = skeleton->transform.rot.x;
        object->mesh.base.rot.y = skeleton->transform.rot.y;
        object->mesh.base.rot.z = skeleton->transform.rot.z;
        object->mesh.base.scale.x = skeleton->transform.scale.x;
        object->mesh.base.scale.y = skeleton->transform.scale.y;
        object->mesh.base.scale.z = skeleton->transform.scale.z;
    }
    PSMTXTrans(world_matrix, object->mesh.base.pos.x, object->mesh.base.pos.y, object->mesh.base.pos.z);
    PSMTXScale(scale_matrix, object->mesh.base.scale.x, object->mesh.base.scale.y, object->mesh.base.scale.z);
    PSMTXConcat(parent_matrix, world_matrix, world_matrix);
    PSMTXRotRad(rotation_matrix, 'z', MTXDegToRad(object->mesh.base.rot.z));
    PSMTXConcat(world_matrix, rotation_matrix, world_matrix);
    PSMTXRotRad(rotation_matrix, 'y', MTXDegToRad(object->mesh.base.rot.y));
    PSMTXConcat(world_matrix, rotation_matrix, world_matrix);
    PSMTXRotRad(rotation_matrix, 'x', MTXDegToRad(object->mesh.base.rot.x));
    PSMTXConcat(world_matrix, rotation_matrix, world_matrix);
    PSMTXConcat(world_matrix, scale_matrix, world_matrix);
    object_index = object - objtop;
    PSMTXCopy(world_matrix, MtxTop[nMesh + object_index]);
    for (i = 0; i < object->mesh.childrenCount; i++) {
        SetMtx(object->mesh.children[i], world_matrix);
    }
}

/**
 * @brief Builds inverse bind matrices for every mesh and model object pair.
 */
static void SetRevMtx(void) {
    HSFOBJECT *object;
    s32 mesh_index;
    s32 i;
    s32 object_index;
    Mtx inverse_object_matrix;
    Mtx mesh_matrix;

    object = CurHsf->object;
    for (mesh_index = i = 0; i < CurHsf->objectNum; i++, object++) {
        if (object->type == 2) {
            PSMTXCopy(MtxTop[nMesh + i], mesh_matrix);
            for (object_index = 0; object_index < CurHsf->objectNum; object_index++) {
                PSMTXInverse(MtxTop[nMesh + object_index], inverse_object_matrix);
                PSMTXConcat(inverse_object_matrix, mesh_matrix, MtxTop[nMesh + nObj + nObj * mesh_index + object_index]);
            }
            PSMTXInverse(MtxTop[nMesh + i], mesh_matrix);
            mesh_index++;
        }
    }
}

/**
 * @brief Finds a skeleton entry by its exact object name.
 *
 * @param object_name Null-terminated HSF object name to match.
 * @return Matching skeleton record, or NULL when the model has no such name.
 */
static HSFSKELETON *SearchSklenton(char *object_name) {
    HSFSKELETON *skeleton;
    s32 i;

    skeleton = CurHsf->skeleton;
    for (i = 0; i < CurHsf->skeletonNum; i++, skeleton++) {
        if (strcmp(object_name, skeleton->name) == 0) {
            return skeleton;
        }
    }
    return NULL;
}
