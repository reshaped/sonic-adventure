/* the Egg Carrier inside's script table, and the people each act places */
#include <sg_xpt.h>

#include "adv0130/scripts.h"
#include "event/script.h"
#include "player/player.h"

#define TEXT_1_LENGTH 46
#define TEXT_2_LENGTH 18
#define TEXT_3_LENGTH 63
#define TEXT_4_LENGTH 22
#define TEXT_5_LENGTH 59
#define TEXT_6_LENGTH 16
#define TEXT_7_LENGTH 28
#define TEXT_8_LENGTH 59
#define TEXT_9_LENGTH 61
#define TEXT_10_LENGTH 47
#define TEXT_11_LENGTH 24
#define TEXT_12_LENGTH 47
#define TEXT_13_LENGTH 45
#define TEXT_14_LENGTH 24
#define TEXT_15_LENGTH 61
#define TEXT_16_LENGTH 21
#define TEXT_17_LENGTH 24
#define TEXT_18_LENGTH 20
#define TEXT_19_LENGTH 53
#define TEXT_20_LENGTH 26
#define TEXT_21_LENGTH 45
#define TEXT_22_LENGTH 41
#define TEXT_23_LENGTH 41
#define TEXT_24_LENGTH 24
#define TEXT_25_LENGTH 63
#define TEXT_26_LENGTH 63
#define TEXT_27_LENGTH 32
#define TEXT_28_LENGTH 53
#define TEXT_29_LENGTH 14
#define TEXT_30_LENGTH 24
#define TEXT_31_LENGTH 22
#define TEXT_32_LENGTH 24
#define TEXT_33_LENGTH 51
#define TEXT_34_LENGTH 59
#define TEXT_35_LENGTH 34
#define TEXT_36_LENGTH 30
#include "adv0130/scripts/text.inc"

/* E-102, who stands facing -x and speaks to Amy */
static const struct person_spot inside_people_act3[1] = {
    {{-123.0F, 0.0F, 31.0F}, 0, -16384, 0, 0, 0},
};

/* clang-format off */
static const Sint16 inside_script_0[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_EVENTS, SCRIPT_ON | 101,
    SCRIPT_FLAGS, 343,
    SCRIPT_VOICE, 1447,
    SCRIPT_END,
};
static const struct script_page inside_runs_0[] = {
    {TEXT_1, 260},
    {NULL, 0},
};

const struct person_list egg_carrier_inside_people_lists[6] = {
    {NULL, 0}, {NULL, 0}, {NULL, 0}, {inside_people_act3, 1}, {NULL, 0}, {NULL, 0},
};

/* rows 2 to 18 of egg_carrier_inside_script_table are the hints of Tikal and
 * of the hint boxes */
