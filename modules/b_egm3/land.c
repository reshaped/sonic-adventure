/* the stage's land table, and the name of the pvm its textures load from */
#include "b_egm3/land.h"
#include "b_egm3/land_textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

extern struct land_entry eggmobile3_land_entries[];
extern struct land_motion eggmobile3_land_motions[];

#pragma section HEAD
/* the pvm's name. eggmobile3_land's initializer comes from the disc, and
 * can point only at a symbol. so the name is an array, not a string
 * literal. SHC aligns an array to 1 and a literal to 4. as this file's only
 * constant, the array starts where the literal would */
static const char eggmobile3_land_pvm[11] = {
#include "b_egm3/land/eggmobile3_land_pvm.inc"
};
struct land_table eggmobile3_land = {
#include "b_egm3/land/eggmobile3_land.inc"
};
#pragma section
