/* the grass. KusaA, KusaB and KusaC are tufts that sway as a player runs
 * through them. Shitakusa is undergrowth that throws up leaves round player 0
 * running through it. GakeKusa is the grass on the cliffs, a model that only
 * draws */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg07/grass.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION grass_a_action;
extern NJS_ACTION grass_b_action;
extern NJS_ACTION grass_c_action;
extern NJS_OBJECT undergrowth_object;
extern NJS_MODEL grass_leaf_model;
extern NJS_OBJECT cliff_grass_object;

struct grass_vars {
    Float frame;
    Sint32 kind;
    Float sway; /* how far frame advances each step, in action frames */
};

/* a leaf rises, its speed halving each step while above -LEAF_FALL. it then
 * falls at LEAF_FALL, spinning */
struct leaf_vars {
    /* the height it starts at. its task ends once it falls below it */
    Float floor;
    Float speed; /* its rise each step */
    Sint32 spin; /* its turn about z each step it falls. it turns half that
                  * about y, and a third about x */
};

#define VARS(work) ((struct grass_vars *)&(work)->vars)
#define LEAF(work) ((struct leaf_vars *)&(work)->vars)

enum grass_kind { GRASS_A, GRASS_B, GRASS_C, UNDERGROWTH };

/* the frames of a tuft's action. a tuft's frame drops by SWAY_FRAMES once it
 * reaches SWAY_FRAMES - 1, which leaves it negative */
#define SWAY_FRAMES 8.0F
/* the sway a tuft loses each step, down to 0 */
#define SWAY_DRAG 0.02F
/* a touch while the work's tick is 0 starts it counting down from
 * TOUCH_STEPS. nothing else reads the tick */
#define TOUCH_STEPS 20

/* a falling leaf's speed */
#define LEAF_FALL (-0.25F)

/* clang-format off */
/* by grass_kind: one shape for each tuft, then the undergrowth's two */
static struct collision_shape grass_shapes[5] = {
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.5F, 0.0F}, 10.0F, 0.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.5F, 0.0F}, 12.0F, 0.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.5F, 0.0F}, 7.0F, 0.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F}, 12.0F, 0.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.5F, 0.0F}, 8.0F, 0.5F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* player 0's speed ahead, as player_ground_speed last gave it. it keeps its
 * value while player 0 is off the ground */
static Float grass_speed;

static void grass_destroy(struct task *task)
{
}

static void grass_leaf_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njEasyDrawModel(&grass_leaf_model);
    njPopMatrix(1);
}

static void grass_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    switch (VARS(work)->kind) {
    case GRASS_A:
        draw_action(&grass_a_action, VARS(work)->frame);
        break;
    case GRASS_B:
        draw_action(&grass_b_action, VARS(work)->frame);
        break;
    case GRASS_C:
        draw_action(&grass_c_action, VARS(work)->frame);
        break;
    case UNDERGROWTH:
        draw_tree(&undergrowth_object);
        break;
    }
    njPopMatrix(1);
}

static void grass_leaf_step(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.y += LEAF(work)->speed;
    if (LEAF(work)->floor > work->pos.y) {
        task_kill(task);
        return;
    }
    if (LEAF(work)->speed != LEAF_FALL) {
        if (LEAF(work)->speed > -LEAF_FALL) {
            /* @hack spelling: the product spelled out, which loads 0.5 into
             * FR2 by MOVA and FMOV.S (0x0c906cda); with *=, SHC loads it
             * through R1 and FPUL, and 4 of 59 words differ */
            LEAF(work)->speed = LEAF(work)->speed * 0.5F;
        } else {
            LEAF(work)->speed = LEAF_FALL;
        }
    } else {
        work->ang.x += LEAF(work)->spin / 3;
        work->ang.y += LEAF(work)->spin / 2;
        work->ang.z += LEAF(work)->spin;
    }
    grass_leaf_draw(task);
}

/* the grass's step. a touch sets a tuft's sway by its kind, unless
 * grass_speed is 0. while grass_speed is 0.1 or more, a touch throws up a
 * leaf from the undergrowth near player 0. the leaf's first rise is 3 more
 * than grass_speed */
