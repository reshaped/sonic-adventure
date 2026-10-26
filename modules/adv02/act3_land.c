/* the Mystic Ruins' act 3 land */
#include "adv02/act3_land.h"
#include "adv02/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

extern struct land_entry mystic_ruins_act3_land_entries[];
extern struct land_motion mystic_ruins_act3_land_motions[];

#pragma section HEAD
struct land_table mystic_ruins_act3_land = {
#include "adv02/act3_land/mystic_ruins_act3_land.inc"
};
#pragma section
