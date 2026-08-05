#include "game/mapspace.h"
#include "game/hu3d.h"

#include "ext_math.h"

#ifdef __MWERKS__
extern void HuSetVecF(Vec*, double, double, double);
#endif

static BOOL PolygonRangeCheck(HSFMAPATTR *arg0, float arg1, float arg2, float *arg3, float arg4);
static s32 DefIfnnerMapCircle(Vec *arg0, s16 *arg1, Vec *arg2, Vec *arg3);
static s32 CalcPPLength(float *arg0, s16 *arg1, Vec *arg2);
static float MapIflnnerCalc(float arg0, float arg1, float arg2, Vec *arg3, Vec *arg4, Vec *arg5);
static float MapCalcPoint(float arg0, float arg1, float arg2, Vec *arg3, u16 *arg4);
static BOOL AreaCheck(float arg0, float arg1, u16 *arg2, Vec *arg3);
static s32 MapIflnnerTriangle(float arg0, float arg1, u16 *arg2, Vec *arg3);
static s32 MapIflnnerQuadrangle(float arg0, float arg1, u16 *arg2, Vec *arg3);
static BOOL GetPolygonCircleMtx(s16 *arg0, Vec *arg1, float *arg2, float *arg3);
static s32 PrecalcPntToTriangle(Vec *arg0, Vec *arg1, Vec *arg2, Vec* arg3, Vec *arg4, Vec *arg5);
static void DefSetHitFace(float arg0, float arg1, float arg2);

OMOBJ *MapObject[16];
Mtx MapMT;
Mtx MapMTR;
static Vec MTRAdd;
static Vec FieldVec;
s32 ColisionIdx[10][3];
Vec HitFaceVec[32];
static Vec OldXYZ;
Vec HitFace[32];
u8 CharObject[0x30];

float AddX;
float AddZ;
s32 nMap;
s32 nChar;
s32 HitFaceCount;
static HSFDATA *AttrHsf;
static Vec *topvtx;
s32 ColisionCount;

/**
 * @brief Resolves a spherical wall probe against every registered map model.
 * @details The probe is transformed into each model's local space. Contact responses accumulate in AddX and AddZ.
 * @param arg0 Probe radius.
 * @param arg1 Probe center X coordinate in world space.
 * @param arg2 Probe center Y coordinate in world space.
 * @param arg3 Probe center Z coordinate in world space.
 */
void MapWall(float arg0, float arg1, float arg2, float arg3) {
    float world_sphere[4];
    float local_sphere[4];
    float radius;
    float local_x;
    float local_z;
    OMOBJ *map_object;
    HU3DMODEL *map_model;
    HSFDATA *map_hsf;
    HSFMAPATTR *map_attributes;
    HSFMAPATTR *map_attribute;
    s32 model_id;
    s32 i;
    s32 j;

    for (i = 0; i < nMap; i++) {
        map_object = MapObject[i];
        model_id = MapObject[i]->mdlId[0];
        local_sphere[0] = world_sphere[0] = arg1;
        local_sphere[1] = world_sphere[1] = arg2;
        local_sphere[2] = world_sphere[2] = arg3;
        local_sphere[3] = world_sphere[3] = arg0;
        radius = world_sphere[3];
        CharRotInv(MapMT, MapMTR, (Vec*) local_sphere, map_object);
        ColisionCount = 0;
        HitFaceCount = 0;
        map_model = &Hu3DData[model_id];
        map_hsf = map_model->hsf;
        AttrHsf = map_hsf;
        map_attributes = AttrHsf->mapAttr;
        map_attribute = map_hsf->mapAttr;
        for (j = 0; j < map_hsf->mapAttrNum; j++, map_attribute++) {
            local_x = local_sphere[0];
            local_z = local_sphere[2];
            local_sphere[3] = arg0;
            if (map_attribute->minX <= local_x + radius && map_attribute->maxX > local_x - radius
                && map_attribute->minZ <= local_z + radius && map_attribute->maxZ > local_z - radius) {
                MapWallCheck(world_sphere, local_sphere, map_attribute);
            }
        }
    }
}

/**
 * @brief Checks wall polygons from one map attribute against a sphere probe.
 * @param arg0 World-space sphere as X, Y, Z, and radius.
 * @param arg1 Model-local sphere as X, Y, Z, and radius.
 * @param arg2 Packed map attribute whose wall polygons are traversed.
 */
void MapWallCheck(float *arg0, float *arg1, HSFMAPATTR *arg2) {
    u32 data_offset;
    u16 polygon_header;
    u16 *polygon_data;
    s32 tested_polygons;
    Mtx inverse_transpose;

    tested_polygons = 0;
    tested_polygons = 0;
    topvtx = AttrHsf->vertex->data;
    polygon_data = arg2->data;
    MTRAdd.x = AddX;
    MTRAdd.z = AddZ;
    MTRAdd.y = 0.0f;
    PSMTXInvXpose(MapMT, inverse_transpose);
    PSMTXMultVec(inverse_transpose, &MTRAdd, &MTRAdd);
    for (data_offset = 0; data_offset < arg2->dataLen;) {
        polygon_header = *polygon_data;
        if (polygon_header & 0x8000) {
            GetPolygonCircleMtx((s16*) polygon_data, topvtx, arg0, arg1);
            tested_polygons++;
        }
        data_offset += (polygon_header & 0xFF) + 1;
        polygon_data += (polygon_header & 0xFF) + 1;
    }
}

/**
 * @brief Finds the closest eligible map surface at or below a world-space point.
 * @param arg0 World-space X coordinate.
 * @param arg1 World-space reference Y coordinate.
 * @param arg2 World-space Z coordinate.
 * @param arg3 Maximum upward search allowance added to the reference Y coordinate.
 * @param arg4 Destination for the selected surface normal; receives an upward normal when no surface is found.
 * @return Selected world-space surface height, or arg1 when no eligible polygon is found.
 */
