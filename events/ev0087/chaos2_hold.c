/* EV0087's hold on the Chaos 2 boss: a task that places, turns and moves
 * him through boss_work. calls into B_CHAOS2.PRS set his modes */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos2/event_modes.h"
#include "chaos/ribbon.h"
#include "chaos/start.h"
#include "draw/motion.h"
#include "ev0087/chaos2_hold.h"
#include "system/task.h"
#include "system/util.h"

/* the hold task's work->mode: what its step does */
enum hold_mode {
    HOLD_STILL,
    /* add hold->speed to the position each step. the move ends once
     * HOLD(work)->timer passes hold->steps, so it runs hold->steps + 1
     * steps */
    HOLD_MOVE,
    /* turn y toward hold->aim by hold->turn a step, the short way for an aim
     * of 0 to 0x8000. within hold->turn of the aim, take it and start
     * HOLD_MOVE. a negative aim can turn y forever, as the step's @bug says */
    HOLD_TURN
};

/* what the hold keeps besides its task's work. ev0087_chaos2_hold allocates
 * it, unchecked, and the task's destructor frees it */
struct boss_hold {
    NJS_POINT3 pos; /* boss_work's position before the hold */
    struct angles ang; /* boss_work's angles before the hold */
    NJS_VECTOR speed; /* how far a move goes each step */
    Angle turn; /* how far a turn turns y each step */
    Angle aim; /* the y angle a turn ends at, the move's direction */
    Uint32 steps; /* the steps a move divides its distance by */
};

/* the hold task's work->vars */
struct hold_vars {
    Uint32 timer; /* the steps HOLD_MOVE has run */
    Uint8 unk_04[4];
    struct boss_hold *hold;
};

#define HOLD(work) ((struct hold_vars *)&(work)->vars)

static struct task *ev0087_chaos2_task = NULL;

static void ev0087_chaos2_init(struct task *task);
static void ev0087_chaos2_step(struct task *task);
static void ev0087_chaos2_destroy(struct task *task);

void ev0087_chaos2_hold(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                        Angle ang_z, Angle turn)
{
    struct task_work *work;
    struct boss_hold *hold;

    ev0087_chaos2_task = task_create(TASK_WORK, 5, ev0087_chaos2_init);
    hold = heap_alloc(sizeof(struct boss_hold));
    work = ev0087_chaos2_task->work;
    HOLD(work)->hold = hold;
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

void ev0087_chaos2_release(void)
{
    /* @hack redundant: the else after the return, which loads boss_work into R3
     * for pos.x (0x0cb8388e, 0x0cb83890); without it, into R2, and 2 of 47
     * words differ */
    if (ev0087_chaos2_task == NULL) {
        return;
    } else {
        struct task_work *work = ev0087_chaos2_task->work;
        struct boss_hold *hold = HOLD(work)->hold;
        /* @hack redundant: a read of work->ang.y that nothing uses, which keeps
         * work in R5 and loads hold before boss_work's address (0x0cb83884
         * to 0x0cb8388a); without it, work takes R4 and 5 of 47 words
         * differ */
        Angle unused = work->ang.y;

        boss_work->pos.x = hold->pos.x;
        boss_work->pos.y = hold->pos.y;
        boss_work->pos.z = hold->pos.z;
        boss_work->ang.x = hold->ang.x;
        boss_work->ang.y = hold->ang.y;
        boss_work->ang.z = hold->ang.z;
        if (ev0087_chaos2_task != NULL) {
            task_destroy(ev0087_chaos2_task);
            ev0087_chaos2_task = NULL;
        }
        chaos_end_event();
    }
}

void ev0087_chaos2_melt(void)
{
    chaos2_event_melt();
}

void ev0087_chaos2_form(void)
{
    chaos2_event_form();
}

void ev0087_chaos2_action(Float speed)
{
    chaos2_event_action(speed);
}

void ev0087_chaos2_ribbon_rate(Float rate)
{
    ribbon_set_extra_rate(rate);
}

void ev0087_chaos2_move(Float x, Float y, Float z, int steps)
{
    struct task_work *work = ev0087_chaos2_task->work;
    struct boss_hold *hold = HOLD(work)->hold;

    hold->speed.x = (x - boss_work->pos.x) / steps;
    hold->speed.y = (y - boss_work->pos.y) / steps;
    hold->speed.z = (z - boss_work->pos.z) / steps;
    hold->steps = steps;
    hold->aim =
        (Angle)(atan2f(hold->speed.z, hold->speed.x) * ANGLE_PER_RADIAN);
    work->mode = HOLD_TURN;
}

void ev0087_chaos2_set_pos(Float x, Float y, Float z)
{
    struct task_work *work = ev0087_chaos2_task->work;

    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    boss_work->pos.x = x;
    boss_work->pos.y = y;
    boss_work->pos.z = z;
}

void ev0087_chaos2_set_ang(Angle x, Angle y, Angle z)
{
    struct task_work *work = ev0087_chaos2_task->work;

    work->ang.x = x;
    work->ang.y = y;
    work->ang.z = z;
    boss_work->ang.x = x;
    boss_work->ang.y = y;
    boss_work->ang.z = z;
}

static void ev0087_chaos2_init(struct task *task)
{
    task->step = ev0087_chaos2_step;
    task->draw = NULL;
    task->destructor = ev0087_chaos2_destroy;
}

/* turn or move the task by work->mode, and give boss_work each new y angle
 * and position. the turn's wraps set y to 0 or 0x10000 exactly, dropping the
 * overshoot */
static void ev0087_chaos2_step(struct task *task)
{
    struct task_work *work = task->work;
    struct boss_hold *hold = HOLD(work)->hold;

    switch (work->mode) {
    case HOLD_STILL:
        break;
    case HOLD_TURN:
        /* @bug logic: atan2f gives a negative aim for a target at a lower
         * z, and this test sends it to the branch whose wrap keeps y above 0
         * (0x0cb839cc to 0x0cb839d8). from a y outside the half turn below
         * the aim, that branch only lowers y and wraps it to 0x10000 at 0
         * (0x0cb83a92 to 0x0cb83aae). unless the aim is within hold->turn of
         * 0, the turn never ends and the boss never moves. nothing calls
         * ev0087_chaos2_move */
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
            HOLD(work)->timer = 0;
        }
        break;
    case HOLD_MOVE:
        work->pos.x += hold->speed.x;
        work->pos.y += hold->speed.y;
        work->pos.z += hold->speed.z;
        boss_work->pos.x = work->pos.x;
        boss_work->pos.y = work->pos.y;
        boss_work->pos.z = work->pos.z;
        if (++HOLD(work)->timer > hold->steps) {
            work->mode = HOLD_STILL;
        }
        break;
    }
}

static void ev0087_chaos2_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct boss_hold *hold = HOLD(work)->hold;

    if (hold != NULL) {
        heap_free(hold);
    }
}
