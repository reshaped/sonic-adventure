/* O EC DOORS sliding door, and O EC DOOR, the same door with a lintel */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/door.h"
#include "adv0100/elevator.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* door_object's child and that child's sibling are the two panels */
extern NJS_OBJECT door_object;
extern NJS_OBJECT door_lintel_object;

/* the nearest player's distance within which the door opens */
#define DOOR_REACH 50.0F
#define DOOR_ACCEL 0.1F /* the speed the panels gain each step */
#define DOOR_SPEED 2.0F /* the farthest the panels slide in a step */
#define DOOR_TRAVEL 15.0F /* how far each panel slides from shut to open */

#define SE_DOOR 577 /* the panels starting to open or shut */

/* what else the door waits for, besides a near player and no battle. the set
 * entry's z angle gives the lock, and LOCK(work) keeps it */
enum door_lock {
    LOCK_NONE,
    LOCK_SONIC_TAILS, /* the player being Sonic or Tails, outside act 2 */
    LOCK_ELEVATOR_1, /* the elevator standing at floor 1 */
    LOCK_ELEVATOR_0 /* the elevator standing at floor 0 */
};

/* in work->flags */
#define DOOR_SHUT 0x200 /* the panels are shut, and the door's box is solid */
#define DOOR_LINTEL 0x400 /* the door has a lintel, as O EC DOOR's does */

struct door_vars {
    Uint8 unk_00[4];
    /* the set entry's x angle, which DOOR_INIT keeps and nothing reads */
    Angle placed;
    Float travel; /* how far each panel has slid, from 0 to DOOR_TRAVEL */
};

#define VARS(work) ((struct door_vars *)&(work)->vars)
#define LOCK(work) ((work)->submode)
/* the panels' speed, above 0 while they open and below 0 while they shut */
#define SPEED(work) ((work)->scl.z)

enum door_mode { DOOR_INIT, DOOR_WAIT, DOOR_OPEN, DOOR_OPENING, DOOR_SHUTTING };

static void door_draw(struct task *task);
static void door_draw_work(struct task_work *work);
static Bool door_opens(struct task_work *work);

/* the box of a door without a lintel, solid only while the door is shut */
static struct collision_shape door_shapes[] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 20.0F, 0.0F},
     15.0F,
     20.0F,
     5.0F,
     {0},
     0,
     NULL},
};
/* the boxes of a door with a lintel: the whole doorway, solid while the door
 * is shut, then the lintel, solid while it is open */
static struct collision_shape lintel_door_shapes[] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 27.5F, 0.0F},
     15.0F,
     27.5F,
     6.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 47.5F, 0.0F},
     15.0F,
     7.5F,
     6.0F,
     {0},
     0,
     NULL},
};

void egg_carrier_door_step(struct task *task)
{
    struct task_work *work = task->work;

    work->flags |= DOOR_LINTEL;
    task->step = egg_carrier_doors_step;
}

