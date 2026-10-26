/* the Mystic Ruins garden's land, and the name of the pvm its textures load
 * from */
#include <ninja.h>

#include "al_garden02/dat_0cb8f1e0.h" /* NOLINT(misc-include-cleaner) */
#include "al_garden02/land.h"
#include "al_garden02/land_textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

extern NJS_OBJECT dat_0cb81bb4;
extern NJS_OBJECT dat_0cb81cc0;
extern NJS_OBJECT dat_0cb82604;
extern NJS_OBJECT dat_0cb82a5c;
extern NJS_OBJECT dat_0cb82b60;
extern NJS_OBJECT dat_0cb82db0;
extern NJS_OBJECT dat_0cb83a6c;
extern NJS_OBJECT dat_0cb83dc8;
extern NJS_OBJECT dat_0cb841a0;
extern NJS_OBJECT dat_0cb84578;
extern NJS_OBJECT dat_0cb84950;
extern NJS_OBJECT dat_0cb84d28;
extern NJS_OBJECT dat_0cb84fe8;
extern NJS_OBJECT dat_0cb8539c;
extern NJS_OBJECT dat_0cb85670;
extern NJS_OBJECT dat_0cb85944;
extern NJS_OBJECT dat_0cb85aa8;
extern NJS_OBJECT dat_0cb863a4;
extern NJS_OBJECT dat_0cb86cd4;
extern NJS_OBJECT dat_0cb87120;
extern NJS_OBJECT dat_0cb8756c;
extern NJS_OBJECT dat_0cb879b8;
extern NJS_OBJECT dat_0cb87e04;
extern NJS_OBJECT dat_0cb88250;
extern NJS_OBJECT dat_0cb8869c;
extern NJS_OBJECT dat_0cb88ae8;
extern NJS_OBJECT dat_0cb88f34;
extern NJS_OBJECT dat_0cb89380;
extern NJS_OBJECT dat_0cb897cc;
extern NJS_OBJECT dat_0cb89c18;
extern NJS_OBJECT dat_0cb8a074;
extern NJS_OBJECT dat_0cb8aad8;
extern NJS_OBJECT dat_0cb8b11c;
extern NJS_OBJECT dat_0cb8b6c8;
extern NJS_OBJECT dat_0cb8c0b8;
extern NJS_OBJECT dat_0cb8ca54;
extern NJS_OBJECT dat_0cb8d8b0;
extern NJS_OBJECT dat_0cb8e1c0;
extern NJS_OBJECT dat_0cb8e630;
extern NJS_OBJECT dat_0cb8eac0;

/* the name is an array, since a body points only at symbols. as the object's
 * only constant, it lands where the string literal would */
static const char mystic_ruins_garden_land_pvm[9] = {
#include "al_garden02/land/mystic_ruins_garden_land_pvm.inc"
};
static struct land_entry mystic_ruins_garden_land_entries[41] = {
#include "al_garden02/land/mystic_ruins_garden_land_entries.inc"
};
struct land_table mystic_ruins_garden_land = {
#include "al_garden02/land/mystic_ruins_garden_land.inc"
};
