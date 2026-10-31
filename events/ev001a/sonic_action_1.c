/* the actions of Sonic, Amy's bird and the Egg Mobile, and the bird's model
 * and texture names. sonic_object, which ev001a_sonic_action_1 moves, lies
 * in 1ST_READ.BIN. dat_0cb9e2b8, ev001a_sonic_motion_1's NJS_MDATA2 table,
 * lies in the data before this object. the Egg Mobile's actions move
 * event_eggmobile_object, which lies in the data after this object */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev001a/sonic_action_1.h"

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
#define TEXT_13_LENGTH 14
#include "ev001a/sonic_action_1/text.inc"

extern NJS_MDATA2 dat_0cb9e2b8[];
extern NJS_OBJECT event_eggmobile_object;
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev001a_sonic_motion_1 = {
#include "ev001a/sonic_action_1/ev001a_sonic_motion_1.inc"
};
NJS_ACTION ev001a_sonic_action_1 = {
#include "ev001a/sonic_action_1/ev001a_sonic_action_1.inc"
};
static NJS_TEXNAME ev001a_bird_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev001a_bird_texlist = {
#include "ev001a/sonic_action_1/ev001a_bird_texlist.inc"
};
static NJS_MATERIAL dat_0cb9e7f4[1] = {
#include "ev001a/sonic_action_1/dat_0cb9e7f4.inc"
};
static Sint16 dat_0cb9e808[6] = {
#include "ev001a/sonic_action_1/dat_0cb9e808.inc"
};
static NJS_COLOR dat_0cb9e814[4] = {
#include "ev001a/sonic_action_1/dat_0cb9e814.inc"
};
static NJS_MESHSET dat_0cb9e824[1] = {
#include "ev001a/sonic_action_1/dat_0cb9e824.inc"
};
static NJS_POINT3 dat_0cb9e83c[4] = {
#include "ev001a/sonic_action_1/dat_0cb9e83c.inc"
};
static NJS_VECTOR dat_0cb9e86c[4] = {
#include "ev001a/sonic_action_1/dat_0cb9e86c.inc"
};
static NJS_MODEL dat_0cb9e89c = {
#include "ev001a/sonic_action_1/dat_0cb9e89c.inc"
};
static NJS_OBJECT dat_0cb9e8c4 = {
#include "ev001a/sonic_action_1/dat_0cb9e8c4.inc"
};
static NJS_MATERIAL dat_0cb9e8f8[1] = {
#include "ev001a/sonic_action_1/dat_0cb9e8f8.inc"
};
static Sint16 dat_0cb9e90c[12] = {
#include "ev001a/sonic_action_1/dat_0cb9e90c.inc"
};
static NJS_COLOR dat_0cb9e924[9] = {
#include "ev001a/sonic_action_1/dat_0cb9e924.inc"
};
static NJS_MESHSET dat_0cb9e948[1] = {
#include "ev001a/sonic_action_1/dat_0cb9e948.inc"
};
static NJS_POINT3 dat_0cb9e960[7] = {
#include "ev001a/sonic_action_1/dat_0cb9e960.inc"
};
static NJS_VECTOR dat_0cb9e9b4[7] = {
#include "ev001a/sonic_action_1/dat_0cb9e9b4.inc"
};
static NJS_MODEL dat_0cb9ea08 = {
#include "ev001a/sonic_action_1/dat_0cb9ea08.inc"
};
static NJS_OBJECT dat_0cb9ea30 = {
#include "ev001a/sonic_action_1/dat_0cb9ea30.inc"
};
static NJS_OBJECT dat_0cb9ea64 = {
#include "ev001a/sonic_action_1/dat_0cb9ea64.inc"
};
static NJS_OBJECT dat_0cb9ea98 = {
#include "ev001a/sonic_action_1/dat_0cb9ea98.inc"
};
static NJS_OBJECT dat_0cb9eacc = {
#include "ev001a/sonic_action_1/dat_0cb9eacc.inc"
};
static NJS_OBJECT dat_0cb9eb00 = {
#include "ev001a/sonic_action_1/dat_0cb9eb00.inc"
};
static NJS_OBJECT dat_0cb9eb34 = {
#include "ev001a/sonic_action_1/dat_0cb9eb34.inc"
};
static NJS_OBJECT dat_0cb9eb68 = {
#include "ev001a/sonic_action_1/dat_0cb9eb68.inc"
};
static NJS_MATERIAL dat_0cb9eb9c[1] = {
#include "ev001a/sonic_action_1/dat_0cb9eb9c.inc"
};
static Sint16 dat_0cb9ebb0[6] = {
#include "ev001a/sonic_action_1/dat_0cb9ebb0.inc"
};
static NJS_COLOR dat_0cb9ebbc[4] = {
#include "ev001a/sonic_action_1/dat_0cb9ebbc.inc"
};
static NJS_MESHSET dat_0cb9ebcc[1] = {
#include "ev001a/sonic_action_1/dat_0cb9ebcc.inc"
};
static NJS_POINT3 dat_0cb9ebe4[4] = {
#include "ev001a/sonic_action_1/dat_0cb9ebe4.inc"
};
static NJS_VECTOR dat_0cb9ec14[4] = {
#include "ev001a/sonic_action_1/dat_0cb9ec14.inc"
};
static NJS_MODEL dat_0cb9ec44 = {
#include "ev001a/sonic_action_1/dat_0cb9ec44.inc"
};
static NJS_OBJECT dat_0cb9ec6c = {
#include "ev001a/sonic_action_1/dat_0cb9ec6c.inc"
};
static NJS_MATERIAL dat_0cb9eca0[1] = {
#include "ev001a/sonic_action_1/dat_0cb9eca0.inc"
};
static Sint16 dat_0cb9ecb4[12] = {
#include "ev001a/sonic_action_1/dat_0cb9ecb4.inc"
};
static NJS_COLOR dat_0cb9eccc[9] = {
#include "ev001a/sonic_action_1/dat_0cb9eccc.inc"
};
static NJS_MESHSET dat_0cb9ecf0[1] = {
#include "ev001a/sonic_action_1/dat_0cb9ecf0.inc"
};
static NJS_POINT3 dat_0cb9ed08[7] = {
#include "ev001a/sonic_action_1/dat_0cb9ed08.inc"
};
static NJS_VECTOR dat_0cb9ed5c[7] = {
#include "ev001a/sonic_action_1/dat_0cb9ed5c.inc"
};
static NJS_MODEL dat_0cb9edb0 = {
#include "ev001a/sonic_action_1/dat_0cb9edb0.inc"
};
static NJS_OBJECT dat_0cb9edd8 = {
#include "ev001a/sonic_action_1/dat_0cb9edd8.inc"
};
static NJS_OBJECT dat_0cb9ee0c = {
#include "ev001a/sonic_action_1/dat_0cb9ee0c.inc"
};
static NJS_OBJECT dat_0cb9ee40 = {
#include "ev001a/sonic_action_1/dat_0cb9ee40.inc"
};
static NJS_OBJECT dat_0cb9ee74 = {
#include "ev001a/sonic_action_1/dat_0cb9ee74.inc"
};
static NJS_OBJECT dat_0cb9eea8 = {
#include "ev001a/sonic_action_1/dat_0cb9eea8.inc"
};
static NJS_OBJECT dat_0cb9eedc = {
#include "ev001a/sonic_action_1/dat_0cb9eedc.inc"
};
static NJS_OBJECT dat_0cb9ef10 = {
#include "ev001a/sonic_action_1/dat_0cb9ef10.inc"
};
static NJS_OBJECT dat_0cb9ef44 = {
#include "ev001a/sonic_action_1/dat_0cb9ef44.inc"
};
static NJS_OBJECT dat_0cb9ef78 = {
#include "ev001a/sonic_action_1/dat_0cb9ef78.inc"
};
static NJS_OBJECT dat_0cb9efac = {
#include "ev001a/sonic_action_1/dat_0cb9efac.inc"
};
static NJS_OBJECT dat_0cb9efe0 = {
#include "ev001a/sonic_action_1/dat_0cb9efe0.inc"
};
static NJS_OBJECT dat_0cb9f014 = {
#include "ev001a/sonic_action_1/dat_0cb9f014.inc"
};
static NJS_MATERIAL dat_0cb9f048[1] = {
#include "ev001a/sonic_action_1/dat_0cb9f048.inc"
};
static Sint16 dat_0cb9f05c[18] = {
#include "ev001a/sonic_action_1/dat_0cb9f05c.inc"
};
static NJS_COLOR dat_0cb9f080[15] = {
#include "ev001a/sonic_action_1/dat_0cb9f080.inc"
};
static NJS_MESHSET dat_0cb9f0bc[1] = {
#include "ev001a/sonic_action_1/dat_0cb9f0bc.inc"
};
static NJS_POINT3 dat_0cb9f0d4[7] = {
#include "ev001a/sonic_action_1/dat_0cb9f0d4.inc"
};
static NJS_VECTOR dat_0cb9f128[7] = {
#include "ev001a/sonic_action_1/dat_0cb9f128.inc"
};
static NJS_MODEL dat_0cb9f17c = {
#include "ev001a/sonic_action_1/dat_0cb9f17c.inc"
};
static NJS_OBJECT dat_0cb9f1a4 = {
#include "ev001a/sonic_action_1/dat_0cb9f1a4.inc"
};
static NJS_OBJECT dat_0cb9f1d8 = {
#include "ev001a/sonic_action_1/dat_0cb9f1d8.inc"
};
static NJS_OBJECT dat_0cb9f20c = {
#include "ev001a/sonic_action_1/dat_0cb9f20c.inc"
};
static NJS_OBJECT dat_0cb9f240 = {
#include "ev001a/sonic_action_1/dat_0cb9f240.inc"
};
static NJS_MATERIAL dat_0cb9f274[1] = {
#include "ev001a/sonic_action_1/dat_0cb9f274.inc"
};
static Sint16 dat_0cb9f288[10] = {
#include "ev001a/sonic_action_1/dat_0cb9f288.inc"
};
static NJS_COLOR dat_0cb9f29c[8] = {
#include "ev001a/sonic_action_1/dat_0cb9f29c.inc"
};
static NJS_MESHSET dat_0cb9f2bc[1] = {
#include "ev001a/sonic_action_1/dat_0cb9f2bc.inc"
};
static NJS_POINT3 dat_0cb9f2d4[8] = {
#include "ev001a/sonic_action_1/dat_0cb9f2d4.inc"
};
static NJS_VECTOR dat_0cb9f334[8] = {
#include "ev001a/sonic_action_1/dat_0cb9f334.inc"
};
static NJS_MODEL dat_0cb9f394 = {
#include "ev001a/sonic_action_1/dat_0cb9f394.inc"
};
static NJS_OBJECT dat_0cb9f3bc = {
#include "ev001a/sonic_action_1/dat_0cb9f3bc.inc"
};
static NJS_MATERIAL dat_0cb9f3f0[1] = {
#include "ev001a/sonic_action_1/dat_0cb9f3f0.inc"
};
static Sint16 dat_0cb9f404[10] = {
#include "ev001a/sonic_action_1/dat_0cb9f404.inc"
};
static NJS_COLOR dat_0cb9f418[8] = {
#include "ev001a/sonic_action_1/dat_0cb9f418.inc"
};
static NJS_MESHSET dat_0cb9f438[1] = {
#include "ev001a/sonic_action_1/dat_0cb9f438.inc"
};
static NJS_POINT3 dat_0cb9f450[8] = {
#include "ev001a/sonic_action_1/dat_0cb9f450.inc"
};
static NJS_VECTOR dat_0cb9f4b0[8] = {
#include "ev001a/sonic_action_1/dat_0cb9f4b0.inc"
};
static NJS_MODEL dat_0cb9f510 = {
#include "ev001a/sonic_action_1/dat_0cb9f510.inc"
};
static NJS_OBJECT dat_0cb9f538 = {
#include "ev001a/sonic_action_1/dat_0cb9f538.inc"
};
static NJS_MATERIAL dat_0cb9f56c[1] = {
#include "ev001a/sonic_action_1/dat_0cb9f56c.inc"
};
static Sint16 dat_0cb9f580[16] = {
#include "ev001a/sonic_action_1/dat_0cb9f580.inc"
};
static NJS_COLOR dat_0cb9f5a0[12] = {
#include "ev001a/sonic_action_1/dat_0cb9f5a0.inc"
};
static NJS_MESHSET dat_0cb9f5d0[1] = {
#include "ev001a/sonic_action_1/dat_0cb9f5d0.inc"
};
static NJS_POINT3 dat_0cb9f5e8[12] = {
#include "ev001a/sonic_action_1/dat_0cb9f5e8.inc"
};
static NJS_VECTOR dat_0cb9f678[12] = {
#include "ev001a/sonic_action_1/dat_0cb9f678.inc"
};
static NJS_MODEL dat_0cb9f708 = {
#include "ev001a/sonic_action_1/dat_0cb9f708.inc"
};
static NJS_OBJECT dat_0cb9f730 = {
#include "ev001a/sonic_action_1/dat_0cb9f730.inc"
};
static NJS_MATERIAL dat_0cb9f764[1] = {
#include "ev001a/sonic_action_1/dat_0cb9f764.inc"
};
static Sint16 dat_0cb9f778[16] = {
#include "ev001a/sonic_action_1/dat_0cb9f778.inc"
};
static NJS_COLOR dat_0cb9f798[12] = {
#include "ev001a/sonic_action_1/dat_0cb9f798.inc"
};
static NJS_MESHSET dat_0cb9f7c8[1] = {
#include "ev001a/sonic_action_1/dat_0cb9f7c8.inc"
};
static NJS_POINT3 dat_0cb9f7e0[12] = {
#include "ev001a/sonic_action_1/dat_0cb9f7e0.inc"
};
static NJS_VECTOR dat_0cb9f870[12] = {
#include "ev001a/sonic_action_1/dat_0cb9f870.inc"
};
static NJS_MODEL dat_0cb9f900 = {
#include "ev001a/sonic_action_1/dat_0cb9f900.inc"
};
static NJS_OBJECT dat_0cb9f928 = {
#include "ev001a/sonic_action_1/dat_0cb9f928.inc"
};
static NJS_MATERIAL dat_0cb9f95c[4] = {
#include "ev001a/sonic_action_1/dat_0cb9f95c.inc"
};
static Sint16 dat_0cb9f9ac[239] = {
#include "ev001a/sonic_action_1/dat_0cb9f9ac.inc"
};
static Sint16 dat_0cb9fb8a[22] = {
#include "ev001a/sonic_action_1/dat_0cb9fb8a.inc"
};
static Sint16 dat_0cb9fbb6[21] = {
#include "ev001a/sonic_action_1/dat_0cb9fbb6.inc"
};
static Sint16 dat_0cb9fbe0[22] = {
#include "ev001a/sonic_action_1/dat_0cb9fbe0.inc"
};
static NJS_COLOR dat_0cb9fc0c[209] = {
#include "ev001a/sonic_action_1/dat_0cb9fc0c.inc"
};
static NJS_COLOR dat_0cb9ff50[20] = {
#include "ev001a/sonic_action_1/dat_0cb9ff50.inc"
};
static NJS_COLOR dat_0cb9ffa0[16] = {
#include "ev001a/sonic_action_1/dat_0cb9ffa0.inc"
};
static NJS_COLOR dat_0cb9ffe0[18] = {
#include "ev001a/sonic_action_1/dat_0cb9ffe0.inc"
};
static NJS_MESHSET dat_0cba0028[4] = {
#include "ev001a/sonic_action_1/dat_0cba0028.inc"
};
static NJS_POINT3 dat_0cba0088[127] = {
#include "ev001a/sonic_action_1/dat_0cba0088.inc"
};
static NJS_VECTOR dat_0cba067c[127] = {
#include "ev001a/sonic_action_1/dat_0cba067c.inc"
};
static NJS_MODEL dat_0cba0c70 = {
#include "ev001a/sonic_action_1/dat_0cba0c70.inc"
};
static NJS_OBJECT dat_0cba0c98 = {
#include "ev001a/sonic_action_1/dat_0cba0c98.inc"
};
static NJS_OBJECT dat_0cba0ccc = {
#include "ev001a/sonic_action_1/dat_0cba0ccc.inc"
};
static NJS_OBJECT dat_0cba0d00 = {
#include "ev001a/sonic_action_1/dat_0cba0d00.inc"
};
static NJS_OBJECT dat_0cba0d34 = {
#include "ev001a/sonic_action_1/dat_0cba0d34.inc"
};
static NJS_MATERIAL dat_0cba0d68[1] = {
#include "ev001a/sonic_action_1/dat_0cba0d68.inc"
};
static Sint16 dat_0cba0d7c[8] = {
#include "ev001a/sonic_action_1/dat_0cba0d7c.inc"
};
static NJS_COLOR dat_0cba0d8c[6] = {
#include "ev001a/sonic_action_1/dat_0cba0d8c.inc"
};
static NJS_MESHSET dat_0cba0da4[1] = {
#include "ev001a/sonic_action_1/dat_0cba0da4.inc"
};
static NJS_POINT3 dat_0cba0dbc[6] = {
#include "ev001a/sonic_action_1/dat_0cba0dbc.inc"
};
static NJS_VECTOR dat_0cba0e04[6] = {
#include "ev001a/sonic_action_1/dat_0cba0e04.inc"
};
static NJS_MODEL dat_0cba0e4c = {
#include "ev001a/sonic_action_1/dat_0cba0e4c.inc"
};
static NJS_OBJECT dat_0cba0e74 = {
#include "ev001a/sonic_action_1/dat_0cba0e74.inc"
};
static NJS_MATERIAL dat_0cba0ea8[1] = {
#include "ev001a/sonic_action_1/dat_0cba0ea8.inc"
};
static Sint16 dat_0cba0ebc[22] = {
#include "ev001a/sonic_action_1/dat_0cba0ebc.inc"
};
static NJS_COLOR dat_0cba0ee8[16] = {
#include "ev001a/sonic_action_1/dat_0cba0ee8.inc"
};
static NJS_MESHSET dat_0cba0f28[1] = {
#include "ev001a/sonic_action_1/dat_0cba0f28.inc"
};
static NJS_POINT3 dat_0cba0f40[5] = {
#include "ev001a/sonic_action_1/dat_0cba0f40.inc"
};
static NJS_VECTOR dat_0cba0f7c[5] = {
#include "ev001a/sonic_action_1/dat_0cba0f7c.inc"
};
static NJS_MODEL dat_0cba0fb8 = {
#include "ev001a/sonic_action_1/dat_0cba0fb8.inc"
};
static NJS_OBJECT dat_0cba0fe0 = {
#include "ev001a/sonic_action_1/dat_0cba0fe0.inc"
};
static NJS_MATERIAL dat_0cba1014[1] = {
#include "ev001a/sonic_action_1/dat_0cba1014.inc"
};
static Sint16 dat_0cba1028[30] = {
#include "ev001a/sonic_action_1/dat_0cba1028.inc"
};
static NJS_COLOR dat_0cba1064[28] = {
#include "ev001a/sonic_action_1/dat_0cba1064.inc"
};
static NJS_MESHSET dat_0cba10d4[1] = {
#include "ev001a/sonic_action_1/dat_0cba10d4.inc"
};
static NJS_POINT3 dat_0cba10ec[18] = {
#include "ev001a/sonic_action_1/dat_0cba10ec.inc"
};
static NJS_VECTOR dat_0cba11c4[18] = {
#include "ev001a/sonic_action_1/dat_0cba11c4.inc"
};
static NJS_MODEL dat_0cba129c = {
#include "ev001a/sonic_action_1/dat_0cba129c.inc"
};
static NJS_OBJECT dat_0cba12c4 = {
#include "ev001a/sonic_action_1/dat_0cba12c4.inc"
};
static NJS_MATERIAL dat_0cba12f8[1] = {
#include "ev001a/sonic_action_1/dat_0cba12f8.inc"
};
static Sint16 dat_0cba130c[20] = {
#include "ev001a/sonic_action_1/dat_0cba130c.inc"
};
static NJS_COLOR dat_0cba1334[16] = {
#include "ev001a/sonic_action_1/dat_0cba1334.inc"
};
static NJS_MESHSET dat_0cba1374[1] = {
#include "ev001a/sonic_action_1/dat_0cba1374.inc"
};
static NJS_POINT3 dat_0cba138c[12] = {
#include "ev001a/sonic_action_1/dat_0cba138c.inc"
};
static NJS_VECTOR dat_0cba141c[12] = {
#include "ev001a/sonic_action_1/dat_0cba141c.inc"
};
static NJS_MODEL dat_0cba14ac = {
#include "ev001a/sonic_action_1/dat_0cba14ac.inc"
};
static NJS_OBJECT dat_0cba14d4 = {
#include "ev001a/sonic_action_1/dat_0cba14d4.inc"
};
static NJS_MATERIAL dat_0cba1508[1] = {
#include "ev001a/sonic_action_1/dat_0cba1508.inc"
};
static Sint16 dat_0cba151c[10] = {
#include "ev001a/sonic_action_1/dat_0cba151c.inc"
};
static NJS_COLOR dat_0cba1530[8] = {
#include "ev001a/sonic_action_1/dat_0cba1530.inc"
};
static NJS_MESHSET dat_0cba1550[1] = {
#include "ev001a/sonic_action_1/dat_0cba1550.inc"
};
static NJS_POINT3 dat_0cba1568[5] = {
#include "ev001a/sonic_action_1/dat_0cba1568.inc"
};
static NJS_VECTOR dat_0cba15a4[5] = {
#include "ev001a/sonic_action_1/dat_0cba15a4.inc"
};
static NJS_MODEL dat_0cba15e0 = {
#include "ev001a/sonic_action_1/dat_0cba15e0.inc"
};
static NJS_OBJECT dat_0cba1608 = {
#include "ev001a/sonic_action_1/dat_0cba1608.inc"
};
static NJS_MATERIAL dat_0cba163c[1] = {
#include "ev001a/sonic_action_1/dat_0cba163c.inc"
};
static Sint16 dat_0cba1650[10] = {
#include "ev001a/sonic_action_1/dat_0cba1650.inc"
};
static NJS_COLOR dat_0cba1664[8] = {
#include "ev001a/sonic_action_1/dat_0cba1664.inc"
};
static NJS_MESHSET dat_0cba1684[1] = {
#include "ev001a/sonic_action_1/dat_0cba1684.inc"
};
static NJS_POINT3 dat_0cba169c[5] = {
#include "ev001a/sonic_action_1/dat_0cba169c.inc"
};
static NJS_VECTOR dat_0cba16d8[5] = {
#include "ev001a/sonic_action_1/dat_0cba16d8.inc"
};
static NJS_MODEL dat_0cba1714 = {
#include "ev001a/sonic_action_1/dat_0cba1714.inc"
};
static NJS_OBJECT dat_0cba173c = {
#include "ev001a/sonic_action_1/dat_0cba173c.inc"
};
static NJS_MATERIAL dat_0cba1770[1] = {
#include "ev001a/sonic_action_1/dat_0cba1770.inc"
};
static Sint16 dat_0cba1784[24] = {
#include "ev001a/sonic_action_1/dat_0cba1784.inc"
};
static NJS_COLOR dat_0cba17b4[20] = {
#include "ev001a/sonic_action_1/dat_0cba17b4.inc"
};
static NJS_MESHSET dat_0cba1804[1] = {
#include "ev001a/sonic_action_1/dat_0cba1804.inc"
};
static NJS_POINT3 dat_0cba181c[12] = {
#include "ev001a/sonic_action_1/dat_0cba181c.inc"
};
static NJS_VECTOR dat_0cba18ac[12] = {
#include "ev001a/sonic_action_1/dat_0cba18ac.inc"
};
static NJS_MODEL dat_0cba193c = {
#include "ev001a/sonic_action_1/dat_0cba193c.inc"
};
static NJS_OBJECT dat_0cba1964 = {
#include "ev001a/sonic_action_1/dat_0cba1964.inc"
};
static NJS_MATERIAL dat_0cba1998[1] = {
#include "ev001a/sonic_action_1/dat_0cba1998.inc"
};
static Sint16 dat_0cba19ac[36] = {
#include "ev001a/sonic_action_1/dat_0cba19ac.inc"
};
static NJS_COLOR dat_0cba19f4[30] = {
#include "ev001a/sonic_action_1/dat_0cba19f4.inc"
};
static NJS_MESHSET dat_0cba1a6c[1] = {
#include "ev001a/sonic_action_1/dat_0cba1a6c.inc"
};
static NJS_POINT3 dat_0cba1a84[12] = {
#include "ev001a/sonic_action_1/dat_0cba1a84.inc"
};
static NJS_VECTOR dat_0cba1b14[12] = {
#include "ev001a/sonic_action_1/dat_0cba1b14.inc"
};
static NJS_MODEL dat_0cba1ba4 = {
#include "ev001a/sonic_action_1/dat_0cba1ba4.inc"
};
static NJS_OBJECT dat_0cba1bcc = {
#include "ev001a/sonic_action_1/dat_0cba1bcc.inc"
};
static NJS_OBJECT dat_0cba1c00 = {
#include "ev001a/sonic_action_1/dat_0cba1c00.inc"
};
static NJS_OBJECT dat_0cba1c34 = {
#include "ev001a/sonic_action_1/dat_0cba1c34.inc"
};
static NJS_OBJECT dat_0cba1c68 = {
#include "ev001a/sonic_action_1/dat_0cba1c68.inc"
};
static NJS_MATERIAL dat_0cba1c9c[1] = {
#include "ev001a/sonic_action_1/dat_0cba1c9c.inc"
};
static Sint16 dat_0cba1cb0[48] = {
#include "ev001a/sonic_action_1/dat_0cba1cb0.inc"
};
static NJS_COLOR dat_0cba1d10[38] = {
#include "ev001a/sonic_action_1/dat_0cba1d10.inc"
};
static NJS_MESHSET dat_0cba1da8[1] = {
#include "ev001a/sonic_action_1/dat_0cba1da8.inc"
};
static NJS_POINT3 dat_0cba1dc0[12] = {
#include "ev001a/sonic_action_1/dat_0cba1dc0.inc"
};
static NJS_VECTOR dat_0cba1e50[12] = {
#include "ev001a/sonic_action_1/dat_0cba1e50.inc"
};
static NJS_MODEL dat_0cba1ee0 = {
#include "ev001a/sonic_action_1/dat_0cba1ee0.inc"
};
static NJS_OBJECT dat_0cba1f08 = {
#include "ev001a/sonic_action_1/dat_0cba1f08.inc"
};
static NJS_OBJECT dat_0cba1f3c = {
#include "ev001a/sonic_action_1/dat_0cba1f3c.inc"
};
static NJS_OBJECT dat_0cba1f70 = {
#include "ev001a/sonic_action_1/dat_0cba1f70.inc"
};
static NJS_OBJECT dat_0cba1fa4 = {
#include "ev001a/sonic_action_1/dat_0cba1fa4.inc"
};
static NJS_MATERIAL dat_0cba1fd8[1] = {
#include "ev001a/sonic_action_1/dat_0cba1fd8.inc"
};
static Sint16 dat_0cba1fec[48] = {
#include "ev001a/sonic_action_1/dat_0cba1fec.inc"
};
static NJS_COLOR dat_0cba204c[38] = {
#include "ev001a/sonic_action_1/dat_0cba204c.inc"
};
static NJS_MESHSET dat_0cba20e4[1] = {
#include "ev001a/sonic_action_1/dat_0cba20e4.inc"
};
static NJS_POINT3 dat_0cba20fc[12] = {
#include "ev001a/sonic_action_1/dat_0cba20fc.inc"
};
static NJS_VECTOR dat_0cba218c[12] = {
#include "ev001a/sonic_action_1/dat_0cba218c.inc"
};
static NJS_MODEL dat_0cba221c = {
#include "ev001a/sonic_action_1/dat_0cba221c.inc"
};
static NJS_OBJECT dat_0cba2244 = {
#include "ev001a/sonic_action_1/dat_0cba2244.inc"
};
static NJS_OBJECT dat_0cba2278 = {
#include "ev001a/sonic_action_1/dat_0cba2278.inc"
};
static NJS_OBJECT dat_0cba22ac = {
#include "ev001a/sonic_action_1/dat_0cba22ac.inc"
};
static NJS_OBJECT dat_0cba22e0 = {
#include "ev001a/sonic_action_1/dat_0cba22e0.inc"
};
static NJS_MATERIAL dat_0cba2314[2] = {
#include "ev001a/sonic_action_1/dat_0cba2314.inc"
};
static Sint16 dat_0cba233c[54] = {
#include "ev001a/sonic_action_1/dat_0cba233c.inc"
};
static Sint16 dat_0cba23a8[22] = {
#include "ev001a/sonic_action_1/dat_0cba23a8.inc"
};
static NJS_COLOR dat_0cba23d4[48] = {
#include "ev001a/sonic_action_1/dat_0cba23d4.inc"
};
static NJS_COLOR dat_0cba2494[18] = {
#include "ev001a/sonic_action_1/dat_0cba2494.inc"
};
static NJS_MESHSET dat_0cba24dc[2] = {
#include "ev001a/sonic_action_1/dat_0cba24dc.inc"
};
static NJS_POINT3 dat_0cba250c[26] = {
#include "ev001a/sonic_action_1/dat_0cba250c.inc"
};
static NJS_VECTOR dat_0cba2644[26] = {
#include "ev001a/sonic_action_1/dat_0cba2644.inc"
};
static NJS_MODEL dat_0cba277c = {
#include "ev001a/sonic_action_1/dat_0cba277c.inc"
};
static NJS_OBJECT dat_0cba27a4 = {
#include "ev001a/sonic_action_1/dat_0cba27a4.inc"
};
static NJS_OBJECT dat_0cba27d8 = {
#include "ev001a/sonic_action_1/dat_0cba27d8.inc"
};
static NJS_OBJECT dat_0cba280c = {
#include "ev001a/sonic_action_1/dat_0cba280c.inc"
};
static NJS_OBJECT dat_0cba2840 = {
#include "ev001a/sonic_action_1/dat_0cba2840.inc"
};
static NJS_OBJECT ev001a_bird_object = {
#include "ev001a/sonic_action_1/ev001a_bird_object.inc"
};
static NJS_MKEY_F dat_0cba28a8[2] = {
#include "ev001a/sonic_action_1/dat_0cba28a8.inc"
};
static NJS_MKEY_F dat_0cba28c8[2] = {
#include "ev001a/sonic_action_1/dat_0cba28c8.inc"
};
static NJS_MKEY_A dat_0cba28e8[2] = {
#include "ev001a/sonic_action_1/dat_0cba28e8.inc"
};
static NJS_MKEY_F dat_0cba2908[2] = {
#include "ev001a/sonic_action_1/dat_0cba2908.inc"
};
static NJS_MKEY_F dat_0cba2928[2] = {
#include "ev001a/sonic_action_1/dat_0cba2928.inc"
};
static NJS_MKEY_A dat_0cba2948[2] = {
#include "ev001a/sonic_action_1/dat_0cba2948.inc"
};
static NJS_MKEY_A dat_0cba2968[2] = {
#include "ev001a/sonic_action_1/dat_0cba2968.inc"
};
static NJS_MKEY_F dat_0cba2988[2] = {
#include "ev001a/sonic_action_1/dat_0cba2988.inc"
};
static NJS_MKEY_F dat_0cba29a8[2] = {
#include "ev001a/sonic_action_1/dat_0cba29a8.inc"
};
static NJS_MKEY_A dat_0cba29c8[2] = {
#include "ev001a/sonic_action_1/dat_0cba29c8.inc"
};
static NJS_MKEY_A dat_0cba29e8[2] = {
#include "ev001a/sonic_action_1/dat_0cba29e8.inc"
};
static NJS_MKEY_F dat_0cba2a08[2] = {
#include "ev001a/sonic_action_1/dat_0cba2a08.inc"
};
static NJS_MKEY_F dat_0cba2a28[2] = {
#include "ev001a/sonic_action_1/dat_0cba2a28.inc"
};
static NJS_MKEY_A dat_0cba2a48[2] = {
#include "ev001a/sonic_action_1/dat_0cba2a48.inc"
};
static NJS_MKEY_A dat_0cba2a68[2] = {
#include "ev001a/sonic_action_1/dat_0cba2a68.inc"
};
static NJS_MKEY_F dat_0cba2a88[2] = {
#include "ev001a/sonic_action_1/dat_0cba2a88.inc"
};
static NJS_MKEY_F dat_0cba2aa8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2aa8.inc"
};
static NJS_MKEY_A dat_0cba2ac8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2ac8.inc"
};
static NJS_MKEY_F dat_0cba2ae8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2ae8.inc"
};
static NJS_MKEY_A dat_0cba2b08[2] = {
#include "ev001a/sonic_action_1/dat_0cba2b08.inc"
};
static NJS_MKEY_F dat_0cba2b28[2] = {
#include "ev001a/sonic_action_1/dat_0cba2b28.inc"
};
static NJS_MKEY_A dat_0cba2b48[2] = {
#include "ev001a/sonic_action_1/dat_0cba2b48.inc"
};
static NJS_MKEY_F dat_0cba2b68[2] = {
#include "ev001a/sonic_action_1/dat_0cba2b68.inc"
};
static NJS_MKEY_A dat_0cba2b88[2] = {
#include "ev001a/sonic_action_1/dat_0cba2b88.inc"
};
static NJS_MKEY_A dat_0cba2ba8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2ba8.inc"
};
static NJS_MKEY_F dat_0cba2bc8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2bc8.inc"
};
static NJS_MKEY_F dat_0cba2be8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2be8.inc"
};
static NJS_MKEY_A dat_0cba2c08[2] = {
#include "ev001a/sonic_action_1/dat_0cba2c08.inc"
};
static NJS_MKEY_F dat_0cba2c28[2] = {
#include "ev001a/sonic_action_1/dat_0cba2c28.inc"
};
static NJS_MKEY_A dat_0cba2c48[2] = {
#include "ev001a/sonic_action_1/dat_0cba2c48.inc"
};
static NJS_MKEY_F dat_0cba2c68[2] = {
#include "ev001a/sonic_action_1/dat_0cba2c68.inc"
};
static NJS_MKEY_A dat_0cba2c88[2] = {
#include "ev001a/sonic_action_1/dat_0cba2c88.inc"
};
static NJS_MKEY_F dat_0cba2ca8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2ca8.inc"
};
static NJS_MKEY_A dat_0cba2cc8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2cc8.inc"
};
static NJS_MKEY_F dat_0cba2ce8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2ce8.inc"
};
static NJS_MKEY_A dat_0cba2d08[2] = {
#include "ev001a/sonic_action_1/dat_0cba2d08.inc"
};
static NJS_MKEY_A dat_0cba2d28[2] = {
#include "ev001a/sonic_action_1/dat_0cba2d28.inc"
};
static NJS_MKEY_F dat_0cba2d48[2] = {
#include "ev001a/sonic_action_1/dat_0cba2d48.inc"
};
static NJS_MKEY_F dat_0cba2d68[2] = {
#include "ev001a/sonic_action_1/dat_0cba2d68.inc"
};
static NJS_MKEY_A dat_0cba2d88[2] = {
#include "ev001a/sonic_action_1/dat_0cba2d88.inc"
};
static NJS_MKEY_F dat_0cba2da8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2da8.inc"
};
static NJS_MKEY_A dat_0cba2dc8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2dc8.inc"
};
static NJS_MKEY_F dat_0cba2de8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2de8.inc"
};
static NJS_MKEY_A dat_0cba2e08[2] = {
#include "ev001a/sonic_action_1/dat_0cba2e08.inc"
};
static NJS_MKEY_F dat_0cba2e28[2] = {
#include "ev001a/sonic_action_1/dat_0cba2e28.inc"
};
static NJS_MKEY_A dat_0cba2e48[2] = {
#include "ev001a/sonic_action_1/dat_0cba2e48.inc"
};
static NJS_MKEY_F dat_0cba2e68[2] = {
#include "ev001a/sonic_action_1/dat_0cba2e68.inc"
};
static NJS_MKEY_A dat_0cba2e88[2] = {
#include "ev001a/sonic_action_1/dat_0cba2e88.inc"
};
static NJS_MKEY_F dat_0cba2ea8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2ea8.inc"
};
static NJS_MKEY_A dat_0cba2ec8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2ec8.inc"
};
static NJS_MKEY_F dat_0cba2ee8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2ee8.inc"
};
static NJS_MKEY_A dat_0cba2f08[2] = {
#include "ev001a/sonic_action_1/dat_0cba2f08.inc"
};
static NJS_MKEY_F dat_0cba2f28[2] = {
#include "ev001a/sonic_action_1/dat_0cba2f28.inc"
};
static NJS_MKEY_F dat_0cba2f48[2] = {
#include "ev001a/sonic_action_1/dat_0cba2f48.inc"
};
static NJS_MKEY_F dat_0cba2f68[2] = {
#include "ev001a/sonic_action_1/dat_0cba2f68.inc"
};
static NJS_MKEY_A dat_0cba2f88[2] = {
#include "ev001a/sonic_action_1/dat_0cba2f88.inc"
};
static NJS_MKEY_F dat_0cba2fa8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2fa8.inc"
};
static NJS_MKEY_A dat_0cba2fc8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2fc8.inc"
};
static NJS_MKEY_F dat_0cba2fe8[2] = {
#include "ev001a/sonic_action_1/dat_0cba2fe8.inc"
};
static NJS_MDATA2 dat_0cba3008[57] = {
#include "ev001a/sonic_action_1/dat_0cba3008.inc"
};
static NJS_MOTION ev001a_bird_motion_0 = {
#include "ev001a/sonic_action_1/ev001a_bird_motion_0.inc"
};
NJS_ACTION ev001a_bird_action_0 = {
#include "ev001a/sonic_action_1/ev001a_bird_action_0.inc"
};
static NJS_MKEY_F dat_0cba33ac[2] = {
#include "ev001a/sonic_action_1/dat_0cba33ac.inc"
};
static NJS_MKEY_F dat_0cba33cc[30] = {
#include "ev001a/sonic_action_1/dat_0cba33cc.inc"
};
static NJS_MKEY_F dat_0cba35ac[2] = {
#include "ev001a/sonic_action_1/dat_0cba35ac.inc"
};
static NJS_MKEY_A dat_0cba35cc[2] = {
#include "ev001a/sonic_action_1/dat_0cba35cc.inc"
};
static NJS_MKEY_F dat_0cba35ec[2] = {
#include "ev001a/sonic_action_1/dat_0cba35ec.inc"
};
static NJS_MKEY_F dat_0cba360c[2] = {
#include "ev001a/sonic_action_1/dat_0cba360c.inc"
};
static NJS_MKEY_F dat_0cba362c[2] = {
#include "ev001a/sonic_action_1/dat_0cba362c.inc"
};
static NJS_MKEY_F dat_0cba364c[2] = {
#include "ev001a/sonic_action_1/dat_0cba364c.inc"
};
static NJS_MKEY_F dat_0cba366c[2] = {
#include "ev001a/sonic_action_1/dat_0cba366c.inc"
};
static NJS_MKEY_A dat_0cba368c[2] = {
#include "ev001a/sonic_action_1/dat_0cba368c.inc"
};
static NJS_MKEY_F dat_0cba36ac[2] = {
#include "ev001a/sonic_action_1/dat_0cba36ac.inc"
};
static NJS_MKEY_A dat_0cba36cc[30] = {
#include "ev001a/sonic_action_1/dat_0cba36cc.inc"
};
static NJS_MKEY_F dat_0cba38ac[2] = {
#include "ev001a/sonic_action_1/dat_0cba38ac.inc"
};
static NJS_MKEY_F dat_0cba38cc[2] = {
#include "ev001a/sonic_action_1/dat_0cba38cc.inc"
};
static NJS_MKEY_F dat_0cba38ec[2] = {
#include "ev001a/sonic_action_1/dat_0cba38ec.inc"
};
static NJS_MKEY_F dat_0cba390c[2] = {
#include "ev001a/sonic_action_1/dat_0cba390c.inc"
};
static NJS_MKEY_F dat_0cba392c[2] = {
#include "ev001a/sonic_action_1/dat_0cba392c.inc"
};
static NJS_MKEY_A dat_0cba394c[2] = {
#include "ev001a/sonic_action_1/dat_0cba394c.inc"
};
static NJS_MKEY_F dat_0cba396c[2] = {
#include "ev001a/sonic_action_1/dat_0cba396c.inc"
};
static NJS_MKEY_A dat_0cba398c[30] = {
#include "ev001a/sonic_action_1/dat_0cba398c.inc"
};
static NJS_MKEY_F dat_0cba3b6c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3b6c.inc"
};
static NJS_MKEY_F dat_0cba3b8c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3b8c.inc"
};
static NJS_MKEY_F dat_0cba3bac[2] = {
#include "ev001a/sonic_action_1/dat_0cba3bac.inc"
};
static NJS_MKEY_F dat_0cba3bcc[2] = {
#include "ev001a/sonic_action_1/dat_0cba3bcc.inc"
};
static NJS_MKEY_F dat_0cba3bec[2] = {
#include "ev001a/sonic_action_1/dat_0cba3bec.inc"
};
static NJS_MKEY_A dat_0cba3c0c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3c0c.inc"
};
static NJS_MKEY_F dat_0cba3c2c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3c2c.inc"
};
static NJS_MKEY_A dat_0cba3c4c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3c4c.inc"
};
static NJS_MKEY_F dat_0cba3c6c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3c6c.inc"
};
static NJS_MKEY_F dat_0cba3c8c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3c8c.inc"
};
static NJS_MKEY_F dat_0cba3cac[2] = {
#include "ev001a/sonic_action_1/dat_0cba3cac.inc"
};
static NJS_MKEY_F dat_0cba3ccc[2] = {
#include "ev001a/sonic_action_1/dat_0cba3ccc.inc"
};
static NJS_MKEY_F dat_0cba3cec[2] = {
#include "ev001a/sonic_action_1/dat_0cba3cec.inc"
};
static NJS_MKEY_F dat_0cba3d0c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3d0c.inc"
};
static NJS_MKEY_F dat_0cba3d2c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3d2c.inc"
};
static NJS_MKEY_F dat_0cba3d4c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3d4c.inc"
};
static NJS_MKEY_A dat_0cba3d6c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3d6c.inc"
};
static NJS_MKEY_F dat_0cba3d8c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3d8c.inc"
};
static NJS_MKEY_F dat_0cba3dac[2] = {
#include "ev001a/sonic_action_1/dat_0cba3dac.inc"
};
static NJS_MKEY_A dat_0cba3dcc[2] = {
#include "ev001a/sonic_action_1/dat_0cba3dcc.inc"
};
static NJS_MKEY_F dat_0cba3dec[2] = {
#include "ev001a/sonic_action_1/dat_0cba3dec.inc"
};
static NJS_MKEY_A dat_0cba3e0c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3e0c.inc"
};
static NJS_MKEY_F dat_0cba3e2c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3e2c.inc"
};
static NJS_MKEY_A dat_0cba3e4c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3e4c.inc"
};
static NJS_MKEY_F dat_0cba3e6c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3e6c.inc"
};
static NJS_MKEY_A dat_0cba3e8c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3e8c.inc"
};
static NJS_MKEY_F dat_0cba3eac[2] = {
#include "ev001a/sonic_action_1/dat_0cba3eac.inc"
};
static NJS_MKEY_F dat_0cba3ecc[2] = {
#include "ev001a/sonic_action_1/dat_0cba3ecc.inc"
};
static NJS_MKEY_F dat_0cba3eec[2] = {
#include "ev001a/sonic_action_1/dat_0cba3eec.inc"
};
static NJS_MKEY_F dat_0cba3f0c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3f0c.inc"
};
static NJS_MKEY_F dat_0cba3f2c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3f2c.inc"
};
static NJS_MKEY_A dat_0cba3f4c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3f4c.inc"
};
static NJS_MKEY_F dat_0cba3f6c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3f6c.inc"
};
static NJS_MKEY_F dat_0cba3f8c[2] = {
#include "ev001a/sonic_action_1/dat_0cba3f8c.inc"
};
static NJS_MKEY_A dat_0cba3fac[2] = {
#include "ev001a/sonic_action_1/dat_0cba3fac.inc"
};
static NJS_MKEY_F dat_0cba3fcc[2] = {
#include "ev001a/sonic_action_1/dat_0cba3fcc.inc"
};
static NJS_MKEY_F dat_0cba3fec[2] = {
#include "ev001a/sonic_action_1/dat_0cba3fec.inc"
};
static NJS_MKEY_A dat_0cba400c[2] = {
#include "ev001a/sonic_action_1/dat_0cba400c.inc"
};
static NJS_MKEY_F dat_0cba402c[2] = {
#include "ev001a/sonic_action_1/dat_0cba402c.inc"
};
static NJS_MKEY_F dat_0cba404c[2] = {
#include "ev001a/sonic_action_1/dat_0cba404c.inc"
};
static NJS_MKEY_A dat_0cba406c[2] = {
#include "ev001a/sonic_action_1/dat_0cba406c.inc"
};
static NJS_MKEY_F dat_0cba408c[2] = {
#include "ev001a/sonic_action_1/dat_0cba408c.inc"
};
static NJS_MKEY_F dat_0cba40ac[2] = {
#include "ev001a/sonic_action_1/dat_0cba40ac.inc"
};
static NJS_MKEY_A dat_0cba40cc[2] = {
#include "ev001a/sonic_action_1/dat_0cba40cc.inc"
};
static NJS_MKEY_F dat_0cba40ec[2] = {
#include "ev001a/sonic_action_1/dat_0cba40ec.inc"
};
static NJS_MKEY_A dat_0cba410c[30] = {
#include "ev001a/sonic_action_1/dat_0cba410c.inc"
};
static NJS_MKEY_F dat_0cba42ec[2] = {
#include "ev001a/sonic_action_1/dat_0cba42ec.inc"
};
static NJS_MKEY_F dat_0cba430c[2] = {
#include "ev001a/sonic_action_1/dat_0cba430c.inc"
};
static NJS_MKEY_F dat_0cba432c[2] = {
#include "ev001a/sonic_action_1/dat_0cba432c.inc"
};
static NJS_MKEY_F dat_0cba434c[2] = {
#include "ev001a/sonic_action_1/dat_0cba434c.inc"
};
static NJS_MKEY_F dat_0cba436c[30] = {
#include "ev001a/sonic_action_1/dat_0cba436c.inc"
};
static NJS_MKEY_A dat_0cba454c[2] = {
#include "ev001a/sonic_action_1/dat_0cba454c.inc"
};
static NJS_MKEY_F dat_0cba456c[2] = {
#include "ev001a/sonic_action_1/dat_0cba456c.inc"
};
static NJS_MKEY_F dat_0cba458c[30] = {
#include "ev001a/sonic_action_1/dat_0cba458c.inc"
};
static NJS_MKEY_A dat_0cba476c[2] = {
#include "ev001a/sonic_action_1/dat_0cba476c.inc"
};
static NJS_MKEY_F dat_0cba478c[2] = {
#include "ev001a/sonic_action_1/dat_0cba478c.inc"
};
static NJS_MKEY_F dat_0cba47ac[2] = {
#include "ev001a/sonic_action_1/dat_0cba47ac.inc"
};
static NJS_MKEY_A dat_0cba47cc[2] = {
#include "ev001a/sonic_action_1/dat_0cba47cc.inc"
};
static NJS_MKEY_F dat_0cba47ec[2] = {
#include "ev001a/sonic_action_1/dat_0cba47ec.inc"
};
static NJS_MKEY_F dat_0cba480c[2] = {
#include "ev001a/sonic_action_1/dat_0cba480c.inc"
};
static NJS_MKEY_A dat_0cba482c[2] = {
#include "ev001a/sonic_action_1/dat_0cba482c.inc"
};
static NJS_MKEY_F dat_0cba484c[2] = {
#include "ev001a/sonic_action_1/dat_0cba484c.inc"
};
static NJS_MKEY_F dat_0cba486c[30] = {
#include "ev001a/sonic_action_1/dat_0cba486c.inc"
};
static NJS_MKEY_A dat_0cba4a4c[2] = {
#include "ev001a/sonic_action_1/dat_0cba4a4c.inc"
};
static NJS_MKEY_F dat_0cba4a6c[2] = {
#include "ev001a/sonic_action_1/dat_0cba4a6c.inc"
};
static NJS_MKEY_F dat_0cba4a8c[2] = {
#include "ev001a/sonic_action_1/dat_0cba4a8c.inc"
};
static NJS_MKEY_A dat_0cba4aac[2] = {
#include "ev001a/sonic_action_1/dat_0cba4aac.inc"
};
static NJS_MKEY_F dat_0cba4acc[2] = {
#include "ev001a/sonic_action_1/dat_0cba4acc.inc"
};
static NJS_MKEY_A dat_0cba4aec[30] = {
#include "ev001a/sonic_action_1/dat_0cba4aec.inc"
};
static NJS_MKEY_F dat_0cba4ccc[2] = {
#include "ev001a/sonic_action_1/dat_0cba4ccc.inc"
};
static NJS_MKEY_F dat_0cba4cec[2] = {
#include "ev001a/sonic_action_1/dat_0cba4cec.inc"
};
static NJS_MKEY_F dat_0cba4d0c[2] = {
#include "ev001a/sonic_action_1/dat_0cba4d0c.inc"
};
static NJS_MKEY_A dat_0cba4d2c[2] = {
#include "ev001a/sonic_action_1/dat_0cba4d2c.inc"
};
static NJS_MKEY_F dat_0cba4d4c[2] = {
#include "ev001a/sonic_action_1/dat_0cba4d4c.inc"
};
static NJS_MKEY_A dat_0cba4d6c[30] = {
#include "ev001a/sonic_action_1/dat_0cba4d6c.inc"
};
static NJS_MKEY_F dat_0cba4f4c[2] = {
#include "ev001a/sonic_action_1/dat_0cba4f4c.inc"
};
static NJS_MKEY_F dat_0cba4f6c[2] = {
#include "ev001a/sonic_action_1/dat_0cba4f6c.inc"
};
static NJS_MKEY_F dat_0cba4f8c[2] = {
#include "ev001a/sonic_action_1/dat_0cba4f8c.inc"
};
static NJS_MKEY_F dat_0cba4fac[2] = {
#include "ev001a/sonic_action_1/dat_0cba4fac.inc"
};
static NJS_MKEY_F dat_0cba4fcc[2] = {
#include "ev001a/sonic_action_1/dat_0cba4fcc.inc"
};
static NJS_MKEY_F dat_0cba4fec[2] = {
#include "ev001a/sonic_action_1/dat_0cba4fec.inc"
};
static NJS_MKEY_F dat_0cba500c[2] = {
#include "ev001a/sonic_action_1/dat_0cba500c.inc"
};
static NJS_MKEY_A dat_0cba502c[2] = {
#include "ev001a/sonic_action_1/dat_0cba502c.inc"
};
static NJS_MKEY_F dat_0cba504c[2] = {
#include "ev001a/sonic_action_1/dat_0cba504c.inc"
};
static NJS_MKEY_A dat_0cba506c[30] = {
#include "ev001a/sonic_action_1/dat_0cba506c.inc"
};
static NJS_MKEY_F dat_0cba524c[2] = {
#include "ev001a/sonic_action_1/dat_0cba524c.inc"
};
static NJS_MKEY_F dat_0cba526c[2] = {
#include "ev001a/sonic_action_1/dat_0cba526c.inc"
};
static NJS_MKEY_F dat_0cba528c[2] = {
#include "ev001a/sonic_action_1/dat_0cba528c.inc"
};
static NJS_MKEY_A dat_0cba52ac[2] = {
#include "ev001a/sonic_action_1/dat_0cba52ac.inc"
};
static NJS_MKEY_F dat_0cba52cc[2] = {
#include "ev001a/sonic_action_1/dat_0cba52cc.inc"
};
static NJS_MKEY_A dat_0cba52ec[30] = {
#include "ev001a/sonic_action_1/dat_0cba52ec.inc"
};
static NJS_MKEY_F dat_0cba54cc[2] = {
#include "ev001a/sonic_action_1/dat_0cba54cc.inc"
};
static NJS_MKEY_F dat_0cba54ec[2] = {
#include "ev001a/sonic_action_1/dat_0cba54ec.inc"
};
static NJS_MKEY_F dat_0cba550c[2] = {
#include "ev001a/sonic_action_1/dat_0cba550c.inc"
};
static NJS_MKEY_F dat_0cba552c[2] = {
#include "ev001a/sonic_action_1/dat_0cba552c.inc"
};
static NJS_MKEY_F dat_0cba554c[2] = {
#include "ev001a/sonic_action_1/dat_0cba554c.inc"
};
static NJS_MDATA3 dat_0cba556c[57] = {
#include "ev001a/sonic_action_1/dat_0cba556c.inc"
};
static NJS_MOTION ev001a_bird_motion_1 = {
#include "ev001a/sonic_action_1/ev001a_bird_motion_1.inc"
};
NJS_ACTION ev001a_bird_action_1 = {
#include "ev001a/sonic_action_1/ev001a_bird_action_1.inc"
};
static NJS_MKEY_F dat_0cba5ad8[2] = {
#include "ev001a/sonic_action_1/dat_0cba5ad8.inc"
};
static NJS_MKEY_F dat_0cba5af8[32] = {
#include "ev001a/sonic_action_1/dat_0cba5af8.inc"
};
static NJS_MKEY_A dat_0cba5cf8[32] = {
#include "ev001a/sonic_action_1/dat_0cba5cf8.inc"
};
static NJS_MKEY_F dat_0cba5ef8[2] = {
#include "ev001a/sonic_action_1/dat_0cba5ef8.inc"
};
static NJS_MKEY_A dat_0cba5f18[32] = {
#include "ev001a/sonic_action_1/dat_0cba5f18.inc"
};
static NJS_MKEY_F dat_0cba6118[2] = {
#include "ev001a/sonic_action_1/dat_0cba6118.inc"
};
static NJS_MKEY_F dat_0cba6138[2] = {
#include "ev001a/sonic_action_1/dat_0cba6138.inc"
};
static NJS_MKEY_F dat_0cba6158[2] = {
#include "ev001a/sonic_action_1/dat_0cba6158.inc"
};
static NJS_MKEY_F dat_0cba6178[2] = {
#include "ev001a/sonic_action_1/dat_0cba6178.inc"
};
static NJS_MKEY_A dat_0cba6198[32] = {
#include "ev001a/sonic_action_1/dat_0cba6198.inc"
};
static NJS_MKEY_F dat_0cba6398[2] = {
#include "ev001a/sonic_action_1/dat_0cba6398.inc"
};
static NJS_MKEY_F dat_0cba63b8[2] = {
#include "ev001a/sonic_action_1/dat_0cba63b8.inc"
};
static NJS_MKEY_F dat_0cba63d8[2] = {
#include "ev001a/sonic_action_1/dat_0cba63d8.inc"
};
static NJS_MKEY_F dat_0cba63f8[2] = {
#include "ev001a/sonic_action_1/dat_0cba63f8.inc"
};
static NJS_MKEY_A dat_0cba6418[32] = {
#include "ev001a/sonic_action_1/dat_0cba6418.inc"
};
static NJS_MKEY_F dat_0cba6618[2] = {
#include "ev001a/sonic_action_1/dat_0cba6618.inc"
};
static NJS_MKEY_F dat_0cba6638[2] = {
#include "ev001a/sonic_action_1/dat_0cba6638.inc"
};
static NJS_MKEY_F dat_0cba6658[2] = {
#include "ev001a/sonic_action_1/dat_0cba6658.inc"
};
static NJS_MKEY_F dat_0cba6678[2] = {
#include "ev001a/sonic_action_1/dat_0cba6678.inc"
};
static NJS_MKEY_A dat_0cba6698[32] = {
#include "ev001a/sonic_action_1/dat_0cba6698.inc"
};
static NJS_MKEY_F dat_0cba6898[2] = {
#include "ev001a/sonic_action_1/dat_0cba6898.inc"
};
static NJS_MKEY_F dat_0cba68b8[2] = {
#include "ev001a/sonic_action_1/dat_0cba68b8.inc"
};
static NJS_MKEY_F dat_0cba68d8[2] = {
#include "ev001a/sonic_action_1/dat_0cba68d8.inc"
};
static NJS_MKEY_F dat_0cba68f8[2] = {
#include "ev001a/sonic_action_1/dat_0cba68f8.inc"
};
static NJS_MKEY_F dat_0cba6918[2] = {
#include "ev001a/sonic_action_1/dat_0cba6918.inc"
};
static NJS_MKEY_F dat_0cba6938[2] = {
#include "ev001a/sonic_action_1/dat_0cba6938.inc"
};
static NJS_MKEY_F dat_0cba6958[2] = {
#include "ev001a/sonic_action_1/dat_0cba6958.inc"
};
static NJS_MKEY_F dat_0cba6978[2] = {
#include "ev001a/sonic_action_1/dat_0cba6978.inc"
};
static NJS_MKEY_F dat_0cba6998[2] = {
#include "ev001a/sonic_action_1/dat_0cba6998.inc"
};
static NJS_MKEY_F dat_0cba69b8[2] = {
#include "ev001a/sonic_action_1/dat_0cba69b8.inc"
};
static NJS_MKEY_F dat_0cba69d8[2] = {
#include "ev001a/sonic_action_1/dat_0cba69d8.inc"
};
static NJS_MKEY_A dat_0cba69f8[32] = {
#include "ev001a/sonic_action_1/dat_0cba69f8.inc"
};
static NJS_MKEY_F dat_0cba6bf8[2] = {
#include "ev001a/sonic_action_1/dat_0cba6bf8.inc"
};
static NJS_MKEY_F dat_0cba6c18[2] = {
#include "ev001a/sonic_action_1/dat_0cba6c18.inc"
};
static NJS_MKEY_F dat_0cba6c38[2] = {
#include "ev001a/sonic_action_1/dat_0cba6c38.inc"
};
static NJS_MKEY_F dat_0cba6c58[2] = {
#include "ev001a/sonic_action_1/dat_0cba6c58.inc"
};
static NJS_MKEY_F dat_0cba6c78[2] = {
#include "ev001a/sonic_action_1/dat_0cba6c78.inc"
};
static NJS_MKEY_F dat_0cba6c98[2] = {
#include "ev001a/sonic_action_1/dat_0cba6c98.inc"
};
static NJS_MKEY_F dat_0cba6cb8[2] = {
#include "ev001a/sonic_action_1/dat_0cba6cb8.inc"
};
static NJS_MKEY_F dat_0cba6cd8[2] = {
#include "ev001a/sonic_action_1/dat_0cba6cd8.inc"
};
static NJS_MKEY_A dat_0cba6cf8[32] = {
#include "ev001a/sonic_action_1/dat_0cba6cf8.inc"
};
static NJS_MKEY_F dat_0cba6ef8[2] = {
#include "ev001a/sonic_action_1/dat_0cba6ef8.inc"
};
static NJS_MKEY_F dat_0cba6f18[2] = {
#include "ev001a/sonic_action_1/dat_0cba6f18.inc"
};
static NJS_MKEY_F dat_0cba6f38[32] = {
#include "ev001a/sonic_action_1/dat_0cba6f38.inc"
};
static NJS_MKEY_F dat_0cba7138[32] = {
#include "ev001a/sonic_action_1/dat_0cba7138.inc"
};
static NJS_MKEY_F dat_0cba7338[32] = {
#include "ev001a/sonic_action_1/dat_0cba7338.inc"
};
static NJS_MKEY_F dat_0cba7538[32] = {
#include "ev001a/sonic_action_1/dat_0cba7538.inc"
};
static NJS_MKEY_F dat_0cba7738[32] = {
#include "ev001a/sonic_action_1/dat_0cba7738.inc"
};
static NJS_MKEY_F dat_0cba7938[2] = {
#include "ev001a/sonic_action_1/dat_0cba7938.inc"
};
static NJS_MKEY_F dat_0cba7958[2] = {
#include "ev001a/sonic_action_1/dat_0cba7958.inc"
};
static NJS_MKEY_A dat_0cba7978[32] = {
#include "ev001a/sonic_action_1/dat_0cba7978.inc"
};
static NJS_MKEY_F dat_0cba7b78[2] = {
#include "ev001a/sonic_action_1/dat_0cba7b78.inc"
};
static NJS_MKEY_F dat_0cba7b98[2] = {
#include "ev001a/sonic_action_1/dat_0cba7b98.inc"
};
static NJS_MKEY_F dat_0cba7bb8[2] = {
#include "ev001a/sonic_action_1/dat_0cba7bb8.inc"
};
static NJS_MKEY_A dat_0cba7bd8[32] = {
#include "ev001a/sonic_action_1/dat_0cba7bd8.inc"
};
static NJS_MKEY_F dat_0cba7dd8[2] = {
#include "ev001a/sonic_action_1/dat_0cba7dd8.inc"
};
static NJS_MKEY_F dat_0cba7df8[2] = {
#include "ev001a/sonic_action_1/dat_0cba7df8.inc"
};
static NJS_MKEY_F dat_0cba7e18[2] = {
#include "ev001a/sonic_action_1/dat_0cba7e18.inc"
};
static NJS_MKEY_F dat_0cba7e38[2] = {
#include "ev001a/sonic_action_1/dat_0cba7e38.inc"
};
static NJS_MKEY_F dat_0cba7e58[2] = {
#include "ev001a/sonic_action_1/dat_0cba7e58.inc"
};
static NJS_MKEY_A dat_0cba7e78[32] = {
#include "ev001a/sonic_action_1/dat_0cba7e78.inc"
};
static NJS_MKEY_F dat_0cba8078[2] = {
#include "ev001a/sonic_action_1/dat_0cba8078.inc"
};
static NJS_MKEY_F dat_0cba8098[2] = {
#include "ev001a/sonic_action_1/dat_0cba8098.inc"
};
static NJS_MKEY_F dat_0cba80b8[2] = {
#include "ev001a/sonic_action_1/dat_0cba80b8.inc"
};
static NJS_MKEY_A dat_0cba80d8[32] = {
#include "ev001a/sonic_action_1/dat_0cba80d8.inc"
};
static NJS_MKEY_F dat_0cba82d8[2] = {
#include "ev001a/sonic_action_1/dat_0cba82d8.inc"
};
static NJS_MKEY_F dat_0cba82f8[2] = {
#include "ev001a/sonic_action_1/dat_0cba82f8.inc"
};
static NJS_MKEY_F dat_0cba8318[2] = {
#include "ev001a/sonic_action_1/dat_0cba8318.inc"
};
static NJS_MDATA2 dat_0cba8338[57] = {
#include "ev001a/sonic_action_1/dat_0cba8338.inc"
};
static NJS_MOTION ev001a_bird_motion_2 = {
#include "ev001a/sonic_action_1/ev001a_bird_motion_2.inc"
};
NJS_ACTION ev001a_bird_action_2 = {
#include "ev001a/sonic_action_1/ev001a_bird_action_2.inc"
};
static NJS_MKEY_A dat_0cba86dc[2] = {
#include "ev001a/sonic_action_1/dat_0cba86dc.inc"
};
static NJS_MKEY_A dat_0cba86fc[15] = {
#include "ev001a/sonic_action_1/dat_0cba86fc.inc"
};
static NJS_MKEY_A dat_0cba87ec[15] = {
#include "ev001a/sonic_action_1/dat_0cba87ec.inc"
};
static NJS_MKEY_A dat_0cba88dc[2] = {
#include "ev001a/sonic_action_1/dat_0cba88dc.inc"
};
static NJS_MKEY_A dat_0cba88fc[15] = {
#include "ev001a/sonic_action_1/dat_0cba88fc.inc"
};
static NJS_MKEY_A dat_0cba89ec[15] = {
#include "ev001a/sonic_action_1/dat_0cba89ec.inc"
};
static NJS_MKEY_A dat_0cba8adc[15] = {
#include "ev001a/sonic_action_1/dat_0cba8adc.inc"
};
static NJS_MKEY_A dat_0cba8bcc[2] = {
#include "ev001a/sonic_action_1/dat_0cba8bcc.inc"
};
static NJS_MKEY_A dat_0cba8bec[2] = {
#include "ev001a/sonic_action_1/dat_0cba8bec.inc"
};
static NJS_MKEY_A dat_0cba8c0c[15] = {
#include "ev001a/sonic_action_1/dat_0cba8c0c.inc"
};
static NJS_MKEY_A dat_0cba8cfc[15] = {
#include "ev001a/sonic_action_1/dat_0cba8cfc.inc"
};
static NJS_MKEY_A dat_0cba8dec[15] = {
#include "ev001a/sonic_action_1/dat_0cba8dec.inc"
};
static NJS_MKEY_A dat_0cba8edc[2] = {
#include "ev001a/sonic_action_1/dat_0cba8edc.inc"
};
static NJS_MKEY_A dat_0cba8efc[2] = {
#include "ev001a/sonic_action_1/dat_0cba8efc.inc"
};
static NJS_MKEY_A dat_0cba8f1c[2] = {
#include "ev001a/sonic_action_1/dat_0cba8f1c.inc"
};
static NJS_MKEY_A dat_0cba8f3c[2] = {
#include "ev001a/sonic_action_1/dat_0cba8f3c.inc"
};
static NJS_MKEY_A dat_0cba8f5c[2] = {
#include "ev001a/sonic_action_1/dat_0cba8f5c.inc"
};
static NJS_MKEY_A dat_0cba8f7c[2] = {
#include "ev001a/sonic_action_1/dat_0cba8f7c.inc"
};
static NJS_MKEY_A dat_0cba8f9c[2] = {
#include "ev001a/sonic_action_1/dat_0cba8f9c.inc"
};
static NJS_MKEY_A dat_0cba8fbc[2] = {
#include "ev001a/sonic_action_1/dat_0cba8fbc.inc"
};
static NJS_MKEY_A dat_0cba8fdc[2] = {
#include "ev001a/sonic_action_1/dat_0cba8fdc.inc"
};
static NJS_MKEY_A dat_0cba8ffc[2] = {
#include "ev001a/sonic_action_1/dat_0cba8ffc.inc"
};
static NJS_MKEY_A dat_0cba901c[2] = {
#include "ev001a/sonic_action_1/dat_0cba901c.inc"
};
static NJS_MKEY_A dat_0cba903c[2] = {
#include "ev001a/sonic_action_1/dat_0cba903c.inc"
};
static NJS_MDATA2 dat_0cba905c[24] = {
#include "ev001a/sonic_action_1/dat_0cba905c.inc"
};
static NJS_MOTION dat_0cba91dc = {
#include "ev001a/sonic_action_1/dat_0cba91dc.inc"
};
/* an action on event_eggmobile_object. no word in EV001A or 1ST_READ.BIN
 * points at it */
