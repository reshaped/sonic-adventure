/* act 3's land: its entries, its table, and the table's texlist and pvm.
 * it also holds the materials of the first entry's model */
#include <ninja.h>

#include "collision/world.h"
#include "minicart/land3.h"

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c980f98;
extern NJS_OBJECT dat_0c982020;
extern NJS_OBJECT dat_0c9830a8;
extern NJS_OBJECT dat_0c984168;
extern NJS_OBJECT dat_0c985228;
extern NJS_OBJECT dat_0c9862e8;
extern NJS_OBJECT dat_0c987370;
extern NJS_OBJECT dat_0c9883f8;
extern NJS_OBJECT dat_0c989480;
extern NJS_OBJECT dat_0c98ac28;
extern NJS_OBJECT dat_0c98b08c;
extern NJS_OBJECT dat_0c98b4f0;
extern NJS_OBJECT dat_0c98b954;
extern NJS_OBJECT dat_0c98bdb8;
extern NJS_OBJECT dat_0c98c21c;
extern NJS_OBJECT dat_0c98c680;
extern NJS_OBJECT dat_0c98cae4;
extern NJS_OBJECT dat_0c98cf48;
extern NJS_OBJECT dat_0c98d3ac;
extern NJS_OBJECT dat_0c98d810;
extern NJS_OBJECT dat_0c98dc74;
extern NJS_OBJECT dat_0c98e0d8;
extern NJS_OBJECT dat_0c98e53c;
extern NJS_OBJECT dat_0c98e9a0;
extern NJS_OBJECT dat_0c98ee04;
extern NJS_OBJECT dat_0c98f268;
extern NJS_OBJECT dat_0c98f6cc;
extern NJS_OBJECT dat_0c98fb30;
extern NJS_OBJECT dat_0c98ff94;
extern NJS_OBJECT dat_0c9903f8;
extern NJS_OBJECT dat_0c99085c;
extern NJS_OBJECT dat_0c990cc0;
extern NJS_OBJECT dat_0c991124;
extern NJS_OBJECT dat_0c991588;
extern NJS_OBJECT dat_0c9919ec;
extern NJS_OBJECT dat_0c991e50;
extern NJS_OBJECT dat_0c9922b4;
extern NJS_OBJECT dat_0c992718;
extern NJS_OBJECT dat_0c992b7c;
extern NJS_OBJECT dat_0c992fe0;
extern NJS_OBJECT dat_0c993444;
extern NJS_OBJECT dat_0c9938a8;
extern NJS_OBJECT dat_0c993d0c;
extern NJS_OBJECT dat_0c994170;
extern NJS_OBJECT dat_0c9945d4;
extern NJS_OBJECT dat_0c994a38;
extern NJS_OBJECT dat_0c994e9c;
extern NJS_OBJECT dat_0c995300;
extern NJS_OBJECT dat_0c995764;
extern NJS_OBJECT dat_0c995bc8;
extern NJS_OBJECT dat_0c99602c;
extern NJS_OBJECT dat_0c996490;
extern NJS_OBJECT dat_0c9968f4;
extern NJS_OBJECT dat_0c996d58;
extern NJS_OBJECT dat_0c9971bc;
extern NJS_OBJECT dat_0c997620;
extern NJS_OBJECT dat_0c997a84;
extern NJS_OBJECT dat_0c997ee8;
extern NJS_OBJECT dat_0c99834c;
extern NJS_OBJECT dat_0c9987b0;
extern NJS_OBJECT dat_0c998c14;
extern NJS_OBJECT dat_0c999078;
extern NJS_OBJECT dat_0c9994dc;
extern NJS_OBJECT dat_0c999940;
extern NJS_OBJECT dat_0c999da4;
extern NJS_OBJECT dat_0c99a208;
extern NJS_OBJECT dat_0c99a66c;
extern NJS_OBJECT dat_0c99aad0;
extern NJS_OBJECT dat_0c99af34;
extern NJS_OBJECT dat_0c99b398;
extern NJS_OBJECT dat_0c99b7fc;
extern NJS_OBJECT dat_0c99bc60;
extern NJS_OBJECT dat_0c99c0c4;
extern NJS_OBJECT dat_0c99c528;
extern NJS_OBJECT dat_0c99c98c;
extern NJS_OBJECT dat_0c99cdf0;
extern NJS_OBJECT dat_0c99d254;
extern NJS_OBJECT dat_0c99d6b8;
extern NJS_OBJECT dat_0c99db1c;
extern NJS_OBJECT dat_0c99df80;
extern NJS_OBJECT dat_0c99e3e4;
extern NJS_OBJECT dat_0c99e848;
extern NJS_OBJECT dat_0c99ecac;
extern NJS_OBJECT dat_0c99f110;
extern NJS_OBJECT dat_0c99f574;
extern NJS_OBJECT dat_0c99f9d8;
extern NJS_OBJECT dat_0c99fe3c;
extern NJS_OBJECT dat_0c9a02a0;
extern NJS_OBJECT dat_0c9a0704;
extern NJS_OBJECT dat_0c9a0b68;
extern NJS_OBJECT dat_0c9a0fcc;
extern NJS_OBJECT dat_0c9a1430;
extern NJS_OBJECT dat_0c9a1894;
extern NJS_OBJECT dat_0c9a1cf8;
extern NJS_OBJECT dat_0c9a215c;
extern NJS_OBJECT dat_0c9a25c0;
extern NJS_OBJECT dat_0c9a2a24;
extern NJS_OBJECT dat_0c9a2e88;
extern NJS_OBJECT dat_0c9a32ec;
extern NJS_OBJECT dat_0c9a3750;
extern NJS_OBJECT dat_0c9a3bb4;
extern NJS_OBJECT dat_0c9a4018;
extern NJS_OBJECT dat_0c9a447c;
extern NJS_OBJECT dat_0c9a48e0;
extern NJS_OBJECT dat_0c9a4d44;
extern NJS_OBJECT dat_0c9a51a8;
extern NJS_OBJECT dat_0c9a560c;
extern NJS_OBJECT dat_0c9a5a70;
extern NJS_OBJECT dat_0c9a5ed4;
extern NJS_OBJECT dat_0c9a6338;
extern NJS_OBJECT dat_0c9a679c;
extern NJS_OBJECT dat_0c9a6c00;
extern NJS_OBJECT dat_0c9a8220;
extern NJS_OBJECT dat_0c9a9888;
extern NJS_OBJECT dat_0c9aaea8;
extern NJS_OBJECT dat_0c9ab30c;
extern NJS_OBJECT dat_0c9ab770;
extern NJS_OBJECT dat_0c9ac434;
extern NJS_OBJECT dat_0c9ad0f8;
extern NJS_OBJECT dat_0c9ae260;
extern NJS_OBJECT dat_0c9af3c8;
extern NJS_OBJECT dat_0c9b0530;
extern NJS_OBJECT dat_0c9b1698;
extern NJS_OBJECT dat_0c9b2800;
extern NJS_OBJECT dat_0c9b3e4c;
extern NJS_OBJECT dat_0c9b546c;
extern NJS_OBJECT dat_0c9b6a8c;
extern NJS_OBJECT dat_0c9b80ac;
extern NJS_OBJECT dat_0c9b96cc;
extern NJS_OBJECT dat_0c9bacec;
extern NJS_OBJECT dat_0c9bc30c;
extern NJS_OBJECT dat_0c9bd92c;
extern NJS_OBJECT dat_0c9bef4c;
extern NJS_OBJECT dat_0c9c056c;
extern NJS_OBJECT dat_0c9c1b8c;

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
#include "minicart/land3/text.inc"

static NJS_TEXNAME cart_land3_textures[11] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0},
};
static NJS_TEXLIST cart_land3_texlist = {
#include "minicart/land3/cart_land3_texlist.inc"
};
NJS_MATERIAL dat_0c97ff0c[5] = {
#include "minicart/land3/dat_0c97ff0c.inc"
};

#pragma section HEAD
/* the pvm's name, an array because the table's generated initializer can
 * name only a symbol. under section HEAD, SHC puts constants in CHEAD, and
 * this is the file's only one. it starts CHEAD on an 8-byte boundary, as a
 * string literal would */
static const char cart_land3_pvm[12] = {
#include "minicart/land3/cart_land3_pvm.inc"
};
/* the table starts straight after the last entry, at 0x0c90568c, 4
 * bytes past an 8-byte boundary. R32 is the SHC release that compiles this
 * file. it starts each object's data on such a boundary, so the entries and
 * the table share an object */
static struct land_entry cart_land3_entries[135] = {
#include "minicart/land3/cart_land3_entries.inc"
};
struct land_table cart_land3 = {
#include "minicart/land3/cart_land3.inc"
};
#pragma section
