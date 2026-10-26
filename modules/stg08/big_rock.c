/* O BicIwa: a big rock that rolls and bounces, and that Big can pick up */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/snow.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg08/big_rock.h"
#include "stg08/streaks.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT big_rock_object;

/* in work->flags, cleared as each move starts. ROCK_HIT_WORLD: the ball's
 * sweep hit the world and left its y unequal to the rock's y speed. the
 * sweep starts at twice that speed. a sweep that keeps its y then sets the
 * bit whenever the rock moves along y. ROCK_HIT_OBJECT: the sphere touched
 * a COLLISION_OBJECT_MUTUAL record in the checks since the rock last
 * registered its collision */
#define ROCK_HIT_WORLD 0x1
#define ROCK_HIT_OBJECT 0x2

/* the radius of the ball it sweeps through the world, and its center's
 * height above the rock's position */
#define ROCK_RADIUS 15.0F
#define ROCK_CENTER 14.5F

/* the sphere's index in the rock's shapes */
#define ROCK_SPHERE 1

/* SNOW_WAIT counts the moves since the rock last threw snow, stood held or
 * edited, or started. it stops at SNOW_REST + 1 */
#define SNOW_REST 5
#define SNOW_WAIT(work) ((work)->submode)

/* a slow rock speeds up only on ground whose normal has a y under
 * ROLL_SLOPE. its bottom must also be above player 0, by less than
 * ROLL_ABOVE */
#define ROLL_SLOPE 0.9F
#define ROLL_ABOVE 100.0F

#define SE_ROCK_HIT 216

/* a cylinder of radius 54 about the rock, and the sphere, of radius 18.
 * SHC converts 0.6 and 0.86 to Floats toward zero, and rounds each product
 * toward zero too. that gives the image's floats, one below 8.4F and
 * 20.64F in the last place */
static struct collision_shape big_rock_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     0,
     0,
     {0.0F, 14.0F * 0.6F, 0.0F},
     54.0F,
     24.0F * 0.86F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL,
     {0.0F, 12.4F, 0.0F},
     18.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};
/* a hit faster than big_rock_impact_speed throws snow and
 * big_rock_streak_count streaks. the snow's scale is big_rock_snow_scale
 * times the rock's speed. the streaks' spread is big_rock_streak_spread
 * times it */
static Float big_rock_snow_scale = 0.15F;
static Float big_rock_impact_speed = 3.0F;
static Float big_rock_streak_spread = 0.32F;
static int big_rock_streak_count = 70;

static void big_rock_start(struct task *task);
static void big_rock_destroy(struct task *task);
static void big_rock_run(struct task *task);
static void big_rock_draw(struct task *task);
static int big_rock_roll(struct task *task, NJS_VECTOR *up);

/* create snow at pos, moved nearer units toward the camera when it is
 * farther than that */
static void big_rock_snow(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale,
                          Float nearer)
{
    NJS_VECTOR offset;
    Float length;

    offset.x = pos->x - camera_work->pos.x;
    offset.y = pos->y - camera_work->pos.y;
    offset.z = pos->z - camera_work->pos.z;
    length = njScalor(&offset);
    if (length > nearer) {
        length = (length - nearer) / length;
        offset.x *= length;
        offset.y *= length;
        offset.z *= length;
    }
    offset.x += camera_work->pos.x;
    offset.y += camera_work->pos.y;
    offset.z += camera_work->pos.z;
    snow_create(&offset, speed, scale);
}

void big_rock_step(struct task *task)
{
    /* @hack spelling: rock, a copy of task, which keeps task in R14 across
     * the range check (MOV R4,R14 at 0x0c90cb84); through task, it goes to
     * the frame and 9 of 14 words differ */
    struct task *rock = task;

    if (!placement_try_leave(rock)) {
        big_rock_start(rock);
    }
}

