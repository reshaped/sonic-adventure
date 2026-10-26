/* O JUMP: a solid ramp with a turning wheel, which launches a player who
 * touches it */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg01/jump.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_OBJECT beach_jump_object;
extern NJS_OBJECT beach_jump_wheel_object;
extern NJS_OBJECT beach_jump_top_object;

/* after a launch, the ramp waits WAIT_STEPS + 1 steps before it can launch
 * again. the world doesn't test the ramp's copy while it waits */
#define WAIT_STEPS 10

/* a launch sends the player off LAUNCH_RISE a step up */
#define LAUNCH_RISE 4.0F

/* the world tests the ramp's copy only while a player is within its radius
 * plus TEST_MARGIN */
#define TEST_MARGIN 16.0F

/* the wheel's turn each draw, in degrees */
#define WHEEL_TURN 5

/* the ramp's vars. work->submode holds the number of the player to launch.
 * work->tick counts the wait down from WAIT_STEPS to 0, a step at each.
 * TASK_TIMER(work) holds the wheel's turn in degrees */
struct jump_vars {
    NJS_OBJECT *world; /* the ramp's copy in the collision world */
    Uint8 unk_04[4];
    NJS_OBJECT *object; /* the ramp model drawn */
};

#define VARS(work) ((struct jump_vars *)&(work)->vars)

/* in JUMP_IDLE, the step only checks its range. no code sets it */
enum jump_mode { JUMP_INIT, JUMP_RUN, JUMP_LAUNCH, JUMP_WAITING, JUMP_IDLE };

/* the box a player touches to be launched */
static struct collision_shape beach_jump_shape = {SHAPE_RECTANGLE,
                                                  0,
                                                  PUSH_FIRM(PUSH_ALL),
                                                  {0.0F, 10.0F, 0.0F},
                                                  20.0F,
                                                  10.0F,
                                                  10.0F,
                                                  {0, 0, 0},
                                                  0,
                                                  NULL};

static void beach_jump_add_world(struct task *task, NJS_OBJECT *object);
static void beach_jump_mark_and_launch(struct task *task);
static void beach_jump_draw(struct task *task);
static void beach_jump_destroy(struct task *task);

/* launch a player who touches the ramp, then wait. the launch sends the
 * player off at scale x a step forward and LAUNCH_RISE up. the speed is in
 * the frame of the ramp's angles, turned a half turn about y. the player's
 * stick is ignored for scale y steps, rounded up */
void beach_jump_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case JUMP_INIT:
        task->destructor = beach_jump_destroy;
        task->draw = beach_jump_draw;
        beach_jump_add_world(task, &beach_jump_object);
        collision_init(task, &beach_jump_shape, COLLISION_OBJECT, 1, 1);
        VARS(work)->object = &beach_jump_object;
        work->mode = JUMP_RUN;
        break;
    case JUMP_RUN:
        if (!placement_editing(task)) {
            beach_jump_mark_and_launch(task);
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "<- -EXPRESS");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<-V SPEED");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<-NO_CONTROL TIME");
        }
        beach_jump_draw(task);
        break;
    case JUMP_LAUNCH: {
        Angle ang[3];
        NJS_VECTOR speed = {0.0F, LAUNCH_RISE, 0.0F};
        int steps;

        sound_play(SE_DASH, 0, 0, 0);
        speed.x = work->scl.x;
        ang[0] = work->ang.x;
        ang[1] = work->ang.y + NJM_DEG_ANG(180);
        ang[2] = work->ang.z;
        steps = ceilf(work->scl.y);
        player_send_flying(work->submode, &speed, ang, steps);
        vibration_start(work->submode, 4, 15, 0);
        work->flags &= ~WORLD_TESTED;
        work->mode = JUMP_WAITING;
        work->tick = WAIT_STEPS;
        beach_jump_draw(task);
        break;
    }
    case JUMP_WAITING:
        /* @hack spelling: the test and the store as one conditional expression,
         * as at 0x0c901518. as an if statement, SHC holds 1 in R12 through
         * the step, its code runs 32 bytes short, and 97 of its 170 words
         * differ */
        (work->tick-- <= 0) ? (void)(work->mode = JUMP_RUN) : (void)0;
        work->flags &= ~WORLD_TESTED;
        beach_jump_draw(task);
        break;
    case JUMP_IDLE:
    default:
        break;
    }
}

