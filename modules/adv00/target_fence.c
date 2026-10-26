/* O M SAKU fence carrying three targets for E-102, which tips over once he
 * shoots them all */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/target_fence.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the fence's children mark the targets' positions */
extern NJS_OBJECT target_fence_object;
extern NJS_OBJECT fence_target_object;

#define FENCE_TARGETS 3

enum target_fence_mode {
    FENCE_STANDING = 0,
    FENCE_SHAKING = 3, /* after each target is shot */
    FENCE_TIPPING = 5,
    FENCE_FADING = 6,
    FENCE_GONE = 7
};
enum fence_target_mode { TARGET_WAITING, TARGET_SHOT };
enum fence_target_fly_mode { FLY_INIT = 0, FLY_RISING = 1, FLY_GONE = 7 };

#define SHAKE_STEPS 35
#define SHAKE_TURN 500 /* the rock each way about z */
/* the tilt the fence starts tipping from, and its tipping speed's gain at
 * each step */
#define TIP_START 35
#define TIP_GAIN 30
/* the tilt at which the tipping fence lies flat and starts to fade, a
 * quarter turn */
#define TIP_FLAT 0x4000

/* the alpha the fading fence and a fading shot target lose each step */
#define FADE_SPEED 0.05F

/* a shot target's first rise, and the rise it loses each step. it turns
 * FLY_SPIN_Y about y and FLY_SPIN_X about x each step. its angle z starts at
 * FLY_ANG_Z, but the draw never turns it about z. it throws up dust every
 * FLY_DUST_STEPS steps, and starts to fade after FLY_STEPS steps */
#define FLY_RISE 2.6F
#define FLY_FALL 0.1F
#define FLY_ANG_Z 2000
#define FLY_SPIN_Y 2000
#define FLY_SPIN_X 5000
#define FLY_DUST_STEPS 3
#define FLY_STEPS 13

/* a bit of the fence's work->tick, which a target sets when it is shot */
#define FENCE_HIT 0x1

/* the fence's vars. its tilt about z is in motion->aim.z, and its tipping
 * speed in motion->aim.y */
struct target_fence_vars {
    /* the constant material's alpha as the fence fades, from 0 down to -1 */
    Float fade;
};

struct fence_target_fly_vars {
    Float rise; /* the height it gains at each step */
    /* the constant material's alpha as the target fades, from 1 down to -1 */
    Float fade;
    Float unk_08;
};

#define VARS(work) ((struct target_fence_vars *)&(work)->vars)
#define FLY(work) ((struct fence_target_fly_vars *)&(work)->vars)
#define TARGETS_LEFT(work) ((work)->submode)

static void target_fence_solid_step(struct task *task);
static void target_fence_draw(struct task *task);
static void target_fence_step(struct task *task);
static void target_fence_tip_draw(struct task *task);
static void target_fence_destroy(struct task *task);
static void fence_target_init(struct task *task);
static void fence_target_step(struct task *task);
static void fence_target_draw(struct task *task);
static void fence_target_destroy(struct task *task);
static void fence_target_fly_init(struct task *task);
static void fence_target_fly_step(struct task *task);
static void fence_target_fly_draw(struct task *task);

/* the fence, and a post at each end. a fence with targets collides with
 * the first alone */
/* clang-format off */
static struct collision_shape target_fence_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 75.0F, 0.0F},
     2.0F, 180.0F, 40.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 75.0F, 38.0F},
     10.0F, 180.0F, 0.5F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 75.0F, -38.0F},
     10.0F, 180.0F, 0.5F, {0}, 0, NULL},
};
static struct collision_shape fence_target_shape[] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 0.0F, 0.0F}, 7.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* TRUE while the fence is gone for player 0's character: for Sonic and Big,
 * and for E-102 once FLAG_E102_SS_HOTEL_POOL is set. this file never sets
 * that flag */
static Bool target_fence_gone(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_E102:
        return event_flags[FLAG_E102_SS_HOTEL_POOL];
    case CHARACTER_SONIC:
    case CHARACTER_BIG:
        return TRUE;
    case CHARACTER_TAILS:
    case CHARACTER_KNUCKLES:
    case CHARACTER_AMY:
        break;
    }
    return FALSE;
}

