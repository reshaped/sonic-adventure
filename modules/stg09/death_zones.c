/* the zones in which a player dies, their models, and each act's list of
 * them */
#include <ninja.h>
#include <sg_xpt.h>

#include "player/player.h"
#include "stg09/death_zones.h"

static NJS_MATERIAL dat_0cb172e8[1] = {
#include "stg09/death_zones/dat_0cb172e8.inc"
};
static Sint16 dat_0cb172fc[16] = {
#include "stg09/death_zones/dat_0cb172fc.inc"
};
static NJS_MESHSET dat_0cb1731c[1] = {
#include "stg09/death_zones/dat_0cb1731c.inc"
};
static NJS_POINT3 dat_0cb17334[8] = {
#include "stg09/death_zones/dat_0cb17334.inc"
};
static NJS_VECTOR dat_0cb17394[8] = {
#include "stg09/death_zones/dat_0cb17394.inc"
};
static NJS_MODEL dat_0cb173f4 = {
#include "stg09/death_zones/dat_0cb173f4.inc"
};
static NJS_OBJECT casino_act0_death_object = {
#include "stg09/death_zones/casino_act0_death_object.inc"
};
static NJS_MATERIAL dat_0cb17450[1] = {
#include "stg09/death_zones/dat_0cb17450.inc"
};
static Sint16 dat_0cb17464[6] = {
#include "stg09/death_zones/dat_0cb17464.inc"
};
static NJS_MESHSET dat_0cb17470[1] = {
#include "stg09/death_zones/dat_0cb17470.inc"
};
static NJS_POINT3 dat_0cb17488[4] = {
#include "stg09/death_zones/dat_0cb17488.inc"
};
static NJS_VECTOR dat_0cb174b8[4] = {
#include "stg09/death_zones/dat_0cb174b8.inc"
};
static NJS_MODEL dat_0cb174e8 = {
#include "stg09/death_zones/dat_0cb174e8.inc"
};
static NJS_OBJECT casino_act1_death_object = {
#include "stg09/death_zones/casino_act1_death_object.inc"
};
/* each act's zones, ended by a null object */
static struct death_zone casino_act0_death_zones[2] = {
#include "stg09/death_zones/casino_act0_death_zones.inc"
};
static struct death_zone casino_act1_death_zones[2] = {
#include "stg09/death_zones/casino_act1_death_zones.inc"
};
/* each act's list of zones. acts 2 and 3 have none */
struct death_zone *casino_death_zones[4] = {
    casino_act0_death_zones,
    casino_act1_death_zones,
    NULL,
    NULL,
};
