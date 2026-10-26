/* Amy's Mystic Ruins story section, GO EGGCARRIER */
#include "adv02/mystic_ruins.h"
#include "adv02/script_amy_go_egg_carrier.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_amy_go_egg_carrier_init(void);
static void script_amy_go_egg_carrier_step(int seq);

struct story_script script_amy_go_egg_carrier = {
    script_amy_go_egg_carrier_init, script_amy_go_egg_carrier_step};

static void script_amy_go_egg_carrier_init(void)
{
    if (stage_cleared(STAGE_FINAL)) {
        story_set_time(TIME_DAY);
    } else {
        story_set_time(TIME_NIGHT);
    }
}

/* event 112 plays once the Final Egg is cleared. the section then ends with
 * the record's wait cleared */
static void script_amy_go_egg_carrier_step(int seq)
{
    switch (seq) {
    case 0:
        if (!stage_cleared(STAGE_FINAL)) {
            return;
        }
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_BASE)) {
            return;
        }
        story_play_event(112);
        break;
    case 1:
        event_flag_clear(FLAG_AMY_UNK_85);
        player_record->wait_at = STORY_NO_WAIT;
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
