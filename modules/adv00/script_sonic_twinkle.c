/* Sonic's Station Square story section, TWINKLE - HIGHWAY */
#include "adv00/script_sonic_twinkle.h"
#include "adv00/station_square.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_sonic_twinkle_init(void);
static void script_sonic_twinkle_step(int seq);

struct story_script script_sonic_twinkle = {script_sonic_twinkle_init,
                                            script_sonic_twinkle_step};

static void script_sonic_twinkle_init(void)
{
    if (stage_cleared(STAGE_TWINKLE)) {
        story_set_time(TIME_NIGHT);
    } else {
        story_set_time(TIME_EVENING);
    }
}

/* once Twinkle Park is cleared, Twinkle Circuit opens and event 20 plays. in
 * the station act, the employee card is made available and event 377 plays. the
 * section ends as the game goes to Speed Highway */
static void script_sonic_twinkle_step(int seq)
{
    switch (seq) {
    case 0:
        if (!stage_cleared(STAGE_TWINKLE)) {
            return;
        }
        event_flag_set(FLAG_SONIC_SS_ENTRANCE_CIRCUIT);
        story_play_event(20);
        break;
    case 1:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        event_flag_set(FLAG_SONIC_SS_CARD);
        story_play_event(377);
        break;
    case 2:
        story_save();
        break;
    case 3:
        if (!story_going_to(STAGE_HIGHWAY, 0)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
