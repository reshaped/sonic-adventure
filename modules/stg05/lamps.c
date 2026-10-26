/* O LAMP1 and O LAMP2, lamps whose glow faces the camera and flickers
 *
 * a set entry's scale x, y and z are the glow's red, green and blue. a scale
 * of all zeros takes 0.7, 0.3 and 0. its x angle is ten times the glow's
 * size, and an x angle of 0 takes 32. the set editor labels the x angle
 * LIGHT SCALE * 10 */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg05/lamps.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT mountain_lamp_1_object;
extern NJS_OBJECT mountain_lamp_2_object;

/* effect/glare.c's texlist, EFF_REGULAR's panlight. glare.c defines it
 * static, but 1ST_READ.BIN's symbols give its address */
extern NJS_TEXLIST glare_texlist;

/* TASK_TIMER(work) counts the draws into the flicker, from 0 */
struct mountain_lamp_vars {
    NJS_OBJECT *object;
    Float dim; /* what the flicker takes off the glow's alpha */
    Float size; /* the glow's size, from the x angle */
};

#define VARS(work) ((struct mountain_lamp_vars *)&(work)->vars)

/* the values a set entry's zeros take */
#define LAMP_RED 0.7F
#define LAMP_GREEN 0.3F
#define LAMP_BLUE 0.0F
#define LAMP_ANGLE_X 32

#define SIZE_PER_ANGLE 0.1F

/* each draw takes FLICKER_STEP more off the glow's alpha. every
 * FLICKER_DRAWS draws, the alpha starts over at GLOW_ALPHA */
#define GLOW_ALPHA 0.5F
#define FLICKER_STEP 0.05F
#define FLICKER_DRAWS 3

#define DRAW_RADIUS 100.0F

static void mountain_lamp_init(struct task *task);
static void mountain_lamp_run(struct task *task);
static void mountain_lamp_draw(struct task *task);
static void mountain_lamp_glow(NJS_POINT3 *pos, NJS_ARGB *color, Float size);

/* the glow's sprite and its frame, copies of glare.c's */
static NJS_TEXANIM mountain_lamp_glow_frame = {
#include "stg05/lamps/mountain_lamp_glow_frame.inc"
};
static NJS_SPRITE mountain_lamp_glow_sprite = {
#include "stg05/lamps/mountain_lamp_glow_sprite.inc"
};

static struct collision_shape mountain_lamp_shape = {
    SHAPE_SPHERE | SHAPE_UNTURNED,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    2.0F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

void mountain_lamp_1_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->object = &mountain_lamp_1_object;
    mountain_lamp_init(task);
}

void mountain_lamp_2_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->object = &mountain_lamp_2_object;
    mountain_lamp_init(task);
}

/* give the set entry's zeros their defaults, and size the glow */
static void mountain_lamp_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = 0;
    TASK_TIMER(work) = 0;
    if (work->scl.x == 0.0F && work->scl.y == 0.0F && work->scl.z == 0.0F) {
        work->scl.x = LAMP_RED;
        work->scl.y = LAMP_GREEN;
        work->scl.z = LAMP_BLUE;
    }
    if (work->ang.x == 0) {
        work->ang.x = LAMP_ANGLE_X;
    }
    VARS(work)->dim = 0.0F;
    VARS(work)->size = work->ang.x * SIZE_PER_ANGLE;
    if (!placement_editing(task)) {
        collision_init(task, &mountain_lamp_shape, COLLISION_OBJECT, 1, 1);
    }
    task->step = mountain_lamp_run;
    task->draw = mountain_lamp_draw;
}

static void mountain_lamp_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            collision_register(work);
        }
        mountain_lamp_draw(task);
        if (placement_editing(task)) {
            VARS(work)->size = work->ang.x * SIZE_PER_ANGLE;
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> LIGHT SCALE * 10");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "        default");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> RED   (0.7)");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> GREEN (0.3)");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> BLUE  (0.0)");
        }
    }
}

static void mountain_lamp_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;
    Angle ang;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        return;
    }
    color.a = GLOW_ALPHA;
    color.r = work->scl.x;
    color.g = work->scl.y;
    color.b = work->scl.z;
    color.a -= VARS(work)->dim;
    if (TASK_TIMER(work) >= FLICKER_DRAWS - 1) {
        TASK_TIMER(work) = 0;
        VARS(work)->dim = 0.0F;
    } else {
        TASK_TIMER(work)++;
        VARS(work)->dim += FLICKER_STEP;
    }
    mountain_lamp_glow(&work->pos, &color, VARS(work)->size);
    njSetTexture(&obj_mountain_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(VARS(work)->object);
    njPopMatrix(1);
}

/* draw the glow at pos as a sprite that faces the camera. f is the cosine
 * of the angle between the camera's heading and its line to pos, both
 * level. the glow's scale is 1.5 * size * (2f - 1). the scale is 0 for an f
 * below 0, with pos behind the camera. it is negative for an f from 0 to
 * 0.5 */
static void mountain_lamp_glow(NJS_POINT3 *pos, NJS_ARGB *color, Float size)
{
    NJS_POINT3 at;
    NJS_VECTOR to_camera;
    NJS_VECTOR back;
    Float facing;
    Float scale;
    Angle ang;
    NJS_POINT3 *eye = &camera_work->pos;

    to_camera.x = eye->x - pos->x;
    to_camera.y = 0.0F;
    to_camera.z = eye->z - pos->z;
    njUnitVector(&to_camera);
    back.x = njSin(camera_work->ang.y);
    back.y = 0.0F;
    back.z = njCos(camera_work->ang.y);
    facing = njInnerProduct(&to_camera, &back);
    if (facing < 0.0F) {
        facing = 0.0F;
    } else {
        facing = (facing - 0.5F) * 2.0F;
    }
    scale = size * facing * 1.5F;
    at.x = pos->x;
    at.y = pos->y;
    at.z = pos->z;
    draw_set_material(color->a, color->r, color->g, color->b);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &at);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    njScale(NULL, scale, scale, scale);
    njDrawSprite3D(&mountain_lamp_glow_sprite, 0,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}
