/* Twinkle Park's script table, whose rows script_pick runs for the stage's
 * hints. a row's script picks the pages its hint shows: the run of its first
 * branch whose tests pass */
#include <sg_xpt.h>

#include "event/script.h"
#include "stg03/scripts.h"

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
#define TEXT_14_LENGTH 37
#define TEXT_15_LENGTH 27
#define TEXT_16_LENGTH 47
#define TEXT_17_LENGTH 55
#define TEXT_18_LENGTH 59
#define TEXT_19_LENGTH 37
#define TEXT_20_LENGTH 21
#define TEXT_21_LENGTH 21
#define TEXT_22_LENGTH 25
#define TEXT_23_LENGTH 43
#define TEXT_24_LENGTH 53
#define TEXT_25_LENGTH 33
#define TEXT_26_LENGTH 23
#define TEXT_27_LENGTH 25
#define TEXT_28_LENGTH 29
#define TEXT_29_LENGTH 51
#define TEXT_30_LENGTH 15
#define TEXT_31_LENGTH 53
#include "stg03/scripts/text.inc"
#include "stg03/scripts/values.inc"

/* each row's script, then its runs, where the row has them, in row order. the
 * scripts' words come from the disc, in their .inc files. so do the pages'
 * texts and steps, in text.inc and values.inc */
static const struct script_page twinkle_runs_0[4] = {
    {TEXT_1, VALUE_1},
    {TEXT_2, VALUE_2},
    {TEXT_3, VALUE_3},
    {NULL, 0},
};
static const Sint16 twinkle_script_1[3] = {
#include "stg03/scripts/twinkle_script_1.inc"
};
static const Sint16 twinkle_script_2[3] = {
#include "stg03/scripts/twinkle_script_2.inc"
};
static const struct script_page twinkle_runs_2[2] = {
    {TEXT_4, VALUE_4},
    {NULL, 0},
};
static const Sint16 twinkle_script_3[5] = {
#include "stg03/scripts/twinkle_script_3.inc"
};
static const struct script_page twinkle_runs_3[3] = {
    {TEXT_5, VALUE_5},
    {TEXT_6, VALUE_6},
    {NULL, 0},
};
static const Sint16 twinkle_script_4[18] = {
#include "stg03/scripts/twinkle_script_4.inc"
};
static const struct script_page twinkle_runs_4[9] = {
    {TEXT_7, VALUE_7}, {TEXT_8, VALUE_8},   {NULL, 0},
    {TEXT_9, VALUE_9}, {NULL, 0},           {TEXT_10, VALUE_10},
    {NULL, 0},         {TEXT_11, VALUE_11}, {NULL, 0},
};
static const Sint16 twinkle_script_5[9] = {
#include "stg03/scripts/twinkle_script_5.inc"
};
static const struct script_page twinkle_runs_5[4] = {
    {TEXT_12, VALUE_12},
    {NULL, 0},
    {TEXT_13, VALUE_13},
    {NULL, 0},
};
static const Sint16 twinkle_script_6[10] = {
#include "stg03/scripts/twinkle_script_6.inc"
};
static const struct script_page twinkle_runs_6[4] = {
    {TEXT_12, VALUE_14},
    {NULL, 0},
    {TEXT_13, VALUE_15},
    {NULL, 0},
};
static const Sint16 twinkle_script_7[14] = {
#include "stg03/scripts/twinkle_script_7.inc"
};
static const struct script_page twinkle_runs_7[4] = {
    {TEXT_12, VALUE_16},
    {NULL, 0},
    {TEXT_13, VALUE_17},
    {NULL, 0},
};
static const Sint16 twinkle_script_8[3] = {
#include "stg03/scripts/twinkle_script_8.inc"
};
static const struct script_page twinkle_runs_8[3] = {
    {TEXT_14, VALUE_18},
    {TEXT_15, VALUE_19},
    {NULL, 0},
};
static const Sint16 twinkle_script_9[3] = {
#include "stg03/scripts/twinkle_script_9.inc"
};
static const struct script_page twinkle_runs_9[2] = {
    {TEXT_16, VALUE_20},
    {NULL, 0},
};
static const Sint16 twinkle_script_10[3] = {
#include "stg03/scripts/twinkle_script_10.inc"
};
static const struct script_page twinkle_runs_10[3] = {
    {TEXT_17, VALUE_21},
    {TEXT_18, VALUE_22},
    {NULL, 0},
};
static const Sint16 twinkle_script_11[3] = {
#include "stg03/scripts/twinkle_script_11.inc"
};
static const struct script_page twinkle_runs_11[3] = {
    {TEXT_19, VALUE_23},
    {TEXT_20, VALUE_24},
    {NULL, 0},
};
static const Sint16 twinkle_script_12[3] = {
#include "stg03/scripts/twinkle_script_12.inc"
};
static const struct script_page twinkle_runs_12[3] = {
    {TEXT_21, VALUE_25},
    {TEXT_22, VALUE_26},
    {NULL, 0},
};
static const Sint16 twinkle_script_13[3] = {
#include "stg03/scripts/twinkle_script_13.inc"
};
static const struct script_page twinkle_runs_13[2] = {
    {TEXT_23, VALUE_27},
    {NULL, 0},
};
static const Sint16 twinkle_script_14[3] = {
#include "stg03/scripts/twinkle_script_14.inc"
};
static const struct script_page twinkle_runs_14[2] = {
    {TEXT_24, VALUE_28},
    {NULL, 0},
};
static const Sint16 twinkle_script_15[3] = {
#include "stg03/scripts/twinkle_script_15.inc"
};
static const struct script_page twinkle_runs_15[3] = {
    {TEXT_25, VALUE_29},
    {TEXT_26, VALUE_30},
    {NULL, 0},
};
static const Sint16 twinkle_script_16[3] = {
#include "stg03/scripts/twinkle_script_16.inc"
};
static const struct script_page twinkle_runs_16[3] = {
    {TEXT_27, VALUE_31},
    {TEXT_28, VALUE_32},
    {NULL, 0},
};
static const Sint16 twinkle_script_17[3] = {
#include "stg03/scripts/twinkle_script_17.inc"
};
static const struct script_page twinkle_runs_17[2] = {
    {TEXT_29, VALUE_33},
    {NULL, 0},
};
static const struct script_page twinkle_runs_18[3] = {
    {TEXT_30, VALUE_34},
    {TEXT_31, VALUE_35},
    {NULL, 0},
};

