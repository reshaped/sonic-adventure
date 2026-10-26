/* O GLASS and O GLASS2 glass panes that break into thirteen shards */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg04/glass.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL glass_model;
extern NJS_OBJECT glass_shard_1_object;
extern NJS_OBJECT glass_shard_2_object;
extern NJS_OBJECT glass_shard_3_object;
extern NJS_OBJECT glass_shard_4_object;
extern NJS_OBJECT glass_shard_5_object;
extern NJS_OBJECT glass_shard_6_object;
extern NJS_OBJECT glass_shard_7_object;
extern NJS_OBJECT glass_shard_8_object;
extern NJS_OBJECT glass_shard_9_object;
extern NJS_OBJECT glass_shard_10_object;
extern NJS_OBJECT glass_shard_11_object;
extern NJS_OBJECT glass_shard_12_object;
extern NJS_OBJECT glass_shard_13_object;

#define GLASS_SHARDS 13

/* O GLASS2's range for placement_try_leave_by, squared, and how far its
 * collision record reaches from its position */
#define GLASS_2_RANGE (1500.0F * 1500.0F)
#define GLASS_2_REACH 200.0F

/* a pane cracks once a touch lasts more than CRACK_STEPS steps in a row. it
 * then stays solid for CRACK_HOLD steps before it falls. O GLASS2 breaks as a
 * cracked pane that holds BURST_HOLD steps */
#define CRACK_STEPS 5
#define CRACK_HOLD 60
#define BURST_HOLD 1

/* each shard flies out SHARD_SPREAD around from the one before it, and ends
 * after SHARD_STEPS steps of flight. its rise falls by SHARD_GRAVITY each
 * step. a cracked shard's rise starts at CRACKED_RISE times its spin, and it
 * moves out CRACKED_SPEED a step. a shattered shard's rise starts at
 * SHATTERED_RISE times its index plus 1. it moves out SHATTERED_SPEED a step,
 * and up SHATTERED_SPEED times its rise */
#define SHARD_SPREAD (25.0F * ANGLE_PER_DEGREE)
#define SHARD_STEPS 200
#define SHARD_GRAVITY 0.0625F
#define CRACKED_RISE 0.1F
#define CRACKED_SPEED 0.81F
#define SHATTERED_RISE 1.3F
#define SHATTERED_SPEED 0.5F
#define CRACKED_SPIN_YZ 2

#define SE_CRACK 94
#define SE_SHATTER 95

/* how a pane or shard broke, in work->tick: a cracked one falls after its hold,
 * and a shattered one at once */
enum glass_break { GLASS_CRACKED = 1, GLASS_SHATTERED = 2 };

#define BREAK(work) ((work)->tick)

enum glass_2_mode {
    GLASS_2_INIT,
    GLASS_2_WAIT,
    GLASS_2_BURST,
    GLASS_2_GONE /* this file never sets it */
};

struct shard_vars {
    Uint8 unk_00[4];
    NJS_OBJECT *object; /* the shard's node of the pane */
    Float index; /* its place among the pane's shards, 0 to 12 */
};

#define SHARD(work) ((struct shard_vars *)&(work)->vars)
/* the shard's spin, 1 or -1, and the steps a cracked shard waits before it
 * falls */
#define SHARD_SPIN(work) ((work)->scl.x)
#define SHARD_HOLD(work) ((work)->scl.z)

static void glass_stand(struct task *task);
static void glass_break(struct task *task);
static void glass_shatter(struct task *task, int hold);
static void glass_draw(struct task *task);
static void glass_destroy(struct task *task);
static void glass_shard_destroy(struct task *task);
static void glass_shard_wait(struct task *task);
static void glass_shard_draw(struct task *task);
static void glass_shard_fly(struct task *task);

static struct collision_shape glass_shape[1] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -3.0F, 0.0F},
     20.0F,
     6.0F,
     20.0F,
     {0},
     0,
     NULL},
};
static struct collision_shape glass_2_shape[1] = {
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     80.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

void glass_step(struct task *task)
{
    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            collision_init(task, glass_shape, COLLISION_OBJECT, 1, 1);
        }
        task->step = glass_stand;
        task->destructor = glass_destroy;
        task->draw = glass_draw;
        glass_draw(task);
    }
}

