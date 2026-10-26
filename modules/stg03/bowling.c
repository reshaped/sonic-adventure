/* O BOWLING: ten pins, with a gate and a platform that gives rings beyond
 * them. O BALL: a ball that rolls off at a player's touch, and its gate */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "object/ring.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg03/bowling.h"
#include "stg03/mirror.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_OBJECT bowling_pin_object;
extern NJS_OBJECT bowling_gate_object;
extern NJS_OBJECT bowling_platform_object;
extern NJS_OBJECT bowling_ball_object;

enum lane_mode {
    LANE_INIT,
    LANE_WAIT,
    LANE_ACTIVE, /* player 0 has stood on the lane. from this mode on, the
                  * gate's delay and the platform's spin run */
    LANE_CLEAR, /* a pin is down, or LANE_ACTIVE's count has passed
                 * ACTIVE_STEPS. the standing pins sink */
    LANE_IDLE
};
enum pin_mode { PIN_INIT, PIN_STANDING, PIN_FLYING, PIN_SINKING, PIN_IDLE };
enum gate_mode { GATE_INIT, GATE_WAIT, GATE_DELAY, GATE_RISE, GATE_OPEN };
enum platform_mode {
    PLATFORM_INIT,
    PLATFORM_WAIT,
    PLATFORM_SPIN,
    PLATFORM_REWARD,
    PLATFORM_DONE
};
/* the ball's modes, which O BALL uses too. the ball sets O BALL's mode to
 * BALL_ROLLING as it starts to roll. a gate waits for its parent's mode to
 * reach LANE_ACTIVE. BALL_ROLLING equals it, so O BALL's gate starts then */
enum ball_mode { BALL_INIT, BALL_WAIT, BALL_ROLLING, BALL_SINKING, BALL_IDLE };

/* pin n's bit. pins are numbered as in bowling: 1 in front, and 7 to 10 in
 * the back row. each row's numbers rise with x. a pin's spec gives its bit as
 * the pin's work flags. the pin's first step moves the bit to its timer.
 * a pin that falls sets its bit in the lane's flags */
#define PIN(n) (1 << ((n) - 1))
#define FRONT_ROW PIN(1)
#define SECOND_ROW (PIN(2) | PIN(3))
#define THIRD_ROW (PIN(4) | PIN(5) | PIN(6))
#define BACK_ROW (PIN(7) | PIN(8) | PIN(9) | PIN(10))
#define ALL_PINS (FRONT_ROW | SECOND_ROW | THIRD_ROW | BACK_ROW)

/* a gate's work flags: BALL_GATE for O BALL's gate, and 0 for the lane's */
#define BALL_GATE 1

/* LANE_ACTIVE ends on the step its count passes ACTIVE_STEPS, its 122nd */
#define ACTIVE_STEPS 120

/* a gate rises once its delay's count passes GATE_DELAY_STEPS */
#define GATE_DELAY_STEPS 120
#define GATE_RISE_HEIGHT 85.0F
#define GATE_RISE_STEPS 180

/* the platform's child spins SPIN_STEPS steps before the platform starts to
 * give rings. it gives a round each time its count passes ROUND_WAIT, every
 * 62 steps. its flashing materials flash until the count passes
 * FLASH_STEPS */
#define SPIN_STEPS 90.0F
#define ROUND_WAIT 60
#define FLASH_STEPS 30
/* while it gives rings, the platform scrolls six meshsets' uvs in v by its
 * timer. the timer rises UV_SCROLL a step, and starts again from 0 at
 * UV_WRAP */
#define UV_SCROLL 8
#define UV_WRAP 255

/* the rings in each round the platform gives */
#define ROUND_RINGS 8

/* the platform's textures, by their numbers in the stage's first texture
 * list. six of its materials mark the rounds of rings left to give. each
 * shows ROUND_TEXTURE, its own, for a round left, and NO_ROUND_TEXTURE
 * otherwise. material 3 and the child's material 1 flash FLASH_TEXTURE in
 * place of their own, PLAIN_TEXTURE */
#define ROUND_TEXTURE 31
#define NO_ROUND_TEXTURE 32
#define PLAIN_TEXTURE 28
#define FLASH_TEXTURE 95

/* the ball's radius. it rests this far above the floor, and keeps this far
 * from the walls */
#define BALL_RADIUS 6.0F
/* the speed the ball gains downward each step it falls */
#define BALL_GRAVITY 0.5F
/* the part of its speed the ball keeps at each wall it meets */
#define WALL_KEEP 0.9F
/* a rolling ball slower than this sinks */
#define SINK_SPEED 0.1F
/* a ball walls hold in place turns its heading by this times a roll of -0.5
 * to 0.5. that is up to about 2.5 turns either way. the range
 * is in angle units */
