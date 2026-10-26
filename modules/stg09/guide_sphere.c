/* O LETSGO spheres: entering one runs player 0 to the sphere with the next
 * ID */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg09/guide_sphere.h"
#include "stg09/helpers.h"
#include "system/task.h"

/* destroyed beyond about 600 units of the camera and player 0 */
#define RANGE ((600.0F * 600.0F) + 10.0F)

struct guide_sphere_vars {
    struct task *prev; /* the newer sphere in guide_sphere_list, or null */
    struct task *next; /* the older, or null */
};

#define VARS(work) ((struct guide_sphere_vars *)&(work)->vars)
#define PLAYER(work) ((work)->submode) /* always player 0 */
#define RADIUS(work) ((work)->scl.x)
#define ID(work) ((work)->scl.y)
#define MODE(work) ((Sint16)(work)->scl.z)

/* a bit of MODE. a sphere with it set starts no run, though a run can go on
 * through it */
#define TARGET_ONLY 0x1

/* the launch_steps a run gives player 0 as it leaves each sphere */
#define RUN_LOCK 60

/* the sphere the run under way left from, or null with no run */
static struct task *guide_sphere_running = NULL;
/* the spheres past their first step and not yet destroyed, newest first */
static struct task *guide_sphere_list = NULL;

static void guide_sphere_destroy(struct task *task);
static void guide_sphere_run(struct task *task);

void guide_sphere_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    task->step = guide_sphere_run;
    task->destructor = guide_sphere_destroy;
    PLAYER(work) = 0;
    VARS(work)->next = guide_sphere_list;
    VARS(work)->prev = NULL;
    if (guide_sphere_list) {
        VARS(guide_sphere_list->work)->prev = task;
    }
    guide_sphere_list = task;
}

static void guide_sphere_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (guide_sphere_running == task) {
        player_event_end(PLAYER(work));
        guide_sphere_running = NULL;
    }
    if (VARS(work)->next) {
        VARS(VARS(work)->next->work)->prev = VARS(work)->prev;
    }
    if (VARS(work)->prev) {
        VARS(VARS(work)->prev->work)->next = VARS(work)->next;
    } else {
        guide_sphere_list = VARS(work)->next;
    }
}

/* the newest sphere other than task whose ID, as a Sint8, is id + 1, or
 * null */
static struct task *guide_sphere_find_next(struct task *task, Sint8 id)
{
    struct task *other = guide_sphere_list;
    struct task_work *work;

    id++;
    while (other) {
        work = other->work;
        if (other != task && (Sint8)ID(work) == id) {
            return other;
        }
        other = VARS(work)->next;
    }
    return NULL;
}

static void guide_sphere_run(struct task *task)
{
    struct task_work *work = task->work;
    struct player *player;
    struct task *next;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> col scl ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> ID");
        if (MODE(work) & TARGET_ONLY) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z),
                     "-> MODE:RENDO ONLY");
        } else {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> MODE:NORMAL");
        }
        casino_draw_sphere(&work->pos,
                           RADIUS(work) > 0.0F ? RADIUS(work) : 0.0F);
        return;
    }
    if (RADIUS(work) >
        njDistanceP2P(&work->pos, &player_work[PLAYER(work)]->pos)) {
        player = player_get(PLAYER(work));
        next = guide_sphere_find_next(task, ID(work));
        if (!guide_sphere_running) {
            if (next && !(MODE(work) & TARGET_ONLY)) {
                player->launch_steps = RUN_LOCK;
                player_event_run(PLAYER(work), next->work->pos.x,
                                 next->work->pos.z);
                guide_sphere_running = task;
            }
        } else if (guide_sphere_running != task) {
            if (guide_sphere_running->placement) {
                placement_kill(guide_sphere_running);
            } else {
                task_kill(guide_sphere_running);
            }
            if (next) {
                player->launch_steps = RUN_LOCK;
                player_event_run(PLAYER(work), next->work->pos.x,
                                 next->work->pos.z);
                guide_sphere_running = task;
            } else {
                player_event_end(PLAYER(work));
                if (task->placement) {
                    placement_kill(task);
                } else {
                    task_kill(task);
                }
                guide_sphere_running = NULL;
            }
        }
    }
}
