/* O FALL TREE, a tree that falls once player 0 or 1 comes within FALL_RADIUS
 * of it. the fall marks the tree's placement used, so a tree made again from
 * it lies down from the start. the set editor plays the fall over and over */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/fall_tree.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT fall_tree_object;
extern NJS_ACTION fall_tree_action;

#define FALL_RADIUS 20.0F
/* the fall plays FALL_SPEED of fall_tree_action's frames each step, from 0 to
 * FALL_END, where the tree lies down. up to UPRIGHT the tree draws as its
 * plain model */
#define FALL_SPEED 0.5F
#define FALL_END 28.99F
#define UPRIGHT 0.01F

enum fall_tree_mode {
    FALL_TREE_INIT,
    FALL_TREE_WAIT,
    FALL_TREE_FALL,
    FALL_TREE_DOWN
};

struct fall_tree_vars {
    Uint8 unk_00[8];
    Float frame; /* fall_tree_action's frame */
};

#define VARS(work) ((struct fall_tree_vars *)&(work)->vars)

static void fall_tree_draw(struct task *task);

void fall_tree_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        VARS(work)->frame += FALL_SPEED;
        if (VARS(work)->frame >= FALL_END) {
            VARS(work)->frame = 0.0F;
        }
    } else {
        switch (work->mode) {
        case FALL_TREE_INIT:
            task->draw = fall_tree_draw;
            if (placement_used(task)) {
                VARS(work)->frame = FALL_END;
                work->mode = FALL_TREE_DOWN;
            } else {
                VARS(work)->frame = 0.0F;
                work->mode = FALL_TREE_WAIT;
            }
            break;
        case FALL_TREE_WAIT:
            if (player_in_range(&work->pos, FALL_RADIUS) != 0) {
                placement_set_used(task);
                work->mode = FALL_TREE_FALL;
            }
            break;
        case FALL_TREE_FALL:
            VARS(work)->frame += FALL_SPEED;
            if (VARS(work)->frame >= FALL_END) {
                VARS(work)->frame = FALL_END;
                work->mode = FALL_TREE_DOWN;
            }
            break;
        case FALL_TREE_DOWN:
            break;
        }
    }
    fall_tree_draw(task);
}

static void fall_tree_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if (VARS(work)->frame <= UPRIGHT) {
        draw_tree(&fall_tree_object);
    } else {
        draw_action(&fall_tree_action, VARS(work)->frame);
    }
    njPopMatrix(1);
}
