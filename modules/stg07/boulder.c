/* the burning boulder that rolls down ruin_boulder_path after player 0. once
 * player 0 has got ahead, the boulder speeds up while it stays ahead */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "object/common.h"
#include "path/path.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg07/boulder.h"
#include "stg07/paths.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL boulder_model;

/* a path task's vars, laid out as path/path.h's struct path_vars. the spin
 * takes the bytes of its two wait counts */
struct boulder_vars {
    Angle spin; /* taken off ang.z each step */
    Sint16 stage_act;
    struct path *path;
};

#define VARS(work) ((struct boulder_vars *)&(work)->vars)

/* the boulder's speed along the path, a step, and its distance along it */
#define BOULDER_SPEED(work) ((work)->scl.x)
#define BOULDER_ALONG(work) ((work)->scl.y)
/* its flames' scale, which each new flame grows by 1 */
#define BOULDER_FLAME(work) ((work)->scl.z)
/* the flames it has taken on */
#define BOULDER_FLAMES(work) ((work)->tick)

/* the boulder's flags. BOULDER_CHASING is set once player 0 has got more
 * than BOULDER_AHEAD ahead. BOULDER_DUSTED is set once it has thrown up its
 * dust */
#define BOULDER_CHASING 0x100
#define BOULDER_DUSTED 0x200

/* a flame child's vars */
struct flame_vars {
    Uint8 unk_00[4];
    Float across; /* its offset in z from the boulder */
};

#define FLAME(work) ((struct flame_vars *)&(work)->vars)

/* a dust child's offset in z from the boulder. its scl.y and scl.z are its
 * puffs' speed in y and z */
#define DUST_ACROSS(work) ((work)->scl.x)

/* the boulder stops past BOULDER_END along the path. it speeds up while
 * player 0 is more than BOULDER_AHEAD ahead of it in x. past BOULDER_DUST
 * along the path, it throws up its dust */
#define BOULDER_END 5755.0F
#define BOULDER_AHEAD 200.0F
#define BOULDER_DUST 150.0F

/* a flame child lights its flames this far from the boulder's middle */
#define BOULDER_RIM 52.0F

#define SE_RUMBLE 199

/* the distances along the path past which the boulder takes on each new
 * flame. the last lies past BOULDER_END */
static int boulder_flame_distances[8] = {200, 300, 400, 500,
                                         600, 700, 800, 10000};

/* clang-format off */
static struct collision_shape boulder_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(3) | SHAPE_HURTS_PLAYERS, PUSH_ALL,
    {0.0F, 0.0F, 0.0F}, 43.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL,
};
/* clang-format on */

static void boulder_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* every 8 steps, the child lights a flame on the boulder's rim. the flame
 * lies at the boulder's turn plus the child's, and moves along x at the
 * boulder's speed */
static void boulder_flame_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *boulder = task->parent->work;
    NJS_POINT3 pos;

    if (!(++work->tick & 7)) {
        pos.x = boulder->pos.x +
                (njCos(boulder->ang.z + work->ang.z) * BOULDER_RIM);
        pos.y = boulder->pos.y +
                (njSin(boulder->ang.z + work->ang.z) * BOULDER_RIM);
        pos.z = boulder->pos.z + FLAME(work)->across;
        work->scl.x = BOULDER_SPEED(boulder);
        flame_create(&pos, &work->scl, BOULDER_FLAME(boulder));
    }
}

static void boulder_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    texture_set_stage();
    draw_model(&boulder_model);
    njPopMatrix(1);
}

/* every 9 steps, the child puffs dust forward from the boulder's front, at
 * its foot */
