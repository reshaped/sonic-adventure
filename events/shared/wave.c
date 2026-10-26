/* an event's waves: rings on the water that spread from a point as their
 * motion plays. an emitter starts one every few steps */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "shared/wave.h"
#include "system/task.h"

#define TEXT_1_LENGTH 6
#include "shared/wave/text.inc"

/* the work modes of a wave and of an emitter. WAVE_START lasts one step,
 * and WAVE_END kills the task */
enum wave_mode { WAVE_START, WAVE_RUN, WAVE_END };

struct wave_params {
    Float speed; /* the motion's frames it plays each step */
    Float scale; /* its scale in x and z */
};

struct wave_emitter_params {
    /* each wave's speed and scale */
    Float speed;
    Float scale;
    Uint32 interval; /* steps between waves */
    Uint32 steps; /* the emitter ends once its age passes this */
};

struct wave_vars {
    Float frame;
    Uint8 unk_04[4];
    struct wave_params *params;
};

struct wave_emitter_vars {
    Uint32 age; /* the steps it has run in WAVE_RUN */
    Uint8 unk_04[4];
    struct wave_emitter_params *params;
};

#define WAVE(work) ((struct wave_vars *)&(work)->vars)
#define EMITTER(work) ((struct wave_emitter_vars *)&(work)->vars)

static NJS_MATERIAL event_wave_materials[1] = {
#include "shared/wave/event_wave_materials.inc"
};
static Sint16 event_wave_meshes0[90] = {
#include "shared/wave/event_wave_meshes0.inc"
};
static NJS_COLOR event_wave_uvs0[72] = {
#include "shared/wave/event_wave_uvs0.inc"
};
static NJS_MESHSET event_wave_meshsets[1] = {
#include "shared/wave/event_wave_meshsets.inc"
};
static NJS_POINT3 event_wave_points[36] = {
#include "shared/wave/event_wave_points.inc"
};
static NJS_VECTOR event_wave_normals[36] = {
#include "shared/wave/event_wave_normals.inc"
};
static NJS_MODEL event_wave_model = {
#include "shared/wave/event_wave_model.inc"
};
/* the ring */
static NJS_OBJECT event_wave_object = {
#include "shared/wave/event_wave_object.inc"
};
static NJS_MDATA2 event_wave_motion_mdata[1] = {
#include "shared/wave/event_wave_motion_mdata.inc"
};
/* a motion with no keys, which holds the ring still */
static NJS_MOTION event_wave_motion = {
#include "shared/wave/event_wave_motion.inc"
};
/* @hack type: an array of one. SHC then pools its address and reads the motion
 * 4 bytes past it (MOV.L @(4,R3),R2 at 0x0cb81f3e in EV0020). as one
 * NJS_ACTION, SHC pools the motion field's own address, and 3 of 50 words
 * differ */
static NJS_ACTION event_wave_action[1] = {
#include "shared/wave/event_wave_action.inc"
};
static NJS_POINT3 event_wave_shape_points0[36] = {
#include "shared/wave/event_wave_shape_points0.inc"
};
static NJS_POINT3 event_wave_shape_points1[36] = {
#include "shared/wave/event_wave_shape_points1.inc"
};
static NJS_POINT3 event_wave_shape_points2[36] = {
#include "shared/wave/event_wave_shape_points2.inc"
};
static NJS_MKEY_P event_wave_shape_points[3] = {
#include "shared/wave/event_wave_shape_points.inc"
};
static NJS_MDATA2 event_wave_shape_mdata[1] = {
#include "shared/wave/event_wave_shape_mdata.inc"
};
/* three keys of the ring's points, which spread it */
static NJS_MOTION event_wave_shape = {
#include "shared/wave/event_wave_shape.inc"
};
static NJS_TEXNAME event_wave_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST event_wave_texlist = {
#include "shared/wave/event_wave_texlist.inc"
};

/* each slot's emitter task, or null. an emitter that ends doesn't clear its
 * slot */
static struct task *event_wave_emitters[EVENT_WAVE_EMITTERS] = {NULL};

static void event_wave_step(struct task *task);
static void event_wave_draw(struct task *task);
static void event_wave_destroy(struct task *task);
static void event_wave_init(struct task *task);
static void event_wave_emitter_init(struct task *task);

