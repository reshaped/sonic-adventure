/* the zones in which a player dies */
#ifndef STG01_DEATH_ZONES_H
#define STG01_DEATH_ZONES_H

#include "player/player.h"

/* the stage's lists by act, one for each of its three acts, which
 * 1ST_READ.BIN's death_zones names. each list ends with a null object.
 * death_zone_step and knuckles_outside_zones read the current act's list */
extern struct death_zone *beach_death_zones[];

#endif
