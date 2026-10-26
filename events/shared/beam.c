/* an event's capture beam: a cone of light that spins about its y while it
 * opens or closes */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/tree.h"
#include "shared/beam.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 11
#include "shared/beam/text.inc"

struct beam_vars {
    Uint32 spin; /* the angle it has turned about its y, below a turn */
};

#define VARS(work) ((struct beam_vars *)&(work)->vars)

/* the beam's shape and the speeds of its widths. the beam keeps them in
 * task->motion, the record TASK_MOTION allocates, in place of a struct
 * task_motion */
struct beam_motion {
    Uint8 unk_00[4];
    Float speed_x; /* how far scl.x moves each step it opens or closes */
    Uint8 unk_08[4];
    Float speed_z; /* how far scl.z moves each step it opens or closes */
    Float open_accel; /* added to both speeds each step it opens */
    Float close_accel; /* added to both speeds each step it closes */
    Float spin_speed; /* added to its spin each step it opens or closes */
    Uint8 unk_1c[24];
    Float width_x; /* the scl.x it opens to */
    Float width_z; /* the scl.z it opens to */
    Uint8 unk_3c[4];
};

#define BEAM(task) ((struct beam_motion *)(task)->motion)

static NJS_TEXNAME event_beam_texnames[1] = {{TEXT_1, 0, 0}};

NJS_TEXLIST event_beam_texlist = {
#include "shared/beam/event_beam_texlist.inc"
};

static NJS_MATERIAL event_beam_materials[1] = {
#include "shared/beam/event_beam_materials.inc"
};

static Sint16 event_beam_meshes[88] = {
#include "shared/beam/event_beam_meshes.inc"
};

static NJS_COLOR event_beam_uvs[80] = {
#include "shared/beam/event_beam_uvs.inc"
};

static NJS_MESHSET event_beam_meshsets[1] = {
#include "shared/beam/event_beam_meshsets.inc"
};

static NJS_POINT3 event_beam_points[40] = {
#include "shared/beam/event_beam_points.inc"
};

static NJS_VECTOR event_beam_normals[40] = {
#include "shared/beam/event_beam_normals.inc"
};

static NJS_MODEL event_beam_cone_model = {
#include "shared/beam/event_beam_cone_model.inc"
};

static NJS_OBJECT event_beam_cone = {
#include "shared/beam/event_beam_cone.inc"
};

/* the tree the draw passes to draw_tree: a hidden root without a model,
 * over the cone */
static NJS_OBJECT event_beam_object = {
#include "shared/beam/event_beam_object.inc"
};

static NJS_MDATA2 event_beam_mdata[2] = {
#include "shared/beam/event_beam_mdata.inc"
};

static NJS_MOTION event_beam_motion = {
#include "shared/beam/event_beam_motion.inc"
};

static NJS_ACTION event_beam_action = {
#include "shared/beam/event_beam_action.inc"
};

static void event_beam_destroy(struct task *task)
{
}

static void event_beam_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&event_beam_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (VARS(work)->spin != 0) {
        njRotateY(NULL, VARS(work)->spin);
    }
    /* @hack redundant: the height plus 0, which keeps the image's FLDI0 and
     * FADD (0x0cb81074 and 0x0cb81080 in EV0009); without it, the draw runs
     * 4 bytes short and 24 of 68 words differ */
    njScale(NULL, work->scl.x, work->scl.y + 0.0F, work->scl.z);
    draw_tree(&event_beam_object);
    njPopMatrix(1);
}

static void event_beam_step(struct task *task)
{
    switch (task->work->mode) {
    case BEAM_STILL:
        break;
    case BEAM_OPEN:
        VARS(task->work)->spin += BEAM(task)->spin_speed;
        VARS(task->work)->spin &= 0xffff;
        BEAM(task)->speed_x += BEAM(task)->open_accel;
        BEAM(task)->speed_z += BEAM(task)->open_accel;
        task->work->scl.x += BEAM(task)->speed_x;
        task->work->scl.z += BEAM(task)->speed_z;
        if (task->work->scl.x > BEAM(task)->width_x) {
            task->work->scl.x = BEAM(task)->width_x;
        }
        if (task->work->scl.z > BEAM(task)->width_z) {
            task->work->scl.z = BEAM(task)->width_z;
        }
        break;
    case BEAM_CLOSE:
        VARS(task->work)->spin += BEAM(task)->spin_speed;
        VARS(task->work)->spin &= 0xffff;
        BEAM(task)->speed_x += BEAM(task)->close_accel;
        BEAM(task)->speed_z += BEAM(task)->close_accel;
        task->work->scl.x -= BEAM(task)->speed_x;
        task->work->scl.z -= BEAM(task)->speed_z;
        if (task->work->scl.x < 0.0F) {
            task->work->scl.x = 0.0F;
        }
        if (task->work->scl.z < 0.0F) {
            task->work->scl.z = 0.0F;
        }
        break;
    }
    event_beam_draw(task);
}

static void event_beam_init(struct task *task)
{
    task->work->scl.x = 0.0F;
    task->work->scl.y = 1.0F;
    task->work->scl.z = 0.0F;
    task->work->tick = 0;
    VARS(task->work)->spin = 0;
    BEAM(task)->speed_x = 0.0F;
    BEAM(task)->speed_z = 0.0F;
    BEAM(task)->open_accel = 0.01F;
    BEAM(task)->close_accel = 0.01F;
    BEAM(task)->spin_speed = 768.0F;
    BEAM(task)->width_x = 1.0F;
    BEAM(task)->width_z = 1.0F;
    task->work->mode = BEAM_STILL;
    task->step = event_beam_step;
    task->draw = event_beam_draw;
    task->destructor = event_beam_destroy;
}

struct task *event_beam_create(Float x, Float y, Float z, Angle ang_x,
                               Angle ang_y, Angle ang_z)
{
    struct task *task =
        task_create(TASK_MOTION | TASK_WORK, 5, event_beam_init);

    if (task != NULL) {
        task->work->pos.x = x;
        task->work->pos.y = y;
        task->work->pos.z = z;
        task->work->ang.x = ang_x;
        task->work->ang.y = ang_y;
        task->work->ang.z = ang_z;
        return task;
    }
    return NULL;
}

void event_beam_kill(struct task *task)
{
    if (task != NULL) {
        task_kill(task);
    }
}

void event_beam_set_mode(struct task *task, enum beam_mode mode)
{
    task->work->mode = mode;
    BEAM(task)->speed_x = 0.0F;
    BEAM(task)->speed_z = 0.0F;
}

void event_beam_set_shape(struct task *task, Float width_x, Float height,
                          Float width_z, Float open_accel, Float close_accel,
                          int spin_speed)
{
    if (width_x > 0.0F) {
        BEAM(task)->width_x = width_x;
    }
    if (height > 0.0F) {
        task->work->scl.y = height;
    }
    if (width_z > 0.0F) {
        BEAM(task)->width_z = width_z;
    }
    if (open_accel > 0.0F) {
        BEAM(task)->open_accel = open_accel;
    }
    if (close_accel > 0.0F) {
        BEAM(task)->close_accel = close_accel;
    }
    if (spin_speed > 0) {
        BEAM(task)->spin_speed = (Float)spin_speed;
    }
}
