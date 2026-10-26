/* O BCHAIR beach chair by the pool, solid for a player to stand on */
#include <ninja.h>

#include "adv0100/beach_chair.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT beach_chair_object;
extern NJS_OBJECT beach_chair_solid_object;

#define SOLID_FLAGS (WORLD_MEDIUM | WORLD_SOLID)

struct beach_chair_vars {
    NJS_OBJECT *solid; /* the surface's copy in the collision world */
    NJS_OBJECT *from; /* the node whose model solid takes */
};

#define VARS(work) ((struct beach_chair_vars *)&(work)->vars)

enum beach_chair_mode { CHAIR_INIT, CHAIR_RUN };

static void beach_chair_draw(struct task *task);
static void beach_chair_destroy(struct task *task);
static void beach_chair_draw_work(struct task_work *work);

void egg_carrier_beach_chair_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *solid;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case CHAIR_INIT:
        if (!placement_editing(task)) {
            VARS(work)->from = &beach_chair_solid_object;
            VARS(work)->solid = solid = world_object_alloc();
            solid->evalflags = NJD_EVAL_BREAK;
            solid->scl[0] = 1.0F;
            solid->scl[1] = 1.0F;
            solid->scl[2] = 1.0F;
            solid->ang[0] = 0;
            solid->ang[1] = work->ang.y;
            solid->ang[2] = 0;
            solid->pos[0] = work->pos.x;
            solid->pos[1] = work->pos.y;
            solid->pos[2] = work->pos.z;
            solid->model = VARS(work)->from->model;
            solid->child = NULL;
            solid->sibling = NULL;
            world_add(SOLID_FLAGS, task, solid);
        }
        task->destructor = beach_chair_destroy;
        task->draw = beach_chair_draw;
        work->mode = CHAIR_RUN;
        break;
    case CHAIR_RUN:
        beach_chair_draw_work(work);
        break;
    }
}

static void beach_chair_draw(struct task *task)
{
    struct task_work *work = task->work;

    beach_chair_draw_work(work);
}

static void beach_chair_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}

static void beach_chair_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&beach_chair_object);
    njPopMatrix(1);
}
