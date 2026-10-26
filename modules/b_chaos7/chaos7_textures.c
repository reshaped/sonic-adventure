/* Perfect Chaos's textures and the water's, with their texlists, and the
 * shape motion of his forming figure */
#include <ninja.h>

#include "b_chaos7/chaos7_textures.h"

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 13
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 10
#define TEXT_14_LENGTH 10
#define TEXT_15_LENGTH 4
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 12
#define TEXT_22_LENGTH 12
#define TEXT_23_LENGTH 12
#define TEXT_24_LENGTH 8
#define TEXT_25_LENGTH 8
#define TEXT_26_LENGTH 8
#define TEXT_27_LENGTH 12
#define TEXT_28_LENGTH 12
#define TEXT_29_LENGTH 12
#define TEXT_30_LENGTH 13
#define TEXT_31_LENGTH 13
#define TEXT_32_LENGTH 13
#define TEXT_33_LENGTH 8
#define TEXT_34_LENGTH 8
#define TEXT_35_LENGTH 8
#include "b_chaos7/chaos7_textures/text.inc"

extern NJS_MDATA2 dat_0ca5e690[];

NJS_MOTION form_shape = {
#include "b_chaos7/chaos7_textures/form_shape.inc"
};
static NJS_TEXNAME chaos7_textures[23] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0},
};
NJS_TEXLIST chaos7_texlist = {
#include "b_chaos7/chaos7_textures/chaos7_texlist.inc"
};
NJS_TEXNAME water_textures[16] = {
    {TEXT_4, 0, 0},  {TEXT_24, 0, 0}, {TEXT_25, 0, 0}, {TEXT_26, 0, 0},
    {TEXT_3, 0, 0},  {TEXT_27, 0, 0}, {TEXT_28, 0, 0}, {TEXT_29, 0, 0},
    {TEXT_10, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_11, 0, 0}, {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0},
};
NJS_TEXLIST chaos7_surface_texlist = {
#include "b_chaos7/chaos7_textures/chaos7_surface_texlist.inc"
};
