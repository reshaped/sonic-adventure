/* the Station Square garden's land, and the name of the pvm its textures
 * load from */
#include <ninja.h>

#include "al_garden00/dat_0cb8fbd8.h" /* NOLINT(misc-include-cleaner) */
#include "al_garden00/land.h"
#include "collision/world.h"

extern NJS_TEXLIST station_square_garden_land_texlist;
extern NJS_OBJECT dat_0cb82174;
extern NJS_OBJECT dat_0cb82760;
extern NJS_OBJECT dat_0cb83580;
extern NJS_OBJECT dat_0cb83b2c;
extern NJS_OBJECT dat_0cb84070;
extern NJS_OBJECT dat_0cb846d8;
extern NJS_OBJECT dat_0cb84b14;
extern NJS_OBJECT dat_0cb84e68;
extern NJS_OBJECT dat_0cb8528c;
extern NJS_OBJECT dat_0cb856b0;
extern NJS_OBJECT dat_0cb85ad4;
extern NJS_OBJECT dat_0cb85ef8;
extern NJS_OBJECT dat_0cb8631c;
extern NJS_OBJECT dat_0cb86740;
extern NJS_OBJECT dat_0cb86b64;
extern NJS_OBJECT dat_0cb86f88;
extern NJS_OBJECT dat_0cb873ac;
extern NJS_OBJECT dat_0cb877d0;
extern NJS_OBJECT dat_0cb87bf4;
extern NJS_OBJECT dat_0cb87ff4;
extern NJS_OBJECT dat_0cb883f4;
extern NJS_OBJECT dat_0cb887f4;
extern NJS_OBJECT dat_0cb88bf4;
extern NJS_OBJECT dat_0cb896b0;
extern NJS_OBJECT dat_0cb89e64;
extern NJS_OBJECT dat_0cb8a170;
extern NJS_OBJECT dat_0cb8a47c;
extern NJS_OBJECT dat_0cb8a788;
extern NJS_OBJECT dat_0cb8aa94;
extern NJS_OBJECT dat_0cb8aee4;
extern NJS_OBJECT dat_0cb8b334;
extern NJS_OBJECT dat_0cb8b784;
extern NJS_OBJECT dat_0cb8bbd4;
extern NJS_OBJECT dat_0cb8be28;
extern NJS_OBJECT dat_0cb8c07c;
extern NJS_OBJECT dat_0cb8c2d0;
extern NJS_OBJECT dat_0cb8c524;
extern NJS_OBJECT dat_0cb8c778;
extern NJS_OBJECT dat_0cb8c9cc;
extern NJS_OBJECT dat_0cb8cc20;
extern NJS_OBJECT dat_0cb8ce74;
extern NJS_OBJECT dat_0cb8d0c8;
extern NJS_OBJECT dat_0cb8d31c;
extern NJS_OBJECT dat_0cb8d570;
extern NJS_OBJECT dat_0cb8d7c4;
extern NJS_OBJECT dat_0cb8da18;
extern NJS_OBJECT dat_0cb8dc6c;
extern NJS_OBJECT dat_0cb8dfbc;
extern NJS_OBJECT dat_0cb8e4b4;
extern NJS_OBJECT dat_0cb8e810;
extern NJS_OBJECT dat_0cb8ed08;
extern NJS_OBJECT dat_0cb8f08c;
extern NJS_OBJECT dat_0cb8f414;
extern NJS_OBJECT dat_0cb8f7b0;
extern NJS_OBJECT dat_0cb8f96c;

/* the name is an array, since a body points only at symbols. as the object's
 * only constant, it lands where the string literal would */
static const char station_square_garden_land_pvm[9] = {
#include "al_garden00/land/station_square_garden_land_pvm.inc"
};
static struct land_entry station_square_garden_land_entries[56] = {
#include "al_garden00/land/station_square_garden_land_entries.inc"
};
struct land_table station_square_garden_land = {
#include "al_garden00/land/station_square_garden_land.inc"
};
