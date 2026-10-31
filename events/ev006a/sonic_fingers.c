/* dat_0cb860dc, a texture list of one name, opens the object. Sonic's fingers
 * follow, then Sonic's six actions, Tails' two and Amy's four. the models those
 * actions move lie in 1ST_READ.BIN. the bird's first model comes next, then
 * ev006a_bird_textures_1 and ev006a_bird_texlist_1. the bird's second model
 * and ev006a_bird_textures_2 end the object. ev006a_bird_action moves the first
 * model, whose root is dat_0cba20f8. the script plays that action with
 * ev006a_bird_texlist_1. the second model, whose root is dat_0cba6244, matches
 * the first but for five nodes. the first scales those to 1e-5, and their
 * models' radii shrink with them. ev006a_bird_textures_2 holds the texture
 * names of ev006a_bird_texlist_2, which lies in the data after this object.
 * only ev006a_textures points at that list, which the event loads from
 * VER2_WING. no word of EV006A.BIN or 1ST_READ.BIN points at dat_0cb860dc or
 * dat_0cba6244 */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev006a/sonic_fingers.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 11
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 7
#define TEXT_12_LENGTH 7
#define TEXT_13_LENGTH 9
#include "ev006a/sonic_fingers/text.inc"

extern NJS_OBJECT sonic_object;
extern NJS_OBJECT tails_object;
extern NJS_OBJECT tails_fly_object;
extern NJS_OBJECT amy_object;

