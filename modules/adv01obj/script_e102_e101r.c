/* E-102's Egg Carrier story section, E-101 R, the last of his story */
#include <ninja.h>

#include "adv01obj/script_e102_e101r.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

/* the story goes on to E-101 R once player 0's z is under E101_R_Z */
#define E101_R_Z 210.0F

static void script_e102_e101r_init(void);
static void script_e102_e101r_step(int seq);

struct story_script script_e102_e101r = {script_e102_e101r_init,
                                         script_e102_e101r_step};

static void script_e102_e101r_init(void)
{
    event_flag_clear(FLAG_E102_EC_TRANSFORM);
}

static void script_e102_e101r_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        story_play_event(321);
        break;
    case 1:
        event_flag_set(FLAG_E102_EC_EGG_LIFT);
        event_flag_set(FLAG_E102_EC_HOT_SHELTER);
        story_save();
        break;
    case 2:
        if (!stage_cleared(STAGE_SHELTER)) {
            return;
        }
        break;
    case 3:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        story_play_event(322);
        break;
    case 4:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        /* @hack redundant: the else after the break, which keeps the calls
         * below in the image's registers, as story_go_to's address in R2 at
         * 0x0c9014c0; without it, R2 and R3 trade in the four calls from
         * there and 8 of 111 words differ */
        if (player_work[0]->pos.z < E101_R_Z) {
            break;
        } else {
            return;
        }
    case 5:
        story_go_to(STAGE_E101_R, 0);
        break;
    case 6:
        if (!stage_cleared(STAGE_E101_R)) {
            return;
        }
        story_play_event(199);
        break;
    case 7:
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
