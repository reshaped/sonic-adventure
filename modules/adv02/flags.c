/* event flag names, and a debug display of the flags around a cursor */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/flags.h"
#include "debug/print.h"
#include "progress/progress.h"

/* the flags shown: the cursor's, and 7 on each side of it */
#define ROWS 15
#define FIRST_ROW 5 /* the screen row of the first flag shown */
#define COLUMN 2 /* the screen column the flags start at */

#define CURSOR_COLOR 0xffff8080
#define TEXT_COLOR 0xffffffff

/* in flags_mode */
#define MODE_SET 0x1 /* store flags_value in the cursor's flag */
#define MODE_SHOW 0x2 /* print the flags around the cursor */

/* flags_value while no value waits to be stored */
#define NO_VALUE (-1)

/* a character's run of count flags from flag first, and their names */
struct flag_names {
    int first;
    int count;
    char **names;
};

/* the display's cursor, waiting value and modes. no code sets them, so
 * flags_print neither stores nor prints anything */
static Uint32 flags_cursor = 0;
static int flags_value = NO_VALUE;
static Uint32 flags_mode = 0;
static struct flag_names flag_names[7];

void flags_print(void)
{
    Uint32 cursor = flags_cursor;
    Uint32 flag;
    int row;
    int y;

    if (cursor >= FLAG_COUNT) {
        cursor = FLAG_COUNT - 1;
    }
    if (flags_mode & MODE_SET) {
        if (flags_value != NO_VALUE) {
            event_flags[cursor] = flags_value;
            flags_value = NO_VALUE;
        }
    }
    if (flags_mode & MODE_SHOW) {
        /* @hack spelling: y = y + 1, which keeps y in R13, as MOV #5,R13 at
         * 0x0c9034ae sets it; as y++, y takes R11 and 38 of 218 words
         * differ */
        for (flag = cursor - (ROWS / 2), row = 0, y = FIRST_ROW; row < ROWS;
             row++, flag++, y = y + 1) {
            if (flag < FLAG_COUNT) {
                if (flag == cursor) {
                    njPrintColor(CURSOR_COLOR);
                } else {
                    njPrintColor(TEXT_COLOR);
                }
                debug_print(NJM_LOCATION(COLUMN, y), "(%02x)=%02x:%s", flag,
                            event_flags[flag], flags_name(flag));
            }
        }
        njPrintColor(TEXT_COLOR);
    }
}

char *flags_name(int flag)
{
    struct flag_names *run = flag_names;
    char *name = "UNKNOWN";

    /* NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion) */
    while (run->names != NULL) {
        int first = run->first;
        int count = run->count;

        if (first <= flag && flag < first + count) {
            name = run->names[flag - first];
            break;
        }
        run++;
    }
    return name;
}

