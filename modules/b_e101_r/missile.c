/* the homing missiles E-101 mkII fires. a missile coasts out, slowing, then
 * plays its motion through as it starts to spin. then it homes on player 0
 * until it bursts */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_e101_r/files.h"
#include "b_e101_r/missile.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/draw.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "enemy/enemy.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/task.h"
#include "system/util.h"

/* the missile's object and motion, as byte offsets in E101R.NB, and the
 * motion's last frame */
#define MISSILE_OBJECT 0x9a1c
#define MISSILE_MOTION 0x2c2a4
#define MOTION_END 9.0F

/* a missile's record, its task's extra */
struct missile {
    NJS_VECTOR speed;
    Angle spin; /* its turn about z each step */
    Sint32 coasted; /* the steps it has coasted */
    Sint32 life; /* the steps it has left to home */
    Uint8 unk_18[4];
    Bool released; /* TRUE once the destructor has run */
    Bool flashing;
    Float floor_y; /* the floor's height under it, from its last shadow_draw */
    Float frame; /* its motion's frame */
};

#define MISSILE_COAST 50 /* the steps a missile coasts */
/* the share of its speed a missile keeps each step until it homes */
#define MISSILE_KEEP 0.9F
/* the spin a missile speeds up toward once it has coasted */
#define MISSILE_SPIN NJM_DEG_ANG(8)

#define MISSILE_LIFE 250 /* the steps a missile homes before it bursts */
#define MISSILE_BLINK 24 /* it blinks with fewer steps left than this */
#define MISSILE_GLOW 5 /* it stays white with fewer steps left than this */

/* the most a missile turns toward player 0 each step */
#define MISSILE_TURN NJM_DEG_ANG(5)

/* a burst's puff, at half a bomb puff's size */
#define BURST_SCALE 0.5F

#define SE_MISSILE_FLIGHT 0x1ff
#define SE_MISSILE_BURST 0x203

static void missile_destroy(struct task *task);
static void missile_coast(struct task *task);
static void missile_launch(struct task *task);
static void missile_home(struct task *task);
static void missile_draw(struct task *task);

static struct collision_shape missile_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE | LOCKON_NO_NUMBER,
    ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
        SHAPE_HURTS_OBJECTS,
    PUSH_ENEMIES,
    {0.0F, 0.0F, 0.0F},
    5.0F,
    0.0F,
    0.65F,
    {0, 0, 0},
    0,
    NULL,
};

Sint32 e101_mk2_missiles_flying = 0;

/* the params of the exhaust's puffs, which every missile shares and every
 * puff keeps for its life. missile_puff sets pos and speed before each puff,
 * which copies them */
static struct dust_params missile_exhaust = {
    0.7F,
    0.03F,
    0.19F,
    0.05F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.56F, 0.56F, 0.7F},
};

void e101_mk2_missiles_reset(void)
{
    e101_mk2_missiles_flying = 0;
}

void e101_mk2_missiles_explode(void)
{
    e101_mk2_missiles_flying = -1;
}

void e101_mk2_missile_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Angle ang)
{
    struct task *task;
    struct task_work *work;
    struct missile *missile;

    if ((task = task_create(TASK_WORK, 3, missile_coast)) != NULL) {
        work = task->work;
        collision_init(task, &missile_shape, COLLISION_ENEMY, 1, 1);
        enemy_alloc_record(task, work);
        /* @hack spelling: the record stored to task->extra and missile in one
         * statement, which keeps pos in R12 and speed in R11 (0x0c9064c6,
         * 0x0c9064be); as two statements, the two trade registers, and 8 of
         * 89 words differ */
        task->extra = missile = heap_alloc(sizeof(struct missile));
        VECTOR_COPY(&work->pos, pos);
        work->ang.y = 0x8000 - ang;
        work->ang.x = 0;
        VECTOR_COPY(&missile->speed, speed);
        missile->spin = 0;
        missile->coasted = 0;
        missile->life = MISSILE_LIFE;
        missile->released = FALSE;
        missile->flashing = FALSE;
        missile->floor_y = -9999.0F;
        missile->frame = 0.0F;
        ++e101_mk2_missiles_flying;
        task->draw = missile_draw;
        task->destructor = missile_destroy;
    }
}

/* take the missile off e101_mk2_missiles_flying, once. a count of 0 or
 * less stays as it is */
