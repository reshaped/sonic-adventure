/* Zero's texture names and zero_texlist, then the keys, the motion data
 * and the motion that zero_hit_behind_action plays */
#include <ninja.h>

#include "enemy/zero.h"
#include "enemy/zero_textures.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 10
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 10
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 11
#define TEXT_18_LENGTH 11
#define TEXT_19_LENGTH 11
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 12
#define TEXT_22_LENGTH 12
#define TEXT_23_LENGTH 12
#define TEXT_24_LENGTH 10
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 10
#define TEXT_27_LENGTH 10
#define TEXT_28_LENGTH 10
#define TEXT_29_LENGTH 10
#include "enemy/zero_textures/text.inc"

static NJS_TEXNAME zero_textures[29] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0},
};
NJS_TEXLIST zero_texlist = {
#include "enemy/zero_textures/zero_texlist.inc"
};
static NJS_MKEY_F dat_8c5e649c[18] = {
#include "enemy/zero_textures/dat_8c5e649c.inc"
};
static NJS_MKEY_A dat_8c5e65bc[30] = {
#include "enemy/zero_textures/dat_8c5e65bc.inc"
};
static NJS_MKEY_A dat_8c5e679c[10] = {
#include "enemy/zero_textures/dat_8c5e679c.inc"
};
static NJS_MKEY_A dat_8c5e683c[23] = {
#include "enemy/zero_textures/dat_8c5e683c.inc"
};
static NJS_MKEY_A dat_8c5e69ac[25] = {
#include "enemy/zero_textures/dat_8c5e69ac.inc"
};
static NJS_MKEY_A dat_8c5e6b3c[23] = {
#include "enemy/zero_textures/dat_8c5e6b3c.inc"
};
static NJS_MKEY_A dat_8c5e6cac[23] = {
#include "enemy/zero_textures/dat_8c5e6cac.inc"
};
static NJS_MKEY_A dat_8c5e6e1c[15] = {
#include "enemy/zero_textures/dat_8c5e6e1c.inc"
};
static NJS_MKEY_A dat_8c5e6f0c[29] = {
#include "enemy/zero_textures/dat_8c5e6f0c.inc"
};
static NJS_MKEY_A dat_8c5e70dc[29] = {
#include "enemy/zero_textures/dat_8c5e70dc.inc"
};
static NJS_MKEY_A dat_8c5e72ac[29] = {
#include "enemy/zero_textures/dat_8c5e72ac.inc"
};
static NJS_MKEY_A dat_8c5e747c[17] = {
#include "enemy/zero_textures/dat_8c5e747c.inc"
};
static NJS_MKEY_A dat_8c5e758c[17] = {
#include "enemy/zero_textures/dat_8c5e758c.inc"
};
static NJS_MKEY_A dat_8c5e769c[21] = {
#include "enemy/zero_textures/dat_8c5e769c.inc"
};
static NJS_MKEY_A dat_8c5e77ec[21] = {
#include "enemy/zero_textures/dat_8c5e77ec.inc"
};
static NJS_MKEY_A dat_8c5e793c[29] = {
#include "enemy/zero_textures/dat_8c5e793c.inc"
};
static NJS_MKEY_A dat_8c5e7b0c[29] = {
#include "enemy/zero_textures/dat_8c5e7b0c.inc"
};
static NJS_MKEY_A dat_8c5e7cdc[29] = {
#include "enemy/zero_textures/dat_8c5e7cdc.inc"
};
static NJS_MKEY_A dat_8c5e7eac[17] = {
#include "enemy/zero_textures/dat_8c5e7eac.inc"
};
static NJS_MKEY_A dat_8c5e7fbc[17] = {
#include "enemy/zero_textures/dat_8c5e7fbc.inc"
};
static NJS_MKEY_A dat_8c5e80cc[21] = {
#include "enemy/zero_textures/dat_8c5e80cc.inc"
};
static NJS_MKEY_A dat_8c5e821c[21] = {
#include "enemy/zero_textures/dat_8c5e821c.inc"
};
static NJS_MKEY_A dat_8c5e836c[23] = {
#include "enemy/zero_textures/dat_8c5e836c.inc"
};
static NJS_MKEY_A dat_8c5e84dc[28] = {
#include "enemy/zero_textures/dat_8c5e84dc.inc"
};
static NJS_MDATA2 dat_8c5e869c[48] = {
#include "enemy/zero_textures/dat_8c5e869c.inc"
};
NJS_MOTION zero_hit_behind_motion = {
#include "enemy/zero_textures/zero_hit_behind_motion.inc"
};
