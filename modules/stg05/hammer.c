/* O HANMA, a hammer whose arm swings down onto the ground, raising dust,
 * and lifts again
 *
 * a set entry's scale x is the hammer's size, and a scale x of 0 counts as 1.
 * its scale y speeds the fall, and its scale z is the angle the arm lifts
 * each step. the set editor labels them HANMA SCALE, DOWN ROT ACCELERATE and
 * UP ROT SPEED */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/models.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg05/hammer.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT hammer_arm_object;
extern NJS_MODEL hammer_base_model;

struct hammer_vars {
    /* the arm's angle about x, from 0 on the ground to HAMMER_RAISED */
    Float swing;
    /* each step of the fall lowers swing by fall * cos(swing) plus pull *
     * sin(swing). fall starts at 0 and gains the set entry's scale y each
     * step */
    Float fall;
    Float pull; /* the set entry's scale y, but at most MOST_PULL */
};

#define VARS(work) ((struct hammer_vars *)&(work)->vars)

/* the hammer's state, in place of its task's motion. its points and
 * lengths are at the hammer's size */
struct hammer_motion {
    void *unk_00;
    NJS_POINT3 head; /* the head's center in the arm's own space */
    NJS_POINT3 impact; /* the head's center in the world with the arm down */
    Uint8 unk_1c[24];
    Float dust_radius; /* the radius of the ring of dust the head raises */
    Float dust_scale; /* the scale of each puff in the ring */
};

/* the vars of the head's sphere, a child task of the hammer. the child's
 * scl holds the sphere's center in the arm's own space, at the hammer's
 * size */
struct hammer_head_vars {
    Uint8 unk_00[8];
    Float radius; /* the sphere's radius, at the hammer's size */
};

#define HEAD(work) ((struct hammer_head_vars *)&(work)->vars)

enum hammer_mode { HAMMER_FALL, HAMMER_LIFT, HAMMER_DOWN, HAMMER_UP };

/* the swing at the top */
#define HAMMER_RAISED ((Float)NJM_DEG_ANG(63))
/* the hammer rests on the ground and at the top while its timer is at most
 * HAMMER_REST_LIMIT. the timer is 0 as the arm gets there. so the arm stays
 * put for the 57 steps after */
#define HAMMER_REST_LIMIT 55
#define MOST_PULL 5.0F

/* the head's attack level while the hammer falls */
#define HEAD_ATTACK 2

/* the hammer destroys itself once neither the camera nor player 0 is
 * within 1000 of it. player 1 as Sonic within 40 also keeps it. the range
 * is squared, as placement_try_leave_by takes it */
#define HAMMER_LEAVE_RANGE (1000.0F * 1000.0F)
#define DRAW_RADIUS 100.0F

#define SE_HAMMER_LAND 125

/* the head's sphere. it hurts players while the hammer falls. it pushes
 * from the step the arm is down until the hammer falls again */
static struct collision_shape hammer_head_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE,
    SHAPE_HURTS_PLAYERS | HEAD_ATTACK,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    25.0F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

static void hammer_run(struct task *task);
static void hammer_draw(struct task *task);
static void hammer_destroy(struct task *task);
static void hammer_dust(NJS_POINT3 *center, Uint8 count, Float radius,
                        Float scale);
static void hammer_head_step(struct task *task);
static void hammer_head_follow(struct task *task);

/* the centers of the head and its sphere in the arm's own space, at a size
 * of 1 */
static const NJS_POINT3 hammer_head = {0.0F, -50.0F, -104.0F};
static const NJS_POINT3 hammer_sphere = {0.0F, -32.0F, -102.0F};

/* set the hammer up raised, about to fall. outside the set editor, it
 * creates the head's sphere as a child task */
