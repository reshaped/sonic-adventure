/* O FSAKU band between two posts that stretches and pulls a player back */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <string.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg10/elastic_fence.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the posts, with the band as their child. nothing plays the action, whose
 * object is the band at full stretch */
extern NJS_OBJECT elastic_fence_object;
extern NJS_ACTION elastic_fence_stretch_action;

/* the kind's bits: how the band pulls a player, and how it throws one off.
 * elastic_fence_set_kind sets FENCE_PULL_RUBBER and FENCE_THROW_NONE, but no
 * test reads them */
#define FENCE_PULL_RUBBER 0x1
/* pull in proportion to the square of the band's extra length. without it,
 * the band pulls in proportion to the extra length itself */
#define FENCE_PULL_STRING 0x2
#define FENCE_THROW_NONE 0x4
/* throw the player back the way it came, reflected in the band. without it,
 * the band throws nothing */
#define FENCE_THROW_SPRING 0x8
/* with FENCE_THROW_SPRING, throw along the heading masked with EIGHTH_BITS */
#define FENCE_THROW_EIGHTHS 0x10

/* a heading's 45-degree and 90-degree bits. masked with them, a heading is
 * 0, 45, 90 or 135 degrees */
#define EIGHTH_BITS (NJM_DEG_ANG(45) | NJM_DEG_ANG(90))

#define BAND_SCALE 2.7F /* the band model's scale along its length */
/* the band's half length and length at rest */
#define BAND_HALF (BAND_SCALE * 2.5F)
#define BAND_LENGTH (BAND_SCALE * 5.0F)
/* the points in the band's model, and in its model at full stretch */
#define BAND_POINTS 15
/* the slide moves the band's middle points: those from MIDDLE_LOW to
 * MIDDLE_HIGH along it, in its model's units. the two ends, at the posts,
 * stay */
#define MIDDLE_LOW 1.57F
#define MIDDLE_HIGH 3.99F
/* a third, as the nearest float. SHC folds 1.0F / 3.0F to the float below */
#define THIRD 0.33333335F

/* the posts' two shapes, then the band's */
#define SHAPES 3
#define BAND_SHAPE 2

/* the share of a player's radius at which the band touches the player */
#define TOUCH_SHARE 0.9F

/* the most numbered fences the chain holds */
#define CHAIN 20
/* a throw turns toward the next numbered fence only within this of its own
 * heading */
#define AIM_SPREAD NJM_DEG_ANG(67.5)
/* a spring band throws a player only past a stretch of THROW_STRETCH either
 * way. it throws only a player moving faster than THROW_MIN_SPEED. the throw's
 * speed is THROW_BASE plus THROW_GAIN times the player's speed, up to
 * THROW_MAX */
#define THROW_STRETCH 2.0F
#define THROW_MIN_SPEED 0.2F
#define THROW_BASE 1.5F
#define THROW_GAIN 1.1F
#define THROW_MAX 6.0F

/* the collision record's radius is the band shape's distance from the
 * fence's position, plus REACH_MARGIN */
#define REACH_MARGIN 9.25F

/* the fence's number, which init moves here from scl.z */
#define NUMBER(work) ((work)->tick)
/* the stretch's change in the last step, in scl.z once init has taken the
 * number from it */
#define RATE(work) ((work)->scl.z)
/* the player the band holds, or last held */
#define HOLDER(work) ((work)->submode)

/* heading, stretch and slide are in the fence's frame. that frame puts the
 * fence's position at the origin, turned by -ang.y. the band lies along z
 * there, between the posts */
struct elastic_fence_vars {
    /* the player's speed's angle in the fence's frame, from x toward z, as the
     * band took hold */
    Uint16 heading;
    Uint16 kind; /* FENCE_ flags */
    Float stretch; /* how far the band is pushed along x, out of line */
    Float slide; /* where along the band, along z, the player pushes it */
};

#define VARS(work) ((struct elastic_fence_vars *)&(work)->vars)

static void elastic_fence_init(struct task *task);
static void elastic_fence_destroy(struct task *task);
static void elastic_fence_step(struct task *task);
static void elastic_fence_draw(struct task *task);

