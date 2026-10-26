/* Amy's Egg Carrier story section, SINK EGGCARRIER */
#include "adv00/station_square.h"
#include "adv01obj/script_amy_sink.h"
#include "progress/movie.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_amy_sink_init(void);
static void script_amy_sink_step(int seq);

struct story_script script_amy_sink = {script_amy_sink_init,
                                       script_amy_sink_step};

static void script_amy_sink_init(void)
{
}

static void script_amy_sink_step(int seq)
{
    switch (seq) {
    case 0:
        if (!stage_cleared(STAGE_SHELTER)) {
            return;
        }
        event_flag_set(FLAG_AMY_EC_TRANSFORM);
        story_save();
        break;
    case 1:
        if (!story_wait_at(STAGE_EGG_CARRIER, 5)) {
            return;
        }
        story_play_event(108);
        break;
    case 2:
        if (!story_wait_at(STAGE_EGG_CARRIER, 2)) {
            return;
        }
        story_play_event(105);
        break;
    case 3:
        story_play_event(106);
        break;
    case 4:
        if (event_flags[FLAG_CLEAR_AMY]) {
            movie_request = 0;
        } else {
            movie_request = MOVIE_SA7;
        }
        story_go_to(STAGE_STATION_SQUARE, STATION_SQUARE_STATION);
        event_flag_set(FLAG_AMY_SS_STATION_FRONT);
        event_flag_set(FLAG_AMY_BOAT);
        event_flag_set(FLAG_AMY_EC_EGG_LIFT);
        event_flag_set(FLAG_AMY_EC_MONORAIL);
        event_flag_set(FLAG_AMY_UNK_85);
        event_flag_clear(FLAG_AMY_EC_TRANSFORM);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
