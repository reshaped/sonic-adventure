/* the Perfect Chaos fight's water column, which rises and sinks again */
#include <ninja.h>

#include "b_chaos7/column.h"
#include "b_chaos7/textures.h" /* NOLINT(misc-include-cleaner) */
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"

extern NJS_OBJECT column_lower_object;
extern NJS_OBJECT column_upper_object;

/* a column destroys itself once neither the camera nor player 0 is within 5010
 * of it. the range is squared, as placement_try_leave_by takes it */
#define COLUMN_RANGE (5010.0F * 5010.0F)

/* each part rises and sinks by COLUMN_SPEED of its full height a step. the
 * lower part stands until its timer passes COLUMN_STAND */
#define COLUMN_SPEED 0.05F
#define COLUMN_STAND 60

#define COLUMN_FRAMES 5

/* COLUMN_LOWER and COLUMN_UPPER are each part's height, as a share of its
 * full height. COLUMN_COLOR is the r, g and b of the constant material the
 * draw sets. the first step sets it to -1, and nothing changes it */
#define COLUMN_LOWER(work) ((work)->scl.x)
#define COLUMN_UPPER(work) ((work)->scl.y)
#define COLUMN_COLOR(work) ((work)->scl.z)

/* the lower part's modes, in work->mode, and the upper part's, in
 * work->submode. the upper part's keep the lower's numbers, and it never
 * stands. the upper part rises once the lower part starts to sink. the column
 * ends once the upper part has sunk again */
enum column_mode {
    COLUMN_INIT,
    COLUMN_RISING,
    COLUMN_STANDING,
    COLUMN_SINKING,
    COLUMN_SUNK
};
enum column_upper {
    UPPER_WAIT = 0,
    UPPER_RISING = 1,
    UPPER_SINKING = 3,
    UPPER_SUNK = 4
};

/* a texlist for each of column_textures, c4_column01 to c4_column05. both
 * parts draw with the one work->tick picks. work->tick advances a step while
 * the lower part moves or stands */
static NJS_TEXLIST column_texlists[COLUMN_FRAMES] = {
#include "b_chaos7/column/column_texlists.inc"
};

static void column_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;

    color.a = 0.0F;
    color.r = color.g = color.b = COLUMN_COLOR(work);
    if (frame_step != 0) {
        return;
    }
    MODEL(work)->texlist = &column_texlists[work->tick];
    njSetTexture(MODEL(work)->texlist);
    njSetConstantMaterial(&color);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScale(NULL, 1.0F, COLUMN_LOWER(work), 1.0F);
    draw_tree(&column_lower_object);
    njPushMatrix(NULL);
    njScale(NULL, 1.0F, COLUMN_UPPER(work), 1.0F);
    draw_tree(&column_upper_object);
    njPopMatrix(1);
    njPopMatrix(1);
    color.r = color.g = color.b = 0.0F;
    njSetConstantMaterial(&color);
}

static void column_destroy(struct task *task)
{
    task_kill(task);
}

void chaos7_column_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, COLUMN_RANGE)) {
        return;
    }
    switch (work->mode) {
    case COLUMN_INIT:
        work->mode = COLUMN_RISING;
        work->submode = UPPER_WAIT;
        COLUMN_LOWER(work) = COLUMN_UPPER(work) = 0.0F;
        COLUMN_COLOR(work) = -1.0F;
        task->draw = column_draw;
        task->destructor = column_destroy;
        break;
    case COLUMN_RISING:
        COLUMN_LOWER(work) += COLUMN_SPEED;
        if (COLUMN_LOWER(work) > 1.0F) {
            work->mode = COLUMN_STANDING;
        }
        if (++work->tick >= COLUMN_FRAMES) {
            work->tick = 0;
        }
        column_draw(task);
        break;
    case COLUMN_STANDING:
        if (++TASK_TIMER(work) > COLUMN_STAND) {
            work->mode = COLUMN_SINKING;
            work->submode = UPPER_RISING;
        }
        if (++work->tick >= COLUMN_FRAMES) {
            work->tick = 0;
        }
        column_draw(task);
        break;
    case COLUMN_SINKING:
        COLUMN_LOWER(work) -= COLUMN_SPEED;
        if (COLUMN_LOWER(work) <= 0.0F) {
            work->mode = COLUMN_SUNK;
        }
        if (++work->tick >= COLUMN_FRAMES) {
            work->tick = 0;
        }
        column_draw(task);
        break;
    case COLUMN_SUNK:
        column_draw(task);
        break;
    }
    switch (work->submode) {
    case UPPER_WAIT:
        break;
    case UPPER_RISING:
        COLUMN_UPPER(work) += COLUMN_SPEED;
        if (COLUMN_UPPER(work) > 1.0F) {
            work->submode = UPPER_SINKING;
        }
        break;
    case UPPER_SINKING:
        COLUMN_UPPER(work) -= COLUMN_SPEED;
        if (COLUMN_UPPER(work) <= 0.0F) {
            work->submode = UPPER_SUNK;
        }
        break;
    case UPPER_SUNK:
        task_kill(task);
        break;
    }
}
