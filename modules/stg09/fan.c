/* O FANFAN: a fan that lifts players in its column of air and blows bits
 * about */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/fan.h"
#include "stg09/helpers.h"
#include "system/cache.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT fan_object;
extern NJS_OBJECT fan_bit_object;

/* the task starts only within 1500 of the camera or player 0, and destroys
 * itself otherwise. placement_try_leave_by takes the range squared */
#define RANGE (1500.0F * 1500.0F)

/* the column's radius is |scl.x + COLUMN_MARGIN|, and its half height
 * |scl.y + COLUMN_MARGIN|. its center is COLUMN_RISE above the fan */
#define COLUMN_MARGIN 10.0F
#define COLUMN_RISE 20.0F
/* past the draw, the step runs only with player 0 or 1 in the column grown
 * NEAR_SCALE times. the fan draws, and its bits move, only while the camera
 * is in the column grown CAMERA_SCALE times */
#define NEAR_SCALE 2.5F
#define CAMERA_SCALE 3.0F

/* the numbers whose fans fan_idle reports, 0 to FAN_NUMBERS - 1. a fan takes
 * its number from scl.z, truncated to a byte */
#define FAN_NUMBERS 5

/* the bits in the buffer the fans share. every fan that holds the buffer
 * moves and draws the same bits */
#define BITS 650
/* a bit higher than scl.y + BIT_TOP, or farther across than
 * scl.x + BIT_REACH, is placed again */
#define BIT_TOP 210.0F
#define BIT_REACH 60.0F
/* a bit lower than BIT_REST_HEIGHT and slower than BIT_REST_SPEED rests */
#define BIT_REST_HEIGHT 0.1F
#define BIT_REST_SPEED 0.001F

#define SE_BLOW 263
#define SE_LIFT 264

/* fan modes, in work->mode */
enum fan_mode {
    FAN_WAITING, /* for a player in the column */
    FAN_BLOWING,
    FAN_RESTING
};

struct fan_bit {
    NJS_POINT3 pos; /* its offset from the fan, which the swirl turns */
    NJS_VECTOR speed;
    struct angles ang;
    NJS_VECTOR spin;
    /* the share of its speed it keeps each frame, times fan_bit_keep */
    Float keep;
    Float scale;
};

struct fan_vars {
    struct fan_bit *bits; /* the shared buffer, while the fan holds it */
    Sint16 swirl; /* the bits' turn about y round the fan */
    Sint16 unk_06;
    Float strength; /* 0 to fan_strength_max */
};

#define VARS(work) ((struct fan_vars *)&(work)->vars)
#define NUMBER(work) ((Uint8)(work)->scl.z)

/* TRUE while player 0 or 1 is in the column. it sets player to player 0's
 * work */
#define IN_COLUMN(work, player)                                                \
    ((((player) = player_work[0]) &&                                           \
      fan_reaches((work), &(player)->pos, 1.0F)) ||                            \
     (player_work[1] && fan_reaches((work), &player_work[1]->pos, 1.0F)))

#define SET_MODE(work, to) ((work)->mode = (to), (work)->timer = 0)

/* the lift at full strength, which the fan doubles for a player and for its
 * bits */
static Float fan_lift = 0.097F;
static Float fan_strength_max = 9.0F;
/* the strength gained each step the fan blows with a player in the
 * column */
static Float fan_strength_up = 0.1F;
/* the share of fan_strength_up lost each step the fan doesn't blow */
static Float fan_strength_down = 0.45F;
/* the least steps of a rest. a rest that ends a whole blow adds up to 150
 * at random. one that starts as the players leave adds up to 300 */
static int fan_rest_steps = 120;
/* the least steps of a blow, to which each blow adds up to 300 at random */
static int fan_blow_steps = 1000;
/* the chance of a gust, each step the fan lifts a player */
static Float fan_gust_chance = 0.15F;
/* the most a gust changes the lift, either way */
static Float fan_gust = 0.25F;
/* the share of its speed a bit keeps each frame, times its own keep */
static Float fan_bit_keep = 0.97F;
static Float fan_bit_spin = 6.0F; /* a bit's least spin. the most is twice it */
/* the y speed a moving bit gains each frame, before the lift */
static Float fan_bit_fall = -0.09F;
static Float fan_bit_weight = 0.38F; /* divides the lift on bits */
/* the chance each frame that a resting bit moves */
static Float fan_bit_stir = 0.03F;
static Float fan_bit_scale_max = 3.0F;
/* the degrees the bits swirl each frame, for each unit of strength */
static Float fan_swirl = 0.04F;
static struct fan_bit *fan_bits = NULL;
static int fan_bit_users = 0;
static int fans_idle[FAN_NUMBERS];

