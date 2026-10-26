/* O BIGTURARA: a big icicle that falls after the small icicles around it.
 * O FUTAL, O FUTAM and O FUTAM2: ice lids (futa) that break when struck */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>
#include <string.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/snow.h"
#include "effect/splash.h"
#include "object/common.h"
#include "object/panel.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg08/big_icicle.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT big_icicle_object;
extern NJS_OBJECT small_icicle_object;
extern NJS_OBJECT dat_0ca7debc;
/* drawn with hikari_texlist's first five textures, hikari02 to hikari06 */
extern NJS_OBJECT big_icicle_glint_object;
extern NJS_TEXLIST hikari_texlist;
/* the lids' models, whole and broken. O FUTAM and O FUTAM2 share
 * futa_m_object. a broken lid's children are its pieces */
extern NJS_OBJECT futa_l_object;
extern NJS_OBJECT futa_m_object;
extern NJS_OBJECT futa_l_broken_object;
extern NJS_OBJECT futa_m_broken_object;
extern NJS_OBJECT futa_m2_broken_object;

/* the ranges placement_try_leave_by ends a task beyond, squared. they are
 * about 2100 units for a big icicle, and about 1000 for a lid */
#define BIG_ICICLE_RANGE 4410100.0F
#define FUTA_RANGE 1000100.0F

/* the small icicles that fall ahead of the big one */
#define ICICLES 11
/* how far below the task the big icicle hangs */
#define BIG_ICICLE_HANG 149.19166F
/* with scl.y 0, how far below the task the big icicle rests */
#define BIG_ICICLE_REST 485.89563F
/* with scl.z 0, how far below the big icicle's rest the small icicles land,
 * less up to 20 */
#define SMALL_ICICLE_LANDING 378.0F
/* the collision world tests the big icicle or a lid only while a player is
 * near it. near means within its model's radius plus PLAYER_NEAR of a
 * point. for the big icicle, the point is the task's position plus height
 * along y. for a lid, it is the model's center, turned and moved with the
 * lid */
#define PLAYER_NEAR 16.0F
/* how far past the whole lid model's radius draw_in_view looks for a lid */
#define VIEW_MARGIN 16.0F
/* the view shakes until this many steps after the big icicle is down */
#define SHAKE_STEPS 200
/* what a falling icicle's upward speed loses each step */
#define ICICLE_GRAVITY 0.8F

#define SE_ICICLE_LAND 210
#define SE_RUMBLE 212
#define SE_FUTA_BREAK 224

/* a puff's alpha: more than 0.4 and at most 0.7, nearer 0.7 more often */
#define PUFF_ALPHA() (0.4F + (0.3F * (1.0F - powf(rand() * RAND_SCALE, 2.0F))))

/* an icicle's phase: it hangs, falls, lands until it settles, then is down.
 * the big icicle's record keeps its own phase in mode */
enum icicle_phase {
    ICICLE_HANGING,
    ICICLE_FALLING,
    ICICLE_LANDING,
    ICICLE_DOWN
};

/* a big icicle's record, from the heap. it holds the big icicle's fall and
 * the shake the landings give the view. it holds the small icicles too, an
 * array for each of their fields */
struct big_icicle {
    Float shake_speed; /* added to shake each step */
    Float shake; /* how far the view sinks */
    Angle shake_turn; /* the direction the view sways in */
    Float speed; /* the big icicle's upward speed */
    Float height; /* the big icicle's y, less the task's */
    Float landing; /* the height its fall ends at */
    Float rest; /* the height it rests at */
    int countdown; /* steps until it falls, then steps since it landed */
    int mode; /* an icicle_phase */
    /* steps until each small icicle falls, then steps since it landed */
    int delays[ICICLES];
    Float landings[ICICLES]; /* the heights they land at */
    Float speeds[ICICLES]; /* their upward speeds */
    Angle turns[ICICLES]; /* added to yaws each step */
    Angle yaws[ICICLES];
    Angle wobble_speeds[ICICLES]; /* added to wobbles each step */
    /* their tilts about x, from the half turn that draws them upside down */
    Angle wobbles[ICICLES];
    NJS_POINT3 pos[ICICLES]; /* their offsets from the task's position */
    int phases[ICICLES];
    /* big_icicle_glint_object's copy, its model's and its first material's,
     * whose texture the draw steps through */
    NJS_OBJECT glint_object;
    NJS_MODEL glint_model;
    NJS_MATERIAL glint_material;
    Bool active; /* until the big icicle is down */
    Bool enabled; /* once the player jumps from jump panel 3 */
};

struct big_icicle_vars {
    NJS_OBJECT *object; /* the big icicle's object in the world */
    Uint8 unk_04[4];
    struct big_icicle *icicle;
};

#define BIG_ICICLE(work) ((struct big_icicle_vars *)&(work)->vars)

struct icicle_puff_vars {
    struct dust_params *params;
    Float ground;
};

#define ICICLE_PUFF(particle) ((struct icicle_puff_vars *)&(particle)->vars)

enum futa_mode { FUTA_WHOLE, FUTA_BREAKING, FUTA_BROKEN };

/* a piece of a broken lid. pos, speed and ang are in the lid's frame */
struct futa_piece {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct angles ang;
    struct angles spin; /* added to ang each step */
};

/* a lid's record, from the heap */
struct futa {
    /* from the heap, from the lid's break until every piece has fallen, and
     * null before and after */
    struct futa_piece *pieces;
    int count; /* of pieces */
    NJS_OBJECT *whole;
    NJS_OBJECT *broken;
};

struct futa_vars {
    NJS_OBJECT *object; /* the lid's object in the world */
    Uint8 unk_04[4];
    struct futa *futa;
};

#define FUTA(work) ((struct futa_vars *)&(work)->vars)

/* the small icicles' places from the task. big_icicle_hang and
 * big_icicle_put_down move each 8 along -x, and big_icicle_hang moves each
 * 826 down too */
static NJS_POINT3 small_icicle_points[ICICLES] = {
    {-77.0F, 791.0F, -15.0F}, {43.0F, 732.0F, -71.0F}, {-8.0F, 397.0F, -21.0F},
    {26.0F, 436.0F, -23.0F},  {55.0F, 481.0F, -11.0F}, {56.0F, 548.0F, 21.0F},
    {-52.0F, 712.0F, 64.0F},  {22.0F, 636.0F, 79.0F},  {-69.0F, 770.0F, 31.0F},
    {-18.0F, 664.0F, 82.0F},  {50.0F, 601.0F, 56.0F},
};
/* the big icicle's cylinders, top to bottom as it hangs.
 * big_icicle_update_collision moves the last three with its fall */
