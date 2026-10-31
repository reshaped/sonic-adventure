/* Knuckles' last action in EV0080's script, and its motion. two models of
 * four nodes follow, each after a texlist of one texture, mr_064s_emegrenb.
 * no word in EV0080 or 1ST_READ.BIN points at either texlist or either
 * model's root. the clouds' three texture names and event_clouds_texlist
 * come next. the file ends on the one texture name of
 * event_chaos0_water_texlist, a texlist that starts the data after it */
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/clouds.h"

#include "ev0080/knuckles_action_10.h"

#define TEXT_1_LENGTH 16
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 11
#include "ev0080/knuckles_action_10/text.inc"

extern NJS_MDATA2 dat_0cba4848[];
extern NJS_OBJECT knuckles_object;

static NJS_MOTION ev0080_knuckles_motion_10 = {
#include "ev0080/knuckles_action_10/ev0080_knuckles_motion_10.inc"
};
NJS_ACTION ev0080_knuckles_action_10 = {
#include "ev0080/knuckles_action_10/ev0080_knuckles_action_10.inc"
};
static NJS_TEXNAME dat_0cba4cec[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cba4cf8 = {
#include "ev0080/knuckles_action_10/dat_0cba4cf8.inc"
};
static NJS_MATERIAL dat_0cba4d00[1] = {
#include "ev0080/knuckles_action_10/dat_0cba4d00.inc"
};
static Sint16 dat_0cba4d14[12] = {
#include "ev0080/knuckles_action_10/dat_0cba4d14.inc"
};
static Sint16 dat_0cba4d2c[60] = {
#include "ev0080/knuckles_action_10/dat_0cba4d2c.inc"
};
static NJS_COLOR dat_0cba4da4[12] = {
#include "ev0080/knuckles_action_10/dat_0cba4da4.inc"
};
static NJS_COLOR dat_0cba4dd4[60] = {
#include "ev0080/knuckles_action_10/dat_0cba4dd4.inc"
};
static NJS_MESHSET dat_0cba4ec4[2] = {
#include "ev0080/knuckles_action_10/dat_0cba4ec4.inc"
};
static NJS_POINT3 dat_0cba4ef4[33] = {
#include "ev0080/knuckles_action_10/dat_0cba4ef4.inc"
};
static NJS_VECTOR dat_0cba5080[33] = {
#include "ev0080/knuckles_action_10/dat_0cba5080.inc"
};
static NJS_MODEL dat_0cba520c = {
#include "ev0080/knuckles_action_10/dat_0cba520c.inc"
};
static NJS_OBJECT dat_0cba5234 = {
#include "ev0080/knuckles_action_10/dat_0cba5234.inc"
};
static NJS_MATERIAL dat_0cba5268[1] = {
#include "ev0080/knuckles_action_10/dat_0cba5268.inc"
};
static Sint16 dat_0cba527c[48] = {
#include "ev0080/knuckles_action_10/dat_0cba527c.inc"
};
static NJS_COLOR dat_0cba52dc[48] = {
#include "ev0080/knuckles_action_10/dat_0cba52dc.inc"
};
static NJS_MESHSET dat_0cba539c[1] = {
#include "ev0080/knuckles_action_10/dat_0cba539c.inc"
};
static NJS_POINT3 dat_0cba53b4[24] = {
#include "ev0080/knuckles_action_10/dat_0cba53b4.inc"
};
static NJS_VECTOR dat_0cba54d4[24] = {
#include "ev0080/knuckles_action_10/dat_0cba54d4.inc"
};
static NJS_MODEL dat_0cba55f4 = {
#include "ev0080/knuckles_action_10/dat_0cba55f4.inc"
};
static NJS_OBJECT dat_0cba561c = {
#include "ev0080/knuckles_action_10/dat_0cba561c.inc"
};
static NJS_MATERIAL dat_0cba5650[1] = {
#include "ev0080/knuckles_action_10/dat_0cba5650.inc"
};
static Sint16 dat_0cba5664[12] = {
#include "ev0080/knuckles_action_10/dat_0cba5664.inc"
};
static Sint16 dat_0cba567c[16] = {
#include "ev0080/knuckles_action_10/dat_0cba567c.inc"
};
static NJS_COLOR dat_0cba569c[12] = {
#include "ev0080/knuckles_action_10/dat_0cba569c.inc"
};
static NJS_COLOR dat_0cba56cc[16] = {
#include "ev0080/knuckles_action_10/dat_0cba56cc.inc"
};
static NJS_MESHSET dat_0cba570c[2] = {
#include "ev0080/knuckles_action_10/dat_0cba570c.inc"
};
static NJS_POINT3 dat_0cba573c[17] = {
#include "ev0080/knuckles_action_10/dat_0cba573c.inc"
};
static NJS_VECTOR dat_0cba5808[17] = {
#include "ev0080/knuckles_action_10/dat_0cba5808.inc"
};
static NJS_MODEL dat_0cba58d4 = {
#include "ev0080/knuckles_action_10/dat_0cba58d4.inc"
};
static NJS_OBJECT dat_0cba58fc = {
#include "ev0080/knuckles_action_10/dat_0cba58fc.inc"
};
static NJS_MATERIAL dat_0cba5930[1] = {
#include "ev0080/knuckles_action_10/dat_0cba5930.inc"
};
static Sint16 dat_0cba5944[4] = {
#include "ev0080/knuckles_action_10/dat_0cba5944.inc"
};
static NJS_MESHSET dat_0cba594c[1] = {
#include "ev0080/knuckles_action_10/dat_0cba594c.inc"
};
static NJS_POINT3 dat_0cba5964[3] = {
#include "ev0080/knuckles_action_10/dat_0cba5964.inc"
};
static NJS_VECTOR dat_0cba5988[3] = {
#include "ev0080/knuckles_action_10/dat_0cba5988.inc"
};
static NJS_MODEL dat_0cba59ac = {
#include "ev0080/knuckles_action_10/dat_0cba59ac.inc"
};
static NJS_OBJECT dat_0cba59d4 = {
#include "ev0080/knuckles_action_10/dat_0cba59d4.inc"
};
static NJS_TEXNAME dat_0cba5a08[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cba5a14 = {
#include "ev0080/knuckles_action_10/dat_0cba5a14.inc"
};
static NJS_MATERIAL dat_0cba5a1c[1] = {
#include "ev0080/knuckles_action_10/dat_0cba5a1c.inc"
};
static Sint16 dat_0cba5a30[126] = {
#include "ev0080/knuckles_action_10/dat_0cba5a30.inc"
};
static NJS_COLOR dat_0cba5b2c[126] = {
#include "ev0080/knuckles_action_10/dat_0cba5b2c.inc"
};
static NJS_MESHSET dat_0cba5d24[1] = {
#include "ev0080/knuckles_action_10/dat_0cba5d24.inc"
};
static NJS_POINT3 dat_0cba5d3c[43] = {
#include "ev0080/knuckles_action_10/dat_0cba5d3c.inc"
};
static NJS_VECTOR dat_0cba5f40[43] = {
#include "ev0080/knuckles_action_10/dat_0cba5f40.inc"
};
static NJS_MODEL dat_0cba6144 = {
#include "ev0080/knuckles_action_10/dat_0cba6144.inc"
};
static NJS_OBJECT dat_0cba616c = {
#include "ev0080/knuckles_action_10/dat_0cba616c.inc"
};
static NJS_MATERIAL dat_0cba61a0[1] = {
#include "ev0080/knuckles_action_10/dat_0cba61a0.inc"
};
static Sint16 dat_0cba61b4[48] = {
#include "ev0080/knuckles_action_10/dat_0cba61b4.inc"
};
static NJS_COLOR dat_0cba6214[48] = {
#include "ev0080/knuckles_action_10/dat_0cba6214.inc"
};
static NJS_MESHSET dat_0cba62d4[1] = {
#include "ev0080/knuckles_action_10/dat_0cba62d4.inc"
};
static NJS_POINT3 dat_0cba62ec[24] = {
#include "ev0080/knuckles_action_10/dat_0cba62ec.inc"
};
static NJS_VECTOR dat_0cba640c[24] = {
#include "ev0080/knuckles_action_10/dat_0cba640c.inc"
};
static NJS_MODEL dat_0cba652c = {
#include "ev0080/knuckles_action_10/dat_0cba652c.inc"
};
static NJS_OBJECT dat_0cba6554 = {
#include "ev0080/knuckles_action_10/dat_0cba6554.inc"
};
static NJS_MATERIAL dat_0cba6588[1] = {
#include "ev0080/knuckles_action_10/dat_0cba6588.inc"
};
static Sint16 dat_0cba659c[36] = {
#include "ev0080/knuckles_action_10/dat_0cba659c.inc"
};
static NJS_COLOR dat_0cba65e4[36] = {
#include "ev0080/knuckles_action_10/dat_0cba65e4.inc"
};
static NJS_MESHSET dat_0cba6674[1] = {
#include "ev0080/knuckles_action_10/dat_0cba6674.inc"
};
static NJS_POINT3 dat_0cba668c[17] = {
#include "ev0080/knuckles_action_10/dat_0cba668c.inc"
};
static NJS_VECTOR dat_0cba6758[17] = {
#include "ev0080/knuckles_action_10/dat_0cba6758.inc"
};
static NJS_MODEL dat_0cba6824 = {
#include "ev0080/knuckles_action_10/dat_0cba6824.inc"
};
static NJS_OBJECT dat_0cba684c = {
#include "ev0080/knuckles_action_10/dat_0cba684c.inc"
};
static NJS_MATERIAL dat_0cba6880[1] = {
#include "ev0080/knuckles_action_10/dat_0cba6880.inc"
};
static Sint16 dat_0cba6894[4] = {
#include "ev0080/knuckles_action_10/dat_0cba6894.inc"
};
static NJS_MESHSET dat_0cba689c[1] = {
#include "ev0080/knuckles_action_10/dat_0cba689c.inc"
};
static NJS_POINT3 dat_0cba68b4[3] = {
#include "ev0080/knuckles_action_10/dat_0cba68b4.inc"
};
static NJS_VECTOR dat_0cba68d8[3] = {
#include "ev0080/knuckles_action_10/dat_0cba68d8.inc"
};
static NJS_MODEL dat_0cba68fc = {
#include "ev0080/knuckles_action_10/dat_0cba68fc.inc"
};
static NJS_OBJECT dat_0cba6924 = {
#include "ev0080/knuckles_action_10/dat_0cba6924.inc"
};
static NJS_TEXNAME event_clouds_textures[3] = {
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
    {TEXT_4, 0, 0},
};
NJS_TEXLIST event_clouds_texlist = {
#include "ev0080/knuckles_action_10/event_clouds_texlist.inc"
};
NJS_TEXNAME event_chaos0_water_textures[1] = {
    {TEXT_5, 0, 0},
};
