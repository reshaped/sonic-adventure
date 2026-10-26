/* O_Hashra1B: a pillar whose top breaks into falling pieces once something
 * touches it. O_Hashra_C and O_Hashra10C: solid pillars */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg08/pillars.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL icecap_pillar_1b_base_model;
extern NJS_MODEL icecap_pillar_1b_top_model;
extern NJS_MODEL icecap_pillar_1b_piece_1_model;
extern NJS_MODEL icecap_pillar_1b_piece_2_model;
extern NJS_MODEL icecap_pillar_1b_piece_3_model;
extern NJS_MODEL icecap_pillar_1b_piece_4_model;
extern NJS_MODEL icecap_pillar_1b_piece_5_model;
extern NJS_MODEL icecap_pillar_1b_piece_6_model;
extern NJS_MODEL icecap_pillar_1b_piece_7_model;
extern NJS_MODEL icecap_pillar_1b_piece_8_model;
extern NJS_MODEL icecap_pillar_c_model;
extern NJS_OBJECT icecap_pillar_10c_object;

/* in work->mode */
enum icecap_pillar_kind { PILLAR_1B, PILLAR_C, PILLAR_10C };

/* in O_Hashra1B's work->flags: its top has broken off, and its pieces have
 * fallen out of the draw */
#define PILLAR_BROKEN 0x100
#define PILLAR_FALLEN 0x200

/* the broken top's pieces start at height PIECES_DROP. each step takes
 * PIECES_FALL from their vertical speed and turns them PIECES_TILT further.
 * they leave the draw once that speed is below PIECES_GONE. a piece's scale
 * loses PIECES_SHRINK for each unit of its downward speed */
#define PIECES_DROP 3.0F
#define PIECES_FALL 0.2F
#define PIECES_GONE (-10.0F)
#define PIECES_TILT 0x800
#define PIECES_SHRINK 0.1F

/* the collision world tests O_Hashra10C only while a player is near it. that
 * is within its model's radius plus PILLAR_NEAR of the pillar's position */
#define PILLAR_NEAR 20.0F

struct icecap_pillar_vars {
    NJS_OBJECT *object; /* O_Hashra10C's copy in the collision world */
    Angle tilt; /* how far O_Hashra1B's pieces have turned */
};

#define VARS(work) ((struct icecap_pillar_vars *)&(work)->vars)

/* in work->scl.x and work->scl.y: the pieces' height and their vertical
 * speed */
#define PIECES_HEIGHT(work) ((work)->scl.x)
#define PIECES_SPEED(work) ((work)->scl.y)

/* one capsule around O_Hashra1B and one around O_Hashra_C, and three on
 * O_Hashra10C */
#define PILLAR_SHAPE(x, y, z, radius, height)                                  \
    {SHAPE_CAPSULE,                                                            \
     0,                                                                        \
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,                                           \
     {(x), (y), (z)},                                                          \
     (radius),                                                                 \
     (height),                                                                 \
     0.0F,                                                                     \
     {0},                                                                      \
     0,                                                                        \
     NULL}
static struct collision_shape icecap_pillar_1b_shape =
    PILLAR_SHAPE(1.0F, -40.0F, 0.0F, 6.0F, 40.0F);
static struct collision_shape icecap_pillar_c_shape =
    PILLAR_SHAPE(0.0F, -18.0F, 0.0F, 4.0F, 18.0F);
static struct collision_shape icecap_pillar_10c_shapes[3] = {
    PILLAR_SHAPE(35.0F, 62.0F, 25.0F, 7.0F, 28.0F),
    PILLAR_SHAPE(45.0F, 56.0F, -9.0F, 7.0F, 34.0F),
    PILLAR_SHAPE(16.0F, 75.0F, -29.0F, 7.0F, 15.0F),
};

/* the pieces' height during a draw, one static shared by every pillar */
static Float icecap_pillar_pieces_height;

static void icecap_pillar_destroy(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (work->mode == PILLAR_10C) {
        object = VARS(work)->object;
        world_remove(task, object);
        world_object_free(object);
    }
}

/* draw model at x, y and z, turned tilt about z and half that about x. it
 * shrinks as speed falls below 0 */
static void icecap_pillar_draw_piece(Float speed, Angle tilt, Float x, Float y,
                                     Float z, NJS_MODEL *model)
{
    Float size = 1.0F + (speed * PIECES_SHRINK);
    Angle ang;

    njPushMatrix(NULL);
    njTranslate(NULL, x, y, z);
    TURN(njRotateZ, tilt);
    TURN_VIA(njRotateX, tilt / 2, ang);
    njScale(NULL, size, size, size);
    draw_model(model);
    njPopMatrix(1);
}

/* O_Hashra1B's step, and its draw too, so its pieces fall on paused frames
 * as well. the pillar stands whole until a touch breaks its top into pieces.
 * its base stays */
