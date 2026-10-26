/* the stage's hint rows, which script_pick runs */
#include <sg_xpt.h>

#include "event/script.h"
#include "stg10/scripts.h"

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
#define TEXT_14_LENGTH 37
#define TEXT_15_LENGTH 33
#define TEXT_16_LENGTH 29
#define TEXT_17_LENGTH 31
#define TEXT_18_LENGTH 55
#define TEXT_19_LENGTH 59
#define TEXT_20_LENGTH 59
#define TEXT_21_LENGTH 47
#define TEXT_22_LENGTH 67
#define TEXT_23_LENGTH 59
#include "stg10/scripts/text.inc"
#include "stg10/scripts/values.inc"

static const struct script_page final_runs_0[4] = {
    {TEXT_1, VALUE_1},
    {TEXT_2, VALUE_2},
    {TEXT_3, VALUE_3},
    {NULL, 0},
};
static const Sint16 final_script_1[3] = {
#include "stg10/scripts/final_script_1.inc"
};
static const Sint16 final_script_2[3] = {
#include "stg10/scripts/final_script_2.inc"
};
static const struct script_page final_runs_1[2] = {
    {TEXT_4, VALUE_4},
    {NULL, 0},
};
static const Sint16 final_script_3[5] = {
#include "stg10/scripts/final_script_3.inc"
};
static const struct script_page final_runs_3[3] = {
    {TEXT_5, VALUE_5},
    {TEXT_6, VALUE_6},
    {NULL, 0},
};
static const Sint16 final_script_4[18] = {
#include "stg10/scripts/final_script_4.inc"
};
static const struct script_page final_runs_4[9] = {
    {TEXT_7, VALUE_7}, {TEXT_8, VALUE_8},   {NULL, 0},
    {TEXT_9, VALUE_9}, {NULL, 0},           {TEXT_10, VALUE_10},
    {NULL, 0},         {TEXT_11, VALUE_11}, {NULL, 0},
};
static const Sint16 final_script_5[9] = {
#include "stg10/scripts/final_script_5.inc"
};
static const struct script_page final_runs_5[4] = {
    {TEXT_12, VALUE_12},
    {NULL, 0},
    {TEXT_13, VALUE_13},
    {NULL, 0},
};
static const Sint16 final_script_6[10] = {
#include "stg10/scripts/final_script_6.inc"
};
static const struct script_page final_runs_6[4] = {
    {TEXT_12, VALUE_14},
    {NULL, 0},
    {TEXT_13, VALUE_15},
    {NULL, 0},
};
static const Sint16 final_script_7[14] = {
#include "stg10/scripts/final_script_7.inc"
};
static const struct script_page final_runs_7[4] = {
    {TEXT_12, VALUE_16},
    {NULL, 0},
    {TEXT_13, VALUE_17},
    {NULL, 0},
};
static const Sint16 final_script_9[3] = {
#include "stg10/scripts/final_script_9.inc"
};
static const struct script_page final_runs_9[3] = {
    {TEXT_14, VALUE_18},
    {TEXT_15, VALUE_19},
    {NULL, 0},
};
static const Sint16 final_script_10[3] = {
#include "stg10/scripts/final_script_10.inc"
};
static const struct script_page final_runs_10[3] = {
    {TEXT_16, VALUE_20},
    {TEXT_17, VALUE_21},
    {NULL, 0},
};
static const Sint16 final_script_11[3] = {
#include "stg10/scripts/final_script_11.inc"
};
static const struct script_page final_runs_11[3] = {
    {TEXT_18, VALUE_22},
    {TEXT_19, VALUE_23},
    {NULL, 0},
};
static const Sint16 final_script_12[3] = {
#include "stg10/scripts/final_script_12.inc"
};
static const struct script_page final_runs_12[2] = {
    {TEXT_20, VALUE_24},
    {NULL, 0},
};
static const Sint16 final_script_13[3] = {
#include "stg10/scripts/final_script_13.inc"
};
static const struct script_page final_runs_13[2] = {
    {TEXT_21, VALUE_25},
    {NULL, 0},
};
static const Sint16 final_script_14[3] = {
#include "stg10/scripts/final_script_14.inc"
};
static const struct script_page final_runs_14[2] = {
    {TEXT_22, VALUE_26},
    {NULL, 0},
};
static const Sint16 final_script_15[3] = {
#include "stg10/scripts/final_script_15.inc"
};
static const struct script_page final_runs_15[2] = {
    {TEXT_23, VALUE_27},
    {NULL, 0},
};

/* 1ST_READ.BIN's stage_scripts names it for the stage */
const struct script_row final_script_table[FINAL_SCRIPT_ROWS] = {
    {NULL, (void *)final_runs_0},
    {(Sint16 *)final_script_1, (void *)final_runs_1},
    {(Sint16 *)final_script_2, (void *)final_runs_1},
    {(Sint16 *)final_script_3, (void *)final_runs_3},
    {(Sint16 *)final_script_4, (void *)final_runs_4},
    {(Sint16 *)final_script_5, (void *)final_runs_5},
    {(Sint16 *)final_script_6, (void *)final_runs_6},
    {(Sint16 *)final_script_7, (void *)final_runs_7},
    {NULL, NULL},
    {(Sint16 *)final_script_9, (void *)final_runs_9},
    {(Sint16 *)final_script_10, (void *)final_runs_10},
    {(Sint16 *)final_script_11, (void *)final_runs_11},
    {(Sint16 *)final_script_12, (void *)final_runs_12},
    {(Sint16 *)final_script_13, (void *)final_runs_13},
    {(Sint16 *)final_script_14, (void *)final_runs_14},
    {(Sint16 *)final_script_15, (void *)final_runs_15},
};