#define HELD_TURN_RANGE 331320.85F
/* how far the ball turns about z for each unit of its speed, in angle units:
 * 9.55 degrees. a ball of radius BALL_RADIUS turns 9.549 degrees for each unit
 * it rolls */
#define ROLL_TURN 1738.5242F

#define SE_FRONT_PIN 66
#define SE_PIN_DOWN 70
#define SE_REWARD 71
#define SE_ROLL 83
#define SE_GATE 88

struct bowling_vars {
    NJS_OBJECT *object;
    Float speed; /* the ball's speed along its heading, in units a step */
    Angle turn; /* the platform's child's turn about y */
};

#define VARS(work) ((struct bowling_vars *)&(work)->vars)

static void pin_step(struct task *task);
static void gate_step(struct task *task);
static void platform_step(struct task *task);
static void ball_step(struct task *task);

static struct collision_shape pin_shape = {
    SHAPE_CYLINDER | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
    {0.0F, 10.0F, 0.0F},
    4.7F,
    10.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static struct collision_shape gate_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
    {0.0F, 42.5F, 0.2F},
    54.0F,
    42.5F,
    3.9F,
    {0},
    0,
    NULL,
};

/* the ball's cylinder and sphere. a player's touch starts the roll, which
 * turns the cylinder off and raises the sphere's attack to 3. the sphere
 * can then hurt enemies and items */
static struct collision_shape ball_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     0,
     0,
     {0.0F, 0.0F, 0.0F},
     8.0F,
     8.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_ENEMIES | GUARD(2),
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL,
     {0.0F, 0.0F, 0.0F},
     BALL_RADIUS,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static struct collision_shape platform_shapes[2] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 1.246F, 0.0F},
     31.28F,
     1.246F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 30.0F, 0.0F},
     9.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* the lane's children, in the order they step. the pins come from the front
 * row back, then the gate and the platform */
