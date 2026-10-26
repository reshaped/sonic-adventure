/* an event's fade: a polygon of one color over the whole screen. its alpha
 * moves by its speed each step, and holds at 0 and 1 */
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/fade.h"
#include "system/task.h"

/* the fade's view of its task_work, whose first 32 bytes it never reads. the
 * color is in pos, and the alpha, its speed and the depth in scl */
struct fade_work {
    Uint8 unk_00[32];
    Float r, g, b;
    Float alpha;
    Float speed; /* added to alpha each step */
    Float depth; /* the draw priority it passes njDrawPolygon2D as pri */
};

static NJS_POINT2 event_fade_corners[4] = {
#include "shared/fade/event_fade_corners.inc"
};

static void event_fade_draw(struct task *task)
{
    struct fade_work *work = (struct fade_work *)task->work;
    Uint8 alpha;
    Uint8 red;
    Uint8 green;
    Uint8 blue;
    NJS_POINT2COL polygon;
    NJS_COLOR colors[4];

    alpha = work->alpha * 255.0F;
    red = work->r * 255.0F;
    green = work->g * 255.0F;
    blue = work->b * 255.0F;
    if (alpha > 0) {
        polygon.p = event_fade_corners;
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
            njDrawPolygon2D(&polygon, 4, work->depth,
                            NJD_FILL | NJD_TRANSPARENT);
        } else {
            njDrawPolygon2D(&polygon, 4, work->depth, NJD_FILL);
        }
    }
}

static void event_fade_step(struct task *task)
{
    struct fade_work *work = (struct fade_work *)task->work;
    Float alpha = work->alpha + work->speed;

    if (alpha > 1.0F) {
        alpha = 1.0F;
    } else if (alpha < 0.0F) {
        alpha = 0.0F;
    }
    work->alpha = alpha;
    event_fade_draw(task);
}

static void event_fade_destroy(struct task *task)
{
}

static void event_fade_init(struct task *task)
{
    task->step = event_fade_step;
    task->draw = event_fade_draw;
    task->destructor = event_fade_destroy;
    event_fade_step(task);
}

struct task *event_fade_create(Float speed, Float alpha, Float red, Float green,
                               Float blue)
{
    struct task *task = task_create(TASK_WORK, 1, event_fade_init);
    struct fade_work *work = (struct fade_work *)task->work;

    work->speed = speed;
    work->alpha = alpha;
    work->r = red;
    work->g = green;
    work->b = blue;
    work->depth = -1.0F;
    return task;
}

void event_fade_set_speed(struct task *task, Float speed)
{
    struct fade_work *work = (struct fade_work *)task->work;

    work->speed = speed;
}

void event_fade_set_alpha(struct task *task, Float alpha)
{
    struct fade_work *work = (struct fade_work *)task->work;

    work->alpha = alpha;
}

void event_fade_set_color(struct task *task, Float red, Float green, Float blue)
{
    struct fade_work *work = (struct fade_work *)task->work;

    work->r = red;
    work->g = green;
    work->b = blue;
}

void event_fade_set_depth(struct task *task, Float depth)
{
    struct fade_work *work = (struct fade_work *)task->work;

    work->depth = depth;
}
