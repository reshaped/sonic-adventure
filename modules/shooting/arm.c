/* the arm: ARM, which swings until anything touches it, and then bursts */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "effect/bomb.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "shooting/arm.h"
#include "shooting/reticle.h"
#include "shooting/shooting.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"

struct arm_vars {
    Float frame; /* of the swing's action */
};

#define VARS(work) ((struct arm_vars *)&(work)->vars)

enum arm_mode { ARM_INIT = 0, ARM_SWING = 1, ARM_GONE = 3 };

#define ARM_POINTS 50
#define SWING_FRAMES 20.0F /* the frames the swing's action loops through */

extern NJS_TEXLIST shooting0_texlist;
extern NJS_ACTION arm_action;

static void arm_draw(struct task *task);
static void arm_destroy(struct task *task);
static void arm_take_hit(struct task *task);

static struct collision_shape arm_shape = {SHAPE_SPHERE | SHAPE_HURTABLE,
                                           ATTACK(1) | SHAPE_HURTS_PLAYERS,
                                           PUSH_FIRM(PUSH_ALL),
                                           {-12.28F, -42.62F, 0.0F},
                                           40.0F,
                                           0.0F,
                                           0.0F,
                                           {0},
                                           0,
                                           NULL};

/* the arm stops and hides while an event or the score plays. it goes once
 * the course is flown */
void arm_step(struct task *task)
{
    /* @hack redundant: 12 bytes of stack no instruction reads or writes, ADD
     * #-12,R15 at 0x0c906b26; without them, 82 of 86 words differ */
    NJS_POINT3 unused;
    struct task_work *work = task->work;

    if (!shooting_event_playing()) {
        /* @hack spelling: a switch of one case, which makes ARM_GONE in R2 (MOV
         * #3,R2 at 0x0c906b3c); as an if, it takes R1, and 2 of 86 words
         * differ */
        switch (shooting_stage_mode) {
        case SHOOTING_CLEAR:
            work->mode = ARM_GONE;
            break;
        }
        switch (work->mode) {
        case ARM_INIT:
            work->mode = ARM_SWING;
            VARS(work)->frame = 0.0F;
            task->destructor = arm_destroy;
            task->draw = arm_draw;
            enemy_alloc_record(task, work);
            collision_init(task, &arm_shape, COLLISION_ITEM, 1, 1);
            break;
        case ARM_SWING:
            arm_draw(task);
            VARS(work)->frame += 1.0F;
            if (VARS(work)->frame >= SWING_FRAMES) {
                VARS(work)->frame = 0.0F;
            }
            arm_take_hit(task);
            collision_register(work);
            break;
        case ARM_GONE:
            placement_kill(task);
        }
        task_step_children(task);
    }
}

/* at a frame's first step only, and not while an event or the score plays */
static void arm_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step != 0) {
        return;
    }
    if (shooting_event_playing()) {
        return;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njSetTexture(&shooting0_texlist);
    draw_action(&arm_action, VARS(work)->frame);
    njPopMatrix(1);
}

static void arm_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}

/* anything that touches the arm bursts it, for ARM_POINTS */
static void arm_take_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->collision->hit != NULL) {
        lock_release(task);
        shooting_score += ARM_POINTS;
        work->timer = 10;
        bomb_create(&work->pos, 3.0F);
        sound_play(SE_SHOT_DOWN, 0, 0, 0);
        work->mode = ARM_GONE;
    }
}
