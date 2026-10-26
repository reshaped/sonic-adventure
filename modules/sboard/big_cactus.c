/* BIG CACTUS A, B and C, solid cacti that sway when touched. they sway the
 * more the faster player 0 goes */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/placement.h"
#include "object/simple.h"
#include "player/player.h"
#include "sboard/big_cactus.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/util.h"

/* a kind of big cactus, which init takes as arg */
struct big_cactus_kind {
    NJS_OBJECT *object;
    struct collision_list list;
};

/* @hack spelling: arg read through KIND at each use, which keeps arg on the
 * stack (MOV.L R5,@R15 at 0x0c9032ee); through a local, SHC keeps it in
 * R13, big_cactus_init runs 4 bytes short and 35 of its 36 words differ */
#define KIND(arg) ((struct big_cactus_kind *)(arg))

/* a big cactus's record, in task->motion */
struct big_cactus_record {
    Sint8 mode;
    NJS_OBJECT *object;
    struct collision_list *list;
    Angle rest; /* ang.x at rest */
    Angle phase; /* the sway's phase */
    Angle swing; /* the most the sway turns it from rest */
};

enum big_cactus_mode { BIG_CACTUS_STAND };

/* the sway's phase turns SWAY_SPEED a step, and its swing shrinks SWAY_DECAY
 * a step */
#define SWAY_SPEED NJM_DEG_ANG(5)
#define SWAY_DECAY NJM_DEG_ANG(0.1)

/* a touch at player 0's speed of SWAY_SPEED_FULL or more swings a cactus
 * SWAY_MOST. a slower one swings it less, in proportion */
#define SWAY_SPEED_FULL (4.6F * 0.4F)
#define SWAY_MOST NJM_DEG_ANG(30)

extern NJS_OBJECT big_cactus_a_object;
extern NJS_OBJECT big_cactus_b_object;
extern NJS_OBJECT big_cactus_c_object;

static void big_cactus_init(struct task *task, void *arg);
static void big_cactus_step(struct task *task);
static void big_cactus_draw(struct task *task);
static Angle big_cactus_push(Angle swing);

static const struct collision_kind big_cactus_collision = {COLLISION_OBJECT, 1};
static const struct task_desc big_cactus_desc = {
    sizeof(struct big_cactus_record),
    TASK_WORK,
    big_cactus_init,
    big_cactus_step,
    big_cactus_draw,
    NULL};

/* each kind's shape: a cylinder 142 high from the cactus's foot, which turns
 * as the cactus sways. it pushes every group, and none can push it */
static struct collision_shape big_cactus_a_shapes[1] = {
    {SHAPE_CYLINDER2,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 71.0F, 0.0F},
     8.0F,
     71.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct big_cactus_kind big_cactus_a = {&big_cactus_a_object,
                                              {big_cactus_a_shapes, 1, 160.0F}};
static struct collision_shape big_cactus_b_shapes[1] = {
    {SHAPE_CYLINDER2,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 71.0F, 0.0F},
     8.0F,
     71.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct big_cactus_kind big_cactus_b = {&big_cactus_b_object,
                                              {big_cactus_b_shapes, 1, 160.0F}};
static struct collision_shape big_cactus_c_shapes[1] = {
    {SHAPE_CYLINDER2,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 71.0F, 0.0F},
     9.0F,
     71.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct big_cactus_kind big_cactus_c = {&big_cactus_c_object,
                                              {big_cactus_c_shapes, 1, 160.0F}};

void big_cactus_a_step(struct task *task)
{
    task_apply_desc(task, &big_cactus_desc, &big_cactus_a);
}

void big_cactus_b_step(struct task *task)
{
    task_apply_desc(task, &big_cactus_desc, &big_cactus_b);
}

void big_cactus_c_step(struct task *task)
{
    task_apply_desc(task, &big_cactus_desc, &big_cactus_c);
}

/* give the cactus its shapes, unless the set editor holds it, and start it
 * at rest */
static void big_cactus_init(struct task *task, void *arg)
{
    struct collision_list *list = &KIND(arg)->list;
    struct task_work *work;
    struct big_cactus_record *record;
    /* @hack spelling: the test through in_play, which SHC computes with MOVT
     * R4 in the branch's delay slot (0x0c9032f8); tested directly, the MOVT
     * goes, big_cactus_init runs 2 bytes short and 24 of its 36 words
     * differ */
    int in_play = !placement_editing(task);

    if (in_play) {
        object_collision_init(task, list, &big_cactus_collision);
    }
    work = task->work;
    record = (struct big_cactus_record *)task->motion;
    record->mode = BIG_CACTUS_STAND;
    record->object = KIND(arg)->object;
    record->list = list;
    record->rest = work->ang.x;
    record->phase = 0;
    record->swing = 0;
}

/* draw the cactus and sway it. a touch sets its swing as big_cactus_push
 * says, and the swing then shrinks to 0. while the set editor holds the
 * cactus, it doesn't sway */
static void big_cactus_step(struct task *task)
{
    struct big_cactus_record *record;
    struct task_work *work;
    struct collision_work *collision;

    if (placement_try_leave(task)) {
        return;
    }
    big_cactus_draw(task);
    work = task->work;
    record = (struct big_cactus_record *)task->motion;
    if (placement_editing(task)) {
        return;
    }
    switch (record->mode) {
    case BIG_CACTUS_STAND:
        collision = work->collision;
        /* @bug memory: a cactus the set editor held as it started has no
         * collision record. once the editor lets it go, this reads the
         * record's flags through null (MOV.W @(2,R4),R0 at 0x0c903354) */
        if (!(collision->flags & COLLISION_HIT)) {
            collision_register(work);
        } else {
            collision->flags &= ~COLLISION_HIT;
            record->swing = big_cactus_push(record->swing);
        }
        work->ang.x =
            record->rest +
            (Angle)(asinf(njSin(record->swing) * njSin(record->phase)) *
                    ANGLE_PER_RADIAN);
        record->phase += SWAY_SPEED;
        record->swing -= SWAY_DECAY;
        if (record->swing <= 0) {
            record->swing = 0;
        }
        break;
    }
}

/* draw the cactus on a frame's first step */
static void big_cactus_draw(struct task *task)
{
    struct task_work *work;
    struct big_cactus_record *record;

    if (frame_step != 0) {
        return;
    }
    work = task->work;
    record = (struct big_cactus_record *)task->motion;
    object_draw_tree(record->object, work, NULL);
}

/* the swing after a touch, from player 0's speed.x, its speed along its
 * own x. the result is SWAY_MOST times |speed.x| / SWAY_SPEED_FULL, at most
 * SWAY_MOST, or swing itself when swing is larger */
static Angle big_cactus_push(Angle swing)
{
    struct player *record = player_get(0);
    Float speed = record->speed.x;
    Angle push;

    if (speed < 0.0F) {
        speed = -speed;
    }
    speed *= 1.0F / SWAY_SPEED_FULL;
    if (speed > 1.0F) {
        speed = 1.0F;
    }
    push = (Angle)(speed * SWAY_MOST);
    if (push < swing) {
        push = swing;
    }
    return push;
}
