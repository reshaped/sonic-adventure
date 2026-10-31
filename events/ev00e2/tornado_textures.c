/* an action on Froggy's model and its motion, which nothing plays, then
 * ev00e2_tornado_texlist and its texture names. the materials and strips
 * after them are a model's in ev00e2_tornado_object's tree */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00e2/dat_0cba35d8.h" /* NOLINT(misc-include-cleaner) */
#include "ev00e2/tornado_textures.h"

#define TEXT_1_LENGTH 14
#define TEXT_2_LENGTH 14
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 14
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 14
#define TEXT_7_LENGTH 13
#define TEXT_8_LENGTH 12
#define TEXT_9_LENGTH 14
#define TEXT_10_LENGTH 14
#define TEXT_11_LENGTH 13
#define TEXT_12_LENGTH 14
#define TEXT_13_LENGTH 14
#define TEXT_14_LENGTH 14
#define TEXT_15_LENGTH 14
#define TEXT_16_LENGTH 14
#define TEXT_17_LENGTH 14
#define TEXT_18_LENGTH 14
#define TEXT_19_LENGTH 13
#define TEXT_20_LENGTH 13
#define TEXT_21_LENGTH 13
#define TEXT_22_LENGTH 12
#define TEXT_23_LENGTH 12
#define TEXT_24_LENGTH 16
#define TEXT_25_LENGTH 16
#define TEXT_26_LENGTH 14
#define TEXT_27_LENGTH 15
#define TEXT_28_LENGTH 15
#include "ev00e2/tornado_textures/text.inc"

extern NJS_MDATA2 dat_0cbaee48[];

static NJS_MOTION dat_0cbaef38 = {
#include "ev00e2/tornado_textures/dat_0cbaef38.inc"
};
static NJS_ACTION dat_0cbaef44 = {
#include "ev00e2/tornado_textures/dat_0cbaef44.inc"
};
static NJS_TEXNAME ev00e2_tornado_textures[28] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
};
NJS_TEXLIST ev00e2_tornado_texlist = {
#include "ev00e2/tornado_textures/ev00e2_tornado_texlist.inc"
};
NJS_MATERIAL dat_0cbaf0a4[2] = {
#include "ev00e2/tornado_textures/dat_0cbaf0a4.inc"
};
Sint16 dat_0cbaf0cc[12] = {
#include "ev00e2/tornado_textures/dat_0cbaf0cc.inc"
};
Sint16 dat_0cbaf0e4[8] = {
#include "ev00e2/tornado_textures/dat_0cbaf0e4.inc"
};
Sint16 dat_0cbaf0f4[30] = {
#include "ev00e2/tornado_textures/dat_0cbaf0f4.inc"
};
