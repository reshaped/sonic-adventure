/* O Antena solid antenna with a pulsing glow on top that faces the camera */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg04/antenna.h"
#include "stg04/corner_light.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT antenna_object;

/* the glow's phase gains GLOW_STEP degrees a step. the glow is drawn at
 * GLOW_SCALE times its frame's size, GLOW_HEIGHT straight above the antenna's
 * position */
#define GLOW_STEP 4
#define GLOW_SCALE 0.4F
#define GLOW_HEIGHT 170.0F

/* the glow's phase, in degrees */
#define GLOW_PHASE(work) TASK_TIMER(work)

static void antenna_destroy(struct task *task);
static void antenna_draw(struct task *task);
static void antenna_run(struct task *task);

/* a cylinder and two boxes centered 65 above the antenna's position, and the
 * same at 15. a seventh shape, a cylinder at 100, follows them */
static struct collision_shape antenna_shapes[7] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-6.0F, 65.0F, -5.0F},
     14.0F,
     40.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-6.0F, 65.0F, 0.0F},
     16.0F,
     40.0F,
     4.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-6.0F, 65.0F, -4.0F},
     14.0F,
     40.0F,
     4.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-6.0F, 15.0F, -5.0F},
     11.0F,
     15.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-6.0F, 15.0F, 0.0F},
     12.0F,
     15.0F,
     4.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-6.0F, 15.0F, -4.0F},
     10.0F,
     15.0F,
     4.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-6.0F, 100.0F, -4.0F},
     12.0F,
     10.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static void antenna_destroy(struct task *task)
{
}

/* draw the antenna, then its glow. the glow faces the camera, whatever the
 * antenna's angles, and draws without fog. its color, a gray of half the sine
 * of its phase, adds to what lies behind it */
static void antenna_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB material;
    Float gray;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njPushMatrix(NULL);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_tree(&antenna_object);
    njPopMatrix(1);
    material.a = -1.0F;
    gray = njSin((Angle)(GLOW_PHASE(work) * ANGLE_PER_DEGREE)) * 0.5F;
    material.r = material.g = material.b = gray;
    njSetConstantMaterial(&material);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njTranslate(NULL, 0.0F, GLOW_HEIGHT, 0.0F);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    njSetTexture(&corner_light_texlist);
    njFogDisable();
    njDrawSprite3D(&corner_light_sprite, 0,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_VFLIP | NJD_SPRITE_COLOR);
    njFogEnable();
    material.a = material.r = material.g = material.b = 0.0F;
    njSetConstantMaterial(&material);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njPopMatrix(1);
}

/* the step antenna_step sets. the glow pulses even while the set editor holds
 * the antenna. the antenna collides only while the editor doesn't */
static void antenna_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        GLOW_PHASE(work) += GLOW_STEP;
        antenna_draw(task);
    }
}

/* the glow draws with O CLIGHT's sprite, which this sets up as
 * corner_light_sprite_init does */
void antenna_step(struct task *task)
{
    /* the first six shapes: the cylinder at 100 never collides */
    collision_init(task, antenna_shapes, COLLISION_OBJECT, 1, 6);
    corner_light_sprite.tlist = &corner_light_texlist;
    corner_light_sprite.tanim = corner_light_texanim;
    corner_light_sprite.sx = GLOW_SCALE;
    corner_light_sprite.sy = GLOW_SCALE;
    corner_light_sprite.p.x = 0.0F;
    corner_light_sprite.p.y = 0.0F;
    corner_light_sprite.p.z = 0.0F;
    task->destructor = antenna_destroy;
    task->step = antenna_run;
    task->draw = antenna_draw;
}
