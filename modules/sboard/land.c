/* Sand Hill's land: its table, its entries and its texlist. the texlist's
 * textures load by name from the pvm the table names */
#include <ninja.h>

#include "collision/world.h"
#include "sboard/land.h"

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c90b5c4;
extern NJS_OBJECT dat_0c90c254;
extern NJS_OBJECT dat_0c90cee4;
extern NJS_OBJECT dat_0c90dbcc;
extern NJS_OBJECT dat_0c90e85c;
extern NJS_OBJECT dat_0c90f0a8;
extern NJS_OBJECT dat_0c90fd38;
extern NJS_OBJECT dat_0c910a20;
extern NJS_OBJECT dat_0c911684;
extern NJS_OBJECT dat_0c912314;
extern NJS_OBJECT dat_0c912fa4;
extern NJS_OBJECT dat_0c9135cc;
extern NJS_OBJECT dat_0c91425c;
extern NJS_OBJECT dat_0c9148b0;
extern NJS_OBJECT dat_0c9150d0;
extern NJS_OBJECT dat_0c915d60;
extern NJS_OBJECT dat_0c9169f0;
extern NJS_OBJECT dat_0c917680;
extern NJS_OBJECT dat_0c917ca8;
extern NJS_OBJECT dat_0c918740;
extern NJS_OBJECT dat_0c919044;
extern NJS_OBJECT dat_0c919810;
extern NJS_OBJECT dat_0c91a330;
extern NJS_OBJECT dat_0c91b150;
extern NJS_OBJECT dat_0c91be0c;
extern NJS_OBJECT dat_0c91cce0;
extern NJS_OBJECT dat_0c91dfa4;
extern NJS_OBJECT dat_0c91ebec;
extern NJS_OBJECT dat_0c91f834;
extern NJS_OBJECT dat_0c920660;
extern NJS_OBJECT dat_0c921048;
extern NJS_OBJECT dat_0c921d34;
extern NJS_OBJECT dat_0c922168;
extern NJS_OBJECT dat_0c922da8;
extern NJS_OBJECT dat_0c923964;
extern NJS_OBJECT dat_0c924718;
extern NJS_OBJECT dat_0c925524;
extern NJS_OBJECT dat_0c926860;
extern NJS_OBJECT dat_0c927b28;
extern NJS_OBJECT dat_0c928df0;
extern NJS_OBJECT dat_0c9298c0;
extern NJS_OBJECT dat_0c92a878;
extern NJS_OBJECT dat_0c92b374;
extern NJS_OBJECT dat_0c92c2f0;
extern NJS_OBJECT dat_0c92c674;
extern NJS_OBJECT dat_0c92d91c;
extern NJS_OBJECT dat_0c92e230;
extern NJS_OBJECT dat_0c92f660;
extern NJS_OBJECT dat_0c930c10;
extern NJS_OBJECT dat_0c931d38;
extern NJS_OBJECT dat_0c932a68;
extern NJS_OBJECT dat_0c933ab0;
extern NJS_OBJECT dat_0c9348fc;
extern NJS_OBJECT dat_0c934cb0;
extern NJS_OBJECT dat_0c935144;
extern NJS_OBJECT dat_0c93527c;
extern NJS_OBJECT dat_0c935760;
extern NJS_OBJECT dat_0c935a5c;
extern NJS_OBJECT dat_0c9375c8;
extern NJS_OBJECT dat_0c938f64;
extern NJS_OBJECT dat_0c939928;
extern NJS_OBJECT dat_0c93a2ec;
extern NJS_OBJECT dat_0c93b0a0;
extern NJS_OBJECT dat_0c93ba64;
extern NJS_OBJECT dat_0c93bfe0;
extern NJS_OBJECT dat_0c93cd94;
extern NJS_OBJECT dat_0c93ebe0;
extern NJS_OBJECT dat_0c93fec8;
extern NJS_OBJECT dat_0c94064c;
extern NJS_OBJECT dat_0c940e18;
extern NJS_OBJECT dat_0c94164c;
extern NJS_OBJECT dat_0c942ba0;
extern NJS_OBJECT dat_0c944034;
extern NJS_OBJECT dat_0c944c44;
extern NJS_OBJECT dat_0c945238;
extern NJS_OBJECT dat_0c945534;
extern NJS_OBJECT dat_0c946620;
extern NJS_OBJECT dat_0c947400;
extern NJS_OBJECT dat_0c948000;
extern NJS_OBJECT dat_0c948acc;
extern NJS_OBJECT dat_0c949404;
extern NJS_OBJECT dat_0c949848;
extern NJS_OBJECT dat_0c949abc;
extern NJS_OBJECT dat_0c94a5ac;
extern NJS_OBJECT dat_0c94ad9c;
extern NJS_OBJECT dat_0c94b560;
extern NJS_OBJECT dat_0c94c3d0;
extern NJS_OBJECT dat_0c94d240;
extern NJS_OBJECT dat_0c94dee4;
extern NJS_OBJECT dat_0c94e488;
extern NJS_OBJECT dat_0c94ed70;
extern NJS_OBJECT dat_0c94f818;
extern NJS_OBJECT dat_0c9501e0;
extern NJS_OBJECT dat_0c950c1c;
extern NJS_OBJECT dat_0c951548;
extern NJS_OBJECT dat_0c951cac;
extern NJS_OBJECT dat_0c9524d8;
extern NJS_OBJECT dat_0c952d14;
extern NJS_OBJECT dat_0c9533ec;
extern NJS_OBJECT dat_0c953b88;
extern NJS_OBJECT dat_0c95434c;
extern NJS_OBJECT dat_0c954b14;
extern NJS_OBJECT dat_0c954d48;
extern NJS_OBJECT dat_0c95558c;
extern NJS_OBJECT dat_0c955cf8;
extern NJS_OBJECT dat_0c956490;
extern NJS_OBJECT dat_0c956c28;
extern NJS_OBJECT dat_0c957418;
extern NJS_OBJECT dat_0c957bdc;
extern NJS_OBJECT dat_0c958374;
extern NJS_OBJECT dat_0c9585fc;
extern NJS_OBJECT dat_0c958dc0;
extern NJS_OBJECT dat_0c95952c;
extern NJS_OBJECT dat_0c959d2c;
extern NJS_OBJECT dat_0c95a594;
extern NJS_OBJECT dat_0c95ae84;
extern NJS_OBJECT dat_0c95b690;
extern NJS_OBJECT dat_0c95be2c;
extern NJS_OBJECT dat_0c95c148;
extern NJS_OBJECT dat_0c95c9b4;
extern NJS_OBJECT dat_0c95d14c;
extern NJS_OBJECT dat_0c95d990;
extern NJS_OBJECT dat_0c95e234;
extern NJS_OBJECT dat_0c95ea14;
extern NJS_OBJECT dat_0c95f180;
extern NJS_OBJECT dat_0c95f858;
extern NJS_OBJECT dat_0c95fab4;
extern NJS_OBJECT dat_0c960320;
extern NJS_OBJECT dat_0c960b08;
extern NJS_OBJECT dat_0c961338;
extern NJS_OBJECT dat_0c961b18;
extern NJS_OBJECT dat_0c962344;
extern NJS_OBJECT dat_0c962bac;
extern NJS_OBJECT dat_0c963318;
extern NJS_OBJECT dat_0c9635f8;
extern NJS_OBJECT dat_0c9639c8;
extern NJS_OBJECT dat_0c963f54;
extern NJS_OBJECT dat_0c9644e0;
extern NJS_OBJECT dat_0c964a48;
extern NJS_OBJECT dat_0c965010;
extern NJS_OBJECT dat_0c965588;
extern NJS_OBJECT dat_0c965a24;
extern NJS_OBJECT dat_0c965b88;
extern NJS_OBJECT dat_0c966064;
extern NJS_OBJECT dat_0c9667cc;
extern NJS_OBJECT dat_0c966cd4;
extern NJS_OBJECT dat_0c967414;
extern NJS_OBJECT dat_0c9679f8;
extern NJS_OBJECT dat_0c967e94;
extern NJS_OBJECT dat_0c9684a4;
extern NJS_OBJECT dat_0c968940;
extern NJS_OBJECT dat_0c968ddc;
extern NJS_OBJECT dat_0c9693ec;
extern NJS_OBJECT dat_0c969888;
extern NJS_OBJECT dat_0c969e6c;
extern NJS_OBJECT dat_0c969ea0;
extern NJS_OBJECT dat_0c969ed4;
extern NJS_OBJECT dat_0c969f08;
extern NJS_OBJECT dat_0c969f3c;
extern NJS_OBJECT dat_0c969f70;
extern NJS_OBJECT dat_0c969fa4;
extern NJS_OBJECT dat_0c969fd8;
extern NJS_OBJECT dat_0c96a00c;
extern NJS_OBJECT dat_0c96a040;
extern NJS_OBJECT dat_0c96a074;
extern NJS_OBJECT dat_0c96a0a8;
extern NJS_OBJECT dat_0c96a0dc;
extern NJS_OBJECT dat_0c96a110;
extern NJS_OBJECT dat_0c96a144;
extern NJS_OBJECT dat_0c96a178;
extern NJS_OBJECT dat_0c96a1ac;
extern NJS_OBJECT dat_0c96a1e0;
extern NJS_OBJECT dat_0c96a214;
extern NJS_OBJECT dat_0c96a248;
extern NJS_OBJECT dat_0c96a27c;
extern NJS_OBJECT dat_0c96a2b0;
extern NJS_OBJECT dat_0c96a2e4;
extern NJS_OBJECT dat_0c96a318;
extern NJS_OBJECT dat_0c96a34c;
extern NJS_OBJECT dat_0c96a380;
extern NJS_OBJECT dat_0c96a3b4;
extern NJS_OBJECT dat_0c96a3e8;
extern NJS_OBJECT dat_0c96a41c;
extern NJS_OBJECT dat_0c96a450;
extern NJS_OBJECT dat_0c96a484;
extern NJS_OBJECT dat_0c96a4b8;
extern NJS_OBJECT dat_0c96a4ec;
extern NJS_OBJECT dat_0c96a520;
extern NJS_OBJECT dat_0c96a554;
extern NJS_OBJECT dat_0c96a588;
extern NJS_OBJECT dat_0c96a5bc;
extern NJS_OBJECT dat_0c96a5f0;
extern NJS_OBJECT dat_0c96a624;
extern NJS_OBJECT dat_0c96a658;
extern NJS_OBJECT dat_0c96a68c;
extern NJS_OBJECT dat_0c96a6c0;
extern NJS_OBJECT dat_0c96a6f4;
extern NJS_OBJECT dat_0c96a728;
extern NJS_OBJECT dat_0c96a75c;
extern NJS_OBJECT dat_0c96a790;
extern NJS_OBJECT dat_0c96a7c4;
extern NJS_OBJECT dat_0c96a7f8;
extern NJS_OBJECT dat_0c96a82c;
extern NJS_OBJECT dat_0c96a860;
extern NJS_OBJECT dat_0c96a894;
extern NJS_OBJECT dat_0c96a8c8;
extern NJS_OBJECT dat_0c96a8fc;
extern NJS_OBJECT dat_0c96a930;
extern NJS_OBJECT dat_0c96a964;
extern NJS_OBJECT dat_0c96a998;
extern NJS_OBJECT dat_0c96a9cc;
extern NJS_OBJECT dat_0c96aa00;
extern NJS_OBJECT dat_0c96aa34;
extern NJS_OBJECT dat_0c96aa68;
extern NJS_OBJECT dat_0c96aa9c;
extern NJS_OBJECT dat_0c96aad0;
extern NJS_OBJECT dat_0c96ab04;
extern NJS_OBJECT dat_0c96ab38;
extern NJS_OBJECT dat_0c96ab6c;
extern NJS_OBJECT dat_0c96aba0;
extern NJS_OBJECT dat_0c96abd4;
extern NJS_OBJECT dat_0c96ac08;
extern NJS_OBJECT dat_0c96ac3c;
extern NJS_OBJECT dat_0c96ac70;
extern NJS_OBJECT dat_0c96aca4;
extern NJS_OBJECT dat_0c96acd8;
extern NJS_OBJECT dat_0c96ad0c;
extern NJS_OBJECT dat_0c96ad40;
extern NJS_OBJECT dat_0c96ad74;
extern NJS_OBJECT dat_0c96ada8;
extern NJS_OBJECT dat_0c96addc;
extern NJS_OBJECT dat_0c96ae10;
extern NJS_OBJECT dat_0c96ae44;
extern NJS_OBJECT dat_0c96ae78;
extern NJS_OBJECT dat_0c96aeac;
extern NJS_OBJECT dat_0c96aee0;
extern NJS_OBJECT dat_0c96af14;
extern NJS_OBJECT dat_0c96af48;
extern NJS_OBJECT dat_0c96af7c;
extern NJS_OBJECT dat_0c96afb0;
extern NJS_OBJECT dat_0c96afe4;
extern NJS_OBJECT dat_0c96b018;
extern NJS_OBJECT dat_0c96b04c;
extern NJS_OBJECT dat_0c96b080;
extern NJS_OBJECT dat_0c96b0b4;
extern NJS_OBJECT dat_0c96b0e8;
extern NJS_OBJECT dat_0c96b11c;
extern NJS_OBJECT dat_0c96b150;
extern NJS_OBJECT dat_0c96b184;
extern NJS_OBJECT dat_0c96b1b8;
extern NJS_OBJECT dat_0c96b1ec;
extern NJS_OBJECT dat_0c96b220;
extern NJS_OBJECT dat_0c96b254;
extern NJS_OBJECT dat_0c96b288;
extern NJS_OBJECT dat_0c96b2bc;
extern NJS_OBJECT dat_0c96b2f0;
extern NJS_OBJECT dat_0c96b324;
extern NJS_OBJECT dat_0c96b358;
extern NJS_OBJECT dat_0c96b38c;
extern NJS_OBJECT dat_0c96b3c0;
extern NJS_OBJECT dat_0c96b3f4;
extern NJS_OBJECT dat_0c96b428;
extern NJS_OBJECT dat_0c96b45c;
extern NJS_OBJECT dat_0c96b490;
extern NJS_OBJECT dat_0c96b4c4;
extern NJS_OBJECT dat_0c96b4f8;
extern NJS_OBJECT dat_0c96b52c;
extern NJS_OBJECT dat_0c96b560;
extern NJS_OBJECT dat_0c96b594;
extern NJS_OBJECT dat_0c96b5c8;
extern NJS_OBJECT dat_0c96b5fc;
extern NJS_OBJECT dat_0c96b630;
extern NJS_OBJECT dat_0c96b664;
extern NJS_OBJECT dat_0c96b698;
extern NJS_OBJECT dat_0c96b6cc;
extern NJS_OBJECT dat_0c96b700;
extern NJS_OBJECT dat_0c96b734;
extern NJS_OBJECT dat_0c96b768;
extern NJS_OBJECT dat_0c96b79c;
extern NJS_OBJECT dat_0c96b7d0;
extern NJS_OBJECT dat_0c96b804;
extern NJS_OBJECT dat_0c96b838;
extern NJS_OBJECT dat_0c96b86c;
extern NJS_OBJECT dat_0c96b8a0;
extern NJS_OBJECT dat_0c96b8d4;
extern NJS_OBJECT dat_0c96b908;
extern NJS_OBJECT dat_0c96b93c;
extern NJS_OBJECT dat_0c96b970;
extern NJS_OBJECT dat_0c96b9a4;
extern NJS_OBJECT dat_0c96b9d8;
extern NJS_OBJECT dat_0c96ba0c;
extern NJS_OBJECT dat_0c96ba40;
extern NJS_OBJECT dat_0c96ba74;
extern NJS_OBJECT dat_0c96baa8;
extern NJS_OBJECT dat_0c96badc;
extern NJS_OBJECT dat_0c96bb10;
extern NJS_OBJECT dat_0c96bb44;
extern NJS_OBJECT dat_0c96bb78;
extern NJS_OBJECT dat_0c96bbac;
extern NJS_OBJECT dat_0c96bbe0;
extern NJS_OBJECT dat_0c96bc14;
extern NJS_OBJECT dat_0c96bc48;
extern NJS_OBJECT dat_0c96bc7c;
extern NJS_OBJECT dat_0c96bcb0;
extern NJS_OBJECT dat_0c96bce4;
extern NJS_OBJECT dat_0c96bd18;
extern NJS_OBJECT dat_0c96bd4c;
extern NJS_OBJECT dat_0c96bd80;
extern NJS_OBJECT dat_0c96bdb4;
extern NJS_OBJECT dat_0c96bde8;
extern NJS_OBJECT dat_0c96be1c;
extern NJS_OBJECT dat_0c96be50;
extern NJS_OBJECT dat_0c96be84;
extern NJS_OBJECT dat_0c96beb8;
extern NJS_OBJECT dat_0c96beec;

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 3
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 9
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 9
#define TEXT_25_LENGTH 9
#define TEXT_26_LENGTH 9
#define TEXT_27_LENGTH 9
#define TEXT_28_LENGTH 9
#define TEXT_29_LENGTH 9
#define TEXT_30_LENGTH 9
#define TEXT_31_LENGTH 4
#define TEXT_32_LENGTH 5
#define TEXT_33_LENGTH 5
#define TEXT_34_LENGTH 5
#define TEXT_35_LENGTH 5
#define TEXT_36_LENGTH 5
#define TEXT_37_LENGTH 6
#define TEXT_38_LENGTH 5
#define TEXT_39_LENGTH 5
#define TEXT_40_LENGTH 5
#define TEXT_41_LENGTH 5
#define TEXT_42_LENGTH 11
#define TEXT_43_LENGTH 11
#define TEXT_44_LENGTH 11
#define TEXT_45_LENGTH 11
#define TEXT_46_LENGTH 11
#define TEXT_47_LENGTH 11
#define TEXT_48_LENGTH 11
#define TEXT_49_LENGTH 11
#define TEXT_50_LENGTH 11
#include "sboard/land/text.inc"

static NJS_TEXNAME sunaboard_land_textures[50] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0},
    {TEXT_49, 0, 0}, {TEXT_50, 0, 0},
};
static NJS_TEXLIST sunaboard_land_texlist = {
#include "sboard/land/sunaboard_land_texlist.inc"
};

#pragma section HEAD
/* the pvm's name, as a named array: the table's generated initializer can
 * point only at a symbol. #pragma section HEAD puts the constants below it
 * in CHEAD, HEAD's constant section. the name is the only one there. SHC
 * aligns the array to 1, and a string literal to 4, but either starts
 * CHEAD, at 0x0c900120 */
static const char sunaboard_land_pvm[10] = {
#include "sboard/land/sunaboard_land_pvm.inc"
};
/* the table, sunaboard_land, starts at 0x0c902df4, right after the last
 * entry and off a multiple of 8. SHC 5.0 R32 builds this module, and it
 * starts each object's data on a multiple of 8. the entries and the table
 * must then share one object */
static struct land_entry sunaboard_land_entries[315] = {
#include "sboard/land/sunaboard_land_entries.inc"
};
struct land_table sunaboard_land = {
#include "sboard/land/sunaboard_land.inc"
};
#pragma section
