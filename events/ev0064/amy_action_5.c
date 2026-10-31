/* Amy's ev0064_amy_action_5 and its motion, then the bird's texlist and its
 * texture names. the last item, dat_0cb91b74, is the material of the first
 * model in the data after this object. that model is a part of the bird that
 * ev0064_bird_action moves */
#include <ninja.h>

#include "ev0064/amy_action_5.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 7
#define TEXT_12_LENGTH 9
#include "ev0064/amy_action_5/text.inc"

extern NJS_MDATA2 dat_0cb915e8[];
extern NJS_OBJECT amy_object;

static NJS_MOTION ev0064_amy_motion_5 = {
#include "ev0064/amy_action_5/ev0064_amy_motion_5.inc"
};
NJS_ACTION ev0064_amy_action_5 = {
#include "ev0064/amy_action_5/ev0064_amy_action_5.inc"
};
static NJS_TEXNAME ev0064_bird_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0064_bird_texlist = {
#include "ev0064/amy_action_5/ev0064_bird_texlist.inc"
};
NJS_MATERIAL dat_0cb91b74[1] = {
#include "ev0064/amy_action_5/dat_0cb91b74.inc"
};
