/* the wall the object list names O BLOCK ENTRY: blocks that fall once the
 * gold and silver statues are in their locks */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv02/block_entry.h"
#include "adv02/handy_key.h"
#include "adv02/lock.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/util.h"

/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address and reaches pos and model through it (ADD #8,R5 at 0x0c90b730,
 * MOV.L @(4,R4),R4 at 0x0c90b75a); declared as one, 7 words of the pools and
 * pad differ, and 74 of block_entry_draw's 140 words */
extern NJS_OBJECT block_entry_glow_object[];
/* the first blocks of the lower run and of the top one. each run's other
 * blocks are its first's siblings */
extern NJS_OBJECT block_entry_blocks_object;
extern NJS_OBJECT block_entry_top_blocks_object;

/* the square of the distance from the camera or player 0 within which the
 * wall stays */
#define WALL_RANGE (500.0F * 500.0F)

/* the radius the wall's collision record takes in place of the one
 * collision_init finds */
#define RECORD_RADIUS 180.0F

/* the blocks of the lower run and of the top one */
#define LOWER_BLOCKS 14
#define TOP_BLOCKS 4
#define BLOCK_COUNT (LOWER_BLOCKS + TOP_BLOCKS)

/* the wall glows for GLOW_STEPS steps, and its blocks then fall for
 * FALL_STEPS more before FLAG_KNUCKLES_MR_ENTRANCE_RUIN is set */
#define GLOW_STEPS 50
#define FALL_STEPS 360

/* the glow's shade rises by GLOW_RISE each step the wall glows, up to 0. it
 * drops by GLOW_FADE each step the blocks fall, down to -1 */
#define GLOW_RISE 0.02F
#define GLOW_FADE 0.025F

#define GRAVITY (98.0F / 3600.0F)

/* the odds, each step, that a falling block drifts across, and that it
 * leaves a trail of dust */
#define DRIFT_ODDS 0.02F
#define TRAIL_ODDS 0.013F

enum block_entry_mode { ENTRY_INIT, ENTRY_RUN };

/* the wall's states, in work->submode */
enum block_wall_state {
    BLOCKS_STANDING,
    BLOCKS_GLOWING,
    BLOCKS_FALLING,
    BLOCKS_FALLEN
};

struct block {
    int delay; /* the steps it waits before it falls */
    NJS_POINT3 home; /* its position in the standing wall */
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct angles ang;
    struct angles spin; /* ang's change each step */
    NJS_MODEL *model;
};

struct block_wall {
    struct block blocks[BLOCK_COUNT];
    int count;
    /* the red, green and blue of the glow's constant material, from -1 as it
     * starts to glow up to 0 */
    Float shade;
};

struct block_entry_vars {
    Uint8 unk_00[8];
    /* from the heap: block_entry_init allocates it, and block_entry_destroy
     * frees it */
    struct block_wall *wall;
};

#define VARS(work) ((struct block_entry_vars *)&(work)->vars)

static void block_entry_init(struct task *task);
static void block_entry_reset(struct task *task);
static void block_entry_stack(struct task_work *work);
static void block_entry_update(struct task *task);
static void block_entry_draw(struct task *task);
static void block_entry_destroy(struct task *task);

static struct collision_shape block_entry_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, -2.125F, 0.0F},
    15.0F,
    2.125F,
    8.5F,
    {0},
    0,
    NULL,
};

static struct dust_params block_entry_dust = {
    1.0F,
    0.08F,
    0.09F,
    0.72F,
    0.15F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};
static struct dust_params block_entry_trail = {
    1.0F,
    0.08F,
    0.09F,
    0.72F,
    0.15F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

static Bool block_entry_unlocked(void)
{
    return lock_flag(KEY_GOLD_STATUE) && lock_flag(KEY_SILVER_STATUE);
}

void block_entry_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, WALL_RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case ENTRY_INIT:
            block_entry_init(task);
            break;
        case ENTRY_RUN:
            block_entry_draw(task);
            block_entry_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case ENTRY_INIT:
            block_entry_init(task);
            break;
        case ENTRY_RUN:
            block_entry_reset(task);
            block_entry_draw(task);
            block_entry_update(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            break;
        }
    }
}

static void block_entry_init(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->wall = heap_alloc(sizeof(struct block_wall));
    task->draw = block_entry_draw;
    task->destructor = block_entry_destroy;
    collision_init(task, &block_entry_shape, COLLISION_OBJECT, 1, 1);
    work->collision->radius = RECORD_RADIUS;
    block_entry_reset(task);
    work->mode = ENTRY_RUN;
}

static void block_entry_reset(struct task *task)
{
    struct task_work *work = task->work;

    block_entry_stack(work);
    if (block_entry_unlocked()) {
        event_flag_set(FLAG_KNUCKLES_MR_ENTRANCE_RUIN);
        work->submode = BLOCKS_FALLEN;
    } else {
        work->submode = BLOCKS_STANDING;
    }
}

static void block_entry_stack(struct task_work *work)
{
    struct block_wall *wall = VARS(work)->wall;
    struct block *block = wall->blocks;
    NJS_OBJECT *object;
    Angle ang;
    int i;
    int count = 0;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    object = &block_entry_blocks_object;
    for (i = 0; i < LOWER_BLOCKS; i++) {
        njCalcPoint(NULL, (NJS_POINT3 *)object->pos, &block->home);
        block->pos = block->home;
        block->model = object->model;
        block->ang.x = 0;
        block->ang.y = work->ang.y;
        block->ang.z = 0;
        count++;
        block++;
        object = object->sibling;
    }
    object = &block_entry_top_blocks_object;
    for (i = 0; i < TOP_BLOCKS; i++) {
        njCalcPoint(NULL, (NJS_POINT3 *)object->pos, &block->home);
        block->pos = block->home;
        block->model = object->model;
        block->ang.x = 0;
        block->ang.y = work->ang.y;
        block->ang.z = 0;
        count++;
        block++;
        object = object->sibling;
    }
    wall->count = count;
    njPopMatrix(1);
}

