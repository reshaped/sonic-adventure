/* O Egg Kanban sign whose face flickers at random */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "object/common.h"
#include "stg10/egg_sign.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define FACE 1 /* the face's material */
/* the face's blend: its color times (1 - its alpha), added to the color
 * behind it */
#define FACE_BLEND (NJD_SA_INV_SRC | NJD_DA_ONE | NJD_FLAG_USE_ALPHA)
/* the fade's phase runs from 0 to FADE_END, a quarter turn less 1, where the
 * face's alpha peaks */
#define FADE_END 0x3fff

/* the sign's view of its task_work, which must keep task_work's fields at
 * their offsets */
struct egg_sign_work {
    Sint8 up; /* in mode's place: 1 while the fade rises, else 0 */
    /* in submode's place: 1 once the fade has stopped at an end, until up
     * changes */
    Sint8 done;
    Uint8 id;
    Uint8 unk_03;
    Sint16 flags;
    Uint8 unk_06[6];
    Angle fade; /* the fade's phase; the face's alpha is its sine */
    NJS_MODEL *model; /* the sign's copy of its model, or null */
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 scl;
    struct collision_work *collision;
    struct event_work *event;
};

static void egg_sign_run(struct task *task);
static void egg_sign_draw(struct task *task);
static void egg_sign_destroy(struct task *task);
static void egg_sign_copy_model(struct task *task, NJS_MODEL *model);

static NJS_MATERIAL egg_sign_node1_materials[1] = {
#include "stg10/egg_sign/egg_sign_node1_materials.inc"
};

static Sint16 egg_sign_node1_meshes[25] = {
#include "stg10/egg_sign/egg_sign_node1_meshes.inc"
};

static NJS_COLOR egg_sign_node1_uvs[20] = {
#include "stg10/egg_sign/egg_sign_node1_uvs.inc"
};

static NJS_MESHSET egg_sign_node1_meshsets[1] = {
#include "stg10/egg_sign/egg_sign_node1_meshsets.inc"
};

static NJS_POINT3 egg_sign_node1_points[8] = {
#include "stg10/egg_sign/egg_sign_node1_points.inc"
};

static NJS_VECTOR egg_sign_node1_normals[8] = {
#include "stg10/egg_sign/egg_sign_node1_normals.inc"
};

static NJS_MODEL egg_sign_node1_model = {
#include "stg10/egg_sign/egg_sign_node1_model.inc"
};

static NJS_OBJECT egg_sign_node1_object = {
#include "stg10/egg_sign/egg_sign_node1_object.inc"
};

static NJS_MATERIAL egg_sign_materials[4] = {
#include "stg10/egg_sign/egg_sign_materials.inc"
};

static Sint16 egg_sign_meshes0[10] = {
#include "stg10/egg_sign/egg_sign_meshes0.inc"
};

static Sint16 egg_sign_meshes1[5] = {
#include "stg10/egg_sign/egg_sign_meshes1.inc"
};

static Sint16 egg_sign_meshes2[10] = {
#include "stg10/egg_sign/egg_sign_meshes2.inc"
};

static Sint16 egg_sign_meshes3[208] = {
#include "stg10/egg_sign/egg_sign_meshes3.inc"
};

static NJS_COLOR egg_sign_uvs0[8] = {
#include "stg10/egg_sign/egg_sign_uvs0.inc"
};

static NJS_COLOR egg_sign_uvs1[4] = {
#include "stg10/egg_sign/egg_sign_uvs1.inc"
};

static NJS_COLOR egg_sign_uvs2[8] = {
#include "stg10/egg_sign/egg_sign_uvs2.inc"
};

static NJS_COLOR egg_sign_uvs3[192] = {
#include "stg10/egg_sign/egg_sign_uvs3.inc"
};

static NJS_MESHSET egg_sign_meshsets[4] = {
#include "stg10/egg_sign/egg_sign_meshsets.inc"
};

static NJS_POINT3 egg_sign_points[104] = {
#include "stg10/egg_sign/egg_sign_points.inc"
};

static NJS_VECTOR egg_sign_normals[104] = {
#include "stg10/egg_sign/egg_sign_normals.inc"
};

static NJS_MODEL egg_sign_model = {
#include "stg10/egg_sign/egg_sign_model.inc"
};

/* the sign, whose child is the frame around the face */
/* @hack type: an array of one object, so SHC reads the model 4 bytes past
 * the object's address (MOV.L @(4,R5),R5 at 0x0c9158b6); as a plain object,
 * its pool word holds the model's own address and 4 of egg_sign_step's 73
 * words differ */