static void big_rock_start(struct task *task)
{
    struct task_motion *motion = task->motion;
    struct task_work *work = task->work;
    Float *component;

    if (!placement_editing(task)) {
        /* @hack spelling: shape, a pointer to big_rock_shapes, which keeps
         * their address in R4 and R5 (0x0c90cbdc to 0x0c90cbf4); through
         * big_rock_shapes, it takes R12, which the function saves, and 57
         * of 60 words differ */
        struct collision_shape *shape = big_rock_shapes;

        shape->push |= PUSH_FIRM(PUSH_ALL);
        big_rock_shapes[ROCK_SPHERE].push |= PUSH_FIRM(PUSH_ALL);
        collision_init(task, shape, COLLISION_OBJECT_MUTUAL, 1, 2);
    }
    carry_set_kind(work, motion, CARRY_KIND_LARGE);
    task->draw = big_rock_draw;
    task->step = big_rock_run;
    task->destructor = big_rock_destroy;
    /* @hack spelling: scl written through VECTOR_FLOATS, a float at a time from
     * 0x0c90cc20; through the members, 30 of 60 words differ */
    component = VECTOR_FLOATS(&work->scl);
    *component++ = 0.0F;
    *component++ = 1.0F;
    *component = 0.0F;
    SNOW_WAIT(work) = 0;
    work->timer = 0;
    work->tick = 0;
}

static void big_rock_destroy(struct task *task)
{
}

/* draw it, highlighted as draw_highlight_near decides, then move it */
static void big_rock_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        draw_highlight_near(work, ALL_CHARACTERS);
        big_rock_draw(task);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        big_rock_roll(task, &work->scl);
    }
}

/* the model, turned about its center by the yaw and pitch of the up vector
 * in scl */
static void big_rock_draw(struct task *task)
{
    NJS_MODEL *model = big_rock_object.model;
    struct task_work *work = task->work;
    Float x;
    Float y;
    Float z;
    Float across;
    Float *component;
    Angle yaw;
    Angle pitch;

    if (!placement_try_leave(task)) {
        if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 50.0F)) {
            /* @hack spelling: scl read through VECTOR_FLOATS, as FMOV.S @R4+
             * from 0x0c90cd02; through the members, 103 of 122 words
             * differ */
            component = VECTOR_FLOATS(&work->scl);
            x = *component++;
            y = *component++;
            z = *component;
            across = njSqrt((x * x) + (z * z));
            texture_set_stage();
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            njTranslateV(NULL, &model->center);
            pitch = (Angle)(atan2f(across, y) * ANGLE_PER_RADIAN);
            yaw = (Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
            TURN(njRotateY, yaw);
            TURN(njRotateX, pitch);
            njTranslate(NULL, -model->center.x, -model->center.y,
                        -model->center.z);
            draw_model(model);
            njPopMatrix(1);
        }
    }
}

/* move the rock on a frame's first step. a move falls, bounces and rolls
 * it. as it rolls, the move turns up, the rock's up vector in work->scl. on
 * the frame's other steps, the rock only registers its collision. a held or
 * edited rock stops and stands upright instead. the result is the move's
 * ROCK_HIT_WORLD bit, or 1 on the frame's other steps.
 *
 * player 0 counts as under water when its shadow found water above it the
 * step before. while it is, the rock's stride, gravity and least speed
 * halve. its damping is then 0.9 in place of 1.
 *
 * a hit faster than big_rock_impact_speed throws snow and streaks, and
 * plays SE_ROCK_HIT. it doesn't while player 0 is under water. nor does it
 * within SNOW_REST moves of the last throw, hold, edit or start */