const struct script_row twinkle_script_table[TWINKLE_SCRIPT_ROWS] = {
    {NULL, (void *)twinkle_runs_0},
    {(Sint16 *)twinkle_script_1, NULL},
    {(Sint16 *)twinkle_script_2, (void *)twinkle_runs_2},
    {(Sint16 *)twinkle_script_3, (void *)twinkle_runs_3},
    {(Sint16 *)twinkle_script_4, (void *)twinkle_runs_4},
    {(Sint16 *)twinkle_script_5, (void *)twinkle_runs_5},
    {(Sint16 *)twinkle_script_6, (void *)twinkle_runs_6},
    {(Sint16 *)twinkle_script_7, (void *)twinkle_runs_7},
    {(Sint16 *)twinkle_script_8, (void *)twinkle_runs_8},
    {(Sint16 *)twinkle_script_9, (void *)twinkle_runs_9},
    {(Sint16 *)twinkle_script_10, (void *)twinkle_runs_10},
    {(Sint16 *)twinkle_script_11, (void *)twinkle_runs_11},
    {(Sint16 *)twinkle_script_12, (void *)twinkle_runs_12},
    {(Sint16 *)twinkle_script_13, (void *)twinkle_runs_13},
    {(Sint16 *)twinkle_script_14, (void *)twinkle_runs_14},
    {(Sint16 *)twinkle_script_15, (void *)twinkle_runs_15},
    {(Sint16 *)twinkle_script_16, (void *)twinkle_runs_16},
    {(Sint16 *)twinkle_script_17, (void *)twinkle_runs_17},
    {NULL, (void *)twinkle_runs_18},
};