static void fan_destroy(struct task *task);
static void fan_run(struct task *task);
static void fan_draw(struct task *task);
static void fan_bits_make(struct task_work *work);
static void fan_bits_free(struct task_work *work);
static int fan_bits_move(Float lift, struct task_work *work);
static void fan_bits_draw(struct task_work *work);

/* TRUE while pos is in the column grown scale times */
static Bool fan_reaches(struct task_work *work, NJS_POINT3 *pos, Float scale)
{
    Float radius = _builtin_fabsf(work->scl.x + COLUMN_MARGIN);
    Float height = _builtin_fabsf(work->scl.y + COLUMN_MARGIN);
    Float x = work->pos.x - pos->x;
    Float y = work->pos.y + COLUMN_RISE;
    Float z = work->pos.z - pos->z;

    y = _builtin_fabsf(y - pos->y);
    if (y > scale * height) {
        return FALSE;
    }
    y = njSqrt((x * x) + (z * z));
    if (y <= scale * radius) {
        return TRUE;
    }
    return FALSE;
}

static Bool fan_alone(struct task_work *work)
{
    struct task_work *player;

    if ((player = player_work[0]) &&
        fan_reaches(work, &player->pos, NEAR_SCALE)) {
        return FALSE;
    }
    if ((player = player_work[1]) &&
        fan_reaches(work, &player->pos, NEAR_SCALE)) {
        return FALSE;
    }
    return TRUE;
}

void fan_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        work->mode = FAN_WAITING;
        work->timer = 0;
        VARS(work)->strength = 0.0F;
        VARS(work)->swirl = 0;
        VARS(work)->bits = NULL;
        task->step = fan_run;
        task->draw = fan_draw;
        task->destructor = fan_destroy;
    }
}

static void fan_destroy(struct task *task)
{
    struct task_work *work = task->work;

    fan_bits_free(work);
}

