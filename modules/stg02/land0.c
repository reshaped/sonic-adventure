/* act 0's land: its entries, and the table that points to them */
#include <ninja.h>

#include "collision/world.h"
#include "stg02/dat_0c948ea0.h" /* NOLINT(misc-include-cleaner) */
#include "stg02/land0.h"
#include "stg02/land0_textures.h" /* NOLINT(misc-include-cleaner) */

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c911414;
extern NJS_OBJECT dat_0c911918;
extern NJS_OBJECT dat_0c911b78;
extern NJS_OBJECT dat_0c911c84;
extern NJS_OBJECT dat_0c911f78;
extern NJS_OBJECT dat_0c91206c;
extern NJS_OBJECT dat_0c913f54;
extern NJS_OBJECT dat_0c916738;
extern NJS_OBJECT dat_0c917818;
extern NJS_OBJECT dat_0c918120;
extern NJS_OBJECT dat_0c919450;
extern NJS_OBJECT dat_0c91a048;
extern NJS_OBJECT dat_0c91aa24;
extern NJS_OBJECT dat_0c91b3c4;
extern NJS_OBJECT dat_0c91bfe0;
extern NJS_OBJECT dat_0c91d54c;
extern NJS_OBJECT dat_0c91e574;
extern NJS_OBJECT dat_0c91f260;
extern NJS_OBJECT dat_0c920dfc;
extern NJS_OBJECT dat_0c9231c4;
extern NJS_OBJECT dat_0c925958;
extern NJS_OBJECT dat_0c926b44;
extern NJS_OBJECT dat_0c928660;
extern NJS_OBJECT dat_0c92980c;
extern NJS_OBJECT dat_0c92a398;
extern NJS_OBJECT dat_0c92bbf4;
extern NJS_OBJECT dat_0c92d088;
extern NJS_OBJECT dat_0c92eb8c;
extern NJS_OBJECT dat_0c92f030;
extern NJS_OBJECT dat_0c9304f4;
extern NJS_OBJECT dat_0c930e24;
extern NJS_OBJECT dat_0c931ea0;
extern NJS_OBJECT dat_0c93265c;
extern NJS_OBJECT dat_0c933b0c;
extern NJS_OBJECT dat_0c9340a4;
extern NJS_OBJECT dat_0c9347a4;
extern NJS_OBJECT dat_0c934e1c;
extern NJS_OBJECT dat_0c935708;
extern NJS_OBJECT dat_0c9362c8;
extern NJS_OBJECT dat_0c93695c;
extern NJS_OBJECT dat_0c936eec;
extern NJS_OBJECT dat_0c937bac;
extern NJS_OBJECT dat_0c9380d0;
extern NJS_OBJECT dat_0c93988c;
extern NJS_OBJECT dat_0c939d3c;
extern NJS_OBJECT dat_0c93a154;
extern NJS_OBJECT dat_0c93b90c;
extern NJS_OBJECT dat_0c93bce0;
extern NJS_OBJECT dat_0c93c610;
extern NJS_OBJECT dat_0c93c910;
extern NJS_OBJECT dat_0c93cf3c;
extern NJS_OBJECT dat_0c93d78c;
extern NJS_OBJECT dat_0c93e12c;
extern NJS_OBJECT dat_0c93e398;
extern NJS_OBJECT dat_0c93e9d0;
extern NJS_OBJECT dat_0c93eb58;
extern NJS_OBJECT dat_0c93f7c4;
extern NJS_OBJECT dat_0c941154;
extern NJS_OBJECT dat_0c94274c;
extern NJS_OBJECT dat_0c943eb0;
extern NJS_OBJECT dat_0c944398;
extern NJS_OBJECT dat_0c944710;
extern NJS_OBJECT dat_0c9457d8;
extern NJS_OBJECT dat_0c946354;
extern NJS_OBJECT dat_0c947224;
extern NJS_OBJECT dat_0c948d68;
extern NJS_OBJECT dat_0c948d9c;
extern NJS_OBJECT dat_0c948dd0;
extern NJS_OBJECT dat_0c948e04;
extern NJS_OBJECT dat_0c948e38;
extern NJS_OBJECT dat_0c948e6c;

static struct land_entry windy_act0_land_entries[72] = {
#include "stg02/land0/windy_act0_land_entries.inc"
};
struct land_table windy_act0_land = {
#include "stg02/land0/windy_act0_land.inc"
};