/* clang-format off */
static struct collision_shape big_icicle_shapes[4] = {
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -76.2F, 0.0F}, 90.0F, 74.0F, 20.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -209.2F, 0.0F}, 90.0F, 60.0F, 20.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -339.2F, 0.0F}, 70.0F, 80.0F, 20.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_NO_PUSH, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -475.2F, 0.0F}, 30.0F, 80.0F, 20.0F, {0}, 0, NULL},
};
static struct dust_params small_icicle_puff = {
    2.3F,
    0.06383999F,
    0.12666667F,
    0.14F,
    -0.8F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.6F, 0.7F, 0.7F, 0.75F},
};
static struct dust_params big_icicle_puff = {
    3.0F,
    0.07523999F,
    0.11176471F,
    0.15F,
    -0.8F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.6F, 0.7F, 0.7F, 0.75F},
};
/* clang-format on */
static struct dust_params landing_puff = {
    2.9F,
    0.030399997F,
    0.076F,
    0.4F,
    -0.4F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.6F, 0.7F, 0.7F, 0.75F},
};
/* the big icicle that started while this was null, until it's destroyed */
static struct task *big_icicle_first = NULL;
/* 4 bytes no code reads; their type is unknown */
static Uint8 dat_0cab1a44[4] = {0};
/* a lid's cylinder. futa_init sets its radius to 0.6 times the whole lid
 * model's */
/* clang-format off */
static struct collision_shape futa_shape = {
    SHAPE_CYLINDER | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F}, 15.0F, 8.0F, 0.0F, {0}, 0, NULL};
/* clang-format on */

static void futa_destroy(struct task *task);
static void futa_run(struct task *task);
static void futa_draw(struct task *task);
static void big_icicle_start(struct task *task);
static void big_icicle_destroy(struct task *task);
static void big_icicle_run(struct task *task);
static void big_icicle_draw(struct task *task);
static void icicle_puff_create(struct dust_params *params, NJS_VECTOR *speed,
                               NJS_POINT3 *pos, Float ground);
static void futa_init(struct task *task, NJS_OBJECT *whole, NJS_OBJECT *broken,
                      int count);

Bool big_icicle_landed(void)
{
    struct task *task = big_icicle_first;

    if (task != NULL) {
        struct task_work *work = task->work;
        struct big_icicle *icicle = BIG_ICICLE(work)->icicle;

        if (!icicle->active || icicle->mode == ICICLE_LANDING ||
            icicle->mode == ICICLE_DOWN) {
            return TRUE;
        }
    }
    return FALSE;
}

/* make a ring of puffs of small_icicle_puff about point, which is in the
 * task's frame. each puff starts radius from point. the puffs stand
 * 360 / count degrees apart, from a random start. each flies outward and up.
 * every other one is slower, and speed scales them all. count must be 1 to
 * 360. 0 divides by zero, and past 360 the step is 0, so the loop never
 * ends */
static void small_icicle_ring(NJS_POINT3 *point, struct task_work *work,
                              Float radius, int count, Float speed)
{
    int degrees;
    int index;
    int step = 360 / count;
    int phase = (int)(rand() * (360.0F * RAND_SCALE));
    struct big_icicle *icicle = BIG_ICICLE(work)->icicle;
    Float landing = icicle->landing;
    NJS_POINT3 pos;
    NJS_POINT3 center;
    NJS_VECTOR velocity;
    Float cosine;
    Float rise;
    Float sine;
    Float ground;
    Angle angle;

    njPushMatrix(&_unit_matrix_);
    /* @hack type: plain 0s for the matrices: with NULL in both calls, index
     * takes R5 for R11 (0x0c909808) and 148 of 205 words differ */
    njTranslateV(0, &work->pos);
    TURN(njRotateY, work->ang.y);
    njCalcPoint(0, point, &center);
    njPopMatrix(1);
    /* @hack spelling: index started from origin, a 0 in a scope of its own,
     * which starts degrees and index from R4 (0x0c909802 to 0x0c909808) and
     * leaves step and landing at sp+8 and sp+12 (0x0c9097b0, 0x0c9097c8);
     * from 0, the two trade and 68 of 205 words differ */
    {
        int origin = 0;

        for (degrees = 0, index = origin; degrees < 360;
             degrees += step, ++index) {
            /* @hack spelling: angle set inside njCos's argument, which keeps
             * the frame at 60 bytes (0x0c909798); set before the call, it takes
             * 56 and 100 of 205 words differ */
            cosine =
                njCos(angle = (Angle)((phase + degrees) * ANGLE_PER_DEGREE));
            rise = 0.8F;
            sine = njSin(angle);
            pos.x = center.x + (radius * cosine);
            pos.y = center.y;
            pos.z = center.z + (radius * sine);
            if (index & 1) {
                velocity.x = speed * cosine * 9.0F;
                velocity.y = speed * rise * 12.0F;
                velocity.z = speed * sine * 9.0F;
            } else {
                velocity.x = speed * cosine * 11.0F;
                velocity.y = speed * rise * 14.0F;
                velocity.z = speed * sine * 11.0F;
            }
            small_icicle_puff.color.a = PUFF_ALPHA();
            ground = landing + (rand() * (30.0F * RAND_SCALE));
            icicle_puff_create(&small_icicle_puff, &velocity, &pos, ground);
        }
        /* @hack redundant: a never-run use of degrees, which keeps work in R14
         * and rand's address in R13 (0x0c90979c, 0x0c9097ac); without it, the
         * two trade and 12 of 205 words differ */
        if (0) {
            velocity.x = degrees;
        }
    }
}

/* the ring small_icicle_ring makes, but of big_icicle_puff's puffs, which
 * fly faster. nothing in the image calls it. it isn't static, since SHC
 * leaves out a static function no code calls */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void big_icicle_ring(NJS_POINT3 *point, struct task_work *work, Float radius,
                     int count, Float speed)
{
    int degrees;
    int index;
    int step = 360 / count;
    int phase = (int)(rand() * (360.0F * RAND_SCALE));
    struct big_icicle *icicle = BIG_ICICLE(work)->icicle;
    Float landing = icicle->landing;
    NJS_POINT3 pos;
    NJS_POINT3 center;
    NJS_VECTOR velocity;
    Float cosine;
    Float rise;
    Float sine;
    Float ground;
    Angle angle;

    njPushMatrix(&_unit_matrix_);
    /* @hack type: plain 0s for the matrices: with NULL in both calls, index
     * takes R5 for R11 (0x0c909a4a) and 86 of 211 words differ */
    njTranslateV(0, &work->pos);
    TURN(njRotateY, work->ang.y);
    njCalcPoint(0, point, &center);
    njPopMatrix(1);
    /* @hack spelling: index started from origin, a 0 in a scope of its own,
     * which starts degrees and index from R4 (0x0c909a44 to 0x0c909a4a) and
     * leaves step and landing at sp+12 and sp+16 (0x0c9099f2, 0x0c909a0a);
     * from 0, the two trade and 94 of 211 words differ */
    {
        int origin = 0;

        for (degrees = 0, index = origin; degrees < 360;
             degrees += step, ++index) {
            /* @hack spelling: angle set inside njCos's argument, which keeps
             * the frame at 64 bytes (0x0c9099d8); set before the call, it takes
             * 60 and 126 of 211 words differ */
            cosine =
                njCos(angle = (Angle)((phase + degrees) * ANGLE_PER_DEGREE));
            rise = 0.8F;
            sine = njSin(angle);
            big_icicle_puff.color.a = PUFF_ALPHA();
            pos.x = center.x + (radius * cosine);
            pos.y = center.y;
            pos.z = center.z + (radius * sine);
            if (index & 1) {
                velocity.x = speed * cosine * 10.0F;
                velocity.y = speed * rise * 16.0F;
                velocity.z = speed * sine * 10.0F;
            } else {
                velocity.x = speed * cosine * 12.0F;
                velocity.y = speed * rise * 19.0F;
                velocity.z = speed * sine * 12.0F;
            }
            ground = landing + (rand() * (30.0F * RAND_SCALE));
            icicle_puff_create(&big_icicle_puff, &velocity, &pos, ground);
        }
        /* @hack redundant: a never-run use of degrees, which keeps work in R14
         * and rand's address in R13 (0x0c9099e0, 0x0c9099ee); without it, the
         * two trade and 12 of 211 words differ */
        if (0) {
            velocity.x = degrees;
        }
    }
}

