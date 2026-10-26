/* O EGGCAP hinged lid, which player 0 comes up through at entrance 1 */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/egg_cap.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT egg_cap_object;

/* the hinge's offset along world x from the lid's position */
#define HINGE_X 18.883199F
#define HINGE_TURN 0x800 /* the lid's turn about its hinge each step */
#define HINGE_OPEN 0xa000 /* the hinge's angle with the lid open */

/* the steps the lid stays open after the one it opens on */
#define OPEN_STEPS 10

/* the entrance under the lid, and player 0's speed up through it there */
#define CAP_ENTRANCE 1
#define RISE_SPEED 5.0F

struct egg_cap_vars {
    Uint8 unk_00[8];
    Angle hinge; /* from 0, shut, to HINGE_OPEN */
};

#define VARS(work) ((struct egg_cap_vars *)&(work)->vars)

enum egg_cap_mode {
    CAP_INIT,
    CAP_OPENING, /* no code switches to it */
    CAP_SHUTTING,
    CAP_SHUT,
    CAP_OPEN, /* counts OPEN_STEPS, then shuts, whatever the hinge's angle */
    CAP_EDITING /* while the set editor runs, the lid is only drawn */
};

static void egg_cap_draw(struct task *task);
static void egg_cap_destroy(struct task *task);
static void egg_cap_draw_work(struct task_work *work);

/* what a player stands on, off while the lid is open */
static struct collision_shape egg_cap_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, -5.0F, 0.0F},
     25.0F,
     5.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

void egg_carrier_egg_cap_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case CAP_INIT:
        task->destructor = egg_cap_destroy;
        task->draw = egg_cap_draw;
        /* @hack redundant: the else after the return, which builds
         * collision_init's two 1s in R1 (0x0c925418 to 0x0c925420); without
         * it, in R2, and 3 of 140 words differ */
        if (placement_editing(task)) {
            work->mode = CAP_EDITING;
            return;
        } else {
            collision_init(task, egg_cap_shapes, COLLISION_OBJECT, 1, 1);
            if (stage_get_entrance() == CAP_ENTRANCE) {
                player_set_speed(0, 0.0F, RISE_SPEED, 0.0F);
                work->mode = CAP_OPEN;
                work->collision->shapes->flags |= SHAPE_OFF;
            } else {
                work->mode = CAP_SHUT;
                work->collision->shapes->flags &= ~SHAPE_OFF;
            }
        }
        break;
    case CAP_SHUT:
        break;
    case CAP_OPEN:
        if (TASK_TIMER(work)++ < OPEN_STEPS) {
            break;
        }
        work->mode = CAP_SHUTTING;
        break;
    case CAP_OPENING:
        VARS(work)->hinge += HINGE_TURN;
        if (VARS(work)->hinge < HINGE_OPEN) {
            break;
        }
        VARS(work)->hinge = HINGE_OPEN;
        work->mode = CAP_OPEN;
        work->timer = 0;
        break;
    case CAP_SHUTTING:
        /* @hack spelling: -HINGE_TURN added, a constant of its own, which SHC
         * loads from the pool for the add (MOV.W at 0x0c9254de) as
         * CAP_OPENING loads 0x800 (0x0c9254c2); with -=, SHC loads 0x800 once
         * into R5 ahead of the switch, the function runs 56 bytes short and
         * 130 of 140 words differ */
        VARS(work)->hinge += -HINGE_TURN;
        if (VARS(work)->hinge > 0) {
            break;
        }
        VARS(work)->hinge = 0;
        work->mode = CAP_SHUT;
        work->collision->shapes->flags &= ~SHAPE_OFF;
        break;
    }
    egg_cap_draw_work(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void egg_cap_draw(struct task *task)
{
    struct task_work *work = task->work;

    egg_cap_draw_work(work);
}

static void egg_cap_destroy(struct task *task)
{
}

static void egg_cap_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njTranslate(NULL, HINGE_X, 0.0F, 0.0F);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, VARS(work)->hinge, ang);
    draw_tree(&egg_cap_object);
    njPopMatrix(1);
}