static NJS_ACTION dat_0cba91e8 = {
#include "ev001a/sonic_action_1/dat_0cba91e8.inc"
};
static NJS_MKEY_F dat_0cba91f0[2] = {
#include "ev001a/sonic_action_1/dat_0cba91f0.inc"
};
static NJS_MKEY_A dat_0cba9210[2] = {
#include "ev001a/sonic_action_1/dat_0cba9210.inc"
};
static NJS_MKEY_F dat_0cba9230[2] = {
#include "ev001a/sonic_action_1/dat_0cba9230.inc"
};
static NJS_MKEY_A dat_0cba9250[60] = {
#include "ev001a/sonic_action_1/dat_0cba9250.inc"
};
static NJS_MKEY_F dat_0cba9610[2] = {
#include "ev001a/sonic_action_1/dat_0cba9610.inc"
};
static NJS_MKEY_A dat_0cba9630[60] = {
#include "ev001a/sonic_action_1/dat_0cba9630.inc"
};
static NJS_MKEY_F dat_0cba99f0[2] = {
#include "ev001a/sonic_action_1/dat_0cba99f0.inc"
};
static NJS_MKEY_A dat_0cba9a10[2] = {
#include "ev001a/sonic_action_1/dat_0cba9a10.inc"
};
static NJS_MKEY_F dat_0cba9a30[2] = {
#include "ev001a/sonic_action_1/dat_0cba9a30.inc"
};
static NJS_MKEY_A dat_0cba9a50[60] = {
#include "ev001a/sonic_action_1/dat_0cba9a50.inc"
};
static NJS_MKEY_F dat_0cba9e10[2] = {
#include "ev001a/sonic_action_1/dat_0cba9e10.inc"
};
static NJS_MKEY_A dat_0cba9e30[60] = {
#include "ev001a/sonic_action_1/dat_0cba9e30.inc"
};
static NJS_MKEY_F dat_0cbaa1f0[2] = {
#include "ev001a/sonic_action_1/dat_0cbaa1f0.inc"
};
static NJS_MKEY_A dat_0cbaa210[2] = {
#include "ev001a/sonic_action_1/dat_0cbaa210.inc"
};
static NJS_MKEY_F dat_0cbaa230[2] = {
#include "ev001a/sonic_action_1/dat_0cbaa230.inc"
};
static NJS_MKEY_A dat_0cbaa250[2] = {
#include "ev001a/sonic_action_1/dat_0cbaa250.inc"
};
static NJS_MKEY_F dat_0cbaa270[2] = {
#include "ev001a/sonic_action_1/dat_0cbaa270.inc"
};
static NJS_MKEY_A dat_0cbaa290[2] = {
#include "ev001a/sonic_action_1/dat_0cbaa290.inc"
};
static NJS_MKEY_F dat_0cbaa2b0[2] = {
#include "ev001a/sonic_action_1/dat_0cbaa2b0.inc"
};
static NJS_MKEY_A dat_0cbaa2d0[60] = {
#include "ev001a/sonic_action_1/dat_0cbaa2d0.inc"
};
static NJS_MKEY_F dat_0cbaa690[2] = {
#include "ev001a/sonic_action_1/dat_0cbaa690.inc"
};
static NJS_MKEY_A dat_0cbaa6b0[60] = {
#include "ev001a/sonic_action_1/dat_0cbaa6b0.inc"
};
static NJS_MKEY_F dat_0cbaaa70[2] = {
#include "ev001a/sonic_action_1/dat_0cbaaa70.inc"
};
static NJS_MKEY_A dat_0cbaaa90[2] = {
#include "ev001a/sonic_action_1/dat_0cbaaa90.inc"
};
static NJS_MKEY_F dat_0cbaaab0[2] = {
#include "ev001a/sonic_action_1/dat_0cbaaab0.inc"
};
static NJS_MKEY_A dat_0cbaaad0[2] = {
#include "ev001a/sonic_action_1/dat_0cbaaad0.inc"
};
static NJS_MKEY_F dat_0cbaaaf0[2] = {
#include "ev001a/sonic_action_1/dat_0cbaaaf0.inc"
};
static NJS_MKEY_A dat_0cbaab10[2] = {
#include "ev001a/sonic_action_1/dat_0cbaab10.inc"
};
static NJS_MKEY_F dat_0cbaab30[2] = {
#include "ev001a/sonic_action_1/dat_0cbaab30.inc"
};
static NJS_MKEY_A dat_0cbaab50[2] = {
#include "ev001a/sonic_action_1/dat_0cbaab50.inc"
};
static NJS_MKEY_F dat_0cbaab70[2] = {
#include "ev001a/sonic_action_1/dat_0cbaab70.inc"
};
static NJS_MKEY_A dat_0cbaab90[2] = {
#include "ev001a/sonic_action_1/dat_0cbaab90.inc"
};
static NJS_MKEY_F dat_0cbaabb0[2] = {
#include "ev001a/sonic_action_1/dat_0cbaabb0.inc"
};
static NJS_MKEY_A dat_0cbaabd0[2] = {
#include "ev001a/sonic_action_1/dat_0cbaabd0.inc"
};
static NJS_MKEY_F dat_0cbaabf0[2] = {
#include "ev001a/sonic_action_1/dat_0cbaabf0.inc"
};
static NJS_MKEY_A dat_0cbaac10[2] = {
#include "ev001a/sonic_action_1/dat_0cbaac10.inc"
};
static NJS_MKEY_F dat_0cbaac30[2] = {
#include "ev001a/sonic_action_1/dat_0cbaac30.inc"
};
static NJS_MKEY_A dat_0cbaac50[2] = {
#include "ev001a/sonic_action_1/dat_0cbaac50.inc"
};
static NJS_MKEY_F dat_0cbaac70[2] = {
#include "ev001a/sonic_action_1/dat_0cbaac70.inc"
};
static NJS_MKEY_A dat_0cbaac90[2] = {
#include "ev001a/sonic_action_1/dat_0cbaac90.inc"
};
static NJS_MKEY_F dat_0cbaacb0[2] = {
#include "ev001a/sonic_action_1/dat_0cbaacb0.inc"
};
static NJS_MKEY_A dat_0cbaacd0[2] = {
#include "ev001a/sonic_action_1/dat_0cbaacd0.inc"
};
static NJS_MKEY_F dat_0cbaacf0[2] = {
#include "ev001a/sonic_action_1/dat_0cbaacf0.inc"
};
static NJS_MKEY_A dat_0cbaad10[2] = {
#include "ev001a/sonic_action_1/dat_0cbaad10.inc"
};
static NJS_MKEY_F dat_0cbaad30[2] = {
#include "ev001a/sonic_action_1/dat_0cbaad30.inc"
};
static NJS_MKEY_A dat_0cbaad50[2] = {
#include "ev001a/sonic_action_1/dat_0cbaad50.inc"
};
static NJS_MKEY_F dat_0cbaad70[2] = {
#include "ev001a/sonic_action_1/dat_0cbaad70.inc"
};
static NJS_MKEY_A dat_0cbaad90[2] = {
#include "ev001a/sonic_action_1/dat_0cbaad90.inc"
};
static NJS_MDATA2 dat_0cbaadb0[24] = {
#include "ev001a/sonic_action_1/dat_0cbaadb0.inc"
};
static NJS_MOTION ev001a_eggmobile_motion_0 = {
#include "ev001a/sonic_action_1/ev001a_eggmobile_motion_0.inc"
};
NJS_ACTION ev001a_eggmobile_action_0 = {
#include "ev001a/sonic_action_1/ev001a_eggmobile_action_0.inc"
};
static NJS_MKEY_F dat_0cbaaf44[2] = {
#include "ev001a/sonic_action_1/dat_0cbaaf44.inc"
};
static NJS_MKEY_A dat_0cbaaf64[2] = {
#include "ev001a/sonic_action_1/dat_0cbaaf64.inc"
};
static NJS_MKEY_F dat_0cbaaf84[2] = {
#include "ev001a/sonic_action_1/dat_0cbaaf84.inc"
};
static NJS_MKEY_A dat_0cbaafa4[30] = {
#include "ev001a/sonic_action_1/dat_0cbaafa4.inc"
};
static NJS_MKEY_F dat_0cbab184[2] = {
#include "ev001a/sonic_action_1/dat_0cbab184.inc"
};
static NJS_MKEY_A dat_0cbab1a4[30] = {
#include "ev001a/sonic_action_1/dat_0cbab1a4.inc"
};
static NJS_MKEY_F dat_0cbab384[2] = {
#include "ev001a/sonic_action_1/dat_0cbab384.inc"
};
static NJS_MKEY_A dat_0cbab3a4[2] = {
#include "ev001a/sonic_action_1/dat_0cbab3a4.inc"
};
static NJS_MKEY_F dat_0cbab3c4[2] = {
#include "ev001a/sonic_action_1/dat_0cbab3c4.inc"
};
static NJS_MKEY_A dat_0cbab3e4[30] = {
#include "ev001a/sonic_action_1/dat_0cbab3e4.inc"
};
static NJS_MKEY_F dat_0cbab5c4[2] = {
#include "ev001a/sonic_action_1/dat_0cbab5c4.inc"
};
static NJS_MKEY_A dat_0cbab5e4[30] = {
#include "ev001a/sonic_action_1/dat_0cbab5e4.inc"
};
static NJS_MKEY_F dat_0cbab7c4[2] = {
#include "ev001a/sonic_action_1/dat_0cbab7c4.inc"
};
static NJS_MKEY_A dat_0cbab7e4[2] = {
#include "ev001a/sonic_action_1/dat_0cbab7e4.inc"
};
static NJS_MKEY_F dat_0cbab804[2] = {
#include "ev001a/sonic_action_1/dat_0cbab804.inc"
};
static NJS_MKEY_A dat_0cbab824[2] = {
#include "ev001a/sonic_action_1/dat_0cbab824.inc"
};
static NJS_MKEY_F dat_0cbab844[2] = {
#include "ev001a/sonic_action_1/dat_0cbab844.inc"
};
static NJS_MKEY_A dat_0cbab864[2] = {
#include "ev001a/sonic_action_1/dat_0cbab864.inc"
};
static NJS_MKEY_F dat_0cbab884[2] = {
#include "ev001a/sonic_action_1/dat_0cbab884.inc"
};
static NJS_MKEY_A dat_0cbab8a4[30] = {
#include "ev001a/sonic_action_1/dat_0cbab8a4.inc"
};
static NJS_MKEY_F dat_0cbaba84[2] = {
#include "ev001a/sonic_action_1/dat_0cbaba84.inc"
};
static NJS_MKEY_A dat_0cbabaa4[30] = {
#include "ev001a/sonic_action_1/dat_0cbabaa4.inc"
};
static NJS_MKEY_F dat_0cbabc84[2] = {
#include "ev001a/sonic_action_1/dat_0cbabc84.inc"
};
static NJS_MKEY_A dat_0cbabca4[2] = {
#include "ev001a/sonic_action_1/dat_0cbabca4.inc"
};
static NJS_MKEY_F dat_0cbabcc4[2] = {
#include "ev001a/sonic_action_1/dat_0cbabcc4.inc"
};
static NJS_MKEY_A dat_0cbabce4[2] = {
#include "ev001a/sonic_action_1/dat_0cbabce4.inc"
};
static NJS_MKEY_F dat_0cbabd04[2] = {
#include "ev001a/sonic_action_1/dat_0cbabd04.inc"
};
static NJS_MKEY_A dat_0cbabd24[2] = {
#include "ev001a/sonic_action_1/dat_0cbabd24.inc"
};
static NJS_MKEY_F dat_0cbabd44[2] = {
#include "ev001a/sonic_action_1/dat_0cbabd44.inc"
};
static NJS_MKEY_A dat_0cbabd64[2] = {
#include "ev001a/sonic_action_1/dat_0cbabd64.inc"
};
static NJS_MKEY_F dat_0cbabd84[2] = {
#include "ev001a/sonic_action_1/dat_0cbabd84.inc"
};
static NJS_MKEY_A dat_0cbabda4[2] = {
#include "ev001a/sonic_action_1/dat_0cbabda4.inc"
};
static NJS_MKEY_F dat_0cbabdc4[2] = {
#include "ev001a/sonic_action_1/dat_0cbabdc4.inc"
};
static NJS_MKEY_A dat_0cbabde4[2] = {
#include "ev001a/sonic_action_1/dat_0cbabde4.inc"
};
static NJS_MKEY_F dat_0cbabe04[2] = {
#include "ev001a/sonic_action_1/dat_0cbabe04.inc"
};
static NJS_MKEY_A dat_0cbabe24[2] = {
#include "ev001a/sonic_action_1/dat_0cbabe24.inc"
};
static NJS_MKEY_F dat_0cbabe44[2] = {
#include "ev001a/sonic_action_1/dat_0cbabe44.inc"
};
static NJS_MKEY_A dat_0cbabe64[2] = {
#include "ev001a/sonic_action_1/dat_0cbabe64.inc"
};
static NJS_MKEY_F dat_0cbabe84[2] = {
#include "ev001a/sonic_action_1/dat_0cbabe84.inc"
};
static NJS_MKEY_A dat_0cbabea4[2] = {
#include "ev001a/sonic_action_1/dat_0cbabea4.inc"
};
static NJS_MKEY_F dat_0cbabec4[2] = {
#include "ev001a/sonic_action_1/dat_0cbabec4.inc"
};
static NJS_MKEY_A dat_0cbabee4[2] = {
#include "ev001a/sonic_action_1/dat_0cbabee4.inc"
};
static NJS_MKEY_F dat_0cbabf04[2] = {
#include "ev001a/sonic_action_1/dat_0cbabf04.inc"
};
static NJS_MKEY_A dat_0cbabf24[2] = {
#include "ev001a/sonic_action_1/dat_0cbabf24.inc"
};
static NJS_MKEY_F dat_0cbabf44[2] = {
#include "ev001a/sonic_action_1/dat_0cbabf44.inc"
};
static NJS_MKEY_A dat_0cbabf64[2] = {
#include "ev001a/sonic_action_1/dat_0cbabf64.inc"
};
static NJS_MKEY_F dat_0cbabf84[2] = {
#include "ev001a/sonic_action_1/dat_0cbabf84.inc"
};
static NJS_MKEY_A dat_0cbabfa4[2] = {
#include "ev001a/sonic_action_1/dat_0cbabfa4.inc"
};
static NJS_MDATA2 dat_0cbabfc4[24] = {
#include "ev001a/sonic_action_1/dat_0cbabfc4.inc"
};
static NJS_MOTION dat_0cbac144 = {
#include "ev001a/sonic_action_1/dat_0cbac144.inc"
};
/* an action on event_eggmobile_object. no word in EV001A or 1ST_READ.BIN
 * points at it */
