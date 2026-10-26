/* group objects that each place a row of one Speed Highway object */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg04/corner_light.h"
#include "stg04/fence.h"
#include "stg04/fence_2.h"
#include "stg04/gate.h"
#include "stg04/green.h"
#include "stg04/group.h"
#include "stg04/pinpin.h"
#include "stg04/raft.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT highway_raft_a_object;
extern NJS_OBJECT curb_object;

/* a group's set entry scale. x is the index of the row's last object, and y
 * the spacing between objects. z is each object's heading, its y angle in
 * degrees. the group's own angles turn the row's line but not its objects */
#define GROUP_LAST(work) ((int)(work)->scl.x)
#define GROUP_SPACING(work) ((work)->scl.y)
#define GROUP_HEADING(work) ((work)->scl.z)

/* the ranges for placement_try_leave_by, squared, of a group and of a curb
 * or raft of its own */
#define GROUP_RANGE (1700.0F * 1700.0F)
#define PART_RANGE (1500.0F * 1500.0F)

/* reach of a curb's collision record from its position */
#define CURB_REACH 10.0F

/* the collision world tests a raft's copy while a player is within its model's
 * radius plus NEAR_MARGIN */
#define NEAR_MARGIN 20.0F

/* a raft's copy is a medium object, without WORLD_SOLID */
#define RAFT_WORLD_FLAGS WORLD_MEDIUM

enum group_mode {
    GROUP_PLACE = 0,
    GROUP_RUN = 1,
    GROUP_GONE = 3, /* an object of the row has left range */
    GROUP_KILL = 4
};

enum part_mode {
    PART_INIT = 0,
    PART_STAND = 1,
    PART_STOP = 4 /* this file never sets it */
};

struct group_vars {
    Uint8 unk_00[4];
    void (*step)(struct task *task); /* the step of each object it places */
    Sint32 create; /* task_create's flags for each object */
};

#define VARS(work) ((struct group_vars *)&(work)->vars)

static void group_step(struct task *task);
static void group_draw_marker(NJS_POINT3 *pos, struct angles *ang);
static void group_destroy(struct task *task);
static Uint8 group_in_range(struct task *task);
static void group_raft_draw(struct task *task);
static void group_raft_step(struct task *task);
static void group_curb_draw(struct task *task);
static void group_curb_step(struct task *task);
static void group_raft_add_world(struct task *task);
static void group_raft_follow(struct task *task);
static void group_raft_destroy(struct task *task);

/* a curb of the group's own takes the second box. this file never uses the
 * first */
static struct collision_shape group_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     10.0F,
     2.0F,
     17.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 2.5F, 0.0F},
     6.0F,
     2.5F,
     2.2F,
     {0},
     0,
     NULL},
};

void fence_group_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->step = fence_ff_step;
    VARS(work)->create = TASK_WORK;
    task->destructor = group_destroy;
    task->step = group_step;
}

void curb_group_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->step = group_curb_step;
    VARS(work)->create = TASK_WORK;
    task->destructor = group_destroy;
    task->step = group_step;
}

void fence_2_group_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->step = fence_2_step;
    VARS(work)->create = TASK_WORK;
    task->destructor = group_destroy;
    task->step = group_step;
}

void pinpin_group_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->step = pinpin_step;
    VARS(work)->create = TASK_WORK;
    task->destructor = group_destroy;
    task->step = group_step;
}

void fence_ff_group_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->step = fence_step;
    VARS(work)->create = TASK_WORK;
    task->destructor = group_destroy;
    task->step = group_step;
}

void raft_a_group_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->step = group_raft_step;
    VARS(work)->create = TASK_WORK | TASK_FORCE;
    task->destructor = group_destroy;
    task->step = group_step;
}

void raft_c_group_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->step = highway_raft_c_step;
    VARS(work)->create = TASK_WORK | TASK_FORCE;
    task->destructor = group_destroy;
    task->step = group_step;
}

void green_a_group_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->step = green_a_step;
    VARS(work)->create = TASK_WORK;
    task->destructor = group_destroy;
    task->step = group_step;
}

