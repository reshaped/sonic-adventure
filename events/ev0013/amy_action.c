/* the action Amy repeats as she and Sonic walk at the event's start, and
 * its motion. the bird's 12 texture names and texlist, ev0013_bird_texlist,
 * follow. the event loads the texlist from VER2_WING. the file ends on the
 * material of a node of the bird's model. the node's other parts start the
 * data after this file */
#include <ninja.h>

#include "ev0013/amy_action.h"

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
#include "ev0013/amy_action/text.inc"

extern NJS_MDATA2 dat_0cb8dfc0[];
extern NJS_OBJECT amy_object;

static NJS_MOTION ev0013_amy_motion = {
#include "ev0013/amy_action/ev0013_amy_motion.inc"
};
NJS_ACTION ev0013_amy_action = {
#include "ev0013/amy_action/ev0013_amy_action.inc"
};
static NJS_TEXNAME ev0013_bird_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
NJS_TEXLIST ev0013_bird_texlist = {
#include "ev0013/amy_action/ev0013_bird_texlist.inc"
};
static NJS_MATERIAL dat_0cb8e54c[1] = {
#include "ev0013/amy_action/dat_0cb8e54c.inc"
};
