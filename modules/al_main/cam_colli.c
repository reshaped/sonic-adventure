/* CAM COLLI, the gardens' camera keep-outs: each a sphere that pushes the
 * garden's camera out, the harder the deeper the camera is */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/cam_colli.h"
#include "al_main/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "system/task.h"

/* how far past its work's scl.x the sphere reaches */
#define COLLI_REACH 15.0F
/* a step's push out of the sphere: COLLI_PUSH, and COLLI_DEPTH_PUSH for
 * each unit of the camera's depth inside it */
#define COLLI_PUSH 0.1F
#define COLLI_DEPTH_PUSH 0.75F

static void cam_colli_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float radius = work->scl.x + COLLI_REACH;
    Float scale;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_set_material(0.03F, 0.5F, 0.5F, 0.0F);
    /* marker_object is a sphere of radius 10 */
    scale = radius * 0.1F;
    njScale(NULL, scale, scale, scale);
    njEasyDrawObject(&marker_object);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}

void cam_colli_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR away;
    Float distance;
    Float depth;
    Float push;

    if (edit_mode) {
        cam_colli_draw(task);
    }
    away.x = garden_camera_pos.x - work->pos.x;
    away.y = garden_camera_pos.y - work->pos.y;
    away.z = garden_camera_pos.z - work->pos.z;
    distance = njScalor(&away);
    if (distance == 0.0F) {
        away.x = away.y = 0.0F;
        away.z = 0.1F;
    } else {
        njUnitVector(&away);
    }
    if (work->scl.x + COLLI_REACH > distance) {
        depth = work->scl.x - distance + COLLI_REACH;
        push = COLLI_PUSH + (depth * COLLI_DEPTH_PUSH);
        away.x *= push;
        away.y *= push;
        away.z *= push;
        garden_camera_pos.x += away.x;
        garden_camera_pos.y += away.y;
        garden_camera_pos.z += away.z;
    }
}