void hammer_step(struct task *task)
{
    struct task_work *work = task->work;
    struct hammer_motion *motion = (struct hammer_motion *)task->motion;
    NJS_POINT3 point;
    Angle ang;
    Float scale;

    work->mode = HAMMER_FALL;
    VARS(work)->fall = 0.0F;
    VARS(work)->swing = HAMMER_RAISED;
    if (work->scl.x == 0.0F) {
        work->scl.x = 1.0F;
    }
    if (work->scl.y <= MOST_PULL) {
        VARS(work)->pull = work->scl.y;
    } else {
        VARS(work)->pull = MOST_PULL;
    }
    point = hammer_head;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN_VIA(njRotateY, work->ang.y, ang);
    scale = work->scl.x;
    njScale(NULL, scale, scale, scale);
    njCalcPoint(NULL, &point, &motion->impact);
    njPopMatrix(1);
    motion->impact.x += work->pos.x;
    motion->impact.y += work->pos.y;
    motion->impact.z += work->pos.z;
    motion->dust_radius = work->scl.x * 15.0F;
    motion->dust_scale = work->scl.x * 1.0F;
    /* nothing in this file reads the tick this sets */
    if (work->scl.x <= 0.3F) {
        work->tick = 8;
    } else {
        work->tick = 16;
    }
    motion->head.x = work->scl.x * 0.0F;
    motion->head.y = -(work->scl.x * 50.0F);
    motion->head.z = -(work->scl.x * 104.0F);
    if (!placement_editing(task)) {
        struct task *child =
            task_create_child(TASK_WORK, hammer_head_step, task);
        struct task_work *head = child->work;

        point = hammer_sphere;
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        scale = work->scl.x;
        njScale(NULL, scale, scale, scale);
        njCalcPoint(NULL, &point, &head->scl);
        njPopMatrix(1);
        HEAD(head)->radius = work->scl.x * 25.0F;
    }
    task->step = hammer_run;
    task->destructor = hammer_destroy;
    task->draw = hammer_draw;
}

/* a second copy of hammer_head in the image, which hammer_run reads */
static const NJS_POINT3 hammer_run_head = {0.0F, -50.0F, -104.0F};

/* fall, rest on the ground, lift and rest at the top, in turn. in the set
 * editor, the hammer has no sphere and takes its set entry's edits each
 * step */
static void hammer_run(struct task *task)
{
    struct task_work *work = task->work;
    struct hammer_motion *motion = (struct hammer_motion *)task->motion;
    NJS_POINT3 point;
    Angle ang;
    Float scale;

    if (placement_try_leave_by(task, HAMMER_LEAVE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case HAMMER_FALL:
        VARS(work)->fall += work->scl.y;
        VARS(work)->swing -= (VARS(work)->fall * njCos(VARS(work)->swing)) +
                             (VARS(work)->pull * njSin(VARS(work)->swing));
        if (VARS(work)->swing <= 0.0F) {
            VARS(work)->swing = 0.0F;
            VARS(work)->fall = 0.0F;
            work->mode = HAMMER_DOWN;
            work->timer = 0;
            sound_play_at(SE_HAMMER_LAND, 0, 0, 64, work->pos.x, work->pos.y,
                          work->pos.z);
            hammer_dust(&motion->impact, 16, motion->dust_radius,
                        motion->dust_scale);
            if (!placement_editing(task)) {
                task->child->work->collision->shapes->push |= PUSH_ALL;
                /* @hack spelling: the clear spelled out, which reads the byte
                 * through R0, MOV.B @(R0,R4),R0 at 0x0c9075ea; with &=, SHC
                 * reads it at @(4,R3), and 8 of 339 words differ */
                task->child->work->collision->shapes->levels =
                    task->child->work->collision->shapes->levels &
                    ~SHAPE_ATTACK;
            }
        }
        break;
    case HAMMER_DOWN:
        if (TASK_TIMER(work) > HAMMER_REST_LIMIT) {
            work->mode = HAMMER_LIFT;
        }
        ++work->timer;
        break;
    case HAMMER_LIFT:
        VARS(work)->swing += work->scl.z;
        if (VARS(work)->swing > HAMMER_RAISED) {
            VARS(work)->swing = HAMMER_RAISED;
            work->mode = HAMMER_UP;
            work->timer = 0;
        }
        break;
    case HAMMER_UP:
        if (TASK_TIMER(work) > HAMMER_REST_LIMIT) {
            work->mode = HAMMER_FALL;
            if (!placement_editing(task)) {
                task->child->work->collision->shapes->push &=
                    PUSH_FIRM(PUSH_ALL);
                task->child->work->collision->shapes->levels =
                    (task->child->work->collision->shapes->levels &
                     ~SHAPE_ATTACK) |
                    HEAD_ATTACK;
            }
        }
        ++work->timer;
        break;
    }
    if (!placement_editing(task)) {
        task_step_children(task);
    }
    hammer_draw(task);
    if (placement_editing(task)) {
        if (work->scl.x == 0.0F) {
            work->scl.x = 1.0F;
        }
        if (work->scl.y <= MOST_PULL) {
            VARS(work)->pull = work->scl.y;
        } else {
            VARS(work)->pull = MOST_PULL;
        }
        point = hammer_run_head;
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        TURN_VIA(njRotateY, work->ang.y, ang);
        scale = work->scl.x;
        njScale(NULL, scale, scale, scale);
        njCalcPoint(NULL, &point, &motion->impact);
        njPopMatrix(1);
        motion->impact.x += work->pos.x;
        motion->impact.y += work->pos.y;
        motion->impact.z += work->pos.z;
        motion->dust_radius = work->scl.x * 15.0F;
        motion->dust_scale = work->scl.x * 1.0F;
        if (work->scl.x <= 0.3F) {
            work->tick = 8;
        } else {
            work->tick = 16;
        }
        motion->head.x = work->scl.x * 0.0F;
        motion->head.y = -(work->scl.x * 50.0F);
        motion->head.z = -(work->scl.x * 104.0F);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> HANMA SCALE");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> DOWN ROT ACCELERATE");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> UP ROT SPEED");
    }
}

