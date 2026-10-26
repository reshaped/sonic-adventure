/* O JAMER road barrier whose lamps blink */
#include <ninja.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg04/barrier.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT barrier_object;
/* the barrier's textures with its lamps lit one way, then the other */
extern NJS_TEXNAME barrier_1_texnames[5];
extern NJS_TEXNAME barrier_2_texnames[5];

#define BARRIER_WORLD_FLAGS WORLD_SMALL

/* the collision world tests the barrier while a player is within BARRIER_NEAR
 * units */
#define BARRIER_NEAR 33.0F

/* the lamps switch each time the timer passes BLINK_TIMER_MOST, so every
 * BLINK_TIMER_MOST + 1 steps */
#define BLINK_TIMER_MOST 25

enum barrier_mode {
    BARRIER_INIT,
    BARRIER_STAND,
    BARRIER_GONE /* this file never sets it */
};

static void barrier_add_world(struct task *task);
static void barrier_follow(struct task_work *work);
static void barrier_blink(struct task_work *work);
static void barrier_draw(struct task *task);
static void barrier_destroy(struct task *task);

static NJS_TEXLIST barrier_texlists[2] = {
#include "stg04/barrier/barrier_texlists.inc"
};

static void barrier_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = world_object_alloc();

    WORLD_MODEL(work)->model = &barrier_object;
    WORLD_MODEL(work)->world = object;
    object->evalflags = WORLD_MODEL(work)->model->evalflags &
                        ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
    object->scl[0] = object->scl[1] = object->scl[2] = 1.0F;
    object->ang[0] = work->ang.x;
    object->ang[1] = work->ang.y;
    object->ang[2] = work->ang.z;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = WORLD_MODEL(work)->model->model;
    object->child = WORLD_MODEL(work)->model->child;
    object->sibling = WORLD_MODEL(work)->model->sibling;
    world_add(BARRIER_WORLD_FLAGS, task, object);
}

static void barrier_follow(struct task_work *work)
{
    NJS_OBJECT *object = WORLD_MODEL(work)->world;

    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->ang[0] = work->ang.x;
    object->ang[1] = work->ang.y;
    object->ang[2] = work->ang.z;
    if (player_in_range(&work->pos, BARRIER_NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void barrier_blink(struct task_work *work)
{
    if (++TASK_TIMER(work) > BLINK_TIMER_MOST) {
        work->timer = 0;
        switch (work->submode) {
        case 0:
            work->submode++;
            WORLD_MODEL(work)->texlist = &barrier_texlists[1];
            break;
        case 1:
        default:
            work->submode--;
            WORLD_MODEL(work)->texlist = &barrier_texlists[0];
            break;
        }
    }
}

static void barrier_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(WORLD_MODEL(work)->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSimpleDrawModel(WORLD_MODEL(work)->world->model);
    njPopMatrix(1);
}

/* unless the set editor holds the barrier, remove its copy from the world and
 * free it. task_kill changes nothing here: task_destroy clears the step once a
 * destructor returns */
static void barrier_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && WORLD_MODEL(work)->world) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
    task_kill(task);
}

/* barrier_draw draws the model of WORLD_MODEL(work)->world. that is
 * barrier_object for a barrier that first steps with the set editor open, else
 * barrier_add_world's copy */
void barrier_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *model;
    NJS_TEXLIST *texlists;

    if (placement_try_leave(task)) {
        return;
    }
    /* @hack spelling: both addresses taken into locals ahead of the editor
     * test, which loads them before it (0x0c908c20, 0x0c908c24); without it,
     * the editor's branch loads barrier_object only where it stores it, 10 of
     * the pool's words differ too, and 54 of 64 words differ */
    texlists = barrier_texlists;
    model = &barrier_object;
    if (edit_mode) {
        if (!WORLD_MODEL(work)->world) {
            WORLD_MODEL(work)->world = model;
        }
        if (!WORLD_MODEL(work)->texlist) {
            WORLD_MODEL(work)->texlist = texlists;
        }
        barrier_blink(work);
        barrier_draw(task);
    } else {
        switch (work->mode) {
        case BARRIER_INIT:
            work->mode = BARRIER_STAND;
            WORLD_MODEL(work)->world = model;
            WORLD_MODEL(work)->texlist = texlists;
            barrier_add_world(task);
            task->destructor = barrier_destroy;
            task->draw = barrier_draw;
            break;
        case BARRIER_STAND:
            barrier_follow(work);
            barrier_blink(work);
            barrier_draw(task);
            break;
        case BARRIER_GONE:
            break;
        }
    }
}
