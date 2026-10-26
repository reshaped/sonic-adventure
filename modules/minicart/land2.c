/* act 2's land: its entries, its table, and the table's texlist and pvm.
 * it also holds the materials and strips of the first entry's model */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "minicart/dat_0c97fe48.h" /* NOLINT(misc-include-cleaner) */
#include "minicart/land2.h"

/* the objects the land's entries name. minicart/dat_0c97fe48.h declares the
 * last entry's */
extern NJS_OBJECT dat_0c971c30;
extern NJS_OBJECT dat_0c973438;
extern NJS_OBJECT dat_0c97389c;
extern NJS_OBJECT dat_0c974ebc;
extern NJS_OBJECT dat_0c9764dc;
extern NJS_OBJECT dat_0c976940;
extern NJS_OBJECT dat_0c976da4;
extern NJS_OBJECT dat_0c977a68;
extern NJS_OBJECT dat_0c978bd0;
extern NJS_OBJECT dat_0c97a21c;
extern NJS_OBJECT dat_0c97b83c;
extern NJS_OBJECT dat_0c97ce5c;
extern NJS_OBJECT dat_0c97e47c;
extern NJS_OBJECT dat_0c97e4b0;
extern NJS_OBJECT dat_0c97e4e4;
extern NJS_OBJECT dat_0c97e518;
extern NJS_OBJECT dat_0c97e54c;
extern NJS_OBJECT dat_0c97e580;
extern NJS_OBJECT dat_0c97e5b4;
extern NJS_OBJECT dat_0c97e5e8;
extern NJS_OBJECT dat_0c97e61c;
extern NJS_OBJECT dat_0c97e650;
extern NJS_OBJECT dat_0c97e684;
extern NJS_OBJECT dat_0c97e6b8;
extern NJS_OBJECT dat_0c97e6ec;
extern NJS_OBJECT dat_0c97e720;
extern NJS_OBJECT dat_0c97e754;
extern NJS_OBJECT dat_0c97e788;
extern NJS_OBJECT dat_0c97e7bc;
extern NJS_OBJECT dat_0c97e7f0;
extern NJS_OBJECT dat_0c97e824;
extern NJS_OBJECT dat_0c97e858;
extern NJS_OBJECT dat_0c97e88c;
extern NJS_OBJECT dat_0c97e8c0;
extern NJS_OBJECT dat_0c97e8f4;
extern NJS_OBJECT dat_0c97e928;
extern NJS_OBJECT dat_0c97e95c;
extern NJS_OBJECT dat_0c97e990;
extern NJS_OBJECT dat_0c97e9c4;
extern NJS_OBJECT dat_0c97e9f8;
extern NJS_OBJECT dat_0c97ea2c;
extern NJS_OBJECT dat_0c97ea60;
extern NJS_OBJECT dat_0c97ea94;
extern NJS_OBJECT dat_0c97eac8;
extern NJS_OBJECT dat_0c97eafc;
extern NJS_OBJECT dat_0c97eb30;
extern NJS_OBJECT dat_0c97eb64;
extern NJS_OBJECT dat_0c97eb98;
extern NJS_OBJECT dat_0c97ebcc;
extern NJS_OBJECT dat_0c97ec00;
extern NJS_OBJECT dat_0c97ec34;
extern NJS_OBJECT dat_0c97ec68;
extern NJS_OBJECT dat_0c97ec9c;
extern NJS_OBJECT dat_0c97ecd0;
extern NJS_OBJECT dat_0c97ed04;
extern NJS_OBJECT dat_0c97ed38;
extern NJS_OBJECT dat_0c97ed6c;
extern NJS_OBJECT dat_0c97eda0;
extern NJS_OBJECT dat_0c97edd4;
extern NJS_OBJECT dat_0c97ee08;
extern NJS_OBJECT dat_0c97ee3c;
extern NJS_OBJECT dat_0c97ee70;
extern NJS_OBJECT dat_0c97eea4;
extern NJS_OBJECT dat_0c97eed8;
extern NJS_OBJECT dat_0c97ef0c;
extern NJS_OBJECT dat_0c97ef40;
extern NJS_OBJECT dat_0c97ef74;
extern NJS_OBJECT dat_0c97efa8;
extern NJS_OBJECT dat_0c97efdc;
extern NJS_OBJECT dat_0c97f010;
extern NJS_OBJECT dat_0c97f044;
extern NJS_OBJECT dat_0c97f078;
extern NJS_OBJECT dat_0c97f0ac;
extern NJS_OBJECT dat_0c97f0e0;
extern NJS_OBJECT dat_0c97f114;
extern NJS_OBJECT dat_0c97f148;
extern NJS_OBJECT dat_0c97f17c;
extern NJS_OBJECT dat_0c97f1b0;
extern NJS_OBJECT dat_0c97f1e4;
extern NJS_OBJECT dat_0c97f218;
extern NJS_OBJECT dat_0c97f24c;
extern NJS_OBJECT dat_0c97f280;
extern NJS_OBJECT dat_0c97f2b4;
extern NJS_OBJECT dat_0c97f2e8;
extern NJS_OBJECT dat_0c97f31c;
extern NJS_OBJECT dat_0c97f350;
extern NJS_OBJECT dat_0c97f384;
extern NJS_OBJECT dat_0c97f3b8;
extern NJS_OBJECT dat_0c97f3ec;
extern NJS_OBJECT dat_0c97f420;
extern NJS_OBJECT dat_0c97f454;
extern NJS_OBJECT dat_0c97f488;
extern NJS_OBJECT dat_0c97f4bc;
extern NJS_OBJECT dat_0c97f4f0;
extern NJS_OBJECT dat_0c97f524;
extern NJS_OBJECT dat_0c97f558;
extern NJS_OBJECT dat_0c97f58c;
extern NJS_OBJECT dat_0c97f5c0;
extern NJS_OBJECT dat_0c97f5f4;
extern NJS_OBJECT dat_0c97f628;
extern NJS_OBJECT dat_0c97f65c;
extern NJS_OBJECT dat_0c97f690;
extern NJS_OBJECT dat_0c97f6c4;
extern NJS_OBJECT dat_0c97f6f8;
extern NJS_OBJECT dat_0c97f72c;
extern NJS_OBJECT dat_0c97f760;
extern NJS_OBJECT dat_0c97f794;
extern NJS_OBJECT dat_0c97f7c8;
extern NJS_OBJECT dat_0c97f7fc;
extern NJS_OBJECT dat_0c97f830;
extern NJS_OBJECT dat_0c97f864;
extern NJS_OBJECT dat_0c97f898;
extern NJS_OBJECT dat_0c97f8cc;
extern NJS_OBJECT dat_0c97f900;
extern NJS_OBJECT dat_0c97f934;
extern NJS_OBJECT dat_0c97f968;
extern NJS_OBJECT dat_0c97f99c;
extern NJS_OBJECT dat_0c97f9d0;
extern NJS_OBJECT dat_0c97fa04;
extern NJS_OBJECT dat_0c97fa38;
extern NJS_OBJECT dat_0c97fa6c;
extern NJS_OBJECT dat_0c97faa0;
extern NJS_OBJECT dat_0c97fad4;
extern NJS_OBJECT dat_0c97fb08;
extern NJS_OBJECT dat_0c97fb3c;
extern NJS_OBJECT dat_0c97fb70;
extern NJS_OBJECT dat_0c97fba4;
extern NJS_OBJECT dat_0c97fbd8;
extern NJS_OBJECT dat_0c97fc0c;
extern NJS_OBJECT dat_0c97fc40;
extern NJS_OBJECT dat_0c97fc74;
extern NJS_OBJECT dat_0c97fca8;
extern NJS_OBJECT dat_0c97fcdc;
extern NJS_OBJECT dat_0c97fd10;
extern NJS_OBJECT dat_0c97fd44;
extern NJS_OBJECT dat_0c97fd78;
extern NJS_OBJECT dat_0c97fdac;
extern NJS_OBJECT dat_0c97fde0;
extern NJS_OBJECT dat_0c97fe14;

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 6
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 5
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 5
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 6
#define TEXT_9_LENGTH 5
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 5
#include "minicart/land2/text.inc"

