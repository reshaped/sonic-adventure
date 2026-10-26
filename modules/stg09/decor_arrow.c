/* KazariYaji: a decorative arrow on the tables, which cycles through four
 * models */
#include <ninja.h>

#include "draw/model.h"
#include "stg09/casino.h"
#include "stg09/decor_arrow.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MODEL decor_arrow_model_0;
extern NJS_MODEL decor_arrow_model_1;
extern NJS_MODEL decor_arrow_model_2;
extern NJS_MODEL decor_arrow_model_3;

#define DECOR_ARROW_MODELS 4
/* work->tick's start as a model shows. the step that finds the tick at 0
 * shows the next model. each model then shows for DECOR_ARROW_COUNTDOWN + 1
 * steps */
#define DECOR_ARROW_COUNTDOWN 4

static NJS_MODEL *decor_arrow_models[DECOR_ARROW_MODELS] = {
    &decor_arrow_model_0,
    &decor_arrow_model_1,
    &decor_arrow_model_2,
    &decor_arrow_model_3,
};

static void decor_arrow_destroy(struct task *task)
{
}

static void decor_arrow_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScaleV(NULL, &work->scl);
    draw_model(decor_arrow_models[work->mode]);
    njPopMatrix(1);
}

static void decor_arrow_run(struct task *task)
{
    struct task_work *work = task->work;

    if (work->tick-- == 0) {
        work->tick = DECOR_ARROW_COUNTDOWN;
        work->mode = (work->mode + 1) % DECOR_ARROW_MODELS;
    }
    decor_arrow_draw(task);
}

void decor_arrow_step(struct task *task)
{
    struct task_work *work = task->work;

    work->tick = work->mode = 0;
    task->step = decor_arrow_run;
    task->draw = decor_arrow_draw;
    task->destructor = decor_arrow_destroy;
}