static int big_rock_roll(struct task *task, NJS_VECTOR *up)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_VECTOR velocity;
    NJS_VECTOR normal;
    NJS_VECTOR sweep;
    NJS_VECTOR offset;
    NJS_POINT3 hit;
    NJS_POINT3 moved;
    struct angles ang;
    Float *component;
    Float speed;
    Float across;
    Float gravity;
    Float damping;
    Float stride; /* how many velocities a move covers */
    Float least; /* the speed across below which a roll is pushed on */
    Float bottom;
    Float factor;
    Float travel;
    Angle angle;
    Angle roll;
    Angle yaw;
    struct collision_shape *shape;

    if (frame_step) {
        if (!(work->flags & CARRY_HELD) && !placement_editing(task)) {
            collision_register(work);
        } else {
            /* @hack spelling: speed cleared in one chain, which steers the
             * spills' frame slots from 0x0c90d422 to 0x0c90d5e0; as three
             * stores from x to z, 9 of 1009 words differ */
            motion->speed.z = motion->speed.y = motion->speed.x = 0.0F;
            /* @hack spelling: accel cleared in one chain, as speed is; as three
             * stores, 11 of 1009 words differ */
            motion->accel.x = motion->accel.y = motion->accel.z = 0.0F;
            /* @hack spelling: velocity cleared in one chain, as speed is; as
             * three stores from x to z, 9 of 1009 words differ */
            velocity.z = velocity.y = velocity.x = 0.0F;
            /* @hack spelling: up written through VECTOR_FLOATS, which keeps up
             * in R10 and the 0 in R12 (0x0c90ce22, 0x0c90ce26); through the
             * members, up takes R12, the registers after it shift down one,
             * and 101 of 1009 words differ */
            component = VECTOR_FLOATS(up);
            *component++ = 0.0F;
            *component++ = 1.0F;
            *component = 0.0F;
            SNOW_WAIT(work) = 0;
            work->timer = 0;
        }
        object_clear_touch(work, motion);
        return 1;
    }
    if (player_get(0)->shadow.last_above_flags & WORLD_WATER) {
        damping = 0.9F;
        least = 1.0F;
        stride = 1.0F;
        gravity = 0.25F;
    } else {
        stride = 2.0F;
        least = 2.0F;
        gravity = 0.5F;
        damping = 1.0F;
    }
    normal.x = 0.0F;
    normal.y = 1.0F;
    normal.z = 0.0F;
    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    work->flags &= ~(ROCK_HIT_WORLD | ROCK_HIT_OBJECT);
    if ((work->flags & CARRY_HELD) || placement_editing(task)) {
        motion->speed.x = 0.0F;
        motion->speed.y = 0.0F;
        motion->speed.z = 0.0F;
        /* @hack spelling: accel cleared in one chain, which steers the spills'
         * frame slots from 0x0c90d422 to 0x0c90d5e0; as three stores, 11 of
         * 1009 words differ */
        motion->accel.x = motion->accel.y = motion->accel.z = 0.0F;
        velocity.x = 0.0F;
        velocity.y = 0.0F;
        velocity.z = 0.0F;
        /* @hack spelling: up written through VECTOR_FLOATS again; through the
         * members, 79 of 1009 words differ */
        component = VECTOR_FLOATS(up);
        *component++ = 0.0F;
        *component++ = 1.0F;
        *component = 0.0F;
        SNOW_WAIT(work) = 0;
        work->timer = 0;
    } else {
        velocity = motion->speed;
        if ((work->collision->flags & COLLISION_HIT) &&
            work->collision->hit->kind == COLLISION_OBJECT_MUTUAL &&
            work->collision->own_shape == ROCK_SPHERE) {
            work->flags |= ROCK_HIT_OBJECT;
        }
        moved = hit = work->pos;
        sweep.x = velocity.x * 2.0F;
        sweep.y = velocity.y * 2.0F;
        sweep.z = velocity.z * 2.0F;
        ang = work->ang;
        offset.x = 0.0F;
        offset.y = ROCK_CENTER;
        offset.z = 0.0F;
        njAddVector(&hit, &offset);
        if (world_sweep_ball_hit(&hit, &sweep, &ang, WORLD_WATER,
                                 ROCK_RADIUS)) {
            /* @hack type: _unit_matrix_ cast from a pointer to its first
             * Float, here and at the second push. it steers the spills'
             * frame slots from 0x0c90d422 to 0x0c90d4ba, and puts travel in
             * FR5 at 0x0c90d6dc; as &_unit_matrix_, 11 of 1009 words
             * differ */
            njPushMatrix((NJS_MATRIX *)_unit_matrix_);
            TURN(njRotateZ, ang.z);
            TURN(njRotateX, ang.x);
            njCalcVector(NULL, &normal, &normal);
            njPopMatrix(1);
            moved.x = hit.x + sweep.x;
            moved.y = hit.y + sweep.y;
            moved.z = hit.z + sweep.z;
            if (sweep.y != motion->speed.y) {
                work->flags |= ROCK_HIT_WORLD;
            }
            offset.x = 0.0F;
            offset.y = ROCK_CENTER;
            offset.z = 0.0F;
            njSubVector(&moved, &offset);
            velocity.x = (moved.x - work->pos.x) * 0.5F;
            velocity.y = (moved.y - work->pos.y) * 0.5F;
            velocity.z = (moved.z - work->pos.z) * 0.5F;
        }
        if (SNOW_WAIT(work) <= SNOW_REST) {
            SNOW_WAIT(work)++;
        }
        if (work->flags & (ROCK_HIT_WORLD | ROCK_HIT_OBJECT)) {
            if (SNOW_WAIT(work) > SNOW_REST &&
                (speed = njScalor(&motion->speed)) > big_rock_impact_speed &&
                !(player_get(0)->shadow.last_above_flags & WORLD_WATER)) {
                sweep.x = motion->speed.x * 0.3F;
                sweep.y = motion->speed.y * 0.1F;
                sweep.z = motion->speed.z * 0.3F;
                offset.x =
                    work->pos.x + work->collision->shapes[ROCK_SPHERE].center.x;
                offset.y =
                    work->pos.y +
                    (work->collision->shapes[ROCK_SPHERE].center.y * 0.6F);
                offset.z =
                    work->pos.z + work->collision->shapes[ROCK_SPHERE].center.z;
                big_rock_snow(&offset, &sweep, big_rock_snow_scale * speed,
                              work->collision->shapes[ROCK_SPHERE].radius +
                                  8.0F);
                sweep.y = speed * 0.8F;
                streaks_ring(&work->pos, &sweep, big_rock_streak_spread * speed,
                             big_rock_streak_count, 15.0F);
                SNOW_WAIT(work) = 0;
                sound_play_at(SE_ROCK_HIT, 0, 0, (int)speed, work->pos.x,
                              work->pos.y, work->pos.z);
            }
            shape = work->collision->shapes;
            bottom = work->pos.y + shape->center.y - (shape->size_y * 0.7F);
            work->pos.y += velocity.y * stride;
            work->pos.x += velocity.x * stride;
            work->pos.z += velocity.z * stride;
            if (work->flags & ROCK_HIT_OBJECT) {
                motion->speed.x = velocity.x;
                motion->speed.z = velocity.z;
            } else if (bottom > player_work[0]->pos.y &&
                       _builtin_fabsf(bottom - player_work[0]->pos.y) <
                           ROLL_ABOVE &&
                       (across = njSqrt((velocity.x * velocity.x) +
                                        (velocity.z * velocity.z))) < least &&
                       normal.y < ROLL_SLOPE) {
                /* a slow rock that hit the world alone speeds up to least
                 * across. with almost no speed across, it heads for player
                 * 0 */
                if (across > 0.001F) {
                    across = least / across;
                    motion->speed.x = velocity.x * across;
                    motion->speed.z = velocity.z * across;
                } else {
                    motion->speed.x = player_work[0]->pos.x - work->pos.x;
                    motion->speed.z = player_work[0]->pos.z - work->pos.z;
                    across =
                        least / njSqrt((motion->speed.x * motion->speed.x) +
                                       (motion->speed.z * motion->speed.z));
                    motion->speed.x *= across;
                    motion->speed.z *= across;
                }
            } else {
                motion->speed.y = motion->speed.y * damping * 0.83F;
                motion->speed.x = velocity.x * damping * 0.83F;
                motion->speed.z = velocity.z * damping * 0.83F;
            }
            if (_builtin_fabsf(motion->speed.y) < gravity) {
                motion->speed.y = 0.0F;
            }
            if (motion->speed.y < 0.0F) {
                speed = njSqrt((motion->speed.x * motion->speed.x) +
                               (motion->speed.z * motion->speed.z));
                if (((work->flags & ROCK_HIT_OBJECT) && speed > least) ||
                    (!(work->flags & ROCK_HIT_OBJECT) && speed > gravity)) {
                    factor = 1.0F +
                             (_builtin_fabsf(motion->speed.y * 0.85F / speed) *
                              damping * 0.91F);
                    /* @hack redundant: a never-run square of factor. SHC
                     * writes no code for it, but the use of factor still
                     * steers the float registers; without it, travel and its
                     * quotient trade FR5 and FR4 at 0x0c90d6dc and 5 of 1009
                     * words differ */
                    if (0) {
                        factor *= factor;
                    }
                    if ((work->flags & ROCK_HIT_WORLD) ||
                        speed > gravity * 7.0F) {
                        /* @hack spelling: the jump into the else arm, which
                         * keeps player_work's address in R12 for the test at
                         * 0x0c90d518; as one test with &&, the test loads it
                         * from a pool word of its own, and 24 of 1009 words
                         * and 5 pool words differ */
                        if (bottom > player_work[0]->pos.y) {
                            goto undamped;
                        }
                        motion->speed.x *= damping * factor * 0.955F;
                        motion->speed.z *= damping * factor * 0.955F;
                    } else {
                    undamped:
                        motion->speed.x *= factor;
                        motion->speed.z *= factor;
                    }
                } else if (work->flags & ROCK_HIT_OBJECT) {
                    if (speed < least * 0.01F) {
                        motion->speed.x = player_work[0]->pos.x - work->pos.x;
                        motion->speed.z = player_work[0]->pos.z - work->pos.z;
                        speed = njSqrt((motion->speed.x * motion->speed.x) +
                                       (motion->speed.z * motion->speed.z));
                        speed = least * 0.99F / speed;
                        motion->speed.x *= speed;
                        motion->speed.z *= speed;
                    } else {
                        speed = least * 0.99F / speed;
                        motion->speed.x *= speed;
                        motion->speed.z *= speed;
                    }
                } else if (work->flags & ROCK_HIT_WORLD) {
                    motion->speed.x = 0.0F;
                    motion->speed.z = 0.0F;
                }
                motion->speed.y *= -0.15F;
                work->pos.y +=
                    (motion->speed.y > 0.5F ? 0.5F : motion->speed.y) * stride;
            }
        } else {
            work->pos.y += velocity.y * stride;
            work->pos.x += velocity.x * stride;
            work->pos.z += velocity.z * stride;
            /* @hack spelling: the subtraction spelled out; as -=, SHC gives
             * the function a frame 4 bytes smaller, and 984 of its 1009
             * words differ */
            motion->speed.y = motion->speed.y - gravity;
            motion->speed.x = velocity.x;
            motion->speed.z = velocity.z;
        }
        collision_register(work);
    }
    velocity.y = 0.0F;
    travel = njScalor(&velocity);
    if (travel > gravity) {
        /* the rock turns 0.9 of a turn per circumference of travel while it
         * hits something, and 0.4 otherwise. each factor is grouped so that
         * SHC folds it to the image's pool word, at 0x0c90d710 and
         * 0x0c90d7c4. SHC converts each literal to a Float toward zero. it
         * rounds each step of the folding toward zero too. the words are
         * two and three below 0.9F * 65536.0F and 0.4F * 65536.0F in the
         * last place */
        Float circumference = ROCK_RADIUS * 2.0F * 3.14F;

        if (work->flags & (ROCK_HIT_WORLD | ROCK_HIT_OBJECT)) {
            roll = (Angle)((travel / circumference) *
                           ((2.0F * 3.141592F * 0.9F) *
                            (65536.0F / (2.0F * 3.141592F))));
        } else {
            roll = (Angle)((travel / circumference) *
                           ((2.0F * 3.141592F) *
                            (0.4F * (65536.0F / (2.0F * 3.141592F)))));
        }
        yaw = (Angle)(atan2f(velocity.z, velocity.x) * ANGLE_PER_RADIAN);
        /* @hack type: _unit_matrix_ cast as at the first push; as
         * &_unit_matrix_, 11 of 1009 words differ */
        njPushMatrix((NJS_MATRIX *)_unit_matrix_);
        if (yaw) {
            njRotateY(NULL, yaw);
        }
        /* @hack spelling: the empty arm, which with the plain if above, not
         * TURN, gives njRotateX's address R2 at 0x0c90d75a; as if (roll), it
         * takes R3 and 2 of 1009 words differ */
        if (!roll) {
        } else {
            /* @hack spelling: angle, a copy of roll, which steers the
             * spills' frame slots from 0x0c90d5d0 to 0x0c90d5e0 and puts
             * travel in FR5 at 0x0c90d6dc; passing roll, 8 of 1009 words
             * differ */
            angle = roll;
            njRotateX(NULL, angle);
        }
        TURN(njRotateY, -yaw);
        njCalcVector(NULL, up, up);
        njPopMatrix(1);
    }
    object_clear_touch(work, motion);
    return work->flags & ROCK_HIT_WORLD;
}
