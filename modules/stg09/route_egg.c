/* SitaAlEgg and UeAlEgg, eggs that loop along the table's lower and upper
 * routes */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "path/path.h"
#include "stg09/casino.h"
#include "stg09/paths.h"
#include "stg09/route_egg.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_ACTION route_egg_action;

struct route_egg_vars {
    Float frame; /* route_egg_action's frame */
    Float distance; /* along the route's path */
};

#define VARS(work) ((struct route_egg_vars *)&(work)->vars)
#define ROUTE(work) ((work)->tick)

/* the routes, as ROUTE holds them */
#define ROUTE_LOWER 0
#define ROUTE_UPPER 1

/* route_egg_action's frames a step, and the frame at which the egg goes
 * back to frame 0. the action has 20 frames */
#define FRAME_SPEED 0.1F
#define LAST_FRAME 19.0F

/* the distances at which an egg goes back to its route's start. the lower
 * path is 1115.8 long, and the upper 1845.4 */
#define LOWER_END 1114.0F
#define UPPER_END 1844.0F

static void route_egg_destroy(struct task *task)
{
}

static void route_egg_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    draw_action(&route_egg_action, VARS(work)->frame);
    njPopMatrix(1);
}

static void route_egg_run(struct task *task)
{
    struct task_work *work = task->work;
    struct path_sample sample;

    VARS(work)->frame += FRAME_SPEED;
    if (VARS(work)->frame >= LAST_FRAME) {
        VARS(work)->frame = 0.0F;
    }
    sample.distance = VARS(work)->distance;
    if (ROUTE(work) == ROUTE_LOWER) {
        path_sample(&route_egg_lower_path, &sample);
    } else {
        path_sample(&route_egg_upper_path, &sample);
    }
    work->pos.x = sample.pos.x;
    work->pos.y = sample.pos.y;
    work->pos.z = sample.pos.z;
    VARS(work)->distance += 1.0F;
    if (ROUTE(work) == ROUTE_LOWER) {
        if (VARS(work)->distance >= LOWER_END) {
            VARS(work)->distance = 0.0F;
        }
    } else {
        if (VARS(work)->distance >= UPPER_END) {
            VARS(work)->distance = 0.0F;
        }
    }
    route_egg_draw(task);
}

static void route_egg_start(struct task *task, Uint8 route)
{
    struct task_work *work = task->work;

    ROUTE(work) = route;
    VARS(work)->frame = 0.0F;
    task->step = route_egg_run;
    task->draw = route_egg_draw;
    task->destructor = route_egg_destroy;
}

void route_egg_lower_step(struct task *task)
{
    route_egg_start(task, ROUTE_LOWER);
}

void route_egg_upper_step(struct task *task)
{
    route_egg_start(task, ROUTE_UPPER);
}
