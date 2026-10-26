/* Tails' Station Square story section, CASINO - HOOREE */
#include "adv00/script_tails_casino.h"
#include "adv00/station_square.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_tails_casino_init(void);
static void script_tails_casino_step(int seq);

struct story_script script_tails_casino = {script_tails_casino_init,
                                           script_tails_casino_step};

static void script_tails_casino_init(void)
{
    story_set_time(TIME_NIGHT);
}

/* the barricade goes, and the hotel's back and the station's back open. once
 * Casinopolis is cleared, events 375 and 53 play in the casino act. the act
 * then ends as a clear that returns to the casino act */
static void script_tails_casino_step(int seq)
{
    switch (seq) {
    case 0:
        event_flag_set(FLAG_TAILS_SS_BARRICADE);
        event_flag_set(FLAG_TAILS_SS_HOTEL_BACK);
        event_flag_set(FLAG_TAILS_SS_STATION_BACK);
        story_save();
        break;
    case 1:
        break;
    case 2:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO)) {
            return;
        }
        if (!stage_cleared(STAGE_CASINO)) {
            return;
        }
        event_flag_set(FLAG_TAILS_SS_ICE_STONE);
        story_play_event(375);
        break;
    case 3:
        story_play_event(53);
        break;
    case 4:
        stage_fade_to(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO);
        stage_end(STAGE_END_CLEAR);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
