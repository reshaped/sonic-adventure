/* story: each character's sections, and the scripts in the field modules
 * they run */
#include "progress/story.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"

/* a story_script by its address in its field's module */
#define SCRIPT(address) ((struct story_script *)(address))

#define LENGTH(story) ((int)(sizeof(story) / sizeof((story)[0])))

static void script_idle_init(void);
static void script_idle_step(int seq);

/* a script that does nothing. no table names it, and no code reads it */
static struct story_script script_idle = {script_idle_init, script_idle_step};

static struct story_section sonic_story[] = {
    {SCRIPT(0x0cb2ee58), STAGE_STATION_SQUARE, 3, 0, "OPENING  - CHAOS0"},
    {SCRIPT(0x0cb2ee60), STAGE_STATION_SQUARE, 4, 1, "BEACH    - TAILS"},
    {SCRIPT(0x0cb39920), STAGE_MYSTIC_RUINS, 0, 0, "MOBILE1  - WINDY"},
    {SCRIPT(0x0cb2ee68), STAGE_STATION_SQUARE, 3, 0, "SS SEWER - CASINO"},
    {SCRIPT(0x0cb2ee78), STAGE_STATION_SQUARE, 1, 0, "WAKE     - "},
    {SCRIPT(0x0cb39928), STAGE_MYSTIC_RUINS, 0, 0, "ICECAP   - SHOOTING"},
    {SCRIPT(0x0cb2ee80), STAGE_STATION_SQUARE, 4, 0, "AMY      - TWINKLE"},
    {SCRIPT(0x0cb2ee88), STAGE_STATION_SQUARE, 5, 0, "TWINKLE  - HIGHWAY"},
    {SCRIPT(0x0cb2ee90), STAGE_STATION_SQUARE, 3, 0, "HIGHWAY  - MR"},
    {SCRIPT(0x0cb39940), STAGE_MYSTIC_RUINS, 0, 0, "MOUNTAIN - SHOOTING2"},
    {SCRIPT(0x0c901bb8), STAGE_EGG_CARRIER, 0, 0, "EC       - SKYDECK"},
    {SCRIPT(0x0c901bd0), STAGE_EGG_CARRIER_INSIDE, 1, 0, "EC HALL  - CHAOS6"},
    {SCRIPT(0x0cb39948), STAGE_MYSTIC_RUINS, 2, 0, "LOST WORLD"},
    {SCRIPT(0x0c900240), STAGE_PAST, 2, 1, "PAST FIRE"},
    {SCRIPT(0x0cb39950), STAGE_MYSTIC_RUINS, 2, 0, "FINALEGG"},
    {SCRIPT(0x0cb39958), STAGE_MYSTIC_RUINS, 0, 0, "ENDING"},
};

static struct story_section tails_story[] = {
    {SCRIPT(0x0cb2ee98), STAGE_STATION_SQUARE, 3, 0, "OPENING  - BEACH"},
    {SCRIPT(0x0cb39960), STAGE_MYSTIC_RUINS, 0, 0, "MOBILE1  - WINDY"},
    {SCRIPT(0x0cb2eea0), STAGE_STATION_SQUARE, 1, 0, "CASINO   - HOOREE"},
    {SCRIPT(0x0cb2eea8), STAGE_STATION_SQUARE, 1, 0, "WAKE     - "},
    {SCRIPT(0x0cb39968), STAGE_MYSTIC_RUINS, 0, 0, "ICECAP   - SHOOTING"},
    {SCRIPT(0x0cb39980), STAGE_MYSTIC_RUINS, 2, 0, "LOSE     - SANDBOARD"},
    {SCRIPT(0x0c900248), STAGE_PAST, 0, 0, "PAST ... TICAL"},
    {SCRIPT(0x0cb39998), STAGE_MYSTIC_RUINS, 0, 0, "SHOOTING2"},
    {SCRIPT(0x0c901bd8), STAGE_EGG_CARRIER, 0, 0, "EC       - SKYDECK"},
    {SCRIPT(0x0c901bf0), STAGE_EGG_CARRIER_INSIDE, 1, 0, "EC HALL  - ESCAPE"},
    {SCRIPT(0x0cb2eeb0), STAGE_STATION_SQUARE, 3, 0, "HIGHWAY  - MOBILE2"},
    {SCRIPT(0x0cb399b0), STAGE_MYSTIC_RUINS, 0, 0, "ENDING"},
};

