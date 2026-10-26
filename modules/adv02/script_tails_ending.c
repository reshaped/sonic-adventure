/* Tails' Mystic Ruins story section, ENDING */
#include "adv02/script_tails_ending.h"
#include "progress/progress.h"
#include "progress/story.h"

static void script_tails_ending_init(void);
static void script_tails_ending_step(int seq);

struct story_script script_tails_ending = {script_tails_ending_init,
                                           script_tails_ending_step};

static void script_tails_ending_init(void)
{
    story_set_time(TIME_DAY);
}

static void script_tails_ending_step(int seq)
{
    switch (seq) {
    case 0:
        story_play_event(82);
        break;
    case 1:
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
