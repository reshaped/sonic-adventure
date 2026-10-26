/* targets: things E-102 shoots down, which the stages count by id */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "object/models.h"
#include "object/piece.h"
#include "object/target.h"
#include "player/lockon.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the most targets a stage can start: the size of target_tasks */
#define TARGET_COUNT 64

#define TARGET_DOWN 0x100 /* in work->flags: shot down */

#define BURST_TIME 30 /* steps a target bursts for once it's down */
#define PUFF_TIME 8 /* steps between its puffs */

/* a target's bytes in work->vars. its id is in work->scl.x */
struct target_vars {
    void *param; /* its kind's param in the object list, as placed */
    Sint16 burst; /* steps left to burst */
    Sint16 puff; /* steps to the next puff */
};

#define VARS(work) ((struct target_vars *)&(work)->vars)

enum target_mode { TARGET_STAND, TARGET_BURST, TARGET_GONE };

static const Uint8 target_shape_count = 1;
/* clang-format off */
static struct collision_shape target_shapes[] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE | LOCKON_NO_NUMBER, SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0},
     0, NULL},
};
/* clang-format on */

static Sint16 target_count;
static struct task *target_tasks[TARGET_COUNT];

static void target_draw(struct task *task)
{
    struct task_work *work;

    work = task->work;
    if (work->mode != TARGET_STAND) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    draw_set_palette(DRAW_PLAYER_PALETTE);
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_tree(&target_object);
    njPopMatrix(1);
    draw_set_palette(0);
}

/* shot down, the target lets the shot fly on through it. it then bursts
 * into puffs for BURST_TIME steps, and is gone */
static void target_step(struct task *task)
{
    struct task_work *work;
    struct lockon_target *record;
    NJS_POINT3 pos;

    work = task->work;
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- ID");
        target_draw(task);
        return;
    }
    switch (work->mode) {
    case TARGET_STAND:
        task_step_children(task);
        if (work->flags & COLLISION_DAMAGED) {
            record = LOCKON_TARGET(task);
            record->flags |= LOCKON_HIT;
            lockon_release(task);
            work->flags |= TARGET_DOWN;
            VARS(work)->burst = BURST_TIME;
            VARS(work)->puff = PUFF_TIME;
            work->mode = TARGET_BURST;
            task_destroy_children(task);
        }
        break;
    case TARGET_BURST:
        VARS(work)->puff--;
        if (VARS(work)->puff == 0) {
            VARS(work)->puff = PUFF_TIME;
            pos = work->pos;
            pos.x += (0.5F - (rand() * RAND_SCALE)) * 12.0F;
            pos.y += (0.5F - (rand() * RAND_SCALE)) * 12.0F;
            pos.z += (0.5F - (rand() * RAND_SCALE)) * 12.0F;
            bomb_create(&pos, 0.35F);
        }
        VARS(work)->burst--;
        /* @hack spelling: the test and its store as an expression, which
         * puts collision_register's address in R2 (MOV.L at 0x8c05e284);
         * as an if, it takes R3, and 2 of 148 words differ */
        !VARS(work)->burst ? (void)(work->mode = TARGET_GONE) : (void)0;
        return;
    case TARGET_GONE:
        return;
    }
    target_draw(task);
    collision_register(work);
}

void target_start(struct task *task)
{
    struct task *child;

    collision_init(task, target_shapes, COLLISION_ITEM, 1, target_shape_count);
    task->motion = heap_calloc(1, sizeof(struct lockon_target));
    task->step = target_step;
    task->draw = target_draw;
    child = task_create_child(TASK_WORK, target_blink_start, task);
    if (child != NULL) {
        child->work->pos = task->work->pos;
    }
    target_tasks[target_count++] = task;
}

Bool target_cleared(int id)
{
    struct task_work *work;
    int count;
    int its_id;
    int i;

    count = 0;
    for (i = 0; i < target_count; i++) {
        work = target_tasks[i]->work;
        /* @hack spelling: the placed id through a local, which takes a
         * third saved register, R14, for TARGET_DOWN (MOV.W at 0x8c05e30a);
         * compared in place, 21 of 37 words differ */
        its_id = (int)work->scl.x;
        /* @hack spelling: the continue, which makes the loop read target_count
         * again on every pass (MOV.W @R11,R3 at 0x8c05e37c); as an if round
         * the test of TARGET_DOWN, 37 of 37 words differ */
        if (id != its_id) {
            continue;
        }
        if (work->flags & TARGET_DOWN) {
            count++;
        } else {
            return FALSE;
        }
    }
    if (count == 0) {
        return FALSE;
    }
    return TRUE;
}

void target_reset(void)
{
    target_count = 0;
}
