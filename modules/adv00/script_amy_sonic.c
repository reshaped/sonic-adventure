/* Amy's Station Square story section, SONIC - TWINKLE */
#include "adv00/script_amy_sonic.h"
#include "adv00/station_square.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

/* event 99 plays once player 0 is beyond both this x and this z */
#define EVENT_REACH_X 320.0F
#define EVENT_REACH_Z 1530.0F

static void script_amy_sonic_init(void);
static void script_amy_sonic_step(int seq);

struct story_script script_amy_sonic = {script_amy_sonic_init,
                                        script_amy_sonic_step};

static void script_amy_sonic_init(void)
{
    story_set_time(TIME_EVENING);
}

/* after event 97, the hotel's front and back open, and event 98 plays in the
 * casino act. after event 99 in the station act, the Twinkle Park elevator
 * opens and the section goes to Twinkle Park. once it is cleared, events 100
 * and 117 lead from the station act through the casino act inside the Egg
 * Carrier */
static void script_amy_sonic_step(int seq)
{
    switch (seq) {
    case 0:
        break;
    case 1:
        break;
    case 2:
        story_play_event(97);
        break;
    case 3:
        event_flag_set(FLAG_AMY_SS_HOTEL_FRONT);
        event_flag_set(FLAG_AMY_SS_HOTEL_BACK);
        event_flag_set(FLAG_AMY_UNK_85);
        story_save();
        break;
    case 4:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO)) {
            return;
        }
        story_play_event(98);
        break;
    case 5:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        if (player_work[0]->pos.x > EVENT_REACH_X &&
            player_work[0]->pos.z > EVENT_REACH_Z) {
            story_play_event(99);
            event_flag_set(FLAG_AMY_SS_TPARK_ELEVATOR);
        } else {
            return;
        }
        break;
    case 6:
        story_go_to(STAGE_TWINKLE, 1);
        break;
    case 7:
        if (!stage_cleared(STAGE_TWINKLE)) {
            return;
        }
        event_flag_set(FLAG_AMY_SS_ENTRANCE_CIRCUIT);
        break;
    case 8:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        story_play_event(100);
        break;
    case 9:
        stage_set_entrance(0);
        story_go_to(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO);
        break;
    case 10:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO)) {
            return;
        }
        story_play_event(117);
        break;
    case 11:
        story_go_to(STAGE_EGG_CARRIER_INSIDE, 3);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
