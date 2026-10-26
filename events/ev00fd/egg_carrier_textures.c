/* the Egg Carrier 2 and the emeralds: the Egg Carrier's texture list, model
 * and action, and the emeralds' texture lists and first action */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00fd/egg_carrier_textures.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 11
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 13
#define TEXT_21_LENGTH 13
#define TEXT_22_LENGTH 13
#define TEXT_23_LENGTH 16
#include "ev00fd/egg_carrier_textures/text.inc"

static NJS_TEXNAME ev00fd_egg_carrier_textures[21] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0},
};
NJS_TEXLIST ev00fd_egg_carrier_texlist = {
#include "ev00fd/egg_carrier_textures/ev00fd_egg_carrier_texlist.inc"
};
static NJS_MATERIAL dat_0cb9e12c[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e12c.inc"
};
static Sint16 dat_0cb9e154[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e154.inc"
};
static Sint16 dat_0cb9e168[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e168.inc"
};
static NJS_COLOR dat_0cb9e17c[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e17c.inc"
};
static NJS_COLOR dat_0cb9e19c[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e19c.inc"
};
static NJS_MESHSET dat_0cb9e1bc[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e1bc.inc"
};
static NJS_POINT3 dat_0cb9e1ec[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e1ec.inc"
};
static NJS_VECTOR dat_0cb9e288[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e288.inc"
};
static NJS_MODEL dat_0cb9e324 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e324.inc"
};
static NJS_OBJECT dat_0cb9e34c = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e34c.inc"
};
static NJS_MATERIAL dat_0cb9e380[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e380.inc"
};
static Sint16 dat_0cb9e3a8[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e3a8.inc"
};
static Sint16 dat_0cb9e3bc[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e3bc.inc"
};
static NJS_COLOR dat_0cb9e3d0[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e3d0.inc"
};
static NJS_COLOR dat_0cb9e3f0[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e3f0.inc"
};
static NJS_MESHSET dat_0cb9e410[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e410.inc"
};
static NJS_POINT3 dat_0cb9e440[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e440.inc"
};
static NJS_VECTOR dat_0cb9e4dc[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e4dc.inc"
};
static NJS_MODEL dat_0cb9e578 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e578.inc"
};
static NJS_OBJECT dat_0cb9e5a0 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e5a0.inc"
};
static NJS_MATERIAL dat_0cb9e5d4[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e5d4.inc"
};
static Sint16 dat_0cb9e5fc[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e5fc.inc"
};
static Sint16 dat_0cb9e610[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e610.inc"
};
static NJS_COLOR dat_0cb9e624[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e624.inc"
};
static NJS_COLOR dat_0cb9e644[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e644.inc"
};
static NJS_MESHSET dat_0cb9e664[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e664.inc"
};
static NJS_POINT3 dat_0cb9e694[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e694.inc"
};
static NJS_VECTOR dat_0cb9e730[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e730.inc"
};
static NJS_MODEL dat_0cb9e7cc = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e7cc.inc"
};
static NJS_OBJECT dat_0cb9e7f4 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e7f4.inc"
};
static NJS_MATERIAL dat_0cb9e828[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e828.inc"
};
static Sint16 dat_0cb9e850[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e850.inc"
};
static Sint16 dat_0cb9e864[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e864.inc"
};
static NJS_COLOR dat_0cb9e878[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e878.inc"
};
static NJS_COLOR dat_0cb9e898[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e898.inc"
};
static NJS_MESHSET dat_0cb9e8b8[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e8b8.inc"
};
static NJS_POINT3 dat_0cb9e8e8[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e8e8.inc"
};
static NJS_VECTOR dat_0cb9e984[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9e984.inc"
};
static NJS_MODEL dat_0cb9ea20 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ea20.inc"
};
static NJS_OBJECT dat_0cb9ea48 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ea48.inc"
};
static NJS_MATERIAL dat_0cb9ea7c[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ea7c.inc"
};
static Sint16 dat_0cb9eaa4[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9eaa4.inc"
};
static Sint16 dat_0cb9eab8[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9eab8.inc"
};
static NJS_COLOR dat_0cb9eacc[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9eacc.inc"
};
static NJS_COLOR dat_0cb9eaec[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9eaec.inc"
};
static NJS_MESHSET dat_0cb9eb0c[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9eb0c.inc"
};
static NJS_POINT3 dat_0cb9eb3c[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9eb3c.inc"
};
static NJS_VECTOR dat_0cb9ebd8[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ebd8.inc"
};
static NJS_MODEL dat_0cb9ec74 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ec74.inc"
};
static NJS_OBJECT dat_0cb9ec9c = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ec9c.inc"
};
static NJS_MATERIAL dat_0cb9ecd0[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ecd0.inc"
};
static Sint16 dat_0cb9ecf8[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ecf8.inc"
};
static Sint16 dat_0cb9ed0c[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ed0c.inc"
};
static NJS_COLOR dat_0cb9ed20[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ed20.inc"
};
static NJS_COLOR dat_0cb9ed40[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ed40.inc"
};
static NJS_MESHSET dat_0cb9ed60[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ed60.inc"
};
static NJS_POINT3 dat_0cb9ed90[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ed90.inc"
};
static NJS_VECTOR dat_0cb9ee2c[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ee2c.inc"
};
static NJS_MODEL dat_0cb9eec8 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9eec8.inc"
};
static NJS_OBJECT dat_0cb9eef0 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9eef0.inc"
};
static NJS_MATERIAL dat_0cb9ef24[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ef24.inc"
};
static Sint16 dat_0cb9ef4c[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ef4c.inc"
};
static Sint16 dat_0cb9ef60[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ef60.inc"
};
static NJS_COLOR dat_0cb9ef74[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ef74.inc"
};
static NJS_COLOR dat_0cb9ef94[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9ef94.inc"
};
static NJS_MESHSET dat_0cb9efb4[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9efb4.inc"
};
static NJS_POINT3 dat_0cb9efe4[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9efe4.inc"
};
static NJS_VECTOR dat_0cb9f080[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f080.inc"
};
static NJS_MODEL dat_0cb9f11c = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f11c.inc"
};
static NJS_OBJECT dat_0cb9f144 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f144.inc"
};
static NJS_MATERIAL dat_0cb9f178[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f178.inc"
};
static Sint16 dat_0cb9f1a0[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f1a0.inc"
};
static Sint16 dat_0cb9f1b4[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f1b4.inc"
};
static NJS_COLOR dat_0cb9f1c8[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f1c8.inc"
};
static NJS_COLOR dat_0cb9f1e8[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f1e8.inc"
};
static NJS_MESHSET dat_0cb9f208[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f208.inc"
};
static NJS_POINT3 dat_0cb9f238[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f238.inc"
};
static NJS_VECTOR dat_0cb9f2d4[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f2d4.inc"
};
static NJS_MODEL dat_0cb9f370 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f370.inc"
};
static NJS_OBJECT dat_0cb9f398 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f398.inc"
};
static NJS_MATERIAL dat_0cb9f3cc[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f3cc.inc"
};
static Sint16 dat_0cb9f3f4[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f3f4.inc"
};
static Sint16 dat_0cb9f408[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f408.inc"
};
static NJS_COLOR dat_0cb9f41c[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f41c.inc"
};
static NJS_COLOR dat_0cb9f43c[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f43c.inc"
};
static NJS_MESHSET dat_0cb9f45c[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f45c.inc"
};
static NJS_POINT3 dat_0cb9f48c[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f48c.inc"
};
static NJS_VECTOR dat_0cb9f528[13] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f528.inc"
};
static NJS_MODEL dat_0cb9f5c4 = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f5c4.inc"
};
static NJS_OBJECT dat_0cb9f5ec = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f5ec.inc"
};
static NJS_MATERIAL dat_0cb9f620[7] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f620.inc"
};
static Sint16 dat_0cb9f6ac[681] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9f6ac.inc"
};
static Sint16 dat_0cb9fbfe[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9fbfe.inc"
};
static Sint16 dat_0cb9fc12[49] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9fc12.inc"
};
static Sint16 dat_0cb9fc74[20] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9fc74.inc"
};
static Sint16 dat_0cb9fc9c[20] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9fc9c.inc"
};
static Sint16 dat_0cb9fcc4[196] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9fcc4.inc"
};
static Sint16 dat_0cb9fe4c[38] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9fe4c.inc"
};
static NJS_COLOR dat_0cb9fe98[606] = {
#include "ev00fd/egg_carrier_textures/dat_0cb9fe98.inc"
};
static NJS_COLOR dat_0cba0810[8] = {
#include "ev00fd/egg_carrier_textures/dat_0cba0810.inc"
};
static NJS_COLOR dat_0cba0830[42] = {
#include "ev00fd/egg_carrier_textures/dat_0cba0830.inc"
};
static NJS_COLOR dat_0cba08d8[16] = {
#include "ev00fd/egg_carrier_textures/dat_0cba08d8.inc"
};
static NJS_COLOR dat_0cba0918[16] = {
#include "ev00fd/egg_carrier_textures/dat_0cba0918.inc"
};
static NJS_COLOR dat_0cba0958[168] = {
#include "ev00fd/egg_carrier_textures/dat_0cba0958.inc"
};
static NJS_COLOR dat_0cba0bf8[32] = {
#include "ev00fd/egg_carrier_textures/dat_0cba0bf8.inc"
};
static NJS_MESHSET dat_0cba0c78[7] = {
#include "ev00fd/egg_carrier_textures/dat_0cba0c78.inc"
};
static NJS_POINT3 dat_0cba0d20[390] = {
#include "ev00fd/egg_carrier_textures/dat_0cba0d20.inc"
};
static NJS_VECTOR dat_0cba1f68[390] = {
#include "ev00fd/egg_carrier_textures/dat_0cba1f68.inc"
};
static NJS_MODEL dat_0cba31b0 = {
#include "ev00fd/egg_carrier_textures/dat_0cba31b0.inc"
};
static NJS_OBJECT dat_0cba31d8 = {
#include "ev00fd/egg_carrier_textures/dat_0cba31d8.inc"
};
static NJS_MATERIAL dat_0cba320c[15] = {
#include "ev00fd/egg_carrier_textures/dat_0cba320c.inc"
};
static Sint16 dat_0cba3338[64] = {
#include "ev00fd/egg_carrier_textures/dat_0cba3338.inc"
};
static Sint16 dat_0cba33b8[237] = {
#include "ev00fd/egg_carrier_textures/dat_0cba33b8.inc"
};
static Sint16 dat_0cba3592[808] = {
#include "ev00fd/egg_carrier_textures/dat_0cba3592.inc"
};
static Sint16 dat_0cba3be2[39] = {
#include "ev00fd/egg_carrier_textures/dat_0cba3be2.inc"
};
static NJS_COLOR dat_0cba3c30[54] = {
#include "ev00fd/egg_carrier_textures/dat_0cba3c30.inc"
};
static NJS_COLOR dat_0cba3d08[208] = {
#include "ev00fd/egg_carrier_textures/dat_0cba3d08.inc"
};
static NJS_COLOR dat_0cba4048[714] = {
#include "ev00fd/egg_carrier_textures/dat_0cba4048.inc"
};
static NJS_COLOR dat_0cba4b70[36] = {
#include "ev00fd/egg_carrier_textures/dat_0cba4b70.inc"
};
static NJS_MESHSET dat_0cba4c00[4] = {
#include "ev00fd/egg_carrier_textures/dat_0cba4c00.inc"
};
static NJS_POINT3 dat_0cba4c60[571] = {
#include "ev00fd/egg_carrier_textures/dat_0cba4c60.inc"
};
static NJS_VECTOR dat_0cba6724[571] = {
#include "ev00fd/egg_carrier_textures/dat_0cba6724.inc"
};
static NJS_MODEL dat_0cba81e8 = {
#include "ev00fd/egg_carrier_textures/dat_0cba81e8.inc"
};
static NJS_OBJECT dat_0cba8210 = {
#include "ev00fd/egg_carrier_textures/dat_0cba8210.inc"
};
static NJS_MATERIAL dat_0cba8244[15] = {
#include "ev00fd/egg_carrier_textures/dat_0cba8244.inc"
};
static Sint16 dat_0cba8370[303] = {
#include "ev00fd/egg_carrier_textures/dat_0cba8370.inc"
};
static Sint16 dat_0cba85ce[14] = {
#include "ev00fd/egg_carrier_textures/dat_0cba85ce.inc"
};
static Sint16 dat_0cba85ea[7] = {
#include "ev00fd/egg_carrier_textures/dat_0cba85ea.inc"
};
static Sint16 dat_0cba85f8[328] = {
#include "ev00fd/egg_carrier_textures/dat_0cba85f8.inc"
};
static Sint16 dat_0cba8888[142] = {
#include "ev00fd/egg_carrier_textures/dat_0cba8888.inc"
};
static Sint16 dat_0cba89a4[448] = {
#include "ev00fd/egg_carrier_textures/dat_0cba89a4.inc"
};
static Sint16 dat_0cba8d24[62] = {
#include "ev00fd/egg_carrier_textures/dat_0cba8d24.inc"
};
static Sint16 dat_0cba8da0[72] = {
#include "ev00fd/egg_carrier_textures/dat_0cba8da0.inc"
};
static Sint16 dat_0cba8e30[96] = {
#include "ev00fd/egg_carrier_textures/dat_0cba8e30.inc"
};
static Sint16 dat_0cba8ef0[48] = {
#include "ev00fd/egg_carrier_textures/dat_0cba8ef0.inc"
};
static Sint16 dat_0cba8f50[22] = {
#include "ev00fd/egg_carrier_textures/dat_0cba8f50.inc"
};
static Sint16 dat_0cba8f7c[11] = {
#include "ev00fd/egg_carrier_textures/dat_0cba8f7c.inc"
};
static Sint16 dat_0cba8f92[63] = {
#include "ev00fd/egg_carrier_textures/dat_0cba8f92.inc"
};
static Sint16 dat_0cba9010[18] = {
#include "ev00fd/egg_carrier_textures/dat_0cba9010.inc"
};
static Sint16 dat_0cba9034[16] = {
#include "ev00fd/egg_carrier_textures/dat_0cba9034.inc"
};
static NJS_COLOR dat_0cba9054[261] = {
#include "ev00fd/egg_carrier_textures/dat_0cba9054.inc"
};
static NJS_COLOR dat_0cba9468[11] = {
#include "ev00fd/egg_carrier_textures/dat_0cba9468.inc"
};
static NJS_COLOR dat_0cba9494[6] = {
#include "ev00fd/egg_carrier_textures/dat_0cba9494.inc"
};
static NJS_COLOR dat_0cba94ac[286] = {
#include "ev00fd/egg_carrier_textures/dat_0cba94ac.inc"
};
static NJS_COLOR dat_0cba9924[132] = {
#include "ev00fd/egg_carrier_textures/dat_0cba9924.inc"
};
static NJS_COLOR dat_0cba9b34[397] = {
#include "ev00fd/egg_carrier_textures/dat_0cba9b34.inc"
};
static NJS_COLOR dat_0cbaa168[52] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaa168.inc"
};
static NJS_COLOR dat_0cbaa238[54] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaa238.inc"
};
static NJS_COLOR dat_0cbaa310[72] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaa310.inc"
};
static NJS_COLOR dat_0cbaa430[44] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaa430.inc"
};
static NJS_COLOR dat_0cbaa4e0[20] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaa4e0.inc"
};
static NJS_COLOR dat_0cbaa530[10] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaa530.inc"
};
static NJS_COLOR dat_0cbaa558[55] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaa558.inc"
};
static NJS_COLOR dat_0cbaa634[16] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaa634.inc"
};
static NJS_COLOR dat_0cbaa674[14] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaa674.inc"
};
static NJS_MESHSET dat_0cbaa6ac[15] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaa6ac.inc"
};
static NJS_POINT3 dat_0cbaa814[597] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaa814.inc"
};
static NJS_VECTOR dat_0cbac410[597] = {
#include "ev00fd/egg_carrier_textures/dat_0cbac410.inc"
};
static NJS_MODEL dat_0cbae00c = {
#include "ev00fd/egg_carrier_textures/dat_0cbae00c.inc"
};
NJS_OBJECT ev00fd_egg_carrier_object = {
#include "ev00fd/egg_carrier_textures/ev00fd_egg_carrier_object.inc"
};
static NJS_MKEY_A dat_0cbae068[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbae068.inc"
};
static NJS_MKEY_F dat_0cbae248[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbae248.inc"
};
static NJS_MKEY_A dat_0cbae428[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbae428.inc"
};
static NJS_MKEY_F dat_0cbae608[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbae608.inc"
};
static NJS_MKEY_A dat_0cbae7e8[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbae7e8.inc"
};
static NJS_MKEY_F dat_0cbae9c8[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbae9c8.inc"
};
static NJS_MKEY_A dat_0cbaeba8[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaeba8.inc"
};
static NJS_MKEY_F dat_0cbaed88[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaed88.inc"
};
static NJS_MKEY_A dat_0cbaef68[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaef68.inc"
};
static NJS_MKEY_F dat_0cbaf148[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaf148.inc"
};
static NJS_MKEY_A dat_0cbaf328[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaf328.inc"
};
static NJS_MKEY_F dat_0cbaf508[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaf508.inc"
};
static NJS_MKEY_A dat_0cbaf6e8[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaf6e8.inc"
};
static NJS_MKEY_F dat_0cbaf8c8[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbaf8c8.inc"
};
static NJS_MKEY_A dat_0cbafaa8[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbafaa8.inc"
};
static NJS_MKEY_F dat_0cbafc88[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbafc88.inc"
};
static NJS_MKEY_A dat_0cbafe68[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbafe68.inc"
};
static NJS_MKEY_F dat_0cbb0048[30] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0048.inc"
};
static NJS_MDATA3 dat_0cbb0228[12] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0228.inc"
};
static NJS_MOTION ev00fd_egg_carrier_motion = {
#include "ev00fd/egg_carrier_textures/ev00fd_egg_carrier_motion.inc"
};
NJS_ACTION ev00fd_egg_carrier_action = {
#include "ev00fd/egg_carrier_textures/ev00fd_egg_carrier_action.inc"
};
static NJS_TEXNAME ev00fd_emerald_textures[1] = {
    {TEXT_22, 0, 0},
};
NJS_TEXLIST ev00fd_emerald_texlist = {
#include "ev00fd/egg_carrier_textures/ev00fd_emerald_texlist.inc"
};
static NJS_MATERIAL dat_0cbb0370[1] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0370.inc"
};
static Sint16 dat_0cbb0384[24] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0384.inc"
};
static Sint16 dat_0cbb03b4[68] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb03b4.inc"
};
static NJS_COLOR dat_0cbb043c[24] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb043c.inc"
};
static NJS_COLOR dat_0cbb049c[68] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb049c.inc"
};
static NJS_MESHSET dat_0cbb05ac[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb05ac.inc"
};
static NJS_POINT3 dat_0cbb05dc[37] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb05dc.inc"
};
static NJS_VECTOR dat_0cbb0798[37] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0798.inc"
};
static NJS_MODEL dat_0cbb0954 = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0954.inc"
};
static NJS_OBJECT dat_0cbb097c = {
#include "ev00fd/egg_carrier_textures/dat_0cbb097c.inc"
};
static NJS_MATERIAL dat_0cbb09b0[1] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb09b0.inc"
};
static Sint16 dat_0cbb09c4[48] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb09c4.inc"
};
static NJS_COLOR dat_0cbb0a24[48] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0a24.inc"
};
static NJS_MESHSET dat_0cbb0ae4[1] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0ae4.inc"
};
static NJS_POINT3 dat_0cbb0afc[24] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0afc.inc"
};
static NJS_VECTOR dat_0cbb0c1c[24] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0c1c.inc"
};
static NJS_MODEL dat_0cbb0d3c = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0d3c.inc"
};
static NJS_OBJECT dat_0cbb0d64 = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0d64.inc"
};
static NJS_MATERIAL dat_0cbb0d98[1] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0d98.inc"
};
static Sint16 dat_0cbb0dac[12] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0dac.inc"
};
static Sint16 dat_0cbb0dc4[16] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0dc4.inc"
};
static NJS_COLOR dat_0cbb0de4[12] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0de4.inc"
};
static NJS_COLOR dat_0cbb0e14[16] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0e14.inc"
};
static NJS_MESHSET dat_0cbb0e54[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0e54.inc"
};
static NJS_POINT3 dat_0cbb0e84[17] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0e84.inc"
};
static NJS_VECTOR dat_0cbb0f50[17] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb0f50.inc"
};
static NJS_MODEL dat_0cbb101c = {
#include "ev00fd/egg_carrier_textures/dat_0cbb101c.inc"
};
static NJS_OBJECT dat_0cbb1044 = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1044.inc"
};
static NJS_MATERIAL dat_0cbb1078[1] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1078.inc"
};
static Sint16 dat_0cbb108c[4] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb108c.inc"
};
static NJS_MESHSET dat_0cbb1094[1] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1094.inc"
};
static NJS_POINT3 dat_0cbb10ac[3] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb10ac.inc"
};
static NJS_VECTOR dat_0cbb10d0[3] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb10d0.inc"
};
static NJS_MODEL dat_0cbb10f4 = {
#include "ev00fd/egg_carrier_textures/dat_0cbb10f4.inc"
};
static NJS_OBJECT dat_0cbb111c = {
#include "ev00fd/egg_carrier_textures/dat_0cbb111c.inc"
};
static NJS_OBJECT dat_0cbb1150 = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1150.inc"
};
static NJS_MKEY_F dat_0cbb1184[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1184.inc"
};
static NJS_MKEY_F dat_0cbb11a4[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb11a4.inc"
};
static NJS_MKEY_A dat_0cbb11c4[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb11c4.inc"
};
static NJS_MKEY_F dat_0cbb11e4[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb11e4.inc"
};
static NJS_MKEY_F dat_0cbb1204[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1204.inc"
};
static NJS_MKEY_A dat_0cbb1224[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1224.inc"
};
static NJS_MKEY_F dat_0cbb1244[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1244.inc"
};
static NJS_MKEY_F dat_0cbb1264[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1264.inc"
};
static NJS_MKEY_F dat_0cbb1284[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1284.inc"
};
static NJS_MDATA3 dat_0cbb12a4[5] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb12a4.inc"
};
static NJS_MOTION ev00fd_emerald_motion_0 = {
#include "ev00fd/egg_carrier_textures/ev00fd_emerald_motion_0.inc"
};
NJS_ACTION ev00fd_emerald_action_0 = {
#include "ev00fd/egg_carrier_textures/ev00fd_emerald_action_0.inc"
};
static NJS_TEXNAME ev00fd_emerald_black_textures[1] = {
    {TEXT_23, 0, 0},
};
NJS_TEXLIST ev00fd_emerald_black_texlist = {
#include "ev00fd/egg_carrier_textures/ev00fd_emerald_black_texlist.inc"
};
static NJS_MATERIAL dat_0cbb1344[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1344.inc"
};
static Sint16 dat_0cbb136c[24] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb136c.inc"
};
static Sint16 dat_0cbb139c[68] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb139c.inc"
};
static NJS_COLOR dat_0cbb1424[24] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1424.inc"
};
static NJS_COLOR dat_0cbb1484[68] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1484.inc"
};
static NJS_MESHSET dat_0cbb1594[2] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb1594.inc"
};
static NJS_POINT3 dat_0cbb15c4[37] = {
#include "ev00fd/egg_carrier_textures/dat_0cbb15c4.inc"
};
