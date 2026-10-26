/* O ENTOTSU, a chimney that puffs smoke from its mouth every fourth step */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "stg12/chimney.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT chimney_object;
extern NJS_OBJECT chimney_mouth_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

#define PUFF_SCALE 5.0F

enum chimney_mode { CHIMNEY_INIT, CHIMNEY_RUN };

struct chimney_vars {
    Uint8 steps;
};

#define VARS(work) ((struct chimney_vars *)&(work)->vars)

static void chimney_init(struct task *task);
static void chimney_draw(struct task *task);
static void chimney_destroy(struct task *task);
static void chimney_puff(struct task_work *work);

void chimney_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case CHIMNEY_INIT:
            chimney_init(task);
            break;
        case CHIMNEY_RUN:
            chimney_draw(task);
            chimney_puff(work);
            chimney_destroy(task);
            break;
        /* @hack redundant: an empty default, which leaves a NOP in the slot of
         * CHIMNEY_RUN's BRA past the switch, at 0x0c90069c. without it, the
         * copy of task into R4 fills the slot, and 40 of 65 words differ */
        default:;
        }
    } else {
        switch (work->mode) {
        case CHIMNEY_INIT:
            chimney_init(task);
            break;
        case CHIMNEY_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            chimney_draw(task);
            chimney_destroy(task);
            break;
        }
    }
}

static void chimney_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = chimney_draw;
    task->destructor = chimney_destroy;
    VARS(work)->steps = 0;
    work->mode = CHIMNEY_RUN;
}

static void chimney_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&chimney_object);
        njPopMatrix(1);
    }
}

static void chimney_destroy(struct task *task)
{
    /* @hack redundant: a read of the work, which keeps the task passed in, MOV
     * R14,R4 at 0x0c90069a and 0x0c9006d8, where SHC expands this in
     * chimney_step; without it, 41 of 65 words differ */
    (void)task->work;
}

/* a puff's speed before the chimney's turn. the vector is defined after
 * chimney_step because SHC places constants in source order. the image puts the
 * step's label strings before it */
static const NJS_VECTOR chimney_puff_speed = {0.0F, 0.0F, -3.0F};

static void chimney_puff(struct task_work *work)
{
    NJS_VECTOR speed;
    NJS_POINT3 pos;
    Angle ang;

    speed = chimney_puff_speed;
    if (!(VARS(work)->steps & 3)) {
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njCalcPoint(NULL, (NJS_POINT3 *)chimney_mouth_object.pos, &pos);
        njUnitMatrix(NULL);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njCalcVector(NULL, &speed, &speed);
        njPopMatrix(1);
        dust_create(&pos, &speed, PUFF_SCALE);
    }
    VARS(work)->steps++;
}