static void missile_destroy(struct task *task)
{
    struct missile *missile = task->extra;

    /* @hack redundant: an else after the return, which keeps TRUE in R2 and the
     * count in R3 (0x0c9065c8, 0x0c9065ce); without it, the two trade
     * registers, and 4 of 15 words differ */
    if (missile->released) {
        return;
    } else {
        missile->released = TRUE;
        if (e101_mk2_missiles_flying > 0) {
            e101_mk2_missiles_flying--;
        }
    }
}

static void missile_burst(struct task *task, Float scale)
{
    struct task_work *work = task->work;

    sound_play_at(SE_MISSILE_BURST, 0, 0, 0, work->pos.x, work->pos.y,
                  work->pos.z);
    bomb_create(&work->pos, scale);
    task_kill(task);
}

/* burst the missile while the count is negative, or once its life runs out.
 * a hit that hurts it while E-102 is locked on to it bursts it too. so does
 * a touch of a player */
static void missile_try_burst(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *target = LOCKON_TARGET(task);
    struct missile *missile = task->extra;
    struct collision_work *hit;

    if (e101_mk2_missiles_flying < 0) {
        lockon_release(task);
        missile_burst(task, BURST_SCALE);
        return;
    }
    /* @hack spelling: the lock-on test in an if of its own, apart from the
     * count's, which keeps missile in R6 and target in R5 (0x0c90662c,
     * 0x0c906632); as one ||, the two trade registers, and 5 of 56 words
     * differ */
    if ((work->flags & COLLISION_DAMAGED) && target->lockon != NULL) {
        lockon_release(task);
        missile_burst(task, BURST_SCALE);
        return;
    }
    if (work->collision->flags & COLLISION_HIT) {
        hit = work->collision->hit;
        if (hit != NULL && hit->kind == COLLISION_PLAYER) {
            missile_burst(task, BURST_SCALE);
            return;
        }
    }
    if (missile->life <= 0) {
        missile_burst(task, BURST_SCALE);
    }
}

static void missile_steer(struct task *task, NJS_POINT3 *target, int max,
                          int min)
{
    struct task_work *work = task->work;
    Float x = target->x - work->pos.x;
    Float z = target->z - work->pos.z;
    Angle heading = -(Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
    Angle turn = angle_difference(work->ang.y, heading);

    work->ang.y += CLAMP(turn, min, max);
}

/* climb or dive toward the height of player 0's center. the y speed never
 * carries the missile past that height, and stays within 1 a step */
static void missile_climb(struct task *task)
{
    struct task_work *work = task->work;
    struct missile *missile = task->extra;
    Float height =
        player_work[0]->pos.y + player_get(0)->consts.center_y - work->pos.y;

    if (njAbs(height) < 0.1F) {
        missile->speed.y *= 0.7F;
    } else {
        missile->speed.y += (height - missile->speed.y) * 0.003F;
    }
    if (height >= 0.0F) {
        if (missile->speed.y >= height) {
            missile->speed.y = height;
        }
    } else if (missile->speed.y <= height) {
        missile->speed.y = height;
    }
    missile->speed.y = CLAMP(missile->speed.y, -1.0F, 1.0F);
}

/* add thrust to speed in x and z, along the y angle ang->y */
static void missile_thrust(NJS_VECTOR *speed, struct angles *ang, Float thrust)
{
    NJS_VECTOR push;
    NJS_VECTOR ahead;
    NJS_MATRIX matrix;

    push.x = thrust;
    push.y = push.z = 0.0F;
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, ang->y);
    njCalcPoint(&matrix, &push, &ahead);
    speed->x += ahead.x;
    speed->z += ahead.z;
}

static void missile_limit(NJS_VECTOR *speed, Float max)
{
    Float length = vector_magnitude(speed);
    Float scale;

    if (length > max) {
        scale = max / length;
        VECTOR_SCALE(speed, speed, scale);
    }
}

static void missile_pitch(NJS_VECTOR *speed, struct angles *ang)
{
    Float across = vector_magnitude_across(speed);

    ang->x = -(Angle)(atan2f(speed->y, across) * ANGLE_PER_RADIAN);
}

/* a glowing puff of exhaust 2 behind the missile, moving back at 2 a
 * step */