void target_fence_plain_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    if (!placement_editing(task)) {
        collision_init(task, target_fence_shapes, COLLISION_OBJECT, 1, 3);
    }
    task->step = target_fence_solid_step;
    task->draw = target_fence_draw;
}

static void target_fence_solid_step(struct task *task)
{
    if (!placement_editing(task)) {
        collision_register(task->work);
    }
    target_fence_draw(task);
}

static void target_fence_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&target_fence_object);
    njPopMatrix(1);
}

void target_fence_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    if (target_fence_gone()) {
        if (!placement_editing(task)) {
            placement_kill(task);
            return;
        }
    }
    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = FENCE_STANDING;
    motion->aim.z = 0;
    VARS(work)->fade = 0.0F;
    if (player_get_character(0) != CHARACTER_E102) {
        if (!placement_editing(task)) {
            collision_init(task, target_fence_shapes, COLLISION_OBJECT, 1, 3);
        }
        task->step = target_fence_solid_step;
        task->draw = target_fence_draw;
    } else {
        if (!placement_editing(task)) {
            NJS_OBJECT *place = target_fence_object.child;
            /* @hack spelling: work->pos read through walk, which loads it with
             * FMOV.S @R4+ (0x0c9095ac to 0x0c9095b4). read by field, 91 of
             * 165 words differ */
            Float *walk = (Float *)&work->pos;
            Float x = *walk++;
            Float y = *walk++;
            Float z = *walk;
            Angle ang = work->ang.y;
            Float sin = njSin(ang);
            Float cos = njCos(ang);
            int i;

            for (i = 0; i < FENCE_TARGETS; i++) {
                struct task *child =
                    task_create_child(TASK_WORK, fence_target_init, task);

                if (child) {
                    struct task_work *child_work = child->work;
                    /* @hack spelling: the target's offset read through offset,
                     * which loads it with FMOV.S @R5+ (0x0c9095e0 to
                     * 0x0c9095ea). indexed, 76 of 165 words differ */
                    Float *offset = place->pos;
                    Float dx = *offset++;
                    Float dy = *offset++;
                    Float dz = *offset;

                    child_work->pos.x = x + ((cos * dx) + (sin * dz));
                    child_work->pos.y = y + dy;
                    child_work->pos.z = z + ((cos * dz) - (sin * dx));
                    place = place->sibling;
                }
            }
            collision_init(task, target_fence_shapes, COLLISION_OBJECT, 1, 1);
        }
        TARGETS_LEFT(work) = FENCE_TARGETS;
        work->tick = 0;
        work->timer = 0;
        work->mode = FENCE_STANDING;
        task->step = target_fence_step;
        task->draw = target_fence_tip_draw;
        task->destructor = target_fence_destroy;
    }
}

static void target_fence_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    if (placement_try_leave(task)) {
        return;
    }
    if (work->tick & FENCE_HIT) {
        work->mode = FENCE_SHAKING;
        work->tick = 0;
        work->timer = 0;
    }
    switch (work->mode) {
    case FENCE_STANDING:
        if (!placement_editing(task)) {
            collision_register(work);
        }
        break;
    case FENCE_SHAKING: {
        Angle rock;

        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > SHAKE_STEPS) {
            if (TARGETS_LEFT(work) == 0) {
                work->mode = FENCE_TIPPING;
                rock = TIP_START;
            } else {
                work->mode = FENCE_STANDING;
                rock = 0;
            }
            motion->aim.y = 0;
            work->timer = 0;
        } else {
            switch (TASK_TIMER(work) % 3) {
            case 0:
                rock = SHAKE_TURN;
                break;
            case 1:
                rock = 0;
                break;
            case 2:
                rock = -SHAKE_TURN;
                break;
            }
        }
        motion->aim.z = rock;
        if (!placement_editing(task)) {
            collision_register(work);
        }
        break;
    }
    case FENCE_TIPPING: {
        int speed = motion->aim.y + TIP_GAIN;
        Angle tip = motion->aim.z;

        tip += speed * njSin(tip);
        if (tip > TIP_FLAT) {
            work->mode = FENCE_FADING;
            VARS(work)->fade = 0.0F;
            tip = TIP_FLAT;
        }
        motion->aim.z = tip;
        motion->aim.y = speed;
        break;
    }
    case FENCE_FADING: {
        Float fade = VARS(work)->fade;

        fade -= FADE_SPEED;
        if (fade < -1.0F) {
            fade = -1.0F;
            work->mode = FENCE_GONE;
            task->draw = NULL;
        }
        VARS(work)->fade = fade;
        break;
    }
    /* no break ends FENCE_GONE's case, the last. with one,
     * task_step_children's address takes R2, not R3 (0x0c90981c), and 2 of
     * 179 words differ */
    case FENCE_GONE:;
    }
    if (!placement_editing(task)) {
        task_step_children(task);
    }
    if (work->mode != FENCE_GONE) {
        target_fence_tip_draw(task);
    }
}

