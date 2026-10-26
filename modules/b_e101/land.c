/* the stage's land: its table and entries, its textures' names and
 * texlist, and its pvm's name */
#include <ninja.h>

#include "b_e101/land.h"
#include "collision/world.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 13
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 12
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 10
#define TEXT_21_LENGTH 9
#define TEXT_22_LENGTH 10
#define TEXT_23_LENGTH 10
#define TEXT_24_LENGTH 10
#define TEXT_25_LENGTH 13
#define TEXT_26_LENGTH 16
#define TEXT_27_LENGTH 13
#define TEXT_28_LENGTH 11
#define TEXT_29_LENGTH 13
#define TEXT_30_LENGTH 13
#define TEXT_31_LENGTH 13
#define TEXT_32_LENGTH 13
#define TEXT_33_LENGTH 13
#define TEXT_34_LENGTH 13
#define TEXT_35_LENGTH 13
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 11
#define TEXT_38_LENGTH 13
#define TEXT_39_LENGTH 10
#define TEXT_40_LENGTH 17
#define TEXT_41_LENGTH 12
#define TEXT_42_LENGTH 9
#define TEXT_43_LENGTH 12
#define TEXT_44_LENGTH 6
#define TEXT_45_LENGTH 12
#define TEXT_46_LENGTH 13
#define TEXT_47_LENGTH 13
#define TEXT_48_LENGTH 13
#define TEXT_49_LENGTH 12
#define TEXT_50_LENGTH 13
#define TEXT_51_LENGTH 13
#define TEXT_52_LENGTH 12
#define TEXT_53_LENGTH 13
#define TEXT_54_LENGTH 15
#define TEXT_55_LENGTH 13
#define TEXT_56_LENGTH 12
#define TEXT_57_LENGTH 12
#define TEXT_58_LENGTH 12
#define TEXT_59_LENGTH 13
#define TEXT_60_LENGTH 14
#define TEXT_61_LENGTH 13
#define TEXT_62_LENGTH 13
#define TEXT_63_LENGTH 12
#define TEXT_64_LENGTH 13
#define TEXT_65_LENGTH 13
#define TEXT_66_LENGTH 13
#define TEXT_67_LENGTH 13
#define TEXT_68_LENGTH 14
#define TEXT_69_LENGTH 13
#define TEXT_70_LENGTH 13
#define TEXT_71_LENGTH 12
#define TEXT_72_LENGTH 8
#define TEXT_73_LENGTH 8
#define TEXT_74_LENGTH 9
#define TEXT_75_LENGTH 8
#define TEXT_76_LENGTH 9
#define TEXT_77_LENGTH 8
#include "b_e101/land/text.inc"

