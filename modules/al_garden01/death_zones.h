/* the zones in which a player dies in the garden */
#ifndef AL_GARDEN01_DEATH_ZONES_H
#define AL_GARDEN01_DEATH_ZONES_H

#include "player/player.h"

/* the garden's list of zones for each act. 1ST_READ.BIN's death_zones
 * reaches this table by its address, 0x0cb9032c */
extern struct death_zone *egg_carrier_garden_death_zones[];

#endif
