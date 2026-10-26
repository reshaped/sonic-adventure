/* the Egg Carrier outside's script table, and the people each act places */
#include <sg_xpt.h>

#include "adv0100/scripts.h"
#include "event/script.h"
#include "player/player.h"
#include "progress/progress.h"

#define TEXT_1_LENGTH 42
#define TEXT_2_LENGTH 35
#define TEXT_3_LENGTH 44
#define TEXT_4_LENGTH 51
#define TEXT_5_LENGTH 55
#define TEXT_6_LENGTH 57
#define TEXT_7_LENGTH 34
#define TEXT_8_LENGTH 20
#define TEXT_9_LENGTH 63
#define TEXT_10_LENGTH 47
#define TEXT_11_LENGTH 47
#define TEXT_12_LENGTH 63
#define TEXT_13_LENGTH 32
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 63
#define TEXT_16_LENGTH 40
#define TEXT_17_LENGTH 16
#define TEXT_18_LENGTH 36
#define TEXT_19_LENGTH 36
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 28
#define TEXT_22_LENGTH 43
#define TEXT_23_LENGTH 28
#include "adv0100/scripts/text.inc"

/* act 0's people, whose rows are 0 to 4 */
static const struct person_spot outside_people_act0[5] = {
    {{115.0F, 744.0F, -480.0F}, 0, -18263, 0, 0, PERSON_BIG},
    {{20.0F, 780.0F, -421.0F}, 0, -24576, 0, 1, PERSON_EGGMOBILE},
    {{-44.9F, 745.6F, -231.87F}, 718, 29278, 281, 2, PERSON_SONIC},
    {{0.0F, 780.0F, -386.0F}, 0, 16384, 0, 3, PERSON_EGGMOBILE},
    {{5.3F, 745.0F, -500.7F}, 0, 26624, 0, 4, PERSON_CHAOS4},
};

/* a person is placed when its row's script passes. the script passes when
 * the character played is one it names, and its events and flags are as it
 * asks. a row with null runs picks no pages, so its person shows no lines */
/* clang-format off */
static const Sint16 outside_script_0[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 263,
    SCRIPT_EVENTS, 28,
    SCRIPT_END,
};
static const Sint16 outside_script_1[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 263,
    SCRIPT_EVENTS, 28,
    SCRIPT_END,
};
static const Sint16 outside_script_2[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_EVENTS, SCRIPT_ON | 154,
    SCRIPT_EVENTS, 153,
    SCRIPT_END,
};
static const Sint16 outside_script_3[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_EVENTS, SCRIPT_ON | 154,
    SCRIPT_EVENTS, 153,
    SCRIPT_END,
};
static const Sint16 outside_script_4[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_EVENTS, SCRIPT_ON | 221,
    SCRIPT_EVENTS, 222,
    SCRIPT_END,
};
/* clang-format on */

/* act 2's people, whose rows are 5 to 11 */
static const struct person_spot outside_people_act2[7] = {
    {{-15.0F, 1525.687F, 3245.5F}, 0, 20448, 0, 5, PERSON_AMY},
    {{22.8F, 1535.69F, 3231.6F}, 0, 2304, 0, 6, PERSON_EGGMOBILE},
    {{-3.67F, 1525.69F, 3231.6F}, 0, -32768, 0, 7, PERSON_AMY},
    {{-43.18F, 1525.69F, 3348.78F}, 0, 12288, 0, 8, PERSON_TAILS},
    {{-3.18F, 1525.69F, 3390.0F}, 0, -32768, 0, 9, PERSON_E102},
    {{-8.0F, 1525.0F, 3284.0F}, 0, -32768, 0, 10, PERSON_AMY},
    {{20.0F, 1535.0F, 3234.0F}, 0, 0, 0, 11, PERSON_EGGMOBILE},
};

/* clang-format off */
static const Sint16 outside_script_5[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_CLEAR_SKY_DECK,
    SCRIPT_EVENTS, 27,
    SCRIPT_END,
};
static const Sint16 outside_script_6[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_CLEAR_SKY_DECK,
    SCRIPT_EVENTS, 27,
    SCRIPT_END,
};
static const Sint16 outside_script_7[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 27,
    SCRIPT_EVENTS, 263,
    SCRIPT_VOICE, 1103,
    SCRIPT_END,
};
static const struct script_page outside_runs_7[] = {
    {TEXT_1, 210},
    {NULL, 0},
};
static const Sint16 outside_script_8[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 27,
    SCRIPT_EVENTS, 263,
    SCRIPT_VOICE, 1102,
    SCRIPT_END,
};
static const struct script_page outside_runs_8[] = {
    {TEXT_2, 180},
    {NULL, 0},
};
static const Sint16 outside_script_9[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 27,
    SCRIPT_EVENTS, 263,
    SCRIPT_VOICE, 910,
    SCRIPT_END,
};
static const struct script_page outside_runs_9[] = {
    {TEXT_3, 240},
    {NULL, 0},
};
static const Sint16 outside_script_10[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_CLEAR_SKY_DECK,
    SCRIPT_EVENTS, 75,
    SCRIPT_END,
};
static const Sint16 outside_script_11[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_CLEAR_SKY_DECK,
    SCRIPT_EVENTS, 75,
    SCRIPT_END,
};
/* clang-format on */

