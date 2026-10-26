/* the Egg Hornet's camera. the fight sets its place and speed, and the
 * follow hands them to the boss camera each step */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm1/camera.h"
#include "camera/camera.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

/* the speed egg_hornet_camera_turn sets, as the boss turns to fly. it is
 * data that nothing writes */
static Float camera_turn_speed = 4.0F;

/* the place's offset from player 0, which the camera's y angle turns */
static NJS_POINT3 camera_offset;
/* how fast the camera moves to its place. each step it moves up to 2.5
 * times this in x and z. in y it moves up to 1.5 times this */
static Float camera_speed;

void egg_hornet_camera_turn(void)
{
    camera_offset.x = 0.0F;
    camera_offset.y = 10.0F;
    camera_offset.z = 40.0F;
    camera_speed = camera_turn_speed;
}

void egg_hornet_camera_wide(void)
{
    camera_offset.x = 0.0F;
    camera_offset.y = 40.0F;
    camera_offset.z = 100.0F;
    camera_speed = 1.0F;
}

void egg_hornet_camera_near(void)
{
    camera_offset.x = 0.0F;
    camera_offset.y = 10.0F;
    camera_offset.z = 40.0F;
    camera_speed = 1.0F;
}

void egg_hornet_camera_snap(void)
{
    camera_offset.x = 0.0F;
    camera_offset.y = 10.0F;
    camera_offset.z = 40.0F;
    camera_speed = 100.0F;
}

void egg_hornet_camera_follow(struct task *task)
{
    NJS_POINT3 target;
    NJS_VECTOR move;
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    target.x = (work->pos.x + player->pos.x) * 0.5F;
    target.y = (work->pos.y + player->pos.y) * 0.5F;
    target.z = (work->pos.z + player->pos.z) * 0.5F;
    VECTOR_DIFF(&target, &target, &work->pos);
    camera_set_boss_offsets(&target, &camera_offset);
    move.x = camera_speed * 2.5F;
    move.y = camera_speed * 1.5F;
    move.z = 0.0F;
    camera_set_boss_limits(NULL, NULL, &move);
}
