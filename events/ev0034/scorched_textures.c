/* ev0034_scorched_texlist and its 27 texture names. the material belongs to
 * a node of ev0034_scorched_object, the scorched Egg Mobile's model */
#include <ninja.h>

#include "ev0034/scorched_textures.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 7
#define TEXT_13_LENGTH 7
#define TEXT_14_LENGTH 7
#define TEXT_15_LENGTH 5
#define TEXT_16_LENGTH 6
#define TEXT_17_LENGTH 7
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 8
#define TEXT_22_LENGTH 6
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 9
#define TEXT_25_LENGTH 7
#define TEXT_26_LENGTH 6
#define TEXT_27_LENGTH 7
#include "ev0034/scorched_textures/text.inc"

static NJS_TEXNAME ev0034_scorched_textures[27] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0},
};
NJS_TEXLIST ev0034_scorched_texlist = {
#include "ev0034/scorched_textures/ev0034_scorched_texlist.inc"
};
NJS_MATERIAL dat_0cbdb174[1] = {
#include "ev0034/scorched_textures/dat_0cbdb174.inc"
};
