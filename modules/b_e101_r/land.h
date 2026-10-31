/* the land on the Egg Carrier's deck, which B_ROBO links too */
#ifndef B_E101_R_LAND_H
#define B_E101_R_LAND_H

#include "collision/world.h"

/* 1ST_READ.BIN's world_lands points at this table for this stage and for
 * B_ROBO's. each image links its own copy: world_lands names B_E101_R's
 * e101_r_land and B_ROBO's eggmanrobo_land */
extern struct land_table e101_r_land;

#endif
