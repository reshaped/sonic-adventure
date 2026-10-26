/* O ICICLE: an icicle Sonic catches hold of and hangs from */
#include <mathf.h>
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg08/icicle.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT icicle_object;

/* the index in icicle_shapes of the grip, the shape Sonic catches hold of */
#define ICICLE_GRIP 3

/* a Sonic's touch of the grip makes player 0 catch it. player 0 must be
 * falling, or have held jump more than CATCH_JUMP_STEPS steps. he must also
 * be within CATCH_CLOSE of the icicle's position, or not moving off it.
 * moving off means the dot product of his speed and his offset from it is
 * CATCH_AWAY or more */
#define CATCH_JUMP_STEPS 5
#define CATCH_CLOSE 1.0F
#define CATCH_AWAY 0.5F

static void icicle_destroy(struct task *task);
static void icicle_run(struct task *task);
static void icicle_draw(struct task *task);

/* three cylinders down the icicle, then the grip, a wider one that pushes
 * nothing */
/* clang-format off */
static struct collision_shape icicle_shapes[4] = {
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, -4.0F, 0.0F}, 3.0F, 4.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, -16.0F, 0.0F}, 0.8F, 8.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, -0.0F, 0.0F}, 0.1F, 1.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_NO_PUSH | SHAPE_HURTABLE | SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, -15.0F, 0.0F}, 10.0F, 7.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

void icicle_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        MODEL(work)->object = &icicle_object;
        if (!placement_editing(task)) {
            collision_init(task, icicle_shapes, COLLISION_OBJECT, 1, 4);
        }
        task->destructor = icicle_destroy;
        task->step = icicle_run;
        task->draw = icicle_draw;
    }
}

static void icicle_destroy(struct task *task)
{
}

/* the icicle's step, which draws it too. once player 0 catches the grip,
 * he hangs from the icicle and turns to face it. while he hangs from it, the
 * icicle's shapes stay out of the collision checks */
static void icicle_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision;
    NJS_VECTOR offset;
    struct task_work *player;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision = work->collision;
        if ((collision->flags & COLLISION_HIT) &&
            collision->hit->kind == COLLISION_PLAYER &&
            PLAYER_VARS(collision->hit->owner->work)->character ==
                CHARACTER_SONIC &&
            collision->own_shape == ICICLE_GRIP &&
            (player_motion[0]->speed.y < 0.0F ||
             player_get(0)->jump_steps > CATCH_JUMP_STEPS)) {
            player = player_work[0];
            offset.x = player->pos.x - work->pos.x;
            offset.y = player->pos.y - work->pos.y;
            offset.z = player->pos.z - work->pos.z;
            if (njScalor(&offset) < CATCH_CLOSE ||
                njInnerProduct(&player_motion[0]->speed, &offset) <
                    CATCH_AWAY) {
                player_dangle(0, task);
                player_set_angles(
                    0, work->ang.x,
                    (Angle)(atan2f(-offset.x, -offset.z) * ANGLE_PER_RADIAN),
                    work->ang.z);
            }
        }
        if (player_get(0)->held != task) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
    }
    icicle_draw(task);
}

static void icicle_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}