static void target_fence_tip_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, motion->aim.z, ang);
    if (work->mode == FENCE_FADING) {
        draw_attr_save();
        draw_attr_set(0, NJD_FLAG_USE_ALPHA);
        draw_set_material(VARS(work)->fade, 0.0F, 0.0F, 0.0F);
        draw_tree(&target_fence_object);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
    } else {
        draw_tree(&target_fence_object);
    }
    njPopMatrix(1);
}

static void target_fence_destroy(struct task *task)
{
    if (!placement_editing(task)) {
        task_destroy_children(task);
    }
}

/* a target has an enemy record, so that E-102 can lock on to it */
static void fence_target_init(struct task *task)
{
    struct task_work *work = task->work;

    enemy_alloc_record(task, work);
    work->mode = TARGET_WAITING;
    collision_init(task, fence_target_shape, COLLISION_ITEM, 1, 1);
    task->step = fence_target_step;
    task->draw = fence_target_draw;
    task->destructor = fence_target_destroy;
}

static void fence_target_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case TARGET_WAITING:
        if (enemy_take_hit(work, LOCKON_TARGET(task))) {
            struct task *fly;

            work->flags &= ~COLLISION_DAMAGED;
            work->mode = TARGET_SHOT;
            bomb_create(&work->pos, 0.4F);
            fly = task_create(TASK_WORK, 3, fence_target_fly_init);
            if (fly) {
                struct task_work *fly_work = fly->work;

                fly_work->pos.x = work->pos.x;
                fly_work->pos.y = work->pos.y;
                fly_work->pos.z = work->pos.z;
                fly_work->ang.y = work->ang.y;
            }
            TARGETS_LEFT(task->parent->work)--;
            task->parent->work->tick |= FENCE_HIT;
        } else {
            collision_register(work);
        }
        break;
    case TARGET_SHOT:
        task_kill(task);
        break;
    }
    fence_target_draw(task);
}

static void fence_target_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&fence_target_object);
    njPopMatrix(1);
}

static void fence_target_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

static void fence_target_fly_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = FLY_INIT;
    FLY(work)->rise = 0.0F;
    FLY(work)->fade = 1.0F;
    FLY(work)->unk_08 = 1.0F;
    work->mode = FLY_RISING;
    work->timer = 0;
    FLY(work)->rise = FLY_RISE;
    work->ang.x = 0;
    work->ang.z = FLY_ANG_Z;
    task->step = fence_target_fly_step;
    task->draw = fence_target_fly_draw;
}

static void fence_target_fly_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case FLY_RISING: {
        Float rise = FLY(work)->rise;
        NJS_VECTOR speed = {0.0F, 0.0F, 0.0F};

        rise -= FLY_FALL;
        work->pos.y += rise;
        FLY(work)->rise = rise;
        work->ang.y += FLY_SPIN_Y;
        work->ang.x += FLY_SPIN_X;
        if (TASK_TIMER(work) % FLY_DUST_STEPS == 0) {
            dust_create(&work->pos, &speed, 1.0F);
        }
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > FLY_STEPS) {
            Float fade = FLY(work)->fade;

            fade -= FADE_SPEED;
            if (fade < -1.0F) {
                fade = -1.0F;
                work->mode = FLY_GONE;
                task->draw = NULL;
            }
            FLY(work)->fade = fade;
        }
        break;
    }
    case FLY_GONE:
        task_kill(task);
        break;
    }
    fence_target_fly_draw(task);
}

static void fence_target_fly_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (work->mode == FLY_RISING) {
        draw_attr_save();
        draw_attr_set(0, NJD_FLAG_USE_ALPHA);
        draw_set_material(FLY(work)->fade, 0.0F, 0.0F, 0.0F);
        draw_tree(&fence_target_object);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
    }
    njPopMatrix(1);
}
