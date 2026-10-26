/* Bumper1: a bumper on the pinball tables. Bumper2: a pair of bumper posts,
 * after whose bounce the stick steers player 0 */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "draw/model.h"
#include "object/common.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/bumper.h"
#include "stg09/casino.h"
#include "stg09/pinball.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_MODEL bumper_model;
extern NJS_MODEL bumper_lit_model;

/* in work->flags. Bumper1 sets BUMPER_TOUCHING from a hit until a step
 * without one, so that a touch hits once. the two posts share Bumper2's.
 * BUMPER2_BOUNCED is set from a post's hit until player 0 falls below
 * Bumper2 */
#define BUMPER_TOUCHING 0x8000
#define BUMPER2_BOUNCED 0x200

/* the draws a hit lights the bumper for, counted down in work->mode */
#define BUMPER_LIT_DRAWS 2
/* the hits to a bonus, counted in work->tick. work->timer counts the
 * bonuses */
#define BUMPER_HITS 20

#define BUMPER2_POST1_Z (-80.0F)
#define BUMPER2_POST2_Z (-36.5F)

#define BUMPER2_STEER 0.05F /* the stick's push along z */
#define BUMPER2_TILT (-NJM_DEG_ANG(15)) /* gravity's tilt about x */

#define SE_BUMPER 245

static struct collision_shape bumper_shape = {
    SHAPE_SPHERE,
    ATTACK(3),
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    15.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape bumper2_post_shape = {
    SHAPE_CYLINDER,
    ATTACK(3),
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    15.0F,
    3.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void bumper_destroy(struct task *task)
{
}

static void bumper_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    if (work->mode != 0) {
        draw_model(&bumper_lit_model);
        work->mode--;
    } else {
        draw_model(&bumper_model);
    }
    njPopMatrix(1);
}

static void bumper_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos = &player_work[0]->pos;
    NJS_VECTOR speed;
    Angle ang[3];

    if (work->collision->flags & COLLISION_HIT) {
        if (!(work->flags & BUMPER_TOUCHING)) {
            ang[0] = 0;
            ang[1] =
                ((Angle)(atan2f(work->pos.z - pos->z, pos->x - work->pos.x) *
                         ANGLE_PER_RADIAN) +
                 NJM_DEG_ANG(90)) &
                0xffff;
            ang[2] = 0;
            speed.x = pinball_cap_speed(work->scl.z + pinball_player_speed() +
                                        (rand() * work->scl.y * RAND_SCALE));
            speed.y = 0.0F;
            speed.z = 0.0F;
            player_send_flying(0, &speed, ang, 0);
            work->mode = BUMPER_LIT_DRAWS;
            sound_play(SE_BUMPER, 0, 0, 0);
            vibration_start(0, 4, 15, 0);
            if (++work->tick == BUMPER_HITS) {
                work->tick = 0;
                /* @bug arithmetic: pinball_give_rings keeps the count's low
                 * byte (0x0c9011ce). from the 26th bonus, 10n + 1 passes
                 * 255, so the 26th pays 5 rings (0x0c902c76 to 0x0c902c88) */
                pinball_give_rings((++work->timer * 10) + 1);
            } else {
                pinball_give_rings(1);
            }
            work->flags |= BUMPER_TOUCHING;
        }
    } else {
        work->flags &= ~BUMPER_TOUCHING;
    }
    bumper_draw(task);
    collision_register(work);
    object_clear_touch(work, NULL);
}

/* a hit sets Bumper2's work->mode to BUMPER_LIT_DRAWS, which no draw reads.
 * Bumper2 never draws, and each post draws by its own work->mode, which
 * stays 0 */
static void bumper2_post_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    NJS_VECTOR *speed;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint32[3]. the image's ADD #-24,R15 reserves them
     * (0x0c902d26); without them, 2 of 77 words differ */
    Uint32 unused[3];
    Angle ang[3];

    if (work->collision->flags & COLLISION_HIT) {
        /* @bug logic: the posts share Bumper2's BUMPER_TOUCHING, and the
         * step of a post without a hit clears it (0x0c902de0). a post that
         * stays in contact while the other doesn't passes this test
         * (0x0c902d3e) and hits again each step */
        if (!(parent->flags & BUMPER_TOUCHING)) {
            speed = &player_motion[0]->speed;
            player_set_accel(0, -speed->x, -speed->y + 2.0F, 0.0F);
            parent->flags |= BUMPER2_BOUNCED;
            parent->mode = BUMPER_LIT_DRAWS;
            sound_play(SE_BUMPER, 0, 0, 0);
            vibration_start(0, 4, 15, 0);
            parent->flags |= BUMPER_TOUCHING;
        }
    } else {
        parent->flags &= ~BUMPER_TOUCHING;
    }
    bumper_draw(task);
    collision_register(work);
    object_clear_touch(work, NULL);
}

static void bumper2_run(struct task *task)
{
    struct task_work *work = task->work;
    Float stick;

    task_step_children(task);
    if (work->flags & BUMPER2_BOUNCED) {
        if (work->pos.y > player_work[0]->pos.y) {
            world_tilt_gravity(BUMPER2_TILT, 0);
            work->flags &= ~BUMPER2_BOUNCED;
        } else if (!(player_work[0]->flags & PLAYER_ON_LAND)) {
            stick = per_info[0]->x1 << 8;
            if (stick < -PAD_DEAD_ZONE) {
                player_set_accel(0, 0.0F, 0.0F, -BUMPER2_STEER);
            } else if (stick > PAD_DEAD_ZONE) {
                player_set_accel(0, 0.0F, 0.0F, BUMPER2_STEER);
            }
        }
    }
}

void bumper_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &bumper_shape, COLLISION_OBJECT, 1, 1);
    work->timer = 0;
    work->tick = 0;
    task->step = bumper_run;
    task->draw = bumper_draw;
    task->destructor = bumper_destroy;
}

/* the posts stand at Bumper2's x and y, but at a fixed z, whatever its
 * placement's */
void bumper2_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *post;
    struct task_work *post_work;
    struct collision_shape *shape;

    post = task_create_child(TASK_WORK, bumper2_post_run, task);
    post_work = post->work;
    post_work->pos.x = work->pos.x;
    post_work->pos.y = work->pos.y;
    post_work->pos.z = BUMPER2_POST1_Z;
    /* @hack spelling: the posts' shape through shape, set inside this call,
     * which keeps its address in the frame for the second (0x0c902f7c,
     * 0x0c902fac). as &bumper2_post_shape at each call, 64 of 68 words
     * differ */
    collision_init(post, shape = &bumper2_post_shape, COLLISION_OBJECT, 1, 1);

    post = task_create_child(TASK_WORK, bumper2_post_run, task);
    post_work = post->work;
    post_work->pos.x = work->pos.x;
    post_work->pos.y = work->pos.y;
    post_work->pos.z = BUMPER2_POST2_Z;
    collision_init(post, shape, COLLISION_OBJECT, 1, 1);

    work->flags &= ~BUMPER2_BOUNCED;
    task->step = bumper2_run;
    task->destructor = bumper_destroy;
}
