/* C HANE fan that spins about y by its set entry's angle x each step */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg10/fan.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define SE_FAN 985

/* the fan's turn each step when its angle x is 0 */
#define SPEED NJM_DEG_ANG(1)
/* a scale below SCALE_MIN is taken as 1 */
#define SCALE_MIN 0.01F

struct final_fan_vars {
    Uint8 unk_00[8];
    Angle speed; /* the turn about y each step */
};

#define VARS(work) ((struct final_fan_vars *)&(work)->vars)

static void final_fan_run(struct task *task);
static void final_fan_draw(struct task *task);
static void final_fan_destroy(struct task *task);

static NJS_MATERIAL final_fan_node6_materials[5] = {
#include "stg10/fan/final_fan_node6_materials.inc"
};

static Sint16 final_fan_node6_meshes0[11] = {
#include "stg10/fan/final_fan_node6_meshes0.inc"
};

static Sint16 final_fan_node6_meshes1[20] = {
#include "stg10/fan/final_fan_node6_meshes1.inc"
};

static Sint16 final_fan_node6_meshes2[5] = {
#include "stg10/fan/final_fan_node6_meshes2.inc"
};

static Sint16 final_fan_node6_meshes3[22] = {
#include "stg10/fan/final_fan_node6_meshes3.inc"
};

static NJS_COLOR final_fan_node6_uvs0[10] = {
#include "stg10/fan/final_fan_node6_uvs0.inc"
};

static NJS_COLOR final_fan_node6_uvs1[16] = {
#include "stg10/fan/final_fan_node6_uvs1.inc"
};

static NJS_COLOR final_fan_node6_uvs2[4] = {
#include "stg10/fan/final_fan_node6_uvs2.inc"
};

static NJS_COLOR final_fan_node6_uvs3[18] = {
#include "stg10/fan/final_fan_node6_uvs3.inc"
};

static NJS_MESHSET final_fan_node6_meshsets[4] = {
#include "stg10/fan/final_fan_node6_meshsets.inc"
};

static NJS_POINT3 final_fan_node6_points[16] = {
#include "stg10/fan/final_fan_node6_points.inc"
};

static NJS_VECTOR final_fan_node6_normals[16] = {
#include "stg10/fan/final_fan_node6_normals.inc"
};

static NJS_MODEL final_fan_node6_model = {
#include "stg10/fan/final_fan_node6_model.inc"
};

static NJS_OBJECT final_fan_node6_object = {
#include "stg10/fan/final_fan_node6_object.inc"
};

static NJS_MATERIAL final_fan_node5_materials[5] = {
#include "stg10/fan/final_fan_node5_materials.inc"
};

static Sint16 final_fan_node5_meshes0[11] = {
#include "stg10/fan/final_fan_node5_meshes0.inc"
};

static Sint16 final_fan_node5_meshes1[20] = {
#include "stg10/fan/final_fan_node5_meshes1.inc"
};

static Sint16 final_fan_node5_meshes2[5] = {
#include "stg10/fan/final_fan_node5_meshes2.inc"
};

static Sint16 final_fan_node5_meshes3[22] = {
#include "stg10/fan/final_fan_node5_meshes3.inc"
};

static NJS_COLOR final_fan_node5_uvs0[10] = {
#include "stg10/fan/final_fan_node5_uvs0.inc"
};

static NJS_COLOR final_fan_node5_uvs1[16] = {
#include "stg10/fan/final_fan_node5_uvs1.inc"
};

static NJS_COLOR final_fan_node5_uvs2[4] = {
#include "stg10/fan/final_fan_node5_uvs2.inc"
};

static NJS_COLOR final_fan_node5_uvs3[18] = {
#include "stg10/fan/final_fan_node5_uvs3.inc"
};

static NJS_MESHSET final_fan_node5_meshsets[4] = {
#include "stg10/fan/final_fan_node5_meshsets.inc"
};

static NJS_POINT3 final_fan_node5_points[16] = {
#include "stg10/fan/final_fan_node5_points.inc"
};

static NJS_VECTOR final_fan_node5_normals[16] = {
#include "stg10/fan/final_fan_node5_normals.inc"
};

static NJS_MODEL final_fan_node5_model = {
#include "stg10/fan/final_fan_node5_model.inc"
};

static NJS_OBJECT final_fan_node5_object = {
#include "stg10/fan/final_fan_node5_object.inc"
};

static NJS_MATERIAL final_fan_node4_materials[5] = {
#include "stg10/fan/final_fan_node4_materials.inc"
};

