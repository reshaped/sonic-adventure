/* E-102's Egg Carrier story section, AMY - SONIC */
#include "adv01obj/script_e102_amy.h"
#include "adv02/mystic_ruins.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_e102_amy_init(void);
static void script_e102_amy_step(int seq);

struct story_script script_e102_amy = {script_e102_amy_init,
                                       script_e102_amy_step};

static void script_e102_amy_init(void)
{
}

static void script_e102_amy_step(int seq)
{
    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_EGG_CARRIER_INSIDE, 1)) {
            return;
        }
        story_play_event(187);
        event_flag_set(FLAG_E102_EC_TRANSFORM);
        break;
    case 1:
        event_flag_clear(FLAG_E102_UNK_86);
        story_save();
        break;
    case 2:
        story_play_event(188);
        break;
    case 3:
        story_play_event(189);
        break;
    case 4:
        story_play_event(190);
        break;
    case 5:
        story_save();
        break;
    case 6:
        if (!story_wait_at(STAGE_EGG_CARRIER_INSIDE, 3)) {
            return;
        }
        story_play_event(191);
        break;
    case 7:
        playable_set(CHARACTER_AMY);
        break;
    case 8:
        if (!story_wait_at(STAGE_EGG_CARRIER_INSIDE, 1)) {
            return;
        }
        story_play_event(320);
        break;
    case 9:
        if (!story_wait_at(STAGE_EGG_CARRIER_INSIDE, 0)) {
            return;
        }
        if (!event_flag_check(FLAG_E102_BOOSTER)) {
            return;
        }
        event_flag_set(FLAG_E102_EC_EGG_LIFT);
        break;
    case 10:
        if (!story_wait_at(STAGE_EGG_CARRIER_INSIDE, 1)) {
            return;
        }
        story_play_event(192);
        break;
    case 11:
        if (!story_wait_at(STAGE_EGG_CARRIER, 2)) {
            return;
        }
        story_save();
        break;
    case 12:
        story_play_event(193);
        break;
    case 13:
        battle_start(E102_VS_SONIC);
        break;
    case 14:
        if (!event_flag_check(FLAG_E102_MBOSS_SONIC)) {
            return;
        }
        break;
    case 15:
        if (!story_wait_at(STAGE_EGG_CARRIER, 2)) {
            return;
        }
        story_play_event(194);
        break;
    case 16:
        event_flag_clear(FLAG_E102_EC_TRANSFORM);
        event_flag_set(395);
        event_flag_set(FLAG_E102_EC_MONORAIL);
        event_flag_set(FLAG_E102_BOAT);
        event_flag_set(FLAG_E102_RAFT);
        event_flag_set(FLAG_E102_UNK_86);
        story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
