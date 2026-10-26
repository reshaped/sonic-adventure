/* an event's black over the screen, which clears at a rate */
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/black.h"
#include "system/task.h"

struct black_vars {
    Uint8 unk_00[4];
    /* the black's transparency, 0 when opaque. the draw's alpha is
     * (1 - transparency) * 255, and it draws nothing while that is below 1 */
    Float transparency;
    Float rate; /* added to transparency each step */
};

#define VARS(work) ((struct black_vars *)&(work)->vars)

static NJS_POINT2 event_black_corners[4] = {
#include "shared/black/event_black_corners.inc"
};
static NJS_COLOR event_black_colors[4] = {
#include "shared/black/event_black_colors.inc"
};
static NJS_POINT2COL event_black_polygon = {
#include "shared/black/event_black_polygon.inc"
};
static NJS_ARGB event_black_unused = {1.0F, 1.0F, 1.0F, 1.0F};

/* the black's task, or null when there is none */
static struct task *event_black = NULL;

static void event_black_destroy(struct task *task)
{
}

static void event_black_draw(struct task *task)
{
    struct task_work *work = task->work;
    int alpha = (1.0F - VARS(work)->transparency) * 255.0F;

    if (alpha > 0) {
        event_black_colors[0].argb.a = event_black_colors[1].argb.a =
            event_black_colors[2].argb.a = event_black_colors[3].argb.a = alpha;
        njDrawPolygon2D(&event_black_polygon, 4, -1.0F,
                        NJD_FILL | NJD_TRANSPARENT);
    }
}

static void event_black_step(struct task *task)
{
    struct task_work *work = task->work;
    Float transparency;
    Float rate;

    /* @hack spelling: the rate read before the transparency it adds to, as the
     * image reads them (0x0cb8256c and 0x0cb82570 in EV0060). as
     * VARS(work)->transparency += VARS(work)->rate, which reads the
     * transparency first, the step runs 2 bytes long and 7 of its 8 words
     * differ */
    rate = VARS(work)->rate;
    transparency = VARS(work)->transparency;
    VARS(work)->transparency = transparency + rate;
    event_black_draw(task);
}

static void event_black_init(struct task *task)
{
    task->step = event_black_step;
    task->draw = event_black_draw;
    task->destructor = event_black_destroy;
    VARS(task->work)->transparency = 0.0F;
    event_black_step(task);
}

void event_black_set_rate(Float rate)
{
    if (event_black == NULL) {
        event_black = task_create(TASK_WORK, 5, event_black_init);
    }
    VARS(event_black->work)->rate = rate;
}

void event_black_kill(void)
{
    if (event_black != NULL) {
        task_kill(event_black);
        event_black = NULL;
    }
}
