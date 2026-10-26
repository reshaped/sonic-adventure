/* O TREE, a tree that burns in PAST_FIRE */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/past.h"
#include "adv03/tree.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT tree_object;
extern NJS_ACTION tree_burning_action;

/* a flag of work->flags, set in PAST_FIRE: the tree draws tree_burning_action
 * instead of its plain model */
#define TREE_BURNING 0x200

/* in TREE_FIRE, tree_burning_action plays FIRE_SPEED of a frame each step,
 * from 0 up to FIRE_FRAMES and round again */
#define FIRE_SPEED 0.5F
#define FIRE_FRAMES 15.0F

enum tree_mode { TREE_INIT, TREE_STILL, TREE_FIRE };

struct tree_vars {
    Uint8 unk_00[8];
    Float frame; /* tree_burning_action's frame */
};

#define VARS(work) ((struct tree_vars *)&(work)->vars)

static void tree_draw(struct task *task);

void tree_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case TREE_INIT:
        task->draw = tree_draw;
        if ((Uint8)stage_get_stage_act() == PAST_FIRE) {
            work->flags |= TREE_BURNING;
            work->mode = TREE_FIRE;
        } else {
            work->mode = TREE_STILL;
        }
        break;
    case TREE_STILL:
        break;
    case TREE_FIRE:
        VARS(work)->frame += FIRE_SPEED;
        if (VARS(work)->frame >= FIRE_FRAMES) {
            VARS(work)->frame = 0.0F;
        }
        break;
    }
    tree_draw(task);
}

static void tree_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if (work->flags & TREE_BURNING) {
        draw_action(&tree_burning_action, VARS(work)->frame);
    } else {
        draw_tree(&tree_object);
    }
    njPopMatrix(1);
}
