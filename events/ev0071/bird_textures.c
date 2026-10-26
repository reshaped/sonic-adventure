/* the bird's two texlists, two models and five actions. then
 * ev0071_wing_p_object with its texlist and action, ev0071_wing_t_texlist,
 * and a material of ev0071_wing_t_action's model. ev0071_bird_action_0 to 2
 * move the bird's second model, ev0071_bird_object */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0071/bird_textures.h"

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
#include "ev0071/bird_textures/text.inc"

/* the same twelve names as ev0071_bird_texlist's. no word in EV0071 or
 * 1ST_READ.BIN points at this list's texlist, dat_0cbb2e78 */
static NJS_TEXNAME dat_0cbb2de8[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
static NJS_TEXLIST dat_0cbb2e78 = {
#include "ev0071/bird_textures/dat_0cbb2e78.inc"
};
static NJS_MATERIAL dat_0cbb2e80[1] = {
#include "ev0071/bird_textures/dat_0cbb2e80.inc"
};
static Sint16 dat_0cbb2e94[6] = {
#include "ev0071/bird_textures/dat_0cbb2e94.inc"
};
static NJS_COLOR dat_0cbb2ea0[4] = {
#include "ev0071/bird_textures/dat_0cbb2ea0.inc"
};
static NJS_MESHSET dat_0cbb2eb0[1] = {
#include "ev0071/bird_textures/dat_0cbb2eb0.inc"
};
static NJS_POINT3 dat_0cbb2ec8[4] = {
#include "ev0071/bird_textures/dat_0cbb2ec8.inc"
};
static NJS_VECTOR dat_0cbb2ef8[4] = {
#include "ev0071/bird_textures/dat_0cbb2ef8.inc"
};
static NJS_MODEL dat_0cbb2f28 = {
#include "ev0071/bird_textures/dat_0cbb2f28.inc"
};
static NJS_OBJECT dat_0cbb2f50 = {
#include "ev0071/bird_textures/dat_0cbb2f50.inc"
};
static NJS_MATERIAL dat_0cbb2f84[1] = {
#include "ev0071/bird_textures/dat_0cbb2f84.inc"
};
static Sint16 dat_0cbb2f98[12] = {
#include "ev0071/bird_textures/dat_0cbb2f98.inc"
};
static NJS_COLOR dat_0cbb2fb0[9] = {
#include "ev0071/bird_textures/dat_0cbb2fb0.inc"
};
static NJS_MESHSET dat_0cbb2fd4[1] = {
#include "ev0071/bird_textures/dat_0cbb2fd4.inc"
};
static NJS_POINT3 dat_0cbb2fec[7] = {
#include "ev0071/bird_textures/dat_0cbb2fec.inc"
};
static NJS_VECTOR dat_0cbb3040[7] = {
#include "ev0071/bird_textures/dat_0cbb3040.inc"
};
static NJS_MODEL dat_0cbb3094 = {
#include "ev0071/bird_textures/dat_0cbb3094.inc"
};
static NJS_OBJECT dat_0cbb30bc = {
#include "ev0071/bird_textures/dat_0cbb30bc.inc"
};
static NJS_OBJECT dat_0cbb30f0 = {
#include "ev0071/bird_textures/dat_0cbb30f0.inc"
};
static NJS_OBJECT dat_0cbb3124 = {
#include "ev0071/bird_textures/dat_0cbb3124.inc"
};
static NJS_OBJECT dat_0cbb3158 = {
#include "ev0071/bird_textures/dat_0cbb3158.inc"
};
static NJS_OBJECT dat_0cbb318c = {
#include "ev0071/bird_textures/dat_0cbb318c.inc"
};
static NJS_OBJECT dat_0cbb31c0 = {
#include "ev0071/bird_textures/dat_0cbb31c0.inc"
};
static NJS_OBJECT dat_0cbb31f4 = {
#include "ev0071/bird_textures/dat_0cbb31f4.inc"
};
static NJS_MATERIAL dat_0cbb3228[1] = {
#include "ev0071/bird_textures/dat_0cbb3228.inc"
};
static Sint16 dat_0cbb323c[6] = {
#include "ev0071/bird_textures/dat_0cbb323c.inc"
};
static NJS_COLOR dat_0cbb3248[4] = {
#include "ev0071/bird_textures/dat_0cbb3248.inc"
};
static NJS_MESHSET dat_0cbb3258[1] = {
#include "ev0071/bird_textures/dat_0cbb3258.inc"
};
static NJS_POINT3 dat_0cbb3270[4] = {
#include "ev0071/bird_textures/dat_0cbb3270.inc"
};
static NJS_VECTOR dat_0cbb32a0[4] = {
#include "ev0071/bird_textures/dat_0cbb32a0.inc"
};
static NJS_MODEL dat_0cbb32d0 = {
#include "ev0071/bird_textures/dat_0cbb32d0.inc"
};
static NJS_OBJECT dat_0cbb32f8 = {
#include "ev0071/bird_textures/dat_0cbb32f8.inc"
};
static NJS_MATERIAL dat_0cbb332c[1] = {
#include "ev0071/bird_textures/dat_0cbb332c.inc"
};
static Sint16 dat_0cbb3340[12] = {
#include "ev0071/bird_textures/dat_0cbb3340.inc"
};
static NJS_COLOR dat_0cbb3358[9] = {
#include "ev0071/bird_textures/dat_0cbb3358.inc"
};
static NJS_MESHSET dat_0cbb337c[1] = {
#include "ev0071/bird_textures/dat_0cbb337c.inc"
};
static NJS_POINT3 dat_0cbb3394[7] = {
#include "ev0071/bird_textures/dat_0cbb3394.inc"
};
static NJS_VECTOR dat_0cbb33e8[7] = {
#include "ev0071/bird_textures/dat_0cbb33e8.inc"
};
static NJS_MODEL dat_0cbb343c = {
#include "ev0071/bird_textures/dat_0cbb343c.inc"
};
static NJS_OBJECT dat_0cbb3464 = {
#include "ev0071/bird_textures/dat_0cbb3464.inc"
};
static NJS_OBJECT dat_0cbb3498 = {
#include "ev0071/bird_textures/dat_0cbb3498.inc"
};
static NJS_OBJECT dat_0cbb34cc = {
#include "ev0071/bird_textures/dat_0cbb34cc.inc"
};
static NJS_OBJECT dat_0cbb3500 = {
#include "ev0071/bird_textures/dat_0cbb3500.inc"
};
static NJS_OBJECT dat_0cbb3534 = {
#include "ev0071/bird_textures/dat_0cbb3534.inc"
};
static NJS_OBJECT dat_0cbb3568 = {
#include "ev0071/bird_textures/dat_0cbb3568.inc"
};
static NJS_OBJECT dat_0cbb359c = {
#include "ev0071/bird_textures/dat_0cbb359c.inc"
};
static NJS_OBJECT dat_0cbb35d0 = {
#include "ev0071/bird_textures/dat_0cbb35d0.inc"
};
static NJS_OBJECT dat_0cbb3604 = {
#include "ev0071/bird_textures/dat_0cbb3604.inc"
};
static NJS_OBJECT dat_0cbb3638 = {
#include "ev0071/bird_textures/dat_0cbb3638.inc"
};
static NJS_OBJECT dat_0cbb366c = {
#include "ev0071/bird_textures/dat_0cbb366c.inc"
};
static NJS_OBJECT dat_0cbb36a0 = {
#include "ev0071/bird_textures/dat_0cbb36a0.inc"
};
static NJS_MATERIAL dat_0cbb36d4[1] = {
#include "ev0071/bird_textures/dat_0cbb36d4.inc"
};
static Sint16 dat_0cbb36e8[18] = {
#include "ev0071/bird_textures/dat_0cbb36e8.inc"
};
static NJS_COLOR dat_0cbb370c[15] = {
#include "ev0071/bird_textures/dat_0cbb370c.inc"
};
static NJS_MESHSET dat_0cbb3748[1] = {
#include "ev0071/bird_textures/dat_0cbb3748.inc"
};
static NJS_POINT3 dat_0cbb3760[7] = {
#include "ev0071/bird_textures/dat_0cbb3760.inc"
};
static NJS_VECTOR dat_0cbb37b4[7] = {
#include "ev0071/bird_textures/dat_0cbb37b4.inc"
};
static NJS_MODEL dat_0cbb3808 = {
#include "ev0071/bird_textures/dat_0cbb3808.inc"
};
static NJS_OBJECT dat_0cbb3830 = {
#include "ev0071/bird_textures/dat_0cbb3830.inc"
};
static NJS_OBJECT dat_0cbb3864 = {
#include "ev0071/bird_textures/dat_0cbb3864.inc"
};
static NJS_OBJECT dat_0cbb3898 = {
#include "ev0071/bird_textures/dat_0cbb3898.inc"
};
static NJS_OBJECT dat_0cbb38cc = {
#include "ev0071/bird_textures/dat_0cbb38cc.inc"
};
static NJS_MATERIAL dat_0cbb3900[1] = {
#include "ev0071/bird_textures/dat_0cbb3900.inc"
};
static Sint16 dat_0cbb3914[10] = {
#include "ev0071/bird_textures/dat_0cbb3914.inc"
};
static NJS_COLOR dat_0cbb3928[8] = {
#include "ev0071/bird_textures/dat_0cbb3928.inc"
};
static NJS_MESHSET dat_0cbb3948[1] = {
#include "ev0071/bird_textures/dat_0cbb3948.inc"
};
static NJS_POINT3 dat_0cbb3960[8] = {
#include "ev0071/bird_textures/dat_0cbb3960.inc"
};
static NJS_VECTOR dat_0cbb39c0[8] = {
#include "ev0071/bird_textures/dat_0cbb39c0.inc"
};
static NJS_MODEL dat_0cbb3a20 = {
#include "ev0071/bird_textures/dat_0cbb3a20.inc"
};
static NJS_OBJECT dat_0cbb3a48 = {
#include "ev0071/bird_textures/dat_0cbb3a48.inc"
};
static NJS_MATERIAL dat_0cbb3a7c[1] = {
#include "ev0071/bird_textures/dat_0cbb3a7c.inc"
};
static Sint16 dat_0cbb3a90[10] = {
#include "ev0071/bird_textures/dat_0cbb3a90.inc"
};
static NJS_COLOR dat_0cbb3aa4[8] = {
#include "ev0071/bird_textures/dat_0cbb3aa4.inc"
};
static NJS_MESHSET dat_0cbb3ac4[1] = {
#include "ev0071/bird_textures/dat_0cbb3ac4.inc"
};
static NJS_POINT3 dat_0cbb3adc[8] = {
#include "ev0071/bird_textures/dat_0cbb3adc.inc"
};
static NJS_VECTOR dat_0cbb3b3c[8] = {
#include "ev0071/bird_textures/dat_0cbb3b3c.inc"
};
static NJS_MODEL dat_0cbb3b9c = {
#include "ev0071/bird_textures/dat_0cbb3b9c.inc"
};
static NJS_OBJECT dat_0cbb3bc4 = {
#include "ev0071/bird_textures/dat_0cbb3bc4.inc"
};
static NJS_MATERIAL dat_0cbb3bf8[1] = {
#include "ev0071/bird_textures/dat_0cbb3bf8.inc"
};
static Sint16 dat_0cbb3c0c[16] = {
#include "ev0071/bird_textures/dat_0cbb3c0c.inc"
};
static NJS_COLOR dat_0cbb3c2c[12] = {
#include "ev0071/bird_textures/dat_0cbb3c2c.inc"
};
static NJS_MESHSET dat_0cbb3c5c[1] = {
#include "ev0071/bird_textures/dat_0cbb3c5c.inc"
};
static NJS_POINT3 dat_0cbb3c74[12] = {
#include "ev0071/bird_textures/dat_0cbb3c74.inc"
};
static NJS_VECTOR dat_0cbb3d04[12] = {
#include "ev0071/bird_textures/dat_0cbb3d04.inc"
};
static NJS_MODEL dat_0cbb3d94 = {
#include "ev0071/bird_textures/dat_0cbb3d94.inc"
};
static NJS_OBJECT dat_0cbb3dbc = {
#include "ev0071/bird_textures/dat_0cbb3dbc.inc"
};
static NJS_MATERIAL dat_0cbb3df0[1] = {
#include "ev0071/bird_textures/dat_0cbb3df0.inc"
};
static Sint16 dat_0cbb3e04[16] = {
#include "ev0071/bird_textures/dat_0cbb3e04.inc"
};
static NJS_COLOR dat_0cbb3e24[12] = {
#include "ev0071/bird_textures/dat_0cbb3e24.inc"
};
static NJS_MESHSET dat_0cbb3e54[1] = {
#include "ev0071/bird_textures/dat_0cbb3e54.inc"
};
static NJS_POINT3 dat_0cbb3e6c[12] = {
#include "ev0071/bird_textures/dat_0cbb3e6c.inc"
};
static NJS_VECTOR dat_0cbb3efc[12] = {
#include "ev0071/bird_textures/dat_0cbb3efc.inc"
};
static NJS_MODEL dat_0cbb3f8c = {
#include "ev0071/bird_textures/dat_0cbb3f8c.inc"
};
static NJS_OBJECT dat_0cbb3fb4 = {
#include "ev0071/bird_textures/dat_0cbb3fb4.inc"
};
static NJS_MATERIAL dat_0cbb3fe8[4] = {
#include "ev0071/bird_textures/dat_0cbb3fe8.inc"
};
static Sint16 dat_0cbb4038[239] = {
#include "ev0071/bird_textures/dat_0cbb4038.inc"
};
static Sint16 dat_0cbb4216[22] = {
#include "ev0071/bird_textures/dat_0cbb4216.inc"
};
static Sint16 dat_0cbb4242[21] = {
#include "ev0071/bird_textures/dat_0cbb4242.inc"
};
static Sint16 dat_0cbb426c[22] = {
#include "ev0071/bird_textures/dat_0cbb426c.inc"
};
static NJS_COLOR dat_0cbb4298[209] = {
#include "ev0071/bird_textures/dat_0cbb4298.inc"
};
static NJS_COLOR dat_0cbb45dc[20] = {
#include "ev0071/bird_textures/dat_0cbb45dc.inc"
};
static NJS_COLOR dat_0cbb462c[16] = {
#include "ev0071/bird_textures/dat_0cbb462c.inc"
};
static NJS_COLOR dat_0cbb466c[18] = {
#include "ev0071/bird_textures/dat_0cbb466c.inc"
};
static NJS_MESHSET dat_0cbb46b4[4] = {
#include "ev0071/bird_textures/dat_0cbb46b4.inc"
};
static NJS_POINT3 dat_0cbb4714[127] = {
#include "ev0071/bird_textures/dat_0cbb4714.inc"
};
static NJS_VECTOR dat_0cbb4d08[127] = {
#include "ev0071/bird_textures/dat_0cbb4d08.inc"
};
static NJS_MODEL dat_0cbb52fc = {
#include "ev0071/bird_textures/dat_0cbb52fc.inc"
};
static NJS_OBJECT dat_0cbb5324 = {
#include "ev0071/bird_textures/dat_0cbb5324.inc"
};
static NJS_OBJECT dat_0cbb5358 = {
#include "ev0071/bird_textures/dat_0cbb5358.inc"
};
static NJS_OBJECT dat_0cbb538c = {
#include "ev0071/bird_textures/dat_0cbb538c.inc"
};
static NJS_OBJECT dat_0cbb53c0 = {
#include "ev0071/bird_textures/dat_0cbb53c0.inc"
};
static NJS_MATERIAL dat_0cbb53f4[1] = {
#include "ev0071/bird_textures/dat_0cbb53f4.inc"
};
static Sint16 dat_0cbb5408[8] = {
#include "ev0071/bird_textures/dat_0cbb5408.inc"
};
static NJS_COLOR dat_0cbb5418[6] = {
#include "ev0071/bird_textures/dat_0cbb5418.inc"
};
static NJS_MESHSET dat_0cbb5430[1] = {
#include "ev0071/bird_textures/dat_0cbb5430.inc"
};
static NJS_POINT3 dat_0cbb5448[6] = {
#include "ev0071/bird_textures/dat_0cbb5448.inc"
};
static NJS_VECTOR dat_0cbb5490[6] = {
#include "ev0071/bird_textures/dat_0cbb5490.inc"
};
static NJS_MODEL dat_0cbb54d8 = {
#include "ev0071/bird_textures/dat_0cbb54d8.inc"
};
static NJS_OBJECT dat_0cbb5500 = {
#include "ev0071/bird_textures/dat_0cbb5500.inc"
};
static NJS_MATERIAL dat_0cbb5534[1] = {
#include "ev0071/bird_textures/dat_0cbb5534.inc"
};
static Sint16 dat_0cbb5548[22] = {
#include "ev0071/bird_textures/dat_0cbb5548.inc"
};
static NJS_COLOR dat_0cbb5574[16] = {
#include "ev0071/bird_textures/dat_0cbb5574.inc"
};
static NJS_MESHSET dat_0cbb55b4[1] = {
#include "ev0071/bird_textures/dat_0cbb55b4.inc"
};
static NJS_POINT3 dat_0cbb55cc[5] = {
#include "ev0071/bird_textures/dat_0cbb55cc.inc"
};
static NJS_VECTOR dat_0cbb5608[5] = {
#include "ev0071/bird_textures/dat_0cbb5608.inc"
};
static NJS_MODEL dat_0cbb5644 = {
#include "ev0071/bird_textures/dat_0cbb5644.inc"
};
static NJS_OBJECT dat_0cbb566c = {
#include "ev0071/bird_textures/dat_0cbb566c.inc"
};
static NJS_MATERIAL dat_0cbb56a0[1] = {
#include "ev0071/bird_textures/dat_0cbb56a0.inc"
};
static Sint16 dat_0cbb56b4[30] = {
#include "ev0071/bird_textures/dat_0cbb56b4.inc"
};
static NJS_COLOR dat_0cbb56f0[28] = {
#include "ev0071/bird_textures/dat_0cbb56f0.inc"
};
static NJS_MESHSET dat_0cbb5760[1] = {
#include "ev0071/bird_textures/dat_0cbb5760.inc"
};
static NJS_POINT3 dat_0cbb5778[18] = {
#include "ev0071/bird_textures/dat_0cbb5778.inc"
};
static NJS_VECTOR dat_0cbb5850[18] = {
#include "ev0071/bird_textures/dat_0cbb5850.inc"
};
static NJS_MODEL dat_0cbb5928 = {
#include "ev0071/bird_textures/dat_0cbb5928.inc"
};
static NJS_OBJECT dat_0cbb5950 = {
#include "ev0071/bird_textures/dat_0cbb5950.inc"
};
static NJS_MATERIAL dat_0cbb5984[1] = {
#include "ev0071/bird_textures/dat_0cbb5984.inc"
};
static Sint16 dat_0cbb5998[20] = {
#include "ev0071/bird_textures/dat_0cbb5998.inc"
};
static NJS_COLOR dat_0cbb59c0[16] = {
#include "ev0071/bird_textures/dat_0cbb59c0.inc"
};
static NJS_MESHSET dat_0cbb5a00[1] = {
#include "ev0071/bird_textures/dat_0cbb5a00.inc"
};
static NJS_POINT3 dat_0cbb5a18[12] = {
#include "ev0071/bird_textures/dat_0cbb5a18.inc"
};
static NJS_VECTOR dat_0cbb5aa8[12] = {
#include "ev0071/bird_textures/dat_0cbb5aa8.inc"
};
static NJS_MODEL dat_0cbb5b38 = {
#include "ev0071/bird_textures/dat_0cbb5b38.inc"
};
static NJS_OBJECT dat_0cbb5b60 = {
#include "ev0071/bird_textures/dat_0cbb5b60.inc"
};
static NJS_MATERIAL dat_0cbb5b94[1] = {
#include "ev0071/bird_textures/dat_0cbb5b94.inc"
};
static Sint16 dat_0cbb5ba8[10] = {
#include "ev0071/bird_textures/dat_0cbb5ba8.inc"
};
static NJS_COLOR dat_0cbb5bbc[8] = {
#include "ev0071/bird_textures/dat_0cbb5bbc.inc"
};
static NJS_MESHSET dat_0cbb5bdc[1] = {
#include "ev0071/bird_textures/dat_0cbb5bdc.inc"
};
static NJS_POINT3 dat_0cbb5bf4[5] = {
#include "ev0071/bird_textures/dat_0cbb5bf4.inc"
};
static NJS_VECTOR dat_0cbb5c30[5] = {
#include "ev0071/bird_textures/dat_0cbb5c30.inc"
};
static NJS_MODEL dat_0cbb5c6c = {
#include "ev0071/bird_textures/dat_0cbb5c6c.inc"
};
static NJS_OBJECT dat_0cbb5c94 = {
#include "ev0071/bird_textures/dat_0cbb5c94.inc"
};
static NJS_MATERIAL dat_0cbb5cc8[1] = {
#include "ev0071/bird_textures/dat_0cbb5cc8.inc"
};
static Sint16 dat_0cbb5cdc[10] = {
#include "ev0071/bird_textures/dat_0cbb5cdc.inc"
};
static NJS_COLOR dat_0cbb5cf0[8] = {
#include "ev0071/bird_textures/dat_0cbb5cf0.inc"
};
static NJS_MESHSET dat_0cbb5d10[1] = {
#include "ev0071/bird_textures/dat_0cbb5d10.inc"
};
static NJS_POINT3 dat_0cbb5d28[5] = {
#include "ev0071/bird_textures/dat_0cbb5d28.inc"
};
static NJS_VECTOR dat_0cbb5d64[5] = {
#include "ev0071/bird_textures/dat_0cbb5d64.inc"
};
static NJS_MODEL dat_0cbb5da0 = {
#include "ev0071/bird_textures/dat_0cbb5da0.inc"
};
static NJS_OBJECT dat_0cbb5dc8 = {
#include "ev0071/bird_textures/dat_0cbb5dc8.inc"
};
static NJS_MATERIAL dat_0cbb5dfc[1] = {
#include "ev0071/bird_textures/dat_0cbb5dfc.inc"
};
static Sint16 dat_0cbb5e10[24] = {
#include "ev0071/bird_textures/dat_0cbb5e10.inc"
};
static NJS_COLOR dat_0cbb5e40[20] = {
#include "ev0071/bird_textures/dat_0cbb5e40.inc"
};
static NJS_MESHSET dat_0cbb5e90[1] = {
#include "ev0071/bird_textures/dat_0cbb5e90.inc"
};
static NJS_POINT3 dat_0cbb5ea8[12] = {
#include "ev0071/bird_textures/dat_0cbb5ea8.inc"
};
static NJS_VECTOR dat_0cbb5f38[12] = {
#include "ev0071/bird_textures/dat_0cbb5f38.inc"
};
static NJS_MODEL dat_0cbb5fc8 = {
#include "ev0071/bird_textures/dat_0cbb5fc8.inc"
};
static NJS_OBJECT dat_0cbb5ff0 = {
#include "ev0071/bird_textures/dat_0cbb5ff0.inc"
};
static NJS_MATERIAL dat_0cbb6024[1] = {
#include "ev0071/bird_textures/dat_0cbb6024.inc"
};
static Sint16 dat_0cbb6038[36] = {
#include "ev0071/bird_textures/dat_0cbb6038.inc"
};
static NJS_COLOR dat_0cbb6080[30] = {
#include "ev0071/bird_textures/dat_0cbb6080.inc"
};
static NJS_MESHSET dat_0cbb60f8[1] = {
#include "ev0071/bird_textures/dat_0cbb60f8.inc"
};
static NJS_POINT3 dat_0cbb6110[12] = {
#include "ev0071/bird_textures/dat_0cbb6110.inc"
};
static NJS_VECTOR dat_0cbb61a0[12] = {
#include "ev0071/bird_textures/dat_0cbb61a0.inc"
};
static NJS_MODEL dat_0cbb6230 = {
#include "ev0071/bird_textures/dat_0cbb6230.inc"
};
static NJS_OBJECT dat_0cbb6258 = {
#include "ev0071/bird_textures/dat_0cbb6258.inc"
};
static NJS_OBJECT dat_0cbb628c = {
#include "ev0071/bird_textures/dat_0cbb628c.inc"
};
static NJS_OBJECT dat_0cbb62c0 = {
#include "ev0071/bird_textures/dat_0cbb62c0.inc"
};
static NJS_OBJECT dat_0cbb62f4 = {
#include "ev0071/bird_textures/dat_0cbb62f4.inc"
};
static NJS_MATERIAL dat_0cbb6328[1] = {
#include "ev0071/bird_textures/dat_0cbb6328.inc"
};
static Sint16 dat_0cbb633c[48] = {
#include "ev0071/bird_textures/dat_0cbb633c.inc"
};
static NJS_COLOR dat_0cbb639c[38] = {
#include "ev0071/bird_textures/dat_0cbb639c.inc"
};
static NJS_MESHSET dat_0cbb6434[1] = {
#include "ev0071/bird_textures/dat_0cbb6434.inc"
};
static NJS_POINT3 dat_0cbb644c[12] = {
#include "ev0071/bird_textures/dat_0cbb644c.inc"
};
static NJS_VECTOR dat_0cbb64dc[12] = {
#include "ev0071/bird_textures/dat_0cbb64dc.inc"
};
static NJS_MODEL dat_0cbb656c = {
#include "ev0071/bird_textures/dat_0cbb656c.inc"
};
static NJS_OBJECT dat_0cbb6594 = {
#include "ev0071/bird_textures/dat_0cbb6594.inc"
};
static NJS_OBJECT dat_0cbb65c8 = {
#include "ev0071/bird_textures/dat_0cbb65c8.inc"
};
static NJS_OBJECT dat_0cbb65fc = {
#include "ev0071/bird_textures/dat_0cbb65fc.inc"
};
static NJS_OBJECT dat_0cbb6630 = {
#include "ev0071/bird_textures/dat_0cbb6630.inc"
};
static NJS_MATERIAL dat_0cbb6664[1] = {
#include "ev0071/bird_textures/dat_0cbb6664.inc"
};
static Sint16 dat_0cbb6678[48] = {
#include "ev0071/bird_textures/dat_0cbb6678.inc"
};
static NJS_COLOR dat_0cbb66d8[38] = {
#include "ev0071/bird_textures/dat_0cbb66d8.inc"
};
static NJS_MESHSET dat_0cbb6770[1] = {
#include "ev0071/bird_textures/dat_0cbb6770.inc"
};
static NJS_POINT3 dat_0cbb6788[12] = {
#include "ev0071/bird_textures/dat_0cbb6788.inc"
};
static NJS_VECTOR dat_0cbb6818[12] = {
#include "ev0071/bird_textures/dat_0cbb6818.inc"
};
static NJS_MODEL dat_0cbb68a8 = {
#include "ev0071/bird_textures/dat_0cbb68a8.inc"
};
static NJS_OBJECT dat_0cbb68d0 = {
#include "ev0071/bird_textures/dat_0cbb68d0.inc"
};
static NJS_OBJECT dat_0cbb6904 = {
#include "ev0071/bird_textures/dat_0cbb6904.inc"
};
static NJS_OBJECT dat_0cbb6938 = {
#include "ev0071/bird_textures/dat_0cbb6938.inc"
};
static NJS_OBJECT dat_0cbb696c = {
#include "ev0071/bird_textures/dat_0cbb696c.inc"
};
static NJS_MATERIAL dat_0cbb69a0[2] = {
#include "ev0071/bird_textures/dat_0cbb69a0.inc"
};
static Sint16 dat_0cbb69c8[54] = {
#include "ev0071/bird_textures/dat_0cbb69c8.inc"
};
static Sint16 dat_0cbb6a34[22] = {
#include "ev0071/bird_textures/dat_0cbb6a34.inc"
};
static NJS_COLOR dat_0cbb6a60[48] = {
#include "ev0071/bird_textures/dat_0cbb6a60.inc"
};
static NJS_COLOR dat_0cbb6b20[18] = {
#include "ev0071/bird_textures/dat_0cbb6b20.inc"
};
static NJS_MESHSET dat_0cbb6b68[2] = {
#include "ev0071/bird_textures/dat_0cbb6b68.inc"
};
static NJS_POINT3 dat_0cbb6b98[26] = {
#include "ev0071/bird_textures/dat_0cbb6b98.inc"
};
static NJS_VECTOR dat_0cbb6cd0[26] = {
#include "ev0071/bird_textures/dat_0cbb6cd0.inc"
};
static NJS_MODEL dat_0cbb6e08 = {
#include "ev0071/bird_textures/dat_0cbb6e08.inc"
};
static NJS_OBJECT dat_0cbb6e30 = {
#include "ev0071/bird_textures/dat_0cbb6e30.inc"
};
static NJS_OBJECT dat_0cbb6e64 = {
#include "ev0071/bird_textures/dat_0cbb6e64.inc"
};
static NJS_OBJECT dat_0cbb6e98 = {
#include "ev0071/bird_textures/dat_0cbb6e98.inc"
};
static NJS_OBJECT dat_0cbb6ecc = {
#include "ev0071/bird_textures/dat_0cbb6ecc.inc"
};
/* this model matches ev0071_bird_object but for five of 57 nodes, shrunk to
 * a scale of 0.00001. of the words in EV0071 and 1ST_READ.BIN, only the
 * actions dat_0cbbbb5c and dat_0cbbd360 point at it. no word in either
 * points at those two actions */
static NJS_OBJECT dat_0cbb6f00 = {
#include "ev0071/bird_textures/dat_0cbb6f00.inc"
};
static NJS_TEXNAME ev0071_bird_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0071_bird_texlist = {
#include "ev0071/bird_textures/ev0071_bird_texlist.inc"
};
static NJS_MATERIAL dat_0cbb6fcc[1] = {
#include "ev0071/bird_textures/dat_0cbb6fcc.inc"
};
static Sint16 dat_0cbb6fe0[6] = {
#include "ev0071/bird_textures/dat_0cbb6fe0.inc"
};
static NJS_COLOR dat_0cbb6fec[4] = {
#include "ev0071/bird_textures/dat_0cbb6fec.inc"
};
static NJS_MESHSET dat_0cbb6ffc[1] = {
#include "ev0071/bird_textures/dat_0cbb6ffc.inc"
};
static NJS_POINT3 dat_0cbb7014[4] = {
#include "ev0071/bird_textures/dat_0cbb7014.inc"
};
static NJS_VECTOR dat_0cbb7044[4] = {
#include "ev0071/bird_textures/dat_0cbb7044.inc"
};
static NJS_MODEL dat_0cbb7074 = {
#include "ev0071/bird_textures/dat_0cbb7074.inc"
};
static NJS_OBJECT dat_0cbb709c = {
#include "ev0071/bird_textures/dat_0cbb709c.inc"
};
static NJS_MATERIAL dat_0cbb70d0[1] = {
#include "ev0071/bird_textures/dat_0cbb70d0.inc"
};
static Sint16 dat_0cbb70e4[12] = {
#include "ev0071/bird_textures/dat_0cbb70e4.inc"
};
static NJS_COLOR dat_0cbb70fc[9] = {
#include "ev0071/bird_textures/dat_0cbb70fc.inc"
};
static NJS_MESHSET dat_0cbb7120[1] = {
#include "ev0071/bird_textures/dat_0cbb7120.inc"
};
static NJS_POINT3 dat_0cbb7138[7] = {
#include "ev0071/bird_textures/dat_0cbb7138.inc"
};
static NJS_VECTOR dat_0cbb718c[7] = {
#include "ev0071/bird_textures/dat_0cbb718c.inc"
};
static NJS_MODEL dat_0cbb71e0 = {
#include "ev0071/bird_textures/dat_0cbb71e0.inc"
};
static NJS_OBJECT dat_0cbb7208 = {
#include "ev0071/bird_textures/dat_0cbb7208.inc"
};
static NJS_OBJECT dat_0cbb723c = {
#include "ev0071/bird_textures/dat_0cbb723c.inc"
};
static NJS_OBJECT dat_0cbb7270 = {
#include "ev0071/bird_textures/dat_0cbb7270.inc"
};
static NJS_OBJECT dat_0cbb72a4 = {
#include "ev0071/bird_textures/dat_0cbb72a4.inc"
};
static NJS_OBJECT dat_0cbb72d8 = {
#include "ev0071/bird_textures/dat_0cbb72d8.inc"
};
static NJS_OBJECT dat_0cbb730c = {
#include "ev0071/bird_textures/dat_0cbb730c.inc"
};
static NJS_OBJECT dat_0cbb7340 = {
#include "ev0071/bird_textures/dat_0cbb7340.inc"
};
static NJS_MATERIAL dat_0cbb7374[1] = {
#include "ev0071/bird_textures/dat_0cbb7374.inc"
};
static Sint16 dat_0cbb7388[6] = {
#include "ev0071/bird_textures/dat_0cbb7388.inc"
};
static NJS_COLOR dat_0cbb7394[4] = {
#include "ev0071/bird_textures/dat_0cbb7394.inc"
};
static NJS_MESHSET dat_0cbb73a4[1] = {
#include "ev0071/bird_textures/dat_0cbb73a4.inc"
};
static NJS_POINT3 dat_0cbb73bc[4] = {
#include "ev0071/bird_textures/dat_0cbb73bc.inc"
};
static NJS_VECTOR dat_0cbb73ec[4] = {
#include "ev0071/bird_textures/dat_0cbb73ec.inc"
};
static NJS_MODEL dat_0cbb741c = {
#include "ev0071/bird_textures/dat_0cbb741c.inc"
};
static NJS_OBJECT dat_0cbb7444 = {
#include "ev0071/bird_textures/dat_0cbb7444.inc"
};
static NJS_MATERIAL dat_0cbb7478[1] = {
#include "ev0071/bird_textures/dat_0cbb7478.inc"
};
static Sint16 dat_0cbb748c[12] = {
#include "ev0071/bird_textures/dat_0cbb748c.inc"
};
static NJS_COLOR dat_0cbb74a4[9] = {
#include "ev0071/bird_textures/dat_0cbb74a4.inc"
};
static NJS_MESHSET dat_0cbb74c8[1] = {
#include "ev0071/bird_textures/dat_0cbb74c8.inc"
};
static NJS_POINT3 dat_0cbb74e0[7] = {
#include "ev0071/bird_textures/dat_0cbb74e0.inc"
};
static NJS_VECTOR dat_0cbb7534[7] = {
#include "ev0071/bird_textures/dat_0cbb7534.inc"
};
static NJS_MODEL dat_0cbb7588 = {
#include "ev0071/bird_textures/dat_0cbb7588.inc"
};
static NJS_OBJECT dat_0cbb75b0 = {
#include "ev0071/bird_textures/dat_0cbb75b0.inc"
};
static NJS_OBJECT dat_0cbb75e4 = {
#include "ev0071/bird_textures/dat_0cbb75e4.inc"
};
static NJS_OBJECT dat_0cbb7618 = {
#include "ev0071/bird_textures/dat_0cbb7618.inc"
};
static NJS_OBJECT dat_0cbb764c = {
#include "ev0071/bird_textures/dat_0cbb764c.inc"
};
static NJS_OBJECT dat_0cbb7680 = {
#include "ev0071/bird_textures/dat_0cbb7680.inc"
};
static NJS_OBJECT dat_0cbb76b4 = {
#include "ev0071/bird_textures/dat_0cbb76b4.inc"
};
static NJS_OBJECT dat_0cbb76e8 = {
#include "ev0071/bird_textures/dat_0cbb76e8.inc"
};
static NJS_OBJECT dat_0cbb771c = {
#include "ev0071/bird_textures/dat_0cbb771c.inc"
};
static NJS_OBJECT dat_0cbb7750 = {
#include "ev0071/bird_textures/dat_0cbb7750.inc"
};
static NJS_OBJECT dat_0cbb7784 = {
#include "ev0071/bird_textures/dat_0cbb7784.inc"
};
static NJS_OBJECT dat_0cbb77b8 = {
#include "ev0071/bird_textures/dat_0cbb77b8.inc"
};
static NJS_OBJECT dat_0cbb77ec = {
#include "ev0071/bird_textures/dat_0cbb77ec.inc"
};
static NJS_MATERIAL dat_0cbb7820[1] = {
#include "ev0071/bird_textures/dat_0cbb7820.inc"
};
static Sint16 dat_0cbb7834[18] = {
#include "ev0071/bird_textures/dat_0cbb7834.inc"
};
static NJS_COLOR dat_0cbb7858[15] = {
#include "ev0071/bird_textures/dat_0cbb7858.inc"
};
static NJS_MESHSET dat_0cbb7894[1] = {
#include "ev0071/bird_textures/dat_0cbb7894.inc"
};
static NJS_POINT3 dat_0cbb78ac[7] = {
#include "ev0071/bird_textures/dat_0cbb78ac.inc"
};
static NJS_VECTOR dat_0cbb7900[7] = {
#include "ev0071/bird_textures/dat_0cbb7900.inc"
};
static NJS_MODEL dat_0cbb7954 = {
#include "ev0071/bird_textures/dat_0cbb7954.inc"
};
static NJS_OBJECT dat_0cbb797c = {
#include "ev0071/bird_textures/dat_0cbb797c.inc"
};
static NJS_OBJECT dat_0cbb79b0 = {
#include "ev0071/bird_textures/dat_0cbb79b0.inc"
};
static NJS_OBJECT dat_0cbb79e4 = {
#include "ev0071/bird_textures/dat_0cbb79e4.inc"
};
static NJS_OBJECT dat_0cbb7a18 = {
#include "ev0071/bird_textures/dat_0cbb7a18.inc"
};
static NJS_MATERIAL dat_0cbb7a4c[1] = {
#include "ev0071/bird_textures/dat_0cbb7a4c.inc"
};
static Sint16 dat_0cbb7a60[10] = {
#include "ev0071/bird_textures/dat_0cbb7a60.inc"
};
static NJS_COLOR dat_0cbb7a74[8] = {
#include "ev0071/bird_textures/dat_0cbb7a74.inc"
};
static NJS_MESHSET dat_0cbb7a94[1] = {
#include "ev0071/bird_textures/dat_0cbb7a94.inc"
};
static NJS_POINT3 dat_0cbb7aac[8] = {
#include "ev0071/bird_textures/dat_0cbb7aac.inc"
};
static NJS_VECTOR dat_0cbb7b0c[8] = {
#include "ev0071/bird_textures/dat_0cbb7b0c.inc"
};
static NJS_MODEL dat_0cbb7b6c = {
#include "ev0071/bird_textures/dat_0cbb7b6c.inc"
};
static NJS_OBJECT dat_0cbb7b94 = {
#include "ev0071/bird_textures/dat_0cbb7b94.inc"
};
static NJS_MATERIAL dat_0cbb7bc8[1] = {
#include "ev0071/bird_textures/dat_0cbb7bc8.inc"
};
static Sint16 dat_0cbb7bdc[10] = {
#include "ev0071/bird_textures/dat_0cbb7bdc.inc"
};
static NJS_COLOR dat_0cbb7bf0[8] = {
#include "ev0071/bird_textures/dat_0cbb7bf0.inc"
};
static NJS_MESHSET dat_0cbb7c10[1] = {
#include "ev0071/bird_textures/dat_0cbb7c10.inc"
};
static NJS_POINT3 dat_0cbb7c28[8] = {
#include "ev0071/bird_textures/dat_0cbb7c28.inc"
};
static NJS_VECTOR dat_0cbb7c88[8] = {
#include "ev0071/bird_textures/dat_0cbb7c88.inc"
};
static NJS_MODEL dat_0cbb7ce8 = {
#include "ev0071/bird_textures/dat_0cbb7ce8.inc"
};
static NJS_OBJECT dat_0cbb7d10 = {
#include "ev0071/bird_textures/dat_0cbb7d10.inc"
};
static NJS_MATERIAL dat_0cbb7d44[1] = {
#include "ev0071/bird_textures/dat_0cbb7d44.inc"
};
static Sint16 dat_0cbb7d58[16] = {
#include "ev0071/bird_textures/dat_0cbb7d58.inc"
};
static NJS_COLOR dat_0cbb7d78[12] = {
#include "ev0071/bird_textures/dat_0cbb7d78.inc"
};
static NJS_MESHSET dat_0cbb7da8[1] = {
#include "ev0071/bird_textures/dat_0cbb7da8.inc"
};
static NJS_POINT3 dat_0cbb7dc0[12] = {
#include "ev0071/bird_textures/dat_0cbb7dc0.inc"
};
static NJS_VECTOR dat_0cbb7e50[12] = {
#include "ev0071/bird_textures/dat_0cbb7e50.inc"
};
static NJS_MODEL dat_0cbb7ee0 = {
#include "ev0071/bird_textures/dat_0cbb7ee0.inc"
};
static NJS_OBJECT dat_0cbb7f08 = {
#include "ev0071/bird_textures/dat_0cbb7f08.inc"
};
static NJS_MATERIAL dat_0cbb7f3c[1] = {
#include "ev0071/bird_textures/dat_0cbb7f3c.inc"
};
static Sint16 dat_0cbb7f50[16] = {
#include "ev0071/bird_textures/dat_0cbb7f50.inc"
};
static NJS_COLOR dat_0cbb7f70[12] = {
#include "ev0071/bird_textures/dat_0cbb7f70.inc"
};
static NJS_MESHSET dat_0cbb7fa0[1] = {
#include "ev0071/bird_textures/dat_0cbb7fa0.inc"
};
static NJS_POINT3 dat_0cbb7fb8[12] = {
#include "ev0071/bird_textures/dat_0cbb7fb8.inc"
};
static NJS_VECTOR dat_0cbb8048[12] = {
#include "ev0071/bird_textures/dat_0cbb8048.inc"
};
static NJS_MODEL dat_0cbb80d8 = {
#include "ev0071/bird_textures/dat_0cbb80d8.inc"
};
static NJS_OBJECT dat_0cbb8100 = {
#include "ev0071/bird_textures/dat_0cbb8100.inc"
};
static NJS_MATERIAL dat_0cbb8134[4] = {
#include "ev0071/bird_textures/dat_0cbb8134.inc"
};
static Sint16 dat_0cbb8184[239] = {
#include "ev0071/bird_textures/dat_0cbb8184.inc"
};
static Sint16 dat_0cbb8362[22] = {
#include "ev0071/bird_textures/dat_0cbb8362.inc"
};
static Sint16 dat_0cbb838e[21] = {
#include "ev0071/bird_textures/dat_0cbb838e.inc"
};
static Sint16 dat_0cbb83b8[22] = {
#include "ev0071/bird_textures/dat_0cbb83b8.inc"
};
static NJS_COLOR dat_0cbb83e4[209] = {
#include "ev0071/bird_textures/dat_0cbb83e4.inc"
};
static NJS_COLOR dat_0cbb8728[20] = {
#include "ev0071/bird_textures/dat_0cbb8728.inc"
};
static NJS_COLOR dat_0cbb8778[16] = {
#include "ev0071/bird_textures/dat_0cbb8778.inc"
};
static NJS_COLOR dat_0cbb87b8[18] = {
#include "ev0071/bird_textures/dat_0cbb87b8.inc"
};
static NJS_MESHSET dat_0cbb8800[4] = {
#include "ev0071/bird_textures/dat_0cbb8800.inc"
};
static NJS_POINT3 dat_0cbb8860[127] = {
#include "ev0071/bird_textures/dat_0cbb8860.inc"
};
static NJS_VECTOR dat_0cbb8e54[127] = {
#include "ev0071/bird_textures/dat_0cbb8e54.inc"
};
static NJS_MODEL dat_0cbb9448 = {
#include "ev0071/bird_textures/dat_0cbb9448.inc"
};
static NJS_OBJECT dat_0cbb9470 = {
#include "ev0071/bird_textures/dat_0cbb9470.inc"
};
static NJS_OBJECT dat_0cbb94a4 = {
#include "ev0071/bird_textures/dat_0cbb94a4.inc"
};
static NJS_OBJECT dat_0cbb94d8 = {
#include "ev0071/bird_textures/dat_0cbb94d8.inc"
};
static NJS_OBJECT dat_0cbb950c = {
#include "ev0071/bird_textures/dat_0cbb950c.inc"
};
static NJS_MATERIAL dat_0cbb9540[1] = {
#include "ev0071/bird_textures/dat_0cbb9540.inc"
};
static Sint16 dat_0cbb9554[8] = {
#include "ev0071/bird_textures/dat_0cbb9554.inc"
};
static NJS_COLOR dat_0cbb9564[6] = {
#include "ev0071/bird_textures/dat_0cbb9564.inc"
};
static NJS_MESHSET dat_0cbb957c[1] = {
#include "ev0071/bird_textures/dat_0cbb957c.inc"
};
static NJS_POINT3 dat_0cbb9594[6] = {
#include "ev0071/bird_textures/dat_0cbb9594.inc"
};
static NJS_VECTOR dat_0cbb95dc[6] = {
#include "ev0071/bird_textures/dat_0cbb95dc.inc"
};
static NJS_MODEL dat_0cbb9624 = {
#include "ev0071/bird_textures/dat_0cbb9624.inc"
};
static NJS_OBJECT dat_0cbb964c = {
#include "ev0071/bird_textures/dat_0cbb964c.inc"
};
static NJS_MATERIAL dat_0cbb9680[1] = {
#include "ev0071/bird_textures/dat_0cbb9680.inc"
};
static Sint16 dat_0cbb9694[22] = {
#include "ev0071/bird_textures/dat_0cbb9694.inc"
};
static NJS_COLOR dat_0cbb96c0[16] = {
#include "ev0071/bird_textures/dat_0cbb96c0.inc"
};
static NJS_MESHSET dat_0cbb9700[1] = {
#include "ev0071/bird_textures/dat_0cbb9700.inc"
};
static NJS_POINT3 dat_0cbb9718[5] = {
#include "ev0071/bird_textures/dat_0cbb9718.inc"
};
static NJS_VECTOR dat_0cbb9754[5] = {
#include "ev0071/bird_textures/dat_0cbb9754.inc"
};
static NJS_MODEL dat_0cbb9790 = {
#include "ev0071/bird_textures/dat_0cbb9790.inc"
};
static NJS_OBJECT dat_0cbb97b8 = {
#include "ev0071/bird_textures/dat_0cbb97b8.inc"
};
static NJS_MATERIAL dat_0cbb97ec[1] = {
#include "ev0071/bird_textures/dat_0cbb97ec.inc"
};
static Sint16 dat_0cbb9800[30] = {
#include "ev0071/bird_textures/dat_0cbb9800.inc"
};
static NJS_COLOR dat_0cbb983c[28] = {
#include "ev0071/bird_textures/dat_0cbb983c.inc"
};
static NJS_MESHSET dat_0cbb98ac[1] = {
#include "ev0071/bird_textures/dat_0cbb98ac.inc"
};
static NJS_POINT3 dat_0cbb98c4[18] = {
#include "ev0071/bird_textures/dat_0cbb98c4.inc"
};
static NJS_VECTOR dat_0cbb999c[18] = {
#include "ev0071/bird_textures/dat_0cbb999c.inc"
};
static NJS_MODEL dat_0cbb9a74 = {
#include "ev0071/bird_textures/dat_0cbb9a74.inc"
};
static NJS_OBJECT dat_0cbb9a9c = {
#include "ev0071/bird_textures/dat_0cbb9a9c.inc"
};
static NJS_MATERIAL dat_0cbb9ad0[1] = {
#include "ev0071/bird_textures/dat_0cbb9ad0.inc"
};
static Sint16 dat_0cbb9ae4[20] = {
#include "ev0071/bird_textures/dat_0cbb9ae4.inc"
};
static NJS_COLOR dat_0cbb9b0c[16] = {
#include "ev0071/bird_textures/dat_0cbb9b0c.inc"
};
static NJS_MESHSET dat_0cbb9b4c[1] = {
#include "ev0071/bird_textures/dat_0cbb9b4c.inc"
};
static NJS_POINT3 dat_0cbb9b64[12] = {
#include "ev0071/bird_textures/dat_0cbb9b64.inc"
};
static NJS_VECTOR dat_0cbb9bf4[12] = {
#include "ev0071/bird_textures/dat_0cbb9bf4.inc"
};
static NJS_MODEL dat_0cbb9c84 = {
#include "ev0071/bird_textures/dat_0cbb9c84.inc"
};
static NJS_OBJECT dat_0cbb9cac = {
#include "ev0071/bird_textures/dat_0cbb9cac.inc"
};
static NJS_MATERIAL dat_0cbb9ce0[1] = {
#include "ev0071/bird_textures/dat_0cbb9ce0.inc"
};
static Sint16 dat_0cbb9cf4[10] = {
#include "ev0071/bird_textures/dat_0cbb9cf4.inc"
};
static NJS_COLOR dat_0cbb9d08[8] = {
#include "ev0071/bird_textures/dat_0cbb9d08.inc"
};
static NJS_MESHSET dat_0cbb9d28[1] = {
#include "ev0071/bird_textures/dat_0cbb9d28.inc"
};
static NJS_POINT3 dat_0cbb9d40[5] = {
#include "ev0071/bird_textures/dat_0cbb9d40.inc"
};
static NJS_VECTOR dat_0cbb9d7c[5] = {
#include "ev0071/bird_textures/dat_0cbb9d7c.inc"
};
static NJS_MODEL dat_0cbb9db8 = {
#include "ev0071/bird_textures/dat_0cbb9db8.inc"
};
static NJS_OBJECT dat_0cbb9de0 = {
#include "ev0071/bird_textures/dat_0cbb9de0.inc"
};
static NJS_MATERIAL dat_0cbb9e14[1] = {
#include "ev0071/bird_textures/dat_0cbb9e14.inc"
};
static Sint16 dat_0cbb9e28[10] = {
#include "ev0071/bird_textures/dat_0cbb9e28.inc"
};
static NJS_COLOR dat_0cbb9e3c[8] = {
#include "ev0071/bird_textures/dat_0cbb9e3c.inc"
};
static NJS_MESHSET dat_0cbb9e5c[1] = {
#include "ev0071/bird_textures/dat_0cbb9e5c.inc"
};
static NJS_POINT3 dat_0cbb9e74[5] = {
#include "ev0071/bird_textures/dat_0cbb9e74.inc"
};
static NJS_VECTOR dat_0cbb9eb0[5] = {
#include "ev0071/bird_textures/dat_0cbb9eb0.inc"
};
static NJS_MODEL dat_0cbb9eec = {
#include "ev0071/bird_textures/dat_0cbb9eec.inc"
};
static NJS_OBJECT dat_0cbb9f14 = {
#include "ev0071/bird_textures/dat_0cbb9f14.inc"
};
static NJS_MATERIAL dat_0cbb9f48[1] = {
#include "ev0071/bird_textures/dat_0cbb9f48.inc"
};
static Sint16 dat_0cbb9f5c[24] = {
#include "ev0071/bird_textures/dat_0cbb9f5c.inc"
};
static NJS_COLOR dat_0cbb9f8c[20] = {
#include "ev0071/bird_textures/dat_0cbb9f8c.inc"
};
static NJS_MESHSET dat_0cbb9fdc[1] = {
#include "ev0071/bird_textures/dat_0cbb9fdc.inc"
};
static NJS_POINT3 dat_0cbb9ff4[12] = {
#include "ev0071/bird_textures/dat_0cbb9ff4.inc"
};
static NJS_VECTOR dat_0cbba084[12] = {
#include "ev0071/bird_textures/dat_0cbba084.inc"
};
static NJS_MODEL dat_0cbba114 = {
#include "ev0071/bird_textures/dat_0cbba114.inc"
};
static NJS_OBJECT dat_0cbba13c = {
#include "ev0071/bird_textures/dat_0cbba13c.inc"
};
static NJS_MATERIAL dat_0cbba170[1] = {
#include "ev0071/bird_textures/dat_0cbba170.inc"
};
static Sint16 dat_0cbba184[36] = {
#include "ev0071/bird_textures/dat_0cbba184.inc"
};
static NJS_COLOR dat_0cbba1cc[30] = {
#include "ev0071/bird_textures/dat_0cbba1cc.inc"
};
static NJS_MESHSET dat_0cbba244[1] = {
#include "ev0071/bird_textures/dat_0cbba244.inc"
};
static NJS_POINT3 dat_0cbba25c[12] = {
#include "ev0071/bird_textures/dat_0cbba25c.inc"
};
static NJS_VECTOR dat_0cbba2ec[12] = {
#include "ev0071/bird_textures/dat_0cbba2ec.inc"
};
static NJS_MODEL dat_0cbba37c = {
#include "ev0071/bird_textures/dat_0cbba37c.inc"
};
static NJS_OBJECT dat_0cbba3a4 = {
#include "ev0071/bird_textures/dat_0cbba3a4.inc"
};
static NJS_OBJECT dat_0cbba3d8 = {
#include "ev0071/bird_textures/dat_0cbba3d8.inc"
};
static NJS_OBJECT dat_0cbba40c = {
#include "ev0071/bird_textures/dat_0cbba40c.inc"
};
static NJS_OBJECT dat_0cbba440 = {
#include "ev0071/bird_textures/dat_0cbba440.inc"
};
static NJS_MATERIAL dat_0cbba474[1] = {
#include "ev0071/bird_textures/dat_0cbba474.inc"
};
static Sint16 dat_0cbba488[48] = {
#include "ev0071/bird_textures/dat_0cbba488.inc"
};
static NJS_COLOR dat_0cbba4e8[38] = {
#include "ev0071/bird_textures/dat_0cbba4e8.inc"
};
static NJS_MESHSET dat_0cbba580[1] = {
#include "ev0071/bird_textures/dat_0cbba580.inc"
};
static NJS_POINT3 dat_0cbba598[12] = {
#include "ev0071/bird_textures/dat_0cbba598.inc"
};
static NJS_VECTOR dat_0cbba628[12] = {
#include "ev0071/bird_textures/dat_0cbba628.inc"
};
static NJS_MODEL dat_0cbba6b8 = {
#include "ev0071/bird_textures/dat_0cbba6b8.inc"
};
static NJS_OBJECT dat_0cbba6e0 = {
#include "ev0071/bird_textures/dat_0cbba6e0.inc"
};
static NJS_OBJECT dat_0cbba714 = {
#include "ev0071/bird_textures/dat_0cbba714.inc"
};
static NJS_OBJECT dat_0cbba748 = {
#include "ev0071/bird_textures/dat_0cbba748.inc"
};
static NJS_OBJECT dat_0cbba77c = {
#include "ev0071/bird_textures/dat_0cbba77c.inc"
};
static NJS_MATERIAL dat_0cbba7b0[1] = {
#include "ev0071/bird_textures/dat_0cbba7b0.inc"
};
static Sint16 dat_0cbba7c4[48] = {
#include "ev0071/bird_textures/dat_0cbba7c4.inc"
};
static NJS_COLOR dat_0cbba824[38] = {
#include "ev0071/bird_textures/dat_0cbba824.inc"
};
static NJS_MESHSET dat_0cbba8bc[1] = {
#include "ev0071/bird_textures/dat_0cbba8bc.inc"
};
static NJS_POINT3 dat_0cbba8d4[12] = {
#include "ev0071/bird_textures/dat_0cbba8d4.inc"
};
static NJS_VECTOR dat_0cbba964[12] = {
#include "ev0071/bird_textures/dat_0cbba964.inc"
};
static NJS_MODEL dat_0cbba9f4 = {
#include "ev0071/bird_textures/dat_0cbba9f4.inc"
};
static NJS_OBJECT dat_0cbbaa1c = {
#include "ev0071/bird_textures/dat_0cbbaa1c.inc"
};
static NJS_OBJECT dat_0cbbaa50 = {
#include "ev0071/bird_textures/dat_0cbbaa50.inc"
};
static NJS_OBJECT dat_0cbbaa84 = {
#include "ev0071/bird_textures/dat_0cbbaa84.inc"
};
static NJS_OBJECT dat_0cbbaab8 = {
#include "ev0071/bird_textures/dat_0cbbaab8.inc"
};
static NJS_MATERIAL dat_0cbbaaec[2] = {
#include "ev0071/bird_textures/dat_0cbbaaec.inc"
};
static Sint16 dat_0cbbab14[54] = {
#include "ev0071/bird_textures/dat_0cbbab14.inc"
};
static Sint16 dat_0cbbab80[22] = {
#include "ev0071/bird_textures/dat_0cbbab80.inc"
};
static NJS_COLOR dat_0cbbabac[48] = {
#include "ev0071/bird_textures/dat_0cbbabac.inc"
};
static NJS_COLOR dat_0cbbac6c[18] = {
#include "ev0071/bird_textures/dat_0cbbac6c.inc"
};
static NJS_MESHSET dat_0cbbacb4[2] = {
#include "ev0071/bird_textures/dat_0cbbacb4.inc"
};
static NJS_POINT3 dat_0cbbace4[26] = {
#include "ev0071/bird_textures/dat_0cbbace4.inc"
};
static NJS_VECTOR dat_0cbbae1c[26] = {
#include "ev0071/bird_textures/dat_0cbbae1c.inc"
};
static NJS_MODEL dat_0cbbaf54 = {
#include "ev0071/bird_textures/dat_0cbbaf54.inc"
};
static NJS_OBJECT dat_0cbbaf7c = {
#include "ev0071/bird_textures/dat_0cbbaf7c.inc"
};
static NJS_OBJECT dat_0cbbafb0 = {
#include "ev0071/bird_textures/dat_0cbbafb0.inc"
};
static NJS_OBJECT dat_0cbbafe4 = {
#include "ev0071/bird_textures/dat_0cbbafe4.inc"
};
static NJS_OBJECT dat_0cbbb018 = {
#include "ev0071/bird_textures/dat_0cbbb018.inc"
};
static NJS_OBJECT ev0071_bird_object = {
#include "ev0071/bird_textures/ev0071_bird_object.inc"
};
static NJS_MKEY_F dat_0cbbb080[2] = {
#include "ev0071/bird_textures/dat_0cbbb080.inc"
};
static NJS_MKEY_A dat_0cbbb0a0[2] = {
#include "ev0071/bird_textures/dat_0cbbb0a0.inc"
};
static NJS_MKEY_F dat_0cbbb0c0[2] = {
#include "ev0071/bird_textures/dat_0cbbb0c0.inc"
};
static NJS_MKEY_F dat_0cbbb0e0[2] = {
#include "ev0071/bird_textures/dat_0cbbb0e0.inc"
};
static NJS_MKEY_A dat_0cbbb100[2] = {
#include "ev0071/bird_textures/dat_0cbbb100.inc"
};
static NJS_MKEY_A dat_0cbbb120[2] = {
#include "ev0071/bird_textures/dat_0cbbb120.inc"
};
static NJS_MKEY_F dat_0cbbb140[2] = {
#include "ev0071/bird_textures/dat_0cbbb140.inc"
};
static NJS_MKEY_F dat_0cbbb160[2] = {
#include "ev0071/bird_textures/dat_0cbbb160.inc"
};
static NJS_MKEY_A dat_0cbbb180[2] = {
#include "ev0071/bird_textures/dat_0cbbb180.inc"
};
static NJS_MKEY_A dat_0cbbb1a0[2] = {
#include "ev0071/bird_textures/dat_0cbbb1a0.inc"
};
static NJS_MKEY_F dat_0cbbb1c0[2] = {
#include "ev0071/bird_textures/dat_0cbbb1c0.inc"
};
static NJS_MKEY_F dat_0cbbb1e0[2] = {
#include "ev0071/bird_textures/dat_0cbbb1e0.inc"
};
static NJS_MKEY_A dat_0cbbb200[2] = {
#include "ev0071/bird_textures/dat_0cbbb200.inc"
};
static NJS_MKEY_A dat_0cbbb220[2] = {
#include "ev0071/bird_textures/dat_0cbbb220.inc"
};
static NJS_MKEY_F dat_0cbbb240[2] = {
#include "ev0071/bird_textures/dat_0cbbb240.inc"
};
static NJS_MKEY_F dat_0cbbb260[2] = {
#include "ev0071/bird_textures/dat_0cbbb260.inc"
};
static NJS_MKEY_A dat_0cbbb280[2] = {
#include "ev0071/bird_textures/dat_0cbbb280.inc"
};
static NJS_MKEY_F dat_0cbbb2a0[2] = {
#include "ev0071/bird_textures/dat_0cbbb2a0.inc"
};
static NJS_MKEY_A dat_0cbbb2c0[2] = {
#include "ev0071/bird_textures/dat_0cbbb2c0.inc"
};
static NJS_MKEY_F dat_0cbbb2e0[2] = {
#include "ev0071/bird_textures/dat_0cbbb2e0.inc"
};
static NJS_MKEY_A dat_0cbbb300[2] = {
#include "ev0071/bird_textures/dat_0cbbb300.inc"
};
static NJS_MKEY_F dat_0cbbb320[2] = {
#include "ev0071/bird_textures/dat_0cbbb320.inc"
};
static NJS_MKEY_A dat_0cbbb340[2] = {
#include "ev0071/bird_textures/dat_0cbbb340.inc"
};
static NJS_MKEY_A dat_0cbbb360[2] = {
#include "ev0071/bird_textures/dat_0cbbb360.inc"
};
static NJS_MKEY_F dat_0cbbb380[2] = {
#include "ev0071/bird_textures/dat_0cbbb380.inc"
};
static NJS_MKEY_F dat_0cbbb3a0[2] = {
#include "ev0071/bird_textures/dat_0cbbb3a0.inc"
};
static NJS_MKEY_A dat_0cbbb3c0[2] = {
#include "ev0071/bird_textures/dat_0cbbb3c0.inc"
};
static NJS_MKEY_F dat_0cbbb3e0[2] = {
#include "ev0071/bird_textures/dat_0cbbb3e0.inc"
};
static NJS_MKEY_A dat_0cbbb400[2] = {
#include "ev0071/bird_textures/dat_0cbbb400.inc"
};
static NJS_MKEY_F dat_0cbbb420[2] = {
#include "ev0071/bird_textures/dat_0cbbb420.inc"
};
static NJS_MKEY_A dat_0cbbb440[2] = {
#include "ev0071/bird_textures/dat_0cbbb440.inc"
};
static NJS_MKEY_F dat_0cbbb460[2] = {
#include "ev0071/bird_textures/dat_0cbbb460.inc"
};
static NJS_MKEY_A dat_0cbbb480[2] = {
#include "ev0071/bird_textures/dat_0cbbb480.inc"
};
static NJS_MKEY_F dat_0cbbb4a0[2] = {
#include "ev0071/bird_textures/dat_0cbbb4a0.inc"
};
static NJS_MKEY_A dat_0cbbb4c0[2] = {
#include "ev0071/bird_textures/dat_0cbbb4c0.inc"
};
static NJS_MKEY_A dat_0cbbb4e0[2] = {
#include "ev0071/bird_textures/dat_0cbbb4e0.inc"
};
static NJS_MKEY_F dat_0cbbb500[2] = {
#include "ev0071/bird_textures/dat_0cbbb500.inc"
};
static NJS_MKEY_F dat_0cbbb520[2] = {
#include "ev0071/bird_textures/dat_0cbbb520.inc"
};
static NJS_MKEY_A dat_0cbbb540[2] = {
#include "ev0071/bird_textures/dat_0cbbb540.inc"
};
static NJS_MKEY_F dat_0cbbb560[2] = {
#include "ev0071/bird_textures/dat_0cbbb560.inc"
};
static NJS_MKEY_A dat_0cbbb580[2] = {
#include "ev0071/bird_textures/dat_0cbbb580.inc"
};
static NJS_MKEY_F dat_0cbbb5a0[2] = {
#include "ev0071/bird_textures/dat_0cbbb5a0.inc"
};
static NJS_MKEY_A dat_0cbbb5c0[2] = {
#include "ev0071/bird_textures/dat_0cbbb5c0.inc"
};
static NJS_MKEY_F dat_0cbbb5e0[2] = {
#include "ev0071/bird_textures/dat_0cbbb5e0.inc"
};
static NJS_MKEY_A dat_0cbbb600[2] = {
#include "ev0071/bird_textures/dat_0cbbb600.inc"
};
static NJS_MKEY_F dat_0cbbb620[2] = {
#include "ev0071/bird_textures/dat_0cbbb620.inc"
};
static NJS_MKEY_A dat_0cbbb640[2] = {
#include "ev0071/bird_textures/dat_0cbbb640.inc"
};
static NJS_MKEY_F dat_0cbbb660[2] = {
#include "ev0071/bird_textures/dat_0cbbb660.inc"
};
static NJS_MKEY_A dat_0cbbb680[2] = {
#include "ev0071/bird_textures/dat_0cbbb680.inc"
};
static NJS_MKEY_F dat_0cbbb6a0[2] = {
#include "ev0071/bird_textures/dat_0cbbb6a0.inc"
};
static NJS_MKEY_A dat_0cbbb6c0[2] = {
#include "ev0071/bird_textures/dat_0cbbb6c0.inc"
};
static NJS_MKEY_F dat_0cbbb6e0[2] = {
#include "ev0071/bird_textures/dat_0cbbb6e0.inc"
};
static NJS_MKEY_F dat_0cbbb700[2] = {
#include "ev0071/bird_textures/dat_0cbbb700.inc"
};
static NJS_MKEY_F dat_0cbbb720[2] = {
#include "ev0071/bird_textures/dat_0cbbb720.inc"
};
static NJS_MKEY_A dat_0cbbb740[2] = {
#include "ev0071/bird_textures/dat_0cbbb740.inc"
};
static NJS_MKEY_F dat_0cbbb760[2] = {
#include "ev0071/bird_textures/dat_0cbbb760.inc"
};
static NJS_MKEY_A dat_0cbbb780[2] = {
#include "ev0071/bird_textures/dat_0cbbb780.inc"
};
static NJS_MKEY_F dat_0cbbb7a0[2] = {
#include "ev0071/bird_textures/dat_0cbbb7a0.inc"
};
static NJS_MDATA2 dat_0cbbb7c0[57] = {
#include "ev0071/bird_textures/dat_0cbbb7c0.inc"
};
static NJS_MOTION dat_0cbbbb50 = {
#include "ev0071/bird_textures/dat_0cbbbb50.inc"
};
static NJS_ACTION dat_0cbbbb5c = {
#include "ev0071/bird_textures/dat_0cbbbb5c.inc"
};
static NJS_MKEY_F dat_0cbbbb64[32] = {
#include "ev0071/bird_textures/dat_0cbbbb64.inc"
};
static NJS_MKEY_A dat_0cbbbd64[2] = {
#include "ev0071/bird_textures/dat_0cbbbd64.inc"
};
static NJS_MKEY_F dat_0cbbbd84[2] = {
#include "ev0071/bird_textures/dat_0cbbbd84.inc"
};
static NJS_MKEY_F dat_0cbbbda4[2] = {
#include "ev0071/bird_textures/dat_0cbbbda4.inc"
};
static NJS_MKEY_A dat_0cbbbdc4[2] = {
#include "ev0071/bird_textures/dat_0cbbbdc4.inc"
};
static NJS_MKEY_A dat_0cbbbde4[32] = {
#include "ev0071/bird_textures/dat_0cbbbde4.inc"
};
static NJS_MKEY_F dat_0cbbbfe4[2] = {
#include "ev0071/bird_textures/dat_0cbbbfe4.inc"
};
static NJS_MKEY_F dat_0cbbc004[2] = {
#include "ev0071/bird_textures/dat_0cbbc004.inc"
};
static NJS_MKEY_A dat_0cbbc024[2] = {
#include "ev0071/bird_textures/dat_0cbbc024.inc"
};
static NJS_MKEY_A dat_0cbbc044[32] = {
#include "ev0071/bird_textures/dat_0cbbc044.inc"
};
static NJS_MKEY_F dat_0cbbc244[2] = {
#include "ev0071/bird_textures/dat_0cbbc244.inc"
};
static NJS_MKEY_F dat_0cbbc264[2] = {
#include "ev0071/bird_textures/dat_0cbbc264.inc"
};
static NJS_MKEY_A dat_0cbbc284[2] = {
#include "ev0071/bird_textures/dat_0cbbc284.inc"
};
static NJS_MKEY_A dat_0cbbc2a4[2] = {
#include "ev0071/bird_textures/dat_0cbbc2a4.inc"
};
static NJS_MKEY_F dat_0cbbc2c4[2] = {
#include "ev0071/bird_textures/dat_0cbbc2c4.inc"
};
static NJS_MKEY_F dat_0cbbc2e4[2] = {
#include "ev0071/bird_textures/dat_0cbbc2e4.inc"
};
static NJS_MKEY_A dat_0cbbc304[2] = {
#include "ev0071/bird_textures/dat_0cbbc304.inc"
};
static NJS_MKEY_F dat_0cbbc324[2] = {
#include "ev0071/bird_textures/dat_0cbbc324.inc"
};
static NJS_MKEY_A dat_0cbbc344[2] = {
#include "ev0071/bird_textures/dat_0cbbc344.inc"
};
static NJS_MKEY_F dat_0cbbc364[2] = {
#include "ev0071/bird_textures/dat_0cbbc364.inc"
};
static NJS_MKEY_A dat_0cbbc384[2] = {
#include "ev0071/bird_textures/dat_0cbbc384.inc"
};
static NJS_MKEY_F dat_0cbbc3a4[2] = {
#include "ev0071/bird_textures/dat_0cbbc3a4.inc"
};
static NJS_MKEY_A dat_0cbbc3c4[2] = {
#include "ev0071/bird_textures/dat_0cbbc3c4.inc"
};
static NJS_MKEY_A dat_0cbbc3e4[32] = {
#include "ev0071/bird_textures/dat_0cbbc3e4.inc"
};
static NJS_MKEY_F dat_0cbbc5e4[2] = {
#include "ev0071/bird_textures/dat_0cbbc5e4.inc"
};
static NJS_MKEY_F dat_0cbbc604[2] = {
#include "ev0071/bird_textures/dat_0cbbc604.inc"
};
static NJS_MKEY_A dat_0cbbc624[2] = {
#include "ev0071/bird_textures/dat_0cbbc624.inc"
};
static NJS_MKEY_F dat_0cbbc644[2] = {
#include "ev0071/bird_textures/dat_0cbbc644.inc"
};
static NJS_MKEY_A dat_0cbbc664[2] = {
#include "ev0071/bird_textures/dat_0cbbc664.inc"
};
static NJS_MKEY_F dat_0cbbc684[2] = {
#include "ev0071/bird_textures/dat_0cbbc684.inc"
};
static NJS_MKEY_A dat_0cbbc6a4[2] = {
#include "ev0071/bird_textures/dat_0cbbc6a4.inc"
};
static NJS_MKEY_F dat_0cbbc6c4[2] = {
#include "ev0071/bird_textures/dat_0cbbc6c4.inc"
};
static NJS_MKEY_A dat_0cbbc6e4[2] = {
#include "ev0071/bird_textures/dat_0cbbc6e4.inc"
};
static NJS_MKEY_F dat_0cbbc704[2] = {
#include "ev0071/bird_textures/dat_0cbbc704.inc"
};
static NJS_MKEY_A dat_0cbbc724[2] = {
#include "ev0071/bird_textures/dat_0cbbc724.inc"
};
static NJS_MKEY_A dat_0cbbc744[2] = {
#include "ev0071/bird_textures/dat_0cbbc744.inc"
};
static NJS_MKEY_F dat_0cbbc764[2] = {
#include "ev0071/bird_textures/dat_0cbbc764.inc"
};
static NJS_MKEY_F dat_0cbbc784[32] = {
#include "ev0071/bird_textures/dat_0cbbc784.inc"
};
static NJS_MKEY_A dat_0cbbc984[2] = {
#include "ev0071/bird_textures/dat_0cbbc984.inc"
};
static NJS_MKEY_F dat_0cbbc9a4[32] = {
#include "ev0071/bird_textures/dat_0cbbc9a4.inc"
};
static NJS_MKEY_A dat_0cbbcba4[2] = {
#include "ev0071/bird_textures/dat_0cbbcba4.inc"
};
static NJS_MKEY_F dat_0cbbcbc4[2] = {
#include "ev0071/bird_textures/dat_0cbbcbc4.inc"
};
static NJS_MKEY_A dat_0cbbcbe4[2] = {
#include "ev0071/bird_textures/dat_0cbbcbe4.inc"
};
static NJS_MKEY_F dat_0cbbcc04[32] = {
#include "ev0071/bird_textures/dat_0cbbcc04.inc"
};
static NJS_MKEY_A dat_0cbbce04[2] = {
#include "ev0071/bird_textures/dat_0cbbce04.inc"
};
static NJS_MKEY_F dat_0cbbce24[2] = {
#include "ev0071/bird_textures/dat_0cbbce24.inc"
};
static NJS_MKEY_A dat_0cbbce44[2] = {
#include "ev0071/bird_textures/dat_0cbbce44.inc"
};
static NJS_MKEY_F dat_0cbbce64[2] = {
#include "ev0071/bird_textures/dat_0cbbce64.inc"
};
static NJS_MKEY_A dat_0cbbce84[2] = {
#include "ev0071/bird_textures/dat_0cbbce84.inc"
};
static NJS_MKEY_F dat_0cbbcea4[2] = {
#include "ev0071/bird_textures/dat_0cbbcea4.inc"
};
static NJS_MKEY_A dat_0cbbcec4[2] = {
#include "ev0071/bird_textures/dat_0cbbcec4.inc"
};
static NJS_MKEY_F dat_0cbbcee4[2] = {
#include "ev0071/bird_textures/dat_0cbbcee4.inc"
};
static NJS_MKEY_F dat_0cbbcf04[2] = {
#include "ev0071/bird_textures/dat_0cbbcf04.inc"
};
static NJS_MKEY_F dat_0cbbcf24[2] = {
#include "ev0071/bird_textures/dat_0cbbcf24.inc"
};
static NJS_MKEY_A dat_0cbbcf44[2] = {
#include "ev0071/bird_textures/dat_0cbbcf44.inc"
};
static NJS_MKEY_F dat_0cbbcf64[2] = {
#include "ev0071/bird_textures/dat_0cbbcf64.inc"
};
static NJS_MKEY_A dat_0cbbcf84[2] = {
#include "ev0071/bird_textures/dat_0cbbcf84.inc"
};
static NJS_MKEY_F dat_0cbbcfa4[2] = {
#include "ev0071/bird_textures/dat_0cbbcfa4.inc"
};
static NJS_MDATA2 dat_0cbbcfc4[57] = {
#include "ev0071/bird_textures/dat_0cbbcfc4.inc"
};
static NJS_MOTION dat_0cbbd354 = {
#include "ev0071/bird_textures/dat_0cbbd354.inc"
};
static NJS_ACTION dat_0cbbd360 = {
#include "ev0071/bird_textures/dat_0cbbd360.inc"
};
static NJS_MKEY_F dat_0cbbd368[2] = {
#include "ev0071/bird_textures/dat_0cbbd368.inc"
};
static NJS_MKEY_F dat_0cbbd388[30] = {
#include "ev0071/bird_textures/dat_0cbbd388.inc"
};
static NJS_MKEY_F dat_0cbbd568[2] = {
#include "ev0071/bird_textures/dat_0cbbd568.inc"
};
static NJS_MKEY_A dat_0cbbd588[2] = {
#include "ev0071/bird_textures/dat_0cbbd588.inc"
};
static NJS_MKEY_F dat_0cbbd5a8[2] = {
#include "ev0071/bird_textures/dat_0cbbd5a8.inc"
};
static NJS_MKEY_F dat_0cbbd5c8[2] = {
#include "ev0071/bird_textures/dat_0cbbd5c8.inc"
};
static NJS_MKEY_F dat_0cbbd5e8[2] = {
#include "ev0071/bird_textures/dat_0cbbd5e8.inc"
};
static NJS_MKEY_F dat_0cbbd608[2] = {
#include "ev0071/bird_textures/dat_0cbbd608.inc"
};
static NJS_MKEY_F dat_0cbbd628[2] = {
#include "ev0071/bird_textures/dat_0cbbd628.inc"
};
static NJS_MKEY_A dat_0cbbd648[2] = {
#include "ev0071/bird_textures/dat_0cbbd648.inc"
};
static NJS_MKEY_F dat_0cbbd668[2] = {
#include "ev0071/bird_textures/dat_0cbbd668.inc"
};
static NJS_MKEY_A dat_0cbbd688[30] = {
#include "ev0071/bird_textures/dat_0cbbd688.inc"
};
static NJS_MKEY_F dat_0cbbd868[2] = {
#include "ev0071/bird_textures/dat_0cbbd868.inc"
};
static NJS_MKEY_F dat_0cbbd888[2] = {
#include "ev0071/bird_textures/dat_0cbbd888.inc"
};
static NJS_MKEY_F dat_0cbbd8a8[2] = {
#include "ev0071/bird_textures/dat_0cbbd8a8.inc"
};
static NJS_MKEY_F dat_0cbbd8c8[2] = {
#include "ev0071/bird_textures/dat_0cbbd8c8.inc"
};
static NJS_MKEY_F dat_0cbbd8e8[2] = {
#include "ev0071/bird_textures/dat_0cbbd8e8.inc"
};
static NJS_MKEY_A dat_0cbbd908[2] = {
#include "ev0071/bird_textures/dat_0cbbd908.inc"
};
static NJS_MKEY_F dat_0cbbd928[2] = {
#include "ev0071/bird_textures/dat_0cbbd928.inc"
};
static NJS_MKEY_A dat_0cbbd948[30] = {
#include "ev0071/bird_textures/dat_0cbbd948.inc"
};
static NJS_MKEY_F dat_0cbbdb28[2] = {
#include "ev0071/bird_textures/dat_0cbbdb28.inc"
};
static NJS_MKEY_F dat_0cbbdb48[2] = {
#include "ev0071/bird_textures/dat_0cbbdb48.inc"
};
static NJS_MKEY_F dat_0cbbdb68[2] = {
#include "ev0071/bird_textures/dat_0cbbdb68.inc"
};
static NJS_MKEY_F dat_0cbbdb88[2] = {
#include "ev0071/bird_textures/dat_0cbbdb88.inc"
};
static NJS_MKEY_F dat_0cbbdba8[2] = {
#include "ev0071/bird_textures/dat_0cbbdba8.inc"
};
static NJS_MKEY_A dat_0cbbdbc8[2] = {
#include "ev0071/bird_textures/dat_0cbbdbc8.inc"
};
static NJS_MKEY_F dat_0cbbdbe8[2] = {
#include "ev0071/bird_textures/dat_0cbbdbe8.inc"
};
static NJS_MKEY_A dat_0cbbdc08[2] = {
#include "ev0071/bird_textures/dat_0cbbdc08.inc"
};
static NJS_MKEY_F dat_0cbbdc28[2] = {
#include "ev0071/bird_textures/dat_0cbbdc28.inc"
};
static NJS_MKEY_F dat_0cbbdc48[2] = {
#include "ev0071/bird_textures/dat_0cbbdc48.inc"
};
static NJS_MKEY_F dat_0cbbdc68[2] = {
#include "ev0071/bird_textures/dat_0cbbdc68.inc"
};
static NJS_MKEY_F dat_0cbbdc88[2] = {
#include "ev0071/bird_textures/dat_0cbbdc88.inc"
};
static NJS_MKEY_F dat_0cbbdca8[2] = {
#include "ev0071/bird_textures/dat_0cbbdca8.inc"
};
static NJS_MKEY_F dat_0cbbdcc8[2] = {
#include "ev0071/bird_textures/dat_0cbbdcc8.inc"
};
static NJS_MKEY_F dat_0cbbdce8[2] = {
#include "ev0071/bird_textures/dat_0cbbdce8.inc"
};
static NJS_MKEY_F dat_0cbbdd08[2] = {
#include "ev0071/bird_textures/dat_0cbbdd08.inc"
};
static NJS_MKEY_A dat_0cbbdd28[2] = {
#include "ev0071/bird_textures/dat_0cbbdd28.inc"
};
static NJS_MKEY_F dat_0cbbdd48[2] = {
#include "ev0071/bird_textures/dat_0cbbdd48.inc"
};
static NJS_MKEY_F dat_0cbbdd68[2] = {
#include "ev0071/bird_textures/dat_0cbbdd68.inc"
};
static NJS_MKEY_A dat_0cbbdd88[2] = {
#include "ev0071/bird_textures/dat_0cbbdd88.inc"
};
static NJS_MKEY_F dat_0cbbdda8[2] = {
#include "ev0071/bird_textures/dat_0cbbdda8.inc"
};
static NJS_MKEY_A dat_0cbbddc8[2] = {
#include "ev0071/bird_textures/dat_0cbbddc8.inc"
};
static NJS_MKEY_F dat_0cbbdde8[2] = {
#include "ev0071/bird_textures/dat_0cbbdde8.inc"
};
static NJS_MKEY_A dat_0cbbde08[2] = {
#include "ev0071/bird_textures/dat_0cbbde08.inc"
};
static NJS_MKEY_F dat_0cbbde28[2] = {
#include "ev0071/bird_textures/dat_0cbbde28.inc"
};
static NJS_MKEY_A dat_0cbbde48[2] = {
#include "ev0071/bird_textures/dat_0cbbde48.inc"
};
static NJS_MKEY_F dat_0cbbde68[2] = {
#include "ev0071/bird_textures/dat_0cbbde68.inc"
};
static NJS_MKEY_F dat_0cbbde88[2] = {
#include "ev0071/bird_textures/dat_0cbbde88.inc"
};
static NJS_MKEY_F dat_0cbbdea8[2] = {
#include "ev0071/bird_textures/dat_0cbbdea8.inc"
};
static NJS_MKEY_F dat_0cbbdec8[2] = {
#include "ev0071/bird_textures/dat_0cbbdec8.inc"
};
static NJS_MKEY_F dat_0cbbdee8[2] = {
#include "ev0071/bird_textures/dat_0cbbdee8.inc"
};
static NJS_MKEY_A dat_0cbbdf08[2] = {
#include "ev0071/bird_textures/dat_0cbbdf08.inc"
};
static NJS_MKEY_F dat_0cbbdf28[2] = {
#include "ev0071/bird_textures/dat_0cbbdf28.inc"
};
static NJS_MKEY_F dat_0cbbdf48[2] = {
#include "ev0071/bird_textures/dat_0cbbdf48.inc"
};
static NJS_MKEY_A dat_0cbbdf68[2] = {
#include "ev0071/bird_textures/dat_0cbbdf68.inc"
};
static NJS_MKEY_F dat_0cbbdf88[2] = {
#include "ev0071/bird_textures/dat_0cbbdf88.inc"
};
static NJS_MKEY_F dat_0cbbdfa8[2] = {
#include "ev0071/bird_textures/dat_0cbbdfa8.inc"
};
static NJS_MKEY_A dat_0cbbdfc8[2] = {
#include "ev0071/bird_textures/dat_0cbbdfc8.inc"
};
static NJS_MKEY_F dat_0cbbdfe8[2] = {
#include "ev0071/bird_textures/dat_0cbbdfe8.inc"
};
static NJS_MKEY_F dat_0cbbe008[2] = {
#include "ev0071/bird_textures/dat_0cbbe008.inc"
};
static NJS_MKEY_A dat_0cbbe028[2] = {
#include "ev0071/bird_textures/dat_0cbbe028.inc"
};
static NJS_MKEY_F dat_0cbbe048[2] = {
#include "ev0071/bird_textures/dat_0cbbe048.inc"
};
static NJS_MKEY_F dat_0cbbe068[2] = {
#include "ev0071/bird_textures/dat_0cbbe068.inc"
};
static NJS_MKEY_A dat_0cbbe088[2] = {
#include "ev0071/bird_textures/dat_0cbbe088.inc"
};
static NJS_MKEY_F dat_0cbbe0a8[2] = {
#include "ev0071/bird_textures/dat_0cbbe0a8.inc"
};
static NJS_MKEY_A dat_0cbbe0c8[30] = {
#include "ev0071/bird_textures/dat_0cbbe0c8.inc"
};
static NJS_MKEY_F dat_0cbbe2a8[2] = {
#include "ev0071/bird_textures/dat_0cbbe2a8.inc"
};
static NJS_MKEY_F dat_0cbbe2c8[2] = {
#include "ev0071/bird_textures/dat_0cbbe2c8.inc"
};
static NJS_MKEY_F dat_0cbbe2e8[2] = {
#include "ev0071/bird_textures/dat_0cbbe2e8.inc"
};
static NJS_MKEY_F dat_0cbbe308[2] = {
#include "ev0071/bird_textures/dat_0cbbe308.inc"
};
static NJS_MKEY_F dat_0cbbe328[30] = {
#include "ev0071/bird_textures/dat_0cbbe328.inc"
};
static NJS_MKEY_A dat_0cbbe508[2] = {
#include "ev0071/bird_textures/dat_0cbbe508.inc"
};
static NJS_MKEY_F dat_0cbbe528[2] = {
#include "ev0071/bird_textures/dat_0cbbe528.inc"
};
static NJS_MKEY_F dat_0cbbe548[30] = {
#include "ev0071/bird_textures/dat_0cbbe548.inc"
};
static NJS_MKEY_A dat_0cbbe728[2] = {
#include "ev0071/bird_textures/dat_0cbbe728.inc"
};
static NJS_MKEY_F dat_0cbbe748[2] = {
#include "ev0071/bird_textures/dat_0cbbe748.inc"
};
static NJS_MKEY_F dat_0cbbe768[2] = {
#include "ev0071/bird_textures/dat_0cbbe768.inc"
};
static NJS_MKEY_A dat_0cbbe788[2] = {
#include "ev0071/bird_textures/dat_0cbbe788.inc"
};
static NJS_MKEY_F dat_0cbbe7a8[2] = {
#include "ev0071/bird_textures/dat_0cbbe7a8.inc"
};
static NJS_MKEY_F dat_0cbbe7c8[2] = {
#include "ev0071/bird_textures/dat_0cbbe7c8.inc"
};
static NJS_MKEY_A dat_0cbbe7e8[2] = {
#include "ev0071/bird_textures/dat_0cbbe7e8.inc"
};
static NJS_MKEY_F dat_0cbbe808[2] = {
#include "ev0071/bird_textures/dat_0cbbe808.inc"
};
static NJS_MKEY_F dat_0cbbe828[30] = {
#include "ev0071/bird_textures/dat_0cbbe828.inc"
};
static NJS_MKEY_A dat_0cbbea08[2] = {
#include "ev0071/bird_textures/dat_0cbbea08.inc"
};
static NJS_MKEY_F dat_0cbbea28[2] = {
#include "ev0071/bird_textures/dat_0cbbea28.inc"
};
static NJS_MKEY_F dat_0cbbea48[2] = {
#include "ev0071/bird_textures/dat_0cbbea48.inc"
};
static NJS_MKEY_A dat_0cbbea68[2] = {
#include "ev0071/bird_textures/dat_0cbbea68.inc"
};
static NJS_MKEY_F dat_0cbbea88[2] = {
#include "ev0071/bird_textures/dat_0cbbea88.inc"
};
static NJS_MKEY_A dat_0cbbeaa8[30] = {
#include "ev0071/bird_textures/dat_0cbbeaa8.inc"
};
static NJS_MKEY_F dat_0cbbec88[2] = {
#include "ev0071/bird_textures/dat_0cbbec88.inc"
};
static NJS_MKEY_F dat_0cbbeca8[2] = {
#include "ev0071/bird_textures/dat_0cbbeca8.inc"
};
static NJS_MKEY_F dat_0cbbecc8[2] = {
#include "ev0071/bird_textures/dat_0cbbecc8.inc"
};
static NJS_MKEY_A dat_0cbbece8[2] = {
#include "ev0071/bird_textures/dat_0cbbece8.inc"
};
static NJS_MKEY_F dat_0cbbed08[2] = {
#include "ev0071/bird_textures/dat_0cbbed08.inc"
};
static NJS_MKEY_A dat_0cbbed28[30] = {
#include "ev0071/bird_textures/dat_0cbbed28.inc"
};
static NJS_MKEY_F dat_0cbbef08[2] = {
#include "ev0071/bird_textures/dat_0cbbef08.inc"
};
static NJS_MKEY_F dat_0cbbef28[2] = {
#include "ev0071/bird_textures/dat_0cbbef28.inc"
};
static NJS_MKEY_F dat_0cbbef48[2] = {
#include "ev0071/bird_textures/dat_0cbbef48.inc"
};
static NJS_MKEY_F dat_0cbbef68[2] = {
#include "ev0071/bird_textures/dat_0cbbef68.inc"
};
static NJS_MKEY_F dat_0cbbef88[2] = {
#include "ev0071/bird_textures/dat_0cbbef88.inc"
};
static NJS_MKEY_F dat_0cbbefa8[2] = {
#include "ev0071/bird_textures/dat_0cbbefa8.inc"
};
static NJS_MKEY_F dat_0cbbefc8[2] = {
#include "ev0071/bird_textures/dat_0cbbefc8.inc"
};
static NJS_MKEY_A dat_0cbbefe8[2] = {
#include "ev0071/bird_textures/dat_0cbbefe8.inc"
};
static NJS_MKEY_F dat_0cbbf008[2] = {
#include "ev0071/bird_textures/dat_0cbbf008.inc"
};
static NJS_MKEY_A dat_0cbbf028[30] = {
#include "ev0071/bird_textures/dat_0cbbf028.inc"
};
static NJS_MKEY_F dat_0cbbf208[2] = {
#include "ev0071/bird_textures/dat_0cbbf208.inc"
};
static NJS_MKEY_F dat_0cbbf228[2] = {
#include "ev0071/bird_textures/dat_0cbbf228.inc"
};
static NJS_MKEY_F dat_0cbbf248[2] = {
#include "ev0071/bird_textures/dat_0cbbf248.inc"
};
static NJS_MKEY_A dat_0cbbf268[2] = {
#include "ev0071/bird_textures/dat_0cbbf268.inc"
};
static NJS_MKEY_F dat_0cbbf288[2] = {
#include "ev0071/bird_textures/dat_0cbbf288.inc"
};
static NJS_MKEY_A dat_0cbbf2a8[30] = {
#include "ev0071/bird_textures/dat_0cbbf2a8.inc"
};
static NJS_MKEY_F dat_0cbbf488[2] = {
#include "ev0071/bird_textures/dat_0cbbf488.inc"
};
static NJS_MKEY_F dat_0cbbf4a8[2] = {
#include "ev0071/bird_textures/dat_0cbbf4a8.inc"
};
static NJS_MKEY_F dat_0cbbf4c8[2] = {
#include "ev0071/bird_textures/dat_0cbbf4c8.inc"
};
static NJS_MKEY_F dat_0cbbf4e8[2] = {
#include "ev0071/bird_textures/dat_0cbbf4e8.inc"
};
static NJS_MKEY_F dat_0cbbf508[2] = {
#include "ev0071/bird_textures/dat_0cbbf508.inc"
};
static NJS_MDATA3 ev0071_bird_motion_0_mdata[57] = {
#include "ev0071/bird_textures/ev0071_bird_motion_0_mdata.inc"
};
static NJS_MOTION ev0071_bird_motion_0 = {
#include "ev0071/bird_textures/ev0071_bird_motion_0.inc"
};
NJS_ACTION ev0071_bird_action_0 = {
#include "ev0071/bird_textures/ev0071_bird_action_0.inc"
};
static NJS_MKEY_F dat_0cbbfa94[50] = {
#include "ev0071/bird_textures/dat_0cbbfa94.inc"
};
static NJS_MKEY_A dat_0cbbfdb4[50] = {
#include "ev0071/bird_textures/dat_0cbbfdb4.inc"
};
static NJS_MKEY_A dat_0cbc00d4[2] = {
#include "ev0071/bird_textures/dat_0cbc00d4.inc"
};
static NJS_MKEY_A dat_0cbc00f4[2] = {
#include "ev0071/bird_textures/dat_0cbc00f4.inc"
};
static NJS_MKEY_A dat_0cbc0114[2] = {
#include "ev0071/bird_textures/dat_0cbc0114.inc"
};
static NJS_MKEY_A dat_0cbc0134[2] = {
#include "ev0071/bird_textures/dat_0cbc0134.inc"
};
static NJS_MKEY_A dat_0cbc0154[2] = {
#include "ev0071/bird_textures/dat_0cbc0154.inc"
};
static NJS_MKEY_A dat_0cbc0174[50] = {
#include "ev0071/bird_textures/dat_0cbc0174.inc"
};
static NJS_MKEY_A dat_0cbc0494[2] = {
#include "ev0071/bird_textures/dat_0cbc0494.inc"
};
static NJS_MKEY_A dat_0cbc04b4[2] = {
#include "ev0071/bird_textures/dat_0cbc04b4.inc"
};
static NJS_MKEY_A dat_0cbc04d4[2] = {
#include "ev0071/bird_textures/dat_0cbc04d4.inc"
};
static NJS_MKEY_A dat_0cbc04f4[50] = {
#include "ev0071/bird_textures/dat_0cbc04f4.inc"
};
static NJS_MKEY_A dat_0cbc0814[2] = {
#include "ev0071/bird_textures/dat_0cbc0814.inc"
};
static NJS_MKEY_A dat_0cbc0834[2] = {
#include "ev0071/bird_textures/dat_0cbc0834.inc"
};
static NJS_MKEY_A dat_0cbc0854[2] = {
#include "ev0071/bird_textures/dat_0cbc0854.inc"
};
static NJS_MKEY_A dat_0cbc0874[50] = {
#include "ev0071/bird_textures/dat_0cbc0874.inc"
};
static NJS_MKEY_A dat_0cbc0b94[2] = {
#include "ev0071/bird_textures/dat_0cbc0b94.inc"
};
static NJS_MKEY_A dat_0cbc0bb4[2] = {
#include "ev0071/bird_textures/dat_0cbc0bb4.inc"
};
static NJS_MKEY_A dat_0cbc0bd4[2] = {
#include "ev0071/bird_textures/dat_0cbc0bd4.inc"
};
static NJS_MKEY_A dat_0cbc0bf4[2] = {
#include "ev0071/bird_textures/dat_0cbc0bf4.inc"
};
static NJS_MKEY_A dat_0cbc0c14[2] = {
#include "ev0071/bird_textures/dat_0cbc0c14.inc"
};
static NJS_MKEY_A dat_0cbc0c34[2] = {
#include "ev0071/bird_textures/dat_0cbc0c34.inc"
};
static NJS_MKEY_A dat_0cbc0c54[2] = {
#include "ev0071/bird_textures/dat_0cbc0c54.inc"
};
static NJS_MKEY_A dat_0cbc0c74[2] = {
#include "ev0071/bird_textures/dat_0cbc0c74.inc"
};
static NJS_MKEY_A dat_0cbc0c94[2] = {
#include "ev0071/bird_textures/dat_0cbc0c94.inc"
};
static NJS_MKEY_A dat_0cbc0cb4[2] = {
#include "ev0071/bird_textures/dat_0cbc0cb4.inc"
};
static NJS_MKEY_A dat_0cbc0cd4[50] = {
#include "ev0071/bird_textures/dat_0cbc0cd4.inc"
};
static NJS_MKEY_A dat_0cbc0ff4[2] = {
#include "ev0071/bird_textures/dat_0cbc0ff4.inc"
};
static NJS_MKEY_A dat_0cbc1014[2] = {
#include "ev0071/bird_textures/dat_0cbc1014.inc"
};
static NJS_MKEY_A dat_0cbc1034[2] = {
#include "ev0071/bird_textures/dat_0cbc1034.inc"
};
static NJS_MKEY_A dat_0cbc1054[2] = {
#include "ev0071/bird_textures/dat_0cbc1054.inc"
};
static NJS_MKEY_A dat_0cbc1074[2] = {
#include "ev0071/bird_textures/dat_0cbc1074.inc"
};
static NJS_MKEY_A dat_0cbc1094[2] = {
#include "ev0071/bird_textures/dat_0cbc1094.inc"
};
static NJS_MKEY_A dat_0cbc10b4[2] = {
#include "ev0071/bird_textures/dat_0cbc10b4.inc"
};
static NJS_MKEY_A dat_0cbc10d4[50] = {
#include "ev0071/bird_textures/dat_0cbc10d4.inc"
};
static NJS_MKEY_A dat_0cbc13f4[2] = {
#include "ev0071/bird_textures/dat_0cbc13f4.inc"
};
static NJS_MKEY_A dat_0cbc1414[2] = {
#include "ev0071/bird_textures/dat_0cbc1414.inc"
};
static NJS_MKEY_A dat_0cbc1434[2] = {
#include "ev0071/bird_textures/dat_0cbc1434.inc"
};
static NJS_MKEY_A dat_0cbc1454[2] = {
#include "ev0071/bird_textures/dat_0cbc1454.inc"
};
static NJS_MKEY_A dat_0cbc1474[2] = {
#include "ev0071/bird_textures/dat_0cbc1474.inc"
};
static NJS_MKEY_A dat_0cbc1494[2] = {
#include "ev0071/bird_textures/dat_0cbc1494.inc"
};
static NJS_MKEY_A dat_0cbc14b4[2] = {
#include "ev0071/bird_textures/dat_0cbc14b4.inc"
};
static NJS_MKEY_A dat_0cbc14d4[2] = {
#include "ev0071/bird_textures/dat_0cbc14d4.inc"
};
static NJS_MKEY_A dat_0cbc14f4[50] = {
#include "ev0071/bird_textures/dat_0cbc14f4.inc"
};
static NJS_MKEY_A dat_0cbc1814[2] = {
#include "ev0071/bird_textures/dat_0cbc1814.inc"
};
static NJS_MKEY_A dat_0cbc1834[2] = {
#include "ev0071/bird_textures/dat_0cbc1834.inc"
};
static NJS_MKEY_A dat_0cbc1854[50] = {
#include "ev0071/bird_textures/dat_0cbc1854.inc"
};
static NJS_MKEY_A dat_0cbc1b74[2] = {
#include "ev0071/bird_textures/dat_0cbc1b74.inc"
};
static NJS_MKEY_A dat_0cbc1b94[2] = {
#include "ev0071/bird_textures/dat_0cbc1b94.inc"
};
static NJS_MKEY_A dat_0cbc1bb4[2] = {
#include "ev0071/bird_textures/dat_0cbc1bb4.inc"
};
static NJS_MKEY_A dat_0cbc1bd4[2] = {
#include "ev0071/bird_textures/dat_0cbc1bd4.inc"
};
static NJS_MKEY_A dat_0cbc1bf4[50] = {
#include "ev0071/bird_textures/dat_0cbc1bf4.inc"
};
static NJS_MKEY_A dat_0cbc1f14[2] = {
#include "ev0071/bird_textures/dat_0cbc1f14.inc"
};
static NJS_MKEY_A dat_0cbc1f34[2] = {
#include "ev0071/bird_textures/dat_0cbc1f34.inc"
};
static NJS_MKEY_A dat_0cbc1f54[50] = {
#include "ev0071/bird_textures/dat_0cbc1f54.inc"
};
static NJS_MKEY_A dat_0cbc2274[2] = {
#include "ev0071/bird_textures/dat_0cbc2274.inc"
};
static NJS_MKEY_A dat_0cbc2294[2] = {
#include "ev0071/bird_textures/dat_0cbc2294.inc"
};
static NJS_MKEY_A dat_0cbc22b4[2] = {
#include "ev0071/bird_textures/dat_0cbc22b4.inc"
};
static NJS_MDATA2 ev0071_bird_motion_1_mdata[57] = {
#include "ev0071/bird_textures/ev0071_bird_motion_1_mdata.inc"
};
static NJS_MOTION ev0071_bird_motion_1 = {
#include "ev0071/bird_textures/ev0071_bird_motion_1.inc"
};
NJS_ACTION ev0071_bird_action_1 = {
#include "ev0071/bird_textures/ev0071_bird_action_1.inc"
};
static NJS_MKEY_F dat_0cbc2678[2] = {
#include "ev0071/bird_textures/dat_0cbc2678.inc"
};
static NJS_MKEY_A dat_0cbc2698[30] = {
#include "ev0071/bird_textures/dat_0cbc2698.inc"
};
static NJS_MKEY_A dat_0cbc2878[2] = {
#include "ev0071/bird_textures/dat_0cbc2878.inc"
};
static NJS_MKEY_A dat_0cbc2898[2] = {
#include "ev0071/bird_textures/dat_0cbc2898.inc"
};
static NJS_MKEY_A dat_0cbc28b8[2] = {
#include "ev0071/bird_textures/dat_0cbc28b8.inc"
};
static NJS_MKEY_A dat_0cbc28d8[2] = {
#include "ev0071/bird_textures/dat_0cbc28d8.inc"
};
static NJS_MKEY_A dat_0cbc28f8[2] = {
#include "ev0071/bird_textures/dat_0cbc28f8.inc"
};
static NJS_MKEY_A dat_0cbc2918[2] = {
#include "ev0071/bird_textures/dat_0cbc2918.inc"
};
static NJS_MKEY_A dat_0cbc2938[2] = {
#include "ev0071/bird_textures/dat_0cbc2938.inc"
};
static NJS_MKEY_A dat_0cbc2958[2] = {
#include "ev0071/bird_textures/dat_0cbc2958.inc"
};
static NJS_MKEY_A dat_0cbc2978[2] = {
#include "ev0071/bird_textures/dat_0cbc2978.inc"
};
static NJS_MKEY_A dat_0cbc2998[2] = {
#include "ev0071/bird_textures/dat_0cbc2998.inc"
};
static NJS_MKEY_A dat_0cbc29b8[2] = {
#include "ev0071/bird_textures/dat_0cbc29b8.inc"
};
static NJS_MKEY_A dat_0cbc29d8[2] = {
#include "ev0071/bird_textures/dat_0cbc29d8.inc"
};
static NJS_MKEY_A dat_0cbc29f8[2] = {
#include "ev0071/bird_textures/dat_0cbc29f8.inc"
};
static NJS_MKEY_A dat_0cbc2a18[2] = {
#include "ev0071/bird_textures/dat_0cbc2a18.inc"
};
static NJS_MKEY_A dat_0cbc2a38[2] = {
#include "ev0071/bird_textures/dat_0cbc2a38.inc"
};
static NJS_MKEY_A dat_0cbc2a58[2] = {
#include "ev0071/bird_textures/dat_0cbc2a58.inc"
};
static NJS_MKEY_A dat_0cbc2a78[2] = {
#include "ev0071/bird_textures/dat_0cbc2a78.inc"
};
static NJS_MKEY_A dat_0cbc2a98[2] = {
#include "ev0071/bird_textures/dat_0cbc2a98.inc"
};
static NJS_MKEY_A dat_0cbc2ab8[2] = {
#include "ev0071/bird_textures/dat_0cbc2ab8.inc"
};
static NJS_MKEY_A dat_0cbc2ad8[2] = {
#include "ev0071/bird_textures/dat_0cbc2ad8.inc"
};
static NJS_MKEY_A dat_0cbc2af8[2] = {
#include "ev0071/bird_textures/dat_0cbc2af8.inc"
};
static NJS_MKEY_A dat_0cbc2b18[2] = {
#include "ev0071/bird_textures/dat_0cbc2b18.inc"
};
static NJS_MKEY_A dat_0cbc2b38[2] = {
#include "ev0071/bird_textures/dat_0cbc2b38.inc"
};
static NJS_MKEY_A dat_0cbc2b58[2] = {
#include "ev0071/bird_textures/dat_0cbc2b58.inc"
};
static NJS_MKEY_A dat_0cbc2b78[2] = {
#include "ev0071/bird_textures/dat_0cbc2b78.inc"
};
static NJS_MKEY_A dat_0cbc2b98[2] = {
#include "ev0071/bird_textures/dat_0cbc2b98.inc"
};
static NJS_MKEY_A dat_0cbc2bb8[2] = {
#include "ev0071/bird_textures/dat_0cbc2bb8.inc"
};
static NJS_MKEY_A dat_0cbc2bd8[2] = {
#include "ev0071/bird_textures/dat_0cbc2bd8.inc"
};
static NJS_MKEY_A dat_0cbc2bf8[2] = {
#include "ev0071/bird_textures/dat_0cbc2bf8.inc"
};
static NJS_MKEY_A dat_0cbc2c18[2] = {
#include "ev0071/bird_textures/dat_0cbc2c18.inc"
};
static NJS_MKEY_A dat_0cbc2c38[2] = {
#include "ev0071/bird_textures/dat_0cbc2c38.inc"
};
static NJS_MKEY_A dat_0cbc2c58[2] = {
#include "ev0071/bird_textures/dat_0cbc2c58.inc"
};
static NJS_MKEY_A dat_0cbc2c78[2] = {
#include "ev0071/bird_textures/dat_0cbc2c78.inc"
};
static NJS_MKEY_A dat_0cbc2c98[2] = {
#include "ev0071/bird_textures/dat_0cbc2c98.inc"
};
static NJS_MKEY_A dat_0cbc2cb8[2] = {
#include "ev0071/bird_textures/dat_0cbc2cb8.inc"
};
static NJS_MKEY_A dat_0cbc2cd8[2] = {
#include "ev0071/bird_textures/dat_0cbc2cd8.inc"
};
static NJS_MKEY_A dat_0cbc2cf8[2] = {
#include "ev0071/bird_textures/dat_0cbc2cf8.inc"
};
static NJS_MKEY_A dat_0cbc2d18[2] = {
#include "ev0071/bird_textures/dat_0cbc2d18.inc"
};
static NJS_MKEY_A dat_0cbc2d38[2] = {
#include "ev0071/bird_textures/dat_0cbc2d38.inc"
};
static NJS_MKEY_A dat_0cbc2d58[2] = {
#include "ev0071/bird_textures/dat_0cbc2d58.inc"
};
static NJS_MKEY_A dat_0cbc2d78[2] = {
#include "ev0071/bird_textures/dat_0cbc2d78.inc"
};
static NJS_MKEY_A dat_0cbc2d98[2] = {
#include "ev0071/bird_textures/dat_0cbc2d98.inc"
};
static NJS_MKEY_A dat_0cbc2db8[2] = {
#include "ev0071/bird_textures/dat_0cbc2db8.inc"
};
static NJS_MKEY_A dat_0cbc2dd8[2] = {
#include "ev0071/bird_textures/dat_0cbc2dd8.inc"
};
static NJS_MKEY_A dat_0cbc2df8[2] = {
#include "ev0071/bird_textures/dat_0cbc2df8.inc"
};
static NJS_MKEY_A dat_0cbc2e18[2] = {
#include "ev0071/bird_textures/dat_0cbc2e18.inc"
};
static NJS_MKEY_A dat_0cbc2e38[2] = {
#include "ev0071/bird_textures/dat_0cbc2e38.inc"
};
static NJS_MKEY_A dat_0cbc2e58[2] = {
#include "ev0071/bird_textures/dat_0cbc2e58.inc"
};
static NJS_MKEY_A dat_0cbc2e78[2] = {
#include "ev0071/bird_textures/dat_0cbc2e78.inc"
};
static NJS_MKEY_A dat_0cbc2e98[2] = {
#include "ev0071/bird_textures/dat_0cbc2e98.inc"
};
static NJS_MKEY_A dat_0cbc2eb8[2] = {
#include "ev0071/bird_textures/dat_0cbc2eb8.inc"
};
static NJS_MKEY_A dat_0cbc2ed8[2] = {
#include "ev0071/bird_textures/dat_0cbc2ed8.inc"
};
static NJS_MKEY_A dat_0cbc2ef8[2] = {
#include "ev0071/bird_textures/dat_0cbc2ef8.inc"
};
static NJS_MKEY_A dat_0cbc2f18[2] = {
#include "ev0071/bird_textures/dat_0cbc2f18.inc"
};
static NJS_MKEY_A dat_0cbc2f38[2] = {
#include "ev0071/bird_textures/dat_0cbc2f38.inc"
};
static NJS_MKEY_A dat_0cbc2f58[2] = {
#include "ev0071/bird_textures/dat_0cbc2f58.inc"
};
static NJS_MDATA2 ev0071_bird_motion_2_mdata[57] = {
#include "ev0071/bird_textures/ev0071_bird_motion_2_mdata.inc"
};
static NJS_MOTION ev0071_bird_motion_2 = {
#include "ev0071/bird_textures/ev0071_bird_motion_2.inc"
};
NJS_ACTION ev0071_bird_action_2 = {
#include "ev0071/bird_textures/ev0071_bird_action_2.inc"
};
static NJS_TEXNAME ev0071_wing_p_textures[7] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_4, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_10, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0},
};
NJS_TEXLIST ev0071_wing_p_texlist = {
#include "ev0071/bird_textures/ev0071_wing_p_texlist.inc"
};
static NJS_MATERIAL dat_0cbc3378[1] = {
#include "ev0071/bird_textures/dat_0cbc3378.inc"
};
static Sint16 dat_0cbc338c[6] = {
#include "ev0071/bird_textures/dat_0cbc338c.inc"
};
static NJS_COLOR dat_0cbc3398[4] = {
#include "ev0071/bird_textures/dat_0cbc3398.inc"
};
static NJS_MESHSET dat_0cbc33a8[1] = {
#include "ev0071/bird_textures/dat_0cbc33a8.inc"
};
static NJS_POINT3 dat_0cbc33c0[4] = {
#include "ev0071/bird_textures/dat_0cbc33c0.inc"
};
static NJS_VECTOR dat_0cbc33f0[4] = {
#include "ev0071/bird_textures/dat_0cbc33f0.inc"
};
static NJS_MODEL dat_0cbc3420 = {
#include "ev0071/bird_textures/dat_0cbc3420.inc"
};
static NJS_OBJECT dat_0cbc3448 = {
#include "ev0071/bird_textures/dat_0cbc3448.inc"
};
static NJS_MATERIAL dat_0cbc347c[1] = {
#include "ev0071/bird_textures/dat_0cbc347c.inc"
};
static Sint16 dat_0cbc3490[12] = {
#include "ev0071/bird_textures/dat_0cbc3490.inc"
};
static NJS_COLOR dat_0cbc34a8[9] = {
#include "ev0071/bird_textures/dat_0cbc34a8.inc"
};
static NJS_MESHSET dat_0cbc34cc[1] = {
#include "ev0071/bird_textures/dat_0cbc34cc.inc"
};
static NJS_POINT3 dat_0cbc34e4[7] = {
#include "ev0071/bird_textures/dat_0cbc34e4.inc"
};
static NJS_VECTOR dat_0cbc3538[7] = {
#include "ev0071/bird_textures/dat_0cbc3538.inc"
};
static NJS_MODEL dat_0cbc358c = {
#include "ev0071/bird_textures/dat_0cbc358c.inc"
};
static NJS_OBJECT dat_0cbc35b4 = {
#include "ev0071/bird_textures/dat_0cbc35b4.inc"
};
static NJS_OBJECT dat_0cbc35e8 = {
#include "ev0071/bird_textures/dat_0cbc35e8.inc"
};
static NJS_OBJECT dat_0cbc361c = {
#include "ev0071/bird_textures/dat_0cbc361c.inc"
};
static NJS_OBJECT dat_0cbc3650 = {
#include "ev0071/bird_textures/dat_0cbc3650.inc"
};
static NJS_OBJECT dat_0cbc3684 = {
#include "ev0071/bird_textures/dat_0cbc3684.inc"
};
static NJS_OBJECT dat_0cbc36b8 = {
#include "ev0071/bird_textures/dat_0cbc36b8.inc"
};
static NJS_OBJECT dat_0cbc36ec = {
#include "ev0071/bird_textures/dat_0cbc36ec.inc"
};
static NJS_MATERIAL dat_0cbc3720[1] = {
#include "ev0071/bird_textures/dat_0cbc3720.inc"
};
static Sint16 dat_0cbc3734[6] = {
#include "ev0071/bird_textures/dat_0cbc3734.inc"
};
static NJS_COLOR dat_0cbc3740[4] = {
#include "ev0071/bird_textures/dat_0cbc3740.inc"
};
static NJS_MESHSET dat_0cbc3750[1] = {
#include "ev0071/bird_textures/dat_0cbc3750.inc"
};
static NJS_POINT3 dat_0cbc3768[4] = {
#include "ev0071/bird_textures/dat_0cbc3768.inc"
};
static NJS_VECTOR dat_0cbc3798[4] = {
#include "ev0071/bird_textures/dat_0cbc3798.inc"
};
static NJS_MODEL dat_0cbc37c8 = {
#include "ev0071/bird_textures/dat_0cbc37c8.inc"
};
static NJS_OBJECT dat_0cbc37f0 = {
#include "ev0071/bird_textures/dat_0cbc37f0.inc"
};
static NJS_MATERIAL dat_0cbc3824[1] = {
#include "ev0071/bird_textures/dat_0cbc3824.inc"
};
static Sint16 dat_0cbc3838[12] = {
#include "ev0071/bird_textures/dat_0cbc3838.inc"
};
static NJS_COLOR dat_0cbc3850[9] = {
#include "ev0071/bird_textures/dat_0cbc3850.inc"
};
static NJS_MESHSET dat_0cbc3874[1] = {
#include "ev0071/bird_textures/dat_0cbc3874.inc"
};
static NJS_POINT3 dat_0cbc388c[7] = {
#include "ev0071/bird_textures/dat_0cbc388c.inc"
};
static NJS_VECTOR dat_0cbc38e0[7] = {
#include "ev0071/bird_textures/dat_0cbc38e0.inc"
};
static NJS_MODEL dat_0cbc3934 = {
#include "ev0071/bird_textures/dat_0cbc3934.inc"
};
static NJS_OBJECT dat_0cbc395c = {
#include "ev0071/bird_textures/dat_0cbc395c.inc"
};
static NJS_OBJECT dat_0cbc3990 = {
#include "ev0071/bird_textures/dat_0cbc3990.inc"
};
static NJS_OBJECT dat_0cbc39c4 = {
#include "ev0071/bird_textures/dat_0cbc39c4.inc"
};
static NJS_OBJECT dat_0cbc39f8 = {
#include "ev0071/bird_textures/dat_0cbc39f8.inc"
};
static NJS_OBJECT dat_0cbc3a2c = {
#include "ev0071/bird_textures/dat_0cbc3a2c.inc"
};
static NJS_OBJECT dat_0cbc3a60 = {
#include "ev0071/bird_textures/dat_0cbc3a60.inc"
};
static NJS_OBJECT dat_0cbc3a94 = {
#include "ev0071/bird_textures/dat_0cbc3a94.inc"
};
static NJS_OBJECT dat_0cbc3ac8 = {
#include "ev0071/bird_textures/dat_0cbc3ac8.inc"
};
static NJS_OBJECT dat_0cbc3afc = {
#include "ev0071/bird_textures/dat_0cbc3afc.inc"
};
static NJS_OBJECT dat_0cbc3b30 = {
#include "ev0071/bird_textures/dat_0cbc3b30.inc"
};
static NJS_OBJECT dat_0cbc3b64 = {
#include "ev0071/bird_textures/dat_0cbc3b64.inc"
};
static NJS_MATERIAL dat_0cbc3b98[1] = {
#include "ev0071/bird_textures/dat_0cbc3b98.inc"
};
static Sint16 dat_0cbc3bac[18] = {
#include "ev0071/bird_textures/dat_0cbc3bac.inc"
};
static NJS_COLOR dat_0cbc3bd0[15] = {
#include "ev0071/bird_textures/dat_0cbc3bd0.inc"
};
static NJS_MESHSET dat_0cbc3c0c[1] = {
#include "ev0071/bird_textures/dat_0cbc3c0c.inc"
};
static NJS_POINT3 dat_0cbc3c24[7] = {
#include "ev0071/bird_textures/dat_0cbc3c24.inc"
};
static NJS_VECTOR dat_0cbc3c78[7] = {
#include "ev0071/bird_textures/dat_0cbc3c78.inc"
};
static NJS_MODEL dat_0cbc3ccc = {
#include "ev0071/bird_textures/dat_0cbc3ccc.inc"
};
static NJS_OBJECT dat_0cbc3cf4 = {
#include "ev0071/bird_textures/dat_0cbc3cf4.inc"
};
static NJS_OBJECT dat_0cbc3d28 = {
#include "ev0071/bird_textures/dat_0cbc3d28.inc"
};
static NJS_OBJECT dat_0cbc3d5c = {
#include "ev0071/bird_textures/dat_0cbc3d5c.inc"
};
static NJS_OBJECT dat_0cbc3d90 = {
#include "ev0071/bird_textures/dat_0cbc3d90.inc"
};
static NJS_MATERIAL dat_0cbc3dc4[1] = {
#include "ev0071/bird_textures/dat_0cbc3dc4.inc"
};
static Sint16 dat_0cbc3dd8[10] = {
#include "ev0071/bird_textures/dat_0cbc3dd8.inc"
};
static NJS_COLOR dat_0cbc3dec[8] = {
#include "ev0071/bird_textures/dat_0cbc3dec.inc"
};
static NJS_MESHSET dat_0cbc3e0c[1] = {
#include "ev0071/bird_textures/dat_0cbc3e0c.inc"
};
static NJS_POINT3 dat_0cbc3e24[8] = {
#include "ev0071/bird_textures/dat_0cbc3e24.inc"
};
static NJS_VECTOR dat_0cbc3e84[8] = {
#include "ev0071/bird_textures/dat_0cbc3e84.inc"
};
static NJS_MODEL dat_0cbc3ee4 = {
#include "ev0071/bird_textures/dat_0cbc3ee4.inc"
};
static NJS_OBJECT dat_0cbc3f0c = {
#include "ev0071/bird_textures/dat_0cbc3f0c.inc"
};
static NJS_MATERIAL dat_0cbc3f40[1] = {
#include "ev0071/bird_textures/dat_0cbc3f40.inc"
};
static Sint16 dat_0cbc3f54[10] = {
#include "ev0071/bird_textures/dat_0cbc3f54.inc"
};
static NJS_COLOR dat_0cbc3f68[8] = {
#include "ev0071/bird_textures/dat_0cbc3f68.inc"
};
static NJS_MESHSET dat_0cbc3f88[1] = {
#include "ev0071/bird_textures/dat_0cbc3f88.inc"
};
static NJS_POINT3 dat_0cbc3fa0[8] = {
#include "ev0071/bird_textures/dat_0cbc3fa0.inc"
};
static NJS_VECTOR dat_0cbc4000[8] = {
#include "ev0071/bird_textures/dat_0cbc4000.inc"
};
static NJS_MODEL dat_0cbc4060 = {
#include "ev0071/bird_textures/dat_0cbc4060.inc"
};
static NJS_OBJECT dat_0cbc4088 = {
#include "ev0071/bird_textures/dat_0cbc4088.inc"
};
static NJS_MATERIAL dat_0cbc40bc[1] = {
#include "ev0071/bird_textures/dat_0cbc40bc.inc"
};
static Sint16 dat_0cbc40d0[16] = {
#include "ev0071/bird_textures/dat_0cbc40d0.inc"
};
static NJS_COLOR dat_0cbc40f0[12] = {
#include "ev0071/bird_textures/dat_0cbc40f0.inc"
};
static NJS_MESHSET dat_0cbc4120[1] = {
#include "ev0071/bird_textures/dat_0cbc4120.inc"
};
static NJS_POINT3 dat_0cbc4138[12] = {
#include "ev0071/bird_textures/dat_0cbc4138.inc"
};
static NJS_VECTOR dat_0cbc41c8[12] = {
#include "ev0071/bird_textures/dat_0cbc41c8.inc"
};
static NJS_MODEL dat_0cbc4258 = {
#include "ev0071/bird_textures/dat_0cbc4258.inc"
};
static NJS_OBJECT dat_0cbc4280 = {
#include "ev0071/bird_textures/dat_0cbc4280.inc"
};
static NJS_MATERIAL dat_0cbc42b4[1] = {
#include "ev0071/bird_textures/dat_0cbc42b4.inc"
};
static Sint16 dat_0cbc42c8[16] = {
#include "ev0071/bird_textures/dat_0cbc42c8.inc"
};
static NJS_COLOR dat_0cbc42e8[12] = {
#include "ev0071/bird_textures/dat_0cbc42e8.inc"
};
static NJS_MESHSET dat_0cbc4318[1] = {
#include "ev0071/bird_textures/dat_0cbc4318.inc"
};
static NJS_POINT3 dat_0cbc4330[12] = {
#include "ev0071/bird_textures/dat_0cbc4330.inc"
};
static NJS_VECTOR dat_0cbc43c0[12] = {
#include "ev0071/bird_textures/dat_0cbc43c0.inc"
};
static NJS_MODEL dat_0cbc4450 = {
#include "ev0071/bird_textures/dat_0cbc4450.inc"
};
static NJS_OBJECT dat_0cbc4478 = {
#include "ev0071/bird_textures/dat_0cbc4478.inc"
};
static NJS_MATERIAL dat_0cbc44ac[4] = {
#include "ev0071/bird_textures/dat_0cbc44ac.inc"
};
static Sint16 dat_0cbc44fc[239] = {
#include "ev0071/bird_textures/dat_0cbc44fc.inc"
};
static Sint16 dat_0cbc46da[22] = {
#include "ev0071/bird_textures/dat_0cbc46da.inc"
};
static Sint16 dat_0cbc4706[21] = {
#include "ev0071/bird_textures/dat_0cbc4706.inc"
};
static Sint16 dat_0cbc4730[22] = {
#include "ev0071/bird_textures/dat_0cbc4730.inc"
};
static NJS_COLOR dat_0cbc475c[209] = {
#include "ev0071/bird_textures/dat_0cbc475c.inc"
};
static NJS_COLOR dat_0cbc4aa0[20] = {
#include "ev0071/bird_textures/dat_0cbc4aa0.inc"
};
static NJS_COLOR dat_0cbc4af0[16] = {
#include "ev0071/bird_textures/dat_0cbc4af0.inc"
};
static NJS_COLOR dat_0cbc4b30[18] = {
#include "ev0071/bird_textures/dat_0cbc4b30.inc"
};
static NJS_MESHSET dat_0cbc4b78[4] = {
#include "ev0071/bird_textures/dat_0cbc4b78.inc"
};
static NJS_POINT3 dat_0cbc4bd8[127] = {
#include "ev0071/bird_textures/dat_0cbc4bd8.inc"
};
static NJS_VECTOR dat_0cbc51cc[127] = {
#include "ev0071/bird_textures/dat_0cbc51cc.inc"
};
static NJS_MODEL dat_0cbc57c0 = {
#include "ev0071/bird_textures/dat_0cbc57c0.inc"
};
static NJS_OBJECT dat_0cbc57e8 = {
#include "ev0071/bird_textures/dat_0cbc57e8.inc"
};
static NJS_OBJECT dat_0cbc581c = {
#include "ev0071/bird_textures/dat_0cbc581c.inc"
};
static NJS_OBJECT dat_0cbc5850 = {
#include "ev0071/bird_textures/dat_0cbc5850.inc"
};
static NJS_OBJECT dat_0cbc5884 = {
#include "ev0071/bird_textures/dat_0cbc5884.inc"
};
static NJS_MATERIAL dat_0cbc58b8[1] = {
#include "ev0071/bird_textures/dat_0cbc58b8.inc"
};
static Sint16 dat_0cbc58cc[48] = {
#include "ev0071/bird_textures/dat_0cbc58cc.inc"
};
static NJS_COLOR dat_0cbc592c[38] = {
#include "ev0071/bird_textures/dat_0cbc592c.inc"
};
static NJS_MESHSET dat_0cbc59c4[1] = {
#include "ev0071/bird_textures/dat_0cbc59c4.inc"
};
static NJS_POINT3 dat_0cbc59dc[12] = {
#include "ev0071/bird_textures/dat_0cbc59dc.inc"
};
static NJS_VECTOR dat_0cbc5a6c[12] = {
#include "ev0071/bird_textures/dat_0cbc5a6c.inc"
};
static NJS_MODEL dat_0cbc5afc = {
#include "ev0071/bird_textures/dat_0cbc5afc.inc"
};
static NJS_OBJECT dat_0cbc5b24 = {
#include "ev0071/bird_textures/dat_0cbc5b24.inc"
};
static NJS_OBJECT dat_0cbc5b58 = {
#include "ev0071/bird_textures/dat_0cbc5b58.inc"
};
static NJS_OBJECT dat_0cbc5b8c = {
#include "ev0071/bird_textures/dat_0cbc5b8c.inc"
};
static NJS_OBJECT dat_0cbc5bc0 = {
#include "ev0071/bird_textures/dat_0cbc5bc0.inc"
};
static NJS_MATERIAL dat_0cbc5bf4[1] = {
#include "ev0071/bird_textures/dat_0cbc5bf4.inc"
};
static Sint16 dat_0cbc5c08[48] = {
#include "ev0071/bird_textures/dat_0cbc5c08.inc"
};
static NJS_COLOR dat_0cbc5c68[38] = {
#include "ev0071/bird_textures/dat_0cbc5c68.inc"
};
static NJS_MESHSET dat_0cbc5d00[1] = {
#include "ev0071/bird_textures/dat_0cbc5d00.inc"
};
static NJS_POINT3 dat_0cbc5d18[12] = {
#include "ev0071/bird_textures/dat_0cbc5d18.inc"
};
static NJS_VECTOR dat_0cbc5da8[12] = {
#include "ev0071/bird_textures/dat_0cbc5da8.inc"
};
static NJS_MODEL dat_0cbc5e38 = {
#include "ev0071/bird_textures/dat_0cbc5e38.inc"
};
static NJS_OBJECT dat_0cbc5e60 = {
#include "ev0071/bird_textures/dat_0cbc5e60.inc"
};
static NJS_OBJECT dat_0cbc5e94 = {
#include "ev0071/bird_textures/dat_0cbc5e94.inc"
};
static NJS_OBJECT dat_0cbc5ec8 = {
#include "ev0071/bird_textures/dat_0cbc5ec8.inc"
};
static NJS_OBJECT dat_0cbc5efc = {
#include "ev0071/bird_textures/dat_0cbc5efc.inc"
};
static NJS_MATERIAL dat_0cbc5f30[2] = {
#include "ev0071/bird_textures/dat_0cbc5f30.inc"
};
static Sint16 dat_0cbc5f58[54] = {
#include "ev0071/bird_textures/dat_0cbc5f58.inc"
};
static Sint16 dat_0cbc5fc4[22] = {
#include "ev0071/bird_textures/dat_0cbc5fc4.inc"
};
static NJS_COLOR dat_0cbc5ff0[48] = {
#include "ev0071/bird_textures/dat_0cbc5ff0.inc"
};
static NJS_COLOR dat_0cbc60b0[18] = {
#include "ev0071/bird_textures/dat_0cbc60b0.inc"
};
static NJS_MESHSET dat_0cbc60f8[2] = {
#include "ev0071/bird_textures/dat_0cbc60f8.inc"
};
static NJS_POINT3 dat_0cbc6128[26] = {
#include "ev0071/bird_textures/dat_0cbc6128.inc"
};
static NJS_VECTOR dat_0cbc6260[26] = {
#include "ev0071/bird_textures/dat_0cbc6260.inc"
};
static NJS_MODEL dat_0cbc6398 = {
#include "ev0071/bird_textures/dat_0cbc6398.inc"
};
static NJS_OBJECT dat_0cbc63c0 = {
#include "ev0071/bird_textures/dat_0cbc63c0.inc"
};
static NJS_OBJECT dat_0cbc63f4 = {
#include "ev0071/bird_textures/dat_0cbc63f4.inc"
};
static NJS_OBJECT dat_0cbc6428 = {
#include "ev0071/bird_textures/dat_0cbc6428.inc"
};
static NJS_OBJECT dat_0cbc645c = {
#include "ev0071/bird_textures/dat_0cbc645c.inc"
};
static NJS_OBJECT ev0071_wing_p_object = {
#include "ev0071/bird_textures/ev0071_wing_p_object.inc"
};
static NJS_MKEY_F dat_0cbc64c4[30] = {
#include "ev0071/bird_textures/dat_0cbc64c4.inc"
};
static NJS_MKEY_A dat_0cbc66a4[2] = {
#include "ev0071/bird_textures/dat_0cbc66a4.inc"
};
static NJS_MKEY_A dat_0cbc66c4[2] = {
#include "ev0071/bird_textures/dat_0cbc66c4.inc"
};
static NJS_MKEY_A dat_0cbc66e4[30] = {
#include "ev0071/bird_textures/dat_0cbc66e4.inc"
};
static NJS_MKEY_A dat_0cbc68c4[2] = {
#include "ev0071/bird_textures/dat_0cbc68c4.inc"
};
static NJS_MKEY_A dat_0cbc68e4[2] = {
#include "ev0071/bird_textures/dat_0cbc68e4.inc"
};
static NJS_MKEY_A dat_0cbc6904[2] = {
#include "ev0071/bird_textures/dat_0cbc6904.inc"
};
static NJS_MKEY_A dat_0cbc6924[30] = {
#include "ev0071/bird_textures/dat_0cbc6924.inc"
};
static NJS_MKEY_A dat_0cbc6b04[2] = {
#include "ev0071/bird_textures/dat_0cbc6b04.inc"
};
static NJS_MKEY_A dat_0cbc6b24[2] = {
#include "ev0071/bird_textures/dat_0cbc6b24.inc"
};
static NJS_MKEY_A dat_0cbc6b44[2] = {
#include "ev0071/bird_textures/dat_0cbc6b44.inc"
};
static NJS_MKEY_A dat_0cbc6b64[30] = {
#include "ev0071/bird_textures/dat_0cbc6b64.inc"
};
static NJS_MKEY_A dat_0cbc6d44[2] = {
#include "ev0071/bird_textures/dat_0cbc6d44.inc"
};
static NJS_MKEY_A dat_0cbc6d64[2] = {
#include "ev0071/bird_textures/dat_0cbc6d64.inc"
};
static NJS_MKEY_A dat_0cbc6d84[2] = {
#include "ev0071/bird_textures/dat_0cbc6d84.inc"
};
static NJS_MKEY_A dat_0cbc6da4[30] = {
#include "ev0071/bird_textures/dat_0cbc6da4.inc"
};
static NJS_MKEY_A dat_0cbc6f84[2] = {
#include "ev0071/bird_textures/dat_0cbc6f84.inc"
};
static NJS_MKEY_A dat_0cbc6fa4[2] = {
#include "ev0071/bird_textures/dat_0cbc6fa4.inc"
};
static NJS_MKEY_A dat_0cbc6fc4[2] = {
#include "ev0071/bird_textures/dat_0cbc6fc4.inc"
};
static NJS_MKEY_A dat_0cbc6fe4[2] = {
#include "ev0071/bird_textures/dat_0cbc6fe4.inc"
};
static NJS_MKEY_A dat_0cbc7004[2] = {
#include "ev0071/bird_textures/dat_0cbc7004.inc"
};
static NJS_MKEY_A dat_0cbc7024[2] = {
#include "ev0071/bird_textures/dat_0cbc7024.inc"
};
static NJS_MKEY_A dat_0cbc7044[2] = {
#include "ev0071/bird_textures/dat_0cbc7044.inc"
};
static NJS_MKEY_A dat_0cbc7064[30] = {
#include "ev0071/bird_textures/dat_0cbc7064.inc"
};
static NJS_MKEY_A dat_0cbc7244[2] = {
#include "ev0071/bird_textures/dat_0cbc7244.inc"
};
static NJS_MKEY_A dat_0cbc7264[2] = {
#include "ev0071/bird_textures/dat_0cbc7264.inc"
};
static NJS_MKEY_A dat_0cbc7284[2] = {
#include "ev0071/bird_textures/dat_0cbc7284.inc"
};
static NJS_MKEY_A dat_0cbc72a4[2] = {
#include "ev0071/bird_textures/dat_0cbc72a4.inc"
};
static NJS_MKEY_A dat_0cbc72c4[2] = {
#include "ev0071/bird_textures/dat_0cbc72c4.inc"
};
static NJS_MKEY_A dat_0cbc72e4[2] = {
#include "ev0071/bird_textures/dat_0cbc72e4.inc"
};
static NJS_MKEY_A dat_0cbc7304[2] = {
#include "ev0071/bird_textures/dat_0cbc7304.inc"
};
static NJS_MKEY_A dat_0cbc7324[30] = {
#include "ev0071/bird_textures/dat_0cbc7324.inc"
};
static NJS_MKEY_A dat_0cbc7504[2] = {
#include "ev0071/bird_textures/dat_0cbc7504.inc"
};
static NJS_MKEY_A dat_0cbc7524[2] = {
#include "ev0071/bird_textures/dat_0cbc7524.inc"
};
static NJS_MKEY_A dat_0cbc7544[30] = {
#include "ev0071/bird_textures/dat_0cbc7544.inc"
};
static NJS_MKEY_A dat_0cbc7724[2] = {
#include "ev0071/bird_textures/dat_0cbc7724.inc"
};
static NJS_MKEY_A dat_0cbc7744[2] = {
#include "ev0071/bird_textures/dat_0cbc7744.inc"
};
static NJS_MKEY_A dat_0cbc7764[2] = {
#include "ev0071/bird_textures/dat_0cbc7764.inc"
};
static NJS_MKEY_A dat_0cbc7784[2] = {
#include "ev0071/bird_textures/dat_0cbc7784.inc"
};
static NJS_MKEY_A dat_0cbc77a4[30] = {
#include "ev0071/bird_textures/dat_0cbc77a4.inc"
};
static NJS_MKEY_A dat_0cbc7984[2] = {
#include "ev0071/bird_textures/dat_0cbc7984.inc"
};
static NJS_MKEY_A dat_0cbc79a4[2] = {
#include "ev0071/bird_textures/dat_0cbc79a4.inc"
};
static NJS_MKEY_A dat_0cbc79c4[30] = {
#include "ev0071/bird_textures/dat_0cbc79c4.inc"
};
static NJS_MKEY_A dat_0cbc7ba4[2] = {
#include "ev0071/bird_textures/dat_0cbc7ba4.inc"
};
static NJS_MKEY_A dat_0cbc7bc4[2] = {
#include "ev0071/bird_textures/dat_0cbc7bc4.inc"
};
static NJS_MKEY_A dat_0cbc7be4[2] = {
#include "ev0071/bird_textures/dat_0cbc7be4.inc"
};
static NJS_MDATA2 ev0071_wing_p_motion_mdata[45] = {
#include "ev0071/bird_textures/ev0071_wing_p_motion_mdata.inc"
};
static NJS_MOTION ev0071_wing_p_motion = {
#include "ev0071/bird_textures/ev0071_wing_p_motion.inc"
};
NJS_ACTION ev0071_wing_p_action = {
#include "ev0071/bird_textures/ev0071_wing_p_action.inc"
};
static NJS_TEXNAME ev0071_wing_t_textures[7] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_4, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_10, 0, 0}, {TEXT_12, 0, 0}, {TEXT_14, 0, 0},
};
NJS_TEXLIST ev0071_wing_t_texlist = {
#include "ev0071/bird_textures/ev0071_wing_t_texlist.inc"
};
/* the one material of node 0x0cbc8014 of the model ev0071_wing_t_action
 * moves. the rest of that model, and the action, lie in the data after this
 * object */
static NJS_MATERIAL dat_0cbc7f44[1] = {
#include "ev0071/bird_textures/dat_0cbc7f44.inc"
};
