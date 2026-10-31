/* a texlist of snb01, and the material trail_object's model draws with */
#include <ninja.h>

#include "stg08/trail_model_textures.h"

#define TEXT_1_LENGTH 5
#include "stg08/trail_model_textures/text.inc"

static NJS_TEXNAME trail_model_textures[1] = {
    {TEXT_1, 0, 0},
};
/* unreferenced: no word in the image or in 1ST_READ.BIN points at it */
static NJS_TEXLIST trail_model_texlist = {
#include "stg08/trail_model_textures/trail_model_texlist.inc"
};
NJS_MATERIAL dat_0ca51214[1] = {
#include "stg08/trail_model_textures/dat_0ca51214.inc"
};
