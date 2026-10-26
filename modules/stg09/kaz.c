/* O KAZB to O KAZG models that turn about z, and O KAZA, whose two parts move
 * apart and back on its action */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "object/placement.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/kaz.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT kaz_b_object;
extern NJS_OBJECT kaz_c_object;
extern NJS_OBJECT kaz_d_object;
extern NJS_OBJECT kaz_e_object;
extern NJS_OBJECT kaz_f_object;
extern NJS_OBJECT kaz_g_object;
/* O KAZA's model: the lower part, whose child is the upper */
extern NJS_OBJECT kaz_a_object;
extern NJS_ACTION kaz_a_action;

/* destroyed beyond about 250 units of the camera and player 0 */
#define RANGE ((250.0F * 250.0F) + 10.0F)
/* the radius of the sphere about O KAZA that draw_in_view tests before O KAZA
 * draws */
#define KAZ_A_VIEW_RADIUS 98.0F

struct kaz_vars {
    Uint16 angle; /* a turning model's, about its axis */
    Uint8 unk_02[2];
    NJS_MODEL *model; /* the turning model */
    Float frame; /* O KAZA's action's */
};

#define VARS(work) ((struct kaz_vars *)&(work)->vars)

/* a turning model turns kaz_turn times kaz_scale, 150 Angle units, a step
 * for each unit of its speed */
static Angle kaz_turn = 100;
static Float kaz_scale = 1.5F;
/* O KAZA's action moves twice this on each step but a frame's first */
static Float kaz_a_frame_rate = 0.1F;
static struct collision_shape kaz_b_shape = {
    SHAPE_CYLINDER2,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    35.625F,
    4.0F,
    0.0F,
    {NJM_DEG_ANG(90), 0, 0},
    0,
    NULL,
};
static struct collision_shape kaz_c_shape = {
    SHAPE_CYLINDER2,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    65.375F,
    3.5F,
    0.0F,
    {NJM_DEG_ANG(90), 0, 0},
    0,
    NULL,
};
static struct collision_shape kaz_d_shape = {
    SHAPE_CYLINDER2,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    52.685F,
    4.84F,
    0.0F,
    {NJM_DEG_ANG(90), 0, 0},
    0,
    NULL,
};
static struct collision_shape kaz_e_shape = {
    SHAPE_CYLINDER2,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 5.14F},
    21.55F,
    5.14F,
    0.0F,
    {NJM_DEG_ANG(90), 0, 0},
    0,
    NULL,
};
/* a cylinder about the axis, and two boxes that kaz_f_run turns with the
 * model */
static struct collision_shape kaz_f_shapes[3] = {
    {
        SHAPE_CYLINDER2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 35.0F},
        25.6F,
        35.0F,
        0.0F,
        {NJM_DEG_ANG(90), 0, 0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {-9.5F, 0.0F, 110.75F},
        9.5F,
        3.0F,
        40.75F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {9.5F, 0.0F, 192.25F},
        9.5F,
        3.0F,
        40.75F,
        {0},
        0,
        NULL,
    },
};
static struct collision_shape kaz_g_shape = {
    SHAPE_CYLINDER2,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    18.5F,
    115.5F,
    0.0F,
    {NJM_DEG_ANG(90), 0, 0},
    0,
    NULL,
};
/* shapes 0 and 2 ride O KAZA's lower part, and 1 and 3 its upper. kaz_a_run
 * puts each at its part's origin plus the center here */
static struct collision_shape kaz_a_shapes[4] = {
    {
        SHAPE_CYLINDER,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 97.8F, 0.0F},
        14.0F,
        8.0F,
        0.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_CYLINDER,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, -106.0F, 0.0F},
        14.0F,
        8.0F,
        0.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_CYLINDER,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 50.0F, 0.0F},
        7.0F,
        55.0F,
        0.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_CYLINDER,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, -50.0F, 0.0F},
        7.0F,
        52.0F,
        0.0F,
        {0},
        0,
        NULL,
    },
};

static void kaz_start(struct task *task, NJS_MODEL *model,
                      const struct collision_shape *shape, Sint8 axis);
static void kaz_f_start(struct task *task, NJS_MODEL *model,
                        const struct collision_shape *shapes, Sint8 axis);

void kaz_b_step(struct task *task)
{
    kaz_start(task, kaz_b_object.model, &kaz_b_shape, 0);
}

void kaz_c_step(struct task *task)
{
    kaz_start(task, kaz_c_object.model, &kaz_c_shape, 0);
}

void kaz_d_step(struct task *task)
{
    kaz_start(task, kaz_d_object.model, &kaz_d_shape, 0);
}

void kaz_e_step(struct task *task)
{
    kaz_start(task, kaz_e_object.model, &kaz_e_shape, 0);
}

void kaz_f_step(struct task *task)
{
    kaz_f_start(task, kaz_f_object.model, kaz_f_shapes, 0);
}

void kaz_g_step(struct task *task)
{
    kaz_start(task, kaz_g_object.model, &kaz_g_shape, 0);
}

