/* O RRAF, a wheel that turns about its x axis. two rings of horns stand on
 * it. while no horn moves, the wheel picks a horn number at random. the
 * horns whose number is that one, or a quarter of it rounded down, rise and
 * spray. odd horns spray flame and hurt players while up, and even ones
 * spray smoke */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg07/rraf.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT rraf_wheel_object;
extern NJS_MODEL rraf_base_model;
/* the wheel as the world tests it */
/* @hack type: an array of one, whose address rraf_add_world loads from its
 * pool to read the model 4 past it (MOV.L @(4,R3),R2 at 0x0c903f8a). as a
 * single object, 10 of rraf_add_world's 46 words and 1 word of its pool
 * differ */
extern NJS_OBJECT rraf_world_object[1];
extern NJS_MODEL rraf_hot_horn_model;
extern NJS_MODEL rraf_horn_model;

struct rraf_vars {
    NJS_OBJECT *world;
};

struct horn_vars {
    NJS_MODEL *model; /* set as the horn is made, and read by no code */
    Float unk_04; /* zeroed as the horn comes to rest, and read by no code */
};

#define VARS(work) ((struct rraf_vars *)&(work)->vars)
#define HORN_VARS(work) ((struct horn_vars *)&(work)->vars)

/* the wheel's scale. RRAF_CHOSEN is the horn number picked last. each ring
 * has RRAF_HORNS horns, rounded up. the wheel's x angle grows while RRAF_WAY
 * is 0 or more, and shrinks while it's negative */
#define RRAF_CHOSEN(work) ((work)->scl.x)
#define RRAF_HORNS(work) ((work)->scl.y)
#define RRAF_WAY(work) ((work)->scl.z)
/* a horn's scale. HORN_HEIGHT is how far it has risen. SPRAY_TIMER counts its
 * steps up to SPRAY_STEPS, then starts over. a moving horn sprays as it reaches
 * SPRAY_STEPS */
#define HORN_HEIGHT(work) ((work)->scl.y)
#define SPRAY_TIMER(work) ((work)->scl.z)

/* a horn's number, in its tick: its index in its ring, from its flags */
#define HORN_NUMBER(work) ((work)->tick)
/* odd horns are hot: they spray flame and hurt players. even ones smoke */
#define HORN_HOT(work) (HORN_NUMBER(work) % 2)

/* the work mode of the wheel and of each horn. no code sets RRAF_IDLE */
enum rraf_mode { RRAF_INIT, RRAF_RUN, RRAF_IDLE };

/* the two sounds the wheel plays while it runs, which it refreshes each
 * step */
#define SE_RRAF_1 203
#define SE_RRAF_2 193

/* how far along the wheel's axis each ring of horns stands from its
 * middle */
#define INNER_RING 52.0F
#define OUTER_RING 152.0F

/* the wheel stays placed while the camera or player 0 is within 1000 of
 * it. placement_try_leave_by takes the range squared */
#define RRAF_RANGE (1000.0F * 1000.0F)
/* the world tests the wheel while player 0 or 1 is within RRAF_REACH past
 * its model's radius */
#define RRAF_REACH 200.0F

/* the wheel and its horns turn RRAF_TURN a step about x */
#define RRAF_TURN 15

/* a horn neither runs nor draws while the camera is within
 * HORN_CAMERA_NEAR of it */
#define HORN_CAMERA_NEAR 50.0F
/* a horn stands up to HORN_RISE high. it rises HORN_RISE_STEP degrees of
 * its timer a step, and sprays every SPRAY_STEPS steps */
#define HORN_RISE 20.0F
#define HORN_RISE_STEP 4
#define SPRAY_STEPS 10.0F
/* a hot horn's attack level while it's up */
#define HOT_HORN_ATTACK 2
/* a spray starts at the horn's tip, HORN_TIP along it. a flame starts
 * FIRE_REACH toward the camera from there, and smoke SMOKE_REACH */
