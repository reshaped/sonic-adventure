/* O TWINDOOR sliding door into Twinkle Park, which opens only for Sonic, Amy
 * and Big */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/twinkle_door.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: the action declared as an array, so SHC pools its address
 * and reads motion through it (MOV.L @(4,R3),R2 at 0x0c90534a); declared
 * as one, summed over the four functions that read it, 28 of 197 words
 * differ */
extern NJS_ACTION twinkle_door_action[];

#define DOOR_REACH 15.0F
#define DOOR_SPEED 0.2F /* the action frames the door plays each step */
#define DOOR_WAIT 90 /* steps the door waits open before it may close */

#define DOOR_HALF 6.875F /* half a panel's width */
#define SE_DOOR 530

/* the door's vars. work->tick counts down DOOR_WAIT */
struct twinkle_door_vars {
    Float frame; /* the action's frame, 0 shut */
};

#define VARS(work) ((struct twinkle_door_vars *)&(work)->vars)

/* twinkle_door_shapes' rows: the two panels, which slide, and the shut
 * doorway and its jambs, which stand in for the panels while the door is
 * shut */
enum twinkle_door_shape {
    DOOR_LEFT,
    DOOR_RIGHT,
    DOOR_SHUT,
    JAMB_RIGHT,
    JAMB_LEFT
};

static Bool twinkle_door_unlocked(struct task *task);
static void twinkle_door_draw(struct task *task);
static void twinkle_door_slide(struct task_work *work);
static void twinkle_door_shut(struct task_work *work);
static void twinkle_door_close(struct task *task);
static void twinkle_door_wait(struct task *task);
static void twinkle_door_opening(struct task *task);
static void twinkle_door_idle(struct task *task);
static void twinkle_door_locked(struct task *task);

/* clang-format off */
static struct collision_shape twinkle_door_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 15.5F, DOOR_HALF}, 0.5F, 15.5F, DOOR_HALF, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 15.5F, -DOOR_HALF}, 0.5F, 15.5F, DOOR_HALF, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 15.5F, 0.0F}, 0.5F, 15.5F, 2.0F * DOOR_HALF, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 15.5F, 2.0F * DOOR_HALF}, 5.0F, 15.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 15.5F, -2.0F * DOOR_HALF}, 5.0F, 15.5F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* TRUE when the door opens for player 0's character: Sonic, Amy or Big */
static Bool twinkle_door_unlocked(struct task *task)
{
    /* @hack redundant: the cases of Tails, Knuckles and E-102, which return
     * FALSE as the default does, and keep their CMP/EQ #2, #3 and #6
     * (0x0c9051ec to 0x0c9051fa); without them, 18 of 23 words differ */
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return TRUE;
    case CHARACTER_TAILS:
    case CHARACTER_KNUCKLES:
        return FALSE;
    case CHARACTER_AMY:
        return TRUE;
    case CHARACTER_E102:
        return FALSE;
    case CHARACTER_BIG:
        return TRUE;
    default:
        return FALSE;
    }
}

static void twinkle_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(twinkle_door_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* slide the panels' boxes apart with the action's frame, and register them
 * in place of the shut doorway's. their centers move from DOOR_HALF either
 * side when shut to 3 * DOOR_HALF when open */
static void twinkle_door_slide(struct task_work *work)
{
    Float z = (VARS(work)->frame * (2.0F * DOOR_HALF) /
               (twinkle_door_action[0].motion->nbFrame - 1)) +
              DOOR_HALF;
    struct collision_shape *shapes = work->collision->shapes;

    shapes[DOOR_LEFT].flags &= ~SHAPE_OFF;
    shapes[DOOR_RIGHT].flags &= ~SHAPE_OFF;
    shapes[DOOR_SHUT].flags |= SHAPE_OFF;
    shapes[JAMB_RIGHT].flags |= SHAPE_OFF;
    shapes[JAMB_LEFT].flags |= SHAPE_OFF;
    /* @hack spelling: the centers stored through work->collision->shapes, which
     * SHC reads again for each (0x0c9052de and 0x0c9052e4); through shapes,
     * 34 of 56 words differ */
    work->collision->shapes[DOOR_LEFT].center.z = -z;
    work->collision->shapes[DOOR_RIGHT].center.z = z;
    collision_register(work);
}

static void twinkle_door_shut(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;

    shapes[DOOR_LEFT].flags |= SHAPE_OFF;
    shapes[DOOR_RIGHT].flags |= SHAPE_OFF;
    shapes[DOOR_SHUT].flags &= ~SHAPE_OFF;
    shapes[JAMB_RIGHT].flags &= ~SHAPE_OFF;
    shapes[JAMB_LEFT].flags &= ~SHAPE_OFF;
    collision_register(work);
}

/* which of players 0 and 1, counted from 1, is within DOOR_REACH of the
 * door: 0 for neither */
static int twinkle_door_near(struct task_work *work)
{
    return player_in_range(&work->pos, DOOR_REACH);
}

static void twinkle_door_close(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: last, the action's last frame, which the image
     * computes at the start (0x0c905340 to 0x0c905358) and never reads;
     * without it, 53 of 54 words differ */
    Float last = twinkle_door_action[0].motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    twinkle_door_draw(task);
    if (!twinkle_door_near(work)) {
        if ((VARS(work)->frame -= DOOR_SPEED) <= 0.0F) {
            VARS(work)->frame = 0.0F;
            task->step = twinkle_door_idle;
        }
    } else {
        task->step = twinkle_door_opening;
    }
    twinkle_door_slide(work);
}

/* the door registers no collision while it waits */
static void twinkle_door_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    twinkle_door_draw(task);
    if (!work->tick) {
        if (!twinkle_door_near(work)) {
            task->step = twinkle_door_close;
            sound_play(SE_DOOR, 0, 0, 0);
        }
    } else {
        work->tick--;
    }
}

static void twinkle_door_opening(struct task *task)
{
    struct task_work *work = task->work;
    Float last = twinkle_door_action[0].motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    twinkle_door_draw(task);
    VARS(work)->frame += DOOR_SPEED;
    if (VARS(work)->frame >= last) {
        VARS(work)->frame = last;
        work->tick = DOOR_WAIT;
        task->step = twinkle_door_wait;
    }
    twinkle_door_slide(work);
}

static void twinkle_door_idle(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        twinkle_door_draw(task);
    } else {
        if (placement_try_leave(task)) {
            return;
        }
        twinkle_door_draw(task);
        if (twinkle_door_near(work)) {
            task->step = twinkle_door_opening;
            sound_play(SE_DOOR, 0, 0, 0);
        }
        twinkle_door_shut(work);
    }
}

static void twinkle_door_locked(struct task *task)
{
    if (placement_try_leave(task)) {
        return;
    }
    twinkle_door_draw(task);
    twinkle_door_shut(task->work);
}

void twinkle_door_init(struct task *task)
{
    if (placement_try_leave(task)) {
        return;
    }
    collision_init(task, twinkle_door_shapes, COLLISION_OBJECT, 1, 5);
    if (twinkle_door_unlocked(task)) {
        task->step = twinkle_door_idle;
    } else {
        task->step = twinkle_door_locked;
    }
    task->draw = twinkle_door_draw;
}
