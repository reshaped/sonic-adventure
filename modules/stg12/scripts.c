/* the stage's hint rows, which script_pick runs */
#include <sg_xpt.h>

#include "event/script.h"
#include "stg12/scripts.h"

#define TEXT_1_LENGTH 21
#define TEXT_2_LENGTH 25
#define TEXT_3_LENGTH 23
#define TEXT_4_LENGTH 16
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 26
#define TEXT_7_LENGTH 13
#define TEXT_8_LENGTH 23
#define TEXT_9_LENGTH 13
#define TEXT_10_LENGTH 19
#define TEXT_11_LENGTH 31
#define TEXT_12_LENGTH 12
#define TEXT_13_LENGTH 12
#define TEXT_14_LENGTH 52
#define TEXT_15_LENGTH 44
#define TEXT_16_LENGTH 52
#define TEXT_17_LENGTH 64
#define TEXT_18_LENGTH 33
#define TEXT_19_LENGTH 46
#define TEXT_20_LENGTH 33
#define TEXT_21_LENGTH 13
#define TEXT_22_LENGTH 68
#define TEXT_23_LENGTH 58
#define TEXT_24_LENGTH 15
#define TEXT_25_LENGTH 37
#define TEXT_26_LENGTH 19
#define TEXT_27_LENGTH 54
#define TEXT_28_LENGTH 21
#define TEXT_29_LENGTH 27
#define TEXT_30_LENGTH 23
#define TEXT_31_LENGTH 29
#define TEXT_32_LENGTH 19
#define TEXT_33_LENGTH 56
#include "stg12/scripts/text.inc"
#include "stg12/scripts/values.inc"

/* from here to shelter_script_table, the arrays are each row's script, if
 * any, and then its runs. a run is the pages one branch of the script
 * picks, ended by a null text. the scripts' words and the pages' steps are
 * the disc's */
