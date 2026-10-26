/* O Pin Lamp and O Side Lamp lamps with a turning glare */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/glare.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg10/lamp.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

enum final_lamp_kind { LAMP_PIN, LAMP_SIDE };

/* the least scale a lamp draws at. a smaller scale becomes 1 */
#define SCALE_MIN 0.1F
#define SPIN 0x400 /* the glare's turn each step */
#define GLARE_SIZE 0.75F

/* the lamp's view of its task_work. fields that share task_work's names
 * must stay at task_work's offsets. pin_lamp_step and side_lamp_step test
 * five of them, pos twice, and start no lamp if one has moved */
struct final_lamp_work {
    Sint8 kind; /* LAMP_PIN or LAMP_SIDE, kept in task_work's mode */
    Uint8 unk_01;
    Uint8 id;
    Uint8 unk_03;
    Sint16 flags;
    Uint8 unk_06[2];
    NJS_POINT3 light; /* the light's position in the world */
    struct angles ang;
    NJS_POINT3 pos;
    Float scale; /* scale x, applied on all three axes */
    Angle spin; /* the glare's turn, kept in scale y */
    Uint8 unk_34[4];
    struct collision_work *collision;
    struct event_work *event;
};

static void final_lamp_start(struct task *task);
static void final_lamp_run(struct task *task);
static void final_lamp_draw(struct task *task);
static void final_lamp_destroy(struct task *task);
static void final_lamp_find_node(NJS_OBJECT *object, NJS_OBJECT *node,
                                 NJS_POINT3 *pos);

static NJS_MATERIAL side_lamp_light_materials[1] = {
#include "stg10/lamp/side_lamp_light_materials.inc"
};

static Sint16 side_lamp_light_meshes[66] = {
#include "stg10/lamp/side_lamp_light_meshes.inc"
};

static NJS_COLOR side_lamp_light_uvs[58] = {
#include "stg10/lamp/side_lamp_light_uvs.inc"
};

static NJS_MESHSET side_lamp_light_meshsets[1] = {
#include "stg10/lamp/side_lamp_light_meshsets.inc"
};

static NJS_POINT3 side_lamp_light_points[28] = {
#include "stg10/lamp/side_lamp_light_points.inc"
};

static NJS_VECTOR side_lamp_light_normals[28] = {
#include "stg10/lamp/side_lamp_light_normals.inc"
};

static NJS_MODEL side_lamp_light_model = {
#include "stg10/lamp/side_lamp_light_model.inc"
};

static NJS_OBJECT side_lamp_light_object = {
#include "stg10/lamp/side_lamp_light_object.inc"
};

static NJS_MATERIAL side_lamp_materials[3] = {
#include "stg10/lamp/side_lamp_materials.inc"
};

static Sint16 side_lamp_meshes0[84] = {
#include "stg10/lamp/side_lamp_meshes0.inc"
};

static Sint16 side_lamp_meshes1[20] = {
#include "stg10/lamp/side_lamp_meshes1.inc"
};

static Sint16 side_lamp_meshes2[10] = {
#include "stg10/lamp/side_lamp_meshes2.inc"
};

static NJS_COLOR side_lamp_uvs0[76] = {
#include "stg10/lamp/side_lamp_uvs0.inc"
};

static NJS_COLOR side_lamp_uvs1[16] = {
#include "stg10/lamp/side_lamp_uvs1.inc"
};

static NJS_COLOR side_lamp_uvs2[8] = {
#include "stg10/lamp/side_lamp_uvs2.inc"
};

static NJS_MESHSET side_lamp_meshsets[3] = {
#include "stg10/lamp/side_lamp_meshsets.inc"
};

static NJS_POINT3 side_lamp_points[44] = {
#include "stg10/lamp/side_lamp_points.inc"
};

static NJS_VECTOR side_lamp_normals[44] = {
#include "stg10/lamp/side_lamp_normals.inc"
};