/* draw the base, the arm at its swing, and a shadow on the ground below the
 * head. the shadow shrinks as the head rises */
static void hammer_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct hammer_motion *motion = (struct hammer_motion *)task->motion;
    NJS_OBJECT *arm;
    Angle ang;
    struct angles slope;
    NJS_POINT3 head;
    Float ground;
    Float scale;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        return;
    }
    /* @hack spelling: the arm through a local, which keeps it at sp+8 and the
     * frame at 36 bytes (ADD #-36,R15 at 0x0c90786a); by name, the frame is
     * 32, the function 4 bytes shorter, and 175 of 195 words differ */
    arm = &hammer_arm_object;
    njSetTexture(&obj_mountain_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    scale = work->scl.x;
    njScale(NULL, scale, scale, scale);
    draw_model(&hammer_base_model);
    TURN_VIA(njRotateX, VARS(work)->swing, ang);
    draw_model(arm->model);
    njPopMatrix(1);
    texture_set_regular();
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, VARS(work)->swing, ang);
    njCalcPoint(NULL, &motion->head, &head);
    njPopMatrix(1);
    ground = ground_below(head.x, head.y, head.z, &slope) + 0.1F;
    scale = 6.0F * work->scl.x;
    if (head.y > ground) {
        scale = scale / (((head.y - ground) * 0.03F) + 1.0F);
        head.y = ground;
        njPushMatrix(NULL);
        njTranslateV(NULL, &head);
        TURN_VIA(njRotateZ, slope.z, ang);
        TURN_VIA(njRotateX, slope.x, ang);
        njScale(NULL, scale, scale, scale);
        draw_tree(shadow_object);
        njPopMatrix(1);
    }
}

static void hammer_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* raise count puffs of dust at scale, radius out from center, each blown
 * outward and up. step is a whole turn over count. for puff n, from 0, the
 * loop turns the matrix n steps further and never resets it. so puff n lies
 * n(n + 1) / 2 steps round, not n */
static void hammer_dust(NJS_POINT3 *center, Uint8 count, Float radius,
                        Float scale)
{
    NJS_VECTOR speed = {0.0F, 0.25F, 2.0F};
    NJS_POINT3 offset = {0.0F, 0.0F, 0.0F};
    NJS_POINT3 pos;
    NJS_VECTOR turned;
    Angle step;
    Angle ang;
    Uint8 i;

    step = 0x10000 / count;
    ang = 0;
    offset.z = radius;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    speed.y *= scale;
    speed.z *= scale;
    for (i = 0; i < count; ++i) {
        TURN(njRotateY, ang);
        njCalcPoint(NULL, &offset, &pos);
        njCalcVector(NULL, &speed, &turned);
        pos.x += center->x;
        pos.y += center->y;
        pos.z += center->z;
        dust_create(&pos, &turned, scale);
        ang += step;
    }
    njPopMatrix(1);
}

static void hammer_head_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &hammer_head_shape, COLLISION_OBJECT, 1, 1);
    work->collision->shapes->radius = HEAD(work)->radius;
    work->collision->shapes->push &= PUSH_FIRM(PUSH_ALL);
    work->collision->shapes->levels =
        (work->collision->shapes->levels & ~SHAPE_ATTACK) | HEAD_ATTACK;
    task->step = hammer_head_follow;
}

/* place the sphere where the arm carries its scl, and register it */
static void hammer_head_follow(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *hammer = task->parent->work;
    Angle ang;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &hammer->pos);
    TURN_VIA(njRotateY, hammer->ang.y, ang);
    TURN_VIA(njRotateX, VARS(hammer)->swing, ang);
    njCalcPoint(NULL, &work->scl, &work->pos);
    njPopMatrix(1);
    collision_register(work);
}
