/* o_komokosu, a cobweb in Red Mountain's caves, scaled by its set entry */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg05/cobweb.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT cobweb_object;

/* the step sets object and texlist, but the draw takes cobweb_object and
 * obj_mountain_texlist itself */
struct cobweb_vars {
    Uint8 unk_00[4];
    NJS_OBJECT *object;
    NJS_TEXLIST *texlist;
};

#define VARS(work) ((struct cobweb_vars *)&(work)->vars)

enum cobweb_mode { COBWEB_INIT, COBWEB_SHOWN };

static void cobweb_draw(struct task *task);
static void cobweb_destroy(struct task *task);

void cobweb_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        VARS(work)->object = &cobweb_object;
        VARS(work)->texlist = &obj_mountain_texlist;
    } else {
        switch (work->mode) {
        case COBWEB_INIT:
            work->mode = COBWEB_SHOWN;
            VARS(work)->object = &cobweb_object;
            VARS(work)->texlist = &obj_mountain_texlist;
            task->draw = cobweb_draw;
            task->destructor = cobweb_destroy;
            break;
        case COBWEB_SHOWN:
            cobweb_draw(task);
            break;
        }
    }
}

static void cobweb_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &cobweb_object;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 100.0F)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
        draw_model(object->model);
        njPopMatrix(1);
    }
}

static void cobweb_destroy(struct task *task)
{
}