/* the ring small_icicle_ring makes, but of landing_puff's puffs, which fly
 * faster than big_icicle_ring's */
static void landing_ring(NJS_POINT3 *point, struct task_work *work,
                         Float radius, int count, Float speed)
{
    int degrees;
    int index;
    int step = 360 / count;
    int phase = (int)(rand() * (360.0F * RAND_SCALE));
    struct big_icicle *icicle = BIG_ICICLE(work)->icicle;
    Float landing = icicle->landing;
    NJS_POINT3 pos;
    NJS_POINT3 center;
    NJS_VECTOR velocity;
    Float cosine;
    Float rise;
    Float sine;
    Float ground;
    Angle angle;

    njPushMatrix(&_unit_matrix_);
    /* @hack type: plain 0s for the matrices: with NULL in both calls, index
     * takes R5 for R11 (0x0c909c88) and 148 of 205 words differ */
    njTranslateV(0, &work->pos);
    TURN(njRotateY, work->ang.y);
    njCalcPoint(0, point, &center);
    njPopMatrix(1);
    /* @hack spelling: index started from origin, a 0 in a scope of its own,
     * which starts degrees and index from R4 (0x0c909c82 to 0x0c909c88) and
     * leaves step and landing at sp+8 and sp+12 (0x0c909c30, 0x0c909c48);
     * from 0, the two trade and 68 of 205 words differ */
    {
        int origin = 0;

        for (degrees = 0, index = origin; degrees < 360;
             degrees += step, ++index) {
            /* @hack spelling: angle set inside njCos's argument, which keeps
             * the frame at 60 bytes (0x0c909c18); set before the call, it takes
             * 56 and 100 of 205 words differ */
            cosine =
                njCos(angle = (Angle)((phase + degrees) * ANGLE_PER_DEGREE));
            rise = 0.8F;
            sine = njSin(angle);
            pos.x = center.x + (radius * cosine);
            pos.y = center.y;
            pos.z = center.z + (radius * sine);
            if (index & 1) {
                velocity.x = speed * cosine * 17.0F;
                velocity.y = speed * rise * 20.0F;
                velocity.z = speed * sine * 17.0F;
            } else {
                velocity.x = speed * cosine * 21.0F;
                velocity.y = speed * rise * 30.0F;
                velocity.z = speed * sine * 21.0F;
            }
            landing_puff.color.a = PUFF_ALPHA();
            ground = landing + (rand() * (30.0F * RAND_SCALE));
            icicle_puff_create(&landing_puff, &velocity, &pos, ground);
        }
        /* @hack redundant: a never-run use of degrees, which keeps work in R14
         * and rand's address in R13 (0x0c909c1c, 0x0c909c2c); without it, the
         * two trade and 12 of 205 words differ */
        if (0) {
            velocity.x = degrees;
        }
    }
}

void big_icicle_step(struct task *task)
{
    if (!placement_try_leave_by(task, BIG_ICICLE_RANGE)) {
        if (big_icicle_first == NULL) {
            big_icicle_first = task;
        }
        big_icicle_start(task);
    }
}

/* hang the big icicle and the small ones. scl.y sets where the big icicle
 * rests, and scl.z where the small ones land, measured from that rest. the
 * set editor labels the two "Turara Height Offset" and "Ice Height Offset" */
static void big_icicle_hang(struct big_icicle *icicle, struct task_work *work)
{
    int i;
    Float rest = work->scl.y - BIG_ICICLE_REST;
    Float drop;
    Float speed;

    for (i = 0; i < ICICLES; ++i) {
        icicle->delays[i] = i == 0 ? 0 : rand() % (100 + (i * 10));
        icicle->landings[i] = rest + (rand() * (20.0F * RAND_SCALE)) +
                              work->scl.z - SMALL_ICICLE_LANDING;
        icicle->speeds[i] = 0.0F;
        icicle->turns[i] = RANDOM(-2.5F, 2.5F) * ANGLE_PER_DEGREE;
        icicle->wobbles[i] = 0;
        icicle->wobble_speeds[i] = 0;
        icicle->phases[i] = ICICLE_HANGING;
        icicle->yaws[i] = 0;
        icicle->pos[i].x = small_icicle_points[i].x - 8.0F;
        icicle->pos[i].y = small_icicle_points[i].y - 826.0F;
        icicle->pos[i].z = small_icicle_points[i].z;
    }
    icicle->mode = ICICLE_HANGING;
    icicle->height = -BIG_ICICLE_HANG;
    icicle->rest = rest;
    icicle->landing = rest;
    icicle->speed = 0.0F;
    icicle->countdown = rand() % 10;
    /* 1.0F - powf(0.5F, 500.0F) is 1, since the power is 0 in a Float.
     * landing then rises by speed, so the fall ends that far above the rest */
    /* @hack redundant: the loop computes the same speed twice, as the image
     * does (0x0c90a450 to 0x0c90a4b2); computed once, big_icicle.c's code
     * runs 96 bytes short and 104 of 712 words differ */
    for (i = 0; i < 2; ++i) {
        drop = icicle->landing - icicle->height;
        speed = njSqrt(-(drop * (2.0F * ICICLE_GRAVITY))) *
                (1.0F - powf(0.5F, 500.0F)) * 2.0F;
    }
    icicle->landing += speed;
    icicle->shake_speed = 0.0F;
    icicle->shake = 0.0F;
    icicle->active = TRUE;
    icicle->enabled = FALSE;
}

/* put the big icicle and the small ones down where they land, for the set
 * editor */
static void big_icicle_put_down(struct big_icicle *icicle,
                                struct task_work *work)
{
    int i;
    Float rest = work->scl.y - BIG_ICICLE_REST;

    for (i = 0; i < ICICLES; ++i) {
        icicle->delays[i] = 0;
        icicle->landings[i] = rest + (rand() * (20.0F * RAND_SCALE)) +
                              work->scl.z - SMALL_ICICLE_LANDING;
        icicle->speeds[i] = 0.0F;
        icicle->turns[i] = 0;
        icicle->phases[i] = ICICLE_DOWN;
        icicle->yaws[i] = 0;
        icicle->pos[i].x = small_icicle_points[i].x - 8.0F;
        icicle->pos[i].y = icicle->landings[i];
        icicle->pos[i].z = small_icicle_points[i].z;
    }
    icicle->mode = ICICLE_DOWN;
    icicle->height = rest;
    icicle->landing = rest;
    icicle->rest = rest;
    icicle->speed = 0.0F;
    icicle->countdown = 0;
    icicle->active = TRUE;
    icicle->enabled = FALSE;
}

