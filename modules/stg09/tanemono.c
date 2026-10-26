/* Tanemono ornament, which shows its four models in a fixed sequence */
#include <ninja.h>

#include "draw/model.h"
#include "stg09/casino.h"
#include "stg09/tanemono.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MODEL tanemono_model_0;
extern NJS_MODEL tanemono_model_1;
extern NJS_MODEL tanemono_model_2;
extern NJS_MODEL tanemono_model_3;

/* the places in tanemono_models. work->mode is the place shown */
#define TANEMONO_PLACES 22
/* work->tick counts down from this to 0 at each place, so each place shows for
 * 31 steps. tanemono_step starts the count at 0, so the first run moves
 * straight past place 0 */
#define TANEMONO_COUNTDOWN 30

static NJS_MODEL *tanemono_models[TANEMONO_PLACES] = {
    &tanemono_model_0, &tanemono_model_0, &tanemono_model_1, &tanemono_model_1,
    &tanemono_model_3, &tanemono_model_0, &tanemono_model_3, &tanemono_model_0,
    &tanemono_model_3, &tanemono_model_3, &tanemono_model_3, &tanemono_model_0,
    &tanemono_model_0, &tanemono_model_2, &tanemono_model_2, &tanemono_model_3,
    &tanemono_model_0, &tanemono_model_3, &tanemono_model_0, &tanemono_model_3,
    &tanemono_model_3, &tanemono_model_3,
};

static void tanemono_destroy(struct task *task)
{
}

static void tanemono_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(tanemono_models[work->mode]);
    njPopMatrix(1);
}

static void tanemono_run(struct task *task)
{
    struct task_work *work = task->work;

    if (work->tick-- == 0) {
        work->tick = TANEMONO_COUNTDOWN;
        work->mode = (work->mode + 1) % TANEMONO_PLACES;
    }
    tanemono_draw(task);
}

void tanemono_step(struct task *task)
{
    struct task_work *work = task->work;

    work->tick = work->mode = 0;
    task->step = tanemono_run;
    task->draw = tanemono_draw;
    task->destructor = tanemono_destroy;
}
