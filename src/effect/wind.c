/* the wind: one task that blows across the ground in gusts */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/wind.h"
#include "system/task.h"
#include "system/util.h"

/* a gust is never more than this share of the wind's strength */
#define GUST_MAX 0.7F
/* the phase's pace, how far it moves a step, is PACE_MAX less a random
 * whole number below PACE_SPREAD: 73 to 364 */
#define PACE_MAX 364
#define PACE_SPREAD 292.0F

static void wind_destroy(struct task *task);
static void wind_step(struct task *task);

/* the wind's task, or null while no wind blows. its work holds the wind:
 * - submode, the level;
 * - ang.y, the heading;
 * - scl.x, the strength;
 * - ang.x, the gusts' phase, and ang.z, how far the phase moves a step;
 * - pos, the far point the gusts are measured from, and scl.z, the distance
 *   from it at which their pattern starts over */
static struct task *wind_task = NULL;

void wind_set(Sint8 level, const Angle *heading)
{
    struct task *task;

    if (wind_task == NULL) {
        task = task_create(TASK_WORK, 0, wind_step);
        if (task == NULL) {
            return;
        }
        task->destructor = wind_destroy;
        wind_task = task;
        task->work->ang.y = rand() * RAND_SCALE * 65535.0F;
        task->work->pos.x = -100000.0F;
        task->work->pos.y = 0.0F;
        task->work->pos.z = -100000.0F;
    }
    wind_task->work->submode = level;
    if (heading != NULL) {
        wind_task->work->ang.y = *heading;
    }
}

void wind_stop(void)
{
    if (wind_task != NULL) {
        task_destroy(wind_task);
        wind_task = NULL;
    }
}

static void wind_destroy(struct task *task)
{
    wind_task = NULL;
}

/* while the phase is 0 or has made a turn, pick a strength for the level
 * and a pace for the phase. then move the phase by the pace */
static void wind_step(struct task *task)
{
    struct task_work *work = task->work;

    /* @bug logic: only its overflow, at least 5.8 million steps on,
     * takes ang.x back below 0x10000. from the phase's first turn until
     * then, the test (CMP/GE at 0x8c049bb6) holds on every step, and the
     * wind picks a new strength and pace each step */
    if (work->ang.x == 0 || work->ang.x >= 0x10000) {
        switch (work->submode) {
        case 0:
            work->scl.x = 0.0F;
            break;
        case 1:
            work->scl.x = RANDOM(0.5F, 1.0F);
            break;
        case 2:
            work->scl.x = RANDOM(1.0F, 2.0F);
            break;
        case 3:
            work->scl.x = RANDOM(2.0F, 3.0F);
            break;
        case 4:
        default:
            work->scl.x = RANDOM(3.0F, 5.0F);
            break;
        }
        work->ang.z = PACE_MAX - (Angle)(rand() * RAND_SCALE * PACE_SPREAD);
    }
    work->ang.x += work->ang.z;
    work->scl.z = work->scl.x / ((Float)work->ang.z * (1.0F / 65536.0F));
}

Bool wind_get_force(NJS_POINT3 *pos, NJS_VECTOR *force)
{
    struct task_work *work;
    Angle phase;
    Float gust;

    if (wind_task == NULL) {
        return FALSE;
    }
    if (force != NULL) {
        work = wind_task->work;
        VECTOR_DIFF(force, pos, &work->pos);
        /* each unit of pos's distance from the far point, mod scl.z, adds
         * a turn to the phase. the gusts are bands a unit apart */
        phase = float_wrap(njScalor(force), work->scl.z) * 65536.0F;
        gust = (njSin(phase + work->ang.x) + 1.0F) * 0.5F;
        if (gust > GUST_MAX) {
            gust = GUST_MAX;
        }
        gust *= work->scl.x;
        force->x = njSin(work->ang.y) * gust;
        force->y = 0.0F;
        force->z = njCos(work->ang.y) * gust;
    }
    return TRUE;
}
