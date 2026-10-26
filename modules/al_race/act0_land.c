/* the race hall's land table, its entries, and the name of the pvm its
 * textures load from */
#include <ninja.h>

#include "al_race/act0_land.h"
#include "al_race/act0_textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

/* the objects the land's entries name */
extern NJS_OBJECT dat_0cb9ebb0;
extern NJS_OBJECT dat_0cb9f3a0;
extern NJS_OBJECT dat_0cb9f700;
extern NJS_OBJECT dat_0cb9f924;
extern NJS_OBJECT dat_0cba1644;
extern NJS_OBJECT dat_0cba1acc;
extern NJS_OBJECT dat_0cba2104;
extern NJS_OBJECT dat_0cba2328;
extern NJS_OBJECT dat_0cba254c;
extern NJS_OBJECT dat_0cba2770;
extern NJS_OBJECT dat_0cba4b8c;

#pragma section HEAD
/* the name of the pvm the hall's textures load from. the land table is
 * generated data, which can point at this array but not at a string
 * literal. SHC aligns an array to 1, and a literal to 4. as the only
 * constant of its object, this array still builds as the literal would */
static const char race_act0_land_pvm[10] = {
#include "al_race/act0_land/race_act0_land_pvm.inc"
};
static struct land_entry race_act0_land_entries[11] = {
#include "al_race/act0_land/race_act0_land_entries.inc"
};
struct land_table race_act0_land = {
#include "al_race/act0_land/race_act0_land.inc"
};
#pragma section
