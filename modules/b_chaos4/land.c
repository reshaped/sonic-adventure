/* the land of Chaos 4's stage: its table, which names its 72 entries, its
 * one motion, its texlist and the PVM the texlist loads from. the texlist
 * names the land's 13 textures */
#include <ninja.h>

#include "b_chaos4/land.h"
#include "collision/world.h"

#define TEXT_1_LENGTH 18
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 12
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 14
#define TEXT_7_LENGTH 13
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 13
#define TEXT_10_LENGTH 13
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 13
#define TEXT_13_LENGTH 11
#include "b_chaos4/land/text.inc"

/* the action the land's motion plays */
extern NJS_ACTION dat_0c95e478;

/* the objects of the land's 72 entries, then the object its motion moves */
extern NJS_OBJECT dat_0c947918;
extern NJS_OBJECT dat_0c94801c;
extern NJS_OBJECT dat_0c948804;
extern NJS_OBJECT dat_0c948a14;
extern NJS_OBJECT dat_0c94a174;
extern NJS_OBJECT dat_0c94b1f4;
extern NJS_OBJECT dat_0c94bb6c;
extern NJS_OBJECT dat_0c94c258;
extern NJS_OBJECT dat_0c94c9f4;
extern NJS_OBJECT dat_0c94ce4c;
extern NJS_OBJECT dat_0c94da74;
extern NJS_OBJECT dat_0c94daa8;
extern NJS_OBJECT dat_0c94dadc;
extern NJS_OBJECT dat_0c94db44;
extern NJS_OBJECT dat_0c94db78;
extern NJS_OBJECT dat_0c94dbac;
extern NJS_OBJECT dat_0c94dc14;
extern NJS_OBJECT dat_0c94dc48;
extern NJS_OBJECT dat_0c94dc7c;
extern NJS_OBJECT dat_0c94dcb0;
extern NJS_OBJECT dat_0c94dce4;
extern NJS_OBJECT dat_0c94dd18;
extern NJS_OBJECT dat_0c94dd4c;
extern NJS_OBJECT dat_0c94dd80;
extern NJS_OBJECT dat_0c94dde8;
extern NJS_OBJECT dat_0c94de1c;
extern NJS_OBJECT dat_0c94de50;
extern NJS_OBJECT dat_0c94de84;
extern NJS_OBJECT dat_0c94deb8;
extern NJS_OBJECT dat_0c94df20;
extern NJS_OBJECT dat_0c94df54;
extern NJS_OBJECT dat_0c94df88;
extern NJS_OBJECT dat_0c94dfbc;
extern NJS_OBJECT dat_0c94dff0;
extern NJS_OBJECT dat_0c94e024;
extern NJS_OBJECT dat_0c94e058;
extern NJS_OBJECT dat_0c94e08c;
extern NJS_OBJECT dat_0c94e0c0;
extern NJS_OBJECT dat_0c94e0f4;
extern NJS_OBJECT dat_0c94e128;
extern NJS_OBJECT dat_0c94e190;
extern NJS_OBJECT dat_0c94e1c4;
extern NJS_OBJECT dat_0c94e22c;
extern NJS_OBJECT dat_0c94e294;
extern NJS_OBJECT dat_0c94e2c8;
extern NJS_OBJECT dat_0c94e330;
extern NJS_OBJECT dat_0c94e364;
extern NJS_OBJECT dat_0c94e3cc;
extern NJS_OBJECT dat_0c94e400;
extern NJS_OBJECT dat_0c94e434;
extern NJS_OBJECT dat_0c94e468;
extern NJS_OBJECT dat_0c94e49c;
extern NJS_OBJECT dat_0c94e4d0;
extern NJS_OBJECT dat_0c94e5a0;
extern NJS_OBJECT dat_0c94e608;
extern NJS_OBJECT dat_0c94e63c;
extern NJS_OBJECT dat_0c94e670;
extern NJS_OBJECT dat_0c94e6a4;
extern NJS_OBJECT dat_0c94e6d8;
extern NJS_OBJECT dat_0c94e740;
extern NJS_OBJECT dat_0c94e774;
extern NJS_OBJECT dat_0c94e7a8;
extern NJS_OBJECT dat_0c94e7dc;
extern NJS_OBJECT dat_0c94e810;
extern NJS_OBJECT dat_0c94e878;
extern NJS_OBJECT dat_0c94e8ac;
extern NJS_OBJECT dat_0c94e948;
extern NJS_OBJECT dat_0c94e9b0;
extern NJS_OBJECT dat_0c94ea18;
extern NJS_OBJECT dat_0c94ea4c;
extern NJS_OBJECT dat_0c94ea80;
extern NJS_OBJECT dat_0c94eab4;
extern NJS_OBJECT dat_0c9506f8;

static NJS_TEXNAME chaos4_land_textures[13] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0},
};
static NJS_TEXLIST chaos4_land_texlist = {
#include "b_chaos4/land/chaos4_land_texlist.inc"
};
static struct land_entry chaos4_land_entries[72] = {
#include "b_chaos4/land/chaos4_land_entries.inc"
};
static struct land_motion chaos4_land_motions[1] = {
#include "b_chaos4/land/chaos4_land_motions.inc"
};

#pragma section HEAD
/* the table's generated initializer can point only at a symbol, so the
 * PVM's name is a named array, not a string literal. as the only constant
 * in this file's HEAD section, it starts where the literal would */
static const char chaos4_land_pvm[13] = {
#include "b_chaos4/land/chaos4_land_pvm.inc"
};
struct land_table chaos4_land = {
#include "b_chaos4/land/chaos4_land.inc"
};
#pragma section
