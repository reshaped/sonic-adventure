/* ev006d_amy_action_9 and its motion, then the bird's two texlists, two
 * models and six actions, then ev006d_picture_texlist. Amy's model lies in
 * 1ST_READ.BIN. her motion's NJS_MDATA2 table, dat_0cba7898, lies in the
 * data before this object */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev006d/amy_action_9.h"

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
#define TEXT_13_LENGTH 6
#define TEXT_14_LENGTH 8
#include "ev006d/amy_action_9/text.inc"

extern NJS_MDATA2 dat_0cba7898[];
extern NJS_OBJECT amy_object;

static NJS_MOTION ev006d_amy_motion_9 = {
#include "ev006d/amy_action_9/ev006d_amy_motion_9.inc"
};
NJS_ACTION ev006d_amy_action_9 = {
#include "ev006d/amy_action_9/ev006d_amy_action_9.inc"
};
/* the bird's two texlists hold the same twelve names. script.c's
 * ev006d_textures pairs the first with the PVM VER1_WING, and the second
 * with VER2_WING */
static NJS_TEXNAME ev006d_bird_textures_1[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev006d_bird_texlist_1 = {
#include "ev006d/amy_action_9/ev006d_bird_texlist_1.inc"
};
static NJS_TEXNAME ev006d_bird_textures_2[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev006d_bird_texlist_2 = {
#include "ev006d/amy_action_9/ev006d_bird_texlist_2.inc"
};
static NJS_MATERIAL dat_0cba7ebc[1] = {
#include "ev006d/amy_action_9/dat_0cba7ebc.inc"
};
static Sint16 dat_0cba7ed0[6] = {
#include "ev006d/amy_action_9/dat_0cba7ed0.inc"
};
static NJS_COLOR dat_0cba7edc[4] = {
#include "ev006d/amy_action_9/dat_0cba7edc.inc"
};
static NJS_MESHSET dat_0cba7eec[1] = {
#include "ev006d/amy_action_9/dat_0cba7eec.inc"
};
static NJS_POINT3 dat_0cba7f04[4] = {
#include "ev006d/amy_action_9/dat_0cba7f04.inc"
};
static NJS_VECTOR dat_0cba7f34[4] = {
#include "ev006d/amy_action_9/dat_0cba7f34.inc"
};
static NJS_MODEL dat_0cba7f64 = {
#include "ev006d/amy_action_9/dat_0cba7f64.inc"
};
static NJS_OBJECT dat_0cba7f8c = {
#include "ev006d/amy_action_9/dat_0cba7f8c.inc"
};
static NJS_MATERIAL dat_0cba7fc0[1] = {
#include "ev006d/amy_action_9/dat_0cba7fc0.inc"
};
static Sint16 dat_0cba7fd4[12] = {
#include "ev006d/amy_action_9/dat_0cba7fd4.inc"
};
static NJS_COLOR dat_0cba7fec[9] = {
#include "ev006d/amy_action_9/dat_0cba7fec.inc"
};
static NJS_MESHSET dat_0cba8010[1] = {
#include "ev006d/amy_action_9/dat_0cba8010.inc"
};
static NJS_POINT3 dat_0cba8028[7] = {
#include "ev006d/amy_action_9/dat_0cba8028.inc"
};
static NJS_VECTOR dat_0cba807c[7] = {
#include "ev006d/amy_action_9/dat_0cba807c.inc"
};
static NJS_MODEL dat_0cba80d0 = {
#include "ev006d/amy_action_9/dat_0cba80d0.inc"
};
static NJS_OBJECT dat_0cba80f8 = {
#include "ev006d/amy_action_9/dat_0cba80f8.inc"
};
static NJS_OBJECT dat_0cba812c = {
#include "ev006d/amy_action_9/dat_0cba812c.inc"
};
static NJS_OBJECT dat_0cba8160 = {
#include "ev006d/amy_action_9/dat_0cba8160.inc"
};
static NJS_OBJECT dat_0cba8194 = {
#include "ev006d/amy_action_9/dat_0cba8194.inc"
};
static NJS_OBJECT dat_0cba81c8 = {
#include "ev006d/amy_action_9/dat_0cba81c8.inc"
};
static NJS_OBJECT dat_0cba81fc = {
#include "ev006d/amy_action_9/dat_0cba81fc.inc"
};
static NJS_OBJECT dat_0cba8230 = {
#include "ev006d/amy_action_9/dat_0cba8230.inc"
};
static NJS_MATERIAL dat_0cba8264[1] = {
#include "ev006d/amy_action_9/dat_0cba8264.inc"
};
static Sint16 dat_0cba8278[6] = {
#include "ev006d/amy_action_9/dat_0cba8278.inc"
};
static NJS_COLOR dat_0cba8284[4] = {
#include "ev006d/amy_action_9/dat_0cba8284.inc"
};
static NJS_MESHSET dat_0cba8294[1] = {
#include "ev006d/amy_action_9/dat_0cba8294.inc"
};
static NJS_POINT3 dat_0cba82ac[4] = {
#include "ev006d/amy_action_9/dat_0cba82ac.inc"
};
static NJS_VECTOR dat_0cba82dc[4] = {
#include "ev006d/amy_action_9/dat_0cba82dc.inc"
};
static NJS_MODEL dat_0cba830c = {
#include "ev006d/amy_action_9/dat_0cba830c.inc"
};
static NJS_OBJECT dat_0cba8334 = {
#include "ev006d/amy_action_9/dat_0cba8334.inc"
};
static NJS_MATERIAL dat_0cba8368[1] = {
#include "ev006d/amy_action_9/dat_0cba8368.inc"
};
static Sint16 dat_0cba837c[12] = {
#include "ev006d/amy_action_9/dat_0cba837c.inc"
};
static NJS_COLOR dat_0cba8394[9] = {
#include "ev006d/amy_action_9/dat_0cba8394.inc"
};
static NJS_MESHSET dat_0cba83b8[1] = {
#include "ev006d/amy_action_9/dat_0cba83b8.inc"
};
static NJS_POINT3 dat_0cba83d0[7] = {
#include "ev006d/amy_action_9/dat_0cba83d0.inc"
};
static NJS_VECTOR dat_0cba8424[7] = {
#include "ev006d/amy_action_9/dat_0cba8424.inc"
};
static NJS_MODEL dat_0cba8478 = {
#include "ev006d/amy_action_9/dat_0cba8478.inc"
};
static NJS_OBJECT dat_0cba84a0 = {
#include "ev006d/amy_action_9/dat_0cba84a0.inc"
};
static NJS_OBJECT dat_0cba84d4 = {
#include "ev006d/amy_action_9/dat_0cba84d4.inc"
};
static NJS_OBJECT dat_0cba8508 = {
#include "ev006d/amy_action_9/dat_0cba8508.inc"
};
static NJS_OBJECT dat_0cba853c = {
#include "ev006d/amy_action_9/dat_0cba853c.inc"
};
static NJS_OBJECT dat_0cba8570 = {
#include "ev006d/amy_action_9/dat_0cba8570.inc"
};
static NJS_OBJECT dat_0cba85a4 = {
#include "ev006d/amy_action_9/dat_0cba85a4.inc"
};
static NJS_OBJECT dat_0cba85d8 = {
#include "ev006d/amy_action_9/dat_0cba85d8.inc"
};
static NJS_OBJECT dat_0cba860c = {
#include "ev006d/amy_action_9/dat_0cba860c.inc"
};
static NJS_OBJECT dat_0cba8640 = {
#include "ev006d/amy_action_9/dat_0cba8640.inc"
};
static NJS_OBJECT dat_0cba8674 = {
#include "ev006d/amy_action_9/dat_0cba8674.inc"
};
static NJS_OBJECT dat_0cba86a8 = {
#include "ev006d/amy_action_9/dat_0cba86a8.inc"
};
static NJS_OBJECT dat_0cba86dc = {
#include "ev006d/amy_action_9/dat_0cba86dc.inc"
};
static NJS_MATERIAL dat_0cba8710[1] = {
#include "ev006d/amy_action_9/dat_0cba8710.inc"
};
static Sint16 dat_0cba8724[18] = {
#include "ev006d/amy_action_9/dat_0cba8724.inc"
};
static NJS_COLOR dat_0cba8748[15] = {
#include "ev006d/amy_action_9/dat_0cba8748.inc"
};
static NJS_MESHSET dat_0cba8784[1] = {
#include "ev006d/amy_action_9/dat_0cba8784.inc"
};
static NJS_POINT3 dat_0cba879c[7] = {
#include "ev006d/amy_action_9/dat_0cba879c.inc"
};
static NJS_VECTOR dat_0cba87f0[7] = {
#include "ev006d/amy_action_9/dat_0cba87f0.inc"
};
static NJS_MODEL dat_0cba8844 = {
#include "ev006d/amy_action_9/dat_0cba8844.inc"
};
static NJS_OBJECT dat_0cba886c = {
#include "ev006d/amy_action_9/dat_0cba886c.inc"
};
static NJS_OBJECT dat_0cba88a0 = {
#include "ev006d/amy_action_9/dat_0cba88a0.inc"
};
static NJS_OBJECT dat_0cba88d4 = {
#include "ev006d/amy_action_9/dat_0cba88d4.inc"
};
static NJS_OBJECT dat_0cba8908 = {
#include "ev006d/amy_action_9/dat_0cba8908.inc"
};
static NJS_MATERIAL dat_0cba893c[1] = {
#include "ev006d/amy_action_9/dat_0cba893c.inc"
};
static Sint16 dat_0cba8950[10] = {
#include "ev006d/amy_action_9/dat_0cba8950.inc"
};
static NJS_COLOR dat_0cba8964[8] = {
#include "ev006d/amy_action_9/dat_0cba8964.inc"
};
static NJS_MESHSET dat_0cba8984[1] = {
#include "ev006d/amy_action_9/dat_0cba8984.inc"
};
static NJS_POINT3 dat_0cba899c[8] = {
#include "ev006d/amy_action_9/dat_0cba899c.inc"
};
static NJS_VECTOR dat_0cba89fc[8] = {
#include "ev006d/amy_action_9/dat_0cba89fc.inc"
};
static NJS_MODEL dat_0cba8a5c = {
#include "ev006d/amy_action_9/dat_0cba8a5c.inc"
};
static NJS_OBJECT dat_0cba8a84 = {
#include "ev006d/amy_action_9/dat_0cba8a84.inc"
};
static NJS_MATERIAL dat_0cba8ab8[1] = {
#include "ev006d/amy_action_9/dat_0cba8ab8.inc"
};
static Sint16 dat_0cba8acc[10] = {
#include "ev006d/amy_action_9/dat_0cba8acc.inc"
};
static NJS_COLOR dat_0cba8ae0[8] = {
#include "ev006d/amy_action_9/dat_0cba8ae0.inc"
};
static NJS_MESHSET dat_0cba8b00[1] = {
#include "ev006d/amy_action_9/dat_0cba8b00.inc"
};
static NJS_POINT3 dat_0cba8b18[8] = {
#include "ev006d/amy_action_9/dat_0cba8b18.inc"
};
static NJS_VECTOR dat_0cba8b78[8] = {
#include "ev006d/amy_action_9/dat_0cba8b78.inc"
};
static NJS_MODEL dat_0cba8bd8 = {
#include "ev006d/amy_action_9/dat_0cba8bd8.inc"
};
static NJS_OBJECT dat_0cba8c00 = {
#include "ev006d/amy_action_9/dat_0cba8c00.inc"
};
static NJS_MATERIAL dat_0cba8c34[1] = {
#include "ev006d/amy_action_9/dat_0cba8c34.inc"
};
static Sint16 dat_0cba8c48[16] = {
#include "ev006d/amy_action_9/dat_0cba8c48.inc"
};
static NJS_COLOR dat_0cba8c68[12] = {
#include "ev006d/amy_action_9/dat_0cba8c68.inc"
};
static NJS_MESHSET dat_0cba8c98[1] = {
#include "ev006d/amy_action_9/dat_0cba8c98.inc"
};
static NJS_POINT3 dat_0cba8cb0[12] = {
#include "ev006d/amy_action_9/dat_0cba8cb0.inc"
};
static NJS_VECTOR dat_0cba8d40[12] = {
#include "ev006d/amy_action_9/dat_0cba8d40.inc"
};
static NJS_MODEL dat_0cba8dd0 = {
#include "ev006d/amy_action_9/dat_0cba8dd0.inc"
};
static NJS_OBJECT dat_0cba8df8 = {
#include "ev006d/amy_action_9/dat_0cba8df8.inc"
};
static NJS_MATERIAL dat_0cba8e2c[1] = {
#include "ev006d/amy_action_9/dat_0cba8e2c.inc"
};
static Sint16 dat_0cba8e40[16] = {
#include "ev006d/amy_action_9/dat_0cba8e40.inc"
};
static NJS_COLOR dat_0cba8e60[12] = {
#include "ev006d/amy_action_9/dat_0cba8e60.inc"
};
static NJS_MESHSET dat_0cba8e90[1] = {
#include "ev006d/amy_action_9/dat_0cba8e90.inc"
};
static NJS_POINT3 dat_0cba8ea8[12] = {
#include "ev006d/amy_action_9/dat_0cba8ea8.inc"
};
static NJS_VECTOR dat_0cba8f38[12] = {
#include "ev006d/amy_action_9/dat_0cba8f38.inc"
};
static NJS_MODEL dat_0cba8fc8 = {
#include "ev006d/amy_action_9/dat_0cba8fc8.inc"
};
static NJS_OBJECT dat_0cba8ff0 = {
#include "ev006d/amy_action_9/dat_0cba8ff0.inc"
};
static NJS_MATERIAL dat_0cba9024[4] = {
#include "ev006d/amy_action_9/dat_0cba9024.inc"
};
static Sint16 dat_0cba9074[239] = {
#include "ev006d/amy_action_9/dat_0cba9074.inc"
};
static Sint16 dat_0cba9252[22] = {
#include "ev006d/amy_action_9/dat_0cba9252.inc"
};
static Sint16 dat_0cba927e[21] = {
#include "ev006d/amy_action_9/dat_0cba927e.inc"
};
static Sint16 dat_0cba92a8[22] = {
#include "ev006d/amy_action_9/dat_0cba92a8.inc"
};
static NJS_COLOR dat_0cba92d4[209] = {
#include "ev006d/amy_action_9/dat_0cba92d4.inc"
};
static NJS_COLOR dat_0cba9618[20] = {
#include "ev006d/amy_action_9/dat_0cba9618.inc"
};
static NJS_COLOR dat_0cba9668[16] = {
#include "ev006d/amy_action_9/dat_0cba9668.inc"
};
static NJS_COLOR dat_0cba96a8[18] = {
#include "ev006d/amy_action_9/dat_0cba96a8.inc"
};
static NJS_MESHSET dat_0cba96f0[4] = {
#include "ev006d/amy_action_9/dat_0cba96f0.inc"
};
static NJS_POINT3 dat_0cba9750[127] = {
#include "ev006d/amy_action_9/dat_0cba9750.inc"
};
static NJS_VECTOR dat_0cba9d44[127] = {
#include "ev006d/amy_action_9/dat_0cba9d44.inc"
};
static NJS_MODEL dat_0cbaa338 = {
#include "ev006d/amy_action_9/dat_0cbaa338.inc"
};
static NJS_OBJECT dat_0cbaa360 = {
#include "ev006d/amy_action_9/dat_0cbaa360.inc"
};
static NJS_OBJECT dat_0cbaa394 = {
#include "ev006d/amy_action_9/dat_0cbaa394.inc"
};
static NJS_OBJECT dat_0cbaa3c8 = {
#include "ev006d/amy_action_9/dat_0cbaa3c8.inc"
};
static NJS_OBJECT dat_0cbaa3fc = {
#include "ev006d/amy_action_9/dat_0cbaa3fc.inc"
};
static NJS_MATERIAL dat_0cbaa430[1] = {
#include "ev006d/amy_action_9/dat_0cbaa430.inc"
};
static Sint16 dat_0cbaa444[8] = {
#include "ev006d/amy_action_9/dat_0cbaa444.inc"
};
static NJS_COLOR dat_0cbaa454[6] = {
#include "ev006d/amy_action_9/dat_0cbaa454.inc"
};
static NJS_MESHSET dat_0cbaa46c[1] = {
#include "ev006d/amy_action_9/dat_0cbaa46c.inc"
};
static NJS_POINT3 dat_0cbaa484[6] = {
#include "ev006d/amy_action_9/dat_0cbaa484.inc"
};
static NJS_VECTOR dat_0cbaa4cc[6] = {
#include "ev006d/amy_action_9/dat_0cbaa4cc.inc"
};
static NJS_MODEL dat_0cbaa514 = {
#include "ev006d/amy_action_9/dat_0cbaa514.inc"
};
static NJS_OBJECT dat_0cbaa53c = {
#include "ev006d/amy_action_9/dat_0cbaa53c.inc"
};
static NJS_MATERIAL dat_0cbaa570[1] = {
#include "ev006d/amy_action_9/dat_0cbaa570.inc"
};
static Sint16 dat_0cbaa584[22] = {
#include "ev006d/amy_action_9/dat_0cbaa584.inc"
};
static NJS_COLOR dat_0cbaa5b0[16] = {
#include "ev006d/amy_action_9/dat_0cbaa5b0.inc"
};
static NJS_MESHSET dat_0cbaa5f0[1] = {
#include "ev006d/amy_action_9/dat_0cbaa5f0.inc"
};
static NJS_POINT3 dat_0cbaa608[5] = {
#include "ev006d/amy_action_9/dat_0cbaa608.inc"
};
static NJS_VECTOR dat_0cbaa644[5] = {
#include "ev006d/amy_action_9/dat_0cbaa644.inc"
};
static NJS_MODEL dat_0cbaa680 = {
#include "ev006d/amy_action_9/dat_0cbaa680.inc"
};
static NJS_OBJECT dat_0cbaa6a8 = {
#include "ev006d/amy_action_9/dat_0cbaa6a8.inc"
};
static NJS_MATERIAL dat_0cbaa6dc[1] = {
#include "ev006d/amy_action_9/dat_0cbaa6dc.inc"
};
static Sint16 dat_0cbaa6f0[30] = {
#include "ev006d/amy_action_9/dat_0cbaa6f0.inc"
};
static NJS_COLOR dat_0cbaa72c[28] = {
#include "ev006d/amy_action_9/dat_0cbaa72c.inc"
};
static NJS_MESHSET dat_0cbaa79c[1] = {
#include "ev006d/amy_action_9/dat_0cbaa79c.inc"
};
static NJS_POINT3 dat_0cbaa7b4[18] = {
#include "ev006d/amy_action_9/dat_0cbaa7b4.inc"
};
static NJS_VECTOR dat_0cbaa88c[18] = {
#include "ev006d/amy_action_9/dat_0cbaa88c.inc"
};
static NJS_MODEL dat_0cbaa964 = {
#include "ev006d/amy_action_9/dat_0cbaa964.inc"
};
static NJS_OBJECT dat_0cbaa98c = {
#include "ev006d/amy_action_9/dat_0cbaa98c.inc"
};
static NJS_MATERIAL dat_0cbaa9c0[1] = {
#include "ev006d/amy_action_9/dat_0cbaa9c0.inc"
};
static Sint16 dat_0cbaa9d4[20] = {
#include "ev006d/amy_action_9/dat_0cbaa9d4.inc"
};
static NJS_COLOR dat_0cbaa9fc[16] = {
#include "ev006d/amy_action_9/dat_0cbaa9fc.inc"
};
static NJS_MESHSET dat_0cbaaa3c[1] = {
#include "ev006d/amy_action_9/dat_0cbaaa3c.inc"
};
static NJS_POINT3 dat_0cbaaa54[12] = {
#include "ev006d/amy_action_9/dat_0cbaaa54.inc"
};
static NJS_VECTOR dat_0cbaaae4[12] = {
#include "ev006d/amy_action_9/dat_0cbaaae4.inc"
};
static NJS_MODEL dat_0cbaab74 = {
#include "ev006d/amy_action_9/dat_0cbaab74.inc"
};
static NJS_OBJECT dat_0cbaab9c = {
#include "ev006d/amy_action_9/dat_0cbaab9c.inc"
};
static NJS_MATERIAL dat_0cbaabd0[1] = {
#include "ev006d/amy_action_9/dat_0cbaabd0.inc"
};
static Sint16 dat_0cbaabe4[10] = {
#include "ev006d/amy_action_9/dat_0cbaabe4.inc"
};
static NJS_COLOR dat_0cbaabf8[8] = {
#include "ev006d/amy_action_9/dat_0cbaabf8.inc"
};
static NJS_MESHSET dat_0cbaac18[1] = {
#include "ev006d/amy_action_9/dat_0cbaac18.inc"
};
static NJS_POINT3 dat_0cbaac30[5] = {
#include "ev006d/amy_action_9/dat_0cbaac30.inc"
};
static NJS_VECTOR dat_0cbaac6c[5] = {
#include "ev006d/amy_action_9/dat_0cbaac6c.inc"
};
static NJS_MODEL dat_0cbaaca8 = {
#include "ev006d/amy_action_9/dat_0cbaaca8.inc"
};
static NJS_OBJECT dat_0cbaacd0 = {
#include "ev006d/amy_action_9/dat_0cbaacd0.inc"
};
static NJS_MATERIAL dat_0cbaad04[1] = {
#include "ev006d/amy_action_9/dat_0cbaad04.inc"
};
static Sint16 dat_0cbaad18[10] = {
#include "ev006d/amy_action_9/dat_0cbaad18.inc"
};
static NJS_COLOR dat_0cbaad2c[8] = {
#include "ev006d/amy_action_9/dat_0cbaad2c.inc"
};
static NJS_MESHSET dat_0cbaad4c[1] = {
#include "ev006d/amy_action_9/dat_0cbaad4c.inc"
};
static NJS_POINT3 dat_0cbaad64[5] = {
#include "ev006d/amy_action_9/dat_0cbaad64.inc"
};
static NJS_VECTOR dat_0cbaada0[5] = {
#include "ev006d/amy_action_9/dat_0cbaada0.inc"
};
static NJS_MODEL dat_0cbaaddc = {
#include "ev006d/amy_action_9/dat_0cbaaddc.inc"
};
static NJS_OBJECT dat_0cbaae04 = {
#include "ev006d/amy_action_9/dat_0cbaae04.inc"
};
static NJS_MATERIAL dat_0cbaae38[1] = {
#include "ev006d/amy_action_9/dat_0cbaae38.inc"
};
static Sint16 dat_0cbaae4c[24] = {
#include "ev006d/amy_action_9/dat_0cbaae4c.inc"
};
static NJS_COLOR dat_0cbaae7c[20] = {
#include "ev006d/amy_action_9/dat_0cbaae7c.inc"
};
static NJS_MESHSET dat_0cbaaecc[1] = {
#include "ev006d/amy_action_9/dat_0cbaaecc.inc"
};
static NJS_POINT3 dat_0cbaaee4[12] = {
#include "ev006d/amy_action_9/dat_0cbaaee4.inc"
};
static NJS_VECTOR dat_0cbaaf74[12] = {
#include "ev006d/amy_action_9/dat_0cbaaf74.inc"
};
static NJS_MODEL dat_0cbab004 = {
#include "ev006d/amy_action_9/dat_0cbab004.inc"
};
static NJS_OBJECT dat_0cbab02c = {
#include "ev006d/amy_action_9/dat_0cbab02c.inc"
};
static NJS_MATERIAL dat_0cbab060[1] = {
#include "ev006d/amy_action_9/dat_0cbab060.inc"
};
static Sint16 dat_0cbab074[36] = {
#include "ev006d/amy_action_9/dat_0cbab074.inc"
};
static NJS_COLOR dat_0cbab0bc[30] = {
#include "ev006d/amy_action_9/dat_0cbab0bc.inc"
};
static NJS_MESHSET dat_0cbab134[1] = {
#include "ev006d/amy_action_9/dat_0cbab134.inc"
};
static NJS_POINT3 dat_0cbab14c[12] = {
#include "ev006d/amy_action_9/dat_0cbab14c.inc"
};
static NJS_VECTOR dat_0cbab1dc[12] = {
#include "ev006d/amy_action_9/dat_0cbab1dc.inc"
};
static NJS_MODEL dat_0cbab26c = {
#include "ev006d/amy_action_9/dat_0cbab26c.inc"
};
static NJS_OBJECT dat_0cbab294 = {
#include "ev006d/amy_action_9/dat_0cbab294.inc"
};
static NJS_OBJECT dat_0cbab2c8 = {
#include "ev006d/amy_action_9/dat_0cbab2c8.inc"
};
static NJS_OBJECT dat_0cbab2fc = {
#include "ev006d/amy_action_9/dat_0cbab2fc.inc"
};
static NJS_OBJECT dat_0cbab330 = {
#include "ev006d/amy_action_9/dat_0cbab330.inc"
};
static NJS_MATERIAL dat_0cbab364[1] = {
#include "ev006d/amy_action_9/dat_0cbab364.inc"
};
static Sint16 dat_0cbab378[48] = {
#include "ev006d/amy_action_9/dat_0cbab378.inc"
};
static NJS_COLOR dat_0cbab3d8[38] = {
#include "ev006d/amy_action_9/dat_0cbab3d8.inc"
};
static NJS_MESHSET dat_0cbab470[1] = {
#include "ev006d/amy_action_9/dat_0cbab470.inc"
};
static NJS_POINT3 dat_0cbab488[12] = {
#include "ev006d/amy_action_9/dat_0cbab488.inc"
};
static NJS_VECTOR dat_0cbab518[12] = {
#include "ev006d/amy_action_9/dat_0cbab518.inc"
};
static NJS_MODEL dat_0cbab5a8 = {
#include "ev006d/amy_action_9/dat_0cbab5a8.inc"
};
static NJS_OBJECT dat_0cbab5d0 = {
#include "ev006d/amy_action_9/dat_0cbab5d0.inc"
};
static NJS_OBJECT dat_0cbab604 = {
#include "ev006d/amy_action_9/dat_0cbab604.inc"
};
static NJS_OBJECT dat_0cbab638 = {
#include "ev006d/amy_action_9/dat_0cbab638.inc"
};
static NJS_OBJECT dat_0cbab66c = {
#include "ev006d/amy_action_9/dat_0cbab66c.inc"
};
static NJS_MATERIAL dat_0cbab6a0[1] = {
#include "ev006d/amy_action_9/dat_0cbab6a0.inc"
};
static Sint16 dat_0cbab6b4[48] = {
#include "ev006d/amy_action_9/dat_0cbab6b4.inc"
};
static NJS_COLOR dat_0cbab714[38] = {
#include "ev006d/amy_action_9/dat_0cbab714.inc"
};
static NJS_MESHSET dat_0cbab7ac[1] = {
#include "ev006d/amy_action_9/dat_0cbab7ac.inc"
};
static NJS_POINT3 dat_0cbab7c4[12] = {
#include "ev006d/amy_action_9/dat_0cbab7c4.inc"
};
static NJS_VECTOR dat_0cbab854[12] = {
#include "ev006d/amy_action_9/dat_0cbab854.inc"
};
static NJS_MODEL dat_0cbab8e4 = {
#include "ev006d/amy_action_9/dat_0cbab8e4.inc"
};
static NJS_OBJECT dat_0cbab90c = {
#include "ev006d/amy_action_9/dat_0cbab90c.inc"
};
static NJS_OBJECT dat_0cbab940 = {
#include "ev006d/amy_action_9/dat_0cbab940.inc"
};
static NJS_OBJECT dat_0cbab974 = {
#include "ev006d/amy_action_9/dat_0cbab974.inc"
};
static NJS_OBJECT dat_0cbab9a8 = {
#include "ev006d/amy_action_9/dat_0cbab9a8.inc"
};
static NJS_MATERIAL dat_0cbab9dc[2] = {
#include "ev006d/amy_action_9/dat_0cbab9dc.inc"
};
static Sint16 dat_0cbaba04[54] = {
#include "ev006d/amy_action_9/dat_0cbaba04.inc"
};
static Sint16 dat_0cbaba70[22] = {
#include "ev006d/amy_action_9/dat_0cbaba70.inc"
};
static NJS_COLOR dat_0cbaba9c[48] = {
#include "ev006d/amy_action_9/dat_0cbaba9c.inc"
};
static NJS_COLOR dat_0cbabb5c[18] = {
#include "ev006d/amy_action_9/dat_0cbabb5c.inc"
};
static NJS_MESHSET dat_0cbabba4[2] = {
#include "ev006d/amy_action_9/dat_0cbabba4.inc"
};
static NJS_POINT3 dat_0cbabbd4[26] = {
#include "ev006d/amy_action_9/dat_0cbabbd4.inc"
};
static NJS_VECTOR dat_0cbabd0c[26] = {
#include "ev006d/amy_action_9/dat_0cbabd0c.inc"
};
static NJS_MODEL dat_0cbabe44 = {
#include "ev006d/amy_action_9/dat_0cbabe44.inc"
};
static NJS_OBJECT dat_0cbabe6c = {
#include "ev006d/amy_action_9/dat_0cbabe6c.inc"
};
static NJS_OBJECT dat_0cbabea0 = {
#include "ev006d/amy_action_9/dat_0cbabea0.inc"
};
static NJS_OBJECT dat_0cbabed4 = {
#include "ev006d/amy_action_9/dat_0cbabed4.inc"
};
static NJS_OBJECT dat_0cbabf08 = {
#include "ev006d/amy_action_9/dat_0cbabf08.inc"
};
/* this model matches ev006d_bird_object but for five of 57 nodes, shrunk to
 * a scale of 0.00001. script.c draws it with VER1_WING's textures, and
 * ev006d_bird_object with VER2_WING's. ev006d_bird_action_1 to 3 move it,
 * as do dat_0cbb4bec and dat_0cbb93f4. no word in EV006D or 1ST_READ.BIN
 * points at those two */
static NJS_OBJECT ev006d_bird_ver1_object = {
#include "ev006d/amy_action_9/ev006d_bird_ver1_object.inc"
};
static NJS_MATERIAL dat_0cbabf70[1] = {
#include "ev006d/amy_action_9/dat_0cbabf70.inc"
};
static Sint16 dat_0cbabf84[6] = {
#include "ev006d/amy_action_9/dat_0cbabf84.inc"
};
static NJS_COLOR dat_0cbabf90[4] = {
#include "ev006d/amy_action_9/dat_0cbabf90.inc"
};
static NJS_MESHSET dat_0cbabfa0[1] = {
#include "ev006d/amy_action_9/dat_0cbabfa0.inc"
};
static NJS_POINT3 dat_0cbabfb8[4] = {
#include "ev006d/amy_action_9/dat_0cbabfb8.inc"
};
static NJS_VECTOR dat_0cbabfe8[4] = {
#include "ev006d/amy_action_9/dat_0cbabfe8.inc"
};
static NJS_MODEL dat_0cbac018 = {
#include "ev006d/amy_action_9/dat_0cbac018.inc"
};
static NJS_OBJECT dat_0cbac040 = {
#include "ev006d/amy_action_9/dat_0cbac040.inc"
};
static NJS_MATERIAL dat_0cbac074[1] = {
#include "ev006d/amy_action_9/dat_0cbac074.inc"
};
static Sint16 dat_0cbac088[12] = {
#include "ev006d/amy_action_9/dat_0cbac088.inc"
};
static NJS_COLOR dat_0cbac0a0[9] = {
#include "ev006d/amy_action_9/dat_0cbac0a0.inc"
};
static NJS_MESHSET dat_0cbac0c4[1] = {
#include "ev006d/amy_action_9/dat_0cbac0c4.inc"
};
static NJS_POINT3 dat_0cbac0dc[7] = {
#include "ev006d/amy_action_9/dat_0cbac0dc.inc"
};
static NJS_VECTOR dat_0cbac130[7] = {
#include "ev006d/amy_action_9/dat_0cbac130.inc"
};
static NJS_MODEL dat_0cbac184 = {
#include "ev006d/amy_action_9/dat_0cbac184.inc"
};
static NJS_OBJECT dat_0cbac1ac = {
#include "ev006d/amy_action_9/dat_0cbac1ac.inc"
};
static NJS_OBJECT dat_0cbac1e0 = {
#include "ev006d/amy_action_9/dat_0cbac1e0.inc"
};
static NJS_OBJECT dat_0cbac214 = {
#include "ev006d/amy_action_9/dat_0cbac214.inc"
};
static NJS_OBJECT dat_0cbac248 = {
#include "ev006d/amy_action_9/dat_0cbac248.inc"
};
static NJS_OBJECT dat_0cbac27c = {
#include "ev006d/amy_action_9/dat_0cbac27c.inc"
};
static NJS_OBJECT dat_0cbac2b0 = {
#include "ev006d/amy_action_9/dat_0cbac2b0.inc"
};
static NJS_OBJECT dat_0cbac2e4 = {
#include "ev006d/amy_action_9/dat_0cbac2e4.inc"
};
static NJS_MATERIAL dat_0cbac318[1] = {
#include "ev006d/amy_action_9/dat_0cbac318.inc"
};
static Sint16 dat_0cbac32c[6] = {
#include "ev006d/amy_action_9/dat_0cbac32c.inc"
};
static NJS_COLOR dat_0cbac338[4] = {
#include "ev006d/amy_action_9/dat_0cbac338.inc"
};
static NJS_MESHSET dat_0cbac348[1] = {
#include "ev006d/amy_action_9/dat_0cbac348.inc"
};
static NJS_POINT3 dat_0cbac360[4] = {
#include "ev006d/amy_action_9/dat_0cbac360.inc"
};
static NJS_VECTOR dat_0cbac390[4] = {
#include "ev006d/amy_action_9/dat_0cbac390.inc"
};
static NJS_MODEL dat_0cbac3c0 = {
#include "ev006d/amy_action_9/dat_0cbac3c0.inc"
};
static NJS_OBJECT dat_0cbac3e8 = {
#include "ev006d/amy_action_9/dat_0cbac3e8.inc"
};
static NJS_MATERIAL dat_0cbac41c[1] = {
#include "ev006d/amy_action_9/dat_0cbac41c.inc"
};
static Sint16 dat_0cbac430[12] = {
#include "ev006d/amy_action_9/dat_0cbac430.inc"
};
static NJS_COLOR dat_0cbac448[9] = {
#include "ev006d/amy_action_9/dat_0cbac448.inc"
};
static NJS_MESHSET dat_0cbac46c[1] = {
#include "ev006d/amy_action_9/dat_0cbac46c.inc"
};
static NJS_POINT3 dat_0cbac484[7] = {
#include "ev006d/amy_action_9/dat_0cbac484.inc"
};
static NJS_VECTOR dat_0cbac4d8[7] = {
#include "ev006d/amy_action_9/dat_0cbac4d8.inc"
};
static NJS_MODEL dat_0cbac52c = {
#include "ev006d/amy_action_9/dat_0cbac52c.inc"
};
static NJS_OBJECT dat_0cbac554 = {
#include "ev006d/amy_action_9/dat_0cbac554.inc"
};
static NJS_OBJECT dat_0cbac588 = {
#include "ev006d/amy_action_9/dat_0cbac588.inc"
};
static NJS_OBJECT dat_0cbac5bc = {
#include "ev006d/amy_action_9/dat_0cbac5bc.inc"
};
static NJS_OBJECT dat_0cbac5f0 = {
#include "ev006d/amy_action_9/dat_0cbac5f0.inc"
};
static NJS_OBJECT dat_0cbac624 = {
#include "ev006d/amy_action_9/dat_0cbac624.inc"
};
static NJS_OBJECT dat_0cbac658 = {
#include "ev006d/amy_action_9/dat_0cbac658.inc"
};
static NJS_OBJECT dat_0cbac68c = {
#include "ev006d/amy_action_9/dat_0cbac68c.inc"
};
static NJS_OBJECT dat_0cbac6c0 = {
#include "ev006d/amy_action_9/dat_0cbac6c0.inc"
};
static NJS_OBJECT dat_0cbac6f4 = {
#include "ev006d/amy_action_9/dat_0cbac6f4.inc"
};
static NJS_OBJECT dat_0cbac728 = {
#include "ev006d/amy_action_9/dat_0cbac728.inc"
};
static NJS_OBJECT dat_0cbac75c = {
#include "ev006d/amy_action_9/dat_0cbac75c.inc"
};
static NJS_OBJECT dat_0cbac790 = {
#include "ev006d/amy_action_9/dat_0cbac790.inc"
};
static NJS_MATERIAL dat_0cbac7c4[1] = {
#include "ev006d/amy_action_9/dat_0cbac7c4.inc"
};
static Sint16 dat_0cbac7d8[18] = {
#include "ev006d/amy_action_9/dat_0cbac7d8.inc"
};
static NJS_COLOR dat_0cbac7fc[15] = {
#include "ev006d/amy_action_9/dat_0cbac7fc.inc"
};
static NJS_MESHSET dat_0cbac838[1] = {
#include "ev006d/amy_action_9/dat_0cbac838.inc"
};
static NJS_POINT3 dat_0cbac850[7] = {
#include "ev006d/amy_action_9/dat_0cbac850.inc"
};
static NJS_VECTOR dat_0cbac8a4[7] = {
#include "ev006d/amy_action_9/dat_0cbac8a4.inc"
};
static NJS_MODEL dat_0cbac8f8 = {
#include "ev006d/amy_action_9/dat_0cbac8f8.inc"
};
static NJS_OBJECT dat_0cbac920 = {
#include "ev006d/amy_action_9/dat_0cbac920.inc"
};
static NJS_OBJECT dat_0cbac954 = {
#include "ev006d/amy_action_9/dat_0cbac954.inc"
};
static NJS_OBJECT dat_0cbac988 = {
#include "ev006d/amy_action_9/dat_0cbac988.inc"
};
static NJS_OBJECT dat_0cbac9bc = {
#include "ev006d/amy_action_9/dat_0cbac9bc.inc"
};
static NJS_MATERIAL dat_0cbac9f0[1] = {
#include "ev006d/amy_action_9/dat_0cbac9f0.inc"
};
static Sint16 dat_0cbaca04[10] = {
#include "ev006d/amy_action_9/dat_0cbaca04.inc"
};
static NJS_COLOR dat_0cbaca18[8] = {
#include "ev006d/amy_action_9/dat_0cbaca18.inc"
};
static NJS_MESHSET dat_0cbaca38[1] = {
#include "ev006d/amy_action_9/dat_0cbaca38.inc"
};
static NJS_POINT3 dat_0cbaca50[8] = {
#include "ev006d/amy_action_9/dat_0cbaca50.inc"
};
static NJS_VECTOR dat_0cbacab0[8] = {
#include "ev006d/amy_action_9/dat_0cbacab0.inc"
};
static NJS_MODEL dat_0cbacb10 = {
#include "ev006d/amy_action_9/dat_0cbacb10.inc"
};
static NJS_OBJECT dat_0cbacb38 = {
#include "ev006d/amy_action_9/dat_0cbacb38.inc"
};
static NJS_MATERIAL dat_0cbacb6c[1] = {
#include "ev006d/amy_action_9/dat_0cbacb6c.inc"
};
static Sint16 dat_0cbacb80[10] = {
#include "ev006d/amy_action_9/dat_0cbacb80.inc"
};
static NJS_COLOR dat_0cbacb94[8] = {
#include "ev006d/amy_action_9/dat_0cbacb94.inc"
};
static NJS_MESHSET dat_0cbacbb4[1] = {
#include "ev006d/amy_action_9/dat_0cbacbb4.inc"
};
static NJS_POINT3 dat_0cbacbcc[8] = {
#include "ev006d/amy_action_9/dat_0cbacbcc.inc"
};
static NJS_VECTOR dat_0cbacc2c[8] = {
#include "ev006d/amy_action_9/dat_0cbacc2c.inc"
};
static NJS_MODEL dat_0cbacc8c = {
#include "ev006d/amy_action_9/dat_0cbacc8c.inc"
};
static NJS_OBJECT dat_0cbaccb4 = {
#include "ev006d/amy_action_9/dat_0cbaccb4.inc"
};
static NJS_MATERIAL dat_0cbacce8[1] = {
#include "ev006d/amy_action_9/dat_0cbacce8.inc"
};
static Sint16 dat_0cbaccfc[16] = {
#include "ev006d/amy_action_9/dat_0cbaccfc.inc"
};
static NJS_COLOR dat_0cbacd1c[12] = {
#include "ev006d/amy_action_9/dat_0cbacd1c.inc"
};
static NJS_MESHSET dat_0cbacd4c[1] = {
#include "ev006d/amy_action_9/dat_0cbacd4c.inc"
};
static NJS_POINT3 dat_0cbacd64[12] = {
#include "ev006d/amy_action_9/dat_0cbacd64.inc"
};
static NJS_VECTOR dat_0cbacdf4[12] = {
#include "ev006d/amy_action_9/dat_0cbacdf4.inc"
};
static NJS_MODEL dat_0cbace84 = {
#include "ev006d/amy_action_9/dat_0cbace84.inc"
};
static NJS_OBJECT dat_0cbaceac = {
#include "ev006d/amy_action_9/dat_0cbaceac.inc"
};
static NJS_MATERIAL dat_0cbacee0[1] = {
#include "ev006d/amy_action_9/dat_0cbacee0.inc"
};
static Sint16 dat_0cbacef4[16] = {
#include "ev006d/amy_action_9/dat_0cbacef4.inc"
};
static NJS_COLOR dat_0cbacf14[12] = {
#include "ev006d/amy_action_9/dat_0cbacf14.inc"
};
static NJS_MESHSET dat_0cbacf44[1] = {
#include "ev006d/amy_action_9/dat_0cbacf44.inc"
};
static NJS_POINT3 dat_0cbacf5c[12] = {
#include "ev006d/amy_action_9/dat_0cbacf5c.inc"
};
static NJS_VECTOR dat_0cbacfec[12] = {
#include "ev006d/amy_action_9/dat_0cbacfec.inc"
};
static NJS_MODEL dat_0cbad07c = {
#include "ev006d/amy_action_9/dat_0cbad07c.inc"
};
static NJS_OBJECT dat_0cbad0a4 = {
#include "ev006d/amy_action_9/dat_0cbad0a4.inc"
};
static NJS_MATERIAL dat_0cbad0d8[4] = {
#include "ev006d/amy_action_9/dat_0cbad0d8.inc"
};
static Sint16 dat_0cbad128[239] = {
#include "ev006d/amy_action_9/dat_0cbad128.inc"
};
static Sint16 dat_0cbad306[22] = {
#include "ev006d/amy_action_9/dat_0cbad306.inc"
};
static Sint16 dat_0cbad332[21] = {
#include "ev006d/amy_action_9/dat_0cbad332.inc"
};
static Sint16 dat_0cbad35c[22] = {
#include "ev006d/amy_action_9/dat_0cbad35c.inc"
};
static NJS_COLOR dat_0cbad388[209] = {
#include "ev006d/amy_action_9/dat_0cbad388.inc"
};
static NJS_COLOR dat_0cbad6cc[20] = {
#include "ev006d/amy_action_9/dat_0cbad6cc.inc"
};
static NJS_COLOR dat_0cbad71c[16] = {
#include "ev006d/amy_action_9/dat_0cbad71c.inc"
};
static NJS_COLOR dat_0cbad75c[18] = {
#include "ev006d/amy_action_9/dat_0cbad75c.inc"
};
static NJS_MESHSET dat_0cbad7a4[4] = {
#include "ev006d/amy_action_9/dat_0cbad7a4.inc"
};
static NJS_POINT3 dat_0cbad804[127] = {
#include "ev006d/amy_action_9/dat_0cbad804.inc"
};
static NJS_VECTOR dat_0cbaddf8[127] = {
#include "ev006d/amy_action_9/dat_0cbaddf8.inc"
};
static NJS_MODEL dat_0cbae3ec = {
#include "ev006d/amy_action_9/dat_0cbae3ec.inc"
};
static NJS_OBJECT dat_0cbae414 = {
#include "ev006d/amy_action_9/dat_0cbae414.inc"
};
static NJS_OBJECT dat_0cbae448 = {
#include "ev006d/amy_action_9/dat_0cbae448.inc"
};
static NJS_OBJECT dat_0cbae47c = {
#include "ev006d/amy_action_9/dat_0cbae47c.inc"
};
static NJS_OBJECT dat_0cbae4b0 = {
#include "ev006d/amy_action_9/dat_0cbae4b0.inc"
};
static NJS_MATERIAL dat_0cbae4e4[1] = {
#include "ev006d/amy_action_9/dat_0cbae4e4.inc"
};
static Sint16 dat_0cbae4f8[8] = {
#include "ev006d/amy_action_9/dat_0cbae4f8.inc"
};
static NJS_COLOR dat_0cbae508[6] = {
#include "ev006d/amy_action_9/dat_0cbae508.inc"
};
static NJS_MESHSET dat_0cbae520[1] = {
#include "ev006d/amy_action_9/dat_0cbae520.inc"
};
static NJS_POINT3 dat_0cbae538[6] = {
#include "ev006d/amy_action_9/dat_0cbae538.inc"
};
static NJS_VECTOR dat_0cbae580[6] = {
#include "ev006d/amy_action_9/dat_0cbae580.inc"
};
static NJS_MODEL dat_0cbae5c8 = {
#include "ev006d/amy_action_9/dat_0cbae5c8.inc"
};
static NJS_OBJECT dat_0cbae5f0 = {
#include "ev006d/amy_action_9/dat_0cbae5f0.inc"
};
static NJS_MATERIAL dat_0cbae624[1] = {
#include "ev006d/amy_action_9/dat_0cbae624.inc"
};
static Sint16 dat_0cbae638[22] = {
#include "ev006d/amy_action_9/dat_0cbae638.inc"
};
static NJS_COLOR dat_0cbae664[16] = {
#include "ev006d/amy_action_9/dat_0cbae664.inc"
};
static NJS_MESHSET dat_0cbae6a4[1] = {
#include "ev006d/amy_action_9/dat_0cbae6a4.inc"
};
static NJS_POINT3 dat_0cbae6bc[5] = {
#include "ev006d/amy_action_9/dat_0cbae6bc.inc"
};
static NJS_VECTOR dat_0cbae6f8[5] = {
#include "ev006d/amy_action_9/dat_0cbae6f8.inc"
};
static NJS_MODEL dat_0cbae734 = {
#include "ev006d/amy_action_9/dat_0cbae734.inc"
};
static NJS_OBJECT dat_0cbae75c = {
#include "ev006d/amy_action_9/dat_0cbae75c.inc"
};
static NJS_MATERIAL dat_0cbae790[1] = {
#include "ev006d/amy_action_9/dat_0cbae790.inc"
};
static Sint16 dat_0cbae7a4[30] = {
#include "ev006d/amy_action_9/dat_0cbae7a4.inc"
};
static NJS_COLOR dat_0cbae7e0[28] = {
#include "ev006d/amy_action_9/dat_0cbae7e0.inc"
};
static NJS_MESHSET dat_0cbae850[1] = {
#include "ev006d/amy_action_9/dat_0cbae850.inc"
};
static NJS_POINT3 dat_0cbae868[18] = {
#include "ev006d/amy_action_9/dat_0cbae868.inc"
};
static NJS_VECTOR dat_0cbae940[18] = {
#include "ev006d/amy_action_9/dat_0cbae940.inc"
};
static NJS_MODEL dat_0cbaea18 = {
#include "ev006d/amy_action_9/dat_0cbaea18.inc"
};
static NJS_OBJECT dat_0cbaea40 = {
#include "ev006d/amy_action_9/dat_0cbaea40.inc"
};
static NJS_MATERIAL dat_0cbaea74[1] = {
#include "ev006d/amy_action_9/dat_0cbaea74.inc"
};
static Sint16 dat_0cbaea88[20] = {
#include "ev006d/amy_action_9/dat_0cbaea88.inc"
};
static NJS_COLOR dat_0cbaeab0[16] = {
#include "ev006d/amy_action_9/dat_0cbaeab0.inc"
};
static NJS_MESHSET dat_0cbaeaf0[1] = {
#include "ev006d/amy_action_9/dat_0cbaeaf0.inc"
};
static NJS_POINT3 dat_0cbaeb08[12] = {
#include "ev006d/amy_action_9/dat_0cbaeb08.inc"
};
static NJS_VECTOR dat_0cbaeb98[12] = {
#include "ev006d/amy_action_9/dat_0cbaeb98.inc"
};
static NJS_MODEL dat_0cbaec28 = {
#include "ev006d/amy_action_9/dat_0cbaec28.inc"
};
static NJS_OBJECT dat_0cbaec50 = {
#include "ev006d/amy_action_9/dat_0cbaec50.inc"
};
static NJS_MATERIAL dat_0cbaec84[1] = {
#include "ev006d/amy_action_9/dat_0cbaec84.inc"
};
static Sint16 dat_0cbaec98[10] = {
#include "ev006d/amy_action_9/dat_0cbaec98.inc"
};
static NJS_COLOR dat_0cbaecac[8] = {
#include "ev006d/amy_action_9/dat_0cbaecac.inc"
};
static NJS_MESHSET dat_0cbaeccc[1] = {
#include "ev006d/amy_action_9/dat_0cbaeccc.inc"
};
static NJS_POINT3 dat_0cbaece4[5] = {
#include "ev006d/amy_action_9/dat_0cbaece4.inc"
};
static NJS_VECTOR dat_0cbaed20[5] = {
#include "ev006d/amy_action_9/dat_0cbaed20.inc"
};
static NJS_MODEL dat_0cbaed5c = {
#include "ev006d/amy_action_9/dat_0cbaed5c.inc"
};
static NJS_OBJECT dat_0cbaed84 = {
#include "ev006d/amy_action_9/dat_0cbaed84.inc"
};
static NJS_MATERIAL dat_0cbaedb8[1] = {
#include "ev006d/amy_action_9/dat_0cbaedb8.inc"
};
static Sint16 dat_0cbaedcc[10] = {
#include "ev006d/amy_action_9/dat_0cbaedcc.inc"
};
static NJS_COLOR dat_0cbaede0[8] = {
#include "ev006d/amy_action_9/dat_0cbaede0.inc"
};
static NJS_MESHSET dat_0cbaee00[1] = {
#include "ev006d/amy_action_9/dat_0cbaee00.inc"
};
static NJS_POINT3 dat_0cbaee18[5] = {
#include "ev006d/amy_action_9/dat_0cbaee18.inc"
};
static NJS_VECTOR dat_0cbaee54[5] = {
#include "ev006d/amy_action_9/dat_0cbaee54.inc"
};
static NJS_MODEL dat_0cbaee90 = {
#include "ev006d/amy_action_9/dat_0cbaee90.inc"
};
static NJS_OBJECT dat_0cbaeeb8 = {
#include "ev006d/amy_action_9/dat_0cbaeeb8.inc"
};
static NJS_MATERIAL dat_0cbaeeec[1] = {
#include "ev006d/amy_action_9/dat_0cbaeeec.inc"
};
static Sint16 dat_0cbaef00[24] = {
#include "ev006d/amy_action_9/dat_0cbaef00.inc"
};
static NJS_COLOR dat_0cbaef30[20] = {
#include "ev006d/amy_action_9/dat_0cbaef30.inc"
};
static NJS_MESHSET dat_0cbaef80[1] = {
#include "ev006d/amy_action_9/dat_0cbaef80.inc"
};
static NJS_POINT3 dat_0cbaef98[12] = {
#include "ev006d/amy_action_9/dat_0cbaef98.inc"
};
static NJS_VECTOR dat_0cbaf028[12] = {
#include "ev006d/amy_action_9/dat_0cbaf028.inc"
};
static NJS_MODEL dat_0cbaf0b8 = {
#include "ev006d/amy_action_9/dat_0cbaf0b8.inc"
};
static NJS_OBJECT dat_0cbaf0e0 = {
#include "ev006d/amy_action_9/dat_0cbaf0e0.inc"
};
static NJS_MATERIAL dat_0cbaf114[1] = {
#include "ev006d/amy_action_9/dat_0cbaf114.inc"
};
static Sint16 dat_0cbaf128[36] = {
#include "ev006d/amy_action_9/dat_0cbaf128.inc"
};
static NJS_COLOR dat_0cbaf170[30] = {
#include "ev006d/amy_action_9/dat_0cbaf170.inc"
};
static NJS_MESHSET dat_0cbaf1e8[1] = {
#include "ev006d/amy_action_9/dat_0cbaf1e8.inc"
};
static NJS_POINT3 dat_0cbaf200[12] = {
#include "ev006d/amy_action_9/dat_0cbaf200.inc"
};
static NJS_VECTOR dat_0cbaf290[12] = {
#include "ev006d/amy_action_9/dat_0cbaf290.inc"
};
static NJS_MODEL dat_0cbaf320 = {
#include "ev006d/amy_action_9/dat_0cbaf320.inc"
};
static NJS_OBJECT dat_0cbaf348 = {
#include "ev006d/amy_action_9/dat_0cbaf348.inc"
};
static NJS_OBJECT dat_0cbaf37c = {
#include "ev006d/amy_action_9/dat_0cbaf37c.inc"
};
static NJS_OBJECT dat_0cbaf3b0 = {
#include "ev006d/amy_action_9/dat_0cbaf3b0.inc"
};
static NJS_OBJECT dat_0cbaf3e4 = {
#include "ev006d/amy_action_9/dat_0cbaf3e4.inc"
};
static NJS_MATERIAL dat_0cbaf418[1] = {
#include "ev006d/amy_action_9/dat_0cbaf418.inc"
};
static Sint16 dat_0cbaf42c[48] = {
#include "ev006d/amy_action_9/dat_0cbaf42c.inc"
};
static NJS_COLOR dat_0cbaf48c[38] = {
#include "ev006d/amy_action_9/dat_0cbaf48c.inc"
};
static NJS_MESHSET dat_0cbaf524[1] = {
#include "ev006d/amy_action_9/dat_0cbaf524.inc"
};
static NJS_POINT3 dat_0cbaf53c[12] = {
#include "ev006d/amy_action_9/dat_0cbaf53c.inc"
};
static NJS_VECTOR dat_0cbaf5cc[12] = {
#include "ev006d/amy_action_9/dat_0cbaf5cc.inc"
};
static NJS_MODEL dat_0cbaf65c = {
#include "ev006d/amy_action_9/dat_0cbaf65c.inc"
};
static NJS_OBJECT dat_0cbaf684 = {
#include "ev006d/amy_action_9/dat_0cbaf684.inc"
};
static NJS_OBJECT dat_0cbaf6b8 = {
#include "ev006d/amy_action_9/dat_0cbaf6b8.inc"
};
static NJS_OBJECT dat_0cbaf6ec = {
#include "ev006d/amy_action_9/dat_0cbaf6ec.inc"
};
static NJS_OBJECT dat_0cbaf720 = {
#include "ev006d/amy_action_9/dat_0cbaf720.inc"
};
static NJS_MATERIAL dat_0cbaf754[1] = {
#include "ev006d/amy_action_9/dat_0cbaf754.inc"
};
static Sint16 dat_0cbaf768[48] = {
#include "ev006d/amy_action_9/dat_0cbaf768.inc"
};
static NJS_COLOR dat_0cbaf7c8[38] = {
#include "ev006d/amy_action_9/dat_0cbaf7c8.inc"
};
static NJS_MESHSET dat_0cbaf860[1] = {
#include "ev006d/amy_action_9/dat_0cbaf860.inc"
};
static NJS_POINT3 dat_0cbaf878[12] = {
#include "ev006d/amy_action_9/dat_0cbaf878.inc"
};
static NJS_VECTOR dat_0cbaf908[12] = {
#include "ev006d/amy_action_9/dat_0cbaf908.inc"
};
static NJS_MODEL dat_0cbaf998 = {
#include "ev006d/amy_action_9/dat_0cbaf998.inc"
};
static NJS_OBJECT dat_0cbaf9c0 = {
#include "ev006d/amy_action_9/dat_0cbaf9c0.inc"
};
static NJS_OBJECT dat_0cbaf9f4 = {
#include "ev006d/amy_action_9/dat_0cbaf9f4.inc"
};
static NJS_OBJECT dat_0cbafa28 = {
#include "ev006d/amy_action_9/dat_0cbafa28.inc"
};
static NJS_OBJECT dat_0cbafa5c = {
#include "ev006d/amy_action_9/dat_0cbafa5c.inc"
};
static NJS_MATERIAL dat_0cbafa90[2] = {
#include "ev006d/amy_action_9/dat_0cbafa90.inc"
};
static Sint16 dat_0cbafab8[54] = {
#include "ev006d/amy_action_9/dat_0cbafab8.inc"
};
static Sint16 dat_0cbafb24[22] = {
#include "ev006d/amy_action_9/dat_0cbafb24.inc"
};
static NJS_COLOR dat_0cbafb50[48] = {
#include "ev006d/amy_action_9/dat_0cbafb50.inc"
};
static NJS_COLOR dat_0cbafc10[18] = {
#include "ev006d/amy_action_9/dat_0cbafc10.inc"
};
static NJS_MESHSET dat_0cbafc58[2] = {
#include "ev006d/amy_action_9/dat_0cbafc58.inc"
};
static NJS_POINT3 dat_0cbafc88[26] = {
#include "ev006d/amy_action_9/dat_0cbafc88.inc"
};
static NJS_VECTOR dat_0cbafdc0[26] = {
#include "ev006d/amy_action_9/dat_0cbafdc0.inc"
};
static NJS_MODEL dat_0cbafef8 = {
#include "ev006d/amy_action_9/dat_0cbafef8.inc"
};
static NJS_OBJECT dat_0cbaff20 = {
#include "ev006d/amy_action_9/dat_0cbaff20.inc"
};
static NJS_OBJECT dat_0cbaff54 = {
#include "ev006d/amy_action_9/dat_0cbaff54.inc"
};
static NJS_OBJECT dat_0cbaff88 = {
#include "ev006d/amy_action_9/dat_0cbaff88.inc"
};
static NJS_OBJECT dat_0cbaffbc = {
#include "ev006d/amy_action_9/dat_0cbaffbc.inc"
};
static NJS_OBJECT ev006d_bird_object = {
#include "ev006d/amy_action_9/ev006d_bird_object.inc"
};
static NJS_MKEY_F dat_0cbb0024[2] = {
#include "ev006d/amy_action_9/dat_0cbb0024.inc"
};
static NJS_MKEY_F dat_0cbb0044[30] = {
#include "ev006d/amy_action_9/dat_0cbb0044.inc"
};
static NJS_MKEY_F dat_0cbb0224[2] = {
#include "ev006d/amy_action_9/dat_0cbb0224.inc"
};
static NJS_MKEY_A dat_0cbb0244[2] = {
#include "ev006d/amy_action_9/dat_0cbb0244.inc"
};
static NJS_MKEY_F dat_0cbb0264[2] = {
#include "ev006d/amy_action_9/dat_0cbb0264.inc"
};
static NJS_MKEY_F dat_0cbb0284[2] = {
#include "ev006d/amy_action_9/dat_0cbb0284.inc"
};
static NJS_MKEY_F dat_0cbb02a4[2] = {
#include "ev006d/amy_action_9/dat_0cbb02a4.inc"
};
static NJS_MKEY_F dat_0cbb02c4[2] = {
#include "ev006d/amy_action_9/dat_0cbb02c4.inc"
};
static NJS_MKEY_F dat_0cbb02e4[2] = {
#include "ev006d/amy_action_9/dat_0cbb02e4.inc"
};
static NJS_MKEY_A dat_0cbb0304[2] = {
#include "ev006d/amy_action_9/dat_0cbb0304.inc"
};
static NJS_MKEY_F dat_0cbb0324[2] = {
#include "ev006d/amy_action_9/dat_0cbb0324.inc"
};
static NJS_MKEY_A dat_0cbb0344[30] = {
#include "ev006d/amy_action_9/dat_0cbb0344.inc"
};
static NJS_MKEY_F dat_0cbb0524[2] = {
#include "ev006d/amy_action_9/dat_0cbb0524.inc"
};
static NJS_MKEY_F dat_0cbb0544[2] = {
#include "ev006d/amy_action_9/dat_0cbb0544.inc"
};
static NJS_MKEY_F dat_0cbb0564[2] = {
#include "ev006d/amy_action_9/dat_0cbb0564.inc"
};
static NJS_MKEY_F dat_0cbb0584[2] = {
#include "ev006d/amy_action_9/dat_0cbb0584.inc"
};
static NJS_MKEY_F dat_0cbb05a4[2] = {
#include "ev006d/amy_action_9/dat_0cbb05a4.inc"
};
static NJS_MKEY_A dat_0cbb05c4[2] = {
#include "ev006d/amy_action_9/dat_0cbb05c4.inc"
};
static NJS_MKEY_F dat_0cbb05e4[2] = {
#include "ev006d/amy_action_9/dat_0cbb05e4.inc"
};
static NJS_MKEY_A dat_0cbb0604[30] = {
#include "ev006d/amy_action_9/dat_0cbb0604.inc"
};
static NJS_MKEY_F dat_0cbb07e4[2] = {
#include "ev006d/amy_action_9/dat_0cbb07e4.inc"
};
static NJS_MKEY_F dat_0cbb0804[2] = {
#include "ev006d/amy_action_9/dat_0cbb0804.inc"
};
static NJS_MKEY_F dat_0cbb0824[2] = {
#include "ev006d/amy_action_9/dat_0cbb0824.inc"
};
static NJS_MKEY_F dat_0cbb0844[2] = {
#include "ev006d/amy_action_9/dat_0cbb0844.inc"
};
static NJS_MKEY_F dat_0cbb0864[2] = {
#include "ev006d/amy_action_9/dat_0cbb0864.inc"
};
static NJS_MKEY_A dat_0cbb0884[2] = {
#include "ev006d/amy_action_9/dat_0cbb0884.inc"
};
static NJS_MKEY_F dat_0cbb08a4[2] = {
#include "ev006d/amy_action_9/dat_0cbb08a4.inc"
};
static NJS_MKEY_A dat_0cbb08c4[2] = {
#include "ev006d/amy_action_9/dat_0cbb08c4.inc"
};
static NJS_MKEY_F dat_0cbb08e4[2] = {
#include "ev006d/amy_action_9/dat_0cbb08e4.inc"
};
static NJS_MKEY_F dat_0cbb0904[2] = {
#include "ev006d/amy_action_9/dat_0cbb0904.inc"
};
static NJS_MKEY_F dat_0cbb0924[2] = {
#include "ev006d/amy_action_9/dat_0cbb0924.inc"
};
static NJS_MKEY_F dat_0cbb0944[2] = {
#include "ev006d/amy_action_9/dat_0cbb0944.inc"
};
static NJS_MKEY_F dat_0cbb0964[2] = {
#include "ev006d/amy_action_9/dat_0cbb0964.inc"
};
static NJS_MKEY_F dat_0cbb0984[2] = {
#include "ev006d/amy_action_9/dat_0cbb0984.inc"
};
static NJS_MKEY_F dat_0cbb09a4[2] = {
#include "ev006d/amy_action_9/dat_0cbb09a4.inc"
};
static NJS_MKEY_F dat_0cbb09c4[2] = {
#include "ev006d/amy_action_9/dat_0cbb09c4.inc"
};
static NJS_MKEY_A dat_0cbb09e4[2] = {
#include "ev006d/amy_action_9/dat_0cbb09e4.inc"
};
static NJS_MKEY_F dat_0cbb0a04[2] = {
#include "ev006d/amy_action_9/dat_0cbb0a04.inc"
};
static NJS_MKEY_F dat_0cbb0a24[2] = {
#include "ev006d/amy_action_9/dat_0cbb0a24.inc"
};
static NJS_MKEY_A dat_0cbb0a44[2] = {
#include "ev006d/amy_action_9/dat_0cbb0a44.inc"
};
static NJS_MKEY_F dat_0cbb0a64[2] = {
#include "ev006d/amy_action_9/dat_0cbb0a64.inc"
};
static NJS_MKEY_A dat_0cbb0a84[2] = {
#include "ev006d/amy_action_9/dat_0cbb0a84.inc"
};
static NJS_MKEY_F dat_0cbb0aa4[2] = {
#include "ev006d/amy_action_9/dat_0cbb0aa4.inc"
};
static NJS_MKEY_A dat_0cbb0ac4[2] = {
#include "ev006d/amy_action_9/dat_0cbb0ac4.inc"
};
static NJS_MKEY_F dat_0cbb0ae4[2] = {
#include "ev006d/amy_action_9/dat_0cbb0ae4.inc"
};
static NJS_MKEY_A dat_0cbb0b04[2] = {
#include "ev006d/amy_action_9/dat_0cbb0b04.inc"
};
static NJS_MKEY_F dat_0cbb0b24[2] = {
#include "ev006d/amy_action_9/dat_0cbb0b24.inc"
};
static NJS_MKEY_F dat_0cbb0b44[2] = {
#include "ev006d/amy_action_9/dat_0cbb0b44.inc"
};
static NJS_MKEY_F dat_0cbb0b64[2] = {
#include "ev006d/amy_action_9/dat_0cbb0b64.inc"
};
static NJS_MKEY_F dat_0cbb0b84[2] = {
#include "ev006d/amy_action_9/dat_0cbb0b84.inc"
};
static NJS_MKEY_F dat_0cbb0ba4[2] = {
#include "ev006d/amy_action_9/dat_0cbb0ba4.inc"
};
static NJS_MKEY_A dat_0cbb0bc4[2] = {
#include "ev006d/amy_action_9/dat_0cbb0bc4.inc"
};
static NJS_MKEY_F dat_0cbb0be4[2] = {
#include "ev006d/amy_action_9/dat_0cbb0be4.inc"
};
static NJS_MKEY_F dat_0cbb0c04[2] = {
#include "ev006d/amy_action_9/dat_0cbb0c04.inc"
};
static NJS_MKEY_A dat_0cbb0c24[2] = {
#include "ev006d/amy_action_9/dat_0cbb0c24.inc"
};
static NJS_MKEY_F dat_0cbb0c44[2] = {
#include "ev006d/amy_action_9/dat_0cbb0c44.inc"
};
static NJS_MKEY_F dat_0cbb0c64[2] = {
#include "ev006d/amy_action_9/dat_0cbb0c64.inc"
};
static NJS_MKEY_A dat_0cbb0c84[2] = {
#include "ev006d/amy_action_9/dat_0cbb0c84.inc"
};
static NJS_MKEY_F dat_0cbb0ca4[2] = {
#include "ev006d/amy_action_9/dat_0cbb0ca4.inc"
};
static NJS_MKEY_F dat_0cbb0cc4[2] = {
#include "ev006d/amy_action_9/dat_0cbb0cc4.inc"
};
static NJS_MKEY_A dat_0cbb0ce4[2] = {
#include "ev006d/amy_action_9/dat_0cbb0ce4.inc"
};
static NJS_MKEY_F dat_0cbb0d04[2] = {
#include "ev006d/amy_action_9/dat_0cbb0d04.inc"
};
static NJS_MKEY_F dat_0cbb0d24[2] = {
#include "ev006d/amy_action_9/dat_0cbb0d24.inc"
};
static NJS_MKEY_A dat_0cbb0d44[2] = {
#include "ev006d/amy_action_9/dat_0cbb0d44.inc"
};
static NJS_MKEY_F dat_0cbb0d64[2] = {
#include "ev006d/amy_action_9/dat_0cbb0d64.inc"
};
static NJS_MKEY_A dat_0cbb0d84[30] = {
#include "ev006d/amy_action_9/dat_0cbb0d84.inc"
};
static NJS_MKEY_F dat_0cbb0f64[2] = {
#include "ev006d/amy_action_9/dat_0cbb0f64.inc"
};
static NJS_MKEY_F dat_0cbb0f84[2] = {
#include "ev006d/amy_action_9/dat_0cbb0f84.inc"
};
static NJS_MKEY_F dat_0cbb0fa4[2] = {
#include "ev006d/amy_action_9/dat_0cbb0fa4.inc"
};
static NJS_MKEY_F dat_0cbb0fc4[2] = {
#include "ev006d/amy_action_9/dat_0cbb0fc4.inc"
};
static NJS_MKEY_F dat_0cbb0fe4[30] = {
#include "ev006d/amy_action_9/dat_0cbb0fe4.inc"
};
static NJS_MKEY_A dat_0cbb11c4[2] = {
#include "ev006d/amy_action_9/dat_0cbb11c4.inc"
};
static NJS_MKEY_F dat_0cbb11e4[2] = {
#include "ev006d/amy_action_9/dat_0cbb11e4.inc"
};
static NJS_MKEY_F dat_0cbb1204[30] = {
#include "ev006d/amy_action_9/dat_0cbb1204.inc"
};
static NJS_MKEY_A dat_0cbb13e4[2] = {
#include "ev006d/amy_action_9/dat_0cbb13e4.inc"
};
static NJS_MKEY_F dat_0cbb1404[2] = {
#include "ev006d/amy_action_9/dat_0cbb1404.inc"
};
static NJS_MKEY_F dat_0cbb1424[2] = {
#include "ev006d/amy_action_9/dat_0cbb1424.inc"
};
static NJS_MKEY_A dat_0cbb1444[2] = {
#include "ev006d/amy_action_9/dat_0cbb1444.inc"
};
static NJS_MKEY_F dat_0cbb1464[2] = {
#include "ev006d/amy_action_9/dat_0cbb1464.inc"
};
static NJS_MKEY_F dat_0cbb1484[2] = {
#include "ev006d/amy_action_9/dat_0cbb1484.inc"
};
static NJS_MKEY_A dat_0cbb14a4[2] = {
#include "ev006d/amy_action_9/dat_0cbb14a4.inc"
};
static NJS_MKEY_F dat_0cbb14c4[2] = {
#include "ev006d/amy_action_9/dat_0cbb14c4.inc"
};
static NJS_MKEY_F dat_0cbb14e4[30] = {
#include "ev006d/amy_action_9/dat_0cbb14e4.inc"
};
static NJS_MKEY_A dat_0cbb16c4[2] = {
#include "ev006d/amy_action_9/dat_0cbb16c4.inc"
};
static NJS_MKEY_F dat_0cbb16e4[2] = {
#include "ev006d/amy_action_9/dat_0cbb16e4.inc"
};
static NJS_MKEY_F dat_0cbb1704[2] = {
#include "ev006d/amy_action_9/dat_0cbb1704.inc"
};
static NJS_MKEY_A dat_0cbb1724[2] = {
#include "ev006d/amy_action_9/dat_0cbb1724.inc"
};
static NJS_MKEY_F dat_0cbb1744[2] = {
#include "ev006d/amy_action_9/dat_0cbb1744.inc"
};
static NJS_MKEY_A dat_0cbb1764[30] = {
#include "ev006d/amy_action_9/dat_0cbb1764.inc"
};
static NJS_MKEY_F dat_0cbb1944[2] = {
#include "ev006d/amy_action_9/dat_0cbb1944.inc"
};
static NJS_MKEY_F dat_0cbb1964[2] = {
#include "ev006d/amy_action_9/dat_0cbb1964.inc"
};
static NJS_MKEY_F dat_0cbb1984[2] = {
#include "ev006d/amy_action_9/dat_0cbb1984.inc"
};
static NJS_MKEY_A dat_0cbb19a4[2] = {
#include "ev006d/amy_action_9/dat_0cbb19a4.inc"
};
static NJS_MKEY_F dat_0cbb19c4[2] = {
#include "ev006d/amy_action_9/dat_0cbb19c4.inc"
};
static NJS_MKEY_A dat_0cbb19e4[30] = {
#include "ev006d/amy_action_9/dat_0cbb19e4.inc"
};
static NJS_MKEY_F dat_0cbb1bc4[2] = {
#include "ev006d/amy_action_9/dat_0cbb1bc4.inc"
};
static NJS_MKEY_F dat_0cbb1be4[2] = {
#include "ev006d/amy_action_9/dat_0cbb1be4.inc"
};
static NJS_MKEY_F dat_0cbb1c04[2] = {
#include "ev006d/amy_action_9/dat_0cbb1c04.inc"
};
static NJS_MKEY_F dat_0cbb1c24[2] = {
#include "ev006d/amy_action_9/dat_0cbb1c24.inc"
};
static NJS_MKEY_F dat_0cbb1c44[2] = {
#include "ev006d/amy_action_9/dat_0cbb1c44.inc"
};
static NJS_MKEY_F dat_0cbb1c64[2] = {
#include "ev006d/amy_action_9/dat_0cbb1c64.inc"
};
static NJS_MKEY_F dat_0cbb1c84[2] = {
#include "ev006d/amy_action_9/dat_0cbb1c84.inc"
};
static NJS_MKEY_A dat_0cbb1ca4[2] = {
#include "ev006d/amy_action_9/dat_0cbb1ca4.inc"
};
static NJS_MKEY_F dat_0cbb1cc4[2] = {
#include "ev006d/amy_action_9/dat_0cbb1cc4.inc"
};
static NJS_MKEY_A dat_0cbb1ce4[30] = {
#include "ev006d/amy_action_9/dat_0cbb1ce4.inc"
};
static NJS_MKEY_F dat_0cbb1ec4[2] = {
#include "ev006d/amy_action_9/dat_0cbb1ec4.inc"
};
static NJS_MKEY_F dat_0cbb1ee4[2] = {
#include "ev006d/amy_action_9/dat_0cbb1ee4.inc"
};
static NJS_MKEY_F dat_0cbb1f04[2] = {
#include "ev006d/amy_action_9/dat_0cbb1f04.inc"
};
static NJS_MKEY_A dat_0cbb1f24[2] = {
#include "ev006d/amy_action_9/dat_0cbb1f24.inc"
};
static NJS_MKEY_F dat_0cbb1f44[2] = {
#include "ev006d/amy_action_9/dat_0cbb1f44.inc"
};
static NJS_MKEY_A dat_0cbb1f64[30] = {
#include "ev006d/amy_action_9/dat_0cbb1f64.inc"
};
static NJS_MKEY_F dat_0cbb2144[2] = {
#include "ev006d/amy_action_9/dat_0cbb2144.inc"
};
static NJS_MKEY_F dat_0cbb2164[2] = {
#include "ev006d/amy_action_9/dat_0cbb2164.inc"
};
static NJS_MKEY_F dat_0cbb2184[2] = {
#include "ev006d/amy_action_9/dat_0cbb2184.inc"
};
static NJS_MKEY_F dat_0cbb21a4[2] = {
#include "ev006d/amy_action_9/dat_0cbb21a4.inc"
};
static NJS_MKEY_F dat_0cbb21c4[2] = {
#include "ev006d/amy_action_9/dat_0cbb21c4.inc"
};
static NJS_MDATA3 ev006d_bird_motion_0_mdata[57] = {
#include "ev006d/amy_action_9/ev006d_bird_motion_0_mdata.inc"
};
static NJS_MOTION ev006d_bird_motion_0 = {
#include "ev006d/amy_action_9/ev006d_bird_motion_0.inc"
};
NJS_ACTION ev006d_bird_action_0[1] = {
#include "ev006d/amy_action_9/ev006d_bird_action_0.inc"
};
static NJS_MKEY_F dat_0cbb2750[2] = {
#include "ev006d/amy_action_9/dat_0cbb2750.inc"
};
static NJS_MKEY_A dat_0cbb2770[2] = {
#include "ev006d/amy_action_9/dat_0cbb2770.inc"
};
static NJS_MKEY_F dat_0cbb2790[32] = {
#include "ev006d/amy_action_9/dat_0cbb2790.inc"
};
static NJS_MKEY_A dat_0cbb2990[32] = {
#include "ev006d/amy_action_9/dat_0cbb2990.inc"
};
static NJS_MKEY_F dat_0cbb2b90[2] = {
#include "ev006d/amy_action_9/dat_0cbb2b90.inc"
};
static NJS_MKEY_A dat_0cbb2bb0[2] = {
#include "ev006d/amy_action_9/dat_0cbb2bb0.inc"
};
static NJS_MKEY_F dat_0cbb2bd0[2] = {
#include "ev006d/amy_action_9/dat_0cbb2bd0.inc"
};
static NJS_MKEY_A dat_0cbb2bf0[2] = {
#include "ev006d/amy_action_9/dat_0cbb2bf0.inc"
};
static NJS_MKEY_F dat_0cbb2c10[2] = {
#include "ev006d/amy_action_9/dat_0cbb2c10.inc"
};
static NJS_MKEY_A dat_0cbb2c30[2] = {
#include "ev006d/amy_action_9/dat_0cbb2c30.inc"
};
static NJS_MKEY_F dat_0cbb2c50[2] = {
#include "ev006d/amy_action_9/dat_0cbb2c50.inc"
};
static NJS_MKEY_A dat_0cbb2c70[2] = {
#include "ev006d/amy_action_9/dat_0cbb2c70.inc"
};
static NJS_MKEY_F dat_0cbb2c90[2] = {
#include "ev006d/amy_action_9/dat_0cbb2c90.inc"
};
static NJS_MKEY_A dat_0cbb2cb0[32] = {
#include "ev006d/amy_action_9/dat_0cbb2cb0.inc"
};
static NJS_MKEY_F dat_0cbb2eb0[2] = {
#include "ev006d/amy_action_9/dat_0cbb2eb0.inc"
};
static NJS_MKEY_A dat_0cbb2ed0[2] = {
#include "ev006d/amy_action_9/dat_0cbb2ed0.inc"
};
static NJS_MKEY_F dat_0cbb2ef0[2] = {
#include "ev006d/amy_action_9/dat_0cbb2ef0.inc"
};
static NJS_MKEY_A dat_0cbb2f10[2] = {
#include "ev006d/amy_action_9/dat_0cbb2f10.inc"
};
static NJS_MKEY_F dat_0cbb2f30[2] = {
#include "ev006d/amy_action_9/dat_0cbb2f30.inc"
};
static NJS_MKEY_A dat_0cbb2f50[2] = {
#include "ev006d/amy_action_9/dat_0cbb2f50.inc"
};
static NJS_MKEY_F dat_0cbb2f70[2] = {
#include "ev006d/amy_action_9/dat_0cbb2f70.inc"
};
static NJS_MKEY_A dat_0cbb2f90[32] = {
#include "ev006d/amy_action_9/dat_0cbb2f90.inc"
};
static NJS_MKEY_F dat_0cbb3190[2] = {
#include "ev006d/amy_action_9/dat_0cbb3190.inc"
};
static NJS_MKEY_A dat_0cbb31b0[2] = {
#include "ev006d/amy_action_9/dat_0cbb31b0.inc"
};
static NJS_MKEY_F dat_0cbb31d0[2] = {
#include "ev006d/amy_action_9/dat_0cbb31d0.inc"
};
static NJS_MKEY_A dat_0cbb31f0[2] = {
#include "ev006d/amy_action_9/dat_0cbb31f0.inc"
};
static NJS_MKEY_F dat_0cbb3210[2] = {
#include "ev006d/amy_action_9/dat_0cbb3210.inc"
};
static NJS_MKEY_A dat_0cbb3230[2] = {
#include "ev006d/amy_action_9/dat_0cbb3230.inc"
};
static NJS_MKEY_F dat_0cbb3250[2] = {
#include "ev006d/amy_action_9/dat_0cbb3250.inc"
};
static NJS_MKEY_A dat_0cbb3270[2] = {
#include "ev006d/amy_action_9/dat_0cbb3270.inc"
};
static NJS_MKEY_F dat_0cbb3290[2] = {
#include "ev006d/amy_action_9/dat_0cbb3290.inc"
};
static NJS_MKEY_A dat_0cbb32b0[2] = {
#include "ev006d/amy_action_9/dat_0cbb32b0.inc"
};
static NJS_MKEY_F dat_0cbb32d0[2] = {
#include "ev006d/amy_action_9/dat_0cbb32d0.inc"
};
static NJS_MKEY_A dat_0cbb32f0[2] = {
#include "ev006d/amy_action_9/dat_0cbb32f0.inc"
};
static NJS_MKEY_F dat_0cbb3310[2] = {
#include "ev006d/amy_action_9/dat_0cbb3310.inc"
};
static NJS_MKEY_A dat_0cbb3330[2] = {
#include "ev006d/amy_action_9/dat_0cbb3330.inc"
};
static NJS_MKEY_F dat_0cbb3350[2] = {
#include "ev006d/amy_action_9/dat_0cbb3350.inc"
};
static NJS_MKEY_A dat_0cbb3370[2] = {
#include "ev006d/amy_action_9/dat_0cbb3370.inc"
};
static NJS_MKEY_F dat_0cbb3390[2] = {
#include "ev006d/amy_action_9/dat_0cbb3390.inc"
};
static NJS_MKEY_A dat_0cbb33b0[2] = {
#include "ev006d/amy_action_9/dat_0cbb33b0.inc"
};
static NJS_MKEY_F dat_0cbb33d0[2] = {
#include "ev006d/amy_action_9/dat_0cbb33d0.inc"
};
static NJS_MKEY_A dat_0cbb33f0[2] = {
#include "ev006d/amy_action_9/dat_0cbb33f0.inc"
};
static NJS_MKEY_F dat_0cbb3410[2] = {
#include "ev006d/amy_action_9/dat_0cbb3410.inc"
};
static NJS_MKEY_A dat_0cbb3430[2] = {
#include "ev006d/amy_action_9/dat_0cbb3430.inc"
};
static NJS_MKEY_F dat_0cbb3450[2] = {
#include "ev006d/amy_action_9/dat_0cbb3450.inc"
};
static NJS_MKEY_A dat_0cbb3470[2] = {
#include "ev006d/amy_action_9/dat_0cbb3470.inc"
};
static NJS_MKEY_F dat_0cbb3490[2] = {
#include "ev006d/amy_action_9/dat_0cbb3490.inc"
};
static NJS_MKEY_A dat_0cbb34b0[2] = {
#include "ev006d/amy_action_9/dat_0cbb34b0.inc"
};
static NJS_MKEY_F dat_0cbb34d0[2] = {
#include "ev006d/amy_action_9/dat_0cbb34d0.inc"
};
static NJS_MKEY_A dat_0cbb34f0[2] = {
#include "ev006d/amy_action_9/dat_0cbb34f0.inc"
};
static NJS_MKEY_F dat_0cbb3510[2] = {
#include "ev006d/amy_action_9/dat_0cbb3510.inc"
};
static NJS_MKEY_A dat_0cbb3530[32] = {
#include "ev006d/amy_action_9/dat_0cbb3530.inc"
};
static NJS_MKEY_F dat_0cbb3730[2] = {
#include "ev006d/amy_action_9/dat_0cbb3730.inc"
};
static NJS_MKEY_A dat_0cbb3750[2] = {
#include "ev006d/amy_action_9/dat_0cbb3750.inc"
};
static NJS_MKEY_F dat_0cbb3770[2] = {
#include "ev006d/amy_action_9/dat_0cbb3770.inc"
};
static NJS_MKEY_A dat_0cbb3790[2] = {
#include "ev006d/amy_action_9/dat_0cbb3790.inc"
};
static NJS_MKEY_F dat_0cbb37b0[2] = {
#include "ev006d/amy_action_9/dat_0cbb37b0.inc"
};
static NJS_MKEY_A dat_0cbb37d0[2] = {
#include "ev006d/amy_action_9/dat_0cbb37d0.inc"
};
static NJS_MKEY_F dat_0cbb37f0[2] = {
#include "ev006d/amy_action_9/dat_0cbb37f0.inc"
};
static NJS_MKEY_A dat_0cbb3810[2] = {
#include "ev006d/amy_action_9/dat_0cbb3810.inc"
};
static NJS_MKEY_F dat_0cbb3830[2] = {
#include "ev006d/amy_action_9/dat_0cbb3830.inc"
};
static NJS_MKEY_A dat_0cbb3850[2] = {
#include "ev006d/amy_action_9/dat_0cbb3850.inc"
};
static NJS_MKEY_F dat_0cbb3870[2] = {
#include "ev006d/amy_action_9/dat_0cbb3870.inc"
};
static NJS_MKEY_A dat_0cbb3890[2] = {
#include "ev006d/amy_action_9/dat_0cbb3890.inc"
};
static NJS_MKEY_F dat_0cbb38b0[2] = {
#include "ev006d/amy_action_9/dat_0cbb38b0.inc"
};
static NJS_MKEY_A dat_0cbb38d0[2] = {
#include "ev006d/amy_action_9/dat_0cbb38d0.inc"
};
static NJS_MKEY_F dat_0cbb38f0[2] = {
#include "ev006d/amy_action_9/dat_0cbb38f0.inc"
};
static NJS_MKEY_A dat_0cbb3910[32] = {
#include "ev006d/amy_action_9/dat_0cbb3910.inc"
};
static NJS_MKEY_F dat_0cbb3b10[2] = {
#include "ev006d/amy_action_9/dat_0cbb3b10.inc"
};
static NJS_MKEY_A dat_0cbb3b30[2] = {
#include "ev006d/amy_action_9/dat_0cbb3b30.inc"
};
static NJS_MKEY_F dat_0cbb3b50[2] = {
#include "ev006d/amy_action_9/dat_0cbb3b50.inc"
};
static NJS_MKEY_A dat_0cbb3b70[2] = {
#include "ev006d/amy_action_9/dat_0cbb3b70.inc"
};
static NJS_MKEY_F dat_0cbb3b90[32] = {
#include "ev006d/amy_action_9/dat_0cbb3b90.inc"
};
static NJS_MKEY_A dat_0cbb3d90[2] = {
#include "ev006d/amy_action_9/dat_0cbb3d90.inc"
};
static NJS_MKEY_F dat_0cbb3db0[32] = {
#include "ev006d/amy_action_9/dat_0cbb3db0.inc"
};
static NJS_MKEY_A dat_0cbb3fb0[2] = {
#include "ev006d/amy_action_9/dat_0cbb3fb0.inc"
};
static NJS_MKEY_F dat_0cbb3fd0[2] = {
#include "ev006d/amy_action_9/dat_0cbb3fd0.inc"
};
static NJS_MKEY_A dat_0cbb3ff0[2] = {
#include "ev006d/amy_action_9/dat_0cbb3ff0.inc"
};
static NJS_MKEY_F dat_0cbb4010[32] = {
#include "ev006d/amy_action_9/dat_0cbb4010.inc"
};
static NJS_MKEY_A dat_0cbb4210[2] = {
#include "ev006d/amy_action_9/dat_0cbb4210.inc"
};
static NJS_MKEY_F dat_0cbb4230[32] = {
#include "ev006d/amy_action_9/dat_0cbb4230.inc"
};
static NJS_MKEY_A dat_0cbb4430[2] = {
#include "ev006d/amy_action_9/dat_0cbb4430.inc"
};
static NJS_MKEY_F dat_0cbb4450[2] = {
#include "ev006d/amy_action_9/dat_0cbb4450.inc"
};
static NJS_MKEY_A dat_0cbb4470[2] = {
#include "ev006d/amy_action_9/dat_0cbb4470.inc"
};
static NJS_MKEY_F dat_0cbb4490[2] = {
#include "ev006d/amy_action_9/dat_0cbb4490.inc"
};
static NJS_MKEY_A dat_0cbb44b0[2] = {
#include "ev006d/amy_action_9/dat_0cbb44b0.inc"
};
static NJS_MKEY_F dat_0cbb44d0[2] = {
#include "ev006d/amy_action_9/dat_0cbb44d0.inc"
};
static NJS_MKEY_A dat_0cbb44f0[2] = {
#include "ev006d/amy_action_9/dat_0cbb44f0.inc"
};
static NJS_MKEY_F dat_0cbb4510[2] = {
#include "ev006d/amy_action_9/dat_0cbb4510.inc"
};
static NJS_MKEY_A dat_0cbb4530[2] = {
#include "ev006d/amy_action_9/dat_0cbb4530.inc"
};
static NJS_MKEY_F dat_0cbb4550[2] = {
#include "ev006d/amy_action_9/dat_0cbb4550.inc"
};
static NJS_MKEY_A dat_0cbb4570[2] = {
#include "ev006d/amy_action_9/dat_0cbb4570.inc"
};
static NJS_MKEY_F dat_0cbb4590[2] = {
#include "ev006d/amy_action_9/dat_0cbb4590.inc"
};
static NJS_MKEY_A dat_0cbb45b0[2] = {
#include "ev006d/amy_action_9/dat_0cbb45b0.inc"
};
static NJS_MKEY_F dat_0cbb45d0[2] = {
#include "ev006d/amy_action_9/dat_0cbb45d0.inc"
};
static NJS_MKEY_A dat_0cbb45f0[2] = {
#include "ev006d/amy_action_9/dat_0cbb45f0.inc"
};
static NJS_MKEY_F dat_0cbb4610[2] = {
#include "ev006d/amy_action_9/dat_0cbb4610.inc"
};
static NJS_MKEY_A dat_0cbb4630[2] = {
#include "ev006d/amy_action_9/dat_0cbb4630.inc"
};
static NJS_MKEY_F dat_0cbb4650[2] = {
#include "ev006d/amy_action_9/dat_0cbb4650.inc"
};
static NJS_MKEY_A dat_0cbb4670[2] = {
#include "ev006d/amy_action_9/dat_0cbb4670.inc"
};
static NJS_MKEY_F dat_0cbb4690[2] = {
#include "ev006d/amy_action_9/dat_0cbb4690.inc"
};
static NJS_MKEY_A dat_0cbb46b0[2] = {
#include "ev006d/amy_action_9/dat_0cbb46b0.inc"
};
static NJS_MKEY_F dat_0cbb46d0[2] = {
#include "ev006d/amy_action_9/dat_0cbb46d0.inc"
};
static NJS_MKEY_A dat_0cbb46f0[2] = {
#include "ev006d/amy_action_9/dat_0cbb46f0.inc"
};
static NJS_MKEY_F dat_0cbb4710[2] = {
#include "ev006d/amy_action_9/dat_0cbb4710.inc"
};
static NJS_MKEY_A dat_0cbb4730[2] = {
#include "ev006d/amy_action_9/dat_0cbb4730.inc"
};
static NJS_MKEY_F dat_0cbb4750[2] = {
#include "ev006d/amy_action_9/dat_0cbb4750.inc"
};
static NJS_MKEY_A dat_0cbb4770[2] = {
#include "ev006d/amy_action_9/dat_0cbb4770.inc"
};
static NJS_MKEY_F dat_0cbb4790[2] = {
#include "ev006d/amy_action_9/dat_0cbb4790.inc"
};
static NJS_MKEY_A dat_0cbb47b0[2] = {
#include "ev006d/amy_action_9/dat_0cbb47b0.inc"
};
static NJS_MKEY_F dat_0cbb47d0[2] = {
#include "ev006d/amy_action_9/dat_0cbb47d0.inc"
};
static NJS_MKEY_A dat_0cbb47f0[2] = {
#include "ev006d/amy_action_9/dat_0cbb47f0.inc"
};
static NJS_MKEY_F dat_0cbb4810[2] = {
#include "ev006d/amy_action_9/dat_0cbb4810.inc"
};
static NJS_MKEY_A dat_0cbb4830[2] = {
#include "ev006d/amy_action_9/dat_0cbb4830.inc"
};
static NJS_MDATA2 dat_0cbb4850[57] = {
#include "ev006d/amy_action_9/dat_0cbb4850.inc"
};
static NJS_MOTION dat_0cbb4be0 = {
#include "ev006d/amy_action_9/dat_0cbb4be0.inc"
};
static NJS_ACTION dat_0cbb4bec = {
#include "ev006d/amy_action_9/dat_0cbb4bec.inc"
};
static NJS_MKEY_F dat_0cbb4bf4[2] = {
#include "ev006d/amy_action_9/dat_0cbb4bf4.inc"
};
static NJS_MKEY_A dat_0cbb4c14[2] = {
#include "ev006d/amy_action_9/dat_0cbb4c14.inc"
};
static NJS_MKEY_F dat_0cbb4c34[32] = {
#include "ev006d/amy_action_9/dat_0cbb4c34.inc"
};
static NJS_MKEY_A dat_0cbb4e34[32] = {
#include "ev006d/amy_action_9/dat_0cbb4e34.inc"
};
static NJS_MKEY_F dat_0cbb5034[2] = {
#include "ev006d/amy_action_9/dat_0cbb5034.inc"
};
static NJS_MKEY_A dat_0cbb5054[2] = {
#include "ev006d/amy_action_9/dat_0cbb5054.inc"
};
static NJS_MKEY_F dat_0cbb5074[2] = {
#include "ev006d/amy_action_9/dat_0cbb5074.inc"
};
static NJS_MKEY_A dat_0cbb5094[2] = {
#include "ev006d/amy_action_9/dat_0cbb5094.inc"
};
static NJS_MKEY_F dat_0cbb50b4[2] = {
#include "ev006d/amy_action_9/dat_0cbb50b4.inc"
};
static NJS_MKEY_A dat_0cbb50d4[2] = {
#include "ev006d/amy_action_9/dat_0cbb50d4.inc"
};
static NJS_MKEY_F dat_0cbb50f4[2] = {
#include "ev006d/amy_action_9/dat_0cbb50f4.inc"
};
static NJS_MKEY_A dat_0cbb5114[2] = {
#include "ev006d/amy_action_9/dat_0cbb5114.inc"
};
static NJS_MKEY_F dat_0cbb5134[2] = {
#include "ev006d/amy_action_9/dat_0cbb5134.inc"
};
static NJS_MKEY_A dat_0cbb5154[32] = {
#include "ev006d/amy_action_9/dat_0cbb5154.inc"
};
static NJS_MKEY_F dat_0cbb5354[2] = {
#include "ev006d/amy_action_9/dat_0cbb5354.inc"
};
static NJS_MKEY_A dat_0cbb5374[2] = {
#include "ev006d/amy_action_9/dat_0cbb5374.inc"
};
static NJS_MKEY_F dat_0cbb5394[2] = {
#include "ev006d/amy_action_9/dat_0cbb5394.inc"
};
static NJS_MKEY_A dat_0cbb53b4[2] = {
#include "ev006d/amy_action_9/dat_0cbb53b4.inc"
};
static NJS_MKEY_F dat_0cbb53d4[2] = {
#include "ev006d/amy_action_9/dat_0cbb53d4.inc"
};
static NJS_MKEY_A dat_0cbb53f4[2] = {
#include "ev006d/amy_action_9/dat_0cbb53f4.inc"
};
static NJS_MKEY_F dat_0cbb5414[2] = {
#include "ev006d/amy_action_9/dat_0cbb5414.inc"
};
static NJS_MKEY_A dat_0cbb5434[32] = {
#include "ev006d/amy_action_9/dat_0cbb5434.inc"
};
static NJS_MKEY_F dat_0cbb5634[2] = {
#include "ev006d/amy_action_9/dat_0cbb5634.inc"
};
static NJS_MKEY_A dat_0cbb5654[2] = {
#include "ev006d/amy_action_9/dat_0cbb5654.inc"
};
static NJS_MKEY_F dat_0cbb5674[2] = {
#include "ev006d/amy_action_9/dat_0cbb5674.inc"
};
static NJS_MKEY_A dat_0cbb5694[2] = {
#include "ev006d/amy_action_9/dat_0cbb5694.inc"
};
static NJS_MKEY_F dat_0cbb56b4[2] = {
#include "ev006d/amy_action_9/dat_0cbb56b4.inc"
};
static NJS_MKEY_A dat_0cbb56d4[2] = {
#include "ev006d/amy_action_9/dat_0cbb56d4.inc"
};
static NJS_MKEY_F dat_0cbb56f4[2] = {
#include "ev006d/amy_action_9/dat_0cbb56f4.inc"
};
static NJS_MKEY_A dat_0cbb5714[2] = {
#include "ev006d/amy_action_9/dat_0cbb5714.inc"
};
static NJS_MKEY_F dat_0cbb5734[2] = {
#include "ev006d/amy_action_9/dat_0cbb5734.inc"
};
static NJS_MKEY_A dat_0cbb5754[2] = {
#include "ev006d/amy_action_9/dat_0cbb5754.inc"
};
static NJS_MKEY_F dat_0cbb5774[2] = {
#include "ev006d/amy_action_9/dat_0cbb5774.inc"
};
static NJS_MKEY_A dat_0cbb5794[2] = {
#include "ev006d/amy_action_9/dat_0cbb5794.inc"
};
static NJS_MKEY_F dat_0cbb57b4[2] = {
#include "ev006d/amy_action_9/dat_0cbb57b4.inc"
};
static NJS_MKEY_A dat_0cbb57d4[2] = {
#include "ev006d/amy_action_9/dat_0cbb57d4.inc"
};
static NJS_MKEY_F dat_0cbb57f4[2] = {
#include "ev006d/amy_action_9/dat_0cbb57f4.inc"
};
static NJS_MKEY_A dat_0cbb5814[2] = {
#include "ev006d/amy_action_9/dat_0cbb5814.inc"
};
static NJS_MKEY_F dat_0cbb5834[2] = {
#include "ev006d/amy_action_9/dat_0cbb5834.inc"
};
static NJS_MKEY_A dat_0cbb5854[2] = {
#include "ev006d/amy_action_9/dat_0cbb5854.inc"
};
static NJS_MKEY_F dat_0cbb5874[2] = {
#include "ev006d/amy_action_9/dat_0cbb5874.inc"
};
static NJS_MKEY_A dat_0cbb5894[2] = {
#include "ev006d/amy_action_9/dat_0cbb5894.inc"
};
static NJS_MKEY_F dat_0cbb58b4[2] = {
#include "ev006d/amy_action_9/dat_0cbb58b4.inc"
};
static NJS_MKEY_A dat_0cbb58d4[2] = {
#include "ev006d/amy_action_9/dat_0cbb58d4.inc"
};
static NJS_MKEY_F dat_0cbb58f4[2] = {
#include "ev006d/amy_action_9/dat_0cbb58f4.inc"
};
static NJS_MKEY_A dat_0cbb5914[2] = {
#include "ev006d/amy_action_9/dat_0cbb5914.inc"
};
static NJS_MKEY_F dat_0cbb5934[2] = {
#include "ev006d/amy_action_9/dat_0cbb5934.inc"
};
static NJS_MKEY_A dat_0cbb5954[2] = {
#include "ev006d/amy_action_9/dat_0cbb5954.inc"
};
static NJS_MKEY_F dat_0cbb5974[2] = {
#include "ev006d/amy_action_9/dat_0cbb5974.inc"
};
static NJS_MKEY_A dat_0cbb5994[2] = {
#include "ev006d/amy_action_9/dat_0cbb5994.inc"
};
static NJS_MKEY_F dat_0cbb59b4[2] = {
#include "ev006d/amy_action_9/dat_0cbb59b4.inc"
};
static NJS_MKEY_A dat_0cbb59d4[32] = {
#include "ev006d/amy_action_9/dat_0cbb59d4.inc"
};
static NJS_MKEY_F dat_0cbb5bd4[2] = {
#include "ev006d/amy_action_9/dat_0cbb5bd4.inc"
};
static NJS_MKEY_A dat_0cbb5bf4[2] = {
#include "ev006d/amy_action_9/dat_0cbb5bf4.inc"
};
static NJS_MKEY_F dat_0cbb5c14[2] = {
#include "ev006d/amy_action_9/dat_0cbb5c14.inc"
};
static NJS_MKEY_A dat_0cbb5c34[2] = {
#include "ev006d/amy_action_9/dat_0cbb5c34.inc"
};
static NJS_MKEY_F dat_0cbb5c54[2] = {
#include "ev006d/amy_action_9/dat_0cbb5c54.inc"
};
static NJS_MKEY_A dat_0cbb5c74[2] = {
#include "ev006d/amy_action_9/dat_0cbb5c74.inc"
};
static NJS_MKEY_F dat_0cbb5c94[2] = {
#include "ev006d/amy_action_9/dat_0cbb5c94.inc"
};
static NJS_MKEY_A dat_0cbb5cb4[2] = {
#include "ev006d/amy_action_9/dat_0cbb5cb4.inc"
};
static NJS_MKEY_F dat_0cbb5cd4[2] = {
#include "ev006d/amy_action_9/dat_0cbb5cd4.inc"
};
static NJS_MKEY_A dat_0cbb5cf4[2] = {
#include "ev006d/amy_action_9/dat_0cbb5cf4.inc"
};
static NJS_MKEY_F dat_0cbb5d14[2] = {
#include "ev006d/amy_action_9/dat_0cbb5d14.inc"
};
static NJS_MKEY_A dat_0cbb5d34[2] = {
#include "ev006d/amy_action_9/dat_0cbb5d34.inc"
};
static NJS_MKEY_F dat_0cbb5d54[2] = {
#include "ev006d/amy_action_9/dat_0cbb5d54.inc"
};
static NJS_MKEY_A dat_0cbb5d74[2] = {
#include "ev006d/amy_action_9/dat_0cbb5d74.inc"
};
static NJS_MKEY_F dat_0cbb5d94[2] = {
#include "ev006d/amy_action_9/dat_0cbb5d94.inc"
};
static NJS_MKEY_A dat_0cbb5db4[32] = {
#include "ev006d/amy_action_9/dat_0cbb5db4.inc"
};
static NJS_MKEY_F dat_0cbb5fb4[2] = {
#include "ev006d/amy_action_9/dat_0cbb5fb4.inc"
};
static NJS_MKEY_A dat_0cbb5fd4[2] = {
#include "ev006d/amy_action_9/dat_0cbb5fd4.inc"
};
static NJS_MKEY_F dat_0cbb5ff4[2] = {
#include "ev006d/amy_action_9/dat_0cbb5ff4.inc"
};
static NJS_MKEY_A dat_0cbb6014[2] = {
#include "ev006d/amy_action_9/dat_0cbb6014.inc"
};
static NJS_MKEY_F dat_0cbb6034[32] = {
#include "ev006d/amy_action_9/dat_0cbb6034.inc"
};
static NJS_MKEY_A dat_0cbb6234[2] = {
#include "ev006d/amy_action_9/dat_0cbb6234.inc"
};
static NJS_MKEY_F dat_0cbb6254[32] = {
#include "ev006d/amy_action_9/dat_0cbb6254.inc"
};
static NJS_MKEY_A dat_0cbb6454[2] = {
#include "ev006d/amy_action_9/dat_0cbb6454.inc"
};
static NJS_MKEY_F dat_0cbb6474[2] = {
#include "ev006d/amy_action_9/dat_0cbb6474.inc"
};
static NJS_MKEY_A dat_0cbb6494[2] = {
#include "ev006d/amy_action_9/dat_0cbb6494.inc"
};
static NJS_MKEY_F dat_0cbb64b4[32] = {
#include "ev006d/amy_action_9/dat_0cbb64b4.inc"
};
static NJS_MKEY_A dat_0cbb66b4[2] = {
#include "ev006d/amy_action_9/dat_0cbb66b4.inc"
};
static NJS_MKEY_F dat_0cbb66d4[32] = {
#include "ev006d/amy_action_9/dat_0cbb66d4.inc"
};
static NJS_MKEY_A dat_0cbb68d4[2] = {
#include "ev006d/amy_action_9/dat_0cbb68d4.inc"
};
static NJS_MKEY_F dat_0cbb68f4[2] = {
#include "ev006d/amy_action_9/dat_0cbb68f4.inc"
};
static NJS_MKEY_A dat_0cbb6914[2] = {
#include "ev006d/amy_action_9/dat_0cbb6914.inc"
};
static NJS_MKEY_F dat_0cbb6934[2] = {
#include "ev006d/amy_action_9/dat_0cbb6934.inc"
};
static NJS_MKEY_A dat_0cbb6954[2] = {
#include "ev006d/amy_action_9/dat_0cbb6954.inc"
};
static NJS_MKEY_F dat_0cbb6974[2] = {
#include "ev006d/amy_action_9/dat_0cbb6974.inc"
};
static NJS_MKEY_A dat_0cbb6994[2] = {
#include "ev006d/amy_action_9/dat_0cbb6994.inc"
};
static NJS_MKEY_F dat_0cbb69b4[2] = {
#include "ev006d/amy_action_9/dat_0cbb69b4.inc"
};
static NJS_MKEY_A dat_0cbb69d4[2] = {
#include "ev006d/amy_action_9/dat_0cbb69d4.inc"
};
static NJS_MKEY_F dat_0cbb69f4[2] = {
#include "ev006d/amy_action_9/dat_0cbb69f4.inc"
};
static NJS_MKEY_A dat_0cbb6a14[2] = {
#include "ev006d/amy_action_9/dat_0cbb6a14.inc"
};
static NJS_MKEY_F dat_0cbb6a34[2] = {
#include "ev006d/amy_action_9/dat_0cbb6a34.inc"
};
static NJS_MKEY_A dat_0cbb6a54[2] = {
#include "ev006d/amy_action_9/dat_0cbb6a54.inc"
};
static NJS_MKEY_F dat_0cbb6a74[2] = {
#include "ev006d/amy_action_9/dat_0cbb6a74.inc"
};
static NJS_MKEY_A dat_0cbb6a94[2] = {
#include "ev006d/amy_action_9/dat_0cbb6a94.inc"
};
static NJS_MKEY_F dat_0cbb6ab4[2] = {
#include "ev006d/amy_action_9/dat_0cbb6ab4.inc"
};
static NJS_MKEY_A dat_0cbb6ad4[2] = {
#include "ev006d/amy_action_9/dat_0cbb6ad4.inc"
};
static NJS_MKEY_F dat_0cbb6af4[2] = {
#include "ev006d/amy_action_9/dat_0cbb6af4.inc"
};
static NJS_MKEY_A dat_0cbb6b14[2] = {
#include "ev006d/amy_action_9/dat_0cbb6b14.inc"
};
static NJS_MKEY_F dat_0cbb6b34[2] = {
#include "ev006d/amy_action_9/dat_0cbb6b34.inc"
};
static NJS_MKEY_A dat_0cbb6b54[2] = {
#include "ev006d/amy_action_9/dat_0cbb6b54.inc"
};
static NJS_MKEY_F dat_0cbb6b74[2] = {
#include "ev006d/amy_action_9/dat_0cbb6b74.inc"
};
static NJS_MKEY_A dat_0cbb6b94[2] = {
#include "ev006d/amy_action_9/dat_0cbb6b94.inc"
};
static NJS_MKEY_F dat_0cbb6bb4[2] = {
#include "ev006d/amy_action_9/dat_0cbb6bb4.inc"
};
static NJS_MKEY_A dat_0cbb6bd4[2] = {
#include "ev006d/amy_action_9/dat_0cbb6bd4.inc"
};
static NJS_MKEY_F dat_0cbb6bf4[2] = {
#include "ev006d/amy_action_9/dat_0cbb6bf4.inc"
};
static NJS_MKEY_A dat_0cbb6c14[2] = {
#include "ev006d/amy_action_9/dat_0cbb6c14.inc"
};
static NJS_MKEY_F dat_0cbb6c34[2] = {
#include "ev006d/amy_action_9/dat_0cbb6c34.inc"
};
static NJS_MKEY_A dat_0cbb6c54[2] = {
#include "ev006d/amy_action_9/dat_0cbb6c54.inc"
};
static NJS_MKEY_F dat_0cbb6c74[2] = {
#include "ev006d/amy_action_9/dat_0cbb6c74.inc"
};
static NJS_MKEY_A dat_0cbb6c94[2] = {
#include "ev006d/amy_action_9/dat_0cbb6c94.inc"
};
static NJS_MKEY_F dat_0cbb6cb4[2] = {
#include "ev006d/amy_action_9/dat_0cbb6cb4.inc"
};
static NJS_MKEY_A dat_0cbb6cd4[2] = {
#include "ev006d/amy_action_9/dat_0cbb6cd4.inc"
};
static NJS_MDATA2 ev006d_bird_motion_1_mdata[57] = {
#include "ev006d/amy_action_9/ev006d_bird_motion_1_mdata.inc"
};
static NJS_MOTION ev006d_bird_motion_1 = {
#include "ev006d/amy_action_9/ev006d_bird_motion_1.inc"
};
NJS_ACTION ev006d_bird_action_1 = {
#include "ev006d/amy_action_9/ev006d_bird_action_1.inc"
};
static NJS_MKEY_F dat_0cbb7098[2] = {
#include "ev006d/amy_action_9/dat_0cbb7098.inc"
};
static NJS_MKEY_A dat_0cbb70b8[2] = {
#include "ev006d/amy_action_9/dat_0cbb70b8.inc"
};
static NJS_MKEY_F dat_0cbb70d8[30] = {
#include "ev006d/amy_action_9/dat_0cbb70d8.inc"
};
static NJS_MKEY_A dat_0cbb72b8[30] = {
#include "ev006d/amy_action_9/dat_0cbb72b8.inc"
};
static NJS_MKEY_F dat_0cbb7498[2] = {
#include "ev006d/amy_action_9/dat_0cbb7498.inc"
};
static NJS_MKEY_A dat_0cbb74b8[2] = {
#include "ev006d/amy_action_9/dat_0cbb74b8.inc"
};
static NJS_MKEY_F dat_0cbb74d8[2] = {
#include "ev006d/amy_action_9/dat_0cbb74d8.inc"
};
static NJS_MKEY_A dat_0cbb74f8[2] = {
#include "ev006d/amy_action_9/dat_0cbb74f8.inc"
};
static NJS_MKEY_F dat_0cbb7518[2] = {
#include "ev006d/amy_action_9/dat_0cbb7518.inc"
};
static NJS_MKEY_A dat_0cbb7538[2] = {
#include "ev006d/amy_action_9/dat_0cbb7538.inc"
};
static NJS_MKEY_F dat_0cbb7558[2] = {
#include "ev006d/amy_action_9/dat_0cbb7558.inc"
};
static NJS_MKEY_A dat_0cbb7578[2] = {
#include "ev006d/amy_action_9/dat_0cbb7578.inc"
};
static NJS_MKEY_F dat_0cbb7598[2] = {
#include "ev006d/amy_action_9/dat_0cbb7598.inc"
};
static NJS_MKEY_A dat_0cbb75b8[30] = {
#include "ev006d/amy_action_9/dat_0cbb75b8.inc"
};
static NJS_MKEY_F dat_0cbb7798[2] = {
#include "ev006d/amy_action_9/dat_0cbb7798.inc"
};
static NJS_MKEY_A dat_0cbb77b8[2] = {
#include "ev006d/amy_action_9/dat_0cbb77b8.inc"
};
static NJS_MKEY_F dat_0cbb77d8[2] = {
#include "ev006d/amy_action_9/dat_0cbb77d8.inc"
};
static NJS_MKEY_A dat_0cbb77f8[2] = {
#include "ev006d/amy_action_9/dat_0cbb77f8.inc"
};
static NJS_MKEY_F dat_0cbb7818[2] = {
#include "ev006d/amy_action_9/dat_0cbb7818.inc"
};
static NJS_MKEY_A dat_0cbb7838[2] = {
#include "ev006d/amy_action_9/dat_0cbb7838.inc"
};
static NJS_MKEY_F dat_0cbb7858[2] = {
#include "ev006d/amy_action_9/dat_0cbb7858.inc"
};
static NJS_MKEY_A dat_0cbb7878[30] = {
#include "ev006d/amy_action_9/dat_0cbb7878.inc"
};
static NJS_MKEY_F dat_0cbb7a58[2] = {
#include "ev006d/amy_action_9/dat_0cbb7a58.inc"
};
static NJS_MKEY_A dat_0cbb7a78[2] = {
#include "ev006d/amy_action_9/dat_0cbb7a78.inc"
};
static NJS_MKEY_F dat_0cbb7a98[2] = {
#include "ev006d/amy_action_9/dat_0cbb7a98.inc"
};
static NJS_MKEY_A dat_0cbb7ab8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7ab8.inc"
};
static NJS_MKEY_F dat_0cbb7ad8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7ad8.inc"
};
static NJS_MKEY_A dat_0cbb7af8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7af8.inc"
};
static NJS_MKEY_F dat_0cbb7b18[2] = {
#include "ev006d/amy_action_9/dat_0cbb7b18.inc"
};
static NJS_MKEY_A dat_0cbb7b38[2] = {
#include "ev006d/amy_action_9/dat_0cbb7b38.inc"
};
static NJS_MKEY_F dat_0cbb7b58[2] = {
#include "ev006d/amy_action_9/dat_0cbb7b58.inc"
};
static NJS_MKEY_A dat_0cbb7b78[2] = {
#include "ev006d/amy_action_9/dat_0cbb7b78.inc"
};
static NJS_MKEY_F dat_0cbb7b98[2] = {
#include "ev006d/amy_action_9/dat_0cbb7b98.inc"
};
static NJS_MKEY_A dat_0cbb7bb8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7bb8.inc"
};
static NJS_MKEY_F dat_0cbb7bd8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7bd8.inc"
};
static NJS_MKEY_A dat_0cbb7bf8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7bf8.inc"
};
static NJS_MKEY_F dat_0cbb7c18[2] = {
#include "ev006d/amy_action_9/dat_0cbb7c18.inc"
};
static NJS_MKEY_A dat_0cbb7c38[2] = {
#include "ev006d/amy_action_9/dat_0cbb7c38.inc"
};
static NJS_MKEY_F dat_0cbb7c58[2] = {
#include "ev006d/amy_action_9/dat_0cbb7c58.inc"
};
static NJS_MKEY_A dat_0cbb7c78[2] = {
#include "ev006d/amy_action_9/dat_0cbb7c78.inc"
};
static NJS_MKEY_F dat_0cbb7c98[2] = {
#include "ev006d/amy_action_9/dat_0cbb7c98.inc"
};
static NJS_MKEY_A dat_0cbb7cb8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7cb8.inc"
};
static NJS_MKEY_F dat_0cbb7cd8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7cd8.inc"
};
static NJS_MKEY_A dat_0cbb7cf8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7cf8.inc"
};
static NJS_MKEY_F dat_0cbb7d18[2] = {
#include "ev006d/amy_action_9/dat_0cbb7d18.inc"
};
static NJS_MKEY_A dat_0cbb7d38[2] = {
#include "ev006d/amy_action_9/dat_0cbb7d38.inc"
};
static NJS_MKEY_F dat_0cbb7d58[2] = {
#include "ev006d/amy_action_9/dat_0cbb7d58.inc"
};
static NJS_MKEY_A dat_0cbb7d78[2] = {
#include "ev006d/amy_action_9/dat_0cbb7d78.inc"
};
static NJS_MKEY_F dat_0cbb7d98[2] = {
#include "ev006d/amy_action_9/dat_0cbb7d98.inc"
};
static NJS_MKEY_A dat_0cbb7db8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7db8.inc"
};
static NJS_MKEY_F dat_0cbb7dd8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7dd8.inc"
};
static NJS_MKEY_A dat_0cbb7df8[30] = {
#include "ev006d/amy_action_9/dat_0cbb7df8.inc"
};
static NJS_MKEY_F dat_0cbb7fd8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7fd8.inc"
};
static NJS_MKEY_A dat_0cbb7ff8[2] = {
#include "ev006d/amy_action_9/dat_0cbb7ff8.inc"
};
static NJS_MKEY_F dat_0cbb8018[2] = {
#include "ev006d/amy_action_9/dat_0cbb8018.inc"
};
static NJS_MKEY_A dat_0cbb8038[2] = {
#include "ev006d/amy_action_9/dat_0cbb8038.inc"
};
static NJS_MKEY_F dat_0cbb8058[2] = {
#include "ev006d/amy_action_9/dat_0cbb8058.inc"
};
static NJS_MKEY_A dat_0cbb8078[2] = {
#include "ev006d/amy_action_9/dat_0cbb8078.inc"
};
static NJS_MKEY_F dat_0cbb8098[2] = {
#include "ev006d/amy_action_9/dat_0cbb8098.inc"
};
static NJS_MKEY_A dat_0cbb80b8[2] = {
#include "ev006d/amy_action_9/dat_0cbb80b8.inc"
};
static NJS_MKEY_F dat_0cbb80d8[2] = {
#include "ev006d/amy_action_9/dat_0cbb80d8.inc"
};
static NJS_MKEY_A dat_0cbb80f8[2] = {
#include "ev006d/amy_action_9/dat_0cbb80f8.inc"
};
static NJS_MKEY_F dat_0cbb8118[2] = {
#include "ev006d/amy_action_9/dat_0cbb8118.inc"
};
static NJS_MKEY_A dat_0cbb8138[2] = {
#include "ev006d/amy_action_9/dat_0cbb8138.inc"
};
static NJS_MKEY_F dat_0cbb8158[2] = {
#include "ev006d/amy_action_9/dat_0cbb8158.inc"
};
static NJS_MKEY_A dat_0cbb8178[2] = {
#include "ev006d/amy_action_9/dat_0cbb8178.inc"
};
static NJS_MKEY_F dat_0cbb8198[2] = {
#include "ev006d/amy_action_9/dat_0cbb8198.inc"
};
static NJS_MKEY_A dat_0cbb81b8[30] = {
#include "ev006d/amy_action_9/dat_0cbb81b8.inc"
};
static NJS_MKEY_F dat_0cbb8398[2] = {
#include "ev006d/amy_action_9/dat_0cbb8398.inc"
};
static NJS_MKEY_A dat_0cbb83b8[2] = {
#include "ev006d/amy_action_9/dat_0cbb83b8.inc"
};
static NJS_MKEY_F dat_0cbb83d8[2] = {
#include "ev006d/amy_action_9/dat_0cbb83d8.inc"
};
static NJS_MKEY_A dat_0cbb83f8[2] = {
#include "ev006d/amy_action_9/dat_0cbb83f8.inc"
};
static NJS_MKEY_F dat_0cbb8418[30] = {
#include "ev006d/amy_action_9/dat_0cbb8418.inc"
};
static NJS_MKEY_A dat_0cbb85f8[2] = {
#include "ev006d/amy_action_9/dat_0cbb85f8.inc"
};
static NJS_MKEY_F dat_0cbb8618[30] = {
#include "ev006d/amy_action_9/dat_0cbb8618.inc"
};
static NJS_MKEY_A dat_0cbb87f8[2] = {
#include "ev006d/amy_action_9/dat_0cbb87f8.inc"
};
static NJS_MKEY_F dat_0cbb8818[2] = {
#include "ev006d/amy_action_9/dat_0cbb8818.inc"
};
static NJS_MKEY_A dat_0cbb8838[2] = {
#include "ev006d/amy_action_9/dat_0cbb8838.inc"
};
static NJS_MKEY_F dat_0cbb8858[30] = {
#include "ev006d/amy_action_9/dat_0cbb8858.inc"
};
static NJS_MKEY_A dat_0cbb8a38[2] = {
#include "ev006d/amy_action_9/dat_0cbb8a38.inc"
};
static NJS_MKEY_F dat_0cbb8a58[30] = {
#include "ev006d/amy_action_9/dat_0cbb8a58.inc"
};
static NJS_MKEY_A dat_0cbb8c38[2] = {
#include "ev006d/amy_action_9/dat_0cbb8c38.inc"
};
static NJS_MKEY_F dat_0cbb8c58[2] = {
#include "ev006d/amy_action_9/dat_0cbb8c58.inc"
};
static NJS_MKEY_A dat_0cbb8c78[2] = {
#include "ev006d/amy_action_9/dat_0cbb8c78.inc"
};
static NJS_MKEY_F dat_0cbb8c98[2] = {
#include "ev006d/amy_action_9/dat_0cbb8c98.inc"
};
static NJS_MKEY_A dat_0cbb8cb8[2] = {
#include "ev006d/amy_action_9/dat_0cbb8cb8.inc"
};
static NJS_MKEY_F dat_0cbb8cd8[2] = {
#include "ev006d/amy_action_9/dat_0cbb8cd8.inc"
};
static NJS_MKEY_A dat_0cbb8cf8[2] = {
#include "ev006d/amy_action_9/dat_0cbb8cf8.inc"
};
static NJS_MKEY_F dat_0cbb8d18[2] = {
#include "ev006d/amy_action_9/dat_0cbb8d18.inc"
};
static NJS_MKEY_A dat_0cbb8d38[2] = {
#include "ev006d/amy_action_9/dat_0cbb8d38.inc"
};
static NJS_MKEY_F dat_0cbb8d58[2] = {
#include "ev006d/amy_action_9/dat_0cbb8d58.inc"
};
static NJS_MKEY_A dat_0cbb8d78[2] = {
#include "ev006d/amy_action_9/dat_0cbb8d78.inc"
};
static NJS_MKEY_F dat_0cbb8d98[2] = {
#include "ev006d/amy_action_9/dat_0cbb8d98.inc"
};
static NJS_MKEY_A dat_0cbb8db8[2] = {
#include "ev006d/amy_action_9/dat_0cbb8db8.inc"
};
static NJS_MKEY_F dat_0cbb8dd8[2] = {
#include "ev006d/amy_action_9/dat_0cbb8dd8.inc"
};
static NJS_MKEY_A dat_0cbb8df8[2] = {
#include "ev006d/amy_action_9/dat_0cbb8df8.inc"
};
static NJS_MKEY_F dat_0cbb8e18[2] = {
#include "ev006d/amy_action_9/dat_0cbb8e18.inc"
};
static NJS_MKEY_A dat_0cbb8e38[2] = {
#include "ev006d/amy_action_9/dat_0cbb8e38.inc"
};
static NJS_MKEY_F dat_0cbb8e58[2] = {
#include "ev006d/amy_action_9/dat_0cbb8e58.inc"
};
static NJS_MKEY_A dat_0cbb8e78[2] = {
#include "ev006d/amy_action_9/dat_0cbb8e78.inc"
};
static NJS_MKEY_F dat_0cbb8e98[2] = {
#include "ev006d/amy_action_9/dat_0cbb8e98.inc"
};
static NJS_MKEY_A dat_0cbb8eb8[2] = {
#include "ev006d/amy_action_9/dat_0cbb8eb8.inc"
};
static NJS_MKEY_F dat_0cbb8ed8[2] = {
#include "ev006d/amy_action_9/dat_0cbb8ed8.inc"
};
static NJS_MKEY_A dat_0cbb8ef8[2] = {
#include "ev006d/amy_action_9/dat_0cbb8ef8.inc"
};
static NJS_MKEY_F dat_0cbb8f18[2] = {
#include "ev006d/amy_action_9/dat_0cbb8f18.inc"
};
static NJS_MKEY_A dat_0cbb8f38[2] = {
#include "ev006d/amy_action_9/dat_0cbb8f38.inc"
};
static NJS_MKEY_F dat_0cbb8f58[2] = {
#include "ev006d/amy_action_9/dat_0cbb8f58.inc"
};
static NJS_MKEY_A dat_0cbb8f78[2] = {
#include "ev006d/amy_action_9/dat_0cbb8f78.inc"
};
static NJS_MKEY_F dat_0cbb8f98[2] = {
#include "ev006d/amy_action_9/dat_0cbb8f98.inc"
};
static NJS_MKEY_A dat_0cbb8fb8[2] = {
#include "ev006d/amy_action_9/dat_0cbb8fb8.inc"
};
static NJS_MKEY_F dat_0cbb8fd8[2] = {
#include "ev006d/amy_action_9/dat_0cbb8fd8.inc"
};
static NJS_MKEY_A dat_0cbb8ff8[2] = {
#include "ev006d/amy_action_9/dat_0cbb8ff8.inc"
};
static NJS_MKEY_F dat_0cbb9018[2] = {
#include "ev006d/amy_action_9/dat_0cbb9018.inc"
};
static NJS_MKEY_A dat_0cbb9038[2] = {
#include "ev006d/amy_action_9/dat_0cbb9038.inc"
};
static NJS_MDATA2 dat_0cbb9058[57] = {
#include "ev006d/amy_action_9/dat_0cbb9058.inc"
};
static NJS_MOTION dat_0cbb93e8 = {
#include "ev006d/amy_action_9/dat_0cbb93e8.inc"
};
static NJS_ACTION dat_0cbb93f4 = {
#include "ev006d/amy_action_9/dat_0cbb93f4.inc"
};
static NJS_MKEY_F dat_0cbb93fc[2] = {
#include "ev006d/amy_action_9/dat_0cbb93fc.inc"
};
static NJS_MKEY_A dat_0cbb941c[2] = {
#include "ev006d/amy_action_9/dat_0cbb941c.inc"
};
static NJS_MKEY_F dat_0cbb943c[30] = {
#include "ev006d/amy_action_9/dat_0cbb943c.inc"
};
static NJS_MKEY_A dat_0cbb961c[29] = {
#include "ev006d/amy_action_9/dat_0cbb961c.inc"
};
static NJS_MKEY_F dat_0cbb97ec[2] = {
#include "ev006d/amy_action_9/dat_0cbb97ec.inc"
};
static NJS_MKEY_A dat_0cbb980c[2] = {
#include "ev006d/amy_action_9/dat_0cbb980c.inc"
};
static NJS_MKEY_F dat_0cbb982c[2] = {
#include "ev006d/amy_action_9/dat_0cbb982c.inc"
};
static NJS_MKEY_A dat_0cbb984c[2] = {
#include "ev006d/amy_action_9/dat_0cbb984c.inc"
};
static NJS_MKEY_F dat_0cbb986c[2] = {
#include "ev006d/amy_action_9/dat_0cbb986c.inc"
};
static NJS_MKEY_A dat_0cbb988c[2] = {
#include "ev006d/amy_action_9/dat_0cbb988c.inc"
};
static NJS_MKEY_F dat_0cbb98ac[2] = {
#include "ev006d/amy_action_9/dat_0cbb98ac.inc"
};
static NJS_MKEY_A dat_0cbb98cc[2] = {
#include "ev006d/amy_action_9/dat_0cbb98cc.inc"
};
static NJS_MKEY_F dat_0cbb98ec[2] = {
#include "ev006d/amy_action_9/dat_0cbb98ec.inc"
};
static NJS_MKEY_A dat_0cbb990c[30] = {
#include "ev006d/amy_action_9/dat_0cbb990c.inc"
};
static NJS_MKEY_F dat_0cbb9aec[2] = {
#include "ev006d/amy_action_9/dat_0cbb9aec.inc"
};
static NJS_MKEY_A dat_0cbb9b0c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9b0c.inc"
};
static NJS_MKEY_F dat_0cbb9b2c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9b2c.inc"
};
static NJS_MKEY_A dat_0cbb9b4c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9b4c.inc"
};
static NJS_MKEY_F dat_0cbb9b6c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9b6c.inc"
};
static NJS_MKEY_A dat_0cbb9b8c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9b8c.inc"
};
static NJS_MKEY_F dat_0cbb9bac[2] = {
#include "ev006d/amy_action_9/dat_0cbb9bac.inc"
};
static NJS_MKEY_A dat_0cbb9bcc[30] = {
#include "ev006d/amy_action_9/dat_0cbb9bcc.inc"
};
static NJS_MKEY_F dat_0cbb9dac[2] = {
#include "ev006d/amy_action_9/dat_0cbb9dac.inc"
};
static NJS_MKEY_A dat_0cbb9dcc[2] = {
#include "ev006d/amy_action_9/dat_0cbb9dcc.inc"
};
static NJS_MKEY_F dat_0cbb9dec[2] = {
#include "ev006d/amy_action_9/dat_0cbb9dec.inc"
};
static NJS_MKEY_A dat_0cbb9e0c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9e0c.inc"
};
static NJS_MKEY_F dat_0cbb9e2c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9e2c.inc"
};
static NJS_MKEY_A dat_0cbb9e4c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9e4c.inc"
};
static NJS_MKEY_F dat_0cbb9e6c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9e6c.inc"
};
static NJS_MKEY_A dat_0cbb9e8c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9e8c.inc"
};
static NJS_MKEY_F dat_0cbb9eac[2] = {
#include "ev006d/amy_action_9/dat_0cbb9eac.inc"
};
static NJS_MKEY_A dat_0cbb9ecc[2] = {
#include "ev006d/amy_action_9/dat_0cbb9ecc.inc"
};
static NJS_MKEY_F dat_0cbb9eec[2] = {
#include "ev006d/amy_action_9/dat_0cbb9eec.inc"
};
static NJS_MKEY_A dat_0cbb9f0c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9f0c.inc"
};
static NJS_MKEY_F dat_0cbb9f2c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9f2c.inc"
};
static NJS_MKEY_A dat_0cbb9f4c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9f4c.inc"
};
static NJS_MKEY_F dat_0cbb9f6c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9f6c.inc"
};
static NJS_MKEY_A dat_0cbb9f8c[2] = {
#include "ev006d/amy_action_9/dat_0cbb9f8c.inc"
};
static NJS_MKEY_F dat_0cbb9fac[2] = {
#include "ev006d/amy_action_9/dat_0cbb9fac.inc"
};
static NJS_MKEY_A dat_0cbb9fcc[2] = {
#include "ev006d/amy_action_9/dat_0cbb9fcc.inc"
};
static NJS_MKEY_F dat_0cbb9fec[2] = {
#include "ev006d/amy_action_9/dat_0cbb9fec.inc"
};
static NJS_MKEY_A dat_0cbba00c[2] = {
#include "ev006d/amy_action_9/dat_0cbba00c.inc"
};
static NJS_MKEY_F dat_0cbba02c[2] = {
#include "ev006d/amy_action_9/dat_0cbba02c.inc"
};
static NJS_MKEY_A dat_0cbba04c[2] = {
#include "ev006d/amy_action_9/dat_0cbba04c.inc"
};
static NJS_MKEY_F dat_0cbba06c[2] = {
#include "ev006d/amy_action_9/dat_0cbba06c.inc"
};
static NJS_MKEY_A dat_0cbba08c[2] = {
#include "ev006d/amy_action_9/dat_0cbba08c.inc"
};
static NJS_MKEY_F dat_0cbba0ac[2] = {
#include "ev006d/amy_action_9/dat_0cbba0ac.inc"
};
static NJS_MKEY_A dat_0cbba0cc[2] = {
#include "ev006d/amy_action_9/dat_0cbba0cc.inc"
};
static NJS_MKEY_F dat_0cbba0ec[2] = {
#include "ev006d/amy_action_9/dat_0cbba0ec.inc"
};
static NJS_MKEY_A dat_0cbba10c[2] = {
#include "ev006d/amy_action_9/dat_0cbba10c.inc"
};
static NJS_MKEY_F dat_0cbba12c[2] = {
#include "ev006d/amy_action_9/dat_0cbba12c.inc"
};
static NJS_MKEY_A dat_0cbba14c[30] = {
#include "ev006d/amy_action_9/dat_0cbba14c.inc"
};
static NJS_MKEY_F dat_0cbba32c[2] = {
#include "ev006d/amy_action_9/dat_0cbba32c.inc"
};
static NJS_MKEY_A dat_0cbba34c[2] = {
#include "ev006d/amy_action_9/dat_0cbba34c.inc"
};
static NJS_MKEY_F dat_0cbba36c[2] = {
#include "ev006d/amy_action_9/dat_0cbba36c.inc"
};
static NJS_MKEY_A dat_0cbba38c[2] = {
#include "ev006d/amy_action_9/dat_0cbba38c.inc"
};
static NJS_MKEY_F dat_0cbba3ac[2] = {
#include "ev006d/amy_action_9/dat_0cbba3ac.inc"
};
static NJS_MKEY_A dat_0cbba3cc[2] = {
#include "ev006d/amy_action_9/dat_0cbba3cc.inc"
};
static NJS_MKEY_F dat_0cbba3ec[2] = {
#include "ev006d/amy_action_9/dat_0cbba3ec.inc"
};
static NJS_MKEY_A dat_0cbba40c[2] = {
#include "ev006d/amy_action_9/dat_0cbba40c.inc"
};
static NJS_MKEY_F dat_0cbba42c[2] = {
#include "ev006d/amy_action_9/dat_0cbba42c.inc"
};
static NJS_MKEY_A dat_0cbba44c[2] = {
#include "ev006d/amy_action_9/dat_0cbba44c.inc"
};
static NJS_MKEY_F dat_0cbba46c[2] = {
#include "ev006d/amy_action_9/dat_0cbba46c.inc"
};
static NJS_MKEY_A dat_0cbba48c[2] = {
#include "ev006d/amy_action_9/dat_0cbba48c.inc"
};
static NJS_MKEY_F dat_0cbba4ac[2] = {
#include "ev006d/amy_action_9/dat_0cbba4ac.inc"
};
static NJS_MKEY_A dat_0cbba4cc[2] = {
#include "ev006d/amy_action_9/dat_0cbba4cc.inc"
};
static NJS_MKEY_F dat_0cbba4ec[2] = {
#include "ev006d/amy_action_9/dat_0cbba4ec.inc"
};
static NJS_MKEY_A dat_0cbba50c[30] = {
#include "ev006d/amy_action_9/dat_0cbba50c.inc"
};
static NJS_MKEY_F dat_0cbba6ec[2] = {
#include "ev006d/amy_action_9/dat_0cbba6ec.inc"
};
static NJS_MKEY_A dat_0cbba70c[2] = {
#include "ev006d/amy_action_9/dat_0cbba70c.inc"
};
static NJS_MKEY_F dat_0cbba72c[2] = {
#include "ev006d/amy_action_9/dat_0cbba72c.inc"
};
static NJS_MKEY_A dat_0cbba74c[2] = {
#include "ev006d/amy_action_9/dat_0cbba74c.inc"
};
static NJS_MKEY_F dat_0cbba76c[30] = {
#include "ev006d/amy_action_9/dat_0cbba76c.inc"
};
static NJS_MKEY_A dat_0cbba94c[2] = {
#include "ev006d/amy_action_9/dat_0cbba94c.inc"
};
static NJS_MKEY_F dat_0cbba96c[30] = {
#include "ev006d/amy_action_9/dat_0cbba96c.inc"
};
static NJS_MKEY_A dat_0cbbab4c[2] = {
#include "ev006d/amy_action_9/dat_0cbbab4c.inc"
};
static NJS_MKEY_F dat_0cbbab6c[2] = {
#include "ev006d/amy_action_9/dat_0cbbab6c.inc"
};
static NJS_MKEY_A dat_0cbbab8c[2] = {
#include "ev006d/amy_action_9/dat_0cbbab8c.inc"
};
static NJS_MKEY_F dat_0cbbabac[30] = {
#include "ev006d/amy_action_9/dat_0cbbabac.inc"
};
static NJS_MKEY_A dat_0cbbad8c[2] = {
#include "ev006d/amy_action_9/dat_0cbbad8c.inc"
};
static NJS_MKEY_F dat_0cbbadac[30] = {
#include "ev006d/amy_action_9/dat_0cbbadac.inc"
};
static NJS_MKEY_A dat_0cbbaf8c[2] = {
#include "ev006d/amy_action_9/dat_0cbbaf8c.inc"
};
static NJS_MKEY_F dat_0cbbafac[2] = {
#include "ev006d/amy_action_9/dat_0cbbafac.inc"
};
static NJS_MKEY_A dat_0cbbafcc[2] = {
#include "ev006d/amy_action_9/dat_0cbbafcc.inc"
};
static NJS_MKEY_F dat_0cbbafec[2] = {
#include "ev006d/amy_action_9/dat_0cbbafec.inc"
};
static NJS_MKEY_A dat_0cbbb00c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb00c.inc"
};
static NJS_MKEY_F dat_0cbbb02c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb02c.inc"
};
static NJS_MKEY_A dat_0cbbb04c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb04c.inc"
};
static NJS_MKEY_F dat_0cbbb06c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb06c.inc"
};
static NJS_MKEY_A dat_0cbbb08c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb08c.inc"
};
static NJS_MKEY_F dat_0cbbb0ac[2] = {
#include "ev006d/amy_action_9/dat_0cbbb0ac.inc"
};
static NJS_MKEY_A dat_0cbbb0cc[2] = {
#include "ev006d/amy_action_9/dat_0cbbb0cc.inc"
};
static NJS_MKEY_F dat_0cbbb0ec[2] = {
#include "ev006d/amy_action_9/dat_0cbbb0ec.inc"
};
static NJS_MKEY_A dat_0cbbb10c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb10c.inc"
};
static NJS_MKEY_F dat_0cbbb12c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb12c.inc"
};
static NJS_MKEY_A dat_0cbbb14c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb14c.inc"
};
static NJS_MKEY_F dat_0cbbb16c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb16c.inc"
};
static NJS_MKEY_A dat_0cbbb18c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb18c.inc"
};
static NJS_MKEY_F dat_0cbbb1ac[2] = {
#include "ev006d/amy_action_9/dat_0cbbb1ac.inc"
};
static NJS_MKEY_A dat_0cbbb1cc[2] = {
#include "ev006d/amy_action_9/dat_0cbbb1cc.inc"
};
static NJS_MKEY_F dat_0cbbb1ec[2] = {
#include "ev006d/amy_action_9/dat_0cbbb1ec.inc"
};
static NJS_MKEY_A dat_0cbbb20c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb20c.inc"
};
static NJS_MKEY_F dat_0cbbb22c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb22c.inc"
};
static NJS_MKEY_A dat_0cbbb24c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb24c.inc"
};
static NJS_MKEY_F dat_0cbbb26c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb26c.inc"
};
static NJS_MKEY_A dat_0cbbb28c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb28c.inc"
};
static NJS_MKEY_F dat_0cbbb2ac[2] = {
#include "ev006d/amy_action_9/dat_0cbbb2ac.inc"
};
static NJS_MKEY_A dat_0cbbb2cc[2] = {
#include "ev006d/amy_action_9/dat_0cbbb2cc.inc"
};
static NJS_MKEY_F dat_0cbbb2ec[2] = {
#include "ev006d/amy_action_9/dat_0cbbb2ec.inc"
};
static NJS_MKEY_A dat_0cbbb30c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb30c.inc"
};
static NJS_MKEY_F dat_0cbbb32c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb32c.inc"
};
static NJS_MKEY_A dat_0cbbb34c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb34c.inc"
};
static NJS_MKEY_F dat_0cbbb36c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb36c.inc"
};
static NJS_MKEY_A dat_0cbbb38c[2] = {
#include "ev006d/amy_action_9/dat_0cbbb38c.inc"
};
static NJS_MDATA2 ev006d_bird_motion_2_mdata[57] = {
#include "ev006d/amy_action_9/ev006d_bird_motion_2_mdata.inc"
};
static NJS_MOTION ev006d_bird_motion_2 = {
#include "ev006d/amy_action_9/ev006d_bird_motion_2.inc"
};
NJS_ACTION ev006d_bird_action_2 = {
#include "ev006d/amy_action_9/ev006d_bird_action_2.inc"
};
static NJS_MKEY_F dat_0cbbb750[2] = {
#include "ev006d/amy_action_9/dat_0cbbb750.inc"
};
static NJS_MKEY_A dat_0cbbb770[2] = {
#include "ev006d/amy_action_9/dat_0cbbb770.inc"
};
static NJS_MKEY_F dat_0cbbb790[30] = {
#include "ev006d/amy_action_9/dat_0cbbb790.inc"
};
static NJS_MKEY_A dat_0cbbb970[29] = {
#include "ev006d/amy_action_9/dat_0cbbb970.inc"
};
static NJS_MKEY_F dat_0cbbbb40[2] = {
#include "ev006d/amy_action_9/dat_0cbbbb40.inc"
};
static NJS_MKEY_A dat_0cbbbb60[2] = {
#include "ev006d/amy_action_9/dat_0cbbbb60.inc"
};
static NJS_MKEY_F dat_0cbbbb80[2] = {
#include "ev006d/amy_action_9/dat_0cbbbb80.inc"
};
static NJS_MKEY_A dat_0cbbbba0[2] = {
#include "ev006d/amy_action_9/dat_0cbbbba0.inc"
};
static NJS_MKEY_F dat_0cbbbbc0[2] = {
#include "ev006d/amy_action_9/dat_0cbbbbc0.inc"
};
static NJS_MKEY_A dat_0cbbbbe0[2] = {
#include "ev006d/amy_action_9/dat_0cbbbbe0.inc"
};
static NJS_MKEY_F dat_0cbbbc00[2] = {
#include "ev006d/amy_action_9/dat_0cbbbc00.inc"
};
static NJS_MKEY_A dat_0cbbbc20[2] = {
#include "ev006d/amy_action_9/dat_0cbbbc20.inc"
};
static NJS_MKEY_F dat_0cbbbc40[2] = {
#include "ev006d/amy_action_9/dat_0cbbbc40.inc"
};
static NJS_MKEY_A dat_0cbbbc60[30] = {
#include "ev006d/amy_action_9/dat_0cbbbc60.inc"
};
static NJS_MKEY_F dat_0cbbbe40[2] = {
#include "ev006d/amy_action_9/dat_0cbbbe40.inc"
};
static NJS_MKEY_A dat_0cbbbe60[2] = {
#include "ev006d/amy_action_9/dat_0cbbbe60.inc"
};
static NJS_MKEY_F dat_0cbbbe80[2] = {
#include "ev006d/amy_action_9/dat_0cbbbe80.inc"
};
static NJS_MKEY_A dat_0cbbbea0[2] = {
#include "ev006d/amy_action_9/dat_0cbbbea0.inc"
};
static NJS_MKEY_F dat_0cbbbec0[2] = {
#include "ev006d/amy_action_9/dat_0cbbbec0.inc"
};
static NJS_MKEY_A dat_0cbbbee0[2] = {
#include "ev006d/amy_action_9/dat_0cbbbee0.inc"
};
static NJS_MKEY_F dat_0cbbbf00[2] = {
#include "ev006d/amy_action_9/dat_0cbbbf00.inc"
};
static NJS_MKEY_A dat_0cbbbf20[30] = {
#include "ev006d/amy_action_9/dat_0cbbbf20.inc"
};
static NJS_MKEY_F dat_0cbbc100[2] = {
#include "ev006d/amy_action_9/dat_0cbbc100.inc"
};
static NJS_MKEY_A dat_0cbbc120[2] = {
#include "ev006d/amy_action_9/dat_0cbbc120.inc"
};
static NJS_MKEY_F dat_0cbbc140[2] = {
#include "ev006d/amy_action_9/dat_0cbbc140.inc"
};
static NJS_MKEY_A dat_0cbbc160[2] = {
#include "ev006d/amy_action_9/dat_0cbbc160.inc"
};
static NJS_MKEY_F dat_0cbbc180[2] = {
#include "ev006d/amy_action_9/dat_0cbbc180.inc"
};
static NJS_MKEY_A dat_0cbbc1a0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc1a0.inc"
};
static NJS_MKEY_F dat_0cbbc1c0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc1c0.inc"
};
static NJS_MKEY_A dat_0cbbc1e0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc1e0.inc"
};
static NJS_MKEY_F dat_0cbbc200[2] = {
#include "ev006d/amy_action_9/dat_0cbbc200.inc"
};
static NJS_MKEY_A dat_0cbbc220[2] = {
#include "ev006d/amy_action_9/dat_0cbbc220.inc"
};
static NJS_MKEY_F dat_0cbbc240[2] = {
#include "ev006d/amy_action_9/dat_0cbbc240.inc"
};
static NJS_MKEY_A dat_0cbbc260[2] = {
#include "ev006d/amy_action_9/dat_0cbbc260.inc"
};
static NJS_MKEY_F dat_0cbbc280[2] = {
#include "ev006d/amy_action_9/dat_0cbbc280.inc"
};
static NJS_MKEY_A dat_0cbbc2a0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc2a0.inc"
};
static NJS_MKEY_F dat_0cbbc2c0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc2c0.inc"
};
static NJS_MKEY_A dat_0cbbc2e0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc2e0.inc"
};
static NJS_MKEY_F dat_0cbbc300[2] = {
#include "ev006d/amy_action_9/dat_0cbbc300.inc"
};
static NJS_MKEY_A dat_0cbbc320[2] = {
#include "ev006d/amy_action_9/dat_0cbbc320.inc"
};
static NJS_MKEY_F dat_0cbbc340[2] = {
#include "ev006d/amy_action_9/dat_0cbbc340.inc"
};
static NJS_MKEY_A dat_0cbbc360[2] = {
#include "ev006d/amy_action_9/dat_0cbbc360.inc"
};
static NJS_MKEY_F dat_0cbbc380[2] = {
#include "ev006d/amy_action_9/dat_0cbbc380.inc"
};
static NJS_MKEY_A dat_0cbbc3a0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc3a0.inc"
};
static NJS_MKEY_F dat_0cbbc3c0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc3c0.inc"
};
static NJS_MKEY_A dat_0cbbc3e0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc3e0.inc"
};
static NJS_MKEY_F dat_0cbbc400[2] = {
#include "ev006d/amy_action_9/dat_0cbbc400.inc"
};
static NJS_MKEY_A dat_0cbbc420[2] = {
#include "ev006d/amy_action_9/dat_0cbbc420.inc"
};
static NJS_MKEY_F dat_0cbbc440[2] = {
#include "ev006d/amy_action_9/dat_0cbbc440.inc"
};
static NJS_MKEY_A dat_0cbbc460[2] = {
#include "ev006d/amy_action_9/dat_0cbbc460.inc"
};
static NJS_MKEY_F dat_0cbbc480[2] = {
#include "ev006d/amy_action_9/dat_0cbbc480.inc"
};
static NJS_MKEY_A dat_0cbbc4a0[30] = {
#include "ev006d/amy_action_9/dat_0cbbc4a0.inc"
};
static NJS_MKEY_F dat_0cbbc680[2] = {
#include "ev006d/amy_action_9/dat_0cbbc680.inc"
};
static NJS_MKEY_A dat_0cbbc6a0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc6a0.inc"
};
static NJS_MKEY_F dat_0cbbc6c0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc6c0.inc"
};
static NJS_MKEY_A dat_0cbbc6e0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc6e0.inc"
};
static NJS_MKEY_F dat_0cbbc700[2] = {
#include "ev006d/amy_action_9/dat_0cbbc700.inc"
};
static NJS_MKEY_A dat_0cbbc720[2] = {
#include "ev006d/amy_action_9/dat_0cbbc720.inc"
};
static NJS_MKEY_F dat_0cbbc740[2] = {
#include "ev006d/amy_action_9/dat_0cbbc740.inc"
};
static NJS_MKEY_A dat_0cbbc760[2] = {
#include "ev006d/amy_action_9/dat_0cbbc760.inc"
};
static NJS_MKEY_F dat_0cbbc780[2] = {
#include "ev006d/amy_action_9/dat_0cbbc780.inc"
};
static NJS_MKEY_A dat_0cbbc7a0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc7a0.inc"
};
static NJS_MKEY_F dat_0cbbc7c0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc7c0.inc"
};
static NJS_MKEY_A dat_0cbbc7e0[2] = {
#include "ev006d/amy_action_9/dat_0cbbc7e0.inc"
};
static NJS_MKEY_F dat_0cbbc800[2] = {
#include "ev006d/amy_action_9/dat_0cbbc800.inc"
};
static NJS_MKEY_A dat_0cbbc820[2] = {
#include "ev006d/amy_action_9/dat_0cbbc820.inc"
};
static NJS_MKEY_F dat_0cbbc840[2] = {
#include "ev006d/amy_action_9/dat_0cbbc840.inc"
};
static NJS_MKEY_A dat_0cbbc860[30] = {
#include "ev006d/amy_action_9/dat_0cbbc860.inc"
};
static NJS_MKEY_F dat_0cbbca40[2] = {
#include "ev006d/amy_action_9/dat_0cbbca40.inc"
};
static NJS_MKEY_A dat_0cbbca60[2] = {
#include "ev006d/amy_action_9/dat_0cbbca60.inc"
};
static NJS_MKEY_F dat_0cbbca80[2] = {
#include "ev006d/amy_action_9/dat_0cbbca80.inc"
};
static NJS_MKEY_A dat_0cbbcaa0[2] = {
#include "ev006d/amy_action_9/dat_0cbbcaa0.inc"
};
static NJS_MKEY_F dat_0cbbcac0[30] = {
#include "ev006d/amy_action_9/dat_0cbbcac0.inc"
};
static NJS_MKEY_A dat_0cbbcca0[2] = {
#include "ev006d/amy_action_9/dat_0cbbcca0.inc"
};
static NJS_MKEY_F dat_0cbbccc0[30] = {
#include "ev006d/amy_action_9/dat_0cbbccc0.inc"
};
static NJS_MKEY_A dat_0cbbcea0[2] = {
#include "ev006d/amy_action_9/dat_0cbbcea0.inc"
};
static NJS_MKEY_F dat_0cbbcec0[2] = {
#include "ev006d/amy_action_9/dat_0cbbcec0.inc"
};
static NJS_MKEY_A dat_0cbbcee0[2] = {
#include "ev006d/amy_action_9/dat_0cbbcee0.inc"
};
static NJS_MKEY_F dat_0cbbcf00[30] = {
#include "ev006d/amy_action_9/dat_0cbbcf00.inc"
};
static NJS_MKEY_A dat_0cbbd0e0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd0e0.inc"
};
static NJS_MKEY_F dat_0cbbd100[30] = {
#include "ev006d/amy_action_9/dat_0cbbd100.inc"
};
static NJS_MKEY_A dat_0cbbd2e0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd2e0.inc"
};
static NJS_MKEY_F dat_0cbbd300[2] = {
#include "ev006d/amy_action_9/dat_0cbbd300.inc"
};
static NJS_MKEY_A dat_0cbbd320[2] = {
#include "ev006d/amy_action_9/dat_0cbbd320.inc"
};
static NJS_MKEY_F dat_0cbbd340[2] = {
#include "ev006d/amy_action_9/dat_0cbbd340.inc"
};
static NJS_MKEY_A dat_0cbbd360[2] = {
#include "ev006d/amy_action_9/dat_0cbbd360.inc"
};
static NJS_MKEY_F dat_0cbbd380[2] = {
#include "ev006d/amy_action_9/dat_0cbbd380.inc"
};
static NJS_MKEY_A dat_0cbbd3a0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd3a0.inc"
};
static NJS_MKEY_F dat_0cbbd3c0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd3c0.inc"
};
static NJS_MKEY_A dat_0cbbd3e0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd3e0.inc"
};
static NJS_MKEY_F dat_0cbbd400[2] = {
#include "ev006d/amy_action_9/dat_0cbbd400.inc"
};
static NJS_MKEY_A dat_0cbbd420[2] = {
#include "ev006d/amy_action_9/dat_0cbbd420.inc"
};
static NJS_MKEY_F dat_0cbbd440[2] = {
#include "ev006d/amy_action_9/dat_0cbbd440.inc"
};
static NJS_MKEY_A dat_0cbbd460[2] = {
#include "ev006d/amy_action_9/dat_0cbbd460.inc"
};
static NJS_MKEY_F dat_0cbbd480[2] = {
#include "ev006d/amy_action_9/dat_0cbbd480.inc"
};
static NJS_MKEY_A dat_0cbbd4a0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd4a0.inc"
};
static NJS_MKEY_F dat_0cbbd4c0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd4c0.inc"
};
static NJS_MKEY_A dat_0cbbd4e0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd4e0.inc"
};
static NJS_MKEY_F dat_0cbbd500[2] = {
#include "ev006d/amy_action_9/dat_0cbbd500.inc"
};
static NJS_MKEY_A dat_0cbbd520[2] = {
#include "ev006d/amy_action_9/dat_0cbbd520.inc"
};
static NJS_MKEY_F dat_0cbbd540[2] = {
#include "ev006d/amy_action_9/dat_0cbbd540.inc"
};
static NJS_MKEY_A dat_0cbbd560[2] = {
#include "ev006d/amy_action_9/dat_0cbbd560.inc"
};
static NJS_MKEY_F dat_0cbbd580[2] = {
#include "ev006d/amy_action_9/dat_0cbbd580.inc"
};
static NJS_MKEY_A dat_0cbbd5a0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd5a0.inc"
};
static NJS_MKEY_F dat_0cbbd5c0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd5c0.inc"
};
static NJS_MKEY_A dat_0cbbd5e0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd5e0.inc"
};
static NJS_MKEY_F dat_0cbbd600[2] = {
#include "ev006d/amy_action_9/dat_0cbbd600.inc"
};
static NJS_MKEY_A dat_0cbbd620[2] = {
#include "ev006d/amy_action_9/dat_0cbbd620.inc"
};
static NJS_MKEY_F dat_0cbbd640[2] = {
#include "ev006d/amy_action_9/dat_0cbbd640.inc"
};
static NJS_MKEY_A dat_0cbbd660[2] = {
#include "ev006d/amy_action_9/dat_0cbbd660.inc"
};
static NJS_MKEY_F dat_0cbbd680[2] = {
#include "ev006d/amy_action_9/dat_0cbbd680.inc"
};
static NJS_MKEY_A dat_0cbbd6a0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd6a0.inc"
};
static NJS_MKEY_F dat_0cbbd6c0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd6c0.inc"
};
static NJS_MKEY_A dat_0cbbd6e0[2] = {
#include "ev006d/amy_action_9/dat_0cbbd6e0.inc"
};
static NJS_MDATA2 ev006d_bird_motion_3_mdata[57] = {
#include "ev006d/amy_action_9/ev006d_bird_motion_3_mdata.inc"
};
static NJS_MOTION ev006d_bird_motion_3 = {
#include "ev006d/amy_action_9/ev006d_bird_motion_3.inc"
};
NJS_ACTION ev006d_bird_action_3 = {
#include "ev006d/amy_action_9/ev006d_bird_action_3.inc"
};
static NJS_TEXNAME ev006d_picture_textures[2] = {
    {TEXT_13, 0, 0},
    {TEXT_14, 0, 0},
};
NJS_TEXLIST ev006d_picture_texlist = {
#include "ev006d/amy_action_9/ev006d_picture_texlist.inc"
};
