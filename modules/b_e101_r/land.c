/* the Egg Carrier deck's land: its table and entries, its textures' names
 * and texlist, and its pvm's name. B_ROBO links the same object, and the
 * dat_ names here give B_E101_R's addresses */
#include <ninja.h>

#include "b_e101_r/land.h"
#include "collision/world.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 13
#define TEXT_8_LENGTH 16
#define TEXT_9_LENGTH 12
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 13
#define TEXT_21_LENGTH 10
#define TEXT_22_LENGTH 10
#define TEXT_23_LENGTH 12
#define TEXT_24_LENGTH 8
#define TEXT_25_LENGTH 9
#define TEXT_26_LENGTH 10
#define TEXT_27_LENGTH 9
#define TEXT_28_LENGTH 10
#define TEXT_29_LENGTH 10
#define TEXT_30_LENGTH 10
#define TEXT_31_LENGTH 10
#define TEXT_32_LENGTH 11
#define TEXT_33_LENGTH 11
#define TEXT_34_LENGTH 9
#define TEXT_35_LENGTH 8
#define TEXT_36_LENGTH 13
#define TEXT_37_LENGTH 10
#define TEXT_38_LENGTH 12
#define TEXT_39_LENGTH 11
#define TEXT_40_LENGTH 10
#define TEXT_41_LENGTH 10
#define TEXT_42_LENGTH 10
#define TEXT_43_LENGTH 10
#define TEXT_44_LENGTH 9
#define TEXT_45_LENGTH 9
#define TEXT_46_LENGTH 10
#define TEXT_47_LENGTH 10
#define TEXT_48_LENGTH 10
#define TEXT_49_LENGTH 13
#define TEXT_50_LENGTH 10
#define TEXT_51_LENGTH 9
#define TEXT_52_LENGTH 10
#define TEXT_53_LENGTH 9
#define TEXT_54_LENGTH 11
#define TEXT_55_LENGTH 9
#define TEXT_56_LENGTH 11
#define TEXT_57_LENGTH 10
#define TEXT_58_LENGTH 8
#define TEXT_59_LENGTH 9
#define TEXT_60_LENGTH 9
#define TEXT_61_LENGTH 9
#define TEXT_62_LENGTH 9
#define TEXT_63_LENGTH 9
#define TEXT_64_LENGTH 9
#define TEXT_65_LENGTH 9
#define TEXT_66_LENGTH 9
#define TEXT_67_LENGTH 9
#define TEXT_68_LENGTH 9
#define TEXT_69_LENGTH 9
#define TEXT_70_LENGTH 10
#define TEXT_71_LENGTH 9
#define TEXT_72_LENGTH 9
#define TEXT_73_LENGTH 12
#define TEXT_74_LENGTH 10
#define TEXT_75_LENGTH 10
#define TEXT_76_LENGTH 9
#define TEXT_77_LENGTH 9
#include "b_e101_r/land/text.inc"

extern struct land_motion e101_r_land_motions[];

/* the objects the land's entries name */
extern NJS_OBJECT dat_0c912cbc;
extern NJS_OBJECT dat_0c913c3c;
extern NJS_OBJECT dat_0c9145ec;
extern NJS_OBJECT dat_0c914b64;
extern NJS_OBJECT dat_0c9150f4;
extern NJS_OBJECT dat_0c91579c;
extern NJS_OBJECT dat_0c915a60;
extern NJS_OBJECT dat_0c915d24;
extern NJS_OBJECT dat_0c9165cc;
extern NJS_OBJECT dat_0c917674;
extern NJS_OBJECT dat_0c918858;
extern NJS_OBJECT dat_0c918e68;
extern NJS_OBJECT dat_0c9194b0;
extern NJS_OBJECT dat_0c919668;
extern NJS_OBJECT dat_0c91983c;
extern NJS_OBJECT dat_0c919b9c;
extern NJS_OBJECT dat_0c919e18;
extern NJS_OBJECT dat_0c91ad5c;
extern NJS_OBJECT dat_0c91b2b0;
extern NJS_OBJECT dat_0c91c33c;
extern NJS_OBJECT dat_0c91c904;
extern NJS_OBJECT dat_0c91cea0;
extern NJS_OBJECT dat_0c91d1cc;
extern NJS_OBJECT dat_0c91d7fc;
extern NJS_OBJECT dat_0c91db28;
extern NJS_OBJECT dat_0c91db5c;
extern NJS_OBJECT dat_0c91db90;
extern NJS_OBJECT dat_0c91dbc4;
extern NJS_OBJECT dat_0c91dbf8;
extern NJS_OBJECT dat_0c91dc2c;
extern NJS_OBJECT dat_0c91dc60;
extern NJS_OBJECT dat_0c91dc94;
extern NJS_OBJECT dat_0c91dcc8;
extern NJS_OBJECT dat_0c91dcfc;
extern NJS_OBJECT dat_0c91dd30;
extern NJS_OBJECT dat_0c91dd64;
extern NJS_OBJECT dat_0c91dd98;
extern NJS_OBJECT dat_0c91ddcc;
extern NJS_OBJECT dat_0c91de00;
extern NJS_OBJECT dat_0c91de34;
extern NJS_OBJECT dat_0c91de68;
extern NJS_OBJECT dat_0c91de9c;
extern NJS_OBJECT dat_0c91ded0;
extern NJS_OBJECT dat_0c91df04;
extern NJS_OBJECT dat_0c91df38;

static NJS_TEXNAME e101_r_land_textures[77] = {
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
static NJS_TEXLIST e101_r_land_texlist = {
#include "b_e101_r/land/e101_r_land_texlist.inc"
};
static struct land_entry e101_r_land_entries[45] = {
#include "b_e101_r/land/e101_r_land_entries.inc"
};

#pragma section HEAD
/* the pvm's name, which the table points at. the table's initializer is
 * generated from the disc, and names each pointer's target by its symbol.
 * so the name is an array, not a string literal. SHC aligns the array to 1
 * and a literal to 4. as the object's only CHEAD constant, the array
 * starts where a literal would */
static const char e101_r_land_pvm[12] = {
#include "b_e101_r/land/e101_r_land_pvm.inc"
};
struct land_table e101_r_land = {
#include "b_e101_r/land/e101_r_land.inc"
};
#pragma section
