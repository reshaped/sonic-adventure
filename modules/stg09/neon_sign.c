/* O M1FNEO neon sign, whose tubes step through their faces */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/neon_sign.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT neon_sign_object;
extern NJS_MESHSET neon_sign_meshsets[];

/* destroyed beyond about 600 units of the camera and player 0 */
#define RANGE ((600.0F * 600.0F) + 10.0F)

/* the faces the tubes show past their own. each face shifts the UVs of the
 * TUBES meshsets */
#define FACES 3
#define TUBES 3

#define FACE(work) ((work)->submode) /* 0 for its own, then 1 to FACES */
/* each face shows for FACE_WAIT + 2 steps */
#define FACE_WAIT(work) ((Sint16)(work)->scl.z)

static struct uv_shift neon_sign_tubes[FACES * TUBES] = {
#include "stg09/neon_sign/neon_sign_tubes.inc"
};

static void neon_sign_destroy(struct task *task);
static void neon_sign_draw(struct task *task);
static void neon_sign_run(struct task *task);

void neon_sign_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        task->draw = neon_sign_draw;
        task->step = neon_sign_run;
        task->destructor = neon_sign_destroy;
        work->timer = 0;
        FACE(work) = 0;
        uv_shift_make(neon_sign_tubes, FACES * TUBES);
        /* @hack redundant: an empty if on placement_editing's result, which
         * leaves the image's TST R0,R0 after the call (0x0c923996). as a
         * call alone, 8 of neon_sign_step's 32 words differ */
        if (placement_editing(task)) {
        }
    }
}

static void neon_sign_destroy(struct task *task)
{
    uv_shift_free(neon_sign_tubes, FACES * TUBES);
}

static void neon_sign_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (FACE(work)) {
        uv_shift_use(&neon_sign_tubes[(FACE(work) - 1) * TUBES], TUBES);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&neon_sign_object);
    njPopMatrix(1);
    /* @hack spelling: != 0 written out, which gives uv_shift_restore's
     * address R2 for the tail call (0x0c923a74). as a bare test, it takes
     * R3, and 2 of neon_sign_draw's 65 words differ */
    if (FACE(work) != 0) {
        uv_shift_restore(neon_sign_tubes, FACES * TUBES);
    }
}

static void neon_sign_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    neon_sign_draw(task);
    if (TASK_TIMER(work)++ > FACE_WAIT(work)) {
        FACE(work)++;
        if (FACE(work) > FACES) {
            FACE(work) = 0;
        }
        work->timer = 0;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> UV Pitch");
    }
}