/* an unturned pane shatters at once when hit from below. a pane turned about z
 * shatters at once when hit from above. any other touch cracks the pane once it
 * lasts more than CRACK_STEPS steps in a row */
static void glass_stand(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (!edit_mode) {
            Sint16 flags = work->flags;
            Angle turned = work->ang.z;

            if (((flags & COLLISION_ON_TOP) && !turned) ||
                ((flags & COLLISION_UNDER) && turned)) {
                work->flags &= ~(COLLISION_UNDER | COLLISION_ON_TOP);
                task->step = glass_break;
                BREAK(work) = GLASS_SHATTERED;
                work->timer = 0;
                sound_play(SE_SHATTER, 0, 0, 0);
            } else if ((work->collision->flags & COLLISION_HIT) ||
                       ((flags & COLLISION_UNDER) && !turned) ||
                       /* @hack redundant: the test of a turned pane under a
                        * record, which the shatter above has already taken.
                        * the image tests it, TST R4,R4 at 0x0c90655e; without
                        * the test, 54 of 109 words differ */
                       ((flags & COLLISION_UNDER) && turned)) {
                work->collision->flags &= ~COLLISION_HIT;
                work->flags &= ~(COLLISION_UNDER | COLLISION_ON_TOP);
                if (++TASK_TIMER(work) > CRACK_STEPS) {
                    task->step = glass_break;
                    BREAK(work) = GLASS_CRACKED;
                    work->timer = 0;
                    sound_play(SE_CRACK, 0, 0, 0);
                }
            } else {
                work->timer = 0;
            }
            collision_register(work);
        }
        glass_draw(task);
    }
}

static void glass_break(struct task *task)
{
    glass_draw(task);
    glass_shatter(task, CRACK_HOLD);
}

/* create the shards on the first call. destroy a shattered pane at once, or a
 * cracked one once it has stayed solid for hold steps. each shard's random
 * spin here is replaced as the shard starts */
static void glass_shatter(struct task *task, int hold)
{
    struct task_work *work = task->work;
    struct task *shard;
    struct task_work *shard_work;
    int i;
    NJS_OBJECT *shards[GLASS_SHARDS] = {
        &glass_shard_1_object,  &glass_shard_2_object,  &glass_shard_3_object,
        &glass_shard_4_object,  &glass_shard_5_object,  &glass_shard_6_object,
        &glass_shard_7_object,  &glass_shard_8_object,  &glass_shard_9_object,
        &glass_shard_10_object, &glass_shard_11_object, &glass_shard_12_object,
        &glass_shard_13_object,
    };

    if (!work->timer) {
        for (i = 0; i < GLASS_SHARDS; i++) {
            if ((shard = task_create(TASK_WORK, 3, glass_shard_wait)) != NULL) {
                shard_work = shard->work;
                shard_work->pos.x = work->pos.x;
                shard_work->pos.y = work->pos.y;
                shard_work->pos.z = work->pos.z;
                SHARD(shard_work)->object = shards[i];
                SHARD(shard_work)->index = (Float)i;
                BREAK(shard_work) = GLASS_SHATTERED;
                SHARD_SPIN(shard_work) = rand() * (1.0F / 16384.0F);
                if (BREAK(work) == GLASS_CRACKED) {
                    BREAK(shard_work) = GLASS_CRACKED;
                    SHARD_HOLD(shard_work) = hold;
                }
            }
        }
    }
    work->timer++;
    if (BREAK(work) == GLASS_SHATTERED) {
        sound_play(SE_SHATTER, 0, 0, 0);
        placement_kill(task);
        return;
    }
    collision_register(work);
    if (TASK_TIMER(work) > hold) {
        sound_play(SE_SHATTER, 0, 0, 0);
        placement_kill(task);
        /* @hack redundant: a return where control returns anyway, which lets
         * the kill above branch to this one's call, BRA at 0x0c90686e; without
         * it, each kill makes its own call, 5 of the pool's words differ too,
         * and 2 of 335 words differ */
        return;
    }
}

