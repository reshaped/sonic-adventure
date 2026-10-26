/* E-102's Mystic Ruins story section, MOUNTAIN */
#include "adv02/mystic_ruins.h"
#include "adv02/script_e102_mountain.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_e102_mountain_init(void);
static void script_e102_mountain_step(int seq);

struct story_script script_e102_mountain = {script_e102_mountain_init,
                                            script_e102_mountain_step};

static void script_e102_mountain_init(void)
{
    if (stage_cleared(STAGE_WINDY)) {
        story_set_time(TIME_EVENING);
    }
}

/* once Windy Valley is cleared, the west rock and the island door open for
 * event 380. the section ends once the game goes to E-102's act of Red
 * Mountain */
static void script_e102_mountain_step(int seq)
{
    switch (seq) {
    case 0:
        if (!stage_cleared(STAGE_WINDY)) {
            return;
        }
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION)) {
            return;
        }
        player_record->unk_01 = 2;
        event_flag_set(FLAG_E102_MR_WEST_ROCK);
        event_flag_set(FLAG_E102_MR_ISLAND_DOOR);
        story_play_event(380);
        break;
    case 1:
        if (!story_going_to(STAGE_MOUNTAIN, 1)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
