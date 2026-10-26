/* homing missiles that E-105 fires */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/draw.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "enemy/enemy.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg12/e105.h"
#include "stg12/missile.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT e105_missile_object;
extern NJS_MOTION e105_missile_motion;

struct e105_missile {
    NJS_POINT3 *anchor;
    NJS_VECTOR *dir;
    NJS_VECTOR speed;
    Angle spin;
    int life;
    Uint8 unk_1c[4];
    Bool uncounted; /* TRUE once taken off e105_missile_count */
    Bool flashing; /* TRUE while it draws all white */
    /* the floor's height under it, as shadow_draw gives it, or -9999 before
     * its first draw. nothing reads it */
    Float floor;
    Float frame;
};

#define MISSILE(task) ((struct e105_missile *)(task)->extra)

/* the steps a missile homes before it bursts. once fewer than MISSILE_BLINK
 * are left, it blinks white, two steps on and two off. once fewer than
 * MISSILE_GLOW are left, it stays white */
#define MISSILE_LIFE 480
#define MISSILE_BLINK 24
#define MISSILE_GLOW 5

/* the most a homing missile turns toward player 0 each step, about 1.2
 * degrees */
#define MISSILE_TURN 218
/* the roll speed a missile's spin eases toward, about 8 degrees a step */
#define MISSILE_SPIN 0x5b0

/* the last frame of e105_missile_motion. the launch plays the motion to it,
 * half a frame a step, and the missile then homes */
#define MOTION_END 9.0F

#define SE_MISSILE_GO 153
#define SE_MISSILE 154
#define SE_MISSILE_BURST 158

static struct collision_shape e105_missile_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE | LOCKON_NO_NUMBER,
    ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
        SHAPE_HURTS_OBJECTS,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    5.0F,
    0.0F,
    0.65F,
    {0, 0, 0},
    0,
    NULL,
};

int e105_missile_count = 0;

/* the params of the exhaust's puffs. each puff keeps a pointer to them, and
 * e105_missile_puff writes their pos and speed before each one */
static struct dust_params e105_missile_exhaust = {
    0.7F,
    0.03F,
    0.19F,
    0.05F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.56F, 0.56F, 0.7F},
};

static void e105_missile_destroy(struct task *task);
static void e105_missile_thrust(NJS_VECTOR *speed, struct angles *ang,
                                Float thrust);
static void e105_missile_hold(struct task *task);
static void e105_missile_launch(struct task *task);
static void e105_missile_home(struct task *task);
static void e105_missile_draw(struct task *task);

struct task *e105_missile_create(NJS_POINT3 *anchor, NJS_VECTOR *dir)
{
    struct task *task;
    struct task_work *work;
    struct e105_missile *missile;

    if ((task = task_create(TASK_WORK, 3, e105_missile_hold)) != NULL) {
        work = task->work;
        collision_init(task, &e105_missile_shape, COLLISION_ENEMY, 1, 1);
        enemy_alloc_record(task, work);
        missile = heap_alloc(sizeof(struct e105_missile));
        task->extra = missile;
        work->pos.x = anchor->x;
        work->pos.y = anchor->y;
        work->pos.z = anchor->z;
        work->ang.y = -(Angle)(atan2f(dir->z, dir->x) * ANGLE_PER_RADIAN);
        work->ang.x = 0;
        missile->anchor = anchor;
        missile->dir = dir;
        missile->speed.x = missile->speed.y = missile->speed.z = 0.0F;
        missile->spin = 0;
        missile->life = MISSILE_LIFE;
        missile->uncounted = FALSE;
        missile->flashing = FALSE;
        missile->floor = -9999.0F;
        missile->frame = 0.0F;
        e105_missile_count++;
        task->draw = e105_missile_draw;
        task->destructor = e105_missile_destroy;
    }
    return task;
}

void e105_missile_release(struct task *task)
{
    struct task_work *work = task->work;
    struct e105_missile *missile = MISSILE(task);

    e105_missile_thrust(&missile->speed, &work->ang, 2.5F);
    task->step = e105_missile_launch;
    sound_play_at(SE_MISSILE_GO, 0, 0, 0, work->pos.x, work->pos.y,
                  work->pos.z);
}

static void e105_missile_destroy(struct task *task)
{
    struct e105_missile *missile = MISSILE(task);

    /* @hack redundant: an else after the return, which puts uncounted's TRUE in
     * R2 and the count in R3 (0x0c916a28, 0x0c916a2e); without it, the two
     * swap, and 4 of 15 words differ */
    if (missile->uncounted) {
        return;
    } else {
        missile->uncounted = TRUE;
        if (e105_missile_count > 0) {
            e105_missile_count--;
        }
    }
}

static void e105_missile_burst(struct task *task, Float scale)
{
    struct task_work *work = task->work;

    bomb_create(&work->pos, scale);
    task_kill(task);
    sound_play(SE_MISSILE_BURST, 0, 0, 0);
}

static void e105_missile_try_burst(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *target = LOCKON_TARGET(task);
    struct e105_missile *missile = MISSILE(task);
    struct collision_work *hit;

    /* @hack spelling: the goto to the last burst, which loads the target's
     * lockon into R2 (MOV.L @(R0,R6),R2 at 0x0c916a8a); with the other tests
     * inside an if, the lockon takes R1, and 2 of 56 words differ */
    if (e105_missile_count < 0) {
        goto burst;
    }
    if ((work->flags & COLLISION_DAMAGED) && target->lockon != NULL) {
        lockon_release(task);
        e105_missile_burst(task, 0.5F);
        return;
    }
    if (work->collision->flags & COLLISION_HIT) {
        hit = work->collision->hit;
        if (hit != NULL && hit->kind == COLLISION_PLAYER) {
            e105_missile_burst(task, 0.5F);
            return;
        }
    }
    if (missile->life > 0) {
        return;
    }
burst:
    e105_missile_burst(task, 0.5F);
}

