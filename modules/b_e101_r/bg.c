/* the sea and the sky, and the names and texlist of their textures.
 * B_E101_R and B_ROBO, the fights with E-101 mkII and Zero, both link this
 * object, and the dat_ names here give B_E101_R's addresses */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_e101_r/bg.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 11
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 15
#define TEXT_12_LENGTH 15
#define TEXT_13_LENGTH 15
#define TEXT_14_LENGTH 14
#include "b_e101_r/bg/text.inc"

static NJS_TEXNAME e101_r_bg_textures[14] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
};
NJS_TEXLIST e101_r_bg_texlist = {
#include "b_e101_r/bg/e101_r_bg_texlist.inc"
};
static NJS_MATERIAL dat_0c9080b8[1] = {
#include "b_e101_r/bg/dat_0c9080b8.inc"
};
static Sint16 dat_0c9080cc[6] = {
#include "b_e101_r/bg/dat_0c9080cc.inc"
};
static NJS_COLOR dat_0c9080d8[4] = {
#include "b_e101_r/bg/dat_0c9080d8.inc"
};
static NJS_MESHSET dat_0c9080e8[1] = {
#include "b_e101_r/bg/dat_0c9080e8.inc"
};
static NJS_POINT3 dat_0c908100[4] = {
#include "b_e101_r/bg/dat_0c908100.inc"
};
static NJS_VECTOR dat_0c908130[4] = {
#include "b_e101_r/bg/dat_0c908130.inc"
};
static NJS_MODEL dat_0c908160 = {
#include "b_e101_r/bg/dat_0c908160.inc"
};
NJS_OBJECT e101_r_sea_object = {
#include "b_e101_r/bg/e101_r_sea_object.inc"
};
static NJS_MATERIAL dat_0c9081bc[4] = {
#include "b_e101_r/bg/dat_0c9081bc.inc"
};
static Sint16 dat_0c90820c[146] = {
#include "b_e101_r/bg/dat_0c90820c.inc"
};
static Sint16 dat_0c908330[146] = {
#include "b_e101_r/bg/dat_0c908330.inc"
};
static Sint16 dat_0c908454[10] = {
#include "b_e101_r/bg/dat_0c908454.inc"
};
static Sint16 dat_0c908468[24] = {
#include "b_e101_r/bg/dat_0c908468.inc"
};
static NJS_COLOR dat_0c908498[140] = {
#include "b_e101_r/bg/dat_0c908498.inc"
};
static NJS_COLOR dat_0c9086c8[140] = {
#include "b_e101_r/bg/dat_0c9086c8.inc"
};
static NJS_COLOR dat_0c9088f8[8] = {
#include "b_e101_r/bg/dat_0c9088f8.inc"
};
static NJS_COLOR dat_0c908918[20] = {
#include "b_e101_r/bg/dat_0c908918.inc"
};
static NJS_MESHSET dat_0c908968[4] = {
#include "b_e101_r/bg/dat_0c908968.inc"
};
static NJS_POINT3 dat_0c9089c8[154] = {
#include "b_e101_r/bg/dat_0c9089c8.inc"
};
static NJS_VECTOR dat_0c909100[154] = {
#include "b_e101_r/bg/dat_0c909100.inc"
};
static NJS_MODEL dat_0c909838 = {
#include "b_e101_r/bg/dat_0c909838.inc"
};
NJS_OBJECT e101_r_sky_object = {
#include "b_e101_r/bg/e101_r_sky_object.inc"
};
