/* three pillars and a terrace: Hasira1, Hasira2, Sekicyuu and Terasu. each
 * only stands where the set file places it */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg07/pillars.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL ruin_pillar_1_model;
extern NJS_MODEL ruin_pillar_2_model;
extern NJS_MODEL ruin_stone_pillar_model;
extern NJS_MODEL ruin_terrace_model;

/* which of them a task is, in its work's mode */
enum pillar_kind { PILLAR_1, PILLAR_2, STONE_PILLAR, TERRACE };

/* the first pillar's two shapes, then one for each of the rest */
/* clang-format off */
static struct collision_shape ruin_pillar_shapes[5] = {
    {SHAPE_CAPSULE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, -15.0F, 0.0F},
     10.0F, 100.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 93.0F, 0.0F},
     12.0F, 3.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, -49.0F, 0.0F},
     17.0F, 60.0F, 17.0F, {0}, 0, NULL},
    {SHAPE_CAPSULE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F},
     5.0F, 16.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.5F, 0.0F},
     1.5F, 7.0F, 1.5F, {0}, 0, NULL},
};
/* clang-format on */

static void ruin_pillar_destroy(struct task *task)
{
}

/* the task's step, and its draw, which runs only while the game is
 * paused. the shapes are registered then too */
static void ruin_pillar_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    switch (work->mode) {
    case PILLAR_1:
        draw_model(&ruin_pillar_1_model);
        break;
    case PILLAR_2:
        draw_model(&ruin_pillar_2_model);
        break;
    case STONE_PILLAR:
        draw_model(&ruin_stone_pillar_model);
        break;
    case TERRACE:
        draw_model(&ruin_terrace_model);
        break;
    }
    njPopMatrix(1);
}

static void ruin_pillar_init(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case PILLAR_1:
        collision_init(task, &ruin_pillar_shapes[0], COLLISION_OBJECT, 1, 2);
        break;
    case PILLAR_2:
        collision_init(task, &ruin_pillar_shapes[2], COLLISION_OBJECT, 1, 1);
        break;
    case STONE_PILLAR:
        collision_init(task, &ruin_pillar_shapes[3], COLLISION_OBJECT, 1, 1);
        break;
    case TERRACE:
        collision_init(task, &ruin_pillar_shapes[4], COLLISION_OBJECT, 1, 1);
        break;
    }
    task->step = ruin_pillar_draw;
    task->draw = ruin_pillar_draw;
    task->destructor = ruin_pillar_destroy;
}

void ruin_pillar_1_step(struct task *task)
{
    task->work->mode = PILLAR_1;
    ruin_pillar_init(task);
}

void ruin_pillar_2_step(struct task *task)
{
    task->work->mode = PILLAR_2;
    ruin_pillar_init(task);
}

void ruin_stone_pillar_step(struct task *task)
{
    task->work->mode = STONE_PILLAR;
    ruin_pillar_init(task);
}

void ruin_terrace_step(struct task *task)
{
    task->work->mode = TERRACE;
    ruin_pillar_init(task);
}
