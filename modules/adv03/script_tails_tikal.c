/* Tails' story section in the past, PAST ... TICAL */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv03/past.h"
#include "adv03/script_tails_tikal.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"

/* in PAST_KNUCKLES, event 69 plays once player 0 comes within POINT_RADIUS of
 * this point */
static NJS_POINT3 script_tails_tikal_point = {17.69F, 0.0F, 480.85F};
#define POINT_RADIUS 30.0F

void script_tails_tikal_init(void)
{
    story_set_time(TIME_DAY);
}

void script_tails_tikal_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_PAST, PAST_KNUCKLES)) {
            return;
        }
        story_play_event(68);
        break;
    case 1:
        story_save();
        break;
    case 2:
        if (!event_flag_check(FLAG_TAILS_RHYTHM_BROOCH)) {
            return;
        }
        break;
    case 3:
        if (!story_wait_at(STAGE_PAST, PAST_KNUCKLES)) {
            return;
        }
        if (player_in_range(&script_tails_tikal_point, POINT_RADIUS) == 1) {
            story_play_event(69);
        } else {
            return;
        }
        break;
    case 4:
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
