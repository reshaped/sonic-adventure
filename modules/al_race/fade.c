/* the Chao race's full-screen gray fade */
#include <ninja.h>

#include "al_race/fade.h"
#include "system/frame.h"
#include "system/task.h"

/* COVER is the gray's opacity, from 0 to 1, in work->scl.x. CHANGE, in
 * work->scl.y, is what each step of a fade adds to it */
#define COVER(work) ((work)->scl.x)
#define CHANGE(work) ((work)->scl.y)

/* the fade's color, a light gray whose alpha the draw sets */
#define FADE_COLOR 0x00c0c0c0

/* the fade's modes. FADE_IN clears the gray, and then the fade is
 * FADE_CLEAR, which draws nothing. FADE_OUT makes the gray opaque, and then
 * the fade is FADE_COVERED */
enum fade_mode { FADE_CLEAR, FADE_IN, FADE_OUT, FADE_COVERED };

static void race_fade_destroy(struct task *task);
static void race_fade_draw(struct task *task);
static void race_fade_step(struct task *task);

/* the corners' colors, filled in by each draw */
static NJS_COLOR race_fade_colors[4];

static struct task *race_fade_task = NULL;
static NJS_POINT2 race_fade_points[4] = {
#include "al_race/fade/race_fade_points.inc"
};
static NJS_POINT2COL race_fade_polygon = {
#include "al_race/fade/race_fade_polygon.inc"
};

void race_fade_in(int steps)
{
    struct task *task = race_fade_task;
    struct task_work *work;

    if (task == NULL) {
        return;
    }
    work = task->work;
    work->mode = FADE_IN;
    if (steps > 0) {
        CHANGE(work) = -COVER(work) / steps;
    } else {
        CHANGE(work) = -COVER(work);
    }
}

void race_fade_out(int steps)
{
    struct task *task = race_fade_task;
    struct task_work *work;

    if (task == NULL) {
        return;
    }
    work = task->work;
    work->mode = FADE_OUT;
    if (steps > 0) {
        CHANGE(work) = (1.0F - COVER(work)) / steps;
    } else {
        CHANGE(work) = 1.0F - COVER(work);
    }
}

void race_fade_create(void)
{
    struct task *task;
    struct task_work *work;

    if (race_fade_task != NULL) {
        return;
    }
    if ((task = task_create(TASK_WORK, 4, race_fade_step)) != NULL) {
        work = task->work;
        work->mode = FADE_CLEAR;
        COVER(work) = 0.0F;
        task->draw = race_fade_draw;
        task->destructor = race_fade_destroy;
        race_fade_task = task;
    }
}

static void race_fade_destroy(struct task *task)
{
    race_fade_task = NULL;
}

static void race_fade_draw(struct task *task)
{
    struct task_work *work;
    NJS_COLOR color;

    if (frame_step != 0) {
        return;
    }
    work = task->work;
    if (work->mode >= FADE_IN) {
        color.color = FADE_COLOR;
        color.argb.a = COVER(work) * 255.0F;
        race_fade_colors[0] = color;
        race_fade_colors[1] = color;
        race_fade_colors[2] = color;
        race_fade_colors[3] = color;
        njDrawPolygon2D(&race_fade_polygon, 4, -1.0F,
                        NJD_FILL | NJD_TRANSPARENT);
    }
}

static void race_fade_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case FADE_CLEAR:
        break;
    case FADE_IN:
        COVER(work) += CHANGE(work);
        if (COVER(work) <= 0.0F) {
            COVER(work) = 0.0F;
            work->mode = FADE_CLEAR;
        }
        break;
    case FADE_OUT:
        COVER(work) += CHANGE(work);
        if (COVER(work) >= 1.0F) {
            COVER(work) = 1.0F;
            work->mode = FADE_COVERED;
        }
        break;
    case FADE_COVERED:
        break;
    }
    race_fade_draw(task);
}