static Sint16 final_fan_node4_meshes0[11] = {
#include "stg10/fan/final_fan_node4_meshes0.inc"
};

static Sint16 final_fan_node4_meshes1[20] = {
#include "stg10/fan/final_fan_node4_meshes1.inc"
};

static Sint16 final_fan_node4_meshes2[5] = {
#include "stg10/fan/final_fan_node4_meshes2.inc"
};

static Sint16 final_fan_node4_meshes3[22] = {
#include "stg10/fan/final_fan_node4_meshes3.inc"
};

static NJS_COLOR final_fan_node4_uvs0[10] = {
#include "stg10/fan/final_fan_node4_uvs0.inc"
};

static NJS_COLOR final_fan_node4_uvs1[16] = {
#include "stg10/fan/final_fan_node4_uvs1.inc"
};

static NJS_COLOR final_fan_node4_uvs2[4] = {
#include "stg10/fan/final_fan_node4_uvs2.inc"
};

static NJS_COLOR final_fan_node4_uvs3[18] = {
#include "stg10/fan/final_fan_node4_uvs3.inc"
};

static NJS_MESHSET final_fan_node4_meshsets[4] = {
#include "stg10/fan/final_fan_node4_meshsets.inc"
};

static NJS_POINT3 final_fan_node4_points[16] = {
#include "stg10/fan/final_fan_node4_points.inc"
};

static NJS_VECTOR final_fan_node4_normals[16] = {
#include "stg10/fan/final_fan_node4_normals.inc"
};

static NJS_MODEL final_fan_node4_model = {
#include "stg10/fan/final_fan_node4_model.inc"
};

static NJS_OBJECT final_fan_node4_object = {
#include "stg10/fan/final_fan_node4_object.inc"
};

static NJS_MATERIAL final_fan_node3_materials[5] = {
#include "stg10/fan/final_fan_node3_materials.inc"
};

static Sint16 final_fan_node3_meshes0[11] = {
#include "stg10/fan/final_fan_node3_meshes0.inc"
};

static Sint16 final_fan_node3_meshes1[20] = {
#include "stg10/fan/final_fan_node3_meshes1.inc"
};

static Sint16 final_fan_node3_meshes2[5] = {
#include "stg10/fan/final_fan_node3_meshes2.inc"
};

static Sint16 final_fan_node3_meshes3[22] = {
#include "stg10/fan/final_fan_node3_meshes3.inc"
};

static NJS_COLOR final_fan_node3_uvs0[10] = {
#include "stg10/fan/final_fan_node3_uvs0.inc"
};

static NJS_COLOR final_fan_node3_uvs1[16] = {
#include "stg10/fan/final_fan_node3_uvs1.inc"
};

static NJS_COLOR final_fan_node3_uvs2[4] = {
#include "stg10/fan/final_fan_node3_uvs2.inc"
};

static NJS_COLOR final_fan_node3_uvs3[18] = {
#include "stg10/fan/final_fan_node3_uvs3.inc"
};

static NJS_MESHSET final_fan_node3_meshsets[4] = {
#include "stg10/fan/final_fan_node3_meshsets.inc"
};

static NJS_POINT3 final_fan_node3_points[16] = {
#include "stg10/fan/final_fan_node3_points.inc"
};

static NJS_VECTOR final_fan_node3_normals[16] = {
#include "stg10/fan/final_fan_node3_normals.inc"
};

static NJS_MODEL final_fan_node3_model = {
#include "stg10/fan/final_fan_node3_model.inc"
};

static NJS_OBJECT final_fan_node3_object = {
#include "stg10/fan/final_fan_node3_object.inc"
};

static NJS_MATERIAL final_fan_node2_materials[5] = {
#include "stg10/fan/final_fan_node2_materials.inc"
};

static Sint16 final_fan_node2_meshes0[11] = {
#include "stg10/fan/final_fan_node2_meshes0.inc"
};

static Sint16 final_fan_node2_meshes1[20] = {
#include "stg10/fan/final_fan_node2_meshes1.inc"
};

static Sint16 final_fan_node2_meshes2[5] = {
#include "stg10/fan/final_fan_node2_meshes2.inc"
};

static Sint16 final_fan_node2_meshes3[22] = {
#include "stg10/fan/final_fan_node2_meshes3.inc"
};

static NJS_COLOR final_fan_node2_uvs0[10] = {
#include "stg10/fan/final_fan_node2_uvs0.inc"
};

static NJS_COLOR final_fan_node2_uvs1[16] = {
#include "stg10/fan/final_fan_node2_uvs1.inc"
};

static NJS_COLOR final_fan_node2_uvs2[4] = {
#include "stg10/fan/final_fan_node2_uvs2.inc"
};

