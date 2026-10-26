/* E-102's Egg Carrier story section, 1ST MISSION */
#include "adv00/station_square.h"
#include "adv01obj/script_e102_mission.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_e102_mission_init(void);
static void script_e102_mission_step(int seq);

struct story_script script_e102_mission = {script_e102_mission_init,
                                           script_e102_mission_step};

static void script_e102_mission_init(void)
{
}

static void script_e102_mission_step(int seq)
{
    switch (seq) {
    case 0:
        story_save();
        break;
    case 1:
        if (!story_wait_at(STAGE_EGG_CARRIER_INSIDE, 1)) {
            return;
        }
        story_play_event(183);
        break;
    case 2:
        event_flag_set(FLAG_E102_SS_HOTEL_FRONT);
        story_go_to(STAGE_STATION_SQUARE, STATION_SQUARE_STATION);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