static NJS_TEXNAME cart_land2_textures[11] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0},
};
static NJS_TEXLIST cart_land2_texlist = {
#include "minicart/land2/cart_land2_texlist.inc"
};
NJS_MATERIAL dat_0c970bdc[4] = {
#include "minicart/land2/dat_0c970bdc.inc"
};
Sint16 dat_0c970c2c[68] = {
#include "minicart/land2/dat_0c970c2c.inc"
};
Sint16 dat_0c970cb4[68] = {
#include "minicart/land2/dat_0c970cb4.inc"
};
Sint16 dat_0c970d3c[68] = {
#include "minicart/land2/dat_0c970d3c.inc"
};
Sint16 dat_0c970dc4[18] = {
#include "minicart/land2/dat_0c970dc4.inc"
};

#pragma section HEAD
/* the pvm's name, an array because the table's generated initializer can
 * name only a symbol. under section HEAD, SHC puts constants in CHEAD, and
 * this is the file's only one. it starts CHEAD on an 8-byte boundary, as a
 * string literal would */
static const char cart_land2_pvm[12] = {
#include "minicart/land2/cart_land2_pvm.inc"
};
static struct land_entry cart_land2_entries[140] = {
#include "minicart/land2/cart_land2_entries.inc"
};
struct land_table cart_land2 = {
#include "minicart/land2/cart_land2.inc"
};
#pragma section
