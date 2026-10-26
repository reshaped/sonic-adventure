/* an event's scan lines: lines of one color across the screen, with
 * spacing rows between them. they scroll down */
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/scanlines.h"
#include "system/task.h"

/* the lines' only mode, in work->mode */
#define SCANLINES_RUNNING 0

/* the lines' record, from the heap, which the destructor frees */
struct scanlines {
    NJS_COLOR color;
    int spacing; /* the rows between two lines */
};

struct scanlines_vars {
    Sint32 unk_00; /* cleared by the create, and never read */
    Uint8 unk_04[4];
    struct scanlines *lines;
};

#define VARS(work) ((struct scanlines_vars *)&(work)->vars)

/* the depth the lines draw at */
#define SCANLINES_DEPTH (-1.05F)

static const int event_scanlines_width = 640;
static const int event_scanlines_height = 480;

/* the lines' task, from create until event_scanlines_kill */
static struct task *event_scanlines = NULL;
/* the top line's row. a new create starts where the last lines left it */
static int event_scanlines_row = 0;

static void event_scanlines_init(struct task *task);
static void event_scanlines_step(struct task *task);
static void event_scanlines_draw(struct task *task);
static void event_scanlines_destroy(struct task *task);
static void event_scanlines_draw_lines(struct task *task);

void event_scanlines_create(Uint8 alpha, Uint8 red, Uint8 green, Uint8 blue,
                            int spacing)
{
    struct task_work *work;
    struct scanlines *lines;

    event_scanlines = task_create(TASK_WORK, 5, event_scanlines_init);
    lines = heap_alloc(sizeof(*lines));
    work = event_scanlines->work;
    VARS(work)->lines = lines;
    work->mode = SCANLINES_RUNNING;
    VARS(work)->unk_00 = 0;
    lines->color.argb.a = alpha;
    lines->color.argb.r = red;
    lines->color.argb.g = green;
    lines->color.argb.b = blue;
    lines->spacing = spacing;
}

void event_scanlines_kill(void)
{
    if (event_scanlines != NULL) {
        task_kill(event_scanlines);
        event_scanlines = NULL;
    }
}

static void event_scanlines_init(struct task *task)
{
    task->step = event_scanlines_step;
    task->draw = event_scanlines_draw;
    task->destructor = event_scanlines_destroy;
}

static void event_scanlines_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SCANLINES_RUNNING:
        break;
    }
    event_scanlines_draw(task);
}

static void event_scanlines_draw_lines(struct task *task)
{
    struct task_work *work = task->work;
    struct scanlines *lines = VARS(work)->lines;
    NJS_POINT2COL line;
    NJS_POINT2 points[2];
    NJS_COLOR colors[2];
    int y;

    line.p = points;
    line.col = colors;
    line.tex = NULL;
    line.num = 2;
    if (++event_scanlines_row == lines->spacing) {
        event_scanlines_row = 0;
    }
    for (y = event_scanlines_row; y < event_scanlines_height;
         y += lines->spacing + 1) {
        line.p[0].x = 0.0F;
        line.p[0].y = (Float)y;
        line.col[0].argb.a = lines->color.argb.a;
        line.col[0].argb.r = lines->color.argb.r;
        line.col[0].argb.g = lines->color.argb.g;
        line.col[0].argb.b = lines->color.argb.b;
        line.p[1].x = (Float)event_scanlines_width;
        line.p[1].y = (Float)y;
        line.col[1].argb.a = lines->color.argb.a;
        line.col[1].argb.r = lines->color.argb.r;
        line.col[1].argb.g = lines->color.argb.g;
        line.col[1].argb.b = lines->color.argb.b;
        njDrawLine2D(&line, 1, SCANLINES_DEPTH, NJD_TRANSPARENT);
    }
}

static void event_scanlines_draw(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SCANLINES_RUNNING:
        event_scanlines_draw_lines(task);
        break;
    }
}

static void event_scanlines_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct scanlines *lines = VARS(work)->lines;

    if (lines != NULL) {
        heap_free(lines);
    }
}
