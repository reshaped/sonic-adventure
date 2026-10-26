/* Sonic's Station Square story section, AMY - TWINKLE */
#include "adv00/script_sonic_amy.h"
#include "adv00/station_square.h"
#include "player/player.h"
#include "progress/playable.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

/* event 19 plays once player 0 is beyond both this x and this z */
#define EVENT_REACH_X 320.0F
#define EVENT_REACH_Z 1530.0F

static void script_sonic_amy_init(void);
static void script_sonic_amy_step(int seq);

struct story_script script_sonic_amy = {script_sonic_amy_init,
                                        script_sonic_amy_step};

static void script_sonic_amy_init(void)
{
    story_set_time(TIME_EVENING);
}

/* after event 17, the station's front and back and the train close. after event
 * 18 in the casino act, Amy is playable. after event 19 in the station act, the
 * Twinkle Park elevator opens and the section goes to Twinkle Park */
static void script_sonic_amy_step(int seq)
{
    switch (seq) {
    case 0:
        story_play_event(17);
        break;
    case 1:
        event_flag_clear(FLAG_SONIC_SS_STATION_FRONT);
        event_flag_clear(FLAG_SONIC_SS_STATION_BACK);
        event_flag_clear(FLAG_SONIC_TRAIN);
        event_flag_clear(FLAG_SONIC_UNK_82);
        story_save();
        break;
    case 2:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO)) {
            return;
        }
        story_play_event(18);
        playable_unlock(CHARACTER_AMY);
        break;
    case 3:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        if (player_work[0]->pos.x > EVENT_REACH_X &&
            player_work[0]->pos.z > EVENT_REACH_Z) {
            story_play_event(19);
            event_flag_set(FLAG_SONIC_SS_TPARK_ELEVATOR);
        } else {
            return;
        }
        break;
    case 4:
        story_go_to(STAGE_TWINKLE, 0);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
