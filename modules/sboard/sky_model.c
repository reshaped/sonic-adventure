/* the sky's object, the names of its textures in BG_SANDBOARD, and their
 * texlist */
#include <ninja.h>
#include <sg_xpt.h>

#include "sboard/sky_model.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#include "sboard/sky_model/text.inc"

static NJS_MATERIAL dat_0c97adb8[2] = {
#include "sboard/sky_model/dat_0c97adb8.inc"
};
static Sint16 dat_0c97ade0[120] = {
#include "sboard/sky_model/dat_0c97ade0.inc"
};
static Sint16 dat_0c97aed0[144] = {
#include "sboard/sky_model/dat_0c97aed0.inc"
};
static NJS_COLOR dat_0c97aff0[120] = {
#include "sboard/sky_model/dat_0c97aff0.inc"
};
static NJS_COLOR dat_0c97b1d0[144] = {
#include "sboard/sky_model/dat_0c97b1d0.inc"
};
static NJS_MESHSET dat_0c97b410[2] = {
#include "sboard/sky_model/dat_0c97b410.inc"
};
static NJS_POINT3 dat_0c97b440[49] = {
#include "sboard/sky_model/dat_0c97b440.inc"
};
static NJS_VECTOR dat_0c97b68c[49] = {
#include "sboard/sky_model/dat_0c97b68c.inc"
};
static NJS_MODEL dat_0c97b8d8 = {
#include "sboard/sky_model/dat_0c97b8d8.inc"
};
NJS_OBJECT sunaboard_sky_object = {
#include "sboard/sky_model/sunaboard_sky_object.inc"
};
static NJS_TEXNAME sunaboard_sky_textures[2] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
};
NJS_TEXLIST sunaboard_sky_texlist = {
#include "sboard/sky_model/sunaboard_sky_texlist.inc"
};
