/* O TOROKKO DOOR double door, which swings open for the player nearest it once
 * a truck has set off with player 0 aboard */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/texlists.h"
#include "adv02/truck.h"
#include "adv02/truck_door.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

/* the door frame, which the door also puts in the collision world */
/* @hack type: the frame declared as an array, and read through ->. SHC then
 * loads its address and reads its model 4 bytes past it (MOV.L @(4,R4),R4 at
 * 0x0c908a6a). as an NJS_OBJECT read through ., 2 of truck_door_draw's 92
 * words differ */
extern NJS_OBJECT truck_door_object[];
/* the two leaves */
/* @hack type: both leaves declared as arrays, and read through ->. SHC then
 * pools each leaf's address and adds 8 to reach its pos (ADD #8,R5 at
 * 0x0c908a72 and 0x0c908aa4). as NJS_OBJECTs read through ., 55 of
 * truck_door_draw's 92 words differ */
extern NJS_OBJECT truck_door_left_object[];
extern NJS_OBJECT truck_door_right_object[];

/* the task lives while the camera or player 0 is within 500 units.
 * placement_try_leave_by takes the range squared */
#define RANGE (500.0F * 500.0F)
/* the collision world tests the door frame only while player 0 or player 1 is
 * within NEAR */
#define NEAR 150.0F
/* the collision record's radius. it reaches past the leaves' shapes, whose
 * farthest point lies about 55 units from the door's position */
#define RECORD_RADIUS 150.0F

/* the nearest player opens the door from within 30 units of it. OPEN_REACH is
 * 30 squared */
#define OPEN_REACH (30.0F * 30.0F)

/* how far each leaf turns out from shut: a quarter turn, in Ninja's angle
 * units */
#define OPEN_SWING 16384.0F
/* a moving leaf speeds up by SPEED_UP a step, up to TOP_SPEED. once it brakes,
 * it slows down by SLOW_DOWN a step, to no less than LOW_SPEED */
#define SPEED_UP 40.0F
#define TOP_SPEED 4096.0F
#define SLOW_DOWN 22.0F
#define LOW_SPEED 50.0F
/* an opening leaf brakes once it is OPENING_BRAKE of OPEN_SWING out from shut,
 * and a shutting one once it is SHUTTING_BRAKE out */
#define OPENING_BRAKE 0.6F
#define SHUTTING_BRAKE 0.4F

/* the door frame's flags in the collision world */
#define FRAME_WORLD_FLAGS WORLD_MEDIUM

enum truck_door_mode { DOOR_INIT, DOOR_RUN };

/* the door's states, in work->submode */
enum truck_door_state { DOOR_OPENING, DOOR_OPEN, DOOR_CLOSING, DOOR_CLOSED };

struct truck_door_vars {
    Float speed; /* the leaves' speed a step, below 0 while they shut */
    Float swing; /* how far the leaves have turned out from shut */
    NJS_OBJECT *object; /* the door frame's object in the collision world */
};

#define VARS(work) ((struct truck_door_vars *)&(work)->vars)

static void truck_door_init(struct task *task);
static void truck_door_reset(struct task *task);
static void truck_door_add_to_world(struct task *task);
static void truck_door_update(struct task *task);
static Bool truck_door_should_open(struct task_work *work);
static void truck_door_draw(struct task *task);
static void truck_door_destroy(struct task *task);

/* the leaves as they stand shut. the door registers them for the collision
 * checks only while it shuts or stays shut */
static struct collision_shape truck_door_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-10.0F, 24.5F, 0.0F},
     10.0F,
     26.5F,
     0.66F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {10.0F, 24.5F, 0.0F},
     10.0F,
     26.5F,
     0.66F,
     {0},
     0,
     NULL},
};

/* while the set editor holds the task, each step puts the door frame in the
 * collision world anew, where the set entry places it. it sets the door wide
 * open or shut, as truck_door_should_open says */
void truck_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            switch (work->mode) {
            case DOOR_INIT:
                truck_door_init(task);
                break;
            case DOOR_RUN:
                truck_door_draw(task);
                truck_door_update(task);
                break;
            }
        } else {
            switch (work->mode) {
            case DOOR_INIT:
                truck_door_init(task);
                break;
            case DOOR_RUN:
                truck_door_destroy(task);
                truck_door_reset(task);
                truck_door_draw(task);
                truck_door_update(task);
                break;
            }
        }
    }
}