/* the pull's stiffness as rubber and as string */
static Float elastic_fence_rubber = 0.02F;
static Float elastic_fence_string = 0.02F;
/* the fences alive. the first to start clears the chain */
static int elastic_fence_count = 0;
static struct collision_shape elastic_fence_posts[2] = {
    {SHAPE_CAPSULE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 3.0F, -8.0F},
     1.5F,
     2.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CAPSULE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 3.0F, 8.0F},
     1.5F,
     2.0F,
     0.0F,
     {0},
     0,
     NULL},
};
/* the band's shape. collision_init copies SHAPES shapes from
 * elastic_fence_posts, so this one must follow them. the step moves the
 * copy's center by the stretch and the slide */
/* @hack type: the band's shape apart from elastic_fence_posts, which keeps
 * the fence the walk to the next numbered fence finds in R7 (0x0c9166b8); as
 * the array's third element, R3 takes it and spills to the stack, the
 * function runs 2 bytes short and 161 of elastic_fence_step's 721 words
 * differ */
static struct collision_shape elastic_fence_band = {
    SHAPE_CYLINDER | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 3.5999998F, 0.0F},
    BAND_HALF,
    2.0F,
    0.0F,
    {0},
    0,
    NULL,
};
/* a zero point. its definition stands after elastic_fence_step, for its
 * place among the constants */
static const NJS_POINT3 elastic_fence_origin;
/* the numbered fences in play, in no order */
static struct task_work *elastic_fence_chain[CHAIN];

/* TRUE when a player at pos, of radius and moving at speed, touches the band.
 * *stretch and *slide then give where, in the fence's frame. contact is unused.
 *
 * a free band takes a player once pos is within radius of the band or past it.
 * the player came from the side pos less speed lies on, and pushes the band
 * away from it. a free band also sets *heading to speed's angle in the fence's
 * frame, from x toward z. a band that holds a player takes one only on the side
 * it is stretched to */
static Bool elastic_fence_touch(NJS_POINT3 *pos, Float radius,
                                NJS_VECTOR *speed, NJS_POINT3 *contact,
                                Float *stretch, Float *slide,
                                struct task_work *work, int *heading)
{
    Bool hit = FALSE;
    Angle ang;
    NJS_POINT3 at;
    NJS_VECTOR going;

    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    njTranslate(NULL, -work->pos.x, -work->pos.y, -work->pos.z);
    njCalcVector(NULL, speed, &going);
    njCalcPoint(NULL, pos, &at);
    njPopMatrix(1);
    if (TASK_TIMER(work) > 1) {
        if (at.x <= radius && at.x > going.x) {
            *stretch = at.x - radius;
            hit = TRUE;
            *heading = (int)(atan2f(going.z, going.x) * ANGLE_PER_RADIAN);
        } else if (at.x >= -radius && at.x < going.x) {
            *stretch = at.x + radius;
            hit = TRUE;
            *heading = (int)(atan2f(going.z, going.x) * ANGLE_PER_RADIAN);
        }
    } else if (VARS(work)->stretch < 0.0F) {
        if (at.x <= radius) {
            *stretch = at.x - radius;
            hit = TRUE;
        }
    } else if (at.x >= -radius) {
        *stretch = at.x + radius;
        hit = TRUE;
    }
    if (!hit) {
        return FALSE;
    }
    *slide = at.z;
    return TRUE;
}

static void elastic_fence_set_kind(Float throwing, Float pulling,
                                   struct task_work *work)
{
    VARS(work)->kind = 0;
    switch ((Sint8)throwing % 3) {
    case 0:
        VARS(work)->kind |= FENCE_THROW_SPRING;
        break;
    case 1:
        VARS(work)->kind |= FENCE_THROW_SPRING;
        VARS(work)->kind |= FENCE_THROW_EIGHTHS;
        break;
    case 2:
        VARS(work)->kind |= FENCE_THROW_NONE;
        break;
    }
    switch ((Sint8)pulling % 3) {
    case 0:
        VARS(work)->kind |= FENCE_PULL_RUBBER;
        break;
    case 1:
        VARS(work)->kind |= FENCE_PULL_STRING;
        break;
    }
}

