/* the workshop's runway, its six trees and its four panels, which event flags
 * move out */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/runway.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "progress/progress.h"
#include "system/task.h"

/* the trees' action's object, whose children are the trees */
/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address at 0x0c90bf60 and reads child through it, MOV.L @(44,R6),R6 at
 * 0x0c90bed6; declared as one, it pools child's address, and 22 of
 * runway_init's 85 words and 1 pool word differ */
extern NJS_OBJECT runway_trees_object[];
/* @hack type: each action declared an array, not the one NJS_ACTION it is, so
 * SHC pools its address and reads motion through it twice. declared as one, it
 * pools motion's address and reads motion once, and runway_update runs 2 bytes
 * shorter. for the panels' action, pooled at 0x0c90c0f8 and read through by
 * MOV.L @(4,R5),R3 at 0x0c90c082 and MOV.L @(4,R5),R2 at 0x0c90c09e, 33 of
 * runway_update's 161 words differ */
extern NJS_ACTION runway_panels_action[];
/* @hack type: as runway_panels_action: pooled at 0x0c90c0f4 and read through by
 * MOV.L @(4,R5),R3 at 0x0c90c034 and MOV.L @(4,R5),R2 at 0x0c90c050. declared
 * as one, 69 of runway_update's 161 words differ */
extern NJS_ACTION runway_trees_action[];
/* @hack type: as runway_panels_action: pooled at 0x0c90c0ec and read through by
 * MOV.L @(4,R5),R3 at 0x0c90bff6 and MOV.L @(4,R5),R2 at 0x0c90c012. declared
 * as one, 97 of runway_update's 161 words differ */
extern NJS_ACTION runway_action[];

#define TREES 6

/* the fixed positions and y angles. the trees share the runway's position */
#define RUNWAY_X 1487.23F
#define RUNWAY_Y 172.26F
#define RUNWAY_Z 802.42F
#define RUNWAY_ANG 0x1ae8
#define TREES_ANG 0x1a7b
#define PANELS_X 1421.98F
#define PANELS_Y 191.8F
#define PANELS_Z 734.89F
#define PANELS_ANG 0x5b43

/* the action frames each action plays each step */
#define FRAME_SPEED 0.3F

#define RADIUS 1000.0F /* the collision record's radius */

enum runway_mode { RUNWAY_INIT, RUNWAY_RUN };

/* work->submode is TRUE while the runway is out, and work->tick while the
 * panels are out */
#define RUNWAY_OUT(work) ((work)->submode)
#define PANELS_OUT(work) ((work)->tick)

/* the frame each of the three actions is at */
struct runway_vars {
    Float panels;
    Float runway;
    Float trees;
};

#define VARS(work) ((struct runway_vars *)&(work)->vars)

static void runway_init(struct task *task);
static void runway_reset(struct task *task);
static void runway_update(struct task *task);
static void runway_draw(struct task *task);
static void runway_destroy(struct task *task);

/* a box along the runway, solid only while the runway is out, and a sphere at
 * each tree, placed by init. all push and stand firm against every group */
static struct collision_shape runway_shapes[1 + TREES] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-4.1614F, 21.3801F, 488.3825F},
     35.68F,
     7.5F,
     489.62F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     20.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     20.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     20.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     20.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     20.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     20.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* in the set editor, put the runway and the panels back in each step */
void runway_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case RUNWAY_INIT:
            runway_init(task);
            break;
        case RUNWAY_RUN:
            runway_draw(task);
            runway_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case RUNWAY_INIT:
            runway_init(task);
            break;
        case RUNWAY_RUN:
            runway_reset(task);
            runway_draw(task);
            runway_update(task);
            break;
        }
    }
}

