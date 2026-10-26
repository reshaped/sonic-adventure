/* the zones in which a player dies */
#ifndef STG04_DEATH_ZONES_H
#define STG04_DEATH_ZONES_H

#include "player/player.h"

/* the stage's zone lists, an array by act, at which 1ST_READ.BIN's
 * death_zones points. death_zone_step and knuckles.c's knuckles_outside_zones
 * index the array by the current act, unchecked */
extern struct death_zone *highway_death_zones[];

#endif
