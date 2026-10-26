/* Knuckles' Egg Carrier story section, CHAOS6 */
#include <ninja.h>

#include "adv01obj/script_knuckles_chaos6.h"
#include "adv02/mystic_ruins.h"
#include "player/player.h"
#include "progress/movie.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

/* the story goes on to Chaos 6 once player 0's z is under CHAOS6_Z */
#define CHAOS6_Z (-220.0F)

static void script_knuckles_chaos6_init(void);
static void script_knuckles_chaos6_step(int seq);

struct story_script script_knuckles_chaos6 = {script_knuckles_chaos6_init,
                                              script_knuckles_chaos6_step};

static void script_knuckles_chaos6_init(void)
{
}

static void script_knuckles_chaos6_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_EGG_CARRIER, 5)) {
            return;
        }
        story_play_event(152);
        break;
    case 1:
        event_flag_clear(FLAG_KNUCKLES_EC_TRANSFORM);
        event_flags[36] = 1;
        story_save();
        break;
    case 2:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        story_play_event(154);
        break;
    case 3:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        /* @hack redundant: the else after the break, which keeps the code below
         * in the image's registers, as story_go_to's address in R2 at
         * 0x0c900c24; without it, R2 and R3 trade from there to 0x0c900c82
         * and 9 of 125 words differ */
        if (player_work[0]->pos.z < CHAOS6_Z) {
            break;
        } else {
            return;
        }
    case 4:
        story_go_to(STAGE_CHAOS6, 1);
        break;
    case 5:
        if (!stage_cleared(STAGE_CHAOS6)) {
            return;
        }
        story_play_event(155);
        break;
    case 6:
        if (event_flags[FLAG_CLEAR_KNUCKLES]) {
            movie_request = 0;
        } else {
            movie_request = MOVIE_SA7;
        }
        event_flag_set(FLAG_KNUCKLES_UNK_84);
        event_flag_set(FLAG_KNUCKLES_EC_MONORAIL);
        event_flag_set(FLAG_KNUCKLES_EC_EGG_LIFT);
        event_flag_clear(FLAG_KNUCKLES_EC_TRANSFORM);
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_ISLAND);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
