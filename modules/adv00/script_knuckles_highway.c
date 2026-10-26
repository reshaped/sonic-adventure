/* Knuckles' Station Square story section, HIGHWAY - CASINO */
#include "adv00/script_knuckles_highway.h"
#include "adv00/station_square.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_knuckles_highway_init(void);
static void script_knuckles_highway_step(int seq);

struct story_script script_knuckles_highway = {script_knuckles_highway_init,
                                               script_knuckles_highway_step};

static void script_knuckles_highway_init(void)
{
    if (stage_cleared(STAGE_HIGHWAY)) {
        story_set_time(TIME_NIGHT);
    } else {
        story_set_time(TIME_DAY);
    }
}

/* after event 130 in the station act, the hotel's front opens, and its back
 * once Speed Highway is cleared. the section ends as the game goes from the
 * casino act to Casinopolis */
static void script_knuckles_highway_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        story_play_event(130);
        break;
    case 1:
        event_flag_set(FLAG_KNUCKLES_UNK_84);
        event_flag_set(FLAG_KNUCKLES_SS_HOTEL_FRONT);
        story_save();
        break;
    case 2:
        break;
    case 3:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_OFFICE)) {
            return;
        }
        if (!stage_cleared(STAGE_HIGHWAY)) {
            return;
        }
        event_flag_set(FLAG_KNUCKLES_SS_HOTEL_BACK);
        break;
    case 4:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO)) {
            return;
        }
        if (!story_going_to(STAGE_CASINO, 0)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
