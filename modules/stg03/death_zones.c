/* the zones in which a player dies, for each act */
#include <ninja.h>

#include "player/player.h"
#include "stg03/death_zones.h"

/* each zone's model */
extern NJS_OBJECT dat_0c9c3604;
extern NJS_OBJECT dat_0c9c36f4;
extern NJS_OBJECT dat_0c9c381c;
extern NJS_OBJECT dat_0c9c390c;
extern NJS_OBJECT dat_0c9c39fc;
extern NJS_OBJECT dat_0c9c3aec;

/* each act's zones, ended by a null object */
static struct death_zone twinkle_act0_death_zones[5] = {
#include "stg03/death_zones/twinkle_act0_death_zones.inc"
};
static struct death_zone twinkle_act1_death_zones[2] = {
#include "stg03/death_zones/twinkle_act1_death_zones.inc"
};
static struct death_zone twinkle_act2_death_zones[2] = {
#include "stg03/death_zones/twinkle_act2_death_zones.inc"
};
/* the lists by act, one for each of the stage's three acts. 1ST_READ.BIN's
 * death_zones names the array. death_zone_step and knuckles_outside_zones
 * read the current act's list */
struct death_zone *twinkle_death_zones[] = {
    twinkle_act0_death_zones,
    twinkle_act1_death_zones,
    twinkle_act2_death_zones,
#ifdef NONMATCHING
    /* keeps the value death_zone_step's tag names: in act 3 it reads this
     * null fourth entry, where the image reads a fill word of 0 */
    NULL,
#endif
};
