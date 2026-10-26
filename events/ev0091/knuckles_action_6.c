/* EV0091's action for knuckles_object, and its motion. a model of four
 * nodes follows, after a texlist of one texture, mr_064s_emegrenb. no word
 * in EV0091 or 1ST_READ.BIN points at the texlist or the model's root. the
 * file ends on the texture name of a second such texlist. that texlist
 * starts the data after this file */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0091/knuckles_action_6.h"

#define TEXT_1_LENGTH 16
#include "ev0091/knuckles_action_6/text.inc"

extern NJS_MDATA2 dat_0cbaa028[];
extern NJS_OBJECT knuckles_object;

static NJS_MOTION ev0091_knuckles_motion_6 = {
#include "ev0091/knuckles_action_6/ev0091_knuckles_motion_6.inc"
};
NJS_ACTION ev0091_knuckles_action_6 = {
#include "ev0091/knuckles_action_6/ev0091_knuckles_action_6.inc"
};
static NJS_TEXNAME dat_0cbaa4cc[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cbaa4d8 = {
#include "ev0091/knuckles_action_6/dat_0cbaa4d8.inc"
};
static NJS_MATERIAL dat_0cbaa4e0[1] = {
#include "ev0091/knuckles_action_6/dat_0cbaa4e0.inc"
};
static Sint16 dat_0cbaa4f4[126] = {
#include "ev0091/knuckles_action_6/dat_0cbaa4f4.inc"
};
static NJS_COLOR dat_0cbaa5f0[126] = {
#include "ev0091/knuckles_action_6/dat_0cbaa5f0.inc"
};
static NJS_MESHSET dat_0cbaa7e8[1] = {
#include "ev0091/knuckles_action_6/dat_0cbaa7e8.inc"
};
static NJS_POINT3 dat_0cbaa800[43] = {
#include "ev0091/knuckles_action_6/dat_0cbaa800.inc"
};
static NJS_VECTOR dat_0cbaaa04[43] = {
#include "ev0091/knuckles_action_6/dat_0cbaaa04.inc"
};
static NJS_MODEL dat_0cbaac08 = {
#include "ev0091/knuckles_action_6/dat_0cbaac08.inc"
};
static NJS_OBJECT dat_0cbaac30 = {
#include "ev0091/knuckles_action_6/dat_0cbaac30.inc"
};
static NJS_MATERIAL dat_0cbaac64[1] = {
#include "ev0091/knuckles_action_6/dat_0cbaac64.inc"
};
static Sint16 dat_0cbaac78[48] = {
#include "ev0091/knuckles_action_6/dat_0cbaac78.inc"
};
static NJS_COLOR dat_0cbaacd8[48] = {
#include "ev0091/knuckles_action_6/dat_0cbaacd8.inc"
};
static NJS_MESHSET dat_0cbaad98[1] = {
#include "ev0091/knuckles_action_6/dat_0cbaad98.inc"
};
static NJS_POINT3 dat_0cbaadb0[24] = {
#include "ev0091/knuckles_action_6/dat_0cbaadb0.inc"
};
static NJS_VECTOR dat_0cbaaed0[24] = {
#include "ev0091/knuckles_action_6/dat_0cbaaed0.inc"
};
static NJS_MODEL dat_0cbaaff0 = {
#include "ev0091/knuckles_action_6/dat_0cbaaff0.inc"
};
static NJS_OBJECT dat_0cbab018 = {
#include "ev0091/knuckles_action_6/dat_0cbab018.inc"
};
static NJS_MATERIAL dat_0cbab04c[1] = {
#include "ev0091/knuckles_action_6/dat_0cbab04c.inc"
};
static Sint16 dat_0cbab060[12] = {
#include "ev0091/knuckles_action_6/dat_0cbab060.inc"
};
static Sint16 dat_0cbab078[16] = {
#include "ev0091/knuckles_action_6/dat_0cbab078.inc"
};
static NJS_COLOR dat_0cbab098[12] = {
#include "ev0091/knuckles_action_6/dat_0cbab098.inc"
};
static NJS_COLOR dat_0cbab0c8[16] = {
#include "ev0091/knuckles_action_6/dat_0cbab0c8.inc"
};
static NJS_MESHSET dat_0cbab108[2] = {
#include "ev0091/knuckles_action_6/dat_0cbab108.inc"
};
static NJS_POINT3 dat_0cbab138[17] = {
#include "ev0091/knuckles_action_6/dat_0cbab138.inc"
};
static NJS_VECTOR dat_0cbab204[17] = {
#include "ev0091/knuckles_action_6/dat_0cbab204.inc"
};
static NJS_MODEL dat_0cbab2d0 = {
#include "ev0091/knuckles_action_6/dat_0cbab2d0.inc"
};
static NJS_OBJECT dat_0cbab2f8 = {
#include "ev0091/knuckles_action_6/dat_0cbab2f8.inc"
};
static NJS_MATERIAL dat_0cbab32c[1] = {
#include "ev0091/knuckles_action_6/dat_0cbab32c.inc"
};
static Sint16 dat_0cbab340[4] = {
#include "ev0091/knuckles_action_6/dat_0cbab340.inc"
};
static NJS_MESHSET dat_0cbab348[1] = {
#include "ev0091/knuckles_action_6/dat_0cbab348.inc"
};
static NJS_POINT3 dat_0cbab360[3] = {
#include "ev0091/knuckles_action_6/dat_0cbab360.inc"
};
static NJS_VECTOR dat_0cbab384[3] = {
#include "ev0091/knuckles_action_6/dat_0cbab384.inc"
};
static NJS_MODEL dat_0cbab3a8 = {
#include "ev0091/knuckles_action_6/dat_0cbab3a8.inc"
};
static NJS_OBJECT dat_0cbab3d0 = {
#include "ev0091/knuckles_action_6/dat_0cbab3d0.inc"
};
static NJS_TEXNAME dat_0cbab404[1] = {
    {TEXT_1, 0, 0},
};
