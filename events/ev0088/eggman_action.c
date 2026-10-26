/* EV0088's action for eggman_object and its motion, then the texture name
 * of event_chaos0_water_texlist. that texlist starts the data after this
 * file */
#include <ninja.h>

#include "ev0088/eggman_action.h"

#define TEXT_1_LENGTH 11
#include "ev0088/eggman_action/text.inc"

extern NJS_MDATA2 dat_0cb993c0[];
extern NJS_OBJECT eggman_object;

static NJS_MOTION ev0088_eggman_motion = {
#include "ev0088/eggman_action/ev0088_eggman_motion.inc"
};
NJS_ACTION ev0088_eggman_action = {
#include "ev0088/eggman_action/ev0088_eggman_action.inc"
};
static NJS_TEXNAME event_chaos0_water_textures[1] = {
    {TEXT_1, 0, 0},
};
