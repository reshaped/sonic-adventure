/* the carrier's hodai: six batteries on the egg carrier's hull. each fires
 * shells at player 0 while hodai_may_fire is set. one hit downs a hodai,
 * which then smokes. once the carrier goes down, a hodai only draws: it
 * stops firing and taking hits */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "enemy/enemy.h"
#include "player/lockon.h"
#include "player/player.h"
#include "shooting/carrier.h"
#include "shooting/carrier_hodai.h"
#include "shooting/hud.h"
#include "shooting/kirai.h"
#include "shooting/reticle.h"
#include "shooting/shooting.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* shared by a hodai and its shells */
struct carrier_hodai_vars {
    Float frame; /* how far a shell's action has played */
    /* steps until a hodai fires again or a shell goes. a downed hodai
     * counts it up instead, for its smoke */
    Sint16 wait;
    Uint8 unk_06[2];
    /* @hack type: the hits and the stretch as one array, whose element SHC
     * reaches through its address, the work plus 16 (ADD #16 at 0x0c9063dc and
     * 0x0c9063fa). as two fields, SHC indexes the work through R0, and 46
     * of carrier_hodai_step's 84 words and 40 of carrier_hodai_take_hit's
     * 46 differ */
    Sint8 counts[2];
};

#define VARS(work) ((struct carrier_hodai_vars *)&(work)->vars)
/* a hodai's counts: the hits it has left, and the stretch of the course its
 * scl.x picks. nothing here reads the stretch */
#define HITS(work) (VARS(work)->counts[0])
#define STRETCH(work) (VARS(work)->counts[1])

/* the work mode of a hodai and a shell */
enum carrier_hodai_mode {
    HODAI_INIT,
    HODAI_RUN,
    HODAI_IDLE, /* the carrier is going down */
    HODAI_DOWN, /* shot down, and smoking */
    HODAI_KILL
};

#define HODAI_POINTS 50
#define SHELL_POINTS 5

/* how near the camera a hodai fires, squared */
#define HODAI_RANGE (7000.0F * 7000.0F)

#define SHELL_SPEED 30.0F /* per step */
#define SHELL_STEPS 240

extern NJS_TEXLIST shooting0_texlist;
extern NJS_OBJECT hodai_object;
extern NJS_ACTION hodai_shell_action;

static const NJS_VECTOR carrier_hodai_smoke_speed = {0.0F, 6.0F, 0.0F};

static struct collision_shape carrier_hodai_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     20.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct collision_shape carrier_hodai_shell_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE,
    ATTACK(1) | SHAPE_HURTS_PLAYERS,
    PUSH_ALL,
    {0.0F, 0.0F, 0.0F},
    30.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL};

static void carrier_hodai_draw(struct task *task);
static void carrier_hodai_destroy(struct task *task);
static void carrier_hodai_take_hit(struct task *task);
static void carrier_hodai_fire(struct task *task);
static void carrier_hodai_smoke(struct task *task);
static void carrier_hodai_idle_if_down(struct task *task);
static void carrier_hodai_shell_draw(struct task *task);
static void carrier_hodai_shell_step(struct task *task);
static void carrier_hodai_shell_aim(struct task_work *work);
static void carrier_hodai_shell_take_hit(struct task *task);
static void carrier_hodai_shell_destroy(struct task *task);

void carrier_hodai_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case HODAI_INIT:
        task->destructor = carrier_hodai_destroy;
        task->draw = carrier_hodai_draw;
        enemy_alloc_record(task, work);
        collision_init(task, carrier_hodai_shapes, COLLISION_ITEM, 1, 1);
        STRETCH(work) = (int)work->scl.x;
        HITS(work) = 1;
        VARS(work)->wait = 0;
        VARS(work)->frame = 0.0F;
        work->mode = HODAI_RUN;
        break;
    case HODAI_RUN:
        carrier_hodai_draw(task);
        carrier_hodai_idle_if_down(task);
        carrier_hodai_take_hit(task);
        carrier_hodai_fire(task);
        collision_register(work);
        break;
    case HODAI_DOWN:
        carrier_hodai_draw(task);
        carrier_hodai_smoke(task);
        break;
    case HODAI_IDLE:
        carrier_hodai_draw(task);
        break;
    case HODAI_KILL:
        task_kill(task);
    }
    task_step_children(task);
}

/* on a frame's first step only */
static void carrier_hodai_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step != 0) {
        return;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njSetTexture(&shooting0_texlist);
    draw_tree(&hodai_object);
    njPopMatrix(1);
}

static void carrier_hodai_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}

/* a hit scores HODAI_POINTS and counts as a target destroyed. the last
 * hit sends the hodai down */
