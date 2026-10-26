/* the stage's land */
#ifndef B_E101_LAND_H
#define B_E101_LAND_H

#include "collision/world.h"

/* 1ST_READ.BIN's world_lands points at this table for the stage. it reaches
 * the table by its address, 0x0c900070, not by its name */
extern struct land_table e101_land;

#endif
