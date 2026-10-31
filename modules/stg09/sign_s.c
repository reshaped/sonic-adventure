/* O KBS, a sign whose lamps step through their faces */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/sign.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT sign_s_object;
extern NJS_MESHSET sign_s_meshsets[5];

/* placement_try_leave_by's squared range: about 600 from the camera or
 * player 0 */
#define RANGE 360010.0F

/* the lamps' faces besides their own. each face shifts the UVs of
 * LAMP_MESHSETS meshsets */
#define FACES 3
#define LAMP_MESHSETS 3

/* the face the lamps show: 0 for their own, or 1 to FACES */
#define FACE(work) ((work)->submode)
/* the face changes once work->timer passes FACE_WAIT, after FACE_WAIT + 2
 * steps */
#define FACE_WAIT(work) ((Sint16)(work)->scl.z)

/* face 1's shifts, then face 2's and face 3's, one for each lamp meshset.
 * each face moves the UVs 64 further than the face before */
static struct uv_shift sign_s_lamps[FACES * LAMP_MESHSETS] = {
#include "stg09/sign_s/sign_s_lamps.inc"
};
/* its post's shape, lying along z */
static struct collision_shape sign_s_post = {
    SHAPE_CYLINDER2,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {-1.699997F, 10.0F, 0.41999817F},
    26.8F,
    3.2F,
    0.0F,
    {NJM_DEG_ANG(90), 0, 0},
    0,
    NULL,
};

static void sign_s_destroy(struct task *task);
static void sign_s_draw(struct task *task);
static void sign_s_run(struct task *task);

void sign_s_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    task->draw = sign_s_draw;
    task->step = sign_s_run;
    task->destructor = sign_s_destroy;
    work->timer = 0;
    FACE(work) = 0;
    uv_shift_make(sign_s_lamps, FACES * LAMP_MESHSETS);
    if (!placement_editing(task)) {
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, &sign_s_post, COLLISION_OBJECT, 1, 1);
    }
}

static void sign_s_destroy(struct task *task)
{
    uv_shift_free(sign_s_lamps, FACES * LAMP_MESHSETS);
}

static void sign_s_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (FACE(work) != 0) {
        uv_shift_use(&sign_s_lamps[(FACE(work) - 1) * LAMP_MESHSETS],
                     LAMP_MESHSETS);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&sign_s_object);
    njPopMatrix(1);
    if (FACE(work) != 0) {
        uv_shift_restore(sign_s_lamps, FACES * LAMP_MESHSETS);
    }
}

static void sign_s_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    sign_s_draw(task);
    if (TASK_TIMER(work)++ > FACE_WAIT(work)) {
        ++FACE(work);
        if (FACE(work) > FACES) {
            FACE(work) = 0;
        }
        work->timer = 0;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> UV Pitch");
        return;
    }
    collision_register(work);
}
