/* Sonic's Mystic Ruins story section, MOUNTAIN - SHOOTING2 */
#include "adv02/mystic_ruins.h"
#include "adv02/script_sonic_mountain_shooting2.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_sonic_mountain_shooting2_init(void);
static void script_sonic_mountain_shooting2_step(int seq);

struct story_script script_sonic_mountain_shooting2 = {
    script_sonic_mountain_shooting2_init, script_sonic_mountain_shooting2_step};

static void script_sonic_mountain_shooting2_init(void)
{
    story_set_time(TIME_DAY);
}

/* the island door opens, and event 22 plays. the section ends once the game
 * is going to act 0 of Red Mountain */
static void script_sonic_mountain_shooting2_step(int seq)
{
    switch (seq) {
    case 0:
        player_record->unk_01 = 2;
        story_save();
        break;
    case 1:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        event_flag_set(FLAG_SONIC_MR_ISLAND_DOOR);
        break;
    case 2:
        story_play_event(22);
        break;
    case 3:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_ISLAND)) {
            return;
        }
        if (!story_going_to(STAGE_MOUNTAIN, 0)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
