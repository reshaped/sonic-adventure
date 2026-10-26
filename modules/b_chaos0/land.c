/* the stage's land table, which points at its entries and at
 * chaos0_land_texlist */
#include "b_chaos0/land.h"
#include "b_chaos0/textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

extern struct land_entry chaos0_land_entries[];

#pragma section HEAD
struct land_table chaos0_land = {
#include "b_chaos0/land/chaos0_land.inc"
};
#pragma section