void green_b_group_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->step = green_b_step;
    VARS(work)->create = TASK_WORK;
    task->destructor = group_destroy;
    task->step = group_step;
}

void green_d_group_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->step = green_d_step;
    VARS(work)->create = TASK_WORK;
    task->destructor = group_destroy;
    task->step = group_step;
}

void corner_light_group_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->step = corner_light_step;
    VARS(work)->create = TASK_WORK;
    task->destructor = group_destroy;
    task->step = group_step;
}

/* place the row along the group's z axis. the first object stands at the
 * group's position. the rest alternate sides, at multiples of the spacing.
 * while the set editor holds the group, it marks each place and labels its
 * scale. otherwise it creates the row once and steps it. once any object of
 * the row has left range, the group kills itself */
static void group_step(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a point. the image's ADD #-104,R15 reserves them
     * (0x0c910fc8); without them, 2 of 409 words differ */
    NJS_POINT3 unused;
    struct angles ang = {0, 0, 0};
    NJS_VECTOR offset = {0.0F, 0.0F, 0.0F};
    NJS_POINT3 pos;
    int i;

    if (placement_try_leave_by(task, GROUP_RANGE)) {
        return;
    }
    switch (work->mode) {
    case GROUP_PLACE:
        for (i = 0; i < GROUP_LAST(work) + 1; i++) {
            if (i % 2) {
                offset.z = GROUP_SPACING(work) * ceilf(i * 0.5F);
            } else {
                offset.z = -(GROUP_SPACING(work) * i * 0.5F);
            }
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, work->ang.y);
            njCalcVector(NULL, &offset, &pos);
            njAddVector(&pos, &work->pos);
            njPopMatrix(1);
            ang.y = (Angle)(GROUP_HEADING(work) * ANGLE_PER_DEGREE);
            if (placement_editing(task)) {
                group_draw_marker(&pos, &ang);
            }
            if (!placement_editing(task)) {
                struct task *one;

                /* @hack spelling: the child tested as it is assigned. SHC then
                 * keeps ceilf in R9 and njCalcVector in R10, at 0x0c911060 and
                 * 0x0c91111a; assigned ahead of the test, SHC loads
                 * njCalcVector from the pool at each call, the function runs
                 * 32 bytes longer, and 278 of 409 words differ */
                if ((one = task_create_child(VARS(work)->create,
                                             VARS(work)->step, task)) != NULL) {
                    one->work->pos.x = pos.x;
                    one->work->pos.y = pos.y;
                    one->work->pos.z = pos.z;
                    one->work->ang.x = ang.x;
                    one->work->ang.y = ang.y;
                    one->work->ang.z = ang.z;
                }
            }
        }
        if (!placement_editing(task)) {
            work->mode = GROUP_RUN;
        }
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- NUM");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- DISTANCE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<-OFFSET ANGY");
        }
        break;
    case GROUP_RUN:
        if (!placement_editing(task)) {
            task_step_children(task);
            if (!group_in_range(task)) {
                work->mode = GROUP_GONE;
            }
        }
        if (placement_editing(task)) {
            /* @hack redundant: 12 bytes more of frame that no code reads, of a
             * type unknown, declared as a point. they move the locals above
             * them, as MOV #76,R0 at 0x0c9110d0 shows; without them, 17 of 409
             * words differ */
            NJS_POINT3 unused_preview;
            struct angles preview_ang = {0, 0, 0};
            NJS_VECTOR preview_offset = {0.0F, 0.0F, 0.0F};
            NJS_POINT3 preview;

            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- NUM");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- DISTANCE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<-OFFSET ANGY");
            for (i = 0; i < GROUP_LAST(work) + 1; i++) {
                if (i % 2) {
                    preview_offset.z = GROUP_SPACING(work) * ceilf(i * 0.5F);
                } else {
                    preview_offset.z = -(GROUP_SPACING(work) * i * 0.5F);
                }
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN(njRotateZ, work->ang.z);
                TURN(njRotateX, work->ang.x);
                TURN(njRotateY, work->ang.y);
                njCalcVector(NULL, &preview_offset, &preview);
                njAddVector(&preview, &work->pos);
                njPopMatrix(1);
                preview_ang.y = (Angle)(GROUP_HEADING(work) * ANGLE_PER_DEGREE);
                group_draw_marker(&preview, &preview_ang);
            }
        }
        break;
    case GROUP_GONE:
        work->mode = GROUP_KILL;
        break;
    case GROUP_KILL:
        task_kill(task);
        break;
    }
}

