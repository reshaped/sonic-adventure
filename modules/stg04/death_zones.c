/* the zones in which a player dies, for each act */
#include <ninja.h>

#include "player/player.h"
#include "stg04/death_zones.h"

/* each zone's model */
extern NJS_OBJECT dat_0ca339b0;
extern NJS_OBJECT dat_0ca33aa4;
extern NJS_OBJECT dat_0ca33b98;
extern NJS_OBJECT dat_0ca33c70;
extern NJS_OBJECT dat_0ca33d48;
extern NJS_OBJECT dat_0ca33e3c;
extern NJS_OBJECT dat_0ca33f8c;

/* each act's zones, ended by a null object */
static struct death_zone highway_act0_death_zones[6] = {
#include "stg04/death_zones/highway_act0_death_zones.inc"
};
static struct death_zone highway_act1_death_zones[2] = {
#include "stg04/death_zones/highway_act1_death_zones.inc"
};
static struct death_zone highway_act2_death_zones[2] = {
#include "stg04/death_zones/highway_act2_death_zones.inc"
};
/* one list for each of the stage's three acts */
struct death_zone *highway_death_zones[3] = {
    highway_act0_death_zones,
    highway_act1_death_zones,
    highway_act2_death_zones,
};
