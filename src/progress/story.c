/* story: each character's sections, and the scripts in the field modules
 * they run */
#include "progress/story.h"
#include "adv00/script_amy_birds.h"
#include "adv00/script_amy_opening.h"
#include "adv00/script_amy_sonic.h"
#include "adv00/script_big_beach.h"
#include "adv00/script_big_twinkle.h"
#include "adv00/script_e102_beach.h"
#include "adv00/script_knuckles_chaos2.h"
#include "adv00/script_knuckles_highway.h"
#include "adv00/script_tails_casino.h"
#include "adv00/script_tails_highway.h"
#include "adv00/script_tails_opening.h"
#include "adv00/script_tails_wake.h"
#include "adv00/script_sonic_amy.h"
#include "adv00/script_sonic_beach.h"
#include "adv00/script_sonic_highway.h"
#include "adv00/script_sonic_opening.h"
#include "adv00/script_sonic_sewer.h"
#include "adv00/script_sonic_twinkle.h"
#include "adv00/script_sonic_wake.h"
#include "adv01obj/script_amy_mogura.h"
#include "adv01obj/script_amy_sink.h"
#include "adv01obj/script_amy_zero.h"
#include "adv01obj/script_big_chaos6.h"
#include "adv01obj/script_big_hot_shelter.h"
#include "adv01obj/script_e102_amy.h"
#include "adv01obj/script_e102_e101r.h"
#include "adv01obj/script_e102_mission.h"
#include "adv01obj/script_knuckles_chaos6.h"
#include "adv01obj/script_knuckles_skydeck.h"
#include "adv01obj/script_tails_escape.h"
#include "adv01obj/script_tails_skydeck.h"
#include "adv01obj/script_sonic_chaos6.h"
#include "adv01obj/script_sonic_skydeck.h"
#include "adv02/script_amy_go_egg_carrier.h"
#include "adv02/script_amy_mr_final_egg.h"
#include "adv02/script_big_ending.h"
#include "adv02/script_big_icecap.h"
#include "adv02/script_big_opening.h"
#include "adv02/script_e102_birth_e101.h"
#include "adv02/script_e102_go_egg_carrier.h"
#include "adv02/script_e102_mountain.h"
#include "adv02/script_e102_windy.h"
#include "adv02/script_knuckles_ending1.h"
#include "adv02/script_knuckles_ending2.h"
#include "adv02/script_knuckles_m_emerald.h"
#include "adv02/script_knuckles_mountain_chaos4.h"
#include "adv02/script_knuckles_opening.h"
#include "adv02/script_last_chaos_emerald.h"
#include "adv02/script_last_opening_1.h"
#include "adv02/script_last_opening_2.h"
#include "adv02/script_tails_ending.h"
#include "adv02/script_tails_icecap_shooting.h"
#include "adv02/script_tails_lose_sand_board.h"
#include "adv02/script_tails_mobile1_windy.h"
#include "adv02/script_tails_shooting2.h"
#include "adv02/script_sonic_ending.h"
#include "adv02/script_sonic_final_egg.h"
#include "adv02/script_sonic_icecap_shooting.h"
#include "adv02/script_sonic_lost_world.h"
#include "adv02/script_sonic_mobile1_windy.h"
#include "adv02/script_sonic_mountain_shooting2.h"
#include "adv03/script_amy_tikal.h"
#include "adv03/script_last_angry.h"
#include "adv03/scripts.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"

#define LENGTH(story) ((int)(sizeof(story) / sizeof((story)[0])))

static void script_idle_init(void);
static void script_idle_step(int seq);

/* a script that does nothing. no table names it, and no code reads it */
static struct story_script script_idle = {script_idle_init, script_idle_step};

