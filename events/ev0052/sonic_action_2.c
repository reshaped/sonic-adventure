/* ev0052_sonic_action_2 and its motion, and the names of the clouds'
 * textures, which event_clouds_texlist points at */
#include <ninja.h>

#include "ev0052/sonic_action_2.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#include "ev0052/sonic_action_2/text.inc"

extern NJS_MDATA2 ev0052_sonic_motion_2_mdata[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev0052_sonic_motion_2 = {
#include "ev0052/sonic_action_2/ev0052_sonic_motion_2.inc"
};
NJS_ACTION ev0052_sonic_action_2 = {
#include "ev0052/sonic_action_2/ev0052_sonic_action_2.inc"
};
static NJS_TEXNAME event_clouds_textures[3] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
