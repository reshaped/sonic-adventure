/* the O KAIZA pirate. it walks round an ellipse until an attack hurts it,
 * then tumbles and breaks apart */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/explosion.h"
#include "object/placement.h"
#include "object/ring.h"
#include "player/player.h"
#include "stg09/casino.h"
#include "stg09/fragments.h"
#include "stg09/pirate.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT pirate_a_object;
extern NJS_ACTION pirate_a_action;

/* placement_try_leave_by's squared range: about 250 from the camera or
 * player 0 */
#define RANGE 62510.0F

#define PHASE_RATE(work) ((work)->scl.x)
#define RADIUS(work) ((work)->scl.y)
#define STRETCH(work) ((work)->scl.z + 1.0F)

/* the walk action's frames for each unit the pirate walks */
static Float pirate_a_frames_per_unit = 1.1F;
/* the share of its speed and spin the tumbling pirate keeps each frame */
static Float pirate_a_drag = 0.95F;
static Float pirate_a_gravity = -0.3F;
/* a tumbling pirate breaks on the floor once its speed in x and z is below
 * this */
static Float pirate_a_rest_speed = 4.0F;
/* a hit sends the pirate off at launch_out horizontally and launch_up
 * upward. each varies at random by up to its spread either way. player 0's
 * y angle sets the horizontal direction */
static Float pirate_a_launch_up = 4.0F;
static Float pirate_a_launch_out = 6.0F;
static Float pirate_a_launch_up_spread = 3.0F;
static Float pirate_a_launch_out_spread = 4.0F;
/* a hit spins the pirate about each axis at up to this many degrees a frame,
 * either way */
static Float pirate_a_spin_spread = 10.0F;
static Float pirate_a_shadow_scale = 0.9F;
/* a tumble breaks the pirate once its timer passes this many frames */
static int pirate_a_tumble_frames = 200;
static struct collision_shape pirate_a_shape = {
    SHAPE_CAPSULE | SHAPE_HURTABLE,
    ATTACK(2) | GUARD(3) | SHAPE_HURTS_PLAYERS,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.05F, 6.0F, 1.0F},
    2.8F,
    3.2F,
    0.0F,
    {0},
    0,
    NULL,
};
static struct fragments_spec pirate_a_fragments = {
    &pirate_a_object,
    {0.0F, 0.0F, 0.0F},
    {0, 0, 0},
    {0.0F, 0.3F, 0.0F},
    {0.0F, -0.08F, 0.0F},
    300.0F,
    {1.0F, 1.0F, 1.0F},
    0.2F,
    8.0F,
    0.4F,
    0.96F,
    NULL,
    &obj_casino9_texlist,
    0.0F,
    0.05F,
    0.1F,
};
/* @hack type: the shadow's point as three Floats, whose addresses the image
 * loads from a pool word each (MOV.L at 0x0c92025e, 0x0c92027e and
 * 0x0c920264, for x, y and z). shadow_draw_at reads all three through x's
 * address. as one NJS_POINT3, SHC reaches y and z from x's address, and 40
 * of 162 words differ */
#ifdef GCC
/* keeps the read the tag names: GCC's three are the members of one
 * NJS_POINT3 */
static NJS_POINT3 pirate_a_shadow;
#define pirate_a_shadow_x pirate_a_shadow.x
#define pirate_a_shadow_y pirate_a_shadow.y
#define pirate_a_shadow_z pirate_a_shadow.z
#else
static Float pirate_a_shadow_x;
static Float pirate_a_shadow_y;
static Float pirate_a_shadow_z;
#endif

static void pirate_a_destroy(struct task *task);
static void pirate_a_run(struct task *task);
static void pirate_a_draw(struct task *task);

/* how far to raise the tumbling pirate, or 0 while it walks. the raise keeps
 * the turned top of its shape at least size_y * 0.8 above the pirate's
 * position */
