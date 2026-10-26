/* StoperC, StoperL and StoperR stoppers on the pinball tables */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "draw/model.h"
#include "object/common.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/pinball.h"
#include "stg09/stopper.h"
#include "system/task.h"
#include "system/util.h"

/* StoperC's base and top, then the two drawn while CASINO_STOPPER_C_UP is
 * set */
extern NJS_MODEL stopper_c_base_model;
extern NJS_MODEL stopper_c_top_model;
extern NJS_MODEL stopper_c_base_up_model;
extern NJS_MODEL stopper_c_top_up_model;
/* StoperL's and StoperR's base and arm, then the two drawn while the
 * stopper's bit is set */
extern NJS_MODEL stopper_base_model;
extern NJS_MODEL stopper_arm_model;
extern NJS_MODEL stopper_base_up_model;
extern NJS_MODEL stopper_arm_up_model;

enum stopper_mode { STOPPER_C, STOPPER_L, STOPPER_R };

struct stopper_vars {
    /* the object kind's param, which the destructor passes to world_remove
     * and world_object_free. StoperC's param is null, and StoperC puts
     * nothing in the world, so both calls do nothing */
    NJS_OBJECT *object;
    Float height; /* the height StoperC's top is drawn at, C_DOWN up to 0 */
};

#define VARS(work) ((struct stopper_vars *)&(work)->vars)

/* set in a switch's work->flags while its collision record is hit, so that
 * one touch toggles CASINO_UNK_100 once */
#define SWITCH_TOUCHING 0x8000

/* each arm's switch sits this far down world z from the arm */
#define SWITCH_GAP 40.0F

/* the height of StoperC's top when down, and how far it rises or sinks a
 * step */
#define C_DOWN (-4.5F)
#define C_RISE 0.5F

/* an arm's x angle when down, and how far it swings a step */
#define ARM_DOWN NJM_DEG_ANG(-25.5)
#define ARM_SWING 0x100

#define ARM_RINGS 8 /* given for a touch of a raised arm */

/* an arm's box, its switch's box and StoperC's cylinder */
static struct collision_shape stopper_shapes[3] = {
    {
        SHAPE_RECTANGLE,
        ATTACK(3),
        PUSH_FIRM(PUSH_ALL),
        {0.0F, 2.5F, -10.0F},
        2.5F,
        2.5F,
        0.1F,
        {0, 0, 0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        ATTACK(3),
        PUSH_FIRM(PUSH_ALL),
        {0.0F, 2.5F, 0.0F},
        2.5F,
        2.5F,
        0.1F,
        {0, 0, 0},
        0,
        NULL,
    },
    {
        SHAPE_CYLINDER,
        ATTACK(3),
        PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
        {0.0F, 4.0F, 0.0F},
        12.0F,
        4.5F,
        0.0F,
        {0, 0, 0},
        0,
        NULL,
    },
};

static void stopper_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (work->mode == STOPPER_C) {
        NJS_OBJECT *object = VARS(work)->object;

        world_remove(task, object);
        world_object_free(object);
    } else {
        task_destroy_children(task);
    }
}

static void stopper_c_draw(struct task *task)
{
    struct task_work *work = task->work;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    if (!(casino_flags & CASINO_STOPPER_C_UP)) {
        draw_model(&stopper_c_base_model);
        njTranslate(NULL, 0.0F, VARS(work)->height, 0.0F);
        draw_model(&stopper_c_top_model);
    } else {
        draw_model(&stopper_c_base_up_model);
        njTranslate(NULL, 0.0F, VARS(work)->height, 0.0F);
        draw_model(&stopper_c_top_up_model);
    }
    njPopMatrix(1);
}