static void kaz_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, RANGE) || frame_step) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if (work->submode) {
        TURN_VIA(njRotateX, VARS(work)->angle, ang);
    } else {
        TURN_VIA(njRotateZ, VARS(work)->angle, ang);
    }
    draw_model(VARS(work)->model);
    njPopMatrix(1);
}

static void kaz_run(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Angle turn;
    Float factor;
    Float steps;
    Float scaled;
    Sint16 *low;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if (work->submode) {
        TURN_VIA(njRotateX, VARS(work)->angle, ang);
    } else {
        TURN_VIA(njRotateZ, VARS(work)->angle, ang);
    }
    draw_model(VARS(work)->model);
    njPopMatrix(1);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X),
                 TURNS_RIGHT(work) ? "RotDir:Migi" : "RotDir:Hidari");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "RotSpd:");
        njPrintD(EDITOR_NUMBER(EDITOR_ROW_SCL_Y), (int)njRoundOff(work->scl.y),
                 4);
        return;
    }
    collision_register(work);
    steps = TASK_TIMER(work);
    turn = kaz_turn;
    if (TURNS_RIGHT(work)) {
        factor = kaz_scale * njRoundOff(work->scl.y) * steps;
    } else {
        factor = -(kaz_scale * njRoundOff(work->scl.y) * steps);
    }
    /* scaled, kaz_turn times factor, can pass a Sint16's range. FTRC converts
     * it to an Angle, and only that Angle's low half is kept (0x0c91267a) */
    /* @hack type: turn scaled through a Sint16 pointer to its low half on
     * the stack, which the image loads at 0x0c912672 and stores at
     * 0x0c91267e; as union angle_halves, 56 of 157 words differ */
    low = (Sint16 *)&turn;
    scaled = *low;
    scaled *= factor;
    *low = (Angle)scaled;
    VARS(work)->angle = turn;
    work->timer++;
}

/* the model turns about x for a nonzero axis, and about z for 0 */
static void kaz_start(struct task *task, NJS_MODEL *model,
                      const struct collision_shape *shape, Sint8 axis)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            collision_init(task, shape, COLLISION_OBJECT, 1, 1);
        }
        task->draw = kaz_draw;
        task->step = kaz_run;
        VARS(work)->model = model;
        work->submode = axis;
        VARS(work)->angle = 0;
        work->timer = 0;
    }
}

static void kaz_f_run(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: a point nothing reads, which gives the image's 20-byte
     * stack frame (ADD #-20 at 0x0c91276a). without it, 2 of its 154 words
     * differ */
    NJS_POINT3 unused;
    Angle ang;
    Angle turn;
    Float factor;
    Float steps;
    Float scaled;
    Sint16 *low;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    kaz_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X),
                 TURNS_RIGHT(work) ? "RotDir:Migi" : "RotDir:Hidari");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "RotSpd:");
        njPrintD(EDITOR_NUMBER(EDITOR_ROW_SCL_Y), (int)njRoundOff(work->scl.y),
                 4);
        return;
    }
    collision_register(work);
    steps = TASK_TIMER(work);
    turn = kaz_turn;
    if (TURNS_RIGHT(work)) {
        factor = kaz_scale * njRoundOff(work->scl.y) * steps;
    } else {
        factor = -(kaz_scale * njRoundOff(work->scl.y) * steps);
    }
    /* @hack type: as in kaz_run, with the load at 0x0c912814, FTRC at
     * 0x0c91281c and the store at 0x0c912820; as union angle_halves, 95 of
     * 154 words differ */
    low = (Sint16 *)&turn;
    scaled = *low;
    scaled *= factor;
    *low = (Angle)scaled;
    VARS(work)->angle = turn;
    work->collision->shapes[1].ang.z = VARS(work)->angle;
    work->collision->shapes[2].ang.z = VARS(work)->angle;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateZ, VARS(work)->angle, ang);
    njCalcPoint(NULL, &kaz_f_shapes[1].center,
                &work->collision->shapes[1].center);
    njCalcPoint(NULL, &kaz_f_shapes[2].center,
                &work->collision->shapes[2].center);
    njPopMatrix(1);
    work->timer++;
}

static void kaz_f_start(struct task *task, NJS_MODEL *model,
                        const struct collision_shape *shapes, Sint8 axis)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            collision_init(task, shapes, COLLISION_OBJECT, 1, 3);
        }
        task->draw = kaz_draw;
        task->step = kaz_f_run;
        VARS(work)->model = model;
        work->submode = axis;
        VARS(work)->angle = 0;
        work->timer = 0;
    }
}

/* pose the action at frame. set first to node 0's origin, and second to node
 * 1's, both in the model's space. node 0 is the lower part, and node 1 the
 * upper. returns TRUE, as pose_action always does */
