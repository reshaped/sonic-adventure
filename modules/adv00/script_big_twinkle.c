/* Big's Station Square story section, TWINKLE */
#include <ninja.h>

#include "adv00/script_big_twinkle.h"
#include "adv00/station_square.h"
#include "adv02/mystic_ruins.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_big_twinkle_init(void);
static void script_big_twinkle_step(int seq);

struct story_script script_big_twinkle = {script_big_twinkle_init,
                                          script_big_twinkle_step};

/* a point nothing reads */
static NJS_POINT3 script_big_twinkle_point = {330.32F, 1.51F, 1000.29F};

static void script_big_twinkle_init(void)
{
    story_set_time(TIME_DAY);
}

/* event 209 plays in the station act and event 210 in the office act. the
 * Twinkle Park elevator opens once the game has passed through the sewer act
 * back to the station act. once Twinkle Park is cleared, the station's front
 * and the train open, and event 376 plays in the station act. the section ends
 * as the game goes to the Mystic Ruins */
static void script_big_twinkle_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        story_play_event(209);
        break;
    case 1:
        event_flag_set(FLAG_BIG_UNK_87);
        story_save();
        break;
    case 2:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_OFFICE)) {
            return;
        }
        story_play_event(210);
        break;
    case 3:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_GESUI)) {
            return;
        }
        break;
    case 4:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        story_save();
        break;
    case 5:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        event_flag_set(FLAG_BIG_SS_TPARK_ELEVATOR);
        break;
    case 6:
        if (!stage_cleared(STAGE_TWINKLE)) {
            return;
        }
        event_flag_set(FLAG_BIG_SS_ENTRANCE_CIRCUIT);
        event_flag_set(FLAG_BIG_SS_STATION_FRONT);
        event_flag_set(FLAG_BIG_TRAIN);
        break;
    case 7:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        event_flag_set(FLAG_BIG_SS_ICE_STONE);
        story_play_event(376);
        break;
    case 8:
        story_save();
        break;
    case 9:
        if (!story_going_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
