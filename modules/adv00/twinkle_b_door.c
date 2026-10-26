/* O TWBDOOR second Twinkle Park sliding door, which opens while a player is
 * near */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/twinkle_b_door.h"
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
 * and reads motion through it (MOV.L @(4,R3),R2 at 0x0c905fb0); declared
 * as one, summed over the four functions that read it, 25 of 175 words
 * differ */
extern NJS_ACTION twinkle_b_door_action[];

#define DOOR_REACH 20.0F
#define DOOR_SPEED 0.2F /* the action frames the door plays each step */
#define DOOR_WAIT 90 /* steps the door waits open before it may close */

#define DOOR_HALF 6.875F /* half a panel's width */
#define SE_DOOR 530

/* the door's vars. work->tick counts down DOOR_WAIT */
struct twinkle_b_door_vars {
    Float frame; /* the action's frame, 0 shut */
};

#define VARS(work) ((struct twinkle_b_door_vars *)&(work)->vars)

static void twinkle_b_door_close(struct task *task);
static void twinkle_b_door_wait(struct task *task);
static void twinkle_b_door_open(struct task *task);
static void twinkle_b_door_idle(struct task *task);

/* clang-format off */
static struct collision_shape twinkle_b_door_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-0.55F, 15.5F, -DOOR_HALF}, 1.0F, 15.5F, DOOR_HALF, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-0.55F, 15.5F, DOOR_HALF}, 1.0F, 15.5F, DOOR_HALF, {0}, 0, NULL},
};
/* clang-format on */

static void twinkle_b_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(twinkle_b_door_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* slide the panels' boxes apart with the action's frame, and register
 * them. their centers move from DOOR_HALF either side when shut to
 * 3 * DOOR_HALF when open */
static void twinkle_b_door_slide(struct task_work *work)
{
    Float z = (VARS(work)->frame * (2.0F * DOOR_HALF) /
               (twinkle_b_door_action->motion->nbFrame - 1)) +
              DOOR_HALF;

    work->collision->shapes[0].center.z = -z;
    work->collision->shapes[1].center.z = z;
    collision_register(work);
}

/* which of players 0 and 1, counted from 1, is within DOOR_REACH of the
 * door: 0 for neither */
static int twinkle_b_door_near(struct task_work *work)
{
    return player_in_range(&work->pos, DOOR_REACH);
}

static void twinkle_b_door_close(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: last, the action's last frame, which the image
     * computes at the start (0x0c905fa6 to 0x0c905fbe) and never reads;
     * without it, 53 of 54 words differ */
    Float last = twinkle_b_door_action->motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    twinkle_b_door_draw(task);
    if (!twinkle_b_door_near(work)) {
        if ((VARS(work)->frame -= DOOR_SPEED) <= 0.0F) {
            VARS(work)->frame = 0.0F;
            task->step = twinkle_b_door_idle;
        }
    } else {
        task->step = twinkle_b_door_open;
    }
    twinkle_b_door_slide(work);
}

/* the door registers no collision while it waits */
static void twinkle_b_door_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    twinkle_b_door_draw(task);
    if (!work->tick) {
        if (!twinkle_b_door_near(work)) {
            task->step = twinkle_b_door_close;
            sound_play(SE_DOOR, 0, 0, 0);
        }
    } else {
        work->tick--;
    }
    twinkle_b_door_slide(work);
}

static void twinkle_b_door_open(struct task *task)
{
    struct task_work *work = task->work;
    Float last = twinkle_b_door_action->motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    twinkle_b_door_draw(task);
    VARS(work)->frame += DOOR_SPEED;
    if (VARS(work)->frame >= last) {
        VARS(work)->frame = last;
        work->tick = DOOR_WAIT;
        task->step = twinkle_b_door_wait;
    }
    twinkle_b_door_slide(work);
}

static void twinkle_b_door_idle(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        twinkle_b_door_draw(task);
    } else {
        if (placement_try_leave(task)) {
            return;
        }
        twinkle_b_door_draw(task);
        if (twinkle_b_door_near(work)) {
            task->step = twinkle_b_door_open;
            sound_play(SE_DOOR, 0, 0, 0);
        }
        twinkle_b_door_slide(work);
    }
}

void twinkle_b_door_init(struct task *task)
{
    if (placement_try_leave(task)) {
        return;
    }
    collision_init(task, twinkle_b_door_shapes, COLLISION_OBJECT, 1, 2);
    task->step = twinkle_b_door_idle;
    task->draw = twinkle_b_door_draw;
}
