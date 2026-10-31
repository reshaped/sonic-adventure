/* O SLOTKAN, a slot sign with spinning parts and changing faces */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/slot_sign.h"
#include "system/task.h"
#include "system/util.h"

/* the base. its child is the middle part, and that child's sibling the upper
 * part */
extern NJS_OBJECT slot_sign_object;
extern NJS_MESHSET slot_sign_base_meshsets[6];
extern NJS_MESHSET slot_sign_middle_meshsets[2];
extern NJS_MESHSET slot_sign_upper_meshsets[4];

/* placement_try_leave_by's squared range: about 600 from the camera or
 * player 0 */
#define RANGE 360010.0F

/* the faces besides the meshsets' own. each face shifts the UVs of
 * FACE_MESHSETS meshsets: the base's, the upper part's and the middle
 * part's */
#define SLOT_SIGN_FACES 3
#define FACE_MESHSETS 3

struct slot_sign_vars {
    Uint16 middle; /* the middle part's turn about y */
    Uint16 upper; /* the upper part's turn about y */
};

/* the upper part's turn as the sign starts */
#define UPPER_START 0xf38f

#define VARS(work) ((struct slot_sign_vars *)&(work)->vars)
/* the face showing: 0 for the meshsets' own UVs, or 1 to SLOT_SIGN_FACES */
#define FACE(work) ((work)->submode)
/* the face changes once work->timer passes FACE_WAIT, after FACE_WAIT + 2
 * steps */
#define FACE_WAIT(work) ((Sint16)(work)->scl.z)

/* face 1's shifts, then face 2's and face 3's, one for each face meshset.
 * each face moves the UVs 64 further along in u than the face before */
static struct uv_shift slot_sign_faces[SLOT_SIGN_FACES * FACE_MESHSETS] = {
#include "stg09/slot_sign/slot_sign_faces.inc"
};
/* the base's shape, then the middle and upper parts', which the run turns
 * with the parts */
static struct collision_shape slot_sign_shapes[3] = {
    {
        SHAPE_CAPSULE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 5.15F, 0.95000005F},
        8.0F,
        1.0F,
        0.0F,
        {0, 0, NJM_DEG_ANG(90)},
        0,
        NULL,
    },
    {
        SHAPE_CYLINDER2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 21.18F, 0.0F},
        10.0F,
        1.5F,
        0.0F,
        {0, 0, NJM_DEG_ANG(90)},
        0,
        NULL,
    },
    {
        SHAPE_CAPSULE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 35.7F, 0.0F},
        4.0F,
        3.5F,
        0.0F,
        {0, 0, NJM_DEG_ANG(90)},
        0,
        NULL,
    },
};

static void slot_sign_destroy(struct task *task);
static void slot_sign_draw(struct task *task);
static void slot_sign_run(struct task *task);

void slot_sign_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        task->draw = slot_sign_draw;
        task->step = slot_sign_run;
        task->destructor = slot_sign_destroy;
        if (!placement_editing(task)) {
            collision_init(task, slot_sign_shapes, COLLISION_OBJECT, 1, 3);
        }
        VARS(work)->middle = 0;
        VARS(work)->upper = UPPER_START;
        work->timer = 0;
        FACE(work) = 0;
        work->ang.x = 0;
        work->ang.z = 0;
        uv_shift_make(slot_sign_faces, SLOT_SIGN_FACES * FACE_MESHSETS);
    }
}

static void slot_sign_destroy(struct task *task)
{
    uv_shift_free(slot_sign_faces, SLOT_SIGN_FACES * FACE_MESHSETS);
}

static void slot_sign_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 40.0F)) {
        return;
    }
    if (FACE(work) != 0) {
        uv_shift_use(&slot_sign_faces[(FACE(work) - 1) * FACE_MESHSETS],
                     FACE_MESHSETS);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    object = &slot_sign_object;
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
    if (FACE(work) != 0) {
        uv_shift_restore(slot_sign_faces, SLOT_SIGN_FACES * FACE_MESHSETS);
    }
}

static void slot_sign_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    slot_sign_draw(task);
    VARS(work)->upper += (Angle)(work->scl.x * ANGLE_PER_DEGREE);
    VARS(work)->middle += (Angle)(work->scl.y * ANGLE_PER_DEGREE);
    if (TASK_TIMER(work)++ > FACE_WAIT(work)) {
        ++FACE(work);
        if (FACE(work) > SLOT_SIGN_FACES) {
            FACE(work) = 0;
        }
        work->timer = 0;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> RotHiSpd ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> RotMidSpd ");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> UV Pitch");
        return;
    }
    work->collision->shapes[1].ang.x = VARS(work)->middle + NJM_DEG_ANG(90);
    work->collision->shapes[2].ang.x = VARS(work)->upper;
    collision_register(work);
}
