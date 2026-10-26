/* an event's white screen: a white polygon over the whole screen. its
 * alpha climbs to 255, holds and falls back. the task kills itself once the
 * alpha has fallen to a step's change or less */
#include <ninja.h>
#include <sg_xpt.h>

#include "event/play.h"
#include "shared/white.h"
#include "system/task.h"

/* the white's modes, in work->mode, in the order it runs them */
#define WHITE_IDLE 0
#define WHITE_IN 1
#define WHITE_HOLD 2
#define WHITE_OUT 3
#define WHITE_DONE 4

/* the depth the white draws at */
#define WHITE_DEPTH (-1.0F)

/* the white's record, which its task's first step takes from the heap */
struct white {
    Uint8 in_step; /* added to the alpha each step as it climbs */
    Uint8 out_step; /* taken from it each step as it falls */
    Uint8 unk_02[2];
    Sint32 hold; /* WHITE_HOLD runs hold + 1 steps */
    Sint32 timer; /* the steps it has held */
    NJS_COLOR color;
};

struct white_vars {
    Uint8 unk_00[8];
    struct white *white;
};

#define VARS(work) ((struct white_vars *)&(work)->vars)

/* the white's task, from a start until it ends or event_white_kill */
static struct task *event_white = NULL;

static void event_white_init(struct task *task);
static void event_white_step(struct task *task);
static void event_white_draw(struct task *task);
static void event_white_destroy(struct task *task);

/* the wait lets the task's first step allocate its record */
void event_white_start(int in_steps, int out_steps)
{
    struct task_work *work;
    struct white *white;

    event_white = task_create(TASK_WORK, 5, event_white_init);
    event_wait(1);
    white = VARS(event_white->work)->white;
    /* 255.0F / 0 is infinite, which the SH-4's conversion to int turns into
     * 0x7fffffff. its low byte makes the change 255 */
    white->in_step = 255.0F / in_steps;
    white->out_step = 255.0F / out_steps;
    white->hold = 0;
    white->timer = 0;
    if (event_white != NULL) {
        work = event_white->work;
        work->mode = WHITE_IN;
    }
}

void event_white_start_hold(int in_steps, int hold, int out_steps)
{
    struct task_work *work;
    struct white *white;

    event_white = task_create(TASK_WORK, 5, event_white_init);
    event_wait(1);
    white = VARS(event_white->work)->white;
    white->in_step = 255.0F / in_steps;
    white->out_step = 255.0F / out_steps;
    white->hold = hold;
    white->timer = 0;
    if (event_white != NULL) {
        work = event_white->work;
        work->mode = WHITE_IN;
    }
}

void event_white_kill(void)
{
    if (event_white != NULL) {
        task_kill(event_white);
        event_white = NULL;
    }
}

static void event_white_init(struct task *task)
{
    struct task_work *work = task->work;
    struct white *white = heap_alloc(sizeof(*white));

    work->mode = WHITE_IDLE;
    white->color.argb.a = 0;
    white->color.argb.r = 255;
    white->color.argb.g = 255;
    white->color.argb.b = 255;
    VARS(work)->white = white;
    task->step = event_white_step;
    task->draw = event_white_draw;
    task->destructor = event_white_destroy;
}

static void event_white_step(struct task *task)
{
    struct task_work *work = task->work;
    struct white *white = VARS(work)->white;

    switch (work->mode) {
    case WHITE_IDLE:
        break;
    case WHITE_IN:
        white->color.argb.a += white->in_step;
        if (white->color.argb.a >= 255 - white->in_step) {
            white->color.argb.a = 255;
            work->mode = WHITE_HOLD;
        }
        break;
    case WHITE_HOLD:
        if (++white->timer > white->hold) {
            work->mode = WHITE_OUT;
        }
        break;
    case WHITE_OUT:
        white->color.argb.a -= white->out_step;
        if (white->color.argb.a <= white->out_step) {
            work->mode = WHITE_DONE;
        }
        break;
    case WHITE_DONE:
        task_kill(task);
        event_white = NULL;
        break;
    }
    event_white_draw(task);
}

static void event_white_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct white *white = VARS(work)->white;
    NJS_POINT2COL polygon;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];

    polygon.p = points;
    polygon.col = colors;
    polygon.tex = NULL;
    polygon.num = 4;
    polygon.p[0].x = 0.0F;
    polygon.p[0].y = 0.0F;
    polygon.col[0] = white->color;
    polygon.p[1].x = 0.0F;
    polygon.p[1].y = 480.0F;
    polygon.col[1] = white->color;
    polygon.p[2].x = 640.0F;
    polygon.p[2].y = 480.0F;
    polygon.col[2] = white->color;
    polygon.p[3].x = 640.0F;
    polygon.p[3].y = 0.0F;
    polygon.col[3] = white->color;
    njDrawPolygon2D(&polygon, 4, WHITE_DEPTH, NJD_FILL | NJD_TRANSPARENT);
}

static void event_white_destroy(struct task *task)
{
    heap_free(VARS(task->work)->white);
}
