/* Knuckles' Mystic Ruins story section, ENDING2 */
#include "adv02/script_knuckles_ending2.h"
#include "progress/progress.h"
#include "progress/story.h"

static void script_knuckles_ending2_init(void);
static void script_knuckles_ending2_step(int seq);

struct story_script script_knuckles_ending2 = {script_knuckles_ending2_init,
                                               script_knuckles_ending2_step};

static void script_knuckles_ending2_init(void)
{
    story_set_time(TIME_DAY);
}

static void script_knuckles_ending2_step(int seq)
{
    switch (seq) {
    case 0:
    case 1:
        break;
    case 2:
        story_play_event(159);
        break;
    case 3:
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