static struct story_section knuckles_story[] = {
    {SCRIPT(0x0cb399b8), STAGE_MYSTIC_RUINS, 1, 0, "OPENING"},
    {SCRIPT(0x0cb2eeb8), STAGE_STATION_SQUARE, 3, 0, "HIGHWAY  - CASINO"},
    {SCRIPT(0x0c900250), STAGE_PAST, 0, 1, "PAST ... TICAL"},
    {SCRIPT(0x0cb2eec0), STAGE_STATION_SQUARE, 1, 0, "CHAOS2"},
    {SCRIPT(0x0cb399cc), STAGE_MYSTIC_RUINS, 0, 0, "MOUNTAIN - CHAOS4"},
    {SCRIPT(0x0c900258), STAGE_PAST, 1, 0, "PAST ... CHAOS"},
    {SCRIPT(0x0cb399e4), STAGE_MYSTIC_RUINS, 1, 0, "M EMERALD"},
    {SCRIPT(0x0c901c04), STAGE_EGG_CARRIER, 0, 5, "EC       - SKYDECK"},
    {SCRIPT(0x0c900260), STAGE_PAST, 2, 0, "PAST ... FIRE"},
    {SCRIPT(0x0c901c10), STAGE_EGG_CARRIER, 5, 1, "CHAOS6"},
    {SCRIPT(0x0cb399f0), STAGE_MYSTIC_RUINS, 1, 0, "ENDING1"},
    {SCRIPT(0x0cb399f8), STAGE_MYSTIC_RUINS, 1, 0, "ENDING2"},
};

static struct story_section amy_story[] = {
    {SCRIPT(0x0cb2eec8), STAGE_STATION_SQUARE, 0, 0, "OPENING  - MOVIE"},
    {SCRIPT(0x0cb2eed0), STAGE_STATION_SQUARE, 0, 0, "SONIC    - TWINKLE"},
    {SCRIPT(0x0c901c18), STAGE_EGG_CARRIER_INSIDE, 3, 0,
     "MOGURA   - HOTSHELTER"},
    {SCRIPT(0x0c927ce0), STAGE_PAST, 0, 0, "PAST ... TICAL"},
    {SCRIPT(0x0c901c20), STAGE_EGG_CARRIER, 5, 0, "SINK EGGCARRIER"},
    {SCRIPT(0x0cb2eed8), STAGE_STATION_SQUARE, 3, 0, "SAVE BIRDS BROS"},
    {SCRIPT(0x0cb39a00), STAGE_MYSTIC_RUINS, 0, 0, "MR       - FINALEGG"},
    {SCRIPT(0x0cb39a18), STAGE_MYSTIC_RUINS, 3, 0, "GO EGGCARRIER"},
    {SCRIPT(0x0c901c28), STAGE_EGG_CARRIER, 0, 0, "ZERO"},
};

static struct story_section e102_story[] = {
    {SCRIPT(0x0cb39a38), STAGE_MYSTIC_RUINS, 3, 0, "BIRTH    - E101"},
    {SCRIPT(0x0c901c30), STAGE_EGG_CARRIER_INSIDE, 1, 0, "1ST MISSION"},
    {SCRIPT(0x0cb2ef00), STAGE_STATION_SQUARE, 3, 0, "BEACH"},
    {SCRIPT(0x0c900268), STAGE_PAST, 1, 0, "PAST ... CHAOS"},
    {SCRIPT(0x0c901c38), STAGE_EGG_CARRIER_INSIDE, 1, 0, "AMY      - SONIC"},
    {SCRIPT(0x0cb39a40), STAGE_MYSTIC_RUINS, 0, 0, "WINDY"},
    {SCRIPT(0x0cb39a48), STAGE_MYSTIC_RUINS, 0, 0, "MOUNTAIN"},
    {SCRIPT(0x0cb39a50), STAGE_MYSTIC_RUINS, 1, 0, "GO EGGCARRIER"},
    {SCRIPT(0x0c901c40), STAGE_EGG_CARRIER, 0, 0, "E-101 R"},
};

static struct story_section big_story[] = {
    {SCRIPT(0x0cb39a58), STAGE_MYSTIC_RUINS, 2, 0, "OPENING"},
    {SCRIPT(0x0cb2eee0), STAGE_STATION_SQUARE, 3, 0, "TWINKLE"},
    {SCRIPT(0x0cb39a60), STAGE_MYSTIC_RUINS, 0, 0, "ICECAP"},
    {SCRIPT(0x0cb2eef8), STAGE_STATION_SQUARE, 1, 0, "BEACH"},
    {SCRIPT(0x0c901c48), STAGE_EGG_CARRIER_INSIDE, 1, 0, "HOT SHELTER"},
    {SCRIPT(0x0c900270), STAGE_PAST, 1, 0, "PAST ... EMERALD"},
    {SCRIPT(0x0c901c50), STAGE_EGG_CARRIER_INSIDE, 1, 0, "CHAOS6"},
    {SCRIPT(0x0cb39a68), STAGE_MYSTIC_RUINS, 2, 0, "ENDING"},
};

static struct story_section last_story_sections[] = {
    {SCRIPT(0x0cb39a70), STAGE_MYSTIC_RUINS, 2, 0, "OPENING 1"},
    {SCRIPT(0x0cb39a88), STAGE_MYSTIC_RUINS, 2, 0, "OPENING 2"},
    {SCRIPT(0x0c927d08), STAGE_PAST, 2, 0, "CHAOS GET ANGRY"},
    {SCRIPT(0x0cb39aa0), STAGE_MYSTIC_RUINS, 1, 0, "LAST CHAOS-EMERALD"},
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
