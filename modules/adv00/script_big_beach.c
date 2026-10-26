/* Big's Station Square story section, BEACH */
#include "adv00/script_big_beach.h"
#include "adv00/station_square.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_big_beach_init(void);
static void script_big_beach_step(int seq);

struct story_script script_big_beach = {script_big_beach_init,
                                        script_big_beach_step};

static void script_big_beach_init(void)
{
    story_set_time(TIME_DAY);
}

/* the hotel's front and pool open, and event 227 plays in the station act. the
 * section ends as the game goes to Emerald Coast act 2 */
static void script_big_beach_step(int seq)
{
    switch (seq) {
    case 0:
        event_flag_set(FLAG_BIG_SS_HOTEL_FRONT);
        event_flag_set(FLAG_BIG_SS_HOTEL_POOL);
        story_save();
        break;
    case 1:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        story_play_event(227);
        break;
    case 2:
        if (!story_going_to(STAGE_BEACH, 2)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
