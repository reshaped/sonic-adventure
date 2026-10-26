/* the zones a player goes down in, the objects that shape them, and each
 * act's list of them */
#ifndef STG12_DEATH_ZONES_H
#define STG12_DEATH_ZONES_H
#include "player/player.h"

/* death_zones, in 1ST_READ.BIN, points at these lists for Hot Shelter, one
 * for each act */
extern struct death_zone *shelter_death_zones[];

#endif
