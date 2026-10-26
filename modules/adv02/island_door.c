/* the door the object list names O ISLAND DOOR: a sliding door, open while
 * the island door flag of player 0's character is set */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/island_door.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/util.h"

/* the door's frame, whose model is its collision model too */
/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address at 0x0c9070e4 and reads model through it, MOV.L @(4,R4),R4 at
 * 0x0c907040; declared as one, 2 of island_door_draw's 81 words and a pool
 * word differ */
extern NJS_OBJECT island_door_object[];
/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address once, at 0x0c9070e8, and reads pos and model through it, ADD
 * #8,R11 at 0x0c90704c and MOV.L @(4,R4),R4 at 0x0c907064; declared as one,
 * 45 of island_door_draw's 81 words differ */
extern NJS_OBJECT island_door_left_object[];
/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address once, at 0x0c9070f4, and reads pos and model through it, ADD
 * #8,R13 at 0x0c907076 and MOV.L @(4,R4),R4 at 0x0c90708e; declared as one,
 * 24 of island_door_draw's 81 words differ */
extern NJS_OBJECT island_door_right_object[];

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 500 */
#define RANGE (500.0F * 500.0F)
/* the collision world tests the frame only while a player is this near */
#define NEAR 150.0F
#define RADIUS 150.0F /* the collision record's radius */

#define TRAVEL 19.0F /* how far each panel slides out from shut */
/* the x distance of each panel's box from the doorway's middle, shut */
#define PANEL_X 13.0F
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

enum island_door_mode { ISLAND_DOOR_INIT, ISLAND_DOOR_RUN };

/* the door's states, in work->submode */
enum island_door_state { DOOR_OPENING, DOOR_OPEN, DOOR_CLOSING, DOOR_CLOSED };

struct island_door_vars {
    Float speed; /* the panels' speed, positive outward */
    Float out; /* how far each panel is out from shut */
    NJS_OBJECT *frame; /* the frame's copy in the collision world */
};

#define VARS(work) ((struct island_door_vars *)&(work)->vars)

static void island_door_init(struct task *task);
static void island_door_reset(struct task *task);
static void island_door_add_frame(struct task *task);
static void island_door_update(struct task *task);
static Bool island_door_opened(struct task_work *work);
static void island_door_draw(struct task *task);
static void island_door_destroy(struct task *task);

/* the flag that opens the door, by character, or -1 for Eggman, Tails, Tikal
 * and Big */
static int island_door_flags[CHARACTER_COUNT] = {
    FLAG_SONIC_MR_ISLAND_DOOR,    -1, -1,
    FLAG_KNUCKLES_MR_ISLAND_DOOR, -1, FLAG_AMY_MR_ISLAND_DOOR,
    FLAG_E102_MR_ISLAND_DOOR,     -1,
};

/* a box for each panel, then one across the shut doorway */
enum { LEFT_SHAPE, RIGHT_SHAPE, SHUT_SHAPE };
static struct collision_shape island_door_shapes[3] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-PANEL_X, 25.0F, -4.0F},
     13.0F,
     25.0F,
     4.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {PANEL_X, 25.0F, -4.0F},
     13.0F,
     25.0F,
     4.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 25.0F, -4.0F},
     26.0F,
     25.0F,
     4.0F,
     {0},
     0,
     NULL},
};

/* while the set editor holds the door, each step rebuilds the frame's world
 * object, so it follows the edit */
void island_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case ISLAND_DOOR_INIT:
            island_door_init(task);
            break;
        case ISLAND_DOOR_RUN:
            island_door_draw(task);
            island_door_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case ISLAND_DOOR_INIT:
            island_door_init(task);
            break;
        case ISLAND_DOOR_RUN:
            island_door_destroy(task);
            island_door_reset(task);
            island_door_draw(task);
            island_door_update(task);
            break;
        }
    }
}

static void island_door_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = island_door_draw;
    task->destructor = island_door_destroy;
    collision_init(task, island_door_shapes, COLLISION_OBJECT, 1, 3);
    work->collision->radius = RADIUS;
    island_door_reset(task);
    work->mode = ISLAND_DOOR_RUN;
}

static void island_door_reset(struct task *task)
{
    struct task_work *work = task->work;

    if (island_door_opened(work)) {
        VARS(work)->out = TRAVEL;
        work->submode = DOOR_OPEN;
    } else {
        VARS(work)->out = 0.0F;
        work->submode = DOOR_CLOSED;
    }
    VARS(work)->speed = 0.0F;
    island_door_add_frame(task);
}

static void island_door_add_frame(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;
    NJS_OBJECT *from;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    from = island_door_object;
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

static void island_door_update(struct task *task)
{
    struct task_work *work = task->work;

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
        }
        if (!island_door_opened(work)) {
            work->submode = DOOR_CLOSING;
        }
        break;
    case DOOR_OPEN:
        VARS(work)->out = TRAVEL;
        if (!island_door_opened(work)) {
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
        if (island_door_opened(work)) {
            work->submode = DOOR_OPENING;
        }
        break;
    case DOOR_CLOSED:
        VARS(work)->out = 0.0F;
        if (island_door_opened(work)) {
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

static Bool island_door_opened(struct task_work *work)
{
    int flag = island_door_flags[player_get_character(0)];

    if (flag >= 0) {
        return event_flags[flag];
    }
    return FALSE;
}

static void island_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(island_door_object->model);
    njPushMatrix(NULL);
    njTranslate(NULL, island_door_left_object->pos[0] - VARS(work)->out,
                island_door_left_object->pos[1],
                island_door_left_object->pos[2]);
    draw_model(island_door_left_object->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, island_door_right_object->pos[0] + VARS(work)->out,
                island_door_right_object->pos[1],
                island_door_right_object->pos[2]);
    draw_model(island_door_right_object->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void island_door_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->frame);
    world_object_free(VARS(work)->frame);
}
