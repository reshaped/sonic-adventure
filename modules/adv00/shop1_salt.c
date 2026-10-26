/* O S1SOLT salt in the first shop, drawn with no collision */
#include <ninja.h>

#include "adv00/shop1_salt.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shop1_salt_object;

enum shop1_salt_mode { SALT_INIT, SALT_SHOWN };

static void shop1_salt_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    texture_set_stage();
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}

static void shop1_salt_destroy(struct task *task)
{
    task_kill(task);
}

/* while the set editor is open, draw the salt and nothing more. the first
 * step with the editor closed starts the salt */
void shop1_salt_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (placement_try_leave(task)) {
        return;
    }
    object = &shop1_salt_object;
    if (edit_mode) {
        if (!MODEL(work)->object) {
            MODEL(work)->object = object;
        }
        shop1_salt_draw(task);
    } else {
        switch (work->mode) {
        case SALT_INIT:
            work->mode = SALT_SHOWN;
            MODEL(work)->object = object;
            task->destructor = shop1_salt_destroy;
            task->draw = shop1_salt_draw;
            break;
        case SALT_SHOWN:
            shop1_salt_draw(task);
            break;
        }
    }
}
