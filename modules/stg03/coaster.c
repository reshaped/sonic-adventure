/* the coaster, which the set editor names O COASTER: a train of four cars.
 * player 0 boards its front car, and the train carries Sonic or Amy along
 * its path */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "path/path.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg03/coaster.h"
#include "stg03/effect_textures.h"
#include "stg03/paths.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_OBJECT twinkle_coaster_car_object;

#define SE_COASTER_START 77
#define SE_COASTER_RUN 76

#define SPARKLE_FRAMES 3

/* player 0 moves into the front car over BOARD_STEPS steps. a stopped
 * train waits more than STOP_STEPS steps before it is done */
#define BOARD_STEPS 20
#define STOP_STEPS 60

/* a car within COASTER_END_NEAR units of twinkle_coaster_end stops the train */
#define COASTER_END_NEAR 10.0F

enum coaster_mode {
    COASTER_INIT,
    COASTER_WAIT, /* until a player touches the train */
    COASTER_BOARD, /* until Sonic or Amy rides in the front car */
    COASTER_RUN,
    COASTER_END, /* for one step, once a car stops the train */
    COASTER_STOP,
    COASTER_DONE
};

/* car modes, which follow the train's */
enum car_mode {
    CAR_INIT,
    CAR_WAIT,
    CAR_BOARD,
    CAR_RUN,
    CAR_END,
    CAR_STOPPED,
    CAR_IDLE /* no code sets it */
};

/* the train's work->scl.x: the distance its cars move along the path each
 * step. a train runs at its set entry's scale x plus BASE_SPEED */
#define COASTER_SPEED(work) ((work)->scl.x)
#define BASE_SPEED 7.5F
/* bits of the train's work->tick: the character who rides the front car.
 * the train runs only for RIDER_SONIC or RIDER_AMY */
#define COASTER_RIDER(work) ((work)->tick)
#define RIDER_SONIC 0x1
#define RIDER_AMY 0x4

struct car_vars {
    NJS_OBJECT *object;
    Float distance; /* along the path */
    /* the car's place in the train, 0 at the front. the front car carries
     * player 0, and gives its seat, 0, wherever a call takes a player's
     * number */
    /* @hack type: a one-element array, whose read SHC makes through its
     * address in R12 (ADD #16,R12 at 0x0c903a7e); as a plain Sint8, which
     * SHC reads @(R0,R14), 57 of 526 words differ */
    Sint8 seat[1];
};

#define CAR(work) ((struct car_vars *)&(work)->vars)
/* the front car's work->scl: player 0's speed into the car each step it
 * boards */
#define BOARD_SPEED(work) ((work)->scl)

static void twinkle_coaster_car_step(struct task *task);

static struct collision_shape twinkle_coaster_shape[1] = {
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_PLAYERS),
     {0.0F, 0.0F, 0.0F},
     5.0F,
     0.0F,
     10.0F,
     {0},
     0,
     NULL},
};

static NJS_TEXANIM twinkle_coaster_sparkle_frames[SPARKLE_FRAMES] = {
#include "stg03/coaster/twinkle_coaster_sparkle_frames.inc"
};
static struct particle_anim twinkle_coaster_sparkle_anim = {
    &eff_twinkle_texlist, twinkle_coaster_sparkle_frames, SPARKLE_FRAMES,
    NJD_COLOR_BLENDING_SRCALPHA, NJD_COLOR_BLENDING_ONE};

struct particle_pool_spec twinkle_coaster_sparkle_pool = {
    &twinkle_coaster_sparkle_anim, 0, NULL, NULL};

/* growth and drag are the shares of a sparkle's size and speed it keeps
 * each step. dust.h gives them other meanings for dust's own step */
