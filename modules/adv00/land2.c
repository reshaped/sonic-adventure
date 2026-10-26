/* act 2's land, as 1ST_READ.BIN's world_lands names it: its entries and its
 * table */
#include <ninja.h>

#include "adv00/land2.h"
#include "adv00/land2_textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c9c3868;
extern NJS_OBJECT dat_0c9c39d8;
extern NJS_OBJECT dat_0c9c3b3c;
extern NJS_OBJECT dat_0c9c3ca0;
extern NJS_OBJECT dat_0c9c3e10;
extern NJS_OBJECT dat_0c9c3f80;
extern NJS_OBJECT dat_0c9c40e4;
extern NJS_OBJECT dat_0c9c4240;
extern NJS_OBJECT dat_0c9c4334;
extern NJS_OBJECT dat_0c9c44f8;
extern NJS_OBJECT dat_0c9c4994;
extern NJS_OBJECT dat_0c9c4b64;
extern NJS_OBJECT dat_0c9c4d34;
extern NJS_OBJECT dat_0c9c4e98;
extern NJS_OBJECT dat_0c9c5310;
extern NJS_OBJECT dat_0c9c58f4;
extern NJS_OBJECT dat_0c9c5e88;
extern NJS_OBJECT dat_0c9c649c;
extern NJS_OBJECT dat_0c9c71bc;
extern NJS_OBJECT dat_0c9c7a54;
extern NJS_OBJECT dat_0c9c7f68;
extern NJS_OBJECT dat_0c9c8264;
extern NJS_OBJECT dat_0c9c89f4;
extern NJS_OBJECT dat_0c9c96cc;
extern NJS_OBJECT dat_0c9cb5cc;
extern NJS_OBJECT dat_0c9cc60c;
extern NJS_OBJECT dat_0c9cd38c;
extern NJS_OBJECT dat_0c9cd514;
extern NJS_OBJECT dat_0c9cdcac;
extern NJS_OBJECT dat_0c9ce304;
extern NJS_OBJECT dat_0c9cf078;
extern NJS_OBJECT dat_0c9cf894;
extern NJS_OBJECT dat_0c9cf9bc;
extern NJS_OBJECT dat_0c9cfab0;
extern NJS_OBJECT dat_0c9d04a4;
extern NJS_OBJECT dat_0c9d0824;
extern NJS_OBJECT dat_0c9d099c;
extern NJS_OBJECT dat_0c9d0aa0;
extern NJS_OBJECT dat_0c9d0c64;
extern NJS_OBJECT dat_0c9d1140;
extern NJS_OBJECT dat_0c9d1cc8;
extern NJS_OBJECT dat_0c9d2190;
extern NJS_OBJECT dat_0c9d27c0;
extern NJS_OBJECT dat_0c9d2984;
extern NJS_OBJECT dat_0c9d3128;
extern NJS_OBJECT dat_0c9d38a0;
extern NJS_OBJECT dat_0c9d44b8;
extern NJS_OBJECT dat_0c9d56f8;
extern NJS_OBJECT dat_0c9d6e58;
extern NJS_OBJECT dat_0c9d7d68;
extern NJS_OBJECT dat_0c9d86b0;
extern NJS_OBJECT dat_0c9d95ec;
extern NJS_OBJECT dat_0c9d97e4;
extern NJS_OBJECT dat_0c9d98e8;
extern NJS_OBJECT dat_0c9da1b4;
extern NJS_OBJECT dat_0c9da420;
extern NJS_OBJECT dat_0c9da7f8;
extern NJS_OBJECT dat_0c9da82c;
extern NJS_OBJECT dat_0c9da860;
extern NJS_OBJECT dat_0c9da894;
extern NJS_OBJECT dat_0c9da8c8;
extern NJS_OBJECT dat_0c9da8fc;
extern NJS_OBJECT dat_0c9da930;
extern NJS_OBJECT dat_0c9da964;
extern NJS_OBJECT dat_0c9da998;
extern NJS_OBJECT dat_0c9da9cc;
extern NJS_OBJECT dat_0c9daa00;
extern NJS_OBJECT dat_0c9daa34;
extern NJS_OBJECT dat_0c9daa68;
extern NJS_OBJECT dat_0c9daa9c;
extern NJS_OBJECT dat_0c9daad0;
extern NJS_OBJECT dat_0c9dab04;
extern NJS_OBJECT dat_0c9dab38;
extern NJS_OBJECT dat_0c9dab6c;
extern NJS_OBJECT dat_0c9daba0;
extern NJS_OBJECT dat_0c9dabd4;
extern NJS_OBJECT dat_0c9dac08;
extern NJS_OBJECT dat_0c9dac3c;
extern NJS_OBJECT dat_0c9dac70;
extern NJS_OBJECT dat_0c9daca4;
extern NJS_OBJECT dat_0c9dacd8;
extern NJS_OBJECT dat_0c9dad0c;
extern NJS_OBJECT dat_0c9dad40;
extern NJS_OBJECT dat_0c9dad74;
extern NJS_OBJECT dat_0c9dada8;
extern NJS_OBJECT dat_0c9daddc;
extern NJS_OBJECT dat_0c9dae10;
extern NJS_OBJECT dat_0c9dae44;
extern NJS_OBJECT dat_0c9dae78;
extern NJS_OBJECT dat_0c9daeac;
extern NJS_OBJECT dat_0c9daee0;
extern NJS_OBJECT dat_0c9daf14;
extern NJS_OBJECT dat_0c9daf48;
extern NJS_OBJECT dat_0c9daf7c;
extern NJS_OBJECT dat_0c9dafb0;
extern NJS_OBJECT dat_0c9dafe4;
extern NJS_OBJECT dat_0c9db018;
extern NJS_OBJECT dat_0c9db04c;
extern NJS_OBJECT dat_0c9db080;
extern NJS_OBJECT dat_0c9db0b4;
extern NJS_OBJECT dat_0c9db0e8;
extern NJS_OBJECT dat_0c9db11c;
extern NJS_OBJECT dat_0c9db150;
extern NJS_OBJECT dat_0c9db184;
extern NJS_OBJECT dat_0c9db1b8;
extern NJS_OBJECT dat_0c9db1ec;
extern NJS_OBJECT dat_0c9db220;
extern NJS_OBJECT dat_0c9db254;
extern NJS_OBJECT dat_0c9db288;
extern NJS_OBJECT dat_0c9db2bc;
extern NJS_OBJECT dat_0c9db2f0;
extern NJS_OBJECT dat_0c9db324;
extern NJS_OBJECT dat_0c9db358;
extern NJS_OBJECT dat_0c9db38c;
extern NJS_OBJECT dat_0c9db3c0;
extern NJS_OBJECT dat_0c9db3f4;
extern NJS_OBJECT dat_0c9db428;
extern NJS_OBJECT dat_0c9db45c;
extern NJS_OBJECT dat_0c9db490;
extern NJS_OBJECT dat_0c9db4c4;
extern NJS_OBJECT dat_0c9db4f8;
extern NJS_OBJECT dat_0c9db52c;

static struct land_entry station_square_land2_entries[122] = {
#include "adv00/land2/station_square_land2_entries.inc"
};
struct land_table station_square_land2 = {
#include "adv00/land2/station_square_land2.inc"
};
