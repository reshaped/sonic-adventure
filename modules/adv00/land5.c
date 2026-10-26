/* the Twinkle Park act's land table and its entries */
#include <ninja.h>

#include "adv00/dat_0ca5e1f0.h" /* NOLINT(misc-include-cleaner) */
#include "adv00/land5.h"
#include "adv00/land5_textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

/* the objects the land's entries name. adv00/dat_0ca5e1f0.h declares the object
 * the last entry names */
extern NJS_OBJECT dat_0ca57fd4;
extern NJS_OBJECT dat_0ca5843c;
extern NJS_OBJECT dat_0ca58530;
extern NJS_OBJECT dat_0ca58710;
extern NJS_OBJECT dat_0ca58878;
extern NJS_OBJECT dat_0ca58a3c;
extern NJS_OBJECT dat_0ca58d34;
extern NJS_OBJECT dat_0ca598fc;
extern NJS_OBJECT dat_0ca5a230;
extern NJS_OBJECT dat_0ca5a43c;
extern NJS_OBJECT dat_0ca5b17c;
extern NJS_OBJECT dat_0ca5b608;
extern NJS_OBJECT dat_0ca5b800;
extern NJS_OBJECT dat_0ca5bd5c;
extern NJS_OBJECT dat_0ca5cdd0;
extern NJS_OBJECT dat_0ca5cf38;
extern NJS_OBJECT dat_0ca5d0fc;
extern NJS_OBJECT dat_0ca5d3f4;
extern NJS_OBJECT dat_0ca5d6c0;
extern NJS_OBJECT dat_0ca5e1bc;

static struct land_entry station_square_land5_entries[21] = {
#include "adv00/land5/station_square_land5_entries.inc"
};
struct land_table station_square_land5 = {
#include "adv00/land5/station_square_land5.inc"
};
