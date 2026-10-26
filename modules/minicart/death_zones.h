/* the zones in which a player dies */
#ifndef MINICART_DEATH_ZONES_H
#define MINICART_DEATH_ZONES_H

#include "player/player.h"

/* the stage's lists by act, which 1ST_READ.BIN's death_zones names. each
 * list ends with a null object */
extern struct death_zone *cart_death_zones[];

#endif
