/* the last story's Mystic Ruins story section, OPENING 1 */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv02/script_last_opening_1.h"
#include "progress/movie.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_last_opening_1_init(void);
static void script_last_opening_1_step(int seq);

struct story_script script_last_opening_1 = {script_last_opening_1_init,
                                             script_last_opening_1_step};

/* a copy of OPENING 2's point for event 245. no code reads this copy */
static NJS_POINT3 script_last_opening_1_point = {-35.0F, -264.0F, 2634.0F};

static void script_last_opening_1_init(void)
{
    story_set_time(TIME_NIGHT);
}

/* event 240 plays. the act then ends as cleared, and MOVIE_SA2 plays */
static void script_last_opening_1_step(int seq)
{
    switch (seq) {
    case 0:
        story_play_event(240);
        break;
    case 1:
        movie_request = MOVIE_SA2;
        stage_fade_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE);
        stage_end(STAGE_END_CLEAR);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
