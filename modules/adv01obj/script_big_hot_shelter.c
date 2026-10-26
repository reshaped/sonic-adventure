/* Big's Egg Carrier story section, HOT SHELTER */
#include "adv01obj/script_big_hot_shelter.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_big_hot_shelter_init(void);
static void script_big_hot_shelter_step(int seq);

struct story_script script_big_hot_shelter = {script_big_hot_shelter_init,
                                              script_big_hot_shelter_step};

static void script_big_hot_shelter_init(void)
{
}

static void script_big_hot_shelter_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_EGG_CARRIER_INSIDE, 1)) {
            return;
        }
        event_flag_clear(FLAG_BIG_UNK_87);
        story_save();
        break;
    case 1:
        story_play_event(216);
        break;
    case 2:
        if (!story_going_to(STAGE_SHELTER, 0)) {
            return;
        }
        break;
    case 3:
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