static void stopper_arm_draw(struct task *task)
{
    struct task_work *work = task->work;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if ((work->mode == STOPPER_L && !(casino_flags & CASINO_STOPPER_L_UP)) ||
        (work->mode == STOPPER_R && !(casino_flags & CASINO_STOPPER_R_UP))) {
        draw_model(&stopper_base_model);
        TURN(njRotateX, work->ang.x);
        draw_model(&stopper_arm_model);
    } else {
        draw_model(&stopper_base_up_model);
        TURN(njRotateX, work->ang.x);
        draw_model(&stopper_arm_up_model);
    }
    njPopMatrix(1);
}

static void stopper_switch_run(struct task *task)
{
    struct task_work *work = task->work;

    if (work->collision->flags & COLLISION_HIT) {
        if (!(work->flags & SWITCH_TOUCHING)) {
            if (casino_flags & CASINO_UNK_100) {
                pinball_view = PINBALL_VIEW_MOVING | PINBALL_VIEW_TABLE;
                casino_flags &= ~CASINO_UNK_100;
            } else {
                casino_flags |= CASINO_UNK_100;
            }
            work->flags |= SWITCH_TOUCHING;
        }
    } else {
        work->flags &= ~SWITCH_TOUCHING;
    }
    collision_register(work);
    object_clear_touch(work, NULL);
}

static void stopper_c_run(struct task *task)
{
    struct task_work *work = task->work;

    if (casino_flags & CASINO_STOPPER_C_UP) {
        if (VARS(work)->height != 0.0F) {
            VARS(work)->height += C_RISE;
            if (VARS(work)->height > 0.0F) {
                VARS(work)->height = 0.0F;
            }
        }
        work->collision->shapes->flags &= ~SHAPE_OFF;
    } else {
        if (VARS(work)->height != C_DOWN) {
            VARS(work)->height -= C_RISE;
            if (VARS(work)->height < C_DOWN) {
                VARS(work)->height = C_DOWN;
            }
        } else {
            work->collision->shapes->flags |= SHAPE_OFF;
        }
    }
    collision_register(work);
    object_clear_touch(work, NULL);
    stopper_c_draw(task);
    /* @hack redundant: the return, which makes SHC open the frame in the
     * prologue with ADD #-4,R15 and store task with MOV.L R4,@R15 (0x0c903804,
     * 0x0c90380c). without it, SHC pushes task with MOV.L R4,@-R15, the
     * function runs 2 bytes short and 64 of 64 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void stopper_l_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed;
    Angle ang[3];

    if (casino_flags & CASINO_STOPPER_L_UP) {
        if (work->collision->flags & COLLISION_HIT) {
            ang[0] = 0;
            ang[1] = NJM_DEG_ANG(180);
            ang[2] = 0;
            speed.x = 5.0F;
            speed.y = 0.0F;
            speed.z = 0.0F;
            player_send_flying(0, &speed, ang, 0);
            sound_play(SE_SPRING, 0, 0, 0);
            pinball_give_rings(ARM_RINGS);
            casino_flags &= ~CASINO_STOPPER_L_UP;
        }
        if (work->ang.x != 0) {
            work->ang.x += ARM_SWING;
            if (work->ang.x > 0) {
                work->ang.x = 0;
            }
        }
    } else {
        if (work->ang.x != ARM_DOWN) {
            work->ang.x -= ARM_SWING;
            if (work->ang.x < ARM_DOWN) {
                work->ang.x = ARM_DOWN;
            }
        }
    }
    collision_register(work);
    object_clear_touch(work, NULL);
    stopper_arm_draw(task);
    task_step_children(task);
}

static void stopper_r_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed;
    Angle ang[3];

    if (casino_flags & CASINO_STOPPER_R_UP) {
        if (work->collision->flags & COLLISION_HIT) {
            ang[0] = 0;
            ang[1] = NJM_DEG_ANG(180);
            ang[2] = 0;
            speed.x = 5.0F;
            speed.y = 0.0F;
            speed.z = 0.0F;
            player_send_flying(0, &speed, ang, 0);
            sound_play(SE_SPRING, 0, 0, 0);
            pinball_give_rings(ARM_RINGS);
            casino_flags &= ~CASINO_STOPPER_R_UP;
        }
        if (work->ang.x != 0) {
            work->ang.x += ARM_SWING;
            if (work->ang.x > 0) {
                work->ang.x = 0;
            }
        }
    } else {
        if (work->ang.x != ARM_DOWN) {
            work->ang.x -= ARM_SWING;
            if (work->ang.x < ARM_DOWN) {
                work->ang.x = ARM_DOWN;
            }
        }
    }
    collision_register(work);
    object_clear_touch(work, NULL);
    stopper_arm_draw(task);
    task_step_children(task);
}

void stopper_c_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &stopper_shapes[2], COLLISION_OBJECT, 1, 1);
    work->mode = STOPPER_C;
    VARS(work)->height = C_DOWN;
    work->collision->shapes->flags |= SHAPE_OFF;
    casino_flags &= ~CASINO_STOPPER_C_UP;
    task->step = stopper_c_run;
    task->draw = stopper_c_draw;
    task->destructor = stopper_destroy;
}

void stopper_l_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *switch_work;
    NJS_POINT3 *from;
    struct angles *from_ang;
    NJS_POINT3 *to;
    struct angles *to_ang;

    work->mode = STOPPER_L;
    collision_init(task, &stopper_shapes[0], COLLISION_OBJECT, 1, 1);
    child = task_create_child(TASK_WORK, stopper_switch_run, task);
    /* @hack spelling: the switch's place and angles copied through pointers,
     * which SHC forms with ADD #32 and ADD #20 (0x0c903b5e, 0x0c903b68).
     * through the members, the step runs 12 bytes short and 58 of 68 words
     * differ */
    /* @hack spelling: from and from_ang set before switch_work, so that SHC
     * loads child->work (0x0c903b58) ahead of MOV R13,R1. set after it, the
     * two trade places and 10 of 68 words differ */
    from = &work->pos;
    from_ang = &work->ang;
    switch_work = child->work;
    to = &switch_work->pos;
    to_ang = &switch_work->ang;
    to->x = from->x;
    to->y = from->y;
    to->z = from->z - SWITCH_GAP;
    to_ang->x = from_ang->x;
    to_ang->y = from_ang->y;
    to_ang->z = from_ang->z;
    collision_init(child, &stopper_shapes[1], COLLISION_OBJECT, 1, 1);
    casino_flags &= ~CASINO_STOPPER_L_UP;
    task->step = stopper_l_run;
    task->draw = stopper_arm_draw;
    task->destructor = stopper_destroy;
}

