/* act 0's land: its entries and its table. only 1ST_READ.BIN's world_lands
 * points at the table */
#include <ninja.h>

#include "collision/world.h"
#include "stg07/dat_0c960e78.h" /* NOLINT(misc-include-cleaner) */
#include "stg07/land0.h"
#include "stg07/ruin01_textures.h" /* NOLINT(misc-include-cleaner) */

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c90eb84;
extern NJS_OBJECT dat_0c912c3c;
extern NJS_OBJECT dat_0c9138e0;
extern NJS_OBJECT dat_0c917118;
extern NJS_OBJECT dat_0c917508;
extern NJS_OBJECT dat_0c9176a4;
extern NJS_OBJECT dat_0c917914;
extern NJS_OBJECT dat_0c917ae0;
extern NJS_OBJECT dat_0c91a584;
extern NJS_OBJECT dat_0c91ad98;
extern NJS_OBJECT dat_0c91b244;
extern NJS_OBJECT dat_0c91bb80;
extern NJS_OBJECT dat_0c91c288;
extern NJS_OBJECT dat_0c91d114;
extern NJS_OBJECT dat_0c91d62c;
extern NJS_OBJECT dat_0c91e4d8;
extern NJS_OBJECT dat_0c923d4c;
extern NJS_OBJECT dat_0c928cf4;
extern NJS_OBJECT dat_0c929c28;
extern NJS_OBJECT dat_0c92b714;
extern NJS_OBJECT dat_0c931b40;
extern NJS_OBJECT dat_0c936dd8;
extern NJS_OBJECT dat_0c93c35c;
extern NJS_OBJECT dat_0c93d23c;
extern NJS_OBJECT dat_0c93d49c;
extern NJS_OBJECT dat_0c93df08;
extern NJS_OBJECT dat_0c93e284;
extern NJS_OBJECT dat_0c93e494;
extern NJS_OBJECT dat_0c93e700;
extern NJS_OBJECT dat_0c93e96c;
extern NJS_OBJECT dat_0c93f6f8;
extern NJS_OBJECT dat_0c93fbf4;
extern NJS_OBJECT dat_0c940c3c;
extern NJS_OBJECT dat_0c941094;
extern NJS_OBJECT dat_0c942368;
extern NJS_OBJECT dat_0c942918;
extern NJS_OBJECT dat_0c9458b0;
extern NJS_OBJECT dat_0c947170;
extern NJS_OBJECT dat_0c948030;
extern NJS_OBJECT dat_0c948c04;
extern NJS_OBJECT dat_0c9493d8;
extern NJS_OBJECT dat_0c94ae40;
extern NJS_OBJECT dat_0c94afa0;
extern NJS_OBJECT dat_0c94b094;
extern NJS_OBJECT dat_0c94b188;
extern NJS_OBJECT dat_0c94b2e4;
extern NJS_OBJECT dat_0c94be8c;
extern NJS_OBJECT dat_0c94cd98;
extern NJS_OBJECT dat_0c94dc4c;
extern NJS_OBJECT dat_0c94ed0c;
extern NJS_OBJECT dat_0c94f20c;
extern NJS_OBJECT dat_0c94fa4c;
extern NJS_OBJECT dat_0c94ff1c;
extern NJS_OBJECT dat_0c951430;
extern NJS_OBJECT dat_0c952c68;
extern NJS_OBJECT dat_0c95348c;
extern NJS_OBJECT ruin_act0_water_1_object;
extern NJS_OBJECT ruin_act0_water_2_object;
extern NJS_OBJECT ruin_act0_water_3_object;
extern NJS_OBJECT dat_0c95b1e4;
extern NJS_OBJECT dat_0c95b684;
extern NJS_OBJECT dat_0c95d354;
extern NJS_OBJECT dat_0c95e7c8;
extern NJS_OBJECT dat_0c9606c0;
extern NJS_OBJECT dat_0c9606f4;
extern NJS_OBJECT dat_0c960728;
extern NJS_OBJECT dat_0c96075c;
extern NJS_OBJECT dat_0c960790;
extern NJS_OBJECT dat_0c9607c4;
extern NJS_OBJECT dat_0c9607f8;
extern NJS_OBJECT dat_0c96082c;
extern NJS_OBJECT dat_0c960860;
extern NJS_OBJECT dat_0c960894;
extern NJS_OBJECT dat_0c9608c8;
extern NJS_OBJECT dat_0c9608fc;
extern NJS_OBJECT dat_0c960930;
extern NJS_OBJECT dat_0c960964;
extern NJS_OBJECT dat_0c960998;
extern NJS_OBJECT dat_0c9609cc;
extern NJS_OBJECT dat_0c960a00;
extern NJS_OBJECT dat_0c960a34;
extern NJS_OBJECT dat_0c960a68;
extern NJS_OBJECT dat_0c960a9c;
extern NJS_OBJECT dat_0c960ad0;
extern NJS_OBJECT dat_0c960b04;
extern NJS_OBJECT dat_0c960b38;
extern NJS_OBJECT dat_0c960b6c;
extern NJS_OBJECT dat_0c960ba0;
extern NJS_OBJECT dat_0c960bd4;
extern NJS_OBJECT dat_0c960c08;
extern NJS_OBJECT dat_0c960c3c;
extern NJS_OBJECT dat_0c960c70;
extern NJS_OBJECT dat_0c960ca4;
extern NJS_OBJECT dat_0c960cd8;
extern NJS_OBJECT dat_0c960d0c;
extern NJS_OBJECT dat_0c960d40;
extern NJS_OBJECT dat_0c960d74;
extern NJS_OBJECT dat_0c960da8;
extern NJS_OBJECT dat_0c960ddc;
extern NJS_OBJECT dat_0c960e10;
extern NJS_OBJECT dat_0c960e44;

static struct land_entry ruin_act0_land_entries[102] = {
#include "stg07/land0/ruin_act0_land_entries.inc"
};
struct land_table ruin_act0_land = {
#include "stg07/land0/ruin_act0_land.inc"
};
