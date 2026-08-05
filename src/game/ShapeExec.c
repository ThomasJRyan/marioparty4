#include "game/ShapeExec.h"
#include "game/EnvelopeExec.h"

/**
 * @brief Applies an HSF mesh's current morph-shape state to its vertices.
 *
 * @details Weighted shapes are blended toward their source buffers. Indexed
 * shapes interpolate between the two buffers surrounding the fractional
 * base-morph value.
 *
 * @param arg0 Mesh object whose vertex buffer is updated in place.
 */
static void SetShapeMain(HSFOBJECT *arg0) {
    HSFBUFFER *next_shape;
    HSFBUFFER *shape;
    float total_weight;
    float blend_weight;
    s32 shape_index;
    s32 vertex_index;
    s32 next_shape_index;
    s32 i;

    if (arg0->mesh.shapeType == 2) {
        total_weight = 0.0f;
        for (i = 0; i < arg0->mesh.shapeNum; i++) {
            total_weight += arg0->mesh.mesh.morphWeight[i];
        }
        shape = *arg0->mesh.shape;
        for (i = 0; i < shape->count; i++) {
            Vertextop[i].x = ((Vec*) shape->data)[i].x;
            Vertextop[i].y = ((Vec*) shape->data)[i].y;
            Vertextop[i].z = ((Vec*) shape->data)[i].z;
        }
        for (i = 0; i < arg0->mesh.shapeNum; i++) {
            shape = arg0->mesh.shape[i];
            blend_weight = arg0->mesh.mesh.morphWeight[i];
            if (blend_weight < 0.0f) {
                blend_weight = 0.0f;
            } else if (total_weight > 1.0f) {
                blend_weight /= total_weight;
            }
            for (vertex_index = 0; vertex_index < shape->count; vertex_index++) {
                Vertextop[vertex_index].x += blend_weight * (((Vec*) shape->data)[vertex_index].x - Vertextop[vertex_index].x);
                Vertextop[vertex_index].y += blend_weight * (((Vec*) shape->data)[vertex_index].y - Vertextop[vertex_index].y);
                Vertextop[vertex_index].z += blend_weight * (((Vec*) shape->data)[vertex_index].z - Vertextop[vertex_index].z);
            }
        }
    } else {
        shape_index = arg0->mesh.mesh.baseMorph;
        next_shape_index = shape_index + 1;
        if (next_shape_index >= arg0->mesh.shapeNum) {
            next_shape_index = shape_index;
        }
        blend_weight = arg0->mesh.mesh.baseMorph - shape_index;
        shape = arg0->mesh.shape[shape_index];
        next_shape = arg0->mesh.shape[next_shape_index];
        for (i = 0; i < shape->count; i++) {
            Vertextop[i].x = ((Vec*) shape->data)[i].x + blend_weight * (((Vec*) next_shape->data)[i].x - ((Vec*) shape->data)[i].x);
            Vertextop[i].y = ((Vec*) shape->data)[i].y + blend_weight * (((Vec*) next_shape->data)[i].y - ((Vec*) shape->data)[i].y);
            Vertextop[i].z = ((Vec*) shape->data)[i].z + blend_weight * (((Vec*) next_shape->data)[i].z - ((Vec*) shape->data)[i].z);
        }
    }
}

/**
 * @brief Updates morphable meshes in an HSF model and flushes their vertices.
 *
 * @param arg0 HSF model data containing the object and shape buffers.
 */
void ShapeProc(HSFDATA *arg0) {
    HSFOBJECT *object;
    s32 i;

    object = arg0->object;
    for (i = 0; i < arg0->objectNum; i++, object++) {
        if (object->type == 2 && object->mesh.shapeNum != 0) {
            Vertextop = object->mesh.vertex->data;
            SetShapeMain(object);
            DCStoreRange(Vertextop, object->mesh.vertex->count * sizeof(Vec));
            object->mesh.writeNum++;
        }
    }
}
