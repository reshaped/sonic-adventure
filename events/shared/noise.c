/* an event's screen noise. the noise is a black screen under NOISE_PIXELS
 * white pixels at random places, all at one alpha. the noise's step ends by
 * drawing it, besides its task's draw */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "main.h"
#include "shared/noise.h"
#include "system/task.h"
#include "system/util.h"

/* the white pixels the noise draws each time */
#define NOISE_PIXELS 1000
/* the steps in one cycle of the flicker */
#define FLICKER_CYCLE 60U

/* the noise's phases, in its work's mode. event_noise_start runs
 * NOISE_BLACK to NOISE_CLEAR in order. event_noise_show runs NOISE_SHOW,
 * then NOISE_CLEAR */
enum noise_mode {
    NOISE_BLACK,
    NOISE_FLICKER,
    NOISE_FADE,
    NOISE_HOLD,
    NOISE_CLEAR,
    NOISE_SHOW
};

/* the noise's record, from the heap, which its destructor frees */
struct noise {
    Uint8 alpha; /* of the noise's black and its white pixels */
    Uint8 fade; /* taken from alpha on each step of the fade but the last */
    Uint8 unk_02[2];
    Uint32 black_steps;
    Uint32 flicker_steps;
    Uint32 fade_steps;
    Uint32 hold_steps;
    Uint32 show_steps;
};

struct noise_vars {
    Uint32 steps; /* spent in the phase so far */
    Uint8 unk_04[4];
    struct noise *noise;
};

#define NOISE(work) ((struct noise_vars *)&(work)->vars)

static struct task *event_noise_task = NULL;

static void event_noise_init(struct task *task);
static void event_noise_step(struct task *task);
static void event_noise_draw(struct task *task);
static void event_noise_destroy(struct task *task);

void event_noise_start(int black_steps, int flicker_steps, int fade_steps,
                       int hold_steps)
{
    struct noise *noise;
    struct task_work *work;

    event_noise_task = task_create(TASK_WORK, 5, event_noise_init);
    noise = heap_alloc(sizeof(struct noise));
    work = event_noise_task->work;
    NOISE(work)->noise = noise;
    work->mode = NOISE_BLACK;
    NOISE(work)->steps = 0;
    noise->alpha = 240;
    noise->fade = (int)(112.0F / fade_steps) + 1;
    noise->black_steps = black_steps;
    noise->flicker_steps = flicker_steps;
    noise->fade_steps = fade_steps;
    noise->hold_steps = hold_steps;
}

void event_noise_create(void)
{
    struct noise *noise;
    struct task_work *work;

    event_noise_task = task_create(TASK_WORK, 5, event_noise_init);
    noise = heap_alloc(sizeof(struct noise));
    work = event_noise_task->work;
    NOISE(work)->noise = noise;
    work->mode = NOISE_CLEAR;
    NOISE(work)->steps = 0;
    noise->alpha = 0;
}

void event_noise_show(int steps, Uint8 alpha)
{
    struct task_work *work;
    struct noise *noise;

    work = event_noise_task->work;
    noise = NOISE(work)->noise;
    work->mode = NOISE_SHOW;
    NOISE(work)->steps = 0;
    noise->show_steps = steps;
    noise->alpha = alpha;
}

void event_noise_kill(void)
{
    if (event_noise_task != NULL) {
        task_kill(event_noise_task);
        event_noise_task = NULL;
    }
}

static void event_noise_init(struct task *task)
{
    task->step = event_noise_step;
    task->draw = event_noise_draw;
    task->destructor = event_noise_destroy;
}

static void event_noise_step(struct task *task)
{
    struct task_work *work = task->work;
    struct noise *noise = NOISE(work)->noise;

    switch (work->mode) {
    case NOISE_BLACK:
        NOISE(work)->steps++;
        if (NOISE(work)->steps >= noise->black_steps) {
            work->mode = NOISE_FLICKER;
            NOISE(work)->steps = 0;
        }
        break;
    case NOISE_FLICKER:
        NOISE(work)->steps++;
        if (NOISE(work)->steps >= noise->flicker_steps) {
            work->mode = NOISE_FADE;
            NOISE(work)->steps = 0;
        }
        break;
    case NOISE_FADE:
        NOISE(work)->steps++;
        if (NOISE(work)->steps >= noise->fade_steps) {
            work->mode = NOISE_HOLD;
            NOISE(work)->steps = 0;
        } else {
            noise->alpha -= noise->fade;
        }
        break;
    case NOISE_HOLD:
        NOISE(work)->steps++;
        if (NOISE(work)->steps >= noise->hold_steps) {
            noise->alpha = 0;
            work->mode = NOISE_CLEAR;
        }
        break;
    case NOISE_CLEAR:
        break;
    case NOISE_SHOW:
        NOISE(work)->steps++;
        if (NOISE(work)->steps >= noise->show_steps) {
            noise->alpha = 0;
            work->mode = NOISE_CLEAR;
        }
        break;
    }
    event_noise_draw(task);
}

