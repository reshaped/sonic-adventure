/* the helicopter's 12 texture names and its texlist, which the event loads
 * from EV_HELI. the helicopter's model of 13 nodes follows, then
 * ev0001_helicopter_action and its motion, which turns one node. the patrol
 * cars' 14 texture names and their texlist, from SSPATCAR_BODY, come next.
 * the file ends on the material of a node of the patrol cars' model. the
 * node's other parts start the data after this file */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0001/helicopter_textures.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 16
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 10
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 4
#define TEXT_13_LENGTH 14
#define TEXT_14_LENGTH 6
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 7
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 10
#define TEXT_21_LENGTH 12
#define TEXT_22_LENGTH 4
#define TEXT_23_LENGTH 8
#include "ev0001/helicopter_textures/text.inc"

static NJS_TEXNAME ev0001_helicopter_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0001_helicopter_texlist = {
#include "ev0001/helicopter_textures/ev0001_helicopter_texlist.inc"
};
static NJS_MATERIAL dat_0cb8da10[1] = {
#include "ev0001/helicopter_textures/dat_0cb8da10.inc"
};
static Sint16 dat_0cb8da24[6] = {
#include "ev0001/helicopter_textures/dat_0cb8da24.inc"
};
static NJS_COLOR dat_0cb8da30[4] = {
#include "ev0001/helicopter_textures/dat_0cb8da30.inc"
};
static NJS_MESHSET dat_0cb8da40[1] = {
#include "ev0001/helicopter_textures/dat_0cb8da40.inc"
};
static NJS_POINT3 dat_0cb8da58[4] = {
#include "ev0001/helicopter_textures/dat_0cb8da58.inc"
};
static NJS_VECTOR dat_0cb8da88[4] = {
#include "ev0001/helicopter_textures/dat_0cb8da88.inc"
};
static NJS_MODEL dat_0cb8dab8 = {
#include "ev0001/helicopter_textures/dat_0cb8dab8.inc"
};
static NJS_OBJECT dat_0cb8dae0 = {
#include "ev0001/helicopter_textures/dat_0cb8dae0.inc"
};
static NJS_MATERIAL dat_0cb8db14[1] = {
#include "ev0001/helicopter_textures/dat_0cb8db14.inc"
};
static Sint16 dat_0cb8db28[6] = {
#include "ev0001/helicopter_textures/dat_0cb8db28.inc"
};
static NJS_COLOR dat_0cb8db34[4] = {
#include "ev0001/helicopter_textures/dat_0cb8db34.inc"
};
static NJS_MESHSET dat_0cb8db44[1] = {
#include "ev0001/helicopter_textures/dat_0cb8db44.inc"
};
static NJS_POINT3 dat_0cb8db5c[4] = {
#include "ev0001/helicopter_textures/dat_0cb8db5c.inc"
};
static NJS_VECTOR dat_0cb8db8c[4] = {
#include "ev0001/helicopter_textures/dat_0cb8db8c.inc"
};
static NJS_MODEL dat_0cb8dbbc = {
#include "ev0001/helicopter_textures/dat_0cb8dbbc.inc"
};
static NJS_OBJECT dat_0cb8dbe4 = {
#include "ev0001/helicopter_textures/dat_0cb8dbe4.inc"
};
static NJS_MATERIAL dat_0cb8dc18[1] = {
#include "ev0001/helicopter_textures/dat_0cb8dc18.inc"
};
static Sint16 dat_0cb8dc2c[12] = {
#include "ev0001/helicopter_textures/dat_0cb8dc2c.inc"
};
static NJS_COLOR dat_0cb8dc44[10] = {
#include "ev0001/helicopter_textures/dat_0cb8dc44.inc"
};
static NJS_MESHSET dat_0cb8dc6c[1] = {
#include "ev0001/helicopter_textures/dat_0cb8dc6c.inc"
};
static NJS_POINT3 dat_0cb8dc84[7] = {
#include "ev0001/helicopter_textures/dat_0cb8dc84.inc"
};
static NJS_VECTOR dat_0cb8dcd8[7] = {
#include "ev0001/helicopter_textures/dat_0cb8dcd8.inc"
};
static NJS_MODEL dat_0cb8dd2c = {
#include "ev0001/helicopter_textures/dat_0cb8dd2c.inc"
};
static NJS_OBJECT dat_0cb8dd54 = {
#include "ev0001/helicopter_textures/dat_0cb8dd54.inc"
};
static NJS_MATERIAL dat_0cb8dd88[1] = {
#include "ev0001/helicopter_textures/dat_0cb8dd88.inc"
};
static Sint16 dat_0cb8dd9c[18] = {
#include "ev0001/helicopter_textures/dat_0cb8dd9c.inc"
};
static NJS_COLOR dat_0cb8ddc0[14] = {
#include "ev0001/helicopter_textures/dat_0cb8ddc0.inc"
};
static NJS_MESHSET dat_0cb8ddf8[1] = {
#include "ev0001/helicopter_textures/dat_0cb8ddf8.inc"
};
static NJS_POINT3 dat_0cb8de10[9] = {
#include "ev0001/helicopter_textures/dat_0cb8de10.inc"
};
static NJS_VECTOR dat_0cb8de7c[9] = {
#include "ev0001/helicopter_textures/dat_0cb8de7c.inc"
};
static NJS_MODEL dat_0cb8dee8 = {
#include "ev0001/helicopter_textures/dat_0cb8dee8.inc"
};
static NJS_OBJECT dat_0cb8df10 = {
#include "ev0001/helicopter_textures/dat_0cb8df10.inc"
};
static NJS_MATERIAL dat_0cb8df44[3] = {
#include "ev0001/helicopter_textures/dat_0cb8df44.inc"
};
static Sint16 dat_0cb8df80[12] = {
#include "ev0001/helicopter_textures/dat_0cb8df80.inc"
};
static Sint16 dat_0cb8df98[15] = {
#include "ev0001/helicopter_textures/dat_0cb8df98.inc"
};
static Sint16 dat_0cb8dfb6[31] = {
#include "ev0001/helicopter_textures/dat_0cb8dfb6.inc"
};
static NJS_COLOR dat_0cb8dff4[10] = {
#include "ev0001/helicopter_textures/dat_0cb8dff4.inc"
};
static NJS_COLOR dat_0cb8e01c[14] = {
#include "ev0001/helicopter_textures/dat_0cb8e01c.inc"
};
static NJS_COLOR dat_0cb8e054[26] = {
#include "ev0001/helicopter_textures/dat_0cb8e054.inc"
};
static NJS_MESHSET dat_0cb8e0bc[3] = {
#include "ev0001/helicopter_textures/dat_0cb8e0bc.inc"
};
static NJS_POINT3 dat_0cb8e104[19] = {
#include "ev0001/helicopter_textures/dat_0cb8e104.inc"
};
static NJS_VECTOR dat_0cb8e1e8[19] = {
#include "ev0001/helicopter_textures/dat_0cb8e1e8.inc"
};
static NJS_MODEL dat_0cb8e2cc = {
#include "ev0001/helicopter_textures/dat_0cb8e2cc.inc"
};
static NJS_OBJECT dat_0cb8e2f4 = {
#include "ev0001/helicopter_textures/dat_0cb8e2f4.inc"
};
static NJS_MATERIAL dat_0cb8e328[1] = {
#include "ev0001/helicopter_textures/dat_0cb8e328.inc"
};
static Sint16 dat_0cb8e33c[8] = {
#include "ev0001/helicopter_textures/dat_0cb8e33c.inc"
};
static NJS_COLOR dat_0cb8e34c[6] = {
#include "ev0001/helicopter_textures/dat_0cb8e34c.inc"
};
static NJS_MESHSET dat_0cb8e364[1] = {
#include "ev0001/helicopter_textures/dat_0cb8e364.inc"
};
static NJS_POINT3 dat_0cb8e37c[6] = {
#include "ev0001/helicopter_textures/dat_0cb8e37c.inc"
};
static NJS_VECTOR dat_0cb8e3c4[6] = {
#include "ev0001/helicopter_textures/dat_0cb8e3c4.inc"
};
static NJS_MODEL dat_0cb8e40c = {
#include "ev0001/helicopter_textures/dat_0cb8e40c.inc"
};
static NJS_OBJECT dat_0cb8e434 = {
#include "ev0001/helicopter_textures/dat_0cb8e434.inc"
};
static NJS_MATERIAL dat_0cb8e468[2] = {
#include "ev0001/helicopter_textures/dat_0cb8e468.inc"
};
static Sint16 dat_0cb8e490[14] = {
#include "ev0001/helicopter_textures/dat_0cb8e490.inc"
};
static Sint16 dat_0cb8e4ac[10] = {
#include "ev0001/helicopter_textures/dat_0cb8e4ac.inc"
};
static NJS_COLOR dat_0cb8e4c0[12] = {
#include "ev0001/helicopter_textures/dat_0cb8e4c0.inc"
};
static NJS_COLOR dat_0cb8e4f0[8] = {
#include "ev0001/helicopter_textures/dat_0cb8e4f0.inc"
};
static NJS_MESHSET dat_0cb8e510[2] = {
#include "ev0001/helicopter_textures/dat_0cb8e510.inc"
};
static NJS_POINT3 dat_0cb8e540[12] = {
#include "ev0001/helicopter_textures/dat_0cb8e540.inc"
};
static NJS_VECTOR dat_0cb8e5d0[12] = {
#include "ev0001/helicopter_textures/dat_0cb8e5d0.inc"
};
static NJS_MODEL dat_0cb8e660 = {
#include "ev0001/helicopter_textures/dat_0cb8e660.inc"
};
static NJS_OBJECT dat_0cb8e688 = {
#include "ev0001/helicopter_textures/dat_0cb8e688.inc"
};
static NJS_MATERIAL dat_0cb8e6bc[1] = {
#include "ev0001/helicopter_textures/dat_0cb8e6bc.inc"
};
static Sint16 dat_0cb8e6d0[126] = {
#include "ev0001/helicopter_textures/dat_0cb8e6d0.inc"
};
static NJS_COLOR dat_0cb8e7cc[114] = {
#include "ev0001/helicopter_textures/dat_0cb8e7cc.inc"
};
static NJS_MESHSET dat_0cb8e994[1] = {
#include "ev0001/helicopter_textures/dat_0cb8e994.inc"
};
static NJS_POINT3 dat_0cb8e9ac[62] = {
#include "ev0001/helicopter_textures/dat_0cb8e9ac.inc"
};
static NJS_VECTOR dat_0cb8ec94[62] = {
#include "ev0001/helicopter_textures/dat_0cb8ec94.inc"
};
static NJS_MODEL dat_0cb8ef7c = {
#include "ev0001/helicopter_textures/dat_0cb8ef7c.inc"
};
static NJS_OBJECT dat_0cb8efa4 = {
#include "ev0001/helicopter_textures/dat_0cb8efa4.inc"
};
static NJS_MATERIAL dat_0cb8efd8[5] = {
#include "ev0001/helicopter_textures/dat_0cb8efd8.inc"
};
static Sint16 dat_0cb8f03c[80] = {
#include "ev0001/helicopter_textures/dat_0cb8f03c.inc"
};
static Sint16 dat_0cb8f0dc[28] = {
#include "ev0001/helicopter_textures/dat_0cb8f0dc.inc"
};
static Sint16 dat_0cb8f114[40] = {
#include "ev0001/helicopter_textures/dat_0cb8f114.inc"
};
static Sint16 dat_0cb8f164[12] = {
#include "ev0001/helicopter_textures/dat_0cb8f164.inc"
};
static Sint16 dat_0cb8f17c[60] = {
#include "ev0001/helicopter_textures/dat_0cb8f17c.inc"
};
static NJS_COLOR dat_0cb8f1f4[60] = {
#include "ev0001/helicopter_textures/dat_0cb8f1f4.inc"
};
static NJS_COLOR dat_0cb8f2e4[24] = {
#include "ev0001/helicopter_textures/dat_0cb8f2e4.inc"
};
static NJS_COLOR dat_0cb8f344[32] = {
#include "ev0001/helicopter_textures/dat_0cb8f344.inc"
};
static NJS_COLOR dat_0cb8f3c4[48] = {
#include "ev0001/helicopter_textures/dat_0cb8f3c4.inc"
};
static NJS_MESHSET dat_0cb8f484[5] = {
#include "ev0001/helicopter_textures/dat_0cb8f484.inc"
};
static NJS_POINT3 dat_0cb8f4fc[58] = {
#include "ev0001/helicopter_textures/dat_0cb8f4fc.inc"
};
static NJS_VECTOR dat_0cb8f7b4[58] = {
#include "ev0001/helicopter_textures/dat_0cb8f7b4.inc"
};
static NJS_MODEL dat_0cb8fa6c = {
#include "ev0001/helicopter_textures/dat_0cb8fa6c.inc"
};
static NJS_OBJECT dat_0cb8fa94 = {
#include "ev0001/helicopter_textures/dat_0cb8fa94.inc"
};
static NJS_MATERIAL dat_0cb8fac8[3] = {
#include "ev0001/helicopter_textures/dat_0cb8fac8.inc"
};
static Sint16 dat_0cb8fb04[15] = {
#include "ev0001/helicopter_textures/dat_0cb8fb04.inc"
};
static Sint16 dat_0cb8fb22[27] = {
#include "ev0001/helicopter_textures/dat_0cb8fb22.inc"
};
static Sint16 dat_0cb8fb58[16] = {
#include "ev0001/helicopter_textures/dat_0cb8fb58.inc"
};
static NJS_COLOR dat_0cb8fb78[12] = {
#include "ev0001/helicopter_textures/dat_0cb8fb78.inc"
};
static NJS_COLOR dat_0cb8fba8[24] = {
#include "ev0001/helicopter_textures/dat_0cb8fba8.inc"
};
static NJS_MESHSET dat_0cb8fc08[3] = {
#include "ev0001/helicopter_textures/dat_0cb8fc08.inc"
};
static NJS_POINT3 dat_0cb8fc50[36] = {
#include "ev0001/helicopter_textures/dat_0cb8fc50.inc"
};
static NJS_VECTOR dat_0cb8fe00[36] = {
#include "ev0001/helicopter_textures/dat_0cb8fe00.inc"
};
static NJS_MODEL dat_0cb8ffb0 = {
#include "ev0001/helicopter_textures/dat_0cb8ffb0.inc"
};
static NJS_OBJECT dat_0cb8ffd8 = {
#include "ev0001/helicopter_textures/dat_0cb8ffd8.inc"
};
static NJS_MATERIAL dat_0cb9000c[1] = {
#include "ev0001/helicopter_textures/dat_0cb9000c.inc"
};
static Sint16 dat_0cb90020[10] = {
#include "ev0001/helicopter_textures/dat_0cb90020.inc"
};
static NJS_COLOR dat_0cb90034[8] = {
#include "ev0001/helicopter_textures/dat_0cb90034.inc"
};
static NJS_MESHSET dat_0cb90054[1] = {
#include "ev0001/helicopter_textures/dat_0cb90054.inc"
};
static NJS_POINT3 dat_0cb9006c[5] = {
#include "ev0001/helicopter_textures/dat_0cb9006c.inc"
};
static NJS_VECTOR dat_0cb900a8[5] = {
#include "ev0001/helicopter_textures/dat_0cb900a8.inc"
};
static NJS_MODEL dat_0cb900e4 = {
#include "ev0001/helicopter_textures/dat_0cb900e4.inc"
};
static NJS_OBJECT dat_0cb9010c = {
#include "ev0001/helicopter_textures/dat_0cb9010c.inc"
};
static NJS_MATERIAL dat_0cb90140[1] = {
#include "ev0001/helicopter_textures/dat_0cb90140.inc"
};
static Sint16 dat_0cb90154[26] = {
#include "ev0001/helicopter_textures/dat_0cb90154.inc"
};
static NJS_COLOR dat_0cb90188[22] = {
#include "ev0001/helicopter_textures/dat_0cb90188.inc"
};
static NJS_MESHSET dat_0cb901e0[1] = {
#include "ev0001/helicopter_textures/dat_0cb901e0.inc"
};
static NJS_POINT3 dat_0cb901f8[12] = {
#include "ev0001/helicopter_textures/dat_0cb901f8.inc"
};
static NJS_VECTOR dat_0cb90288[12] = {
#include "ev0001/helicopter_textures/dat_0cb90288.inc"
};
static NJS_MODEL dat_0cb90318 = {
#include "ev0001/helicopter_textures/dat_0cb90318.inc"
};
static NJS_OBJECT dat_0cb90340 = {
#include "ev0001/helicopter_textures/dat_0cb90340.inc"
};
static NJS_MATERIAL dat_0cb90374[6] = {
#include "ev0001/helicopter_textures/dat_0cb90374.inc"
};
static Sint16 dat_0cb903ec[19] = {
#include "ev0001/helicopter_textures/dat_0cb903ec.inc"
};
static Sint16 dat_0cb90412[55] = {
#include "ev0001/helicopter_textures/dat_0cb90412.inc"
};
static Sint16 dat_0cb90480[62] = {
#include "ev0001/helicopter_textures/dat_0cb90480.inc"
};
static Sint16 dat_0cb904fc[13] = {
#include "ev0001/helicopter_textures/dat_0cb904fc.inc"
};
static Sint16 dat_0cb90516[36] = {
#include "ev0001/helicopter_textures/dat_0cb90516.inc"
};
static Sint16 dat_0cb9055e[11] = {
#include "ev0001/helicopter_textures/dat_0cb9055e.inc"
};
static NJS_COLOR dat_0cb90574[16] = {
#include "ev0001/helicopter_textures/dat_0cb90574.inc"
};
static NJS_COLOR dat_0cb905b4[48] = {
#include "ev0001/helicopter_textures/dat_0cb905b4.inc"
};
static NJS_COLOR dat_0cb90674[54] = {
#include "ev0001/helicopter_textures/dat_0cb90674.inc"
};
static NJS_COLOR dat_0cb9074c[10] = {
#include "ev0001/helicopter_textures/dat_0cb9074c.inc"
};
static NJS_COLOR dat_0cb90774[32] = {
#include "ev0001/helicopter_textures/dat_0cb90774.inc"
};
static NJS_COLOR dat_0cb907f4[8] = {
#include "ev0001/helicopter_textures/dat_0cb907f4.inc"
};
static NJS_MESHSET dat_0cb90814[6] = {
#include "ev0001/helicopter_textures/dat_0cb90814.inc"
};
static NJS_POINT3 dat_0cb908a4[67] = {
#include "ev0001/helicopter_textures/dat_0cb908a4.inc"
};
static NJS_VECTOR dat_0cb90bc8[67] = {
#include "ev0001/helicopter_textures/dat_0cb90bc8.inc"
};
static NJS_MODEL dat_0cb90eec = {
#include "ev0001/helicopter_textures/dat_0cb90eec.inc"
};
static NJS_OBJECT dat_0cb90f14 = {
#include "ev0001/helicopter_textures/dat_0cb90f14.inc"
};
static NJS_MKEY_A dat_0cb90f48[8] = {
#include "ev0001/helicopter_textures/dat_0cb90f48.inc"
};
static NJS_MDATA2 dat_0cb90fc8[13] = {
#include "ev0001/helicopter_textures/dat_0cb90fc8.inc"
};
static NJS_MOTION ev0001_helicopter_motion = {
#include "ev0001/helicopter_textures/ev0001_helicopter_motion.inc"
};
NJS_ACTION ev0001_helicopter_action = {
#include "ev0001/helicopter_textures/ev0001_helicopter_action.inc"
};
static NJS_TEXNAME ev0001_patrol_car_textures[14] = {
    {TEXT_13, 0, 0}, {TEXT_4, 0, 0},  {TEXT_14, 0, 0}, {TEXT_15, 0, 0},
    {TEXT_8, 0, 0},  {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0},
    {TEXT_19, 0, 0}, {TEXT_20, 0, 0}, {TEXT_21, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_22, 0, 0}, {TEXT_23, 0, 0},
};
NJS_TEXLIST ev0001_patrol_car_texlist = {
#include "ev0001/helicopter_textures/ev0001_patrol_car_texlist.inc"
};
NJS_MATERIAL dat_0cb9115c[1] = {
#include "ev0001/helicopter_textures/dat_0cb9115c.inc"
};
