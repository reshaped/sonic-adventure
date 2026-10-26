/* the Egg Carrier outside's act 5 land, which 1ST_READ.BIN's world_lands
 * names */
#include "adv0100/act5_land.h"
#include "adv0100/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

/* its entries, which generated data defines */
extern struct land_entry outside_act5_land_entries[];

struct land_table outside_act5_land = {
#include "adv0100/act5_land/outside_act5_land.inc"
};
