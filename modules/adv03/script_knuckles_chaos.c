/* Knuckles' second story section in the past, PAST ... CHAOS */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv03/past.h"
#include "adv03/script_knuckles_chaos.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"

/* event 143 plays once player 0 comes within POINT_RADIUS of this point, at a
 * height of EVENT_FLOOR or more */
static NJS_POINT3 script_knuckles_chaos_point = {0.0F, 90.0F, 48.0F};
#define POINT_RADIUS 40.0F
#define EVENT_FLOOR 89.0F

void script_knuckles_chaos_init(void)
{
    story_set_time(TIME_DAY);
}

void script_knuckles_chaos_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_PAST, PAST_CHAOS)) {
            return;
        }
        story_play_event(142);
        break;
    case 1:
        story_save();
        break;
    case 2:
        if (player_in_range(&script_knuckles_chaos_point, POINT_RADIUS) == 1) {
            if (player_work[0]->pos.y < EVENT_FLOOR) {
                return;
            }
            story_play_event(143);
        } else {
            return;
        }
        break;
    case 3:
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_ISLAND);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