void elastic_fence_start(struct task *task)
{
    /* @hack redundant: work set and never read, which keeps task in R14
     * across placement_try_leave (0x0c915f6a); without it, task goes to the
     * stack and 10 of 14 words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        elastic_fence_init(task);
    }
}

static void elastic_fence_init(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    if (!placement_editing(task)) {
        collision_init(task, elastic_fence_posts, COLLISION_OBJECT, 1, SHAPES);
    }
    task->draw = elastic_fence_draw;
    task->step = elastic_fence_step;
    task->destructor = elastic_fence_destroy;
    elastic_fence_set_kind(work->scl.x, work->scl.y, work);
    NUMBER(work) = (Uint8)work->scl.z;
    work->timer = 2;
    VARS(work)->stretch = 0.0F;
    VARS(work)->slide = 0.0F;
    RATE(work) = 0.0F;
    if (!elastic_fence_count++) {
        memset((void *)elastic_fence_chain, 0, sizeof(elastic_fence_chain));
    }
    if (NUMBER(work) != 0) {
        for (i = 0; i < CHAIN; i++) {
            if (!elastic_fence_chain[i]) {
                elastic_fence_chain[i] = work;
                break;
            }
        }
    }
}

static void elastic_fence_destroy(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    --elastic_fence_count;
    for (i = 0; i < CHAIN; ++i) {
        if (elastic_fence_chain[i] == work) {
            elastic_fence_chain[i] = NULL;
        }
    }
}

static void elastic_fence_step(struct task *task)
{
    NJS_POINT3 point;
    NJS_VECTOR pull;
    struct task_work *work = task->work;
    Float stretch = VARS(work)->stretch;
    Float slide = VARS(work)->slide;
    Float rate = RATE(work);
    Sint8 player = HOLDER(work);
    short timer = work->timer;
    struct task_work *who = player_work[player];
    struct player *record = player_get(player);
    int heading;
    Float zero;
    int throw_heading;
    int off;
    int i;
    Angle ang;
    Float to_post_a;
    Float to_post_b;
    Float extra;
    Float length;
    Float scale;
    Float bounce;
    Float stiffness;

    if (placement_try_leave(task)) {
        return;
    }
    zero = 0.0F;
    /* @hack redundant: a store of 1.0 to zero that never runs, which keeps zero
     * in a register of its own for the test at 0x0c916414; without it, 14 of
     * 721 words differ */
    if (0) {
        zero = 1.0F;
    }
    /* @hack redundant: a store to work->mode that never runs, which keeps the
     * task at @(8,R15) in a 48-byte frame (0x0c9160d0 to 0x0c9160d8); without
     * it, the frame takes 44 bytes, the function runs 32 bytes short and 705 of
     * 721 words differ */
    if (0) {
        work->mode = 0;
    }
    if (placement_editing(task)) {
        elastic_fence_set_kind(work->scl.x, work->scl.y, work);
        NUMBER(work) = (Uint8)work->scl.z;
        work->timer = 0;
        VARS(work)->stretch = zero;
        VARS(work)->slide = zero;
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X),
                 VARS(work)->kind & FENCE_THROW_SPRING
                     ? (VARS(work)->kind & FENCE_THROW_EIGHTHS
                            ? "ReflectMode: BANE45"
                            : "ReflectMode: BANE")
                     : "ReflectMode: GOMU");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y),
                 VARS(work)->kind & FENCE_PULL_STRING ? "Stretchmode: HIMO"
                                                      : "Stretchmode: GOMU");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "ID:");
        njPrintD(NJM_LOCATION(22, EDITOR_ROW_SCL_Z), NUMBER(work), 2);
        elastic_fence_draw(task);
        return;
    }
    elastic_fence_draw(task);
    /* a hit on the band lets a free band take the nearest player. a band that
     * holds a player heeds only that player's hits */
    if ((work->collision->flags & COLLISION_HIT) &&
        work->collision->own_shape == BAND_SHAPE &&
        (timer > 1 || work->collision->hit->owner->work == who)) {
        heading = VARS(work)->heading;
        if (timer > 1) {
            player = player_nearest(&work->pos);
            who = player_work[player];
            record = player_get(player);
        }
        if (elastic_fence_touch(&who->pos, record->consts.radius * TOUCH_SHARE,
                                &player_motion[player]->speed, &point, &stretch,
                                &slide, work, &heading)) {
            timer = 0;
        }
        VARS(work)->heading = heading;
    }
    if (timer == 0 || stretch > 0.1F || slide > 0.1F) {
        work->collision->shapes[BAND_SHAPE].center.x =
            elastic_fence_band.center.x + stretch;
        work->collision->shapes[BAND_SHAPE].center.z =
            elastic_fence_band.center.z + slide;
    }
    {
        Float x = work->collision->shapes[BAND_SHAPE].center.x;
        Float z = work->collision->shapes[BAND_SHAPE].center.z;
        Float reach = njSqrt((x * x) + (z * z)) + REACH_MARGIN;

        work->collision->radius = reach;
    }
    collision_register(work);
    if (timer > 0) {
        if (_builtin_fabsf(slide) < 0.1F) {
            slide = zero;
        }
        if (_builtin_fabsf(rate) < 0.1F) {
            rate = zero;
        }
        slide *= 0.92F;
        rate += -(stretch * 0.5F);
        rate *= 0.95F;
        stretch += rate;
    } else {
        /* @hack redundant: the stretch tested twice, as the image tests it
         * (0x0c9163cc to 0x0c9163da); tested once, 52 of 721 words differ */
        /* NOLINTNEXTLINE(misc-redundant-expression) */
        if (_builtin_fabsf(stretch) > 0.1F || _builtin_fabsf(stretch) > 0.1F) {
            /* pull the player toward a point on the side it came from. the
             * point lies TOUCH_SHARE of the player's radius off the band's
             * middle at rest */
            point = elastic_fence_origin;
            if (stretch < 0.0F) {
                point.x += record->consts.radius * TOUCH_SHARE;
            } else {
                point.x -= record->consts.radius * TOUCH_SHARE;
            }
            /* zero is 0, so either arm leaves point.z at 0 */
            if (zero < 0.0F) {
                point.z += slide * zero;
            } else {
                point.z = slide * zero;
            }
            /* how much longer than at rest the band is, from each post to
             * the player */
            to_post_a = BAND_HALF - slide;
            to_post_b = -BAND_HALF - slide;
            extra = njSqrt((stretch * stretch) + (to_post_a * to_post_a)) +
                    njSqrt((stretch * stretch) + (to_post_b * to_post_b)) -
                    BAND_LENGTH;
            if (VARS(work)->kind & FENCE_PULL_STRING) {
                /* @hack redundant: tests of to_post_b and extra that never run,
                 * which keep AIM_SPREAD in R9 (0x0c9166b2) and load the half
                 * turn where it is used (0x0c916702); without them, R9 takes
                 * the half turn and 19 of 721 words differ */
                if (0) {
                    if (to_post_b) {
                    }
                    if (extra) {
                    }
                }
                stiffness = elastic_fence_string;
                rate = extra * extra * stiffness;
            } else {
                stiffness = elastic_fence_rubber;
                rate = extra * stiffness;
            }
            /* @hack redundant: a product of rate and stiffness that never runs,
             * which keeps the pull subtraction's cursors by in R5 and out in R4
             * (0x0c91651e to 0x0c91652a); without it, the two trade, with
             * registers further on, and 21 of 721 words differ */
            if (0) {
                rate *= stiffness;
            }
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njCalcPoint(NULL, &point, &point);
            njPopMatrix(1);
            /* @hack spelling: pull, point less the player's position, through
             * cursors over the three, for the image's post-increment loads
             * (0x0c916526 to 0x0c91653a); through the members, the frame takes
             * 44 bytes, the function runs 32 bytes short and 707 of 721 words
             * differ */
            {
                Float *out = VECTOR_FLOATS(&pull);
                Float *from = VECTOR_FLOATS(&point);
                Float *by = VECTOR_FLOATS(&who->pos);

                /* @hack redundant: a test of out that never runs, which keeps
                 * the pull subtraction's cursors by in R5 and out in R4
                 * (0x0c91651e to 0x0c91652a); without it, the two trade and 16
                 * of 721 words differ */
                if (0) {
                    if (out) {
                    }
                }
                /* @hack redundant: a test of by that never runs, which keeps by
                 * in R5 and from in R6 (0x0c91651c to 0x0c916528); without it,
                 * the two trade and 10 of 721 words differ */
                if (0) {
                    if (by) {
                    }
                }
                *out = *from - *by;
                out++;
                from++;
                by++;
                *out = *from - *by;
                out++;
                from++;
                by++;
                *out = *from - *by;
            }
            pull.y = zero;
            length = njScalor(&pull);
            scale = rate / length;
            /* @hack spelling: pull scaled to length rate through a cursor,
             * which the image steps by 4 between the products (0x0c91655c to
             * 0x0c916572); through the members, 186 of 721 words differ */
            {
                Float *out = VECTOR_FLOATS(&pull);

                *out++ *= scale;
                *out++ *= scale;
                *out *= scale;
            }
            player_push(player, pull.x, pull.y, pull.z);
            /* a pull stronger than the player's gravity adds that gravity back
             * to the player's acceleration */
            if (rate > record->consts.gravity) {
                player_motion[player]->accel.y += record->consts.gravity;
            }
        }
        /* a spring band throws the player off once the stretch stops moving
         * the way it moved last step */
        if (_builtin_fabsf(stretch) > THROW_STRETCH &&
            (VARS(work)->kind & FENCE_THROW_SPRING) &&
            ((RATE(work) < 0.0F && VARS(work)->stretch <= stretch) ||
             (RATE(work) > 0.0F && VARS(work)->stretch >= stretch))) {
            point = player_motion[player]->speed;
            bounce = njScalor(&point);
            if (bounce > THROW_MIN_SPEED) {
                bounce = -THROW_BASE - (bounce * THROW_GAIN);
                if (bounce < -THROW_MAX) {
                    bounce = -THROW_MAX;
                }
                if (VARS(work)->kind & FENCE_THROW_EIGHTHS) {
                    /* @hack spelling: the kept heading masked through a
                     * pointer, which the image forms as work + 8 and reads back
                     * sign-extended (0x0c91668e to 0x0c91669c); through
                     * VARS(work)->heading, the function runs 2 bytes short
                     * and 174 of 721 words differ */
                    Uint16 *kept = &VARS(work)->heading;

                    *kept &= EIGHTH_BITS;
                    throw_heading = (Sint16)*kept;
                } else {
                    throw_heading = VARS(work)->heading;
                }
                /* a numbered band throws toward the fence numbered one more. it
                 * does so only where that fence lies within AIM_SPREAD of the
                 * throw's heading */
                if (NUMBER(work)) {
                    struct task_work **fence;
                    struct task_work **next;
                    struct task_work *found;

                    /* @hack spelling: the chain walked by two pointers, next
                     * for the test and fence for the position, which the image
                     * keeps in R6 and R4 (0x0c9166b4 to 0x0c9166cc); with
                     * fence alone, the function runs 32 bytes short and 187
                     * of 721 words differ */
                    for (i = 0, fence = elastic_fence_chain,
                        next = elastic_fence_chain;
                         i < CHAIN; ++i, ++fence) {
                        /* @hack redundant: a test of fence that never runs,
                         * which, like the store to work->mode above, keeps the
                         * task at @(8,R15) in a 48-byte frame (0x0c9160d0 to
                         * 0x0c9160d8); without it, the frame takes 44 bytes,
                         * the function runs 32 bytes short and 705 of 721 words
                         * differ */
                        if (0) {
                            if (fence) {
                            }
                        }
                        if ((found = *next++) &&
                            NUMBER(found) == NUMBER(work) + 1) {
                            /* @hack spelling: point, the next fence's position
                             * less the player's, through cursors over the
                             * three, for the image's post-increment loads
                             * (0x0c9166d6 to 0x0c9166f0); through the members,
                             * the frame takes 44 bytes, the function runs 32
                             * bytes short and 705 of 721 words differ */
                            {
                                Float *out = VECTOR_FLOATS(&point);
                                Float *from = VECTOR_FLOATS(&(*fence)->pos);
                                Float *by = VECTOR_FLOATS(&who->pos);

                                *out = *from - *by;
                                out++;
                                from++;
                                *out = *from - *++by;
                                out++;
                                from++;
                                by++;
                                *out = *from - *by;
                            }
                            ang = NJM_DEG_ANG(180) -
                                  (int)(atan2f(point.z, point.x) *
                                        ANGLE_PER_RADIAN) -
                                  work->ang.y;
                            off = ang - throw_heading;
                            if ((short)off < AIM_SPREAD &&
                                (short)off > -AIM_SPREAD) {
                                throw_heading = ang;
                            }
                            break;
                        }
                    }
                }
                point.x = bounce;
                point.y = zero;
                point.z = zero;
                njPushMatrix(&_unit_matrix_);
                TURN_VIA(njRotateY, throw_heading + work->ang.y, ang);
                njCalcVector(NULL, &point, &point);
                njPopMatrix(1);
                ang = -(int)(atan2f(point.z, point.x) * ANGLE_PER_RADIAN);
                player_set_angles(player, 0, ang + NJM_DEG_ANG(90), 0);
                player_bounce(player, point.x, point.y, point.z);
            }
        }
        rate = stretch - VARS(work)->stretch;
    }
    /* work->timer counts the steps since the band last held a player. the timer
     * is 1 after a step that held one. the band is free while the timer is
     * above 1. the count stops at 0x7fff rather than wrap */
    timer = timer + 1;
    if (timer <= 0) {
        --timer;
    }
    VARS(work)->stretch = stretch;
    RATE(work) = rate;
    VARS(work)->slide = slide;
    HOLDER(work) = player;
    work->timer = timer;
}