#define HORN_TIP 38.0F
#define FIRE_REACH 13.3F
#define SMOKE_REACH 14.3F
/* a horn's shape's half height is SHAPE_HEIGHT at rest. it grows by
 * SHAPE_GROWTH of that for each unit the horn has risen */
#define SHAPE_HEIGHT 5.0F
#define SHAPE_GROWTH 0.2F
/* a flame moves FLAME_SPEED a step toward the camera */
#define FLAME_SPEED 0.4F
/* a flame's scale is FLAME_SCALE plus a part that grows with its distance from
 * the camera. beyond FLAME_FAR, the part is FLAME_FAR_SCALE. within it, the
 * part is FLAME_NEAR_SCALE times the share of FLAME_FAR the distance is. that
 * never comes near FLAME_FAR_SCALE */
#define FLAME_SCALE 0.225F
#define FLAME_FAR 300.0F
#define FLAME_FAR_SCALE 0.035F
#define FLAME_NEAR_SCALE 0.000625F

static void rraf_horn_step(struct task *task);

/* eight horns to a ring, as specs for task_create_children. no word in the
 * image or in 1ST_READ.BIN points at the table */
/* clang-format off */
#define HORN(number, ang, ring, model)                                         \
    {rraf_horn_step, TASK_WORK | TASK_FORCE, (number), {(ang), 0, 0},          \
     {(ring), 0.0F, 0.0F}, (Sint32)&(model)}
/* clang-format on */
static struct task_spec rraf_horns[17] = {
    HORN(0, 0x071c, INNER_RING, rraf_hot_horn_model),
    HORN(1, 0x271c, INNER_RING, rraf_horn_model),
    HORN(2, 0x471c, INNER_RING, rraf_hot_horn_model),
    HORN(3, 0x671c, INNER_RING, rraf_horn_model),
    HORN(4, 0x871c, INNER_RING, rraf_hot_horn_model),
    HORN(5, 0xa71c, INNER_RING, rraf_horn_model),
    HORN(6, 0xc71c, INNER_RING, rraf_hot_horn_model),
    HORN(7, 0xe71c, INNER_RING, rraf_horn_model),
    HORN(5, 0x071c, OUTER_RING, rraf_hot_horn_model),
    HORN(4, 0x271c, OUTER_RING, rraf_horn_model),
    HORN(2, 0x471c, OUTER_RING, rraf_hot_horn_model),
    HORN(3, 0x671c, OUTER_RING, rraf_horn_model),
    HORN(1, 0x871c, OUTER_RING, rraf_hot_horn_model),
    HORN(7, 0xa71c, OUTER_RING, rraf_horn_model),
    HORN(6, 0xc71c, OUTER_RING, rraf_hot_horn_model),
    HORN(0, 0xe71c, OUTER_RING, rraf_horn_model),
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

/* rraf_destroy clears it. no code reads it */
static Uint32 dat_0ca57e34 = 0;

/* a horn's shape, and a hot horn's, which hurts players while it's up */
/* @hack type: each an array of one, so rraf_horn_step makes two
 * collision_init calls (JSR at 0x0c903fe4 and 0x0c903ff8). as plain
 * structs, SHC merges them into one, and 65 of rraf_horn_step's 79 words
 * differ */
/* clang-format off */
static struct collision_shape rraf_horn_shape[1] = {
    {SHAPE_CYLINDER2, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 34.0F, 0.0F},
    7.5F, 5.0F, 0.0F, {0}, 0, NULL},
};
static struct collision_shape rraf_hot_horn_shape[1] = {
    {SHAPE_CYLINDER2 | SHAPE_HURTABLE, HOT_HORN_ATTACK | SHAPE_HURTS_PLAYERS,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 34.0F, 0.0F},
    7.5F, 2.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* clang-format off */
static struct dust_params rraf_smoke = {
    1.0F, 0.23F, 0.17F, 0.06F, 0.025F,
    {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F, 1.0F},
};
/* clang-format on */

static void rraf_move_world(struct task *task);
static void rraf_draw(struct task *task);
static void rraf_destroy(struct task *task);
static void rraf_add_world(struct task *task);
static void rraf_horn_draw(struct task *task);
static void rraf_horn_destroy(struct task *task);
static void rraf_horn_run(struct task *task);
static void rraf_fire_create(NJS_POINT3 *pos);
static void rraf_fire_step(struct particle *particle,
                           struct particle_pool *pool);
static void rraf_smoke_create(NJS_POINT3 *pos);

void rraf_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *horn;
    int i;
    int count;
    Angle ang;
    NJS_VECTOR pos;
    NJS_VECTOR offset;

