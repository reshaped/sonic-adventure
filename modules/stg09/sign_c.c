/* O KBC, a sign of card suits that runs its motion while its lamps step
 * through their faces */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/sign.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_ACTION sign_c_action;
extern NJS_MESHSET sign_c_meshsets[];

/* placement_try_leave_by's squared range: about 600 from the camera or
 * player 0 */
#define RANGE 360010.0F

/* the lamps' faces besides their own. each face shifts the UVs of
 * LAMP_MESHSETS meshsets */
#define FACES 3
#define LAMP_MESHSETS 2

struct sign_c_vars {
    Uint8 unk_00[8];
    Float frame; /* the frame of sign_c_action's motion */
};

#define VARS(work) ((struct sign_c_vars *)&(work)->vars)
/* the face the lamps show: 0 for their own, or 1 to FACES */
#define FACE(work) ((work)->submode)
/* scl.y, whose magnitude is the motion's frames a step */
#define FRAME_SPEED(work) ((work)->scl.y)
/* the face changes once work->timer passes FACE_WAIT, after FACE_WAIT + 2
 * steps */
#define FACE_WAIT(work) ((Sint16)(work)->scl.z)

/* face 1's shifts, then face 2's and face 3's, one for each lamp meshset.
 * each face moves the UVs 64 further than the face before */
static struct uv_shift sign_c_lamps[FACES * LAMP_MESHSETS] = {
#include "stg09/sign_c/sign_c_lamps.inc"
};
/* its two posts' shapes, each lying along z */
static struct collision_shape sign_c_posts[2] = {
    {
        SHAPE_CYLINDER2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {16.360001F, 15.499993F, -1.5F},
        25.0F,
        3.7F,
        0.0F,
        {NJM_DEG_ANG(90), 0, 0},
        0,
        NULL,
    },
    {
        SHAPE_CYLINDER2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {-17.0F, 15.499993F, -1.1600037F},
        25.0F,
        3.7F,
        0.0F,
        {NJM_DEG_ANG(90), 0, 0},
        0,
        NULL,
    },
};

static void sign_c_destroy(struct task *task);
static void sign_c_draw(struct task *task);
static void sign_c_run(struct task *task);

void sign_c_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    task->draw = sign_c_draw;
    task->step = sign_c_run;
    task->destructor = sign_c_destroy;
    work->timer = 0;
    VARS(work)->frame = 0.0F;
    FACE(work) = 0;
    uv_shift_make(sign_c_lamps, FACES * LAMP_MESHSETS);
    if (!placement_editing(task)) {
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, sign_c_posts, COLLISION_OBJECT, 1, 2);
    }
}

static void sign_c_destroy(struct task *task)
{
    uv_shift_free(sign_c_lamps, FACES * LAMP_MESHSETS);
}

static void sign_c_draw(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: room for 150 UVs nothing uses, which makes the frame 604
     * bytes (MOV.W and ADD R0,R15 at 0x0c923524). without it, the code is 64
     * bytes short and 80 of 82 words differ */
    NJS_TEX room[150];
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 40.0F)) {
        return;
    }
    if (FACE(work) != 0) {
        uv_shift_use(&sign_c_lamps[(FACE(work) - 1) * LAMP_MESHSETS],
                     LAMP_MESHSETS);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(&sign_c_action, VARS(work)->frame);
    njPopMatrix(1);
    if (FACE(work) != 0) {
        uv_shift_restore(sign_c_lamps, FACES * LAMP_MESHSETS);
    }
}

static void sign_c_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    sign_c_draw(task);
    VARS(work)->frame += _builtin_fabsf(FRAME_SPEED(work));
    if (VARS(work)->frame >= sign_c_action.motion->nbFrame) {
        VARS(work)->frame = 0.0F;
    }
    if (TASK_TIMER(work)++ > FACE_WAIT(work)) {
        FACE(work)++;
        if (FACE(work) > FACES) {
            FACE(work) = 0;
        }
        work->timer = 0;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> Frame Pitch");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> UV Pitch");
        return;
    }
    collision_register(work);
}
