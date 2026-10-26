/* the kusa, CMN KUSA: a plant that Tails' tail attack or Amy's hammer spin
 * cuts down. once cut, it may drop an item, and its seven leaves fly off
 * and fade */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/helpers.h"
#include "object/kusa.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/amy.h"
#include "player/tails.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texlists.h"
#include "system/util.h"

/* a leaf's motion each step. an x or z speed of more than LEAF_SLOW either
 * way scales by LEAF_DRAG, and the leaf rises LEAF_RISE */
#define LEAF_DRAG 0.7F
#define LEAF_SLOW 0.15F
#define LEAF_RISE 0.06F

/* the alpha a leaf loses a step once its flight's steps run out */
#define LEAF_FADE 0.05F

/* the vars of the plant and of each leaf. a leaf counts its flight's steps
 * down in work->timer */
struct kusa_vars {
    NJS_OBJECT *object;
    /* a leaf's constant material alpha, an offset. it falls from 0, which
     * draws the leaf as it is, to -1, which clears it */
    Float alpha;
    NJS_TEXLIST *texlist;
};

#define VARS(work) ((struct kusa_vars *)&(work)->vars)

enum kusa_mode { KUSA_INIT, KUSA_WAIT, KUSA_BROKEN };
enum leaf_mode { LEAF_INIT, LEAF_FLY, LEAF_FADE_OUT };

static void kusa_leaf_step(struct task *task);
static void kusa_leaf_start(struct task *task, struct task_work *work,
                            struct task_force *force);
static void kusa_leaf_move(struct task_work *work, struct task_force *force);
static void kusa_drop_item(struct task_work *work);
static void kusa_draw(struct task *task);
static void kusa_leaf_draw(struct task *task);

/* clang-format off */
static struct collision_shape kusa_shapes[] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE, 0, PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F},
     3.5F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* the leaves, numbered 0 to 6 in their work's flags. each starts at its
 * offset from the plant */
static struct task_spec kusa_leaves[] = {
    {kusa_leaf_step, TASK_WORK | TASK_FORCE, 0, {0}, {0.1F, 3.5F, -0.0F}, 0},
    {kusa_leaf_step, TASK_WORK | TASK_FORCE, 1, {0}, {-0.3F, 3.2F, 0.2F}, 0},
    {kusa_leaf_step, TASK_WORK | TASK_FORCE, 2, {0}, {0.4F, 2.8F, -0.6F}, 0},
    {kusa_leaf_step, TASK_WORK | TASK_FORCE, 3, {0}, {-0.6F, 2.5F, -0.4F}, 0},
    {kusa_leaf_step, TASK_WORK | TASK_FORCE, 4, {0}, {0.6F, 2.3F, -0.8F}, 0},
    {kusa_leaf_step, TASK_WORK | TASK_FORCE, 5, {0}, {-0.8F, 2.2F, 0.8F}, 0},
    {kusa_leaf_step, TASK_WORK | TASK_FORCE, 6, {0}, {1.0F, 2.0F, 1.2F}, 0},
    {NULL, 0, 0, {0}, {0.0F, 0.0F, 0.0F}, 0},
};

/* what the plant drops: each chance, then the drop_kind it gives. the
 * chances add up to 0.25, so three times in four it drops nothing */
static Float kusa_drops[] = {
    0.15F,  DROP_RING,         0.05F,  DROP_5_RINGS, 0.03F, DROP_10_RINGS,
    0.015F, DROP_RANDOM_RINGS, 0.005F, DROP_1UP,     0.0F,  0.0F,
};

void kusa_step(struct task *task)
{
    struct task_work *work = task->work;
    struct touch *touch = task->extra;
    struct task_work *by;
    int touched;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case KUSA_INIT:
        VARS(work)->texlist = &obj_regular_texlist;
        task->draw = kusa_draw;
        /* a plant cut down before comes back cut down, and only draws until
         * it leaves */
        if (placement_used(task)) {
            VARS(work)->object = &kusa_cut_object;
            task->step = object_step_draw_range;
            break;
        } else {
            work->mode = KUSA_WAIT;
            VARS(work)->object = &kusa_object;
            collision_init(task, kusa_shapes, COLLISION_OBJECT, 1, 1);
        }
        /* falls through, so it starts waiting this step */
    case KUSA_WAIT:
        /* @hack spelling: touched assigned in the test, which tests
         * object_touch_moved's R0 and copies it to R12 in the BT/S's slot
         * (0x8c05dae6, 0x8c05daea); assigned apart, SHC tests R12,
         * kusa.c's code runs 32 bytes short and 23 of 115 words differ */
        if ((touched = object_touch_moved(work, touch)) != 0) {
            by = touch->by.owner->work;
            if (by != NULL && (((touched & (1 << CHARACTER_TAILS)) &&
                                (TAILS_STATE(by) & TAILS_ATTACKING)) ||
                               ((touched & (1 << CHARACTER_AMY)) &&
                                amy_get_action(0) == AMY_SPINNING))) {
                work->mode = KUSA_BROKEN;
                placement_set_used(task);
                VARS(work)->object = &kusa_cut_object;
                sound_play(SE_LAND, 0, 0, 0);
                kusa_drop_item(work);
                task_create_children(kusa_leaves, task);
                task_step_children(task);
                break;
            }
        }
        collision_register(work);
        break;
    case KUSA_BROKEN:
        task_step_children(task);
        break;
    }
    object_step_draw(task);
}

