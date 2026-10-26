/* Big's Mystic Ruins story section, ENDING */
#include "adv02/mystic_ruins.h"
#include "adv02/script_big_ending.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_big_ending_init(void);
static void script_big_ending_step(int seq);

struct story_script script_big_ending = {script_big_ending_init,
                                         script_big_ending_step};

static void script_big_ending_init(void)
{
    story_set_time(TIME_DAY);
}

/* the section sinks the Egg Carrier and plays event 226 */
static void script_big_ending_step(int seq)
{
    switch (seq) {
    case 0:
        event_flag_set(FLAG_BIG_EC_SINK);
        break;
    case 1:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE)) {
            return;
        }
        story_play_event(226);
        break;
    case 2:
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
