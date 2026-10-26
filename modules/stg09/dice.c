/* Saico: a die on the tables, which rolls when hit */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "object/common.h"
#include "stg09/casino.h"
#include "stg09/dice.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MODEL dice_model_0;
extern NJS_MODEL dice_model_1;
extern NJS_MODEL dice_model_2;
extern NJS_MODEL dice_model_3;
extern NJS_MODEL dice_model_4;
extern NJS_MODEL dice_model_5;

/* die modes, in work->mode */
#define DICE_WAIT 0
#define DICE_ROLL 1
#define DICE_SLOW 2

struct dice_vars {
    Uint8 delay; /* while the die slows, the steps each face shows, less 1 */
    Uint8 shown; /* the steps the face has shown */
    /* the turned count at which the fast roll ends, and later the one at
     * which the die stops */
    Uint8 last_turn;
    Uint8 turned; /* the turns since the hit */
};

#define VARS(work) ((struct dice_vars *)&(work)->vars)
/* the face showing, 0 to DICE_FACES - 1 */
#define FACE(work) ((work)->submode)

#define DICE_FACES 6
/* the turns the die slows for. the last stops it without turning a face */
#define DICE_SLOW_TURNS 20

static struct collision_shape dice_shape = {
    SHAPE_SPHERE,
    ATTACK(3),
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    10.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};
static NJS_MODEL *dice_models[DICE_FACES] = {
    &dice_model_0, &dice_model_1, &dice_model_2,
    &dice_model_3, &dice_model_4, &dice_model_5,
};

static void dice_destroy(struct task *task)
{
}

static void dice_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(dice_models[FACE(work)]);
    njPopMatrix(1);
}

static void dice_run(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case DICE_WAIT:
        if (work->collision->flags & COLLISION_HIT) {
            VARS(work)->turned = 0;
            VARS(work)->last_turn = RANDOM(25.0F, 30.0F);
            work->mode++;
        }
        break;
    case DICE_ROLL:
        if (++VARS(work)->turned == VARS(work)->last_turn) {
            /* @hack spelling: shown and delay cleared through a pointer, which
             * SHC forms as work plus 8 (ADD #8,R5 at 0x0c908fb0). through
             * VARS(work), 61 of 103 words differ */
            struct dice_vars *vars = VARS(work);

            vars->shown = 0;
            vars->delay = 0;
            VARS(work)->last_turn += DICE_SLOW_TURNS;
            work->mode++;
        }
        FACE(work) = (FACE(work) + 1) % DICE_FACES;
        break;
    case DICE_SLOW:
        if (VARS(work)->shown++ == VARS(work)->delay) {
            if (++VARS(work)->turned != VARS(work)->last_turn) {
                VARS(work)->shown = 0;
                VARS(work)->delay++;
                FACE(work) = (FACE(work) + 1) % DICE_FACES;
                break;
            }
            work->mode = DICE_WAIT;
        }
        break;
    }
    dice_draw(task);
    collision_register(work);
    object_clear_touch(work, NULL);
    /* @hack redundant: a return where control returns anyway, which keeps task
     * in the frame for the call of dice_draw (MOV.L R4,@R15 at 0x0c908f54).
     * without it, 100 of 103 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

void dice_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &dice_shape, COLLISION_OBJECT, 1, 1);
    work->mode = DICE_WAIT;
    task->step = dice_run;
    task->draw = dice_draw;
    task->destructor = dice_destroy;
}
