/* the stage's land for act 1: its table and entries. it also holds the
 * texlist its textures load into, their names, and the name of the pvm
 * they load from */
#include <ninja.h>

#include "b_chaos6/dat_0ca70168.h" /* NOLINT(misc-include-cleaner) */
#include "b_chaos6/dat_0ca91950.h" /* NOLINT(misc-include-cleaner) */
#include "b_chaos6/land_act1.h"
#include "collision/world.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 10
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 10
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 12
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 10
#define TEXT_27_LENGTH 9
#define TEXT_28_LENGTH 9
#define TEXT_29_LENGTH 9
#define TEXT_30_LENGTH 9
#define TEXT_31_LENGTH 9
#define TEXT_32_LENGTH 10
#define TEXT_33_LENGTH 10
#define TEXT_34_LENGTH 11
#define TEXT_35_LENGTH 12
#define TEXT_36_LENGTH 9
#define TEXT_37_LENGTH 11
#define TEXT_38_LENGTH 9
#define TEXT_39_LENGTH 10
#define TEXT_40_LENGTH 9
#define TEXT_41_LENGTH 9
#define TEXT_42_LENGTH 10
#define TEXT_43_LENGTH 10
#define TEXT_44_LENGTH 10
#define TEXT_45_LENGTH 10
#define TEXT_46_LENGTH 9
#define TEXT_47_LENGTH 11
#define TEXT_48_LENGTH 8
#define TEXT_49_LENGTH 13
#define TEXT_50_LENGTH 10
#define TEXT_51_LENGTH 12
#define TEXT_52_LENGTH 11
#define TEXT_53_LENGTH 13
#define TEXT_54_LENGTH 10
#define TEXT_55_LENGTH 10
#define TEXT_56_LENGTH 10
#define TEXT_57_LENGTH 10
#define TEXT_58_LENGTH 10
#define TEXT_59_LENGTH 10
#define TEXT_60_LENGTH 10
#define TEXT_61_LENGTH 11
#define TEXT_62_LENGTH 13
#define TEXT_63_LENGTH 16
#define TEXT_64_LENGTH 12
#define TEXT_65_LENGTH 12
#define TEXT_66_LENGTH 11
#define TEXT_67_LENGTH 11
#define TEXT_68_LENGTH 10
#define TEXT_69_LENGTH 10
#define TEXT_70_LENGTH 8
#define TEXT_71_LENGTH 10
#define TEXT_72_LENGTH 8
#define TEXT_73_LENGTH 11
#define TEXT_74_LENGTH 13
#define TEXT_75_LENGTH 10
#define TEXT_76_LENGTH 10
#define TEXT_77_LENGTH 11
#define TEXT_78_LENGTH 9
#define TEXT_79_LENGTH 12
#include "b_chaos6/land_act1/text.inc"

/* the objects the land's entries name */
extern NJS_OBJECT dat_0ca78c90;
extern NJS_OBJECT dat_0ca79c24;
extern NJS_OBJECT dat_0ca79f48;
extern NJS_OBJECT dat_0ca7a4c0;
extern NJS_OBJECT dat_0ca7aa50;
extern NJS_OBJECT dat_0ca7b120;
extern NJS_OBJECT dat_0ca7b3e4;
extern NJS_OBJECT dat_0ca7b6a8;
extern NJS_OBJECT dat_0ca7bf50;
extern NJS_OBJECT dat_0ca7d00c;
extern NJS_OBJECT dat_0ca7e204;
extern NJS_OBJECT dat_0ca7e828;
extern NJS_OBJECT dat_0ca7ee70;
extern NJS_OBJECT dat_0ca7f028;
extern NJS_OBJECT dat_0ca7f1fc;
extern NJS_OBJECT dat_0ca7f570;
extern NJS_OBJECT dat_0ca7f814;
extern NJS_OBJECT dat_0ca80758;
extern NJS_OBJECT dat_0ca80cac;
extern NJS_OBJECT dat_0ca81b5c;
extern NJS_OBJECT dat_0ca84070;
extern NJS_OBJECT dat_0ca86774;
extern NJS_OBJECT dat_0ca86db0;
extern NJS_OBJECT dat_0ca873ec;
extern NJS_OBJECT dat_0ca87a28;
extern NJS_OBJECT dat_0ca88070;
extern NJS_OBJECT dat_0ca886b8;
extern NJS_OBJECT dat_0ca88d00;
extern NJS_OBJECT dat_0ca89794;
extern NJS_OBJECT dat_0ca89e34;
extern NJS_OBJECT dat_0ca8a4e0;
extern NJS_OBJECT dat_0ca8af74;
extern NJS_OBJECT dat_0ca8b614;
extern NJS_OBJECT dat_0ca8c0a8;
extern NJS_OBJECT dat_0ca8cfa4;
extern NJS_OBJECT dat_0ca8e900;
extern NJS_OBJECT dat_0ca8ef84;
extern NJS_OBJECT dat_0ca90010;
extern NJS_OBJECT dat_0ca905d8;
extern NJS_OBJECT dat_0ca90b74;
extern NJS_OBJECT dat_0ca90d04;
extern NJS_OBJECT dat_0ca91334;
extern NJS_OBJECT dat_0ca914c4;
extern NJS_OBJECT dat_0ca914f8;
extern NJS_OBJECT dat_0ca9152c;
extern NJS_OBJECT dat_0ca91560;
extern NJS_OBJECT dat_0ca91594;
extern NJS_OBJECT dat_0ca915c8;
extern NJS_OBJECT dat_0ca915fc;
extern NJS_OBJECT dat_0ca91630;
extern NJS_OBJECT dat_0ca91664;
extern NJS_OBJECT dat_0ca91698;
extern NJS_OBJECT dat_0ca916cc;
extern NJS_OBJECT dat_0ca91700;
extern NJS_OBJECT dat_0ca91734;
extern NJS_OBJECT dat_0ca91768;
extern NJS_OBJECT dat_0ca9179c;
extern NJS_OBJECT dat_0ca917d0;
extern NJS_OBJECT dat_0ca91804;
extern NJS_OBJECT dat_0ca91838;
extern NJS_OBJECT dat_0ca9186c;
extern NJS_OBJECT dat_0ca918a0;
extern NJS_OBJECT dat_0ca918d4;
extern NJS_OBJECT dat_0ca91908;

static NJS_TEXNAME chaos6_act1_land_textures[79] = {
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
    {TEXT_77, 0, 0}, {TEXT_78, 0, 0}, {TEXT_79, 0, 0},
};
static NJS_TEXLIST chaos6_act1_land_texlist = {
#include "b_chaos6/land_act1/chaos6_act1_land_texlist.inc"
};
static struct land_entry chaos6_act1_land_entries[66] = {
#include "b_chaos6/land_act1/chaos6_act1_land_entries.inc"
};

#pragma section HEAD
/* the name of the pvm the land's textures load from. a body is an
 * initializer the build extracts from the disc, and it can point only at a
 * named symbol. so the name is an array, not a string literal. SHC aligns
 * an array to 1 and a literal to 4. the array is the object's only
 * constant, so it starts where the literal would */
static const char chaos6_act1_land_pvm[12] = {
#include "b_chaos6/land_act1/chaos6_act1_land_pvm.inc"
};
struct land_table chaos6_act1_land = {
#include "b_chaos6/land_act1/chaos6_act1_land.inc"
};
#pragma section
