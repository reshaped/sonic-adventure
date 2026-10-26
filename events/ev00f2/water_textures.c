/* event_chaos0_water_texlist, with its texture name */
#include <ninja.h>

#include "ev00f2/water_textures.h"

#define TEXT_1_LENGTH 11
#include "ev00f2/water_textures/text.inc"

static NJS_TEXNAME ev00f2_water_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST event_chaos0_water_texlist = {
#include "ev00f2/water_textures/event_chaos0_water_texlist.inc"
};
