/* O CLIGHT solid lamp with a pulsing glow that faces the camera */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg04/corner_light.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL corner_light_model;
extern NJS_OBJECT corner_light_object;
extern NJS_TEXNAME corner_light_texname[1];

/* the reach of the lamp's collision record. the lamp draws only while the
 * sphere of radius CORNER_LIGHT_SIZE about its position is in view */
#define CORNER_LIGHT_REACH 11.0F
#define CORNER_LIGHT_SIZE 14.0F

/* the glow's phase gains GLOW_STEP degrees a step. the glow is drawn at
 * GLOW_SCALE times its frame's size, GLOW_HEIGHT up the lamp from its
 * position */
#define GLOW_STEP 4
#define GLOW_SCALE 0.4F
#define GLOW_HEIGHT 10.0F

enum corner_light_mode {
    CORNER_LIGHT_INIT = 0,
    CORNER_LIGHT_STAND = 1,
    CORNER_LIGHT_GONE = 3 /* this file never sets it */
};

/* the glow's phase, in degrees */
#define GLOW_PHASE(work) TASK_TIMER(work)

static void corner_light_pulse(struct task *task);
static void corner_light_draw(struct task *task);
static void corner_light_destroy(struct task *task);
static void corner_light_sprite_init(void);

static struct collision_shape corner_light_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 5.0F, 0.0F},
    5.0F,
    3.0F,
    0.0F,
    {0},
    0,
    NULL,
};

NJS_TEXLIST corner_light_texlist = {
#include "stg04/corner_light/corner_light_texlist.inc"
};
NJS_TEXANIM corner_light_texanim[1] = {
#include "stg04/corner_light/corner_light_texanim.inc"
};

NJS_SPRITE corner_light_sprite;

void corner_light_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case CORNER_LIGHT_INIT:
        MODEL(work)->object = &corner_light_object;
        if (!placement_editing(task)) {
            collision_init(task, &corner_light_shape, COLLISION_OBJECT, 1, 1);
            work->collision->radius = CORNER_LIGHT_REACH;
        }
        corner_light_sprite_init();
        task->destructor = corner_light_destroy;
        task->draw = corner_light_draw;
        work->mode = CORNER_LIGHT_STAND;
        break;
    case CORNER_LIGHT_STAND:
        if (!placement_editing(task)) {
            corner_light_pulse(task);
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        corner_light_draw(task);
        break;
    case CORNER_LIGHT_GONE:
        break;
    }
}

static void corner_light_pulse(struct task *task)
{
    struct task_work *work = task->work;

    GLOW_PHASE(work) += GLOW_STEP;
    if (work->collision->flags & COLLISION_HIT) {
        work->collision->flags &= ~COLLISION_HIT;
    }
}

/* while the lamp is in view, draw it and then its glow. the glow faces the
 * camera and draws without fog. its color, a gray of half the sine of its
 * phase, adds to what lies behind it */
static void corner_light_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB material;
    NJS_POINT3 glow = {0.0F, GLOW_HEIGHT, 0.0F};
    Float gray;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                      CORNER_LIGHT_SIZE)) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_model(&corner_light_model);
    njPopMatrix(1);
    material.a = -1.0F;
    gray = njSin((Angle)(GLOW_PHASE(work) * ANGLE_PER_DEGREE)) * 0.5F;
    material.r = material.g = material.b = gray;
    njSetConstantMaterial(&material);
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njCalcPoint(NULL, &glow, &glow);
    njPopMatrix(1);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&corner_light_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x + glow.x, work->pos.y + glow.y,
                work->pos.z + glow.z);
    TURN(njRotateY, camera_work->ang.y);
    TURN(njRotateX, camera_work->ang.x);
    njFogDisable();
    njDrawSprite3D(&corner_light_sprite, 0,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_VFLIP | NJD_SPRITE_COLOR);
    njFogEnable();
    njPopMatrix(1);
    material.a = 0.0F;
    material.r = material.g = material.b = 0.0F;
    njSetConstantMaterial(&material);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
}

static void corner_light_destroy(struct task *task)
{
}

static void corner_light_sprite_init(void)
{
    corner_light_sprite.tlist = &corner_light_texlist;
    corner_light_sprite.tanim = corner_light_texanim;
    corner_light_sprite.sx = GLOW_SCALE;
    corner_light_sprite.sy = GLOW_SCALE;
    corner_light_sprite.p.x = 0.0F;
    corner_light_sprite.p.y = 0.0F;
    corner_light_sprite.p.z = 0.0F;
}
