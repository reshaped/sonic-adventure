/* an event's fade in: a polygon of one color over the whole screen. its
 * alpha rises to full and holds. the work keeps the color in pos, and the
 * alpha and its step in scl.x and scl.y */
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/fade_in.h"
#include "system/task.h"

static NJS_POINT2 event_fade_in_corners[4] = {
#include "shared/fade_in/event_fade_in_corners.inc"
};

#define FADE_IN_STEP 0.01F /* the alpha it gains each step */

/* the last fade in's task, or null before the first create and after
 * event_fade_in_kill */
static struct task *event_fade_in = NULL;

static void event_fade_in_destroy(struct task *task)
{
}

static void event_fade_in_draw(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 alpha;
    Uint8 red;
    Uint8 green;
    Uint8 blue;
    NJS_POINT2COL polygon;
    NJS_COLOR colors[4];

    alpha = work->scl.x * 255.0F;
    red = work->pos.x;
    green = work->pos.y;
    blue = work->pos.z;
    /* @hack type: alpha, a Uint8, tested against 0.0F, so SHC converts it and
     * compares floats (FLOAT and FCMP/GT at 0x0cb81e90 in EV001B). tested
     * against 0, the draw runs 4 bytes short and 70 of 76 words differ */
    if (alpha > 0.0F) {
        polygon.p = event_fade_in_corners;
        polygon.col = colors;
        polygon.tex = NULL;
        polygon.num = 4;
        colors[3].argb.a = alpha;
        colors[3].argb.r = red;
        colors[3].argb.g = green;
        colors[3].argb.b = blue;
        colors[0] = colors[1] = colors[2] = colors[3];
        if (alpha < 255) {
            njColorBlendingMode(NJD_SOURCE_COLOR,
                                NJD_COLOR_BLENDING_BOTHSRCALPHA);
            njDrawPolygon2D(&polygon, 4, -1.0F, NJD_FILL | NJD_TRANSPARENT);
        } else {
            njDrawPolygon2D(&polygon, 4, -1.0F, NJD_FILL);
        }
    }
}

static void event_fade_in_step(struct task *task)
{
    struct task_work *work = task->work;
    Float alpha = work->scl.x + work->scl.y;

    if (alpha > 1.0F) {
        alpha = 1.0F;
    } else if (alpha < 0.0F) {
        alpha = 0.0F;
    }
    work->scl.x = alpha;
    event_fade_in_draw(task);
}

static void event_fade_in_init(struct task *task)
{
    struct task_work *work = task->work;

    work->scl.x = 0.0F;
    work->scl.y = FADE_IN_STEP;
    task->step = event_fade_in_step;
    task->draw = event_fade_in_draw;
    task->destructor = event_fade_in_destroy;
    event_fade_in_step(task);
}

void event_fade_in_create(Float red, Float green, Float blue)
{
    struct task *task = task_create(TASK_WORK, 1, event_fade_in_init);

    if (task != NULL) {
        struct task_work *work = task->work;

        work->pos.x = red;
        work->pos.y = green;
        work->pos.z = blue;
        event_fade_in = task;
    }
}

void event_fade_in_kill(void)
{
    if (event_fade_in != NULL) {
        task_kill(event_fade_in);
        event_fade_in = NULL;
    }
}
