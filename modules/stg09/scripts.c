/* the stage's hint rows, which script_pick runs */
#include <sg_xpt.h>

#include "event/script.h"
#include "stg09/scripts.h"

#define TEXT_1_LENGTH 21
#define TEXT_2_LENGTH 25
#define TEXT_3_LENGTH 23
#define TEXT_4_LENGTH 23
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 26
#define TEXT_7_LENGTH 13
#define TEXT_8_LENGTH 23
#define TEXT_9_LENGTH 13
#define TEXT_10_LENGTH 19
#define TEXT_11_LENGTH 31
#define TEXT_12_LENGTH 12
#define TEXT_13_LENGTH 12
#define TEXT_14_LENGTH 59
#define TEXT_15_LENGTH 55
#define TEXT_16_LENGTH 57
#define TEXT_17_LENGTH 33
#define TEXT_18_LENGTH 55
#define TEXT_19_LENGTH 63
#define TEXT_20_LENGTH 47
#define TEXT_21_LENGTH 55
#define TEXT_22_LENGTH 47
#define TEXT_23_LENGTH 67
#define TEXT_24_LENGTH 31
#define TEXT_25_LENGTH 57
#define TEXT_26_LENGTH 59
#define TEXT_27_LENGTH 57
#define TEXT_28_LENGTH 41
#define TEXT_29_LENGTH 67
#define TEXT_30_LENGTH 59
#include "stg09/scripts/text.inc"
#include "stg09/scripts/values.inc"

/* up to casino_script_table, each row's script, if it has one, and then
 * its runs of pages. the build reads the scripts' words and the pages' steps
 * from the disc */
static const Sint16 casino_script_0[3] = {
#include "stg09/scripts/casino_script_0.inc"
};
static const struct script_page casino_runs_0[4] = {
    {TEXT_1, VALUE_1},
    {TEXT_2, VALUE_2},
    {TEXT_3, VALUE_3},
    {NULL, 0},
};
static const Sint16 casino_script_1[3] = {
#include "stg09/scripts/casino_script_1.inc"
};
static const Sint16 casino_script_2[3] = {
#include "stg09/scripts/casino_script_2.inc"
};
static const struct script_page casino_runs_2[2] = {
    {TEXT_4, VALUE_4},
    {NULL, 0},
};
static const Sint16 casino_script_3[5] = {
#include "stg09/scripts/casino_script_3.inc"
};
static const struct script_page casino_runs_3[3] = {
    {TEXT_5, VALUE_5},
    {TEXT_6, VALUE_6},
    {NULL, 0},
};
static const Sint16 casino_script_4[18] = {
#include "stg09/scripts/casino_script_4.inc"
};
static const struct script_page casino_runs_4[9] = {
    {TEXT_7, VALUE_7}, {TEXT_8, VALUE_8},   {NULL, 0},
    {TEXT_9, VALUE_9}, {NULL, 0},           {TEXT_10, VALUE_10},
    {NULL, 0},         {TEXT_11, VALUE_11}, {NULL, 0},
};
static const Sint16 casino_script_5[9] = {
#include "stg09/scripts/casino_script_5.inc"
};
static const struct script_page casino_runs_5[4] = {
    {TEXT_12, VALUE_12},
    {NULL, 0},
    {TEXT_13, VALUE_13},
    {NULL, 0},
};
static const Sint16 casino_script_6[10] = {
#include "stg09/scripts/casino_script_6.inc"
};
static const struct script_page casino_runs_6[4] = {
    {TEXT_12, VALUE_14},
    {NULL, 0},
    {TEXT_13, VALUE_15},
    {NULL, 0},
};
static const Sint16 casino_script_7[14] = {
#include "stg09/scripts/casino_script_7.inc"
};
static const struct script_page casino_runs_7[4] = {
    {TEXT_12, VALUE_16},
    {NULL, 0},
    {TEXT_13, VALUE_17},
    {NULL, 0},
};
static const Sint16 casino_script_8[3] = {
#include "stg09/scripts/casino_script_8.inc"
};
static const struct script_page casino_runs_8[2] = {
    {TEXT_14, VALUE_18},
    {NULL, 0},
};
static const Sint16 casino_script_9[3] = {
#include "stg09/scripts/casino_script_9.inc"
};
static const struct script_page casino_runs_9[2] = {
    {TEXT_15, VALUE_19},
    {NULL, 0},
};
static const Sint16 casino_script_10[3] = {
#include "stg09/scripts/casino_script_10.inc"
};
static const struct script_page casino_runs_10[2] = {
    {TEXT_16, VALUE_20},
    {NULL, 0},
};
static const struct script_page casino_runs_11[7] = {
    {TEXT_17, VALUE_21}, {TEXT_18, VALUE_22}, {TEXT_19, VALUE_23},
    {TEXT_20, VALUE_24}, {TEXT_21, VALUE_25}, {TEXT_22, VALUE_26},
    {NULL, 0},
};
static const struct script_page casino_runs_12[2] = {
    {TEXT_23, VALUE_27},
    {NULL, 0},
};
static const struct script_page casino_runs_13[7] = {
    {TEXT_24, VALUE_28}, {TEXT_18, VALUE_29}, {TEXT_25, VALUE_30},
    {TEXT_20, VALUE_31}, {TEXT_21, VALUE_32}, {TEXT_22, VALUE_33},
    {NULL, 0},
};
static const struct script_page casino_runs_14[2] = {
    {TEXT_26, VALUE_34},
    {NULL, 0},
};
static const struct script_page casino_runs_15[2] = {
    {TEXT_27, VALUE_35},
    {NULL, 0},
};
static const struct script_page casino_runs_16[3] = {
    {TEXT_28, VALUE_36},
    {TEXT_29, VALUE_37},
    {NULL, 0},
};
static const struct script_page casino_runs_17[2] = {
    {TEXT_30, VALUE_38},
    {NULL, 0},
};

/* 1ST_READ.BIN's stage_scripts names it for the stage */
const struct script_row casino_script_table[CASINO_SCRIPT_ROWS] = {
    {(Sint16 *)casino_script_0, (void *)casino_runs_0},
    {(Sint16 *)casino_script_1, NULL},
    {(Sint16 *)casino_script_2, (void *)casino_runs_2},
    {(Sint16 *)casino_script_3, (void *)casino_runs_3},
    {(Sint16 *)casino_script_4, (void *)casino_runs_4},
    {(Sint16 *)casino_script_5, (void *)casino_runs_5},
    {(Sint16 *)casino_script_6, (void *)casino_runs_6},
    {(Sint16 *)casino_script_7, (void *)casino_runs_7},
    {(Sint16 *)casino_script_8, (void *)casino_runs_8},
    {(Sint16 *)casino_script_9, (void *)casino_runs_9},
    {(Sint16 *)casino_script_10, (void *)casino_runs_10},
    {NULL, (void *)casino_runs_11},
    {NULL, (void *)casino_runs_12},
    {NULL, (void *)casino_runs_13},
    {NULL, (void *)casino_runs_14},
    {NULL, (void *)casino_runs_15},
    {NULL, (void *)casino_runs_16},
    {NULL, (void *)casino_runs_17},
};
