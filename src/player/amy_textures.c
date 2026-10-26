/* Amy's textures, from AMY, and her effects', from AMY_EFF. the materials
 * and first meshes of the model data after them belong to this object too.
 * R32, the SHC release that compiles this file, starts each object's data on
 * an 8-byte boundary. each of those materials and meshes starts 4 bytes
 * past one, right after the datum before it */
#include <ninja.h>
#include <sg_xpt.h>

#include "player/textures.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 12
#define TEXT_9_LENGTH 12
#define TEXT_10_LENGTH 14
#define TEXT_11_LENGTH 14
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 6
#define TEXT_14_LENGTH 7
#define TEXT_15_LENGTH 10
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 13
#define TEXT_22_LENGTH 8
#define TEXT_23_LENGTH 10
#define TEXT_24_LENGTH 10
#define TEXT_25_LENGTH 9
#define TEXT_26_LENGTH 10
#define TEXT_27_LENGTH 9
#define TEXT_28_LENGTH 9
#define TEXT_29_LENGTH 9
#define TEXT_30_LENGTH 9
#define TEXT_31_LENGTH 9
#define TEXT_32_LENGTH 13
#define TEXT_33_LENGTH 9
#define TEXT_34_LENGTH 9
#define TEXT_35_LENGTH 8
#define TEXT_36_LENGTH 8
#define TEXT_37_LENGTH 10
#define TEXT_38_LENGTH 10
#define TEXT_39_LENGTH 11
#define TEXT_40_LENGTH 11
#define TEXT_41_LENGTH 11
#define TEXT_42_LENGTH 10
#define TEXT_43_LENGTH 10
#define TEXT_44_LENGTH 10
#define TEXT_45_LENGTH 9
#define TEXT_46_LENGTH 9
#define TEXT_47_LENGTH 10
#define TEXT_48_LENGTH 8
#define TEXT_49_LENGTH 9
#define TEXT_50_LENGTH 9
#define TEXT_51_LENGTH 11
#define TEXT_52_LENGTH 9
#define TEXT_53_LENGTH 7
#define TEXT_54_LENGTH 7
#define TEXT_55_LENGTH 8
#define TEXT_56_LENGTH 8
#define TEXT_57_LENGTH 8
#include "player/amy_textures/text.inc"

static NJS_TEXNAME amy_textures[53] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0},
    {TEXT_49, 0, 0}, {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0},
    {TEXT_53, 0, 0},
};
NJS_TEXLIST amy_texlist = {
#include "player/amy_textures/amy_texlist.inc"
};

static NJS_TEXNAME amy_eff_textures[4] = {
    {TEXT_54, 0, 0},
    {TEXT_55, 0, 0},
    {TEXT_56, 0, 0},
    {TEXT_57, 0, 0},
};
NJS_TEXLIST amy_eff_texlist = {
#include "player/amy_textures/amy_eff_texlist.inc"
};
static NJS_MATERIAL dat_8c4d5cec[10] = {
#include "player/amy_textures/dat_8c4d5cec.inc"
};
static Sint16 dat_8c4d5db4[24] = {
#include "player/amy_textures/dat_8c4d5db4.inc"
};
static Sint16 dat_8c4d5de4[72] = {
#include "player/amy_textures/dat_8c4d5de4.inc"
};
static Sint16 dat_8c4d5e74[22] = {
#include "player/amy_textures/dat_8c4d5e74.inc"
};
