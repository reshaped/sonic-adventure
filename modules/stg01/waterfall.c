/* O TAKI waterfall whose texture slides down it */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg01/waterfall.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT beach_waterfall_object;

/* the draw slides the first FALL_UVS uvs of each of the model's first
 * FALL_MESHSETS meshsets */
#define FALL_MESHSETS 3
#define FALL_UVS 50

/* the task's range from the camera and player 0, squared for
 * placement_try_leave_by */
#define RANGE (1200.0F * 1200.0F)

/* how far the texture slides down a step, in uv units, with ang.x added. the
 * slide wraps at 256 */
static Sint8 beach_waterfall_speed = 10;

static void beach_waterfall_start(struct task *task);
static void beach_waterfall_destroy(struct task *task);
static void beach_waterfall_run(struct task *task);
static void beach_waterfall_draw(struct task *task);

void beach_waterfall_step(struct task *task)
{
    /* @hack redundant: work, set and never read, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c907428). without it, 12 of 16
     * words differ and the three functions after it move */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        beach_waterfall_start(task);
    }
}

static void beach_waterfall_start(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = beach_waterfall_draw;
    task->step = beach_waterfall_run;
    task->destructor = beach_waterfall_destroy;
    work->timer = 0;
}

static void beach_waterfall_destroy(struct task *task)
{
}

static void beach_waterfall_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_X), "TakiSpeed:");
            njPrintD(NJM_LOCATION(32, EDITOR_ROW_ANG_X),
                     (Uint8)(work->ang.x + beach_waterfall_speed), 3);
        }
        beach_waterfall_draw(task);
        work->timer++;
    }
}

/* draw the model with a copy of each meshset's uvs, slid along v. then give
 * each meshset its own uvs back */
static void beach_waterfall_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &beach_waterfall_object;
    Uint32 slide = (Uint8)(work->timer * (work->ang.x + beach_waterfall_speed));
    NJS_MESHSET *meshset;
    NJS_COLOR uvs[FALL_MESHSETS * FALL_UVS];
    NJS_COLOR *own[FALL_MESHSETS];
    NJS_COLOR *from;
    NJS_COLOR *to;
    int i;
    int j;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F, work->scl.z + 1.0F);
    meshset = object->model->meshsets;
    for (i = 0; i < FALL_MESHSETS; ++i, ++meshset) {
        /* @hack spelling: down, slide copied into an int, which keeps slide in
         * R8 (EXTU.B R8,R8 at 0x0c90752c). the store keeps the same low 16
         * bits either way: as from->tex.v - slide, 30 of 136 words differ */
        int down = slide;

        j = FALL_UVS;
        from = meshset->vertuv;
        to = &uvs[i * FALL_UVS];
        own[i] = from;
        meshset->vertuv = to;
        while (j--) {
            to->tex.u = from->tex.u;
            to->tex.v = from->tex.v - down;
            ++from;
            ++to;
        }
    }
    draw_model(object->model);
    meshset = object->model->meshsets;
    for (i = 0; i < FALL_MESHSETS; ++i, ++meshset) {
        meshset->vertuv = own[i];
    }
    njPopMatrix(1);
}