static struct dust_params twinkle_coaster_sparkle_params = {
    1.0F,
    0.95F,
    0.1F,
    0.95F,
    -0.1F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* the train's four cars, then a null spec that ends the list. each car's
 * work flags give its seat, and its var the car model */
static struct task_spec twinkle_coaster_cars[5] = {
    {twinkle_coaster_car_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&twinkle_coaster_car_object},
    {twinkle_coaster_car_step,
     TASK_WORK,
     1,
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&twinkle_coaster_car_object},
    {twinkle_coaster_car_step,
     TASK_WORK,
     2,
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&twinkle_coaster_car_object},
    {twinkle_coaster_car_step,
     TASK_WORK,
     3,
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&twinkle_coaster_car_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

static const NJS_POINT3 twinkle_coaster_end = {-145.0F, 152.0F, -1000.0F};
/* a running car throws a sparkle from each side, at these offsets in its
 * own axes */
static const NJS_VECTOR twinkle_coaster_sparkle_right = {9.3F, 8.3F, 5.0F};
static const NJS_VECTOR twinkle_coaster_sparkle_left = {9.3F, 8.3F, -5.0F};
static const NJS_VECTOR twinkle_coaster_sparkle_speed = {0.0F, 0.0F, 0.0F};

/* play a sparkle's frames once at params->rate, then kill it. each step
 * scales its speed by drag and its size by growth. lift adds to its upward
 * speed */
static void twinkle_coaster_sparkle_step(struct particle *particle,
                                         struct particle_pool *pool)
{
    struct dust_params *params = DUST(particle)->params;

    particle->frame = particle->time;
    if (particle->frame > SPARKLE_FRAMES - 1) {
        particle->frame = SPARKLE_FRAMES - 1;
        particle_kill(particle);
        return;
    }
    /* @hack redundant: rate times 1.0F, which SHC adds with FLDI1 FR0 and
     * FMAC (0x0c903820, 0x0c90382e); as a plain add, an FADD, the function
     * runs 2 bytes short and 55 of 74 words differ */
    particle->time += params->rate * 1.0F;
    particle->speed.x *= params->drag;
    particle->speed.y = (particle->speed.y * params->drag) + params->lift;
    particle->speed.z *= params->drag;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    particle->scale *= params->growth;
}

static void twinkle_coaster_sparkle_create(NJS_POINT3 *pos, NJS_VECTOR *speed,
                                           Float scale)
{
    struct particle_pool *pool = twinkle_coaster_sparkle_pool.pool;
    struct particle *particle =
        particle_alloc(pool, twinkle_coaster_sparkle_step);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = scale;
        particle->time = rand() * RAND_SCALE * 5.0F;
        particle->ang = RANDOM(100.0F, 260.0F) * ANGLE_PER_DEGREE;
        particle->color = twinkle_coaster_sparkle_params.color;
        DUST(particle)->params = &twinkle_coaster_sparkle_params;
    }
}

static void twinkle_coaster_car_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        draw_tree(CAR(work)->object);
        njPopMatrix(1);
    }
}

/* outside the set editor, pass the car's model to world_remove and to
 * world_object_free. both leave it as it is. no car adds its model to the
 * collision world, and world_object_alloc never lent it */
static void twinkle_coaster_car_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && CAR(work)->object != NULL) {
        world_remove(task, CAR(work)->object);
        world_object_free(CAR(work)->object);
    }
}

static void twinkle_coaster_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* step a car through the train's modes. the front car draws player 0 in
 * and carries it along the path. at the end, it sends player 0 off. each
 * running car throws sparkles */
