/* the jump stand: LostJumpdai, a solid stand that the set file places in the
 * world */
#include <ninja.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg07/jump_stand.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: an array of one, whose address jump_stand_step loads from its
 * pool to read the model 4 past it (MOV.L @(4,R3),R2 at 0x0c9034c0). as a
 * single object, the pool holds its model member's address too, and 9 of
 * jump_stand_step's 85 words differ */
extern NJS_OBJECT jump_stand_object[1];

struct jump_stand_vars {
    /* its copy in the world, or in the set editor, jump_stand_object */
    NJS_OBJECT *object;
};

#define VARS(work) ((struct jump_stand_vars *)&(work)->vars)

#define STAND_FLAGS (WORLD_MEDIUM | WORLD_SOLID)
/* the world tests the stand while player 0 or 1 is within STAND_REACH past
 * its model's radius */
#define STAND_REACH 20.0F

static void jump_stand_destroy(struct task *task);
static void jump_stand_run(struct task *task);
static void jump_stand_draw(struct task *task);

void jump_stand_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;

    if (placement_try_leave(task)) {
        return;
    }
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    task->destructor = jump_stand_destroy;
    task->step = jump_stand_run;
    task->draw = jump_stand_draw;
    if (!placement_editing(task)) {
        VARS(work)->object = object = world_object_alloc();
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = 1.0F;
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->model = jump_stand_object->model;
        work->flags |= WORLD_TESTED;
        world_add(STAND_FLAGS, task, object);
    } else {
        VARS(work)->object = jump_stand_object;
    }
}

static void jump_stand_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}

/* the stand's step: its object follows the task */
static void jump_stand_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (placement_try_leave(task)) {
        return;
    }
    object = VARS(work)->object;
    if (object != NULL) {
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
        if (player_in_range(&work->pos, object->model->r + STAND_REACH)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
    jump_stand_draw(task);
}

static void jump_stand_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(VARS(work)->object->model);
    njPopMatrix(1);
}
