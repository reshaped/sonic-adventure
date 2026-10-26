/* EV00B7.PRS's spotlights: a lamp whose beam sways from a random phase
 * until the script aims it at a point */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "draw/tree.h"
#include "ev00b7/spotlight.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 9
#include "ev00b7/spotlight/text.inc"

/* in order: SPOTLIGHT_AIM moves on to SPOTLIGHT_HOLD by mode++ */
enum spotlight_mode { SPOTLIGHT_SWAY, SPOTLIGHT_AIM, SPOTLIGHT_HOLD };

struct spotlight_vars {
    Uint8 unk_00[4];
    Sint32 phase; /* the sway's count of steps, from a random 0 to 999 */
};

#define VARS(work) ((struct spotlight_vars *)&(work)->vars)

static NJS_TEXNAME ev00b7_spotlight_textures[5] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0},
};
NJS_TEXLIST ev00b7_spotlight_texlist = {
#include "ev00b7/spotlight/ev00b7_spotlight_texlist.inc"
};
/* the figure: the lamp, with its beam and lens as children */
static NJS_MATERIAL ev00b7_spotlight_lens_materials[1] = {
#include "ev00b7/spotlight/ev00b7_spotlight_lens_materials.inc"
};
static Sint16 ev00b7_spotlight_lens_meshes0[12] = {
#include "ev00b7/spotlight/ev00b7_spotlight_lens_meshes0.inc"
};
static NJS_COLOR ev00b7_spotlight_lens_uvs0[10] = {
#include "ev00b7/spotlight/ev00b7_spotlight_lens_uvs0.inc"
};
static NJS_MESHSET ev00b7_spotlight_lens_meshsets[1] = {
#include "ev00b7/spotlight/ev00b7_spotlight_lens_meshsets.inc"
};
static NJS_POINT3 ev00b7_spotlight_lens_points[7] = {
#include "ev00b7/spotlight/ev00b7_spotlight_lens_points.inc"
};
static NJS_VECTOR ev00b7_spotlight_lens_normals[7] = {
#include "ev00b7/spotlight/ev00b7_spotlight_lens_normals.inc"
};
static NJS_MODEL ev00b7_spotlight_lens_model = {
#include "ev00b7/spotlight/ev00b7_spotlight_lens_model.inc"
};
static NJS_OBJECT ev00b7_spotlight_lens_object = {
#include "ev00b7/spotlight/ev00b7_spotlight_lens_object.inc"
};
static NJS_MATERIAL ev00b7_spotlight_beam_materials[1] = {
#include "ev00b7/spotlight/ev00b7_spotlight_beam_materials.inc"
};
static Sint16 ev00b7_spotlight_beam_meshes0[17] = {
#include "ev00b7/spotlight/ev00b7_spotlight_beam_meshes0.inc"
};
static NJS_COLOR ev00b7_spotlight_beam_uvs0[14] = {
#include "ev00b7/spotlight/ev00b7_spotlight_beam_uvs0.inc"
};
static NJS_MESHSET ev00b7_spotlight_beam_meshsets[1] = {
#include "ev00b7/spotlight/ev00b7_spotlight_beam_meshsets.inc"
};
static NJS_POINT3 ev00b7_spotlight_beam_points[9] = {
#include "ev00b7/spotlight/ev00b7_spotlight_beam_points.inc"
};
static NJS_VECTOR ev00b7_spotlight_beam_normals[9] = {
#include "ev00b7/spotlight/ev00b7_spotlight_beam_normals.inc"
};
static NJS_MODEL ev00b7_spotlight_beam_model = {
#include "ev00b7/spotlight/ev00b7_spotlight_beam_model.inc"
};
static NJS_OBJECT ev00b7_spotlight_beam_object = {
#include "ev00b7/spotlight/ev00b7_spotlight_beam_object.inc"
};
static NJS_MATERIAL ev00b7_spotlight_materials[3] = {
#include "ev00b7/spotlight/ev00b7_spotlight_materials.inc"
};
static Sint16 ev00b7_spotlight_meshes0[12] = {
#include "ev00b7/spotlight/ev00b7_spotlight_meshes0.inc"
};
static Sint16 ev00b7_spotlight_meshes1[24] = {
#include "ev00b7/spotlight/ev00b7_spotlight_meshes1.inc"
};
static Sint16 ev00b7_spotlight_meshes2[16] = {
#include "ev00b7/spotlight/ev00b7_spotlight_meshes2.inc"
};
static Sint16 ev00b7_spotlight_meshes3[24] = {
#include "ev00b7/spotlight/ev00b7_spotlight_meshes3.inc"
};
static NJS_COLOR ev00b7_spotlight_uvs0[10] = {
#include "ev00b7/spotlight/ev00b7_spotlight_uvs0.inc"
};
static NJS_COLOR ev00b7_spotlight_uvs1[24] = {
#include "ev00b7/spotlight/ev00b7_spotlight_uvs1.inc"
};
static NJS_COLOR ev00b7_spotlight_uvs2[12] = {
#include "ev00b7/spotlight/ev00b7_spotlight_uvs2.inc"
};
static NJS_COLOR ev00b7_spotlight_uvs3[24] = {
#include "ev00b7/spotlight/ev00b7_spotlight_uvs3.inc"
};
static NJS_MESHSET ev00b7_spotlight_meshsets[4] = {
#include "ev00b7/spotlight/ev00b7_spotlight_meshsets.inc"
};
static NJS_POINT3 ev00b7_spotlight_points[19] = {
#include "ev00b7/spotlight/ev00b7_spotlight_points.inc"
};
static NJS_VECTOR ev00b7_spotlight_normals[19] = {
#include "ev00b7/spotlight/ev00b7_spotlight_normals.inc"
};
static NJS_MODEL ev00b7_spotlight_model = {
#include "ev00b7/spotlight/ev00b7_spotlight_model.inc"
};
static NJS_OBJECT ev00b7_spotlight_object = {
#include "ev00b7/spotlight/ev00b7_spotlight_object.inc"
};