static void carrier_hodai_take_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & COLLISION_DAMAGED) {
        HITS(work)--;
        lock_release(task);
        shooting_score_hit(HODAI_POINTS, work);
        shooting_count_hit();
        if (HITS(work) <= 0) {
            bomb_create(&work->pos, 6.0F);
            work->mode = HODAI_DOWN;
        } else {
            bomb_create(&work->pos, 3.0F);
        }
    }
}

/* fire a shell once the wait runs out, then wait 120 to 199 steps. the
 * hodai waits and fires only while it is on the screen. it fires only while
 * hodai_may_fire is set and the camera is within HODAI_RANGE */
static void carrier_hodai_fire(struct task *task)
{
    struct task_work *work = task->work;

    if (draw_on_screen(work)) {
        if (VARS(work)->wait == 0) {
            if (hodai_may_fire && hodai_near(task, HODAI_RANGE)) {
                sound_play(SE_HODAI, 0, 0, 0);
                task_create_child(TASK_WORK, carrier_hodai_shell_step, task);
                VARS(work)->wait = (int)(rand() * (80.0F * RAND_SCALE)) + 120;
            }
        } else {
            VARS(work)->wait--;
        }
    }
}

/* a puff of smoke every 4 steps */
static void carrier_hodai_smoke(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed;

    speed = carrier_hodai_smoke_speed;
    VARS(work)->wait++;
    if (!(VARS(work)->wait & 3)) {
        dust_create(&work->pos, &speed, 9.0F);
    }
}

/* the carrier sets its down as its gauge empties */
static void carrier_hodai_idle_if_down(struct task *task)
{
    struct task_work *carrier = task->parent->work;

    if (CARRIER_VARS(carrier)->down) {
        task->work->mode = HODAI_IDLE;
    }
}

/* the shell, mirrored in z, at a frame's first step only */
static void carrier_hodai_shell_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step != 0) {
        return;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njScale(NULL, 1.0F, 1.0F, -1.0F);
    njSetTexture(&shooting0_texlist);
    draw_action(&hodai_shell_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* a shell the hodai fires at player 0, which flies for SHELL_STEPS. its
 * scl holds its speed */
static void carrier_hodai_shell_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *hodai = task->parent->work;

    switch (work->mode) {
    case HODAI_INIT:
        enemy_alloc_record(task, work);
        collision_init(task, &carrier_hodai_shell_shape, COLLISION_ENEMY, 1, 1);
        task->destructor = carrier_hodai_shell_destroy;
        task->draw = carrier_hodai_shell_draw;
        work->ang.x = work->ang.y = work->ang.z = 0;
        work->pos = hodai->pos;
        carrier_hodai_shell_aim(work);
        VARS(work)->wait = SHELL_STEPS;
        VARS(work)->frame = 0.0F;
        work->mode = HODAI_RUN;
        break;
    case HODAI_RUN:
        carrier_hodai_shell_draw(task);
        work->pos.x += work->scl.x;
        work->pos.y += work->scl.y;
        work->pos.z += work->scl.z;
        if (--VARS(work)->wait <= 0) {
            work->mode = HODAI_KILL;
        }
        if (VARS(work)->frame >= 60.0F) {
            VARS(work)->frame = 0.0F;
        } else {
            VARS(work)->frame += 1.0F;
        }
        carrier_hodai_shell_take_hit(task);
        collision_register(work);
        break;
    case HODAI_KILL:
        task_kill(task);
        break;
    }
}

/* aim the shell at player 0, at SHELL_SPEED, and turn it toward player 0 */
static void carrier_hodai_shell_aim(struct task_work *work)
{
    NJS_VECTOR way;
    Float x;
    Float y;
    Float z;
    Float across;
    Angle ang_y;
    Angle ang_x;

    way.x = x = player_work[0]->pos.x - work->pos.x;
    way.y = y = player_work[0]->pos.y - work->pos.y;
    way.z = z = player_work[0]->pos.z - work->pos.z;
    njUnitVector(&way);
    work->scl.x = way.x * SHELL_SPEED;
    work->scl.y = way.y * SHELL_SPEED;
    work->scl.z = way.z * SHELL_SPEED;
    across = njSqrt((x * x) + (z * z));
    ang_y = 0x4000 - (Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
    ang_x = -(Angle)(atan2f(y, across) * ANGLE_PER_RADIAN);
    work->ang.x = ang_x;
    work->ang.y = ang_y;
}

/* a hit bursts the shell, for SHELL_POINTS */
static void carrier_hodai_shell_take_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & COLLISION_DAMAGED) {
        lock_release(task);
        shooting_score_hit(SHELL_POINTS, work);
        work->mode = HODAI_KILL;
        bomb_create(&work->pos, 3.0F);
        sound_play(SE_HIT, 0, 0, 0);
    }
}

static void carrier_hodai_shell_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}
