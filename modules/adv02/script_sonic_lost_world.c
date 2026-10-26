/* Sonic's Mystic Ruins story section, LOST WORLD */
#include "adv02/mystic_ruins.h"
#include "adv02/script_sonic_lost_world.h"
#include "player/player.h"
#include "progress/playable.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_sonic_lost_world_init(void);
static void script_sonic_lost_world_step(int seq);

struct story_script script_sonic_lost_world = {script_sonic_lost_world_init,
                                               script_sonic_lost_world_step};

static void script_sonic_lost_world_init(void)
{
    story_set_time(TIME_DAY);
}

/* event 30 plays. the last step clears the record's wait through
 * story_going_to, whose result goes unread */
static void script_sonic_lost_world_step(int seq)
{
    switch (seq) {
    case 0:
        event_flag_set(FLAG_SONIC_EC_SINK);
        event_flag_set(FLAG_SONIC_MR_TRUCK);
        event_flag_set(FLAG_SONIC_MR_ENTRANCE_RUIN);
        event_flag_set(FLAG_SONIC_RAFT);
        break;
    case 1:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE)) {
            return;
        }
        story_play_event(30);
        break;
    case 2:
        playable_unlock(CHARACTER_BIG);
        break;
    case 3:
        story_going_to(STAGE_RUIN, 0);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
