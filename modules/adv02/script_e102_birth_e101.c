/* E-102's Mystic Ruins story section, BIRTH - E101 */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv02/script_e102_birth_e101.h"
#include "player/player.h"
#include "progress/movie.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_e102_birth_e101_init(void);
static void script_e102_birth_e101_step(int seq);

/* a player starts event 177 within 50 units of the first point, and event 178
 * within 20 units of the second, in the base act */
static NJS_POINT3 script_e102_birth_e101_point_177 = {34.34F, 108.73F,
                                                      -140.86F};
static NJS_POINT3 script_e102_birth_e101_point_178 = {-140.36F, 0.27F, 5.44F};

struct story_script script_e102_birth_e101 = {script_e102_birth_e101_init,
                                              script_e102_birth_e101_step};

static void script_e102_birth_e101_init(void)
{
    story_set_time(TIME_DAY);
}

/* events 176 and 177 play before E-102's act of the Final Egg. once that act
 * is cleared, events 178 and 180 play, then the fight with E-101 and event
 * 181. the story then goes inside the Egg Carrier, and MOVIE_SA8 plays on the
 * way unless E-102's story is cleared */
static void script_e102_birth_e101_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_BASE)) {
            return;
        }
        story_play_event(176);
        break;
    case 1:
        story_save();
        break;
    case 2:
        if (!player_in_range(&script_e102_birth_e101_point_177, 50.0F)) {
            return;
        }
        story_play_event(177);
        break;
    case 3:
        story_go_to(STAGE_FINAL, 2);
        break;
    case 4:
        if (!stage_cleared(STAGE_FINAL)) {
            return;
        }
        break;
    case 5:
        if (!player_in_range(&script_e102_birth_e101_point_178, 20.0F)) {
            return;
        }
        story_play_event(178);
        break;
    case 6:
        story_save();
        break;
    case 7:
        story_play_event(180);
        break;
    case 8:
        story_go_to(STAGE_E101, 0);
        break;
    case 9:
        if (!stage_cleared(STAGE_E101)) {
            return;
        }
        story_play_event(181);
        break;
    case 10:
        if (event_flags[FLAG_CLEAR_E102]) {
            movie_request = 0;
        } else {
            movie_request = MOVIE_SA8;
        }
        stage_set_entrance(0);
        story_go_to(STAGE_EGG_CARRIER_INSIDE, 1);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
