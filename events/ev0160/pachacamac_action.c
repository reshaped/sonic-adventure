/* ev0160_pachacamac_action and its motion, and the texture names of
 * ev0160_emerald_blue_texlist */
#include <ninja.h>

#include "ev0160/pachacamac_action.h"

#define TEXT_1_LENGTH 13
#include "ev0160/pachacamac_action/text.inc"

extern NJS_OBJECT dat_0cba7420;
extern NJS_MDATA2 dat_0cbaee88[];

static NJS_MOTION ev0160_pachacamac_motion = {
#include "ev0160/pachacamac_action/ev0160_pachacamac_motion.inc"
};
NJS_ACTION ev0160_pachacamac_action = {
#include "ev0160/pachacamac_action/ev0160_pachacamac_action.inc"
};
static NJS_TEXNAME ev0160_emerald_blue_textures[1] = {
    {TEXT_1, 0, 0},
};
