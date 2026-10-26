/* Knuckles' first story section in the past, PAST ... TICAL */
#include <ninja.h>

#include "adv00/station_square.h"
#include "adv03/past.h"
#include "adv03/script_knuckles_tikal.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"

/* event 133 plays once player 0 comes within POINT_RADIUS of this point */
static NJS_POINT3 script_knuckles_tikal_point = {0.0F, 10.0F, 1165.0F};
#define POINT_RADIUS 300.0F

void script_knuckles_tikal_init(void)
{
    story_set_time(TIME_DAY);
}

void script_knuckles_tikal_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_PAST, PAST_KNUCKLES)) {
            return;
        }
        story_play_event(132);
        break;
    case 1:
        story_save();
        break;
    case 2:
        if (player_in_range(&script_knuckles_tikal_point, POINT_RADIUS) == 1) {
            story_play_event(133);
        } else {
            return;
        }
        break;
    case 3:
        stage_set_entrance(1);
        story_go_to(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
