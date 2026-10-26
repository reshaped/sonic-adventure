/* SlingL and SlingR, the slingshots on the pinball tables */
#include <ninja.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/pinball.h"
#include "stg09/sling.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_MODEL sling_left_model;
extern NJS_MODEL sling_left_lit_model;
extern NJS_MODEL sling_right_model;
extern NJS_MODEL sling_right_lit_model;
extern NJS_OBJECT sling_left_object;
extern NJS_OBJECT sling_right_object;

struct sling_vars {
    NJS_OBJECT *object; /* the sling's copy in the collision world */
};

#define VARS(work) ((struct sling_vars *)&(work)->vars)

/* in work->flags: the sling is SlingR, and a touch is under way. a touch
 * launches only on its first step */
#define SLING_RIGHT 0x200
#define SLING_TOUCHING 0x8000

/* the draws after a touch that show the lit model, counted down in
 * work->submode */
#define SLING_LIT_DRAWS 2
/* the touches to a bonus, counted in work->tick. work->timer counts the
 * bonuses, and the nth gives 10n + 1 rings */
#define SLING_TOUCHES 20

#define SLING_WORLD (WORLD_MEDIUM | WORLD_SOLID)

/* the launch's turn from SlingL's y angle. SlingR's turns the other way */
#define SLING_LAUNCH_TURN 0x5a7d

/* SlingL's shape, then SlingR's, its mirror image */
static struct collision_shape sling_shapes[2] = {
    {
        SHAPE_RECTANGLE,
        ATTACK(3),
        PUSH_FIRM(PUSH_ALL),
        {6.5F, 10.0F, -7.0F},
        24.0F,
        10.0F,
        0.5F,
        {0, -NJM_DEG_ANG(71), 0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        ATTACK(3),
        PUSH_FIRM(PUSH_ALL),
        {-6.5F, 10.0F, -7.0F},
        24.0F,
        10.0F,
        0.5F,
        {0, NJM_DEG_ANG(71), 0},
        0,
        NULL,
    },
};

static void sling_destroy(struct task *task)
{
    NJS_OBJECT *object = VARS(task->work)->object;

    world_remove(task, object);
    world_object_free(object);
}

static void sling_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    if (!(work->flags & SLING_RIGHT)) {
        if (work->submode) {
            draw_model(&sling_left_lit_model);
            work->submode--;
        } else {
            draw_model(&sling_left_model);
        }
    } else {
        if (work->submode) {
            draw_model(&sling_right_lit_model);
            work->submode--;
        } else {
            draw_model(&sling_right_model);
        }
    }
    njPopMatrix(1);
}

static void sling_left_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed;
    Angle ang[3];
    NJS_OBJECT *object = VARS(work)->object;

    if (player_in_range(&work->pos, object->model->r + 20.0F)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (work->collision->flags & COLLISION_HIT) {
        if (!(work->flags & SLING_TOUCHING)) {
            ang[0] = 0;
            ang[1] = work->ang.y + SLING_LAUNCH_TURN;
            ang[2] = 0;
            speed.x = pinball_cap_speed(work->scl.z + pinball_player_speed() +
                                        (rand() * work->scl.y * RAND_SCALE));
            speed.y = 0.0F;
            speed.z = 0.0F;
            player_send_flying(0, &speed, ang, 0);
            work->submode = SLING_LIT_DRAWS;
            sound_play(SE_SPRING, 0, 0, 0);
            vibration_start(0, 4, 15, 0);
            if (++work->tick == SLING_TOUCHES) {
                work->tick = 0;
                pinball_give_rings((++work->timer * 10) + 1);
            } else {
                pinball_give_rings(1);
            }
            work->flags |= SLING_TOUCHING;
        }
    } else {
        work->flags &= ~SLING_TOUCHING;
    }
    if (placement_editing(task)) {
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
    }
    collision_register(work);
    object_clear_touch(work, NULL);
    sling_draw(task);
}

static void sling_right_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed;
    Angle ang[3];
    NJS_OBJECT *object = VARS(work)->object;

    if (player_in_range(&work->pos, object->model->r + 20.0F)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (work->collision->flags & COLLISION_HIT) {
        if (!(work->flags & SLING_TOUCHING)) {
            ang[0] = 0;
            ang[1] = work->ang.y - SLING_LAUNCH_TURN;
            ang[2] = 0;
            speed.x = pinball_cap_speed(work->scl.z + pinball_player_speed() +
                                        (rand() * work->scl.y * RAND_SCALE));
            speed.y = 0.0F;
            speed.z = 0.0F;
            player_send_flying(0, &speed, ang, 0);
            work->submode = SLING_LIT_DRAWS;
            sound_play(SE_SPRING, 0, 0, 0);
            vibration_start(0, 4, 15, 0);
            if (++work->tick == SLING_TOUCHES) {
                work->tick = 0;
                pinball_give_rings((++work->timer * 10) + 1);
            } else {
                pinball_give_rings(1);
            }
            work->flags |= SLING_TOUCHING;
        }
    } else {
        work->flags &= ~SLING_TOUCHING;
    }
    if (placement_editing(task)) {
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
    }
    collision_register(work);
    object_clear_touch(work, NULL);
    sling_draw(task);
}

void sling_left_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    object = VARS(work)->object =
        object_add_world(&sling_left_object, task, SLING_WORLD);
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    collision_init(task, &sling_shapes[0], COLLISION_OBJECT, 1, 1);
    work->timer = 0;
    work->tick = 0;
    work->submode = 0;
    /* @hack spelling: the clear written out, which keeps task in R13 and
     * work in R14 (MOV R4,R13 at 0x0c9034aa). as &=, the two trade, and 12 of
     * 46 words differ */
    work->flags = work->flags & ~SLING_RIGHT;
    task->step = sling_left_run;
    task->draw = sling_draw;
    task->destructor = sling_destroy;
}

void sling_right_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    object = VARS(work)->object =
        object_add_world(&sling_right_object, task, SLING_WORLD);
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    collision_init(task, &sling_shapes[1], COLLISION_OBJECT, 1, 1);
    work->timer = 0;
    work->tick = 0;
    work->submode = 0;
    /* @hack spelling: the set written out, which keeps task in R13 and work
     * in R14 (MOV R4,R13 at 0x0c903546). as |=, the two trade, and 12 of 46
     * words differ */
    work->flags = work->flags | SLING_RIGHT;
    task->step = sling_right_run;
    task->draw = sling_draw;
    task->destructor = sling_destroy;
}