void event_wave_create(Float x, Float y, Float z, Float speed, Float scale)
{
    struct task *task = task_create(TASK_WORK, 5, event_wave_init);
    struct wave_params *params = heap_alloc(sizeof(*params));
    struct task_work *work = task->work;

    WAVE(work)->params = params;
    work->mode = WAVE_START;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    WAVE(work)->frame = 0.0F;
    params->speed = speed;
    params->scale = scale;
}

static void event_wave_init(struct task *task)
{
    task->step = event_wave_step;
    task->draw = event_wave_draw;
    task->destructor = event_wave_destroy;
}

static void event_wave_step(struct task *task)
{
    struct task_work *work = task->work;
    struct wave_params *params = WAVE(work)->params;

    switch (work->mode) {
    case WAVE_START:
        work->mode = WAVE_RUN;
        break;
    case WAVE_RUN:
        WAVE(work)->frame += params->speed;
        if (WAVE(work)->frame >= event_wave_action->motion->nbFrame - 1.0F) {
            work->mode = WAVE_END;
        }
        break;
    case WAVE_END:
        if (task != NULL) {
            task_kill(task);
        }
        break;
    }
    event_wave_draw(task);
}

static void event_wave_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct wave_params *params = WAVE(work)->params;

    njSetTexture(&event_wave_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScale(NULL, params->scale, 1.0F, params->scale);
    draw_shape_motion(&event_wave_object, &event_wave_motion, &event_wave_shape,
                      WAVE(work)->frame);
    njPopMatrix(1);
}

static void event_wave_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct wave_params *params = WAVE(work)->params;

    if (params != NULL) {
        heap_free(params);
    }
}

void event_wave_load_textures(void)
{
    njLoadTexture(&event_wave_texlist);
}

void event_wave_release_textures(void)
{
    njReleaseTexture(&event_wave_texlist);
}

void event_wave_emitter_create(Float x, Float y, Float z, Float speed,
                               Float scale, int interval, int steps, int slot)
{
    struct wave_emitter_params *params;
    struct task_work *work;

    event_wave_emitters[slot] =
        task_create(TASK_WORK, 5, event_wave_emitter_init);
    params = heap_alloc(sizeof(*params));
    work = event_wave_emitters[slot]->work;
    EMITTER(work)->params = params;
    work->mode = WAVE_START;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    EMITTER(work)->age = 0;
    params->speed = speed;
    params->scale = scale;
    params->interval = interval;
    params->steps = steps;
}

void event_wave_emitter_kill(int slot)
{
    if (event_wave_emitters[slot] != NULL) {
        /* @bug memory: an emitter that ends leaves its slot set, as its
         * step kills its task and nothing clears the slot (JMP @R3 at
         * 0x0cb85984 in EV00F7). killing the slot then stores task_destroy
         * into the step of a task back in the task pool, which may hold
         * another object by now (JSR @R2 at 0x0cb8590a). EV00F7's script
         * starts an emitter of 18 steps in slot 1, and kills that slot 200
         * steps later */
        task_kill(event_wave_emitters[slot]);
        event_wave_emitters[slot] = NULL;
    }
}

static void event_wave_emitter_step(struct task *task);
static void event_wave_emitter_draw(struct task *task);
static void event_wave_emitter_destroy(struct task *task);

static void event_wave_emitter_init(struct task *task)
{
    task->step = event_wave_emitter_step;
    task->draw = event_wave_emitter_draw;
    task->destructor = event_wave_emitter_destroy;
}

static void event_wave_emitter_step(struct task *task)
{
    struct task_work *work = task->work;
    struct wave_emitter_params *params = EMITTER(work)->params;

    switch (work->mode) {
    case WAVE_START:
        work->mode = WAVE_RUN;
        break;
    case WAVE_RUN:
        ++EMITTER(work)->age;
        if (EMITTER(work)->age % params->interval == 0) {
            event_wave_create(work->pos.x, work->pos.y, work->pos.z,
                              params->speed, params->scale);
        }
        if (EMITTER(work)->age > params->steps) {
            work->mode = WAVE_END;
        }
        break;
    case WAVE_END:
        if (task != NULL) {
            task_kill(task);
        }
        break;
    }
}

static void event_wave_emitter_draw(struct task *task)
{
}

static void event_wave_emitter_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct wave_emitter_params *params = EMITTER(work)->params;

    if (params != NULL) {
        heap_free(params);
    }
}
