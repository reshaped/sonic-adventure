/* Windy Valley's script table. script_pick runs a row's script for an
 * O TIKAL. the script picks the pages the hint shows */
#include <sg_xpt.h>

#include "event/script.h"
#include "stg02/scripts.h"

#define TEXT_1_LENGTH 73
#define TEXT_2_LENGTH 63
#define TEXT_3_LENGTH 41
#define TEXT_4_LENGTH 71
#define TEXT_5_LENGTH 49
#define TEXT_6_LENGTH 63
#define TEXT_7_LENGTH 61
#define TEXT_8_LENGTH 31
#include "stg02/scripts/text.inc"
#include "stg02/scripts/values.inc"

/* each row's script, then its runs, in row order. a script's words are the
 * disc's, in its .inc file. a page's steps are the disc's too, in values.inc */
static const Sint16 windy_script_0[3] = {
#include "stg02/scripts/windy_script_0.inc"
};
static const struct script_page windy_runs_0[3] = {
    {TEXT_1, VALUE_1},
    {TEXT_2, VALUE_2},
    {NULL, 0},
};
static const Sint16 windy_script_1[3] = {
#include "stg02/scripts/windy_script_1.inc"
};
static const struct script_page windy_runs_1[2] = {
    {TEXT_3, VALUE_3},
    {NULL, 0},
};
static const Sint16 windy_script_2[3] = {
#include "stg02/scripts/windy_script_2.inc"
};
static const struct script_page windy_runs_2[2] = {
    {TEXT_4, VALUE_4},
    {NULL, 0},
};
static const Sint16 windy_script_3[3] = {
#include "stg02/scripts/windy_script_3.inc"
};
static const struct script_page windy_runs_3[2] = {
    {TEXT_5, VALUE_5},
    {NULL, 0},
};
static const Sint16 windy_script_4[3] = {
#include "stg02/scripts/windy_script_4.inc"
};
static const struct script_page windy_runs_4[2] = {
    {TEXT_6, VALUE_6},
    {NULL, 0},
};
static const Sint16 windy_script_5[3] = {
#include "stg02/scripts/windy_script_5.inc"
};
static const struct script_page windy_runs_5[2] = {
    {TEXT_7, VALUE_7},
    {NULL, 0},
};
static const Sint16 windy_script_6[3] = {
#include "stg02/scripts/windy_script_6.inc"
};
static const struct script_page windy_runs_6[2] = {
    {TEXT_8, VALUE_8},
    {NULL, 0},
};

const struct script_row windy_script_table[WINDY_SCRIPT_ROWS] = {
    {(Sint16 *)windy_script_0, (void *)windy_runs_0},
    {(Sint16 *)windy_script_1, (void *)windy_runs_1},
    {(Sint16 *)windy_script_2, (void *)windy_runs_2},
    {(Sint16 *)windy_script_3, (void *)windy_runs_3},
    {(Sint16 *)windy_script_4, (void *)windy_runs_4},
    {(Sint16 *)windy_script_5, (void *)windy_runs_5},
    {(Sint16 *)windy_script_6, (void *)windy_runs_6},
};
