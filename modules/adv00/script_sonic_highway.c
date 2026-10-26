/* Sonic's Station Square story section, HIGHWAY - MR */
#include "adv00/script_sonic_highway.h"
#include "adv00/station_square.h"
#include "adv02/mystic_ruins.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_sonic_highway_init(void);
static void script_sonic_highway_step(int seq);

struct story_script script_sonic_highway = {script_sonic_highway_init,
                                            script_sonic_highway_step};

static void script_sonic_highway_init(void)
{
    if (stage_cleared(STAGE_HIGHWAY)) {
        story_set_time(TIME_DAY);
    } else {
        story_set_time(TIME_NIGHT);
    }
}

/* once Speed Highway is cleared, event 21 plays in the casino act, and the
 * train and the station's front and back open again. the section ends as the
 * game goes to the Mystic Ruins */
static void script_sonic_highway_step(int seq)
{
    switch (seq) {
    case 0:
        if (!stage_cleared(STAGE_HIGHWAY)) {
            return;
        }
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO)) {
            return;
        }
        story_play_event(21);
        event_flag_set(FLAG_SONIC_TRAIN);
        event_flag_set(FLAG_SONIC_UNK_82);
        event_flag_set(FLAG_SONIC_SS_STATION_FRONT);
        event_flag_set(FLAG_SONIC_SS_STATION_BACK);
        break;
    case 1:
        if (!story_going_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
