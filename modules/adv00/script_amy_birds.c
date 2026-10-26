/* Amy's Station Square story section, SAVE BIRDS BROS */
#include "adv00/script_amy_birds.h"
#include "adv00/station_square.h"
#include "adv02/mystic_ruins.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_amy_birds_init(void);
static void script_amy_birds_step(int seq);

struct story_script script_amy_birds = {script_amy_birds_init,
                                        script_amy_birds_step};

static void script_amy_birds_init(void)
{
    story_set_time(TIME_EVENING);
}

/* after event 109 in the station act, the station's front and back and the
 * train open, and the Egg Carrier is marked sunk. the section then goes to the
 * Mystic Ruins */
static void script_amy_birds_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        story_play_event(109);
        break;
    case 1:
        event_flag_set(FLAG_AMY_SS_STATION_FRONT);
        event_flag_set(FLAG_AMY_SS_STATION_BACK);
        event_flag_set(FLAG_AMY_EC_SINK);
        event_flag_set(FLAG_AMY_TRAIN);
        story_save();
        break;
    case 2:
        stage_set_entrance(0);
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