static struct story_section sonic_story[] = {
    {&script_sonic_opening, STAGE_STATION_SQUARE, 3, 0, "OPENING  - CHAOS0"},
    {&script_sonic_beach, STAGE_STATION_SQUARE, 4, 1, "BEACH    - TAILS"},
    {&script_sonic_mobile1_windy, STAGE_MYSTIC_RUINS, 0, 0, "MOBILE1  - WINDY"},
    {&script_sonic_sewer, STAGE_STATION_SQUARE, 3, 0, "SS SEWER - CASINO"},
    {&script_sonic_wake, STAGE_STATION_SQUARE, 1, 0, "WAKE     - "},
    {&script_sonic_icecap_shooting, STAGE_MYSTIC_RUINS, 0, 0,
     "ICECAP   - SHOOTING"},
    {&script_sonic_amy, STAGE_STATION_SQUARE, 4, 0, "AMY      - TWINKLE"},
    {&script_sonic_twinkle, STAGE_STATION_SQUARE, 5, 0, "TWINKLE  - HIGHWAY"},
    {&script_sonic_highway, STAGE_STATION_SQUARE, 3, 0, "HIGHWAY  - MR"},
    {&script_sonic_mountain_shooting2, STAGE_MYSTIC_RUINS, 0, 0,
     "MOUNTAIN - SHOOTING2"},
    {&script_sonic_skydeck, STAGE_EGG_CARRIER, 0, 0, "EC       - SKYDECK"},
    {&script_sonic_chaos6, STAGE_EGG_CARRIER_INSIDE, 1, 0, "EC HALL  - CHAOS6"},
    {&script_sonic_lost_world, STAGE_MYSTIC_RUINS, 2, 0, "LOST WORLD"},
    {&script_sonic_fire, STAGE_PAST, 2, 1, "PAST FIRE"},
    {&script_sonic_final_egg, STAGE_MYSTIC_RUINS, 2, 0, "FINALEGG"},
    {&script_sonic_ending, STAGE_MYSTIC_RUINS, 0, 0, "ENDING"},
};

static struct story_section tails_story[] = {
    {&script_tails_opening, STAGE_STATION_SQUARE, 3, 0, "OPENING  - BEACH"},
    {&script_tails_mobile1_windy, STAGE_MYSTIC_RUINS, 0, 0, "MOBILE1  - WINDY"},
    {&script_tails_casino, STAGE_STATION_SQUARE, 1, 0, "CASINO   - HOOREE"},
    {&script_tails_wake, STAGE_STATION_SQUARE, 1, 0, "WAKE     - "},
    {&script_tails_icecap_shooting, STAGE_MYSTIC_RUINS, 0, 0,
     "ICECAP   - SHOOTING"},
    {&script_tails_lose_sand_board, STAGE_MYSTIC_RUINS, 2, 0,
     "LOSE     - SANDBOARD"},
    {&script_tails_tikal, STAGE_PAST, 0, 0, "PAST ... TICAL"},
    {&script_tails_shooting2, STAGE_MYSTIC_RUINS, 0, 0, "SHOOTING2"},
    {&script_tails_skydeck, STAGE_EGG_CARRIER, 0, 0, "EC       - SKYDECK"},
    {&script_tails_escape, STAGE_EGG_CARRIER_INSIDE, 1, 0, "EC HALL  - ESCAPE"},
    {&script_tails_highway, STAGE_STATION_SQUARE, 3, 0, "HIGHWAY  - MOBILE2"},
    {&script_tails_ending, STAGE_MYSTIC_RUINS, 0, 0, "ENDING"},
};

static struct story_section knuckles_story[] = {
    {&script_knuckles_opening, STAGE_MYSTIC_RUINS, 1, 0, "OPENING"},
    {&script_knuckles_highway, STAGE_STATION_SQUARE, 3, 0, "HIGHWAY  - CASINO"},
    {&script_knuckles_tikal, STAGE_PAST, 0, 1, "PAST ... TICAL"},
    {&script_knuckles_chaos2, STAGE_STATION_SQUARE, 1, 0, "CHAOS2"},
    {&script_knuckles_mountain_chaos4, STAGE_MYSTIC_RUINS, 0, 0,
     "MOUNTAIN - CHAOS4"},
    {&script_knuckles_chaos, STAGE_PAST, 1, 0, "PAST ... CHAOS"},
    {&script_knuckles_m_emerald, STAGE_MYSTIC_RUINS, 1, 0, "M EMERALD"},
    {&script_knuckles_skydeck, STAGE_EGG_CARRIER, 0, 5, "EC       - SKYDECK"},
    {&script_knuckles_fire, STAGE_PAST, 2, 0, "PAST ... FIRE"},
    {&script_knuckles_chaos6, STAGE_EGG_CARRIER, 5, 1, "CHAOS6"},
    {&script_knuckles_ending1, STAGE_MYSTIC_RUINS, 1, 0, "ENDING1"},
    {&script_knuckles_ending2, STAGE_MYSTIC_RUINS, 1, 0, "ENDING2"},
};

