/* the zones in which a player dies, for each act */
#include <ninja.h>

#include "player/player.h"
#include "stg01/death_zones.h"

/* each zone's model */
extern NJS_OBJECT dat_0cab0ffc;
extern NJS_OBJECT dat_0cab10ec;
extern NJS_OBJECT dat_0cab11dc;
extern NJS_OBJECT dat_0cab12cc;

/* each act's zones, ended by a null object */
static struct death_zone beach_act0_death_zones[2] = {
#include "stg01/death_zones/beach_act0_death_zones.inc"
};
static struct death_zone beach_act1_death_zones[3] = {
#include "stg01/death_zones/beach_act1_death_zones.inc"
};
static struct death_zone beach_act2_death_zones[2] = {
#include "stg01/death_zones/beach_act2_death_zones.inc"
};
struct death_zone *beach_death_zones[3] = {
    beach_act0_death_zones,
    beach_act1_death_zones,
    beach_act2_death_zones,
};
