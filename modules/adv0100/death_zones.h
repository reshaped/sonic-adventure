/* the zones in which a player dies on the Egg Carrier's outside */
#ifndef ADV0100_DEATH_ZONES_H
#define ADV0100_DEATH_ZONES_H

#include "player/player.h"

/* a list of zones for each of acts 0 to 6. 1ST_READ.BIN's death_zones names
 * the table for the stage. death_zone_step and knuckles.c read the list of
 * the act in hand */
extern struct death_zone *outside_death_zones[];

#endif