static struct story_section amy_story[] = {
    {&script_amy_opening, STAGE_STATION_SQUARE, 0, 0, "OPENING  - MOVIE"},
    {&script_amy_sonic, STAGE_STATION_SQUARE, 0, 0, "SONIC    - TWINKLE"},
    {&script_amy_mogura, STAGE_EGG_CARRIER_INSIDE, 3, 0,
     "MOGURA   - HOTSHELTER"},
    {&script_amy_tikal, STAGE_PAST, 0, 0, "PAST ... TICAL"},
    {&script_amy_sink, STAGE_EGG_CARRIER, 5, 0, "SINK EGGCARRIER"},
    {&script_amy_birds, STAGE_STATION_SQUARE, 3, 0, "SAVE BIRDS BROS"},
    {&script_amy_mr_final_egg, STAGE_MYSTIC_RUINS, 0, 0, "MR       - FINALEGG"},
    {&script_amy_go_egg_carrier, STAGE_MYSTIC_RUINS, 3, 0, "GO EGGCARRIER"},
    {&script_amy_zero, STAGE_EGG_CARRIER, 0, 0, "ZERO"},
};

static struct story_section e102_story[] = {
    {&script_e102_birth_e101, STAGE_MYSTIC_RUINS, 3, 0, "BIRTH    - E101"},
    {&script_e102_mission, STAGE_EGG_CARRIER_INSIDE, 1, 0, "1ST MISSION"},
    {&script_e102_beach, STAGE_STATION_SQUARE, 3, 0, "BEACH"},
    {&script_e102_chaos, STAGE_PAST, 1, 0, "PAST ... CHAOS"},
    {&script_e102_amy, STAGE_EGG_CARRIER_INSIDE, 1, 0, "AMY      - SONIC"},
    {&script_e102_windy, STAGE_MYSTIC_RUINS, 0, 0, "WINDY"},
    {&script_e102_mountain, STAGE_MYSTIC_RUINS, 0, 0, "MOUNTAIN"},
    {&script_e102_go_egg_carrier, STAGE_MYSTIC_RUINS, 1, 0, "GO EGGCARRIER"},
    {&script_e102_e101r, STAGE_EGG_CARRIER, 0, 0, "E-101 R"},
};

static struct story_section big_story[] = {
    {&script_big_opening, STAGE_MYSTIC_RUINS, 2, 0, "OPENING"},
    {&script_big_twinkle, STAGE_STATION_SQUARE, 3, 0, "TWINKLE"},
    {&script_big_icecap, STAGE_MYSTIC_RUINS, 0, 0, "ICECAP"},
    {&script_big_beach, STAGE_STATION_SQUARE, 1, 0, "BEACH"},
    {&script_big_hot_shelter, STAGE_EGG_CARRIER_INSIDE, 1, 0, "HOT SHELTER"},
    {&script_big_emerald, STAGE_PAST, 1, 0, "PAST ... EMERALD"},
    {&script_big_chaos6, STAGE_EGG_CARRIER_INSIDE, 1, 0, "CHAOS6"},
    {&script_big_ending, STAGE_MYSTIC_RUINS, 2, 0, "ENDING"},
};

static struct story_section last_story_sections[] = {
    {&script_last_opening_1, STAGE_MYSTIC_RUINS, 2, 0, "OPENING 1"},
    {&script_last_opening_2, STAGE_MYSTIC_RUINS, 2, 0, "OPENING 2"},
    {&script_last_angry, STAGE_PAST, 2, 0, "CHAOS GET ANGRY"},
    {&script_last_chaos_emerald, STAGE_MYSTIC_RUINS, 1, 0,
     "LAST CHAOS-EMERALD"},
};

struct story_section *story_get_table(int character)
{
    if (last_story) {
        return last_story_sections;
    }
    switch (character) {
    case CHARACTER_SONIC:
        return sonic_story;
    case CHARACTER_TAILS:
        return tails_story;
    case CHARACTER_KNUCKLES:
        return knuckles_story;
    case CHARACTER_AMY:
        return amy_story;
    case CHARACTER_E102:
        return e102_story;
    case CHARACTER_BIG:
        return big_story;
    }
    return NULL;
}

int story_get_length(int character)
{
    if (last_story) {
        return LENGTH(last_story_sections);
    }
    switch (character) {
    case CHARACTER_SONIC:
        return LENGTH(sonic_story);
    case CHARACTER_TAILS:
        return LENGTH(tails_story);
    case CHARACTER_KNUCKLES:
        return LENGTH(knuckles_story);
    case CHARACTER_AMY:
        return LENGTH(amy_story);
    case CHARACTER_E102:
        return LENGTH(e102_story);
    case CHARACTER_BIG:
        return LENGTH(big_story);
    }
    return 0;
}

static void script_idle_init(void)
{
}

static void script_idle_step(int seq)
{
}
