/* Knuckles' Egg Carrier story section, EC - SKYDECK */
#include <ninja.h>

#include "adv01obj/script_knuckles_skydeck.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

/* event 288 plays once player 0's z is at most EVENT_288_Z */
#define EVENT_288_Z 600.0F
/* player 0 within EVENT_290_RADIUS across script_knuckles_skydeck_point,
 * and within EVENT_290_HEIGHT above or below it, starts event 290 */
#define EVENT_290_RADIUS 100.0F
#define EVENT_290_HEIGHT 20.0F

static void script_knuckles_skydeck_init(void);
static void script_knuckles_skydeck_step(int seq);

/* where event 290 starts, in act 5 */
static NJS_POINT3 script_knuckles_skydeck_point = {-1.0F, -56.0F, 0.0F};

struct story_script script_knuckles_skydeck = {script_knuckles_skydeck_init,
                                               script_knuckles_skydeck_step};

static void script_knuckles_skydeck_init(void)
{
}

static void script_knuckles_skydeck_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        story_play_event(148);
        break;
    case 1:
        event_flag_clear(FLAG_KNUCKLES_UNK_84);
        story_save();
        break;
    case 2:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        if (player_work[0]->pos.z <= EVENT_288_Z) {
            story_play_event(288);
        } else {
            return;
        }
        break;
    case 3:
        stage_set_entrance(5);
        story_go_to(STAGE_EGG_CARRIER, 6);
        break;
    case 4:
        if (!story_wait_at(STAGE_EGG_CARRIER, 2)) {
            return;
        }
        story_play_event(289);
        event_flag_set(269);
        break;
    case 5:
        story_save();
        break;
    case 6:
        if (!story_wait_at(STAGE_EGG_CARRIER, 5)) {
            return;
        }
        if (player_in_cylinder(&script_knuckles_skydeck_point, EVENT_290_RADIUS,
                               EVENT_290_HEIGHT) == 1) {
            story_play_event(290);
        } else {
            return;
        }
        break;
    case 7:
        if (!story_going_to(STAGE_SKY_DECK, 2)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
