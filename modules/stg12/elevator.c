/* O ELEVATOR: a car that carries the players down its shaft, then opens its
 * doors */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg12/elevator.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shelter_elevator_object;
extern NJS_OBJECT shelter_elevator_door_a_object;
extern NJS_OBJECT shelter_elevator_door_b_object;
extern NJS_OBJECT shelter_elevator_world_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* each step, the car sinks DOWN_SPEED and the doors open OPEN_SPEED of the
 * way */
#define DOWN_SPEED 0.6F
#define OPEN_SPEED 0.02F

#define CAR_FLAGS (WORLD_LARGE | WORLD_MOVES)

#define SE_ELEVATOR_RIDE 306
#define SE_CLANK 307

/* in act ACT_STILL the car stands still, its doors shut. in act ACT_PADS_OFF
 * the pads are off until the doors open */
#define ACT_STILL 1
#define ACT_PADS_OFF 2

enum shelter_elevator_mode {
    ELEVATOR_INIT,
    ELEVATOR_OPEN,
    ELEVATOR_WAIT,
    ELEVATOR_DOWN,
    ELEVATOR_OPENING,
    ELEVATOR_STILL
};

struct shelter_elevator_vars {
    Uint8 act;
    Uint8 unk_01[7];
    NJS_OBJECT *car; /* the car's object in the collision world */
};

/* the doors' state, in work->scl. SHUT_SHARE is the share of the way the
 * doors are from open: 1 shut, 0 open. DOOR_A_X and DOOR_B_X are their
 * offsets along x */
#define SHUT_SHARE(work) ((work)->scl.z)
#define DOOR_A_X(work) ((work)->scl.y)
#define DOOR_B_X(work) ((work)->scl.x)

#define VARS(work) ((struct shelter_elevator_vars *)&(work)->vars)
#define FORCE(task) ((struct task_force *)(task)->force)

/* each act's shaft: where the car starts and where it stops */
struct shelter_elevator_shaft {
    NJS_POINT3 top;
    NJS_POINT3 bottom;
};

static void shelter_elevator_init(struct task *task);
static void shelter_elevator_update(struct task *task);
static void shelter_elevator_draw(struct task *task);
static void shelter_elevator_destroy(struct task *task);
static void shelter_elevator_wait(struct task *task);
static void shelter_elevator_down(struct task *task);
static void shelter_elevator_open(struct task *task);
static void shelter_elevator_set_doors(struct task_work *work);

/* the doorway, solid until the doors open */
static struct collision_shape shelter_elevator_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 15.0F, -45.0F},
    17.0F,
    40.0F,
    1.0F,
    {0},
    0,
    NULL,
};

static struct shelter_elevator_shaft shelter_elevator_shafts[3] = {
#include "stg12/elevator/shelter_elevator_shafts.inc"
};

void shelter_elevator_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case ELEVATOR_INIT:
            shelter_elevator_init(task);
            break;
        case ELEVATOR_OPEN:
            shelter_elevator_draw(task);
            break;
        case ELEVATOR_WAIT:
            shelter_elevator_draw(task);
            shelter_elevator_wait(task);
            shelter_elevator_update(task);
            break;
        case ELEVATOR_DOWN:
            shelter_elevator_draw(task);
            shelter_elevator_down(task);
            shelter_elevator_update(task);
            if ((Uint8)stage_get_stage_act() == ACT_PADS_OFF) {
                pad_disable();
            }
            break;
        case ELEVATOR_OPENING:
            shelter_elevator_draw(task);
            shelter_elevator_open(task);
            shelter_elevator_update(task);
            break;
        case ELEVATOR_STILL:
            shelter_elevator_draw(task);
            shelter_elevator_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case ELEVATOR_INIT:
            shelter_elevator_init(task);
            break;
        case ELEVATOR_OPEN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            shelter_elevator_draw(task);
            shelter_elevator_update(task);
            break;
        }
    }
}

