/* Tails' Egg Carrier story section, EC HALL - ESCAPE */
#include "adv00/station_square.h"
#include "adv01obj/script_tails_escape.h"
#include "player/player.h"
#include "progress/movie.h"
#include "progress/playable.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_tails_escape_init(void);
static void script_tails_escape_step(int seq);

struct story_script script_tails_escape = {script_tails_escape_init,
                                           script_tails_escape_step};

static void script_tails_escape_init(void)
{
}

static void script_tails_escape_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_EGG_CARRIER_INSIDE, 1)) {
            return;
        }
        story_play_event(276);
        event_flag_set(FLAG_TAILS_EC_EGG_LIFT);
        event_flag_set(FLAG_TAILS_EC_TRANSFORM);
        break;
    case 1:
        if (!story_wait_at(STAGE_EGG_CARRIER, 2)) {
            return;
        }
        story_save();
        break;
    case 2:
        story_play_event(75);
        break;
    case 3:
        battle_start(TAILS_VS_E102);
        break;
    case 4:
        if (!event_flag_check(FLAG_TAILS_MBOSS_E102)) {
            return;
        }
        story_save();
        break;
    case 5:
        story_play_event(76);
        break;
    case 6:
        playable_unlock(CHARACTER_AMY);
        break;
    case 7:
        playable_unlock(CHARACTER_E102);
        break;
    case 8:
        if (event_flags[FLAG_CLEAR_TAILS]) {
            movie_request = 0;
        } else {
            movie_request = MOVIE_SA7;
        }
        event_flag_set(FLAG_TAILS_EC_EGG_LIFT);
        event_flag_set(FLAG_TAILS_EC_MONORAIL);
        event_flag_set(FLAG_TAILS_BOAT);
        event_flag_set(FLAG_TAILS_RAFT);
        event_flag_set(FLAG_TAILS_UNK_83);
        story_go_to(STAGE_STATION_SQUARE, STATION_SQUARE_STATION);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