static void group_draw_marker(NJS_POINT3 *pos, struct angles *ang)
{
    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y, pos->z);
    TURN(njRotateZ, ang->z);
    TURN(njRotateX, ang->x);
    TURN(njRotateY, ang->y);
    draw_tree(&marker_box_object);
    njPopMatrix(1);
}

static void group_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* 1 while every object of the row is in range. 0 once one has left range and
 * destroys itself on its next step */
/* @hack type: the result as a Uint8, which group_step zero-extends before its
 * test, EXTU.B R0,R0 at 0x0c9111fa; as a Bool, group_step runs 2 bytes
 * shorter and 68 of 409 words differ */
static Uint8 group_in_range(struct task *task)
{
    struct task *one;

    for (one = task->child; one != NULL; one = one->next) {
        if (one->step == task_destroy) {
            return 0;
        }
    }
    return 1;
}

static void group_raft_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_tree(&highway_raft_a_object);
    njPopMatrix(1);
}

/* step a raft of O GRAFTA's row, drawn as HIGH RAFT A's model, with a copy in
 * the collision world */
static void group_raft_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, PART_RANGE)) {
        return;
    }
    switch (work->mode) {
    case PART_INIT:
        task->draw = group_raft_draw;
        task->destructor = group_raft_destroy;
        group_raft_add_world(task);
        work->mode = PART_STAND;
        break;
    case PART_STAND:
        group_raft_draw(task);
        group_raft_follow(task);
        break;
    case PART_STOP:
        break;
    }
}

static void group_curb_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_tree(&curb_object);
    njPopMatrix(1);
}

/* step a solid curb of O GCURB's row */
static void group_curb_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, PART_RANGE)) {
        return;
    }
    switch (work->mode) {
    case PART_INIT:
        task->draw = group_curb_draw;
        collision_init(task, &group_shapes[1], COLLISION_OBJECT, 1, 1);
        work->collision->radius = CURB_REACH;
        work->mode = PART_STAND;
        break;
    case PART_STAND:
        group_curb_draw(task);
        collision_register(work);
        break;
    case PART_STOP:
        break;
    }
}

/* add an unscaled copy of the raft's model to the collision world, turned about
 * x and z but not y until group_raft_follow first updates it */
static void group_raft_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    WORLD_MODEL(work)->world = object = world_object_alloc();
    object->evalflags = NJD_EVAL_BREAK;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = work->ang.x;
    object->ang[1] = 0;
    object->ang[2] = work->ang.z;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    /* @hack spelling: the model reached through the raft's address, which loads
     * that address from the pool and the model at @(4,R3), at 0x0c911674 and
     * 0x0c911692; as highway_raft_a_object.model, the pool holds the field's
     * own address, 1 of the pool's words differs too, and 10 of 49 words
     * differ */
    object->model = (&highway_raft_a_object)->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(RAFT_WORLD_FLAGS, task, object);
    /* @hack redundant: a return where control returns anyway, which has the
     * entry reserve a frame word for task, ADD #-4,R15 at 0x0c91164a; without
     * it, SHC pushes task before the call, the function runs 2 bytes shorter,
     * and 47 of 49 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

/* keep the copy at the raft's position and angles */
static void group_raft_follow(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = WORLD_MODEL(work)->world;

    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->ang[0] = work->ang.x;
    object->ang[1] = work->ang.y;
    object->ang[2] = work->ang.z;
    if (player_in_range(&work->pos, object->model->r + NEAR_MARGIN)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void group_raft_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, WORLD_MODEL(work)->world);
    world_object_free(WORLD_MODEL(work)->world);
}
