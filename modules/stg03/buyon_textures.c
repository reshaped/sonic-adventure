/* the buyon's texture names and texlist, and its first model's materials */
#include <ninja.h>

#include "stg03/buyon_textures.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 4
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 4
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 5
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 6
#define TEXT_12_LENGTH 6
#define TEXT_13_LENGTH 6
#define TEXT_14_LENGTH 5
#define TEXT_15_LENGTH 4
#define TEXT_16_LENGTH 7
#define TEXT_17_LENGTH 5
#define TEXT_18_LENGTH 5
#define TEXT_19_LENGTH 5
#define TEXT_20_LENGTH 5
#define TEXT_21_LENGTH 3
#include "stg03/buyon_textures/text.inc"

static NJS_TEXNAME buyon_textures[21] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0},
};
NJS_TEXLIST buyon_texlist = {
#include "stg03/buyon_textures/buyon_texlist.inc"
};
/* the buyon's first model's materials. each file's data starts on a
 * multiple of 8. the materials start 4 bytes past one, right after the
 * texlist, so they are this file's. the model's strips after them start on
 * a multiple of 8, so they may start another file */
NJS_MATERIAL dat_0c9c3eb4[3] = {
#include "stg03/buyon_textures/dat_0c9c3eb4.inc"
};
