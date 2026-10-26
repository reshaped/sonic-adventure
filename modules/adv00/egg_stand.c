/* O EGGSTAND stand for the shop's egg, which sinks under an egg's weight */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/egg_stand.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT egg_stand_object;
extern NJS_OBJECT fake_egg_object;
extern NJS_OBJECT egg_stand_solid_object;

#define STAND_SINK 2.8F /* how far the stand sinks under an egg */
#define STAND_SPEED 0.2F /* how far the stand sinks or rises each step */
/* the top's height above the stand's position, where the solid and an egg
 * sit */
#define STAND_TOP 3.5F

/* the world flags of the stand's solid, which players stand on */
#define SOLID_WORLD_FLAGS (WORLD_SMALL | WORLD_MOVES | WORLD_SOLID)

/* the stand's work->mode, which skips 2: no code sets it */
enum egg_stand_mode {
    STAND_UP = 0,
    STAND_SINKING = 1,
    STAND_DOWN = 3,
    STAND_RISING = 4
};

struct egg_stand_vars {
    /* the solid the stand adds to the world, or NULL in the set editor and
     * once the real egg is taken into a garden */
    NJS_OBJECT *solid;
    Float up_y; /* the stand's height with no egg on it */
};

#define VARS(work) ((struct egg_stand_vars *)&(work)->vars)

static void egg_stand_draw(struct task *task);
static void egg_stand_swapped_draw(struct task *task);
static void egg_stand_step(struct task *task);
static void egg_stand_swapped_step(struct task *task);
static void egg_stand_destroy(struct task *task);

/* the shape the stand registers once the real egg is taken into a garden,
 * with the fake egg on it, in place of a solid */
/* clang-format off */
static struct collision_shape egg_stand_swapped_shape = {
    SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 3.5F, 0.0F},
    4.0F, 3.5F, 0.0F, {0}, 0, NULL,
};
/* clang-format on */

void station_square_egg_stand_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *solid;
    Bool swapped;

    work->mode = STAND_UP;
    VARS(work)->up_y = work->pos.y;
    work->timer = 0;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (placement_editing(task)) {
        VARS(work)->solid = NULL;
        task->step = egg_stand_step;
        return;
    }
    /* @hack spelling: the flag read into swapped, which the image tests in
     * R5 (MOV.B @(R0,R4),R5 at 0x0c9013a8); tested in place, it takes R2, and
     * 3 of 114 words differ */
    swapped = event_flags[FLAG_GOLD_EGG_IN];
    if (swapped) {
        event_flags[FLAG_EGG_ON_STAND] = TRUE;
        VARS(work)->solid = NULL;
        collision_init(task, &egg_stand_swapped_shape, COLLISION_OBJECT, 1, 1);
        task->step = egg_stand_swapped_step;
        task->draw = egg_stand_swapped_draw;
    } else {
        event_flags[FLAG_EGG_ON_STAND] = FALSE;
        VARS(work)->solid = solid = world_object_alloc();
        solid->evalflags =
            NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
        solid->scl[0] = 1.0F;
        solid->scl[1] = 1.0F;
        solid->scl[2] = 1.0F;
        solid->ang[0] = 0;
        solid->ang[1] = work->ang.y;
        solid->ang[2] = 0;
        solid->pos[0] = work->pos.x;
        solid->pos[1] = work->pos.y + STAND_TOP;
        solid->pos[2] = work->pos.z;
        solid->model = egg_stand_solid_object.model;
        solid->child = NULL;
        solid->sibling = NULL;
        world_add(SOLID_WORLD_FLAGS, task, solid);
        work->flags |= WORLD_TESTED;
        task->step = egg_stand_step;
        task->draw = egg_stand_draw;
    }
    task->destructor = egg_stand_destroy;
}

static void egg_stand_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&egg_stand_object);
    njPopMatrix(1);
}

/* draw the stand sunk, with the fake egg on it */
static void egg_stand_swapped_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y - STAND_SINK, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&egg_stand_object);
    njTranslate(NULL, 0.0F, STAND_TOP, 0.0F);
    draw_tree(&fake_egg_object);
    njPopMatrix(1);
}

/* sink the stand and its solid once an egg is on it, and raise them once the
 * egg is off */
static void egg_stand_step(struct task *task)
{
    struct task_work *work = task->work;
    Float *pos_y;
    Float y;
    Float up_y;
    Float sunk;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case STAND_UP:
            if (event_flags[FLAG_EGG_ON_STAND]) {
                work->mode = STAND_SINKING;
            }
            break;
        case STAND_SINKING:
            /* @hack spelling: the height read and written through pos_y, which
             * SHC forms in R4 (ADD #36,R4 at 0x0c9015ae), with y, up_y and
             * sunk in locals; as arithmetic on work->pos.y, 70 of 94 words
             * differ */
            pos_y = &work->pos.y;
            y = *pos_y;
            up_y = VARS(work)->up_y;
            y -= STAND_SPEED;
            sunk = up_y - y;
            if (sunk > STAND_SINK) {
                y = up_y - STAND_SINK;
                work->mode = STAND_DOWN;
            }
            VARS(work)->solid->pos[1] = y + STAND_TOP;
            *pos_y = y;
            break;
        case STAND_DOWN:
            if (!event_flags[FLAG_EGG_ON_STAND]) {
                work->mode = STAND_RISING;
            }
            break;
        case STAND_RISING:
            /* @hack spelling: the height read and written through pos_y, which
             * SHC forms in R4 (ADD #36,R4 at 0x0c9015ea), with y, up_y and
             * sunk in locals; as arithmetic on work->pos.y, 54 of 94 words
             * differ */
            pos_y = &work->pos.y;
            y = *pos_y;
            up_y = VARS(work)->up_y;
            y += STAND_SPEED;
            sunk = up_y - y;
            if (sunk < 0.0F) {
                work->mode = STAND_UP;
                y = up_y;
            }
            VARS(work)->solid->pos[1] = y + STAND_TOP;
            *pos_y = y;
            break;
        }
    }
    egg_stand_draw(task);
}

static void egg_stand_swapped_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_register(work);
    }
    egg_stand_swapped_draw(task);
}

static void egg_stand_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}
