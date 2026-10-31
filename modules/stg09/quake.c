/* a quake: the camera bobs like a settling spring, and pad 0 vibrates */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "player/player.h"
#include "stg09/quake.h"
#include "system/task.h"
#include "vmu/vibration.h"

/* the spring, in work->scl. the camera's offset in y is the stretch times
 * the strength */
#define STRETCH(work) ((work)->scl.x)
#define SPEED(work) ((work)->scl.y)
#define STRENGTH(work) ((work)->scl.z)

/* the share of its speed the spring keeps each step */
static Float quake_keep = 0.86F;
/* each step takes the stretch times the pull from the spring's speed. the
 * pull is quake_pull_above while the stretch is 0 or more, and
 * quake_pull_below while it is less */
static Float quake_pull_above = 0.75F;
static Float quake_pull_below = 0.87F;

static void quake_run(struct task *task);

static void quake_step(struct task *task)
{
    task->step = quake_run;
}

/* the quake's step after the first. it ends the quake once the camera's
 * offset and the strength are both less than 0.5 in size. the strength never
 * changes, so a quake whose strength is 0.5 or more never ends */
static void quake_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR offset;

    STRETCH(work) += SPEED(work);
    SPEED(work) -= STRETCH(work) >= 0.0F ? STRETCH(work) * quake_pull_above
                                         : STRETCH(work) * quake_pull_below;
    SPEED(work) *= quake_keep;
    offset.x = 0.0F;
    offset.y = STRETCH(work) * STRENGTH(work);
    offset.z = 0.0F;
    camera_add_offset(&offset);
    if (_builtin_fabsf(offset.y) < 0.5F &&
        _builtin_fabsf(STRENGTH(work)) < 0.5F) {
        task_kill(task);
    }
}

struct task *quake_start(NJS_POINT3 *pos, Float radius, Float speed)
{
    struct task_work *player = player_work[0];
    struct task *task;
    struct task_work *work;
    Float power;
    Float distance = njDistanceP2P(pos, &player->pos);

    if (distance < radius) {
        task = task_create(TASK_WORK, 3, quake_step);
        if (task) {
            work = task->work;
            STRETCH(work) = 0.0F;
            SPEED(work) = speed;
            /* @bug uninitialized: no path sets power before the strength
             * divides it by radius. SHC reads it from FR15 as the caller left
             * it (FMOV FR15,FR2 at 0x0c915a38). lion_slam, the one caller,
             * calls this at 0x0c910804 before it writes FR15. FR15 then holds
             * the 0.0F that lion_run loads (FLDI0 at 0x0c910d0e) before both
             * its calls of lion_slam. every quake's strength is 1.0F, and no
             * quake ends */
#ifdef GCC
            /* keeps the shipped strength the tag names: power is lion_run's
             * 0.0F */
            power = 0.0F;
#endif
            /* NOLINTNEXTLINE(clang-diagnostic-uninitialized) */
            STRENGTH(work) = 1.0F - (power / radius);
            power = 1.0F + (SPEED(work) * STRENGTH(work) * 2.0F);
            if (power > 6.0F) {
                power = 6.0F;
            }
            vibration_start(0, (int)power, 15, 0);
            return task;
        }
    }
    return NULL;
}
