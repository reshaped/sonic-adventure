/* the balloon, O BALOON in the object lists, and the sign it carries. it
 * bobs and sways until Amy grabs it. then it clears the stage and carries
 * her off */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "event/play.h"
#include "hud/hud.h"
#include "object/balloon.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* Amy grabs the balloon from within GRAB_RANGE of the point GRAB_DEPTH
 * below it. she hangs HANG_DEPTH below it while it dips and rises */
#define GRAB_RANGE 8.0F
#define GRAB_DEPTH 22.0F
#define HANG_DEPTH 24.0F

/* the dip's speed, upward, starts at DIP_SPEED and gains DIP_ACCEL each
 * step. the rise starts once the speed passes 0 */
#define DIP_SPEED (-1.5F)
#define DIP_ACCEL 0.15F

/* the shadow's size with the balloon at work->pos's height. as the bob
 * lifts the balloon, the shadow shrinks by the height */
#define SHADOW_SIZE 4.8F

struct balloon_vars {
    /* while the balloon waits and rises, steps paces its bob, sway and
     * squash. while it dips, the union holds its speed instead. the rise's
     * count then starts from the last speed's bits, read as a Uint32 */
    union {
        Uint32 steps; /* while it waits and rises */
        Float speed; /* while it dips: upward, per step */
    } move;
    Uint8 unk_04[4];
    Float height; /* above work->pos */
};

#define VARS(work) ((struct balloon_vars *)&(work)->vars)

enum balloon_mode {
    BALLOON_INIT,
    BALLOON_WAIT, /* bobs and sways until Amy grabs it */
    BALLOON_DIP, /* sinks with Amy, slowing to a stop */
    BALLOON_RISE, /* carries Amy up until an event takes control */
    BALLOON_DONE
};

/* balloon_shapes' third shape, a rectangle, goes unread: balloon_step has
 * collision_init copy only the first two */
/* clang-format off */
static struct collision_shape balloon_shapes[] = {
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 17.0F, 0.0F},
     17.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F},
     1.0F, 20.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {4.0F, -22.0F, 0.0F},
     5.0F, 4.0F, 1.0F, {0}, 0, NULL},
};
/* clang-format on */

static void balloon_start(struct task_work *work);
static void balloon_wait(struct task_work *work);
static void balloon_dip(struct task_work *work);
static void balloon_rise(struct task_work *work);
static void balloon_draw(struct task *task);
static void balloon_destroy(struct task *task);
static void balloon_bob(struct task_work *work);
static void balloon_sway(struct task_work *work);
static void balloon_squash(struct task_work *work);

void balloon_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BALLOON_INIT:
        task->destructor = balloon_destroy;
        task->draw = balloon_draw;
        collision_init(task, balloon_shapes, COLLISION_OBJECT, 1, 2);
        balloon_start(work);
        break;
    case BALLOON_WAIT:
        balloon_wait(work);
        break;
    case BALLOON_DIP:
        balloon_dip(work);
        break;
    case BALLOON_RISE:
        balloon_rise(work);
        break;
        /* @hack redundant: a second break, which loads placement_kill's address
         * into R2 and BALLOON_INIT into R3 (0x8c05cfa6, 0x8c05cfac); without
         * it, the two trade, and 4 of 65 words differ */
        break;
    case BALLOON_DONE:
        placement_kill(task);
        /* fall through */
    default:
        work->mode = BALLOON_INIT;
    }
    balloon_draw(task);
}

/* set the balloon waiting at work->pos, level and unscaled, with its step
 * count at random below 100 */
static void balloon_start(struct task_work *work)
{
    /* @hack spelling: the count through a local, which SHC holds in FR15,
     * saved on entry (0x8c05cfc6, 0x8c05cfde); stored straight from rand's
     * product, 41 of 43 words differ */
    Float count;

    work->mode = BALLOON_WAIT;
    count = rand() * (100.0F * RAND_SCALE);
    VARS(work)->move.steps = count;
    VARS(work)->height = 0.0F;
    work->ang.z = 0;
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    work->scl.z = 1.0F;
}

