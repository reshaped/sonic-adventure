/* the spike blocks: TogedaiUD and TogedaiD. each drops along its own y axis
 * once player 0 comes near, and raises dust as it lands */
#include <mathf.h>
#include <ninja.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg07/spike_block.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT spike_block_object;

struct spike_block_vars {
    NJS_POINT3 home;
};

#define VARS(work) ((struct spike_block_vars *)&(work)->vars)

/* in the work's scale, along the block's y axis. BLOCK_DEPTH is where the
 * drop ends, and BLOCK_SPEED the block's move each step of it. both come
 * from the set entry and are below 0. BLOCK_MOVED is how far it has moved */
#define BLOCK_DEPTH(work) ((work)->scl.x)
#define BLOCK_SPEED(work) ((work)->scl.y)
#define BLOCK_MOVED(work) ((work)->scl.z)

/* in the work's flags: the block stays down once it drops */
#define BLOCK_STAYS 0x100

enum spike_block_mode {
    BLOCK_WAIT,
    BLOCK_DROP,
    BLOCK_LAND,
    BLOCK_RISE,
    BLOCK_REST,
    BLOCK_DOWN
};

/* the block drops once player 0 is within BLOCK_REACH. it then waits
 * LAND_STEPS down, and REST_STEPS back up */
#define BLOCK_REACH 60.0F
#define LAND_STEPS 30
#define REST_STEPS 10

/* the block rises at RISE_SHARE of its drop's speed */
#define RISE_SHARE 0.1F

/* the dust starts DUST_AWAY from the block toward the camera, and
 * DUST_BELOW under it. the block's angles turn that offset. the dust is
 * DUST_SCALE times a puff's size */
#define DUST_AWAY 20.0F
#define DUST_BELOW 12.0F
#define DUST_SCALE 3.0F

#define SE_DROP 204

/* the block, solid, and its spikes below, which hurt a player */
/* clang-format off */
static struct collision_shape spike_block_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 2.0F, 0.0F},
     10.0F, 8.0F, 10.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(3) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, -10.0F, 0.0F}, 9.0F, 0.0F, 0.0F,
     {0}, 0, NULL},
};
/* clang-format on */

static void spike_block_destroy(struct task *task)
{
}

static void spike_block_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&spike_block_object);
    njPopMatrix(1);
}

static void spike_block_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos = &work->pos;
    struct angles *ang = &work->ang;
    NJS_POINT3 *player = &player_work[0]->pos;
    NJS_VECTOR move = {0.0F, 0.0F, 0.0F};
    NJS_POINT3 dust;
    NJS_VECTOR side;
    Angle toward;
    Angle turn;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BLOCK_WAIT:
        if (njDistanceP2P(player, pos) < BLOCK_REACH) {
            sound_play(SE_DROP, 0, 0, 0);
            work->mode++;
        }
        break;
    case BLOCK_DROP:
        move.y = BLOCK_SPEED(work);
        BLOCK_MOVED(work) += move.y;
        if (BLOCK_MOVED(work) < BLOCK_DEPTH(work)) {
            toward = (Angle)(atan2f(camera_work->pos.x - pos->x,
                                    camera_work->pos.z - pos->z) *
                             ANGLE_PER_RADIAN);
            side.x = njSin(toward) * DUST_AWAY;
            side.y = -DUST_BELOW;
            side.z = njCos(toward) * DUST_AWAY;
            njPushMatrix(&_unit_matrix_);
            TURN_VIA(njRotateZ, ang->z, turn);
            TURN_VIA(njRotateX, ang->x, turn);
            TURN_VIA(njRotateY, ang->y, turn);
            njCalcVector(NULL, &side, &side);
            njPopMatrix(1);
            dust.x = pos->x + side.x;
            dust.y = pos->y + side.y;
            dust.z = pos->z + side.z;
            side.x = side.y = side.z = 0.0F;
            dust_create(&dust, &side, DUST_SCALE);
            work->tick = LAND_STEPS;
            work->mode++;
        }
        break;
    case BLOCK_LAND:
        if (!(work->flags & BLOCK_STAYS)) {
            if (--work->tick == 0) {
                work->mode++;
            }
        } else {
            work->mode = BLOCK_DOWN;
        }
        break;
    case BLOCK_RISE:
        move.y = -(BLOCK_SPEED(work) * RISE_SHARE);
        BLOCK_MOVED(work) += move.y;
        if (BLOCK_MOVED(work) > 0.0F) {
            move.y = 0.0F;
            pos->x = VARS(work)->home.x;
            pos->y = VARS(work)->home.y;
            pos->z = VARS(work)->home.z;
            work->tick = REST_STEPS;
            work->mode++;
        }
        break;
    case BLOCK_REST:
        if (--work->tick == 0) {
            work->mode = BLOCK_WAIT;
        }
        break;
    case BLOCK_DOWN:
        break;
    }
    if (work->mode != BLOCK_DOWN) {
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateZ, ang->z, turn);
        TURN_VIA(njRotateX, ang->x, turn);
        TURN_VIA(njRotateY, ang->y, turn);
        njCalcVector(NULL, &move, &move);
        njPopMatrix(1);
        pos->x += move.x;
        pos->y += move.y;
        pos->z += move.z;
    }
    spike_block_draw(task);
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
}

static void spike_block_start(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, spike_block_shapes, COLLISION_OBJECT, 1, 2);
    BLOCK_MOVED(work) = 0.0F;
    VARS(work)->home.x = work->pos.x;
    VARS(work)->home.y = work->pos.y;
    VARS(work)->home.z = work->pos.z;
    task->step = spike_block_run;
    task->draw = spike_block_draw;
    task->destructor = spike_block_destroy;
}

void spike_block_up_down_step(struct task *task)
{
    spike_block_start(task);
    task->work->flags &= ~BLOCK_STAYS;
}

void spike_block_down_step(struct task *task)
{
    spike_block_start(task);
    task->work->flags |= BLOCK_STAYS;
}