float MapPos(float arg0, float arg1, float arg2, float arg3, Vec *arg4) {
    Vec local_position;
    float best_height;
    float local_x;
    float polygon_height;
    float local_z;
    HSFMAPATTR *map_attribute;
    HU3DMODEL *map_model;
    OMOBJ *map_object;
    s32 i;
    s32 j;
    HSFDATA *map_hsf;
    Mtx inverse_transpose;

    best_height = -100000.0f;
    ColisionCount = 0;
    for (i = 0; i < nMap; i++) {
        map_object = MapObject[i];
        map_model = &Hu3DData[*map_object->mdlId];
        map_hsf = map_model->hsf;
        local_position.x = arg0;
        local_position.y = arg1;
        local_position.z = arg2;
        CharRotInv(MapMT, MapMTR, &local_position, map_object);
        local_x = local_position.x;
        local_z = local_position.z;
        AttrHsf = map_hsf;
        map_attribute = AttrHsf->mapAttr;
        for (j = 0; j < map_hsf->mapAttrNum; j++, map_attribute++) {
            if (map_attribute->minX <= local_x && map_attribute->maxX >= local_x
                && map_attribute->minZ <= local_z && map_attribute->maxZ >= local_z
                && PolygonRangeCheck(map_attribute, local_x, local_z, &polygon_height, arg1 + arg3) == TRUE) {
                local_position.x = local_x;
                local_position.y = polygon_height;
                local_position.z = local_z;
                PSMTXMultVec(MapMT, &local_position, &local_position);
                polygon_height = local_position.y;
                if (polygon_height > arg1 + arg3 || fabs(arg1 - polygon_height) > fabs(arg1 - best_height)) {
                    continue;
                }
                best_height = polygon_height;
                arg4->x = FieldVec.x;
                arg4->y = FieldVec.y;
                arg4->z = FieldVec.z;
                PSMTXInvXpose(MapMT, inverse_transpose);
                PSMTXMultVec(inverse_transpose, arg4, arg4);
                best_height = local_position.y;
            }
        }
    }
    if (best_height == -100000.0f) {
        arg4->x = 0.0f;
        arg4->y = 1.0f;
        arg4->x = 0.0f;
        return arg1;
    } else {
        return best_height;
    }
}

/**
 * @brief Searches an attribute's packed polygons for the nearest surface below a point.
 * @param arg0 Map attribute containing packed polygon records.
 * @param arg1 Model-local X coordinate.
 * @param arg2 Model-local Z coordinate.
 * @param arg3 Destination for the selected model-local surface height.
 * @param arg4 World-space upper height limit used to choose the closest polygon.
 * @return TRUE when a qualifying triangle or quadrilateral is found; otherwise FALSE.
 */
