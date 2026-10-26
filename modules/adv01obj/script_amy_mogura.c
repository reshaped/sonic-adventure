/* Amy's Egg Carrier story section, MOGURA - HOTSHELTER */
#include "adv01obj/script_amy_mogura.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_amy_mogura_init(void);
static void script_amy_mogura_step(int seq);

struct story_script script_amy_mogura = {script_amy_mogura_init,
                                         script_amy_mogura_step};

static void script_amy_mogura_init(void)
{
}

static void script_amy_mogura_step(int seq)
{
    switch (seq) {
    case 0:
        event_flag_set(FLAG_AMY_EC_TRANSFORM);
        event_flag_clear(FLAG_AMY_UNK_85);
        story_save();
        break;
    case 1:
        if (!story_wait_at(STAGE_EGG_CARRIER_INSIDE, 3)) {
            return;
        }
        story_play_event(101);
        break;
    case 2:
        playable_set(CHARACTER_E102);
        break;
    case 3:
        if (!story_wait_at(STAGE_EGG_CARRIER_INSIDE, 2)) {
            return;
        }
        story_play_event(304);
        break;
    case 4:
        if (!event_flag_check(FLAG_AMY_HAMMER_WON)) {
            return;
        }
        story_play_event(305);
        break;
    case 5:
        story_save();
        break;
    case 6:
        if (!story_wait_at(STAGE_EGG_CARRIER_INSIDE, 1)) {
            return;
        }
        if (!story_going_to(STAGE_SHELTER, 0)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