static void icecap_pillar_1b_run(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the angles read through a pointer of their own, which
     * holds &work->ang in R12 (ADD #20,R12 at 0x0c9019f6); through work, R12
     * goes unused and 234 of 238 words differ */
    struct angles *turn = &work->ang;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, turn->z, ang);
    TURN_VIA(njRotateX, turn->x, ang);
    TURN_VIA(njRotateY, turn->y, ang);
    draw_model(&icecap_pillar_1b_base_model);
    if (work->flags & PILLAR_BROKEN) {
        if (!(work->flags & PILLAR_FALLEN)) {
            icecap_pillar_pieces_height = PIECES_HEIGHT(work);
            icecap_pillar_draw_piece(PIECES_SPEED(work), VARS(work)->tilt, 0.0F,
                                     icecap_pillar_pieces_height - 10.0F, -7.0F,
                                     &icecap_pillar_1b_piece_1_model);
            icecap_pillar_draw_piece(PIECES_SPEED(work), VARS(work)->tilt, 0.0F,
                                     icecap_pillar_pieces_height - 10.000006F,
                                     7.0F, &icecap_pillar_1b_piece_2_model);
            icecap_pillar_draw_piece(PIECES_SPEED(work), VARS(work)->tilt,
                                     -6.0F,
                                     icecap_pillar_pieces_height - 29.999991F,
                                     0.0F, &icecap_pillar_1b_piece_3_model);
            icecap_pillar_draw_piece(PIECES_SPEED(work), VARS(work)->tilt, 3.0F,
                                     icecap_pillar_pieces_height - 29.999995F,
                                     0.0F, &icecap_pillar_1b_piece_4_model);
            icecap_pillar_draw_piece(PIECES_SPEED(work), VARS(work)->tilt, 0.0F,
                                     icecap_pillar_pieces_height - 50.0F, -3.0F,
                                     &icecap_pillar_1b_piece_5_model);
            icecap_pillar_draw_piece(PIECES_SPEED(work), VARS(work)->tilt, 0.0F,
                                     icecap_pillar_pieces_height - 50.0F, 3.0F,
                                     &icecap_pillar_1b_piece_6_model);
            icecap_pillar_draw_piece(PIECES_SPEED(work), VARS(work)->tilt, 0.0F,
                                     icecap_pillar_pieces_height - 70.0F, 0.0F,
                                     &icecap_pillar_1b_piece_7_model);
            icecap_pillar_draw_piece(PIECES_SPEED(work), VARS(work)->tilt, 0.0F,
                                     icecap_pillar_pieces_height - 88.0F, 0.0F,
                                     &icecap_pillar_1b_piece_8_model);
            PIECES_SPEED(work) -= PIECES_FALL;
            if (PIECES_SPEED(work) < PIECES_GONE) {
                work->flags += PILLAR_FALLEN;
            } else {
                VARS(work)->tilt += PIECES_TILT;
                icecap_pillar_pieces_height += PIECES_SPEED(work);
                PIECES_HEIGHT(work) = icecap_pillar_pieces_height;
            }
        }
    } else {
        draw_model(&icecap_pillar_1b_top_model);
        if (work->collision->flags & COLLISION_HIT) {
            work->flags += PILLAR_BROKEN;
        } else if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
    }
    njPopMatrix(1);
}

static void icecap_pillar_c_run(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&icecap_pillar_c_model);
    njPopMatrix(1);
}

static void icecap_pillar_10c_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    if (player_in_range(&work->pos, object->model->r + PILLAR_NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(object->model);
    njPopMatrix(1);
}

void icecap_pillar_1b_step(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = PILLAR_1B;
    work->flags &= ~(PILLAR_BROKEN | PILLAR_FALLEN);
    VARS(work)->tilt = 0;
    PIECES_HEIGHT(work) = PIECES_DROP;
    PIECES_SPEED(work) = 0.0F;
    collision_init(task, &icecap_pillar_1b_shape, COLLISION_OBJECT, 1, 1);
    task->step = icecap_pillar_1b_run;
    task->draw = icecap_pillar_1b_run;
    task->destructor = icecap_pillar_destroy;
}

void icecap_pillar_c_step(struct task *task)
{
    task->work->mode = PILLAR_C;
    collision_init(task, &icecap_pillar_c_shape, COLLISION_OBJECT, 1, 1);
    task->step = icecap_pillar_c_run;
    task->draw = icecap_pillar_c_run;
    task->destructor = icecap_pillar_destroy;
}

void icecap_pillar_10c_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    VARS(work)->object = object_add_world(&icecap_pillar_10c_object, task,
                                          WORLD_MEDIUM | WORLD_SOLID);
    object = VARS(work)->object;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    collision_init(task, icecap_pillar_10c_shapes, COLLISION_OBJECT, 1, 3);
    work->mode = PILLAR_10C;
    task->step = icecap_pillar_10c_run;
    task->draw = icecap_pillar_10c_run;
    task->destructor = icecap_pillar_destroy;
}
