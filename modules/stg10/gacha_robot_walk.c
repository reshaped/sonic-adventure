/* the E GACHA robot's walk, and an action that pairs it with the robot */
#include <ninja.h>

#include "stg10/gacha_robot.h" /* NOLINT(misc-include-cleaner) */
#include "stg10/gacha_robot_walk.h"

extern NJS_MDATA2 dat_0cb03840[];

NJS_MOTION gacha_robot_walk_motion = {
#include "stg10/gacha_robot_walk/gacha_robot_walk_motion.inc"
};
/* gacha_robot_walk_motion on gacha_robot_object. no word in the image or in
 * 1ST_READ.BIN points at it */
static NJS_ACTION gacha_robot_walk_action = {
#include "stg10/gacha_robot_walk/gacha_robot_walk_action.inc"
};
