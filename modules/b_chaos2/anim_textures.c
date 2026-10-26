/* two lists of texture names, of five and of twelve, which dat_0c908064
 * and dat_0c9080fc list. each of dat_0c908104's twelve texlists lists one
 * of the twelve. nothing in the image names the texlists */
#include <ninja.h>

#include "b_chaos2/anim_textures.h"

#define TEXT_1_LENGTH 14
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 14
#define TEXT_6_LENGTH 14
#define TEXT_7_LENGTH 14
#define TEXT_8_LENGTH 14
#define TEXT_9_LENGTH 14
#define TEXT_10_LENGTH 14
#define TEXT_11_LENGTH 14
#define TEXT_12_LENGTH 14
#define TEXT_13_LENGTH 14
#define TEXT_14_LENGTH 14
#define TEXT_15_LENGTH 14
#define TEXT_16_LENGTH 14
#define TEXT_17_LENGTH 14
#include "b_chaos2/anim_textures/text.inc"

static NJS_TEXNAME dat_0c908028[5] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0},
};
NJS_TEXLIST dat_0c908064 = {
#include "b_chaos2/anim_textures/dat_0c908064.inc"
};
static NJS_TEXNAME dat_0c90806c[12] = {
    {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},  {TEXT_9, 0, 0},
    {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0},
    {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0}, {TEXT_17, 0, 0},
};
NJS_TEXLIST dat_0c9080fc = {
#include "b_chaos2/anim_textures/dat_0c9080fc.inc"
};
NJS_TEXLIST dat_0c908104[12] = {
#include "b_chaos2/anim_textures/dat_0c908104.inc"
};