static Bool kaz_a_pose(Float frame, NJS_POINT3 *first, NJS_POINT3 *second)
{
    /* Ninja needs a matrix on an 8-byte boundary, and the stack is aligned
     * only to 4. matrix points at the boundary at or below shifted.matrix's
     * start. storage keeps that boundary, and a whole matrix after it, inside
     * the struct */
    struct {
        NJS_MATRIX *matrix;
        union {
            NJS_MATRIX matrix;
            struct {
                Float alignment;
                NJS_MATRIX matrix;
            } shifted;
        } storage;
    } aligned;
    union point_floats origin;
    Float *at;

    aligned.matrix =
        (NJS_MATRIX *)((Uint32)&aligned.storage.shifted.matrix & ~7);
    njPushMatrix(&_unit_matrix_);
    if (pose_action(NULL, &kaz_a_action, frame, 0, aligned.matrix)) {
        /* @hack spelling: the array walked as (&origin)->elements, whose
         * address SHC forms from the stack pointer at each use (MOV R15,R4 at
         * 0x0c9129b0). as origin.elements, the address takes R13, and 73 of
         * kaz_a_pose's 79 words differ */
        at = (&origin)->elements;
        *at++ = 0.0F;
        *at++ = 0.0F;
        *at = 0.0F;
        njCalcPoint(aligned.matrix, &origin.point, first);
        njUnitMatrix(NULL);
        if (pose_action(NULL, &kaz_a_action, frame, 1, aligned.matrix)) {
            /* the translation below moves the current matrix, which
             * njCalcPoint doesn't read. second is node 1's origin as posed */
            njTranslateV(NULL, (NJS_VECTOR *)kaz_a_object.child->pos);
            njCalcPoint(aligned.matrix, &origin.point, second);
        }
        njPopMatrix(1);
        return TRUE;
    }
    njPopMatrix(1);
    return FALSE;
}

static void kaz_a_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                      KAZ_A_VIEW_RADIUS)) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_action(&kaz_a_action, VARS(work)->frame);
    njPopMatrix(1);
}

static void kaz_a_run(struct task *task)
{
    struct task_work *work = task->work;
    Float frames;
    /* @hack spelling: a copy of kaz_a_shapes' address, which SHC loads into
     * R4 (0x0c912bfa). read through kaz_a_shapes, the address takes R5, and 44
     * of kaz_a_run's 177 words differ */
    const struct collision_shape *shapes;
    NJS_POINT3 first;
    NJS_POINT3 second;
    /* @hack spelling: Float cursors over the three points of each center's sum,
     * which SHC loads with FMOV.S @Rm+ (0x0c912c06). as member sums, the
     * function runs 96 bytes longer, and 108 of its 177 words differ */
    Float *destination;
    const Float *source;
    const Float *offset;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    frames = kaz_a_action.motion->nbFrame;
    if (VARS(work)->frame < 0.0F) {
        VARS(work)->frame += frames;
    }
    if (VARS(work)->frame >= frames) {
        VARS(work)->frame -= frames;
    }
    kaz_a_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "Frame:");
        njPrintD(EDITOR_NUMBER(EDITOR_ROW_SCL_X), (int)njRoundOff(work->scl.x),
                 10);
        return;
    }
    collision_register(work);
    if (!frame_step) {
        return;
    }
    kaz_a_pose(VARS(work)->frame, &first, &second);
    shapes = kaz_a_shapes;
    destination = VECTOR_FLOATS(&work->collision->shapes[0].center);
    source = VECTOR_FLOATS(&first);
    offset = VECTOR_FLOATS(&shapes[0].center);
    *destination++ = *source++ + *offset++;
    *destination++ = *source++ + *offset++;
    *destination = *source + *offset;
    destination = VECTOR_FLOATS(&work->collision->shapes[1].center);
    source = VECTOR_FLOATS(&second);
    offset = VECTOR_FLOATS(&shapes[1].center);
    *destination++ = *source++ + *offset++;
    *destination++ = *source++ + *offset++;
    *destination = *source + *offset;
    destination = VECTOR_FLOATS(&work->collision->shapes[2].center);
    source = VECTOR_FLOATS(&first);
    offset = VECTOR_FLOATS(&shapes[2].center);
    *destination++ = *source++ + *offset++;
    *destination++ = *source++ + *offset++;
    *destination = *source + *offset;
    destination = VECTOR_FLOATS(&work->collision->shapes[3].center);
    source = VECTOR_FLOATS(&second);
    offset = VECTOR_FLOATS(&shapes[3].center);
    /* @hack spelling: the fourth center's advances as statements of their own,
     * offset's as offset = offset + 1. SHC then gives the first center's
     * source, offset and destination R5, R6 and R7 (0x0c912bfc to
     * 0x0c912c0a). advanced in the sums, or with offset++, they take R6, R7
     * and R5, and 36 of its 177 words differ */
    *destination = *source + *offset;
    offset = offset + 1;
    source++;
    destination++;
    *destination = *source + *offset;
    offset = offset + 1;
    source++;
    destination++;
    *destination = *source + *offset;
    VARS(work)->frame += 2.0F * kaz_a_frame_rate;
}

void kaz_a_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            collision_init(task, kaz_a_shapes, COLLISION_OBJECT, 1, 4);
        }
        task->draw = kaz_a_draw;
        task->step = kaz_a_run;
        VARS(work)->frame = work->scl.x;
    }
}
