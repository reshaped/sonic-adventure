/* Amy's Mystic Ruins story section, MR - FINALEGG */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv02/script_amy_mr_final_egg.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_amy_mr_final_egg_init(void);
static void script_amy_mr_final_egg_step(int seq);

struct story_script script_amy_mr_final_egg = {script_amy_mr_final_egg_init,
                                               script_amy_mr_final_egg_step};

/* player 0 starts event 110 within 100 units of this point in the jungle */
static NJS_POINT3 script_amy_mr_final_egg_point = {68.8F, 0.0F, -1713.2F};

static void script_amy_mr_final_egg_init(void)
{
    story_set_time(TIME_NIGHT);
}

/* the truck and the raft open and the Final Egg appears, and then event 110
 * plays. the section ends once the game goes to the Final Egg */
static void script_amy_mr_final_egg_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        event_flag_set(FLAG_AMY_MR_TRUCK);
        event_flag_set(FLAG_AMY_MR_APPEAR_FINAL_EGG);
        event_flag_set(FLAG_AMY_RAFT);
        story_save();
        break;
    case 1:
        if (story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE) &&
            player_in_range(&script_amy_mr_final_egg_point, 100.0F) == 1) {
            story_play_event(110);
        } else {
            return;
        }
        break;
    case 2:
        if (!event_flag_check(FLAG_AMY_MR_ENTRANCE_FINAL_EGG)) {
            return;
        }
        story_save();
        break;
    case 3:
        if (!story_going_to(STAGE_FINAL, 0)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