static void fan_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    Float lift;
    Float gust;
    NJS_POINT3 middle;

    fan_draw(task);
    if (!fan_reaches(work, &camera_work->pos, CAMERA_SCALE)) {
        fan_bits_free(work);
    }
    if (fan_alone(work)) {
        return;
    }
    if (placement_editing(task)) {
        middle = work->pos;
        middle.y += COLUMN_RISE;
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "->XZ Size");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "->Y Size");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> ID ");
        casino_draw_cylinder(&middle, work->scl.x + COLUMN_MARGIN,
                             work->scl.y + COLUMN_MARGIN);
        return;
    }
    switch (work->mode) {
    default:
        if (IN_COLUMN(work, player)) {
            SET_MODE(work, FAN_BLOWING);
            /* @hack type: fan_blow_steps read as an Sint16, MOV.W at
             * 0x0c919812. nothing writes fan_blow_steps, and its 1000 fits an
             * Sint16. as an int, it is MOV.L, and 1 of 457 words differ */
            work->timer =
                (int)(rand() * (300.0F * RAND_SCALE)) + (Sint16)fan_blow_steps;
        }
        /* @hack spelling: the decrease written out, under which the lift
         * below takes FR13 (FMOV FR2,FR13 at 0x0c919a1e). as -=, 8 of 457
         * words differ */
        VARS(work)->strength =
            VARS(work)->strength - (fan_strength_up * fan_strength_down);
        if (VARS(work)->strength < 0.0F) {
            VARS(work)->strength = 0.0F;
        }
        break;
    case FAN_RESTING:
        if (--TASK_TIMER(work) <= 0) {
            SET_MODE(work, FAN_WAITING);
        }
        /* @hack spelling: the decrease written out, as in the case above.
         * as -=, 8 of 457 words differ */
        VARS(work)->strength =
            VARS(work)->strength - (fan_strength_up * fan_strength_down);
        if (VARS(work)->strength < 0.0F) {
            VARS(work)->strength = 0.0F;
        }
        break;
    case FAN_BLOWING:
        sound_play_at_for(SE_BLOW, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 30);
        if (IN_COLUMN(work, player)) {
            VARS(work)->strength += fan_strength_up;
            if (VARS(work)->strength >= fan_strength_max) {
                VARS(work)->strength = fan_strength_max;
            }
            if (--TASK_TIMER(work) <= 0) {
                SET_MODE(work, FAN_RESTING);
                /* @hack type: fan_rest_steps read as an Sint16, MOV.W at
                 * 0x0c91995c. nothing writes fan_rest_steps, and its 120 fits
                 * an Sint16. as an int, 1 of 457 words differ */
                work->timer = (int)(rand() * (150.0F * RAND_SCALE)) +
                              (Sint16)fan_rest_steps;
            }
        } else {
            SET_MODE(work, FAN_RESTING);
            /* @hack type: fan_rest_steps read as an Sint16, MOV.W at
             * 0x0c9199b4. nothing writes fan_rest_steps, and its 120 fits an
             * Sint16. as an int, 1 of 457 words differ */
            work->timer =
                (int)(rand() * (300.0F * RAND_SCALE)) + (Sint16)fan_rest_steps;
        }
        break;
    }
    work->ang.y += (Angle)(VARS(work)->strength * ANGLE_PER_DEGREE);
    if (IN_COLUMN(work, player) && VARS(work)->strength > 0.0F) {
        lift = VARS(work)->strength / fan_strength_max * fan_lift;
        if (rand() * RAND_SCALE <= fan_gust_chance) {
            /* a gust draws three random numbers, and uses only the second */
            rand();
            gust = ((rand() * RAND_SCALE) - 0.5F) * fan_gust * 2.0F;
            rand();
            lift += gust;
        }
        if (!frame_step) {
            if ((player = player_work[0]) &&
                fan_reaches(work, &player->pos, 1.0F)) {
                player_updraft(0, 0.0F, lift * 2.0F, 0.0F);
            }
            if ((player = player_work[1]) &&
                fan_reaches(work, &player->pos, 1.0F)) {
                player_updraft(1, 0.0F, lift * 2.0F, 0.0F);
            }
            if (fan_reaches(work, &camera_work->pos, CAMERA_SCALE)) {
                fan_bits_make(work);
                fan_bits_move(lift * 2.0F, work);
            }
        }
        sound_play_for(SE_LIFT, (int)work, 1, 0, 30);
    } else if (!frame_step &&
               fan_reaches(work, &camera_work->pos, CAMERA_SCALE)) {
        fan_bits_make(work);
        fan_bits_move(0.0F, work);
    }
    if (NUMBER(work) < FAN_NUMBERS) {
        /* @hack spelling: the flag picked as 0 or 1, which SHC branches for
         * (CMP/EQ #1,R0 at 0x0c919b66). as mode != FAN_BLOWING, 15 of 457
         * words differ */
        fans_idle[NUMBER(work)] = work->mode == FAN_BLOWING ? 0 : 1;
    }
    collision_register(work);
}

static void fan_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (fan_reaches(work, &camera_work->pos, CAMERA_SCALE)) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_tree(&fan_object);
        njPopMatrix(1);
        fan_bits_draw(work);
    }
}

int fan_idle(Sint8 number)
{
    if (number < FAN_NUMBERS) {
        return fans_idle[number];
    }
    return 0;
}

static void fan_bit_place(struct fan_bit *bit, struct task_work *work)
{
    Angle ang;
    Float distance;

    if (bit) {
        ang = (Angle)(rand() * (720.0F * ANGLE_PER_DEGREE * RAND_SCALE));
        distance = rand() * (work->scl.x + COLUMN_MARGIN) * RAND_SCALE;
        bit->pos.x = distance * njCos(ang);
        bit->pos.z = distance * njSin(ang);
        bit->pos.y = 0.0F;
        bit->speed.x = 0.0F;
        bit->speed.y = 0.0F;
        bit->speed.z = 0.0F;
        bit->ang.x = NJM_DEG_ANG(90);
        bit->ang.y = (Angle)(rand() * (720.0F * ANGLE_PER_DEGREE * RAND_SCALE));
        bit->ang.z = 0;
        bit->spin.x = (2.0F - (rand() * RAND_SCALE)) * fan_bit_spin;
        bit->spin.y = (2.0F - (rand() * RAND_SCALE)) * fan_bit_spin;
        bit->spin.z = (2.0F - (rand() * RAND_SCALE)) * fan_bit_spin;
        bit->keep = 0.8F + (rand() * (0.2F * RAND_SCALE));
        bit->scale = 0.6F + (rand() * (fan_bit_scale_max - 0.6F) * RAND_SCALE);
    }
}

/* take a share of the bits' buffer, which its first user allocates. returns
 * the buffer, or NULL when it can't be allocated */
