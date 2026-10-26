/* act 1's land: its entries, and the table that points to them */
#include <ninja.h>

#include "collision/world.h"
#include "stg02/land1.h"
#include "stg02/land1_textures.h" /* NOLINT(misc-include-cleaner) */

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c94a160;
extern NJS_OBJECT dat_0c94a530;
extern NJS_OBJECT dat_0c94b850;
extern NJS_OBJECT dat_0c94c908;
extern NJS_OBJECT dat_0c94df24;
extern NJS_OBJECT dat_0c94f4f8;
extern NJS_OBJECT dat_0c94ff3c;
extern NJS_OBJECT dat_0c950184;
extern NJS_OBJECT dat_0c951930;
extern NJS_OBJECT dat_0c9528f8;
extern NJS_OBJECT dat_0c952e9c;
extern NJS_OBJECT dat_0c953e3c;
extern NJS_OBJECT dat_0c954a14;
extern NJS_OBJECT dat_0c9558b0;
extern NJS_OBJECT dat_0c956cbc;
extern NJS_OBJECT dat_0c956cf0;
extern NJS_OBJECT dat_0c956d24;
extern NJS_OBJECT dat_0c956d58;
extern NJS_OBJECT dat_0c956d8c;
extern NJS_OBJECT dat_0c956dc0;
extern NJS_OBJECT dat_0c956df4;
extern NJS_OBJECT dat_0c956e28;
extern NJS_OBJECT dat_0c956e5c;

static struct land_entry windy_act1_land_entries[23] = {
#include "stg02/land1/windy_act1_land_entries.inc"
};
struct land_table windy_act1_land = {
#include "stg02/land1/windy_act1_land.inc"
};
