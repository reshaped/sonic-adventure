/* the bird's two texture lists and a model of the bird between them open the
 * object. the bird's four actions and the Egg Mobile's four follow.
 * ev0069_bird_action_0 and ev0069_bird_action_3 move the model here, whose root
 * is dat_0cb9fe20. ev0069_bird_action_1 and ev0069_bird_action_2 move
 * dat_0cb9bcd4, a model in the data before this object. dat_0cb9bcd4 matches
 * the model here but for five nodes. it scales those to 1e-5, and their models'
 * radii shrink with them. the script draws the bird with ev0069_bird_texlist_2
 * alone. only ev0069_textures points at ev0069_bird_texlist_1, which the event
 * loads from VER1_WING. the object ends with ev0069_emerald_textures, the
 * texture name of ev0069_emerald_texlist. that list lies in the data after this
 * object */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0069/bird_textures_1.h"

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
#define TEXT_13_LENGTH 14
#include "ev0069/bird_textures_1/text.inc"

extern NJS_OBJECT dat_0cb9bcd4;
extern NJS_OBJECT event_eggmobile_object;

static NJS_TEXNAME dat_0cb9bd08[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0069_bird_texlist_1 = {
#include "ev0069/bird_textures_1/ev0069_bird_texlist_1.inc"
};
static NJS_MATERIAL dat_0cb9bda0[1] = {
#include "ev0069/bird_textures_1/dat_0cb9bda0.inc"
};
static Sint16 dat_0cb9bdb4[6] = {
#include "ev0069/bird_textures_1/dat_0cb9bdb4.inc"
};
static NJS_COLOR dat_0cb9bdc0[4] = {
#include "ev0069/bird_textures_1/dat_0cb9bdc0.inc"
};
static NJS_MESHSET dat_0cb9bdd0[1] = {
#include "ev0069/bird_textures_1/dat_0cb9bdd0.inc"
};
static NJS_POINT3 dat_0cb9bde8[4] = {
#include "ev0069/bird_textures_1/dat_0cb9bde8.inc"
};
static NJS_VECTOR dat_0cb9be18[4] = {
#include "ev0069/bird_textures_1/dat_0cb9be18.inc"
};
static NJS_MODEL dat_0cb9be48 = {
#include "ev0069/bird_textures_1/dat_0cb9be48.inc"
};
static NJS_OBJECT dat_0cb9be70 = {
#include "ev0069/bird_textures_1/dat_0cb9be70.inc"
};
static NJS_MATERIAL dat_0cb9bea4[1] = {
#include "ev0069/bird_textures_1/dat_0cb9bea4.inc"
};
static Sint16 dat_0cb9beb8[12] = {
#include "ev0069/bird_textures_1/dat_0cb9beb8.inc"
};
static NJS_COLOR dat_0cb9bed0[9] = {
#include "ev0069/bird_textures_1/dat_0cb9bed0.inc"
};
static NJS_MESHSET dat_0cb9bef4[1] = {
#include "ev0069/bird_textures_1/dat_0cb9bef4.inc"
};
static NJS_POINT3 dat_0cb9bf0c[7] = {
#include "ev0069/bird_textures_1/dat_0cb9bf0c.inc"
};
static NJS_VECTOR dat_0cb9bf60[7] = {
#include "ev0069/bird_textures_1/dat_0cb9bf60.inc"
};
static NJS_MODEL dat_0cb9bfb4 = {
#include "ev0069/bird_textures_1/dat_0cb9bfb4.inc"
};
static NJS_OBJECT dat_0cb9bfdc = {
#include "ev0069/bird_textures_1/dat_0cb9bfdc.inc"
};
static NJS_OBJECT dat_0cb9c010 = {
#include "ev0069/bird_textures_1/dat_0cb9c010.inc"
};
static NJS_OBJECT dat_0cb9c044 = {
#include "ev0069/bird_textures_1/dat_0cb9c044.inc"
};
static NJS_OBJECT dat_0cb9c078 = {
#include "ev0069/bird_textures_1/dat_0cb9c078.inc"
};
static NJS_OBJECT dat_0cb9c0ac = {
#include "ev0069/bird_textures_1/dat_0cb9c0ac.inc"
};
static NJS_OBJECT dat_0cb9c0e0 = {
#include "ev0069/bird_textures_1/dat_0cb9c0e0.inc"
};
static NJS_OBJECT dat_0cb9c114 = {
#include "ev0069/bird_textures_1/dat_0cb9c114.inc"
};
static NJS_MATERIAL dat_0cb9c148[1] = {
#include "ev0069/bird_textures_1/dat_0cb9c148.inc"
};
static Sint16 dat_0cb9c15c[6] = {
#include "ev0069/bird_textures_1/dat_0cb9c15c.inc"
};
static NJS_COLOR dat_0cb9c168[4] = {
#include "ev0069/bird_textures_1/dat_0cb9c168.inc"
};
static NJS_MESHSET dat_0cb9c178[1] = {
#include "ev0069/bird_textures_1/dat_0cb9c178.inc"
};
static NJS_POINT3 dat_0cb9c190[4] = {
#include "ev0069/bird_textures_1/dat_0cb9c190.inc"
};
static NJS_VECTOR dat_0cb9c1c0[4] = {
#include "ev0069/bird_textures_1/dat_0cb9c1c0.inc"
};
static NJS_MODEL dat_0cb9c1f0 = {
#include "ev0069/bird_textures_1/dat_0cb9c1f0.inc"
};
static NJS_OBJECT dat_0cb9c218 = {
#include "ev0069/bird_textures_1/dat_0cb9c218.inc"
};
static NJS_MATERIAL dat_0cb9c24c[1] = {
#include "ev0069/bird_textures_1/dat_0cb9c24c.inc"
};
static Sint16 dat_0cb9c260[12] = {
#include "ev0069/bird_textures_1/dat_0cb9c260.inc"
};
static NJS_COLOR dat_0cb9c278[9] = {
#include "ev0069/bird_textures_1/dat_0cb9c278.inc"
};
static NJS_MESHSET dat_0cb9c29c[1] = {
#include "ev0069/bird_textures_1/dat_0cb9c29c.inc"
};
static NJS_POINT3 dat_0cb9c2b4[7] = {
#include "ev0069/bird_textures_1/dat_0cb9c2b4.inc"
};
static NJS_VECTOR dat_0cb9c308[7] = {
#include "ev0069/bird_textures_1/dat_0cb9c308.inc"
};
static NJS_MODEL dat_0cb9c35c = {
#include "ev0069/bird_textures_1/dat_0cb9c35c.inc"
};
static NJS_OBJECT dat_0cb9c384 = {
#include "ev0069/bird_textures_1/dat_0cb9c384.inc"
};
static NJS_OBJECT dat_0cb9c3b8 = {
#include "ev0069/bird_textures_1/dat_0cb9c3b8.inc"
};
static NJS_OBJECT dat_0cb9c3ec = {
#include "ev0069/bird_textures_1/dat_0cb9c3ec.inc"
};
static NJS_OBJECT dat_0cb9c420 = {
#include "ev0069/bird_textures_1/dat_0cb9c420.inc"
};
static NJS_OBJECT dat_0cb9c454 = {
#include "ev0069/bird_textures_1/dat_0cb9c454.inc"
};
static NJS_OBJECT dat_0cb9c488 = {
#include "ev0069/bird_textures_1/dat_0cb9c488.inc"
};
static NJS_OBJECT dat_0cb9c4bc = {
#include "ev0069/bird_textures_1/dat_0cb9c4bc.inc"
};
static NJS_OBJECT dat_0cb9c4f0 = {
#include "ev0069/bird_textures_1/dat_0cb9c4f0.inc"
};
static NJS_OBJECT dat_0cb9c524 = {
#include "ev0069/bird_textures_1/dat_0cb9c524.inc"
};
static NJS_OBJECT dat_0cb9c558 = {
#include "ev0069/bird_textures_1/dat_0cb9c558.inc"
};
static NJS_OBJECT dat_0cb9c58c = {
#include "ev0069/bird_textures_1/dat_0cb9c58c.inc"
};
static NJS_OBJECT dat_0cb9c5c0 = {
#include "ev0069/bird_textures_1/dat_0cb9c5c0.inc"
};
static NJS_MATERIAL dat_0cb9c5f4[1] = {
#include "ev0069/bird_textures_1/dat_0cb9c5f4.inc"
};
static Sint16 dat_0cb9c608[18] = {
#include "ev0069/bird_textures_1/dat_0cb9c608.inc"
};
static NJS_COLOR dat_0cb9c62c[15] = {
#include "ev0069/bird_textures_1/dat_0cb9c62c.inc"
};
static NJS_MESHSET dat_0cb9c668[1] = {
#include "ev0069/bird_textures_1/dat_0cb9c668.inc"
};
static NJS_POINT3 dat_0cb9c680[7] = {
#include "ev0069/bird_textures_1/dat_0cb9c680.inc"
};
static NJS_VECTOR dat_0cb9c6d4[7] = {
#include "ev0069/bird_textures_1/dat_0cb9c6d4.inc"
};
static NJS_MODEL dat_0cb9c728 = {
#include "ev0069/bird_textures_1/dat_0cb9c728.inc"
};
static NJS_OBJECT dat_0cb9c750 = {
#include "ev0069/bird_textures_1/dat_0cb9c750.inc"
};
static NJS_OBJECT dat_0cb9c784 = {
#include "ev0069/bird_textures_1/dat_0cb9c784.inc"
};
static NJS_OBJECT dat_0cb9c7b8 = {
#include "ev0069/bird_textures_1/dat_0cb9c7b8.inc"
};
static NJS_OBJECT dat_0cb9c7ec = {
#include "ev0069/bird_textures_1/dat_0cb9c7ec.inc"
};
static NJS_MATERIAL dat_0cb9c820[1] = {
#include "ev0069/bird_textures_1/dat_0cb9c820.inc"
};
static Sint16 dat_0cb9c834[10] = {
#include "ev0069/bird_textures_1/dat_0cb9c834.inc"
};
static NJS_COLOR dat_0cb9c848[8] = {
#include "ev0069/bird_textures_1/dat_0cb9c848.inc"
};
static NJS_MESHSET dat_0cb9c868[1] = {
#include "ev0069/bird_textures_1/dat_0cb9c868.inc"
};
static NJS_POINT3 dat_0cb9c880[8] = {
#include "ev0069/bird_textures_1/dat_0cb9c880.inc"
};
static NJS_VECTOR dat_0cb9c8e0[8] = {
#include "ev0069/bird_textures_1/dat_0cb9c8e0.inc"
};
static NJS_MODEL dat_0cb9c940 = {
#include "ev0069/bird_textures_1/dat_0cb9c940.inc"
};
static NJS_OBJECT dat_0cb9c968 = {
#include "ev0069/bird_textures_1/dat_0cb9c968.inc"
};
static NJS_MATERIAL dat_0cb9c99c[1] = {
#include "ev0069/bird_textures_1/dat_0cb9c99c.inc"
};
static Sint16 dat_0cb9c9b0[10] = {
#include "ev0069/bird_textures_1/dat_0cb9c9b0.inc"
};
static NJS_COLOR dat_0cb9c9c4[8] = {
#include "ev0069/bird_textures_1/dat_0cb9c9c4.inc"
};
static NJS_MESHSET dat_0cb9c9e4[1] = {
#include "ev0069/bird_textures_1/dat_0cb9c9e4.inc"
};
static NJS_POINT3 dat_0cb9c9fc[8] = {
#include "ev0069/bird_textures_1/dat_0cb9c9fc.inc"
};
static NJS_VECTOR dat_0cb9ca5c[8] = {
#include "ev0069/bird_textures_1/dat_0cb9ca5c.inc"
};
static NJS_MODEL dat_0cb9cabc = {
#include "ev0069/bird_textures_1/dat_0cb9cabc.inc"
};
static NJS_OBJECT dat_0cb9cae4 = {
#include "ev0069/bird_textures_1/dat_0cb9cae4.inc"
};
static NJS_MATERIAL dat_0cb9cb18[1] = {
#include "ev0069/bird_textures_1/dat_0cb9cb18.inc"
};
static Sint16 dat_0cb9cb2c[16] = {
#include "ev0069/bird_textures_1/dat_0cb9cb2c.inc"
};
static NJS_COLOR dat_0cb9cb4c[12] = {
#include "ev0069/bird_textures_1/dat_0cb9cb4c.inc"
};
static NJS_MESHSET dat_0cb9cb7c[1] = {
#include "ev0069/bird_textures_1/dat_0cb9cb7c.inc"
};
static NJS_POINT3 dat_0cb9cb94[12] = {
#include "ev0069/bird_textures_1/dat_0cb9cb94.inc"
};
static NJS_VECTOR dat_0cb9cc24[12] = {
#include "ev0069/bird_textures_1/dat_0cb9cc24.inc"
};
static NJS_MODEL dat_0cb9ccb4 = {
#include "ev0069/bird_textures_1/dat_0cb9ccb4.inc"
};
static NJS_OBJECT dat_0cb9ccdc = {
#include "ev0069/bird_textures_1/dat_0cb9ccdc.inc"
};
static NJS_MATERIAL dat_0cb9cd10[1] = {
#include "ev0069/bird_textures_1/dat_0cb9cd10.inc"
};
static Sint16 dat_0cb9cd24[16] = {
#include "ev0069/bird_textures_1/dat_0cb9cd24.inc"
};
static NJS_COLOR dat_0cb9cd44[12] = {
#include "ev0069/bird_textures_1/dat_0cb9cd44.inc"
};
static NJS_MESHSET dat_0cb9cd74[1] = {
#include "ev0069/bird_textures_1/dat_0cb9cd74.inc"
};
static NJS_POINT3 dat_0cb9cd8c[12] = {
#include "ev0069/bird_textures_1/dat_0cb9cd8c.inc"
};
static NJS_VECTOR dat_0cb9ce1c[12] = {
#include "ev0069/bird_textures_1/dat_0cb9ce1c.inc"
};
static NJS_MODEL dat_0cb9ceac = {
#include "ev0069/bird_textures_1/dat_0cb9ceac.inc"
};
static NJS_OBJECT dat_0cb9ced4 = {
#include "ev0069/bird_textures_1/dat_0cb9ced4.inc"
};
static NJS_MATERIAL dat_0cb9cf08[4] = {
#include "ev0069/bird_textures_1/dat_0cb9cf08.inc"
};
static Sint16 dat_0cb9cf58[239] = {
#include "ev0069/bird_textures_1/dat_0cb9cf58.inc"
};
static Sint16 dat_0cb9d136[22] = {
#include "ev0069/bird_textures_1/dat_0cb9d136.inc"
};
static Sint16 dat_0cb9d162[21] = {
#include "ev0069/bird_textures_1/dat_0cb9d162.inc"
};
static Sint16 dat_0cb9d18c[22] = {
#include "ev0069/bird_textures_1/dat_0cb9d18c.inc"
};
static NJS_COLOR dat_0cb9d1b8[209] = {
#include "ev0069/bird_textures_1/dat_0cb9d1b8.inc"
};
static NJS_COLOR dat_0cb9d4fc[20] = {
#include "ev0069/bird_textures_1/dat_0cb9d4fc.inc"
};
static NJS_COLOR dat_0cb9d54c[16] = {
#include "ev0069/bird_textures_1/dat_0cb9d54c.inc"
};
static NJS_COLOR dat_0cb9d58c[18] = {
#include "ev0069/bird_textures_1/dat_0cb9d58c.inc"
};
static NJS_MESHSET dat_0cb9d5d4[4] = {
#include "ev0069/bird_textures_1/dat_0cb9d5d4.inc"
};
static NJS_POINT3 dat_0cb9d634[127] = {
#include "ev0069/bird_textures_1/dat_0cb9d634.inc"
};
static NJS_VECTOR dat_0cb9dc28[127] = {
#include "ev0069/bird_textures_1/dat_0cb9dc28.inc"
};
static NJS_MODEL dat_0cb9e21c = {
#include "ev0069/bird_textures_1/dat_0cb9e21c.inc"
};
static NJS_OBJECT dat_0cb9e244 = {
#include "ev0069/bird_textures_1/dat_0cb9e244.inc"
};
static NJS_OBJECT dat_0cb9e278 = {
#include "ev0069/bird_textures_1/dat_0cb9e278.inc"
};
static NJS_OBJECT dat_0cb9e2ac = {
#include "ev0069/bird_textures_1/dat_0cb9e2ac.inc"
};
static NJS_OBJECT dat_0cb9e2e0 = {
#include "ev0069/bird_textures_1/dat_0cb9e2e0.inc"
};
static NJS_MATERIAL dat_0cb9e314[1] = {
#include "ev0069/bird_textures_1/dat_0cb9e314.inc"
};
static Sint16 dat_0cb9e328[8] = {
#include "ev0069/bird_textures_1/dat_0cb9e328.inc"
};
static NJS_COLOR dat_0cb9e338[6] = {
#include "ev0069/bird_textures_1/dat_0cb9e338.inc"
};
static NJS_MESHSET dat_0cb9e350[1] = {
#include "ev0069/bird_textures_1/dat_0cb9e350.inc"
};
static NJS_POINT3 dat_0cb9e368[6] = {
#include "ev0069/bird_textures_1/dat_0cb9e368.inc"
};
static NJS_VECTOR dat_0cb9e3b0[6] = {
#include "ev0069/bird_textures_1/dat_0cb9e3b0.inc"
};
static NJS_MODEL dat_0cb9e3f8 = {
#include "ev0069/bird_textures_1/dat_0cb9e3f8.inc"
};
static NJS_OBJECT dat_0cb9e420 = {
#include "ev0069/bird_textures_1/dat_0cb9e420.inc"
};
static NJS_MATERIAL dat_0cb9e454[1] = {
#include "ev0069/bird_textures_1/dat_0cb9e454.inc"
};
static Sint16 dat_0cb9e468[22] = {
#include "ev0069/bird_textures_1/dat_0cb9e468.inc"
};
static NJS_COLOR dat_0cb9e494[16] = {
#include "ev0069/bird_textures_1/dat_0cb9e494.inc"
};
static NJS_MESHSET dat_0cb9e4d4[1] = {
#include "ev0069/bird_textures_1/dat_0cb9e4d4.inc"
};
static NJS_POINT3 dat_0cb9e4ec[5] = {
#include "ev0069/bird_textures_1/dat_0cb9e4ec.inc"
};
static NJS_VECTOR dat_0cb9e528[5] = {
#include "ev0069/bird_textures_1/dat_0cb9e528.inc"
};
static NJS_MODEL dat_0cb9e564 = {
#include "ev0069/bird_textures_1/dat_0cb9e564.inc"
};
static NJS_OBJECT dat_0cb9e58c = {
#include "ev0069/bird_textures_1/dat_0cb9e58c.inc"
};
static NJS_MATERIAL dat_0cb9e5c0[1] = {
#include "ev0069/bird_textures_1/dat_0cb9e5c0.inc"
};
static Sint16 dat_0cb9e5d4[30] = {
#include "ev0069/bird_textures_1/dat_0cb9e5d4.inc"
};
static NJS_COLOR dat_0cb9e610[28] = {
#include "ev0069/bird_textures_1/dat_0cb9e610.inc"
};
static NJS_MESHSET dat_0cb9e680[1] = {
#include "ev0069/bird_textures_1/dat_0cb9e680.inc"
};
static NJS_POINT3 dat_0cb9e698[18] = {
#include "ev0069/bird_textures_1/dat_0cb9e698.inc"
};
static NJS_VECTOR dat_0cb9e770[18] = {
#include "ev0069/bird_textures_1/dat_0cb9e770.inc"
};
static NJS_MODEL dat_0cb9e848 = {
#include "ev0069/bird_textures_1/dat_0cb9e848.inc"
};
static NJS_OBJECT dat_0cb9e870 = {
#include "ev0069/bird_textures_1/dat_0cb9e870.inc"
};
static NJS_MATERIAL dat_0cb9e8a4[1] = {
#include "ev0069/bird_textures_1/dat_0cb9e8a4.inc"
};
static Sint16 dat_0cb9e8b8[20] = {
#include "ev0069/bird_textures_1/dat_0cb9e8b8.inc"
};
static NJS_COLOR dat_0cb9e8e0[16] = {
#include "ev0069/bird_textures_1/dat_0cb9e8e0.inc"
};
static NJS_MESHSET dat_0cb9e920[1] = {
#include "ev0069/bird_textures_1/dat_0cb9e920.inc"
};
static NJS_POINT3 dat_0cb9e938[12] = {
#include "ev0069/bird_textures_1/dat_0cb9e938.inc"
};
static NJS_VECTOR dat_0cb9e9c8[12] = {
#include "ev0069/bird_textures_1/dat_0cb9e9c8.inc"
};
static NJS_MODEL dat_0cb9ea58 = {
#include "ev0069/bird_textures_1/dat_0cb9ea58.inc"
};
static NJS_OBJECT dat_0cb9ea80 = {
#include "ev0069/bird_textures_1/dat_0cb9ea80.inc"
};
static NJS_MATERIAL dat_0cb9eab4[1] = {
#include "ev0069/bird_textures_1/dat_0cb9eab4.inc"
};
static Sint16 dat_0cb9eac8[10] = {
#include "ev0069/bird_textures_1/dat_0cb9eac8.inc"
};
static NJS_COLOR dat_0cb9eadc[8] = {
#include "ev0069/bird_textures_1/dat_0cb9eadc.inc"
};
static NJS_MESHSET dat_0cb9eafc[1] = {
#include "ev0069/bird_textures_1/dat_0cb9eafc.inc"
};
static NJS_POINT3 dat_0cb9eb14[5] = {
#include "ev0069/bird_textures_1/dat_0cb9eb14.inc"
};
static NJS_VECTOR dat_0cb9eb50[5] = {
#include "ev0069/bird_textures_1/dat_0cb9eb50.inc"
};
static NJS_MODEL dat_0cb9eb8c = {
#include "ev0069/bird_textures_1/dat_0cb9eb8c.inc"
};
static NJS_OBJECT dat_0cb9ebb4 = {
#include "ev0069/bird_textures_1/dat_0cb9ebb4.inc"
};
static NJS_MATERIAL dat_0cb9ebe8[1] = {
#include "ev0069/bird_textures_1/dat_0cb9ebe8.inc"
};
static Sint16 dat_0cb9ebfc[10] = {
#include "ev0069/bird_textures_1/dat_0cb9ebfc.inc"
};
static NJS_COLOR dat_0cb9ec10[8] = {
#include "ev0069/bird_textures_1/dat_0cb9ec10.inc"
};
static NJS_MESHSET dat_0cb9ec30[1] = {
#include "ev0069/bird_textures_1/dat_0cb9ec30.inc"
};
static NJS_POINT3 dat_0cb9ec48[5] = {
#include "ev0069/bird_textures_1/dat_0cb9ec48.inc"
};
static NJS_VECTOR dat_0cb9ec84[5] = {
#include "ev0069/bird_textures_1/dat_0cb9ec84.inc"
};
static NJS_MODEL dat_0cb9ecc0 = {
#include "ev0069/bird_textures_1/dat_0cb9ecc0.inc"
};
static NJS_OBJECT dat_0cb9ece8 = {
#include "ev0069/bird_textures_1/dat_0cb9ece8.inc"
};
static NJS_MATERIAL dat_0cb9ed1c[1] = {
#include "ev0069/bird_textures_1/dat_0cb9ed1c.inc"
};
static Sint16 dat_0cb9ed30[24] = {
#include "ev0069/bird_textures_1/dat_0cb9ed30.inc"
};
static NJS_COLOR dat_0cb9ed60[20] = {
#include "ev0069/bird_textures_1/dat_0cb9ed60.inc"
};
static NJS_MESHSET dat_0cb9edb0[1] = {
#include "ev0069/bird_textures_1/dat_0cb9edb0.inc"
};
static NJS_POINT3 dat_0cb9edc8[12] = {
#include "ev0069/bird_textures_1/dat_0cb9edc8.inc"
};
static NJS_VECTOR dat_0cb9ee58[12] = {
#include "ev0069/bird_textures_1/dat_0cb9ee58.inc"
};
static NJS_MODEL dat_0cb9eee8 = {
#include "ev0069/bird_textures_1/dat_0cb9eee8.inc"
};
static NJS_OBJECT dat_0cb9ef10 = {
#include "ev0069/bird_textures_1/dat_0cb9ef10.inc"
};
static NJS_MATERIAL dat_0cb9ef44[1] = {
#include "ev0069/bird_textures_1/dat_0cb9ef44.inc"
};
static Sint16 dat_0cb9ef58[36] = {
#include "ev0069/bird_textures_1/dat_0cb9ef58.inc"
};
static NJS_COLOR dat_0cb9efa0[30] = {
#include "ev0069/bird_textures_1/dat_0cb9efa0.inc"
};
static NJS_MESHSET dat_0cb9f018[1] = {
#include "ev0069/bird_textures_1/dat_0cb9f018.inc"
};
static NJS_POINT3 dat_0cb9f030[12] = {
#include "ev0069/bird_textures_1/dat_0cb9f030.inc"
};
static NJS_VECTOR dat_0cb9f0c0[12] = {
#include "ev0069/bird_textures_1/dat_0cb9f0c0.inc"
};
static NJS_MODEL dat_0cb9f150 = {
#include "ev0069/bird_textures_1/dat_0cb9f150.inc"
};
static NJS_OBJECT dat_0cb9f178 = {
#include "ev0069/bird_textures_1/dat_0cb9f178.inc"
};
static NJS_OBJECT dat_0cb9f1ac = {
#include "ev0069/bird_textures_1/dat_0cb9f1ac.inc"
};
static NJS_OBJECT dat_0cb9f1e0 = {
#include "ev0069/bird_textures_1/dat_0cb9f1e0.inc"
};
static NJS_OBJECT dat_0cb9f214 = {
#include "ev0069/bird_textures_1/dat_0cb9f214.inc"
};
static NJS_MATERIAL dat_0cb9f248[1] = {
#include "ev0069/bird_textures_1/dat_0cb9f248.inc"
};
static Sint16 dat_0cb9f25c[48] = {
#include "ev0069/bird_textures_1/dat_0cb9f25c.inc"
};
static NJS_COLOR dat_0cb9f2bc[38] = {
#include "ev0069/bird_textures_1/dat_0cb9f2bc.inc"
};
static NJS_MESHSET dat_0cb9f354[1] = {
#include "ev0069/bird_textures_1/dat_0cb9f354.inc"
};
static NJS_POINT3 dat_0cb9f36c[12] = {
#include "ev0069/bird_textures_1/dat_0cb9f36c.inc"
};
static NJS_VECTOR dat_0cb9f3fc[12] = {
#include "ev0069/bird_textures_1/dat_0cb9f3fc.inc"
};
static NJS_MODEL dat_0cb9f48c = {
#include "ev0069/bird_textures_1/dat_0cb9f48c.inc"
};
static NJS_OBJECT dat_0cb9f4b4 = {
#include "ev0069/bird_textures_1/dat_0cb9f4b4.inc"
};
static NJS_OBJECT dat_0cb9f4e8 = {
#include "ev0069/bird_textures_1/dat_0cb9f4e8.inc"
};
static NJS_OBJECT dat_0cb9f51c = {
#include "ev0069/bird_textures_1/dat_0cb9f51c.inc"
};
static NJS_OBJECT dat_0cb9f550 = {
#include "ev0069/bird_textures_1/dat_0cb9f550.inc"
};
static NJS_MATERIAL dat_0cb9f584[1] = {
#include "ev0069/bird_textures_1/dat_0cb9f584.inc"
};
static Sint16 dat_0cb9f598[48] = {
#include "ev0069/bird_textures_1/dat_0cb9f598.inc"
};
static NJS_COLOR dat_0cb9f5f8[38] = {
#include "ev0069/bird_textures_1/dat_0cb9f5f8.inc"
};
static NJS_MESHSET dat_0cb9f690[1] = {
#include "ev0069/bird_textures_1/dat_0cb9f690.inc"
};
static NJS_POINT3 dat_0cb9f6a8[12] = {
#include "ev0069/bird_textures_1/dat_0cb9f6a8.inc"
};
static NJS_VECTOR dat_0cb9f738[12] = {
#include "ev0069/bird_textures_1/dat_0cb9f738.inc"
};
static NJS_MODEL dat_0cb9f7c8 = {
#include "ev0069/bird_textures_1/dat_0cb9f7c8.inc"
};
static NJS_OBJECT dat_0cb9f7f0 = {
#include "ev0069/bird_textures_1/dat_0cb9f7f0.inc"
};
static NJS_OBJECT dat_0cb9f824 = {
#include "ev0069/bird_textures_1/dat_0cb9f824.inc"
};
static NJS_OBJECT dat_0cb9f858 = {
#include "ev0069/bird_textures_1/dat_0cb9f858.inc"
};
static NJS_OBJECT dat_0cb9f88c = {
#include "ev0069/bird_textures_1/dat_0cb9f88c.inc"
};
static NJS_MATERIAL dat_0cb9f8c0[2] = {
#include "ev0069/bird_textures_1/dat_0cb9f8c0.inc"
};
static Sint16 dat_0cb9f8e8[54] = {
#include "ev0069/bird_textures_1/dat_0cb9f8e8.inc"
};
static Sint16 dat_0cb9f954[22] = {
#include "ev0069/bird_textures_1/dat_0cb9f954.inc"
};
static NJS_COLOR dat_0cb9f980[48] = {
#include "ev0069/bird_textures_1/dat_0cb9f980.inc"
};
static NJS_COLOR dat_0cb9fa40[18] = {
#include "ev0069/bird_textures_1/dat_0cb9fa40.inc"
};
static NJS_MESHSET dat_0cb9fa88[2] = {
#include "ev0069/bird_textures_1/dat_0cb9fa88.inc"
};
static NJS_POINT3 dat_0cb9fab8[26] = {
#include "ev0069/bird_textures_1/dat_0cb9fab8.inc"
};
static NJS_VECTOR dat_0cb9fbf0[26] = {
#include "ev0069/bird_textures_1/dat_0cb9fbf0.inc"
};
static NJS_MODEL dat_0cb9fd28 = {
#include "ev0069/bird_textures_1/dat_0cb9fd28.inc"
};
static NJS_OBJECT dat_0cb9fd50 = {
#include "ev0069/bird_textures_1/dat_0cb9fd50.inc"
};
static NJS_OBJECT dat_0cb9fd84 = {
#include "ev0069/bird_textures_1/dat_0cb9fd84.inc"
};
static NJS_OBJECT dat_0cb9fdb8 = {
#include "ev0069/bird_textures_1/dat_0cb9fdb8.inc"
};
static NJS_OBJECT dat_0cb9fdec = {
#include "ev0069/bird_textures_1/dat_0cb9fdec.inc"
};
static NJS_OBJECT dat_0cb9fe20 = {
#include "ev0069/bird_textures_1/dat_0cb9fe20.inc"
};
static NJS_TEXNAME dat_0cb9fe54[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0069_bird_texlist_2 = {
#include "ev0069/bird_textures_1/ev0069_bird_texlist_2.inc"
};
static NJS_MKEY_F dat_0cb9feec[32] = {
#include "ev0069/bird_textures_1/dat_0cb9feec.inc"
};
static NJS_MKEY_F dat_0cba00ec[32] = {
#include "ev0069/bird_textures_1/dat_0cba00ec.inc"
};
static NJS_MKEY_A dat_0cba02ec[32] = {
#include "ev0069/bird_textures_1/dat_0cba02ec.inc"
};
static NJS_MKEY_A dat_0cba04ec[2] = {
#include "ev0069/bird_textures_1/dat_0cba04ec.inc"
};
static NJS_MKEY_F dat_0cba050c[2] = {
#include "ev0069/bird_textures_1/dat_0cba050c.inc"
};
static NJS_MKEY_F dat_0cba052c[2] = {
#include "ev0069/bird_textures_1/dat_0cba052c.inc"
};
static NJS_MKEY_A dat_0cba054c[2] = {
#include "ev0069/bird_textures_1/dat_0cba054c.inc"
};
static NJS_MKEY_A dat_0cba056c[2] = {
#include "ev0069/bird_textures_1/dat_0cba056c.inc"
};
static NJS_MKEY_F dat_0cba058c[2] = {
#include "ev0069/bird_textures_1/dat_0cba058c.inc"
};
static NJS_MKEY_F dat_0cba05ac[2] = {
#include "ev0069/bird_textures_1/dat_0cba05ac.inc"
};
static NJS_MKEY_A dat_0cba05cc[2] = {
#include "ev0069/bird_textures_1/dat_0cba05cc.inc"
};
static NJS_MKEY_A dat_0cba05ec[2] = {
#include "ev0069/bird_textures_1/dat_0cba05ec.inc"
};
static NJS_MKEY_F dat_0cba060c[2] = {
#include "ev0069/bird_textures_1/dat_0cba060c.inc"
};
static NJS_MKEY_F dat_0cba062c[2] = {
#include "ev0069/bird_textures_1/dat_0cba062c.inc"
};
static NJS_MKEY_A dat_0cba064c[2] = {
#include "ev0069/bird_textures_1/dat_0cba064c.inc"
};
static NJS_MKEY_A dat_0cba066c[2] = {
#include "ev0069/bird_textures_1/dat_0cba066c.inc"
};
static NJS_MKEY_F dat_0cba068c[2] = {
#include "ev0069/bird_textures_1/dat_0cba068c.inc"
};
static NJS_MKEY_F dat_0cba06ac[2] = {
#include "ev0069/bird_textures_1/dat_0cba06ac.inc"
};
static NJS_MKEY_A dat_0cba06cc[2] = {
#include "ev0069/bird_textures_1/dat_0cba06cc.inc"
};
static NJS_MKEY_F dat_0cba06ec[2] = {
#include "ev0069/bird_textures_1/dat_0cba06ec.inc"
};
static NJS_MKEY_A dat_0cba070c[2] = {
#include "ev0069/bird_textures_1/dat_0cba070c.inc"
};
static NJS_MKEY_F dat_0cba072c[2] = {
#include "ev0069/bird_textures_1/dat_0cba072c.inc"
};
static NJS_MKEY_A dat_0cba074c[2] = {
#include "ev0069/bird_textures_1/dat_0cba074c.inc"
};
static NJS_MKEY_F dat_0cba076c[2] = {
#include "ev0069/bird_textures_1/dat_0cba076c.inc"
};
static NJS_MKEY_A dat_0cba078c[2] = {
#include "ev0069/bird_textures_1/dat_0cba078c.inc"
};
static NJS_MKEY_A dat_0cba07ac[2] = {
#include "ev0069/bird_textures_1/dat_0cba07ac.inc"
};
static NJS_MKEY_F dat_0cba07cc[2] = {
#include "ev0069/bird_textures_1/dat_0cba07cc.inc"
};
static NJS_MKEY_F dat_0cba07ec[2] = {
#include "ev0069/bird_textures_1/dat_0cba07ec.inc"
};
static NJS_MKEY_A dat_0cba080c[2] = {
#include "ev0069/bird_textures_1/dat_0cba080c.inc"
};
static NJS_MKEY_F dat_0cba082c[2] = {
#include "ev0069/bird_textures_1/dat_0cba082c.inc"
};
static NJS_MKEY_A dat_0cba084c[2] = {
#include "ev0069/bird_textures_1/dat_0cba084c.inc"
};
static NJS_MKEY_F dat_0cba086c[2] = {
#include "ev0069/bird_textures_1/dat_0cba086c.inc"
};
static NJS_MKEY_A dat_0cba088c[2] = {
#include "ev0069/bird_textures_1/dat_0cba088c.inc"
};
static NJS_MKEY_F dat_0cba08ac[2] = {
#include "ev0069/bird_textures_1/dat_0cba08ac.inc"
};
static NJS_MKEY_A dat_0cba08cc[2] = {
#include "ev0069/bird_textures_1/dat_0cba08cc.inc"
};
static NJS_MKEY_F dat_0cba08ec[2] = {
#include "ev0069/bird_textures_1/dat_0cba08ec.inc"
};
static NJS_MKEY_A dat_0cba090c[2] = {
#include "ev0069/bird_textures_1/dat_0cba090c.inc"
};
static NJS_MKEY_A dat_0cba092c[2] = {
#include "ev0069/bird_textures_1/dat_0cba092c.inc"
};
static NJS_MKEY_F dat_0cba094c[2] = {
#include "ev0069/bird_textures_1/dat_0cba094c.inc"
};
static NJS_MKEY_F dat_0cba096c[2] = {
#include "ev0069/bird_textures_1/dat_0cba096c.inc"
};
static NJS_MKEY_A dat_0cba098c[2] = {
#include "ev0069/bird_textures_1/dat_0cba098c.inc"
};
static NJS_MKEY_F dat_0cba09ac[2] = {
#include "ev0069/bird_textures_1/dat_0cba09ac.inc"
};
static NJS_MKEY_A dat_0cba09cc[2] = {
#include "ev0069/bird_textures_1/dat_0cba09cc.inc"
};
static NJS_MKEY_F dat_0cba09ec[2] = {
#include "ev0069/bird_textures_1/dat_0cba09ec.inc"
};
static NJS_MKEY_A dat_0cba0a0c[2] = {
#include "ev0069/bird_textures_1/dat_0cba0a0c.inc"
};
static NJS_MKEY_F dat_0cba0a2c[2] = {
#include "ev0069/bird_textures_1/dat_0cba0a2c.inc"
};
static NJS_MKEY_A dat_0cba0a4c[2] = {
#include "ev0069/bird_textures_1/dat_0cba0a4c.inc"
};
static NJS_MKEY_F dat_0cba0a6c[2] = {
#include "ev0069/bird_textures_1/dat_0cba0a6c.inc"
};
static NJS_MKEY_A dat_0cba0a8c[2] = {
#include "ev0069/bird_textures_1/dat_0cba0a8c.inc"
};
static NJS_MKEY_F dat_0cba0aac[2] = {
#include "ev0069/bird_textures_1/dat_0cba0aac.inc"
};
static NJS_MKEY_A dat_0cba0acc[2] = {
#include "ev0069/bird_textures_1/dat_0cba0acc.inc"
};
static NJS_MKEY_F dat_0cba0aec[2] = {
#include "ev0069/bird_textures_1/dat_0cba0aec.inc"
};
static NJS_MKEY_A dat_0cba0b0c[2] = {
#include "ev0069/bird_textures_1/dat_0cba0b0c.inc"
};
static NJS_MKEY_F dat_0cba0b2c[2] = {
#include "ev0069/bird_textures_1/dat_0cba0b2c.inc"
};
static NJS_MKEY_F dat_0cba0b4c[2] = {
#include "ev0069/bird_textures_1/dat_0cba0b4c.inc"
};
static NJS_MKEY_F dat_0cba0b6c[2] = {
#include "ev0069/bird_textures_1/dat_0cba0b6c.inc"
};
static NJS_MKEY_A dat_0cba0b8c[2] = {
#include "ev0069/bird_textures_1/dat_0cba0b8c.inc"
};
static NJS_MKEY_F dat_0cba0bac[2] = {
#include "ev0069/bird_textures_1/dat_0cba0bac.inc"
};
static NJS_MKEY_A dat_0cba0bcc[2] = {
#include "ev0069/bird_textures_1/dat_0cba0bcc.inc"
};
static NJS_MKEY_F dat_0cba0bec[2] = {
#include "ev0069/bird_textures_1/dat_0cba0bec.inc"
};
static NJS_MDATA2 dat_0cba0c0c[57] = {
#include "ev0069/bird_textures_1/dat_0cba0c0c.inc"
};
static NJS_MOTION ev0069_bird_motion_0 = {
#include "ev0069/bird_textures_1/ev0069_bird_motion_0.inc"
};
NJS_ACTION ev0069_bird_action_0 = {
#include "ev0069/bird_textures_1/ev0069_bird_action_0.inc"
};
static NJS_MKEY_F dat_0cba0fb0[2] = {
#include "ev0069/bird_textures_1/dat_0cba0fb0.inc"
};
static NJS_MKEY_A dat_0cba0fd0[2] = {
#include "ev0069/bird_textures_1/dat_0cba0fd0.inc"
};
static NJS_MKEY_F dat_0cba0ff0[31] = {
#include "ev0069/bird_textures_1/dat_0cba0ff0.inc"
};
static NJS_MKEY_A dat_0cba11e0[30] = {
#include "ev0069/bird_textures_1/dat_0cba11e0.inc"
};
static NJS_MKEY_F dat_0cba13c0[2] = {
#include "ev0069/bird_textures_1/dat_0cba13c0.inc"
};
static NJS_MKEY_A dat_0cba13e0[2] = {
#include "ev0069/bird_textures_1/dat_0cba13e0.inc"
};
static NJS_MKEY_F dat_0cba1400[2] = {
#include "ev0069/bird_textures_1/dat_0cba1400.inc"
};
static NJS_MKEY_A dat_0cba1420[2] = {
#include "ev0069/bird_textures_1/dat_0cba1420.inc"
};
static NJS_MKEY_F dat_0cba1440[2] = {
#include "ev0069/bird_textures_1/dat_0cba1440.inc"
};
static NJS_MKEY_A dat_0cba1460[2] = {
#include "ev0069/bird_textures_1/dat_0cba1460.inc"
};
static NJS_MKEY_F dat_0cba1480[2] = {
#include "ev0069/bird_textures_1/dat_0cba1480.inc"
};
static NJS_MKEY_A dat_0cba14a0[2] = {
#include "ev0069/bird_textures_1/dat_0cba14a0.inc"
};
static NJS_MKEY_F dat_0cba14c0[2] = {
#include "ev0069/bird_textures_1/dat_0cba14c0.inc"
};
static NJS_MKEY_A dat_0cba14e0[31] = {
#include "ev0069/bird_textures_1/dat_0cba14e0.inc"
};
static NJS_MKEY_F dat_0cba16d0[2] = {
#include "ev0069/bird_textures_1/dat_0cba16d0.inc"
};
static NJS_MKEY_A dat_0cba16f0[2] = {
#include "ev0069/bird_textures_1/dat_0cba16f0.inc"
};
static NJS_MKEY_F dat_0cba1710[2] = {
#include "ev0069/bird_textures_1/dat_0cba1710.inc"
};
static NJS_MKEY_A dat_0cba1730[2] = {
#include "ev0069/bird_textures_1/dat_0cba1730.inc"
};
static NJS_MKEY_F dat_0cba1750[2] = {
#include "ev0069/bird_textures_1/dat_0cba1750.inc"
};
static NJS_MKEY_A dat_0cba1770[2] = {
#include "ev0069/bird_textures_1/dat_0cba1770.inc"
};
static NJS_MKEY_F dat_0cba1790[2] = {
#include "ev0069/bird_textures_1/dat_0cba1790.inc"
};
static NJS_MKEY_A dat_0cba17b0[31] = {
#include "ev0069/bird_textures_1/dat_0cba17b0.inc"
};
static NJS_MKEY_F dat_0cba19a0[2] = {
#include "ev0069/bird_textures_1/dat_0cba19a0.inc"
};
static NJS_MKEY_A dat_0cba19c0[2] = {
#include "ev0069/bird_textures_1/dat_0cba19c0.inc"
};
static NJS_MKEY_F dat_0cba19e0[2] = {
#include "ev0069/bird_textures_1/dat_0cba19e0.inc"
};
static NJS_MKEY_A dat_0cba1a00[2] = {
#include "ev0069/bird_textures_1/dat_0cba1a00.inc"
};
static NJS_MKEY_F dat_0cba1a20[2] = {
#include "ev0069/bird_textures_1/dat_0cba1a20.inc"
};
static NJS_MKEY_A dat_0cba1a40[2] = {
#include "ev0069/bird_textures_1/dat_0cba1a40.inc"
};
static NJS_MKEY_F dat_0cba1a60[2] = {
#include "ev0069/bird_textures_1/dat_0cba1a60.inc"
};
static NJS_MKEY_A dat_0cba1a80[2] = {
#include "ev0069/bird_textures_1/dat_0cba1a80.inc"
};
static NJS_MKEY_F dat_0cba1aa0[2] = {
#include "ev0069/bird_textures_1/dat_0cba1aa0.inc"
};
static NJS_MKEY_A dat_0cba1ac0[2] = {
#include "ev0069/bird_textures_1/dat_0cba1ac0.inc"
};
static NJS_MKEY_F dat_0cba1ae0[2] = {
#include "ev0069/bird_textures_1/dat_0cba1ae0.inc"
};
static NJS_MKEY_A dat_0cba1b00[2] = {
#include "ev0069/bird_textures_1/dat_0cba1b00.inc"
};
static NJS_MKEY_F dat_0cba1b20[2] = {
#include "ev0069/bird_textures_1/dat_0cba1b20.inc"
};
static NJS_MKEY_A dat_0cba1b40[2] = {
#include "ev0069/bird_textures_1/dat_0cba1b40.inc"
};
static NJS_MKEY_F dat_0cba1b60[2] = {
#include "ev0069/bird_textures_1/dat_0cba1b60.inc"
};
static NJS_MKEY_A dat_0cba1b80[2] = {
#include "ev0069/bird_textures_1/dat_0cba1b80.inc"
};
static NJS_MKEY_F dat_0cba1ba0[2] = {
#include "ev0069/bird_textures_1/dat_0cba1ba0.inc"
};
static NJS_MKEY_A dat_0cba1bc0[2] = {
#include "ev0069/bird_textures_1/dat_0cba1bc0.inc"
};
static NJS_MKEY_F dat_0cba1be0[2] = {
#include "ev0069/bird_textures_1/dat_0cba1be0.inc"
};
static NJS_MKEY_A dat_0cba1c00[2] = {
#include "ev0069/bird_textures_1/dat_0cba1c00.inc"
};
static NJS_MKEY_F dat_0cba1c20[2] = {
#include "ev0069/bird_textures_1/dat_0cba1c20.inc"
};
static NJS_MKEY_A dat_0cba1c40[2] = {
#include "ev0069/bird_textures_1/dat_0cba1c40.inc"
};
static NJS_MKEY_F dat_0cba1c60[2] = {
#include "ev0069/bird_textures_1/dat_0cba1c60.inc"
};
static NJS_MKEY_A dat_0cba1c80[2] = {
#include "ev0069/bird_textures_1/dat_0cba1c80.inc"
};
static NJS_MKEY_F dat_0cba1ca0[2] = {
#include "ev0069/bird_textures_1/dat_0cba1ca0.inc"
};
static NJS_MKEY_A dat_0cba1cc0[2] = {
#include "ev0069/bird_textures_1/dat_0cba1cc0.inc"
};
static NJS_MKEY_F dat_0cba1ce0[2] = {
#include "ev0069/bird_textures_1/dat_0cba1ce0.inc"
};
static NJS_MKEY_A dat_0cba1d00[2] = {
#include "ev0069/bird_textures_1/dat_0cba1d00.inc"
};
static NJS_MKEY_F dat_0cba1d20[2] = {
#include "ev0069/bird_textures_1/dat_0cba1d20.inc"
};
static NJS_MKEY_A dat_0cba1d40[31] = {
#include "ev0069/bird_textures_1/dat_0cba1d40.inc"
};
static NJS_MKEY_F dat_0cba1f30[2] = {
#include "ev0069/bird_textures_1/dat_0cba1f30.inc"
};
static NJS_MKEY_A dat_0cba1f50[2] = {
#include "ev0069/bird_textures_1/dat_0cba1f50.inc"
};
static NJS_MKEY_F dat_0cba1f70[2] = {
#include "ev0069/bird_textures_1/dat_0cba1f70.inc"
};
static NJS_MKEY_A dat_0cba1f90[2] = {
#include "ev0069/bird_textures_1/dat_0cba1f90.inc"
};
static NJS_MKEY_F dat_0cba1fb0[2] = {
#include "ev0069/bird_textures_1/dat_0cba1fb0.inc"
};
static NJS_MKEY_A dat_0cba1fd0[2] = {
#include "ev0069/bird_textures_1/dat_0cba1fd0.inc"
};
static NJS_MKEY_F dat_0cba1ff0[2] = {
#include "ev0069/bird_textures_1/dat_0cba1ff0.inc"
};
static NJS_MKEY_A dat_0cba2010[2] = {
#include "ev0069/bird_textures_1/dat_0cba2010.inc"
};
static NJS_MKEY_F dat_0cba2030[2] = {
#include "ev0069/bird_textures_1/dat_0cba2030.inc"
};
static NJS_MKEY_A dat_0cba2050[2] = {
#include "ev0069/bird_textures_1/dat_0cba2050.inc"
};
static NJS_MKEY_F dat_0cba2070[2] = {
#include "ev0069/bird_textures_1/dat_0cba2070.inc"
};
static NJS_MKEY_A dat_0cba2090[2] = {
#include "ev0069/bird_textures_1/dat_0cba2090.inc"
};
static NJS_MKEY_F dat_0cba20b0[2] = {
#include "ev0069/bird_textures_1/dat_0cba20b0.inc"
};
static NJS_MKEY_A dat_0cba20d0[2] = {
#include "ev0069/bird_textures_1/dat_0cba20d0.inc"
};
static NJS_MKEY_F dat_0cba20f0[2] = {
#include "ev0069/bird_textures_1/dat_0cba20f0.inc"
};
static NJS_MKEY_A dat_0cba2110[2] = {
#include "ev0069/bird_textures_1/dat_0cba2110.inc"
};
static NJS_MKEY_F dat_0cba2130[2] = {
#include "ev0069/bird_textures_1/dat_0cba2130.inc"
};
static NJS_MKEY_A dat_0cba2150[2] = {
#include "ev0069/bird_textures_1/dat_0cba2150.inc"
};
static NJS_MKEY_F dat_0cba2170[2] = {
#include "ev0069/bird_textures_1/dat_0cba2170.inc"
};
static NJS_MKEY_A dat_0cba2190[2] = {
#include "ev0069/bird_textures_1/dat_0cba2190.inc"
};
static NJS_MKEY_F dat_0cba21b0[31] = {
#include "ev0069/bird_textures_1/dat_0cba21b0.inc"
};
static NJS_MKEY_A dat_0cba23a0[2] = {
#include "ev0069/bird_textures_1/dat_0cba23a0.inc"
};
static NJS_MKEY_F dat_0cba23c0[31] = {
#include "ev0069/bird_textures_1/dat_0cba23c0.inc"
};
static NJS_MKEY_A dat_0cba25b0[2] = {
#include "ev0069/bird_textures_1/dat_0cba25b0.inc"
};
static NJS_MKEY_F dat_0cba25d0[2] = {
#include "ev0069/bird_textures_1/dat_0cba25d0.inc"
};
static NJS_MKEY_A dat_0cba25f0[2] = {
#include "ev0069/bird_textures_1/dat_0cba25f0.inc"
};
static NJS_MKEY_F dat_0cba2610[31] = {
#include "ev0069/bird_textures_1/dat_0cba2610.inc"
};
static NJS_MKEY_A dat_0cba2800[2] = {
#include "ev0069/bird_textures_1/dat_0cba2800.inc"
};
static NJS_MKEY_F dat_0cba2820[2] = {
#include "ev0069/bird_textures_1/dat_0cba2820.inc"
};
static NJS_MKEY_A dat_0cba2840[2] = {
#include "ev0069/bird_textures_1/dat_0cba2840.inc"
};
static NJS_MKEY_F dat_0cba2860[2] = {
#include "ev0069/bird_textures_1/dat_0cba2860.inc"
};
static NJS_MKEY_A dat_0cba2880[2] = {
#include "ev0069/bird_textures_1/dat_0cba2880.inc"
};
static NJS_MKEY_F dat_0cba28a0[2] = {
#include "ev0069/bird_textures_1/dat_0cba28a0.inc"
};
static NJS_MKEY_A dat_0cba28c0[2] = {
#include "ev0069/bird_textures_1/dat_0cba28c0.inc"
};
static NJS_MKEY_F dat_0cba28e0[2] = {
#include "ev0069/bird_textures_1/dat_0cba28e0.inc"
};
static NJS_MKEY_A dat_0cba2900[2] = {
#include "ev0069/bird_textures_1/dat_0cba2900.inc"
};
static NJS_MKEY_F dat_0cba2920[2] = {
#include "ev0069/bird_textures_1/dat_0cba2920.inc"
};
static NJS_MKEY_A dat_0cba2940[2] = {
#include "ev0069/bird_textures_1/dat_0cba2940.inc"
};
static NJS_MKEY_F dat_0cba2960[2] = {
#include "ev0069/bird_textures_1/dat_0cba2960.inc"
};
static NJS_MKEY_A dat_0cba2980[2] = {
#include "ev0069/bird_textures_1/dat_0cba2980.inc"
};
static NJS_MKEY_F dat_0cba29a0[2] = {
#include "ev0069/bird_textures_1/dat_0cba29a0.inc"
};
static NJS_MKEY_A dat_0cba29c0[2] = {
#include "ev0069/bird_textures_1/dat_0cba29c0.inc"
};
static NJS_MKEY_F dat_0cba29e0[2] = {
#include "ev0069/bird_textures_1/dat_0cba29e0.inc"
};
static NJS_MKEY_A dat_0cba2a00[2] = {
#include "ev0069/bird_textures_1/dat_0cba2a00.inc"
};
static NJS_MKEY_F dat_0cba2a20[2] = {
#include "ev0069/bird_textures_1/dat_0cba2a20.inc"
};
static NJS_MKEY_A dat_0cba2a40[2] = {
#include "ev0069/bird_textures_1/dat_0cba2a40.inc"
};
static NJS_MKEY_F dat_0cba2a60[2] = {
#include "ev0069/bird_textures_1/dat_0cba2a60.inc"
};
static NJS_MKEY_A dat_0cba2a80[2] = {
#include "ev0069/bird_textures_1/dat_0cba2a80.inc"
};
static NJS_MKEY_F dat_0cba2aa0[2] = {
#include "ev0069/bird_textures_1/dat_0cba2aa0.inc"
};
static NJS_MKEY_A dat_0cba2ac0[2] = {
#include "ev0069/bird_textures_1/dat_0cba2ac0.inc"
};
static NJS_MKEY_F dat_0cba2ae0[2] = {
#include "ev0069/bird_textures_1/dat_0cba2ae0.inc"
};
static NJS_MKEY_A dat_0cba2b00[2] = {
#include "ev0069/bird_textures_1/dat_0cba2b00.inc"
};
static NJS_MKEY_F dat_0cba2b20[2] = {
#include "ev0069/bird_textures_1/dat_0cba2b20.inc"
};
static NJS_MKEY_A dat_0cba2b40[2] = {
#include "ev0069/bird_textures_1/dat_0cba2b40.inc"
};
static NJS_MKEY_F dat_0cba2b60[2] = {
#include "ev0069/bird_textures_1/dat_0cba2b60.inc"
};
static NJS_MKEY_A dat_0cba2b80[2] = {
#include "ev0069/bird_textures_1/dat_0cba2b80.inc"
};
static NJS_MKEY_F dat_0cba2ba0[2] = {
#include "ev0069/bird_textures_1/dat_0cba2ba0.inc"
};
static NJS_MKEY_A dat_0cba2bc0[2] = {
#include "ev0069/bird_textures_1/dat_0cba2bc0.inc"
};
static NJS_MKEY_F dat_0cba2be0[2] = {
#include "ev0069/bird_textures_1/dat_0cba2be0.inc"
};
static NJS_MKEY_A dat_0cba2c00[2] = {
#include "ev0069/bird_textures_1/dat_0cba2c00.inc"
};
static NJS_MKEY_F dat_0cba2c20[2] = {
#include "ev0069/bird_textures_1/dat_0cba2c20.inc"
};
static NJS_MKEY_A dat_0cba2c40[2] = {
#include "ev0069/bird_textures_1/dat_0cba2c40.inc"
};
static NJS_MDATA2 dat_0cba2c60[57] = {
#include "ev0069/bird_textures_1/dat_0cba2c60.inc"
};
static NJS_MOTION ev0069_bird_motion_1 = {
#include "ev0069/bird_textures_1/ev0069_bird_motion_1.inc"
};
NJS_ACTION ev0069_bird_action_1 = {
#include "ev0069/bird_textures_1/ev0069_bird_action_1.inc"
};
static NJS_MKEY_F dat_0cba3004[2] = {
#include "ev0069/bird_textures_1/dat_0cba3004.inc"
};
static NJS_MKEY_A dat_0cba3024[2] = {
#include "ev0069/bird_textures_1/dat_0cba3024.inc"
};
static NJS_MKEY_F dat_0cba3044[30] = {
#include "ev0069/bird_textures_1/dat_0cba3044.inc"
};
static NJS_MKEY_A dat_0cba3224[30] = {
#include "ev0069/bird_textures_1/dat_0cba3224.inc"
};
static NJS_MKEY_F dat_0cba3404[2] = {
#include "ev0069/bird_textures_1/dat_0cba3404.inc"
};
static NJS_MKEY_A dat_0cba3424[2] = {
#include "ev0069/bird_textures_1/dat_0cba3424.inc"
};
static NJS_MKEY_F dat_0cba3444[2] = {
#include "ev0069/bird_textures_1/dat_0cba3444.inc"
};
static NJS_MKEY_A dat_0cba3464[2] = {
#include "ev0069/bird_textures_1/dat_0cba3464.inc"
};
static NJS_MKEY_F dat_0cba3484[2] = {
#include "ev0069/bird_textures_1/dat_0cba3484.inc"
};
static NJS_MKEY_A dat_0cba34a4[2] = {
#include "ev0069/bird_textures_1/dat_0cba34a4.inc"
};
static NJS_MKEY_F dat_0cba34c4[2] = {
#include "ev0069/bird_textures_1/dat_0cba34c4.inc"
};
static NJS_MKEY_A dat_0cba34e4[2] = {
#include "ev0069/bird_textures_1/dat_0cba34e4.inc"
};
static NJS_MKEY_F dat_0cba3504[2] = {
#include "ev0069/bird_textures_1/dat_0cba3504.inc"
};
static NJS_MKEY_A dat_0cba3524[30] = {
#include "ev0069/bird_textures_1/dat_0cba3524.inc"
};
static NJS_MKEY_F dat_0cba3704[2] = {
#include "ev0069/bird_textures_1/dat_0cba3704.inc"
};
static NJS_MKEY_A dat_0cba3724[2] = {
#include "ev0069/bird_textures_1/dat_0cba3724.inc"
};
static NJS_MKEY_F dat_0cba3744[2] = {
#include "ev0069/bird_textures_1/dat_0cba3744.inc"
};
static NJS_MKEY_A dat_0cba3764[2] = {
#include "ev0069/bird_textures_1/dat_0cba3764.inc"
};
static NJS_MKEY_F dat_0cba3784[2] = {
#include "ev0069/bird_textures_1/dat_0cba3784.inc"
};
static NJS_MKEY_A dat_0cba37a4[2] = {
#include "ev0069/bird_textures_1/dat_0cba37a4.inc"
};
static NJS_MKEY_F dat_0cba37c4[2] = {
#include "ev0069/bird_textures_1/dat_0cba37c4.inc"
};
static NJS_MKEY_A dat_0cba37e4[30] = {
#include "ev0069/bird_textures_1/dat_0cba37e4.inc"
};
static NJS_MKEY_F dat_0cba39c4[2] = {
#include "ev0069/bird_textures_1/dat_0cba39c4.inc"
};
static NJS_MKEY_A dat_0cba39e4[2] = {
#include "ev0069/bird_textures_1/dat_0cba39e4.inc"
};
static NJS_MKEY_F dat_0cba3a04[2] = {
#include "ev0069/bird_textures_1/dat_0cba3a04.inc"
};
static NJS_MKEY_A dat_0cba3a24[2] = {
#include "ev0069/bird_textures_1/dat_0cba3a24.inc"
};
static NJS_MKEY_F dat_0cba3a44[2] = {
#include "ev0069/bird_textures_1/dat_0cba3a44.inc"
};
static NJS_MKEY_A dat_0cba3a64[2] = {
#include "ev0069/bird_textures_1/dat_0cba3a64.inc"
};
static NJS_MKEY_F dat_0cba3a84[2] = {
#include "ev0069/bird_textures_1/dat_0cba3a84.inc"
};
static NJS_MKEY_A dat_0cba3aa4[2] = {
#include "ev0069/bird_textures_1/dat_0cba3aa4.inc"
};
static NJS_MKEY_F dat_0cba3ac4[2] = {
#include "ev0069/bird_textures_1/dat_0cba3ac4.inc"
};
static NJS_MKEY_A dat_0cba3ae4[2] = {
#include "ev0069/bird_textures_1/dat_0cba3ae4.inc"
};
static NJS_MKEY_F dat_0cba3b04[2] = {
#include "ev0069/bird_textures_1/dat_0cba3b04.inc"
};
static NJS_MKEY_A dat_0cba3b24[2] = {
#include "ev0069/bird_textures_1/dat_0cba3b24.inc"
};
static NJS_MKEY_F dat_0cba3b44[2] = {
#include "ev0069/bird_textures_1/dat_0cba3b44.inc"
};
static NJS_MKEY_A dat_0cba3b64[2] = {
#include "ev0069/bird_textures_1/dat_0cba3b64.inc"
};
static NJS_MKEY_F dat_0cba3b84[2] = {
#include "ev0069/bird_textures_1/dat_0cba3b84.inc"
};
static NJS_MKEY_A dat_0cba3ba4[2] = {
#include "ev0069/bird_textures_1/dat_0cba3ba4.inc"
};
static NJS_MKEY_F dat_0cba3bc4[2] = {
#include "ev0069/bird_textures_1/dat_0cba3bc4.inc"
};
static NJS_MKEY_A dat_0cba3be4[2] = {
#include "ev0069/bird_textures_1/dat_0cba3be4.inc"
};
static NJS_MKEY_F dat_0cba3c04[2] = {
#include "ev0069/bird_textures_1/dat_0cba3c04.inc"
};
static NJS_MKEY_A dat_0cba3c24[2] = {
#include "ev0069/bird_textures_1/dat_0cba3c24.inc"
};
static NJS_MKEY_F dat_0cba3c44[2] = {
#include "ev0069/bird_textures_1/dat_0cba3c44.inc"
};
static NJS_MKEY_A dat_0cba3c64[2] = {
#include "ev0069/bird_textures_1/dat_0cba3c64.inc"
};
static NJS_MKEY_F dat_0cba3c84[2] = {
#include "ev0069/bird_textures_1/dat_0cba3c84.inc"
};
static NJS_MKEY_A dat_0cba3ca4[2] = {
#include "ev0069/bird_textures_1/dat_0cba3ca4.inc"
};
static NJS_MKEY_F dat_0cba3cc4[2] = {
#include "ev0069/bird_textures_1/dat_0cba3cc4.inc"
};
static NJS_MKEY_A dat_0cba3ce4[2] = {
#include "ev0069/bird_textures_1/dat_0cba3ce4.inc"
};
static NJS_MKEY_F dat_0cba3d04[2] = {
#include "ev0069/bird_textures_1/dat_0cba3d04.inc"
};
static NJS_MKEY_A dat_0cba3d24[2] = {
#include "ev0069/bird_textures_1/dat_0cba3d24.inc"
};
static NJS_MKEY_F dat_0cba3d44[2] = {
#include "ev0069/bird_textures_1/dat_0cba3d44.inc"
};
static NJS_MKEY_A dat_0cba3d64[30] = {
#include "ev0069/bird_textures_1/dat_0cba3d64.inc"
};
static NJS_MKEY_F dat_0cba3f44[2] = {
#include "ev0069/bird_textures_1/dat_0cba3f44.inc"
};
static NJS_MKEY_A dat_0cba3f64[2] = {
#include "ev0069/bird_textures_1/dat_0cba3f64.inc"
};
static NJS_MKEY_F dat_0cba3f84[2] = {
#include "ev0069/bird_textures_1/dat_0cba3f84.inc"
};
static NJS_MKEY_A dat_0cba3fa4[2] = {
#include "ev0069/bird_textures_1/dat_0cba3fa4.inc"
};
static NJS_MKEY_F dat_0cba3fc4[2] = {
#include "ev0069/bird_textures_1/dat_0cba3fc4.inc"
};
static NJS_MKEY_A dat_0cba3fe4[2] = {
#include "ev0069/bird_textures_1/dat_0cba3fe4.inc"
};
static NJS_MKEY_F dat_0cba4004[2] = {
#include "ev0069/bird_textures_1/dat_0cba4004.inc"
};
static NJS_MKEY_A dat_0cba4024[2] = {
#include "ev0069/bird_textures_1/dat_0cba4024.inc"
};
static NJS_MKEY_F dat_0cba4044[2] = {
#include "ev0069/bird_textures_1/dat_0cba4044.inc"
};
static NJS_MKEY_A dat_0cba4064[2] = {
#include "ev0069/bird_textures_1/dat_0cba4064.inc"
};
static NJS_MKEY_F dat_0cba4084[2] = {
#include "ev0069/bird_textures_1/dat_0cba4084.inc"
};
static NJS_MKEY_A dat_0cba40a4[2] = {
#include "ev0069/bird_textures_1/dat_0cba40a4.inc"
};
static NJS_MKEY_F dat_0cba40c4[2] = {
#include "ev0069/bird_textures_1/dat_0cba40c4.inc"
};
static NJS_MKEY_A dat_0cba40e4[2] = {
#include "ev0069/bird_textures_1/dat_0cba40e4.inc"
};
static NJS_MKEY_F dat_0cba4104[2] = {
#include "ev0069/bird_textures_1/dat_0cba4104.inc"
};
static NJS_MKEY_A dat_0cba4124[30] = {
#include "ev0069/bird_textures_1/dat_0cba4124.inc"
};
static NJS_MKEY_F dat_0cba4304[2] = {
#include "ev0069/bird_textures_1/dat_0cba4304.inc"
};
static NJS_MKEY_A dat_0cba4324[2] = {
#include "ev0069/bird_textures_1/dat_0cba4324.inc"
};
static NJS_MKEY_F dat_0cba4344[2] = {
#include "ev0069/bird_textures_1/dat_0cba4344.inc"
};
static NJS_MKEY_A dat_0cba4364[2] = {
#include "ev0069/bird_textures_1/dat_0cba4364.inc"
};
static NJS_MKEY_F dat_0cba4384[30] = {
#include "ev0069/bird_textures_1/dat_0cba4384.inc"
};
static NJS_MKEY_A dat_0cba4564[2] = {
#include "ev0069/bird_textures_1/dat_0cba4564.inc"
};
static NJS_MKEY_F dat_0cba4584[30] = {
#include "ev0069/bird_textures_1/dat_0cba4584.inc"
};
static NJS_MKEY_A dat_0cba4764[2] = {
#include "ev0069/bird_textures_1/dat_0cba4764.inc"
};
static NJS_MKEY_F dat_0cba4784[2] = {
#include "ev0069/bird_textures_1/dat_0cba4784.inc"
};
static NJS_MKEY_A dat_0cba47a4[2] = {
#include "ev0069/bird_textures_1/dat_0cba47a4.inc"
};
static NJS_MKEY_F dat_0cba47c4[30] = {
#include "ev0069/bird_textures_1/dat_0cba47c4.inc"
};
static NJS_MKEY_A dat_0cba49a4[2] = {
#include "ev0069/bird_textures_1/dat_0cba49a4.inc"
};
static NJS_MKEY_F dat_0cba49c4[30] = {
#include "ev0069/bird_textures_1/dat_0cba49c4.inc"
};
static NJS_MKEY_A dat_0cba4ba4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4ba4.inc"
};
static NJS_MKEY_F dat_0cba4bc4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4bc4.inc"
};
static NJS_MKEY_A dat_0cba4be4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4be4.inc"
};
static NJS_MKEY_F dat_0cba4c04[2] = {
#include "ev0069/bird_textures_1/dat_0cba4c04.inc"
};
static NJS_MKEY_A dat_0cba4c24[2] = {
#include "ev0069/bird_textures_1/dat_0cba4c24.inc"
};
static NJS_MKEY_F dat_0cba4c44[2] = {
#include "ev0069/bird_textures_1/dat_0cba4c44.inc"
};
static NJS_MKEY_A dat_0cba4c64[2] = {
#include "ev0069/bird_textures_1/dat_0cba4c64.inc"
};
static NJS_MKEY_F dat_0cba4c84[2] = {
#include "ev0069/bird_textures_1/dat_0cba4c84.inc"
};
static NJS_MKEY_A dat_0cba4ca4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4ca4.inc"
};
static NJS_MKEY_F dat_0cba4cc4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4cc4.inc"
};
static NJS_MKEY_A dat_0cba4ce4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4ce4.inc"
};
static NJS_MKEY_F dat_0cba4d04[2] = {
#include "ev0069/bird_textures_1/dat_0cba4d04.inc"
};
static NJS_MKEY_A dat_0cba4d24[2] = {
#include "ev0069/bird_textures_1/dat_0cba4d24.inc"
};
static NJS_MKEY_F dat_0cba4d44[2] = {
#include "ev0069/bird_textures_1/dat_0cba4d44.inc"
};
static NJS_MKEY_A dat_0cba4d64[2] = {
#include "ev0069/bird_textures_1/dat_0cba4d64.inc"
};
static NJS_MKEY_F dat_0cba4d84[2] = {
#include "ev0069/bird_textures_1/dat_0cba4d84.inc"
};
static NJS_MKEY_A dat_0cba4da4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4da4.inc"
};
static NJS_MKEY_F dat_0cba4dc4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4dc4.inc"
};
static NJS_MKEY_A dat_0cba4de4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4de4.inc"
};
static NJS_MKEY_F dat_0cba4e04[2] = {
#include "ev0069/bird_textures_1/dat_0cba4e04.inc"
};
static NJS_MKEY_A dat_0cba4e24[2] = {
#include "ev0069/bird_textures_1/dat_0cba4e24.inc"
};
static NJS_MKEY_F dat_0cba4e44[2] = {
#include "ev0069/bird_textures_1/dat_0cba4e44.inc"
};
static NJS_MKEY_A dat_0cba4e64[2] = {
#include "ev0069/bird_textures_1/dat_0cba4e64.inc"
};
static NJS_MKEY_F dat_0cba4e84[2] = {
#include "ev0069/bird_textures_1/dat_0cba4e84.inc"
};
static NJS_MKEY_A dat_0cba4ea4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4ea4.inc"
};
static NJS_MKEY_F dat_0cba4ec4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4ec4.inc"
};
static NJS_MKEY_A dat_0cba4ee4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4ee4.inc"
};
static NJS_MKEY_F dat_0cba4f04[2] = {
#include "ev0069/bird_textures_1/dat_0cba4f04.inc"
};
static NJS_MKEY_A dat_0cba4f24[2] = {
#include "ev0069/bird_textures_1/dat_0cba4f24.inc"
};
static NJS_MKEY_F dat_0cba4f44[2] = {
#include "ev0069/bird_textures_1/dat_0cba4f44.inc"
};
static NJS_MKEY_A dat_0cba4f64[2] = {
#include "ev0069/bird_textures_1/dat_0cba4f64.inc"
};
static NJS_MKEY_F dat_0cba4f84[2] = {
#include "ev0069/bird_textures_1/dat_0cba4f84.inc"
};
static NJS_MKEY_A dat_0cba4fa4[2] = {
#include "ev0069/bird_textures_1/dat_0cba4fa4.inc"
};
static NJS_MDATA2 dat_0cba4fc4[57] = {
#include "ev0069/bird_textures_1/dat_0cba4fc4.inc"
};
static NJS_MOTION ev0069_bird_motion_2 = {
#include "ev0069/bird_textures_1/ev0069_bird_motion_2.inc"
};
NJS_ACTION ev0069_bird_action_2 = {
#include "ev0069/bird_textures_1/ev0069_bird_action_2.inc"
};
static NJS_MKEY_F dat_0cba5368[2] = {
#include "ev0069/bird_textures_1/dat_0cba5368.inc"
};
static NJS_MKEY_F dat_0cba5388[32] = {
#include "ev0069/bird_textures_1/dat_0cba5388.inc"
};
static NJS_MKEY_A dat_0cba5588[32] = {
#include "ev0069/bird_textures_1/dat_0cba5588.inc"
};
static NJS_MKEY_F dat_0cba5788[2] = {
#include "ev0069/bird_textures_1/dat_0cba5788.inc"
};
static NJS_MKEY_A dat_0cba57a8[32] = {
#include "ev0069/bird_textures_1/dat_0cba57a8.inc"
};
static NJS_MKEY_F dat_0cba59a8[2] = {
#include "ev0069/bird_textures_1/dat_0cba59a8.inc"
};
static NJS_MKEY_F dat_0cba59c8[2] = {
#include "ev0069/bird_textures_1/dat_0cba59c8.inc"
};
static NJS_MKEY_F dat_0cba59e8[2] = {
#include "ev0069/bird_textures_1/dat_0cba59e8.inc"
};
static NJS_MKEY_F dat_0cba5a08[2] = {
#include "ev0069/bird_textures_1/dat_0cba5a08.inc"
};
static NJS_MKEY_A dat_0cba5a28[32] = {
#include "ev0069/bird_textures_1/dat_0cba5a28.inc"
};
static NJS_MKEY_F dat_0cba5c28[2] = {
#include "ev0069/bird_textures_1/dat_0cba5c28.inc"
};
static NJS_MKEY_F dat_0cba5c48[2] = {
#include "ev0069/bird_textures_1/dat_0cba5c48.inc"
};
static NJS_MKEY_F dat_0cba5c68[2] = {
#include "ev0069/bird_textures_1/dat_0cba5c68.inc"
};
static NJS_MKEY_F dat_0cba5c88[2] = {
#include "ev0069/bird_textures_1/dat_0cba5c88.inc"
};
static NJS_MKEY_A dat_0cba5ca8[32] = {
#include "ev0069/bird_textures_1/dat_0cba5ca8.inc"
};
static NJS_MKEY_F dat_0cba5ea8[2] = {
#include "ev0069/bird_textures_1/dat_0cba5ea8.inc"
};
static NJS_MKEY_F dat_0cba5ec8[2] = {
#include "ev0069/bird_textures_1/dat_0cba5ec8.inc"
};
static NJS_MKEY_F dat_0cba5ee8[2] = {
#include "ev0069/bird_textures_1/dat_0cba5ee8.inc"
};
static NJS_MKEY_F dat_0cba5f08[2] = {
#include "ev0069/bird_textures_1/dat_0cba5f08.inc"
};
static NJS_MKEY_A dat_0cba5f28[32] = {
#include "ev0069/bird_textures_1/dat_0cba5f28.inc"
};
static NJS_MKEY_F dat_0cba6128[2] = {
#include "ev0069/bird_textures_1/dat_0cba6128.inc"
};
static NJS_MKEY_F dat_0cba6148[2] = {
#include "ev0069/bird_textures_1/dat_0cba6148.inc"
};
static NJS_MKEY_F dat_0cba6168[2] = {
#include "ev0069/bird_textures_1/dat_0cba6168.inc"
};
static NJS_MKEY_F dat_0cba6188[2] = {
#include "ev0069/bird_textures_1/dat_0cba6188.inc"
};
static NJS_MKEY_F dat_0cba61a8[2] = {
#include "ev0069/bird_textures_1/dat_0cba61a8.inc"
};
static NJS_MKEY_F dat_0cba61c8[2] = {
#include "ev0069/bird_textures_1/dat_0cba61c8.inc"
};
static NJS_MKEY_F dat_0cba61e8[2] = {
#include "ev0069/bird_textures_1/dat_0cba61e8.inc"
};
static NJS_MKEY_F dat_0cba6208[2] = {
#include "ev0069/bird_textures_1/dat_0cba6208.inc"
};
static NJS_MKEY_F dat_0cba6228[2] = {
#include "ev0069/bird_textures_1/dat_0cba6228.inc"
};
static NJS_MKEY_F dat_0cba6248[2] = {
#include "ev0069/bird_textures_1/dat_0cba6248.inc"
};
static NJS_MKEY_F dat_0cba6268[2] = {
#include "ev0069/bird_textures_1/dat_0cba6268.inc"
};
static NJS_MKEY_A dat_0cba6288[32] = {
#include "ev0069/bird_textures_1/dat_0cba6288.inc"
};
static NJS_MKEY_F dat_0cba6488[2] = {
#include "ev0069/bird_textures_1/dat_0cba6488.inc"
};
static NJS_MKEY_F dat_0cba64a8[2] = {
#include "ev0069/bird_textures_1/dat_0cba64a8.inc"
};
static NJS_MKEY_F dat_0cba64c8[2] = {
#include "ev0069/bird_textures_1/dat_0cba64c8.inc"
};
static NJS_MKEY_F dat_0cba64e8[2] = {
#include "ev0069/bird_textures_1/dat_0cba64e8.inc"
};
static NJS_MKEY_F dat_0cba6508[2] = {
#include "ev0069/bird_textures_1/dat_0cba6508.inc"
};
static NJS_MKEY_F dat_0cba6528[2] = {
#include "ev0069/bird_textures_1/dat_0cba6528.inc"
};
static NJS_MKEY_F dat_0cba6548[2] = {
#include "ev0069/bird_textures_1/dat_0cba6548.inc"
};
static NJS_MKEY_F dat_0cba6568[2] = {
#include "ev0069/bird_textures_1/dat_0cba6568.inc"
};
static NJS_MKEY_A dat_0cba6588[32] = {
#include "ev0069/bird_textures_1/dat_0cba6588.inc"
};
static NJS_MKEY_F dat_0cba6788[2] = {
#include "ev0069/bird_textures_1/dat_0cba6788.inc"
};
static NJS_MKEY_F dat_0cba67a8[2] = {
#include "ev0069/bird_textures_1/dat_0cba67a8.inc"
};
static NJS_MKEY_F dat_0cba67c8[32] = {
#include "ev0069/bird_textures_1/dat_0cba67c8.inc"
};
static NJS_MKEY_F dat_0cba69c8[32] = {
#include "ev0069/bird_textures_1/dat_0cba69c8.inc"
};
static NJS_MKEY_F dat_0cba6bc8[32] = {
#include "ev0069/bird_textures_1/dat_0cba6bc8.inc"
};
static NJS_MKEY_F dat_0cba6dc8[32] = {
#include "ev0069/bird_textures_1/dat_0cba6dc8.inc"
};
static NJS_MKEY_F dat_0cba6fc8[32] = {
#include "ev0069/bird_textures_1/dat_0cba6fc8.inc"
};
static NJS_MKEY_F dat_0cba71c8[2] = {
#include "ev0069/bird_textures_1/dat_0cba71c8.inc"
};
static NJS_MKEY_F dat_0cba71e8[2] = {
#include "ev0069/bird_textures_1/dat_0cba71e8.inc"
};
static NJS_MKEY_A dat_0cba7208[32] = {
#include "ev0069/bird_textures_1/dat_0cba7208.inc"
};
static NJS_MKEY_F dat_0cba7408[2] = {
#include "ev0069/bird_textures_1/dat_0cba7408.inc"
};
static NJS_MKEY_F dat_0cba7428[2] = {
#include "ev0069/bird_textures_1/dat_0cba7428.inc"
};
static NJS_MKEY_F dat_0cba7448[2] = {
#include "ev0069/bird_textures_1/dat_0cba7448.inc"
};
static NJS_MKEY_A dat_0cba7468[32] = {
#include "ev0069/bird_textures_1/dat_0cba7468.inc"
};
static NJS_MKEY_F dat_0cba7668[2] = {
#include "ev0069/bird_textures_1/dat_0cba7668.inc"
};
static NJS_MKEY_F dat_0cba7688[2] = {
#include "ev0069/bird_textures_1/dat_0cba7688.inc"
};
static NJS_MKEY_F dat_0cba76a8[2] = {
#include "ev0069/bird_textures_1/dat_0cba76a8.inc"
};
static NJS_MKEY_F dat_0cba76c8[2] = {
#include "ev0069/bird_textures_1/dat_0cba76c8.inc"
};
static NJS_MKEY_F dat_0cba76e8[2] = {
#include "ev0069/bird_textures_1/dat_0cba76e8.inc"
};
static NJS_MKEY_A dat_0cba7708[32] = {
#include "ev0069/bird_textures_1/dat_0cba7708.inc"
};
static NJS_MKEY_F dat_0cba7908[2] = {
#include "ev0069/bird_textures_1/dat_0cba7908.inc"
};
static NJS_MKEY_F dat_0cba7928[2] = {
#include "ev0069/bird_textures_1/dat_0cba7928.inc"
};
static NJS_MKEY_F dat_0cba7948[2] = {
#include "ev0069/bird_textures_1/dat_0cba7948.inc"
};
static NJS_MKEY_A dat_0cba7968[32] = {
#include "ev0069/bird_textures_1/dat_0cba7968.inc"
};
static NJS_MKEY_F dat_0cba7b68[2] = {
#include "ev0069/bird_textures_1/dat_0cba7b68.inc"
};
static NJS_MKEY_F dat_0cba7b88[2] = {
#include "ev0069/bird_textures_1/dat_0cba7b88.inc"
};
static NJS_MKEY_F dat_0cba7ba8[2] = {
#include "ev0069/bird_textures_1/dat_0cba7ba8.inc"
};
static NJS_MDATA2 dat_0cba7bc8[57] = {
#include "ev0069/bird_textures_1/dat_0cba7bc8.inc"
};
static NJS_MOTION ev0069_bird_motion_3 = {
#include "ev0069/bird_textures_1/ev0069_bird_motion_3.inc"
};
NJS_ACTION ev0069_bird_action_3 = {
#include "ev0069/bird_textures_1/ev0069_bird_action_3.inc"
};
static NJS_MKEY_F dat_0cba7f6c[2] = {
#include "ev0069/bird_textures_1/dat_0cba7f6c.inc"
};
static NJS_MKEY_A dat_0cba7f8c[2] = {
#include "ev0069/bird_textures_1/dat_0cba7f8c.inc"
};
static NJS_MKEY_F dat_0cba7fac[2] = {
#include "ev0069/bird_textures_1/dat_0cba7fac.inc"
};
static NJS_MKEY_A dat_0cba7fcc[32] = {
#include "ev0069/bird_textures_1/dat_0cba7fcc.inc"
};
static NJS_MKEY_F dat_0cba81cc[2] = {
#include "ev0069/bird_textures_1/dat_0cba81cc.inc"
};
static NJS_MKEY_A dat_0cba81ec[32] = {
#include "ev0069/bird_textures_1/dat_0cba81ec.inc"
};
static NJS_MKEY_F dat_0cba83ec[2] = {
#include "ev0069/bird_textures_1/dat_0cba83ec.inc"
};
static NJS_MKEY_A dat_0cba840c[2] = {
#include "ev0069/bird_textures_1/dat_0cba840c.inc"
};
static NJS_MKEY_F dat_0cba842c[2] = {
#include "ev0069/bird_textures_1/dat_0cba842c.inc"
};
static NJS_MKEY_A dat_0cba844c[32] = {
#include "ev0069/bird_textures_1/dat_0cba844c.inc"
};
static NJS_MKEY_F dat_0cba864c[2] = {
#include "ev0069/bird_textures_1/dat_0cba864c.inc"
};
static NJS_MKEY_A dat_0cba866c[32] = {
#include "ev0069/bird_textures_1/dat_0cba866c.inc"
};
static NJS_MKEY_F dat_0cba886c[2] = {
#include "ev0069/bird_textures_1/dat_0cba886c.inc"
};
static NJS_MKEY_A dat_0cba888c[2] = {
#include "ev0069/bird_textures_1/dat_0cba888c.inc"
};
static NJS_MKEY_F dat_0cba88ac[2] = {
#include "ev0069/bird_textures_1/dat_0cba88ac.inc"
};
static NJS_MKEY_A dat_0cba88cc[2] = {
#include "ev0069/bird_textures_1/dat_0cba88cc.inc"
};
static NJS_MKEY_F dat_0cba88ec[2] = {
#include "ev0069/bird_textures_1/dat_0cba88ec.inc"
};
static NJS_MKEY_A dat_0cba890c[2] = {
#include "ev0069/bird_textures_1/dat_0cba890c.inc"
};
static NJS_MKEY_F dat_0cba892c[2] = {
#include "ev0069/bird_textures_1/dat_0cba892c.inc"
};
static NJS_MKEY_A dat_0cba894c[32] = {
#include "ev0069/bird_textures_1/dat_0cba894c.inc"
};
static NJS_MKEY_F dat_0cba8b4c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8b4c.inc"
};
static NJS_MKEY_A dat_0cba8b6c[32] = {
#include "ev0069/bird_textures_1/dat_0cba8b6c.inc"
};
static NJS_MKEY_F dat_0cba8d6c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8d6c.inc"
};
static NJS_MKEY_A dat_0cba8d8c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8d8c.inc"
};
static NJS_MKEY_F dat_0cba8dac[2] = {
#include "ev0069/bird_textures_1/dat_0cba8dac.inc"
};
static NJS_MKEY_A dat_0cba8dcc[2] = {
#include "ev0069/bird_textures_1/dat_0cba8dcc.inc"
};
static NJS_MKEY_F dat_0cba8dec[2] = {
#include "ev0069/bird_textures_1/dat_0cba8dec.inc"
};
static NJS_MKEY_A dat_0cba8e0c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8e0c.inc"
};
static NJS_MKEY_F dat_0cba8e2c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8e2c.inc"
};
static NJS_MKEY_A dat_0cba8e4c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8e4c.inc"
};
static NJS_MKEY_F dat_0cba8e6c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8e6c.inc"
};
static NJS_MKEY_A dat_0cba8e8c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8e8c.inc"
};
static NJS_MKEY_F dat_0cba8eac[2] = {
#include "ev0069/bird_textures_1/dat_0cba8eac.inc"
};
static NJS_MKEY_A dat_0cba8ecc[2] = {
#include "ev0069/bird_textures_1/dat_0cba8ecc.inc"
};
static NJS_MKEY_F dat_0cba8eec[2] = {
#include "ev0069/bird_textures_1/dat_0cba8eec.inc"
};
static NJS_MKEY_A dat_0cba8f0c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8f0c.inc"
};
static NJS_MKEY_F dat_0cba8f2c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8f2c.inc"
};
static NJS_MKEY_A dat_0cba8f4c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8f4c.inc"
};
static NJS_MKEY_F dat_0cba8f6c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8f6c.inc"
};
static NJS_MKEY_A dat_0cba8f8c[2] = {
#include "ev0069/bird_textures_1/dat_0cba8f8c.inc"
};
static NJS_MKEY_F dat_0cba8fac[2] = {
#include "ev0069/bird_textures_1/dat_0cba8fac.inc"
};
static NJS_MKEY_A dat_0cba8fcc[2] = {
#include "ev0069/bird_textures_1/dat_0cba8fcc.inc"
};
static NJS_MKEY_F dat_0cba8fec[2] = {
#include "ev0069/bird_textures_1/dat_0cba8fec.inc"
};
static NJS_MKEY_A dat_0cba900c[2] = {
#include "ev0069/bird_textures_1/dat_0cba900c.inc"
};
static NJS_MKEY_F dat_0cba902c[2] = {
#include "ev0069/bird_textures_1/dat_0cba902c.inc"
};
static NJS_MKEY_A dat_0cba904c[2] = {
#include "ev0069/bird_textures_1/dat_0cba904c.inc"
};
static NJS_MKEY_F dat_0cba906c[2] = {
#include "ev0069/bird_textures_1/dat_0cba906c.inc"
};
static NJS_MKEY_A dat_0cba908c[2] = {
#include "ev0069/bird_textures_1/dat_0cba908c.inc"
};
static NJS_MDATA2 dat_0cba90ac[24] = {
#include "ev0069/bird_textures_1/dat_0cba90ac.inc"
};
static NJS_MOTION ev0069_eggmobile_motion_0 = {
#include "ev0069/bird_textures_1/ev0069_eggmobile_motion_0.inc"
};
NJS_ACTION ev0069_eggmobile_action_0 = {
#include "ev0069/bird_textures_1/ev0069_eggmobile_action_0.inc"
};
static NJS_MKEY_F dat_0cba9240[2] = {
#include "ev0069/bird_textures_1/dat_0cba9240.inc"
};
static NJS_MKEY_F dat_0cba9260[2] = {
#include "ev0069/bird_textures_1/dat_0cba9260.inc"
};
static NJS_MKEY_A dat_0cba9280[32] = {
#include "ev0069/bird_textures_1/dat_0cba9280.inc"
};
static NJS_MKEY_F dat_0cba9480[2] = {
#include "ev0069/bird_textures_1/dat_0cba9480.inc"
};
static NJS_MKEY_A dat_0cba94a0[33] = {
#include "ev0069/bird_textures_1/dat_0cba94a0.inc"
};
static NJS_MKEY_F dat_0cba96b0[2] = {
#include "ev0069/bird_textures_1/dat_0cba96b0.inc"
};
static NJS_MKEY_F dat_0cba96d0[2] = {
#include "ev0069/bird_textures_1/dat_0cba96d0.inc"
};
static NJS_MKEY_A dat_0cba96f0[33] = {
#include "ev0069/bird_textures_1/dat_0cba96f0.inc"
};
static NJS_MKEY_F dat_0cba9900[2] = {
#include "ev0069/bird_textures_1/dat_0cba9900.inc"
};
static NJS_MKEY_A dat_0cba9920[33] = {
#include "ev0069/bird_textures_1/dat_0cba9920.inc"
};
static NJS_MKEY_F dat_0cba9b30[2] = {
#include "ev0069/bird_textures_1/dat_0cba9b30.inc"
};
static NJS_MKEY_F dat_0cba9b50[2] = {
#include "ev0069/bird_textures_1/dat_0cba9b50.inc"
};
static NJS_MKEY_F dat_0cba9b70[2] = {
#include "ev0069/bird_textures_1/dat_0cba9b70.inc"
};
static NJS_MKEY_F dat_0cba9b90[2] = {
#include "ev0069/bird_textures_1/dat_0cba9b90.inc"
};
static NJS_MKEY_A dat_0cba9bb0[33] = {
#include "ev0069/bird_textures_1/dat_0cba9bb0.inc"
};
static NJS_MKEY_F dat_0cba9dc0[2] = {
#include "ev0069/bird_textures_1/dat_0cba9dc0.inc"
};
static NJS_MKEY_A dat_0cba9de0[33] = {
#include "ev0069/bird_textures_1/dat_0cba9de0.inc"
};
static NJS_MKEY_F dat_0cba9ff0[2] = {
#include "ev0069/bird_textures_1/dat_0cba9ff0.inc"
};
static NJS_MKEY_A dat_0cbaa010[32] = {
#include "ev0069/bird_textures_1/dat_0cbaa010.inc"
};
static NJS_MKEY_F dat_0cbaa210[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa210.inc"
};
static NJS_MKEY_A dat_0cbaa230[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa230.inc"
};
static NJS_MKEY_F dat_0cbaa250[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa250.inc"
};
static NJS_MKEY_F dat_0cbaa270[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa270.inc"
};
static NJS_MKEY_F dat_0cbaa290[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa290.inc"
};
static NJS_MKEY_F dat_0cbaa2b0[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa2b0.inc"
};
static NJS_MKEY_F dat_0cbaa2d0[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa2d0.inc"
};
static NJS_MKEY_A dat_0cbaa2f0[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa2f0.inc"
};
static NJS_MKEY_F dat_0cbaa310[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa310.inc"
};
static NJS_MKEY_A dat_0cbaa330[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa330.inc"
};
static NJS_MKEY_F dat_0cbaa350[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa350.inc"
};
static NJS_MKEY_A dat_0cbaa370[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa370.inc"
};
static NJS_MKEY_F dat_0cbaa390[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa390.inc"
};
static NJS_MKEY_A dat_0cbaa3b0[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa3b0.inc"
};
static NJS_MKEY_F dat_0cbaa3d0[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa3d0.inc"
};
static NJS_MKEY_A dat_0cbaa3f0[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa3f0.inc"
};
static NJS_MKEY_F dat_0cbaa410[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa410.inc"
};
static NJS_MKEY_A dat_0cbaa430[2] = {
#include "ev0069/bird_textures_1/dat_0cbaa430.inc"
};
static NJS_MDATA2 dat_0cbaa450[24] = {
#include "ev0069/bird_textures_1/dat_0cbaa450.inc"
};
static NJS_MOTION ev0069_eggmobile_motion_1 = {
#include "ev0069/bird_textures_1/ev0069_eggmobile_motion_1.inc"
};
NJS_ACTION ev0069_eggmobile_action_1 = {
#include "ev0069/bird_textures_1/ev0069_eggmobile_action_1.inc"
};
static NJS_MKEY_F dat_0cbaa5e4[31] = {
#include "ev0069/bird_textures_1/dat_0cbaa5e4.inc"
};
static NJS_MKEY_A dat_0cbaa7d4[40] = {
#include "ev0069/bird_textures_1/dat_0cbaa7d4.inc"
};
static NJS_MKEY_A dat_0cbaaa54[40] = {
#include "ev0069/bird_textures_1/dat_0cbaaa54.inc"
};
static NJS_MKEY_A dat_0cbaacd4[40] = {
#include "ev0069/bird_textures_1/dat_0cbaacd4.inc"
};
static NJS_MKEY_A dat_0cbaaf54[40] = {
#include "ev0069/bird_textures_1/dat_0cbaaf54.inc"
};
static NJS_MKEY_A dat_0cbab1d4[40] = {
#include "ev0069/bird_textures_1/dat_0cbab1d4.inc"
};
static NJS_MKEY_A dat_0cbab454[40] = {
#include "ev0069/bird_textures_1/dat_0cbab454.inc"
};
static NJS_MKEY_A dat_0cbab6d4[40] = {
#include "ev0069/bird_textures_1/dat_0cbab6d4.inc"
};
static NJS_MKEY_F dat_0cbab954[31] = {
#include "ev0069/bird_textures_1/dat_0cbab954.inc"
};
static NJS_MDATA2 dat_0cbabb44[24] = {
#include "ev0069/bird_textures_1/dat_0cbabb44.inc"
};
static NJS_MOTION ev0069_eggmobile_motion_2 = {
#include "ev0069/bird_textures_1/ev0069_eggmobile_motion_2.inc"
};
NJS_ACTION ev0069_eggmobile_action_2 = {
#include "ev0069/bird_textures_1/ev0069_eggmobile_action_2.inc"
};
static NJS_MKEY_A dat_0cbabcd8[31] = {
#include "ev0069/bird_textures_1/dat_0cbabcd8.inc"
};
static NJS_MKEY_A dat_0cbabec8[32] = {
#include "ev0069/bird_textures_1/dat_0cbabec8.inc"
};
static NJS_MKEY_A dat_0cbac0c8[32] = {
#include "ev0069/bird_textures_1/dat_0cbac0c8.inc"
};
static NJS_MKEY_A dat_0cbac2c8[32] = {
#include "ev0069/bird_textures_1/dat_0cbac2c8.inc"
};
static NJS_MKEY_A dat_0cbac4c8[32] = {
#include "ev0069/bird_textures_1/dat_0cbac4c8.inc"
};
static NJS_MKEY_A dat_0cbac6c8[32] = {
#include "ev0069/bird_textures_1/dat_0cbac6c8.inc"
};
static NJS_MKEY_A dat_0cbac8c8[2] = {
#include "ev0069/bird_textures_1/dat_0cbac8c8.inc"
};
static NJS_MKEY_A dat_0cbac8e8[2] = {
#include "ev0069/bird_textures_1/dat_0cbac8e8.inc"
};
static NJS_MKEY_A dat_0cbac908[31] = {
#include "ev0069/bird_textures_1/dat_0cbac908.inc"
};
static NJS_MDATA2 dat_0cbacaf8[24] = {
#include "ev0069/bird_textures_1/dat_0cbacaf8.inc"
};
static NJS_MOTION ev0069_eggmobile_motion_3 = {
#include "ev0069/bird_textures_1/ev0069_eggmobile_motion_3.inc"
};
NJS_ACTION ev0069_eggmobile_action_3 = {
#include "ev0069/bird_textures_1/ev0069_eggmobile_action_3.inc"
};
static NJS_TEXNAME ev0069_emerald_textures[1] = {
    {TEXT_13, 0, 0},
};