static struct task_spec bowling_children[13] = {
    {pin_step,
     TASK_WORK,
     PIN(1),
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&bowling_pin_object},
    {pin_step,
     TASK_WORK,
     PIN(2),
     {0, 0, 0},
     {-6.0F, 0.0F, -10.0F},
     (Sint32)&bowling_pin_object},
    {pin_step,
     TASK_WORK,
     PIN(3),
     {0, 0, 0},
     {6.0F, 0.0F, -10.0F},
     (Sint32)&bowling_pin_object},
    {pin_step,
     TASK_WORK,
     PIN(4),
     {0, 0, 0},
     {-12.0F, 0.0F, -20.0F},
     (Sint32)&bowling_pin_object},
    {pin_step,
     TASK_WORK,
     PIN(5),
     {0, 0, 0},
     {0.0F, 0.0F, -20.0F},
     (Sint32)&bowling_pin_object},
    {pin_step,
     TASK_WORK,
     PIN(6),
     {0, 0, 0},
     {12.0F, 0.0F, -20.0F},
     (Sint32)&bowling_pin_object},
    {pin_step,
     TASK_WORK,
     PIN(7),
     {0, 0, 0},
     {-18.0F, 0.0F, -30.0F},
     (Sint32)&bowling_pin_object},
    {pin_step,
     TASK_WORK,
     PIN(8),
     {0, 0, 0},
     {-6.0F, 0.0F, -30.0F},
     (Sint32)&bowling_pin_object},
    {pin_step,
     TASK_WORK,
     PIN(9),
     {0, 0, 0},
     {6.0F, 0.0F, -30.0F},
     (Sint32)&bowling_pin_object},
    {pin_step,
     TASK_WORK,
     PIN(10),
     {0, 0, 0},
     {18.0F, 0.0F, -30.0F},
     (Sint32)&bowling_pin_object},
    {gate_step,
     TASK_WORK,
     0,
     {0, 0x8000, 0},
     {0.0F, 0.0F, -110.0F},
     (Sint32)&bowling_gate_object},
    {platform_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {0.0F, 0.5F, -200.0F},
     (Sint32)&bowling_platform_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

static struct task_spec bowling_ball_children[3] = {
    {ball_step,
     TASK_WORK,
     0,
     {0, 0x8000, 0},
     {0.0F, BALL_RADIUS, 0.0F},
     (Sint32)&bowling_ball_object},
    {gate_step,
     TASK_WORK,
     BALL_GATE,
     {0, 0x8000, 0},
     {0.0F, 0.0F, -110.0F},
     (Sint32)&bowling_gate_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

static const NJS_ARGB pin_material = {-0.5F, 0.0F, 0.0F, 0.0F};
static const NJS_ARGB ball_material = {-0.5F, 0.0F, 0.0F, 0.0F};

static void parent_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* set the platform's submode to the rounds of rings it gives. the foremost
 * row with a pin down sets them. the front row gives 6, the next 4, then 2
 * and 1. with every pin standing, there are none. pin 1 down also plays
 * SE_FRONT_PIN at player 0 */
static void set_reward_rounds(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    struct task_work *player;

    /* @hack spelling: a goto to done ending each arm, which keeps
     * sound_play_at's JSR at 0x0c901616; as a plain if-else, SHC tail-calls
     * sound_play_at and 40 of 47 words differ */
    if (parent->flags & FRONT_ROW) {
        work->submode = 6;
        player = player_work[0];
        sound_play_at(SE_FRONT_PIN, 0, 0, 96, player->pos.x, player->pos.y,
                      player->pos.z);
        goto done;
    } else if (parent->flags & SECOND_ROW) {
        work->submode = 4;
        goto done;
    } else if (parent->flags & THIRD_ROW) {
        work->submode = 2;
        goto done;
    } else if (parent->flags & BACK_ROW) {
        work->submode = 1;
        goto done;
    } else {
        work->submode = 0;
        goto done;
    }
done:;
}

static void child_destroy(struct task *task)
{
    task_kill(task);
}

static void pin_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle angle;

    if (!frame_step) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        if (work->mode == PIN_FLYING) {
            TURN_VIA(njRotateZ, work->ang.z, angle);
            TURN_VIA(njRotateX, work->ang.x, angle);
        }
        TURN_VIA(njRotateY, work->ang.y, angle);
        draw_tree(VARS(work)->object);
        njPopMatrix(1);
    }
}

static void pin_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    NJS_ARGB material;

    if (edit_mode) {
        pin_draw(task);
    }
    switch (work->mode) {
    case PIN_INIT:
        collision_init(task, &pin_shape, COLLISION_OBJECT, 1, 1);
        task->work->collision->radius = 20.0F;
        work->timer = work->flags;
        work->timer &= ALL_PINS;
        work->flags = 0;
        task->destructor = child_destroy;
        task->draw = pin_draw;

        /* @hack redundant: a select whose condition is constant, which loads
         * the 1 at its store (MOV #1,R2 at 0x0c9017d4); as a plain
         * PIN_STANDING, SHC holds 1 in R11 through pin_step, and 101 of its
         * 354 words and 3 of the 147 words of its pools and pad differ */
        /* NOLINTNEXTLINE(clang-diagnostic-*,misc-redundant-expression) */
        work->mode = 0.5F ? PIN_STANDING : PIN_STANDING;
        break;
    case PIN_STANDING:
        pin_draw(task);
        if (parent->mode == LANE_CLEAR) {
            work->mode = PIN_SINKING;
        }
        /* a standing pin falls once a pin just ahead of it is down. pin 1 has
         * none ahead. the pins step from the front row back, so a fall
         * reaches the back row within one step */
        switch (TASK_TIMER(work)) {
        case PIN(1):
            break;
        case PIN(2):
        case PIN(3):
            if (parent->flags & PIN(1)) {
                work->flags |= COLLISION_DAMAGED;
            }
            break;
        case PIN(4):
            if (parent->flags & PIN(2)) {
                work->flags |= COLLISION_DAMAGED;
            }
            break;
        case PIN(5):
            if (parent->flags & (PIN(2) | PIN(3))) {
                work->flags |= COLLISION_DAMAGED;
            }
            break;
        case PIN(6):
            if (parent->flags & PIN(3)) {
                work->flags |= COLLISION_DAMAGED;
            }
            break;
        case PIN(7):
            if (parent->flags & PIN(4)) {
                work->flags |= COLLISION_DAMAGED;
            }
            break;
        case PIN(8):
            if (parent->flags & (PIN(4) | PIN(5))) {
                work->flags |= COLLISION_DAMAGED;
            }
            break;
        case PIN(9):
            if (parent->flags & (PIN(5) | PIN(6))) {
                work->flags |= COLLISION_DAMAGED;
            }
            break;
        case PIN(10):
            if (parent->flags & PIN(6)) {
                work->flags |= COLLISION_DAMAGED;
            }
            break;
        }
        if ((work->flags & COLLISION_DAMAGED) ||
            (work->collision->flags & COLLISION_HIT)) {
            work->flags |= COLLISION_DAMAGED;
            work->mode = PIN_FLYING;
            parent->flags |= work->timer;
            /* scl is the flying pin's speed. in x and z it starts at an eighth
             * of the pin's offset from player 0. in y it starts upward at
             * random */
            work->scl.x = (work->pos.x - player_work[0]->pos.x) * 0.125F;
            work->scl.y = RANDOM(0.4F, 1.4F) * 3.0F;
            work->scl.z = (work->pos.z - player_work[0]->pos.z) * 0.125F;
            work->ang.x = (int)RANDOM_ANGLE() & 0xffff;
            work->ang.y = (int)RANDOM_ANGLE() & 0xffff;
            work->ang.z = (int)RANDOM_ANGLE() & 0xffff;
            vibration_start(0, 3, 7, 0);
        }
        collision_register(work);
        break;
    case PIN_FLYING:
        if (placement_try_leave(task)) {
            break;
        }
        work->scl.x *= 0.95F;
        work->scl.z *= 0.95F;
        work->scl.y -= 0.05F;
        work->pos.x += work->scl.x;
        work->pos.y += work->scl.y;
        work->pos.z += work->scl.z;
        work->ang.x += 0x1000;
        work->ang.y += 0x500;
        work->ang.z += 0x500;
        pin_draw(task);
        break;
    case PIN_SINKING:
        if (placement_try_leave(task)) {
            break;
        }
        work->pos.y -= 1.0F;
        material = pin_material;
        draw_attr_save();
        draw_attr_set(0, NJD_FLAG_USE_ALPHA);
        njSetConstantMaterial(&material);
        pin_draw(task);

        /* @hack redundant: material.a, stored after njSetConstantMaterial
         * has copied the material, so Ninja's constant material keeps the
         * -0.5 alpha. the store is the image's FMOV.S at 0x0c901aa0; without
         * it, 19 of 354 words differ */
        material.a = 0.0F;
        draw_attr_restore();
        break;
    case PIN_IDLE:
        break;
    }
}

static void gate_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;
    Angle angle;

