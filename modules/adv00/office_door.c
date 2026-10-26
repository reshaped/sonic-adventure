/* O OFFICE DOOR pair of office doors, which open only for Knuckles, once he
 * has knocked the barricade down */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/office_door.h"
#include "adv00/station_square.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the frame; its children are the two panels beside the doors and the two
 * doors, each with its window */
extern NJS_OBJECT office_door_object;

/* the distance within which a player opens the shut or closing doors */
#define DOOR_REACH 20.0F
/* the share of their full swing the doors open or close each step */
#define DOOR_SPEED 0.01F
/* the doors' full swing */
#define DOOR_TURN NJM_DEG_ANG(75)
/* open doors close once they have waited more than DOOR_WAIT steps. doors the
 * barricade has broken in swing open after the same wait */
#define DOOR_WAIT 120

/* each door hangs on a hinge DOOR_HINGE to the side of the frame's center.
 * its shape's center lies DOOR_HALF from the hinge */
#define DOOR_HINGE 20.0F
#define DOOR_HALF 10.0F

enum office_door_mode {
    DOORS_SHUT = 0,
    DOORS_OPENING = 1,
    DOORS_CLOSING = 2,
    DOORS_OPEN = 3,
    DOORS_LOCKED = 4,
    DOORS_BROKEN_IN = 5, /* the barricade has landed: wait, then swing open */
    DOORS_SWINGING = 6,
    DOORS_LEFT_OPEN = 7
};

/* work->timer counts the steps the doors have waited in DOORS_OPEN or
 * DOORS_BROKEN_IN */
struct office_door_vars {
    Float open; /* the share of their full swing the doors stand open */
};

#define VARS(work) ((struct office_door_vars *)&(work)->vars)

static void office_door_step(struct task *task);
static void office_door_draw(struct task *task);

/* the two doors, the two panels beside them, a wall across all four, and the
 * two jambs. the wall stands in for the doors and panels while the doors are
 * shut */
/* clang-format off */
static struct collision_shape office_door_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {10.0F, 20.5F, 0.0F},
     10.0F, 16.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-10.0F, 20.5F, 0.0F},
     10.0F, 16.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {30.0F, 20.5F, 0.0F},
     10.0F, 16.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-30.0F, 20.5F, 0.0F},
     10.0F, 16.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 20.5F, 0.0F},
     40.0F, 16.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {40.0F, 20.5F, 0.0F},
     5.0F, 16.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-40.0F, 20.5F, 0.0F},
     5.0F, 16.5F, 1.0F, {0}, 0, NULL},
};
/* clang-format on */

/* TRUE for Knuckles once FLAG_KNUCKLES_SS_BARRICADE is set, and FALSE for every
 * other character */
static Bool office_door_unlocked(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_BARRICADE];
    case CHARACTER_SONIC:
    case CHARACTER_TAILS:
    case CHARACTER_AMY:
    case CHARACTER_E102:
    case CHARACTER_BIG:
        return FALSE;
    }
    return FALSE;
}

/* switch the shapes by the mode. while the doors are shut, locked or not,
 * the wall and the jambs are on. otherwise the wall is off and the rest are
 * on, each door's shape turned with its door. the shapes are then registered
 * for this step's checks */
static void office_door_collide(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;

    switch (work->mode) {
    case DOORS_SHUT:
    case DOORS_LOCKED:
    case DOORS_BROKEN_IN:
        shapes[0].flags |= SHAPE_OFF;
        shapes[1].flags |= SHAPE_OFF;
        shapes[2].flags |= SHAPE_OFF;
        shapes[3].flags |= SHAPE_OFF;
        shapes[4].flags &= ~SHAPE_OFF;
        shapes[5].flags &= ~SHAPE_OFF;
        shapes[6].flags &= ~SHAPE_OFF;
        break;
    default: {
        Angle ang;
        Float x;
        Float z;

        shapes[0].flags &= ~SHAPE_OFF;
        shapes[1].flags &= ~SHAPE_OFF;
        shapes[2].flags &= ~SHAPE_OFF;
        shapes[3].flags &= ~SHAPE_OFF;
        shapes[4].flags |= SHAPE_OFF;
        shapes[5].flags &= ~SHAPE_OFF;
        shapes[6].flags &= ~SHAPE_OFF;
        ang = VARS(work)->open * DOOR_TURN;
        x = -DOOR_HINGE + (DOOR_HALF * njCos(ang));
        z = -(njSin(ang) * DOOR_HALF);
        shapes[1].center.x = x;
        shapes[0].center.x = -x;
        shapes[1].center.z = shapes[0].center.z = z;
        shapes[0].ang.y = -ang;
        shapes[1].ang.y = ang;
    }
    }
    collision_register(work);
}

