/* Big's Mystic Ruins story section, ICECAP */
#include "adv02/mystic_ruins.h"
#include "adv02/script_big_icecap.h"
#include "adv00/station_square.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_big_icecap_init(void);
static void script_big_icecap_step(int seq);

struct story_script script_big_icecap = {script_big_icecap_init,
                                         script_big_icecap_step};

static void script_big_icecap_init(void)
{
    if (stage_cleared(STAGE_ICECAP)) {
        story_set_time(TIME_NIGHT);
    } else {
        story_set_time(TIME_DAY);
    }
}

/* the west rock and the truck open. once Ice Cap is cleared, the train closes
 * for event 211 and opens again after it. the last step clears the record's
 * wait through story_going_to, whose result goes unread */
static void script_big_icecap_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        event_flag_set(FLAG_BIG_MR_WEST_ROCK);
        event_flag_set(FLAG_BIG_MR_TRUCK);
        story_save();
        break;
    case 1:
        break;
    case 2:
        if (!stage_cleared(STAGE_ICECAP)) {
            return;
        }
        event_flag_clear(FLAG_BIG_TRAIN);
        event_flag_clear(FLAG_BIG_UNK_87);
        break;
    case 3:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        story_play_event(211);
        break;
    case 4:
        event_flag_set(FLAG_BIG_TRAIN);
        event_flag_set(FLAG_BIG_UNK_87);
        break;
    case 5:
        story_going_to(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