    if (placement_try_leave_by(task, RRAF_RANGE)) {
        return;
    }
    switch (work->mode) {
    case RRAF_INIT:
        task->destructor = rraf_destroy;
        task->draw = rraf_draw;
        rraf_add_world(task);
        work->tick = 0;
        work->mode = RRAF_RUN;
        work->ang.y = 0;
        if (placement_editing(task)) {
            break;
        }
        count = (int)ceilf(RRAF_HORNS(work));
        for (i = 0; i < count; i++) {
            /* @hack spelling: child assigned in the test, which SHC tests in R0
             * (TST R0,R0 at 0x0c903a74) and copies to R4 in the branch's
             * delay slot. assigned in a statement of its own, 3 of
             * rraf_step's 403 words differ */
            if ((child = task_create_child(TASK_WORK | TASK_FORCE,
                                           rraf_horn_step, task)) != NULL) {
                horn = child->work;
                /* the horn's place round the ring, in whole degrees. the
                 * inner ring stands a quarter turn on from the outer */
                /* NOLINTBEGIN(bugprone-integer-division) */
                horn->ang.x =
                    (Angle)(((i * 360 / count) + 90) * ANGLE_PER_DEGREE);
                /* NOLINTEND(bugprone-integer-division) */
                horn->ang.y = 0;
                horn->ang.z = work->ang.z;
                offset.x = INNER_RING;
                offset.y = 0.0F;
                offset.z = 0.0F;
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN_VIA(njRotateZ, work->ang.z, ang);
                TURN_VIA(njRotateX, work->ang.x, ang);
                TURN_VIA(njRotateY, work->ang.y, ang);
                njCalcVector(NULL, &offset, &pos);
                njAddVector(&pos, &work->pos);
                horn->pos = pos;
                njPopMatrix(1);
                if (i % 2 == 0) {
                    HORN_VARS(horn)->model = &rraf_hot_horn_model;
                } else {
                    HORN_VARS(horn)->model = &rraf_horn_model;
                }
                horn->flags = (Uint8)i;
            }
        }
        for (i = 0; i < count; i++) {
            /* @hack spelling: child assigned in the test, which SHC tests in R0
             * (TST R0,R0 at 0x0c903bcc) and copies to R4 in the branch's
             * delay slot. assigned in a statement of its own, 3 of
             * rraf_step's 403 words differ */
            if ((child = task_create_child(TASK_WORK | TASK_FORCE,
                                           rraf_horn_step, task)) != NULL) {
                horn = child->work;
                /* NOLINTNEXTLINE(bugprone-integer-division) */
                horn->ang.x = (Angle)((i * 360 / count) * ANGLE_PER_DEGREE);
                horn->ang.y = 0;
                horn->ang.z = work->ang.z;
                offset.x = OUTER_RING;
                offset.y = 0.0F;
                offset.z = 0.0F;
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN_VIA(njRotateZ, work->ang.z, ang);
                TURN_VIA(njRotateX, work->ang.x, ang);
                TURN_VIA(njRotateY, work->ang.y, ang);
                njCalcVector(NULL, &offset, &pos);
                njAddVector(&pos, &work->pos);
                horn->pos = pos;
                njPopMatrix(1);
                if (i % 2 == 0) {
                    HORN_VARS(horn)->model = &rraf_hot_horn_model;
                } else {
                    HORN_VARS(horn)->model = &rraf_horn_model;
                }
                horn->flags = (Uint8)i;
            }
        }
        break;
    case RRAF_RUN:
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- NUM TUNO MAX 6");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<- ANG_ROT PULS MINUS");
        }
        if (!placement_editing(task)) {
            rraf_move_world(task);
            object_clear_touch(work, NULL);
            /* while no horn moves, pick the next number */
            if (work->tick == 0) {
                RRAF_CHOSEN(work) = njAbs(
                    njRoundOff(rand() * ceilf(RRAF_HORNS(work)) * RAND_SCALE));
            }
            task_step_children(task);
            sound_play_at_for(SE_RRAF_1, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 120);
            sound_play_at_for(SE_RRAF_2, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 120);
        }
        break;
    case RRAF_IDLE:
        break;
    }
    TASK_TIMER(work)++;
    if (RRAF_WAY(work) < 0.0F) {
        work->ang.x -= RRAF_TURN;
    } else {
        work->ang.x += RRAF_TURN;
    }
    rraf_draw(task);
}

