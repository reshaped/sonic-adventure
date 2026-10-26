/* Perfect Chaos's breath beams, and the lights that rise as he breathes */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos7/boss.h"
#include "b_chaos7/breath.h"
#include "b_chaos7/chaos7_textures.h"
#include "b_chaos7/column.h"
#include "b_chaos7/fight.h"
#include "b_chaos7/ray.h"
#include "collision/collision.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT breath_object;

/* a beam destroys itself once neither the camera nor player 0 is within 7010
 * of it. the range is squared, as placement_try_leave_by takes it */
#define BREATH_RANGE (7010.0F * 7010.0F)

#define SE_BREATH 440
#define SE_BREATH_HIT 443

/* the beam's model grows along x by BREATH_GROWTH a step, from a scale of 1
 * to BREATH_LENGTH. its scale across is BREATH_WIDTH */
#define BREATH_GROWTH (1.0F / 6.0F)
#define BREATH_LENGTH 6.0F
#define BREATH_WIDTH 6.0F
/* once grown, the beam flies BREATH_SPEED a step. each step it steers, it adds
 * a pull BREATH_TURN long toward the target to its speed. it then scales the
 * sum back to BREATH_SPEED. it stops steering once its squared distance from
 * the target across the ground is under BREATH_NEAR */
#define BREATH_SPEED 20.0F
#define BREATH_TURN 1.4F
#define BREATH_NEAR (400.0F * 400.0F)
/* the beam's tip lies BREATH_TIP along its model's x axis, which the beam's
 * scale stretches. the beam ends once its tip is below y BREATH_DEPTH, under
 * the sea */
#define BREATH_TIP 60.0F
#define BREATH_DEPTH (-30.0F)
/* the scl.x the beam gives the column it raises. the column's first step sets
 * it to 0 */
#define BREATH_COLUMN 8.0F
/* the flare plays c7_ya01 to c7_ya04, textures FLARE_TEXTURE to
 * FLARE_TEXTURE + FLARE_LAST of chaos7_breath_texlist. each shows for
 * FLARE_WAIT + 1 steps */
#define FLARE_TEXTURE 51
#define FLARE_LAST 3
#define FLARE_WAIT 2

/* in work->flags, BREATH_FLARE has the beam draw its flare. the beam sets it as
 * it starts, and nothing clears it */
#define BREATH_FLARE 0x100
/* a bit of breath_object's evalflags that no Ninja flag uses. each new beam
 * clears it, and draw_tree doesn't test it */
#define BREATH_EVAL_BIT 0x100000

struct breath_vars {
    NJS_POINT3 *target;
    Sint16 flare_wait;
    Sint16 flare_frame;
    NJS_MATRIX *mouth;
};

#define VARS(work) ((struct breath_vars *)&(work)->vars)

enum breath_mode {
    BREATH_INIT,
    BREATH_GROWING,
    BREATH_STEERING,
    BREATH_FLYING
};

struct light_vars {
    Float radius; /* of the circle the light rides */
};

/* a light's pulse and size, whose product scales its sprite */
#define LIGHT_RADIUS(work) (((struct light_vars *)&(work)->vars)->radius)
#define LIGHT_PULSE(work) ((work)->scl.x)
#define LIGHT_SIZE(work) ((work)->scl.y)
/* a light's pulse rises to 1 by LIGHT_GROWTH a step as it appears. as it
 * fades, the pulse falls to 0 by as much. its circle is centered LIGHT_CENTER
 * from Chaos toward player 0. the circle shrinks to a radius of LIGHT_INNER, no
 * less */
#define LIGHT_GROWTH (1.0F / 30.0F)
#define LIGHT_CENTER 200.0F
#define LIGHT_INNER 50.0F
/* a light starts with a radius from breath_light_ring to LIGHT_RING_SPREAD
 * more. its pulse's phase turns LIGHT_PULSE_TURN to twice that a step */
#define LIGHT_RING_SPREAD 50.0F
#define LIGHT_PULSE_TURN 4096.0F

enum light_mode { LIGHT_GROWING, LIGHT_RISING, LIGHT_FADING };

/* a light rises breath_light_rise a step, and its circle's radius shrinks by
 * breath_light_close. it fades once it reaches the height breath_light_top */
