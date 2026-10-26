/* sonic_fidget_action and its motion, then the models of Sonic's dash, his
 * ball and the ice. the ice is the block ice_create puts a player in. each
 * model follows a table of its texture's name and that table's texlist. a
 * fourth table, of the ice's texture again, and its texlist end the data.
 * nothing reads the dash's, the ball's or the fourth texlist */
#include <ninja.h>
#include <sg_xpt.h>

#include "player/sonic_effect_models.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 8
#include "player/sonic_effect_models/text.inc"

extern NJS_OBJECT sonic_object;
extern NJS_MDATA2 dat_8c26b748[];

static NJS_MOTION sonic_fidget_motion = {
#include "player/sonic_effect_models/sonic_fidget_motion.inc"
};
NJS_ACTION sonic_fidget_action = {
#include "player/sonic_effect_models/sonic_fidget_action.inc"
};
static NJS_TEXNAME sonic_dash_textures[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST sonic_dash_texlist = {
#include "player/sonic_effect_models/sonic_dash_texlist.inc"
};
static NJS_MATERIAL dat_8c26bc00[1] = {
#include "player/sonic_effect_models/dat_8c26bc00.inc"
};
static Sint16 dat_8c26bc14[15] = {
#include "player/sonic_effect_models/dat_8c26bc14.inc"
};
static NJS_COLOR dat_8c26bc34[14] = {
#include "player/sonic_effect_models/dat_8c26bc34.inc"
};
static NJS_MESHSET dat_8c26bc6c[1] = {
#include "player/sonic_effect_models/dat_8c26bc6c.inc"
};
static NJS_POINT3 dat_8c26bc84[12] = {
#include "player/sonic_effect_models/dat_8c26bc84.inc"
};
static NJS_VECTOR dat_8c26bd14[12] = {
#include "player/sonic_effect_models/dat_8c26bd14.inc"
};
static NJS_MODEL dat_8c26bda4 = {
#include "player/sonic_effect_models/dat_8c26bda4.inc"
};
NJS_OBJECT sonic_dash_object = {
#include "player/sonic_effect_models/sonic_dash_object.inc"
};
static NJS_TEXNAME sonic_ball_textures[1] = {
    {TEXT_2, 0, 0},
};
static NJS_TEXLIST sonic_ball_texlist = {
#include "player/sonic_effect_models/sonic_ball_texlist.inc"
};
static NJS_MATERIAL dat_8c26be14[1] = {
#include "player/sonic_effect_models/dat_8c26be14.inc"
};
static Sint16 dat_8c26be28[44] = {
#include "player/sonic_effect_models/dat_8c26be28.inc"
};
static Sint16 dat_8c26be80[160] = {
#include "player/sonic_effect_models/dat_8c26be80.inc"
};
static NJS_COLOR dat_8c26bfc0[36] = {
#include "player/sonic_effect_models/dat_8c26bfc0.inc"
};
static NJS_COLOR dat_8c26c050[160] = {
#include "player/sonic_effect_models/dat_8c26c050.inc"
};
static NJS_MESHSET dat_8c26c2d0[2] = {
#include "player/sonic_effect_models/dat_8c26c2d0.inc"
};
static NJS_POINT3 dat_8c26c300[52] = {
#include "player/sonic_effect_models/dat_8c26c300.inc"
};
static NJS_VECTOR dat_8c26c570[52] = {
#include "player/sonic_effect_models/dat_8c26c570.inc"
};
static NJS_MODEL dat_8c26c7e0 = {
#include "player/sonic_effect_models/dat_8c26c7e0.inc"
};
NJS_OBJECT sonic_ball_object = {
#include "player/sonic_effect_models/sonic_ball_object.inc"
};
static NJS_TEXNAME sonic_ice_textures[1] = {
    {TEXT_3, 0, 0},
};
NJS_TEXLIST sonic_ice_texlist = {
#include "player/sonic_effect_models/sonic_ice_texlist.inc"
};
static NJS_MATERIAL dat_8c26c850[1] = {
#include "player/sonic_effect_models/dat_8c26c850.inc"
};
static Sint16 dat_8c26c864[102] = {
#include "player/sonic_effect_models/dat_8c26c864.inc"
};
static Sint16 dat_8c26c930[28] = {
#include "player/sonic_effect_models/dat_8c26c930.inc"
};
static NJS_COLOR dat_8c26c968[102] = {
#include "player/sonic_effect_models/dat_8c26c968.inc"
};
static NJS_COLOR dat_8c26cb00[28] = {
#include "player/sonic_effect_models/dat_8c26cb00.inc"
};
static NJS_MESHSET dat_8c26cb70[2] = {
#include "player/sonic_effect_models/dat_8c26cb70.inc"
};
static NJS_POINT3 dat_8c26cba0[26] = {
#include "player/sonic_effect_models/dat_8c26cba0.inc"
};
static NJS_VECTOR dat_8c26ccd8[26] = {
#include "player/sonic_effect_models/dat_8c26ccd8.inc"
};
static NJS_MODEL dat_8c26ce10 = {
#include "player/sonic_effect_models/dat_8c26ce10.inc"
};
NJS_OBJECT sonic_ice_object = {
#include "player/sonic_effect_models/sonic_ice_object.inc"
};
static NJS_TEXNAME dat_8c26ce6c[1] = {
    {TEXT_3, 0, 0},
};
static NJS_TEXLIST dat_8c26ce78 = {
#include "player/sonic_effect_models/dat_8c26ce78.inc"
};
