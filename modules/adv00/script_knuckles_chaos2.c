/* Knuckles' Station Square story section, CHAOS2 */
#include "adv00/script_knuckles_chaos2.h"
#include "adv00/station_square.h"
#include "adv02/mystic_ruins.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_knuckles_chaos2_init(void);
static void script_knuckles_chaos2_step(int seq);

struct story_script script_knuckles_chaos2 = {script_knuckles_chaos2_init,
                                              script_knuckles_chaos2_step};

static void script_knuckles_chaos2_init(void)
{
    story_set_time(TIME_NIGHT);
}

/* event 134 plays in the casino act. after event 160 in the hotel act, the
 * lookout elevator opens. once Chaos 2 is cleared, the station's front and back
 * and the train open, and the section goes to the Mystic Ruins */
static void script_knuckles_chaos2_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO)) {
            return;
        }
        story_play_event(134);
        break;
    case 1:
        story_save();
        break;
    case 2:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_HOTEL)) {
            return;
        }
        story_play_event(160);
        event_flag_set(FLAG_KNUCKLES_LOOKOUT);
        break;
    case 3:
        if (!stage_cleared(STAGE_CHAOS2)) {
            return;
        }
        break;
    case 4:
        event_flag_set(FLAG_KNUCKLES_SS_STATION_FRONT);
        event_flag_set(FLAG_KNUCKLES_SS_STATION_BACK);
        event_flag_set(FLAG_KNUCKLES_TRAIN);
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