BOOL PolygonRangeCheck(HSFMAPATTR *arg0, float arg1, float arg2, float *arg3, float arg4) {
    Vec world_point;
    float local_height;
    float closest_distance;
    u16 *polygon_data;
    u16 polygon_header;
    s32 bounded_polygon_count;
    s32 found_surface;
    s32 i;

    bounded_polygon_count = 0;
    found_surface = 0;
    closest_distance = 100000.0f;
    topvtx = AttrHsf->vertex->data;
    polygon_data = arg0->data;
    for (i = 0; i < arg0->dataLen;) {
        polygon_header = *polygon_data;
        if (polygon_header & 0x8000) {
            i += (polygon_header & 0xFF) + 1;
            polygon_data += (polygon_header & 0xFF) + 1;
        } else {
            switch (polygon_header & 0xFF) {
                case 1:
                    i += 2;
                    polygon_data += 2;
                    break;
                case 2:
                    i += 3;
                    polygon_data += 3;
                    break;
                case 3:
                    if (AreaCheck(arg1, arg2, polygon_data, topvtx) == TRUE) {
                        bounded_polygon_count++;
                        if (MapIflnnerTriangle(arg1, arg2, polygon_data, topvtx) == 1) {
                            local_height = MapCalcPoint(arg1, 0.0f, arg2, topvtx, polygon_data);
                            world_point.x = arg1;
                            world_point.y = local_height;
                            world_point.z = arg2;
                            PSMTXMultVec(MapMT, &world_point, &world_point);
                            if (arg4 > world_point.y && closest_distance > fabs(arg4 - world_point.y)) {
                                closest_distance = fabs(arg4 - world_point.y);
                                *arg3 = local_height;
                                found_surface = 1;
                            }
                        }
                    }
                    i += 4;
                    polygon_data += 4;
                    break;
                case 4:
                    if (AreaCheck(arg1, arg2, polygon_data, topvtx) == TRUE) {
                        bounded_polygon_count++;
                        if (MapIflnnerQuadrangle(arg1, arg2, polygon_data, topvtx) == 1) {
                            local_height = MapCalcPoint(arg1, 0.0f, arg2, topvtx, polygon_data);
                            world_point.x = arg1;
                            world_point.y = local_height;
                            world_point.z = arg2;
                            PSMTXMultVec(MapMT, &world_point, &world_point);
                            if (arg4 > world_point.y) {
                                if (closest_distance > fabs(arg4 - world_point.y)) {
                                    closest_distance = fabs(arg4 - world_point.y);
                                    *arg3 = local_height;
                                    found_surface = 1;
                                }
                            }
                        }
                    }
                    i += 5;
                    polygon_data += 5;
                    break;
                default:
                    i++;
                    polygon_data++;
                    break;
            }
        }
    }
    if (found_surface != 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * @brief Tests a point against the oriented edges of a packed triangle or quadrilateral.
 * @param arg0 Point to test in model space.
 * @param arg1 Packed polygon header followed by vertex indices.
 * @param arg2 Model vertex array.
 * @param arg3 Reference direction used to orient edge tests.
 * @return 1 when the point is inside a supported polygon; otherwise 0.
 */
static s32 DefIfnnerMapCircle(Vec *arg0, s16 *arg1, Vec *arg2, Vec *arg3) {
    float point_x;
    float point_y;
    float point_z;
    float edge_side;
    s32 edge_index;
    s32 vertex_count;
    s32 next_index;

    point_x = arg0->x;
    point_y = arg0->y;
    point_z = arg0->z;
    vertex_count = *arg1 & 0xFF;
    arg1++;
    if (vertex_count == 3) {
        edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[0]], &arg2[arg1[1]], arg3);
        if (edge_side > 0.0f) {
            for (edge_index = 1; edge_index < vertex_count; edge_index++) {
                next_index = (edge_index + 1) % vertex_count;
                edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[edge_index]], &arg2[arg1[next_index]], arg3);
                if (edge_side < 0.0f) {
                    return 0;
                }
            }
            return 1;
        } else {
            for (edge_index = 1; edge_index < vertex_count; edge_index++) {
                next_index = (edge_index + 1) % vertex_count;
                edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[edge_index]], &arg2[arg1[next_index]], arg3);
                if (edge_side > 0.0f) {
                    return 0;
                }
            }
            return 1;
        }
    } else if (vertex_count == 4) {
        edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[0]], &arg2[arg1[2]], arg3);
        if (edge_side > 0.0f) {
            edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[2]], &arg2[arg1[3]], arg3);
            if (edge_side < 0.0f) {
                vertex_count = 1;
            } else {
                edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[3]], &arg2[arg1[0]], arg3);
                if (edge_side < 0.0f) {
                    vertex_count = 1;
                }
            }
        } else {
            edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[2]], &arg2[arg1[3]], arg3);
            if (edge_side > 0.0f) {
                vertex_count = 1;
            } else {
                edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[3]], &arg2[arg1[0]], arg3);
                if (edge_side > 0.0f) {
                    vertex_count = 1;
                }
            }
        }
        if (vertex_count != 0) {
            edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[0]], &arg2[arg1[3]], arg3);
            if (edge_side > 0.0f) {
                edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[3]], &arg2[arg1[1]], arg3);
                if (edge_side < 0.0f) {
                    return 0;
                }
                edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[1]], &arg2[arg1[0]], arg3);
                if (edge_side < 0.0f) {
                    return 0;
                }
            } else {
                edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[3]], &arg2[arg1[1]], arg3);
                if (edge_side > 0.0f) {
                    return 0;
                }
                edge_side = MapIflnnerCalc(point_x, point_y, point_z, &arg2[arg1[1]], &arg2[arg1[0]], arg3);
                if (edge_side > 0.0f) {
                    return 0;
                }
            }
        }
    }
    return 0;
}

/**
 * @brief Normalizes a vector in place, leaving a zero-length vector unchanged.
 * @param vec Vector to normalize.
 */
static inline void normalize_vec(Vec *vec) {
    float x;
    float y;
    float z;
    float magnitude;

    x = vec->x;
    y = vec->y;
    z = vec->z;
    magnitude = sqrtf(x * x + y * y + z * z);
    if (magnitude != 0.0f) {
        vec->x /= magnitude;
        vec->y /= magnitude;
        vec->z /= magnitude;
    }
}

/**
 * @brief Calculates a unit normal from three triangle vertices.
 * @param result Destination for the normalized cross product.
 * @param point1 First triangle vertex.
 * @param point2 Second triangle vertex.
 * @param point3 Third triangle vertex.
 */
static inline void compute_tri_normal(Vec *result, Vec *point1, Vec *point2, Vec *point3) {
    float ux;
    float uy;
    float uz;

    float vx;
    float vy;
    float vz;

    ux = point2->x - point1->x;
    uy = point2->y - point1->y;
    uz = point2->z - point1->z;

    vx = point3->x - point1->x;
    vy = point3->y - point1->y;
    vz = point3->z - point1->z;

    result->x = uy * vz - uz * vy;
    result->y = uz * vx - ux * vz;
    result->z = ux * vy - uy * vx;
    normalize_vec(result);
}

/**
 * @brief Projects a sphere center onto a packed polygon plane when the sphere reaches that plane.
 * @param arg0 In/out sphere represented by center X, Y, Z, and radius.
 * @param arg1 Packed polygon header and vertex indices.
 * @param arg2 Model vertex array.
 * @return 0 when the record is not a wall or lies beyond the radius, 1 for the normal-facing side, or -1 for the opposite side.
 */
static s32 CalcPPLength(float *arg0, s16 *arg1, Vec *arg2) {
    Vec *plane_point;
    Vec normal;
    float projection_distance;
    float delta_x;
    float delta_y;
    float delta_z;
    float signed_distance;
    float plane_x;
    float plane_y;
    float plane_z;
    s16 polygon_header;
    s16 plane_vertex_index;
    s32 facing_side;

    facing_side = -1;
    polygon_header = arg1[0];
    if (!(polygon_header & 0x8000)) {
        return 0;
    }
    if ((polygon_header & 0xFF) == 4) {
        compute_tri_normal(&normal, &arg2[arg1[1]], &arg2[arg1[4]], &arg2[arg1[3]]);
    } else {
        compute_tri_normal(&normal, &arg2[arg1[1]], &arg2[arg1[2]], &arg2[arg1[3]]);
    }
    plane_vertex_index = arg1[1];
    plane_point = &arg2[plane_vertex_index];
    plane_x = plane_point->x;
    plane_y = plane_point->y;
    plane_z = plane_point->z;
    delta_x = plane_x - arg0[0];
    delta_y = plane_y - arg0[1];
    delta_z = plane_z - arg0[2];
    signed_distance = normal.x * delta_x + normal.y * delta_y + normal.z * delta_z;
    if (signed_distance >= 0.0f) {
        facing_side = 1;
    }
    if (fabs(signed_distance) > arg0[3]) {
        return 0;
    }
    projection_distance = normal.x * delta_x + normal.y * delta_y + normal.z * delta_z;
    arg0[0] += normal.x * projection_distance;
    arg0[1] += normal.y * projection_distance;
    arg0[2] += normal.z * projection_distance;
    return facing_side;
}

