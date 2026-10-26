/* Tails' Mystic Ruins story section, SHOOTING2 */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv02/script_tails_shooting2.h"
#include "player/player.h"
#include "progress/playable.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_tails_shooting2_init(void);
static void script_tails_shooting2_step(int seq);

struct story_script script_tails_shooting2 = {script_tails_shooting2_init,
                                              script_tails_shooting2_step};

/* player 0 starts event 71 within 80 units of this point, by the workshop */
static NJS_POINT3 script_tails_shooting2_point = {1432.0F, 201.0F, 809.0F};

static void script_tails_shooting2_init(void)
{
    story_set_time(TIME_NIGHT);
}

/* event 70 plays, and then event 71 by the workshop. the train is closed
 * between them */
static void script_tails_shooting2_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        story_play_event(70);
        break;
    case 1:
        playable_unlock(CHARACTER_BIG);
        event_flag_set(FLAG_TAILS_MR_TRUCK);
        event_flag_clear(FLAG_TAILS_TRAIN);
        event_flag_clear(FLAG_TAILS_UNK_83);
        break;
    case 2:
        if (story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION) &&
            player_in_range(&script_tails_shooting2_point, 80.0F) == 1) {
            story_play_event(71);
        } else {
            return;
        }
        break;
    case 3:
        event_flag_set(FLAG_TAILS_TRAIN);
        event_flag_set(FLAG_TAILS_UNK_83);
        story_go_to(STAGE_MOUNTAIN, 1);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
