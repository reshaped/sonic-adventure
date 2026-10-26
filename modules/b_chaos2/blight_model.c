/* the model blight.c draws */
#include <ninja.h>

#include "b_chaos2/blight_model.h"
#include <sg_xpt.h>

NJS_MATERIAL blight_materials[1] = {
#include "b_chaos2/blight_model/blight_materials.inc"
};
static Sint16 blight_meshes0[64] = {
#include "b_chaos2/blight_model/blight_meshes0.inc"
};
static NJS_COLOR blight_uvs0[64] = {
#include "b_chaos2/blight_model/blight_uvs0.inc"
};
static NJS_MESHSET blight_meshsets[1] = {
#include "b_chaos2/blight_model/blight_meshsets.inc"
};
static NJS_POINT3 blight_points[32] = {
#include "b_chaos2/blight_model/blight_points.inc"
};
static NJS_VECTOR blight_normals[32] = {
#include "b_chaos2/blight_model/blight_normals.inc"
};
static NJS_MODEL blight_model = {
#include "b_chaos2/blight_model/blight_model.inc"
};
NJS_OBJECT blight_object = {
#include "b_chaos2/blight_model/blight_object.inc"
};
