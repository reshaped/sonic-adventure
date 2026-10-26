/* Perfect Chaos's run paths, which carry player 0 up to ram the boss */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos7/fight.h"
#include "b_chaos7/run.h"
#include "collision/polygon.h"
#include "path/path.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/task.h"

/* a run counts the steps it is open while it doesn't hold player 0. it can
 * take him once the count reaches RUN_WAIT. the count stops there, and the run
 * never resets it, so it waits only once */
#define RUN_WAIT 300
/* a run takes player 0 within 300 of its path's first point. the reach is
 * squared, as polygon_point_distance2 gives the distance */
#define RUN_REACH (300.0F * 300.0F)
/* a run lets player 0 go once he is at RUN_TOP or above */
#define RUN_TOP 600.0F

/* in work->flags, set while chaos7_run_open is TRUE for the run's place */
#define RUN_OPEN 0x100
/* in work->tick, set while the run holds player 0 */
#define RUN_HOLDING 1

enum run_mode { RUN_INIT, RUN_RUN };

/* the three runs' paths, in chaos7_paths's order */
static struct path_point run_2_points[50] = {
#include "b_chaos7/run/run_2_points.inc"
};

struct path run_2_path = {PATH_STOP, 50, 2013.3344F, run_2_points,
                          chaos7_run_2_step};

static struct path_point run_0_points[50] = {
#include "b_chaos7/run/run_0_points.inc"
};

struct path run_0_path = {PATH_STOP, 50, 2013.3338F, run_0_points,
                          chaos7_run_0_step};

static struct path_point run_1_points[50] = {
#include "b_chaos7/run/run_1_points.inc"
};

struct path run_1_path = {PATH_STOP, 50, 2013.3343F, run_1_points,
                          chaos7_run_1_step};

/* outside act 0, end the task. otherwise, take player 0 onto the path when he
 * comes within RUN_REACH of its start. the run must be open, and its count
 * must have reached RUN_WAIT. let him go when he leaves the path or reaches
 * RUN_TOP. tell the fight as the run takes him and as it lets him go */
static void run_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_POINT3 start;
    Float reach = RUN_REACH;
    Float distance;

    switch (work->mode) {
    case RUN_INIT:
        work->mode = RUN_RUN;
        break;
    case RUN_RUN:
        if (stage_get_stage_act() != STAGE_ACT(STAGE_CHAOS7, 0)) {
            task_kill(task);
            break;
        }
        if (player == NULL) {
            break;
        }
        if (!(work->tick & RUN_HOLDING)) {
            if (!(work->flags & RUN_OPEN)) {
                break;
            }
            if (++PATH_VARS(work)->wait[0] < RUN_WAIT) {
                break;
            }
            PATH_VARS(work)->wait[0] = RUN_WAIT;
            start.x = PATH_VARS(work)->path->points[0].pos.x;
            start.y = PATH_VARS(work)->path->points[0].pos.y;
            start.z = PATH_VARS(work)->path->points[0].pos.z;
            distance = polygon_point_distance2(&start, &player->pos);
            if (distance < RUN_REACH) {
                player_run_path(0, PATH_VARS(work)->path);
                work->tick |= RUN_HOLDING;
                chaos7_run_start();
            }
            /* @hack redundant: a test against reach, a copy of RUN_REACH on the
             * stack, whose branch is empty (FCMP/GT FR15,FR3 at 0x0c91b1d8).
             * with neither the test nor reach, 119 of 122 words differ */
            if (distance < reach) {
            }
        } else {
            if (player->pos.y >= RUN_TOP) {
                player->flags &= ~PLAYER_ON_PATH;
            }
            if (!(player->flags & PLAYER_ON_PATH)) {
                work->tick &= ~RUN_HOLDING;
                chaos7_run_end();
            }
        }
        break;
    }
}

void chaos7_run_0_step(struct task *task)
{
    struct task_work *work = task->work;

    if (chaos7_run_open(0)) {
        work->flags |= RUN_OPEN;
    } else {
        work->flags &= ~RUN_OPEN;
    }
    run_step(task);
}

void chaos7_run_1_step(struct task *task)
{
    struct task_work *work = task->work;

    if (chaos7_run_open(1)) {
        work->flags |= RUN_OPEN;
    } else {
        work->flags &= ~RUN_OPEN;
    }
    run_step(task);
}

void chaos7_run_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (chaos7_run_open(2)) {
        work->flags |= RUN_OPEN;
    } else {
        work->flags &= ~RUN_OPEN;
    }
    run_step(task);
}
