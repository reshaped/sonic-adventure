/* ev00f3_yellow_emerald_object, then the texture name of
 * event_chaos0_water_texlist */
#include <ninja.h>

#include "ev00f3/yellow_emerald_object.h"

#define TEXT_1_LENGTH 11
#include "ev00f3/yellow_emerald_object/text.inc"

extern NJS_OBJECT dat_0cb9d84c;

NJS_OBJECT ev00f3_yellow_emerald_object = {
#include "ev00f3/yellow_emerald_object/ev00f3_yellow_emerald_object.inc"
};
NJS_TEXNAME event_chaos0_water_textures[1] = {
    {TEXT_1, 0, 0},
};
