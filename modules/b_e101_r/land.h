/* the land on the Egg Carrier's deck, which B_ROBO links too */
#ifndef B_E101_R_LAND_H
#define B_E101_R_LAND_H

#include "collision/world.h"

/* 1ST_READ.BIN's world_lands points at this table for this stage and for
 * B_ROBO's. it reaches the table by its address, 0x0c900188 here and
 * 0x0c900118 in B_ROBO, not by its name */
extern struct land_table e101_r_land;

#endif