static char *sonic_flags[53] = {
    "FLAG_SONIC_SS_BARRICADE",
    "FLAG_SONIC_SS_HOTEL_FRONT",
    "FLAG_SONIC_SS_STATION_FRONT",
    "FLAG_SONIC_SS_ENTRANCE_SEWER",
    "FLAG_SONIC_SS_EXIT_SEWER",
    "FLAG_SONIC_SS_ICESTONE",
    "FLAG_SONIC_SS_HOTEL_BACK",
    "FLAG_SONIC_SS_ENTRANCE_CASINO",
    "FLAG_SONIC_SS_STATION_BACK",
    "FLAG_SONIC_SS_TPARK_ELEVATOR",
    "FLAG_SONIC_SS_ENTRANCE_CIRCUIT",
    "FLAG_SONIC_SS_CARD",
    "FLAG_SONIC_SS_ENTRANCE_HIGHWAY",
    "FLAG_SONIC_SS_LIGHTSHOOSE",
    "FLAG_SONIC_SS_CRYSTALRING",
    "FLAG_SONIC_EC_MONORAIL",
    "FLAG_SONIC_EC_EGGLIFT",
    "FLAG_SONIC_EC_TRANSFORM",
    "FLAG_SONIC_EC_TORNADO2_LOST",
    "FLAG_SONIC_EC_SINK",
    "FLAG_SONIC_MR_WINDYSTONE",
    "FLAG_SONIC_MR_ENTRANCE_WINDY",
    "FLAG_SONIC_MR_WESTROCK",
    "FLAG_SONIC_MR_ICESTONE",
    "FLAG_SONIC_MR_ENTRANCE_ICECAP",
    "FLAG_SONIC_MR_ENTRANCE_MOUNTAIN",
    "FLAG_SONIC_MR_ISLANDDOOR",
    "FLAG_SONIC_MR_TRUCK",
    "FLAG_SONIC_MR_ENTRANCE_RUIN",
    "FLAG_SONIC_MR_APPEAR_FINALEGG",
    "FLAG_SONIC_MR_ENTRANCE_SANDBOARD",
    "FLAG_SONIC_MR_ANCIENT_LIGHT",
    "FLAG_SONIC_MR_ENTRANCE_FINALEGG",
    "FLAG_SONIC_TRAIN",
    "FLAG_SONIC_BOAT",
    "FLAG_SONIC_RAFT",
    "FLAG_SONIC_MBOSS_E102",
    "FLAG_SONIC_MBOSS_KNUCKLES",
    "FLAG_SONIC_CLEAR_BEACH",
    "FLAG_SONIC_CLEAR_WINDY",
    "FLAG_SONIC_CLEAR_CASINO",
    "FLAG_SONIC_CLEAR_TWINKLEPARK",
    "FLAG_SONIC_CLEAR_HIGHWAY",
    "FLAG_SONIC_CLEAR_MOUNTAIN",
    "FLAG_SONIC_CLEAR_SNOW",
    "FLAG_SONIC_CLEAR_SKYDECK",
    "FLAG_SONIC_CLEAR_RUIN",
    "FLAG_SONIC_CLEAR_FINALEGG",
    "FLAG_SONIC_CLEAR_CHAOS0",
    "FLAG_SONIC_CLEAR_CHAOS4",
    "FLAG_SONIC_CLEAR_CHAOS6",
    "FLAG_SONIC_CLEAR_EGGMOBILE1",
    "FLAG_SONIC_CLEAR_EGGMOBILE3",
};

static char *tails_flags[41] = {
    "FLAG_MILES_SS_BARRICADE",
    "FLAG_MILES_SS_HOTEL_FRONT",
    "FLAG_MILES_SS_HOTEL_BACK",
    "FLAG_MILES_SS_HOTEL_POOL",
    "FLAG_MILES_SS_ICESTONE",
    "FLAG_MILES_SS_ENTRANCE_CASINO",
    "FLAG_MILES_SS_ENTRANCE_HIGHWAY",
    "FLAG_MILES_SS_TPARK_ELEVATOR",
    "FLAG_MILES_SS_ENTRANCE_CIRCUIT",
    "FLAG_MILES_SS_CARTPASSPORT",
    "FLAG_MILES_SS_STATION_FRONT",
    "FLAG_MILES_SS_STATION_BACK",
    "FLAG_MILES_SS_HOTELSWITCH",
    "FLAG_MILES_SS_JETANKLET",
    "FLAG_MILES_EC_MONORAIL",
    "FLAG_MILES_EC_EGGLIFT",
    "FLAG_MILES_EC_TRANSFORM",
    "FLAG_MILES_EC_TORNADO2_LOST",
    "FLAG_MILES_EC_SINK",
    "FLAG_MILES_MR_WINDYSTONE",
    "FLAG_MILES_MR_ENTRANCE_WINDY",
    "FLAG_MILES_MR_WESTROCK",
    "FLAG_MILES_MR_ICESTONE",
    "FLAG_MILES_MR_ENTRANCE_ICECAP",
    "FLAG_MILES_MR_TRUCK",
    "FLAG_MILES_MR_ENTRANCE_RUIN",
    "FLAG_MILES_MR_ENTRANCE_SANDBOARD",
    "FLAG_MILES_MR_RHYTHMBROOCH",
    "FLAG_MILES_TRAIN",
    "FLAG_MILES_BOAT",
    "FLAG_MILES_RAFT",
    "FLAG_MILES_MBOSS_E102",
    "FLAG_MILES_MBOSS_KNUCKLES",
    "FLAG_MILES_CLEAR_WINDY",
    "FLAG_MILES_CLEAR_CASINO",
    "FLAG_MILES_CLEAR_HIGHWAY",
    "FLAG_MILES_CLEAR_SNOW",
    "FLAG_MILES_CLEAR_SKYDECK",
    "FLAG_MILES_CLEAR_SANDBOARD",
    "FLAG_MILES_CLEAR_CHAOS4",
    "FLAG_MILES_CLEAR_EGGMOBILE2",
};

