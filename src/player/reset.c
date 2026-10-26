/* put a player back where the stage places it, or hold it still with its
 * pad off */
#include <sg_xpt.h>

#include "player/player.h"
#include "player/reset.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"

void player_place_start(Sint8 player)
{
    struct task_work *work = player_work[player];

    if (work != NULL) {
        stage_place_player(work);
    }
}

void player_hold(Sint8 player)
{
    pad_disable_player(player);
    player_clear_speed(player_motion[player], player_get(player));
}
