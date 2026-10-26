/* the zones in which a player dies, for each act */
#include <ninja.h>

#include "player/player.h"
#include "stg06/death_zones.h"

/* each zone's model */
extern NJS_OBJECT dat_0c91f5fc;
extern NJS_OBJECT dat_0c91f6ec;
extern NJS_OBJECT dat_0c91f7dc;
extern NJS_OBJECT dat_0c91f8cc;

/* each act's zones, ended by a null object */
static struct death_zone sky_deck_act0_death_zones[2] = {
#include "stg06/death_zones/sky_deck_act0_death_zones.inc"
};
static struct death_zone sky_deck_act1_death_zones[2] = {
#include "stg06/death_zones/sky_deck_act1_death_zones.inc"
};
static struct death_zone sky_deck_act2_death_zones[3] = {
#include "stg06/death_zones/sky_deck_act2_death_zones.inc"
};
/* the lists by act, one for each of the stage's three acts. 1ST_READ.BIN's
 * death_zones names the array. death_zone_step and knuckles_outside_zones read
 * the current act's list */
struct death_zone *sky_deck_death_zones[3] = {
    sky_deck_act0_death_zones,
    sky_deck_act1_death_zones,
    sky_deck_act2_death_zones,
};