static void grass_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task *leaf;
    struct task_work *leaf_work;
    NJS_POINT3 *player_pos;

    if (placement_try_leave(task)) {
        return;
    }
    player_ground_speed(0, &grass_speed);
    switch (VARS(work)->kind) {
    case GRASS_A:
        if ((work->collision->flags & COLLISION_HIT) && grass_speed != 0.0F) {
            VARS(work)->sway = 0.5F;
            if (!work->tick) {
                work->tick = TOUCH_STEPS;
            }
        }
        VARS(work)->frame += VARS(work)->sway;
        if (VARS(work)->frame >= SWAY_FRAMES - 1.0F) {
            VARS(work)->frame -= SWAY_FRAMES;
        }
        VARS(work)->sway -= SWAY_DRAG;
        if (VARS(work)->sway < 0.0F) {
            VARS(work)->sway = 0.0F;
        }
        break;
    case GRASS_B:
        if ((work->collision->flags & COLLISION_HIT) && grass_speed != 0.0F) {
            VARS(work)->sway = 0.8F;
            if (!work->tick) {
                work->tick = TOUCH_STEPS;
            }
        }
        VARS(work)->frame += VARS(work)->sway;
        if (VARS(work)->frame >= SWAY_FRAMES - 1.0F) {
            VARS(work)->frame -= SWAY_FRAMES;
        }
        VARS(work)->sway -= SWAY_DRAG;
        if (VARS(work)->sway < 0.0F) {
            VARS(work)->sway = 0.0F;
        }
        break;
    case GRASS_C:
        if ((work->collision->flags & COLLISION_HIT) && grass_speed != 0.0F) {
            VARS(work)->sway = 1.0F;
            if (!work->tick) {
                work->tick = TOUCH_STEPS;
            }
        }
        VARS(work)->frame += VARS(work)->sway;
        if (VARS(work)->frame >= SWAY_FRAMES - 1.0F) {
            VARS(work)->frame -= SWAY_FRAMES;
        }
        VARS(work)->sway -= SWAY_DRAG;
        if (VARS(work)->sway < 0.0F) {
            VARS(work)->sway = 0.0F;
        }
        break;
    case UNDERGROWTH:
        if ((work->collision->flags & COLLISION_HIT) && grass_speed >= 0.1F) {
            player_pos = &player_work[0]->pos;
            leaf = task_create(TASK_WORK, 3, grass_leaf_step);
            leaf->draw = grass_leaf_draw;
            leaf->destructor = grass_destroy;
            leaf_work = leaf->work;
            leaf_work->pos.x =
                player_pos->x + (rand() * (8.0F * RAND_SCALE)) - 4.0F;
            leaf_work->pos.y = LEAF(leaf_work)->floor = work->pos.y;
            leaf_work->pos.z =
                player_pos->z + (rand() * (8.0F * RAND_SCALE)) - 4.0F;
            LEAF(leaf_work)->spin = (int)(rand() * (1.0F / 9.0F)) + 16;
            LEAF(leaf_work)->speed = grass_speed + 3.0F;
            if (!work->tick) {
                work->tick = TOUCH_STEPS;
            }
        }
        break;
    }
    if (work->tick) {
        work->tick--;
    }
    grass_draw(task);
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
}

static void grass_start(struct task *task)
{
    struct task_work *work = task->work;

    switch (VARS(work)->kind) {
    case GRASS_A:
        collision_init(task, &grass_shapes[0], COLLISION_OBJECT, 1, 1);
        VARS(work)->frame = 0.0F;
        break;
    case GRASS_B:
        collision_init(task, &grass_shapes[1], COLLISION_OBJECT, 1, 1);
        VARS(work)->frame = 0.0F;
        break;
    case GRASS_C:
        collision_init(task, &grass_shapes[2], COLLISION_OBJECT, 1, 1);
        VARS(work)->frame = 0.0F;
        break;
    case UNDERGROWTH:
        collision_init(task, &grass_shapes[3], COLLISION_OBJECT, 1, 2);
        break;
    }
    work->tick = 0;
    task->step = grass_run;
    task->draw = grass_draw;
    task->destructor = grass_destroy;
}

void grass_a_step(struct task *task)
{
    VARS(task->work)->kind = GRASS_A;
    grass_start(task);
}

void grass_b_step(struct task *task)
{
    VARS(task->work)->kind = GRASS_B;
    grass_start(task);
}

void grass_c_step(struct task *task)
{
    VARS(task->work)->kind = GRASS_C;
    grass_start(task);
}

void undergrowth_step(struct task *task)
{
    VARS(task->work)->kind = UNDERGROWTH;
    grass_start(task);
}

void cliff_grass_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &cliff_grass_object;
    model_start(task);
}
