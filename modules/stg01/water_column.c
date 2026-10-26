/* water column splash that rises from the sea and sinks back */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/snow.h"
#include "stg01/water_column.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST beach_water_column_texlist;
extern NJS_OBJECT beach_water_column_object_0;
extern NJS_OBJECT beach_water_column_object_1;

static NJS_OBJECT *beach_water_column_inner = &beach_water_column_object_0;
static NJS_OBJECT *beach_water_column_outer = &beach_water_column_object_1;

/* the column's state, in work->vars. scl.x is the column's width and scl.y
 * its height. scl.z counts steps toward its next spray puff. a column of size
 * above 0.2 throws a puff each step of its rise. a smaller one throws none */
struct water_column_vars {
    Float shade; /* twice this is added to the sheets' material color */
    Float rise; /* the height grows by rise squared times size a step */
    Float size; /* the scale the column was made at */
};

#define VARS(work) ((struct water_column_vars *)&(work)->vars)

enum water_column_mode { COLUMN_RISE, COLUMN_SINK, COLUMN_END };

static void beach_water_column_step(struct task *task);
static void beach_water_column_run(struct task *task);
static void beach_water_column_draw(struct task *task);
static void beach_water_column_destroy(struct task *task);

void beach_water_column_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale)
{
    struct task *task =
        task_create(TASK_MOTION | TASK_WORK, 3, beach_water_column_step);
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    work->pos.x = pos->x;
    work->pos.y = pos->y;
    work->pos.z = pos->z;
    motion->speed.x = speed->x;
    motion->speed.y = 0.0F;
    motion->speed.z = speed->z;
    work->scl.x = work->scl.z = scale * 2.0F;
    work->scl.y = 0.0F;
    work->scl.z = 0.0F;
    VARS(work)->shade = scale * 0.1F;
    VARS(work)->size = scale;
    VARS(work)->rise = 0.1F;
}

static void beach_water_column_step(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = COLUMN_RISE;
    task->step = beach_water_column_run;
    task->draw = beach_water_column_draw;
    task->destructor = beach_water_column_destroy;
}

/* rise until the height passes size times 0.04. then sink and darken until
 * shade falls below -1, and end */
static void beach_water_column_run(struct task *task)
{
    struct task_work *work = task->work;
    Float most;
    Float shade;
    Float rise;
    Float size;
    Float time;

    ++work->timer;
    switch (work->mode) {
    case COLUMN_RISE:
        work->scl.x += VARS(work)->size * 0.03F;
        time = TASK_TIMER(work);
        work->scl.y =
            (time * VARS(work)->rise * VARS(work)->rise * VARS(work)->size) +
            0.001F;
        if (work->scl.y > VARS(work)->size * (0.2F * 0.2F)) {
            work->mode = COLUMN_SINK;
            VARS(work)->shade = 1.0F;
            work->timer = 0;
        }
        work->scl.z += 1.0F;
        most = VARS(work)->size * 5.0F;
        if (most > 5.0F) {
            most = 5.0F;
        }
        if (work->scl.z++ < most) {
            NJS_VECTOR speed = {0.0F, 1.5F, 0.0F};

            speed.y *= VARS(work)->rise * 1.5F;
            snow_create(&work->pos, &speed,
                        ((VARS(work)->size * 2.0F) + 1.0F) * 0.5F);
            work->scl.z = 0.0F;
        }
        break;
    case COLUMN_SINK:
        shade = VARS(work)->shade;
        /* @hack redundant: size and rise, loaded and stored back unchanged
         * (FMOV.S at 0x0c902a0a and 0x0c902a0e, and at 0x0c902a40 and
         * 0x0c902a46). without them, 64 of 172 words differ and the functions
         * after it move */
        size = VARS(work)->size;
        rise = VARS(work)->rise;
        work->scl.y -= 0.001F;
        shade -= 0.01F;
        if (shade < -1.0F) {
            work->mode = COLUMN_END;
        }
        work->scl.x += VARS(work)->size * 0.03F;
        VARS(work)->shade = shade;
        VARS(work)->size = size;
        VARS(work)->rise = rise;
        break;
    case COLUMN_END:
        task_kill(task);
        break;
    }
    if (work->scl.y < 0.1F) {
        work->scl.y = 0.1F;
    }
    beach_water_column_draw(task);
}

/* draw the two sheets with alpha, scaled to the column. twice shade is added
 * to their material's color */
static void beach_water_column_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;

    njSetTexture(&beach_water_column_texlist);
    njPushMatrix(NULL);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    color.r = color.g = color.b = VARS(work)->shade * 0.8F;
    draw_set_material(0.0F, color.r, color.g, color.b);
    njTranslateV(NULL, &work->pos);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
    color.r = color.g = color.b = VARS(work)->shade * 2.0F;
    draw_set_material(0.0F, color.r, color.g, color.b);
    /* @hack spelling: each sheet through its pointer's address, which takes
     * draw_tree's address into R3 and then R2 (0x0c902b80 and 0x0c902b88).
     * without it, 12 of 103 words differ */
    {
        NJS_OBJECT **slot = &beach_water_column_inner;
        NJS_OBJECT *sheet = *slot;

        draw_tree(sheet);
    }
    {
        NJS_OBJECT **slot = &beach_water_column_outer;
        NJS_OBJECT *sheet = *slot;

        draw_tree(sheet);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
    njPopMatrix(1);
}

static void beach_water_column_destroy(struct task *task)
{
}
