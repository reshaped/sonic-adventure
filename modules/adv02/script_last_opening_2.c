/* the last story's Mystic Ruins story section, OPENING 2 */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv02/script_last_opening_2.h"
#include "adv03/past.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_last_opening_2_init(void);
static void script_last_opening_2_step(int seq);

struct story_script script_last_opening_2 = {script_last_opening_2_init,
                                             script_last_opening_2_step};

/* player 0 starts event 245 within 100 units of this point in the island act */
static NJS_POINT3 script_last_opening_2_point = {-35.0F, -264.0F, 2634.0F};

static void script_last_opening_2_init(void)
{
    story_set_time(TIME_DAY);
}

/* events 242, 243, 244 and 245 play in turn. the story then goes to the
 * Past */
static void script_last_opening_2_step(int seq)
{
    switch (seq) {
    case 0:
    case 1:
        break;
    case 2:
        story_play_event(242);
        break;
    case 3:
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_ISLAND);
        break;
    case 4:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_ISLAND)) {
            return;
        }
        story_play_event(243);
        break;
    case 5:
        story_set_time(TIME_DAY);
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
        break;
    case 6:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        story_play_event(244);
        break;
    case 7:
        if (story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_ISLAND) &&
            player_in_range(&script_last_opening_2_point, 100.0F) == 1) {
            story_play_event(245);
        } else {
            return;
        }
        break;
    case 8:
        story_go_to(STAGE_PAST, PAST_FIRE);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
