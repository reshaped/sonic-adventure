/* Sonic's Mystic Ruins story section, ENDING */
#include "adv02/script_sonic_ending.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_sonic_ending_init(void);
static void script_sonic_ending_step(int seq);

struct story_script script_sonic_ending = {script_sonic_ending_init,
                                           script_sonic_ending_step};

static void script_sonic_ending_init(void)
{
    if (stage_cleared(STAGE_FINAL)) {
        story_set_time(TIME_DAY);
    } else {
        story_set_time(TIME_EVENING);
    }
}

static void script_sonic_ending_step(int seq)
{
    switch (seq) {
    case 0:
        if (!stage_cleared(STAGE_FINAL)) {
            return;
        }
        story_play_event(38);
        break;
    case 1:
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
