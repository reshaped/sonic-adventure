/* the zones in which a player dies, for each act, and the object of act 2's
 * last zone */
#include <ninja.h>

#include "player/player.h"
#include "stg08/death_zones.h"

/* the zones' objects but the last, and the last one's model */
extern NJS_OBJECT dat_0ca50de8;
extern NJS_OBJECT dat_0ca50edc;
extern NJS_OBJECT dat_0ca50fb4;
extern NJS_OBJECT dat_0ca510a8;
extern NJS_MODEL dat_0ca51158;

/* the object of act 2's last zone */
static NJS_OBJECT dat_0ca51180 = {
#include "stg08/death_zones/dat_0ca51180.inc"
};

/* the zones of acts 1 and 3, and act 2's. each list ends with a null
 * object */
static struct death_zone icecap_act1_death_zones[2] = {
#include "stg08/death_zones/icecap_act1_death_zones.inc"
};
static struct death_zone icecap_act2_death_zones[5] = {
#include "stg08/death_zones/icecap_act2_death_zones.inc"
};
/* by act: death_zone_step and knuckles.c read the entry for the act in
 * hand, and the stage has four; act 0 has no zones */
struct death_zone *icecap_death_zones[4] = {
    NULL,
    icecap_act1_death_zones,
    icecap_act2_death_zones,
    icecap_act1_death_zones,
};