static NJS_MODEL side_lamp_model = {
#include "stg10/lamp/side_lamp_model.inc"
};

static NJS_OBJECT side_lamp_object = {
#include "stg10/lamp/side_lamp_object.inc"
};

static NJS_MATERIAL pin_lamp_light_materials[1] = {
#include "stg10/lamp/pin_lamp_light_materials.inc"
};

static Sint16 pin_lamp_light_meshes[63] = {
#include "stg10/lamp/pin_lamp_light_meshes.inc"
};

static NJS_COLOR pin_lamp_light_uvs[56] = {
#include "stg10/lamp/pin_lamp_light_uvs.inc"
};

static NJS_MESHSET pin_lamp_light_meshsets[1] = {
#include "stg10/lamp/pin_lamp_light_meshsets.inc"
};

static NJS_POINT3 pin_lamp_light_points[28] = {
#include "stg10/lamp/pin_lamp_light_points.inc"
};

static NJS_VECTOR pin_lamp_light_normals[28] = {
#include "stg10/lamp/pin_lamp_light_normals.inc"
};

static NJS_MODEL pin_lamp_light_model = {
#include "stg10/lamp/pin_lamp_light_model.inc"
};

static NJS_OBJECT pin_lamp_light_object = {
#include "stg10/lamp/pin_lamp_light_object.inc"
};

static NJS_MATERIAL pin_lamp_materials[4] = {
#include "stg10/lamp/pin_lamp_materials.inc"
};

static Sint16 pin_lamp_meshes0[20] = {
#include "stg10/lamp/pin_lamp_meshes0.inc"
};

static Sint16 pin_lamp_meshes1[81] = {
#include "stg10/lamp/pin_lamp_meshes1.inc"
};

static Sint16 pin_lamp_meshes2[20] = {
#include "stg10/lamp/pin_lamp_meshes2.inc"
};

static Sint16 pin_lamp_meshes3[5] = {
#include "stg10/lamp/pin_lamp_meshes3.inc"
};

static NJS_COLOR pin_lamp_uvs0[16] = {
#include "stg10/lamp/pin_lamp_uvs0.inc"
};

static NJS_COLOR pin_lamp_uvs1[72] = {
#include "stg10/lamp/pin_lamp_uvs1.inc"
};

static NJS_COLOR pin_lamp_uvs2[16] = {
#include "stg10/lamp/pin_lamp_uvs2.inc"
};

static NJS_COLOR pin_lamp_uvs3[4] = {
#include "stg10/lamp/pin_lamp_uvs3.inc"
};

static NJS_MESHSET pin_lamp_meshsets[4] = {
#include "stg10/lamp/pin_lamp_meshsets.inc"
};

static NJS_POINT3 pin_lamp_points[47] = {
#include "stg10/lamp/pin_lamp_points.inc"
};

static NJS_VECTOR pin_lamp_normals[47] = {
#include "stg10/lamp/pin_lamp_normals.inc"
};

static NJS_MODEL pin_lamp_model = {
#include "stg10/lamp/pin_lamp_model.inc"
};

static NJS_OBJECT pin_lamp_object = {
#include "stg10/lamp/pin_lamp_object.inc"
};

/* each kind's lamp, and the node of the lamp's tree that its light shines
 * from */
static NJS_OBJECT *const final_lamp_objects[2] = {&pin_lamp_object,
                                                  &side_lamp_object};
static NJS_OBJECT *const final_lamp_lights[2] = {&pin_lamp_light_object,
                                                 &side_lamp_light_object};

static NJS_ARGB final_lamp_glare_color = {0.4F, 1.0F, 0.2F, 0.4F};

