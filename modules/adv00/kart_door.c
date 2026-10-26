/* O KARTDOOR sliding door to Twinkle Circuit, locked until the story opens
 * it */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/kart_door.h"
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

/* @hack type: the panels' sliding action declared as an array. SHC then
 * pools its address and reads its motion through it, MOV.L @(4,R3),R2 at
 * 0x0c9026ca. declared as one NJS_ACTION, SHC pools the motion field's
 * address, and 5 of kart_door_close's 54 words and 3 of kart_door_opening's 54
 * differ */
extern NJS_ACTION kart_door_action[];

#define DOOR_REACH 20.0F /* a player within this opens the door */
#define DOOR_SPEED 0.2F /* motion frames a step */
/* steps the open door waits before it looks for a player within DOOR_REACH */
#define DOOR_WAIT 90

#define DOOR_HALF 6.875F /* half a panel's width */
#define SE_DOOR 530 /* the door starting to open or shut */

struct kart_door_vars {
    Float frame; /* the door's frame in its motion, 0 shut */
};

#define VARS(work) ((struct kart_door_vars *)&(work)->vars)

enum kart_door_shape {
    DOOR_LEFT,
    DOOR_RIGHT,
    DOOR_WALL, /* a wall across the shut doorway */
    JAMB_RIGHT,
    JAMB_LEFT
};

static Bool kart_door_unlocked(struct task *task);
static void kart_door_draw(struct task *task);
static void kart_door_collide_open(struct task_work *work);
static void kart_door_collide_shut(struct task_work *work);
static void kart_door_close(struct task *task);
static void kart_door_wait(struct task *task);
static void kart_door_opening(struct task *task);
static void kart_door_idle(struct task *task);
static void kart_door_locked(struct task *task);

/* clang-format off */
static struct collision_shape kart_door_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {DOOR_HALF, 15.5F, 0.0F}, DOOR_HALF, 15.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-DOOR_HALF, 15.5F, 0.0F}, DOOR_HALF, 15.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 15.5F, 0.0F}, 2.0F * DOOR_HALF, 15.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {2.0F * DOOR_HALF, 15.5F, 0.0F}, 5.0F, 15.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-2.0F * DOOR_HALF, 15.5F, 0.0F}, 5.0F, 15.5F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static Bool kart_door_unlocked(struct task *task)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_SS_ENTRANCE_CIRCUIT];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_ENTRANCE_CIRCUIT];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_ENTRANCE_CIRCUIT];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_SS_ENTRANCE_CIRCUIT];
    case CHARACTER_E102:
        return event_flags[FLAG_E102_SS_ENTRANCE_CIRCUIT];
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_SS_ENTRANCE_CIRCUIT];
    default:
        return FALSE;
    }
}