#pragma section HEAD
static Float breath_light_rise = 5.0F;
static Float breath_light_close = 1.2F;
static Float breath_light_top = 530.0F;
static Float breath_light_ring = 150.0F;
#pragma section

static const Uint8 breath_shape_count = 1;
/* both points are the origin. njCalcPoint turns a copy of the first into the
 * beam's tip. breath_light_step sets a copy of the second's x to the light's
 * radius. njCalcPoint then turns that copy into the light's offset from its
 * circle's center */
static const NJS_POINT3 breath_origin = {0.0F, 0.0F, 0.0F};
static const NJS_POINT3 breath_light_origin = {0.0F, 0.0F, 0.0F};

static NJS_MATERIAL breath_flare_materials[1] = {
#include "b_chaos7/breath/breath_flare_materials.inc"
};

static Sint16 breath_flare_meshes[59] = {
#include "b_chaos7/breath/breath_flare_meshes.inc"
};

static NJS_COLOR breath_flare_uvs[51] = {
#include "b_chaos7/breath/breath_flare_uvs.inc"
};

static NJS_MESHSET breath_flare_meshsets[1] = {
#include "b_chaos7/breath/breath_flare_meshsets.inc"
};

static NJS_POINT3 breath_flare_points[21] = {
#include "b_chaos7/breath/breath_flare_points.inc"
};

static NJS_VECTOR breath_flare_normals[21] = {
#include "b_chaos7/breath/breath_flare_normals.inc"
};

static NJS_MODEL breath_flare_model = {
#include "b_chaos7/breath/breath_flare_model.inc"
};

static NJS_OBJECT breath_flare_object = {
#include "b_chaos7/breath/breath_flare_object.inc"
};

/* a cylinder 36 in radius and 600 long, centered on the beam's position. its
 * turn of 90 degrees about z lays its axis along the beam's x */
static struct collision_shape breath_shape[1] = {
    {SHAPE_CYLINDER2 | SHAPE_TURN_YZX | SHAPE_HURTABLE,
     ATTACK(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     36.0F,
     300.0F,
     0.0F,
     {0, 0, NJM_DEG_ANG(90)},
     0,
     NULL},
};

/* the beam's place and speed in the mouth matrix's frame while it grows. the
 * beam doesn't move until it steers. the speed, 1.4 long in that frame, then
 * weighs against the pull of the first step it steers */
static NJS_POINT3 breath_mouth_pos = {0.0F, 0.0F, 0.0F};
static NJS_VECTOR breath_mouth_dir = {1.4F, 0.0F, 0.0F};

/* c7_breathflash, texture 55 of chaos7_breath_texlist */
static NJS_TEXANIM breath_light_frame = {
#include "b_chaos7/breath/breath_light_frame.inc"
};
static NJS_SPRITE breath_light_sprite = {
#include "b_chaos7/breath/breath_light_sprite.inc"
};

/* on a frame's first step, draw the beam and its flare. the flare is at the
 * tip, mirrored along the beam's x */
static void breath_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&chaos7_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
        draw_tree(&breath_object);
        if (work->flags & BREATH_FLARE) {
            njSetTexture(&chaos7_breath_texlist);
            /* @hack spelling: the flare's model read at 4 past the object's
             * address (MOV.L @(4,R3),R2 at 0x0c918076), whose pool word
             * draw_tree's argument shares; as a plain member access, SHC pools
             * the model member's own address, and 7 of 91 words and 5 pool
             * words differ */
            (&breath_flare_object)->model->mats[0].attr_texId =
                VARS(work)->flare_frame + FLARE_TEXTURE;
            njScale(NULL, -1.0F, 1.0F, 1.0F);
            njTranslate(NULL, -BREATH_TIP, 0.0F, 0.0F);
            draw_tree(&breath_flare_object);
        }
        njPopMatrix(1);
    }
}

/* grow the beam from the mouth, then fly it at BREATH_SPEED. it steers at the
 * target until it is near. when the beam hits a player, it knocks player 0
 * back. once its tip is below BREATH_DEPTH, the beam raises a column and
 * ends */