void stopper_r_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *switch_work;
    NJS_POINT3 *from;
    struct angles *from_ang;
    NJS_POINT3 *to;
    struct angles *to_ang;

    work->mode = STOPPER_R;
    collision_init(task, &stopper_shapes[0], COLLISION_OBJECT, 1, 1);
    child = task_create_child(TASK_WORK, stopper_switch_run, task);
    /* @hack spelling: the switch's place and angles copied through pointers, as
     * in stopper_l_step (0x0c903be8, 0x0c903bf2). through the members, the
     * step runs 12 bytes short and 58 of 69 words differ */
    /* @hack spelling: from and from_ang set before switch_work, as in
     * stopper_l_step (0x0c903be2). set after it, 10 of 69 words differ */
    from = &work->pos;
    from_ang = &work->ang;
    switch_work = child->work;
    to = &switch_work->pos;
    to_ang = &switch_work->ang;
    to->x = from->x;
    to->y = from->y;
    to->z = from->z - SWITCH_GAP;
    to_ang->x = from_ang->x;
    to_ang->y = from_ang->y;
    to_ang->z = from_ang->z;
    collision_init(child, &stopper_shapes[1], COLLISION_OBJECT, 1, 1);
    casino_flags &= ~CASINO_STOPPER_R_UP;
    task->step = stopper_r_run;
    task->draw = stopper_arm_draw;
    task->destructor = stopper_destroy;
}
