/* the patrol cars' action and its motion, then station3, the texture name
 * of the picture over the screen. its texlist, ev0001_picture_texlist,
 * starts the data after this file. the event loads it from ICM0001_3 */
#include <ninja.h>

#include "ev0001/patrol_car_action.h"

#define TEXT_1_LENGTH 8
#include "ev0001/patrol_car_action/text.inc"

extern NJS_OBJECT dat_0cb9377c;
extern NJS_MDATA2 dat_0cb939f0[];

static NJS_MOTION ev0001_patrol_car_motion = {
#include "ev0001/patrol_car_action/ev0001_patrol_car_motion.inc"
};
NJS_ACTION ev0001_patrol_car_action = {
#include "ev0001/patrol_car_action/ev0001_patrol_car_action.inc"
};
static NJS_TEXNAME ev0001_picture_textures[1] = {
    {TEXT_1, 0, 0},
};
