/* Tails' Mystic Ruins story section, ICECAP - SHOOTING */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv02/script_tails_icecap_shooting.h"
#include "player/player.h"
#include "progress/playable.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

/* player 0 starts event 56 in the station act once its x and its z are both
 * under these */
#define STATION_X 500.0F
#define STATION_Z 450.0F

/* the entrance the next act starts player 0 at */
#define BATTLE_ENTRANCE 5

static void script_tails_icecap_shooting_init(void);
static void script_tails_icecap_shooting_step(int seq);

struct story_script script_tails_icecap_shooting = {
    script_tails_icecap_shooting_init, script_tails_icecap_shooting_step};

/* player 0 starts event 59 within 144 units of this point across, and 10 above
 * or below it, by the workshop */
static NJS_POINT3 script_tails_icecap_shooting_point = {1422.0F, 202.0F,
                                                        737.0F};

static void script_tails_icecap_shooting_init(void)
{
    story_set_time(TIME_DAY);
}

/* event 379 plays first. once Ice Cap is cleared, event 56 plays and Knuckles
 * fights Tails. once Tails wins, event 57 plays before Chaos 4 and event 58
 * after it. event 59 then plays by the workshop, before the Sky Chase */
static void script_tails_icecap_shooting_step(int seq)
{
    switch (seq) {
    case 0:
        event_flag_set(FLAG_TAILS_MR_WEST_ROCK);
        story_play_event(379);
        break;
    case 1:
        story_save();
        break;
    case 2:
        if (!stage_cleared(STAGE_ICECAP)) {
            return;
        }
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        if (player_work[0]->pos.z < STATION_Z &&
            player_work[0]->pos.x < STATION_X) {
            story_play_event(56);
        } else {
            return;
        }
        break;
    case 3:
        battle_start(TAILS_VS_KNUCKLES);
        stage_set_entrance(BATTLE_ENTRANCE);
        break;
    case 4:
        if (!event_flag_check(FLAG_TAILS_MBOSS_KNUCKLES)) {
            return;
        }
        story_save();
        break;
    case 5:
        story_play_event(57);
        break;
    case 6:
        story_go_to(STAGE_CHAOS4, 0);
        break;
    case 7:
        if (!stage_cleared(STAGE_CHAOS4)) {
            return;
        }
        break;
    case 8:
        story_play_event(58);
        break;
    case 9:
        playable_unlock(CHARACTER_KNUCKLES);
        break;
    case 10:
        if (story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION) &&
            player_in_cylinder(&script_tails_icecap_shooting_point, 144.0F,
                               10.0F) == 1) {
            story_play_event(59);
        } else {
            return;
        }
        break;
    case 11:
        story_go_to(STAGE_SHOOTING1, 0);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
