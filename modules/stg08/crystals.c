/* O SuiTaore: a crystal that topples once player 0 comes near. O SuisyouB,
 * O SuisyouBM and O SuisyouR: standing crystals that sparkle */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/sparkle.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg08/crystals.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION crystal_topple_action;
extern NJS_OBJECT crystal_b_object;
extern NJS_OBJECT crystal_bm_object;
extern NJS_OBJECT crystal_r_object;

/* the toppling crystal starts once player 0 is within TOPPLE_NEAR. its
 * action then runs TOPPLE_RATE frames a step, and stops at TOPPLE_END with
 * SE_TOPPLE */
#define TOPPLE_NEAR 850.0F
#define TOPPLE_RATE 2.0F
#define TOPPLE_END 29.0F
#define SE_TOPPLE 224

/* in work->flags: player 0 has come near, and the crystal has toppled */
#define CRYSTAL_NEAR 0x100
#define CRYSTAL_TOPPLED 0x200

/* a standing crystal sparkles with a chance of SPARKLE_CHANCE each step.
 * the sparkle starts SPARKLE_RISE above the crystal's position, moved a
 * random 0 to SPARKLE_SPREAD along each axis */
#define SPARKLE_CHANCE 0.01F
#define SPARKLE_SPREAD 4.0F
#define SPARKLE_RISE 7.0F
#define SPARKLE_SIZE 0.03F
#define SPARKLE_SPIN 64

struct crystal_vars {
    Uint8 unk_00[8];
    Float frame; /* the toppling crystal's action frame */
};

#define VARS(work) ((struct crystal_vars *)&(work)->vars)

/* in work->mode */
enum crystal_mode {
    CRYSTAL_TOPPLE,
    CRYSTAL_B,
    CRYSTAL_BM,
    CRYSTAL_UNPLACED, /* no step sets it, so crystal_shapes[2] goes unused */
    CRYSTAL_R
};

/* the toppling crystal's two capsules, where it stands and where it falls */
static struct collision_shape crystal_topple_shapes[2] = {
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 44.0F, 0.0F},
     60.0F,
     45.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {180.0F, 44.0F, 0.0F},
     50.0F,
     45.0F,
     0.0F,
     {0},
     0,
     NULL},
};
#define CRYSTAL_SHAPE(y, radius)                                               \
    {SHAPE_CAPSULE,                                                            \
     0,                                                                        \
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,                                           \
     {0.0F, (y), 0.0F},                                                        \
     (radius),                                                                 \
     (radius),                                                                 \
     0.0F,                                                                     \
     {0},                                                                      \
     0,                                                                        \
     NULL}
/* a capsule for each standing crystal's mode, from CRYSTAL_B on, as SHAPE_OF
 * finds it */
static struct collision_shape crystal_shapes[4] = {
    CRYSTAL_SHAPE(8.0F, 9.0F),
    CRYSTAL_SHAPE(6.0F, 7.0F),
    CRYSTAL_SHAPE(6.0F, 7.0F),
    CRYSTAL_SHAPE(4.0F, 5.0F),
};

#define SHAPE_OF(mode) (&crystal_shapes[(mode) - CRYSTAL_B])

/* the crystal's draw, with specular highlights allowed */
static void crystal_draw(struct task *task)
{
    struct task_work *work = task->work;

    draw_attr_save();
    draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    switch (work->mode) {
    case CRYSTAL_TOPPLE:
        draw_action(&crystal_topple_action, VARS(work)->frame);
        break;
    case CRYSTAL_B:
        draw_tree(&crystal_b_object);
        break;
    case CRYSTAL_BM:
        draw_tree(&crystal_bm_object);
        break;
    case CRYSTAL_R:
        draw_tree(&crystal_r_object);
        break;
    }
    njPopMatrix(1);
    draw_attr_restore();
}

static void crystal_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_VECTOR speed;
    NJS_POINT3 pos;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    switch (work->mode) {
    case CRYSTAL_TOPPLE:
        if (!(work->flags & CRYSTAL_NEAR)) {
            if (njDistanceP2P(&work->pos, &player->pos) < TOPPLE_NEAR) {
                /* @hack spelling: the image adds each bit rather than ORing
                 * it in, CRYSTAL_NEAR here and CRYSTAL_TOPPLED below. each is
                 * clear where it is added, so adding sets it; with |=, 2 of
                 * 154 words differ */
                work->flags += CRYSTAL_NEAR;
            }
        } else if (!(work->flags & CRYSTAL_TOPPLED)) {
            VARS(work)->frame += TOPPLE_RATE;
            if (VARS(work)->frame > TOPPLE_END) {
                VARS(work)->frame = TOPPLE_END;
                work->flags += CRYSTAL_TOPPLED;
                sound_play(SE_TOPPLE, 0, 0, 0);
            }
        }
        break;
    default:
        if (rand() * RAND_SCALE < SPARKLE_CHANCE) {
            speed.x = 0.0F;
            speed.y = 0.0F;
            speed.z = 0.0F;
            pos.x = work->pos.x + (rand() * (SPARKLE_SPREAD * RAND_SCALE));
            pos.y = work->pos.y + (rand() * (SPARKLE_SPREAD * RAND_SCALE)) +
                    SPARKLE_RISE;
            pos.z = work->pos.z + (rand() * (SPARKLE_SPREAD * RAND_SCALE));
            sparkle_create(&pos, &speed, SPARKLE_SIZE, SPARKLE_SPIN);
        }
        break;
    }
    crystal_draw(task);
}

void crystal_topple_step(struct task *task)
{
    task->work->flags = ~(CRYSTAL_NEAR | CRYSTAL_TOPPLED);
    collision_init(task, crystal_topple_shapes, COLLISION_OBJECT, 1, 2);
    task->step = crystal_run;
    task->draw = crystal_draw;
    task->work->mode = CRYSTAL_TOPPLE;
}

void crystal_b_step(struct task *task)
{
    collision_init(task, SHAPE_OF(CRYSTAL_B), COLLISION_OBJECT, 1, 1);
    task->step = crystal_run;
    task->draw = crystal_draw;
    task->work->mode = CRYSTAL_B;
}

void crystal_bm_step(struct task *task)
{
    collision_init(task, SHAPE_OF(CRYSTAL_BM), COLLISION_OBJECT, 1, 1);
    task->step = crystal_run;
    task->draw = crystal_draw;
    task->work->mode = CRYSTAL_BM;
}

void crystal_r_step(struct task *task)
{
    collision_init(task, SHAPE_OF(CRYSTAL_R), COLLISION_OBJECT, 1, 1);
    task->step = crystal_run;
    task->draw = crystal_draw;
    task->work->mode = CRYSTAL_R;
}
