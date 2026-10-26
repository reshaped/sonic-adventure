/* O CARDKAN: a card sign whose middle and upper parts turn, and whose faces
 * change in phases */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg09/card_sign.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "system/task.h"
#include "system/util.h"

/* the sign's base, whose child is the middle part. the middle part's sibling
 * is the upper part */
extern NJS_OBJECT card_sign_object;
extern NJS_MESHSET card_sign_base_meshset;
extern NJS_MESHSET card_sign_middle_meshset;
extern NJS_MESHSET card_sign_upper_meshset;

/* the task destroys itself beyond about 600 of the camera and player 0.
 * placement_try_leave_by takes the range squared */
#define RANGE ((600.0F * 600.0F) + 10.0F)

#define CARD_SIGN_FACES 3 /* the meshsets whose UVs a phase shifts */
/* the phases after phase 0, which shows the meshsets' own UVs */
#define CARD_SIGN_PHASES 3

/* the upper part's turn about y as the sign starts */
#define UPPER_START NJM_DEG_ANG(-17.5)

struct card_sign_vars {
    Uint16 middle; /* the middle part's turn about y */
    Uint16 upper; /* the upper part's turn about y */
};

#define VARS(work) ((struct card_sign_vars *)&(work)->vars)
/* the phase showing, 0 to CARD_SIGN_PHASES */
#define PHASE(work) ((work)->submode)

/* CARD_SIGN_FACES rows for each phase from 1. each phase shows the texture a
 * quarter further along in u than the phase before */
static struct uv_shift card_sign_faces[CARD_SIGN_PHASES * CARD_SIGN_FACES] = {
#include "stg09/card_sign/card_sign_faces.inc"
};
static struct collision_shape card_sign_shapes[2] = {
    {
        SHAPE_CAPSULE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 14.600007F, 0.0F},
        12.0F,
        10.0F,
        0.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_CYLINDER2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 34.749997F, 0.0F},
        6.6F,
        1.5F,
        0.0F,
        {0, 0, NJM_DEG_ANG(90)},
        0,
        NULL,
    },
};

static void card_sign_destroy(struct task *task);
static void card_sign_draw(struct task *task);
static void card_sign_run(struct task *task);

void card_sign_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        task->draw = card_sign_draw;
        task->step = card_sign_run;
        task->destructor = card_sign_destroy;
        if (!placement_editing(task)) {
            collision_init(task, card_sign_shapes, COLLISION_OBJECT, 1, 2);
        }
        VARS(work)->middle = 0;
        VARS(work)->upper = UPPER_START;
        work->timer = 0;
        PHASE(work) = 0;
        uv_shift_make(card_sign_faces, CARD_SIGN_PHASES * CARD_SIGN_FACES);
    }
}

static void card_sign_destroy(struct task *task)
{
    uv_shift_free(card_sign_faces, CARD_SIGN_PHASES * CARD_SIGN_FACES);
}

static void card_sign_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (PHASE(work) != 0) {
        uv_shift_use(&card_sign_faces[(PHASE(work) - 1) * CARD_SIGN_FACES],
                     CARD_SIGN_FACES);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    object = &card_sign_object;
    draw_model(object->model);
    object = object->child;
    njPushMatrix(NULL);
    TURN_VIA(njRotateY, VARS(work)->middle, ang);
    draw_model(object->model);
    njPopMatrix(1);
    object = object->sibling;
    njPushMatrix(NULL);
    TURN_VIA(njRotateY, VARS(work)->upper, ang);
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
    if (PHASE(work) != 0) {
        uv_shift_restore(card_sign_faces, CARD_SIGN_PHASES * CARD_SIGN_FACES);
    }
}

static void card_sign_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    card_sign_draw(task);
    VARS(work)->upper += (Angle)(work->scl.x * ANGLE_PER_DEGREE);
    VARS(work)->middle += (Angle)(work->scl.y * ANGLE_PER_DEGREE);
    if (TASK_TIMER(work)++ > (Sint16)work->scl.z) {
        ++PHASE(work);
        if (PHASE(work) > CARD_SIGN_PHASES) {
            PHASE(work) = 0;
        }
        work->timer = 0;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> RotHiSpd ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> RotMidSpd ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> UV Pitch");
        return;
    }
    work->collision->shapes[1].ang.x = VARS(work)->upper + NJM_DEG_ANG(90);
    collision_register(work);
}