const struct person_list egg_carrier_people_lists[7] = {
    {outside_people_act0, 5},
    {NULL, 0},
    {outside_people_act2, 7},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
};

/* rows 13 to 25, for Tikal's hints and the hint boxes. rows 13 to 20 play a
 * voice with their pages */
/* clang-format off */
static const Sint16 outside_script_13[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 27,
    SCRIPT_EVENTS, 263,
    SCRIPT_VOICE, 546,
    SCRIPT_END,
};
static const struct script_page outside_runs_13[] = {
    {TEXT_4, 120},
    {NULL, 0},
};
static const Sint16 outside_script_14[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 27,
    SCRIPT_EVENTS, 263,
    SCRIPT_VOICE, 547,
    SCRIPT_END,
};
static const struct script_page outside_runs_14[] = {
    {TEXT_5, 120},
    {NULL, 0},
};
static const Sint16 outside_script_15[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_EVENTS, SCRIPT_ON | 289,
    SCRIPT_EVENTS, 149,
    SCRIPT_VOICE, 548,
    SCRIPT_END,
};
static const struct script_page outside_runs_15[] = {
    {TEXT_6, 120},
    {NULL, 0},
};
static const Sint16 outside_script_16[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 258,
    SCRIPT_EVENTS, 259,
    SCRIPT_VOICE, 550,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_EVENTS, SCRIPT_ON | 274,
    SCRIPT_EVENTS, 275,
    SCRIPT_VOICE, 550,
    SCRIPT_END,
};
static const struct script_page outside_runs_16[] = {
    {TEXT_7, 60},
    {NULL, 0},
    {TEXT_7, 60},
    {NULL, 0},
};
static const Sint16 outside_script_17[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 27,
    SCRIPT_EVENTS, 263,
    SCRIPT_VOICE, 551,
    SCRIPT_END,
};
static const struct script_page outside_runs_17[] = {
    {TEXT_8, 60},
    {NULL, 0},
};
static const Sint16 outside_script_18[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_EVENTS, SCRIPT_ON | 289,
    SCRIPT_EVENTS, 149,
    SCRIPT_VOICE, 552,
    SCRIPT_END,
};
static const struct script_page outside_runs_18[] = {
    {TEXT_9, 120},
    {NULL, 0},
};
static const Sint16 outside_script_19[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_EVENTS, SCRIPT_ON | 152,
    SCRIPT_EVENTS, 153,
    SCRIPT_VOICE, 553,
    SCRIPT_END,
};
static const struct script_page outside_runs_19[] = {
    {TEXT_10, 120},
    {NULL, 0},
};
static const Sint16 outside_script_20[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_EVENTS, SCRIPT_ON | 263,
    SCRIPT_EVENTS, 28,
    SCRIPT_VOICE, 558,
    SCRIPT_END,
};
static const struct script_page outside_runs_20[] = {
    {TEXT_11, 120},
    {NULL, 0},
};
/* each character's branch tests that character's EC_MONORAIL flag, but
 * Tails' branch tests the next flag, FLAG_TAILS_EC_EGG_LIFT */