static NJS_ACTION dat_0cbac150 = {
#include "ev001a/sonic_action_1/dat_0cbac150.inc"
};
static NJS_MKEY_F dat_0cbac158[2] = {
#include "ev001a/sonic_action_1/dat_0cbac158.inc"
};
static NJS_MKEY_A dat_0cbac178[2] = {
#include "ev001a/sonic_action_1/dat_0cbac178.inc"
};
static NJS_MKEY_F dat_0cbac198[2] = {
#include "ev001a/sonic_action_1/dat_0cbac198.inc"
};
static NJS_MKEY_A dat_0cbac1b8[30] = {
#include "ev001a/sonic_action_1/dat_0cbac1b8.inc"
};
static NJS_MKEY_F dat_0cbac398[2] = {
#include "ev001a/sonic_action_1/dat_0cbac398.inc"
};
static NJS_MKEY_A dat_0cbac3b8[30] = {
#include "ev001a/sonic_action_1/dat_0cbac3b8.inc"
};
static NJS_MKEY_F dat_0cbac598[2] = {
#include "ev001a/sonic_action_1/dat_0cbac598.inc"
};
static NJS_MKEY_A dat_0cbac5b8[2] = {
#include "ev001a/sonic_action_1/dat_0cbac5b8.inc"
};
static NJS_MKEY_F dat_0cbac5d8[2] = {
#include "ev001a/sonic_action_1/dat_0cbac5d8.inc"
};
static NJS_MKEY_A dat_0cbac5f8[30] = {
#include "ev001a/sonic_action_1/dat_0cbac5f8.inc"
};
static NJS_MKEY_F dat_0cbac7d8[2] = {
#include "ev001a/sonic_action_1/dat_0cbac7d8.inc"
};
static NJS_MKEY_A dat_0cbac7f8[30] = {
#include "ev001a/sonic_action_1/dat_0cbac7f8.inc"
};
static NJS_MKEY_F dat_0cbac9d8[2] = {
#include "ev001a/sonic_action_1/dat_0cbac9d8.inc"
};
static NJS_MKEY_A dat_0cbac9f8[2] = {
#include "ev001a/sonic_action_1/dat_0cbac9f8.inc"
};
static NJS_MKEY_F dat_0cbaca18[2] = {
#include "ev001a/sonic_action_1/dat_0cbaca18.inc"
};
static NJS_MKEY_A dat_0cbaca38[2] = {
#include "ev001a/sonic_action_1/dat_0cbaca38.inc"
};
static NJS_MKEY_F dat_0cbaca58[2] = {
#include "ev001a/sonic_action_1/dat_0cbaca58.inc"
};
static NJS_MKEY_A dat_0cbaca78[2] = {
#include "ev001a/sonic_action_1/dat_0cbaca78.inc"
};
static NJS_MKEY_F dat_0cbaca98[2] = {
#include "ev001a/sonic_action_1/dat_0cbaca98.inc"
};
static NJS_MKEY_A dat_0cbacab8[30] = {
#include "ev001a/sonic_action_1/dat_0cbacab8.inc"
};
static NJS_MKEY_F dat_0cbacc98[2] = {
#include "ev001a/sonic_action_1/dat_0cbacc98.inc"
};
static NJS_MKEY_A dat_0cbaccb8[30] = {
#include "ev001a/sonic_action_1/dat_0cbaccb8.inc"
};
static NJS_MKEY_F dat_0cbace98[2] = {
#include "ev001a/sonic_action_1/dat_0cbace98.inc"
};
static NJS_MKEY_A dat_0cbaceb8[2] = {
#include "ev001a/sonic_action_1/dat_0cbaceb8.inc"
};
static NJS_MKEY_F dat_0cbaced8[2] = {
#include "ev001a/sonic_action_1/dat_0cbaced8.inc"
};
static NJS_MKEY_A dat_0cbacef8[2] = {
#include "ev001a/sonic_action_1/dat_0cbacef8.inc"
};
static NJS_MKEY_F dat_0cbacf18[2] = {
#include "ev001a/sonic_action_1/dat_0cbacf18.inc"
};
static NJS_MKEY_A dat_0cbacf38[2] = {
#include "ev001a/sonic_action_1/dat_0cbacf38.inc"
};
static NJS_MKEY_F dat_0cbacf58[2] = {
#include "ev001a/sonic_action_1/dat_0cbacf58.inc"
};
static NJS_MKEY_A dat_0cbacf78[2] = {
#include "ev001a/sonic_action_1/dat_0cbacf78.inc"
};
static NJS_MKEY_F dat_0cbacf98[2] = {
#include "ev001a/sonic_action_1/dat_0cbacf98.inc"
};
static NJS_MKEY_A dat_0cbacfb8[2] = {
#include "ev001a/sonic_action_1/dat_0cbacfb8.inc"
};
static NJS_MKEY_F dat_0cbacfd8[2] = {
#include "ev001a/sonic_action_1/dat_0cbacfd8.inc"
};
static NJS_MKEY_A dat_0cbacff8[2] = {
#include "ev001a/sonic_action_1/dat_0cbacff8.inc"
};
static NJS_MKEY_F dat_0cbad018[2] = {
#include "ev001a/sonic_action_1/dat_0cbad018.inc"
};
static NJS_MKEY_A dat_0cbad038[2] = {
#include "ev001a/sonic_action_1/dat_0cbad038.inc"
};
static NJS_MKEY_F dat_0cbad058[2] = {
#include "ev001a/sonic_action_1/dat_0cbad058.inc"
};
static NJS_MKEY_A dat_0cbad078[2] = {
#include "ev001a/sonic_action_1/dat_0cbad078.inc"
};
static NJS_MKEY_F dat_0cbad098[2] = {
#include "ev001a/sonic_action_1/dat_0cbad098.inc"
};
static NJS_MKEY_A dat_0cbad0b8[2] = {
#include "ev001a/sonic_action_1/dat_0cbad0b8.inc"
};
static NJS_MKEY_F dat_0cbad0d8[2] = {
#include "ev001a/sonic_action_1/dat_0cbad0d8.inc"
};
static NJS_MKEY_A dat_0cbad0f8[2] = {
#include "ev001a/sonic_action_1/dat_0cbad0f8.inc"
};
static NJS_MKEY_F dat_0cbad118[2] = {
#include "ev001a/sonic_action_1/dat_0cbad118.inc"
};
static NJS_MKEY_A dat_0cbad138[2] = {
#include "ev001a/sonic_action_1/dat_0cbad138.inc"
};
static NJS_MKEY_F dat_0cbad158[2] = {
#include "ev001a/sonic_action_1/dat_0cbad158.inc"
};
static NJS_MKEY_A dat_0cbad178[2] = {
#include "ev001a/sonic_action_1/dat_0cbad178.inc"
};
static NJS_MKEY_F dat_0cbad198[2] = {
#include "ev001a/sonic_action_1/dat_0cbad198.inc"
};
static NJS_MKEY_A dat_0cbad1b8[2] = {
#include "ev001a/sonic_action_1/dat_0cbad1b8.inc"
};
static NJS_MDATA2 dat_0cbad1d8[24] = {
#include "ev001a/sonic_action_1/dat_0cbad1d8.inc"
};
static NJS_MOTION dat_0cbad358 = {
#include "ev001a/sonic_action_1/dat_0cbad358.inc"
};
/* an action on event_eggmobile_object. no word in EV001A or 1ST_READ.BIN
 * points at it */