static Float pirate_a_lift(struct task_work *work)
{
    NJS_VECTOR top;
    Angle ang;

    if (work->mode == PIRATE_WALKING) {
        return 0.0F;
    }
    top.x = 0.0F;
    top.y = pirate_a_shape.center.y + pirate_a_shape.size_y;
    top.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, PIRATE_VARS(work)->state->heading, ang);
    njTranslateV(NULL, &PIRATE_VARS(work)->state->pos.point);
    TURN_VIA(njRotateZ, PIRATE_VARS(work)->state->ang.z, ang);
    TURN_VIA(njRotateY, PIRATE_VARS(work)->state->ang.y, ang);
    TURN_VIA(njRotateX, PIRATE_VARS(work)->state->ang.x, ang);
    njCalcVector(NULL, &top, &top);
    njPopMatrix(1);
    top.y -= pirate_a_shape.size_y * 0.8F;
    if (!(top.y < 0.0F)) {
        return 0.0F;
    }
    return -top.y;
}

/* the pirate's position in the world */
static void pirate_a_get_pos(struct task_work *work, NJS_POINT3 *pos)
{
    Angle ang;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &PIRATE_VARS(work)->state->origin);
    TURN_VIA(njRotateY, PIRATE_VARS(work)->state->heading, ang);
    njCalcPoint(NULL, &PIRATE_VARS(work)->state->pos.point, pos);
    njPopMatrix(1);
}

/* the pirate's position and speed in the world */
static void pirate_a_get_motion(struct task_work *work, NJS_POINT3 *pos,
                                NJS_VECTOR *speed)
{
    Angle ang;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &PIRATE_VARS(work)->state->origin);
    TURN_VIA(njRotateY, PIRATE_VARS(work)->state->heading, ang);
    njCalcPoint(NULL, &PIRATE_VARS(work)->state->pos.point, pos);
    njCalcVector(NULL, &PIRATE_VARS(work)->state->speed.vector, speed);
    njPopMatrix(1);
}

/* break the pirate into fragments and kill its task. the fragments take on
 * 0.8 of its speed in x and z. a placed pirate's placement makes no more */
static void pirate_a_break(struct task *task)
{
    struct task_work *work = task->work;
    union point_floats accel;
    union point_floats speed;
    NJS_POINT3 pos;
    struct angles ang;
    /* @hack redundant: an NJS_OBJECT never used, 52 of the 100 bytes ADD
     * #-100,R15 reserves (0x0c91f906). without it, it reserves 48 and 16 of
     * 85 words differ */
    NJS_OBJECT unused;
    Float *at;

    pirate_a_get_motion(work, &pos, &speed.vector);
    /* @hack spelling: accel cleared through a cursor over its elements, as the
     * image steps R4 through it (0x0c91f918 to 0x0c91f926). through its
     * members, the function runs 2 bytes long and 62 of 85 words differ */
    at = accel.elements;
    *at++ = 0.0F;
    *at++ = 0.0F;
    *at = 0.0F;
    /* @hack spelling: speed scaled through a cursor from (&speed)->elements.
     * SHC forms that address from the stack pointer, as the image does (MOV
     * R15,R4 and ADD #76,R4 at 0x0c91f928 and 0x0c91f92c). from
     * speed.elements, it keeps the address in a saved register, R12. the
     * function then runs 2 bytes long, and 55 of 85 words differ. through
     * the members, 67 differ */
    at = (&speed)->elements;
    *at++ *= 0.8F;
    *at++ *= 0.8F;
    *at *= 0.8F;
    speed.vector.y = 0.0F;
    ang.y = PIRATE_VARS(work)->state->ang.y;
    ang.x = PIRATE_VARS(work)->state->ang.x;
    ang.z = PIRATE_VARS(work)->state->ang.z;
    pos.y += 0.5F;
    fragments_scatter(&pos, &ang, &speed.vector, &accel.vector,
                      &pirate_a_fragments, PIRATE_VARS(work)->state->origin.y);
    if (task->placement) {
        placement_kill(task);
    } else {
        task_kill(task);
    }
}

/* set *pos to the point at phase round an ellipse at y 0. the ellipse's
 * radius is radius along z and radius * stretch along x */
static void pirate_a_ellipse(NJS_POINT3 *pos, Float radius, Float stretch,
                             Angle phase)
{
    pos->y = 0.0F;
    pos->x = radius * njCos(phase) * stretch;
    pos->z = radius * njSin(phase);
}

