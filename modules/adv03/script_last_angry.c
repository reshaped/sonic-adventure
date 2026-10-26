/* the last story's section in the past, CHAOS GET ANGRY */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv03/script_last_angry.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

static void script_last_angry_init(void);
static void script_last_angry_step(int seq);

struct story_script script_last_angry = {script_last_angry_init,
                                         script_last_angry_step};

/* event 251 plays once player 0 comes within POINT_RADIUS of this point */
static NJS_POINT3 script_last_angry_point = {-15.8F, -4.2F, 434.39F};
#define POINT_RADIUS 50.0F

/* event 247 plays once player 0's z falls below EVENT_247_Z. event 248 plays
 * once its y is EVENT_248_Y or more and its z EVENT_248_Z or less */
#define EVENT_247_Z 500.0F
#define EVENT_248_Y 90.0F
#define EVENT_248_Z 100.0F

static void script_last_angry_init(void)
{
    story_set_time(TIME_NIGHT);
}

static void script_last_angry_step(int seq)
{
    switch (seq) {
    case 0:
        story_play_event(246);
        break;
    case 1:
        if (player_work[0]->pos.z < EVENT_247_Z) {
            story_play_event(247);
        } else {
            return;
        }
        break;
    case 2:
        story_play_event(352);
        break;
    case 3:
        if (player_in_range(&script_last_angry_point, POINT_RADIUS) == 1) {
            story_play_event(251);
        } else {
            return;
        }
        break;
    case 4:
        if (player_work[0]->pos.y >= EVENT_248_Y &&
            player_work[0]->pos.z <= EVENT_248_Z) {
            story_play_event(248);
        } else {
            return;
        }
        break;
    case 5:
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_ISLAND);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
