/* Pachacamac's 21 texture names and his texlist, then one node of his
 * model, ev0085_pachacamac_object. the model's other nodes lie in the data
 * after this file */
#include <ninja.h>

#include "ev0085/pachacamac_textures.h"

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 6
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 6
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 6
#define TEXT_12_LENGTH 6
#define TEXT_13_LENGTH 6
#define TEXT_14_LENGTH 6
#define TEXT_15_LENGTH 6
#define TEXT_16_LENGTH 6
#define TEXT_17_LENGTH 6
#define TEXT_18_LENGTH 6
#define TEXT_19_LENGTH 6
#define TEXT_20_LENGTH 6
#define TEXT_21_LENGTH 6
#include "ev0085/pachacamac_textures/text.inc"

static NJS_TEXNAME ev0085_pachacamac_textures[21] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0},
};
NJS_TEXLIST ev0085_pachacamac_texlist = {
#include "ev0085/pachacamac_textures/ev0085_pachacamac_texlist.inc"
};
static NJS_OBJECT dat_0cb80fa4 = {
#include "ev0085/pachacamac_textures/dat_0cb80fa4.inc"
};