static void boulder_dust_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *boulder = task->parent->work;
    NJS_POINT3 pos;
    NJS_VECTOR speed;

    if (work->tick == 8) {
        work->tick = 0;
        pos.x = boulder->pos.x + 45.0F;
        pos.y = boulder->pos.y - 40.0F;
        pos.z = boulder->pos.z + DUST_ACROSS(work);
        speed.x = 3.0F;
        speed.y = work->scl.y;
        speed.z = work->scl.z;
        dust_create(&pos, &speed, 2.0F);
    } else {
        work->tick++;
    }
}

/* set up a dust child across in z from the boulder. its puffs rise at ang
 * from up toward z */
static void boulder_dust_init(struct task_work *work, Float across, Angle ang)
{
    DUST_ACROSS(work) = across;
    work->scl.y = njCos(ang) * 1.5F;
    work->scl.z = njSin(ang) * 1.5F;
}

/* puff dust across in z from the boulder, at its front and foot. the puff
 * moves forward, and rises at ang from up toward z */
static void boulder_dust_burst(struct task_work *work, Float across, Angle ang)
{
    NJS_POINT3 pos;
    NJS_VECTOR speed;

    pos.x = work->pos.x + 30.0F;
    pos.y = work->pos.y - 40.0F;
    pos.z = work->pos.z + across;
    speed.x = 5.0F;
    speed.y = njCos(ang) * 4.0F;
    speed.z = njSin(ang) * 4.0F;
    dust_create(&pos, &speed, 4.0F);
}

/* roll along the path. a touch sends player 0 flying. the boulder stops past
 * BOULDER_END, or once the stage or act changes from the path task's. then
 * each step only destroys the flames and the dust */
