/* PianTama: a Chao egg on the tables, whose Chao flies out once a touch
 * springs its top off. PianPach, PianWalk, PianFish and PianSlee: Chao on
 * the tables, each looping an action */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg09/casino.h"
#include "stg09/chao.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_ACTION casino_chao_egg_action;
extern NJS_MODEL casino_chao_shell_model;
extern NJS_MODEL casino_chao_shell_top_model;
extern NJS_ACTION casino_chao_fly_action;
extern NJS_ACTION casino_chao_clap_action;
extern NJS_ACTION casino_chao_walk_action;
extern NJS_ACTION casino_chao_fish_action;
extern NJS_ACTION casino_chao_sleep_action;

struct casino_chao_vars {
    NJS_ACTION *action;
    Uint8 unk_04[4];
    Float frame;
};

/* the egg's work->vars. work->scl is the top's offset from the egg as it
 * springs. it is the Chao's as it flies. TASK_TIMER(work) is the spring's
 * phase */
struct casino_chao_egg_vars {
    Float speed_x; /* with speed_z, the offset's drift each step */
    Float speed_z;
    Float frame; /* of the egg's or the flying Chao's action */
};

#define VARS(work) ((struct casino_chao_vars *)&(work)->vars)
#define EGG(work) ((struct casino_chao_egg_vars *)&(work)->vars)

/* egg modes, in work->mode */
#define EGG_ROCKING 0
#define EGG_SPRINGING 1
#define EGG_FLYING 2
#define EGG_EMPTY 3

/* the task destroys itself beyond 1000 of the camera and player 0.
 * placement_try_leave_by takes the range squared */
#define CASINO_CHAO_RANGE (1000.0F * 1000.0F)
#define CASINO_CHAO_FRAME 0.1F /* the action frames each step plays */
#define CASINO_CHAO_FRAMES 15.0F /* the frame each action starts over at */

/* the top drifts away from player 0 at TOP_SPEED a step. its height above
 * the egg is TOP_HEIGHT, plus TOP_BOB times the sine of the spring's phase.
 * the phase turns SPRING_STEP each step, until SPRING_END */
#define TOP_SPEED 0.2F
#define TOP_HEIGHT 8.0F
#define TOP_BOB 10.0F
#define SPRING_STEP 0x200
#define SPRING_END NJM_DEG_ANG(270)
/* the Chao starts CHAO_HEIGHT above the egg and rises CHAO_RISE a step. it
 * drifts up to CHAO_DRIFT a step on x and on z, at random. the Chao is gone
 * once its height above the egg passes the egg's own y plus CHAO_TOP. the
 * egg then stands empty */
#define CHAO_HEIGHT 4.0F
#define CHAO_RISE 0.4F
#define CHAO_DRIFT 0.5F
#define CHAO_TOP 200.0F

static struct collision_shape casino_chao_egg_shape = {
    SHAPE_SPHERE,
    ATTACK(3),
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    5.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void casino_chao_destroy(struct task *task)
{
}

static void casino_chao_egg_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    switch (work->mode) {
    case EGG_ROCKING:
        draw_action(&casino_chao_egg_action, EGG(work)->frame);
        break;
    case EGG_SPRINGING:
        draw_model(&casino_chao_shell_model);
        njTranslateV(NULL, &work->scl);
        draw_model(&casino_chao_shell_top_model);
        break;
    case EGG_FLYING:
        draw_model(&casino_chao_shell_model);
        njTranslateV(NULL, &work->scl);
        draw_action(&casino_chao_fly_action, EGG(work)->frame);
        break;
    case EGG_EMPTY:
        draw_model(&casino_chao_shell_model);
        break;
    }
    njPopMatrix(1);
}

static void casino_chao_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(VARS(work)->action, VARS(work)->frame);
    njPopMatrix(1);
}

/* a touch springs the egg's top off, away from player 0. once the spring's
 * phase reaches SPRING_END, the Chao flies up out of the egg. once it passes
 * the height CHAO_TOP sets, it is gone and the egg stands empty */
