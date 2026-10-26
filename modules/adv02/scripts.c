/* the Mystic Ruins' people, by act, and the field's script table, whose rows
 * pick what the people say */
#include <sg_xpt.h>

#include "adv02/people.h"
#include "adv02/scripts.h"
#include "event/script.h"

#define TEXT_1_LENGTH 34
#define TEXT_2_LENGTH 50
#define TEXT_3_LENGTH 40
#define TEXT_4_LENGTH 38
#define TEXT_5_LENGTH 38
#define TEXT_6_LENGTH 40
#define TEXT_7_LENGTH 30
#define TEXT_8_LENGTH 31
#define TEXT_9_LENGTH 55
#define TEXT_10_LENGTH 39
#define TEXT_11_LENGTH 47
#define TEXT_12_LENGTH 45
#define TEXT_13_LENGTH 63
#define TEXT_14_LENGTH 33
#define TEXT_15_LENGTH 47
#define TEXT_16_LENGTH 51
#define TEXT_17_LENGTH 57
#define TEXT_18_LENGTH 71
#define TEXT_19_LENGTH 35
#define TEXT_20_LENGTH 55
#define TEXT_21_LENGTH 51
#define TEXT_22_LENGTH 49
#include "adv02/scripts/text.inc"
#include "adv02/scripts/values.inc"

/* each act's people, and each row's script with the runs of pages its
 * branches pick. the build generates the spots and the scripts from the disc.
 * it generates each page's text and steps too, as the macros TEXT_n and
 * VALUE_n */
