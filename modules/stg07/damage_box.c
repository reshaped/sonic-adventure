/* two boxes and UkiUki. O R DBOX and O RNDBOX rise out of the ground and
 * sink back, over and over. O R DBOX hurts while it rises and sinks, and
 * lets off steam toward the camera. it throws up a ring of dust just before
 * it rises again. O RNDBOX hurts nothing, and throws up dust as it rises.
 * UkiUki is a model in the world */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "object/common.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg07/damage_box.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL damage_box_model;
extern NJS_MODEL no_damage_box_model;
extern NJS_OBJECT ukiuki_object;

struct damage_box_vars {
    /* BOX_HALF_HEIGHT as the box starts, and nothing reads it */
    Float height;
    Uint8 unk_04[4];
    Float ringed; /* 1 once it has thrown up its ring this cycle, else 0 */
};

#define VARS(work) ((struct damage_box_vars *)&(work)->vars)

/* how far the box has risen */
#define BOX_LIFT(work) ((work)->scl.y)
/* the most it rises */
#define BOX_RISE 20.0F
/* its shape's half height at rest. as the box rises, the half height is its
 * lift plus this */
#define BOX_HALF_HEIGHT 4.0F

enum damage_box_mode { BOX_START, BOX_RUN, BOX_UNUSED, BOX_STILL };

/* a cycle's timer counts the degrees of a sine that lifts the box. it runs
 * from 0 until it passes CYCLE_END. up to CYCLE_REST, it gains 2 a step,
 * while the box rises and sinks. past CYCLE_REST, it gains 1 a step, while
 * the box rests. O R DBOX throws up its ring once the timer passes
 * CYCLE_RING */
#define CYCLE_REST 180
#define CYCLE_RING 330
#define CYCLE_END 360

/* O R DBOX lets off steam every STEAM_STEPS steps while it rises and sinks.
 * O RNDBOX throws up a ring every RING_STEPS steps while its timer is below
 * CYCLE_RINGS_END */
#define STEAM_STEPS 10
#define RING_STEPS 20
#define CYCLE_RINGS_END 90

/* a ring's puffs lie RING_GAP degrees apart, up to RING_SPREAD either way
 * of the camera's direction */
#define RING_GAP 40
#define RING_SPREAD 90

/* how far from the box's middle its rings and steam start */
#define RING_REACH 14.3F
#define STEAM_REACH 13.3F

/* each box's sound as it starts to rise */
#define SE_DAMAGE_BOX 202
#define SE_NO_DAMAGE_BOX 193

#define NO_DAMAGE_RANGE 400.0F

/* clang-format off */
static struct collision_shape no_damage_box_shape = {
    SHAPE_CYLINDER2, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F},
    10.0F, BOX_HALF_HEIGHT, 10.0F, {0}, 0, NULL,
};
static struct collision_shape damage_box_shape = {
    SHAPE_CYLINDER2 | SHAPE_HURTABLE, ATTACK(2) | SHAPE_HURTS_PLAYERS,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 10.0F,
    BOX_HALF_HEIGHT, 0.0F, {0}, 0, NULL,
};
/* clang-format on */

static struct dust_params damage_box_ring = {
    1.0F,
    0.23F,
    0.17F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* clang-format off */
static struct collision_shape damage_box_unused_shape = {
    SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F},
    25.0F, 20.0F, 25.0F, {0}, 0, NULL,
};
/* clang-format on */

static void damage_box_draw(struct task *task);
static void damage_box_destroy(struct task *task);
static void damage_box_move(struct task *task);
static void no_damage_box_move(struct task *task);
static void damage_box_ring_create(struct task *task, int unused);
static void damage_box_steam_create(NJS_POINT3 *pos);
static void damage_box_steam_step(struct particle *particle,
                                  struct particle_pool *pool);
static void no_damage_box_draw(struct task *task);
static void no_damage_box_destroy(struct task *task);

void damage_box_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BOX_START:
        collision_init(task, &damage_box_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = damage_box_destroy;
        task->draw = damage_box_draw;
        work->mode = BOX_RUN;
        VARS(work)->height = BOX_HALF_HEIGHT;
        TASK_TIMER(work) = 0;
        break;
    case BOX_RUN:
        if (!placement_editing(task)) {
            damage_box_move(task);
            collision_register(work);
        }
        /* the labels name scl.x and scl.z as speeds, which the move never
         * reads */
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<-UP SPD");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<-DOWN SPD");
        }
        damage_box_draw(task);
        break;
    case BOX_STILL:
        break;
    }
}

static void damage_box_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, 0.0F, BOX_LIFT(work), 0.0F);
    draw_model(&damage_box_model);
    njPopMatrix(1);
}

static void damage_box_destroy(struct task *task)
{
}