static void truck_door_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = truck_door_draw;
    task->destructor = truck_door_destroy;
    collision_init(task, truck_door_shapes, COLLISION_OBJECT, 1, 2);
    work->collision->radius = RECORD_RADIUS;
    truck_door_reset(task);
    work->mode = DOOR_RUN;
}

static void truck_door_reset(struct task *task)
{
    struct task_work *work = task->work;

    if (truck_door_should_open(work)) {
        VARS(work)->swing = OPEN_SWING;
        work->submode = DOOR_OPEN;
    } else {
        VARS(work)->swing = 0.0F;
        work->submode = DOOR_CLOSED;
    }
    VARS(work)->speed = 0.0F;
    truck_door_add_to_world(task);
}

static void truck_door_add_to_world(struct task *task)
{
    struct task_work *work;
    struct task_force *force;
    NJS_OBJECT *from;
    NJS_OBJECT *object;

    work = task->work;
    force = task->force;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    from = truck_door_object;
    VARS(work)->object = object = world_object_alloc();
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
    world_add(FRAME_WORLD_FLAGS, task, object);
}

static void truck_door_update(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->submode) {
    case DOOR_OPENING:
        if (VARS(work)->swing < OPEN_SWING * OPENING_BRAKE) {
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
        VARS(work)->swing += VARS(work)->speed;
        if (VARS(work)->swing < 0.0F) {
            VARS(work)->swing = 0.0F;
        } else if (VARS(work)->swing > OPEN_SWING) {
            VARS(work)->swing = OPEN_SWING;
            work->submode = DOOR_OPEN;
        }
        if (!truck_door_should_open(work)) {
            work->submode = DOOR_CLOSING;
        }
        break;
    case DOOR_OPEN:
        VARS(work)->swing = OPEN_SWING;
        if (!truck_door_should_open(work)) {
            work->submode = DOOR_CLOSING;
        }
        break;
    case DOOR_CLOSING:
        if (VARS(work)->swing > OPEN_SWING * SHUTTING_BRAKE) {
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
        VARS(work)->swing += VARS(work)->speed;
        if (VARS(work)->swing < 0.0F) {
            VARS(work)->swing = 0.0F;
            work->submode = DOOR_CLOSED;
        } else if (VARS(work)->swing > OPEN_SWING) {
            VARS(work)->swing = OPEN_SWING;
        }
        if (truck_door_should_open(work)) {
            work->submode = DOOR_OPENING;
        }
        collision_register(work);
        break;
    case DOOR_CLOSED:
        VARS(work)->swing = 0.0F;
        if (truck_door_should_open(work)) {
            work->submode = DOOR_OPENING;
        }
        collision_register(work);
        break;
    }
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* TRUE while a truck has set off with player 0 aboard and the player nearest
 * the door is within OPEN_REACH of it, else FALSE */
static Bool truck_door_should_open(struct task_work *work)
{
    NJS_POINT3 player;
    Float x;
    Float y;
    Float z;

    if (truck_ridden) {
        player = player_work[player_nearest(&work->pos)]->pos;
        x = player.x - work->pos.x;
        y = player.y - work->pos.y;
        z = player.z - work->pos.z;
        if ((x * x) + (y * y) + (z * z) < OPEN_REACH) {
            return TRUE;
        }
    }
    return FALSE;
}

static void truck_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float left;
    Float right;
    Angle ang;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(truck_door_object->model);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)truck_door_left_object->pos);
    /* @hack spelling: left compared with 0.0F, which tests nothing a test of
     * left alone doesn't. SHC then reads left back off the stack for the call
     * (FMOV.S @R15,FR3 at 0x0c908a86). written as TURN_VIA, as the right leaf
     * is, 48 of 92 words differ */
    ((left = -VARS(work)->swing) != 0.0F) ? njRotateY(NULL, left) : (void)0;
    draw_model(truck_door_left_object->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)truck_door_right_object->pos);
    TURN_VIA(njRotateY, VARS(work)->swing, right);
    draw_model(truck_door_right_object->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void truck_door_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}
