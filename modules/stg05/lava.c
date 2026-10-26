/* o_yougan, a lava sheet that rises as MagmaCnt sets it, and O MYOUGAN and
 * O YE102, lava pools. all three draw one texture animation */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg05/lava.h"
#include "stg05/mountain.h"
#include "system/task.h"
#include "system/util.h"

/* YOUGAN_ANIM's twelve textures, the frames of the lava's animation */
extern NJS_TEXLIST lava_anim_texlist;
extern NJS_OBJECT lava_pool_object;
extern NJS_OBJECT lava_object;
extern NJS_OBJECT lava_e102_object;

enum lava_mode { LAVA_INIT = 0, LAVA_RISE = 1, LAVA_BOB = 3 };

/* the pools' modes. no step sets POOL_IDLE, in which a pool does nothing */
enum lava_pool_mode { POOL_INIT = 0, POOL_RUN = 1, POOL_IDLE = 4 };
/* the animation's modes. no step sets ANIM_IDLE, in which it does nothing */
enum lava_anim_mode { ANIM_INIT = 0, ANIM_RUN = 1, ANIM_IDLE = 4 };

/* the sheet's sound, which each step of a rise refreshes */
#define SE_LAVA 130

/* the sheet's x and z, which replace its set entry's */
#define LAVA_X (-1468.0F)
#define LAVA_Z 1559.0F
/* the sheet's first level: LAVA_LEVEL_E102 when the character is E-102, and
 * LAVA_LEVEL otherwise */
#define LAVA_LEVEL 130.0F
#define LAVA_LEVEL_E102 28.0F

/* the bob's height above and below the sheet's level */
#define LAVA_BOB_HEIGHT 0.7F

/* the sheet is not drawn while the character is Sonic and player 0 is above
 * this height */
#define LAVA_SONIC_ABOVE 880.0F

/* O YE102 destroys itself once neither the camera nor player 0 is within
 * 5000 of it. the range is squared, as placement_try_leave_by takes it */
#define LAVA_E102_LEAVE_RANGE (5000.0F * 5000.0F)

#define LAVA_ANIM_LAST 11

/* the phase of the sheet's bob, in degrees */
#define BOB_PHASE(work) TASK_TIMER(work)
/* the stage and act the animation's task started in */
#define ANIM_ACT(work) TASK_TIMER(work)

/* the YOUGAN_ANIM frame every kind of lava draws */
static Uint8 lava_anim_frame = 0;

/* @hack type: an array of one, not the single shape it is. as one shape,
 * lava_step keeps a 1 in R11 through the step and saves it, runs 10 bytes
 * longer, and 39 of 193 words differ */
static struct collision_shape lava_shape[1] = {
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     ATTACK(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -10.0F, 0.0F},
     2084.0F,
     10.0F,
     1830.0F,
     {0},
     0,
     NULL},
};
/* @hack type: an array of one, not the single shape it is. as one shape,
 * lava_pool_step keeps a 1 in R12 through the step and saves it, runs 4
 * bytes longer, and 87 of 107 words differ */
static struct collision_shape lava_pool_shape[1] = {
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     ATTACK(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -5.0F, 0.0F},
     150.0F,
     5.0F,
     150.0F,
     {0},
     0,
     NULL},
};
static struct collision_shape lava_e102_shapes[3] = {
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     ATTACK(2) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-630.0F, -10.0F, 0.0F},
     630.0F,
     10.0F,
     720.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     ATTACK(2) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {635.0F, -10.0F, -180.0F},
     540.0F,
     10.0F,
     500.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     ATTACK(2) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {800.0F, -10.0F, 540.0F},
     417.0F,
     10.0F,
     180.0F,
     {0},
     0,
     NULL},
};

static void lava_draw(struct task *task);
static void lava_destroy(struct task *task);
static void lava_pool_draw(struct task *task);
static void lava_pool_destroy(struct task *task);
static void lava_e102_draw(struct task *task);
static void lava_e102_destroy(struct task *task);

