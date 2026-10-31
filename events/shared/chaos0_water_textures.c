/* the texlist of Chaos 0's puddle and the name of its one texture. the
 * material belongs to event_chaos0_puddle_object, the puddle's model, in the
 * generated data of each event that links it */
#include <ninja.h>

#include "shared/chaos0_water_textures.h"

#define TEXT_1_LENGTH 11
#include "shared/chaos0_water_textures/text.inc"

static NJS_TEXNAME event_chaos0_water_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST event_chaos0_water_texlist = {
#include "shared/chaos0_water_textures/event_chaos0_water_texlist.inc"
};
NJS_MATERIAL event_chaos0_water_material[1] = {
#include "shared/chaos0_water_textures/event_chaos0_water_material.inc"
};