static void breath_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_MATRIX *mouth = VARS(work)->mouth;
    NJS_POINT3 *target = VARS(work)->target;
    NJS_POINT3 to;
    NJS_POINT3 tip;
    Float reach;
    Float turn;
    Float across;
    Float scale;
    struct task *column;
    Angle ang;

    switch (work->mode) {
    case BREATH_INIT:
        task->draw = breath_draw;
        work->scl.x = 1.0F;
        work->scl.y = work->scl.z = BREATH_WIDTH;
        work->flags |= BREATH_FLARE;
        work->mode = BREATH_GROWING;
        /* falls through, so the beam starts growing on this step */
    case BREATH_GROWING:
        njPushMatrix(mouth);
        njCalcPoint(NULL, &breath_mouth_pos, &work->pos);
        njCalcVector(NULL, &breath_mouth_dir, &motion->speed);
        njPopMatrix(1);
        work->scl.x += BREATH_GROWTH;
        if (work->scl.x >= BREATH_LENGTH) {
            work->flags |= BREATH_FLARE;
            sound_play(SE_BREATH, 0, 0, chaos7_volume);
            work->mode = BREATH_STEERING;
        }
        break;
    case BREATH_STEERING:
        /* @hack redundant: a test of chaos7_part_2 whose two branches set the
         * same, which keeps its load and TST (0x0c918228, 0x0c918230);
         * without it, 291 of 392 words differ */
        if (chaos7_part_2) {
            turn = BREATH_TURN;
            reach = BREATH_NEAR;
        } else {
            turn = BREATH_TURN;
            reach = BREATH_NEAR;
        }
        to = *target;
        njSubVector(&to, &work->pos);
        across = (to.x * to.x) + (to.z * to.z);
        scale = turn / njSqrt(across + (to.y * to.y));
        to.x *= scale;
        to.y *= scale;
        to.z *= scale;
        njAddVector(&motion->speed, &to);
        njUnitVector(&motion->speed);
        motion->speed.x *= BREATH_SPEED;
        motion->speed.y *= BREATH_SPEED;
        motion->speed.z *= BREATH_SPEED;
        if (across < reach) {
            work->mode = BREATH_FLYING;
        }
        njAddVector(&work->pos, &motion->speed);
        break;
    case BREATH_FLYING:
        njAddVector(&work->pos, &motion->speed);
        break;
    }
    if (work->collision->flags & COLLISION_HIT) {
        if (work->collision->hit->kind == COLLISION_PLAYER) {
            chaos7_knock_player_back();
        }
    }
    to = breath_origin;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
    njTranslate(NULL, BREATH_TIP, 0.0F, 0.0F);
    njCalcPoint(NULL, &to, &tip);
    njPopMatrix(1);
    if (tip.y < BREATH_DEPTH) {
        if ((column = task_create(TASK_WORK, 3, chaos7_column_step)) != NULL) {
            column->work->pos = tip;
            column->work->pos.y = 0.0F;
            column->work->scl.x = BREATH_COLUMN;
        }
        sound_play_at(SE_BREATH_HIT, 0, 0, 120, tip.x, tip.y, tip.z);
        task_kill(task);
        return;
    }
    if (++VARS(work)->flare_wait > FLARE_WAIT) {
        VARS(work)->flare_wait = 0;
        if (++VARS(work)->flare_frame > FLARE_LAST) {
            VARS(work)->flare_frame = 0;
        }
    }
    work->ang.y =
        NJM_DEG_ANG(180) -
        (Angle)(atan2f(motion->speed.z, motion->speed.x) * ANGLE_PER_RADIAN);
    work->ang.z = NJM_DEG_ANG(180) -
                  (Angle)(atan2f(motion->speed.y,
                                 njSqrt((motion->speed.x * motion->speed.x) +
                                        (motion->speed.z * motion->speed.z))) *
                          ANGLE_PER_RADIAN);
    breath_draw(task);
    collision_register(work);
    /* @hack redundant: a return on placement_try_leave_by's result at the
     * function's end, which keeps the result's TST R0,R0 at 0x0c9184c4;
     * without it, 13 of 392 words differ */
    if (placement_try_leave_by(task, BREATH_RANGE)) {
        return;
    }
}

