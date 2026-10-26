/* O PLANET 1 to 3 and O METEORITE 1 to 3, models that spin about y */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "stg03/planets.h"
#include "stg03/shareobj_textures.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT meteorite_1_object;
extern NJS_OBJECT meteorite_2_object;
extern NJS_OBJECT meteorite_3_object;
extern NJS_OBJECT planet_1_object;
extern NJS_OBJECT planet_2_object;
extern NJS_OBJECT planet_3_object;

/* each object's range, squared, as placement_try_leave_by takes it */
#define PLANET_RANGE (3010.0F * 3010.0F)

#define PLANET_KIND(work) ((work)->tick) /* its kind, of enum planet_kind */
/* the object drawn */
#define PLANET_OBJECT(work) (*(NJS_OBJECT **)&(work)->vars)
/* the turn about y a step, from scl.x in degrees */
#define PLANET_SPIN(work) TASK_TIMER(work)

enum planet_mode { PLANET_INIT, PLANET_SPINNING };

/* the kinds, each an index in planet_objects */
enum planet_kind {
    METEORITE_1,
    METEORITE_2,
    METEORITE_3,
    PLANET_1,
    PLANET_2,
    PLANET_3
};

static NJS_OBJECT *planet_objects[] = {
    &meteorite_1_object, &meteorite_2_object, &meteorite_3_object,
    &planet_1_object,    &planet_2_object,    &planet_3_object,
};

static void planet_destroy(struct task *task)
{
    task_kill(task);
}

static void planet_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&obj_shareobj_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_set_palette(DRAW_PLAYER_PALETTE);
        draw_tree(PLANET_OBJECT(work));
        draw_set_palette(0);
        njPopMatrix(1);
    }
}

/* the step every kind shares, once its kind is set */
static void planet_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 kind = PLANET_KIND(work);
    /* @hack spelling: the table read through object, a pointer to its entry,
     * which adds the entry's offset to the table's address and loads through
     * the sum (ADD R5,R4 at 0x0c9146f8); read by index, it loads with
     * @(R0,R5) and 11 of 99 words differ */
    NJS_OBJECT **object;

    if (placement_try_leave_by(task, PLANET_RANGE)) {
        return;
    }
    if (stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 0) ||
        IN_STAGE(STAGE_CART)) {
        /* in the set editor, the turn comes from scl.x each step, so a new
         * scale shows at once */
        if (edit_mode != 0) {
            object = &planet_objects[kind];
            PLANET_OBJECT(work) = *object;
            work->ang.y += (Angle)(work->scl.x * ANGLE_PER_DEGREE);
            planet_draw(task);
        } else {
            switch (work->mode) {
            case PLANET_INIT:
                work->mode = PLANET_SPINNING;
                PLANET_SPIN(work) = (Angle)(work->scl.x * ANGLE_PER_DEGREE);
                object = &planet_objects[kind];
                PLANET_OBJECT(work) = *object;
                task->destructor = planet_destroy;
                task->draw = planet_draw;
                break;
            case PLANET_SPINNING:
                work->ang.y += PLANET_SPIN(work);
                planet_draw(task);
                break;
            }
        }
    }
}

void planet_1_step(struct task *task)
{
    struct task_work *work = task->work;

    PLANET_KIND(work) = PLANET_1;
    planet_step(task);
}

void planet_2_step(struct task *task)
{
    struct task_work *work = task->work;

    PLANET_KIND(work) = PLANET_2;
    planet_step(task);
}

void planet_3_step(struct task *task)
{
    struct task_work *work = task->work;

    PLANET_KIND(work) = PLANET_3;
    planet_step(task);
}

void meteorite_1_step(struct task *task)
{
    struct task_work *work = task->work;

    PLANET_KIND(work) = METEORITE_1;
    planet_step(task);
}

void meteorite_2_step(struct task *task)
{
    struct task_work *work = task->work;

    PLANET_KIND(work) = METEORITE_2;
    planet_step(task);
}

void meteorite_3_step(struct task *task)
{
    struct task_work *work = task->work;

    PLANET_KIND(work) = METEORITE_3;
    planet_step(task);
}
