/* O FUUSYA1, a windmill whose blades turn at a speed that swells and fades */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/model.h"
#include "object/placement.h"
#include "stg08/windmill.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the mill, with its blades as its child */
extern NJS_OBJECT windmill_object;

/* squared: destroyed beyond about 1000 units of the camera and player 0 */
#define WINDMILL_RANGE 1000100.0F

/* each step, the blades turn BLADE_DEGREES plus BLADE_SWELL_DEGREES times
 * SWELL of work->timer. SWELL(step) rises from 0 to 1 and falls back every
 * SWELL_STEPS / 2 steps */
#define BLADE_DEGREES 2.2F
#define BLADE_SWELL_DEGREES 0.5F
#define SWELL_STEPS 260.0F
#define SWELL(step)                                                            \
    njAbs(njSin((Angle)((step) * (360.0F / SWELL_STEPS) * ANGLE_PER_DEGREE)))
#define BLADE_TURN(work) ((work)->ang.z)

static void windmill_destroy(struct task *task);
static void windmill_run(struct task *task);
static void windmill_draw(struct task *task);

void windmill_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, WINDMILL_RANGE)) {
        BLADE_TURN(work) = 0;
        work->timer = 0;
        task->draw = windmill_draw;
        task->step = windmill_run;
        task->destructor = windmill_destroy;
    }
}

static void windmill_destroy(struct task *task)
{
}

static void windmill_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, WINDMILL_RANGE)) {
        windmill_draw(task);
        BLADE_TURN(work) +=
            (Angle)((BLADE_DEGREES +
                     (BLADE_SWELL_DEGREES * SWELL(TASK_TIMER(work)++))) *
                    ANGLE_PER_DEGREE);
    }
}

/* the blades turn about their model's center */
static void windmill_draw(struct task *task)
{
    NJS_OBJECT *mill = &windmill_object;
    struct task_work *work = task->work;
    NJS_OBJECT *child;
    NJS_MODEL *blades;
    Float *walk;
    Float x;
    Float y;
    Float z;
    Angle ang;

    if (!placement_try_leave_by(task, WINDMILL_RANGE)) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(mill->model);
        /* @hack spelling: the child read into a local of its own, which
         * loads it into R4 (0x0c9092ac); read in the model's load, it takes
         * R2 and 2 of 96 words differ */
        child = mill->child;
        blades = child->model;
        /* @hack spelling: the center read through a Float walk, which steps one
         * pointer through it (FMOV.S @R4+,FR14 at 0x0c9092b6); through the
         * members, work and the mill trade R14 and R13, the function runs 2
         * bytes long and 58 of 96 words differ */
        walk = (Float *)&blades->center;
        x = *walk++;
        y = *walk++;
        z = *walk;
        njTranslate(NULL, x, y, z);
        TURN_VIA(njRotateZ, BLADE_TURN(work), ang);
        njTranslate(NULL, -x, -y, -z);
        draw_model(blades);
        njPopMatrix(1);
    }
}
