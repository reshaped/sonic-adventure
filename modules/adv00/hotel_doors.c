/* O HOTEL FRONT, O HOTEL BACK and O HOTEL POOL folding doors, locked until the
 * story of player 0's character opens them */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/hotel_doors.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: the folding door's action declared as an array. SHC then pools
 * its address and reads its motion through it, MOV.L @(4,R2),R1 at 0x0c90c66e.
 * declared as one NJS_ACTION, SHC pools the motion field's address, and 10 of
 * hotel_door_collide_open's 125 words, 5 of hotel_door_close's 54 and 3 of
 * hotel_door_open's 54 differ */
extern NJS_ACTION folding_door_action[];

#define DOOR_REACH 20.0F /* within which a player opens the door */
#define DOOR_SPEED 0.2F /* motion frames a step */
#define DOOR_WAIT 90 /* steps the door stays fully open */
#define DOOR_TURN 12743.0F /* each panel's turn when fully open, 70 degrees */
#define PANEL_HALF 6.25F /* half a panel's width */
/* half the four panels' width, where each pair hinges */
#define DOOR_HALF 25.0F

/* sound of the door folding open or shut */
#define SE_DOOR 530

enum hotel_door_shape {
    DOOR_PANEL = 0, /* the first of four, from the left */
    DOOR_SIDE_LEFT = 4,
    DOOR_SIDE_RIGHT = 5,
    DOOR_WALL = 6, /* a wall across the shut doorway */
    DOOR_POST_LEFT = 7,
    DOOR_POST_RIGHT = 8,
    DOOR_SHAPES = 9
};

struct hotel_door_vars {
    Float frame; /* the door's frame in its motion, 0 shut */
    Bool (*open_test)(void); /* the test that unlocks it */
};

#define VARS(work) ((struct hotel_door_vars *)&(work)->vars)

static void hotel_door_wait(struct task *task);
static void hotel_door_open(struct task *task);
static void hotel_door_idle(struct task *task);

/* clang-format off */
static struct collision_shape hotel_door_shapes[DOOR_SHAPES] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-18.75F, 12.5F, 0.0F},
     PANEL_HALF, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-6.25F, 12.5F, 0.0F},
     PANEL_HALF, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {6.25F, 12.5F, 0.0F},
     PANEL_HALF, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {18.75F, 12.5F, 0.0F},
     PANEL_HALF, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-27.5F, 12.5F, 0.0F},
     2.5F, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {27.5F, 12.5F, 0.0F},
     2.5F, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 12.5F, 0.0F},
     30.0F, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-30.0F, 12.5F, 0.0F},
     5.0F, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {30.0F, 12.5F, 0.0F},
     5.0F, 12.5F, 1.0F, {0}, 0, NULL},
};
/* clang-format on */

static Bool hotel_front_open(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_SS_HOTEL_FRONT];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_HOTEL_FRONT];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_HOTEL_FRONT];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_SS_HOTEL_FRONT];
    case CHARACTER_E102:
        return event_flags[FLAG_E102_SS_HOTEL_FRONT];
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_SS_HOTEL_FRONT];
    }
    return FALSE;
}

static Bool hotel_back_open(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_SS_HOTEL_BACK];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_HOTEL_BACK];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_HOTEL_BACK];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_SS_HOTEL_BACK];
    case CHARACTER_E102:
        return FALSE;
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_SS_HOTEL_BACK];
    }
    return FALSE;
}

static Bool hotel_pool_open(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
    case CHARACTER_TAILS:
    case CHARACTER_KNUCKLES:
    case CHARACTER_AMY:
    case CHARACTER_E102:
        return TRUE;
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_SS_HOTEL_POOL];
    }
    return FALSE;
}

