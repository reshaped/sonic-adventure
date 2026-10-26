/* E-102's Mystic Ruins story section, WINDY */
#include "adv02/script_e102_windy.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_e102_windy_init(void);
static void script_e102_windy_step(int seq);

struct story_script script_e102_windy = {script_e102_windy_init,
                                         script_e102_windy_step};

static void script_e102_windy_init(void)
{
    story_set_time(TIME_DAY);
}

/* event 195 plays once the first step has set the section's flags and saved.
 * the last step clears the record's wait through story_going_to, whose result
 * goes unread */
static void script_e102_windy_step(int seq)
{
    switch (seq) {
    case 0:
        event_flag_set(FLAG_E102_TRAIN);
        event_flag_set(FLAG_E102_MR_TRUCK);
        event_flag_set(FLAG_E102_MR_APPEAR_FINAL_EGG);
        event_flag_set(FLAG_E102_MR_FREE_PASS);
        event_flag_set(FLAG_E102_SS_STATION_FRONT);
        event_flag_set(FLAG_E102_EC_SINK);
        story_save();
        break;
    case 1:
        story_play_event(195);
        break;
    case 2:
        break;
    case 3:
        story_going_to(STAGE_WINDY, 0);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
