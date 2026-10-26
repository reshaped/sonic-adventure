/* the stage's hint rows, which script_pick runs */
#include <sg_xpt.h>

#include "event/script.h"
#include "stg01/scripts.h"

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
#define TEXT_14_LENGTH 73
#define TEXT_15_LENGTH 63
#define TEXT_16_LENGTH 69
#define TEXT_17_LENGTH 47
#define TEXT_18_LENGTH 45
#include "stg01/scripts/text.inc"
#include "stg01/scripts/values.inc"

/* up to beach_script_table, each row's script, if it has one, and then its
 * runs of pages. the build reads the scripts' words and the pages' steps
 * from the disc */
static const struct script_page beach_runs_0[4] = {
    {TEXT_1, VALUE_1},
    {TEXT_2, VALUE_2},
    {TEXT_3, VALUE_3},
    {NULL, 0},
};
static const Sint16 beach_script_1[3] = {
#include "stg01/scripts/beach_script_1.inc"
};
static const Sint16 beach_script_2[3] = {
#include "stg01/scripts/beach_script_2.inc"
};
static const struct script_page beach_runs_2[2] = {
    {TEXT_4, VALUE_4},
    {NULL, 0},
};
static const Sint16 beach_script_3[5] = {
#include "stg01/scripts/beach_script_3.inc"
};
static const struct script_page beach_runs_3[3] = {
    {TEXT_5, VALUE_5},
    {TEXT_6, VALUE_6},
    {NULL, 0},
};
static const Sint16 beach_script_4[18] = {
#include "stg01/scripts/beach_script_4.inc"
};
static const struct script_page beach_runs_4[9] = {
    {TEXT_7, VALUE_7}, {TEXT_8, VALUE_8},   {NULL, 0},
    {TEXT_9, VALUE_9}, {NULL, 0},           {TEXT_10, VALUE_10},
    {NULL, 0},         {TEXT_11, VALUE_11}, {NULL, 0},
};
static const Sint16 beach_script_5[9] = {
#include "stg01/scripts/beach_script_5.inc"
};
static const struct script_page beach_runs_5[4] = {
    {TEXT_12, VALUE_12},
    {NULL, 0},
    {TEXT_13, VALUE_13},
    {NULL, 0},
};
static const Sint16 beach_script_6[10] = {
#include "stg01/scripts/beach_script_6.inc"
};
static const struct script_page beach_runs_6[4] = {
    {TEXT_12, VALUE_14},
    {NULL, 0},
    {TEXT_13, VALUE_15},
    {NULL, 0},
};
static const Sint16 beach_script_7[14] = {
#include "stg01/scripts/beach_script_7.inc"
};
static const struct script_page beach_runs_7[4] = {
    {TEXT_12, VALUE_16},
    {NULL, 0},
    {TEXT_13, VALUE_17},
    {NULL, 0},
};
static const Sint16 beach_script_8[3] = {
#include "stg01/scripts/beach_script_8.inc"
};
static const struct script_page beach_runs_8[3] = {
    {TEXT_14, VALUE_18},
    {TEXT_15, VALUE_19},
    {NULL, 0},
};
static const Sint16 beach_script_9[3] = {
#include "stg01/scripts/beach_script_9.inc"
};
static const struct script_page beach_runs_9[2] = {
    {TEXT_16, VALUE_20},
    {NULL, 0},
};
static const Sint16 beach_script_10[3] = {
#include "stg01/scripts/beach_script_10.inc"
};
static const struct script_page beach_runs_10[2] = {
    {TEXT_17, VALUE_21},
    {NULL, 0},
};
static const Sint16 beach_script_11[3] = {
#include "stg01/scripts/beach_script_11.inc"
};
static const struct script_page beach_runs_11[2] = {
    {TEXT_18, VALUE_22},
    {NULL, 0},
};

/* 1ST_READ.BIN's stage_scripts names it for the stage */
const struct script_row beach_script_table[BEACH_SCRIPT_ROWS] = {
    {NULL, (void *)beach_runs_0},
    {(Sint16 *)beach_script_1, NULL},
    {(Sint16 *)beach_script_2, (void *)beach_runs_2},
    {(Sint16 *)beach_script_3, (void *)beach_runs_3},
    {(Sint16 *)beach_script_4, (void *)beach_runs_4},
    {(Sint16 *)beach_script_5, (void *)beach_runs_5},
    {(Sint16 *)beach_script_6, (void *)beach_runs_6},
    {(Sint16 *)beach_script_7, (void *)beach_runs_7},
    {(Sint16 *)beach_script_8, (void *)beach_runs_8},
    {(Sint16 *)beach_script_9, (void *)beach_runs_9},
    {(Sint16 *)beach_script_10, (void *)beach_runs_10},
    {(Sint16 *)beach_script_11, (void *)beach_runs_11},
};