static void big_icicle_start(struct task *task)
{
    struct task_work *work = task->work;
    struct big_icicle *icicle;
    NJS_OBJECT *object;

    task->draw = big_icicle_draw;
    task->step = big_icicle_run;
    task->destructor = big_icicle_destroy;
    icicle = BIG_ICICLE(work)->icicle = heap_alloc(sizeof(struct big_icicle));
    big_icicle_hang(icicle, work);
    work->timer = 0;
    work->submode = 0;
    work->ang.x = 0;
    work->ang.z = 0;
    if (!placement_editing(task)) {
        BIG_ICICLE(work)->object = object = world_object_alloc();
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = 1.0F;
        object->ang[0] = 0;
        object->ang[1] = work->ang.y;
        object->ang[2] = 0;
        object->pos[0] = work->pos.x + 8.0F;
        object->pos[1] = work->pos.y + icicle->height;
        object->pos[2] = work->pos.z;
        /* @hack spelling: the model reached through big_icicle_object's
         * address, which loads the object's pool word and the model at
         * @(4,R3) (0x0c90aacc, 0x0c90aa26); as big_icicle_object.model, the
         * pool holds the member's address and 8 of 119 words differ */
        object->model = (&big_icicle_object)->model;
        work->flags |= WORLD_TESTED;
        world_add(WORLD_LARGE | WORLD_MOVES | WORLD_SOLID, task, object);
        collision_init(task, big_icicle_shapes, COLLISION_OBJECT, 1, 4);
    }
    memcpy(&icicle->glint_object, &big_icicle_glint_object, sizeof(NJS_OBJECT));
    memcpy(&icicle->glint_model, big_icicle_glint_object.model,
           sizeof(NJS_MODEL));
    memcpy(&icicle->glint_material, big_icicle_glint_object.model->mats,
           sizeof(NJS_MATERIAL));
    icicle->glint_object.model = &icicle->glint_model;
    icicle->glint_model.mats = &icicle->glint_material;
}

static void big_icicle_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (big_icicle_first == task) {
        big_icicle_first = NULL;
    }
    if (BIG_ICICLE(work)->object != NULL) {
        world_remove(task, BIG_ICICLE(work)->object);
        world_object_free(BIG_ICICLE(work)->object);
    }
    heap_free(BIG_ICICLE(work)->icicle);
}

/* move the world object and shapes 1 to 3 with the big icicle, and register
 * the shapes. until the big icicle is down, shape 1 is on, and the collision
 * world doesn't test the object. once it is down, shape 1 is off. the world
 * then tests the object while a player is near */
