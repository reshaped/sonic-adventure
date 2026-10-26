/* O BALLOON1 to O BALLOON4: hot air balloons that rise and turn */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg08/hot_air_balloon.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL hot_air_balloon_1_model;
extern NJS_MODEL hot_air_balloon_2_model;
extern NJS_MODEL hot_air_balloon_3_model;
extern NJS_MODEL hot_air_balloon_4_model;

/* a balloon goes once the camera is BALLOON_RANGE or more from it. its
 * placement then makes no more */
#define BALLOON_RANGE 3000.0F

/* every balloon's turn and rise each step */
#define BALLOON_SPIN 102
#define BALLOON_RISE 1.0F

struct hot_air_balloon_vars {
    Angle spin;
    Float rise;
};

#define VARS(work) ((struct hot_air_balloon_vars *)&(work)->vars)
/* work->tick: which of the four balloons the task is, 0 to 3 */
#define BALLOON_KIND(work) ((work)->tick)

static void hot_air_balloon_destroy(struct task *task)
{
}

static void hot_air_balloon_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    switch (BALLOON_KIND(work)) {
    case 0:
        draw_model(&hot_air_balloon_1_model);
        break;
    case 1:
        draw_model(&hot_air_balloon_2_model);
        break;
    case 2:
        draw_model(&hot_air_balloon_3_model);
        break;
    case 3:
        draw_model(&hot_air_balloon_4_model);
        break;
    }
    njPopMatrix(1);
}

static void hot_air_balloon_rise(struct task *task)
{
    struct task_work *work = task->work;

    if (njDistanceP2P(&camera_work->pos, &work->pos) < BALLOON_RANGE) {
        work->ang.y += VARS(work)->spin;
        work->pos.y += VARS(work)->rise;
        hot_air_balloon_draw(task);
    } else {
        placement_kill(task);
    }
}

static void hot_air_balloon_init(struct task *task, Uint8 kind)
{
    struct task_work *work = task->work;

    /* @hack redundant: a case for each kind, though all four rise and turn
     * alike; as the two stores alone, the function shrinks and 24 of its 24
     * words differ */
    switch (kind) {
    case 0:
        VARS(work)->spin = BALLOON_SPIN;
        VARS(work)->rise = BALLOON_RISE;
        break;
    case 1:
        VARS(work)->spin = BALLOON_SPIN;
        VARS(work)->rise = BALLOON_RISE;
        break;
    case 2:
        VARS(work)->spin = BALLOON_SPIN;
        VARS(work)->rise = BALLOON_RISE;
        break;
    case 3:
        VARS(work)->spin = BALLOON_SPIN;
        VARS(work)->rise = BALLOON_RISE;
        break;
    }
    BALLOON_KIND(work) = kind;
    task->step = hot_air_balloon_rise;
    task->draw = hot_air_balloon_draw;
    task->destructor = hot_air_balloon_destroy;
}

void hot_air_balloon_1_step(struct task *task)
{
    hot_air_balloon_init(task, 0);
}

void hot_air_balloon_2_step(struct task *task)
{
    hot_air_balloon_init(task, 1);
}

void hot_air_balloon_3_step(struct task *task)
{
    hot_air_balloon_init(task, 2);
}

void hot_air_balloon_4_step(struct task *task)
{
    hot_air_balloon_init(task, 3);
}