static const struct person_spot mystic_ruins_people_act0[8] = {
#include "adv02/scripts/mystic_ruins_people_act0.inc"
};
static const Sint16 mystic_ruins_script_0[14] = {
#include "adv02/scripts/mystic_ruins_script_0.inc"
};
static const Sint16 mystic_ruins_script_1[18] = {
#include "adv02/scripts/mystic_ruins_script_1.inc"
};
static const struct script_page mystic_ruins_runs_1[4] = {
    {TEXT_1, VALUE_1},
    {NULL, 0},
    {TEXT_1, VALUE_2},
    {NULL, 0},
};
static const Sint16 mystic_ruins_script_2[7] = {
#include "adv02/scripts/mystic_ruins_script_2.inc"
};
static const Sint16 mystic_ruins_script_3[7] = {
#include "adv02/scripts/mystic_ruins_script_3.inc"
};
static const Sint16 mystic_ruins_script_4[9] = {
#include "adv02/scripts/mystic_ruins_script_4.inc"
};
static const struct script_page mystic_ruins_runs_4[2] = {
    {TEXT_2, VALUE_3},
    {NULL, 0},
};
static const Sint16 mystic_ruins_script_5[9] = {
#include "adv02/scripts/mystic_ruins_script_5.inc"
};
static const struct script_page mystic_ruins_runs_5[2] = {
    {TEXT_3, VALUE_4},
    {NULL, 0},
};
static const Sint16 mystic_ruins_script_6[9] = {
#include "adv02/scripts/mystic_ruins_script_6.inc"
};
static const struct script_page mystic_ruins_runs_6[2] = {
    {TEXT_4, VALUE_5},
    {NULL, 0},
};
static const Sint16 mystic_ruins_script_7[9] = {
#include "adv02/scripts/mystic_ruins_script_7.inc"
};
static const struct script_page mystic_ruins_runs_7[2] = {
    {TEXT_5, VALUE_6},
    {NULL, 0},
};
static const struct person_spot mystic_ruins_people_act2[2] = {
#include "adv02/scripts/mystic_ruins_people_act2.inc"
};
static const Sint16 mystic_ruins_script_8[7] = {
#include "adv02/scripts/mystic_ruins_script_8.inc"
};
static const Sint16 mystic_ruins_script_9[9] = {
#include "adv02/scripts/mystic_ruins_script_9.inc"
};
static const struct script_page mystic_ruins_runs_9[2] = {
    {TEXT_6, VALUE_7},
    {NULL, 0},
};
static const struct person_spot mystic_ruins_people_act1[2] = {
#include "adv02/scripts/mystic_ruins_people_act1.inc"
};
static const Sint16 mystic_ruins_script_10[5] = {
#include "adv02/scripts/mystic_ruins_script_10.inc"
};
static const Sint16 mystic_ruins_script_11[5] = {
#include "adv02/scripts/mystic_ruins_script_11.inc"
};
static const struct person_spot mystic_ruins_people_act3[3] = {
#include "adv02/scripts/mystic_ruins_people_act3.inc"
};
static const Sint16 mystic_ruins_script_12[7] = {
#include "adv02/scripts/mystic_ruins_script_12.inc"
};
static const Sint16 mystic_ruins_script_13[7] = {
#include "adv02/scripts/mystic_ruins_script_13.inc"
};
static const Sint16 mystic_ruins_script_14[7] = {
#include "adv02/scripts/mystic_ruins_script_14.inc"
};
const struct person_list mystic_ruins_people_lists[4] = {
#include "adv02/scripts/mystic_ruins_people_lists.inc"
};
static const Sint16 mystic_ruins_script_15[9] = {
#include "adv02/scripts/mystic_ruins_script_15.inc"
};
static const struct script_page mystic_ruins_runs_15[2] = {
    {TEXT_7, VALUE_8},
    {NULL, 0},
};
static const Sint16 mystic_ruins_script_16[9] = {
#include "adv02/scripts/mystic_ruins_script_16.inc"
};
static const struct script_page mystic_ruins_runs_16[2] = {
    {TEXT_8, VALUE_9},
    {NULL, 0},
};
static const Sint16 mystic_ruins_script_17[9] = {
#include "adv02/scripts/mystic_ruins_script_17.inc"
};
static const struct script_page mystic_ruins_runs_17[2] = {
    {TEXT_9, VALUE_10},
    {NULL, 0},
};
static const Sint16 mystic_ruins_script_18[225] = {
#include "adv02/scripts/mystic_ruins_script_18.inc"
};
static const struct script_page mystic_ruins_runs_18[50] = {
    {TEXT_10, VALUE_11}, {NULL, 0}, {TEXT_11, VALUE_12}, {NULL, 0},
    {TEXT_12, VALUE_13}, {NULL, 0}, {TEXT_11, VALUE_14}, {NULL, 0},
    {TEXT_13, VALUE_15}, {NULL, 0}, {TEXT_14, VALUE_16}, {NULL, 0},
    {TEXT_9, VALUE_17},  {NULL, 0}, {TEXT_15, VALUE_18}, {NULL, 0},
    {TEXT_16, VALUE_19}, {NULL, 0}, {TEXT_11, VALUE_20}, {NULL, 0},
    {TEXT_12, VALUE_21}, {NULL, 0}, {TEXT_17, VALUE_22}, {NULL, 0},
    {TEXT_18, VALUE_23}, {NULL, 0}, {TEXT_11, VALUE_24}, {NULL, 0},
    {TEXT_19, VALUE_25}, {NULL, 0}, {TEXT_20, VALUE_26}, {NULL, 0},
    {TEXT_9, VALUE_27},  {NULL, 0}, {TEXT_15, VALUE_28}, {NULL, 0},
    {TEXT_21, VALUE_29}, {NULL, 0}, {TEXT_22, VALUE_30}, {NULL, 0},
    {TEXT_12, VALUE_31}, {NULL, 0}, {TEXT_11, VALUE_32}, {NULL, 0},
    {TEXT_16, VALUE_33}, {NULL, 0}, {TEXT_14, VALUE_34}, {NULL, 0},
    {TEXT_22, VALUE_35}, {NULL, 0},
};
static const Sint16 mystic_ruins_script_19[9] = {
#include "adv02/scripts/mystic_ruins_script_19.inc"
};
static const struct script_page mystic_ruins_runs_19[2] = {
    {TEXT_14, VALUE_36},
    {NULL, 0},
};
const struct script_row mystic_ruins_script_table[MYSTIC_RUINS_SCRIPT_ROWS] = {
    {(Sint16 *)mystic_ruins_script_0, NULL},
    {(Sint16 *)mystic_ruins_script_1, (void *)mystic_ruins_runs_1},
    {(Sint16 *)mystic_ruins_script_2, NULL},
    {(Sint16 *)mystic_ruins_script_3, NULL},
    {(Sint16 *)mystic_ruins_script_4, (void *)mystic_ruins_runs_4},
    {(Sint16 *)mystic_ruins_script_5, (void *)mystic_ruins_runs_5},
    {(Sint16 *)mystic_ruins_script_6, (void *)mystic_ruins_runs_6},
    {(Sint16 *)mystic_ruins_script_7, (void *)mystic_ruins_runs_7},
    {(Sint16 *)mystic_ruins_script_8, NULL},
    {(Sint16 *)mystic_ruins_script_9, (void *)mystic_ruins_runs_9},
    {(Sint16 *)mystic_ruins_script_10, NULL},
    {(Sint16 *)mystic_ruins_script_11, NULL},
    {(Sint16 *)mystic_ruins_script_12, NULL},
    {(Sint16 *)mystic_ruins_script_13, NULL},
    {(Sint16 *)mystic_ruins_script_14, NULL},
    {(Sint16 *)mystic_ruins_script_15, (void *)mystic_ruins_runs_15},
    {(Sint16 *)mystic_ruins_script_16, (void *)mystic_ruins_runs_16},
    {(Sint16 *)mystic_ruins_script_17, (void *)mystic_ruins_runs_17},
    {(Sint16 *)mystic_ruins_script_18, (void *)mystic_ruins_runs_18},
    {(Sint16 *)mystic_ruins_script_19, (void *)mystic_ruins_runs_19},
};
