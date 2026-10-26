/* E-102's Mystic Ruins story section, GO EGGCARRIER */
#include "adv02/script_e102_go_egg_carrier.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_e102_go_egg_carrier_init(void);
static void script_e102_go_egg_carrier_step(int seq);

struct story_script script_e102_go_egg_carrier = {
    script_e102_go_egg_carrier_init, script_e102_go_egg_carrier_step};

static void script_e102_go_egg_carrier_init(void)
{
    if (stage_cleared(STAGE_MOUNTAIN)) {
        story_set_time(TIME_NIGHT);
    }
}

/* event 197 plays once Red Mountain is cleared, and the raft opens. the last
 * step clears the record's wait through story_going_to, whose result goes
 * unread */
static void script_e102_go_egg_carrier_step(int seq)
{
    switch (seq) {
    case 0:
        if (!stage_cleared(STAGE_MOUNTAIN)) {
            return;
        }
        story_play_event(197);
        break;
    case 1:
        event_flag_set(FLAG_E102_RAFT);
        event_flag_clear(FLAG_E102_UNK_86);
        break;
    case 2:
        story_going_to(STAGE_EGG_CARRIER, 0);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