void chaos7_breath_create(NJS_MATRIX *mouth, NJS_POINT3 *target)
{
    struct task *task;
    struct task_work *work;

    task = task_create(TASK_WORK | TASK_MOTION, 2, breath_step);
    if (task != NULL) {
        collision_init(task, breath_shape, COLLISION_OBJECT, 1,
                       breath_shape_count);
        /* @hack spelling: breath_object reached through its address, which
         * leaves R0 to the load of mouth (MOV.L @(4,R15),R0 at 0x0c918558);
         * without it, the load takes R3, and 2 of 37 words differ */
        (&breath_object)->evalflags &= ~BREATH_EVAL_BIT;
        work = task->work;
        VARS(work)->mouth = mouth;
        VARS(work)->target = target;
    }
}

/* on a frame's first step, draw a light, added to the color behind it */
static void breath_light_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;

    if (frame_step == 0) {
        /* @hack spelling: the alpha stored apart and first, so SHC loads the
         * 1.0F into FR4 (FLDI1 FR4 at 0x0c91857c). as one chained store,
         * color.r = color.g = color.b = color.a = 1.0F, the 1.0F takes FR3,
         * and 5 of 56 words differ */
        color.a = 1.0F;
        color.r = color.g = color.b = 1.0F;
        njSetConstantMaterial(&color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        breath_light_sprite.sx = breath_light_sprite.sy =
            LIGHT_PULSE(work) * LIGHT_SIZE(work);
        njDrawSprite3D(&breath_light_sprite, 0,
                       NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    }
}

/* grow a light, pulse it as it rises, and fade it out at breath_light_top. it
 * rides a circle round a point between Chaos and player 0, and the circle
 * closes in */
static void breath_light_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 out;
    NJS_POINT3 turned;
    NJS_POINT3 center;
    Angle toward;
    Angle ang;
    Float radius;
    Float x;
    Float z;
    struct task_work *player;

    switch (work->mode) {
    case LIGHT_GROWING:
        LIGHT_PULSE(work) += LIGHT_GROWTH;
        if (LIGHT_PULSE(work) >= 1.0F) {
            work->mode = LIGHT_RISING;
        }
        break;
    case LIGHT_RISING:
        work->ang.x += work->ang.y;
        LIGHT_PULSE(work) = (njSin(work->ang.x) * 0.5F) + 1.0F;
        if (work->pos.y >= breath_light_top) {
            work->mode = LIGHT_FADING;
        }
        break;
    case LIGHT_FADING:
        LIGHT_PULSE(work) -= LIGHT_GROWTH;
        if (LIGHT_PULSE(work) <= 0.0F) {
            task_destroy(task);
            return;
        }
        break;
    }
    work->pos.y += breath_light_rise;
    LIGHT_RADIUS(work) =
        (radius = LIGHT_RADIUS(work) - breath_light_close) > LIGHT_INNER
            ? radius
            : LIGHT_INNER;
    njPushMatrix(&_unit_matrix_);
    out = breath_light_origin;
    player = player_work[0];
    x = boss_work->pos.x - player->pos.x;
    z = boss_work->pos.z - player->pos.z;
    toward = (Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
    center.x = boss_work->pos.x - (njCos(toward) * LIGHT_CENTER);
    center.z = boss_work->pos.z - (njSin(toward) * LIGHT_CENTER);
    TURN_VIA(njRotateY, work->ang.z, ang);
    out.x = LIGHT_RADIUS(work);
    njCalcPoint(NULL, &out, &turned);
    work->pos.x = center.x + turned.x;
    work->pos.z = center.z + turned.z;
    njPopMatrix(1);
    breath_light_draw(task);
}

void chaos7_breath_light_start(struct task *task)
{
    struct task_work *work = task->work;

    task->step = breath_light_step;
    task->draw = breath_light_draw;
    LIGHT_PULSE(work) = 0.0F;
    LIGHT_SIZE(work) = (rand() * RAND_SCALE) + 0.5F;
    work->ang.x = 0;
    work->ang.y =
        (Angle)((rand() * RAND_SCALE * LIGHT_PULSE_TURN) + LIGHT_PULSE_TURN);
    LIGHT_RADIUS(work) =
        breath_light_ring + (rand() * RAND_SCALE * LIGHT_RING_SPREAD);
    work->ang.z = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    work->pos.y = 0.0F;
}