static NJS_ACTION dat_0cbad364 = {
#include "ev001a/sonic_action_1/dat_0cbad364.inc"
};
static NJS_MKEY_F dat_0cbad36c[2] = {
#include "ev001a/sonic_action_1/dat_0cbad36c.inc"
};
static NJS_MKEY_F dat_0cbad38c[2] = {
#include "ev001a/sonic_action_1/dat_0cbad38c.inc"
};
static NJS_MKEY_A dat_0cbad3ac[32] = {
#include "ev001a/sonic_action_1/dat_0cbad3ac.inc"
};
static NJS_MKEY_F dat_0cbad5ac[2] = {
#include "ev001a/sonic_action_1/dat_0cbad5ac.inc"
};
static NJS_MKEY_A dat_0cbad5cc[33] = {
#include "ev001a/sonic_action_1/dat_0cbad5cc.inc"
};
static NJS_MKEY_F dat_0cbad7dc[2] = {
#include "ev001a/sonic_action_1/dat_0cbad7dc.inc"
};
static NJS_MKEY_F dat_0cbad7fc[2] = {
#include "ev001a/sonic_action_1/dat_0cbad7fc.inc"
};
static NJS_MKEY_A dat_0cbad81c[33] = {
#include "ev001a/sonic_action_1/dat_0cbad81c.inc"
};
static NJS_MKEY_F dat_0cbada2c[2] = {
#include "ev001a/sonic_action_1/dat_0cbada2c.inc"
};
static NJS_MKEY_A dat_0cbada4c[33] = {
#include "ev001a/sonic_action_1/dat_0cbada4c.inc"
};
static NJS_MKEY_F dat_0cbadc5c[2] = {
#include "ev001a/sonic_action_1/dat_0cbadc5c.inc"
};
static NJS_MKEY_F dat_0cbadc7c[2] = {
#include "ev001a/sonic_action_1/dat_0cbadc7c.inc"
};
static NJS_MKEY_F dat_0cbadc9c[2] = {
#include "ev001a/sonic_action_1/dat_0cbadc9c.inc"
};
static NJS_MKEY_F dat_0cbadcbc[2] = {
#include "ev001a/sonic_action_1/dat_0cbadcbc.inc"
};
static NJS_MKEY_A dat_0cbadcdc[33] = {
#include "ev001a/sonic_action_1/dat_0cbadcdc.inc"
};
static NJS_MKEY_F dat_0cbadeec[2] = {
#include "ev001a/sonic_action_1/dat_0cbadeec.inc"
};
static NJS_MKEY_A dat_0cbadf0c[33] = {
#include "ev001a/sonic_action_1/dat_0cbadf0c.inc"
};
static NJS_MKEY_F dat_0cbae11c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae11c.inc"
};
static NJS_MKEY_A dat_0cbae13c[32] = {
#include "ev001a/sonic_action_1/dat_0cbae13c.inc"
};
static NJS_MKEY_F dat_0cbae33c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae33c.inc"
};
static NJS_MKEY_A dat_0cbae35c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae35c.inc"
};
static NJS_MKEY_F dat_0cbae37c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae37c.inc"
};
static NJS_MKEY_F dat_0cbae39c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae39c.inc"
};
static NJS_MKEY_F dat_0cbae3bc[2] = {
#include "ev001a/sonic_action_1/dat_0cbae3bc.inc"
};
static NJS_MKEY_F dat_0cbae3dc[2] = {
#include "ev001a/sonic_action_1/dat_0cbae3dc.inc"
};
static NJS_MKEY_F dat_0cbae3fc[2] = {
#include "ev001a/sonic_action_1/dat_0cbae3fc.inc"
};
static NJS_MKEY_A dat_0cbae41c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae41c.inc"
};
static NJS_MKEY_F dat_0cbae43c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae43c.inc"
};
static NJS_MKEY_A dat_0cbae45c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae45c.inc"
};
static NJS_MKEY_F dat_0cbae47c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae47c.inc"
};
static NJS_MKEY_A dat_0cbae49c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae49c.inc"
};
static NJS_MKEY_F dat_0cbae4bc[2] = {
#include "ev001a/sonic_action_1/dat_0cbae4bc.inc"
};
static NJS_MKEY_A dat_0cbae4dc[2] = {
#include "ev001a/sonic_action_1/dat_0cbae4dc.inc"
};
static NJS_MKEY_F dat_0cbae4fc[2] = {
#include "ev001a/sonic_action_1/dat_0cbae4fc.inc"
};
static NJS_MKEY_A dat_0cbae51c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae51c.inc"
};
static NJS_MKEY_F dat_0cbae53c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae53c.inc"
};
static NJS_MKEY_A dat_0cbae55c[2] = {
#include "ev001a/sonic_action_1/dat_0cbae55c.inc"
};
static NJS_MDATA2 dat_0cbae57c[24] = {
#include "ev001a/sonic_action_1/dat_0cbae57c.inc"
};
static NJS_MOTION ev001a_eggmobile_motion_1 = {
#include "ev001a/sonic_action_1/ev001a_eggmobile_motion_1.inc"
};
NJS_ACTION ev001a_eggmobile_action_1 = {
#include "ev001a/sonic_action_1/ev001a_eggmobile_action_1.inc"
};
static NJS_MKEY_F dat_0cbae710[31] = {
#include "ev001a/sonic_action_1/dat_0cbae710.inc"
};
static NJS_MKEY_A dat_0cbae900[40] = {
#include "ev001a/sonic_action_1/dat_0cbae900.inc"
};
static NJS_MKEY_A dat_0cbaeb80[40] = {
#include "ev001a/sonic_action_1/dat_0cbaeb80.inc"
};
static NJS_MKEY_A dat_0cbaee00[40] = {
#include "ev001a/sonic_action_1/dat_0cbaee00.inc"
};
static NJS_MKEY_A dat_0cbaf080[40] = {
#include "ev001a/sonic_action_1/dat_0cbaf080.inc"
};
static NJS_MKEY_A dat_0cbaf300[40] = {
#include "ev001a/sonic_action_1/dat_0cbaf300.inc"
};
static NJS_MKEY_A dat_0cbaf580[40] = {
#include "ev001a/sonic_action_1/dat_0cbaf580.inc"
};
static NJS_MKEY_A dat_0cbaf800[40] = {
#include "ev001a/sonic_action_1/dat_0cbaf800.inc"
};
static NJS_MKEY_F dat_0cbafa80[31] = {
#include "ev001a/sonic_action_1/dat_0cbafa80.inc"
};
static NJS_MDATA2 dat_0cbafc70[24] = {
#include "ev001a/sonic_action_1/dat_0cbafc70.inc"
};
static NJS_MOTION ev001a_eggmobile_motion_2 = {
#include "ev001a/sonic_action_1/ev001a_eggmobile_motion_2.inc"
};
NJS_ACTION ev001a_eggmobile_action_2 = {
#include "ev001a/sonic_action_1/ev001a_eggmobile_action_2.inc"
};
static NJS_MKEY_F dat_0cbafe04[16] = {
#include "ev001a/sonic_action_1/dat_0cbafe04.inc"
};
static NJS_MKEY_A dat_0cbaff04[16] = {
#include "ev001a/sonic_action_1/dat_0cbaff04.inc"
};
static NJS_MKEY_A dat_0cbb0004[16] = {
#include "ev001a/sonic_action_1/dat_0cbb0004.inc"
};
static NJS_MKEY_A dat_0cbb0104[16] = {
#include "ev001a/sonic_action_1/dat_0cbb0104.inc"
};
static NJS_MKEY_A dat_0cbb0204[16] = {
#include "ev001a/sonic_action_1/dat_0cbb0204.inc"
};
static NJS_MKEY_A dat_0cbb0304[16] = {
#include "ev001a/sonic_action_1/dat_0cbb0304.inc"
};
static NJS_MKEY_A dat_0cbb0404[16] = {
#include "ev001a/sonic_action_1/dat_0cbb0404.inc"
};
static NJS_MDATA2 dat_0cbb0504[24] = {
#include "ev001a/sonic_action_1/dat_0cbb0504.inc"
};
static NJS_MOTION dat_0cbb0684 = {
#include "ev001a/sonic_action_1/dat_0cbb0684.inc"
};
/* an action on event_eggmobile_object. no word in EV001A or 1ST_READ.BIN
 * points at it */