static void e105_missile_steer(struct task *task, NJS_POINT3 *target, int max,
                               int min)
{
    struct task_work *work = task->work;
    Float x;
    Float z;
    int turn;
    Angle heading;

    x = target->x - work->pos.x;
    z = target->z - work->pos.z;
    heading = -(Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
    turn = angle_difference(work->ang.y, heading);
    work->ang.y += CLAMP(turn, min, max);
}

static void e105_missile_climb(struct task *task)
{
    struct task_work *work = task->work;
    struct e105_missile *missile = MISSILE(task);
    Float height =
        player_work[0]->pos.y + player_get(0)->consts.center_y - work->pos.y;

    if (njAbs(height) < 0.1F) {
        missile->speed.y *= 0.7F;
    } else {
        missile->speed.y += (height - missile->speed.y) * 0.001F;
    }
    missile->speed.y = CLAMP(missile->speed.y, -0.7F, 0.7F);
}

static void e105_missile_thrust(NJS_VECTOR *speed, struct angles *ang,
                                Float thrust)
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

static void e105_missile_limit(NJS_VECTOR *speed, Float max)
{
    Float length = njSqrt(VECTOR_SQUARED(speed));
    Float scale;

    if (length > max) {
        scale = max / length;
        VECTOR_SCALE(speed, speed, scale);
    }
}

static void e105_missile_pitch(NJS_VECTOR *speed, Angle *ang_x)
{
    Float across = njSqrt(ACROSS_SQUARED(speed));

    *ang_x = -(Angle)(atan2f(speed->y, across) * ANGLE_PER_RADIAN);
}

static void e105_missile_puff(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR back;
    Angle ang;

    njPushMatrix(&_unit_matrix_);
    back.y = back.x = 0.0F;
    back.z = -2.0F;
    TURN_VIA(njRotateY, work->ang.y + 0x4000, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njCalcPoint(NULL, &back, &e105_missile_exhaust.speed);
    njPopMatrix(1);
    VECTOR_SUM(&e105_missile_exhaust.pos, &work->pos,
               &e105_missile_exhaust.speed);
    dust_create_glow(&e105_missile_exhaust);
}

static void e105_missile_hold(struct task *task)
{
    struct task_work *work = task->work;
    struct e105_missile *missile = MISSILE(task);

    work->pos.x = missile->anchor->x;
    work->pos.y = missile->anchor->y;
    work->pos.z = missile->anchor->z;
    work->ang.y =
        -(Angle)(atan2f(missile->dir->z, missile->dir->x) * ANGLE_PER_RADIAN);
    e105_missile_draw(task);
}

static void e105_missile_launch(struct task *task)
{
    struct task_work *work = task->work;
    struct e105_missile *missile = MISSILE(task);

    missile->frame += 0.5F;
    if (missile->frame >= MOTION_END) {
        missile->frame = MOTION_END;
        task->step = e105_missile_home;
    }
    VECTOR_SUM(&work->pos, &work->pos, &missile->speed);
    /* @hack spelling: the x product written out, which keeps the z product
     * in FR3 (0x0c916f20 to 0x0c916f24); as *=, the z product takes FR2, and
     * 3 of 92 words differ */
    missile->speed.x = missile->speed.x * 0.93F;
    missile->speed.z *= 0.93F;
    missile->spin += (MISSILE_SPIN - missile->spin) * 0.06F;
    work->ang.z += missile->spin;
    sound_play_at_for(SE_MISSILE, (int)task, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 4);
    collision_register(work);
    e105_missile_draw(task);
}

static void e105_missile_home(struct task *task)
{
    struct task_work *work = task->work;
    struct e105_missile *missile = MISSILE(task);

    missile->life--;
    e105_missile_try_burst(task);
    e105_missile_steer(task, &player_work[0]->pos, MISSILE_TURN, -MISSILE_TURN);
    e105_missile_climb(task);
    e105_missile_thrust(&missile->speed, &work->ang, 0.1F);
    e105_missile_limit(&missile->speed, 1.2F);
    e105_missile_pitch(&missile->speed, &work->ang.x);
    VECTOR_SUM(&work->pos, &work->pos, &missile->speed);
    missile->spin += (MISSILE_SPIN - missile->spin) * 0.06F;
    work->ang.z += missile->spin;
    sound_play_at_for(SE_MISSILE, (int)task, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 4);
    if (missile->life < MISSILE_GLOW) {
        missile->flashing = TRUE;
    } else if (missile->life < MISSILE_BLINK) {
        missile->flashing = (missile->life >> 1) & 1;
    }
    e105_missile_puff(task);
    collision_register(work);
    e105_missile_draw(task);
}

static void e105_missile_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct e105_missile *missile = MISSILE(task);
    Angle ang;
    NJS_ARGB white;

    missile->floor = shadow_draw(work, 1.0F);
    if (frame_step == 0) {
        njSetTexture(&e105_texlist);
        if (!missile->flashing) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            TURN_VIA(njRotateY, work->ang.y + 0x4000, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            draw_motion(&e105_missile_object, &e105_missile_motion,
                        missile->frame);
            njPopMatrix(1);
        } else {
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
            TURN_VIA(njRotateY, work->ang.y + 0x4000, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            njDrawMotion(&e105_missile_object, &e105_missile_motion,
                         missile->frame);
            njPopMatrix(1);
            draw_attr_restore();
            draw_control_restore();
        }
    }
}