static void big_icicle_update_collision(struct task *task)
{
    struct task_work *work = task->work;
    struct big_icicle *icicle = BIG_ICICLE(work)->icicle;
    NJS_OBJECT *object;
    NJS_POINT3 pos;
    Float drop;

    if (placement_editing(task)) {
        return;
    }
    if (BIG_ICICLE(work)->object != NULL) {
        object = BIG_ICICLE(work)->object;
        pos.x = work->pos.x;
        pos.y = work->pos.y + icicle->height;
        pos.z = work->pos.z;
        object->pos[0] = pos.x;
        object->pos[1] = pos.y;
        object->pos[2] = pos.z;
        object->ang[1] = work->ang.y;
        if (player_in_range(&pos, object->model->r + PLAYER_NEAR)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
    /* @bug memory: a big icicle the set editor held as it started has
     * no collision record, and once the editor lets it go, this reads the
     * record's shapes through null, at 0x0c90abf4 and 0x0c90ac0a, and writes
     * through what it finds there, at 0x0c90abfc and 0x0c90ac12 */
    if (icicle->active) {
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        work->flags &= ~WORLD_TESTED;
    } else {
        work->collision->shapes[1].flags |= SHAPE_OFF;
    }
    drop = icicle->height + BIG_ICICLE_HANG;
    work->collision->shapes[1].center.y = big_icicle_shapes[1].center.y + drop;
    work->collision->shapes[2].center.y = big_icicle_shapes[2].center.y + drop;
    work->collision->shapes[3].center.y = big_icicle_shapes[3].center.y + drop;
    collision_set_radius(work->collision);
    collision_register(work);
}

static void big_icicle_run(struct task *task)
{
    struct task_work *work = task->work;
    struct big_icicle *icicle = BIG_ICICLE(work)->icicle;
    int i;
    int j;
    int landed;
    Float landing;
    Float shake;
    Float sway;
    Float ground;
    Angle wobble;
    NJS_VECTOR offset;
    NJS_POINT3 pos;
    NJS_VECTOR velocity;

    if (placement_try_leave_by(task, BIG_ICICLE_RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        big_icicle_put_down(icicle, work);
    }
    if (++TASK_TIMER(work) == 0) {
        --TASK_TIMER(work);
    }
    /* submode counts the glint's frames, 0 to 9, one a frame */
    if (!frame_step) {
        if (++work->submode >= 10) {
            work->submode = 0;
        }
    }
    big_icicle_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y),
                 "-> Turara Height Offset");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> Ice Height Offset");
        return;
    }
    if (!icicle->enabled) {
        if (panel_get_left() == 3) {
            icicle->enabled = TRUE;
        }
        big_icicle_update_collision(task);
    } else {
        if (icicle->active) {
            landed = 0;
            for (i = 0; i < ICICLES; ++i) {
                switch (icicle->phases[i]) {
                /* @hack spelling: ICICLE_FALLING's case ahead of
                 * ICICLE_HANGING's, which writes its body first (0x0c90ae20,
                 * then 0x0c90b080); in order, 1060 of 1176 words differ */
                case ICICLE_FALLING:
                    icicle->pos[i].y += icicle->speeds[i];
                    icicle->yaws[i] += icicle->turns[i];
                    if (icicle->pos[i].y < icicle->landings[i]) {
                        small_icicle_ring(&icicle->pos[i], work, 15.0F, 3,
                                          1.0F);
                        sound_play(SE_ICICLE_LAND, 0, 0, 0);
                        icicle->shake_speed +=
                            0.5F + (rand() * (0.2F * RAND_SCALE));
                        icicle->delays[i] = 0;
                        icicle->wobble_speeds[i] = rand() * (1.0F / 12.0F);
                        icicle->phases[i] = ICICLE_LANDING;
                    } else if (rand() % 30 == 0) {
                        landing = icicle->landing;
                        pos.x = work->pos.x + icicle->pos[i].x;
                        pos.y = work->pos.y + icicle->pos[i].y;
                        pos.z = work->pos.z + icicle->pos[i].z;
                        velocity.x = 0.0F;
                        velocity.y = icicle->speeds[i] * 0.9F;
                        velocity.z = 0.0F;
                        small_icicle_puff.color.a = PUFF_ALPHA();
                        ground = landing + (rand() * (30.0F * RAND_SCALE));
                        icicle_puff_create(&small_icicle_puff, &velocity, &pos,
                                           ground);
                    }
                    icicle->speeds[i] += -ICICLE_GRAVITY;
                    if (icicle->speeds[i] < -2000.0F) {
                        icicle->speeds[i] = -2000.0F;
                    }
                    break;
                case ICICLE_HANGING:
                    if (icicle->delays[i]-- == 0) {
                        landing = icicle->landing;
                        pos.x = work->pos.x + icicle->pos[i].x;
                        pos.y = work->pos.y + icicle->pos[i].y;
                        pos.z = work->pos.z + icicle->pos[i].z;
                        for (j = 0; j < 2; ++j) {
                            velocity.x = RANDOM(-2.5F, 2.5F);
                            velocity.y = 0.0F;
                            velocity.z = RANDOM(-2.5F, 2.5F);
                            pos.y -= RANDOM(25.0F, 30.0F);
                            small_icicle_puff.color.a = PUFF_ALPHA();
                            ground = landing + (rand() * (30.0F * RAND_SCALE));
                            icicle_puff_create(&small_icicle_puff, &velocity,
                                               &pos, ground);
                        }
                        icicle->phases[i] = ICICLE_FALLING;
                    }
                    break;
                case ICICLE_LANDING:
                    ++icicle->delays[i];
                    icicle->speeds[i] *= 0.4F;
                    icicle->pos[i].y += icicle->speeds[i];
                    icicle->turns[i] = icicle->turns[i] * 0.3F;
                    icicle->yaws[i] += icicle->turns[i];
                    wobble = icicle->wobbles[i] + icicle->wobble_speeds[i];
                    icicle->wobble_speeds[i] += (Angle)(-wobble * 0.7F);
                    icicle->wobbles[i] = wobble;
                    icicle->wobble_speeds[i] = icicle->wobble_speeds[i] * 0.5F;
                    if (_builtin_fabsf((Float)icicle->wobble_speeds[i]) <
                        36.0F) {
                        icicle->phases[i] = ICICLE_DOWN;
                    }
                    ++landed;
                    break;
                case ICICLE_DOWN:
                    /* @hack spelling: the increment written out, which forms
                     * the element's address in R4 (0x0c90b342 to 0x0c90b34e);
                     * as ++icicle->delays[i], R1 takes it and 5 of 1176 words
                     * differ */
                    icicle->delays[i] = icicle->delays[i] + 1;
                    ++landed;
                    break;
                }
            }
            if (landed == ICICLES) {
                switch (icicle->mode) {
                case ICICLE_HANGING:
                    if (icicle->countdown > 0) {
                        --icicle->countdown;
                    } else {
                        landing = icicle->landing;
                        pos.x = work->pos.x;
                        pos.y = work->pos.y + icicle->height;
                        pos.z = work->pos.z;
                        for (i = 0; i < 6; ++i) {
                            velocity.x = 0.0F;
                            velocity.y = 0.0F;
                            velocity.z = 0.0F;
                            pos.y -= 27.0F;
                            big_icicle_puff.color.a = PUFF_ALPHA();
                            ground = landing + (rand() * (30.0F * RAND_SCALE));
                            icicle_puff_create(&big_icicle_puff, &velocity,
                                               &pos, ground);
                        }
                        icicle->mode = ICICLE_FALLING;
                    }
                    break;
                case ICICLE_FALLING:
                    icicle->height += icicle->speed;
                    icicle->speed += -ICICLE_GRAVITY;
                    if (icicle->height < icicle->landing) {
                        icicle->shake_speed += 5.0F;
                        icicle->countdown = 0;
                        icicle->mode = ICICLE_LANDING;
                    } else if (TASK_TIMER(work) % (12 + (rand() % 7)) == 0) {
                        landing = icicle->landing;
                        pos.x = work->pos.x;
                        pos.y = work->pos.y + icicle->height;
                        pos.z = work->pos.z;
                        velocity.x = RANDOM(-7.5F, 7.5F);
                        /* @bug memory: i is ICICLES here, as
                         * the small icicles' loop leaves it in R12
                         * (0x0c90b35a), and speeds[i] is the word past the
                         * array, turns[0], an Angle, which the image reads as
                         * a Float, at 0x0c90b59a, 0x0c90b5a4 */
                        velocity.y = icicle->speeds[i] * 0.9F;
                        velocity.z = RANDOM(-7.5F, 7.5F);
                        big_icicle_puff.color.a = PUFF_ALPHA();
                        ground = landing + (rand() * (30.0F * RAND_SCALE));
                        icicle_puff_create(&big_icicle_puff, &velocity, &pos,
                                           ground);
                    }
                    break;
                case ICICLE_LANDING:
                    ++icicle->countdown;
                    icicle->speed *= 0.4F;
                    icicle->height += icicle->speed;
                    if (icicle->countdown == 1) {
                        pos.x = 0.0F;
                        pos.y = icicle->landing - 50.4F;
                        pos.z = 0.0F;
                        landing_ring(&pos, work, 90.0F, 8, 1.0F);
                    }
                    if (icicle->speed < 0.1F || icicle->height < icicle->rest) {
                        icicle->height = icicle->rest;
                        icicle->mode = ICICLE_DOWN;
                    }
                    break;
                case ICICLE_DOWN:
                    ++icicle->countdown;
                    icicle->active = FALSE;
                    work->timer = 0;
                }
            }
        }
        if (icicle->active || TASK_TIMER(work) < SHAKE_STEPS) {
            shake = icicle->shake_speed + icicle->shake;
            sway = _builtin_fabsf(icicle->shake_speed) * 0.3F;
            icicle->shake_turn += 2002;
            offset.x = njSin(icicle->shake_turn) * sway;
            offset.z = njCos(icicle->shake_turn) * sway;
            offset.y = -icicle->shake;
            if (shake < 0.0F) {
                icicle->shake_speed = -(icicle->shake_speed * 0.6F);
                icicle->shake = 0.0F;
                offset.y = 0.0F;
            } else {
                icicle->shake = shake;
                icicle->shake_speed -= icicle->shake * 0.75F;
                icicle->shake_speed *= 0.7F;
            }
            if (icicle->shake > 0.1F ||
                _builtin_fabsf(icicle->shake_speed) > 0.02F) {
                sound_play_for(SE_RUMBLE, (int)work, 1, 0, 180);
            }
            camera_add_offset(&offset);
        }
        big_icicle_update_collision(task);
    }
}

