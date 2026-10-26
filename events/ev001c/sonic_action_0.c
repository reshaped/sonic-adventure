/* the actions script.c plays on Sonic and Big, with their motions.
 * ev001c_sonic_action_0 to _5 move sonic_object, and ev001c_big_action_0 to
 * _2 move big_object. both models lie in 1ST_READ.BIN */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev001c/sonic_action_0.h"
#include "player/big_models.h" /* NOLINT(misc-include-cleaner) */

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 7
#include "ev001c/sonic_action_0/text.inc"

extern NJS_OBJECT sonic_object;

/* the texture name s_testhand and its texlist, dat_0cb83e7c, then a model
 * of Sonic's fingers and its node, dat_0cb84314. no word in EV001C or
 * 1ST_READ.BIN points at the texlist or the node. from here through the
 * node, the bytes match EV001B's from dat_0cb821d0 through
 * ev001b_sonic_fingers. only the words that point into the image differ */
static NJS_TEXNAME dat_0cb83e70[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cb83e7c = {
#include "ev001c/sonic_action_0/dat_0cb83e7c.inc"
};
static NJS_MATERIAL dat_0cb83e84[3] = {
#include "ev001c/sonic_action_0/dat_0cb83e84.inc"
};
static Sint16 dat_0cb83ec0[20] = {
#include "ev001c/sonic_action_0/dat_0cb83ec0.inc"
};
static Sint16 dat_0cb83ee8[38] = {
#include "ev001c/sonic_action_0/dat_0cb83ee8.inc"
};
static Sint16 dat_0cb83f34[40] = {
#include "ev001c/sonic_action_0/dat_0cb83f34.inc"
};
static NJS_COLOR dat_0cb83f84[32] = {
#include "ev001c/sonic_action_0/dat_0cb83f84.inc"
};
static NJS_MESHSET dat_0cb84004[3] = {
#include "ev001c/sonic_action_0/dat_0cb84004.inc"
};
static NJS_POINT3 dat_0cb8404c[28] = {
#include "ev001c/sonic_action_0/dat_0cb8404c.inc"
};
static NJS_VECTOR dat_0cb8419c[28] = {
#include "ev001c/sonic_action_0/dat_0cb8419c.inc"
};
static NJS_MODEL dat_0cb842ec = {
#include "ev001c/sonic_action_0/dat_0cb842ec.inc"
};
static NJS_OBJECT dat_0cb84314 = {
#include "ev001c/sonic_action_0/dat_0cb84314.inc"
};
static NJS_MKEY_F dat_0cb84348[15] = {
#include "ev001c/sonic_action_0/dat_0cb84348.inc"
};
static NJS_MKEY_A dat_0cb84438[2] = {
#include "ev001c/sonic_action_0/dat_0cb84438.inc"
};
static NJS_MKEY_A dat_0cb84458[2] = {
#include "ev001c/sonic_action_0/dat_0cb84458.inc"
};
static NJS_MKEY_A dat_0cb84478[2] = {
#include "ev001c/sonic_action_0/dat_0cb84478.inc"
};
static NJS_MKEY_A dat_0cb84498[2] = {
#include "ev001c/sonic_action_0/dat_0cb84498.inc"
};
static NJS_MKEY_A dat_0cb844b8[2] = {
#include "ev001c/sonic_action_0/dat_0cb844b8.inc"
};
static NJS_MKEY_A dat_0cb844d8[2] = {
#include "ev001c/sonic_action_0/dat_0cb844d8.inc"
};
static NJS_MKEY_A dat_0cb844f8[2] = {
#include "ev001c/sonic_action_0/dat_0cb844f8.inc"
};
static NJS_MKEY_A dat_0cb84518[2] = {
#include "ev001c/sonic_action_0/dat_0cb84518.inc"
};
static NJS_MKEY_A dat_0cb84538[2] = {
#include "ev001c/sonic_action_0/dat_0cb84538.inc"
};
static NJS_MKEY_A dat_0cb84558[2] = {
#include "ev001c/sonic_action_0/dat_0cb84558.inc"
};
static NJS_MKEY_A dat_0cb84578[2] = {
#include "ev001c/sonic_action_0/dat_0cb84578.inc"
};
static NJS_MKEY_A dat_0cb84598[2] = {
#include "ev001c/sonic_action_0/dat_0cb84598.inc"
};
static NJS_MKEY_A dat_0cb845b8[2] = {
#include "ev001c/sonic_action_0/dat_0cb845b8.inc"
};
static NJS_MKEY_A dat_0cb845d8[2] = {
#include "ev001c/sonic_action_0/dat_0cb845d8.inc"
};
static NJS_MKEY_A dat_0cb845f8[2] = {
#include "ev001c/sonic_action_0/dat_0cb845f8.inc"
};
static NJS_MKEY_A dat_0cb84618[2] = {
#include "ev001c/sonic_action_0/dat_0cb84618.inc"
};
static NJS_MKEY_A dat_0cb84638[2] = {
#include "ev001c/sonic_action_0/dat_0cb84638.inc"
};
static NJS_MKEY_A dat_0cb84658[2] = {
#include "ev001c/sonic_action_0/dat_0cb84658.inc"
};
static NJS_MKEY_A dat_0cb84678[2] = {
#include "ev001c/sonic_action_0/dat_0cb84678.inc"
};
static NJS_MKEY_A dat_0cb84698[2] = {
#include "ev001c/sonic_action_0/dat_0cb84698.inc"
};
static NJS_MKEY_A dat_0cb846b8[2] = {
#include "ev001c/sonic_action_0/dat_0cb846b8.inc"
};
static NJS_MKEY_A dat_0cb846d8[2] = {
#include "ev001c/sonic_action_0/dat_0cb846d8.inc"
};
static NJS_MKEY_A dat_0cb846f8[2] = {
#include "ev001c/sonic_action_0/dat_0cb846f8.inc"
};
static NJS_MKEY_A dat_0cb84718[2] = {
#include "ev001c/sonic_action_0/dat_0cb84718.inc"
};
static NJS_MKEY_A dat_0cb84738[2] = {
#include "ev001c/sonic_action_0/dat_0cb84738.inc"
};
static NJS_MKEY_A dat_0cb84758[15] = {
#include "ev001c/sonic_action_0/dat_0cb84758.inc"
};
static NJS_MKEY_A dat_0cb84848[15] = {
#include "ev001c/sonic_action_0/dat_0cb84848.inc"
};
static NJS_MKEY_A dat_0cb84938[15] = {
#include "ev001c/sonic_action_0/dat_0cb84938.inc"
};
static NJS_MKEY_A dat_0cb84a28[2] = {
#include "ev001c/sonic_action_0/dat_0cb84a28.inc"
};
static NJS_MKEY_A dat_0cb84a48[2] = {
#include "ev001c/sonic_action_0/dat_0cb84a48.inc"
};
static NJS_MKEY_A dat_0cb84a68[2] = {
#include "ev001c/sonic_action_0/dat_0cb84a68.inc"
};
static NJS_MKEY_A dat_0cb84a88[2] = {
#include "ev001c/sonic_action_0/dat_0cb84a88.inc"
};
static NJS_MKEY_A dat_0cb84aa8[2] = {
#include "ev001c/sonic_action_0/dat_0cb84aa8.inc"
};
static NJS_MKEY_A dat_0cb84ac8[2] = {
#include "ev001c/sonic_action_0/dat_0cb84ac8.inc"
};
static NJS_MKEY_A dat_0cb84ae8[2] = {
#include "ev001c/sonic_action_0/dat_0cb84ae8.inc"
};
static NJS_MKEY_A dat_0cb84b08[2] = {
#include "ev001c/sonic_action_0/dat_0cb84b08.inc"
};
static NJS_MKEY_A dat_0cb84b28[15] = {
#include "ev001c/sonic_action_0/dat_0cb84b28.inc"
};
static NJS_MKEY_A dat_0cb84c18[15] = {
#include "ev001c/sonic_action_0/dat_0cb84c18.inc"
};
static NJS_MKEY_A dat_0cb84d08[15] = {
#include "ev001c/sonic_action_0/dat_0cb84d08.inc"
};
static NJS_MKEY_A dat_0cb84df8[2] = {
#include "ev001c/sonic_action_0/dat_0cb84df8.inc"
};
static NJS_MKEY_A dat_0cb84e18[2] = {
#include "ev001c/sonic_action_0/dat_0cb84e18.inc"
};
static NJS_MKEY_A dat_0cb84e38[2] = {
#include "ev001c/sonic_action_0/dat_0cb84e38.inc"
};
static NJS_MKEY_A dat_0cb84e58[2] = {
#include "ev001c/sonic_action_0/dat_0cb84e58.inc"
};
static NJS_MKEY_A dat_0cb84e78[2] = {
#include "ev001c/sonic_action_0/dat_0cb84e78.inc"
};
static NJS_MKEY_A dat_0cb84e98[2] = {
#include "ev001c/sonic_action_0/dat_0cb84e98.inc"
};
static NJS_MKEY_A dat_0cb84eb8[2] = {
#include "ev001c/sonic_action_0/dat_0cb84eb8.inc"
};
static NJS_MKEY_A dat_0cb84ed8[2] = {
#include "ev001c/sonic_action_0/dat_0cb84ed8.inc"
};
static NJS_MKEY_A dat_0cb84ef8[15] = {
#include "ev001c/sonic_action_0/dat_0cb84ef8.inc"
};
static NJS_MKEY_A dat_0cb84fe8[15] = {
#include "ev001c/sonic_action_0/dat_0cb84fe8.inc"
};
static NJS_MKEY_A dat_0cb850d8[15] = {
#include "ev001c/sonic_action_0/dat_0cb850d8.inc"
};
static NJS_MKEY_A dat_0cb851c8[2] = {
#include "ev001c/sonic_action_0/dat_0cb851c8.inc"
};
static NJS_MKEY_A dat_0cb851e8[2] = {
#include "ev001c/sonic_action_0/dat_0cb851e8.inc"
};
static NJS_MKEY_A dat_0cb85208[15] = {
#include "ev001c/sonic_action_0/dat_0cb85208.inc"
};
static NJS_MKEY_A dat_0cb852f8[2] = {
#include "ev001c/sonic_action_0/dat_0cb852f8.inc"
};
static NJS_MKEY_A dat_0cb85318[2] = {
#include "ev001c/sonic_action_0/dat_0cb85318.inc"
};
static NJS_MKEY_A dat_0cb85338[2] = {
#include "ev001c/sonic_action_0/dat_0cb85338.inc"
};
static NJS_MKEY_A dat_0cb85358[2] = {
#include "ev001c/sonic_action_0/dat_0cb85358.inc"
};
static NJS_MKEY_A dat_0cb85378[15] = {
#include "ev001c/sonic_action_0/dat_0cb85378.inc"
};
static NJS_MKEY_A dat_0cb85468[15] = {
#include "ev001c/sonic_action_0/dat_0cb85468.inc"
};
static NJS_MKEY_A dat_0cb85558[15] = {
#include "ev001c/sonic_action_0/dat_0cb85558.inc"
};
static NJS_MKEY_A dat_0cb85648[2] = {
#include "ev001c/sonic_action_0/dat_0cb85648.inc"
};
static NJS_MKEY_A dat_0cb85668[2] = {
#include "ev001c/sonic_action_0/dat_0cb85668.inc"
};
static NJS_MKEY_A dat_0cb85688[15] = {
#include "ev001c/sonic_action_0/dat_0cb85688.inc"
};
static NJS_MKEY_A dat_0cb85778[2] = {
#include "ev001c/sonic_action_0/dat_0cb85778.inc"
};
static NJS_MKEY_A dat_0cb85798[2] = {
#include "ev001c/sonic_action_0/dat_0cb85798.inc"
};
static NJS_MKEY_A dat_0cb857b8[2] = {
#include "ev001c/sonic_action_0/dat_0cb857b8.inc"
};
static NJS_MKEY_A dat_0cb857d8[2] = {
#include "ev001c/sonic_action_0/dat_0cb857d8.inc"
};
static NJS_MKEY_A dat_0cb857f8[2] = {
#include "ev001c/sonic_action_0/dat_0cb857f8.inc"
};
static NJS_MKEY_A dat_0cb85818[2] = {
#include "ev001c/sonic_action_0/dat_0cb85818.inc"
};
static NJS_MKEY_A dat_0cb85838[2] = {
#include "ev001c/sonic_action_0/dat_0cb85838.inc"
};
static NJS_MKEY_A dat_0cb85858[2] = {
#include "ev001c/sonic_action_0/dat_0cb85858.inc"
};
static NJS_MKEY_A dat_0cb85878[2] = {
#include "ev001c/sonic_action_0/dat_0cb85878.inc"
};
static NJS_MKEY_A dat_0cb85898[2] = {
#include "ev001c/sonic_action_0/dat_0cb85898.inc"
};
static NJS_MDATA2 dat_0cb858b8[73] = {
#include "ev001c/sonic_action_0/dat_0cb858b8.inc"
};
static NJS_MOTION ev001c_sonic_motion_0 = {
#include "ev001c/sonic_action_0/ev001c_sonic_motion_0.inc"
};
NJS_ACTION ev001c_sonic_action_0 = {
#include "ev001c/sonic_action_0/ev001c_sonic_action_0.inc"
};
static NJS_MKEY_F dat_0cb85d5c[30] = {
#include "ev001c/sonic_action_0/dat_0cb85d5c.inc"
};
static NJS_MKEY_A dat_0cb85f3c[30] = {
#include "ev001c/sonic_action_0/dat_0cb85f3c.inc"
};
static NJS_MKEY_A dat_0cb8611c[30] = {
#include "ev001c/sonic_action_0/dat_0cb8611c.inc"
};
static NJS_MKEY_A dat_0cb862fc[29] = {
#include "ev001c/sonic_action_0/dat_0cb862fc.inc"
};
static NJS_MKEY_A dat_0cb864cc[30] = {
#include "ev001c/sonic_action_0/dat_0cb864cc.inc"
};
static NJS_MKEY_A dat_0cb866ac[29] = {
#include "ev001c/sonic_action_0/dat_0cb866ac.inc"
};
static NJS_MKEY_A dat_0cb8687c[29] = {
#include "ev001c/sonic_action_0/dat_0cb8687c.inc"
};
static NJS_MKEY_A dat_0cb86a4c[30] = {
#include "ev001c/sonic_action_0/dat_0cb86a4c.inc"
};
static NJS_MKEY_A dat_0cb86c2c[30] = {
#include "ev001c/sonic_action_0/dat_0cb86c2c.inc"
};
static NJS_MKEY_A dat_0cb86e0c[8] = {
#include "ev001c/sonic_action_0/dat_0cb86e0c.inc"
};
static NJS_MKEY_A dat_0cb86e8c[23] = {
#include "ev001c/sonic_action_0/dat_0cb86e8c.inc"
};
static NJS_MKEY_A dat_0cb86ffc[23] = {
#include "ev001c/sonic_action_0/dat_0cb86ffc.inc"
};
static NJS_MKEY_A dat_0cb8716c[22] = {
#include "ev001c/sonic_action_0/dat_0cb8716c.inc"
};
static NJS_MKEY_A dat_0cb872cc[24] = {
#include "ev001c/sonic_action_0/dat_0cb872cc.inc"
};
static NJS_MKEY_A dat_0cb8744c[30] = {
#include "ev001c/sonic_action_0/dat_0cb8744c.inc"
};
static NJS_MKEY_A dat_0cb8762c[24] = {
#include "ev001c/sonic_action_0/dat_0cb8762c.inc"
};
static NJS_MKEY_A dat_0cb877ac[26] = {
#include "ev001c/sonic_action_0/dat_0cb877ac.inc"
};
static NJS_MKEY_A dat_0cb8794c[29] = {
#include "ev001c/sonic_action_0/dat_0cb8794c.inc"
};
static NJS_MKEY_A dat_0cb87b1c[30] = {
#include "ev001c/sonic_action_0/dat_0cb87b1c.inc"
};
static NJS_MKEY_A dat_0cb87cfc[30] = {
#include "ev001c/sonic_action_0/dat_0cb87cfc.inc"
};
static NJS_MKEY_A dat_0cb87edc[30] = {
#include "ev001c/sonic_action_0/dat_0cb87edc.inc"
};
static NJS_MKEY_A dat_0cb880bc[30] = {
#include "ev001c/sonic_action_0/dat_0cb880bc.inc"
};
static NJS_MDATA2 dat_0cb8829c[73] = {
#include "ev001c/sonic_action_0/dat_0cb8829c.inc"
};
static NJS_MOTION ev001c_sonic_motion_1 = {
#include "ev001c/sonic_action_0/ev001c_sonic_motion_1.inc"
};
NJS_ACTION ev001c_sonic_action_1 = {
#include "ev001c/sonic_action_0/ev001c_sonic_action_1.inc"
};
static NJS_MKEY_F dat_0cb88740[31] = {
#include "ev001c/sonic_action_0/dat_0cb88740.inc"
};
static NJS_MKEY_A dat_0cb88930[2] = {
#include "ev001c/sonic_action_0/dat_0cb88930.inc"
};
static NJS_MKEY_A dat_0cb88950[30] = {
#include "ev001c/sonic_action_0/dat_0cb88950.inc"
};
static NJS_MKEY_A dat_0cb88b30[2] = {
#include "ev001c/sonic_action_0/dat_0cb88b30.inc"
};
static NJS_MKEY_A dat_0cb88b50[2] = {
#include "ev001c/sonic_action_0/dat_0cb88b50.inc"
};
static NJS_MKEY_A dat_0cb88b70[2] = {
#include "ev001c/sonic_action_0/dat_0cb88b70.inc"
};
static NJS_MKEY_A dat_0cb88b90[2] = {
#include "ev001c/sonic_action_0/dat_0cb88b90.inc"
};
static NJS_MKEY_A dat_0cb88bb0[2] = {
#include "ev001c/sonic_action_0/dat_0cb88bb0.inc"
};
static NJS_MKEY_A dat_0cb88bd0[2] = {
#include "ev001c/sonic_action_0/dat_0cb88bd0.inc"
};
static NJS_MKEY_A dat_0cb88bf0[2] = {
#include "ev001c/sonic_action_0/dat_0cb88bf0.inc"
};
static NJS_MKEY_A dat_0cb88c10[2] = {
#include "ev001c/sonic_action_0/dat_0cb88c10.inc"
};
static NJS_MKEY_A dat_0cb88c30[2] = {
#include "ev001c/sonic_action_0/dat_0cb88c30.inc"
};
static NJS_MKEY_A dat_0cb88c50[2] = {
#include "ev001c/sonic_action_0/dat_0cb88c50.inc"
};
static NJS_MKEY_A dat_0cb88c70[21] = {
#include "ev001c/sonic_action_0/dat_0cb88c70.inc"
};
static NJS_MKEY_A dat_0cb88dc0[2] = {
#include "ev001c/sonic_action_0/dat_0cb88dc0.inc"
};
static NJS_MKEY_A dat_0cb88de0[2] = {
#include "ev001c/sonic_action_0/dat_0cb88de0.inc"
};
static NJS_MKEY_A dat_0cb88e00[2] = {
#include "ev001c/sonic_action_0/dat_0cb88e00.inc"
};
static NJS_MKEY_A dat_0cb88e20[2] = {
#include "ev001c/sonic_action_0/dat_0cb88e20.inc"
};
static NJS_MKEY_A dat_0cb88e40[2] = {
#include "ev001c/sonic_action_0/dat_0cb88e40.inc"
};
static NJS_MKEY_A dat_0cb88e60[2] = {
#include "ev001c/sonic_action_0/dat_0cb88e60.inc"
};
static NJS_MKEY_A dat_0cb88e80[6] = {
#include "ev001c/sonic_action_0/dat_0cb88e80.inc"
};
static NJS_MKEY_A dat_0cb88ee0[2] = {
#include "ev001c/sonic_action_0/dat_0cb88ee0.inc"
};
static NJS_MKEY_A dat_0cb88f00[2] = {
#include "ev001c/sonic_action_0/dat_0cb88f00.inc"
};
static NJS_MKEY_A dat_0cb88f20[2] = {
#include "ev001c/sonic_action_0/dat_0cb88f20.inc"
};
static NJS_MKEY_A dat_0cb88f40[2] = {
#include "ev001c/sonic_action_0/dat_0cb88f40.inc"
};
static NJS_MKEY_A dat_0cb88f60[2] = {
#include "ev001c/sonic_action_0/dat_0cb88f60.inc"
};
static NJS_MKEY_A dat_0cb88f80[30] = {
#include "ev001c/sonic_action_0/dat_0cb88f80.inc"
};
static NJS_MKEY_A dat_0cb89160[30] = {
#include "ev001c/sonic_action_0/dat_0cb89160.inc"
};
static NJS_MKEY_A dat_0cb89340[29] = {
#include "ev001c/sonic_action_0/dat_0cb89340.inc"
};
static NJS_MKEY_A dat_0cb89510[2] = {
#include "ev001c/sonic_action_0/dat_0cb89510.inc"
};
static NJS_MKEY_A dat_0cb89530[2] = {
#include "ev001c/sonic_action_0/dat_0cb89530.inc"
};
static NJS_MKEY_A dat_0cb89550[31] = {
#include "ev001c/sonic_action_0/dat_0cb89550.inc"
};
static NJS_MKEY_A dat_0cb89740[2] = {
#include "ev001c/sonic_action_0/dat_0cb89740.inc"
};
static NJS_MKEY_A dat_0cb89760[2] = {
#include "ev001c/sonic_action_0/dat_0cb89760.inc"
};
static NJS_MKEY_A dat_0cb89780[2] = {
#include "ev001c/sonic_action_0/dat_0cb89780.inc"
};
static NJS_MKEY_A dat_0cb897a0[2] = {
#include "ev001c/sonic_action_0/dat_0cb897a0.inc"
};
static NJS_MKEY_A dat_0cb897c0[2] = {
#include "ev001c/sonic_action_0/dat_0cb897c0.inc"
};
static NJS_MKEY_A dat_0cb897e0[31] = {
#include "ev001c/sonic_action_0/dat_0cb897e0.inc"
};
static NJS_MKEY_A dat_0cb899d0[31] = {
#include "ev001c/sonic_action_0/dat_0cb899d0.inc"
};
static NJS_MKEY_A dat_0cb89bc0[31] = {
#include "ev001c/sonic_action_0/dat_0cb89bc0.inc"
};
static NJS_MKEY_A dat_0cb89db0[2] = {
#include "ev001c/sonic_action_0/dat_0cb89db0.inc"
};
static NJS_MKEY_A dat_0cb89dd0[2] = {
#include "ev001c/sonic_action_0/dat_0cb89dd0.inc"
};
static NJS_MKEY_A dat_0cb89df0[2] = {
#include "ev001c/sonic_action_0/dat_0cb89df0.inc"
};
static NJS_MKEY_A dat_0cb89e10[2] = {
#include "ev001c/sonic_action_0/dat_0cb89e10.inc"
};
static NJS_MKEY_A dat_0cb89e30[2] = {
#include "ev001c/sonic_action_0/dat_0cb89e30.inc"
};
static NJS_MKEY_A dat_0cb89e50[2] = {
#include "ev001c/sonic_action_0/dat_0cb89e50.inc"
};
static NJS_MKEY_A dat_0cb89e70[2] = {
#include "ev001c/sonic_action_0/dat_0cb89e70.inc"
};
static NJS_MKEY_A dat_0cb89e90[2] = {
#include "ev001c/sonic_action_0/dat_0cb89e90.inc"
};
static NJS_MKEY_A dat_0cb89eb0[30] = {
#include "ev001c/sonic_action_0/dat_0cb89eb0.inc"
};
static NJS_MKEY_A dat_0cb8a090[30] = {
#include "ev001c/sonic_action_0/dat_0cb8a090.inc"
};
static NJS_MKEY_A dat_0cb8a270[30] = {
#include "ev001c/sonic_action_0/dat_0cb8a270.inc"
};
static NJS_MKEY_A dat_0cb8a450[2] = {
#include "ev001c/sonic_action_0/dat_0cb8a450.inc"
};
static NJS_MKEY_A dat_0cb8a470[2] = {
#include "ev001c/sonic_action_0/dat_0cb8a470.inc"
};
static NJS_MKEY_A dat_0cb8a490[31] = {
#include "ev001c/sonic_action_0/dat_0cb8a490.inc"
};
static NJS_MKEY_A dat_0cb8a680[2] = {
#include "ev001c/sonic_action_0/dat_0cb8a680.inc"
};
static NJS_MKEY_A dat_0cb8a6a0[2] = {
#include "ev001c/sonic_action_0/dat_0cb8a6a0.inc"
};
static NJS_MKEY_A dat_0cb8a6c0[2] = {
#include "ev001c/sonic_action_0/dat_0cb8a6c0.inc"
};
static NJS_MKEY_A dat_0cb8a6e0[2] = {
#include "ev001c/sonic_action_0/dat_0cb8a6e0.inc"
};
static NJS_MKEY_A dat_0cb8a700[30] = {
#include "ev001c/sonic_action_0/dat_0cb8a700.inc"
};
static NJS_MKEY_A dat_0cb8a8e0[30] = {
#include "ev001c/sonic_action_0/dat_0cb8a8e0.inc"
};
static NJS_MKEY_A dat_0cb8aac0[30] = {
#include "ev001c/sonic_action_0/dat_0cb8aac0.inc"
};
static NJS_MKEY_A dat_0cb8aca0[2] = {
#include "ev001c/sonic_action_0/dat_0cb8aca0.inc"
};
static NJS_MKEY_A dat_0cb8acc0[2] = {
#include "ev001c/sonic_action_0/dat_0cb8acc0.inc"
};
static NJS_MKEY_A dat_0cb8ace0[31] = {
#include "ev001c/sonic_action_0/dat_0cb8ace0.inc"
};
static NJS_MKEY_A dat_0cb8aed0[2] = {
#include "ev001c/sonic_action_0/dat_0cb8aed0.inc"
};
static NJS_MKEY_A dat_0cb8aef0[2] = {
#include "ev001c/sonic_action_0/dat_0cb8aef0.inc"
};
static NJS_MKEY_A dat_0cb8af10[2] = {
#include "ev001c/sonic_action_0/dat_0cb8af10.inc"
};
static NJS_MKEY_A dat_0cb8af30[2] = {
#include "ev001c/sonic_action_0/dat_0cb8af30.inc"
};
static NJS_MKEY_A dat_0cb8af50[2] = {
#include "ev001c/sonic_action_0/dat_0cb8af50.inc"
};
static NJS_MKEY_A dat_0cb8af70[2] = {
#include "ev001c/sonic_action_0/dat_0cb8af70.inc"
};
static NJS_MKEY_A dat_0cb8af90[2] = {
#include "ev001c/sonic_action_0/dat_0cb8af90.inc"
};
static NJS_MKEY_A dat_0cb8afb0[2] = {
#include "ev001c/sonic_action_0/dat_0cb8afb0.inc"
};
static NJS_MKEY_A dat_0cb8afd0[2] = {
#include "ev001c/sonic_action_0/dat_0cb8afd0.inc"
};
static NJS_MKEY_A dat_0cb8aff0[2] = {
#include "ev001c/sonic_action_0/dat_0cb8aff0.inc"
};
static NJS_MDATA2 dat_0cb8b010[73] = {
#include "ev001c/sonic_action_0/dat_0cb8b010.inc"
};
static NJS_MOTION ev001c_sonic_motion_2 = {
#include "ev001c/sonic_action_0/ev001c_sonic_motion_2.inc"
};
NJS_ACTION ev001c_sonic_action_2 = {
#include "ev001c/sonic_action_0/ev001c_sonic_action_2.inc"
};
static NJS_MKEY_F dat_0cb8b4b4[15] = {
#include "ev001c/sonic_action_0/dat_0cb8b4b4.inc"
};
static NJS_MKEY_A dat_0cb8b5a4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b5a4.inc"
};
static NJS_MKEY_A dat_0cb8b5c4[15] = {
#include "ev001c/sonic_action_0/dat_0cb8b5c4.inc"
};
static NJS_MKEY_A dat_0cb8b6b4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b6b4.inc"
};
static NJS_MKEY_A dat_0cb8b6d4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b6d4.inc"
};
static NJS_MKEY_A dat_0cb8b6f4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b6f4.inc"
};
static NJS_MKEY_A dat_0cb8b714[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b714.inc"
};
static NJS_MKEY_A dat_0cb8b734[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b734.inc"
};
static NJS_MKEY_A dat_0cb8b754[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b754.inc"
};
static NJS_MKEY_A dat_0cb8b774[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b774.inc"
};
static NJS_MKEY_A dat_0cb8b794[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b794.inc"
};
static NJS_MKEY_A dat_0cb8b7b4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b7b4.inc"
};
static NJS_MKEY_A dat_0cb8b7d4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b7d4.inc"
};
static NJS_MKEY_A dat_0cb8b7f4[14] = {
#include "ev001c/sonic_action_0/dat_0cb8b7f4.inc"
};
static NJS_MKEY_A dat_0cb8b8d4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b8d4.inc"
};
static NJS_MKEY_A dat_0cb8b8f4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8b8f4.inc"
};
static NJS_MKEY_A dat_0cb8b914[15] = {
#include "ev001c/sonic_action_0/dat_0cb8b914.inc"
};
static NJS_MKEY_A dat_0cb8ba04[2] = {
#include "ev001c/sonic_action_0/dat_0cb8ba04.inc"
};
static NJS_MKEY_A dat_0cb8ba24[2] = {
#include "ev001c/sonic_action_0/dat_0cb8ba24.inc"
};
static NJS_MKEY_A dat_0cb8ba44[2] = {
#include "ev001c/sonic_action_0/dat_0cb8ba44.inc"
};
static NJS_MKEY_A dat_0cb8ba64[15] = {
#include "ev001c/sonic_action_0/dat_0cb8ba64.inc"
};
static NJS_MKEY_A dat_0cb8bb54[2] = {
#include "ev001c/sonic_action_0/dat_0cb8bb54.inc"
};
static NJS_MKEY_A dat_0cb8bb74[2] = {
#include "ev001c/sonic_action_0/dat_0cb8bb74.inc"
};
static NJS_MKEY_A dat_0cb8bb94[2] = {
#include "ev001c/sonic_action_0/dat_0cb8bb94.inc"
};
static NJS_MKEY_A dat_0cb8bbb4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8bbb4.inc"
};
static NJS_MKEY_A dat_0cb8bbd4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8bbd4.inc"
};
static NJS_MKEY_A dat_0cb8bbf4[15] = {
#include "ev001c/sonic_action_0/dat_0cb8bbf4.inc"
};
static NJS_MKEY_A dat_0cb8bce4[15] = {
#include "ev001c/sonic_action_0/dat_0cb8bce4.inc"
};
static NJS_MKEY_A dat_0cb8bdd4[15] = {
#include "ev001c/sonic_action_0/dat_0cb8bdd4.inc"
};
static NJS_MKEY_A dat_0cb8bec4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8bec4.inc"
};
static NJS_MKEY_A dat_0cb8bee4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8bee4.inc"
};
static NJS_MKEY_A dat_0cb8bf04[15] = {
#include "ev001c/sonic_action_0/dat_0cb8bf04.inc"
};
static NJS_MKEY_A dat_0cb8bff4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8bff4.inc"
};
static NJS_MKEY_A dat_0cb8c014[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c014.inc"
};
static NJS_MKEY_A dat_0cb8c034[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c034.inc"
};
static NJS_MKEY_A dat_0cb8c054[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c054.inc"
};
static NJS_MKEY_A dat_0cb8c074[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c074.inc"
};
static NJS_MKEY_A dat_0cb8c094[15] = {
#include "ev001c/sonic_action_0/dat_0cb8c094.inc"
};
static NJS_MKEY_A dat_0cb8c184[15] = {
#include "ev001c/sonic_action_0/dat_0cb8c184.inc"
};
static NJS_MKEY_A dat_0cb8c274[15] = {
#include "ev001c/sonic_action_0/dat_0cb8c274.inc"
};
static NJS_MKEY_A dat_0cb8c364[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c364.inc"
};
static NJS_MKEY_A dat_0cb8c384[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c384.inc"
};
static NJS_MKEY_A dat_0cb8c3a4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c3a4.inc"
};
static NJS_MKEY_A dat_0cb8c3c4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c3c4.inc"
};
static NJS_MKEY_A dat_0cb8c3e4[15] = {
#include "ev001c/sonic_action_0/dat_0cb8c3e4.inc"
};
static NJS_MKEY_A dat_0cb8c4d4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c4d4.inc"
};
static NJS_MKEY_A dat_0cb8c4f4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c4f4.inc"
};
static NJS_MKEY_A dat_0cb8c514[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c514.inc"
};
static NJS_MKEY_A dat_0cb8c534[15] = {
#include "ev001c/sonic_action_0/dat_0cb8c534.inc"
};
static NJS_MKEY_A dat_0cb8c624[15] = {
#include "ev001c/sonic_action_0/dat_0cb8c624.inc"
};
static NJS_MKEY_A dat_0cb8c714[15] = {
#include "ev001c/sonic_action_0/dat_0cb8c714.inc"
};
static NJS_MKEY_A dat_0cb8c804[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c804.inc"
};
static NJS_MKEY_A dat_0cb8c824[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c824.inc"
};
static NJS_MKEY_A dat_0cb8c844[15] = {
#include "ev001c/sonic_action_0/dat_0cb8c844.inc"
};
static NJS_MKEY_A dat_0cb8c934[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c934.inc"
};
static NJS_MKEY_A dat_0cb8c954[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c954.inc"
};
static NJS_MKEY_A dat_0cb8c974[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c974.inc"
};
static NJS_MKEY_A dat_0cb8c994[2] = {
#include "ev001c/sonic_action_0/dat_0cb8c994.inc"
};
static NJS_MKEY_A dat_0cb8c9b4[15] = {
#include "ev001c/sonic_action_0/dat_0cb8c9b4.inc"
};
static NJS_MKEY_A dat_0cb8caa4[15] = {
#include "ev001c/sonic_action_0/dat_0cb8caa4.inc"
};
static NJS_MKEY_A dat_0cb8cb94[15] = {
#include "ev001c/sonic_action_0/dat_0cb8cb94.inc"
};
static NJS_MKEY_A dat_0cb8cc84[2] = {
#include "ev001c/sonic_action_0/dat_0cb8cc84.inc"
};
static NJS_MKEY_A dat_0cb8cca4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8cca4.inc"
};
static NJS_MKEY_A dat_0cb8ccc4[15] = {
#include "ev001c/sonic_action_0/dat_0cb8ccc4.inc"
};
static NJS_MKEY_A dat_0cb8cdb4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8cdb4.inc"
};
static NJS_MKEY_A dat_0cb8cdd4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8cdd4.inc"
};
static NJS_MKEY_A dat_0cb8cdf4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8cdf4.inc"
};
static NJS_MKEY_A dat_0cb8ce14[2] = {
#include "ev001c/sonic_action_0/dat_0cb8ce14.inc"
};
static NJS_MKEY_A dat_0cb8ce34[15] = {
#include "ev001c/sonic_action_0/dat_0cb8ce34.inc"
};
static NJS_MKEY_A dat_0cb8cf24[2] = {
#include "ev001c/sonic_action_0/dat_0cb8cf24.inc"
};
static NJS_MKEY_A dat_0cb8cf44[2] = {
#include "ev001c/sonic_action_0/dat_0cb8cf44.inc"
};
static NJS_MKEY_A dat_0cb8cf64[2] = {
#include "ev001c/sonic_action_0/dat_0cb8cf64.inc"
};
static NJS_MKEY_A dat_0cb8cf84[2] = {
#include "ev001c/sonic_action_0/dat_0cb8cf84.inc"
};
static NJS_MKEY_A dat_0cb8cfa4[2] = {
#include "ev001c/sonic_action_0/dat_0cb8cfa4.inc"
};
static NJS_MDATA2 dat_0cb8cfc4[73] = {
#include "ev001c/sonic_action_0/dat_0cb8cfc4.inc"
};
static NJS_MOTION ev001c_sonic_motion_3 = {
#include "ev001c/sonic_action_0/ev001c_sonic_motion_3.inc"
};
NJS_ACTION ev001c_sonic_action_3 = {
#include "ev001c/sonic_action_0/ev001c_sonic_action_3.inc"
};
static NJS_MKEY_F dat_0cb8d468[15] = {
#include "ev001c/sonic_action_0/dat_0cb8d468.inc"
};
static NJS_MKEY_A dat_0cb8d558[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d558.inc"
};
static NJS_MKEY_A dat_0cb8d578[15] = {
#include "ev001c/sonic_action_0/dat_0cb8d578.inc"
};
static NJS_MKEY_A dat_0cb8d668[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d668.inc"
};
static NJS_MKEY_A dat_0cb8d688[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d688.inc"
};
static NJS_MKEY_A dat_0cb8d6a8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d6a8.inc"
};
static NJS_MKEY_A dat_0cb8d6c8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d6c8.inc"
};
static NJS_MKEY_A dat_0cb8d6e8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d6e8.inc"
};
static NJS_MKEY_A dat_0cb8d708[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d708.inc"
};
static NJS_MKEY_A dat_0cb8d728[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d728.inc"
};
static NJS_MKEY_A dat_0cb8d748[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d748.inc"
};
static NJS_MKEY_A dat_0cb8d768[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d768.inc"
};
static NJS_MKEY_A dat_0cb8d788[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d788.inc"
};
static NJS_MKEY_A dat_0cb8d7a8[15] = {
#include "ev001c/sonic_action_0/dat_0cb8d7a8.inc"
};
static NJS_MKEY_A dat_0cb8d898[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d898.inc"
};
static NJS_MKEY_A dat_0cb8d8b8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d8b8.inc"
};
static NJS_MKEY_A dat_0cb8d8d8[15] = {
#include "ev001c/sonic_action_0/dat_0cb8d8d8.inc"
};
static NJS_MKEY_A dat_0cb8d9c8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d9c8.inc"
};
static NJS_MKEY_A dat_0cb8d9e8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8d9e8.inc"
};
static NJS_MKEY_A dat_0cb8da08[2] = {
#include "ev001c/sonic_action_0/dat_0cb8da08.inc"
};
static NJS_MKEY_A dat_0cb8da28[15] = {
#include "ev001c/sonic_action_0/dat_0cb8da28.inc"
};
static NJS_MKEY_A dat_0cb8db18[2] = {
#include "ev001c/sonic_action_0/dat_0cb8db18.inc"
};
static NJS_MKEY_A dat_0cb8db38[2] = {
#include "ev001c/sonic_action_0/dat_0cb8db38.inc"
};
static NJS_MKEY_A dat_0cb8db58[2] = {
#include "ev001c/sonic_action_0/dat_0cb8db58.inc"
};
static NJS_MKEY_A dat_0cb8db78[2] = {
#include "ev001c/sonic_action_0/dat_0cb8db78.inc"
};
static NJS_MKEY_A dat_0cb8db98[2] = {
#include "ev001c/sonic_action_0/dat_0cb8db98.inc"
};
static NJS_MKEY_A dat_0cb8dbb8[15] = {
#include "ev001c/sonic_action_0/dat_0cb8dbb8.inc"
};
static NJS_MKEY_A dat_0cb8dca8[15] = {
#include "ev001c/sonic_action_0/dat_0cb8dca8.inc"
};
static NJS_MKEY_A dat_0cb8dd98[15] = {
#include "ev001c/sonic_action_0/dat_0cb8dd98.inc"
};
static NJS_MKEY_A dat_0cb8de88[2] = {
#include "ev001c/sonic_action_0/dat_0cb8de88.inc"
};
static NJS_MKEY_A dat_0cb8dea8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8dea8.inc"
};
static NJS_MKEY_A dat_0cb8dec8[15] = {
#include "ev001c/sonic_action_0/dat_0cb8dec8.inc"
};
static NJS_MKEY_A dat_0cb8dfb8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8dfb8.inc"
};
static NJS_MKEY_A dat_0cb8dfd8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8dfd8.inc"
};
static NJS_MKEY_A dat_0cb8dff8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8dff8.inc"
};
static NJS_MKEY_A dat_0cb8e018[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e018.inc"
};
static NJS_MKEY_A dat_0cb8e038[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e038.inc"
};
static NJS_MKEY_A dat_0cb8e058[15] = {
#include "ev001c/sonic_action_0/dat_0cb8e058.inc"
};
static NJS_MKEY_A dat_0cb8e148[15] = {
#include "ev001c/sonic_action_0/dat_0cb8e148.inc"
};
static NJS_MKEY_A dat_0cb8e238[15] = {
#include "ev001c/sonic_action_0/dat_0cb8e238.inc"
};
static NJS_MKEY_A dat_0cb8e328[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e328.inc"
};
static NJS_MKEY_A dat_0cb8e348[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e348.inc"
};
static NJS_MKEY_A dat_0cb8e368[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e368.inc"
};
static NJS_MKEY_A dat_0cb8e388[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e388.inc"
};
static NJS_MKEY_A dat_0cb8e3a8[15] = {
#include "ev001c/sonic_action_0/dat_0cb8e3a8.inc"
};
static NJS_MKEY_A dat_0cb8e498[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e498.inc"
};
static NJS_MKEY_A dat_0cb8e4b8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e4b8.inc"
};
static NJS_MKEY_A dat_0cb8e4d8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e4d8.inc"
};
static NJS_MKEY_A dat_0cb8e4f8[15] = {
#include "ev001c/sonic_action_0/dat_0cb8e4f8.inc"
};
static NJS_MKEY_A dat_0cb8e5e8[15] = {
#include "ev001c/sonic_action_0/dat_0cb8e5e8.inc"
};
static NJS_MKEY_A dat_0cb8e6d8[15] = {
#include "ev001c/sonic_action_0/dat_0cb8e6d8.inc"
};
static NJS_MKEY_A dat_0cb8e7c8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e7c8.inc"
};
static NJS_MKEY_A dat_0cb8e7e8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e7e8.inc"
};
static NJS_MKEY_A dat_0cb8e808[15] = {
#include "ev001c/sonic_action_0/dat_0cb8e808.inc"
};
static NJS_MKEY_A dat_0cb8e8f8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e8f8.inc"
};
static NJS_MKEY_A dat_0cb8e918[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e918.inc"
};
static NJS_MKEY_A dat_0cb8e938[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e938.inc"
};
static NJS_MKEY_A dat_0cb8e958[2] = {
#include "ev001c/sonic_action_0/dat_0cb8e958.inc"
};
static NJS_MKEY_A dat_0cb8e978[15] = {
#include "ev001c/sonic_action_0/dat_0cb8e978.inc"
};
static NJS_MKEY_A dat_0cb8ea68[15] = {
#include "ev001c/sonic_action_0/dat_0cb8ea68.inc"
};
static NJS_MKEY_A dat_0cb8eb58[15] = {
#include "ev001c/sonic_action_0/dat_0cb8eb58.inc"
};
static NJS_MKEY_A dat_0cb8ec48[2] = {
#include "ev001c/sonic_action_0/dat_0cb8ec48.inc"
};
static NJS_MKEY_A dat_0cb8ec68[2] = {
#include "ev001c/sonic_action_0/dat_0cb8ec68.inc"
};
static NJS_MKEY_A dat_0cb8ec88[15] = {
#include "ev001c/sonic_action_0/dat_0cb8ec88.inc"
};
static NJS_MKEY_A dat_0cb8ed78[2] = {
#include "ev001c/sonic_action_0/dat_0cb8ed78.inc"
};
static NJS_MKEY_A dat_0cb8ed98[2] = {
#include "ev001c/sonic_action_0/dat_0cb8ed98.inc"
};
static NJS_MKEY_A dat_0cb8edb8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8edb8.inc"
};
static NJS_MKEY_A dat_0cb8edd8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8edd8.inc"
};
static NJS_MKEY_A dat_0cb8edf8[15] = {
#include "ev001c/sonic_action_0/dat_0cb8edf8.inc"
};
static NJS_MKEY_A dat_0cb8eee8[2] = {
#include "ev001c/sonic_action_0/dat_0cb8eee8.inc"
};
static NJS_MKEY_A dat_0cb8ef08[2] = {
#include "ev001c/sonic_action_0/dat_0cb8ef08.inc"
};
static NJS_MKEY_A dat_0cb8ef28[2] = {
#include "ev001c/sonic_action_0/dat_0cb8ef28.inc"
};
static NJS_MKEY_A dat_0cb8ef48[2] = {
#include "ev001c/sonic_action_0/dat_0cb8ef48.inc"
};
static NJS_MKEY_A dat_0cb8ef68[2] = {
#include "ev001c/sonic_action_0/dat_0cb8ef68.inc"
};
static NJS_MDATA2 dat_0cb8ef88[73] = {
#include "ev001c/sonic_action_0/dat_0cb8ef88.inc"
};
static NJS_MOTION ev001c_sonic_motion_4 = {
#include "ev001c/sonic_action_0/ev001c_sonic_motion_4.inc"
};
NJS_ACTION ev001c_sonic_action_4 = {
#include "ev001c/sonic_action_0/ev001c_sonic_action_4.inc"
};
static NJS_MKEY_F dat_0cb8f42c[15] = {
#include "ev001c/sonic_action_0/dat_0cb8f42c.inc"
};
static NJS_MKEY_A dat_0cb8f51c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f51c.inc"
};
static NJS_MKEY_A dat_0cb8f53c[15] = {
#include "ev001c/sonic_action_0/dat_0cb8f53c.inc"
};
static NJS_MKEY_A dat_0cb8f62c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f62c.inc"
};
static NJS_MKEY_A dat_0cb8f64c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f64c.inc"
};
static NJS_MKEY_A dat_0cb8f66c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f66c.inc"
};
static NJS_MKEY_A dat_0cb8f68c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f68c.inc"
};
static NJS_MKEY_A dat_0cb8f6ac[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f6ac.inc"
};
static NJS_MKEY_A dat_0cb8f6cc[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f6cc.inc"
};
static NJS_MKEY_A dat_0cb8f6ec[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f6ec.inc"
};
static NJS_MKEY_A dat_0cb8f70c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f70c.inc"
};
static NJS_MKEY_A dat_0cb8f72c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f72c.inc"
};
static NJS_MKEY_A dat_0cb8f74c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f74c.inc"
};
static NJS_MKEY_A dat_0cb8f76c[14] = {
#include "ev001c/sonic_action_0/dat_0cb8f76c.inc"
};
static NJS_MKEY_A dat_0cb8f84c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f84c.inc"
};
static NJS_MKEY_A dat_0cb8f86c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f86c.inc"
};
static NJS_MKEY_A dat_0cb8f88c[15] = {
#include "ev001c/sonic_action_0/dat_0cb8f88c.inc"
};
static NJS_MKEY_A dat_0cb8f97c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f97c.inc"
};
static NJS_MKEY_A dat_0cb8f99c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f99c.inc"
};
static NJS_MKEY_A dat_0cb8f9bc[2] = {
#include "ev001c/sonic_action_0/dat_0cb8f9bc.inc"
};
static NJS_MKEY_A dat_0cb8f9dc[15] = {
#include "ev001c/sonic_action_0/dat_0cb8f9dc.inc"
};
static NJS_MKEY_A dat_0cb8facc[2] = {
#include "ev001c/sonic_action_0/dat_0cb8facc.inc"
};
static NJS_MKEY_A dat_0cb8faec[2] = {
#include "ev001c/sonic_action_0/dat_0cb8faec.inc"
};
static NJS_MKEY_A dat_0cb8fb0c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8fb0c.inc"
};
static NJS_MKEY_A dat_0cb8fb2c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8fb2c.inc"
};
static NJS_MKEY_A dat_0cb8fb4c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8fb4c.inc"
};
static NJS_MKEY_A dat_0cb8fb6c[15] = {
#include "ev001c/sonic_action_0/dat_0cb8fb6c.inc"
};
static NJS_MKEY_A dat_0cb8fc5c[15] = {
#include "ev001c/sonic_action_0/dat_0cb8fc5c.inc"
};
static NJS_MKEY_A dat_0cb8fd4c[15] = {
#include "ev001c/sonic_action_0/dat_0cb8fd4c.inc"
};
static NJS_MKEY_A dat_0cb8fe3c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8fe3c.inc"
};
static NJS_MKEY_A dat_0cb8fe5c[2] = {
#include "ev001c/sonic_action_0/dat_0cb8fe5c.inc"
};
static NJS_MKEY_A dat_0cb8fe7c[15] = {
#include "ev001c/sonic_action_0/dat_0cb8fe7c.inc"
};
static NJS_MKEY_A dat_0cb8ff6c[9] = {
#include "ev001c/sonic_action_0/dat_0cb8ff6c.inc"
};
static NJS_MKEY_A dat_0cb8fffc[9] = {
#include "ev001c/sonic_action_0/dat_0cb8fffc.inc"
};
static NJS_MKEY_A dat_0cb9008c[2] = {
#include "ev001c/sonic_action_0/dat_0cb9008c.inc"
};
static NJS_MKEY_A dat_0cb900ac[2] = {
#include "ev001c/sonic_action_0/dat_0cb900ac.inc"
};
static NJS_MKEY_A dat_0cb900cc[2] = {
#include "ev001c/sonic_action_0/dat_0cb900cc.inc"
};
static NJS_MKEY_A dat_0cb900ec[15] = {
#include "ev001c/sonic_action_0/dat_0cb900ec.inc"
};
static NJS_MKEY_A dat_0cb901dc[15] = {
#include "ev001c/sonic_action_0/dat_0cb901dc.inc"
};
static NJS_MKEY_A dat_0cb902cc[15] = {
#include "ev001c/sonic_action_0/dat_0cb902cc.inc"
};
static NJS_MKEY_A dat_0cb903bc[2] = {
#include "ev001c/sonic_action_0/dat_0cb903bc.inc"
};
static NJS_MKEY_A dat_0cb903dc[2] = {
#include "ev001c/sonic_action_0/dat_0cb903dc.inc"
};
static NJS_MKEY_A dat_0cb903fc[2] = {
#include "ev001c/sonic_action_0/dat_0cb903fc.inc"
};
static NJS_MKEY_A dat_0cb9041c[2] = {
#include "ev001c/sonic_action_0/dat_0cb9041c.inc"
};
static NJS_MKEY_A dat_0cb9043c[15] = {
#include "ev001c/sonic_action_0/dat_0cb9043c.inc"
};
static NJS_MKEY_A dat_0cb9052c[2] = {
#include "ev001c/sonic_action_0/dat_0cb9052c.inc"
};
static NJS_MKEY_A dat_0cb9054c[2] = {
#include "ev001c/sonic_action_0/dat_0cb9054c.inc"
};
static NJS_MKEY_A dat_0cb9056c[2] = {
#include "ev001c/sonic_action_0/dat_0cb9056c.inc"
};
static NJS_MKEY_A dat_0cb9058c[15] = {
#include "ev001c/sonic_action_0/dat_0cb9058c.inc"
};
static NJS_MKEY_A dat_0cb9067c[15] = {
#include "ev001c/sonic_action_0/dat_0cb9067c.inc"
};
static NJS_MKEY_A dat_0cb9076c[15] = {
#include "ev001c/sonic_action_0/dat_0cb9076c.inc"
};
static NJS_MKEY_A dat_0cb9085c[2] = {
#include "ev001c/sonic_action_0/dat_0cb9085c.inc"
};
static NJS_MKEY_A dat_0cb9087c[2] = {
#include "ev001c/sonic_action_0/dat_0cb9087c.inc"
};
static NJS_MKEY_A dat_0cb9089c[15] = {
#include "ev001c/sonic_action_0/dat_0cb9089c.inc"
};
static NJS_MKEY_A dat_0cb9098c[2] = {
#include "ev001c/sonic_action_0/dat_0cb9098c.inc"
};
static NJS_MKEY_A dat_0cb909ac[2] = {
#include "ev001c/sonic_action_0/dat_0cb909ac.inc"
};
static NJS_MKEY_A dat_0cb909cc[2] = {
#include "ev001c/sonic_action_0/dat_0cb909cc.inc"
};
static NJS_MKEY_A dat_0cb909ec[2] = {
#include "ev001c/sonic_action_0/dat_0cb909ec.inc"
};
static NJS_MKEY_A dat_0cb90a0c[15] = {
#include "ev001c/sonic_action_0/dat_0cb90a0c.inc"
};
static NJS_MKEY_A dat_0cb90afc[15] = {
#include "ev001c/sonic_action_0/dat_0cb90afc.inc"
};
static NJS_MKEY_A dat_0cb90bec[15] = {
#include "ev001c/sonic_action_0/dat_0cb90bec.inc"
};
static NJS_MKEY_A dat_0cb90cdc[2] = {
#include "ev001c/sonic_action_0/dat_0cb90cdc.inc"
};
static NJS_MKEY_A dat_0cb90cfc[2] = {
#include "ev001c/sonic_action_0/dat_0cb90cfc.inc"
};
static NJS_MKEY_A dat_0cb90d1c[15] = {
#include "ev001c/sonic_action_0/dat_0cb90d1c.inc"
};
static NJS_MKEY_A dat_0cb90e0c[2] = {
#include "ev001c/sonic_action_0/dat_0cb90e0c.inc"
};
static NJS_MKEY_A dat_0cb90e2c[2] = {
#include "ev001c/sonic_action_0/dat_0cb90e2c.inc"
};
static NJS_MKEY_A dat_0cb90e4c[2] = {
#include "ev001c/sonic_action_0/dat_0cb90e4c.inc"
};
static NJS_MKEY_A dat_0cb90e6c[2] = {
#include "ev001c/sonic_action_0/dat_0cb90e6c.inc"
};
static NJS_MKEY_A dat_0cb90e8c[15] = {
#include "ev001c/sonic_action_0/dat_0cb90e8c.inc"
};
static NJS_MKEY_A dat_0cb90f7c[2] = {
#include "ev001c/sonic_action_0/dat_0cb90f7c.inc"
};
static NJS_MKEY_A dat_0cb90f9c[2] = {
#include "ev001c/sonic_action_0/dat_0cb90f9c.inc"
};
static NJS_MKEY_A dat_0cb90fbc[2] = {
#include "ev001c/sonic_action_0/dat_0cb90fbc.inc"
};
static NJS_MKEY_A dat_0cb90fdc[2] = {
#include "ev001c/sonic_action_0/dat_0cb90fdc.inc"
};
static NJS_MKEY_A dat_0cb90ffc[2] = {
#include "ev001c/sonic_action_0/dat_0cb90ffc.inc"
};
static NJS_MDATA2 dat_0cb9101c[73] = {
#include "ev001c/sonic_action_0/dat_0cb9101c.inc"
};
static NJS_MOTION ev001c_sonic_motion_5 = {
#include "ev001c/sonic_action_0/ev001c_sonic_motion_5.inc"
};
NJS_ACTION ev001c_sonic_action_5 = {
#include "ev001c/sonic_action_0/ev001c_sonic_action_5.inc"
};
static NJS_MKEY_F dat_0cb914c0[30] = {
#include "ev001c/sonic_action_0/dat_0cb914c0.inc"
};
static NJS_MKEY_A dat_0cb916a0[2] = {
#include "ev001c/sonic_action_0/dat_0cb916a0.inc"
};
static NJS_MKEY_A dat_0cb916c0[30] = {
#include "ev001c/sonic_action_0/dat_0cb916c0.inc"
};
static NJS_MKEY_A dat_0cb918a0[30] = {
#include "ev001c/sonic_action_0/dat_0cb918a0.inc"
};
static NJS_MKEY_A dat_0cb91a80[30] = {
#include "ev001c/sonic_action_0/dat_0cb91a80.inc"
};
static NJS_MKEY_A dat_0cb91c60[2] = {
#include "ev001c/sonic_action_0/dat_0cb91c60.inc"
};
static NJS_MKEY_A dat_0cb91c80[30] = {
#include "ev001c/sonic_action_0/dat_0cb91c80.inc"
};
static NJS_MKEY_A dat_0cb91e60[30] = {
#include "ev001c/sonic_action_0/dat_0cb91e60.inc"
};
static NJS_MKEY_A dat_0cb92040[30] = {
#include "ev001c/sonic_action_0/dat_0cb92040.inc"
};
static NJS_MKEY_A dat_0cb92220[30] = {
#include "ev001c/sonic_action_0/dat_0cb92220.inc"
};
static NJS_MKEY_A dat_0cb92400[30] = {
#include "ev001c/sonic_action_0/dat_0cb92400.inc"
};
static NJS_MKEY_A dat_0cb925e0[30] = {
#include "ev001c/sonic_action_0/dat_0cb925e0.inc"
};
static NJS_MKEY_A dat_0cb927c0[30] = {
#include "ev001c/sonic_action_0/dat_0cb927c0.inc"
};
static NJS_MKEY_A dat_0cb929a0[30] = {
#include "ev001c/sonic_action_0/dat_0cb929a0.inc"
};
static NJS_MKEY_A dat_0cb92b80[30] = {
#include "ev001c/sonic_action_0/dat_0cb92b80.inc"
};
static NJS_MKEY_A dat_0cb92d60[28] = {
#include "ev001c/sonic_action_0/dat_0cb92d60.inc"
};
static NJS_MKEY_A dat_0cb92f20[25] = {
#include "ev001c/sonic_action_0/dat_0cb92f20.inc"
};
static NJS_MKEY_A dat_0cb930b0[2] = {
#include "ev001c/sonic_action_0/dat_0cb930b0.inc"
};
static NJS_MKEY_A dat_0cb930d0[30] = {
#include "ev001c/sonic_action_0/dat_0cb930d0.inc"
};
static NJS_MDATA2 dat_0cb932b0[73] = {
#include "ev001c/sonic_action_0/dat_0cb932b0.inc"
};
static NJS_MOTION ev001c_big_motion_0 = {
#include "ev001c/sonic_action_0/ev001c_big_motion_0.inc"
};
NJS_ACTION ev001c_big_action_0 = {
#include "ev001c/sonic_action_0/ev001c_big_action_0.inc"
};
static NJS_MKEY_F dat_0cb93754[31] = {
#include "ev001c/sonic_action_0/dat_0cb93754.inc"
};
static NJS_MKEY_A dat_0cb93944[40] = {
#include "ev001c/sonic_action_0/dat_0cb93944.inc"
};
static NJS_MKEY_A dat_0cb93bc4[2] = {
#include "ev001c/sonic_action_0/dat_0cb93bc4.inc"
};
static NJS_MKEY_A dat_0cb93be4[40] = {
#include "ev001c/sonic_action_0/dat_0cb93be4.inc"
};
static NJS_MKEY_A dat_0cb93e64[40] = {
#include "ev001c/sonic_action_0/dat_0cb93e64.inc"
};
static NJS_MKEY_A dat_0cb940e4[23] = {
#include "ev001c/sonic_action_0/dat_0cb940e4.inc"
};
static NJS_MKEY_A dat_0cb94254[20] = {
#include "ev001c/sonic_action_0/dat_0cb94254.inc"
};
static NJS_MKEY_A dat_0cb94394[2] = {
#include "ev001c/sonic_action_0/dat_0cb94394.inc"
};
static NJS_MKEY_A dat_0cb943b4[40] = {
#include "ev001c/sonic_action_0/dat_0cb943b4.inc"
};
static NJS_MKEY_A dat_0cb94634[40] = {
#include "ev001c/sonic_action_0/dat_0cb94634.inc"
};
static NJS_MKEY_A dat_0cb948b4[40] = {
#include "ev001c/sonic_action_0/dat_0cb948b4.inc"
};
static NJS_MKEY_A dat_0cb94b34[21] = {
#include "ev001c/sonic_action_0/dat_0cb94b34.inc"
};
static NJS_MKEY_A dat_0cb94c84[34] = {
#include "ev001c/sonic_action_0/dat_0cb94c84.inc"
};
static NJS_MKEY_A dat_0cb94ea4[40] = {
#include "ev001c/sonic_action_0/dat_0cb94ea4.inc"
};
static NJS_MKEY_A dat_0cb95124[40] = {
#include "ev001c/sonic_action_0/dat_0cb95124.inc"
};
static NJS_MKEY_A dat_0cb953a4[34] = {
#include "ev001c/sonic_action_0/dat_0cb953a4.inc"
};
static NJS_MKEY_A dat_0cb955c4[40] = {
#include "ev001c/sonic_action_0/dat_0cb955c4.inc"
};
static NJS_MKEY_A dat_0cb95844[40] = {
#include "ev001c/sonic_action_0/dat_0cb95844.inc"
};
static NJS_MKEY_A dat_0cb95ac4[2] = {
#include "ev001c/sonic_action_0/dat_0cb95ac4.inc"
};
static NJS_MKEY_A dat_0cb95ae4[31] = {
#include "ev001c/sonic_action_0/dat_0cb95ae4.inc"
};
static NJS_MDATA2 dat_0cb95cd4[73] = {
#include "ev001c/sonic_action_0/dat_0cb95cd4.inc"
};
static NJS_MOTION ev001c_big_motion_1 = {
#include "ev001c/sonic_action_0/ev001c_big_motion_1.inc"
};
NJS_ACTION ev001c_big_action_1 = {
#include "ev001c/sonic_action_0/ev001c_big_action_1.inc"
};
static NJS_MKEY_F dat_0cb96178[30] = {
#include "ev001c/sonic_action_0/dat_0cb96178.inc"
};
static NJS_MKEY_A dat_0cb96358[2] = {
#include "ev001c/sonic_action_0/dat_0cb96358.inc"
};
static NJS_MKEY_A dat_0cb96378[2] = {
#include "ev001c/sonic_action_0/dat_0cb96378.inc"
};
static NJS_MKEY_A dat_0cb96398[30] = {
#include "ev001c/sonic_action_0/dat_0cb96398.inc"
};
static NJS_MKEY_A dat_0cb96578[30] = {
#include "ev001c/sonic_action_0/dat_0cb96578.inc"
};
static NJS_MKEY_A dat_0cb96758[30] = {
#include "ev001c/sonic_action_0/dat_0cb96758.inc"
};
static NJS_MKEY_A dat_0cb96938[2] = {
#include "ev001c/sonic_action_0/dat_0cb96938.inc"
};
static NJS_MKEY_A dat_0cb96958[30] = {
#include "ev001c/sonic_action_0/dat_0cb96958.inc"
};
static NJS_MKEY_A dat_0cb96b38[30] = {
#include "ev001c/sonic_action_0/dat_0cb96b38.inc"
};
static NJS_MKEY_A dat_0cb96d18[30] = {
#include "ev001c/sonic_action_0/dat_0cb96d18.inc"
};
static NJS_MKEY_A dat_0cb96ef8[30] = {
#include "ev001c/sonic_action_0/dat_0cb96ef8.inc"
};
static NJS_MKEY_A dat_0cb970d8[29] = {
#include "ev001c/sonic_action_0/dat_0cb970d8.inc"
};
static NJS_MKEY_A dat_0cb972a8[30] = {
#include "ev001c/sonic_action_0/dat_0cb972a8.inc"
};
static NJS_MKEY_A dat_0cb97488[29] = {
#include "ev001c/sonic_action_0/dat_0cb97488.inc"
};
static NJS_MKEY_A dat_0cb97658[29] = {
#include "ev001c/sonic_action_0/dat_0cb97658.inc"
};
static NJS_MKEY_A dat_0cb97828[30] = {
#include "ev001c/sonic_action_0/dat_0cb97828.inc"
};
static NJS_MKEY_A dat_0cb97a08[29] = {
#include "ev001c/sonic_action_0/dat_0cb97a08.inc"
};
static NJS_MKEY_A dat_0cb97bd8[29] = {
#include "ev001c/sonic_action_0/dat_0cb97bd8.inc"
};
static NJS_MKEY_A dat_0cb97da8[30] = {
#include "ev001c/sonic_action_0/dat_0cb97da8.inc"
};
static NJS_MKEY_A dat_0cb97f88[2] = {
#include "ev001c/sonic_action_0/dat_0cb97f88.inc"
};
static NJS_MKEY_A dat_0cb97fa8[30] = {
#include "ev001c/sonic_action_0/dat_0cb97fa8.inc"
};
static NJS_MDATA2 dat_0cb98188[73] = {
#include "ev001c/sonic_action_0/dat_0cb98188.inc"
};
static NJS_MOTION ev001c_big_motion_2 = {
#include "ev001c/sonic_action_0/ev001c_big_motion_2.inc"
};
NJS_ACTION ev001c_big_action_2 = {
#include "ev001c/sonic_action_0/ev001c_big_action_2.inc"
};
/* the names of Froggy's textures. no word in EV001C or 1ST_READ.BIN points
 * at their texlist, dat_0cb9868c */
static NJS_TEXNAME dat_0cb9862c[8] = {
    {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0}, {TEXT_5, 0, 0},
    {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0}, {TEXT_9, 0, 0},
};
static NJS_TEXLIST dat_0cb9868c = {
#include "ev001c/sonic_action_0/dat_0cb9868c.inc"
};
/* the material of the model at 0x0cb9873c, one of the models in Froggy's
 * tree, which ev001c_frog_action moves. that model lies in the generated
 * data after this object */
static NJS_MATERIAL dat_0cb98694[1] = {
#include "ev001c/sonic_action_0/dat_0cb98694.inc"
};
