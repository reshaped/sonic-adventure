/* O BigHone: a big bone, solid ground in the collision world */
#include <ninja.h>

#include "collision/world.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg08/big_bone.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT big_bone_object;

/* the collision world tests the bone only while a player is near it. that
 * is within its model's radius plus BONE_NEAR of the bone's position */
#define BONE_NEAR 20.0F

struct big_bone_vars {
    NJS_OBJECT *object; /* its copy of big_bone_object in the world */
};

#define VARS(work) ((struct big_bone_vars *)&(work)->vars)

static void big_bone_destroy(struct task *task)
{
    NJS_OBJECT *object = VARS(task->work)->object;

    world_remove(task, object);
    world_object_free(object);
}

/* the bone's step, and its draw too. out of range, it does nothing, and the
 * task ends on its next step */
static void big_bone_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;

    if (!placement_try_leave(task)) {
        if (player_in_range(&work->pos, object->model->r + BONE_NEAR)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        texture_set_stage();
        draw_model(object->model);
        njPopMatrix(1);
    }
}

void big_bone_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    VARS(work)->object =
        object_add_world(&big_bone_object, task, WORLD_LARGE | WORLD_SOLID);
    object = VARS(work)->object;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    task->step = big_bone_run;
    task->draw = big_bone_run;
    task->destructor = big_bone_destroy;
}