void egg_carrier_doors_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case DOOR_INIT:
        LOCK(work) = work->ang.z;
        work->ang.z = 0;
        if (!placement_editing(task)) {
            if (work->flags & DOOR_LINTEL) {
                collision_init(task, lintel_door_shapes, COLLISION_OBJECT, 1,
                               2);
                work->collision->shapes[1].flags |= SHAPE_OFF;
            } else {
                collision_init(task, door_shapes, COLLISION_OBJECT, 1, 1);
            }
            VARS(work)->placed = work->ang.x;
            work->ang.x = 0;
            work->ang.z = 0;
            SPEED(work) = 0.0F;
            work->flags |= DOOR_SHUT;
        }
        task->draw = door_draw;
        work->mode = DOOR_WAIT;
        break;
    case DOOR_WAIT:
        if (door_opens(work)) {
            work->flags &= ~DOOR_SHUT;
            work->mode = DOOR_OPENING;
            sound_play(SE_DOOR, 0, 0, 0);
        }
        break;
    case DOOR_OPEN:
        /* @hack spelling: door_opens compared with TRUE, CMP/EQ #1 at
         * 0x0c92bada. door_opens returns only TRUE or FALSE, so a plain test of
         * its result is the same; as one, 2 of 254 words differ */
        if (door_opens(work) == TRUE) {
            break;
        }
        work->mode = DOOR_SHUTTING;
        sound_play(SE_DOOR, 0, 0, 0);
        break;
    case DOOR_OPENING:
        SPEED(work) += DOOR_ACCEL;
        if (SPEED(work) > DOOR_SPEED) {
            SPEED(work) = DOOR_SPEED;
        }
        VARS(work)->travel += SPEED(work);
        if (VARS(work)->travel < DOOR_TRAVEL) {
            break;
        }
        work->mode = DOOR_OPEN;
        SPEED(work) = 0.0F;
        VARS(work)->travel = DOOR_TRAVEL;
        break;
    case DOOR_SHUTTING:
        /* @hack spelling: -DOOR_ACCEL added, a constant of its own, which SHC
         * loads through R1 and FPUL (0x0c92bb64) as DOOR_OPENING loads 0.1
         * (0x0c92bb20); with -=, SHC loads 0.1 once into FR4 ahead of the
         * switch, the function runs 32 bytes short and 185 of 254 words
         * differ */
        SPEED(work) += -DOOR_ACCEL;
        if (SPEED(work) < -DOOR_SPEED) {
            SPEED(work) = -DOOR_SPEED;
        }
        VARS(work)->travel += SPEED(work);
        if (VARS(work)->travel <= 0.0F) {
            work->flags |= DOOR_SHUT;
            work->mode = DOOR_WAIT;
            VARS(work)->travel = SPEED(work) = 0.0F;
        }
        break;
    }
    door_draw_work(work);
    if (!placement_editing(task)) {
        if (work->flags & DOOR_LINTEL) {
            collision_register(work);
            if (work->flags & DOOR_SHUT) {
                work->collision->shapes[0].flags &= ~SHAPE_OFF;
                work->collision->shapes[1].flags |= SHAPE_OFF;
            } else {
                work->collision->shapes[0].flags |= SHAPE_OFF;
                work->collision->shapes[1].flags &= ~SHAPE_OFF;
            }
        } else if (work->flags & DOOR_SHUT) {
            collision_register(work);
        }
    }
}

static void door_draw(struct task *task)
{
    struct task_work *work = task->work;

    door_draw_work(work);
}

static void door_draw_work(struct task_work *work)
{
    NJS_OBJECT *object;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    texture_set_stage();
    njPushMatrix(NULL);
    object = &door_object;
    draw_model(object->model);
    object = object->child;
    njTranslate(NULL, -VARS(work)->travel, 0.0F, 0.0F);
    draw_model(object->model);
    object = object->sibling;
    njTranslate(NULL, 2.0F * VARS(work)->travel, 0.0F, 0.0F);
    draw_model(object->model);
    njPopMatrix(1);
    if (work->flags & DOOR_LINTEL) {
        draw_tree(&door_lintel_object);
    }
    njPopMatrix(1);
}

/* TRUE while the nearest player is within DOOR_REACH, no battle is on, and
 * the door's lock lets that player through; else FALSE */
static Bool door_opens(struct task_work *work)
{
    int player = player_nearest(&work->pos);
    int character;

    if (njDistanceP2P(&player_work[player]->pos, &work->pos) > DOOR_REACH) {
        return FALSE;
    }
    if (battle_rival != -1) {
        return FALSE;
    }
    character = player_get_character(player);
    switch (LOCK(work)) {
    case LOCK_NONE:
        break;
    case LOCK_SONIC_TAILS:
        if ((Uint8)stage_get_stage_act() == 2) {
            return FALSE;
        }
        return character == CHARACTER_SONIC || character == CHARACTER_TAILS;
    case LOCK_ELEVATOR_1:
        if (event_flags[FLAG_ELEVATOR_AT_1] != 1) {
            return FALSE;
        }
        break;
    case LOCK_ELEVATOR_0:
        if (event_flags[FLAG_ELEVATOR_AT_0] != 1) {
            return FALSE;
        }
        break;
    }
    return TRUE;
}
