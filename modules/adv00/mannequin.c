/* O MANNEQUIN mannequin that players pick up and carry */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/mannequin.h"
#include "adv00/texlists.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/common.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT mannequin_object;
extern NJS_MOTION mannequin_motion;

/* the model's height above the mannequin's position */
#define MANNEQUIN_LIFT 8.0F
/* the shadow's height above the mannequin's position. the draw shows the shadow
 * only while the mannequin rests on something */
#define MANNEQUIN_SHADOW_LIFT 0.1F

enum mannequin_mode { MANNEQUIN_INIT, MANNEQUIN_LYING, MANNEQUIN_HELD };

struct mannequin_vars {
    Uint8 unk_00[8];
    Float lift;
};

#define VARS(work) ((struct mannequin_vars *)&(work)->vars)

static void mannequin_draw(struct task *task);

/* clang-format off */
static struct collision_shape mannequin_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 1.0F, 0.0F}, 5.5F, 10.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL, {0.0F, 1.0F, 0.0F},
     5.0F, 10.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

void mannequin_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    if (placement_try_leave(task)) {
        return;
    }
    if (event_in_control()) {
        return;
    }
    switch (work->mode) {
    case MANNEQUIN_INIT:
        collision_init(task, mannequin_shapes, COLLISION_OBJECT, 1, 2);
        carry_set_kind(work, motion, CARRY_KIND_SMALL);
        work->flags |= CARRY_WALLS;
        task->draw = mannequin_draw;
        VARS(work)->lift = MANNEQUIN_LIFT;
        work->mode = MANNEQUIN_LYING;
        break;
    case MANNEQUIN_LYING:
        carry_step_small(task);
        if (work->flags & CARRY_HELD) {
            work->mode = MANNEQUIN_HELD;
        }
        break;
    case MANNEQUIN_HELD:
        work->ang.y = -player_work[0]->ang.y;
        carry_step_small(task);
        if (work->flags & CARRY_HELD) {
            placement_hold(task);
        } else {
            work->mode = MANNEQUIN_LYING;
        }
        break;
    }
    mannequin_draw(task);
}

/* draw the mannequin posed at its motion's first frame, and its shadow while
 * it rests on something */
static void mannequin_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njSetTexture(&ss_people_texlist);
    njTranslate(NULL, work->pos.x, work->pos.y + VARS(work)->lift, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_motion(&mannequin_object, &mannequin_motion, 0.0F);
    if (work->flags & CARRY_GROUND) {
        texture_set_regular();
        njTranslate(NULL, 0.0F, -VARS(work)->lift + MANNEQUIN_SHADOW_LIFT,
                    0.0F);
        njScale(NULL, 1.0F, 1.0F, 1.0F);
        draw_tree(shadow_object);
    }
    njPopMatrix(1);
}
