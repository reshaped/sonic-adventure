/* Knuckles' Mystic Ruins story section, ENDING1 */
#include "adv02/mystic_ruins.h"
#include "adv02/script_knuckles_ending1.h"
#include "progress/movie.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_knuckles_ending1_init(void);
static void script_knuckles_ending1_step(int seq);

struct story_script script_knuckles_ending1 = {script_knuckles_ending1_init,
                                               script_knuckles_ending1_step};

static void script_knuckles_ending1_init(void)
{
    story_set_time(TIME_DAY);
}

/* event 157 plays. the act then ends as cleared, fading back into the same
 * act, and MOVIE_SA3, which ends Knuckles' story, plays unless the story is
 * cleared */
static void script_knuckles_ending1_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_ISLAND)) {
            return;
        }
        story_play_event(157);
        break;
    case 1:
        if (event_flags[FLAG_CLEAR_KNUCKLES]) {
            movie_request = 0;
        } else {
            movie_request = MOVIE_SA3;
        }
        stage_fade_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_ISLAND);
        stage_end(STAGE_END_CLEAR);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
