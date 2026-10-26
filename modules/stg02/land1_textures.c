/* the names of act 1's land's textures, and its texlist */
#include <ninja.h>

#include "stg02/land1_textures.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 8
#include "stg02/land1_textures/text.inc"

static NJS_TEXNAME windy_land1_textures[17] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0},
};
NJS_TEXLIST windy_land1_texlist = {
#include "stg02/land1_textures/windy_land1_texlist.inc"
};
/* the materials of the first model after them, dat_0c94a138. SHC starts
 * each object's data on a multiple of 8. the materials start right after the
 * texlist, 4 bytes past one, so they are this file's */
static NJS_MATERIAL dat_0c948fac[7] = {
#include "stg02/land1_textures/dat_0c948fac.inc"
};
