/* Big's story section in the past, PAST ... EMERALD */
#include <ninja.h>

#include "adv03/past.h"
#include "adv03/script_big_emerald.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"

/* in PAST_CHAOS, event 220 plays once player 0 comes within POINT_RADIUS of
 * this point */
static NJS_POINT3 script_big_emerald_point = {-2.74F, 7.51F, 254.65F};
#define POINT_RADIUS 40.0F

void script_big_emerald_init(void)
{
    story_set_time(TIME_DAY);
}

void script_big_emerald_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_PAST, PAST_CHAOS)) {
            return;
        }
        story_play_event(219);
        break;
    case 1:
        story_save();
        break;
    case 2:
        if (!story_wait_at(STAGE_PAST, PAST_CHAOS)) {
            return;
        }
        if (player_in_range(&script_big_emerald_point, POINT_RADIUS) == 1) {
            story_play_event(220);
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