    if (!frame_step) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, angle);
        draw_model(object->model);
        njPopMatrix(1);
    }
}

/* a gate of the lane or of O BALL. it rises once its parent's mode reaches
 * LANE_ACTIVE, after a delay */
static void gate_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    NJS_OBJECT *object;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_TWINKLE, 1) ||
        (work->flags == BALL_GATE &&
         player_get_character(0) == CHARACTER_SONIC)) {
        /* task_kill destroys the gate on its next step, so this one runs on */
        task_kill(task);
    }
    object = &bowling_gate_object;
    if (edit_mode) {
        VARS(work)->object = object;
        gate_draw(task);
    } else {
        /* @hack spelling: collision_register called through register_collision,
         * which loads its address into R12 and the constant 2 into R11
         * (0x0c901bd6, 0x0c901bdc); called by name, the two trade registers
         * and 6 of 147 words differ */
        void (*register_collision)(struct task_work *) = collision_register;
        switch (work->mode) {
        case GATE_INIT:
            work->mode = GATE_WAIT;
            VARS(work)->object = object;
            collision_init(task, &gate_shape, COLLISION_OBJECT, 1, 1);
            task->work->collision->radius = 90.0F;
            task->destructor = child_destroy;
            task->draw = gate_draw;
            break;
        case GATE_WAIT:
            if (parent->mode >= LANE_ACTIVE) {
                work->mode = GATE_DELAY;
            }
            register_collision(work);
            gate_draw(task);
            break;
        case GATE_DELAY:
            if (work->tick++ > GATE_DELAY_STEPS) {
                work->mode = GATE_RISE;
            }
            register_collision(work);
            gate_draw(task);
            break;
        case GATE_RISE:
            if (TASK_TIMER(work)++ < GATE_RISE_STEPS) {
                work->pos.y += GATE_RISE_HEIGHT / GATE_RISE_STEPS;
                sound_play_at_for(SE_GATE, (int)work, 1, 0, work->pos.x,
                                  work->pos.y, work->pos.z, 2);
            } else {
                work->mode = GATE_OPEN;
            }
            register_collision(work);
            gate_draw(task);
            break;
        case GATE_OPEN:
            break;
        }
    }
}

/* show the rounds of rings left, the submode, on the six materials that mark
 * them. material 3 and the child's material 1 show FLASH_TEXTURE until the
 * tick passes FLASH_STEPS, and PLAIN_TEXTURE after */
