/* act 0's land: its entries, its table, and the table's texlist and pvm */
#include <ninja.h>

#include "collision/world.h"
#include "minicart/land0.h"

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c919884;
extern NJS_OBJECT dat_0c91aea4;
extern NJS_OBJECT dat_0c91c64c;
extern NJS_OBJECT dat_0c91dc6c;
extern NJS_OBJECT dat_0c91f2b8;
extern NJS_OBJECT dat_0c920420;
extern NJS_OBJECT dat_0c9210e4;
extern NJS_OBJECT dat_0c921548;
extern NJS_OBJECT dat_0c9219ac;
extern NJS_OBJECT dat_0c922fcc;
extern NJS_OBJECT dat_0c9245ec;
extern NJS_OBJECT dat_0c924a50;
extern NJS_OBJECT dat_0c926258;
extern NJS_OBJECT dat_0c9272e0;
extern NJS_OBJECT dat_0c927314;
extern NJS_OBJECT dat_0c927348;
extern NJS_OBJECT dat_0c92737c;
extern NJS_OBJECT dat_0c9273b0;
extern NJS_OBJECT dat_0c9273e4;
extern NJS_OBJECT dat_0c927418;
extern NJS_OBJECT dat_0c92744c;
extern NJS_OBJECT dat_0c927480;
extern NJS_OBJECT dat_0c9274b4;
extern NJS_OBJECT dat_0c9274e8;
extern NJS_OBJECT dat_0c92751c;
extern NJS_OBJECT dat_0c927550;
extern NJS_OBJECT dat_0c927584;
extern NJS_OBJECT dat_0c9275b8;
extern NJS_OBJECT dat_0c9275ec;
extern NJS_OBJECT dat_0c927620;
extern NJS_OBJECT dat_0c927654;
extern NJS_OBJECT dat_0c927688;
extern NJS_OBJECT dat_0c9276bc;
extern NJS_OBJECT dat_0c9276f0;
extern NJS_OBJECT dat_0c927724;
extern NJS_OBJECT dat_0c927758;
extern NJS_OBJECT dat_0c92778c;
extern NJS_OBJECT dat_0c9277c0;
extern NJS_OBJECT dat_0c9277f4;
extern NJS_OBJECT dat_0c927828;
extern NJS_OBJECT dat_0c92785c;
extern NJS_OBJECT dat_0c927890;
extern NJS_OBJECT dat_0c9278c4;
extern NJS_OBJECT dat_0c9278f8;
extern NJS_OBJECT dat_0c92792c;
extern NJS_OBJECT dat_0c927960;
extern NJS_OBJECT dat_0c927994;
extern NJS_OBJECT dat_0c9279c8;
extern NJS_OBJECT dat_0c9279fc;
extern NJS_OBJECT dat_0c927a30;
extern NJS_OBJECT dat_0c927a64;
extern NJS_OBJECT dat_0c927a98;
extern NJS_OBJECT dat_0c927acc;
extern NJS_OBJECT dat_0c927b00;
extern NJS_OBJECT dat_0c927b34;
extern NJS_OBJECT dat_0c927b68;
extern NJS_OBJECT dat_0c927b9c;
extern NJS_OBJECT dat_0c927bd0;
extern NJS_OBJECT dat_0c927c04;
extern NJS_OBJECT dat_0c927c38;
extern NJS_OBJECT dat_0c927c6c;
extern NJS_OBJECT dat_0c927ca0;
extern NJS_OBJECT dat_0c927cd4;
extern NJS_OBJECT dat_0c927d08;
extern NJS_OBJECT dat_0c927d3c;
extern NJS_OBJECT dat_0c927d70;
extern NJS_OBJECT dat_0c927da4;
extern NJS_OBJECT dat_0c927dd8;
extern NJS_OBJECT dat_0c927e0c;
extern NJS_OBJECT dat_0c927e40;
extern NJS_OBJECT dat_0c927e74;
extern NJS_OBJECT dat_0c927ea8;
extern NJS_OBJECT dat_0c927edc;
extern NJS_OBJECT dat_0c927f10;
extern NJS_OBJECT dat_0c927f44;
extern NJS_OBJECT dat_0c927f78;
extern NJS_OBJECT dat_0c927fac;
extern NJS_OBJECT dat_0c927fe0;
extern NJS_OBJECT dat_0c928014;
extern NJS_OBJECT dat_0c928048;
extern NJS_OBJECT dat_0c92807c;
extern NJS_OBJECT dat_0c9280b0;
extern NJS_OBJECT dat_0c9280e4;
extern NJS_OBJECT dat_0c928118;
extern NJS_OBJECT dat_0c92814c;
extern NJS_OBJECT dat_0c928180;
extern NJS_OBJECT dat_0c9281b4;
extern NJS_OBJECT dat_0c9281e8;
extern NJS_OBJECT dat_0c92821c;
extern NJS_OBJECT dat_0c928250;
extern NJS_OBJECT dat_0c928284;
extern NJS_OBJECT dat_0c9282b8;
extern NJS_OBJECT dat_0c9282ec;
extern NJS_OBJECT dat_0c928320;
extern NJS_OBJECT dat_0c928354;
extern NJS_OBJECT dat_0c928388;
extern NJS_OBJECT dat_0c9283bc;
extern NJS_OBJECT dat_0c9283f0;
extern NJS_OBJECT dat_0c928424;
extern NJS_OBJECT dat_0c928458;
extern NJS_OBJECT dat_0c92848c;
extern NJS_OBJECT dat_0c9284c0;
extern NJS_OBJECT dat_0c9284f4;
extern NJS_OBJECT dat_0c928528;
extern NJS_OBJECT dat_0c92855c;
extern NJS_OBJECT dat_0c928590;
extern NJS_OBJECT dat_0c9285c4;
extern NJS_OBJECT dat_0c9285f8;
extern NJS_OBJECT dat_0c92862c;
extern NJS_OBJECT dat_0c928660;
extern NJS_OBJECT dat_0c928694;
extern NJS_OBJECT dat_0c9286c8;
extern NJS_OBJECT dat_0c9286fc;
extern NJS_OBJECT dat_0c928730;
extern NJS_OBJECT dat_0c928764;
extern NJS_OBJECT dat_0c928798;
extern NJS_OBJECT dat_0c9287cc;
extern NJS_OBJECT dat_0c928800;
extern NJS_OBJECT dat_0c928834;
extern NJS_OBJECT dat_0c928868;
extern NJS_OBJECT dat_0c92889c;
extern NJS_OBJECT dat_0c9288d0;
extern NJS_OBJECT dat_0c928904;
extern NJS_OBJECT dat_0c928938;
extern NJS_OBJECT dat_0c92896c;
extern NJS_OBJECT dat_0c9289a0;
extern NJS_OBJECT dat_0c9289d4;
extern NJS_OBJECT dat_0c928a08;
extern NJS_OBJECT dat_0c928a3c;
extern NJS_OBJECT dat_0c928a70;
extern NJS_OBJECT dat_0c928aa4;
extern NJS_OBJECT dat_0c928ad8;
extern NJS_OBJECT dat_0c928b0c;
extern NJS_OBJECT dat_0c928b40;
extern NJS_OBJECT dat_0c928b74;
extern NJS_OBJECT dat_0c928ba8;
extern NJS_OBJECT dat_0c928bdc;
extern NJS_OBJECT dat_0c928c10;
extern NJS_OBJECT dat_0c928c44;
extern NJS_OBJECT dat_0c928c78;
extern NJS_OBJECT dat_0c928cac;
extern NJS_OBJECT dat_0c928ce0;
extern NJS_OBJECT dat_0c928d14;
extern NJS_OBJECT dat_0c928d48;
extern NJS_OBJECT dat_0c928d7c;
extern NJS_OBJECT dat_0c928db0;
extern NJS_OBJECT dat_0c928de4;
extern NJS_OBJECT dat_0c928e18;
extern NJS_OBJECT dat_0c928e4c;
extern NJS_OBJECT dat_0c928e80;
extern NJS_OBJECT dat_0c928eb4;
extern NJS_OBJECT dat_0c928ee8;
extern NJS_OBJECT dat_0c928f1c;
extern NJS_OBJECT dat_0c928f50;
extern NJS_OBJECT dat_0c928f84;
extern NJS_OBJECT dat_0c928fb8;
extern NJS_OBJECT dat_0c928fec;
extern NJS_OBJECT dat_0c929020;
extern NJS_OBJECT dat_0c929054;
extern NJS_OBJECT dat_0c929088;
extern NJS_OBJECT dat_0c9290bc;
extern NJS_OBJECT dat_0c9290f0;
extern NJS_OBJECT dat_0c929124;
extern NJS_OBJECT dat_0c929158;
extern NJS_OBJECT dat_0c92918c;
extern NJS_OBJECT dat_0c9291c0;
extern NJS_OBJECT dat_0c9291f4;
extern NJS_OBJECT dat_0c929228;
extern NJS_OBJECT dat_0c92925c;
extern NJS_OBJECT dat_0c929290;
extern NJS_OBJECT dat_0c9292c4;

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 6
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 5
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 5
#define TEXT_8_LENGTH 6
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 5
#define TEXT_11_LENGTH 6
#define TEXT_12_LENGTH 5
#include "minicart/land0/text.inc"

static NJS_TEXNAME cart_land0_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
static NJS_TEXLIST cart_land0_texlist = {
#include "minicart/land0/cart_land0_texlist.inc"
};

#pragma section HEAD
/* the pvm's name, an array because the table's generated initializer can
 * name only a symbol. under section HEAD, SHC puts constants in CHEAD, and
 * this is the file's only one. it starts CHEAD on an 8-byte boundary, as a
 * string literal would */
static const char cart_land0_pvm[12] = {
#include "minicart/land0/cart_land0_pvm.inc"
};
/* the table starts straight after the last entry, at 0x0c901a54, 4
 * bytes past an 8-byte boundary. R32 is the SHC release that compiles this
 * file. it starts each object's data on such a boundary, so the entries and
 * the table share an object */
static struct land_entry cart_land0_entries[171] = {
#include "minicart/land0/cart_land0_entries.inc"
};
struct land_table cart_land0 = {
#include "minicart/land0/cart_land0.inc"
};
#pragma section