extern struct land_motion e101_land_motions[];

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c904ed0;
extern NJS_OBJECT dat_0c9052ec;
extern NJS_OBJECT dat_0c9055fc;
extern NJS_OBJECT dat_0c906e68;
extern NJS_OBJECT dat_0c907958;
extern NJS_OBJECT dat_0c908098;
extern NJS_OBJECT dat_0c90843c;
extern NJS_OBJECT dat_0c908d00;
extern NJS_OBJECT dat_0c90959c;
extern NJS_OBJECT dat_0c909840;
extern NJS_OBJECT dat_0c90aa60;
extern NJS_OBJECT dat_0c90b244;
extern NJS_OBJECT dat_0c90c424;
extern NJS_OBJECT dat_0c90cab0;
extern NJS_OBJECT dat_0c90d024;
extern NJS_OBJECT dat_0c90d598;
extern NJS_OBJECT dat_0c90db0c;
extern NJS_OBJECT dat_0c90e080;
extern NJS_OBJECT dat_0c90e2ec;
extern NJS_OBJECT dat_0c90e6e0;
extern NJS_OBJECT dat_0c90f55c;
extern NJS_OBJECT dat_0c90f8a8;
extern NJS_OBJECT dat_0c90fd44;
extern NJS_OBJECT dat_0c910448;
extern NJS_OBJECT dat_0c9107b0;
extern NJS_OBJECT dat_0c910a58;
extern NJS_OBJECT dat_0c910cd4;
extern NJS_OBJECT dat_0c911298;
extern NJS_OBJECT dat_0c911730;
extern NJS_OBJECT dat_0c9118e0;
extern NJS_OBJECT dat_0c911b2c;
extern NJS_OBJECT dat_0c911d78;
extern NJS_OBJECT dat_0c911fc4;
extern NJS_OBJECT dat_0c912210;
extern NJS_OBJECT dat_0c9125cc;
extern NJS_OBJECT dat_0c912a9c;
extern NJS_OBJECT dat_0c9131b0;
extern NJS_OBJECT dat_0c913344;
extern NJS_OBJECT dat_0c913734;
extern NJS_OBJECT dat_0c913ad4;
extern NJS_OBJECT dat_0c914020;
extern NJS_OBJECT dat_0c914464;
extern NJS_OBJECT dat_0c914b04;
extern NJS_OBJECT dat_0c914c8c;
extern NJS_OBJECT dat_0c914dd8;
extern NJS_OBJECT dat_0c914f78;
extern NJS_OBJECT dat_0c915118;
extern NJS_OBJECT dat_0c9152b8;
extern NJS_OBJECT dat_0c915b4c;
extern NJS_OBJECT dat_0c915dcc;
extern NJS_OBJECT dat_0c916c58;
extern NJS_OBJECT dat_0c916c8c;
extern NJS_OBJECT dat_0c916cc0;
extern NJS_OBJECT dat_0c916cf4;
extern NJS_OBJECT dat_0c916d28;
extern NJS_OBJECT dat_0c916d5c;
extern NJS_OBJECT dat_0c916d90;
extern NJS_OBJECT dat_0c916dc4;
extern NJS_OBJECT dat_0c916df8;
extern NJS_OBJECT dat_0c916e2c;
extern NJS_OBJECT dat_0c916e60;
extern NJS_OBJECT dat_0c916e94;
extern NJS_OBJECT dat_0c916ec8;
extern NJS_OBJECT dat_0c916efc;
extern NJS_OBJECT dat_0c916f30;
extern NJS_OBJECT dat_0c916f64;
extern NJS_OBJECT dat_0c916f98;
extern NJS_OBJECT dat_0c916fcc;
extern NJS_OBJECT dat_0c917000;
extern NJS_OBJECT dat_0c917034;
extern NJS_OBJECT dat_0c917068;
extern NJS_OBJECT dat_0c91709c;
extern NJS_OBJECT dat_0c9170d0;
extern NJS_OBJECT dat_0c917104;
extern NJS_OBJECT dat_0c917138;
extern NJS_OBJECT dat_0c91716c;
extern NJS_OBJECT dat_0c9171a0;
extern NJS_OBJECT dat_0c9171d4;
extern NJS_OBJECT dat_0c917208;
extern NJS_OBJECT dat_0c91723c;
extern NJS_OBJECT dat_0c917270;
extern NJS_OBJECT dat_0c9172a4;
extern NJS_OBJECT dat_0c9172d8;
extern NJS_OBJECT dat_0c91730c;
extern NJS_OBJECT dat_0c917340;
extern NJS_OBJECT dat_0c917374;
extern NJS_OBJECT dat_0c9173a8;
extern NJS_OBJECT dat_0c9173dc;
extern NJS_OBJECT dat_0c917410;
extern NJS_OBJECT dat_0c917444;
extern NJS_OBJECT dat_0c917478;
extern NJS_OBJECT dat_0c9174ac;
extern NJS_OBJECT dat_0c9174e0;
extern NJS_OBJECT dat_0c917514;
extern NJS_OBJECT dat_0c917548;
extern NJS_OBJECT dat_0c91757c;
extern NJS_OBJECT dat_0c9175b0;
extern NJS_OBJECT dat_0c9175e4;
extern NJS_OBJECT dat_0c917618;
extern NJS_OBJECT dat_0c91764c;
extern NJS_OBJECT dat_0c917680;
extern NJS_OBJECT dat_0c9176b4;
extern NJS_OBJECT dat_0c9176e8;
extern NJS_OBJECT dat_0c91771c;
extern NJS_OBJECT dat_0c917750;
extern NJS_OBJECT dat_0c917784;
extern NJS_OBJECT dat_0c9177b8;
extern NJS_OBJECT dat_0c9177ec;
extern NJS_OBJECT dat_0c917820;
extern NJS_OBJECT dat_0c917854;
extern NJS_OBJECT dat_0c917888;
extern NJS_OBJECT dat_0c9178bc;
extern NJS_OBJECT dat_0c9178f0;
extern NJS_OBJECT dat_0c917924;
extern NJS_OBJECT dat_0c917958;
extern NJS_OBJECT dat_0c91798c;
extern NJS_OBJECT dat_0c9179c0;
extern NJS_OBJECT dat_0c9179f4;
extern NJS_OBJECT dat_0c917a28;
extern NJS_OBJECT dat_0c917a5c;
extern NJS_OBJECT dat_0c917a90;
extern NJS_OBJECT dat_0c917ac4;
extern NJS_OBJECT dat_0c917af8;
extern NJS_OBJECT dat_0c917b2c;
extern NJS_OBJECT dat_0c917b60;
extern NJS_OBJECT dat_0c917b94;
extern NJS_OBJECT dat_0c917bc8;
extern NJS_OBJECT dat_0c917bfc;
extern NJS_OBJECT dat_0c917c30;
extern NJS_OBJECT dat_0c917c64;
extern NJS_OBJECT dat_0c917c98;
extern NJS_OBJECT dat_0c917ccc;
extern NJS_OBJECT dat_0c917d00;
extern NJS_OBJECT dat_0c917d34;
extern NJS_OBJECT dat_0c917d68;
extern NJS_OBJECT dat_0c917d9c;
extern NJS_OBJECT dat_0c917dd0;
extern NJS_OBJECT dat_0c917e04;
extern NJS_OBJECT dat_0c917e38;
extern NJS_OBJECT dat_0c917e6c;
extern NJS_OBJECT dat_0c917ea0;
extern NJS_OBJECT dat_0c917ed4;
extern NJS_OBJECT dat_0c917f08;
extern NJS_OBJECT dat_0c917f3c;
extern NJS_OBJECT dat_0c917f70;
extern NJS_OBJECT dat_0c917fa4;
extern NJS_OBJECT dat_0c917fd8;
extern NJS_OBJECT dat_0c91800c;
extern NJS_OBJECT dat_0c918040;
extern NJS_OBJECT dat_0c918074;
extern NJS_OBJECT dat_0c9180a8;
extern NJS_OBJECT dat_0c9180dc;
extern NJS_OBJECT dat_0c918110;
extern NJS_OBJECT dat_0c918144;
extern NJS_OBJECT dat_0c918178;
extern NJS_OBJECT dat_0c9181ac;
extern NJS_OBJECT dat_0c9181e0;
extern NJS_OBJECT dat_0c918214;
extern NJS_OBJECT dat_0c918248;

