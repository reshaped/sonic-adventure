/* a texlist and its texture name, then ev0026_sonic_fingers, which
 * script.c puts in place of Sonic's -z fingers. then Sonic's actions
 * ev0026_sonic_action_0 to _3 and Tails' ev0026_tails_action_0 to _2, each
 * with its motion. the file ends on another texlist and on
 * event_clouds_texlist, each with its texture names */
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/clouds.h"

#include "ev0026/sonic_fingers.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 5
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 5
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 5
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 10
#define TEXT_9_LENGTH 10
#include "ev0026/sonic_fingers/text.inc"

extern NJS_OBJECT sonic_object;
extern NJS_OBJECT tails_object;
extern NJS_OBJECT tails_fly_object;

static NJS_TEXNAME dat_0cb8b678[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cb8b684 = {
#include "ev0026/sonic_fingers/dat_0cb8b684.inc"
};
static NJS_MATERIAL dat_0cb8b68c[3] = {
#include "ev0026/sonic_fingers/dat_0cb8b68c.inc"
};
static Sint16 dat_0cb8b6c8[20] = {
#include "ev0026/sonic_fingers/dat_0cb8b6c8.inc"
};
static Sint16 dat_0cb8b6f0[38] = {
#include "ev0026/sonic_fingers/dat_0cb8b6f0.inc"
};
static Sint16 dat_0cb8b73c[40] = {
#include "ev0026/sonic_fingers/dat_0cb8b73c.inc"
};
static NJS_COLOR dat_0cb8b78c[32] = {
#include "ev0026/sonic_fingers/dat_0cb8b78c.inc"
};
static NJS_MESHSET dat_0cb8b80c[3] = {
#include "ev0026/sonic_fingers/dat_0cb8b80c.inc"
};
static NJS_POINT3 dat_0cb8b854[28] = {
#include "ev0026/sonic_fingers/dat_0cb8b854.inc"
};
static NJS_VECTOR dat_0cb8b9a4[28] = {
#include "ev0026/sonic_fingers/dat_0cb8b9a4.inc"
};
static NJS_MODEL dat_0cb8baf4 = {
#include "ev0026/sonic_fingers/dat_0cb8baf4.inc"
};
NJS_OBJECT ev0026_sonic_fingers = {
#include "ev0026/sonic_fingers/ev0026_sonic_fingers.inc"
};
static NJS_MKEY_F dat_0cb8bb50[15] = {
#include "ev0026/sonic_fingers/dat_0cb8bb50.inc"
};
static NJS_MKEY_A dat_0cb8bc40[2] = {
#include "ev0026/sonic_fingers/dat_0cb8bc40.inc"
};
static NJS_MKEY_A dat_0cb8bc60[15] = {
#include "ev0026/sonic_fingers/dat_0cb8bc60.inc"
};
static NJS_MKEY_A dat_0cb8bd50[2] = {
#include "ev0026/sonic_fingers/dat_0cb8bd50.inc"
};
static NJS_MKEY_A dat_0cb8bd70[2] = {
#include "ev0026/sonic_fingers/dat_0cb8bd70.inc"
};
static NJS_MKEY_A dat_0cb8bd90[2] = {
#include "ev0026/sonic_fingers/dat_0cb8bd90.inc"
};
static NJS_MKEY_A dat_0cb8bdb0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8bdb0.inc"
};
static NJS_MKEY_A dat_0cb8bdd0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8bdd0.inc"
};
static NJS_MKEY_A dat_0cb8bdf0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8bdf0.inc"
};
static NJS_MKEY_A dat_0cb8be10[2] = {
#include "ev0026/sonic_fingers/dat_0cb8be10.inc"
};
static NJS_MKEY_A dat_0cb8be30[2] = {
#include "ev0026/sonic_fingers/dat_0cb8be30.inc"
};
static NJS_MKEY_A dat_0cb8be50[2] = {
#include "ev0026/sonic_fingers/dat_0cb8be50.inc"
};
static NJS_MKEY_A dat_0cb8be70[2] = {
#include "ev0026/sonic_fingers/dat_0cb8be70.inc"
};
static NJS_MKEY_A dat_0cb8be90[14] = {
#include "ev0026/sonic_fingers/dat_0cb8be90.inc"
};
static NJS_MKEY_A dat_0cb8bf70[2] = {
#include "ev0026/sonic_fingers/dat_0cb8bf70.inc"
};
static NJS_MKEY_A dat_0cb8bf90[2] = {
#include "ev0026/sonic_fingers/dat_0cb8bf90.inc"
};
static NJS_MKEY_A dat_0cb8bfb0[15] = {
#include "ev0026/sonic_fingers/dat_0cb8bfb0.inc"
};
static NJS_MKEY_A dat_0cb8c0a0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c0a0.inc"
};
static NJS_MKEY_A dat_0cb8c0c0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c0c0.inc"
};
static NJS_MKEY_A dat_0cb8c0e0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c0e0.inc"
};
static NJS_MKEY_A dat_0cb8c100[15] = {
#include "ev0026/sonic_fingers/dat_0cb8c100.inc"
};
static NJS_MKEY_A dat_0cb8c1f0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c1f0.inc"
};
static NJS_MKEY_A dat_0cb8c210[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c210.inc"
};
static NJS_MKEY_A dat_0cb8c230[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c230.inc"
};
static NJS_MKEY_A dat_0cb8c250[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c250.inc"
};
static NJS_MKEY_A dat_0cb8c270[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c270.inc"
};
static NJS_MKEY_A dat_0cb8c290[15] = {
#include "ev0026/sonic_fingers/dat_0cb8c290.inc"
};
static NJS_MKEY_A dat_0cb8c380[15] = {
#include "ev0026/sonic_fingers/dat_0cb8c380.inc"
};
static NJS_MKEY_A dat_0cb8c470[15] = {
#include "ev0026/sonic_fingers/dat_0cb8c470.inc"
};
static NJS_MKEY_A dat_0cb8c560[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c560.inc"
};
static NJS_MKEY_A dat_0cb8c580[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c580.inc"
};
static NJS_MKEY_A dat_0cb8c5a0[15] = {
#include "ev0026/sonic_fingers/dat_0cb8c5a0.inc"
};
static NJS_MKEY_A dat_0cb8c690[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c690.inc"
};
static NJS_MKEY_A dat_0cb8c6b0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c6b0.inc"
};
static NJS_MKEY_A dat_0cb8c6d0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c6d0.inc"
};
static NJS_MKEY_A dat_0cb8c6f0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c6f0.inc"
};
static NJS_MKEY_A dat_0cb8c710[2] = {
#include "ev0026/sonic_fingers/dat_0cb8c710.inc"
};
static NJS_MKEY_A dat_0cb8c730[15] = {
#include "ev0026/sonic_fingers/dat_0cb8c730.inc"
};
static NJS_MKEY_A dat_0cb8c820[15] = {
#include "ev0026/sonic_fingers/dat_0cb8c820.inc"
};
static NJS_MKEY_A dat_0cb8c910[15] = {
#include "ev0026/sonic_fingers/dat_0cb8c910.inc"
};
static NJS_MKEY_A dat_0cb8ca00[2] = {
#include "ev0026/sonic_fingers/dat_0cb8ca00.inc"
};
static NJS_MKEY_A dat_0cb8ca20[2] = {
#include "ev0026/sonic_fingers/dat_0cb8ca20.inc"
};
static NJS_MKEY_A dat_0cb8ca40[2] = {
#include "ev0026/sonic_fingers/dat_0cb8ca40.inc"
};
static NJS_MKEY_A dat_0cb8ca60[2] = {
#include "ev0026/sonic_fingers/dat_0cb8ca60.inc"
};
static NJS_MKEY_A dat_0cb8ca80[15] = {
#include "ev0026/sonic_fingers/dat_0cb8ca80.inc"
};
static NJS_MKEY_A dat_0cb8cb70[2] = {
#include "ev0026/sonic_fingers/dat_0cb8cb70.inc"
};
static NJS_MKEY_A dat_0cb8cb90[2] = {
#include "ev0026/sonic_fingers/dat_0cb8cb90.inc"
};
static NJS_MKEY_A dat_0cb8cbb0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8cbb0.inc"
};
static NJS_MKEY_A dat_0cb8cbd0[15] = {
#include "ev0026/sonic_fingers/dat_0cb8cbd0.inc"
};
static NJS_MKEY_A dat_0cb8ccc0[15] = {
#include "ev0026/sonic_fingers/dat_0cb8ccc0.inc"
};
static NJS_MKEY_A dat_0cb8cdb0[15] = {
#include "ev0026/sonic_fingers/dat_0cb8cdb0.inc"
};
static NJS_MKEY_A dat_0cb8cea0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8cea0.inc"
};
static NJS_MKEY_A dat_0cb8cec0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8cec0.inc"
};
static NJS_MKEY_A dat_0cb8cee0[15] = {
#include "ev0026/sonic_fingers/dat_0cb8cee0.inc"
};
static NJS_MKEY_A dat_0cb8cfd0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8cfd0.inc"
};
static NJS_MKEY_A dat_0cb8cff0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8cff0.inc"
};
static NJS_MKEY_A dat_0cb8d010[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d010.inc"
};
static NJS_MKEY_A dat_0cb8d030[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d030.inc"
};
static NJS_MKEY_A dat_0cb8d050[15] = {
#include "ev0026/sonic_fingers/dat_0cb8d050.inc"
};
static NJS_MKEY_A dat_0cb8d140[15] = {
#include "ev0026/sonic_fingers/dat_0cb8d140.inc"
};
static NJS_MKEY_A dat_0cb8d230[15] = {
#include "ev0026/sonic_fingers/dat_0cb8d230.inc"
};
static NJS_MKEY_A dat_0cb8d320[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d320.inc"
};
static NJS_MKEY_A dat_0cb8d340[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d340.inc"
};
static NJS_MKEY_A dat_0cb8d360[15] = {
#include "ev0026/sonic_fingers/dat_0cb8d360.inc"
};
static NJS_MKEY_A dat_0cb8d450[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d450.inc"
};
static NJS_MKEY_A dat_0cb8d470[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d470.inc"
};
static NJS_MKEY_A dat_0cb8d490[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d490.inc"
};
static NJS_MKEY_A dat_0cb8d4b0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d4b0.inc"
};
static NJS_MKEY_A dat_0cb8d4d0[15] = {
#include "ev0026/sonic_fingers/dat_0cb8d4d0.inc"
};
static NJS_MKEY_A dat_0cb8d5c0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d5c0.inc"
};
static NJS_MKEY_A dat_0cb8d5e0[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d5e0.inc"
};
static NJS_MKEY_A dat_0cb8d600[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d600.inc"
};
static NJS_MKEY_A dat_0cb8d620[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d620.inc"
};
static NJS_MKEY_A dat_0cb8d640[2] = {
#include "ev0026/sonic_fingers/dat_0cb8d640.inc"
};
static NJS_MDATA2 dat_0cb8d660[73] = {
#include "ev0026/sonic_fingers/dat_0cb8d660.inc"
};
static NJS_MOTION ev0026_sonic_motion_0 = {
#include "ev0026/sonic_fingers/ev0026_sonic_motion_0.inc"
};
NJS_ACTION ev0026_sonic_action_0 = {
#include "ev0026/sonic_fingers/ev0026_sonic_action_0.inc"
};
static NJS_MKEY_F dat_0cb8db04[15] = {
#include "ev0026/sonic_fingers/dat_0cb8db04.inc"
};
static NJS_MKEY_A dat_0cb8dbf4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8dbf4.inc"
};
static NJS_MKEY_A dat_0cb8dc14[15] = {
#include "ev0026/sonic_fingers/dat_0cb8dc14.inc"
};
static NJS_MKEY_A dat_0cb8dd04[2] = {
#include "ev0026/sonic_fingers/dat_0cb8dd04.inc"
};
static NJS_MKEY_A dat_0cb8dd24[2] = {
#include "ev0026/sonic_fingers/dat_0cb8dd24.inc"
};
static NJS_MKEY_A dat_0cb8dd44[2] = {
#include "ev0026/sonic_fingers/dat_0cb8dd44.inc"
};
static NJS_MKEY_A dat_0cb8dd64[2] = {
#include "ev0026/sonic_fingers/dat_0cb8dd64.inc"
};
static NJS_MKEY_A dat_0cb8dd84[2] = {
#include "ev0026/sonic_fingers/dat_0cb8dd84.inc"
};
static NJS_MKEY_A dat_0cb8dda4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8dda4.inc"
};
static NJS_MKEY_A dat_0cb8ddc4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8ddc4.inc"
};
static NJS_MKEY_A dat_0cb8dde4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8dde4.inc"
};
static NJS_MKEY_A dat_0cb8de04[2] = {
#include "ev0026/sonic_fingers/dat_0cb8de04.inc"
};
static NJS_MKEY_A dat_0cb8de24[2] = {
#include "ev0026/sonic_fingers/dat_0cb8de24.inc"
};
static NJS_MKEY_A dat_0cb8de44[2] = {
#include "ev0026/sonic_fingers/dat_0cb8de44.inc"
};
static NJS_MKEY_A dat_0cb8de64[2] = {
#include "ev0026/sonic_fingers/dat_0cb8de64.inc"
};
static NJS_MKEY_A dat_0cb8de84[2] = {
#include "ev0026/sonic_fingers/dat_0cb8de84.inc"
};
static NJS_MKEY_A dat_0cb8dea4[15] = {
#include "ev0026/sonic_fingers/dat_0cb8dea4.inc"
};
static NJS_MKEY_A dat_0cb8df94[2] = {
#include "ev0026/sonic_fingers/dat_0cb8df94.inc"
};
static NJS_MKEY_A dat_0cb8dfb4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8dfb4.inc"
};
static NJS_MKEY_A dat_0cb8dfd4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8dfd4.inc"
};
static NJS_MKEY_A dat_0cb8dff4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8dff4.inc"
};
static NJS_MKEY_A dat_0cb8e014[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e014.inc"
};
static NJS_MKEY_A dat_0cb8e034[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e034.inc"
};
static NJS_MKEY_A dat_0cb8e054[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e054.inc"
};
static NJS_MKEY_A dat_0cb8e074[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e074.inc"
};
static NJS_MKEY_A dat_0cb8e094[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e094.inc"
};
static NJS_MKEY_A dat_0cb8e0b4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e0b4.inc"
};
static NJS_MKEY_A dat_0cb8e0d4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e0d4.inc"
};
static NJS_MKEY_A dat_0cb8e0f4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e0f4.inc"
};
static NJS_MKEY_A dat_0cb8e114[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e114.inc"
};
static NJS_MKEY_A dat_0cb8e134[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e134.inc"
};
static NJS_MKEY_A dat_0cb8e154[15] = {
#include "ev0026/sonic_fingers/dat_0cb8e154.inc"
};
static NJS_MKEY_A dat_0cb8e244[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e244.inc"
};
static NJS_MKEY_A dat_0cb8e264[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e264.inc"
};
static NJS_MKEY_A dat_0cb8e284[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e284.inc"
};
static NJS_MKEY_A dat_0cb8e2a4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e2a4.inc"
};
static NJS_MKEY_A dat_0cb8e2c4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e2c4.inc"
};
static NJS_MKEY_A dat_0cb8e2e4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e2e4.inc"
};
static NJS_MKEY_A dat_0cb8e304[15] = {
#include "ev0026/sonic_fingers/dat_0cb8e304.inc"
};
static NJS_MKEY_A dat_0cb8e3f4[15] = {
#include "ev0026/sonic_fingers/dat_0cb8e3f4.inc"
};
static NJS_MKEY_A dat_0cb8e4e4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e4e4.inc"
};
static NJS_MKEY_A dat_0cb8e504[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e504.inc"
};
static NJS_MKEY_A dat_0cb8e524[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e524.inc"
};
static NJS_MKEY_A dat_0cb8e544[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e544.inc"
};
static NJS_MKEY_A dat_0cb8e564[15] = {
#include "ev0026/sonic_fingers/dat_0cb8e564.inc"
};
static NJS_MKEY_A dat_0cb8e654[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e654.inc"
};
static NJS_MKEY_A dat_0cb8e674[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e674.inc"
};
static NJS_MKEY_A dat_0cb8e694[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e694.inc"
};
static NJS_MKEY_A dat_0cb8e6b4[15] = {
#include "ev0026/sonic_fingers/dat_0cb8e6b4.inc"
};
static NJS_MKEY_A dat_0cb8e7a4[15] = {
#include "ev0026/sonic_fingers/dat_0cb8e7a4.inc"
};
static NJS_MKEY_A dat_0cb8e894[15] = {
#include "ev0026/sonic_fingers/dat_0cb8e894.inc"
};
static NJS_MKEY_A dat_0cb8e984[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e984.inc"
};
static NJS_MKEY_A dat_0cb8e9a4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8e9a4.inc"
};
static NJS_MKEY_A dat_0cb8e9c4[15] = {
#include "ev0026/sonic_fingers/dat_0cb8e9c4.inc"
};
static NJS_MKEY_A dat_0cb8eab4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8eab4.inc"
};
static NJS_MKEY_A dat_0cb8ead4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8ead4.inc"
};
static NJS_MKEY_A dat_0cb8eaf4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8eaf4.inc"
};
static NJS_MKEY_A dat_0cb8eb14[2] = {
#include "ev0026/sonic_fingers/dat_0cb8eb14.inc"
};
static NJS_MKEY_A dat_0cb8eb34[15] = {
#include "ev0026/sonic_fingers/dat_0cb8eb34.inc"
};
static NJS_MKEY_A dat_0cb8ec24[15] = {
#include "ev0026/sonic_fingers/dat_0cb8ec24.inc"
};
static NJS_MKEY_A dat_0cb8ed14[15] = {
#include "ev0026/sonic_fingers/dat_0cb8ed14.inc"
};
static NJS_MKEY_A dat_0cb8ee04[2] = {
#include "ev0026/sonic_fingers/dat_0cb8ee04.inc"
};
static NJS_MKEY_A dat_0cb8ee24[2] = {
#include "ev0026/sonic_fingers/dat_0cb8ee24.inc"
};
static NJS_MKEY_A dat_0cb8ee44[15] = {
#include "ev0026/sonic_fingers/dat_0cb8ee44.inc"
};
static NJS_MKEY_A dat_0cb8ef34[2] = {
#include "ev0026/sonic_fingers/dat_0cb8ef34.inc"
};
static NJS_MKEY_A dat_0cb8ef54[2] = {
#include "ev0026/sonic_fingers/dat_0cb8ef54.inc"
};
static NJS_MKEY_A dat_0cb8ef74[2] = {
#include "ev0026/sonic_fingers/dat_0cb8ef74.inc"
};
static NJS_MKEY_A dat_0cb8ef94[2] = {
#include "ev0026/sonic_fingers/dat_0cb8ef94.inc"
};
static NJS_MKEY_A dat_0cb8efb4[15] = {
#include "ev0026/sonic_fingers/dat_0cb8efb4.inc"
};
static NJS_MKEY_A dat_0cb8f0a4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8f0a4.inc"
};
static NJS_MKEY_A dat_0cb8f0c4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8f0c4.inc"
};
static NJS_MKEY_A dat_0cb8f0e4[2] = {
#include "ev0026/sonic_fingers/dat_0cb8f0e4.inc"
};
static NJS_MKEY_A dat_0cb8f104[2] = {
#include "ev0026/sonic_fingers/dat_0cb8f104.inc"
};
static NJS_MKEY_A dat_0cb8f124[2] = {
#include "ev0026/sonic_fingers/dat_0cb8f124.inc"
};
static NJS_MDATA2 dat_0cb8f144[73] = {
#include "ev0026/sonic_fingers/dat_0cb8f144.inc"
};
static NJS_MOTION ev0026_sonic_motion_1 = {
#include "ev0026/sonic_fingers/ev0026_sonic_motion_1.inc"
};
NJS_ACTION ev0026_sonic_action_1 = {
#include "ev0026/sonic_fingers/ev0026_sonic_action_1.inc"
};
static NJS_MKEY_F dat_0cb8f5e8[51] = {
#include "ev0026/sonic_fingers/dat_0cb8f5e8.inc"
};
static NJS_MKEY_A dat_0cb8f918[51] = {
#include "ev0026/sonic_fingers/dat_0cb8f918.inc"
};
static NJS_MKEY_A dat_0cb8fc48[2] = {
#include "ev0026/sonic_fingers/dat_0cb8fc48.inc"
};
static NJS_MKEY_A dat_0cb8fc68[2] = {
#include "ev0026/sonic_fingers/dat_0cb8fc68.inc"
};
static NJS_MKEY_A dat_0cb8fc88[2] = {
#include "ev0026/sonic_fingers/dat_0cb8fc88.inc"
};
static NJS_MKEY_A dat_0cb8fca8[2] = {
#include "ev0026/sonic_fingers/dat_0cb8fca8.inc"
};
static NJS_MKEY_A dat_0cb8fcc8[2] = {
#include "ev0026/sonic_fingers/dat_0cb8fcc8.inc"
};
static NJS_MKEY_A dat_0cb8fce8[2] = {
#include "ev0026/sonic_fingers/dat_0cb8fce8.inc"
};
static NJS_MKEY_A dat_0cb8fd08[2] = {
#include "ev0026/sonic_fingers/dat_0cb8fd08.inc"
};
static NJS_MKEY_A dat_0cb8fd28[2] = {
#include "ev0026/sonic_fingers/dat_0cb8fd28.inc"
};
static NJS_MKEY_A dat_0cb8fd48[2] = {
#include "ev0026/sonic_fingers/dat_0cb8fd48.inc"
};
static NJS_MKEY_A dat_0cb8fd68[2] = {
#include "ev0026/sonic_fingers/dat_0cb8fd68.inc"
};
static NJS_MKEY_A dat_0cb8fd88[2] = {
#include "ev0026/sonic_fingers/dat_0cb8fd88.inc"
};
static NJS_MKEY_A dat_0cb8fda8[51] = {
#include "ev0026/sonic_fingers/dat_0cb8fda8.inc"
};
static NJS_MKEY_A dat_0cb900d8[2] = {
#include "ev0026/sonic_fingers/dat_0cb900d8.inc"
};
static NJS_MKEY_A dat_0cb900f8[2] = {
#include "ev0026/sonic_fingers/dat_0cb900f8.inc"
};
static NJS_MKEY_A dat_0cb90118[51] = {
#include "ev0026/sonic_fingers/dat_0cb90118.inc"
};
static NJS_MKEY_A dat_0cb90448[2] = {
#include "ev0026/sonic_fingers/dat_0cb90448.inc"
};
static NJS_MKEY_A dat_0cb90468[2] = {
#include "ev0026/sonic_fingers/dat_0cb90468.inc"
};
static NJS_MKEY_A dat_0cb90488[2] = {
#include "ev0026/sonic_fingers/dat_0cb90488.inc"
};
static NJS_MKEY_A dat_0cb904a8[44] = {
#include "ev0026/sonic_fingers/dat_0cb904a8.inc"
};
static NJS_MKEY_A dat_0cb90768[2] = {
#include "ev0026/sonic_fingers/dat_0cb90768.inc"
};
static NJS_MKEY_A dat_0cb90788[2] = {
#include "ev0026/sonic_fingers/dat_0cb90788.inc"
};
static NJS_MKEY_A dat_0cb907a8[2] = {
#include "ev0026/sonic_fingers/dat_0cb907a8.inc"
};
static NJS_MKEY_A dat_0cb907c8[2] = {
#include "ev0026/sonic_fingers/dat_0cb907c8.inc"
};
static NJS_MKEY_A dat_0cb907e8[2] = {
#include "ev0026/sonic_fingers/dat_0cb907e8.inc"
};
static NJS_MKEY_A dat_0cb90808[51] = {
#include "ev0026/sonic_fingers/dat_0cb90808.inc"
};
static NJS_MKEY_A dat_0cb90b38[51] = {
#include "ev0026/sonic_fingers/dat_0cb90b38.inc"
};
static NJS_MKEY_A dat_0cb90e68[51] = {
#include "ev0026/sonic_fingers/dat_0cb90e68.inc"
};
static NJS_MKEY_A dat_0cb91198[2] = {
#include "ev0026/sonic_fingers/dat_0cb91198.inc"
};
static NJS_MKEY_A dat_0cb911b8[2] = {
#include "ev0026/sonic_fingers/dat_0cb911b8.inc"
};
static NJS_MKEY_A dat_0cb911d8[51] = {
#include "ev0026/sonic_fingers/dat_0cb911d8.inc"
};
static NJS_MKEY_A dat_0cb91508[2] = {
#include "ev0026/sonic_fingers/dat_0cb91508.inc"
};
static NJS_MKEY_A dat_0cb91528[2] = {
#include "ev0026/sonic_fingers/dat_0cb91528.inc"
};
static NJS_MKEY_A dat_0cb91548[2] = {
#include "ev0026/sonic_fingers/dat_0cb91548.inc"
};
static NJS_MKEY_A dat_0cb91568[2] = {
#include "ev0026/sonic_fingers/dat_0cb91568.inc"
};
static NJS_MKEY_A dat_0cb91588[2] = {
#include "ev0026/sonic_fingers/dat_0cb91588.inc"
};
static NJS_MKEY_A dat_0cb915a8[51] = {
#include "ev0026/sonic_fingers/dat_0cb915a8.inc"
};
static NJS_MKEY_A dat_0cb918d8[51] = {
#include "ev0026/sonic_fingers/dat_0cb918d8.inc"
};
static NJS_MKEY_A dat_0cb91c08[50] = {
#include "ev0026/sonic_fingers/dat_0cb91c08.inc"
};
static NJS_MKEY_A dat_0cb91f28[2] = {
#include "ev0026/sonic_fingers/dat_0cb91f28.inc"
};
static NJS_MKEY_A dat_0cb91f48[2] = {
#include "ev0026/sonic_fingers/dat_0cb91f48.inc"
};
static NJS_MKEY_A dat_0cb91f68[51] = {
#include "ev0026/sonic_fingers/dat_0cb91f68.inc"
};
static NJS_MKEY_A dat_0cb92298[2] = {
#include "ev0026/sonic_fingers/dat_0cb92298.inc"
};
static NJS_MKEY_A dat_0cb922b8[2] = {
#include "ev0026/sonic_fingers/dat_0cb922b8.inc"
};
static NJS_MKEY_A dat_0cb922d8[2] = {
#include "ev0026/sonic_fingers/dat_0cb922d8.inc"
};
static NJS_MKEY_A dat_0cb922f8[2] = {
#include "ev0026/sonic_fingers/dat_0cb922f8.inc"
};
static NJS_MKEY_A dat_0cb92318[2] = {
#include "ev0026/sonic_fingers/dat_0cb92318.inc"
};
static NJS_MKEY_A dat_0cb92338[51] = {
#include "ev0026/sonic_fingers/dat_0cb92338.inc"
};
static NJS_MKEY_A dat_0cb92668[32] = {
#include "ev0026/sonic_fingers/dat_0cb92668.inc"
};
static NJS_MKEY_A dat_0cb92868[32] = {
#include "ev0026/sonic_fingers/dat_0cb92868.inc"
};
static NJS_MKEY_A dat_0cb92a68[2] = {
#include "ev0026/sonic_fingers/dat_0cb92a68.inc"
};
static NJS_MKEY_A dat_0cb92a88[2] = {
#include "ev0026/sonic_fingers/dat_0cb92a88.inc"
};
static NJS_MKEY_A dat_0cb92aa8[50] = {
#include "ev0026/sonic_fingers/dat_0cb92aa8.inc"
};
static NJS_MKEY_A dat_0cb92dc8[2] = {
#include "ev0026/sonic_fingers/dat_0cb92dc8.inc"
};
static NJS_MKEY_A dat_0cb92de8[2] = {
#include "ev0026/sonic_fingers/dat_0cb92de8.inc"
};
static NJS_MKEY_A dat_0cb92e08[2] = {
#include "ev0026/sonic_fingers/dat_0cb92e08.inc"
};
static NJS_MKEY_A dat_0cb92e28[38] = {
#include "ev0026/sonic_fingers/dat_0cb92e28.inc"
};
static NJS_MKEY_A dat_0cb93088[51] = {
#include "ev0026/sonic_fingers/dat_0cb93088.inc"
};
static NJS_MKEY_A dat_0cb933b8[5] = {
#include "ev0026/sonic_fingers/dat_0cb933b8.inc"
};
static NJS_MKEY_A dat_0cb93408[5] = {
#include "ev0026/sonic_fingers/dat_0cb93408.inc"
};
static NJS_MKEY_A dat_0cb93458[2] = {
#include "ev0026/sonic_fingers/dat_0cb93458.inc"
};
static NJS_MKEY_A dat_0cb93478[2] = {
#include "ev0026/sonic_fingers/dat_0cb93478.inc"
};
static NJS_MKEY_A dat_0cb93498[50] = {
#include "ev0026/sonic_fingers/dat_0cb93498.inc"
};
static NJS_MKEY_A dat_0cb937b8[2] = {
#include "ev0026/sonic_fingers/dat_0cb937b8.inc"
};
static NJS_MKEY_A dat_0cb937d8[2] = {
#include "ev0026/sonic_fingers/dat_0cb937d8.inc"
};
static NJS_MKEY_A dat_0cb937f8[2] = {
#include "ev0026/sonic_fingers/dat_0cb937f8.inc"
};
static NJS_MKEY_A dat_0cb93818[2] = {
#include "ev0026/sonic_fingers/dat_0cb93818.inc"
};
static NJS_MKEY_A dat_0cb93838[2] = {
#include "ev0026/sonic_fingers/dat_0cb93838.inc"
};
static NJS_MKEY_A dat_0cb93858[2] = {
#include "ev0026/sonic_fingers/dat_0cb93858.inc"
};
static NJS_MKEY_A dat_0cb93878[2] = {
#include "ev0026/sonic_fingers/dat_0cb93878.inc"
};
static NJS_MKEY_A dat_0cb93898[2] = {
#include "ev0026/sonic_fingers/dat_0cb93898.inc"
};
static NJS_MKEY_A dat_0cb938b8[2] = {
#include "ev0026/sonic_fingers/dat_0cb938b8.inc"
};
static NJS_MKEY_A dat_0cb938d8[2] = {
#include "ev0026/sonic_fingers/dat_0cb938d8.inc"
};
static NJS_MDATA2 dat_0cb938f8[73] = {
#include "ev0026/sonic_fingers/dat_0cb938f8.inc"
};
static NJS_MOTION ev0026_sonic_motion_2 = {
#include "ev0026/sonic_fingers/ev0026_sonic_motion_2.inc"
};
NJS_ACTION ev0026_sonic_action_2 = {
#include "ev0026/sonic_fingers/ev0026_sonic_action_2.inc"
};
static NJS_MKEY_F dat_0cb93d9c[2] = {
#include "ev0026/sonic_fingers/dat_0cb93d9c.inc"
};
static NJS_MKEY_A dat_0cb93dbc[2] = {
#include "ev0026/sonic_fingers/dat_0cb93dbc.inc"
};
static NJS_MKEY_A dat_0cb93ddc[2] = {
#include "ev0026/sonic_fingers/dat_0cb93ddc.inc"
};
static NJS_MKEY_A dat_0cb93dfc[2] = {
#include "ev0026/sonic_fingers/dat_0cb93dfc.inc"
};
static NJS_MKEY_A dat_0cb93e1c[2] = {
#include "ev0026/sonic_fingers/dat_0cb93e1c.inc"
};
static NJS_MKEY_A dat_0cb93e3c[2] = {
#include "ev0026/sonic_fingers/dat_0cb93e3c.inc"
};
static NJS_MKEY_A dat_0cb93e5c[2] = {
#include "ev0026/sonic_fingers/dat_0cb93e5c.inc"
};
static NJS_MKEY_A dat_0cb93e7c[2] = {
#include "ev0026/sonic_fingers/dat_0cb93e7c.inc"
};
static NJS_MKEY_A dat_0cb93e9c[2] = {
#include "ev0026/sonic_fingers/dat_0cb93e9c.inc"
};
static NJS_MKEY_A dat_0cb93ebc[2] = {
#include "ev0026/sonic_fingers/dat_0cb93ebc.inc"
};
static NJS_MKEY_A dat_0cb93edc[2] = {
#include "ev0026/sonic_fingers/dat_0cb93edc.inc"
};
static NJS_MKEY_A dat_0cb93efc[2] = {
#include "ev0026/sonic_fingers/dat_0cb93efc.inc"
};
static NJS_MKEY_A dat_0cb93f1c[2] = {
#include "ev0026/sonic_fingers/dat_0cb93f1c.inc"
};
static NJS_MKEY_A dat_0cb93f3c[2] = {
#include "ev0026/sonic_fingers/dat_0cb93f3c.inc"
};
static NJS_MKEY_A dat_0cb93f5c[2] = {
#include "ev0026/sonic_fingers/dat_0cb93f5c.inc"
};
static NJS_MKEY_A dat_0cb93f7c[2] = {
#include "ev0026/sonic_fingers/dat_0cb93f7c.inc"
};
static NJS_MKEY_A dat_0cb93f9c[2] = {
#include "ev0026/sonic_fingers/dat_0cb93f9c.inc"
};
static NJS_MKEY_A dat_0cb93fbc[2] = {
#include "ev0026/sonic_fingers/dat_0cb93fbc.inc"
};
static NJS_MKEY_A dat_0cb93fdc[2] = {
#include "ev0026/sonic_fingers/dat_0cb93fdc.inc"
};
static NJS_MKEY_A dat_0cb93ffc[2] = {
#include "ev0026/sonic_fingers/dat_0cb93ffc.inc"
};
static NJS_MKEY_A dat_0cb9401c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9401c.inc"
};
static NJS_MKEY_A dat_0cb9403c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9403c.inc"
};
static NJS_MKEY_A dat_0cb9405c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9405c.inc"
};
static NJS_MKEY_A dat_0cb9407c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9407c.inc"
};
static NJS_MKEY_A dat_0cb9409c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9409c.inc"
};
static NJS_MKEY_A dat_0cb940bc[2] = {
#include "ev0026/sonic_fingers/dat_0cb940bc.inc"
};
static NJS_MKEY_A dat_0cb940dc[2] = {
#include "ev0026/sonic_fingers/dat_0cb940dc.inc"
};
static NJS_MKEY_A dat_0cb940fc[2] = {
#include "ev0026/sonic_fingers/dat_0cb940fc.inc"
};
static NJS_MKEY_A dat_0cb9411c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9411c.inc"
};
static NJS_MKEY_A dat_0cb9413c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9413c.inc"
};
static NJS_MKEY_A dat_0cb9415c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9415c.inc"
};
static NJS_MKEY_A dat_0cb9417c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9417c.inc"
};
static NJS_MKEY_A dat_0cb9419c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9419c.inc"
};
static NJS_MKEY_A dat_0cb941bc[2] = {
#include "ev0026/sonic_fingers/dat_0cb941bc.inc"
};
static NJS_MKEY_A dat_0cb941dc[2] = {
#include "ev0026/sonic_fingers/dat_0cb941dc.inc"
};
static NJS_MKEY_A dat_0cb941fc[2] = {
#include "ev0026/sonic_fingers/dat_0cb941fc.inc"
};
static NJS_MKEY_A dat_0cb9421c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9421c.inc"
};
static NJS_MKEY_A dat_0cb9423c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9423c.inc"
};
static NJS_MKEY_A dat_0cb9425c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9425c.inc"
};
static NJS_MKEY_A dat_0cb9427c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9427c.inc"
};
static NJS_MKEY_A dat_0cb9429c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9429c.inc"
};
static NJS_MKEY_A dat_0cb942bc[2] = {
#include "ev0026/sonic_fingers/dat_0cb942bc.inc"
};
static NJS_MKEY_A dat_0cb942dc[2] = {
#include "ev0026/sonic_fingers/dat_0cb942dc.inc"
};
static NJS_MKEY_A dat_0cb942fc[2] = {
#include "ev0026/sonic_fingers/dat_0cb942fc.inc"
};
static NJS_MKEY_A dat_0cb9431c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9431c.inc"
};
static NJS_MKEY_A dat_0cb9433c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9433c.inc"
};
static NJS_MKEY_A dat_0cb9435c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9435c.inc"
};
static NJS_MKEY_A dat_0cb9437c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9437c.inc"
};
static NJS_MKEY_A dat_0cb9439c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9439c.inc"
};
static NJS_MKEY_A dat_0cb943bc[2] = {
#include "ev0026/sonic_fingers/dat_0cb943bc.inc"
};
static NJS_MKEY_A dat_0cb943dc[2] = {
#include "ev0026/sonic_fingers/dat_0cb943dc.inc"
};
static NJS_MKEY_A dat_0cb943fc[2] = {
#include "ev0026/sonic_fingers/dat_0cb943fc.inc"
};
static NJS_MKEY_A dat_0cb9441c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9441c.inc"
};
static NJS_MKEY_A dat_0cb9443c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9443c.inc"
};
static NJS_MKEY_A dat_0cb9445c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9445c.inc"
};
static NJS_MKEY_A dat_0cb9447c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9447c.inc"
};
static NJS_MKEY_A dat_0cb9449c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9449c.inc"
};
static NJS_MKEY_A dat_0cb944bc[2] = {
#include "ev0026/sonic_fingers/dat_0cb944bc.inc"
};
static NJS_MKEY_A dat_0cb944dc[2] = {
#include "ev0026/sonic_fingers/dat_0cb944dc.inc"
};
static NJS_MKEY_A dat_0cb944fc[2] = {
#include "ev0026/sonic_fingers/dat_0cb944fc.inc"
};
static NJS_MKEY_A dat_0cb9451c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9451c.inc"
};
static NJS_MKEY_A dat_0cb9453c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9453c.inc"
};
static NJS_MKEY_A dat_0cb9455c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9455c.inc"
};
static NJS_MKEY_A dat_0cb9457c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9457c.inc"
};
static NJS_MKEY_A dat_0cb9459c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9459c.inc"
};
static NJS_MKEY_A dat_0cb945bc[2] = {
#include "ev0026/sonic_fingers/dat_0cb945bc.inc"
};
static NJS_MKEY_A dat_0cb945dc[2] = {
#include "ev0026/sonic_fingers/dat_0cb945dc.inc"
};
static NJS_MKEY_A dat_0cb945fc[2] = {
#include "ev0026/sonic_fingers/dat_0cb945fc.inc"
};
static NJS_MKEY_A dat_0cb9461c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9461c.inc"
};
static NJS_MKEY_A dat_0cb9463c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9463c.inc"
};
static NJS_MKEY_A dat_0cb9465c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9465c.inc"
};
static NJS_MKEY_A dat_0cb9467c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9467c.inc"
};
static NJS_MKEY_A dat_0cb9469c[2] = {
#include "ev0026/sonic_fingers/dat_0cb9469c.inc"
};
static NJS_MKEY_A dat_0cb946bc[2] = {
#include "ev0026/sonic_fingers/dat_0cb946bc.inc"
};
static NJS_MDATA2 dat_0cb946dc[73] = {
#include "ev0026/sonic_fingers/dat_0cb946dc.inc"
};
static NJS_MOTION ev0026_sonic_motion_3 = {
#include "ev0026/sonic_fingers/ev0026_sonic_motion_3.inc"
};
NJS_ACTION ev0026_sonic_action_3 = {
#include "ev0026/sonic_fingers/ev0026_sonic_action_3.inc"
};
static NJS_MKEY_F dat_0cb94b80[30] = {
#include "ev0026/sonic_fingers/dat_0cb94b80.inc"
};
static NJS_MKEY_A dat_0cb94d60[2] = {
#include "ev0026/sonic_fingers/dat_0cb94d60.inc"
};
static NJS_MKEY_A dat_0cb94d80[2] = {
#include "ev0026/sonic_fingers/dat_0cb94d80.inc"
};
static NJS_MKEY_A dat_0cb94da0[2] = {
#include "ev0026/sonic_fingers/dat_0cb94da0.inc"
};
static NJS_MKEY_A dat_0cb94dc0[2] = {
#include "ev0026/sonic_fingers/dat_0cb94dc0.inc"
};
static NJS_MKEY_A dat_0cb94de0[2] = {
#include "ev0026/sonic_fingers/dat_0cb94de0.inc"
};
static NJS_MKEY_A dat_0cb94e00[2] = {
#include "ev0026/sonic_fingers/dat_0cb94e00.inc"
};
static NJS_MKEY_A dat_0cb94e20[30] = {
#include "ev0026/sonic_fingers/dat_0cb94e20.inc"
};
static NJS_MKEY_A dat_0cb95000[28] = {
#include "ev0026/sonic_fingers/dat_0cb95000.inc"
};
static NJS_MKEY_A dat_0cb951c0[29] = {
#include "ev0026/sonic_fingers/dat_0cb951c0.inc"
};
static NJS_MKEY_A dat_0cb95390[2] = {
#include "ev0026/sonic_fingers/dat_0cb95390.inc"
};
static NJS_MKEY_A dat_0cb953b0[2] = {
#include "ev0026/sonic_fingers/dat_0cb953b0.inc"
};
static NJS_MKEY_A dat_0cb953d0[2] = {
#include "ev0026/sonic_fingers/dat_0cb953d0.inc"
};
static NJS_MKEY_A dat_0cb953f0[30] = {
#include "ev0026/sonic_fingers/dat_0cb953f0.inc"
};
static NJS_MKEY_A dat_0cb955d0[30] = {
#include "ev0026/sonic_fingers/dat_0cb955d0.inc"
};
static NJS_MKEY_A dat_0cb957b0[30] = {
#include "ev0026/sonic_fingers/dat_0cb957b0.inc"
};
static NJS_MKEY_A dat_0cb95990[2] = {
#include "ev0026/sonic_fingers/dat_0cb95990.inc"
};
static NJS_MKEY_A dat_0cb959b0[2] = {
#include "ev0026/sonic_fingers/dat_0cb959b0.inc"
};
static NJS_MKEY_A dat_0cb959d0[2] = {
#include "ev0026/sonic_fingers/dat_0cb959d0.inc"
};
static NJS_MKEY_A dat_0cb959f0[2] = {
#include "ev0026/sonic_fingers/dat_0cb959f0.inc"
};
static NJS_MKEY_A dat_0cb95a10[30] = {
#include "ev0026/sonic_fingers/dat_0cb95a10.inc"
};
static NJS_MKEY_A dat_0cb95bf0[30] = {
#include "ev0026/sonic_fingers/dat_0cb95bf0.inc"
};
static NJS_MKEY_A dat_0cb95dd0[30] = {
#include "ev0026/sonic_fingers/dat_0cb95dd0.inc"
};
static NJS_MKEY_A dat_0cb95fb0[30] = {
#include "ev0026/sonic_fingers/dat_0cb95fb0.inc"
};
static NJS_MKEY_A dat_0cb96190[2] = {
#include "ev0026/sonic_fingers/dat_0cb96190.inc"
};
static NJS_MKEY_A dat_0cb961b0[30] = {
#include "ev0026/sonic_fingers/dat_0cb961b0.inc"
};
static NJS_MKEY_A dat_0cb96390[30] = {
#include "ev0026/sonic_fingers/dat_0cb96390.inc"
};
static NJS_MKEY_A dat_0cb96570[30] = {
#include "ev0026/sonic_fingers/dat_0cb96570.inc"
};
static NJS_MKEY_A dat_0cb96750[30] = {
#include "ev0026/sonic_fingers/dat_0cb96750.inc"
};
static NJS_MKEY_A dat_0cb96930[2] = {
#include "ev0026/sonic_fingers/dat_0cb96930.inc"
};
static NJS_MKEY_A dat_0cb96950[2] = {
#include "ev0026/sonic_fingers/dat_0cb96950.inc"
};
static NJS_MKEY_A dat_0cb96970[2] = {
#include "ev0026/sonic_fingers/dat_0cb96970.inc"
};
static NJS_MKEY_A dat_0cb96990[2] = {
#include "ev0026/sonic_fingers/dat_0cb96990.inc"
};
static NJS_MDATA2 dat_0cb969b0[68] = {
#include "ev0026/sonic_fingers/dat_0cb969b0.inc"
};
static NJS_MOTION ev0026_tails_motion_0 = {
#include "ev0026/sonic_fingers/ev0026_tails_motion_0.inc"
};
NJS_ACTION ev0026_tails_action_0 = {
#include "ev0026/sonic_fingers/ev0026_tails_action_0.inc"
};
static NJS_MKEY_A dat_0cb96e04[32] = {
#include "ev0026/sonic_fingers/dat_0cb96e04.inc"
};
static NJS_MKEY_A dat_0cb97004[32] = {
#include "ev0026/sonic_fingers/dat_0cb97004.inc"
};
static NJS_MKEY_A dat_0cb97204[2] = {
#include "ev0026/sonic_fingers/dat_0cb97204.inc"
};
static NJS_MKEY_A dat_0cb97224[2] = {
#include "ev0026/sonic_fingers/dat_0cb97224.inc"
};
static NJS_MKEY_A dat_0cb97244[2] = {
#include "ev0026/sonic_fingers/dat_0cb97244.inc"
};
static NJS_MKEY_A dat_0cb97264[31] = {
#include "ev0026/sonic_fingers/dat_0cb97264.inc"
};
static NJS_MKEY_A dat_0cb97454[2] = {
#include "ev0026/sonic_fingers/dat_0cb97454.inc"
};
static NJS_MKEY_A dat_0cb97474[2] = {
#include "ev0026/sonic_fingers/dat_0cb97474.inc"
};
static NJS_MKEY_A dat_0cb97494[2] = {
#include "ev0026/sonic_fingers/dat_0cb97494.inc"
};
static NJS_MKEY_A dat_0cb974b4[2] = {
#include "ev0026/sonic_fingers/dat_0cb974b4.inc"
};
static NJS_MKEY_A dat_0cb974d4[2] = {
#include "ev0026/sonic_fingers/dat_0cb974d4.inc"
};
static NJS_MKEY_A dat_0cb974f4[2] = {
#include "ev0026/sonic_fingers/dat_0cb974f4.inc"
};
static NJS_MKEY_A dat_0cb97514[31] = {
#include "ev0026/sonic_fingers/dat_0cb97514.inc"
};
static NJS_MKEY_A dat_0cb97704[2] = {
#include "ev0026/sonic_fingers/dat_0cb97704.inc"
};
static NJS_MKEY_A dat_0cb97724[2] = {
#include "ev0026/sonic_fingers/dat_0cb97724.inc"
};
static NJS_MKEY_A dat_0cb97744[2] = {
#include "ev0026/sonic_fingers/dat_0cb97744.inc"
};
static NJS_MKEY_A dat_0cb97764[2] = {
#include "ev0026/sonic_fingers/dat_0cb97764.inc"
};
static NJS_MKEY_A dat_0cb97784[2] = {
#include "ev0026/sonic_fingers/dat_0cb97784.inc"
};
static NJS_MKEY_A dat_0cb977a4[2] = {
#include "ev0026/sonic_fingers/dat_0cb977a4.inc"
};
static NJS_MKEY_A dat_0cb977c4[32] = {
#include "ev0026/sonic_fingers/dat_0cb977c4.inc"
};
static NJS_MKEY_A dat_0cb979c4[28] = {
#include "ev0026/sonic_fingers/dat_0cb979c4.inc"
};
static NJS_MKEY_A dat_0cb97b84[29] = {
#include "ev0026/sonic_fingers/dat_0cb97b84.inc"
};
static NJS_MKEY_A dat_0cb97d54[32] = {
#include "ev0026/sonic_fingers/dat_0cb97d54.inc"
};
static NJS_MKEY_A dat_0cb97f54[2] = {
#include "ev0026/sonic_fingers/dat_0cb97f54.inc"
};
static NJS_MKEY_A dat_0cb97f74[32] = {
#include "ev0026/sonic_fingers/dat_0cb97f74.inc"
};
static NJS_MKEY_A dat_0cb98174[27] = {
#include "ev0026/sonic_fingers/dat_0cb98174.inc"
};
static NJS_MKEY_A dat_0cb98324[30] = {
#include "ev0026/sonic_fingers/dat_0cb98324.inc"
};
static NJS_MKEY_A dat_0cb98504[32] = {
#include "ev0026/sonic_fingers/dat_0cb98504.inc"
};
static NJS_MDATA2 dat_0cb98704[68] = {
#include "ev0026/sonic_fingers/dat_0cb98704.inc"
};
static NJS_MOTION ev0026_tails_motion_1 = {
#include "ev0026/sonic_fingers/ev0026_tails_motion_1.inc"
};
NJS_ACTION ev0026_tails_action_1 = {
#include "ev0026/sonic_fingers/ev0026_tails_action_1.inc"
};
static NJS_MKEY_F dat_0cb98b58[16] = {
#include "ev0026/sonic_fingers/dat_0cb98b58.inc"
};
static NJS_MKEY_A dat_0cb98c58[16] = {
#include "ev0026/sonic_fingers/dat_0cb98c58.inc"
};
static NJS_MKEY_A dat_0cb98d58[16] = {
#include "ev0026/sonic_fingers/dat_0cb98d58.inc"
};
static NJS_MKEY_A dat_0cb98e58[16] = {
#include "ev0026/sonic_fingers/dat_0cb98e58.inc"
};
static NJS_MKEY_A dat_0cb98f58[16] = {
#include "ev0026/sonic_fingers/dat_0cb98f58.inc"
};
static NJS_MKEY_A dat_0cb99058[16] = {
#include "ev0026/sonic_fingers/dat_0cb99058.inc"
};
static NJS_MKEY_A dat_0cb99158[16] = {
#include "ev0026/sonic_fingers/dat_0cb99158.inc"
};
static NJS_MKEY_A dat_0cb99258[16] = {
#include "ev0026/sonic_fingers/dat_0cb99258.inc"
};
static NJS_MKEY_A dat_0cb99358[16] = {
#include "ev0026/sonic_fingers/dat_0cb99358.inc"
};
static NJS_MKEY_A dat_0cb99458[16] = {
#include "ev0026/sonic_fingers/dat_0cb99458.inc"
};
static NJS_MKEY_A dat_0cb99558[16] = {
#include "ev0026/sonic_fingers/dat_0cb99558.inc"
};
static NJS_MKEY_A dat_0cb99658[16] = {
#include "ev0026/sonic_fingers/dat_0cb99658.inc"
};
static NJS_MKEY_A dat_0cb99758[16] = {
#include "ev0026/sonic_fingers/dat_0cb99758.inc"
};
static NJS_MKEY_A dat_0cb99858[16] = {
#include "ev0026/sonic_fingers/dat_0cb99858.inc"
};
static NJS_MKEY_A dat_0cb99958[16] = {
#include "ev0026/sonic_fingers/dat_0cb99958.inc"
};
static NJS_MKEY_A dat_0cb99a58[16] = {
#include "ev0026/sonic_fingers/dat_0cb99a58.inc"
};
static NJS_MKEY_A dat_0cb99b58[16] = {
#include "ev0026/sonic_fingers/dat_0cb99b58.inc"
};
static NJS_MKEY_A dat_0cb99c58[16] = {
#include "ev0026/sonic_fingers/dat_0cb99c58.inc"
};
static NJS_MKEY_A dat_0cb99d58[16] = {
#include "ev0026/sonic_fingers/dat_0cb99d58.inc"
};
static NJS_MKEY_A dat_0cb99e58[16] = {
#include "ev0026/sonic_fingers/dat_0cb99e58.inc"
};
static NJS_MKEY_A dat_0cb99f58[16] = {
#include "ev0026/sonic_fingers/dat_0cb99f58.inc"
};
static NJS_MDATA2 dat_0cb9a058[68] = {
#include "ev0026/sonic_fingers/dat_0cb9a058.inc"
};
static NJS_MOTION ev0026_tails_motion_2 = {
#include "ev0026/sonic_fingers/ev0026_tails_motion_2.inc"
};
NJS_ACTION ev0026_tails_action_2 = {
#include "ev0026/sonic_fingers/ev0026_tails_action_2.inc"
};
static NJS_TEXNAME dat_0cb9a4ac[5] = {
    {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
};
static NJS_TEXLIST dat_0cb9a4e8 = {
#include "ev0026/sonic_fingers/dat_0cb9a4e8.inc"
};
static NJS_TEXNAME event_clouds_textures[3] = {
    {TEXT_7, 0, 0},
    {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},
};
NJS_TEXLIST event_clouds_texlist = {
#include "ev0026/sonic_fingers/event_clouds_texlist.inc"
};
