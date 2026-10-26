/* Sky Deck's script table. script_pick runs a row's script for an O HINT or
 * O TIKAL. the script picks the pages the hint shows */
#include <sg_xpt.h>

#include "event/script.h"
#include "stg06/scripts.h"

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
#define TEXT_15_LENGTH 56
#define TEXT_16_LENGTH 21
#define TEXT_17_LENGTH 54
#define TEXT_18_LENGTH 72
#define TEXT_19_LENGTH 52
#define TEXT_20_LENGTH 13
#define TEXT_21_LENGTH 33
#define TEXT_22_LENGTH 27
#define TEXT_23_LENGTH 60
#define TEXT_24_LENGTH 42
#define TEXT_25_LENGTH 42
#define TEXT_26_LENGTH 33
#define TEXT_27_LENGTH 15
#define TEXT_28_LENGTH 42
#define TEXT_29_LENGTH 42
#include "stg06/scripts/text.inc"
#include "stg06/scripts/values.inc"

/* each row's script, then its runs, in row order. a script's words are the
 * disc's, in its .inc file. a page's steps are the disc's too, in values.inc */
static const struct script_page sky_deck_runs_0[4] = {
    {TEXT_1, VALUE_1},
    {TEXT_2, VALUE_2},
    {TEXT_3, VALUE_3},
    {NULL, 0},
};
static const Sint16 sky_deck_script_1[3] = {
#include "stg06/scripts/sky_deck_script_1.inc"
};
static const Sint16 sky_deck_script_2[3] = {
#include "stg06/scripts/sky_deck_script_2.inc"
};
static const struct script_page sky_deck_runs_2[2] = {
    {TEXT_4, VALUE_4},
    {NULL, 0},
};
static const Sint16 sky_deck_script_3[5] = {
#include "stg06/scripts/sky_deck_script_3.inc"
};
static const struct script_page sky_deck_runs_3[3] = {
    {TEXT_5, VALUE_5},
    {TEXT_6, VALUE_6},
    {NULL, 0},
};
static const Sint16 sky_deck_script_4[18] = {
#include "stg06/scripts/sky_deck_script_4.inc"
};
static const struct script_page sky_deck_runs_4[9] = {
    {TEXT_7, VALUE_7}, {TEXT_8, VALUE_8},   {NULL, 0},
    {TEXT_9, VALUE_9}, {NULL, 0},           {TEXT_10, VALUE_10},
    {NULL, 0},         {TEXT_11, VALUE_11}, {NULL, 0},
};
static const Sint16 sky_deck_script_5[9] = {
#include "stg06/scripts/sky_deck_script_5.inc"
};
static const struct script_page sky_deck_runs_5[4] = {
    {TEXT_12, VALUE_12},
    {NULL, 0},
    {TEXT_13, VALUE_13},
    {NULL, 0},
};
static const Sint16 sky_deck_script_6[10] = {
#include "stg06/scripts/sky_deck_script_6.inc"
};
static const struct script_page sky_deck_runs_6[4] = {
    {TEXT_12, VALUE_14},
    {NULL, 0},
    {TEXT_13, VALUE_15},
    {NULL, 0},
};
static const Sint16 sky_deck_script_7[14] = {
#include "stg06/scripts/sky_deck_script_7.inc"
};
static const struct script_page sky_deck_runs_7[4] = {
    {TEXT_12, VALUE_16},
    {NULL, 0},
    {TEXT_13, VALUE_17},
    {NULL, 0},
};
static const Sint16 sky_deck_script_8[3] = {
#include "stg06/scripts/sky_deck_script_8.inc"
};
static const struct script_page sky_deck_runs_8[3] = {
    {TEXT_14, VALUE_18},
    {TEXT_15, VALUE_19},
    {NULL, 0},
};
static const Sint16 sky_deck_script_9[3] = {
#include "stg06/scripts/sky_deck_script_9.inc"
};
static const struct script_page sky_deck_runs_9[3] = {
    {TEXT_16, VALUE_20},
    {TEXT_17, VALUE_21},
    {NULL, 0},
};
static const Sint16 sky_deck_script_10[3] = {
#include "stg06/scripts/sky_deck_script_10.inc"
};
static const struct script_page sky_deck_runs_10[2] = {
    {TEXT_18, VALUE_22},
    {NULL, 0},
};
static const Sint16 sky_deck_script_11[3] = {
#include "stg06/scripts/sky_deck_script_11.inc"
};
static const struct script_page sky_deck_runs_11[2] = {
    {TEXT_19, VALUE_23},
    {NULL, 0},
};
static const struct script_page sky_deck_runs_12[5] = {
    {TEXT_20, VALUE_24}, {TEXT_21, VALUE_25}, {TEXT_22, VALUE_26},
    {TEXT_23, VALUE_27}, {NULL, 0},
};
static const struct script_page sky_deck_runs_13[4] = {
    {TEXT_20, VALUE_28},
    {TEXT_24, VALUE_29},
    {TEXT_25, VALUE_30},
    {NULL, 0},
};
static const struct script_page sky_deck_runs_14[4] = {
    {TEXT_20, VALUE_31},
    {TEXT_26, VALUE_32},
    {TEXT_27, VALUE_33},
    {NULL, 0},
};
static const struct script_page sky_deck_runs_15[4] = {
    {TEXT_20, VALUE_34},
    {TEXT_28, VALUE_35},
    {TEXT_29, VALUE_36},
    {NULL, 0},
};

const struct script_row sky_deck_script_table[SKY_DECK_SCRIPT_ROWS] = {
    {NULL, (void *)sky_deck_runs_0},
    {(Sint16 *)sky_deck_script_1, NULL},
    {(Sint16 *)sky_deck_script_2, (void *)sky_deck_runs_2},
    {(Sint16 *)sky_deck_script_3, (void *)sky_deck_runs_3},
    {(Sint16 *)sky_deck_script_4, (void *)sky_deck_runs_4},
    {(Sint16 *)sky_deck_script_5, (void *)sky_deck_runs_5},
    {(Sint16 *)sky_deck_script_6, (void *)sky_deck_runs_6},
    {(Sint16 *)sky_deck_script_7, (void *)sky_deck_runs_7},
    {(Sint16 *)sky_deck_script_8, (void *)sky_deck_runs_8},
    {(Sint16 *)sky_deck_script_9, (void *)sky_deck_runs_9},
    {(Sint16 *)sky_deck_script_10, (void *)sky_deck_runs_10},
    {(Sint16 *)sky_deck_script_11, (void *)sky_deck_runs_11},
    {NULL, (void *)sky_deck_runs_12},
    {NULL, (void *)sky_deck_runs_13},
    {NULL, (void *)sky_deck_runs_14},
    {NULL, (void *)sky_deck_runs_15},
};