static NJS_ACTION dat_0cbb0690 = {
#include "ev001a/sonic_action_1/dat_0cbb0690.inc"
};
static NJS_MKEY_F dat_0cbb0698[16] = {
#include "ev001a/sonic_action_1/dat_0cbb0698.inc"
};
static NJS_MKEY_A dat_0cbb0798[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0798.inc"
};
static NJS_MKEY_F dat_0cbb07b8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb07b8.inc"
};
static NJS_MKEY_A dat_0cbb07d8[16] = {
#include "ev001a/sonic_action_1/dat_0cbb07d8.inc"
};
static NJS_MKEY_F dat_0cbb08d8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb08d8.inc"
};
static NJS_MKEY_A dat_0cbb08f8[16] = {
#include "ev001a/sonic_action_1/dat_0cbb08f8.inc"
};
static NJS_MKEY_F dat_0cbb09f8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb09f8.inc"
};
static NJS_MKEY_A dat_0cbb0a18[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0a18.inc"
};
static NJS_MKEY_F dat_0cbb0a38[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0a38.inc"
};
static NJS_MKEY_A dat_0cbb0a58[16] = {
#include "ev001a/sonic_action_1/dat_0cbb0a58.inc"
};
static NJS_MKEY_F dat_0cbb0b58[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0b58.inc"
};
static NJS_MKEY_A dat_0cbb0b78[16] = {
#include "ev001a/sonic_action_1/dat_0cbb0b78.inc"
};
static NJS_MKEY_F dat_0cbb0c78[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0c78.inc"
};
static NJS_MKEY_A dat_0cbb0c98[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0c98.inc"
};
static NJS_MKEY_F dat_0cbb0cb8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0cb8.inc"
};
static NJS_MKEY_A dat_0cbb0cd8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0cd8.inc"
};
static NJS_MKEY_F dat_0cbb0cf8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0cf8.inc"
};
static NJS_MKEY_A dat_0cbb0d18[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0d18.inc"
};
static NJS_MKEY_F dat_0cbb0d38[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0d38.inc"
};
static NJS_MKEY_A dat_0cbb0d58[16] = {
#include "ev001a/sonic_action_1/dat_0cbb0d58.inc"
};
static NJS_MKEY_F dat_0cbb0e58[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0e58.inc"
};
static NJS_MKEY_A dat_0cbb0e78[16] = {
#include "ev001a/sonic_action_1/dat_0cbb0e78.inc"
};
static NJS_MKEY_F dat_0cbb0f78[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0f78.inc"
};
static NJS_MKEY_A dat_0cbb0f98[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0f98.inc"
};
static NJS_MKEY_F dat_0cbb0fb8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0fb8.inc"
};
static NJS_MKEY_A dat_0cbb0fd8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0fd8.inc"
};
static NJS_MKEY_F dat_0cbb0ff8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb0ff8.inc"
};
static NJS_MKEY_A dat_0cbb1018[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1018.inc"
};
static NJS_MKEY_F dat_0cbb1038[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1038.inc"
};
static NJS_MKEY_A dat_0cbb1058[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1058.inc"
};
static NJS_MKEY_F dat_0cbb1078[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1078.inc"
};
static NJS_MKEY_A dat_0cbb1098[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1098.inc"
};
static NJS_MKEY_F dat_0cbb10b8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb10b8.inc"
};
static NJS_MKEY_A dat_0cbb10d8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb10d8.inc"
};
static NJS_MKEY_F dat_0cbb10f8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb10f8.inc"
};
static NJS_MKEY_A dat_0cbb1118[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1118.inc"
};
static NJS_MKEY_F dat_0cbb1138[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1138.inc"
};
static NJS_MKEY_A dat_0cbb1158[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1158.inc"
};
static NJS_MKEY_F dat_0cbb1178[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1178.inc"
};
static NJS_MKEY_A dat_0cbb1198[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1198.inc"
};
static NJS_MKEY_F dat_0cbb11b8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb11b8.inc"
};
static NJS_MKEY_A dat_0cbb11d8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb11d8.inc"
};
static NJS_MKEY_F dat_0cbb11f8[2] = {
#include "ev001a/sonic_action_1/dat_0cbb11f8.inc"
};
static NJS_MKEY_A dat_0cbb1218[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1218.inc"
};
static NJS_MKEY_F dat_0cbb1238[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1238.inc"
};
static NJS_MKEY_A dat_0cbb1258[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1258.inc"
};
static NJS_MKEY_F dat_0cbb1278[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1278.inc"
};
static NJS_MKEY_A dat_0cbb1298[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1298.inc"
};
static NJS_MDATA2 dat_0cbb12b8[24] = {
#include "ev001a/sonic_action_1/dat_0cbb12b8.inc"
};
static NJS_MOTION dat_0cbb1438 = {
#include "ev001a/sonic_action_1/dat_0cbb1438.inc"
};
/* an action on event_eggmobile_object. no word in EV001A or 1ST_READ.BIN
 * points at it */