/* the sheet rises by the stage task's scl.y each step until it reaches
 * scl.x. it then bobs at that level until scl.y is nonzero again. each step
 * it sets the stage task's pos.y to its height, and lava_moved to how far it
 * moved. the step a rise ends on sets neither */
void lava_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MODEL *model;
    Float bob;

    switch (work->mode) {
    case LAVA_INIT:
        collision_init(task, lava_shape, COLLISION_OBJECT, 1, 1);
        work->mode = LAVA_RISE;
        task->draw = lava_draw;
        task->destructor = lava_destroy;
        work->pos.x = LAVA_X;
        work->pos.z = LAVA_Z;
        work->collision->radius = 10000.0F;
        if (get_character() == CHARACTER_E102) {
            work->pos.y = LAVA_LEVEL_E102;
            work->scl.z = LAVA_LEVEL_E102;
            mountain_work->pos.y = LAVA_LEVEL_E102;
            mountain_work->scl.x = LAVA_LEVEL_E102;
        } else {
            work->pos.y = LAVA_LEVEL;
            work->scl.z = LAVA_LEVEL;
            mountain_work->pos.y = LAVA_LEVEL;
            mountain_work->scl.x = LAVA_LEVEL;
        }
        break;
    case LAVA_RISE:
        work->pos.x = LAVA_X;
        work->pos.z = LAVA_Z;
        if (mountain_work) {
            work->pos.y += mountain_work->scl.y;
            if (work->pos.y >= mountain_work->scl.x) {
                work->pos.y = mountain_work->scl.x;
                work->scl.z = work->pos.y;
                mountain_work->scl.y = 0.0F;
                work->mode = LAVA_BOB;
                break;
            }
            mountain_work->pos.y = work->pos.y;
            MOUNTAIN_VARS(mountain_work)->lava_moved = mountain_work->scl.y;
        }
        sound_play_for(SE_LAVA, (int)work, 1, 0, 30);
        collision_register(work);
        break;
    case LAVA_BOB:
        BOB_PHASE(work)++;
        bob = njSin((Angle)(BOB_PHASE(work) * ANGLE_PER_DEGREE)) *
              LAVA_BOB_HEIGHT;
        MOUNTAIN_VARS(mountain_work)->lava_moved =
            work->scl.z + bob - work->pos.y;
        work->pos.y = work->scl.z + bob;
        mountain_work->pos.y = work->pos.y;
        if (mountain_work->scl.y != 0.0F) {
            work->mode = LAVA_RISE;
        }
        collision_register(work);
        break;
    }
    /* @hack spelling: the model read through the object's address, whose pool
     * word lava_draw shares (0x0c90595a, 0x0c905978). read from the object
     * itself, the member's address takes a pool word of its own, and 7 of 193
     * words differ */
    model = (&lava_object)->model;
    model->mats->attr_texId = lava_anim_frame;
    lava_draw(task);
}

static void lava_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &lava_object;

    /* @hack spelling: three nested tests; as one condition, player_work's
     * address is kept in the frame, which is 12 bytes, not 8 (ADD #-8,R15 at
     * 0x0c905974), and 41 of 47 words differ */
    if (player_work[0]) {
        if (get_character() == CHARACTER_SONIC) {
            if (player_work[0]->pos.y > LAVA_SONIC_ABOVE) {
                return;
            }
        }
    }
    njSetTexture(&lava_anim_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_tree(object);
    njPopMatrix(1);
}

static void lava_destroy(struct task *task)
{
}

/* a pool draws scaled by scale x + 1 along x and scale z + 1 along z. a
 * positive scale x multiplies its box's half size along x by scale x + 1. a
 * positive scale z does the same along z. init multiplies the radius of the
 * model, which every pool shares, by scale x + 1. so each pool that starts
 * multiplies that radius again */
