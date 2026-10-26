/* SIGNBOARD1 to 5, solid boards that each show two textures by turns */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "minicart/object_textures.h"
#include "minicart/signboard.h"
#include "object/placement.h"
#include "object/simple.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* a kind of signboard: its textures, and the steps each one shows */
struct signboard_kind {
    const Sint16 *textures;
    Sint16 count;
    Sint16 delay;
};

/* @hack spelling: arg read through KIND at each use, which keeps arg in R10
 * (MOV R5,R10 at 0x0c90be7e); through a local, SHC keeps it on the stack,
 * signboard_init is 8 bytes longer and 64 of 69 words differ */
#define KIND(arg) ((const struct signboard_kind *)(arg))

/* a signboard's record, in task->motion */
struct signboard_record {
    Sint16 shown; /* the index in textures of the one shown */
    Sint16 count;
    Sint16 wait; /* the steps until the next texture */
    Sint16 delay;
    NJS_MODEL *model;
    NJS_MATERIAL *face; /* the material the picture is on */
    const Sint16 *textures;
    NJS_OBJECT *world; /* the board's world object */
    Float reach; /* the test's radius: twice the model's */
};

extern NJS_OBJECT signboard_object;

static void signboard_init(struct task *task, void *arg);
static void signboard_run(struct task *task);
static void signboard_draw(struct task *task);
static void signboard_destroy(struct task *task);

static const struct task_desc signboard_desc = {sizeof(struct signboard_record),
                                                TASK_WORK | TASK_FORCE,
                                                signboard_init,
                                                signboard_run,
                                                signboard_draw,
                                                signboard_destroy};
static const Sint16 signboard_1_textures[2] = {
#include "minicart/signboard/signboard_1_textures.inc"
};
static const struct signboard_kind signboard_1 = {
#include "minicart/signboard/signboard_1.inc"
};
static const Sint16 signboard_2_textures[2] = {
#include "minicart/signboard/signboard_2_textures.inc"
};
static const struct signboard_kind signboard_2 = {
#include "minicart/signboard/signboard_2.inc"
};
static const Sint16 signboard_3_textures[2] = {
#include "minicart/signboard/signboard_3_textures.inc"
};
static const struct signboard_kind signboard_3 = {
#include "minicart/signboard/signboard_3.inc"
};
static const Sint16 signboard_4_textures[2] = {
#include "minicart/signboard/signboard_4_textures.inc"
};
static const struct signboard_kind signboard_4 = {
#include "minicart/signboard/signboard_4.inc"
};
static const Sint16 signboard_5_textures[2] = {
#include "minicart/signboard/signboard_5_textures.inc"
};
static const struct signboard_kind signboard_5 = {
#include "minicart/signboard/signboard_5.inc"
};

void signboard_1_step(struct task *task)
{
    task_apply_desc(task, &signboard_desc, (void *)&signboard_1);
}

void signboard_2_step(struct task *task)
{
    task_apply_desc(task, &signboard_desc, (void *)&signboard_2);
}

void signboard_3_step(struct task *task)
{
    task_apply_desc(task, &signboard_desc, (void *)&signboard_3);
}

void signboard_4_step(struct task *task)
{
    task_apply_desc(task, &signboard_desc, (void *)&signboard_4);
}

void signboard_5_step(struct task *task)
{
    task_apply_desc(task, &signboard_desc, (void *)&signboard_5);
}

static void signboard_init(struct task *task, void *arg)
{
    NJS_OBJECT *object = &signboard_object;
    NJS_OBJECT *world = object_world_copy(object);
    struct signboard_record *record = (struct signboard_record *)task->motion;
    NJS_MODEL *model;
    Sint16 delay;

    record->world = world;
    if (world == NULL) {
        task_kill(task);
    } else {
        object_world_place(world, task->work, 1.0F);
        object_world_add_sized(task, world, WORLD_SOLID, 1.0F);
        object_force_clear(task->force);
        delay = KIND(arg)->delay;
        model = object->model;
        record->shown = 0;
        record->count = KIND(arg)->count;
        record->wait = delay;
        record->delay = delay;
        record->model = model;
        record->face = signboard_face;
        record->textures = KIND(arg)->textures;
        record->reach = model->r * 2.0F;
    }
}

/* draw the board. unless the set editor holds it, test its world object
 * while player 0 or 1 is within reach. the board shows its next texture
 * every delay steps */
static void signboard_run(struct task *task)
{
    struct task_work *work;
    struct signboard_record *record;

    if (placement_try_leave(task)) {
        return;
    }
    signboard_draw(task);
    work = task->work;
    record = (struct signboard_record *)task->motion;
    if (!placement_editing(task)) {
        object_world_test_radius(work, record->reach);
    }
    record->wait--;
    if (record->wait <= 0) {
        record->wait = record->delay;
        record->shown++;
        if (record->shown >= record->count) {
            record->shown = 0;
        }
    }
}

/* on a frame's first step, draw the board with its texture shown. every
 * board draws signboard_object, so each sets the face's texture first */
static void signboard_draw(struct task *task)
{
    struct signboard_record *record;
    struct task_work *work;
    struct angles *ang;
    Angle turn;

    if (frame_step != 0) {
        return;
    }
    record = (struct signboard_record *)task->motion;
    work = task->work;
    ang = &work->ang;
    record->face->attr_texId = (Uint16)record->textures[record->shown];
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, ang->z, turn);
    TURN_VIA(njRotateY, ang->y, turn);
    texture_set_stage();
    /* @hack redundant: a never-run call of njScaleV with work's scale, which
     * makes SHC weigh work as the image does: work takes R13 and ang R12
     * (0x0c90bfd4, 0x0c90bfda), which otherwise trade; without it, 6 of 64
     * words differ */
    if (0) {
        njScaleV(NULL, &work->scl);
    }
    draw_model(record->model);
    njPopMatrix(1);
}

static void signboard_destroy(struct task *task)
{
    struct signboard_record *record = (struct signboard_record *)task->motion;

    object_world_free(task, record->world);
}