/**
 * @brief Calculates the signed scalar triple product for an oriented polygon edge.
 * @param arg0 Test-point X coordinate.
 * @param arg1 Test-point Y coordinate.
 * @param arg2 Test-point Z coordinate.
 * @param arg3 First edge vertex.
 * @param arg4 Second edge vertex.
 * @param arg5 Reference direction that defines the polygon-facing side.
 * @return Signed edge-side value; its sign indicates which side contains the test point.
 */
static float MapIflnnerCalc(float arg0, float arg1, float arg2, Vec *arg3, Vec *arg4, Vec *arg5) {
    float edge1_x;
    float edge1_y;
    float edge1_z;
    float edge2_x;
    float edge2_y;
    float edge2_z;
    float signed_side;

    edge1_x = arg3->x - arg0;
    edge1_y = arg3->y - arg1;
    edge1_z = arg3->z - arg2;
    edge2_x = arg4->x - arg0;
    edge2_y = arg4->y - arg1;
    edge2_z = arg4->z - arg2;
    signed_side = arg5->x * (edge1_y * edge2_z - edge1_z * edge2_y)
        + arg5->y * (edge1_z * edge2_x - edge1_x * edge2_z)
        + arg5->z * (edge1_x * edge2_y - edge1_y * edge2_x);
    return signed_side;
}

/**
 * @brief Solves the current collision triangle's plane equation for Y.
 * @param arg0 Model-local X coordinate.
 * @param arg1 Starting Y coordinate.
 * @param arg2 Model-local Z coordinate.
 * @param arg3 Model vertex array.
 * @param arg4 Packed polygon record; retained for calling compatibility.
 * @return Model-local Y coordinate on the current collision triangle.
 */
static float MapCalcPoint(float arg0, float arg1, float arg2, Vec *arg3, u16 *arg4) {
    Vec triangle_normal;
    float plane_z;
    float plane_y;
    float plane_x;
    float point_x;
    float point_z;
    float step_scale;
    float height_delta;
    float point_y;
    float normal_x;
    float normal_y;
    float normal_z;
    s32 collision_index;
    Vec *plane_point;

    collision_index = ColisionCount - 1;
    plane_point = &arg3[ColisionIdx[collision_index][0]];
    plane_x = plane_point->x;
    plane_y = plane_point->y;
    plane_z = plane_point->z;
    point_x = arg0;
    point_y = arg1;
    point_z = arg2;
    step_scale = 1.0f;
    compute_tri_normal(&triangle_normal, &arg3[ColisionIdx[collision_index][0]], &arg3[ColisionIdx[collision_index][1]], &arg3[ColisionIdx[collision_index][2]]);
    normal_x = triangle_normal.x;
    normal_y = triangle_normal.y;
    normal_z = triangle_normal.z;
    FieldVec.x = normal_x;
    FieldVec.y = normal_y;
    FieldVec.z = normal_z;
    height_delta = normal_x * (plane_x - point_x) + normal_y * (plane_y - point_y) + normal_z * (plane_z - point_z);
    height_delta /= normal_y;
    return point_y + step_scale * height_delta;
}

/**
 * @brief Tests an XZ point against the axis-aligned bounds of a packed polygon.
 * @param arg0 X coordinate to test.
 * @param arg1 Z coordinate to test.
 * @param arg2 Packed polygon header and vertex indices.
 * @param arg3 Model vertex array.
 * @return TRUE when the point lies within the inclusive XZ bounds; otherwise FALSE.
 */
