/* the emeralds' texture list, model and action, the seven emeralds' actions
 * as they gather about Sonic, and the texture names of Amy's bird */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00ff/emerald_textures.h"

#define TEXT_1_LENGTH 16
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 9
#include "ev00ff/emerald_textures/text.inc"

static NJS_TEXNAME ev00ff_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev00ff_emerald_texlist = {
#include "ev00ff/emerald_textures/ev00ff_emerald_texlist.inc"
};
static NJS_MATERIAL dat_0cb98cdc[2] = {
#include "ev00ff/emerald_textures/dat_0cb98cdc.inc"
};
static Sint16 dat_0cb98d04[24] = {
#include "ev00ff/emerald_textures/dat_0cb98d04.inc"
};
static Sint16 dat_0cb98d34[68] = {
#include "ev00ff/emerald_textures/dat_0cb98d34.inc"
};
static NJS_COLOR dat_0cb98dbc[24] = {
#include "ev00ff/emerald_textures/dat_0cb98dbc.inc"
};
static NJS_COLOR dat_0cb98e1c[68] = {
#include "ev00ff/emerald_textures/dat_0cb98e1c.inc"
};
static NJS_MESHSET dat_0cb98f2c[2] = {
#include "ev00ff/emerald_textures/dat_0cb98f2c.inc"
};
static NJS_POINT3 dat_0cb98f5c[37] = {
#include "ev00ff/emerald_textures/dat_0cb98f5c.inc"
};
static NJS_VECTOR dat_0cb99118[37] = {
#include "ev00ff/emerald_textures/dat_0cb99118.inc"
};
static NJS_MODEL dat_0cb992d4 = {
#include "ev00ff/emerald_textures/dat_0cb992d4.inc"
};
static NJS_OBJECT dat_0cb992fc = {
#include "ev00ff/emerald_textures/dat_0cb992fc.inc"
};
static NJS_MATERIAL dat_0cb99330[1] = {
#include "ev00ff/emerald_textures/dat_0cb99330.inc"
};
static Sint16 dat_0cb99344[48] = {
#include "ev00ff/emerald_textures/dat_0cb99344.inc"
};
static NJS_COLOR dat_0cb993a4[48] = {
#include "ev00ff/emerald_textures/dat_0cb993a4.inc"
};
static NJS_MESHSET dat_0cb99464[1] = {
#include "ev00ff/emerald_textures/dat_0cb99464.inc"
};
static NJS_POINT3 dat_0cb9947c[24] = {
#include "ev00ff/emerald_textures/dat_0cb9947c.inc"
};
static NJS_VECTOR dat_0cb9959c[24] = {
#include "ev00ff/emerald_textures/dat_0cb9959c.inc"
};
static NJS_MODEL dat_0cb996bc = {
#include "ev00ff/emerald_textures/dat_0cb996bc.inc"
};
static NJS_OBJECT dat_0cb996e4 = {
#include "ev00ff/emerald_textures/dat_0cb996e4.inc"
};
static NJS_MATERIAL dat_0cb99718[1] = {
#include "ev00ff/emerald_textures/dat_0cb99718.inc"
};
static Sint16 dat_0cb9972c[12] = {
#include "ev00ff/emerald_textures/dat_0cb9972c.inc"
};
static Sint16 dat_0cb99744[16] = {
#include "ev00ff/emerald_textures/dat_0cb99744.inc"
};
static NJS_COLOR dat_0cb99764[12] = {
#include "ev00ff/emerald_textures/dat_0cb99764.inc"
};
static NJS_COLOR dat_0cb99794[16] = {
#include "ev00ff/emerald_textures/dat_0cb99794.inc"
};
static NJS_MESHSET dat_0cb997d4[2] = {
#include "ev00ff/emerald_textures/dat_0cb997d4.inc"
};
static NJS_POINT3 dat_0cb99804[17] = {
#include "ev00ff/emerald_textures/dat_0cb99804.inc"
};
static NJS_VECTOR dat_0cb998d0[17] = {
#include "ev00ff/emerald_textures/dat_0cb998d0.inc"
};
static NJS_MODEL dat_0cb9999c = {
#include "ev00ff/emerald_textures/dat_0cb9999c.inc"
};
static NJS_OBJECT dat_0cb999c4 = {
#include "ev00ff/emerald_textures/dat_0cb999c4.inc"
};
static NJS_MATERIAL dat_0cb999f8[1] = {
#include "ev00ff/emerald_textures/dat_0cb999f8.inc"
};
static Sint16 dat_0cb99a0c[4] = {
#include "ev00ff/emerald_textures/dat_0cb99a0c.inc"
};
static NJS_MESHSET dat_0cb99a14[1] = {
#include "ev00ff/emerald_textures/dat_0cb99a14.inc"
};
static NJS_POINT3 dat_0cb99a2c[3] = {
#include "ev00ff/emerald_textures/dat_0cb99a2c.inc"
};
static NJS_VECTOR dat_0cb99a50[3] = {
#include "ev00ff/emerald_textures/dat_0cb99a50.inc"
};
static NJS_MODEL dat_0cb99a74 = {
#include "ev00ff/emerald_textures/dat_0cb99a74.inc"
};
static NJS_OBJECT dat_0cb99a9c = {
#include "ev00ff/emerald_textures/dat_0cb99a9c.inc"
};
static NJS_OBJECT dat_0cb99ad0 = {
#include "ev00ff/emerald_textures/dat_0cb99ad0.inc"
};
static NJS_MDATA2 dat_0cb99b04[5] = {
#include "ev00ff/emerald_textures/dat_0cb99b04.inc"
};
static NJS_MOTION ev00ff_emerald_motion = {
#include "ev00ff/emerald_textures/ev00ff_emerald_motion.inc"
};
NJS_ACTION ev00ff_emerald_action = {
#include "ev00ff/emerald_textures/ev00ff_emerald_action.inc"
};
static NJS_MKEY_F dat_0cb99b68[29] = {
#include "ev00ff/emerald_textures/dat_0cb99b68.inc"
};
static NJS_MKEY_A dat_0cb99d38[2] = {
#include "ev00ff/emerald_textures/dat_0cb99d38.inc"
};
static NJS_MKEY_F dat_0cb99d58[30] = {
#include "ev00ff/emerald_textures/dat_0cb99d58.inc"
};
static NJS_MKEY_A dat_0cb99f38[2] = {
#include "ev00ff/emerald_textures/dat_0cb99f38.inc"
};
static NJS_MKEY_A dat_0cb99f58[2] = {
#include "ev00ff/emerald_textures/dat_0cb99f58.inc"
};
static NJS_MKEY_A dat_0cb99f78[2] = {
#include "ev00ff/emerald_textures/dat_0cb99f78.inc"
};
static NJS_MKEY_A dat_0cb99f98[2] = {
#include "ev00ff/emerald_textures/dat_0cb99f98.inc"
};
static NJS_MDATA2 dat_0cb99fb8[5] = {
#include "ev00ff/emerald_textures/dat_0cb99fb8.inc"
};
static NJS_MOTION ev00ff_gather_motion_0 = {
#include "ev00ff/emerald_textures/ev00ff_gather_motion_0.inc"
};
NJS_ACTION ev00ff_gather_action_0 = {
#include "ev00ff/emerald_textures/ev00ff_gather_action_0.inc"
};
static NJS_MKEY_A dat_0cb9a01c[2] = {
#include "ev00ff/emerald_textures/dat_0cb9a01c.inc"
};
static NJS_MKEY_F dat_0cb9a03c[30] = {
#include "ev00ff/emerald_textures/dat_0cb9a03c.inc"
};
static NJS_MKEY_A dat_0cb9a21c[29] = {
#include "ev00ff/emerald_textures/dat_0cb9a21c.inc"
};
static NJS_MKEY_A dat_0cb9a3ec[2] = {
#include "ev00ff/emerald_textures/dat_0cb9a3ec.inc"
};
static NJS_MKEY_A dat_0cb9a40c[2] = {
#include "ev00ff/emerald_textures/dat_0cb9a40c.inc"
};
static NJS_MKEY_A dat_0cb9a42c[2] = {
#include "ev00ff/emerald_textures/dat_0cb9a42c.inc"
};
static NJS_MDATA2 dat_0cb9a44c[5] = {
#include "ev00ff/emerald_textures/dat_0cb9a44c.inc"
};
static NJS_MOTION ev00ff_gather_motion_1 = {
#include "ev00ff/emerald_textures/ev00ff_gather_motion_1.inc"
};
NJS_ACTION ev00ff_gather_action_1 = {
#include "ev00ff/emerald_textures/ev00ff_gather_action_1.inc"
};
static NJS_MKEY_A dat_0cb9a4b0[2] = {
#include "ev00ff/emerald_textures/dat_0cb9a4b0.inc"
};
static NJS_MKEY_F dat_0cb9a4d0[30] = {
#include "ev00ff/emerald_textures/dat_0cb9a4d0.inc"
};
static NJS_MKEY_A dat_0cb9a6b0[29] = {
#include "ev00ff/emerald_textures/dat_0cb9a6b0.inc"
};
static NJS_MKEY_A dat_0cb9a880[2] = {
#include "ev00ff/emerald_textures/dat_0cb9a880.inc"
};
static NJS_MKEY_A dat_0cb9a8a0[2] = {
#include "ev00ff/emerald_textures/dat_0cb9a8a0.inc"
};
static NJS_MKEY_A dat_0cb9a8c0[2] = {
#include "ev00ff/emerald_textures/dat_0cb9a8c0.inc"
};
static NJS_MDATA2 dat_0cb9a8e0[5] = {
#include "ev00ff/emerald_textures/dat_0cb9a8e0.inc"
};
static NJS_MOTION ev00ff_gather_motion_2 = {
#include "ev00ff/emerald_textures/ev00ff_gather_motion_2.inc"
};
NJS_ACTION ev00ff_gather_action_2 = {
#include "ev00ff/emerald_textures/ev00ff_gather_action_2.inc"
};
static NJS_MKEY_A dat_0cb9a944[2] = {
#include "ev00ff/emerald_textures/dat_0cb9a944.inc"
};
static NJS_MKEY_F dat_0cb9a964[30] = {
#include "ev00ff/emerald_textures/dat_0cb9a964.inc"
};
static NJS_MKEY_A dat_0cb9ab44[30] = {
#include "ev00ff/emerald_textures/dat_0cb9ab44.inc"
};
static NJS_MKEY_A dat_0cb9ad24[2] = {
#include "ev00ff/emerald_textures/dat_0cb9ad24.inc"
};
static NJS_MKEY_A dat_0cb9ad44[2] = {
#include "ev00ff/emerald_textures/dat_0cb9ad44.inc"
};
static NJS_MKEY_A dat_0cb9ad64[2] = {
#include "ev00ff/emerald_textures/dat_0cb9ad64.inc"
};
static NJS_MDATA2 dat_0cb9ad84[5] = {
#include "ev00ff/emerald_textures/dat_0cb9ad84.inc"
};
static NJS_MOTION ev00ff_gather_motion_3 = {
#include "ev00ff/emerald_textures/ev00ff_gather_motion_3.inc"
};
NJS_ACTION ev00ff_gather_action_3 = {
#include "ev00ff/emerald_textures/ev00ff_gather_action_3.inc"
};
static NJS_MKEY_A dat_0cb9ade8[2] = {
#include "ev00ff/emerald_textures/dat_0cb9ade8.inc"
};
static NJS_MKEY_F dat_0cb9ae08[30] = {
#include "ev00ff/emerald_textures/dat_0cb9ae08.inc"
};
static NJS_MKEY_A dat_0cb9afe8[29] = {
#include "ev00ff/emerald_textures/dat_0cb9afe8.inc"
};
static NJS_MKEY_A dat_0cb9b1b8[2] = {
#include "ev00ff/emerald_textures/dat_0cb9b1b8.inc"
};
static NJS_MKEY_A dat_0cb9b1d8[2] = {
#include "ev00ff/emerald_textures/dat_0cb9b1d8.inc"
};
static NJS_MKEY_A dat_0cb9b1f8[2] = {
#include "ev00ff/emerald_textures/dat_0cb9b1f8.inc"
};
static NJS_MDATA2 dat_0cb9b218[5] = {
#include "ev00ff/emerald_textures/dat_0cb9b218.inc"
};
static NJS_MOTION ev00ff_gather_motion_4 = {
#include "ev00ff/emerald_textures/ev00ff_gather_motion_4.inc"
};
NJS_ACTION ev00ff_gather_action_4 = {
#include "ev00ff/emerald_textures/ev00ff_gather_action_4.inc"
};
static NJS_MKEY_A dat_0cb9b27c[2] = {
#include "ev00ff/emerald_textures/dat_0cb9b27c.inc"
};
static NJS_MKEY_F dat_0cb9b29c[30] = {
#include "ev00ff/emerald_textures/dat_0cb9b29c.inc"
};
static NJS_MKEY_A dat_0cb9b47c[29] = {
#include "ev00ff/emerald_textures/dat_0cb9b47c.inc"
};
static NJS_MKEY_A dat_0cb9b64c[2] = {
#include "ev00ff/emerald_textures/dat_0cb9b64c.inc"
};
static NJS_MKEY_A dat_0cb9b66c[2] = {
#include "ev00ff/emerald_textures/dat_0cb9b66c.inc"
};
static NJS_MKEY_A dat_0cb9b68c[2] = {
#include "ev00ff/emerald_textures/dat_0cb9b68c.inc"
};
static NJS_MDATA2 dat_0cb9b6ac[5] = {
#include "ev00ff/emerald_textures/dat_0cb9b6ac.inc"
};
static NJS_MOTION ev00ff_gather_motion_5 = {
#include "ev00ff/emerald_textures/ev00ff_gather_motion_5.inc"
};
NJS_ACTION ev00ff_gather_action_5 = {
#include "ev00ff/emerald_textures/ev00ff_gather_action_5.inc"
};
static NJS_MKEY_A dat_0cb9b710[2] = {
#include "ev00ff/emerald_textures/dat_0cb9b710.inc"
};
static NJS_MKEY_F dat_0cb9b730[30] = {
#include "ev00ff/emerald_textures/dat_0cb9b730.inc"
};
static NJS_MKEY_A dat_0cb9b910[29] = {
#include "ev00ff/emerald_textures/dat_0cb9b910.inc"
};
static NJS_MKEY_A dat_0cb9bae0[2] = {
#include "ev00ff/emerald_textures/dat_0cb9bae0.inc"
};
static NJS_MKEY_A dat_0cb9bb00[2] = {
#include "ev00ff/emerald_textures/dat_0cb9bb00.inc"
};
static NJS_MKEY_A dat_0cb9bb20[2] = {
#include "ev00ff/emerald_textures/dat_0cb9bb20.inc"
};
static NJS_MDATA2 dat_0cb9bb40[5] = {
#include "ev00ff/emerald_textures/dat_0cb9bb40.inc"
};
static NJS_MOTION ev00ff_gather_motion_6 = {
#include "ev00ff/emerald_textures/ev00ff_gather_motion_6.inc"
};
NJS_ACTION ev00ff_gather_action_6 = {
#include "ev00ff/emerald_textures/ev00ff_gather_action_6.inc"
};
static NJS_TEXNAME ev00ff_bird_textures[7] = {
    {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0}, {TEXT_5, 0, 0},
    {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