static void missile_puff(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR back;

    njPushMatrix(&_unit_matrix_);
    back.y = back.x = 0.0F;
    back.z = -2.0F;
    TURN(njRotateY, work->ang.y + 0x4000);
    TURN(njRotateX, work->ang.x);
    njCalcPoint(NULL, &back, &missile_exhaust.speed);
    njPopMatrix(1);
    VECTOR_SUM(&missile_exhaust.pos, &work->pos, &missile_exhaust.speed);
    dust_create_glow(&missile_exhaust);
}

static void missile_coast(struct task *task)
{
    struct task_work *work = task->work;
    struct missile *missile = task->extra;

    missile_try_burst(task);
    if (++missile->coasted >= MISSILE_COAST) {
        task->step = missile_launch;
    }
    VECTOR_SUM(&work->pos, &work->pos, &missile->speed);
    VECTOR_SCALE(&missile->speed, &missile->speed, MISSILE_KEEP);
    collision_register(work);
    missile_draw(task);
}

static void missile_launch(struct task *task)
{
    struct task_work *work = task->work;
    struct missile *missile = task->extra;

    missile_try_burst(task);
    missile->frame += 0.5F;
    if (missile->frame >= MOTION_END) {
        missile->frame = MOTION_END;
        task->step = missile_home;
    }
    VECTOR_SUM(&work->pos, &work->pos, &missile->speed);
    VECTOR_SCALE(&missile->speed, &missile->speed, MISSILE_KEEP);
    missile->spin += (MISSILE_SPIN - missile->spin) * 0.06F;
    work->ang.z += missile->spin;
    sound_play_at_for(SE_MISSILE_FLIGHT, (int)task, 1, 0, work->pos.x,
                      work->pos.y, work->pos.z, 4);
    collision_register(work);
    missile_draw(task);
}

static void missile_home(struct task *task)
{
    struct task_work *work = task->work;
    struct missile *missile = task->extra;

    missile->life--;
    missile_try_burst(task);
    missile_steer(task, &player_work[0]->pos, MISSILE_TURN, -MISSILE_TURN);
    missile_climb(task);
    missile_thrust(&missile->speed, &work->ang, 0.2F);
    missile_limit(&missile->speed, 2.0F);
    missile_pitch(&missile->speed, &work->ang);
    VECTOR_SUM(&work->pos, &work->pos, &missile->speed);
    missile->spin += (MISSILE_SPIN - missile->spin) * 0.06F;
    work->ang.z += missile->spin;
    if (missile->life < MISSILE_GLOW) {
        missile->flashing = TRUE;
    } else if (missile->life < MISSILE_BLINK) {
        missile->flashing = (missile->life >> 1) & 1;
    }
    missile_puff(task);
    sound_play_at_for(SE_MISSILE_FLIGHT, (int)task, 1, 0, work->pos.x,
                      work->pos.y, work->pos.z, 4);
    collision_register(work);
    missile_draw(task);
}

/* draw the missile's shadow each step, and the missile on a frame's first
 * step. it draws white and unlit while it flashes */
static void missile_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct missile *missile = task->extra;
    NJS_ARGB white;

    missile->floor_y = shadow_draw(work, 1.0F);
    njSetTexture(e101_r_texlist);
    if (frame_step == 0) {
        if (missile->flashing) {
            draw_control_save();
            draw_attr_save();
            njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR |
                        NJD_CONTROL_3D_CONSTANT_MATERIAL);
            white.a = 0.0F;
            white.r = white.g = white.b = 1.0F;
            njSetConstantMaterial(&white);
            draw_attr_clear(NJD_FLAG_USE_TEXTURE, 0);
            draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT);
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, work->ang.y + 0x4000);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateZ, work->ang.z);
            njDrawMotion((NJS_OBJECT *)(e101_r_models + MISSILE_OBJECT),
                         (NJS_MOTION *)(e101_r_models + MISSILE_MOTION),
                         missile->frame);
            njPopMatrix(1);
            draw_attr_restore();
            draw_control_restore();
        } else {
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, work->ang.y + 0x4000);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateZ, work->ang.z);
            draw_motion((NJS_OBJECT *)(e101_r_models + MISSILE_OBJECT),
                        (NJS_MOTION *)(e101_r_models + MISSILE_MOTION),
                        missile->frame);
            njPopMatrix(1);
        }
    }
}