void office_door_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        work->ang.z = work->ang.x = 0;
        work->submode = 0;
        if (!placement_editing(task)) {
            collision_init(task, office_door_shapes, COLLISION_OBJECT, 1, 7);
        }
        if (office_door_unlocked()) {
            work->mode = DOORS_SHUT;
        } else {
            work->mode = DOORS_LOCKED;
        }
        event_flags[FLAG_OFFICE_BARRICADE] = 0;
        task->step = office_door_step;
        task->draw = office_door_draw;
    }
}

static void office_door_step(struct task *task)
{
    struct task_work *work = task->work;
    int near;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case DOORS_SHUT:
        near = player_in_range(&work->pos, DOOR_REACH);
        if (!near) {
            break;
        }
        work->mode = DOORS_OPENING;
    /* @hack spelling: DOORS_SHUT falls into the default's break, and keeps a
     * store of DOORS_OPENING of its own (MOV.B R2,@R14 at 0x0c90ea9c). with
     * a break of its own, it shares DOORS_CLOSING's store, and 19 of 168
     * words differ */
    default:
        break;
    case DOORS_OPENING:
        VARS(work)->open += DOOR_SPEED;
        if (VARS(work)->open >= 1.0F) {
            VARS(work)->open = 1.0F;
            work->timer = 0;
            work->mode = DOORS_OPEN;
        }
        break;
    case DOORS_OPEN:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > DOOR_WAIT) {
            work->timer = 0;
            work->mode = DOORS_CLOSING;
        }
        break;
    case DOORS_CLOSING:
        near = player_in_range(&work->pos, DOOR_REACH);
        /* @hack spelling: an empty branch for no player near, which loads
         * DOORS_OPENING into R2 (MOV #1,R2 at 0x0c90eb2a). as if (near), R3
         * takes it and 2 of 168 words differ */
        if (!near) {
        } else {
            work->mode = DOORS_OPENING;
            break;
        }
        VARS(work)->open -= DOOR_SPEED;
        if (VARS(work)->open <= 0.0F) {
            VARS(work)->open = 0.0F;
            work->mode = DOORS_SHUT;
        }
        break;
    case DOORS_LOCKED:
        if (office_door_unlocked()) {
            work->mode = DOORS_SHUT;
        }
        if (event_flags[FLAG_OFFICE_BARRICADE]) {
            work->mode = DOORS_BROKEN_IN;
            work->timer = 0;
        }
        break;
    case DOORS_BROKEN_IN:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > DOOR_WAIT) {
            work->timer = 0;
            work->mode = DOORS_SWINGING;
        }
        break;
    case DOORS_SWINGING:
        VARS(work)->open += DOOR_SPEED;
        if (VARS(work)->open >= 1.0F) {
            VARS(work)->open = 1.0F;
            work->timer = 0;
            work->mode = DOORS_LEFT_OPEN;
        }
        break;
    case DOORS_LEFT_OPEN:
        break;
    }
    if (!placement_editing(task)) {
        office_door_collide(work);
    }
    office_door_draw(task);
}

/* draw the frame and its panels, then each door with its window, the two doors
 * swung open in opposite directions */
static void office_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle open = VARS(work)->open * DOOR_TURN;
    Angle ang;
    NJS_OBJECT *object;
    NJS_OBJECT *child;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(office_door_object.model);
    object = office_door_object.child;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    draw_model(object->model);
    njPopMatrix(1);
    object = object->sibling;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    draw_model(object->model);
    njPopMatrix(1);
    object = object->sibling;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN(njRotateY, open);
    draw_model(object->model);
    child = object->child;
    njTranslate(NULL, child->pos[0], child->pos[1], child->pos[2]);
    draw_model(child->model);
    njPopMatrix(1);
    object = object->sibling;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN_VIA(njRotateY, -open, ang);
    draw_model(object->model);
    child = object->child;
    njTranslate(NULL, child->pos[0], child->pos[1], child->pos[2]);
    draw_model(child->model);
    njPopMatrix(1);
    njPopMatrix(1);
}