static BOOL AreaCheck(float arg0, float arg1, u16 *arg2, Vec *arg3) {
    float max_x;
    float max_z;
    float min_x;
    float min_z;
    s32 vertex_count;
    s32 vertex_index;
    s32 i;

    max_x = max_z = -100000.0f;
    min_x = min_z = 100000.0f;
    vertex_count = *arg2 & 0xFF;
    arg2++;
    for (i = 0; i < vertex_count; i++, arg2++) {
        vertex_index = *arg2;
        if (min_x > arg3[vertex_index].x) {
            min_x = arg3[vertex_index].x;
        }
        if (max_x < arg3[vertex_index].x) {
            max_x = arg3[vertex_index].x;
        }
        if (min_z > arg3[vertex_index].z) {
            min_z = arg3[vertex_index].z;
        }
        if (max_z < arg3[vertex_index].z) {
            max_z = arg3[vertex_index].z;
        }
    }
    if (min_x <= arg0 && max_x >= arg0
        && min_z <= arg1 && max_z >= arg1) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * @brief Calculates the signed two-dimensional cross product of two XZ offsets.
 * @param x_org Shared origin X coordinate.
 * @param z_org Shared origin Z coordinate.
 * @param vec1 First endpoint.
 * @param vec2 Second endpoint.
 * @return Signed XZ cross product used for polygon winding tests.
 */
static inline float xz_scalar_cross_from_origin(float x_org, float z_org, Vec *vec1, Vec *vec2) {
    float vec1_dx;
    float vec1_dz;
    float vec2_dx;
    float vec2_dz;
    float cross_prod_2d;

    vec1_dx = vec1->x - x_org;
    vec1_dz = vec1->z - z_org;
    vec2_dx = vec2->x - x_org;
    vec2_dz = vec2->z - z_org;
    cross_prod_2d = -(vec1_dz * vec2_dx - vec1_dx * vec2_dz);
    return cross_prod_2d;
}

/**
 * @brief Tests whether an XZ point is inside a nonvertical triangle and records its vertices.
 * @param x_org Point X coordinate.
 * @param y_org Point Z coordinate.
 * @param arg2 Packed triangle header and vertex indices.
 * @param arg3 Model vertex array.
 * @return 1 when inside; otherwise 0.
 */
static s32 MapIflnnerTriangle(float x_org, float y_org, u16 *arg2, Vec *arg3) {
    Vec normal;
    float edge_side;
    s32 next_index;
    s32 i;

    compute_tri_normal(&normal, &arg3[arg2[1]], &arg3[arg2[2]], &arg3[arg2[3]]);
    if (normal.y == 0.0f) {
        return 0;
    }
    arg2++;
    edge_side = xz_scalar_cross_from_origin(x_org, y_org, &arg3[arg2[0]], &arg3[arg2[1]]);
    if (edge_side > 0.0f) {
        for (i = 1; i < 3; i++) {
            next_index = (i + 1) % 3;
            edge_side = xz_scalar_cross_from_origin(x_org, y_org, &arg3[arg2[i]], &arg3[arg2[next_index]]);
            if (edge_side < 0.0f) {
                return 0;
            }
        }
    } else {
        for (i = 1; i < 3; i++) {
            next_index = (i + 1) % 3;
            edge_side = xz_scalar_cross_from_origin(x_org, y_org, &arg3[arg2[i]], &arg3[arg2[next_index]]);
            if (edge_side > 0.0f) {
                return 0;
            }
        }
    }
    ColisionIdx[ColisionCount][0] = arg2[0];
    ColisionIdx[ColisionCount][1] = arg2[1];
    ColisionIdx[ColisionCount][2] = arg2[2];
    ColisionCount++;
    return 1;
}

/**
 * @brief Tests whether an XZ point is inside a quadrilateral and records the containing triangle.
 * @param arg0 Point X coordinate.
 * @param arg1 Point Z coordinate.
 * @param arg2 Packed quadrilateral header and vertex indices.
 * @param arg3 Model vertex array.
 * @return 1 when either component triangle contains the point; otherwise 0.
 */
static s32 MapIflnnerQuadrangle(float arg0, float arg1, u16 *arg2, Vec *arg3) {
    Vec normal;
    float edge_side;
    s32 use_second_triangle;

    compute_tri_normal(&normal, &arg3[arg2[1]], &arg3[arg2[2]], &arg3[arg2[3]]);
    if (normal.y == 0.0f) {
        return 0;
    }
    use_second_triangle = 0;
    arg2++;
    edge_side = xz_scalar_cross_from_origin(arg0, arg1, &arg3[arg2[0]], &arg3[arg2[3]]);
    if (edge_side > 0.0f) {
        edge_side = xz_scalar_cross_from_origin(arg0, arg1, &arg3[arg2[3]], &arg3[arg2[2]]);
        if (edge_side < 0.0f) {
            use_second_triangle = 1;
        } else {
            edge_side = xz_scalar_cross_from_origin(arg0, arg1, &arg3[arg2[2]], &arg3[arg2[0]]);
            if (edge_side < 0.0f) {
                use_second_triangle = 1;
            }
        }
    } else {
        edge_side = xz_scalar_cross_from_origin(arg0, arg1, &arg3[arg2[3]], &arg3[arg2[2]]);
        if (edge_side > 0.0f) {
            use_second_triangle = 1;
        } else {
            edge_side = xz_scalar_cross_from_origin(arg0, arg1, &arg3[arg2[2]], &arg3[arg2[0]]);
            if (edge_side > 0.0f) {
                use_second_triangle = 1;
            }
        }
    }
    if (use_second_triangle == 0) {
        ColisionIdx[ColisionCount][0] = arg2[0];
        ColisionIdx[ColisionCount][1] = arg2[3];
        ColisionIdx[ColisionCount][2] = arg2[2];
        ColisionCount++;
        return 1;
    }
    edge_side = xz_scalar_cross_from_origin(arg0, arg1, &arg3[arg2[0]], &arg3[arg2[1]]);
    if (edge_side > 0.0f) {
        edge_side = xz_scalar_cross_from_origin(arg0, arg1, &arg3[arg2[1]], &arg3[arg2[3]]);
        if (edge_side < 0.0f) {
            return 0;
        }
        edge_side = xz_scalar_cross_from_origin(arg0, arg1, &arg3[arg2[3]], &arg3[arg2[0]]);
        if (edge_side < 0.0f) {
            return 0;
        }
    } else {
        edge_side = xz_scalar_cross_from_origin(arg0, arg1, &arg3[arg2[1]], &arg3[arg2[3]]);
        if (edge_side > 0.0f) {
            return 0;
        }
        edge_side = xz_scalar_cross_from_origin(arg0, arg1, &arg3[arg2[3]], &arg3[arg2[0]]);
        if (edge_side > 0.0f) {
            return 0;
        }
    }
    ColisionIdx[ColisionCount][0] = arg2[0];
    ColisionIdx[ColisionCount][1] = arg2[1];
    ColisionIdx[ColisionCount][2] = arg2[3];
    ColisionCount++;
    return 1;
}

// I havent fully understood the call-site code for this function, so I am not totally certain of its use.
// However, it seems very likely this is checking for back-face culls. So I have named the function should_cull, and refered to the origin as camera.
// This function could be used for other things (eg checking for flipped normals), but I have named it for what I see as the most likely case.
/**
 * @brief Classifies a viewpoint against the oriented plane of a triangle.
 * @param camera Viewpoint coordinates as X, Y, and Z.
 * @param tri_indices Three indices into the vertex array.
 * @param vec Vertex array containing the triangle.
 * @return -1 when the viewpoint-facing dot product is negative; otherwise 1.
 */
static inline s32 should_cull(float *camera, s16 *tri_indices, Vec *vec) {
    Vec normal;
    Vec *plane_point;
    float x;
    float y;
    float z;
    float dot_prod;
    s16 plane_point_idx;

    compute_tri_normal(&normal, &vec[tri_indices[0]], &vec[tri_indices[1]], &vec[tri_indices[2]]);
    plane_point_idx = tri_indices[1];
    plane_point = &vec[plane_point_idx];
    x = plane_point->x;
    y = plane_point->y;
    z = plane_point->z;

    // Subtract viewpoint to change basis from absolute -> camera relative coordinate system
    x -= camera[0];
    y -= camera[1];
    z -= camera[2];

    dot_prod = normal.x * x + normal.y * y + normal.z * z; 
    return (dot_prod < 0.0f) ? -1 : 1;
}

/**
 * @brief Tests one packed wall polygon against a moving sphere and accumulates its XZ response.
 * @param arg0 Packed polygon header and vertex indices.
 * @param arg1 Model vertex array.
 * @param arg2 World-space sphere center and radius before model transformation.
 * @param arg3 Model-local sphere center and radius.
 * @return TRUE when the sphere intersects the polygon; otherwise FALSE.
 */
static BOOL GetPolygonCircleMtx(s16 *arg0, Vec *arg1, float *arg2, float *arg3) {
    Vec quad_vertices[4];
    Vec triangle_vertices[3];
    float projected_sphere[4];
    float world_sphere[4];
    float contact_delta_x;
    float contact_delta_z;
    float scale;
    Vec closest_point;
    Vec travel_direction;
    s32 unused_index;
    float contact_delta_y;
    s32 plane_side;
    BOOL intersects;
    s16 *vertex_indices;
    Vec *hit_normal;
    Mtx inverse_transpose;

    unused_index = 0;
    world_sphere[0] = projected_sphere[0] = arg3[0] + MTRAdd.x;
    world_sphere[1] = projected_sphere[1] = arg3[1];
    world_sphere[2] = projected_sphere[2] = arg3[2] + MTRAdd.z;
    world_sphere[3] = projected_sphere[3] = arg3[3];
    vertex_indices = arg0 + 1;
    if ((plane_side = CalcPPLength(projected_sphere, arg0, arg1)) == 0) {
        return 0;
    }
    closest_point.x = closest_point.y = closest_point.z = 0.0f;
    if ((arg0[0] & 0xFF) == 4) {
        quad_vertices[0].x = arg1[vertex_indices[0]].x;
        quad_vertices[0].y = arg1[vertex_indices[0]].y;
        quad_vertices[0].z = arg1[vertex_indices[0]].z;
        quad_vertices[1].x = arg1[vertex_indices[1]].x;
        quad_vertices[1].y = arg1[vertex_indices[1]].y;
        quad_vertices[1].z = arg1[vertex_indices[1]].z;
        quad_vertices[2].x = arg1[vertex_indices[2]].x;
        quad_vertices[2].y = arg1[vertex_indices[2]].y;
        quad_vertices[2].z = arg1[vertex_indices[2]].z;
        quad_vertices[3].x = arg1[vertex_indices[3]].x;
        quad_vertices[3].y = arg1[vertex_indices[3]].y;
        quad_vertices[3].z = arg1[vertex_indices[3]].z;
        intersects = Hitcheck_Quadrangle_with_Sphere(quad_vertices, (Vec*) world_sphere, projected_sphere[3], &closest_point);
    } else {
        triangle_vertices[0].x = arg1[vertex_indices[0]].x;
        triangle_vertices[0].y = arg1[vertex_indices[0]].y;
        triangle_vertices[0].z = arg1[vertex_indices[0]].z;
        triangle_vertices[1].x = arg1[vertex_indices[1]].x;
        triangle_vertices[1].y = arg1[vertex_indices[1]].y;
        triangle_vertices[1].z = arg1[vertex_indices[1]].z;
        triangle_vertices[2].x = arg1[vertex_indices[2]].x;
        triangle_vertices[2].y = arg1[vertex_indices[2]].y;
        triangle_vertices[2].z = arg1[vertex_indices[2]].z;
        intersects = Hitcheck_Triangle_with_Sphere(triangle_vertices, (Vec*) world_sphere, projected_sphere[3], &closest_point);
    }
    if (intersects == TRUE) {
        world_sphere[0] = arg2[0] + AddX;
        world_sphere[1] = arg2[1];
        world_sphere[2] = arg2[2] + AddZ;
        PSMTXMultVec(MapMT, &closest_point, &closest_point);
        DefSetHitFace(closest_point.x, closest_point.y, closest_point.z);
        hit_normal = &HitFaceVec[HitFaceCount];
#ifdef NON_MATCHING
        compute_tri_normal(hit_normal, &arg1[arg0[0] & 0x7FFF], &arg1[arg0[1]], &arg1[arg0[2]]);
#else
        compute_tri_normal(hit_normal, &arg1[arg0[0]], &arg1[arg0[1]], &arg1[arg0[2]]);
#endif
        contact_delta_x = closest_point.x - world_sphere[0];
        contact_delta_y = closest_point.y - world_sphere[1];
        contact_delta_z = closest_point.z - world_sphere[2];
        scale = projected_sphere[3] - sqrtf(contact_delta_x * contact_delta_x + contact_delta_z * contact_delta_z);
        HitFaceCount++;
        if (plane_side > 0) {
            projected_sphere[0] = OldXYZ.x;
            projected_sphere[1] = OldXYZ.y;
            projected_sphere[2] = OldXYZ.z;
            PSMTXMultVec(MapMTR, (Vec*) &projected_sphere, (Vec*) &projected_sphere);
            if (should_cull(projected_sphere, vertex_indices, arg1) < 0) {
                travel_direction.x = projected_sphere[0] - world_sphere[0];
                travel_direction.y = projected_sphere[1] - world_sphere[1];
                travel_direction.z = projected_sphere[2] - world_sphere[2];
                normalize_vec(&travel_direction);
                if (DefIfnnerMapCircle((Vec*) world_sphere, arg0 - 1, arg1, &travel_direction) == 1) {
                    scale = projected_sphere[3] + sqrtf(contact_delta_x * contact_delta_x + contact_delta_z * contact_delta_z);
                }
            } else {
                scale = 0.0f;
            }
        }
        if (scale > 0.0f) {
            AppendAddXZ(-contact_delta_x, -contact_delta_z, scale);
            MTRAdd.x = AddX;
            MTRAdd.z = AddZ;
            MTRAdd.y = 0.0f;
            PSMTXInvXpose(MapMT, inverse_transpose);
            PSMTXMultVec(inverse_transpose, &MTRAdd, &MTRAdd);
        }
    }
    return intersects;
}

/**
 * @brief Calculates the closest point offset on a triangle expressed from a shared origin.
 * @param arg0 Triangle origin; retained for the final point reconstruction by callers.
 * @param arg1 First triangle edge from the origin.
 * @param arg2 Second triangle edge from the origin.
 * @param arg3 Triangle normal.
 * @param arg4 Point offset from the triangle origin.
 * @param arg5 Destination for the closest offset on the triangle or one of its edges.
 * @return 1 after writing the closest offset.
 */
static s32 PrecalcPntToTriangle(Vec *arg0, Vec *arg1, Vec *arg2, Vec* arg3, Vec *arg4, Vec *arg5) {
    Vec negated_normal;
    Vec edge_offset;
    float inverse_determinant;
    float edge1_weight;
    float edge2_weight;
    float edge_fraction;

    HuSetVecF(&negated_normal, -arg4->x, -arg4->y, -arg4->z);
    inverse_determinant = 1.0f / (-(arg1->z * arg2->y * arg3->x) + arg1->y * arg2->z * arg3->x + arg1->z * arg2->x * arg3->y - arg1->x * arg2->z * arg3->y - arg1->y * arg2->x * arg3->z + arg1->x * arg2->y * arg3->z);
    edge1_weight = inverse_determinant * (arg2->z * (arg3->y * negated_normal.x - arg3->x * negated_normal.y) + arg2->y * (arg3->x * negated_normal.z - arg3->z * negated_normal.x) + arg2->x * (arg3->z * negated_normal.y - arg3->y * negated_normal.z));
    edge2_weight = inverse_determinant * (arg1->z * (arg3->x * negated_normal.y - arg3->y * negated_normal.x) + arg1->y * (arg3->z * negated_normal.x - arg3->x * negated_normal.z) + arg1->x * (arg3->y * negated_normal.z - arg3->z * negated_normal.y));
    if (edge1_weight > 0.0f && edge2_weight > 0.0f && edge1_weight + edge2_weight > 1.0f) {
        VECSubtract(arg2, arg1, &negated_normal);
        VECSubtract(arg4, arg1, &edge_offset);
        edge_fraction = VECDotProduct(&negated_normal, &edge_offset) / VECDotProduct(&negated_normal, &negated_normal);
        if (edge_fraction <= 0.0f) {
            arg5 = arg1;
        } else {
            if (edge_fraction >= 1.0f) {
                arg5 = arg2;
            } else {
                VECScale(&negated_normal, &edge_offset, edge_fraction);
                VECAdd(arg1, &edge_offset, arg5);
            }
        }
    } else if (edge2_weight < 0.0f) {
        edge_fraction = VECDotProduct(arg1, arg4) / VECDotProduct(arg1, arg1);
        if (edge_fraction <= 0.0f) {
            HuSetVecF(arg5, 0.0, 0.0, 0.0);
        } else {
            if (edge_fraction >= 1.0f) {
                arg5 = arg1;
            } else {
                VECScale(arg1, arg5, edge_fraction);
            }
        }
    } else if (edge1_weight < 0.0f) {
        edge_fraction = VECDotProduct(arg2, arg4) / VECDotProduct(arg2, arg2);
        if (edge_fraction <= 0.0f) {
            HuSetVecF(arg5, 0.0, 0.0, 0.0);
        } else {
            if (edge_fraction >= 1.0f) {
                arg5 = arg2;
            } else {
                VECScale(arg2, arg5, edge_fraction);
            }
        }
    } else {
        HuSetVecF(arg5, edge1_weight * arg1->x + edge2_weight * arg2->x, edge1_weight * arg1->y + edge2_weight * arg2->y, edge1_weight * arg1->z + edge2_weight * arg2->z);
    }
    return 1;
}

/**
 * @brief Tests a triangle against a sphere using the triangle's closest point to the sphere center.
 * @param triangle Array of three triangle vertices.
 * @param sphere_origin Sphere center in the same coordinate space.
 * @param sphere_radius Sphere radius in coordinate units.
 * @param closest_point Destination for the closest point on the triangle.
 * @return TRUE when the closest-point distance is no greater than the radius; otherwise FALSE.
 */
BOOL Hitcheck_Triangle_with_Sphere(Vec *triangle, Vec *sphere_origin, float sphere_radius, Vec *closest_point) {
    Vec tri_point;
    Vec ux;
    Vec vx;
    Vec normal;
    Vec triangle_to_sphere; // Distance between the (arbitrary?) point on triangle, and the origin of the sphere.
    Vec offset_to_closest_point;
    float dist;

    tri_point.x = triangle[0].x;
    tri_point.y = triangle[0].y;
    tri_point.z = triangle[0].z;
    VECSubtract(&triangle[1], &triangle[0], &ux);
    VECSubtract(&triangle[2], &triangle[0], &vx);
    VECCrossProduct(&ux, &vx, &normal);
    VECSubtract(sphere_origin, &triangle[0], &triangle_to_sphere);
    PrecalcPntToTriangle(&tri_point, &ux, &vx, &normal, &triangle_to_sphere, &offset_to_closest_point);
    VECAdd(&offset_to_closest_point, &tri_point, closest_point);
    dist = VECDistance(closest_point, sphere_origin);
    if (dist > sphere_radius) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/**
 * @brief Tests a quadrilateral, split into two triangles, against a sphere.
 * @param arg0 Array of four quadrilateral vertices.
 * @param arg1 Sphere center in the same coordinate space.
 * @param arg2 Sphere radius in coordinate units.
 * @param arg3 Destination for the closer point from the two component triangles.
 * @return TRUE when the closest-point distance is no greater than the radius; otherwise FALSE.
 */
BOOL Hitcheck_Quadrangle_with_Sphere(Vec *arg0, Vec *arg1, float arg2, Vec *arg3) {
    Vec quad_origin;
    Vec diagonal_edge;
    Vec third_edge;
    Vec second_edge;
    Vec normal;
    Vec sphere_offset;
    Vec closest_offset;
    Vec first_closest_point;
    Vec second_closest_point;
    float second_distance;
    float closest_distance;

    quad_origin.x = arg0->x;
    quad_origin.y = arg0->y;
    quad_origin.z = arg0->z;
    VECSubtract(&arg0[2], &arg0[0], &diagonal_edge);
    VECSubtract(&arg0[3], &arg0[0], &third_edge);
    VECSubtract(&arg0[1], &arg0[0], &second_edge);
    VECCrossProduct(&diagonal_edge, &third_edge, &normal);
    VECSubtract(arg1, &arg0[0], &sphere_offset);
    PrecalcPntToTriangle(&quad_origin, &diagonal_edge, &third_edge, &normal, &sphere_offset, &closest_offset);
    VECAdd(&closest_offset, &quad_origin, &first_closest_point);
    PrecalcPntToTriangle(&quad_origin, &third_edge, &second_edge, &normal, &sphere_offset, &closest_offset);
    VECAdd(&closest_offset, &quad_origin, &second_closest_point);
    closest_distance = VECDistance(&first_closest_point, arg1);
    second_distance = VECDistance(&second_closest_point, arg1);
    if (second_distance > closest_distance) {
        arg3->x = first_closest_point.x;
        arg3->y = first_closest_point.y;
        arg3->z = first_closest_point.z;
    } else {
        closest_distance = second_distance;
        arg3->x = second_closest_point.x;
        arg3->y = second_closest_point.y;
        arg3->z = second_closest_point.z;
    }
    if (closest_distance > arg2) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/**
 * @brief Stores the current collision contact point in the hit-face array.
 * @param arg0 Contact X coordinate.
 * @param arg1 Contact Y coordinate.
 * @param arg2 Contact Z coordinate.
 */
static void DefSetHitFace(float arg0, float arg1, float arg2) {
    HitFace[HitFaceCount].x = arg0;
    HitFace[HitFaceCount].y = arg1;
    HitFace[HitFaceCount].z = arg2;
}

// floats x_comp and z_comp are essentially a direction vector (x_comp, 0, z_comp)
/**
 * @brief Adds a normalized horizontal collision response to AddX and AddZ.
 * @param x_comp Response-direction X component.
 * @param z_comp Response-direction Z component.
 * @param scale Response distance in coordinate units.
 */
void AppendAddXZ(float x_comp, float z_comp, float scale) {
    Vec dir_vec;

    dir_vec.x = x_comp;
    dir_vec.y = 0.0f;
    dir_vec.z = z_comp;
    normalize_vec(&dir_vec);
    AddX += dir_vec.x * scale;
    AddZ += dir_vec.z * scale;
}

/**
 * @brief Builds an object's rotation/translation matrix and transforms a point into object space.
 * @param arg0 Destination for the object-to-world matrix.
 * @param arg1 Destination for its inverse matrix.
 * @param arg2 Point transformed in place from world space to object space.
 * @param arg3 Map object supplying translation and Euler rotation in degrees.
 */
void CharRotInv(Mtx arg0, Mtx arg1, Vec *arg2, OMOBJ *arg3) {
    Mtx rotation_matrix;

    PSMTXTrans(arg0, arg3->trans.x, arg3->trans.y, arg3->trans.z);
    if (arg3->rot.z) {
        PSMTXRotRad(rotation_matrix, 'z', MTXDegToRad(arg3->rot.z));
        PSMTXConcat(arg0, rotation_matrix, arg0);
    }
    if (arg3->rot.y) {
        PSMTXRotRad(rotation_matrix, 'y', MTXDegToRad(arg3->rot.y));
        PSMTXConcat(arg0, rotation_matrix, arg0);
    }
    if (arg3->rot.x) {
        PSMTXRotRad(rotation_matrix, 'x', MTXDegToRad(arg3->rot.x));
        PSMTXConcat(arg0, rotation_matrix, arg0);
    }
    PSMTXInverse(arg0, arg1);
    PSMTXMultVec(arg1, arg2, arg2);
}