static NJS_ACTION dat_0cbb1444 = {
#include "ev001a/sonic_action_1/dat_0cbb1444.inc"
};
static NJS_MKEY_F dat_0cbb144c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb144c.inc"
};
static NJS_MKEY_A dat_0cbb146c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb146c.inc"
};
static NJS_MKEY_F dat_0cbb148c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb148c.inc"
};
static NJS_MKEY_A dat_0cbb14ac[32] = {
#include "ev001a/sonic_action_1/dat_0cbb14ac.inc"
};
static NJS_MKEY_F dat_0cbb16ac[2] = {
#include "ev001a/sonic_action_1/dat_0cbb16ac.inc"
};
static NJS_MKEY_A dat_0cbb16cc[32] = {
#include "ev001a/sonic_action_1/dat_0cbb16cc.inc"
};
static NJS_MKEY_F dat_0cbb18cc[2] = {
#include "ev001a/sonic_action_1/dat_0cbb18cc.inc"
};
static NJS_MKEY_A dat_0cbb18ec[2] = {
#include "ev001a/sonic_action_1/dat_0cbb18ec.inc"
};
static NJS_MKEY_F dat_0cbb190c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb190c.inc"
};
static NJS_MKEY_A dat_0cbb192c[28] = {
#include "ev001a/sonic_action_1/dat_0cbb192c.inc"
};
static NJS_MKEY_F dat_0cbb1aec[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1aec.inc"
};
static NJS_MKEY_A dat_0cbb1b0c[32] = {
#include "ev001a/sonic_action_1/dat_0cbb1b0c.inc"
};
static NJS_MKEY_F dat_0cbb1d0c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1d0c.inc"
};
static NJS_MKEY_A dat_0cbb1d2c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1d2c.inc"
};
static NJS_MKEY_F dat_0cbb1d4c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1d4c.inc"
};
static NJS_MKEY_A dat_0cbb1d6c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1d6c.inc"
};
static NJS_MKEY_F dat_0cbb1d8c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1d8c.inc"
};
static NJS_MKEY_A dat_0cbb1dac[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1dac.inc"
};
static NJS_MKEY_F dat_0cbb1dcc[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1dcc.inc"
};
static NJS_MKEY_A dat_0cbb1dec[28] = {
#include "ev001a/sonic_action_1/dat_0cbb1dec.inc"
};
static NJS_MKEY_F dat_0cbb1fac[2] = {
#include "ev001a/sonic_action_1/dat_0cbb1fac.inc"
};
static NJS_MKEY_A dat_0cbb1fcc[32] = {
#include "ev001a/sonic_action_1/dat_0cbb1fcc.inc"
};
static NJS_MKEY_F dat_0cbb21cc[2] = {
#include "ev001a/sonic_action_1/dat_0cbb21cc.inc"
};
static NJS_MKEY_A dat_0cbb21ec[2] = {
#include "ev001a/sonic_action_1/dat_0cbb21ec.inc"
};
static NJS_MKEY_F dat_0cbb220c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb220c.inc"
};
static NJS_MKEY_A dat_0cbb222c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb222c.inc"
};
static NJS_MKEY_F dat_0cbb224c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb224c.inc"
};
static NJS_MKEY_A dat_0cbb226c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb226c.inc"
};
static NJS_MKEY_F dat_0cbb228c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb228c.inc"
};
static NJS_MKEY_A dat_0cbb22ac[2] = {
#include "ev001a/sonic_action_1/dat_0cbb22ac.inc"
};
static NJS_MKEY_F dat_0cbb22cc[2] = {
#include "ev001a/sonic_action_1/dat_0cbb22cc.inc"
};
static NJS_MKEY_A dat_0cbb22ec[2] = {
#include "ev001a/sonic_action_1/dat_0cbb22ec.inc"
};
static NJS_MKEY_F dat_0cbb230c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb230c.inc"
};
static NJS_MKEY_A dat_0cbb232c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb232c.inc"
};
static NJS_MKEY_F dat_0cbb234c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb234c.inc"
};
static NJS_MKEY_A dat_0cbb236c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb236c.inc"
};
static NJS_MKEY_F dat_0cbb238c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb238c.inc"
};
static NJS_MKEY_A dat_0cbb23ac[2] = {
#include "ev001a/sonic_action_1/dat_0cbb23ac.inc"
};
static NJS_MKEY_F dat_0cbb23cc[2] = {
#include "ev001a/sonic_action_1/dat_0cbb23cc.inc"
};
static NJS_MKEY_A dat_0cbb23ec[2] = {
#include "ev001a/sonic_action_1/dat_0cbb23ec.inc"
};
static NJS_MKEY_F dat_0cbb240c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb240c.inc"
};
static NJS_MKEY_A dat_0cbb242c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb242c.inc"
};
static NJS_MKEY_F dat_0cbb244c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb244c.inc"
};
static NJS_MKEY_A dat_0cbb246c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb246c.inc"
};
static NJS_MKEY_F dat_0cbb248c[2] = {
#include "ev001a/sonic_action_1/dat_0cbb248c.inc"
};
static NJS_MKEY_A dat_0cbb24ac[2] = {
#include "ev001a/sonic_action_1/dat_0cbb24ac.inc"
};
static NJS_MKEY_F dat_0cbb24cc[2] = {
#include "ev001a/sonic_action_1/dat_0cbb24cc.inc"
};
static NJS_MKEY_A dat_0cbb24ec[2] = {
#include "ev001a/sonic_action_1/dat_0cbb24ec.inc"
};
static NJS_MDATA2 dat_0cbb250c[24] = {
#include "ev001a/sonic_action_1/dat_0cbb250c.inc"
};
static NJS_MOTION ev001a_eggmobile_motion_3 = {
#include "ev001a/sonic_action_1/ev001a_eggmobile_motion_3.inc"
};
NJS_ACTION ev001a_eggmobile_action_3 = {
#include "ev001a/sonic_action_1/ev001a_eggmobile_action_3.inc"
};
static NJS_MKEY_A dat_0cbb26a0[31] = {
#include "ev001a/sonic_action_1/dat_0cbb26a0.inc"
};
static NJS_MKEY_A dat_0cbb2890[32] = {
#include "ev001a/sonic_action_1/dat_0cbb2890.inc"
};
static NJS_MKEY_A dat_0cbb2a90[32] = {
#include "ev001a/sonic_action_1/dat_0cbb2a90.inc"
};
static NJS_MKEY_A dat_0cbb2c90[32] = {
#include "ev001a/sonic_action_1/dat_0cbb2c90.inc"
};
static NJS_MKEY_A dat_0cbb2e90[32] = {
#include "ev001a/sonic_action_1/dat_0cbb2e90.inc"
};
static NJS_MKEY_A dat_0cbb3090[32] = {
#include "ev001a/sonic_action_1/dat_0cbb3090.inc"
};
static NJS_MKEY_A dat_0cbb3290[2] = {
#include "ev001a/sonic_action_1/dat_0cbb3290.inc"
};
static NJS_MKEY_A dat_0cbb32b0[2] = {
#include "ev001a/sonic_action_1/dat_0cbb32b0.inc"
};
static NJS_MKEY_A dat_0cbb32d0[31] = {
#include "ev001a/sonic_action_1/dat_0cbb32d0.inc"
};
static NJS_MDATA2 dat_0cbb34c0[24] = {
#include "ev001a/sonic_action_1/dat_0cbb34c0.inc"
};
static NJS_MOTION ev001a_eggmobile_motion_4 = {
#include "ev001a/sonic_action_1/ev001a_eggmobile_motion_4.inc"
};
NJS_ACTION ev001a_eggmobile_action_4 = {
#include "ev001a/sonic_action_1/ev001a_eggmobile_action_4.inc"
};
/* the texture name that ev001a_emerald_texlist, in the generated data after
 * this object, points at */
NJS_TEXNAME ev001a_emerald_textures[1] = {
    {TEXT_13, 0, 0},
};
