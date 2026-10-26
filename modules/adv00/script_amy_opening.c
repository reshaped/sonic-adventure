/* Amy's first Station Square story section, OPENING - MOVIE */
#include "adv00/script_amy_opening.h"
#include "adv00/station_square.h"
#include "progress/movie.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_amy_opening_init(void);
static void script_amy_opening_step(int seq);

struct story_script script_amy_opening = {script_amy_opening_init,
                                          script_amy_opening_step};

static void script_amy_opening_init(void)
{
    story_set_time(TIME_EVENING);
}

/* after event 96 in the office act, the act ends as a clear that returns to the
 * office act. MOVIE_SA4, which opens Amy's story, plays as the act leaves,
 * unless FLAG_CLEAR_AMY is set */
static void script_amy_opening_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_OFFICE)) {
            return;
        }
        story_play_event(96);
        break;
    case 1:
        if (event_flags[FLAG_CLEAR_AMY]) {
            movie_request = 0;
        } else {
            movie_request = MOVIE_SA4;
        }
        stage_fade_to(STAGE_STATION_SQUARE, STATION_SQUARE_OFFICE);
        stage_end(STAGE_END_CLEAR);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