/* turn the wheel's copy in the world with the wheel, and set WORLD_TESTED
 * while a player is near */
static void rraf_move_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world = VARS(work)->world;

    world->ang[0] = work->ang.x;
    world->ang[1] = 0;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    if (player_in_range(&work->pos, world->model->r + RRAF_REACH)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* draw the base, and the wheel turning on it */
static void rraf_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *wheel = &rraf_wheel_object;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    draw_model(&rraf_base_model);
    njPushMatrix(NULL);
    njTranslate(NULL, wheel->pos[0], wheel->pos[1], wheel->pos[2]);
    njRotateXYZ(NULL, wheel->ang[0], wheel->ang[1], wheel->ang[2]);
    TURN_VIA(njRotateX, work->ang.x, ang);
    draw_model(wheel->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void rraf_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->world);
    world_object_free(VARS(work)->world);
    task_destroy_children(task);
    dat_0ca57e34 = 0;
}

static void rraf_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;

    VARS(work)->world = world = world_object_alloc();
    world->scl[0] = 1.0F;
    world->scl[1] = 1.0F;
    world->scl[2] = 1.0F;
    world->ang[0] = work->ang.x;
    world->ang[1] = 0;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->model = rraf_world_object->model;
    world->child = NULL;
    world_add(WORLD_SMALL, task, world);
    /* @hack redundant: a return where the function returns anyway. with it, SHC
     * keeps task in a stack slot for world_add (MOV.L R4,@R15 at
     * 0x0c903f4c). without it, 44 of 46 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

/* a horn. its first step takes its number and its shape. later steps
 * draw it, run it and register its shape, unless the camera is near */
static void rraf_horn_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case RRAF_INIT:
        task->destructor = rraf_horn_destroy;
        task->draw = rraf_horn_draw;
        HORN_NUMBER(work) = (Uint8)work->flags;
        if (HORN_HOT(work)) {
            collision_init(task, rraf_hot_horn_shape, COLLISION_OBJECT, 1, 1);
        } else {
            collision_init(task, rraf_horn_shape, COLLISION_OBJECT, 1, 1);
        }
        work->mode = RRAF_RUN;
        SPRAY_TIMER(work) = 0.0F;
        break;
    case RRAF_RUN:
        if (njDistanceP2P(&work->pos, &camera_work->pos) > HORN_CAMERA_NEAR) {
            rraf_horn_draw(task);
            rraf_horn_run(task);
            collision_register(work);
        }
        break;
    case RRAF_IDLE:
        break;
    }
}

/* the horn, as high as it has risen */
static void rraf_horn_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MODEL *model;
    Angle ang;

    if (HORN_HOT(work)) {
        texture_set_stage();
        model = &rraf_hot_horn_model;
    } else {
        texture_set_stage();
        model = &rraf_horn_model;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njTranslate(NULL, 0.0F, -HORN_HEIGHT(work), 0.0F);
    draw_model(model);
    njPopMatrix(1);
}

static void rraf_horn_destroy(struct task *task)
{
}