static void ev00b7_spotlight_destroy(struct task *task)
{
}

static void ev00b7_spotlight_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&ev00b7_spotlight_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y + 0x8000, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    njScale(NULL, 0.5F, 1.0F, 0.5F);
    draw_tree(&ev00b7_spotlight_object);
    njPopMatrix(1);
}

static void ev00b7_spotlight_step(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float y;
    Float z;

    switch (work->mode) {
    case SPOTLIGHT_SWAY:
        /* the beam tilts round an ellipse, up to 4608 about x and 4096
         * about z. each step moves it on by njSqrt(1088888.9), about 1043.5
         * of the 65536 a lap takes, so a lap lasts about 63 steps. its
         * heading turns by 1 a step */
        work->ang.x =
            (Angle)(njCos((Angle)(njSqrt(1088888.9F) * VARS(work)->phase)) *
                    4608.0F);
        work->ang.y = VARS(work)->phase;
        work->ang.z =
            (Angle)(njSin((Angle)(njSqrt(1088888.9F) * VARS(work)->phase)) *
                    4096.0F);
        VARS(work)->phase++;
        break;
    case SPOTLIGHT_AIM:
        x = work->scl.x - work->pos.x;
        y = work->scl.y - work->pos.y;
        z = work->scl.z - work->pos.z;
        work->ang.x = 0;
        work->ang.y = -(Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
        work->ang.z =
            (Angle)(atan2f(y, njSqrt((z * z) + (x * x))) * ANGLE_PER_RADIAN);
        work->mode++;
        break;
    case SPOTLIGHT_HOLD:
        break;
    }
    ev00b7_spotlight_draw(task);
}

static void ev00b7_spotlight_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = SPOTLIGHT_SWAY;
    work->pos = work->scl;
    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    VARS(work)->phase = rand() * (1000 * RAND_SCALE);
    task->step = ev00b7_spotlight_step;
    task->draw = ev00b7_spotlight_draw;
    task->destructor = ev00b7_spotlight_destroy;
}

struct task *ev00b7_spotlight_create(Float x, Float y, Float z)
{
    struct task *task = task_create(TASK_WORK, 5, ev00b7_spotlight_init);
    struct task_work *work;

    if (task != NULL) {
        work = task->work;
        work->scl.x = x;
        work->scl.y = y;
        work->scl.z = z;
    }
    return task;
}

void ev00b7_spotlight_aim(struct task *task, Float x, Float y, Float z)
{
    struct task_work *work = task->work;

    work->scl.x = x;
    work->scl.y = y;
    work->scl.z = z;
    work->mode = SPOTLIGHT_AIM;
}

void ev00b7_spotlight_kill(struct task *task)
{
    if (task != NULL) {
        task_kill(task);
        /* @hack redundant: the parameter cleared, which nothing reads, and
         * which keeps task_kill a call that saves PR (STS.L PR,@-R15 at
         * 0x0cb832fe); without it, task_kill becomes a tail jump, the code
         * runs 4 bytes short and 9 of the function's 9 words differ */
        task = NULL;
    }
}
