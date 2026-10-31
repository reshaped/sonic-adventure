/* the bird's texlists, trees and actions, the Egg Mobile's actions, and the
 * emerald's texlist */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev004b/bird_textures.h"

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
#include "ev004b/bird_textures/text.inc"

extern NJS_OBJECT event_eggmobile_object;

/* the same twelve names as ev004b_bird_textures. no word in EV004B or
 * 1ST_READ.BIN points at this list's texlist, dat_0cb95c88 */
static NJS_TEXNAME dat_0cb95bf8[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
static NJS_TEXLIST dat_0cb95c88 = {
#include "ev004b/bird_textures/dat_0cb95c88.inc"
};
static NJS_TEXNAME ev004b_bird_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev004b_bird_texlist = {
#include "ev004b/bird_textures/ev004b_bird_texlist.inc"
};
static NJS_MATERIAL dat_0cb95d28[1] = {
#include "ev004b/bird_textures/dat_0cb95d28.inc"
};
static Sint16 dat_0cb95d3c[6] = {
#include "ev004b/bird_textures/dat_0cb95d3c.inc"
};
static NJS_COLOR dat_0cb95d48[4] = {
#include "ev004b/bird_textures/dat_0cb95d48.inc"
};
static NJS_MESHSET dat_0cb95d58[1] = {
#include "ev004b/bird_textures/dat_0cb95d58.inc"
};
static NJS_POINT3 dat_0cb95d70[4] = {
#include "ev004b/bird_textures/dat_0cb95d70.inc"
};
static NJS_VECTOR dat_0cb95da0[4] = {
#include "ev004b/bird_textures/dat_0cb95da0.inc"
};
static NJS_MODEL dat_0cb95dd0 = {
#include "ev004b/bird_textures/dat_0cb95dd0.inc"
};
static NJS_OBJECT dat_0cb95df8 = {
#include "ev004b/bird_textures/dat_0cb95df8.inc"
};
static NJS_MATERIAL dat_0cb95e2c[1] = {
#include "ev004b/bird_textures/dat_0cb95e2c.inc"
};
static Sint16 dat_0cb95e40[12] = {
#include "ev004b/bird_textures/dat_0cb95e40.inc"
};
static NJS_COLOR dat_0cb95e58[9] = {
#include "ev004b/bird_textures/dat_0cb95e58.inc"
};
static NJS_MESHSET dat_0cb95e7c[1] = {
#include "ev004b/bird_textures/dat_0cb95e7c.inc"
};
static NJS_POINT3 dat_0cb95e94[7] = {
#include "ev004b/bird_textures/dat_0cb95e94.inc"
};
static NJS_VECTOR dat_0cb95ee8[7] = {
#include "ev004b/bird_textures/dat_0cb95ee8.inc"
};
static NJS_MODEL dat_0cb95f3c = {
#include "ev004b/bird_textures/dat_0cb95f3c.inc"
};
static NJS_OBJECT dat_0cb95f64 = {
#include "ev004b/bird_textures/dat_0cb95f64.inc"
};
static NJS_OBJECT dat_0cb95f98 = {
#include "ev004b/bird_textures/dat_0cb95f98.inc"
};
static NJS_OBJECT dat_0cb95fcc = {
#include "ev004b/bird_textures/dat_0cb95fcc.inc"
};
static NJS_OBJECT dat_0cb96000 = {
#include "ev004b/bird_textures/dat_0cb96000.inc"
};
static NJS_OBJECT dat_0cb96034 = {
#include "ev004b/bird_textures/dat_0cb96034.inc"
};
static NJS_OBJECT dat_0cb96068 = {
#include "ev004b/bird_textures/dat_0cb96068.inc"
};
static NJS_OBJECT dat_0cb9609c = {
#include "ev004b/bird_textures/dat_0cb9609c.inc"
};
static NJS_MATERIAL dat_0cb960d0[1] = {
#include "ev004b/bird_textures/dat_0cb960d0.inc"
};
static Sint16 dat_0cb960e4[6] = {
#include "ev004b/bird_textures/dat_0cb960e4.inc"
};
static NJS_COLOR dat_0cb960f0[4] = {
#include "ev004b/bird_textures/dat_0cb960f0.inc"
};
static NJS_MESHSET dat_0cb96100[1] = {
#include "ev004b/bird_textures/dat_0cb96100.inc"
};
static NJS_POINT3 dat_0cb96118[4] = {
#include "ev004b/bird_textures/dat_0cb96118.inc"
};
static NJS_VECTOR dat_0cb96148[4] = {
#include "ev004b/bird_textures/dat_0cb96148.inc"
};
static NJS_MODEL dat_0cb96178 = {
#include "ev004b/bird_textures/dat_0cb96178.inc"
};
static NJS_OBJECT dat_0cb961a0 = {
#include "ev004b/bird_textures/dat_0cb961a0.inc"
};
static NJS_MATERIAL dat_0cb961d4[1] = {
#include "ev004b/bird_textures/dat_0cb961d4.inc"
};
static Sint16 dat_0cb961e8[12] = {
#include "ev004b/bird_textures/dat_0cb961e8.inc"
};
static NJS_COLOR dat_0cb96200[9] = {
#include "ev004b/bird_textures/dat_0cb96200.inc"
};
static NJS_MESHSET dat_0cb96224[1] = {
#include "ev004b/bird_textures/dat_0cb96224.inc"
};
static NJS_POINT3 dat_0cb9623c[7] = {
#include "ev004b/bird_textures/dat_0cb9623c.inc"
};
static NJS_VECTOR dat_0cb96290[7] = {
#include "ev004b/bird_textures/dat_0cb96290.inc"
};
static NJS_MODEL dat_0cb962e4 = {
#include "ev004b/bird_textures/dat_0cb962e4.inc"
};
static NJS_OBJECT dat_0cb9630c = {
#include "ev004b/bird_textures/dat_0cb9630c.inc"
};
static NJS_OBJECT dat_0cb96340 = {
#include "ev004b/bird_textures/dat_0cb96340.inc"
};
static NJS_OBJECT dat_0cb96374 = {
#include "ev004b/bird_textures/dat_0cb96374.inc"
};
static NJS_OBJECT dat_0cb963a8 = {
#include "ev004b/bird_textures/dat_0cb963a8.inc"
};
static NJS_OBJECT dat_0cb963dc = {
#include "ev004b/bird_textures/dat_0cb963dc.inc"
};
static NJS_OBJECT dat_0cb96410 = {
#include "ev004b/bird_textures/dat_0cb96410.inc"
};
static NJS_OBJECT dat_0cb96444 = {
#include "ev004b/bird_textures/dat_0cb96444.inc"
};
static NJS_OBJECT dat_0cb96478 = {
#include "ev004b/bird_textures/dat_0cb96478.inc"
};
static NJS_OBJECT dat_0cb964ac = {
#include "ev004b/bird_textures/dat_0cb964ac.inc"
};
static NJS_OBJECT dat_0cb964e0 = {
#include "ev004b/bird_textures/dat_0cb964e0.inc"
};
static NJS_OBJECT dat_0cb96514 = {
#include "ev004b/bird_textures/dat_0cb96514.inc"
};
static NJS_OBJECT dat_0cb96548 = {
#include "ev004b/bird_textures/dat_0cb96548.inc"
};
static NJS_MATERIAL dat_0cb9657c[1] = {
#include "ev004b/bird_textures/dat_0cb9657c.inc"
};
static Sint16 dat_0cb96590[18] = {
#include "ev004b/bird_textures/dat_0cb96590.inc"
};
static NJS_COLOR dat_0cb965b4[15] = {
#include "ev004b/bird_textures/dat_0cb965b4.inc"
};
static NJS_MESHSET dat_0cb965f0[1] = {
#include "ev004b/bird_textures/dat_0cb965f0.inc"
};
static NJS_POINT3 dat_0cb96608[7] = {
#include "ev004b/bird_textures/dat_0cb96608.inc"
};
static NJS_VECTOR dat_0cb9665c[7] = {
#include "ev004b/bird_textures/dat_0cb9665c.inc"
};
static NJS_MODEL dat_0cb966b0 = {
#include "ev004b/bird_textures/dat_0cb966b0.inc"
};
static NJS_OBJECT dat_0cb966d8 = {
#include "ev004b/bird_textures/dat_0cb966d8.inc"
};
static NJS_OBJECT dat_0cb9670c = {
#include "ev004b/bird_textures/dat_0cb9670c.inc"
};
static NJS_OBJECT dat_0cb96740 = {
#include "ev004b/bird_textures/dat_0cb96740.inc"
};
static NJS_OBJECT dat_0cb96774 = {
#include "ev004b/bird_textures/dat_0cb96774.inc"
};
static NJS_MATERIAL dat_0cb967a8[1] = {
#include "ev004b/bird_textures/dat_0cb967a8.inc"
};
static Sint16 dat_0cb967bc[10] = {
#include "ev004b/bird_textures/dat_0cb967bc.inc"
};
static NJS_COLOR dat_0cb967d0[8] = {
#include "ev004b/bird_textures/dat_0cb967d0.inc"
};
static NJS_MESHSET dat_0cb967f0[1] = {
#include "ev004b/bird_textures/dat_0cb967f0.inc"
};
static NJS_POINT3 dat_0cb96808[8] = {
#include "ev004b/bird_textures/dat_0cb96808.inc"
};
static NJS_VECTOR dat_0cb96868[8] = {
#include "ev004b/bird_textures/dat_0cb96868.inc"
};
static NJS_MODEL dat_0cb968c8 = {
#include "ev004b/bird_textures/dat_0cb968c8.inc"
};
static NJS_OBJECT dat_0cb968f0 = {
#include "ev004b/bird_textures/dat_0cb968f0.inc"
};
static NJS_MATERIAL dat_0cb96924[1] = {
#include "ev004b/bird_textures/dat_0cb96924.inc"
};
static Sint16 dat_0cb96938[10] = {
#include "ev004b/bird_textures/dat_0cb96938.inc"
};
static NJS_COLOR dat_0cb9694c[8] = {
#include "ev004b/bird_textures/dat_0cb9694c.inc"
};
static NJS_MESHSET dat_0cb9696c[1] = {
#include "ev004b/bird_textures/dat_0cb9696c.inc"
};
static NJS_POINT3 dat_0cb96984[8] = {
#include "ev004b/bird_textures/dat_0cb96984.inc"
};
static NJS_VECTOR dat_0cb969e4[8] = {
#include "ev004b/bird_textures/dat_0cb969e4.inc"
};
static NJS_MODEL dat_0cb96a44 = {
#include "ev004b/bird_textures/dat_0cb96a44.inc"
};
static NJS_OBJECT dat_0cb96a6c = {
#include "ev004b/bird_textures/dat_0cb96a6c.inc"
};
static NJS_MATERIAL dat_0cb96aa0[1] = {
#include "ev004b/bird_textures/dat_0cb96aa0.inc"
};
static Sint16 dat_0cb96ab4[16] = {
#include "ev004b/bird_textures/dat_0cb96ab4.inc"
};
static NJS_COLOR dat_0cb96ad4[12] = {
#include "ev004b/bird_textures/dat_0cb96ad4.inc"
};
static NJS_MESHSET dat_0cb96b04[1] = {
#include "ev004b/bird_textures/dat_0cb96b04.inc"
};
static NJS_POINT3 dat_0cb96b1c[12] = {
#include "ev004b/bird_textures/dat_0cb96b1c.inc"
};
static NJS_VECTOR dat_0cb96bac[12] = {
#include "ev004b/bird_textures/dat_0cb96bac.inc"
};
static NJS_MODEL dat_0cb96c3c = {
#include "ev004b/bird_textures/dat_0cb96c3c.inc"
};
static NJS_OBJECT dat_0cb96c64 = {
#include "ev004b/bird_textures/dat_0cb96c64.inc"
};
static NJS_MATERIAL dat_0cb96c98[1] = {
#include "ev004b/bird_textures/dat_0cb96c98.inc"
};
static Sint16 dat_0cb96cac[16] = {
#include "ev004b/bird_textures/dat_0cb96cac.inc"
};
static NJS_COLOR dat_0cb96ccc[12] = {
#include "ev004b/bird_textures/dat_0cb96ccc.inc"
};
static NJS_MESHSET dat_0cb96cfc[1] = {
#include "ev004b/bird_textures/dat_0cb96cfc.inc"
};
static NJS_POINT3 dat_0cb96d14[12] = {
#include "ev004b/bird_textures/dat_0cb96d14.inc"
};
static NJS_VECTOR dat_0cb96da4[12] = {
#include "ev004b/bird_textures/dat_0cb96da4.inc"
};
static NJS_MODEL dat_0cb96e34 = {
#include "ev004b/bird_textures/dat_0cb96e34.inc"
};
static NJS_OBJECT dat_0cb96e5c = {
#include "ev004b/bird_textures/dat_0cb96e5c.inc"
};
static NJS_MATERIAL dat_0cb96e90[4] = {
#include "ev004b/bird_textures/dat_0cb96e90.inc"
};
static Sint16 dat_0cb96ee0[239] = {
#include "ev004b/bird_textures/dat_0cb96ee0.inc"
};
static Sint16 dat_0cb970be[22] = {
#include "ev004b/bird_textures/dat_0cb970be.inc"
};
static Sint16 dat_0cb970ea[21] = {
#include "ev004b/bird_textures/dat_0cb970ea.inc"
};
static Sint16 dat_0cb97114[22] = {
#include "ev004b/bird_textures/dat_0cb97114.inc"
};
static NJS_COLOR dat_0cb97140[209] = {
#include "ev004b/bird_textures/dat_0cb97140.inc"
};
static NJS_COLOR dat_0cb97484[20] = {
#include "ev004b/bird_textures/dat_0cb97484.inc"
};
static NJS_COLOR dat_0cb974d4[16] = {
#include "ev004b/bird_textures/dat_0cb974d4.inc"
};
static NJS_COLOR dat_0cb97514[18] = {
#include "ev004b/bird_textures/dat_0cb97514.inc"
};
static NJS_MESHSET dat_0cb9755c[4] = {
#include "ev004b/bird_textures/dat_0cb9755c.inc"
};
static NJS_POINT3 dat_0cb975bc[127] = {
#include "ev004b/bird_textures/dat_0cb975bc.inc"
};
static NJS_VECTOR dat_0cb97bb0[127] = {
#include "ev004b/bird_textures/dat_0cb97bb0.inc"
};
static NJS_MODEL dat_0cb981a4 = {
#include "ev004b/bird_textures/dat_0cb981a4.inc"
};
static NJS_OBJECT dat_0cb981cc = {
#include "ev004b/bird_textures/dat_0cb981cc.inc"
};
static NJS_OBJECT dat_0cb98200 = {
#include "ev004b/bird_textures/dat_0cb98200.inc"
};
static NJS_OBJECT dat_0cb98234 = {
#include "ev004b/bird_textures/dat_0cb98234.inc"
};
static NJS_OBJECT dat_0cb98268 = {
#include "ev004b/bird_textures/dat_0cb98268.inc"
};
static NJS_MATERIAL dat_0cb9829c[1] = {
#include "ev004b/bird_textures/dat_0cb9829c.inc"
};
static Sint16 dat_0cb982b0[8] = {
#include "ev004b/bird_textures/dat_0cb982b0.inc"
};
static NJS_COLOR dat_0cb982c0[6] = {
#include "ev004b/bird_textures/dat_0cb982c0.inc"
};
static NJS_MESHSET dat_0cb982d8[1] = {
#include "ev004b/bird_textures/dat_0cb982d8.inc"
};
static NJS_POINT3 dat_0cb982f0[6] = {
#include "ev004b/bird_textures/dat_0cb982f0.inc"
};
static NJS_VECTOR dat_0cb98338[6] = {
#include "ev004b/bird_textures/dat_0cb98338.inc"
};
static NJS_MODEL dat_0cb98380 = {
#include "ev004b/bird_textures/dat_0cb98380.inc"
};
static NJS_OBJECT dat_0cb983a8 = {
#include "ev004b/bird_textures/dat_0cb983a8.inc"
};
static NJS_MATERIAL dat_0cb983dc[1] = {
#include "ev004b/bird_textures/dat_0cb983dc.inc"
};
static Sint16 dat_0cb983f0[22] = {
#include "ev004b/bird_textures/dat_0cb983f0.inc"
};
static NJS_COLOR dat_0cb9841c[16] = {
#include "ev004b/bird_textures/dat_0cb9841c.inc"
};
static NJS_MESHSET dat_0cb9845c[1] = {
#include "ev004b/bird_textures/dat_0cb9845c.inc"
};
static NJS_POINT3 dat_0cb98474[5] = {
#include "ev004b/bird_textures/dat_0cb98474.inc"
};
static NJS_VECTOR dat_0cb984b0[5] = {
#include "ev004b/bird_textures/dat_0cb984b0.inc"
};
static NJS_MODEL dat_0cb984ec = {
#include "ev004b/bird_textures/dat_0cb984ec.inc"
};
static NJS_OBJECT dat_0cb98514 = {
#include "ev004b/bird_textures/dat_0cb98514.inc"
};
static NJS_MATERIAL dat_0cb98548[1] = {
#include "ev004b/bird_textures/dat_0cb98548.inc"
};
static Sint16 dat_0cb9855c[30] = {
#include "ev004b/bird_textures/dat_0cb9855c.inc"
};
static NJS_COLOR dat_0cb98598[28] = {
#include "ev004b/bird_textures/dat_0cb98598.inc"
};
static NJS_MESHSET dat_0cb98608[1] = {
#include "ev004b/bird_textures/dat_0cb98608.inc"
};
static NJS_POINT3 dat_0cb98620[18] = {
#include "ev004b/bird_textures/dat_0cb98620.inc"
};
static NJS_VECTOR dat_0cb986f8[18] = {
#include "ev004b/bird_textures/dat_0cb986f8.inc"
};
static NJS_MODEL dat_0cb987d0 = {
#include "ev004b/bird_textures/dat_0cb987d0.inc"
};
static NJS_OBJECT dat_0cb987f8 = {
#include "ev004b/bird_textures/dat_0cb987f8.inc"
};
static NJS_MATERIAL dat_0cb9882c[1] = {
#include "ev004b/bird_textures/dat_0cb9882c.inc"
};
static Sint16 dat_0cb98840[20] = {
#include "ev004b/bird_textures/dat_0cb98840.inc"
};
static NJS_COLOR dat_0cb98868[16] = {
#include "ev004b/bird_textures/dat_0cb98868.inc"
};
static NJS_MESHSET dat_0cb988a8[1] = {
#include "ev004b/bird_textures/dat_0cb988a8.inc"
};
static NJS_POINT3 dat_0cb988c0[12] = {
#include "ev004b/bird_textures/dat_0cb988c0.inc"
};
static NJS_VECTOR dat_0cb98950[12] = {
#include "ev004b/bird_textures/dat_0cb98950.inc"
};
static NJS_MODEL dat_0cb989e0 = {
#include "ev004b/bird_textures/dat_0cb989e0.inc"
};
static NJS_OBJECT dat_0cb98a08 = {
#include "ev004b/bird_textures/dat_0cb98a08.inc"
};
static NJS_MATERIAL dat_0cb98a3c[1] = {
#include "ev004b/bird_textures/dat_0cb98a3c.inc"
};
static Sint16 dat_0cb98a50[10] = {
#include "ev004b/bird_textures/dat_0cb98a50.inc"
};
static NJS_COLOR dat_0cb98a64[8] = {
#include "ev004b/bird_textures/dat_0cb98a64.inc"
};
static NJS_MESHSET dat_0cb98a84[1] = {
#include "ev004b/bird_textures/dat_0cb98a84.inc"
};
static NJS_POINT3 dat_0cb98a9c[5] = {
#include "ev004b/bird_textures/dat_0cb98a9c.inc"
};
static NJS_VECTOR dat_0cb98ad8[5] = {
#include "ev004b/bird_textures/dat_0cb98ad8.inc"
};
static NJS_MODEL dat_0cb98b14 = {
#include "ev004b/bird_textures/dat_0cb98b14.inc"
};
static NJS_OBJECT dat_0cb98b3c = {
#include "ev004b/bird_textures/dat_0cb98b3c.inc"
};
static NJS_MATERIAL dat_0cb98b70[1] = {
#include "ev004b/bird_textures/dat_0cb98b70.inc"
};
static Sint16 dat_0cb98b84[10] = {
#include "ev004b/bird_textures/dat_0cb98b84.inc"
};
static NJS_COLOR dat_0cb98b98[8] = {
#include "ev004b/bird_textures/dat_0cb98b98.inc"
};
static NJS_MESHSET dat_0cb98bb8[1] = {
#include "ev004b/bird_textures/dat_0cb98bb8.inc"
};
static NJS_POINT3 dat_0cb98bd0[5] = {
#include "ev004b/bird_textures/dat_0cb98bd0.inc"
};
static NJS_VECTOR dat_0cb98c0c[5] = {
#include "ev004b/bird_textures/dat_0cb98c0c.inc"
};
static NJS_MODEL dat_0cb98c48 = {
#include "ev004b/bird_textures/dat_0cb98c48.inc"
};
static NJS_OBJECT dat_0cb98c70 = {
#include "ev004b/bird_textures/dat_0cb98c70.inc"
};
static NJS_MATERIAL dat_0cb98ca4[1] = {
#include "ev004b/bird_textures/dat_0cb98ca4.inc"
};
static Sint16 dat_0cb98cb8[24] = {
#include "ev004b/bird_textures/dat_0cb98cb8.inc"
};
static NJS_COLOR dat_0cb98ce8[20] = {
#include "ev004b/bird_textures/dat_0cb98ce8.inc"
};
static NJS_MESHSET dat_0cb98d38[1] = {
#include "ev004b/bird_textures/dat_0cb98d38.inc"
};
static NJS_POINT3 dat_0cb98d50[12] = {
#include "ev004b/bird_textures/dat_0cb98d50.inc"
};
static NJS_VECTOR dat_0cb98de0[12] = {
#include "ev004b/bird_textures/dat_0cb98de0.inc"
};
static NJS_MODEL dat_0cb98e70 = {
#include "ev004b/bird_textures/dat_0cb98e70.inc"
};
static NJS_OBJECT dat_0cb98e98 = {
#include "ev004b/bird_textures/dat_0cb98e98.inc"
};
static NJS_MATERIAL dat_0cb98ecc[1] = {
#include "ev004b/bird_textures/dat_0cb98ecc.inc"
};
static Sint16 dat_0cb98ee0[36] = {
#include "ev004b/bird_textures/dat_0cb98ee0.inc"
};
static NJS_COLOR dat_0cb98f28[30] = {
#include "ev004b/bird_textures/dat_0cb98f28.inc"
};
static NJS_MESHSET dat_0cb98fa0[1] = {
#include "ev004b/bird_textures/dat_0cb98fa0.inc"
};
static NJS_POINT3 dat_0cb98fb8[12] = {
#include "ev004b/bird_textures/dat_0cb98fb8.inc"
};
static NJS_VECTOR dat_0cb99048[12] = {
#include "ev004b/bird_textures/dat_0cb99048.inc"
};
static NJS_MODEL dat_0cb990d8 = {
#include "ev004b/bird_textures/dat_0cb990d8.inc"
};
static NJS_OBJECT dat_0cb99100 = {
#include "ev004b/bird_textures/dat_0cb99100.inc"
};
static NJS_OBJECT dat_0cb99134 = {
#include "ev004b/bird_textures/dat_0cb99134.inc"
};
static NJS_OBJECT dat_0cb99168 = {
#include "ev004b/bird_textures/dat_0cb99168.inc"
};
static NJS_OBJECT dat_0cb9919c = {
#include "ev004b/bird_textures/dat_0cb9919c.inc"
};
static NJS_MATERIAL dat_0cb991d0[1] = {
#include "ev004b/bird_textures/dat_0cb991d0.inc"
};
static Sint16 dat_0cb991e4[48] = {
#include "ev004b/bird_textures/dat_0cb991e4.inc"
};
static NJS_COLOR dat_0cb99244[38] = {
#include "ev004b/bird_textures/dat_0cb99244.inc"
};
static NJS_MESHSET dat_0cb992dc[1] = {
#include "ev004b/bird_textures/dat_0cb992dc.inc"
};
static NJS_POINT3 dat_0cb992f4[12] = {
#include "ev004b/bird_textures/dat_0cb992f4.inc"
};
static NJS_VECTOR dat_0cb99384[12] = {
#include "ev004b/bird_textures/dat_0cb99384.inc"
};
static NJS_MODEL dat_0cb99414 = {
#include "ev004b/bird_textures/dat_0cb99414.inc"
};
static NJS_OBJECT dat_0cb9943c = {
#include "ev004b/bird_textures/dat_0cb9943c.inc"
};
static NJS_OBJECT dat_0cb99470 = {
#include "ev004b/bird_textures/dat_0cb99470.inc"
};
static NJS_OBJECT dat_0cb994a4 = {
#include "ev004b/bird_textures/dat_0cb994a4.inc"
};
static NJS_OBJECT dat_0cb994d8 = {
#include "ev004b/bird_textures/dat_0cb994d8.inc"
};
static NJS_MATERIAL dat_0cb9950c[1] = {
#include "ev004b/bird_textures/dat_0cb9950c.inc"
};
static Sint16 dat_0cb99520[48] = {
#include "ev004b/bird_textures/dat_0cb99520.inc"
};
static NJS_COLOR dat_0cb99580[38] = {
#include "ev004b/bird_textures/dat_0cb99580.inc"
};
static NJS_MESHSET dat_0cb99618[1] = {
#include "ev004b/bird_textures/dat_0cb99618.inc"
};
static NJS_POINT3 dat_0cb99630[12] = {
#include "ev004b/bird_textures/dat_0cb99630.inc"
};
static NJS_VECTOR dat_0cb996c0[12] = {
#include "ev004b/bird_textures/dat_0cb996c0.inc"
};
static NJS_MODEL dat_0cb99750 = {
#include "ev004b/bird_textures/dat_0cb99750.inc"
};
static NJS_OBJECT dat_0cb99778 = {
#include "ev004b/bird_textures/dat_0cb99778.inc"
};
static NJS_OBJECT dat_0cb997ac = {
#include "ev004b/bird_textures/dat_0cb997ac.inc"
};
static NJS_OBJECT dat_0cb997e0 = {
#include "ev004b/bird_textures/dat_0cb997e0.inc"
};
static NJS_OBJECT dat_0cb99814 = {
#include "ev004b/bird_textures/dat_0cb99814.inc"
};
static NJS_MATERIAL dat_0cb99848[2] = {
#include "ev004b/bird_textures/dat_0cb99848.inc"
};
static Sint16 dat_0cb99870[54] = {
#include "ev004b/bird_textures/dat_0cb99870.inc"
};
static Sint16 dat_0cb998dc[22] = {
#include "ev004b/bird_textures/dat_0cb998dc.inc"
};
static NJS_COLOR dat_0cb99908[48] = {
#include "ev004b/bird_textures/dat_0cb99908.inc"
};
static NJS_COLOR dat_0cb999c8[18] = {
#include "ev004b/bird_textures/dat_0cb999c8.inc"
};
static NJS_MESHSET dat_0cb99a10[2] = {
#include "ev004b/bird_textures/dat_0cb99a10.inc"
};
static NJS_POINT3 dat_0cb99a40[26] = {
#include "ev004b/bird_textures/dat_0cb99a40.inc"
};
static NJS_VECTOR dat_0cb99b78[26] = {
#include "ev004b/bird_textures/dat_0cb99b78.inc"
};
static NJS_MODEL dat_0cb99cb0 = {
#include "ev004b/bird_textures/dat_0cb99cb0.inc"
};
static NJS_OBJECT dat_0cb99cd8 = {
#include "ev004b/bird_textures/dat_0cb99cd8.inc"
};
static NJS_OBJECT dat_0cb99d0c = {
#include "ev004b/bird_textures/dat_0cb99d0c.inc"
};
static NJS_OBJECT dat_0cb99d40 = {
#include "ev004b/bird_textures/dat_0cb99d40.inc"
};
static NJS_OBJECT dat_0cb99d74 = {
#include "ev004b/bird_textures/dat_0cb99d74.inc"
};
/* the root of a tree that matches ev004b_bird_object's but for five nodes.
 * each of those is scaled to 1e-5, and its model's radius is 5e-6, not
 * about 0.5. only dat_0cb9e96c moves this tree */
static NJS_OBJECT dat_0cb99da8 = {
#include "ev004b/bird_textures/dat_0cb99da8.inc"
};
static NJS_MATERIAL dat_0cb99ddc[1] = {
#include "ev004b/bird_textures/dat_0cb99ddc.inc"
};
static Sint16 dat_0cb99df0[6] = {
#include "ev004b/bird_textures/dat_0cb99df0.inc"
};
static NJS_COLOR dat_0cb99dfc[4] = {
#include "ev004b/bird_textures/dat_0cb99dfc.inc"
};
static NJS_MESHSET dat_0cb99e0c[1] = {
#include "ev004b/bird_textures/dat_0cb99e0c.inc"
};
static NJS_POINT3 dat_0cb99e24[4] = {
#include "ev004b/bird_textures/dat_0cb99e24.inc"
};
static NJS_VECTOR dat_0cb99e54[4] = {
#include "ev004b/bird_textures/dat_0cb99e54.inc"
};
static NJS_MODEL dat_0cb99e84 = {
#include "ev004b/bird_textures/dat_0cb99e84.inc"
};
static NJS_OBJECT dat_0cb99eac = {
#include "ev004b/bird_textures/dat_0cb99eac.inc"
};
static NJS_MATERIAL dat_0cb99ee0[1] = {
#include "ev004b/bird_textures/dat_0cb99ee0.inc"
};
static Sint16 dat_0cb99ef4[12] = {
#include "ev004b/bird_textures/dat_0cb99ef4.inc"
};
static NJS_COLOR dat_0cb99f0c[9] = {
#include "ev004b/bird_textures/dat_0cb99f0c.inc"
};
static NJS_MESHSET dat_0cb99f30[1] = {
#include "ev004b/bird_textures/dat_0cb99f30.inc"
};
static NJS_POINT3 dat_0cb99f48[7] = {
#include "ev004b/bird_textures/dat_0cb99f48.inc"
};
static NJS_VECTOR dat_0cb99f9c[7] = {
#include "ev004b/bird_textures/dat_0cb99f9c.inc"
};
static NJS_MODEL dat_0cb99ff0 = {
#include "ev004b/bird_textures/dat_0cb99ff0.inc"
};
static NJS_OBJECT dat_0cb9a018 = {
#include "ev004b/bird_textures/dat_0cb9a018.inc"
};
static NJS_OBJECT dat_0cb9a04c = {
#include "ev004b/bird_textures/dat_0cb9a04c.inc"
};
static NJS_OBJECT dat_0cb9a080 = {
#include "ev004b/bird_textures/dat_0cb9a080.inc"
};
static NJS_OBJECT dat_0cb9a0b4 = {
#include "ev004b/bird_textures/dat_0cb9a0b4.inc"
};
static NJS_OBJECT dat_0cb9a0e8 = {
#include "ev004b/bird_textures/dat_0cb9a0e8.inc"
};
static NJS_OBJECT dat_0cb9a11c = {
#include "ev004b/bird_textures/dat_0cb9a11c.inc"
};
static NJS_OBJECT dat_0cb9a150 = {
#include "ev004b/bird_textures/dat_0cb9a150.inc"
};
static NJS_MATERIAL dat_0cb9a184[1] = {
#include "ev004b/bird_textures/dat_0cb9a184.inc"
};
static Sint16 dat_0cb9a198[6] = {
#include "ev004b/bird_textures/dat_0cb9a198.inc"
};
static NJS_COLOR dat_0cb9a1a4[4] = {
#include "ev004b/bird_textures/dat_0cb9a1a4.inc"
};
static NJS_MESHSET dat_0cb9a1b4[1] = {
#include "ev004b/bird_textures/dat_0cb9a1b4.inc"
};
static NJS_POINT3 dat_0cb9a1cc[4] = {
#include "ev004b/bird_textures/dat_0cb9a1cc.inc"
};
static NJS_VECTOR dat_0cb9a1fc[4] = {
#include "ev004b/bird_textures/dat_0cb9a1fc.inc"
};
static NJS_MODEL dat_0cb9a22c = {
#include "ev004b/bird_textures/dat_0cb9a22c.inc"
};
static NJS_OBJECT dat_0cb9a254 = {
#include "ev004b/bird_textures/dat_0cb9a254.inc"
};
static NJS_MATERIAL dat_0cb9a288[1] = {
#include "ev004b/bird_textures/dat_0cb9a288.inc"
};
static Sint16 dat_0cb9a29c[12] = {
#include "ev004b/bird_textures/dat_0cb9a29c.inc"
};
static NJS_COLOR dat_0cb9a2b4[9] = {
#include "ev004b/bird_textures/dat_0cb9a2b4.inc"
};
static NJS_MESHSET dat_0cb9a2d8[1] = {
#include "ev004b/bird_textures/dat_0cb9a2d8.inc"
};
static NJS_POINT3 dat_0cb9a2f0[7] = {
#include "ev004b/bird_textures/dat_0cb9a2f0.inc"
};
static NJS_VECTOR dat_0cb9a344[7] = {
#include "ev004b/bird_textures/dat_0cb9a344.inc"
};
static NJS_MODEL dat_0cb9a398 = {
#include "ev004b/bird_textures/dat_0cb9a398.inc"
};
static NJS_OBJECT dat_0cb9a3c0 = {
#include "ev004b/bird_textures/dat_0cb9a3c0.inc"
};
static NJS_OBJECT dat_0cb9a3f4 = {
#include "ev004b/bird_textures/dat_0cb9a3f4.inc"
};
static NJS_OBJECT dat_0cb9a428 = {
#include "ev004b/bird_textures/dat_0cb9a428.inc"
};
static NJS_OBJECT dat_0cb9a45c = {
#include "ev004b/bird_textures/dat_0cb9a45c.inc"
};
static NJS_OBJECT dat_0cb9a490 = {
#include "ev004b/bird_textures/dat_0cb9a490.inc"
};
static NJS_OBJECT dat_0cb9a4c4 = {
#include "ev004b/bird_textures/dat_0cb9a4c4.inc"
};
static NJS_OBJECT dat_0cb9a4f8 = {
#include "ev004b/bird_textures/dat_0cb9a4f8.inc"
};
static NJS_OBJECT dat_0cb9a52c = {
#include "ev004b/bird_textures/dat_0cb9a52c.inc"
};
static NJS_OBJECT dat_0cb9a560 = {
#include "ev004b/bird_textures/dat_0cb9a560.inc"
};
static NJS_OBJECT dat_0cb9a594 = {
#include "ev004b/bird_textures/dat_0cb9a594.inc"
};
static NJS_OBJECT dat_0cb9a5c8 = {
#include "ev004b/bird_textures/dat_0cb9a5c8.inc"
};
static NJS_OBJECT dat_0cb9a5fc = {
#include "ev004b/bird_textures/dat_0cb9a5fc.inc"
};
static NJS_MATERIAL dat_0cb9a630[1] = {
#include "ev004b/bird_textures/dat_0cb9a630.inc"
};
static Sint16 dat_0cb9a644[18] = {
#include "ev004b/bird_textures/dat_0cb9a644.inc"
};
static NJS_COLOR dat_0cb9a668[15] = {
#include "ev004b/bird_textures/dat_0cb9a668.inc"
};
static NJS_MESHSET dat_0cb9a6a4[1] = {
#include "ev004b/bird_textures/dat_0cb9a6a4.inc"
};
static NJS_POINT3 dat_0cb9a6bc[7] = {
#include "ev004b/bird_textures/dat_0cb9a6bc.inc"
};
static NJS_VECTOR dat_0cb9a710[7] = {
#include "ev004b/bird_textures/dat_0cb9a710.inc"
};
static NJS_MODEL dat_0cb9a764 = {
#include "ev004b/bird_textures/dat_0cb9a764.inc"
};
static NJS_OBJECT dat_0cb9a78c = {
#include "ev004b/bird_textures/dat_0cb9a78c.inc"
};
static NJS_OBJECT dat_0cb9a7c0 = {
#include "ev004b/bird_textures/dat_0cb9a7c0.inc"
};
static NJS_OBJECT dat_0cb9a7f4 = {
#include "ev004b/bird_textures/dat_0cb9a7f4.inc"
};
static NJS_OBJECT dat_0cb9a828 = {
#include "ev004b/bird_textures/dat_0cb9a828.inc"
};
static NJS_MATERIAL dat_0cb9a85c[1] = {
#include "ev004b/bird_textures/dat_0cb9a85c.inc"
};
static Sint16 dat_0cb9a870[10] = {
#include "ev004b/bird_textures/dat_0cb9a870.inc"
};
static NJS_COLOR dat_0cb9a884[8] = {
#include "ev004b/bird_textures/dat_0cb9a884.inc"
};
static NJS_MESHSET dat_0cb9a8a4[1] = {
#include "ev004b/bird_textures/dat_0cb9a8a4.inc"
};
static NJS_POINT3 dat_0cb9a8bc[8] = {
#include "ev004b/bird_textures/dat_0cb9a8bc.inc"
};
static NJS_VECTOR dat_0cb9a91c[8] = {
#include "ev004b/bird_textures/dat_0cb9a91c.inc"
};
static NJS_MODEL dat_0cb9a97c = {
#include "ev004b/bird_textures/dat_0cb9a97c.inc"
};
static NJS_OBJECT dat_0cb9a9a4 = {
#include "ev004b/bird_textures/dat_0cb9a9a4.inc"
};
static NJS_MATERIAL dat_0cb9a9d8[1] = {
#include "ev004b/bird_textures/dat_0cb9a9d8.inc"
};
static Sint16 dat_0cb9a9ec[10] = {
#include "ev004b/bird_textures/dat_0cb9a9ec.inc"
};
static NJS_COLOR dat_0cb9aa00[8] = {
#include "ev004b/bird_textures/dat_0cb9aa00.inc"
};
static NJS_MESHSET dat_0cb9aa20[1] = {
#include "ev004b/bird_textures/dat_0cb9aa20.inc"
};
static NJS_POINT3 dat_0cb9aa38[8] = {
#include "ev004b/bird_textures/dat_0cb9aa38.inc"
};
static NJS_VECTOR dat_0cb9aa98[8] = {
#include "ev004b/bird_textures/dat_0cb9aa98.inc"
};
static NJS_MODEL dat_0cb9aaf8 = {
#include "ev004b/bird_textures/dat_0cb9aaf8.inc"
};
static NJS_OBJECT dat_0cb9ab20 = {
#include "ev004b/bird_textures/dat_0cb9ab20.inc"
};
static NJS_MATERIAL dat_0cb9ab54[1] = {
#include "ev004b/bird_textures/dat_0cb9ab54.inc"
};
static Sint16 dat_0cb9ab68[16] = {
#include "ev004b/bird_textures/dat_0cb9ab68.inc"
};
static NJS_COLOR dat_0cb9ab88[12] = {
#include "ev004b/bird_textures/dat_0cb9ab88.inc"
};
static NJS_MESHSET dat_0cb9abb8[1] = {
#include "ev004b/bird_textures/dat_0cb9abb8.inc"
};
static NJS_POINT3 dat_0cb9abd0[12] = {
#include "ev004b/bird_textures/dat_0cb9abd0.inc"
};
static NJS_VECTOR dat_0cb9ac60[12] = {
#include "ev004b/bird_textures/dat_0cb9ac60.inc"
};
static NJS_MODEL dat_0cb9acf0 = {
#include "ev004b/bird_textures/dat_0cb9acf0.inc"
};
static NJS_OBJECT dat_0cb9ad18 = {
#include "ev004b/bird_textures/dat_0cb9ad18.inc"
};
static NJS_MATERIAL dat_0cb9ad4c[1] = {
#include "ev004b/bird_textures/dat_0cb9ad4c.inc"
};
static Sint16 dat_0cb9ad60[16] = {
#include "ev004b/bird_textures/dat_0cb9ad60.inc"
};
static NJS_COLOR dat_0cb9ad80[12] = {
#include "ev004b/bird_textures/dat_0cb9ad80.inc"
};
static NJS_MESHSET dat_0cb9adb0[1] = {
#include "ev004b/bird_textures/dat_0cb9adb0.inc"
};
static NJS_POINT3 dat_0cb9adc8[12] = {
#include "ev004b/bird_textures/dat_0cb9adc8.inc"
};
static NJS_VECTOR dat_0cb9ae58[12] = {
#include "ev004b/bird_textures/dat_0cb9ae58.inc"
};
static NJS_MODEL dat_0cb9aee8 = {
#include "ev004b/bird_textures/dat_0cb9aee8.inc"
};
static NJS_OBJECT dat_0cb9af10 = {
#include "ev004b/bird_textures/dat_0cb9af10.inc"
};
static NJS_MATERIAL dat_0cb9af44[4] = {
#include "ev004b/bird_textures/dat_0cb9af44.inc"
};
static Sint16 dat_0cb9af94[239] = {
#include "ev004b/bird_textures/dat_0cb9af94.inc"
};
static Sint16 dat_0cb9b172[22] = {
#include "ev004b/bird_textures/dat_0cb9b172.inc"
};
static Sint16 dat_0cb9b19e[21] = {
#include "ev004b/bird_textures/dat_0cb9b19e.inc"
};
static Sint16 dat_0cb9b1c8[22] = {
#include "ev004b/bird_textures/dat_0cb9b1c8.inc"
};
static NJS_COLOR dat_0cb9b1f4[209] = {
#include "ev004b/bird_textures/dat_0cb9b1f4.inc"
};
static NJS_COLOR dat_0cb9b538[20] = {
#include "ev004b/bird_textures/dat_0cb9b538.inc"
};
static NJS_COLOR dat_0cb9b588[16] = {
#include "ev004b/bird_textures/dat_0cb9b588.inc"
};
static NJS_COLOR dat_0cb9b5c8[18] = {
#include "ev004b/bird_textures/dat_0cb9b5c8.inc"
};
static NJS_MESHSET dat_0cb9b610[4] = {
#include "ev004b/bird_textures/dat_0cb9b610.inc"
};
static NJS_POINT3 dat_0cb9b670[127] = {
#include "ev004b/bird_textures/dat_0cb9b670.inc"
};
static NJS_VECTOR dat_0cb9bc64[127] = {
#include "ev004b/bird_textures/dat_0cb9bc64.inc"
};
static NJS_MODEL dat_0cb9c258 = {
#include "ev004b/bird_textures/dat_0cb9c258.inc"
};
static NJS_OBJECT dat_0cb9c280 = {
#include "ev004b/bird_textures/dat_0cb9c280.inc"
};
static NJS_OBJECT dat_0cb9c2b4 = {
#include "ev004b/bird_textures/dat_0cb9c2b4.inc"
};
static NJS_OBJECT dat_0cb9c2e8 = {
#include "ev004b/bird_textures/dat_0cb9c2e8.inc"
};
static NJS_OBJECT dat_0cb9c31c = {
#include "ev004b/bird_textures/dat_0cb9c31c.inc"
};
static NJS_MATERIAL dat_0cb9c350[1] = {
#include "ev004b/bird_textures/dat_0cb9c350.inc"
};
static Sint16 dat_0cb9c364[8] = {
#include "ev004b/bird_textures/dat_0cb9c364.inc"
};
static NJS_COLOR dat_0cb9c374[6] = {
#include "ev004b/bird_textures/dat_0cb9c374.inc"
};
static NJS_MESHSET dat_0cb9c38c[1] = {
#include "ev004b/bird_textures/dat_0cb9c38c.inc"
};
static NJS_POINT3 dat_0cb9c3a4[6] = {
#include "ev004b/bird_textures/dat_0cb9c3a4.inc"
};
static NJS_VECTOR dat_0cb9c3ec[6] = {
#include "ev004b/bird_textures/dat_0cb9c3ec.inc"
};
static NJS_MODEL dat_0cb9c434 = {
#include "ev004b/bird_textures/dat_0cb9c434.inc"
};
static NJS_OBJECT dat_0cb9c45c = {
#include "ev004b/bird_textures/dat_0cb9c45c.inc"
};
static NJS_MATERIAL dat_0cb9c490[1] = {
#include "ev004b/bird_textures/dat_0cb9c490.inc"
};
static Sint16 dat_0cb9c4a4[22] = {
#include "ev004b/bird_textures/dat_0cb9c4a4.inc"
};
static NJS_COLOR dat_0cb9c4d0[16] = {
#include "ev004b/bird_textures/dat_0cb9c4d0.inc"
};
static NJS_MESHSET dat_0cb9c510[1] = {
#include "ev004b/bird_textures/dat_0cb9c510.inc"
};
static NJS_POINT3 dat_0cb9c528[5] = {
#include "ev004b/bird_textures/dat_0cb9c528.inc"
};
static NJS_VECTOR dat_0cb9c564[5] = {
#include "ev004b/bird_textures/dat_0cb9c564.inc"
};
static NJS_MODEL dat_0cb9c5a0 = {
#include "ev004b/bird_textures/dat_0cb9c5a0.inc"
};
static NJS_OBJECT dat_0cb9c5c8 = {
#include "ev004b/bird_textures/dat_0cb9c5c8.inc"
};
static NJS_MATERIAL dat_0cb9c5fc[1] = {
#include "ev004b/bird_textures/dat_0cb9c5fc.inc"
};
static Sint16 dat_0cb9c610[30] = {
#include "ev004b/bird_textures/dat_0cb9c610.inc"
};
static NJS_COLOR dat_0cb9c64c[28] = {
#include "ev004b/bird_textures/dat_0cb9c64c.inc"
};
static NJS_MESHSET dat_0cb9c6bc[1] = {
#include "ev004b/bird_textures/dat_0cb9c6bc.inc"
};
static NJS_POINT3 dat_0cb9c6d4[18] = {
#include "ev004b/bird_textures/dat_0cb9c6d4.inc"
};
static NJS_VECTOR dat_0cb9c7ac[18] = {
#include "ev004b/bird_textures/dat_0cb9c7ac.inc"
};
static NJS_MODEL dat_0cb9c884 = {
#include "ev004b/bird_textures/dat_0cb9c884.inc"
};
static NJS_OBJECT dat_0cb9c8ac = {
#include "ev004b/bird_textures/dat_0cb9c8ac.inc"
};
static NJS_MATERIAL dat_0cb9c8e0[1] = {
#include "ev004b/bird_textures/dat_0cb9c8e0.inc"
};
static Sint16 dat_0cb9c8f4[20] = {
#include "ev004b/bird_textures/dat_0cb9c8f4.inc"
};
static NJS_COLOR dat_0cb9c91c[16] = {
#include "ev004b/bird_textures/dat_0cb9c91c.inc"
};
static NJS_MESHSET dat_0cb9c95c[1] = {
#include "ev004b/bird_textures/dat_0cb9c95c.inc"
};
static NJS_POINT3 dat_0cb9c974[12] = {
#include "ev004b/bird_textures/dat_0cb9c974.inc"
};
static NJS_VECTOR dat_0cb9ca04[12] = {
#include "ev004b/bird_textures/dat_0cb9ca04.inc"
};
static NJS_MODEL dat_0cb9ca94 = {
#include "ev004b/bird_textures/dat_0cb9ca94.inc"
};
static NJS_OBJECT dat_0cb9cabc = {
#include "ev004b/bird_textures/dat_0cb9cabc.inc"
};
static NJS_MATERIAL dat_0cb9caf0[1] = {
#include "ev004b/bird_textures/dat_0cb9caf0.inc"
};
static Sint16 dat_0cb9cb04[10] = {
#include "ev004b/bird_textures/dat_0cb9cb04.inc"
};
static NJS_COLOR dat_0cb9cb18[8] = {
#include "ev004b/bird_textures/dat_0cb9cb18.inc"
};
static NJS_MESHSET dat_0cb9cb38[1] = {
#include "ev004b/bird_textures/dat_0cb9cb38.inc"
};
static NJS_POINT3 dat_0cb9cb50[5] = {
#include "ev004b/bird_textures/dat_0cb9cb50.inc"
};
static NJS_VECTOR dat_0cb9cb8c[5] = {
#include "ev004b/bird_textures/dat_0cb9cb8c.inc"
};
static NJS_MODEL dat_0cb9cbc8 = {
#include "ev004b/bird_textures/dat_0cb9cbc8.inc"
};
static NJS_OBJECT dat_0cb9cbf0 = {
#include "ev004b/bird_textures/dat_0cb9cbf0.inc"
};
static NJS_MATERIAL dat_0cb9cc24[1] = {
#include "ev004b/bird_textures/dat_0cb9cc24.inc"
};
static Sint16 dat_0cb9cc38[10] = {
#include "ev004b/bird_textures/dat_0cb9cc38.inc"
};
static NJS_COLOR dat_0cb9cc4c[8] = {
#include "ev004b/bird_textures/dat_0cb9cc4c.inc"
};
static NJS_MESHSET dat_0cb9cc6c[1] = {
#include "ev004b/bird_textures/dat_0cb9cc6c.inc"
};
static NJS_POINT3 dat_0cb9cc84[5] = {
#include "ev004b/bird_textures/dat_0cb9cc84.inc"
};
static NJS_VECTOR dat_0cb9ccc0[5] = {
#include "ev004b/bird_textures/dat_0cb9ccc0.inc"
};
static NJS_MODEL dat_0cb9ccfc = {
#include "ev004b/bird_textures/dat_0cb9ccfc.inc"
};
static NJS_OBJECT dat_0cb9cd24 = {
#include "ev004b/bird_textures/dat_0cb9cd24.inc"
};
static NJS_MATERIAL dat_0cb9cd58[1] = {
#include "ev004b/bird_textures/dat_0cb9cd58.inc"
};
static Sint16 dat_0cb9cd6c[24] = {
#include "ev004b/bird_textures/dat_0cb9cd6c.inc"
};
static NJS_COLOR dat_0cb9cd9c[20] = {
#include "ev004b/bird_textures/dat_0cb9cd9c.inc"
};
static NJS_MESHSET dat_0cb9cdec[1] = {
#include "ev004b/bird_textures/dat_0cb9cdec.inc"
};
static NJS_POINT3 dat_0cb9ce04[12] = {
#include "ev004b/bird_textures/dat_0cb9ce04.inc"
};
static NJS_VECTOR dat_0cb9ce94[12] = {
#include "ev004b/bird_textures/dat_0cb9ce94.inc"
};
static NJS_MODEL dat_0cb9cf24 = {
#include "ev004b/bird_textures/dat_0cb9cf24.inc"
};
static NJS_OBJECT dat_0cb9cf4c = {
#include "ev004b/bird_textures/dat_0cb9cf4c.inc"
};
static NJS_MATERIAL dat_0cb9cf80[1] = {
#include "ev004b/bird_textures/dat_0cb9cf80.inc"
};
static Sint16 dat_0cb9cf94[36] = {
#include "ev004b/bird_textures/dat_0cb9cf94.inc"
};
static NJS_COLOR dat_0cb9cfdc[30] = {
#include "ev004b/bird_textures/dat_0cb9cfdc.inc"
};
static NJS_MESHSET dat_0cb9d054[1] = {
#include "ev004b/bird_textures/dat_0cb9d054.inc"
};
static NJS_POINT3 dat_0cb9d06c[12] = {
#include "ev004b/bird_textures/dat_0cb9d06c.inc"
};
static NJS_VECTOR dat_0cb9d0fc[12] = {
#include "ev004b/bird_textures/dat_0cb9d0fc.inc"
};
static NJS_MODEL dat_0cb9d18c = {
#include "ev004b/bird_textures/dat_0cb9d18c.inc"
};
static NJS_OBJECT dat_0cb9d1b4 = {
#include "ev004b/bird_textures/dat_0cb9d1b4.inc"
};
static NJS_OBJECT dat_0cb9d1e8 = {
#include "ev004b/bird_textures/dat_0cb9d1e8.inc"
};
static NJS_OBJECT dat_0cb9d21c = {
#include "ev004b/bird_textures/dat_0cb9d21c.inc"
};
static NJS_OBJECT dat_0cb9d250 = {
#include "ev004b/bird_textures/dat_0cb9d250.inc"
};
static NJS_MATERIAL dat_0cb9d284[1] = {
#include "ev004b/bird_textures/dat_0cb9d284.inc"
};
static Sint16 dat_0cb9d298[48] = {
#include "ev004b/bird_textures/dat_0cb9d298.inc"
};
static NJS_COLOR dat_0cb9d2f8[38] = {
#include "ev004b/bird_textures/dat_0cb9d2f8.inc"
};
static NJS_MESHSET dat_0cb9d390[1] = {
#include "ev004b/bird_textures/dat_0cb9d390.inc"
};
static NJS_POINT3 dat_0cb9d3a8[12] = {
#include "ev004b/bird_textures/dat_0cb9d3a8.inc"
};
static NJS_VECTOR dat_0cb9d438[12] = {
#include "ev004b/bird_textures/dat_0cb9d438.inc"
};
static NJS_MODEL dat_0cb9d4c8 = {
#include "ev004b/bird_textures/dat_0cb9d4c8.inc"
};
static NJS_OBJECT dat_0cb9d4f0 = {
#include "ev004b/bird_textures/dat_0cb9d4f0.inc"
};
static NJS_OBJECT dat_0cb9d524 = {
#include "ev004b/bird_textures/dat_0cb9d524.inc"
};
static NJS_OBJECT dat_0cb9d558 = {
#include "ev004b/bird_textures/dat_0cb9d558.inc"
};
static NJS_OBJECT dat_0cb9d58c = {
#include "ev004b/bird_textures/dat_0cb9d58c.inc"
};
static NJS_MATERIAL dat_0cb9d5c0[1] = {
#include "ev004b/bird_textures/dat_0cb9d5c0.inc"
};
static Sint16 dat_0cb9d5d4[48] = {
#include "ev004b/bird_textures/dat_0cb9d5d4.inc"
};
static NJS_COLOR dat_0cb9d634[38] = {
#include "ev004b/bird_textures/dat_0cb9d634.inc"
};
static NJS_MESHSET dat_0cb9d6cc[1] = {
#include "ev004b/bird_textures/dat_0cb9d6cc.inc"
};
static NJS_POINT3 dat_0cb9d6e4[12] = {
#include "ev004b/bird_textures/dat_0cb9d6e4.inc"
};
static NJS_VECTOR dat_0cb9d774[12] = {
#include "ev004b/bird_textures/dat_0cb9d774.inc"
};
static NJS_MODEL dat_0cb9d804 = {
#include "ev004b/bird_textures/dat_0cb9d804.inc"
};
static NJS_OBJECT dat_0cb9d82c = {
#include "ev004b/bird_textures/dat_0cb9d82c.inc"
};
static NJS_OBJECT dat_0cb9d860 = {
#include "ev004b/bird_textures/dat_0cb9d860.inc"
};
static NJS_OBJECT dat_0cb9d894 = {
#include "ev004b/bird_textures/dat_0cb9d894.inc"
};
static NJS_OBJECT dat_0cb9d8c8 = {
#include "ev004b/bird_textures/dat_0cb9d8c8.inc"
};
static NJS_MATERIAL dat_0cb9d8fc[2] = {
#include "ev004b/bird_textures/dat_0cb9d8fc.inc"
};
static Sint16 dat_0cb9d924[54] = {
#include "ev004b/bird_textures/dat_0cb9d924.inc"
};
static Sint16 dat_0cb9d990[22] = {
#include "ev004b/bird_textures/dat_0cb9d990.inc"
};
static NJS_COLOR dat_0cb9d9bc[48] = {
#include "ev004b/bird_textures/dat_0cb9d9bc.inc"
};
static NJS_COLOR dat_0cb9da7c[18] = {
#include "ev004b/bird_textures/dat_0cb9da7c.inc"
};
static NJS_MESHSET dat_0cb9dac4[2] = {
#include "ev004b/bird_textures/dat_0cb9dac4.inc"
};
static NJS_POINT3 dat_0cb9daf4[26] = {
#include "ev004b/bird_textures/dat_0cb9daf4.inc"
};
static NJS_VECTOR dat_0cb9dc2c[26] = {
#include "ev004b/bird_textures/dat_0cb9dc2c.inc"
};
static NJS_MODEL dat_0cb9dd64 = {
#include "ev004b/bird_textures/dat_0cb9dd64.inc"
};
static NJS_OBJECT dat_0cb9dd8c = {
#include "ev004b/bird_textures/dat_0cb9dd8c.inc"
};
static NJS_OBJECT dat_0cb9ddc0 = {
#include "ev004b/bird_textures/dat_0cb9ddc0.inc"
};
static NJS_OBJECT dat_0cb9ddf4 = {
#include "ev004b/bird_textures/dat_0cb9ddf4.inc"
};
static NJS_OBJECT dat_0cb9de28 = {
#include "ev004b/bird_textures/dat_0cb9de28.inc"
};
static NJS_OBJECT ev004b_bird_object = {
#include "ev004b/bird_textures/ev004b_bird_object.inc"
};
static NJS_MKEY_F dat_0cb9de90[2] = {
#include "ev004b/bird_textures/dat_0cb9de90.inc"
};
static NJS_MKEY_A dat_0cb9deb0[2] = {
#include "ev004b/bird_textures/dat_0cb9deb0.inc"
};
static NJS_MKEY_F dat_0cb9ded0[2] = {
#include "ev004b/bird_textures/dat_0cb9ded0.inc"
};
static NJS_MKEY_F dat_0cb9def0[2] = {
#include "ev004b/bird_textures/dat_0cb9def0.inc"
};
static NJS_MKEY_A dat_0cb9df10[2] = {
#include "ev004b/bird_textures/dat_0cb9df10.inc"
};
static NJS_MKEY_A dat_0cb9df30[2] = {
#include "ev004b/bird_textures/dat_0cb9df30.inc"
};
static NJS_MKEY_F dat_0cb9df50[2] = {
#include "ev004b/bird_textures/dat_0cb9df50.inc"
};
static NJS_MKEY_F dat_0cb9df70[2] = {
#include "ev004b/bird_textures/dat_0cb9df70.inc"
};
static NJS_MKEY_A dat_0cb9df90[2] = {
#include "ev004b/bird_textures/dat_0cb9df90.inc"
};
static NJS_MKEY_A dat_0cb9dfb0[2] = {
#include "ev004b/bird_textures/dat_0cb9dfb0.inc"
};
static NJS_MKEY_F dat_0cb9dfd0[2] = {
#include "ev004b/bird_textures/dat_0cb9dfd0.inc"
};
static NJS_MKEY_F dat_0cb9dff0[2] = {
#include "ev004b/bird_textures/dat_0cb9dff0.inc"
};
static NJS_MKEY_A dat_0cb9e010[2] = {
#include "ev004b/bird_textures/dat_0cb9e010.inc"
};
static NJS_MKEY_A dat_0cb9e030[2] = {
#include "ev004b/bird_textures/dat_0cb9e030.inc"
};
static NJS_MKEY_F dat_0cb9e050[2] = {
#include "ev004b/bird_textures/dat_0cb9e050.inc"
};
static NJS_MKEY_F dat_0cb9e070[2] = {
#include "ev004b/bird_textures/dat_0cb9e070.inc"
};
static NJS_MKEY_A dat_0cb9e090[2] = {
#include "ev004b/bird_textures/dat_0cb9e090.inc"
};
static NJS_MKEY_F dat_0cb9e0b0[2] = {
#include "ev004b/bird_textures/dat_0cb9e0b0.inc"
};
static NJS_MKEY_A dat_0cb9e0d0[2] = {
#include "ev004b/bird_textures/dat_0cb9e0d0.inc"
};
static NJS_MKEY_F dat_0cb9e0f0[2] = {
#include "ev004b/bird_textures/dat_0cb9e0f0.inc"
};
static NJS_MKEY_A dat_0cb9e110[2] = {
#include "ev004b/bird_textures/dat_0cb9e110.inc"
};
static NJS_MKEY_F dat_0cb9e130[2] = {
#include "ev004b/bird_textures/dat_0cb9e130.inc"
};
static NJS_MKEY_A dat_0cb9e150[2] = {
#include "ev004b/bird_textures/dat_0cb9e150.inc"
};
static NJS_MKEY_A dat_0cb9e170[2] = {
#include "ev004b/bird_textures/dat_0cb9e170.inc"
};
static NJS_MKEY_F dat_0cb9e190[2] = {
#include "ev004b/bird_textures/dat_0cb9e190.inc"
};
static NJS_MKEY_F dat_0cb9e1b0[2] = {
#include "ev004b/bird_textures/dat_0cb9e1b0.inc"
};
static NJS_MKEY_A dat_0cb9e1d0[2] = {
#include "ev004b/bird_textures/dat_0cb9e1d0.inc"
};
static NJS_MKEY_F dat_0cb9e1f0[2] = {
#include "ev004b/bird_textures/dat_0cb9e1f0.inc"
};
static NJS_MKEY_A dat_0cb9e210[2] = {
#include "ev004b/bird_textures/dat_0cb9e210.inc"
};
static NJS_MKEY_F dat_0cb9e230[2] = {
#include "ev004b/bird_textures/dat_0cb9e230.inc"
};
static NJS_MKEY_A dat_0cb9e250[2] = {
#include "ev004b/bird_textures/dat_0cb9e250.inc"
};
static NJS_MKEY_F dat_0cb9e270[2] = {
#include "ev004b/bird_textures/dat_0cb9e270.inc"
};
static NJS_MKEY_A dat_0cb9e290[2] = {
#include "ev004b/bird_textures/dat_0cb9e290.inc"
};
static NJS_MKEY_F dat_0cb9e2b0[2] = {
#include "ev004b/bird_textures/dat_0cb9e2b0.inc"
};
static NJS_MKEY_A dat_0cb9e2d0[2] = {
#include "ev004b/bird_textures/dat_0cb9e2d0.inc"
};
static NJS_MKEY_A dat_0cb9e2f0[2] = {
#include "ev004b/bird_textures/dat_0cb9e2f0.inc"
};
static NJS_MKEY_F dat_0cb9e310[2] = {
#include "ev004b/bird_textures/dat_0cb9e310.inc"
};
static NJS_MKEY_F dat_0cb9e330[2] = {
#include "ev004b/bird_textures/dat_0cb9e330.inc"
};
static NJS_MKEY_A dat_0cb9e350[2] = {
#include "ev004b/bird_textures/dat_0cb9e350.inc"
};
static NJS_MKEY_F dat_0cb9e370[2] = {
#include "ev004b/bird_textures/dat_0cb9e370.inc"
};
static NJS_MKEY_A dat_0cb9e390[2] = {
#include "ev004b/bird_textures/dat_0cb9e390.inc"
};
static NJS_MKEY_F dat_0cb9e3b0[2] = {
#include "ev004b/bird_textures/dat_0cb9e3b0.inc"
};
static NJS_MKEY_A dat_0cb9e3d0[2] = {
#include "ev004b/bird_textures/dat_0cb9e3d0.inc"
};
static NJS_MKEY_F dat_0cb9e3f0[2] = {
#include "ev004b/bird_textures/dat_0cb9e3f0.inc"
};
static NJS_MKEY_A dat_0cb9e410[2] = {
#include "ev004b/bird_textures/dat_0cb9e410.inc"
};
static NJS_MKEY_F dat_0cb9e430[2] = {
#include "ev004b/bird_textures/dat_0cb9e430.inc"
};
static NJS_MKEY_A dat_0cb9e450[2] = {
#include "ev004b/bird_textures/dat_0cb9e450.inc"
};
static NJS_MKEY_F dat_0cb9e470[2] = {
#include "ev004b/bird_textures/dat_0cb9e470.inc"
};
static NJS_MKEY_A dat_0cb9e490[2] = {
#include "ev004b/bird_textures/dat_0cb9e490.inc"
};
static NJS_MKEY_F dat_0cb9e4b0[2] = {
#include "ev004b/bird_textures/dat_0cb9e4b0.inc"
};
static NJS_MKEY_A dat_0cb9e4d0[2] = {
#include "ev004b/bird_textures/dat_0cb9e4d0.inc"
};
static NJS_MKEY_F dat_0cb9e4f0[2] = {
#include "ev004b/bird_textures/dat_0cb9e4f0.inc"
};
static NJS_MKEY_F dat_0cb9e510[2] = {
#include "ev004b/bird_textures/dat_0cb9e510.inc"
};
static NJS_MKEY_F dat_0cb9e530[2] = {
#include "ev004b/bird_textures/dat_0cb9e530.inc"
};
static NJS_MKEY_A dat_0cb9e550[2] = {
#include "ev004b/bird_textures/dat_0cb9e550.inc"
};
static NJS_MKEY_F dat_0cb9e570[2] = {
#include "ev004b/bird_textures/dat_0cb9e570.inc"
};
static NJS_MKEY_A dat_0cb9e590[2] = {
#include "ev004b/bird_textures/dat_0cb9e590.inc"
};
static NJS_MKEY_F dat_0cb9e5b0[2] = {
#include "ev004b/bird_textures/dat_0cb9e5b0.inc"
};
static NJS_MDATA2 dat_0cb9e5d0[57] = {
#include "ev004b/bird_textures/dat_0cb9e5d0.inc"
};
static NJS_MOTION dat_0cb9e960 = {
#include "ev004b/bird_textures/dat_0cb9e960.inc"
};
/* no word in EV004B or 1ST_READ.BIN points at this action */
static NJS_ACTION dat_0cb9e96c = {
#include "ev004b/bird_textures/dat_0cb9e96c.inc"
};
static NJS_MKEY_F dat_0cb9e974[2] = {
#include "ev004b/bird_textures/dat_0cb9e974.inc"
};
static NJS_MKEY_F dat_0cb9e994[2] = {
#include "ev004b/bird_textures/dat_0cb9e994.inc"
};
static NJS_MKEY_A dat_0cb9e9b4[2] = {
#include "ev004b/bird_textures/dat_0cb9e9b4.inc"
};
static NJS_MKEY_F dat_0cb9e9d4[2] = {
#include "ev004b/bird_textures/dat_0cb9e9d4.inc"
};
static NJS_MKEY_F dat_0cb9e9f4[2] = {
#include "ev004b/bird_textures/dat_0cb9e9f4.inc"
};
static NJS_MKEY_A dat_0cb9ea14[2] = {
#include "ev004b/bird_textures/dat_0cb9ea14.inc"
};
static NJS_MKEY_A dat_0cb9ea34[2] = {
#include "ev004b/bird_textures/dat_0cb9ea34.inc"
};
static NJS_MKEY_F dat_0cb9ea54[2] = {
#include "ev004b/bird_textures/dat_0cb9ea54.inc"
};
static NJS_MKEY_F dat_0cb9ea74[2] = {
#include "ev004b/bird_textures/dat_0cb9ea74.inc"
};
static NJS_MKEY_A dat_0cb9ea94[2] = {
#include "ev004b/bird_textures/dat_0cb9ea94.inc"
};
static NJS_MKEY_A dat_0cb9eab4[2] = {
#include "ev004b/bird_textures/dat_0cb9eab4.inc"
};
static NJS_MKEY_F dat_0cb9ead4[2] = {
#include "ev004b/bird_textures/dat_0cb9ead4.inc"
};
static NJS_MKEY_F dat_0cb9eaf4[2] = {
#include "ev004b/bird_textures/dat_0cb9eaf4.inc"
};
static NJS_MKEY_A dat_0cb9eb14[2] = {
#include "ev004b/bird_textures/dat_0cb9eb14.inc"
};
static NJS_MKEY_A dat_0cb9eb34[2] = {
#include "ev004b/bird_textures/dat_0cb9eb34.inc"
};
static NJS_MKEY_F dat_0cb9eb54[2] = {
#include "ev004b/bird_textures/dat_0cb9eb54.inc"
};
static NJS_MKEY_F dat_0cb9eb74[2] = {
#include "ev004b/bird_textures/dat_0cb9eb74.inc"
};
static NJS_MKEY_A dat_0cb9eb94[2] = {
#include "ev004b/bird_textures/dat_0cb9eb94.inc"
};
static NJS_MKEY_F dat_0cb9ebb4[2] = {
#include "ev004b/bird_textures/dat_0cb9ebb4.inc"
};
static NJS_MKEY_A dat_0cb9ebd4[2] = {
#include "ev004b/bird_textures/dat_0cb9ebd4.inc"
};
static NJS_MKEY_F dat_0cb9ebf4[2] = {
#include "ev004b/bird_textures/dat_0cb9ebf4.inc"
};
static NJS_MKEY_A dat_0cb9ec14[2] = {
#include "ev004b/bird_textures/dat_0cb9ec14.inc"
};
static NJS_MKEY_F dat_0cb9ec34[2] = {
#include "ev004b/bird_textures/dat_0cb9ec34.inc"
};
static NJS_MKEY_A dat_0cb9ec54[2] = {
#include "ev004b/bird_textures/dat_0cb9ec54.inc"
};
static NJS_MKEY_A dat_0cb9ec74[2] = {
#include "ev004b/bird_textures/dat_0cb9ec74.inc"
};
static NJS_MKEY_F dat_0cb9ec94[2] = {
#include "ev004b/bird_textures/dat_0cb9ec94.inc"
};
static NJS_MKEY_F dat_0cb9ecb4[2] = {
#include "ev004b/bird_textures/dat_0cb9ecb4.inc"
};
static NJS_MKEY_A dat_0cb9ecd4[2] = {
#include "ev004b/bird_textures/dat_0cb9ecd4.inc"
};
static NJS_MKEY_F dat_0cb9ecf4[2] = {
#include "ev004b/bird_textures/dat_0cb9ecf4.inc"
};
static NJS_MKEY_A dat_0cb9ed14[2] = {
#include "ev004b/bird_textures/dat_0cb9ed14.inc"
};
static NJS_MKEY_F dat_0cb9ed34[2] = {
#include "ev004b/bird_textures/dat_0cb9ed34.inc"
};
static NJS_MKEY_A dat_0cb9ed54[2] = {
#include "ev004b/bird_textures/dat_0cb9ed54.inc"
};
static NJS_MKEY_F dat_0cb9ed74[2] = {
#include "ev004b/bird_textures/dat_0cb9ed74.inc"
};
static NJS_MKEY_A dat_0cb9ed94[2] = {
#include "ev004b/bird_textures/dat_0cb9ed94.inc"
};
static NJS_MKEY_F dat_0cb9edb4[2] = {
#include "ev004b/bird_textures/dat_0cb9edb4.inc"
};
static NJS_MKEY_A dat_0cb9edd4[2] = {
#include "ev004b/bird_textures/dat_0cb9edd4.inc"
};
static NJS_MKEY_A dat_0cb9edf4[2] = {
#include "ev004b/bird_textures/dat_0cb9edf4.inc"
};
static NJS_MKEY_F dat_0cb9ee14[2] = {
#include "ev004b/bird_textures/dat_0cb9ee14.inc"
};
static NJS_MKEY_F dat_0cb9ee34[2] = {
#include "ev004b/bird_textures/dat_0cb9ee34.inc"
};
static NJS_MKEY_A dat_0cb9ee54[2] = {
#include "ev004b/bird_textures/dat_0cb9ee54.inc"
};
static NJS_MKEY_F dat_0cb9ee74[2] = {
#include "ev004b/bird_textures/dat_0cb9ee74.inc"
};
static NJS_MKEY_A dat_0cb9ee94[2] = {
#include "ev004b/bird_textures/dat_0cb9ee94.inc"
};
static NJS_MKEY_F dat_0cb9eeb4[2] = {
#include "ev004b/bird_textures/dat_0cb9eeb4.inc"
};
static NJS_MKEY_A dat_0cb9eed4[2] = {
#include "ev004b/bird_textures/dat_0cb9eed4.inc"
};
static NJS_MKEY_F dat_0cb9eef4[2] = {
#include "ev004b/bird_textures/dat_0cb9eef4.inc"
};
static NJS_MKEY_A dat_0cb9ef14[2] = {
#include "ev004b/bird_textures/dat_0cb9ef14.inc"
};
static NJS_MKEY_F dat_0cb9ef34[2] = {
#include "ev004b/bird_textures/dat_0cb9ef34.inc"
};
static NJS_MKEY_A dat_0cb9ef54[2] = {
#include "ev004b/bird_textures/dat_0cb9ef54.inc"
};
static NJS_MKEY_F dat_0cb9ef74[2] = {
#include "ev004b/bird_textures/dat_0cb9ef74.inc"
};
static NJS_MKEY_A dat_0cb9ef94[2] = {
#include "ev004b/bird_textures/dat_0cb9ef94.inc"
};
static NJS_MKEY_F dat_0cb9efb4[2] = {
#include "ev004b/bird_textures/dat_0cb9efb4.inc"
};
static NJS_MKEY_A dat_0cb9efd4[2] = {
#include "ev004b/bird_textures/dat_0cb9efd4.inc"
};
static NJS_MKEY_F dat_0cb9eff4[2] = {
#include "ev004b/bird_textures/dat_0cb9eff4.inc"
};
static NJS_MKEY_F dat_0cb9f014[2] = {
#include "ev004b/bird_textures/dat_0cb9f014.inc"
};
static NJS_MKEY_F dat_0cb9f034[2] = {
#include "ev004b/bird_textures/dat_0cb9f034.inc"
};
static NJS_MKEY_A dat_0cb9f054[2] = {
#include "ev004b/bird_textures/dat_0cb9f054.inc"
};
static NJS_MKEY_F dat_0cb9f074[2] = {
#include "ev004b/bird_textures/dat_0cb9f074.inc"
};
static NJS_MKEY_A dat_0cb9f094[2] = {
#include "ev004b/bird_textures/dat_0cb9f094.inc"
};
static NJS_MKEY_F dat_0cb9f0b4[2] = {
#include "ev004b/bird_textures/dat_0cb9f0b4.inc"
};
static NJS_MDATA2 ev004b_bird_motion_0_mdata[57] = {
#include "ev004b/bird_textures/ev004b_bird_motion_0_mdata.inc"
};
static NJS_MOTION ev004b_bird_motion_0 = {
#include "ev004b/bird_textures/ev004b_bird_motion_0.inc"
};
NJS_ACTION ev004b_bird_action_0 = {
#include "ev004b/bird_textures/ev004b_bird_action_0.inc"
};
static NJS_MKEY_F dat_0cb9f478[2] = {
#include "ev004b/bird_textures/dat_0cb9f478.inc"
};
static NJS_MKEY_F dat_0cb9f498[30] = {
#include "ev004b/bird_textures/dat_0cb9f498.inc"
};
static NJS_MKEY_F dat_0cb9f678[2] = {
#include "ev004b/bird_textures/dat_0cb9f678.inc"
};
static NJS_MKEY_A dat_0cb9f698[2] = {
#include "ev004b/bird_textures/dat_0cb9f698.inc"
};
static NJS_MKEY_F dat_0cb9f6b8[2] = {
#include "ev004b/bird_textures/dat_0cb9f6b8.inc"
};
static NJS_MKEY_F dat_0cb9f6d8[2] = {
#include "ev004b/bird_textures/dat_0cb9f6d8.inc"
};
static NJS_MKEY_F dat_0cb9f6f8[2] = {
#include "ev004b/bird_textures/dat_0cb9f6f8.inc"
};
static NJS_MKEY_F dat_0cb9f718[2] = {
#include "ev004b/bird_textures/dat_0cb9f718.inc"
};
static NJS_MKEY_F dat_0cb9f738[2] = {
#include "ev004b/bird_textures/dat_0cb9f738.inc"
};
static NJS_MKEY_A dat_0cb9f758[2] = {
#include "ev004b/bird_textures/dat_0cb9f758.inc"
};
static NJS_MKEY_F dat_0cb9f778[2] = {
#include "ev004b/bird_textures/dat_0cb9f778.inc"
};
static NJS_MKEY_A dat_0cb9f798[30] = {
#include "ev004b/bird_textures/dat_0cb9f798.inc"
};
static NJS_MKEY_F dat_0cb9f978[2] = {
#include "ev004b/bird_textures/dat_0cb9f978.inc"
};
static NJS_MKEY_F dat_0cb9f998[2] = {
#include "ev004b/bird_textures/dat_0cb9f998.inc"
};
static NJS_MKEY_F dat_0cb9f9b8[2] = {
#include "ev004b/bird_textures/dat_0cb9f9b8.inc"
};
static NJS_MKEY_F dat_0cb9f9d8[2] = {
#include "ev004b/bird_textures/dat_0cb9f9d8.inc"
};
static NJS_MKEY_F dat_0cb9f9f8[2] = {
#include "ev004b/bird_textures/dat_0cb9f9f8.inc"
};
static NJS_MKEY_A dat_0cb9fa18[2] = {
#include "ev004b/bird_textures/dat_0cb9fa18.inc"
};
static NJS_MKEY_F dat_0cb9fa38[2] = {
#include "ev004b/bird_textures/dat_0cb9fa38.inc"
};
static NJS_MKEY_A dat_0cb9fa58[30] = {
#include "ev004b/bird_textures/dat_0cb9fa58.inc"
};
static NJS_MKEY_F dat_0cb9fc38[2] = {
#include "ev004b/bird_textures/dat_0cb9fc38.inc"
};
static NJS_MKEY_F dat_0cb9fc58[2] = {
#include "ev004b/bird_textures/dat_0cb9fc58.inc"
};
static NJS_MKEY_F dat_0cb9fc78[2] = {
#include "ev004b/bird_textures/dat_0cb9fc78.inc"
};
static NJS_MKEY_F dat_0cb9fc98[2] = {
#include "ev004b/bird_textures/dat_0cb9fc98.inc"
};
static NJS_MKEY_F dat_0cb9fcb8[2] = {
#include "ev004b/bird_textures/dat_0cb9fcb8.inc"
};
static NJS_MKEY_A dat_0cb9fcd8[2] = {
#include "ev004b/bird_textures/dat_0cb9fcd8.inc"
};
static NJS_MKEY_F dat_0cb9fcf8[2] = {
#include "ev004b/bird_textures/dat_0cb9fcf8.inc"
};
static NJS_MKEY_A dat_0cb9fd18[2] = {
#include "ev004b/bird_textures/dat_0cb9fd18.inc"
};
static NJS_MKEY_F dat_0cb9fd38[2] = {
#include "ev004b/bird_textures/dat_0cb9fd38.inc"
};
static NJS_MKEY_F dat_0cb9fd58[2] = {
#include "ev004b/bird_textures/dat_0cb9fd58.inc"
};
static NJS_MKEY_F dat_0cb9fd78[2] = {
#include "ev004b/bird_textures/dat_0cb9fd78.inc"
};
static NJS_MKEY_F dat_0cb9fd98[2] = {
#include "ev004b/bird_textures/dat_0cb9fd98.inc"
};
static NJS_MKEY_F dat_0cb9fdb8[2] = {
#include "ev004b/bird_textures/dat_0cb9fdb8.inc"
};
static NJS_MKEY_F dat_0cb9fdd8[2] = {
#include "ev004b/bird_textures/dat_0cb9fdd8.inc"
};
static NJS_MKEY_F dat_0cb9fdf8[2] = {
#include "ev004b/bird_textures/dat_0cb9fdf8.inc"
};
static NJS_MKEY_F dat_0cb9fe18[2] = {
#include "ev004b/bird_textures/dat_0cb9fe18.inc"
};
static NJS_MKEY_A dat_0cb9fe38[2] = {
#include "ev004b/bird_textures/dat_0cb9fe38.inc"
};
static NJS_MKEY_F dat_0cb9fe58[2] = {
#include "ev004b/bird_textures/dat_0cb9fe58.inc"
};
static NJS_MKEY_F dat_0cb9fe78[2] = {
#include "ev004b/bird_textures/dat_0cb9fe78.inc"
};
static NJS_MKEY_A dat_0cb9fe98[2] = {
#include "ev004b/bird_textures/dat_0cb9fe98.inc"
};
static NJS_MKEY_F dat_0cb9feb8[2] = {
#include "ev004b/bird_textures/dat_0cb9feb8.inc"
};
static NJS_MKEY_A dat_0cb9fed8[2] = {
#include "ev004b/bird_textures/dat_0cb9fed8.inc"
};
static NJS_MKEY_F dat_0cb9fef8[2] = {
#include "ev004b/bird_textures/dat_0cb9fef8.inc"
};
static NJS_MKEY_A dat_0cb9ff18[2] = {
#include "ev004b/bird_textures/dat_0cb9ff18.inc"
};
static NJS_MKEY_F dat_0cb9ff38[2] = {
#include "ev004b/bird_textures/dat_0cb9ff38.inc"
};
static NJS_MKEY_A dat_0cb9ff58[2] = {
#include "ev004b/bird_textures/dat_0cb9ff58.inc"
};
static NJS_MKEY_F dat_0cb9ff78[2] = {
#include "ev004b/bird_textures/dat_0cb9ff78.inc"
};
static NJS_MKEY_F dat_0cb9ff98[2] = {
#include "ev004b/bird_textures/dat_0cb9ff98.inc"
};
static NJS_MKEY_F dat_0cb9ffb8[2] = {
#include "ev004b/bird_textures/dat_0cb9ffb8.inc"
};
static NJS_MKEY_F dat_0cb9ffd8[2] = {
#include "ev004b/bird_textures/dat_0cb9ffd8.inc"
};
static NJS_MKEY_F dat_0cb9fff8[2] = {
#include "ev004b/bird_textures/dat_0cb9fff8.inc"
};
static NJS_MKEY_A dat_0cba0018[2] = {
#include "ev004b/bird_textures/dat_0cba0018.inc"
};
static NJS_MKEY_F dat_0cba0038[2] = {
#include "ev004b/bird_textures/dat_0cba0038.inc"
};
static NJS_MKEY_F dat_0cba0058[2] = {
#include "ev004b/bird_textures/dat_0cba0058.inc"
};
static NJS_MKEY_A dat_0cba0078[2] = {
#include "ev004b/bird_textures/dat_0cba0078.inc"
};
static NJS_MKEY_F dat_0cba0098[2] = {
#include "ev004b/bird_textures/dat_0cba0098.inc"
};
static NJS_MKEY_F dat_0cba00b8[2] = {
#include "ev004b/bird_textures/dat_0cba00b8.inc"
};
static NJS_MKEY_A dat_0cba00d8[2] = {
#include "ev004b/bird_textures/dat_0cba00d8.inc"
};
static NJS_MKEY_F dat_0cba00f8[2] = {
#include "ev004b/bird_textures/dat_0cba00f8.inc"
};
static NJS_MKEY_F dat_0cba0118[2] = {
#include "ev004b/bird_textures/dat_0cba0118.inc"
};
static NJS_MKEY_A dat_0cba0138[2] = {
#include "ev004b/bird_textures/dat_0cba0138.inc"
};
static NJS_MKEY_F dat_0cba0158[2] = {
#include "ev004b/bird_textures/dat_0cba0158.inc"
};
static NJS_MKEY_F dat_0cba0178[2] = {
#include "ev004b/bird_textures/dat_0cba0178.inc"
};
static NJS_MKEY_A dat_0cba0198[2] = {
#include "ev004b/bird_textures/dat_0cba0198.inc"
};
static NJS_MKEY_F dat_0cba01b8[2] = {
#include "ev004b/bird_textures/dat_0cba01b8.inc"
};
static NJS_MKEY_A dat_0cba01d8[30] = {
#include "ev004b/bird_textures/dat_0cba01d8.inc"
};
static NJS_MKEY_F dat_0cba03b8[2] = {
#include "ev004b/bird_textures/dat_0cba03b8.inc"
};
static NJS_MKEY_F dat_0cba03d8[2] = {
#include "ev004b/bird_textures/dat_0cba03d8.inc"
};
static NJS_MKEY_F dat_0cba03f8[2] = {
#include "ev004b/bird_textures/dat_0cba03f8.inc"
};
static NJS_MKEY_F dat_0cba0418[2] = {
#include "ev004b/bird_textures/dat_0cba0418.inc"
};
static NJS_MKEY_F dat_0cba0438[30] = {
#include "ev004b/bird_textures/dat_0cba0438.inc"
};
static NJS_MKEY_A dat_0cba0618[2] = {
#include "ev004b/bird_textures/dat_0cba0618.inc"
};
static NJS_MKEY_F dat_0cba0638[2] = {
#include "ev004b/bird_textures/dat_0cba0638.inc"
};
static NJS_MKEY_F dat_0cba0658[30] = {
#include "ev004b/bird_textures/dat_0cba0658.inc"
};
static NJS_MKEY_A dat_0cba0838[2] = {
#include "ev004b/bird_textures/dat_0cba0838.inc"
};
static NJS_MKEY_F dat_0cba0858[2] = {
#include "ev004b/bird_textures/dat_0cba0858.inc"
};
static NJS_MKEY_F dat_0cba0878[2] = {
#include "ev004b/bird_textures/dat_0cba0878.inc"
};
static NJS_MKEY_A dat_0cba0898[2] = {
#include "ev004b/bird_textures/dat_0cba0898.inc"
};
static NJS_MKEY_F dat_0cba08b8[2] = {
#include "ev004b/bird_textures/dat_0cba08b8.inc"
};
static NJS_MKEY_F dat_0cba08d8[2] = {
#include "ev004b/bird_textures/dat_0cba08d8.inc"
};
static NJS_MKEY_A dat_0cba08f8[2] = {
#include "ev004b/bird_textures/dat_0cba08f8.inc"
};
static NJS_MKEY_F dat_0cba0918[2] = {
#include "ev004b/bird_textures/dat_0cba0918.inc"
};
static NJS_MKEY_F dat_0cba0938[30] = {
#include "ev004b/bird_textures/dat_0cba0938.inc"
};
static NJS_MKEY_A dat_0cba0b18[2] = {
#include "ev004b/bird_textures/dat_0cba0b18.inc"
};
static NJS_MKEY_F dat_0cba0b38[2] = {
#include "ev004b/bird_textures/dat_0cba0b38.inc"
};
static NJS_MKEY_F dat_0cba0b58[2] = {
#include "ev004b/bird_textures/dat_0cba0b58.inc"
};
static NJS_MKEY_A dat_0cba0b78[2] = {
#include "ev004b/bird_textures/dat_0cba0b78.inc"
};
static NJS_MKEY_F dat_0cba0b98[2] = {
#include "ev004b/bird_textures/dat_0cba0b98.inc"
};
static NJS_MKEY_A dat_0cba0bb8[30] = {
#include "ev004b/bird_textures/dat_0cba0bb8.inc"
};
static NJS_MKEY_F dat_0cba0d98[2] = {
#include "ev004b/bird_textures/dat_0cba0d98.inc"
};
static NJS_MKEY_F dat_0cba0db8[2] = {
#include "ev004b/bird_textures/dat_0cba0db8.inc"
};
static NJS_MKEY_F dat_0cba0dd8[2] = {
#include "ev004b/bird_textures/dat_0cba0dd8.inc"
};
static NJS_MKEY_A dat_0cba0df8[2] = {
#include "ev004b/bird_textures/dat_0cba0df8.inc"
};
static NJS_MKEY_F dat_0cba0e18[2] = {
#include "ev004b/bird_textures/dat_0cba0e18.inc"
};
static NJS_MKEY_A dat_0cba0e38[30] = {
#include "ev004b/bird_textures/dat_0cba0e38.inc"
};
static NJS_MKEY_F dat_0cba1018[2] = {
#include "ev004b/bird_textures/dat_0cba1018.inc"
};
static NJS_MKEY_F dat_0cba1038[2] = {
#include "ev004b/bird_textures/dat_0cba1038.inc"
};
static NJS_MKEY_F dat_0cba1058[2] = {
#include "ev004b/bird_textures/dat_0cba1058.inc"
};
static NJS_MKEY_F dat_0cba1078[2] = {
#include "ev004b/bird_textures/dat_0cba1078.inc"
};
static NJS_MKEY_F dat_0cba1098[2] = {
#include "ev004b/bird_textures/dat_0cba1098.inc"
};
static NJS_MKEY_F dat_0cba10b8[2] = {
#include "ev004b/bird_textures/dat_0cba10b8.inc"
};
static NJS_MKEY_F dat_0cba10d8[2] = {
#include "ev004b/bird_textures/dat_0cba10d8.inc"
};
static NJS_MKEY_A dat_0cba10f8[2] = {
#include "ev004b/bird_textures/dat_0cba10f8.inc"
};
static NJS_MKEY_F dat_0cba1118[2] = {
#include "ev004b/bird_textures/dat_0cba1118.inc"
};
static NJS_MKEY_A dat_0cba1138[30] = {
#include "ev004b/bird_textures/dat_0cba1138.inc"
};
static NJS_MKEY_F dat_0cba1318[2] = {
#include "ev004b/bird_textures/dat_0cba1318.inc"
};
static NJS_MKEY_F dat_0cba1338[2] = {
#include "ev004b/bird_textures/dat_0cba1338.inc"
};
static NJS_MKEY_F dat_0cba1358[2] = {
#include "ev004b/bird_textures/dat_0cba1358.inc"
};
static NJS_MKEY_A dat_0cba1378[2] = {
#include "ev004b/bird_textures/dat_0cba1378.inc"
};
static NJS_MKEY_F dat_0cba1398[2] = {
#include "ev004b/bird_textures/dat_0cba1398.inc"
};
static NJS_MKEY_A dat_0cba13b8[30] = {
#include "ev004b/bird_textures/dat_0cba13b8.inc"
};
static NJS_MKEY_F dat_0cba1598[2] = {
#include "ev004b/bird_textures/dat_0cba1598.inc"
};
static NJS_MKEY_F dat_0cba15b8[2] = {
#include "ev004b/bird_textures/dat_0cba15b8.inc"
};
static NJS_MKEY_F dat_0cba15d8[2] = {
#include "ev004b/bird_textures/dat_0cba15d8.inc"
};
static NJS_MKEY_F dat_0cba15f8[2] = {
#include "ev004b/bird_textures/dat_0cba15f8.inc"
};
static NJS_MKEY_F dat_0cba1618[2] = {
#include "ev004b/bird_textures/dat_0cba1618.inc"
};
static NJS_MDATA3 ev004b_bird_motion_1_mdata[57] = {
#include "ev004b/bird_textures/ev004b_bird_motion_1_mdata.inc"
};
static NJS_MOTION ev004b_bird_motion_1 = {
#include "ev004b/bird_textures/ev004b_bird_motion_1.inc"
};
NJS_ACTION ev004b_bird_action_1 = {
#include "ev004b/bird_textures/ev004b_bird_action_1.inc"
};
static NJS_MKEY_F dat_0cba1ba4[2] = {
#include "ev004b/bird_textures/dat_0cba1ba4.inc"
};
static NJS_MKEY_F dat_0cba1bc4[32] = {
#include "ev004b/bird_textures/dat_0cba1bc4.inc"
};
static NJS_MKEY_A dat_0cba1dc4[32] = {
#include "ev004b/bird_textures/dat_0cba1dc4.inc"
};
static NJS_MKEY_F dat_0cba1fc4[2] = {
#include "ev004b/bird_textures/dat_0cba1fc4.inc"
};
static NJS_MKEY_A dat_0cba1fe4[32] = {
#include "ev004b/bird_textures/dat_0cba1fe4.inc"
};
static NJS_MKEY_F dat_0cba21e4[2] = {
#include "ev004b/bird_textures/dat_0cba21e4.inc"
};
static NJS_MKEY_F dat_0cba2204[2] = {
#include "ev004b/bird_textures/dat_0cba2204.inc"
};
static NJS_MKEY_F dat_0cba2224[2] = {
#include "ev004b/bird_textures/dat_0cba2224.inc"
};
static NJS_MKEY_F dat_0cba2244[2] = {
#include "ev004b/bird_textures/dat_0cba2244.inc"
};
static NJS_MKEY_A dat_0cba2264[32] = {
#include "ev004b/bird_textures/dat_0cba2264.inc"
};
static NJS_MKEY_F dat_0cba2464[2] = {
#include "ev004b/bird_textures/dat_0cba2464.inc"
};
static NJS_MKEY_F dat_0cba2484[2] = {
#include "ev004b/bird_textures/dat_0cba2484.inc"
};
static NJS_MKEY_F dat_0cba24a4[2] = {
#include "ev004b/bird_textures/dat_0cba24a4.inc"
};
static NJS_MKEY_F dat_0cba24c4[2] = {
#include "ev004b/bird_textures/dat_0cba24c4.inc"
};
static NJS_MKEY_A dat_0cba24e4[32] = {
#include "ev004b/bird_textures/dat_0cba24e4.inc"
};
static NJS_MKEY_F dat_0cba26e4[2] = {
#include "ev004b/bird_textures/dat_0cba26e4.inc"
};
static NJS_MKEY_F dat_0cba2704[2] = {
#include "ev004b/bird_textures/dat_0cba2704.inc"
};
static NJS_MKEY_F dat_0cba2724[2] = {
#include "ev004b/bird_textures/dat_0cba2724.inc"
};
static NJS_MKEY_F dat_0cba2744[2] = {
#include "ev004b/bird_textures/dat_0cba2744.inc"
};
static NJS_MKEY_A dat_0cba2764[32] = {
#include "ev004b/bird_textures/dat_0cba2764.inc"
};
static NJS_MKEY_F dat_0cba2964[2] = {
#include "ev004b/bird_textures/dat_0cba2964.inc"
};
static NJS_MKEY_F dat_0cba2984[2] = {
#include "ev004b/bird_textures/dat_0cba2984.inc"
};
static NJS_MKEY_F dat_0cba29a4[2] = {
#include "ev004b/bird_textures/dat_0cba29a4.inc"
};
static NJS_MKEY_F dat_0cba29c4[2] = {
#include "ev004b/bird_textures/dat_0cba29c4.inc"
};
static NJS_MKEY_F dat_0cba29e4[2] = {
#include "ev004b/bird_textures/dat_0cba29e4.inc"
};
static NJS_MKEY_F dat_0cba2a04[2] = {
#include "ev004b/bird_textures/dat_0cba2a04.inc"
};
static NJS_MKEY_F dat_0cba2a24[2] = {
#include "ev004b/bird_textures/dat_0cba2a24.inc"
};
static NJS_MKEY_F dat_0cba2a44[2] = {
#include "ev004b/bird_textures/dat_0cba2a44.inc"
};
static NJS_MKEY_F dat_0cba2a64[2] = {
#include "ev004b/bird_textures/dat_0cba2a64.inc"
};
static NJS_MKEY_F dat_0cba2a84[2] = {
#include "ev004b/bird_textures/dat_0cba2a84.inc"
};
static NJS_MKEY_F dat_0cba2aa4[2] = {
#include "ev004b/bird_textures/dat_0cba2aa4.inc"
};
static NJS_MKEY_A dat_0cba2ac4[32] = {
#include "ev004b/bird_textures/dat_0cba2ac4.inc"
};
static NJS_MKEY_F dat_0cba2cc4[2] = {
#include "ev004b/bird_textures/dat_0cba2cc4.inc"
};
static NJS_MKEY_F dat_0cba2ce4[2] = {
#include "ev004b/bird_textures/dat_0cba2ce4.inc"
};
static NJS_MKEY_F dat_0cba2d04[2] = {
#include "ev004b/bird_textures/dat_0cba2d04.inc"
};
static NJS_MKEY_F dat_0cba2d24[2] = {
#include "ev004b/bird_textures/dat_0cba2d24.inc"
};
static NJS_MKEY_F dat_0cba2d44[2] = {
#include "ev004b/bird_textures/dat_0cba2d44.inc"
};
static NJS_MKEY_F dat_0cba2d64[2] = {
#include "ev004b/bird_textures/dat_0cba2d64.inc"
};
static NJS_MKEY_F dat_0cba2d84[2] = {
#include "ev004b/bird_textures/dat_0cba2d84.inc"
};
static NJS_MKEY_F dat_0cba2da4[2] = {
#include "ev004b/bird_textures/dat_0cba2da4.inc"
};
static NJS_MKEY_A dat_0cba2dc4[32] = {
#include "ev004b/bird_textures/dat_0cba2dc4.inc"
};
static NJS_MKEY_F dat_0cba2fc4[2] = {
#include "ev004b/bird_textures/dat_0cba2fc4.inc"
};
static NJS_MKEY_F dat_0cba2fe4[2] = {
#include "ev004b/bird_textures/dat_0cba2fe4.inc"
};
static NJS_MKEY_F dat_0cba3004[32] = {
#include "ev004b/bird_textures/dat_0cba3004.inc"
};
static NJS_MKEY_F dat_0cba3204[32] = {
#include "ev004b/bird_textures/dat_0cba3204.inc"
};
static NJS_MKEY_F dat_0cba3404[32] = {
#include "ev004b/bird_textures/dat_0cba3404.inc"
};
static NJS_MKEY_F dat_0cba3604[32] = {
#include "ev004b/bird_textures/dat_0cba3604.inc"
};
static NJS_MKEY_F dat_0cba3804[32] = {
#include "ev004b/bird_textures/dat_0cba3804.inc"
};
static NJS_MKEY_F dat_0cba3a04[2] = {
#include "ev004b/bird_textures/dat_0cba3a04.inc"
};
static NJS_MKEY_F dat_0cba3a24[2] = {
#include "ev004b/bird_textures/dat_0cba3a24.inc"
};
static NJS_MKEY_A dat_0cba3a44[32] = {
#include "ev004b/bird_textures/dat_0cba3a44.inc"
};
static NJS_MKEY_F dat_0cba3c44[2] = {
#include "ev004b/bird_textures/dat_0cba3c44.inc"
};
static NJS_MKEY_F dat_0cba3c64[2] = {
#include "ev004b/bird_textures/dat_0cba3c64.inc"
};
static NJS_MKEY_F dat_0cba3c84[2] = {
#include "ev004b/bird_textures/dat_0cba3c84.inc"
};
static NJS_MKEY_A dat_0cba3ca4[32] = {
#include "ev004b/bird_textures/dat_0cba3ca4.inc"
};
static NJS_MKEY_F dat_0cba3ea4[2] = {
#include "ev004b/bird_textures/dat_0cba3ea4.inc"
};
static NJS_MKEY_F dat_0cba3ec4[2] = {
#include "ev004b/bird_textures/dat_0cba3ec4.inc"
};
static NJS_MKEY_F dat_0cba3ee4[2] = {
#include "ev004b/bird_textures/dat_0cba3ee4.inc"
};
static NJS_MKEY_F dat_0cba3f04[2] = {
#include "ev004b/bird_textures/dat_0cba3f04.inc"
};
static NJS_MKEY_F dat_0cba3f24[2] = {
#include "ev004b/bird_textures/dat_0cba3f24.inc"
};
static NJS_MKEY_A dat_0cba3f44[32] = {
#include "ev004b/bird_textures/dat_0cba3f44.inc"
};
static NJS_MKEY_F dat_0cba4144[2] = {
#include "ev004b/bird_textures/dat_0cba4144.inc"
};
static NJS_MKEY_F dat_0cba4164[2] = {
#include "ev004b/bird_textures/dat_0cba4164.inc"
};
static NJS_MKEY_F dat_0cba4184[2] = {
#include "ev004b/bird_textures/dat_0cba4184.inc"
};
static NJS_MKEY_A dat_0cba41a4[32] = {
#include "ev004b/bird_textures/dat_0cba41a4.inc"
};
static NJS_MKEY_F dat_0cba43a4[2] = {
#include "ev004b/bird_textures/dat_0cba43a4.inc"
};
static NJS_MKEY_F dat_0cba43c4[2] = {
#include "ev004b/bird_textures/dat_0cba43c4.inc"
};
static NJS_MKEY_F dat_0cba43e4[2] = {
#include "ev004b/bird_textures/dat_0cba43e4.inc"
};
static NJS_MDATA2 ev004b_bird_motion_2_mdata[57] = {
#include "ev004b/bird_textures/ev004b_bird_motion_2_mdata.inc"
};
static NJS_MOTION ev004b_bird_motion_2 = {
#include "ev004b/bird_textures/ev004b_bird_motion_2.inc"
};
NJS_ACTION ev004b_bird_action_2 = {
#include "ev004b/bird_textures/ev004b_bird_action_2.inc"
};
static NJS_MKEY_A dat_0cba47a8[2] = {
#include "ev004b/bird_textures/dat_0cba47a8.inc"
};
static NJS_MKEY_A dat_0cba47c8[15] = {
#include "ev004b/bird_textures/dat_0cba47c8.inc"
};
static NJS_MKEY_A dat_0cba48b8[15] = {
#include "ev004b/bird_textures/dat_0cba48b8.inc"
};
static NJS_MKEY_A dat_0cba49a8[2] = {
#include "ev004b/bird_textures/dat_0cba49a8.inc"
};
static NJS_MKEY_A dat_0cba49c8[15] = {
#include "ev004b/bird_textures/dat_0cba49c8.inc"
};
static NJS_MKEY_A dat_0cba4ab8[15] = {
#include "ev004b/bird_textures/dat_0cba4ab8.inc"
};
static NJS_MKEY_A dat_0cba4ba8[15] = {
#include "ev004b/bird_textures/dat_0cba4ba8.inc"
};
static NJS_MKEY_A dat_0cba4c98[2] = {
#include "ev004b/bird_textures/dat_0cba4c98.inc"
};
static NJS_MKEY_A dat_0cba4cb8[2] = {
#include "ev004b/bird_textures/dat_0cba4cb8.inc"
};
static NJS_MKEY_A dat_0cba4cd8[15] = {
#include "ev004b/bird_textures/dat_0cba4cd8.inc"
};
static NJS_MKEY_A dat_0cba4dc8[15] = {
#include "ev004b/bird_textures/dat_0cba4dc8.inc"
};
static NJS_MKEY_A dat_0cba4eb8[15] = {
#include "ev004b/bird_textures/dat_0cba4eb8.inc"
};
static NJS_MKEY_A dat_0cba4fa8[2] = {
#include "ev004b/bird_textures/dat_0cba4fa8.inc"
};
static NJS_MKEY_A dat_0cba4fc8[2] = {
#include "ev004b/bird_textures/dat_0cba4fc8.inc"
};
static NJS_MKEY_A dat_0cba4fe8[2] = {
#include "ev004b/bird_textures/dat_0cba4fe8.inc"
};
static NJS_MKEY_A dat_0cba5008[2] = {
#include "ev004b/bird_textures/dat_0cba5008.inc"
};
static NJS_MKEY_A dat_0cba5028[2] = {
#include "ev004b/bird_textures/dat_0cba5028.inc"
};
static NJS_MKEY_A dat_0cba5048[2] = {
#include "ev004b/bird_textures/dat_0cba5048.inc"
};
static NJS_MKEY_A dat_0cba5068[2] = {
#include "ev004b/bird_textures/dat_0cba5068.inc"
};
static NJS_MKEY_A dat_0cba5088[2] = {
#include "ev004b/bird_textures/dat_0cba5088.inc"
};
static NJS_MKEY_A dat_0cba50a8[2] = {
#include "ev004b/bird_textures/dat_0cba50a8.inc"
};
static NJS_MKEY_A dat_0cba50c8[2] = {
#include "ev004b/bird_textures/dat_0cba50c8.inc"
};
static NJS_MKEY_A dat_0cba50e8[2] = {
#include "ev004b/bird_textures/dat_0cba50e8.inc"
};
static NJS_MKEY_A dat_0cba5108[2] = {
#include "ev004b/bird_textures/dat_0cba5108.inc"
};
static NJS_MDATA2 dat_0cba5128[24] = {
#include "ev004b/bird_textures/dat_0cba5128.inc"
};
static NJS_MOTION dat_0cba52a8 = {
#include "ev004b/bird_textures/dat_0cba52a8.inc"
};
/* an action on event_eggmobile_object. no word in EV004B or 1ST_READ.BIN
 * points at it */
static NJS_ACTION dat_0cba52b4 = {
#include "ev004b/bird_textures/dat_0cba52b4.inc"
};
static NJS_MKEY_F dat_0cba52bc[2] = {
#include "ev004b/bird_textures/dat_0cba52bc.inc"
};
static NJS_MKEY_A dat_0cba52dc[2] = {
#include "ev004b/bird_textures/dat_0cba52dc.inc"
};
static NJS_MKEY_F dat_0cba52fc[2] = {
#include "ev004b/bird_textures/dat_0cba52fc.inc"
};
static NJS_MKEY_A dat_0cba531c[60] = {
#include "ev004b/bird_textures/dat_0cba531c.inc"
};
static NJS_MKEY_F dat_0cba56dc[2] = {
#include "ev004b/bird_textures/dat_0cba56dc.inc"
};
static NJS_MKEY_A dat_0cba56fc[60] = {
#include "ev004b/bird_textures/dat_0cba56fc.inc"
};
static NJS_MKEY_F dat_0cba5abc[2] = {
#include "ev004b/bird_textures/dat_0cba5abc.inc"
};
static NJS_MKEY_A dat_0cba5adc[2] = {
#include "ev004b/bird_textures/dat_0cba5adc.inc"
};
static NJS_MKEY_F dat_0cba5afc[2] = {
#include "ev004b/bird_textures/dat_0cba5afc.inc"
};
static NJS_MKEY_A dat_0cba5b1c[60] = {
#include "ev004b/bird_textures/dat_0cba5b1c.inc"
};
static NJS_MKEY_F dat_0cba5edc[2] = {
#include "ev004b/bird_textures/dat_0cba5edc.inc"
};
static NJS_MKEY_A dat_0cba5efc[60] = {
#include "ev004b/bird_textures/dat_0cba5efc.inc"
};
static NJS_MKEY_F dat_0cba62bc[2] = {
#include "ev004b/bird_textures/dat_0cba62bc.inc"
};
static NJS_MKEY_A dat_0cba62dc[2] = {
#include "ev004b/bird_textures/dat_0cba62dc.inc"
};
static NJS_MKEY_F dat_0cba62fc[2] = {
#include "ev004b/bird_textures/dat_0cba62fc.inc"
};
static NJS_MKEY_A dat_0cba631c[2] = {
#include "ev004b/bird_textures/dat_0cba631c.inc"
};
static NJS_MKEY_F dat_0cba633c[2] = {
#include "ev004b/bird_textures/dat_0cba633c.inc"
};
static NJS_MKEY_A dat_0cba635c[2] = {
#include "ev004b/bird_textures/dat_0cba635c.inc"
};
static NJS_MKEY_F dat_0cba637c[2] = {
#include "ev004b/bird_textures/dat_0cba637c.inc"
};
static NJS_MKEY_A dat_0cba639c[60] = {
#include "ev004b/bird_textures/dat_0cba639c.inc"
};
static NJS_MKEY_F dat_0cba675c[2] = {
#include "ev004b/bird_textures/dat_0cba675c.inc"
};
static NJS_MKEY_A dat_0cba677c[60] = {
#include "ev004b/bird_textures/dat_0cba677c.inc"
};
static NJS_MKEY_F dat_0cba6b3c[2] = {
#include "ev004b/bird_textures/dat_0cba6b3c.inc"
};
static NJS_MKEY_A dat_0cba6b5c[2] = {
#include "ev004b/bird_textures/dat_0cba6b5c.inc"
};
static NJS_MKEY_F dat_0cba6b7c[2] = {
#include "ev004b/bird_textures/dat_0cba6b7c.inc"
};
static NJS_MKEY_A dat_0cba6b9c[2] = {
#include "ev004b/bird_textures/dat_0cba6b9c.inc"
};
static NJS_MKEY_F dat_0cba6bbc[2] = {
#include "ev004b/bird_textures/dat_0cba6bbc.inc"
};
static NJS_MKEY_A dat_0cba6bdc[2] = {
#include "ev004b/bird_textures/dat_0cba6bdc.inc"
};
static NJS_MKEY_F dat_0cba6bfc[2] = {
#include "ev004b/bird_textures/dat_0cba6bfc.inc"
};
static NJS_MKEY_A dat_0cba6c1c[2] = {
#include "ev004b/bird_textures/dat_0cba6c1c.inc"
};
static NJS_MKEY_F dat_0cba6c3c[2] = {
#include "ev004b/bird_textures/dat_0cba6c3c.inc"
};
static NJS_MKEY_A dat_0cba6c5c[2] = {
#include "ev004b/bird_textures/dat_0cba6c5c.inc"
};
static NJS_MKEY_F dat_0cba6c7c[2] = {
#include "ev004b/bird_textures/dat_0cba6c7c.inc"
};
static NJS_MKEY_A dat_0cba6c9c[2] = {
#include "ev004b/bird_textures/dat_0cba6c9c.inc"
};
static NJS_MKEY_F dat_0cba6cbc[2] = {
#include "ev004b/bird_textures/dat_0cba6cbc.inc"
};
static NJS_MKEY_A dat_0cba6cdc[2] = {
#include "ev004b/bird_textures/dat_0cba6cdc.inc"
};
static NJS_MKEY_F dat_0cba6cfc[2] = {
#include "ev004b/bird_textures/dat_0cba6cfc.inc"
};
static NJS_MKEY_A dat_0cba6d1c[2] = {
#include "ev004b/bird_textures/dat_0cba6d1c.inc"
};
static NJS_MKEY_F dat_0cba6d3c[2] = {
#include "ev004b/bird_textures/dat_0cba6d3c.inc"
};
static NJS_MKEY_A dat_0cba6d5c[2] = {
#include "ev004b/bird_textures/dat_0cba6d5c.inc"
};
static NJS_MKEY_F dat_0cba6d7c[2] = {
#include "ev004b/bird_textures/dat_0cba6d7c.inc"
};
static NJS_MKEY_A dat_0cba6d9c[2] = {
#include "ev004b/bird_textures/dat_0cba6d9c.inc"
};
static NJS_MKEY_F dat_0cba6dbc[2] = {
#include "ev004b/bird_textures/dat_0cba6dbc.inc"
};
static NJS_MKEY_A dat_0cba6ddc[2] = {
#include "ev004b/bird_textures/dat_0cba6ddc.inc"
};
static NJS_MKEY_F dat_0cba6dfc[2] = {
#include "ev004b/bird_textures/dat_0cba6dfc.inc"
};
static NJS_MKEY_A dat_0cba6e1c[2] = {
#include "ev004b/bird_textures/dat_0cba6e1c.inc"
};
static NJS_MKEY_F dat_0cba6e3c[2] = {
#include "ev004b/bird_textures/dat_0cba6e3c.inc"
};
static NJS_MKEY_A dat_0cba6e5c[2] = {
#include "ev004b/bird_textures/dat_0cba6e5c.inc"
};
static NJS_MDATA2 ev004b_eggmobile_motion_0_mdata[24] = {
#include "ev004b/bird_textures/ev004b_eggmobile_motion_0_mdata.inc"
};
static NJS_MOTION ev004b_eggmobile_motion_0 = {
#include "ev004b/bird_textures/ev004b_eggmobile_motion_0.inc"
};
NJS_ACTION ev004b_eggmobile_action_0 = {
#include "ev004b/bird_textures/ev004b_eggmobile_action_0.inc"
};
static NJS_MKEY_F dat_0cba7010[2] = {
#include "ev004b/bird_textures/dat_0cba7010.inc"
};
static NJS_MKEY_A dat_0cba7030[2] = {
#include "ev004b/bird_textures/dat_0cba7030.inc"
};
static NJS_MKEY_F dat_0cba7050[2] = {
#include "ev004b/bird_textures/dat_0cba7050.inc"
};
static NJS_MKEY_A dat_0cba7070[30] = {
#include "ev004b/bird_textures/dat_0cba7070.inc"
};
static NJS_MKEY_F dat_0cba7250[2] = {
#include "ev004b/bird_textures/dat_0cba7250.inc"
};
static NJS_MKEY_A dat_0cba7270[30] = {
#include "ev004b/bird_textures/dat_0cba7270.inc"
};
static NJS_MKEY_F dat_0cba7450[2] = {
#include "ev004b/bird_textures/dat_0cba7450.inc"
};
static NJS_MKEY_A dat_0cba7470[2] = {
#include "ev004b/bird_textures/dat_0cba7470.inc"
};
static NJS_MKEY_F dat_0cba7490[2] = {
#include "ev004b/bird_textures/dat_0cba7490.inc"
};
static NJS_MKEY_A dat_0cba74b0[30] = {
#include "ev004b/bird_textures/dat_0cba74b0.inc"
};
static NJS_MKEY_F dat_0cba7690[2] = {
#include "ev004b/bird_textures/dat_0cba7690.inc"
};
static NJS_MKEY_A dat_0cba76b0[30] = {
#include "ev004b/bird_textures/dat_0cba76b0.inc"
};
static NJS_MKEY_F dat_0cba7890[2] = {
#include "ev004b/bird_textures/dat_0cba7890.inc"
};
static NJS_MKEY_A dat_0cba78b0[2] = {
#include "ev004b/bird_textures/dat_0cba78b0.inc"
};
static NJS_MKEY_F dat_0cba78d0[2] = {
#include "ev004b/bird_textures/dat_0cba78d0.inc"
};
static NJS_MKEY_A dat_0cba78f0[2] = {
#include "ev004b/bird_textures/dat_0cba78f0.inc"
};
static NJS_MKEY_F dat_0cba7910[2] = {
#include "ev004b/bird_textures/dat_0cba7910.inc"
};
static NJS_MKEY_A dat_0cba7930[2] = {
#include "ev004b/bird_textures/dat_0cba7930.inc"
};
static NJS_MKEY_F dat_0cba7950[2] = {
#include "ev004b/bird_textures/dat_0cba7950.inc"
};
static NJS_MKEY_A dat_0cba7970[30] = {
#include "ev004b/bird_textures/dat_0cba7970.inc"
};
static NJS_MKEY_F dat_0cba7b50[2] = {
#include "ev004b/bird_textures/dat_0cba7b50.inc"
};
static NJS_MKEY_A dat_0cba7b70[30] = {
#include "ev004b/bird_textures/dat_0cba7b70.inc"
};
static NJS_MKEY_F dat_0cba7d50[2] = {
#include "ev004b/bird_textures/dat_0cba7d50.inc"
};
static NJS_MKEY_A dat_0cba7d70[2] = {
#include "ev004b/bird_textures/dat_0cba7d70.inc"
};
static NJS_MKEY_F dat_0cba7d90[2] = {
#include "ev004b/bird_textures/dat_0cba7d90.inc"
};
static NJS_MKEY_A dat_0cba7db0[2] = {
#include "ev004b/bird_textures/dat_0cba7db0.inc"
};
static NJS_MKEY_F dat_0cba7dd0[2] = {
#include "ev004b/bird_textures/dat_0cba7dd0.inc"
};
static NJS_MKEY_A dat_0cba7df0[2] = {
#include "ev004b/bird_textures/dat_0cba7df0.inc"
};
static NJS_MKEY_F dat_0cba7e10[2] = {
#include "ev004b/bird_textures/dat_0cba7e10.inc"
};
static NJS_MKEY_A dat_0cba7e30[2] = {
#include "ev004b/bird_textures/dat_0cba7e30.inc"
};
static NJS_MKEY_F dat_0cba7e50[2] = {
#include "ev004b/bird_textures/dat_0cba7e50.inc"
};
static NJS_MKEY_A dat_0cba7e70[2] = {
#include "ev004b/bird_textures/dat_0cba7e70.inc"
};
static NJS_MKEY_F dat_0cba7e90[2] = {
#include "ev004b/bird_textures/dat_0cba7e90.inc"
};
static NJS_MKEY_A dat_0cba7eb0[2] = {
#include "ev004b/bird_textures/dat_0cba7eb0.inc"
};
static NJS_MKEY_F dat_0cba7ed0[2] = {
#include "ev004b/bird_textures/dat_0cba7ed0.inc"
};
static NJS_MKEY_A dat_0cba7ef0[2] = {
#include "ev004b/bird_textures/dat_0cba7ef0.inc"
};
static NJS_MKEY_F dat_0cba7f10[2] = {
#include "ev004b/bird_textures/dat_0cba7f10.inc"
};
static NJS_MKEY_A dat_0cba7f30[2] = {
#include "ev004b/bird_textures/dat_0cba7f30.inc"
};
static NJS_MKEY_F dat_0cba7f50[2] = {
#include "ev004b/bird_textures/dat_0cba7f50.inc"
};
static NJS_MKEY_A dat_0cba7f70[2] = {
#include "ev004b/bird_textures/dat_0cba7f70.inc"
};
static NJS_MKEY_F dat_0cba7f90[2] = {
#include "ev004b/bird_textures/dat_0cba7f90.inc"
};
static NJS_MKEY_A dat_0cba7fb0[2] = {
#include "ev004b/bird_textures/dat_0cba7fb0.inc"
};
static NJS_MKEY_F dat_0cba7fd0[2] = {
#include "ev004b/bird_textures/dat_0cba7fd0.inc"
};
static NJS_MKEY_A dat_0cba7ff0[2] = {
#include "ev004b/bird_textures/dat_0cba7ff0.inc"
};
static NJS_MKEY_F dat_0cba8010[2] = {
#include "ev004b/bird_textures/dat_0cba8010.inc"
};
static NJS_MKEY_A dat_0cba8030[2] = {
#include "ev004b/bird_textures/dat_0cba8030.inc"
};
static NJS_MKEY_F dat_0cba8050[2] = {
#include "ev004b/bird_textures/dat_0cba8050.inc"
};
static NJS_MKEY_A dat_0cba8070[2] = {
#include "ev004b/bird_textures/dat_0cba8070.inc"
};
static NJS_MDATA2 dat_0cba8090[24] = {
#include "ev004b/bird_textures/dat_0cba8090.inc"
};
static NJS_MOTION dat_0cba8210 = {
#include "ev004b/bird_textures/dat_0cba8210.inc"
};
/* an action on event_eggmobile_object. no word in EV004B or 1ST_READ.BIN
 * points at it */
static NJS_ACTION dat_0cba821c = {
#include "ev004b/bird_textures/dat_0cba821c.inc"
};
static NJS_MKEY_F dat_0cba8224[2] = {
#include "ev004b/bird_textures/dat_0cba8224.inc"
};
static NJS_MKEY_A dat_0cba8244[2] = {
#include "ev004b/bird_textures/dat_0cba8244.inc"
};
static NJS_MKEY_F dat_0cba8264[2] = {
#include "ev004b/bird_textures/dat_0cba8264.inc"
};
static NJS_MKEY_A dat_0cba8284[30] = {
#include "ev004b/bird_textures/dat_0cba8284.inc"
};
static NJS_MKEY_F dat_0cba8464[2] = {
#include "ev004b/bird_textures/dat_0cba8464.inc"
};
static NJS_MKEY_A dat_0cba8484[30] = {
#include "ev004b/bird_textures/dat_0cba8484.inc"
};
static NJS_MKEY_F dat_0cba8664[2] = {
#include "ev004b/bird_textures/dat_0cba8664.inc"
};
static NJS_MKEY_A dat_0cba8684[2] = {
#include "ev004b/bird_textures/dat_0cba8684.inc"
};
static NJS_MKEY_F dat_0cba86a4[2] = {
#include "ev004b/bird_textures/dat_0cba86a4.inc"
};
static NJS_MKEY_A dat_0cba86c4[30] = {
#include "ev004b/bird_textures/dat_0cba86c4.inc"
};
static NJS_MKEY_F dat_0cba88a4[2] = {
#include "ev004b/bird_textures/dat_0cba88a4.inc"
};
static NJS_MKEY_A dat_0cba88c4[30] = {
#include "ev004b/bird_textures/dat_0cba88c4.inc"
};
static NJS_MKEY_F dat_0cba8aa4[2] = {
#include "ev004b/bird_textures/dat_0cba8aa4.inc"
};
static NJS_MKEY_A dat_0cba8ac4[2] = {
#include "ev004b/bird_textures/dat_0cba8ac4.inc"
};
static NJS_MKEY_F dat_0cba8ae4[2] = {
#include "ev004b/bird_textures/dat_0cba8ae4.inc"
};
static NJS_MKEY_A dat_0cba8b04[2] = {
#include "ev004b/bird_textures/dat_0cba8b04.inc"
};
static NJS_MKEY_F dat_0cba8b24[2] = {
#include "ev004b/bird_textures/dat_0cba8b24.inc"
};
static NJS_MKEY_A dat_0cba8b44[2] = {
#include "ev004b/bird_textures/dat_0cba8b44.inc"
};
static NJS_MKEY_F dat_0cba8b64[2] = {
#include "ev004b/bird_textures/dat_0cba8b64.inc"
};
static NJS_MKEY_A dat_0cba8b84[30] = {
#include "ev004b/bird_textures/dat_0cba8b84.inc"
};
static NJS_MKEY_F dat_0cba8d64[2] = {
#include "ev004b/bird_textures/dat_0cba8d64.inc"
};
static NJS_MKEY_A dat_0cba8d84[30] = {
#include "ev004b/bird_textures/dat_0cba8d84.inc"
};
static NJS_MKEY_F dat_0cba8f64[2] = {
#include "ev004b/bird_textures/dat_0cba8f64.inc"
};
static NJS_MKEY_A dat_0cba8f84[2] = {
#include "ev004b/bird_textures/dat_0cba8f84.inc"
};
static NJS_MKEY_F dat_0cba8fa4[2] = {
#include "ev004b/bird_textures/dat_0cba8fa4.inc"
};
static NJS_MKEY_A dat_0cba8fc4[2] = {
#include "ev004b/bird_textures/dat_0cba8fc4.inc"
};
static NJS_MKEY_F dat_0cba8fe4[2] = {
#include "ev004b/bird_textures/dat_0cba8fe4.inc"
};
static NJS_MKEY_A dat_0cba9004[2] = {
#include "ev004b/bird_textures/dat_0cba9004.inc"
};
static NJS_MKEY_F dat_0cba9024[2] = {
#include "ev004b/bird_textures/dat_0cba9024.inc"
};
static NJS_MKEY_A dat_0cba9044[2] = {
#include "ev004b/bird_textures/dat_0cba9044.inc"
};
static NJS_MKEY_F dat_0cba9064[2] = {
#include "ev004b/bird_textures/dat_0cba9064.inc"
};
static NJS_MKEY_A dat_0cba9084[2] = {
#include "ev004b/bird_textures/dat_0cba9084.inc"
};
static NJS_MKEY_F dat_0cba90a4[2] = {
#include "ev004b/bird_textures/dat_0cba90a4.inc"
};
static NJS_MKEY_A dat_0cba90c4[2] = {
#include "ev004b/bird_textures/dat_0cba90c4.inc"
};
static NJS_MKEY_F dat_0cba90e4[2] = {
#include "ev004b/bird_textures/dat_0cba90e4.inc"
};
static NJS_MKEY_A dat_0cba9104[2] = {
#include "ev004b/bird_textures/dat_0cba9104.inc"
};
static NJS_MKEY_F dat_0cba9124[2] = {
#include "ev004b/bird_textures/dat_0cba9124.inc"
};
static NJS_MKEY_A dat_0cba9144[2] = {
#include "ev004b/bird_textures/dat_0cba9144.inc"
};
static NJS_MKEY_F dat_0cba9164[2] = {
#include "ev004b/bird_textures/dat_0cba9164.inc"
};
static NJS_MKEY_A dat_0cba9184[2] = {
#include "ev004b/bird_textures/dat_0cba9184.inc"
};
static NJS_MKEY_F dat_0cba91a4[2] = {
#include "ev004b/bird_textures/dat_0cba91a4.inc"
};
static NJS_MKEY_A dat_0cba91c4[2] = {
#include "ev004b/bird_textures/dat_0cba91c4.inc"
};
static NJS_MKEY_F dat_0cba91e4[2] = {
#include "ev004b/bird_textures/dat_0cba91e4.inc"
};
static NJS_MKEY_A dat_0cba9204[2] = {
#include "ev004b/bird_textures/dat_0cba9204.inc"
};
static NJS_MKEY_F dat_0cba9224[2] = {
#include "ev004b/bird_textures/dat_0cba9224.inc"
};
static NJS_MKEY_A dat_0cba9244[2] = {
#include "ev004b/bird_textures/dat_0cba9244.inc"
};
static NJS_MKEY_F dat_0cba9264[2] = {
#include "ev004b/bird_textures/dat_0cba9264.inc"
};
static NJS_MKEY_A dat_0cba9284[2] = {
#include "ev004b/bird_textures/dat_0cba9284.inc"
};
static NJS_MDATA2 dat_0cba92a4[24] = {
#include "ev004b/bird_textures/dat_0cba92a4.inc"
};
static NJS_MOTION dat_0cba9424 = {
#include "ev004b/bird_textures/dat_0cba9424.inc"
};
/* an action on event_eggmobile_object. no word in EV004B or 1ST_READ.BIN
 * points at it */
static NJS_ACTION dat_0cba9430 = {
#include "ev004b/bird_textures/dat_0cba9430.inc"
};
static NJS_MKEY_F dat_0cba9438[2] = {
#include "ev004b/bird_textures/dat_0cba9438.inc"
};
static NJS_MKEY_F dat_0cba9458[2] = {
#include "ev004b/bird_textures/dat_0cba9458.inc"
};
static NJS_MKEY_A dat_0cba9478[32] = {
#include "ev004b/bird_textures/dat_0cba9478.inc"
};
static NJS_MKEY_F dat_0cba9678[2] = {
#include "ev004b/bird_textures/dat_0cba9678.inc"
};
static NJS_MKEY_A dat_0cba9698[33] = {
#include "ev004b/bird_textures/dat_0cba9698.inc"
};
static NJS_MKEY_F dat_0cba98a8[2] = {
#include "ev004b/bird_textures/dat_0cba98a8.inc"
};
static NJS_MKEY_F dat_0cba98c8[2] = {
#include "ev004b/bird_textures/dat_0cba98c8.inc"
};
static NJS_MKEY_A dat_0cba98e8[33] = {
#include "ev004b/bird_textures/dat_0cba98e8.inc"
};
static NJS_MKEY_F dat_0cba9af8[2] = {
#include "ev004b/bird_textures/dat_0cba9af8.inc"
};
static NJS_MKEY_A dat_0cba9b18[33] = {
#include "ev004b/bird_textures/dat_0cba9b18.inc"
};
static NJS_MKEY_F dat_0cba9d28[2] = {
#include "ev004b/bird_textures/dat_0cba9d28.inc"
};
static NJS_MKEY_F dat_0cba9d48[2] = {
#include "ev004b/bird_textures/dat_0cba9d48.inc"
};
static NJS_MKEY_F dat_0cba9d68[2] = {
#include "ev004b/bird_textures/dat_0cba9d68.inc"
};
static NJS_MKEY_F dat_0cba9d88[2] = {
#include "ev004b/bird_textures/dat_0cba9d88.inc"
};
static NJS_MKEY_A dat_0cba9da8[33] = {
#include "ev004b/bird_textures/dat_0cba9da8.inc"
};
static NJS_MKEY_F dat_0cba9fb8[2] = {
#include "ev004b/bird_textures/dat_0cba9fb8.inc"
};
static NJS_MKEY_A dat_0cba9fd8[33] = {
#include "ev004b/bird_textures/dat_0cba9fd8.inc"
};
static NJS_MKEY_F dat_0cbaa1e8[2] = {
#include "ev004b/bird_textures/dat_0cbaa1e8.inc"
};
static NJS_MKEY_A dat_0cbaa208[32] = {
#include "ev004b/bird_textures/dat_0cbaa208.inc"
};
static NJS_MKEY_F dat_0cbaa408[2] = {
#include "ev004b/bird_textures/dat_0cbaa408.inc"
};
static NJS_MKEY_A dat_0cbaa428[2] = {
#include "ev004b/bird_textures/dat_0cbaa428.inc"
};
static NJS_MKEY_F dat_0cbaa448[2] = {
#include "ev004b/bird_textures/dat_0cbaa448.inc"
};
static NJS_MKEY_F dat_0cbaa468[2] = {
#include "ev004b/bird_textures/dat_0cbaa468.inc"
};
static NJS_MKEY_F dat_0cbaa488[2] = {
#include "ev004b/bird_textures/dat_0cbaa488.inc"
};
static NJS_MKEY_F dat_0cbaa4a8[2] = {
#include "ev004b/bird_textures/dat_0cbaa4a8.inc"
};
static NJS_MKEY_F dat_0cbaa4c8[2] = {
#include "ev004b/bird_textures/dat_0cbaa4c8.inc"
};
static NJS_MKEY_A dat_0cbaa4e8[2] = {
#include "ev004b/bird_textures/dat_0cbaa4e8.inc"
};
static NJS_MKEY_F dat_0cbaa508[2] = {
#include "ev004b/bird_textures/dat_0cbaa508.inc"
};
static NJS_MKEY_A dat_0cbaa528[2] = {
#include "ev004b/bird_textures/dat_0cbaa528.inc"
};
static NJS_MKEY_F dat_0cbaa548[2] = {
#include "ev004b/bird_textures/dat_0cbaa548.inc"
};
static NJS_MKEY_A dat_0cbaa568[2] = {
#include "ev004b/bird_textures/dat_0cbaa568.inc"
};
static NJS_MKEY_F dat_0cbaa588[2] = {
#include "ev004b/bird_textures/dat_0cbaa588.inc"
};
static NJS_MKEY_A dat_0cbaa5a8[2] = {
#include "ev004b/bird_textures/dat_0cbaa5a8.inc"
};
static NJS_MKEY_F dat_0cbaa5c8[2] = {
#include "ev004b/bird_textures/dat_0cbaa5c8.inc"
};
static NJS_MKEY_A dat_0cbaa5e8[2] = {
#include "ev004b/bird_textures/dat_0cbaa5e8.inc"
};
static NJS_MKEY_F dat_0cbaa608[2] = {
#include "ev004b/bird_textures/dat_0cbaa608.inc"
};
static NJS_MKEY_A dat_0cbaa628[2] = {
#include "ev004b/bird_textures/dat_0cbaa628.inc"
};
static NJS_MDATA2 ev004b_eggmobile_motion_1_mdata[24] = {
#include "ev004b/bird_textures/ev004b_eggmobile_motion_1_mdata.inc"
};
static NJS_MOTION ev004b_eggmobile_motion_1 = {
#include "ev004b/bird_textures/ev004b_eggmobile_motion_1.inc"
};
NJS_ACTION ev004b_eggmobile_action_1 = {
#include "ev004b/bird_textures/ev004b_eggmobile_action_1.inc"
};
static NJS_MKEY_F dat_0cbaa7dc[31] = {
#include "ev004b/bird_textures/dat_0cbaa7dc.inc"
};
static NJS_MKEY_A dat_0cbaa9cc[40] = {
#include "ev004b/bird_textures/dat_0cbaa9cc.inc"
};
static NJS_MKEY_A dat_0cbaac4c[40] = {
#include "ev004b/bird_textures/dat_0cbaac4c.inc"
};
static NJS_MKEY_A dat_0cbaaecc[40] = {
#include "ev004b/bird_textures/dat_0cbaaecc.inc"
};
static NJS_MKEY_A dat_0cbab14c[40] = {
#include "ev004b/bird_textures/dat_0cbab14c.inc"
};
static NJS_MKEY_A dat_0cbab3cc[40] = {
#include "ev004b/bird_textures/dat_0cbab3cc.inc"
};
static NJS_MKEY_A dat_0cbab64c[40] = {
#include "ev004b/bird_textures/dat_0cbab64c.inc"
};
static NJS_MKEY_A dat_0cbab8cc[40] = {
#include "ev004b/bird_textures/dat_0cbab8cc.inc"
};
static NJS_MKEY_F dat_0cbabb4c[31] = {
#include "ev004b/bird_textures/dat_0cbabb4c.inc"
};
static NJS_MDATA2 ev004b_eggmobile_motion_2_mdata[24] = {
#include "ev004b/bird_textures/ev004b_eggmobile_motion_2_mdata.inc"
};
static NJS_MOTION ev004b_eggmobile_motion_2 = {
#include "ev004b/bird_textures/ev004b_eggmobile_motion_2.inc"
};
NJS_ACTION ev004b_eggmobile_action_2 = {
#include "ev004b/bird_textures/ev004b_eggmobile_action_2.inc"
};
static NJS_MKEY_F dat_0cbabed0[16] = {
#include "ev004b/bird_textures/dat_0cbabed0.inc"
};
static NJS_MKEY_A dat_0cbabfd0[16] = {
#include "ev004b/bird_textures/dat_0cbabfd0.inc"
};
static NJS_MKEY_A dat_0cbac0d0[16] = {
#include "ev004b/bird_textures/dat_0cbac0d0.inc"
};
static NJS_MKEY_A dat_0cbac1d0[16] = {
#include "ev004b/bird_textures/dat_0cbac1d0.inc"
};
static NJS_MKEY_A dat_0cbac2d0[16] = {
#include "ev004b/bird_textures/dat_0cbac2d0.inc"
};
static NJS_MKEY_A dat_0cbac3d0[16] = {
#include "ev004b/bird_textures/dat_0cbac3d0.inc"
};
static NJS_MKEY_A dat_0cbac4d0[16] = {
#include "ev004b/bird_textures/dat_0cbac4d0.inc"
};
static NJS_MDATA2 dat_0cbac5d0[24] = {
#include "ev004b/bird_textures/dat_0cbac5d0.inc"
};
static NJS_MOTION dat_0cbac750 = {
#include "ev004b/bird_textures/dat_0cbac750.inc"
};
/* an action on event_eggmobile_object. no word in EV004B or 1ST_READ.BIN
 * points at it */
static NJS_ACTION dat_0cbac75c = {
#include "ev004b/bird_textures/dat_0cbac75c.inc"
};
static NJS_MKEY_F dat_0cbac764[16] = {
#include "ev004b/bird_textures/dat_0cbac764.inc"
};
static NJS_MKEY_A dat_0cbac864[2] = {
#include "ev004b/bird_textures/dat_0cbac864.inc"
};
static NJS_MKEY_F dat_0cbac884[2] = {
#include "ev004b/bird_textures/dat_0cbac884.inc"
};
static NJS_MKEY_A dat_0cbac8a4[16] = {
#include "ev004b/bird_textures/dat_0cbac8a4.inc"
};
static NJS_MKEY_F dat_0cbac9a4[2] = {
#include "ev004b/bird_textures/dat_0cbac9a4.inc"
};
static NJS_MKEY_A dat_0cbac9c4[16] = {
#include "ev004b/bird_textures/dat_0cbac9c4.inc"
};
static NJS_MKEY_F dat_0cbacac4[2] = {
#include "ev004b/bird_textures/dat_0cbacac4.inc"
};
static NJS_MKEY_A dat_0cbacae4[2] = {
#include "ev004b/bird_textures/dat_0cbacae4.inc"
};
static NJS_MKEY_F dat_0cbacb04[2] = {
#include "ev004b/bird_textures/dat_0cbacb04.inc"
};
static NJS_MKEY_A dat_0cbacb24[16] = {
#include "ev004b/bird_textures/dat_0cbacb24.inc"
};
static NJS_MKEY_F dat_0cbacc24[2] = {
#include "ev004b/bird_textures/dat_0cbacc24.inc"
};
static NJS_MKEY_A dat_0cbacc44[16] = {
#include "ev004b/bird_textures/dat_0cbacc44.inc"
};
static NJS_MKEY_F dat_0cbacd44[2] = {
#include "ev004b/bird_textures/dat_0cbacd44.inc"
};
static NJS_MKEY_A dat_0cbacd64[2] = {
#include "ev004b/bird_textures/dat_0cbacd64.inc"
};
static NJS_MKEY_F dat_0cbacd84[2] = {
#include "ev004b/bird_textures/dat_0cbacd84.inc"
};
static NJS_MKEY_A dat_0cbacda4[2] = {
#include "ev004b/bird_textures/dat_0cbacda4.inc"
};
static NJS_MKEY_F dat_0cbacdc4[2] = {
#include "ev004b/bird_textures/dat_0cbacdc4.inc"
};
static NJS_MKEY_A dat_0cbacde4[2] = {
#include "ev004b/bird_textures/dat_0cbacde4.inc"
};
static NJS_MKEY_F dat_0cbace04[2] = {
#include "ev004b/bird_textures/dat_0cbace04.inc"
};
static NJS_MKEY_A dat_0cbace24[16] = {
#include "ev004b/bird_textures/dat_0cbace24.inc"
};
static NJS_MKEY_F dat_0cbacf24[2] = {
#include "ev004b/bird_textures/dat_0cbacf24.inc"
};
static NJS_MKEY_A dat_0cbacf44[16] = {
#include "ev004b/bird_textures/dat_0cbacf44.inc"
};
static NJS_MKEY_F dat_0cbad044[2] = {
#include "ev004b/bird_textures/dat_0cbad044.inc"
};
static NJS_MKEY_A dat_0cbad064[2] = {
#include "ev004b/bird_textures/dat_0cbad064.inc"
};
static NJS_MKEY_F dat_0cbad084[2] = {
#include "ev004b/bird_textures/dat_0cbad084.inc"
};
static NJS_MKEY_A dat_0cbad0a4[2] = {
#include "ev004b/bird_textures/dat_0cbad0a4.inc"
};
static NJS_MKEY_F dat_0cbad0c4[2] = {
#include "ev004b/bird_textures/dat_0cbad0c4.inc"
};
static NJS_MKEY_A dat_0cbad0e4[2] = {
#include "ev004b/bird_textures/dat_0cbad0e4.inc"
};
static NJS_MKEY_F dat_0cbad104[2] = {
#include "ev004b/bird_textures/dat_0cbad104.inc"
};
static NJS_MKEY_A dat_0cbad124[2] = {
#include "ev004b/bird_textures/dat_0cbad124.inc"
};
static NJS_MKEY_F dat_0cbad144[2] = {
#include "ev004b/bird_textures/dat_0cbad144.inc"
};
static NJS_MKEY_A dat_0cbad164[2] = {
#include "ev004b/bird_textures/dat_0cbad164.inc"
};
static NJS_MKEY_F dat_0cbad184[2] = {
#include "ev004b/bird_textures/dat_0cbad184.inc"
};
static NJS_MKEY_A dat_0cbad1a4[2] = {
#include "ev004b/bird_textures/dat_0cbad1a4.inc"
};
static NJS_MKEY_F dat_0cbad1c4[2] = {
#include "ev004b/bird_textures/dat_0cbad1c4.inc"
};
static NJS_MKEY_A dat_0cbad1e4[2] = {
#include "ev004b/bird_textures/dat_0cbad1e4.inc"
};
static NJS_MKEY_F dat_0cbad204[2] = {
#include "ev004b/bird_textures/dat_0cbad204.inc"
};
static NJS_MKEY_A dat_0cbad224[2] = {
#include "ev004b/bird_textures/dat_0cbad224.inc"
};
static NJS_MKEY_F dat_0cbad244[2] = {
#include "ev004b/bird_textures/dat_0cbad244.inc"
};
static NJS_MKEY_A dat_0cbad264[2] = {
#include "ev004b/bird_textures/dat_0cbad264.inc"
};
static NJS_MKEY_F dat_0cbad284[2] = {
#include "ev004b/bird_textures/dat_0cbad284.inc"
};
static NJS_MKEY_A dat_0cbad2a4[2] = {
#include "ev004b/bird_textures/dat_0cbad2a4.inc"
};
static NJS_MKEY_F dat_0cbad2c4[2] = {
#include "ev004b/bird_textures/dat_0cbad2c4.inc"
};
static NJS_MKEY_A dat_0cbad2e4[2] = {
#include "ev004b/bird_textures/dat_0cbad2e4.inc"
};
static NJS_MKEY_F dat_0cbad304[2] = {
#include "ev004b/bird_textures/dat_0cbad304.inc"
};
static NJS_MKEY_A dat_0cbad324[2] = {
#include "ev004b/bird_textures/dat_0cbad324.inc"
};
static NJS_MKEY_F dat_0cbad344[2] = {
#include "ev004b/bird_textures/dat_0cbad344.inc"
};
static NJS_MKEY_A dat_0cbad364[2] = {
#include "ev004b/bird_textures/dat_0cbad364.inc"
};
static NJS_MDATA2 dat_0cbad384[24] = {
#include "ev004b/bird_textures/dat_0cbad384.inc"
};
static NJS_MOTION dat_0cbad504 = {
#include "ev004b/bird_textures/dat_0cbad504.inc"
};
/* an action on event_eggmobile_object. no word in EV004B or 1ST_READ.BIN
 * points at it */
static NJS_ACTION dat_0cbad510 = {
#include "ev004b/bird_textures/dat_0cbad510.inc"
};
static NJS_MKEY_F dat_0cbad518[2] = {
#include "ev004b/bird_textures/dat_0cbad518.inc"
};
static NJS_MKEY_A dat_0cbad538[2] = {
#include "ev004b/bird_textures/dat_0cbad538.inc"
};
static NJS_MKEY_F dat_0cbad558[2] = {
#include "ev004b/bird_textures/dat_0cbad558.inc"
};
static NJS_MKEY_A dat_0cbad578[32] = {
#include "ev004b/bird_textures/dat_0cbad578.inc"
};
static NJS_MKEY_F dat_0cbad778[2] = {
#include "ev004b/bird_textures/dat_0cbad778.inc"
};
static NJS_MKEY_A dat_0cbad798[32] = {
#include "ev004b/bird_textures/dat_0cbad798.inc"
};
static NJS_MKEY_F dat_0cbad998[2] = {
#include "ev004b/bird_textures/dat_0cbad998.inc"
};
static NJS_MKEY_A dat_0cbad9b8[2] = {
#include "ev004b/bird_textures/dat_0cbad9b8.inc"
};
static NJS_MKEY_F dat_0cbad9d8[2] = {
#include "ev004b/bird_textures/dat_0cbad9d8.inc"
};
static NJS_MKEY_A dat_0cbad9f8[28] = {
#include "ev004b/bird_textures/dat_0cbad9f8.inc"
};
static NJS_MKEY_F dat_0cbadbb8[2] = {
#include "ev004b/bird_textures/dat_0cbadbb8.inc"
};
static NJS_MKEY_A dat_0cbadbd8[32] = {
#include "ev004b/bird_textures/dat_0cbadbd8.inc"
};
static NJS_MKEY_F dat_0cbaddd8[2] = {
#include "ev004b/bird_textures/dat_0cbaddd8.inc"
};
static NJS_MKEY_A dat_0cbaddf8[2] = {
#include "ev004b/bird_textures/dat_0cbaddf8.inc"
};
static NJS_MKEY_F dat_0cbade18[2] = {
#include "ev004b/bird_textures/dat_0cbade18.inc"
};
static NJS_MKEY_A dat_0cbade38[2] = {
#include "ev004b/bird_textures/dat_0cbade38.inc"
};
static NJS_MKEY_F dat_0cbade58[2] = {
#include "ev004b/bird_textures/dat_0cbade58.inc"
};
static NJS_MKEY_A dat_0cbade78[2] = {
#include "ev004b/bird_textures/dat_0cbade78.inc"
};
static NJS_MKEY_F dat_0cbade98[2] = {
#include "ev004b/bird_textures/dat_0cbade98.inc"
};
static NJS_MKEY_A dat_0cbadeb8[28] = {
#include "ev004b/bird_textures/dat_0cbadeb8.inc"
};
static NJS_MKEY_F dat_0cbae078[2] = {
#include "ev004b/bird_textures/dat_0cbae078.inc"
};
static NJS_MKEY_A dat_0cbae098[32] = {
#include "ev004b/bird_textures/dat_0cbae098.inc"
};
static NJS_MKEY_F dat_0cbae298[2] = {
#include "ev004b/bird_textures/dat_0cbae298.inc"
};
static NJS_MKEY_A dat_0cbae2b8[2] = {
#include "ev004b/bird_textures/dat_0cbae2b8.inc"
};
static NJS_MKEY_F dat_0cbae2d8[2] = {
#include "ev004b/bird_textures/dat_0cbae2d8.inc"
};
static NJS_MKEY_A dat_0cbae2f8[2] = {
#include "ev004b/bird_textures/dat_0cbae2f8.inc"
};
static NJS_MKEY_F dat_0cbae318[2] = {
#include "ev004b/bird_textures/dat_0cbae318.inc"
};
static NJS_MKEY_A dat_0cbae338[2] = {
#include "ev004b/bird_textures/dat_0cbae338.inc"
};
static NJS_MKEY_F dat_0cbae358[2] = {
#include "ev004b/bird_textures/dat_0cbae358.inc"
};
static NJS_MKEY_A dat_0cbae378[2] = {
#include "ev004b/bird_textures/dat_0cbae378.inc"
};
static NJS_MKEY_F dat_0cbae398[2] = {
#include "ev004b/bird_textures/dat_0cbae398.inc"
};
static NJS_MKEY_A dat_0cbae3b8[2] = {
#include "ev004b/bird_textures/dat_0cbae3b8.inc"
};
static NJS_MKEY_F dat_0cbae3d8[2] = {
#include "ev004b/bird_textures/dat_0cbae3d8.inc"
};
static NJS_MKEY_A dat_0cbae3f8[2] = {
#include "ev004b/bird_textures/dat_0cbae3f8.inc"
};
static NJS_MKEY_F dat_0cbae418[2] = {
#include "ev004b/bird_textures/dat_0cbae418.inc"
};
static NJS_MKEY_A dat_0cbae438[2] = {
#include "ev004b/bird_textures/dat_0cbae438.inc"
};
static NJS_MKEY_F dat_0cbae458[2] = {
#include "ev004b/bird_textures/dat_0cbae458.inc"
};
static NJS_MKEY_A dat_0cbae478[2] = {
#include "ev004b/bird_textures/dat_0cbae478.inc"
};
static NJS_MKEY_F dat_0cbae498[2] = {
#include "ev004b/bird_textures/dat_0cbae498.inc"
};
static NJS_MKEY_A dat_0cbae4b8[2] = {
#include "ev004b/bird_textures/dat_0cbae4b8.inc"
};
static NJS_MKEY_F dat_0cbae4d8[2] = {
#include "ev004b/bird_textures/dat_0cbae4d8.inc"
};
static NJS_MKEY_A dat_0cbae4f8[2] = {
#include "ev004b/bird_textures/dat_0cbae4f8.inc"
};
static NJS_MKEY_F dat_0cbae518[2] = {
#include "ev004b/bird_textures/dat_0cbae518.inc"
};
static NJS_MKEY_A dat_0cbae538[2] = {
#include "ev004b/bird_textures/dat_0cbae538.inc"
};
static NJS_MKEY_F dat_0cbae558[2] = {
#include "ev004b/bird_textures/dat_0cbae558.inc"
};
static NJS_MKEY_A dat_0cbae578[2] = {
#include "ev004b/bird_textures/dat_0cbae578.inc"
};
static NJS_MKEY_F dat_0cbae598[2] = {
#include "ev004b/bird_textures/dat_0cbae598.inc"
};
static NJS_MKEY_A dat_0cbae5b8[2] = {
#include "ev004b/bird_textures/dat_0cbae5b8.inc"
};
static NJS_MDATA2 ev004b_eggmobile_motion_3_mdata[24] = {
#include "ev004b/bird_textures/ev004b_eggmobile_motion_3_mdata.inc"
};
static NJS_MOTION ev004b_eggmobile_motion_3 = {
#include "ev004b/bird_textures/ev004b_eggmobile_motion_3.inc"
};
NJS_ACTION ev004b_eggmobile_action_3 = {
#include "ev004b/bird_textures/ev004b_eggmobile_action_3.inc"
};
static NJS_MKEY_A dat_0cbae76c[31] = {
#include "ev004b/bird_textures/dat_0cbae76c.inc"
};
static NJS_MKEY_A dat_0cbae95c[32] = {
#include "ev004b/bird_textures/dat_0cbae95c.inc"
};
static NJS_MKEY_A dat_0cbaeb5c[32] = {
#include "ev004b/bird_textures/dat_0cbaeb5c.inc"
};
static NJS_MKEY_A dat_0cbaed5c[32] = {
#include "ev004b/bird_textures/dat_0cbaed5c.inc"
};
static NJS_MKEY_A dat_0cbaef5c[32] = {
#include "ev004b/bird_textures/dat_0cbaef5c.inc"
};
static NJS_MKEY_A dat_0cbaf15c[32] = {
#include "ev004b/bird_textures/dat_0cbaf15c.inc"
};
static NJS_MKEY_A dat_0cbaf35c[2] = {
#include "ev004b/bird_textures/dat_0cbaf35c.inc"
};
static NJS_MKEY_A dat_0cbaf37c[2] = {
#include "ev004b/bird_textures/dat_0cbaf37c.inc"
};
static NJS_MKEY_A dat_0cbaf39c[31] = {
#include "ev004b/bird_textures/dat_0cbaf39c.inc"
};
static NJS_MDATA2 ev004b_eggmobile_motion_4_mdata[24] = {
#include "ev004b/bird_textures/ev004b_eggmobile_motion_4_mdata.inc"
};
static NJS_MOTION ev004b_eggmobile_motion_4 = {
#include "ev004b/bird_textures/ev004b_eggmobile_motion_4.inc"
};
NJS_ACTION ev004b_eggmobile_action_4 = {
#include "ev004b/bird_textures/ev004b_eggmobile_action_4.inc"
};
static NJS_TEXNAME ev004b_emerald_textures[1] = {
    {TEXT_13, 0, 0},
};
NJS_TEXLIST ev004b_emerald_texlist = {
#include "ev004b/bird_textures/ev004b_emerald_texlist.inc"
};
/* the material of dat_0cbafd18, one of the four models in
 * ev004b_emerald_object's tree, which ev004b_emerald_action moves */
NJS_MATERIAL dat_0cbaf734[1] = {
#include "ev004b/bird_textures/dat_0cbaf734.inc"
};
