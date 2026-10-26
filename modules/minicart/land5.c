/* act 5's land: its entries, its table, and the table's texlist and pvm */
#include <ninja.h>

#include "collision/world.h"
#include "minicart/land5.h"

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c9fa874;
extern NJS_OBJECT dat_0c9fb934;
extern NJS_OBJECT dat_0c9fc9f4;
extern NJS_OBJECT dat_0c9fdab4;
extern NJS_OBJECT dat_0c9feb3c;
extern NJS_OBJECT dat_0c9ffbc4;
extern NJS_OBJECT dat_0ca00c4c;
extern NJS_OBJECT dat_0ca01cd4;
extern NJS_OBJECT dat_0ca02d5c;
extern NJS_OBJECT dat_0ca03de4;
extern NJS_OBJECT dat_0ca04e6c;
extern NJS_OBJECT dat_0ca05ef4;
extern NJS_OBJECT dat_0ca06f7c;
extern NJS_OBJECT dat_0ca0803c;
extern NJS_OBJECT dat_0ca090c4;
extern NJS_OBJECT dat_0ca0a86c;
extern NJS_OBJECT dat_0ca0acd0;
extern NJS_OBJECT dat_0ca0b134;
extern NJS_OBJECT dat_0ca0b598;
extern NJS_OBJECT dat_0ca0b9fc;
extern NJS_OBJECT dat_0ca0be60;
extern NJS_OBJECT dat_0ca0c2c4;
extern NJS_OBJECT dat_0ca0c728;
extern NJS_OBJECT dat_0ca0cb8c;
extern NJS_OBJECT dat_0ca0cff0;
extern NJS_OBJECT dat_0ca0d454;
extern NJS_OBJECT dat_0ca0d8b8;
extern NJS_OBJECT dat_0ca0dd1c;
extern NJS_OBJECT dat_0ca0e180;
extern NJS_OBJECT dat_0ca0e5e4;
extern NJS_OBJECT dat_0ca0ea48;
extern NJS_OBJECT dat_0ca0eeac;
extern NJS_OBJECT dat_0ca0f310;
extern NJS_OBJECT dat_0ca0f774;
extern NJS_OBJECT dat_0ca0fbd8;
extern NJS_OBJECT dat_0ca1003c;
extern NJS_OBJECT dat_0ca104a0;
extern NJS_OBJECT dat_0ca10904;
extern NJS_OBJECT dat_0ca10d68;
extern NJS_OBJECT dat_0ca111cc;
extern NJS_OBJECT dat_0ca11630;
extern NJS_OBJECT dat_0ca11a94;
extern NJS_OBJECT dat_0ca11ef8;
extern NJS_OBJECT dat_0ca1235c;
extern NJS_OBJECT dat_0ca127c0;
extern NJS_OBJECT dat_0ca12c24;
extern NJS_OBJECT dat_0ca13088;
extern NJS_OBJECT dat_0ca134ec;
extern NJS_OBJECT dat_0ca13950;
extern NJS_OBJECT dat_0ca13db4;
extern NJS_OBJECT dat_0ca14218;
extern NJS_OBJECT dat_0ca1467c;
extern NJS_OBJECT dat_0ca14ae0;
extern NJS_OBJECT dat_0ca14f44;
extern NJS_OBJECT dat_0ca153a8;
extern NJS_OBJECT dat_0ca1580c;
extern NJS_OBJECT dat_0ca15c70;
extern NJS_OBJECT dat_0ca160d4;
extern NJS_OBJECT dat_0ca16538;
extern NJS_OBJECT dat_0ca1699c;
extern NJS_OBJECT dat_0ca16e00;
extern NJS_OBJECT dat_0ca17264;
extern NJS_OBJECT dat_0ca176c8;
extern NJS_OBJECT dat_0ca17b2c;
extern NJS_OBJECT dat_0ca17f90;
extern NJS_OBJECT dat_0ca183f4;
extern NJS_OBJECT dat_0ca18858;
extern NJS_OBJECT dat_0ca18cbc;
extern NJS_OBJECT dat_0ca19120;
extern NJS_OBJECT dat_0ca19584;
extern NJS_OBJECT dat_0ca199e8;
extern NJS_OBJECT dat_0ca19e4c;
extern NJS_OBJECT dat_0ca1a2b0;
extern NJS_OBJECT dat_0ca1a714;
extern NJS_OBJECT dat_0ca1ab78;
extern NJS_OBJECT dat_0ca1afdc;
extern NJS_OBJECT dat_0ca1b440;
extern NJS_OBJECT dat_0ca1b8a4;
extern NJS_OBJECT dat_0ca1bd08;
extern NJS_OBJECT dat_0ca1c16c;
extern NJS_OBJECT dat_0ca1c5d0;
extern NJS_OBJECT dat_0ca1ca34;
extern NJS_OBJECT dat_0ca1ce98;
extern NJS_OBJECT dat_0ca1d2fc;
extern NJS_OBJECT dat_0ca1d760;
extern NJS_OBJECT dat_0ca1dbc4;
extern NJS_OBJECT dat_0ca1e028;
extern NJS_OBJECT dat_0ca1e48c;
extern NJS_OBJECT dat_0ca1e8f0;
extern NJS_OBJECT dat_0ca1ed54;
extern NJS_OBJECT dat_0ca1f1b8;
extern NJS_OBJECT dat_0ca1f61c;
extern NJS_OBJECT dat_0ca1fa80;
extern NJS_OBJECT dat_0ca1fee4;
extern NJS_OBJECT dat_0ca20348;
extern NJS_OBJECT dat_0ca207ac;
extern NJS_OBJECT dat_0ca20c10;
extern NJS_OBJECT dat_0ca21074;
extern NJS_OBJECT dat_0ca214d8;
extern NJS_OBJECT dat_0ca2193c;
extern NJS_OBJECT dat_0ca21da0;
extern NJS_OBJECT dat_0ca22204;
extern NJS_OBJECT dat_0ca22668;
extern NJS_OBJECT dat_0ca22acc;
extern NJS_OBJECT dat_0ca22f30;
extern NJS_OBJECT dat_0ca23394;
extern NJS_OBJECT dat_0ca237f8;
extern NJS_OBJECT dat_0ca23c5c;
extern NJS_OBJECT dat_0ca2527c;
extern NJS_OBJECT dat_0ca2689c;
extern NJS_OBJECT dat_0ca26d00;
extern NJS_OBJECT dat_0ca27164;
extern NJS_OBJECT dat_0ca27e28;
extern NJS_OBJECT dat_0ca28aec;
extern NJS_OBJECT dat_0ca297b0;
extern NJS_OBJECT dat_0ca2a474;
extern NJS_OBJECT dat_0ca2b138;
extern NJS_OBJECT dat_0ca2bdfc;
extern NJS_OBJECT dat_0ca2cac0;
extern NJS_OBJECT dat_0ca2d784;
extern NJS_OBJECT dat_0ca2e448;
extern NJS_OBJECT dat_0ca2f10c;
extern NJS_OBJECT dat_0ca2fdd0;
extern NJS_OBJECT dat_0ca30f38;
extern NJS_OBJECT dat_0ca32584;
extern NJS_OBJECT dat_0ca33ba4;
extern NJS_OBJECT dat_0ca351c4;
extern NJS_OBJECT dat_0ca367e4;
extern NJS_OBJECT dat_0ca37e04;

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 6
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 5
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 5
#include "minicart/land5/text.inc"

static NJS_TEXNAME cart_land5_textures[10] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0},
};
static NJS_TEXLIST cart_land5_texlist = {
#include "minicart/land5/cart_land5_texlist.inc"
};

#pragma section HEAD
/* the pvm's name, an array because the table's generated initializer can
 * name only a symbol. under section HEAD, SHC puts constants in CHEAD, and
 * this is the file's only one. it starts CHEAD on an 8-byte boundary, as a
 * string literal would */
static const char cart_land5_pvm[12] = {
#include "minicart/land5/cart_land5_pvm.inc"
};
/* the table starts straight after the last entry, at 0x0c9078dc, 4
 * bytes past an 8-byte boundary. R32 is the SHC release that compiles this
 * file. it starts each object's data on such a boundary, so the entries and
 * the table share an object */
static struct land_entry cart_land5_entries[129] = {
#include "minicart/land5/cart_land5_entries.inc"
};
struct land_table cart_land5 = {
#include "minicart/land5/cart_land5.inc"
};
#pragma section
