/* Sonic's Station Square story section, BEACH - TAILS */
#include "adv00/script_sonic_beach.h"
#include "adv00/station_square.h"
#include "adv02/mystic_ruins.h"
#include "player/player.h"
#include "progress/playable.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_sonic_beach_init(void);
static void script_sonic_beach_step(int seq);

struct story_script script_sonic_beach = {script_sonic_beach_init,
                                          script_sonic_beach_step};

static void script_sonic_beach_init(void)
{
    story_set_time(TIME_DAY);
}

/* event 3 plays in the hotel act. once Emerald Coast is cleared, Tails is
 * playable after event 6, and the hotel's front, the station's front and the
 * train open. the section ends whatever story_going_to returns */
static void script_sonic_beach_step(int seq)
{
    switch (seq) {
    case 0:
        break;
    case 1:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_HOTEL)) {
            return;
        }
        story_play_event(3);
        break;
    case 2:
        if (!stage_cleared(STAGE_BEACH)) {
            return;
        }
        break;
    case 3:
        story_play_event(6);
        playable_unlock(CHARACTER_TAILS);
        event_flag_set(FLAG_SONIC_SS_HOTEL_FRONT);
        event_flag_set(FLAG_SONIC_SS_STATION_FRONT);
        event_flag_set(FLAG_SONIC_TRAIN);
        event_flag_set(FLAG_SONIC_UNK_82);
        break;
    case 4:
        story_going_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
