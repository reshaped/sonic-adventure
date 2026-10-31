/* E-102's textures from E102, his beam's from E102BEAM and his effects' from
 * E102EFFECT, then a material that a model in the data after them names. the
 * material starts 4 bytes past an 8-byte boundary, straight after the texlist
 * before it. an object that SHC's R32 release compiles starts on an 8-byte
 * boundary, so the material belongs to this object */
#include <ninja.h>

#include "player/e102_textures.h"

#include "player/textures.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 8
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 6
#define TEXT_17_LENGTH 7
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 10
#define TEXT_22_LENGTH 7
#define TEXT_23_LENGTH 8
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 11
#define TEXT_26_LENGTH 9
#define TEXT_27_LENGTH 9
#define TEXT_28_LENGTH 11
#define TEXT_29_LENGTH 8
#define TEXT_30_LENGTH 9
#define TEXT_31_LENGTH 9
#define TEXT_32_LENGTH 9
#define TEXT_33_LENGTH 10
#define TEXT_34_LENGTH 10
#define TEXT_35_LENGTH 8
#define TEXT_36_LENGTH 6
#define TEXT_37_LENGTH 6
#define TEXT_38_LENGTH 6
#define TEXT_39_LENGTH 6
#define TEXT_40_LENGTH 6
#define TEXT_41_LENGTH 6
#define TEXT_42_LENGTH 6
#define TEXT_43_LENGTH 6
#define TEXT_44_LENGTH 4
#define TEXT_45_LENGTH 7
#define TEXT_46_LENGTH 5
#define TEXT_47_LENGTH 11
#define TEXT_48_LENGTH 12
#define TEXT_49_LENGTH 7
#define TEXT_50_LENGTH 5
#define TEXT_51_LENGTH 5
#define TEXT_52_LENGTH 5
#define TEXT_53_LENGTH 5
#define TEXT_54_LENGTH 5
#define TEXT_55_LENGTH 5
#define TEXT_56_LENGTH 5
#define TEXT_57_LENGTH 5
#define TEXT_58_LENGTH 7
#define TEXT_59_LENGTH 7
#define TEXT_60_LENGTH 7
#define TEXT_61_LENGTH 7
#include "player/e102_textures/text.inc"

static NJS_TEXNAME e102_textures[35] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0},
};
NJS_TEXLIST e102_texlist = {
#include "player/e102_textures/e102_texlist.inc"
};

NJS_TEXNAME e102beam_textures[E102BEAM_TEXTURES] = {
    {TEXT_36, 0, 0}, {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0},
    {TEXT_40, 0, 0}, {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0},
    {TEXT_44, 0, 0}, {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0},
    {TEXT_48, 0, 0}, {TEXT_49, 0, 0},
};
NJS_TEXLIST e102beam_texlist = {
#include "player/e102_textures/e102beam_texlist.inc"
};

static NJS_TEXNAME e102effect_textures[12] = {
    {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0}, {TEXT_53, 0, 0},
    {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0}, {TEXT_57, 0, 0},
    {TEXT_58, 0, 0}, {TEXT_59, 0, 0}, {TEXT_60, 0, 0}, {TEXT_61, 0, 0},
};
NJS_TEXLIST e102effect_texlist = {
#include "player/e102_textures/e102effect_texlist.inc"
};
NJS_MATERIAL dat_8c439d1c[1] = {
#include "player/e102_textures/dat_8c439d1c.inc"
};
