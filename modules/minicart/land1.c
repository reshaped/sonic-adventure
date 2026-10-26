/* act 1's land: its entries, its table, and the table's texlist and pvm.
 * it also holds the materials of the first entry's model */
#include <ninja.h>

#include "collision/world.h"
#include "minicart/dat_0c970b18.h" /* NOLINT(misc-include-cleaner) */
#include "minicart/land1.h"

/* the objects the land's entries name. minicart/dat_0c970b18.h declares the
 * last entry's */
extern NJS_OBJECT dat_0c92a410;
extern NJS_OBJECT dat_0c92b498;
extern NJS_OBJECT dat_0c92c558;
extern NJS_OBJECT dat_0c92d5e0;
extern NJS_OBJECT dat_0c92e6a0;
extern NJS_OBJECT dat_0c92f760;
extern NJS_OBJECT dat_0c930820;
extern NJS_OBJECT dat_0c9318a8;
extern NJS_OBJECT dat_0c932930;
extern NJS_OBJECT dat_0c9339f0;
extern NJS_OBJECT dat_0c934a78;
extern NJS_OBJECT dat_0c936220;
extern NJS_OBJECT dat_0c936684;
extern NJS_OBJECT dat_0c936ae8;
extern NJS_OBJECT dat_0c936f4c;
extern NJS_OBJECT dat_0c9373b0;
extern NJS_OBJECT dat_0c937814;
extern NJS_OBJECT dat_0c937c78;
extern NJS_OBJECT dat_0c9380dc;
extern NJS_OBJECT dat_0c938540;
extern NJS_OBJECT dat_0c9389a4;
extern NJS_OBJECT dat_0c938e08;
extern NJS_OBJECT dat_0c93926c;
extern NJS_OBJECT dat_0c9396d0;
extern NJS_OBJECT dat_0c939b34;
extern NJS_OBJECT dat_0c939f98;
extern NJS_OBJECT dat_0c93a3fc;
extern NJS_OBJECT dat_0c93a860;
extern NJS_OBJECT dat_0c93acc4;
extern NJS_OBJECT dat_0c93b128;
extern NJS_OBJECT dat_0c93b58c;
extern NJS_OBJECT dat_0c93b9f0;
extern NJS_OBJECT dat_0c93be54;
extern NJS_OBJECT dat_0c93c2b8;
extern NJS_OBJECT dat_0c93c71c;
extern NJS_OBJECT dat_0c93cb80;
extern NJS_OBJECT dat_0c93cfe4;
extern NJS_OBJECT dat_0c93d448;
extern NJS_OBJECT dat_0c93d8ac;
extern NJS_OBJECT dat_0c93dd10;
extern NJS_OBJECT dat_0c93e174;
extern NJS_OBJECT dat_0c93e5d8;
extern NJS_OBJECT dat_0c93ea3c;
extern NJS_OBJECT dat_0c93eea0;
extern NJS_OBJECT dat_0c93f304;
extern NJS_OBJECT dat_0c93f768;
extern NJS_OBJECT dat_0c93fbcc;
extern NJS_OBJECT dat_0c940030;
extern NJS_OBJECT dat_0c940494;
extern NJS_OBJECT dat_0c9408f8;
extern NJS_OBJECT dat_0c940d5c;
extern NJS_OBJECT dat_0c9411c0;
extern NJS_OBJECT dat_0c941624;
extern NJS_OBJECT dat_0c941a88;
extern NJS_OBJECT dat_0c941eec;
extern NJS_OBJECT dat_0c942350;
extern NJS_OBJECT dat_0c9427b4;
extern NJS_OBJECT dat_0c942c18;
extern NJS_OBJECT dat_0c94307c;
extern NJS_OBJECT dat_0c9434e0;
extern NJS_OBJECT dat_0c943944;
extern NJS_OBJECT dat_0c943da8;
extern NJS_OBJECT dat_0c94420c;
extern NJS_OBJECT dat_0c944670;
extern NJS_OBJECT dat_0c944ad4;
extern NJS_OBJECT dat_0c944f38;
extern NJS_OBJECT dat_0c94539c;
extern NJS_OBJECT dat_0c945800;
extern NJS_OBJECT dat_0c945c64;
extern NJS_OBJECT dat_0c9460c8;
extern NJS_OBJECT dat_0c94652c;
extern NJS_OBJECT dat_0c946990;
extern NJS_OBJECT dat_0c946df4;
extern NJS_OBJECT dat_0c947258;
extern NJS_OBJECT dat_0c9476bc;
extern NJS_OBJECT dat_0c947b20;
extern NJS_OBJECT dat_0c947f84;
extern NJS_OBJECT dat_0c9483e8;
extern NJS_OBJECT dat_0c94884c;
extern NJS_OBJECT dat_0c948cb0;
extern NJS_OBJECT dat_0c949114;
extern NJS_OBJECT dat_0c949578;
extern NJS_OBJECT dat_0c9499dc;
extern NJS_OBJECT dat_0c949e40;
extern NJS_OBJECT dat_0c94a2a4;
extern NJS_OBJECT dat_0c94a708;
extern NJS_OBJECT dat_0c94ab6c;
extern NJS_OBJECT dat_0c94afd0;
extern NJS_OBJECT dat_0c94b434;
extern NJS_OBJECT dat_0c94b898;
extern NJS_OBJECT dat_0c94bcfc;
extern NJS_OBJECT dat_0c94c160;
extern NJS_OBJECT dat_0c94c5c4;
extern NJS_OBJECT dat_0c94ca28;
extern NJS_OBJECT dat_0c94ce8c;
extern NJS_OBJECT dat_0c94d2f0;
extern NJS_OBJECT dat_0c94d754;
extern NJS_OBJECT dat_0c94dbb8;
extern NJS_OBJECT dat_0c94e01c;
extern NJS_OBJECT dat_0c94e480;
extern NJS_OBJECT dat_0c94e8e4;
extern NJS_OBJECT dat_0c94ed48;
extern NJS_OBJECT dat_0c94f1ac;
extern NJS_OBJECT dat_0c94f610;
extern NJS_OBJECT dat_0c94fa74;
extern NJS_OBJECT dat_0c94fed8;
extern NJS_OBJECT dat_0c95033c;
extern NJS_OBJECT dat_0c9507a0;
extern NJS_OBJECT dat_0c950c04;
extern NJS_OBJECT dat_0c951068;
extern NJS_OBJECT dat_0c9514cc;
extern NJS_OBJECT dat_0c951930;
extern NJS_OBJECT dat_0c951d94;
extern NJS_OBJECT dat_0c9521f8;
extern NJS_OBJECT dat_0c95265c;
extern NJS_OBJECT dat_0c952ac0;
extern NJS_OBJECT dat_0c952f24;
extern NJS_OBJECT dat_0c953388;
extern NJS_OBJECT dat_0c9537ec;
extern NJS_OBJECT dat_0c953c50;
extern NJS_OBJECT dat_0c9540b4;
extern NJS_OBJECT dat_0c954518;
extern NJS_OBJECT dat_0c95497c;
extern NJS_OBJECT dat_0c954de0;
extern NJS_OBJECT dat_0c955244;
extern NJS_OBJECT dat_0c9556a8;
extern NJS_OBJECT dat_0c955b0c;
extern NJS_OBJECT dat_0c95712c;
extern NJS_OBJECT dat_0c958794;
extern NJS_OBJECT dat_0c959db4;
extern NJS_OBJECT dat_0c95a218;
extern NJS_OBJECT dat_0c95a67c;
extern NJS_OBJECT dat_0c95b340;
extern NJS_OBJECT dat_0c95c004;
extern NJS_OBJECT dat_0c95ccc8;
extern NJS_OBJECT dat_0c95d98c;
extern NJS_OBJECT dat_0c95eaf4;
extern NJS_OBJECT dat_0c960140;
extern NJS_OBJECT dat_0c96178c;
extern NJS_OBJECT dat_0c962dd8;
extern NJS_OBJECT dat_0c9643f8;
extern NJS_OBJECT dat_0c965a18;
extern NJS_OBJECT dat_0c967038;
extern NJS_OBJECT dat_0c968658;
extern NJS_OBJECT dat_0c969c78;
extern NJS_OBJECT dat_0c96b298;
extern NJS_OBJECT dat_0c96c8b8;
extern NJS_OBJECT dat_0c96ded8;
extern NJS_OBJECT dat_0c96f4f8;

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
#include "minicart/land1/text.inc"

static NJS_TEXNAME cart_land1_textures[11] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0},
};
static NJS_TEXLIST cart_land1_texlist = {
#include "minicart/land1/cart_land1_texlist.inc"
};
NJS_MATERIAL dat_0c929384[5] = {
#include "minicart/land1/dat_0c929384.inc"
};

#pragma section HEAD
/* the pvm's name, an array because the table's generated initializer can
 * name only a symbol. under section HEAD, SHC puts constants in CHEAD, and
 * this is the file's only one. it starts CHEAD on an 8-byte boundary, as a
 * string literal would */
static const char cart_land1_pvm[12] = {
#include "minicart/land1/cart_land1_pvm.inc"
};
static struct land_entry cart_land1_entries[150] = {
#include "minicart/land1/cart_land1_entries.inc"
};
struct land_table cart_land1 = {
#include "minicart/land1/cart_land1.inc"
};
#pragma section
