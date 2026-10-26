/* Knuckles' Mystic Ruins story section, OPENING */
#include "adv02/mystic_ruins.h"
#include "adv02/script_knuckles_opening.h"
#include "adv00/station_square.h"
#include "event/play.h"
#include "progress/movie.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_knuckles_opening_init(void);
static void script_knuckles_opening_step(int seq);

struct story_script script_knuckles_opening = {script_knuckles_opening_init,
                                               script_knuckles_opening_step};

static void script_knuckles_opening_init(void)
{
    story_set_time(TIME_NIGHT);
}

/* the step starts event 128 itself, not through story_play_event, so it plays
 * whether or not it has played before. MOVIE_SA2 plays on the way to
 * Station Square unless Knuckles' story is cleared */
static void script_knuckles_opening_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_ISLAND)) {
            return;
        }
        event_load(128);
        event_start();
        break;
    case 1:
        player_record->unk_01 = 2;
        if (event_flags[FLAG_CLEAR_KNUCKLES]) {
            movie_request = 0;
        } else {
            movie_request = MOVIE_SA2;
        }
        stage_set_entrance(0);
        story_go_to(STAGE_STATION_SQUARE, STATION_SQUARE_STATION);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
