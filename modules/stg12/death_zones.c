/* the zones a player goes down in, the objects that shape them, and each
 * act's list of them */
#include <ninja.h>
#include <sg_xpt.h>

#include "player/player.h"
#include "stg12/death_zones.h"

static NJS_MATERIAL dat_0ca906f8[1] = {
#include "stg12/death_zones/dat_0ca906f8.inc"
};
static Sint16 dat_0ca9070c[6] = {
#include "stg12/death_zones/dat_0ca9070c.inc"
};
static NJS_MESHSET dat_0ca90718[1] = {
#include "stg12/death_zones/dat_0ca90718.inc"
};
static NJS_POINT3 dat_0ca90730[4] = {
#include "stg12/death_zones/dat_0ca90730.inc"
};
static NJS_VECTOR dat_0ca90760[4] = {
#include "stg12/death_zones/dat_0ca90760.inc"
};
static NJS_MODEL dat_0ca90790 = {
#include "stg12/death_zones/dat_0ca90790.inc"
};
static NJS_OBJECT dat_0ca907b8 = {
#include "stg12/death_zones/dat_0ca907b8.inc"
};
static NJS_MATERIAL dat_0ca907ec[1] = {
#include "stg12/death_zones/dat_0ca907ec.inc"
};
static Sint16 dat_0ca90800[14] = {
#include "stg12/death_zones/dat_0ca90800.inc"
};
static NJS_MESHSET dat_0ca9081c[1] = {
#include "stg12/death_zones/dat_0ca9081c.inc"
};
static NJS_POINT3 dat_0ca90834[9] = {
#include "stg12/death_zones/dat_0ca90834.inc"
};
static NJS_VECTOR dat_0ca908a0[9] = {
#include "stg12/death_zones/dat_0ca908a0.inc"
};
static NJS_MODEL dat_0ca9090c = {
#include "stg12/death_zones/dat_0ca9090c.inc"
};
static NJS_OBJECT dat_0ca90934 = {
#include "stg12/death_zones/dat_0ca90934.inc"
};
static NJS_MATERIAL dat_0ca90968[1] = {
#include "stg12/death_zones/dat_0ca90968.inc"
};
static Sint16 dat_0ca9097c[8] = {
#include "stg12/death_zones/dat_0ca9097c.inc"
};
static NJS_MESHSET dat_0ca9098c[1] = {
#include "stg12/death_zones/dat_0ca9098c.inc"
};
static NJS_POINT3 dat_0ca909a4[6] = {
#include "stg12/death_zones/dat_0ca909a4.inc"
};
static NJS_VECTOR dat_0ca909ec[6] = {
#include "stg12/death_zones/dat_0ca909ec.inc"
};
static NJS_MODEL dat_0ca90a34 = {
#include "stg12/death_zones/dat_0ca90a34.inc"
};
static NJS_OBJECT dat_0ca90a5c = {
#include "stg12/death_zones/dat_0ca90a5c.inc"
};
static NJS_MATERIAL dat_0ca90a90[1] = {
#include "stg12/death_zones/dat_0ca90a90.inc"
};
static Sint16 dat_0ca90aa4[14] = {
#include "stg12/death_zones/dat_0ca90aa4.inc"
};
static NJS_MESHSET dat_0ca90ac0[1] = {
#include "stg12/death_zones/dat_0ca90ac0.inc"
};
static NJS_POINT3 dat_0ca90ad8[9] = {
#include "stg12/death_zones/dat_0ca90ad8.inc"
};
static NJS_VECTOR dat_0ca90b44[9] = {
#include "stg12/death_zones/dat_0ca90b44.inc"
};
static NJS_MODEL dat_0ca90bb0 = {
#include "stg12/death_zones/dat_0ca90bb0.inc"
};
static NJS_OBJECT dat_0ca90bd8 = {
#include "stg12/death_zones/dat_0ca90bd8.inc"
};
static NJS_MATERIAL dat_0ca90c0c[1] = {
#include "stg12/death_zones/dat_0ca90c0c.inc"
};
static Sint16 dat_0ca90c20[14] = {
#include "stg12/death_zones/dat_0ca90c20.inc"
};
static NJS_MESHSET dat_0ca90c3c[1] = {
#include "stg12/death_zones/dat_0ca90c3c.inc"
};
static NJS_POINT3 dat_0ca90c54[9] = {
#include "stg12/death_zones/dat_0ca90c54.inc"
};
static NJS_VECTOR dat_0ca90cc0[9] = {
#include "stg12/death_zones/dat_0ca90cc0.inc"
};
static NJS_MODEL dat_0ca90d2c = {
#include "stg12/death_zones/dat_0ca90d2c.inc"
};
static NJS_OBJECT dat_0ca90d54 = {
#include "stg12/death_zones/dat_0ca90d54.inc"
};
static NJS_MATERIAL dat_0ca90d88[1] = {
#include "stg12/death_zones/dat_0ca90d88.inc"
};
static Sint16 dat_0ca90d9c[14] = {
#include "stg12/death_zones/dat_0ca90d9c.inc"
};
static NJS_MESHSET dat_0ca90db8[1] = {
#include "stg12/death_zones/dat_0ca90db8.inc"
};
static NJS_POINT3 dat_0ca90dd0[9] = {
#include "stg12/death_zones/dat_0ca90dd0.inc"
};
static NJS_VECTOR dat_0ca90e3c[9] = {
#include "stg12/death_zones/dat_0ca90e3c.inc"
};
static NJS_MODEL dat_0ca90ea8 = {
#include "stg12/death_zones/dat_0ca90ea8.inc"
};
static NJS_OBJECT dat_0ca90ed0 = {
#include "stg12/death_zones/dat_0ca90ed0.inc"
};
static NJS_MATERIAL dat_0ca90f04[1] = {
#include "stg12/death_zones/dat_0ca90f04.inc"
};
static Sint16 dat_0ca90f18[14] = {
#include "stg12/death_zones/dat_0ca90f18.inc"
};
static NJS_MESHSET dat_0ca90f34[1] = {
#include "stg12/death_zones/dat_0ca90f34.inc"
};
static NJS_POINT3 dat_0ca90f4c[9] = {
#include "stg12/death_zones/dat_0ca90f4c.inc"
};
static NJS_VECTOR dat_0ca90fb8[9] = {
#include "stg12/death_zones/dat_0ca90fb8.inc"
};
static NJS_MODEL dat_0ca91024 = {
#include "stg12/death_zones/dat_0ca91024.inc"
};
static NJS_OBJECT dat_0ca9104c = {
#include "stg12/death_zones/dat_0ca9104c.inc"
};
static NJS_MATERIAL dat_0ca91080[1] = {
#include "stg12/death_zones/dat_0ca91080.inc"
};
static Sint16 dat_0ca91094[14] = {
#include "stg12/death_zones/dat_0ca91094.inc"
};
static NJS_MESHSET dat_0ca910b0[1] = {
#include "stg12/death_zones/dat_0ca910b0.inc"
};
static NJS_POINT3 dat_0ca910c8[9] = {
#include "stg12/death_zones/dat_0ca910c8.inc"
};
static NJS_VECTOR dat_0ca91134[9] = {
#include "stg12/death_zones/dat_0ca91134.inc"
};
static NJS_MODEL dat_0ca911a0 = {
#include "stg12/death_zones/dat_0ca911a0.inc"
};
static NJS_OBJECT dat_0ca911c8 = {
#include "stg12/death_zones/dat_0ca911c8.inc"
};
static NJS_MATERIAL dat_0ca911fc[1] = {
#include "stg12/death_zones/dat_0ca911fc.inc"
};
static Sint16 dat_0ca91210[14] = {
#include "stg12/death_zones/dat_0ca91210.inc"
};
static NJS_MESHSET dat_0ca9122c[1] = {
#include "stg12/death_zones/dat_0ca9122c.inc"
};
static NJS_POINT3 dat_0ca91244[9] = {
#include "stg12/death_zones/dat_0ca91244.inc"
};
static NJS_VECTOR dat_0ca912b0[9] = {
#include "stg12/death_zones/dat_0ca912b0.inc"
};
static NJS_MODEL dat_0ca9131c = {
#include "stg12/death_zones/dat_0ca9131c.inc"
};
static NJS_OBJECT dat_0ca91344 = {
#include "stg12/death_zones/dat_0ca91344.inc"
};
static NJS_MATERIAL dat_0ca91378[1] = {
#include "stg12/death_zones/dat_0ca91378.inc"
};
static Sint16 dat_0ca9138c[8] = {
#include "stg12/death_zones/dat_0ca9138c.inc"
};
static NJS_MESHSET dat_0ca9139c[1] = {
#include "stg12/death_zones/dat_0ca9139c.inc"
};
static NJS_POINT3 dat_0ca913b4[6] = {
#include "stg12/death_zones/dat_0ca913b4.inc"
};
static NJS_VECTOR dat_0ca913fc[6] = {
#include "stg12/death_zones/dat_0ca913fc.inc"
};
static NJS_MODEL dat_0ca91444 = {
#include "stg12/death_zones/dat_0ca91444.inc"
};
static NJS_OBJECT dat_0ca9146c = {
#include "stg12/death_zones/dat_0ca9146c.inc"
};
/* each act's zones, ended by a null object */
static struct death_zone shelter_act0_death_zones[2] = {
#include "stg12/death_zones/shelter_act0_death_zones.inc"
};
static struct death_zone shelter_act1_death_zones[9] = {
#include "stg12/death_zones/shelter_act1_death_zones.inc"
};
static struct death_zone shelter_act2_death_zones[5] = {
#include "stg12/death_zones/shelter_act2_death_zones.inc"
};
/* a list for each of the stage's four acts. act 3 has none */
struct death_zone *shelter_death_zones[4] = {
    shelter_act0_death_zones,
    shelter_act1_death_zones,
    shelter_act2_death_zones,
    NULL,
};