void pirate_a_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_init(task, &pirate_a_shape, COLLISION_ITEM, 1, 1);
        /* work->collision's copy of the shape takes guard 0 and attack 1. any
         * attack of 1 or more hurts it */
        work->collision->shapes->levels =
            work->collision->shapes->levels & ~SHAPE_GUARD;
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_ATTACK) | ATTACK(1);
        work->collision->radius = RADIUS(work) + 10.0F;
    }
    PIRATE_VARS(work)->state = heap_calloc(1, sizeof(struct pirate_state));
    PIRATE_VARS(work)->state->origin = work->pos;
    PIRATE_VARS(work)->state->heading = work->ang.y;
    task->draw = pirate_a_draw;
    task->step = pirate_a_run;
    task->destructor = pirate_a_destroy;
    work->timer = 0;
    PIRATE_VARS(work)->frame = 0.0F;
    work->mode = PIRATE_WALKING;
    pirate_a_ellipse(&PIRATE_VARS(work)->state->pos.point, RADIUS(work),
                     STRETCH(work), PIRATE_VARS(work)->phase);
}

static void pirate_a_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(PIRATE_VARS(work)->state);
}

static void pirate_a_run(struct task *task)
{
    struct task_work *work = task->work;
    union point_floats pos;
    union point_floats away;
    Float length;
    Float bounce;
    Angle ang;
    Float x;
    Float z;
    /* @hack redundant: flat_speed, which the image stores into its stack
     * frame and never reads (FMOV.S FR0,@(R0,R15) at 0x0c92004a). without
     * the store, the frame is 32 bytes, not 36 (ADD #-36,R15 at
     * 0x0c91fb32), and 90 of 612 words differ */
    Float flat_speed;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    /* the pirate runs on a frame's first step alone, so its timer and rates
     * count frames */
    if (frame_step) {
        return;
    }
    /* count frames, held at 65535 rather than wrapped to 0 */
    /* @hack spelling: <= 0, which for the unsigned timer is == 0, keeps the
     * image's CMP/PL R0 at 0x0c91fb5c. as == 0, 2 of 612 words differ */
    if (++TASK_TIMER(work) <= 0) {
        --TASK_TIMER(work);
    }
    pirate_a_draw(task);
    if (work->mode == PIRATE_WALKING) {
        /* @hack type: pos passed by the whole union's address. as
         * &pos.point, which the call to pirate_a_get_pos passes, SHC keeps
         * pos's address in one more saved register, R10. the function then
         * runs 32 bytes short, and 603 of 612 words differ */
        pirate_a_ellipse((NJS_POINT3 *)&pos, RADIUS(work), STRETCH(work),
                         PIRATE_VARS(work)->phase);
        /* @hack spelling: the speed, the move from the last point to the new,
         * formed through cursors over the elements. through the members, 164
         * of 612 words differ */
        {
            Float *speed = PIRATE_VARS(work)->state->speed.elements;
            Float *to = pos.elements;
            Float *from = PIRATE_VARS(work)->state->pos.elements;

            *speed++ = *to++ - *from++;
            *speed++ = *to++ - *from++;
            *speed = *to - *from;
        }
        PIRATE_VARS(work)->state->speed.vector.y = 0.0F;
        PIRATE_VARS(work)->state->pos.point = pos.point;
        if ((length = njScalor(&PIRATE_VARS(work)->state->speed.vector)) >
            0.0F) {
            PIRATE_VARS(work)->facing =
                (Angle)(atan2f(PIRATE_VARS(work)->state->speed.vector.x,
                               PIRATE_VARS(work)->state->speed.vector.z) *
                        ANGLE_PER_RADIAN);
        }
        PIRATE_VARS(work)->frame += length * pirate_a_frames_per_unit;
        if (PIRATE_VARS(work)->frame >= pirate_a_action.motion->nbFrame) {
            PIRATE_VARS(work)->frame = 0.0F;
            work->timer = 0;
        }
        PIRATE_VARS(work)->phase +=
            (Angle)(PHASE_RATE(work) * ANGLE_PER_DEGREE);
    }
    if (placement_editing(task)) {
        /* label the editor's scl rows. Hankei is Japanese for radius, and
         * Daen for ellipse */
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> Speed ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> Hankei ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> Daen ");
        PIRATE_VARS(work)->state->origin = work->pos;
        PIRATE_VARS(work)->state->heading = work->ang.y;
        return;
    }
    if (work->mode == PIRATE_WALKING) {
        pirate_a_get_pos(work, &pos.point);
        work->pos = pos.point;
        work->ang.y =
            PIRATE_VARS(work)->state->heading + PIRATE_VARS(work)->facing;
        if (work->flags & COLLISION_DAMAGED) {
            /* @hack spelling: away, from the pirate to player 0, formed through
             * cursors over the elements, with player 0's pos read through
             * VECTOR_FLOATS. through the members, 398 of 612 words differ */
            {
                Float *at = away.elements;
                Float *player = VECTOR_FLOATS(&player_work[0]->pos);
                Float *from = pos.elements;

                *at++ = *player++ - *from++;
                *at++ = *player++ - *from++;
                *at = *player - *from;
            }
            /* @hack type: away passed by the whole union's address. as
             * &away.vector, which the call to njCalcVector passes, SHC keeps
             * away's address in one more saved register, R10, and 541 of 612
             * words differ */
            length = njScalor((NJS_VECTOR *)&away);
            if (length > 0.01F &&
                _builtin_fabsf(pos.point.y - player_work[0]->pos.y) < 5.0F) {
                /* @hack spelling: away set through a cursor over its elements.
                 * through the members, 395 of 612 words differ */
                Float *at = away.elements;

                *at++ = 0.0F;
                *at++ = 0.0F;
                *at = pirate_a_launch_out + (((rand() * RAND_SCALE) - 0.5F) *
                                             pirate_a_launch_out_spread * 2.0F);
                njPushMatrix(&_unit_matrix_);
                TURN_VIA(njRotateY,
                         -PIRATE_VARS(work)->state->heading -
                             player_work[0]->ang.y + NJM_DEG_ANG(90),
                         ang);
                njCalcVector(NULL, &away.vector, &away.vector);
                njPopMatrix(1);
                PIRATE_VARS(work)->state->speed.vector = away.vector;
                PIRATE_VARS(work)->state->speed.vector.y +=
                    pirate_a_launch_up + (((rand() * RAND_SCALE) - 0.5F) *
                                          pirate_a_launch_up_spread * 2.0F);
                PIRATE_VARS(work)->state->spin.x =
                    (Angle)(pirate_a_spin_spread *
                            ((rand() * RAND_SCALE) - 0.5F) *
                            (2.0F * ANGLE_PER_DEGREE));
                PIRATE_VARS(work)->state->spin.y =
                    (Angle)(pirate_a_spin_spread *
                            ((rand() * RAND_SCALE) - 0.5F) *
                            (2.0F * ANGLE_PER_DEGREE));
                PIRATE_VARS(work)->state->spin.z =
                    (Angle)(pirate_a_spin_spread *
                            ((rand() * RAND_SCALE) - 0.5F) *
                            (2.0F * ANGLE_PER_DEGREE));
                ring_scatter(pos.point.x, pos.point.y + 1.0F, pos.point.z, 3);
                work->mode = PIRATE_TUMBLING;
                work->timer = 0;
            } else {
                pirate_a_break(task);
            }
            explosion_create_at(work->pos.x,
                                work->pos.y + work->collision->shapes->center.y,
                                work->pos.z, 5.0F);
            casino_knock_player_back_slow(work);
        }
        collision_register(work);
    } else {
        /* @hack spelling: speed added into pos through cursors over the
         * elements. through the members, 197 of 612 words differ */
        {
            Float *at = PIRATE_VARS(work)->state->pos.elements;
            Float *speed = PIRATE_VARS(work)->state->speed.elements;

            *at++ += *speed++;
            *at++ += *speed++;
            *at += *speed;
        }
        /* @hack spelling: speed scaled through a cursor over its elements.
         * through the members, 203 of 612 words differ */
        {
            Float *speed = PIRATE_VARS(work)->state->speed.elements;

            *speed++ *= pirate_a_drag;
            *speed++ *= pirate_a_drag;
            *speed *= pirate_a_drag;
        }
        if (PIRATE_VARS(work)->state->pos.point.y < 0.0F) {
            bounce = 0.7F + (rand() * (0.5F * RAND_SCALE));
            if (rand() * RAND_SCALE > 0.5F) {
                bounce = -bounce;
            }
            PIRATE_SCALE_SPIN(PIRATE_VARS(work)->state->spin.x, bounce);
            PIRATE_SCALE_SPIN(PIRATE_VARS(work)->state->spin.y, bounce);
            PIRATE_SCALE_SPIN(PIRATE_VARS(work)->state->spin.z, bounce);
            PIRATE_VARS(work)->state->speed.vector.y =
                -PIRATE_VARS(work)->state->speed.vector.y;
            PIRATE_VARS(work)->state->pos.point.y = 0.0F;
        } else {
            PIRATE_VARS(work)->state->speed.vector.y += pirate_a_gravity;
        }
        x = PIRATE_VARS(work)->state->speed.vector.x;
        z = PIRATE_VARS(work)->state->speed.vector.z;
        if (TASK_TIMER(work) > pirate_a_tumble_frames ||
            (PIRATE_VARS(work)->state->pos.point.y == 0.0F &&
             (flat_speed = njSqrt((x * x) + (z * z))) < pirate_a_rest_speed)) {
            pirate_a_break(task);
            return;
        }
        PIRATE_VARS(work)->state->ang.x += PIRATE_VARS(work)->state->spin.x;
        PIRATE_VARS(work)->state->ang.y += PIRATE_VARS(work)->state->spin.y;
        PIRATE_VARS(work)->state->ang.z += PIRATE_VARS(work)->state->spin.z;
        PIRATE_SCALE_SPIN(PIRATE_VARS(work)->state->spin.x, pirate_a_drag);
        PIRATE_SCALE_SPIN(PIRATE_VARS(work)->state->spin.y, pirate_a_drag);
        PIRATE_SCALE_SPIN(PIRATE_VARS(work)->state->spin.z, pirate_a_drag);
    }
}

