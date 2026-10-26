/* a collision record's shapes, drawn in the colors of their attacks */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/draw.h"
#include "system/task.h"
#include "system/util.h"

static void collision_draw_shape(struct collision_shape *shape,
                                 struct task_work *work);
static void collision_draw_sphere(struct collision_shape *shape,
                                  struct task_work *work);
static void collision_draw_cylinder(struct collision_shape *shape,
                                    struct task_work *work);
static void collision_draw_cylinder2(struct collision_shape *shape,
                                     struct task_work *work);
static void collision_draw_capsule(struct collision_shape *shape,
                                   struct task_work *work);
static void collision_draw_rectangle(struct collision_shape *shape,
                                     struct task_work *work);
static void collision_draw_rectangle2(struct collision_shape *shape,
                                      struct task_work *work);
static void collision_draw_wall(struct collision_shape *shape,
                                struct task_work *work);
static void collision_draw_place(struct collision_shape *shape,
                                 struct task_work *work);
static void collision_draw_model(NJS_MODEL *model,
                                 struct collision_shape *shape);

/* the constant material for each of SHAPE_ATTACK's levels, green at 0 to
 * red at 3, and the clear material each model leaves set. they're const,
 * and njSetConstantMaterial's parameter isn't, so it takes them through a
 * cast */
#define ATTACK_COLORS (SHAPE_ATTACK + 1)
static const NJS_ARGB collision_draw_colors[ATTACK_COLORS] = {
#include "collision/draw/collision_draw_colors.inc"
};
static const NJS_ARGB collision_draw_clear = {0.0F, 0.0F, 0.0F, 0.0F};

void collision_draw(struct collision_work *collision)
{
    int count;
    struct collision_shape *shapes;
    struct task *owner;
    struct task_work *work;
    struct collision_shape *shape;
    int i;

    if (collision == NULL) {
        return;
    }
    count = collision->count;
    shapes = collision->shapes;
    owner = collision->owner;
    if (count <= 0 || shapes == NULL || owner == NULL) {
        return;
    }
    work = owner->work;
    if (work == NULL) {
        return;
    }
    for (i = 0, shape = shapes; i < count; i++, shape++) {
        collision_draw_shape(shape, work);
    }
}

static void collision_draw_shape(struct collision_shape *shape,
                                 struct task_work *work)
{
    if (shape == NULL || work == NULL) {
        return;
    }
    switch (shape->flags & SHAPE_KIND) {
    case SHAPE_SPHERE:
        collision_draw_sphere(shape, work);
        break;
    case SHAPE_CYLINDER:
        collision_draw_cylinder(shape, work);
        break;
    case SHAPE_CYLINDER2:
        collision_draw_cylinder2(shape, work);
        break;
    case SHAPE_CAPSULE:
        collision_draw_capsule(shape, work);
        break;
    case SHAPE_RECTANGLE:
        collision_draw_rectangle(shape, work);
        break;
    case SHAPE_RECTANGLE2:
        collision_draw_rectangle2(shape, work);
        break;
    case SHAPE_WALL:
        collision_draw_wall(shape, work);
        break;
    }
}

/* @hack spelling: a marker's model read through its object's address, which
 * the image loads and reads 4 bytes past (MOV.L @(4,R4),R4 at 0x8c0a61d4);
 * written object.model, SHC loads the model field's own address, and 30 of
 * the 399 words of the four functions that use it differ */
#define MARKER_MODEL(object) ((&(object))->model)

static void collision_draw_sphere(struct collision_shape *shape,
                                  struct task_work *work)
{
    Float radius = shape->radius * MARKER_FIT;

    njPushMatrix(NULL);
    collision_draw_place(shape, work);
    njScale(NULL, radius, radius, radius);
    collision_draw_model(MARKER_MODEL(marker_object), shape);
    njPopMatrix(1);
}

/* a cylinder stays upright: when its task tilts, the task's turn is
 * undone */
static void collision_draw_cylinder(struct collision_shape *shape,
                                    struct task_work *work)
{
    Float radius;
    Float height;
    Uint32 flags;
    struct angles *ang;