/* cover the screen in opaque black */
static void event_noise_draw_black(struct task *task)
{
    NJS_POINT2COL polygon;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];

    polygon.p = points;
    polygon.col = colors;
    polygon.tex = NULL;
    polygon.num = 4;
    polygon.p[0].x = 0.0F;
    polygon.p[0].y = 0.0F;
    polygon.col[0].color = 0xff000000;
    polygon.p[1].x = 0.0F;
    polygon.p[1].y = SCREEN_HEIGHT;
    polygon.col[1].color = 0xff000000;
    polygon.p[2].x = SCREEN_WIDTH;
    polygon.p[2].y = SCREEN_HEIGHT;
    polygon.col[2].color = 0xff000000;
    polygon.p[3].x = SCREEN_WIDTH;
    polygon.p[3].y = 0.0F;
    polygon.col[3].color = 0xff000000;
    njDrawPolygon2D(&polygon, 4, -1.5F, NJD_FILL);
}

/* draw the black at the noise's alpha, and NOISE_PIXELS white pixels at
 * random places over it. each pixel is a square 1 wide */
static void event_noise_draw_noise(struct task *task)
{
    NJS_POINT2COL polygon;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    struct task_work *work = task->work;
    struct noise *noise = NOISE(work)->noise;
    int i;

    polygon.p = points;
    polygon.col = colors;
    polygon.tex = NULL;
    polygon.num = 4;
    polygon.p[0].x = 0.0F;
    polygon.p[0].y = 0.0F;
    polygon.col[0].argb.a = noise->alpha;
    polygon.col[0].argb.r = 0;
    polygon.col[0].argb.g = 0;
    polygon.col[0].argb.b = 0;
    polygon.p[1].x = 0.0F;
    polygon.p[1].y = SCREEN_HEIGHT;
    polygon.col[1].argb.a = noise->alpha;
    polygon.col[1].argb.r = 0;
    polygon.col[1].argb.g = 0;
    polygon.col[1].argb.b = 0;
    polygon.p[2].x = SCREEN_WIDTH;
    polygon.p[2].y = SCREEN_HEIGHT;
    polygon.col[2].argb.a = noise->alpha;
    polygon.col[2].argb.r = 0;
    polygon.col[2].argb.g = 0;
    polygon.col[2].argb.b = 0;
    polygon.p[3].x = SCREEN_WIDTH;
    polygon.p[3].y = 0.0F;
    polygon.col[3].argb.a = noise->alpha;
    polygon.col[3].argb.r = 0;
    polygon.col[3].argb.g = 0;
    polygon.col[3].argb.b = 0;
    njDrawPolygon2D(&polygon, 4, -1.5F, NJD_FILL | NJD_TRANSPARENT);
    for (i = 0; i < NOISE_PIXELS; i++) {
        polygon.p[0].x = (Sint16)(rand() * (SCREEN_WIDTH * RAND_SCALE));
        polygon.p[0].y = (Sint16)(rand() * (SCREEN_HEIGHT * RAND_SCALE));
        polygon.col[0].argb.a = noise->alpha;
        polygon.col[0].argb.r = 255;
        polygon.col[0].argb.g = 255;
        polygon.col[0].argb.b = 255;
        polygon.p[1].x = polygon.p[0].x;
        polygon.p[1].y = polygon.p[0].y + 1.0F;
        polygon.col[1].argb.a = noise->alpha;
        polygon.col[1].argb.r = 255;
        polygon.col[1].argb.g = 255;
        polygon.col[1].argb.b = 255;
        polygon.p[2].x = polygon.p[0].x + 1.0F;
        polygon.p[2].y = polygon.p[0].y + 1.0F;
        polygon.col[2].argb.a = noise->alpha;
        polygon.col[2].argb.r = 255;
        polygon.col[2].argb.g = 255;
        polygon.col[2].argb.b = 255;
        polygon.p[3].x = polygon.p[0].x + 1.0F;
        polygon.p[3].y = polygon.p[0].y;
        polygon.col[3].argb.a = noise->alpha;
        polygon.col[3].argb.r = 255;
        polygon.col[3].argb.g = 255;
        polygon.col[3].argb.b = 255;
        njDrawPolygon2D(&polygon, 4, -1.0F, NJD_FILL | NJD_TRANSPARENT);
    }
}

/* draw what the phase shows: the black, or the noise */
static void event_noise_draw(struct task *task)
{
    /* @hack redundant: register on work, which keeps work in R5 (MOV.L
     * @(32,R4),R5 at 0x0cb813ec in EV00B0); without it, 7 of 45 words
     * differ */
    register struct task_work *work = task->work;
    Uint32 step;

    switch (work->mode) {
    case NOISE_BLACK:
        event_noise_draw_black(task);
        break;
    case NOISE_FADE:
        event_noise_draw_noise(task);
        break;
    case NOISE_FLICKER:
        if (((step = NOISE(work)->steps % FLICKER_CYCLE) >= 15 && step <= 20) ||
            (step >= 50 && step <= 53)) {
            event_noise_draw_noise(task);
        } else {
            event_noise_draw_black(task);
        }
        break;
    case NOISE_HOLD:
    case NOISE_CLEAR:
    case NOISE_SHOW:
        event_noise_draw_noise(task);
        break;
    }
}

static void event_noise_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct noise *noise = NOISE(work)->noise;

    if (noise != NULL) {
        heap_free(noise);
    }
}
