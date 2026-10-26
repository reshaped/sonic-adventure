/* Chaos 2's fight camera. it aims at the boss while player 0 is lower than
 * chaos2_camera_player_y, and at player 0 from there up. it rises no higher
 * than chaos2_camera_top */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos2/camera.h"
#include "camera/camera.h"
#include "draw/motion.h"
#include "player/player.h"
#include "system/task.h"

static Float chaos2_camera_top = 35.0F;
static Float chaos2_camera_player_y = 30.0F;

void chaos2_camera_step(struct camera_view *view)
{
    struct task_work *player = player_work[0];

    if (player->pos.y < chaos2_camera_player_y) {
        camera_state->target.x = boss_work->pos.x;
        camera_state->target.y = boss_work->pos.y;
        camera_state->target.z = boss_work->pos.z;
    } else {
        camera_state->target.x = player->pos.x;
        camera_state->target.y = player->pos.y;
        camera_state->target.z = player->pos.z;
    }
    camera_state->target.x += camera_boss_target_offset.x;
    camera_state->target.y += camera_boss_target_offset.y;
    camera_state->target.z += camera_boss_target_offset.z;
    camera_boss_turn(0);
    camera_boss_place_from_player(1);
    camera_boss_move(0);
    camera_state->pos.y = camera_state->pos.y < chaos2_camera_top
                              ? camera_state->pos.y
                              : chaos2_camera_top;
}