static void glass_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    draw_model(&glass_model);
    njPopMatrix(1);
}

static void glass_destroy(struct task *task)
{
}

static void glass_shard_destroy(struct task *task)
{
}

/* start a shard at its node's place in the pane, and wait out the pane's hold.
 * its spin is 1 for an even index and -1 for an odd one */
static void glass_shard_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (work->timer == 0) {
        NJS_OBJECT *object = SHARD(work)->object;

        work->pos.x += object->pos[0];
        work->pos.y += object->pos[1];
        work->pos.z += object->pos[2];
        SHARD_SPIN(work) = powf(-1.0F, (int)SHARD(work)->index);
        task->destructor = glass_shard_destroy;
        task->draw = glass_shard_draw;
    }
    if (BREAK(work) == GLASS_SHATTERED) {
        task->step = glass_shard_fly;
        work->timer = 0;
    } else if (TASK_TIMER(work)++ > SHARD_HOLD(work)) {
        task->step = glass_shard_fly;
        work->timer = 0;
    }
    glass_shard_draw(task);
}

static void glass_shard_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = SHARD(work)->object;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    draw_model(object->model);
    njPopMatrix(1);
}

/* fly a shard out along its heading, its index times SHARD_SPREAD. a shattered
 * shard turns SHARD_SPIN degrees a step about each axis. a cracked shard turns
 * SHARD_SPIN degrees a step about x, and CRACKED_SPIN_YZ times that about y
 * and z */
static void glass_shard_fly(struct task *task)
{
    struct task_work *work = task->work;
    Angle heading;
    Angle turn_x;
    Angle turn_y;
    Angle turn_z;
    Float rise;

    if (TASK_TIMER(work) > SHARD_STEPS) {
        task_kill(task);
        return;
    }
    heading = (Angle)(SHARD(work)->index * SHARD_SPREAD);
    if (BREAK(work) == GLASS_SHATTERED) {
        work->timer++;
        work->pos.x += SHATTERED_SPEED * njCos(heading);
        work->pos.z += SHATTERED_SPEED * njSin(heading);
        rise = ((SHARD(work)->index + 1.0F) * SHATTERED_RISE) -
               (TASK_TIMER(work) * SHARD_GRAVITY);
        work->pos.y += SHATTERED_SPEED * rise;
        turn_x = turn_y = turn_z = (Angle)(SHARD_SPIN(work) * ANGLE_PER_DEGREE);
    } else {
        work->timer++;
        work->pos.y += (SHARD_SPIN(work) * CRACKED_RISE) -
                       (TASK_TIMER(work) * SHARD_GRAVITY);
        work->pos.x += CRACKED_SPEED * njCos(heading);
        work->pos.z += CRACKED_SPEED * njSin(heading);
        turn_x = (Angle)(SHARD_SPIN(work) * ANGLE_PER_DEGREE);
        turn_y = turn_z = turn_x * CRACKED_SPIN_YZ;
    }
    work->ang.x += turn_x;
    work->ang.y += turn_y;
    work->ang.z += turn_z;
    glass_shard_draw(task);
}

void glass_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, GLASS_2_RANGE)) {
        switch (work->mode) {
        case GLASS_2_INIT:
            task->destructor = glass_destroy;
            task->draw = glass_draw;
            collision_init(task, glass_2_shape, COLLISION_OBJECT, 1, 1);
            work->collision->radius = GLASS_2_REACH;
            work->mode = GLASS_2_WAIT;
            break;
        case GLASS_2_WAIT:
            if (!placement_editing(task)) {
                if (work->collision->flags & COLLISION_HIT) {
                    work->collision->flags &= ~COLLISION_HIT;
                    work->mode = GLASS_2_BURST;
                    BREAK(work) = GLASS_CRACKED;
                    break;
                }
                collision_register(work);
            }
            glass_draw(task);
            break;
        case GLASS_2_BURST:
            glass_shatter(task, BURST_HOLD);
            break;
        case GLASS_2_GONE:
            placement_kill(task);
            /* falls through. the mode's reset changes nothing, since the task
             * destroys itself on its next step */
        default:
            work->mode = GLASS_2_INIT;
            break;
        }
    }
}
