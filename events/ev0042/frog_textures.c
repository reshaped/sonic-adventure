/* an action on Tails' model and its motion, which nothing plays, then
 * ev0042_frog_texlist and its texture names. the file ends on the material
 * of one node of the frog's model */
#include <ninja.h>

#include "ev0042/frog_textures.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#include "ev0042/frog_textures/text.inc"

extern NJS_MDATA2 dat_0cb92910[];
extern NJS_OBJECT tails_object;

static NJS_MOTION dat_0cb92d50 = {
#include "ev0042/frog_textures/dat_0cb92d50.inc"
};
static NJS_ACTION dat_0cb92d5c = {
#include "ev0042/frog_textures/dat_0cb92d5c.inc"
};
static NJS_TEXNAME ev0042_frog_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
NJS_TEXLIST ev0042_frog_texlist = {
#include "ev0042/frog_textures/ev0042_frog_texlist.inc"
};
NJS_MATERIAL dat_0cb92dcc[1] = {
#include "ev0042/frog_textures/dat_0cb92dcc.inc"
};
