/* Big's Mystic Ruins story section, OPENING */
#include "adv02/mystic_ruins.h"
#include "adv02/script_big_opening.h"
#include "adv00/station_square.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_big_opening_init(void);
static void script_big_opening_step(int seq);

struct story_script script_big_opening = {script_big_opening_init,
                                          script_big_opening_step};

static void script_big_opening_init(void)
{
    story_set_time(TIME_NIGHT);
}

static void script_big_opening_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE)) {
            return;
        }
        story_play_event(208);
        break;
    case 1:
        story_go_to(STAGE_STATION_SQUARE, STATION_SQUARE_STATION);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
