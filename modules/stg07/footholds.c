/* the footholds Ashiba01 to Ashiba03 and the foothold walls Ashikabe01 and
 * Ashikabe02. each holds a solid copy of its model in the world, where the
 * set file puts it. each draws the copy there */
#include <ninja.h>

#include "collision/world.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg07/footholds.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT ruin_foothold_1_object;
extern NJS_OBJECT ruin_foothold_2_object;
extern NJS_OBJECT ruin_foothold_3_object;
extern NJS_OBJECT ruin_foothold_wall_1_object;
extern NJS_OBJECT ruin_foothold_wall_2_object;

/* the foothold ruin_foothold_init makes */
enum foothold_kind {
    FOOTHOLD_1,
    FOOTHOLD_2,
    FOOTHOLD_3,
    FOOTHOLD_WALL_1,
    FOOTHOLD_WALL_2
};

struct foothold_vars {
    NJS_OBJECT *object; /* the copy of its model in the world */
};

#define VARS(work) ((struct foothold_vars *)&(work)->vars)

#define FOOTHOLD_FLAGS (WORLD_MEDIUM | WORLD_SOLID)

/* the world tests a foothold while player 0 or 1 is within its model's
 * radius and FOOTHOLD_REACH more. a foothold draws while a sphere of
 * FOOTHOLD_DRAW_RADIUS round it is in view */
#define FOOTHOLD_REACH 20.0F
#define FOOTHOLD_DRAW_RADIUS 30.0F

static void ruin_foothold_destroy(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;

    world_object_free(object);
    world_remove(task, object);
    /* @hack redundant: a return where the function ends, which makes the frame
     * with ADD #-4,R15 and stores task in it across world_object_free
     * (0x0c900004, 0x0c900006); without it, a push does both, the function
     * runs 2 bytes short and 13 of 16 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void ruin_foothold_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (player_in_range(&work->pos, object->model->r + FOOTHOLD_REACH)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                     FOOTHOLD_DRAW_RADIUS)) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        draw_model(object->model);
        njPopMatrix(1);
    }
}

static void ruin_foothold_init(struct task *task, enum foothold_kind kind)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    switch (kind) {
    case FOOTHOLD_1:
        VARS(work)->object =
            object_add_world(&ruin_foothold_1_object, task, FOOTHOLD_FLAGS);
        break;
    case FOOTHOLD_2:
        VARS(work)->object =
            object_add_world(&ruin_foothold_2_object, task, FOOTHOLD_FLAGS);
        break;
    case FOOTHOLD_3:
        VARS(work)->object =
            object_add_world(&ruin_foothold_3_object, task, FOOTHOLD_FLAGS);
        break;
    case FOOTHOLD_WALL_1:
        VARS(work)->object = object_add_world(&ruin_foothold_wall_1_object,
                                              task, FOOTHOLD_FLAGS);
        break;
    case FOOTHOLD_WALL_2:
        VARS(work)->object = object_add_world(&ruin_foothold_wall_2_object,
                                              task, FOOTHOLD_FLAGS);
        break;
    }
    object = VARS(work)->object;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    task->step = ruin_foothold_run;
    task->draw = ruin_foothold_run;
    task->destructor = ruin_foothold_destroy;
}

void ruin_foothold_1_step(struct task *task)
{
    ruin_foothold_init(task, FOOTHOLD_1);
}

void ruin_foothold_2_step(struct task *task)
{
    ruin_foothold_init(task, FOOTHOLD_2);
}

void ruin_foothold_3_step(struct task *task)
{
    ruin_foothold_init(task, FOOTHOLD_3);
}

void ruin_foothold_wall_1_step(struct task *task)
{
    ruin_foothold_init(task, FOOTHOLD_WALL_1);
}

void ruin_foothold_wall_2_step(struct task *task)
{
    ruin_foothold_init(task, FOOTHOLD_WALL_2);
}
