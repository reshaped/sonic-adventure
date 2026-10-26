/* O HAGO, a base with two rails. an arm on it rocks about x, 15 degrees each
 * way */
#include <ninja.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg05/rocker.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_MODEL rocker_arm_model;
extern NJS_OBJECT rocker_arm_object;
extern NJS_MODEL rocker_base_model;
extern NJS_OBJECT rocker_world_object;

/* the arm's tilt about x, in angle units, as a float. it takes the place of
 * the set entry's scale x, which the rocker never reads */
#define TILT(work) ((work)->scl.x)

enum rocker_mode { ROCKER_UP, ROCKER_DOWN };

#define ROCKER_STEP 80.0F /* the tilt's change each step */
#define ROCKER_BOUND 2730.5F /* the tilt's bound each way: 15 degrees */

/* the world tests the arm's copy only while a player is within this */
#define ROCKER_REACH 68.0F
#define DRAW_RADIUS 500.0F

/* the rails' boxes, which run along z from the rocker's place. they turn
 * with its y angle, not with the arm */
static struct collision_shape rocker_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {22.6F, 0.0F, 57.0F},
     1.5F,
     2.8F,
     57.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-22.6F, 0.0F, 57.0F},
     1.5F,
     2.8F,
     57.0F,
     {0, 0, 0},
     0,
     NULL},
};

static void rocker_run(struct task *task);
static void rocker_draw(struct task *task);
static void rocker_destroy(struct task *task);

/* start the arm level. unless the set editor holds the rocker, put a copy of
 * rocker_world_object in the world. the rails then get their boxes. the copy
 * stands at the rocker's place plus that object's position */
void rocker_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;

    work->ang.x = 0;
    work->ang.z = 0;
    TILT(work) = 0.0F;
    work->mode = ROCKER_UP;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (!placement_editing(task)) {
        WORLD_MODEL(work)->model = &rocker_world_object;
        world = world_object_alloc();
        WORLD_MODEL(work)->world = world;
        world->evalflags =
            NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
        world->scl[0] = WORLD_MODEL(work)->model->scl[0];
        world->scl[1] = WORLD_MODEL(work)->model->scl[1];
        world->scl[2] = WORLD_MODEL(work)->model->scl[2];
        world->ang[0] = 0;
        world->ang[1] = work->ang.y;
        world->ang[2] = 0;
        world->pos[0] = work->pos.x + WORLD_MODEL(work)->model->pos[0];
        world->pos[1] = work->pos.y + WORLD_MODEL(work)->model->pos[1];
        world->pos[2] = work->pos.z + WORLD_MODEL(work)->model->pos[2];
        world->model = WORLD_MODEL(work)->model->model;
        world->child = NULL;
        world->sibling = NULL;
        world_add(WORLD_MOVES, task, world);
        collision_init(task, rocker_shapes, COLLISION_OBJECT, 1, 2);
    }
    task->step = rocker_run;
    task->draw = rocker_draw;
    task->destructor = rocker_destroy;
}

/* rock the arm between -ROCKER_BOUND and ROCKER_BOUND, ROCKER_STEP a step.
 * unless the set editor holds the rocker, its copy in the world tilts with
 * it */
static void rocker_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case ROCKER_UP:
        TILT(work) += ROCKER_STEP;
        if (TILT(work) >= ROCKER_BOUND) {
            TILT(work) = ROCKER_BOUND;
            work->mode = ROCKER_DOWN;
        }
        break;
    case ROCKER_DOWN:
        TILT(work) -= ROCKER_STEP;
        if (TILT(work) <= -ROCKER_BOUND) {
            TILT(work) = -ROCKER_BOUND;
            work->mode = ROCKER_UP;
        }
        break;
    }
    if (!placement_editing(task)) {
        if (player_in_range(&work->pos, ROCKER_REACH)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
        WORLD_MODEL(work)->world->ang[0] = (Angle)TILT(work);
        WORLD_MODEL(work)->world->ang[1] = work->ang.y;
        collision_register(work);
    }
    rocker_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "->");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "->");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "->");
    }
}

static void rocker_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(&rocker_base_model);
        njTranslate(NULL, rocker_arm_object.pos[0], rocker_arm_object.pos[1],
                    rocker_arm_object.pos[2]);
        TURN_VIA(njRotateX, (Angle)TILT(work), ang);
        draw_model(&rocker_arm_model);
        njPopMatrix(1);
    }
}

static void rocker_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (WORLD_MODEL(work)->world != NULL) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
}