static const Sint16 outside_script_21[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_EC_MONORAIL,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_EC_EGG_LIFT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_EC_MONORAIL,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_AMY_EC_MONORAIL,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_BIG_EC_MONORAIL,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_E102_EC_MONORAIL,
    SCRIPT_BRANCH,
    SCRIPT_END,
};
static const struct script_page outside_runs_21[] = {
    {TEXT_12, 0},
    {NULL, 0},
    {TEXT_12, 0},
    {NULL, 0},
    {TEXT_12, 0},
    {NULL, 0},
    {TEXT_12, 0},
    {NULL, 0},
    {TEXT_12, 0},
    {NULL, 0},
    {TEXT_12, 0},
    {NULL, 0},
    {TEXT_13, 0},
    {NULL, 0},
};
static const Sint16 outside_script_22[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_BRANCH,
    SCRIPT_END,
};
static const struct script_page outside_runs_22[] = {
    {TEXT_14, 0},
    {TEXT_15, 0},
    {TEXT_16, 0},
    {NULL, 0},
    {TEXT_14, 0},
    {TEXT_17, 0},
    {NULL, 0},
};
static const Sint16 outside_script_23[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_BRANCH,
    SCRIPT_END,
};
static const struct script_page outside_runs_23[] = {
    {TEXT_18, 0},
    {NULL, 0},
    {TEXT_18, 0},
    {NULL, 0},
    {TEXT_19, 0},
    {NULL, 0},
};
/* each character's branch tests that character's BOAT flag, but Tails'
 * branch tests FLAG_TAILS_MBOSS_E102, two past FLAG_TAILS_BOAT */
static const Sint16 outside_script_24[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_BOAT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_MBOSS_E102,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_BOAT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_AMY_BOAT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_BIG_BOAT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_E102_BOAT,
    SCRIPT_BRANCH,
    SCRIPT_END,
};
static const struct script_page outside_runs_24[] = {
    {TEXT_20, 0},
    {TEXT_21, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {TEXT_21, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {TEXT_21, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {TEXT_21, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {TEXT_21, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {TEXT_21, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {TEXT_22, 0},
    {NULL, 0},
};
/* each character's branch tests that character's RAFT flag, but Tails'
 * branch tests FLAG_TAILS_MBOSS_KNUCKLES, two past FLAG_TAILS_RAFT */
static const Sint16 outside_script_25[] = {
    SCRIPT_CHARACTERS, 1 << CHARACTER_SONIC,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_SONIC_RAFT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_TAILS,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_TAILS_MBOSS_KNUCKLES,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_KNUCKLES,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_KNUCKLES_RAFT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_AMY,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_AMY_RAFT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_BIG,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_BIG_RAFT,
    SCRIPT_BRANCH,
    SCRIPT_CHARACTERS, 1 << CHARACTER_E102,
    SCRIPT_FLAGS, SCRIPT_ON | FLAG_E102_RAFT,
    SCRIPT_BRANCH,
    SCRIPT_END,
};
static const struct script_page outside_runs_25[] = {
    {TEXT_20, 0},
    {TEXT_23, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {TEXT_23, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {TEXT_23, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {TEXT_23, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {TEXT_23, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {TEXT_23, 0},
    {NULL, 0},
    {TEXT_20, 0},
    {TEXT_22, 0},
    {NULL, 0},
};
/* clang-format on */

/* the people's rows 0 to 11, then the hints' rows 13 to 25. row 12's null
 * script passes at once, and its null runs pick no pages */
const struct script_row egg_carrier_script_table[EGG_CARRIER_SCRIPT_ROWS] = {
    {(Sint16 *)outside_script_0, NULL},
    {(Sint16 *)outside_script_1, NULL},
    {(Sint16 *)outside_script_2, NULL},
    {(Sint16 *)outside_script_3, NULL},
    {(Sint16 *)outside_script_4, NULL},
    {(Sint16 *)outside_script_5, NULL},
    {(Sint16 *)outside_script_6, NULL},
    {(Sint16 *)outside_script_7, (void *)outside_runs_7},
    {(Sint16 *)outside_script_8, (void *)outside_runs_8},
    {(Sint16 *)outside_script_9, (void *)outside_runs_9},
    {(Sint16 *)outside_script_10, NULL},
    {(Sint16 *)outside_script_11, NULL},
    {NULL, NULL},
    {(Sint16 *)outside_script_13, (void *)outside_runs_13},
    {(Sint16 *)outside_script_14, (void *)outside_runs_14},
    {(Sint16 *)outside_script_15, (void *)outside_runs_15},
    {(Sint16 *)outside_script_16, (void *)outside_runs_16},
    {(Sint16 *)outside_script_17, (void *)outside_runs_17},
    {(Sint16 *)outside_script_18, (void *)outside_runs_18},
    {(Sint16 *)outside_script_19, (void *)outside_runs_19},
    {(Sint16 *)outside_script_20, (void *)outside_runs_20},
    {(Sint16 *)outside_script_21, (void *)outside_runs_21},
    {(Sint16 *)outside_script_22, (void *)outside_runs_22},
    {(Sint16 *)outside_script_23, (void *)outside_runs_23},
    {(Sint16 *)outside_script_24, (void *)outside_runs_24},
    {(Sint16 *)outside_script_25, (void *)outside_runs_25},
};
