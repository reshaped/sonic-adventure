/* the tables of Chaos 2's hall, the kind TABLE__. a table draws with the
 * textures of chaos2_object_texlist, and its flowers are one flat model,
 * turned about y to face the camera */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos2/table.h"
#include "b_chaos2/textures.h"
#include "camera/camera.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/util.h"

/* the height of the flowers' model over the table's pos */
#define FLOWER_HEIGHT 7.5F

static NJS_MATERIAL chaos2_table_materials[9] = {
#include "b_chaos2/table/chaos2_table_materials.inc"
};
static Sint16 chaos2_table_meshes0[16] = {
#include "b_chaos2/table/chaos2_table_meshes0.inc"
};
static Sint16 chaos2_table_meshes1[20] = {
#include "b_chaos2/table/chaos2_table_meshes1.inc"
};
static Sint16 chaos2_table_meshes2[16] = {
#include "b_chaos2/table/chaos2_table_meshes2.inc"
};
static Sint16 chaos2_table_meshes3[80] = {
#include "b_chaos2/table/chaos2_table_meshes3.inc"
};
static Sint16 chaos2_table_meshes4[80] = {
#include "b_chaos2/table/chaos2_table_meshes4.inc"
};
static Sint16 chaos2_table_meshes5[20] = {
#include "b_chaos2/table/chaos2_table_meshes5.inc"
};
static Sint16 chaos2_table_meshes6[32] = {
#include "b_chaos2/table/chaos2_table_meshes6.inc"
};
static Sint16 chaos2_table_meshes7[104] = {
#include "b_chaos2/table/chaos2_table_meshes7.inc"
};
static Sint16 chaos2_table_meshes8[192] = {
#include "b_chaos2/table/chaos2_table_meshes8.inc"
};
static Sint16 chaos2_table_meshes9[64] = {
#include "b_chaos2/table/chaos2_table_meshes9.inc"
};
static Sint16 chaos2_table_meshes10[48] = {
#include "b_chaos2/table/chaos2_table_meshes10.inc"
};
static Sint16 chaos2_table_meshes11[192] = {
#include "b_chaos2/table/chaos2_table_meshes11.inc"
};
static NJS_COLOR chaos2_table_uvs0[12] = {
#include "b_chaos2/table/chaos2_table_uvs0.inc"
};
static NJS_COLOR chaos2_table_uvs1[20] = {
#include "b_chaos2/table/chaos2_table_uvs1.inc"
};
static NJS_COLOR chaos2_table_uvs2[16] = {
#include "b_chaos2/table/chaos2_table_uvs2.inc"
};
static NJS_COLOR chaos2_table_uvs3[60] = {
#include "b_chaos2/table/chaos2_table_uvs3.inc"
};
static NJS_COLOR chaos2_table_uvs4[80] = {
#include "b_chaos2/table/chaos2_table_uvs4.inc"
};
static NJS_COLOR chaos2_table_uvs5[16] = {
#include "b_chaos2/table/chaos2_table_uvs5.inc"
};
static NJS_COLOR chaos2_table_uvs6[32] = {
#include "b_chaos2/table/chaos2_table_uvs6.inc"
};
static NJS_COLOR chaos2_table_uvs7[104] = {
#include "b_chaos2/table/chaos2_table_uvs7.inc"
};
static NJS_COLOR chaos2_table_uvs8[192] = {
#include "b_chaos2/table/chaos2_table_uvs8.inc"
};
static NJS_COLOR chaos2_table_uvs9[64] = {
#include "b_chaos2/table/chaos2_table_uvs9.inc"
};
static NJS_COLOR chaos2_table_uvs10[48] = {
#include "b_chaos2/table/chaos2_table_uvs10.inc"
};
static NJS_COLOR chaos2_table_uvs11[192] = {
#include "b_chaos2/table/chaos2_table_uvs11.inc"
};
static NJS_MESHSET chaos2_table_meshsets[12] = {
#include "b_chaos2/table/chaos2_table_meshsets.inc"
};
static NJS_POINT3 chaos2_table_points[314] = {
#include "b_chaos2/table/chaos2_table_points.inc"
};
static NJS_VECTOR chaos2_table_normals[314] = {
#include "b_chaos2/table/chaos2_table_normals.inc"
};
static NJS_MODEL chaos2_table_model = {
#include "b_chaos2/table/chaos2_table_model.inc"
};
static NJS_OBJECT chaos2_table_object = {
#include "b_chaos2/table/chaos2_table_object.inc"
};
static NJS_MATERIAL chaos2_table_flower_materials[1] = {
#include "b_chaos2/table/chaos2_table_flower_materials.inc"
};
static Sint16 chaos2_table_flower_meshes0[4] = {
#include "b_chaos2/table/chaos2_table_flower_meshes0.inc"
};
static NJS_COLOR chaos2_table_flower_uvs0[4] = {
#include "b_chaos2/table/chaos2_table_flower_uvs0.inc"
};
static NJS_MESHSET chaos2_table_flower_meshsets[1] = {
#include "b_chaos2/table/chaos2_table_flower_meshsets.inc"
};
static NJS_POINT3 chaos2_table_flower_points[4] = {
#include "b_chaos2/table/chaos2_table_flower_points.inc"
};
static NJS_VECTOR chaos2_table_flower_normals[4] = {
#include "b_chaos2/table/chaos2_table_flower_normals.inc"
};
static NJS_MODEL chaos2_table_flower_model = {
#include "b_chaos2/table/chaos2_table_flower_model.inc"
};
static NJS_OBJECT chaos2_table_flower_object = {
#include "b_chaos2/table/chaos2_table_flower_object.inc"
};

static void table_destroy(struct task *task)
{
}

static void table_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        object_clear_touch(work, NULL);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njPushMatrix(NULL);
    njSetTexture(MODEL(work)->texlist);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
    njTranslate(NULL, 0.0F, FLOWER_HEIGHT, 0.0F);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    njEasyDrawModel(&chaos2_table_flower_model);
    njPopMatrix(1);
}

/* table_draw is both the table's step and its draw. the table draws
 * itself each step, and task_draw_all draws it under the pause menu, where
 * no step runs */
void chaos2_table_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &chaos2_table_object;
    MODEL(work)->texlist = &chaos2_object_texlist;
    task->destructor = table_destroy;
    task->step = table_draw;
    task->draw = table_draw;
}
