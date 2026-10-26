/* the last story's Mystic Ruins story section, LAST CHAOS-EMERALD */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv02/script_last_chaos_emerald.h"
#include "progress/movie.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_last_chaos_emerald_init(void);
static void script_last_chaos_emerald_step(int seq);

struct story_script script_last_chaos_emerald = {
    script_last_chaos_emerald_init, script_last_chaos_emerald_step};

/* no code reads this point */
static NJS_POINT3 script_last_chaos_emerald_point = {1226.0F, 99.0F, -904.66F};

static void script_last_chaos_emerald_init(void)
{
    story_set_time(TIME_DAY);
}

/* event 249 plays, then event 250, and MOVIE_SA1 plays on the way to
 * Chaos 7 */
static void script_last_chaos_emerald_step(int seq)
{
    switch (seq) {
    case 0:
        story_play_event(249);
        break;
    case 1:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE)) {
            return;
        }
        story_play_event(250);
        break;
    case 2:
        movie_request = MOVIE_SA1;
        story_go_to(STAGE_CHAOS7, 0);
        break;
    default:
        return;
    }
    story_next_seq();
}
