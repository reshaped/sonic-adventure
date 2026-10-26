/* Sonic's Mystic Ruins story section, MOBILE1 - WINDY */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv02/script_sonic_mobile1_windy.h"
#include "adv00/station_square.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

/* player 0 starts event 7 in the station act, by the workshop, once its x, y
 * and z are all over these */
#define WORKSHOP_X 800.0F
#define WORKSHOP_Y 120.0F
#define WORKSHOP_Z 660.0F

static void script_sonic_mobile1_windy_init(void);
static void script_sonic_mobile1_windy_step(int seq);

struct story_script script_sonic_mobile1_windy = {
    script_sonic_mobile1_windy_init, script_sonic_mobile1_windy_step};

static void script_sonic_mobile1_windy_init(void)
{
    story_set_time(TIME_DAY);
}

/* event 7 plays by the workshop before Egg Mobile 1, and event 8 after it. the
 * section ends once Windy Valley is cleared. its last step clears the record's
 * wait through story_going_to, whose result goes unread */
static void script_sonic_mobile1_windy_step(int seq)
{
    struct task_work *work;

    switch (seq) {
    case 1:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        work = player_work[0];
        if (work->pos.z > WORKSHOP_Z && work->pos.x > WORKSHOP_X &&
            work->pos.y > WORKSHOP_Y) {
            story_play_event(7);
        } else {
            return;
        }
        break;
    case 2:
        story_go_to(STAGE_EGGMOBILE1, 0);
        break;
    case 3:
        if (!stage_cleared(STAGE_EGGMOBILE1)) {
            return;
        }
        story_play_event(8);
        break;
    case 0:
    case 4:
        story_save();
        break;
    case 5:
        if (!stage_cleared(STAGE_WINDY)) {
            return;
        }
        break;
    case 6:
        story_going_to(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
