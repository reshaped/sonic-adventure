/* Tails' first Station Square story section, OPENING - BEACH */
#include "adv00/script_tails_opening.h"
#include "adv00/station_square.h"
#include "adv02/mystic_ruins.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_tails_opening_init(void);
static void script_tails_opening_step(int seq);

struct story_script script_tails_opening = {script_tails_opening_init,
                                            script_tails_opening_step};

static void script_tails_opening_init(void)
{
    story_set_time(TIME_DAY);
}

/* after event 48 in the station act, the section goes to Emerald Coast act 1.
 * the hotel's front, the station's front and the train open then. once Emerald
 * Coast is cleared, event 50 plays in the hotel act, and the section ends
 * whatever story_going_to returns */
static void script_tails_opening_step(int seq)
{
    switch (seq) {
    case 0:
        if (stage_get_stage_act() !=
            STAGE_ACT(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        story_play_event(48);
        break;
    case 1:
        story_go_to(STAGE_BEACH, 1);
        event_flag_set(FLAG_TAILS_SS_HOTEL_FRONT);
        event_flag_set(FLAG_TAILS_SS_STATION_FRONT);
        event_flag_set(FLAG_TAILS_TRAIN);
        event_flag_set(FLAG_TAILS_UNK_83);
        break;
    case 2:
        if (!stage_cleared(STAGE_BEACH)) {
            return;
        }
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_HOTEL)) {
            return;
        }
        story_play_event(50);
        break;
    case 3:
        story_going_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
