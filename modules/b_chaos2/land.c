/* the stage's land table, which points at its entries and at
 * chaos2_land_texlist */
#include "b_chaos2/land.h"
#include "b_chaos2/textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

extern struct land_entry chaos2_land_entries[];

#pragma section HEAD
struct land_table chaos2_land = {
#include "b_chaos2/land/chaos2_land.inc"
};
#pragma section