static void runway_init(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape;
    NJS_OBJECT *tree;
    int i;

    work->pos.x = RUNWAY_X;
    work->pos.y = RUNWAY_Y;
    work->pos.z = RUNWAY_Z;
    work->ang.y = RUNWAY_ANG;
    task->draw = runway_draw;
    task->destructor = runway_destroy;
    VARS(work)->panels = 0.0F;
    VARS(work)->runway = 0.0F;
    VARS(work)->trees = 0.0F;
    collision_init(task, runway_shapes, COLLISION_OBJECT, 1, 1 + TREES);
    shape = work->collision->shapes + 1;
    tree = runway_trees_object->child;
    for (i = 0; i < TREES; i++) {
        /* @hack type: tree->pos, a Float[3], read whole as an NJS_POINT3;
         * without it, as three element reads, the function runs 18 bytes long
         * and 51 of 85 words differ */
        shape->center = *(NJS_POINT3 *)tree->pos;
        shape++;
        tree = tree->sibling;
    }
    work->collision->radius = RADIUS;
    runway_reset(task);
    work->mode = RUNWAY_RUN;
}

/* put the runway in, with its box not solid, and the panels in */
static void runway_reset(struct task *task)
{
    struct task_work *work = task->work;

    work->collision->shapes->flags |= SHAPE_OFF;
    RUNWAY_OUT(work) = FALSE;
    PANELS_OUT(work) = FALSE;
}

/* while FLAG_RUNWAY is set, the runway's and the trees' actions play to their
 * last frames, and the runway's box is solid. while FLAG_RUNWAY_PANELS is set,
 * the panels' action plays the same way. each action goes back to its first
 * frame once its flag clears */
static void runway_update(struct task *task)
{
    struct task_work *work = task->work;

    if (event_flags[FLAG_RUNWAY]) {
        if (!RUNWAY_OUT(work)) {
            work->collision->shapes->flags &= ~SHAPE_OFF;
            RUNWAY_OUT(work) = TRUE;
        }
    } else {
        if (RUNWAY_OUT(work)) {
            work->collision->shapes->flags |= SHAPE_OFF;
            RUNWAY_OUT(work) = FALSE;
        }
    }
    if (event_flags[FLAG_RUNWAY_PANELS]) {
        if (!PANELS_OUT(work)) {
            PANELS_OUT(work) = TRUE;
        }
    } else {
        if (PANELS_OUT(work)) {
            PANELS_OUT(work) = FALSE;
        }
    }
    if (RUNWAY_OUT(work)) {
        VARS(work)->runway += FRAME_SPEED;
        if (VARS(work)->runway >= runway_action->motion->nbFrame - 1.0F) {
            VARS(work)->runway = runway_action->motion->nbFrame - 1.0F;
        }
        VARS(work)->trees += FRAME_SPEED;
        if (VARS(work)->trees >= runway_trees_action->motion->nbFrame - 1.0F) {
            VARS(work)->trees = runway_trees_action->motion->nbFrame - 1.0F;
        }
    } else {
        VARS(work)->runway = 0.0F;
        VARS(work)->trees = 0.0F;
    }
    if (PANELS_OUT(work)) {
        VARS(work)->panels += FRAME_SPEED;
        if (VARS(work)->panels >=
            runway_panels_action->motion->nbFrame - 1.0F) {
            VARS(work)->panels = runway_panels_action->motion->nbFrame - 1.0F;
        }
    } else {
        VARS(work)->panels = 0.0F;
    }
    collision_register(work);
}

static void runway_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslate(NULL, PANELS_X, PANELS_Y, PANELS_Z);
    njRotateY(NULL, PANELS_ANG);
    njSetTexture(&mrobj_texlist);
    draw_action(runway_panels_action, VARS(work)->panels);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, RUNWAY_X, RUNWAY_Y, RUNWAY_Z);
    njRotateY(NULL, RUNWAY_ANG);
    njSetTexture(&mrobj_texlist);
    draw_action(runway_action, VARS(work)->runway);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, RUNWAY_X, RUNWAY_Y, RUNWAY_Z);
    njRotateY(NULL, TREES_ANG);
    njSetTexture(&mrobj_texlist);
    draw_action(runway_trees_action, VARS(work)->trees);
    njPopMatrix(1);
}

static void runway_destroy(struct task *task)
{
}
