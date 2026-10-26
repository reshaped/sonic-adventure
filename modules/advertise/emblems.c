/* the emblems of the menus' save, saves[save_slot]: how many it has, and
 * whether it has one */
#include <sg_xpt.h>

#include "advertise/emblems.h"
#include "advertise/records.h"
#include "advertise/trial.h"
#include "progress/emblem.h"
#include "save/save.h"

Uint32 emblems_won(void)
{
    return emblem_count(record_save());
}

Uint32 emblem_won(int story, int stage, int mission)
{
    struct save *save = record_save();
    int number = emblem_mission_number(story_characters[story], stage, mission);

    return emblem_check(save, number);
}
