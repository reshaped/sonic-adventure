/* O PALM, a palm tree, which burns in PAST_FIRE */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/palm.h"
#include "adv03/past.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT palm_object;
extern NJS_ACTION palm_burning_action;

/* a flag of work->flags, set in PAST_FIRE: the palm draws palm_burning_action
 * instead of its plain model */
#define PALM_BURNING 0x200

/* in PALM_FIRE, palm_burning_action plays FIRE_SPEED of a frame each step,
 * from 0 up to FIRE_FRAMES and round again */
#define FIRE_SPEED 0.4F
#define FIRE_FRAMES 15.0F

enum palm_mode { PALM_INIT, PALM_STILL, PALM_FIRE };

struct palm_vars {
    Uint8 unk_00[8];
    Float frame; /* palm_burning_action's frame */
};

#define VARS(work) ((struct palm_vars *)&(work)->vars)

static void palm_draw(struct task *task);

void palm_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PALM_INIT:
        task->draw = palm_draw;
        if ((Uint8)stage_get_stage_act() == PAST_FIRE) {
            work->flags |= PALM_BURNING;
            work->mode = PALM_FIRE;
        } else {
            work->mode = PALM_STILL;
        }
        break;
    case PALM_STILL:
        break;
    case PALM_FIRE:
        VARS(work)->frame += FIRE_SPEED;
        if (VARS(work)->frame >= FIRE_FRAMES) {
            VARS(work)->frame = 0.0F;
        }
        break;
    }
    palm_draw(task);
}

static void palm_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if (work->flags & PALM_BURNING) {
        draw_action(&palm_burning_action, VARS(work)->frame);
    } else {
        draw_tree(&palm_object);
    }
    njPopMatrix(1);
}
