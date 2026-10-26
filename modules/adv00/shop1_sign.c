/* O S1KNBN sign of the first shop, with a day face by day and a night face in
 * the evening and at night */
#include <ninja.h>

#include "adv00/shop1_sign.h"
#include "adv00/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "object/placement.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT shop1_sign_object;

/* the face's material, and its day and night textures, s1kan_d and
 * s1kan_n */
#define SIGN_FACE 2
#define SIGN_DAY 2
#define SIGN_NIGHT 3

enum shop1_sign_mode { SIGN_INIT, SIGN_SHOWN };

/* clang-format off */
static struct collision_shape shop1_sign_shape = {
    SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {6.57F, -6.25F, 0.0F},
    6.57F, 4.0F, 1.02F, {0}, 0, NULL,
};
/* clang-format on */

static void shop1_sign_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSetTexture(MODEL(work)->texlist);
    njSimpleDrawObject(MODEL(work)->object);
    njPopMatrix(1);
}

static void shop1_sign_destroy(struct task *task)
{
    task_kill(task);
}

/* while the set editor is open, draw the sign and nothing more. the first
 * step with the editor closed gives the face its texture by the time of day,
 * and the sign its collision */
void shop1_sign_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_TEXLIST *texlist;

    if (placement_try_leave(task)) {
        return;
    }
    texlist = &ss_kanban_texlist;
    object = &shop1_sign_object;
    if (edit_mode) {
        if (!MODEL(work)->object) {
            MODEL(work)->object = object;
        }
        if (!MODEL(work)->texlist) {
            MODEL(work)->texlist = texlist;
        }
        shop1_sign_draw(task);
    } else {
        switch (work->mode) {
        case SIGN_INIT:
            work->mode = SIGN_SHOWN;
            MODEL(work)->object = object;
            MODEL(work)->texlist = texlist;
            if (story_get_time() == TIME_DAY) {
                MODEL(work)->object->model->mats[SIGN_FACE].attr_texId =
                    SIGN_DAY;
            } else {
                MODEL(work)->object->model->mats[SIGN_FACE].attr_texId =
                    SIGN_NIGHT;
            }
            collision_init(task, &shop1_sign_shape, COLLISION_OBJECT, 1, 1);
            task->destructor = shop1_sign_destroy;
            task->draw = shop1_sign_draw;
            break;
        case SIGN_SHOWN:
            collision_register(work);
            shop1_sign_draw(task);
            break;
        }
    }
}
