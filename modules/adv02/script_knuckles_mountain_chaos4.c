/* Knuckles' Mystic Ruins story section, MOUNTAIN - CHAOS4 */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv02/script_knuckles_mountain_chaos4.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"
#include "system/task.h"

/* player 0 starts event 138 in the station act once its x and its z are both
 * under these */
#define STATION_X 500.0F
#define STATION_Z 450.0F

/* the entrance the next act starts player 0 at */
#define BATTLE_ENTRANCE 5

static void script_knuckles_mountain_chaos4_init(void);
static void script_knuckles_mountain_chaos4_step(int seq);

/* no code reads this point */
static NJS_POINT3 script_knuckles_mountain_chaos4_point = {1405.0F, 0.0F,
                                                           -180.0F};

struct story_script script_knuckles_mountain_chaos4 = {
    script_knuckles_mountain_chaos4_init, script_knuckles_mountain_chaos4_step};

static void script_knuckles_mountain_chaos4_init(void)
{
    event_flag_set(FLAG_KNUCKLES_MR_WEST_ROCK);
    story_set_time(TIME_DAY);
}

/* the island door opens for event 137. once Red Mountain is cleared, event 138
 * plays and Sonic fights Knuckles. once Knuckles wins, event 139 plays before
 * Chaos 4 and event 140 after it. the section ends once the game goes to
 * Knuckles' act of Lost World */
static void script_knuckles_mountain_chaos4_step(int seq)
{
    struct task_work *work;

    switch (seq) {
    case 0:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        event_flag_set(FLAG_KNUCKLES_MR_ISLAND_DOOR);
        break;
    case 1:
        story_play_event(137);
        break;
    case 2:
        story_save();
        break;
    /* @hack redundant: case 3's break, which gives case 3 a jump of its own
     * ahead of case 4's, as the switch table's entries at 0x0c9157da and
     * 0x0c9157dc show. with one break for both, the step grows by 32 bytes, and
     * measured with its P row lengthened, 122 of 154 words differ */
    case 3:
        break;
    case 4:
        break;
    case 5:
        if (!stage_cleared(STAGE_MOUNTAIN)) {
            return;
        }
        break;
    case 6:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        work = player_work[0];
        if (work->pos.z < STATION_Z && work->pos.x < STATION_X) {
            story_play_event(138);
        } else {
            return;
        }
        break;
    case 7:
        battle_start(KNUCKLES_VS_SONIC);
        stage_set_entrance(BATTLE_ENTRANCE);
        break;
    case 8:
        if (!event_flag_check(FLAG_KNUCKLES_MBOSS_SONIC)) {
            return;
        }
        story_save();
        break;
    case 9:
        story_play_event(139);
        break;
    case 10:
        story_go_to(STAGE_CHAOS4, 0);
        break;
    case 11:
        if (!stage_cleared(STAGE_CHAOS4)) {
            return;
        }
        break;
    case 12:
        story_play_event(140);
        break;
    case 13:
        event_flag_set(FLAG_KNUCKLES_MR_TRUCK);
        story_save();
        break;
    case 14:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE)) {
            return;
        }
        if (!story_going_to(STAGE_RUIN, 1)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
