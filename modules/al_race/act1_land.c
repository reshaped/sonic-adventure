/* the race course's land table, its entries, and the name of the pvm its
 * textures load from */
#include <ninja.h>

#include "al_race/act1_land.h"
#include "al_race/act1_textures.h" /* NOLINT(misc-include-cleaner) */
#include "al_race/dat_0cbc0348.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

/* the objects the land's entries name */
extern NJS_OBJECT dat_0cba4ff8;
extern NJS_OBJECT dat_0cba5580;
extern NJS_OBJECT dat_0cba59b0;
extern NJS_OBJECT dat_0cba5cc8;
extern NJS_OBJECT dat_0cba5e58;
extern NJS_OBJECT dat_0cba5f4c;
extern NJS_OBJECT dat_0cba637c;
extern NJS_OBJECT dat_0cba6470;
extern NJS_OBJECT dat_0cba65e0;
extern NJS_OBJECT dat_0cba6770;
extern NJS_OBJECT dat_0cba69e8;
extern NJS_OBJECT dat_0cba702c;
extern NJS_OBJECT dat_0cba78d0;
extern NJS_OBJECT dat_0cba7c08;
extern NJS_OBJECT dat_0cba7f98;
extern NJS_OBJECT dat_0cba81f0;
extern NJS_OBJECT dat_0cba8448;
extern NJS_OBJECT dat_0cba86a0;
extern NJS_OBJECT dat_0cba88f8;
extern NJS_OBJECT dat_0cba8b50;
extern NJS_OBJECT dat_0cba8da8;
extern NJS_OBJECT dat_0cba9000;
extern NJS_OBJECT dat_0cba9258;
extern NJS_OBJECT dat_0cba94b8;
extern NJS_OBJECT dat_0cba96ec;
extern NJS_OBJECT dat_0cba9920;
extern NJS_OBJECT dat_0cba9b78;
extern NJS_OBJECT dat_0cbaa724;
extern NJS_OBJECT dat_0cbaac08;
extern NJS_OBJECT dat_0cbab0ec;
extern NJS_OBJECT dat_0cbab298;
extern NJS_OBJECT dat_0cbab460;
extern NJS_OBJECT dat_0cbab7c0;
extern NJS_OBJECT dat_0cbab8c4;
extern NJS_OBJECT dat_0cbabd14;
extern NJS_OBJECT dat_0cbace18;
extern NJS_OBJECT dat_0cbadea0;
extern NJS_OBJECT dat_0cbae8e4;
extern NJS_OBJECT dat_0cbaf064;
extern NJS_OBJECT dat_0cbaff68;
extern NJS_OBJECT dat_0cbb06e0;
extern NJS_OBJECT dat_0cbb0c48;
extern NJS_OBJECT dat_0cbb0f30;
extern NJS_OBJECT dat_0cbb27b0;
extern NJS_OBJECT dat_0cbb2ba8;
extern NJS_OBJECT dat_0cbb2f9c;
extern NJS_OBJECT dat_0cbb35a0;
extern NJS_OBJECT dat_0cbb3978;
extern NJS_OBJECT dat_0cbb4190;
extern NJS_OBJECT dat_0cbb4340;
extern NJS_OBJECT dat_0cbb44f0;
extern NJS_OBJECT dat_0cbb46a0;
extern NJS_OBJECT dat_0cbb4850;
extern NJS_OBJECT dat_0cbb4a00;
extern NJS_OBJECT dat_0cbb4bb0;
extern NJS_OBJECT dat_0cbb4d60;
extern NJS_OBJECT dat_0cbb4f10;
extern NJS_OBJECT dat_0cbb50c0;
extern NJS_OBJECT dat_0cbb523c;
extern NJS_OBJECT dat_0cbb546c;
extern NJS_OBJECT dat_0cbb560c;
extern NJS_OBJECT dat_0cbb57ac;
extern NJS_OBJECT dat_0cbb5b68;
extern NJS_OBJECT dat_0cbb61e8;
extern NJS_OBJECT dat_0cbb6984;
extern NJS_OBJECT dat_0cbb8db4;
extern NJS_OBJECT dat_0cbb9cf0;
extern NJS_OBJECT dat_0cbba450;
extern NJS_OBJECT dat_0cbbab7c;
extern NJS_OBJECT dat_0cbbb030;
extern NJS_OBJECT dat_0cbbb450;
extern NJS_OBJECT dat_0cbbcb04;
extern NJS_OBJECT dat_0cbbdca4;
extern NJS_OBJECT dat_0cbbe29c;
extern NJS_OBJECT dat_0cbbe8a0;
extern NJS_OBJECT dat_0cbbfb54;
extern NJS_OBJECT dat_0cbbfe80;
extern NJS_OBJECT dat_0cbc0210;
extern NJS_OBJECT dat_0cbc0244;
extern NJS_OBJECT dat_0cbc0278;
extern NJS_OBJECT dat_0cbc02ac;
extern NJS_OBJECT dat_0cbc02e0;
extern NJS_OBJECT dat_0cbc0314;

#pragma section HEAD
/* the name of the pvm the course's textures load from. the land table is
 * generated data, which can point at this array but not at a string
 * literal. SHC aligns an array to 1, and a literal to 4. as the only
 * constant of its object, this array still builds as the literal would */
static const char race_act1_land_pvm[10] = {
#include "al_race/act1_land/race_act1_land_pvm.inc"
};
static struct land_entry race_act1_land_entries[84] = {
#include "al_race/act1_land/race_act1_land_entries.inc"
};
struct land_table race_act1_land = {
#include "al_race/act1_land/race_act1_land.inc"
};
#pragma section
