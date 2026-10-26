/* the Egg Carrier garden's land, the part of it that moves, and the name of
 * the pvm its textures load from */
#include <ninja.h>

#include "al_garden01/land.h"
#include "collision/world.h"

extern NJS_TEXLIST egg_carrier_garden_land_texlist;
extern NJS_ACTION dat_0cb8d690;
extern NJS_OBJECT dat_0cb81560;
extern NJS_OBJECT dat_0cb81758;
extern NJS_OBJECT dat_0cb819f8;
extern NJS_OBJECT dat_0cb81c4c;
extern NJS_OBJECT dat_0cb81db4;
extern NJS_OBJECT dat_0cb83960;
extern NJS_OBJECT dat_0cb840b4;
extern NJS_OBJECT dat_0cb84f40;
extern NJS_OBJECT dat_0cb85c38;
extern NJS_OBJECT dat_0cb85f68;
extern NJS_OBJECT dat_0cb8630c;
extern NJS_OBJECT dat_0cb8646c;
extern NJS_OBJECT dat_0cb86cf4;
extern NJS_OBJECT dat_0cb87654;
extern NJS_OBJECT dat_0cb88100;
extern NJS_OBJECT dat_0cb88ca4;
extern NJS_OBJECT dat_0cb89574;
extern NJS_OBJECT dat_0cb89b80;
extern NJS_OBJECT dat_0cb8a088;
extern NJS_OBJECT dat_0cb8ab08;
extern NJS_OBJECT dat_0cb8c1a0;
extern NJS_OBJECT dat_0cb8cad8;
extern NJS_OBJECT dat_0cb8d3e0;

/* the name is an array, since a body points only at symbols. as the object's
 * only constant, it lands where the string literal would */
static const char egg_carrier_garden_land_pvm[9] = {
#include "al_garden01/land/egg_carrier_garden_land_pvm.inc"
};
static struct land_entry egg_carrier_garden_land_entries[22] = {
#include "al_garden01/land/egg_carrier_garden_land_entries.inc"
};
static struct land_motion egg_carrier_garden_land_motions[1] = {
#include "al_garden01/land/egg_carrier_garden_land_motions.inc"
};
struct land_table egg_carrier_garden_land = {
#include "al_garden01/land/egg_carrier_garden_land.inc"
};
