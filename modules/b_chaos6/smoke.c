/* SMOKE__, Chaos 6's smoke: a puff of dust at the object's pos once every
 * SMOKE_WAIT + 1 steps. the object's scl.x sets each puff's scale, and its
 * scl.y the gray of its color, from 0 to 1. the set editor labels both
 * fields */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos6/smoke.h"
#include "debug/editor.h"
#include "effect/dust.h"
#include "system/task.h"

/* the steps with no puff between two puffs */
#define SMOKE_WAIT 6

struct smoke_vars {
    Uint8 unk_00[4];
    Sint16 steps; /* the steps since its last puff */
};

#define VARS(work) ((struct smoke_vars *)&(work)->vars)

enum smoke_mode { SMOKE_START, SMOKE_RUN };

/* every puff's params, which each live puff keeps reading. the step sets
 * scale, pos and the gray of color before each puff, which the puff copies
 * as it starts */
static struct dust_params chaos6_smoke_params = {
    0.45F,
    0.04F,
    0.23F,
    0.06F,
    0.015F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 1.0F, 0.0F},
    {1.0F, 0.35F, 0.35F, 0.35F},
};

void chaos6_smoke_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- SIZE");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- COLOR(0.0 - 1.0)");
    }
    switch (work->mode) {
    case SMOKE_START:
        work->mode = SMOKE_RUN;
        /* fall through */
    case SMOKE_RUN:
        if (++VARS(work)->steps > SMOKE_WAIT) {
            VARS(work)->steps = 0;
            chaos6_smoke_params.scale = work->scl.x;
            chaos6_smoke_params.pos = work->pos;
            chaos6_smoke_params.color.r = chaos6_smoke_params.color.g =
                chaos6_smoke_params.color.b = work->scl.y;
            dust_create_from(&chaos6_smoke_params);
        }
        break;
    }
}
