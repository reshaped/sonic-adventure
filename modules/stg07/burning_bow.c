/* BurningBow, a bow that shoots volleys of burning arrows the way it faces.
 * each arrow trails flames */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "object/placement.h"
#include "stg07/burning_bow.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT burning_bow_object;
extern NJS_OBJECT burning_bow_arrow_object;

/* an arrow's extra */
struct arrow_extra {
    Float life; /* the steps it flies, which the bow sets */
    Float age; /* the steps it has flown */
};

/* an arrow's speed, its move a step */
#define ARROW_SPEED(work) ((work)->scl)

/* from the bow's set entry */
#define BOW_LIFE(work) ((work)->scl.x) /* an arrow's life */
#define BOW_WAIT(work) ((work)->scl.y) /* the steps before each volley */
#define BOW_ARROWS(work) ((work)->scl.z) /* the arrows in a volley */

struct bow_vars {
    Uint8 unk_00[4];
    Sint16 shot; /* the arrows shot so far in this volley */
};

#define VARS(work) ((struct bow_vars *)&(work)->vars)

enum bow_mode { BOW_START, BOW_RUN };

enum arrow_mode { ARROW_START = 0, ARROW_FLY = 4, ARROW_END = 5 };

/* the bow shoots on the step its tick passes ARROW_TICK. an arrow lights a
 * flame on the step its tick passes FLAME_TICK. each tick then starts again
 * from 0. the bow tests its tick before counting it, so arrows leave
 * ARROW_TICK + 2 steps apart. an arrow counts its tick on the step it starts
 * again. so flames start FLAME_TICK + 1 steps apart */
#define ARROW_TICK 34
#define FLAME_TICK 3

/* clang-format off */
static struct collision_shape burning_bow_shape = {
    SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F},
    5.0F, 5.0F, 0.0F, {0}, 0, NULL,
};
/* the shaft, turned to lie along z, and the head */
static struct collision_shape burning_bow_arrow_shapes[2] = {
    {SHAPE_CAPSULE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 1.0F, 3.0F, 0.0F,
     {0x4000, 0, 0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 4.8F}, 2.0F, 0.0F, 0.0F,
     {0, 0, 0}, 0, NULL},
};
/* clang-format on */

static struct dust_params burning_bow_arrow_flames = {
    0.5F,
    0.07F,
    0.35F,
    0.06F,
    0.02F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* where an arrow's flames start, before its turn about y */
static const NJS_VECTOR arrow_head = {0.0F, 0.0F, 5.0F};

static void burning_bow_arrow_draw(struct task *task);
static void burning_bow_arrow_destroy(struct task *task);

static void burning_bow_arrow_step(struct task *task)
{
    struct task_work *work = task->work;
    struct arrow_extra *extra = task->extra;
    /* @hack type: the extra read again through a volatile pointer, which
     * copies it to R4 and reads age and life back after the store (0x0c9071aa
     * to 0x0c9071b2); through extra, 102 of 147 words differ */
    volatile struct arrow_extra *flown;
    NJS_VECTOR head;
    NJS_VECTOR at;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case ARROW_START:
        task->destructor = burning_bow_arrow_destroy;
        task->draw = burning_bow_arrow_draw;
        work->submode = 0;
        work->timer = 0;
        work->mode = ARROW_FLY;
        collision_init(task, burning_bow_arrow_shapes, COLLISION_OBJECT, 1, 2);
        break;
    case ARROW_FLY:
        njAddVector(&work->pos, &ARROW_SPEED(work));
        extra->age += 1.0F;
        flown = (volatile struct arrow_extra *)&extra->life;
        if (flown->life < flown->age) {
            work->mode = ARROW_END;
        }
        if (work->tick > FLAME_TICK) {
            head = arrow_head;
            njPushMatrix(&_unit_matrix_);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njCalcVector(NULL, &head, &at);
            njPopMatrix(1);
            burning_bow_arrow_flames.pos.x = work->pos.x + at.x;
            burning_bow_arrow_flames.pos.y = work->pos.y + at.y;
            burning_bow_arrow_flames.pos.z = work->pos.z + at.z;
            flame_create_from(&burning_bow_arrow_flames);
            work->tick = 0;
        }
        ++work->tick;
        /* a flying arrow registers here and again below, so it enters the
         * collision checks twice a step */
        collision_register(work);
        break;
    case ARROW_END:
        task_kill(task);
    }
    collision_register(work);
    burning_bow_arrow_draw(task);
}

