/* the zones a player goes down in, listed for each act */
#include <ninja.h>

#include "player/player.h"
#include "stg07/death_zones.h"

/* the objects that shape the zones */
extern NJS_OBJECT dat_0c90c79c;
extern NJS_OBJECT dat_0c90c88c;
extern NJS_OBJECT dat_0c90c97c;
extern NJS_OBJECT dat_0c90ca6c;
extern NJS_OBJECT dat_0c90cb5c;
extern NJS_OBJECT dat_0c90cc84;
extern NJS_OBJECT dat_0c90cd74;

/* each act's zones, ended by a null object */
static struct death_zone ruin_act0_death_zones[4] = {
#include "stg07/death_zones/ruin_act0_death_zones.inc"
};
static struct death_zone ruin_act1_death_zones[4] = {
#include "stg07/death_zones/ruin_act1_death_zones.inc"
};
static struct death_zone ruin_act2_death_zones[2] = {
#include "stg07/death_zones/ruin_act2_death_zones.inc"
};
/* a list for each of the stage's three acts */
struct death_zone *ruin_death_zones[3] = {
    ruin_act0_death_zones,
    ruin_act1_death_zones,
    ruin_act2_death_zones,
};