    flags = shape->flags;
    radius = shape->radius * MARKER_FIT;
    height = shape->size_y * MARKER_FIT;
    ang = &work->ang;
    njPushMatrix(NULL);
    collision_draw_place(shape, work);
    if (!(flags & SHAPE_IN_WORLD) && !(flags & SHAPE_UNTURNED) &&
        (ang->x != 0 || ang->z != 0)) {
        if (flags & SHAPE_TURN_YZX) {
            TURN(njRotateX, -ang->x);
            TURN(njRotateZ, -ang->z);
            TURN(njRotateY, -ang->y);
        } else {
            TURN(njRotateY, -ang->y);
            TURN(njRotateX, -ang->x);
            TURN(njRotateZ, -ang->z);
        }
    }
    njScale(NULL, radius, height, radius);
    collision_draw_model(MARKER_MODEL(marker_cylinder_object), shape);
    njPopMatrix(1);
}

static void collision_draw_cylinder2(struct collision_shape *shape,
                                     struct task_work *work)
{
    Float radius;
    Float height;
    Uint32 flags;

    flags = shape->flags;
    radius = shape->radius * MARKER_FIT;
    height = shape->size_y * MARKER_FIT;
    njPushMatrix(NULL);
    collision_draw_place(shape, work);
    if (flags & SHAPE_TURN_YZX) {
        TURN(njRotateY, shape->ang.y);
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
    } else {
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
        TURN(njRotateY, shape->ang.y);
    }
    njScale(NULL, radius, height, radius);
    collision_draw_model(MARKER_MODEL(marker_cylinder_object), shape);
    njPopMatrix(1);
}

static void collision_draw_capsule(struct collision_shape *shape,
                                   struct task_work *work)
{
    NJS_MODEL *dome;
    Float radius;
    Float height;
    Float length;
    Float scale;
    Uint32 flags;

    flags = shape->flags;
    length = shape->size_y;
    /* @hack spelling: MARKER_FIT through scale, which keeps dome in R13
     * and njPushMatrix in R12 (0x8c0a6414, 0x8c0a6416); written at each use,
     * the two trade and 8 of 152 words differ */
    scale = MARKER_FIT;
    radius = shape->radius * scale;
    height = length * scale;
    dome = MARKER_MODEL(marker_dome_object);
    njPushMatrix(NULL);
    collision_draw_place(shape, work);
    if (flags & SHAPE_TURN_YZX) {
        TURN(njRotateY, shape->ang.y);
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
    } else {
        TURN(njRotateZ, shape->ang.z);
        TURN(njRotateX, shape->ang.x);
        TURN(njRotateY, shape->ang.y);
    }
    njPushMatrix(NULL);
    njScale(NULL, radius, height, radius);
    collision_draw_model(MARKER_MODEL(marker_tube_object), shape);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, length, 0.0F);
    njScale(NULL, radius, radius, radius);
    collision_draw_model(dome, shape);
    njPopMatrix(1);
    njRotateX(NULL, NJM_DEG_ANG(180));
    njTranslate(NULL, 0.0F, length, 0.0F);
    njScale(NULL, radius, radius, radius);
    collision_draw_model(dome, shape);
    njPopMatrix(1);
}

/* a box turns about y alone: when its task tilts, the task's turn is
 * undone but for y */
static void collision_draw_rectangle(struct collision_shape *shape,
                                     struct task_work *work)
{
    Uint32 flags;
    struct angles *ang;

    flags = shape->flags;
    ang = &work->ang;
    njPushMatrix(NULL);
    collision_draw_place(shape, work);
    if (!(flags & SHAPE_IN_WORLD) && !(flags & SHAPE_UNTURNED) &&
        (ang->x != 0 || ang->z != 0)) {
        if (flags & SHAPE_TURN_YZX) {
            TURN(njRotateX, -ang->x);
            TURN(njRotateZ, -ang->z);
        } else {
            TURN(njRotateY, -ang->y);
            TURN(njRotateX, -ang->x);
            TURN(njRotateZ, -ang->z);
            TURN(njRotateY, ang->y);
        }
    }
    TURN(njRotateY, shape->ang.y);
    njScale(NULL, shape->radius * CUBE_FIT, shape->size_y * CUBE_FIT,
            shape->size_z * CUBE_FIT);
    collision_draw_model(marker_cube_object->model, shape);
    njPopMatrix(1);
}

static void collision_draw_rectangle2(struct collision_shape *shape,
                                      struct task_work *work)
{
    Uint32 flags;
    struct angles *ang;