/* @hack spelling: defined after elastic_fence_step, which places it after the
 * step's strings among the constants, at 0x0c919fb0. defined above the step,
 * it comes before them: 57 of the constants' 58 words differ, and so do 7 of
 * the step's pool words */
static const NJS_POINT3 elastic_fence_origin = {0.0F, 0.0F, 0.0F};

/* a band at rest draws as its model is. a stretched one draws from a stack
 * copy of its points. each point moves -stretch / 3 of the way toward its
 * place at full stretch. the middle points also move along the band by the
 * slide */
static void elastic_fence_draw(struct task *task)
{
    NJS_POINT3 points[BAND_POINTS];
    struct task_work *work = task->work;
    NJS_OBJECT *object = &elastic_fence_object;
    NJS_OBJECT *posts;
    NJS_POINT3 *kept;
    NJS_OBJECT *full;
    Float *from;
    Float *to;
    Float *out;
    Angle ang;
    Float blend;
    Float shift;
    Float x;
    Float y;
    Float z;
    Float from_y;
    Float from_z;
    Float to_x;
    Float to_y;
    Float to_z;
    int i;

    if (frame_step) {
        return;
    }
    if (placement_try_leave(task)) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y - NJM_DEG_ANG(90), ang);
    njTranslate(NULL, -7.4983958F, 0.0F, 0.0F);
    posts = object;
    draw_model(posts->model);
    object = object->child;
    if (VARS(work)->stretch == 0.0F && VARS(work)->slide == 0.0F) {
        njTranslate(NULL, 0.0F, 0.0F, 0.0F);
        njScale(NULL, BAND_SCALE, 1.0F, -1.0F);
        draw_model(object->model);
    } else {
        full = elastic_fence_stretch_action.object;
        /* @hack spelling: cursors over the band's points, its points at full
         * stretch and the stack copy, for the image's post-increment loads in
         * one loop (0x0c916976 to 0x0c9169b8); through the members, the
         * function runs 452 bytes long and 163 of 167 words differ */
        from = (Float *)(kept = object->model->points);
        to = (Float *)full->model->points;
        out = (Float *)&points;
        object->model->points = points;
        blend = -(VARS(work)->stretch * THIRD);
        shift = VARS(work)->slide * (1.0F / BAND_SCALE);
        for (i = 0; i < BAND_POINTS; ++i) {
            x = *from;
            /* @hack redundant: a test of out that never runs, which keeps the
             * loads at 0x0c916970 to 0x0c91697a in FR8, FR10 and FR9; without
             * it, they take FR11, FR9 and FR8 and 11 of 167 words differ */
            if (0) {
                if (out) {
                }
            }
            from++;
            from_y = *from;
            from++;
            from_z = *from;
            from++;
            to_x = *to;
            to++;
            to_y = *to;
            to++;
            to_z = *to;
            to++;
            x = x + ((to_x - x) * blend);
            y = from_y + ((to_y - from_y) * blend);
            z = from_z + ((to_z - from_z) * blend);
            if (x >= MIDDLE_LOW && x <= MIDDLE_HIGH) {
                x += shift;
            }
            *out = x;
            *++out = y;
            *++out = z;
            out++;
        }
        njScale(NULL, BAND_SCALE, 1.0F, -1.0F);
        draw_model(object->model);
        object->model->points = kept;
    }
    njPopMatrix(1);
}
