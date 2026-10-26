/* act 0's two floats: platforms that ride the rising water up, for Big */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/tree.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg12/floats.h"
#include "stg12/water.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT shelter_float_object;

#define FLOATS 2

#define REACH 50.0F

enum shelter_float_mode { FLOAT_STILL, FLOAT_WAIT, FLOAT_RISE, FLOAT_TOP };

struct shelter_float_vars {
    Float top;
};

#define VARS(work) ((struct shelter_float_vars *)&(work)->vars)

static void shelter_float_start(struct task *task);
static void shelter_float_step(struct task *task);
static void shelter_float_draw(struct task *task);

/* where each float starts, and the height it stops rising at */
static NJS_POINT3 shelter_float_places[FLOATS] = {
    {750.0F, -35.5F, -851.0F},
    {750.0F, 14.5F, -456.0F},
};
static Float shelter_float_tops[FLOATS] = {-11.5F, 44.5F};

static struct collision_shape shelter_float_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 10.5F, 0.0F},
    20.0F,
    15.0F,
    1.0F,
    {0},
    0,
    NULL,
};

void shelter_floats_start(void)
{
    struct task *task;
    struct task_work *work;
    int i;

    for (i = 0; i < FLOATS; i++) {
        task = task_create(TASK_WORK, 3, shelter_float_start);
        work = task->work;
        work->pos.x = shelter_float_places[i].x;
        work->pos.y = shelter_float_places[i].y;
        work->pos.z = shelter_float_places[i].z;
        VARS(work)->top = shelter_float_tops[i];
    }
}

static void shelter_float_start(struct task *task)
{
    struct task_work *work = task->work;

    if (player_get_character(0) == CHARACTER_BIG) {
        work->mode = FLOAT_WAIT;
    } else {
        work->mode = FLOAT_STILL;
    }
    TASK_TIMER(work) = stage_get_stage_act();
    collision_init(task, &shelter_float_shape, COLLISION_OBJECT, 1, 1);
    work->collision->radius = REACH;
    task->step = shelter_float_step;
    task->draw = shelter_float_draw;
}

static void shelter_float_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case FLOAT_STILL:
        collision_register(work);
        break;
    case FLOAT_WAIT:
        if (shelter_water_level >= work->pos.y) {
            work->pos.y = shelter_water_level;
            work->mode = FLOAT_RISE;
        }
        collision_register(work);
        break;
    case FLOAT_RISE:
        if (shelter_water_level >= VARS(work)->top) {
            work->pos.y = VARS(work)->top;
            work->mode = FLOAT_TOP;
        } else {
            work->pos.y = shelter_water_level;
        }
        collision_register(work);
        break;
    case FLOAT_TOP:
        collision_register(work);
        break;
    }
    shelter_float_draw(task);
}

static void shelter_float_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_tree(&shelter_float_object);
        njPopMatrix(1);
    }
}