void lava_pool_step(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape;
    NJS_MODEL *model;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case POOL_INIT:
        work->mode = POOL_RUN;
        collision_init(task, lava_pool_shape, COLLISION_OBJECT, 1, 1);
        task->draw = lava_pool_draw;
        task->destructor = lava_pool_destroy;
        /* @hack spelling: the products spelled out; with *=, each field's
         * address is computed apart (0x0c905a30 on) and 37 of 107 words
         * differ */
        shape = work->collision->shapes;
        if (work->scl.x > 0.0F) {
            shape->radius = shape->radius * (work->scl.x + 1.0F);
        }
        if (work->scl.z > 0.0F) {
            shape->size_z = shape->size_z * (work->scl.z + 1.0F);
        }
        work->collision->radius = 1500.0F;
        /* @hack spelling: the model read through the object's address, which
         * the image loads at 0x0c905a5c, reading the model 4 bytes past it.
         * read from the object itself, 17 of 107 words differ */
        (&lava_pool_object)->model->r =
            (&lava_pool_object)->model->r * (work->scl.x + 1.0F);
        work->mode = POOL_RUN;
        break;
    case POOL_RUN:
        work->ang.x = 0;
        work->ang.y = 0;
        work->ang.z = 0;
        collision_register(work);
        /* @hack spelling: the model read through the object's address, whose
         * pool word lava_pool_draw shares (0x0c905aee, 0x0c905b16). read from
         * the object itself, 6 of 107 words differ, and 8 of lava_pool_draw's
         * 41 */
        model = (&lava_pool_object)->model;
        model->mats->attr_texId = lava_anim_frame;
        lava_pool_draw(task);
        break;
    case POOL_IDLE:
        break;
    }
}

static void lava_pool_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &lava_pool_object;

    njSetTexture(&lava_anim_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScale(NULL, work->scl.x + 1.0F, 1.0F, work->scl.z + 1.0F);
    draw_model(object->model);
    njPopMatrix(1);
}

static void lava_pool_destroy(struct task *task)
{
}

void lava_e102_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MODEL *model;

    if (placement_try_leave_by(task, LAVA_E102_LEAVE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case POOL_INIT:
        work->mode = POOL_RUN;
        collision_init(task, lava_e102_shapes, COLLISION_OBJECT, 1, 3);
        task->draw = lava_e102_draw;
        task->destructor = lava_e102_destroy;
        work->mode = POOL_RUN;
        work->collision->radius = 5000.0F;
        break;
    case POOL_RUN:
        work->ang.y = 0;
        work->ang.x = 0;
        work->ang.z = 0;
        /* @hack spelling: the model read through the object's address, which
         * the image loads into R4 (0x0c905bc4). read from the object itself,
         * SHC loads the model member's address instead, and 11 of 70 words
         * differ */
        model = (&lava_e102_object)->model;
        model->mats->attr_texId = lava_anim_frame;
        lava_e102_draw(task);
        collision_register(work);
        object_clear_touch(work, NULL);
        break;
    case POOL_IDLE:
        break;
    }
}

static void lava_e102_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &lava_e102_object;

    njSetTexture(&lava_anim_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_model(object->model);
    njPopMatrix(1);
}

static void lava_e102_destroy(struct task *task)
{
}

/* advance lava_anim_frame every third step. the task destroys itself once
 * the stage and act are no longer those it started in */
void lava_anim_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case ANIM_INIT:
        ANIM_ACT(work) = stage_get_stage_act();
        work->mode = ANIM_RUN;
        break;
    case ANIM_RUN:
        if (ANIM_ACT(work) != stage_get_stage_act()) {
            task_kill(task);
            return;
        }
        if (work->tick++ > 1) {
            lava_anim_frame++;
            if (lava_anim_frame > LAVA_ANIM_LAST) {
                lava_anim_frame = 0;
            }
            work->tick = 0;
        }
        break;
    case ANIM_IDLE:
        break;
    }
}