static void shelter_elevator_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *car;

    task->draw = shelter_elevator_draw;
    task->destructor = shelter_elevator_destroy;
    VARS(work)->act = stage_get_stage_act();
    work->pos = shelter_elevator_shafts[VARS(work)->act].top;
    if (VARS(work)->act == ACT_PADS_OFF) {
        pad_disable();
    }
    VARS(work)->car = car = world_object_alloc();
    car->evalflags = shelter_elevator_world_object.evalflags;
    car->scl[0] = 1.0F;
    car->scl[1] = 1.0F;
    car->scl[2] = 1.0F;
    car->ang[0] = 0;
    car->ang[1] = 0;
    car->ang[2] = 0;
    car->pos[0] = work->pos.x + shelter_elevator_world_object.pos[0];
    car->pos[1] = work->pos.y + shelter_elevator_world_object.pos[1];
    car->pos[2] = work->pos.z + shelter_elevator_world_object.pos[2];
    car->model = shelter_elevator_world_object.model;
    car->child = NULL;
    car->sibling = NULL;
    world_add(CAR_FLAGS, task, car);
    FORCE(task)->speed.x = 0.0F;
    FORCE(task)->speed.y = 0.0F;
    FORCE(task)->speed.z = 0.0F;
    FORCE(task)->turn.x = 0;
    FORCE(task)->turn.y = 0;
    FORCE(task)->turn.z = 0;
    collision_init(task, &shelter_elevator_shape, COLLISION_OBJECT, 1, 1);
    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    SHUT_SHARE(work) = 1.0F;
    shelter_elevator_set_doors(work);
    if (VARS(work)->act == ACT_STILL) {
        work->mode = ELEVATOR_STILL;
    } else {
        work->mode = ELEVATOR_WAIT;
    }
}

static void shelter_elevator_update(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *car = VARS(work)->car;
    NJS_OBJECT *from = &shelter_elevator_world_object;
    /* @hack redundant: 24 bytes of frame that no code reads, of a type unknown,
     * declared as two points. the image's ADD #-24,R15 reserves them
     * (0x0c902158); without them, 32 of 33 words differ */
    NJS_POINT3 unused[2];

    car->ang[0] = 0;
    car->ang[1] = work->ang.y;
    car->ang[2] = 0;
    car->pos[0] = work->pos.x + from->pos[0];
    car->pos[1] = work->pos.y + from->pos[1];
    car->pos[2] = work->pos.z + from->pos[2];
    collision_register(work);
}

static void shelter_elevator_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&shelter_elevator_object);
        njPushMatrix(NULL);
        njTranslate(NULL, DOOR_A_X(work), 15.0F, -45.0F);
        draw_tree(&shelter_elevator_door_a_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, DOOR_B_X(work), 15.0F, -45.0F);
        draw_tree(&shelter_elevator_door_b_object);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

static void shelter_elevator_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->car);
    world_object_free(VARS(work)->car);
}

static void shelter_elevator_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (player_stands_on(0, task) || player_stands_on(1, task)) {
        work->mode = ELEVATOR_DOWN;
    }
}

/* the car's force carries player 0 down with it */
static void shelter_elevator_down(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 bottom;

    bottom = shelter_elevator_shafts[VARS(work)->act].bottom;
    if (work->pos.y <= bottom.y) {
        FORCE(task)->speed.y = 0.0F;
        work->mode = ELEVATOR_OPENING;
        sound_play(SE_CLANK, 0, 0, 0);
    } else {
        work->pos.y -= DOWN_SPEED;
        FORCE(task)->speed.y = -DOWN_SPEED;
        sound_play_for(SE_ELEVATOR_RIDE, (int)task, 1, 0, 2);
    }
}

static void shelter_elevator_open(struct task *task)
{
    struct task_work *work = task->work;

    SHUT_SHARE(work) -= OPEN_SPEED;
    if (SHUT_SHARE(work) <= 0.0F) {
        SHUT_SHARE(work) = 0.0F;
        work->mode = ELEVATOR_OPEN;
        pad_enable();
    }
    shelter_elevator_set_doors(work);
}

static void shelter_elevator_set_doors(struct task_work *work)
{
    DOOR_B_X(work) = (SHUT_SHARE(work) * 12.0F) - 25.0F;
    DOOR_A_X(work) = (SHUT_SHARE(work) * 24.0F) - 24.0F;
}
