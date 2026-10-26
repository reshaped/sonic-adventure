/* the zones a player goes down in, listed for each act */
#ifndef STG07_DEATH_ZONES_H
#define STG07_DEATH_ZONES_H

#include "player/player.h"

/* the zone lists, one for each act. 1ST_READ.BIN's death_zones holds the
 * array's address for Lost World */
extern struct death_zone *ruin_death_zones[];

#endif
