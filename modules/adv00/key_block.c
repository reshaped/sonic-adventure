/* O KEYBLOCK stand and key block, which Big carries to its stand to open the
 * hotel's pool */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/key_block.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "event/script.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 12
#include "adv00/key_block/text.inc"

extern NJS_OBJECT key_block_stand_object;
extern NJS_OBJECT key_block_object;
extern NJS_OBJECT key_block_light_object;

/* set once the first block settles on its stand. each empty stand clears it
 * as it starts */
#define FLAG_FIRST_BLOCK_ON 0

/* a lying block settles on its stand within STAND_HEIGHT above or below it,
 * where its squared distance across is under STAND_REACH_SQUARED */
#define STAND_REACH_SQUARED 50.0F
#define STAND_HEIGHT 2.0F
#define SE_BLOCK_ON 0x20a /* the block settling on its stand */

enum key_block_stand_mode { STAND_FULL, STAND_EMPTY };
enum key_block_mode { BLOCK_LYING = 2, BLOCK_HELD = 3 };

static void key_block_full_step(struct task *task);
static void key_block_full_draw(struct task *task);
static void key_block_empty_step(struct task *task);
static void key_block_empty_draw(struct task *task);
static void key_block_destroy(struct task *task);
static void key_block_loose_init(struct task *task);
static void key_block_loose_step(struct task *task);
static void key_block_loose_draw(struct task *task);

/* clang-format off */
static struct collision_shape key_block_full_shape = {
    SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 3.0F, 0.0F},
    4.0F, 3.0F, 4.0F, {0}, 0, NULL,
};
static struct collision_shape key_block_empty_shape = {
    SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 1.5F, 0.0F},
    5.0F, 1.5F, 5.0F, {0}, 0, NULL,
};
static struct collision_shape key_block_loose_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 3.0F, 0.0F}, 6.5F, 3.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL, {0.0F, 3.0F, 0.0F},
     3.5F, 3.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static char *key_block_found[] = {
    TEXT_1,
    NULL,
};

/* TRUE when the blocks start on their stands */
static Bool key_block_on(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
    case CHARACTER_TAILS:
    case CHARACTER_KNUCKLES:
    case CHARACTER_AMY:
    case CHARACTER_E102:
        break;
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_SS_HOTEL_POOL];
    }
    return TRUE;
}

/* an empty stand creates its loose block at the stand's position plus the set
 * entry's scale. the set editor labels the scale's rows as the block's offset,
 * and draws the block there */
/* @hack redundant: register on task, which keeps it in R14 and work in R13
 * (MOV R4,R14 at 0x0c90ef5c); without it, the two trade and 23 of 144 words
 * differ */
void key_block_init(register struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *block;
    Angle ang;

    work->ang.x = 0;
    work->ang.z = 0;
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> BLOCK X");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> BLOCK Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> BLOCK Z");
        key_block_empty_draw(task);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njTranslate(NULL, work->scl.x, work->scl.y, work->scl.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&key_block_object);
        njPopMatrix(1);
    } else if (key_block_on()) {
        collision_init(task, &key_block_full_shape, COLLISION_OBJECT, 1, 1);
        task->step = key_block_full_step;
        task->draw = key_block_full_draw;
    } else {
        /* @hack spelling: the mode set in collision_init's count, which is also
         * 1. SHC then loads a 1 into R1 for the mode, the unused argument and
         * the count (MOV #1,R1 at 0x0c90effe). set before the call, SHC keeps
         * the 1 in R11, and 92 of 144 words differ */
        collision_init(task, &key_block_empty_shape, COLLISION_OBJECT, 1,
                       work->mode = STAND_EMPTY);
        child = task_create_child(TASK_WORK | TASK_MOTION, key_block_loose_init,
                                  task);
        block = child->work;
        block->pos.x += work->scl.x;
        block->pos.y += work->scl.y;
        block->pos.z += work->scl.z;
        key_block_light_object.model->mats[0].attrflags |= NJD_FLAG_USE_ALPHA;
        task->step = key_block_empty_step;
        task->draw = key_block_empty_draw;
        task->destructor = key_block_destroy;
        event_flags[FLAG_FIRST_BLOCK_ON] = FALSE;
    }
}

static void key_block_full_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        collision_register(work);
        key_block_full_draw(task);
    }
}

/* draw the stand with the block on it */
static void key_block_full_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&key_block_stand_object);
    draw_tree(&key_block_object);
    njPopMatrix(1);
}

/* settle the stand's block once it lies within reach. the block's task ends,
 * and the stand draws the block from then on but keeps its empty shape. the
 * first block to settle sets FLAG_FIRST_BLOCK_ON, and the second sets
 * FLAG_BIG_SS_HOTEL_POOL */
static void key_block_empty_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *block;
    Float x;
    Float y;
    Float z;
    Float distance;

    if (!placement_try_leave(task)) {
        switch (work->mode) {
        case STAND_EMPTY:
            block = task->child->work;
            if (!(block->flags & CARRY_HELD)) {
                x = block->pos.x - work->pos.x;
                y = block->pos.y - work->pos.y;
                z = block->pos.z - work->pos.z;
                if (_builtin_fabsf(y) <= STAND_HEIGHT) {
                    distance = (x * x) + (z * z);
                    if (distance < STAND_REACH_SQUARED) {
                        work->mode = STAND_FULL;
                        task_destroy_children(task);
                        sound_play(SE_BLOCK_ON, 0, 0, 0);
                        task->draw = key_block_full_draw;
                        if (event_flags[FLAG_FIRST_BLOCK_ON] == TRUE) {
                            event_flags[FLAG_BIG_SS_HOTEL_POOL] = TRUE;
                        } else {
                            event_flags[FLAG_FIRST_BLOCK_ON] = TRUE;
                        }
                    }
                }
            }
            key_block_empty_draw(task);
            break;
        case STAND_FULL:
            key_block_full_draw(task);
            break;
        }
        collision_register(work);
        task_step_children(task);
    }
}

/* draw the stand with the light over it */
static void key_block_empty_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&key_block_stand_object);
    draw_tree(&key_block_light_object);
    njPopMatrix(1);
}

static void key_block_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void key_block_loose_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    work->mode = BLOCK_LYING;
    carry_set_kind(work, motion, CARRY_KIND_SMALL);
    work->flags |= CARRY_WALLS;
    collision_init(task, key_block_loose_shapes, COLLISION_OBJECT, 1, 2);
    task->step = key_block_loose_step;
    task->draw = key_block_loose_draw;
}

static void key_block_loose_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BLOCK_LYING:
        carry_step_small(task);
        if (work->flags & CARRY_HELD) {
            work->mode = BLOCK_HELD;
            message_show_timed(key_block_found);
        }
        break;
    case BLOCK_HELD:
        if (!(work->flags & CARRY_HELD)) {
            work->mode = BLOCK_LYING;
        } else {
            work->ang.y = -player_work[0]->ang.y;
        }
        break;
    }
    key_block_loose_draw(task);
}

static void key_block_loose_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&key_block_object);
    njPopMatrix(1);
}
