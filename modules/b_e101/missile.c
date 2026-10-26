/* the homing missiles E-101 fires. they steer and speed up as E-103's
 * do in Windy Valley, and fly twice as long */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_e101/boss.h"
#include "b_e101/missile.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "enemy/enemy.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg05/walls.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT e101_missile_object;

/* a missile's record, in task->extra, which task_destroy frees to the heap */
struct missile {
    NJS_VECTOR speed;
    int life; /* the steps left before it bursts */
    Uint8 unk_10[4];
    Bool released; /* TRUE once the destructor has run */
    Bool flashing;
    Float floor_y; /* the floor's height under it, from its last shadow_draw */
    /* the launcher's collision record, which the missile flies through
     * without bursting */
    struct collision_work *source;
};

#define MISSILE(task) ((struct missile *)(task)->extra)

#define MISSILE_LIFE 420 /* the steps a missile flies before it bursts */
#define MISSILE_BLINK 24 /* it blinks with fewer steps left than this */
#define MISSILE_GLOW 5 /* it stays white with fewer steps left than this */

/* the most a missile turns toward player 0 each step */
#define MISSILE_TURN NJM_DEG_ANG(5)

/* a burst's puff, at half a bomb puff's size */
#define BURST_SCALE 0.5F

#define SE_MISSILE 154
#define SE_MISSILE_BURST 158

int e101_missile_count = 0;

static struct collision_shape missile_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE | LOCKON_NO_NUMBER,
    ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
        SHAPE_HURTS_OBJECTS,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    2.5F,
    4.0F,
    0.65F,
    {0, 0, 0},
    0,
    NULL};
/* the params of the exhaust's puffs, which every missile shares and every
 * puff keeps for its life. e101_missile_puff sets pos and speed before each
 * puff, which copies them */
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

static void e101_missile_step(struct task *task);
static void e101_missile_draw(struct task *task);
static void e101_missile_destroy(struct task *task);

void e101_missile_create(NJS_POINT3 *pos, NJS_VECTOR *dir,
                         struct collision_work *source)
{
    struct task *task;
    struct task_work *work;
    struct missile *missile;

    if ((task = task_create(TASK_WORK, 3, e101_missile_step)) != NULL) {
        work = task->work;
        collision_init(task, &missile_shape, COLLISION_ENEMY, 1, 1);
        enemy_alloc_record(task, work);
        missile = heap_alloc(sizeof(struct missile));
        task->extra = missile;
        work->pos = *pos;
        work->ang.y = -(Angle)(atan2f(dir->z, dir->x) * ANGLE_PER_RADIAN);
        missile->speed.x = missile->speed.y = missile->speed.z = 0.0F;
        missile->life = MISSILE_LIFE;
        missile->released = FALSE;
        missile->flashing = FALSE;
        missile->floor_y = -9999.0F;
        missile->source = source;
        e101_missile_count++;
        /* @bug memory: a missile whose lock-on record can't be allocated
         * flies on, and reads the record through null. enemy_alloc_record
         * kills the task when it can't allocate the record (JSR at
         * 0x0c902378), but this store sets the task's step again (MOV.L
         * R2,@(16,R13) at 0x0c9023de). the missile reads the record on a hit
         * that hurts it, and on a shot with a lock-on that touches it */
        task->step = e101_missile_step;
        task->draw = e101_missile_draw;
        task->destructor = e101_missile_destroy;
    }
}

static void e101_missile_burst(struct task *task, Float scale)
{
    struct task_work *work = task->work;

    bomb_create(&work->pos, scale);
    task_kill(task);
    sound_play_at(SE_MISSILE_BURST, 0, 0, 0, work->pos.x, work->pos.y,
                  work->pos.z);
}

/* burst the missile when the boss is down or its life runs out. a hit that
 * hurts it while E-102 is locked on to it bursts it too. so does a wall it
 * touches, and any record but the launcher's that touches it. of the
 * attacks, only E-102's shot at its lock-on bursts it */
static void e101_missile_try_burst(struct task *task)
{
    struct missile *missile;
    struct task_work *work;
    struct lockon_target *target;
    NJS_VECTOR push;
    struct task *lockon;
    struct task *shot;

    work = task->work;
    target = LOCKON_TARGET(task);
    missile = MISSILE(task);
    if (e101_missile_count < 0) {
        lockon_release(task);
        e101_missile_burst(task, BURST_SCALE);
        return;
    }
    if ((work->flags & COLLISION_DAMAGED) && target->lockon != NULL) {
        lockon_release(task);
        e101_missile_burst(task, BURST_SCALE);
        return;
    }
    /* @hack spelling: the wall test's result compared with TRUE, which SHC
     * tests with CMP/EQ #1 (0x0c9024ca). e100_push_from_walls returns only TRUE
     * or FALSE, so a plain test of it is the same; as one, 2 of 86 words
     * differ */
    if (e100_push_from_walls(&work->pos, 2.0F, NULL, &push) == TRUE) {
        e101_missile_burst(task, BURST_SCALE);
        return;
    }
    if (work->collision->flags & COLLISION_HIT) {
        if (work->collision->hit != missile->source) {
            if (work->collision->hit->kind == COLLISION_ATTACK) {
                shot = work->collision->hit->owner;
                if ((lockon = lockon_of_shot(shot)) != NULL) {
                    if (target->lockon == lockon) {
                        e101_missile_burst(task, BURST_SCALE);
                        return;
                    }
                }
            } else {
                e101_missile_burst(task, BURST_SCALE);
                return;
            }
        }
    }
    if (missile->life <= 0) {
        e101_missile_burst(task, BURST_SCALE);
        return;
    }
}