static void balloon_wait(struct task_work *work)
{
    int character;
    struct task *task;
    struct task_work *player;
    Float dx;
    Float dy;
    Float dz;
    Float distance;

    if (edit_mode) {
        return;
    }
    balloon_bob(work);
    balloon_sway(work);
    balloon_squash(work);
    VARS(work)->move.steps++;
    shadow_draw(work, SHADOW_SIZE - VARS(work)->height);
    character = player_get_character(0);
    if (character == CHARACTER_AMY) {
        task = player_get_task(0);
        player = task->work;
        dx = player->pos.x - work->pos.x;
        dy = player->pos.y - work->pos.y - VARS(work)->height + GRAB_DEPTH;
        dz = player->pos.z - work->pos.z;
        /* @hack spelling: each square stored back in its difference, which
         * SHC multiplies with FMUL and sums with FADD (0x8c05d0da to
         * 0x8c05d0ea); squared in njSqrt's argument, it fuses them with
         * FMAC, and 54 of 104 words differ */
        dx = dx * dx;
        dy = dy * dy;
        dz = dz * dz;
        distance = njSqrt(dx + dy + dz);
        if (distance < GRAB_RANGE) {
            work->mode = BALLOON_DIP;
            player_set_mode(0, PLAYER_MODE_STILL);
            VARS(work)->move.speed = DIP_SPEED;
            time_stop();
            stage_clear();
        }
    }
    collision_register(work);
}

static void balloon_dip(struct task_work *work)
{
    /* @hack spelling: DIP_ACCEL through a local, which SHC loads first and
     * keeps at @R15 for the end (0x8c05d142, 0x8c05d17a); added as
     * DIP_ACCEL, 49 of 50 words differ */
    Float accel = DIP_ACCEL;
    struct task *task;
    struct task_work *player;

    VARS(work)->height += VARS(work)->move.speed;
    task = player_get_task(0);
    player = task->work;
    player->pos = work->pos;
    player->pos.y += VARS(work)->height - HANG_DEPTH;
    VARS(work)->move.speed += accel;
    if (VARS(work)->move.speed > 0.0F) {
        work->mode = BALLOON_RISE;
    }
}

static void balloon_rise(struct task_work *work)
{
    struct task *task;
    struct task_work *player;

    balloon_sway(work);
    balloon_squash(work);
    VARS(work)->height +=
        ((njSin(VARS(work)->move.steps * 200) * 0.5F) + 0.3F) * 0.5F;
    VARS(work)->move.steps++;
    task = player_get_task(0);
    player = task->work;
    player->pos = work->pos;
    player->pos.y += VARS(work)->height - HANG_DEPTH;
    if (event_in_control()) {
        work->mode = BALLOON_DONE;
    }
}

static void balloon_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_regular();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + VARS(work)->height,
                work->pos.z);
    TURN(njRotateZ, work->ang.z);
    njScaleV(NULL, &work->scl);
    draw_model(&balloon_model);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 0.0F);
    TURN(njRotateZ, -work->ang.z);
    njScale(NULL, 1.0F / work->scl.x, 1.0F / work->scl.y, 1.0F / work->scl.z);
    draw_model(&balloon_knot_model);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 0.0F);
    njScale(NULL, 1.0F, 0.5F, 1.0F);
    draw_model(&balloon_sign_model);
    njScale(NULL, 1.0F, 2.0F, 1.0F);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, -9.0F, 0.0F);
    draw_model(&balloon_sign_frame_model);
    njPopMatrix(4);
}

static void balloon_destroy(struct task *task)
{
}

static void balloon_bob(struct task_work *work)
{
    VARS(work)->height = njSin((VARS(work)->move.steps + 800) * 256) * 4.0F;
}

static void balloon_sway(struct task_work *work)
{
    work->ang.z = njCos((VARS(work)->move.steps + 2000) * 200) * 4096.0F;
}

static void balloon_squash(struct task_work *work)
{
    work->scl.x = 1.0F - (njCos(VARS(work)->move.steps * 2048) * 0.05F);
    work->scl.y = 1.0F - (njSin(VARS(work)->move.steps * 2048) * 0.05F);
}