static struct fan_bit *fan_bits_take(void)
{
    if (!fan_bits && !fan_bit_users) {
        fan_bits = heap_alloc(BITS * sizeof(struct fan_bit));
    }
    if (fan_bits) {
        fan_bit_users++;
    }
    return fan_bits;
}

static void fan_bits_give_back(void)
{
    if (fan_bits && --fan_bit_users == 0) {
        heap_free(fan_bits);
        fan_bits = NULL;
    }
}

static void fan_bits_make(struct task_work *work)
{
    int i;

    if (!VARS(work)->bits) {
        VARS(work)->bits = fan_bits_take();
        if (VARS(work)->bits) {
            for (i = 0; i < BITS; i++) {
                fan_bit_place(&VARS(work)->bits[i], work);
            }
        }
    }
}

static void fan_bits_free(struct task_work *work)
{
    if (VARS(work)->bits) {
        fan_bits_give_back();
        VARS(work)->bits = NULL;
    }
}

/* move each bit a step, lifted by lift, and swirl the bits on. returns the
 * bits at least 1 up, which no caller reads */
static int fan_bits_move(Float lift, struct task_work *work)
{
    int aloft = 0;
    int i;
    struct fan_bit *bit;
    Float x;
    Float z;
    Float speed;

    if (!VARS(work)->bits) {
        return 0;
    }
    lift /= fan_bit_weight;
    for (i = 0; i < BITS; i++) {
        bit = &VARS(work)->bits[i];
        x = bit->pos.x;
        z = bit->pos.z;
        if (bit->pos.y > work->scl.y + BIT_TOP ||
            njSqrt((x * x) + (z * z)) > work->scl.x + BIT_REACH) {
            fan_bit_place(bit, work);
        } else {
            {
                /* @hack spelling: speed scaled through a Float cursor, which
                 * ADD #4 steps (0x0c919f9c to 0x0c919fbc). through the members,
                 * 841 of 885 words differ */
                Float *to = &bit->speed.x;

                *to++ *= fan_bit_keep * bit->keep;
                *to++ *= fan_bit_keep * bit->keep;
                *to *= fan_bit_keep * bit->keep;
            }
            speed = njScalor(&bit->speed);
            if (bit->pos.y >= BIT_REST_HEIGHT || speed >= BIT_REST_SPEED ||
                rand() * RAND_SCALE < fan_bit_stir) {
                {
                    /* @hack spelling: speed added into pos through Float
                     * cursors, which FMOV.S @R4+ and ADD #4 step (0x0c919ffa
                     * to 0x0c91a016). through the members, 828 of 885 words
                     * differ */
                    Float *to = &bit->pos.x;
                    Float *by = &bit->speed.x;

                    *to++ += *by++;
                    *to++ += *by++;
                    *to += *by;
                }
                bit->ang.x += (Angle)(bit->spin.x * speed * ANGLE_PER_DEGREE);
                bit->ang.y += (Angle)(bit->spin.y * speed * ANGLE_PER_DEGREE);
                bit->ang.z += (Angle)(bit->spin.z * speed * ANGLE_PER_DEGREE);
                if (bit->pos.y < 0.0F) {
                    bit->pos.y = 0.0F;
                    bit->speed.y = 0.0F;
                } else {
                    bit->speed.y += fan_bit_fall + lift;
                }
            }
        }
        if (bit->pos.y >= 1.0F) {
            aloft++;
        }
    }
    VARS(work)->swirl +=
        (Angle)(VARS(work)->strength * fan_swirl * ANGLE_PER_DEGREE);
    return aloft;
}

static void fan_bits_draw(struct task_work *work)
{
    struct fan_bit *bit;
    NJS_MODEL *model;
    Float scale;
    Angle ang;
    int count;

    if (frame_step) {
        return;
    }
    if (!VARS(work)->bits) {
        return;
    }
    bit = VARS(work)->bits;
    count = BITS;
    model = fan_bit_object.model;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, VARS(work)->swirl, ang);
    njPushMatrix(NULL);
    while (count--) {
        scale = bit->scale;
        njTranslateV(NULL, &bit->pos);
        TURN(njRotateZ, bit->ang.z);
        TURN(njRotateX, bit->ang.x);
        TURN(njRotateY, bit->ang.y);
        njScale(NULL, scale, scale, scale);
        draw_model(model);
        matrix_load_parent();
        bit++;
    }
    njPopMatrix(2);
}
