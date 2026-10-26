/* Tails' Station Square story section, WAKE */
#include "adv00/script_tails_wake.h"
#include "adv02/mystic_ruins.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_tails_wake_init(void);
static void script_tails_wake_step(int seq);

struct story_script script_tails_wake = {script_tails_wake_init,
                                         script_tails_wake_step};

static void script_tails_wake_init(void)
{
    story_set_time(TIME_DAY);
}

/* event 86 plays, and the section ends whatever story_going_to returns */
static void script_tails_wake_step(int seq)
{
    switch (seq) {
    case 0:
        story_play_event(86);
        break;
    case 1:
        story_save();
        break;
    case 2:
        story_going_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
