/* the zones in which a player dies, for each act */
#include <ninja.h>

#include "player/player.h"
#include "stg02/death_zones.h"

/* each zone's model */
extern NJS_OBJECT dat_0ca1402c;
extern NJS_OBJECT dat_0ca1411c;
extern NJS_OBJECT dat_0ca14244;
extern NJS_OBJECT dat_0ca14334;
extern NJS_OBJECT dat_0ca1445c;
extern NJS_OBJECT dat_0ca14584;

/* each act's zones, ended by a null object */
static struct death_zone windy_act0_death_zones[2] = {
#include "stg02/death_zones/windy_act0_death_zones.inc"
};
static struct death_zone windy_act1_death_zones[2] = {
#include "stg02/death_zones/windy_act1_death_zones.inc"
};
static struct death_zone windy_act2_death_zones[5] = {
#include "stg02/death_zones/windy_act2_death_zones.inc"
};
struct death_zone *windy_death_zones[3] = {
    windy_act0_death_zones,
    windy_act1_death_zones,
    windy_act2_death_zones,
};