    flags = shape->flags;
    ang = &work->ang;
    njPushMatrix(NULL);
    collision_draw_place(shape, work);
    if (!(flags & SHAPE_IN_WORLD) && !(flags & SHAPE_UNTURNED) &&
        (ang->x != 0 || ang->z != 0)) {
        if (flags & SHAPE_TURN_YZX) {
            TURN(njRotateX, -ang->x);
            TURN(njRotateZ, -ang->z);
        } else {
            TURN(njRotateY, -ang->y);
            TURN(njRotateX, -ang->x);
            TURN(njRotateZ, -ang->z);
            TURN(njRotateY, ang->y);
        }
    }
    TURN(njRotateY, shape->ang.y);
    TURN(njRotateZ, shape->ang.z);
    njScale(NULL, shape->radius * CUBE_FIT, shape->size_y * CUBE_FIT,
            shape->size_z * CUBE_FIT);
    collision_draw_model(marker_cube_object->model, shape);
    njPopMatrix(1);
}

/* draw the wall's face as a flat box, with an arrow out of it along z, the
 * way the wall pushes */
static void collision_draw_wall(struct collision_shape *shape,
                                struct task_work *work)
{
    NJS_MODEL *cube;
    Uint32 flags;
    struct angles *ang;

    flags = shape->flags;
    ang = &work->ang;
    cube = marker_cube_object->model;
    njPushMatrix(NULL);
    collision_draw_place(shape, work);
    if (!(flags & SHAPE_IN_WORLD) && !(flags & SHAPE_UNTURNED) &&
        (ang->x != 0 || ang->z != 0)) {
        if (flags & SHAPE_TURN_YZX) {
            TURN(njRotateX, -ang->x);
            TURN(njRotateZ, -ang->z);
        } else {
            TURN(njRotateY, -ang->y);
            TURN(njRotateX, -ang->x);
            TURN(njRotateZ, -ang->z);
            TURN(njRotateY, ang->y);
        }
    }
    TURN(njRotateY, shape->ang.y);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 10.0F);
    njScale(NULL, 0.1F, 0.1F, 2.0F);
    collision_draw_model(cube, shape);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 20.0F);
    njRotateX(NULL, NJM_DEG_ANG(45));
    njTranslate(NULL, 0.0F, 0.0F, -3.0F);
    njScale(NULL, 0.1F, 0.1F, 0.7F);
    collision_draw_model(cube, shape);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 20.0F);
    njRotateX(NULL, NJM_DEG_ANG(-45));
    njTranslate(NULL, 0.0F, 0.0F, -3.0F);
    njScale(NULL, 0.1F, 0.1F, 0.7F);
    collision_draw_model(cube, shape);
    njPopMatrix(1);
    njScale(NULL, shape->radius * CUBE_FIT, shape->size_y * CUBE_FIT, 0.1F);
    collision_draw_model(cube, shape);
    njPopMatrix(1);
}

/* move the matrix to the shape's center, as collision_get_shape_center
 * finds it. the matrix keeps the task's turn unless the shape is
 * SHAPE_IN_WORLD or SHAPE_UNTURNED */
static void collision_draw_place(struct collision_shape *shape,
                                 struct task_work *work)
{
    Uint32 flags = shape->flags;
    struct angles *ang;

    if (!(flags & SHAPE_IN_WORLD)) {
        njTranslateV(NULL, &work->pos);
        if (!(flags & SHAPE_UNTURNED)) {
            ang = &work->ang;
            if (flags & SHAPE_TURN_YZX) {
                TURN(njRotateY, ang->y);
                TURN(njRotateZ, ang->z);
                TURN(njRotateX, ang->x);
            } else {
                TURN(njRotateZ, ang->z);
                TURN(njRotateX, ang->x);
                TURN(njRotateY, ang->y);
            }
        }
    }
    njTranslateV(NULL, &shape->center);
}

/* draw the model in the color of the shape's attack, then set the clear
 * material */
static void collision_draw_model(NJS_MODEL *model,
                                 struct collision_shape *shape)
{
    int attack;

    attack = shape->levels & SHAPE_ATTACK;
    if (attack < ATTACK_COLORS) {
        njSetConstantMaterial((NJS_ARGB *)&collision_draw_colors[attack]);
    }
    njSimpleDrawModel(model);
    njSetConstantMaterial((NJS_ARGB *)&collision_draw_clear);
}
