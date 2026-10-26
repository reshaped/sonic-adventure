/* Tails' Egg Carrier story section, EC - SKYDECK */
#include <ninja.h>

#include "adv01obj/script_tails_skydeck.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

/* event 273 plays once player 0's z is at least EVENT_273_Z */
#define EVENT_273_Z (-500.0F)
/* player 0 within EVENT_275_RADIUS of script_tails_skydeck_point starts
 * event 275 */
#define EVENT_275_RADIUS 60.0F

static void script_tails_skydeck_init(void);
static void script_tails_skydeck_step(int seq);

struct story_script script_tails_skydeck = {script_tails_skydeck_init,
                                            script_tails_skydeck_step};

/* where event 275 starts */
static NJS_POINT3 script_tails_skydeck_point = {0.0F, 748.0F, 402.0F};

static void script_tails_skydeck_init(void)
{
}

static void script_tails_skydeck_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        event_flag_set(209);
        story_play_event(84);
        break;
    case 1:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        event_flag_clear(209);
        event_flag_clear(FLAG_TAILS_UNK_83);
        story_play_event(272);
        break;
    case 2:
        story_save();
        break;
    case 3:
        if (player_work[0]->pos.z < EVENT_273_Z) {
            return;
        }
        story_play_event(273);
        break;
    case 4:
        stage_set_entrance(0);
        story_go_to(STAGE_EGG_CARRIER, 6);
        break;
    case 5:
        if (!story_wait_at(STAGE_EGG_CARRIER, 1)) {
            return;
        }
        story_play_event(274);
        break;
    case 6:
        story_save();
        break;
    case 7:
        if (player_in_range(&script_tails_skydeck_point, EVENT_275_RADIUS) !=
            1) {
            return;
        }
        story_play_event(275);
        break;
    case 8:
        if (!story_going_to(STAGE_SKY_DECK, 0)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