static char *knuckles_flags[38] = {
    "FLAG_KNUCKLES_SS_BARRICADE",
    "FLAG_KNUCKLES_SS_HOTEL_FRONT",
    "FLAG_KNUCKLES_SS_HOTEL_BACK",
    "FLAG_KNUCKLES_SS_ENTRANCE_CASINO",
    "FLAG_KNUCKLES_SS_ENTRANCE_HIGHWAY",
    "FLAG_KNUCKLES_SS_TPARK_ELEVATOR",
    "FLAG_KNUCKLES_SS_ENTRANCE_CIRCUIT",
    "FLAG_KNUCKLES_SS_CARTPASSPORT",
    "FLAG_KNUCKLES_SS_STATION_FRONT",
    "FLAG_KNUCKLES_SS_STATION_BACK",
    "FLAG_KNUCKLES_EC_MONORAIL",
    "FLAG_KNUCKLES_EC_EGGLIFT",
    "FLAG_KNUCKLES_EC_TRANSFORM",
    "FLAG_KNUCKLES_EC_PALMSWITCH",
    "FLAG_KNUCKLES_EC_TORNADO2_LOST",
    "FLAG_KNUCKLES_EC_SINK",
    "FLAG_KNUCKLES_MR_ENTRANCE_MOUNTAIN",
    "FLAG_KNUCKLES_MR_WESTROCK",
    "FLAG_KNUCKLES_MR_ISLANDDOOR",
    "FLAG_KNUCKLES_MR_MONKEYDOOR_ENTER",
    "FLAG_KNUCKLES_MR_MONKEYCAGEA_BOMB",
    "FLAG_KNUCKLES_MR_TRUCK",
    "FLAG_KNUCKLES_MR_REDCUBE",
    "FLAG_KNUCKLES_MR_BLUECUBE",
    "FLAG_KNUCKLES_MR_ENTRANCE_RUIN",
    "FLAG_KNUCKLES_MR_APPEAR_FINALEGG",
    "FLAG_KNUCKLES_MR_SHOVELCLAW",
    "FLAG_KNUCKLES_MR_FIGHTINGGROVE",
    "FLAG_KNUCKLES_TRAIN",
    "FLAG_KNUCKLES_BOAT",
    "FLAG_KNUCKLES_RAFT",
    "FLAG_KNUCKLES_MBOSS_SONIC",
    "FLAG_KNUCKLES_CLEAR_CASINO",
    "FLAG_KNUCKLES_CLEAR_HIGHWAY",
    "FLAG_KNUCKLES_CLEAR_MOUNTAIN",
    "FLAG_KNUCKLES_CLEAR_RUIN",
    "FLAG_KNUCKLES_CLEAR_CHAOS2",
    "FLAG_KNUCKLES_CLEAR_CHAOS6",
};

static char *amy_flags[26] = {
    "FLAG_AMY_SS_HOTEL_FRONT",
    "FLAG_AMY_SS_HOTEL_BACK",
    "FLAG_AMY_SS_HOTEL_POOL",
    "FLAG_AMY_SS_ENTRANCE_CASINO",
    "FLAG_AMY_SS_TPARK_ELEVATOR",
    "FLAG_AMY_SS_ENTRANCE_CIRCUIT",
    "FLAG_AMY_SS_STATION_FRONT",
    "FLAG_AMY_SS_STATION_BACK",
    "FLAG_AMY_EC_MONORAIL",
    "FLAG_AMY_EC_EGGLIFT",
    "FLAG_AMY_EC_TRANSFORM",
    "FLAG_AMY_EC_MOGURATATAKI",
    "FLAG_AMY_EC_TORNADO2_LOST",
    "FLAG_AMY_EC_SINK",
    "FLAG_AMY_MR_ISLANDDOOR",
    "FLAG_AMY_MR_TRUCK",
    "FLAG_AMY_MR_APPEAR_FINALEGG",
    "FLAG_AMY_MR_ENTRANCE_FINALEGG",
    "FLAG_AMY_MR_FIGHTERSFEATHER",
    "FLAG_AMY_TRAIN",
    "FLAG_AMY_BOAT",
    "FLAG_AMY_RAFT",
    "FLAG_AMY_CLEAR_TWINKLEPARK",
    "FLAG_AMY_CLEAR_HOTSHELTER",
    "FLAG_AMY_CLEAR_FINALEGG",
    "FLAG_AMY_CLEAR_ZERO",
};