static void twinkle_coaster_car_step(struct task *task)
{
    struct task_work *work = task->work;
    struct path *path = &twinkle_coaster_path;
    Sint8 seat = CAR(work)->seat[0];
    struct path_sample sample;
    NJS_POINT3 end;
    NJS_VECTOR ahead;
    NJS_VECTOR left;
    NJS_VECTOR speed;
    NJS_POINT3 right_pos;
    NJS_POINT3 left_pos;
    Angle ang;

    switch (work->mode) {
    case CAR_INIT:
        work->mode = CAR_WAIT;
        /* @hack redundant: the model again, over the bytes the switch below
         * sets as distance (MOV.L R2,@(12,R14) at 0x0c903ac4); without it, 23
         * of 526 words differ */
        work->vars.p[1] = &twinkle_coaster_car_object;
        task->destructor = twinkle_coaster_car_destroy;
        task->draw = twinkle_coaster_car_draw;
        CAR(work)->seat[0] = work->flags;
        switch (work->flags) {
        case 0:
            CAR(work)->distance = 54.0F;
            break;
        case 1:
            CAR(work)->distance = 36.0F;
            break;
        case 2:
            CAR(work)->distance = 18.0F;
            break;
        case 3:
            CAR(work)->distance = 0.0F;
            break;
        }
        sample.distance = CAR(work)->distance;
        if (path_sample(path, &sample)) {
            work->pos = sample.pos_copy;
            work->ang.x = sample.smooth_x;
            work->ang.y = (Angle)(atan2f(sample.ahead.x, sample.ahead.z) *
                                  ANGLE_PER_RADIAN) +
                          0x4000;
            work->ang.z = sample.smooth_z;
        }
        break;
    case CAR_WAIT:
        if (task->parent->work->mode == COASTER_BOARD) {
            work->mode = CAR_BOARD;
            if (seat == 0) {
                BOARD_SPEED(work).x = (work->pos.x - player_work[seat]->pos.x) *
                                      (1.0F / BOARD_STEPS);
                BOARD_SPEED(work).y = (work->pos.y - player_work[seat]->pos.y) *
                                      (1.0F / BOARD_STEPS);
                BOARD_SPEED(work).z = (work->pos.z - player_work[seat]->pos.z) *
                                      (1.0F / BOARD_STEPS);
                player_set_mode(seat, PLAYER_MODE_GET_ON);
            }
        }
        twinkle_coaster_car_draw(task);
        break;
    case CAR_BOARD:
        if (task->parent->work->mode == COASTER_RUN) {
            work->mode = CAR_RUN;
        } else if (seat == 0) {
            if (++TASK_TIMER(work) > BOARD_STEPS) {
                player_set_mode(seat, PLAYER_MODE_RIDE);
                switch (player_get_character(seat)) {
                case CHARACTER_SONIC:
                    COASTER_RIDER(task->parent->work) |= RIDER_SONIC;
                    break;
                case CHARACTER_AMY:
                    COASTER_RIDER(task->parent->work) |= RIDER_AMY;
                    break;
                default:
                    break;
                }
            } else {
                player_work[seat]->pos.x += BOARD_SPEED(work).x;
                player_work[seat]->pos.y += BOARD_SPEED(work).y;
                player_work[seat]->pos.z += BOARD_SPEED(work).z;
            }
        }
        twinkle_coaster_car_draw(task);
        break;
    case CAR_RUN:
        if (task->parent->work->mode == COASTER_END) {
            work->mode = CAR_END;
        } else {
            end = twinkle_coaster_end;
            if (njDistanceP2P(&end, &work->pos) < COASTER_END_NEAR) {
                task->parent->work->mode = COASTER_END;
                work->mode = CAR_END;
            }
            CAR(work)->distance += COASTER_SPEED(task->parent->work);
            sample.distance = CAR(work)->distance;
            if (path_sample(path, &sample)) {
                work->pos = sample.pos_copy;
                work->ang.x = sample.smooth_x;
                work->ang.z = sample.smooth_z;
                njPushMatrix(&_unit_matrix_);
                TURN_VIA(njRotateX, -work->ang.x, ang);
                TURN_VIA(njRotateZ, -work->ang.z, ang);
                njCalcVector(NULL, &sample.ahead, &ahead);
                njPopMatrix(1);
                work->ang.y =
                    (Angle)(atan2f(ahead.x, ahead.z) * ANGLE_PER_RADIAN) +
                    0x4000;
            }
            if (seat == 0) {
                player_set_position(seat, work->pos.x, work->pos.y,
                                    work->pos.z);
                player_set_angles(seat, work->ang.x, work->ang.y - 0x4000,
                                  work->ang.z);
            }
            ahead = twinkle_coaster_sparkle_right;
            left = twinkle_coaster_sparkle_left;
            speed = twinkle_coaster_sparkle_speed;
            njPushMatrix(&_unit_matrix_);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njCalcVector(NULL, &ahead, &right_pos);
            njCalcVector(NULL, &left, &left_pos);
            njPopMatrix(1);
            njAddVector(&right_pos, &work->pos);
            njAddVector(&left_pos, &work->pos);
            twinkle_coaster_sparkle_create(&right_pos, &speed, 0.5F);
            twinkle_coaster_sparkle_create(&left_pos, &speed, 0.5F);
            if (rand() * RAND_SCALE > 0.5F) {
                twinkle_coaster_sparkle_create(&work->pos, &speed, 1.0F);
            }
        }
        twinkle_coaster_car_draw(task);
        break;
    case CAR_END:
        if (seat == 0) {
            player_ride_off(seat, -1.2F, 1.5F, 0.0F);
            vibration_start(seat, 4, 15, 0);
            work->mode = CAR_STOPPED;
        }
        twinkle_coaster_car_draw(task);
        break;
    case CAR_STOPPED:
        twinkle_coaster_car_draw(task);
        break;
    case CAR_IDLE:
        break;
    }
}

void twinkle_coaster_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_editing(task)) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        draw_tree(&twinkle_coaster_car_object);
        njPopMatrix(1);
        return;
    }
    switch (work->mode) {
    case COASTER_INIT:
        COASTER_SPEED(work) += BASE_SPEED;
        work->mode = COASTER_WAIT;
        collision_init(task, twinkle_coaster_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = twinkle_coaster_destroy;
        task_create_children(twinkle_coaster_cars, task);
        return;
    case COASTER_WAIT:
        if ((work->collision->flags & COLLISION_HIT) &&
            work->collision->hit->kind == COLLISION_PLAYER) {
            work->mode = COASTER_BOARD;
        } else {
            collision_register(work);
        }
        task_step_children(task);
        break;
    case COASTER_BOARD:
        if ((COASTER_RIDER(work) & RIDER_SONIC) ||
            (COASTER_RIDER(work) & RIDER_AMY)) {
            work->mode = COASTER_RUN;
            sound_play(SE_COASTER_START, 0, 0, 0);
        }
        task_step_children(task);
        break;
    case COASTER_RUN:
        task_step_children(task);
        sound_play_at_for(SE_COASTER_RUN, (int)work, 1, 96,
                          player_work[0]->pos.x, player_work[0]->pos.y,
                          player_work[0]->pos.z, 2);
        vibration_start(0, 1, 48, 0);
        break;
    case COASTER_END:
        work->mode = COASTER_STOP;
        sound_stop(SE_COASTER_RUN);
        sound_play(SE_CLUNK, 0, 0, 0);
        task_step_children(task);
        break;
    case COASTER_STOP:
        if (++TASK_TIMER(work) > STOP_STEPS) {
            work->mode = COASTER_DONE;
        }
        task_step_children(task);
        break;
    case COASTER_DONE:
        break;
    }
}
