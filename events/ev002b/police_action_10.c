/* ev002b_police_action_10 and ev002b_bullet_texlist, with a motion, texture
 * names, materials and strips */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev002b/police_action_10.h"

#define TEXT_1_LENGTH 5
#define TEXT_2_LENGTH 5
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 5
#include "ev002b/police_action_10/text.inc"

extern NJS_OBJECT dat_0cb8feb4;
extern NJS_MDATA2 dat_0cbac630[];

static NJS_MOTION ev002b_police_motion_10 = {
#include "ev002b/police_action_10/ev002b_police_motion_10.inc"
};
NJS_ACTION ev002b_police_action_10 = {
#include "ev002b/police_action_10/ev002b_police_action_10.inc"
};
static NJS_TEXNAME ev002b_bullet_textures[4] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
    {TEXT_4, 0, 0},
};
NJS_TEXLIST ev002b_bullet_texlist = {
#include "ev002b/police_action_10/ev002b_bullet_texlist.inc"
};
static NJS_MATERIAL dat_0cbacb8c[4] = {
#include "ev002b/police_action_10/dat_0cbacb8c.inc"
};
static Sint16 dat_0cbacbdc[15] = {
#include "ev002b/police_action_10/dat_0cbacbdc.inc"
};
static Sint16 dat_0cbacbfa[7] = {
#include "ev002b/police_action_10/dat_0cbacbfa.inc"
};
