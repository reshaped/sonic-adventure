/* the sky's object, the names of its textures in BG_SHAREOBJ, and their
 * texlist */
#include <ninja.h>
#include <sg_xpt.h>

#include "minicart/sky_model.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#include "minicart/sky_model/text.inc"

static NJS_MATERIAL dat_0ca3c390[2] = {
#include "minicart/sky_model/dat_0ca3c390.inc"
};
static Sint16 dat_0ca3c3b8[40] = {
#include "minicart/sky_model/dat_0ca3c3b8.inc"
};
static Sint16 dat_0ca3c408[88] = {
#include "minicart/sky_model/dat_0ca3c408.inc"
};
static NJS_COLOR dat_0ca3c4b8[32] = {
#include "minicart/sky_model/dat_0ca3c4b8.inc"
};
static NJS_COLOR dat_0ca3c538[80] = {
#include "minicart/sky_model/dat_0ca3c538.inc"
};
static NJS_MESHSET dat_0ca3c678[2] = {
#include "minicart/sky_model/dat_0ca3c678.inc"
};
static NJS_POINT3 dat_0ca3c6a8[42] = {
#include "minicart/sky_model/dat_0ca3c6a8.inc"
};
static NJS_VECTOR dat_0ca3c8a0[42] = {
#include "minicart/sky_model/dat_0ca3c8a0.inc"
};
static NJS_MODEL dat_0ca3ca98 = {
#include "minicart/sky_model/dat_0ca3ca98.inc"
};
NJS_OBJECT cart_sky_object = {
#include "minicart/sky_model/cart_sky_object.inc"
};
static NJS_TEXNAME cart_sky_textures[2] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
};
NJS_TEXLIST cart_sky_texlist = {
#include "minicart/sky_model/cart_sky_texlist.inc"
};
