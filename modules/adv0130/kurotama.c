/* O KUROTAMA, the black egg a player can pick up and carry, which a Chao
 * garden takes in as a Chao egg */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/dat_0ca14ff0.h"
#include "adv0130/kurotama.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/target.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/util.h"

/* an egg that has been carried is killed once more than DOWN_STEPS steps
 * pass after a player lets go of it */
#define DOWN_STEPS 1800

/* TRUE until a player first picks the egg up. until then, the egg counts no
 * steps toward DOWN_STEPS */
#define NEVER_CARRIED(work) ((work)->submode)

enum kurotama_mode { KUROTAMA_INIT, KUROTAMA_DOWN, KUROTAMA_CARRIED };

/* the first, wider cylinder pushes no group, and no group can push it */
static struct collision_shape kurotama_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 2.0F, 0.0F},
     5.5F,
     4.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 2.0F, 0.0F},
     2.0F,
     4.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static void kurotama_draw(struct task *task);

void egg_carrier_kurotama_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    if (event_flag_check(FLAG_BLACK_EGG_IN)) {
        return;
    }
    switch (work->mode) {
    case KUROTAMA_INIT:
        if (!placement_editing(task)) {
            collision_init(task, kurotama_shapes, COLLISION_OBJECT, 1, 2);
        }
        carry_set_kind(work, motion, CARRY_KIND_SMALL);
        work->flags |= CARRY_WALLS;
        task->draw = kurotama_draw;
        NEVER_CARRIED(work) = TRUE;
        work->mode = KUROTAMA_DOWN;
        break;
    case KUROTAMA_DOWN:
        carry_step_small(task);
        if (work->flags & CARRY_HELD) {
            player_set_held_kind(0, HELD_BLACK_EGG);
            NEVER_CARRIED(work) = FALSE;
            work->mode = KUROTAMA_CARRIED;
            break;
        }
        if (!NEVER_CARRIED(work)) {
            ++work->timer;
            if (TASK_TIMER(work) > DOWN_STEPS) {
                task_kill(task);
                return;
            }
        }
        break;
    case KUROTAMA_CARRIED:
        work->ang.y = -player_work[0]->ang.y;
        carry_step_small(task);
        if (work->flags & CARRY_HELD) {
            placement_hold(task);
        } else {
            task->placement->flags &= ~PLACEMENT_HELD;
            player_set_held_kind(0, 0);
            work->mode = KUROTAMA_DOWN;
            work->timer = 0;
        }
        break;
    }
    kurotama_draw(task);
}

static void kurotama_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&ec_alife_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(&kurotama_object);
    njPopMatrix(1);
}