static void big_icicle_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct big_icicle *icicle = BIG_ICICLE(work)->icicle;
    int i;
    Angle ang;
    Sint8 frame;
    NJS_OBJECT *glint;
    NJS_MATERIAL *material;

    if (placement_try_leave_by(task, BIG_ICICLE_RANGE)) {
        return;
    }
    if (frame_step) {
        return;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    for (i = 0; i < ICICLES; ++i) {
        if (icicle->phases[i] == ICICLE_HANGING ||
            icicle->phases[i] == ICICLE_FALLING) {
            njPushMatrix(NULL);
            njTranslate(NULL, icicle->pos[i].x, icicle->pos[i].y,
                        icicle->pos[i].z);
            TURN_VIA(njRotateX, icicle->wobbles[i] + 0x8000, ang);
            TURN_VIA(njRotateY, icicle->yaws[i], ang);
            draw_tree(&small_icicle_object);
            njPopMatrix(1);
        }
    }
    njPushMatrix(NULL);
    draw_tree(&dat_0ca7debc);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, icicle->height, 0.0F);
    draw_tree(&big_icicle_object);
    njPopMatrix(1);
    if (icicle->active) {
        glint = &icicle->glint_object;
        material = &icicle->glint_material;
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, -203.80005F, 0.0F);
        njSetTexture(&hikari_texlist);
        /* the glint's texture runs 0 to 4 and back over submode's 10
         * frames */
        frame = work->submode;
        if (frame >= 5) {
            frame = 9 - frame;
        }
        /* @hack redundant: a switch that goes on either way, which writes the
         * image's CMP/EQ #5 of frame in njFogDisable's delay slot
         * (0x0c90bc06); without it, 33 of 472 words differ */
        switch (frame) {
        case 5:
            break;
        default:
            break;
        }
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        material->attr_texId = frame;
        material->attrflags =
            (material->attrflags & ~(NJD_SA_MASK | NJD_DA_MASK)) | NJD_SA_SRC |
            NJD_DA_ONE | NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA;
        njFogDisable();
        draw_tree(glint);
        njFogEnable();
        njPopMatrix(1);
    }
    njPopMatrix(1);
}

/* a puff from snow_pool. it drifts, slows and runs through its frames. it
 * runs through them at five times its rate while it falls below its
 * ground */
static void icicle_puff_step(struct particle *particle,
                             struct particle_pool *pool)
{
    struct dust_params *params = ICICLE_PUFF(particle)->params;
    Float progress;
    Float drag;
    Float alpha;

    particle->frame = particle->time;
    progress = particle->time / (Sint16)pool->anim->frames;
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = pool->anim->frames - 1;
        /* @bug logic: the fade reads color.a as a byte from 0 to 255,
         * but it is a Float from 0.4 to 0.7, which converts to 0 (0x0c90bcb6
         * to 0x0c90bcbc). a puff ends with its last frame, and the fade below
         * never runs */
        if ((Uint8)particle->color.a <= 32) {
            particle_kill(particle);
            return;
        }
        alpha = (Uint32)particle->color.a & 0xff;
        particle->color.a = (Sint8)(alpha * 0.8F);
        return;
    }
    drag = params->drag;
    particle->speed.x += -(particle->speed.x * drag);
    particle->speed.y += params->lift;
    particle->speed.y *= 1.0F - drag;
    particle->speed.z += -(particle->speed.z * drag);
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    particle->scale += params->growth;
    if (particle->pos.y < ICICLE_PUFF(particle)->ground &&
        particle->speed.y < 0.0F) {
        particle->time += 5.0F * params->rate;
    } else {
        particle->time += params->rate * (1.0F - (progress * 0.6F));
    }
    if (particle->ang & 1) {
        particle->ang += 80;
    } else {
        particle->ang -= 80;
    }
}

/* a puff from snow_pool at pos, moving at speed, with params, which it
 * keeps. its own ground is ground plus up to 10. it makes none when no
 * particle is free */
static void icicle_puff_create(struct dust_params *params, NJS_VECTOR *speed,
                               NJS_POINT3 *pos, Float ground)
{
    struct particle_pool *pool = snow_pool.pool;
    struct particle *particle = particle_alloc(pool, icicle_puff_step);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = params->scale;
        particle->time = RANDOM(1.0F, 3.0F);
        particle->ang = 0x2000 - (int)(rand() * (4096.0F * RAND_SCALE));
        particle->color = params->color;
        ICICLE_PUFF(particle)->params = params;
        ICICLE_PUFF(particle)->ground =
            ground + (rand() * (10.0F * RAND_SCALE));
    }
}

/* put model in the collision world as the lid's object, where the lid is.
 * the object's size is the smallest that takes the model's reach. the reach
 * is its center's distance from its origin, plus its radius, plus
 * PLAYER_NEAR */
static void futa_world_add(struct task *task, NJS_MODEL *model)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Sint32 flags;
    Float reach;

    FUTA(work)->object = object = world_object_alloc();
    object->evalflags = 0;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = model;
    object->child = NULL;
    object->sibling = NULL;
    work->flags &= ~WORLD_TESTED;
    flags = WORLD_HUGE | WORLD_MOVES | WORLD_GRIP_LEAST | WORLD_SOLID;
    reach = njScalor(&model->center);
    reach += model->r;
    reach += PLAYER_NEAR;
    if (reach <= WORLD_SMALL_REACH) {
        flags = WORLD_SMALL | WORLD_MOVES | WORLD_GRIP_LEAST | WORLD_SOLID;
    } else if (reach <= WORLD_MEDIUM_REACH) {
        flags = WORLD_MEDIUM | WORLD_MOVES | WORLD_GRIP_LEAST | WORLD_SOLID;
    } else if (reach <= WORLD_LARGE_REACH) {
        flags = WORLD_LARGE | WORLD_MOVES | WORLD_GRIP_LEAST | WORLD_SOLID;
    }
    world_add(flags, task, object);
}

static void futa_world_remove(struct task *task)
{
    struct task_work *work = task->work;

    if (FUTA(work)->object != NULL) {
        world_remove(task, FUTA(work)->object);
        world_object_free(FUTA(work)->object);
    }
}

/* move the lid's world object to the lid, with model. the collision world
 * tests it only while a player is near its model's center. a null model
 * leaves it untested */
static void futa_world_move(struct task *task, NJS_MODEL *model)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = FUTA(work)->object;
    NJS_POINT3 center;
    Angle ang;

    if (model != NULL && object != NULL) {
        object->model = model;
        center = object->model->center;
        ang = work->ang.y;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, ang);
        njCalcPoint(NULL, &center, &center);
        njPopMatrix(1);
        /* @hack type: object->pos, a Float[3], stored in place as an
         * NJS_POINT3; without it, as three element stores, 22 of 83 words
         * differ */
        *(NJS_POINT3 *)object->pos = work->pos;
        object->ang[1] = ang;
        if (player_in_range(&center, object->model->r + PLAYER_NEAR)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    } else {
        if (object != NULL) {
            object->model = model;
        }
        work->flags &= ~WORLD_TESTED;
    }
}

/* spin's low 16 bits, as a signed angle, times scale */
static Angle futa_spin_scale(Angle spin, Float scale)
{
    /* @hack type: the low half read through a union with the word, as the
     * image stores R4 to the frame and loads the half back with MOV.W
     * (0x0c90c106, 0x0c90c10a); as (Sint16)spin, the function runs 8 bytes
     * short and 12 of 12 words differ */
    union {
        Angle word;
        Sint16 low;
    } value, *view;

    value.word = spin;
    /* @hack spelling: the half read through a pointer to the union, which keeps
     * its address in a register; as value.low, 3 of 12 words differ */
    view = &value;
    return (Angle)view->low * scale;
}

