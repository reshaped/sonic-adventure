/* Amy's story section in the past, PAST ... TICAL */
#include <ninja.h>

#include "adv03/past.h"
#include "adv03/script_amy_tikal.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

/* in PAST_CHAOS, event 104 plays once player 0's z falls below EVENT_Z */
#define EVENT_Z 500.0F

static void script_amy_tikal_init(void);
static void script_amy_tikal_step(int seq);

struct story_script script_amy_tikal = {script_amy_tikal_init,
                                        script_amy_tikal_step};

/* in PAST_KNUCKLES, player 0 goes on to PAST_CHAOS once it comes within
 * POINT_RADIUS of this point */
static NJS_POINT3 script_amy_tikal_point = {0.0F, 0.0F, 1450.0F};
#define POINT_RADIUS 30.0F

static void script_amy_tikal_init(void)
{
    story_set_time(TIME_DAY);
}

static void script_amy_tikal_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_PAST, PAST_KNUCKLES)) {
            return;
        }
        story_play_event(103);
        break;
    case 1:
        story_save();
        break;
    case 2:
        if (player_in_range(&script_amy_tikal_point, POINT_RADIUS) == 1) {
            story_go_to(STAGE_PAST, PAST_CHAOS);
        } else {
            return;
        }
        break;
    case 3:
        if (!story_wait_at(STAGE_PAST, PAST_CHAOS)) {
            return;
        }
        if (player_work[0]->pos.z < EVENT_Z) {
            story_play_event(104);
        } else {
            return;
        }
        break;
    case 4:
        story_go_to(STAGE_EGG_CARRIER, 5);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
