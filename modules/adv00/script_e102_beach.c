/* E-102's Station Square story section, BEACH */
#include "adv00/script_e102_beach.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_e102_beach_init(void);
static void script_e102_beach_step(int seq);

struct story_script script_e102_beach = {script_e102_beach_init,
                                         script_e102_beach_step};

static void script_e102_beach_init(void)
{
    story_set_time(TIME_DAY);
}

/* the hotel's front opens, and the section ends as the game goes to Emerald
 * Coast */
static void script_e102_beach_step(int seq)
{
    switch (seq) {
    case 0:
        event_flag_set(FLAG_E102_SS_HOTEL_FRONT);
        event_flag_set(FLAG_E102_UNK_86);
        story_save();
        break;
    case 1:
        if (!story_going_to(STAGE_BEACH, 0)) {
            return;
        }
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
