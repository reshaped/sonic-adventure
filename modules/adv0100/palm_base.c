/* O PALMBASE palm planter where Knuckles digs up a switch */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/palm_base.h"
#include "collision/world.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "object/switch.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT palm_base_object;

struct palm_base_vars {
    NJS_OBJECT *ground; /* the ground's object in the collision world */
};

#define VARS(work) ((struct palm_base_vars *)&(work)->vars)
#define DUG(work) ((work)->submode) /* TRUE once the switch is dug up */

/* the ground's flags in the collision world: diggable, but not solid */
#define GROUND_FLAGS (WORLD_MEDIUM | WORLD_DIGGABLE)
/* how far above the planter the ground and the dug-up switch stand */
#define GROUND_RISE 5.0F
#define SWITCH_RISE 2.0F
/* a player's distance within which the collision world tests the ground */
#define TEST_RANGE 40.0F

#define SWITCH_KIND 0.0F /* the dug-up switch's scl.x: a toggle */
/* the dug-up switch's scl.y, the id it asks for. the switch clamps it to 31,
 * its last id */
#define SWITCH_ID 36.0F

enum palm_base_mode { PALM_BASE_INIT, PALM_BASE_RUN };

static void palm_base_destroy(struct task *task);
static void palm_base_draw(struct task *task);
static void palm_base_draw_work(struct task_work *work);
static void palm_base_dig(struct task *task);

/* the ground, as one strip. the model gives a point count of 3 for its four
 * points */
static Sint16 palm_base_ground_strip[5] = {
#include "adv0100/palm_base/palm_base_ground_strip.inc"
};
static NJS_MESHSET palm_base_ground_meshsets[1] = {
#include "adv0100/palm_base/palm_base_ground_meshsets.inc"
};
static NJS_POINT3 palm_base_ground_points[4] = {
#include "adv0100/palm_base/palm_base_ground_points.inc"
};
static NJS_MODEL palm_base_ground_model = {
#include "adv0100/palm_base/palm_base_ground_model.inc"
};

/* add the ground to the collision world, GROUND_RISE above the planter. the
 * first time player 0's action is KNUCKLES_DIGGING_UP, wherever Knuckles is
 * digging, the planter creates the switch. the collision world tests the ground
 * only while a player is within TEST_RANGE */
void egg_carrier_palm_base_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *ground;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PALM_BASE_INIT:
        VARS(work)->ground = ground = world_object_alloc();
        ground->evalflags =
            NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK;
        ground->scl[0] = 1.0F;
        ground->scl[1] = 1.0F;
        ground->scl[2] = 1.0F;
        ground->ang[0] = 0;
        ground->ang[1] = work->ang.y;
        ground->ang[2] = 0;
        ground->pos[0] = work->pos.x;
        ground->pos[1] = work->pos.y + GROUND_RISE;
        ground->pos[2] = work->pos.z;
        ground->model = &palm_base_ground_model;
        ground->child = NULL;
        ground->sibling = NULL;
        world_add(GROUND_FLAGS, task, ground);
        task->destructor = palm_base_destroy;
        task->draw = palm_base_draw;
        work->mode = PALM_BASE_RUN;
        break;
    case PALM_BASE_RUN:
        if (!DUG(work) && knuckles_get_activity(0) == KNUCKLES_DIGGING_UP) {
            palm_base_dig(task);
            DUG(work) = TRUE;
        }
        break;
    }
    work->flags &= ~WORLD_TESTED;
    if (player_in_range(&work->pos, TEST_RANGE) != 0) {
        work->flags |= WORLD_TESTED;
    }
    task_step_children(task);
    palm_base_draw_work(work);
}

static void palm_base_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->ground != NULL) {
        world_remove(task, VARS(work)->ground);
        world_object_free(VARS(work)->ground);
    }
    task_destroy_children(task);
}

static void palm_base_draw(struct task *task)
{
    palm_base_draw_work(task->work);
}

static void palm_base_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&palm_base_object);
    njPopMatrix(1);
}

/* create the dug-up switch as the planter's child task, SWITCH_RISE above the
 * planter */
static void palm_base_dig(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child =
        task_create_child(TASK_WORK | TASK_FORCE, switch_step, task);

    if (child != NULL) {
        struct task_work *switch_work = child->work;

        switch_work->pos.x = work->pos.x;
        switch_work->pos.y = work->pos.y + SWITCH_RISE;
        switch_work->pos.z = work->pos.z;
        switch_work->scl.x = SWITCH_KIND;
        switch_work->scl.y = SWITCH_ID;
    }
}