/* 1 for pos within height of center along y and within radius across x and
 * z, else 0 */
static int futa_in_cylinder(NJS_POINT3 *pos, NJS_POINT3 *center, Float radius,
                            Float height)
{
    Float x = pos->x - center->x;
    Float y = pos->y - center->y;
    Float z = pos->z - center->z;
    Float across;

    if (_builtin_fabsf(y) > height) {
        return 0;
    }
    across = njSqrt((x * x) + (z * z));
    if (across <= radius) {
        return 1;
    }
    return 0;
}

/* whether the lid was struck hard enough to break, 1 or 0. a large carried
 * thing strikes it when the thing's second shape touches the lid. the thing
 * must move faster than 0.1 a step. Big, as player 0, strikes it from
 * within the lid's cylinder. he strikes it falling faster than 1.5 while
 * above the lid's height less 3. he strikes it rising faster than 1.5 while
 * at or below that height. a lid the set editor holds is never struck */
static int futa_struck(struct task *task)
{
    struct task_work *work = task->work;

    /* @bug memory: a lid the set editor held as it started has no
     * collision record, and once the editor lets it go, this reads the
     * record's hit shape through null, at 0x0c90c1a0 */
    if (!placement_editing(task) && work->collision->hit_shape == 1 &&
        (work->collision->flags & COLLISION_HIT) &&
        work->collision->hit->owner->work->id == CARRY_KIND_LARGE &&
        work->collision->hit->owner->motion != NULL &&
        njScalor(&work->collision->hit->owner->motion->speed) > 0.1F) {
        /* @hack spelling: the carried thing's success shares the rising
         * player's return, which keeps placement_editing's and
         * player_get_character's addresses in R2 (0x0c90c1e2, 0x0c90c1ec); as
         * return 1, R3 takes them and 4 of 101 words differ */
        goto struck;
    } else if (!placement_editing(task) &&
               /* @hack spelling: a select in place of &&, which loads the
                * record and its shapes through R3 and R2 for the cylinder's
                * arguments (0x0c90c1f6, 0x0c90c1fe); as &&, the two trade and
                * 9 of 101 words differ */
               (player_get_character(0) != CHARACTER_BIG
                    ? 0
                    : futa_in_cylinder(&work->pos, &player_work[0]->pos,
                                       work->collision->shapes->radius,
                                       work->collision->shapes->size_y))) {
        /* @hack redundant: a jump to the next line, which loads player_work's
         * address and player 0's work through R3 and R2 (0x0c90c212,
         * 0x0c90c21a); without it, the two trade and 5 of 101 words differ */
        goto crossing;
    crossing:
        if (player_work[0]->pos.y + 3.0F > work->pos.y) {
            if (player_motion[0]->speed.y < -1.5F) {
                return 1;
            }
        } else if (player_motion[0]->speed.y > 1.5F) {
        struck:
            return 1;
        }
    }
    return 0;
}

void futa_m_step(struct task *task)
{
    futa_init(task, &futa_m_object, &futa_m_broken_object, 13);
}

void futa_m2_step(struct task *task)
{
    futa_init(task, &futa_m_object, &futa_m2_broken_object, 13);
}

void futa_l_step(struct task *task)
{
    futa_init(task, &futa_l_object, &futa_l_broken_object, 17);
}

static void futa_init(struct task *task, NJS_OBJECT *whole, NJS_OBJECT *broken,
                      int count)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, FUTA_RANGE)) {
        /* heap_calloc takes the count first, so the two are swapped. it
         * rounds the size of 1 up to a word. it gives sizeof(struct futa)
         * cleared words: 64 bytes, four times the record */
        FUTA(work)->futa = heap_calloc(sizeof(struct futa), 1);
        FUTA(work)->futa->count = count;
        FUTA(work)->futa->whole = whole;
        FUTA(work)->futa->broken = broken;
        if (!placement_editing(task)) {
            futa_world_add(task, whole->model);
            collision_init(task, &futa_shape, COLLISION_OBJECT_MUTUAL, 1, 1);
            work->collision->shapes->radius = whole->model->r * 0.6F;
            collision_set_radius(work->collision);
        }
        task->draw = futa_draw;
        task->step = futa_run;
        task->destructor = futa_destroy;
        work->mode = FUTA_WHOLE;
        work->timer = 0;
    }
}

static void futa_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (FUTA(work)->futa != NULL) {
        heap_free(FUTA(work)->futa->pieces);
    }
    heap_free(FUTA(work)->futa);
    futa_world_remove(task);
}