static void e101_missile_steer(struct task *task, NJS_POINT3 *target, int max,
                               int min)
{
    struct task_work *work = task->work;
    Float x;
    Float z;
    int turn;
    Angle heading;

    x = work->pos.x - target->x;
    z = work->pos.z - target->z;
    heading = -(Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
    turn = angle_difference(work->ang.y, heading);
    work->ang.y += CLAMP(turn, min, max);
}

/* climb or dive toward the height of player 0's center */
static void e101_missile_climb(struct task *task)
{
    struct task_work *work = task->work;
    struct missile *missile = MISSILE(task);
    Float height =
        player_work[0]->pos.y + player_get(0)->consts.center_y - work->pos.y;

    if (njAbs(height) < 0.1F) {
        missile->speed.y *= 0.7F;
    } else {
        missile->speed.y += height > 0.0F ? 0.03F : -0.02F;
    }
    missile->speed.y = CLAMP(missile->speed.y, -1.5F, 2.0F);
}

static void e101_missile_thrust(NJS_VECTOR *speed, struct angles *ang,
                                Float thrust)
{
    NJS_VECTOR push;
    NJS_VECTOR ahead;
    NJS_MATRIX matrix;

    push.x = -thrust;
    push.y = push.z = 0.0F;
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, ang->y);
    njCalcPoint(&matrix, &push, &ahead);
    speed->x += ahead.x;
    speed->z += ahead.z;
}

static void e101_missile_limit(NJS_VECTOR *speed, Float max)
{
    Float length = njSqrt(VECTOR_SQUARED(speed));
    Float scale;

    if (length > max) {
        scale = max / length;
        VECTOR_SCALE(speed, speed, scale);
    }
}

static void e101_missile_pitch(NJS_VECTOR *speed, Angle *ang_x)
{
    Float across = njSqrt(ACROSS_SQUARED(speed));

    *ang_x = (Angle)(atan2f(across, speed->y) * ANGLE_PER_RADIAN);
}

static void e101_missile_puff(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR back;
    Angle ang;

    njPushMatrix(&_unit_matrix_);
    back.y = back.z = 0.0F;
    back.x = 2.0F;
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njCalcPoint(NULL, &back, &missile_exhaust.speed);
    njPopMatrix(1);
    VECTOR_SUM(&missile_exhaust.pos, &work->pos, &missile_exhaust.speed);
    dust_create_glow(&missile_exhaust);
}

/* a missile that bursts still flies, sounds and draws this step. task_kill
 * destroys it on its next */
static void e101_missile_step(struct task *task)
{
    struct task_work *work = task->work;
    struct missile *missile = MISSILE(task);

    missile->life--;
    e101_missile_try_burst(task);
    e101_missile_steer(task, &player_work[0]->pos, MISSILE_TURN, -MISSILE_TURN);
    e101_missile_climb(task);
    e101_missile_thrust(&missile->speed, &work->ang, 0.4F);
    e101_missile_limit(&missile->speed, 1.0F);
    e101_missile_pitch(&missile->speed, &work->ang.x);
    VECTOR_SUM(&work->pos, &work->pos, &missile->speed);
    if (missile->life < MISSILE_GLOW) {
        missile->flashing = TRUE;
    } else if (missile->life < MISSILE_BLINK) {
        missile->flashing = (missile->life >> 1) & 1;
    }
    e101_missile_puff(task);
    collision_register(work);
    sound_play_at_for(SE_MISSILE, (int)task, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 4);
    e101_missile_draw(task);
}

/* the missile draws white and unlit while it flashes */
static void e101_missile_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct missile *missile = MISSILE(task);
    NJS_ARGB white;
    Angle ang;

    missile->floor_y = shadow_draw(work, 1.0F);
    njSetTexture(&e101_texlist);
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
        TURN_VIA(njRotateY, work->ang.y - 0x4000, ang);
        TURN_VIA(njRotateX, work->ang.x + 0x4000, ang);
        njDrawObject(&e101_missile_object);
        njPopMatrix(1);
        draw_attr_restore();
        draw_control_restore();
    } else {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y - 0x4000, ang);
        TURN_VIA(njRotateX, work->ang.x + 0x4000, ang);
        njEasyDrawObject(&e101_missile_object);
        njPopMatrix(1);
    }
}

/* take the missile off e101_missile_count, once. a count of 0 or less stays
 * as it is */
static void e101_missile_destroy(struct task *task)
{
    struct missile *missile = MISSILE(task);

    /* @hack redundant: an else after the return, which keeps TRUE in R2 and the
     * count in R3 (0x0c902ad8, 0x0c902ade); without it, the two trade
     * registers, and 4 of 15 words differ */
    if (missile->released) {
        return;
    } else {
        missile->released = TRUE;
        if (e101_missile_count > 0) {
            e101_missile_count--;
        }
    }
}