/* put a copy of object in the world, posed as the task. the copy is
 * WORLD_SOLID, with the WORLD_SIZE its model's radius picks */
static void beach_jump_add_world(struct task *task, NJS_OBJECT *object)
{
    struct task_work *work = task->work;
    NJS_OBJECT *copy;
    Uint32 flags;

    VARS(work)->world = copy = world_object_alloc();
    copy->scl[0] = 1.0F;
    copy->scl[1] = 1.0F;
    copy->scl[2] = 1.0F;
    copy->ang[0] = work->ang.x;
    copy->ang[1] = work->ang.y;
    copy->ang[2] = work->ang.z;
    copy->pos[0] = work->pos.x;
    copy->pos[1] = work->pos.y;
    copy->pos[2] = work->pos.z;
    copy->model = object->model;
    copy->child = object->child;
    copy->sibling = object->sibling;
    flags = WORLD_HUGE | WORLD_SOLID;
    /* @hack spelling: the empty branch for a radius over 20, which keeps flags
     * as set, by the BT/S at 0x0c9015e2. as if (r <= 20.0F) around the other
     * sizes, 2 of 80 words differ */
    if (copy->model->r > 20.0F) {
    } else if (copy->model->r > 10.0F) {
        flags = WORLD_LARGE | WORLD_SOLID;
    } else if (copy->model->r > 3.0F) {
        flags = WORLD_MEDIUM | WORLD_SOLID;
    } else {
        flags = WORLD_SMALL | WORLD_SOLID;
    }
    world_add(flags, task, copy);
    /* @hack redundant: a return where the function ends, which has SHC keep
     * task in a stack slot: ADD #-4,R15 at 0x0c901586 makes it, and MOV.L
     * R4,@R15 at 0x0c90158c fills it. without it, SHC pushes task, and 79 of 80
     * words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

/* keep the ramp's copy in the world posed as the task, and tested while a
 * player is near. a player whose task hit the box is launched on the next
 * step */
static void beach_jump_mark_and_launch(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *copy = VARS(work)->world;
    struct task *player;
    int i;

    copy->pos[0] = work->pos.x;
    copy->pos[1] = work->pos.y;
    copy->pos[2] = work->pos.z;
    copy->ang[0] = work->ang.x;
    copy->ang[1] = work->ang.y;
    copy->ang[2] = work->ang.z;
    if (player_in_range(&work->pos, copy->model->r + TEST_MARGIN)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (work->collision->flags & COLLISION_HIT) {
        work->collision->flags &= ~COLLISION_HIT;
        for (i = 0; i < 2; ++i) {
            player = player_get_task(i);
            if (work->collision->hit->owner == player) {
                work->submode = i;
                work->mode = JUMP_LAUNCH;
            }
        }
    }
}

/* draw the ramp, its top and its wheel. the wheel turns backward about x by
 * WHEEL_TURN degrees each draw */
static void beach_jump_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;
    NJS_OBJECT *top = &beach_jump_top_object;
    NJS_OBJECT *wheel = &beach_jump_wheel_object;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    draw_model(object->model);
    njPushMatrix(NULL);
    njTranslate(NULL, top->pos[0], top->pos[1], top->pos[2]);
    TURN_VIA(njRotateZ, top->ang[2], ang);
    TURN_VIA(njRotateY, top->ang[1], ang);
    TURN_VIA(njRotateX, top->ang[0], ang);
    draw_model(top->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, wheel->pos[0], wheel->pos[1], wheel->pos[2]);
    TURN_VIA(njRotateZ, wheel->ang[2], ang);
    TURN_VIA(njRotateY, wheel->ang[1], ang);
    TURN_VIA(njRotateX, wheel->ang[0], ang);
    TURN_VIA(njRotateX, -(Angle)(TASK_TIMER(work) * ANGLE_PER_DEGREE), ang);
    draw_model(wheel->model);
    njPopMatrix(1);
    njPopMatrix(1);
    TASK_TIMER(work) += WHEEL_TURN;
}

static void beach_jump_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->world);
    world_object_free(VARS(work)->world);
}