static void kart_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_action(kart_door_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* make only the panels solid, and register the record for this step's checks.
 * each panel's center lies x out from the doorway's middle: DOOR_HALF shut,
 * and three times that at the motion's last frame */
static void kart_door_collide_open(struct task_work *work)
{
    Float x = (VARS(work)->frame * (2.0F * DOOR_HALF) /
               (kart_door_action[0].motion->nbFrame - 1)) +
              DOOR_HALF;
    struct collision_shape *shapes = work->collision->shapes;

    shapes[DOOR_LEFT].flags &= ~SHAPE_OFF;
    shapes[DOOR_RIGHT].flags &= ~SHAPE_OFF;
    shapes[DOOR_WALL].flags |= SHAPE_OFF;
    shapes[JAMB_RIGHT].flags |= SHAPE_OFF;
    shapes[JAMB_LEFT].flags |= SHAPE_OFF;
    /* @hack spelling: the store through work->collision, which reloads the
     * shapes' address (0x0c90265e). through shapes, 9 of 56 words differ */
    work->collision->shapes[DOOR_LEFT].center.x = -x;
    /* @hack spelling: the store through work->collision, as above (0x0c902664).
     * through shapes, 31 of 56 words differ */
    work->collision->shapes[DOOR_RIGHT].center.x = x;
    collision_register(work);
}

/* make only the wall and the jambs solid, and register the record for this
 * step's checks */
static void kart_door_collide_shut(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;

    shapes[DOOR_LEFT].flags |= SHAPE_OFF;
    shapes[DOOR_RIGHT].flags |= SHAPE_OFF;
    shapes[DOOR_WALL].flags &= ~SHAPE_OFF;
    shapes[JAMB_RIGHT].flags &= ~SHAPE_OFF;
    shapes[JAMB_LEFT].flags &= ~SHAPE_OFF;
    collision_register(work);
}

/* slide the door shut, and open it again while a player is near */
static void kart_door_close(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: last set and never read, which loads the motion's frame
     * count and converts it ahead of the range check (FLOAT at 0x0c9026d8);
     * without it, 53 of 54 words differ */
    Float last = kart_door_action[0].motion->nbFrame - 1;
    int near;

    if (placement_try_leave(task)) {
        return;
    }
    kart_door_draw(task);
    near = player_in_range(&work->pos, DOOR_REACH);
    if (!near) {
        /* @hack spelling: the frame decremented in the test, which SHC tests in
         * FR2 and stores in the slot of the BT/S at 0x0c90270a. as a statement
         * of its own, SHC reads the frame back to test it, and 29 of 54 words
         * differ */
        if ((VARS(work)->frame -= DOOR_SPEED) <= 0.0F) {
            VARS(work)->frame = 0.0F;
            task->step = kart_door_idle;
        }
    } else {
        task->step = kart_door_opening;
    }
    kart_door_collide_open(work);
}

/* hold the door fully open for DOOR_WAIT steps, and then while a player is
 * near. this step registers no collision record, so no part of the door is
 * solid while it waits */
static void kart_door_wait(struct task *task)
{
    struct task_work *work = task->work;
    int near;

    if (placement_try_leave(task)) {
        return;
    }
    kart_door_draw(task);
    if (!work->tick) {
        near = player_in_range(&work->pos, DOOR_REACH);
        if (near) {
            /* @hack redundant: a return where control returns anyway, which has
             * SHC load kart_door_close's address into R2 and sound_play's into
             * R3 (JMP @R3 at 0x0c9027a0). written as one if (!near) arm, the
             * two swap and 4 of 41 words differ */
            return;
        } else {
            task->step = kart_door_close;
            sound_play(SE_DOOR, 0, 0, 0);
        }
    } else {
        work->tick--;
    }
}

static void kart_door_opening(struct task *task)
{
    struct task_work *work = task->work;
    Float last = kart_door_action[0].motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    kart_door_draw(task);
    VARS(work)->frame += DOOR_SPEED;
    if (VARS(work)->frame >= last) {
        VARS(work)->frame = last;
        work->tick = DOOR_WAIT;
        task->step = kart_door_wait;
    }
    kart_door_collide_open(work);
}

static void kart_door_idle(struct task *task)
{
    struct task_work *work = task->work;
    int near;

    if (placement_editing(task)) {
        kart_door_draw(task);
    } else {
        if (placement_try_leave(task)) {
            return;
        }
        kart_door_draw(task);
        near = player_in_range(&work->pos, DOOR_REACH);
        /* @hack spelling: an empty then-arm, which has SHC load
         * kart_door_opening's address into R2 and sound_play's into R3 (JSR
         * @R3 at 0x0c9028ac). as a plain if (near), the two trade and 4 of 47
         * words differ */
        if (!near) {
        } else {
            task->step = kart_door_opening;
            sound_play(SE_DOOR, 0, 0, 0);
        }
        kart_door_collide_shut(work);
    }
}

static void kart_door_locked(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        if (kart_door_unlocked(task)) {
            task->step = kart_door_idle;
        }
        kart_door_collide_shut(work);
    }
    kart_door_draw(task);
}

void kart_door_init(struct task *task)
{
    if (placement_try_leave(task)) {
        return;
    }
    collision_init(task, kart_door_shapes, COLLISION_OBJECT, 1, 5);
    if (kart_door_unlocked(task)) {
        task->step = kart_door_idle;
    } else {
        task->step = kart_door_locked;
    }
    task->draw = kart_door_draw;
}