static void casino_chao_egg_run(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, CASINO_CHAO_RANGE)) {
        return;
    }
    switch (work->mode) {
    case EGG_ROCKING:
        if (work->collision->flags & COLLISION_HIT) {
            ang = ((Angle)(atan2f(work->pos.z - player_work[0]->pos.z,
                                  player_work[0]->pos.x - work->pos.x) *
                           ANGLE_PER_RADIAN) -
                   NJM_DEG_ANG(90)) &
                  0xffff;
            work->scl.z = 0.0F;
            work->scl.x = 0.0F;
            work->scl.y = TOP_HEIGHT;
            EGG(work)->speed_x = njSin(ang) * TOP_SPEED;
            EGG(work)->speed_z = njCos(ang) * TOP_SPEED;
            TASK_TIMER(work) = 0;
            work->mode++;
        } else {
            EGG(work)->frame += CASINO_CHAO_FRAME;
            if (EGG(work)->frame >= CASINO_CHAO_FRAMES) {
                EGG(work)->frame = 0.0F;
            }
        }
        break;
    case EGG_SPRINGING:
        if (TASK_TIMER(work) >= SPRING_END) {
            work->scl.z = 0.0F;
            work->scl.x = 0.0F;
            work->scl.y = CHAO_HEIGHT;
            EGG(work)->speed_x = rand() * (CHAO_DRIFT * RAND_SCALE);
            EGG(work)->speed_z = rand() * (CHAO_DRIFT * RAND_SCALE);
            work->mode++;
        } else {
            work->scl.x += EGG(work)->speed_x;
            work->scl.y = TOP_HEIGHT + (njSin(TASK_TIMER(work)) * TOP_BOB);
            work->scl.z += EGG(work)->speed_z;
            TASK_TIMER(work) += SPRING_STEP;
        }
        break;
    case EGG_FLYING:
        /* @bug logic: scl.y is the Chao's height above the egg, but the
         * test takes the egg's own y as its base (FCMP/GT at 0x0c90a26e). how
         * far the Chao rises then rests on where the egg stands */
        if (work->scl.y > work->pos.y + CHAO_TOP) {
            work->mode++;
        } else {
            work->scl.x += EGG(work)->speed_x;
            work->scl.y += CHAO_RISE;
            work->scl.z += EGG(work)->speed_z;
            EGG(work)->frame += CASINO_CHAO_FRAME;
            if (EGG(work)->frame >= CASINO_CHAO_FRAMES) {
                EGG(work)->frame = 0.0F;
            }
        }
        break;
    }
    casino_chao_egg_draw(task);
    collision_register(work);
    object_clear_touch(work, NULL);
}

static void casino_chao_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, CASINO_CHAO_RANGE)) {
        return;
    }
    VARS(work)->frame += CASINO_CHAO_FRAME;
    if (VARS(work)->frame >= CASINO_CHAO_FRAMES) {
        VARS(work)->frame = 0.0F;
    }
    casino_chao_draw(task);
}

static void casino_chao_start(struct task *task, NJS_ACTION *action)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, CASINO_CHAO_RANGE)) {
        return;
    }
    VARS(work)->action = action;
    VARS(work)->frame = 0.0F;
    task->step = casino_chao_run;
    task->draw = casino_chao_draw;
    task->destructor = casino_chao_destroy;
}

void casino_chao_egg_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, CASINO_CHAO_RANGE)) {
        return;
    }
    work->mode = EGG_ROCKING;
    EGG(work)->frame = 0.0F;
    collision_init(task, &casino_chao_egg_shape, COLLISION_OBJECT, 1, 1);
    task->step = casino_chao_egg_run;
    task->draw = casino_chao_egg_draw;
    task->destructor = casino_chao_destroy;
}

void casino_chao_clap_step(struct task *task)
{
    casino_chao_start(task, &casino_chao_clap_action);
}

void casino_chao_walk_step(struct task *task)
{
    casino_chao_start(task, &casino_chao_walk_action);
}

void casino_chao_fish_step(struct task *task)
{
    casino_chao_start(task, &casino_chao_fish_action);
}

void casino_chao_sleep_step(struct task *task)
{
    casino_chao_start(task, &casino_chao_sleep_action);
}
