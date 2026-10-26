/* O Escalator1 and O Escalator2 escalators whose steps scroll */
#include <ninja.h>

#include "collision/world.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg04/escalator.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* each escalator's surface in the collision world, its body, and its steps */
extern NJS_OBJECT escalator_1_object;
extern NJS_MODEL escalator_1_model;
extern NJS_MODEL escalator_1_steps_model;
extern NJS_OBJECT escalator_2_object;
extern NJS_MODEL escalator_2_model;
extern NJS_MODEL escalator_2_steps_model;

/* the escalator's range for placement_try_leave_by, squared */
#define ESCALATOR_RANGE (2040.0F * 2040.0F)

/* the collision world tests the escalator while a player is within twice its
 * surface's radius plus NEAR_MARGIN */
#define NEAR_MARGIN 20.0F

/* what each run adds to the v of each of the steps' uvs. each of their
 * meshes has four */
#define SCROLL_STEP 50

/* which escalator the task is, in work->mode as it starts */
enum escalator_kind { ESCALATOR_1, ESCALATOR_2 };

static void escalator_destroy(struct task *task);
static void escalator_1_run(struct task *task);
static void escalator_2_run(struct task *task);
static void escalator_start(struct task *task);

static void escalator_destroy(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = MODEL(work)->object;

    world_object_free(object);
    world_remove(task, object);
    /* @hack redundant: a return where control returns anyway, which has the
     * entry reserve a frame word for task, ADD #-4,R15 at 0x0c901ce4; without
     * it, SHC pushes task after the first load, the function runs 2 bytes
     * shorter, and 13 of 16 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* each escalator's run is both its step and its draw. the game draws its
 * tasks while it is paused, so the steps scroll on then too */
static void escalator_1_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = MODEL(work)->object;
    Angle ang;
    /* @hack spelling: the angles through a pointer, which SHC keeps in R12, MOV
     * R14,R12 and ADD #20,R12 at 0x0c901d10; through work, the function runs
     * 32 bytes shorter and 69 of 120 words differ */
    struct angles *angs = &work->ang;
    NJS_MODEL *steps;
    NJS_MODEL *bound;
    NJS_COLOR *uv;
    int i;

    if (placement_try_leave_by(task, ESCALATOR_RANGE)) {
        return;
    }
    if (player_in_range(&work->pos, (object->model->r * 2.0F) + NEAR_MARGIN)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    steps = &escalator_1_steps_model;
    bound = &escalator_1_steps_model;
    uv = steps->meshsets->vertuv;
    for (i = 0; i < bound->meshsets->nbMesh * 4; i++, uv++) {
        uv->tex.v += SCROLL_STEP;
        /* @hack spelling: bound, a copy of steps stored again on each pass.
         * the loop's first test then reads the meshset through R6, MOV
         * R13,R6 at 0x0c901d8a, and work keeps R14; without bound, work and
         * steps swap R13 and R14, the function runs 2 bytes shorter, and 86
         * of 120 words differ */
        bound = steps;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, angs->z, ang);
    TURN_VIA(njRotateX, angs->x, ang);
    TURN_VIA(njRotateY, angs->y, ang);
    draw_model(&escalator_1_model);
    draw_model(steps);
    njPopMatrix(1);
}

static void escalator_2_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = MODEL(work)->object;
    Angle ang;
    /* @hack spelling: the angles through a pointer, which SHC keeps in R12, MOV
     * R14,R12 and ADD #20,R12 at 0x0c901e90; through work, the function runs
     * 32 bytes shorter and 69 of 120 words differ */
    struct angles *angs = &work->ang;
    NJS_MODEL *steps;
    NJS_MODEL *bound;
    NJS_COLOR *uv;
    int i;

    if (placement_try_leave_by(task, ESCALATOR_RANGE)) {
        return;
    }
    if (player_in_range(&work->pos, (object->model->r * 2.0F) + NEAR_MARGIN)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    steps = &escalator_2_steps_model;
    bound = &escalator_2_steps_model;
    uv = steps->meshsets->vertuv;
    for (i = 0; i < bound->meshsets->nbMesh * 4; i++, uv++) {
        uv->tex.v += SCROLL_STEP;
        /* @hack spelling: bound, a copy of steps stored again on each pass.
         * the loop's first test then reads the meshset through R6, MOV
         * R13,R6 at 0x0c901f0a, and work keeps R14; without bound, work and
         * steps swap R13 and R14, the function runs 2 bytes shorter, and 86
         * of 120 words differ */
        bound = steps;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, angs->z, ang);
    TURN_VIA(njRotateX, angs->x, ang);
    TURN_VIA(njRotateY, angs->y, ang);
    draw_model(&escalator_2_model);
    draw_model(steps);
    njPopMatrix(1);
}

static void escalator_start(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (work->mode == ESCALATOR_1) {
        task->step = escalator_1_run;
        task->draw = task->step;
        MODEL(work)->object =
            object_add_world(&escalator_1_object, task, WORLD_SOLID);
    } else {
        task->step = escalator_2_run;
        task->draw = task->step;
        MODEL(work)->object =
            object_add_world(&escalator_2_object, task, WORLD_SOLID);
    }
    object = MODEL(work)->object;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    task->destructor = escalator_destroy;
}

void escalator_1_step(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = ESCALATOR_1;
    escalator_start(task);
}

void escalator_2_step(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = ESCALATOR_2;
    escalator_start(task);
}
