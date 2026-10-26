/* act 4's land: its entries, its table, and the table's texlist and pvm */
#include <ninja.h>

#include "collision/world.h"
#include "minicart/dat_0c9f9730.h" /* NOLINT(misc-include-cleaner) */
#include "minicart/land4.h"

/* the objects the land's entries name. minicart/dat_0c9f9730.h declares the
 * last entry's */
extern NJS_OBJECT dat_0c9c2ccc;
extern NJS_OBJECT dat_0c9c3d54;
extern NJS_OBJECT dat_0c9c4e14;
extern NJS_OBJECT dat_0c9c5ed4;
extern NJS_OBJECT dat_0c9c6f94;
extern NJS_OBJECT dat_0c9c801c;
extern NJS_OBJECT dat_0c9c90a4;
extern NJS_OBJECT dat_0c9ca12c;
extern NJS_OBJECT dat_0c9cb1b4;
extern NJS_OBJECT dat_0c9cc23c;
extern NJS_OBJECT dat_0c9cd2c4;
extern NJS_OBJECT dat_0c9ce384;
extern NJS_OBJECT dat_0c9cf40c;
extern NJS_OBJECT dat_0c9d0bb4;
extern NJS_OBJECT dat_0c9d1018;
extern NJS_OBJECT dat_0c9d147c;
extern NJS_OBJECT dat_0c9d18e0;
extern NJS_OBJECT dat_0c9d1d44;
extern NJS_OBJECT dat_0c9d21a8;
extern NJS_OBJECT dat_0c9d260c;
extern NJS_OBJECT dat_0c9d2a70;
extern NJS_OBJECT dat_0c9d2ed4;
extern NJS_OBJECT dat_0c9d3338;
extern NJS_OBJECT dat_0c9d379c;
extern NJS_OBJECT dat_0c9d3c00;
extern NJS_OBJECT dat_0c9d4064;
extern NJS_OBJECT dat_0c9d44c8;
extern NJS_OBJECT dat_0c9d492c;
extern NJS_OBJECT dat_0c9d4d90;
extern NJS_OBJECT dat_0c9d51f4;
extern NJS_OBJECT dat_0c9d5658;
extern NJS_OBJECT dat_0c9d5abc;
extern NJS_OBJECT dat_0c9d5f20;
extern NJS_OBJECT dat_0c9d6384;
extern NJS_OBJECT dat_0c9d67e8;
extern NJS_OBJECT dat_0c9d6c4c;
extern NJS_OBJECT dat_0c9d70b0;
extern NJS_OBJECT dat_0c9d7514;
extern NJS_OBJECT dat_0c9d7978;
extern NJS_OBJECT dat_0c9d7ddc;
extern NJS_OBJECT dat_0c9d8240;
extern NJS_OBJECT dat_0c9d86a4;
extern NJS_OBJECT dat_0c9d8b08;
extern NJS_OBJECT dat_0c9d8f6c;
extern NJS_OBJECT dat_0c9d93d0;
extern NJS_OBJECT dat_0c9d9834;
extern NJS_OBJECT dat_0c9d9c98;
extern NJS_OBJECT dat_0c9da0fc;
extern NJS_OBJECT dat_0c9da560;
extern NJS_OBJECT dat_0c9da9c4;
extern NJS_OBJECT dat_0c9dae28;
extern NJS_OBJECT dat_0c9db28c;
extern NJS_OBJECT dat_0c9db6f0;
extern NJS_OBJECT dat_0c9dbb54;
extern NJS_OBJECT dat_0c9dbfb8;
extern NJS_OBJECT dat_0c9dc41c;
extern NJS_OBJECT dat_0c9dc880;
extern NJS_OBJECT dat_0c9dcce4;
extern NJS_OBJECT dat_0c9dd148;
extern NJS_OBJECT dat_0c9dd5ac;
extern NJS_OBJECT dat_0c9dda10;
extern NJS_OBJECT dat_0c9dde74;
extern NJS_OBJECT dat_0c9de2d8;
extern NJS_OBJECT dat_0c9de73c;
extern NJS_OBJECT dat_0c9deba0;
extern NJS_OBJECT dat_0c9df004;
extern NJS_OBJECT dat_0c9df468;
extern NJS_OBJECT dat_0c9df8cc;
extern NJS_OBJECT dat_0c9dfd30;
extern NJS_OBJECT dat_0c9e0194;
extern NJS_OBJECT dat_0c9e05f8;
extern NJS_OBJECT dat_0c9e0a5c;
extern NJS_OBJECT dat_0c9e0ec0;
extern NJS_OBJECT dat_0c9e1324;
extern NJS_OBJECT dat_0c9e1788;
extern NJS_OBJECT dat_0c9e1bec;
extern NJS_OBJECT dat_0c9e2050;
extern NJS_OBJECT dat_0c9e24b4;
extern NJS_OBJECT dat_0c9e2918;
extern NJS_OBJECT dat_0c9e2d7c;
extern NJS_OBJECT dat_0c9e31e0;
extern NJS_OBJECT dat_0c9e3644;
extern NJS_OBJECT dat_0c9e3aa8;
extern NJS_OBJECT dat_0c9e3f0c;
extern NJS_OBJECT dat_0c9e4370;
extern NJS_OBJECT dat_0c9e47d4;
extern NJS_OBJECT dat_0c9e4c38;
extern NJS_OBJECT dat_0c9e509c;
extern NJS_OBJECT dat_0c9e5500;
extern NJS_OBJECT dat_0c9e5964;
extern NJS_OBJECT dat_0c9e5dc8;
extern NJS_OBJECT dat_0c9e622c;
extern NJS_OBJECT dat_0c9e6690;
extern NJS_OBJECT dat_0c9e6af4;
extern NJS_OBJECT dat_0c9e6f58;
extern NJS_OBJECT dat_0c9e73bc;
extern NJS_OBJECT dat_0c9e7820;
extern NJS_OBJECT dat_0c9e7c84;
extern NJS_OBJECT dat_0c9e92a4;
extern NJS_OBJECT dat_0c9ea8c4;
extern NJS_OBJECT dat_0c9ead28;
extern NJS_OBJECT dat_0c9eb18c;
extern NJS_OBJECT dat_0c9ebe50;
extern NJS_OBJECT dat_0c9ecfb8;
extern NJS_OBJECT dat_0c9ee604;
extern NJS_OBJECT dat_0c9efc50;
extern NJS_OBJECT dat_0c9f1270;
extern NJS_OBJECT dat_0c9f2890;
extern NJS_OBJECT dat_0c9f3eb0;
extern NJS_OBJECT dat_0c9f54d0;
extern NJS_OBJECT dat_0c9f6af0;
extern NJS_OBJECT dat_0c9f8110;

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
#include "minicart/land4/text.inc"

static NJS_TEXNAME cart_land4_textures[10] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0},
};
static NJS_TEXLIST cart_land4_texlist = {
#include "minicart/land4/cart_land4_texlist.inc"
};

#pragma section HEAD
/* the pvm's name, an array because the table's generated initializer can
 * name only a symbol. under section HEAD, SHC puts constants in CHEAD, and
 * this is the file's only one. it starts CHEAD on an 8-byte boundary, as a
 * string literal would */
static const char cart_land4_pvm[12] = {
#include "minicart/land4/cart_land4_pvm.inc"
};
/* the table starts straight after the last entry, at 0x0c906694, 4
 * bytes past an 8-byte boundary. R32 is the SHC release that compiles this
 * file. it starts each object's data on such a boundary, so the entries and
 * the table share an object */
static struct land_entry cart_land4_entries[113] = {
#include "minicart/land4/cart_land4_entries.inc"
};
struct land_table cart_land4 = {
#include "minicart/land4/cart_land4.inc"
};
#pragma section
