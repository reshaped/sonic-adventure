/* the waterfall: O TAKI, whose texture slides down it */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg07/waterfall.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT waterfall_object;

/* the meshsets the slide takes, and the uvs it copies from each */
#define FALL_MESHSETS 3
#define FALL_UVS 88
#ifdef GCC
/* the model's meshsets, and the uvs each one's strips take */
#define MODEL_MESHSETS 2
static const int meshset_uvs[MODEL_MESHSETS] = {88, 90};
#endif

/* the waterfall destroys itself unless the camera or player 0 is within
 * 1200 of it */
#define FALL_RANGE SQUARE(1200.0F)

/* the texture slides down waterfall_speed + SLIDE_BASE in v each step. the
 * set editor shows waterfall_speed plus the set entry's x angle. the slide
 * doesn't read that angle */
#define SLIDE_BASE 8
static Sint8 waterfall_speed = 10;

static void waterfall_start(struct task *task);
static void waterfall_destroy(struct task *task);
static void waterfall_run(struct task *task);
static void waterfall_draw(struct task *task);

void waterfall_step(struct task *task)
{
    /* @hack redundant: work set and never read, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c90b6e8); without it, task is
     * kept at sp+0, the function is 2 bytes longer and 12 of 16 words
     * differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, FALL_RANGE)) {
        waterfall_start(task);
    }
}

static void waterfall_start(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = waterfall_draw;
    task->step = waterfall_run;
    task->destructor = waterfall_destroy;
    work->timer = 0;
}

static void waterfall_destroy(struct task *task)
{
}

static void waterfall_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, FALL_RANGE)) {
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_X), "TakiSpeed:");
            njPrintD(NJM_LOCATION(32, EDITOR_ROW_ANG_X),
                     (Uint8)(work->ang.x + waterfall_speed), 3);
        }
        waterfall_draw(task);
        work->timer++;
    }
}

/* draw the model with each meshset's uvs swapped for a copy, slid down by
 * slide. slide wraps at 256. the meshsets get their own uvs back after */
static void waterfall_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &waterfall_object;
    Uint32 slide = (Uint8)(work->timer * (waterfall_speed + SLIDE_BASE));
    NJS_MESHSET *meshset;
    NJS_COLOR uvs[FALL_MESHSETS * FALL_UVS];
    NJS_COLOR *own[FALL_MESHSETS];
    NJS_COLOR *from;
    NJS_COLOR *to;
    int i;
    int j;
    Angle ang;

    if (placement_try_leave_by(task, FALL_RANGE)) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F, work->scl.z + 1.0F);
    /* @bug memory: the loops take FALL_MESHSETS meshsets, but the
     * model has two (MOV #3,R12 at 0x0c90b864). the model's points follow
     * its meshsets, so the third meshset the loops read is its first two
     * points, and the word where that meshset's uv pointer would be is point
     * 1's z. the copy reads from that z as an address (MOV.L @(20,R13),R4 at
     * 0x0c90b86c). point 1's z then holds a stack address until the draw puts
     * it back (0x0c90b880, 0x0c90b92a) */
    meshset = object->model->meshsets;
#ifdef GCC
    /* repairs the reads the tag names: GCC's loops take the model's two
     * meshsets */
    for (i = 0; i < MODEL_MESHSETS; ++i, ++meshset) {
#else
    for (i = 0; i < FALL_MESHSETS; ++i, ++meshset) {
#endif
        /* @hack spelling: the slide through an int, which SHC copies from R8
         * into R10 each pass for the SUB R10,R0 (0x0c90b872, 0x0c90b8e8).
         * without it, the object's code ends 32 bytes sooner, and 92 of
         * waterfall_draw's 151 words differ */
        int down = slide;

#ifdef GCC
        /* repairs the second meshset's copy: its strips take 90 uvs. the
         * image copies 88, so its draw takes the last two from the third
         * meshset's copy */
        j = meshset_uvs[i];
#else
        j = FALL_UVS;
#endif
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
#ifdef GCC
    /* repairs the second loop the same way */
    for (i = 0; i < MODEL_MESHSETS; ++i, ++meshset) {
#else
    for (i = 0; i < FALL_MESHSETS; ++i, ++meshset) {
#endif
        meshset->vertuv = own[i];
    }
    njPopMatrix(1);
}
