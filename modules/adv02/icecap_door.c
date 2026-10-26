/* the door the object list names O ICECAP DOOR: a sliding door to Ice Cap,
 * open while the Ice Stone's lock holds its key */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/handy_key.h"
#include "adv02/icecap_door.h"
#include "adv02/lock.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the door's frame, whose model is its collision model too */
/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address at 0x0c9090b8 and reads model through it, MOV.L @(4,R4),R4 at
 * 0x0c90901c; declared as one, 2 of icecap_door_draw's 81 words and a pool
 * word differ */
extern NJS_OBJECT icecap_door_object[];
/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address once, at 0x0c9090bc, and reads pos and model through it, ADD
 * #8,R11 at 0x0c909028 and MOV.L @(4,R4),R4 at 0x0c909040; declared as one,
 * 45 of icecap_door_draw's 81 words differ */
extern NJS_OBJECT icecap_door_left_object[];
/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address once, at 0x0c9090c8, and reads pos and model through it, ADD
 * #8,R13 at 0x0c909052 and MOV.L @(4,R4),R4 at 0x0c90906a; declared as one,
 * 24 of icecap_door_draw's 81 words differ */
extern NJS_OBJECT icecap_door_right_object[];

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 500 */
#define RANGE (500.0F * 500.0F)
/* the collision world tests the frame only while a player is this near */
#define NEAR 150.0F
#define RADIUS 150.0F /* the collision record's radius */

#define LOCK KEY_ICE_STONE

#define TRAVEL 21.0F /* how far each panel slides out from shut */
/* the x distance of each panel's box from the doorway's middle, shut */
#define PANEL_X 21.0F
/* the panels' speed, in units a step. moving either way, they gain SPEED_UP
 * each step up to TOP_SPEED until they pass the turn. they then lose
 * SLOW_DOWN each step down to LOW_SPEED */
#define TOP_SPEED 1.5F
#define LOW_SPEED 0.1F
#define SPEED_UP 0.05F
#define SLOW_DOWN 0.08F
/* the turn, as distances out: opening and shutting panels pass it 60 percent
 * of the way along */
#define OPENING_TURN (TRAVEL * 0.6F)
#define SHUTTING_TURN (TRAVEL * 0.4F)

#define FRAME_FLAGS WORLD_MEDIUM

enum icecap_door_mode { ICECAP_DOOR_INIT, ICECAP_DOOR_RUN };

/* the door's states, in work->submode */
enum icecap_door_state { DOOR_OPENING, DOOR_OPEN, DOOR_CLOSING, DOOR_CLOSED };

struct icecap_door_vars {
    Float speed; /* the panels' speed, positive outward */
    Float out; /* how far each panel is out from shut */
    NJS_OBJECT *frame; /* the frame's copy in the collision world */
};

#define VARS(work) ((struct icecap_door_vars *)&(work)->vars)

static void icecap_door_init(struct task *task);
static void icecap_door_reset(struct task *task);
static void icecap_door_add_frame(struct task *task);
static void icecap_door_update(struct task *task);
static void icecap_door_draw(struct task *task);
static void icecap_door_destroy(struct task *task);

/* a box for each panel, then one across the shut doorway */
enum { LEFT_SHAPE, RIGHT_SHAPE, SHUT_SHAPE };
static struct collision_shape icecap_door_shapes[3] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-PANEL_X, 24.0F, 2.62F},
     21.25F,
     24.25F,
     4.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {PANEL_X, 24.0F, 2.62F},
     21.25F,
     24.25F,
     4.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 24.0F, 2.62F},
     42.5F,
     24.25F,
     4.0F,
     {0},
     0,
     NULL},
};

/* while the set editor holds the door, each step rebuilds the frame's world
 * object, so it follows the edit */
void icecap_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case ICECAP_DOOR_INIT:
            icecap_door_init(task);
            break;
        case ICECAP_DOOR_RUN:
            icecap_door_draw(task);
            icecap_door_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case ICECAP_DOOR_INIT:
            icecap_door_init(task);
            break;
        case ICECAP_DOOR_RUN:
            icecap_door_destroy(task);
            icecap_door_reset(task);
            icecap_door_draw(task);
            icecap_door_update(task);
            break;
        }
    }
}

static void icecap_door_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = icecap_door_draw;
    task->destructor = icecap_door_destroy;
    collision_init(task, icecap_door_shapes, COLLISION_OBJECT, 1, 3);
    work->collision->radius = RADIUS;
    icecap_door_reset(task);
    work->mode = ICECAP_DOOR_RUN;
}

static void icecap_door_reset(struct task *task)
{
    struct task_work *work = task->work;
    Bool open = lock_flag(LOCK);

    if (open) {
        VARS(work)->out = TRAVEL;
        work->submode = DOOR_OPEN;
    } else {
        VARS(work)->out = 0.0F;
        work->submode = DOOR_CLOSED;
    }
    VARS(work)->speed = 0.0F;
    icecap_door_add_frame(task);
}

