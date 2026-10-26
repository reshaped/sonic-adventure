/* Amy's Egg Carrier story section, ZERO, the last of her story */
#include "adv01obj/script_amy_zero.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_amy_zero_init(void);
static void script_amy_zero_step(int seq);

struct story_script script_amy_zero = {script_amy_zero_init,
                                       script_amy_zero_step};

static void script_amy_zero_init(void)
{
}

static void script_amy_zero_step(int seq)
{
    switch (seq) {
    case 0:
        story_save();
        break;
    case 1:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        story_play_event(113);
        break;
    case 2:
        story_go_to(STAGE_EGGMANROBO, 0);
        break;
    case 3:
        if (!stage_cleared(STAGE_EGGMANROBO)) {
            return;
        }
        break;
    case 4:
        story_play_event(114);
        break;
    case 5:
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