static void platform_set_textures(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->submode) {
    case 0:
        VARS(work)->object->model->mats[2].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[5].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[7].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[8].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[10].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[13].attr_texId = NO_ROUND_TEXTURE;
        break;
    case 1:
        VARS(work)->object->model->mats[2].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[5].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[7].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[8].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[10].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[13].attr_texId = ROUND_TEXTURE;
        break;
    case 2:
        VARS(work)->object->model->mats[2].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[5].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[7].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[8].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[10].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[13].attr_texId = ROUND_TEXTURE;
        break;
    case 3:
        VARS(work)->object->model->mats[2].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[5].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[7].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[8].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[10].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[13].attr_texId = ROUND_TEXTURE;
        break;
    case 4:
        VARS(work)->object->model->mats[2].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[5].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[7].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[8].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[10].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[13].attr_texId = ROUND_TEXTURE;
        break;
    case 5:
        VARS(work)->object->model->mats[2].attr_texId = NO_ROUND_TEXTURE;
        VARS(work)->object->model->mats[5].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[7].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[8].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[10].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[13].attr_texId = ROUND_TEXTURE;
        break;
    case 6:
        VARS(work)->object->model->mats[2].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[5].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[7].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[8].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[10].attr_texId = ROUND_TEXTURE;
        VARS(work)->object->model->mats[13].attr_texId = ROUND_TEXTURE;
        break;
    }
    if (work->tick > FLASH_STEPS) {
        VARS(work)->object->model->mats[3].attr_texId = PLAIN_TEXTURE;
        VARS(work)->object->child->model->mats[1].attr_texId = PLAIN_TEXTURE;
    } else {
        VARS(work)->object->model->mats[3].attr_texId = FLASH_TEXTURE;
        VARS(work)->object->child->model->mats[1].attr_texId = FLASH_TEXTURE;
    }
}

static void platform_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Angle angle;

    if (!frame_step) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, angle);
        njPushMatrix(NULL);
        object = VARS(work)->object;
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
        draw_model(object->model);
        njPopMatrix(1);
        njPushMatrix(NULL);
        object = object->child;
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
        TURN_VIA(njRotateY, VARS(work)->turn, angle);
        draw_model(object->model);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

/* once the lane's mode reaches LANE_ACTIVE, the platform's child spins. after
 * SPIN_STEPS, the platform gives rounds of ROUND_RINGS rings, as many as
 * set_reward_rounds sets */
static void platform_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    NJS_OBJECT *object = &bowling_platform_object;
    Uint16 i;

    if (edit_mode) {
        VARS(work)->object = object;
        platform_draw(task);
    } else {
        switch (work->mode) {
        case PLATFORM_INIT:
            work->mode = PLATFORM_WAIT;
            VARS(work)->object = object;
            collision_init(task, platform_shapes, COLLISION_OBJECT, 1, 2);
            task->destructor = child_destroy;
            task->draw = platform_draw;
            break;
        case PLATFORM_WAIT:
            if (parent->mode >= LANE_ACTIVE) {
                work->mode = PLATFORM_SPIN;
            }
            break;
        case PLATFORM_SPIN:
            if (++work->tick >= SPIN_STEPS) {
                work->mode = PLATFORM_REWARD;
                set_reward_rounds(task);
                work->tick = 0;
            }
            VARS(work)->turn += 0x100;
            platform_draw(task);
            collision_register(work);
            break;
        case PLATFORM_REWARD:
            VARS(work)->turn += 0x100;
            if ((TASK_TIMER(work) += UV_SCROLL) >= UV_WRAP) {
                work->timer = 0;
            }
            /* each meshset below is one triangle. its three uvs scroll in v by
             * timer for this step's draw. the loop after the draw scrolls them
             * back */
            for (i = 0; i < 3; ++i) {
                VARS(work)->object->model->meshsets[0].vertuv[i].tex.v +=
                    work->timer;
                VARS(work)->object->model->meshsets[4].vertuv[i].tex.v +=
                    work->timer;
                VARS(work)->object->model->meshsets[6].vertuv[i].tex.v +=
                    work->timer;
                VARS(work)->object->model->meshsets[9].vertuv[i].tex.v +=
                    work->timer;
                VARS(work)->object->model->meshsets[11].vertuv[i].tex.v +=
                    work->timer;
                VARS(work)->object->model->meshsets[12].vertuv[i].tex.v +=
                    work->timer;
            }

            /* @hack redundant: a test of meshset 9's uvs that never runs. it
             * puts meshset 6's byte offset, 144, in R10 and meshset 9's, 216,
             * in R11 (0x0c90217a, 0x0c902184), which otherwise trade; without
             * it, 13 of 638 words differ */
            if (0) {
                if (VARS(work)->object->model->meshsets[9].vertuv) {
                }
            }
            /* @hack redundant: a test of meshset 11's uvs that never runs. it
             * puts meshset 12's byte offset, 288, in R12 and meshset 11's, 264,
             * in R13 (0x0c90217e, 0x0c902180); without it, 19 of 638 words
             * differ */
            if (0) {
                if (VARS(work)->object->model->meshsets[11].vertuv) {
                }
            }
            /* @hack redundant: a test of meshset 12's uvs that never runs. it
             * keeps meshset 9's byte offset, 216, in R11 and meshset 12's, 288,
             * in R12 (0x0c902184, 0x0c90217e), which otherwise trade; without
             * it, 17 of 638 words differ */
            if (0) {
                if (VARS(work)->object->model->meshsets[12].vertuv) {
                }
            }
            if (work->tick++ > ROUND_WAIT) {
                work->tick = 0;
                if (work->submode == 0) {
                    work->mode = PLATFORM_DONE;
                } else {
                    --work->submode;
                    ring_scatter(work->pos.x, work->pos.y, work->pos.z,
                                 ROUND_RINGS);
                }
            }
            platform_set_textures(task);
            collision_register(work);
            platform_draw(task);
            for (i = 0; i < 3; ++i) {
                VARS(work)->object->model->meshsets[0].vertuv[i].tex.v -=
                    work->timer;
                VARS(work)->object->model->meshsets[4].vertuv[i].tex.v -=
                    work->timer;
                VARS(work)->object->model->meshsets[6].vertuv[i].tex.v -=
                    work->timer;
                VARS(work)->object->model->meshsets[9].vertuv[i].tex.v -=
                    work->timer;
                VARS(work)->object->model->meshsets[11].vertuv[i].tex.v -=
                    work->timer;
                VARS(work)->object->model->meshsets[12].vertuv[i].tex.v -=
                    work->timer;
            }
            VARS(work)->object->model->mats[3].attr_texId = PLAIN_TEXTURE;
            VARS(work)->object->child->model->mats[1].attr_texId =
                PLAIN_TEXTURE;
            sound_play_at_for(SE_REWARD, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 2);
            break;
        case PLATFORM_DONE:
            if (work->tick++ > ROUND_WAIT) {
                work->tick = 0;
            }
            VARS(work)->turn += 0x100;
            platform_set_textures(task);
            collision_register(work);
            platform_draw(task);
            VARS(work)->object->model->mats[3].attr_texId = PLAIN_TEXTURE;
            VARS(work)->object->child->model->mats[1].attr_texId =
                PLAIN_TEXTURE;
            break;
        }
    }
}