static NJS_TEXNAME dat_0cb860d0[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cb860dc = {
#include "ev006a/sonic_fingers/dat_0cb860dc.inc"
};
static NJS_MATERIAL dat_0cb860e4[3] = {
#include "ev006a/sonic_fingers/dat_0cb860e4.inc"
};
static Sint16 dat_0cb86120[20] = {
#include "ev006a/sonic_fingers/dat_0cb86120.inc"
};
static Sint16 dat_0cb86148[38] = {
#include "ev006a/sonic_fingers/dat_0cb86148.inc"
};
static Sint16 dat_0cb86194[40] = {
#include "ev006a/sonic_fingers/dat_0cb86194.inc"
};
static NJS_COLOR dat_0cb861e4[32] = {
#include "ev006a/sonic_fingers/dat_0cb861e4.inc"
};
static NJS_MESHSET dat_0cb86264[3] = {
#include "ev006a/sonic_fingers/dat_0cb86264.inc"
};
static NJS_POINT3 dat_0cb862ac[28] = {
#include "ev006a/sonic_fingers/dat_0cb862ac.inc"
};
static NJS_VECTOR dat_0cb863fc[28] = {
#include "ev006a/sonic_fingers/dat_0cb863fc.inc"
};
static NJS_MODEL dat_0cb8654c = {
#include "ev006a/sonic_fingers/dat_0cb8654c.inc"
};
NJS_OBJECT ev006a_sonic_fingers = {
#include "ev006a/sonic_fingers/ev006a_sonic_fingers.inc"
};
static NJS_MKEY_F dat_0cb865a8[8] = {
#include "ev006a/sonic_fingers/dat_0cb865a8.inc"
};
static NJS_MKEY_A dat_0cb86628[8] = {
#include "ev006a/sonic_fingers/dat_0cb86628.inc"
};
static NJS_MKEY_A dat_0cb866a8[8] = {
#include "ev006a/sonic_fingers/dat_0cb866a8.inc"
};
static NJS_MKEY_A dat_0cb86728[8] = {
#include "ev006a/sonic_fingers/dat_0cb86728.inc"
};
static NJS_MKEY_A dat_0cb867a8[8] = {
#include "ev006a/sonic_fingers/dat_0cb867a8.inc"
};
static NJS_MKEY_A dat_0cb86828[8] = {
#include "ev006a/sonic_fingers/dat_0cb86828.inc"
};
static NJS_MKEY_A dat_0cb868a8[8] = {
#include "ev006a/sonic_fingers/dat_0cb868a8.inc"
};
static NJS_MKEY_A dat_0cb86928[8] = {
#include "ev006a/sonic_fingers/dat_0cb86928.inc"
};
static NJS_MKEY_A dat_0cb869a8[8] = {
#include "ev006a/sonic_fingers/dat_0cb869a8.inc"
};
static NJS_MKEY_A dat_0cb86a28[8] = {
#include "ev006a/sonic_fingers/dat_0cb86a28.inc"
};
static NJS_MKEY_A dat_0cb86aa8[8] = {
#include "ev006a/sonic_fingers/dat_0cb86aa8.inc"
};
static NJS_MKEY_A dat_0cb86b28[7] = {
#include "ev006a/sonic_fingers/dat_0cb86b28.inc"
};
static NJS_MKEY_A dat_0cb86b98[8] = {
#include "ev006a/sonic_fingers/dat_0cb86b98.inc"
};
static NJS_MKEY_A dat_0cb86c18[8] = {
#include "ev006a/sonic_fingers/dat_0cb86c18.inc"
};
static NJS_MKEY_A dat_0cb86c98[8] = {
#include "ev006a/sonic_fingers/dat_0cb86c98.inc"
};
static NJS_MKEY_A dat_0cb86d18[8] = {
#include "ev006a/sonic_fingers/dat_0cb86d18.inc"
};
static NJS_MKEY_A dat_0cb86d98[8] = {
#include "ev006a/sonic_fingers/dat_0cb86d98.inc"
};
static NJS_MKEY_A dat_0cb86e18[8] = {
#include "ev006a/sonic_fingers/dat_0cb86e18.inc"
};
static NJS_MKEY_A dat_0cb86e98[8] = {
#include "ev006a/sonic_fingers/dat_0cb86e98.inc"
};
static NJS_MKEY_A dat_0cb86f18[8] = {
#include "ev006a/sonic_fingers/dat_0cb86f18.inc"
};
static NJS_MKEY_A dat_0cb86f98[8] = {
#include "ev006a/sonic_fingers/dat_0cb86f98.inc"
};
static NJS_MKEY_A dat_0cb87018[8] = {
#include "ev006a/sonic_fingers/dat_0cb87018.inc"
};
static NJS_MKEY_A dat_0cb87098[6] = {
#include "ev006a/sonic_fingers/dat_0cb87098.inc"
};
static NJS_MKEY_A dat_0cb870f8[6] = {
#include "ev006a/sonic_fingers/dat_0cb870f8.inc"
};
static NJS_MKEY_A dat_0cb87158[8] = {
#include "ev006a/sonic_fingers/dat_0cb87158.inc"
};
static NJS_MKEY_A dat_0cb871d8[8] = {
#include "ev006a/sonic_fingers/dat_0cb871d8.inc"
};
static NJS_MDATA2 dat_0cb87258[73] = {
#include "ev006a/sonic_fingers/dat_0cb87258.inc"
};
static NJS_MOTION ev006a_sonic_motion_0 = {
#include "ev006a/sonic_fingers/ev006a_sonic_motion_0.inc"
};
NJS_ACTION ev006a_sonic_action_0 = {
#include "ev006a/sonic_fingers/ev006a_sonic_action_0.inc"
};
static NJS_MKEY_F dat_0cb876fc[5] = {
#include "ev006a/sonic_fingers/dat_0cb876fc.inc"
};
static NJS_MKEY_A dat_0cb8774c[4] = {
#include "ev006a/sonic_fingers/dat_0cb8774c.inc"
};
static NJS_MKEY_A dat_0cb8778c[9] = {
#include "ev006a/sonic_fingers/dat_0cb8778c.inc"
};
static NJS_MKEY_A dat_0cb8781c[11] = {
#include "ev006a/sonic_fingers/dat_0cb8781c.inc"
};
static NJS_MKEY_A dat_0cb878cc[8] = {
#include "ev006a/sonic_fingers/dat_0cb878cc.inc"
};
static NJS_MKEY_A dat_0cb8794c[31] = {
#include "ev006a/sonic_fingers/dat_0cb8794c.inc"
};
static NJS_MKEY_A dat_0cb87b3c[31] = {
#include "ev006a/sonic_fingers/dat_0cb87b3c.inc"
};
static NJS_MKEY_A dat_0cb87d2c[31] = {
#include "ev006a/sonic_fingers/dat_0cb87d2c.inc"
};
static NJS_MKEY_A dat_0cb87f1c[31] = {
#include "ev006a/sonic_fingers/dat_0cb87f1c.inc"
};
static NJS_MKEY_A dat_0cb8810c[31] = {
#include "ev006a/sonic_fingers/dat_0cb8810c.inc"
};
static NJS_MKEY_A dat_0cb882fc[32] = {
#include "ev006a/sonic_fingers/dat_0cb882fc.inc"
};
static NJS_MKEY_A dat_0cb884fc[31] = {
#include "ev006a/sonic_fingers/dat_0cb884fc.inc"
};
static NJS_MKEY_A dat_0cb886ec[31] = {
#include "ev006a/sonic_fingers/dat_0cb886ec.inc"
};
static NJS_MKEY_A dat_0cb888dc[31] = {
#include "ev006a/sonic_fingers/dat_0cb888dc.inc"
};
static NJS_MKEY_A dat_0cb88acc[6] = {
#include "ev006a/sonic_fingers/dat_0cb88acc.inc"
};
static NJS_MKEY_A dat_0cb88b2c[4] = {
#include "ev006a/sonic_fingers/dat_0cb88b2c.inc"
};
static NJS_MKEY_A dat_0cb88b6c[4] = {
#include "ev006a/sonic_fingers/dat_0cb88b6c.inc"
};
static NJS_MKEY_A dat_0cb88bac[30] = {
#include "ev006a/sonic_fingers/dat_0cb88bac.inc"
};
static NJS_MKEY_A dat_0cb88d8c[31] = {
#include "ev006a/sonic_fingers/dat_0cb88d8c.inc"
};
static NJS_MKEY_A dat_0cb88f7c[9] = {
#include "ev006a/sonic_fingers/dat_0cb88f7c.inc"
};
static NJS_MDATA2 dat_0cb8900c[73] = {
#include "ev006a/sonic_fingers/dat_0cb8900c.inc"
};
static NJS_MOTION ev006a_sonic_motion_1 = {
#include "ev006a/sonic_fingers/ev006a_sonic_motion_1.inc"
};
NJS_ACTION ev006a_sonic_action_1 = {
#include "ev006a/sonic_fingers/ev006a_sonic_action_1.inc"
};
static NJS_MKEY_F dat_0cb894b0[30] = {
#include "ev006a/sonic_fingers/dat_0cb894b0.inc"
};
static NJS_MKEY_A dat_0cb89690[30] = {
#include "ev006a/sonic_fingers/dat_0cb89690.inc"
};
static NJS_MKEY_A dat_0cb89870[30] = {
#include "ev006a/sonic_fingers/dat_0cb89870.inc"
};
static NJS_MKEY_A dat_0cb89a50[29] = {
#include "ev006a/sonic_fingers/dat_0cb89a50.inc"
};
static NJS_MKEY_A dat_0cb89c20[30] = {
#include "ev006a/sonic_fingers/dat_0cb89c20.inc"
};
static NJS_MKEY_A dat_0cb89e00[29] = {
#include "ev006a/sonic_fingers/dat_0cb89e00.inc"
};
static NJS_MKEY_A dat_0cb89fd0[29] = {
#include "ev006a/sonic_fingers/dat_0cb89fd0.inc"
};
static NJS_MKEY_A dat_0cb8a1a0[30] = {
#include "ev006a/sonic_fingers/dat_0cb8a1a0.inc"
};
static NJS_MKEY_A dat_0cb8a380[30] = {
#include "ev006a/sonic_fingers/dat_0cb8a380.inc"
};
static NJS_MKEY_A dat_0cb8a560[8] = {
#include "ev006a/sonic_fingers/dat_0cb8a560.inc"
};
static NJS_MKEY_A dat_0cb8a5e0[23] = {
#include "ev006a/sonic_fingers/dat_0cb8a5e0.inc"
};
static NJS_MKEY_A dat_0cb8a750[23] = {
#include "ev006a/sonic_fingers/dat_0cb8a750.inc"
};
static NJS_MKEY_A dat_0cb8a8c0[22] = {
#include "ev006a/sonic_fingers/dat_0cb8a8c0.inc"
};
static NJS_MKEY_A dat_0cb8aa20[24] = {
#include "ev006a/sonic_fingers/dat_0cb8aa20.inc"
};
static NJS_MKEY_A dat_0cb8aba0[30] = {
#include "ev006a/sonic_fingers/dat_0cb8aba0.inc"
};
static NJS_MKEY_A dat_0cb8ad80[24] = {
#include "ev006a/sonic_fingers/dat_0cb8ad80.inc"
};
static NJS_MKEY_A dat_0cb8af00[26] = {
#include "ev006a/sonic_fingers/dat_0cb8af00.inc"
};
static NJS_MKEY_A dat_0cb8b0a0[29] = {
#include "ev006a/sonic_fingers/dat_0cb8b0a0.inc"
};
static NJS_MKEY_A dat_0cb8b270[30] = {
#include "ev006a/sonic_fingers/dat_0cb8b270.inc"
};
static NJS_MKEY_A dat_0cb8b450[30] = {
#include "ev006a/sonic_fingers/dat_0cb8b450.inc"
};
static NJS_MKEY_A dat_0cb8b630[30] = {
#include "ev006a/sonic_fingers/dat_0cb8b630.inc"
};
static NJS_MKEY_A dat_0cb8b810[30] = {
#include "ev006a/sonic_fingers/dat_0cb8b810.inc"
};
static NJS_MDATA2 dat_0cb8b9f0[73] = {
#include "ev006a/sonic_fingers/dat_0cb8b9f0.inc"
};
static NJS_MOTION ev006a_sonic_motion_2 = {
#include "ev006a/sonic_fingers/ev006a_sonic_motion_2.inc"
};
NJS_ACTION ev006a_sonic_action_2 = {
#include "ev006a/sonic_fingers/ev006a_sonic_action_2.inc"
};
static NJS_MKEY_F dat_0cb8be94[15] = {
#include "ev006a/sonic_fingers/dat_0cb8be94.inc"
};
static NJS_MKEY_A dat_0cb8bf84[2] = {
#include "ev006a/sonic_fingers/dat_0cb8bf84.inc"
};
static NJS_MKEY_A dat_0cb8bfa4[14] = {
#include "ev006a/sonic_fingers/dat_0cb8bfa4.inc"
};
static NJS_MKEY_A dat_0cb8c084[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c084.inc"
};
static NJS_MKEY_A dat_0cb8c0a4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c0a4.inc"
};
static NJS_MKEY_A dat_0cb8c0c4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c0c4.inc"
};
static NJS_MKEY_A dat_0cb8c0e4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c0e4.inc"
};
static NJS_MKEY_A dat_0cb8c104[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c104.inc"
};
static NJS_MKEY_A dat_0cb8c124[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c124.inc"
};
static NJS_MKEY_A dat_0cb8c144[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c144.inc"
};
static NJS_MKEY_A dat_0cb8c164[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c164.inc"
};
static NJS_MKEY_A dat_0cb8c184[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c184.inc"
};
static NJS_MKEY_A dat_0cb8c1a4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c1a4.inc"
};
static NJS_MKEY_A dat_0cb8c1c4[15] = {
#include "ev006a/sonic_fingers/dat_0cb8c1c4.inc"
};
static NJS_MKEY_A dat_0cb8c2b4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c2b4.inc"
};
static NJS_MKEY_A dat_0cb8c2d4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c2d4.inc"
};
static NJS_MKEY_A dat_0cb8c2f4[12] = {
#include "ev006a/sonic_fingers/dat_0cb8c2f4.inc"
};
static NJS_MKEY_A dat_0cb8c3b4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c3b4.inc"
};
static NJS_MKEY_A dat_0cb8c3d4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c3d4.inc"
};
static NJS_MKEY_A dat_0cb8c3f4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c3f4.inc"
};
static NJS_MKEY_A dat_0cb8c414[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c414.inc"
};
static NJS_MKEY_A dat_0cb8c434[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c434.inc"
};
static NJS_MKEY_A dat_0cb8c454[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c454.inc"
};
static NJS_MKEY_A dat_0cb8c474[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c474.inc"
};
static NJS_MKEY_A dat_0cb8c494[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c494.inc"
};
static NJS_MKEY_A dat_0cb8c4b4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c4b4.inc"
};
static NJS_MKEY_A dat_0cb8c4d4[15] = {
#include "ev006a/sonic_fingers/dat_0cb8c4d4.inc"
};
static NJS_MKEY_A dat_0cb8c5c4[15] = {
#include "ev006a/sonic_fingers/dat_0cb8c5c4.inc"
};
static NJS_MKEY_A dat_0cb8c6b4[15] = {
#include "ev006a/sonic_fingers/dat_0cb8c6b4.inc"
};
static NJS_MKEY_A dat_0cb8c7a4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c7a4.inc"
};
static NJS_MKEY_A dat_0cb8c7c4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c7c4.inc"
};
static NJS_MKEY_A dat_0cb8c7e4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c7e4.inc"
};
static NJS_MKEY_A dat_0cb8c804[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c804.inc"
};
static NJS_MKEY_A dat_0cb8c824[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c824.inc"
};
static NJS_MKEY_A dat_0cb8c844[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c844.inc"
};
static NJS_MKEY_A dat_0cb8c864[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c864.inc"
};
static NJS_MKEY_A dat_0cb8c884[2] = {
#include "ev006a/sonic_fingers/dat_0cb8c884.inc"
};
static NJS_MKEY_A dat_0cb8c8a4[15] = {
#include "ev006a/sonic_fingers/dat_0cb8c8a4.inc"
};
static NJS_MKEY_A dat_0cb8c994[15] = {
#include "ev006a/sonic_fingers/dat_0cb8c994.inc"
};
static NJS_MKEY_A dat_0cb8ca84[15] = {
#include "ev006a/sonic_fingers/dat_0cb8ca84.inc"
};
static NJS_MKEY_A dat_0cb8cb74[2] = {
#include "ev006a/sonic_fingers/dat_0cb8cb74.inc"
};
static NJS_MKEY_A dat_0cb8cb94[2] = {
#include "ev006a/sonic_fingers/dat_0cb8cb94.inc"
};
static NJS_MKEY_A dat_0cb8cbb4[15] = {
#include "ev006a/sonic_fingers/dat_0cb8cbb4.inc"
};
static NJS_MKEY_A dat_0cb8cca4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8cca4.inc"
};
static NJS_MKEY_A dat_0cb8ccc4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8ccc4.inc"
};
static NJS_MKEY_A dat_0cb8cce4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8cce4.inc"
};
static NJS_MKEY_A dat_0cb8cd04[2] = {
#include "ev006a/sonic_fingers/dat_0cb8cd04.inc"
};
static NJS_MKEY_A dat_0cb8cd24[2] = {
#include "ev006a/sonic_fingers/dat_0cb8cd24.inc"
};
static NJS_MKEY_A dat_0cb8cd44[15] = {
#include "ev006a/sonic_fingers/dat_0cb8cd44.inc"
};
static NJS_MKEY_A dat_0cb8ce34[15] = {
#include "ev006a/sonic_fingers/dat_0cb8ce34.inc"
};
static NJS_MKEY_A dat_0cb8cf24[15] = {
#include "ev006a/sonic_fingers/dat_0cb8cf24.inc"
};
static NJS_MKEY_A dat_0cb8d014[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d014.inc"
};
static NJS_MKEY_A dat_0cb8d034[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d034.inc"
};
static NJS_MKEY_A dat_0cb8d054[15] = {
#include "ev006a/sonic_fingers/dat_0cb8d054.inc"
};
static NJS_MKEY_A dat_0cb8d144[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d144.inc"
};
static NJS_MKEY_A dat_0cb8d164[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d164.inc"
};
static NJS_MKEY_A dat_0cb8d184[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d184.inc"
};
static NJS_MKEY_A dat_0cb8d1a4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d1a4.inc"
};
static NJS_MKEY_A dat_0cb8d1c4[15] = {
#include "ev006a/sonic_fingers/dat_0cb8d1c4.inc"
};
static NJS_MKEY_A dat_0cb8d2b4[15] = {
#include "ev006a/sonic_fingers/dat_0cb8d2b4.inc"
};
static NJS_MKEY_A dat_0cb8d3a4[15] = {
#include "ev006a/sonic_fingers/dat_0cb8d3a4.inc"
};
static NJS_MKEY_A dat_0cb8d494[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d494.inc"
};
static NJS_MKEY_A dat_0cb8d4b4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d4b4.inc"
};
static NJS_MKEY_A dat_0cb8d4d4[15] = {
#include "ev006a/sonic_fingers/dat_0cb8d4d4.inc"
};
static NJS_MKEY_A dat_0cb8d5c4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d5c4.inc"
};
static NJS_MKEY_A dat_0cb8d5e4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d5e4.inc"
};
static NJS_MKEY_A dat_0cb8d604[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d604.inc"
};
static NJS_MKEY_A dat_0cb8d624[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d624.inc"
};
static NJS_MKEY_A dat_0cb8d644[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d644.inc"
};
static NJS_MKEY_A dat_0cb8d664[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d664.inc"
};
static NJS_MKEY_A dat_0cb8d684[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d684.inc"
};
static NJS_MKEY_A dat_0cb8d6a4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d6a4.inc"
};
static NJS_MKEY_A dat_0cb8d6c4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d6c4.inc"
};
static NJS_MKEY_A dat_0cb8d6e4[2] = {
#include "ev006a/sonic_fingers/dat_0cb8d6e4.inc"
};
static NJS_MDATA2 dat_0cb8d704[73] = {
#include "ev006a/sonic_fingers/dat_0cb8d704.inc"
};
static NJS_MOTION ev006a_sonic_motion_3 = {
#include "ev006a/sonic_fingers/ev006a_sonic_motion_3.inc"
};
NJS_ACTION ev006a_sonic_action_3 = {
#include "ev006a/sonic_fingers/ev006a_sonic_action_3.inc"
};
static NJS_MKEY_F dat_0cb8dba8[31] = {
#include "ev006a/sonic_fingers/dat_0cb8dba8.inc"
};
static NJS_MKEY_A dat_0cb8dd98[2] = {
#include "ev006a/sonic_fingers/dat_0cb8dd98.inc"
};
static NJS_MKEY_A dat_0cb8ddb8[30] = {
#include "ev006a/sonic_fingers/dat_0cb8ddb8.inc"
};
static NJS_MKEY_A dat_0cb8df98[2] = {
#include "ev006a/sonic_fingers/dat_0cb8df98.inc"
};
static NJS_MKEY_A dat_0cb8dfb8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8dfb8.inc"
};
static NJS_MKEY_A dat_0cb8dfd8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8dfd8.inc"
};
static NJS_MKEY_A dat_0cb8dff8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8dff8.inc"
};
static NJS_MKEY_A dat_0cb8e018[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e018.inc"
};
static NJS_MKEY_A dat_0cb8e038[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e038.inc"
};
static NJS_MKEY_A dat_0cb8e058[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e058.inc"
};
static NJS_MKEY_A dat_0cb8e078[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e078.inc"
};
static NJS_MKEY_A dat_0cb8e098[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e098.inc"
};
static NJS_MKEY_A dat_0cb8e0b8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e0b8.inc"
};
static NJS_MKEY_A dat_0cb8e0d8[21] = {
#include "ev006a/sonic_fingers/dat_0cb8e0d8.inc"
};
static NJS_MKEY_A dat_0cb8e228[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e228.inc"
};
static NJS_MKEY_A dat_0cb8e248[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e248.inc"
};
static NJS_MKEY_A dat_0cb8e268[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e268.inc"
};
static NJS_MKEY_A dat_0cb8e288[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e288.inc"
};
static NJS_MKEY_A dat_0cb8e2a8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e2a8.inc"
};
static NJS_MKEY_A dat_0cb8e2c8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e2c8.inc"
};
static NJS_MKEY_A dat_0cb8e2e8[6] = {
#include "ev006a/sonic_fingers/dat_0cb8e2e8.inc"
};
static NJS_MKEY_A dat_0cb8e348[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e348.inc"
};
static NJS_MKEY_A dat_0cb8e368[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e368.inc"
};
static NJS_MKEY_A dat_0cb8e388[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e388.inc"
};
static NJS_MKEY_A dat_0cb8e3a8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e3a8.inc"
};
static NJS_MKEY_A dat_0cb8e3c8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e3c8.inc"
};
static NJS_MKEY_A dat_0cb8e3e8[30] = {
#include "ev006a/sonic_fingers/dat_0cb8e3e8.inc"
};
static NJS_MKEY_A dat_0cb8e5c8[30] = {
#include "ev006a/sonic_fingers/dat_0cb8e5c8.inc"
};
static NJS_MKEY_A dat_0cb8e7a8[29] = {
#include "ev006a/sonic_fingers/dat_0cb8e7a8.inc"
};
static NJS_MKEY_A dat_0cb8e978[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e978.inc"
};
static NJS_MKEY_A dat_0cb8e998[2] = {
#include "ev006a/sonic_fingers/dat_0cb8e998.inc"
};
static NJS_MKEY_A dat_0cb8e9b8[31] = {
#include "ev006a/sonic_fingers/dat_0cb8e9b8.inc"
};
static NJS_MKEY_A dat_0cb8eba8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8eba8.inc"
};
static NJS_MKEY_A dat_0cb8ebc8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8ebc8.inc"
};
static NJS_MKEY_A dat_0cb8ebe8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8ebe8.inc"
};
static NJS_MKEY_A dat_0cb8ec08[2] = {
#include "ev006a/sonic_fingers/dat_0cb8ec08.inc"
};
static NJS_MKEY_A dat_0cb8ec28[2] = {
#include "ev006a/sonic_fingers/dat_0cb8ec28.inc"
};
static NJS_MKEY_A dat_0cb8ec48[31] = {
#include "ev006a/sonic_fingers/dat_0cb8ec48.inc"
};
static NJS_MKEY_A dat_0cb8ee38[31] = {
#include "ev006a/sonic_fingers/dat_0cb8ee38.inc"
};
static NJS_MKEY_A dat_0cb8f028[31] = {
#include "ev006a/sonic_fingers/dat_0cb8f028.inc"
};
static NJS_MKEY_A dat_0cb8f218[2] = {
#include "ev006a/sonic_fingers/dat_0cb8f218.inc"
};
static NJS_MKEY_A dat_0cb8f238[2] = {
#include "ev006a/sonic_fingers/dat_0cb8f238.inc"
};
static NJS_MKEY_A dat_0cb8f258[2] = {
#include "ev006a/sonic_fingers/dat_0cb8f258.inc"
};
static NJS_MKEY_A dat_0cb8f278[2] = {
#include "ev006a/sonic_fingers/dat_0cb8f278.inc"
};
static NJS_MKEY_A dat_0cb8f298[2] = {
#include "ev006a/sonic_fingers/dat_0cb8f298.inc"
};
static NJS_MKEY_A dat_0cb8f2b8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8f2b8.inc"
};
static NJS_MKEY_A dat_0cb8f2d8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8f2d8.inc"
};
static NJS_MKEY_A dat_0cb8f2f8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8f2f8.inc"
};
static NJS_MKEY_A dat_0cb8f318[30] = {
#include "ev006a/sonic_fingers/dat_0cb8f318.inc"
};
static NJS_MKEY_A dat_0cb8f4f8[30] = {
#include "ev006a/sonic_fingers/dat_0cb8f4f8.inc"
};
static NJS_MKEY_A dat_0cb8f6d8[30] = {
#include "ev006a/sonic_fingers/dat_0cb8f6d8.inc"
};
static NJS_MKEY_A dat_0cb8f8b8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8f8b8.inc"
};
static NJS_MKEY_A dat_0cb8f8d8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8f8d8.inc"
};
static NJS_MKEY_A dat_0cb8f8f8[31] = {
#include "ev006a/sonic_fingers/dat_0cb8f8f8.inc"
};
static NJS_MKEY_A dat_0cb8fae8[2] = {
#include "ev006a/sonic_fingers/dat_0cb8fae8.inc"
};
static NJS_MKEY_A dat_0cb8fb08[2] = {
#include "ev006a/sonic_fingers/dat_0cb8fb08.inc"
};
static NJS_MKEY_A dat_0cb8fb28[2] = {
#include "ev006a/sonic_fingers/dat_0cb8fb28.inc"
};
static NJS_MKEY_A dat_0cb8fb48[2] = {
#include "ev006a/sonic_fingers/dat_0cb8fb48.inc"
};
static NJS_MKEY_A dat_0cb8fb68[30] = {
#include "ev006a/sonic_fingers/dat_0cb8fb68.inc"
};
static NJS_MKEY_A dat_0cb8fd48[30] = {
#include "ev006a/sonic_fingers/dat_0cb8fd48.inc"
};
static NJS_MKEY_A dat_0cb8ff28[30] = {
#include "ev006a/sonic_fingers/dat_0cb8ff28.inc"
};
static NJS_MKEY_A dat_0cb90108[2] = {
#include "ev006a/sonic_fingers/dat_0cb90108.inc"
};
static NJS_MKEY_A dat_0cb90128[2] = {
#include "ev006a/sonic_fingers/dat_0cb90128.inc"
};
static NJS_MKEY_A dat_0cb90148[31] = {
#include "ev006a/sonic_fingers/dat_0cb90148.inc"
};
static NJS_MKEY_A dat_0cb90338[2] = {
#include "ev006a/sonic_fingers/dat_0cb90338.inc"
};
static NJS_MKEY_A dat_0cb90358[2] = {
#include "ev006a/sonic_fingers/dat_0cb90358.inc"
};
static NJS_MKEY_A dat_0cb90378[2] = {
#include "ev006a/sonic_fingers/dat_0cb90378.inc"
};
static NJS_MKEY_A dat_0cb90398[2] = {
#include "ev006a/sonic_fingers/dat_0cb90398.inc"
};
static NJS_MKEY_A dat_0cb903b8[2] = {
#include "ev006a/sonic_fingers/dat_0cb903b8.inc"
};
static NJS_MKEY_A dat_0cb903d8[2] = {
#include "ev006a/sonic_fingers/dat_0cb903d8.inc"
};
static NJS_MKEY_A dat_0cb903f8[2] = {
#include "ev006a/sonic_fingers/dat_0cb903f8.inc"
};
static NJS_MKEY_A dat_0cb90418[2] = {
#include "ev006a/sonic_fingers/dat_0cb90418.inc"
};
static NJS_MKEY_A dat_0cb90438[2] = {
#include "ev006a/sonic_fingers/dat_0cb90438.inc"
};
static NJS_MKEY_A dat_0cb90458[2] = {
#include "ev006a/sonic_fingers/dat_0cb90458.inc"
};
static NJS_MDATA2 dat_0cb90478[73] = {
#include "ev006a/sonic_fingers/dat_0cb90478.inc"
};
static NJS_MOTION ev006a_sonic_motion_4 = {
#include "ev006a/sonic_fingers/ev006a_sonic_motion_4.inc"
};
NJS_ACTION ev006a_sonic_action_4 = {
#include "ev006a/sonic_fingers/ev006a_sonic_action_4.inc"
};
static NJS_MKEY_F dat_0cb9091c[15] = {
#include "ev006a/sonic_fingers/dat_0cb9091c.inc"
};
static NJS_MKEY_A dat_0cb90a0c[2] = {
#include "ev006a/sonic_fingers/dat_0cb90a0c.inc"
};
static NJS_MKEY_A dat_0cb90a2c[15] = {
#include "ev006a/sonic_fingers/dat_0cb90a2c.inc"
};
static NJS_MKEY_A dat_0cb90b1c[2] = {
#include "ev006a/sonic_fingers/dat_0cb90b1c.inc"
};
static NJS_MKEY_A dat_0cb90b3c[2] = {
#include "ev006a/sonic_fingers/dat_0cb90b3c.inc"
};
static NJS_MKEY_A dat_0cb90b5c[2] = {
#include "ev006a/sonic_fingers/dat_0cb90b5c.inc"
};
static NJS_MKEY_A dat_0cb90b7c[2] = {
#include "ev006a/sonic_fingers/dat_0cb90b7c.inc"
};
static NJS_MKEY_A dat_0cb90b9c[2] = {
#include "ev006a/sonic_fingers/dat_0cb90b9c.inc"
};
static NJS_MKEY_A dat_0cb90bbc[2] = {
#include "ev006a/sonic_fingers/dat_0cb90bbc.inc"
};
static NJS_MKEY_A dat_0cb90bdc[2] = {
#include "ev006a/sonic_fingers/dat_0cb90bdc.inc"
};
static NJS_MKEY_A dat_0cb90bfc[2] = {
#include "ev006a/sonic_fingers/dat_0cb90bfc.inc"
};
static NJS_MKEY_A dat_0cb90c1c[2] = {
#include "ev006a/sonic_fingers/dat_0cb90c1c.inc"
};
static NJS_MKEY_A dat_0cb90c3c[2] = {
#include "ev006a/sonic_fingers/dat_0cb90c3c.inc"
};
static NJS_MKEY_A dat_0cb90c5c[15] = {
#include "ev006a/sonic_fingers/dat_0cb90c5c.inc"
};
static NJS_MKEY_A dat_0cb90d4c[2] = {
#include "ev006a/sonic_fingers/dat_0cb90d4c.inc"
};
static NJS_MKEY_A dat_0cb90d6c[2] = {
#include "ev006a/sonic_fingers/dat_0cb90d6c.inc"
};
static NJS_MKEY_A dat_0cb90d8c[15] = {
#include "ev006a/sonic_fingers/dat_0cb90d8c.inc"
};
static NJS_MKEY_A dat_0cb90e7c[2] = {
#include "ev006a/sonic_fingers/dat_0cb90e7c.inc"
};
static NJS_MKEY_A dat_0cb90e9c[2] = {
#include "ev006a/sonic_fingers/dat_0cb90e9c.inc"
};
static NJS_MKEY_A dat_0cb90ebc[2] = {
#include "ev006a/sonic_fingers/dat_0cb90ebc.inc"
};
static NJS_MKEY_A dat_0cb90edc[15] = {
#include "ev006a/sonic_fingers/dat_0cb90edc.inc"
};
static NJS_MKEY_A dat_0cb90fcc[2] = {
#include "ev006a/sonic_fingers/dat_0cb90fcc.inc"
};
static NJS_MKEY_A dat_0cb90fec[2] = {
#include "ev006a/sonic_fingers/dat_0cb90fec.inc"
};
static NJS_MKEY_A dat_0cb9100c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9100c.inc"
};
static NJS_MKEY_A dat_0cb9102c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9102c.inc"
};
static NJS_MKEY_A dat_0cb9104c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9104c.inc"
};
static NJS_MKEY_A dat_0cb9106c[15] = {
#include "ev006a/sonic_fingers/dat_0cb9106c.inc"
};
static NJS_MKEY_A dat_0cb9115c[15] = {
#include "ev006a/sonic_fingers/dat_0cb9115c.inc"
};
static NJS_MKEY_A dat_0cb9124c[15] = {
#include "ev006a/sonic_fingers/dat_0cb9124c.inc"
};
static NJS_MKEY_A dat_0cb9133c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9133c.inc"
};
static NJS_MKEY_A dat_0cb9135c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9135c.inc"
};
static NJS_MKEY_A dat_0cb9137c[15] = {
#include "ev006a/sonic_fingers/dat_0cb9137c.inc"
};
static NJS_MKEY_A dat_0cb9146c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9146c.inc"
};
static NJS_MKEY_A dat_0cb9148c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9148c.inc"
};
static NJS_MKEY_A dat_0cb914ac[2] = {
#include "ev006a/sonic_fingers/dat_0cb914ac.inc"
};
static NJS_MKEY_A dat_0cb914cc[2] = {
#include "ev006a/sonic_fingers/dat_0cb914cc.inc"
};
static NJS_MKEY_A dat_0cb914ec[2] = {
#include "ev006a/sonic_fingers/dat_0cb914ec.inc"
};
static NJS_MKEY_A dat_0cb9150c[15] = {
#include "ev006a/sonic_fingers/dat_0cb9150c.inc"
};
static NJS_MKEY_A dat_0cb915fc[15] = {
#include "ev006a/sonic_fingers/dat_0cb915fc.inc"
};
static NJS_MKEY_A dat_0cb916ec[15] = {
#include "ev006a/sonic_fingers/dat_0cb916ec.inc"
};
static NJS_MKEY_A dat_0cb917dc[2] = {
#include "ev006a/sonic_fingers/dat_0cb917dc.inc"
};
static NJS_MKEY_A dat_0cb917fc[2] = {
#include "ev006a/sonic_fingers/dat_0cb917fc.inc"
};
static NJS_MKEY_A dat_0cb9181c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9181c.inc"
};
static NJS_MKEY_A dat_0cb9183c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9183c.inc"
};
static NJS_MKEY_A dat_0cb9185c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9185c.inc"
};
static NJS_MKEY_A dat_0cb9187c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9187c.inc"
};
static NJS_MKEY_A dat_0cb9189c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9189c.inc"
};
static NJS_MKEY_A dat_0cb918bc[2] = {
#include "ev006a/sonic_fingers/dat_0cb918bc.inc"
};
static NJS_MKEY_A dat_0cb918dc[15] = {
#include "ev006a/sonic_fingers/dat_0cb918dc.inc"
};
static NJS_MKEY_A dat_0cb919cc[15] = {
#include "ev006a/sonic_fingers/dat_0cb919cc.inc"
};
static NJS_MKEY_A dat_0cb91abc[15] = {
#include "ev006a/sonic_fingers/dat_0cb91abc.inc"
};
static NJS_MKEY_A dat_0cb91bac[2] = {
#include "ev006a/sonic_fingers/dat_0cb91bac.inc"
};
static NJS_MKEY_A dat_0cb91bcc[2] = {
#include "ev006a/sonic_fingers/dat_0cb91bcc.inc"
};
static NJS_MKEY_A dat_0cb91bec[15] = {
#include "ev006a/sonic_fingers/dat_0cb91bec.inc"
};
static NJS_MKEY_A dat_0cb91cdc[2] = {
#include "ev006a/sonic_fingers/dat_0cb91cdc.inc"
};
static NJS_MKEY_A dat_0cb91cfc[2] = {
#include "ev006a/sonic_fingers/dat_0cb91cfc.inc"
};
static NJS_MKEY_A dat_0cb91d1c[2] = {
#include "ev006a/sonic_fingers/dat_0cb91d1c.inc"
};
static NJS_MKEY_A dat_0cb91d3c[2] = {
#include "ev006a/sonic_fingers/dat_0cb91d3c.inc"
};
static NJS_MKEY_A dat_0cb91d5c[15] = {
#include "ev006a/sonic_fingers/dat_0cb91d5c.inc"
};
static NJS_MKEY_A dat_0cb91e4c[15] = {
#include "ev006a/sonic_fingers/dat_0cb91e4c.inc"
};
static NJS_MKEY_A dat_0cb91f3c[15] = {
#include "ev006a/sonic_fingers/dat_0cb91f3c.inc"
};
static NJS_MKEY_A dat_0cb9202c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9202c.inc"
};
static NJS_MKEY_A dat_0cb9204c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9204c.inc"
};
static NJS_MKEY_A dat_0cb9206c[15] = {
#include "ev006a/sonic_fingers/dat_0cb9206c.inc"
};
static NJS_MKEY_A dat_0cb9215c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9215c.inc"
};
static NJS_MKEY_A dat_0cb9217c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9217c.inc"
};
static NJS_MKEY_A dat_0cb9219c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9219c.inc"
};
static NJS_MKEY_A dat_0cb921bc[2] = {
#include "ev006a/sonic_fingers/dat_0cb921bc.inc"
};
static NJS_MKEY_A dat_0cb921dc[2] = {
#include "ev006a/sonic_fingers/dat_0cb921dc.inc"
};
static NJS_MKEY_A dat_0cb921fc[2] = {
#include "ev006a/sonic_fingers/dat_0cb921fc.inc"
};
static NJS_MKEY_A dat_0cb9221c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9221c.inc"
};
static NJS_MKEY_A dat_0cb9223c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9223c.inc"
};
static NJS_MKEY_A dat_0cb9225c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9225c.inc"
};
static NJS_MKEY_A dat_0cb9227c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9227c.inc"
};
static NJS_MDATA2 dat_0cb9229c[73] = {
#include "ev006a/sonic_fingers/dat_0cb9229c.inc"
};
static NJS_MOTION ev006a_sonic_motion_5 = {
#include "ev006a/sonic_fingers/ev006a_sonic_motion_5.inc"
};
NJS_ACTION ev006a_sonic_action_5 = {
#include "ev006a/sonic_fingers/ev006a_sonic_action_5.inc"
};
static NJS_MKEY_F dat_0cb92740[30] = {
#include "ev006a/sonic_fingers/dat_0cb92740.inc"
};
static NJS_MKEY_A dat_0cb92920[2] = {
#include "ev006a/sonic_fingers/dat_0cb92920.inc"
};
static NJS_MKEY_A dat_0cb92940[2] = {
#include "ev006a/sonic_fingers/dat_0cb92940.inc"
};
static NJS_MKEY_A dat_0cb92960[2] = {
#include "ev006a/sonic_fingers/dat_0cb92960.inc"
};
static NJS_MKEY_A dat_0cb92980[2] = {
#include "ev006a/sonic_fingers/dat_0cb92980.inc"
};
static NJS_MKEY_A dat_0cb929a0[2] = {
#include "ev006a/sonic_fingers/dat_0cb929a0.inc"
};
static NJS_MKEY_A dat_0cb929c0[2] = {
#include "ev006a/sonic_fingers/dat_0cb929c0.inc"
};
static NJS_MKEY_A dat_0cb929e0[30] = {
#include "ev006a/sonic_fingers/dat_0cb929e0.inc"
};
static NJS_MKEY_A dat_0cb92bc0[28] = {
#include "ev006a/sonic_fingers/dat_0cb92bc0.inc"
};
static NJS_MKEY_A dat_0cb92d80[29] = {
#include "ev006a/sonic_fingers/dat_0cb92d80.inc"
};
static NJS_MKEY_A dat_0cb92f50[2] = {
#include "ev006a/sonic_fingers/dat_0cb92f50.inc"
};
static NJS_MKEY_A dat_0cb92f70[2] = {
#include "ev006a/sonic_fingers/dat_0cb92f70.inc"
};
static NJS_MKEY_A dat_0cb92f90[2] = {
#include "ev006a/sonic_fingers/dat_0cb92f90.inc"
};
static NJS_MKEY_A dat_0cb92fb0[30] = {
#include "ev006a/sonic_fingers/dat_0cb92fb0.inc"
};
static NJS_MKEY_A dat_0cb93190[30] = {
#include "ev006a/sonic_fingers/dat_0cb93190.inc"
};
static NJS_MKEY_A dat_0cb93370[30] = {
#include "ev006a/sonic_fingers/dat_0cb93370.inc"
};
static NJS_MKEY_A dat_0cb93550[2] = {
#include "ev006a/sonic_fingers/dat_0cb93550.inc"
};
static NJS_MKEY_A dat_0cb93570[2] = {
#include "ev006a/sonic_fingers/dat_0cb93570.inc"
};
static NJS_MKEY_A dat_0cb93590[2] = {
#include "ev006a/sonic_fingers/dat_0cb93590.inc"
};
static NJS_MKEY_A dat_0cb935b0[2] = {
#include "ev006a/sonic_fingers/dat_0cb935b0.inc"
};
static NJS_MKEY_A dat_0cb935d0[30] = {
#include "ev006a/sonic_fingers/dat_0cb935d0.inc"
};
static NJS_MKEY_A dat_0cb937b0[30] = {
#include "ev006a/sonic_fingers/dat_0cb937b0.inc"
};
static NJS_MKEY_A dat_0cb93990[30] = {
#include "ev006a/sonic_fingers/dat_0cb93990.inc"
};
static NJS_MKEY_A dat_0cb93b70[30] = {
#include "ev006a/sonic_fingers/dat_0cb93b70.inc"
};
static NJS_MKEY_A dat_0cb93d50[2] = {
#include "ev006a/sonic_fingers/dat_0cb93d50.inc"
};
static NJS_MKEY_A dat_0cb93d70[30] = {
#include "ev006a/sonic_fingers/dat_0cb93d70.inc"
};
static NJS_MKEY_A dat_0cb93f50[30] = {
#include "ev006a/sonic_fingers/dat_0cb93f50.inc"
};
static NJS_MKEY_A dat_0cb94130[30] = {
#include "ev006a/sonic_fingers/dat_0cb94130.inc"
};
static NJS_MKEY_A dat_0cb94310[30] = {
#include "ev006a/sonic_fingers/dat_0cb94310.inc"
};
static NJS_MKEY_A dat_0cb944f0[2] = {
#include "ev006a/sonic_fingers/dat_0cb944f0.inc"
};
static NJS_MKEY_A dat_0cb94510[2] = {
#include "ev006a/sonic_fingers/dat_0cb94510.inc"
};
static NJS_MKEY_A dat_0cb94530[2] = {
#include "ev006a/sonic_fingers/dat_0cb94530.inc"
};
static NJS_MKEY_A dat_0cb94550[2] = {
#include "ev006a/sonic_fingers/dat_0cb94550.inc"
};
static NJS_MDATA2 dat_0cb94570[68] = {
#include "ev006a/sonic_fingers/dat_0cb94570.inc"
};
static NJS_MOTION ev006a_tails_motion_0 = {
#include "ev006a/sonic_fingers/ev006a_tails_motion_0.inc"
};
NJS_ACTION ev006a_tails_action_0 = {
#include "ev006a/sonic_fingers/ev006a_tails_action_0.inc"
};
static NJS_MKEY_F dat_0cb949c4[2] = {
#include "ev006a/sonic_fingers/dat_0cb949c4.inc"
};
static NJS_MKEY_A dat_0cb949e4[32] = {
#include "ev006a/sonic_fingers/dat_0cb949e4.inc"
};
static NJS_MKEY_A dat_0cb94be4[31] = {
#include "ev006a/sonic_fingers/dat_0cb94be4.inc"
};
static NJS_MKEY_A dat_0cb94dd4[2] = {
#include "ev006a/sonic_fingers/dat_0cb94dd4.inc"
};
static NJS_MKEY_A dat_0cb94df4[2] = {
#include "ev006a/sonic_fingers/dat_0cb94df4.inc"
};
static NJS_MKEY_A dat_0cb94e14[32] = {
#include "ev006a/sonic_fingers/dat_0cb94e14.inc"
};
static NJS_MKEY_A dat_0cb95014[25] = {
#include "ev006a/sonic_fingers/dat_0cb95014.inc"
};
static NJS_MKEY_A dat_0cb951a4[2] = {
#include "ev006a/sonic_fingers/dat_0cb951a4.inc"
};
static NJS_MKEY_A dat_0cb951c4[2] = {
#include "ev006a/sonic_fingers/dat_0cb951c4.inc"
};
static NJS_MKEY_A dat_0cb951e4[2] = {
#include "ev006a/sonic_fingers/dat_0cb951e4.inc"
};
static NJS_MKEY_A dat_0cb95204[32] = {
#include "ev006a/sonic_fingers/dat_0cb95204.inc"
};
static NJS_MKEY_A dat_0cb95404[23] = {
#include "ev006a/sonic_fingers/dat_0cb95404.inc"
};
static NJS_MKEY_A dat_0cb95574[2] = {
#include "ev006a/sonic_fingers/dat_0cb95574.inc"
};
static NJS_MKEY_A dat_0cb95594[2] = {
#include "ev006a/sonic_fingers/dat_0cb95594.inc"
};
static NJS_MKEY_A dat_0cb955b4[2] = {
#include "ev006a/sonic_fingers/dat_0cb955b4.inc"
};
static NJS_MKEY_A dat_0cb955d4[32] = {
#include "ev006a/sonic_fingers/dat_0cb955d4.inc"
};
static NJS_MKEY_A dat_0cb957d4[29] = {
#include "ev006a/sonic_fingers/dat_0cb957d4.inc"
};
static NJS_MKEY_A dat_0cb959a4[31] = {
#include "ev006a/sonic_fingers/dat_0cb959a4.inc"
};
static NJS_MKEY_A dat_0cb95b94[32] = {
#include "ev006a/sonic_fingers/dat_0cb95b94.inc"
};
static NJS_MKEY_A dat_0cb95d94[32] = {
#include "ev006a/sonic_fingers/dat_0cb95d94.inc"
};
static NJS_MKEY_A dat_0cb95f94[29] = {
#include "ev006a/sonic_fingers/dat_0cb95f94.inc"
};
static NJS_MKEY_A dat_0cb96164[31] = {
#include "ev006a/sonic_fingers/dat_0cb96164.inc"
};
static NJS_MKEY_A dat_0cb96354[32] = {
#include "ev006a/sonic_fingers/dat_0cb96354.inc"
};
static NJS_MDATA2 dat_0cb96554[68] = {
#include "ev006a/sonic_fingers/dat_0cb96554.inc"
};
static NJS_MOTION ev006a_tails_motion_1 = {
#include "ev006a/sonic_fingers/ev006a_tails_motion_1.inc"
};
NJS_ACTION ev006a_tails_action_1 = {
#include "ev006a/sonic_fingers/ev006a_tails_action_1.inc"
};
static NJS_MKEY_F dat_0cb969a8[2] = {
#include "ev006a/sonic_fingers/dat_0cb969a8.inc"
};
static NJS_MKEY_A dat_0cb969c8[25] = {
#include "ev006a/sonic_fingers/dat_0cb969c8.inc"
};
static NJS_MKEY_A dat_0cb96b58[25] = {
#include "ev006a/sonic_fingers/dat_0cb96b58.inc"
};
static NJS_MKEY_A dat_0cb96ce8[2] = {
#include "ev006a/sonic_fingers/dat_0cb96ce8.inc"
};
static NJS_MKEY_A dat_0cb96d08[24] = {
#include "ev006a/sonic_fingers/dat_0cb96d08.inc"
};
static NJS_MKEY_A dat_0cb96e88[2] = {
#include "ev006a/sonic_fingers/dat_0cb96e88.inc"
};
static NJS_MKEY_A dat_0cb96ea8[2] = {
#include "ev006a/sonic_fingers/dat_0cb96ea8.inc"
};
static NJS_MKEY_A dat_0cb96ec8[24] = {
#include "ev006a/sonic_fingers/dat_0cb96ec8.inc"
};
static NJS_MKEY_A dat_0cb97048[2] = {
#include "ev006a/sonic_fingers/dat_0cb97048.inc"
};
static NJS_MKEY_A dat_0cb97068[2] = {
#include "ev006a/sonic_fingers/dat_0cb97068.inc"
};
static NJS_MKEY_A dat_0cb97088[2] = {
#include "ev006a/sonic_fingers/dat_0cb97088.inc"
};
static NJS_MKEY_A dat_0cb970a8[25] = {
#include "ev006a/sonic_fingers/dat_0cb970a8.inc"
};
static NJS_MKEY_A dat_0cb97238[25] = {
#include "ev006a/sonic_fingers/dat_0cb97238.inc"
};
static NJS_MKEY_A dat_0cb973c8[11] = {
#include "ev006a/sonic_fingers/dat_0cb973c8.inc"
};
static NJS_MKEY_A dat_0cb97478[2] = {
#include "ev006a/sonic_fingers/dat_0cb97478.inc"
};
static NJS_MKEY_A dat_0cb97498[2] = {
#include "ev006a/sonic_fingers/dat_0cb97498.inc"
};
static NJS_MKEY_A dat_0cb974b8[2] = {
#include "ev006a/sonic_fingers/dat_0cb974b8.inc"
};
static NJS_MKEY_A dat_0cb974d8[2] = {
#include "ev006a/sonic_fingers/dat_0cb974d8.inc"
};
static NJS_MKEY_A dat_0cb974f8[25] = {
#include "ev006a/sonic_fingers/dat_0cb974f8.inc"
};
static NJS_MKEY_A dat_0cb97688[25] = {
#include "ev006a/sonic_fingers/dat_0cb97688.inc"
};
static NJS_MKEY_A dat_0cb97818[11] = {
#include "ev006a/sonic_fingers/dat_0cb97818.inc"
};
static NJS_MKEY_A dat_0cb978c8[2] = {
#include "ev006a/sonic_fingers/dat_0cb978c8.inc"
};
static NJS_MKEY_A dat_0cb978e8[2] = {
#include "ev006a/sonic_fingers/dat_0cb978e8.inc"
};
static NJS_MKEY_A dat_0cb97908[25] = {
#include "ev006a/sonic_fingers/dat_0cb97908.inc"
};
static NJS_MKEY_A dat_0cb97a98[5] = {
#include "ev006a/sonic_fingers/dat_0cb97a98.inc"
};
static NJS_MKEY_A dat_0cb97ae8[5] = {
#include "ev006a/sonic_fingers/dat_0cb97ae8.inc"
};
static NJS_MKEY_A dat_0cb97b38[25] = {
#include "ev006a/sonic_fingers/dat_0cb97b38.inc"
};
static NJS_MKEY_A dat_0cb97cc8[25] = {
#include "ev006a/sonic_fingers/dat_0cb97cc8.inc"
};
static NJS_MKEY_A dat_0cb97e58[5] = {
#include "ev006a/sonic_fingers/dat_0cb97e58.inc"
};
static NJS_MKEY_A dat_0cb97ea8[5] = {
#include "ev006a/sonic_fingers/dat_0cb97ea8.inc"
};
static NJS_MKEY_A dat_0cb97ef8[25] = {
#include "ev006a/sonic_fingers/dat_0cb97ef8.inc"
};
static NJS_MKEY_A dat_0cb98088[25] = {
#include "ev006a/sonic_fingers/dat_0cb98088.inc"
};
static NJS_MDATA2 dat_0cb98218[78] = {
#include "ev006a/sonic_fingers/dat_0cb98218.inc"
};
static NJS_MOTION ev006a_amy_motion_0 = {
#include "ev006a/sonic_fingers/ev006a_amy_motion_0.inc"
};
NJS_ACTION ev006a_amy_action_0 = {
#include "ev006a/sonic_fingers/ev006a_amy_action_0.inc"
};
static NJS_MKEY_F dat_0cb9870c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9870c.inc"
};
static NJS_MKEY_A dat_0cb9872c[32] = {
#include "ev006a/sonic_fingers/dat_0cb9872c.inc"
};
static NJS_MKEY_A dat_0cb9892c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9892c.inc"
};
static NJS_MKEY_A dat_0cb9894c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9894c.inc"
};
static NJS_MKEY_A dat_0cb9896c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9896c.inc"
};
static NJS_MKEY_A dat_0cb9898c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9898c.inc"
};
static NJS_MKEY_A dat_0cb989ac[2] = {
#include "ev006a/sonic_fingers/dat_0cb989ac.inc"
};
static NJS_MKEY_A dat_0cb989cc[2] = {
#include "ev006a/sonic_fingers/dat_0cb989cc.inc"
};
static NJS_MKEY_A dat_0cb989ec[2] = {
#include "ev006a/sonic_fingers/dat_0cb989ec.inc"
};
static NJS_MKEY_A dat_0cb98a0c[2] = {
#include "ev006a/sonic_fingers/dat_0cb98a0c.inc"
};
static NJS_MKEY_A dat_0cb98a2c[32] = {
#include "ev006a/sonic_fingers/dat_0cb98a2c.inc"
};
static NJS_MKEY_A dat_0cb98c2c[32] = {
#include "ev006a/sonic_fingers/dat_0cb98c2c.inc"
};
static NJS_MKEY_A dat_0cb98e2c[32] = {
#include "ev006a/sonic_fingers/dat_0cb98e2c.inc"
};
static NJS_MKEY_A dat_0cb9902c[32] = {
#include "ev006a/sonic_fingers/dat_0cb9902c.inc"
};
static NJS_MKEY_A dat_0cb9922c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9922c.inc"
};
static NJS_MKEY_A dat_0cb9924c[2] = {
#include "ev006a/sonic_fingers/dat_0cb9924c.inc"
};
static NJS_MKEY_A dat_0cb9926c[32] = {
#include "ev006a/sonic_fingers/dat_0cb9926c.inc"
};
static NJS_MKEY_A dat_0cb9946c[32] = {
#include "ev006a/sonic_fingers/dat_0cb9946c.inc"
};
static NJS_MKEY_A dat_0cb9966c[32] = {
#include "ev006a/sonic_fingers/dat_0cb9966c.inc"
};
static NJS_MKEY_A dat_0cb9986c[32] = {
#include "ev006a/sonic_fingers/dat_0cb9986c.inc"
};
static NJS_MKEY_A dat_0cb99a6c[2] = {
#include "ev006a/sonic_fingers/dat_0cb99a6c.inc"
};
static NJS_MKEY_A dat_0cb99a8c[17] = {
#include "ev006a/sonic_fingers/dat_0cb99a8c.inc"
};
static NJS_MKEY_A dat_0cb99b9c[32] = {
#include "ev006a/sonic_fingers/dat_0cb99b9c.inc"
};
static NJS_MKEY_A dat_0cb99d9c[25] = {
#include "ev006a/sonic_fingers/dat_0cb99d9c.inc"
};
static NJS_MKEY_A dat_0cb99f2c[25] = {
#include "ev006a/sonic_fingers/dat_0cb99f2c.inc"
};
static NJS_MKEY_A dat_0cb9a0bc[32] = {
#include "ev006a/sonic_fingers/dat_0cb9a0bc.inc"
};
static NJS_MKEY_A dat_0cb9a2bc[18] = {
#include "ev006a/sonic_fingers/dat_0cb9a2bc.inc"
};
static NJS_MKEY_A dat_0cb9a3dc[32] = {
#include "ev006a/sonic_fingers/dat_0cb9a3dc.inc"
};
static NJS_MKEY_A dat_0cb9a5dc[25] = {
#include "ev006a/sonic_fingers/dat_0cb9a5dc.inc"
};
static NJS_MKEY_A dat_0cb9a76c[25] = {
#include "ev006a/sonic_fingers/dat_0cb9a76c.inc"
};
static NJS_MKEY_A dat_0cb9a8fc[32] = {
#include "ev006a/sonic_fingers/dat_0cb9a8fc.inc"
};
static NJS_MKEY_A dat_0cb9aafc[32] = {
#include "ev006a/sonic_fingers/dat_0cb9aafc.inc"
};
static NJS_MDATA2 dat_0cb9acfc[78] = {
#include "ev006a/sonic_fingers/dat_0cb9acfc.inc"
};
static NJS_MOTION ev006a_amy_motion_1 = {
#include "ev006a/sonic_fingers/ev006a_amy_motion_1.inc"
};
NJS_ACTION ev006a_amy_action_1 = {
#include "ev006a/sonic_fingers/ev006a_amy_action_1.inc"
};
static NJS_MKEY_F dat_0cb9b1f0[32] = {
#include "ev006a/sonic_fingers/dat_0cb9b1f0.inc"
};
static NJS_MKEY_A dat_0cb9b3f0[2] = {
#include "ev006a/sonic_fingers/dat_0cb9b3f0.inc"
};
static NJS_MKEY_A dat_0cb9b410[32] = {
#include "ev006a/sonic_fingers/dat_0cb9b410.inc"
};
static NJS_MKEY_A dat_0cb9b610[32] = {
#include "ev006a/sonic_fingers/dat_0cb9b610.inc"
};
static NJS_MKEY_A dat_0cb9b810[2] = {
#include "ev006a/sonic_fingers/dat_0cb9b810.inc"
};
static NJS_MKEY_A dat_0cb9b830[2] = {
#include "ev006a/sonic_fingers/dat_0cb9b830.inc"
};
static NJS_MKEY_A dat_0cb9b850[2] = {
#include "ev006a/sonic_fingers/dat_0cb9b850.inc"
};
static NJS_MKEY_A dat_0cb9b870[2] = {
#include "ev006a/sonic_fingers/dat_0cb9b870.inc"
};
static NJS_MKEY_A dat_0cb9b890[2] = {
#include "ev006a/sonic_fingers/dat_0cb9b890.inc"
};
static NJS_MKEY_A dat_0cb9b8b0[2] = {
#include "ev006a/sonic_fingers/dat_0cb9b8b0.inc"
};
static NJS_MKEY_A dat_0cb9b8d0[2] = {
#include "ev006a/sonic_fingers/dat_0cb9b8d0.inc"
};
static NJS_MKEY_A dat_0cb9b8f0[31] = {
#include "ev006a/sonic_fingers/dat_0cb9b8f0.inc"
};
static NJS_MKEY_A dat_0cb9bae0[31] = {
#include "ev006a/sonic_fingers/dat_0cb9bae0.inc"
};
static NJS_MKEY_A dat_0cb9bcd0[31] = {
#include "ev006a/sonic_fingers/dat_0cb9bcd0.inc"
};
static NJS_MKEY_A dat_0cb9bec0[2] = {
#include "ev006a/sonic_fingers/dat_0cb9bec0.inc"
};
static NJS_MKEY_A dat_0cb9bee0[2] = {
#include "ev006a/sonic_fingers/dat_0cb9bee0.inc"
};
static NJS_MKEY_A dat_0cb9bf00[2] = {
#include "ev006a/sonic_fingers/dat_0cb9bf00.inc"
};
static NJS_MKEY_A dat_0cb9bf20[2] = {
#include "ev006a/sonic_fingers/dat_0cb9bf20.inc"
};
static NJS_MKEY_A dat_0cb9bf40[31] = {
#include "ev006a/sonic_fingers/dat_0cb9bf40.inc"
};
static NJS_MKEY_A dat_0cb9c130[32] = {
#include "ev006a/sonic_fingers/dat_0cb9c130.inc"
};
static NJS_MKEY_A dat_0cb9c330[25] = {
#include "ev006a/sonic_fingers/dat_0cb9c330.inc"
};
static NJS_MKEY_A dat_0cb9c4c0[2] = {
#include "ev006a/sonic_fingers/dat_0cb9c4c0.inc"
};
static NJS_MKEY_A dat_0cb9c4e0[2] = {
#include "ev006a/sonic_fingers/dat_0cb9c4e0.inc"
};
static NJS_MKEY_A dat_0cb9c500[29] = {
#include "ev006a/sonic_fingers/dat_0cb9c500.inc"
};
static NJS_MKEY_A dat_0cb9c6d0[29] = {
#include "ev006a/sonic_fingers/dat_0cb9c6d0.inc"
};
static NJS_MKEY_A dat_0cb9c8a0[29] = {
#include "ev006a/sonic_fingers/dat_0cb9c8a0.inc"
};
static NJS_MKEY_A dat_0cb9ca70[31] = {
#include "ev006a/sonic_fingers/dat_0cb9ca70.inc"
};
static NJS_MKEY_A dat_0cb9cc60[2] = {
#include "ev006a/sonic_fingers/dat_0cb9cc60.inc"
};
static NJS_MKEY_A dat_0cb9cc80[31] = {
#include "ev006a/sonic_fingers/dat_0cb9cc80.inc"
};
static NJS_MKEY_A dat_0cb9ce70[17] = {
#include "ev006a/sonic_fingers/dat_0cb9ce70.inc"
};
static NJS_MKEY_A dat_0cb9cf80[26] = {
#include "ev006a/sonic_fingers/dat_0cb9cf80.inc"
};
static NJS_MKEY_A dat_0cb9d120[31] = {
#include "ev006a/sonic_fingers/dat_0cb9d120.inc"
};
static NJS_MKEY_A dat_0cb9d310[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d310.inc"
};
static NJS_MDATA2 dat_0cb9d330[78] = {
#include "ev006a/sonic_fingers/dat_0cb9d330.inc"
};
static NJS_MOTION ev006a_amy_motion_2 = {
#include "ev006a/sonic_fingers/ev006a_amy_motion_2.inc"
};
NJS_ACTION ev006a_amy_action_2 = {
#include "ev006a/sonic_fingers/ev006a_amy_action_2.inc"
};
static NJS_MKEY_F dat_0cb9d824[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d824.inc"
};
static NJS_MKEY_A dat_0cb9d844[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d844.inc"
};
static NJS_MKEY_A dat_0cb9d864[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d864.inc"
};
static NJS_MKEY_A dat_0cb9d884[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d884.inc"
};
static NJS_MKEY_A dat_0cb9d8a4[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d8a4.inc"
};
static NJS_MKEY_A dat_0cb9d8c4[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d8c4.inc"
};
static NJS_MKEY_A dat_0cb9d8e4[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d8e4.inc"
};
static NJS_MKEY_A dat_0cb9d904[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d904.inc"
};
static NJS_MKEY_A dat_0cb9d924[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d924.inc"
};
static NJS_MKEY_A dat_0cb9d944[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d944.inc"
};
static NJS_MKEY_A dat_0cb9d964[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d964.inc"
};
static NJS_MKEY_A dat_0cb9d984[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d984.inc"
};
static NJS_MKEY_A dat_0cb9d9a4[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d9a4.inc"
};
static NJS_MKEY_A dat_0cb9d9c4[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d9c4.inc"
};
static NJS_MKEY_A dat_0cb9d9e4[2] = {
#include "ev006a/sonic_fingers/dat_0cb9d9e4.inc"
};
static NJS_MKEY_A dat_0cb9da04[2] = {
#include "ev006a/sonic_fingers/dat_0cb9da04.inc"
};
static NJS_MKEY_A dat_0cb9da24[2] = {
#include "ev006a/sonic_fingers/dat_0cb9da24.inc"
};
static NJS_MKEY_A dat_0cb9da44[2] = {
#include "ev006a/sonic_fingers/dat_0cb9da44.inc"
};
static NJS_MKEY_A dat_0cb9da64[2] = {
#include "ev006a/sonic_fingers/dat_0cb9da64.inc"
};
static NJS_MKEY_A dat_0cb9da84[2] = {
#include "ev006a/sonic_fingers/dat_0cb9da84.inc"
};
static NJS_MKEY_A dat_0cb9daa4[2] = {
#include "ev006a/sonic_fingers/dat_0cb9daa4.inc"
};
static NJS_MKEY_A dat_0cb9dac4[2] = {
#include "ev006a/sonic_fingers/dat_0cb9dac4.inc"
};
static NJS_MKEY_A dat_0cb9dae4[2] = {
#include "ev006a/sonic_fingers/dat_0cb9dae4.inc"
};
static NJS_MKEY_A dat_0cb9db04[2] = {
#include "ev006a/sonic_fingers/dat_0cb9db04.inc"
};
static NJS_MKEY_A dat_0cb9db24[2] = {
#include "ev006a/sonic_fingers/dat_0cb9db24.inc"
};
static NJS_MKEY_A dat_0cb9db44[2] = {
#include "ev006a/sonic_fingers/dat_0cb9db44.inc"
};
static NJS_MKEY_A dat_0cb9db64[2] = {
#include "ev006a/sonic_fingers/dat_0cb9db64.inc"
};
static NJS_MDATA2 dat_0cb9db84[78] = {
#include "ev006a/sonic_fingers/dat_0cb9db84.inc"
};
static NJS_MOTION ev006a_amy_motion_3 = {
#include "ev006a/sonic_fingers/ev006a_amy_motion_3.inc"
};
NJS_ACTION ev006a_amy_action_3 = {
#include "ev006a/sonic_fingers/ev006a_amy_action_3.inc"
};
static NJS_MATERIAL dat_0cb9e078[1] = {
#include "ev006a/sonic_fingers/dat_0cb9e078.inc"
};
static Sint16 dat_0cb9e08c[6] = {
#include "ev006a/sonic_fingers/dat_0cb9e08c.inc"
};
static NJS_COLOR dat_0cb9e098[4] = {
#include "ev006a/sonic_fingers/dat_0cb9e098.inc"
};
static NJS_MESHSET dat_0cb9e0a8[1] = {
#include "ev006a/sonic_fingers/dat_0cb9e0a8.inc"
};
static NJS_POINT3 dat_0cb9e0c0[4] = {
#include "ev006a/sonic_fingers/dat_0cb9e0c0.inc"
};
static NJS_VECTOR dat_0cb9e0f0[4] = {
#include "ev006a/sonic_fingers/dat_0cb9e0f0.inc"
};
static NJS_MODEL dat_0cb9e120 = {
#include "ev006a/sonic_fingers/dat_0cb9e120.inc"
};
static NJS_OBJECT dat_0cb9e148 = {
#include "ev006a/sonic_fingers/dat_0cb9e148.inc"
};
static NJS_MATERIAL dat_0cb9e17c[1] = {
#include "ev006a/sonic_fingers/dat_0cb9e17c.inc"
};
static Sint16 dat_0cb9e190[12] = {
#include "ev006a/sonic_fingers/dat_0cb9e190.inc"
};
static NJS_COLOR dat_0cb9e1a8[9] = {
#include "ev006a/sonic_fingers/dat_0cb9e1a8.inc"
};
static NJS_MESHSET dat_0cb9e1cc[1] = {
#include "ev006a/sonic_fingers/dat_0cb9e1cc.inc"
};
static NJS_POINT3 dat_0cb9e1e4[7] = {
#include "ev006a/sonic_fingers/dat_0cb9e1e4.inc"
};
static NJS_VECTOR dat_0cb9e238[7] = {
#include "ev006a/sonic_fingers/dat_0cb9e238.inc"
};
static NJS_MODEL dat_0cb9e28c = {
#include "ev006a/sonic_fingers/dat_0cb9e28c.inc"
};
static NJS_OBJECT dat_0cb9e2b4 = {
#include "ev006a/sonic_fingers/dat_0cb9e2b4.inc"
};
static NJS_OBJECT dat_0cb9e2e8 = {
#include "ev006a/sonic_fingers/dat_0cb9e2e8.inc"
};
static NJS_OBJECT dat_0cb9e31c = {
#include "ev006a/sonic_fingers/dat_0cb9e31c.inc"
};
static NJS_OBJECT dat_0cb9e350 = {
#include "ev006a/sonic_fingers/dat_0cb9e350.inc"
};
static NJS_OBJECT dat_0cb9e384 = {
#include "ev006a/sonic_fingers/dat_0cb9e384.inc"
};
static NJS_OBJECT dat_0cb9e3b8 = {
#include "ev006a/sonic_fingers/dat_0cb9e3b8.inc"
};
static NJS_OBJECT dat_0cb9e3ec = {
#include "ev006a/sonic_fingers/dat_0cb9e3ec.inc"
};
static NJS_MATERIAL dat_0cb9e420[1] = {
#include "ev006a/sonic_fingers/dat_0cb9e420.inc"
};
static Sint16 dat_0cb9e434[6] = {
#include "ev006a/sonic_fingers/dat_0cb9e434.inc"
};
static NJS_COLOR dat_0cb9e440[4] = {
#include "ev006a/sonic_fingers/dat_0cb9e440.inc"
};
static NJS_MESHSET dat_0cb9e450[1] = {
#include "ev006a/sonic_fingers/dat_0cb9e450.inc"
};
static NJS_POINT3 dat_0cb9e468[4] = {
#include "ev006a/sonic_fingers/dat_0cb9e468.inc"
};
static NJS_VECTOR dat_0cb9e498[4] = {
#include "ev006a/sonic_fingers/dat_0cb9e498.inc"
};
static NJS_MODEL dat_0cb9e4c8 = {
#include "ev006a/sonic_fingers/dat_0cb9e4c8.inc"
};
static NJS_OBJECT dat_0cb9e4f0 = {
#include "ev006a/sonic_fingers/dat_0cb9e4f0.inc"
};
static NJS_MATERIAL dat_0cb9e524[1] = {
#include "ev006a/sonic_fingers/dat_0cb9e524.inc"
};
static Sint16 dat_0cb9e538[12] = {
#include "ev006a/sonic_fingers/dat_0cb9e538.inc"
};
static NJS_COLOR dat_0cb9e550[9] = {
#include "ev006a/sonic_fingers/dat_0cb9e550.inc"
};
static NJS_MESHSET dat_0cb9e574[1] = {
#include "ev006a/sonic_fingers/dat_0cb9e574.inc"
};
static NJS_POINT3 dat_0cb9e58c[7] = {
#include "ev006a/sonic_fingers/dat_0cb9e58c.inc"
};
static NJS_VECTOR dat_0cb9e5e0[7] = {
#include "ev006a/sonic_fingers/dat_0cb9e5e0.inc"
};
static NJS_MODEL dat_0cb9e634 = {
#include "ev006a/sonic_fingers/dat_0cb9e634.inc"
};
static NJS_OBJECT dat_0cb9e65c = {
#include "ev006a/sonic_fingers/dat_0cb9e65c.inc"
};
static NJS_OBJECT dat_0cb9e690 = {
#include "ev006a/sonic_fingers/dat_0cb9e690.inc"
};
static NJS_OBJECT dat_0cb9e6c4 = {
#include "ev006a/sonic_fingers/dat_0cb9e6c4.inc"
};
static NJS_OBJECT dat_0cb9e6f8 = {
#include "ev006a/sonic_fingers/dat_0cb9e6f8.inc"
};
static NJS_OBJECT dat_0cb9e72c = {
#include "ev006a/sonic_fingers/dat_0cb9e72c.inc"
};
static NJS_OBJECT dat_0cb9e760 = {
#include "ev006a/sonic_fingers/dat_0cb9e760.inc"
};
static NJS_OBJECT dat_0cb9e794 = {
#include "ev006a/sonic_fingers/dat_0cb9e794.inc"
};
static NJS_OBJECT dat_0cb9e7c8 = {
#include "ev006a/sonic_fingers/dat_0cb9e7c8.inc"
};
static NJS_OBJECT dat_0cb9e7fc = {
#include "ev006a/sonic_fingers/dat_0cb9e7fc.inc"
};
static NJS_OBJECT dat_0cb9e830 = {
#include "ev006a/sonic_fingers/dat_0cb9e830.inc"
};
static NJS_OBJECT dat_0cb9e864 = {
#include "ev006a/sonic_fingers/dat_0cb9e864.inc"
};
static NJS_OBJECT dat_0cb9e898 = {
#include "ev006a/sonic_fingers/dat_0cb9e898.inc"
};
static NJS_MATERIAL dat_0cb9e8cc[1] = {
#include "ev006a/sonic_fingers/dat_0cb9e8cc.inc"
};
static Sint16 dat_0cb9e8e0[18] = {
#include "ev006a/sonic_fingers/dat_0cb9e8e0.inc"
};
static NJS_COLOR dat_0cb9e904[15] = {
#include "ev006a/sonic_fingers/dat_0cb9e904.inc"
};
static NJS_MESHSET dat_0cb9e940[1] = {
#include "ev006a/sonic_fingers/dat_0cb9e940.inc"
};
static NJS_POINT3 dat_0cb9e958[7] = {
#include "ev006a/sonic_fingers/dat_0cb9e958.inc"
};
static NJS_VECTOR dat_0cb9e9ac[7] = {
#include "ev006a/sonic_fingers/dat_0cb9e9ac.inc"
};
static NJS_MODEL dat_0cb9ea00 = {
#include "ev006a/sonic_fingers/dat_0cb9ea00.inc"
};
static NJS_OBJECT dat_0cb9ea28 = {
#include "ev006a/sonic_fingers/dat_0cb9ea28.inc"
};
static NJS_OBJECT dat_0cb9ea5c = {
#include "ev006a/sonic_fingers/dat_0cb9ea5c.inc"
};
static NJS_OBJECT dat_0cb9ea90 = {
#include "ev006a/sonic_fingers/dat_0cb9ea90.inc"
};
static NJS_OBJECT dat_0cb9eac4 = {
#include "ev006a/sonic_fingers/dat_0cb9eac4.inc"
};
static NJS_MATERIAL dat_0cb9eaf8[1] = {
#include "ev006a/sonic_fingers/dat_0cb9eaf8.inc"
};
static Sint16 dat_0cb9eb0c[10] = {
#include "ev006a/sonic_fingers/dat_0cb9eb0c.inc"
};
static NJS_COLOR dat_0cb9eb20[8] = {
#include "ev006a/sonic_fingers/dat_0cb9eb20.inc"
};
static NJS_MESHSET dat_0cb9eb40[1] = {
#include "ev006a/sonic_fingers/dat_0cb9eb40.inc"
};
static NJS_POINT3 dat_0cb9eb58[8] = {
#include "ev006a/sonic_fingers/dat_0cb9eb58.inc"
};
static NJS_VECTOR dat_0cb9ebb8[8] = {
#include "ev006a/sonic_fingers/dat_0cb9ebb8.inc"
};
static NJS_MODEL dat_0cb9ec18 = {
#include "ev006a/sonic_fingers/dat_0cb9ec18.inc"
};
static NJS_OBJECT dat_0cb9ec40 = {
#include "ev006a/sonic_fingers/dat_0cb9ec40.inc"
};
static NJS_MATERIAL dat_0cb9ec74[1] = {
#include "ev006a/sonic_fingers/dat_0cb9ec74.inc"
};
static Sint16 dat_0cb9ec88[10] = {
#include "ev006a/sonic_fingers/dat_0cb9ec88.inc"
};
static NJS_COLOR dat_0cb9ec9c[8] = {
#include "ev006a/sonic_fingers/dat_0cb9ec9c.inc"
};
static NJS_MESHSET dat_0cb9ecbc[1] = {
#include "ev006a/sonic_fingers/dat_0cb9ecbc.inc"
};
static NJS_POINT3 dat_0cb9ecd4[8] = {
#include "ev006a/sonic_fingers/dat_0cb9ecd4.inc"
};
static NJS_VECTOR dat_0cb9ed34[8] = {
#include "ev006a/sonic_fingers/dat_0cb9ed34.inc"
};
static NJS_MODEL dat_0cb9ed94 = {
#include "ev006a/sonic_fingers/dat_0cb9ed94.inc"
};
static NJS_OBJECT dat_0cb9edbc = {
#include "ev006a/sonic_fingers/dat_0cb9edbc.inc"
};
static NJS_MATERIAL dat_0cb9edf0[1] = {
#include "ev006a/sonic_fingers/dat_0cb9edf0.inc"
};
static Sint16 dat_0cb9ee04[16] = {
#include "ev006a/sonic_fingers/dat_0cb9ee04.inc"
};
static NJS_COLOR dat_0cb9ee24[12] = {
#include "ev006a/sonic_fingers/dat_0cb9ee24.inc"
};
static NJS_MESHSET dat_0cb9ee54[1] = {
#include "ev006a/sonic_fingers/dat_0cb9ee54.inc"
};
static NJS_POINT3 dat_0cb9ee6c[12] = {
#include "ev006a/sonic_fingers/dat_0cb9ee6c.inc"
};
static NJS_VECTOR dat_0cb9eefc[12] = {
#include "ev006a/sonic_fingers/dat_0cb9eefc.inc"
};
static NJS_MODEL dat_0cb9ef8c = {
#include "ev006a/sonic_fingers/dat_0cb9ef8c.inc"
};
static NJS_OBJECT dat_0cb9efb4 = {
#include "ev006a/sonic_fingers/dat_0cb9efb4.inc"
};
static NJS_MATERIAL dat_0cb9efe8[1] = {
#include "ev006a/sonic_fingers/dat_0cb9efe8.inc"
};
static Sint16 dat_0cb9effc[16] = {
#include "ev006a/sonic_fingers/dat_0cb9effc.inc"
};
static NJS_COLOR dat_0cb9f01c[12] = {
#include "ev006a/sonic_fingers/dat_0cb9f01c.inc"
};
static NJS_MESHSET dat_0cb9f04c[1] = {
#include "ev006a/sonic_fingers/dat_0cb9f04c.inc"
};
static NJS_POINT3 dat_0cb9f064[12] = {
#include "ev006a/sonic_fingers/dat_0cb9f064.inc"
};
static NJS_VECTOR dat_0cb9f0f4[12] = {
#include "ev006a/sonic_fingers/dat_0cb9f0f4.inc"
};
static NJS_MODEL dat_0cb9f184 = {
#include "ev006a/sonic_fingers/dat_0cb9f184.inc"
};
static NJS_OBJECT dat_0cb9f1ac = {
#include "ev006a/sonic_fingers/dat_0cb9f1ac.inc"
};
static NJS_MATERIAL dat_0cb9f1e0[4] = {
#include "ev006a/sonic_fingers/dat_0cb9f1e0.inc"
};
static Sint16 dat_0cb9f230[239] = {
#include "ev006a/sonic_fingers/dat_0cb9f230.inc"
};
static Sint16 dat_0cb9f40e[22] = {
#include "ev006a/sonic_fingers/dat_0cb9f40e.inc"
};
static Sint16 dat_0cb9f43a[21] = {
#include "ev006a/sonic_fingers/dat_0cb9f43a.inc"
};
static Sint16 dat_0cb9f464[22] = {
#include "ev006a/sonic_fingers/dat_0cb9f464.inc"
};
static NJS_COLOR dat_0cb9f490[209] = {
#include "ev006a/sonic_fingers/dat_0cb9f490.inc"
};
static NJS_COLOR dat_0cb9f7d4[20] = {
#include "ev006a/sonic_fingers/dat_0cb9f7d4.inc"
};
static NJS_COLOR dat_0cb9f824[16] = {
#include "ev006a/sonic_fingers/dat_0cb9f824.inc"
};
static NJS_COLOR dat_0cb9f864[18] = {
#include "ev006a/sonic_fingers/dat_0cb9f864.inc"
};
static NJS_MESHSET dat_0cb9f8ac[4] = {
#include "ev006a/sonic_fingers/dat_0cb9f8ac.inc"
};
static NJS_POINT3 dat_0cb9f90c[127] = {
#include "ev006a/sonic_fingers/dat_0cb9f90c.inc"
};
static NJS_VECTOR dat_0cb9ff00[127] = {
#include "ev006a/sonic_fingers/dat_0cb9ff00.inc"
};
static NJS_MODEL dat_0cba04f4 = {
#include "ev006a/sonic_fingers/dat_0cba04f4.inc"
};
static NJS_OBJECT dat_0cba051c = {
#include "ev006a/sonic_fingers/dat_0cba051c.inc"
};
static NJS_OBJECT dat_0cba0550 = {
#include "ev006a/sonic_fingers/dat_0cba0550.inc"
};
static NJS_OBJECT dat_0cba0584 = {
#include "ev006a/sonic_fingers/dat_0cba0584.inc"
};
static NJS_OBJECT dat_0cba05b8 = {
#include "ev006a/sonic_fingers/dat_0cba05b8.inc"
};
static NJS_MATERIAL dat_0cba05ec[1] = {
#include "ev006a/sonic_fingers/dat_0cba05ec.inc"
};
static Sint16 dat_0cba0600[8] = {
#include "ev006a/sonic_fingers/dat_0cba0600.inc"
};
static NJS_COLOR dat_0cba0610[6] = {
#include "ev006a/sonic_fingers/dat_0cba0610.inc"
};
static NJS_MESHSET dat_0cba0628[1] = {
#include "ev006a/sonic_fingers/dat_0cba0628.inc"
};
static NJS_POINT3 dat_0cba0640[6] = {
#include "ev006a/sonic_fingers/dat_0cba0640.inc"
};
static NJS_VECTOR dat_0cba0688[6] = {
#include "ev006a/sonic_fingers/dat_0cba0688.inc"
};
static NJS_MODEL dat_0cba06d0 = {
#include "ev006a/sonic_fingers/dat_0cba06d0.inc"
};
static NJS_OBJECT dat_0cba06f8 = {
#include "ev006a/sonic_fingers/dat_0cba06f8.inc"
};
static NJS_MATERIAL dat_0cba072c[1] = {
#include "ev006a/sonic_fingers/dat_0cba072c.inc"
};
static Sint16 dat_0cba0740[22] = {
#include "ev006a/sonic_fingers/dat_0cba0740.inc"
};
static NJS_COLOR dat_0cba076c[16] = {
#include "ev006a/sonic_fingers/dat_0cba076c.inc"
};
static NJS_MESHSET dat_0cba07ac[1] = {
#include "ev006a/sonic_fingers/dat_0cba07ac.inc"
};
static NJS_POINT3 dat_0cba07c4[5] = {
#include "ev006a/sonic_fingers/dat_0cba07c4.inc"
};
static NJS_VECTOR dat_0cba0800[5] = {
#include "ev006a/sonic_fingers/dat_0cba0800.inc"
};
static NJS_MODEL dat_0cba083c = {
#include "ev006a/sonic_fingers/dat_0cba083c.inc"
};
static NJS_OBJECT dat_0cba0864 = {
#include "ev006a/sonic_fingers/dat_0cba0864.inc"
};
static NJS_MATERIAL dat_0cba0898[1] = {
#include "ev006a/sonic_fingers/dat_0cba0898.inc"
};
static Sint16 dat_0cba08ac[30] = {
#include "ev006a/sonic_fingers/dat_0cba08ac.inc"
};
static NJS_COLOR dat_0cba08e8[28] = {
#include "ev006a/sonic_fingers/dat_0cba08e8.inc"
};
static NJS_MESHSET dat_0cba0958[1] = {
#include "ev006a/sonic_fingers/dat_0cba0958.inc"
};
static NJS_POINT3 dat_0cba0970[18] = {
#include "ev006a/sonic_fingers/dat_0cba0970.inc"
};
static NJS_VECTOR dat_0cba0a48[18] = {
#include "ev006a/sonic_fingers/dat_0cba0a48.inc"
};
static NJS_MODEL dat_0cba0b20 = {
#include "ev006a/sonic_fingers/dat_0cba0b20.inc"
};
static NJS_OBJECT dat_0cba0b48 = {
#include "ev006a/sonic_fingers/dat_0cba0b48.inc"
};
static NJS_MATERIAL dat_0cba0b7c[1] = {
#include "ev006a/sonic_fingers/dat_0cba0b7c.inc"
};
static Sint16 dat_0cba0b90[20] = {
#include "ev006a/sonic_fingers/dat_0cba0b90.inc"
};
static NJS_COLOR dat_0cba0bb8[16] = {
#include "ev006a/sonic_fingers/dat_0cba0bb8.inc"
};
static NJS_MESHSET dat_0cba0bf8[1] = {
#include "ev006a/sonic_fingers/dat_0cba0bf8.inc"
};
static NJS_POINT3 dat_0cba0c10[12] = {
#include "ev006a/sonic_fingers/dat_0cba0c10.inc"
};
static NJS_VECTOR dat_0cba0ca0[12] = {
#include "ev006a/sonic_fingers/dat_0cba0ca0.inc"
};
static NJS_MODEL dat_0cba0d30 = {
#include "ev006a/sonic_fingers/dat_0cba0d30.inc"
};
static NJS_OBJECT dat_0cba0d58 = {
#include "ev006a/sonic_fingers/dat_0cba0d58.inc"
};
static NJS_MATERIAL dat_0cba0d8c[1] = {
#include "ev006a/sonic_fingers/dat_0cba0d8c.inc"
};
static Sint16 dat_0cba0da0[10] = {
#include "ev006a/sonic_fingers/dat_0cba0da0.inc"
};
static NJS_COLOR dat_0cba0db4[8] = {
#include "ev006a/sonic_fingers/dat_0cba0db4.inc"
};
static NJS_MESHSET dat_0cba0dd4[1] = {
#include "ev006a/sonic_fingers/dat_0cba0dd4.inc"
};
static NJS_POINT3 dat_0cba0dec[5] = {
#include "ev006a/sonic_fingers/dat_0cba0dec.inc"
};
static NJS_VECTOR dat_0cba0e28[5] = {
#include "ev006a/sonic_fingers/dat_0cba0e28.inc"
};
static NJS_MODEL dat_0cba0e64 = {
#include "ev006a/sonic_fingers/dat_0cba0e64.inc"
};
static NJS_OBJECT dat_0cba0e8c = {
#include "ev006a/sonic_fingers/dat_0cba0e8c.inc"
};
static NJS_MATERIAL dat_0cba0ec0[1] = {
#include "ev006a/sonic_fingers/dat_0cba0ec0.inc"
};
static Sint16 dat_0cba0ed4[10] = {
#include "ev006a/sonic_fingers/dat_0cba0ed4.inc"
};
static NJS_COLOR dat_0cba0ee8[8] = {
#include "ev006a/sonic_fingers/dat_0cba0ee8.inc"
};
static NJS_MESHSET dat_0cba0f08[1] = {
#include "ev006a/sonic_fingers/dat_0cba0f08.inc"
};
static NJS_POINT3 dat_0cba0f20[5] = {
#include "ev006a/sonic_fingers/dat_0cba0f20.inc"
};
static NJS_VECTOR dat_0cba0f5c[5] = {
#include "ev006a/sonic_fingers/dat_0cba0f5c.inc"
};
static NJS_MODEL dat_0cba0f98 = {
#include "ev006a/sonic_fingers/dat_0cba0f98.inc"
};
static NJS_OBJECT dat_0cba0fc0 = {
#include "ev006a/sonic_fingers/dat_0cba0fc0.inc"
};
static NJS_MATERIAL dat_0cba0ff4[1] = {
#include "ev006a/sonic_fingers/dat_0cba0ff4.inc"
};
static Sint16 dat_0cba1008[24] = {
#include "ev006a/sonic_fingers/dat_0cba1008.inc"
};
static NJS_COLOR dat_0cba1038[20] = {
#include "ev006a/sonic_fingers/dat_0cba1038.inc"
};
static NJS_MESHSET dat_0cba1088[1] = {
#include "ev006a/sonic_fingers/dat_0cba1088.inc"
};
static NJS_POINT3 dat_0cba10a0[12] = {
#include "ev006a/sonic_fingers/dat_0cba10a0.inc"
};
static NJS_VECTOR dat_0cba1130[12] = {
#include "ev006a/sonic_fingers/dat_0cba1130.inc"
};
static NJS_MODEL dat_0cba11c0 = {
#include "ev006a/sonic_fingers/dat_0cba11c0.inc"
};
static NJS_OBJECT dat_0cba11e8 = {
#include "ev006a/sonic_fingers/dat_0cba11e8.inc"
};
static NJS_MATERIAL dat_0cba121c[1] = {
#include "ev006a/sonic_fingers/dat_0cba121c.inc"
};
static Sint16 dat_0cba1230[36] = {
#include "ev006a/sonic_fingers/dat_0cba1230.inc"
};
static NJS_COLOR dat_0cba1278[30] = {
#include "ev006a/sonic_fingers/dat_0cba1278.inc"
};
static NJS_MESHSET dat_0cba12f0[1] = {
#include "ev006a/sonic_fingers/dat_0cba12f0.inc"
};
static NJS_POINT3 dat_0cba1308[12] = {
#include "ev006a/sonic_fingers/dat_0cba1308.inc"
};
static NJS_VECTOR dat_0cba1398[12] = {
#include "ev006a/sonic_fingers/dat_0cba1398.inc"
};
static NJS_MODEL dat_0cba1428 = {
#include "ev006a/sonic_fingers/dat_0cba1428.inc"
};
static NJS_OBJECT dat_0cba1450 = {
#include "ev006a/sonic_fingers/dat_0cba1450.inc"
};
static NJS_OBJECT dat_0cba1484 = {
#include "ev006a/sonic_fingers/dat_0cba1484.inc"
};
static NJS_OBJECT dat_0cba14b8 = {
#include "ev006a/sonic_fingers/dat_0cba14b8.inc"
};
static NJS_OBJECT dat_0cba14ec = {
#include "ev006a/sonic_fingers/dat_0cba14ec.inc"
};
static NJS_MATERIAL dat_0cba1520[1] = {
#include "ev006a/sonic_fingers/dat_0cba1520.inc"
};
static Sint16 dat_0cba1534[48] = {
#include "ev006a/sonic_fingers/dat_0cba1534.inc"
};
static NJS_COLOR dat_0cba1594[38] = {
#include "ev006a/sonic_fingers/dat_0cba1594.inc"
};
static NJS_MESHSET dat_0cba162c[1] = {
#include "ev006a/sonic_fingers/dat_0cba162c.inc"
};
static NJS_POINT3 dat_0cba1644[12] = {
#include "ev006a/sonic_fingers/dat_0cba1644.inc"
};
static NJS_VECTOR dat_0cba16d4[12] = {
#include "ev006a/sonic_fingers/dat_0cba16d4.inc"
};
static NJS_MODEL dat_0cba1764 = {
#include "ev006a/sonic_fingers/dat_0cba1764.inc"
};
static NJS_OBJECT dat_0cba178c = {
#include "ev006a/sonic_fingers/dat_0cba178c.inc"
};
static NJS_OBJECT dat_0cba17c0 = {
#include "ev006a/sonic_fingers/dat_0cba17c0.inc"
};
static NJS_OBJECT dat_0cba17f4 = {
#include "ev006a/sonic_fingers/dat_0cba17f4.inc"
};
static NJS_OBJECT dat_0cba1828 = {
#include "ev006a/sonic_fingers/dat_0cba1828.inc"
};
static NJS_MATERIAL dat_0cba185c[1] = {
#include "ev006a/sonic_fingers/dat_0cba185c.inc"
};
static Sint16 dat_0cba1870[48] = {
#include "ev006a/sonic_fingers/dat_0cba1870.inc"
};
static NJS_COLOR dat_0cba18d0[38] = {
#include "ev006a/sonic_fingers/dat_0cba18d0.inc"
};
static NJS_MESHSET dat_0cba1968[1] = {
#include "ev006a/sonic_fingers/dat_0cba1968.inc"
};
static NJS_POINT3 dat_0cba1980[12] = {
#include "ev006a/sonic_fingers/dat_0cba1980.inc"
};
static NJS_VECTOR dat_0cba1a10[12] = {
#include "ev006a/sonic_fingers/dat_0cba1a10.inc"
};
static NJS_MODEL dat_0cba1aa0 = {
#include "ev006a/sonic_fingers/dat_0cba1aa0.inc"
};
static NJS_OBJECT dat_0cba1ac8 = {
#include "ev006a/sonic_fingers/dat_0cba1ac8.inc"
};
static NJS_OBJECT dat_0cba1afc = {
#include "ev006a/sonic_fingers/dat_0cba1afc.inc"
};
static NJS_OBJECT dat_0cba1b30 = {
#include "ev006a/sonic_fingers/dat_0cba1b30.inc"
};
static NJS_OBJECT dat_0cba1b64 = {
#include "ev006a/sonic_fingers/dat_0cba1b64.inc"
};
static NJS_MATERIAL dat_0cba1b98[2] = {
#include "ev006a/sonic_fingers/dat_0cba1b98.inc"
};
static Sint16 dat_0cba1bc0[54] = {
#include "ev006a/sonic_fingers/dat_0cba1bc0.inc"
};
static Sint16 dat_0cba1c2c[22] = {
#include "ev006a/sonic_fingers/dat_0cba1c2c.inc"
};
static NJS_COLOR dat_0cba1c58[48] = {
#include "ev006a/sonic_fingers/dat_0cba1c58.inc"
};
static NJS_COLOR dat_0cba1d18[18] = {
#include "ev006a/sonic_fingers/dat_0cba1d18.inc"
};
static NJS_MESHSET dat_0cba1d60[2] = {
#include "ev006a/sonic_fingers/dat_0cba1d60.inc"
};
static NJS_POINT3 dat_0cba1d90[26] = {
#include "ev006a/sonic_fingers/dat_0cba1d90.inc"
};
static NJS_VECTOR dat_0cba1ec8[26] = {
#include "ev006a/sonic_fingers/dat_0cba1ec8.inc"
};
static NJS_MODEL dat_0cba2000 = {
#include "ev006a/sonic_fingers/dat_0cba2000.inc"
};
static NJS_OBJECT dat_0cba2028 = {
#include "ev006a/sonic_fingers/dat_0cba2028.inc"
};
static NJS_OBJECT dat_0cba205c = {
#include "ev006a/sonic_fingers/dat_0cba205c.inc"
};
static NJS_OBJECT dat_0cba2090 = {
#include "ev006a/sonic_fingers/dat_0cba2090.inc"
};
static NJS_OBJECT dat_0cba20c4 = {
#include "ev006a/sonic_fingers/dat_0cba20c4.inc"
};
NJS_OBJECT dat_0cba20f8 = {
#include "ev006a/sonic_fingers/dat_0cba20f8.inc"
};
static NJS_TEXNAME ev006a_bird_textures_1[12] = {
    {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},  {TEXT_5, 0, 0},
    {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},  {TEXT_9, 0, 0},
    {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0},
};
NJS_TEXLIST ev006a_bird_texlist_1 = {
#include "ev006a/sonic_fingers/ev006a_bird_texlist_1.inc"
};
static NJS_MATERIAL dat_0cba21c4[1] = {
#include "ev006a/sonic_fingers/dat_0cba21c4.inc"
};
static Sint16 dat_0cba21d8[6] = {
#include "ev006a/sonic_fingers/dat_0cba21d8.inc"
};
static NJS_COLOR dat_0cba21e4[4] = {
#include "ev006a/sonic_fingers/dat_0cba21e4.inc"
};
static NJS_MESHSET dat_0cba21f4[1] = {
#include "ev006a/sonic_fingers/dat_0cba21f4.inc"
};
static NJS_POINT3 dat_0cba220c[4] = {
#include "ev006a/sonic_fingers/dat_0cba220c.inc"
};
static NJS_VECTOR dat_0cba223c[4] = {
#include "ev006a/sonic_fingers/dat_0cba223c.inc"
};
static NJS_MODEL dat_0cba226c = {
#include "ev006a/sonic_fingers/dat_0cba226c.inc"
};
static NJS_OBJECT dat_0cba2294 = {
#include "ev006a/sonic_fingers/dat_0cba2294.inc"
};
static NJS_MATERIAL dat_0cba22c8[1] = {
#include "ev006a/sonic_fingers/dat_0cba22c8.inc"
};
static Sint16 dat_0cba22dc[12] = {
#include "ev006a/sonic_fingers/dat_0cba22dc.inc"
};
static NJS_COLOR dat_0cba22f4[9] = {
#include "ev006a/sonic_fingers/dat_0cba22f4.inc"
};
static NJS_MESHSET dat_0cba2318[1] = {
#include "ev006a/sonic_fingers/dat_0cba2318.inc"
};
static NJS_POINT3 dat_0cba2330[7] = {
#include "ev006a/sonic_fingers/dat_0cba2330.inc"
};
static NJS_VECTOR dat_0cba2384[7] = {
#include "ev006a/sonic_fingers/dat_0cba2384.inc"
};
static NJS_MODEL dat_0cba23d8 = {
#include "ev006a/sonic_fingers/dat_0cba23d8.inc"
};
static NJS_OBJECT dat_0cba2400 = {
#include "ev006a/sonic_fingers/dat_0cba2400.inc"
};
static NJS_OBJECT dat_0cba2434 = {
#include "ev006a/sonic_fingers/dat_0cba2434.inc"
};
static NJS_OBJECT dat_0cba2468 = {
#include "ev006a/sonic_fingers/dat_0cba2468.inc"
};
static NJS_OBJECT dat_0cba249c = {
#include "ev006a/sonic_fingers/dat_0cba249c.inc"
};
static NJS_OBJECT dat_0cba24d0 = {
#include "ev006a/sonic_fingers/dat_0cba24d0.inc"
};
static NJS_OBJECT dat_0cba2504 = {
#include "ev006a/sonic_fingers/dat_0cba2504.inc"
};
static NJS_OBJECT dat_0cba2538 = {
#include "ev006a/sonic_fingers/dat_0cba2538.inc"
};
static NJS_MATERIAL dat_0cba256c[1] = {
#include "ev006a/sonic_fingers/dat_0cba256c.inc"
};
static Sint16 dat_0cba2580[6] = {
#include "ev006a/sonic_fingers/dat_0cba2580.inc"
};
static NJS_COLOR dat_0cba258c[4] = {
#include "ev006a/sonic_fingers/dat_0cba258c.inc"
};
static NJS_MESHSET dat_0cba259c[1] = {
#include "ev006a/sonic_fingers/dat_0cba259c.inc"
};
static NJS_POINT3 dat_0cba25b4[4] = {
#include "ev006a/sonic_fingers/dat_0cba25b4.inc"
};
static NJS_VECTOR dat_0cba25e4[4] = {
#include "ev006a/sonic_fingers/dat_0cba25e4.inc"
};
static NJS_MODEL dat_0cba2614 = {
#include "ev006a/sonic_fingers/dat_0cba2614.inc"
};
static NJS_OBJECT dat_0cba263c = {
#include "ev006a/sonic_fingers/dat_0cba263c.inc"
};
static NJS_MATERIAL dat_0cba2670[1] = {
#include "ev006a/sonic_fingers/dat_0cba2670.inc"
};
static Sint16 dat_0cba2684[12] = {
#include "ev006a/sonic_fingers/dat_0cba2684.inc"
};
static NJS_COLOR dat_0cba269c[9] = {
#include "ev006a/sonic_fingers/dat_0cba269c.inc"
};
static NJS_MESHSET dat_0cba26c0[1] = {
#include "ev006a/sonic_fingers/dat_0cba26c0.inc"
};
static NJS_POINT3 dat_0cba26d8[7] = {
#include "ev006a/sonic_fingers/dat_0cba26d8.inc"
};
static NJS_VECTOR dat_0cba272c[7] = {
#include "ev006a/sonic_fingers/dat_0cba272c.inc"
};
static NJS_MODEL dat_0cba2780 = {
#include "ev006a/sonic_fingers/dat_0cba2780.inc"
};
static NJS_OBJECT dat_0cba27a8 = {
#include "ev006a/sonic_fingers/dat_0cba27a8.inc"
};
static NJS_OBJECT dat_0cba27dc = {
#include "ev006a/sonic_fingers/dat_0cba27dc.inc"
};
static NJS_OBJECT dat_0cba2810 = {
#include "ev006a/sonic_fingers/dat_0cba2810.inc"
};
static NJS_OBJECT dat_0cba2844 = {
#include "ev006a/sonic_fingers/dat_0cba2844.inc"
};
static NJS_OBJECT dat_0cba2878 = {
#include "ev006a/sonic_fingers/dat_0cba2878.inc"
};
static NJS_OBJECT dat_0cba28ac = {
#include "ev006a/sonic_fingers/dat_0cba28ac.inc"
};
static NJS_OBJECT dat_0cba28e0 = {
#include "ev006a/sonic_fingers/dat_0cba28e0.inc"
};
static NJS_OBJECT dat_0cba2914 = {
#include "ev006a/sonic_fingers/dat_0cba2914.inc"
};
static NJS_OBJECT dat_0cba2948 = {
#include "ev006a/sonic_fingers/dat_0cba2948.inc"
};
static NJS_OBJECT dat_0cba297c = {
#include "ev006a/sonic_fingers/dat_0cba297c.inc"
};
static NJS_OBJECT dat_0cba29b0 = {
#include "ev006a/sonic_fingers/dat_0cba29b0.inc"
};
static NJS_OBJECT dat_0cba29e4 = {
#include "ev006a/sonic_fingers/dat_0cba29e4.inc"
};
static NJS_MATERIAL dat_0cba2a18[1] = {
#include "ev006a/sonic_fingers/dat_0cba2a18.inc"
};
static Sint16 dat_0cba2a2c[18] = {
#include "ev006a/sonic_fingers/dat_0cba2a2c.inc"
};
static NJS_COLOR dat_0cba2a50[15] = {
#include "ev006a/sonic_fingers/dat_0cba2a50.inc"
};
static NJS_MESHSET dat_0cba2a8c[1] = {
#include "ev006a/sonic_fingers/dat_0cba2a8c.inc"
};
static NJS_POINT3 dat_0cba2aa4[7] = {
#include "ev006a/sonic_fingers/dat_0cba2aa4.inc"
};
static NJS_VECTOR dat_0cba2af8[7] = {
#include "ev006a/sonic_fingers/dat_0cba2af8.inc"
};
static NJS_MODEL dat_0cba2b4c = {
#include "ev006a/sonic_fingers/dat_0cba2b4c.inc"
};
static NJS_OBJECT dat_0cba2b74 = {
#include "ev006a/sonic_fingers/dat_0cba2b74.inc"
};
static NJS_OBJECT dat_0cba2ba8 = {
#include "ev006a/sonic_fingers/dat_0cba2ba8.inc"
};
static NJS_OBJECT dat_0cba2bdc = {
#include "ev006a/sonic_fingers/dat_0cba2bdc.inc"
};
static NJS_OBJECT dat_0cba2c10 = {
#include "ev006a/sonic_fingers/dat_0cba2c10.inc"
};
static NJS_MATERIAL dat_0cba2c44[1] = {
#include "ev006a/sonic_fingers/dat_0cba2c44.inc"
};
static Sint16 dat_0cba2c58[10] = {
#include "ev006a/sonic_fingers/dat_0cba2c58.inc"
};
static NJS_COLOR dat_0cba2c6c[8] = {
#include "ev006a/sonic_fingers/dat_0cba2c6c.inc"
};
static NJS_MESHSET dat_0cba2c8c[1] = {
#include "ev006a/sonic_fingers/dat_0cba2c8c.inc"
};
static NJS_POINT3 dat_0cba2ca4[8] = {
#include "ev006a/sonic_fingers/dat_0cba2ca4.inc"
};
static NJS_VECTOR dat_0cba2d04[8] = {
#include "ev006a/sonic_fingers/dat_0cba2d04.inc"
};
static NJS_MODEL dat_0cba2d64 = {
#include "ev006a/sonic_fingers/dat_0cba2d64.inc"
};
static NJS_OBJECT dat_0cba2d8c = {
#include "ev006a/sonic_fingers/dat_0cba2d8c.inc"
};
static NJS_MATERIAL dat_0cba2dc0[1] = {
#include "ev006a/sonic_fingers/dat_0cba2dc0.inc"
};
static Sint16 dat_0cba2dd4[10] = {
#include "ev006a/sonic_fingers/dat_0cba2dd4.inc"
};
static NJS_COLOR dat_0cba2de8[8] = {
#include "ev006a/sonic_fingers/dat_0cba2de8.inc"
};
static NJS_MESHSET dat_0cba2e08[1] = {
#include "ev006a/sonic_fingers/dat_0cba2e08.inc"
};
static NJS_POINT3 dat_0cba2e20[8] = {
#include "ev006a/sonic_fingers/dat_0cba2e20.inc"
};
static NJS_VECTOR dat_0cba2e80[8] = {
#include "ev006a/sonic_fingers/dat_0cba2e80.inc"
};
static NJS_MODEL dat_0cba2ee0 = {
#include "ev006a/sonic_fingers/dat_0cba2ee0.inc"
};
static NJS_OBJECT dat_0cba2f08 = {
#include "ev006a/sonic_fingers/dat_0cba2f08.inc"
};
static NJS_MATERIAL dat_0cba2f3c[1] = {
#include "ev006a/sonic_fingers/dat_0cba2f3c.inc"
};
static Sint16 dat_0cba2f50[16] = {
#include "ev006a/sonic_fingers/dat_0cba2f50.inc"
};
static NJS_COLOR dat_0cba2f70[12] = {
#include "ev006a/sonic_fingers/dat_0cba2f70.inc"
};
static NJS_MESHSET dat_0cba2fa0[1] = {
#include "ev006a/sonic_fingers/dat_0cba2fa0.inc"
};
static NJS_POINT3 dat_0cba2fb8[12] = {
#include "ev006a/sonic_fingers/dat_0cba2fb8.inc"
};
static NJS_VECTOR dat_0cba3048[12] = {
#include "ev006a/sonic_fingers/dat_0cba3048.inc"
};
static NJS_MODEL dat_0cba30d8 = {
#include "ev006a/sonic_fingers/dat_0cba30d8.inc"
};
static NJS_OBJECT dat_0cba3100 = {
#include "ev006a/sonic_fingers/dat_0cba3100.inc"
};
static NJS_MATERIAL dat_0cba3134[1] = {
#include "ev006a/sonic_fingers/dat_0cba3134.inc"
};
static Sint16 dat_0cba3148[16] = {
#include "ev006a/sonic_fingers/dat_0cba3148.inc"
};
static NJS_COLOR dat_0cba3168[12] = {
#include "ev006a/sonic_fingers/dat_0cba3168.inc"
};
static NJS_MESHSET dat_0cba3198[1] = {
#include "ev006a/sonic_fingers/dat_0cba3198.inc"
};
static NJS_POINT3 dat_0cba31b0[12] = {
#include "ev006a/sonic_fingers/dat_0cba31b0.inc"
};
static NJS_VECTOR dat_0cba3240[12] = {
#include "ev006a/sonic_fingers/dat_0cba3240.inc"
};
static NJS_MODEL dat_0cba32d0 = {
#include "ev006a/sonic_fingers/dat_0cba32d0.inc"
};
static NJS_OBJECT dat_0cba32f8 = {
#include "ev006a/sonic_fingers/dat_0cba32f8.inc"
};
static NJS_MATERIAL dat_0cba332c[4] = {
#include "ev006a/sonic_fingers/dat_0cba332c.inc"
};
static Sint16 dat_0cba337c[239] = {
#include "ev006a/sonic_fingers/dat_0cba337c.inc"
};
static Sint16 dat_0cba355a[22] = {
#include "ev006a/sonic_fingers/dat_0cba355a.inc"
};
static Sint16 dat_0cba3586[21] = {
#include "ev006a/sonic_fingers/dat_0cba3586.inc"
};
static Sint16 dat_0cba35b0[22] = {
#include "ev006a/sonic_fingers/dat_0cba35b0.inc"
};
static NJS_COLOR dat_0cba35dc[209] = {
#include "ev006a/sonic_fingers/dat_0cba35dc.inc"
};
static NJS_COLOR dat_0cba3920[20] = {
#include "ev006a/sonic_fingers/dat_0cba3920.inc"
};
static NJS_COLOR dat_0cba3970[16] = {
#include "ev006a/sonic_fingers/dat_0cba3970.inc"
};
static NJS_COLOR dat_0cba39b0[18] = {
#include "ev006a/sonic_fingers/dat_0cba39b0.inc"
};
static NJS_MESHSET dat_0cba39f8[4] = {
#include "ev006a/sonic_fingers/dat_0cba39f8.inc"
};
static NJS_POINT3 dat_0cba3a58[127] = {
#include "ev006a/sonic_fingers/dat_0cba3a58.inc"
};
static NJS_VECTOR dat_0cba404c[127] = {
#include "ev006a/sonic_fingers/dat_0cba404c.inc"
};
static NJS_MODEL dat_0cba4640 = {
#include "ev006a/sonic_fingers/dat_0cba4640.inc"
};
static NJS_OBJECT dat_0cba4668 = {
#include "ev006a/sonic_fingers/dat_0cba4668.inc"
};
static NJS_OBJECT dat_0cba469c = {
#include "ev006a/sonic_fingers/dat_0cba469c.inc"
};
static NJS_OBJECT dat_0cba46d0 = {
#include "ev006a/sonic_fingers/dat_0cba46d0.inc"
};
static NJS_OBJECT dat_0cba4704 = {
#include "ev006a/sonic_fingers/dat_0cba4704.inc"
};
static NJS_MATERIAL dat_0cba4738[1] = {
#include "ev006a/sonic_fingers/dat_0cba4738.inc"
};
static Sint16 dat_0cba474c[8] = {
#include "ev006a/sonic_fingers/dat_0cba474c.inc"
};
static NJS_COLOR dat_0cba475c[6] = {
#include "ev006a/sonic_fingers/dat_0cba475c.inc"
};
static NJS_MESHSET dat_0cba4774[1] = {
#include "ev006a/sonic_fingers/dat_0cba4774.inc"
};
static NJS_POINT3 dat_0cba478c[6] = {
#include "ev006a/sonic_fingers/dat_0cba478c.inc"
};
static NJS_VECTOR dat_0cba47d4[6] = {
#include "ev006a/sonic_fingers/dat_0cba47d4.inc"
};
static NJS_MODEL dat_0cba481c = {
#include "ev006a/sonic_fingers/dat_0cba481c.inc"
};
static NJS_OBJECT dat_0cba4844 = {
#include "ev006a/sonic_fingers/dat_0cba4844.inc"
};
static NJS_MATERIAL dat_0cba4878[1] = {
#include "ev006a/sonic_fingers/dat_0cba4878.inc"
};
static Sint16 dat_0cba488c[22] = {
#include "ev006a/sonic_fingers/dat_0cba488c.inc"
};
static NJS_COLOR dat_0cba48b8[16] = {
#include "ev006a/sonic_fingers/dat_0cba48b8.inc"
};
static NJS_MESHSET dat_0cba48f8[1] = {
#include "ev006a/sonic_fingers/dat_0cba48f8.inc"
};
static NJS_POINT3 dat_0cba4910[5] = {
#include "ev006a/sonic_fingers/dat_0cba4910.inc"
};
static NJS_VECTOR dat_0cba494c[5] = {
#include "ev006a/sonic_fingers/dat_0cba494c.inc"
};
static NJS_MODEL dat_0cba4988 = {
#include "ev006a/sonic_fingers/dat_0cba4988.inc"
};
static NJS_OBJECT dat_0cba49b0 = {
#include "ev006a/sonic_fingers/dat_0cba49b0.inc"
};
static NJS_MATERIAL dat_0cba49e4[1] = {
#include "ev006a/sonic_fingers/dat_0cba49e4.inc"
};
static Sint16 dat_0cba49f8[30] = {
#include "ev006a/sonic_fingers/dat_0cba49f8.inc"
};
static NJS_COLOR dat_0cba4a34[28] = {
#include "ev006a/sonic_fingers/dat_0cba4a34.inc"
};
static NJS_MESHSET dat_0cba4aa4[1] = {
#include "ev006a/sonic_fingers/dat_0cba4aa4.inc"
};
static NJS_POINT3 dat_0cba4abc[18] = {
#include "ev006a/sonic_fingers/dat_0cba4abc.inc"
};
static NJS_VECTOR dat_0cba4b94[18] = {
#include "ev006a/sonic_fingers/dat_0cba4b94.inc"
};
static NJS_MODEL dat_0cba4c6c = {
#include "ev006a/sonic_fingers/dat_0cba4c6c.inc"
};
static NJS_OBJECT dat_0cba4c94 = {
#include "ev006a/sonic_fingers/dat_0cba4c94.inc"
};
static NJS_MATERIAL dat_0cba4cc8[1] = {
#include "ev006a/sonic_fingers/dat_0cba4cc8.inc"
};
static Sint16 dat_0cba4cdc[20] = {
#include "ev006a/sonic_fingers/dat_0cba4cdc.inc"
};
static NJS_COLOR dat_0cba4d04[16] = {
#include "ev006a/sonic_fingers/dat_0cba4d04.inc"
};
static NJS_MESHSET dat_0cba4d44[1] = {
#include "ev006a/sonic_fingers/dat_0cba4d44.inc"
};
static NJS_POINT3 dat_0cba4d5c[12] = {
#include "ev006a/sonic_fingers/dat_0cba4d5c.inc"
};
static NJS_VECTOR dat_0cba4dec[12] = {
#include "ev006a/sonic_fingers/dat_0cba4dec.inc"
};
static NJS_MODEL dat_0cba4e7c = {
#include "ev006a/sonic_fingers/dat_0cba4e7c.inc"
};
static NJS_OBJECT dat_0cba4ea4 = {
#include "ev006a/sonic_fingers/dat_0cba4ea4.inc"
};
static NJS_MATERIAL dat_0cba4ed8[1] = {
#include "ev006a/sonic_fingers/dat_0cba4ed8.inc"
};
static Sint16 dat_0cba4eec[10] = {
#include "ev006a/sonic_fingers/dat_0cba4eec.inc"
};
static NJS_COLOR dat_0cba4f00[8] = {
#include "ev006a/sonic_fingers/dat_0cba4f00.inc"
};
static NJS_MESHSET dat_0cba4f20[1] = {
#include "ev006a/sonic_fingers/dat_0cba4f20.inc"
};
static NJS_POINT3 dat_0cba4f38[5] = {
#include "ev006a/sonic_fingers/dat_0cba4f38.inc"
};
static NJS_VECTOR dat_0cba4f74[5] = {
#include "ev006a/sonic_fingers/dat_0cba4f74.inc"
};
static NJS_MODEL dat_0cba4fb0 = {
#include "ev006a/sonic_fingers/dat_0cba4fb0.inc"
};
static NJS_OBJECT dat_0cba4fd8 = {
#include "ev006a/sonic_fingers/dat_0cba4fd8.inc"
};
static NJS_MATERIAL dat_0cba500c[1] = {
#include "ev006a/sonic_fingers/dat_0cba500c.inc"
};
static Sint16 dat_0cba5020[10] = {
#include "ev006a/sonic_fingers/dat_0cba5020.inc"
};
static NJS_COLOR dat_0cba5034[8] = {
#include "ev006a/sonic_fingers/dat_0cba5034.inc"
};
static NJS_MESHSET dat_0cba5054[1] = {
#include "ev006a/sonic_fingers/dat_0cba5054.inc"
};
static NJS_POINT3 dat_0cba506c[5] = {
#include "ev006a/sonic_fingers/dat_0cba506c.inc"
};
static NJS_VECTOR dat_0cba50a8[5] = {
#include "ev006a/sonic_fingers/dat_0cba50a8.inc"
};
static NJS_MODEL dat_0cba50e4 = {
#include "ev006a/sonic_fingers/dat_0cba50e4.inc"
};
static NJS_OBJECT dat_0cba510c = {
#include "ev006a/sonic_fingers/dat_0cba510c.inc"
};
static NJS_MATERIAL dat_0cba5140[1] = {
#include "ev006a/sonic_fingers/dat_0cba5140.inc"
};
static Sint16 dat_0cba5154[24] = {
#include "ev006a/sonic_fingers/dat_0cba5154.inc"
};
static NJS_COLOR dat_0cba5184[20] = {
#include "ev006a/sonic_fingers/dat_0cba5184.inc"
};
static NJS_MESHSET dat_0cba51d4[1] = {
#include "ev006a/sonic_fingers/dat_0cba51d4.inc"
};
static NJS_POINT3 dat_0cba51ec[12] = {
#include "ev006a/sonic_fingers/dat_0cba51ec.inc"
};
static NJS_VECTOR dat_0cba527c[12] = {
#include "ev006a/sonic_fingers/dat_0cba527c.inc"
};
static NJS_MODEL dat_0cba530c = {
#include "ev006a/sonic_fingers/dat_0cba530c.inc"
};
static NJS_OBJECT dat_0cba5334 = {
#include "ev006a/sonic_fingers/dat_0cba5334.inc"
};
static NJS_MATERIAL dat_0cba5368[1] = {
#include "ev006a/sonic_fingers/dat_0cba5368.inc"
};
static Sint16 dat_0cba537c[36] = {
#include "ev006a/sonic_fingers/dat_0cba537c.inc"
};
static NJS_COLOR dat_0cba53c4[30] = {
#include "ev006a/sonic_fingers/dat_0cba53c4.inc"
};
static NJS_MESHSET dat_0cba543c[1] = {
#include "ev006a/sonic_fingers/dat_0cba543c.inc"
};
static NJS_POINT3 dat_0cba5454[12] = {
#include "ev006a/sonic_fingers/dat_0cba5454.inc"
};
static NJS_VECTOR dat_0cba54e4[12] = {
#include "ev006a/sonic_fingers/dat_0cba54e4.inc"
};
static NJS_MODEL dat_0cba5574 = {
#include "ev006a/sonic_fingers/dat_0cba5574.inc"
};
static NJS_OBJECT dat_0cba559c = {
#include "ev006a/sonic_fingers/dat_0cba559c.inc"
};
static NJS_OBJECT dat_0cba55d0 = {
#include "ev006a/sonic_fingers/dat_0cba55d0.inc"
};
static NJS_OBJECT dat_0cba5604 = {
#include "ev006a/sonic_fingers/dat_0cba5604.inc"
};
static NJS_OBJECT dat_0cba5638 = {
#include "ev006a/sonic_fingers/dat_0cba5638.inc"
};
static NJS_MATERIAL dat_0cba566c[1] = {
#include "ev006a/sonic_fingers/dat_0cba566c.inc"
};
static Sint16 dat_0cba5680[48] = {
#include "ev006a/sonic_fingers/dat_0cba5680.inc"
};
static NJS_COLOR dat_0cba56e0[38] = {
#include "ev006a/sonic_fingers/dat_0cba56e0.inc"
};
static NJS_MESHSET dat_0cba5778[1] = {
#include "ev006a/sonic_fingers/dat_0cba5778.inc"
};
static NJS_POINT3 dat_0cba5790[12] = {
#include "ev006a/sonic_fingers/dat_0cba5790.inc"
};
static NJS_VECTOR dat_0cba5820[12] = {
#include "ev006a/sonic_fingers/dat_0cba5820.inc"
};
static NJS_MODEL dat_0cba58b0 = {
#include "ev006a/sonic_fingers/dat_0cba58b0.inc"
};
static NJS_OBJECT dat_0cba58d8 = {
#include "ev006a/sonic_fingers/dat_0cba58d8.inc"
};
static NJS_OBJECT dat_0cba590c = {
#include "ev006a/sonic_fingers/dat_0cba590c.inc"
};
static NJS_OBJECT dat_0cba5940 = {
#include "ev006a/sonic_fingers/dat_0cba5940.inc"
};
static NJS_OBJECT dat_0cba5974 = {
#include "ev006a/sonic_fingers/dat_0cba5974.inc"
};
static NJS_MATERIAL dat_0cba59a8[1] = {
#include "ev006a/sonic_fingers/dat_0cba59a8.inc"
};
static Sint16 dat_0cba59bc[48] = {
#include "ev006a/sonic_fingers/dat_0cba59bc.inc"
};
static NJS_COLOR dat_0cba5a1c[38] = {
#include "ev006a/sonic_fingers/dat_0cba5a1c.inc"
};
static NJS_MESHSET dat_0cba5ab4[1] = {
#include "ev006a/sonic_fingers/dat_0cba5ab4.inc"
};
static NJS_POINT3 dat_0cba5acc[12] = {
#include "ev006a/sonic_fingers/dat_0cba5acc.inc"
};
static NJS_VECTOR dat_0cba5b5c[12] = {
#include "ev006a/sonic_fingers/dat_0cba5b5c.inc"
};
static NJS_MODEL dat_0cba5bec = {
#include "ev006a/sonic_fingers/dat_0cba5bec.inc"
};
static NJS_OBJECT dat_0cba5c14 = {
#include "ev006a/sonic_fingers/dat_0cba5c14.inc"
};
static NJS_OBJECT dat_0cba5c48 = {
#include "ev006a/sonic_fingers/dat_0cba5c48.inc"
};
static NJS_OBJECT dat_0cba5c7c = {
#include "ev006a/sonic_fingers/dat_0cba5c7c.inc"
};
static NJS_OBJECT dat_0cba5cb0 = {
#include "ev006a/sonic_fingers/dat_0cba5cb0.inc"
};
static NJS_MATERIAL dat_0cba5ce4[2] = {
#include "ev006a/sonic_fingers/dat_0cba5ce4.inc"
};
static Sint16 dat_0cba5d0c[54] = {
#include "ev006a/sonic_fingers/dat_0cba5d0c.inc"
};
static Sint16 dat_0cba5d78[22] = {
#include "ev006a/sonic_fingers/dat_0cba5d78.inc"
};
static NJS_COLOR dat_0cba5da4[48] = {
#include "ev006a/sonic_fingers/dat_0cba5da4.inc"
};
static NJS_COLOR dat_0cba5e64[18] = {
#include "ev006a/sonic_fingers/dat_0cba5e64.inc"
};
static NJS_MESHSET dat_0cba5eac[2] = {
#include "ev006a/sonic_fingers/dat_0cba5eac.inc"
};
static NJS_POINT3 dat_0cba5edc[26] = {
#include "ev006a/sonic_fingers/dat_0cba5edc.inc"
};
static NJS_VECTOR dat_0cba6014[26] = {
#include "ev006a/sonic_fingers/dat_0cba6014.inc"
};
static NJS_MODEL dat_0cba614c = {
#include "ev006a/sonic_fingers/dat_0cba614c.inc"
};
static NJS_OBJECT dat_0cba6174 = {
#include "ev006a/sonic_fingers/dat_0cba6174.inc"
};
static NJS_OBJECT dat_0cba61a8 = {
#include "ev006a/sonic_fingers/dat_0cba61a8.inc"
};
static NJS_OBJECT dat_0cba61dc = {
#include "ev006a/sonic_fingers/dat_0cba61dc.inc"
};
static NJS_OBJECT dat_0cba6210 = {
#include "ev006a/sonic_fingers/dat_0cba6210.inc"
};
static NJS_OBJECT dat_0cba6244 = {
#include "ev006a/sonic_fingers/dat_0cba6244.inc"
};
NJS_TEXNAME ev006a_bird_textures_2[12] = {
    {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},  {TEXT_5, 0, 0},
    {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},  {TEXT_9, 0, 0},
    {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0},
};
