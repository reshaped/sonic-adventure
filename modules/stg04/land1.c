/* act 1's land: its entries, its table, and the name of the pvm its texlist
 * loads from */
#include <ninja.h>

#include "collision/world.h"
#include "stg04/dat_0c9a96e0.h" /* NOLINT(misc-include-cleaner) */
#include "stg04/land1.h"
#include "stg04/land1_textures.h" /* NOLINT(misc-include-cleaner) */

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c99d9cc;
extern NJS_OBJECT dat_0c99dfe4;
extern NJS_OBJECT dat_0c99ee5c;
extern NJS_OBJECT dat_0c99efd4;
extern NJS_OBJECT dat_0c99f14c;
extern NJS_OBJECT dat_0c99fae8;
extern NJS_OBJECT dat_0c9a0484;
extern NJS_OBJECT dat_0c9a0fb0;
extern NJS_OBJECT dat_0c9a1b4c;
extern NJS_OBJECT dat_0c9a2324;
extern NJS_OBJECT dat_0c9a2868;
extern NJS_OBJECT dat_0c9a2e08;
extern NJS_OBJECT dat_0c9a3628;
extern NJS_OBJECT dat_0c9a3798;
extern NJS_OBJECT dat_0c9a3a04;
extern NJS_OBJECT dat_0c9a3b2c;
extern NJS_OBJECT dat_0c9a3c54;
extern NJS_OBJECT dat_0c9a3dd0;
extern NJS_OBJECT dat_0c9a3f4c;
extern NJS_OBJECT dat_0c9a4e84;
extern NJS_OBJECT dat_0c9a5d68;
extern NJS_OBJECT dat_0c9a68fc;
extern NJS_OBJECT dat_0c9a72b0;
extern NJS_OBJECT dat_0c9a7518;
extern NJS_OBJECT dat_0c9a8a7c;
extern NJS_OBJECT dat_0c9a8ab0;
extern NJS_OBJECT dat_0c9a8ae4;
extern NJS_OBJECT dat_0c9a8b18;
extern NJS_OBJECT dat_0c9a8b4c;
extern NJS_OBJECT dat_0c9a8b80;
extern NJS_OBJECT dat_0c9a8bb4;
extern NJS_OBJECT dat_0c9a8be8;
extern NJS_OBJECT dat_0c9a8c1c;
extern NJS_OBJECT dat_0c9a8c50;
extern NJS_OBJECT dat_0c9a8c84;
extern NJS_OBJECT dat_0c9a8cb8;
extern NJS_OBJECT dat_0c9a8cec;
extern NJS_OBJECT dat_0c9a8d20;
extern NJS_OBJECT dat_0c9a8d54;
extern NJS_OBJECT dat_0c9a8d88;
extern NJS_OBJECT dat_0c9a8dbc;
extern NJS_OBJECT dat_0c9a8df0;
extern NJS_OBJECT dat_0c9a8e24;
extern NJS_OBJECT dat_0c9a8e58;
extern NJS_OBJECT dat_0c9a8e8c;
extern NJS_OBJECT dat_0c9a8ec0;
extern NJS_OBJECT dat_0c9a8ef4;
extern NJS_OBJECT dat_0c9a8f28;
extern NJS_OBJECT dat_0c9a8f5c;
extern NJS_OBJECT dat_0c9a8f90;
extern NJS_OBJECT dat_0c9a8fc4;
extern NJS_OBJECT dat_0c9a8ff8;
extern NJS_OBJECT dat_0c9a902c;
extern NJS_OBJECT dat_0c9a9060;
extern NJS_OBJECT dat_0c9a9094;
extern NJS_OBJECT dat_0c9a90c8;
extern NJS_OBJECT dat_0c9a90fc;
extern NJS_OBJECT dat_0c9a9130;
extern NJS_OBJECT dat_0c9a9164;
extern NJS_OBJECT dat_0c9a9198;
extern NJS_OBJECT dat_0c9a91cc;
extern NJS_OBJECT dat_0c9a9200;
extern NJS_OBJECT dat_0c9a9234;
extern NJS_OBJECT dat_0c9a9268;
extern NJS_OBJECT dat_0c9a929c;
extern NJS_OBJECT dat_0c9a92d0;
extern NJS_OBJECT dat_0c9a9304;
extern NJS_OBJECT dat_0c9a9338;
extern NJS_OBJECT dat_0c9a936c;
extern NJS_OBJECT dat_0c9a93a0;
extern NJS_OBJECT dat_0c9a93d4;
extern NJS_OBJECT dat_0c9a9408;
extern NJS_OBJECT dat_0c9a943c;
extern NJS_OBJECT dat_0c9a9470;
extern NJS_OBJECT dat_0c9a94a4;
extern NJS_OBJECT dat_0c9a94d8;
extern NJS_OBJECT dat_0c9a950c;
extern NJS_OBJECT dat_0c9a9540;
extern NJS_OBJECT dat_0c9a9574;
extern NJS_OBJECT dat_0c9a95a8;
extern NJS_OBJECT dat_0c9a95dc;
extern NJS_OBJECT dat_0c9a9610;
extern NJS_OBJECT dat_0c9a9644;
extern NJS_OBJECT dat_0c9a9678;
extern NJS_OBJECT dat_0c9a96ac;

/* the pvm's name. a body points only at symbols, so the name is an array,
 * not a string literal. SHC aligns an array to 1 and a literal to 4. as this
 * file's only constant, the array starts where the literal would */
static const char highway_land1_pvm[10] = {
#include "stg04/land1/highway_land1_pvm.inc"
};
static struct land_entry highway_land1_entries[86] = {
#include "stg04/land1/highway_land1_entries.inc"
};
struct land_table highway_land1 = {
#include "stg04/land1/highway_land1.inc"
};
