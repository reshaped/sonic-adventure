/* ev006e_amy_action_3 and its motion, then the bird's texture names and
 * texlist, then a material of the bird's model. Amy's model lies in
 * 1ST_READ.BIN. the motion's NJS_MDATA2 table, dat_0cb8d8c0, lies in the
 * data before this object */
#include <ninja.h>

#include "ev006e/amy_action_3.h"

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
#include "ev006e/amy_action_3/text.inc"

extern NJS_MDATA2 dat_0cb8d8c0[];
extern NJS_OBJECT amy_object;

static NJS_MOTION ev006e_amy_motion_3 = {
#include "ev006e/amy_action_3/ev006e_amy_motion_3.inc"
};
NJS_ACTION ev006e_amy_action_3 = {
#include "ev006e/amy_action_3/ev006e_amy_action_3.inc"
};
static NJS_TEXNAME ev006e_bird_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev006e_bird_texlist = {
#include "ev006e/amy_action_3/ev006e_bird_texlist.inc"
};
/* the one material of node 0x0cb8df1c of dat_0cb91ecc, the model
 * ev006e_bird_action moves. the rest of that model lies in the data after
 * this object */
static NJS_MATERIAL dat_0cb8de4c[1] = {
#include "ev006e/amy_action_3/dat_0cb8de4c.inc"
};
