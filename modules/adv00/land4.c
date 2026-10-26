/* the hotel act's land table and its entries */
#include <ninja.h>

#include "adv00/dat_0ca57a20.h" /* NOLINT(misc-include-cleaner) */
#include "adv00/land4.h"
#include "adv00/land4_textures.h" /* NOLINT(misc-include-cleaner) */
#include "collision/world.h"

/* the objects the land's entries name. adv00/dat_0ca57a20.h declares the object
 * the last entry names */
extern NJS_OBJECT dat_0ca37050;
extern NJS_OBJECT dat_0ca371b0;
extern NJS_OBJECT dat_0ca372bc;
extern NJS_OBJECT dat_0ca373e4;
extern NJS_OBJECT dat_0ca377f0;
extern NJS_OBJECT dat_0ca37950;
extern NJS_OBJECT dat_0ca37acc;
extern NJS_OBJECT dat_0ca37bf4;
extern NJS_OBJECT dat_0ca37d50;
extern NJS_OBJECT dat_0ca38ab4;
extern NJS_OBJECT dat_0ca39844;
extern NJS_OBJECT dat_0ca3a544;
extern NJS_OBJECT dat_0ca3a810;
extern NJS_OBJECT dat_0ca3ad28;
extern NJS_OBJECT dat_0ca3b668;
extern NJS_OBJECT dat_0ca3c800;
extern NJS_OBJECT dat_0ca3d298;
extern NJS_OBJECT dat_0ca3d38c;
extern NJS_OBJECT dat_0ca3d554;
extern NJS_OBJECT dat_0ca3d750;
extern NJS_OBJECT dat_0ca3d8ac;
extern NJS_OBJECT dat_0ca3dc58;
extern NJS_OBJECT dat_0ca3e9e8;
extern NJS_OBJECT dat_0ca3eaec;
extern NJS_OBJECT dat_0ca3ee6c;
extern NJS_OBJECT dat_0ca3f05c;
extern NJS_OBJECT dat_0ca3f24c;
extern NJS_OBJECT dat_0ca3f340;
extern NJS_OBJECT dat_0ca3f4bc;
extern NJS_OBJECT dat_0ca403c8;
extern NJS_OBJECT dat_0ca40508;
extern NJS_OBJECT dat_0ca407f4;
extern NJS_OBJECT dat_0ca40dd0;
extern NJS_OBJECT dat_0ca40fc0;
extern NJS_OBJECT dat_0ca411b0;
extern NJS_OBJECT dat_0ca41b4c;
extern NJS_OBJECT dat_0ca41ed0;
extern NJS_OBJECT dat_0ca42850;
extern NJS_OBJECT dat_0ca44368;
extern NJS_OBJECT dat_0ca44a04;
extern NJS_OBJECT dat_0ca45bb0;
extern NJS_OBJECT dat_0ca46acc;
extern NJS_OBJECT dat_0ca47ea4;
extern NJS_OBJECT dat_0ca4819c;
extern NJS_OBJECT dat_0ca482a0;
extern NJS_OBJECT dat_0ca48fd0;
extern NJS_OBJECT dat_0ca49518;
extern NJS_OBJECT dat_0ca4b5a0;
extern NJS_OBJECT dat_0ca4bc5c;
extern NJS_OBJECT dat_0ca4c3e8;
extern NJS_OBJECT dat_0ca4c820;
extern NJS_OBJECT dat_0ca4ce64;
extern NJS_OBJECT dat_0ca4df14;
extern NJS_OBJECT dat_0ca4ee10;
extern NJS_OBJECT dat_0ca4f4cc;
extern NJS_OBJECT dat_0ca4f838;
extern NJS_OBJECT dat_0ca4fb30;
extern NJS_OBJECT dat_0ca51398;
extern NJS_OBJECT dat_0ca5279c;
extern NJS_OBJECT dat_0ca528a0;
extern NJS_OBJECT dat_0ca52a58;
extern NJS_OBJECT dat_0ca52c10;
extern NJS_OBJECT dat_0ca552f4;
extern NJS_OBJECT dat_0ca55638;
extern NJS_OBJECT dat_0ca564dc;
extern NJS_OBJECT dat_0ca56714;
extern NJS_OBJECT dat_0ca56dac;
extern NJS_OBJECT dat_0ca56eec;
extern NJS_OBJECT dat_0ca5702c;
extern NJS_OBJECT dat_0ca5716c;
extern NJS_OBJECT dat_0ca5731c;
extern NJS_OBJECT dat_0ca57540;
extern NJS_OBJECT dat_0ca57574;
extern NJS_OBJECT dat_0ca575a8;
extern NJS_OBJECT dat_0ca575dc;
extern NJS_OBJECT dat_0ca57610;
extern NJS_OBJECT dat_0ca57644;
extern NJS_OBJECT dat_0ca57678;
extern NJS_OBJECT dat_0ca576ac;
extern NJS_OBJECT dat_0ca576e0;
extern NJS_OBJECT dat_0ca57714;
extern NJS_OBJECT dat_0ca57748;
extern NJS_OBJECT dat_0ca5777c;
extern NJS_OBJECT dat_0ca577b0;
extern NJS_OBJECT dat_0ca577e4;
extern NJS_OBJECT dat_0ca57818;
extern NJS_OBJECT dat_0ca5784c;
extern NJS_OBJECT dat_0ca57880;
extern NJS_OBJECT dat_0ca578b4;
extern NJS_OBJECT dat_0ca578e8;
extern NJS_OBJECT dat_0ca5791c;
extern NJS_OBJECT dat_0ca57950;
extern NJS_OBJECT dat_0ca57984;
extern NJS_OBJECT dat_0ca579b8;
extern NJS_OBJECT dat_0ca579ec;

static struct land_entry station_square_land4_entries[96] = {
#include "adv00/land4/station_square_land4_entries.inc"
};
struct land_table station_square_land4 = {
#include "adv00/land4/station_square_land4.inc"
};