static void pirate_a_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &pirate_a_object;
    NJS_ACTION *action = &pirate_a_action;
    Float lift;
    Angle ang;
    struct angles flat;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 7.0F)) {
        return;
    }
    lift = pirate_a_lift(work);
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &PIRATE_VARS(work)->state->origin);
    TURN_VIA(njRotateY, PIRATE_VARS(work)->state->heading, ang);
    njTranslate(NULL, PIRATE_VARS(work)->state->pos.point.x,
                PIRATE_VARS(work)->state->pos.point.y + lift,
                PIRATE_VARS(work)->state->pos.point.z);
    if (work->mode == PIRATE_WALKING) {
        TURN_VIA(njRotateY, PIRATE_VARS(work)->facing, ang);
        if (PIRATE_VARS(work)->frame == 0.0F) {
            draw_tree(object);
        } else {
            draw_action(action, PIRATE_VARS(work)->frame);
        }
    } else {
        TURN_VIA(njRotateZ, PIRATE_VARS(work)->state->ang.z, ang);
        TURN_VIA(njRotateY, PIRATE_VARS(work)->state->ang.y, ang);
        TURN_VIA(njRotateX, PIRATE_VARS(work)->state->ang.x, ang);
        draw_tree(object);
    }
    flat.x = 0;
    flat.y = 0;
    flat.z = 0;
    /* the shadow's point, 0.1 above the floor under the pirate while it
     * walks. a tumble's turns, still in the matrix, turn it too */
    pirate_a_shadow_x = 0.0F;
    pirate_a_shadow_y = -PIRATE_VARS(work)->state->pos.point.y - lift + 0.1F;
    pirate_a_shadow_z = 0.0F;
    njRotateY(NULL, NJM_DEG_ANG(90));
    shadow_draw_at(&flat, (NJS_POINT3 *)&pirate_a_shadow_x,
                   pirate_a_shadow_scale);
    njPopMatrix(1);
}