void bowling_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR delta;
    NJS_VECTOR local;
    NJS_OBJECT *object;
    Angle angle;

    if (placement_editing(task)) {
        /* the set editor draws pin 1 and the back row's ends, pins 7 and 10 */
        njPushMatrix(NULL);
        /* @hack redundant: an add of 0 to the lane's y, FADD FR15,FR5 at
         * 0x0c902616; without it, 92 of 240 words differ */
        njTranslate(NULL, work->pos.x, work->pos.y + 0.0F, work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, angle);
        texture_set_stage();
        object = &bowling_pin_object;
        draw_tree(object);
        njPushMatrix(NULL);
        njTranslate(NULL, 18.0F, 0.0F, -30.0F);
        draw_tree(object);
        njPopMatrix(1);

        /* @hack redundant: calls of njPopMatrix and draw_tree that never run.
         * they put njPopMatrix in R12, njPushMatrix in R10, draw_tree in R14
         * and the pin in R13 (0x0c9025f4 to 0x0c902634), where otherwise each
         * pair trades; without it, 16 of 240 words differ */
        if (0) {
            njPopMatrix(0);
            draw_tree(NULL);
        }
        njPushMatrix(NULL);
        njTranslate(NULL, -18.0F, 0.0F, -30.0F);
        draw_tree(object);
        njPopMatrix(1);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case LANE_INIT:
            work->mode = LANE_WAIT;
            /* @hack redundant: an add of 0 to the lane's y, FADD FR15,FR3 at
             * 0x0c9026ac; without it, 49 of 240 words differ */
            work->pos.y += 0.0F;
            task->destructor = parent_destroy;
            task_create_children(bowling_children, task);
            break;
        case LANE_WAIT:
            if (!edit_mode) {
                delta.x = player_work[0]->pos.x - work->pos.x;
                delta.y = player_work[0]->pos.y - work->pos.y - 1.0F;
                delta.z = player_work[0]->pos.z - work->pos.z;
                njPushMatrix(&_unit_matrix_);
                /* turn delta into the lane's frame. 0xffff - ang.y undoes the
                 * lane's turn to within one angle unit */
                TURN_VIA(njRotateY, 0xffff - work->ang.y, angle);
                njCalcVector(NULL, &delta, &local);
                njPopMatrix(1);
                /* the lane's box runs from the back row to 100 in front of
                 * pin 1. it reaches 53 to each side. delta.y is measured
                 * from 1 above the lane. so the box reaches from 9 below the
                 * lane to 11 above */
                if ((player_work[0]->flags & PLAYER_ON_LAND) &&
                    njAbs(local.x) < 53.0F && njAbs(local.y) < 10.0F &&
                    -30.0F < local.z && local.z < 100.0F) {
                    work->mode = LANE_ACTIVE;
                }
                if (work->flags & ALL_PINS) {
                    work->mode = LANE_CLEAR;
                    sound_play(SE_PIN_DOWN, 0, 0, 0);
                }
            }
            goto run_children;
        case LANE_ACTIVE:
            if (work->flags & ALL_PINS) {
                work->mode = LANE_CLEAR;
                sound_play(SE_PIN_DOWN, 0, 0, 0);
            }
            if (work->tick++ > ACTIVE_STEPS) {
                work->mode = LANE_CLEAR;
            }
            goto run_children;
        case LANE_CLEAR:
            /* @hack redundant: a goto to the next statement, which loads
             * task_step_children's address into R3 at 0x0c902848; falling into
             * the label, it takes R2 and 2 of 240 words differ */
            goto run_children;
        /* @hack spelling: the cases' gotos to one call of task_step_children,
         * at 0x0c902848. called in each case of bowling_step, its address takes
         * R9, where the image keeps LANE_CLEAR, and 111 of its 240 words and
         * 10 of the 81 words of its pools and pad differ */
        run_children:
            task_step_children(task);
            break;
        case LANE_IDLE:
            break;
        }
    }
}

