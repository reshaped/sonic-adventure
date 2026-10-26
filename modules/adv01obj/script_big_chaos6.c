/* Big's Egg Carrier story section, CHAOS6 */
#include <ninja.h>

#include "adv01obj/script_big_chaos6.h"
#include "adv02/mystic_ruins.h"
#include "player/player.h"
#include "progress/movie.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

/* player 0 or 1 within EVENT_224_RADIUS of script_big_chaos6_point starts
 * event 224 */
#define EVENT_224_RADIUS 40.0F
/* player 0 within CHAOS6_RADIUS across script_big_chaos6_cylinder, and
 * within CHAOS6_HEIGHT above or below it, goes on to Chaos 6 unless Big has
 * cleared it */
#define CHAOS6_RADIUS 100.0F
#define CHAOS6_HEIGHT 10.0F

static void script_big_chaos6_init(void);
static void script_big_chaos6_step(int seq);

struct story_script script_big_chaos6 = {script_big_chaos6_init,
                                         script_big_chaos6_step};

/* where event 224 and Chaos 6 start, in act 0 */
static NJS_POINT3 script_big_chaos6_point = {0.0F, 700.0F, -1100.0F};
static NJS_POINT3 script_big_chaos6_cylinder = {0.0F, 750.0F, -380.0F};

static void script_big_chaos6_init(void)
{
}

static void script_big_chaos6_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_EGG_CARRIER_INSIDE, 1)) {
            return;
        }
        story_play_event(221);
        break;
    case 1:
        story_play_event(336);
        break;
    case 2:
        event_flag_set(FLAG_BIG_EC_MONORAIL);
        event_flag_clear(FLAG_BIG_EC_TRANSFORM);
        story_save();
        break;
    case 3:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        /* @hack redundant: the else after the break, which keeps the calls
         * below in the image's registers, as stage_cleared's address in R3
         * at 0x0c9016c2; without it, three of them take R2 and 6 of 153
         * words differ */
        if (player_in_cylinder(&script_big_chaos6_cylinder, CHAOS6_RADIUS,
                               CHAOS6_HEIGHT) == 1) {
            break;
        } else {
            return;
        }
    case 4:
        if (!stage_cleared(STAGE_CHAOS6)) {
            story_go_to(STAGE_CHAOS6, 0);
        }
        break;
    case 5:
        if (!stage_cleared(STAGE_CHAOS6)) {
            return;
        }
        break;
    case 6:
        break;
    case 7:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        if (player_in_range(&script_big_chaos6_point, EVENT_224_RADIUS)) {
            story_play_event(224);
        } else {
            return;
        }
        break;
    case 8:
        if (event_flags[FLAG_CLEAR_BIG]) {
            movie_request = 0;
        } else {
            movie_request = MOVIE_SA7;
        }
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE);
        event_flag_set(FLAG_BIG_UNK_87);
        event_flag_set(FLAG_BIG_EC_EGG_LIFT);
        event_flag_set(FLAG_BIG_RAFT);
        event_flag_set(FLAG_BIG_BOAT);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
