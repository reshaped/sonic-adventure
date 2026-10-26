/* an event's hold on the boss Chaos 7 */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos7/event_modes.h"
#include "chaos/start.h"
#include "draw/motion.h"
#include "shared/chaos7.h"
#include "system/task.h"
#include "system/util.h"

/* what the hold's step does, by work->mode */
enum hold_mode {
    HOLD_STILL, /* nothing */
    HOLD_MOVE, /* it moves the boss by speed */
    HOLD_TURN /* it turns the boss toward aim, then sets HOLD_MOVE */
};

/* the hold's record, which event_chaos7_hold allocates and its destructor
 * frees */
struct boss_hold {
    NJS_POINT3 pos; /* where the boss stood before the hold */
    struct angles ang; /* how the boss faced before the hold */
    NJS_VECTOR speed; /* how far the move takes the boss each step */
    Angle turn; /* how far the turn turns him each step */
    Angle aim; /* the y angle the turn ends at, from -0x8000 to 0x8000 */
    Uint32 steps; /* the steps speed divides the move's way into */
};

struct hold_vars {
    Uint32 timer; /* steps into the move */
    Uint8 unk_04[4];
    struct boss_hold *hold;
};

#define VARS(work) ((struct hold_vars *)&(work)->vars)

/* the last hold created, until release clears it */
static struct task *event_chaos7_task = NULL;

static void event_chaos7_init(struct task *task);
static void event_chaos7_step(struct task *task);
static void event_chaos7_destroy(struct task *task);

void event_chaos7_hold(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                       Angle ang_z, Angle turn)
{
    struct task_work *work;
    struct boss_hold *hold;

    event_chaos7_task = task_create(TASK_WORK, 5, event_chaos7_init);
    hold = heap_alloc(sizeof(struct boss_hold));
    work = event_chaos7_task->work;
    VARS(work)->hold = hold;
    work->mode = HOLD_STILL;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang.x = ang_x;
    work->ang.y = ang_y;
    work->ang.z = ang_z;
    hold->pos.x = boss_work->pos.x;
    hold->pos.y = boss_work->pos.y;
    hold->pos.z = boss_work->pos.z;
    hold->ang.x = boss_work->ang.x;
    hold->ang.y = boss_work->ang.y;
    hold->ang.z = boss_work->ang.z;
    hold->turn = turn;
    boss_work->pos.x = x;
    boss_work->pos.y = y;
    boss_work->pos.z = z;
    boss_work->ang.x = ang_x;
    boss_work->ang.y = ang_y;
    boss_work->ang.z = ang_z;
}

void event_chaos7_release(Bool fight)
{
    /* @hack spelling: the return, and the else around the rest, keep
     * boss_work's first load in R3 (MOV.L @R5,R3 at 0x0cb8754e in EV00FD). as
     * an if (event_chaos7_task != NULL) around the rest, the load takes R2 and
     * 2 of 54 words differ */
    if (event_chaos7_task == NULL) {
        return;
    } else {
        struct task_work *work = event_chaos7_task->work;
        struct boss_hold *hold = VARS(work)->hold;
        /* @hack redundant: a read of work->ang.y that makes no instruction, but
         * keeps work in R5 (MOV.L @(32,R4),R5 at 0x0cb87544 in EV00FD);
         * without it, 5 of 54 words differ */
        Angle unused = work->ang.y;

        boss_work->pos.x = hold->pos.x;
        boss_work->pos.y = hold->pos.y;
        boss_work->pos.z = hold->pos.z;
        boss_work->ang.x = hold->ang.x;
        boss_work->ang.y = hold->ang.y;
        boss_work->ang.z = hold->ang.z;
        if (event_chaos7_task != NULL) {
            task_destroy(event_chaos7_task);
            event_chaos7_task = NULL;
        }
        if (fight) {
            chaos_end_event();
        }
    }
}

void event_chaos7_mode_1(void)
{
    chaos7_event_mode_1();
}

void event_chaos7_mode_2(void)
{
    chaos7_event_mode_2();
}

void event_chaos7_mode_13(void)
{
    chaos7_event_mode_13();
}

void event_chaos7_mode_14(void)
{
    chaos7_event_mode_14();
}

void event_chaos7_ray(Float length)
{
    chaos7_event_ray(length);
}

