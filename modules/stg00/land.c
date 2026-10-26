/* the Hedgehog Hammer's land: its entries, and the table that points to
 * them */
#include <ninja.h>

#include "collision/world.h"
#include "stg00/land.h"
/* only the full build's table points to practice_texlist. the full build
 * compiles the table's initializer as extracted from the image, where the
 * stub build compiles zeros */
#include "stg00/texlists.h" /* NOLINT(misc-include-cleaner) */

/* the object of each of the land's entries */
extern NJS_OBJECT dat_0c904bf8;
extern NJS_OBJECT dat_0c9056d8;
extern NJS_OBJECT dat_0c905ddc;
extern NJS_OBJECT dat_0c90601c;
extern NJS_OBJECT dat_0c90625c;
extern NJS_OBJECT dat_0c9074dc;
extern NJS_OBJECT dat_0c907ab4;
extern NJS_OBJECT dat_0c908258;
extern NJS_OBJECT dat_0c9096fc;
extern NJS_OBJECT dat_0c90aba0;
extern NJS_OBJECT dat_0c90b32c;
extern NJS_OBJECT dat_0c90bc6c;
extern NJS_OBJECT dat_0c90e768;
extern NJS_OBJECT dat_0c90efc4;
extern NJS_OBJECT dat_0c90f820;
extern NJS_OBJECT dat_0c90ff98;
extern NJS_OBJECT dat_0c910710;
extern NJS_OBJECT dat_0c9110cc;
extern NJS_OBJECT dat_0c9118a4;
extern NJS_OBJECT dat_0c911d60;
extern NJS_OBJECT dat_0c912244;
extern NJS_OBJECT dat_0c912278;
extern NJS_OBJECT dat_0c9122ac;
extern NJS_OBJECT dat_0c9122e0;
extern NJS_OBJECT dat_0c912314;
extern NJS_OBJECT dat_0c912348;
extern NJS_OBJECT dat_0c91237c;
extern NJS_OBJECT dat_0c9123b0;
extern NJS_OBJECT dat_0c9123e4;
extern NJS_OBJECT dat_0c912418;
extern NJS_OBJECT dat_0c91244c;
extern NJS_OBJECT dat_0c912480;
extern NJS_OBJECT dat_0c9124b4;
extern NJS_OBJECT dat_0c9124e8;
extern NJS_OBJECT dat_0c91251c;
extern NJS_OBJECT dat_0c912550;
extern NJS_OBJECT dat_0c912584;
extern NJS_OBJECT dat_0c9125b8;
extern NJS_OBJECT dat_0c9125ec;
extern NJS_OBJECT dat_0c912620;
extern NJS_OBJECT dat_0c912654;
extern NJS_OBJECT dat_0c912688;
extern NJS_OBJECT dat_0c9126bc;
extern NJS_OBJECT dat_0c9126f0;
extern NJS_OBJECT dat_0c912724;
extern NJS_OBJECT dat_0c912758;
extern NJS_OBJECT dat_0c91278c;
extern NJS_OBJECT dat_0c9127c0;
extern NJS_OBJECT dat_0c9127f4;
extern NJS_OBJECT dat_0c912828;
extern NJS_OBJECT dat_0c91285c;
extern NJS_OBJECT dat_0c912890;
extern NJS_OBJECT dat_0c9128c4;
extern NJS_OBJECT dat_0c9128f8;
extern NJS_OBJECT dat_0c91292c;
extern NJS_OBJECT dat_0c912960;
extern NJS_OBJECT dat_0c912994;
extern NJS_OBJECT dat_0c9129c8;
extern NJS_OBJECT dat_0c9129fc;
extern NJS_OBJECT dat_0c912a30;
extern NJS_OBJECT dat_0c912a64;
extern NJS_OBJECT dat_0c912a98;
extern NJS_OBJECT dat_0c912acc;
extern NJS_OBJECT dat_0c912b00;
extern NJS_OBJECT dat_0c912b34;
extern NJS_OBJECT dat_0c912b68;
extern NJS_OBJECT dat_0c912b9c;
extern NJS_OBJECT dat_0c912bd0;
extern NJS_OBJECT dat_0c912c04;
extern NJS_OBJECT dat_0c912c38;
extern NJS_OBJECT dat_0c912c6c;

#pragma section HEAD
static struct land_entry practice_land_entries[71] = {
#include "stg00/land/practice_land_entries.inc"
};
struct land_table practice_land = {
#include "stg00/land/practice_land.inc"
};
#pragma section
