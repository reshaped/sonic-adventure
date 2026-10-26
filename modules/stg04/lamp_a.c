/* O Lmpa lamp at the end of an arm, with a glow that faces the camera */
#include <ninja.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg04/lamp_a.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL highway_lamp_a_glow_model;
extern NJS_MODEL highway_lamp_a_model;

#define LAMP_A_REACH 9.7F
/* the lamp is drawn while a sphere of this radius around it is in view */
#define LAMP_A_SIZE 10.0F

/* the glow's square lies in its model's yz plane. the camera's angles would
 * face a square in the xy plane to the camera. GLOW_FACING turns the glow the
 * quarter turn between the two */
#define GLOW_FACING 0x4000

static void highway_lamp_a_destroy(struct task *task);
static void highway_lamp_a_stand(struct task *task);

static struct collision_shape highway_lamp_a_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, -1.0F, 7.0F},
    2.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void highway_lamp_a_destroy(struct task *task)
{
}

/* the lamp's step and draw, which do nothing out of range. outside the set
 * editor, it registers the lamp's collision. while the lamp is in view, it
 * draws the arm, then the glow facing the camera without fog */
static void highway_lamp_a_stand(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, LAMP_A_SIZE)) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_model(&highway_lamp_a_model);
    njTranslate(NULL, 0.0F, -0.65F, 7.378999F);
    TURN(njRotateY, camera_work->ang.y - work->ang.y + GLOW_FACING);
    TURN(njRotateX, camera_work->ang.x);
    njFogDisable();
    draw_model(&highway_lamp_a_glow_model);
    njFogEnable();
    njPopMatrix(1);
}

void highway_lamp_a_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &highway_lamp_a_shape, COLLISION_OBJECT, 1, 1);
    work->collision->radius = LAMP_A_REACH;
    task->destructor = highway_lamp_a_destroy;
    task->step = highway_lamp_a_stand;
    task->draw = highway_lamp_a_stand;
}
