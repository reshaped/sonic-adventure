/* Sonic's Egg Carrier story section, EC - SKYDECK */
#include <ninja.h>

#include "adv01obj/script_sonic_skydeck.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

/* event 257 plays once player 0's z is at least EVENT_257_Z */
#define EVENT_257_Z (-500.0F)
/* player 0 within EVENT_259_RADIUS of script_sonic_skydeck_point starts
 * event 259 */
#define EVENT_259_RADIUS 60.0F

static void script_sonic_skydeck_init(void);
static void script_sonic_skydeck_step(int seq);

struct story_script script_sonic_skydeck = {script_sonic_skydeck_init,
                                            script_sonic_skydeck_step};

/* where event 259 starts */
static NJS_POINT3 script_sonic_skydeck_point = {0.0F, 748.0F, 402.0F};

static void script_sonic_skydeck_init(void)
{
}

static void script_sonic_skydeck_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        event_flag_set(146);
        story_play_event(41);
        break;
    case 1:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        event_flag_clear(146);
        story_play_event(256);
        break;
    case 2:
        event_flag_clear(FLAG_SONIC_UNK_82);
        story_save();
        break;
    case 3:
        if (player_work[0]->pos.z < EVENT_257_Z) {
            return;
        }
        story_play_event(257);
        break;
    case 4:
        stage_set_entrance(0);
        story_go_to(STAGE_EGG_CARRIER, 6);
        break;
    case 5:
        if (!story_wait_at(STAGE_EGG_CARRIER, 1)) {
            return;
        }
        story_play_event(258);
        break;
    case 6:
        story_save();
        break;
    case 7:
        if (player_in_range(&script_sonic_skydeck_point, EVENT_259_RADIUS) !=
            1) {
            return;
        }
        story_play_event(259);
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
