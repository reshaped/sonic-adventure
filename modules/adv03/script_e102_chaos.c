/* E-102's story section in the past, PAST ... CHAOS */
#include <ninja.h>

#include "adv03/past.h"
#include "adv03/script_e102_chaos.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"

/* in PAST_CHAOS, event 186 plays once player 0's z falls below EVENT_Z */
#define EVENT_Z 800.0F

void script_e102_chaos_init(void)
{
    story_set_time(TIME_DAY);
}

void script_e102_chaos_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_PAST, PAST_CHAOS)) {
            return;
        }
        story_play_event(185);
        break;
    case 1:
        story_save();
        break;
    case 2:
        if (!story_wait_at(STAGE_PAST, PAST_CHAOS)) {
            return;
        }
        if (player_work[0]->pos.z < EVENT_Z) {
            story_play_event(186);
        } else {
            return;
        }
        break;
    case 3:
        story_go_to(STAGE_EGG_CARRIER_INSIDE, 1);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