static char *e102_flags[34] = {
    "FLAG_E102_SS_HOTEL_FRONT",
    "FLAG_E102_SS_HOTEL_POOL",
    "FLAG_E102_SS_STATION_FRONT",
    "FLAG_E102_SS_TPARK_ELEVATOR",
    "FLAG_E102_SS_ENTRANCE_CIRCUIT",
    "FLAG_E102_SS_CARTPASSPORT",
    "FLAG_E102_EC_MONORAIL",
    "FLAG_E102_EC_EGGLIFT",
    "FLAG_E102_EC_TRANSFORM",
    "FLAG_E102_EC_BOOSTER",
    "FLAG_E102_EC_TYPE3LASER",
    "FLAG_E102_EC_TORNADO2_LOST",
    "FLAG_E102_EC_SINK",
    "FLAG_E102_EC_HOTSHELTER",
    "FLAG_E102_MR_GOODSCORE",
    "FLAG_E102_MR_FREEPASS",
    "FLAG_E102_MR_WINDYSTONE",
    "FLAG_E102_MR_ENTRANCE_WINDY",
    "FLAG_E102_MR_WESTROCK",
    "FLAG_E102_MR_ENTRANCE_MOUNTAIN",
    "FLAG_E102_MR_ISLANDDOOR",
    "FLAG_E102_MR_TRUCK",
    "FLAG_E102_MR_APPEAR_FINALEGG",
    "FLAG_E102_TRAIN",
    "FLAG_E102_BOAT",
    "FLAG_E102_RAFT",
    "FLAG_E102_MBOSS_SONIC",
    "FLAG_E102_CLEAR_BEACH",
    "FLAG_E102_CLEAR_WINDY",
    "FLAG_E102_CLEAR_MOUNTAIN",
    "FLAG_E102_CLEAR_HOTSHELTER",
    "FLAG_E102_CLEAR_FINALEGG",
    "FLAG_E102_CLEAR_E101",
    "FLAG_E102_CLEAR_E101R",
};

static char *big_flags[25] = {
    "FLAG_BIG_SS_HOTEL_FRONT",
    "FLAG_BIG_SS_HOTEL_BACK",
    "FLAG_BIG_SS_TPARK_ELEVATOR",
    "FLAG_BIG_SS_ENTRANCE_CIRCUIT",
    "FLAG_BIG_SS_STATION_FRONT",
    "FLAG_BIG_SS_ICESTONE",
    "FLAG_BIG_SS_HOTEL_POOL",
    "FLAG_BIG_EC_MONORAIL",
    "FLAG_BIG_EC_EGGLIFT",
    "FLAG_BIG_EC_TRANSFORM",
    "FLAG_BIG_EC_TORNADO2_LOST",
    "FLAG_BIG_EC_SINK",
    "FLAG_BIG_MR_WESTROCK",
    "FLAG_BIG_MR_ICESTONE",
    "FLAG_BIG_MR_ENTRANCE_ICECAP",
    "FLAG_BIG_MR_TRUCK",
    "FLAG_BIG_MR_LIFEBELT",
    "FLAG_BIG_MR_POWERROD",
    "FLAG_BIG_TRAIN",
    "FLAG_BIG_BOAT",
    "FLAG_BIG_RAFT",
    "FLAG_BIG_CLEAR_BEACH",
    "FLAG_BIG_CLEAR_HOTSHELTER",
    "FLAG_BIG_CLEAR_TWINKLEPARK",
    "FLAG_BIG_CLEAR_SNOW",
};

/* each character's flags, Sonic's to Big's, ended by an entry with no
 * names */
static struct flag_names flag_names[7] = {
    {128, 53, sonic_flags}, {192, 41, tails_flags}, {256, 38, knuckles_flags},
    {320, 26, amy_flags},   {384, 34, e102_flags},  {448, 25, big_flags},
    {0xffff, 0, NULL},
};
