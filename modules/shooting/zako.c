/* the zako: ZAKO1, a small craft that holds its place on the course. a
 * hit sends it falling away */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "effect/bomb.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "shooting/hud.h"
#include "shooting/shooting.h"
#include "shooting/zako.h"
#include "system/task.h"
#include "system/util.h"

/* a zako's work->vars. as it falls, its work->scl is its velocity */
struct zako_vars {
    Uint32 falling; /* steps left to fall */
    Uint8 unk_04[7];
    Sint8 frame;
};

#define VARS(work) ((struct zako_vars *)&(work)->vars)

enum zako_mode { ZAKO_INIT = 0, ZAKO_RUN = 1, ZAKO_FALL = 3, ZAKO_GONE = 4 };

#define ZAKO_POINTS 30 /* a hit's points */
#define ZAKO_FALL_STEPS 120 /* the steps it falls once hit */
#define ZAKO_FRAMES 15 /* the frames its action loops through */

/* how near the camera it draws zako_near_action rather than
 * zako_far_action */
#define ZAKO_NEAR 500.0F

extern NJS_TEXLIST shooting0_texlist;
extern NJS_ACTION zako_near_action;
extern NJS_ACTION zako_far_action;

static void zako_take_hit(struct task *task);
static void zako_draw(struct task *task);
static void zako_fall(struct task *task);
static void zako_destroy(struct task *task);
static void zako_animate(struct task_work *work);

static struct collision_shape zako_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL,
     {0.0F, 0.0F, 0.0F},
     30.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

void zako1_step(struct task *task)
{
    struct task_work *work = task->work;

    if (shooting_event_playing()) {
        return;
    }
    /* @hack spelling: a switch of one case, which makes ZAKO_GONE in R2 (MOV
     * #4,R2 at 0x0c908c7a); as an if, it takes R1, and 2 of 77 words
     * differ */
    switch (shooting_stage_mode) {
    case SHOOTING_CLEAR:
        work->mode = ZAKO_GONE;
        break;
    }
    switch (work->mode) {
    case ZAKO_INIT:
        work->mode = ZAKO_RUN;
        task->destructor = zako_destroy;
        task->draw = zako_draw;
        enemy_alloc_record(task, work);
        collision_init(task, zako_shapes, COLLISION_ENEMY, 1, 1);
        VARS(work)->frame = 0;
        break;
    case ZAKO_RUN:
        zako_draw(task);
        zako_take_hit(task);
        collision_register(work);
        zako_animate(work);
        break;
    case ZAKO_FALL:
        zako_draw(task);
        zako_fall(task);
        zako_animate(work);
        break;
    case ZAKO_GONE:
        placement_kill(task);
        break;
    }
}

/* a hit scores ZAKO_POINTS and counts as a target destroyed. it sends the
 * zako falling with a puff, at a random velocity that starts upward or
 * level */
static void zako_take_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & COLLISION_DAMAGED) {
        shooting_score_hit(ZAKO_POINTS, work);
        shooting_count_hit();
        work->mode = ZAKO_FALL;
        VARS(work)->falling = ZAKO_FALL_STEPS;
        bomb_create(&work->pos, 1.0F);
        work->scl.x = RANDOM(-5.0F, 5.0F);
        work->scl.y = rand() * (10.0F * RAND_SCALE);
        work->scl.z = RANDOM(-5.0F, 5.0F);
    }
}

static void zako_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *action;
    Angle ang;
    Float distance;

    distance = njDistanceP2P(&camera_work->pos, &work->pos);
    if (distance < ZAKO_NEAR) {
        action = &zako_near_action;
    } else {
        action = &zako_far_action;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSetTexture(&shooting0_texlist);
    draw_action(action, VARS(work)->frame);
    njPopMatrix(1);
}

/* fall for ZAKO_FALL_STEPS, with a puff every 8 steps, then go */
static void zako_fall(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->falling-- > 0) {
        /* @hack spelling: the sums spelled out, which read each scl field
         * with FMOV.S @(R0,R14) (0x0c908e6c); with +=, SHC forms each one's
         * address in R1 first, and 36 of 46 words differ */
        work->pos.x = work->pos.x + work->scl.x;
        work->pos.y = work->pos.y + work->scl.y;
        work->pos.z = work->pos.z + work->scl.z;
        work->scl.y -= 0.2F;
        if (!(VARS(work)->falling & 7)) {
            bomb_create(&work->pos, 1.0F);
        }
    } else {
        work->mode = ZAKO_GONE;
    }
}

static void zako_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}

static void zako_animate(struct task_work *work)
{
    VARS(work)->frame++;
    if (VARS(work)->frame >= ZAKO_FRAMES) {
        VARS(work)->frame = 0;
    }
}