static void icecap_door_add_frame(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *from;
    NJS_OBJECT *object;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    from = icecap_door_object;
    VARS(work)->frame = object = world_object_alloc();
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = from->pos[0] + work->pos.x;
    object->pos[1] = from->pos[1] + work->pos.y;
    object->pos[2] = from->pos[2] + work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(FRAME_FLAGS, task, object);
}

static void icecap_door_update(struct task *task)
{
    struct task_work *work = task->work;
    Bool open;

    switch (work->submode) {
    case DOOR_OPENING:
        if (VARS(work)->out < OPENING_TURN) {
            VARS(work)->speed += SPEED_UP;
            if (VARS(work)->speed > TOP_SPEED) {
                VARS(work)->speed = TOP_SPEED;
            }
        } else {
            VARS(work)->speed -= SLOW_DOWN;
            if (VARS(work)->speed < LOW_SPEED) {
                VARS(work)->speed = LOW_SPEED;
            }
        }
        VARS(work)->out += VARS(work)->speed;
        if (VARS(work)->out < 0.0F) {
            VARS(work)->out = 0.0F;
        } else if (VARS(work)->out > TRAVEL) {
            VARS(work)->out = TRAVEL;
            work->submode = DOOR_OPEN;
            switch (get_character()) {
            case CHARACTER_SONIC:
                event_flag_set(FLAG_SONIC_MR_ENTRANCE_ICECAP);
                break;
            case CHARACTER_TAILS:
                event_flag_set(FLAG_TAILS_MR_ENTRANCE_ICECAP);
                break;
            case CHARACTER_BIG:
                event_flag_set(FLAG_BIG_MR_ENTRANCE_ICECAP);
                break;
            }
        }
        open = lock_flag(LOCK);
        if (!open) {
            work->submode = DOOR_CLOSING;
        }
        break;
    case DOOR_OPEN:
        VARS(work)->out = TRAVEL;
        open = lock_flag(LOCK);
        if (!open) {
            work->submode = DOOR_CLOSING;
        }
        break;
    case DOOR_CLOSING:
        if (VARS(work)->out > SHUTTING_TURN) {
            VARS(work)->speed -= SPEED_UP;
            if (VARS(work)->speed < -TOP_SPEED) {
                VARS(work)->speed = -TOP_SPEED;
            }
        } else {
            VARS(work)->speed += SLOW_DOWN;
            if (VARS(work)->speed > -LOW_SPEED) {
                VARS(work)->speed = -LOW_SPEED;
            }
        }
        VARS(work)->out += VARS(work)->speed;
        if (VARS(work)->out < 0.0F) {
            VARS(work)->out = 0.0F;
            work->submode = DOOR_CLOSED;
        } else if (VARS(work)->out > TRAVEL) {
            VARS(work)->out = TRAVEL;
        }
        open = lock_flag(LOCK);
        if (open) {
            work->submode = DOOR_OPENING;
        }
        break;
    case DOOR_CLOSED:
        VARS(work)->out = 0.0F;
        open = lock_flag(LOCK);
        if (open) {
            work->submode = DOOR_OPENING;
        }
        break;
    }
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (work->submode == DOOR_CLOSED) {
        work->collision->shapes[LEFT_SHAPE].flags |= SHAPE_OFF;
        work->collision->shapes[RIGHT_SHAPE].flags |= SHAPE_OFF;
        work->collision->shapes[SHUT_SHAPE].flags &= ~SHAPE_OFF;
    } else {
        work->collision->shapes[LEFT_SHAPE].center.x =
            -VARS(work)->out - PANEL_X;
        work->collision->shapes[RIGHT_SHAPE].center.x =
            VARS(work)->out + PANEL_X;
        work->collision->shapes[LEFT_SHAPE].flags &= ~SHAPE_OFF;
        work->collision->shapes[RIGHT_SHAPE].flags &= ~SHAPE_OFF;
        work->collision->shapes[SHUT_SHAPE].flags |= SHAPE_OFF;
    }
    collision_register(work);
}

static void icecap_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(icecap_door_object->model);
    njPushMatrix(NULL);
    njTranslate(NULL, icecap_door_left_object->pos[0] - VARS(work)->out,
                icecap_door_left_object->pos[1],
                icecap_door_left_object->pos[2]);
    draw_model(icecap_door_left_object->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, icecap_door_right_object->pos[0] + VARS(work)->out,
                icecap_door_right_object->pos[1],
                icecap_door_right_object->pos[2]);
    draw_model(icecap_door_right_object->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void icecap_door_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->frame);
    world_object_free(VARS(work)->frame);
}