static NJS_TEXNAME e101_land_textures[77] = {
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
    {TEXT_49, 0, 0}, {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0},
    {TEXT_53, 0, 0}, {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0},
    {TEXT_57, 0, 0}, {TEXT_58, 0, 0}, {TEXT_59, 0, 0}, {TEXT_60, 0, 0},
    {TEXT_61, 0, 0}, {TEXT_62, 0, 0}, {TEXT_63, 0, 0}, {TEXT_64, 0, 0},
    {TEXT_65, 0, 0}, {TEXT_66, 0, 0}, {TEXT_67, 0, 0}, {TEXT_68, 0, 0},
    {TEXT_69, 0, 0}, {TEXT_70, 0, 0}, {TEXT_71, 0, 0}, {TEXT_72, 0, 0},
    {TEXT_73, 0, 0}, {TEXT_74, 0, 0}, {TEXT_75, 0, 0}, {TEXT_76, 0, 0},
    {TEXT_77, 0, 0},
};
static NJS_TEXLIST e101_land_texlist = {
#include "b_e101/land/e101_land_texlist.inc"
};
static struct land_entry e101_land_entries[159] = {
#include "b_e101/land/e101_land_entries.inc"
};

#pragma section HEAD
/* the pvm's name, which the table points at. the table's initializer is
 * generated from the disc, and names each pointer's target by its symbol.
 * so the name is an array, not a string literal. SHC aligns the array to 1
 * and a literal to 4. as the object's only CHEAD constant, the array
 * starts where a literal would */
static const char e101_land_pvm[11] = {
#include "b_e101/land/e101_land_pvm.inc"
};
struct land_table e101_land = {
#include "b_e101/land/e101_land.inc"
};
#pragma section