static void burning_bow_arrow_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    texture_set_stage();
    TURN_VIA(njRotateY, work->ang.y, ang);
    /* @hack spelling: the model read through the object's address, which the
     * pool holds, with MOV.L @(4,R4),R4 at 0x0c90729a. as
     * burning_bow_arrow_object.model, the pool holds the field's address,
     * and 6 of 33 words differ */
    draw_model((&burning_bow_arrow_object)->model);
    njPopMatrix(1);
}

static void burning_bow_arrow_destroy(struct task *task)
{
}

static void burning_bow_destroy(struct task *task)
{
}

static void burning_bow_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    /* @hack spelling: the model read through the object's address, which the
     * pool holds, with MOV.L @(4,R4),R4 at 0x0c907356. as
     * burning_bow_object.model, the pool holds the field's address, and 6 of
     * 32 words differ */
    draw_model((&burning_bow_object)->model);
    njPopMatrix(1);
}

/* once its wait is up, the bow shoots the volley an arrow at a time, then
 * waits again. an arrow flies a unit a step the way the bow faces */
void burning_bow_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *arrow;
    /* @hack redundant: 24 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint8[24]. the frame takes 52 bytes (ADD #-52,R15 at
     * 0x0c90736e); without them, it takes 28, and 2 of 170 words differ */
    Uint8 unk_frame[24];
    NJS_VECTOR ahead;
    NJS_VECTOR speed;
    Angle ang;

    switch (work->mode) {
    case BOW_START:
        work->mode = BOW_RUN;
        task->destructor = burning_bow_destroy;
        task->draw = burning_bow_draw;
        collision_init(task, &burning_bow_shape, COLLISION_OBJECT, 1, 1);
        /* fall through */
    case BOW_RUN:
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<-DEAD FRAME SPEED");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<-TIMING FRAME");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<-ARROW NUM ");
        }
        if (BOW_WAIT(work) <= TASK_TIMER(work)++) {
            if (work->tick++ > ARROW_TICK) {
                work->tick = 0;
                ++VARS(work)->shot;
                if (VARS(work)->shot > (int)BOW_ARROWS(work)) {
                    work->timer = 0;
                    VARS(work)->shot = 0;
                } else {
                    arrow = task_create(TASK_WORK | TASK_EXTRA, 3,
                                        burning_bow_arrow_step);
                    arrow->work->pos.x = work->pos.x;
                    arrow->work->pos.y = work->pos.y;
                    arrow->work->pos.z = work->pos.z;
                    arrow->work->ang.x = work->ang.x;
                    arrow->work->ang.y = work->ang.y;
                    ahead.x = ahead.y = 0.0F;
                    ahead.z = 1.0F;
                    njPushMatrix(&_unit_matrix_);
                    TURN_VIA(njRotateY, work->ang.y, ang);
                    njCalcVector(NULL, &ahead, &speed);
                    njPopMatrix(1);
                    work->ang.z = 0;
                    ARROW_SPEED(arrow->work).x = speed.x;
                    ARROW_SPEED(arrow->work).y = speed.y;
                    ARROW_SPEED(arrow->work).z = speed.z;
                    ((struct arrow_extra *)arrow->extra)->life = BOW_LIFE(work);
                }
            }
        }
    }
    collision_register(work);
    burning_bow_draw(task);
}
