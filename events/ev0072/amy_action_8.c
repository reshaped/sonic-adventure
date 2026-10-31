/* Amy's action 8, and the birds' models, actions and texlists. script.c
 * draws ev0072_bird_object with VER2_WING's textures and
 * ev0072_bird_ver4_object with VER4_WING's, as two models of Amy's bird.
 * script.c draws ev0072_wing_object as the bird's two siblings, one with
 * WING_P's textures and one with WING_T's. nothing in EV0072.PRS or
 * 1ST_READ.BIN points at dat_0cba71e4, dat_0cbafec8, dat_0cbb16cc or
 * dat_0cbb9128. dat_0cba71e4 lists the same 12 texture names as the bird's
 * two texlists. dat_0cbafec8 and dat_0cbb16cc animate dat_0cbab26c, a model
 * laid out item for item as the bird's two are. dat_0cbb9128 animates
 * ev0072_bird_ver4_object, as ev0072_bird_ver4_action_0, _1 and _2 do */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0072/amy_action_8.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 7
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 9
#include "ev0072/amy_action_8/text.inc"

extern NJS_MDATA2 dat_0cba6c60[];
extern NJS_OBJECT amy_object;

static NJS_MOTION ev0072_amy_motion_8 = {
#include "ev0072/amy_action_8/ev0072_amy_motion_8.inc"
};
NJS_ACTION ev0072_amy_action_8 = {
#include "ev0072/amy_action_8/ev0072_amy_action_8.inc"
};
static NJS_TEXNAME dat_0cba7154[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
static NJS_TEXLIST dat_0cba71e4 = {
#include "ev0072/amy_action_8/dat_0cba71e4.inc"
};
static NJS_MATERIAL dat_0cba71ec[1] = {
#include "ev0072/amy_action_8/dat_0cba71ec.inc"
};
static Sint16 dat_0cba7200[6] = {
#include "ev0072/amy_action_8/dat_0cba7200.inc"
};
static NJS_COLOR dat_0cba720c[4] = {
#include "ev0072/amy_action_8/dat_0cba720c.inc"
};
static NJS_MESHSET dat_0cba721c[1] = {
#include "ev0072/amy_action_8/dat_0cba721c.inc"
};
static NJS_POINT3 dat_0cba7234[4] = {
#include "ev0072/amy_action_8/dat_0cba7234.inc"
};
static NJS_VECTOR dat_0cba7264[4] = {
#include "ev0072/amy_action_8/dat_0cba7264.inc"
};
static NJS_MODEL dat_0cba7294 = {
#include "ev0072/amy_action_8/dat_0cba7294.inc"
};
static NJS_OBJECT dat_0cba72bc = {
#include "ev0072/amy_action_8/dat_0cba72bc.inc"
};
static NJS_MATERIAL dat_0cba72f0[1] = {
#include "ev0072/amy_action_8/dat_0cba72f0.inc"
};
static Sint16 dat_0cba7304[12] = {
#include "ev0072/amy_action_8/dat_0cba7304.inc"
};
static NJS_COLOR dat_0cba731c[9] = {
#include "ev0072/amy_action_8/dat_0cba731c.inc"
};
static NJS_MESHSET dat_0cba7340[1] = {
#include "ev0072/amy_action_8/dat_0cba7340.inc"
};
static NJS_POINT3 dat_0cba7358[7] = {
#include "ev0072/amy_action_8/dat_0cba7358.inc"
};
static NJS_VECTOR dat_0cba73ac[7] = {
#include "ev0072/amy_action_8/dat_0cba73ac.inc"
};
static NJS_MODEL dat_0cba7400 = {
#include "ev0072/amy_action_8/dat_0cba7400.inc"
};
static NJS_OBJECT dat_0cba7428 = {
#include "ev0072/amy_action_8/dat_0cba7428.inc"
};
static NJS_OBJECT dat_0cba745c = {
#include "ev0072/amy_action_8/dat_0cba745c.inc"
};
static NJS_OBJECT dat_0cba7490 = {
#include "ev0072/amy_action_8/dat_0cba7490.inc"
};
static NJS_OBJECT dat_0cba74c4 = {
#include "ev0072/amy_action_8/dat_0cba74c4.inc"
};
static NJS_OBJECT dat_0cba74f8 = {
#include "ev0072/amy_action_8/dat_0cba74f8.inc"
};
static NJS_OBJECT dat_0cba752c = {
#include "ev0072/amy_action_8/dat_0cba752c.inc"
};
static NJS_OBJECT dat_0cba7560 = {
#include "ev0072/amy_action_8/dat_0cba7560.inc"
};
static NJS_MATERIAL dat_0cba7594[1] = {
#include "ev0072/amy_action_8/dat_0cba7594.inc"
};
static Sint16 dat_0cba75a8[6] = {
#include "ev0072/amy_action_8/dat_0cba75a8.inc"
};
static NJS_COLOR dat_0cba75b4[4] = {
#include "ev0072/amy_action_8/dat_0cba75b4.inc"
};
static NJS_MESHSET dat_0cba75c4[1] = {
#include "ev0072/amy_action_8/dat_0cba75c4.inc"
};
static NJS_POINT3 dat_0cba75dc[4] = {
#include "ev0072/amy_action_8/dat_0cba75dc.inc"
};
static NJS_VECTOR dat_0cba760c[4] = {
#include "ev0072/amy_action_8/dat_0cba760c.inc"
};
static NJS_MODEL dat_0cba763c = {
#include "ev0072/amy_action_8/dat_0cba763c.inc"
};
static NJS_OBJECT dat_0cba7664 = {
#include "ev0072/amy_action_8/dat_0cba7664.inc"
};
static NJS_MATERIAL dat_0cba7698[1] = {
#include "ev0072/amy_action_8/dat_0cba7698.inc"
};
static Sint16 dat_0cba76ac[12] = {
#include "ev0072/amy_action_8/dat_0cba76ac.inc"
};
static NJS_COLOR dat_0cba76c4[9] = {
#include "ev0072/amy_action_8/dat_0cba76c4.inc"
};
static NJS_MESHSET dat_0cba76e8[1] = {
#include "ev0072/amy_action_8/dat_0cba76e8.inc"
};
static NJS_POINT3 dat_0cba7700[7] = {
#include "ev0072/amy_action_8/dat_0cba7700.inc"
};
static NJS_VECTOR dat_0cba7754[7] = {
#include "ev0072/amy_action_8/dat_0cba7754.inc"
};
static NJS_MODEL dat_0cba77a8 = {
#include "ev0072/amy_action_8/dat_0cba77a8.inc"
};
static NJS_OBJECT dat_0cba77d0 = {
#include "ev0072/amy_action_8/dat_0cba77d0.inc"
};
static NJS_OBJECT dat_0cba7804 = {
#include "ev0072/amy_action_8/dat_0cba7804.inc"
};
static NJS_OBJECT dat_0cba7838 = {
#include "ev0072/amy_action_8/dat_0cba7838.inc"
};
static NJS_OBJECT dat_0cba786c = {
#include "ev0072/amy_action_8/dat_0cba786c.inc"
};
static NJS_OBJECT dat_0cba78a0 = {
#include "ev0072/amy_action_8/dat_0cba78a0.inc"
};
static NJS_OBJECT dat_0cba78d4 = {
#include "ev0072/amy_action_8/dat_0cba78d4.inc"
};
static NJS_OBJECT dat_0cba7908 = {
#include "ev0072/amy_action_8/dat_0cba7908.inc"
};
static NJS_OBJECT dat_0cba793c = {
#include "ev0072/amy_action_8/dat_0cba793c.inc"
};
static NJS_OBJECT dat_0cba7970 = {
#include "ev0072/amy_action_8/dat_0cba7970.inc"
};
static NJS_OBJECT dat_0cba79a4 = {
#include "ev0072/amy_action_8/dat_0cba79a4.inc"
};
static NJS_OBJECT dat_0cba79d8 = {
#include "ev0072/amy_action_8/dat_0cba79d8.inc"
};
static NJS_OBJECT dat_0cba7a0c = {
#include "ev0072/amy_action_8/dat_0cba7a0c.inc"
};
static NJS_MATERIAL dat_0cba7a40[1] = {
#include "ev0072/amy_action_8/dat_0cba7a40.inc"
};
static Sint16 dat_0cba7a54[18] = {
#include "ev0072/amy_action_8/dat_0cba7a54.inc"
};
static NJS_COLOR dat_0cba7a78[15] = {
#include "ev0072/amy_action_8/dat_0cba7a78.inc"
};
static NJS_MESHSET dat_0cba7ab4[1] = {
#include "ev0072/amy_action_8/dat_0cba7ab4.inc"
};
static NJS_POINT3 dat_0cba7acc[7] = {
#include "ev0072/amy_action_8/dat_0cba7acc.inc"
};
static NJS_VECTOR dat_0cba7b20[7] = {
#include "ev0072/amy_action_8/dat_0cba7b20.inc"
};
static NJS_MODEL dat_0cba7b74 = {
#include "ev0072/amy_action_8/dat_0cba7b74.inc"
};
static NJS_OBJECT dat_0cba7b9c = {
#include "ev0072/amy_action_8/dat_0cba7b9c.inc"
};
static NJS_OBJECT dat_0cba7bd0 = {
#include "ev0072/amy_action_8/dat_0cba7bd0.inc"
};
static NJS_OBJECT dat_0cba7c04 = {
#include "ev0072/amy_action_8/dat_0cba7c04.inc"
};
static NJS_OBJECT dat_0cba7c38 = {
#include "ev0072/amy_action_8/dat_0cba7c38.inc"
};
static NJS_MATERIAL dat_0cba7c6c[1] = {
#include "ev0072/amy_action_8/dat_0cba7c6c.inc"
};
static Sint16 dat_0cba7c80[10] = {
#include "ev0072/amy_action_8/dat_0cba7c80.inc"
};
static NJS_COLOR dat_0cba7c94[8] = {
#include "ev0072/amy_action_8/dat_0cba7c94.inc"
};
static NJS_MESHSET dat_0cba7cb4[1] = {
#include "ev0072/amy_action_8/dat_0cba7cb4.inc"
};
static NJS_POINT3 dat_0cba7ccc[8] = {
#include "ev0072/amy_action_8/dat_0cba7ccc.inc"
};
static NJS_VECTOR dat_0cba7d2c[8] = {
#include "ev0072/amy_action_8/dat_0cba7d2c.inc"
};
static NJS_MODEL dat_0cba7d8c = {
#include "ev0072/amy_action_8/dat_0cba7d8c.inc"
};
static NJS_OBJECT dat_0cba7db4 = {
#include "ev0072/amy_action_8/dat_0cba7db4.inc"
};
static NJS_MATERIAL dat_0cba7de8[1] = {
#include "ev0072/amy_action_8/dat_0cba7de8.inc"
};
static Sint16 dat_0cba7dfc[10] = {
#include "ev0072/amy_action_8/dat_0cba7dfc.inc"
};
static NJS_COLOR dat_0cba7e10[8] = {
#include "ev0072/amy_action_8/dat_0cba7e10.inc"
};
static NJS_MESHSET dat_0cba7e30[1] = {
#include "ev0072/amy_action_8/dat_0cba7e30.inc"
};
static NJS_POINT3 dat_0cba7e48[8] = {
#include "ev0072/amy_action_8/dat_0cba7e48.inc"
};
static NJS_VECTOR dat_0cba7ea8[8] = {
#include "ev0072/amy_action_8/dat_0cba7ea8.inc"
};
static NJS_MODEL dat_0cba7f08 = {
#include "ev0072/amy_action_8/dat_0cba7f08.inc"
};
static NJS_OBJECT dat_0cba7f30 = {
#include "ev0072/amy_action_8/dat_0cba7f30.inc"
};
static NJS_MATERIAL dat_0cba7f64[1] = {
#include "ev0072/amy_action_8/dat_0cba7f64.inc"
};
static Sint16 dat_0cba7f78[16] = {
#include "ev0072/amy_action_8/dat_0cba7f78.inc"
};
static NJS_COLOR dat_0cba7f98[12] = {
#include "ev0072/amy_action_8/dat_0cba7f98.inc"
};
static NJS_MESHSET dat_0cba7fc8[1] = {
#include "ev0072/amy_action_8/dat_0cba7fc8.inc"
};
static NJS_POINT3 dat_0cba7fe0[12] = {
#include "ev0072/amy_action_8/dat_0cba7fe0.inc"
};
static NJS_VECTOR dat_0cba8070[12] = {
#include "ev0072/amy_action_8/dat_0cba8070.inc"
};
static NJS_MODEL dat_0cba8100 = {
#include "ev0072/amy_action_8/dat_0cba8100.inc"
};
static NJS_OBJECT dat_0cba8128 = {
#include "ev0072/amy_action_8/dat_0cba8128.inc"
};
static NJS_MATERIAL dat_0cba815c[1] = {
#include "ev0072/amy_action_8/dat_0cba815c.inc"
};
static Sint16 dat_0cba8170[16] = {
#include "ev0072/amy_action_8/dat_0cba8170.inc"
};
static NJS_COLOR dat_0cba8190[12] = {
#include "ev0072/amy_action_8/dat_0cba8190.inc"
};
static NJS_MESHSET dat_0cba81c0[1] = {
#include "ev0072/amy_action_8/dat_0cba81c0.inc"
};
static NJS_POINT3 dat_0cba81d8[12] = {
#include "ev0072/amy_action_8/dat_0cba81d8.inc"
};
static NJS_VECTOR dat_0cba8268[12] = {
#include "ev0072/amy_action_8/dat_0cba8268.inc"
};
static NJS_MODEL dat_0cba82f8 = {
#include "ev0072/amy_action_8/dat_0cba82f8.inc"
};
static NJS_OBJECT dat_0cba8320 = {
#include "ev0072/amy_action_8/dat_0cba8320.inc"
};
static NJS_MATERIAL dat_0cba8354[4] = {
#include "ev0072/amy_action_8/dat_0cba8354.inc"
};
static Sint16 dat_0cba83a4[239] = {
#include "ev0072/amy_action_8/dat_0cba83a4.inc"
};
static Sint16 dat_0cba8582[22] = {
#include "ev0072/amy_action_8/dat_0cba8582.inc"
};
static Sint16 dat_0cba85ae[21] = {
#include "ev0072/amy_action_8/dat_0cba85ae.inc"
};
static Sint16 dat_0cba85d8[22] = {
#include "ev0072/amy_action_8/dat_0cba85d8.inc"
};
static NJS_COLOR dat_0cba8604[209] = {
#include "ev0072/amy_action_8/dat_0cba8604.inc"
};
static NJS_COLOR dat_0cba8948[20] = {
#include "ev0072/amy_action_8/dat_0cba8948.inc"
};
static NJS_COLOR dat_0cba8998[16] = {
#include "ev0072/amy_action_8/dat_0cba8998.inc"
};
static NJS_COLOR dat_0cba89d8[18] = {
#include "ev0072/amy_action_8/dat_0cba89d8.inc"
};
static NJS_MESHSET dat_0cba8a20[4] = {
#include "ev0072/amy_action_8/dat_0cba8a20.inc"
};
static NJS_POINT3 dat_0cba8a80[127] = {
#include "ev0072/amy_action_8/dat_0cba8a80.inc"
};
static NJS_VECTOR dat_0cba9074[127] = {
#include "ev0072/amy_action_8/dat_0cba9074.inc"
};
static NJS_MODEL dat_0cba9668 = {
#include "ev0072/amy_action_8/dat_0cba9668.inc"
};
static NJS_OBJECT dat_0cba9690 = {
#include "ev0072/amy_action_8/dat_0cba9690.inc"
};
static NJS_OBJECT dat_0cba96c4 = {
#include "ev0072/amy_action_8/dat_0cba96c4.inc"
};
static NJS_OBJECT dat_0cba96f8 = {
#include "ev0072/amy_action_8/dat_0cba96f8.inc"
};
static NJS_OBJECT dat_0cba972c = {
#include "ev0072/amy_action_8/dat_0cba972c.inc"
};
static NJS_MATERIAL dat_0cba9760[1] = {
#include "ev0072/amy_action_8/dat_0cba9760.inc"
};
static Sint16 dat_0cba9774[8] = {
#include "ev0072/amy_action_8/dat_0cba9774.inc"
};
static NJS_COLOR dat_0cba9784[6] = {
#include "ev0072/amy_action_8/dat_0cba9784.inc"
};
static NJS_MESHSET dat_0cba979c[1] = {
#include "ev0072/amy_action_8/dat_0cba979c.inc"
};
static NJS_POINT3 dat_0cba97b4[6] = {
#include "ev0072/amy_action_8/dat_0cba97b4.inc"
};
static NJS_VECTOR dat_0cba97fc[6] = {
#include "ev0072/amy_action_8/dat_0cba97fc.inc"
};
static NJS_MODEL dat_0cba9844 = {
#include "ev0072/amy_action_8/dat_0cba9844.inc"
};
static NJS_OBJECT dat_0cba986c = {
#include "ev0072/amy_action_8/dat_0cba986c.inc"
};
static NJS_MATERIAL dat_0cba98a0[1] = {
#include "ev0072/amy_action_8/dat_0cba98a0.inc"
};
static Sint16 dat_0cba98b4[22] = {
#include "ev0072/amy_action_8/dat_0cba98b4.inc"
};
static NJS_COLOR dat_0cba98e0[16] = {
#include "ev0072/amy_action_8/dat_0cba98e0.inc"
};
static NJS_MESHSET dat_0cba9920[1] = {
#include "ev0072/amy_action_8/dat_0cba9920.inc"
};
static NJS_POINT3 dat_0cba9938[5] = {
#include "ev0072/amy_action_8/dat_0cba9938.inc"
};
static NJS_VECTOR dat_0cba9974[5] = {
#include "ev0072/amy_action_8/dat_0cba9974.inc"
};
static NJS_MODEL dat_0cba99b0 = {
#include "ev0072/amy_action_8/dat_0cba99b0.inc"
};
static NJS_OBJECT dat_0cba99d8 = {
#include "ev0072/amy_action_8/dat_0cba99d8.inc"
};
static NJS_MATERIAL dat_0cba9a0c[1] = {
#include "ev0072/amy_action_8/dat_0cba9a0c.inc"
};
static Sint16 dat_0cba9a20[30] = {
#include "ev0072/amy_action_8/dat_0cba9a20.inc"
};
static NJS_COLOR dat_0cba9a5c[28] = {
#include "ev0072/amy_action_8/dat_0cba9a5c.inc"
};
static NJS_MESHSET dat_0cba9acc[1] = {
#include "ev0072/amy_action_8/dat_0cba9acc.inc"
};
static NJS_POINT3 dat_0cba9ae4[18] = {
#include "ev0072/amy_action_8/dat_0cba9ae4.inc"
};
static NJS_VECTOR dat_0cba9bbc[18] = {
#include "ev0072/amy_action_8/dat_0cba9bbc.inc"
};
static NJS_MODEL dat_0cba9c94 = {
#include "ev0072/amy_action_8/dat_0cba9c94.inc"
};
static NJS_OBJECT dat_0cba9cbc = {
#include "ev0072/amy_action_8/dat_0cba9cbc.inc"
};
static NJS_MATERIAL dat_0cba9cf0[1] = {
#include "ev0072/amy_action_8/dat_0cba9cf0.inc"
};
static Sint16 dat_0cba9d04[20] = {
#include "ev0072/amy_action_8/dat_0cba9d04.inc"
};
static NJS_COLOR dat_0cba9d2c[16] = {
#include "ev0072/amy_action_8/dat_0cba9d2c.inc"
};
static NJS_MESHSET dat_0cba9d6c[1] = {
#include "ev0072/amy_action_8/dat_0cba9d6c.inc"
};
static NJS_POINT3 dat_0cba9d84[12] = {
#include "ev0072/amy_action_8/dat_0cba9d84.inc"
};
static NJS_VECTOR dat_0cba9e14[12] = {
#include "ev0072/amy_action_8/dat_0cba9e14.inc"
};
static NJS_MODEL dat_0cba9ea4 = {
#include "ev0072/amy_action_8/dat_0cba9ea4.inc"
};
static NJS_OBJECT dat_0cba9ecc = {
#include "ev0072/amy_action_8/dat_0cba9ecc.inc"
};
static NJS_MATERIAL dat_0cba9f00[1] = {
#include "ev0072/amy_action_8/dat_0cba9f00.inc"
};
static Sint16 dat_0cba9f14[10] = {
#include "ev0072/amy_action_8/dat_0cba9f14.inc"
};
static NJS_COLOR dat_0cba9f28[8] = {
#include "ev0072/amy_action_8/dat_0cba9f28.inc"
};
static NJS_MESHSET dat_0cba9f48[1] = {
#include "ev0072/amy_action_8/dat_0cba9f48.inc"
};
static NJS_POINT3 dat_0cba9f60[5] = {
#include "ev0072/amy_action_8/dat_0cba9f60.inc"
};
static NJS_VECTOR dat_0cba9f9c[5] = {
#include "ev0072/amy_action_8/dat_0cba9f9c.inc"
};
static NJS_MODEL dat_0cba9fd8 = {
#include "ev0072/amy_action_8/dat_0cba9fd8.inc"
};
static NJS_OBJECT dat_0cbaa000 = {
#include "ev0072/amy_action_8/dat_0cbaa000.inc"
};
static NJS_MATERIAL dat_0cbaa034[1] = {
#include "ev0072/amy_action_8/dat_0cbaa034.inc"
};
static Sint16 dat_0cbaa048[10] = {
#include "ev0072/amy_action_8/dat_0cbaa048.inc"
};
static NJS_COLOR dat_0cbaa05c[8] = {
#include "ev0072/amy_action_8/dat_0cbaa05c.inc"
};
static NJS_MESHSET dat_0cbaa07c[1] = {
#include "ev0072/amy_action_8/dat_0cbaa07c.inc"
};
static NJS_POINT3 dat_0cbaa094[5] = {
#include "ev0072/amy_action_8/dat_0cbaa094.inc"
};
static NJS_VECTOR dat_0cbaa0d0[5] = {
#include "ev0072/amy_action_8/dat_0cbaa0d0.inc"
};
static NJS_MODEL dat_0cbaa10c = {
#include "ev0072/amy_action_8/dat_0cbaa10c.inc"
};
static NJS_OBJECT dat_0cbaa134 = {
#include "ev0072/amy_action_8/dat_0cbaa134.inc"
};
static NJS_MATERIAL dat_0cbaa168[1] = {
#include "ev0072/amy_action_8/dat_0cbaa168.inc"
};
static Sint16 dat_0cbaa17c[24] = {
#include "ev0072/amy_action_8/dat_0cbaa17c.inc"
};
static NJS_COLOR dat_0cbaa1ac[20] = {
#include "ev0072/amy_action_8/dat_0cbaa1ac.inc"
};
static NJS_MESHSET dat_0cbaa1fc[1] = {
#include "ev0072/amy_action_8/dat_0cbaa1fc.inc"
};
static NJS_POINT3 dat_0cbaa214[12] = {
#include "ev0072/amy_action_8/dat_0cbaa214.inc"
};
static NJS_VECTOR dat_0cbaa2a4[12] = {
#include "ev0072/amy_action_8/dat_0cbaa2a4.inc"
};
static NJS_MODEL dat_0cbaa334 = {
#include "ev0072/amy_action_8/dat_0cbaa334.inc"
};
static NJS_OBJECT dat_0cbaa35c = {
#include "ev0072/amy_action_8/dat_0cbaa35c.inc"
};
static NJS_MATERIAL dat_0cbaa390[1] = {
#include "ev0072/amy_action_8/dat_0cbaa390.inc"
};
static Sint16 dat_0cbaa3a4[36] = {
#include "ev0072/amy_action_8/dat_0cbaa3a4.inc"
};
static NJS_COLOR dat_0cbaa3ec[30] = {
#include "ev0072/amy_action_8/dat_0cbaa3ec.inc"
};
static NJS_MESHSET dat_0cbaa464[1] = {
#include "ev0072/amy_action_8/dat_0cbaa464.inc"
};
static NJS_POINT3 dat_0cbaa47c[12] = {
#include "ev0072/amy_action_8/dat_0cbaa47c.inc"
};
static NJS_VECTOR dat_0cbaa50c[12] = {
#include "ev0072/amy_action_8/dat_0cbaa50c.inc"
};
static NJS_MODEL dat_0cbaa59c = {
#include "ev0072/amy_action_8/dat_0cbaa59c.inc"
};
static NJS_OBJECT dat_0cbaa5c4 = {
#include "ev0072/amy_action_8/dat_0cbaa5c4.inc"
};
static NJS_OBJECT dat_0cbaa5f8 = {
#include "ev0072/amy_action_8/dat_0cbaa5f8.inc"
};
static NJS_OBJECT dat_0cbaa62c = {
#include "ev0072/amy_action_8/dat_0cbaa62c.inc"
};
static NJS_OBJECT dat_0cbaa660 = {
#include "ev0072/amy_action_8/dat_0cbaa660.inc"
};
static NJS_MATERIAL dat_0cbaa694[1] = {
#include "ev0072/amy_action_8/dat_0cbaa694.inc"
};
static Sint16 dat_0cbaa6a8[48] = {
#include "ev0072/amy_action_8/dat_0cbaa6a8.inc"
};
static NJS_COLOR dat_0cbaa708[38] = {
#include "ev0072/amy_action_8/dat_0cbaa708.inc"
};
static NJS_MESHSET dat_0cbaa7a0[1] = {
#include "ev0072/amy_action_8/dat_0cbaa7a0.inc"
};
static NJS_POINT3 dat_0cbaa7b8[12] = {
#include "ev0072/amy_action_8/dat_0cbaa7b8.inc"
};
static NJS_VECTOR dat_0cbaa848[12] = {
#include "ev0072/amy_action_8/dat_0cbaa848.inc"
};
static NJS_MODEL dat_0cbaa8d8 = {
#include "ev0072/amy_action_8/dat_0cbaa8d8.inc"
};
static NJS_OBJECT dat_0cbaa900 = {
#include "ev0072/amy_action_8/dat_0cbaa900.inc"
};
static NJS_OBJECT dat_0cbaa934 = {
#include "ev0072/amy_action_8/dat_0cbaa934.inc"
};
static NJS_OBJECT dat_0cbaa968 = {
#include "ev0072/amy_action_8/dat_0cbaa968.inc"
};
static NJS_OBJECT dat_0cbaa99c = {
#include "ev0072/amy_action_8/dat_0cbaa99c.inc"
};
static NJS_MATERIAL dat_0cbaa9d0[1] = {
#include "ev0072/amy_action_8/dat_0cbaa9d0.inc"
};
static Sint16 dat_0cbaa9e4[48] = {
#include "ev0072/amy_action_8/dat_0cbaa9e4.inc"
};
static NJS_COLOR dat_0cbaaa44[38] = {
#include "ev0072/amy_action_8/dat_0cbaaa44.inc"
};
static NJS_MESHSET dat_0cbaaadc[1] = {
#include "ev0072/amy_action_8/dat_0cbaaadc.inc"
};
static NJS_POINT3 dat_0cbaaaf4[12] = {
#include "ev0072/amy_action_8/dat_0cbaaaf4.inc"
};
static NJS_VECTOR dat_0cbaab84[12] = {
#include "ev0072/amy_action_8/dat_0cbaab84.inc"
};
static NJS_MODEL dat_0cbaac14 = {
#include "ev0072/amy_action_8/dat_0cbaac14.inc"
};
static NJS_OBJECT dat_0cbaac3c = {
#include "ev0072/amy_action_8/dat_0cbaac3c.inc"
};
static NJS_OBJECT dat_0cbaac70 = {
#include "ev0072/amy_action_8/dat_0cbaac70.inc"
};
static NJS_OBJECT dat_0cbaaca4 = {
#include "ev0072/amy_action_8/dat_0cbaaca4.inc"
};
static NJS_OBJECT dat_0cbaacd8 = {
#include "ev0072/amy_action_8/dat_0cbaacd8.inc"
};
static NJS_MATERIAL dat_0cbaad0c[2] = {
#include "ev0072/amy_action_8/dat_0cbaad0c.inc"
};
static Sint16 dat_0cbaad34[54] = {
#include "ev0072/amy_action_8/dat_0cbaad34.inc"
};
static Sint16 dat_0cbaada0[22] = {
#include "ev0072/amy_action_8/dat_0cbaada0.inc"
};
static NJS_COLOR dat_0cbaadcc[48] = {
#include "ev0072/amy_action_8/dat_0cbaadcc.inc"
};
static NJS_COLOR dat_0cbaae8c[18] = {
#include "ev0072/amy_action_8/dat_0cbaae8c.inc"
};
static NJS_MESHSET dat_0cbaaed4[2] = {
#include "ev0072/amy_action_8/dat_0cbaaed4.inc"
};
static NJS_POINT3 dat_0cbaaf04[26] = {
#include "ev0072/amy_action_8/dat_0cbaaf04.inc"
};
static NJS_VECTOR dat_0cbab03c[26] = {
#include "ev0072/amy_action_8/dat_0cbab03c.inc"
};
static NJS_MODEL dat_0cbab174 = {
#include "ev0072/amy_action_8/dat_0cbab174.inc"
};
static NJS_OBJECT dat_0cbab19c = {
#include "ev0072/amy_action_8/dat_0cbab19c.inc"
};
static NJS_OBJECT dat_0cbab1d0 = {
#include "ev0072/amy_action_8/dat_0cbab1d0.inc"
};
static NJS_OBJECT dat_0cbab204 = {
#include "ev0072/amy_action_8/dat_0cbab204.inc"
};
static NJS_OBJECT dat_0cbab238 = {
#include "ev0072/amy_action_8/dat_0cbab238.inc"
};
static NJS_OBJECT dat_0cbab26c = {
#include "ev0072/amy_action_8/dat_0cbab26c.inc"
};
static NJS_TEXNAME ev0072_bird_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0072_bird_texlist = {
#include "ev0072/amy_action_8/ev0072_bird_texlist.inc"
};
static NJS_MATERIAL dat_0cbab338[1] = {
#include "ev0072/amy_action_8/dat_0cbab338.inc"
};
static Sint16 dat_0cbab34c[6] = {
#include "ev0072/amy_action_8/dat_0cbab34c.inc"
};
static NJS_COLOR dat_0cbab358[4] = {
#include "ev0072/amy_action_8/dat_0cbab358.inc"
};
static NJS_MESHSET dat_0cbab368[1] = {
#include "ev0072/amy_action_8/dat_0cbab368.inc"
};
static NJS_POINT3 dat_0cbab380[4] = {
#include "ev0072/amy_action_8/dat_0cbab380.inc"
};
static NJS_VECTOR dat_0cbab3b0[4] = {
#include "ev0072/amy_action_8/dat_0cbab3b0.inc"
};
static NJS_MODEL dat_0cbab3e0 = {
#include "ev0072/amy_action_8/dat_0cbab3e0.inc"
};
static NJS_OBJECT dat_0cbab408 = {
#include "ev0072/amy_action_8/dat_0cbab408.inc"
};
static NJS_MATERIAL dat_0cbab43c[1] = {
#include "ev0072/amy_action_8/dat_0cbab43c.inc"
};
static Sint16 dat_0cbab450[12] = {
#include "ev0072/amy_action_8/dat_0cbab450.inc"
};
static NJS_COLOR dat_0cbab468[9] = {
#include "ev0072/amy_action_8/dat_0cbab468.inc"
};
static NJS_MESHSET dat_0cbab48c[1] = {
#include "ev0072/amy_action_8/dat_0cbab48c.inc"
};
static NJS_POINT3 dat_0cbab4a4[7] = {
#include "ev0072/amy_action_8/dat_0cbab4a4.inc"
};
static NJS_VECTOR dat_0cbab4f8[7] = {
#include "ev0072/amy_action_8/dat_0cbab4f8.inc"
};
static NJS_MODEL dat_0cbab54c = {
#include "ev0072/amy_action_8/dat_0cbab54c.inc"
};
static NJS_OBJECT dat_0cbab574 = {
#include "ev0072/amy_action_8/dat_0cbab574.inc"
};
static NJS_OBJECT dat_0cbab5a8 = {
#include "ev0072/amy_action_8/dat_0cbab5a8.inc"
};
static NJS_OBJECT dat_0cbab5dc = {
#include "ev0072/amy_action_8/dat_0cbab5dc.inc"
};
static NJS_OBJECT dat_0cbab610 = {
#include "ev0072/amy_action_8/dat_0cbab610.inc"
};
static NJS_OBJECT dat_0cbab644 = {
#include "ev0072/amy_action_8/dat_0cbab644.inc"
};
static NJS_OBJECT dat_0cbab678 = {
#include "ev0072/amy_action_8/dat_0cbab678.inc"
};
static NJS_OBJECT dat_0cbab6ac = {
#include "ev0072/amy_action_8/dat_0cbab6ac.inc"
};
static NJS_MATERIAL dat_0cbab6e0[1] = {
#include "ev0072/amy_action_8/dat_0cbab6e0.inc"
};
static Sint16 dat_0cbab6f4[6] = {
#include "ev0072/amy_action_8/dat_0cbab6f4.inc"
};
static NJS_COLOR dat_0cbab700[4] = {
#include "ev0072/amy_action_8/dat_0cbab700.inc"
};
static NJS_MESHSET dat_0cbab710[1] = {
#include "ev0072/amy_action_8/dat_0cbab710.inc"
};
static NJS_POINT3 dat_0cbab728[4] = {
#include "ev0072/amy_action_8/dat_0cbab728.inc"
};
static NJS_VECTOR dat_0cbab758[4] = {
#include "ev0072/amy_action_8/dat_0cbab758.inc"
};
static NJS_MODEL dat_0cbab788 = {
#include "ev0072/amy_action_8/dat_0cbab788.inc"
};
static NJS_OBJECT dat_0cbab7b0 = {
#include "ev0072/amy_action_8/dat_0cbab7b0.inc"
};
static NJS_MATERIAL dat_0cbab7e4[1] = {
#include "ev0072/amy_action_8/dat_0cbab7e4.inc"
};
static Sint16 dat_0cbab7f8[12] = {
#include "ev0072/amy_action_8/dat_0cbab7f8.inc"
};
static NJS_COLOR dat_0cbab810[9] = {
#include "ev0072/amy_action_8/dat_0cbab810.inc"
};
static NJS_MESHSET dat_0cbab834[1] = {
#include "ev0072/amy_action_8/dat_0cbab834.inc"
};
static NJS_POINT3 dat_0cbab84c[7] = {
#include "ev0072/amy_action_8/dat_0cbab84c.inc"
};
static NJS_VECTOR dat_0cbab8a0[7] = {
#include "ev0072/amy_action_8/dat_0cbab8a0.inc"
};
static NJS_MODEL dat_0cbab8f4 = {
#include "ev0072/amy_action_8/dat_0cbab8f4.inc"
};
static NJS_OBJECT dat_0cbab91c = {
#include "ev0072/amy_action_8/dat_0cbab91c.inc"
};
static NJS_OBJECT dat_0cbab950 = {
#include "ev0072/amy_action_8/dat_0cbab950.inc"
};
static NJS_OBJECT dat_0cbab984 = {
#include "ev0072/amy_action_8/dat_0cbab984.inc"
};
static NJS_OBJECT dat_0cbab9b8 = {
#include "ev0072/amy_action_8/dat_0cbab9b8.inc"
};
static NJS_OBJECT dat_0cbab9ec = {
#include "ev0072/amy_action_8/dat_0cbab9ec.inc"
};
static NJS_OBJECT dat_0cbaba20 = {
#include "ev0072/amy_action_8/dat_0cbaba20.inc"
};
static NJS_OBJECT dat_0cbaba54 = {
#include "ev0072/amy_action_8/dat_0cbaba54.inc"
};
static NJS_OBJECT dat_0cbaba88 = {
#include "ev0072/amy_action_8/dat_0cbaba88.inc"
};
static NJS_OBJECT dat_0cbababc = {
#include "ev0072/amy_action_8/dat_0cbababc.inc"
};
static NJS_OBJECT dat_0cbabaf0 = {
#include "ev0072/amy_action_8/dat_0cbabaf0.inc"
};
static NJS_OBJECT dat_0cbabb24 = {
#include "ev0072/amy_action_8/dat_0cbabb24.inc"
};
static NJS_OBJECT dat_0cbabb58 = {
#include "ev0072/amy_action_8/dat_0cbabb58.inc"
};
static NJS_MATERIAL dat_0cbabb8c[1] = {
#include "ev0072/amy_action_8/dat_0cbabb8c.inc"
};
static Sint16 dat_0cbabba0[18] = {
#include "ev0072/amy_action_8/dat_0cbabba0.inc"
};
static NJS_COLOR dat_0cbabbc4[15] = {
#include "ev0072/amy_action_8/dat_0cbabbc4.inc"
};
static NJS_MESHSET dat_0cbabc00[1] = {
#include "ev0072/amy_action_8/dat_0cbabc00.inc"
};
static NJS_POINT3 dat_0cbabc18[7] = {
#include "ev0072/amy_action_8/dat_0cbabc18.inc"
};
static NJS_VECTOR dat_0cbabc6c[7] = {
#include "ev0072/amy_action_8/dat_0cbabc6c.inc"
};
static NJS_MODEL dat_0cbabcc0 = {
#include "ev0072/amy_action_8/dat_0cbabcc0.inc"
};
static NJS_OBJECT dat_0cbabce8 = {
#include "ev0072/amy_action_8/dat_0cbabce8.inc"
};
static NJS_OBJECT dat_0cbabd1c = {
#include "ev0072/amy_action_8/dat_0cbabd1c.inc"
};
static NJS_OBJECT dat_0cbabd50 = {
#include "ev0072/amy_action_8/dat_0cbabd50.inc"
};
static NJS_OBJECT dat_0cbabd84 = {
#include "ev0072/amy_action_8/dat_0cbabd84.inc"
};
static NJS_MATERIAL dat_0cbabdb8[1] = {
#include "ev0072/amy_action_8/dat_0cbabdb8.inc"
};
static Sint16 dat_0cbabdcc[10] = {
#include "ev0072/amy_action_8/dat_0cbabdcc.inc"
};
static NJS_COLOR dat_0cbabde0[8] = {
#include "ev0072/amy_action_8/dat_0cbabde0.inc"
};
static NJS_MESHSET dat_0cbabe00[1] = {
#include "ev0072/amy_action_8/dat_0cbabe00.inc"
};
static NJS_POINT3 dat_0cbabe18[8] = {
#include "ev0072/amy_action_8/dat_0cbabe18.inc"
};
static NJS_VECTOR dat_0cbabe78[8] = {
#include "ev0072/amy_action_8/dat_0cbabe78.inc"
};
static NJS_MODEL dat_0cbabed8 = {
#include "ev0072/amy_action_8/dat_0cbabed8.inc"
};
static NJS_OBJECT dat_0cbabf00 = {
#include "ev0072/amy_action_8/dat_0cbabf00.inc"
};
static NJS_MATERIAL dat_0cbabf34[1] = {
#include "ev0072/amy_action_8/dat_0cbabf34.inc"
};
static Sint16 dat_0cbabf48[10] = {
#include "ev0072/amy_action_8/dat_0cbabf48.inc"
};
static NJS_COLOR dat_0cbabf5c[8] = {
#include "ev0072/amy_action_8/dat_0cbabf5c.inc"
};
static NJS_MESHSET dat_0cbabf7c[1] = {
#include "ev0072/amy_action_8/dat_0cbabf7c.inc"
};
static NJS_POINT3 dat_0cbabf94[8] = {
#include "ev0072/amy_action_8/dat_0cbabf94.inc"
};
static NJS_VECTOR dat_0cbabff4[8] = {
#include "ev0072/amy_action_8/dat_0cbabff4.inc"
};
static NJS_MODEL dat_0cbac054 = {
#include "ev0072/amy_action_8/dat_0cbac054.inc"
};
static NJS_OBJECT dat_0cbac07c = {
#include "ev0072/amy_action_8/dat_0cbac07c.inc"
};
static NJS_MATERIAL dat_0cbac0b0[1] = {
#include "ev0072/amy_action_8/dat_0cbac0b0.inc"
};
static Sint16 dat_0cbac0c4[16] = {
#include "ev0072/amy_action_8/dat_0cbac0c4.inc"
};
static NJS_COLOR dat_0cbac0e4[12] = {
#include "ev0072/amy_action_8/dat_0cbac0e4.inc"
};
static NJS_MESHSET dat_0cbac114[1] = {
#include "ev0072/amy_action_8/dat_0cbac114.inc"
};
static NJS_POINT3 dat_0cbac12c[12] = {
#include "ev0072/amy_action_8/dat_0cbac12c.inc"
};
static NJS_VECTOR dat_0cbac1bc[12] = {
#include "ev0072/amy_action_8/dat_0cbac1bc.inc"
};
static NJS_MODEL dat_0cbac24c = {
#include "ev0072/amy_action_8/dat_0cbac24c.inc"
};
static NJS_OBJECT dat_0cbac274 = {
#include "ev0072/amy_action_8/dat_0cbac274.inc"
};
static NJS_MATERIAL dat_0cbac2a8[1] = {
#include "ev0072/amy_action_8/dat_0cbac2a8.inc"
};
static Sint16 dat_0cbac2bc[16] = {
#include "ev0072/amy_action_8/dat_0cbac2bc.inc"
};
static NJS_COLOR dat_0cbac2dc[12] = {
#include "ev0072/amy_action_8/dat_0cbac2dc.inc"
};
static NJS_MESHSET dat_0cbac30c[1] = {
#include "ev0072/amy_action_8/dat_0cbac30c.inc"
};
static NJS_POINT3 dat_0cbac324[12] = {
#include "ev0072/amy_action_8/dat_0cbac324.inc"
};
static NJS_VECTOR dat_0cbac3b4[12] = {
#include "ev0072/amy_action_8/dat_0cbac3b4.inc"
};
static NJS_MODEL dat_0cbac444 = {
#include "ev0072/amy_action_8/dat_0cbac444.inc"
};
static NJS_OBJECT dat_0cbac46c = {
#include "ev0072/amy_action_8/dat_0cbac46c.inc"
};
static NJS_MATERIAL dat_0cbac4a0[4] = {
#include "ev0072/amy_action_8/dat_0cbac4a0.inc"
};
static Sint16 dat_0cbac4f0[239] = {
#include "ev0072/amy_action_8/dat_0cbac4f0.inc"
};
static Sint16 dat_0cbac6ce[22] = {
#include "ev0072/amy_action_8/dat_0cbac6ce.inc"
};
static Sint16 dat_0cbac6fa[21] = {
#include "ev0072/amy_action_8/dat_0cbac6fa.inc"
};
static Sint16 dat_0cbac724[22] = {
#include "ev0072/amy_action_8/dat_0cbac724.inc"
};
static NJS_COLOR dat_0cbac750[209] = {
#include "ev0072/amy_action_8/dat_0cbac750.inc"
};
static NJS_COLOR dat_0cbaca94[20] = {
#include "ev0072/amy_action_8/dat_0cbaca94.inc"
};
static NJS_COLOR dat_0cbacae4[16] = {
#include "ev0072/amy_action_8/dat_0cbacae4.inc"
};
static NJS_COLOR dat_0cbacb24[18] = {
#include "ev0072/amy_action_8/dat_0cbacb24.inc"
};
static NJS_MESHSET dat_0cbacb6c[4] = {
#include "ev0072/amy_action_8/dat_0cbacb6c.inc"
};
static NJS_POINT3 dat_0cbacbcc[127] = {
#include "ev0072/amy_action_8/dat_0cbacbcc.inc"
};
static NJS_VECTOR dat_0cbad1c0[127] = {
#include "ev0072/amy_action_8/dat_0cbad1c0.inc"
};
static NJS_MODEL dat_0cbad7b4 = {
#include "ev0072/amy_action_8/dat_0cbad7b4.inc"
};
static NJS_OBJECT dat_0cbad7dc = {
#include "ev0072/amy_action_8/dat_0cbad7dc.inc"
};
static NJS_OBJECT dat_0cbad810 = {
#include "ev0072/amy_action_8/dat_0cbad810.inc"
};
static NJS_OBJECT dat_0cbad844 = {
#include "ev0072/amy_action_8/dat_0cbad844.inc"
};
static NJS_OBJECT dat_0cbad878 = {
#include "ev0072/amy_action_8/dat_0cbad878.inc"
};
static NJS_MATERIAL dat_0cbad8ac[1] = {
#include "ev0072/amy_action_8/dat_0cbad8ac.inc"
};
static Sint16 dat_0cbad8c0[8] = {
#include "ev0072/amy_action_8/dat_0cbad8c0.inc"
};
static NJS_COLOR dat_0cbad8d0[6] = {
#include "ev0072/amy_action_8/dat_0cbad8d0.inc"
};
static NJS_MESHSET dat_0cbad8e8[1] = {
#include "ev0072/amy_action_8/dat_0cbad8e8.inc"
};
static NJS_POINT3 dat_0cbad900[6] = {
#include "ev0072/amy_action_8/dat_0cbad900.inc"
};
static NJS_VECTOR dat_0cbad948[6] = {
#include "ev0072/amy_action_8/dat_0cbad948.inc"
};
static NJS_MODEL dat_0cbad990 = {
#include "ev0072/amy_action_8/dat_0cbad990.inc"
};
static NJS_OBJECT dat_0cbad9b8 = {
#include "ev0072/amy_action_8/dat_0cbad9b8.inc"
};
static NJS_MATERIAL dat_0cbad9ec[1] = {
#include "ev0072/amy_action_8/dat_0cbad9ec.inc"
};
static Sint16 dat_0cbada00[22] = {
#include "ev0072/amy_action_8/dat_0cbada00.inc"
};
static NJS_COLOR dat_0cbada2c[16] = {
#include "ev0072/amy_action_8/dat_0cbada2c.inc"
};
static NJS_MESHSET dat_0cbada6c[1] = {
#include "ev0072/amy_action_8/dat_0cbada6c.inc"
};
static NJS_POINT3 dat_0cbada84[5] = {
#include "ev0072/amy_action_8/dat_0cbada84.inc"
};
static NJS_VECTOR dat_0cbadac0[5] = {
#include "ev0072/amy_action_8/dat_0cbadac0.inc"
};
static NJS_MODEL dat_0cbadafc = {
#include "ev0072/amy_action_8/dat_0cbadafc.inc"
};
static NJS_OBJECT dat_0cbadb24 = {
#include "ev0072/amy_action_8/dat_0cbadb24.inc"
};
static NJS_MATERIAL dat_0cbadb58[1] = {
#include "ev0072/amy_action_8/dat_0cbadb58.inc"
};
static Sint16 dat_0cbadb6c[30] = {
#include "ev0072/amy_action_8/dat_0cbadb6c.inc"
};
static NJS_COLOR dat_0cbadba8[28] = {
#include "ev0072/amy_action_8/dat_0cbadba8.inc"
};
static NJS_MESHSET dat_0cbadc18[1] = {
#include "ev0072/amy_action_8/dat_0cbadc18.inc"
};
static NJS_POINT3 dat_0cbadc30[18] = {
#include "ev0072/amy_action_8/dat_0cbadc30.inc"
};
static NJS_VECTOR dat_0cbadd08[18] = {
#include "ev0072/amy_action_8/dat_0cbadd08.inc"
};
static NJS_MODEL dat_0cbadde0 = {
#include "ev0072/amy_action_8/dat_0cbadde0.inc"
};
static NJS_OBJECT dat_0cbade08 = {
#include "ev0072/amy_action_8/dat_0cbade08.inc"
};
static NJS_MATERIAL dat_0cbade3c[1] = {
#include "ev0072/amy_action_8/dat_0cbade3c.inc"
};
static Sint16 dat_0cbade50[20] = {
#include "ev0072/amy_action_8/dat_0cbade50.inc"
};
static NJS_COLOR dat_0cbade78[16] = {
#include "ev0072/amy_action_8/dat_0cbade78.inc"
};
static NJS_MESHSET dat_0cbadeb8[1] = {
#include "ev0072/amy_action_8/dat_0cbadeb8.inc"
};
static NJS_POINT3 dat_0cbaded0[12] = {
#include "ev0072/amy_action_8/dat_0cbaded0.inc"
};
static NJS_VECTOR dat_0cbadf60[12] = {
#include "ev0072/amy_action_8/dat_0cbadf60.inc"
};
static NJS_MODEL dat_0cbadff0 = {
#include "ev0072/amy_action_8/dat_0cbadff0.inc"
};
static NJS_OBJECT dat_0cbae018 = {
#include "ev0072/amy_action_8/dat_0cbae018.inc"
};
static NJS_MATERIAL dat_0cbae04c[1] = {
#include "ev0072/amy_action_8/dat_0cbae04c.inc"
};
static Sint16 dat_0cbae060[10] = {
#include "ev0072/amy_action_8/dat_0cbae060.inc"
};
static NJS_COLOR dat_0cbae074[8] = {
#include "ev0072/amy_action_8/dat_0cbae074.inc"
};
static NJS_MESHSET dat_0cbae094[1] = {
#include "ev0072/amy_action_8/dat_0cbae094.inc"
};
static NJS_POINT3 dat_0cbae0ac[5] = {
#include "ev0072/amy_action_8/dat_0cbae0ac.inc"
};
static NJS_VECTOR dat_0cbae0e8[5] = {
#include "ev0072/amy_action_8/dat_0cbae0e8.inc"
};
static NJS_MODEL dat_0cbae124 = {
#include "ev0072/amy_action_8/dat_0cbae124.inc"
};
static NJS_OBJECT dat_0cbae14c = {
#include "ev0072/amy_action_8/dat_0cbae14c.inc"
};
static NJS_MATERIAL dat_0cbae180[1] = {
#include "ev0072/amy_action_8/dat_0cbae180.inc"
};
static Sint16 dat_0cbae194[10] = {
#include "ev0072/amy_action_8/dat_0cbae194.inc"
};
static NJS_COLOR dat_0cbae1a8[8] = {
#include "ev0072/amy_action_8/dat_0cbae1a8.inc"
};
static NJS_MESHSET dat_0cbae1c8[1] = {
#include "ev0072/amy_action_8/dat_0cbae1c8.inc"
};
static NJS_POINT3 dat_0cbae1e0[5] = {
#include "ev0072/amy_action_8/dat_0cbae1e0.inc"
};
static NJS_VECTOR dat_0cbae21c[5] = {
#include "ev0072/amy_action_8/dat_0cbae21c.inc"
};
static NJS_MODEL dat_0cbae258 = {
#include "ev0072/amy_action_8/dat_0cbae258.inc"
};
static NJS_OBJECT dat_0cbae280 = {
#include "ev0072/amy_action_8/dat_0cbae280.inc"
};
static NJS_MATERIAL dat_0cbae2b4[1] = {
#include "ev0072/amy_action_8/dat_0cbae2b4.inc"
};
static Sint16 dat_0cbae2c8[24] = {
#include "ev0072/amy_action_8/dat_0cbae2c8.inc"
};
static NJS_COLOR dat_0cbae2f8[20] = {
#include "ev0072/amy_action_8/dat_0cbae2f8.inc"
};
static NJS_MESHSET dat_0cbae348[1] = {
#include "ev0072/amy_action_8/dat_0cbae348.inc"
};
static NJS_POINT3 dat_0cbae360[12] = {
#include "ev0072/amy_action_8/dat_0cbae360.inc"
};
static NJS_VECTOR dat_0cbae3f0[12] = {
#include "ev0072/amy_action_8/dat_0cbae3f0.inc"
};
static NJS_MODEL dat_0cbae480 = {
#include "ev0072/amy_action_8/dat_0cbae480.inc"
};
static NJS_OBJECT dat_0cbae4a8 = {
#include "ev0072/amy_action_8/dat_0cbae4a8.inc"
};
static NJS_MATERIAL dat_0cbae4dc[1] = {
#include "ev0072/amy_action_8/dat_0cbae4dc.inc"
};
static Sint16 dat_0cbae4f0[36] = {
#include "ev0072/amy_action_8/dat_0cbae4f0.inc"
};
static NJS_COLOR dat_0cbae538[30] = {
#include "ev0072/amy_action_8/dat_0cbae538.inc"
};
static NJS_MESHSET dat_0cbae5b0[1] = {
#include "ev0072/amy_action_8/dat_0cbae5b0.inc"
};
static NJS_POINT3 dat_0cbae5c8[12] = {
#include "ev0072/amy_action_8/dat_0cbae5c8.inc"
};
static NJS_VECTOR dat_0cbae658[12] = {
#include "ev0072/amy_action_8/dat_0cbae658.inc"
};
static NJS_MODEL dat_0cbae6e8 = {
#include "ev0072/amy_action_8/dat_0cbae6e8.inc"
};
static NJS_OBJECT dat_0cbae710 = {
#include "ev0072/amy_action_8/dat_0cbae710.inc"
};
static NJS_OBJECT dat_0cbae744 = {
#include "ev0072/amy_action_8/dat_0cbae744.inc"
};
static NJS_OBJECT dat_0cbae778 = {
#include "ev0072/amy_action_8/dat_0cbae778.inc"
};
static NJS_OBJECT dat_0cbae7ac = {
#include "ev0072/amy_action_8/dat_0cbae7ac.inc"
};
static NJS_MATERIAL dat_0cbae7e0[1] = {
#include "ev0072/amy_action_8/dat_0cbae7e0.inc"
};
static Sint16 dat_0cbae7f4[48] = {
#include "ev0072/amy_action_8/dat_0cbae7f4.inc"
};
static NJS_COLOR dat_0cbae854[38] = {
#include "ev0072/amy_action_8/dat_0cbae854.inc"
};
static NJS_MESHSET dat_0cbae8ec[1] = {
#include "ev0072/amy_action_8/dat_0cbae8ec.inc"
};
static NJS_POINT3 dat_0cbae904[12] = {
#include "ev0072/amy_action_8/dat_0cbae904.inc"
};
static NJS_VECTOR dat_0cbae994[12] = {
#include "ev0072/amy_action_8/dat_0cbae994.inc"
};
static NJS_MODEL dat_0cbaea24 = {
#include "ev0072/amy_action_8/dat_0cbaea24.inc"
};
static NJS_OBJECT dat_0cbaea4c = {
#include "ev0072/amy_action_8/dat_0cbaea4c.inc"
};
static NJS_OBJECT dat_0cbaea80 = {
#include "ev0072/amy_action_8/dat_0cbaea80.inc"
};
static NJS_OBJECT dat_0cbaeab4 = {
#include "ev0072/amy_action_8/dat_0cbaeab4.inc"
};
static NJS_OBJECT dat_0cbaeae8 = {
#include "ev0072/amy_action_8/dat_0cbaeae8.inc"
};
static NJS_MATERIAL dat_0cbaeb1c[1] = {
#include "ev0072/amy_action_8/dat_0cbaeb1c.inc"
};
static Sint16 dat_0cbaeb30[48] = {
#include "ev0072/amy_action_8/dat_0cbaeb30.inc"
};
static NJS_COLOR dat_0cbaeb90[38] = {
#include "ev0072/amy_action_8/dat_0cbaeb90.inc"
};
static NJS_MESHSET dat_0cbaec28[1] = {
#include "ev0072/amy_action_8/dat_0cbaec28.inc"
};
static NJS_POINT3 dat_0cbaec40[12] = {
#include "ev0072/amy_action_8/dat_0cbaec40.inc"
};
static NJS_VECTOR dat_0cbaecd0[12] = {
#include "ev0072/amy_action_8/dat_0cbaecd0.inc"
};
static NJS_MODEL dat_0cbaed60 = {
#include "ev0072/amy_action_8/dat_0cbaed60.inc"
};
static NJS_OBJECT dat_0cbaed88 = {
#include "ev0072/amy_action_8/dat_0cbaed88.inc"
};
static NJS_OBJECT dat_0cbaedbc = {
#include "ev0072/amy_action_8/dat_0cbaedbc.inc"
};
static NJS_OBJECT dat_0cbaedf0 = {
#include "ev0072/amy_action_8/dat_0cbaedf0.inc"
};
static NJS_OBJECT dat_0cbaee24 = {
#include "ev0072/amy_action_8/dat_0cbaee24.inc"
};
static NJS_MATERIAL dat_0cbaee58[2] = {
#include "ev0072/amy_action_8/dat_0cbaee58.inc"
};
static Sint16 dat_0cbaee80[54] = {
#include "ev0072/amy_action_8/dat_0cbaee80.inc"
};
static Sint16 dat_0cbaeeec[22] = {
#include "ev0072/amy_action_8/dat_0cbaeeec.inc"
};
static NJS_COLOR dat_0cbaef18[48] = {
#include "ev0072/amy_action_8/dat_0cbaef18.inc"
};
static NJS_COLOR dat_0cbaefd8[18] = {
#include "ev0072/amy_action_8/dat_0cbaefd8.inc"
};
static NJS_MESHSET dat_0cbaf020[2] = {
#include "ev0072/amy_action_8/dat_0cbaf020.inc"
};
static NJS_POINT3 dat_0cbaf050[26] = {
#include "ev0072/amy_action_8/dat_0cbaf050.inc"
};
static NJS_VECTOR dat_0cbaf188[26] = {
#include "ev0072/amy_action_8/dat_0cbaf188.inc"
};
static NJS_MODEL dat_0cbaf2c0 = {
#include "ev0072/amy_action_8/dat_0cbaf2c0.inc"
};
static NJS_OBJECT dat_0cbaf2e8 = {
#include "ev0072/amy_action_8/dat_0cbaf2e8.inc"
};
static NJS_OBJECT dat_0cbaf31c = {
#include "ev0072/amy_action_8/dat_0cbaf31c.inc"
};
static NJS_OBJECT dat_0cbaf350 = {
#include "ev0072/amy_action_8/dat_0cbaf350.inc"
};
static NJS_OBJECT dat_0cbaf384 = {
#include "ev0072/amy_action_8/dat_0cbaf384.inc"
};
static NJS_OBJECT ev0072_bird_object = {
#include "ev0072/amy_action_8/ev0072_bird_object.inc"
};
static NJS_MKEY_F dat_0cbaf3ec[2] = {
#include "ev0072/amy_action_8/dat_0cbaf3ec.inc"
};
static NJS_MKEY_A dat_0cbaf40c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf40c.inc"
};
static NJS_MKEY_F dat_0cbaf42c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf42c.inc"
};
static NJS_MKEY_F dat_0cbaf44c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf44c.inc"
};
static NJS_MKEY_A dat_0cbaf46c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf46c.inc"
};
static NJS_MKEY_A dat_0cbaf48c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf48c.inc"
};
static NJS_MKEY_F dat_0cbaf4ac[2] = {
#include "ev0072/amy_action_8/dat_0cbaf4ac.inc"
};
static NJS_MKEY_F dat_0cbaf4cc[2] = {
#include "ev0072/amy_action_8/dat_0cbaf4cc.inc"
};
static NJS_MKEY_A dat_0cbaf4ec[2] = {
#include "ev0072/amy_action_8/dat_0cbaf4ec.inc"
};
static NJS_MKEY_A dat_0cbaf50c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf50c.inc"
};
static NJS_MKEY_F dat_0cbaf52c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf52c.inc"
};
static NJS_MKEY_F dat_0cbaf54c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf54c.inc"
};
static NJS_MKEY_A dat_0cbaf56c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf56c.inc"
};
static NJS_MKEY_A dat_0cbaf58c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf58c.inc"
};
static NJS_MKEY_F dat_0cbaf5ac[2] = {
#include "ev0072/amy_action_8/dat_0cbaf5ac.inc"
};
static NJS_MKEY_F dat_0cbaf5cc[2] = {
#include "ev0072/amy_action_8/dat_0cbaf5cc.inc"
};
static NJS_MKEY_A dat_0cbaf5ec[2] = {
#include "ev0072/amy_action_8/dat_0cbaf5ec.inc"
};
static NJS_MKEY_F dat_0cbaf60c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf60c.inc"
};
static NJS_MKEY_A dat_0cbaf62c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf62c.inc"
};
static NJS_MKEY_F dat_0cbaf64c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf64c.inc"
};
static NJS_MKEY_A dat_0cbaf66c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf66c.inc"
};
static NJS_MKEY_F dat_0cbaf68c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf68c.inc"
};
static NJS_MKEY_A dat_0cbaf6ac[2] = {
#include "ev0072/amy_action_8/dat_0cbaf6ac.inc"
};
static NJS_MKEY_A dat_0cbaf6cc[2] = {
#include "ev0072/amy_action_8/dat_0cbaf6cc.inc"
};
static NJS_MKEY_F dat_0cbaf6ec[2] = {
#include "ev0072/amy_action_8/dat_0cbaf6ec.inc"
};
static NJS_MKEY_F dat_0cbaf70c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf70c.inc"
};
static NJS_MKEY_A dat_0cbaf72c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf72c.inc"
};
static NJS_MKEY_F dat_0cbaf74c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf74c.inc"
};
static NJS_MKEY_A dat_0cbaf76c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf76c.inc"
};
static NJS_MKEY_F dat_0cbaf78c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf78c.inc"
};
static NJS_MKEY_A dat_0cbaf7ac[2] = {
#include "ev0072/amy_action_8/dat_0cbaf7ac.inc"
};
static NJS_MKEY_F dat_0cbaf7cc[2] = {
#include "ev0072/amy_action_8/dat_0cbaf7cc.inc"
};
static NJS_MKEY_A dat_0cbaf7ec[2] = {
#include "ev0072/amy_action_8/dat_0cbaf7ec.inc"
};
static NJS_MKEY_F dat_0cbaf80c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf80c.inc"
};
static NJS_MKEY_A dat_0cbaf82c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf82c.inc"
};
static NJS_MKEY_A dat_0cbaf84c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf84c.inc"
};
static NJS_MKEY_F dat_0cbaf86c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf86c.inc"
};
static NJS_MKEY_F dat_0cbaf88c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf88c.inc"
};
static NJS_MKEY_A dat_0cbaf8ac[2] = {
#include "ev0072/amy_action_8/dat_0cbaf8ac.inc"
};
static NJS_MKEY_F dat_0cbaf8cc[2] = {
#include "ev0072/amy_action_8/dat_0cbaf8cc.inc"
};
static NJS_MKEY_A dat_0cbaf8ec[2] = {
#include "ev0072/amy_action_8/dat_0cbaf8ec.inc"
};
static NJS_MKEY_F dat_0cbaf90c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf90c.inc"
};
static NJS_MKEY_A dat_0cbaf92c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf92c.inc"
};
static NJS_MKEY_F dat_0cbaf94c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf94c.inc"
};
static NJS_MKEY_A dat_0cbaf96c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf96c.inc"
};
static NJS_MKEY_F dat_0cbaf98c[2] = {
#include "ev0072/amy_action_8/dat_0cbaf98c.inc"
};
static NJS_MKEY_A dat_0cbaf9ac[2] = {
#include "ev0072/amy_action_8/dat_0cbaf9ac.inc"
};
static NJS_MKEY_F dat_0cbaf9cc[2] = {
#include "ev0072/amy_action_8/dat_0cbaf9cc.inc"
};
static NJS_MKEY_A dat_0cbaf9ec[2] = {
#include "ev0072/amy_action_8/dat_0cbaf9ec.inc"
};
static NJS_MKEY_F dat_0cbafa0c[2] = {
#include "ev0072/amy_action_8/dat_0cbafa0c.inc"
};
static NJS_MKEY_A dat_0cbafa2c[2] = {
#include "ev0072/amy_action_8/dat_0cbafa2c.inc"
};
static NJS_MKEY_F dat_0cbafa4c[2] = {
#include "ev0072/amy_action_8/dat_0cbafa4c.inc"
};
static NJS_MKEY_F dat_0cbafa6c[2] = {
#include "ev0072/amy_action_8/dat_0cbafa6c.inc"
};
static NJS_MKEY_F dat_0cbafa8c[2] = {
#include "ev0072/amy_action_8/dat_0cbafa8c.inc"
};
static NJS_MKEY_A dat_0cbafaac[2] = {
#include "ev0072/amy_action_8/dat_0cbafaac.inc"
};
static NJS_MKEY_F dat_0cbafacc[2] = {
#include "ev0072/amy_action_8/dat_0cbafacc.inc"
};
static NJS_MKEY_A dat_0cbafaec[2] = {
#include "ev0072/amy_action_8/dat_0cbafaec.inc"
};
static NJS_MKEY_F dat_0cbafb0c[2] = {
#include "ev0072/amy_action_8/dat_0cbafb0c.inc"
};
static NJS_MDATA2 dat_0cbafb2c[57] = {
#include "ev0072/amy_action_8/dat_0cbafb2c.inc"
};
static NJS_MOTION dat_0cbafebc = {
#include "ev0072/amy_action_8/dat_0cbafebc.inc"
};
static NJS_ACTION dat_0cbafec8 = {
#include "ev0072/amy_action_8/dat_0cbafec8.inc"
};
static NJS_MKEY_F dat_0cbafed0[32] = {
#include "ev0072/amy_action_8/dat_0cbafed0.inc"
};
static NJS_MKEY_A dat_0cbb00d0[2] = {
#include "ev0072/amy_action_8/dat_0cbb00d0.inc"
};
static NJS_MKEY_F dat_0cbb00f0[2] = {
#include "ev0072/amy_action_8/dat_0cbb00f0.inc"
};
static NJS_MKEY_F dat_0cbb0110[2] = {
#include "ev0072/amy_action_8/dat_0cbb0110.inc"
};
static NJS_MKEY_A dat_0cbb0130[2] = {
#include "ev0072/amy_action_8/dat_0cbb0130.inc"
};
static NJS_MKEY_A dat_0cbb0150[32] = {
#include "ev0072/amy_action_8/dat_0cbb0150.inc"
};
static NJS_MKEY_F dat_0cbb0350[2] = {
#include "ev0072/amy_action_8/dat_0cbb0350.inc"
};
static NJS_MKEY_F dat_0cbb0370[2] = {
#include "ev0072/amy_action_8/dat_0cbb0370.inc"
};
static NJS_MKEY_A dat_0cbb0390[2] = {
#include "ev0072/amy_action_8/dat_0cbb0390.inc"
};
static NJS_MKEY_A dat_0cbb03b0[32] = {
#include "ev0072/amy_action_8/dat_0cbb03b0.inc"
};
static NJS_MKEY_F dat_0cbb05b0[2] = {
#include "ev0072/amy_action_8/dat_0cbb05b0.inc"
};
static NJS_MKEY_F dat_0cbb05d0[2] = {
#include "ev0072/amy_action_8/dat_0cbb05d0.inc"
};
static NJS_MKEY_A dat_0cbb05f0[2] = {
#include "ev0072/amy_action_8/dat_0cbb05f0.inc"
};
static NJS_MKEY_A dat_0cbb0610[2] = {
#include "ev0072/amy_action_8/dat_0cbb0610.inc"
};
static NJS_MKEY_F dat_0cbb0630[2] = {
#include "ev0072/amy_action_8/dat_0cbb0630.inc"
};
static NJS_MKEY_F dat_0cbb0650[2] = {
#include "ev0072/amy_action_8/dat_0cbb0650.inc"
};
static NJS_MKEY_A dat_0cbb0670[2] = {
#include "ev0072/amy_action_8/dat_0cbb0670.inc"
};
static NJS_MKEY_F dat_0cbb0690[2] = {
#include "ev0072/amy_action_8/dat_0cbb0690.inc"
};
static NJS_MKEY_A dat_0cbb06b0[2] = {
#include "ev0072/amy_action_8/dat_0cbb06b0.inc"
};
static NJS_MKEY_F dat_0cbb06d0[2] = {
#include "ev0072/amy_action_8/dat_0cbb06d0.inc"
};
static NJS_MKEY_A dat_0cbb06f0[2] = {
#include "ev0072/amy_action_8/dat_0cbb06f0.inc"
};
static NJS_MKEY_F dat_0cbb0710[2] = {
#include "ev0072/amy_action_8/dat_0cbb0710.inc"
};
static NJS_MKEY_A dat_0cbb0730[2] = {
#include "ev0072/amy_action_8/dat_0cbb0730.inc"
};
static NJS_MKEY_A dat_0cbb0750[32] = {
#include "ev0072/amy_action_8/dat_0cbb0750.inc"
};
static NJS_MKEY_F dat_0cbb0950[2] = {
#include "ev0072/amy_action_8/dat_0cbb0950.inc"
};
static NJS_MKEY_F dat_0cbb0970[2] = {
#include "ev0072/amy_action_8/dat_0cbb0970.inc"
};
static NJS_MKEY_A dat_0cbb0990[2] = {
#include "ev0072/amy_action_8/dat_0cbb0990.inc"
};
static NJS_MKEY_F dat_0cbb09b0[2] = {
#include "ev0072/amy_action_8/dat_0cbb09b0.inc"
};
static NJS_MKEY_A dat_0cbb09d0[2] = {
#include "ev0072/amy_action_8/dat_0cbb09d0.inc"
};
static NJS_MKEY_F dat_0cbb09f0[2] = {
#include "ev0072/amy_action_8/dat_0cbb09f0.inc"
};
static NJS_MKEY_A dat_0cbb0a10[2] = {
#include "ev0072/amy_action_8/dat_0cbb0a10.inc"
};
static NJS_MKEY_F dat_0cbb0a30[2] = {
#include "ev0072/amy_action_8/dat_0cbb0a30.inc"
};
static NJS_MKEY_A dat_0cbb0a50[2] = {
#include "ev0072/amy_action_8/dat_0cbb0a50.inc"
};
static NJS_MKEY_F dat_0cbb0a70[2] = {
#include "ev0072/amy_action_8/dat_0cbb0a70.inc"
};
static NJS_MKEY_A dat_0cbb0a90[2] = {
#include "ev0072/amy_action_8/dat_0cbb0a90.inc"
};
static NJS_MKEY_A dat_0cbb0ab0[2] = {
#include "ev0072/amy_action_8/dat_0cbb0ab0.inc"
};
static NJS_MKEY_F dat_0cbb0ad0[2] = {
#include "ev0072/amy_action_8/dat_0cbb0ad0.inc"
};
static NJS_MKEY_F dat_0cbb0af0[32] = {
#include "ev0072/amy_action_8/dat_0cbb0af0.inc"
};
static NJS_MKEY_A dat_0cbb0cf0[2] = {
#include "ev0072/amy_action_8/dat_0cbb0cf0.inc"
};
static NJS_MKEY_F dat_0cbb0d10[32] = {
#include "ev0072/amy_action_8/dat_0cbb0d10.inc"
};
static NJS_MKEY_A dat_0cbb0f10[2] = {
#include "ev0072/amy_action_8/dat_0cbb0f10.inc"
};
static NJS_MKEY_F dat_0cbb0f30[2] = {
#include "ev0072/amy_action_8/dat_0cbb0f30.inc"
};
static NJS_MKEY_A dat_0cbb0f50[2] = {
#include "ev0072/amy_action_8/dat_0cbb0f50.inc"
};
static NJS_MKEY_F dat_0cbb0f70[32] = {
#include "ev0072/amy_action_8/dat_0cbb0f70.inc"
};
static NJS_MKEY_A dat_0cbb1170[2] = {
#include "ev0072/amy_action_8/dat_0cbb1170.inc"
};
static NJS_MKEY_F dat_0cbb1190[2] = {
#include "ev0072/amy_action_8/dat_0cbb1190.inc"
};
static NJS_MKEY_A dat_0cbb11b0[2] = {
#include "ev0072/amy_action_8/dat_0cbb11b0.inc"
};
static NJS_MKEY_F dat_0cbb11d0[2] = {
#include "ev0072/amy_action_8/dat_0cbb11d0.inc"
};
static NJS_MKEY_A dat_0cbb11f0[2] = {
#include "ev0072/amy_action_8/dat_0cbb11f0.inc"
};
static NJS_MKEY_F dat_0cbb1210[2] = {
#include "ev0072/amy_action_8/dat_0cbb1210.inc"
};
static NJS_MKEY_A dat_0cbb1230[2] = {
#include "ev0072/amy_action_8/dat_0cbb1230.inc"
};
static NJS_MKEY_F dat_0cbb1250[2] = {
#include "ev0072/amy_action_8/dat_0cbb1250.inc"
};
static NJS_MKEY_F dat_0cbb1270[2] = {
#include "ev0072/amy_action_8/dat_0cbb1270.inc"
};
static NJS_MKEY_F dat_0cbb1290[2] = {
#include "ev0072/amy_action_8/dat_0cbb1290.inc"
};
static NJS_MKEY_A dat_0cbb12b0[2] = {
#include "ev0072/amy_action_8/dat_0cbb12b0.inc"
};
static NJS_MKEY_F dat_0cbb12d0[2] = {
#include "ev0072/amy_action_8/dat_0cbb12d0.inc"
};
static NJS_MKEY_A dat_0cbb12f0[2] = {
#include "ev0072/amy_action_8/dat_0cbb12f0.inc"
};
static NJS_MKEY_F dat_0cbb1310[2] = {
#include "ev0072/amy_action_8/dat_0cbb1310.inc"
};
static NJS_MDATA2 dat_0cbb1330[57] = {
#include "ev0072/amy_action_8/dat_0cbb1330.inc"
};
static NJS_MOTION dat_0cbb16c0 = {
#include "ev0072/amy_action_8/dat_0cbb16c0.inc"
};
static NJS_ACTION dat_0cbb16cc = {
#include "ev0072/amy_action_8/dat_0cbb16cc.inc"
};
static NJS_MKEY_F dat_0cbb16d4[2] = {
#include "ev0072/amy_action_8/dat_0cbb16d4.inc"
};
static NJS_MKEY_F dat_0cbb16f4[30] = {
#include "ev0072/amy_action_8/dat_0cbb16f4.inc"
};
static NJS_MKEY_F dat_0cbb18d4[2] = {
#include "ev0072/amy_action_8/dat_0cbb18d4.inc"
};
static NJS_MKEY_A dat_0cbb18f4[2] = {
#include "ev0072/amy_action_8/dat_0cbb18f4.inc"
};
static NJS_MKEY_F dat_0cbb1914[2] = {
#include "ev0072/amy_action_8/dat_0cbb1914.inc"
};
static NJS_MKEY_F dat_0cbb1934[2] = {
#include "ev0072/amy_action_8/dat_0cbb1934.inc"
};
static NJS_MKEY_F dat_0cbb1954[2] = {
#include "ev0072/amy_action_8/dat_0cbb1954.inc"
};
static NJS_MKEY_F dat_0cbb1974[2] = {
#include "ev0072/amy_action_8/dat_0cbb1974.inc"
};
static NJS_MKEY_F dat_0cbb1994[2] = {
#include "ev0072/amy_action_8/dat_0cbb1994.inc"
};
static NJS_MKEY_A dat_0cbb19b4[2] = {
#include "ev0072/amy_action_8/dat_0cbb19b4.inc"
};
static NJS_MKEY_F dat_0cbb19d4[2] = {
#include "ev0072/amy_action_8/dat_0cbb19d4.inc"
};
static NJS_MKEY_A dat_0cbb19f4[30] = {
#include "ev0072/amy_action_8/dat_0cbb19f4.inc"
};
static NJS_MKEY_F dat_0cbb1bd4[2] = {
#include "ev0072/amy_action_8/dat_0cbb1bd4.inc"
};
static NJS_MKEY_F dat_0cbb1bf4[2] = {
#include "ev0072/amy_action_8/dat_0cbb1bf4.inc"
};
static NJS_MKEY_F dat_0cbb1c14[2] = {
#include "ev0072/amy_action_8/dat_0cbb1c14.inc"
};
static NJS_MKEY_F dat_0cbb1c34[2] = {
#include "ev0072/amy_action_8/dat_0cbb1c34.inc"
};
static NJS_MKEY_F dat_0cbb1c54[2] = {
#include "ev0072/amy_action_8/dat_0cbb1c54.inc"
};
static NJS_MKEY_A dat_0cbb1c74[2] = {
#include "ev0072/amy_action_8/dat_0cbb1c74.inc"
};
static NJS_MKEY_F dat_0cbb1c94[2] = {
#include "ev0072/amy_action_8/dat_0cbb1c94.inc"
};
static NJS_MKEY_A dat_0cbb1cb4[30] = {
#include "ev0072/amy_action_8/dat_0cbb1cb4.inc"
};
static NJS_MKEY_F dat_0cbb1e94[2] = {
#include "ev0072/amy_action_8/dat_0cbb1e94.inc"
};
static NJS_MKEY_F dat_0cbb1eb4[2] = {
#include "ev0072/amy_action_8/dat_0cbb1eb4.inc"
};
static NJS_MKEY_F dat_0cbb1ed4[2] = {
#include "ev0072/amy_action_8/dat_0cbb1ed4.inc"
};
static NJS_MKEY_F dat_0cbb1ef4[2] = {
#include "ev0072/amy_action_8/dat_0cbb1ef4.inc"
};
static NJS_MKEY_F dat_0cbb1f14[2] = {
#include "ev0072/amy_action_8/dat_0cbb1f14.inc"
};
static NJS_MKEY_A dat_0cbb1f34[2] = {
#include "ev0072/amy_action_8/dat_0cbb1f34.inc"
};
static NJS_MKEY_F dat_0cbb1f54[2] = {
#include "ev0072/amy_action_8/dat_0cbb1f54.inc"
};
static NJS_MKEY_A dat_0cbb1f74[2] = {
#include "ev0072/amy_action_8/dat_0cbb1f74.inc"
};
static NJS_MKEY_F dat_0cbb1f94[2] = {
#include "ev0072/amy_action_8/dat_0cbb1f94.inc"
};
static NJS_MKEY_F dat_0cbb1fb4[2] = {
#include "ev0072/amy_action_8/dat_0cbb1fb4.inc"
};
static NJS_MKEY_F dat_0cbb1fd4[2] = {
#include "ev0072/amy_action_8/dat_0cbb1fd4.inc"
};
static NJS_MKEY_F dat_0cbb1ff4[2] = {
#include "ev0072/amy_action_8/dat_0cbb1ff4.inc"
};
static NJS_MKEY_F dat_0cbb2014[2] = {
#include "ev0072/amy_action_8/dat_0cbb2014.inc"
};
static NJS_MKEY_F dat_0cbb2034[2] = {
#include "ev0072/amy_action_8/dat_0cbb2034.inc"
};
static NJS_MKEY_F dat_0cbb2054[2] = {
#include "ev0072/amy_action_8/dat_0cbb2054.inc"
};
static NJS_MKEY_F dat_0cbb2074[2] = {
#include "ev0072/amy_action_8/dat_0cbb2074.inc"
};
static NJS_MKEY_A dat_0cbb2094[2] = {
#include "ev0072/amy_action_8/dat_0cbb2094.inc"
};
static NJS_MKEY_F dat_0cbb20b4[2] = {
#include "ev0072/amy_action_8/dat_0cbb20b4.inc"
};
static NJS_MKEY_F dat_0cbb20d4[2] = {
#include "ev0072/amy_action_8/dat_0cbb20d4.inc"
};
static NJS_MKEY_A dat_0cbb20f4[2] = {
#include "ev0072/amy_action_8/dat_0cbb20f4.inc"
};
static NJS_MKEY_F dat_0cbb2114[2] = {
#include "ev0072/amy_action_8/dat_0cbb2114.inc"
};
static NJS_MKEY_A dat_0cbb2134[2] = {
#include "ev0072/amy_action_8/dat_0cbb2134.inc"
};
static NJS_MKEY_F dat_0cbb2154[2] = {
#include "ev0072/amy_action_8/dat_0cbb2154.inc"
};
static NJS_MKEY_A dat_0cbb2174[2] = {
#include "ev0072/amy_action_8/dat_0cbb2174.inc"
};
static NJS_MKEY_F dat_0cbb2194[2] = {
#include "ev0072/amy_action_8/dat_0cbb2194.inc"
};
static NJS_MKEY_A dat_0cbb21b4[2] = {
#include "ev0072/amy_action_8/dat_0cbb21b4.inc"
};
static NJS_MKEY_F dat_0cbb21d4[2] = {
#include "ev0072/amy_action_8/dat_0cbb21d4.inc"
};
static NJS_MKEY_F dat_0cbb21f4[2] = {
#include "ev0072/amy_action_8/dat_0cbb21f4.inc"
};
static NJS_MKEY_F dat_0cbb2214[2] = {
#include "ev0072/amy_action_8/dat_0cbb2214.inc"
};
static NJS_MKEY_F dat_0cbb2234[2] = {
#include "ev0072/amy_action_8/dat_0cbb2234.inc"
};
static NJS_MKEY_F dat_0cbb2254[2] = {
#include "ev0072/amy_action_8/dat_0cbb2254.inc"
};
static NJS_MKEY_A dat_0cbb2274[2] = {
#include "ev0072/amy_action_8/dat_0cbb2274.inc"
};
static NJS_MKEY_F dat_0cbb2294[2] = {
#include "ev0072/amy_action_8/dat_0cbb2294.inc"
};
static NJS_MKEY_F dat_0cbb22b4[2] = {
#include "ev0072/amy_action_8/dat_0cbb22b4.inc"
};
static NJS_MKEY_A dat_0cbb22d4[2] = {
#include "ev0072/amy_action_8/dat_0cbb22d4.inc"
};
static NJS_MKEY_F dat_0cbb22f4[2] = {
#include "ev0072/amy_action_8/dat_0cbb22f4.inc"
};
static NJS_MKEY_F dat_0cbb2314[2] = {
#include "ev0072/amy_action_8/dat_0cbb2314.inc"
};
static NJS_MKEY_A dat_0cbb2334[2] = {
#include "ev0072/amy_action_8/dat_0cbb2334.inc"
};
static NJS_MKEY_F dat_0cbb2354[2] = {
#include "ev0072/amy_action_8/dat_0cbb2354.inc"
};
static NJS_MKEY_F dat_0cbb2374[2] = {
#include "ev0072/amy_action_8/dat_0cbb2374.inc"
};
static NJS_MKEY_A dat_0cbb2394[2] = {
#include "ev0072/amy_action_8/dat_0cbb2394.inc"
};
static NJS_MKEY_F dat_0cbb23b4[2] = {
#include "ev0072/amy_action_8/dat_0cbb23b4.inc"
};
static NJS_MKEY_F dat_0cbb23d4[2] = {
#include "ev0072/amy_action_8/dat_0cbb23d4.inc"
};
static NJS_MKEY_A dat_0cbb23f4[2] = {
#include "ev0072/amy_action_8/dat_0cbb23f4.inc"
};
static NJS_MKEY_F dat_0cbb2414[2] = {
#include "ev0072/amy_action_8/dat_0cbb2414.inc"
};
static NJS_MKEY_A dat_0cbb2434[30] = {
#include "ev0072/amy_action_8/dat_0cbb2434.inc"
};
static NJS_MKEY_F dat_0cbb2614[2] = {
#include "ev0072/amy_action_8/dat_0cbb2614.inc"
};
static NJS_MKEY_F dat_0cbb2634[2] = {
#include "ev0072/amy_action_8/dat_0cbb2634.inc"
};
static NJS_MKEY_F dat_0cbb2654[2] = {
#include "ev0072/amy_action_8/dat_0cbb2654.inc"
};
static NJS_MKEY_F dat_0cbb2674[2] = {
#include "ev0072/amy_action_8/dat_0cbb2674.inc"
};
static NJS_MKEY_F dat_0cbb2694[30] = {
#include "ev0072/amy_action_8/dat_0cbb2694.inc"
};
static NJS_MKEY_A dat_0cbb2874[2] = {
#include "ev0072/amy_action_8/dat_0cbb2874.inc"
};
static NJS_MKEY_F dat_0cbb2894[2] = {
#include "ev0072/amy_action_8/dat_0cbb2894.inc"
};
static NJS_MKEY_F dat_0cbb28b4[30] = {
#include "ev0072/amy_action_8/dat_0cbb28b4.inc"
};
static NJS_MKEY_A dat_0cbb2a94[2] = {
#include "ev0072/amy_action_8/dat_0cbb2a94.inc"
};
static NJS_MKEY_F dat_0cbb2ab4[2] = {
#include "ev0072/amy_action_8/dat_0cbb2ab4.inc"
};
static NJS_MKEY_F dat_0cbb2ad4[2] = {
#include "ev0072/amy_action_8/dat_0cbb2ad4.inc"
};
static NJS_MKEY_A dat_0cbb2af4[2] = {
#include "ev0072/amy_action_8/dat_0cbb2af4.inc"
};
static NJS_MKEY_F dat_0cbb2b14[2] = {
#include "ev0072/amy_action_8/dat_0cbb2b14.inc"
};
static NJS_MKEY_F dat_0cbb2b34[2] = {
#include "ev0072/amy_action_8/dat_0cbb2b34.inc"
};
static NJS_MKEY_A dat_0cbb2b54[2] = {
#include "ev0072/amy_action_8/dat_0cbb2b54.inc"
};
static NJS_MKEY_F dat_0cbb2b74[2] = {
#include "ev0072/amy_action_8/dat_0cbb2b74.inc"
};
static NJS_MKEY_F dat_0cbb2b94[30] = {
#include "ev0072/amy_action_8/dat_0cbb2b94.inc"
};
static NJS_MKEY_A dat_0cbb2d74[2] = {
#include "ev0072/amy_action_8/dat_0cbb2d74.inc"
};
static NJS_MKEY_F dat_0cbb2d94[2] = {
#include "ev0072/amy_action_8/dat_0cbb2d94.inc"
};
static NJS_MKEY_F dat_0cbb2db4[2] = {
#include "ev0072/amy_action_8/dat_0cbb2db4.inc"
};
static NJS_MKEY_A dat_0cbb2dd4[2] = {
#include "ev0072/amy_action_8/dat_0cbb2dd4.inc"
};
static NJS_MKEY_F dat_0cbb2df4[2] = {
#include "ev0072/amy_action_8/dat_0cbb2df4.inc"
};
static NJS_MKEY_A dat_0cbb2e14[30] = {
#include "ev0072/amy_action_8/dat_0cbb2e14.inc"
};
static NJS_MKEY_F dat_0cbb2ff4[2] = {
#include "ev0072/amy_action_8/dat_0cbb2ff4.inc"
};
static NJS_MKEY_F dat_0cbb3014[2] = {
#include "ev0072/amy_action_8/dat_0cbb3014.inc"
};
static NJS_MKEY_F dat_0cbb3034[2] = {
#include "ev0072/amy_action_8/dat_0cbb3034.inc"
};
static NJS_MKEY_A dat_0cbb3054[2] = {
#include "ev0072/amy_action_8/dat_0cbb3054.inc"
};
static NJS_MKEY_F dat_0cbb3074[2] = {
#include "ev0072/amy_action_8/dat_0cbb3074.inc"
};
static NJS_MKEY_A dat_0cbb3094[30] = {
#include "ev0072/amy_action_8/dat_0cbb3094.inc"
};
static NJS_MKEY_F dat_0cbb3274[2] = {
#include "ev0072/amy_action_8/dat_0cbb3274.inc"
};
static NJS_MKEY_F dat_0cbb3294[2] = {
#include "ev0072/amy_action_8/dat_0cbb3294.inc"
};
static NJS_MKEY_F dat_0cbb32b4[2] = {
#include "ev0072/amy_action_8/dat_0cbb32b4.inc"
};
static NJS_MKEY_F dat_0cbb32d4[2] = {
#include "ev0072/amy_action_8/dat_0cbb32d4.inc"
};
static NJS_MKEY_F dat_0cbb32f4[2] = {
#include "ev0072/amy_action_8/dat_0cbb32f4.inc"
};
static NJS_MKEY_F dat_0cbb3314[2] = {
#include "ev0072/amy_action_8/dat_0cbb3314.inc"
};
static NJS_MKEY_F dat_0cbb3334[2] = {
#include "ev0072/amy_action_8/dat_0cbb3334.inc"
};
static NJS_MKEY_A dat_0cbb3354[2] = {
#include "ev0072/amy_action_8/dat_0cbb3354.inc"
};
static NJS_MKEY_F dat_0cbb3374[2] = {
#include "ev0072/amy_action_8/dat_0cbb3374.inc"
};
static NJS_MKEY_A dat_0cbb3394[30] = {
#include "ev0072/amy_action_8/dat_0cbb3394.inc"
};
static NJS_MKEY_F dat_0cbb3574[2] = {
#include "ev0072/amy_action_8/dat_0cbb3574.inc"
};
static NJS_MKEY_F dat_0cbb3594[2] = {
#include "ev0072/amy_action_8/dat_0cbb3594.inc"
};
static NJS_MKEY_F dat_0cbb35b4[2] = {
#include "ev0072/amy_action_8/dat_0cbb35b4.inc"
};
static NJS_MKEY_A dat_0cbb35d4[2] = {
#include "ev0072/amy_action_8/dat_0cbb35d4.inc"
};
static NJS_MKEY_F dat_0cbb35f4[2] = {
#include "ev0072/amy_action_8/dat_0cbb35f4.inc"
};
static NJS_MKEY_A dat_0cbb3614[30] = {
#include "ev0072/amy_action_8/dat_0cbb3614.inc"
};
static NJS_MKEY_F dat_0cbb37f4[2] = {
#include "ev0072/amy_action_8/dat_0cbb37f4.inc"
};
static NJS_MKEY_F dat_0cbb3814[2] = {
#include "ev0072/amy_action_8/dat_0cbb3814.inc"
};
static NJS_MKEY_F dat_0cbb3834[2] = {
#include "ev0072/amy_action_8/dat_0cbb3834.inc"
};
static NJS_MKEY_F dat_0cbb3854[2] = {
#include "ev0072/amy_action_8/dat_0cbb3854.inc"
};
static NJS_MKEY_F dat_0cbb3874[2] = {
#include "ev0072/amy_action_8/dat_0cbb3874.inc"
};
static NJS_MDATA3 dat_0cbb3894[57] = {
#include "ev0072/amy_action_8/dat_0cbb3894.inc"
};
static NJS_MOTION ev0072_bird_motion = {
#include "ev0072/amy_action_8/ev0072_bird_motion.inc"
};
NJS_ACTION ev0072_bird_action = {
#include "ev0072/amy_action_8/ev0072_bird_action.inc"
};
static NJS_TEXNAME ev0072_bird_ver4_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0072_bird_ver4_texlist = {
#include "ev0072/amy_action_8/ev0072_bird_ver4_texlist.inc"
};
static NJS_MATERIAL dat_0cbb3e98[1] = {
#include "ev0072/amy_action_8/dat_0cbb3e98.inc"
};
static Sint16 dat_0cbb3eac[6] = {
#include "ev0072/amy_action_8/dat_0cbb3eac.inc"
};
static NJS_COLOR dat_0cbb3eb8[4] = {
#include "ev0072/amy_action_8/dat_0cbb3eb8.inc"
};
static NJS_MESHSET dat_0cbb3ec8[1] = {
#include "ev0072/amy_action_8/dat_0cbb3ec8.inc"
};
static NJS_POINT3 dat_0cbb3ee0[4] = {
#include "ev0072/amy_action_8/dat_0cbb3ee0.inc"
};
static NJS_VECTOR dat_0cbb3f10[4] = {
#include "ev0072/amy_action_8/dat_0cbb3f10.inc"
};
static NJS_MODEL dat_0cbb3f40 = {
#include "ev0072/amy_action_8/dat_0cbb3f40.inc"
};
static NJS_OBJECT dat_0cbb3f68 = {
#include "ev0072/amy_action_8/dat_0cbb3f68.inc"
};
static NJS_MATERIAL dat_0cbb3f9c[1] = {
#include "ev0072/amy_action_8/dat_0cbb3f9c.inc"
};
static Sint16 dat_0cbb3fb0[12] = {
#include "ev0072/amy_action_8/dat_0cbb3fb0.inc"
};
static NJS_COLOR dat_0cbb3fc8[9] = {
#include "ev0072/amy_action_8/dat_0cbb3fc8.inc"
};
static NJS_MESHSET dat_0cbb3fec[1] = {
#include "ev0072/amy_action_8/dat_0cbb3fec.inc"
};
static NJS_POINT3 dat_0cbb4004[7] = {
#include "ev0072/amy_action_8/dat_0cbb4004.inc"
};
static NJS_VECTOR dat_0cbb4058[7] = {
#include "ev0072/amy_action_8/dat_0cbb4058.inc"
};
static NJS_MODEL dat_0cbb40ac = {
#include "ev0072/amy_action_8/dat_0cbb40ac.inc"
};
static NJS_OBJECT dat_0cbb40d4 = {
#include "ev0072/amy_action_8/dat_0cbb40d4.inc"
};
static NJS_OBJECT dat_0cbb4108 = {
#include "ev0072/amy_action_8/dat_0cbb4108.inc"
};
static NJS_OBJECT dat_0cbb413c = {
#include "ev0072/amy_action_8/dat_0cbb413c.inc"
};
static NJS_OBJECT dat_0cbb4170 = {
#include "ev0072/amy_action_8/dat_0cbb4170.inc"
};
static NJS_OBJECT dat_0cbb41a4 = {
#include "ev0072/amy_action_8/dat_0cbb41a4.inc"
};
static NJS_OBJECT dat_0cbb41d8 = {
#include "ev0072/amy_action_8/dat_0cbb41d8.inc"
};
static NJS_OBJECT dat_0cbb420c = {
#include "ev0072/amy_action_8/dat_0cbb420c.inc"
};
static NJS_MATERIAL dat_0cbb4240[1] = {
#include "ev0072/amy_action_8/dat_0cbb4240.inc"
};
static Sint16 dat_0cbb4254[6] = {
#include "ev0072/amy_action_8/dat_0cbb4254.inc"
};
static NJS_COLOR dat_0cbb4260[4] = {
#include "ev0072/amy_action_8/dat_0cbb4260.inc"
};
static NJS_MESHSET dat_0cbb4270[1] = {
#include "ev0072/amy_action_8/dat_0cbb4270.inc"
};
static NJS_POINT3 dat_0cbb4288[4] = {
#include "ev0072/amy_action_8/dat_0cbb4288.inc"
};
static NJS_VECTOR dat_0cbb42b8[4] = {
#include "ev0072/amy_action_8/dat_0cbb42b8.inc"
};
static NJS_MODEL dat_0cbb42e8 = {
#include "ev0072/amy_action_8/dat_0cbb42e8.inc"
};
static NJS_OBJECT dat_0cbb4310 = {
#include "ev0072/amy_action_8/dat_0cbb4310.inc"
};
static NJS_MATERIAL dat_0cbb4344[1] = {
#include "ev0072/amy_action_8/dat_0cbb4344.inc"
};
static Sint16 dat_0cbb4358[12] = {
#include "ev0072/amy_action_8/dat_0cbb4358.inc"
};
static NJS_COLOR dat_0cbb4370[9] = {
#include "ev0072/amy_action_8/dat_0cbb4370.inc"
};
static NJS_MESHSET dat_0cbb4394[1] = {
#include "ev0072/amy_action_8/dat_0cbb4394.inc"
};
static NJS_POINT3 dat_0cbb43ac[7] = {
#include "ev0072/amy_action_8/dat_0cbb43ac.inc"
};
static NJS_VECTOR dat_0cbb4400[7] = {
#include "ev0072/amy_action_8/dat_0cbb4400.inc"
};
static NJS_MODEL dat_0cbb4454 = {
#include "ev0072/amy_action_8/dat_0cbb4454.inc"
};
static NJS_OBJECT dat_0cbb447c = {
#include "ev0072/amy_action_8/dat_0cbb447c.inc"
};
static NJS_OBJECT dat_0cbb44b0 = {
#include "ev0072/amy_action_8/dat_0cbb44b0.inc"
};
static NJS_OBJECT dat_0cbb44e4 = {
#include "ev0072/amy_action_8/dat_0cbb44e4.inc"
};
static NJS_OBJECT dat_0cbb4518 = {
#include "ev0072/amy_action_8/dat_0cbb4518.inc"
};
static NJS_OBJECT dat_0cbb454c = {
#include "ev0072/amy_action_8/dat_0cbb454c.inc"
};
static NJS_OBJECT dat_0cbb4580 = {
#include "ev0072/amy_action_8/dat_0cbb4580.inc"
};
static NJS_OBJECT dat_0cbb45b4 = {
#include "ev0072/amy_action_8/dat_0cbb45b4.inc"
};
static NJS_OBJECT dat_0cbb45e8 = {
#include "ev0072/amy_action_8/dat_0cbb45e8.inc"
};
static NJS_OBJECT dat_0cbb461c = {
#include "ev0072/amy_action_8/dat_0cbb461c.inc"
};
static NJS_OBJECT dat_0cbb4650 = {
#include "ev0072/amy_action_8/dat_0cbb4650.inc"
};
static NJS_OBJECT dat_0cbb4684 = {
#include "ev0072/amy_action_8/dat_0cbb4684.inc"
};
static NJS_OBJECT dat_0cbb46b8 = {
#include "ev0072/amy_action_8/dat_0cbb46b8.inc"
};
static NJS_MATERIAL dat_0cbb46ec[1] = {
#include "ev0072/amy_action_8/dat_0cbb46ec.inc"
};
static Sint16 dat_0cbb4700[18] = {
#include "ev0072/amy_action_8/dat_0cbb4700.inc"
};
static NJS_COLOR dat_0cbb4724[15] = {
#include "ev0072/amy_action_8/dat_0cbb4724.inc"
};
static NJS_MESHSET dat_0cbb4760[1] = {
#include "ev0072/amy_action_8/dat_0cbb4760.inc"
};
static NJS_POINT3 dat_0cbb4778[7] = {
#include "ev0072/amy_action_8/dat_0cbb4778.inc"
};
static NJS_VECTOR dat_0cbb47cc[7] = {
#include "ev0072/amy_action_8/dat_0cbb47cc.inc"
};
static NJS_MODEL dat_0cbb4820 = {
#include "ev0072/amy_action_8/dat_0cbb4820.inc"
};
static NJS_OBJECT dat_0cbb4848 = {
#include "ev0072/amy_action_8/dat_0cbb4848.inc"
};
static NJS_OBJECT dat_0cbb487c = {
#include "ev0072/amy_action_8/dat_0cbb487c.inc"
};
static NJS_OBJECT dat_0cbb48b0 = {
#include "ev0072/amy_action_8/dat_0cbb48b0.inc"
};
static NJS_OBJECT dat_0cbb48e4 = {
#include "ev0072/amy_action_8/dat_0cbb48e4.inc"
};
static NJS_MATERIAL dat_0cbb4918[1] = {
#include "ev0072/amy_action_8/dat_0cbb4918.inc"
};
static Sint16 dat_0cbb492c[10] = {
#include "ev0072/amy_action_8/dat_0cbb492c.inc"
};
static NJS_COLOR dat_0cbb4940[8] = {
#include "ev0072/amy_action_8/dat_0cbb4940.inc"
};
static NJS_MESHSET dat_0cbb4960[1] = {
#include "ev0072/amy_action_8/dat_0cbb4960.inc"
};
static NJS_POINT3 dat_0cbb4978[8] = {
#include "ev0072/amy_action_8/dat_0cbb4978.inc"
};
static NJS_VECTOR dat_0cbb49d8[8] = {
#include "ev0072/amy_action_8/dat_0cbb49d8.inc"
};
static NJS_MODEL dat_0cbb4a38 = {
#include "ev0072/amy_action_8/dat_0cbb4a38.inc"
};
static NJS_OBJECT dat_0cbb4a60 = {
#include "ev0072/amy_action_8/dat_0cbb4a60.inc"
};
static NJS_MATERIAL dat_0cbb4a94[1] = {
#include "ev0072/amy_action_8/dat_0cbb4a94.inc"
};
static Sint16 dat_0cbb4aa8[10] = {
#include "ev0072/amy_action_8/dat_0cbb4aa8.inc"
};
static NJS_COLOR dat_0cbb4abc[8] = {
#include "ev0072/amy_action_8/dat_0cbb4abc.inc"
};
static NJS_MESHSET dat_0cbb4adc[1] = {
#include "ev0072/amy_action_8/dat_0cbb4adc.inc"
};
static NJS_POINT3 dat_0cbb4af4[8] = {
#include "ev0072/amy_action_8/dat_0cbb4af4.inc"
};
static NJS_VECTOR dat_0cbb4b54[8] = {
#include "ev0072/amy_action_8/dat_0cbb4b54.inc"
};
static NJS_MODEL dat_0cbb4bb4 = {
#include "ev0072/amy_action_8/dat_0cbb4bb4.inc"
};
static NJS_OBJECT dat_0cbb4bdc = {
#include "ev0072/amy_action_8/dat_0cbb4bdc.inc"
};
static NJS_MATERIAL dat_0cbb4c10[1] = {
#include "ev0072/amy_action_8/dat_0cbb4c10.inc"
};
static Sint16 dat_0cbb4c24[16] = {
#include "ev0072/amy_action_8/dat_0cbb4c24.inc"
};
static NJS_COLOR dat_0cbb4c44[12] = {
#include "ev0072/amy_action_8/dat_0cbb4c44.inc"
};
static NJS_MESHSET dat_0cbb4c74[1] = {
#include "ev0072/amy_action_8/dat_0cbb4c74.inc"
};
static NJS_POINT3 dat_0cbb4c8c[12] = {
#include "ev0072/amy_action_8/dat_0cbb4c8c.inc"
};
static NJS_VECTOR dat_0cbb4d1c[12] = {
#include "ev0072/amy_action_8/dat_0cbb4d1c.inc"
};
static NJS_MODEL dat_0cbb4dac = {
#include "ev0072/amy_action_8/dat_0cbb4dac.inc"
};
static NJS_OBJECT dat_0cbb4dd4 = {
#include "ev0072/amy_action_8/dat_0cbb4dd4.inc"
};
static NJS_MATERIAL dat_0cbb4e08[1] = {
#include "ev0072/amy_action_8/dat_0cbb4e08.inc"
};
static Sint16 dat_0cbb4e1c[16] = {
#include "ev0072/amy_action_8/dat_0cbb4e1c.inc"
};
static NJS_COLOR dat_0cbb4e3c[12] = {
#include "ev0072/amy_action_8/dat_0cbb4e3c.inc"
};
static NJS_MESHSET dat_0cbb4e6c[1] = {
#include "ev0072/amy_action_8/dat_0cbb4e6c.inc"
};
static NJS_POINT3 dat_0cbb4e84[12] = {
#include "ev0072/amy_action_8/dat_0cbb4e84.inc"
};
static NJS_VECTOR dat_0cbb4f14[12] = {
#include "ev0072/amy_action_8/dat_0cbb4f14.inc"
};
static NJS_MODEL dat_0cbb4fa4 = {
#include "ev0072/amy_action_8/dat_0cbb4fa4.inc"
};
static NJS_OBJECT dat_0cbb4fcc = {
#include "ev0072/amy_action_8/dat_0cbb4fcc.inc"
};
static NJS_MATERIAL dat_0cbb5000[4] = {
#include "ev0072/amy_action_8/dat_0cbb5000.inc"
};
static Sint16 dat_0cbb5050[239] = {
#include "ev0072/amy_action_8/dat_0cbb5050.inc"
};
static Sint16 dat_0cbb522e[22] = {
#include "ev0072/amy_action_8/dat_0cbb522e.inc"
};
static Sint16 dat_0cbb525a[21] = {
#include "ev0072/amy_action_8/dat_0cbb525a.inc"
};
static Sint16 dat_0cbb5284[22] = {
#include "ev0072/amy_action_8/dat_0cbb5284.inc"
};
static NJS_COLOR dat_0cbb52b0[209] = {
#include "ev0072/amy_action_8/dat_0cbb52b0.inc"
};
static NJS_COLOR dat_0cbb55f4[20] = {
#include "ev0072/amy_action_8/dat_0cbb55f4.inc"
};
static NJS_COLOR dat_0cbb5644[16] = {
#include "ev0072/amy_action_8/dat_0cbb5644.inc"
};
static NJS_COLOR dat_0cbb5684[18] = {
#include "ev0072/amy_action_8/dat_0cbb5684.inc"
};
static NJS_MESHSET dat_0cbb56cc[4] = {
#include "ev0072/amy_action_8/dat_0cbb56cc.inc"
};
static NJS_POINT3 dat_0cbb572c[127] = {
#include "ev0072/amy_action_8/dat_0cbb572c.inc"
};
static NJS_VECTOR dat_0cbb5d20[127] = {
#include "ev0072/amy_action_8/dat_0cbb5d20.inc"
};
static NJS_MODEL dat_0cbb6314 = {
#include "ev0072/amy_action_8/dat_0cbb6314.inc"
};
static NJS_OBJECT dat_0cbb633c = {
#include "ev0072/amy_action_8/dat_0cbb633c.inc"
};
static NJS_OBJECT dat_0cbb6370 = {
#include "ev0072/amy_action_8/dat_0cbb6370.inc"
};
static NJS_OBJECT dat_0cbb63a4 = {
#include "ev0072/amy_action_8/dat_0cbb63a4.inc"
};
static NJS_OBJECT dat_0cbb63d8 = {
#include "ev0072/amy_action_8/dat_0cbb63d8.inc"
};
static NJS_MATERIAL dat_0cbb640c[1] = {
#include "ev0072/amy_action_8/dat_0cbb640c.inc"
};
static Sint16 dat_0cbb6420[8] = {
#include "ev0072/amy_action_8/dat_0cbb6420.inc"
};
static NJS_COLOR dat_0cbb6430[6] = {
#include "ev0072/amy_action_8/dat_0cbb6430.inc"
};
static NJS_MESHSET dat_0cbb6448[1] = {
#include "ev0072/amy_action_8/dat_0cbb6448.inc"
};
static NJS_POINT3 dat_0cbb6460[6] = {
#include "ev0072/amy_action_8/dat_0cbb6460.inc"
};
static NJS_VECTOR dat_0cbb64a8[6] = {
#include "ev0072/amy_action_8/dat_0cbb64a8.inc"
};
static NJS_MODEL dat_0cbb64f0 = {
#include "ev0072/amy_action_8/dat_0cbb64f0.inc"
};
static NJS_OBJECT dat_0cbb6518 = {
#include "ev0072/amy_action_8/dat_0cbb6518.inc"
};
static NJS_MATERIAL dat_0cbb654c[1] = {
#include "ev0072/amy_action_8/dat_0cbb654c.inc"
};
static Sint16 dat_0cbb6560[22] = {
#include "ev0072/amy_action_8/dat_0cbb6560.inc"
};
static NJS_COLOR dat_0cbb658c[16] = {
#include "ev0072/amy_action_8/dat_0cbb658c.inc"
};
static NJS_MESHSET dat_0cbb65cc[1] = {
#include "ev0072/amy_action_8/dat_0cbb65cc.inc"
};
static NJS_POINT3 dat_0cbb65e4[5] = {
#include "ev0072/amy_action_8/dat_0cbb65e4.inc"
};
static NJS_VECTOR dat_0cbb6620[5] = {
#include "ev0072/amy_action_8/dat_0cbb6620.inc"
};
static NJS_MODEL dat_0cbb665c = {
#include "ev0072/amy_action_8/dat_0cbb665c.inc"
};
static NJS_OBJECT dat_0cbb6684 = {
#include "ev0072/amy_action_8/dat_0cbb6684.inc"
};
static NJS_MATERIAL dat_0cbb66b8[1] = {
#include "ev0072/amy_action_8/dat_0cbb66b8.inc"
};
static Sint16 dat_0cbb66cc[30] = {
#include "ev0072/amy_action_8/dat_0cbb66cc.inc"
};
static NJS_COLOR dat_0cbb6708[28] = {
#include "ev0072/amy_action_8/dat_0cbb6708.inc"
};
static NJS_MESHSET dat_0cbb6778[1] = {
#include "ev0072/amy_action_8/dat_0cbb6778.inc"
};
static NJS_POINT3 dat_0cbb6790[18] = {
#include "ev0072/amy_action_8/dat_0cbb6790.inc"
};
static NJS_VECTOR dat_0cbb6868[18] = {
#include "ev0072/amy_action_8/dat_0cbb6868.inc"
};
static NJS_MODEL dat_0cbb6940 = {
#include "ev0072/amy_action_8/dat_0cbb6940.inc"
};
static NJS_OBJECT dat_0cbb6968 = {
#include "ev0072/amy_action_8/dat_0cbb6968.inc"
};
static NJS_MATERIAL dat_0cbb699c[1] = {
#include "ev0072/amy_action_8/dat_0cbb699c.inc"
};
static Sint16 dat_0cbb69b0[20] = {
#include "ev0072/amy_action_8/dat_0cbb69b0.inc"
};
static NJS_COLOR dat_0cbb69d8[16] = {
#include "ev0072/amy_action_8/dat_0cbb69d8.inc"
};
static NJS_MESHSET dat_0cbb6a18[1] = {
#include "ev0072/amy_action_8/dat_0cbb6a18.inc"
};
static NJS_POINT3 dat_0cbb6a30[12] = {
#include "ev0072/amy_action_8/dat_0cbb6a30.inc"
};
static NJS_VECTOR dat_0cbb6ac0[12] = {
#include "ev0072/amy_action_8/dat_0cbb6ac0.inc"
};
static NJS_MODEL dat_0cbb6b50 = {
#include "ev0072/amy_action_8/dat_0cbb6b50.inc"
};
static NJS_OBJECT dat_0cbb6b78 = {
#include "ev0072/amy_action_8/dat_0cbb6b78.inc"
};
static NJS_MATERIAL dat_0cbb6bac[1] = {
#include "ev0072/amy_action_8/dat_0cbb6bac.inc"
};
static Sint16 dat_0cbb6bc0[10] = {
#include "ev0072/amy_action_8/dat_0cbb6bc0.inc"
};
static NJS_COLOR dat_0cbb6bd4[8] = {
#include "ev0072/amy_action_8/dat_0cbb6bd4.inc"
};
static NJS_MESHSET dat_0cbb6bf4[1] = {
#include "ev0072/amy_action_8/dat_0cbb6bf4.inc"
};
static NJS_POINT3 dat_0cbb6c0c[5] = {
#include "ev0072/amy_action_8/dat_0cbb6c0c.inc"
};
static NJS_VECTOR dat_0cbb6c48[5] = {
#include "ev0072/amy_action_8/dat_0cbb6c48.inc"
};
static NJS_MODEL dat_0cbb6c84 = {
#include "ev0072/amy_action_8/dat_0cbb6c84.inc"
};
static NJS_OBJECT dat_0cbb6cac = {
#include "ev0072/amy_action_8/dat_0cbb6cac.inc"
};
static NJS_MATERIAL dat_0cbb6ce0[1] = {
#include "ev0072/amy_action_8/dat_0cbb6ce0.inc"
};
static Sint16 dat_0cbb6cf4[10] = {
#include "ev0072/amy_action_8/dat_0cbb6cf4.inc"
};
static NJS_COLOR dat_0cbb6d08[8] = {
#include "ev0072/amy_action_8/dat_0cbb6d08.inc"
};
static NJS_MESHSET dat_0cbb6d28[1] = {
#include "ev0072/amy_action_8/dat_0cbb6d28.inc"
};
static NJS_POINT3 dat_0cbb6d40[5] = {
#include "ev0072/amy_action_8/dat_0cbb6d40.inc"
};
static NJS_VECTOR dat_0cbb6d7c[5] = {
#include "ev0072/amy_action_8/dat_0cbb6d7c.inc"
};
static NJS_MODEL dat_0cbb6db8 = {
#include "ev0072/amy_action_8/dat_0cbb6db8.inc"
};
static NJS_OBJECT dat_0cbb6de0 = {
#include "ev0072/amy_action_8/dat_0cbb6de0.inc"
};
static NJS_MATERIAL dat_0cbb6e14[1] = {
#include "ev0072/amy_action_8/dat_0cbb6e14.inc"
};
static Sint16 dat_0cbb6e28[24] = {
#include "ev0072/amy_action_8/dat_0cbb6e28.inc"
};
static NJS_COLOR dat_0cbb6e58[20] = {
#include "ev0072/amy_action_8/dat_0cbb6e58.inc"
};
static NJS_MESHSET dat_0cbb6ea8[1] = {
#include "ev0072/amy_action_8/dat_0cbb6ea8.inc"
};
static NJS_POINT3 dat_0cbb6ec0[12] = {
#include "ev0072/amy_action_8/dat_0cbb6ec0.inc"
};
static NJS_VECTOR dat_0cbb6f50[12] = {
#include "ev0072/amy_action_8/dat_0cbb6f50.inc"
};
static NJS_MODEL dat_0cbb6fe0 = {
#include "ev0072/amy_action_8/dat_0cbb6fe0.inc"
};
static NJS_OBJECT dat_0cbb7008 = {
#include "ev0072/amy_action_8/dat_0cbb7008.inc"
};
static NJS_MATERIAL dat_0cbb703c[1] = {
#include "ev0072/amy_action_8/dat_0cbb703c.inc"
};
static Sint16 dat_0cbb7050[36] = {
#include "ev0072/amy_action_8/dat_0cbb7050.inc"
};
static NJS_COLOR dat_0cbb7098[30] = {
#include "ev0072/amy_action_8/dat_0cbb7098.inc"
};
static NJS_MESHSET dat_0cbb7110[1] = {
#include "ev0072/amy_action_8/dat_0cbb7110.inc"
};
static NJS_POINT3 dat_0cbb7128[12] = {
#include "ev0072/amy_action_8/dat_0cbb7128.inc"
};
static NJS_VECTOR dat_0cbb71b8[12] = {
#include "ev0072/amy_action_8/dat_0cbb71b8.inc"
};
static NJS_MODEL dat_0cbb7248 = {
#include "ev0072/amy_action_8/dat_0cbb7248.inc"
};
static NJS_OBJECT dat_0cbb7270 = {
#include "ev0072/amy_action_8/dat_0cbb7270.inc"
};
static NJS_OBJECT dat_0cbb72a4 = {
#include "ev0072/amy_action_8/dat_0cbb72a4.inc"
};
static NJS_OBJECT dat_0cbb72d8 = {
#include "ev0072/amy_action_8/dat_0cbb72d8.inc"
};
static NJS_OBJECT dat_0cbb730c = {
#include "ev0072/amy_action_8/dat_0cbb730c.inc"
};
static NJS_MATERIAL dat_0cbb7340[1] = {
#include "ev0072/amy_action_8/dat_0cbb7340.inc"
};
static Sint16 dat_0cbb7354[48] = {
#include "ev0072/amy_action_8/dat_0cbb7354.inc"
};
static NJS_COLOR dat_0cbb73b4[38] = {
#include "ev0072/amy_action_8/dat_0cbb73b4.inc"
};
static NJS_MESHSET dat_0cbb744c[1] = {
#include "ev0072/amy_action_8/dat_0cbb744c.inc"
};
static NJS_POINT3 dat_0cbb7464[12] = {
#include "ev0072/amy_action_8/dat_0cbb7464.inc"
};
static NJS_VECTOR dat_0cbb74f4[12] = {
#include "ev0072/amy_action_8/dat_0cbb74f4.inc"
};
static NJS_MODEL dat_0cbb7584 = {
#include "ev0072/amy_action_8/dat_0cbb7584.inc"
};
static NJS_OBJECT dat_0cbb75ac = {
#include "ev0072/amy_action_8/dat_0cbb75ac.inc"
};
static NJS_OBJECT dat_0cbb75e0 = {
#include "ev0072/amy_action_8/dat_0cbb75e0.inc"
};
static NJS_OBJECT dat_0cbb7614 = {
#include "ev0072/amy_action_8/dat_0cbb7614.inc"
};
static NJS_OBJECT dat_0cbb7648 = {
#include "ev0072/amy_action_8/dat_0cbb7648.inc"
};
static NJS_MATERIAL dat_0cbb767c[1] = {
#include "ev0072/amy_action_8/dat_0cbb767c.inc"
};
static Sint16 dat_0cbb7690[48] = {
#include "ev0072/amy_action_8/dat_0cbb7690.inc"
};
static NJS_COLOR dat_0cbb76f0[38] = {
#include "ev0072/amy_action_8/dat_0cbb76f0.inc"
};
static NJS_MESHSET dat_0cbb7788[1] = {
#include "ev0072/amy_action_8/dat_0cbb7788.inc"
};
static NJS_POINT3 dat_0cbb77a0[12] = {
#include "ev0072/amy_action_8/dat_0cbb77a0.inc"
};
static NJS_VECTOR dat_0cbb7830[12] = {
#include "ev0072/amy_action_8/dat_0cbb7830.inc"
};
static NJS_MODEL dat_0cbb78c0 = {
#include "ev0072/amy_action_8/dat_0cbb78c0.inc"
};
static NJS_OBJECT dat_0cbb78e8 = {
#include "ev0072/amy_action_8/dat_0cbb78e8.inc"
};
static NJS_OBJECT dat_0cbb791c = {
#include "ev0072/amy_action_8/dat_0cbb791c.inc"
};
static NJS_OBJECT dat_0cbb7950 = {
#include "ev0072/amy_action_8/dat_0cbb7950.inc"
};
static NJS_OBJECT dat_0cbb7984 = {
#include "ev0072/amy_action_8/dat_0cbb7984.inc"
};
static NJS_MATERIAL dat_0cbb79b8[2] = {
#include "ev0072/amy_action_8/dat_0cbb79b8.inc"
};
static Sint16 dat_0cbb79e0[54] = {
#include "ev0072/amy_action_8/dat_0cbb79e0.inc"
};
static Sint16 dat_0cbb7a4c[22] = {
#include "ev0072/amy_action_8/dat_0cbb7a4c.inc"
};
static NJS_COLOR dat_0cbb7a78[48] = {
#include "ev0072/amy_action_8/dat_0cbb7a78.inc"
};
static NJS_COLOR dat_0cbb7b38[18] = {
#include "ev0072/amy_action_8/dat_0cbb7b38.inc"
};
static NJS_MESHSET dat_0cbb7b80[2] = {
#include "ev0072/amy_action_8/dat_0cbb7b80.inc"
};
static NJS_POINT3 dat_0cbb7bb0[26] = {
#include "ev0072/amy_action_8/dat_0cbb7bb0.inc"
};
static NJS_VECTOR dat_0cbb7ce8[26] = {
#include "ev0072/amy_action_8/dat_0cbb7ce8.inc"
};
static NJS_MODEL dat_0cbb7e20 = {
#include "ev0072/amy_action_8/dat_0cbb7e20.inc"
};
static NJS_OBJECT dat_0cbb7e48 = {
#include "ev0072/amy_action_8/dat_0cbb7e48.inc"
};
static NJS_OBJECT dat_0cbb7e7c = {
#include "ev0072/amy_action_8/dat_0cbb7e7c.inc"
};
static NJS_OBJECT dat_0cbb7eb0 = {
#include "ev0072/amy_action_8/dat_0cbb7eb0.inc"
};
static NJS_OBJECT dat_0cbb7ee4 = {
#include "ev0072/amy_action_8/dat_0cbb7ee4.inc"
};
static NJS_OBJECT ev0072_bird_ver4_object = {
#include "ev0072/amy_action_8/ev0072_bird_ver4_object.inc"
};
static NJS_MKEY_F dat_0cbb7f4c[2] = {
#include "ev0072/amy_action_8/dat_0cbb7f4c.inc"
};
static NJS_MKEY_A dat_0cbb7f6c[2] = {
#include "ev0072/amy_action_8/dat_0cbb7f6c.inc"
};
static NJS_MKEY_F dat_0cbb7f8c[2] = {
#include "ev0072/amy_action_8/dat_0cbb7f8c.inc"
};
static NJS_MKEY_A dat_0cbb7fac[2] = {
#include "ev0072/amy_action_8/dat_0cbb7fac.inc"
};
static NJS_MKEY_F dat_0cbb7fcc[2] = {
#include "ev0072/amy_action_8/dat_0cbb7fcc.inc"
};
static NJS_MKEY_A dat_0cbb7fec[2] = {
#include "ev0072/amy_action_8/dat_0cbb7fec.inc"
};
static NJS_MKEY_F dat_0cbb800c[2] = {
#include "ev0072/amy_action_8/dat_0cbb800c.inc"
};
static NJS_MKEY_A dat_0cbb802c[2] = {
#include "ev0072/amy_action_8/dat_0cbb802c.inc"
};
static NJS_MKEY_F dat_0cbb804c[2] = {
#include "ev0072/amy_action_8/dat_0cbb804c.inc"
};
static NJS_MKEY_A dat_0cbb806c[2] = {
#include "ev0072/amy_action_8/dat_0cbb806c.inc"
};
static NJS_MKEY_F dat_0cbb808c[2] = {
#include "ev0072/amy_action_8/dat_0cbb808c.inc"
};
static NJS_MKEY_A dat_0cbb80ac[2] = {
#include "ev0072/amy_action_8/dat_0cbb80ac.inc"
};
static NJS_MKEY_F dat_0cbb80cc[2] = {
#include "ev0072/amy_action_8/dat_0cbb80cc.inc"
};
static NJS_MKEY_A dat_0cbb80ec[2] = {
#include "ev0072/amy_action_8/dat_0cbb80ec.inc"
};
static NJS_MKEY_F dat_0cbb810c[2] = {
#include "ev0072/amy_action_8/dat_0cbb810c.inc"
};
static NJS_MKEY_A dat_0cbb812c[2] = {
#include "ev0072/amy_action_8/dat_0cbb812c.inc"
};
static NJS_MKEY_F dat_0cbb814c[2] = {
#include "ev0072/amy_action_8/dat_0cbb814c.inc"
};
static NJS_MKEY_A dat_0cbb816c[2] = {
#include "ev0072/amy_action_8/dat_0cbb816c.inc"
};
static NJS_MKEY_F dat_0cbb818c[2] = {
#include "ev0072/amy_action_8/dat_0cbb818c.inc"
};
static NJS_MKEY_A dat_0cbb81ac[2] = {
#include "ev0072/amy_action_8/dat_0cbb81ac.inc"
};
static NJS_MKEY_F dat_0cbb81cc[2] = {
#include "ev0072/amy_action_8/dat_0cbb81cc.inc"
};
static NJS_MKEY_A dat_0cbb81ec[2] = {
#include "ev0072/amy_action_8/dat_0cbb81ec.inc"
};
static NJS_MKEY_F dat_0cbb820c[2] = {
#include "ev0072/amy_action_8/dat_0cbb820c.inc"
};
static NJS_MKEY_A dat_0cbb822c[2] = {
#include "ev0072/amy_action_8/dat_0cbb822c.inc"
};
static NJS_MKEY_F dat_0cbb824c[2] = {
#include "ev0072/amy_action_8/dat_0cbb824c.inc"
};
static NJS_MKEY_A dat_0cbb826c[2] = {
#include "ev0072/amy_action_8/dat_0cbb826c.inc"
};
static NJS_MKEY_F dat_0cbb828c[2] = {
#include "ev0072/amy_action_8/dat_0cbb828c.inc"
};
static NJS_MKEY_A dat_0cbb82ac[2] = {
#include "ev0072/amy_action_8/dat_0cbb82ac.inc"
};
static NJS_MKEY_F dat_0cbb82cc[2] = {
#include "ev0072/amy_action_8/dat_0cbb82cc.inc"
};
static NJS_MKEY_A dat_0cbb82ec[2] = {
#include "ev0072/amy_action_8/dat_0cbb82ec.inc"
};
static NJS_MKEY_F dat_0cbb830c[2] = {
#include "ev0072/amy_action_8/dat_0cbb830c.inc"
};
static NJS_MKEY_A dat_0cbb832c[2] = {
#include "ev0072/amy_action_8/dat_0cbb832c.inc"
};
static NJS_MKEY_F dat_0cbb834c[2] = {
#include "ev0072/amy_action_8/dat_0cbb834c.inc"
};
static NJS_MKEY_A dat_0cbb836c[2] = {
#include "ev0072/amy_action_8/dat_0cbb836c.inc"
};
static NJS_MKEY_F dat_0cbb838c[2] = {
#include "ev0072/amy_action_8/dat_0cbb838c.inc"
};
static NJS_MKEY_A dat_0cbb83ac[2] = {
#include "ev0072/amy_action_8/dat_0cbb83ac.inc"
};
static NJS_MKEY_F dat_0cbb83cc[2] = {
#include "ev0072/amy_action_8/dat_0cbb83cc.inc"
};
static NJS_MKEY_A dat_0cbb83ec[2] = {
#include "ev0072/amy_action_8/dat_0cbb83ec.inc"
};
static NJS_MKEY_F dat_0cbb840c[2] = {
#include "ev0072/amy_action_8/dat_0cbb840c.inc"
};
static NJS_MKEY_A dat_0cbb842c[2] = {
#include "ev0072/amy_action_8/dat_0cbb842c.inc"
};
static NJS_MKEY_F dat_0cbb844c[2] = {
#include "ev0072/amy_action_8/dat_0cbb844c.inc"
};
static NJS_MKEY_A dat_0cbb846c[2] = {
#include "ev0072/amy_action_8/dat_0cbb846c.inc"
};
static NJS_MKEY_F dat_0cbb848c[2] = {
#include "ev0072/amy_action_8/dat_0cbb848c.inc"
};
static NJS_MKEY_A dat_0cbb84ac[2] = {
#include "ev0072/amy_action_8/dat_0cbb84ac.inc"
};
static NJS_MKEY_F dat_0cbb84cc[2] = {
#include "ev0072/amy_action_8/dat_0cbb84cc.inc"
};
static NJS_MKEY_A dat_0cbb84ec[2] = {
#include "ev0072/amy_action_8/dat_0cbb84ec.inc"
};
static NJS_MKEY_F dat_0cbb850c[2] = {
#include "ev0072/amy_action_8/dat_0cbb850c.inc"
};
static NJS_MKEY_A dat_0cbb852c[2] = {
#include "ev0072/amy_action_8/dat_0cbb852c.inc"
};
static NJS_MKEY_F dat_0cbb854c[2] = {
#include "ev0072/amy_action_8/dat_0cbb854c.inc"
};
static NJS_MKEY_A dat_0cbb856c[2] = {
#include "ev0072/amy_action_8/dat_0cbb856c.inc"
};
static NJS_MKEY_F dat_0cbb858c[2] = {
#include "ev0072/amy_action_8/dat_0cbb858c.inc"
};
static NJS_MKEY_A dat_0cbb85ac[2] = {
#include "ev0072/amy_action_8/dat_0cbb85ac.inc"
};
static NJS_MKEY_F dat_0cbb85cc[2] = {
#include "ev0072/amy_action_8/dat_0cbb85cc.inc"
};
static NJS_MKEY_A dat_0cbb85ec[2] = {
#include "ev0072/amy_action_8/dat_0cbb85ec.inc"
};
static NJS_MKEY_F dat_0cbb860c[2] = {
#include "ev0072/amy_action_8/dat_0cbb860c.inc"
};
static NJS_MKEY_A dat_0cbb862c[2] = {
#include "ev0072/amy_action_8/dat_0cbb862c.inc"
};
static NJS_MKEY_F dat_0cbb864c[2] = {
#include "ev0072/amy_action_8/dat_0cbb864c.inc"
};
static NJS_MKEY_A dat_0cbb866c[2] = {
#include "ev0072/amy_action_8/dat_0cbb866c.inc"
};
static NJS_MKEY_F dat_0cbb868c[2] = {
#include "ev0072/amy_action_8/dat_0cbb868c.inc"
};
static NJS_MKEY_A dat_0cbb86ac[2] = {
#include "ev0072/amy_action_8/dat_0cbb86ac.inc"
};
static NJS_MKEY_F dat_0cbb86cc[2] = {
#include "ev0072/amy_action_8/dat_0cbb86cc.inc"
};
static NJS_MKEY_A dat_0cbb86ec[2] = {
#include "ev0072/amy_action_8/dat_0cbb86ec.inc"
};
static NJS_MKEY_F dat_0cbb870c[2] = {
#include "ev0072/amy_action_8/dat_0cbb870c.inc"
};
static NJS_MKEY_A dat_0cbb872c[2] = {
#include "ev0072/amy_action_8/dat_0cbb872c.inc"
};
static NJS_MKEY_F dat_0cbb874c[2] = {
#include "ev0072/amy_action_8/dat_0cbb874c.inc"
};
static NJS_MKEY_A dat_0cbb876c[2] = {
#include "ev0072/amy_action_8/dat_0cbb876c.inc"
};
static NJS_MKEY_F dat_0cbb878c[2] = {
#include "ev0072/amy_action_8/dat_0cbb878c.inc"
};
static NJS_MKEY_A dat_0cbb87ac[2] = {
#include "ev0072/amy_action_8/dat_0cbb87ac.inc"
};
static NJS_MKEY_F dat_0cbb87cc[2] = {
#include "ev0072/amy_action_8/dat_0cbb87cc.inc"
};
static NJS_MKEY_A dat_0cbb87ec[2] = {
#include "ev0072/amy_action_8/dat_0cbb87ec.inc"
};
static NJS_MKEY_F dat_0cbb880c[2] = {
#include "ev0072/amy_action_8/dat_0cbb880c.inc"
};
static NJS_MKEY_A dat_0cbb882c[2] = {
#include "ev0072/amy_action_8/dat_0cbb882c.inc"
};
static NJS_MKEY_F dat_0cbb884c[2] = {
#include "ev0072/amy_action_8/dat_0cbb884c.inc"
};
static NJS_MKEY_A dat_0cbb886c[2] = {
#include "ev0072/amy_action_8/dat_0cbb886c.inc"
};
static NJS_MKEY_F dat_0cbb888c[2] = {
#include "ev0072/amy_action_8/dat_0cbb888c.inc"
};
static NJS_MKEY_A dat_0cbb88ac[2] = {
#include "ev0072/amy_action_8/dat_0cbb88ac.inc"
};
static NJS_MKEY_F dat_0cbb88cc[2] = {
#include "ev0072/amy_action_8/dat_0cbb88cc.inc"
};
static NJS_MKEY_A dat_0cbb88ec[2] = {
#include "ev0072/amy_action_8/dat_0cbb88ec.inc"
};
static NJS_MKEY_F dat_0cbb890c[2] = {
#include "ev0072/amy_action_8/dat_0cbb890c.inc"
};
static NJS_MKEY_A dat_0cbb892c[2] = {
#include "ev0072/amy_action_8/dat_0cbb892c.inc"
};
static NJS_MKEY_F dat_0cbb894c[2] = {
#include "ev0072/amy_action_8/dat_0cbb894c.inc"
};
static NJS_MKEY_A dat_0cbb896c[2] = {
#include "ev0072/amy_action_8/dat_0cbb896c.inc"
};
static NJS_MKEY_F dat_0cbb898c[2] = {
#include "ev0072/amy_action_8/dat_0cbb898c.inc"
};
static NJS_MKEY_A dat_0cbb89ac[2] = {
#include "ev0072/amy_action_8/dat_0cbb89ac.inc"
};
static NJS_MKEY_F dat_0cbb89cc[2] = {
#include "ev0072/amy_action_8/dat_0cbb89cc.inc"
};
static NJS_MKEY_A dat_0cbb89ec[2] = {
#include "ev0072/amy_action_8/dat_0cbb89ec.inc"
};
static NJS_MKEY_F dat_0cbb8a0c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8a0c.inc"
};
static NJS_MKEY_A dat_0cbb8a2c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8a2c.inc"
};
static NJS_MKEY_F dat_0cbb8a4c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8a4c.inc"
};
static NJS_MKEY_A dat_0cbb8a6c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8a6c.inc"
};
static NJS_MKEY_F dat_0cbb8a8c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8a8c.inc"
};
static NJS_MKEY_A dat_0cbb8aac[2] = {
#include "ev0072/amy_action_8/dat_0cbb8aac.inc"
};
static NJS_MKEY_F dat_0cbb8acc[2] = {
#include "ev0072/amy_action_8/dat_0cbb8acc.inc"
};
static NJS_MKEY_A dat_0cbb8aec[2] = {
#include "ev0072/amy_action_8/dat_0cbb8aec.inc"
};
static NJS_MKEY_F dat_0cbb8b0c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8b0c.inc"
};
static NJS_MKEY_A dat_0cbb8b2c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8b2c.inc"
};
static NJS_MKEY_F dat_0cbb8b4c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8b4c.inc"
};
static NJS_MKEY_A dat_0cbb8b6c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8b6c.inc"
};
static NJS_MKEY_F dat_0cbb8b8c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8b8c.inc"
};
static NJS_MKEY_A dat_0cbb8bac[2] = {
#include "ev0072/amy_action_8/dat_0cbb8bac.inc"
};
static NJS_MKEY_F dat_0cbb8bcc[2] = {
#include "ev0072/amy_action_8/dat_0cbb8bcc.inc"
};
static NJS_MKEY_A dat_0cbb8bec[2] = {
#include "ev0072/amy_action_8/dat_0cbb8bec.inc"
};
static NJS_MKEY_F dat_0cbb8c0c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8c0c.inc"
};
static NJS_MKEY_A dat_0cbb8c2c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8c2c.inc"
};
static NJS_MKEY_F dat_0cbb8c4c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8c4c.inc"
};
static NJS_MKEY_A dat_0cbb8c6c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8c6c.inc"
};
static NJS_MKEY_F dat_0cbb8c8c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8c8c.inc"
};
static NJS_MKEY_A dat_0cbb8cac[2] = {
#include "ev0072/amy_action_8/dat_0cbb8cac.inc"
};
static NJS_MKEY_F dat_0cbb8ccc[2] = {
#include "ev0072/amy_action_8/dat_0cbb8ccc.inc"
};
static NJS_MKEY_A dat_0cbb8cec[2] = {
#include "ev0072/amy_action_8/dat_0cbb8cec.inc"
};
static NJS_MKEY_F dat_0cbb8d0c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8d0c.inc"
};
static NJS_MKEY_A dat_0cbb8d2c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8d2c.inc"
};
static NJS_MKEY_F dat_0cbb8d4c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8d4c.inc"
};
static NJS_MKEY_A dat_0cbb8d6c[2] = {
#include "ev0072/amy_action_8/dat_0cbb8d6c.inc"
};
static NJS_MDATA2 dat_0cbb8d8c[57] = {
#include "ev0072/amy_action_8/dat_0cbb8d8c.inc"
};
static NJS_MOTION dat_0cbb911c = {
#include "ev0072/amy_action_8/dat_0cbb911c.inc"
};
static NJS_ACTION dat_0cbb9128 = {
#include "ev0072/amy_action_8/dat_0cbb9128.inc"
};
static NJS_MKEY_F dat_0cbb9130[60] = {
#include "ev0072/amy_action_8/dat_0cbb9130.inc"
};
static NJS_MKEY_A dat_0cbb94f0[2] = {
#include "ev0072/amy_action_8/dat_0cbb94f0.inc"
};
static NJS_MKEY_F dat_0cbb9510[2] = {
#include "ev0072/amy_action_8/dat_0cbb9510.inc"
};
static NJS_MKEY_A dat_0cbb9530[2] = {
#include "ev0072/amy_action_8/dat_0cbb9530.inc"
};
static NJS_MKEY_F dat_0cbb9550[2] = {
#include "ev0072/amy_action_8/dat_0cbb9550.inc"
};
static NJS_MKEY_A dat_0cbb9570[2] = {
#include "ev0072/amy_action_8/dat_0cbb9570.inc"
};
static NJS_MKEY_F dat_0cbb9590[2] = {
#include "ev0072/amy_action_8/dat_0cbb9590.inc"
};
static NJS_MKEY_A dat_0cbb95b0[2] = {
#include "ev0072/amy_action_8/dat_0cbb95b0.inc"
};
static NJS_MKEY_F dat_0cbb95d0[2] = {
#include "ev0072/amy_action_8/dat_0cbb95d0.inc"
};
static NJS_MKEY_A dat_0cbb95f0[2] = {
#include "ev0072/amy_action_8/dat_0cbb95f0.inc"
};
static NJS_MKEY_F dat_0cbb9610[2] = {
#include "ev0072/amy_action_8/dat_0cbb9610.inc"
};
static NJS_MKEY_A dat_0cbb9630[2] = {
#include "ev0072/amy_action_8/dat_0cbb9630.inc"
};
static NJS_MKEY_F dat_0cbb9650[2] = {
#include "ev0072/amy_action_8/dat_0cbb9650.inc"
};
static NJS_MKEY_A dat_0cbb9670[59] = {
#include "ev0072/amy_action_8/dat_0cbb9670.inc"
};
static NJS_MKEY_F dat_0cbb9a20[2] = {
#include "ev0072/amy_action_8/dat_0cbb9a20.inc"
};
static NJS_MKEY_A dat_0cbb9a40[2] = {
#include "ev0072/amy_action_8/dat_0cbb9a40.inc"
};
static NJS_MKEY_F dat_0cbb9a60[2] = {
#include "ev0072/amy_action_8/dat_0cbb9a60.inc"
};
static NJS_MKEY_A dat_0cbb9a80[2] = {
#include "ev0072/amy_action_8/dat_0cbb9a80.inc"
};
static NJS_MKEY_F dat_0cbb9aa0[2] = {
#include "ev0072/amy_action_8/dat_0cbb9aa0.inc"
};
static NJS_MKEY_A dat_0cbb9ac0[2] = {
#include "ev0072/amy_action_8/dat_0cbb9ac0.inc"
};
static NJS_MKEY_F dat_0cbb9ae0[2] = {
#include "ev0072/amy_action_8/dat_0cbb9ae0.inc"
};
static NJS_MKEY_A dat_0cbb9b00[59] = {
#include "ev0072/amy_action_8/dat_0cbb9b00.inc"
};
static NJS_MKEY_F dat_0cbb9eb0[2] = {
#include "ev0072/amy_action_8/dat_0cbb9eb0.inc"
};
static NJS_MKEY_A dat_0cbb9ed0[2] = {
#include "ev0072/amy_action_8/dat_0cbb9ed0.inc"
};
static NJS_MKEY_F dat_0cbb9ef0[2] = {
#include "ev0072/amy_action_8/dat_0cbb9ef0.inc"
};
static NJS_MKEY_A dat_0cbb9f10[2] = {
#include "ev0072/amy_action_8/dat_0cbb9f10.inc"
};
static NJS_MKEY_F dat_0cbb9f30[2] = {
#include "ev0072/amy_action_8/dat_0cbb9f30.inc"
};
static NJS_MKEY_A dat_0cbb9f50[2] = {
#include "ev0072/amy_action_8/dat_0cbb9f50.inc"
};
static NJS_MKEY_F dat_0cbb9f70[2] = {
#include "ev0072/amy_action_8/dat_0cbb9f70.inc"
};
static NJS_MKEY_A dat_0cbb9f90[60] = {
#include "ev0072/amy_action_8/dat_0cbb9f90.inc"
};
static NJS_MKEY_F dat_0cbba350[2] = {
#include "ev0072/amy_action_8/dat_0cbba350.inc"
};
static NJS_MKEY_A dat_0cbba370[2] = {
#include "ev0072/amy_action_8/dat_0cbba370.inc"
};
static NJS_MKEY_F dat_0cbba390[2] = {
#include "ev0072/amy_action_8/dat_0cbba390.inc"
};
static NJS_MKEY_A dat_0cbba3b0[2] = {
#include "ev0072/amy_action_8/dat_0cbba3b0.inc"
};
static NJS_MKEY_F dat_0cbba3d0[2] = {
#include "ev0072/amy_action_8/dat_0cbba3d0.inc"
};
static NJS_MKEY_A dat_0cbba3f0[2] = {
#include "ev0072/amy_action_8/dat_0cbba3f0.inc"
};
static NJS_MKEY_F dat_0cbba410[2] = {
#include "ev0072/amy_action_8/dat_0cbba410.inc"
};
static NJS_MKEY_A dat_0cbba430[2] = {
#include "ev0072/amy_action_8/dat_0cbba430.inc"
};
static NJS_MKEY_F dat_0cbba450[2] = {
#include "ev0072/amy_action_8/dat_0cbba450.inc"
};
static NJS_MKEY_A dat_0cbba470[2] = {
#include "ev0072/amy_action_8/dat_0cbba470.inc"
};
static NJS_MKEY_F dat_0cbba490[2] = {
#include "ev0072/amy_action_8/dat_0cbba490.inc"
};
static NJS_MKEY_A dat_0cbba4b0[2] = {
#include "ev0072/amy_action_8/dat_0cbba4b0.inc"
};
static NJS_MKEY_F dat_0cbba4d0[2] = {
#include "ev0072/amy_action_8/dat_0cbba4d0.inc"
};
static NJS_MKEY_A dat_0cbba4f0[2] = {
#include "ev0072/amy_action_8/dat_0cbba4f0.inc"
};
static NJS_MKEY_F dat_0cbba510[2] = {
#include "ev0072/amy_action_8/dat_0cbba510.inc"
};
static NJS_MKEY_A dat_0cbba530[2] = {
#include "ev0072/amy_action_8/dat_0cbba530.inc"
};
static NJS_MKEY_F dat_0cbba550[2] = {
#include "ev0072/amy_action_8/dat_0cbba550.inc"
};
static NJS_MKEY_A dat_0cbba570[2] = {
#include "ev0072/amy_action_8/dat_0cbba570.inc"
};
static NJS_MKEY_F dat_0cbba590[2] = {
#include "ev0072/amy_action_8/dat_0cbba590.inc"
};
static NJS_MKEY_A dat_0cbba5b0[2] = {
#include "ev0072/amy_action_8/dat_0cbba5b0.inc"
};
static NJS_MKEY_F dat_0cbba5d0[2] = {
#include "ev0072/amy_action_8/dat_0cbba5d0.inc"
};
static NJS_MKEY_A dat_0cbba5f0[60] = {
#include "ev0072/amy_action_8/dat_0cbba5f0.inc"
};
static NJS_MKEY_F dat_0cbba9b0[2] = {
#include "ev0072/amy_action_8/dat_0cbba9b0.inc"
};
static NJS_MKEY_A dat_0cbba9d0[2] = {
#include "ev0072/amy_action_8/dat_0cbba9d0.inc"
};
static NJS_MKEY_F dat_0cbba9f0[2] = {
#include "ev0072/amy_action_8/dat_0cbba9f0.inc"
};
static NJS_MKEY_A dat_0cbbaa10[2] = {
#include "ev0072/amy_action_8/dat_0cbbaa10.inc"
};
static NJS_MKEY_F dat_0cbbaa30[2] = {
#include "ev0072/amy_action_8/dat_0cbbaa30.inc"
};
static NJS_MKEY_A dat_0cbbaa50[2] = {
#include "ev0072/amy_action_8/dat_0cbbaa50.inc"
};
static NJS_MKEY_F dat_0cbbaa70[2] = {
#include "ev0072/amy_action_8/dat_0cbbaa70.inc"
};
static NJS_MKEY_A dat_0cbbaa90[2] = {
#include "ev0072/amy_action_8/dat_0cbbaa90.inc"
};
static NJS_MKEY_F dat_0cbbaab0[2] = {
#include "ev0072/amy_action_8/dat_0cbbaab0.inc"
};
static NJS_MKEY_A dat_0cbbaad0[2] = {
#include "ev0072/amy_action_8/dat_0cbbaad0.inc"
};
static NJS_MKEY_F dat_0cbbaaf0[2] = {
#include "ev0072/amy_action_8/dat_0cbbaaf0.inc"
};
static NJS_MKEY_A dat_0cbbab10[2] = {
#include "ev0072/amy_action_8/dat_0cbbab10.inc"
};
static NJS_MKEY_F dat_0cbbab30[2] = {
#include "ev0072/amy_action_8/dat_0cbbab30.inc"
};
static NJS_MKEY_A dat_0cbbab50[2] = {
#include "ev0072/amy_action_8/dat_0cbbab50.inc"
};
static NJS_MKEY_F dat_0cbbab70[2] = {
#include "ev0072/amy_action_8/dat_0cbbab70.inc"
};
static NJS_MKEY_A dat_0cbbab90[60] = {
#include "ev0072/amy_action_8/dat_0cbbab90.inc"
};
static NJS_MKEY_F dat_0cbbaf50[2] = {
#include "ev0072/amy_action_8/dat_0cbbaf50.inc"
};
static NJS_MKEY_A dat_0cbbaf70[2] = {
#include "ev0072/amy_action_8/dat_0cbbaf70.inc"
};
static NJS_MKEY_F dat_0cbbaf90[2] = {
#include "ev0072/amy_action_8/dat_0cbbaf90.inc"
};
static NJS_MKEY_A dat_0cbbafb0[2] = {
#include "ev0072/amy_action_8/dat_0cbbafb0.inc"
};
static NJS_MKEY_F dat_0cbbafd0[60] = {
#include "ev0072/amy_action_8/dat_0cbbafd0.inc"
};
static NJS_MKEY_A dat_0cbbb390[2] = {
#include "ev0072/amy_action_8/dat_0cbbb390.inc"
};
static NJS_MKEY_F dat_0cbbb3b0[59] = {
#include "ev0072/amy_action_8/dat_0cbbb3b0.inc"
};
static NJS_MKEY_A dat_0cbbb760[2] = {
#include "ev0072/amy_action_8/dat_0cbbb760.inc"
};
static NJS_MKEY_F dat_0cbbb780[60] = {
#include "ev0072/amy_action_8/dat_0cbbb780.inc"
};
static NJS_MKEY_A dat_0cbbbb40[2] = {
#include "ev0072/amy_action_8/dat_0cbbbb40.inc"
};
static NJS_MKEY_F dat_0cbbbb60[60] = {
#include "ev0072/amy_action_8/dat_0cbbbb60.inc"
};
static NJS_MKEY_A dat_0cbbbf20[2] = {
#include "ev0072/amy_action_8/dat_0cbbbf20.inc"
};
static NJS_MKEY_F dat_0cbbbf40[60] = {
#include "ev0072/amy_action_8/dat_0cbbbf40.inc"
};
static NJS_MKEY_A dat_0cbbc300[2] = {
#include "ev0072/amy_action_8/dat_0cbbc300.inc"
};
static NJS_MKEY_F dat_0cbbc320[2] = {
#include "ev0072/amy_action_8/dat_0cbbc320.inc"
};
static NJS_MKEY_A dat_0cbbc340[2] = {
#include "ev0072/amy_action_8/dat_0cbbc340.inc"
};
static NJS_MKEY_F dat_0cbbc360[2] = {
#include "ev0072/amy_action_8/dat_0cbbc360.inc"
};
static NJS_MKEY_A dat_0cbbc380[60] = {
#include "ev0072/amy_action_8/dat_0cbbc380.inc"
};
static NJS_MKEY_F dat_0cbbc740[2] = {
#include "ev0072/amy_action_8/dat_0cbbc740.inc"
};
static NJS_MKEY_A dat_0cbbc760[2] = {
#include "ev0072/amy_action_8/dat_0cbbc760.inc"
};
static NJS_MKEY_F dat_0cbbc780[2] = {
#include "ev0072/amy_action_8/dat_0cbbc780.inc"
};
static NJS_MKEY_A dat_0cbbc7a0[2] = {
#include "ev0072/amy_action_8/dat_0cbbc7a0.inc"
};
static NJS_MKEY_F dat_0cbbc7c0[2] = {
#include "ev0072/amy_action_8/dat_0cbbc7c0.inc"
};
static NJS_MKEY_A dat_0cbbc7e0[60] = {
#include "ev0072/amy_action_8/dat_0cbbc7e0.inc"
};
static NJS_MKEY_F dat_0cbbcba0[2] = {
#include "ev0072/amy_action_8/dat_0cbbcba0.inc"
};
static NJS_MKEY_A dat_0cbbcbc0[2] = {
#include "ev0072/amy_action_8/dat_0cbbcbc0.inc"
};
static NJS_MKEY_F dat_0cbbcbe0[2] = {
#include "ev0072/amy_action_8/dat_0cbbcbe0.inc"
};
static NJS_MKEY_A dat_0cbbcc00[2] = {
#include "ev0072/amy_action_8/dat_0cbbcc00.inc"
};
static NJS_MKEY_F dat_0cbbcc20[2] = {
#include "ev0072/amy_action_8/dat_0cbbcc20.inc"
};
static NJS_MKEY_A dat_0cbbcc40[2] = {
#include "ev0072/amy_action_8/dat_0cbbcc40.inc"
};
static NJS_MKEY_F dat_0cbbcc60[2] = {
#include "ev0072/amy_action_8/dat_0cbbcc60.inc"
};
static NJS_MKEY_A dat_0cbbcc80[2] = {
#include "ev0072/amy_action_8/dat_0cbbcc80.inc"
};
static NJS_MKEY_F dat_0cbbcca0[2] = {
#include "ev0072/amy_action_8/dat_0cbbcca0.inc"
};
static NJS_MKEY_A dat_0cbbccc0[60] = {
#include "ev0072/amy_action_8/dat_0cbbccc0.inc"
};
static NJS_MKEY_F dat_0cbbd080[2] = {
#include "ev0072/amy_action_8/dat_0cbbd080.inc"
};
static NJS_MKEY_A dat_0cbbd0a0[2] = {
#include "ev0072/amy_action_8/dat_0cbbd0a0.inc"
};
static NJS_MKEY_F dat_0cbbd0c0[2] = {
#include "ev0072/amy_action_8/dat_0cbbd0c0.inc"
};
static NJS_MKEY_A dat_0cbbd0e0[2] = {
#include "ev0072/amy_action_8/dat_0cbbd0e0.inc"
};
static NJS_MKEY_F dat_0cbbd100[2] = {
#include "ev0072/amy_action_8/dat_0cbbd100.inc"
};
static NJS_MKEY_A dat_0cbbd120[60] = {
#include "ev0072/amy_action_8/dat_0cbbd120.inc"
};
static NJS_MKEY_F dat_0cbbd4e0[2] = {
#include "ev0072/amy_action_8/dat_0cbbd4e0.inc"
};
static NJS_MKEY_A dat_0cbbd500[2] = {
#include "ev0072/amy_action_8/dat_0cbbd500.inc"
};
static NJS_MKEY_F dat_0cbbd520[2] = {
#include "ev0072/amy_action_8/dat_0cbbd520.inc"
};
static NJS_MKEY_A dat_0cbbd540[2] = {
#include "ev0072/amy_action_8/dat_0cbbd540.inc"
};
static NJS_MKEY_F dat_0cbbd560[2] = {
#include "ev0072/amy_action_8/dat_0cbbd560.inc"
};
static NJS_MKEY_A dat_0cbbd580[2] = {
#include "ev0072/amy_action_8/dat_0cbbd580.inc"
};
static NJS_MDATA2 dat_0cbbd5a0[57] = {
#include "ev0072/amy_action_8/dat_0cbbd5a0.inc"
};
static NJS_MOTION ev0072_bird_ver4_motion_0 = {
#include "ev0072/amy_action_8/ev0072_bird_ver4_motion_0.inc"
};
NJS_ACTION ev0072_bird_ver4_action_0 = {
#include "ev0072/amy_action_8/ev0072_bird_ver4_action_0.inc"
};
static NJS_MKEY_F dat_0cbbd944[2] = {
#include "ev0072/amy_action_8/dat_0cbbd944.inc"
};
static NJS_MKEY_F dat_0cbbd964[30] = {
#include "ev0072/amy_action_8/dat_0cbbd964.inc"
};
static NJS_MKEY_F dat_0cbbdb44[2] = {
#include "ev0072/amy_action_8/dat_0cbbdb44.inc"
};
static NJS_MKEY_F dat_0cbbdb64[2] = {
#include "ev0072/amy_action_8/dat_0cbbdb64.inc"
};
static NJS_MKEY_F dat_0cbbdb84[2] = {
#include "ev0072/amy_action_8/dat_0cbbdb84.inc"
};
static NJS_MKEY_F dat_0cbbdba4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdba4.inc"
};
static NJS_MKEY_F dat_0cbbdbc4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdbc4.inc"
};
static NJS_MKEY_F dat_0cbbdbe4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdbe4.inc"
};
static NJS_MKEY_F dat_0cbbdc04[2] = {
#include "ev0072/amy_action_8/dat_0cbbdc04.inc"
};
static NJS_MKEY_F dat_0cbbdc24[2] = {
#include "ev0072/amy_action_8/dat_0cbbdc24.inc"
};
static NJS_MKEY_F dat_0cbbdc44[2] = {
#include "ev0072/amy_action_8/dat_0cbbdc44.inc"
};
static NJS_MKEY_F dat_0cbbdc64[2] = {
#include "ev0072/amy_action_8/dat_0cbbdc64.inc"
};
static NJS_MKEY_F dat_0cbbdc84[2] = {
#include "ev0072/amy_action_8/dat_0cbbdc84.inc"
};
static NJS_MKEY_F dat_0cbbdca4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdca4.inc"
};
static NJS_MKEY_F dat_0cbbdcc4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdcc4.inc"
};
static NJS_MKEY_F dat_0cbbdce4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdce4.inc"
};
static NJS_MKEY_F dat_0cbbdd04[2] = {
#include "ev0072/amy_action_8/dat_0cbbdd04.inc"
};
static NJS_MKEY_F dat_0cbbdd24[2] = {
#include "ev0072/amy_action_8/dat_0cbbdd24.inc"
};
static NJS_MKEY_F dat_0cbbdd44[2] = {
#include "ev0072/amy_action_8/dat_0cbbdd44.inc"
};
static NJS_MKEY_F dat_0cbbdd64[2] = {
#include "ev0072/amy_action_8/dat_0cbbdd64.inc"
};
static NJS_MKEY_F dat_0cbbdd84[2] = {
#include "ev0072/amy_action_8/dat_0cbbdd84.inc"
};
static NJS_MKEY_F dat_0cbbdda4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdda4.inc"
};
static NJS_MKEY_F dat_0cbbddc4[2] = {
#include "ev0072/amy_action_8/dat_0cbbddc4.inc"
};
static NJS_MKEY_F dat_0cbbdde4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdde4.inc"
};
static NJS_MKEY_F dat_0cbbde04[2] = {
#include "ev0072/amy_action_8/dat_0cbbde04.inc"
};
static NJS_MKEY_F dat_0cbbde24[2] = {
#include "ev0072/amy_action_8/dat_0cbbde24.inc"
};
static NJS_MKEY_F dat_0cbbde44[2] = {
#include "ev0072/amy_action_8/dat_0cbbde44.inc"
};
static NJS_MKEY_F dat_0cbbde64[2] = {
#include "ev0072/amy_action_8/dat_0cbbde64.inc"
};
static NJS_MKEY_F dat_0cbbde84[2] = {
#include "ev0072/amy_action_8/dat_0cbbde84.inc"
};
static NJS_MKEY_F dat_0cbbdea4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdea4.inc"
};
static NJS_MKEY_F dat_0cbbdec4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdec4.inc"
};
static NJS_MKEY_F dat_0cbbdee4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdee4.inc"
};
static NJS_MKEY_F dat_0cbbdf04[2] = {
#include "ev0072/amy_action_8/dat_0cbbdf04.inc"
};
static NJS_MKEY_F dat_0cbbdf24[2] = {
#include "ev0072/amy_action_8/dat_0cbbdf24.inc"
};
static NJS_MKEY_F dat_0cbbdf44[2] = {
#include "ev0072/amy_action_8/dat_0cbbdf44.inc"
};
static NJS_MKEY_F dat_0cbbdf64[2] = {
#include "ev0072/amy_action_8/dat_0cbbdf64.inc"
};
static NJS_MKEY_F dat_0cbbdf84[2] = {
#include "ev0072/amy_action_8/dat_0cbbdf84.inc"
};
static NJS_MKEY_F dat_0cbbdfa4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdfa4.inc"
};
static NJS_MKEY_F dat_0cbbdfc4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdfc4.inc"
};
static NJS_MKEY_F dat_0cbbdfe4[2] = {
#include "ev0072/amy_action_8/dat_0cbbdfe4.inc"
};
static NJS_MKEY_F dat_0cbbe004[2] = {
#include "ev0072/amy_action_8/dat_0cbbe004.inc"
};
static NJS_MKEY_F dat_0cbbe024[2] = {
#include "ev0072/amy_action_8/dat_0cbbe024.inc"
};
static NJS_MKEY_F dat_0cbbe044[2] = {
#include "ev0072/amy_action_8/dat_0cbbe044.inc"
};
static NJS_MKEY_F dat_0cbbe064[2] = {
#include "ev0072/amy_action_8/dat_0cbbe064.inc"
};
static NJS_MKEY_F dat_0cbbe084[2] = {
#include "ev0072/amy_action_8/dat_0cbbe084.inc"
};
static NJS_MKEY_F dat_0cbbe0a4[2] = {
#include "ev0072/amy_action_8/dat_0cbbe0a4.inc"
};
static NJS_MKEY_F dat_0cbbe0c4[2] = {
#include "ev0072/amy_action_8/dat_0cbbe0c4.inc"
};
static NJS_MKEY_F dat_0cbbe0e4[2] = {
#include "ev0072/amy_action_8/dat_0cbbe0e4.inc"
};
static NJS_MKEY_F dat_0cbbe104[2] = {
#include "ev0072/amy_action_8/dat_0cbbe104.inc"
};
static NJS_MKEY_F dat_0cbbe124[2] = {
#include "ev0072/amy_action_8/dat_0cbbe124.inc"
};
static NJS_MKEY_F dat_0cbbe144[2] = {
#include "ev0072/amy_action_8/dat_0cbbe144.inc"
};
static NJS_MKEY_F dat_0cbbe164[2] = {
#include "ev0072/amy_action_8/dat_0cbbe164.inc"
};
static NJS_MKEY_F dat_0cbbe184[2] = {
#include "ev0072/amy_action_8/dat_0cbbe184.inc"
};
static NJS_MKEY_F dat_0cbbe1a4[2] = {
#include "ev0072/amy_action_8/dat_0cbbe1a4.inc"
};
static NJS_MKEY_F dat_0cbbe1c4[2] = {
#include "ev0072/amy_action_8/dat_0cbbe1c4.inc"
};
static NJS_MKEY_F dat_0cbbe1e4[2] = {
#include "ev0072/amy_action_8/dat_0cbbe1e4.inc"
};
static NJS_MKEY_F dat_0cbbe204[2] = {
#include "ev0072/amy_action_8/dat_0cbbe204.inc"
};
static NJS_MDATA2 dat_0cbbe224[57] = {
#include "ev0072/amy_action_8/dat_0cbbe224.inc"
};
static NJS_MOTION ev0072_bird_ver4_motion_1 = {
#include "ev0072/amy_action_8/ev0072_bird_ver4_motion_1.inc"
};
NJS_ACTION ev0072_bird_ver4_action_1 = {
#include "ev0072/amy_action_8/ev0072_bird_ver4_action_1.inc"
};
static NJS_MKEY_F dat_0cbbe5c8[60] = {
#include "ev0072/amy_action_8/dat_0cbbe5c8.inc"
};
static NJS_MKEY_F dat_0cbbe988[13] = {
#include "ev0072/amy_action_8/dat_0cbbe988.inc"
};
static NJS_MKEY_F dat_0cbbea58[13] = {
#include "ev0072/amy_action_8/dat_0cbbea58.inc"
};
static NJS_MKEY_F dat_0cbbeb28[13] = {
#include "ev0072/amy_action_8/dat_0cbbeb28.inc"
};
static NJS_MKEY_F dat_0cbbebf8[13] = {
#include "ev0072/amy_action_8/dat_0cbbebf8.inc"
};
static NJS_MDATA2 dat_0cbbecc8[57] = {
#include "ev0072/amy_action_8/dat_0cbbecc8.inc"
};
static NJS_MOTION ev0072_bird_ver4_motion_2 = {
#include "ev0072/amy_action_8/ev0072_bird_ver4_motion_2.inc"
};
NJS_ACTION ev0072_bird_ver4_action_2 = {
#include "ev0072/amy_action_8/ev0072_bird_ver4_action_2.inc"
};
static NJS_TEXNAME ev0072_wing_p_textures[7] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_4, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_10, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0},
};
NJS_TEXLIST ev0072_wing_p_texlist = {
#include "ev0072/amy_action_8/ev0072_wing_p_texlist.inc"
};
static NJS_MATERIAL dat_0cbbf0c8[1] = {
#include "ev0072/amy_action_8/dat_0cbbf0c8.inc"
};
static Sint16 dat_0cbbf0dc[6] = {
#include "ev0072/amy_action_8/dat_0cbbf0dc.inc"
};
static NJS_COLOR dat_0cbbf0e8[4] = {
#include "ev0072/amy_action_8/dat_0cbbf0e8.inc"
};
static NJS_MESHSET dat_0cbbf0f8[1] = {
#include "ev0072/amy_action_8/dat_0cbbf0f8.inc"
};
static NJS_POINT3 dat_0cbbf110[4] = {
#include "ev0072/amy_action_8/dat_0cbbf110.inc"
};
static NJS_VECTOR dat_0cbbf140[4] = {
#include "ev0072/amy_action_8/dat_0cbbf140.inc"
};
static NJS_MODEL dat_0cbbf170 = {
#include "ev0072/amy_action_8/dat_0cbbf170.inc"
};
static NJS_OBJECT dat_0cbbf198 = {
#include "ev0072/amy_action_8/dat_0cbbf198.inc"
};
static NJS_MATERIAL dat_0cbbf1cc[1] = {
#include "ev0072/amy_action_8/dat_0cbbf1cc.inc"
};
static Sint16 dat_0cbbf1e0[12] = {
#include "ev0072/amy_action_8/dat_0cbbf1e0.inc"
};
static NJS_COLOR dat_0cbbf1f8[9] = {
#include "ev0072/amy_action_8/dat_0cbbf1f8.inc"
};
static NJS_MESHSET dat_0cbbf21c[1] = {
#include "ev0072/amy_action_8/dat_0cbbf21c.inc"
};
static NJS_POINT3 dat_0cbbf234[7] = {
#include "ev0072/amy_action_8/dat_0cbbf234.inc"
};
static NJS_VECTOR dat_0cbbf288[7] = {
#include "ev0072/amy_action_8/dat_0cbbf288.inc"
};
static NJS_MODEL dat_0cbbf2dc = {
#include "ev0072/amy_action_8/dat_0cbbf2dc.inc"
};
static NJS_OBJECT dat_0cbbf304 = {
#include "ev0072/amy_action_8/dat_0cbbf304.inc"
};
static NJS_OBJECT dat_0cbbf338 = {
#include "ev0072/amy_action_8/dat_0cbbf338.inc"
};
static NJS_OBJECT dat_0cbbf36c = {
#include "ev0072/amy_action_8/dat_0cbbf36c.inc"
};
static NJS_OBJECT dat_0cbbf3a0 = {
#include "ev0072/amy_action_8/dat_0cbbf3a0.inc"
};
static NJS_OBJECT dat_0cbbf3d4 = {
#include "ev0072/amy_action_8/dat_0cbbf3d4.inc"
};
static NJS_OBJECT dat_0cbbf408 = {
#include "ev0072/amy_action_8/dat_0cbbf408.inc"
};
static NJS_OBJECT dat_0cbbf43c = {
#include "ev0072/amy_action_8/dat_0cbbf43c.inc"
};
static NJS_MATERIAL dat_0cbbf470[1] = {
#include "ev0072/amy_action_8/dat_0cbbf470.inc"
};
static Sint16 dat_0cbbf484[6] = {
#include "ev0072/amy_action_8/dat_0cbbf484.inc"
};
static NJS_COLOR dat_0cbbf490[4] = {
#include "ev0072/amy_action_8/dat_0cbbf490.inc"
};
static NJS_MESHSET dat_0cbbf4a0[1] = {
#include "ev0072/amy_action_8/dat_0cbbf4a0.inc"
};
static NJS_POINT3 dat_0cbbf4b8[4] = {
#include "ev0072/amy_action_8/dat_0cbbf4b8.inc"
};
static NJS_VECTOR dat_0cbbf4e8[4] = {
#include "ev0072/amy_action_8/dat_0cbbf4e8.inc"
};
static NJS_MODEL dat_0cbbf518 = {
#include "ev0072/amy_action_8/dat_0cbbf518.inc"
};
static NJS_OBJECT dat_0cbbf540 = {
#include "ev0072/amy_action_8/dat_0cbbf540.inc"
};
static NJS_MATERIAL dat_0cbbf574[1] = {
#include "ev0072/amy_action_8/dat_0cbbf574.inc"
};
static Sint16 dat_0cbbf588[12] = {
#include "ev0072/amy_action_8/dat_0cbbf588.inc"
};
static NJS_COLOR dat_0cbbf5a0[9] = {
#include "ev0072/amy_action_8/dat_0cbbf5a0.inc"
};
static NJS_MESHSET dat_0cbbf5c4[1] = {
#include "ev0072/amy_action_8/dat_0cbbf5c4.inc"
};
static NJS_POINT3 dat_0cbbf5dc[7] = {
#include "ev0072/amy_action_8/dat_0cbbf5dc.inc"
};
static NJS_VECTOR dat_0cbbf630[7] = {
#include "ev0072/amy_action_8/dat_0cbbf630.inc"
};
static NJS_MODEL dat_0cbbf684 = {
#include "ev0072/amy_action_8/dat_0cbbf684.inc"
};
static NJS_OBJECT dat_0cbbf6ac = {
#include "ev0072/amy_action_8/dat_0cbbf6ac.inc"
};
static NJS_OBJECT dat_0cbbf6e0 = {
#include "ev0072/amy_action_8/dat_0cbbf6e0.inc"
};
static NJS_OBJECT dat_0cbbf714 = {
#include "ev0072/amy_action_8/dat_0cbbf714.inc"
};
static NJS_OBJECT dat_0cbbf748 = {
#include "ev0072/amy_action_8/dat_0cbbf748.inc"
};
static NJS_OBJECT dat_0cbbf77c = {
#include "ev0072/amy_action_8/dat_0cbbf77c.inc"
};
static NJS_OBJECT dat_0cbbf7b0 = {
#include "ev0072/amy_action_8/dat_0cbbf7b0.inc"
};
static NJS_OBJECT dat_0cbbf7e4 = {
#include "ev0072/amy_action_8/dat_0cbbf7e4.inc"
};
static NJS_OBJECT dat_0cbbf818 = {
#include "ev0072/amy_action_8/dat_0cbbf818.inc"
};
static NJS_OBJECT dat_0cbbf84c = {
#include "ev0072/amy_action_8/dat_0cbbf84c.inc"
};
static NJS_OBJECT dat_0cbbf880 = {
#include "ev0072/amy_action_8/dat_0cbbf880.inc"
};
static NJS_OBJECT dat_0cbbf8b4 = {
#include "ev0072/amy_action_8/dat_0cbbf8b4.inc"
};
static NJS_MATERIAL dat_0cbbf8e8[1] = {
#include "ev0072/amy_action_8/dat_0cbbf8e8.inc"
};
static Sint16 dat_0cbbf8fc[18] = {
#include "ev0072/amy_action_8/dat_0cbbf8fc.inc"
};
static NJS_COLOR dat_0cbbf920[15] = {
#include "ev0072/amy_action_8/dat_0cbbf920.inc"
};
static NJS_MESHSET dat_0cbbf95c[1] = {
#include "ev0072/amy_action_8/dat_0cbbf95c.inc"
};
static NJS_POINT3 dat_0cbbf974[7] = {
#include "ev0072/amy_action_8/dat_0cbbf974.inc"
};
static NJS_VECTOR dat_0cbbf9c8[7] = {
#include "ev0072/amy_action_8/dat_0cbbf9c8.inc"
};
static NJS_MODEL dat_0cbbfa1c = {
#include "ev0072/amy_action_8/dat_0cbbfa1c.inc"
};
static NJS_OBJECT dat_0cbbfa44 = {
#include "ev0072/amy_action_8/dat_0cbbfa44.inc"
};
static NJS_OBJECT dat_0cbbfa78 = {
#include "ev0072/amy_action_8/dat_0cbbfa78.inc"
};
static NJS_OBJECT dat_0cbbfaac = {
#include "ev0072/amy_action_8/dat_0cbbfaac.inc"
};
static NJS_OBJECT dat_0cbbfae0 = {
#include "ev0072/amy_action_8/dat_0cbbfae0.inc"
};
static NJS_MATERIAL dat_0cbbfb14[1] = {
#include "ev0072/amy_action_8/dat_0cbbfb14.inc"
};
static Sint16 dat_0cbbfb28[10] = {
#include "ev0072/amy_action_8/dat_0cbbfb28.inc"
};
static NJS_COLOR dat_0cbbfb3c[8] = {
#include "ev0072/amy_action_8/dat_0cbbfb3c.inc"
};
static NJS_MESHSET dat_0cbbfb5c[1] = {
#include "ev0072/amy_action_8/dat_0cbbfb5c.inc"
};
static NJS_POINT3 dat_0cbbfb74[8] = {
#include "ev0072/amy_action_8/dat_0cbbfb74.inc"
};
static NJS_VECTOR dat_0cbbfbd4[8] = {
#include "ev0072/amy_action_8/dat_0cbbfbd4.inc"
};
static NJS_MODEL dat_0cbbfc34 = {
#include "ev0072/amy_action_8/dat_0cbbfc34.inc"
};
static NJS_OBJECT dat_0cbbfc5c = {
#include "ev0072/amy_action_8/dat_0cbbfc5c.inc"
};
static NJS_MATERIAL dat_0cbbfc90[1] = {
#include "ev0072/amy_action_8/dat_0cbbfc90.inc"
};
static Sint16 dat_0cbbfca4[10] = {
#include "ev0072/amy_action_8/dat_0cbbfca4.inc"
};
static NJS_COLOR dat_0cbbfcb8[8] = {
#include "ev0072/amy_action_8/dat_0cbbfcb8.inc"
};
static NJS_MESHSET dat_0cbbfcd8[1] = {
#include "ev0072/amy_action_8/dat_0cbbfcd8.inc"
};
static NJS_POINT3 dat_0cbbfcf0[8] = {
#include "ev0072/amy_action_8/dat_0cbbfcf0.inc"
};
static NJS_VECTOR dat_0cbbfd50[8] = {
#include "ev0072/amy_action_8/dat_0cbbfd50.inc"
};
static NJS_MODEL dat_0cbbfdb0 = {
#include "ev0072/amy_action_8/dat_0cbbfdb0.inc"
};
static NJS_OBJECT dat_0cbbfdd8 = {
#include "ev0072/amy_action_8/dat_0cbbfdd8.inc"
};
static NJS_MATERIAL dat_0cbbfe0c[1] = {
#include "ev0072/amy_action_8/dat_0cbbfe0c.inc"
};
static Sint16 dat_0cbbfe20[16] = {
#include "ev0072/amy_action_8/dat_0cbbfe20.inc"
};
static NJS_COLOR dat_0cbbfe40[12] = {
#include "ev0072/amy_action_8/dat_0cbbfe40.inc"
};
static NJS_MESHSET dat_0cbbfe70[1] = {
#include "ev0072/amy_action_8/dat_0cbbfe70.inc"
};
static NJS_POINT3 dat_0cbbfe88[12] = {
#include "ev0072/amy_action_8/dat_0cbbfe88.inc"
};
static NJS_VECTOR dat_0cbbff18[12] = {
#include "ev0072/amy_action_8/dat_0cbbff18.inc"
};
static NJS_MODEL dat_0cbbffa8 = {
#include "ev0072/amy_action_8/dat_0cbbffa8.inc"
};
static NJS_OBJECT dat_0cbbffd0 = {
#include "ev0072/amy_action_8/dat_0cbbffd0.inc"
};
static NJS_MATERIAL dat_0cbc0004[1] = {
#include "ev0072/amy_action_8/dat_0cbc0004.inc"
};
static Sint16 dat_0cbc0018[16] = {
#include "ev0072/amy_action_8/dat_0cbc0018.inc"
};
static NJS_COLOR dat_0cbc0038[12] = {
#include "ev0072/amy_action_8/dat_0cbc0038.inc"
};
static NJS_MESHSET dat_0cbc0068[1] = {
#include "ev0072/amy_action_8/dat_0cbc0068.inc"
};
static NJS_POINT3 dat_0cbc0080[12] = {
#include "ev0072/amy_action_8/dat_0cbc0080.inc"
};
static NJS_VECTOR dat_0cbc0110[12] = {
#include "ev0072/amy_action_8/dat_0cbc0110.inc"
};
static NJS_MODEL dat_0cbc01a0 = {
#include "ev0072/amy_action_8/dat_0cbc01a0.inc"
};
static NJS_OBJECT dat_0cbc01c8 = {
#include "ev0072/amy_action_8/dat_0cbc01c8.inc"
};
static NJS_MATERIAL dat_0cbc01fc[4] = {
#include "ev0072/amy_action_8/dat_0cbc01fc.inc"
};
static Sint16 dat_0cbc024c[239] = {
#include "ev0072/amy_action_8/dat_0cbc024c.inc"
};
static Sint16 dat_0cbc042a[22] = {
#include "ev0072/amy_action_8/dat_0cbc042a.inc"
};
static Sint16 dat_0cbc0456[21] = {
#include "ev0072/amy_action_8/dat_0cbc0456.inc"
};
static Sint16 dat_0cbc0480[22] = {
#include "ev0072/amy_action_8/dat_0cbc0480.inc"
};
static NJS_COLOR dat_0cbc04ac[209] = {
#include "ev0072/amy_action_8/dat_0cbc04ac.inc"
};
static NJS_COLOR dat_0cbc07f0[20] = {
#include "ev0072/amy_action_8/dat_0cbc07f0.inc"
};
static NJS_COLOR dat_0cbc0840[16] = {
#include "ev0072/amy_action_8/dat_0cbc0840.inc"
};
static NJS_COLOR dat_0cbc0880[18] = {
#include "ev0072/amy_action_8/dat_0cbc0880.inc"
};
static NJS_MESHSET dat_0cbc08c8[4] = {
#include "ev0072/amy_action_8/dat_0cbc08c8.inc"
};
static NJS_POINT3 dat_0cbc0928[127] = {
#include "ev0072/amy_action_8/dat_0cbc0928.inc"
};
static NJS_VECTOR dat_0cbc0f1c[127] = {
#include "ev0072/amy_action_8/dat_0cbc0f1c.inc"
};
static NJS_MODEL dat_0cbc1510 = {
#include "ev0072/amy_action_8/dat_0cbc1510.inc"
};
static NJS_OBJECT dat_0cbc1538 = {
#include "ev0072/amy_action_8/dat_0cbc1538.inc"
};
static NJS_OBJECT dat_0cbc156c = {
#include "ev0072/amy_action_8/dat_0cbc156c.inc"
};
static NJS_OBJECT dat_0cbc15a0 = {
#include "ev0072/amy_action_8/dat_0cbc15a0.inc"
};
static NJS_OBJECT dat_0cbc15d4 = {
#include "ev0072/amy_action_8/dat_0cbc15d4.inc"
};
static NJS_MATERIAL dat_0cbc1608[1] = {
#include "ev0072/amy_action_8/dat_0cbc1608.inc"
};
static Sint16 dat_0cbc161c[48] = {
#include "ev0072/amy_action_8/dat_0cbc161c.inc"
};
static NJS_COLOR dat_0cbc167c[38] = {
#include "ev0072/amy_action_8/dat_0cbc167c.inc"
};
static NJS_MESHSET dat_0cbc1714[1] = {
#include "ev0072/amy_action_8/dat_0cbc1714.inc"
};
static NJS_POINT3 dat_0cbc172c[12] = {
#include "ev0072/amy_action_8/dat_0cbc172c.inc"
};
static NJS_VECTOR dat_0cbc17bc[12] = {
#include "ev0072/amy_action_8/dat_0cbc17bc.inc"
};
static NJS_MODEL dat_0cbc184c = {
#include "ev0072/amy_action_8/dat_0cbc184c.inc"
};
static NJS_OBJECT dat_0cbc1874 = {
#include "ev0072/amy_action_8/dat_0cbc1874.inc"
};
static NJS_OBJECT dat_0cbc18a8 = {
#include "ev0072/amy_action_8/dat_0cbc18a8.inc"
};
static NJS_OBJECT dat_0cbc18dc = {
#include "ev0072/amy_action_8/dat_0cbc18dc.inc"
};
static NJS_OBJECT dat_0cbc1910 = {
#include "ev0072/amy_action_8/dat_0cbc1910.inc"
};
static NJS_MATERIAL dat_0cbc1944[1] = {
#include "ev0072/amy_action_8/dat_0cbc1944.inc"
};
static Sint16 dat_0cbc1958[48] = {
#include "ev0072/amy_action_8/dat_0cbc1958.inc"
};
static NJS_COLOR dat_0cbc19b8[38] = {
#include "ev0072/amy_action_8/dat_0cbc19b8.inc"
};
static NJS_MESHSET dat_0cbc1a50[1] = {
#include "ev0072/amy_action_8/dat_0cbc1a50.inc"
};
static NJS_POINT3 dat_0cbc1a68[12] = {
#include "ev0072/amy_action_8/dat_0cbc1a68.inc"
};
static NJS_VECTOR dat_0cbc1af8[12] = {
#include "ev0072/amy_action_8/dat_0cbc1af8.inc"
};
static NJS_MODEL dat_0cbc1b88 = {
#include "ev0072/amy_action_8/dat_0cbc1b88.inc"
};
static NJS_OBJECT dat_0cbc1bb0 = {
#include "ev0072/amy_action_8/dat_0cbc1bb0.inc"
};
static NJS_OBJECT dat_0cbc1be4 = {
#include "ev0072/amy_action_8/dat_0cbc1be4.inc"
};
static NJS_OBJECT dat_0cbc1c18 = {
#include "ev0072/amy_action_8/dat_0cbc1c18.inc"
};
static NJS_OBJECT dat_0cbc1c4c = {
#include "ev0072/amy_action_8/dat_0cbc1c4c.inc"
};
static NJS_MATERIAL dat_0cbc1c80[2] = {
#include "ev0072/amy_action_8/dat_0cbc1c80.inc"
};
static Sint16 dat_0cbc1ca8[54] = {
#include "ev0072/amy_action_8/dat_0cbc1ca8.inc"
};
static Sint16 dat_0cbc1d14[22] = {
#include "ev0072/amy_action_8/dat_0cbc1d14.inc"
};
static NJS_COLOR dat_0cbc1d40[48] = {
#include "ev0072/amy_action_8/dat_0cbc1d40.inc"
};
static NJS_COLOR dat_0cbc1e00[18] = {
#include "ev0072/amy_action_8/dat_0cbc1e00.inc"
};
static NJS_MESHSET dat_0cbc1e48[2] = {
#include "ev0072/amy_action_8/dat_0cbc1e48.inc"
};
static NJS_POINT3 dat_0cbc1e78[26] = {
#include "ev0072/amy_action_8/dat_0cbc1e78.inc"
};
static NJS_VECTOR dat_0cbc1fb0[26] = {
#include "ev0072/amy_action_8/dat_0cbc1fb0.inc"
};
static NJS_MODEL dat_0cbc20e8 = {
#include "ev0072/amy_action_8/dat_0cbc20e8.inc"
};
static NJS_OBJECT dat_0cbc2110 = {
#include "ev0072/amy_action_8/dat_0cbc2110.inc"
};
static NJS_OBJECT dat_0cbc2144 = {
#include "ev0072/amy_action_8/dat_0cbc2144.inc"
};
static NJS_OBJECT dat_0cbc2178 = {
#include "ev0072/amy_action_8/dat_0cbc2178.inc"
};
static NJS_OBJECT dat_0cbc21ac = {
#include "ev0072/amy_action_8/dat_0cbc21ac.inc"
};
static NJS_OBJECT ev0072_wing_object = {
#include "ev0072/amy_action_8/ev0072_wing_object.inc"
};
static NJS_MKEY_F dat_0cbc2214[30] = {
#include "ev0072/amy_action_8/dat_0cbc2214.inc"
};
static NJS_MKEY_A dat_0cbc23f4[2] = {
#include "ev0072/amy_action_8/dat_0cbc23f4.inc"
};
static NJS_MKEY_A dat_0cbc2414[2] = {
#include "ev0072/amy_action_8/dat_0cbc2414.inc"
};
static NJS_MKEY_A dat_0cbc2434[30] = {
#include "ev0072/amy_action_8/dat_0cbc2434.inc"
};
static NJS_MKEY_A dat_0cbc2614[2] = {
#include "ev0072/amy_action_8/dat_0cbc2614.inc"
};
static NJS_MKEY_A dat_0cbc2634[2] = {
#include "ev0072/amy_action_8/dat_0cbc2634.inc"
};
static NJS_MKEY_A dat_0cbc2654[2] = {
#include "ev0072/amy_action_8/dat_0cbc2654.inc"
};
static NJS_MKEY_A dat_0cbc2674[30] = {
#include "ev0072/amy_action_8/dat_0cbc2674.inc"
};
static NJS_MKEY_A dat_0cbc2854[2] = {
#include "ev0072/amy_action_8/dat_0cbc2854.inc"
};
static NJS_MKEY_A dat_0cbc2874[2] = {
#include "ev0072/amy_action_8/dat_0cbc2874.inc"
};
static NJS_MKEY_A dat_0cbc2894[2] = {
#include "ev0072/amy_action_8/dat_0cbc2894.inc"
};
static NJS_MKEY_A dat_0cbc28b4[30] = {
#include "ev0072/amy_action_8/dat_0cbc28b4.inc"
};
static NJS_MKEY_A dat_0cbc2a94[2] = {
#include "ev0072/amy_action_8/dat_0cbc2a94.inc"
};
static NJS_MKEY_A dat_0cbc2ab4[2] = {
#include "ev0072/amy_action_8/dat_0cbc2ab4.inc"
};
static NJS_MKEY_A dat_0cbc2ad4[2] = {
#include "ev0072/amy_action_8/dat_0cbc2ad4.inc"
};
static NJS_MKEY_A dat_0cbc2af4[30] = {
#include "ev0072/amy_action_8/dat_0cbc2af4.inc"
};
static NJS_MKEY_A dat_0cbc2cd4[2] = {
#include "ev0072/amy_action_8/dat_0cbc2cd4.inc"
};
static NJS_MKEY_A dat_0cbc2cf4[2] = {
#include "ev0072/amy_action_8/dat_0cbc2cf4.inc"
};
static NJS_MKEY_A dat_0cbc2d14[2] = {
#include "ev0072/amy_action_8/dat_0cbc2d14.inc"
};
static NJS_MKEY_A dat_0cbc2d34[2] = {
#include "ev0072/amy_action_8/dat_0cbc2d34.inc"
};
static NJS_MKEY_A dat_0cbc2d54[2] = {
#include "ev0072/amy_action_8/dat_0cbc2d54.inc"
};
static NJS_MKEY_A dat_0cbc2d74[2] = {
#include "ev0072/amy_action_8/dat_0cbc2d74.inc"
};
static NJS_MKEY_A dat_0cbc2d94[2] = {
#include "ev0072/amy_action_8/dat_0cbc2d94.inc"
};
static NJS_MKEY_A dat_0cbc2db4[30] = {
#include "ev0072/amy_action_8/dat_0cbc2db4.inc"
};
static NJS_MKEY_A dat_0cbc2f94[2] = {
#include "ev0072/amy_action_8/dat_0cbc2f94.inc"
};
static NJS_MKEY_A dat_0cbc2fb4[2] = {
#include "ev0072/amy_action_8/dat_0cbc2fb4.inc"
};
static NJS_MKEY_A dat_0cbc2fd4[2] = {
#include "ev0072/amy_action_8/dat_0cbc2fd4.inc"
};
static NJS_MKEY_A dat_0cbc2ff4[2] = {
#include "ev0072/amy_action_8/dat_0cbc2ff4.inc"
};
static NJS_MKEY_A dat_0cbc3014[2] = {
#include "ev0072/amy_action_8/dat_0cbc3014.inc"
};
static NJS_MKEY_A dat_0cbc3034[2] = {
#include "ev0072/amy_action_8/dat_0cbc3034.inc"
};
static NJS_MKEY_A dat_0cbc3054[2] = {
#include "ev0072/amy_action_8/dat_0cbc3054.inc"
};
static NJS_MKEY_A dat_0cbc3074[30] = {
#include "ev0072/amy_action_8/dat_0cbc3074.inc"
};
static NJS_MKEY_A dat_0cbc3254[2] = {
#include "ev0072/amy_action_8/dat_0cbc3254.inc"
};
static NJS_MKEY_A dat_0cbc3274[2] = {
#include "ev0072/amy_action_8/dat_0cbc3274.inc"
};
static NJS_MKEY_A dat_0cbc3294[30] = {
#include "ev0072/amy_action_8/dat_0cbc3294.inc"
};
static NJS_MKEY_A dat_0cbc3474[2] = {
#include "ev0072/amy_action_8/dat_0cbc3474.inc"
};
static NJS_MKEY_A dat_0cbc3494[2] = {
#include "ev0072/amy_action_8/dat_0cbc3494.inc"
};
static NJS_MKEY_A dat_0cbc34b4[2] = {
#include "ev0072/amy_action_8/dat_0cbc34b4.inc"
};
static NJS_MKEY_A dat_0cbc34d4[2] = {
#include "ev0072/amy_action_8/dat_0cbc34d4.inc"
};
static NJS_MKEY_A dat_0cbc34f4[30] = {
#include "ev0072/amy_action_8/dat_0cbc34f4.inc"
};
static NJS_MKEY_A dat_0cbc36d4[2] = {
#include "ev0072/amy_action_8/dat_0cbc36d4.inc"
};
static NJS_MKEY_A dat_0cbc36f4[2] = {
#include "ev0072/amy_action_8/dat_0cbc36f4.inc"
};
static NJS_MKEY_A dat_0cbc3714[30] = {
#include "ev0072/amy_action_8/dat_0cbc3714.inc"
};
static NJS_MKEY_A dat_0cbc38f4[2] = {
#include "ev0072/amy_action_8/dat_0cbc38f4.inc"
};
static NJS_MKEY_A dat_0cbc3914[2] = {
#include "ev0072/amy_action_8/dat_0cbc3914.inc"
};
static NJS_MKEY_A dat_0cbc3934[2] = {
#include "ev0072/amy_action_8/dat_0cbc3934.inc"
};
static NJS_MDATA2 dat_0cbc3954[45] = {
#include "ev0072/amy_action_8/dat_0cbc3954.inc"
};
static NJS_MOTION ev0072_wing_motion = {
#include "ev0072/amy_action_8/ev0072_wing_motion.inc"
};
NJS_ACTION ev0072_wing_action = {
#include "ev0072/amy_action_8/ev0072_wing_action.inc"
};
static NJS_TEXNAME ev0072_wing_t_textures[7] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_4, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_10, 0, 0}, {TEXT_12, 0, 0}, {TEXT_14, 0, 0},
};
NJS_TEXLIST ev0072_wing_t_texlist = {
#include "ev0072/amy_action_8/ev0072_wing_t_texlist.inc"
};
NJS_MATERIAL dat_0cbc3c94[1] = {
#include "ev0072/amy_action_8/dat_0cbc3c94.inc"
};
