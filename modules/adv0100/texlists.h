/* the texture lists of the Egg Carrier's outside acts, and the pvm lists
 * that load them */
#ifndef ADV0100_TEXLISTS_H
#define ADV0100_TEXLISTS_H
#include <ninja.h>

#include "system/texture.h"

/* the lists lie in ADV0100, so use them only while it is loaded. the pvm
 * lists below name each of them */
extern NJS_TEXLIST ec_objects_texlist;
extern NJS_TEXLIST ec_water_texlist;
extern NJS_TEXLIST ec_act_0_texlist;
extern NJS_TEXLIST ec_act_1_texlist;
extern NJS_TEXLIST ec_act_2_texlist;
extern NJS_TEXLIST ec_act_3_texlist;
extern NJS_TEXLIST ec_act_4_texlist;
extern NJS_TEXLIST ec_act_5_texlist;
extern NJS_TEXLIST ec_sky_texlist;
extern NJS_TEXLIST ec_tornado_texlist;
extern NJS_TEXLIST ec_sea_texlist;
extern NJS_TEXLIST boat_texlist;
extern NJS_TEXLIST raft_texlist;

/* texture_load_stage loads egg_carrier_pvms, which 1ST_READ.BIN's
 * stage_pvms names. once egg_carrier_has_sunk is TRUE, it loads
 * egg_carrier_sea_pvms in its place. texture_load_act loads each act's rows
 * of egg_carrier_act_pvms, which 1ST_READ.BIN's adv0100_acts names */
extern struct texture_pvm egg_carrier_pvms[];
extern struct texture_pvm egg_carrier_sea_pvms[];
extern struct texture_pvm egg_carrier_act_pvms[];

#endif
