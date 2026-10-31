/* ev00bf_amy_action_5 and its motion, and the bird's nodes, models, actions
 * 0 to 6 and two texlists */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00bf/amy_action_5.h"

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
#include "ev00bf/amy_action_5/text.inc"

extern NJS_MDATA2 dat_0cb8d310[];
extern NJS_OBJECT amy_object;

static NJS_MOTION ev00bf_amy_motion_5 = {
#include "ev00bf/amy_action_5/ev00bf_amy_motion_5.inc"
};
NJS_ACTION ev00bf_amy_action_5 = {
#include "ev00bf/amy_action_5/ev00bf_amy_action_5.inc"
};
static NJS_TEXNAME dat_0cb8d804[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev00bf_bird_texlist_1 = {
#include "ev00bf/amy_action_5/ev00bf_bird_texlist_1.inc"
};
static NJS_MATERIAL dat_0cb8d89c[1] = {
#include "ev00bf/amy_action_5/dat_0cb8d89c.inc"
};
static Sint16 dat_0cb8d8b0[6] = {
#include "ev00bf/amy_action_5/dat_0cb8d8b0.inc"
};
static NJS_COLOR dat_0cb8d8bc[4] = {
#include "ev00bf/amy_action_5/dat_0cb8d8bc.inc"
};
static NJS_MESHSET dat_0cb8d8cc[1] = {
#include "ev00bf/amy_action_5/dat_0cb8d8cc.inc"
};
static NJS_POINT3 dat_0cb8d8e4[4] = {
#include "ev00bf/amy_action_5/dat_0cb8d8e4.inc"
};
static NJS_VECTOR dat_0cb8d914[4] = {
#include "ev00bf/amy_action_5/dat_0cb8d914.inc"
};
static NJS_MODEL dat_0cb8d944 = {
#include "ev00bf/amy_action_5/dat_0cb8d944.inc"
};
static NJS_OBJECT dat_0cb8d96c = {
#include "ev00bf/amy_action_5/dat_0cb8d96c.inc"
};
static NJS_MATERIAL dat_0cb8d9a0[1] = {
#include "ev00bf/amy_action_5/dat_0cb8d9a0.inc"
};
static Sint16 dat_0cb8d9b4[12] = {
#include "ev00bf/amy_action_5/dat_0cb8d9b4.inc"
};
static NJS_COLOR dat_0cb8d9cc[9] = {
#include "ev00bf/amy_action_5/dat_0cb8d9cc.inc"
};
static NJS_MESHSET dat_0cb8d9f0[1] = {
#include "ev00bf/amy_action_5/dat_0cb8d9f0.inc"
};
static NJS_POINT3 dat_0cb8da08[7] = {
#include "ev00bf/amy_action_5/dat_0cb8da08.inc"
};
static NJS_VECTOR dat_0cb8da5c[7] = {
#include "ev00bf/amy_action_5/dat_0cb8da5c.inc"
};
static NJS_MODEL dat_0cb8dab0 = {
#include "ev00bf/amy_action_5/dat_0cb8dab0.inc"
};
static NJS_OBJECT dat_0cb8dad8 = {
#include "ev00bf/amy_action_5/dat_0cb8dad8.inc"
};
static NJS_OBJECT dat_0cb8db0c = {
#include "ev00bf/amy_action_5/dat_0cb8db0c.inc"
};
static NJS_OBJECT dat_0cb8db40 = {
#include "ev00bf/amy_action_5/dat_0cb8db40.inc"
};
static NJS_OBJECT dat_0cb8db74 = {
#include "ev00bf/amy_action_5/dat_0cb8db74.inc"
};
static NJS_OBJECT dat_0cb8dba8 = {
#include "ev00bf/amy_action_5/dat_0cb8dba8.inc"
};
static NJS_OBJECT dat_0cb8dbdc = {
#include "ev00bf/amy_action_5/dat_0cb8dbdc.inc"
};
static NJS_OBJECT dat_0cb8dc10 = {
#include "ev00bf/amy_action_5/dat_0cb8dc10.inc"
};
static NJS_MATERIAL dat_0cb8dc44[1] = {
#include "ev00bf/amy_action_5/dat_0cb8dc44.inc"
};
static Sint16 dat_0cb8dc58[6] = {
#include "ev00bf/amy_action_5/dat_0cb8dc58.inc"
};
static NJS_COLOR dat_0cb8dc64[4] = {
#include "ev00bf/amy_action_5/dat_0cb8dc64.inc"
};
static NJS_MESHSET dat_0cb8dc74[1] = {
#include "ev00bf/amy_action_5/dat_0cb8dc74.inc"
};
static NJS_POINT3 dat_0cb8dc8c[4] = {
#include "ev00bf/amy_action_5/dat_0cb8dc8c.inc"
};
static NJS_VECTOR dat_0cb8dcbc[4] = {
#include "ev00bf/amy_action_5/dat_0cb8dcbc.inc"
};
static NJS_MODEL dat_0cb8dcec = {
#include "ev00bf/amy_action_5/dat_0cb8dcec.inc"
};
static NJS_OBJECT dat_0cb8dd14 = {
#include "ev00bf/amy_action_5/dat_0cb8dd14.inc"
};
static NJS_MATERIAL dat_0cb8dd48[1] = {
#include "ev00bf/amy_action_5/dat_0cb8dd48.inc"
};
static Sint16 dat_0cb8dd5c[12] = {
#include "ev00bf/amy_action_5/dat_0cb8dd5c.inc"
};
static NJS_COLOR dat_0cb8dd74[9] = {
#include "ev00bf/amy_action_5/dat_0cb8dd74.inc"
};
static NJS_MESHSET dat_0cb8dd98[1] = {
#include "ev00bf/amy_action_5/dat_0cb8dd98.inc"
};
static NJS_POINT3 dat_0cb8ddb0[7] = {
#include "ev00bf/amy_action_5/dat_0cb8ddb0.inc"
};
static NJS_VECTOR dat_0cb8de04[7] = {
#include "ev00bf/amy_action_5/dat_0cb8de04.inc"
};
static NJS_MODEL dat_0cb8de58 = {
#include "ev00bf/amy_action_5/dat_0cb8de58.inc"
};
static NJS_OBJECT dat_0cb8de80 = {
#include "ev00bf/amy_action_5/dat_0cb8de80.inc"
};
static NJS_OBJECT dat_0cb8deb4 = {
#include "ev00bf/amy_action_5/dat_0cb8deb4.inc"
};
static NJS_OBJECT dat_0cb8dee8 = {
#include "ev00bf/amy_action_5/dat_0cb8dee8.inc"
};
static NJS_OBJECT dat_0cb8df1c = {
#include "ev00bf/amy_action_5/dat_0cb8df1c.inc"
};
static NJS_OBJECT dat_0cb8df50 = {
#include "ev00bf/amy_action_5/dat_0cb8df50.inc"
};
static NJS_OBJECT dat_0cb8df84 = {
#include "ev00bf/amy_action_5/dat_0cb8df84.inc"
};
static NJS_OBJECT dat_0cb8dfb8 = {
#include "ev00bf/amy_action_5/dat_0cb8dfb8.inc"
};
static NJS_OBJECT dat_0cb8dfec = {
#include "ev00bf/amy_action_5/dat_0cb8dfec.inc"
};
static NJS_OBJECT dat_0cb8e020 = {
#include "ev00bf/amy_action_5/dat_0cb8e020.inc"
};
static NJS_OBJECT dat_0cb8e054 = {
#include "ev00bf/amy_action_5/dat_0cb8e054.inc"
};
static NJS_OBJECT dat_0cb8e088 = {
#include "ev00bf/amy_action_5/dat_0cb8e088.inc"
};
static NJS_OBJECT dat_0cb8e0bc = {
#include "ev00bf/amy_action_5/dat_0cb8e0bc.inc"
};
static NJS_MATERIAL dat_0cb8e0f0[1] = {
#include "ev00bf/amy_action_5/dat_0cb8e0f0.inc"
};
static Sint16 dat_0cb8e104[18] = {
#include "ev00bf/amy_action_5/dat_0cb8e104.inc"
};
static NJS_COLOR dat_0cb8e128[15] = {
#include "ev00bf/amy_action_5/dat_0cb8e128.inc"
};
static NJS_MESHSET dat_0cb8e164[1] = {
#include "ev00bf/amy_action_5/dat_0cb8e164.inc"
};
static NJS_POINT3 dat_0cb8e17c[7] = {
#include "ev00bf/amy_action_5/dat_0cb8e17c.inc"
};
static NJS_VECTOR dat_0cb8e1d0[7] = {
#include "ev00bf/amy_action_5/dat_0cb8e1d0.inc"
};
static NJS_MODEL dat_0cb8e224 = {
#include "ev00bf/amy_action_5/dat_0cb8e224.inc"
};
static NJS_OBJECT dat_0cb8e24c = {
#include "ev00bf/amy_action_5/dat_0cb8e24c.inc"
};
static NJS_OBJECT dat_0cb8e280 = {
#include "ev00bf/amy_action_5/dat_0cb8e280.inc"
};
static NJS_OBJECT dat_0cb8e2b4 = {
#include "ev00bf/amy_action_5/dat_0cb8e2b4.inc"
};
static NJS_OBJECT dat_0cb8e2e8 = {
#include "ev00bf/amy_action_5/dat_0cb8e2e8.inc"
};
static NJS_MATERIAL dat_0cb8e31c[1] = {
#include "ev00bf/amy_action_5/dat_0cb8e31c.inc"
};
static Sint16 dat_0cb8e330[10] = {
#include "ev00bf/amy_action_5/dat_0cb8e330.inc"
};
static NJS_COLOR dat_0cb8e344[8] = {
#include "ev00bf/amy_action_5/dat_0cb8e344.inc"
};
static NJS_MESHSET dat_0cb8e364[1] = {
#include "ev00bf/amy_action_5/dat_0cb8e364.inc"
};
static NJS_POINT3 dat_0cb8e37c[8] = {
#include "ev00bf/amy_action_5/dat_0cb8e37c.inc"
};
static NJS_VECTOR dat_0cb8e3dc[8] = {
#include "ev00bf/amy_action_5/dat_0cb8e3dc.inc"
};
static NJS_MODEL dat_0cb8e43c = {
#include "ev00bf/amy_action_5/dat_0cb8e43c.inc"
};
static NJS_OBJECT dat_0cb8e464 = {
#include "ev00bf/amy_action_5/dat_0cb8e464.inc"
};
static NJS_MATERIAL dat_0cb8e498[1] = {
#include "ev00bf/amy_action_5/dat_0cb8e498.inc"
};
static Sint16 dat_0cb8e4ac[10] = {
#include "ev00bf/amy_action_5/dat_0cb8e4ac.inc"
};
static NJS_COLOR dat_0cb8e4c0[8] = {
#include "ev00bf/amy_action_5/dat_0cb8e4c0.inc"
};
static NJS_MESHSET dat_0cb8e4e0[1] = {
#include "ev00bf/amy_action_5/dat_0cb8e4e0.inc"
};
static NJS_POINT3 dat_0cb8e4f8[8] = {
#include "ev00bf/amy_action_5/dat_0cb8e4f8.inc"
};
static NJS_VECTOR dat_0cb8e558[8] = {
#include "ev00bf/amy_action_5/dat_0cb8e558.inc"
};
static NJS_MODEL dat_0cb8e5b8 = {
#include "ev00bf/amy_action_5/dat_0cb8e5b8.inc"
};
static NJS_OBJECT dat_0cb8e5e0 = {
#include "ev00bf/amy_action_5/dat_0cb8e5e0.inc"
};
static NJS_MATERIAL dat_0cb8e614[1] = {
#include "ev00bf/amy_action_5/dat_0cb8e614.inc"
};
static Sint16 dat_0cb8e628[16] = {
#include "ev00bf/amy_action_5/dat_0cb8e628.inc"
};
static NJS_COLOR dat_0cb8e648[12] = {
#include "ev00bf/amy_action_5/dat_0cb8e648.inc"
};
static NJS_MESHSET dat_0cb8e678[1] = {
#include "ev00bf/amy_action_5/dat_0cb8e678.inc"
};
static NJS_POINT3 dat_0cb8e690[12] = {
#include "ev00bf/amy_action_5/dat_0cb8e690.inc"
};
static NJS_VECTOR dat_0cb8e720[12] = {
#include "ev00bf/amy_action_5/dat_0cb8e720.inc"
};
static NJS_MODEL dat_0cb8e7b0 = {
#include "ev00bf/amy_action_5/dat_0cb8e7b0.inc"
};
static NJS_OBJECT dat_0cb8e7d8 = {
#include "ev00bf/amy_action_5/dat_0cb8e7d8.inc"
};
static NJS_MATERIAL dat_0cb8e80c[1] = {
#include "ev00bf/amy_action_5/dat_0cb8e80c.inc"
};
static Sint16 dat_0cb8e820[16] = {
#include "ev00bf/amy_action_5/dat_0cb8e820.inc"
};
static NJS_COLOR dat_0cb8e840[12] = {
#include "ev00bf/amy_action_5/dat_0cb8e840.inc"
};
static NJS_MESHSET dat_0cb8e870[1] = {
#include "ev00bf/amy_action_5/dat_0cb8e870.inc"
};
static NJS_POINT3 dat_0cb8e888[12] = {
#include "ev00bf/amy_action_5/dat_0cb8e888.inc"
};
static NJS_VECTOR dat_0cb8e918[12] = {
#include "ev00bf/amy_action_5/dat_0cb8e918.inc"
};
static NJS_MODEL dat_0cb8e9a8 = {
#include "ev00bf/amy_action_5/dat_0cb8e9a8.inc"
};
static NJS_OBJECT dat_0cb8e9d0 = {
#include "ev00bf/amy_action_5/dat_0cb8e9d0.inc"
};
static NJS_MATERIAL dat_0cb8ea04[4] = {
#include "ev00bf/amy_action_5/dat_0cb8ea04.inc"
};
static Sint16 dat_0cb8ea54[239] = {
#include "ev00bf/amy_action_5/dat_0cb8ea54.inc"
};
static Sint16 dat_0cb8ec32[22] = {
#include "ev00bf/amy_action_5/dat_0cb8ec32.inc"
};
static Sint16 dat_0cb8ec5e[21] = {
#include "ev00bf/amy_action_5/dat_0cb8ec5e.inc"
};
static Sint16 dat_0cb8ec88[22] = {
#include "ev00bf/amy_action_5/dat_0cb8ec88.inc"
};
static NJS_COLOR dat_0cb8ecb4[209] = {
#include "ev00bf/amy_action_5/dat_0cb8ecb4.inc"
};
static NJS_COLOR dat_0cb8eff8[20] = {
#include "ev00bf/amy_action_5/dat_0cb8eff8.inc"
};
static NJS_COLOR dat_0cb8f048[16] = {
#include "ev00bf/amy_action_5/dat_0cb8f048.inc"
};
static NJS_COLOR dat_0cb8f088[18] = {
#include "ev00bf/amy_action_5/dat_0cb8f088.inc"
};
static NJS_MESHSET dat_0cb8f0d0[4] = {
#include "ev00bf/amy_action_5/dat_0cb8f0d0.inc"
};
static NJS_POINT3 dat_0cb8f130[127] = {
#include "ev00bf/amy_action_5/dat_0cb8f130.inc"
};
static NJS_VECTOR dat_0cb8f724[127] = {
#include "ev00bf/amy_action_5/dat_0cb8f724.inc"
};
static NJS_MODEL dat_0cb8fd18 = {
#include "ev00bf/amy_action_5/dat_0cb8fd18.inc"
};
static NJS_OBJECT dat_0cb8fd40 = {
#include "ev00bf/amy_action_5/dat_0cb8fd40.inc"
};
static NJS_OBJECT dat_0cb8fd74 = {
#include "ev00bf/amy_action_5/dat_0cb8fd74.inc"
};
static NJS_OBJECT dat_0cb8fda8 = {
#include "ev00bf/amy_action_5/dat_0cb8fda8.inc"
};
static NJS_OBJECT dat_0cb8fddc = {
#include "ev00bf/amy_action_5/dat_0cb8fddc.inc"
};
static NJS_MATERIAL dat_0cb8fe10[1] = {
#include "ev00bf/amy_action_5/dat_0cb8fe10.inc"
};
static Sint16 dat_0cb8fe24[8] = {
#include "ev00bf/amy_action_5/dat_0cb8fe24.inc"
};
static NJS_COLOR dat_0cb8fe34[6] = {
#include "ev00bf/amy_action_5/dat_0cb8fe34.inc"
};
static NJS_MESHSET dat_0cb8fe4c[1] = {
#include "ev00bf/amy_action_5/dat_0cb8fe4c.inc"
};
static NJS_POINT3 dat_0cb8fe64[6] = {
#include "ev00bf/amy_action_5/dat_0cb8fe64.inc"
};
static NJS_VECTOR dat_0cb8feac[6] = {
#include "ev00bf/amy_action_5/dat_0cb8feac.inc"
};
static NJS_MODEL dat_0cb8fef4 = {
#include "ev00bf/amy_action_5/dat_0cb8fef4.inc"
};
static NJS_OBJECT dat_0cb8ff1c = {
#include "ev00bf/amy_action_5/dat_0cb8ff1c.inc"
};
static NJS_MATERIAL dat_0cb8ff50[1] = {
#include "ev00bf/amy_action_5/dat_0cb8ff50.inc"
};
static Sint16 dat_0cb8ff64[22] = {
#include "ev00bf/amy_action_5/dat_0cb8ff64.inc"
};
static NJS_COLOR dat_0cb8ff90[16] = {
#include "ev00bf/amy_action_5/dat_0cb8ff90.inc"
};
static NJS_MESHSET dat_0cb8ffd0[1] = {
#include "ev00bf/amy_action_5/dat_0cb8ffd0.inc"
};
static NJS_POINT3 dat_0cb8ffe8[5] = {
#include "ev00bf/amy_action_5/dat_0cb8ffe8.inc"
};
static NJS_VECTOR dat_0cb90024[5] = {
#include "ev00bf/amy_action_5/dat_0cb90024.inc"
};
static NJS_MODEL dat_0cb90060 = {
#include "ev00bf/amy_action_5/dat_0cb90060.inc"
};
static NJS_OBJECT dat_0cb90088 = {
#include "ev00bf/amy_action_5/dat_0cb90088.inc"
};
static NJS_MATERIAL dat_0cb900bc[1] = {
#include "ev00bf/amy_action_5/dat_0cb900bc.inc"
};
static Sint16 dat_0cb900d0[30] = {
#include "ev00bf/amy_action_5/dat_0cb900d0.inc"
};
static NJS_COLOR dat_0cb9010c[28] = {
#include "ev00bf/amy_action_5/dat_0cb9010c.inc"
};
static NJS_MESHSET dat_0cb9017c[1] = {
#include "ev00bf/amy_action_5/dat_0cb9017c.inc"
};
static NJS_POINT3 dat_0cb90194[18] = {
#include "ev00bf/amy_action_5/dat_0cb90194.inc"
};
static NJS_VECTOR dat_0cb9026c[18] = {
#include "ev00bf/amy_action_5/dat_0cb9026c.inc"
};
static NJS_MODEL dat_0cb90344 = {
#include "ev00bf/amy_action_5/dat_0cb90344.inc"
};
static NJS_OBJECT dat_0cb9036c = {
#include "ev00bf/amy_action_5/dat_0cb9036c.inc"
};
static NJS_MATERIAL dat_0cb903a0[1] = {
#include "ev00bf/amy_action_5/dat_0cb903a0.inc"
};
static Sint16 dat_0cb903b4[20] = {
#include "ev00bf/amy_action_5/dat_0cb903b4.inc"
};
static NJS_COLOR dat_0cb903dc[16] = {
#include "ev00bf/amy_action_5/dat_0cb903dc.inc"
};
static NJS_MESHSET dat_0cb9041c[1] = {
#include "ev00bf/amy_action_5/dat_0cb9041c.inc"
};
static NJS_POINT3 dat_0cb90434[12] = {
#include "ev00bf/amy_action_5/dat_0cb90434.inc"
};
static NJS_VECTOR dat_0cb904c4[12] = {
#include "ev00bf/amy_action_5/dat_0cb904c4.inc"
};
static NJS_MODEL dat_0cb90554 = {
#include "ev00bf/amy_action_5/dat_0cb90554.inc"
};
static NJS_OBJECT dat_0cb9057c = {
#include "ev00bf/amy_action_5/dat_0cb9057c.inc"
};
static NJS_MATERIAL dat_0cb905b0[1] = {
#include "ev00bf/amy_action_5/dat_0cb905b0.inc"
};
static Sint16 dat_0cb905c4[10] = {
#include "ev00bf/amy_action_5/dat_0cb905c4.inc"
};
static NJS_COLOR dat_0cb905d8[8] = {
#include "ev00bf/amy_action_5/dat_0cb905d8.inc"
};
static NJS_MESHSET dat_0cb905f8[1] = {
#include "ev00bf/amy_action_5/dat_0cb905f8.inc"
};
static NJS_POINT3 dat_0cb90610[5] = {
#include "ev00bf/amy_action_5/dat_0cb90610.inc"
};
static NJS_VECTOR dat_0cb9064c[5] = {
#include "ev00bf/amy_action_5/dat_0cb9064c.inc"
};
static NJS_MODEL dat_0cb90688 = {
#include "ev00bf/amy_action_5/dat_0cb90688.inc"
};
static NJS_OBJECT dat_0cb906b0 = {
#include "ev00bf/amy_action_5/dat_0cb906b0.inc"
};
static NJS_MATERIAL dat_0cb906e4[1] = {
#include "ev00bf/amy_action_5/dat_0cb906e4.inc"
};
static Sint16 dat_0cb906f8[10] = {
#include "ev00bf/amy_action_5/dat_0cb906f8.inc"
};
static NJS_COLOR dat_0cb9070c[8] = {
#include "ev00bf/amy_action_5/dat_0cb9070c.inc"
};
static NJS_MESHSET dat_0cb9072c[1] = {
#include "ev00bf/amy_action_5/dat_0cb9072c.inc"
};
static NJS_POINT3 dat_0cb90744[5] = {
#include "ev00bf/amy_action_5/dat_0cb90744.inc"
};
static NJS_VECTOR dat_0cb90780[5] = {
#include "ev00bf/amy_action_5/dat_0cb90780.inc"
};
static NJS_MODEL dat_0cb907bc = {
#include "ev00bf/amy_action_5/dat_0cb907bc.inc"
};
static NJS_OBJECT dat_0cb907e4 = {
#include "ev00bf/amy_action_5/dat_0cb907e4.inc"
};
static NJS_MATERIAL dat_0cb90818[1] = {
#include "ev00bf/amy_action_5/dat_0cb90818.inc"
};
static Sint16 dat_0cb9082c[24] = {
#include "ev00bf/amy_action_5/dat_0cb9082c.inc"
};
static NJS_COLOR dat_0cb9085c[20] = {
#include "ev00bf/amy_action_5/dat_0cb9085c.inc"
};
static NJS_MESHSET dat_0cb908ac[1] = {
#include "ev00bf/amy_action_5/dat_0cb908ac.inc"
};
static NJS_POINT3 dat_0cb908c4[12] = {
#include "ev00bf/amy_action_5/dat_0cb908c4.inc"
};
static NJS_VECTOR dat_0cb90954[12] = {
#include "ev00bf/amy_action_5/dat_0cb90954.inc"
};
static NJS_MODEL dat_0cb909e4 = {
#include "ev00bf/amy_action_5/dat_0cb909e4.inc"
};
static NJS_OBJECT dat_0cb90a0c = {
#include "ev00bf/amy_action_5/dat_0cb90a0c.inc"
};
static NJS_MATERIAL dat_0cb90a40[1] = {
#include "ev00bf/amy_action_5/dat_0cb90a40.inc"
};
static Sint16 dat_0cb90a54[36] = {
#include "ev00bf/amy_action_5/dat_0cb90a54.inc"
};
static NJS_COLOR dat_0cb90a9c[30] = {
#include "ev00bf/amy_action_5/dat_0cb90a9c.inc"
};
static NJS_MESHSET dat_0cb90b14[1] = {
#include "ev00bf/amy_action_5/dat_0cb90b14.inc"
};
static NJS_POINT3 dat_0cb90b2c[12] = {
#include "ev00bf/amy_action_5/dat_0cb90b2c.inc"
};
static NJS_VECTOR dat_0cb90bbc[12] = {
#include "ev00bf/amy_action_5/dat_0cb90bbc.inc"
};
static NJS_MODEL dat_0cb90c4c = {
#include "ev00bf/amy_action_5/dat_0cb90c4c.inc"
};
static NJS_OBJECT dat_0cb90c74 = {
#include "ev00bf/amy_action_5/dat_0cb90c74.inc"
};
static NJS_OBJECT dat_0cb90ca8 = {
#include "ev00bf/amy_action_5/dat_0cb90ca8.inc"
};
static NJS_OBJECT dat_0cb90cdc = {
#include "ev00bf/amy_action_5/dat_0cb90cdc.inc"
};
static NJS_OBJECT dat_0cb90d10 = {
#include "ev00bf/amy_action_5/dat_0cb90d10.inc"
};
static NJS_MATERIAL dat_0cb90d44[1] = {
#include "ev00bf/amy_action_5/dat_0cb90d44.inc"
};
static Sint16 dat_0cb90d58[48] = {
#include "ev00bf/amy_action_5/dat_0cb90d58.inc"
};
static NJS_COLOR dat_0cb90db8[38] = {
#include "ev00bf/amy_action_5/dat_0cb90db8.inc"
};
static NJS_MESHSET dat_0cb90e50[1] = {
#include "ev00bf/amy_action_5/dat_0cb90e50.inc"
};
static NJS_POINT3 dat_0cb90e68[12] = {
#include "ev00bf/amy_action_5/dat_0cb90e68.inc"
};
static NJS_VECTOR dat_0cb90ef8[12] = {
#include "ev00bf/amy_action_5/dat_0cb90ef8.inc"
};
static NJS_MODEL dat_0cb90f88 = {
#include "ev00bf/amy_action_5/dat_0cb90f88.inc"
};
static NJS_OBJECT dat_0cb90fb0 = {
#include "ev00bf/amy_action_5/dat_0cb90fb0.inc"
};
static NJS_OBJECT dat_0cb90fe4 = {
#include "ev00bf/amy_action_5/dat_0cb90fe4.inc"
};
static NJS_OBJECT dat_0cb91018 = {
#include "ev00bf/amy_action_5/dat_0cb91018.inc"
};
static NJS_OBJECT dat_0cb9104c = {
#include "ev00bf/amy_action_5/dat_0cb9104c.inc"
};
static NJS_MATERIAL dat_0cb91080[1] = {
#include "ev00bf/amy_action_5/dat_0cb91080.inc"
};
static Sint16 dat_0cb91094[48] = {
#include "ev00bf/amy_action_5/dat_0cb91094.inc"
};
static NJS_COLOR dat_0cb910f4[38] = {
#include "ev00bf/amy_action_5/dat_0cb910f4.inc"
};
static NJS_MESHSET dat_0cb9118c[1] = {
#include "ev00bf/amy_action_5/dat_0cb9118c.inc"
};
static NJS_POINT3 dat_0cb911a4[12] = {
#include "ev00bf/amy_action_5/dat_0cb911a4.inc"
};
static NJS_VECTOR dat_0cb91234[12] = {
#include "ev00bf/amy_action_5/dat_0cb91234.inc"
};
static NJS_MODEL dat_0cb912c4 = {
#include "ev00bf/amy_action_5/dat_0cb912c4.inc"
};
static NJS_OBJECT dat_0cb912ec = {
#include "ev00bf/amy_action_5/dat_0cb912ec.inc"
};
static NJS_OBJECT dat_0cb91320 = {
#include "ev00bf/amy_action_5/dat_0cb91320.inc"
};
static NJS_OBJECT dat_0cb91354 = {
#include "ev00bf/amy_action_5/dat_0cb91354.inc"
};
static NJS_OBJECT dat_0cb91388 = {
#include "ev00bf/amy_action_5/dat_0cb91388.inc"
};
static NJS_MATERIAL dat_0cb913bc[2] = {
#include "ev00bf/amy_action_5/dat_0cb913bc.inc"
};
static Sint16 dat_0cb913e4[54] = {
#include "ev00bf/amy_action_5/dat_0cb913e4.inc"
};
static Sint16 dat_0cb91450[22] = {
#include "ev00bf/amy_action_5/dat_0cb91450.inc"
};
static NJS_COLOR dat_0cb9147c[48] = {
#include "ev00bf/amy_action_5/dat_0cb9147c.inc"
};
static NJS_COLOR dat_0cb9153c[18] = {
#include "ev00bf/amy_action_5/dat_0cb9153c.inc"
};
static NJS_MESHSET dat_0cb91584[2] = {
#include "ev00bf/amy_action_5/dat_0cb91584.inc"
};
static NJS_POINT3 dat_0cb915b4[26] = {
#include "ev00bf/amy_action_5/dat_0cb915b4.inc"
};
static NJS_VECTOR dat_0cb916ec[26] = {
#include "ev00bf/amy_action_5/dat_0cb916ec.inc"
};
static NJS_MODEL dat_0cb91824 = {
#include "ev00bf/amy_action_5/dat_0cb91824.inc"
};
static NJS_OBJECT dat_0cb9184c = {
#include "ev00bf/amy_action_5/dat_0cb9184c.inc"
};
static NJS_OBJECT dat_0cb91880 = {
#include "ev00bf/amy_action_5/dat_0cb91880.inc"
};
static NJS_OBJECT dat_0cb918b4 = {
#include "ev00bf/amy_action_5/dat_0cb918b4.inc"
};
static NJS_OBJECT dat_0cb918e8 = {
#include "ev00bf/amy_action_5/dat_0cb918e8.inc"
};
static NJS_OBJECT dat_0cb9191c = {
#include "ev00bf/amy_action_5/dat_0cb9191c.inc"
};
static NJS_MKEY_F dat_0cb91950[2] = {
#include "ev00bf/amy_action_5/dat_0cb91950.inc"
};
static NJS_MKEY_A dat_0cb91970[2] = {
#include "ev00bf/amy_action_5/dat_0cb91970.inc"
};
static NJS_MKEY_F dat_0cb91990[2] = {
#include "ev00bf/amy_action_5/dat_0cb91990.inc"
};
static NJS_MKEY_F dat_0cb919b0[2] = {
#include "ev00bf/amy_action_5/dat_0cb919b0.inc"
};
static NJS_MKEY_A dat_0cb919d0[2] = {
#include "ev00bf/amy_action_5/dat_0cb919d0.inc"
};
static NJS_MKEY_A dat_0cb919f0[2] = {
#include "ev00bf/amy_action_5/dat_0cb919f0.inc"
};
static NJS_MKEY_F dat_0cb91a10[2] = {
#include "ev00bf/amy_action_5/dat_0cb91a10.inc"
};
static NJS_MKEY_F dat_0cb91a30[2] = {
#include "ev00bf/amy_action_5/dat_0cb91a30.inc"
};
static NJS_MKEY_A dat_0cb91a50[2] = {
#include "ev00bf/amy_action_5/dat_0cb91a50.inc"
};
static NJS_MKEY_A dat_0cb91a70[2] = {
#include "ev00bf/amy_action_5/dat_0cb91a70.inc"
};
static NJS_MKEY_F dat_0cb91a90[2] = {
#include "ev00bf/amy_action_5/dat_0cb91a90.inc"
};
static NJS_MKEY_F dat_0cb91ab0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91ab0.inc"
};
static NJS_MKEY_A dat_0cb91ad0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91ad0.inc"
};
static NJS_MKEY_A dat_0cb91af0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91af0.inc"
};
static NJS_MKEY_F dat_0cb91b10[2] = {
#include "ev00bf/amy_action_5/dat_0cb91b10.inc"
};
static NJS_MKEY_F dat_0cb91b30[2] = {
#include "ev00bf/amy_action_5/dat_0cb91b30.inc"
};
static NJS_MKEY_A dat_0cb91b50[2] = {
#include "ev00bf/amy_action_5/dat_0cb91b50.inc"
};
static NJS_MKEY_F dat_0cb91b70[2] = {
#include "ev00bf/amy_action_5/dat_0cb91b70.inc"
};
static NJS_MKEY_A dat_0cb91b90[2] = {
#include "ev00bf/amy_action_5/dat_0cb91b90.inc"
};
static NJS_MKEY_F dat_0cb91bb0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91bb0.inc"
};
static NJS_MKEY_A dat_0cb91bd0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91bd0.inc"
};
static NJS_MKEY_F dat_0cb91bf0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91bf0.inc"
};
static NJS_MKEY_A dat_0cb91c10[2] = {
#include "ev00bf/amy_action_5/dat_0cb91c10.inc"
};
static NJS_MKEY_A dat_0cb91c30[2] = {
#include "ev00bf/amy_action_5/dat_0cb91c30.inc"
};
static NJS_MKEY_F dat_0cb91c50[2] = {
#include "ev00bf/amy_action_5/dat_0cb91c50.inc"
};
static NJS_MKEY_F dat_0cb91c70[2] = {
#include "ev00bf/amy_action_5/dat_0cb91c70.inc"
};
static NJS_MKEY_A dat_0cb91c90[2] = {
#include "ev00bf/amy_action_5/dat_0cb91c90.inc"
};
static NJS_MKEY_F dat_0cb91cb0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91cb0.inc"
};
static NJS_MKEY_A dat_0cb91cd0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91cd0.inc"
};
static NJS_MKEY_F dat_0cb91cf0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91cf0.inc"
};
static NJS_MKEY_A dat_0cb91d10[2] = {
#include "ev00bf/amy_action_5/dat_0cb91d10.inc"
};
static NJS_MKEY_F dat_0cb91d30[2] = {
#include "ev00bf/amy_action_5/dat_0cb91d30.inc"
};
static NJS_MKEY_A dat_0cb91d50[2] = {
#include "ev00bf/amy_action_5/dat_0cb91d50.inc"
};
static NJS_MKEY_F dat_0cb91d70[2] = {
#include "ev00bf/amy_action_5/dat_0cb91d70.inc"
};
static NJS_MKEY_A dat_0cb91d90[2] = {
#include "ev00bf/amy_action_5/dat_0cb91d90.inc"
};
static NJS_MKEY_A dat_0cb91db0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91db0.inc"
};
static NJS_MKEY_F dat_0cb91dd0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91dd0.inc"
};
static NJS_MKEY_F dat_0cb91df0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91df0.inc"
};
static NJS_MKEY_A dat_0cb91e10[2] = {
#include "ev00bf/amy_action_5/dat_0cb91e10.inc"
};
static NJS_MKEY_F dat_0cb91e30[2] = {
#include "ev00bf/amy_action_5/dat_0cb91e30.inc"
};
static NJS_MKEY_A dat_0cb91e50[2] = {
#include "ev00bf/amy_action_5/dat_0cb91e50.inc"
};
static NJS_MKEY_F dat_0cb91e70[2] = {
#include "ev00bf/amy_action_5/dat_0cb91e70.inc"
};
static NJS_MKEY_A dat_0cb91e90[2] = {
#include "ev00bf/amy_action_5/dat_0cb91e90.inc"
};
static NJS_MKEY_F dat_0cb91eb0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91eb0.inc"
};
static NJS_MKEY_A dat_0cb91ed0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91ed0.inc"
};
static NJS_MKEY_F dat_0cb91ef0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91ef0.inc"
};
static NJS_MKEY_A dat_0cb91f10[2] = {
#include "ev00bf/amy_action_5/dat_0cb91f10.inc"
};
static NJS_MKEY_F dat_0cb91f30[2] = {
#include "ev00bf/amy_action_5/dat_0cb91f30.inc"
};
static NJS_MKEY_A dat_0cb91f50[2] = {
#include "ev00bf/amy_action_5/dat_0cb91f50.inc"
};
static NJS_MKEY_F dat_0cb91f70[2] = {
#include "ev00bf/amy_action_5/dat_0cb91f70.inc"
};
static NJS_MKEY_A dat_0cb91f90[2] = {
#include "ev00bf/amy_action_5/dat_0cb91f90.inc"
};
static NJS_MKEY_F dat_0cb91fb0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91fb0.inc"
};
static NJS_MKEY_F dat_0cb91fd0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91fd0.inc"
};
static NJS_MKEY_F dat_0cb91ff0[2] = {
#include "ev00bf/amy_action_5/dat_0cb91ff0.inc"
};
static NJS_MKEY_A dat_0cb92010[2] = {
#include "ev00bf/amy_action_5/dat_0cb92010.inc"
};
static NJS_MKEY_F dat_0cb92030[2] = {
#include "ev00bf/amy_action_5/dat_0cb92030.inc"
};
static NJS_MKEY_A dat_0cb92050[2] = {
#include "ev00bf/amy_action_5/dat_0cb92050.inc"
};
static NJS_MKEY_F dat_0cb92070[2] = {
#include "ev00bf/amy_action_5/dat_0cb92070.inc"
};
static NJS_MDATA2 dat_0cb92090[57] = {
#include "ev00bf/amy_action_5/dat_0cb92090.inc"
};
static NJS_MOTION ev00bf_bird_motion_0 = {
#include "ev00bf/amy_action_5/ev00bf_bird_motion_0.inc"
};
NJS_ACTION ev00bf_bird_action_0 = {
#include "ev00bf/amy_action_5/ev00bf_bird_action_0.inc"
};
static NJS_MKEY_F dat_0cb92434[2] = {
#include "ev00bf/amy_action_5/dat_0cb92434.inc"
};
static NJS_MKEY_A dat_0cb92454[2] = {
#include "ev00bf/amy_action_5/dat_0cb92454.inc"
};
static NJS_MKEY_F dat_0cb92474[2] = {
#include "ev00bf/amy_action_5/dat_0cb92474.inc"
};
static NJS_MKEY_F dat_0cb92494[2] = {
#include "ev00bf/amy_action_5/dat_0cb92494.inc"
};
static NJS_MKEY_A dat_0cb924b4[2] = {
#include "ev00bf/amy_action_5/dat_0cb924b4.inc"
};
static NJS_MKEY_A dat_0cb924d4[2] = {
#include "ev00bf/amy_action_5/dat_0cb924d4.inc"
};
static NJS_MKEY_F dat_0cb924f4[2] = {
#include "ev00bf/amy_action_5/dat_0cb924f4.inc"
};
static NJS_MKEY_F dat_0cb92514[2] = {
#include "ev00bf/amy_action_5/dat_0cb92514.inc"
};
static NJS_MKEY_A dat_0cb92534[2] = {
#include "ev00bf/amy_action_5/dat_0cb92534.inc"
};
static NJS_MKEY_A dat_0cb92554[2] = {
#include "ev00bf/amy_action_5/dat_0cb92554.inc"
};
static NJS_MKEY_F dat_0cb92574[2] = {
#include "ev00bf/amy_action_5/dat_0cb92574.inc"
};
static NJS_MKEY_F dat_0cb92594[2] = {
#include "ev00bf/amy_action_5/dat_0cb92594.inc"
};
static NJS_MKEY_A dat_0cb925b4[2] = {
#include "ev00bf/amy_action_5/dat_0cb925b4.inc"
};
static NJS_MKEY_A dat_0cb925d4[2] = {
#include "ev00bf/amy_action_5/dat_0cb925d4.inc"
};
static NJS_MKEY_F dat_0cb925f4[2] = {
#include "ev00bf/amy_action_5/dat_0cb925f4.inc"
};
static NJS_MKEY_F dat_0cb92614[2] = {
#include "ev00bf/amy_action_5/dat_0cb92614.inc"
};
static NJS_MKEY_A dat_0cb92634[2] = {
#include "ev00bf/amy_action_5/dat_0cb92634.inc"
};
static NJS_MKEY_F dat_0cb92654[2] = {
#include "ev00bf/amy_action_5/dat_0cb92654.inc"
};
static NJS_MKEY_A dat_0cb92674[2] = {
#include "ev00bf/amy_action_5/dat_0cb92674.inc"
};
static NJS_MKEY_F dat_0cb92694[2] = {
#include "ev00bf/amy_action_5/dat_0cb92694.inc"
};
static NJS_MKEY_A dat_0cb926b4[2] = {
#include "ev00bf/amy_action_5/dat_0cb926b4.inc"
};
static NJS_MKEY_F dat_0cb926d4[2] = {
#include "ev00bf/amy_action_5/dat_0cb926d4.inc"
};
static NJS_MKEY_A dat_0cb926f4[2] = {
#include "ev00bf/amy_action_5/dat_0cb926f4.inc"
};
static NJS_MKEY_A dat_0cb92714[29] = {
#include "ev00bf/amy_action_5/dat_0cb92714.inc"
};
static NJS_MKEY_F dat_0cb928e4[2] = {
#include "ev00bf/amy_action_5/dat_0cb928e4.inc"
};
static NJS_MKEY_F dat_0cb92904[2] = {
#include "ev00bf/amy_action_5/dat_0cb92904.inc"
};
static NJS_MKEY_A dat_0cb92924[2] = {
#include "ev00bf/amy_action_5/dat_0cb92924.inc"
};
static NJS_MKEY_F dat_0cb92944[2] = {
#include "ev00bf/amy_action_5/dat_0cb92944.inc"
};
static NJS_MKEY_A dat_0cb92964[2] = {
#include "ev00bf/amy_action_5/dat_0cb92964.inc"
};
static NJS_MKEY_F dat_0cb92984[2] = {
#include "ev00bf/amy_action_5/dat_0cb92984.inc"
};
static NJS_MKEY_A dat_0cb929a4[2] = {
#include "ev00bf/amy_action_5/dat_0cb929a4.inc"
};
static NJS_MKEY_F dat_0cb929c4[2] = {
#include "ev00bf/amy_action_5/dat_0cb929c4.inc"
};
static NJS_MKEY_A dat_0cb929e4[2] = {
#include "ev00bf/amy_action_5/dat_0cb929e4.inc"
};
static NJS_MKEY_F dat_0cb92a04[2] = {
#include "ev00bf/amy_action_5/dat_0cb92a04.inc"
};
static NJS_MKEY_A dat_0cb92a24[2] = {
#include "ev00bf/amy_action_5/dat_0cb92a24.inc"
};
static NJS_MKEY_A dat_0cb92a44[2] = {
#include "ev00bf/amy_action_5/dat_0cb92a44.inc"
};
static NJS_MKEY_F dat_0cb92a64[2] = {
#include "ev00bf/amy_action_5/dat_0cb92a64.inc"
};
static NJS_MKEY_F dat_0cb92a84[2] = {
#include "ev00bf/amy_action_5/dat_0cb92a84.inc"
};
static NJS_MKEY_A dat_0cb92aa4[2] = {
#include "ev00bf/amy_action_5/dat_0cb92aa4.inc"
};
static NJS_MKEY_F dat_0cb92ac4[2] = {
#include "ev00bf/amy_action_5/dat_0cb92ac4.inc"
};
static NJS_MKEY_A dat_0cb92ae4[2] = {
#include "ev00bf/amy_action_5/dat_0cb92ae4.inc"
};
static NJS_MKEY_F dat_0cb92b04[2] = {
#include "ev00bf/amy_action_5/dat_0cb92b04.inc"
};
static NJS_MKEY_A dat_0cb92b24[2] = {
#include "ev00bf/amy_action_5/dat_0cb92b24.inc"
};
static NJS_MKEY_F dat_0cb92b44[29] = {
#include "ev00bf/amy_action_5/dat_0cb92b44.inc"
};
static NJS_MKEY_A dat_0cb92d14[2] = {
#include "ev00bf/amy_action_5/dat_0cb92d14.inc"
};
static NJS_MKEY_F dat_0cb92d34[2] = {
#include "ev00bf/amy_action_5/dat_0cb92d34.inc"
};
static NJS_MKEY_A dat_0cb92d54[2] = {
#include "ev00bf/amy_action_5/dat_0cb92d54.inc"
};
static NJS_MKEY_F dat_0cb92d74[2] = {
#include "ev00bf/amy_action_5/dat_0cb92d74.inc"
};
static NJS_MKEY_A dat_0cb92d94[2] = {
#include "ev00bf/amy_action_5/dat_0cb92d94.inc"
};
static NJS_MKEY_F dat_0cb92db4[2] = {
#include "ev00bf/amy_action_5/dat_0cb92db4.inc"
};
static NJS_MKEY_A dat_0cb92dd4[2] = {
#include "ev00bf/amy_action_5/dat_0cb92dd4.inc"
};
static NJS_MKEY_F dat_0cb92df4[2] = {
#include "ev00bf/amy_action_5/dat_0cb92df4.inc"
};
static NJS_MKEY_F dat_0cb92e14[2] = {
#include "ev00bf/amy_action_5/dat_0cb92e14.inc"
};
static NJS_MKEY_F dat_0cb92e34[2] = {
#include "ev00bf/amy_action_5/dat_0cb92e34.inc"
};
static NJS_MKEY_A dat_0cb92e54[2] = {
#include "ev00bf/amy_action_5/dat_0cb92e54.inc"
};
static NJS_MKEY_F dat_0cb92e74[2] = {
#include "ev00bf/amy_action_5/dat_0cb92e74.inc"
};
static NJS_MKEY_A dat_0cb92e94[2] = {
#include "ev00bf/amy_action_5/dat_0cb92e94.inc"
};
static NJS_MKEY_F dat_0cb92eb4[2] = {
#include "ev00bf/amy_action_5/dat_0cb92eb4.inc"
};
static NJS_MDATA2 dat_0cb92ed4[57] = {
#include "ev00bf/amy_action_5/dat_0cb92ed4.inc"
};
static NJS_MOTION ev00bf_bird_motion_1 = {
#include "ev00bf/amy_action_5/ev00bf_bird_motion_1.inc"
};
NJS_ACTION ev00bf_bird_action_1 = {
#include "ev00bf/amy_action_5/ev00bf_bird_action_1.inc"
};
static NJS_MKEY_F dat_0cb93278[21] = {
#include "ev00bf/amy_action_5/dat_0cb93278.inc"
};
static NJS_MKEY_A dat_0cb933c8[21] = {
#include "ev00bf/amy_action_5/dat_0cb933c8.inc"
};
static NJS_MKEY_A dat_0cb93518[21] = {
#include "ev00bf/amy_action_5/dat_0cb93518.inc"
};
static NJS_MKEY_F dat_0cb93668[2] = {
#include "ev00bf/amy_action_5/dat_0cb93668.inc"
};
static NJS_MKEY_F dat_0cb93688[2] = {
#include "ev00bf/amy_action_5/dat_0cb93688.inc"
};
static NJS_MKEY_A dat_0cb936a8[2] = {
#include "ev00bf/amy_action_5/dat_0cb936a8.inc"
};
static NJS_MKEY_A dat_0cb936c8[21] = {
#include "ev00bf/amy_action_5/dat_0cb936c8.inc"
};
static NJS_MKEY_F dat_0cb93818[2] = {
#include "ev00bf/amy_action_5/dat_0cb93818.inc"
};
static NJS_MKEY_F dat_0cb93838[2] = {
#include "ev00bf/amy_action_5/dat_0cb93838.inc"
};
static NJS_MKEY_A dat_0cb93858[2] = {
#include "ev00bf/amy_action_5/dat_0cb93858.inc"
};
static NJS_MKEY_A dat_0cb93878[21] = {
#include "ev00bf/amy_action_5/dat_0cb93878.inc"
};
static NJS_MKEY_F dat_0cb939c8[2] = {
#include "ev00bf/amy_action_5/dat_0cb939c8.inc"
};
static NJS_MKEY_F dat_0cb939e8[2] = {
#include "ev00bf/amy_action_5/dat_0cb939e8.inc"
};
static NJS_MKEY_A dat_0cb93a08[2] = {
#include "ev00bf/amy_action_5/dat_0cb93a08.inc"
};
static NJS_MKEY_A dat_0cb93a28[2] = {
#include "ev00bf/amy_action_5/dat_0cb93a28.inc"
};
static NJS_MKEY_F dat_0cb93a48[2] = {
#include "ev00bf/amy_action_5/dat_0cb93a48.inc"
};
static NJS_MKEY_F dat_0cb93a68[2] = {
#include "ev00bf/amy_action_5/dat_0cb93a68.inc"
};
static NJS_MKEY_A dat_0cb93a88[2] = {
#include "ev00bf/amy_action_5/dat_0cb93a88.inc"
};
static NJS_MKEY_F dat_0cb93aa8[2] = {
#include "ev00bf/amy_action_5/dat_0cb93aa8.inc"
};
static NJS_MKEY_A dat_0cb93ac8[2] = {
#include "ev00bf/amy_action_5/dat_0cb93ac8.inc"
};
static NJS_MKEY_F dat_0cb93ae8[2] = {
#include "ev00bf/amy_action_5/dat_0cb93ae8.inc"
};
static NJS_MKEY_A dat_0cb93b08[2] = {
#include "ev00bf/amy_action_5/dat_0cb93b08.inc"
};
static NJS_MKEY_F dat_0cb93b28[2] = {
#include "ev00bf/amy_action_5/dat_0cb93b28.inc"
};
static NJS_MKEY_A dat_0cb93b48[2] = {
#include "ev00bf/amy_action_5/dat_0cb93b48.inc"
};
static NJS_MKEY_A dat_0cb93b68[21] = {
#include "ev00bf/amy_action_5/dat_0cb93b68.inc"
};
static NJS_MKEY_F dat_0cb93cb8[2] = {
#include "ev00bf/amy_action_5/dat_0cb93cb8.inc"
};
static NJS_MKEY_F dat_0cb93cd8[2] = {
#include "ev00bf/amy_action_5/dat_0cb93cd8.inc"
};
static NJS_MKEY_A dat_0cb93cf8[2] = {
#include "ev00bf/amy_action_5/dat_0cb93cf8.inc"
};
static NJS_MKEY_F dat_0cb93d18[2] = {
#include "ev00bf/amy_action_5/dat_0cb93d18.inc"
};
static NJS_MKEY_A dat_0cb93d38[2] = {
#include "ev00bf/amy_action_5/dat_0cb93d38.inc"
};
static NJS_MKEY_F dat_0cb93d58[2] = {
#include "ev00bf/amy_action_5/dat_0cb93d58.inc"
};
static NJS_MKEY_A dat_0cb93d78[2] = {
#include "ev00bf/amy_action_5/dat_0cb93d78.inc"
};
static NJS_MKEY_F dat_0cb93d98[2] = {
#include "ev00bf/amy_action_5/dat_0cb93d98.inc"
};
static NJS_MKEY_A dat_0cb93db8[2] = {
#include "ev00bf/amy_action_5/dat_0cb93db8.inc"
};
static NJS_MKEY_F dat_0cb93dd8[2] = {
#include "ev00bf/amy_action_5/dat_0cb93dd8.inc"
};
static NJS_MKEY_A dat_0cb93df8[2] = {
#include "ev00bf/amy_action_5/dat_0cb93df8.inc"
};
static NJS_MKEY_A dat_0cb93e18[2] = {
#include "ev00bf/amy_action_5/dat_0cb93e18.inc"
};
static NJS_MKEY_F dat_0cb93e38[2] = {
#include "ev00bf/amy_action_5/dat_0cb93e38.inc"
};
static NJS_MKEY_F dat_0cb93e58[21] = {
#include "ev00bf/amy_action_5/dat_0cb93e58.inc"
};
static NJS_MKEY_A dat_0cb93fa8[2] = {
#include "ev00bf/amy_action_5/dat_0cb93fa8.inc"
};
static NJS_MKEY_F dat_0cb93fc8[21] = {
#include "ev00bf/amy_action_5/dat_0cb93fc8.inc"
};
static NJS_MKEY_A dat_0cb94118[2] = {
#include "ev00bf/amy_action_5/dat_0cb94118.inc"
};
static NJS_MKEY_F dat_0cb94138[2] = {
#include "ev00bf/amy_action_5/dat_0cb94138.inc"
};
static NJS_MKEY_A dat_0cb94158[2] = {
#include "ev00bf/amy_action_5/dat_0cb94158.inc"
};
static NJS_MKEY_F dat_0cb94178[21] = {
#include "ev00bf/amy_action_5/dat_0cb94178.inc"
};
static NJS_MKEY_A dat_0cb942c8[2] = {
#include "ev00bf/amy_action_5/dat_0cb942c8.inc"
};
static NJS_MKEY_F dat_0cb942e8[2] = {
#include "ev00bf/amy_action_5/dat_0cb942e8.inc"
};
static NJS_MKEY_A dat_0cb94308[2] = {
#include "ev00bf/amy_action_5/dat_0cb94308.inc"
};
static NJS_MKEY_F dat_0cb94328[2] = {
#include "ev00bf/amy_action_5/dat_0cb94328.inc"
};
static NJS_MKEY_A dat_0cb94348[2] = {
#include "ev00bf/amy_action_5/dat_0cb94348.inc"
};
static NJS_MKEY_F dat_0cb94368[2] = {
#include "ev00bf/amy_action_5/dat_0cb94368.inc"
};
static NJS_MKEY_A dat_0cb94388[2] = {
#include "ev00bf/amy_action_5/dat_0cb94388.inc"
};
static NJS_MKEY_F dat_0cb943a8[2] = {
#include "ev00bf/amy_action_5/dat_0cb943a8.inc"
};
static NJS_MKEY_F dat_0cb943c8[2] = {
#include "ev00bf/amy_action_5/dat_0cb943c8.inc"
};
static NJS_MKEY_F dat_0cb943e8[2] = {
#include "ev00bf/amy_action_5/dat_0cb943e8.inc"
};
static NJS_MKEY_A dat_0cb94408[2] = {
#include "ev00bf/amy_action_5/dat_0cb94408.inc"
};
static NJS_MKEY_F dat_0cb94428[2] = {
#include "ev00bf/amy_action_5/dat_0cb94428.inc"
};
static NJS_MKEY_A dat_0cb94448[2] = {
#include "ev00bf/amy_action_5/dat_0cb94448.inc"
};
static NJS_MKEY_F dat_0cb94468[2] = {
#include "ev00bf/amy_action_5/dat_0cb94468.inc"
};
static NJS_MDATA2 dat_0cb94488[57] = {
#include "ev00bf/amy_action_5/dat_0cb94488.inc"
};
static NJS_MOTION ev00bf_bird_motion_2 = {
#include "ev00bf/amy_action_5/ev00bf_bird_motion_2.inc"
};
NJS_ACTION ev00bf_bird_action_2 = {
#include "ev00bf/amy_action_5/ev00bf_bird_action_2.inc"
};
static NJS_MKEY_F dat_0cb9482c[2] = {
#include "ev00bf/amy_action_5/dat_0cb9482c.inc"
};
static NJS_MKEY_A dat_0cb9484c[2] = {
#include "ev00bf/amy_action_5/dat_0cb9484c.inc"
};
static NJS_MKEY_A dat_0cb9486c[2] = {
#include "ev00bf/amy_action_5/dat_0cb9486c.inc"
};
static NJS_MKEY_F dat_0cb9488c[2] = {
#include "ev00bf/amy_action_5/dat_0cb9488c.inc"
};
static NJS_MKEY_F dat_0cb948ac[2] = {
#include "ev00bf/amy_action_5/dat_0cb948ac.inc"
};
static NJS_MKEY_A dat_0cb948cc[2] = {
#include "ev00bf/amy_action_5/dat_0cb948cc.inc"
};
static NJS_MKEY_A dat_0cb948ec[2] = {
#include "ev00bf/amy_action_5/dat_0cb948ec.inc"
};
static NJS_MKEY_F dat_0cb9490c[2] = {
#include "ev00bf/amy_action_5/dat_0cb9490c.inc"
};
static NJS_MKEY_F dat_0cb9492c[2] = {
#include "ev00bf/amy_action_5/dat_0cb9492c.inc"
};
static NJS_MKEY_A dat_0cb9494c[2] = {
#include "ev00bf/amy_action_5/dat_0cb9494c.inc"
};
static NJS_MKEY_A dat_0cb9496c[2] = {
#include "ev00bf/amy_action_5/dat_0cb9496c.inc"
};
static NJS_MKEY_F dat_0cb9498c[2] = {
#include "ev00bf/amy_action_5/dat_0cb9498c.inc"
};
static NJS_MKEY_F dat_0cb949ac[2] = {
#include "ev00bf/amy_action_5/dat_0cb949ac.inc"
};
static NJS_MKEY_A dat_0cb949cc[2] = {
#include "ev00bf/amy_action_5/dat_0cb949cc.inc"
};
static NJS_MKEY_A dat_0cb949ec[2] = {
#include "ev00bf/amy_action_5/dat_0cb949ec.inc"
};
static NJS_MKEY_F dat_0cb94a0c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94a0c.inc"
};
static NJS_MKEY_F dat_0cb94a2c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94a2c.inc"
};
static NJS_MKEY_A dat_0cb94a4c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94a4c.inc"
};
static NJS_MKEY_F dat_0cb94a6c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94a6c.inc"
};
static NJS_MKEY_A dat_0cb94a8c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94a8c.inc"
};
static NJS_MKEY_F dat_0cb94aac[2] = {
#include "ev00bf/amy_action_5/dat_0cb94aac.inc"
};
static NJS_MKEY_A dat_0cb94acc[2] = {
#include "ev00bf/amy_action_5/dat_0cb94acc.inc"
};
static NJS_MKEY_F dat_0cb94aec[2] = {
#include "ev00bf/amy_action_5/dat_0cb94aec.inc"
};
static NJS_MKEY_A dat_0cb94b0c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94b0c.inc"
};
static NJS_MKEY_A dat_0cb94b2c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94b2c.inc"
};
static NJS_MKEY_F dat_0cb94b4c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94b4c.inc"
};
static NJS_MKEY_F dat_0cb94b6c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94b6c.inc"
};
static NJS_MKEY_A dat_0cb94b8c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94b8c.inc"
};
static NJS_MKEY_F dat_0cb94bac[2] = {
#include "ev00bf/amy_action_5/dat_0cb94bac.inc"
};
static NJS_MKEY_A dat_0cb94bcc[2] = {
#include "ev00bf/amy_action_5/dat_0cb94bcc.inc"
};
static NJS_MKEY_F dat_0cb94bec[2] = {
#include "ev00bf/amy_action_5/dat_0cb94bec.inc"
};
static NJS_MKEY_A dat_0cb94c0c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94c0c.inc"
};
static NJS_MKEY_F dat_0cb94c2c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94c2c.inc"
};
static NJS_MKEY_A dat_0cb94c4c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94c4c.inc"
};
static NJS_MKEY_F dat_0cb94c6c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94c6c.inc"
};
static NJS_MKEY_A dat_0cb94c8c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94c8c.inc"
};
static NJS_MKEY_A dat_0cb94cac[2] = {
#include "ev00bf/amy_action_5/dat_0cb94cac.inc"
};
static NJS_MKEY_F dat_0cb94ccc[2] = {
#include "ev00bf/amy_action_5/dat_0cb94ccc.inc"
};
static NJS_MKEY_F dat_0cb94cec[2] = {
#include "ev00bf/amy_action_5/dat_0cb94cec.inc"
};
static NJS_MKEY_A dat_0cb94d0c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94d0c.inc"
};
static NJS_MKEY_F dat_0cb94d2c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94d2c.inc"
};
static NJS_MKEY_A dat_0cb94d4c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94d4c.inc"
};
static NJS_MKEY_F dat_0cb94d6c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94d6c.inc"
};
static NJS_MKEY_A dat_0cb94d8c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94d8c.inc"
};
static NJS_MKEY_F dat_0cb94dac[2] = {
#include "ev00bf/amy_action_5/dat_0cb94dac.inc"
};
static NJS_MKEY_A dat_0cb94dcc[2] = {
#include "ev00bf/amy_action_5/dat_0cb94dcc.inc"
};
static NJS_MKEY_F dat_0cb94dec[2] = {
#include "ev00bf/amy_action_5/dat_0cb94dec.inc"
};
static NJS_MKEY_A dat_0cb94e0c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94e0c.inc"
};
static NJS_MKEY_F dat_0cb94e2c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94e2c.inc"
};
static NJS_MKEY_A dat_0cb94e4c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94e4c.inc"
};
static NJS_MKEY_F dat_0cb94e6c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94e6c.inc"
};
static NJS_MKEY_A dat_0cb94e8c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94e8c.inc"
};
static NJS_MKEY_F dat_0cb94eac[2] = {
#include "ev00bf/amy_action_5/dat_0cb94eac.inc"
};
static NJS_MKEY_F dat_0cb94ecc[2] = {
#include "ev00bf/amy_action_5/dat_0cb94ecc.inc"
};
static NJS_MKEY_F dat_0cb94eec[2] = {
#include "ev00bf/amy_action_5/dat_0cb94eec.inc"
};
static NJS_MKEY_A dat_0cb94f0c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94f0c.inc"
};
static NJS_MKEY_F dat_0cb94f2c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94f2c.inc"
};
static NJS_MKEY_A dat_0cb94f4c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94f4c.inc"
};
static NJS_MKEY_F dat_0cb94f6c[2] = {
#include "ev00bf/amy_action_5/dat_0cb94f6c.inc"
};
static NJS_MDATA2 dat_0cb94f8c[57] = {
#include "ev00bf/amy_action_5/dat_0cb94f8c.inc"
};
static NJS_MOTION ev00bf_bird_motion_3 = {
#include "ev00bf/amy_action_5/ev00bf_bird_motion_3.inc"
};
NJS_ACTION ev00bf_bird_action_3 = {
#include "ev00bf/amy_action_5/ev00bf_bird_action_3.inc"
};
static NJS_MKEY_F dat_0cb95330[31] = {
#include "ev00bf/amy_action_5/dat_0cb95330.inc"
};
static NJS_MKEY_A dat_0cb95520[31] = {
#include "ev00bf/amy_action_5/dat_0cb95520.inc"
};
static NJS_MKEY_A dat_0cb95710[2] = {
#include "ev00bf/amy_action_5/dat_0cb95710.inc"
};
static NJS_MKEY_F dat_0cb95730[2] = {
#include "ev00bf/amy_action_5/dat_0cb95730.inc"
};
static NJS_MKEY_F dat_0cb95750[2] = {
#include "ev00bf/amy_action_5/dat_0cb95750.inc"
};
static NJS_MKEY_A dat_0cb95770[2] = {
#include "ev00bf/amy_action_5/dat_0cb95770.inc"
};
static NJS_MKEY_A dat_0cb95790[31] = {
#include "ev00bf/amy_action_5/dat_0cb95790.inc"
};
static NJS_MKEY_F dat_0cb95980[2] = {
#include "ev00bf/amy_action_5/dat_0cb95980.inc"
};
static NJS_MKEY_F dat_0cb959a0[2] = {
#include "ev00bf/amy_action_5/dat_0cb959a0.inc"
};
static NJS_MKEY_A dat_0cb959c0[2] = {
#include "ev00bf/amy_action_5/dat_0cb959c0.inc"
};
static NJS_MKEY_A dat_0cb959e0[31] = {
#include "ev00bf/amy_action_5/dat_0cb959e0.inc"
};
static NJS_MKEY_F dat_0cb95bd0[2] = {
#include "ev00bf/amy_action_5/dat_0cb95bd0.inc"
};
static NJS_MKEY_F dat_0cb95bf0[2] = {
#include "ev00bf/amy_action_5/dat_0cb95bf0.inc"
};
static NJS_MKEY_A dat_0cb95c10[2] = {
#include "ev00bf/amy_action_5/dat_0cb95c10.inc"
};
static NJS_MKEY_A dat_0cb95c30[2] = {
#include "ev00bf/amy_action_5/dat_0cb95c30.inc"
};
static NJS_MKEY_F dat_0cb95c50[2] = {
#include "ev00bf/amy_action_5/dat_0cb95c50.inc"
};
static NJS_MKEY_F dat_0cb95c70[2] = {
#include "ev00bf/amy_action_5/dat_0cb95c70.inc"
};
static NJS_MKEY_A dat_0cb95c90[2] = {
#include "ev00bf/amy_action_5/dat_0cb95c90.inc"
};
static NJS_MKEY_F dat_0cb95cb0[2] = {
#include "ev00bf/amy_action_5/dat_0cb95cb0.inc"
};
static NJS_MKEY_A dat_0cb95cd0[2] = {
#include "ev00bf/amy_action_5/dat_0cb95cd0.inc"
};
static NJS_MKEY_F dat_0cb95cf0[2] = {
#include "ev00bf/amy_action_5/dat_0cb95cf0.inc"
};
static NJS_MKEY_A dat_0cb95d10[2] = {
#include "ev00bf/amy_action_5/dat_0cb95d10.inc"
};
static NJS_MKEY_F dat_0cb95d30[2] = {
#include "ev00bf/amy_action_5/dat_0cb95d30.inc"
};
static NJS_MKEY_A dat_0cb95d50[2] = {
#include "ev00bf/amy_action_5/dat_0cb95d50.inc"
};
static NJS_MKEY_A dat_0cb95d70[31] = {
#include "ev00bf/amy_action_5/dat_0cb95d70.inc"
};
static NJS_MKEY_F dat_0cb95f60[2] = {
#include "ev00bf/amy_action_5/dat_0cb95f60.inc"
};
static NJS_MKEY_F dat_0cb95f80[2] = {
#include "ev00bf/amy_action_5/dat_0cb95f80.inc"
};
static NJS_MKEY_A dat_0cb95fa0[2] = {
#include "ev00bf/amy_action_5/dat_0cb95fa0.inc"
};
static NJS_MKEY_F dat_0cb95fc0[2] = {
#include "ev00bf/amy_action_5/dat_0cb95fc0.inc"
};
static NJS_MKEY_A dat_0cb95fe0[2] = {
#include "ev00bf/amy_action_5/dat_0cb95fe0.inc"
};
static NJS_MKEY_F dat_0cb96000[2] = {
#include "ev00bf/amy_action_5/dat_0cb96000.inc"
};
static NJS_MKEY_A dat_0cb96020[2] = {
#include "ev00bf/amy_action_5/dat_0cb96020.inc"
};
static NJS_MKEY_F dat_0cb96040[2] = {
#include "ev00bf/amy_action_5/dat_0cb96040.inc"
};
static NJS_MKEY_A dat_0cb96060[2] = {
#include "ev00bf/amy_action_5/dat_0cb96060.inc"
};
static NJS_MKEY_F dat_0cb96080[2] = {
#include "ev00bf/amy_action_5/dat_0cb96080.inc"
};
static NJS_MKEY_A dat_0cb960a0[2] = {
#include "ev00bf/amy_action_5/dat_0cb960a0.inc"
};
static NJS_MKEY_A dat_0cb960c0[2] = {
#include "ev00bf/amy_action_5/dat_0cb960c0.inc"
};
static NJS_MKEY_F dat_0cb960e0[2] = {
#include "ev00bf/amy_action_5/dat_0cb960e0.inc"
};
static NJS_MKEY_F dat_0cb96100[31] = {
#include "ev00bf/amy_action_5/dat_0cb96100.inc"
};
static NJS_MKEY_A dat_0cb962f0[2] = {
#include "ev00bf/amy_action_5/dat_0cb962f0.inc"
};
static NJS_MKEY_F dat_0cb96310[31] = {
#include "ev00bf/amy_action_5/dat_0cb96310.inc"
};
static NJS_MKEY_A dat_0cb96500[2] = {
#include "ev00bf/amy_action_5/dat_0cb96500.inc"
};
static NJS_MKEY_F dat_0cb96520[2] = {
#include "ev00bf/amy_action_5/dat_0cb96520.inc"
};
static NJS_MKEY_A dat_0cb96540[2] = {
#include "ev00bf/amy_action_5/dat_0cb96540.inc"
};
static NJS_MKEY_F dat_0cb96560[31] = {
#include "ev00bf/amy_action_5/dat_0cb96560.inc"
};
static NJS_MKEY_A dat_0cb96750[2] = {
#include "ev00bf/amy_action_5/dat_0cb96750.inc"
};
static NJS_MKEY_F dat_0cb96770[2] = {
#include "ev00bf/amy_action_5/dat_0cb96770.inc"
};
static NJS_MKEY_A dat_0cb96790[2] = {
#include "ev00bf/amy_action_5/dat_0cb96790.inc"
};
static NJS_MKEY_F dat_0cb967b0[2] = {
#include "ev00bf/amy_action_5/dat_0cb967b0.inc"
};
static NJS_MKEY_A dat_0cb967d0[2] = {
#include "ev00bf/amy_action_5/dat_0cb967d0.inc"
};
static NJS_MKEY_F dat_0cb967f0[2] = {
#include "ev00bf/amy_action_5/dat_0cb967f0.inc"
};
static NJS_MKEY_A dat_0cb96810[2] = {
#include "ev00bf/amy_action_5/dat_0cb96810.inc"
};
static NJS_MKEY_F dat_0cb96830[2] = {
#include "ev00bf/amy_action_5/dat_0cb96830.inc"
};
static NJS_MKEY_F dat_0cb96850[2] = {
#include "ev00bf/amy_action_5/dat_0cb96850.inc"
};
static NJS_MKEY_F dat_0cb96870[2] = {
#include "ev00bf/amy_action_5/dat_0cb96870.inc"
};
static NJS_MKEY_A dat_0cb96890[2] = {
#include "ev00bf/amy_action_5/dat_0cb96890.inc"
};
static NJS_MKEY_F dat_0cb968b0[2] = {
#include "ev00bf/amy_action_5/dat_0cb968b0.inc"
};
static NJS_MKEY_A dat_0cb968d0[2] = {
#include "ev00bf/amy_action_5/dat_0cb968d0.inc"
};
static NJS_MKEY_F dat_0cb968f0[2] = {
#include "ev00bf/amy_action_5/dat_0cb968f0.inc"
};
static NJS_MDATA2 dat_0cb96910[57] = {
#include "ev00bf/amy_action_5/dat_0cb96910.inc"
};
static NJS_MOTION ev00bf_bird_motion_4 = {
#include "ev00bf/amy_action_5/ev00bf_bird_motion_4.inc"
};
NJS_ACTION ev00bf_bird_action_4 = {
#include "ev00bf/amy_action_5/ev00bf_bird_action_4.inc"
};
static NJS_MKEY_F dat_0cb96cb4[2] = {
#include "ev00bf/amy_action_5/dat_0cb96cb4.inc"
};
static NJS_MKEY_A dat_0cb96cd4[2] = {
#include "ev00bf/amy_action_5/dat_0cb96cd4.inc"
};
static NJS_MKEY_F dat_0cb96cf4[2] = {
#include "ev00bf/amy_action_5/dat_0cb96cf4.inc"
};
static NJS_MKEY_F dat_0cb96d14[2] = {
#include "ev00bf/amy_action_5/dat_0cb96d14.inc"
};
static NJS_MKEY_A dat_0cb96d34[2] = {
#include "ev00bf/amy_action_5/dat_0cb96d34.inc"
};
static NJS_MKEY_A dat_0cb96d54[2] = {
#include "ev00bf/amy_action_5/dat_0cb96d54.inc"
};
static NJS_MKEY_F dat_0cb96d74[2] = {
#include "ev00bf/amy_action_5/dat_0cb96d74.inc"
};
static NJS_MKEY_F dat_0cb96d94[2] = {
#include "ev00bf/amy_action_5/dat_0cb96d94.inc"
};
static NJS_MKEY_A dat_0cb96db4[2] = {
#include "ev00bf/amy_action_5/dat_0cb96db4.inc"
};
static NJS_MKEY_A dat_0cb96dd4[2] = {
#include "ev00bf/amy_action_5/dat_0cb96dd4.inc"
};
static NJS_MKEY_F dat_0cb96df4[2] = {
#include "ev00bf/amy_action_5/dat_0cb96df4.inc"
};
static NJS_MKEY_F dat_0cb96e14[2] = {
#include "ev00bf/amy_action_5/dat_0cb96e14.inc"
};
static NJS_MKEY_A dat_0cb96e34[2] = {
#include "ev00bf/amy_action_5/dat_0cb96e34.inc"
};
static NJS_MKEY_A dat_0cb96e54[2] = {
#include "ev00bf/amy_action_5/dat_0cb96e54.inc"
};
static NJS_MKEY_F dat_0cb96e74[2] = {
#include "ev00bf/amy_action_5/dat_0cb96e74.inc"
};
static NJS_MKEY_F dat_0cb96e94[2] = {
#include "ev00bf/amy_action_5/dat_0cb96e94.inc"
};
static NJS_MKEY_A dat_0cb96eb4[2] = {
#include "ev00bf/amy_action_5/dat_0cb96eb4.inc"
};
static NJS_MKEY_F dat_0cb96ed4[2] = {
#include "ev00bf/amy_action_5/dat_0cb96ed4.inc"
};
static NJS_MKEY_A dat_0cb96ef4[2] = {
#include "ev00bf/amy_action_5/dat_0cb96ef4.inc"
};
static NJS_MKEY_F dat_0cb96f14[2] = {
#include "ev00bf/amy_action_5/dat_0cb96f14.inc"
};
static NJS_MKEY_A dat_0cb96f34[2] = {
#include "ev00bf/amy_action_5/dat_0cb96f34.inc"
};
static NJS_MKEY_F dat_0cb96f54[2] = {
#include "ev00bf/amy_action_5/dat_0cb96f54.inc"
};
static NJS_MKEY_A dat_0cb96f74[2] = {
#include "ev00bf/amy_action_5/dat_0cb96f74.inc"
};
static NJS_MKEY_A dat_0cb96f94[2] = {
#include "ev00bf/amy_action_5/dat_0cb96f94.inc"
};
static NJS_MKEY_F dat_0cb96fb4[2] = {
#include "ev00bf/amy_action_5/dat_0cb96fb4.inc"
};
static NJS_MKEY_F dat_0cb96fd4[2] = {
#include "ev00bf/amy_action_5/dat_0cb96fd4.inc"
};
static NJS_MKEY_A dat_0cb96ff4[2] = {
#include "ev00bf/amy_action_5/dat_0cb96ff4.inc"
};
static NJS_MKEY_F dat_0cb97014[2] = {
#include "ev00bf/amy_action_5/dat_0cb97014.inc"
};
static NJS_MKEY_A dat_0cb97034[2] = {
#include "ev00bf/amy_action_5/dat_0cb97034.inc"
};
static NJS_MKEY_F dat_0cb97054[2] = {
#include "ev00bf/amy_action_5/dat_0cb97054.inc"
};
static NJS_MKEY_A dat_0cb97074[2] = {
#include "ev00bf/amy_action_5/dat_0cb97074.inc"
};
static NJS_MKEY_F dat_0cb97094[2] = {
#include "ev00bf/amy_action_5/dat_0cb97094.inc"
};
static NJS_MKEY_A dat_0cb970b4[2] = {
#include "ev00bf/amy_action_5/dat_0cb970b4.inc"
};
static NJS_MKEY_F dat_0cb970d4[2] = {
#include "ev00bf/amy_action_5/dat_0cb970d4.inc"
};
static NJS_MKEY_A dat_0cb970f4[2] = {
#include "ev00bf/amy_action_5/dat_0cb970f4.inc"
};
static NJS_MKEY_A dat_0cb97114[2] = {
#include "ev00bf/amy_action_5/dat_0cb97114.inc"
};
static NJS_MKEY_F dat_0cb97134[2] = {
#include "ev00bf/amy_action_5/dat_0cb97134.inc"
};
static NJS_MKEY_F dat_0cb97154[2] = {
#include "ev00bf/amy_action_5/dat_0cb97154.inc"
};
static NJS_MKEY_A dat_0cb97174[2] = {
#include "ev00bf/amy_action_5/dat_0cb97174.inc"
};
static NJS_MKEY_F dat_0cb97194[2] = {
#include "ev00bf/amy_action_5/dat_0cb97194.inc"
};
static NJS_MKEY_A dat_0cb971b4[2] = {
#include "ev00bf/amy_action_5/dat_0cb971b4.inc"
};
static NJS_MKEY_F dat_0cb971d4[2] = {
#include "ev00bf/amy_action_5/dat_0cb971d4.inc"
};
static NJS_MKEY_A dat_0cb971f4[2] = {
#include "ev00bf/amy_action_5/dat_0cb971f4.inc"
};
static NJS_MKEY_F dat_0cb97214[2] = {
#include "ev00bf/amy_action_5/dat_0cb97214.inc"
};
static NJS_MKEY_A dat_0cb97234[2] = {
#include "ev00bf/amy_action_5/dat_0cb97234.inc"
};
static NJS_MKEY_F dat_0cb97254[2] = {
#include "ev00bf/amy_action_5/dat_0cb97254.inc"
};
static NJS_MKEY_A dat_0cb97274[2] = {
#include "ev00bf/amy_action_5/dat_0cb97274.inc"
};
static NJS_MKEY_F dat_0cb97294[2] = {
#include "ev00bf/amy_action_5/dat_0cb97294.inc"
};
static NJS_MKEY_A dat_0cb972b4[2] = {
#include "ev00bf/amy_action_5/dat_0cb972b4.inc"
};
static NJS_MKEY_F dat_0cb972d4[2] = {
#include "ev00bf/amy_action_5/dat_0cb972d4.inc"
};
static NJS_MKEY_A dat_0cb972f4[2] = {
#include "ev00bf/amy_action_5/dat_0cb972f4.inc"
};
static NJS_MKEY_F dat_0cb97314[2] = {
#include "ev00bf/amy_action_5/dat_0cb97314.inc"
};
static NJS_MKEY_F dat_0cb97334[2] = {
#include "ev00bf/amy_action_5/dat_0cb97334.inc"
};
static NJS_MKEY_F dat_0cb97354[2] = {
#include "ev00bf/amy_action_5/dat_0cb97354.inc"
};
static NJS_MKEY_A dat_0cb97374[2] = {
#include "ev00bf/amy_action_5/dat_0cb97374.inc"
};
static NJS_MKEY_F dat_0cb97394[2] = {
#include "ev00bf/amy_action_5/dat_0cb97394.inc"
};
static NJS_MKEY_A dat_0cb973b4[2] = {
#include "ev00bf/amy_action_5/dat_0cb973b4.inc"
};
static NJS_MKEY_F dat_0cb973d4[2] = {
#include "ev00bf/amy_action_5/dat_0cb973d4.inc"
};
static NJS_MDATA2 dat_0cb973f4[57] = {
#include "ev00bf/amy_action_5/dat_0cb973f4.inc"
};
static NJS_MOTION ev00bf_bird_motion_5 = {
#include "ev00bf/amy_action_5/ev00bf_bird_motion_5.inc"
};
NJS_ACTION ev00bf_bird_action_5 = {
#include "ev00bf/amy_action_5/ev00bf_bird_action_5.inc"
};
static NJS_MKEY_F dat_0cb97798[32] = {
#include "ev00bf/amy_action_5/dat_0cb97798.inc"
};
static NJS_MKEY_A dat_0cb97998[2] = {
#include "ev00bf/amy_action_5/dat_0cb97998.inc"
};
static NJS_MKEY_F dat_0cb979b8[2] = {
#include "ev00bf/amy_action_5/dat_0cb979b8.inc"
};
static NJS_MKEY_F dat_0cb979d8[2] = {
#include "ev00bf/amy_action_5/dat_0cb979d8.inc"
};
static NJS_MKEY_A dat_0cb979f8[2] = {
#include "ev00bf/amy_action_5/dat_0cb979f8.inc"
};
static NJS_MKEY_A dat_0cb97a18[32] = {
#include "ev00bf/amy_action_5/dat_0cb97a18.inc"
};
static NJS_MKEY_F dat_0cb97c18[2] = {
#include "ev00bf/amy_action_5/dat_0cb97c18.inc"
};
static NJS_MKEY_F dat_0cb97c38[2] = {
#include "ev00bf/amy_action_5/dat_0cb97c38.inc"
};
static NJS_MKEY_A dat_0cb97c58[2] = {
#include "ev00bf/amy_action_5/dat_0cb97c58.inc"
};
static NJS_MKEY_A dat_0cb97c78[32] = {
#include "ev00bf/amy_action_5/dat_0cb97c78.inc"
};
static NJS_MKEY_F dat_0cb97e78[2] = {
#include "ev00bf/amy_action_5/dat_0cb97e78.inc"
};
static NJS_MKEY_F dat_0cb97e98[2] = {
#include "ev00bf/amy_action_5/dat_0cb97e98.inc"
};
static NJS_MKEY_A dat_0cb97eb8[2] = {
#include "ev00bf/amy_action_5/dat_0cb97eb8.inc"
};
static NJS_MKEY_A dat_0cb97ed8[2] = {
#include "ev00bf/amy_action_5/dat_0cb97ed8.inc"
};
static NJS_MKEY_F dat_0cb97ef8[2] = {
#include "ev00bf/amy_action_5/dat_0cb97ef8.inc"
};
static NJS_MKEY_F dat_0cb97f18[2] = {
#include "ev00bf/amy_action_5/dat_0cb97f18.inc"
};
static NJS_MKEY_A dat_0cb97f38[2] = {
#include "ev00bf/amy_action_5/dat_0cb97f38.inc"
};
static NJS_MKEY_F dat_0cb97f58[2] = {
#include "ev00bf/amy_action_5/dat_0cb97f58.inc"
};
static NJS_MKEY_A dat_0cb97f78[2] = {
#include "ev00bf/amy_action_5/dat_0cb97f78.inc"
};
static NJS_MKEY_F dat_0cb97f98[2] = {
#include "ev00bf/amy_action_5/dat_0cb97f98.inc"
};
static NJS_MKEY_A dat_0cb97fb8[2] = {
#include "ev00bf/amy_action_5/dat_0cb97fb8.inc"
};
static NJS_MKEY_F dat_0cb97fd8[2] = {
#include "ev00bf/amy_action_5/dat_0cb97fd8.inc"
};
static NJS_MKEY_A dat_0cb97ff8[2] = {
#include "ev00bf/amy_action_5/dat_0cb97ff8.inc"
};
static NJS_MKEY_A dat_0cb98018[32] = {
#include "ev00bf/amy_action_5/dat_0cb98018.inc"
};
static NJS_MKEY_F dat_0cb98218[2] = {
#include "ev00bf/amy_action_5/dat_0cb98218.inc"
};
static NJS_MKEY_F dat_0cb98238[2] = {
#include "ev00bf/amy_action_5/dat_0cb98238.inc"
};
static NJS_MKEY_A dat_0cb98258[2] = {
#include "ev00bf/amy_action_5/dat_0cb98258.inc"
};
static NJS_MKEY_F dat_0cb98278[2] = {
#include "ev00bf/amy_action_5/dat_0cb98278.inc"
};
static NJS_MKEY_A dat_0cb98298[2] = {
#include "ev00bf/amy_action_5/dat_0cb98298.inc"
};
static NJS_MKEY_F dat_0cb982b8[2] = {
#include "ev00bf/amy_action_5/dat_0cb982b8.inc"
};
static NJS_MKEY_A dat_0cb982d8[2] = {
#include "ev00bf/amy_action_5/dat_0cb982d8.inc"
};
static NJS_MKEY_F dat_0cb982f8[2] = {
#include "ev00bf/amy_action_5/dat_0cb982f8.inc"
};
static NJS_MKEY_A dat_0cb98318[2] = {
#include "ev00bf/amy_action_5/dat_0cb98318.inc"
};
static NJS_MKEY_F dat_0cb98338[2] = {
#include "ev00bf/amy_action_5/dat_0cb98338.inc"
};
static NJS_MKEY_A dat_0cb98358[2] = {
#include "ev00bf/amy_action_5/dat_0cb98358.inc"
};
static NJS_MKEY_A dat_0cb98378[2] = {
#include "ev00bf/amy_action_5/dat_0cb98378.inc"
};
static NJS_MKEY_F dat_0cb98398[2] = {
#include "ev00bf/amy_action_5/dat_0cb98398.inc"
};
static NJS_MKEY_F dat_0cb983b8[32] = {
#include "ev00bf/amy_action_5/dat_0cb983b8.inc"
};
static NJS_MKEY_A dat_0cb985b8[2] = {
#include "ev00bf/amy_action_5/dat_0cb985b8.inc"
};
static NJS_MKEY_F dat_0cb985d8[32] = {
#include "ev00bf/amy_action_5/dat_0cb985d8.inc"
};
static NJS_MKEY_A dat_0cb987d8[2] = {
#include "ev00bf/amy_action_5/dat_0cb987d8.inc"
};
static NJS_MKEY_F dat_0cb987f8[2] = {
#include "ev00bf/amy_action_5/dat_0cb987f8.inc"
};
static NJS_MKEY_A dat_0cb98818[2] = {
#include "ev00bf/amy_action_5/dat_0cb98818.inc"
};
static NJS_MKEY_F dat_0cb98838[32] = {
#include "ev00bf/amy_action_5/dat_0cb98838.inc"
};
static NJS_MKEY_A dat_0cb98a38[2] = {
#include "ev00bf/amy_action_5/dat_0cb98a38.inc"
};
static NJS_MKEY_F dat_0cb98a58[2] = {
#include "ev00bf/amy_action_5/dat_0cb98a58.inc"
};
static NJS_MKEY_A dat_0cb98a78[2] = {
#include "ev00bf/amy_action_5/dat_0cb98a78.inc"
};
static NJS_MKEY_F dat_0cb98a98[2] = {
#include "ev00bf/amy_action_5/dat_0cb98a98.inc"
};
static NJS_MKEY_A dat_0cb98ab8[2] = {
#include "ev00bf/amy_action_5/dat_0cb98ab8.inc"
};
static NJS_MKEY_F dat_0cb98ad8[2] = {
#include "ev00bf/amy_action_5/dat_0cb98ad8.inc"
};
static NJS_MKEY_A dat_0cb98af8[2] = {
#include "ev00bf/amy_action_5/dat_0cb98af8.inc"
};
static NJS_MKEY_F dat_0cb98b18[2] = {
#include "ev00bf/amy_action_5/dat_0cb98b18.inc"
};
static NJS_MKEY_F dat_0cb98b38[2] = {
#include "ev00bf/amy_action_5/dat_0cb98b38.inc"
};
static NJS_MKEY_F dat_0cb98b58[2] = {
#include "ev00bf/amy_action_5/dat_0cb98b58.inc"
};
static NJS_MKEY_A dat_0cb98b78[2] = {
#include "ev00bf/amy_action_5/dat_0cb98b78.inc"
};
static NJS_MKEY_F dat_0cb98b98[2] = {
#include "ev00bf/amy_action_5/dat_0cb98b98.inc"
};
static NJS_MKEY_A dat_0cb98bb8[2] = {
#include "ev00bf/amy_action_5/dat_0cb98bb8.inc"
};
static NJS_MKEY_F dat_0cb98bd8[2] = {
#include "ev00bf/amy_action_5/dat_0cb98bd8.inc"
};
static NJS_MDATA2 dat_0cb98bf8[57] = {
#include "ev00bf/amy_action_5/dat_0cb98bf8.inc"
};
static NJS_MOTION ev00bf_bird_motion_6 = {
#include "ev00bf/amy_action_5/ev00bf_bird_motion_6.inc"
};
NJS_ACTION ev00bf_bird_action_6 = {
#include "ev00bf/amy_action_5/ev00bf_bird_action_6.inc"
};
static NJS_TEXNAME dat_0cb98f9c[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev00bf_bird_texlist_2 = {
#include "ev00bf/amy_action_5/ev00bf_bird_texlist_2.inc"
};
/* the material of dat_0cb990dc, the model of one of ev00bf_bird_action_7's
 * nodes. those nodes follow this file's data */
NJS_MATERIAL dat_0cb99034[1] = {
#include "ev00bf/amy_action_5/dat_0cb99034.inc"
};
