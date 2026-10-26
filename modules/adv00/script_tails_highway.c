/* Tails' Station Square story section, HIGHWAY - MOBILE2 */
#include "adv00/script_tails_highway.h"
#include "adv00/station_square.h"
#include "adv02/mystic_ruins.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_tails_highway_init(void);
static void script_tails_highway_step(int seq);

struct story_script script_tails_highway = {script_tails_highway_init,
                                            script_tails_highway_step};

static void script_tails_highway_init(void)
{
    if (stage_cleared(STAGE_HIGHWAY)) {
        story_set_time(TIME_NIGHT);
    } else {
        story_set_time(TIME_EVENING);
    }
}

/* after event 77 in the station act, the train closes and the Egg Carrier is
 * marked sunk. event 78 leads to Speed Highway by night, and once it is
 * cleared, event 80 in the casino act leads to Egg Mobile 2. once that is
 * cleared, event 81 opens the train and the boat, and the section goes to the
 * Mystic Ruins */
static void script_tails_highway_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        story_play_event(77);
        break;
    case 1:
        event_flag_clear(FLAG_TAILS_TRAIN);
        event_flag_clear(FLAG_TAILS_UNK_83);
        event_flag_set(FLAG_TAILS_EC_SINK);
        story_save();
        break;
    case 2:
        story_play_event(78);
        break;
    case 3:
        story_go_to(STAGE_HIGHWAY, 0);
        story_set_time(TIME_NIGHT);
        break;
    case 4:
        if (!stage_cleared(STAGE_HIGHWAY)) {
            return;
        }
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO)) {
            return;
        }
        story_play_event(80);
        break;
    case 5:
        story_go_to(STAGE_EGGMOBILE2, 0);
        break;
    case 6:
        if (!stage_cleared(STAGE_EGGMOBILE2)) {
            return;
        }
        story_play_event(81);
        break;
    case 7:
        event_flag_set(FLAG_TAILS_TRAIN);
        event_flag_set(FLAG_TAILS_BOAT);
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