static void futa_run(struct task *task)
{
    struct task_work *work = task->work;
    struct futa_piece *piece;
    NJS_OBJECT *object;
    NJS_VECTOR velocity;
    NJS_POINT3 pos;
    NJS_VECTOR normal;
    Float spread;
    Float push;
    Float strength;
    Float radius;
    int count;
    int fallen;
    int i;
    Angle ang;

    if (placement_try_leave_by(task, FUTA_RANGE)) {
        return;
    }
    futa_draw(task);
    switch (work->mode) {
    /* @hack spelling: FUTA_BREAKING's case ahead of FUTA_WHOLE's, which writes
     * its body first (0x0c90c3d6, then 0x0c90c560); in order, 526 of 556
     * words differ */
    case FUTA_BREAKING:
        piece = FUTA(work)->futa->pieces;
        if (piece != NULL) {
            fallen = 0;
            count = FUTA(work)->futa->count;
            while (count--) {
                /* a piece spreads only as it sinks below the lid, fully from
                 * 20 below */
                if (piece->pos.y < -20.0F) {
                    spread = 1.0F;
                } else if (piece->pos.y > 0.0F) {
                    spread = 0.0F;
                } else {
                    spread = -(piece->pos.y * 0.05F);
                }
                /* @hack redundant: never-run stores of 0, which give the 0 to
                 * FR15 (0x0c90c410) and leave -20 to a load for each piece
                 * (0x0c90c3f6); without them, 60 of 556 words differ */
                if (0) {
                    piece->pos.x = 0.0F;
                    piece->pos.y = 0.0F;
                    piece->pos.z = 0.0F;
                    piece->speed.y = 0.0F;
                }
                piece->pos.x += spread * piece->speed.x;
                piece->pos.y += piece->speed.y;
                piece->pos.z += spread * piece->speed.z;
                piece->speed.y -= 0.08F;
                piece->speed.x *= 0.91F;
                piece->speed.y *= 0.91F;
                piece->speed.z *= 0.91F;
                piece->ang.x += piece->spin.x;
                piece->ang.y += piece->spin.y;
                piece->ang.z += piece->spin.z;
                piece->spin.x = futa_spin_scale(piece->spin.x, 0.91F);
                piece->spin.y = futa_spin_scale(piece->spin.y, 0.91F);
                piece->spin.z = futa_spin_scale(piece->spin.z, 0.91F);
                if (piece->pos.y < -200.0F) {
                    ++fallen;
                }
                ++piece;
            }
            if (fallen >= FUTA(work)->futa->count) {
                heap_free(FUTA(work)->futa->pieces);
                FUTA(work)->futa->pieces = NULL;
                work->mode = FUTA_BROKEN;
            }
        }
        break;
    case FUTA_WHOLE:
        if (futa_struck(task)) {
            object = FUTA(work)->futa->broken;
            if (collision_get_hit_normal(work, &normal)) {
                if (FUTA(work)->futa->pieces == NULL) {
                    /* heap_calloc takes the count first, so the two are
                     * swapped. it gives sizeof(struct futa_piece) records of
                     * count bytes, each rounded up to a word. they hold
                     * count pieces and more */
                    FUTA(work)->futa->pieces = heap_calloc(
                        sizeof(struct futa_piece), FUTA(work)->futa->count);
                }
                piece = FUTA(work)->futa->pieces;
                work->mode = FUTA_BREAKING;
                strength = 0.3F;
                radius = object->model->r;
                object = object->child;
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN_VIA(njRotateY, work->ang.y, ang);
                for (i = 0; i < FUTA(work)->futa->count && object != NULL;
                     ++i, object = object->sibling, ++piece) {
                    velocity.x =
                        -normal.x + object->model->center.x + object->pos[0];
                    velocity.y = 0.0F;
                    velocity.z =
                        -normal.z + object->model->center.z + object->pos[2];
                    spread = njScalor(&velocity) / radius;
                    push = spread * 0.1F;
                    /* @hack redundant: a never-run test of push, which with the
                     * one below keeps push in FR12 (0x0c90c64a) and the frame
                     * at 48 bytes (0x0c90c39e); without it, push goes to the
                     * frame and 262 of 556 words differ */
                    if (0) {
                        if (push) {
                        }
                    }
                    piece->speed.x = velocity.x * push;
                    piece->speed.y =
                        strength * (-0.1F + (rand() * (4.0F * RAND_SCALE)));
                    piece->speed.z = velocity.z * push;
                    piece->spin.x = RANDOM(-1.0F, 1.0F) * ANGLE_PER_DEGREE;
                    piece->spin.y = RANDOM(-1.0F, 1.0F) * ANGLE_PER_DEGREE;
                    piece->spin.z = RANDOM(-1.0F, 1.0F) * ANGLE_PER_DEGREE;
                    piece->pos.x = 0.0F;
                    piece->pos.y = 0.0F;
                    piece->pos.z = 0.0F;
                    piece->ang.x = 0;
                    piece->ang.y = 0;
                    piece->ang.z = 0;
                    pos.x = object->model->center.x + object->pos[0];
                    pos.y = object->model->center.y + object->pos[1];
                    pos.z = object->model->center.z + object->pos[2];
                    velocity.x *=
                        push * (0.9F + (rand() * (0.2F * RAND_SCALE)));
                    velocity.y = (0.9F + (rand() * (0.2F * RAND_SCALE))) * 0.1F;
                    velocity.z *=
                        push * (0.9F + (rand() * (0.2F * RAND_SCALE)));
                    /* @hack redundant: a never-run test of push, which keeps it
                     * in FR12 through the last product (0x0c90c750); without
                     * it, push goes to the frame and 262 of 556 words differ */
                    if (1) {
                    } else if (push) {
                    } else {
                    }
                    njCalcVector(NULL, &velocity, &velocity);
                    njCalcPoint(NULL, &pos, &pos);
                    splash_create(&pos, &velocity, 1.0F);
                }
                /* @hack redundant: never-run stores of the noise's and the
                 * spins' scales, which keep 2 / 32768 in FR13 and 0.2 / 32768
                 * in FR14 (0x0c90c5e6, 0x0c90c5ee); without them, 15 of 556
                 * words differ */
                if (0) {
                    velocity.x = 0.2F * RAND_SCALE;
                    velocity.y = 0.2F * RAND_SCALE;
                    velocity.x = 2.0F * RAND_SCALE;
                    velocity.y = 2.0F * RAND_SCALE;
                }
                njPopMatrix(1);
                pos.x = work->pos.x;
                pos.y = work->pos.y;
                pos.z = work->pos.z;
                velocity.x = 0.0F;
                velocity.y = (0.6F + (rand() * (0.2F * RAND_SCALE))) * 0.1F;
                velocity.z = 0.0F;
                sound_play(SE_FUTA_BREAK, 0, 0, 0);
                splash_create(&pos, &velocity, 1.2F);
            }
        }
        break;
    case FUTA_BROKEN:
        break;
    /* @hack redundant: a default that only leaves, which puts
     * placement_editing's address in R3 (0x0c90c858); without it, R2 takes it
     * and 4 of 556 words differ */
    default:
        break;
    }
    if (!placement_editing(task)) {
        if (work->mode != FUTA_WHOLE && !futa_struck(task)) {
            ++work->timer;
        }
        /* @bug memory: a lid the set editor held as it started has no
         * collision record, and once the editor lets it go, this reads the
         * record's shapes through null, at 0x0c90c87a, and writes through
         * what it finds there, at 0x0c90c880 */
        work->collision->shapes->flags &= ~SHAPE_OFF;
        if (work->mode == FUTA_WHOLE) {
            futa_world_move(task, FUTA(work)->futa->whole->model);
        } else {
            futa_world_move(task, FUTA(work)->futa->broken->model);
            /* timer stays 0 while the lid is whole. on the step the lid
             * breaks, timer is still below 10, so the shape stays on. from
             * the next step, timer is 10 or more, and the shape is off */
            if (TASK_TIMER(work) >= 10) {
                work->collision->shapes->flags |= SHAPE_OFF;
            }
            work->timer = 10;
        }
        collision_register(work);
    }
}

static void futa_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct futa_piece *piece;
    NJS_OBJECT *object;
    int i;
    Angle ang;

    if (placement_try_leave_by(task, FUTA_RANGE)) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                      FUTA(work)->futa->whole->model->r + VIEW_MARGIN)) {
        return;
    }
    texture_set_stage();
    switch (work->mode) {
    case FUTA_WHOLE:
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(FUTA(work)->futa->whole->model);
        njPopMatrix(1);
        break;
    case FUTA_BREAKING:
        object = FUTA(work)->futa->broken;
        piece = FUTA(work)->futa->pieces;
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(object->model);
        object = object->child;
        for (i = 0; i < FUTA(work)->futa->count && object != NULL;
             ++i, ++piece, object = object->sibling) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &piece->pos);
            TURN_VIA(njRotateZ, piece->ang.z, ang);
            TURN_VIA(njRotateX, piece->ang.x, ang);
            TURN_VIA(njRotateY, piece->ang.y, ang);
            draw_model(object->model);
            njPopMatrix(1);
        }
        njPopMatrix(1);
        break;
    case FUTA_BROKEN:
        object = FUTA(work)->futa->broken;
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(object->model);
        njPopMatrix(1);
        break;
    }
    /* @hack redundant: a never-run pop, which keeps njPushMatrix's address
     * in R9 and draw_model's in R10 (0x0c90c95c, 0x0c90c956); without it, the
     * two trade and 7 of 178 words differ */
    if (0) {
        njPopMatrix(1);
    }
}