static void kusa_leaf_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case LEAF_INIT:
        work->mode = LEAF_FLY;
        VARS(work)->object = &kusa_leaf_object;
        VARS(work)->texlist = &obj_regular_texlist;
        task->draw = kusa_leaf_draw;
        kusa_leaf_start(task, work, force);
        break;
    case LEAF_FLY:
        kusa_leaf_move(work, force);
        if (work->timer-- < 0) {
            work->mode = LEAF_FADE_OUT;
            VARS(work)->alpha = 0.0F;
        }
        break;
    case LEAF_FADE_OUT:
        /* @hack spelling: alpha stepped in the test, which compares the FR2 it
         * stores (0x8c05dc48, 0x8c05dc4e); stepped apart, SHC loads it back,
         * the function runs 4 bytes long and 32 of 87 words differ */
        if ((VARS(work)->alpha -= LEAF_FADE) < -1.0F) {
            task_kill(task);
            return;
        }
        kusa_leaf_move(work, force);
        /* a fading leaf draws here and again in object_step_draw */
        kusa_leaf_draw(task);
        break;
    }
    object_step_draw(task);
}

/* start a leaf. its speed is the hitter's last move, plus a random push of
 * -2.25 to 2.5 in x and in z. it takes random angles. each step, it turns
 * about each axis by a 64th of its starting angle there. it flies until its
 * timer, set from 30 to 59, runs out */
static void kusa_leaf_start(struct task *task, struct task_work *work,
                            struct task_force *force)
{
    NJS_VECTOR *speed = &((struct touch *)task->parent->extra)->moved;
    Float roll[3];
    int i;

    for (i = 0; i < 3; i++) {
        /* @hack spelling: roll[i] assigned in the test, which stores the roll
         * in the BF/S's slot (FMOV.S FR3,@R12 at 0x8c05dcc2); assigned apart,
         * 91 of 117 words differ */
        if ((roll[i] = rand() * RAND_SCALE) < 0.05F) {
            roll[i] = 0.05F;
        }
    }
    force->speed.x = speed->x + (roll[0] * 5.0F) - 2.5F;
    force->speed.y = speed->y;
    force->speed.z = speed->z + (roll[2] * 5.0F) - 2.5F;
    work->ang.x = (Angle)(roll[0] * 360.0F * ANGLE_PER_DEGREE);
    work->ang.y = (Angle)(roll[1] * 360.0F * ANGLE_PER_DEGREE);
    work->ang.z = (Angle)(roll[2] * 360.0F * ANGLE_PER_DEGREE);
    force->turn.x = work->ang.x >> 6;
    force->turn.y = work->ang.y >> 6;
    force->turn.z = work->ang.z >> 6;
    work->timer = RANDOM(30.0F, 60.0F);
}

static void kusa_leaf_move(struct task_work *work, struct task_force *force)
{
    /* @hack spelling: the x and z speeds through pointers, formed with ADD #16
     * and ADD #24 (0x8c05ddc2, 0x8c05ddd2); through the fields, 63 of 64
     * words differ */
    Float *x = &force->speed.x;
    Float *z = &force->speed.z;

    if ((*x < 0.0F ? -*x : *x) > LEAF_SLOW) {
        *x *= LEAF_DRAG;
    }
    work->pos.x += *x;
    /* @hack spelling: LEAF_RISE subtracted as its negative, which keeps the
     * image's FSUB and its -0.06F (FSUB FR3,FR2 at 0x8c05de00); added, SHC
     * writes an FADD and 1 of 64 words differ */
    work->pos.y -= -LEAF_RISE;
    if ((*z < 0.0F ? -*z : *z) > LEAF_SLOW) {
        *z *= LEAF_DRAG;
    }
    work->pos.z += *z;
    work->ang.x += force->turn.x;
    work->ang.y += force->turn.y;
    work->ang.z += force->turn.z;
}

static void kusa_drop_item(struct task_work *work)
{
    enum drop_kind kind;

    /* @hack spelling: kind assigned in the test, which tests item_pick's R0 and
     * copies it to R5 in the BT/S's slot (0x8c05de4e, 0x8c05de52); assigned
     * apart, SHC tests R5 and 3 of 19 words differ */
    if ((kind = item_pick(kusa_drops)) != 0) {
        item_drop(work, kind);
    }
}

static void kusa_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    njSetTexture(VARS(work)->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_tree(VARS(work)->object);
    njPopMatrix(1);
}

static void kusa_leaf_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    njSetTexture(VARS(work)->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(VARS(work)->alpha, 0.0F, 0.0F, 0.0F);
    draw_model(VARS(work)->object->model);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
    njPopMatrix(1);
}
