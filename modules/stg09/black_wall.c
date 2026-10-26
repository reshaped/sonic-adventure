/* KuroKabe: the tables' black walls, whose alpha pulses. one wall is left out
 * while ShoogaiL is down, and another while ShoogaiR is */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"
#include "stg09/black_wall.h"
#include "stg09/casino.h"
#include "stg09/pinball.h"
#include "system/task.h"

extern NJS_MODEL black_wall_model;
extern NJS_MODEL black_wall_a_model;
extern NJS_MODEL black_wall_b_model;

struct black_wall_vars {
    Uint8 unk_00[8];
    Angle phase; /* the alpha pulse's angle */
};

#define VARS(work) ((struct black_wall_vars *)&(work)->vars)

/* in work->flags. the step sets it, and the paused draw clears it. the
 * pulse moves on only while it is set */
#define BLACK_WALL_PULSING 0x100

#define BLACK_WALL_PULSE 0x400 /* the phase's turn each step */

static void black_wall_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void black_wall_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & BLACK_WALL_PULSING) {
        VARS(work)->phase += BLACK_WALL_PULSE;
    }
    draw_set_material((njSin(VARS(work)->phase) * (1.0F / 3.0F)) - 0.3F, 1.0F,
                      1.0F, 1.0F);
    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    if (!(casino_flags & CASINO_SHOOGAI_L_DOWN)) {
        draw_model(&black_wall_a_model);
    }
    draw_model(&black_wall_model);
    if (!(casino_flags & CASINO_SHOOGAI_R_DOWN)) {
        draw_model(&black_wall_b_model);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void black_wall_draw_held(struct task *task)
{
    task->work->flags &= ~BLACK_WALL_PULSING;
    black_wall_draw(task);
}

static void black_wall_run(struct task *task)
{
    task->work->flags |= BLACK_WALL_PULSING;
    black_wall_draw(task);
}

void black_wall_step(struct task *task)
{
    casino_flags &= ~(CASINO_SHOOGAI_L_DOWN | CASINO_SHOOGAI_R_DOWN);
    task->step = black_wall_run;
    task->draw = black_wall_draw_held;
    task->destructor = black_wall_destroy;
}