/* a horn moves while its number is RRAF_CHOSEN, or a quarter of it rounded
 * down. its timer is an angle in degrees, and the horn stands HORN_RISE times
 * its sine high. the timer climbs HORN_RISE_STEP a step while under 90. it then
 * climbs 1 a step until it passes 180, and starts over at 0. a hot horn sprays
 * flame as it rises and sinks. a cold one sprays smoke only as it rises. the
 * wheel's tick is 1 while a horn moves */
static void rraf_horn_run(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float scale;

    SPRAY_TIMER(work) += 1.0F;
    if ((int)RRAF_CHOSEN(task->parent->work) == HORN_NUMBER(work) ||
        (int)(RRAF_CHOSEN(task->parent->work) * 0.25F) == HORN_NUMBER(work)) {
        if (TASK_TIMER(work) < 90) {
            TASK_TIMER(work) += HORN_RISE_STEP;
            task->parent->work->tick = 1;
            if (SPRAY_TIMER(work) == SPRAY_STEPS) {
                if (HORN_HOT(work)) {
                    NJS_POINT3 tip = {0.0F, HORN_TIP, 0.0F};
                    NJS_POINT3 pos;

                    njPushMatrix(&_unit_matrix_);
                    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
                    TURN_VIA(njRotateZ, work->ang.z, ang);
                    TURN_VIA(njRotateX, work->ang.x, ang);
                    njTranslate(NULL, 0.0F, -HORN_HEIGHT(work), 0.0F);
                    njCalcPoint(NULL, &tip, &pos);
                    njPopMatrix(1);
                    rraf_fire_create(&pos);
                } else {
                    NJS_POINT3 tip = {0.0F, HORN_TIP, 0.0F};
                    NJS_POINT3 pos;

                    njPushMatrix(&_unit_matrix_);
                    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
                    TURN_VIA(njRotateZ, work->ang.z, ang);
                    TURN_VIA(njRotateX, work->ang.x, ang);
                    njTranslate(NULL, 0.0F, -HORN_HEIGHT(work), 0.0F);
                    njCalcPoint(NULL, &tip, &pos);
                    njPopMatrix(1);
                    rraf_smoke_create(&pos);
                }
            }
        } else if (TASK_TIMER(work) > 180) {
            TASK_TIMER(work) = 0;
            HORN_VARS(work)->unk_04 = 0.0F;
            task->parent->work->tick = 0;
        } else {
            TASK_TIMER(work)++;
            task->parent->work->tick = 1;
            if (SPRAY_TIMER(work) == SPRAY_STEPS) {
                if (HORN_HOT(work)) {
                    NJS_POINT3 tip = {0.0F, HORN_TIP, 0.0F};
                    NJS_POINT3 pos;

                    njPushMatrix(&_unit_matrix_);
                    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
                    TURN_VIA(njRotateZ, work->ang.z, ang);
                    TURN_VIA(njRotateX, work->ang.x, ang);
                    njTranslate(NULL, 0.0F, -HORN_HEIGHT(work), 0.0F);
                    njCalcPoint(NULL, &tip, &pos);
                    njPopMatrix(1);
                    rraf_fire_create(&pos);
                }
            }
        }
    }
    if (SPRAY_TIMER(work) == SPRAY_STEPS) {
        SPRAY_TIMER(work) = 0.0F;
    }
    if (RRAF_WAY(task->parent->work) < 0.0F) {
        work->ang.x -= RRAF_TURN;
    } else {
        work->ang.x += RRAF_TURN;
    }
    HORN_HEIGHT(work) =
        njSin((Angle)(TASK_TIMER(work) * ANGLE_PER_DEGREE)) * HORN_RISE;
    if (HORN_HEIGHT(work) < 0.0F) {
        HORN_HEIGHT(work) = 0.0F;
    }
    /* the shape grows with the horn's height, and a hot one hurts only while
     * the horn is up. the height is 0 or more here, so the floor of -0.9 never
     * applies */
    scale = HORN_HEIGHT(work) * SHAPE_GROWTH;
    if (scale <= -1.0F) {
        scale = -0.9F;
    }
    work->collision->shapes->size_y = (scale + 1.0F) * SHAPE_HEIGHT;
    if (HORN_HEIGHT(work) == 0.0F) {
        if (HORN_HOT(work)) {
            /* @hack spelling: the clear spelled out, as the set below it is
             * (AND #252 at 0x0c9044b8). as &=, 6 of rraf_horn_run's 372
             * words differ */
            work->collision->shapes->levels =
                work->collision->shapes->levels & ~SHAPE_ATTACK;
        }
    } else {
        if (HORN_HOT(work)) {
            work->collision->shapes->levels =
                (work->collision->shapes->levels & ~SHAPE_ATTACK) |
                HOT_HORN_ATTACK;
        }
    }
}