static const Sint16 inside_script_2[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_EVENTS, SCRIPT_ON | 320,
    SCRIPT_EVENTS, 193,
    SCRIPT_VOICE, 411,
    SCRIPT_END,
};
static const struct script_page inside_runs_2[] = {
    {TEXT_2, 60},
    {TEXT_3, 120},
    {NULL, 0},
};
static const Sint16 inside_script_3[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_EVENTS, SCRIPT_ON | 321,
    SCRIPT_FLAGS, 394,
    SCRIPT_VOICE, 412,
    SCRIPT_END,
};
static const struct script_page inside_runs_3[] = {
    {TEXT_4, 60},
    {TEXT_5, 120},
    {NULL, 0},
};
static const Sint16 inside_script_4[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_EVENTS, SCRIPT_ON | 305,
    SCRIPT_EVENTS, 102,
    SCRIPT_VOICE, 477,
    SCRIPT_END,
};
static const struct script_page inside_runs_4[] = {
    {TEXT_6, 60},
    {TEXT_7, 60},
    {TEXT_8, 120},
    {NULL, 0},
};
static const Sint16 inside_script_5[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_FLAGS, SCRIPT_ON | 331,
    SCRIPT_FLAGS, 343,
    SCRIPT_VOICE, 554,
    SCRIPT_END,
};
static const struct script_page inside_runs_5[] = {
    {TEXT_9, 120},
    {NULL, 0},
};
static const Sint16 inside_script_6[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_EVENTS, SCRIPT_ON | 321,
    SCRIPT_FLAGS, 414,
    SCRIPT_VOICE, 555,
    SCRIPT_END,
};
static const struct script_page inside_runs_6[] = {
    {TEXT_10, 120},
    {TEXT_11, 60},
    {NULL, 0},
};
static const Sint16 inside_script_7[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_EVENTS, SCRIPT_ON | 216,
    SCRIPT_EVENTS, 218,
    SCRIPT_VOICE, 556,
    SCRIPT_END,
};
static const struct script_page inside_runs_7[] = {
    {TEXT_12, 120},
    {TEXT_11, 60},
    {NULL, 0},
};
static const Sint16 inside_script_8[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_EVENTS, SCRIPT_ON | 218,
    SCRIPT_EVENTS, 223,
    SCRIPT_VOICE, 557,
    SCRIPT_END,
};
static const struct script_page inside_runs_8[] = {
    {TEXT_13, 120},
    {NULL, 0},
};
static const Sint16 inside_script_9[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_BRANCH,
    SCRIPT_END,
};
static const struct script_page inside_runs_9[] = {
    {TEXT_14, 0},
    {TEXT_15, 0},
    {NULL, 0},
    {TEXT_16, 0},
    {NULL, 0},
};
static const Sint16 inside_script_10[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_BRANCH,
    SCRIPT_END,
};
static const struct script_page inside_runs_10[] = {
    {TEXT_17, 0},
    {NULL, 0},
    {TEXT_18, 0},
    {TEXT_19, 0},
    {NULL, 0},
    {TEXT_18, 0},
    {TEXT_19, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {NULL, 0},
};
static const struct script_page inside_runs_11[] = {
    {TEXT_21, 0},
    {NULL, 0},
};
static const Sint16 inside_script_12[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_EVENTS, SCRIPT_ON | 321,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_BRANCH,
    SCRIPT_END,
};
static const struct script_page inside_runs_12[] = {
    {TEXT_22, 0},
    {NULL, 0},
    {TEXT_22, 0},
    {NULL, 0},
    {TEXT_22, 0},
    {NULL, 0},
    {TEXT_23, 0},
    {NULL, 0},
};
static const Sint16 inside_script_13[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_EVENTS, SCRIPT_ON | 320,
    SCRIPT_BRANCH,
    SCRIPT_END,
};
static const struct script_page inside_runs_13[] = {
    {TEXT_24, 0},
    {NULL, 0},
    {TEXT_16, 0},
    {NULL, 0},
};
static const struct script_page inside_runs_14[] = {
    {TEXT_25, 0},
    {NULL, 0},
};
static const Sint16 inside_script_15[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | 143,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | 207,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | 266,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_FLAGS, SCRIPT_ON | 328,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_FLAGS, SCRIPT_ON | 455,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_FLAGS, SCRIPT_ON | 390,
    SCRIPT_BRANCH,
    SCRIPT_END,
};
static const struct script_page inside_runs_15[] = {
    {TEXT_26, 0},
    {NULL, 0},
    {TEXT_26, 0},
    {NULL, 0},
    {TEXT_26, 0},
    {NULL, 0},
    {TEXT_26, 0},
    {NULL, 0},
    {TEXT_26, 0},
    {NULL, 0},
    {TEXT_26, 0},
    {NULL, 0},
    {TEXT_27, 0},
    {NULL, 0},
};
static const Sint16 inside_script_16[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_EVENTS, SCRIPT_ON | 101,
    SCRIPT_FLAGS, 331,
    SCRIPT_VOICE, 549,
    SCRIPT_END,
};
static const struct script_page inside_runs_16[] = {
    {TEXT_28, 120},
    {NULL, 0},
};
static const Sint16 inside_script_17[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 30,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_EVENTS, SCRIPT_ON | 83,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_EVENTS, SCRIPT_ON | 156,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_EVENTS, SCRIPT_ON | 107,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_EVENTS, SCRIPT_ON | 225,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_EVENTS, SCRIPT_ON | 196,
    SCRIPT_BRANCH,
    SCRIPT_END,
};
static const struct script_page inside_runs_17[] = {
    {TEXT_29, 0},
    {TEXT_30, 0},
    {NULL, 0},
    {TEXT_29, 0},
    {TEXT_30, 0},
    {NULL, 0},
    {TEXT_29, 0},
    {TEXT_30, 0},
    {NULL, 0},
    {TEXT_29, 0},
    {TEXT_30, 0},
    {NULL, 0},
    {TEXT_29, 0},
    {TEXT_30, 0},
    {NULL, 0},
    {TEXT_29, 0},
    {TEXT_30, 0},
    {NULL, 0},
    {TEXT_29, 0},
    {TEXT_31, 0},
    {NULL, 0},
};
static const Sint16 inside_script_18[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_BRANCH,
    SCRIPT_END,
};
static const struct script_page inside_runs_18[] = {
    {TEXT_32, 0},
    {TEXT_33, 0},
    {TEXT_34, 0},
    {TEXT_35, 0},
    {NULL, 0},
    {TEXT_32, 0},
    {TEXT_36, 0},
    {NULL, 0},
};
/* clang-format on */

/* rows 1, 11 and 14 have no script, so each passes at once. row 1's runs is
 * null as well, so it picks no pages. rows 11 and 14 pick their one run */
const struct script_row egg_carrier_inside_script_table[] = {
    {(Sint16 *)inside_script_0, (void *)inside_runs_0},
    {NULL, NULL},
    {(Sint16 *)inside_script_2, (void *)inside_runs_2},
    {(Sint16 *)inside_script_3, (void *)inside_runs_3},
    {(Sint16 *)inside_script_4, (void *)inside_runs_4},
    {(Sint16 *)inside_script_5, (void *)inside_runs_5},
    {(Sint16 *)inside_script_6, (void *)inside_runs_6},
    {(Sint16 *)inside_script_7, (void *)inside_runs_7},
    {(Sint16 *)inside_script_8, (void *)inside_runs_8},
    {(Sint16 *)inside_script_9, (void *)inside_runs_9},
    {(Sint16 *)inside_script_10, (void *)inside_runs_10},
    {NULL, (void *)inside_runs_11},
    {(Sint16 *)inside_script_12, (void *)inside_runs_12},
    {(Sint16 *)inside_script_13, (void *)inside_runs_13},
    {NULL, (void *)inside_runs_14},
    {(Sint16 *)inside_script_15, (void *)inside_runs_15},
    {(Sint16 *)inside_script_16, (void *)inside_runs_16},
    {(Sint16 *)inside_script_17, (void *)inside_runs_17},
    {(Sint16 *)inside_script_18, (void *)inside_runs_18},
};
