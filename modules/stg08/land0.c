/* act 0's land: its entries, and the table that points to them */
#include <ninja.h>

#include "collision/world.h"
#include "stg08/icecap01_textures.h" /* NOLINT(misc-include-cleaner) */
#include "stg08/land0.h"

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c91d5d0;
extern NJS_OBJECT dat_0c91dc64;
extern NJS_OBJECT dat_0c91e538;
extern NJS_OBJECT dat_0c91f3e4;
extern NJS_OBJECT dat_0c91ffc8;
extern NJS_OBJECT dat_0c920744;
extern NJS_OBJECT dat_0c922074;
extern NJS_OBJECT dat_0c92378c;
extern NJS_OBJECT dat_0c924ed0;
extern NJS_OBJECT dat_0c9265c4;
extern NJS_OBJECT dat_0c927e18;
extern NJS_OBJECT dat_0c929538;
extern NJS_OBJECT dat_0c92ac50;
extern NJS_OBJECT dat_0c92c370;
extern NJS_OBJECT dat_0c92cb74;
extern NJS_OBJECT dat_0c92d328;
extern NJS_OBJECT dat_0c92e0a0;
extern NJS_OBJECT dat_0c92e9f0;
extern NJS_OBJECT dat_0c92f440;
extern NJS_OBJECT dat_0c92fe90;
extern NJS_OBJECT dat_0c9308e4;
extern NJS_OBJECT dat_0c931334;
extern NJS_OBJECT dat_0c93366c;
extern NJS_OBJECT dat_0c935c94;
extern NJS_OBJECT dat_0c937340;
extern NJS_OBJECT dat_0c939840;
extern NJS_OBJECT dat_0c93b204;
extern NJS_OBJECT dat_0c93c25c;
extern NJS_OBJECT dat_0c93cc54;
extern NJS_OBJECT dat_0c93d384;
extern NJS_OBJECT dat_0c93e230;
extern NJS_OBJECT dat_0c93e79c;
extern NJS_OBJECT dat_0c93eec0;
extern NJS_OBJECT dat_0c93f42c;
extern NJS_OBJECT dat_0c93f78c;
extern NJS_OBJECT dat_0c93fa54;
extern NJS_OBJECT dat_0c93fd1c;
extern NJS_OBJECT dat_0c940298;
extern NJS_OBJECT dat_0c9409bc;
extern NJS_OBJECT dat_0c945300;
extern NJS_OBJECT dat_0c945c58;
extern NJS_OBJECT dat_0c947a28;
extern NJS_OBJECT dat_0c947dd4;
extern NJS_OBJECT dat_0c9483c4;
extern NJS_OBJECT dat_0c94ac90;
extern NJS_OBJECT dat_0c94b064;
extern NJS_OBJECT dat_0c94b438;
extern NJS_OBJECT dat_0c95298c;
extern NJS_OBJECT dat_0c959e44;
extern NJS_OBJECT dat_0c95a18c;
extern NJS_OBJECT dat_0c95a450;
extern NJS_OBJECT dat_0c95a7d0;
extern NJS_OBJECT dat_0c95ab60;
extern NJS_OBJECT dat_0c95b104;
extern NJS_OBJECT dat_0c95b424;
extern NJS_OBJECT dat_0c95b718;
extern NJS_OBJECT dat_0c95bd70;
extern NJS_OBJECT dat_0c95c12c;
extern NJS_OBJECT dat_0c95c87c;

static struct land_entry icecap_act0_land_entries[59] = {
#include "stg08/land0/icecap_act0_land_entries.inc"
};
struct land_table icecap_act0_land = {
#include "stg08/land0/icecap_act0_land.inc"
};
