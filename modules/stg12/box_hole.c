/* O Box Ana, a hole with places for four box switches. once all four boxes
 * have sunk into their places, it turns its switch on */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "object/switch.h"
#include "stg12/box_hole.h"
#include "stg12/box_switch.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* @hack type: the action declared as an array, so SHC pools only its address,
 * at 0x0c9089bc, and box_hole_animate reads its motion through it at
 * 0x0c90891a. declared as one, SHC pools the motion field's address too. then 3
 * of box_hole_animate's 15 words differ, as do 3 of
 * box_hole_set_switch_when_filled's 59 words and 6 of the 34 words of its pool
 * and pad */
extern NJS_ACTION box_hole_action[];
extern NJS_OBJECT box_hole_world_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* each hole's switch */
#define HOLE_0_SWITCH 4
#define HOLE_1_SWITCH 17

enum box_hole_mode { HOLE_INIT, HOLE_RUN };

struct box_hole_vars {
    Uint8 hole; /* which hole, 0 or 1, as scale x gives it */
    Uint8 frame; /* the action's frame to draw */
    Uint8 unk_02[6];
    NJS_OBJECT *world; /* the hole's object in the collision world */
};

#define VARS(work) ((struct box_hole_vars *)&(work)->vars)

static void box_hole_init(struct task *task);
static void box_hole_update(struct task *task);
static void box_hole_draw(struct task *task);
static void box_hole_destroy(struct task *task);
static void box_hole_animate(struct task *task);
static void box_hole_set_switch_when_filled(struct task *task);

void box_hole_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case HOLE_INIT:
            box_hole_init(task);
            break;
        case HOLE_RUN:
            box_hole_draw(task);
            box_hole_update(task);
            box_hole_animate(task);
            box_hole_set_switch_when_filled(task);
            break;
        }
    } else {
        switch (work->mode) {
        case HOLE_INIT:
            box_hole_init(task);
            break;
        case HOLE_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> ANA ID (0-1)");
            box_hole_draw(task);
            box_hole_update(task);
            break;
        }
    }
}

static void box_hole_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;

    task->draw = box_hole_draw;
    task->destructor = box_hole_destroy;
    VARS(work)->hole = (int)work->scl.x;
    VARS(work)->world = world = world_object_alloc();
    world->evalflags = box_hole_world_object.evalflags;
    world->scl[0] = 1.0F;
    world->scl[1] = 1.0F;
    world->scl[2] = 1.0F;
    world->ang[0] = 0;
    world->ang[1] = work->ang.y;
    world->ang[2] = 0;
    world->pos[0] = work->pos.x + box_hole_world_object.pos[0];
    world->pos[1] = work->pos.y + box_hole_world_object.pos[1];
    world->pos[2] = work->pos.z + box_hole_world_object.pos[2];
    world->model = box_hole_world_object.model;
    world->child = NULL;
    world->sibling = NULL;
    world_add(WORLD_LARGE, task, world);
    work->ang.x = 0;
    work->ang.z = 0;
    VARS(work)->frame = 0;
    work->mode = HOLE_RUN;
}

static void box_hole_update(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world = VARS(work)->world;
    NJS_OBJECT *from = &box_hole_world_object;
    /* @hack redundant: 24 bytes of frame that no code reads, of a type unknown,
     * declared as two points. the image's ADD #-24,R15 reserves them
     * (0x0c908860); without them, 31 of 32 words differ */
    NJS_POINT3 unused[2];

    world->ang[0] = 0;
    world->ang[1] = work->ang.y;
    world->ang[2] = 0;
    world->pos[0] = work->pos.x + from->pos[0];
    world->pos[1] = work->pos.y + from->pos[1];
    world->pos[2] = work->pos.z + from->pos[2];
}

static void box_hole_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter2_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(box_hole_action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

static void box_hole_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->world);
    world_object_free(VARS(work)->world);
}

static void box_hole_animate(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame++;
    if (VARS(work)->frame >= box_hole_action->motion->nbFrame) {
        VARS(work)->frame = 0;
    }
}

static void box_hole_set_switch_when_filled(struct task *task)
{
    struct task_work *work = task->work;
    Bool on;

    switch (VARS(work)->hole) {
    case 0:
        on = box_switch_is_on(0);
        on &= box_switch_is_on(1);
        on &= box_switch_is_on(2);
        on &= box_switch_is_on(3);
        if (on) {
            switch_set(HOLE_0_SWITCH, TRUE);
        }
        break;
    case 1:
        on = box_switch_is_on(4);
        on &= box_switch_is_on(5);
        on &= box_switch_is_on(6);
        on &= box_switch_is_on(7);
        if (on) {
            switch_set(HOLE_1_SWITCH, TRUE);
        }
        break;
    }
}