static NJS_OBJECT egg_sign_object[1] = {
#include "stg10/egg_sign/egg_sign_object.inc"
};

/* the fade's change each step, indexed by up */
static const Angle egg_sign_fade_steps[2] = {-NJM_DEG_ANG(6), NJM_DEG_ANG(6)};

void egg_sign_step(struct task *task)
{
    struct egg_sign_work *work = (struct egg_sign_work *)task->work;

    /* @hack redundant: pos is tested twice, as the image's first and fifth
     * compares both test its offset (0x0c915872, 0x0c9158a4); tested once,
     * 44 of 73 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct egg_sign_work, pos) ||
        !WORK_SAME_PLACE(struct egg_sign_work, id) ||
        !WORK_SAME_PLACE(struct egg_sign_work, collision) ||
        !WORK_SAME_PLACE(struct egg_sign_work, event) ||
        !WORK_SAME_PLACE(struct egg_sign_work, pos) ||
        !WORK_SAME_PLACE(struct egg_sign_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    egg_sign_copy_model(task, egg_sign_object[0].model);
    if (work->model != NULL) {
        work->model->mats[FACE].attrflags &= ~(NJD_SA_MASK | NJD_DA_MASK);
        work->model->mats[FACE].attrflags |= FACE_BLEND;
    }
    task->step = egg_sign_run;
    task->draw = egg_sign_draw;
    task->destructor = egg_sign_destroy;
}

/* each step a coin toss picks the fade's way. the fade steps that way, and
 * stops at its end until a toss turns it */
static void egg_sign_run(struct task *task)
{
    struct egg_sign_work *work = (struct egg_sign_work *)task->work;
    Sint8 up;
    int alpha;

    if (work->model != NULL) {
        up = rand() * RAND_SCALE > 0.5F;
        if (work->up == up) {
            if (!work->done) {
                work->fade += egg_sign_fade_steps[work->up];
                if (work->fade & ~FADE_END) {
                    work->done = 1;
                    work->fade = work->up == 0 ? 0 : FADE_END;
                }
            }
        } else {
            work->done = 0;
            work->up = up;
            work->fade += egg_sign_fade_steps[work->up];
        }
        alpha = (int)(njSin(work->fade) * 255.0F);
        work->model->mats[FACE].diffuse.argb.a = alpha;
    }
    egg_sign_draw(task);
}

static void egg_sign_draw(struct task *task)
{
    struct egg_sign_work *work = (struct egg_sign_work *)task->work;
    NJS_OBJECT *object;
    NJS_OBJECT *child;

    if (frame_step == 0) {
        texture_set_stage();
        draw_control_save();
        draw_control_clear(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL |
                           NJD_CONTROL_3D_OFFSET_MATERIAL);
        njPushMatrix(NULL);
        object = egg_sign_object;
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        if (work->model != NULL) {
            njDrawModel(work->model);
        } else {
            njDrawModel(object->model);
        }
        child = object->child;
        njDrawModel(child->model);
        njPopMatrix(1);
        draw_control_restore();
    }
}

static void egg_sign_destroy(struct task *task)
{
    struct egg_sign_work *work = (struct egg_sign_work *)task->work;

    if (work->model != NULL) {
        heap_free(work->model->mats);
        heap_free(work->model);
    }
}

/* give the sign its own copy of model and its materials, or leave work->model
 * null if either allocation fails */
static void egg_sign_copy_model(struct task *task, NJS_MODEL *model)
{
    struct egg_sign_work *work = (struct egg_sign_work *)task->work;
    int count;
    NJS_MATERIAL *mats;

    /* @hack spelling: work->model assigned in the test, which SHC tests in R0
     * (TST R0,R0 at 0x0c915ad8) and stores in the branch's delay slot. as a
     * statement of its own, 61 of the function's 71 words differ */
    if ((work->model = heap_alloc(sizeof(NJS_MODEL))) != NULL) {
        count = model->nbMat;
        *work->model = *model;
        /* @hack spelling: mats assigned in the test, which SHC tests in R0 (TST
         * R0,R0 at 0x0c915b1e) and copies to R12 in the branch's delay slot.
         * as a statement of its own, 3 of the function's 71 words differ */
        if ((mats = heap_alloc(count * sizeof(NJS_MATERIAL))) != NULL) {
            njMemCopy4(mats, work->model->mats,
                       count * (sizeof(NJS_MATERIAL) / 4));
            work->model->mats = mats;
        } else {
            heap_free(work->model);
            work->model = NULL;
        }
    }
}
