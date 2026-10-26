/* O HAMMERSW, a button that turns a switch on when Big stands on it or Amy's
 * attack hits it */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/hammer_switch.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "object/switch.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL hammer_switch_button_model;
extern NJS_MODEL hammer_switch_base_model;

#define SWITCH_ID(work) ((int)(work)->scl.x)

enum hammer_switch_mode { HAMMER_SWITCH_INIT, HAMMER_SWITCH_RUN };

static struct collision_shape hammer_switch_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 1.75F, 0.0F},
     3.54F,
     1.75F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static void hammer_switch_init(struct task *task);
static void hammer_switch_shape_init(struct task *task);
static void hammer_switch_collide(struct task *task);
static void hammer_switch_draw(struct task *task);
static void hammer_switch_destroy(struct task *task);
static void hammer_switch_latch(struct task_work *work);
static Bool hammer_switch_pressed(struct task_work *work);

void egg_carrier_hammer_switch_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        switch (work->mode) {
        case HAMMER_SWITCH_INIT:
            hammer_switch_init(task);
            break;
        case HAMMER_SWITCH_RUN:
            /* @bug memory: while the set editor holds the switch, each step
             * calls collision_init again, through hammer_switch_shape_init
             * (BSR at 0x0c925994; its JSR at 0x0c925a02). each call gives the
             * work a new record, and the one the last call gave it leaks */
            hammer_switch_shape_init(task);
            hammer_switch_draw(task);
            break;
        }
        return;
    }
    switch (work->mode) {
    case HAMMER_SWITCH_INIT:
        hammer_switch_init(task);
        break;
    case HAMMER_SWITCH_RUN:
        hammer_switch_draw(task);
        hammer_switch_collide(task);
        break;
    }
}

static void hammer_switch_init(struct task *task)
{
    struct task_work *work = task->work;

    task->destructor = hammer_switch_destroy;
    task->draw = hammer_switch_draw;
    hammer_switch_shape_init(task);
    work->mode = HAMMER_SWITCH_RUN;
}

static void hammer_switch_shape_init(struct task *task)
{
    collision_init(task, hammer_switch_shapes, COLLISION_OBJECT, 1, 1);
    task->work->tick = 0;
}

/* turn the switch on if the last collision checks found the button pressed.
 * then sink the button's shape while the switch is on, and register its
 * record */
static void hammer_switch_collide(struct task *task)
{
    struct task_work *work = task->work;

    hammer_switch_latch(work);
    if (switch_is_on(SWITCH_ID(work))) {
        work->collision->shapes->center.y = -1.02F;
    } else {
        work->collision->shapes->center.y = 1.75F;
    }
    collision_register(task->work);
}

static void hammer_switch_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float down;
    Angle ang;

    if (switch_is_on(SWITCH_ID(work))) {
        down = -2.77F;
    } else {
        down = 0.0F;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_model(&hammer_switch_base_model);
    njTranslate(NULL, 0.0F, down, 0.0F);
    draw_model(&hammer_switch_button_model);
    njPopMatrix(1);
}

static void hammer_switch_destroy(struct task *task)
{
}

/* turn the switch on while the button is pressed. the button never turns it
 * off */
static void hammer_switch_latch(struct task_work *work)
{
    if (hammer_switch_pressed(work)) {
        switch_set(SWITCH_ID(work), TRUE);
    }
}

/* TRUE when Big stands on the button or an attack of Amy's hits it, else
 * FALSE */
static Bool hammer_switch_pressed(struct task_work *work)
{
    if (((work->flags & COLLISION_UNDER) &&
         player_task_character(work->collision->hit->owner) == CHARACTER_BIG) ||
        ((work->flags & COLLISION_DAMAGED) &&
         player_task_character(work->collision->hit->owner) == CHARACTER_AMY)) {
        return TRUE;
    }
    return FALSE;
}
