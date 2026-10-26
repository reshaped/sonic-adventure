/* the names of the race hall land's 23 textures, its texlist, and the
 * first materials of the model that follows them */
#include <ninja.h>

#include "al_race/act0_textures.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 7
#define TEXT_12_LENGTH 7
#define TEXT_13_LENGTH 7
#define TEXT_14_LENGTH 7
#define TEXT_15_LENGTH 7
#define TEXT_16_LENGTH 7
#define TEXT_17_LENGTH 7
#define TEXT_18_LENGTH 7
#define TEXT_19_LENGTH 7
#define TEXT_20_LENGTH 7
#define TEXT_21_LENGTH 7
#define TEXT_22_LENGTH 11
#define TEXT_23_LENGTH 7
#include "al_race/act0_textures/text.inc"

static NJS_TEXNAME dat_0cb9d248[23] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0},
};
NJS_TEXLIST race_act0_land_texlist = {
#include "al_race/act0_textures/race_act0_land_texlist.inc"
};
NJS_MATERIAL dat_0cb9d364[3] = {
#include "al_race/act0_textures/dat_0cb9d364.inc"
};