void pin_lamp_step(struct task *task)
{
    struct final_lamp_work *work = (struct final_lamp_work *)task->work;

    /* @hack redundant: pos tested twice, as the image tests it (ADD #32,R2 at
     * 0x0c9154a6 and 0x0c9154d8); tested once, 84 of 109 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct final_lamp_work, pos) ||
        !WORK_SAME_PLACE(struct final_lamp_work, id) ||
        !WORK_SAME_PLACE(struct final_lamp_work, collision) ||
        !WORK_SAME_PLACE(struct final_lamp_work, event) ||
        !WORK_SAME_PLACE(struct final_lamp_work, pos) ||
        !WORK_SAME_PLACE(struct final_lamp_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    work->kind = LAMP_PIN;
    final_lamp_start(task);
}

void side_lamp_step(struct task *task)
{
    struct final_lamp_work *work = (struct final_lamp_work *)task->work;

    /* @hack redundant: pos tested twice, as the image tests it (ADD #32,R2 at
     * 0x0c9154fa and 0x0c91552c); tested once, 85 of 110 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct final_lamp_work, pos) ||
        !WORK_SAME_PLACE(struct final_lamp_work, id) ||
        !WORK_SAME_PLACE(struct final_lamp_work, collision) ||
        !WORK_SAME_PLACE(struct final_lamp_work, event) ||
        !WORK_SAME_PLACE(struct final_lamp_work, pos) ||
        !WORK_SAME_PLACE(struct final_lamp_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    work->kind = LAMP_SIDE;
    final_lamp_start(task);
}

/* set work->light by the set entry's scale, then raise a scale below
 * SCALE_MIN to 1. such a lamp draws at 1, but its light stays where a lamp
 * that small has it */
static void final_lamp_start(struct task *task)
{
    struct final_lamp_work *work = (struct final_lamp_work *)task->work;
    int kind = work->kind; /* NOLINT(bugprone-signed-char-misuse) */
    Float scale;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    scale = work->scale;
    njScale(NULL, scale, scale, scale);
    final_lamp_find_node(final_lamp_objects[kind], final_lamp_lights[kind],
                         &work->light);
    njPopMatrix(1);
    if (work->scale < SCALE_MIN) {
        work->scale = 1.0F;
    }
    task->step = final_lamp_run;
    task->draw = final_lamp_draw;
    task->destructor = final_lamp_destroy;
}

static void final_lamp_run(struct task *task)
{
    struct final_lamp_work *work = (struct final_lamp_work *)task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- scale");
        njPrintColor(0xffff0000);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "Don't EDIT!");
        njPrintColor(0xffc0c0c0);
    }
    work->spin = (work->spin + SPIN) & 0xffff;
    final_lamp_draw(task);
}

static void final_lamp_draw(struct task *task)
{
    struct final_lamp_work *work = (struct final_lamp_work *)task->work;
    int kind = work->kind; /* NOLINT(bugprone-signed-char-misuse) */
    NJS_VECTOR dir = {1.0F, 0.0F, 0.0F};
    Float scale;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    scale = work->scale;
    njScale(NULL, scale, scale, scale);
    draw_tree(final_lamp_objects[kind]);
    njUnitMatrix(NULL);
    TURN(njRotateY, work->spin);
    njCalcVector(NULL, &dir, &dir);
    njPopMatrix(1);
    glare_draw(&work->light, &dir, GLARE_SIZE, &final_lamp_glare_color);
}

static void final_lamp_destroy(struct task *task)
{
}

/* set *pos to where node lies in the tree from object, under the current
 * matrix. each node's pos and ang apply, but not its scl */
static void final_lamp_find_node(NJS_OBJECT *object, NJS_OBJECT *node,
                                 NJS_POINT3 *pos)
{
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    njRotateXYZ(NULL, object->ang[0], object->ang[1], object->ang[2]);
    if (object == node) {
        njGetTranslation(NULL, pos);
    } else if (object->child != NULL) {
        final_lamp_find_node(object->child, node, pos);
    }
    njPopMatrix(1);
    if (object->sibling != NULL) {
        final_lamp_find_node(object->sibling, node, pos);
    }
}
