/* fires far below the Egg Carrier's deck, drawn while it flies */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/back_fire.h"
#include "adv0100/texlists.h"
#include "draw/draw.h"
#include "stage/stage.h"
#include "system/task.h"

extern NJS_ACTION back_fire_act0_action;
extern NJS_ACTION back_fire_action;

#define BACK_FIRE_FRAMES 30.0F

struct back_fire_vars {
    Uint8 unk_00[4];
    NJS_ACTION *action;
    Float frame;
};

#define VARS(work) ((struct back_fire_vars *)&(work)->vars)

enum back_fire_mode { BACK_FIRE_INIT, BACK_FIRE_RUN };

static void back_fire_draw(struct task *task);

void egg_carrier_back_fire_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BACK_FIRE_INIT:
        task->draw = back_fire_draw;
        switch ((Uint8)stage_get_stage_act()) {
        case 0:
            VARS(work)->action = &back_fire_act0_action;
            break;
        case 1:
        case 2:
            VARS(work)->action = &back_fire_action;
            break;
        }
        work->mode = BACK_FIRE_RUN;
        break;
    case BACK_FIRE_RUN:
        VARS(work)->frame += 1.0F;
        if (VARS(work)->frame >= BACK_FIRE_FRAMES) {
            VARS(work)->frame = 0.0F;
        }
        break;
    }
    back_fire_draw(task);
}

static void back_fire_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&ec_sky_texlist);
    draw_action(VARS(work)->action, VARS(work)->frame);
}
