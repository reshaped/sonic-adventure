/* Sonic's first Station Square story section, OPENING - CHAOS0 */
#include "adv00/script_sonic_opening.h"
#include "adv00/station_square.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_sonic_opening_init(void);
static void script_sonic_opening_step(int seq);

struct story_script script_sonic_opening = {script_sonic_opening_init,
                                            script_sonic_opening_step};

static void script_sonic_opening_init(void)
{
    story_set_time(TIME_NIGHT);
}

/* after event 1 in the station act, the section goes to Chaos 0 */
static void script_sonic_opening_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        story_play_event(1);
        break;
    case 1:
        story_go_to(STAGE_CHAOS0, 0);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