/* a flame that starts FIRE_REACH from pos toward the camera, and moves on
 * that way and up */
static void rraf_fire_create(NJS_POINT3 *pos)
{
    struct particle *particle = particle_alloc(bomb_pool.pool, rraf_fire_step);
    Angle ang;
    Float distance;
    Float scale;

    if (particle != NULL) {
        ang = (Angle)(atan2f(camera_work->pos.x - pos->x,
                             camera_work->pos.z - pos->z) *
                      ANGLE_PER_RADIAN) -
              NJM_DEG_ANG(90);
        ang += (Angle)(rand() * RAND_SCALE * 2048.0F) - 1024;
        particle->speed.x = njCos(ang) * FLAME_SPEED;
        particle->speed.z = -(njSin(ang) * FLAME_SPEED);
        particle->pos.x = pos->x + (njCos(ang) * FIRE_REACH);
        particle->pos.z = pos->z - (njSin(ang) * FIRE_REACH);
        particle->speed.y = rand() * RAND_SCALE * 0.22F;
        particle->pos.y = pos->y;
        distance = njDistanceP2P(&camera_work->pos, pos);
        if (distance > FLAME_FAR) {
            scale = FLAME_FAR_SCALE;
        } else {
            scale = distance * ((1.0F / FLAME_FAR) * FLAME_NEAR_SCALE);
        }
        particle->scale = scale + FLAME_SCALE;
        particle->time = rand() * RAND_SCALE * 6.0F;
        particle->ang = (Angle)(rand() * RAND_SCALE * 8192.0F) - 4096;
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = 0.8F;
        particle->color.b = 0.8F;
    }
}

/* a flame shows its animation's frames once, a third of a frame a step. it
 * slows, rises and grows as it goes */
static void rraf_fire_step(struct particle *particle,
                           struct particle_pool *pool)
{
    Float drag = 0.05F;

    particle->frame = floorf(particle->time);
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    particle->time += 0.33F;
    particle->speed.x += -particle->speed.x * drag;
    particle->speed.y += (-particle->speed.y * drag) + 0.02F;
    particle->speed.z += -particle->speed.z * drag;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    particle->scale += 0.0005F;
}

/* a puff of smoke SMOKE_REACH from pos toward the camera */
static void rraf_smoke_create(NJS_POINT3 *pos)
{
    /* @hack redundant: speed, set from its initializer and never read,
     * which copies three words onto the frame (0x0c9047f8 to 0x0c904808).
     * without it, the frame loses 12 bytes, and 71 of 71 words differ */
    NJS_VECTOR speed = {0.0F, 0.21F, 0.0F};
    Angle ang;

    ang = (Angle)(atan2f(camera_work->pos.x - pos->x,
                         camera_work->pos.z - pos->z) *
                  ANGLE_PER_RADIAN) -
          NJM_DEG_ANG(90);
    rraf_smoke.pos.x = pos->x + (njCos(ang) * SMOKE_REACH);
    rraf_smoke.pos.y = pos->y;
    rraf_smoke.pos.z = pos->z - (njSin(ang) * SMOKE_REACH);
    dust_create_from(&rraf_smoke);
}