static NJS_COLOR final_fan_node2_uvs3[18] = {
#include "stg10/fan/final_fan_node2_uvs3.inc"
};

static NJS_MESHSET final_fan_node2_meshsets[4] = {
#include "stg10/fan/final_fan_node2_meshsets.inc"
};

static NJS_POINT3 final_fan_node2_points[16] = {
#include "stg10/fan/final_fan_node2_points.inc"
};

static NJS_VECTOR final_fan_node2_normals[16] = {
#include "stg10/fan/final_fan_node2_normals.inc"
};

static NJS_MODEL final_fan_node2_model = {
#include "stg10/fan/final_fan_node2_model.inc"
};

static NJS_OBJECT final_fan_node2_object = {
#include "stg10/fan/final_fan_node2_object.inc"
};

static NJS_MATERIAL final_fan_node1_materials[5] = {
#include "stg10/fan/final_fan_node1_materials.inc"
};

static Sint16 final_fan_node1_meshes0[39] = {
#include "stg10/fan/final_fan_node1_meshes0.inc"
};

static Sint16 final_fan_node1_meshes1[19] = {
#include "stg10/fan/final_fan_node1_meshes1.inc"
};

static Sint16 final_fan_node1_meshes2[19] = {
#include "stg10/fan/final_fan_node1_meshes2.inc"
};

static Sint16 final_fan_node1_meshes3[19] = {
#include "stg10/fan/final_fan_node1_meshes3.inc"
};

static NJS_COLOR final_fan_node1_uvs0[34] = {
#include "stg10/fan/final_fan_node1_uvs0.inc"
};

static NJS_COLOR final_fan_node1_uvs1[18] = {
#include "stg10/fan/final_fan_node1_uvs1.inc"
};

static NJS_COLOR final_fan_node1_uvs2[18] = {
#include "stg10/fan/final_fan_node1_uvs2.inc"
};

static NJS_COLOR final_fan_node1_uvs3[18] = {
#include "stg10/fan/final_fan_node1_uvs3.inc"
};

static NJS_MESHSET final_fan_node1_meshsets[4] = {
#include "stg10/fan/final_fan_node1_meshsets.inc"
};

static NJS_POINT3 final_fan_node1_points[41] = {
#include "stg10/fan/final_fan_node1_points.inc"
};

static NJS_VECTOR final_fan_node1_normals[41] = {
#include "stg10/fan/final_fan_node1_normals.inc"
};

static NJS_MODEL final_fan_node1_model = {
#include "stg10/fan/final_fan_node1_model.inc"
};

static NJS_OBJECT final_fan_node1_object = {
#include "stg10/fan/final_fan_node1_object.inc"
};

/* @hack type: final_fan_object declared as an array of one, and read as
 * final_fan_object->child. SHC then loads the object's address and reads the
 * child 44 bytes past it (MOV.L @(44,R4),R4 at 0x0c915d18). as one NJS_OBJECT
 * read as final_fan_object.child, the pool word holds the child field's own
 * address, and 2 of final_fan_draw's 55 words differ */
static NJS_OBJECT final_fan_object[1] = {
#include "stg10/fan/final_fan_object.inc"
};

void final_fan_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->speed = work->ang.x ? work->ang.x : SPEED;
    work->ang.x = 0;
    /* @hack spelling: the three clamps as one expression, which loads each
     * scale into a register of its own, FR5 to FR7 (0x0c915b9a, 0x0c915baa,
     * 0x0c915bba). as three statements, each takes FR5, and 6 of 43 words
     * differ */
    work->scl.x = work->scl.x < SCALE_MIN ? 1.0F : work->scl.x,
    work->scl.y = work->scl.y < SCALE_MIN ? 1.0F : work->scl.y,
    work->scl.z = work->scl.z < SCALE_MIN ? 1.0F : work->scl.z;
    task->step = final_fan_run;
    task->draw = final_fan_draw;
    task->destructor = final_fan_destroy;
}

static void final_fan_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "<- FAN SPEED(ANGLE)");
        if (work->ang.x) {
            VARS(work)->speed = work->ang.x;
        }
    }
    work->ang.y += VARS(work)->speed;
    final_fan_draw(task);
    sound_play_at_for(SE_FAN, (int)task, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 2);
}

static void final_fan_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        if (!placement_try_leave(task)) {
            texture_set_stage();
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, work->ang.y);
            njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
            draw_tree(final_fan_object->child);
            njPopMatrix(1);
        }
    }
}

static void final_fan_destroy(struct task *task)
{
}