static void boulder_roll(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct task *child;
    struct path_sample sample;
    NJS_VECTOR speed;
    Angle ang[3];

    /* @hack type: stage_get_stage_act called through a cast as returning
     * int, so the compare takes R0 as it is (MOV R0,R3 at 0x0c9022aa). as
     * the Uint16 stage.h declares, SHC extends it with EXTU.W, and 1 of 368
     * words differ */
    /* @hack redundant: an else after the return, which loads the collision
     * record into R3 for the hit's test (MOV.L @(56,R14),R3 at 0x0c902300);
     * without it, R2 takes it and 2 of 368 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-cast-function-type-mismatch) */
    if (((int (*)(void))stage_get_stage_act)() != VARS(work)->stage_act) {
        task_destroy_children(task);
        return;
    } else {
        /* @hack spelling: the hit's two tests nested, which give player R11
         * (the loads at 0x0c9022a2 and 0x0c9022a8); as one test with &&, R9
         * takes it and 112 of 368 words differ */
        if (work->collision->flags & COLLISION_HIT) {
            if (work->collision->hit == player_work[0]->collision) {
                ang[0] = 0;
                ang[1] = 0x4000;
                ang[2] = 0;
                speed.x = 5.0F;
                speed.y = 1.0F;
                speed.z = 0.0F;
                player_send_flying(0, &speed, ang, 60);
            }
        }
        BOULDER_ALONG(work) += BOULDER_SPEED(work);
        if (BOULDER_ALONG(work) >= BOULDER_END) {
            task_destroy_children(task);
            return;
        }
        work->ang.z -= VARS(work)->spin;
        sample.distance = BOULDER_ALONG(work);
        path_sample(&ruin_boulder_path, &sample);
        work->pos.x = sample.pos.x;
        work->pos.y = sample.pos.y;
        work->pos.z = sample.pos.z;
        boulder_draw(task);
        if (!(work->flags & BOULDER_CHASING)) {
            if (player->pos.x > work->pos.x + BOULDER_AHEAD) {
                /* @hack spelling: the clear bit added, ADD at 0x0c9023be;
                 * or'd, it takes an OR, and 1 of 368 words differ */
                work->flags += BOULDER_CHASING;
            }
        } else {
            sound_play_at_for(SE_RUMBLE, (int)work, 1, 96, work->pos.x,
                              work->pos.y, work->pos.z, 2);
            if (player->pos.x > work->pos.x + BOULDER_AHEAD) {
                BOULDER_SPEED(work) += 0.15F;
                VARS(work)->spin += 24;
            } else if (BOULDER_SPEED(work) > 1.5F) {
                BOULDER_SPEED(work) -= 0.3F;
                VARS(work)->spin -= 48;
            }
        }
        task_step_children(task);
        if (!(work->flags & BOULDER_DUSTED) &&
            BOULDER_ALONG(work) >= BOULDER_DUST) {
            /* @hack spelling: the clear bit added, ADD at 0x0c9024e2; or'd,
             * it takes an OR, and 1 of 368 words differ */
            work->flags += BOULDER_DUSTED;
            boulder_dust_burst(work, -35.0F, NJM_DEG_ANG(-50));
            boulder_dust_burst(work, -30.0F, NJM_DEG_ANG(-40));
            boulder_dust_burst(work, -25.0F, NJM_DEG_ANG(-30));
            boulder_dust_burst(work, -20.0F, NJM_DEG_ANG(-20));
            boulder_dust_burst(work, -15.0F, NJM_DEG_ANG(-10));
            boulder_dust_burst(work, 0.0F, NJM_DEG_ANG(0));
            boulder_dust_burst(work, 15.0F, NJM_DEG_ANG(10));
            boulder_dust_burst(work, 20.0F, NJM_DEG_ANG(20));
            boulder_dust_burst(work, 25.0F, NJM_DEG_ANG(30));
            boulder_dust_burst(work, 30.0F, NJM_DEG_ANG(40));
            boulder_dust_burst(work, 35.0F, NJM_DEG_ANG(50));
            child = task_create_child(TASK_WORK, boulder_dust_step, task);
            boulder_dust_init(child->work, -25.0F, NJM_DEG_ANG(-50));
            child->work->tick = 0;
            child = task_create_child(TASK_WORK, boulder_dust_step, task);
            boulder_dust_init(child->work, 5.0F, NJM_DEG_ANG(-85));
            child->work->tick = 4;
            child = task_create_child(TASK_WORK, boulder_dust_step, task);
            boulder_dust_init(child->work, -5.0F, NJM_DEG_ANG(85));
            child->work->tick = 2;
            child = task_create_child(TASK_WORK, boulder_dust_step, task);
            boulder_dust_init(child->work, 25.0F, NJM_DEG_ANG(50));
            child->work->tick = 6;
        }
        if (BOULDER_ALONG(work) >
            boulder_flame_distances[BOULDER_FLAMES(work)]) {
            BOULDER_FLAMES(work)++;
            BOULDER_FLAME(work) += 1.0F;
            child = task_create_child(TASK_WORK, boulder_flame_step, task);
            child->work->ang.z = work->ang.z;
            child->work->tick = 0;
            child->work->scl.y = 0.0F;
            child->work->scl.z = 0.0F;
            FLAME(child->work)->across =
                (rand() * (50.0F * RAND_SCALE)) - 25.0F;
        }
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
    }
}

void ruin_path_boulder_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &boulder_shape, COLLISION_ENEMY, 1, 1);
    /* @hack redundant: the flame count's 0 written as a select on 1.0F, which
     * SHC folds too late to share. the count takes MOV #0,R0, and the spin
     * its own MOV #0,R3 (0x0c9026f8, 0x0c902704). as a plain 0, both take R4
     * and 11 of 39 words differ */
    BOULDER_FLAMES(work) = 1.0F ? 0 : 0; /* NOLINT(misc-redundant-expression) */
    BOULDER_SPEED(work) = 0.0F;
    VARS(work)->spin = 0;
    BOULDER_ALONG(work) = 0.0F;
    BOULDER_FLAME(work) = 0.5F;
    work->flags = ~(BOULDER_CHASING | BOULDER_DUSTED);
    task->step = boulder_roll;
    task->draw = boulder_draw;
    task->destructor = boulder_destroy;
}
