/* EFF_SANDBOARD's 24 textures and eff_sandboard_texlist, which the stage's
 * pvm list loads */
#include <ninja.h>

#include "sboard/sand_textures.h"

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
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 8
#define TEXT_22_LENGTH 8
#define TEXT_23_LENGTH 8
#define TEXT_24_LENGTH 8
#include "sboard/sand_textures/text.inc"

static NJS_TEXNAME eff_sandboard_textures[24] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
};
NJS_TEXLIST eff_sandboard_texlist = {
#include "sboard/sand_textures/eff_sandboard_texlist.inc"
};