static void ball_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle angle;

    if (!frame_step) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, angle);
        TURN_VIA(njRotateZ, work->ang.z, angle);
        draw_tree(VARS(work)->object);
        njPopMatrix(1);
    }
}

/* draw the ball's shadow at scale 2, then let the ball fall. scl.y is its
 * speed up the y axis. the ball stops BALL_RADIUS above the floor, with scl.y
 * back at -BALL_GRAVITY */
static void ball_fall(struct task *task)
{
    struct task_work *work = task->work;
    Float y;
    Float floor = shadow_draw(work, 2.0F) + BALL_RADIUS;

    work->scl.y += -BALL_GRAVITY;
    y = work->pos.y + work->scl.y;
    if (y < floor) {
        work->pos.y = floor;
        work->scl.y = -BALL_GRAVITY;
    } else {
        work->pos.y = y;
    }
}

/* move the ball its speed along its heading, ang.x. scl.x and scl.z keep
 * where the ball was. its center stays BALL_RADIUS from the nearest wall on
 * each side in x and z. each wall it meets leaves it WALL_KEEP of its
 * speed. the heading then takes the way the ball moved. where the walls held
 * the ball in place, the heading turns at random instead. ang.y faces a
 * quarter turn from the heading, and ang.z rolls the ball about z */
static void ball_roll(struct task *task)
{
    struct task_work *work = task->work;
    struct sides_query query;
    Float min_x;
    Float max_x;
    Float min_z;
    Float max_z;
    Float dx;
    Float dz;

    work->scl.x = work->pos.x;
    work->scl.z = work->pos.z;
    work->pos.x += VARS(work)->speed * njSin(work->ang.x);
    work->pos.z += VARS(work)->speed * njCos(work->ang.x);
    query.pos = work->pos;
    ground_find_sides(&query);
    min_x = query.sides[SIDE_MINUS_X].level + BALL_RADIUS;
    max_x = query.sides[SIDE_PLUS_X].level - BALL_RADIUS;
    min_z = query.sides[SIDE_MINUS_Z].level + BALL_RADIUS;
    max_z = query.sides[SIDE_PLUS_Z].level - BALL_RADIUS;
    if (min_x > work->pos.x) {
        work->pos.x = min_x;
        VARS(work)->speed *= WALL_KEEP;
    }
    if (work->pos.x > max_x) {
        work->pos.x = max_x;
        VARS(work)->speed *= WALL_KEEP;
    }
    if (min_z > work->pos.z) {
        work->pos.z = min_z;
        VARS(work)->speed *= WALL_KEEP;
    }
    if (work->pos.z > max_z) {
        work->pos.z = max_z;
        VARS(work)->speed *= WALL_KEEP;
    }
    dx = work->pos.x - work->scl.x;
    dz = work->pos.z - work->scl.z;
    if (dx == 0.0F && dz == 0.0F) {
        work->ang.x += (int)(RANDOM(-0.5F, 0.5F) * HELD_TURN_RANGE);
    } else {
        work->ang.x = (int)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
    }
    work->ang.y = work->ang.x + 0x4000;
    work->ang.z += (int)(VARS(work)->speed * ROLL_TURN);
    if (VARS(work)->speed < SINK_SPEED) {
        work->mode = BALL_SINKING;
    }
}

