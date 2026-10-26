/* Knuckles' third story section in the past, PAST ... FIRE */
#include <ninja.h>

#include "adv03/past.h"
#include "adv03/script_knuckles_fire.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"

/* event 151 plays once player 0 comes within POINT_RADIUS of this point */
static NJS_POINT3 script_knuckles_fire_point = {-15.8F, -4.2F, 434.39F};
#define POINT_RADIUS 30.0F

void script_knuckles_fire_init(void)
{
    story_set_time(TIME_NIGHT);
}

void script_knuckles_fire_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_PAST, PAST_FIRE)) {
            return;
        }
        story_play_event(150);
        break;
    case 1:
        story_save();
        break;
    case 2:
        if (player_in_range(&script_knuckles_fire_point, POINT_RADIUS) == 1) {
            story_play_event(151);
        } else {
            return;
        }
        break;
    case 3:
        stage_set_entrance(1);
        story_go_to(STAGE_EGG_CARRIER, 5);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
