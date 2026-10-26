/* Sonic's Egg Carrier story section, EC HALL - CHAOS6 */
#include <ninja.h>

#include "adv01obj/script_sonic_chaos6.h"
#include "adv02/mystic_ruins.h"
#include "player/player.h"
#include "progress/playable.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

/* event 262 plays once player 0's z is under EVENT_262_Z */
#define EVENT_262_Z 3000.0F
/* the story goes on to Chaos 6 once player 0's z is under CHAOS6_Z */
#define CHAOS6_Z (-200.0F)

static void script_sonic_chaos6_init(void);
static void script_sonic_chaos6_step(int seq);

struct story_script script_sonic_chaos6 = {script_sonic_chaos6_init,
                                           script_sonic_chaos6_step};

static void script_sonic_chaos6_init(void)
{
}

static void script_sonic_chaos6_step(int seq)
{
    switch (seq) {
    case 0:
        story_play_event(260);
        event_flag_set(FLAG_SONIC_EC_TRANSFORM);
        event_flag_set(FLAG_SONIC_EC_EGG_LIFT);
        break;
    case 1:
        if (!story_wait_at(STAGE_EGG_CARRIER, 2)) {
            return;
        }
        story_save();
        break;
    case 2:
        story_play_event(26);
        break;
    case 3:
        battle_start(SONIC_VS_E102);
        break;
    case 4:
        if (!event_flag_check(FLAG_SONIC_MBOSS_E102)) {
            return;
        }
        story_save();
        break;
    case 5:
        story_play_event(27);
        break;
    case 6:
        playable_unlock(CHARACTER_E102);
        break;
    case 7:
        if (!story_wait_at(STAGE_EGG_CARRIER, 2)) {
            return;
        }
        if (player_work[0]->pos.z < EVENT_262_Z) {
            story_play_event(262);
        } else {
            return;
        }
        break;
    case 8:
        if (!story_wait_at(STAGE_EGG_CARRIER, 3)) {
            return;
        }
        if (!event_flag_check(FLAG_SONIC_EC_TRANSFORM)) {
            story_play_event(263);
        } else {
            return;
        }
        break;
    case 9:
        event_flag_set(FLAG_SONIC_EC_MONORAIL);
        story_save();
        break;
    case 10:
        if (!story_wait_at(STAGE_EGG_CARRIER, 0)) {
            return;
        }
        if (player_work[0]->pos.z < CHAOS6_Z) {
            story_go_to(STAGE_CHAOS6, 0);
        } else {
            return;
        }
        break;
    case 11:
        if (!stage_cleared(STAGE_CHAOS6)) {
            return;
        }
        story_play_event(29);
        break;
    case 12:
        event_flag_set(FLAG_SONIC_UNK_82);
        event_flag_set(FLAG_SONIC_BOAT);
        event_flag_set(FLAG_SONIC_RAFT);
        event_flag_set(146);
        event_flag_set(FLAG_SONIC_EC_EGG_LIFT);
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
