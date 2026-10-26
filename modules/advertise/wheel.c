/* the stories on the wheel of characters, and how many of them it shows */
#include "advertise/wheel.h"
#include "object/common.h"
#include "progress/progress.h"

int wheel_count(void)
{
    if (menu_state.mode == MENU_MODE_ADVENTURE) {
        if (menu_state.all_open[0] != 0) {
            return STORIES;
        }
        if (event_flag_check(FLAG_ALL_CLEAR)) {
            return STORIES;
        }
    }
    return STORY_LAST;
}