/* the wall glows and falls once both statues are in their locks, and a
 * fallen wall stands again once either leaves its lock. the wall is solid
 * only while it stands */
static void block_entry_update(struct task *task)
{
    struct task_work *work = task->work;
    struct block_wall *wall = VARS(work)->wall;
    struct block *block;
    int count;
    int left;

    if (work->submode == BLOCKS_STANDING) {
        if (block_entry_unlocked()) {
            wall->shade = -1.0F;
            TASK_TIMER(work) = 0;
            work->submode = BLOCKS_GLOWING;
        }
    } else if (work->submode == BLOCKS_GLOWING) {
        wall->shade += GLOW_RISE;
        if (wall->shade > 0.0F) {
            wall->shade = 0.0F;
        }
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= GLOW_STEPS) {
            block = wall->blocks;
            for (count = wall->count; count > 0; count--) {
                block->delay = (int)((rand() * RAND_SCALE * 155.0F) + 5.0F);
                block->speed.x = 0.0F;
                block->speed.z = 0.0F;
                block->speed.y = 0.0F;
                block->spin.x = ((rand() * RAND_SCALE * 2.0F) - 1.0F) * 256.0F;
                block->spin.y = ((rand() * RAND_SCALE * 2.0F) - 1.0F) * 256.0F;
                block->spin.z = ((rand() * RAND_SCALE * 2.0F) - 1.0F) * 256.0F;
                block++;
            }
            TASK_TIMER(work) = 0;
            work->submode = BLOCKS_FALLING;
        }
    } else if (work->submode == BLOCKS_FALLING) {
        block = wall->blocks;
        for (left = wall->count; left > 0; left--) {
            if (block->delay != 0) {
                block->delay--;
                if (block->delay == 0) {
                    block_entry_dust.scale = 0.5F;
                    block_entry_dust.speed.x = 0.0F;
                    block_entry_dust.speed.y = 1.0F;
                    block_entry_dust.speed.z = 0.0F;
                    block_entry_dust.pos = block->pos;
                    dust_create_from(&block_entry_dust);
                }
            } else {
                njAddVector(&block->pos, &block->speed);
                block->ang.x += block->spin.x;
                block->ang.y += block->spin.y;
                block->ang.z += block->spin.z;
                if (rand() * RAND_SCALE < DRIFT_ODDS) {
                    block->speed.x +=
                        ((rand() * RAND_SCALE * 2.0F) - 1.0F) * 0.25F;
                    block->speed.z +=
                        ((rand() * RAND_SCALE * 2.0F) - 1.0F) * 0.25F;
                }
                block->speed.y -= GRAVITY;
                block->speed.x *= 0.84F;
                block->speed.y *= 0.93F;
                block->speed.z *= 0.84F;
                block->spin.x *= 0.9F;
                block->spin.y *= 0.9F;
                block->spin.z *= 0.9F;
                if (rand() * RAND_SCALE < TRAIL_ODDS) {
                    block_entry_trail.scale = 0.5F;
                    block_entry_trail.speed.x = block->speed.x;
                    block_entry_trail.speed.z = block->speed.z;
                    block_entry_trail.speed.y = block->speed.y + 1.0F;
                    block_entry_trail.pos = block->pos;
                    dust_create_from(&block_entry_trail);
                }
            }
            block++;
        }
        wall->shade -= GLOW_FADE;
        if (wall->shade < -1.0F) {
            wall->shade = -1.0F;
        }
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= FALL_STEPS) {
            event_flag_set(FLAG_KNUCKLES_MR_ENTRANCE_RUIN);
            work->submode = BLOCKS_FALLEN;
        }
    } else if (!block_entry_unlocked()) {
        block_entry_stack(work);
        work->submode = BLOCKS_STANDING;
    }
    if (work->submode == BLOCKS_STANDING) {
        collision_register(work);
    }
}

static void block_entry_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct block_wall *wall = VARS(work)->wall;
    struct block *block;
    Angle ang;
    int count;
    Float shade;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (work->submode == BLOCKS_GLOWING || work->submode == BLOCKS_FALLING) {
        njTranslateV(NULL, (NJS_VECTOR *)block_entry_glow_object->pos);
        njScale(NULL, 1.0F, 3.0F, 1.0F);
        shade = wall->shade;
        draw_set_material(-0.4F, shade, shade, shade);
        draw_model(block_entry_glow_object->model);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    njPopMatrix(1);
    if (work->submode != BLOCKS_FALLEN) {
        block = wall->blocks;
        for (count = wall->count; count > 0; count--) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &block->pos);
            if (work->submode == BLOCKS_FALLING) {
                TURN_VIA(njRotateX, block->ang.x, ang);
                TURN_VIA(njRotateY, block->ang.y, ang);
                TURN_VIA(njRotateZ, block->ang.z, ang);
            } else {
                TURN_VIA(njRotateY, block->ang.y, ang);
            }
            draw_model(block->model);
            njPopMatrix(1);
            block++;
        }
    }
}

static void block_entry_destroy(struct task *task)
{
    heap_free(VARS(task->work)->wall);
}