static void hotel_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_action(folding_door_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* the number, from 1, of a player within DOOR_REACH of the door, or 0 for
 * none */
static int hotel_door_near(struct task_work *work)
{
    return player_in_range(&work->pos, DOOR_REACH);
}

/* make the open door's panels, sides and posts solid, and register the record
 * for this step's checks. each panel's shape turns as far as frame has folded
 * the door, each pair folding toward its own side. x is a panel's half width
 * along the doorway, so an outer panel's center lies x in from its hinge, and
 * an inner one's 3x */
static void hotel_door_collide_open(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;
    Angle ang;
    Float x;
    Float z;

    shapes[DOOR_PANEL].flags &= ~SHAPE_OFF;
    shapes[DOOR_PANEL + 1].flags &= ~SHAPE_OFF;
    shapes[DOOR_PANEL + 2].flags &= ~SHAPE_OFF;
    shapes[DOOR_PANEL + 3].flags &= ~SHAPE_OFF;
    shapes[DOOR_SIDE_LEFT].flags &= ~SHAPE_OFF;
    shapes[DOOR_SIDE_RIGHT].flags &= ~SHAPE_OFF;
    shapes[DOOR_WALL].flags |= SHAPE_OFF;
    shapes[DOOR_POST_LEFT].flags &= ~SHAPE_OFF;
    shapes[DOOR_POST_RIGHT].flags &= ~SHAPE_OFF;
    ang = floorf(VARS(work)->frame * DOOR_TURN /
                 (folding_door_action->motion->nbFrame - 1));
    x = njCos(ang) * PANEL_HALF;
    z = njSin(ang) * PANEL_HALF;
    shapes[3].center.x = -(shapes[0].center.x = x - DOOR_HALF);
    shapes[2].center.x = -(shapes[1].center.x = (x * 3.0F) - DOOR_HALF);
    shapes[0].center.z = shapes[1].center.z = shapes[2].center.z =
        shapes[3].center.z = z;
    shapes[2].ang.y = shapes[0].ang.y = -ang;
    shapes[3].ang.y = shapes[1].ang.y = ang;
    collision_register(work);
}

/* make the shut door's wall and posts solid, and register the record for this
 * step's checks */
static void hotel_door_collide_shut(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;

    shapes[DOOR_PANEL].flags |= SHAPE_OFF;
    shapes[DOOR_PANEL + 1].flags |= SHAPE_OFF;
    shapes[DOOR_PANEL + 2].flags |= SHAPE_OFF;
    shapes[DOOR_PANEL + 3].flags |= SHAPE_OFF;
    shapes[DOOR_SIDE_LEFT].flags |= SHAPE_OFF;
    shapes[DOOR_SIDE_RIGHT].flags |= SHAPE_OFF;
    shapes[DOOR_WALL].flags &= ~SHAPE_OFF;
    shapes[DOOR_POST_LEFT].flags &= ~SHAPE_OFF;
    shapes[DOOR_POST_RIGHT].flags &= ~SHAPE_OFF;
    collision_register(work);
}

/* fold the door shut, and open it again while a player is near */
static void hotel_door_close(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: last set and never read, which loads the motion's frame
     * count and converts it ahead of the range check (FLOAT at 0x0c90c7a8);
     * without it, 53 of 54 words differ */
    Float last = folding_door_action->motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    hotel_door_draw(task);
    if (!hotel_door_near(work)) {
        /* @hack spelling: the frame decremented in the test, which SHC tests in
         * FR2 and stores in the slot of the BT/S at 0x0c90c7da. as a statement
         * of its own, SHC reads the frame back to test it, and 29 of 54 words
         * differ */
        if ((VARS(work)->frame -= DOOR_SPEED) <= 0.0F) {
            VARS(work)->frame = 0.0F;
            task->step = hotel_door_idle;
        }
    } else {
        task->step = hotel_door_open;
    }
    hotel_door_collide_open(work);
}

static void hotel_door_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    hotel_door_draw(task);
    if (!--work->tick) {
        task->step = hotel_door_close;
        sound_play(SE_DOOR, 0, 0, 0);
    }
    hotel_door_collide_open(work);
}

static void hotel_door_open(struct task *task)
{
    struct task_work *work = task->work;
    Float last = folding_door_action->motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    hotel_door_draw(task);
    VARS(work)->frame += DOOR_SPEED;
    if (VARS(work)->frame >= last) {
        VARS(work)->frame = last;
        work->tick = DOOR_WAIT;
        task->step = hotel_door_wait;
    }
    hotel_door_collide_open(work);
}

/* wait shut until a player comes near */
static void hotel_door_idle(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        hotel_door_draw(task);
    } else {
        if (placement_try_leave(task)) {
            return;
        }
        hotel_door_draw(task);
        if (hotel_door_near(work)) {
            task->step = hotel_door_open;
            sound_play(SE_DOOR, 0, 0, 0);
        }
        hotel_door_collide_shut(work);
    }
}

/* stay locked until the door's open test passes */
static void hotel_door_locked(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        if (VARS(work)->open_test) {
            Bool (*open_test)(void) = VARS(work)->open_test;

            if (open_test()) {
                task->step = hotel_door_idle;
            }
        }
        hotel_door_collide_shut(work);
    }
    hotel_door_draw(task);
}

/* stay locked until hotel_pool_open passes. unlike hotel_door_locked, this
 * step doesn't call placement_try_leave first */
static void hotel_pool_locked(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        if (hotel_pool_open()) {
            task->step = hotel_door_idle;
        }
        hotel_door_collide_shut(work);
    }
    hotel_door_draw(task);
}

static void hotel_door_destroy(struct task *task)
{
    task_destroy_children(task);
}

void hotel_front_init(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_init(task, hotel_door_shapes, COLLISION_OBJECT, 1,
                       DOOR_SHAPES);
    }
    VARS(work)->frame = 0.0F;
    work->tick = 0;
    VARS(work)->open_test = hotel_front_open;
    if (hotel_front_open()) {
        task->step = hotel_door_idle;
    } else {
        task->step = hotel_door_locked;
    }
    task->draw = hotel_door_draw;
    task->destructor = hotel_door_destroy;
}

void hotel_back_init(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_init(task, hotel_door_shapes, COLLISION_OBJECT, 1,
                       DOOR_SHAPES);
    }
    VARS(work)->frame = 0.0F;
    work->tick = 0;
    VARS(work)->open_test = hotel_back_open;
    if (hotel_back_open()) {
        task->step = hotel_door_idle;
    } else {
        task->step = hotel_door_locked;
    }
    task->draw = hotel_door_draw;
    task->destructor = hotel_door_destroy;
}

void hotel_pool_init(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_init(task, hotel_door_shapes, COLLISION_OBJECT, 1,
                       DOOR_SHAPES);
    }
    VARS(work)->frame = 0.0F;
    work->tick = 0;
    VARS(work)->open_test = hotel_pool_open;
    if (hotel_pool_open()) {
        task->step = hotel_door_idle;
    } else {
        task->step = hotel_pool_locked;
    }
    task->draw = hotel_door_draw;
    task->destructor = hotel_door_destroy;
}