static const struct script_page shelter_runs_0[4] = {
    {TEXT_1, VALUE_1},
    {TEXT_2, VALUE_2},
    {TEXT_3, VALUE_3},
    {NULL, 0},
};
static const Sint16 shelter_script_1[3] = {
#include "stg12/scripts/shelter_script_1.inc"
};
static const Sint16 shelter_script_2[3] = {
#include "stg12/scripts/shelter_script_2.inc"
};
static const struct script_page shelter_runs_1[2] = {
    {TEXT_4, VALUE_4},
    {NULL, 0},
};
static const Sint16 shelter_script_3[5] = {
#include "stg12/scripts/shelter_script_3.inc"
};
static const struct script_page shelter_runs_3[3] = {
    {TEXT_5, VALUE_5},
    {TEXT_6, VALUE_6},
    {NULL, 0},
};
static const Sint16 shelter_script_4[18] = {
#include "stg12/scripts/shelter_script_4.inc"
};
static const struct script_page shelter_runs_4[9] = {
    {TEXT_7, VALUE_7}, {TEXT_8, VALUE_8},   {NULL, 0},
    {TEXT_9, VALUE_9}, {NULL, 0},           {TEXT_10, VALUE_10},
    {NULL, 0},         {TEXT_11, VALUE_11}, {NULL, 0},
};
static const Sint16 shelter_script_5[9] = {
#include "stg12/scripts/shelter_script_5.inc"
};
static const struct script_page shelter_runs_5[4] = {
    {TEXT_12, VALUE_12},
    {NULL, 0},
    {TEXT_13, VALUE_13},
    {NULL, 0},
};
static const Sint16 shelter_script_6[10] = {
#include "stg12/scripts/shelter_script_6.inc"
};
static const struct script_page shelter_runs_6[4] = {
    {TEXT_12, VALUE_14},
    {NULL, 0},
    {TEXT_13, VALUE_15},
    {NULL, 0},
};
static const Sint16 shelter_script_7[14] = {
#include "stg12/scripts/shelter_script_7.inc"
};
static const struct script_page shelter_runs_7[4] = {
    {TEXT_12, VALUE_16},
    {NULL, 0},
    {TEXT_13, VALUE_17},
    {NULL, 0},
};
static const Sint16 shelter_script_8[3] = {
#include "stg12/scripts/shelter_script_8.inc"
};
static const struct script_page shelter_runs_8[2] = {
    {TEXT_14, VALUE_18},
    {NULL, 0},
};
static const Sint16 shelter_script_9[3] = {
#include "stg12/scripts/shelter_script_9.inc"
};
static const struct script_page shelter_runs_9[2] = {
    {TEXT_15, VALUE_19},
    {NULL, 0},
};
static const Sint16 shelter_script_10[3] = {
#include "stg12/scripts/shelter_script_10.inc"
};
static const struct script_page shelter_runs_10[2] = {
    {TEXT_16, VALUE_20},
    {NULL, 0},
};
static const Sint16 shelter_script_11[3] = {
#include "stg12/scripts/shelter_script_11.inc"
};
static const struct script_page shelter_runs_11[2] = {
    {TEXT_17, VALUE_21},
    {NULL, 0},
};
static const Sint16 shelter_script_12[3] = {
#include "stg12/scripts/shelter_script_12.inc"
};
static const struct script_page shelter_runs_12[2] = {
    {TEXT_18, VALUE_22},
    {NULL, 0},
};
static const Sint16 shelter_script_13[3] = {
#include "stg12/scripts/shelter_script_13.inc"
};
static const struct script_page shelter_runs_13[2] = {
    {TEXT_19, VALUE_23},
    {NULL, 0},
};
static const Sint16 shelter_script_14[3] = {
#include "stg12/scripts/shelter_script_14.inc"
};
static const struct script_page shelter_runs_14[2] = {
    {TEXT_20, VALUE_24},
    {NULL, 0},
};
static const struct script_page shelter_runs_15[3] = {
    {TEXT_21, VALUE_25},
    {TEXT_22, VALUE_26},
    {NULL, 0},
};
static const struct script_page shelter_runs_16[3] = {
    {TEXT_21, VALUE_27},
    {TEXT_23, VALUE_28},
    {NULL, 0},
};
static const struct script_page shelter_runs_17[3] = {
    {TEXT_24, VALUE_29},
    {TEXT_25, VALUE_30},
    {NULL, 0},
};
static const struct script_page shelter_runs_18[3] = {
    {TEXT_26, VALUE_31},
    {TEXT_27, VALUE_32},
    {NULL, 0},
};
static const struct script_page shelter_runs_19[4] = {
    {TEXT_28, VALUE_33},
    {TEXT_29, VALUE_34},
    {TEXT_30, VALUE_35},
    {NULL, 0},
};
static const Sint16 shelter_script_20[3] = {
#include "stg12/scripts/shelter_script_20.inc"
};
static const struct script_page shelter_runs_20[2] = {
    {TEXT_31, VALUE_36},
    {NULL, 0},
};
static const struct script_page shelter_runs_21[3] = {
    {TEXT_32, VALUE_37},
    {TEXT_33, VALUE_38},
    {NULL, 0},
};

/* 1ST_READ.BIN's stage_scripts names it for the stage */
const struct script_row shelter_script_table[SHELTER_SCRIPT_ROWS] = {
    {NULL, (void *)shelter_runs_0},
    {(Sint16 *)shelter_script_1, (void *)shelter_runs_1},
    {(Sint16 *)shelter_script_2, (void *)shelter_runs_1},
    {(Sint16 *)shelter_script_3, (void *)shelter_runs_3},
    {(Sint16 *)shelter_script_4, (void *)shelter_runs_4},
    {(Sint16 *)shelter_script_5, (void *)shelter_runs_5},
    {(Sint16 *)shelter_script_6, (void *)shelter_runs_6},
    {(Sint16 *)shelter_script_7, (void *)shelter_runs_7},
    {(Sint16 *)shelter_script_8, (void *)shelter_runs_8},
    {(Sint16 *)shelter_script_9, (void *)shelter_runs_9},
    {(Sint16 *)shelter_script_10, (void *)shelter_runs_10},
    {(Sint16 *)shelter_script_11, (void *)shelter_runs_11},
    {(Sint16 *)shelter_script_12, (void *)shelter_runs_12},
    {(Sint16 *)shelter_script_13, (void *)shelter_runs_13},
    {(Sint16 *)shelter_script_14, (void *)shelter_runs_14},
    {NULL, (void *)shelter_runs_15},
    {NULL, (void *)shelter_runs_16},
    {NULL, (void *)shelter_runs_17},
    {NULL, (void *)shelter_runs_18},
    {NULL, (void *)shelter_runs_19},
    {(Sint16 *)shelter_script_20, (void *)shelter_runs_20},
    {NULL, (void *)shelter_runs_21},
};