void event_chaos7_move(Float x, Float y, Float z, int steps)
{
    struct task_work *work = event_chaos7_task->work;
    struct boss_hold *hold = VARS(work)->hold;

    hold->speed.x = (x - boss_work->pos.x) / steps;
    hold->speed.y = (y - boss_work->pos.y) / steps;
    hold->speed.z = (z - boss_work->pos.z) / steps;
    hold->steps = steps;
    hold->aim =
        (Angle)(atan2f(hold->speed.z, hold->speed.x) * ANGLE_PER_RADIAN);
    work->mode = HOLD_TURN;
}

void event_chaos7_set_pos(Float x, Float y, Float z)
{
    struct task_work *work = event_chaos7_task->work;

    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    boss_work->pos.x = x;
    boss_work->pos.y = y;
    boss_work->pos.z = z;
}

void event_chaos7_set_ang(Angle x, Angle y, Angle z)
{
    struct task_work *work = event_chaos7_task->work;

    work->ang.x = x;
    work->ang.y = y;
    work->ang.z = z;
    boss_work->ang.x = x;
    boss_work->ang.y = y;
    boss_work->ang.z = z;
}

static void event_chaos7_init(struct task *task)
{
    task->step = event_chaos7_step;
    task->draw = NULL;
    task->destructor = event_chaos7_destroy;
}

/* the boss takes each place and angle the hold takes. HOLD_TURN ends once ang.y
 * is within turn of aim. going up, ang.y becomes 0 as it reaches 0x10000, and
 * going down, 0x10000 as it reaches 0. starting from 0 to 0x10000, ang.y turns
 * the short way to an aim from 0 to 0x8000 */
static void event_chaos7_step(struct task *task)
{
    struct task_work *work = task->work;
    struct boss_hold *hold = VARS(work)->hold;

    switch (work->mode) {
    case HOLD_STILL:
        break;
    case HOLD_TURN:
        if (hold->aim >= 0 && hold->aim < 0x8000) {
            if (work->ang.y <= hold->aim + 0x8000 && work->ang.y >= hold->aim) {
                work->ang.y -= hold->turn;
                boss_work->ang.y = work->ang.y;
            } else {
                work->ang.y += hold->turn;
                boss_work->ang.y = work->ang.y;
                if (work->ang.y >= 0x10000) {
                    work->ang.y = 0;
                    boss_work->ang.y = work->ang.y;
                }
            }
        } else {
            if (work->ang.y >= hold->aim - 0x8000 && work->ang.y <= hold->aim) {
                work->ang.y += hold->turn;
                boss_work->ang.y = work->ang.y;
            } else {
                work->ang.y -= hold->turn;
                boss_work->ang.y = work->ang.y;
                /* @bug logic: a turn toward a point of lower z can run forever.
                 * atan2f gives such a point an aim below 0, but this wrap keeps
                 * ang.y from 0 to 0x10000 (CMP/PL at 0x0cb83f6e in EV00FF). the
                 * turn then ends only if a step lands ang.y above 0 and at most
                 * aim + turn, unless ang.y started from aim - 0x8000 to aim and
                 * turned up. no event calls event_chaos7_move, which starts the
                 * turn */
                if (work->ang.y <= 0) {
                    work->ang.y = 0x10000;
                    boss_work->ang.y = work->ang.y;
                }
            }
        }
        if (work->ang.y <= hold->aim + hold->turn &&
            work->ang.y >= hold->aim - hold->turn) {
            work->ang.y = hold->aim;
            boss_work->ang.y = work->ang.y;
            work->mode = HOLD_MOVE;
            VARS(work)->timer = 0;
        }
        break;
    case HOLD_MOVE:
        work->pos.x += hold->speed.x;
        work->pos.y += hold->speed.y;
        work->pos.z += hold->speed.z;
        boss_work->pos.x = work->pos.x;
        boss_work->pos.y = work->pos.y;
        boss_work->pos.z = work->pos.z;
        if (++VARS(work)->timer > hold->steps) {
            work->mode = HOLD_STILL;
        }
        break;
    }
}

static void event_chaos7_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct boss_hold *hold = VARS(work)->hold;

    if (hold != NULL) {
        heap_free(hold);
    }
}
