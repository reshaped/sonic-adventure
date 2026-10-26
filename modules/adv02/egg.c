/* the Chao egg the egg stand holds, which a player can carry to another act */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/egg.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "player/player.h"
#include "player/target.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 3
#include "adv02/egg/text.inc"

/* an egg ends once it has lain in EGG_DOWN, out of the stand, more steps than
 * this */
#define IDLE_STEPS 1800

#define RADIUS 50.0F /* the collision record's radius */

/* the egg's modes. egg_step sets EGG_DOWN last, and egg_update moves an egg
 * only between EGG_DOWN and EGG_HELD. no code sets EGG_UNK_6, so egg_update's
 * EGG_UNK_3 and EGG_UNK_6 cases never run */
enum egg_mode {
    EGG_PLACED = 0,
    EGG_DOWN = 1,
    EGG_HELD = 2,
    EGG_UNK_3 = 3,
    EGG_UNK_6 = 6
};

/* work->timer holds the egg's stage and act, as stage_get_stage_act returns
 * them */
struct egg_vars {
    Uint8 unk_00[4];
    int idle; /* the steps it has lain since it was put down */
};

#define VARS(work) ((struct egg_vars *)&(work)->vars)

static void egg_update(struct task *task);
static void egg_draw(struct task *task);
static void egg_destroy(struct task *task);

static NJS_TEXNAME egg_textures[1] = {
    {TEXT_1, 0, 0},
};

NJS_TEXLIST egg_texlist = {
#include "adv02/egg/egg_texlist.inc"
};

static NJS_MATERIAL egg_materials[1] = {
#include "adv02/egg/egg_materials.inc"
};

static Sint16 egg_meshes0[116] = {
#include "adv02/egg/egg_meshes0.inc"
};

static NJS_COLOR egg_uvs0[104] = {
#include "adv02/egg/egg_uvs0.inc"
};

static NJS_MESHSET egg_meshsets[1] = {
#include "adv02/egg/egg_meshsets.inc"
};

static NJS_POINT3 egg_points[42] = {
#include "adv02/egg/egg_points.inc"
};

static NJS_VECTOR egg_normals[42] = {
#include "adv02/egg/egg_normals.inc"
};

static NJS_MODEL egg_model = {
#include "adv02/egg/egg_model.inc"
};

static NJS_OBJECT egg_object = {
#include "adv02/egg/egg_object.inc"
};

static struct collision_shape egg_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 2.0F, 0.0F},
     5.5F,
     4.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 2.0F, 0.0F},
     2.0F,
     4.0F,
     0.0F,
     {0},
     0,
     NULL},
};

struct task *egg_task = NULL;
Bool egg_in_stand = FALSE;

/* one egg lives at a time: the first step kills the task while egg_task or
 * FLAG_SILVER_EGG_IN is set. while the set editor holds the task, the step only
 * draws */
void egg_step(struct task *task)
{
    struct task_work *work = task->work;
    Bool gone;

    work->ang.x = 0;
    work->ang.z = 0;
    TASK_TIMER(work) = 0;
    if (placement_editing(task)) {
        egg_draw(task);
        return;
    } else {
        gone = event_flag_check(FLAG_SILVER_EGG_IN);
    }
    if (gone) {
        /* @hack redundant: a goto to the label on the next line, which gives
         * this arm its own tail jump to task_kill, JMP @R2 at 0x0c90f4a2;
         * without it, the arm shares the egg_task arm's, the code runs 32 bytes
         * shorter, and 70 of egg_step's 94 words differ */
        goto kill;
    kill:
        task_kill(task);
        return;
    }
    if (egg_task != NULL) {
        task_kill(task);
        return;
    } else {
        egg_task = task;
        TASK_TIMER(work) = stage_get_stage_act();
        if (player_get_held_kind(0) == HELD_SILVER_EGG) {
            work->mode = EGG_UNK_3;
        } else {
            work->mode = EGG_PLACED;
        }
        collision_init(task, egg_shapes, COLLISION_OBJECT, 1, 2);
        work->collision->radius = RADIUS;
        carry_set_kind(work, task->motion, CARRY_KIND_SMALL);
        work->flags |= CARRY_WALLS;
        task->step = egg_update;
        task->draw = egg_draw;
        task->destructor = egg_destroy;
        work->mode = EGG_DOWN;
    }
}

/* a stage change ends the egg. an act change ends it too, unless it is
 * EGG_HELD, which carries it into the new act */
static void egg_update(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 here = stage_get_stage_act();

    if ((TASK_TIMER(work) & 0xff00) != (here & 0xff00)) {
        task_kill(task);
        return;
    }
    if (TASK_TIMER(work) != here) {
        if (work->mode != EGG_HELD) {
            task_kill(task);
            return;
        } else {
            TASK_TIMER(work) = stage_get_stage_act();
        }
    }
    egg_draw(task);
    if (!egg_in_stand) {
        switch (work->mode) {
        case EGG_PLACED:
        case EGG_DOWN:
            carry_step_small(task);
            if (work->mode == EGG_DOWN) {
                VARS(work)->idle++;
                if (VARS(work)->idle > IDLE_STEPS) {
                    task_kill(task);
                }
            }
            if (work->flags & CARRY_HELD) {
                player_set_held_kind(0, HELD_SILVER_EGG);
                work->mode = EGG_HELD;
            }
            break;
        case EGG_HELD:
            work->ang.y = -player_work[0]->ang.y;
            carry_step_small(task);
            if (!(work->flags & CARRY_HELD)) {
                player_set_held_kind(0, 0);
                VARS(work)->idle = 0;
                work->mode = EGG_DOWN;
            }
            break;
        case EGG_UNK_3:
            break;
        case EGG_UNK_6:
            task_kill(task);
            break;
        }
    }
}

static void egg_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&egg_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&egg_object);
    njPopMatrix(1);
}

static void egg_destroy(struct task *task)
{
    if (egg_task == task) {
        egg_task = NULL;
    }
}
