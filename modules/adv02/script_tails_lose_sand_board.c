/* Tails' Mystic Ruins story section, LOSE - SANDBOARD */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv02/script_tails_lose_sand_board.h"
#include "event/play.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_tails_lose_sand_board_init(void);
static void script_tails_lose_sand_board_step(int seq);

struct story_script script_tails_lose_sand_board = {
    script_tails_lose_sand_board_init, script_tails_lose_sand_board_step};

/* player 0 starts event 66 within 240 units of this point, by the way to Sand
 * Hill */
static NJS_POINT3 script_tails_lose_sand_board_point = {-1193.45F, 49.84F,
                                                        -194.97F};

static void script_tails_lose_sand_board_init(void)
{
    if (event_was_played(62)) {
        story_set_time(TIME_EVENING);
    } else {
        story_set_time(TIME_DAY);
    }
}

/* events 62, 64 and 66 play in turn. the section ends once the game goes to
 * Sand Hill */
static void script_tails_lose_sand_board_step(int seq)
{
    switch (seq) {
    case 0:
        event_flag_set(FLAG_TAILS_MR_TRUCK);
        event_flag_clear(FLAG_TAILS_TRAIN);
        event_flag_clear(FLAG_TAILS_UNK_83);
        break;
    case 1:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE)) {
            return;
        }
        story_play_event(62);
        break;
    case 2:
        story_set_time(TIME_EVENING);
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
        break;
    case 3:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        story_play_event(64);
        break;
    case 4:
        if (story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE) &&
            player_in_range(&script_tails_lose_sand_board_point, 240.0F) == 1) {
            story_play_event(66);
        } else {
            return;
        }
        break;
    case 5:
        if (!story_going_to(STAGE_SUNABOARD, 0)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
