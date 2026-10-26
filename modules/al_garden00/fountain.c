/* the fountain: O AR_FUN, four jets of water whose texture runs down them */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_garden00/fountain.h"
#include "al_garden00/fountain_object.h"
#include "al_garden00/textures.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/util.h"

#define RANGE (1200.0F * 1200.0F)

/* the draw takes the jets as JETS meshsets of JET_UVS UVs each */
#define JETS 4
#define JET_UVS 40

/* the jets' v runs down by this plus 8 a step, wrapping at 256. the set
 * editor prints UVSPEED as this plus the placement's x angle, which the draw
 * doesn't add */
static Sint8 fountain_uv_speed = 10;

static void fountain_draw(struct task *task);

void fountain_init(struct task *task)
{
}

void fun_0cb80744(struct task *task)
{
}

void fountain_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_X), "UVSPEED:");
        njPrintD(NJM_LOCATION(32, EDITOR_ROW_ANG_X),
                 (Uint8)(work->ang.x + fountain_uv_speed), 3);
    }
    fountain_draw(task);
    work->timer++;
}

/* draw the jets with their v run down by work->timer. for the draw, the
 * meshsets point at a copy of their UVs on the stack, then at their own
 * again */
static void fountain_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &fountain_object;
    Uint32 scroll = (Uint8)(work->timer * (fountain_uv_speed + 8));
    NJS_MESHSET *meshset;
    NJS_COLOR uvs[JETS * JET_UVS];
    NJS_COLOR *saved[JETS];
    NJS_COLOR *from;
    NJS_COLOR *to;
    int i;
    int j;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    njSetTexture(&garden_objects_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    meshset = object->model->meshsets;
    /* @bug memory: fountain_object's model has one meshset, whose four
     * strips take all JET_UVS of the model's UVs. so this loop and the one
     * after the draw take the model's points as three more meshsets: they
     * read UVs through them and store into them (CMP/GE R13,R6 at
     * 0x0cb808f6, R13 set to JETS at 0x0cb80866, ADD #24,R7 at 0x0cb808fa).
     * nothing runs fountain_step, so neither loop runs */
    for (i = 0; i < JETS; ++i, ++meshset) {
        /* @hack spelling: offset, an int copy of scroll for each jet, which
         * keeps the image's MOV R9,R12 at 0x0cb8087e; without it, 41 of 135
         * words differ */
        int offset = scroll;

        j = JET_UVS;
        from = meshset->vertuv;
        to = &uvs[i * JET_UVS];
        saved[i] = from;
        meshset->vertuv = to;
        while (j--) {
            to->tex.u = from->tex.u;
            to->tex.v = from->tex.v - offset;
            ++from;
            ++to;
        }
    }
    draw_model(object->model);
    meshset = object->model->meshsets;
    for (i = 0; i < JETS; ++i, ++meshset) {
        meshset->vertuv = saved[i];
    }
    njPopMatrix(1);
}
