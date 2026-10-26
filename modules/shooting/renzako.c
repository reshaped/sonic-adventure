/* the renzako: RENZAKO, a small craft that RENZAKO_HITS hits send down. it
 * falls with a blast every 8 steps */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "effect/bomb.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "shooting/hud.h"
#include "shooting/renzako.h"
#include "shooting/shooting.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

struct renzako_vars {
    Uint8 hits; /* left before it falls */
    Uint8 unk_01[3];
    int falling; /* steps left to fall */
    Float frame; /* how far its action has played */
};

#define VARS(work) ((struct renzako_vars *)&(work)->vars)

enum renzako_mode {
    RENZAKO_INIT = 0,
    RENZAKO_RUN = 1,
    RENZAKO_FALL = 3,
    RENZAKO_GONE = 4
};

#define RENZAKO_POINTS 50 /* each hit's points */
#define RENZAKO_HITS 3 /* the hits that send it down */
#define RENZAKO_FALL_STEPS 90 /* the steps it falls */
#define RENZAKO_FRAMES 15.0F /* the frames its action loops through */

extern NJS_TEXLIST shooting0_texlist;
extern NJS_ACTION renzako_action;

static void renzako_take_hit(struct task *task);
static void renzako_fall(struct task *task);
static void renzako_draw(struct task *task);
static void renzako_destroy(struct task *task);

static struct collision_shape renzako_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL,
     {20.0F, 0.0F, 0.0F},
     30.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL,
     {-20.0F, 0.0F, 0.0F},
     30.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

void renzako_step(struct task *task)
{
    struct task_work *work = task->work;

    if (shooting_event_playing()) {
        return;
    }
    switch (work->mode) {
    case RENZAKO_INIT:
        task->destructor = renzako_destroy;
        task->draw = renzako_draw;
        enemy_alloc_record(task, work);
        collision_init(task, renzako_shapes, COLLISION_ENEMY, 1, 2);
        VARS(work)->hits = RENZAKO_HITS;
        VARS(work)->frame = 0.0F;
        work->mode = RENZAKO_RUN;
        break;
    case RENZAKO_RUN:
        renzako_draw(task);
        renzako_take_hit(task);
        VARS(work)->frame += 1.0F;
        if (VARS(work)->frame >= RENZAKO_FRAMES) {
            VARS(work)->frame = 0.0F;
        }
        collision_register(work);
        break;
    case RENZAKO_FALL:
        renzako_draw(task);
        renzako_fall(task);
        VARS(work)->frame += 1.0F;
        if (VARS(work)->frame >= RENZAKO_FRAMES) {
            VARS(work)->frame = 0.0F;
        }
        break;
    case RENZAKO_GONE:
        placement_kill(task);
        break;
    }
}

/* each hit scores RENZAKO_POINTS and counts as a target destroyed. the
 * last of RENZAKO_HITS sends the renzako down */
static void renzako_take_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & COLLISION_DAMAGED) {
        if (--VARS(work)->hits <= 0) {
            VARS(work)->falling = RENZAKO_FALL_STEPS;
            work->mode = RENZAKO_FALL;
        }
        shooting_score_hit(RENZAKO_POINTS, work);
        shooting_count_hit();
        bomb_create(&work->pos, 1.0F);
    }
}

/* fall 2 a step for RENZAKO_FALL_STEPS, with a blast every 8 steps, and
 * then go */
static void renzako_fall(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->falling-- > 0) {
        work->pos.y -= 2.0F;
        if (!(VARS(work)->falling & 7)) {
            bomb_create(&work->pos, 1.0F);
        }
    } else {
        work->mode = RENZAKO_GONE;
    }
}

/* at a frame's first step only, and not while an event or the score
 * plays */
static void renzako_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step != 0) {
        return;
    }
    if (shooting_event_playing()) {
        return;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSetTexture(&shooting0_texlist);
    draw_action(&renzako_action, VARS(work)->frame);
    njPopMatrix(1);
}

static void renzako_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}