/* while the box rises and sinks, its shape attacks at level 2, and it lets
 * off steam. its shape's half height is its lift plus BOX_HALF_HEIGHT */
static void damage_box_move(struct task *task)
{
    struct task_work *work = task->work;
    Float height;

    if (TASK_TIMER(work) <= CYCLE_REST) {
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_ATTACK) | ATTACK(2);
        if (TASK_TIMER(work) == 0) {
            sound_play_at(SE_DAMAGE_BOX, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        TASK_TIMER(work) += 2;
        work->tick++;
        if (work->tick >= STEAM_STEPS) {
            NJS_POINT3 top = {0.0F, -5.0F, 0.0F};
            NJS_POINT3 pos;
            Angle ang;

            njPushMatrix(&_unit_matrix_);
            njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            njTranslate(NULL, 0.0F, BOX_LIFT(work), 0.0F);
            njCalcPoint(NULL, &top, &pos);
            njPopMatrix(1);
            damage_box_steam_create(&pos);
            work->tick = 0;
        }
    } else {
        /* @hack spelling: the mask written out, which reads levels through
         * R0's index 4 (MOV #4,R0 and MOV.B @(R0,R4),R0 at 0x0c9081a2 and
         * 0x0c9081a6). with &=, SHC reads it at @(4,R2), the function runs 2
         * bytes short and 82 of 189 words differ */
        work->collision->shapes->levels =
            work->collision->shapes->levels & ~SHAPE_ATTACK;
        TASK_TIMER(work)++;
    }
    if (TASK_TIMER(work) > CYCLE_RING && VARS(work)->ringed == 0.0F) {
        damage_box_ring_create(task, 1);
        VARS(work)->ringed = 1.0F;
    }
    if (TASK_TIMER(work) > CYCLE_END) {
        TASK_TIMER(work) = 0;
        VARS(work)->ringed = 0.0F;
    }
    BOX_LIFT(work) = njSin(TASK_TIMER(work) * ANGLE_PER_DEGREE) * BOX_RISE;
    if (BOX_LIFT(work) < 0.0F) {
        BOX_LIFT(work) = 0.0F;
    }
    height = BOX_LIFT(work) * 0.25F;
    if (height <= -1.0F) {
        height = -0.9F;
    }
    work->collision->shapes->size_y = (height + 1.0F) * BOX_HALF_HEIGHT;
}

/* damage_box_move's rise, without the attack or the steam. it throws up a
 * ring every RING_STEPS steps while its timer is below CYCLE_RINGS_END */
static void no_damage_box_move(struct task *task)
{
    struct task_work *work = task->work;
    Float height;

    if (TASK_TIMER(work) <= CYCLE_REST) {
        if (TASK_TIMER(work) == 0) {
            sound_play_at(SE_NO_DAMAGE_BOX, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        TASK_TIMER(work) += 2;
        if (TASK_TIMER(work) < CYCLE_RINGS_END) {
            work->tick++;
            if (work->tick >= RING_STEPS) {
                damage_box_ring_create(task, 0);
                work->tick = 0;
            }
        }
    } else {
        TASK_TIMER(work)++;
    }
    if (TASK_TIMER(work) > CYCLE_END) {
        TASK_TIMER(work) = 0;
    }
    BOX_LIFT(work) = njSin(TASK_TIMER(work) * ANGLE_PER_DEGREE) * BOX_RISE;
    if (BOX_LIFT(work) < 0.0F) {
        BOX_LIFT(work) = 0.0F;
    }
    height = BOX_LIFT(work) * 0.25F;
    if (height <= -1.0F) {
        height = -0.9F;
    }
    work->collision->shapes->size_y = (height + 1.0F) * BOX_HALF_HEIGHT;
}

/* six puffs of dust round the half of the box that faces the camera. three
 * lie 0, 40 and 80 degrees one way from the camera's direction. the others
 * lie 10, 50 and 90 degrees the other way. nothing reads unused, which is 1
 * from O R DBOX and 0 from O RNDBOX */
static void damage_box_ring_create(struct task *task, int unused)
{
    struct task_work *work = task->work;
    /* @hack redundant: speed, which nothing reads, copied into the frame
     * from its constant (ADD #-16,R15 at 0x0c908394, then the copy from
     * 0x0c908398); without it, 252 of 262 words differ */
    NJS_VECTOR speed = {0.0F, 0.21F, 0.0F};
    Angle toward;
    Angle ang;
    int i;

    toward = (Angle)(atan2f(camera_work->pos.x - work->pos.x,
                            camera_work->pos.z - work->pos.z) *
                     ANGLE_PER_RADIAN) -
             0x4000;
    /* @hack spelling: ang set inside each njCos call, which computes it in R4
     * and stores it to the frame from there (0x0c9083f4 to 0x0c9083fc); set
     * apart, it takes R3, and 123 of 262 words differ */
    for (i = 0; i < RING_SPREAD; i += RING_GAP) {
        damage_box_ring.pos.x =
            work->pos.x +
            (njCos(ang = toward + (Angle)(i * ANGLE_PER_DEGREE)) * RING_REACH);
        damage_box_ring.pos.y = work->pos.y;
        damage_box_ring.pos.z = work->pos.z - (njSin(ang) * RING_REACH);
        dust_create_from(&damage_box_ring);
    }
    for (i = RING_SPREAD; i > 0; i -= RING_GAP) {
        damage_box_ring.pos.x =
            work->pos.x +
            (njCos(ang = toward - (Angle)(i * ANGLE_PER_DEGREE)) * RING_REACH);
        damage_box_ring.pos.y = work->pos.y;
        damage_box_ring.pos.z = work->pos.z - (njSin(ang) * RING_REACH);
        dust_create_from(&damage_box_ring);
    }
}

/* a puff of steam at pos, drifting out toward the camera. within 300 of the
 * camera, its scale grows from 0.225 to 0.225625 with the distance. beyond
 * 300, it is 0.26 */
static void damage_box_steam_create(NJS_POINT3 *pos)
{
    struct particle *particle =
        particle_alloc(bomb_pool.pool, damage_box_steam_step);
    Angle ang;
    Float distance;
    Float scale;

    if (particle == NULL) {
        return;
    }
    ang = (Angle)(atan2f(camera_work->pos.x - pos->x,
                         camera_work->pos.z - pos->z) *
                  ANGLE_PER_RADIAN) -
          0x4000;
    ang += (Angle)(rand() * RAND_SCALE * 2048.0F) - 1024;
    particle->speed.x = njCos(ang) * 0.4F;
    particle->speed.z = -njSin(ang) * 0.4F;
    particle->pos.x = pos->x + (njCos(ang) * STEAM_REACH);
    particle->pos.z = pos->z - (njSin(ang) * STEAM_REACH);
    particle->speed.y = rand() * RAND_SCALE * 0.22F;
    particle->pos.y = pos->y;
    distance = njDistanceP2P(&camera_work->pos, pos);
    if (distance > 300.0F) {
        scale = 0.035F;
    } else {
        scale = distance * 2.0833333e-6F;
    }
    particle->scale = scale + 0.225F;
    particle->time = rand() * RAND_SCALE * 6.0F;
    particle->ang = (Angle)(rand() * RAND_SCALE * 8192.0F) - 4096;
    particle->color.a = 1.0F;
    particle->color.r = 1.0F;
    particle->color.g = 0.8F;
    particle->color.b = 0.8F;
}

/* a puff plays its animation once, then dies. its drag takes its speed
 * toward 0 along x and z, and toward 0.4 a step up */
static void damage_box_steam_step(struct particle *particle,
                                  struct particle_pool *pool)
{
    Float drag = 0.05F;

    particle->frame = floorf(particle->time);
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = (Sint16)pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    particle->time += 0.33F;
    /* @hack spelling: the drag's products for x and z negated and added,
     * FNEG and FADD at 0x0c90885a and 0x0c90885c, and at 0x0c90888a and
     * 0x0c90888c; with -=, SHC subtracts, the function runs 4 bytes short and
     * 57 of 105 words differ */
    particle->speed.x += -particle->speed.x * drag;
    particle->speed.y += (-particle->speed.y * drag) + 0.02F;
    particle->speed.z += -particle->speed.z * drag;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    particle->scale += 0.0005F;
}

/* O RNDBOX runs while the camera or player 0 is within NO_DAMAGE_RANGE.
 * O R DBOX takes its placement's range instead */
void no_damage_box_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, NO_DAMAGE_RANGE * NO_DAMAGE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case BOX_START:
        collision_init(task, &no_damage_box_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = no_damage_box_destroy;
        task->draw = no_damage_box_draw;
        work->mode = BOX_RUN;
        VARS(work)->height = BOX_HALF_HEIGHT;
        TASK_TIMER(work) = 0;
        break;
    case BOX_RUN:
        if (!placement_editing(task)) {
            no_damage_box_move(task);
            collision_register(work);
        }
        /* the labels name scl.x and scl.z as speeds, which the move never
         * reads */
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<-UP SPD");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<-DOWN SPD");
        }
        no_damage_box_draw(task);
        break;
    case BOX_STILL:
        break;
    }
}

static void no_damage_box_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, 0.0F, BOX_LIFT(work), 0.0F);
    draw_model(&no_damage_box_model);
    njPopMatrix(1);
}

static void no_damage_box_destroy(struct task *task)
{
}

void ukiuki_step(struct task *task)
{
    struct task_work *work = task->work;

    WORLD_MODEL(work)->model = &ukiuki_object;
    model_start_world(task);
}