/* the ball, a child of O BALL, whose mode it sets */
static void ball_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    NJS_OBJECT *object = &bowling_ball_object;

    if (edit_mode) {
        VARS(work)->object = object;
        ball_draw(task);
    } else {
        /* @hack spelling: task_step_children called through step_children,
         * which loads its address into R12 and the constant 2 into R11
         * (0x0c902b54, 0x0c902b5a); called by name, the two trade registers
         * and 10 of 252 words differ */
        void (*step_children)(struct task *) = task_step_children;
        switch (work->mode) {
        case BALL_INIT:
            work->mode = BALL_WAIT;
            /* the spec sets no scale, so scl.x is 0 and the speed starts at
             * 1 */
            VARS(work)->speed = work->scl.x + 1.0F;
            work->scl.x = work->scl.z = 0.0F;
            work->scl.y = -BALL_GRAVITY;
            work->ang.x = work->ang.z = 0;
            VARS(work)->object = object;
            task->draw = ball_draw;
            task->destructor = child_destroy;
            collision_init(task, ball_shapes, COLLISION_ATTACK, 1, 2);
            task->work->collision->radius = 9.0F;
            if (stage_get_stage_act() == STAGE_TWINKLE_MIRRORS) {
                twinkle_reflect(task);
            }
            break;
        case BALL_WAIT:
            if ((work->collision->flags & COLLISION_HIT) &&
                work->collision->hit->kind == COLLISION_PLAYER) {
                parent->mode = work->mode = BALL_ROLLING;
                /* away from player 0, whichever player touched it */
                work->ang.x =
                    (int)(atan2f(work->pos.x - player_work[0]->pos.x,
                                 work->pos.z - player_work[0]->pos.z) *
                          ANGLE_PER_RADIAN);
                work->ang.y = work->ang.x + 0x4000;
                work->collision->shapes[0].flags |= SHAPE_OFF;
                work->collision->shapes[1].levels =
                    (work->collision->shapes[1].levels & ~SHAPE_ATTACK) |
                    ATTACK(3);
            }
            shadow_draw(work, 2.0F);
            step_children(task);
            ball_draw(task);
            collision_register(work);
            break;
        case BALL_ROLLING:
            if (placement_try_leave(task)) {
                break;
            }
            /* @bug logic: the test never holds. the ball's record is an
             * attack, and the checks test no attack against another, so the
             * ball sinks only once ball_roll slows it. CMP/EQ #1 at
             * 0x0c902cfa reads the hit's kind */
            if ((work->collision->flags & COLLISION_HIT) &&
                work->collision->hit->kind == COLLISION_ATTACK) {
                parent->mode = work->mode = BALL_SINKING;
            }
            ball_roll(task);
            sound_play_at_for(SE_ROLL, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 2);
            ball_fall(task);
            step_children(task);
            ball_draw(task);
            collision_register(work);
            break;
        case BALL_SINKING: {
            NJS_ARGB material;

            if (placement_try_leave(task)) {
                break;
            }
            work->pos.y += -0.5F;
            step_children(task);
            material = ball_material;
            draw_attr_save();
            draw_attr_set(0, NJD_FLAG_USE_ALPHA);
            njSetConstantMaterial(&material);
            ball_draw(task);

            /* @hack redundant: material.a, stored after
             * njSetConstantMaterial has copied the material, so Ninja's
             * constant material keeps the -0.5 alpha. the store is the image's
             * FMOV.S at 0x0c902d8e; without it, 22 of 252 words differ */
            material.a = 0.0F;
            draw_attr_restore();
            break;
        }
        case BALL_IDLE:
            break;
        }
    }
}

void bowling_ball_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle angle;

    if (placement_editing(task)) {
        /* the set editor draws the ball and, in act 1, its gate, where
         * bowling_ball_children places them */
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, angle);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, BALL_RADIUS, 0.0F);
        draw_tree(&bowling_ball_object);
        njPopMatrix(1);
        if (stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 1)) {
            njPushMatrix(NULL);
            njTranslate(NULL, 0.0F, 0.0F, -110.0F);
            njRotateY(NULL, 0x8000);
            draw_tree(&bowling_gate_object);
            njPopMatrix(1);
        }
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case BALL_INIT:
            work->mode = BALL_WAIT;
            task->destructor = parent_destroy;
            task_create_children(bowling_ball_children, task);
            break;
        case BALL_WAIT:
        case BALL_ROLLING:
        case BALL_SINKING:
        case BALL_IDLE:
            task_step_children(task);
            break;
        }
    }
}
