/* Eggman's texlist, and the models and texlists of the clods, the sky, the
 * barrier and the jets */
#include <ninja.h>

#include "b_egm1/effect_textures.h"
#include <sg_xpt.h>

#include "b_egm1/texlists.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 5
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 5
#define TEXT_13_LENGTH 6
#define TEXT_14_LENGTH 6
#define TEXT_15_LENGTH 13
#define TEXT_16_LENGTH 13
#define TEXT_17_LENGTH 20
#define TEXT_18_LENGTH 18
#define TEXT_19_LENGTH 18
#define TEXT_20_LENGTH 13
#define TEXT_21_LENGTH 11
#include "b_egm1/effect_textures/text.inc"

static NJS_TEXNAME egm1eggman_textures[14] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
};
NJS_TEXLIST egm1eggman_texlist = {
#include "b_egm1/effect_textures/egm1eggman_texlist.inc"
};
static NJS_MATERIAL dat_0c914690[1] = {
#include "b_egm1/effect_textures/dat_0c914690.inc"
};
static Sint16 dat_0c9146a4[78] = {
#include "b_egm1/effect_textures/dat_0c9146a4.inc"
};
static NJS_COLOR dat_0c914740[62] = {
#include "b_egm1/effect_textures/dat_0c914740.inc"
};
static NJS_MESHSET dat_0c914838[1] = {
#include "b_egm1/effect_textures/dat_0c914838.inc"
};
static NJS_POINT3 dat_0c914850[17] = {
#include "b_egm1/effect_textures/dat_0c914850.inc"
};
static NJS_VECTOR dat_0c91491c[17] = {
#include "b_egm1/effect_textures/dat_0c91491c.inc"
};
static NJS_MODEL dat_0c9149e8 = {
#include "b_egm1/effect_textures/dat_0c9149e8.inc"
};
NJS_OBJECT clod_a_object = {
#include "b_egm1/effect_textures/clod_a_object.inc"
};
static NJS_MATERIAL dat_0c914a44[1] = {
#include "b_egm1/effect_textures/dat_0c914a44.inc"
};
static Sint16 dat_0c914a58[82] = {
#include "b_egm1/effect_textures/dat_0c914a58.inc"
};
static NJS_COLOR dat_0c914afc[64] = {
#include "b_egm1/effect_textures/dat_0c914afc.inc"
};
static NJS_MESHSET dat_0c914bfc[1] = {
#include "b_egm1/effect_textures/dat_0c914bfc.inc"
};
static NJS_POINT3 dat_0c914c14[17] = {
#include "b_egm1/effect_textures/dat_0c914c14.inc"
};
static NJS_VECTOR dat_0c914ce0[17] = {
#include "b_egm1/effect_textures/dat_0c914ce0.inc"
};
static NJS_MODEL dat_0c914dac = {
#include "b_egm1/effect_textures/dat_0c914dac.inc"
};
NJS_OBJECT clod_b_object = {
#include "b_egm1/effect_textures/clod_b_object.inc"
};
static NJS_TEXNAME dat_0c914e08[2] = {
    {TEXT_15, 0, 0},
    {TEXT_16, 0, 0},
};
NJS_TEXLIST egm1tsuchi_texlist = {
#include "b_egm1/effect_textures/egm1tsuchi_texlist.inc"
};
static NJS_MATERIAL dat_0c914e28[3] = {
#include "b_egm1/effect_textures/dat_0c914e28.inc"
};
static Sint16 dat_0c914e64[35] = {
#include "b_egm1/effect_textures/dat_0c914e64.inc"
};
static Sint16 dat_0c914eaa[175] = {
#include "b_egm1/effect_textures/dat_0c914eaa.inc"
};
static Sint16 dat_0c915008[34] = {
#include "b_egm1/effect_textures/dat_0c915008.inc"
};
static NJS_COLOR dat_0c91504c[34] = {
#include "b_egm1/effect_textures/dat_0c91504c.inc"
};
static NJS_COLOR dat_0c9150d4[170] = {
#include "b_egm1/effect_textures/dat_0c9150d4.inc"
};
static NJS_COLOR dat_0c91537c[28] = {
#include "b_egm1/effect_textures/dat_0c91537c.inc"
};
static NJS_MESHSET dat_0c9153ec[3] = {
#include "b_egm1/effect_textures/dat_0c9153ec.inc"
};
static NJS_POINT3 dat_0c915434[113] = {
#include "b_egm1/effect_textures/dat_0c915434.inc"
};
static NJS_VECTOR dat_0c915980[113] = {
#include "b_egm1/effect_textures/dat_0c915980.inc"
};
static NJS_MODEL dat_0c915ecc = {
#include "b_egm1/effect_textures/dat_0c915ecc.inc"
};
NJS_OBJECT eggmobile1_sky_object = {
#include "b_egm1/effect_textures/eggmobile1_sky_object.inc"
};
static NJS_TEXNAME dat_0c915f28[3] = {
    {TEXT_17, 0, 0},
    {TEXT_18, 0, 0},
    {TEXT_19, 0, 0},
};
NJS_TEXLIST eggmobile1_sky_texlist = {
#include "b_egm1/effect_textures/eggmobile1_sky_texlist.inc"
};
static NJS_TEXNAME dat_0c915f54[1] = {
    {TEXT_20, 0, 0},
};
static NJS_TEXLIST dat_0c915f60 = {
#include "b_egm1/effect_textures/dat_0c915f60.inc"
};
static NJS_MATERIAL dat_0c915f68[1] = {
#include "b_egm1/effect_textures/dat_0c915f68.inc"
};
static Sint16 dat_0c915f7c[116] = {
#include "b_egm1/effect_textures/dat_0c915f7c.inc"
};
static NJS_COLOR dat_0c916064[96] = {
#include "b_egm1/effect_textures/dat_0c916064.inc"
};
static NJS_MESHSET dat_0c9161e4[1] = {
#include "b_egm1/effect_textures/dat_0c9161e4.inc"
};
static NJS_POINT3 dat_0c9161fc[33] = {
#include "b_egm1/effect_textures/dat_0c9161fc.inc"
};
static NJS_VECTOR dat_0c916388[33] = {
#include "b_egm1/effect_textures/dat_0c916388.inc"
};
static NJS_MODEL dat_0c916514 = {
#include "b_egm1/effect_textures/dat_0c916514.inc"
};
NJS_OBJECT egg_hornet_barrier_object = {
#include "b_egm1/effect_textures/egg_hornet_barrier_object.inc"
};
static NJS_TEXNAME dat_0c916570[1] = {
    {TEXT_21, 0, 0},
};
static NJS_TEXLIST dat_0c91657c = {
#include "b_egm1/effect_textures/dat_0c91657c.inc"
};
static NJS_MATERIAL dat_0c916584[1] = {
#include "b_egm1/effect_textures/dat_0c916584.inc"
};
static Sint16 dat_0c916598[40] = {
#include "b_egm1/effect_textures/dat_0c916598.inc"
};
static NJS_COLOR dat_0c9165e8[32] = {
#include "b_egm1/effect_textures/dat_0c9165e8.inc"
};
static NJS_MESHSET dat_0c916668[1] = {
#include "b_egm1/effect_textures/dat_0c916668.inc"
};
static NJS_POINT3 dat_0c916680[16] = {
#include "b_egm1/effect_textures/dat_0c916680.inc"
};
static NJS_VECTOR dat_0c916740[16] = {
#include "b_egm1/effect_textures/dat_0c916740.inc"
};
static NJS_MODEL dat_0c916800 = {
#include "b_egm1/effect_textures/dat_0c916800.inc"
};
NJS_OBJECT jet_ring_object = {
#include "b_egm1/effect_textures/jet_ring_object.inc"
};
static NJS_TEXNAME dat_0c91685c[1] = {
    {TEXT_21, 0, 0},
};
static NJS_TEXLIST dat_0c916868 = {
#include "b_egm1/effect_textures/dat_0c916868.inc"
};
static NJS_MATERIAL dat_0c916870[1] = {
#include "b_egm1/effect_textures/dat_0c916870.inc"
};
static Sint16 dat_0c916884[20] = {
#include "b_egm1/effect_textures/dat_0c916884.inc"
};
static NJS_COLOR dat_0c9168ac[18] = {
#include "b_egm1/effect_textures/dat_0c9168ac.inc"
};
static NJS_MESHSET dat_0c9168f4[1] = {
#include "b_egm1/effect_textures/dat_0c9168f4.inc"
};
static NJS_POINT3 dat_0c91690c[16] = {
#include "b_egm1/effect_textures/dat_0c91690c.inc"
};
static NJS_VECTOR dat_0c9169cc[16] = {
#include "b_egm1/effect_textures/dat_0c9169cc.inc"
};
static NJS_MODEL dat_0c916a8c = {
#include "b_egm1/effect_textures/dat_0c916a8c.inc"
};
NJS_OBJECT jet_cone_object = {
#include "b_egm1/effect_textures/jet_cone_object.inc"
};
static NJS_TEXNAME dat_0c916ae8[1] = {
    {TEXT_21, 0, 0},
};
static NJS_TEXLIST dat_0c916af4 = {
#include "b_egm1/effect_textures/dat_0c916af4.inc"
};
static NJS_MATERIAL dat_0c916afc[1] = {
#include "b_egm1/effect_textures/dat_0c916afc.inc"
};
static Sint16 dat_0c916b10[30] = {
#include "b_egm1/effect_textures/dat_0c916b10.inc"
};
static NJS_COLOR dat_0c916b4c[24] = {
#include "b_egm1/effect_textures/dat_0c916b4c.inc"
};
static NJS_MESHSET dat_0c916bac[1] = {
#include "b_egm1/effect_textures/dat_0c916bac.inc"
};
static NJS_POINT3 dat_0c916bc4[12] = {
#include "b_egm1/effect_textures/dat_0c916bc4.inc"
};
static NJS_VECTOR dat_0c916c54[12] = {
#include "b_egm1/effect_textures/dat_0c916c54.inc"
};
static NJS_MODEL dat_0c916ce4 = {
#include "b_egm1/effect_textures/dat_0c916ce4.inc"
};
NJS_OBJECT jet_plus_x_object = {
#include "b_egm1/effect_textures/jet_plus_x_object.inc"
};
static NJS_TEXNAME dat_0c916d40[1] = {
    {TEXT_21, 0, 0},
};
static NJS_TEXLIST dat_0c916d4c = {
#include "b_egm1/effect_textures/dat_0c916d4c.inc"
};
static NJS_MATERIAL dat_0c916d54[1] = {
#include "b_egm1/effect_textures/dat_0c916d54.inc"
};
static Sint16 dat_0c916d68[30] = {
#include "b_egm1/effect_textures/dat_0c916d68.inc"
};
static NJS_COLOR dat_0c916da4[24] = {
#include "b_egm1/effect_textures/dat_0c916da4.inc"
};
static NJS_MESHSET dat_0c916e04[1] = {
#include "b_egm1/effect_textures/dat_0c916e04.inc"
};
static NJS_POINT3 dat_0c916e1c[12] = {
#include "b_egm1/effect_textures/dat_0c916e1c.inc"
};
static NJS_VECTOR dat_0c916eac[12] = {
#include "b_egm1/effect_textures/dat_0c916eac.inc"
};
static NJS_MODEL dat_0c916f3c = {
#include "b_egm1/effect_textures/dat_0c916f3c.inc"
};
NJS_OBJECT jet_minus_x_object = {
#include "b_egm1/effect_textures/jet_minus_x_object.inc"
};
static NJS_TEXNAME dat_0c916f98[1] = {
    {TEXT_21, 0, 0},
};
static NJS_TEXLIST dat_0c916fa4 = {
#include "b_egm1/effect_textures/dat_0c916fa4.inc"
};
static NJS_MATERIAL dat_0c916fac[1] = {
#include "b_egm1/effect_textures/dat_0c916fac.inc"
};
static Sint16 dat_0c916fc0[20] = {
#include "b_egm1/effect_textures/dat_0c916fc0.inc"
};
static NJS_COLOR dat_0c916fe8[18] = {
#include "b_egm1/effect_textures/dat_0c916fe8.inc"
};
static NJS_MESHSET dat_0c917030[1] = {
#include "b_egm1/effect_textures/dat_0c917030.inc"
};
static NJS_POINT3 dat_0c917048[16] = {
#include "b_egm1/effect_textures/dat_0c917048.inc"
};
static NJS_VECTOR dat_0c917108[16] = {
#include "b_egm1/effect_textures/dat_0c917108.inc"
};
static NJS_MODEL dat_0c9171c8 = {
#include "b_egm1/effect_textures/dat_0c9171c8.inc"
};
NJS_OBJECT jet_core_object = {
#include "b_egm1/effect_textures/jet_core_object.inc"
};
static NJS_TEXNAME dat_0c917224[1] = {
    {TEXT_21, 0, 0},
};
static NJS_TEXLIST dat_0c917230 = {
#include "b_egm1/effect_textures/dat_0c917230.inc"
};
static NJS_MATERIAL dat_0c917238[1] = {
#include "b_egm1/effect_textures/dat_0c917238.inc"
};
static Sint16 dat_0c91724c[40] = {
#include "b_egm1/effect_textures/dat_0c91724c.inc"
};
static NJS_COLOR dat_0c91729c[32] = {
#include "b_egm1/effect_textures/dat_0c91729c.inc"
};
static NJS_MESHSET dat_0c91731c[1] = {
#include "b_egm1/effect_textures/dat_0c91731c.inc"
};
static NJS_POINT3 dat_0c917334[16] = {
#include "b_egm1/effect_textures/dat_0c917334.inc"
};
static NJS_VECTOR dat_0c9173f4[16] = {
#include "b_egm1/effect_textures/dat_0c9173f4.inc"
};
static NJS_MODEL dat_0c9174b4 = {
#include "b_egm1/effect_textures/dat_0c9174b4.inc"
};
NJS_OBJECT jet_ring_long_object = {
#include "b_egm1/effect_textures/jet_ring_long_object.inc"
};
static NJS_TEXNAME dat_0c917510[1] = {
    {TEXT_21, 0, 0},
};
static NJS_TEXLIST dat_0c91751c = {
#include "b_egm1/effect_textures/dat_0c91751c.inc"
};
static NJS_MATERIAL dat_0c917524[1] = {
#include "b_egm1/effect_textures/dat_0c917524.inc"
};
static Sint16 dat_0c917538[20] = {
#include "b_egm1/effect_textures/dat_0c917538.inc"
};
static NJS_COLOR dat_0c917560[18] = {
#include "b_egm1/effect_textures/dat_0c917560.inc"
};
static NJS_MESHSET dat_0c9175a8[1] = {
#include "b_egm1/effect_textures/dat_0c9175a8.inc"
};
static NJS_POINT3 dat_0c9175c0[16] = {
#include "b_egm1/effect_textures/dat_0c9175c0.inc"
};
static NJS_VECTOR dat_0c917680[16] = {
#include "b_egm1/effect_textures/dat_0c917680.inc"
};
static NJS_MODEL dat_0c917740 = {
#include "b_egm1/effect_textures/dat_0c917740.inc"
};
NJS_OBJECT jet_cone_long_object = {
#include "b_egm1/effect_textures/jet_cone_long_object.inc"
};
static NJS_TEXNAME dat_0c91779c[1] = {
    {TEXT_21, 0, 0},
};
static NJS_TEXLIST dat_0c9177a8 = {
#include "b_egm1/effect_textures/dat_0c9177a8.inc"
};
static NJS_MATERIAL dat_0c9177b0[1] = {
#include "b_egm1/effect_textures/dat_0c9177b0.inc"
};
static Sint16 dat_0c9177c4[30] = {
#include "b_egm1/effect_textures/dat_0c9177c4.inc"
};
static NJS_COLOR dat_0c917800[24] = {
#include "b_egm1/effect_textures/dat_0c917800.inc"
};
static NJS_MESHSET dat_0c917860[1] = {
#include "b_egm1/effect_textures/dat_0c917860.inc"
};
static NJS_POINT3 dat_0c917878[12] = {
#include "b_egm1/effect_textures/dat_0c917878.inc"
};
static NJS_VECTOR dat_0c917908[12] = {
#include "b_egm1/effect_textures/dat_0c917908.inc"
};
static NJS_MODEL dat_0c917998 = {
#include "b_egm1/effect_textures/dat_0c917998.inc"
};
NJS_OBJECT jet_plus_x_long_object = {
#include "b_egm1/effect_textures/jet_plus_x_long_object.inc"
};
static NJS_TEXNAME dat_0c9179f4[1] = {
    {TEXT_21, 0, 0},
};
static NJS_TEXLIST dat_0c917a00 = {
#include "b_egm1/effect_textures/dat_0c917a00.inc"
};
static NJS_MATERIAL dat_0c917a08[1] = {
#include "b_egm1/effect_textures/dat_0c917a08.inc"
};
static Sint16 dat_0c917a1c[30] = {
#include "b_egm1/effect_textures/dat_0c917a1c.inc"
};
static NJS_COLOR dat_0c917a58[24] = {
#include "b_egm1/effect_textures/dat_0c917a58.inc"
};
static NJS_MESHSET dat_0c917ab8[1] = {
#include "b_egm1/effect_textures/dat_0c917ab8.inc"
};
static NJS_POINT3 dat_0c917ad0[12] = {
#include "b_egm1/effect_textures/dat_0c917ad0.inc"
};
static NJS_VECTOR dat_0c917b60[12] = {
#include "b_egm1/effect_textures/dat_0c917b60.inc"
};
static NJS_MODEL dat_0c917bf0 = {
#include "b_egm1/effect_textures/dat_0c917bf0.inc"
};
NJS_OBJECT jet_minus_x_long_object = {
#include "b_egm1/effect_textures/jet_minus_x_long_object.inc"
};
static NJS_TEXNAME dat_0c917c4c[1] = {
    {TEXT_21, 0, 0},
};
static NJS_TEXLIST dat_0c917c58 = {
#include "b_egm1/effect_textures/dat_0c917c58.inc"
};
