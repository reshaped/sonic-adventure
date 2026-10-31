/* Amy's ev0062_amy_action and its motion, then the bird's VER1_WING texlist,
 * model and action, and its VER2_WING texlist. script.c plays the VER1_WING
 * action on the bird while Amy plays hers. the last item, dat_0cb92004, is the
 * material of the first model in the data after this object. that model is a
 * part of the bird that ev0062_ver2_wing_action moves */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0062/amy_action.h"

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
#include "ev0062/amy_action/text.inc"

extern NJS_MDATA2 dat_0cb8b8d8[];
extern NJS_OBJECT amy_object;

static NJS_MOTION ev0062_amy_motion = {
#include "ev0062/amy_action/ev0062_amy_motion.inc"
};
NJS_ACTION ev0062_amy_action = {
#include "ev0062/amy_action/ev0062_amy_action.inc"
};
static NJS_TEXNAME ev0062_ver1_wing_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0062_ver1_wing_texlist = {
#include "ev0062/amy_action/ev0062_ver1_wing_texlist.inc"
};
static NJS_MATERIAL dat_0cb8be64[1] = {
#include "ev0062/amy_action/dat_0cb8be64.inc"
};
static Sint16 dat_0cb8be78[6] = {
#include "ev0062/amy_action/dat_0cb8be78.inc"
};
static NJS_COLOR dat_0cb8be84[4] = {
#include "ev0062/amy_action/dat_0cb8be84.inc"
};
static NJS_MESHSET dat_0cb8be94[1] = {
#include "ev0062/amy_action/dat_0cb8be94.inc"
};
static NJS_POINT3 dat_0cb8beac[4] = {
#include "ev0062/amy_action/dat_0cb8beac.inc"
};
static NJS_VECTOR dat_0cb8bedc[4] = {
#include "ev0062/amy_action/dat_0cb8bedc.inc"
};
static NJS_MODEL dat_0cb8bf0c = {
#include "ev0062/amy_action/dat_0cb8bf0c.inc"
};
static NJS_OBJECT dat_0cb8bf34 = {
#include "ev0062/amy_action/dat_0cb8bf34.inc"
};
static NJS_MATERIAL dat_0cb8bf68[1] = {
#include "ev0062/amy_action/dat_0cb8bf68.inc"
};
static Sint16 dat_0cb8bf7c[12] = {
#include "ev0062/amy_action/dat_0cb8bf7c.inc"
};
static NJS_COLOR dat_0cb8bf94[9] = {
#include "ev0062/amy_action/dat_0cb8bf94.inc"
};
static NJS_MESHSET dat_0cb8bfb8[1] = {
#include "ev0062/amy_action/dat_0cb8bfb8.inc"
};
static NJS_POINT3 dat_0cb8bfd0[7] = {
#include "ev0062/amy_action/dat_0cb8bfd0.inc"
};
static NJS_VECTOR dat_0cb8c024[7] = {
#include "ev0062/amy_action/dat_0cb8c024.inc"
};
static NJS_MODEL dat_0cb8c078 = {
#include "ev0062/amy_action/dat_0cb8c078.inc"
};
static NJS_OBJECT dat_0cb8c0a0 = {
#include "ev0062/amy_action/dat_0cb8c0a0.inc"
};
static NJS_OBJECT dat_0cb8c0d4 = {
#include "ev0062/amy_action/dat_0cb8c0d4.inc"
};
static NJS_OBJECT dat_0cb8c108 = {
#include "ev0062/amy_action/dat_0cb8c108.inc"
};
static NJS_OBJECT dat_0cb8c13c = {
#include "ev0062/amy_action/dat_0cb8c13c.inc"
};
static NJS_OBJECT dat_0cb8c170 = {
#include "ev0062/amy_action/dat_0cb8c170.inc"
};
static NJS_OBJECT dat_0cb8c1a4 = {
#include "ev0062/amy_action/dat_0cb8c1a4.inc"
};
static NJS_OBJECT dat_0cb8c1d8 = {
#include "ev0062/amy_action/dat_0cb8c1d8.inc"
};
static NJS_MATERIAL dat_0cb8c20c[1] = {
#include "ev0062/amy_action/dat_0cb8c20c.inc"
};
static Sint16 dat_0cb8c220[6] = {
#include "ev0062/amy_action/dat_0cb8c220.inc"
};
static NJS_COLOR dat_0cb8c22c[4] = {
#include "ev0062/amy_action/dat_0cb8c22c.inc"
};
static NJS_MESHSET dat_0cb8c23c[1] = {
#include "ev0062/amy_action/dat_0cb8c23c.inc"
};
static NJS_POINT3 dat_0cb8c254[4] = {
#include "ev0062/amy_action/dat_0cb8c254.inc"
};
static NJS_VECTOR dat_0cb8c284[4] = {
#include "ev0062/amy_action/dat_0cb8c284.inc"
};
static NJS_MODEL dat_0cb8c2b4 = {
#include "ev0062/amy_action/dat_0cb8c2b4.inc"
};
static NJS_OBJECT dat_0cb8c2dc = {
#include "ev0062/amy_action/dat_0cb8c2dc.inc"
};
static NJS_MATERIAL dat_0cb8c310[1] = {
#include "ev0062/amy_action/dat_0cb8c310.inc"
};
static Sint16 dat_0cb8c324[12] = {
#include "ev0062/amy_action/dat_0cb8c324.inc"
};
static NJS_COLOR dat_0cb8c33c[9] = {
#include "ev0062/amy_action/dat_0cb8c33c.inc"
};
static NJS_MESHSET dat_0cb8c360[1] = {
#include "ev0062/amy_action/dat_0cb8c360.inc"
};
static NJS_POINT3 dat_0cb8c378[7] = {
#include "ev0062/amy_action/dat_0cb8c378.inc"
};
static NJS_VECTOR dat_0cb8c3cc[7] = {
#include "ev0062/amy_action/dat_0cb8c3cc.inc"
};
static NJS_MODEL dat_0cb8c420 = {
#include "ev0062/amy_action/dat_0cb8c420.inc"
};
static NJS_OBJECT dat_0cb8c448 = {
#include "ev0062/amy_action/dat_0cb8c448.inc"
};
static NJS_OBJECT dat_0cb8c47c = {
#include "ev0062/amy_action/dat_0cb8c47c.inc"
};
static NJS_OBJECT dat_0cb8c4b0 = {
#include "ev0062/amy_action/dat_0cb8c4b0.inc"
};
static NJS_OBJECT dat_0cb8c4e4 = {
#include "ev0062/amy_action/dat_0cb8c4e4.inc"
};
static NJS_OBJECT dat_0cb8c518 = {
#include "ev0062/amy_action/dat_0cb8c518.inc"
};
static NJS_OBJECT dat_0cb8c54c = {
#include "ev0062/amy_action/dat_0cb8c54c.inc"
};
static NJS_OBJECT dat_0cb8c580 = {
#include "ev0062/amy_action/dat_0cb8c580.inc"
};
static NJS_OBJECT dat_0cb8c5b4 = {
#include "ev0062/amy_action/dat_0cb8c5b4.inc"
};
static NJS_OBJECT dat_0cb8c5e8 = {
#include "ev0062/amy_action/dat_0cb8c5e8.inc"
};
static NJS_OBJECT dat_0cb8c61c = {
#include "ev0062/amy_action/dat_0cb8c61c.inc"
};
static NJS_OBJECT dat_0cb8c650 = {
#include "ev0062/amy_action/dat_0cb8c650.inc"
};
static NJS_OBJECT dat_0cb8c684 = {
#include "ev0062/amy_action/dat_0cb8c684.inc"
};
static NJS_MATERIAL dat_0cb8c6b8[1] = {
#include "ev0062/amy_action/dat_0cb8c6b8.inc"
};
static Sint16 dat_0cb8c6cc[18] = {
#include "ev0062/amy_action/dat_0cb8c6cc.inc"
};
static NJS_COLOR dat_0cb8c6f0[15] = {
#include "ev0062/amy_action/dat_0cb8c6f0.inc"
};
static NJS_MESHSET dat_0cb8c72c[1] = {
#include "ev0062/amy_action/dat_0cb8c72c.inc"
};
static NJS_POINT3 dat_0cb8c744[7] = {
#include "ev0062/amy_action/dat_0cb8c744.inc"
};
static NJS_VECTOR dat_0cb8c798[7] = {
#include "ev0062/amy_action/dat_0cb8c798.inc"
};
static NJS_MODEL dat_0cb8c7ec = {
#include "ev0062/amy_action/dat_0cb8c7ec.inc"
};
static NJS_OBJECT dat_0cb8c814 = {
#include "ev0062/amy_action/dat_0cb8c814.inc"
};
static NJS_OBJECT dat_0cb8c848 = {
#include "ev0062/amy_action/dat_0cb8c848.inc"
};
static NJS_OBJECT dat_0cb8c87c = {
#include "ev0062/amy_action/dat_0cb8c87c.inc"
};
static NJS_OBJECT dat_0cb8c8b0 = {
#include "ev0062/amy_action/dat_0cb8c8b0.inc"
};
static NJS_MATERIAL dat_0cb8c8e4[1] = {
#include "ev0062/amy_action/dat_0cb8c8e4.inc"
};
static Sint16 dat_0cb8c8f8[10] = {
#include "ev0062/amy_action/dat_0cb8c8f8.inc"
};
static NJS_COLOR dat_0cb8c90c[8] = {
#include "ev0062/amy_action/dat_0cb8c90c.inc"
};
static NJS_MESHSET dat_0cb8c92c[1] = {
#include "ev0062/amy_action/dat_0cb8c92c.inc"
};
static NJS_POINT3 dat_0cb8c944[8] = {
#include "ev0062/amy_action/dat_0cb8c944.inc"
};
static NJS_VECTOR dat_0cb8c9a4[8] = {
#include "ev0062/amy_action/dat_0cb8c9a4.inc"
};
static NJS_MODEL dat_0cb8ca04 = {
#include "ev0062/amy_action/dat_0cb8ca04.inc"
};
static NJS_OBJECT dat_0cb8ca2c = {
#include "ev0062/amy_action/dat_0cb8ca2c.inc"
};
static NJS_MATERIAL dat_0cb8ca60[1] = {
#include "ev0062/amy_action/dat_0cb8ca60.inc"
};
static Sint16 dat_0cb8ca74[10] = {
#include "ev0062/amy_action/dat_0cb8ca74.inc"
};
static NJS_COLOR dat_0cb8ca88[8] = {
#include "ev0062/amy_action/dat_0cb8ca88.inc"
};
static NJS_MESHSET dat_0cb8caa8[1] = {
#include "ev0062/amy_action/dat_0cb8caa8.inc"
};
static NJS_POINT3 dat_0cb8cac0[8] = {
#include "ev0062/amy_action/dat_0cb8cac0.inc"
};
static NJS_VECTOR dat_0cb8cb20[8] = {
#include "ev0062/amy_action/dat_0cb8cb20.inc"
};
static NJS_MODEL dat_0cb8cb80 = {
#include "ev0062/amy_action/dat_0cb8cb80.inc"
};
static NJS_OBJECT dat_0cb8cba8 = {
#include "ev0062/amy_action/dat_0cb8cba8.inc"
};
static NJS_MATERIAL dat_0cb8cbdc[1] = {
#include "ev0062/amy_action/dat_0cb8cbdc.inc"
};
static Sint16 dat_0cb8cbf0[16] = {
#include "ev0062/amy_action/dat_0cb8cbf0.inc"
};
static NJS_COLOR dat_0cb8cc10[12] = {
#include "ev0062/amy_action/dat_0cb8cc10.inc"
};
static NJS_MESHSET dat_0cb8cc40[1] = {
#include "ev0062/amy_action/dat_0cb8cc40.inc"
};
static NJS_POINT3 dat_0cb8cc58[12] = {
#include "ev0062/amy_action/dat_0cb8cc58.inc"
};
static NJS_VECTOR dat_0cb8cce8[12] = {
#include "ev0062/amy_action/dat_0cb8cce8.inc"
};
static NJS_MODEL dat_0cb8cd78 = {
#include "ev0062/amy_action/dat_0cb8cd78.inc"
};
static NJS_OBJECT dat_0cb8cda0 = {
#include "ev0062/amy_action/dat_0cb8cda0.inc"
};
static NJS_MATERIAL dat_0cb8cdd4[1] = {
#include "ev0062/amy_action/dat_0cb8cdd4.inc"
};
static Sint16 dat_0cb8cde8[16] = {
#include "ev0062/amy_action/dat_0cb8cde8.inc"
};
static NJS_COLOR dat_0cb8ce08[12] = {
#include "ev0062/amy_action/dat_0cb8ce08.inc"
};
static NJS_MESHSET dat_0cb8ce38[1] = {
#include "ev0062/amy_action/dat_0cb8ce38.inc"
};
static NJS_POINT3 dat_0cb8ce50[12] = {
#include "ev0062/amy_action/dat_0cb8ce50.inc"
};
static NJS_VECTOR dat_0cb8cee0[12] = {
#include "ev0062/amy_action/dat_0cb8cee0.inc"
};
static NJS_MODEL dat_0cb8cf70 = {
#include "ev0062/amy_action/dat_0cb8cf70.inc"
};
static NJS_OBJECT dat_0cb8cf98 = {
#include "ev0062/amy_action/dat_0cb8cf98.inc"
};
static NJS_MATERIAL dat_0cb8cfcc[4] = {
#include "ev0062/amy_action/dat_0cb8cfcc.inc"
};
static Sint16 dat_0cb8d01c[239] = {
#include "ev0062/amy_action/dat_0cb8d01c.inc"
};
static Sint16 dat_0cb8d1fa[22] = {
#include "ev0062/amy_action/dat_0cb8d1fa.inc"
};
static Sint16 dat_0cb8d226[21] = {
#include "ev0062/amy_action/dat_0cb8d226.inc"
};
static Sint16 dat_0cb8d250[22] = {
#include "ev0062/amy_action/dat_0cb8d250.inc"
};
static NJS_COLOR dat_0cb8d27c[209] = {
#include "ev0062/amy_action/dat_0cb8d27c.inc"
};
static NJS_COLOR dat_0cb8d5c0[20] = {
#include "ev0062/amy_action/dat_0cb8d5c0.inc"
};
static NJS_COLOR dat_0cb8d610[16] = {
#include "ev0062/amy_action/dat_0cb8d610.inc"
};
static NJS_COLOR dat_0cb8d650[18] = {
#include "ev0062/amy_action/dat_0cb8d650.inc"
};
static NJS_MESHSET dat_0cb8d698[4] = {
#include "ev0062/amy_action/dat_0cb8d698.inc"
};
static NJS_POINT3 dat_0cb8d6f8[127] = {
#include "ev0062/amy_action/dat_0cb8d6f8.inc"
};
static NJS_VECTOR dat_0cb8dcec[127] = {
#include "ev0062/amy_action/dat_0cb8dcec.inc"
};
static NJS_MODEL dat_0cb8e2e0 = {
#include "ev0062/amy_action/dat_0cb8e2e0.inc"
};
static NJS_OBJECT dat_0cb8e308 = {
#include "ev0062/amy_action/dat_0cb8e308.inc"
};
static NJS_OBJECT dat_0cb8e33c = {
#include "ev0062/amy_action/dat_0cb8e33c.inc"
};
static NJS_OBJECT dat_0cb8e370 = {
#include "ev0062/amy_action/dat_0cb8e370.inc"
};
static NJS_OBJECT dat_0cb8e3a4 = {
#include "ev0062/amy_action/dat_0cb8e3a4.inc"
};
static NJS_MATERIAL dat_0cb8e3d8[1] = {
#include "ev0062/amy_action/dat_0cb8e3d8.inc"
};
static Sint16 dat_0cb8e3ec[8] = {
#include "ev0062/amy_action/dat_0cb8e3ec.inc"
};
static NJS_COLOR dat_0cb8e3fc[6] = {
#include "ev0062/amy_action/dat_0cb8e3fc.inc"
};
static NJS_MESHSET dat_0cb8e414[1] = {
#include "ev0062/amy_action/dat_0cb8e414.inc"
};
static NJS_POINT3 dat_0cb8e42c[6] = {
#include "ev0062/amy_action/dat_0cb8e42c.inc"
};
static NJS_VECTOR dat_0cb8e474[6] = {
#include "ev0062/amy_action/dat_0cb8e474.inc"
};
static NJS_MODEL dat_0cb8e4bc = {
#include "ev0062/amy_action/dat_0cb8e4bc.inc"
};
static NJS_OBJECT dat_0cb8e4e4 = {
#include "ev0062/amy_action/dat_0cb8e4e4.inc"
};
static NJS_MATERIAL dat_0cb8e518[1] = {
#include "ev0062/amy_action/dat_0cb8e518.inc"
};
static Sint16 dat_0cb8e52c[22] = {
#include "ev0062/amy_action/dat_0cb8e52c.inc"
};
static NJS_COLOR dat_0cb8e558[16] = {
#include "ev0062/amy_action/dat_0cb8e558.inc"
};
static NJS_MESHSET dat_0cb8e598[1] = {
#include "ev0062/amy_action/dat_0cb8e598.inc"
};
static NJS_POINT3 dat_0cb8e5b0[5] = {
#include "ev0062/amy_action/dat_0cb8e5b0.inc"
};
static NJS_VECTOR dat_0cb8e5ec[5] = {
#include "ev0062/amy_action/dat_0cb8e5ec.inc"
};
static NJS_MODEL dat_0cb8e628 = {
#include "ev0062/amy_action/dat_0cb8e628.inc"
};
static NJS_OBJECT dat_0cb8e650 = {
#include "ev0062/amy_action/dat_0cb8e650.inc"
};
static NJS_MATERIAL dat_0cb8e684[1] = {
#include "ev0062/amy_action/dat_0cb8e684.inc"
};
static Sint16 dat_0cb8e698[30] = {
#include "ev0062/amy_action/dat_0cb8e698.inc"
};
static NJS_COLOR dat_0cb8e6d4[28] = {
#include "ev0062/amy_action/dat_0cb8e6d4.inc"
};
static NJS_MESHSET dat_0cb8e744[1] = {
#include "ev0062/amy_action/dat_0cb8e744.inc"
};
static NJS_POINT3 dat_0cb8e75c[18] = {
#include "ev0062/amy_action/dat_0cb8e75c.inc"
};
static NJS_VECTOR dat_0cb8e834[18] = {
#include "ev0062/amy_action/dat_0cb8e834.inc"
};
static NJS_MODEL dat_0cb8e90c = {
#include "ev0062/amy_action/dat_0cb8e90c.inc"
};
static NJS_OBJECT dat_0cb8e934 = {
#include "ev0062/amy_action/dat_0cb8e934.inc"
};
static NJS_MATERIAL dat_0cb8e968[1] = {
#include "ev0062/amy_action/dat_0cb8e968.inc"
};
static Sint16 dat_0cb8e97c[20] = {
#include "ev0062/amy_action/dat_0cb8e97c.inc"
};
static NJS_COLOR dat_0cb8e9a4[16] = {
#include "ev0062/amy_action/dat_0cb8e9a4.inc"
};
static NJS_MESHSET dat_0cb8e9e4[1] = {
#include "ev0062/amy_action/dat_0cb8e9e4.inc"
};
static NJS_POINT3 dat_0cb8e9fc[12] = {
#include "ev0062/amy_action/dat_0cb8e9fc.inc"
};
static NJS_VECTOR dat_0cb8ea8c[12] = {
#include "ev0062/amy_action/dat_0cb8ea8c.inc"
};
static NJS_MODEL dat_0cb8eb1c = {
#include "ev0062/amy_action/dat_0cb8eb1c.inc"
};
static NJS_OBJECT dat_0cb8eb44 = {
#include "ev0062/amy_action/dat_0cb8eb44.inc"
};
static NJS_MATERIAL dat_0cb8eb78[1] = {
#include "ev0062/amy_action/dat_0cb8eb78.inc"
};
static Sint16 dat_0cb8eb8c[10] = {
#include "ev0062/amy_action/dat_0cb8eb8c.inc"
};
static NJS_COLOR dat_0cb8eba0[8] = {
#include "ev0062/amy_action/dat_0cb8eba0.inc"
};
static NJS_MESHSET dat_0cb8ebc0[1] = {
#include "ev0062/amy_action/dat_0cb8ebc0.inc"
};
static NJS_POINT3 dat_0cb8ebd8[5] = {
#include "ev0062/amy_action/dat_0cb8ebd8.inc"
};
static NJS_VECTOR dat_0cb8ec14[5] = {
#include "ev0062/amy_action/dat_0cb8ec14.inc"
};
static NJS_MODEL dat_0cb8ec50 = {
#include "ev0062/amy_action/dat_0cb8ec50.inc"
};
static NJS_OBJECT dat_0cb8ec78 = {
#include "ev0062/amy_action/dat_0cb8ec78.inc"
};
static NJS_MATERIAL dat_0cb8ecac[1] = {
#include "ev0062/amy_action/dat_0cb8ecac.inc"
};
static Sint16 dat_0cb8ecc0[10] = {
#include "ev0062/amy_action/dat_0cb8ecc0.inc"
};
static NJS_COLOR dat_0cb8ecd4[8] = {
#include "ev0062/amy_action/dat_0cb8ecd4.inc"
};
static NJS_MESHSET dat_0cb8ecf4[1] = {
#include "ev0062/amy_action/dat_0cb8ecf4.inc"
};
static NJS_POINT3 dat_0cb8ed0c[5] = {
#include "ev0062/amy_action/dat_0cb8ed0c.inc"
};
static NJS_VECTOR dat_0cb8ed48[5] = {
#include "ev0062/amy_action/dat_0cb8ed48.inc"
};
static NJS_MODEL dat_0cb8ed84 = {
#include "ev0062/amy_action/dat_0cb8ed84.inc"
};
static NJS_OBJECT dat_0cb8edac = {
#include "ev0062/amy_action/dat_0cb8edac.inc"
};
static NJS_MATERIAL dat_0cb8ede0[1] = {
#include "ev0062/amy_action/dat_0cb8ede0.inc"
};
static Sint16 dat_0cb8edf4[24] = {
#include "ev0062/amy_action/dat_0cb8edf4.inc"
};
static NJS_COLOR dat_0cb8ee24[20] = {
#include "ev0062/amy_action/dat_0cb8ee24.inc"
};
static NJS_MESHSET dat_0cb8ee74[1] = {
#include "ev0062/amy_action/dat_0cb8ee74.inc"
};
static NJS_POINT3 dat_0cb8ee8c[12] = {
#include "ev0062/amy_action/dat_0cb8ee8c.inc"
};
static NJS_VECTOR dat_0cb8ef1c[12] = {
#include "ev0062/amy_action/dat_0cb8ef1c.inc"
};
static NJS_MODEL dat_0cb8efac = {
#include "ev0062/amy_action/dat_0cb8efac.inc"
};
static NJS_OBJECT dat_0cb8efd4 = {
#include "ev0062/amy_action/dat_0cb8efd4.inc"
};
static NJS_MATERIAL dat_0cb8f008[1] = {
#include "ev0062/amy_action/dat_0cb8f008.inc"
};
static Sint16 dat_0cb8f01c[36] = {
#include "ev0062/amy_action/dat_0cb8f01c.inc"
};
static NJS_COLOR dat_0cb8f064[30] = {
#include "ev0062/amy_action/dat_0cb8f064.inc"
};
static NJS_MESHSET dat_0cb8f0dc[1] = {
#include "ev0062/amy_action/dat_0cb8f0dc.inc"
};
static NJS_POINT3 dat_0cb8f0f4[12] = {
#include "ev0062/amy_action/dat_0cb8f0f4.inc"
};
static NJS_VECTOR dat_0cb8f184[12] = {
#include "ev0062/amy_action/dat_0cb8f184.inc"
};
static NJS_MODEL dat_0cb8f214 = {
#include "ev0062/amy_action/dat_0cb8f214.inc"
};
static NJS_OBJECT dat_0cb8f23c = {
#include "ev0062/amy_action/dat_0cb8f23c.inc"
};
static NJS_OBJECT dat_0cb8f270 = {
#include "ev0062/amy_action/dat_0cb8f270.inc"
};
static NJS_OBJECT dat_0cb8f2a4 = {
#include "ev0062/amy_action/dat_0cb8f2a4.inc"
};
static NJS_OBJECT dat_0cb8f2d8 = {
#include "ev0062/amy_action/dat_0cb8f2d8.inc"
};
static NJS_MATERIAL dat_0cb8f30c[1] = {
#include "ev0062/amy_action/dat_0cb8f30c.inc"
};
static Sint16 dat_0cb8f320[48] = {
#include "ev0062/amy_action/dat_0cb8f320.inc"
};
static NJS_COLOR dat_0cb8f380[38] = {
#include "ev0062/amy_action/dat_0cb8f380.inc"
};
static NJS_MESHSET dat_0cb8f418[1] = {
#include "ev0062/amy_action/dat_0cb8f418.inc"
};
static NJS_POINT3 dat_0cb8f430[12] = {
#include "ev0062/amy_action/dat_0cb8f430.inc"
};
static NJS_VECTOR dat_0cb8f4c0[12] = {
#include "ev0062/amy_action/dat_0cb8f4c0.inc"
};
static NJS_MODEL dat_0cb8f550 = {
#include "ev0062/amy_action/dat_0cb8f550.inc"
};
static NJS_OBJECT dat_0cb8f578 = {
#include "ev0062/amy_action/dat_0cb8f578.inc"
};
static NJS_OBJECT dat_0cb8f5ac = {
#include "ev0062/amy_action/dat_0cb8f5ac.inc"
};
static NJS_OBJECT dat_0cb8f5e0 = {
#include "ev0062/amy_action/dat_0cb8f5e0.inc"
};
static NJS_OBJECT dat_0cb8f614 = {
#include "ev0062/amy_action/dat_0cb8f614.inc"
};
static NJS_MATERIAL dat_0cb8f648[1] = {
#include "ev0062/amy_action/dat_0cb8f648.inc"
};
static Sint16 dat_0cb8f65c[48] = {
#include "ev0062/amy_action/dat_0cb8f65c.inc"
};
static NJS_COLOR dat_0cb8f6bc[38] = {
#include "ev0062/amy_action/dat_0cb8f6bc.inc"
};
static NJS_MESHSET dat_0cb8f754[1] = {
#include "ev0062/amy_action/dat_0cb8f754.inc"
};
static NJS_POINT3 dat_0cb8f76c[12] = {
#include "ev0062/amy_action/dat_0cb8f76c.inc"
};
static NJS_VECTOR dat_0cb8f7fc[12] = {
#include "ev0062/amy_action/dat_0cb8f7fc.inc"
};
static NJS_MODEL dat_0cb8f88c = {
#include "ev0062/amy_action/dat_0cb8f88c.inc"
};
static NJS_OBJECT dat_0cb8f8b4 = {
#include "ev0062/amy_action/dat_0cb8f8b4.inc"
};
static NJS_OBJECT dat_0cb8f8e8 = {
#include "ev0062/amy_action/dat_0cb8f8e8.inc"
};
static NJS_OBJECT dat_0cb8f91c = {
#include "ev0062/amy_action/dat_0cb8f91c.inc"
};
static NJS_OBJECT dat_0cb8f950 = {
#include "ev0062/amy_action/dat_0cb8f950.inc"
};
static NJS_MATERIAL dat_0cb8f984[2] = {
#include "ev0062/amy_action/dat_0cb8f984.inc"
};
static Sint16 dat_0cb8f9ac[54] = {
#include "ev0062/amy_action/dat_0cb8f9ac.inc"
};
static Sint16 dat_0cb8fa18[22] = {
#include "ev0062/amy_action/dat_0cb8fa18.inc"
};
static NJS_COLOR dat_0cb8fa44[48] = {
#include "ev0062/amy_action/dat_0cb8fa44.inc"
};
static NJS_COLOR dat_0cb8fb04[18] = {
#include "ev0062/amy_action/dat_0cb8fb04.inc"
};
static NJS_MESHSET dat_0cb8fb4c[2] = {
#include "ev0062/amy_action/dat_0cb8fb4c.inc"
};
static NJS_POINT3 dat_0cb8fb7c[26] = {
#include "ev0062/amy_action/dat_0cb8fb7c.inc"
};
static NJS_VECTOR dat_0cb8fcb4[26] = {
#include "ev0062/amy_action/dat_0cb8fcb4.inc"
};
static NJS_MODEL dat_0cb8fdec = {
#include "ev0062/amy_action/dat_0cb8fdec.inc"
};
static NJS_OBJECT dat_0cb8fe14 = {
#include "ev0062/amy_action/dat_0cb8fe14.inc"
};
static NJS_OBJECT dat_0cb8fe48 = {
#include "ev0062/amy_action/dat_0cb8fe48.inc"
};
static NJS_OBJECT dat_0cb8fe7c = {
#include "ev0062/amy_action/dat_0cb8fe7c.inc"
};
static NJS_OBJECT dat_0cb8feb0 = {
#include "ev0062/amy_action/dat_0cb8feb0.inc"
};
static NJS_OBJECT ev0062_ver1_wing_object = {
#include "ev0062/amy_action/ev0062_ver1_wing_object.inc"
};
static NJS_MKEY_F dat_0cb8ff18[2] = {
#include "ev0062/amy_action/dat_0cb8ff18.inc"
};
static NJS_MKEY_A dat_0cb8ff38[2] = {
#include "ev0062/amy_action/dat_0cb8ff38.inc"
};
static NJS_MKEY_F dat_0cb8ff58[31] = {
#include "ev0062/amy_action/dat_0cb8ff58.inc"
};
static NJS_MKEY_A dat_0cb90148[30] = {
#include "ev0062/amy_action/dat_0cb90148.inc"
};
static NJS_MKEY_F dat_0cb90328[2] = {
#include "ev0062/amy_action/dat_0cb90328.inc"
};
static NJS_MKEY_A dat_0cb90348[2] = {
#include "ev0062/amy_action/dat_0cb90348.inc"
};
static NJS_MKEY_F dat_0cb90368[2] = {
#include "ev0062/amy_action/dat_0cb90368.inc"
};
static NJS_MKEY_A dat_0cb90388[2] = {
#include "ev0062/amy_action/dat_0cb90388.inc"
};
static NJS_MKEY_F dat_0cb903a8[2] = {
#include "ev0062/amy_action/dat_0cb903a8.inc"
};
static NJS_MKEY_A dat_0cb903c8[2] = {
#include "ev0062/amy_action/dat_0cb903c8.inc"
};
static NJS_MKEY_F dat_0cb903e8[2] = {
#include "ev0062/amy_action/dat_0cb903e8.inc"
};
static NJS_MKEY_A dat_0cb90408[2] = {
#include "ev0062/amy_action/dat_0cb90408.inc"
};
static NJS_MKEY_F dat_0cb90428[2] = {
#include "ev0062/amy_action/dat_0cb90428.inc"
};
static NJS_MKEY_A dat_0cb90448[31] = {
#include "ev0062/amy_action/dat_0cb90448.inc"
};
static NJS_MKEY_F dat_0cb90638[2] = {
#include "ev0062/amy_action/dat_0cb90638.inc"
};
static NJS_MKEY_A dat_0cb90658[2] = {
#include "ev0062/amy_action/dat_0cb90658.inc"
};
static NJS_MKEY_F dat_0cb90678[2] = {
#include "ev0062/amy_action/dat_0cb90678.inc"
};
static NJS_MKEY_A dat_0cb90698[2] = {
#include "ev0062/amy_action/dat_0cb90698.inc"
};
static NJS_MKEY_F dat_0cb906b8[2] = {
#include "ev0062/amy_action/dat_0cb906b8.inc"
};
static NJS_MKEY_A dat_0cb906d8[2] = {
#include "ev0062/amy_action/dat_0cb906d8.inc"
};
static NJS_MKEY_F dat_0cb906f8[2] = {
#include "ev0062/amy_action/dat_0cb906f8.inc"
};
static NJS_MKEY_A dat_0cb90718[31] = {
#include "ev0062/amy_action/dat_0cb90718.inc"
};
static NJS_MKEY_F dat_0cb90908[2] = {
#include "ev0062/amy_action/dat_0cb90908.inc"
};
static NJS_MKEY_A dat_0cb90928[2] = {
#include "ev0062/amy_action/dat_0cb90928.inc"
};
static NJS_MKEY_F dat_0cb90948[2] = {
#include "ev0062/amy_action/dat_0cb90948.inc"
};
static NJS_MKEY_A dat_0cb90968[2] = {
#include "ev0062/amy_action/dat_0cb90968.inc"
};
static NJS_MKEY_F dat_0cb90988[2] = {
#include "ev0062/amy_action/dat_0cb90988.inc"
};
static NJS_MKEY_A dat_0cb909a8[2] = {
#include "ev0062/amy_action/dat_0cb909a8.inc"
};
static NJS_MKEY_F dat_0cb909c8[2] = {
#include "ev0062/amy_action/dat_0cb909c8.inc"
};
static NJS_MKEY_A dat_0cb909e8[2] = {
#include "ev0062/amy_action/dat_0cb909e8.inc"
};
static NJS_MKEY_F dat_0cb90a08[2] = {
#include "ev0062/amy_action/dat_0cb90a08.inc"
};
static NJS_MKEY_A dat_0cb90a28[2] = {
#include "ev0062/amy_action/dat_0cb90a28.inc"
};
static NJS_MKEY_F dat_0cb90a48[2] = {
#include "ev0062/amy_action/dat_0cb90a48.inc"
};
static NJS_MKEY_A dat_0cb90a68[2] = {
#include "ev0062/amy_action/dat_0cb90a68.inc"
};
static NJS_MKEY_F dat_0cb90a88[2] = {
#include "ev0062/amy_action/dat_0cb90a88.inc"
};
static NJS_MKEY_A dat_0cb90aa8[2] = {
#include "ev0062/amy_action/dat_0cb90aa8.inc"
};
static NJS_MKEY_F dat_0cb90ac8[2] = {
#include "ev0062/amy_action/dat_0cb90ac8.inc"
};
static NJS_MKEY_A dat_0cb90ae8[2] = {
#include "ev0062/amy_action/dat_0cb90ae8.inc"
};
static NJS_MKEY_F dat_0cb90b08[2] = {
#include "ev0062/amy_action/dat_0cb90b08.inc"
};
static NJS_MKEY_A dat_0cb90b28[2] = {
#include "ev0062/amy_action/dat_0cb90b28.inc"
};
static NJS_MKEY_F dat_0cb90b48[2] = {
#include "ev0062/amy_action/dat_0cb90b48.inc"
};
static NJS_MKEY_A dat_0cb90b68[2] = {
#include "ev0062/amy_action/dat_0cb90b68.inc"
};
static NJS_MKEY_F dat_0cb90b88[2] = {
#include "ev0062/amy_action/dat_0cb90b88.inc"
};
static NJS_MKEY_A dat_0cb90ba8[2] = {
#include "ev0062/amy_action/dat_0cb90ba8.inc"
};
static NJS_MKEY_F dat_0cb90bc8[2] = {
#include "ev0062/amy_action/dat_0cb90bc8.inc"
};
static NJS_MKEY_A dat_0cb90be8[2] = {
#include "ev0062/amy_action/dat_0cb90be8.inc"
};
static NJS_MKEY_F dat_0cb90c08[2] = {
#include "ev0062/amy_action/dat_0cb90c08.inc"
};
static NJS_MKEY_A dat_0cb90c28[2] = {
#include "ev0062/amy_action/dat_0cb90c28.inc"
};
static NJS_MKEY_F dat_0cb90c48[2] = {
#include "ev0062/amy_action/dat_0cb90c48.inc"
};
static NJS_MKEY_A dat_0cb90c68[2] = {
#include "ev0062/amy_action/dat_0cb90c68.inc"
};
static NJS_MKEY_F dat_0cb90c88[2] = {
#include "ev0062/amy_action/dat_0cb90c88.inc"
};
static NJS_MKEY_A dat_0cb90ca8[31] = {
#include "ev0062/amy_action/dat_0cb90ca8.inc"
};
static NJS_MKEY_F dat_0cb90e98[2] = {
#include "ev0062/amy_action/dat_0cb90e98.inc"
};
static NJS_MKEY_A dat_0cb90eb8[2] = {
#include "ev0062/amy_action/dat_0cb90eb8.inc"
};
static NJS_MKEY_F dat_0cb90ed8[2] = {
#include "ev0062/amy_action/dat_0cb90ed8.inc"
};
static NJS_MKEY_A dat_0cb90ef8[2] = {
#include "ev0062/amy_action/dat_0cb90ef8.inc"
};
static NJS_MKEY_F dat_0cb90f18[2] = {
#include "ev0062/amy_action/dat_0cb90f18.inc"
};
static NJS_MKEY_A dat_0cb90f38[2] = {
#include "ev0062/amy_action/dat_0cb90f38.inc"
};
static NJS_MKEY_F dat_0cb90f58[2] = {
#include "ev0062/amy_action/dat_0cb90f58.inc"
};
static NJS_MKEY_A dat_0cb90f78[2] = {
#include "ev0062/amy_action/dat_0cb90f78.inc"
};
static NJS_MKEY_F dat_0cb90f98[2] = {
#include "ev0062/amy_action/dat_0cb90f98.inc"
};
static NJS_MKEY_A dat_0cb90fb8[2] = {
#include "ev0062/amy_action/dat_0cb90fb8.inc"
};
static NJS_MKEY_F dat_0cb90fd8[2] = {
#include "ev0062/amy_action/dat_0cb90fd8.inc"
};
static NJS_MKEY_A dat_0cb90ff8[2] = {
#include "ev0062/amy_action/dat_0cb90ff8.inc"
};
static NJS_MKEY_F dat_0cb91018[2] = {
#include "ev0062/amy_action/dat_0cb91018.inc"
};
static NJS_MKEY_A dat_0cb91038[2] = {
#include "ev0062/amy_action/dat_0cb91038.inc"
};
static NJS_MKEY_F dat_0cb91058[2] = {
#include "ev0062/amy_action/dat_0cb91058.inc"
};
static NJS_MKEY_A dat_0cb91078[2] = {
#include "ev0062/amy_action/dat_0cb91078.inc"
};
static NJS_MKEY_F dat_0cb91098[2] = {
#include "ev0062/amy_action/dat_0cb91098.inc"
};
static NJS_MKEY_A dat_0cb910b8[2] = {
#include "ev0062/amy_action/dat_0cb910b8.inc"
};
static NJS_MKEY_F dat_0cb910d8[2] = {
#include "ev0062/amy_action/dat_0cb910d8.inc"
};
static NJS_MKEY_A dat_0cb910f8[2] = {
#include "ev0062/amy_action/dat_0cb910f8.inc"
};
static NJS_MKEY_F dat_0cb91118[31] = {
#include "ev0062/amy_action/dat_0cb91118.inc"
};
static NJS_MKEY_A dat_0cb91308[2] = {
#include "ev0062/amy_action/dat_0cb91308.inc"
};
static NJS_MKEY_F dat_0cb91328[31] = {
#include "ev0062/amy_action/dat_0cb91328.inc"
};
static NJS_MKEY_A dat_0cb91518[2] = {
#include "ev0062/amy_action/dat_0cb91518.inc"
};
static NJS_MKEY_F dat_0cb91538[2] = {
#include "ev0062/amy_action/dat_0cb91538.inc"
};
static NJS_MKEY_A dat_0cb91558[2] = {
#include "ev0062/amy_action/dat_0cb91558.inc"
};
static NJS_MKEY_F dat_0cb91578[31] = {
#include "ev0062/amy_action/dat_0cb91578.inc"
};
static NJS_MKEY_A dat_0cb91768[2] = {
#include "ev0062/amy_action/dat_0cb91768.inc"
};
static NJS_MKEY_F dat_0cb91788[2] = {
#include "ev0062/amy_action/dat_0cb91788.inc"
};
static NJS_MKEY_A dat_0cb917a8[2] = {
#include "ev0062/amy_action/dat_0cb917a8.inc"
};
static NJS_MKEY_F dat_0cb917c8[2] = {
#include "ev0062/amy_action/dat_0cb917c8.inc"
};
static NJS_MKEY_A dat_0cb917e8[2] = {
#include "ev0062/amy_action/dat_0cb917e8.inc"
};
static NJS_MKEY_F dat_0cb91808[2] = {
#include "ev0062/amy_action/dat_0cb91808.inc"
};
static NJS_MKEY_A dat_0cb91828[2] = {
#include "ev0062/amy_action/dat_0cb91828.inc"
};
static NJS_MKEY_F dat_0cb91848[2] = {
#include "ev0062/amy_action/dat_0cb91848.inc"
};
static NJS_MKEY_A dat_0cb91868[2] = {
#include "ev0062/amy_action/dat_0cb91868.inc"
};
static NJS_MKEY_F dat_0cb91888[2] = {
#include "ev0062/amy_action/dat_0cb91888.inc"
};
static NJS_MKEY_A dat_0cb918a8[2] = {
#include "ev0062/amy_action/dat_0cb918a8.inc"
};
static NJS_MKEY_F dat_0cb918c8[2] = {
#include "ev0062/amy_action/dat_0cb918c8.inc"
};
static NJS_MKEY_A dat_0cb918e8[2] = {
#include "ev0062/amy_action/dat_0cb918e8.inc"
};
static NJS_MKEY_F dat_0cb91908[2] = {
#include "ev0062/amy_action/dat_0cb91908.inc"
};
static NJS_MKEY_A dat_0cb91928[2] = {
#include "ev0062/amy_action/dat_0cb91928.inc"
};
static NJS_MKEY_F dat_0cb91948[2] = {
#include "ev0062/amy_action/dat_0cb91948.inc"
};
static NJS_MKEY_A dat_0cb91968[2] = {
#include "ev0062/amy_action/dat_0cb91968.inc"
};
static NJS_MKEY_F dat_0cb91988[2] = {
#include "ev0062/amy_action/dat_0cb91988.inc"
};
static NJS_MKEY_A dat_0cb919a8[2] = {
#include "ev0062/amy_action/dat_0cb919a8.inc"
};
static NJS_MKEY_F dat_0cb919c8[2] = {
#include "ev0062/amy_action/dat_0cb919c8.inc"
};
static NJS_MKEY_A dat_0cb919e8[2] = {
#include "ev0062/amy_action/dat_0cb919e8.inc"
};
static NJS_MKEY_F dat_0cb91a08[2] = {
#include "ev0062/amy_action/dat_0cb91a08.inc"
};
static NJS_MKEY_A dat_0cb91a28[2] = {
#include "ev0062/amy_action/dat_0cb91a28.inc"
};
static NJS_MKEY_F dat_0cb91a48[2] = {
#include "ev0062/amy_action/dat_0cb91a48.inc"
};
static NJS_MKEY_A dat_0cb91a68[2] = {
#include "ev0062/amy_action/dat_0cb91a68.inc"
};
static NJS_MKEY_F dat_0cb91a88[2] = {
#include "ev0062/amy_action/dat_0cb91a88.inc"
};
static NJS_MKEY_A dat_0cb91aa8[2] = {
#include "ev0062/amy_action/dat_0cb91aa8.inc"
};
static NJS_MKEY_F dat_0cb91ac8[2] = {
#include "ev0062/amy_action/dat_0cb91ac8.inc"
};
static NJS_MKEY_A dat_0cb91ae8[2] = {
#include "ev0062/amy_action/dat_0cb91ae8.inc"
};
static NJS_MKEY_F dat_0cb91b08[2] = {
#include "ev0062/amy_action/dat_0cb91b08.inc"
};
static NJS_MKEY_A dat_0cb91b28[2] = {
#include "ev0062/amy_action/dat_0cb91b28.inc"
};
static NJS_MKEY_F dat_0cb91b48[2] = {
#include "ev0062/amy_action/dat_0cb91b48.inc"
};
static NJS_MKEY_A dat_0cb91b68[2] = {
#include "ev0062/amy_action/dat_0cb91b68.inc"
};
static NJS_MKEY_F dat_0cb91b88[2] = {
#include "ev0062/amy_action/dat_0cb91b88.inc"
};
static NJS_MKEY_A dat_0cb91ba8[2] = {
#include "ev0062/amy_action/dat_0cb91ba8.inc"
};
static NJS_MDATA2 dat_0cb91bc8[57] = {
#include "ev0062/amy_action/dat_0cb91bc8.inc"
};
static NJS_MOTION ev0062_ver1_wing_motion = {
#include "ev0062/amy_action/ev0062_ver1_wing_motion.inc"
};
NJS_ACTION ev0062_ver1_wing_action = {
#include "ev0062/amy_action/ev0062_ver1_wing_action.inc"
};
static NJS_TEXNAME ev0062_ver2_wing_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0062_ver2_wing_texlist = {
#include "ev0062/amy_action/ev0062_ver2_wing_texlist.inc"
};
NJS_MATERIAL dat_0cb92004[1] = {
#include "ev0062/amy_action/dat_0cb92004.inc"
};
